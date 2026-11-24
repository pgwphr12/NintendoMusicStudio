#include "core/IMusicBackend.hpp"
#include <Gb_Apu.h>
#include <gme.h>
#include <fstream>
#include <string>
#include <iostream>
#include <cmath>
#include <algorithm>
using namespace ngmv;
void need(bool b,const char*m){if(!b)throw std::runtime_error(m);}
std::vector<short> pcm(int frequency,int pattern,bool modulate=true){
 Gb_Apu a;Blip_Buffer b;need(!b.set_sample_rate(48000),"Sample rate");b.clock_rate(4194304);a.set_output(&b);a.reset(Gb_Apu::mode_agb,false);
 a.write_register(0,0xff26,0x80);a.write_register(0,0xff24,0x77);a.write_register(0,0xff25,0x44);
 for(int i=0;i<16;++i)a.write_register(0,0xff30+i,pattern);
 a.write_register(0,0xff1a,0x80);a.write_register(0,0xff1c,0x20);a.write_register(0,0xff1d,frequency&255);a.write_register(0,0xff1e,0x80|(frequency>>8));
 unsigned bits=1;for(int t=2048;t<419000;t+=336){bits=bits*1664525+1013904223;if(modulate)a.write_register(t,0xff1c,(bits>>30)&1?0x20:0);}
 a.end_frame(419430);b.end_frame(419430);std::vector<short> v(b.samples_avail());b.read_samples(v.data(),long(v.size()));return v;
}
int wmain(int argc,wchar_t**argv){try{
 if(argc>2){auto bytes=readMusicFile(argv[1]);Music_Emu*e=nullptr;need(!gme_open_data(bytes.data(),long(bytes.size()),&e,48000),"GBS");gme_ignore_silence(e,1);gme_set_autoload_playback_limit(e,0);gme_enable_accuracy(e,1);std::ofstream f{std::filesystem::path(argv[2])};f<<"track,energy,peak,hash,warning\n";
 std::vector<short> v(2048*2),first(1024);for(int t=0;t<gme_track_count(e);++t){need(!gme_start_track(e,t),"Track");double en=0;int peak=0;uint64_t h=1469598103934665603ull;for(int n=0;n<72;++n){need(!gme_play(e,int(v.size()),v.data()),"Render");if(n==0)std::copy_n(v.begin(),first.size(),first.begin());for(short x:v){en+=double(x)*x;peak=std::max(peak,std::abs(int(x)));h^=uint16_t(x);h*=1099511628211ull;}}auto rawWarning=gme_warning(e);std::string warning=rawWarning?rawWarning:"";
 need(!gme_start_track(e,t),"Replay start");need(!gme_play(e,int(v.size()),v.data()),"Replay render");need(std::equal(first.begin(),first.end(),v.begin()),"Subsong replay mismatch");
 gme_mute_voices(e,15);need(!gme_start_track(e,t),"Mute start");for(int n=0;n<8;++n)need(!gme_play(e,int(v.size()),v.data()),"Muted render");need(std::all_of(v.begin(),v.end(),[](short x){return x==0;}),"Muted mix not silent");gme_mute_voices(e,0);
 f<<t+1<<','<<en<<','<<peak<<','<<h<<','<<warning<<'\n';}gme_delete(e);std::cout<<"GBS98-track bounded metrics complete\n";return 0;}
 auto low=pcm(0x7fb,0xff),high=pcm(0x7ff,0xff),zero=pcm(0x7ff,0),zeroReference=pcm(0x7ff,0,false);need(low.size()==high.size()&&high.size()==zero.size(),"Sizes");double error=0,energy=0,z=0;for(size_t i=200;i<low.size();++i){double d=double(high[i])-low[i];error+=d*d;energy+=double(low[i])*low[i];double dz=double(zero[i])-zeroReference[i];z+=dz*dz;}
 std::cout<<"constant-wave NR32 PCM relative_error="<<std::sqrt(error/std::max(1.,energy))<<" zero_relative_energy="<<z/std::max(1.,energy)<<'\n';need(error/std::max(1.,energy)<.00001,"High-frequency constant wave loses actual DAC level");need(z/std::max(1.,energy)<.00001,"Zero wave invents PCM");std::cout<<"PASS own high-frequency NR32 PCM waveform/zero sample reference\n";return 0;
 }catch(const std::exception&e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
