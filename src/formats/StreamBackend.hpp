#pragma once
#include "core/IMusicBackend.hpp"
#include <string>
namespace ngmv {
std::unique_ptr<IMusicBackend> makeStreamBackend(const std::filesystem::path &, const std::string &system = "Nintendo 3DS · Audio Stream", bool groupStereoTracks = false, bool splitTwoChannelStereo = false);
}
