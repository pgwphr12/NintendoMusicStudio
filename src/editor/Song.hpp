#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>
#include <memory>
namespace ngmv::editor {
struct Instrument { int duty=2, volume=10; bool mute=false; int sample=-1,pan=0; std::string name; };
struct SampleInstrument {
    std::string name;
    int rootKey=60;
    double sampleRate=0; // Explicit Hz supplied by importer; never a raw GBA frequency.
    bool loop=false;
    uint32_t loopStart=0;
    std::shared_ptr<const std::vector<int8_t>> pcm;
};
// Piecewise-constant multipliers at relative musical ticks. Rate changes
// accumulate source phase; they do not retrigger the sample.
struct NoteControl {double at=0,rate=1,left=1,right=1;};
struct Note {
    int channel=0,step=0,length=1,pitch=60,velocity=15,duty=-1,sample=-1;
    double sourceOffset=0; // Source samples including fractional position.
    std::shared_ptr<const std::vector<NoteControl>>controls;
};
struct Song {
    int bpm=120,bars=4,resolution=4; bool pal=false;
    int ticks()const{return bars*4*resolution;}
    std::string title="New song";
    bool sampled=false; // Whole song profile; NES and sampled tracks are not silently mixed.
    std::vector<Instrument> instruments=std::vector<Instrument>(4);
    std::vector<SampleInstrument> samples;
    std::vector<Note> notes;
    double seconds() const {return bars*4*60.0/bpm;}
    void validate() const;
    void put(Note note);
    std::string serialize() const;
    static Song parse(const std::string&);
    static Song demo();
};
std::vector<uint8_t> compileNsf(const Song&);
}
