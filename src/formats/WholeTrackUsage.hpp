// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "core/IMusicBackend.hpp"
namespace ngmv {
// Uses known whole-track metadata or a bounded offline unmuted pass. Unknown
// means unproven, never an observed prefix disguised as a whole-song result.
std::unique_ptr<IMusicBackend> withWholeTrackUsage(std::unique_ptr<IMusicBackend>,int durationMs=-1);
}
