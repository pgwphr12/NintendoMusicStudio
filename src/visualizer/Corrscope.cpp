// CorrelationTrigger/get_period/LogFreqSpectrum translated from corrscope 0.11.1.
// Copyright (c) 2018-2020+, nyanpasu64. BSD-2-Clause; see third_party/corrscope/LICENSE.
// Native FFT, live-input adapter and worker integration: Nintendo Music Studio.
#include "Corrscope.hpp"
#include <algorithm>
#include <cmath>
#include <complex>
#include <condition_variable>
#include <map>
#include <mutex>
#include <thread>
#include <stdexcept>
namespace ngmv { namespace {
using Complex=std::complex<double>;
constexpr double Pi=3.14159265358979323846;
int rounded(double x){return int(std::nearbyint(x));}
size_t power2(size_t n){size_t p=1;while(p<n)p*=2;return p;}
void fft(std::vector<Complex>& a,bool inverse=false){
 const size_t n=a.size();
 for(size_t i=1,j=0;i<n;++i){size_t bit=n/2;for(;j&bit;bit/=2)j^=bit;j^=bit;if(i<j)std::swap(a[i],a[j]);}
 for(size_t size=2;size<=n;size*=2){const auto root=std::polar(1.,(inverse?2:-2)*Pi/size);for(size_t at=0;at<n;at+=size){Complex w=1;for(size_t j=0;j<size/2;++j){auto u=a[at+j],v=a[at+j+size/2]*w;a[at+j]=u+v;a[at+j+size/2]=u-v;w*=root;}}}
 if(inverse)for(auto &v:a)v/=double(n);
}
size_t fastLength(size_t n){for(size_t k=n;;++k){size_t q=k;for(size_t p:{2u,3u,5u})while(q%p==0)q/=p;if(q==1)return k;}}
std::vector<Complex> fftSmooth(const std::vector<Complex>& a){
 const size_t n=a.size();if(n==1)return a;if((n&(n-1))==0){auto out=a;fft(out);return out;}
 size_t radix=n%2==0?2:n%3==0?3:5,m=n/radix;
 std::vector<std::vector<Complex>> sub(radix,std::vector<Complex>(m));
 for(size_t j=0;j<radix;++j){for(size_t k=0;k<m;++k)sub[j][k]=a[j+radix*k];sub[j]=fftSmooth(sub[j]);}
 thread_local std::map<size_t,std::vector<Complex>> roots;
 auto &r=roots[n];if(r.empty()){r.resize(n);for(size_t k=0;k<n;++k)r[k]=std::polar(1.,-2*Pi*k/n);}
 std::vector<Complex> out(n);for(size_t k=0;k<n;++k)for(size_t j=0;j<radix;++j)out[k]+=sub[j][k%m]*r[(j*k)%n];return out;
}
std::vector<double> correlate(const std::vector<float>& data,const std::vector<float>& kernel,bool full=false){
 const size_t n=power2(full?data.size()+kernel.size()-1:data.size());
 std::vector<Complex> x(n),y(n);for(size_t i=0;i<data.size();++i)x[i]=data[i];for(size_t i=0;i<kernel.size();++i)y[i]=kernel[i];
 fft(x);fft(y);for(size_t i=0;i<n;++i)x[i]*=std::conj(y[i]);fft(x,true);
 std::vector<double> result(full?data.size():data.size()-kernel.size()+1);for(size_t i=0;i<result.size();++i)result[i]=x[i].real();return result;
}
template<class T> void normalize(std::vector<T>& data){double peak=.01;for(auto v:data)peak=std::max(peak,std::abs(double(v)));for(auto &v:data)v=T(v/peak);}
std::vector<float> gaussian(size_t n,double sigma){std::vector<float> w(n);if(sigma>0)for(size_t i=0;i<n;++i){double x=(double(i)-(n-1)*.5)/sigma;w[i]=float(std::exp(-.5*x*x));}return w;}
int periodOf(const std::vector<float>& data,int rate){
 float peak=0;for(float v:data)peak=std::max(peak,std::abs(v));if(peak<.01f)return 0;
 auto corr=correlate(data,data,true);size_t zero=0;while(zero<corr.size()&&corr[zero]>=0)++zero;if(zero==corr.size())return 0;
 size_t first=std::max(zero,size_t(rounded(rate/4000.)));if(first>=corr.size())return 0;
 auto peakAt=[&](){return size_t(std::max_element(corr.begin()+first,corr.end())-corr.begin());};
 auto p=peakAt();if(p>.1*data.size()){for(size_t i=0;i<corr.size();++i)corr[i]/=std::max(float(1-.9*i/data.size()),.5f);p=peakAt();}return int(p);
}
std::vector<double> spectrum(const std::vector<float>& data,int rate){
 size_t n=fastLength(data.size());std::vector<size_t> fences(49);
 for(;;){for(size_t i=0;i<49;++i)fences[i]=size_t(float(n/double(rate))*float(20*std::pow(256.,i/48.)));bool good=true;for(size_t i=1;i<49;++i)good&=fences[i]!=fences[i-1];if(good)break;n=fastLength(n+n/5+1);}
 std::vector<Complex> a(n);for(size_t i=0;i<std::min(n,data.size());++i)a[i]=data[i];a=fftSmooth(a);
 std::vector<double> out(48);for(size_t b=0;b<48;++b){size_t lo=fences[b],hi=std::min(fences[b+1],n/2+1);if(lo>=hi)continue;for(size_t k=lo;k<hi;++k)out[b]+=std::abs(a[k]);out[b]/=hi-lo;}return out;
}
void rescaleBuffer(std::vector<float>& buffer,const std::vector<float>& data,int rate){
 auto current=spectrum(data,rate),previous=spectrum(buffer,rate);normalize(current);
 double best=-1e300;int shift=0;for(int k=-6;k<=6;++k){double dot=0;for(int i=0;i<48;++i){int j=i-k;if(j>=0&&j<48)dot+=current[size_t(i)]*previous[size_t(j)];}if(dot>best){best=dot;shift=k;}}
 if(!shift)return;size_t length=size_t(std::max(1,rounded(buffer.size()/std::pow(2.,shift/6.))));std::vector<float> resized(length);
 for(size_t i=0;i<length;++i){double at=length>1?double(i)*(buffer.size()-1)/(length-1):0;size_t lo=size_t(at),hi=std::min(lo+1,buffer.size()-1);resized[i]=float(buffer[lo]+(at-lo)*(buffer[hi]-buffer[lo]));}
 const int diff=int(length)-int(buffer.size());for(size_t i=0;i<buffer.size();++i){int at=int(i)+(diff>0?diff/2:-((-diff)/2));buffer[i]=at>=0&&size_t(at)<resized.size()?resized[size_t(at)]:0;}
}
}
float CorrscopeDisplayScale::update(float amplitude,float peak,double seconds){
 if(!std::isfinite(amplitude)||!std::isfinite(peak)||amplitude<=.0001f||peak<=0)return gain_;
 const double elapsed=std::isfinite(seconds)?std::clamp(seconds,0.,.1):0.;
 // Estimate a channel's normal strength, not each sample's height. Hold peaks
 // through note releases so natural envelopes are not flattened or pumped.
 if(!ready_){
  level_=std::max(level_,amplitude);warmup_+=elapsed;
  if(warmup_<.1)return gain_=std::min(gain_,.95f/peak);
  gain_=std::clamp(.65f/level_,.125f,256.f);ready_=true;hold_=.75;
 }else{
  if(amplitude>=level_){level_=amplitude;hold_=.75;}
  else if(amplitude>=level_*.9f)hold_=.75;
  else if((hold_-=elapsed)<=0)level_+=float(-std::expm1(-elapsed/4.))*(amplitude-level_);
  const float target=std::clamp(.65f/std::max(level_,.0001f),.125f,256.f);
  const double alpha=-std::expm1(-elapsed/(target<gain_?.08:.4));
  gain_=float(std::exp(std::log(gain_)+alpha*std::log(target/gain_)));
 }
 // A DC-biased signal keeps its real offset, so limit only the common scalar.
 // Never clamp/filter individual waveform points into a different shape.
 return gain_=std::min(gain_,.95f/peak);
}
CorrscopeTrigger::CorrscopeTrigger(size_t size,int rate,double fps):size_(size-size%2),rate_(rate),fps_(fps),buffer_(size_),slope_(size_),window_(size_){if(size_<8||size_>8192||rate<=0||fps<=0)throw std::invalid_argument("Invalid corrscope setup");}
CorrscopeTrigger::Result CorrscopeTrigger::process(const std::vector<float>& source,int64_t base,int64_t pos){
 const int64_t half=int64_t(size_/2),diameter=int64_t(size_/2);
 auto sample=[&](int64_t at){at-=base;return at>=0&&uint64_t(at)<source.size()?source[size_t(at)]:0.f;};
 int64_t begin=std::max(pos-rounded(rate_/fps_),pos-diameter/2),dataBegin=begin-half;
 std::vector<float> data(size_+size_t(diameter)),periodData(data.size());float mean=0;
 for(size_t i=0;i<data.size();++i){data[i]=sample(dataBegin+int64_t(i));mean+=data[i];}mean/=float(data.size());for(size_t i=0;i<data.size();++i)periodData[i]=data[i]-mean;
 if(std::all_of(data.begin(),data.end(),[](float v){return v==0;})){
    std::fill(buffer_.begin(),buffer_.end(),0);std::fill(window_.begin(),window_.end(),0);if(previousPeriod_<0){previousPeriod_=0;slope_=gaussian(size_,1);for(size_t i=0;i<size_;++i)slope_[i]*=i<size_/2?-1.f:1.f;}
    previousTrigger_=std::max(pos,previousTrigger_);++framesSinceSpectrum_;return {previousTrigger_,0,0};
 }
 const int period=periodOf(periodData,rate_);bool invalid=previousPeriod_<0||(period!=0&&(previousPeriod_==0||std::abs(-12*std::log2(double(period)/previousPeriod_))>1));
 if(invalid){double width=std::clamp(.25*period,1.,half/3.);slope_=gaussian(size_,width);for(size_t i=0;i<size_;++i)slope_[i]*=i<size_/2?-1.f:1.f;
  if(framesSinceSpectrum_>=1&&std::any_of(data.begin(),data.end(),[](float v){return v!=0;})){rescaleBuffer(buffer_,data,rate_);framesSinceSpectrum_=0;}
  previousPeriod_=period;
 }
 auto quality=correlate(data,buffer_);bool enabled=true;size_t peak=size_t(std::max_element(quality.begin(),quality.end())-quality.begin());
 std::vector<float> aligned(size_);for(size_t i=0;i<size_;++i)aligned[i]=data[peak+i]-mean;normalize(aligned);double self=0;for(size_t i=0;i<size_;++i)self+=data[peak+i]*aligned[i]*window_[i];
 if(quality[peak]/(self+.001)<.3){std::fill(quality.begin(),quality.end(),0);std::fill(buffer_.begin(),buffer_.end(),0);enabled=false;}
 auto kernel=slope_;if(enabled)for(size_t i=0;i<size_;++i)kernel[i]+=buffer_[i];auto corr=correlate(data,kernel);
 float sum=0;auto peaks=quality;for(size_t i=0;i<peaks.size();++i){sum+=data[size_/2-1+i];peaks[i]-=sum;}
 const size_t middle=corr.size()/2;size_t left=0,right=corr.size();if(period){size_t radius=size_t(std::max(0,rounded(period*1.5)));left=middle>radius?middle-radius:0;right=std::min(middle+radius+1,corr.size());}
 double minimum=*std::min_element(corr.begin()+left,corr.begin()+right);
 for(size_t i=left;i<right;++i)if(i==left||i+1==right||(i+1<right&&peaks[i]<peaks[i+1])||(i>left&&peaks[i]<peaks[i-1]))corr[i]=minimum;
 auto found=size_t(std::max_element(corr.begin()+left,corr.begin()+right)-corr.begin());if(corr[found]==minimum)found=middle;
 const int64_t trigger=std::max(begin+int64_t(found),previousTrigger_);previousTrigger_=trigger;
 double alignedMean=0;for(size_t i=0;i<size_;++i){aligned[i]=sample(trigger-half+int64_t(i));alignedMean+=aligned[i];}alignedMean/=size_;
 for(auto &v:aligned)v-=float(alignedMean);normalize(aligned);window_=gaussian(size_,period*.5);normalize(buffer_);for(size_t i=0;i<size_;++i)buffer_[i]=buffer_[i]*.5f+aligned[i]*window_[i]*.5f;
 ++framesSinceSpectrum_;return {trigger,period,alignedMean};
}
struct CorrscopeWorker::State {
 struct Job {std::vector<std::vector<float>> samples;std::vector<size_t> windows;uint64_t mask=0,epoch=0;double seconds=0;};
 std::mutex mutex;std::condition_variable condition;bool stop=false,pending=false;uint64_t epoch=0,completed=0;Job job;
 std::vector<std::vector<EnvelopePoint>> ready;std::thread thread;
 State():thread([this]{run();}){}
 ~State(){{std::lock_guard<std::mutex> l(mutex);stop=true;}condition.notify_one();thread.join();}
 void run(){std::vector<CorrscopeTrigger> triggers;uint64_t current=~uint64_t(0);int64_t cursor=0;double previous=0;
  for(;;){Job j;{std::unique_lock<std::mutex> l(mutex);condition.wait(l,[&]{return stop||pending;});if(stop)return;j=std::move(job);pending=false;}
   if(current!=j.epoch){triggers.clear();current=j.epoch;cursor=0;previous=j.seconds;}
   cursor+=std::max(0,rounded(std::clamp(j.seconds-previous,0.,1.)*48000));previous=j.seconds;
   while(triggers.size()<j.samples.size())triggers.emplace_back();std::vector<std::vector<EnvelopePoint>> output(j.samples.size());
   for(size_t c=0;c<j.samples.size()&&c<64;++c)if(j.mask&(uint64_t(1)<<c)){
    const auto &s=j.samples[c];size_t w=c<j.windows.size()?std::min(j.windows[c],s.size()):0;if(!w)continue;
    int64_t pos=cursor+int64_t(s.size())-int64_t(std::max(w/2+480,size_t(1440)));
    auto result=triggers[c].process(s,cursor,pos);int64_t at=result.sample-cursor-int64_t(w/2);at=std::clamp(at,int64_t(0),int64_t(s.size()-w));
    // The trigger is upstream corrscope; the line is drawn by the host UI.
    const size_t columns=w;auto &line=output[c];line.resize(columns);
    for(size_t x=0;x<columns;++x){double p=double(at)+(columns>1?double(x)*(w-1)/(columns-1):0.);size_t a=size_t(p),b=std::min(a+1,s.size()-1);float value=s[a]+float(p-a)*(s[b]-s[a]);line[x]={value,value,value};}
   }
   {std::lock_guard<std::mutex> l(mutex);if(current==epoch){ready=std::move(output);++completed;}}
  }
 }
};
CorrscopeWorker::CorrscopeWorker():state_(std::make_unique<State>()){}
CorrscopeWorker::~CorrscopeWorker()=default;
uint64_t CorrscopeWorker::completed(){std::lock_guard<std::mutex> l(state_->mutex);return state_->completed;}
void CorrscopeWorker::reset(){std::lock_guard<std::mutex> l(state_->mutex);++state_->epoch;state_->completed=0;state_->pending=false;state_->ready.clear();}
void CorrscopeWorker::submit(const std::vector<std::vector<float>>& samples,const std::vector<size_t>& windows,uint64_t mask,double seconds){
 std::lock_guard<std::mutex> l(state_->mutex);state_->job={samples,windows,mask,state_->epoch,seconds};state_->pending=true;state_->condition.notify_one();
}
std::vector<std::vector<EnvelopePoint>> CorrscopeWorker::result(){std::lock_guard<std::mutex> l(state_->mutex);return state_->ready;}
}
