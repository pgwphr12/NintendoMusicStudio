#include "RipWindow.hpp"
#include "GameRip.hpp"
#include "core/IMusicBackend.hpp"
#include "player/Player.hpp"
#include <commdlg.h>
#include <shellapi.h>
#include <shlobj.h>
#include <thread>
#include <mutex>
namespace ngmv::editor {
namespace {
std::wstring wide(const std::string&s){int n=MultiByteToWideChar(CP_UTF8,0,s.data(),int(s.size()),nullptr,0);std::wstring w(n,0);MultiByteToWideChar(CP_UTF8,0,s.data(),int(s.size()),w.data(),n);return w;}
struct RipWindow {
 HWND hwnd=nullptr,owner=nullptr,list=nullptr,tracks=nullptr,seconds=nullptr,tempo=nullptr,label=nullptr,state=nullptr;
 std::filesystem::path file,output;std::vector<ChannelInfo>channels;std::thread worker;std::atomic<bool>cancel{false},running{false},done{false};std::atomic<int>progress{0};
 std::mutex mutex;std::optional<RipResult>result;std::optional<Song>imported;std::string error;bool finished=false;Player preview;HFONT font=nullptr;
 ~RipWindow(){cancel=true;if(worker.joinable())worker.join();preview.pause();if(font)DeleteObject(font);}
 HWND control(const wchar_t*cls,const wchar_t*title,int id,int x,int y,int w,int h,DWORD style=0){auto c=CreateWindowW(cls,title,WS_CHILD|WS_VISIBLE|style,x,y,w,h,hwnd,reinterpret_cast<HMENU>(INT_PTR(id)),nullptr,nullptr);SendMessageW(c,WM_SETFONT,reinterpret_cast<WPARAM>(font),TRUE);return c;}
 void init(int bpm){
  font=CreateFontW(-17,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
  control(L"STATIC",L"게임 음원 분석 · 채널별 추출",0,24,18,700,32);control(L"BUTTON",L"게임 음원 열기",1,24,62,160,36);
  label=control(L"STATIC",L"NSF / NSFE / SPC / GBS / GSF / 2SF / USF / BCSTM / BCWAV / VGM / VGZ",0,200,64,708,42);
  control(L"STATIC",L"하위 트랙",0,24,122,110,24);tracks=control(L"COMBOBOX",L"",10,130,116,450,280,CBS_DROPDOWNLIST|WS_VSCROLL);
  control(L"STATIC",L"분석 길이(초)",0,24,165,112,26);seconds=control(L"EDIT",L"30",11,142,160,90,30,WS_BORDER|ES_NUMBER);
  control(L"STATIC",L"1–300초",0,246,165,130,26);control(L"STATIC",L"음표 격자 BPM",0,430,165,145,26);tempo=control(L"EDIT",std::to_wstring(bpm).c_str(),12,580,160,90,30,WS_BORDER|ES_NUMBER);
  control(L"STATIC",L"디코더가 제공하는 채널 / 버스",0,24,216,500,28);list=control(L"LISTBOX",L"",13,24,250,545,260,LBS_NOTIFY|WS_VSCROLL|WS_BORDER);
  control(L"BUTTON",L"선택 채널만 듣기",2,600,250,300,38);control(L"BUTTON",L"미리 듣기 정지",3,600,299,300,38);
  control(L"BUTTON",L"전체 채널 WAV + 분석 추출",4,600,366,300,42);control(L"BUTTON",L"NSF 기본 음표 → 작곡 화면",5,600,421,300,42);EnableWindow(GetDlgItem(hwnd,5),FALSE);
  control(L"BUTTON",L"결과 폴더",6,600,477,143,36);control(L"BUTTON",L"닫기 / 취소",7,754,477,146,36);
  state=control(L"STATIC",L"원본 파일은 수정하지 않습니다. NSF 음표 가져오기는 기본 4채널의 16분음표 격자 변환이며, DPCM·확장 음원·원래 악기는 복원하지 않습니다.",0,24,535,880,60);
  SetTimer(hwnd,1,100,nullptr);
 }
 void open(){
  if(running)return;preview.stop();wchar_t p[32768]{};OPENFILENAMEW f{};f.lStructSize=sizeof(f);f.hwndOwner=hwnd;f.lpstrFile=p;f.nMaxFile=32768;f.lpstrFilter=L"게임 음원\0*.nsf;*.nsfe;*.spc;*.gbs;*.gsf;*.minigsf;*.2sf;*.mini2sf;*.usf;*.miniusf;*.bcstm;*.bcwav;*.vgm;*.vgz\0\0";f.Flags=OFN_FILEMUSTEXIST|OFN_NOCHANGEDIR|OFN_PATHMUSTEXIST;if(!GetOpenFileNameW(&f))return;
  auto bytes=readMusicFile(p);auto b=makeBackend(bytes,p);channels=b->channels();file=p;result.reset();EnableWindow(GetDlgItem(hwnd,5),FALSE);SetWindowTextW(label,file.filename().c_str());SendMessageW(tracks,CB_RESETCONTENT,0,0);
  for(size_t i=0;i<b->tracks().size();i++){auto title=std::to_wstring(i+1)+L" · "+wide(b->tracks()[i].title);SendMessageW(tracks,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(title.c_str()));}SendMessageW(tracks,CB_SETCURSEL,0,0);
  SendMessageW(list,LB_RESETCONTENT,0,0);for(size_t i=0;i<channels.size();i++){auto title=std::to_wstring(i+1)+L" · "+wide(channels[i].name);SendMessageW(list,LB_ADDSTRING,0,reinterpret_cast<LPARAM>(title.c_str()));}SendMessageW(list,LB_SETCURSEL,0,0);
  SetWindowTextW(state,L"파일을 열었습니다. 트랙과 분석 길이를 고른 뒤 추출하세요. 채널/버스 수는 원본 형식과 디코더에 따라 다릅니다.");
 }
 int number(HWND h){wchar_t b[32];GetWindowTextW(h,b,32);try{return std::stoi(b);}catch(...){return 0;}}
 void extract(){
  if(file.empty()||running)return;int len=number(seconds),bpm=number(tempo),track=int(SendMessageW(tracks,CB_GETCURSEL,0,0));if(len<1||len>300||bpm<30||bpm>300)throw std::runtime_error("Length:1–300 seconds; BPM:30–300");
  BROWSEINFOW b{};b.hwndOwner=hwnd;b.lpszTitle=L"새 추출 폴더를 만들 상위 폴더 선택";b.ulFlags=BIF_RETURNONLYFSDIRS|BIF_NEWDIALOGSTYLE;auto id=SHBrowseForFolderW(&b);if(!id)return;wchar_t p[MAX_PATH];bool valid=SHGetPathFromIDListW(id,p)!=FALSE;CoTaskMemFree(id);if(!valid)return;
  output=std::filesystem::path(p)/(file.stem().wstring()+L"-track"+std::to_wstring(track+1)+L"-"+std::to_wstring(GetTickCount64()));
  preview.stop();if(worker.joinable())worker.join();cancel=false;running=true;done=false;progress=0;result.reset();error.clear();EnableWindow(GetDlgItem(hwnd,5),FALSE);
  for(int i:{1,2,3,4,10,11,12,13})EnableWindow(GetDlgItem(hwnd,i),FALSE);
  worker=std::thread([this,track,len,bpm]{try{auto r=ripGameMusic(file,track,len,bpm,output,cancel,progress);std::lock_guard<std::mutex>lock(mutex);result=std::move(r);}catch(const std::exception&e){std::lock_guard<std::mutex>lock(mutex);error=e.what();}running=false;done=true;});
 }
 void command(int id){switch(id){
  case 1:open();break;
  case 2:if(!file.empty()&&!running){preview.stop();preview.open(file);preview.select(int(SendMessageW(tracks,CB_GETCURSEL,0,0)));int channel=int(SendMessageW(list,LB_GETCURSEL,0,0));auto states=preview.states();for(size_t i=0;i<states.size();i++){states[i].mute=int(i)!=channel;preview.channel(i,states[i]);}preview.play();}break;
  case 3:preview.stop();break;case 4:extract();break;
  case 5:{std::lock_guard<std::mutex>lock(mutex);if(result&&result->editable){imported=result->editable;finished=true;DestroyWindow(hwnd);}}break;
  case 6:if(!output.empty()&&std::filesystem::exists(output))ShellExecuteW(hwnd,L"open",output.c_str(),nullptr,nullptr,SW_SHOWNORMAL);break;
  case 7:if(running){cancel=true;SetWindowTextW(state,L"취소 중 · 쓰고 있던 파일을 정리하고 있습니다.");}else{finished=true;DestroyWindow(hwnd);}break;
 }}
 static LRESULT CALLBACK proc(HWND h,UINT m,WPARAM w,LPARAM l){auto*s=reinterpret_cast<RipWindow*>(GetWindowLongPtrW(h,GWLP_USERDATA));if(m==WM_NCCREATE){s=static_cast<RipWindow*>(reinterpret_cast<CREATESTRUCTW*>(l)->lpCreateParams);s->hwnd=h;SetWindowLongPtrW(h,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(s));}if(!s)return DefWindowProcW(h,m,w,l);
  try{switch(m){case WM_COMMAND:if(HIWORD(w)==BN_CLICKED)s->command(LOWORD(w));return 0;
  case WM_TIMER:if(s->running){SetWindowTextW(s->state,(L"추출 중  "+std::to_wstring(s->progress.load())+L"% · 원본을 수정하지 않고 채널별 WAV를 생성합니다.").c_str());}
   if(s->done.exchange(false)){if(s->worker.joinable())s->worker.join();for(int i:{1,2,3,4,10,11,12,13})EnableWindow(GetDlgItem(h,i),TRUE);std::lock_guard<std::mutex>lock(s->mutex);if(!s->error.empty())SetWindowTextW(s->state,wide(s->error).c_str());else if(s->result){EnableWindow(GetDlgItem(h,5),s->result->editable.has_value());SetWindowTextW(s->state,s->result->editable?L"추출 완료. 채널 WAV와 분석 CSV를 저장했습니다. 기본 NES 음표는 격자 변환본이며 DPCM/확장 음원은 WAV로 보존됩니다.":L"채널/버스별 WAV 추출 완료. 이 형식은 아직 음표 프로젝트 가져오기를 지원하지 않습니다.");}}return 0;
  case WM_CLOSE:if(s->running){s->cancel=true;return 0;}s->finished=true;DestroyWindow(h);return 0;
  case WM_DESTROY:s->finished=true;s->preview.pause();return 0;
  }}catch(const std::exception&e){MessageBoxW(h,wide(e.what()).c_str(),L"게임 음원 분석",MB_ICONERROR);}return DefWindowProcW(h,m,w,l);
 }
};
}
std::optional<Song> runGameRipDialog(HWND owner,int bpm){
 RipWindow dialog;dialog.owner=owner;auto instance=reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(owner,GWLP_HINSTANCE));WNDCLASSW c{};c.hInstance=instance;c.lpszClassName=L"NintendoGameRipWindow";c.lpfnWndProc=RipWindow::proc;c.hCursor=LoadCursor(nullptr,IDC_ARROW);c.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);RegisterClassW(&c);
 HWND h=CreateWindowW(c.lpszClassName,L"Nintendo Music Studio · 게임 음원 분석",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,100,100,960,670,owner,nullptr,instance,&dialog);if(!h)return {};dialog.init(bpm);EnableWindow(owner,FALSE);ShowWindow(h,SW_SHOWNORMAL);MSG msg{};while(!dialog.finished&&GetMessageW(&msg,nullptr,0,0)>0){if(!IsDialogMessageW(h,&msg)){TranslateMessage(&msg);DispatchMessageW(&msg);}}EnableWindow(owner,TRUE);SetForegroundWindow(owner);return dialog.imported;
}
}
