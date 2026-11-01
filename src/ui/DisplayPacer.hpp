#pragma once
#include <windows.h>
#include <dxgi1_2.h>
#include <wrl/client.h>
#include <atomic>
#include <thread>
#include <mutex>
#include <stdexcept>
#include <algorithm>
namespace ngmv {
// Wait on the output containing THIS window, independently of audio rate.
// At most one pending UI request; occluded/minimized windows don't redraw.
class DisplayPacer {
    HANDLE stop_=nullptr;std::thread worker_;std::atomic<bool> pending_{false};
    std::atomic<unsigned> vblanks_{0},fallbacks_{0};
public:
    static constexpr UINT Message=WM_APP+58;
    ~DisplayPacer(){stop();}
    void acknowledge(){pending_=false;}
    unsigned vblanks()const{return vblanks_;}unsigned fallbacks()const{return fallbacks_;}
    void stop(){if(stop_){SetEvent(stop_);if(worker_.joinable())worker_.join();CloseHandle(stop_);stop_=nullptr;}}
    void start(HWND window,bool hiddenTest=false){stop();pending_=false;stop_=CreateEventW(nullptr,TRUE,FALSE,nullptr);if(!stop_)throw std::runtime_error("Display pacing event failed");worker_=std::thread([this,window,hiddenTest]{
        using Microsoft::WRL::ComPtr;static std::once_flag once;
        std::call_once(once,[]{auto module=GetModuleHandleW(L"dxgi.dll");if(module){auto disable=reinterpret_cast<HRESULT(WINAPI*)()>(GetProcAddress(module,"DXGIDisableVBlankVirtualization"));if(disable)disable();}});
        ComPtr<IDXGIFactory1>factory;CreateDXGIFactory1(IID_PPV_ARGS(&factory));ComPtr<IDXGIOutput>output;HMONITOR current=nullptr;
        while(WaitForSingleObject(stop_,0)!=WAIT_OBJECT_0){
            if(!hiddenTest&&(!IsWindowVisible(window)||IsIconic(window))){WaitForSingleObject(stop_,100);continue;}
            if(factory&&!factory->IsCurrent()){factory.Reset();CreateDXGIFactory1(IID_PPV_ARGS(&factory));output.Reset();current=nullptr;}
            HMONITOR monitor=MonitorFromWindow(window,MONITOR_DEFAULTTONEAREST);
            if(monitor!=current||!output){output.Reset();current=monitor;if(factory){for(UINT i=0;!output;i++){ComPtr<IDXGIAdapter1>adapter;if(FAILED(factory->EnumAdapters1(i,&adapter)))break;for(UINT j=0;;j++){ComPtr<IDXGIOutput>candidate;if(FAILED(adapter->EnumOutputs(j,&candidate)))break;DXGI_OUTPUT_DESC desc{};if(SUCCEEDED(candidate->GetDesc(&desc))&&desc.AttachedToDesktop&&desc.Monitor==monitor){output=candidate;break;}}}}}
            if(output&&SUCCEEDED(output->WaitForVBlank()))++vblanks_;
            else {++fallbacks_;MONITORINFOEXW info{};info.cbSize=sizeof(info);DEVMODEW mode{};mode.dmSize=sizeof(mode);DWORD ms=16;if(GetMonitorInfoW(monitor,&info)&&EnumDisplaySettingsW(info.szDevice,ENUM_CURRENT_SETTINGS,&mode)&&mode.dmDisplayFrequency>1)ms=std::max(DWORD(1),DWORD(1000/mode.dmDisplayFrequency));if(WaitForSingleObject(stop_,ms)==WAIT_OBJECT_0)break;}
            if(WaitForSingleObject(stop_,0)==WAIT_OBJECT_0)break;
            if(!pending_.exchange(true)&&!PostMessageW(window,Message,0,0))pending_=false;
        }
    });}
};
}
