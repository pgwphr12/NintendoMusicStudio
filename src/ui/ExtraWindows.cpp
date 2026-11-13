#include "Window.hpp"
#include <commctrl.h>
#include <stdexcept>
#include <string>
namespace ngmv {
std::wstring wide(const std::string&);
static HWND control(HWND owner,const wchar_t* cls,const wchar_t* label,DWORD style,int id,int x,int y,int w,int h){
 auto c=CreateWindowExW(0,cls,label,WS_CHILD|WS_VISIBLE|style,x,y,w,h,owner,reinterpret_cast<HMENU>(INT_PTR(id)),GetModuleHandleW(nullptr),nullptr);
 SendMessageW(c,WM_SETFONT,reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),TRUE);return c;
}
static void paintToolClient(HWND h,HDC dc){
 static thread_local bool printing=false;
 RECT r{};GetClientRect(h,&r);FillRect(dc,&r,GetSysColorBrush(COLOR_BTNFACE));
 if(printing)return;printing=true;
 struct Context{HWND parent;HDC dc;};Context context{h,dc};
 EnumChildWindows(h,[](HWND child,LPARAM data)->BOOL{
  auto c=reinterpret_cast<Context*>(data);if(GetParent(child)!=c->parent)return TRUE;
  RECT r{};GetWindowRect(child,&r);MapWindowPoints(nullptr,c->parent,reinterpret_cast<POINT*>(&r),2);
  int saved=SaveDC(c->dc);SetViewportOrgEx(c->dc,r.left,r.top,nullptr);IntersectClipRect(c->dc,0,0,r.right-r.left,r.bottom-r.top);
  SendMessageW(child,WM_PRINT,c->dc?reinterpret_cast<WPARAM>(c->dc):0,PRF_CLIENT|PRF_ERASEBKGND|PRF_CHILDREN);RestoreDC(c->dc,saved);return TRUE;
 },reinterpret_cast<LPARAM>(&context));
 printing=false;
}
static std::wstring clockText(double v){int n=std::max(0,int(v));return std::to_wstring(n/60)+L":"+(n%60<10?L"0":L"")+std::to_wstring(n%60);}
void Window::setRecording(bool v){recording_=v;updateRemote();}
void Window::updateRemote(){
 if(!remoteWindow_||!IsWindow(remoteWindow_))return;
 SetWindowTextW(GetDlgItem(remoteWindow_,2106),recording_?L"녹화뷰 끄기 (F9)":L"녹화뷰 켜기 (F9)");
 std::wstring title=player_.loaded()?wide(player_.tracks()[player_.track()].title):L"음악을 열어 주세요";
 SetWindowTextW(GetDlgItem(remoteWindow_,2100),(clockText(player_.seconds())+L"   "+title).c_str());
 SetWindowTextW(GetDlgItem(remoteWindow_,2102),player_.playing()?L"일시정지":L"재생");
 SetWindowTextW(GetDlgItem(remoteWindow_,2105),player_.repeat()?L"반복 켜짐":L"반복 꺼짐");
 SetWindowTextW(GetDlgItem(remoteWindow_,2109),spectrumStyle_==0?L"종류: 막대형… (F7)":spectrumStyle_==1?L"종류: 선형… (F7)":L"종류: 가로흐름… (F7)");
 SendDlgItemMessageW(remoteWindow_,2115,BM_SETCHECK,player_.showAllChannels()?BST_CHECKED:BST_UNCHECKED,0);
 SetWindowTextW(GetDlgItem(remoteWindow_,2114),(L"숨긴 채널 ("+std::to_wstring(hiddenChannelCount())+L")…").c_str());
 float common=player_.commonChannelVolume();auto states=player_.states();if(common<0)common=states.empty()?1:states[0].volume;
 int volume=int(std::lround(common*100));SendDlgItemMessageW(remoteWindow_,2110,TBM_SETPOS,TRUE,volume);
 bool mixed=std::any_of(states.begin(),states.end(),[&](const auto& s){return std::abs(s.volume-states.front().volume)>.001f;});
 SetWindowTextW(GetDlgItem(remoteWindow_,2111),(mixed?std::wstring(L"모든 채널 혼합"):L"모든 채널 "+std::to_wstring(states.empty()?volume:int(std::lround(states.front().volume*100)))+L"%").c_str());
 int master=int(std::lround(player_.master()*100));SendDlgItemMessageW(remoteWindow_,2112,TBM_SETPOS,TRUE,master);
 SetWindowTextW(GetDlgItem(remoteWindow_,2113),(L"마스터 "+std::to_wstring(master)+L"%").c_str());
 int zoomLevel=int(std::lround(std::log2(4096.0/std::max(256,windowSamples_))));
 zoomLevel=std::clamp(zoomLevel,0,4);
 SendDlgItemMessageW(remoteWindow_,2116,TBM_SETPOS,TRUE,zoomLevel);
 std::wstring zoomText=zoomLevel==0?L"0.5×":std::to_wstring(1<<(zoomLevel-1))+L"×";
 SetWindowTextW(GetDlgItem(remoteWindow_,2118),(L"파형 간격: "+zoomText).c_str());
}
void Window::toggleRemote(){
 if(remoteWindow_&&IsWindowVisible(remoteWindow_))ShowWindow(remoteWindow_,SW_HIDE);
 else showRemote();
 InvalidateRect(hwnd_,nullptr,FALSE);
}
void Window::showRemote(){
 if(!hwnd_)return;
 if(remoteWindow_&&IsWindow(remoteWindow_)){ShowWindow(remoteWindow_,screenshot_.empty()?SW_SHOWNORMAL:SW_SHOWNOACTIVATE);updateRemote();return;}
 INITCOMMONCONTROLSEX init{sizeof(init),ICC_BAR_CLASSES};InitCommonControlsEx(&init);
 WNDCLASSW wc{};wc.hInstance=GetModuleHandleW(nullptr);wc.lpszClassName=L"NintendoMusicStudioRemote";wc.hCursor=LoadCursorW(nullptr,IDC_ARROW);wc.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_BTNFACE+1);
 wc.lpfnWndProc=[](HWND h,UINT m,WPARAM a,LPARAM b)->LRESULT{
  auto self=reinterpret_cast<Window*>(GetWindowLongPtrW(h,GWLP_USERDATA));if(m==WM_NCCREATE){self=static_cast<Window*>(reinterpret_cast<CREATESTRUCTW*>(b)->lpCreateParams);SetWindowLongPtrW(h,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(self));}
  if(m==WM_CREATE){
   control(h,L"STATIC",L"",0,2100,14,12,510,28);
   const wchar_t*ko[]={L"이전 곡",L"재생",L"정지",L"다음 곡",L"반복"};
   for(int i=0;i<5;i++)control(h,L"BUTTON",ko[i],WS_TABSTOP,2101+i,14+102*i,48,94,34);
   control(h,L"STATIC",L"",0,2111,14,96,145,24);auto v=control(h,TRACKBAR_CLASSW,L"",WS_TABSTOP|TBS_HORZ,2110,158,88,360,36);SendMessageW(v,TBM_SETRANGE,TRUE,MAKELPARAM(0,200));
   control(h,L"STATIC",L"",0,2113,14,136,145,24);v=control(h,TRACKBAR_CLASSW,L"",WS_TABSTOP|TBS_HORZ,2112,158,128,360,36);SendMessageW(v,TBM_SETRANGE,TRUE,MAKELPARAM(0,100));
   control(h,L"BUTTON",L"20밴드 EQ…",WS_TABSTOP,2107,14,178,180,34);control(h,L"BUTTON",L"녹화뷰 전환 (F9)",WS_TABSTOP,2106,210,178,230,34);
   control(h,L"BUTTON",L"파형 / 스펙트럼 (F6)",WS_TABSTOP,2108,14,218,240,30);
   control(h,L"BUTTON",L"스펙트럼 종류 (F7)",WS_TABSTOP,2109,270,218,240,30);
   control(h,L"BUTTON",L"숨긴 채널…",WS_TABSTOP,2114,14,258,496,30);
   control(h,L"BUTTON",L"모든 채널 표시",WS_TABSTOP|BS_AUTOCHECKBOX,2115,14,298,496,26);
   control(h,L"STATIC",L"파형 간격: 2×",0,2118,14,337,136,24);
   auto zoom=control(h,TRACKBAR_CLASSW,L"",WS_TABSTOP|TBS_HORZ|TBS_AUTOTICKS,2116,158,327,360,34);
   SendMessageW(zoom,TBM_SETRANGE,TRUE,MAKELPARAM(0,4));SendMessageW(zoom,TBM_SETTICFREQ,1,0);SendMessageW(zoom,TBM_SETLINESIZE,0,1);SendMessageW(zoom,TBM_SETPAGESIZE,0,1);
   SetTimer(h,1,200,nullptr);return 0;
  }
  if(!self)return DefWindowProcW(h,m,a,b);
  if(m==WM_PRINT||m==WM_PRINTCLIENT){paintToolClient(h,reinterpret_cast<HDC>(a));return 0;}
  if(m==WM_TIMER){self->updateRemote();return 0;}
  if(m==WM_COMMAND&&HIWORD(a)==BN_CLICKED&&!self->changing_){try{switch(LOWORD(a)){
   case IDCANCEL:self->toggleRemote();break;
   case 2101:self->nextTrack(-1);break;case 2102:if(self->player_.playing())self->player_.pause();else self->player_.play();break;
   case 2103:self->player_.stop();break;case 2104:self->nextTrack(1);break;case 2105:self->player_.repeat(!self->player_.repeat());break;
   case 2106:PostMessageW(self->hwnd_,WM_KEYDOWN,VK_F9,0);break;case 2107:self->showEqualizer();break;case 2108:self->toggleSpectrum();break;case 2109:self->showSpectrumChoices();break;case 2114:self->showHiddenChannels();break;case 2115:self->player_.showAllChannels(SendDlgItemMessageW(h,2115,BM_GETCHECK,0,0)==BST_CHECKED);self->savePreferences();break;
  }self->updateRemote();InvalidateRect(self->hwnd_,nullptr,FALSE);}catch(const std::exception&e){self->report(e);}return 0;}
  if(m==WM_HSCROLL&&!self->changing_){int id=GetDlgCtrlID(reinterpret_cast<HWND>(b));int v=int(SendMessageW(reinterpret_cast<HWND>(b),TBM_GETPOS,0,0));if(id==2110){self->setAllChannelVolume(v/100.f);self->savePreferences();self->allVolumeDirty_=false;}else if(id==2112){self->player_.master(v/100.f);self->savePreferences();}else if(id==2116){self->windowSamples_=4096>>std::clamp(v,0,4);self->savePreferences();InvalidateRect(self->hwnd_,nullptr,FALSE);}self->updateRemote();return 0;}
  if(m==WM_CLOSE){ShowWindow(h,SW_HIDE);InvalidateRect(self->hwnd_,nullptr,FALSE);return 0;}if(m==WM_DESTROY){KillTimer(h,1);self->remoteWindow_=nullptr;return 0;}
  return DefWindowProcW(h,m,a,b);
 };
 if(!RegisterClassW(&wc)&&GetLastError()!=ERROR_CLASS_ALREADY_EXISTS)throw std::runtime_error("Cannot create recording remote.");
 RECT r{};GetWindowRect(hwnd_,&r);remoteWindow_=CreateWindowExW(WS_EX_TOOLWINDOW|WS_EX_CONTROLPARENT,wc.lpszClassName,L"Nintendo Music Studio · 리모컨",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,r.right-570,r.bottom-450,560,430,hwnd_,nullptr,wc.hInstance,this);
 if(!remoteWindow_)throw std::runtime_error("Cannot open recording remote.");updateRemote();ShowWindow(remoteWindow_,screenshot_.empty()?SW_SHOWNORMAL:SW_SHOWNOACTIVATE);
}
void Window::showSpectrumChoices(){
 auto sync=[&]{CheckRadioButton(spectrumChoiceWindow_,2600,2602,spectrumStyle_==3?2602:2600+spectrumStyle_);};
 if(spectrumChoiceWindow_&&IsWindow(spectrumChoiceWindow_)){
  sync();ShowWindow(spectrumChoiceWindow_,screenshot_.empty()?SW_SHOWNORMAL:SW_SHOWNOACTIVATE);
  if(screenshot_.empty())SetForegroundWindow(spectrumChoiceWindow_);return;
 }
 WNDCLASSW wc{};wc.hInstance=GetModuleHandleW(nullptr);wc.lpszClassName=L"NintendoMusicStudioSpectrumChoices";
 wc.hCursor=LoadCursorW(nullptr,IDC_ARROW);wc.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_BTNFACE+1);
 wc.lpfnWndProc=[](HWND h,UINT m,WPARAM a,LPARAM b)->LRESULT{
  auto self=reinterpret_cast<Window*>(GetWindowLongPtrW(h,GWLP_USERDATA));
  if(m==WM_NCCREATE){self=static_cast<Window*>(reinterpret_cast<CREATESTRUCTW*>(b)->lpCreateParams);SetWindowLongPtrW(h,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(self));}
  if(m==WM_CREATE){
   control(h,L"STATIC",L"원하는 표시를 선택하세요. 바로 적용됩니다.",0,2610,20,16,420,24);
   control(h,L"BUTTON",L"막대형 — 주파수별 막대",BS_AUTORADIOBUTTON|WS_GROUP|WS_TABSTOP,2600,20,52,420,36);
   control(h,L"BUTTON",L"선형 — 부드러운 곡선",BS_AUTORADIOBUTTON|WS_TABSTOP,2601,20,94,420,36);
   control(h,L"BUTTON",L"가로흐름 — 옆으로 흐르는 스펙트로그램",BS_AUTORADIOBUTTON|WS_TABSTOP,2602,20,136,420,36);
   control(h,L"BUTTON",L"닫기",WS_TABSTOP|WS_GROUP,IDCANCEL,330,192,110,32);return 0;
  }
  if(!self)return DefWindowProcW(h,m,a,b);
  if(m==WM_COMMAND&&HIWORD(a)==BN_CLICKED){
   int id=LOWORD(a);if(id==IDCANCEL){ShowWindow(h,SW_HIDE);return 0;}
   if(id>=2600&&id<=2602){try{self->setSpectrumStyle(id==2602?3:id-2600);CheckRadioButton(h,2600,2602,id);}catch(const std::exception&e){self->report(e);}return 0;}
  }
  if(m==WM_CLOSE){ShowWindow(h,SW_HIDE);return 0;}
  if(m==WM_DESTROY){self->spectrumChoiceWindow_=nullptr;return 0;}
  return DefWindowProcW(h,m,a,b);
 };
 if(!RegisterClassW(&wc)&&GetLastError()!=ERROR_CLASS_ALREADY_EXISTS)throw std::runtime_error("Cannot create spectrum choices.");
 RECT owner{};GetWindowRect(hwnd_,&owner);RECT rect{0,0,460,242};
 AdjustWindowRectEx(&rect,WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,FALSE,WS_EX_TOOLWINDOW|WS_EX_CONTROLPARENT);
 spectrumChoiceWindow_=CreateWindowExW(WS_EX_TOOLWINDOW|WS_EX_CONTROLPARENT,wc.lpszClassName,L"스펙트럼 종류",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,
   owner.left+40,owner.top+70,rect.right-rect.left,rect.bottom-rect.top,hwnd_,nullptr,wc.hInstance,this);
 if(!spectrumChoiceWindow_)throw std::runtime_error("Cannot open spectrum choices.");
 SetWindowTextW(spectrumChoiceWindow_,L"스펙트럼 종류");sync();
 ShowWindow(spectrumChoiceWindow_,screenshot_.empty()?SW_SHOWNORMAL:SW_SHOWNOACTIVATE);
 if(screenshot_.empty())SetFocus(GetDlgItem(spectrumChoiceWindow_,spectrumStyle_==3?2602:2600+spectrumStyle_));
}
void Window::updateEqualizer(bool force){
 if(!equalizerWindow_||!IsWindow(equalizerWindow_))return;
 equalizerSync_=true;
 bool enabled=player_.equalizerEnabled();CheckDlgButton(equalizerWindow_,2200,enabled?BST_CHECKED:BST_UNCHECKED);
 for(size_t i=0;i<20;i++){
  int gain=player_.equalizerGain(i)/10;auto slider=GetDlgItem(equalizerWindow_,2300+int(i)),edit=GetDlgItem(equalizerWindow_,2400+int(i));
  SendMessageW(slider,TBM_SETPOS,TRUE,-gain);
  if(force||GetFocus()!=edit)SetWindowTextW(edit,std::to_wstring(gain).c_str());
  EnableWindow(slider,enabled);EnableWindow(edit,enabled);EnableWindow(GetDlgItem(equalizerWindow_,2500+int(i)),enabled);
 }
 equalizerSync_=false;
}
void Window::showEqualizer(){
 if(equalizerWindow_&&IsWindow(equalizerWindow_)){ShowWindow(equalizerWindow_,screenshot_.empty()?SW_SHOWNORMAL:SW_SHOWNOACTIVATE);return;}
 INITCOMMONCONTROLSEX init{sizeof(init),ICC_BAR_CLASSES};InitCommonControlsEx(&init);
 WNDCLASSW wc{};wc.hInstance=GetModuleHandleW(nullptr);wc.lpszClassName=L"NintendoMusicStudioEqualizer";wc.hCursor=LoadCursorW(nullptr,IDC_ARROW);wc.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_BTNFACE+1);
 wc.lpfnWndProc=[](HWND h,UINT m,WPARAM a,LPARAM b)->LRESULT{
  auto self=reinterpret_cast<Window*>(GetWindowLongPtrW(h,GWLP_USERDATA));if(m==WM_NCCREATE){self=static_cast<Window*>(reinterpret_cast<CREATESTRUCTW*>(b)->lpCreateParams);SetWindowLongPtrW(h,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(self));}
  if(m==WM_CREATE){
   control(h,L"BUTTON",L"EQ 사용",BS_AUTOCHECKBOX|WS_TABSTOP,2200,14,10,150,26);
   for(int i=0;i<20;i++){int x=14+(i%10)*60,y=i<10?45:270;auto slider=control(h,TRACKBAR_CLASSW,L"",TBS_VERT|WS_TABSTOP|TBS_AUTOTICKS,2300+i,x+10,y,36,150);SendMessageW(slider,TBM_SETRANGE,TRUE,MAKELPARAM(-20,20));SendMessageW(slider,TBM_SETTICFREQ,10,0);SendMessageW(slider,TBM_SETLINESIZE,0,1);SendMessageW(slider,TBM_SETPAGESIZE,0,5);
    auto edit=control(h,L"EDIT",L"0",ES_CENTER|WS_BORDER|WS_TABSTOP,2400+i,x,y+153,58,22);
    SendMessageW(edit,EM_SETLIMITTEXT,3,0);
    SetWindowSubclass(edit,[](HWND e,UINT message,WPARAM a,LPARAM b,UINT_PTR id,DWORD_PTR)->LRESULT{
     if(message==WM_SETFOCUS){auto result=DefSubclassProc(e,message,a,b);SendMessageW(e,EM_SETSEL,0,-1);return result;}
     if(message==WM_KEYDOWN&&a=='A'&&(GetKeyState(VK_CONTROL)&0x8000)){SendMessageW(e,EM_SETSEL,0,-1);return 0;}
     if(message==WM_CHAR&&a>=32&&!((a>='0'&&a<='9')||a=='-'))return 0;
     (void)id;
     return DefSubclassProc(e,message,a,b);
    },1,0);
    int hz=GraphicEqualizer::frequencies[i];std::wstring label=hz>=1000?std::to_wstring(hz/1000.) : std::to_wstring(hz);if(hz>=1000){wchar_t value[24];swprintf_s(value,L"%.1f kHz",hz/1000.);label=value;}else label+=L" Hz";control(h,L"STATIC",label.c_str(),SS_CENTER,2500+i,x,y+179,58,24);
   }
   control(h,L"BUTTON",L"모두 0 dB로 초기화",WS_TABSTOP,2201,14,488,200,30);control(h,L"BUTTON",L"닫기",WS_TABSTOP,IDCANCEL,510,488,100,30);return 0;
  }
  if(!self)return DefWindowProcW(h,m,a,b);
  if(m==WM_PRINT||m==WM_PRINTCLIENT){paintToolClient(h,reinterpret_cast<HDC>(a));return 0;}
  if(m==WM_COMMAND&&LOWORD(a)>=2400&&LOWORD(a)<2420&&!self->equalizerSync_){
   if(HIWORD(a)==EN_CHANGE||HIWORD(a)==EN_KILLFOCUS){
    wchar_t text[32]{};GetWindowTextW(reinterpret_cast<HWND>(b),text,32);wchar_t*end=nullptr;long value=wcstol(text,&end,10);
    while(end&&(*end==L' '||*end==L'\t'||*end==L'\r'||*end==L'\n'))end++;
    bool blank=wcsspn(text,L" \t\r\n")==wcslen(text);
    bool valid=blank||(end!=text&&*end==0);
    if(valid){if(blank)value=0;else if(value==0&&text[0]==L'-')value=-1;self->player_.equalizerGain(LOWORD(a)-2400,int(std::clamp(value,-20L,20L))*10);}
    self->updateEqualizer(HIWORD(a)==EN_KILLFOCUS);if(valid)self->savePreferences();
   }return 0;
  }
  if(m==WM_COMMAND&&HIWORD(a)==BN_CLICKED){if(LOWORD(a)==IDCANCEL){ShowWindow(h,SW_HIDE);return 0;}if(LOWORD(a)==2200)self->player_.equalizerEnabled(IsDlgButtonChecked(h,2200)==BST_CHECKED);else if(LOWORD(a)==2201)for(size_t i=0;i<20;i++)self->player_.equalizerGain(i,0);self->updateEqualizer();self->savePreferences();return 0;}
  if(m==WM_VSCROLL){int id=GetDlgCtrlID(reinterpret_cast<HWND>(b));if(id>=2300&&id<2320){self->player_.equalizerGain(id-2300,-10*int(SendMessageW(reinterpret_cast<HWND>(b),TBM_GETPOS,0,0)));self->updateEqualizer();if(LOWORD(a)==TB_ENDTRACK)self->savePreferences();}return 0;}
  if(m==WM_CLOSE){self->savePreferences();ShowWindow(h,SW_HIDE);return 0;}if(m==WM_DESTROY){self->equalizerWindow_=nullptr;return 0;}
  return DefWindowProcW(h,m,a,b);
 };
 if(!RegisterClassW(&wc)&&GetLastError()!=ERROR_CLASS_ALREADY_EXISTS)throw std::runtime_error("Cannot create equalizer.");RECT r{};GetWindowRect(hwnd_,&r);
 equalizerSync_=true;
 equalizerWindow_=CreateWindowExW(WS_EX_TOOLWINDOW|WS_EX_CONTROLPARENT,wc.lpszClassName,L"Nintendo Music Studio · 20밴드 이퀄라이저",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,r.left+40,r.top+40,650,570,hwnd_,nullptr,wc.hInstance,this);
 equalizerSync_=false;
 if(!equalizerWindow_)throw std::runtime_error("Cannot open equalizer.");updateEqualizer();ShowWindow(equalizerWindow_,screenshot_.empty()?SW_SHOWNORMAL:SW_SHOWNOACTIVATE);
}
}

