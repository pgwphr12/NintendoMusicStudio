#include "editor/SampleBackend.hpp"
#include "editor/SampleSong.hpp"
#include "player/Player.hpp"
#include <cmath>
#include <cstring>
#include <iostream>
#include <thread>
using namespace ngmv;using namespace ngmv::editor;
void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
int main(){try{
    Song s;s.sampled=true;s.resolution=960;s.bars=1;s.instruments.resize(64);for(auto&i:s.instruments)i.sample=0;
    s.samples.push_back({"Own PCM",60,12000,true,0,std::make_shared<const std::vector<int8_t>>(std::initializer_list<int8_t>{0,64,-32,-64})});
    s.notes.push_back({0,0,960,60,12});s.notes.push_back({63,0,1920,72,10});
    s.notes[1].sourceOffset=.5;s.notes[1].controls=std::make_shared<const std::vector<NoteControl>>(std::initializer_list<NoteControl>{{0,1,1,.5},{480,2,.5,1}});
    auto core=makeSampleBackend(s);need(core->channels().size()==64&&core->tracks().size()==1&&core->tracks()[0].system=="Sampled project","Preview metadata/track count");
    SampleSongRenderer reference(s);std::vector<Stereo>expected;AudioBlock b;
    for(int i=0;i<20;i++){reference.render(127,expected);core->render(127,b);need(b.voices.size()==64&&b.mix.size()==127&&!std::memcmp(expected.data(),b.mix.data(),127*8),"Preview and WAV renderer diverge");for(int j=0;j<127;j++){double l=0,r=0;for(auto&v:b.voices){l+=v[j].l;r+=v[j].r;}need(std::abs(l-b.mix[j].l)<1e-7&&std::abs(r-b.mix[j].r)<1e-7,"Track histories do not sum to mix");}}
    core->seek(100);reference.seek(4800);core->render(512,b);reference.render(512,expected);need(!std::memcmp(expected.data(),b.mix.data(),4096),"Preview seek phase");
    core->selectTrack(0);core->muteMask(1);core->render(512,b);double low=0,high=0;for(auto x:b.voices[0])low+=x.l*x.l+x.r*x.r;for(auto x:b.voices[63])high+=x.l*x.l+x.r*x.r;need(low==0&&high>0,"32-bit legacy mask wrapped onto track63");
    core->seek(999999);core->render(512,b);for(auto x:b.mix)need(x.l==0&&x.r==0,"Preview end is not silent");bool failed=false;try{core->selectTrack(1);}catch(...){failed=true;}need(failed,"Invalid track accepted");
    Player player;player.openPrepared(makeSampleBackend(s),Format::SampledSong);player.play();std::this_thread::sleep_for(std::chrono::milliseconds(150));need(player.playing()&&player.seconds()>0&&player.error().empty(),"Prepared sampled Player callback");player.pause();auto before=player.seconds();std::this_thread::sleep_for(std::chrono::milliseconds(30));need(player.seconds()==before,"Preview pause clock");player.channel(63,{true,1});player.seek(50);player.play();std::this_thread::sleep_for(std::chrono::milliseconds(100));need(player.waveforms().size()==64&&player.error().empty(),"High-track mute/seek history");player.stop();player.releaseDevice();
    std::cout<<"PASS SAME sampled renderer/64tracks/voice-sum/phase+seek/mask/no-wrap/end/invalid-track/prepared-Player dummy callback/pause/high-track mute/history\n";return 0;
}catch(const std::exception&e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
