#include <3ds.h>
#include <cstddef>

// libctru's LINEAR heap allocator is not thread-safe. The menu asset loader
// creates textures and model buffers on a background thread, so every LINEAR
// allocation in the program (including Citro3D's) is routed through one lock
// with -Wl,--wrap=linear*. A zero-initialized RecursiveLock is a valid,
// unlocked lock, so this is usable before static constructors run.
extern "C" {
void* __real_linearAlloc(size_t size);
void* __real_linearMemAlign(size_t size,size_t alignment);
void* __real_linearRealloc(void* mem,size_t size);
size_t __real_linearGetSize(void* mem);
void __real_linearFree(void* mem);
u32 __real_linearSpaceFree(void);
}
namespace {
RecursiveLock linearLock;
struct Guard {
 Guard(){RecursiveLock_Lock(&linearLock);}
 ~Guard(){RecursiveLock_Unlock(&linearLock);}
};
}
extern "C" {
void* __wrap_linearAlloc(size_t size){Guard g;return __real_linearAlloc(size);}
void* __wrap_linearMemAlign(size_t size,size_t alignment){Guard g;return __real_linearMemAlign(size,alignment);}
void* __wrap_linearRealloc(void* mem,size_t size){Guard g;return __real_linearRealloc(mem,size);}
size_t __wrap_linearGetSize(void* mem){Guard g;return __real_linearGetSize(mem);}
void __wrap_linearFree(void* mem){Guard g;__real_linearFree(mem);}
u32 __wrap_linearSpaceFree(void){Guard g;return __real_linearSpaceFree();}
}

