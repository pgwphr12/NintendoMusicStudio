#include "Window.hpp"
#include <array>
namespace ngmv {
namespace {
struct Palette{
    std::array<HBRUSH,SpectrumAnalyzer::Bands> brushes{};
    HBRUSH line=CreateSolidBrush(RGB(131,157,179));
    Palette(){
        for(size_t i=0;i<brushes.size();++i){
            float sector=(.83f-.67f*float(i)/float(brushes.size()-1))*6;
            int k=int(sector);float f=sector-k,r=0,g=0,b=0;
            switch(k%6){case 0:r=1;g=f;break;case 1:r=1-f;g=1;break;case 2:g=1;b=f;break;case 3:g=1-f;b=1;break;case 4:r=f;b=1;break;default:r=1;b=1-f;}
            brushes[i]=CreateSolidBrush(RGB(int(r*255),int(g*255),int(b*255)));
        }
    }
    ~Palette(){for(auto b:brushes)DeleteObject(b);DeleteObject(line);}
};
void block(HDC dc,RECT r,HBRUSH b){if(r.right>r.left&&r.bottom>r.top)FillRect(dc,&r,b);}
}
void Window::paintSpectrum(HDC dc){
    static thread_local Palette palette;
    const ULONGLONG now=GetTickCount64();
    if(spectrumLastPaint_&&spectrogramWasPlaying_&&player_.playing())spectrogramTimeMs_+=now-spectrumLastPaint_;
    spectrogramWasPlaying_=player_.playing();
    const auto epoch=player_.spectrumEpoch();
    if(epoch!=spectrumEpoch_){
        spectrumEpoch_=epoch;spectrumLastSample_=0;spectrum_.left.fill(0);spectrum_.right.fill(0);
        spectrumMotion_={};spectrogram_.clear();spectrogramTimeMs_=0;
    }
    if(now-spectrumLastAnalysis_>=16){
        const auto end=player_.spectrumSamples(spectrumSamples_);
        if(player_.playing()&&end!=spectrumLastSample_){
            spectrum_.analyze(spectrumSamples_,player_.rate());spectrumLastSample_=end;spectrumLastAudio_=now;
            spectrogram_.push(spectrum_.detailLeft,spectrum_.detailRight,spectrogramTimeMs_);
        }else if(!player_.playing()||now-spectrumLastAudio_>250){spectrum_.left.fill(0);spectrum_.right.fill(0);}
        spectrumLastAnalysis_=now;
    }
    const double elapsed=spectrumLastPaint_?double(now-spectrumLastPaint_)/1000.:1./60.;
    spectrumMotion_.step(spectrum_.left,spectrum_.right,elapsed);spectrumLastPaint_=now;
    spectrumPeak_=0;
    for(size_t i=0;i<SpectrumAnalyzer::Bands;++i)spectrumPeak_=std::max({spectrumPeak_,spectrumMotion_.left[i],spectrumMotion_.right[i]});
    const int top=recording_?230:280,bottom=recording_?1030:990;
    if(spectrumStyle_==3){paintScrollingSpectrum(dc,top,bottom);return;}
    const int gap=28,row=(bottom-top-gap)/2;
    // Each channel gets its own horizontal frequency axis, matching the user's
    // two-row reference. There is no image path/decoder/background feature.
    for(int channel=0;channel<2;++channel){
        const int y0=top+channel*(row+gap),baseline=y0+row-4;
        const auto& levels=channel?spectrumMotion_.right:spectrumMotion_.left;
        if(spectrumStyle_==0){
            block(dc,{54,baseline,1866,baseline+2},GetSysColorBrush(COLOR_GRAYTEXT));
            for(size_t i=0;i<SpectrumAnalyzer::Bands;++i){
                int x=54+int(i*1812/SpectrumAnalyzer::Bands),next=54+int((i+1)*1812/SpectrumAnalyzer::Bands);
                int height=int(levels[i]*(row-12));
                block(dc,{x,baseline-height,std::max(x+1,next-4),baseline},palette.brushes[i]);
            }
            continue;
        }
        // Smooth single trace; the removed perspective/history style is absent.
        constexpr int Points=256;
        POINT previous{};
        for(int point=0;point<Points;++point){
            float fraction=float(point)/(Points-1);
            POINT current{LONG(std::lround(54.f+fraction*1812.f)),LONG(std::lround(baseline-spectrumCurve(levels,fraction)*(row-14)*.92f))};
            if(point)segments_.push_back({previous,current,RGB(184,213,235)});
            previous=current;
        }
        block(dc,{54,baseline,1866,baseline+2},palette.line);
    }
}
void Window::paintScrollingSpectrum(HDC dc,int top,int bottom){
    constexpr int Width=int(SpectrogramHistory::Columns)+1,Height=int(SpectrogramHistory::Bins);
    if(spectrogramRaster_.size()!=size_t(Width*Height*2))spectrogramRaster_.resize(Width*Height*2);
    static const auto colours=[](){
        std::array<uint32_t,256> result{};
        for(int i=0;i<256;++i){
            float v=i/255.f;
            int r=int(12+220*std::pow(v,2.2f)),g=int(16+226*std::pow(v,1.7f)),b=int(24+224*std::pow(v,1.2f));
            result[i]=uint32_t((r<<16)|(g<<8)|b);
        }
        return result;
    }();
    const uint64_t current=spectrogramTimeMs_/SpectrogramHistory::StepMs;
    for(int x=0;x<Width;++x){
        int age=std::max(0,Width-2-x);
        const auto* c=current>=uint64_t(age)?spectrogram_.column(current-age):nullptr;
        for(int y=0;y<Height;++y){
            int bin=Height-1-y;
            spectrogramRaster_[y*Width+x]=colours[c?c->left[bin]:0];
            spectrogramRaster_[(Height+y)*Width+x]=colours[c?c->right[bin]:0];
        }
    }
    BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth=Width;info.bmiHeader.biHeight=-Height;info.bmiHeader.biPlanes=1;
    info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
    const int gap=28,row=(bottom-top-gap)/2;
    const double columnWidth=1812./SpectrogramHistory::Columns;
    const int shift=int(std::lround(double(spectrogramTimeMs_%SpectrogramHistory::StepMs)/SpectrogramHistory::StepMs*columnWidth));
    for(int channel=0;channel<2;++channel){
        int y=top+channel*(row+gap),saved=SaveDC(dc);
        IntersectClipRect(dc,54,y,1866,y+row);
        SetStretchBltMode(dc,HALFTONE);SetBrushOrgEx(dc,0,0,nullptr);
        StretchDIBits(dc,54-shift,y,1812+int(std::ceil(columnWidth)),row,0,0,Width,Height,
                      spectrogramRaster_.data()+channel*Width*Height,&info,DIB_RGB_COLORS,SRCCOPY);
        RestoreDC(dc,saved);
    }
}
void Window::toggleSpectrum(){
    spectrumMode_=!spectrumMode_;spectrumLastPaint_=0;
    if(spectrumMode_){spectrogram_.clear();spectrogramTimeMs_=0;spectrumLastSample_=0;}
    savePreferences();InvalidateRect(hwnd_,nullptr,FALSE);
}
void Window::setSpectrumStyle(int style){
    if(style!=0&&style!=1&&style!=3)return;
    if(!spectrumMode_){spectrogram_.clear();spectrogramTimeMs_=0;spectrumLastPaint_=0;spectrumLastSample_=0;}
    spectrumStyle_=style;spectrumMode_=true;
    savePreferences();updateRemote();InvalidateRect(hwnd_,nullptr,FALSE);
}
void Window::loadVisualPreferences(){
    if(!preferencesEnabled_)return;
    spectrumMode_=preferences_.get("spectrumView",0,0,1)!=0;
    spectrumStyle_=preferences_.get("spectrumStyle",3,0,3);
    if(spectrumStyle_==2)spectrumStyle_=3; // Removed legacy perspective style.
}
}
