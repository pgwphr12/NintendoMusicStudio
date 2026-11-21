#pragma once
#include <cstdint>
#include <vector>
#include <memory>
#include "Envelope.hpp"
namespace ngmv {
// Slowly adapt one uniform display multiplier; never filter, recenter or
// normalize individual points. Reference: 0.1625 input amplitude at 4x = 0.65.
class CorrscopeDisplayScale {
    float gain_=4.f,level_=0;
    double hold_=0,warmup_=0;
    bool ready_=false;
public:
    float update(float amplitude,float peak,double seconds);
};
// Native translation of corrscope 0.11.1's default CorrelationTrigger.
// See third_party/corrscope for the original source and BSD-2-Clause license.
class CorrscopeTrigger {
    size_t size_;
    int rate_;
    double fps_;
    std::vector<float> buffer_,slope_,window_;
    int previousPeriod_=-1,framesSinceSpectrum_=0;
    int64_t previousTrigger_=0;
public:
    struct Result {int64_t sample=0;int period=0;double mean=0;};
    explicit CorrscopeTrigger(size_t size=1920,int rate=48000,double fps=60);
    Result process(const std::vector<float>& samples,int64_t base,int64_t position);
};
// Latest-only bounded queue; the UI and audio threads never run the FFT work.
class CorrscopeWorker {
    struct State;
    std::unique_ptr<State> state_;
public:
    CorrscopeWorker();
    ~CorrscopeWorker();
    CorrscopeWorker(const CorrscopeWorker&)=delete;
    CorrscopeWorker& operator=(const CorrscopeWorker&)=delete;
    void reset();
    uint64_t completed();
    void submit(const std::vector<std::vector<float>>&,const std::vector<size_t>& windows,uint64_t mask,double seconds);
    std::vector<std::vector<EnvelopePoint>> result();
};
}
