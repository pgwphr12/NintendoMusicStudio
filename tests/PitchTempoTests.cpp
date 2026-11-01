#include "audio/PitchTempo.hpp"
#include "audio/Resampler.hpp"
#include "player/Player.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace ngmv;
static void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
static Stereo tone(size_t i){return {float(.25*std::sin(2*3.141592653589793*440*i/48000)),float(.2*std::sin(2*3.141592653589793*660*i/48000))};}
static double frequency(const std::vector<float>&v,int rate,int ch){
    size_t begin=size_t(.25*rate),end=std::min(v.size()/2,size_t(.85*rate));
    int crosses=0;double first=0,last=0;
    for(size_t i=begin+1;i<end;++i){float a=v[2*(i-1)+ch],b=v[2*i+ch];if(a<=0&&b>0){double at=i-1+double(-a)/(b-a);if(!crosses)first=at;last=at;++crosses;}}
    need(crosses>30,"No audible pitched sine");return (crosses-1)*rate/(last-first);
}
static void check(int rate,double speed,double pitch){
    StereoRing<> source;PitchTempo engine;engine.rate(rate);
    constexpr size_t total=96000;
    size_t written=0;std::vector<float> result;std::array<Stereo,512> input;std::array<float,512> output;
    uint64_t previous=0;
    for(int cycle=0;cycle<10000;++cycle){
        while(source.free()>=input.size() && written<total){size_t n=std::min(input.size(),total-written);for(size_t i=0;i<n;++i)input[i]=tone(written+i);need(source.push(input.data(),n),"Source push");written+=n;}
        engine.pull(source,output.data(),256,1,speed,pitch,written==total);
        for(auto v:output)need(std::isfinite(v)&&std::abs(v)<2,"Nonfinite/clipped DSP output");
        result.insert(result.end(),output.begin(),output.end());
        need(engine.consumed>=previous && engine.consumed<=total,"Source clock incorrect");previous=engine.consumed;
        if(engine.drained(source,written==total))break;
        need(cycle<9999,"DSP did not reach real EOF");
    }
    double duration=double(result.size()/2)/rate;
    need(std::abs(duration-2/speed)<.025,"Speed changed duration incorrectly");
    need(engine.underruns==0,"DSP underrun with supplied input");
    double left=frequency(result,rate,0),right=frequency(result,rate,1);
    need(std::abs(left-440*pitch)<440*pitch*.015,"Pitch/speed are still coupled (left)");
    need(std::abs(right-660*pitch)<660*pitch*.015,"Pitch/speed are still coupled (right)");
    engine.reset();need(engine.consumed==0 && engine.underruns==0,"Reset retained old timeline");
    std::cout<<"rate="<<rate<<" speed="<<speed<<" pitch="<<pitch<<" duration="<<duration<<" frequency="<<left<<","<<right<<" PASS\n";
}
int main(){try{
    for(int rate:{44100,48000,96000})for(auto pair: {std::pair<double,double>{1.5,1.},{1.,1.25},{.75,.8},{2.,.5}})check(rate,pair.first,pair.second);
    StereoRing<> a,b;std::array<Stereo,4096> input;for(size_t i=0;i<input.size();++i)input[i]=tone(i);a.push(input.data(),input.size());b.push(input.data(),input.size());
    PitchTempo unity;OutputResampler old;unity.rate(96000);old.rate(96000);std::array<float,4096> x,y;unity.pull(a,x.data(),2048,1,1,1,false);old.pull(b,y.data(),2048,1);
    need(x==y && unity.consumed==old.consumed,"Default output changed");
    Player player;need(player.pitch()==1 && player.speed()==1,"Default factors");
    need(player.reverbAmount()==.76f && player.reverbDecay()==3.f,"Default reverb");
    player.pitch(1.25f);need(player.speed()==1,"Pitch changed speed control");player.speed(.75f);need(player.pitch()==1.25f,"Speed changed pitch control");
    player.pitch(NAN);player.speed(NAN);need(player.pitch()==1.25f && player.speed()==.75f,"Nonfinite factors accepted");
    std::cout<<"PASS independent pitch/tempo, stereo, EOF, source clock, reset, exact unity bypass and defaults\n";return 0;
}catch(const std::exception&e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
