#include "formats/GbaVoiceCapture.hpp"
#include "core/IMusicBackend.hpp"
#include <algorithm>
#include <array>
#include <iostream>
#include <set>
#include <cmath>
using namespace ngmv;
static void require(bool b,const char*s){if(!b)throw std::runtime_error(s);}
struct OwnMemory {
    std::array<uint8_t,0x8000>ram{};std::array<uint8_t,4096>rom{};
    static constexpr uint32_t Info=0x03000100,Channel=Info+0x50,Wave=0x08000100;
    const uint8_t*read(uint32_t p,size_t n)const{if(p>=0x03000000&&p-0x03000000<=ram.size()&&n<=ram.size()-(p-0x03000000))return ram.data()+p-0x03000000;if(p>=0x08000000&&p-0x08000000<=rom.size()&&n<=rom.size()-(p-0x08000000))return rom.data()+p-0x08000000;return nullptr;}
    void put(uint32_t p,uint32_t v,int n=4){auto b=const_cast<uint8_t*>(read(p,n));require(b!=nullptr,"Own fixture write bounds");for(int i=0;i<n;i++)b[i]=uint8_t(v>>(i*8));}
    OwnMemory(){put(Info,0x68736d54);put(Channel+8,60,1);put(Channel+17,60,1);put(Channel+18,100,1);put(Channel+16,24,1);put(Channel+2,80,1);put(Channel+3,90,1);put(Channel+4,0x8040ff20);put(Channel+32,123456);put(Channel+36,Wave);put(Channel+44,0x03002000);put(Wave,0x40000000);put(Wave+4,16000*1024);put(Wave+8,8);put(Wave+12,64);for(int i=0;i<64;i++)rom[0x110+i]=uint8_t(i-32);}
};
static void unit(){
 OwnMemory m;auto read=[&](uint32_t p,size_t n){return m.read(p,n);};auto capture=std::make_unique<GbaVoiceCapture>();auto beforeRAM=m.ram;auto beforeROM=m.rom;
 auto write=[&](int offset,uint32_t value,int width,uint64_t time){capture->onWrite(read,m.Info,12,m.Channel+offset,value,width,time);};
 write(0,0x80,1,10);m.put(m.Channel,0x80,1);write(0,3,1,20);m.put(m.Channel,3,1);
 write(32,456789,4,30);m.put(m.Channel+32,456789);write(2,0x3c64,2,40);m.put(m.Channel+2,0x3c64,2);
 write(0,0x43,1,50);m.put(m.Channel,0x43,1);write(0,0x43,1,51);write(0,0x10,1,60);m.put(m.Channel,0x10,1);
 write(0,0x5a500880,4,70);write(0,0x5a500880,4,80);
 std::vector<GbaVoiceEvent>events;std::string error;require(capture->drain(events,error)&&error.empty()&&events.size()==7,"Start/control/release/end/retrigger event sequence");
 require(events[0].kind==GbaVoiceEventKind::NoteOn&&events[0].midiKey==60&&events[0].key==60&&events[0].velocity==100&&events[0].gateTicks==24&&events[0].sourceTrack==0x03002000,"Original note metadata");
 require(events[0].sample.readable&&events[0].sample.pcm8&&events[0].sample.looped&&events[0].sample.loopStart==8&&events[0].sample.length==64&&events[0].sample.rawFrequency==16384000,"Sample/header metadata");
 require(events[1].kind==GbaVoiceEventKind::Pitch&&events[1].frequency==456789&&events[2].kind==GbaVoiceEventKind::Volume&&events[2].rightVolume==100&&events[2].leftVolume==60,"Prospective multi-byte controls");
 require(events[3].kind==GbaVoiceEventKind::GateOff&&events[4].kind==GbaVoiceEventKind::VoiceEnd&&events[5].generation==2&&events[6].generation==3&&events[5].type==8,"Distinct gate/end and identical-key retriggers");
 require(m.rom==beforeROM&&m.ram[0x150]==0x10&&m.ram[0x151]==0,"Observer changed emulated memory");
 events={{}};require(capture->drain(events,error)&&events.empty(),"Drained queue repeated events");
 GbaVoiceEvent e;e.slot=99;m.put(m.Channel+8,128,1);require(!GbaVoiceCapture::decode(read,m.Channel,0,e)&&e.slot==99,"Invalid key did not preserve output");m.put(m.Channel+8,60,1);
 m.put(m.Wave+8,64);require(GbaVoiceCapture::decode(read,m.Channel,0,e)&&!e.sample.readable,"Invalid sample loop accepted");m.put(m.Wave+8,8);
 m.put(m.Wave,0x40000001);require(GbaVoiceCapture::decode(read,m.Channel,0,e)&&e.sample.readable&&!e.sample.pcm8,"Unsupported sample type silently became PCM8");m.put(m.Wave,0x40000000);
 m.put(m.Wave,0x80000000);require(GbaVoiceCapture::decode(read,m.Channel,0,e)&&e.sample.readable&&!e.sample.pcm8,"Unknown sample status silently became PCM8");m.put(m.Wave,0x40000000);
 m.put(m.Wave+4,0);require(GbaVoiceCapture::decode(read,m.Channel,0,e)&&!e.sample.pcm8,"Zero sample frequency silently accepted");m.put(m.Wave+4,16000*1024);
 m.put(m.Wave+12,0xffffffff);require(GbaVoiceCapture::decode(read,m.Channel,0,e)&&!e.sample.readable,"Oversized sample range");m.put(m.Wave+12,64);
 m.put(m.Channel+36,0xfffffff8);require(GbaVoiceCapture::decode(read,m.Channel,0,e)&&!e.sample.readable,"Wrapped sample pointer");m.put(m.Channel+36,m.Wave);
 auto unchanged=m.ram;auto c=std::make_unique<GbaVoiceCapture>();for(uint64_t i=0;i<8193;i++)c->onWrite(read,m.Info,12,m.Channel,0x80,1,i);events={{}};events[0].slot=99;require(!c->drain(events,error)&&error.find("overflow")!=std::string::npos&&events.size()==1&&events[0].slot==99,"Overflow must fail atomically");require(m.ram==unchanged,"Queue collection wrote RAM");
 c=std::make_unique<GbaVoiceCapture>();c->onWrite(read,m.Info,12,m.Channel,0x80,1,10);c->onWrite(read,m.Info,12,m.Channel,0x80,1,9);require(!c->drain(events,error)&&error.find("backwards")!=std::string::npos,"Clock regression accepted");
 c=std::make_unique<GbaVoiceCapture>();c->onWrite(read,m.Info,13,m.Channel,0x80,1,1);require(c->drain(events,error)&&events.empty(),"Out-of-range voice count");
 std::cout<<"PASS own metadata/prospective byte-half-word/control/gate/end/retrigger/ranges/types/overflow/clock/no-write fixtures\n";
}
static void actual(const std::filesystem::path&p){
 auto bytes=readMusicFile(p);auto baseline=makeBackend(bytes,p);auto observed=makeBackend(bytes,p);require(observed->prepareGbaInspection(),"Actual m4a inspection not available");baseline->continuousEnd(true);observed->continuousEnd(true);
 std::set<int>slots,keys;std::set<uint32_t>samples,tracks;size_t ons=0,ends=0,gates=0,controls=0,readable=0,pcm8=0;uint64_t last=0;double maxError=0;std::vector<GbaVoiceEvent>first;std::vector<Stereo>reference;
 for(int i=0;i<563;i++){
  AudioBlock a,b;baseline->render(512,a);observed->render(512,b);reference.insert(reference.end(),a.mix.begin(),a.mix.end());
  for(size_t j=0;j<a.mix.size();j++)maxError=std::max(maxError,double(std::max(std::abs(a.mix[j].l-b.mix[j].l),std::abs(a.mix[j].r-b.mix[j].r))));
  std::vector<GbaVoiceEvent>events;std::string error;require(observed->drainGbaEvents(events,error),error.c_str());for(auto&e:events){require(e.cpuCycles>=last&&e.cpuCycles<7ull*16777216,"Actual note clock not monotonic/bounded");last=e.cpuCycles;
   if(e.kind==GbaVoiceEventKind::NoteOn){ons++;slots.insert(e.slot);keys.insert(e.midiKey);samples.insert(e.sample.address);tracks.insert(e.sourceTrack);readable+=e.sample.readable;pcm8+=e.sample.pcm8;if(first.size()<5)first.push_back(e);}
   else if(e.kind==GbaVoiceEventKind::GateOff)gates++;else if(e.kind==GbaVoiceEventKind::VoiceEnd)ends++;else controls++;
  }
 }
 require(maxError==0,"Read-only note observer changed original full mix");require(ons>=20&&slots.size()>=4&&keys.size()>=5&&samples.size()>=2&&readable>=ons*9/10,"Actual metadata coverage insufficient");
 require(observed->prepareGbaInspection(),"Inspection restart");std::vector<GbaVoiceEvent>repeat;
 for(int i=0;i<563&&repeat.size()<first.size();i++){AudioBlock b;observed->render(512,b);for(int j=0;j<512;j++)require(b.mix[j].l==reference[i*512+j].l&&b.mix[j].r==reference[i*512+j].r,"Inspection reset changed/reused prefetched audio");std::vector<GbaVoiceEvent>events;std::string error;require(observed->drainGbaEvents(events,error),error.c_str());for(auto&e:events)if(e.kind==GbaVoiceEventKind::NoteOn)repeat.push_back(e);}
 require(repeat.size()>=first.size(),"No repeated notes inside bounded six-second replay");for(size_t i=0;i<first.size();i++)require(repeat[i].generation==first[i].generation&&repeat[i].cpuCycles==first[i].cpuCycles&&repeat[i].midiKey==first[i].midiKey&&repeat[i].slot==first[i].slot,"Inspection reset did not reset timing/generations");
 std::cout<<"PASS actual six-second original_mix_max_error="<<maxError<<" note_on="<<ons<<" gate_off="<<gates<<" voice_end="<<ends<<" controls="<<controls<<" slots="<<slots.size()<<" keys="<<keys.size()<<" sample_headers="<<samples.size()<<" source_tracks="<<tracks.size()<<" readable_samples="<<readable<<" pcm8="<<pcm8<<"\n";
}
int wmain(int argc,wchar_t**argv){try{unit();if(argc>1){auto p=std::filesystem::path(argv[1])/L"channels.gsf";auto core=makeBackend(readMusicFile(p),p);require(!core->prepareGbaInspection(),"Unrecognized engine falsely accepted");std::vector<GbaVoiceEvent>out(1);out[0].slot=99;std::string error="canary";require(!core->drainGbaEvents(out,error)&&out[0].slot==99&&error=="canary","Unsupported output was altered");std::cout<<"PASS unrecognized own GSF returns unsupported\n";}if(argc>2)actual(argv[2]);return 0;}catch(const std::exception&e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
