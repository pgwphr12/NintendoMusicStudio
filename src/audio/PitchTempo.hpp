#pragma once
#include "StereoRing.hpp"
#include <memory>
namespace ngmv {
// Separate source-time rate and pitch factor. Only the device callback touches
// DSP state; controls are atomics in SdlOutput. Unity takes the original path.
class PitchTempo {
    struct State;
    std::unique_ptr<State> state_;

  public:
    uint64_t consumed = 0, underruns = 0;
    PitchTempo();
    ~PitchTempo();
    void rate(int rate);
    void reset();
    bool drained(const StereoRing<> &source, bool finished) const;
    void pull(StereoRing<> &source, float *output, size_t frames, float master, double speed,
              double pitch, bool finished);
};
} // namespace ngmv
