// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <algorithm>
#include <cmath>
#include <vector>
namespace ngmv {
// Display-only tail. Never advances the decoder, audible clock or audio ring.
class WaveformTail {
 std::vector<std::vector<float>> values_;
 double fractional_=0;size_t remaining_=0;
public:
 void clear(){values_.clear();fractional_=0;remaining_=0;}
 void replace(std::vector<std::vector<float>> values){values_=std::move(values);fractional_=0;remaining_=0;for(const auto&v:values_)remaining_=std::max(remaining_,v.size());}
 void drain(double seconds,unsigned rate){
  if(!remaining_||!std::isfinite(seconds)||seconds<=0||!rate)return;
  double distance=seconds*rate+fractional_;
  size_t count=size_t(std::min(double(remaining_),std::floor(distance)));fractional_=distance-std::floor(distance);
  for(auto&v:values_){size_t n=std::min(count,v.size());std::move(v.begin()+n,v.end(),v.begin());std::fill(v.end()-n,v.end(),0.f);}
  remaining_-=count;
 }
 bool active()const{return remaining_!=0;}
 float peak()const{float result=0;for(const auto&v:values_)for(float sample:v)result=std::max(result,std::abs(sample));return result;}
 const std::vector<std::vector<float>>&values()const{return values_;}
};
}
