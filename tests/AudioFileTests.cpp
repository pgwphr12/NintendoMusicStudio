#include "core/IMusicBackend.hpp"
#include "player/FolderPlaylist.hpp"
#include "player/Player.hpp"
#include <iostream>
#include <fstream>
#include <cmath>
#include <cstring>
#include <thread>
using namespace ngmv;
static void need(bool ok,const char*msg){if(!ok)throw std::runtime_error(msg);}
static double frequency(const std::vector<Stereo>&v,bool right=false){unsigned crosses=0;for(size_t i=1;i<v.size();++i){float a=right?v[i-1].r:v[i-1].l,b=right?v[i].r:v[i].l;if(a<=0&&b>0)++crosses;}return crosses*double(InternalRate)/v.size();}
static std::vector<Stereo> read(IMusicBackend&b,size_t n){AudioBlock out;std::vector<Stereo> all;while(n){size_t count=std::min(n,size_t(4096));b.render(count,out);for(size_t i=0;i<count;++i){auto x=out.mix[i];need(std::isfinite(x.l)&&std::isfinite(x.r),"Nonfinite audio");Stereo sum{};for(auto&v:out.voices){sum.l+=v[i].l;sum.r+=v[i].r;}need(std::abs(sum.l-x.l)<1e-6&&std::abs(sum.r-x.r)<1e-6,"Audio lanes do not recombine");}all.insert(all.end(),out.mix.begin(),out.mix.end());n-=count;}return all;}
int wmain(int argc,wchar_t**argv){try{
 need(argc==2,"Fixture directory required");std::filesystem::path dir=argv[1];
 for(auto name:{L"tone.wav",L"tone.mp3",L"tone.ogg",L"tone.flac",L"한글 파일.mp3",L"한글 파일.ogg",L"padded.MP3"}){
  auto path=dir/name;auto bytes=readMusicFile(path);need(bytes.size()==64&&identify(bytes,path)==Format::AudioFile,"Ordinary audio must use bounded header/streaming decode");
  auto b=makeBackend(bytes,path);need(b->channels().size()==2,"Stereo lanes");need(b->tracks()[0].lengthMs>=1900&&b->tracks()[0].lengthMs<2200,"Audio duration");
  auto first=read(*b,24000);auto pitchWindow=path.filename()==L"padded.MP3"?std::vector<Stereo>(first.begin()+4096,first.end()):first;double l=frequency(pitchWindow),r=frequency(pitchWindow,true);need(std::abs(l-440)<8&&std::abs(r-880)<8,"Initial stereo pitch/routing");
  if(path.extension()==L".wav"||path.extension()==L".flac")for(size_t i=0;i<first.size();++i){double expected=std::round(12000*std::sin(2*3.14159265358979323846*440*i/48000))/32768.;need(std::abs(first[i].l-expected)<2e-6,"Lossless source PCM changed");}
  b->seek(1250);auto second=read(*b,12000);need(std::abs(frequency(second)-660)<8&&std::abs(frequency(second,true)-990)<8,"Audio seek did not move to second musical section");
  b->selectTrack(0);auto again=read(*b,24000);need(again.size()==first.size(),"Replay length");double diff=0;for(size_t i=0;i<first.size();++i)diff+=std::abs(first[i].l-again[i].l);need(diff/first.size()<.0001,"Audio replay nondeterministic");
  AudioBlock block;b->seek(0);b->muteMask(1);b->render(4096,block);double re=0;for(auto x:block.mix){need(x.l==0,"Left mute failed");re+=x.r*x.r;}need(re>1,"Left mute removed right");
  b->muteMask(3);b->render(4096,block);for(auto x:block.mix)need(x.l==0&&x.r==0,"All audio mute");
  b->muteMask(0);b->seek(1750);for(int i=0;i<12;++i)b->render(4096,block);for(auto x:block.mix)need(x.l==0&&x.r==0,"EOF emitted stale audio");
  b->seek(b->tracks()[0].lengthMs);b->render(32,block);for(auto x:block.mix)need(x.l==0&&x.r==0,"Exact-end seek replayed audio");
  need(readMusicFile(path)==bytes,"Audio source header modified");std::cout<<path.filename().u8string()<<" stereo/seek/mute/EOF/replay PASS\n";
 }
 std::vector<uint8_t> knownGame(128);std::memcpy(knownGame.data(),"NESM\x1a",5);need(identify(knownGame,dir/L"renamed.mp3")==Format::Nsf,"Extension overrode known game signature");
 auto renamed=dir/L"renamed-game.mp3";auto rb=readMusicFile(renamed);need(rb.size()>128&&identify(rb,renamed)==Format::Nsf,"Header-only extension fallback hid known game file");auto rg=makeBackend(rb,renamed);need(rg->channels().size()>=4,"Renamed game routed to audio decoder");
 bool rejected=false;try{identify(std::vector<uint8_t>(64),dir/L"unknown.xyz");}catch(const std::exception&){rejected=true;}need(rejected,"Unknown bytes/extension accepted");
 auto mono=dir/L"mono22050.wav";auto b=makeBackend(readMusicFile(mono),mono);need(b->channels().size()==1,"Mono lane count");auto v=read(*b,24000);need(std::abs(frequency(v)-330)<8,"Mono resampling pitch");for(auto x:v)need(std::abs(x.l-x.r)<1e-6,"Mono stereo routing");
 auto fp=dir/L"float96000.wav";auto fb=makeBackend(readMusicFile(fp),fp);auto fv=read(*fb,24000);need(std::abs(frequency(fv)-330)<8&&std::abs(frequency(fv,true)-550)<8,"Float96k WAV resampling");
 for(auto name:{L"bad.wav",L"bad.ogg",L"bad.mp3"}){bool failed=false;try{auto f=dir/name;auto bad=makeBackend(readMusicFile(f),f);}catch(const std::exception&){failed=true;}need(failed,"Malformed ordinary audio accepted");}
 auto list=FolderPlaylist::scan(dir/L"tone.mp3",Format::AudioFile);need(list.files.size()>=6,"Mixed ordinary audio folder navigation");
 auto over=dir/L"overstated.mp3";auto ob=makeBackend(readMusicFile(over),over);need(ob->tracks()[0].lengthMs>10000,"Overstated-duration fixture invalid");AudioBlock chunk;for(int i=0;i<30;++i)ob->render(4096,chunk);uint64_t actualEnd=0;need(ob->decodedEndFrame(actualEnd)&&actualEnd>90000&&actualEnd<110000,"Actual audio EOF not exposed independently of wrong metadata");ob->selectTrack(0);need(!ob->decodedEndFrame(actualEnd),"Replay retained old EOF");
 Player player;player.open(dir/L"tone.ogg");player.play();std::this_thread::sleep_for(std::chrono::milliseconds(350));player.pause();need(player.seconds()>.15&&player.error().empty(),"Ordinary audio app worker failed");auto waves=player.waveforms();need(waves.size()==2,"Ordinary audio waveform lanes missing");
 std::cout<<"Mono/resample/errors/folder/dummy player/waveform PASS\n";return 0;
}catch(const std::exception&e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
