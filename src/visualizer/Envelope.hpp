#pragma once
#include <vector>
#include <array>
namespace ngmv {
struct EnvelopePoint {
    float minimum = 0, maximum = 0, center = 0;
};
struct EnvelopeScale {
    float center = 0, amplitude = 0;
    double offset = 0;
    bool triggered = false;
};
std::vector<EnvelopePoint> envelope(const std::vector<float> &samples, size_t columns,
                                    size_t window = 2048, EnvelopeScale *scale = nullptr, double offset = -1);
// Match the previous real shape, not an edge/center zero crossing. Noise and
// changed notes immediately fall back to current audio; no invented motion.
class WaveformTracker {
    std::array<float, 96> reference_{};
    std::array<double, 96> positions_{};
    size_t window_ = 0;
    double anchor_ = 0;
    float anchorLevel_ = 0, anchorSlope_ = 0;
    std::vector<float> lastInput_;
    double lastAt_=0;
public:
    double locate(const std::vector<float>&, size_t window);
};
// Display-only time-based easing for changed notes and gain. Independent of
// sample rate and refresh rate; never runs on the audio thread.
class WaveformTransition {
    std::vector<EnvelopePoint> previous_;
public:
    void apply(std::vector<EnvelopePoint>&,double elapsed,bool playing);
};
} // namespace ngmv
