#pragma once
#include "core/IMusicBackend.hpp"
namespace ngmv {
struct GbaRomSong { unsigned id, sequences; };
enum class GbaRomDriver { M4a, Nintendo };
struct GbaRomMusic {
    uint32_t crc = 0, init = 0, start = 0, main = 0, vsync = 0, update = 0;
    GbaRomDriver driver = GbaRomDriver::M4a;
    std::string title, code;
    std::vector<GbaRomSong> songs;
};
// Verified ROM revisions only; no driver/code/sample bank is bundled.
GbaRomMusic inspectGbaRom(const std::vector<uint8_t>&);
std::vector<uint8_t> gbaMusicImage(const std::vector<uint8_t>&, const GbaRomMusic&, unsigned song);
std::unique_ptr<IMusicBackend> makeGbaRomBackend(const std::vector<uint8_t>&);
}
