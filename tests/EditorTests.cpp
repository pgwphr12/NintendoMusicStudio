#include "editor/Song.hpp"
#include <gme.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <cmath>
#include <stdexcept>
using namespace ngmv::editor;
void require(bool b,const char*s){if(!b)throw std::runtime_error(s);}
double render(const std::vector<uint8_t>&data,double seconds,int mask=0,double sampleFrom=0){
 Music_Emu*e=nullptr;auto error=gme_open_data(data.data(),long(data.size()),&e,48000);require(!error,error?error:"");
 gme_ignore_silence(e,1);gme_set_autoload_playback_limit(e,0);require(!gme_start_track(e,0),"Start failed");gme_mute_voices(e,mask);
 std::vector<short>b(4096);double energy=0;size_t count=0;
 for(int i=0;i<int(seconds*48000*2);i+=4096){int n=std::min(4096,int(seconds*48000*2)-i);if(n%2)n--;require(!gme_play(e,n,b.data()),"Render failed");for(int j=0;j<n;j++)if(i+j>=sampleFrom*48000*2){double x=b[j]/32768.;energy+=x*x;count++;}require(!gme_warning(e),"Emulator warning");}
 gme_delete(e);return std::sqrt(energy/std::max(size_t(1),count));
}
int main(int argc,char**argv){try{
 auto dir=std::filesystem::path(argc>1?argv[1]:"editor-test-output");std::filesystem::create_directories(dir);
 Song s=Song::demo();require(Song::parse(s.serialize()).serialize()==s.serialize(),"Round trip");
 auto before=s.serialize();try{Song::parse(before+"garbage");throw 1;}catch(const std::exception&){}
 Song conflict;conflict.put({0,0,4,60,15});conflict.put({0,2,2,64,12});require(conflict.notes.size()==1&&conflict.notes[0].pitch==64,"Monophonic replacement");
 bool bad=false;try{conflict.put({2,255,8,60,15});}catch(const std::exception&){bad=true;}require(bad&&conflict.notes.size()==1,"Invalid edit atomicity");
 for(bool pal:{false,true}){
  s.pal=pal;auto data=compileNsf(s);require(data[122]==int(pal)&&data[123]==0,"Region/expansion");
  std::ofstream out(dir/(pal?"demo-pal.nsf":"demo-ntsc.nsf"),std::ios::binary);out.write(reinterpret_cast<char*>(data.data()),data.size());
  require(render(data,s.seconds()+1)>.01,"Silent export");for(int i=0;i<4;i++)require(render(data,3,31^(1<<i))>.001,"Missing voice");require(render(data,3,15)<1e-8,"DPCM unexpectedly active");
  require(render(data,s.seconds()+.5,0,s.seconds()+.25)<.0001,"Export failed to silence at end");
  auto muted=s;for(auto&i:muted.instruments)i.mute=true;require(render(compileNsf(muted),2)<1e-8,"Mute export");
 }
 Song longSong;longSong.bars=16;longSong.bpm=30;for(int i=0;i<256;i++)for(int c=0;c<4;c++)longSong.notes.push_back({c,i,1,c==3?i%16:48+i%24,15});require(render(compileNsf(longSong),12)>.001,"Bank crossing");
 std::ofstream(dir/"First light.ngms")<<s.serialize();
 std::cout<<"PASS project roundtrip, malformed input, monophony, atomic invalid edits, PAL/NTSC, four voices, mute, no-DPCM, bank crossing\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}catch(...){return 2;}}
