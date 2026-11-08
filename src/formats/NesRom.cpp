#include "NesRom.hpp"
#include "GmeBackend.hpp"
#include "NesRomProfiles.hpp"
#include <algorithm>
#include <array>
#include <cstring>
#include <stdexcept>
#include <zlib.h>
namespace ngmv {
namespace {
constexpr const char*areaNames[]={"Ground","Water","Underground","Castle","Cloud","Pipe intro","Star"};
constexpr const char*eventNames[]={"Death","Game over","Victory","Castle clear","Alternate game over","Level clear","Time running out"};
void put16(std::vector<uint8_t>&b,size_t p,uint16_t n){b[p]=uint8_t(n);b[p+1]=uint8_t(n>>8);}
class NesRomBackend final:public IMusicBackend {
    std::vector<TrackInfo>tracks_;
    std::unique_ptr<IMusicBackend>core_;
    int track_=0;
    uint32_t mask_=0;
public:
    explicit NesRomBackend(const std::vector<uint8_t>&r){
        auto m=inspectNesRom(r);core_=std::make_unique<NSFBackend>(nesMusicImage(m));
        constexpr const char*hex[]={"01","02","04","08","10","20","40"};
        for(int kind=0;kind<2;kind++)for(int bit=0;bit<7;bit++)tracks_.push_back({
            std::string(kind?"Event 0x":"Area 0x")+hex[bit]+" - "+(kind?eventNames[bit]:areaNames[bit]),
            "Super Mario Bros. sound engine","",m.pal?"NES ROM / PAL":"NES ROM / NTSC",
            std::string("Original ")+(kind?"event":"area")+" music queue bit; aliases/jingles retained, stop command excluded. Original driver/data in user ROM; no game boot.",-1});
    }
    const std::vector<TrackInfo>&tracks()const override{return tracks_;}
    const std::vector<ChannelInfo>&channels()const override{return core_->channels();}
    void selectTrack(int i)override{if(i<0||i>=14)throw std::runtime_error("Invalid NES ROM music selection.");core_->selectTrack(i);track_=i;}
    void seek(int ms)override{
        if(ms<0)throw std::runtime_error("Invalid NES ROM seek time.");
        // Short seeks replay original state exactly. Long seeks use the existing
        // core's fast seek, then at most one second of real APU/filter warmup.
        // Do not linearly render every visible voice from zero for long seeks.
        if(ms<=1000)core_->selectTrack(track_);else core_->seek(ms-1000);
        core_->muteMask(mask_);AudioBlock discarded;
        size_t left=size_t(std::min(ms,1000))*InternalRate/1000;
        while(left){size_t n=std::min(left,size_t(4096));core_->render(n,discarded);left-=n;}
    }
    void muteMask(uint32_t mask)override{mask_=mask;core_->muteMask(mask);}
    void render(size_t n,AudioBlock&b)override{core_->render(n,b);}
    bool channelUsage(uint64_t&mask)const override{
        // Original engine modulates the4011 DAC only for area masks01/02.
        // Retain that real DMC/DAC output; other entries use the four base lanes.
        mask=track_<2?31:15;return true;
    }
};
}
NesRomMusic inspectNesRom(const std::vector<uint8_t>&r){
    if(r.size()<16||std::memcmp(r.data(),"NES\x1a",4))throw std::runtime_error("이 파일은 NES 카트리지 ROM이 아니거나 파일이 손상되었습니다.");
    if((r[7]&12)==8)throw std::runtime_error("이 NES ROM 형식은 아직 지원하지 않습니다. 현재 슈퍼 마리오 브라더스 1 계열의 일부 판본만 열 수 있습니다.");
    unsigned mapper=(r[6]>>4)|(r[7]&0xf0);
    if(mapper!=0||r[4]!=2||(r[6]&4))throw std::runtime_error("이 NES ROM 형식은 아직 지원하지 않습니다. 현재 슈퍼 마리오 브라더스 1 계열의 일부 판본만 열 수 있습니다.");
    size_t declared=16+size_t(r[4])*16384+size_t(r[5])*8192;
    if(r.size()<declared)throw std::runtime_error("NES ROM 파일의 내용이 잘려 있어 열 수 없습니다.");
    NesRomMusic m;m.program.assign(r.begin()+16,r.begin()+16+32768);m.programCrc=uint32_t(crc32(0,m.program.data(),unsigned(m.program.size())));
    auto p=std::find_if(std::begin(NesRomProfiles),std::end(NesRomProfiles),[&](auto x){return x.crc==m.programCrc;});
    if(p==std::end(NesRomProfiles))throw std::runtime_error("이 NES 게임 또는 판본의 음악은 아직 지원하지 않습니다. 현재 슈퍼 마리오 브라더스 1 계열의 등록된 판본만 지원합니다.");
    m.engine=p->engine;m.pal=p->pal;return m;
}
std::vector<uint8_t> nesMusicImage(const NesRomMusic&m){
    if(m.program.size()!=32768||m.engine<0xf000)throw std::runtime_error("Invalid NES ROM music program.");
    std::vector<uint8_t>b(128+32768);std::memcpy(b.data(),"NESM\x1a",5);b[5]=1;b[6]=14;b[7]=1;
    put16(b,8,0x8000);put16(b,10,0x8000);put16(b,12,0x8040);
    const char title[]="Original ROM sound driver";std::copy(std::begin(title),std::end(title),b.begin()+14);
    put16(b,0x6e,16639);put16(b,0x78,19997);b[0x7a]=m.pal?1:0;
    std::copy(m.program.begin(),m.program.end(),b.begin()+128);
    // Own6502 shim replaces only unused game-startup code in a private PRG
    // copy. All original music code/data stay intact. No original reset/NMI,
    // PPU/game execution, CHR graphics, trainer/save or extracted NSF file.
    constexpr uint8_t init[]={0xaa,0xa9,1,0x8d,0x70,7,0xbd,0x80,0x80,0x85,0xfb,0xbd,0x90,0x80,0x85,0xfc,0x60};
    std::copy(std::begin(init),std::end(init),b.begin()+128);
    uint8_t play[]={0xe6,9,0x20,uint8_t(m.engine),uint8_t(m.engine>>8),0x60};
    std::copy(std::begin(play),std::end(play),b.begin()+128+0x40);
    for(unsigned i=0;i<14;i++){b[128+0x80+i]=i<7?uint8_t(1u<<i):0;b[128+0x90+i]=i>=7?uint8_t(1u<<(i-7)):0;}
    return b;
}
std::unique_ptr<IMusicBackend> makeNesRomBackend(const std::vector<uint8_t>&r){return std::make_unique<NesRomBackend>(r);}
}
