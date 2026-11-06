#include "Midi.hpp"
#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <queue>
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
MidiSong parseMidi(const std::vector<uint8_t>& b,size_t eventLimit){
    eventLimit=std::min(eventLimit,MidiMaxEvents);
    if(b.size()>=12&&!memcmp(b.data(),"RIFF",4)&&!memcmp(b.data()+8,"RMID",4)){
        auto le=[&](size_t p){return uint32_t(b[p])|(uint32_t(b[p+1])<<8)|(uint32_t(b[p+2])<<16)|(uint32_t(b[p+3])<<24);};
        uint64_t end=uint64_t(le(4))+8;if(end>b.size()||end<12)throw std::runtime_error("Truncated RIFF MIDI.");
        for(size_t p=12;p+8<=end;){size_t n=le(p+4);if(n>end-p-8)throw std::runtime_error("Truncated RIFF MIDI chunk.");if(!memcmp(b.data()+p,"data",4))return parseMidi(std::vector<uint8_t>(b.begin()+p+8,b.begin()+p+8+n),eventLimit);p+=8+n+(n&1);}
        throw std::runtime_error("RIFF MIDI has no data chunk.");
    }
    if(b.size()<14||memcmp(b.data(),"MThd",4))throw std::runtime_error("Not a Standard MIDI file.");
    Reader r{b,4,b.size()};auto header=r.be(4);if(header<6||header>b.size()-8)throw std::runtime_error("Invalid MIDI header.");
    auto format=r.be(2),tracks=r.be(2),division=r.be(2);
    if(format>1||!tracks||tracks>1024||(format==0&&tracks!=1)||!division)throw std::runtime_error("Only MIDI format0/1 is supported (format2 is independent songs).");
    if((division&0x8000)&&((division&255)==0||(-int(int8_t(division>>8))!=24&&-int(int8_t(division>>8))!=25&&-int(int8_t(division>>8))!=29&&-int(int8_t(division>>8))!=30)))throw std::runtime_error("Unsupported MIDI SMPTE division.");
    r.p=8+header;std::vector<Raw> raw;raw.reserve(std::min(eventLimit,b.size()/3+1));std::vector<std::pair<size_t,size_t>> ranges;MidiSong song;uint64_t endTick=0;
    for(unsigned t=0;t<tracks;t++){
        if(r.end-r.p<8||memcmp(b.data()+r.p,"MTrk",4))throw std::runtime_error("Missing MIDI track.");r.p+=4;
        auto size=r.be(4);if(size>r.end-r.p)throw std::runtime_error("Truncated MIDI track.");Reader tr{b,r.p,r.p+size};r.p+=size;
        const size_t begin=raw.size();uint64_t tick=0;uint8_t running=0;bool ended=false;
        while(tr.p<tr.end){
            tick+=tr.vlq();if(tick>uint64_t(1)<<40)throw std::runtime_error("MIDI time limit exceeded.");
            auto s=tr.byte();if(s<128){if(!running)throw std::runtime_error("MIDI running status without channel status.");--tr.p;s=running;}
            if(s==0xff){auto type=tr.byte();auto n=tr.vlq();if(n>tr.end-tr.p)throw std::runtime_error("Truncated MIDI metadata.");
                if(type==0x51){if(n!=3)throw std::runtime_error("Invalid MIDI tempo.");auto tempo=(uint32_t(b[tr.p])<<16)|(uint32_t(b[tr.p+1])<<8)|b[tr.p+2];if(!tempo)throw std::runtime_error("Zero MIDI tempo.");raw.push_back({tick,tempo,true});}
                if(type==3&&song.title.empty()){for(size_t i=0;i<std::min<size_t>(n,160);i++)song.title+=b[tr.p+i]>=32&&b[tr.p+i]<127?char(b[tr.p+i]):' ';}
                tr.p+=n;if(type==0x2f){if(n)throw std::runtime_error("Invalid MIDI end-of-track.");ended=true;break;}
            }else if(s==0xf0||s==0xf7){running=0;auto n=tr.vlq();if(n>tr.end-tr.p)throw std::runtime_error("Truncated MIDI SysEx.");
                // GM System On resets all synth channels. Other vendor SysEx is disclosed.
                if(s==0xf0&&n==5&&b[tr.p]==0x7e&&b[tr.p+2]==9&&(b[tr.p+3]==1||b[tr.p+3]==3)&&b[tr.p+4]==0xf7)raw.push_back({tick,0xff,false});else {if(n>1048576||song.sysex.size()>=65536)throw std::runtime_error("MIDI SysEx limit exceeded.");std::vector<uint8_t> data;if(s==0xf0)data.push_back(0xf0);data.insert(data.end(),b.begin()+tr.p,b.begin()+tr.p+n);song.sysex.push_back(std::move(data));raw.push_back({tick,0x1000000u|uint32_t(song.sysex.size()-1),false});++song.ignoredSysex;}
                tr.p+=n;
            }else if(s>=0x80&&s<=0xef){running=s;auto a=tr.byte();auto c=(s&0xe0)==0xc0?0:tr.byte();if(a>127||c>127)throw std::runtime_error("Invalid MIDI channel data.");
                raw.push_back({tick,uint32_t(s)|(uint32_t(a)<<8)|(uint32_t(c)<<16),false});if((s&0xf0)==0x90&&c)song.used|=uint16_t(1<<(s&15));
            }else throw std::runtime_error("Unsupported system event in MIDI track.");
            if(raw.size()>eventLimit)throw std::runtime_error("MIDI event limit exceeded.");
        }
        // A complete declared track boundary is usable even without optional EOT.
        (void)ended;endTick=std::max(endTick,tick);ranges.push_back({begin,raw.size()});
    }
    if(!song.used)throw std::runtime_error("MIDI contains no note-on events.");
    // Each track already has monotonic ticks. Merge only the track heads:
    // O(events*log(tracks)), with no large stable-sort scratch allocation.
    auto later=[&](size_t a,size_t b){return raw[a].tick!=raw[b].tick?raw[a].tick>raw[b].tick:a>b;};
    std::priority_queue<size_t,std::vector<size_t>,decltype(later)>heads(later);
    for(size_t i=0;i<ranges.size();i++)if(ranges[i].first<ranges[i].second)heads.push(ranges[i].first);
    song.events.reserve(raw.size()+32);
    uint64_t tick=0,frame=0,remainder=0;uint32_t tempo=500000;
    auto advance=[&](uint64_t target){
        uint64_t numerator,denominator;
        if(division&0x8000){unsigned fps=unsigned(-int(int8_t(division>>8)));numerator=InternalRate*(fps==29?1001ull:1ull);denominator=uint64_t(division&255)*(fps==29?30000:fps);}
        else{numerator=uint64_t(tempo)*InternalRate;denominator=uint64_t(division)*1000000;}
        auto delta=target-tick;if(delta>(UINT64_MAX-remainder)/numerator)throw std::runtime_error("MIDI duration overflow.");auto amount=delta*numerator+remainder;frame+=amount/denominator;remainder=amount%denominator;tick=target;
        if(frame>uint64_t(InternalRate)*7200)throw std::runtime_error("MIDI exceeds the two-hour limit.");
    };
    while(!heads.empty()){size_t at=heads.top();heads.pop();const auto&e=raw[at];advance(e.tick);if(e.tempo)tempo=e.value;else song.events.push_back({frame,e.value});
        auto range=std::upper_bound(ranges.begin(),ranges.end(),at,[](size_t p,const auto&r){return p<r.second;});
        if(range!=ranges.end()&&at+1<range->second)heads.push(at+1);
    }advance(endTick);
    // A bounded release tail; the file's event boundary is preserved separately.
    song.endFrame=frame+2*InternalRate;
    for(unsigned c=0;c<16;c++){song.events.push_back({frame,0x40b0u+c});song.events.back().message|=64u<<8;song.events.push_back({frame,0x7bb0u+c});}
    return song;
}
}
