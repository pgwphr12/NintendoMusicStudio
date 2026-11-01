#include "audio/PresentationClock.hpp"
#include "channels/ChannelHistory.hpp"
#include "visualizer/Envelope.hpp"
#include <iostream>
#include <cmath>
#include <stdexcept>
using namespace ngmv;
void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
int main(){try{for(int rate:{44100,48000,96000}){PresentationClock clock;uint64_t span=1024ull*1000000000/rate;clock.publish(1000,2000,100,span);double previous=999;for(int i=0;i<=100;i++){double at=clock.position(100+span*i/100);need(at>=previous&&std::abs(at-(1000+10*i))<.01,"Display clock not smooth/monotonic");previous=at;}need(clock.position(100+span*10)==2000,"Display invented future audio");clock.publish(0,uint64_t(std::llround(48000.*span/1e9)),100,span);need(std::abs(clock.position(100+4000000)-192)<.5,"Display speed depends on output sample rate");clock.publish(0,0,0,1);need(clock.position(9000)==0,"Display reset");}
 ChannelHistory history;history.reset(1);AudioBlock b;b.mix.resize(16);b.voices={{}};for(int i=0;i<16;i++)b.voices[0].push_back({float(i),float(i)});history.append(b);auto snap=history.interpolatedSnapshot(12.5,4);for(int i=0;i<4;i++)need(std::abs(snap[0][i]-(8.5+i))<1e-6,"Fractional waveform position");
 auto env=envelope({0,1,2,3},7,4);for(size_t i=0;i<env.size();i++)need(std::abs(env[i].center-float(i)*.5)<1e-6,"Upsampled display still stair-stepped");
 std::cout<<"PASS display-only interpolation44100/48000/96000; bounded no-future/reset/fractional-history/spatial interpolation; audio samples untouched\n";return 0;
}catch(const std::exception&e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
