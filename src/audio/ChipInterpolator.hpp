// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "core/Models.hpp"
#include <array>
#include <algorithm>
#include <cmath>
namespace ngmv {
// Causal, fixed-work band-limited interpolation. No future chip ticks/register
// writes: a16-native-sample delay supplies the symmetric32-tap kernel.
class ChipInterpolator {
    std::array<Stereo,32> history_{};
    unsigned head_=0;
    static const std::array<std::array<float,32>,257>& kernels() {
        static const auto table=[] {
            std::array<std::array<float,32>,257> result{};
            constexpr double pi=3.14159265358979323846;
            for(unsigned p=0;p<=256;++p){
                double sum=0,phase=double(p)/256;
                for(unsigned t=0;t<32;++t){
                    double x=double(t)+phase-16;
                    double sinc=std::abs(x)<1e-12?.94:std::sin(pi*.94*x)/(pi*x);
                    double window=.42+.5*std::cos(pi*x/16)+.08*std::cos(2*pi*x/16);
                    sum+=result[p][t]=float(sinc*window);
                }
                for(auto&w:result[p])w=float(w/sum);
            }
            return result;
        }();
        return table;
    }
  public:
    void push(Stereo v){head_=(head_+31)&31;history_[head_]=v;}
    Stereo at(double phase)const {
        phase=std::clamp(phase,0.,1.);double index=phase*256;
        unsigned p=std::min(255u,unsigned(index));float blend=float(index-p);
        const auto&k=kernels();Stereo out{};
        for(unsigned t=0;t<32;++t){
            float w=k[p][t]+(k[p+1][t]-k[p][t])*blend;
            const auto&v=history_[(head_+t)&31];out.l+=v.l*w;out.r+=v.r*w;
        }
        return out;
    }
};
}
