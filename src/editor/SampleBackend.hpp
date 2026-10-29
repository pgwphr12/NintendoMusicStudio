#pragma once
#include "Song.hpp"
#include "core/IMusicBackend.hpp"
namespace ngmv::editor {
// Same immutable Song snapshot and sampler as edited WAV, no NSF conversion.
std::unique_ptr<IMusicBackend> makeSampleBackend(const Song&);
}
