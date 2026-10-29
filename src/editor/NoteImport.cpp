#include "NoteImport.hpp"
#include "SampleSong.hpp"
#include "SourceEdit.hpp"
#include "core/IMusicBackend.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <windows.h>
namespace ngmv::editor {
namespace {
void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
int pitch(const std::vector<int>&s,int c){
 if(!(s[20]&(1<<c))||s[21+c]<=0||(c==2?s[36]<=0:s[26+c]<=0))return -1;
 if(c==3)return s[14]&15;
 int p=s[31+c];if(p<(c==2?2:8))return -1;
 double hz=(s[38]?1662607.:1789773.)/((c==2?32.:16.)*(p+1));
 int n=int(std::lround(69+12*std::log2(hz/440.)));return n>=24&&n<=95?n:-1;
}
}
Song importNesNotes(const std::filesystem::path&path,int track,double seconds,int bpm,std::atomic<bool>&cancel,std::atomic<int>&progress){
 need(std::isfinite(seconds)&&seconds>=.1&&seconds<=600&&bpm>=30&&bpm<=300,"Import duration 0.1–600 seconds; BPM 30–300");
 need(seconds<=512*240./bpm,"Selected duration exceeds 512 bars at this tempo");
 auto data=readMusicFile(path);auto format=identify(data);need(format==Format::Nsf||format==Format::Nsfe,"This format does not yet support note/instrument import into the shared piano roll");
 auto core=makeBackend(data,path);need(track>=0&&track<int(core->tracks().size()),"Invalid subtrack");if(track)core->selectTrack(track);need(core->prepareNesInspection(),"Fine inspection mode unavailable");core->continuousEnd(true);
 Song song;song.bpm=bpm;song.resolution=120;song.bars=std::clamp(int(std::ceil(seconds*bpm/240.)),1,512);song.title=path.stem().u8string();if(song.title.size()>120)song.title="Imported NES song";for(auto&i:song.instruments)i.volume=15;
 std::array<Note,4>active;std::array<bool,4>on{};std::array<int,4>counter{};std::vector<int>state;AudioBlock block;
 auto close=[&](int c,int at){if(on[c]&&at>active[c].step){auto n=active[c];n.length=at-n.step;song.notes.push_back(n);}on[c]=false;need(song.notes.size()<=32768,"Too many imported notes; choose a shorter range");};
 int total=int(std::llround(seconds*48000));progress=0;
 for(int at=0;at<total;){
  need(!cancel.load(),"Note import cancelled");int n=std::min(240,total-at);core->render(n,block);need(core->nesState(state)&&state.size()>=40,"NES APU state unavailable");need(state[39]==0,"Expansion-chip notes are not supported yet; import was not substituted with base NES channels");need(state[25]==0&&state[30]==0,"DPCM sample notes are not supported yet; import was not substituted with incomplete notes");
  song.pal=state[38]!=0;int tick=std::clamp(int(std::llround(double(at)*bpm*song.resolution/(48000*60.))),0,song.ticks());
  for(int c=0;c<4;c++){
   int p=pitch(state,c),d=c<2?(state[c*4]>>6)&3:c==3?((state[14]>>7)&1):0;bool attack=counter[c]>0&&state[21+c]>counter[c]+1;
   if(on[c]&&(p!=active[c].pitch||d!=active[c].duty||attack))close(c,tick);
   if(p>=0&&!on[c]){active[c]={c,tick,1,p,std::clamp(state[26+c],1,15),d};on[c]=true;}
   counter[c]=state[21+c];
  }
  at+=n;progress=at*100/total;
 }
 int end=std::min(song.ticks(),int(std::llround(seconds*bpm*song.resolution/60.)));for(int c=0;c<4;c++)close(c,end);song.validate();return song;
}
void exportSongWav(const Song&song,const std::filesystem::path&output,std::atomic<bool>&cancel,std::atomic<int>&progress){
 if(song.sampled){exportSampleSongWav(song,output,cancel,progress);return;}
 need(!cancel.load(),"WAV export cancelled");auto data=compileNsf(song);wchar_t path[MAX_PATH];need(GetTempPathW(MAX_PATH,path)!=0,"Temporary directory unavailable");
 auto own=std::filesystem::path(path)/(L"NGMS-render-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64()));need(std::filesystem::create_directory(own),"Could not create render folder");auto music=own/L"edited.nsf";
 try{std::ofstream f(music,std::ios::binary);f.write(reinterpret_cast<const char*>(data.data()),data.size());f.close();need(bool(f),"Could not prepare edited music");SourceEdit e;e.source=music;e.end=song.seconds();exportSourceWav(e,output,cancel,progress);std::filesystem::remove(music);std::filesystem::remove(own);}
 catch(...){std::error_code ec;std::filesystem::remove(music,ec);std::filesystem::remove(own,ec);throw;}
}
}
