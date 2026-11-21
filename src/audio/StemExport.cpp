#include "StemExport.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <stdexcept>
#include <string>
#include <windows.h>
namespace ngmv { namespace {
uint32_t read32(const std::vector<uint8_t>& b,size_t p){if(p>b.size()||b.size()-p<4)throw std::runtime_error("Truncated VGM header");return uint32_t(b[p])|(uint32_t(b[p+1])<<8)|(uint32_t(b[p+2])<<16)|(uint32_t(b[p+3])<<24);}
void put32(std::vector<uint8_t>& b,size_t p,uint32_t n){for(unsigned i=0;i<4;++i)b[p+i]=uint8_t(n>>(i*8));}
void u16(std::ostream& f,uint16_t x){f.put(char(x));f.put(char(x>>8));}
void u32(std::ostream& f,uint32_t x){u16(f,uint16_t(x));u16(f,uint16_t(x>>16));}
std::wstring safeName(const std::string& name){
    std::wstring s;int n=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,name.data(),int(name.size()),nullptr,0);if(n>0){s.resize(size_t(n));MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,name.data(),int(name.size()),s.data(),n);}
    for(auto&c:s)if(c<32||c==L'<'||c==L'>'||c==L':'||c==L'"'||c==L'/'||c==L'\\'||c==L'|'||c==L'?'||c==L'*')c=L'_';
    while(!s.empty()&&(s.back()==L' '||s.back()==L'.'))s.pop_back();if(s.size()>80)s.resize(80);return s.empty()?L"channel":s;
}
void header(std::ostream& f,uint32_t bytes){f.write("RIFF",4);u32(f,36+bytes);f.write("WAVEfmt ",8);u32(f,16);u16(f,1);u16(f,2);u32(f,InternalRate);u32(f,InternalRate*4);u16(f,4);u16(f,16);f.write("data",4);u32(f,bytes);}
int16_t pcm(float x){if(!std::isfinite(x))throw std::runtime_error("Decoder returned invalid audio");return int16_t(std::lround(std::clamp(x,-1.f,1.f)*32767.f));}
struct Output {std::filesystem::path path;std::ofstream file;bool audible=false;};
}
std::vector<uint8_t> expandVgmLoops(const std::vector<uint8_t>& b,unsigned repeats,uint64_t& frames){
    if(!repeats||repeats>99||b.size()<64||std::string(reinterpret_cast<const char*>(b.data()),4)!="Vgm ")throw std::runtime_error("Invalid VGM loop request");
    const uint64_t end64=uint64_t(read32(b,4))+4;if(end64>b.size()||end64<64)throw std::runtime_error("Invalid VGM length");size_t end=size_t(end64);
    size_t start=read32(b,8)>=0x150&&read32(b,0x34)?size_t(uint64_t(read32(b,0x34))+0x34):0x40;
    uint32_t loopOffset=read32(b,0x1c);if(!loopOffset)throw std::runtime_error("This VGM does not declare a loop");size_t loop=size_t(uint64_t(loopOffset)+0x1c);
    size_t gd=read32(b,0x14)?size_t(uint64_t(read32(b,0x14))+0x14):0;if(gd){if(gd<start||gd>=end)throw std::runtime_error("Invalid VGM metadata offset");end=gd;}
    if(start>=end||loop<start||loop>=end)throw std::runtime_error("Invalid VGM loop offset");
    uint64_t ticks=0,loopTick=0;size_t stop=0;bool seen=false;
    for(size_t p=start;p<end;){if(p==loop){seen=true;loopTick=ticks;}uint8_t c=b[p];size_t n=1;
      if(c==0x61||c==0x51||c==0x52||c==0x53||c==0x5a||c==0x5b||c==0xb8||c==0xbe||c==0xa0)n=3;
      else if(c==0xd2)n=4;else if(c==0xe0||c==0x90||c==0x91||c==0x95)n=5;else if(c==0x92)n=6;else if(c==0x93)n=11;
      else if(c==0x94||c==0x31||c==0x50||c==0x4f)n=2;
      else if(c==0x67){if(p+7>end||b[p+1]!=0x66)throw std::runtime_error("Invalid VGM data block");n=size_t(uint64_t(read32(b,p+3))+7);if(seen)throw std::runtime_error("VGM loop contains a data block; repeated bank semantics are not supported yet");}
      else if(c!=0x66&&c!=0x62&&c!=0x63&&!(c>=0x70&&c<=0x8f))throw std::runtime_error("Unsupported VGM loop command");
      if(n>end-p)throw std::runtime_error("Truncated VGM command");
      if(c==0x61)ticks+=uint32_t(b[p+1])|(uint32_t(b[p+2])<<8);else if(c==0x62)ticks+=735;else if(c==0x63)ticks+=882;else if(c>=0x70&&c<=0x7f)ticks+=(c&15)+1;else if(c>=0x80&&c<=0x8f)ticks+=c&15;
      if(c==0x66){stop=p;break;}p+=n;
    }
    if(!seen||!stop||loop>=stop||ticks<=loopTick)throw std::runtime_error("Invalid or empty VGM loop");
    const uint64_t total=ticks+(ticks-loopTick)*repeats;const uint64_t extra=uint64_t(stop-loop)*repeats;
    if(total>44100ull*3600||b.size()+extra>64ull*1024*1024||uint64_t(read32(b,4))+extra>UINT32_MAX||(gd&&uint64_t(read32(b,0x14))+extra>UINT32_MAX))throw std::runtime_error("Expanded VGM loop exceeds the export safety limit");
    std::vector<uint8_t> out;out.reserve(b.size()+size_t(extra));out.insert(out.end(),b.begin(),b.begin()+stop);for(unsigned i=0;i<repeats;++i)out.insert(out.end(),b.begin()+loop,b.begin()+stop);out.insert(out.end(),b.begin()+stop,b.end());
    put32(out,4,uint32_t(read32(b,4)+extra));if(gd)put32(out,0x14,uint32_t(read32(b,0x14)+extra));put32(out,0x18,uint32_t(total));put32(out,0x1c,0);put32(out,0x20,0);
    frames=(total*InternalRate+44099)/44100;return out;
}
StemExportResult exportStems(IMusicBackend& b,const std::filesystem::path& stage,const StemExportOptions& o,const std::atomic<bool>& cancel,const std::function<void(unsigned)>& progress){
    if(o.track<0||size_t(o.track)>=b.tracks().size()||!o.frames||o.plays!=1)throw std::runtime_error("Invalid export settings");
    if(o.frames>uint64_t((UINT32_MAX-36)/4)/o.plays)throw std::runtime_error("WAV output exceeds 4 GB limit");
    if(b.hasDeviceOutput())throw std::runtime_error("External MIDI device output cannot be exported as separate channels");
    if(!std::filesystem::is_directory(stage)||!std::filesystem::is_empty(stage))throw std::runtime_error("Export staging folder must be empty");
    b.selectTrack(o.track);b.muteMask64(0);auto sourceChannels=b.channels();
    if(sourceChannels.empty()||sourceChannels.size()>64)throw std::runtime_error("Unsupported channel count");
    uint64_t usage=0;const bool usageKnown=!o.mix&&b.channelUsage(usage);
    auto channels=sourceChannels;
    if(o.mix)channels.clear();
    const size_t masterIndex=channels.size();channels.push_back({"Master"});
    std::vector<Output> files(channels.size());
    for(size_t i=0;i<channels.size();++i){auto& f=files[i];f.path=stage/(i==masterIndex?std::wstring(L"master.wav"):std::to_wstring(i+1)+L"_"+safeName(channels[i].name)+L".wav");f.file.open(f.path,std::ios::binary|std::ios::trunc);if(!f.file)throw std::runtime_error("Cannot create WAV");header(f.file,uint32_t(o.frames*o.plays*4));}
    AudioBlock block;std::vector<char> encoded;uint64_t done=0,total=o.frames*o.plays;unsigned last=101;
    for(unsigned pass=0;pass<o.plays;++pass){if(pass)b.selectTrack(o.track);uint64_t left=o.frames;
      while(left){if(cancel.load())throw std::runtime_error("Export cancelled");size_t n=size_t(std::min<uint64_t>(left,4096));b.render(n,block);
        if(b.channels().size()!=sourceChannels.size()||block.voices.size()!=sourceChannels.size()||block.mix.size()!=n)throw std::runtime_error("Channel layout changed during export");
        encoded.resize(n*4);for(size_t c=0;c<channels.size();++c){const auto& samples=c==masterIndex?block.mix:block.voices[c];if(samples.size()!=n)throw std::runtime_error("Incomplete channel audio");auto& f=files[c];for(size_t i=0;i<n;++i){auto s=samples[i];uint16_t l=uint16_t(pcm(s.l)),r=uint16_t(pcm(s.r));f.audible|=(l!=0||r!=0);encoded[4*i]=char(l);encoded[4*i+1]=char(l>>8);encoded[4*i+2]=char(r);encoded[4*i+3]=char(r>>8);}f.file.write(encoded.data(),std::streamsize(encoded.size()));if(!f.file)throw std::runtime_error("WAV write failed");}
        left-=n;done+=n;unsigned pct=unsigned(done*100/total);if(progress&&pct!=last){last=pct;progress(pct);}
      }
    }
    StemExportResult result;for(size_t i=0;i<files.size();++i){auto&f=files[i];f.file.close();if(!f.file)throw std::runtime_error("WAV close failed");if(i==masterIndex||(f.audible&&(!usageKnown||(usage&(uint64_t(1)<<i)))))++result.written;else{std::filesystem::remove(f.path);++result.silent;}}
    return result;
}
}
