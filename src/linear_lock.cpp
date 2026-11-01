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
#include <mkw/diag.hpp>
namespace mkw {std::atomic<unsigned> fileOpens{0};std::atomic<unsigned long long> fileOpenTicks{0};
std::atomic<unsigned long long> packReadBytes{0},packReadTicks{0};
// DIAG: the last opens (path, ticks, found), printed under each load stage.
OpenTrace openTrace;}
extern "C" FILE* __real_fopen(const char* path,const char* mode);
// Diagnostic overlay (mkw/diag.hpp): while a race is profiled, append-mode
// logs under sdmc:/3ds/mkw-3ds/ write into RAM through funopen and reach
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
extern "C" {
FILE* __wrap_fopen(const char* path,const char* mode){
 const u64 start=svcGetSystemTick();FILE* file=nullptr;
 if(mkw::diag::deferring.load(std::memory_order_relaxed)&&path&&mode&&mode[0]=='a'&&
    !std::strncmp(path,"sdmc:/3ds/mkw-3ds/",18)&&std::strncmp(path+18,"perf-",5))file=mkw::diag::deferredOpen(path);
 if(!file)file=__real_fopen(path,mode);
 const u64 ticks=svcGetSystemTick()-start;
 mkw::fileOpens.fetch_add(1,std::memory_order_relaxed);mkw::fileOpenTicks.fetch_add(ticks,std::memory_order_relaxed);
 if(path)mkw::openTrace.add(path,ticks,file!=nullptr);
 // DIAG: every failed open of a game asset (relative path) is logged, so a
 // missing loose file shows even outside the traced load stages.
 if(!file&&path&&!std::strchr(path,':'))if(FILE* log=__real_fopen("sdmc:/3ds/mkw-3ds/missing-opens.txt","a")){std::fprintf(log,"%s %s\n",path,mode?mode:"");std::fclose(log);}
 return file;
}
}
