#include "GameRip.hpp"
#include "core/IMusicBackend.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <memory>
#include <sstream>
#include <stdexcept>
namespace ngmv::editor {
namespace {
void word(std::ostream&o,uint32_t n,int count){for(int i=0;i<count;i++)o.put(char(n>>(8*i)));}
struct Wav {
 std::filesystem::path path;std::ofstream f;uint32_t frames=0;
 explicit Wav(const std::filesystem::path&p):path(p),f(p,std::ios::binary){if(!f)throw std::runtime_error("Could not create channel file");f.write("RIFF",4);word(f,0,4);f.write("WAVEfmt ",8);word(f,16,4);word(f,3,2);word(f,2,2);word(f,48000,4);word(f,384000,4);word(f,8,2);word(f,32,2);f.write("fact",4);word(f,4,4);word(f,0,4);f.write("data",4);word(f,0,4);}
 void append(const std::vector<Stereo>&data){std::vector<float>b;b.reserve(data.size()*2);for(auto s:data){if(!std::isfinite(s.l)||!std::isfinite(s.r))throw std::runtime_error("Decoder returned non-finite PCM");b.push_back(s.l);b.push_back(s.r);}f.write(reinterpret_cast<const char*>(b.data()),std::streamsize(b.size()*4));frames+=uint32_t(data.size());if(!f)throw std::runtime_error("Channel export failed; check free space");}
 void finish(){f.seekp(4);word(f,48+frames*8,4);f.seekp(44);word(f,frames,4);f.seekp(52);word(f,frames*8,4);f.close();if(!f)throw std::runtime_error("Could not finish channel file");}
};
std::string json(const std::string&s){std::ostringstream o;o<<'"';for(unsigned char c:s){if(c=='"'||c=='\\')o<<'\\'<<char(c);else if(c<32)o<<"\\u"<<std::hex<<std::setw(4)<<std::setfill('0')<<int(c)<<std::dec;else o<<char(c);}return o.str()+'"';}
struct Frame {double time;std::array<int,40>state;};
int notePitch(const std::array<int,40>&s,int ch){
 if(!(s[20]&(1<<ch))||s[21+ch]<=0)return -1;
 if(ch==2){if(s[36]<=0)return -1;}else if(s[26+ch]<=0)return -1;
 if(ch==3)return s[14]&15;
 int p=s[31+ch];if(p<(ch<2?8:2))return -1;
 double hz=(s[38]?1662607.:1789773.)/((ch==2?32.:16.)*(p+1));int pitch=int(std::lround(69+12*std::log2(hz/440.)));
 return pitch>=24&&pitch<=95?pitch:-1;
}
Song toSong(const std::vector<Frame>&frames,int bpm,double seconds){
 Song song;song.bpm=bpm;song.bars=std::clamp(int(std::ceil(seconds*bpm/240.)),1,512);song.title="Imported NES base channels";song.pal=frames.front().state[38]!=0;
 for(int ch=0;ch<4;ch++){
  int last=-1,begin=0,volume=15;std::array<int,4>duties{};
  auto flush=[&](int end){if(last>=0&&end>begin)song.notes.push_back({ch,begin,end-begin,last,std::clamp(volume,1,15)});};
  for(int step=0;step<song.bars*16;step++){
   double time=(step+.5)*15./bpm;int pitch=-1,v=15;
   if(time<seconds){auto it=std::upper_bound(frames.begin(),frames.end(),time,[](double t,const Frame&f){return t<f.time;});if(it!=frames.begin())--it;pitch=notePitch(it->state,ch);v=it->state[26+ch];if(ch<2&&pitch>=0)duties[(it->state[ch*4]>>6)&3]++;}
   if(pitch!=last){flush(step);last=pitch;begin=step;volume=v;}
  }
  flush(song.bars*16);song.instruments[ch].duty=int(std::max_element(duties.begin(),duties.end())-duties.begin());song.instruments[ch].volume=15;
 }
 song.validate();return song;
}
}
RipResult ripGameMusic(const std::filesystem::path&file,int track,int seconds,int bpm,const std::filesystem::path&output,std::atomic<bool>&cancel,std::atomic<int>&progress){
 if(seconds<1||seconds>300||bpm<30||bpm>300)throw std::runtime_error("Analysis length/tempo out of range");
 if(std::filesystem::exists(output)&&!std::filesystem::is_empty(output))throw std::runtime_error("Choose a new empty output folder");
 auto bytes=readMusicFile(file);auto backend=makeBackend(bytes,file);if(track<0||track>=int(backend->tracks().size()))throw std::runtime_error("Invalid subtrack");
 backend->selectTrack(track);RipResult r;r.directory=output;r.channels=backend->channels();
 if(r.channels.empty()||r.channels.size()>64)throw std::runtime_error("Channel count exceeds analysis limit");
 double length=seconds;int known=backend->tracks()[track].lengthMs;if(known>0)length=std::min(length,known/1000.);
 auto parent=output.parent_path();auto space=std::filesystem::space(parent.empty()?std::filesystem::current_path():parent);
 if(double(space.available)<length*384000*(r.channels.size()+1)*1.1)throw std::runtime_error("Not enough disk space for all channel WAV files");
 std::filesystem::create_directories(output);
 std::vector<std::unique_ptr<Wav>>files;files.push_back(std::make_unique<Wav>(output/L"mix.wav"));
 for(size_t i=0;i<r.channels.size();i++){std::ostringstream n;n<<"channel-"<<std::setw(2)<<std::setfill('0')<<i+1<<".wav";files.push_back(std::make_unique<Wav>(output/n.str()));}
 std::ofstream csv(output/L"nes-state.csv");csv<<"seconds,channel,active,midi_pitch,volume,duty,period\n";std::vector<Frame>frames;std::vector<double>energy(r.channels.size(),0);AudioBlock block;std::vector<int>state;
 try{
  const int total=int(std::lround(length*48000));
  for(int at=0;at<total;){
   if(cancel.load())throw std::runtime_error("Analysis cancelled");int n=std::min(960,total-at);backend->render(n,block);
   if(block.voices.size()!=r.channels.size()||block.mix.size()!=size_t(n))throw std::runtime_error("Decoder returned inconsistent channel data");
   files[0]->append(block.mix);
   for(size_t ch=0;ch<r.channels.size();ch++){if(block.voices[ch].size()!=size_t(n))throw std::runtime_error("Decoder returned short channel data");files[ch+1]->append(block.voices[ch]);for(auto s:block.voices[ch])energy[ch]+=(double(s.l)*s.l+double(s.r)*s.r)/2;}
   if(backend->nesState(state)){
    Frame f{};f.time=double(at+n)/48000;std::copy_n(state.begin(),40,f.state.begin());frames.push_back(f);
    for(int ch=0;ch<4;ch++){int p=notePitch(f.state,ch);csv<<std::fixed<<std::setprecision(5)<<f.time<<','<<ch+1<<','<<(p>=0)<<','<<p<<','<<f.state[26+ch]<<','<<(ch<2?(f.state[ch*4]>>6)&3:0)<<','<<f.state[31+ch]<<'\n';}
   }
   at+=n;progress.store(at*100/total);
  }
  for(auto&f:files)f->finish();csv.close();r.seconds=length;r.samples=int(std::lround(length*48000));r.baseNesState=!frames.empty();
  if(!frames.empty()){r.expansion=frames.front().state[39]!=0;r.editable=toSong(frames,bpm,length);r.notes=int(r.editable->notes.size());std::ofstream out(output/L"NES-base.ngms",std::ios::binary);out<<r.editable->serialize();if(!out)throw std::runtime_error("Could not save note project");}
  else{std::filesystem::remove(output/L"nes-state.csv");}
  std::ofstream meta(output/L"analysis.json");meta<<"{\n\"track\":"<<track+1<<",\"seconds\":"<<length<<",\"requested_seconds\":"<<seconds<<",\"sample_rate\":48000,\"base_NES_state_available\":"<<(r.baseNesState?"true":"false")<<",\"expansion_not_in_note_project\":"<<(r.expansion?"true":"false")<<",\"original_tracker_project_restored\":false,\"note_grid\":\"1/16; sampled base NES channels only; DPCM omitted\",\"decoded_channels_or_buses\":[\n";
  for(size_t i=0;i<r.channels.size();i++){if(i)meta<<",\n";meta<<"{\"index\":"<<i+1<<",\"name\":"<<json(r.channels[i].name)<<",\"rms\":"<<std::sqrt(energy[i]/r.samples)<<'}';}meta<<"\n]}\n";if(!meta)throw std::runtime_error("Could not save analysis manifest");return r;
 }catch(...){files.clear();csv.close();std::ofstream(output/L"INCOMPLETE.txt")<<"Cancelled or failed extraction. These files are incomplete; original input was not modified.\n";throw;}
}
}
