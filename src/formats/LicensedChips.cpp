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
#include "furnace-ay8930/ay8910.h"
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
    std::unique_ptr<ay8930_device> ay;uint32_t ayMask=0;uint8_t ayStereo=0x3f;
    std::array<double,3> ayPrevious{},ayDc{};double ayPole=0;
    double phase=0;Stereo held{},previous{};bool primed=false;
    ChipInterpolator esInterpolator;
    std::array<Stereo,33> esNativeVoices{},esOutputVoices{};
    std::array<ChipInterpolator,33> esVoiceInterpolators;
    std::array<unsigned,33> esHistoryActive{};
    bool esStemCapture=false;
    void pushEs(){
        esInterpolator.push(native());
        for(unsigned v=esStemCapture?0:32;v<33;v++){
            auto sample=esNativeVoices[v];esVoiceInterpolators[v].push(sample);
            if(sample.l!=0||sample.r!=0)esHistoryActive[v]=32;
            else if(esHistoryActive[v])--esHistoryActive[v];
        }
    }
    explicit State(ChipKind k,uint32_t c,uint8_t flags):kind(k),clock(c){
        if(k==ChipKind::Ay8930){ay=std::make_unique<ay8930_device>(c,bool(flags&16));ay->set_flags(flags);ay->device_start();ay->device_reset();ayPole=std::exp(-2*3.14159265358979323846*20/(double(c)/4));}
        else if(k==ChipKind::Es5506){valid(flags&128,"ES5505 is not implemented by the licensed ES5506 core.");otto.reset();}
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
    double rate(){if(kind==ChipKind::Ay8930)return double(clock)/4;if(kind==ChipKind::Es5506)return otto.rate(clock);if(kind==ChipKind::Oki)return function<uint32_t(*)(void*)>(RWF_SRATE|RWF_READ,DEVRW_VALUE)(chip);if(kind==ChipKind::Scc)return info.sampleRate;return InternalRate;}
    Stereo native(){
        if(kind==ChipKind::Ay8930){short output[3]{};ay->sound_stream_update(output,1);Stereo sum{};for(unsigned c=0;c<3;c++){double value=output[c]/32768.;ayDc[c]=value-ayPrevious[c]+ayPole*ayDc[c];ayPrevious[c]=value;if(!(ayMask&(1u<<c))){if(ayStereo&(1u<<(c*2)))sum.l+=float(ayDc[c]);if(ayStereo&(2u<<(c*2)))sum.r+=float(ayDc[c]);}}return sum;}
        if(kind==ChipKind::Es5506){
            valid(otto.serialBits()<=20,"ES5506 serial output precision above20bits is unsupported.");
            // Capture the prior voice frame BEFORE tick_perf advances voices.
            // Quantize cumulative bus contributions in original voice order,
            // matching hardware serial truncation, not raw debug preview taps.
            esNativeVoices.fill({});
            if(otto.serial_output_enabled()){
                int shift=20-otto.serialBits();int32_t buses[6][2]{};
                double scale=std::ldexp(1.,otto.serialBits()-1);
                for(unsigned v=0;v<32;v++){
                    auto bus=otto.observed_bus(v);if(bus>=6)continue;
                    int32_t beforeL=buses[bus][0]>>shift,beforeR=buses[bus][1]>>shift;
                    buses[bus][0]+=otto.observed_left(v);buses[bus][1]+=otto.observed_right(v);
                    esNativeVoices[v]={float(((buses[bus][0]>>shift)-beforeL)/scale),float(((buses[bus][1]>>shift)-beforeR)/scale)};
                }
            }
            otto.tick_perf();valid(!memory.failed,"ES5506 sample address outside supplied ROM.");
            // Use the configured hardware output buses, not debug voice taps.
            // tick_perf publishes the prior frame through W_ST/W_END precision,
            // routing, and output saturation, then advances the voices.
            int bits=otto.serialBits();
            double scale=bits>0?std::ldexp(1.,bits-1):1.;
            int64_t l=0,r=0;for(unsigned bus=0;bus<6;++bus){l+=otto.lout(bus);r+=otto.rout(bus);}
            Stereo total{};for(unsigned v=0;v<32;v++){total.l+=esNativeVoices[v].l;total.r+=esNativeVoices[v].r;}
            // Host writes to bus registers (if any) are real output but do not
            // belong to a voice. Keep them explicit rather than invent a voice.
            esNativeVoices[32]={float(l/scale)-total.l,float(r/scale)-total.r};
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
    case ChipKind::Ay8930:if(port==1)s.ayStereo=value;else{s.ay->address_w(reg);s.ay->data_w(value);}break;
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
void LicensedChip::mute(uint32_t mask){auto&s=*state_;if(s.kind==ChipKind::Ay8930)s.ayMask=mask;else if(s.kind==ChipKind::Es5506){for(unsigned v=0;v<32;++v)s.otto.set_mute(v,bool(mask&(1u<<v)));}else if(s.kind==ChipKind::Ym2612)nukedopn2_set_mute_mask(s.chip,mask);else if(s.kind==ChipKind::Opl2)opl_set_mute_mask(s.chip,mask);else s.info.devDef->SetMuteMask(s.chip,mask);}
Stereo LicensedChip::sample(){auto&s=*state_;double rate=s.rate();valid(rate>0&&rate<=(s.kind==ChipKind::Ay8930?5000000:1000000),"Invalid native chip rate.");
    if(rate==InternalRate){auto result=s.native();s.esOutputVoices=s.esNativeVoices;return result;}
    if(rate>InternalRate){
        // Integrate native sample cells over one output interval. Point
        // interpolation alone aliases when a chip generates >1 sample/frame.
        if(!s.primed){s.held=s.native();s.primed=true;}
        double left=rate/InternalRate,total=left,l=0,r=0;
        s.esOutputVoices.fill({});
        while(left>1e-12){
            if(s.phase>=1-1e-12){s.phase=0;s.held=s.native();}
            double weight=std::min(left,1-s.phase);
            l+=s.held.l*weight;r+=s.held.r*weight;
            if(s.kind==ChipKind::Es5506)for(unsigned v=s.esStemCapture?0:32;v<33;v++){s.esOutputVoices[v].l+=float(s.esNativeVoices[v].l*weight/total);s.esOutputVoices[v].r+=float(s.esNativeVoices[v].r*weight/total);}
            s.phase+=weight;left-=weight;
        }
        return {float(l/total),float(r/total)};
    }
    if(s.kind==ChipKind::Es5506){
        if(!s.primed){s.pushEs();s.primed=true;}
        s.phase+=rate/InternalRate;
        while(s.phase>=1){s.pushEs();s.phase-=1;}
        for(unsigned v=s.esStemCapture?0:32;v<33;v++)s.esOutputVoices[v]=s.esHistoryActive[v]?s.esVoiceInterpolators[v].at(s.phase):Stereo{};
        return s.esInterpolator.at(s.phase);
    }
    // Independent causal linear interpolation; at most one native-frame delay.
    // No render-block-dependent reset or borrowed resampler implementation.
    if(!s.primed){s.previous=s.held=s.native();s.primed=true;}
    s.phase+=rate/InternalRate;while(s.phase>=1){s.previous=s.held;s.held=s.native();s.phase-=1;}
    return {s.previous.l+float(s.phase)*(s.held.l-s.previous.l),s.previous.r+float(s.phase)*(s.held.r-s.previous.r)};
}
const std::array<Stereo,33>& LicensedChip::esVoices()const{return state_->esOutputVoices;}
void LicensedChip::captureEsVoices(){valid(!state_->primed,"Enable ES voice capture before rendering.");state_->esStemCapture=true;}
uint64_t LicensedChip::programmedEsVoices(){return state_->kind==ChipKind::Es5506?state_->otto.programmed_voice_mask():0;}
uint64_t LicensedChip::programmedAyVoices(){uint64_t used=0;if(state_->ay)for(unsigned c=0;c<3;c++)if(state_->ay->m_tone[c].volume)used|=uint64_t(1)<<c;return used;}
}
