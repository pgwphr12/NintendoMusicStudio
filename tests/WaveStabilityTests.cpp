#include "visualizer/Envelope.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
using namespace ngmv;
void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
int main(){try{
 {float low=10,high=-10;
  for(int frame=0;frame<30;++frame){std::vector<float> samples(4096);for(size_t i=0;i<samples.size();++i)samples[i]=float((.4+.2*std::sin(frame*.71))*std::sin((i+frame*37)*.023)+.1*std::sin((i+frame*71)*.009));
   auto original=samples;auto wave=legacyEnvelope(samples,801,1024);need(samples==original,"Centered view modified input");float mean=0,peak=0;for(float v:samples){mean+=v;peak=std::max(peak,std::abs(v));}mean/=float(samples.size());float mid=(wave[400].center-mean)*std::min(64.f,.65f/peak);low=std::min(low,mid);high=std::max(high,mid);
  }need(high-low>.02f,"Legacy middle height is pinned");
  auto quiet=legacyEnvelope(std::vector<float>(16,0.f),8,8);for(auto p:quiet)need(p.center==0,"Legacy silence unstable");}
 {std::vector<float> samples(32,-1.f);samples[16]=.5f;samples[17]=.75f;samples[21]=.25f;samples[22]=.75f;
  auto old=legacyEnvelope(samples,16,16);need(old[0].center==samples[14]&&old[8].center==samples[22],"Legacy search is not centered or old integer buckets changed");need(old[8].center!=0,"Legacy center pinned to zero");
  auto fresh=envelope(samples,16,16);need(old[8].center!=fresh[8].center,"Legacy and new rendering paths collapsed together");
  for(size_t size:{1u,2u,8u})for(size_t cols:{1u,8u,801u}){auto small=legacyEnvelope(std::vector<float>(size,.25f),cols,4096);for(auto point:small)need(point.center==.25f,"Short legacy history out of bounds");}
 }
 unsigned cases=0;
 for(int shape=0;shape<4;shape++)for(size_t window:{256u,2048u,4096u})for(int refresh:{60,144}){
  std::vector<float>input(4096);float firstMin=10,firstMax=-10,midMin=10,midMax=-10;
  for(int frame=0;frame<48;frame++){
   double hz=shape==0?220.3:shape==1?32.1:shape==2?110.1:375.;double phase=frame*48000./refresh+.37;
   for(size_t i=0;i<input.size();i++){double p=6.283185307179586*hz*(i+phase)/48000.;input[i]=shape==3?(std::fmod(i+phase,128.)<32?.8f:.2f):float((shape==0?.65:0)+.4*(std::sin(p)+(shape==2?.22*std::sin(2*p+.4):0)));}
   auto original=input;EnvelopeScale scale;auto fresh=envelope(input,800,window,&scale);
   need(original==input,"Display modified source samples");need(!scale.triggered&&scale.offset==input.size()-window,"Viewport still locks to a crossing");
   firstMin=std::min(firstMin,fresh.front().center);firstMax=std::max(firstMax,fresh.front().center);midMin=std::min(midMin,fresh[400].center);midMax=std::max(midMax,fresh[400].center);
   auto paused=envelope(input,800,window);for(size_t i=0;i<fresh.size();i++)need(fresh[i].center==paused[i].center,"Unchanged/paused audio invented display motion");
  }
  need(firstMax-firstMin>.05f&&midMax-midMin>.05f,"Left edge or middle remains pinned");
  std::cout<<"shape="<<shape<<" window="<<window<<" refresh="<<refresh<<" left_range="<<firstMax-firstMin<<" middle_range="<<midMax-midMin<<'\n';cases++;
 }
 for(float dc:{0.f,.5f}){EnvelopeScale scale;auto e=envelope(std::vector<float>(4096,dc),800,4096,&scale);for(auto p:e)need(p.minimum==dc&&p.maximum==dc&&p.center==dc,"Silence/DC unstable");}
 std::vector<float>peak(4096);for(size_t i=0;i<peak.size();i++)peak[i]=i%2?1.f:-1.f;auto e=envelope(peak,128);for(auto p:e)need(p.minimum==-1&&p.maximum==1,"Pixel buckets dropped audio peaks");
 auto e2=envelope({0,1,2,3},7,4);for(size_t i=0;i<e2.size();i++)need(std::abs(e2[i].center-i*.5)<1e-6,"Fractional interpolation regressed");
 EnvelopeScale scale;need(envelope({},0,0,&scale).empty(),"Empty input");auto shortView=envelope({1},8,4096,&scale);for(auto p:shortView)need(p.center==1,"Short history out of bounds");
 std::vector<float>noise(4096);uint32_t seed=7;auto random=[&]{seed=seed*1664525+1013904223;return (int(seed>>8)/8388608.f)-1;};for(auto&v:noise)v=random();auto before=envelope(noise,800);for(auto&v:noise)v=random();auto after=envelope(noise,800);double change=0;for(size_t i=0;i<after.size();i++)change+=std::abs(after[i].center-before[i].center);need(change>1,"Noise/transients frozen");
 auto legacyStart=std::chrono::steady_clock::now();for(unsigned n=0;n<1000;n++)e=legacyEnvelope(noise,800,4096);auto legacyMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-legacyStart).count();std::cout<<"PASS centered legacy trigger/integer buckets, paths distinct;1000 legacy scopes="<<legacyMs<<"ms\n";
 auto start=std::chrono::steady_clock::now();for(unsigned n=0;n<100;n++)e=envelope(noise,800,4096);auto ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
 std::cout<<"PASS "<<cases<<" free-running phase sweeps; left/middle not pinned; paused deterministic;100 max-window scopes="<<ms<<"ms; silence/DC/peaks/short/noise/no-write PASS\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
