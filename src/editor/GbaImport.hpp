#pragma once
#include "Song.hpp"
#include "GbaRecording.hpp"
#include <atomic>
#include <filesystem>
namespace ngmv::editor {
struct GbaImportCapture {
    GbaRecording recording;
    std::vector<GbaMixFrame>frames;
    std::vector<GbaSampleData>samples;
    bool psgObserved=false;
};
struct GbaImportResult {
    Song song; // The existing composer model, never a separate GBA project.
    size_t originalNotes=0,mixerFrames=0,controlPoints=0,sourceTracks=0,sampleBytes=0,truncatedNotes=0;
    std::string limitation;
};
GbaImportResult projectGbaNotes(const GbaImportCapture&,int bpm,std::atomic<bool>&cancel);
GbaImportResult importGbaNotes(const std::filesystem::path&,double seconds,int bpm,
                               std::atomic<bool>&cancel,std::atomic<int>&progress);
}
