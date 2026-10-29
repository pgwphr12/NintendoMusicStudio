#include "GbaImport.hpp"
#include "core/IMusicBackend.hpp"
#include "formats/GbaMixCapture.hpp"
#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <stdexcept>
#include <tuple>
namespace ngmv::editor {
namespace {
constexpr double Cpu=16777216.,Phase=8388608.;
using Identity=std::pair<int,uint64_t>;
void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
bool sameSample(const GbaSampleInfo&a,const GbaSampleInfo&b){return std::tie(a.address,a.rawFrequency,a.loopStart,a.length,a.type,a.status,a.readable,a.pcm8,a.looped)==std::tie(b.address,b.rawFrequency,b.loopStart,b.length,b.type,b.status,b.readable,b.pcm8,b.looped);}
void checkSample(const GbaSampleData&s){
    const auto&i=s.info;auto region=i.address>>24;uint32_t offset=i.address&0x1ffffffu;
    need(region>=8&&region<=13,"Mutable RAM samples are not supported by GBA note import");
    need(i.readable&&i.pcm8&&i.type==0&&!(i.status&~0x4000u)&&i.rawFrequency&&i.length==s.pcm.size()&&!s.pcm.empty()&&i.length<=8u*1024u*1024u,"Unsupported or incomplete GBA PCM8 bank");
    need(i.looped==bool(i.status&0x4000u)&&(!i.looped||i.loopStart<i.length)&&offset<=0x2000000u-16&&i.length<=0x2000000u-offset-16,"Invalid GBA ROM sample bounds or loop");
}
bool trackAddress(uint32_t p){auto region=p>>24,offset=p&0xffffffu;return !(p&3u)&&((region==2&&offset<=0x40000u-80)||(region==3&&offset<=0x8000u-80));}
double increment(const GbaMixFrame&f){return (f.voice.type&8u)?1.:double(uint64_t(f.voice.frequency)*f.divFreq)/Phase;}
}
GbaImportResult projectGbaNotes(const GbaImportCapture&capture,int bpm,std::atomic<bool>&cancel){
    need(!cancel.load(),"Note import cancelled");need(bpm>=30&&bpm<=300,"Import BPM must be30–300");
    need(!capture.psgObserved,"This GBA source contains PSG sound; PSG notes cannot yet be imported without losing voices");
    need(!capture.recording.notes.empty()&&!capture.frames.empty(),"No supported original GBA notes/mixer states in the selected range");
    need(capture.recording.notes.size()<=32768&&capture.frames.size()<=524288,"GBA import capture exceeds limits");
    need(capture.recording.endCycles>0&&capture.recording.endCycles/Cpu<=512*240./bpm,"GBA capture exceeds512 bars at this BPM");
    GbaImportResult result;auto&s=result.song;s.sampled=true;s.bpm=bpm;s.resolution=960;s.title="Imported GBA song";s.instruments.clear();
    s.bars=std::max(1,int(std::ceil(capture.recording.endCycles/Cpu*bpm/240.)));
    const double ticksPerCycle=bpm*s.resolution/(60.*Cpu);
    std::map<uint32_t,int>bank,tracks;
    need(capture.samples.size()<=256,"Too many original GBA samples");
    for(auto&data:capture.samples){
        need(!cancel.load(),"Note import cancelled");checkSample(data);need(bank.emplace(data.info.address,int(s.samples.size())).second,"Duplicate GBA sample identity");
        double referenceRate=data.info.rawFrequency/1024.;need(referenceRate>=100&&referenceRate<=384000,"Unsupported GBA sample reference normalization");
        s.samples.push_back({"GBA sample "+std::to_string(s.samples.size()+1),60,referenceRate,data.info.looped,data.info.loopStart,std::make_shared<const std::vector<int8_t>>(data.pcm)});
        result.sampleBytes+=data.pcm.size();need(result.sampleBytes<=8u*1024u*1024u,"GBA bank exceeds8 MiB");
    }
    std::map<Identity,size_t>original;std::map<Identity,std::vector<const GbaMixFrame*>>frames;
    for(size_t i=0;i<capture.recording.notes.size();i++){
        const auto&n=capture.recording.notes[i];const auto&e=n.start;
        need(e.kind==GbaVoiceEventKind::NoteOn&&e.slot>=0&&e.slot<12&&e.generation&&e.midiKey>=0&&e.midiKey<=127&&e.velocity>=0&&e.velocity<=127&&trackAddress(e.sourceTrack)&&!(e.type&~8u),"Unsupported original GBA note identity/type");
        need(n.ending!=GbaNoteEnding::Open&&n.endCycles>=e.cpuCycles&&n.endCycles<=capture.recording.endCycles,"Incomplete GBA note lifetime");
        need(original.emplace(Identity{e.slot,e.generation},i).second,"Duplicate original GBA note generation");
    }
    struct Group {uint64_t cycles;uint32_t samples,period;};std::map<uint64_t,Group>groups;
    uint64_t lastCycles=0,lastEpoch=0;
    for(auto&f:capture.frames){
        need(!cancel.load(),"Note import cancelled");
        need(f.voice.kind==GbaVoiceEventKind::MixerState&&f.lockEpoch>0&&f.lockEpoch>=lastEpoch&&f.voice.cpuCycles>=lastCycles&&f.lockCycles<=f.voice.cpuCycles&&f.voice.cpuCycles<=capture.recording.endCycles,"Invalid GBA mixer ordering/boundary");
        need(f.timer0Period>0&&!(f.soundControl&0x4400u),"GBA import currently requires both DirectSound buses on observed timer0");
        GbaMixPhase predicted;need(predictGbaMixPhase(f,predicted),"Unsupported GBA mixer encoding/phase/rate");
        auto found=original.find({f.voice.slot,f.voice.generation});need(found!=original.end(),"Mixer state has no original GBA note");const auto&note=capture.recording.notes[found->second];
        need(note.start.cpuCycles<=f.voice.cpuCycles&&f.voice.cpuCycles<=note.endCycles&&f.voice.sourceTrack==note.start.sourceTrack&&f.voice.midiKey==note.start.midiKey&&f.voice.type==note.start.type&&sameSample(f.voice.sample,note.start.sample),"Original GBA note/mixer binding changed");
        auto inserted=groups.emplace(f.lockEpoch,Group{f.lockCycles,f.samplesPerVblank,f.timer0Period});const auto&g=inserted.first->second;
        need(g.cycles==f.lockCycles&&g.samples==f.samplesPerVblank&&g.period==f.timer0Period,"Mixer group timing is inconsistent");
        frames[{f.voice.slot,f.voice.generation}].push_back(&f);lastCycles=f.voice.cpuCycles;lastEpoch=f.lockEpoch;
    }
    std::map<uint64_t,size_t>groupIndex;uint64_t priorTime=0;size_t order=0;for(auto&g:groups){need(order==0||g.second.cycles>priorTime,"Mixer lock timestamps are not increasing");priorTime=g.second.cycles;groupIndex[g.first]=order++;}
    for(auto&raw:capture.recording.notes){
        need(!cancel.load(),"Note import cancelled");const auto&start=raw.start;auto found=frames.find({start.slot,start.generation});
        need(found!=frames.end()&&!found->second.empty(),"A GBA start has no completed mixer observation; choose a longer range");auto&observations=found->second;const auto&first=*observations.front();
        auto sample=bank.find(start.sample.address);need(sample!=bank.end()&&sameSample(start.sample,capture.samples[size_t(sample->second)].info),"Original GBA sample was not owned/copied");
        auto track=tracks.emplace(start.sourceTrack,int(tracks.size()));if(track.second){need(tracks.size()<=64,"Too many GBA musical tracks");Instrument instrument;instrument.volume=10;instrument.sample=sample->second;instrument.name="GBA track "+std::to_string(tracks.size());s.instruments.push_back(std::move(instrument));}
        Note note;note.channel=track.first->second;note.sample=sample->second;note.pitch=start.midiKey;note.velocity=std::clamp(int(std::lround(start.velocity*12./127)),1,12);
        note.step=int(std::llround(first.lockCycles*ticksPerCycle));note.sourceOffset=first.sourcePointer-(first.voice.sample.address+16u)+first.sourceFraction/Phase;
        const auto&last=*observations.back();uint64_t end=std::min<uint64_t>(capture.recording.endCycles,last.lockCycles+uint64_t(last.samplesPerVblank)*last.timer0Period);
        // A later generation on the same physical slot replaces this one,
        // while independent slots on one musical track remain polyphonic.
        if(raw.ending==GbaNoteEnding::Stolen){auto future=frames.find({start.slot,start.generation+1});if(future!=frames.end())end=std::min(end,future->second.front()->lockCycles);}
        need(end>first.lockCycles,"GBA note has no captured duration");note.length=int(std::llround(end*ticksPerCycle))-note.step;
        need(note.length>0,"GBA note is shorter than import timing resolution; no duration was invented");
        auto controls=std::make_shared<std::vector<NoteControl>>();double base=s.samples[size_t(note.sample)].sampleRate*std::pow(2.,(note.pitch-60)/12.);
        double userGain=.25*(s.instruments[size_t(note.channel)].volume/15.)*(note.velocity/15.);
        for(size_t i=0;i<observations.size();i++){
            const auto&f=*observations[i];
            if(i){const auto&previous=*observations[i-1];GbaMixPhase expected;
                need(groupIndex[f.lockEpoch]==groupIndex[previous.lockEpoch]+1&&f.lockEpoch>previous.lockEpoch,"GBA voice skipped or repeated a mixer group");
                uint64_t period=uint64_t(previous.samplesPerVblank)*previous.timer0Period;double elapsed=double(f.lockCycles-previous.lockCycles)/period;
                need(std::llround(elapsed)==1,"Irregular GBA mixing schedule is not supported by this importer");
                need(predictGbaMixPhase(previous,expected)&&!expected.ended&&expected.offset==f.sourcePointer-(f.voice.sample.address+16u)&&expected.fraction==f.sourceFraction&&expected.remaining==f.sourceRemaining,"GBA source phase is discontinuous");
            }
            double a=f.gainRight/256.*((f.soundControl&4)?1.:.5),b=f.gainLeft/256.*((f.soundControl&8)?1.:.5);
            double right=((f.soundControl&0x100)?a:0)+((f.soundControl&0x1000)?b:0),left=((f.soundControl&0x200)?a:0)+((f.soundControl&0x2000)?b:0);
            NoteControl c;c.at=(f.lockCycles-first.lockCycles)*ticksPerCycle;c.rate=(increment(f)*Cpu/f.timer0Period)/base;c.left=left/userGain;c.right=right/userGain;
            if(controls->empty()||controls->back().rate!=c.rate||controls->back().left!=c.left||controls->back().right!=c.right)controls->push_back(c);
        }
        result.controlPoints+=controls->size();note.controls=std::move(controls);s.notes.push_back(std::move(note));result.truncatedNotes+=raw.ending==GbaNoteEnding::CaptureBoundary;
    }
    // Quantized final block end may reach beyond the raw capture clock.
    int maxEnd=0;for(auto&n:s.notes)maxEnd=std::max(maxEnd,n.step+n.length);s.bars=std::max(s.bars,(maxEnd+4*s.resolution-1)/(4*s.resolution));s.validate();
    result.originalNotes=capture.recording.notes.size();result.mixerFrames=capture.frames.size();result.sourceTracks=tracks.size();
    result.limitation="Original PCM notes/samples with applied rate and stereo envelopes. Timeline uses emulated mixer-lock times; DAC delay, filtering, reverb and exact original output are not restored. PSG and unknown drivers are rejected.";return result;
}
GbaImportResult importGbaNotes(const std::filesystem::path&path,double seconds,int bpm,std::atomic<bool>&cancel,std::atomic<int>&progress){
    need(!cancel.load(),"Note import cancelled");need(std::isfinite(seconds)&&seconds>=.1&&seconds<=600&&bpm>=30&&bpm<=300&&seconds<=512*240./bpm,"Invalid GBA import duration/BPM");progress=0;
    auto bytes=readMusicFile(path);need(identify(bytes)==Format::Gsf,"GBA note import requires GSF/MINIGSF");auto core=makeBackend(bytes,path);need(core->prepareGbaMixerInspection(),"This GBA driver does not support original note/sample import");core->continuousEnd(true);
    GbaImportCapture capture;GbaNoteRecorder recorder;std::map<uint32_t,size_t>samples;std::string error;uint64_t total=uint64_t(std::llround(seconds*InternalRate)),at=0;size_t capturedBytes=0;AudioBlock audio;
    auto checked=[&](bool ok){if(!ok)throw std::runtime_error(error.empty()?"GBA inspection unavailable":error);};
    while(at<total){
        need(!cancel.load(),"Note import cancelled");size_t n=size_t(std::min<uint64_t>(512,total-at));core->render(n,audio);
        uint32_t psgMask=0;need(core->gbaPsgActivity(psgMask),"GBA PSG hardware-state check unavailable");capture.psgObserved=psgMask!=0;
        need(!capture.psgObserved,"This GBA source contains PSG notes or active PSG sound; importing only PCM would lose voices");
        std::vector<GbaVoiceEvent>events;checked(core->drainGbaEvents(events,error));checked(recorder.append(events,error));
        for(auto&e:events)if(e.kind==GbaVoiceEventKind::NoteOn){auto found=samples.find(e.sample.address);if(found==samples.end()){need(samples.size()<256,"GBA bank exceeds256 samples");need(e.sample.length<=8u*1024u*1024u-capturedBytes,"GBA bank exceeds8 MiB");GbaSampleData sample;checked(core->copyGbaSample(e.sample,sample,error));capturedBytes+=sample.pcm.size();samples[e.sample.address]=capture.samples.size();capture.samples.push_back(std::move(sample));}else need(sameSample(e.sample,capture.samples[found->second].info),"GBA sample changed during capture");}
        std::vector<GbaMixFrame>frames;checked(core->drainGbaMixFrames(frames,error));need(capture.frames.size()+frames.size()<=524288,"GBA mixer capture limit exceeded");capture.frames.insert(capture.frames.end(),frames.begin(),frames.end());at+=n;progress=int(at*80/total);
    }
    uint64_t boundary=0;need(core->gbaEmulatedCycles(boundary),"GBA emulated boundary unavailable");checked(recorder.finish(boundary,capture.recording,error));progress=85;
    auto result=projectGbaNotes(capture,bpm,cancel);auto title=path.stem().u8string();if(title.size()<=120&&title.find_first_of("\r\n")==std::string::npos)result.song.title=title;need(!cancel.load(),"Note import cancelled");progress=100;return result;
}
}
