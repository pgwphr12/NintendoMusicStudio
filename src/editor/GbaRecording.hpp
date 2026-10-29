#pragma once
#include "core/GbaInspection.hpp"
#include <array>
#include <optional>
namespace ngmv::editor {
// Read-only import intermediate, NOT a second editor/project model. Original
// cycles and raw values are retained until conversion into the shared Song.
enum class GbaNoteEnding { Open, VoiceEnd, Stolen, CaptureBoundary };
struct GbaRecordedNote {
    GbaVoiceEvent start;
    std::optional<GbaVoiceEvent> gateOff,endEvent;
    std::vector<GbaVoiceEvent> controls;
    uint64_t endCycles=0;
    GbaNoteEnding ending=GbaNoteEnding::Open;
};
struct GbaRecording {
    std::vector<GbaRecordedNote> notes;
    uint64_t endCycles=0;
    size_t events=0,controlMetadataChanges=0;
};
class GbaNoteRecorder {
    GbaRecording recording_;
    std::array<int,12>active_;
    std::array<uint64_t,12>generation_{};
    uint64_t lastCycles_=0;
    bool sealed_=false;
    std::string fault_;
    bool fail(const char*,std::string&);
public:
    static constexpr size_t MaxEvents=262144,MaxNotes=32768;
    GbaNoteRecorder(){active_.fill(-1);}
    // Any malformed/truncated stream is a terminal fault, never a partial
    // successful recording. Caller must restart inspection with a new recorder.
    bool append(const std::vector<GbaVoiceEvent>&,std::string&error);
    // Boundary is an observed emulation clock, not the audio playback cursor.
    // Open voices are marked truncated; no release/tail is invented.
    bool finish(uint64_t boundary,GbaRecording&out,std::string&error);
};
}
