// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "core/Models.hpp"
#include <memory>
namespace ngmv {
enum class ChipKind {Ym2612,Opl2,Oki,Scc,Es5506};
class LicensedChip {
    struct State;std::unique_ptr<State> state_;
  public:
    LicensedChip(ChipKind,uint32_t clock,uint8_t flags);
    ~LicensedChip();
    void write(uint8_t port,uint8_t reg,uint8_t value);
    void rom(uint32_t size,uint32_t offset,const uint8_t*data,size_t length);
    void mute(uint32_t mask);
    Stereo sample();
};
}
