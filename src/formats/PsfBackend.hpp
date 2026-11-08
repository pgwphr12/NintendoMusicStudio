#pragma once
#include "core/IMusicBackend.hpp"
namespace ngmv {
std::unique_ptr<IMusicBackend> makePsfBackend(const std::filesystem::path &, Format);
std::unique_ptr<IMusicBackend> makeGbaImageBackend(std::vector<uint8_t>,const std::string&);
}
