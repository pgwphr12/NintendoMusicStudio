#pragma once
#include "core/IMusicBackend.hpp"
#include <atomic>
#include <filesystem>
#include <functional>
namespace ngmv {
struct StemExportOptions {
    int track = 0;
    unsigned plays = 1;
    uint64_t frames = 0;
    bool mix = false;
};
struct StemExportResult { unsigned written = 0, silent = 0; };
// Materialize recorded VGM loop commands for offline export, preserving chip
// state across iterations. Reject unsupported loop structures rather than
// silently restarting the song. Returned frame count includes all repeats.
std::vector<uint8_t> expandVgmLoops(const std::vector<uint8_t>& input,unsigned repeats,uint64_t& frames);
// Writes into an empty, caller-owned staging directory; caller publishes it only
// on success. A zero frame count is rejected instead of inventing an end time.
StemExportResult exportStems(IMusicBackend& backend, const std::filesystem::path& staging,
                             const StemExportOptions& options, const std::atomic<bool>& cancel,
                             const std::function<void(unsigned)>& progress = {});
}
