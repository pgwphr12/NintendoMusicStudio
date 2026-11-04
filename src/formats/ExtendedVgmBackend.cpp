// SPDX-License-Identifier: GPL-2.0-or-later
// Independently authored bounded VGM scheduler/metadata/stream reader.
// No libvgm player, loader, device framework or resampler implementation.
#include "core/IMusicBackend.hpp"
#include "LicensedChips.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>
#include <limits>
namespace ngmv {
namespace {
void require(bool valid, const char *text) {
    if (!valid)
        throw std::runtime_error(text);
}
// A bounded preflight of this integration's supported subset. Do not silently
// discard unsupported chips/commands or trust allocation sizes in music files.
std::vector<uint8_t> validateExtended(const std::vector<uint8_t> &b) {
    auto le = [&](size_t p) -> uint32_t {
        require(p <= b.size() && b.size() - p >= 4, "Truncated VGM field.");
        return uint32_t(b[p]) | uint32_t(b[p + 1]) << 8 | uint32_t(b[p + 2]) << 16 |
               uint32_t(b[p + 3]) << 24;
    };
    require(b.size() >= 0x40 && b.size() <= 64 * 1024 * 1024, "Invalid VGM size.");
    size_t end = uint64_t(le(4)) + 4;
    require(end <= b.size() && end >= 0x40, "Invalid VGM length.");
    require(le(8) >= 0x150 && le(8) <= 0x171, "Unsupported extended VGM version.");
    size_t start = le(0x34) ? uint64_t(le(0x34)) + 0x34 : 0x40;
    require(start >= 0x40 && start < end, "Invalid VGM data offset.");
    auto hdr = [&](size_t p) { return p + 4 <= start ? le(p) : 0u; };
    const size_t clocks[] = {0x0c, 0x10, 0x2c, 0x30, 0x38, 0x40, 0x44, 0x48, 0x4c, 0x50, 0x54,
                             0x58, 0x5c, 0x60, 0x64, 0x68, 0x6c, 0x70, 0x74, 0x80, 0x84, 0x88,
                             0x8c, 0x90, 0x98, 0x9c, 0xa0, 0xa4, 0xa8, 0xac, 0xb0, 0xb4, 0xb8,
                             0xc0, 0xc4, 0xc8, 0xcc, 0xd0, 0xd8, 0xdc, 0xe0};
    unsigned chips = 0;
    for (auto p : clocks)
        if (auto clock = hdr(p)) {
            require(p == 0x2c || p == 0x50 || p == 0x98 || p == 0x9c || p == 0xd0,
                    "This VGM chip combination is not supported yet.");
            require(!(clock & 0x40000000) && (p == 0xd0 || p == 0x98 || p == 0x9c || !(clock & 0x80000000)) &&
                        (clock & 0x3fffffff) >= 1000 && (clock & 0x3fffffff) <= 50000000,
                    "Unsupported VGM clock/dual-chip mode.");
            ++chips;
        }
    require(chips != 0, "VGM has no supported sound chips.");
    if (hdr(0xbc)) { // Optional extra clock/volume tables, relative offsets.
        size_t x = uint64_t(hdr(0xbc)) + 0xbc;
        require(x >= 0xc0 && x + 12 <= start && le(x) >= 12 && uint64_t(le(x)) + x <= start,
                "Invalid VGM extra header.");
        for (unsigned j = 0; j < 2; ++j)
            if (auto offset = le(x + 4 + j * 4)) {
                size_t t = uint64_t(x + 4 + j * 4) + offset;
                require(t < start && size_t(b[t]) * (j ? 4 : 5) <= start - t - 1,
                        "Invalid VGM extra table.");
            }
    }
    size_t gd = le(0x14) ? uint64_t(le(0x14)) + 0x14 : 0;
    if (gd) {
        require(gd >= start && gd + 12 <= end && le(gd) == 0x20336447 && !(le(gd + 8) & 1) &&
                    uint64_t(le(gd + 8)) + gd + 12 <= end,
                "Invalid VGM metadata.");
        // Require all eleven UTF16 fields to terminate within the bounded tag.
        size_t p = gd + 12, stop = p + le(gd + 8);
        unsigned strings = 0;
        while (p + 2 <= stop) {
            if (!b[p] && !b[p + 1])
                ++strings;
            p += 2;
        }
        require(strings >= 11, "Unterminated VGM metadata.");
        end = gd;
    }
    size_t loop = le(0x1c) ? uint64_t(le(0x1c)) + 0x1c : 0;
    bool loopSeen = !loop, ended = false;
    uint64_t ticks = 0, bank = 0;
    size_t pcm = 0;
    for (size_t p = start; p < end;) {
        if (p == loop)
            loopSeen = true;
        uint8_t c = b[p];
        size_t n = 1;
        if (c == 0x52 || c == 0x53 || c == 0x5a || c == 0xb8 || c == 0xbe || c == 0x61)
            n = 3;
        else if (c == 0xd2)
            n = 4;
        else if (c == 0xe0)
            n = 5;
        else if (c == 0x90 || c == 0x91 || c == 0x95)
            n = 5;
        else if (c == 0x92)
            n = 6;
        else if (c == 0x93)
            n = 11;
        else if (c == 0x94)
            n = 2;
        else if (c == 0x67) {
            require(p + 7 <= end && b[p + 1] == 0x66, "Invalid VGM data block.");
            n = uint64_t(le(p + 3)) + 7;
        } else
            require(c == 0x66 || c == 0x62 || c == 0x63 || (c >= 0x70 && c <= 0x8f),
                    "Unsupported extended VGM command.");
        require(n <= end - p, "Truncated extended VGM command/data block.");
        auto chip = [&](size_t field) {
            require(hdr(field) != 0, "VGM command references an undeclared chip.");
        };
        if (c == 0x52 || c == 0x53)
            chip(0x2c);
        if (c == 0x5a)
            chip(0x50);
        if (c == 0xb8) {
            chip(0x98);
            require(!(b[p + 1] & 0x80), "Unsupported second OKI chip.");
        }
        if (c == 0xbe) {
            chip(0xd0);
            require(!(b[p + 1] & 0x80), "Unsupported second ES5506 chip.");
        }
        if (c == 0xd2) {
            chip(0x9c);
            require(b[p + 1] <= 5, "Invalid SCC port.");
        }
        if (c == 0x67) {
            auto type = b[p + 2];
            size_t len = n - 7;
            require(type == 0 || type == 0x8b || type == 0x90, "Unsupported VGM data block type.");
            if (!type) {
                bank += len;
                require(bank <= 64 * 1024 * 1024, "VGM data bank too large.");
            } else {
                require(len >= 8, "Truncated VGM ROM block.");
                uint32_t size = le(p + 7), off = le(p + 11);
                if (type == 0x90)
                    off &= 0x0fffffff;
                require(size && size <= 64 * 1024 * 1024 && off <= size && len - 8 <= size - off,
                        "Invalid VGM ROM allocation/range.");
            }
        }
        if (c == 0xe0) {
            pcm = le(p + 1);
            require(pcm <= bank, "Invalid VGM PCM seek.");
        }
        if (c >= 0x80 && c <= 0x8f) {
            chip(0x2c);
            require(pcm < bank, "VGM PCM cursor exceeds bank.");
            ++pcm;
            ticks += c & 15;
        }
        if (c >= 0x90 && c <= 0x95) {
            require(b[p + 1] != 255 || c == 0x94, "Invalid VGM stream ID.");
            if (c == 0x90) {
                require(b[p + 2] == 2 && b[p + 3] == 0 && b[p + 4] == 0x2a,
                        "Unsupported DAC stream target.");
                chip(0x2c);
            }
            if (c == 0x91)
                require(b[p + 2] == 0 && b[p + 3] != 0, "Unsupported DAC stream bank/stride.");
            if (c == 0x92)
                require(le(p + 2) <= 384000, "VGM stream frequency too large.");
        }
        if (c == 0x61)
            ticks += b[p + 1] | unsigned(b[p + 2]) << 8;
        if (c == 0x62)
            ticks += 735;
        if (c == 0x63)
            ticks += 882;
        if (c >= 0x70 && c <= 0x7f)
            ticks += (c & 15) + 1;
        require(ticks <= 44100ull * 3600, "VGM duration exceeds one hour.");
        p += n;
        if (c == 0x66) {
            ended = true;
            break;
        }
    }
    require(ended && ticks && loopSeen, "VGM has no valid timed end/loop.");
    auto result = b;
    auto put = [&](size_t p, uint32_t v) {
        for (unsigned i = 0; i < 4; ++i)
            result[p + i] = uint8_t(v >> (i * 8));
    };
    put(0x1c, 0);
    put(0x20, 0);
    put(0x18, uint32_t(ticks)); // App owns repeat/next-track.
    return result;
}

uint32_t read32(const std::vector<uint8_t>&b,size_t p){require(p<=b.size()&&b.size()-p>=4,"Truncated VGM integer.");return b[p]|uint32_t(b[p+1])<<8|uint32_t(b[p+2])<<16|uint32_t(b[p+3])<<24;}
uint16_t read16(const std::vector<uint8_t>&b,size_t p){require(p+2<=b.size(),"Truncated VGM word.");return b[p]|uint16_t(b[p+1])<<8;}
struct Event {uint64_t tick;size_t offset;};
struct Config {ChipKind kind;uint32_t clock;uint8_t flags;unsigned voices,id;const char*name;float gain=1;};
struct Program {
    std::vector<uint8_t> bytes,pcm;
    std::vector<std::pair<size_t,size_t>> blocks;
    std::vector<Event> events;
    std::vector<Config> configs;
    TrackInfo info;
    uint64_t ticks=0;
    float gain=1;
    explicit Program(const std::vector<uint8_t>&input):bytes(validateExtended(input)){
        auto&b=bytes;size_t start=read32(b,0x34)?read32(b,0x34)+0x34:0x40;
        auto header=[&](size_t p){return p+4<=start?read32(b,p):0u;};
        auto add=[&](size_t at,ChipKind kind,unsigned voices,unsigned id,const char*name,uint8_t flags){auto clock=header(at);if(clock)configs.push_back({kind,clock&0x3fffffff,flags,voices,id,name});};
        add(0x2c,ChipKind::Ym2612,7,2,"YM2612",0);
        add(0x50,ChipKind::Opl2,14,10,"YM3812",0);
        add(0x98,ChipKind::Oki,4,24,"MSM6295",uint8_t(header(0x98)>>31));
        add(0x9c,ChipKind::Scc,5,25,"K051649",uint8_t(header(0x9c)>>31));
        add(0xd0,ChipKind::Es5506,1,37,"ES5506",uint8_t(header(0xd0)>>24)&128);
        for(auto&c:configs)if(c.kind==ChipKind::Es5506){require(c.flags&128,"Licensed ES5505 support is not implemented.");} // Real serial output is already normalized; preserve unity chip balance.
        if(auto extra=header(0xbc)){
            size_t x=extra+0xbc;auto volume=read32(b,x+8);
            if(volume){size_t t=x+8+volume;for(unsigned i=0;i<b[t];++i){size_t e=t+1+4*i;
                require(!(b[e]&128)&&b[e+1]==0,"Unsupported paired/second-chip volume entry.");
                auto v=read16(b,e+2);for(auto&c:configs)if(c.id==b[e])c.gain=(v&32768)?c.gain*((v&32767)/256.f):v/256.f;
            }}
        }
        int modifier=start>0x7c?b[0x7c]:0;if(modifier>=193)modifier-=256;if(modifier==-63)modifier=-64;
        // The file's modifier controls the mix. Merely declaring another chip
        // must not attenuate all existing voices (regression in16/17).
        gain=float(std::pow(2.,modifier/32.))*.5f;
        for(size_t p=start;p<b.size();){
            uint8_t c=b[p];size_t n=1;
            if(c==0x66)break;
            if(c==0x61){ticks+=read16(b,p+1);n=3;}
            else if(c==0x62)ticks+=735;
            else if(c==0x63)ticks+=882;
            else if(c>=0x70&&c<=0x7f)ticks+=(c&15)+1;
            else {
                events.push_back({ticks,p});
                if(c==0x67){n=7+read32(b,p+3);if(b[p+2]==0){blocks.push_back({pcm.size(),n-7});pcm.insert(pcm.end(),b.begin()+p+7,b.begin()+p+n);}}
                else if(c==0x52||c==0x53||c==0x5a||c==0xb8||c==0xbe)n=3;
                else if(c==0xd2)n=4;
                else if(c==0xe0||c==0x90||c==0x91||c==0x95)n=5;
                else if(c==0x92)n=6;
                else if(c==0x93)n=11;
                else if(c==0x94)n=2;
                else if(c>=0x80&&c<=0x8f)ticks+=c&15;
            }
            p+=n;
        }
        info.lengthMs=int((ticks*1000+22050)/44100);
        auto utf8=[](std::string&s,uint32_t cp){if(cp<128)s+=char(cp);else if(cp<2048){s+=char(192|(cp>>6));s+=char(128|(cp&63));}else if(cp<65536){s+=char(224|(cp>>12));s+=char(128|((cp>>6)&63));s+=char(128|(cp&63));}else{s+=char(240|(cp>>18));s+=char(128|((cp>>12)&63));s+=char(128|((cp>>6)&63));s+=char(128|(cp&63));}};
        if(auto gd=read32(b,0x14)){
            size_t p=gd+0x14+12;std::array<std::string,11> tags;
            for(auto&tag:tags){while(auto c=read16(b,p)){p+=2;uint32_t cp=c;
                if(c>=0xd800&&c<=0xdbff){auto lo=read16(b,p);require(lo>=0xdc00&&lo<=0xdfff,"Invalid VGM UTF16 surrogate.");p+=2;cp=0x10000+((c-0xd800)<<10)+(lo-0xdc00);}
                else require(c<0xdc00||c>0xdfff,"Invalid VGM UTF16 surrogate.");utf8(tag,cp);}p+=2;}
            info.title=tags[0].empty()?tags[1]:tags[0];info.game=tags[2].empty()?tags[3]:tags[2];info.system=tags[4].empty()?tags[5]:tags[4];info.author=tags[6].empty()?tags[7]:tags[6];info.comment=tags[10];
        }
    }
};
class Core {
    std::shared_ptr<const Program> program;
    std::array<std::unique_ptr<LicensedChip>,5> chips;
    struct Stream {bool setup=false,data=false,on=false,loop=false,reverse=false;uint32_t frequency=0,stride=1,base=0;size_t start=0,count=0,index=0;double next=0;};
    std::array<Stream,255> streams{};
    size_t event=0,pcm=0;uint64_t frame=0;
    LicensedChip*chip(ChipKind kind){return chips[size_t(kind)].get();}
    void write(ChipKind kind,uint8_t port,uint8_t reg,uint8_t value){if(auto*c=chip(kind))c->write(port,reg,value);}
    void start(Stream&s,size_t offset,size_t commands,bool loop,bool reverse){
        require(s.setup&&s.data&&s.frequency,"Incomplete VGM DAC stream setup.");
        require(commands && offset<program->pcm.size() && s.base<program->pcm.size()-offset,"VGM stream start outside PCM.");
        require(commands-1<=(program->pcm.size()-1-offset-s.base)/s.stride,"VGM stream length outside PCM.");
        s.start=offset;s.count=commands;s.loop=loop;s.reverse=reverse;s.index=0;s.next=double(frame);s.on=true;
    }
    void apply(size_t p){auto&b=program->bytes;auto c=b[p];
        if(c==0x52||c==0x53)write(ChipKind::Ym2612,c-0x52,b[p+1],b[p+2]);
        else if(c==0x5a)write(ChipKind::Opl2,0,b[p+1],b[p+2]);
        else if(c==0xb8)write(ChipKind::Oki,0,b[p+1],b[p+2]);
        else if(c==0xbe)write(ChipKind::Es5506,0,b[p+1],b[p+2]);
        else if(c==0xd2)write(ChipKind::Scc,b[p+1],b[p+2],b[p+3]);
        else if(c==0x67 && b[p+2]){auto kind=b[p+2]==0x8b?ChipKind::Oki:ChipKind::Es5506;if(auto*d=chip(kind))d->rom(read32(b,p+7),read32(b,p+11),b.data()+p+15,read32(b,p+3)-8);}
        else if(c==0xe0)pcm=read32(b,p+1);
        else if(c>=0x80&&c<=0x8f){require(pcm<program->pcm.size(),"VGM PCM cursor overflow.");write(ChipKind::Ym2612,0,0x2a,program->pcm[pcm++]);}
        else if(c>=0x90&&c<=0x95){
            auto id=b[p+1];if(c==0x94&&id==255){for(auto&s:streams)s.on=false;return;}
            require(id<streams.size(),"Invalid DAC stream ID.");auto&s=streams[id];
            if(c==0x90){s.setup=true;s.on=false;}
            else if(c==0x91){s.data=true;s.stride=b[p+3];s.base=b[p+4];}
            else if(c==0x92)s.frequency=read32(b,p+2);
            else if(c==0x94)s.on=false;
            else if(c==0x93){auto offset=read32(b,p+2),length=read32(b,p+7);unsigned mode=b[p+6]&15;
                require(!(b[p+6]&0x60)&&mode<=3,"Unsupported VGM stream length mode.");
                size_t at=offset==UINT32_MAX?s.start:offset;
                if(mode==0){require(at<=program->pcm.size(),"Invalid stream position.");s.start=at;s.index=0;}
                else {size_t n=mode==1?length:mode==2?uint64_t(length)*s.frequency/1000:(at+s.base<program->pcm.size()?(program->pcm.size()-at-s.base+s.stride-1)/s.stride:0);start(s,at,n,b[p+6]&128,b[p+6]&16);}
            }else {unsigned block=read16(b,p+2);require(block<program->blocks.size(),"Invalid stream block.");require(!(b[p+4]&0xee),"Invalid stream fast flags.");auto range=program->blocks[block];require(s.base<range.second,"Invalid stream base.");start(s,range.first,(range.second-s.base+s.stride-1)/s.stride,b[p+4]&1,b[p+4]&16);}
        }
    }
  public:
    Core(std::shared_ptr<const Program>p,int solo=-1,unsigned voice=0):program(std::move(p)){
        for(size_t i=0;i<program->configs.size();++i)if(solo<0||size_t(solo)==i){auto&c=program->configs[i];auto d=std::make_unique<LicensedChip>(c.kind,c.clock,c.flags);if(solo>=0)d->mute(c.kind==ChipKind::Es5506?0:~(1u<<voice));chips[size_t(c.kind)]=std::move(d);}
    }
    void mask(uint32_t mask){unsigned offset=0;for(auto&c:program->configs){if(auto*d=chip(c.kind))d->mute(c.kind==ChipKind::Es5506?(mask&(1u<<offset)?UINT32_MAX:0):(mask>>offset)&((1u<<c.voices)-1));offset+=c.voices;}}
    Stereo sample(){
        if(frame*44100>=program->ticks*48000){++frame;return {};}
        while(event<program->events.size()&&program->events[event].tick*48000<=frame*44100)apply(program->events[event++].offset);
        for(auto&s:streams)if(s.on){unsigned writes=0;while(s.on&&s.next<=double(frame)){
            require(++writes<=9,"Unbounded DAC stream work.");size_t index=s.reverse?s.count-1-s.index:s.index;size_t at=s.start+s.base+index*s.stride;
            require(at<program->pcm.size(),"VGM stream data overflow.");write(ChipKind::Ym2612,0,0x2a,program->pcm[at]);s.next+=48000./s.frequency;
            if(++s.index==s.count){s.index=0;if(!s.loop)s.on=false;}
        }}
        Stereo result{};for(auto&c:program->configs)if(auto*d=chip(c.kind)){auto v=d->sample();result.l+=v.l*c.gain*program->gain;result.r+=v.r*c.gain*program->gain;}
        ++frame;return result;
    }
};
class ExtendedVgmBackend final:public IMusicBackend {
    std::shared_ptr<const Program> program;
    std::vector<TrackInfo> tracks_;
    std::vector<ChannelInfo> channels_;
    std::unique_ptr<Core> full;
    std::vector<std::unique_ptr<Core>> stems;
    uint32_t mask_=0;
    void restart(){full=std::make_unique<Core>(program);full->mask(mask_);stems.clear();for(size_t i=0;i<program->configs.size();++i)for(unsigned v=0;v<program->configs[i].voices;++v)stems.push_back(std::make_unique<Core>(program,int(i),v));}
  public:
    explicit ExtendedVgmBackend(const std::vector<uint8_t>&b):program(std::make_shared<Program>(b)){
        tracks_.push_back(program->info);for(auto&c:program->configs)for(unsigned v=0;v<c.voices;++v)channels_.push_back({std::string(c.name)+(c.kind==ChipKind::Es5506?" MIX":" "+std::to_string(v+1))});require(channels_.size()<=30,"Too many VGM channels.");restart();
    }
    const std::vector<TrackInfo>&tracks()const override{return tracks_;}
    const std::vector<ChannelInfo>&channels()const override{return channels_;}
    void selectTrack(int i)override{require(i==0,"Invalid VGM track.");restart();}
    void muteMask(uint32_t mask)override{mask_=mask;full->mask(mask);}
    void render(size_t frames,AudioBlock&out)override{require(frames<=8192,"VGM audio block too large.");out.mix.resize(frames);out.voices.resize(stems.size());for(auto&v:out.voices)v.resize(frames);for(size_t i=0;i<frames;++i){out.mix[i]=full->sample();for(size_t c=0;c<stems.size();++c)out.voices[c][i]=stems[c]->sample();}}
    void seek(int ms)override{require(ms>=0&&ms<=3600000,"Invalid VGM seek.");restart();AudioBlock discard;size_t left=size_t(ms)*48;while(left){size_t n=std::min(left,size_t(8192));render(n,discard);left-=n;}}
};
} // namespace
std::unique_ptr<IMusicBackend> makeExtendedVgmBackend(const std::vector<uint8_t>&b){return std::make_unique<ExtendedVgmBackend>(b);}
} // namespace ngmv

