#pragma once
#include "core/GbaInspection.hpp"
#include <array>
#include <algorithm>
#include <cstddef>
#include <limits>
namespace ngmv {
// Independently written bounded parser for the documented standard 64-byte
// SoundChannel layout. No game code, wave bytes, or ROM assets are copied.
class GbaVoiceCapture {
    static constexpr size_t Capacity=8192;
    std::array<GbaVoiceEvent,Capacity> events_{};
    size_t used_=0;
    std::array<uint64_t,12> generations_{};
    std::array<bool,12> on_{},released_{};
    enum class Fault { None, Overflow, Malformed, Clock } fault_=Fault::None;
    uint64_t lastCycles_=0;
    static uint16_t u16(const uint8_t*p){return uint16_t(p[0])|(uint16_t(p[1])<<8);}
    static uint32_t u32(const uint8_t*p){return uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);}
    void push(GbaVoiceEvent event){
        if(fault_!=Fault::None)return;
        if(used_==Capacity){fault_=Fault::Overflow;return;}
        if(event.cpuCycles<lastCycles_){fault_=Fault::Clock;return;}
        lastCycles_=event.cpuCycles;events_[used_++]=event;
    }
public:
    bool activeGeneration(int slot,uint64_t&out)const{
        if(slot<0||slot>=12||!on_[size_t(slot)])return false;
        out=generations_[size_t(slot)];return true;
    }
    template<class Read> static bool decode(Read read,uint32_t channel,int slot,GbaVoiceEvent&event){
        auto p=read(channel,size_t(64));if(!p||slot<0||slot>=12)return false;
        if((p[1]&7)!=0||p[8]>127||p[17]>127||p[18]>127)return false;
        GbaVoiceEvent e;e.slot=slot;e.status=p[0];e.type=p[1];e.rightVolume=p[2];e.leftVolume=p[3];e.attack=p[4];e.decay=p[5];e.sustain=p[6];e.release=p[7];e.key=p[8];e.envelope=p[9];e.gateTicks=p[16];e.midiKey=p[17];e.velocity=p[18];e.frequency=u32(p+32);e.sourceTrack=u32(p+44);e.sample.address=u32(p+36);
        auto w=read(e.sample.address,size_t(16));
        if(w){
            auto&s=e.sample;s.type=u16(w);s.status=u16(w+2);s.rawFrequency=u32(w+4);s.loopStart=u32(w+8);s.length=u32(w+12);s.looped=(s.status&0x4000)!=0;
            bool range=s.length>0&&s.length<=32u*1024u*1024u&&s.address<=std::numeric_limits<uint32_t>::max()-16u;
            if(range&&(!s.looped||s.loopStart<s.length)&&read(s.address+16,size_t(s.length))){s.readable=true;s.pcm8=s.type==0&&(s.status&~0x4000u)==0&&s.rawFrequency>0;}
        }
        event=e;return true;
    }
    template<class Read> void onWrite(Read read,uint32_t soundInfo,int voices,uint32_t address,uint32_t value,int width,uint64_t cycles){
        if(fault_!=Fault::None||voices<1||voices>12||soundInfo>0xffffffffu-0x350u||width<1||width>4)return;
        uint32_t base=soundInfo+0x50u;
        if(address<base||address-base>=uint32_t(voices)*64u)return;
        uint32_t relative=address-base;int slot=int(relative/64u),offset=int(relative%64u);
        if(offset+width>64)return;
        bool status=offset==0,frequency=offset==32&&width==4,volume=offset<=3&&offset+width>2;
        if(!status&&!frequency&&!volume)return;
        auto raw=read(base+uint32_t(slot)*64u,size_t(64));if(!raw){fault_=Fault::Malformed;return;}
        std::array<uint8_t,64>copy;std::copy_n(raw,64,copy.begin());
        for(int i=0;i<width;i++)copy[size_t(offset+i)]=uint8_t(value>>(i*8));
        // Decode the prospective value, before the emulated RAM write occurs.
        auto prospective=[&](uint32_t p,size_t n)->const uint8_t*{if(p==base+uint32_t(slot)*64u&&n==64)return copy.data();return read(p,n);};
        GbaVoiceEvent e;if(!decode(prospective,base+uint32_t(slot)*64u,slot,e)){if(on_[slot]||(status&&(value&0x80)))fault_=Fault::Malformed;return;}
        e.cpuCycles=cycles;e.generation=generations_[slot];
        if(status&&(e.status&0x80)){
            e.kind=GbaVoiceEventKind::NoteOn;e.generation=++generations_[slot];on_[slot]=true;released_[slot]=false;push(e);
        }else if(status&&on_[slot]){
            if(!(e.status&0xc7)){e.kind=GbaVoiceEventKind::VoiceEnd;on_[slot]=false;push(e);}
            else if((e.status&0x40)&&!released_[slot]){e.kind=GbaVoiceEventKind::GateOff;released_[slot]=true;push(e);}
        }else if(on_[slot]&&frequency&&u32(raw+32)!=e.frequency){e.kind=GbaVoiceEventKind::Pitch;push(e);}
        else if(on_[slot]&&volume&&(raw[2]!=e.rightVolume||raw[3]!=e.leftVolume)){e.kind=GbaVoiceEventKind::Volume;push(e);}
    }
    bool drain(std::vector<GbaVoiceEvent>&out,std::string&error){
        if(fault_!=Fault::None){error=fault_==Fault::Overflow?"GBA event queue overflow; restart inspection with smaller render blocks":fault_==Fault::Clock?"GBA event clock moved backwards":"Unsupported or malformed GBA voice metadata";return false;}
        std::vector<GbaVoiceEvent>candidate(events_.begin(),events_.begin()+used_);out=std::move(candidate);used_=0;error.clear();return true;
    }
};
}
