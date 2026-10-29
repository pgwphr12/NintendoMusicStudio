#include "editor/NoteImport.hpp"
#include "editor/SampleSong.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <thread>
#include <stdexcept>
using namespace ngmv;using namespace ngmv::editor;
static void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
static Song own(){
 Song s;s.sampled=true;s.bars=1;s.resolution=120;s.title="Own sample composition";s.instruments.resize(8);
 auto a=std::make_shared<std::vector<int8_t>>(std::initializer_list<int8_t>{0,64,127,64,0,-64,-128,-64});
 auto b=std::make_shared<std::vector<int8_t>>(std::initializer_list<int8_t>{127,100,75,50,25,0,-25,-50});
 s.samples.push_back({"Own loop",60,48000,true,0,a});s.samples.push_back({"Own one-shot",60,24000,false,0,b});
 for(int i=0;i<8;i++){s.instruments[i].volume=15;s.instruments[i].sample=0;s.instruments[i].name="Track "+std::to_string(i+1);}
 s.put({6,0,120,60,15,-1,0});s.put({6,30,120,67,7,-1,0});s.put({7,60,60,48,12,-1,1});return s;
}
static std::vector<Stereo>render(const Song&s,int block=4096){
 SampleSongRenderer renderer(s);std::vector<Stereo>out;out.reserve(size_t(renderer.frames()));
 while(renderer.position()<renderer.frames()){std::vector<Stereo>part;renderer.render(size_t(std::min<uint64_t>(block,renderer.frames()-renderer.position())),part);out.insert(out.end(),part.begin(),part.end());}return out;
}
static bool same(const std::vector<Stereo>&a,const std::vector<Stereo>&b){return a.size()==b.size()&&!std::memcmp(a.data(),b.data(),a.size()*sizeof(Stereo));}
static void reject(const Song&s){bool bad=false;try{s.validate();}catch(const std::exception&){bad=true;}need(bad,"Invalid sample Song accepted");}
static std::string read(const std::filesystem::path&p){std::ifstream f(p,std::ios::binary);return {(std::istreambuf_iterator<char>(f)),{}};}
int wmain(int argc,wchar_t**argv){try{
 need(argc==2,"Usage: sample_song_tests own-output-dir");std::filesystem::path out=argv[1];std::filesystem::create_directories(out);
 Song s=own();auto text=s.serialize();need(text.rfind("NGMS 3\n",0)==0&&Song::parse(text).serialize()==text,"Sample project roundtrip");need(s.notes.size()==3&&s.notes[0].channel==6&&s.notes[1].channel==6,"Source-track polyphony deleted or reduced to NES4");
 std::ofstream(out/L"own-sample.ngms")<<text;auto restored=Song::parse(read(out/L"own-sample.ngms"));
 need(restored.samples[0].pcm!=s.samples[0].pcm&&*restored.samples[0].pcm==*s.samples[0].pcm,"Project depends on original sample pointer");
 auto base=render(restored);need(same(base,render(s,17)),"Block boundaries changed sample rendering");
 need(base[0].l==0&&base[1].l==.125f&&base[2].l==float(127./128*.25)&&base[2].l==base[2].r,"Known-rate/root signed PCM does not match expected audio");
 SampleSongRenderer renderer(s);renderer.seek(5997);std::vector<Stereo>part;renderer.render(2048,part);for(size_t i=0;i<part.size();i++)need(part[i].l==base[5997+i].l&&part[i].r==base[5997+i].r,"Seek/overlap audio changed");
 renderer.seek(renderer.frames());renderer.render(32,part);for(auto x:part)need(x.l==0&&x.r==0,"Renderer does not silence at end");
 renderer.seek(0);renderer.render(2048,part,uint64_t(1)<<6);for(auto x:part)need(x.l==0&&x.r==0,"Track6 mask failed");
 // One-shot sample stops on its own data boundary; looping is not fabricated.
 auto shot=s;shot.notes={s.notes.back()};shot.notes[0].step=0;auto one=render(shot);need(one[0].l>0&&one[32].l==0&&one[1024].l==0,"One-shot lifetime incorrect");
 auto loop=s;loop.notes={s.notes[0]};auto lp=*loop.samples[0].pcm;loop.samples[0].loopStart=2;auto looped=render(loop);need(looped[8].l==float(lp[2]/128.*.25),"Loop start not respected");
 // Actual note edits on the SAME Song, persisted then rendered through the same WAV API.
 std::atomic<bool>cancel{false};std::atomic<int>progress{0};exportSongWav(restored,out/L"base.wav",cancel,progress);need(progress==100,"Sample WAV progress");
 auto wav=read(out/L"base.wav");need(wav.size()==56+base.size()*8&&!std::memcmp(wav.data(),"RIFF",4)&&wav[20]==3&&!std::memcmp(wav.data()+36,"fact",4)&&!std::memcmp(wav.data()+48,"data",4),"Sample WAV layout/duration");need(!std::memcmp(wav.data()+56,base.data(),base.size()*8),"WAV does not use common Song renderer");
 for(int edit=0;edit<8;edit++){
  auto changed=restored;
  if(edit==0)changed.notes[0].step+=12;
  if(edit==1)changed.notes[0].length-=12;
  if(edit==2)changed.notes[0].pitch+=12;
  if(edit==3)changed.notes[0].velocity=3;
  if(edit==4)changed.notes[0].sample=1;
  if(edit==5)changed.instruments[6].volume=4;
  if(edit==6)changed.instruments[6].mute=true;
  if(edit==7)changed.instruments[6].pan=100;
  auto saved=changed.serialize();changed=Song::parse(saved);auto pcm=render(changed);need(!same(base,pcm),"Note/instrument edit made no sound change");
  auto path=out/(L"edit-"+std::to_wstring(edit)+L".wav");exportSongWav(changed,path,cancel,progress);auto bytes=read(path);need(bytes.size()==56+pcm.size()*8&&!std::memcmp(bytes.data()+56,pcm.data(),pcm.size()*8),"Saved note edit was not reflected in exported WAV");
 }
 // Song snapshots used by common undo retain immutable sample storage but independent notes.
 auto undo=s;s.notes[0].pitch=80;need(undo.notes[0].pitch==60&&undo.samples[0].pcm==s.samples[0].pcm,"Common undo snapshot semantics");s=undo;
 auto before=s.serialize();try{s.put({63,0,1,60,15});}catch(const std::exception&){}need(s.serialize()==before,"Invalid sample edit was not atomic");
 auto wide=s;wide.instruments.resize(64,wide.instruments[0]);wide.notes={s.notes[0]};wide.notes[0].channel=63;SampleSongRenderer lastTrack(wide);lastTrack.render(16,part,uint64_t(1)<<63);for(auto x:part)need(x.l==0&&x.r==0,"Track63 mask failed");
 auto dense=s;dense.notes.assign(256,s.notes[0]);dense.validate();SampleSongRenderer maximum(dense);maximum.render(3,part);need(part[2].l==float(256*127./128*.25),"Supported polyphony clipped or silently lost voices");
 bool noNsf=false;try{compileNsf(s);}catch(const std::exception&e){noNsf=std::string(e.what()).find("cannot be compiled")!=std::string::npos;}need(noNsf,"Sample song silently substituted with NES4");
 auto bad=s;bad.samples[0].sampleRate=std::numeric_limits<double>::quiet_NaN();reject(bad);bad=s;bad.samples[0].sampleRate=0;reject(bad);bad=s;bad.samples[0].loopStart=8;reject(bad);bad=s;bad.samples[0].pcm.reset();reject(bad);bad=s;bad.notes[0].sample=256;reject(bad);bad=s;bad.instruments[0].sample=-1;reject(bad);bad=s;bad.instruments.resize(65);reject(bad);bad=s;bad.notes[0].pitch=128;reject(bad);bad=s;bad.instruments[0].pan=101;reject(bad);bad=s;bad.notes.assign(257,bad.notes[0]);reject(bad);bad=s;bad.samples[0].pcm=std::make_shared<const std::vector<int8_t>>(8u*1024u*1024u+1);reject(bad);
 for(auto invalid:{text+"extra",std::string("NGMS 3\n120 120 1 0 \"bad\"\n65 1\n"),std::string("NGMS 3\n120 120 1 0 \"bad\"\n1 999999\n")}){bool rejected=false;try{Song::parse(invalid);}catch(const std::exception&){rejected=true;}need(rejected,"Malformed sample project accepted");}
 auto corrupt=text;auto hex=corrupt.find("00407f4000c080c0");need(hex!=std::string::npos,"Own PCM fixture hex missing");corrupt[hex]='x';bool malformed=false;try{Song::parse(corrupt);}catch(const std::exception&){malformed=true;}need(malformed,"Bad PCM hex accepted");
 auto keep=out/L"cancel.wav";std::ofstream(keep)<<"KEEP";cancel=true;bool cancelled=false;try{exportSongWav(s,keep,cancel,progress);}catch(const std::exception&){cancelled=true;}need(cancelled&&read(keep)=="KEEP","Cancelled export replaced existing destination");cancel=false;
 // Mid-export cancellation occurs after the first real progress update.
 auto longSong=s;longSong.bars=512;progress=0;std::atomic<bool>finished{false},sawProgress{false};
 std::thread canceller([&]{while(progress.load()==0&&!finished.load())std::this_thread::yield();if(!finished.load()&&progress.load()<100){sawProgress=true;cancel=true;}});
 cancelled=false;try{exportSongWav(longSong,keep,cancel,progress);}catch(const std::exception&){cancelled=true;}finished=true;canceller.join();
 need(sawProgress&&cancelled&&read(keep)=="KEEP","Mid-export cancellation did not preserve destination");cancel=false;
 for(auto&entry:std::filesystem::directory_iterator(out))need(entry.path().filename().wstring().find(L".sample-tmp-")==std::wstring::npos,"Cancelled export leaked temporary file");
 auto legacy=Song::parse("NGMS 1\n120 1 0 \"legacy\"\n2 10 0\n2 10 0\n2 10 0\n2 10 0\n1\n0 0 4 60 15\n");need(!legacy.sampled&&legacy.notes[0].duty==-1&&Song::parse(legacy.serialize()).serialize()==legacy.serialize(),"Legacy versions1/2 not preserved");
 std::cout<<"PASS shared Song v3/8 tracks/polyphony/owned bank/project roundtrip/known PCM/loop/one-shot/block/seek/mask/8 independent edits/WAV exact samples/undo/strict validation/cancel/legacy1-2/NSF rejection\n";return 0;
 }catch(const std::exception&e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
