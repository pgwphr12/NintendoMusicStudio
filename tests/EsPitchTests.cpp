#include <array>
#include "core/IMusicBackend.hpp"
#include "formats/FurnaceEsRate.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
using namespace ngmv;
static void check(bool b,const char*s){if(!b)throw std::runtime_error(s);}
static void put(std::vector<uint8_t>&b,size_t p,uint32_t v){for(unsigned i=0;i<4;++i)b[p+i]=uint8_t(v>>(8*i));}
static std::vector<uint8_t> fixture(unsigned initial,bool legacy,bool marked,bool words=false,bool badSerial=false,bool later=false){
 std::vector<uint8_t>b(256);std::memcpy(b.data(),"Vgm ",4);put(b,8,0x171);put(b,0x34,0xcc);put(b,0xd0,0x80000000u|16000000);
 auto reg=[&](unsigned r,uint32_t v){for(unsigned i=0;i<4;++i)b.insert(b.end(),{0xbe,uint8_t(r*4+i),uint8_t(v>>(24-8*i))});};
 std::vector<uint8_t>data;for(unsigned i=0;i<8192;++i){int16_t v=int16_t((words?20000:100)*std::sin(i*6.283185307179586/128));if(words){data.push_back(uint8_t(v));data.push_back(uint8_t(v>>8));}else data.push_back(uint8_t(v));}
 b.insert(b.end(),{0x67,0x66,0x90});size_t p=b.size();b.resize(p+12);put(b,p,uint32_t(data.size()+8));put(b,p+4,uint32_t(data.size()));put(b,p+8,words?0:0x80000000u);b.insert(b.end(),data.begin(),data.end());
 reg(15,legacy?63:0);reg(11,initial);reg(11,31);
 reg(15,32);reg(10,1);reg(11,badSerial?16:17);reg(12,32);
 reg(1,0);reg(2,4096u<<11);reg(3,0);reg(15,0);reg(12,8);
 reg(1,2048);reg(2,0xffff);reg(4,0xffff);reg(7,0xffff);reg(9,0xffff);reg(0,0x308);
 if(later){b.insert(b.end(),{0x61,0x22,0x56});reg(11,31);b.insert(b.end(),{0x61,0x22,0x56});}
 else b.insert(b.end(),{0x61,0x44,0xac});
 b.push_back(0x66);put(b,0x18,44100);put(b,0x14,uint32_t(b.size()-0x14));
 b.insert(b.end(),{'G','d','3',' ',0,1,0,0});p=b.size();b.resize(p+4);size_t start=b.size();
 for(unsigned tag=0;tag<11;++tag){std::string text=tag==0?"Owned pitch test":tag==9?(marked?"Furnace (chiptune tracker)":"Other writer"):"";for(auto c:text){b.push_back(uint8_t(c));b.push_back(0);}b.push_back(0);b.push_back(0);}
 put(b,p,uint32_t(b.size()-start));put(b,4,uint32_t(b.size()-4));return b;
}
int main(){try{
 unsigned cases=0;
 for(auto item:{std::array<unsigned,6>{15,1,1,0,0,0}, {15,0,1,0,0,0}, {4,1,1,0,0,0}, {31,1,1,0,0,0}, {15,1,0,0,0,0}, {15,1,1,1,0,0}, {15,1,1,0,1,0}, {15,1,1,0,0,1}}){
  auto b=fixture(item[0],item[1],item[2],item[3],item[4],item[5]);auto core=makeBackend(b);AudioBlock out;std::vector<float> samples;double sumError=0;
  for(unsigned batch=0;batch<6;++batch){core->render(4000,out);for(size_t i=0;i<out.mix.size();++i){samples.push_back(out.mix[i].l);Stereo sum{};for(auto&v:out.voices){sum.l+=v[i].l;sum.r+=v[i].r;}sumError=std::max(sumError,double(std::max(std::abs(sum.l-out.mix[i].l),std::abs(sum.r-out.mix[i].r))));}}
  std::vector<double>crossings;for(size_t i=1000;i<samples.size();++i)if(samples[i-1]<=0&&samples[i]>0)crossings.push_back(i-1-samples[i-1]/double(samples[i]-samples[i-1]));
  check(crossings.size()>10,"Pitch test silent");double hz=48000*(crossings.size()-1)/(crossings.back()-crossings.front());
  unsigned voices=item[2]&&item[0]<31&&!item[4]&&!item[5]?item[0]+1:32;double expected=16000000./(16*voices*128);
  std::cout<<"case "<<cases++<<" Hz="<<hz<<" expected="<<expected<<" sum="<<sumError<<'\n';
  check(std::abs(hz/expected-1)<.0002,"Furnace/generic ES pitch contract failed");check(sumError<.00002,"Corrected ES stems do not sum to mix");
  core->selectTrack(0);core->render(4096,out);auto first=out.mix;core->seek(0);core->render(4096,out);for(size_t i=0;i<first.size();++i)check(first[i].l==out.mix[i].l&&first[i].r==out.mix[i].r,"Replay/seek changed corrected pitch");
  core->muteMask64(1);core->seek(0);core->render(4096,out);for(auto v:out.mix)check(v.l==0&&v.r==0,"Corrected ES mute failed");
 }
 FurnaceEsRate d;auto reg=[&](unsigned r,unsigned v,unsigned tick=0){for(unsigned i=0;i<4;++i)d.write(uint8_t(r*4+i),uint8_t(v>>(24-i*8)),tick);};
 reg(15,63);reg(11,15);reg(11,31);reg(15,32);reg(10,1);reg(11,17);reg(12,32);reg(15,0);reg(12,8);
 check(d.voices("Furnace (chiptune tracker)")==16,"Legacy export signature not found");check(d.voices("")==0,"Unknown producer corrected");
 reg(11,31,100);check(d.voices("Furnace (chiptune tracker)")==0,"Later dynamic ACTV was overridden");
 std::cout<<"PASS "<<cases<<" pitch cases: legacy/lower page,5/16/32voice clock,8/16bitROM,wrong creator/serial/later ACTV,stems,mute,seek\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

