#include "audio/StemExport.hpp"
#include "formats/StreamBackend.hpp"
extern "C" {
#include <libvgmstream.h>
}
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace ngmv;
struct Fixture : IMusicBackend {
 std::vector<TrackInfo> t{{"fixture","","","","",100}};
 std::vector<ChannelInfo> c{{"Silent"},{"Later"},{"Right"},{"InitNoise"}};uint64_t at=0;
 const std::vector<TrackInfo>& tracks()const override{return t;}
 const std::vector<ChannelInfo>& channels()const override{return c;}
 void selectTrack(int n)override{if(n!=0)throw std::runtime_error("track");at=0;}
 void seek(int)override{throw std::runtime_error("unexpected seek");}
 void muteMask(uint32_t)override{}
 bool channelUsage(uint64_t& mask)const override{mask=(1u<<1)|(1u<<2);return true;}
 void render(size_t n,AudioBlock& b)override{b.mix.assign(n,{});b.voices.assign(4,std::vector<Stereo>(n));for(size_t i=0;i<n;i++,at++){if(at>=4100)b.voices[1][i].l=.25f;b.voices[2][i].r=-.5f;if(at==0)b.voices[3][i].l=.25f;b.mix[i]={.125f,-.25f};}}
};
void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
int main(int argc,char**argv){try{
 if(argc==3&&std::string(argv[1])=="--probe-stream-loop"){auto src=std::filesystem::u8path(argv[2]);auto file=std::unique_ptr<libstreamfile_t,decltype(&libstreamfile_close)>(libstreamfile_open_from_stdio(src.u8string().c_str()),libstreamfile_close);check(bool(file),"Cannot inspect stream");libvgmstream_config_t cfg{};cfg.ignore_fade=true;cfg.disable_config_override=true;auto raw=std::unique_ptr<libvgmstream_t,decltype(&libvgmstream_free)>(libvgmstream_create(file.get(),0,&cfg),libvgmstream_free);check(bool(raw)&&raw->format->loop_flag&&raw->format->loop_end>raw->format->loop_start,"Stream has no recorded loop");auto &info=*raw->format;uint64_t expected=(uint64_t(info.stream_samples+info.loop_end-info.loop_start)*InternalRate+uint64_t(info.sample_rate)-1)/uint64_t(info.sample_rate);auto a=makeStreamBackend(src,"fixture",true,true);auto b=makeLoopingStreamBackend(src,"fixture",true,true,1);uint64_t one=0,two=0;check(a->decodedEndFrame(one)&&b->decodedEndFrame(two)&&two>one,"Stream loop did not extend duration");check(two==expected,"Stream repeat length differs from recorded loop points");b->selectTrack(0);uint64_t reset=0;check(b->decodedEndFrame(reset)&&reset==two,"Export reset changed the looped duration");AudioBlock x,y;double maxDiff=0;for(int j=0;j<16;++j){a->render(4096,x);b->render(4096,y);check(x.mix.size()==y.mix.size(),"Stream mismatch");for(size_t i=0;i<x.mix.size();++i)maxDiff=std::max(maxDiff,double(std::max(std::abs(x.mix[i].l-y.mix[i].l),std::abs(x.mix[i].r-y.mix[i].r))));}check(maxDiff<1e-6,"Loop configuration altered stream intro");std::cout<<"PASS stream recorded-loop extension: first_pass_frames="<<one<<" looped_frames="<<two<<" intro_max_diff="<<maxDiff<<"\n";return 0;}
 if(argc==3&&std::string(argv[1])=="--probe-vgm-boundary"){auto src=std::filesystem::u8path(argv[2]);auto data=readMusicFile(src);auto le=[&](size_t p){return uint32_t(data[p])|uint32_t(data[p+1])<<8|uint32_t(data[p+2])<<16|uint32_t(data[p+3])<<24;};uint64_t end=(uint64_t(le(0x18))*InternalRate+44099)/44100,frames=0;auto expanded=expandVgmLoops(data,1,frames);auto a=makeBackend(data,src),b=makeBackend(expanded,src);AudioBlock x,y;uint64_t done=0;double maxDiff=0;while(done<end){auto n=size_t(std::min<uint64_t>(4096,end-done));a->render(n,x);b->render(n,y);check(x.mix.size()==n&&y.mix.size()==n,"Boundary render size");for(size_t i=0;i<n;++i)maxDiff=std::max(maxDiff,double(std::max(std::abs(x.mix[i].l-y.mix[i].l),std::abs(x.mix[i].r-y.mix[i].r))));done+=n;}check(maxDiff<1e-6,"Loop expansion altered original pass");double energy=0;for(unsigned j=0;j<24;++j){b->render(4096,y);for(auto s:y.mix)energy+=std::abs(s.l)+std::abs(s.r);}check(energy>1,"Expanded VGM became silent after loop boundary");std::cout<<"PASS first pass identical; audio continues after recorded loop boundary: max_diff="<<maxDiff<<" energy="<<energy<<"\n";return 0;}
 if(argc==3&&std::string(argv[1])=="--probe-vgm-loop"){auto src=std::filesystem::u8path(argv[2]);auto data=readMusicFile(src);uint64_t frames=0;auto expanded=expandVgmLoops(data,1,frames);auto core=makeBackend(expanded,src);AudioBlock block;core->render(4096,block);check(!block.voices.empty(),"Expanded file lost channels");std::cout<<"PASS VGM loop expansion: frames="<<frames<<" channels="<<block.voices.size()<<"\n";return 0;}
 if(argc==3){auto path=std::filesystem::u8path(argv[2]);std::filesystem::create_directories(path);check(std::filesystem::is_empty(path),"Directory must be empty");auto src=std::filesystem::u8path(argv[1]);auto b=makeBackend(readMusicFile(src),src);std::atomic<bool> cancel{false};auto r=exportStems(*b,path,{0,1,InternalRate*3},cancel);check(r.written>0,"No non-silent channels in real file excerpt");std::cout<<"PASS actual decoder: written="<<r.written<<" silent="<<r.silent<<"\n";return 0;}
 if(argc!=2)throw std::runtime_error("Expected an empty test directory");auto path=std::filesystem::u8path(argv[1]);std::filesystem::create_directories(path);check(std::filesystem::is_empty(path),"Directory must be empty");
 Fixture f;std::atomic<bool> cancelled{false};unsigned progress=0;auto r=exportStems(f,path,{0,1,4800},cancelled,[&](unsigned n){progress=n;});
 check(r.written==3&&r.silent==2&&progress==100,"Used/silent channels or progress mismatch");
 check(!std::filesystem::exists(path/L"1_Silent.wav"),"Silent channel retained");
 check(!std::filesystem::exists(path/L"4_InitNoise.wav"),"Initialization-only channel retained despite whole-track usage");
 for(auto name:{L"2_Later.wav",L"3_Right.wav"}){auto p=path/name;check(std::filesystem::file_size(p)==44+4800*4,"Incorrect WAV size");std::ifstream in(p,std::ios::binary);char h[44];in.read(h,44);check(std::string(h,4)=="RIFF"&&std::string(h+8,4)=="WAVE"&&uint8_t(h[22])==2,"Incorrect WAV header");}
 std::ifstream in(path/L"2_Later.wav",std::ios::binary);in.seekg(44+4099*4);char b[8];in.read(b,8);check(b[0]==0&&b[1]==0&&uint8_t(b[4])==0&&uint8_t(b[5])==32,"Late audio missing or initial silence removed");
 std::ifstream master(path/L"master.wav",std::ios::binary);master.seekg(44);char mix[4];master.read(mix,4);check(uint8_t(mix[0])==0&&uint8_t(mix[1])==16&&uint8_t(mix[2])==0&&uint8_t(mix[3])==224,"Master must use original decoder mix, not a sum of stems");
 auto mixOnly=path/L"mixed";std::filesystem::create_directory(mixOnly);auto mixed=exportStems(f,mixOnly,{0,1,4800,true},cancelled);check(mixed.written==1&&std::filesystem::exists(mixOnly/L"master.wav"),"Mix-only output missing");
 struct WideFixture : Fixture {WideFixture(){c.assign(64,{"Voice"});}bool channelUsage(uint64_t& mask)const override{mask=~uint64_t(0);return true;}void render(size_t n,AudioBlock& b)override{b.mix.assign(n,{.25f,-.25f});b.voices.assign(64,std::vector<Stereo>(n,{.001f,0}));}} wide;
 auto many=path/L"64voices";std::filesystem::create_directory(many);auto wr=exportStems(wide,many,{0,1,8},cancelled);check(wr.written==65&&wr.silent==0&&std::filesystem::exists(many/L"master.wav"),"64-channel master boundary failed");
 auto rejected=path/L"cancelled";std::filesystem::create_directory(rejected);cancelled=true;bool stopped=false;try{exportStems(f,rejected,{0,1,4800},cancelled);}catch(const std::exception&){stopped=true;}check(stopped,"Cancellation was ignored");
 std::vector<uint8_t> vgm(0x100,0);auto put=[&](size_t p,uint32_t n){for(int i=0;i<4;++i)vgm[p+i]=uint8_t(n>>(i*8));};vgm[0]='V';vgm[1]='g';vgm[2]='m';vgm[3]=' ';put(8,0x150);put(0x0c,3579545);put(0x34,0xcc);vgm.insert(vgm.end(),{0x50,0x80,0x50,0x01,0x50,0x90,0x62});size_t loop=vgm.size();vgm.insert(vgm.end(),{0x50,0x9f,0x62,0x50,0x90,0x62,0x66});put(4,uint32_t(vgm.size()-4));put(0x1c,uint32_t(loop-0x1c));put(0x18,2205);put(0x20,1470);
 uint64_t frames=0;auto expanded=expandVgmLoops(vgm,2,frames);check(frames==5600&&expanded.size()==vgm.size()+12,"VGM loop duration/size incorrect");check(expanded[0x1c]==0&&expanded[0x20]==0,"Expanded VGM still loops");for(size_t i=0;i<6;++i)check(expanded[loop+i]==expanded[loop+6+i]&&expanded[loop+i]==expanded[loop+12+i],"Loop events were not repeated exactly");
 auto backend=makeBackend(expanded);AudioBlock audio;backend->render(5600,audio);check(audio.mix.size()==5600,"Expanded VGM did not render");
 auto energy=[&](size_t a,size_t z){double sum=0;for(size_t i=a;i<z;++i)sum+=std::abs(audio.mix[i].l)+std::abs(audio.mix[i].r);return sum;};
 double intro=energy(300,700),off1=energy(1000,1400),on1=energy(1800,2200),off2=energy(2600,3000),on2=energy(3400,3800),off3=energy(4200,4600),on3=energy(5000,5400);
 check(intro>1&&on1>1&&on2>1&&on3>1&&off1<intro*.1&&off2<intro*.1&&off3<intro*.1,"Recorded loop did not preserve the expected intro and loop on/off sequence");
 vgm[0x1c]=0;bool noLoop=false;try{expandVgmLoops(vgm,1,frames);}catch(const std::exception&){noLoop=true;}check(noLoop,"Missing VGM loop accepted");
 std::cout<<"PASS late/silent channels, stereo WAV, cancellation, VGM recorded-loop timing and rendering\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
