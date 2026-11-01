#include "core/IMusicBackend.hpp"
#include "player/Player.hpp"
#include <chrono>
#include <cmath>
#include <iostream>
#include <thread>
namespace ngmv {std::unique_ptr<IMusicBackend> makePsfBackend149(const std::filesystem::path&,Format);}
using namespace ngmv;
void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
int wmain(int argc,wchar_t**argv){try{need(argc>1,"Provide own/readonly GSF cases");for(int a=1;a<argc;a++){
    std::filesystem::path p=argv[a];auto bytes=readMusicFile(p);auto begin=std::chrono::steady_clock::now();auto old=makePsfBackend149(p,Format::Gsf);double oldLoad=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count();
    std::vector<AudioBlock>baseline(563);begin=std::chrono::steady_clock::now();for(auto&b:baseline)old->render(512,b);double oldRender=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count();old.reset();
    begin=std::chrono::steady_clock::now();auto current=makeBackend(bytes,p);double load=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count();AudioBlock b;double mixError=0,voiceError=0;begin=std::chrono::steady_clock::now();
    size_t blockIndex=0;bool reported=false;for(auto&ref:baseline){current->render(512,b);for(size_t i=0;i<b.mix.size();i++){mixError=std::max({mixError,double(std::abs(b.mix[i].l-ref.mix[i].l)),double(std::abs(b.mix[i].r-ref.mix[i].r))});for(size_t c=0;c<(b.voices.size()==ref.voices.size()?b.voices.size():size_t(4));c++){double delta=std::max(std::abs(b.voices[c][i].l-ref.voices[c][i].l),std::abs(b.voices[c][i].r-ref.voices[c][i].r));if(delta&&!reported){std::cout<<"first_stem_difference frame="<<blockIndex*512+i<<" channel="<<c<<" old="<<ref.voices[c][i].l<<" new="<<b.voices[c][i].l<<'\n';reported=true;}voiceError=std::max(voiceError,delta);}}blockIndex++;}
    double render=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count();uint32_t used=0;need(current->gbaPsgUsed(used),"Missing source PSG activity");
    std::cout<<"case="<<a<<" channels="<<current->channels().size()<<" PSG_used="<<used<<" old_load_ms="<<oldLoad<<" new_load_ms="<<load<<" old_render_ms="<<oldRender<<" new_render_and_compare_ms="<<render<<" mix_error="<<mixError<<" voice_error="<<voiceError<<'\n';need(mixError==0&&voiceError==0,"Audio-only PSG mirror differs from CPU baseline");
    auto first=baseline[0].mix;current->selectTrack(0);current->render(512,b);for(size_t i=0;i<b.mix.size();i++)need(b.mix[i].l==first[i].l&&b.mix[i].r==first[i].r,"Replay changed original");current->seek(300);current->muteMask((1u<<current->channels().size())-1);for(int i=0;i<100;i++)current->render(512,b);for(auto x:b.mix)need(std::abs(x.l)<.001&&std::abs(x.r)<.001,"Mute failed");
    current.reset();baseline.clear();
}
{
    Player player;player.open(argv[1]);player.play();std::this_thread::sleep_for(std::chrono::milliseconds(250));need(player.channelVisible(0)&&player.channelVisible(1)&&!player.channelVisible(2)&&!player.channelVisible(3),"Real pulse-only GSF visibility");player.pause();player.open(argv[2]);need(!player.channelVisible(0)&&!player.channelVisible(1)&&!player.channelVisible(2)&&!player.channelVisible(3)&&player.channelVisible(4)&&player.channelVisible(5),"PCM-only GSF visibility");auto delayed=std::filesystem::path(argv[1]).parent_path()/L"psg-delayed.gsf";if(std::filesystem::exists(delayed)){player.open(delayed);need(!player.channelVisible(2)&&!player.channelVisible(3),"Late PSG was visible before use");player.play();for(int wait=0;wait<650&&(!player.channelVisible(2)||!player.channelVisible(3));wait++)std::this_thread::sleep_for(std::chrono::milliseconds(10));need(player.channelVisible(2)&&player.channelVisible(3),"Late PSG remained permanently hidden");player.pause();player.seek(0);need(!player.channelVisible(2)&&!player.channelVisible(3),"Replay retained future activity");}player.releaseDevice();
}
std::cout<<"PASS original/isolated exact audio, source activity, replay/seek/mute; times include comparison overhead, hardware audio not tested\n";return 0;
}catch(const std::exception&e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
