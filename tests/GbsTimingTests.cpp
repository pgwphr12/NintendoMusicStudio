#include <gme.h>
#include <vector>
#include <cmath>
#include <iostream>
#include <stdexcept>
void need(bool b,const char*m){if(!b)throw std::runtime_error(m);}
int main(){try{
 for(int mode=0;mode<3;++mode){
 std::vector<unsigned char>b(112);b[0]='G';b[1]='B';b[2]='S';b[3]=1;b[4]=b[5]=1;b[7]=b[9]=b[11]=4;b[12]=0;b[13]=0xdf;
 auto write=[&](int reg,int value){b.insert(b.end(),{0x3e,(unsigned char)value,0xe0,(unsigned char)reg});};
 write(0x26,0x80);write(0x24,0x77);write(0x25,0x44);write(0x1a,0);
 for(int i=0;i<16;++i)write(0x30+i,255);
 write(0x1a,0x80);write(0x1c,0x20);write(0x1d,255);write(0x1e,0x87);
 // An independently counted SM83 delay-loop period, including taken/not-taken
 // JR, CALL/RET and register/BIT(HL)/RES(HL) CB timings.
 b.insert(b.end(),{0x21,0x00,0xc0});
 size_t loop=b.size();std::vector<size_t> calls;
 for(int level:{0,32}){
  b.insert(b.end(),{0x3e,(unsigned char)level,0xe0,0x1c});
  if(mode){calls.push_back(b.size()+1);b.insert(b.end(),{0xcd,0,0});}
  else b.insert(b.end(),{0x06,128,0x05,0x20,0xfd});
 }
 b.push_back(0x18);int relative=int(loop)-int(b.size()+1);b.push_back((unsigned char)relative);
 if(mode){unsigned addr=0x400+unsigned(b.size()-112);for(auto at:calls){b[at]=addr&255;b[at+1]=addr>>8;}
  if(mode==2)b.insert(b.end(),{0xcb,0x7c,0xcb,0x46,0xcb,0x86});
  b.insert(b.end(),{0x06,128,0x05,0x20,0xfd,0xc9});}
 Music_Emu* e=nullptr;need(!gme_open_data(b.data(),long(b.size()),&e,48000),"Own GBS load");gme_ignore_silence(e,1);gme_enable_accuracy(e,1);need(!gme_start_track(e,0),"Own GBS start");std::vector<short>samples(48000*2);need(!gme_play(e,int(samples.size()),samples.data()),"Own GBS render");
 int crossings=0,first=0,last=0;for(int i=5001;i<47000;++i)if(samples[(i-1)*2]<=0&&samples[i*2]>0){if(!crossings)first=i;last=i;++crossings;}
 double hz=(crossings-1)*48000./(last-first),expected=4194304./(mode==0?4156:mode==1?4236:4308);std::cout<<"mode="<<mode<<" CPU timed NR32 tone="<<hz<<" expected="<<expected<<'\n';need(std::abs(hz/expected-1)<.002,"CPU delay loop has wrong instruction durations");need(!gme_warning(e),"CPU warning");gme_delete(e);}
 std::cout<<"PASS3 generated GBS delay loops with conditionalJR/CALL/RET/CB register and memory timing\n";return 0;
 }catch(const std::exception&e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
