#pragma once
#include <cstdint>
#include <vector>
#include <string>
namespace ngmv {
// Original m4a/MP2K metadata, not notes inferred from mixed PCM. Slot numbers
// represent temporary polyphony voices; sourceTrack is a separate identity.
enum class GbaVoiceEventKind { NoteOn, GateOff, VoiceEnd, Pitch, Volume, MixerState };
struct GbaSampleInfo {
    uint32_t address=0,rawFrequency=0,loopStart=0,length=0;
    uint16_t type=0,status=0;
    bool readable=false,pcm8=false,looped=false;
};
// Owned signed PCM8 bytes copied only from a verified ROM WaveData range.
// Frequencies remain raw engine values; no guessed Hz/root-pitch conversion.
struct GbaSampleData {
    GbaSampleInfo info;
    std::vector<int8_t> pcm;
};
struct GbaVoiceEvent {
    GbaVoiceEventKind kind=GbaVoiceEventKind::NoteOn;
    uint64_t cpuCycles=0,generation=0;
    uint32_t sourceTrack=0,frequency=0;
    int slot=0,midiKey=0,key=0,velocity=0,gateTicks=0;
    int rightVolume=0,leftVolume=0,envelope=0;
    uint8_t type=0,status=0,attack=0,decay=0,sustain=0,release=0;
    GbaSampleInfo sample;
};
// State at the final per-side-gain write, immediately before this voice is
// mixed. Distinct from sequencer/setup writes and from audible output time.
struct GbaMixFrame {
    GbaVoiceEvent voice;
    uint64_t lockEpoch=0; // Observed SoundInfo lock transition, not an audible frame counter.
    uint64_t lockCycles=0; // Shared execution timestamp, before per-voice mixer work.
    uint32_t samplesPerVblank=0,pcmRate=0,divFreq=0;
    uint32_t sourceRemaining=0,sourceFraction=0,sourcePointer=0;
    uint32_t timer0Period=0;
    uint16_t soundControl=0;
    uint8_t gainRight=0,gainLeft=0,masterVolume=0,reverb=0;
};
}
