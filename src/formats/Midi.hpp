#pragma once
#include "core/IMusicBackend.hpp"
#include <array>
namespace ngmv {
struct MidiEvent { uint64_t frame=0; uint32_t message=0; };
struct MidiSong {
    std::vector<MidiEvent> events;
    uint64_t endFrame=0;
    uint16_t used=0;
    unsigned ignoredSysex=0;
    std::string title;
};
MidiSong parseMidi(const std::vector<uint8_t>&);
// Empty selection resolves Windows DirectMusic GMFilePath on each new load.
void midiSoundFont(const std::filesystem::path&);
std::filesystem::path midiSoundFont();
std::filesystem::path windowsMidiBank();
void validateMidiBank(const std::filesystem::path&);
std::unique_ptr<IMusicBackend> makeMidiBackend(const std::vector<uint8_t>&,
                                             const std::filesystem::path& file,
                                             const std::filesystem::path& bank={});
}
