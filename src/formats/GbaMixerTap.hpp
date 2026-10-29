#pragma once
#include <GBA.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include "GbaVoiceCapture.hpp"
#include "GbaMixCapture.hpp"
namespace ngmv {
// Recognized standard MP2K/m4a layout only. The original core is read-only.
// Isolated cores suppress other voice contributions in the emulated PCM
// destination, without muting sequencer state or changing voice allocation.
// Layout references are documented in docs/GBA-MIXER.md; no guest code runs natively.
struct GbaMixerTap {
    std::unique_ptr<GbaVoiceCapture> capture;
    std::unique_ptr<GbaMixCapture> mixCapture;
    int isolate=-1,voices=0,currentVoice=-1;uint32_t info=0;uint64_t writes=0,recognized=0;unsigned checks=0;
    static const uint8_t* ram(GBASystem*g,uint32_t a,unsigned n){
        if((a>>24)==2&&(a&0xffffff)<=0x40000-n)return g->workRAM+(a&0x3ffff);
        if((a>>24)==3&&(a&0xffffff)<=0x8000-n)return g->internalRAM+(a&0x7fff);
        return nullptr;
    }
    static uint32_t word(const uint8_t*p){return uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);}
    void consider(GBASystem*g,uint32_t base){
        auto s=ram(g,base,0xfb0);if(!s)return;
        uint32_t id=word(s);int count=s[6];uint32_t samples=word(s+16),hz=word(s+20);
        if((id!=0x68736d53&&id!=0x68736d54)||count<1||count>12||samples<64||samples>704||hz<4000||hz>48000)return;
        info=base;voices=count;
    }
    void detect(GBASystem*g){auto p=ram(g,0x03007ff0,4);if(p)consider(g,word(p));}
    void scan(GBASystem*g){for(uint32_t region:{0x03000000u,0x02000000u}){unsigned size=(region>>24)==3?0x8000:0x40000;auto r=ram(g,region,size);if(!r)continue;for(unsigned a=0;a+0xfb0<=size;a+=4)if(word(r+a)==0x68736d53||word(r+a)==0x68736d54){consider(g,region+a);if(info)return;}}}
    int slot(uint32_t p)const{if(p<info+0x50||p>=info+0x50+uint32_t(voices)*64||(p-info-0x50)%64)return -1;return int((p-info-0x50)/64);}
    uint32_t filter(GBASystem*g,uint32_t address,uint32_t value,int width){
        if(!info){if(!(++checks&255))detect(g);if(!info)return value;}
        if(capture){
            auto identity=ram(g,info,4);
            if(identity&&(word(identity)==0x68736d53||word(identity)==0x68736d54)){
                auto read=[&](uint32_t p,size_t n)->const uint8_t*{
                    unsigned region=p>>24;size_t offset=p&0x1ffffffu;
                    if(region==2){offset=p&0xffffffu;return n<=0x40000&&offset<=0x40000-n?g->workRAM+offset:nullptr;}
                    if(region==3){offset=p&0xffffffu;return n<=0x8000&&offset<=0x8000-n?g->internalRAM+offset:nullptr;}
                    if(region>=8&&region<=13&&g->romSize>0&&n<=size_t(g->romSize)&&offset<=size_t(g->romSize)-n)return g->rom+offset;
                    return nullptr;
                };
                capture->onWrite(read,info,voices,address,value,width,g->viewerEventCycles+uint64_t(std::max(0,g->cpuTotalTicks)));
                if(mixCapture){
                    uint32_t period=g->timer0On&&g->timer0ClockReload>=0&&g->timer0ClockReload<=10?(0x10000u-uint16_t(g->timer0Reload))<<g->timer0ClockReload:0;
                    uint16_t control=uint16_t(g->ioMem[0x82])|uint16_t(g->ioMem[0x83])<<8;
                    mixCapture->onWrite(read,*capture,info,voices,g->reg[4].I,address,value,width,g->viewerEventCycles+uint64_t(std::max(0,g->cpuTotalTicks)),period,control);
                }
            }
        }
        if(address==info){currentVoice=-1;return value;}
        auto s=ram(g,info,0xfb0);if(!s||word(s)!=0x68736d54)return value;
        if(address>=info+0x50&&address<info+0x50+uint32_t(voices)*64){
            uint32_t relative=address-info-0x50;int candidate=int(relative/64),field=int(relative%64);
            // The mixer computes per-side envelope gains immediately before
            // entering its sample loop. Retain that exact slot while its CPU
            // register is reused as a frequency accumulator inside the loop.
            if((field==10||field==11)&&slot(g->reg[4].I)==candidate)currentVoice=candidate;
            return value;
        }
        if(address<info+0x350||address+uint32_t(width)>info+0x350+3168)return value;
        writes++;int voice=-1;
        {
            voice=slot(g->reg[4].I);
            if(voice<0)voice=currentVoice;
        }
        if(voice>=0)recognized++;
        if(isolate<0)return value; // original full mix is never changed
        if(voice==isolate)return value;
        if(voice>=0){auto old=ram(g,address,width);if(!old)return value;uint32_t v=0;for(int i=0;i<width;i++)v|=uint32_t(old[i])<<(i*8);return v;}
        return 0; // reset/reverb are excluded from dry voices and kept in residual
    }
    static u32 callback(void*p,GBASystem*g,u32 a,u32 v,int n){return static_cast<GbaMixerTap*>(p)->filter(g,a,v,n);}
};
}
