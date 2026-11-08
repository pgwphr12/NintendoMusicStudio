#include "formats/GbaRom.hpp"
#include <cmath>
#include <iostream>
#include <algorithm>
using namespace ngmv;
void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
int wmain(int argc,wchar_t**argv){try{
 for(int a=1;a<argc;a++){
  auto r=readMusicFile(argv[a]);auto original=r;auto info=inspectGbaRom(r);
  _putenv_s("NGMV_GBA_HARDWARE_ONLY","1");auto raw=makeBackend(r,argv[a]);
  _putenv_s("NGMV_GBA_HARDWARE_ONLY","");auto split=makeBackend(r,argv[a]);
  const auto count=split->channels().size();need(count>6&&count<32,"ROM still exposes mixed PCM buses");
  need(split->channels()[4].name=="PCM VOICE 1","Wrong PCM slot labels");
  std::vector<double> energy(count);double error=0,total=0,reconstruct=0;AudioBlock x,y;std::vector<Stereo> start;
  for(int b=0;b<750;b++){
   raw->render(512,x);split->render(512,y);need(y.voices.size()==count,"Unstable voice layout");
   if(b<50)start.insert(start.end(),y.mix.begin(),y.mix.end());
   for(size_t i=0;i<512;i++){Stereo sum{};error=std::max({error,double(std::abs(x.mix[i].l-y.mix[i].l)),double(std::abs(x.mix[i].r-y.mix[i].r))});total+=y.mix[i].l*y.mix[i].l+y.mix[i].r*y.mix[i].r;for(size_t c=0;c<count;c++){auto v=y.voices[c][i];need(std::isfinite(v.l)&&std::isfinite(v.r),"Nonfinite voice");energy[c]+=v.l*v.l+v.r*v.r;sum.l+=v.l;sum.r+=v.r;}reconstruct=std::max({reconstruct,double(std::abs(sum.l-y.mix[i].l)),double(std::abs(sum.r-y.mix[i].r))});}
  }
  int active=0;for(size_t c=4;c+1<count;c++)active+=energy[c]>.01;
  auto residual=energy.back()/std::max(total,1e-12);need(error==0&&active>=3&&residual<.05&&reconstruct<.0001,"Lost mix or ineffective slot separation");
  raw.reset();split->selectTrack(0);split->render(512,y);for(int i=0;i<512;i++)need(y.mix[i].l==start[i].l&&y.mix[i].r==start[i].r,"Split replay mismatch");
  split->seek(500);split->render(512,y);for(int i=0;i<512;i++)need(y.mix[i].l==start[24000+i].l&&y.mix[i].r==start[24000+i].r,"Split seek mismatch");
  split->muteMask((1u<<count)-1);split->render(512,y);for(auto s:y.mix)need(s.l==0&&s.r==0,"All slot mute failed");
  split->muteMask(1u<<4);split->render(512,y);for(size_t i=0;i<512;i++){Stereo sum{};for(size_t c=0;c<count;c++)if(c!=4){sum.l+=y.voices[c][i].l;sum.r+=y.voices[c][i].r;}need(sum.l==y.mix[i].l&&sum.r==y.mix[i].r,"Individual slot mute failed");}
  split->muteMask(0);split->selectTrack(1);need(split->channels().size()>6,"Song switch fell back to mixed buses");split->render(512,y);
  need(r==original&&readMusicFile(argv[a])==original,"Source ROM changed");
  std::cout<<"PASS ROM="<<info.code<<" lanes="<<count<<" activePCM="<<active<<" original_mix_error="<<error<<" residual_ratio="<<residual<<" reconstruction_error="<<reconstruct<<" 8seconds/replay/seek/individual+allmute/song-switch/read-only\n";
 }
 return 0;
}catch(const std::exception&e){_putenv_s("NGMV_GBA_HARDWARE_ONLY","");std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}