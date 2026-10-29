#pragma once
#include "Song.hpp"
#include "core/Models.hpp"
#include <atomic>
#include <filesystem>
#include <optional>
namespace ngmv::editor {
struct RipResult {
    std::filesystem::path directory;
    std::vector<ChannelInfo> channels;
    std::optional<Song> editable;
    int samples=0,notes=0;double seconds=0;
    bool baseNesState=false,expansion=false;
};
RipResult ripGameMusic(const std::filesystem::path&file,int track,int seconds,int bpm,
                      const std::filesystem::path&output,std::atomic<bool>&cancel,
                      std::atomic<int>&progress);
}
