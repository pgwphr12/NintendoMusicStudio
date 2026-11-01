#pragma once
#include <atomic>
#include <algorithm>
#include <chrono>
#include <cstdint>
namespace ngmv {
// Display-only interpolation within the most recently submitted device block.
// Never advances beyond actual consumed source frames, and never alters audio.
class PresentationClock {
    std::atomic<uint64_t> sequence_{0},begin_{0},end_{0},tick_{0},duration_{1};
public:
    static uint64_t now(){return uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());}
    void publish(uint64_t begin,uint64_t end,uint64_t tick,uint64_t duration){sequence_.fetch_add(1);begin_=begin;end_=end;tick_=tick;duration_=std::max(uint64_t(1),duration);sequence_.fetch_add(1);}
    double position(uint64_t tick)const{for(int i=0;i<8;i++){auto serial=sequence_.load();if(serial&1)continue;auto a=begin_.load(),b=end_.load(),start=tick_.load(),span=duration_.load();if(sequence_.load()!=serial)continue;double f=tick>start?std::min(1.,double(tick-start)/span):0.;return a+double(b-a)*f;}return double(end_.load());}
};
}
