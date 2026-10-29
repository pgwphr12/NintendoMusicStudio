#include "editor/NoteImport.hpp"
#include "editor/SampleSong.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace ngmv;using namespace ngmv::editor;
static void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
static auto controls(std::initializer_list<NoteControl>points){return std::make_shared<const std::vector<NoteControl>>(points);}
static Song own(){
 Song s;s.sampled=true;s.bars=1;s.resolution=120;s.title="Own automation fixture";
 auto pcm=std::make_shared<const std::vector<int8_t>>(std::initializer_list<int8_t>{0,32,64,96,127,-96,-64,-32});
 s.samples.push_back({"Own loop",60,48000,true,0,pcm});s.samples.push_back({"Own other loop",60,24000,true,0,std::make_shared<const std::vector<int8_t>>(std::initializer_list<int8_t>{127,80,40,0,-40,-80,-128,0})});
 for(auto&i:s.instruments){i.sample=0;i.volume=15;}
 Note n{0,0,120,60,15};n.sourceOffset=.5;n.controls=controls({{0,1,1,1},{.02,2,.5,0},{.04,.5,0,2}});s.put(n);return s;
}
static std::vector<Stereo>render(const Song&s,int block=4096){SampleSongRenderer r(s);std::vector<Stereo>result;while(r.position()<r.frames()){std::vector<Stereo>part;r.render(size_t(std::min<uint64_t>(block,r.frames()-r.position())),part);result.insert(result.end(),part.begin(),part.end());}return result;}
static bool same(const std::vector<Stereo>&a,const std::vector<Stereo>&b){return a.size()==b.size()&&!std::memcmp(a.data(),b.data(),a.size()*8);}
static std::string read(const std::filesystem::path&p){std::ifstream f(p,std::ios::binary);return {(std::istreambuf_iterator<char>(f)),{}};}
static void rejected(const Song&s){bool failed=false;try{s.validate();}catch(const std::exception&){failed=true;}need(failed,"Invalid automation accepted");}
int wmain(int argc,wchar_t**argv){try{
 need(argc==2,"Usage: automation_tests own-output-dir");std::filesystem::path out=argv[1];std::filesystem::create_directories(out);auto s=own();auto text=s.serialize();need(text.rfind("NGMS 4\n",0)==0,"Automation did not choose version4");
 std::ofstream(out/L"own-automation.ngms")<<text;auto saved=Song::parse(read(out/L"own-automation.ngms"));need(saved.serialize()==text&&saved.notes[0].sourceOffset==.5&&saved.notes[0].controls->size()==3,"Automation save/reopen changed data");
 auto audio=render(saved);double source[]={16,48,80,111.5,15.5,-48,16,80,15.5,-96,-80,-64};
 for(size_t i=0;i<12;i++){double l=i<4?1:i<8?.5:0,r=i<4?1:i<8?0:2;need(audio[i].l==float(source[i]/128.*.25*l)&&audio[i].r==float(source[i]/128.*.25*r),"Known fractional source/continuous phase/stereo control samples differ");}
 need(same(audio,render(saved,1))&&same(audio,render(saved,7)),"Automation depends on audio block boundaries");
 SampleSongRenderer seek(saved);for(uint64_t at:{0ull,3ull,4ull,7ull,8ull,9ull,11991ull}){seek.seek(at);std::vector<Stereo>part;seek.render(37,part);for(size_t i=0;i<part.size();i++)need(part[i].l==audio[size_t(at)+i].l&&part[i].r==audio[size_t(at)+i].r,"Seek reset or skipped source automation phase");}
 auto snapshot=saved;auto edited=saved;edited.notes[0].controls=controls({{0,1,0,0}});need(snapshot.notes[0].controls==saved.notes[0].controls&&snapshot.serialize()==text&&!same(audio,render(edited)),"Immutable control/undo snapshot semantics");
 // Musical note edits keep controls relative to the note. Every edit is
 // saved/reopened and compared with the SAME common WAV export pipeline.
 std::atomic<bool>cancel{false};std::atomic<int>progress{0};
 for(int edit=0;edit<8;edit++){
  auto changed=saved;
  if(edit==0)changed.notes[0].pitch+=12;
  if(edit==1)changed.notes[0].velocity=6;
  if(edit==2)changed.notes[0].step=12;
  if(edit==3)changed.notes[0].length=1;
  if(edit==4)changed.notes[0].sample=1;
  if(edit==5)changed.notes[0].sourceOffset=2.25;
  if(edit==6)changed.notes[0].controls=controls({{0,.25,.3,.8},{.02,3,1,0}});
  if(edit==7)changed.bpm=240;
  changed=Song::parse(changed.serialize());auto result=render(changed);need(!same(result,audio),"Automation note/instrument edit did not change sound");
  auto path=out/(L"automation-edit-"+std::to_wstring(edit)+L".wav");exportSongWav(changed,path,cancel,progress);auto bytes=read(path);need(progress==100&&bytes.size()==56+result.size()*8&&!std::memcmp(bytes.data()+56,result.data(),result.size()*8),"Saved automated Song is not rendered into exported WAV");
 }
 auto shifted=saved;shifted.notes[0].step=1;auto moved=render(shifted);need(moved[199].l==0&&moved[201].l==audio[1].l&&moved[204].l==audio[4].l,"Move lost relative automation");
 auto pitch=saved;pitch.notes[0].pitch+=12;need(render(pitch)[1].l==float(80./128*.25),"Pitch edit ignored rate automation or reset offset");
 auto tempo=saved;tempo.bpm=240;auto fast=render(tempo);need(fast[1].l==audio[1].l&&fast[2].r==0&&audio[2].r!=0,"Tempo did not move control ticks independently of pitch");
 auto rounded=saved;rounded.notes[0].controls=controls({{0,1,1,1},{.0001,2,0,0}});auto zero=render(rounded);for(auto x:zero)need(x.l==0&&x.r==0,"Coincident rounded control boundary is not last-point-wins");
 auto freeze=saved;freeze.notes[0].controls=controls({{0,0,1,1}});auto held=render(freeze);need(held[0].l==held[100].l&&held[100].l==float(16./128*.25),"Explicit zero source rate does not hold phase");
 auto clipped=saved;clipped.notes[0].controls=controls({{0,1,1,1},{2,2,0,0}});clipped.notes[0].length=1;auto shortAudio=render(clipped);need(shortAudio[200].l==0&&clipped.notes[0].controls->size()==2,"Resize deleted future automation or failed to stop note");clipped.notes[0].length=3;auto extended=render(clipped);need(extended[201].l!=0&&extended[400].l==0,"Extending note did not restore retained control point");
 auto tail=saved;tail.samples[0].loop=false;tail.notes[0].controls.reset();tail.notes[0].sourceOffset=7.5;auto one=render(tail);need(one[0].l==float(-16./128*.25)&&one[1].l==0,"Fractional one-shot source offset incorrect");
 auto bad=saved;bad.notes[0].sourceOffset=-1;rejected(bad);bad=saved;bad.notes[0].sourceOffset=8;rejected(bad);bad=saved;bad.notes[0].sourceOffset=std::numeric_limits<double>::infinity();rejected(bad);
 for(auto points:{controls({{.01,1,1,1}}),controls({{0,1,1,1},{0,2,1,1}}),controls({{0,1,1,1},{2,1,1,1},{1,1,1,1}}),controls({{0,-1,1,1}}),controls({{0,4097,1,1}}),controls({{0,1,-1,1}}),controls({{0,1,1,17}}),controls({{0,std::numeric_limits<double>::quiet_NaN(),1,1}}),controls({{0,1,1,1},{300000,1,1,1}})}){bad=saved;bad.notes[0].controls=points;rejected(bad);}
 auto many=std::make_shared<std::vector<NoteControl>>();for(int i=0;i<8193;i++)many->push_back({double(i),1,1,1});bad=saved;bad.notes[0].controls=many;rejected(bad);many->pop_back();auto limit=saved;limit.notes.assign(32,limit.notes[0]);for(auto&n:limit.notes)n.controls=many;limit.validate();limit.notes.push_back(limit.notes[0]);rejected(limit);
 auto nes=Song::demo();nes.notes[0].controls=controls({{0,1,1,1}});rejected(nes);nes=Song::demo();nes.notes[0].sourceOffset=.5;rejected(nes);
 // Parser rejection is bounded before allocation; truncated/bad records may
 // not be silently accepted as a flat version3 song.
 for(auto corrupt:{text+"junk",text.substr(0,text.size()-4)}){bool failed=false;try{Song::parse(corrupt);}catch(const std::exception&){failed=true;}need(failed,"Malformed automated project accepted");}
 auto count=text;auto at=count.find("0 0 120 60 15 -1 -1 0.5 3");need(at!=std::string::npos,"Own note record missing");count.replace(at,std::string("0 0 120 60 15 -1 -1 0.5 3").size(),"0 0 120 60 15 -1 -1 0.5 999999");bool failed=false;try{Song::parse(count);}catch(const std::exception&){failed=true;}need(failed,"Unbounded control count accepted");
 auto flat=saved;flat.notes[0].sourceOffset=0;flat.notes[0].controls.reset();need(flat.serialize().rfind("NGMS 3\n",0)==0&&Song::parse(flat.serialize()).serialize()==flat.serialize(),"Version3 flat sample project changed");
 std::cout<<"PASS common Song v4 automation/known fractional samples/continuous phase/stereo/block+seek/immutable undo/8 persisted musical edits+exact WAV/move/pitch/tempo/rounding/freeze/resize/one-shot/strict limits/parser/NES rejection/v3 compatibility\n";return 0;
 }catch(const std::exception&e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
