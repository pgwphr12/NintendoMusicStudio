#include "Midi.hpp"
#include "MidiDevice.hpp"
#include <windows.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <map>
#include <mutex>
#include <stdexcept>
#define TSF_IMPLEMENTATION
#define TSF_NO_STDIO
#ifdef _MSC_VER
#pragma warning(push,0)
#endif
#include "tinysoundfont/tsf.h"
#ifdef _MSC_VER
#pragma warning(pop)
#endif
namespace ngmv {
namespace {
std::mutex fontMutex;std::filesystem::path selectedFont;
using Font=std::unique_ptr<tsf,decltype(&tsf_close)>;
#include "MidiBank.inl"
struct LoadedFont {Font font{nullptr,tsf_close};unsigned unsupported=0;};
LoadedFont loadFont(const std::filesystem::path&path){
    std::ifstream f(path,std::ios::binary|std::ios::ate);if(!f)throw std::runtime_error("Cannot open MIDI sound bank. Choose an SF2/DLS bank with F8.");auto n=f.tellg();if(n<12||n>256*1024*1024)throw std::runtime_error("Sound bank is empty or exceeds256MB.");
    std::vector<uint8_t>b(static_cast<size_t>(n));f.seekg(0);if(!f.read(reinterpret_cast<char*>(b.data()),n))throw std::runtime_error("Incomplete sound bank read.");
    unsigned count=0;auto roots=bankChunks(b,0,b.size(),0,count);if(roots.size()!=1||roots[0].id!="RIFF")throw std::runtime_error("Not an SF2/DLS RIFF bank.");LoadedFont result;
    if(roots[0].type=="DLS ")result.font.reset(loadDls(b,roots[0],result.unsupported));
    else if(roots[0].type=="sfbk"){
        auto samples=validateSf2(b,roots[0]);result.font.reset(tsf_load_memory(b.data(),int(b.size())));if(!result.font)throw std::runtime_error("Cannot decode SF2 sound bank.");
        for(int p=0;p<result.font->presetNum;p++)for(int i=0;i<result.font->presets[p].regionNum;i++){auto&r=result.font->presets[p].regions[i];bool loop=r.loop_mode&&r.loop_start<r.loop_end;
            // TinySoundFont treats empty/reversed loop endpoints as no loop.
            // Only an active loop can dereference its endpoints.
            if(r.offset>=r.end||r.end>samples||(loop&&(r.loop_start>=samples||r.loop_end>=samples)))throw std::runtime_error("SF2 region exceeds sample data.");
            if(r.loop_mode&&!loop)++result.unsupported;
        }
    }else throw std::runtime_error("Supported sound banks: PCM SF2 or DLS1 (no SF3/SFZ).");
    return result;
}
class MidiBackend final:public IMusicBackend {
    MidiSong song_;bool deviceMode_=false;std::unique_ptr<MidiDevice> device_;std::vector<TrackInfo> tracks_;std::vector<ChannelInfo> channels_;
    Font font_{nullptr,tsf_close};std::vector<Font> synths_;std::array<int,16> slots_{};
    uint64_t position_=0;size_t next_=0;uint32_t mask_=0;std::vector<float> scratch_;
    void reset(){
        synths_.clear();slots_.fill(-1);if(deviceMode_){position_=0;next_=0;return;}for(unsigned c=0;c<16;c++)if(song_.used&(1<<c)){
            Font f(tsf_copy(font_.get()),tsf_close);if(!f)throw std::bad_alloc();tsf_set_output(f.get(),TSF_STEREO_INTERLEAVED,InternalRate,-6.f);if(!tsf_set_max_voices(f.get(),256))throw std::bad_alloc();
            tsf_channel_set_bank(f.get(),0,c==9?128:0);if(!tsf_channel_set_presetnumber(f.get(),0,0,c==9))tsf_channel_set_presetindex(f.get(),0,0);
            slots_[c]=int(synths_.size());synths_.push_back(std::move(f));
        }position_=0;next_=0;
    }
    void event(uint32_t msg){
        if(msg&0x1000000)return;
        if(msg==0xff){for(unsigned c=0;c<16;c++)if(slots_[c]>=0){auto*f=synths_[slots_[c]].get();tsf_channel_sounds_off_all(f,0);tsf_channel_midi_control(f,0,121,0);tsf_channel_set_bank(f,0,c==9?128:0);tsf_channel_set_presetnumber(f,0,0,c==9);}return;}
        unsigned c=msg&15;int slot=slots_[c];if(slot<0)return;auto*f=synths_[slot].get();unsigned a=(msg>>8)&127,b=(msg>>16)&127;
        switch(msg&0xf0){case 0x80:tsf_channel_note_off(f,0,int(a));break;case 0x90:if(!tsf_channel_note_on(f,0,int(a),b/127.f))throw std::bad_alloc();break;
            case 0xb0:if(!tsf_channel_midi_control(f,0,int(a),int(b)))throw std::bad_alloc();break;
            case 0xc0:if(!tsf_channel_set_presetnumber(f,0,int(a),c==9)){
                // XG/GS bank-select can name a bank absent from a GM/SF2 bank.
                // Try its GM program, then the bank's first available preset.
                tsf_channel_set_bank(f,0,c==9?128:0);
                if(!tsf_channel_set_presetnumber(f,0,int(a),c==9))tsf_channel_set_presetindex(f,0,0);
            }break;
            case 0xe0:tsf_channel_set_pitchwheel(f,0,int(a|(b<<7)));break;
            default:break; // channel/poly aftertouch are not implemented by TinySoundFont.
        }
    }
  public:
    MidiBackend(const std::vector<uint8_t>&b,const std::filesystem::path&file,const std::filesystem::path&bank):song_(parseMidi(b)){
        auto path=bank.empty()?midiSoundFont():bank;bool system=path.empty();deviceMode_=system&&midiDeviceOutput();LoadedFont loaded;
        // Hidden/dummy test sessions must never emit hardware MIDI notes.
        wchar_t dummy[32]{};if(GetEnvironmentVariableW(L"NGMV_TEST_SESSION",nullptr,0)&&GetEnvironmentVariableW(L"SDL_AUDIODRIVER",dummy,32)&&!wcscmp(dummy,L"dummy"))deviceMode_=false;
        if(!deviceMode_){if(system)path=automaticMidiBank();loaded=loadFont(path);font_=std::move(loaded.font);}else device_=std::make_unique<MidiDevice>(song_,windowsMidiPort());
        for(unsigned c=0;c<16;c++)if(song_.used&(1<<c))channels_.push_back({"Channel "+std::to_string(c+1)});
        TrackInfo t;t.title=song_.title.empty()?file.stem().u8string():song_.title;t.system="MIDI";t.lengthMs=int((song_.endFrame*1000+InternalRate-1)/InternalRate);
        t.game=path.filename().u8string();
        t.comment=std::string(system?"Automatic MIDI bank: ":"Sound bank: ")+path.filename().u8string()+" / internal TinySoundFont synthesis; not MIDI Mapper audio. Release tail2s. Aftertouch/vendor SysEx/custom modulators are limited. Missing bank/program falls back to GM program or first preset.";
        if(deviceMode_)t.comment="Windows default MIDI Mapper output (F8: internal synthesis). Device supplies audio. Internal waveforms/reverb/pitch do not describe or process device output; silent waveform lanes. Default changes re-open output at current position. Custom SysEx forwarded.";
        if(song_.ignoredSysex||loaded.unsupported)t.comment+=" Limited SysEx/bank routes: "+std::to_string(song_.ignoredSysex)+"/"+std::to_string(loaded.unsupported);
        tracks_.push_back(std::move(t));reset();
    }
    const std::vector<TrackInfo>&tracks()const override{return tracks_;}
    const std::vector<ChannelInfo>&channels()const override{return channels_;}
    void selectTrack(int i)override{if(i)throw std::runtime_error("Invalid MIDI track.");reset();}
    void muteMask(uint32_t m)override{mask_=m;}
    bool hasDeviceOutput()const override{return deviceMode_;}
    void devicePlayback(uint64_t frame,bool playing,const std::vector<ChannelState>&states,float master)override{if(device_)device_->update(frame,playing,states,master);}
    bool decodedEndFrame(uint64_t&frame)const override{frame=song_.endFrame;return true;}
    void seek(int ms)override{
        if(ms<0||ms>tracks_[0].lengthMs)throw std::runtime_error("MIDI seek outside duration.");reset();uint64_t target=std::min(song_.endFrame,uint64_t(ms)*InternalRate/1000);if(deviceMode_){position_=target;return;}AudioBlock ignored;
        while(position_<target)render(size_t(std::min<uint64_t>(8192,target-position_)),ignored);
    }
    void render(size_t frames,AudioBlock&out)override{
        if(frames>8192)throw std::runtime_error("MIDI render block too large.");out.mix.assign(frames,{});out.voices.resize(channels_.size());for(auto&v:out.voices)v.assign(frames,{});scratch_.resize(frames*2);
        if(deviceMode_){position_+=frames;return;}
        size_t done=0;while(done<frames){
            while(next_<song_.events.size()&&song_.events[next_].frame<=position_)event(song_.events[next_++].message);
            if(position_>=song_.endFrame){position_+=frames-done;break;}
            uint64_t until=std::min<uint64_t>(position_+frames-done,song_.endFrame);if(next_<song_.events.size())until=std::min(until,song_.events[next_].frame);size_t n=size_t(until-position_);if(!n)throw std::runtime_error("MIDI event scheduling made no progress.");
            for(size_t c=0;c<synths_.size();c++){
                tsf_render_float(synths_[c].get(),scratch_.data(),int(n),0);
                for(size_t i=0;i<n;i++){Stereo v{scratch_[i*2],scratch_[i*2+1]};if(!std::isfinite(v.l)||!std::isfinite(v.r))throw std::runtime_error("Nonfinite MIDI synthesis.");out.voices[c][done+i]=v;if(!(mask_&(uint32_t(1)<<c))){out.mix[done+i].l+=v.l;out.mix[done+i].r+=v.r;}}
            }done+=n;position_+=n;
        }
    }
};
}
void midiSoundFont(const std::filesystem::path&p){std::lock_guard<std::mutex>lock(fontMutex);selectedFont=p;}
std::filesystem::path midiSoundFont(){std::lock_guard<std::mutex>lock(fontMutex);return selectedFont;}
std::filesystem::path windowsMidiBank(){
    wchar_t text[32768]{};DWORD bytes=sizeof(text);if(RegGetValueW(HKEY_LOCAL_MACHINE,L"SOFTWARE\\Microsoft\\DirectMusic",L"GMFilePath",RRF_RT_REG_SZ|RRF_RT_REG_EXPAND_SZ,nullptr,text,&bytes)==ERROR_SUCCESS&&text[0])return text;
    wchar_t system[MAX_PATH]{};if(!GetSystemDirectoryW(system,MAX_PATH))throw std::runtime_error("Cannot find Windows default MIDI sound bank.");return std::filesystem::path(system)/L"drivers"/L"gm.dls";
}
void validateMidiBank(const std::filesystem::path&p){auto font=loadFont(p.empty()?automaticMidiBank():p);}
std::unique_ptr<IMusicBackend> makeMidiBackend(const std::vector<uint8_t>&b,const std::filesystem::path&file,const std::filesystem::path&bank){return std::make_unique<MidiBackend>(b,file,bank);}
}
