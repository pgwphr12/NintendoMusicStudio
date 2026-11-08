#pragma once
#include "core/Models.hpp"
#include <array>
#include <memory>
namespace ngmv {
// MesenCE-compatible EQ stage; provenance in third_party/mesence-eq/UPSTREAM.json.
class GraphicEqualizer {
 struct State;std::unique_ptr<State> state_;bool active_=false;
public:
 inline static constexpr std::array<int,20> frequencies={40,56,80,113,160,225,320,450,600,750,1000,2000,3000,4000,5000,6000,7000,10000,12500,15000};
 // Preserve MesenCE's actual20th filter center despite its15kHz UI label.
 inline static constexpr std::array<int,20> filterFrequencies={40,56,80,113,160,225,320,450,600,750,1000,2000,3000,4000,5000,6000,7000,10000,12500,13000};
 GraphicEqualizer();~GraphicEqualizer();
 void reset();void beginBlock(bool enabled,const std::array<float,20>& db);Stereo process(Stereo);
};
}
