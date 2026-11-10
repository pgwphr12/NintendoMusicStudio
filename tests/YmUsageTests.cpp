#include "player/Player.hpp"
#include <windows.h>
#include <cstring>
#include <fstream>
#include <iostream>
using namespace ngmv;
static void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
static void put(std::vector<uint8_t>&b,size_t p,uint32_t v){for(unsigned i=0;i<4;i++)b[p+i]=uint8_t(v>>(8*i));}
static std::vector<uint8_t> fixture(unsigned mode){
 std::vector<uint8_t>b(256);std::memcpy(b.data(),"Vgm ",4);put(b,8,0x171);put(b,0x34,0xcc);put(b,0x2c,7670454);put(b,0x50,4000000);
 auto w=[&](unsigned reg,unsigned value){b.insert(b.end(),{0x52,uint8_t(reg),uint8_t(value)});};auto wait=[&](unsigned n){b.insert(b.end(),{0x61,uint8_t(n),uint8_t(n>>8)});};
 w(0x28,0xf0);w(0x28,0); // zero-time reset pulse, never an actual note interval
 if(mode==0){w(0x28,0xf4);wait(1);w(0x28,4);wait(20000);w(0x28,0xf5);wait(1);w(0x28,5);w(0x28,0x11);wait(1);w(0x28,1);}
 if(mode==1){w(0x2b,128);wait(100);w(0x2b,0);} // enabled without sample writes
 if(mode==2){w(0x2a,128);w(0x2b,128);wait(100);w(0x2b,0);} // DC center only
 if(mode==3){w(0x2a,129);w(0x2b,128);wait(100);w(0x2b,0);} // real direct DAC
 if(mode==4){b.insert(b.end(),{0x67,0x66,0,1,0,0,0,129,0x90,0,2,0,0x2a,0x91,0,0,1,0,0x92,0,0x44,0xac,0,0});w(0x2b,128);b.insert(b.end(),{0x95,0,0,0,0});wait(100);b.insert(b.end(),{0x94,0});w(0x2b,0);}
 wait(735);b.push_back(0x66);put(b,4,uint32_t(b.size()-4));return b;
}
int wmain(int argc,wchar_t**argv){try{
 SetEnvironmentVariableW(L"SDL_AUDIODRIVER",L"dummy");auto out=std::filesystem::path(argv[1]);std::filesystem::create_directories(out);
 for(unsigned mode=0;mode<5;mode++){
  auto b=fixture(mode);auto core=makeBackend(b);uint64_t m=0;need(core->channelUsage(m),"No YM whole-program mask");uint64_t expected=mode==0?26:mode>=3?64:0;need(m==expected,"YM reset/FM/late/DAC usage wrong");
  auto file=out/("owned-ym-"+std::to_string(mode)+".vgm");{std::ofstream f(file,std::ios::binary);f.write(reinterpret_cast<const char*>(b.data()),b.size());}
  Player p;p.open(file);auto mask=[&]{uint64_t v=0;for(size_t i=0;i<p.channels().size();i++)if(p.channelVisible(i))v|=1ull<<i;return v;};need(mask()==expected,"YM channels appear only after audio instead of at load");
  for(size_t i=0;i<p.channels().size();i++)p.channel(i,{true,0});p.stop();p.play();p.pause();need(mask()==expected,"YM visibility follows live output/mute");
  std::cout<<"YM mode="<<mode<<" mask="<<m<<" preplay/late/brief-intro/reset/DAC/mute/pause PASS\n";
 }
 if(argc>2){auto file=std::filesystem::path(argv[2]);auto before=readMusicFile(file);Player p;p.open(file);need(!p.channelVisible(0)&&!p.channelVisible(1)&&!p.channelVisible(2)&&!p.channelVisible(6),"Unused actual YM1..3/DAC still visible");need(p.channelVisible(3)&&p.channelVisible(4)&&p.channelVisible(5),"Actual late YM not visible before its note");unsigned count=0;for(size_t i=0;i<p.channels().size();i++)count+=p.channelVisible(i);need(readMusicFile(file)==before,"Original music modified");std::cout<<"Actual YM1..3/DAC hidden, YM4..6 visible at load; visible="<<count<<" decoder="<<p.channels().size()<<" PASS\n";}
 std::cout<<"PASS YM2612 whole-program musical commands, no arbitrary intro cutoff or live audio filter\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
