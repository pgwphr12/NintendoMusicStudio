#include "visualizer/Envelope.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include "channels/ChannelHistory.hpp"
using namespace ngmv;
void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
int main(){try{
 double rawError=0,trackedError=0;unsigned cases=0;bool edgeFree=false;
 for(size_t window:{256u,2048u,4096u})for(int refresh:{60,144,240})for(int shape:{0,1,2})for(double hz:{32.1,110.1,220.3,997.7}){
  WaveformTracker tracker;std::vector<float>samples(window+2048);std::vector<EnvelopePoint>previous,rawPrevious;
  double initialEdge=0;
  for(unsigned frame=0;frame<60;++frame){for(size_t i=0;i<samples.size();++i){double phase=(i+frame*48000./refresh+.37)*hz/48000.;samples[i]=shape==2?(std::fmod(phase,1.)<.3?.6f:-.2f):float(.6*std::sin(phase*6.28318530718)+(shape==1?.1*std::sin(phase*12.56637061436+.3):0));}
   auto original=samples;double at=tracker.locate(samples,window);need(at>=0&&at<=2048,"Invalid viewport");
   auto view=envelope(samples,800,window,nullptr,at),raw=envelope(samples,800,window);
   if(frame==0){initialEdge=view.front().center;edgeFree|=std::abs(initialEdge)>.05;}
   if(frame>2&&shape!=2)for(size_t x=0;x<view.size();++x){trackedError+=std::abs(view[x].center-previous[x].center);rawError+=std::abs(raw[x].center-rawPrevious[x].center);}
   need(samples==original,"Source modified");need(tracker.locate(samples,window)==at,"Paused display changed");previous=view;rawPrevious=raw;
  }++cases;
 }
 need(edgeFree,"Edge forced to zero");need(trackedError<rawError*.12,"Periodic shape still drifts/jitters");
 WaveformTracker noise;std::vector<float>s(6144);unsigned seed=1;for(int f=0;f<4;++f){for(auto&v:s){seed=seed*1664525+1013904223;v=float(int(seed>>8))/8388608.f-1;}need(noise.locate(s,2048)==4096,"Noise frozen as old shape");}
 std::fill(s.begin(),s.end(),0);need(noise.locate(s,2048)==4096,"Silence delayed");
 float eased=0;
 for(int hz:{60,144,240}){WaveformTransition t;std::vector<EnvelopePoint>p(16);t.apply(p,0,true);
  for(int f=0;f<hz/2;++f){p.assign(16,{1,1,1});t.apply(p,1./hz,true);if(f==0)need(p[0].center>0&&p[0].center<1,"Hard visual transition");}
  if(hz==60)eased=p[0].center;else need(std::abs(p[0].center-eased)<1e-5,"Refresh-dependent transition");
  p.assign(16,{.25f,.25f,.25f});t.apply(p,1./hz,false);need(p[0].center==.25f,"Paused display keeps animating");
 }
 ChannelHistory history;history.reset(3,0);AudioBlock block;block.mix.resize(2048);block.voices.assign(3,std::vector<Stereo>(2048));
 for(size_t c=0;c<3;++c)for(size_t i=0;i<2048;++i)block.voices[c][i]={float(i*.001+c),float(i*.002-c)};history.append(block);
 for(double endpoint:{0.,.25,1500.375,2048.,3000.}){auto full=history.interpolatedSnapshot(endpoint,4096),selected=history.interpolatedSnapshot(endpoint,4096,5);
  need(selected[1].empty(),"Hidden channel copied");need(full[0]==selected[0]&&full[2]==selected[2],"Selected interpolation changed audio history");
  double end=std::min(endpoint,2048.);for(size_t j=0;j<4096;++j){double pos=end+j-4096;float expected=0;if(pos>=0){size_t a=size_t(pos),b=std::min(size_t(2047),a+1);float f=float(pos-a);expected=(block.voices[0][a].l+block.voices[0][a].r)*.5f*(1-f)+(block.voices[0][b].l+block.voices[0][b].r)*.5f*f;}need(std::abs(full[0][j]-expected)<1e-6,"History interpolation boundary changed");}
 }
 std::cout<<"PASS "<<cases<<" periodic refresh/shape/frequency sweeps; tracking/raw error="<<trackedError/rawError<<"; arbitrary nonzero edge, pause, noise, silence, source unchanged\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
