#include "editor/GameRip.hpp"
#include "editor/Song.hpp"
#include "core/IMusicBackend.hpp"
#include "player/Player.hpp"
#include <cmath>
#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>
#include <cstring>
#include "SourceEditChecks.hpp"
#include "NoteImportChecks.hpp"
using namespace ngmv;using namespace ngmv::editor;
static void require(bool b,const char*s){if(!b)throw std::runtime_error(s);}
int wmain(int argc,wchar_t**argv){try{
 require(argc>=3,"Usage: studio_tests fixtures output [actual-GSF]");std::filesystem::path fixtures=argv[1],out=argv[2];std::filesystem::create_directories(out);
 std::atomic<bool>cancel{false};std::atomic<int>progress{0};
 checkSharedNotes(out);
 Song demo=Song::demo();auto bytes=compileNsf(demo);auto input=out/L"generated.nsf";{std::ofstream f(input,std::ios::binary);f.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());}
 checkSourceEdit(input,out);
 auto nsf=ripGameMusic(input,0,4,128,out/L"nsf",cancel,progress);require(nsf.baseNesState&&nsf.editable&&nsf.notes>5&&progress==100,"NES notes/state import");require(nsf.editable->instruments[0].duty==1,"NES duty readback");
 {std::ifstream wav(out/L"nsf/mix.wav",std::ios::binary);char header[56];wav.read(header,56);require(header[20]==3&&!std::memcmp(header+36,"fact",4)&&!std::memcmp(header+48,"data",4),"Float WAV format");std::vector<float>samples(4096*2);wav.read(reinterpret_cast<char*>(samples.data()),samples.size()*4);auto core=makeBackend(bytes,input);AudioBlock block;core->render(4096,block);for(size_t i=0;i<4096;i++)require(samples[i*2]==block.mix[i].l&&samples[i*2+1]==block.mix[i].r,"Export changed decoded PCM");}
 for(auto name:{L"channels.nsfe",L"channels.spc",L"channels.gbs",L"channels.gsf",L"channels.2sf",L"channels.usf",L"channels.bcstm",L"channels.bcwav",L"sms.vgm",L"sms.vgz"}){
  auto p=fixtures/name;if(!std::filesystem::exists(p))continue;
  checkSourceEdit(p,out);
  auto r=ripGameMusic(p,0,1,120,out/(p.filename().wstring()+L"-rip"),cancel,progress);require(!r.channels.empty()&&std::filesystem::file_size(r.directory/L"mix.wav")==56+48000*8,"Channel extraction duration");std::cout<<p.filename().u8string()<<" channels="<<r.channels.size()<<" PASS\n";
 }
 cancel=true;bool cancelled=false;try{ripGameMusic(input,0,4,120,out/L"cancelled",cancel,progress);}catch(const std::exception&){cancelled=true;}require(cancelled&&std::filesystem::exists(out/L"cancelled/INCOMPLETE.txt"),"Cancellation");cancel=false;
 // A short tagged GSF must keep rendering/consuming past metadata end when enabled.
 auto shortFile=out/L"short.gsf";{std::ifstream f(fixtures/L"channels.gsf",std::ios::binary);std::string data((std::istreambuf_iterator<char>(f)),{});auto pos=data.find("[TAG]");require(pos!=std::string::npos,"GSF fixture tags");data.resize(pos+5);data+="\nlength=0.05\nfade=0\n";std::ofstream g(shortFile,std::ios::binary);g<<data;}
 // This fixture may reference a side library, so retain its named local dependency.
 for(auto&e:std::filesystem::directory_iterator(fixtures))if(e.path().extension()==L".gsflib")std::filesystem::copy_file(e.path(),out/e.path().filename(),std::filesystem::copy_options::overwrite_existing);
 Player p;p.continueAfterLength(true);p.open(shortFile);p.play();std::this_thread::sleep_for(std::chrono::milliseconds(240));double t1=p.seconds();std::this_thread::sleep_for(std::chrono::milliseconds(150));require(p.playing()&&!p.ended()&&p.seconds()>t1+.05,"Post-end waveform clock froze");double tailEnergy=0;for(auto&w:p.waveforms())for(float x:w)tailEnergy+=x*x;require(tailEnergy>.001,"Metadata fade still forced silence");p.pause();double paused=p.seconds();std::this_thread::sleep_for(std::chrono::milliseconds(80));require(p.seconds()==paused,"Pause did not hold clock");p.stop();
 if(argc>3){
  auto path=std::filesystem::path(argv[3]);auto data=readMusicFile(path);_putenv_s("NGMV_GBA_HARDWARE_ONLY","1");auto baseline=makeBackend(data,path);_putenv_s("NGMV_GBA_HARDWARE_ONLY","");auto split=makeBackend(data,path);
  require(baseline->channels().size()==6&&split->channels().size()>6,"Actual GSF mixer not recognized");AudioBlock a,b;double error=0,energy=0,residual=0;std::vector<double>voices(split->channels().size());
  for(int k=0;k<563;k++){baseline->render(512,a);split->render(512,b);for(size_t i=0;i<512;i++){error=std::max(error,double(std::max(std::abs(a.mix[i].l-b.mix[i].l),std::abs(a.mix[i].r-b.mix[i].r))));Stereo sum{};for(size_t c=0;c<b.voices.size();c++){sum.l+=b.voices[c][i].l;sum.r+=b.voices[c][i].r;voices[c]+=b.voices[c][i].l*b.voices[c][i].l+b.voices[c][i].r*b.voices[c][i].r;}require(std::abs(sum.l-b.mix[i].l)<.0001&&std::abs(sum.r-b.mix[i].r)<.0001,"GSF stem recombination");energy+=b.mix[i].l*b.mix[i].l+b.mix[i].r*b.mix[i].r;residual+=b.voices.back()[i].l*b.voices.back()[i].l+b.voices.back()[i].r*b.voices.back()[i].r;}}
  require(error<1e-7,"Original GSF mix changed");int active=0;for(size_t i=4;i+1<voices.size();i++)if(voices[i]>.01)active++;require(active>=4&&residual/energy<.01,"GSF attribution incomplete");std::cout<<"GSF actual PCM voices="<<active<<" original_mix_max_error="<<error<<" residual_energy_ratio="<<residual/energy<<" PASS\n";
 }
 std::cout<<"PASS NSF state/notes, supported-format stems, cancel, post-end clock/pause, optional actual-GSF separation\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
