#pragma once
#include "Song.hpp"
#include "core/Models.hpp"
#include <atomic>
#include <filesystem>
namespace ngmv::editor {
// Renderer for the SAME Song edited/serialized by the composer. This is a
// known-rate PCM sampler, not an m4a engine or an audio-to-note estimator.
class SampleSongRenderer {
    Song song_;
    struct Segment {uint64_t frame;double position,increment,left,right;};
    struct Scheduled {uint64_t begin,end;size_t note;std::vector<Segment>segments;};
    std::vector<Scheduled>schedule_;
    std::vector<size_t>active_;
    size_t cursor_=0;
    uint64_t position_=0,total_=0;
public:
    explicit SampleSongRenderer(const Song&);
    uint64_t frames()const{return total_;}
    uint64_t position()const{return position_;}
    void seek(uint64_t frame);
    void render(size_t frames,std::vector<Stereo>&out,uint64_t muteMask=0,
                std::vector<std::vector<Stereo>>*voices=nullptr);
};
void exportSampleSongWav(const Song&,const std::filesystem::path&,
                         std::atomic<bool>&cancel,std::atomic<int>&progress);
}
