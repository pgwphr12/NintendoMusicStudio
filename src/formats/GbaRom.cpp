#include "GbaRom.hpp"
#include "PsfBackend.hpp"
#include <algorithm>
#include <array>
#include <cstring>
#include <stdexcept>
#include <zlib.h>
namespace ngmv {
namespace {
uint32_t word(const std::vector<uint8_t>&r,size_t p){
    if(p>r.size()||r.size()-p<4)throw std::runtime_error("Truncated GBA music data.");
    return uint32_t(r[p])|uint32_t(r[p+1])<<8|uint32_t(r[p+2])<<16|uint32_t(r[p+3])<<24;
}
void put(std::vector<uint8_t>&r,size_t p,uint32_t v){for(int i=0;i<4;i++)r[p+i]=uint8_t(v>>(i*8));}
bool pointer(uint32_t p,size_t n){return p>=0x08000000u&&uint64_t(p)<0x08000000ull+n;}
struct Profile {uint32_t crc, select, vsync;};
constexpr Profile profiles[]={
    {0xc95824c4,0xb52a4,0xb4bd8}, {0x69924cbd,0xb52a4,0xb4bd8},
    {0x1e4c6d6a,0xb551c,0xb4e50}, {0xb993b92f,0xb50e8,0xb4a1c}
};
struct NintendoProfile {
    uint32_t crc, size, init, start, main, vsync, update, table;
    unsigned entries, sequenceLimit, allowedFlags;
};
constexpr NintendoProfile nintendoProfiles[]={
    {0xfcfef343,4194304,0x9be50,0x9bf40,0x9c678,0x9c66c,0x9be6c,0x1b3f04,48,46,0x0b},
    {0x5206880a,4194304,0x9b258,0x9b348,0x9ba80,0x9ba74,0x9b274,0x177064,48,46,0x0b},
    {0x639e9d3b,8388608,0x12ed9c,0x12ef3c,0x12fae4,0x12fad8,0x12edb8,0x3a7830,40,40,0x51},
    {0x40a48276,4194304,0x12c214,0x12c3b4,0x12cf5c,0x12cf50,0x12c230,0x3077e4,40,40,0x51},
    {0xedebea60,8388608,0xe4414,0xe4598,0xe4de8,0xe4ddc,0xe4430,0x263cac,57,55,0x03},
    {0xf3c87306,4194304,0xe2418,0xe259c,0xe2dec,0xe2de0,0xe2434,0x263808,57,55,0x03},
    {0x37141f32,8388608,0xe7908,0xe7a8c,0xe82dc,0xe82d0,0xe7924,0x3084dc,57,55,0x03},
    {0xa44ba79f,4194304,0xe3bf4,0xe3d78,0xe45c8,0xe45bc,0xe3c10,0x2630e0,57,55,0x03},
    {0x88dab27f,4194304,0xe4414,0xe4598,0xe4de8,0xe4ddc,0xe4430,0x263cac,57,55,0x03}
};
class GbaRomBackend final:public IMusicBackend {
    std::vector<uint8_t> rom_;
    GbaRomMusic music_;
    std::vector<TrackInfo> tracks_;
    std::unique_ptr<IMusicBackend> core_;
    uint32_t mask_=0;
public:
    explicit GbaRomBackend(const std::vector<uint8_t>&r):rom_(r),music_(inspectGbaRom(r)){
        for(auto s:music_.songs)tracks_.push_back({"Song "+std::to_string(s.id),music_.title,"","Game Boy Advance ROM",
            "Original ROM song ID "+std::to_string(s.id)+(s.sequences?"; "+std::to_string(s.sequences)+" sequence tracks.":"; Nintendo driver, sequence-track count unavailable.")+" Includes jingles/aliases; original titles/durations unavailable.",-1});
        selectTrack(0);
    }
    const std::vector<TrackInfo>&tracks()const override{return tracks_;}
    const std::vector<ChannelInfo>&channels()const override{return core_->channels();}
    void selectTrack(int i)override{
        if(i<0||size_t(i)>=music_.songs.size())throw std::runtime_error("Invalid ROM song selection.");
        auto candidate=makeGbaImageBackend(gbaMusicImage(rom_,music_,music_.songs[i].id),music_.title);
        candidate->muteMask(mask_);core_=std::move(candidate);
    }
    void seek(int ms)override{core_->seek(ms);}
    void muteMask(uint32_t mask)override{mask_=mask;core_->muteMask(mask);}
    void render(size_t n,AudioBlock&b)override{core_->render(n,b);}
    bool gbaPsgActivity(uint32_t&v)const override{return core_->gbaPsgActivity(v);}
    bool gbaPsgUsed(uint32_t&v)const override{return core_->gbaPsgUsed(v);}
};
}
GbaRomMusic inspectGbaRom(const std::vector<uint8_t>&r){
    if(r.size()<0xc0||r.size()>32*1024*1024||r[0xb2]!=0x96)throw std::runtime_error("Invalid or unsupported GBA ROM header.");
    GbaRomMusic m;m.crc=uint32_t(crc32(0,r.data(),unsigned(r.size())));
    auto p=std::find_if(std::begin(profiles),std::end(profiles),[&](auto x){return x.crc==m.crc;});
    auto n=std::find_if(std::begin(nintendoProfiles),std::end(nintendoProfiles),[&](auto x){return x.crc==m.crc&&x.size==r.size();});
    if((p==std::end(profiles)||r.size()!=4*1024*1024)&&n==std::end(nintendoProfiles))throw std::runtime_error("This ROM driver/revision is not supported yet. Verified Super Mario Advance1-4 revisions only; this is not universal ROM support. GSF files remain supported.");
    m.title.assign(reinterpret_cast<const char*>(r.data()+0xa0),12);m.title.erase(std::find(m.title.begin(),m.title.end(),'\0'),m.title.end());
    m.code.assign(reinterpret_cast<const char*>(r.data()+0xac),4);
    if(n!=std::end(nintendoProfiles)){
        m.driver=GbaRomDriver::Nintendo;m.init=n->init;m.start=n->start;m.main=n->main;m.vsync=n->vsync;m.update=n->update;
        if(n->table>r.size()||n->entries>(r.size()-n->table)/2)throw std::runtime_error("Truncated Nintendo music-ID table.");
        for(unsigned id=0;id<n->entries;id++){
            size_t at=n->table+id*2;unsigned descriptor=r[at]|unsigned(r[at+1])<<8;
            // Real game IDs can alias the same sequence; the upper bits are
            // original playback flags, not another song or channel count.
            unsigned flags=descriptor>>8;
            if(flags>=8||!(n->allowedFlags&(1u<<flags))||(descriptor&255)>=n->sequenceLimit)throw std::runtime_error("Invalid Nintendo music-ID mapping.");
            m.songs.push_back({id,0}); // Unknown track count, never synthesize metadata.
        }
        return m;
    }
    m.start=p->select;m.init=p->select-0x98;m.main=p->select-12;m.vsync=p->vsync;
    uint32_t table=word(r,m.start+40);
    if(!pointer(table,r.size()))throw std::runtime_error("Invalid GBA song table.");
    size_t t=table-0x08000000u;
    for(unsigned id=0;id<4096&&t+size_t(id)*8+8<=r.size();id++){
        uint32_t ptr=word(r,t+id*8);if(!pointer(ptr,r.size()))break;
        size_t h=ptr-0x08000000u;if(r.size()-h<8)break;
        unsigned count=r[h];if(count>16||r.size()-h<8+count*4)break;
        if(!count)continue; // Deliberate empty song-table entries, not invented tracks.
        if(!pointer(word(r,h+4),r.size()))break;
        bool valid=true;for(unsigned j=0;j<count;j++)valid=valid&&pointer(word(r,h+8+j*4),r.size());
        if(!valid)break;
        m.songs.push_back({id,count});
    }
    if(m.songs.empty())throw std::runtime_error("No valid songs in this GBA ROM.");
    // These profiles share title-song ID170. Begin there rather than with a
    // very short sound effect; keep every real ID visible without inventing names.
    auto title=std::find_if(m.songs.begin(),m.songs.end(),[](auto s){return s.id==170;});
    if(title!=m.songs.end())std::rotate(m.songs.begin(),title,m.songs.end());
    return m;
}
std::vector<uint8_t> gbaMusicImage(const std::vector<uint8_t>&r,const GbaRomMusic&m,unsigned song){
    if(std::none_of(m.songs.begin(),m.songs.end(),[&](auto s){return s.id==song;}))throw std::runtime_error("ROM song is empty or out of range.");
    // Own ARM bootstrap appended to a private memory copy. Only the reset
    // branch is replaced. Calls execute the user's original driver in emulation.
    // Poll VCOUNT for one update per frame; no game boot, save file, BIOS or IRQ hook.
    std::vector<uint32_t>w;struct Fix{size_t i;unsigned reg;uint32_t value;};std::vector<Fix>fix;
    auto emit=[&](uint32_t x){w.push_back(x);};
    auto load=[&](unsigned reg,uint32_t value){fix.push_back({w.size(),reg,value});emit(0);};
    auto call=[&](uint32_t offset){load(3,0x08000001u+offset);emit(0xe1a0e00f);emit(0xe12fff13);};
    call(m.init);load(0,song);call(m.start);load(4,0x04000006);
    size_t loop=w.size();emit(0xe1d400b0);emit(0xe35000a0);emit(0x0afffffc);
    emit(0xe1d400b0);emit(0xe35000a0);emit(0x1afffffc);
    call(m.vsync);if(m.update)call(m.update);call(m.main);emit(0xea000000u|uint32_t((int64_t(loop)-int64_t(w.size())-2)&0xffffff));
    for(auto f:fix){w[f.i]=0xe59f0000u|(f.reg<<12)|uint32_t(w.size()*4-(f.i*4+8));emit(f.value);}
    auto image=r;size_t base=image.size();image.resize(base+w.size()*4);
    put(image,0,0xea000000u|uint32_t((base-8)/4));for(size_t i=0;i<w.size();i++)put(image,base+i*4,w[i]);
    return image;
}
std::unique_ptr<IMusicBackend> makeGbaRomBackend(const std::vector<uint8_t>&r){return std::make_unique<GbaRomBackend>(r);}
}
