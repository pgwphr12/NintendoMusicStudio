#include "GraphicEqualizer.hpp"
#include <algorithm>
#include <cmath>
#include "mesence-eq/orfanidis_eq.h"
namespace ngmv {
struct GraphicEqualizer::State {
 orfanidis_eq::freq_grid grid;
 std::unique_ptr<orfanidis_eq::eq1> left,right;
 std::array<int,20> gains{};
 explicit State(const std::array<int,20>& values):gains(values){
  for(size_t i=0;i<20;i++){
   int center=filterFrequencies[i];
   int previous=i?filterFrequencies[i-1]:center-(filterFrequencies[1]-center);
   int next=i<19?filterFrequencies[i+1]:center+(center-filterFrequencies[18]);
   grid.add_band((center+previous)/2.,center,(next+center)/2.);
  }
  left=std::make_unique<orfanidis_eq::eq1>(&grid,orfanidis_eq::butterworth);
  right=std::make_unique<orfanidis_eq::eq1>(&grid,orfanidis_eq::butterworth);
  left->set_sample_rate(InternalRate);right->set_sample_rate(InternalRate);
  for(size_t i=0;i<20;i++){left->change_band_gain_db(unsigned(i),values[i]);right->change_band_gain_db(unsigned(i),values[i]);}
 }
};
GraphicEqualizer::GraphicEqualizer()=default;
GraphicEqualizer::~GraphicEqualizer()=default;
void GraphicEqualizer::reset(){state_.reset();active_=false;}
void GraphicEqualizer::beginBlock(bool enabled,const std::array<float,20>& db){
 active_=enabled;if(!enabled)return;
 std::array<int,20> values{};
 for(size_t i=0;i<20;i++)values[i]=std::isfinite(db[i])?int(std::lround(std::clamp(db[i],-20.f,20.f))):0;
 if(!state_||state_->gains!=values)state_=std::make_unique<State>(values);
}
Stereo GraphicEqualizer::process(Stereo s){
 if(!active_||!state_)return s;
 auto pcm=[](double x){return int16_t(std::clamp(x,-32768.,32767.));};
 double inL=pcm(double(s.l)*32768.),inR=pcm(double(s.r)*32768.),l=0,r=0;
 state_->left->sbs_process(&inL,&l);state_->right->sbs_process(&inR,&r);
 return {pcm(l)/32768.f,pcm(r)/32768.f};
}
}
