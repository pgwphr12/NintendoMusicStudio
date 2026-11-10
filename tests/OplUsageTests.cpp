#include "player/Player.hpp"
#include <windows.h>
#include <cstring>
#include <fstream>
#include <iostream>
using namespace ngmv;
static void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
static void put(std::vector<uint8_t>&b,size_t p,uint32_t v){for(unsigned i=0;i<4;i++)b[p+i]=uint8_t(v>>(8*i));}
static std::vector<uint8_t> fixture(bool opl1,bool both=false){
 std::vector<uint8_t>b(256);std::memcpy(b.data(),"Vgm ",4);put(b,8,0x172);put(b,0x34,0xcc);put(b,opl1?0x54:0x50,3579545);if(both)put(b,0x54,3579545);
 auto w=[&](unsigned r,unsigned v,unsigned cmd=0){b.insert(b.end(),{uint8_t(cmd?cmd:opl1?0x5b:0x5a),uint8_t(r),uint8_t(v)});};auto wait=[&](unsigned n){b.insert(b.end(),{0x61,uint8_t(n),uint8_t(n>>8)});};
 w(0xb4,0x20);wait(735);w(0xb4,0);wait(22050);w(0xb1,0x20);wait(735);w(0xb1,0);w(0xbd,0x21);wait(735);w(0xbd,0);
 // Key-on/off at a single tick has no sustained usage; rhythm-mapped melodic
 // channels6..8 must not be confused with percussion slots9..13.
 w(0xb7,0x20);w(0xb7,0);if(both){w(0xb8,0x20,0x5b);wait(735);w(0xb8,0,0x5b);}
 b.push_back(0x66);put(b,4,uint32_t(b.size()-4));put(b,0x18,both?24990:24255);return b;
}
int wmain(int argc,wchar_t**argv){try{
 SetEnvironmentVariableW(L"SDL_AUDIODRIVER",L"dummy");auto out=std::filesystem::path(argv[1]);std::filesystem::create_directories(out);
 for(unsigned mode=0;mode<3;mode++){
  auto b=fixture(mode==1,mode==2);auto core=makeBackend(b);uint64_t mask=0;need(core->channelUsage(mask),"No whole-song metadata");uint64_t expected=(1ull<<1)|(1ull<<4)|(1ull<<13);if(mode==2)expected|=1ull<<22;
  need(mask==expected,"Primary OPL late/rhythm/dual-kind usage incorrect");need(core->channels().size()==(mode==2?28:14),"Hiding deleted decoder channels");
  auto file=out/("owned-opl-"+std::to_string(mode)+".vgm");{std::ofstream f(file,std::ios::binary);f.write(reinterpret_cast<const char*>(b.data()),b.size());}
  Player p;p.open(file);auto visible=[&]{uint64_t m=0;for(size_t i=0;i<p.channels().size();i++)if(p.channelVisible(i))m|=1ull<<i;return m;};need(visible()==expected,"Later channels not visible at initial load");
  for(size_t i=0;i<p.channels().size();i++)p.channel(i,{true,0});p.seek(300);need(visible()==expected,"Mute/zero volume/seek changed list");p.stop();need(visible()==expected,"Rewind changed list");p.play();p.pause();need(visible()==expected,"Live playback changed list");
  std::cout<<"OPL mode="<<mode<<" mask="<<mask<<" late/FM/rhythm/preplay/mute/seek/stop PASS\n";
 }
 if(argc>2){Player p;auto file=std::filesystem::path(argv[2]);auto before=readMusicFile(file);p.open(file);unsigned count=0;for(size_t i=0;i<p.channels().size();i++)if(p.channelVisible(i)){count++;std::cout<<p.channels()[i].name<<'\n';}need(count<p.channels().size(),"Actual multi-chip file did not hide unused channels");need(readMusicFile(file)==before,"Source file modified");std::cout<<"Actual visible="<<count<<" decoder="<<p.channels().size()<<" at initial load PASS\n";}
 std::cout<<"PASS primary OPL1/OPL2 and combined independent full-song masks; no live amplitude filtering\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
