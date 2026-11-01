#pragma once
#include "GbaMixerTap.hpp"
#include <vector>
namespace ngmv {
// Independently decoded instruction/structure recognizer. No title IDs, fixed
// ROM offsets, copied routines, guest patches or waveform-source guessing.
struct NintendoVoicePool {uint32_t base=0;unsigned count=0;};
inline std::vector<NintendoVoicePool> findNintendoVoicePools(const std::vector<uint8_t>&rom){
    std::vector<NintendoVoicePool>out;auto half=[&](size_t a){return uint16_t(rom[a])|uint16_t(rom[a+1])<<8;};
    for(size_t at=8;at+8<=rom.size();at+=2){
        // Thumb cleanup loop: stride120, decrement counter, compare0, BGE.
        if(half(at)!=0x3578||half(at+2)!=0x3c01||half(at+4)!=0x2c00||(half(at+6)&0xff00)!=0xda00)continue;
        size_t begin=at>64?at-64:0;
        for(size_t a=begin;a+8<at;a+=2){if(half(a)!=0x2500||(half(a+2)&0xff00)!=0x2400||(half(a+4)&0xff00)!=0x4800||half(a+6)!=0x1829)continue;
            unsigned count=(half(a+2)&255)+1;if(count<1||count>16)continue;
            size_t literal=((a+8)&~size_t(3))+(half(a+4)&255)*4;if(literal+4>rom.size())continue;uint32_t base=GbaMixerTap::word(rom.data()+literal);unsigned region=base>>24,offset=base&0xffffff;
            if((base&3)||!((region==2&&offset<=0x40000-count*120)||(region==3&&offset<=0x8000-count*120)))continue;
            bool duplicate=false;for(auto p:out)if(p.base==base&&p.count==count)duplicate=true;if(!duplicate&&out.size()<8)out.push_back({base,count});
        }
    }return out;
}
struct NintendoMixerTap {
    std::vector<NintendoVoicePool>candidates;NintendoVoicePool selected{};int isolate=-1;uint64_t recognized=0;bool fault=false;uint32_t observed=0;
    struct Kernel {uint32_t pc=0;unsigned savedBytes=0;uint32_t op=0,prior=0,load=0;};std::vector<Kernel>kernels;
    explicit NintendoMixerTap(std::vector<NintendoVoicePool>p={},int voice=-1):candidates(std::move(p)),isolate(voice){}
    uint32_t filter(GBASystem*g,uint32_t address,uint32_t value,int width){
        if(candidates.empty()||width!=2||!g->armState)return value;uint32_t pc=g->reg[15].I-8;if((pc>>24)!=3)return value;
        auto code=GbaMixerTap::ram(g,pc-8,12);if(!code)return value;uint32_t op=GbaMixerTap::word(code+8),prior=GbaMixerTap::word(code+4),load=GbaMixerTap::word(code);
        for(auto&k:kernels)if(k.pc==pc&&(k.op!=op||k.prior!=prior||k.load!=load)){fault=true;return value;}
        // Postincrement STRH r12,[r1/r2],2, following signed halfword accumulation.
        int baseReg=(op>>16)&15;if((baseReg!=1&&baseReg!=2)||(op&0xfff0ffffu)!=0xe0c0c0b2u)return value;
        bool add=(prior==0xe08cc449u),mla=(prior==0xe02cc896u||prior==0xe02cc897u);
        if(!add&&!mla)return value;if(load!=(baseReg==1?0xe1d1c0f0u:0xe1d2c0f0u))return value;
        unsigned saved=0;for(auto&k:kernels)if(k.pc==pc){if(k.op!=op||k.prior!=prior||k.load!=load){fault=true;return value;}saved=k.savedBytes;break;}
        if(!saved){for(unsigned back=4;back<=512&&back<=pc;back+=4){auto p=GbaMixerTap::ram(g,pc-back,4);if(!p)break;uint32_t inst=GbaMixerTap::word(p);if((inst&0xffff0000u)==0xe92d0000u){if(inst==0xe92d03f0u&&add)saved=24;if(inst==0xe92d01f0u&&mla)saved=20;break;}}
            if(!saved)return value;if(kernels.size()>=64){fault=true;return value;}kernels.push_back({pc,saved,op,prior,load});}
        auto pointer=GbaMixerTap::ram(g,g->reg[13].I+saved+16,4);if(!pointer)return value;uint32_t context=GbaMixerTap::word(pointer);
        NintendoVoicePool pool=selected;if(!pool.count){for(auto p:candidates)if(context>=p.base&&context-p.base<p.count*120&&(context-p.base)%120==0){pool=p;break;}}
        if(!pool.count||context<pool.base||context-pool.base>=pool.count*120||(context-pool.base)%120){if(selected.count)fault=true;return value;}
        uint32_t left=g->reg[1].I,right=g->reg[2].I;if(right<=left||right-left<128||right-left>1408||((right-left)&1)||address!=g->reg[baseReg].I||!GbaMixerTap::ram(g,address,2))return value;
        selected=pool;unsigned voice=(context-pool.base)/120;recognized++;observed|=1u<<voice;
        if(isolate>=0&&voice!=unsigned(isolate)){auto old=GbaMixerTap::ram(g,address,2);return uint32_t(old[0])|uint32_t(old[1])<<8;}return value;
    }
};
}
