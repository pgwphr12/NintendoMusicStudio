#pragma once
#include "Spectrum.hpp"
#include <cstdint>
#include <memory>
#include <limits>
namespace ngmv {
// Fixed memory, UI-thread-only log-frequency history. Horizontal position is
// monotonic display time (including stopped silence), not frequency or the
// number of repaint requests. View/transport changes do not erase the history.
class SpectrogramHistory {
public:
    static constexpr size_t Columns=640,Bins=SpectrumAnalyzer::DetailBins;
    static constexpr uint64_t StepMs=20,WindowMs=Columns*StepMs;
    using Values=std::array<uint8_t,Bins>;
    struct Column {uint64_t slot=UINT64_MAX;Values left{},right{};};
private:
    std::unique_ptr<Column[]> columns_{new Column[Columns]};
    uint64_t last_=0;bool has_=false;
    Values previousLeft_{},previousRight_{};
    static Values quantize(const SpectrumAnalyzer::Detail& source){
        Values v{};for(size_t i=0;i<Bins;++i)v[i]=uint8_t(std::lround(std::clamp(std::isfinite(source[i])?source[i]:0.f,0.f,1.f)*255));return v;
    }
public:
    void clear(){for(size_t i=0;i<Columns;++i)columns_[i].slot=UINT64_MAX;has_=false;last_=0;}
    const Column* column(uint64_t slot)const{
        const auto& c=columns_[slot%Columns];return c.slot==slot?&c:nullptr;
    }
    size_t size()const{
        if(!has_)return 0;
        size_t n=0;for(size_t i=0;i<Columns;++i)if(columns_[i].slot!=UINT64_MAX&&columns_[i].slot<=last_&&last_-columns_[i].slot<Columns)++n;return n;
    }
    uint64_t latestSlot()const{return last_;}
    void push(const SpectrumAnalyzer::Detail& l,const SpectrumAnalyzer::Detail& r,uint64_t milliseconds){
        const uint64_t slot=milliseconds/StepMs;
        Values left=quantize(l),right=quantize(r);
        if(has_&&slot<last_)clear();
        uint64_t gap=has_?slot-last_:0;
        if(gap>=Columns){clear();gap=0;}
        // Interpolate only short, normal GUI/analysis intervals. A long hidden
        // interval remains blank instead of manufacturing missing audio history.
        if(has_&&gap>0&&gap<=5){
            for(uint64_t step=1;step<gap;++step){
                auto& c=columns_[(last_+step)%Columns];c.slot=last_+step;
                const float f=float(step)/gap;
                for(size_t i=0;i<Bins;++i){
                    c.left[i]=uint8_t(std::lround(previousLeft_[i]*(1-f)+left[i]*f));
                    c.right[i]=uint8_t(std::lround(previousRight_[i]*(1-f)+right[i]*f));
                }
            }
        }
        auto& c=columns_[slot%Columns];c.slot=slot;c.left=left;c.right=right;
        last_=slot;has_=true;previousLeft_=left;previousRight_=right;
    }
};
}
