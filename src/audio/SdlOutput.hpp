#pragma once
#include "PitchTempo.hpp"
#include "PresentationClock.hpp"
#include <SDL.h>
#include <atomic>
#include <cmath>
#include <string>
namespace ngmv {
class SdlOutput {
    StereoRing<> &ring_;
    SDL_AudioDeviceID device_ = 0;
    PitchTempo resampler_;
    std::atomic<uint64_t> consumed_{0}, underruns_{0};
    std::atomic<float> master_{1.f}, speed_{1.f}, pitch_{1.f};
    std::atomic<bool> finished_{false}, drained_{false};
    int rate_ = 96000;
    bool paused_ = true;
    PresentationClock visualClock_;double frozenVisual_=0;
    static void callback(void *self, Uint8 *out, int bytes);
    SDL_AudioDeviceID create(int rate);

  public:
    explicit SdlOutput(StereoRing<> &ring);
    ~SdlOutput();
    void pause(bool pause);
    void releaseDevice();
    void changeRate(int rate);
    void reset();
    int rate() const {
        return rate_;
    }
    double visualConsumed()const{return paused_?frozenVisual_:visualClock_.position(PresentationClock::now());}
    uint64_t consumed() const {
        return consumed_.load();
    }
    uint64_t underruns() const {
        return underruns_.load();
    }
    float master() const {
        return master_.load();
    }
    float speed() const {
        return speed_.load();
    }
    float pitch() const { return pitch_.load(); }
    void pitch(float value) { if(std::isfinite(value))pitch_=std::clamp(value,.5f,2.f); }
    void finished() { finished_=true; }
    bool drained() const { return drained_.load(); }
    void speed(float value) {
        if(std::isfinite(value))speed_ = std::clamp(value, .5f, 2.f);
    }
    void master(float value) {
        master_ = std::clamp(value, 0.f, 1.f);
    }
};
} // namespace ngmv
