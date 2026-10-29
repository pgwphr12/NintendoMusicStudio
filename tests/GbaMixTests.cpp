#include "formats/GbaMixCapture.hpp"
#include "editor/GbaRecording.hpp"
#include "core/IMusicBackend.hpp"
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <tuple>
using namespace ngmv;using namespace ngmv::editor;
static void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
struct Own {
 std::array<uint8_t,32768>ram{};std::array<uint8_t,1024>rom{};
 static constexpr uint32_t Info=0x03000100,Channel=Info+0x50,Wave=0x08000100;
 const uint8_t*read(uint32_t p,size_t n)const{if(p>=0x03000000&&p-0x03000000<=ram.size()&&n<=ram.size()-(p-0x03000000))return ram.data()+p-0x03000000;if(p>=0x08000000&&p-0x08000000<=rom.size()&&n<=rom.size()-(p-0x08000000))return rom.data()+p-0x08000000;return nullptr;}
 void put(uint32_t p,uint32_t v,int n=4){auto q=const_cast<uint8_t*>(read(p,n));need(q!=nullptr,"Own memory write bounds");for(int j=0;j<n;j++)q[j]=uint8_t(v>>(j*8));}
 Own(){put(Info,0x68736d54);put(Info+7,15,1);put(Info+16,8);put(Info+20,16384);put(Info+24,512);put(Channel+8,60,1);put(Channel+17,60,1);put(Channel+18,100,1);put(Channel+2,64,1);put(Channel+3,96,1);put(Channel+9,128,1);put(Channel+10,32,1);put(Channel+24,64);put(Channel+32,15360);put(Channel+36,Wave);put(Channel+40,Wave+16);put(Channel+44,0x03002000);put(Wave,0x40000000);put(Wave+4,16000*1024);put(Wave+8,8);put(Wave+12,64);}
};
static void unit(){
 Own m;auto reader=[&](uint32_t p,size_t n){return m.read(p,n);};auto notes=std::make_unique<GbaVoiceCapture>();notes->onWrite(reader,m.Info,1,m.Channel,0x80,1,1);m.put(m.Channel,3,1);
 std::unique_ptr<GbaMixCapture>mix;auto fresh=[&]{mix=std::make_unique<GbaMixCapture>();m.put(m.Info,0x68736d53);mix->onWrite(reader,*notes,m.Info,1,0,m.Info,0x68736d54,4,2,0,0);m.put(m.Info,0x68736d54);};fresh();auto before=m.ram;auto beforeROM=m.rom;
 auto emit=[&](int offset=11,uint32_t value=48,int width=1,uint64_t time=20,uint32_t reg=Own::Channel){mix->onWrite(reader,*notes,m.Info,1,reg,m.Channel+offset,value,width,time,1024,0x9a0c);};
 emit(2,100);emit(10,32);emit(11,48,1,20,m.Channel+64);m.put(m.Info,0x68736d53);emit();m.put(m.Info,0x68736d54);
 std::vector<GbaMixFrame>frames;std::string error;need(mix->drain(frames,error)&&frames.empty(),"Sequencer/right-gain/unlocked/wrong-register write captured as mixer frame");
 m.put(m.Channel,0,1);emit();m.put(m.Channel,0x80,1);emit();m.put(m.Channel,3,1);need(mix->drain(frames,error)&&frames.empty(),"Inactive cleanup or unconsumed START was captured as mixed audio");
 emit();emit(10,0x3020,2,21);emit(8,0x3020803c,4,22);need(mix->drain(frames,error)&&frames.size()==3&&error.empty(),"Prospective byte/halfword/word mixer snapshots");
 auto validMixerVoice=frames[0].voice;
 need(frames[0].lockCycles==2,"Shared pre-mix lock timestamp lost");
 for(auto&f:frames)need(f.voice.kind==GbaVoiceEventKind::MixerState&&f.lockEpoch==1&&f.voice.generation==1&&f.voice.midiKey==60&&f.voice.sourceTrack==0x03002000&&f.voice.envelope==128&&f.gainRight==32&&f.gainLeft==48&&f.masterVolume==15&&f.samplesPerVblank==8&&f.pcmRate==16384&&f.divFreq==512&&f.timer0Period==1024&&f.soundControl==0x9a0c,"Original mixer state fields wrong");
 need(m.ram==before&&m.rom==beforeROM,"Mixer snapshot changed emulated memory");
 GbaMixPhase phase;need(predictGbaMixPhase(frames[0],phase)&&phase.offset==7&&phase.fraction==(1u<<22)&&phase.remaining==57&&!phase.ended,"Independent 15/16 sample increment prediction");
 auto f=frames[0];f.sourcePointer=m.Wave+16+60;f.sourceRemaining=4;f.voice.frequency=24576;need(predictGbaMixPhase(f,phase)&&phase.offset==16&&phase.fraction==0&&phase.remaining==48,"Loop wrap phase prediction");f.voice.sample.looped=false;f.voice.sample.status=0;need(predictGbaMixPhase(f,phase)&&phase.ended,"One-shot termination prediction");
 f=frames[0];f.voice.type=8;f.voice.frequency=1;need(predictGbaMixPhase(f,phase)&&phase.offset==8&&phase.fraction==0,"Fixed-rate tone incorrectly uses frequency");
 auto reject=[&](GbaMixFrame invalid){GbaMixPhase out;out.offset=999;need(!predictGbaMixPhase(invalid,out)&&out.offset==999,"Unsupported phase succeeded or modified output");};
 f=frames[0];f.voice.type=16;reject(f);f=frames[0];f.voice.sample.pcm8=false;reject(f);f=frames[0];f.sourceFraction=1u<<23;reject(f);f=frames[0];f.sourceRemaining--;reject(f);f=frames[0];f.sourcePointer=m.Wave+15;reject(f);f=frames[0];f.divFreq=0;reject(f);f=frames[0];f.voice.frequency=0xffffffff;reject(f);f=frames[0];f.voice.sample.loopStart=64;reject(f);f=frames[0];f.samplesPerVblank=705;reject(f);
 f=frames[0];f.voice.sample.type=1;reject(f);f=frames[0];f.voice.sample.status=0x2000;reject(f);f=frames[0];f.voice.sample.looped=false;reject(f);f=frames[0];f.voice.sample.rawFrequency=0;reject(f);
 auto canary=[&](const char*message){frames.resize(1);frames[0].voice.slot=99;need(!mix->drain(frames,error)&&frames.size()==1&&frames[0].voice.slot==99&&error.find(message)!=std::string::npos,"Fault did not preserve caller output/error");};
 fresh();emit();emit(11,48,1,19);canary("backwards");
 fresh();for(int i=0;i<8193;i++)emit();canary("overflow");
 fresh();auto missing=std::make_unique<GbaVoiceCapture>();mix->onWrite(reader,*missing,m.Info,1,m.Channel,m.Channel+11,48,1,20,0,0);canary("generation");
 GbaNoteRecorder recorder;auto start=validMixerVoice;start.kind=GbaVoiceEventKind::NoteOn;need(recorder.append({start},error)&&!recorder.append({validMixerVoice},error)&&error.find("Unknown")!=std::string::npos,"MixerState masqueraded as a sequencer event");
 std::cout<<"PASS mixer byte/half/word/register/lock/generation/canary/overflow/clock/no-write and independent forward/fixed/loop/one-shot/unsupported phase fixtures\n";
}
static void actual(const std::filesystem::path&p){
 auto bytes=readMusicFile(p);auto observed=makeBackend(bytes,p);auto baseline=makeBackend(bytes,p);std::vector<GbaMixFrame>unsupported(1);unsupported[0].voice.slot=88;std::string error="canary";
 need(!observed->drainGbaMixFrames(unsupported,error)&&unsupported[0].voice.slot==88&&error=="canary","Mixer API active without opt-in");need(observed->prepareGbaMixerInspection(),"Mixer inspection unavailable");baseline->continuousEnd(true);observed->continuousEnd(true);
 std::map<int,GbaMixFrame>previous;std::map<std::pair<int,uint64_t>,GbaVoiceEvent>starts;std::set<std::pair<int,uint64_t>>mixed;
 std::set<int>slots;std::set<uint32_t>rates,periods,controlValues;std::set<uint64_t>epochs;size_t snapshots=0,phaseCompared=0,phaseMismatch=0,unsupportedPhase=0,gainMismatch=0,bindingMismatch=0,fixed=0,loops=0;double maxError=0;uint64_t last=0;std::vector<GbaMixFrame>first;
 auto checked=[&](bool ok){need(ok,error.c_str());};
 for(int i=0;i<563;i++){
  AudioBlock a,b;baseline->render(512,a);observed->render(512,b);need(a.mix.size()==b.mix.size(),"Original render size changed");for(size_t j=0;j<a.mix.size();j++)maxError=std::max(maxError,double(std::max(std::abs(a.mix[j].l-b.mix[j].l),std::abs(a.mix[j].r-b.mix[j].r))));
  std::vector<GbaVoiceEvent>events;checked(observed->drainGbaEvents(events,error));for(auto&e:events)if(e.kind==GbaVoiceEventKind::NoteOn)starts[{e.slot,e.generation}]=e;
  std::vector<GbaMixFrame>frames;checked(observed->drainGbaMixFrames(frames,error));
  for(auto&f:frames){
   need(f.voice.kind==GbaVoiceEventKind::MixerState&&f.voice.cpuCycles>=last&&f.voice.generation>0&&f.lockEpoch>0&&f.lockCycles<=f.voice.cpuCycles,"Invalid mixer kind/clock/generation/lock");last=f.voice.cpuCycles;snapshots++;slots.insert(f.voice.slot);rates.insert(f.pcmRate);periods.insert(f.timer0Period);controlValues.insert(f.soundControl);epochs.insert(f.lockEpoch);fixed+=(f.voice.type&8)!=0;loops+=f.voice.sample.looped;if(first.size()<5)first.push_back(f);
   auto identity=std::make_pair(f.voice.slot,f.voice.generation);mixed.insert(identity);auto start=starts.find(identity);need(start!=starts.end()&&start->second.cpuCycles<=f.voice.cpuCycles,"Mixer has no prior original start");
   const auto&aSample=start->second.sample;const auto&bSample=f.voice.sample;
   bindingMismatch+=start->second.sourceTrack!=f.voice.sourceTrack||start->second.midiKey!=f.voice.midiKey||std::tie(aSample.address,aSample.rawFrequency,aSample.loopStart,aSample.length,aSample.type,aSample.status)!=std::tie(bSample.address,bSample.rawFrequency,bSample.loopStart,bSample.length,bSample.type,bSample.status);
   int envelope=((int(f.masterVolume)+1)*f.voice.envelope)>>4;
   gainMismatch+=f.gainRight!=((f.voice.rightVolume*envelope)>>8)||f.gainLeft!=((f.voice.leftVolume*envelope)>>8);
   auto prior=previous.find(f.voice.slot);if(prior!=previous.end()&&prior->second.voice.generation==f.voice.generation){
    need(prior->second.lockEpoch<f.lockEpoch,"Duplicate same-voice mix observation in one lock epoch");GbaMixPhase predicted;if(predictGbaMixPhase(prior->second,predicted)&&!predicted.ended){phaseCompared++;uint32_t offset=f.sourcePointer-(f.voice.sample.address+16u);if(predicted.offset!=offset||predicted.fraction!=f.sourceFraction||predicted.remaining!=f.sourceRemaining)phaseMismatch++;}else unsupportedPhase++;
   }
   previous[f.voice.slot]=f;
  }
 }
 std::cout<<"OBSERVED frames="<<snapshots<<" epochs="<<epochs.size()<<" slots="<<slots.size()<<" mixed_allocations="<<mixed.size()<<" binding_mismatch="<<bindingMismatch<<" phase_compared="<<phaseCompared<<" phase_mismatch="<<phaseMismatch<<" unsupported_phase="<<unsupportedPhase<<" gain_mismatch="<<gainMismatch<<" fixed_frames="<<fixed<<" loop_frames="<<loops<<" rates=";for(auto x:rates)std::cout<<x<<',';std::cout<<" timer0_periods=";for(auto x:periods)std::cout<<x<<',';std::cout<<" sound_control=";for(auto x:controlValues)std::cout<<x<<',';std::cout<<" original_mix_max_error="<<maxError<<'\n';
 need(snapshots>=100&&slots.size()>=4&&phaseCompared>=100&&phaseMismatch==0&&gainMismatch==0&&bindingMismatch==0&&maxError==0,"Actual mixer state/rate/phase/gain or full-mix check failed");
 need(observed->prepareGbaMixerInspection(),"Mixer inspection reset failed");std::vector<GbaMixFrame>repeat;
 for(int i=0;i<563&&repeat.size()<first.size();i++){AudioBlock b;observed->render(512,b);std::vector<GbaVoiceEvent>events;checked(observed->drainGbaEvents(events,error));std::vector<GbaMixFrame>frames;checked(observed->drainGbaMixFrames(frames,error));repeat.insert(repeat.end(),frames.begin(),frames.end());}
 need(repeat.size()>=first.size(),"Reset produced no mixer frames");for(size_t i=0;i<first.size();i++)need(first[i].voice.cpuCycles==repeat[i].voice.cpuCycles&&first[i].voice.generation==repeat[i].voice.generation&&first[i].lockEpoch==repeat[i].lockEpoch&&first[i].lockCycles==repeat[i].lockCycles&&first[i].sourcePointer==repeat[i].sourcePointer&&first[i].sourceFraction==repeat[i].sourceFraction&&first[i].gainRight==repeat[i].gainRight&&first[i].gainLeft==repeat[i].gainLeft,"Mixer state reset not deterministic");
 need(observed->prepareGbaInspection(),"Return to raw-only inspection failed");unsupported[0].voice.slot=88;error="canary";need(!observed->drainGbaMixFrames(unsupported,error)&&unsupported[0].voice.slot==88&&error=="canary","Raw-only mode retained mixer queue");
 std::cout<<"PASS actual mixer-state phase/gain/full-mix/reset/explicit-mode checks; emulated mixer time not calibrated audible time; no asset files\n";
}
int wmain(int argc,wchar_t**argv){try{unit();if(argc>1){auto p=std::filesystem::path(argv[1])/L"channels.gsf";auto core=makeBackend(readMusicFile(p),p);need(!core->prepareGbaMixerInspection(),"Unknown engine mixer accepted");std::vector<GbaMixFrame>out(1);out[0].voice.slot=99;std::string error="canary";need(!core->drainGbaMixFrames(out,error)&&out[0].voice.slot==99&&error=="canary","Unsupported mixer outputs changed");}if(argc>2)actual(argv[2]);return 0;}catch(const std::exception&e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
