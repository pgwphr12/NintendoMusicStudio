#include "Window.hpp"
#include "ToolLayout.hpp"
#include "UiLanguage.hpp"
#include "audio/StemExport.hpp"
#include "app/WindowsIntegration.hpp"
#include "formats/StreamBackend.hpp"
#include <commctrl.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <wrl/client.h>
#include <cwchar>
#include <cwctype>
#include <stdexcept>
#include <string>
namespace ngmv {
std::wstring wide(const std::string&);
static HWND control(HWND owner,const wchar_t* cls,const wchar_t* label,DWORD style,int id,int x,int y,int w,int h){
 auto c=CreateWindowExW(0,cls,label,WS_CHILD|WS_VISIBLE|style,x,y,w,h,owner,reinterpret_cast<HMENU>(INT_PTR(id)),GetModuleHandleW(nullptr),nullptr);
 setUiText(c,label);
 SendMessageW(c,WM_SETFONT,reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),TRUE);return c;
}
static void paintToolClient(HWND h,HDC dc,bool children){
 static thread_local bool printing=false;
 RECT r{};GetClientRect(h,&r);FillRect(dc,&r,GetSysColorBrush(COLOR_BTNFACE));
 if(printing||!children)return;printing=true;
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
void Window::syncExportTrack(){
 if(!exportWindow_||!player_.loaded()||exportRunning_)return;
 int track=player_.track();if(track<0||size_t(track)>=player_.tracks().size())return;if(file_==exportInput_&&track==exportTrack_)return;
 exportInput_=file_;exportTrack_=track;int ms=player_.tracks().at(size_t(track)).lengthMs;
 setUiDlgText(exportWindow_,2703,ms>0?std::to_wstring((ms+999)/1000).c_str():L"180");setUiDlgText(exportWindow_,2705,L"0");
 if(desktop::automatedSession()){wchar_t n[8]{};if(GetEnvironmentVariableW(L"NGMV_TEST_EXPORT_REPEATS",n,8))setUiDlgText(exportWindow_,2705,n);}
}
void Window::startExport(){
 if(exportRunning_||!player_.loaded())return;
 syncExportTrack();
 wchar_t durationText[32]{},playsText[32]{};GetDlgItemTextW(exportWindow_,2703,durationText,32);GetDlgItemTextW(exportWindow_,2705,playsText,32);
 wchar_t* end=nullptr;double seconds=std::wcstod(durationText,&end);if(end==durationText||*end||!std::isfinite(seconds)||seconds<=0||seconds>3600)throw std::runtime_error("Enter a duration between 0 and 3600 seconds.");
 end=nullptr;unsigned long repeats=std::wcstoul(playsText,&end,10);if(end==playsText||*end||repeats>99)throw std::runtime_error("Enter a repeat count between 0 and 99.");
 std::filesystem::path output;
 if(desktop::automatedSession()){wchar_t testFolder[32768]{};auto n=GetEnvironmentVariableW(L"NGMV_TEST_EXPORT_DIR",testFolder,32768);if(n>0&&n<32768)output=testFolder;}
 if(output.empty()){
  Microsoft::WRL::ComPtr<IFileOpenDialog> dialog;
  HRESULT hr=CoCreateInstance(CLSID_FileOpenDialog,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(dialog.GetAddressOf()));
  if(FAILED(hr))throw std::runtime_error("Cannot open the folder selection window.");
  DWORD options=0;if(FAILED(dialog->GetOptions(&options))||FAILED(dialog->SetOptions(options|FOS_PICKFOLDERS|FOS_FORCEFILESYSTEM|FOS_PATHMUSTEXIST|FOS_NOCHANGEDIR)))throw std::runtime_error("Cannot configure folder selection.");
  const auto parent=std::filesystem::absolute(file_).parent_path();
  Microsoft::WRL::ComPtr<IShellItem> initial;
  hr=SHCreateItemFromParsingName(parent.c_str(),nullptr,IID_PPV_ARGS(initial.GetAddressOf()));
  if(FAILED(hr)||FAILED(dialog->SetFolder(initial.Get())))throw std::runtime_error("Cannot select the music folder.");
  dialog->SetTitle(translateUi(L"저장 위치 선택 — 선택한 위치에 "+file_.stem().wstring()+L" 폴더를 만듭니다").c_str());
  dialog->SetOkButtonLabel(translateUi(L"여기에 저장").c_str());
  hr=dialog->Show(exportWindow_);if(hr==HRESULT_FROM_WIN32(ERROR_CANCELLED))return;
  if(FAILED(hr))throw std::runtime_error("Folder selection failed.");
  Microsoft::WRL::ComPtr<IShellItem> selected;if(FAILED(dialog->GetResult(selected.GetAddressOf())))throw std::runtime_error("Cannot read the selected folder.");
  PWSTR name=nullptr;hr=selected->GetDisplayName(SIGDN_FILESYSPATH,&name);
  if(FAILED(hr))throw std::runtime_error("Cannot use the selected folder.");
  try{output=name;}catch(...){CoTaskMemFree(name);throw;}CoTaskMemFree(name);
 }
 const auto input=file_;const int track=player_.track();const uint64_t frames=uint64_t(std::llround(seconds*InternalRate));const bool mixed=SendDlgItemMessageW(exportWindow_,2701,CB_GETCURSEL,0,0)==1;
 if(exportThread_.joinable())exportThread_.join();exportCancel_=false;exportProgress_=0;exportRunning_=true;
 {std::lock_guard<std::mutex>lock(exportMutex_);exportMessage_=L"준비 중…";}
 try{exportThread_=std::thread([this,input,output,track,frames,repeats,mixed]{
  std::filesystem::path stage;bool owned=false;
  try{
   auto bytes=readMusicFile(input);uint64_t exportFrames=frames;auto format=identify(bytes,input);std::unique_ptr<IMusicBackend> backend;
   if(repeats&&format==Format::Vgm)bytes=expandVgmLoops(bytes,unsigned(repeats),exportFrames);
   else if(repeats&&(format==Format::Bcstm||format==Format::Bcwav||format==Format::GameStream)){
    const bool is3ds=format!=Format::GameStream;auto ext=input.extension().wstring();std::transform(ext.begin(),ext.end(),ext.begin(),[](wchar_t c){return std::towlower(c);});
    const char* system=is3ds?"Nintendo 3DS · Audio Stream":ext==L".brstm"?"Nintendo Wii · BRSTM":ext==L".bns"?"Nintendo Wii · BNS":"CRI ADX · Game Audio";
    backend=makeLoopingStreamBackend(input,system,true,is3ds,unsigned(repeats));if(!backend->decodedEndFrame(exportFrames)||!exportFrames)throw std::runtime_error("Cannot determine the looped stream duration.");
   }else if(repeats)throw std::runtime_error(u8"기록된 루프 반복은 현재 VGM/VGZ와 일부 게임 오디오 스트림만 지원합니다.");
   if(!backend)backend=makeBackend(bytes,input);
   for(unsigned i=0;i<1000;++i){stage=output/(L".nms-export-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64())+L"-"+std::to_wstring(i));if(std::filesystem::create_directory(stage)){owned=true;break;}}
   if(!owned)throw std::runtime_error("Cannot create an export staging folder.");
   auto result=exportStems(*backend,stage,{track,1,exportFrames,mixed},exportCancel_,[this](unsigned p){exportProgress_=p;});
   if(exportCancel_)throw std::runtime_error("Export cancelled");if(!result.written)throw std::runtime_error("No audible channels were found within the selected duration.");
   auto base=input.stem().wstring();std::filesystem::path final;
   for(unsigned i=0;i<1000;++i){final=output/(base+(i?L"_"+std::to_wstring(i):L""));if(!std::filesystem::exists(final)){std::filesystem::rename(stage,final);owned=false;break;}}
   if(owned)throw std::runtime_error("Cannot find an unused output folder name.");
   std::lock_guard<std::mutex>lock(exportMutex_);exportMessage_=L"완료: "+std::to_wstring(result.written)+L"개 WAV\n"+final.wstring();
  }catch(const std::exception&e){if(owned){std::error_code ec;std::filesystem::remove_all(stage,ec);}std::lock_guard<std::mutex>lock(exportMutex_);exportMessage_=exportCancel_.load()?L"내보내기를 취소했습니다.":L"내보내기 실패: "+wide(e.what());}
  exportRunning_=false;
 });}catch(...){exportRunning_=false;throw;}
}
void Window::showExport(){
 if(!player_.loaded())return;if(exportWindow_&&IsWindow(exportWindow_)){syncExportTrack();ShowWindow(exportWindow_,SW_SHOWNORMAL);SetForegroundWindow(exportWindow_);return;}
 WNDCLASSW wc{};wc.hInstance=GetModuleHandleW(nullptr);wc.lpszClassName=L"NintendoMusicStudioExport";wc.hCursor=LoadCursorW(nullptr,IDC_ARROW);wc.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_BTNFACE+1);
 wc.lpfnWndProc=[](HWND h,UINT m,WPARAM a,LPARAM b)->LRESULT{
  auto self=reinterpret_cast<Window*>(GetWindowLongPtrW(h,GWLP_USERDATA));if(m==WM_NCCREATE){self=static_cast<Window*>(reinterpret_cast<CREATESTRUCTW*>(b)->lpCreateParams);SetWindowLongPtrW(h,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(self));}
  if(m==WM_CREATE){control(h,L"STATIC",L"내보내기 방식",0,2700,14,14,145,25);auto mode=control(h,WC_COMBOBOXW,L"",CBS_DROPDOWNLIST|WS_TABSTOP,2701,170,10,310,120);SendMessageW(mode,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(translateUi(L"사용 중인 채널별 WAV + master.wav").c_str()));SendMessageW(mode,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(translateUi(L"원본 믹스 WAV (효과 제외)").c_str()));SendMessageW(mode,CB_SETCURSEL,0,0);control(h,L"STATIC",L"길이: 0회 반복 시 초",0,2702,14,50,150,24);control(h,L"EDIT",L"",WS_TABSTOP|WS_BORDER|ES_AUTOHSCROLL,2703,170,46,120,26);control(h,L"STATIC",L"기록된 루프 반복",0,2704,14,88,150,24);control(h,L"EDIT",L"0",WS_TABSTOP|WS_BORDER|ES_NUMBER,2705,170,84,120,26);control(h,L"STATIC",L"",0,2706,14,158,470,65);control(h,L"BUTTON",L"폴더 선택 후 내보내기",WS_TABSTOP,2707,14,120,270,30);control(h,L"BUTTON",L"취소",WS_TABSTOP,2708,300,120,100,30);SetTimer(h,1,200,nullptr);return 0;}
  if(!self)return DefWindowProcW(h,m,a,b);
  if(m==WM_TIMER){self->syncExportTrack();std::wstring msg;{std::lock_guard<std::mutex>lock(self->exportMutex_);msg=self->exportMessage_;}if(self->exportRunning_)msg=L"내보내는 중: "+std::to_wstring(self->exportProgress_.load())+L"%";setUiDlgText(h,2706,msg.c_str());EnableWindow(GetDlgItem(h,2707),!self->exportRunning_&&self->player_.loaded());EnableWindow(GetDlgItem(h,2708),self->exportRunning_);return 0;}
  if(m==WM_COMMAND&&HIWORD(a)==BN_CLICKED){try{if(LOWORD(a)==2707)self->startExport();else if(LOWORD(a)==2708)self->exportCancel_=true;}catch(const std::exception&e){self->report(e);}return 0;}
  if(m==WM_CLOSE){ShowWindow(h,SW_HIDE);return 0;}if(m==WM_DESTROY){KillTimer(h,1);self->exportWindow_=nullptr;return 0;}return DefWindowProcW(h,m,a,b);
 };
 if(!RegisterClassW(&wc)&&GetLastError()!=ERROR_CLASS_ALREADY_EXISTS)throw std::runtime_error("Cannot create export window.");
 exportWindow_=CreateWindowExW(WS_EX_TOOLWINDOW|WS_EX_CONTROLPARENT,wc.lpszClassName,L"Nintendo Music Studio · 내보내기",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,CW_USEDEFAULT,CW_USEDEFAULT,520,280,hwnd_,nullptr,wc.hInstance,this);
 if(!exportWindow_)throw std::runtime_error("Cannot open export window.");fitToolWindow(exportWindow_,500,245);rememberUiTree(exportWindow_);exportInput_.clear();exportTrack_=-1;syncExportTrack();ShowWindow(exportWindow_,SW_SHOWNORMAL);
}
void Window::changeLanguage(bool english){
 setUiLanguage(english);if(remoteWindow_)SendDlgItemMessageW(remoteWindow_,2125,CB_SETCURSEL,english?1:0,0);remoteZoomItems_.clear();
 if(exportWindow_){auto combo=GetDlgItem(exportWindow_,2701);auto selected=SendMessageW(combo,CB_GETCURSEL,0,0);SendMessageW(combo,CB_RESETCONTENT,0,0);for(auto label:{L"사용 중인 채널별 WAV + master.wav",L"원본 믹스 WAV (효과 제외)"})SendMessageW(combo,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(translateUi(label).c_str()));SendMessageW(combo,CB_SETCURSEL,selected,0);}
 ModifyMenuW(GetSystemMenu(hwnd_,FALSE),desktop::DefaultAppsCommand,MF_BYCOMMAND|MF_STRING,desktop::DefaultAppsCommand,translateUi(L"기본 앱 설정... (F10)").c_str());DrawMenuBar(hwnd_);
 updateRemote();updateHiddenChannels();savePreferences();labelCacheKey_=0;InvalidateRect(hwnd_,nullptr,FALSE);
}
void Window::setRecording(bool v){recording_=v;updateRemote();}
void Window::updateRemote(){
 if(!remoteWindow_||!IsWindow(remoteWindow_))return;
 setUiText(GetDlgItem(remoteWindow_,2106),recording_?L"녹화뷰 끄기 (F9)":L"녹화뷰 켜기 (F9)");
 std::wstring title=player_.loaded()?wide(player_.tracks()[player_.track()].title):L"음악을 열어 주세요";
 setUiText(GetDlgItem(remoteWindow_,2100),(clockText(player_.seconds())+L"   "+title).c_str());
 setUiText(GetDlgItem(remoteWindow_,2102),player_.playing()?L"일시정지":L"재생");
 setUiText(GetDlgItem(remoteWindow_,2105),player_.repeat()?L"반복 켜짐":L"반복 꺼짐");
 setUiText(GetDlgItem(remoteWindow_,2109),spectrumStyle_==0?L"종류: 막대형… (F7)":spectrumStyle_==1?L"종류: 선형… (F7)":L"종류: 가로흐름… (F7)");
 SendDlgItemMessageW(remoteWindow_,2115,BM_SETCHECK,player_.showAllChannels()?BST_CHECKED:BST_UNCHECKED,0);
 SendDlgItemMessageW(remoteWindow_,2122,BM_SETCHECK,legacyWaveformMotion_?BST_CHECKED:BST_UNCHECKED,0);
 SendDlgItemMessageW(remoteWindow_,2123,BM_SETCHECK,corrscopeWaveform_?BST_CHECKED:BST_UNCHECKED,0);
 setUiText(GetDlgItem(remoteWindow_,2114),(L"숨긴 채널 ("+std::to_wstring(hiddenChannelCount())+L")…").c_str());
 float common=player_.commonChannelVolume();auto states=player_.states();if(common<0)common=states.empty()?1:states[0].volume;
 SendDlgItemMessageW(remoteWindow_,2110,TBM_SETRANGE,TRUE,MAKELPARAM(0,int(player_.channelLimit()*100)));
 int volume=int(std::lround(common*100));SendDlgItemMessageW(remoteWindow_,2110,TBM_SETPOS,TRUE,volume);
 bool mixed=std::any_of(states.begin(),states.end(),[&](const auto& s){return std::abs(s.volume-states.front().volume)>.001f;});
 setUiText(GetDlgItem(remoteWindow_,2111),(mixed?std::wstring(L"모든 채널 혼합"):L"모든 채널 "+std::to_wstring(states.empty()?volume:int(std::lround(states.front().volume*100)))+L"%").c_str());
 SendDlgItemMessageW(remoteWindow_,2112,TBM_SETRANGE,TRUE,MAKELPARAM(0,int(player_.masterLimit()*100)));
 int master=int(std::lround(player_.master()*100));SendDlgItemMessageW(remoteWindow_,2112,TBM_SETPOS,TRUE,master);
 setUiText(GetDlgItem(remoteWindow_,2113),(L"마스터 "+std::to_wstring(master)+L"%").c_str());
 int zoomLevel=std::clamp(waveformZoomStep_,0,32);
 SendDlgItemMessageW(remoteWindow_,2116,TBM_SETPOS,TRUE,zoomLevel);
 std::wstring zoomText=zoomLevel==0?L"0.5×":std::to_wstring(zoomLevel)+L"×";
 int selected=-1;
 const auto channels=player_.channels();
 std::vector<std::wstring> items{L"전체 채널 (공통)"};
 items.reserve(channels.size()+1);
 for(size_t i=0;i<channels.size();++i)items.push_back(std::to_wstring(i+1)+L": "+wide(channels[i].name));
 if(items!=remoteZoomItems_){
  remoteZoomItems_=items;auto combo=GetDlgItem(remoteWindow_,2119);SendMessageW(combo,CB_RESETCONTENT,0,0);
  for(const auto&item:items)SendMessageW(combo,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(translateUi(item).c_str()));
  SendMessageW(combo,CB_SETCURSEL,selectedWaveZoomChannel_+1,0);
 }
 selected=selectedWaveZoomChannel_;
 const int effective=selected>=0&&size_t(selected)<channelWaveformZoomSteps_.size()?waveformZoomStepForChannel(size_t(selected)):waveformZoomStep_;
 SendDlgItemMessageW(remoteWindow_,2116,TBM_SETPOS,TRUE,effective);
 zoomText=effective==0?L"0.5×":std::to_wstring(effective)+L"×";
 bool hasOverride=false;
 if(selected>=0&&size_t(selected)<channels.size()){
  hasOverride=channelWaveformZoomSteps_[size_t(selected)]>=0;
 }
 setUiText(GetDlgItem(remoteWindow_,2118),(L"파형 간격: "+zoomText+(hasOverride?L" (개별)":L"")).c_str());
 setUiText(GetDlgItem(remoteWindow_,2121),selected<0?L"선택 채널":(std::to_wstring(selected+1)+L"번 채널").c_str());
 auto combo=GetDlgItem(remoteWindow_,2119);
 if(int(SendMessageW(combo,CB_GETCURSEL,0,0))!=selected+1)SendMessageW(combo,CB_SETCURSEL,selected+1,0);
 EnableWindow(GetDlgItem(remoteWindow_,2119),player_.loaded());
 EnableWindow(GetDlgItem(remoteWindow_,2120),hasOverride);
}
void Window::toggleRemote(){
 if(remoteWindow_&&IsWindowVisible(remoteWindow_))ShowWindow(remoteWindow_,SW_HIDE);
 else showRemote();
 InvalidateRect(hwnd_,nullptr,FALSE);
}
void Window::showRemote(){
 if(!hwnd_)return;
 if(remoteWindow_&&IsWindow(remoteWindow_)){placeToolVisible(remoteWindow_);ShowWindow(remoteWindow_,screenshot_.empty()?SW_SHOWNORMAL:SW_SHOWNOACTIVATE);updateRemote();return;}
 INITCOMMONCONTROLSEX init{sizeof(init),ICC_BAR_CLASSES};InitCommonControlsEx(&init);
 WNDCLASSW wc{};wc.hInstance=GetModuleHandleW(nullptr);wc.lpszClassName=L"NintendoMusicStudioRemote";wc.hCursor=LoadCursorW(nullptr,IDC_ARROW);wc.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_BTNFACE+1);
 wc.lpfnWndProc=[](HWND h,UINT m,WPARAM a,LPARAM b)->LRESULT{
  auto self=reinterpret_cast<Window*>(GetWindowLongPtrW(h,GWLP_USERDATA));if(m==WM_NCCREATE){self=static_cast<Window*>(reinterpret_cast<CREATESTRUCTW*>(b)->lpCreateParams);SetWindowLongPtrW(h,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(self));}
  if(m==WM_CREATE){
   control(h,L"STATIC",L"",0,2100,14,12,510,28);
   const wchar_t*ko[]={L"이전 곡",L"재생",L"정지",L"다음 곡",L"반복"};
   for(int i=0;i<5;i++)control(h,L"BUTTON",ko[i],WS_TABSTOP,2101+i,14+102*i,48,94,34);
   control(h,L"STATIC",L"",0,2111,14,96,138,24);auto v=control(h,TRACKBAR_CLASSW,L"",WS_TABSTOP|TBS_HORZ,2110,158,88,360,36);SendMessageW(v,TBM_SETRANGE,TRUE,MAKELPARAM(0,200));
   control(h,L"STATIC",L"",0,2113,14,136,138,24);v=control(h,TRACKBAR_CLASSW,L"",WS_TABSTOP|TBS_HORZ,2112,158,128,360,36);SendMessageW(v,TBM_SETRANGE,TRUE,MAKELPARAM(0,100));
   control(h,L"BUTTON",L"20밴드 EQ…",WS_TABSTOP,2107,14,178,180,34);control(h,L"BUTTON",L"녹화뷰 전환 (F9)",WS_TABSTOP,2106,210,178,230,34);
   control(h,L"BUTTON",L"파형 / 스펙트럼 (F6)",WS_TABSTOP,2108,14,218,240,30);
   control(h,L"BUTTON",L"스펙트럼 종류 (F7)",WS_TABSTOP,2109,270,218,240,30);
   control(h,L"BUTTON",L"숨긴 채널…",WS_TABSTOP,2114,14,258,496,30);
   control(h,L"BUTTON",L"모든 채널 표시",WS_TABSTOP|BS_AUTOCHECKBOX,2115,14,298,496,26);
   control(h,L"STATIC",L"언어",0,2124,14,331,130,24);
   auto languages=control(h,WC_COMBOBOXW,L"",CBS_DROPDOWNLIST|WS_TABSTOP,2125,158,327,360,100);
   SendMessageW(languages,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(L"한국어"));SendMessageW(languages,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(L"English"));SendMessageW(languages,CB_SETCURSEL,uiEnglish()?1:0,0);
   control(h,L"STATIC",L"파형 간격: 2×",0,2118,14,367,190,34);
   auto zoom=control(h,TRACKBAR_CLASSW,L"",WS_TABSTOP|TBS_HORZ|TBS_AUTOTICKS,2116,210,367,308,34);
   SendMessageW(zoom,TBM_SETRANGE,TRUE,MAKELPARAM(0,32));SendMessageW(zoom,TBM_SETTICFREQ,1,0);SendMessageW(zoom,TBM_SETLINESIZE,0,1);SendMessageW(zoom,TBM_SETPAGESIZE,0,1);
   control(h,L"STATIC",L"선택 채널",0,2121,14,415,130,24);
   control(h,WC_COMBOBOXW,L"",CBS_DROPDOWNLIST|WS_TABSTOP|WS_VSCROLL,2119,158,410,224,220);
   control(h,L"BUTTON",L"개별 해제",WS_TABSTOP,2120,398,410,120,28);
   control(h,L"BUTTON",L"구 파형 움직임",WS_TABSTOP|BS_AUTOCHECKBOX,2122,14,450,496,26);
   control(h,L"BUTTON",L"코르스코프 파형",WS_TABSTOP|BS_AUTOCHECKBOX,2123,14,482,496,26);
   SetTimer(h,1,200,nullptr);return 0;
  }
  if(!self)return DefWindowProcW(h,m,a,b);
  if(m==WM_PRINT||m==WM_PRINTCLIENT){paintToolClient(h,reinterpret_cast<HDC>(a),m==WM_PRINT);return 0;}
  if(m==WM_TIMER){self->updateRemote();return 0;}
  if(m==WM_COMMAND&&HIWORD(a)==BN_CLICKED&&!self->changing_){try{switch(LOWORD(a)){
   case IDCANCEL:self->toggleRemote();break;
   case 2101:self->nextTrack(-1);break;case 2102:if(self->player_.playing())self->player_.pause();else self->player_.play();break;
   case 2103:self->player_.stop();self->resetSpectrum();break;case 2104:self->nextTrack(1);break;case 2105:self->player_.repeat(!self->player_.repeat());self->savePreferences();break;
   case 2106:PostMessageW(self->hwnd_,WM_KEYDOWN,VK_F9,0);break;case 2107:self->showEqualizer();break;case 2108:self->toggleSpectrum();break;case 2109:self->showSpectrumChoices();break;case 2114:self->showHiddenChannels();break;case 2115:self->player_.showAllChannels(SendDlgItemMessageW(h,2115,BM_GETCHECK,0,0)==BST_CHECKED);self->savePreferences();break;
   case 2120:if(self->selectedWaveZoomChannel_>=0&&size_t(self->selectedWaveZoomChannel_)<self->channelWaveformZoomSteps_.size()){size_t i=size_t(self->selectedWaveZoomChannel_);self->channelWaveformZoomSteps_[i]=-1;if(self->preferencesEnabled_)self->preferences_.values.erase(self->waveformZoomSettingKey(i));self->savePreferences();}break;
   case 2123:self->corrscopeWaveform_=SendDlgItemMessageW(h,2123,BM_GETCHECK,0,0)==BST_CHECKED;if(self->corrscopeWaveform_)self->legacyWaveformMotion_=false;if(self->corrscopeWorker_)self->corrscopeWorker_->reset();self->waveformTrackers_.clear();self->corrscopeScales_.clear();self->waveformTransitions_.clear();self->savePreferences();break;
   case 2122:self->legacyWaveformMotion_=SendDlgItemMessageW(h,2122,BM_GETCHECK,0,0)==BST_CHECKED;if(self->legacyWaveformMotion_)self->corrscopeWaveform_=false;if(self->corrscopeWorker_)self->corrscopeWorker_->reset();self->waveformTrackers_.clear();self->corrscopeScales_.clear();self->waveformTransitions_.clear();self->savePreferences();break;
  }self->updateRemote();InvalidateRect(self->hwnd_,nullptr,FALSE);}catch(const std::exception&e){self->report(e);}return 0;}
  if(m==WM_COMMAND&&LOWORD(a)==2125&&HIWORD(a)==CBN_SELCHANGE){self->changeLanguage(SendDlgItemMessageW(h,2125,CB_GETCURSEL,0,0)==1);return 0;}
  if(m==WM_COMMAND&&LOWORD(a)==2119&&HIWORD(a)==CBN_SELCHANGE){int choice=int(SendDlgItemMessageW(h,2119,CB_GETCURSEL,0,0));self->selectedWaveZoomChannel_=choice-1;self->updateRemote();return 0;}
  if(m==WM_HSCROLL&&!self->changing_){int id=GetDlgCtrlID(reinterpret_cast<HWND>(b));int v=int(SendMessageW(reinterpret_cast<HWND>(b),TBM_GETPOS,0,0));if(id==2110){self->setAllChannelVolume(v/100.f);self->savePreferences();self->allVolumeDirty_=false;}else if(id==2112){self->player_.master(v/100.f);self->savePreferences();}else if(id==2116){self->setSelectedWaveformZoomStep(v);self->savePreferences();InvalidateRect(self->hwnd_,nullptr,FALSE);}self->updateRemote();return 0;}
  if(m==WM_CLOSE){ShowWindow(h,SW_HIDE);InvalidateRect(self->hwnd_,nullptr,FALSE);return 0;}if(m==WM_DESTROY){KillTimer(h,1);self->remoteWindow_=nullptr;return 0;}
  return DefWindowProcW(h,m,a,b);
 };
 if(!RegisterClassW(&wc)&&GetLastError()!=ERROR_CLASS_ALREADY_EXISTS)throw std::runtime_error("Cannot create recording remote.");
 RECT r{};GetWindowRect(hwnd_,&r);remoteWindow_=CreateWindowExW(WS_EX_TOOLWINDOW|WS_EX_CONTROLPARENT,wc.lpszClassName,L"Nintendo Music Studio · 리모컨",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,r.right-570,r.bottom-500,560,500,hwnd_,nullptr,wc.hInstance,this);
 if(!remoteWindow_)throw std::runtime_error("Cannot open recording remote.");fitToolWindow(remoteWindow_,540,530);rememberUiTree(remoteWindow_);SendDlgItemMessageW(remoteWindow_,2119,CB_SETDROPPEDWIDTH,MulDiv(440,toolDpi(remoteWindow_),96),0);updateRemote();ShowWindow(remoteWindow_,screenshot_.empty()?SW_SHOWNORMAL:SW_SHOWNOACTIVATE);
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
 if(!spectrumChoiceWindow_)throw std::runtime_error("Cannot open spectrum choices.");fitToolWindow(spectrumChoiceWindow_,460,242);rememberUiTree(spectrumChoiceWindow_);
 setUiText(spectrumChoiceWindow_,L"스펙트럼 종류");sync();
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
  if(force||GetFocus()!=edit)setUiText(edit,std::to_wstring(gain).c_str());
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
  if(m==WM_PRINT||m==WM_PRINTCLIENT){paintToolClient(h,reinterpret_cast<HDC>(a),m==WM_PRINT);return 0;}
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
 if(!equalizerWindow_)throw std::runtime_error("Cannot open equalizer.");fitToolWindow(equalizerWindow_,630,535);rememberUiTree(equalizerWindow_);updateEqualizer();ShowWindow(equalizerWindow_,screenshot_.empty()?SW_SHOWNORMAL:SW_SHOWNOACTIVATE);
}
}

