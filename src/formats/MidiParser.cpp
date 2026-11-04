#include "Midi.hpp"
#include <algorithm>
#include <cstring>
#include <stdexcept>
namespace ngmv {
namespace {
struct Reader {
    const std::vector<uint8_t>& b; size_t p,end;
    uint8_t byte(){if(p>=end)throw std::runtime_error("Truncated MIDI event.");return b[p++];}
    uint32_t be(unsigned n){uint32_t v=0;while(n--)v=(v<<8)|byte();return v;}
    uint32_t vlq(){uint32_t v=0;for(int i=0;i<4;i++){auto c=byte();v=(v<<7)|(c&127);if(!(c&128))return v;}throw std::runtime_error("Invalid MIDI variable-length value.");}
};
struct Raw {uint64_t tick;uint32_t value;bool tempo;};
}
MidiSong parseMidi(const std::vector<uint8_t>& b){
    if(b.size()<14||memcmp(b.data(),"MThd",4))throw std::runtime_error("Not a Standard MIDI file.");
    Reader r{b,4,b.size()};auto header=r.be(4);if(header<6||header>b.size()-8)throw std::runtime_error("Invalid MIDI header.");
    auto format=r.be(2),tracks=r.be(2),division=r.be(2);
    if(format>1||!tracks||tracks>1024||(format==0&&tracks!=1)||!division)throw std::runtime_error("Only MIDI format0/1 is supported (format2 is independent songs).");
    if((division&0x8000)&&((division&255)==0||(-int(int8_t(division>>8))!=24&&-int(int8_t(division>>8))!=25&&-int(int8_t(division>>8))!=29&&-int(int8_t(division>>8))!=30)))throw std::runtime_error("Unsupported MIDI SMPTE division.");
    r.p=8+header;std::vector<Raw> raw;MidiSong song;uint64_t endTick=0;
    for(unsigned t=0;t<tracks;t++){
        if(r.end-r.p<8||memcmp(b.data()+r.p,"MTrk",4))throw std::runtime_error("Missing MIDI track.");r.p+=4;
        auto size=r.be(4);if(size>r.end-r.p)throw std::runtime_error("Truncated MIDI track.");Reader tr{b,r.p,r.p+size};r.p+=size;
        uint64_t tick=0;uint8_t running=0;bool ended=false;
        while(tr.p<tr.end){
            tick+=tr.vlq();if(tick>uint64_t(1)<<40)throw std::runtime_error("MIDI time limit exceeded.");
            auto s=tr.byte();if(s<128){if(!running)throw std::runtime_error("MIDI running status without channel status.");--tr.p;s=running;}
            if(s==0xff){running=0;auto type=tr.byte();auto n=tr.vlq();if(n>tr.end-tr.p)throw std::runtime_error("Truncated MIDI metadata.");
                if(type==0x51){if(n!=3)throw std::runtime_error("Invalid MIDI tempo.");auto tempo=(uint32_t(b[tr.p])<<16)|(uint32_t(b[tr.p+1])<<8)|b[tr.p+2];if(!tempo)throw std::runtime_error("Zero MIDI tempo.");raw.push_back({tick,tempo,true});}
                if(type==3&&song.title.empty()){for(size_t i=0;i<std::min<size_t>(n,160);i++)song.title+=b[tr.p+i]>=32&&b[tr.p+i]<127?char(b[tr.p+i]):' ';}
                tr.p+=n;if(type==0x2f){if(n)throw std::runtime_error("Invalid MIDI end-of-track.");ended=true;break;}
            }else if(s==0xf0||s==0xf7){running=0;auto n=tr.vlq();if(n>tr.end-tr.p)throw std::runtime_error("Truncated MIDI SysEx.");
                // GM System On resets all synth channels. Other vendor SysEx is disclosed.
                if(s==0xf0&&n==5&&b[tr.p]==0x7e&&b[tr.p+2]==9&&(b[tr.p+3]==1||b[tr.p+3]==3)&&b[tr.p+4]==0xf7)raw.push_back({tick,0xff,false});else ++song.ignoredSysex;
                tr.p+=n;
            }else if(s>=0x80&&s<=0xef){running=s;auto a=tr.byte();auto c=(s&0xe0)==0xc0?0:tr.byte();if(a>127||c>127)throw std::runtime_error("Invalid MIDI channel data.");
                raw.push_back({tick,uint32_t(s)|(uint32_t(a)<<8)|(uint32_t(c)<<16),false});if((s&0xf0)==0x90&&c)song.used|=uint16_t(1<<(s&15));
            }else throw std::runtime_error("Unsupported system event in MIDI track.");
            if(raw.size()>1000000)throw std::runtime_error("MIDI event limit exceeded.");
        }
        if(!ended)throw std::runtime_error("MIDI track has no end marker.");endTick=std::max(endTick,tick);
    }
    if(!song.used)throw std::runtime_error("MIDI contains no note-on events.");
    std::stable_sort(raw.begin(),raw.end(),[](const Raw&a,const Raw&b){return a.tick<b.tick;});
    uint64_t tick=0,frame=0,remainder=0;uint32_t tempo=500000;
    auto advance=[&](uint64_t target){
        uint64_t numerator,denominator;
        if(division&0x8000){unsigned fps=unsigned(-int(int8_t(division>>8)));numerator=InternalRate*(fps==29?1001ull:1ull);denominator=uint64_t(division&255)*(fps==29?30000:fps);}
        else{numerator=uint64_t(tempo)*InternalRate;denominator=uint64_t(division)*1000000;}
        auto delta=target-tick;if(delta>(UINT64_MAX-remainder)/numerator)throw std::runtime_error("MIDI duration overflow.");auto amount=delta*numerator+remainder;frame+=amount/denominator;remainder=amount%denominator;tick=target;
        if(frame>uint64_t(InternalRate)*7200)throw std::runtime_error("MIDI exceeds the two-hour limit.");
    };
    for(const auto&e:raw){advance(e.tick);if(e.tempo)tempo=e.value;else song.events.push_back({frame,e.value});}advance(endTick);
    // A bounded release tail; the file's event boundary is preserved separately.
    song.endFrame=frame+2*InternalRate;
    for(unsigned c=0;c<16;c++){song.events.push_back({frame,0x40b0u+c});song.events.back().message|=64u<<8;song.events.push_back({frame,0x7bb0u+c});}
    return song;
}
}
