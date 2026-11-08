#include "formats/NesRom.hpp"
#include "formats/GmeBackend.hpp"
#include <cmath>
#include <iostream>
#include <set>
#include <chrono>
#include <stdexcept>
using namespace ngmv;
void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
template<class F>void rejects(F f){bool b=false;try{f();}catch(const std::exception&){b=true;}need(b,"Expected failure");}
double render(IMusicBackend&b,int blocks,std::vector<Stereo>*save=nullptr){AudioBlock a;double energy=0;for(int n=0;n<blocks;n++){b.render(512,a);need(a.voices.size()==5,"Five real APU lanes");for(auto s:a.mix){need(std::isfinite(s.l)&&std::isfinite(s.r),"Nonfinite PCM");energy+=s.l*s.l+s.r*s.r;}if(save)save->insert(save->end(),a.mix.begin(),a.mix.end());}return energy;}
int wmain(int argc,wchar_t**argv){try{
    std::vector<uint8_t>bad(16);rejects([&]{inspectNesRom(bad);});
    bad[0]='N';bad[1]='E';bad[2]='S';bad[3]=0x1a;bad[4]=2;bad[5]=1;
    need(identify(bad,L"own.nes")==Format::NesRom,"NES identification");rejects([&]{inspectNesRom(bad);});
    bad.resize(16+32768+8192);bad[6]=0x10;rejects([&]{inspectNesRom(bad);});bad[6]=4;rejects([&]{inspectNesRom(bad);});bad[6]=0;bad[7]=8;rejects([&]{inspectNesRom(bad);});bad[7]=0;rejects([&]{inspectNesRom(bad);});
    NesRomMusic own;own.engine=0xf000;own.program.assign(32768,0x60);
    // Own6502 signal generator observes both init queues through APU timers.
    const uint8_t signal[]={0xa9,3,0x8d,0x15,0x40,0xa9,0xbf,0x8d,0,0x40,0xa5,0xfb,0x05,0xfc,0x09,0x40,0x8d,2,0x40,0xa9,1,0x8d,3,0x40,0xa9,0xbf,0x8d,4,0x40,0xa5,0xfc,0x09,0x80,0x8d,6,0x40,0xa9,0,0x8d,7,0x40,0x60};
    std::copy(std::begin(signal),std::end(signal),own.program.begin()+0x7000);auto before=own.program;auto image=nesMusicImage(own);need(own.program==before,"Changed source PRG");need(std::equal(before.begin()+0x100,before.end(),image.begin()+128+0x100),"Changed music/program outside private bootstrap");
    NSFBackend fixture(image);need(fixture.tracks().size()==14,"Queue catalog size");std::vector<Stereo>x,y;fixture.selectTrack(0);need(render(fixture,20,&x)>1,"Own area queue failed");fixture.selectTrack(7);need(render(fixture,20,&y)>1,"Own event queue failed");bool different=false;for(size_t i=0;i<x.size();i++)different|=x[i].l!=y[i].l||x[i].r!=y[i].r;need(different,"Init queues not distinguished");
    own.pal=true;auto pal=nesMusicImage(own);need(pal[0x7a]==1&&pal[0x78]==0x1d&&pal[0x79]==0x4e,"PAL cadence metadata");
    std::cout<<"PASS own header/mapper/trainer/NES2/revision failures, source preservation, area/event queue execution and PAL metadata\n";
    bool longSeek=argc>1&&std::wstring(argv[1])==L"--long-seek";
    for(int input=longSeek?2:1;input<argc;input++){
        auto rom=readMusicFile(argv[input]),original=rom;auto info=inspectNesRom(rom);auto b=makeBackend(rom,argv[input]);need(b->tracks().size()==14,"Original command catalog");
        if(longSeek){auto begin=std::chrono::steady_clock::now();b->seek(60000);need(render(*b,188)>.1,"Long seek lost original looping music");double elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-begin).count();b->seek(0);need(render(*b,20)>.1,"Rewind after long seek failed");std::cout<<"PASS long seek60000ms / looping original driver / rewind PAL="<<info.pal<<" offline_wall_seconds="<<elapsed<<" (not a timing guarantee)\n";continue;}
        std::vector<Stereo>first;std::set<uint64_t>fingerprints;double minEnergy=1e30;for(int t=0;t<14;t++){b->selectTrack(t);uint64_t mask=0;need(b->channelUsage(mask)&&mask==uint64_t(t<2?31:15),"Whole-song APU usage metadata");std::vector<Stereo>song;double e=render(*b,188,&song);need(e>.1,"Silent real music command");minEnergy=std::min(minEnergy,e);if(!t)first=song;uint64_t h=1469598103934665603ull;for(auto s:song){h^=uint32_t(int32_t(s.l*32768));h*=1099511628211ull;h^=uint32_t(int32_t(s.r*32768));h*=1099511628211ull;}fingerprints.insert(h);}need(fingerprints.size()>=8,"Catalog entries collapsed to the same music");
        b->selectTrack(0);std::vector<Stereo>replay;render(*b,188,&replay);for(size_t i=0;i<first.size();i++)need(first[i].l==replay[i].l&&first[i].r==replay[i].r,"Deterministic replay failed");
        b->seek(500);AudioBlock a;b->render(512,a);double delta=0;for(size_t i=0;i<512;i++)delta=std::max(delta,double(std::max(std::abs(a.mix[i].l-first[24000+i].l),std::abs(a.mix[i].r-first[24000+i].r))));need(delta<.04,"Seek position changed beyond existing emulator filter tolerance");
        b->muteMask(31);render(*b,100);need(render(*b,4)<.0001,"NES mute failed");rejects([&]{b->selectTrack(-1);});rejects([&]{b->selectTrack(14);});
        auto changed=rom;if(changed.size()>16+32768)changed[16+32768]^=255;else changed[6]^=1;auto gfx=makeBackend(changed,L"graphics-only.nes");std::vector<Stereo>same;render(*gfx,188,&same);for(size_t i=0;i<first.size();i++)need(same[i].l==first[i].l&&same[i].r==first[i].r,"Graphics affected music");changed=rom;changed[16+info.engine-0x8000]^=1;rejects([&]{makeBackend(changed,L"unknown-program.nes");});
        need(rom==original&&readMusicFile(argv[input])==original,"Original ROM changed");
        std::cout<<"PASS original PRG CRC="<<std::hex<<info.programCrc<<std::dec<<" PAL="<<info.pal<<" 14commands distinct_two_second_outputs="<<fingerprints.size()<<" min_energy="<<minEnergy<<" replay/seek(delta="<<delta<<")/mute/CHR-independent/read-only; beginnings only\n";
    }
    return 0;
}catch(const std::exception&e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
