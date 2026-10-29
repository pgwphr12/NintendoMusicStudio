#pragma once
#include "GbaVoiceCapture.hpp"
namespace ngmv {
class GbaMixCapture {
    static constexpr size_t Capacity=8192;
    std::array<GbaMixFrame,Capacity>frames_{};
    size_t used_=0;
    uint64_t lastCycles_=0;
    uint64_t lockEpoch_=0;
    uint64_t lockCycles_=0;
    bool psgNoteSeen_=false;
    enum class Fault {None,Overflow,Metadata,Generation,Clock}fault_=Fault::None;
    static uint32_t word(const uint8_t*p){return uint32_t(p[0])|uint32_t(p[1])<<8|uint32_t(p[2])<<16|uint32_t(p[3])<<24;}
public:
    bool psgNoteSeen()const{return psgNoteSeen_;}
    template<class Read>void onWrite(Read read,const GbaVoiceCapture&notes,uint32_t info,int voices,
        uint32_t registerChannel,uint32_t address,uint32_t value,int width,uint64_t cycles,
        uint32_t timerPeriod,uint16_t soundControl){
        if(fault_!=Fault::None||voices<1||voices>12||info>0xffffffffu-0x350u||(width!=1&&width!=2&&width!=4))return;
        // Standard m4a CGB channels share the64-byte stride. Keep even muted
        // sequencer STARTs unsupported, independently of audio/DAC residue.
        if(auto pointer=read(info+28u,size_t(4))){uint32_t cgb=word(pointer);unsigned region=cgb>>24;
            if((region==2||region==3)&&address>=cgb&&address-cgb<256u&&((address-cgb)%64u)==0){
                int c=int((address-cgb)/64u);auto channel=read(cgb+uint32_t(c)*64u,size_t(64));
                if(channel){uint8_t type=width>1?uint8_t(value>>8):channel[1];if(type==c+1&&(value&0x80u))psgNoteSeen_=true;}
            }
        }
        if(address==info&&width==4&&value==0x68736d54u){auto prior=read(info,size_t(4));if(prior&&word(prior)==0x68736d53u){if(++lockEpoch_==0)fault_=Fault::Metadata;lockCycles_=cycles;}return;}
        uint32_t base=info+0x50u;if(address<base||address-base>=uint32_t(voices)*64u)return;
        uint32_t relative=address-base;int slot=int(relative/64u),offset=int(relative%64u);
        // A completed left-gain write while R4 is THIS channel. Ordinary
        // channel volume writes (offset2/3) are not mixer-state observations.
        if(offset+width>64||offset>11||offset+width<=11||registerChannel!=base+uint32_t(slot)*64u)return;
        auto sound=read(info,size_t(32));if(!sound||word(sound)!=0x68736d54u)return;
        auto raw=read(registerChannel,size_t(64));if(!raw){fault_=Fault::Metadata;return;}
        std::array<uint8_t,64>copy;std::copy_n(raw,64,copy.begin());
        for(int i=0;i<width;i++)copy[size_t(offset+i)]=uint8_t(value>>(i*8));
        // SoundMain consumes START before computing gains. Initialization,
        // inactive cleanup and a merely scheduled start are not a mix pass.
        if(!(copy[0]&0x47u)||(copy[0]&0x80u))return;
        auto prospective=[&](uint32_t p,size_t n)->const uint8_t*{if(p==registerChannel&&n==64)return copy.data();return read(p,n);};
        GbaMixFrame frame;
        if(!GbaVoiceCapture::decode(prospective,registerChannel,slot,frame.voice)){fault_=Fault::Metadata;return;}
        if(!notes.activeGeneration(slot,frame.voice.generation)){fault_=Fault::Generation;return;}
        frame.voice.kind=GbaVoiceEventKind::MixerState;frame.voice.cpuCycles=cycles;frame.lockEpoch=lockEpoch_;frame.lockCycles=lockCycles_;
        frame.samplesPerVblank=word(sound+16);frame.pcmRate=word(sound+20);frame.divFreq=word(sound+24);
        frame.masterVolume=sound[7];frame.reverb=sound[5];frame.gainRight=copy[10];frame.gainLeft=copy[11];
        frame.sourceRemaining=word(copy.data()+24);frame.sourceFraction=word(copy.data()+28);frame.sourcePointer=word(copy.data()+40);
        frame.timer0Period=timerPeriod;frame.soundControl=soundControl;
        if(cycles<lastCycles_){fault_=Fault::Clock;return;}
        if(used_==Capacity){fault_=Fault::Overflow;return;}
        lastCycles_=cycles;frames_[used_++]=std::move(frame);
    }
    bool drain(std::vector<GbaMixFrame>&out,std::string&error){
        if(fault_!=Fault::None){error=fault_==Fault::Overflow?"GBA mixer-state queue overflow":fault_==Fault::Clock?"GBA mixer clock moved backwards":fault_==Fault::Generation?"GBA mixer has no matching original note generation":"Malformed GBA mixer metadata";return false;}
        std::vector<GbaMixFrame>candidate(frames_.begin(),frames_.begin()+used_);out=std::move(candidate);used_=0;error.clear();return true;
    }
};
struct GbaMixPhase {uint32_t offset=0,fraction=0,remaining=0;bool ended=false;};
// Independent bounded phase prediction for ordinary forward PCM8 only. No
// game instructions/tables copied; unknown/reverse/compressed paths rejected.
inline bool predictGbaMixPhase(const GbaMixFrame&f,GbaMixPhase&out){
    constexpr uint32_t Unit=1u<<23;const auto&s=f.voice.sample;
    if(!s.readable||!s.pcm8||s.type!=0||(s.status&~0x4000u)||!s.rawFrequency||s.looped!=bool(s.status&0x4000u)||(f.voice.type&~8u)||!s.length||s.length>32u*1024u*1024u||
       s.address>0xffffffffu-16u||s.length>0xffffffffu-s.address-16u||f.sourcePointer<s.address+16u||
       f.sourceFraction>=Unit||!f.samplesPerVblank||f.samplesPerVblank>704||
       !f.divFreq||(s.looped&&s.loopStart>=s.length))return false;
    uint32_t offset=f.sourcePointer-(s.address+16u);
    if(offset>=s.length||f.sourceRemaining!=s.length-offset)return false;
    uint64_t increment=(f.voice.type&8u)?Unit:uint64_t(f.voice.frequency)*f.divFreq;
    // Out-of-range wrapped ARM multiplication/addition is not interpreted as
    // a physical rate. Preserve raw frame, return unsupported, never guess.
    if(increment>0xffffffffu-(Unit-1))return false;
    uint64_t phase=f.sourceFraction+increment*f.samplesPerVblank;
    uint64_t position=offset+(phase>>23);GbaMixPhase result;result.fraction=uint32_t(phase&(Unit-1));
    if(position>=s.length){if(!s.looped){result.offset=s.length;result.ended=true;out=result;return true;}position=s.loopStart+(position-s.loopStart)%(s.length-s.loopStart);}
    result.offset=uint32_t(position);result.remaining=s.length-result.offset;out=result;return true;
}
}
