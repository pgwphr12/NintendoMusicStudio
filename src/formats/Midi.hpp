#pragma once
#include "core/IMusicBackend.hpp"
#include <array>
namespace ngmv {
struct MidiEvent { uint64_t frame=0; uint32_t message=0; };
struct MidiSong {
    std::vector<MidiEvent> events;
    std::vector<std::vector<uint8_t>> sysex;
    uint64_t endFrame=0;
    uint16_t used=0;
    unsigned ignoredSysex=0;
    std::string title;
};
inline constexpr size_t MidiMaxEvents=16000000;
MidiSong parseMidi(const std::vector<uint8_t>&,size_t eventLimit=MidiMaxEvents);
// Automatic file opens try exact sibling DLS then SF2 before VMS/default.
// Explicit bank arguments remain one-off overrides; absent VMS uses Windows GM.
void midiSoundFont(const std::filesystem::path&);
std::filesystem::path midiSoundFont();
std::filesystem::path windowsMidiBank();
bool virtualMidiInstalled();
std::vector<std::filesystem::path> virtualMidiFonts(const std::filesystem::path& config);
std::filesystem::path automaticMidiBank();
std::vector<std::filesystem::path> localMidiBankCandidates(const std::filesystem::path& midi);
std::vector<std::filesystem::path> virtualMidiFontsToImport();
void validateMidiBank(const std::filesystem::path&);
std::unique_ptr<IMusicBackend> makeMidiBackend(const std::vector<uint8_t>&,
                                             const std::filesystem::path& file,
                                             const std::filesystem::path& bank={});
}
