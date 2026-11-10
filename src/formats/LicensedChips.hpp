// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "core/Models.hpp"
#include <memory>
#include <array>
namespace ngmv {
enum class ChipKind {Ym2612,Opl2,Oki,Scc,Es5506,Ay8930,Opl1,Sn76489};
class LicensedChip {
    struct State;std::unique_ptr<State> state_;
  public:
    LicensedChip(ChipKind,uint32_t clock,uint8_t flags,uint16_t noiseTaps=9,uint8_t noiseWidth=16);
    ~LicensedChip();
    void write(uint8_t port,uint8_t reg,uint8_t value);
    void rom(uint32_t size,uint32_t offset,const uint8_t*data,size_t length);
    void mute(uint32_t mask);
    Stereo sample();
    // Last output-frame stems from one ES core:32 voices + explicit bus residual.
    const std::array<Stereo,33>& esVoices() const;
    void furnaceEsRate(unsigned voices); // Verified export signature only; before first sample.
    void captureEsVoices(); // Before first sample; the main mix only needs residual.
    uint64_t programmedEsVoices(); // Independent register-trace instance only.
    uint64_t programmedAyVoices();
};
}
