#include "SampleBackend.hpp"
#include "SampleSong.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace ngmv::editor {
namespace {
class Backend final:public IMusicBackend {
    SampleSongRenderer renderer_;
    std::vector<TrackInfo> tracks_;
    std::vector<ChannelInfo> channels_;
    uint32_t mask_=0;
public:
    explicit Backend(const Song&s):renderer_(s){
        tracks_.push_back({s.title,"","","Sampled project","Edited dry sample instruments",int(std::llround(s.seconds()*1000))});
        for(size_t i=0;i<s.instruments.size();i++)channels_.push_back({s.instruments[i].name.empty()?"Track "+std::to_string(i+1):s.instruments[i].name});
    }
    const std::vector<TrackInfo>&tracks()const override{return tracks_;}
    const std::vector<ChannelInfo>&channels()const override{return channels_;}
    void selectTrack(int index)override{if(index!=0)throw std::runtime_error("Invalid sampled project track");renderer_.seek(0);}
    void seek(int ms)override{renderer_.seek(uint64_t(std::max(0,ms))*InternalRate/1000);}
    void muteMask(uint32_t mask)override{mask_=mask;}
    void render(size_t n,AudioBlock&out)override{renderer_.render(n,out.mix,mask_,&out.voices);}
};
}
std::unique_ptr<IMusicBackend>makeSampleBackend(const Song&s){return std::make_unique<Backend>(s);}
}
