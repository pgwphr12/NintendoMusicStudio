#pragma once
#include "Spectrum.hpp"
#include <cstdint>
namespace ngmv {
inline float spectrumCurve(const SpectrumAnalyzer::Levels& v,float fraction){
    const float position=std::clamp(fraction,0.f,1.f)*(v.size()-1);
    const int i=int(position);const float t=position-i;
    auto at=[&](int index){return v[size_t(std::clamp(index,0,int(v.size()-1)))];};
    const float p=at(i-1),a=at(i),b=at(i+1),n=at(i+2);
    const float y=.5f*((2*a)+(-p+b)*t+(2*p-5*a+4*b-n)*t*t+(-p+3*a-3*b+n)*t*t*t);
    return std::clamp(y,std::min(a,b),std::max(a,b));
}
}
