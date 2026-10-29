#include "GbaRecording.hpp"
#include <new>
#include <tuple>
namespace ngmv::editor {
namespace {
bool sameSample(const GbaSampleInfo&a,const GbaSampleInfo&b){
    return std::tie(a.address,a.rawFrequency,a.loopStart,a.length,a.type,a.status,a.readable,a.pcm8,a.looped)==std::tie(b.address,b.rawFrequency,b.loopStart,b.length,b.type,b.status,b.readable,b.pcm8,b.looped);
}
}
bool GbaNoteRecorder::fail(const char*message,std::string&error){fault_=message;error=fault_;return false;}
bool GbaNoteRecorder::append(const std::vector<GbaVoiceEvent>&events,std::string&error){
    if(!fault_.empty()){error=fault_;return false;}
    if(sealed_){error="GBA recording already finalized";return false;}
    try{for(const auto&e:events){
        if(recording_.events==MaxEvents)return fail("GBA recording event limit exceeded; restart with a shorter capture",error);
        if(e.slot<0||e.slot>=12||!e.generation)return fail("Invalid GBA slot or generation",error);
        if(e.cpuCycles<lastCycles_)return fail("GBA recording clock moved backwards",error);
        auto slot=size_t(e.slot);int active=active_[slot];
        if(e.kind==GbaVoiceEventKind::NoteOn){
            if(recording_.notes.size()==MaxNotes)return fail("GBA recording note limit exceeded",error);
            if(e.generation<=generation_[slot]||e.generation-generation_[slot]!=1)return fail("GBA start generation is missing or repeated; restart inspection",error);
            if(active>=0){auto&previous=recording_.notes[size_t(active)];previous.endCycles=e.cpuCycles;previous.ending=GbaNoteEnding::Stolen;}
            GbaRecordedNote note;note.start=e;recording_.notes.push_back(std::move(note));active_[slot]=int(recording_.notes.size())-1;generation_[slot]=e.generation;
        }else{
            if(active<0||e.generation!=generation_[slot])return fail("GBA event has no matching active note generation",error);
            auto&note=recording_.notes[size_t(active)];
            switch(e.kind){
            case GbaVoiceEventKind::GateOff:
                if(note.gateOff)return fail("Duplicate GBA gate release",error);
                note.gateOff=e;break;
            case GbaVoiceEventKind::VoiceEnd:
                note.endEvent=e;note.endCycles=e.cpuCycles;note.ending=GbaNoteEnding::VoiceEnd;active_[slot]=-1;break;
            case GbaVoiceEventKind::Pitch:case GbaVoiceEventKind::Volume:
                if(e.sourceTrack!=note.start.sourceTrack||!sameSample(e.sample,note.start.sample))recording_.controlMetadataChanges++;
                note.controls.push_back(e);break;
            default:return fail("Unknown GBA event kind",error);
            }
        }
        lastCycles_=e.cpuCycles;recording_.events++;
    }}catch(const std::bad_alloc&){return fail("Not enough memory for GBA recording",error);}
    error.clear();return true;
}
bool GbaNoteRecorder::finish(uint64_t boundary,GbaRecording&out,std::string&error){
    if(!fault_.empty()){error=fault_;return false;}
    if(sealed_){error="GBA recording already finalized";return false;}
    if(boundary<lastCycles_)return fail("GBA recording boundary precedes observed events",error);
    for(auto index:active_)if(index>=0){auto&note=recording_.notes[size_t(index)];note.endCycles=boundary;note.ending=GbaNoteEnding::CaptureBoundary;}
    recording_.endCycles=boundary;out=std::move(recording_);sealed_=true;error.clear();return true;
}
}
