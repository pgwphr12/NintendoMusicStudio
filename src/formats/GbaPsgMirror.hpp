#pragma once
#include <GBA.h>
#include <Gb_Apu.h>
#include <Multi_Buffer.h>
#include <array>
#include <deque>
#include <memory>
#include <stdexcept>
namespace ngmv {
// Audio-only mirrors of the exact observed APU writes. No duplicate ARM CPUs,
// no source-memory changes and no inference from the mixed PCM waveform.
class GbaPsgMirror {
    struct Voice {GBA::Gb_Apu apu;GBA::Stereo_Buffer buffer;std::deque<Stereo> pending;};
    std::array<std::unique_ptr<Voice>,4> voices_;
public:
    explicit GbaPsgMirror(GBASystem&g){// Attach before CPUReset; resets and BIOS sound writes are mirrored too.

        for(int c=0;c<4;c++){auto&v=voices_[c];v=std::make_unique<Voice>();if(v->buffer.set_sample_rate(InternalRate))throw std::runtime_error("PSG buffer allocation failed");v->buffer.clock_rate(GBA::Gb_Apu::clock_rate);v->apu.reduce_clicks(g.soundDeclicking);v->apu.reset(GBA::Gb_Apu::mode_agb,true);v->apu.treble_eq(GBA::blip_eq_t(0,0,InternalRate,InternalRate/2));
            static const float gain[]={.25f,.5f,1.f,.25f};v->apu.volume(g.soundVolume_*gain[g.ioMem[0x82]&3]);v->apu.set_output(v->buffer.center(),v->buffer.left(),v->buffer.right(),c);v->apu.write_register(0,0xff26,0x80);}
    }
    void event(int kind,int time,int address,int value,float volume){for(auto&v:voices_){if(kind==0)v->apu.write_register(time,address,value);else if(kind==2)v->apu.volume(volume);else if(kind==3){v->apu.reset(GBA::Gb_Apu::mode_agb,true);v->buffer.clear();v->pending.clear();}else{
        v->apu.end_frame(time);v->buffer.end_frame(time);std::array<GBA::blip_sample_t,2048> samples{};while(v->buffer.samples_avail()){long n=v->buffer.read_samples(samples.data(),long(samples.size()));for(long i=0;i<n;i+=2)v->pending.push_back({samples[i]/32768.f,samples[i+1]/32768.f});}
    }}}
    void render(int channel,size_t n,std::vector<Stereo>&out){auto&q=voices_[channel]->pending;if(q.size()<n)throw std::runtime_error("PSG mirror lost audio alignment");out.resize(n);for(auto&v:out){v=q.front();q.pop_front();}}
    static void callback(void*p,int kind,int time,int address,int value,float volume){static_cast<GbaPsgMirror*>(p)->event(kind,time,address,value,volume);}
};
}
