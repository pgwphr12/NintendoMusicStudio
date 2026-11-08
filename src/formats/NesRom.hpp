#pragma once
#include "core/IMusicBackend.hpp"
namespace ngmv {
struct NesRomMusic {
    uint32_t programCrc=0;
    uint16_t engine=0;
    bool pal=false;
    std::vector<uint8_t> program;
};
NesRomMusic inspectNesRom(const std::vector<uint8_t>&);
std::vector<uint8_t> nesMusicImage(const NesRomMusic&);
std::unique_ptr<IMusicBackend> makeNesRomBackend(const std::vector<uint8_t>&);
}