// QA: counts file opens and the time spent opening (fopen is wrapped at link).
#include <cstdio>
#include <cstring>
#include <atomic>
#include <string>
#include <mkw/asset_pack.hpp>
#include <mkw/diag.hpp>
namespace mkw {std::atomic<unsigned> fileOpens{0};std::atomic<unsigned long long> fileOpenTicks{0};
std::atomic<unsigned> packReads{0};std::atomic<unsigned long long> packReadBytes{0},packReadTicks{0};
// DIAG: the last opens (path, ticks, found), printed under each load stage.
OpenTrace openTrace;}
extern "C" FILE* __real_fopen(const char* path,const char* mode);
// Diagnostic overlay (mkw/diag.hpp): while a race is profiled, append-mode
// logs under sdmc:/3ds/MKWiiDS/ write into RAM through funopen and reach
// the SD card when profiling stops. Bounded: later bytes are counted, not kept.
namespace mkw::diag {
std::atomic<unsigned> deferredOpens{0};
namespace {
struct Log {char path[64];std::string text;};
constexpr unsigned MaxLogs=16;constexpr std::size_t MaxBytes=256*1024;
Log logs[MaxLogs];unsigned logCount=0;std::size_t logBytes=0,droppedBytes=0;
RecursiveLock logLock; // zero-initialized: valid before static constructors
std::atomic<bool> deferring{false};
int logWrite(void* cookie,const char* data,size_t size){
 RecursiveLock_Lock(&logLock);
 if(logBytes+size<=MaxBytes){logs[reinterpret_cast<uintptr_t>(cookie)].text.append(data,size);logBytes+=size;}else droppedBytes+=size;
 RecursiveLock_Unlock(&logLock);
 return size;
}
int logClose(void*){return 0;}
FILE* deferredOpen(const char* path){
 RecursiveLock_Lock(&logLock);
 unsigned i=0;while(i<logCount&&std::strcmp(logs[i].path,path))++i;
 if(i==logCount){
  if(logCount==MaxLogs||std::strlen(path)>=sizeof logs[0].path){RecursiveLock_Unlock(&logLock);return nullptr;}
  std::strcpy(logs[logCount++].path,path);
 }
 RecursiveLock_Unlock(&logLock);
 // Write-only stream, no seek: bytes append to the RAM copy.
 FILE* f=funopen(reinterpret_cast<void*>(uintptr_t(i)),nullptr,logWrite,nullptr,logClose);
 if(f)deferredOpens.fetch_add(1,std::memory_order_relaxed);
 return f;
}
}
void deferSdLogs(bool on){
 if(on){deferring.store(true,std::memory_order_relaxed);return;}
 if(!deferring.exchange(false))return;
 RecursiveLock_Lock(&logLock);
 for(unsigned i=0;i<logCount;++i){
  auto& log=logs[i];if(log.text.empty())continue;
  if(FILE* f=__real_fopen(log.path,"a")){std::fwrite(log.text.data(),1,log.text.size(),f);
   if(droppedBytes&&!std::strcmp(std::strrchr(log.path,'/'),"/runtime.log"))std::fprintf(f,"diag: %u deferred log bytes dropped\n",unsigned(droppedBytes));
   std::fclose(f);}
  log.text.clear();log.text.shrink_to_fit();
 }
 logBytes=0;droppedBytes=0;
 RecursiveLock_Unlock(&logLock);
}
unsigned deferredLogBytes(){RecursiveLock_Lock(&logLock);const unsigned n=unsigned(logBytes);RecursiveLock_Unlock(&logLock);return n;}
}
#ifndef MKW_RELEASE
namespace {
// DIAG (cfl-fallback-test.txt): hide the packed Mii resource so cfl_mii.c
// takes its fallback, the console's own CFL system archive (title
// 0004009B00010202); cfl-fallback.txt records each step.
bool cflFallbackTest(){
 static const int on=[]{FILE* f=__real_fopen("sdmc:/3ds/MKWiiDS/cfl-fallback-test.txt","r");if(f)std::fclose(f);return f?1:0;}();
 return on;
}
void cflFallbackLog(const char* text){
 if(FILE* f=__real_fopen("sdmc:/3ds/MKWiiDS/cfl-fallback.txt","a")){std::fputs(text,f);std::fclose(f);}
}
}
#endif
extern "C" {
#ifdef MKW_WRAP_ROMFS_MOUNT
Result __real_romfsMountFromTitle(u64 tid,FS_MediaType mediatype,const char* name);
Result __wrap_romfsMountFromTitle(u64 tid,FS_MediaType mediatype,const char* name){
 const Result rc=__real_romfsMountFromTitle(tid,mediatype,name);
 char line[128];std::snprintf(line,sizeof line,"romfsMountFromTitle(%016llX, media %d, %s) = 0x%08lX (%s)\n",
  (unsigned long long)tid,int(mediatype),name?name:"",(unsigned long)rc,R_SUCCEEDED(rc)?"mounted":"failed");
 cflFallbackLog(line);
 return rc;
}
#endif
FILE* __wrap_fopen(const char* path,const char* mode){
#ifdef MKW_PGO_GEN
 // devkitARM's Windows GCC names each counter file "<-fprofile-generate dir>\<host
 // object path>.gcda" with backslashes and the drive: write it flat instead, as
 // pgo/<path below objects, backslash -> '#'>.gcda (tools/diag/compile.py MKW_PGO=use reads it back).
 static constexpr char prefix[]="sdmc:/3ds/MKWiiDS/pgo\\";
 if(path&&!std::strncmp(path,prefix,sizeof prefix-1)){
  const char* rest=std::strstr(path,"objects\\");rest=rest?rest+8:path+sizeof prefix-1;
  char flat[256];int n=std::snprintf(flat,sizeof flat,"sdmc:/3ds/MKWiiDS/pgo/");
  for(;*rest&&n<int(sizeof flat)-1;++rest)flat[n++]=*rest=='\\'||*rest=='/'||*rest==':'?'#':*rest;
  flat[n]=0;return __real_fopen(flat,mode);
 }
#endif
#ifndef MKW_RELEASE
 if(path&&cflFallbackTest()){
  if(!std::strcmp(path,"mii/CFL_Res.dat")){cflFallbackLog("test: packed mii/CFL_Res.dat hidden\n");return nullptr;}
  if(!std::strncmp(path,"$CFLRES:",8)){
   FILE* f=__real_fopen(path,mode);long size=-1;
   if(f){std::fseek(f,0,SEEK_END);size=std::ftell(f);std::rewind(f);}
   char line[160];std::snprintf(line,sizeof line,"open %s: %s, %ld bytes\n",path,f?"ok":"FAILED",size);cflFallbackLog(line);
   return f;
  }
 }
#endif
 const u64 start=svcGetSystemTick();FILE* file=nullptr;
 if(mkw::diag::deferring.load(std::memory_order_relaxed)&&path&&mode&&mode[0]=='a'&&
    !std::strncmp(path,"sdmc:/3ds/MKWiiDS/",18)&&std::strncmp(path+18,"perf-",5))file=mkw::diag::deferredOpen(path);
 // Game files (relative paths, read-only) come from the packs first; a file
 // no pack holds is opened loose, as are older SD folders' files.
 if(!file&&path&&mode&&mode[0]=='r'&&!std::strchr(mode,'+')&&!std::strchr(path,':'))file=mkw::pack::openFile(path);
 if(!file)file=__real_fopen(path,mode);
 const u64 ticks=svcGetSystemTick()-start;
 mkw::fileOpens.fetch_add(1,std::memory_order_relaxed);mkw::fileOpenTicks.fetch_add(ticks,std::memory_order_relaxed);
 if(path)mkw::openTrace.add(path,ticks,file!=nullptr);
 // DIAG: every failed open of a game asset (relative path) is logged, so a
 // missing loose file shows even outside the traced load stages.
 if(!file&&path&&!std::strchr(path,':'))if(FILE* log=__real_fopen("sdmc:/3ds/MKWiiDS/missing-opens.txt","a")){std::fprintf(log,"%s %s\n",path,mode?mode:"");std::fclose(log);}
 return file;
}
}
