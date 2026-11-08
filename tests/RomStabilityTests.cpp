#include "player/Player.hpp"
#include "formats/GbaRom.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <algorithm>
#include <cmath>
using namespace ngmv;
void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
struct Changing:IMusicBackend{
 std::vector<TrackInfo> t=std::vector<TrackInfo>(3);std::vector<ChannelInfo> c;bool malformed=false;
 Changing(bool bad=false):malformed(bad){selectTrack(0);}
 const std::vector<TrackInfo>&tracks()const override{return t;}const std::vector<ChannelInfo>&channels()const override{return c;}
 void selectTrack(int n)override{c.clear();for(int i=0;i<(n==1?6:12);i++)c.push_back({"VOICE "+std::to_string(i)});}
 void seek(int)override{}void muteMask(uint32_t)override{}
 void render(size_t n,AudioBlock&b)override{b.mix.assign(n,{.1f,.1f});b.voices.assign(c.size()-(malformed?1:0),std::vector<Stereo>(n,{.01f,.01f}));}
};
int wmain(int argc,wchar_t**argv){try{
 if(argc>1&&std::wstring(argv[1])==L"--catalog"){
  for(int a=2;a<argc;a++){auto r=readMusicFile(argv[a]);auto m=inspectGbaRom(r);auto b=makeBackend(r,argv[a]);size_t prior=b->channels().size();int changes=0,errors=0;AudioBlock out;for(size_t i=0;i<m.songs.size();i++){try{b->selectTrack(int(i));auto count=b->channels().size();if(count!=prior){std::cout<<"LAYOUT code="<<m.code<<" index="<<i<<" song="<<m.songs[i].id<<" old="<<prior<<" new="<<count<<std::endl;changes++;}prior=count;b->render(512,out);}catch(const std::exception&e){std::cout<<"SONG_ERROR code="<<m.code<<" song="<<m.songs[i].id<<" message="<<e.what()<<std::endl;errors++;}}std::cout<<"CATALOG code="<<m.code<<" songs="<<m.songs.size()<<" layout_changes="<<changes<<" errors="<<errors<<std::endl;}
 }else if(argc>2&&std::wstring(argv[1])==L"--sparse"){
  auto r=readMusicFile(argv[2]);auto m=inspectGbaRom(r);_putenv_s("NGMV_GBA_HARDWARE_ONLY","1");auto raw=makeBackend(r,argv[2]);_putenv_s("NGMV_GBA_HARDWARE_ONLY","");auto split=makeBackend(r,argv[2]);
  for(unsigned id:{199u,200u,235u,237u,247u,248u}){auto it=std::find_if(m.songs.begin(),m.songs.end(),[&](auto x){return x.id==id;});if(it==m.songs.end())continue;int index=int(it-m.songs.begin());_putenv_s("NGMV_GBA_HARDWARE_ONLY","1");raw->selectTrack(index);_putenv_s("NGMV_GBA_HARDWARE_ONLY","");split->selectTrack(index);need(raw->channels().size()==6&&split->channels().size()==11,"Sparse song lost known PCM slots");AudioBlock a,b;double error=0;for(int n=0;n<50;n++){raw->render(512,a);split->render(512,b);for(size_t i=0;i<512;i++)error=std::max({error,double(std::abs(a.mix[i].l-b.mix[i].l)),double(std::abs(a.mix[i].r-b.mix[i].r))});}need(error==0,"Sparse output changed original mix");std::cout<<"PASS sparse song="<<id<<" stable11lanes raw6-vs-split original_mix_error="<<error<<std::endl;}need(readMusicFile(argv[2])==r,"Original changed");
 }else if(argc>2&&std::wstring(argv[1])==L"--player"){
  _putenv_s("SDL_AUDIODRIVER","dummy");auto r=readMusicFile(argv[2]);auto m=inspectGbaRom(r);Player p;p.open(argv[2]);auto initial=p.channels().size();p.channel(0,{false,.75f});
  for(unsigned id:{199u,200u,235u,237u,247u,248u,1u,170u}){auto it=std::find_if(m.songs.begin(),m.songs.end(),[&](auto x){return x.id==id;});if(it==m.songs.end())continue;p.select(int(it-m.songs.begin()));need(p.error().empty(),"Actual song playback error");need(p.channels().size()==initial&&p.states().size()==initial,"Actual PCM layout changed");p.seek(100);p.stop();std::cout<<"PASS Player song="<<id<<" lanes="<<p.channels().size()<<" state="<<p.states().size()<<" volume/seek/stop"<<std::endl;}
 }else{
  _putenv_s("SDL_AUDIODRIVER","dummy");Player p;p.openPrepared(std::make_unique<Changing>(),Format::GbaRom);p.channel(0,{false,.75f});
  for(int n=0;n<16;n++){p.select(1);need(p.error().empty(),"Shrink playback error");need(p.states().size()==6,"State count did not shrink");need(p.states()[0].volume==.75f,"Common channel setting lost");p.select(2);need(p.states().size()==12,"State count did not grow");need(p.states()[6].volume==1.f&&!p.states()[6].mute,"New channel has stale setting");need(p.error().empty(),"Growth playback error");}
  bool rejected=false;try{p.openPrepared(std::make_unique<Changing>(true),Format::GbaRom);}catch(const std::exception&){rejected=true;}need(rejected,"Malformed audio layout was not rejected");std::cout<<"PASS 16shrink-grow cycles/shared settings/new channel defaults/malformed decoder safe rejection"<<std::endl;
 }
 return 0;
}catch(const std::exception&e){std::cerr<<"FAIL "<<e.what()<<std::endl;return 1;}}