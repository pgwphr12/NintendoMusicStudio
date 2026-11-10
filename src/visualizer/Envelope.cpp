#include "Envelope.hpp"
#include <algorithm>
#include <cmath>
namespace ngmv {
std::vector<EnvelopePoint> envelope(const std::vector<float> &s, size_t columns, size_t window, EnvelopeScale *scale, double offset) {
    std::vector<EnvelopePoint> out(columns);
    if (scale) *scale = {};
    if (s.empty() || columns == 0 || window == 0) return out;
    window = std::min(window, s.size());
    auto range = std::minmax_element(s.begin(), s.end());
    const float level = (*range.first + *range.second) * .5f;
    const float amplitude = (*range.second - *range.first) * .5f;
    const size_t newest = s.size() - window;
    // Free-running viewport: no left-edge or center zero-crossing lock.
    // Every point comes from the newest real captured audio window.
    const double begin = offset < 0 ? double(newest) : std::clamp(offset, 0., double(newest));
    if (scale) *scale = {level, amplitude, begin, false};
    auto value = [&](double at) {
        at = std::clamp(at, 0., double(s.size()-1));
        size_t a = size_t(at), b = std::min(a+1, s.size()-1);
        return s[a] + float(at-a) * (s[b]-s[a]);
    };
    for (size_t x = 0; x < columns; ++x) {
        if (window < columns) {
            float v = value(begin + double(x)*(window-1)/std::max(size_t(1),columns-1));
            out[x] = {v,v,v}; continue;
        }
        // N captured samples span N-1 intervals. Extending one sample beyond
        // that clips the last bucket only on certain trigger phases (jitter).
        const double a = begin + double(x)*(window-1)/columns;
        const double b = begin + double(x+1)*(window-1)/columns;
        double at = a, integral = 0;
        float v = value(a), mn = v, mx = v;
        // Integrate fractional buckets so sub-sample trigger alignment also
        // aligns the drawn line. Retain actual interior sample peaks.
        while (at < b) {
            double next = std::min(b, std::floor(at)+1);
            float w = value(next);
            integral += (v+w)*.5*(next-at);
            mn = std::min(mn,w); mx = std::max(mx,w);
            at = next; v = w;
        }
        out[x] = {mn,mx,b>a?float(integral/(b-a)):v};
    }
    return out;
}
void WaveformTransition::apply(std::vector<EnvelopePoint>& points,double elapsed,bool playing){
    if(!playing||previous_.size()!=points.size()){previous_=points;return;}
    float alpha=float(-std::expm1(-std::clamp(elapsed,0.,.1)/.012));
    for(size_t i=0;i<points.size();++i){auto&p=points[i];const auto&old=previous_[i];
        p.minimum=old.minimum+alpha*(p.minimum-old.minimum);
        p.maximum=old.maximum+alpha*(p.maximum-old.maximum);
        p.center=old.center+alpha*(p.center-old.center);
    }
    previous_=points;
}
double WaveformTracker::locate(const std::vector<float>& s, size_t window) {
    if(s.empty() || !window){window_=0;return 0;}
    window=std::min(window,s.size());
    if(window_==window && lastInput_==s)return lastAt_;
    double latest=double(s.size()-window), bestAt=latest;
    auto value=[&](double at){size_t i=size_t(at);return s[i]+float(at-i)*(s[std::min(i+1,s.size()-1)]-s[i]);};
    // Irregular sample locations avoid aliasing a high note at a fixed stride.
    if(window_!=window)for(size_t i=0;i<positions_.size();++i)positions_[i]=std::clamp((window-1)*(double(i)+.31*std::sin(double(i)*2.3999632297))/(positions_.size()-1),0.,double(window-1));
    auto point=[&](size_t i){return positions_[i];};
    double mean=0,energy=0;for(auto v:reference_)mean+=v;mean/=reference_.size();
    for(auto v:reference_)energy+=(v-mean)*(v-mean);
    double best=-2;
    if(window_==window && energy>1e-9 && latest>0){
        auto score=[&](double at,size_t stride){double sum=0,square=0,dot=0,rsum=0,rsquare=0;size_t count=0;
            for(size_t i=0;i<reference_.size();i+=stride){double v=value(at+point(i)),r=reference_[i];sum+=v;square+=v*v;dot+=r*v;rsum+=r;rsquare+=r*r;++count;}
            double e=square-sum*sum/count,re=rsquare-rsum*rsum/count;
            if(e<re*.16 || e>re*6.25 || re<1e-12)return -2.;
            return (dot-rsum*sum/count)/std::sqrt(std::max(1e-30,e*re));};
        double lo=std::max(0.,latest-2048.);
        // A distinctive interior slope supplies fractional candidate positions.
        // This matches the old shape at an arbitrary interior level; it never
        // pins the left edge/center to zero. Reject noise with a cheap subset
        // before evaluating the full reference. No 513x96 exhaustive scan.
        if(std::abs(anchorSlope_)>1e-7f){
            size_t first=size_t(std::ceil(lo+anchor_)),last=std::min(s.size()-2,size_t(std::floor(latest+anchor_)));
            for(size_t j=last+1;j-->first;){
                float a=s[j],b=s[j+1],delta=b-a;
                if((anchorSlope_>0?delta<=0:delta>=0)||anchorLevel_<std::min(a,b)||anchorLevel_>std::max(a,b))continue;
                double at=double(j)+(anchorLevel_-a)/delta-anchor_;
                if(at<lo||at>latest||score(at,8)<.94)continue;
                double v=score(at,1);if(v>best){best=v;bestAt=at;}
            }
        }
    }
    if(best<.97)bestAt=latest;
    // Keep a strongly matching reference: refreshing every frame accumulates
    // fractional phase error into the same slow sideways drift being fixed.
    if(window_!=window || best<.97){
        for(size_t i=0;i<reference_.size();++i)reference_[i]=value(bestAt+point(i));
        anchorSlope_=0;
        for(size_t i=1;i+2<window;++i){float a=value(bestAt+i),b=value(bestAt+i+1),slope=b-a;if(std::abs(slope)>std::abs(anchorSlope_)){anchor_=i+.5;anchorLevel_=(a+b)*.5f;anchorSlope_=slope;}}
        window_=window;
    }
    lastInput_=s;lastAt_=bestAt;return bestAt;
}
} // namespace ngmv
