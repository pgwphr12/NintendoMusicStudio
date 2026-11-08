#include "core/IMusicBackend.hpp"
#include <gme.h>
#include <Gb_Apu.h>
#include <cmath>
#include <iostream>
#include <algorithm>
extern "C" int ngmv_gbs_agb_mode(Music_Emu*);
using namespace ngmv;
void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
std::vector<short> wave(Gb_Apu::mode_t mode){Gb_Apu a;Blip_Buffer b;need(!b.set_sample_rate(48000),"Blip rate");b.clock_rate(4194304);a.set_output(&b);a.reset(mode,false);a.write_register(0,0xff26,0x80);a.write_register(0,0xff24,0x77);a.write_register(0,0xff25,0x44);for(int i=0;i<16;i++)a.write_register(0,0xff30+i,i*17);a.write_register(0,0xff1a,0x80);a.write_register(0,0xff1c,0x20);a.write_register(0,0xff1d,0x80);a.write_register(0,0xff1e,0x87);need(a.compatibility_model()==mode&&!a.native_wave_features(),"Wrong hardware or native wave extension enabled");a.write_register(300000,0xff1a,0);a.end_frame(419430);b.end_frame(419430);std::vector<short> out(b.samples_avail());b.read_samples(out.data(),long(out.size()));return out;}
int wmain(int argc,wchar_t**argv){try{
 auto cgb=wave(Gb_Apu::mode_cgb),agb=wave(Gb_Apu::mode_agb);need(cgb.size()==agb.size()&&cgb!=agb,"AGB selector did not change actual synthesis");
 for(int f=1;f<argc;f++){
 auto r=readMusicFile(argv[f]);auto original=r;Music_Emu* core=nullptr;need(!gme_open_data(r.data(),long(r.size()),&core,48000),"GBS load");need(ngmv_gbs_agb_mode(core)==1,"GBS not using AGB hardware");need(!gme_start_track(core,0)&&ngmv_gbs_agb_mode(core)==1,"Restart lost AGB model");gme_delete(core);
 auto b=makeBackend(r,argv[f]);need(b->channels().size()==4,"GB must retain four hardware lanes, no fake GBA FIFO");AudioBlock out;std::vector<Stereo> first;double energy=0;std::vector<double> voices(4);
 for(int n=0;n<188;n++){b->render(512,out);need(out.voices.size()==4,"Lost four lanes");first.insert(first.end(),out.mix.begin(),out.mix.end());for(size_t i=0;i<512;i++){need(std::isfinite(out.mix[i].l)&&std::isfinite(out.mix[i].r),"Nonfinite PCM");energy+=out.mix[i].l*out.mix[i].l+out.mix[i].r*out.mix[i].r;for(int c=0;c<4;c++)voices[c]+=out.voices[c][i].l*out.voices[c][i].l+out.voices[c][i].r*out.voices[c][i].r;}}
 need(energy>.001,"Silent GBS");b->selectTrack(0);b->render(512,out);for(int i=0;i<512;i++)need(first[i].l==out.mix[i].l&&first[i].r==out.mix[i].r,"GBS replay mismatch");b->seek(500);b->render(512,out);double seekError=0;for(int i=0;i<512;i++)seekError=std::max({seekError,double(std::abs(out.mix[i].l-first[24000+i].l)),double(std::abs(out.mix[i].r-first[24000+i].r))});need(seekError<.001,"GBS seek mismatch");b->muteMask(15);for(int n=0;n<100;n++)b->render(512,out);double muted=0;for(auto v:out.mix)muted+=v.l*v.l+v.r*v.r;need(muted<.00001,"GBS mute failed");if(b->tracks().size()>1){b->muteMask(0);b->selectTrack(1);b->render(512,out);}need(readMusicFile(argv[f])==original,"GBS source changed");
 std::cout<<"PASS case="<<f<<" GBS_CPU+AGB_APU GBwave16bytes/nativeFIFOoff lanes4 energy="<<energy<<" seek_error="<<seekError<<" replay/seek/mute/subsong/read-only\n";
 }
 std::cout<<"PASS own CGB-vs-AGB synthesis distinction, hardware selection and disabled native GBA wave extension\n";return 0;
}catch(const std::exception&e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}