#include "audio/GraphicEqualizer.hpp"
#include "../third_party/mesence-eq/Equalizer.h"
#include <iostream>
#include <random>
#include <stdexcept>
using namespace ngmv;
void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
int main(){try{
 GraphicEqualizer eq;Equalizer reference;std::array<float,20> gains{};std::mt19937 rng(1628);uint64_t checked=0;int maxError=0;
 auto compare=[&](int frames){std::vector<int16_t> expected(frames*2),input(frames*2);for(auto&v:input)v=int16_t((int(rng()%65536)-32768)/3);expected=input;reference.ApplyEqualizer(frames,expected.data());for(int n=0;n<frames;n++){auto out=eq.process({input[n*2]/32768.f,input[n*2+1]/32768.f});int l=int(out.l*32768),r=int(out.r*32768);maxError=std::max({maxError,std::abs(l-int(expected[n*2])),std::abs(r-int(expected[n*2+1]))});need(l==expected[n*2]&&r==expected[n*2+1],"MesenCE reference mismatch");checked++;}};
 // Single20bands plus flat/all+15/mixed extrema. Reference is upstream source,
 // not an independently reimplemented duplicate of the new integration wrapper.
 for(int scenario=0;scenario<24;scenario++){
  gains={};if(scenario<20)gains[scenario]=15;else if(scenario==21)gains.fill(15);else if(scenario==22)for(int i=0;i<20;i++)gains[i]=i%2?20:-20;else if(scenario==23)gains.fill(-20);
  std::vector<double> d(gains.begin(),gains.end());reference.UpdateEqualizers(d,48000);eq.beginBlock(true,gains);compare(8192);
 }
 // Same settings retain filter history, gain change resets it like upstream.
 eq.beginBlock(true,gains);compare(1024);
 eq.beginBlock(false,gains);for(int i=0;i<2048;i++){Stereo in={float(i*.0001),float(-i*.0002)};auto out=eq.process(in);need(out.l==in.l&&out.r==in.r,"Disabled path changed PCM");}
 eq.beginBlock(true,gains);compare(1024);
 gains[0]=20;reference.UpdateEqualizers(std::vector<double>(gains.begin(),gains.end()),48000);eq.beginBlock(true,gains);compare(1024);
 need(GraphicEqualizer::frequencies[19]==15000&&GraphicEqualizer::filterFrequencies[19]==13000,"MesenCE label/filter distinction was lost");
 std::cout<<"PASS upstream MesenCE4b8a196 reference24profiles/"<<checked<<" stereo frames exact max_PCM16_error="<<maxError<<"; zero/all+15/extremes/updates/frozen bypass/re-enable/history/clipping\n";return 0;
}catch(const std::exception&e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}