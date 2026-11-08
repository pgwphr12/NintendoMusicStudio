#include "player/Player.hpp"
#include <iostream>
#include <cmath>
#include <functional>
#include <chrono>
using namespace ngmv;
void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
struct Probe:IMusicBackend{
 std::vector<TrackInfo> t=std::vector<TrackInfo>(2);std::vector<ChannelInfo> c;std::function<void()> check;bool checked=false;
 Probe(int count,std::function<void()> f={}):check(std::move(f)){for(int i=0;i<count;i++)c.push_back({"Channel "+std::to_string(i+1)});}
 const std::vector<TrackInfo>&tracks()const override{return t;}const std::vector<ChannelInfo>&channels()const override{return c;}
 void selectTrack(int n)override{checked=false;c.resize(n?8:4);for(size_t i=0;i<c.size();i++)c[i].name="Channel "+std::to_string(i+1);}
 void seek(int)override{}void muteMask(uint32_t)override{}
 void render(size_t n,AudioBlock&b)override{if(check&&!checked){check();checked=true;}b.mix.assign(n,{.1f,.1f});b.voices.assign(c.size(),std::vector<Stereo>(n,{.01f,.01f}));}
};
struct MixedProbe:IMusicBackend{
 std::vector<TrackInfo> t=std::vector<TrackInfo>(2);std::vector<ChannelInfo> c={{"PCM MIX LEFT"},{"PCM MIX RIGHT"}};
 const std::vector<TrackInfo>&tracks()const override{return t;}const std::vector<ChannelInfo>&channels()const override{return c;}
 void selectTrack(int)override{}void seek(int)override{}void muteMask(uint32_t)override{}
 void render(size_t n,AudioBlock&b)override{b.mix.assign(n,{.1f,.2f});b.voices={std::vector<Stereo>(n,{.1f,0}),std::vector<Stereo>(n,{0,.2f})};}
};
int main(){try{
 _putenv_s("SDL_AUDIODRIVER","dummy");Player p;p.channelVolumes(.4f);
 auto uniform=[&](float v){auto states=p.states();need(!states.empty(),"No states");for(auto s:states)need(std::abs(s.volume-v)<.00001f,"Common volume not applied before first decoded audio");};
 auto open=[&](int n,Format f){p.openPrepared(std::make_unique<Probe>(n,[&]{uniform(.4f);}),f);uniform(.4f);need(p.error().empty(),"Initial render failed");p.pause();};
 open(4,Format::Gbs);p.channel(0,{true,.9f});p.select(1);uniform(.4f);need(p.states()[0].mute,"Group priority changed mute");need(p.states().size()==8,"Growing layout");p.select(0);uniform(.4f);p.stop();uniform(.4f);
 open(12,Format::GbaRom);open(2,Format::AudioFile);open(16,Format::Midi);
 p.channelVolumes(NAN);uniform(.4f);p.channelVolumes(0);uniform(0);need(p.commonChannelVolume()==0,"Zero default not retained");p.channelVolumes(3);uniform(2);p.channelVolumes(-1);uniform(0);
 p.channelVolumes(.4f);p.openPrepared(std::make_unique<MixedProbe>(),Format::Gsf);
 p.channel(0,{false,.25f});p.channel(1,{false,.75f});
 auto mixed=[&]{auto v=p.states();need(v.size()==2&&std::abs(v[0].volume-.25f)<.00001f&&std::abs(v[1].volume-.75f)<.00001f,"Same-song restart overwrote mixed gains");};
 for(int cycle=0;cycle<3;++cycle){
  p.stop();mixed();p.play();std::this_thread::sleep_for(std::chrono::milliseconds(180));p.pause();mixed();
  SpectrumAnalyzer::Samples pcm{};need(p.spectrumSamples(pcm)>4096,"No actual internal output");
  for(size_t i=SpectrumAnalyzer::Frames-512;i<SpectrumAnalyzer::Frames;++i){need(std::abs(pcm[i*2]-.025f)<.0002f,"Left mixed output gain changed");need(std::abs(pcm[i*2+1]-.15f)<.0002f,"Right mixed output gain changed");}
  p.seek(100);mixed();
 }
 p.channel(0,{true,.25f});p.stop();mixed();need(p.states()[0].mute,"Stop discarded mute");
 p.select(1);uniform(.4f);need(p.states()[0].mute,"New song discarded mute");
 std::cout<<"PASS mixed stop/play/seek3cycles with actual internal PCM gain retained; new-song common priority; common gain before first render/across4formats/track growth-shrink/rewind/priority over individual gains/mute preservation/0..200clamps/NaN ignored\n";return 0;
}catch(const std::exception&e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}