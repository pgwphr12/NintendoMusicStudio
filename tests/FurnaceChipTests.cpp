// SPDX-License-Identifier: GPL-2.0-or-later
#include "core/IMusicBackend.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
namespace ngmv {std::unique_ptr<IMusicBackend> makeExtendedVgmBackend(const std::vector<uint8_t>&);}
using namespace ngmv;
static void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
static void put(std::vector<uint8_t>&b,size_t p,uint32_t v){for(unsigned i=0;i<4;i++)b[p+i]=uint8_t(v>>(8*i));}
static std::vector<uint8_t> fixture(bool sn,bool low=false){
 std::vector<uint8_t>b(256);std::memcpy(b.data(),"Vgm ",4);put(b,8,0x172);put(b,0x34,0xcc);
 if(sn){put(b,0x0c,3579545);b[0x28]=9;b[0x2a]=16;
  for(auto v:{0x9f,0xbf,0xdf,0xff,0x84,0x10,0x90})b.insert(b.end(),{0x50,uint8_t(v)}); // tone260 ~430.23Hz
  b.insert(b.end(),{0x4f,0xff});
 }else{put(b,0x54,low?335582:3579545);auto w=[&](int r,int v){b.insert(b.end(),{0x5b,uint8_t(r),uint8_t(v)});};
  w(0x20,0x21);w(0x23,0x21);w(0x40,63);w(0x43,0);w(0x60,0xf0);w(0x63,0xf0);w(0x80,0);w(0x83,0);w(0xc0,1);w(0xa0,0x44);w(0xb0,0x2e); // fnum580 block3
 }
 b.insert(b.end(),{0x61,0x44,0xac,0x66});put(b,4,uint32_t(b.size()-4));put(b,0x18,44100);return b;
}
static void tone(bool sn,bool low){auto data=fixture(sn,low);auto core=sn?makeExtendedVgmBackend(data):makeBackend(data);need(core->channels().size()==(sn?4:14),"New-chip channel count");uint64_t usage=0;need(core->channelUsage(usage)&&usage==1,"Unused primary OPL/SN channels retained");AudioBlock b;std::vector<float>values;double err=0;
 for(unsigned n=0;n<12;n++){core->render(4000,b);for(size_t i=0;i<b.mix.size();i++){Stereo s{};for(auto&v:b.voices){s.l+=v[i].l;s.r+=v[i].r;}err=std::max(err,double(std::max(std::abs(s.l-b.mix[i].l),std::abs(s.r-b.mix[i].r))));values.push_back(b.mix[i].l);}}
 unsigned crossings=0;for(size_t i=8001;i<values.size();i++)if(values[i-1]<=0&&values[i]>0)crossings++;
 // Unipolar PSG crosses its mid-level rather than zero.
 if(sn){crossings=0;double mid=(*std::min_element(values.begin()+8000,values.end())+*std::max_element(values.begin()+8000,values.end()))/2;for(size_t i=8001;i<values.size();i++)if(values[i-1]<=mid&&values[i]>mid)crossings++;}
 double expected=sn?3579545./(32*260):(low?335582.:3579545.)/72*580*8/1048576;
 double hz=crossings*48000./40000;std::cout<<(sn?"SN76489":"YM3526")<<" clock="<<(low?335582:3579545)<<" Hz="<<hz<<" expected="<<expected<<" stemError="<<err<<'\n';need(std::abs(hz-expected)<3,"New-chip pitch mismatch");need(err<.001,"New-chip stems/mix mismatch");
 if(!sn&&low){size_t held=0;for(size_t i=8001;i<values.size();++i)held+=values[i]==values[i-1];double ratio=double(held)/(values.size()-8001);need(ratio>.8&&ratio<.99,"Low-clock OPL native sample hold was smoothed or frozen");std::cout<<"Native OPL held ratio="<<ratio<<'\n';}
 core->muteMask64(UINT64_MAX);core->seek(0);core->render(4096,b);for(auto v:b.mix)need(v.l==0&&v.r==0,"New-chip mute");
 core->muteMask64(0);core->seek(0);core->render(4096,b);auto first=b.mix;core->seek(0);core->render(4096,b);for(size_t i=0;i<first.size();i++)need(first[i].l==b.mix[i].l&&first[i].r==b.mix[i].r,"New-chip restart mismatch");
 if(sn){auto left=data;left[0x100+14+1]=0x10;auto c=makeExtendedVgmBackend(left);c->render(4096,b);double energy=0;for(auto v:b.mix){energy+=v.l*v.l;need(v.r==0,"SN stereo right not muted");}need(energy>0,"SN stereo left silent");}
}
int main(){try{tone(false,false);tone(false,true);tone(true,false);unsigned cases=0;auto reject=[&](std::vector<uint8_t>b){bool threw=false;try{auto c=makeExtendedVgmBackend(b);}catch(const std::exception&){threw=true;}need(threw,"Unsupported/malformed new-chip VGM accepted");cases++;};
 auto sn=fixture(true);for(size_t n=256;n<sn.size();n++){auto b=sn;b.resize(n);put(b,4,uint32_t(n-4));reject(b);}
 for(auto pair:{std::pair<size_t,uint32_t>{8,0x173},{0x0c,0x40369e99},{0x0c,0x80369e99},{0x54,0x40051ede},{0xe4,16000000}}){auto b=sn;put(b,pair.first,pair.second);reject(b);}
 for(unsigned flags:{32u,64u,128u}){auto b=sn;b[0x2b]=uint8_t(flags);reject(b);}for(unsigned width:{1u,32u,255u}){auto b=sn;b[0x2a]=uint8_t(width);reject(b);}
 {auto b=sn;b[0x28]=1;reject(b);} {auto b=sn;b[256]=0x5b;reject(b);} {auto b=fixture(false);b[256]=0x50;reject(b);}
 std::cout<<"PASS 3 clock/tone cases, stereo, stems, mute, replay and "<<cases<<" bounded negatives\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}




