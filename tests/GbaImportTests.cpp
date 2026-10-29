#include "editor/GbaImport.hpp"
#include "editor/NoteImport.hpp"
#include "editor/SampleSong.hpp"
#include "formats/GbaMixCapture.hpp"
#include "../third_party/viogsf/vbam/apu/Gb_Apu.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace ngmv;using namespace ngmv::editor;
static void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
static GbaImportCapture own(){
 GbaImportCapture c;GbaSampleData sample;sample.info.address=0x08000100;sample.info.rawFrequency=16384*1024;sample.info.length=64;sample.info.loopStart=8;sample.info.status=0x4000;sample.info.looped=sample.info.readable=sample.info.pcm8=true;for(int i=0;i<64;i++)sample.pcm.push_back(int8_t(i*4-128));c.samples.push_back(sample);
 auto other=sample;other.info.address=0x08000200;other.info.status=0;other.info.looped=false;other.info.loopStart=0;other.info.length=32;other.pcm.resize(32);for(size_t i=0;i<other.pcm.size();i++)other.pcm[i]=int8_t(127-int(i)*4);c.samples.push_back(other);
 for(int slot=0;slot<2;slot++){GbaRecordedNote note;auto&e=note.start;e.kind=GbaVoiceEventKind::NoteOn;e.slot=slot;e.generation=1;e.cpuCycles=100+slot*20;e.sourceTrack=0x03002000;e.midiKey=slot?72:60;e.key=e.midiKey;e.velocity=slot?100:127;e.type=slot?8:0;e.frequency=slot?1:15360;e.sample=sample.info;note.endCycles=slot?30000:50000;note.ending=slot?GbaNoteEnding::VoiceEnd:GbaNoteEnding::CaptureBoundary;c.recording.notes.push_back(note);}
 c.recording.endCycles=50000;
 for(int frame=0;frame<3;frame++)for(int slot=0;slot<2;slot++){
  GbaMixFrame f;f.voice=c.recording.notes[slot].start;f.voice.kind=GbaVoiceEventKind::MixerState;f.lockEpoch=frame+1;f.lockCycles=uint64_t(frame)*8192;f.voice.cpuCycles=f.lockCycles+200+slot*20;f.samplesPerVblank=8;f.pcmRate=16384;f.divFreq=512;f.timer0Period=1024;f.soundControl=0x210c;
  uint32_t offset=slot?uint32_t(frame*8):frame==0?0:frame==1?8:20;f.sourcePointer=sample.info.address+16+offset;f.sourceRemaining=64-offset;f.sourceFraction=!slot&&!frame?1u<<22:0;
  if(!slot&&frame)f.voice.frequency=24576;f.gainRight=slot?16:frame==2?16:32;f.gainLeft=slot?16:frame==2?64:48;c.frames.push_back(f);
 }return c;
}
static std::vector<Stereo>render(const Song&s){SampleSongRenderer r(s);std::vector<Stereo>result;while(r.position()<r.frames()){std::vector<Stereo>block;r.render(size_t(std::min<uint64_t>(4096,r.frames()-r.position())),block);result.insert(result.end(),block.begin(),block.end());}return result;}
static std::string read(const std::filesystem::path&p){std::ifstream f(p,std::ios::binary);return {(std::istreambuf_iterator<char>(f)),{}};}
static void psg(){
 GBA::Gb_Apu apu;apu.reset(GBA::Gb_Apu::mode_agb,true);apu.viewer_inspection(true);
 auto write=[&](unsigned address,int value){apu.write_register(0,address,value);};
 write(0xff26,0x80);write(0xff24,0x77);write(0xff25,0x11);write(0xff12,8);write(0xff14,0x80);apu.end_frame(8192);need(apu.viewer_psg_mask()==0,"Zero-envelope DAC setup counted as a PSG musical voice");
 write(0xff12,0xf0);write(0xff14,0x80);apu.end_frame(8192);need(apu.viewer_psg_mask()==1,"Sourced PSG pulse was not detected");
 write(0xff12,0);apu.end_frame(8192);need(apu.viewer_psg_mask()==1,"PSG history was not sticky");apu.viewer_inspection(true);write(0xff25,0);write(0xff12,0xf0);write(0xff14,0x80);apu.end_frame(8192);need(apu.viewer_psg_mask()==0,"Unrouted channel detected as audible PSG");
 apu.reset(GBA::Gb_Apu::mode_agb,true);apu.viewer_inspection(true);write(0xff26,0x80);write(0xff25,0x44);write(0xff1a,0x80);write(0xff1c,0x80);write(0xff1e,0x80);apu.end_frame(8192);need(apu.viewer_psg_mask()==4,"AGB wave75percent level not detected");
 apu.reset(GBA::Gb_Apu::mode_agb,true);apu.viewer_inspection(true);write(0xff26,0x80);write(0xff25,0x88);write(0xff21,0xf0);write(0xff23,0x80);apu.end_frame(8192);need(apu.viewer_psg_mask()==8,"PSG noise not detected");
 apu.viewer_inspection(false);apu.end_frame(8192);need(apu.viewer_psg_mask()==0,"Disabled inspector kept collecting PSG");
 std::array<uint8_t,512>memory{};auto read=[&](uint32_t p,size_t n)->const uint8_t*{return p>=0x03000000&&p-0x03000000<=memory.size()&&n<=memory.size()-(p-0x03000000)?memory.data()+p-0x03000000:nullptr;};
 uint32_t base=0x03000100;for(int i=0;i<4;i++)memory[28+i]=uint8_t(base>>(i*8));memory[0x101]=1;auto notes=std::make_unique<GbaVoiceCapture>();auto mix=std::make_unique<GbaMixCapture>();
 mix->onWrite(read,*notes,0x03000000,1,0,base,0x80,1,1,0,0);need(mix->psgNoteSeen(),"Muted original CGB sequencer START was lost");
 std::cout<<"PASS PSG source-state gate: zero-envelope DAC bias excluded; actual pulse/noise/AGB-wave/routing/history/default-off and muted original CGB START supported\n";
}
static void unit(const std::filesystem::path&out){
 auto c=own();std::atomic<bool>cancel{false};auto result=projectGbaNotes(c,120,cancel);auto&s=result.song;
 need(s.sampled&&s.resolution==960&&s.notes.size()==2&&s.instruments.size()==1&&s.samples.size()==2&&result.sourceTracks==1&&result.controlPoints==4,"Original note/track/sample projection or unchanged-state coalescing");
 need(s.notes[0].channel==s.notes[1].channel&&s.notes[0].pitch==60&&s.notes[1].pitch==72&&s.notes[0].velocity==12&&s.notes[1].velocity==9,"Logical MIDI/polyphony/headroom lost");
 need(s.notes[0].sourceOffset==.5&&s.notes[0].controls->size()==3&&s.notes[1].controls->size()==1,"Fractional phase or fixed-rate controls lost");
 auto restored=Song::parse(s.serialize());need(restored.serialize()==s.serialize()&&s.serialize().rfind("NGMS 4",0)==0,"Imported Song is not in common version4 project");
 auto solo=restored;solo.notes.resize(1);auto audio=render(solo);need(std::abs(audio[0].l-float(-126./128*48/256))<1e-7&&std::abs(audio[0].r-float(-126./128*32/256))<1e-7,"Original stereo envelope/velocity/master was multiplied twice");
 double hz=solo.samples[0].sampleRate*std::pow(2.,(solo.notes[0].pitch-60)/12.)*solo.notes[0].controls->front().rate;need(std::abs(hz-15360)<1e-8,"Applied variable source rate not normalized correctly");
 auto&fixed=restored.notes[1];double fixedHz=restored.samples[0].sampleRate*std::pow(2.,(fixed.pitch-60)/12.)*fixed.controls->front().rate;need(std::abs(fixedHz-16384)<1e-8,"Fixed-rate source used ignored frequency field");
 auto before=s.serialize();c.samples[0].pcm[0]=99;need(s.serialize()==before,"Imported sample bank aliases capture memory");
 std::atomic<int>progress{0};exportSongWav(restored,out/L"own-projected.wav",cancel,progress);auto bytes=read(out/L"own-projected.wav");auto exact=render(restored);need(progress==100&&bytes.size()==56+exact.size()*8&&!std::memcmp(bytes.data()+56,exact.data(),exact.size()*8),"Projected Song WAV is not current common-model audio");
 auto reject=[&](GbaImportCapture bad){bool failed=false;try{projectGbaNotes(bad,120,cancel);}catch(const std::exception&){failed=true;}need(failed,"Unsupported/inconsistent capture falsely imported");};
 auto bad=own();bad.psgObserved=true;reject(bad);bad=own();bad.samples[0].info.address=0x02000100;reject(bad);bad=own();bad.samples[0].info.type=1;reject(bad);bad=own();bad.samples[0].pcm.pop_back();reject(bad);bad=own();bad.frames[0].soundControl|=0x400;reject(bad);bad=own();bad.frames[0].voice.type=16;reject(bad);bad=own();bad.frames[0].voice.sourceTrack+=80;reject(bad);bad=own();bad.frames[2].sourceFraction=1;reject(bad);bad=own();bad.frames[0].sourceRemaining--;reject(bad);bad=own();bad.frames[2].lockCycles=4000;bad.frames[2].voice.cpuCycles=4200;bad.frames[3].lockCycles=4000;bad.frames[3].voice.cpuCycles=4220;reject(bad);bad=own();bad.frames.erase(bad.frames.begin()+2);reject(bad);bad=own();bad.recording.notes[0].ending=GbaNoteEnding::Open;reject(bad);bad=own();bad.samples.clear();reject(bad);bad=own();bad.recording.notes[0].start.generation=3;reject(bad);
 cancel=true;bool stopped=false;try{projectGbaNotes(own(),120,cancel);}catch(const std::exception&){stopped=true;}need(stopped,"Projection cancellation ignored");
 std::cout<<"PASS own original-MIDI/polyphonic track/owned bank/phase/fixed-variable rate/stereo gain/no-double-velocity/common project+WAV/coalescing/14 unsupported cases/cancel\n";
}
static void actual(const std::filesystem::path&source,const std::filesystem::path&privateOut){
 std::atomic<bool>cancel{false};std::atomic<int>progress{0};auto imported=importGbaNotes(source,6,120,cancel,progress);auto&s=imported.song;need(progress==100&&s.sampled&&s.notes.size()>=20&&s.instruments.size()>=4&&s.samples.size()>=2&&imported.controlPoints>=s.notes.size(),"Actual original GBA note import coverage");
 std::filesystem::create_directories(privateOut);auto project=privateOut/L"private-import.ngms";{std::ofstream f(project,std::ios::binary);f<<s.serialize();}auto loaded=Song::parse(read(project));need(loaded.serialize()==s.serialize(),"Actual imported bank/control project did not reopen");auto original=render(loaded);double energy=0;for(auto x:original)energy+=x.l*x.l+x.r*x.r;need(energy>1,"Actual imported PCM song is silent");
 for(int edit=0;edit<6;edit++){auto changed=loaded;auto&n=changed.notes[0];if(edit==0)n.pitch+=12;if(edit==1)n.step+=12;if(edit==2)n.length=std::max(1,n.length/2);if(edit==3)n.velocity=std::max(1,n.velocity/2);if(edit==4)n.sample=(n.sample+1)%int(changed.samples.size());if(edit==5)changed.instruments[n.channel].mute=true;changed=Song::parse(changed.serialize());auto audio=render(changed);need(audio.size()==original.size()&&std::memcmp(audio.data(),original.data(),audio.size()*8)!=0,"Actual imported musical edit did not affect sound");auto wav=privateOut/(L"private-edit-"+std::to_wstring(edit)+L".wav");exportSongWav(changed,wav,cancel,progress);auto bytes=read(wav);need(bytes.size()==56+audio.size()*8&&!std::memcmp(bytes.data()+56,audio.data(),audio.size()*8),"Actual imported edit was not rendered to WAV");}
 std::cout<<"PASS actual import notes="<<s.notes.size()<<" tracks="<<s.instruments.size()<<" samples="<<s.samples.size()<<" bank_bytes="<<imported.sampleBytes<<" frames="<<imported.mixerFrames<<" coalesced_points="<<imported.controlPoints<<" boundary_notes="<<imported.truncatedNotes<<" project/reopen/6independent-edits/exact-edited-WAV; private artifacts only, dry reconstruction not bit-exact original\n";
}
int wmain(int argc,wchar_t**argv){try{need(argc>=2,"Usage: gba_import_tests own-output [actual-GSF private-output]");std::filesystem::path out=argv[1];std::filesystem::create_directories(out);psg();unit(out);if(argc>=4)actual(argv[2],argv[3]);return 0;}catch(const std::exception&e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
