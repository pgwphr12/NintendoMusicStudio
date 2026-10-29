#pragma once
#include "Song.hpp"
#include <filesystem>
#include <atomic>
namespace ngmv::editor {
Song importNesNotes(const std::filesystem::path&,int track,double seconds,int bpm,
                    std::atomic<bool>&cancel,std::atomic<int>&progress);
void exportSongWav(const Song&,const std::filesystem::path&,
                   std::atomic<bool>&cancel,std::atomic<int>&progress);
}
