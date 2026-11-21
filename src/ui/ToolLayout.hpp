#pragma once
#include <windows.h>
#include <map>
#include <algorithm>
namespace ngmv {
// Native tools use 96-DPI layout units and an explicit matching font, rather
// than mixing fixed pixels with DEFAULT_GUI_FONT's system-scaled metrics.
inline UINT toolDpi(HWND window){
 using Query=UINT(WINAPI*)(HWND);static auto query=reinterpret_cast<Query>(GetProcAddress(GetModuleHandleW(L"user32.dll"),"GetDpiForWindow"));if(query)return query(window);
 HDC dc=GetDC(window);UINT dpi=dc?UINT(GetDeviceCaps(dc,LOGPIXELSX)):96;if(dc)ReleaseDC(window,dc);return dpi;
}
inline RECT toolWorkArea(HWND window){
 MONITORINFO info{sizeof(info)};
 if(GetMonitorInfoW(MonitorFromWindow(window,MONITOR_DEFAULTTONEAREST),&info))return info.rcWork;
 RECT work{};SystemParametersInfoW(SPI_GETWORKAREA,0,&work,0);return work;
}
inline RECT clampToolRect(RECT r,const RECT& work){
 LONG width=r.right-r.left,height=r.bottom-r.top;
 r.left=std::clamp(r.left,work.left,std::max(work.left,work.right-width));
 r.top=std::clamp(r.top,work.top,std::max(work.top,work.bottom-height));
 r.right=r.left+width;r.bottom=r.top+height;return r;
}
inline void placeToolVisible(HWND window){
 RECT before{};GetWindowRect(window,&before);auto r=clampToolRect(before,toolWorkArea(window));
 SetWindowPos(window,nullptr,r.left,r.top,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
}
inline void fitToolWindow(HWND window,int width,int height){
 const UINT dpi=toolDpi(window);const UINT actual=dpi?dpi:96;
 using Adjust=BOOL(WINAPI*)(LPRECT,DWORD,BOOL,DWORD,UINT);static auto adjust=reinterpret_cast<Adjust>(GetProcAddress(GetModuleHandleW(L"user32.dll"),"AdjustWindowRectExForDpi"));
 auto outer=[&](RECT& r){if(adjust)adjust(&r,DWORD(GetWindowLongPtrW(window,GWL_STYLE)),FALSE,DWORD(GetWindowLongPtrW(window,GWL_EXSTYLE)),actual);else AdjustWindowRectEx(&r,DWORD(GetWindowLongPtrW(window,GWL_STYLE)),FALSE,DWORD(GetWindowLongPtrW(window,GWL_EXSTYLE)));};
 RECT border{};outer(border);auto work=toolWorkArea(window);
 int availableW=work.right-work.left-(border.right-border.left),availableH=work.bottom-work.top-(border.bottom-border.top);
 const UINT d=UINT(std::max(1,std::min({int(actual),availableW*96/std::max(1,width),availableH*96/std::max(1,height)})));
 struct Fonts {std::map<UINT,HFONT> values;~Fonts(){for(auto &entry:values)DeleteObject(entry.second);}};
 static Fonts fonts;auto &font=fonts.values[d];if(!font)font=CreateFontW(-MulDiv(12,d,96),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"맑은 고딕");
 struct Context {HWND window;UINT dpi;HFONT font;};Context ctx{window,d,font};
 EnumChildWindows(window,[](HWND child,LPARAM data)->BOOL{
  auto &c=*reinterpret_cast<Context*>(data);if(GetParent(child)!=c.window)return TRUE;
  RECT r{};GetWindowRect(child,&r);MapWindowPoints(nullptr,c.window,reinterpret_cast<POINT*>(&r),2);
  SetWindowPos(child,nullptr,MulDiv(r.left,c.dpi,96),MulDiv(r.top,c.dpi,96),MulDiv(r.right-r.left,c.dpi,96),MulDiv(r.bottom-r.top,c.dpi,96),SWP_NOZORDER|SWP_NOACTIVATE);
  SendMessageW(child,WM_SETFONT,reinterpret_cast<WPARAM>(c.font),TRUE);return TRUE;
 },reinterpret_cast<LPARAM>(&ctx));
 RECT r{0,0,MulDiv(width,d,96),MulDiv(height,d,96)};
 outer(r);
 SetWindowPos(window,nullptr,0,0,r.right-r.left,r.bottom-r.top,SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
 placeToolVisible(window);
}
}
