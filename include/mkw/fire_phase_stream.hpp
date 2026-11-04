#pragma once
#include <cstdint>
#include <cstdio>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <initializer_list>
#include <mkw/asset_pack.hpp>
namespace mkw {
// Course-local cache. File I/O ends before open returns; read is memcpy only.
// Memory pressure reduces temporal samples, never resolution or loop speed.
class FirePhaseStream {
    unsigned char* cache_=nullptr;unsigned char* block_=nullptr;unsigned stride_=1;size_t bytes_=0;
    bool accept(const uint32_t* h){
        float scale,bias;std::memcpy(&scale,h+8,4);std::memcpy(&bias,h+9,4);
        if(std::memcmp(h,"FTAN",4)||h[1]!=1||h[2]!=256||h[3]!=256||h[4]!=5||h[5]!=100||h[6]!=131072||h[7]!=40)return false;
        if(!std::isfinite(scale)||!std::isfinite(bias)||std::fabs(scale-16.f/17.f)>=1e-7f||std::fabs(bias-1.f/34.f)>=1e-7f)return false;
        width=h[2];height=h[3];frames=h[5];frameBytes=h[6];uvScale=scale;uvBias=bias;return true;
    }
public:
    uint32_t width=0,height=0,frames=0,frameBytes=0;
    float uvScale=0,uvBias=0;
    FirePhaseStream()=default;
    FirePhaseStream(const FirePhaseStream&)=delete;
    FirePhaseStream& operator=(const FirePhaseStream&)=delete;
    ~FirePhaseStream(){std::free(block_);}
    unsigned phase(unsigned tick)const{return (tick%frames)/stride_*stride_;}
    unsigned stride()const{return stride_;}
    size_t bytes()const{return bytes_;}
    bool open(const char* path,size_t budget=13107200){
        if(cache_)return false;
        // A repacked race.pak (tools/pakpack.c) holds the stream compressed:
        // read it whole when every frame fits, header included.
        constexpr long fileBytes=40+100*131072;
        if(pack::size(path)==fileBytes&&size_t(fileBytes-40)<=budget){
            if(auto* whole=static_cast<unsigned char*>(std::malloc(fileBytes))){
                uint32_t h[10];
                if(pack::readInto(path,whole,fileBytes)&&(std::memcpy(h,whole,sizeof h),accept(h))){
                    block_=whole;cache_=whole+40;stride_=1;bytes_=size_t(fileBytes-40);return true;
                }
                std::free(whole);
            }
        }
        FILE* f=std::fopen(path,"rb");if(!f)return false;
        uint32_t h[10]{};
        bool ok=std::fread(h,1,sizeof h,f)==sizeof h&&accept(h);
        if(ok)ok=std::fseek(f,0,SEEK_END)==0&&std::ftell(f)==long(40+h[5]*h[6]);
        if(!ok){std::fclose(f);return false;}
        for(unsigned step:{1u,2u,4u,10u,20u,100u}){
            const size_t bytes=size_t(frames/step)*frameBytes;
            if(bytes>budget)continue;
            cache_=block_=static_cast<unsigned char*>(std::malloc(bytes));
            if(cache_){stride_=step;bytes_=bytes;break;}
        }
        if(!cache_){std::fclose(f);return false;}
        for(unsigned i=0;i<frames/stride_;++i){
            if(std::fseek(f,long(40+size_t(i)*stride_*frameBytes),SEEK_SET)!=0||
               std::fread(cache_+size_t(i)*frameBytes,1,frameBytes,f)!=frameBytes){
                std::fclose(f);std::free(block_);block_=cache_=nullptr;bytes_=0;return false;
            }
        }
        std::fclose(f);return true;
    }
    bool read(unsigned phase,void* target,size_t capacity){
        if(!cache_||!target||phase>=frames||capacity<frameBytes)return false;
        std::memcpy(target,cache_+size_t(phase/stride_)*frameBytes,frameBytes);return true;
    }
    void mapUv(float* uv)const{
        for(unsigned i=0;i<6;++i)uv[i]*=uvScale;
        uv[2]+=uvBias;uv[5]+=uvBias;
    }
};
}
