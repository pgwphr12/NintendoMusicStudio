#include "formats/GbaRom.hpp"
#include "formats/PsfBackend.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <numeric>
#include <stdexcept>
using namespace ngmv;
void need(bool v,const char*s){if(!v)throw std::runtime_error(s);}
template<class F>void rejects(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}need(caught,"Expected rejection");}
double render(IMusicBackend&b,int count,std::vector<Stereo>*save=nullptr,double*ac=nullptr){AudioBlock out;double e=0,sumL=0,sumR=0,centered=0;size_t n=0;for(int i=0;i<count;i++){b.render(512,out);need(out.voices.size()==b.channels().size(),"Stable exposed channels");for(auto s:out.mix){need(std::isfinite(s.l)&&std::isfinite(s.r),"Finite PCM");e+=s.l*s.l+s.r*s.r;if(i>=8){sumL+=s.l;sumR+=s.r;centered+=s.l*s.l+s.r*s.r;n++;}}if(save)save->insert(save->end(),out.mix.begin(),out.mix.end());}if(ac)*ac=n?std::max(0.,centered-(sumL*sumL+sumR*sumR)/n):0;return e;}
int wmain(int argc,wchar_t**argv){try{
    std::vector<uint8_t>small(128);rejects([&]{inspectGbaRom(small);});
    std::vector<uint8_t>own(65536);own[0xb2]=0x96;rejects([&]{inspectGbaRom(own);});
    need(identify(own,L"own.GBA")==Format::GbaRom,"GBA extension identification");
    GbaRomMusic m;m.init=0x200;m.start=0x202;m.main=0x204;m.vsync=0x206;m.songs={{7,1}};
    for(size_t p=0x200;p<0x20a;p+=2){own[p]=0x70;own[p+1]=0x47;} // Own Thumb bx lr stubs.
    auto before=own;auto image=gbaMusicImage(own,m,7);need(own==before,"Input mutation");need(image.size()>own.size(),"Bootstrap missing");need(std::equal(own.begin()+4,own.end(),image.begin()+4),"ROM body changed");
    rejects([&]{gbaMusicImage(own,m,8);});
    auto stub=makeGbaImageBackend(std::move(image),"Own silent driver test");need(render(*stub,4)==0,"Own return-only driver should be silent");
    m.update=0x208;auto updated=gbaMusicImage(own,m,7);need(updated.size()>own.size(),"Optional frame update bootstrap missing");
    auto updateStub=makeGbaImageBackend(std::move(updated),"Own frame-update driver test");need(render(*updateStub,4)==0,"Own update driver should be silent");
    std::cout<<"PASS own header/revision rejection, ID bounds, private bootstrap/body preservation, real emulated return-only driver\n";
    bool sweep=argc>1&&std::wstring(argv[1])==L"--sweep";
    for(int a=sweep?2:1;a<argc;a++){
        auto r=readMusicFile(argv[a]);auto original=r;auto info=inspectGbaRom(r);auto b=makeBackend(r,argv[a]);
        need(b->tracks().size()==info.songs.size(),"Song table count");
        bool nintendo=info.driver==GbaRomDriver::Nintendo;
        if(nintendo){size_t expected=info.code.substr(0,3)=="AA2"?48:info.code.substr(0,3)=="A3A"?40:57;need(info.songs.size()==expected,"Nintendo catalog range");need(info.songs.front().id==0,"Nintendo initial song ID");for(auto s:info.songs)need(s.sequences==0,"Invented Nintendo sequence count");}
        auto index=[&](unsigned id){auto i=std::find_if(info.songs.begin(),info.songs.end(),[&](auto s){return s.id==id;});need(i!=info.songs.end(),"Known song missing");return int(i-info.songs.begin());};
        unsigned firstId=nintendo?0:171,otherId=nintendo?1:170;
        b->selectTrack(index(firstId));std::vector<Stereo>first;double e=render(*b,188,&first);need(e>1,"First music silent");
        b->selectTrack(index(otherId));std::vector<Stereo>other;double e2=render(*b,188,&other);need(e2>1,"Second music silent");bool different=false;for(size_t i=0;i<first.size();i++)different=different||first[i].l!=other[i].l||first[i].r!=other[i].r;need(different,"Songs identical");
        b->selectTrack(index(firstId));std::vector<Stereo>replay;render(*b,188,&replay);for(size_t i=0;i<first.size();i++)need(first[i].l==replay[i].l&&first[i].r==replay[i].r,"Replay mismatch");
        b->seek(500);AudioBlock seek;b->render(512,seek);for(size_t i=0;i<512;i++)need(seek.mix[i].l==first[24000+i].l&&seek.mix[i].r==first[24000+i].r,"Seek mismatch");
        b->muteMask((1u<<b->channels().size())-1);render(*b,100);need(render(*b,4)<.0001,"Mute failed");
        rejects([&]{b->selectTrack(-1);});rejects([&]{b->selectTrack(int(info.songs.size()));});
        need(r==original&&readMusicFile(argv[a])==original,"Original ROM changed");
        std::cout<<"PASS ROM "<<info.code<<" CRC="<<std::hex<<info.crc<<std::dec<<" entries="<<info.songs.size()<<" song"<<firstId<<"_energy="<<e<<" song"<<otherId<<"_energy="<<e2<<" distinct/replay/seek/mute/read-only\n";
        if(sweep){unsigned nonzero=0,dynamic=0;double minAc=1e30;b->muteMask(0);for(size_t s=0;s<info.songs.size();s++){b->selectTrack(int(s));double ac=0;if(render(*b,188,nullptr,&ac)>.001)nonzero++;if(ac>.01)dynamic++;else std::cout<<"LOW_AC id="<<info.songs[s].id<<" ac_energy="<<ac<<'\n';minAc=std::min(minAc,ac);}std::cout<<"PASS SWEEP entries="<<info.songs.size()<<" finite_two_second_starts="<<info.songs.size()<<" nonzero="<<nonzero<<" nonconstant_after_first85ms="<<dynamic<<" minimum_ac_energy="<<minAc<<"; not full-song listening\n";}
    }
    return 0;
}catch(const std::exception&e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
