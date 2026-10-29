#pragma once
#include "core/IMusicBackend.hpp"
#include <atomic>
namespace ngmv::editor {
// Edit decisions reference a local source; no game data is embedded in the project.
struct SourceEdit {
    std::filesystem::path source;
    int track=0;
    double begin=0,end=30,cutBegin=0,cutEnd=0,fadeIn=0,fadeOut=0;
    std::vector<float> gains;
    void validate() const;
    double duration() const;
    std::string serialize() const;
    static SourceEdit parse(const std::string&);
};
std::unique_ptr<IMusicBackend> makeEditedSource(const SourceEdit&,std::atomic<bool>*cancel=nullptr);
void saveSourceEdit(const SourceEdit&,const std::filesystem::path&);
void exportSourceWav(const SourceEdit&,const std::filesystem::path&,
                     std::atomic<bool>&cancel,std::atomic<int>&progress);
}
