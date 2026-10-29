#include "editor/GbaRecording.hpp"
#include "core/IMusicBackend.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <set>
#include <stdexcept>
#include <tuple>
using namespace ngmv;using namespace ngmv::editor;
static void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
static GbaVoiceEvent event(GbaVoiceEventKind kind,int slot,uint64_t generation,uint64_t cycles,uint32_t track=0x03001000){
 GbaVoiceEvent e;e.kind=kind;e.slot=slot;e.generation=generation;e.cpuCycles=cycles;e.sourceTrack=track;e.frequency=16000;e.midiKey=60;e.key=60;e.velocity=100;e.sample.address=0x08000020;e.sample.rawFrequency=16000*1024;e.sample.length=8;e.sample.readable=e.sample.pcm8=true;return e;
}
static void unit(){
 using K=GbaVoiceEventKind;using E=GbaNoteEnding;
 std::vector<GbaVoiceEvent>events={event(K::NoteOn,0,1,10),event(K::NoteOn,1,1,15),event(K::Pitch,0,1,20),event(K::GateOff,0,1,30),event(K::Volume,0,1,40),event(K::NoteOn,0,2,50),event(K::VoiceEnd,1,1,70),event(K::NoteOn,2,1,80),event(K::GateOff,2,1,90)};
 // Keep changed raw control metadata intact; don't silently attribute setup
 // writes to an audible bend or modify a previous note's original identity.
 events[4].sample.address=0x08000200;events[4].sourceTrack=0x03002000;
 std::string error;auto checked=[&](bool ok){need(ok,error.c_str());};GbaNoteRecorder recorder;checked(recorder.append(events,error));GbaRecording whole;
 need(recorder.finish(100,whole,error)&&error.empty(),"Valid recording finalization");
 need(whole.notes.size()==4&&whole.events==9&&whole.endCycles==100&&whole.controlMetadataChanges==1,"Recording count/metadata-change diagnostic");
 auto&a=whole.notes[0];need(a.start.cpuCycles==10&&a.start.generation==1&&a.start.sourceTrack==0x03001000&&a.start.sample.address==0x08000020,"Original start metadata changed");
 need(a.gateOff&&a.gateOff->cpuCycles==30&&a.endCycles==50&&a.ending==E::Stolen&&!a.endEvent,"Gate/steal/end conflated");
 need(a.controls.size()==2&&a.controls[0].kind==K::Pitch&&a.controls[1].sourceTrack==0x03002000&&a.controls[1].sample.address==0x08000200,"Control identity/order lost");
 need(whole.notes[1].ending==E::VoiceEnd&&whole.notes[1].endEvent&&whole.notes[1].endCycles==70&&!whole.notes[1].gateOff,"One-shot natural end fabricated gate");
 need(whole.notes[2].start.generation==2&&whole.notes[2].ending==E::CaptureBoundary&&whole.notes[2].endCycles==100&&!whole.notes[2].gateOff,"Retrigger/boundary fabricated release");
 need(whole.notes[3].gateOff&&whole.notes[3].gateOff->cpuCycles==90&&whole.notes[3].ending==E::CaptureBoundary,"Released-but-active capture tail lost");
 GbaNoteRecorder chunks;for(auto&e:events)need(chunks.append({e},error),"Single-event chunk failed");GbaRecording split;need(chunks.finish(100,split,error)&&split.events==whole.events&&split.notes.size()==whole.notes.size(),"Chunk-dependent recording");
 for(size_t i=0;i<whole.notes.size();i++){auto&x=whole.notes[i];auto&y=split.notes[i];need(x.start.cpuCycles==y.start.cpuCycles&&x.start.sourceTrack==y.start.sourceTrack&&x.start.generation==y.start.generation&&x.endCycles==y.endCycles&&x.ending==y.ending&&x.controls.size()==y.controls.size()&&bool(x.gateOff)==bool(y.gateOff),"Chunk changed lifetime or identity");}
 GbaRecording canary;canary.events=777;need(!recorder.finish(100,canary,error)&&canary.events==777&&!recorder.append({},error),"Sealed recording accepted new input");
 auto reject=[&](std::vector<GbaVoiceEvent>bad,uint64_t end=100){GbaNoteRecorder r;bool accepted=r.append(bad,error);if(accepted)accepted=r.finish(end,canary,error);need(!accepted&&!error.empty()&&canary.events==777,"Malformed stream succeeded or modified output");need(!r.finish(1000,canary,error)&&canary.events==777,"Faulted stream exported partial success");};
 reject({event(K::NoteOn,-1,1,0)});reject({event(K::NoteOn,12,1,0)});reject({event(K::NoteOn,0,0,0)});reject({event(K::NoteOn,0,2,0)});
 reject({event(K::Pitch,0,1,0)});reject({event(K::GateOff,0,1,0)});reject({event(K::VoiceEnd,0,1,0)});
 reject({event(K::NoteOn,0,1,10),event(K::Pitch,0,1,9)});
 reject({event(K::NoteOn,0,1,0),event(K::NoteOn,0,1,1)});reject({event(K::NoteOn,0,1,0),event(K::NoteOn,0,3,1)});
 reject({event(K::NoteOn,0,1,0),event(K::Pitch,0,2,1)});
 reject({event(K::NoteOn,0,1,0),event(K::GateOff,0,1,1),event(K::GateOff,0,1,2)});
 reject({event(K::NoteOn,0,1,0),event(K::VoiceEnd,0,1,1),event(K::Pitch,0,1,2)});
 reject({event(K::NoteOn,0,1,0),event(static_cast<K>(99),0,1,1)});reject({event(K::NoteOn,0,1,10)},9);
 GbaNoteRecorder zero;GbaRecording z;need(zero.append({event(K::NoteOn,0,1,1),event(K::NoteOn,0,2,1)},error)&&zero.finish(1,z,error)&&z.notes.size()==2&&z.notes[0].endCycles==z.notes[0].start.cpuCycles&&z.notes[1].ending==E::CaptureBoundary,"Zero-cycle retrigger dropped or length invented");
 {GbaNoteRecorder limit;std::vector<GbaVoiceEvent>starts;starts.reserve(GbaNoteRecorder::MaxNotes+1);for(size_t i=0;i<=GbaNoteRecorder::MaxNotes;i++)starts.push_back(event(K::NoteOn,0,i+1,i));need(!limit.append(starts,error)&&error.find("note limit")!=std::string::npos&&!limit.finish(starts.size(),canary,error)&&canary.events==777,"Note limit silently truncated");}
 {GbaNoteRecorder limit;need(limit.append({event(K::NoteOn,0,1,0)},error),"Limit fixture start");std::vector<GbaVoiceEvent>controls(1024,event(K::Pitch,0,1,0));size_t remaining=GbaNoteRecorder::MaxEvents-1;while(remaining){controls.resize(std::min(remaining,size_t(1024)),event(K::Pitch,0,1,0));need(limit.append(controls,error),"Premature event limit");remaining-=controls.size();}need(!limit.append({event(K::Pitch,0,1,0)},error)&&error.find("event limit")!=std::string::npos&&!limit.finish(0,canary,error)&&canary.events==777,"Event limit silently truncated");}
 std::cout<<"PASS original identity/same-track overlap/generation/retrigger/steal/gate/natural-end/boundary/controls/chunk invariance/zero-time/15 malformed cases/sealed/output-canary/32768-note and262144-event limits\n";
}
static void actual(const std::filesystem::path&p){
 auto bytes=readMusicFile(p);auto core=makeBackend(bytes,p);auto baseline=makeBackend(bytes,p);uint64_t clock=99;
 need(!core->gbaEmulatedCycles(clock)&&clock==99,"Clock available without inspection opt-in");need(core->prepareGbaInspection()&&core->gbaEmulatedCycles(clock)&&clock==0,"Clock reset/opt-in");core->continuousEnd(true);baseline->continuousEnd(true);
 GbaNoteRecorder recorder;std::string error;auto checked=[&](bool ok){need(ok,error.c_str());};uint64_t lastClock=0;size_t ons=0,gates=0,ends=0,controls=0;double maxError=0;std::set<uint32_t>tracks,samples;
 for(int i=0;i<563;i++){
  AudioBlock a,b;baseline->render(512,a);core->render(512,b);need(a.mix.size()==b.mix.size(),"Original block size changed");for(size_t j=0;j<a.mix.size();j++)maxError=std::max(maxError,double(std::max(std::abs(a.mix[j].l-b.mix[j].l),std::abs(a.mix[j].r-b.mix[j].r))));
  std::vector<GbaVoiceEvent>events;checked(core->drainGbaEvents(events,error));need(core->gbaEmulatedCycles(clock)&&clock>=lastClock,"Emulated boundary not monotonic");
  for(auto&e:events){need(e.cpuCycles<=clock,"Event after observed emulated boundary");if(e.kind==GbaVoiceEventKind::NoteOn){ons++;tracks.insert(e.sourceTrack);samples.insert(e.sample.address);}else if(e.kind==GbaVoiceEventKind::GateOff)gates++;else if(e.kind==GbaVoiceEventKind::VoiceEnd)ends++;else controls++;}
  checked(recorder.append(events,error));lastClock=clock;
 }
 GbaRecording result;checked(recorder.finish(clock,result,error));need(result.notes.size()==ons&&ons>=20&&tracks.size()>=4&&samples.size()>=2&&maxError==0,"Actual recording coverage/mix regression");
 size_t gotGates=0,gotEnds=0,stolen=0,truncated=0,gotControls=0;
 for(auto&n:result.notes){need(n.endCycles>=n.start.cpuCycles&&n.ending!=GbaNoteEnding::Open,"Unclosed/negative actual note");gotGates+=bool(n.gateOff);gotEnds+=n.ending==GbaNoteEnding::VoiceEnd;stolen+=n.ending==GbaNoteEnding::Stolen;truncated+=n.ending==GbaNoteEnding::CaptureBoundary;gotControls+=n.controls.size();}
 need(gotGates==gates&&gotEnds==ends&&gotControls==controls&&gotEnds+stolen+truncated==ons,"Actual events lost or ending invented");
 auto sameBinding=[](const GbaVoiceEvent&a,const GbaVoiceEvent&b){const auto&x=a.sample;const auto&y=b.sample;return a.sourceTrack==b.sourceTrack&&std::tie(x.address,x.rawFrequency,x.loopStart,x.length,x.type,x.status,x.readable,x.pcm8,x.looped)==std::tie(y.address,y.rawFrequency,y.loopStart,y.length,y.type,y.status,y.readable,y.pcm8,y.looped);};
 std::array<const GbaRecordedNote*,12>next{};size_t observedChanges=0,matchesNextStart=0;
 for(auto i=result.notes.rbegin();i!=result.notes.rend();++i){for(auto&c:i->controls)if(!sameBinding(c,i->start)){observedChanges++;auto future=next[size_t(i->start.slot)];if(future&&sameBinding(c,future->start)&&c.cpuCycles<=future->start.cpuCycles)matchesNextStart++;}next[size_t(i->start.slot)]=&*i;}
 need(observedChanges==result.controlMetadataChanges,"Raw control binding diagnostic mismatch");
 need(core->prepareGbaInspection()&&core->gbaEmulatedCycles(clock)&&clock==0,"Clock not reset with inspection");
 std::cout<<"PASS actual six-second notes="<<ons<<" gates="<<gates<<" natural_ends="<<gotEnds<<" stolen="<<stolen<<" boundary="<<truncated<<" controls="<<gotControls<<" control_metadata_changes="<<result.controlMetadataChanges<<" changed_controls_match_next_start="<<matchesNextStart<<" tracks="<<tracks.size()<<" samples="<<samples.size()<<" emulated_boundary_cycles="<<lastClock<<" original_mix_max_error="<<maxError<<"; no Song conversion or private asset files\n";
}
int wmain(int argc,wchar_t**argv){try{unit();if(argc>1){auto p=std::filesystem::path(argv[1])/L"channels.gsf";auto core=makeBackend(readMusicFile(p),p);uint64_t out=123;need(!core->gbaEmulatedCycles(out)&&out==123,"Unsupported engine clock mutated output");}if(argc>2)actual(argv[2]);return 0;}catch(const std::exception&e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
