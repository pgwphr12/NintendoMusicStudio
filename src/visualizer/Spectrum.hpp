#pragma once
#include <array>
#include <complex>
#include <algorithm>
#include <cmath>
namespace ngmv {
class SpectrumAnalyzer {
public:
    static constexpr size_t Frames=4096,Bands=72;
    using Samples=std::array<float,Frames*2>;
    using Levels=std::array<float,Bands>;
    static constexpr size_t DetailBins=256;
    using Detail=std::array<float,DetailBins>;
    Levels left{},right{};
    Detail detailLeft{},detailRight{};
    static float edge(size_t i){return 40.f*std::pow(400.f,float(i)/Bands);}
    void analyze(const Samples& stereo,int rate){
        if(rate<1000){left.fill(0);right.fill(0);detailLeft.fill(0);detailRight.fill(0);return;}
        transform(stereo,0,rate,left,detailLeft);transform(stereo,1,rate,right,detailRight);
    }
private:
    std::array<std::complex<float>,Frames> bins_{};
    void transform(const Samples& src,size_t channel,int rate,Levels& levels,Detail& detail){
        constexpr float pi=3.14159265358979323846f;
        float mean=0;
        for(size_t i=0;i<Frames;++i){float v=src[i*2+channel];if(std::isfinite(v))mean+=v/Frames;}
        for(size_t i=0;i<Frames;++i){
            float x=src[i*2+channel];if(!std::isfinite(x))x=0;
            bins_[i]={(x-mean)*(.5f-.5f*std::cos(2*pi*float(i)/(Frames-1))),0};
        }
        for(size_t i=1,j=0;i<Frames;++i){
            size_t bit=Frames>>1;for(;j&bit;bit>>=1)j^=bit;j^=bit;
            if(i<j)std::swap(bins_[i],bins_[j]);
        }
        for(size_t n=2;n<=Frames;n<<=1){
            std::complex<float> root=std::polar(1.f,-2*pi/float(n));
            for(size_t start=0;start<Frames;start+=n){
                std::complex<float> w{1,0};
                for(size_t j=0;j<n/2;++j){
                    auto a=bins_[start+j],b=bins_[start+j+n/2]*w;
                    bins_[start+j]=a+b;bins_[start+j+n/2]=a-b;w*=root;
                }
            }
        }
        for(size_t band=0;band<Bands;++band){
            float low=edge(band)*Frames/rate,high=edge(band+1)*Frames/rate;
            if(low>=Frames/2){levels[band]=0;continue;}
            // Interpolate sub-bin bands; average power over wider log bands.
            float power=0;size_t count=0;
            size_t first=std::max(size_t(1),size_t(std::ceil(low)));
            size_t last=std::min(Frames/2-1,size_t(std::floor(high)));
            if(first<=last){for(size_t k=first;k<=last;++k){power+=std::norm(bins_[k]);++count;}}
            else{
                float k=std::clamp((low+high)*.5f,1.f,float(Frames/2-2));
                size_t at=size_t(k);float f=k-float(at);
                power=std::norm(bins_[at])*(1-f)+std::norm(bins_[at+1])*f;count=1;
            }
            float amplitude=std::sqrt(power/float(count))*4.f/Frames;
            float db=20.f*std::log10(std::max(amplitude,1e-9f));
            levels[band]=std::pow(std::clamp((db+72.f)/72.f,0.f,1.f),1.5f);
        }
        // Separate high-resolution, vertical log-frequency display for the
        // scrolling spectrogram; the original72-band output remains unchanged.
        for(size_t i=0;i<detail.size();++i){
            const float frequency=20.f*std::pow(1000.f,float(i)/(detail.size()-1));
            float bin=frequency*Frames/rate;
            if(bin>=Frames/2){detail[i]=0;continue;}
            bin=std::clamp(bin,1.f,float(Frames/2-2));
            const size_t at=size_t(bin);const float blend=bin-float(at);
            const float power=std::norm(bins_[at])*(1-blend)+std::norm(bins_[at+1])*blend;
            const float amplitude=std::sqrt(power)*4.f/Frames;
            const float db=20.f*std::log10(std::max(amplitude,1e-9f));
            detail[i]=std::pow(std::clamp((db+84.f)/84.f,0.f,1.f),1.7f);
        }
    }
};
class SpectrumMotion {
public:
    SpectrumAnalyzer::Levels left{},right{};
    void step(const SpectrumAnalyzer::Levels& l,const SpectrumAnalyzer::Levels& r,double seconds){
        float dt=float(std::clamp(seconds,0.,.1));
        for(size_t i=0;i<left.size();++i){
            auto ease=[dt](float current,float goal){return current+(goal-current)*(1.f-std::exp(-dt/(goal>current?.035f:.22f)));};
            left[i]=ease(left[i],l[i]);right[i]=ease(right[i],r[i]);
        }
    }
};
}
