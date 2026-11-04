// SPDX-License-Identifier: GPL-2.0-or-later
// Independent app adapter. Chip implementations retain their own notices.
#include "LicensedChips.hpp"
#include "audio/ChipInterpolator.hpp"
extern "C" {
#include "licensed-chips/emu/cores/fmopl.h"
#include "licensed-chips/emu/cores/ym3438.h"
#include "licensed-chips/emu/cores/okim6295.h"
#include "licensed-chips/emu/cores/k051649.h"
}
#include "vgsound-es5506/src/es550x/es5506.hpp"
#include <array>
#include <vector>
#include <stdexcept>
#include <algorithm>
#include <cmath>
namespace ngmv {
namespace {
void valid(bool b,const char*s){if(!b)throw std::runtime_error(s);}
struct SampleMemory:es550x_intf {
    std::array<std::vector<uint16_t>,4> banks;
    bool failed=false;
    int16_t read_sample(uint8_t bank,uint32_t offset) override {
        if(bank>=4 || offset>=banks[bank].size()){failed=true;return 0;}
        return int16_t(banks[bank][offset]);
    }
};
struct Otto:es5506_core {
    using es5506_core::es5506_core;
    double rate(uint32_t clock)const{return double(clock)/(16*(std::clamp(uint32_t(m_active),4u,31u)+1));}
    int serialBits()const{return serial_output_bits();}
};
}
struct LicensedChip::State {
    ChipKind kind;uint32_t clock;void*chip=nullptr;DEV_INFO info{};
    SampleMemory memory;Otto otto{memory};
    double phase=0;Stereo held{},previous{};bool primed=false;
    ChipInterpolator esInterpolator;
    explicit State(ChipKind k,uint32_t c,uint8_t flags):kind(k),clock(c){
        if(k==ChipKind::Es5506){valid(flags&128,"ES5505 is not implemented by the licensed ES5506 core.");otto.reset();}
        else if(k==ChipKind::Ym2612){chip=nukedopn2_init(c,InternalRate);valid(chip,"YM2612 allocation failed.");nukedopn2_set_options(chip,0);nukedopn2_reset_chip(chip);}
        else if(k==ChipKind::Opl2){chip=ym3812_init(c,InternalRate);valid(chip,"OPL2 allocation failed.");ym3812_reset_chip(chip);}
        else {
            const auto*d=k==ChipKind::Oki?sndDev_MSM6295.cores[0]:sndDev_K051649.cores[0];
            DEV_GEN_CFG cfg{};cfg.clock=c;cfg.smplRate=InternalRate;cfg.srMode=1;cfg.flags=flags;
            valid(!d->Start(&cfg,&info)&&info.dataPtr,"Chip allocation failed.");chip=info.dataPtr;d->Reset(chip);
        }
    }
    ~State(){if(!chip)return;if(kind==ChipKind::Ym2612)nukedopn2_shutdown(chip);else if(kind==ChipKind::Opl2)ym3812_shutdown(chip);else info.devDef->Stop(chip);}
    template<class F> F function(unsigned type,unsigned rw){
        for(auto*p=info.devDef->rwFuncs;p->funcPtr;++p)if(p->funcType==type&&p->rwType==rw)return reinterpret_cast<F>(p->funcPtr);
        throw std::runtime_error("Required licensed-chip operation unavailable.");
    }
    double rate(){if(kind==ChipKind::Es5506)return otto.rate(clock);if(kind==ChipKind::Oki)return function<uint32_t(*)(void*)>(RWF_SRATE|RWF_READ,DEVRW_VALUE)(chip);if(kind==ChipKind::Scc)return info.sampleRate;return InternalRate;}
    Stereo native(){
        if(kind==ChipKind::Es5506){
            valid(otto.serialBits()<=20,"ES5506 serial output precision above20bits is unsupported.");
            otto.tick_perf();valid(!memory.failed,"ES5506 sample address outside supplied ROM.");
            // Use the configured hardware output buses, not debug voice taps.
            // tick_perf publishes the prior frame through W_ST/W_END precision,
            // routing, and output saturation, then advances the voices.
            int bits=otto.serialBits();
            double scale=bits>0?std::ldexp(1.,bits-1):1.;
            int64_t l=0,r=0;for(unsigned bus=0;bus<6;++bus){l+=otto.lout(bus);r+=otto.rout(bus);}
            return {float(l/scale),float(r/scale)};
        }
        int32_t left=0,right=0;int32_t*planes[]={&left,&right};
        if(kind==ChipKind::Ym2612)nukedopn2_update(chip,1,planes);
        else if(kind==ChipKind::Opl2)ym3812_update_one(chip,1,planes);
        else info.devDef->Update(chip,1,planes);
        return {left/32768.f,right/32768.f};
    }
};
LicensedChip::LicensedChip(ChipKind k,uint32_t c,uint8_t flags):state_(std::make_unique<State>(k,c,flags)){}
LicensedChip::~LicensedChip()=default;
void LicensedChip::write(uint8_t port,uint8_t reg,uint8_t value){
    auto&s=*state_;
    switch(s.kind){
    case ChipKind::Ym2612:nukedopn2_write(s.chip,port*2,reg);nukedopn2_write(s.chip,port*2+1,value);break;
    case ChipKind::Opl2:ym3812_write(s.chip,0,reg);ym3812_write(s.chip,1,value);break;
    case ChipKind::Oki:s.function<void(*)(void*,uint8_t,uint8_t)>(RWF_REGISTER|RWF_WRITE,DEVRW_A8D8)(s.chip,reg,value);break;
    case ChipKind::Scc:{auto f=s.function<void(*)(void*,uint8_t,uint8_t)>(RWF_REGISTER|RWF_WRITE,DEVRW_A8D8);f(s.chip,port*2,reg);f(s.chip,port*2+1,value);break;}
    case ChipKind::Es5506:s.otto.host_w(reg,value);break;
    }
}
void LicensedChip::rom(uint32_t size,uint32_t offset,const uint8_t*data,size_t length){
    valid(size && size<=64*1024*1024,"Invalid sample ROM size.");auto&s=*state_;
    if(s.kind==ChipKind::Oki){valid(offset<=size&&length<=size-offset,"Invalid OKI ROM range.");s.function<void(*)(void*,uint32_t)>(RWF_MEMORY|RWF_WRITE,DEVRW_MEMSIZE)(s.chip,size);s.function<void(*)(void*,uint32_t,uint32_t,const uint8_t*)>(RWF_MEMORY|RWF_WRITE,DEVRW_BLOCK)(s.chip,offset,uint32_t(length),data);}
    else if(s.kind==ChipKind::Es5506){
        unsigned bank=(offset>>28)&3;bool bytes=bool(offset&0x80000000u);offset&=0x0fffffff;
        valid(offset<=size&&length<=size-offset,"Invalid ES5506 ROM range.");
        valid(bytes || (!(offset&1)&&!(length&1)&&!(size&1)),"Unaligned ES5506 16-bit ROM.");
        auto&dst=s.memory.banks[bank];size_t words=bytes?size:size/2;
        if(dst.size()!=words)dst.assign(words,0);
        for(size_t i=0;i<(bytes?length:length/2);++i)
            dst[(bytes?offset:offset/2)+i]=bytes?uint16_t(data[i])<<8:uint16_t(data[2*i])|uint16_t(data[2*i+1])<<8;
    }else throw std::runtime_error("ROM data targets an incompatible chip.");
}
void LicensedChip::mute(uint32_t mask){auto&s=*state_;if(s.kind==ChipKind::Es5506){for(unsigned v=0;v<32;++v)s.otto.set_mute(v,bool(mask&(1u<<v)));}else if(s.kind==ChipKind::Ym2612)nukedopn2_set_mute_mask(s.chip,mask);else if(s.kind==ChipKind::Opl2)opl_set_mute_mask(s.chip,mask);else s.info.devDef->SetMuteMask(s.chip,mask);}
Stereo LicensedChip::sample(){auto&s=*state_;double rate=s.rate();valid(rate>0&&rate<=1000000,"Invalid native chip rate.");
    if(rate==InternalRate)return s.native();
    if(rate>InternalRate){
        // Integrate native sample cells over one output interval. Point
        // interpolation alone aliases when a chip generates >1 sample/frame.
        if(!s.primed){s.held=s.native();s.primed=true;}
        double left=rate/InternalRate,total=left,l=0,r=0;
        while(left>1e-12){
            if(s.phase>=1-1e-12){s.phase=0;s.held=s.native();}
            double weight=std::min(left,1-s.phase);
            l+=s.held.l*weight;r+=s.held.r*weight;
            s.phase+=weight;left-=weight;
        }
        return {float(l/total),float(r/total)};
    }
    if(s.kind==ChipKind::Es5506){
        if(!s.primed){s.esInterpolator.push(s.native());s.primed=true;}
        s.phase+=rate/InternalRate;
        while(s.phase>=1){s.esInterpolator.push(s.native());s.phase-=1;}
        return s.esInterpolator.at(s.phase);
    }
    // Independent causal linear interpolation; at most one native-frame delay.
    // No render-block-dependent reset or borrowed resampler implementation.
    if(!s.primed){s.previous=s.held=s.native();s.primed=true;}
    s.phase+=rate/InternalRate;while(s.phase>=1){s.previous=s.held;s.held=s.native();s.phase-=1;}
    return {s.previous.l+float(s.phase)*(s.held.l-s.previous.l),s.previous.r+float(s.phase)*(s.held.r-s.previous.r)};
}
}
