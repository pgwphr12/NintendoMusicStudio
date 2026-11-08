#pragma once
#include <array>
#include <atomic>
#include <algorithm>
#include <cmath>
#include <memory>
#include <cstdint>
namespace ngmv {
// One audio-callback writer, one UI reader. Atomic sample slots avoid a data
// race even if a reader is overtaken; the audio thread never waits or allocates.
class AudioTap {
    static constexpr size_t Capacity=16384;
    struct Sample {std::atomic<float> l{0},r{0};};
    std::unique_ptr<Sample[]> data_{new Sample[Capacity]};
    std::atomic<uint64_t> end_{0};
    uint64_t written_=0;
public:
    static_assert(std::atomic<float>::is_always_lock_free,"Audio samples must be lock free");
    void push(const float* stereo,size_t frames){
        for(size_t i=0;i<frames;++i){
            auto& s=data_[(written_+i)&(Capacity-1)];
            const float l=stereo[i*2],r=stereo[i*2+1];
            s.l.store(std::isfinite(l)?l:0,std::memory_order_relaxed);
            s.r.store(std::isfinite(r)?r:0,std::memory_order_relaxed);
        }
        written_+=frames;end_.store(written_,std::memory_order_release);
    }
    // Reset only while the device is paused (the same rule as resampler reset).
    void reset(){written_=0;end_.store(0,std::memory_order_release);}
    template<size_t N> uint64_t snapshot(std::array<float,N*2>& out)const{
        static_assert(N<=Capacity/2);
        for(int attempt=0;attempt<2;++attempt){
            auto end=end_.load(std::memory_order_acquire);
            size_t count=size_t(std::min<uint64_t>(end,N));
            std::fill(out.begin(),out.end(),0.f);
            for(size_t i=0;i<count;++i){
                const auto& s=data_[(end-count+i)&(Capacity-1)];
                out[(N-count+i)*2]=s.l.load(std::memory_order_relaxed);
                out[(N-count+i)*2+1]=s.r.load(std::memory_order_relaxed);
            }
            if(end_.load(std::memory_order_acquire)-end<Capacity-N-2048)return end;
        }
        out.fill(0);return 0; // Skip an overtaken frame, never block the callback.
    }
};
}
