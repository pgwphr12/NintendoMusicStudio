#include "SourceWindow.hpp"
#include "SourceEdit.hpp"
#include "player/Player.hpp"
#include <commdlg.h>
#include <future>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <cmath>
namespace ngmv::editor {
namespace {
std::wstring wide(const std::string&s){int n=MultiByteToWideChar(CP_UTF8,0,s.data(),int(s.size()),nullptr,0);std::wstring w(n,0);MultiByteToWideChar(CP_UTF8,0,s.data(),int(s.size()),w.data(),n);return w;}
struct Loaded {SourceEdit edit;std::vector<TrackInfo>tracks;std::vector<ChannelInfo>channels;std::unique_ptr<Player>preview;bool opened=false;};
struct SourceWindow {
    HWND hwnd=nullptr,owner=nullptr,list=nullptr,tracks=nullptr,state=nullptr,gainLabel=nullptr,fileLabel=nullptr;
    HFONT font=nullptr;SourceEdit edit;std::vector<TrackInfo>trackInfo;std::vector<ChannelInfo>names;
    std::filesystem::path project;std::string saved;std::unique_ptr<Player>preview;
    std::future<Loaded> task;std::atomic<bool>cancel{false};std::atomic<int>progress{0};bool finished=false,closeAfter=false;
    ~SourceWindow(){cancel=true;if(task.valid())task.wait();preview.reset();if(font)DeleteObject(font);}
    bool busy()const{return task.valid();}
    bool dirty()const{return !edit.source.empty()&&edit.serialize()!=saved;}
    HWND control(const wchar_t*cls,const std::wstring&text,int id,int x,int y,int w,int h,DWORD style=0){auto c=CreateWindowW(cls,text.c_str(),WS_CHILD|WS_VISIBLE|style,x,y,w,h,hwnd,reinterpret_cast<HMENU>(INT_PTR(id)),nullptr,nullptr);SendMessageW(c,WM_SETFONT,reinterpret_cast<WPARAM>(font),TRUE);return c;}
    void init(){
        font=CreateFontW(-17,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
        control(L"STATIC",L"게임 음악 편집  ·  1.4.9",0,24,18,800,30);
        control(L"BUTTON",L"음악 / 편집본 열기",1,24,60,190,36);control(L"BUTTON",L"편집 저장",2,228,60,140,36);control(L"BUTTON",L"WAV 내보내기",3,382,60,180,36);control(L"BUTTON",L"편집 결과 듣기",4,576,60,170,36);control(L"BUTTON",L"정지",5,760,60,120,36);
        fileLabel=control(L"STATIC",L"NSF · NSFE · SPC · GBS · GSF · 2SF · USF · BCSTM · BCWAV · VGM/VGZ",0,24,110,870,28);
        control(L"STATIC",L"트랙",0,24,151,72,28);tracks=control(L"COMBOBOX",L"",10,100,146,780,260,CBS_DROPDOWNLIST|WS_VSCROLL);
        control(L"STATIC",L"채널 / 음성",0,24,200,380,28);list=control(L"LISTBOX",L"",11,24,235,410,280,LBS_NOTIFY|WS_VSCROLL|WS_BORDER);
        gainLabel=control(L"STATIC",L"선택 채널 100%",0,24,527,410,26);
        control(L"BUTTON",L"−10%",12,24,563,95,36);control(L"BUTTON",L"+10%",13,129,563,95,36);control(L"BUTTON",L"음소거 / 복원",14,234,563,200,36);
        const wchar_t*labels[]={L"시작 (초)",L"끝 (초)",L"삭제 시작 (초)",L"삭제 끝 (초)",L"페이드 인 (초)",L"페이드 아웃 (초)"};
        for(int i=0;i<6;i++){control(L"STATIC",labels[i],0,470,210+i*53,200,28);control(L"EDIT",i==1?L"30":L"0",20+i,690,206+i*53,190,32,WS_BORDER|ES_AUTOHSCROLL);}
        control(L"STATIC",L"삭제 시작 = 끝이면 중간 삭제 없음\n시작·끝은 원곡 시각, 페이드는 편집 결과 기준",0,470,541,420,55);
        state=control(L"STATIC",L"원곡을 열어 음량·구간을 수정하고 편집 결과를 WAV로 내보내세요. 저장한 .ngedit 파일은 원곡 위치와 편집 내용을 보관합니다.",0,24,620,865,60);
        control(L"BUTTON",L"작업 취소",6,610,690,130,36);control(L"BUTTON",L"닫기",7,754,690,126,36);SetTimer(hwnd,1,100,nullptr);
    }
    std::filesystem::path choose(bool save,bool wav=false){wchar_t p[32768]{};OPENFILENAMEW f{};f.lStructSize=sizeof(f);f.hwndOwner=hwnd;f.lpstrFile=p;f.nMaxFile=32768;f.lpstrDefExt=wav?L"wav":L"ngedit";f.lpstrFilter=save?(wav?L"WAV (*.wav)\0*.wav\0\0":L"편집 프로젝트 (*.ngedit)\0*.ngedit\0\0"):L"게임 음악 / 편집 프로젝트\0*.ngedit;*.nsf;*.nsfe;*.spc;*.gbs;*.gsf;*.minigsf;*.2sf;*.mini2sf;*.usf;*.miniusf;*.bcstm;*.bcwav;*.vgm;*.vgz\0\0";f.Flags=OFN_NOCHANGEDIR|OFN_PATHMUSTEXIST|(save?OFN_OVERWRITEPROMPT:OFN_FILEMUSTEXIST);return (save?GetSaveFileNameW(&f):GetOpenFileNameW(&f))?std::filesystem::path(p):std::filesystem::path{};}
    bool confirm(){if(!edit.source.empty())capture();return !dirty()||MessageBoxW(hwnd,L"저장하지 않은 편집 내용을 버릴까요?",L"음악 편집",MB_YESNO|MB_ICONQUESTION)==IDYES;}
    void enabled(bool on){for(int id:{1,2,3,4,10,11,12,13,14,20,21,22,23,24,25})EnableWindow(GetDlgItem(hwnd,id),on);}
    void refresh(){
        SendMessageW(tracks,CB_RESETCONTENT,0,0);for(size_t i=0;i<trackInfo.size();i++){auto s=std::to_wstring(i+1)+L" · "+wide(trackInfo[i].title);SendMessageW(tracks,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(s.c_str()));}SendMessageW(tracks,CB_SETCURSEL,edit.track,0);
        rows();double values[]={edit.begin,edit.end,edit.cutBegin,edit.cutEnd,edit.fadeIn,edit.fadeOut};for(int i=0;i<6;i++){std::wostringstream s;s<<std::fixed<<std::setprecision(3)<<values[i];SetWindowTextW(GetDlgItem(hwnd,20+i),s.str().c_str());}SetWindowTextW(fileLabel,edit.source.filename().c_str());
    }
    void rows(){int selected=int(SendMessageW(list,LB_GETCURSEL,0,0));if(selected<0)selected=0;SendMessageW(list,LB_RESETCONTENT,0,0);for(size_t c=0;c<names.size();c++){auto s=std::to_wstring(c+1)+L" · "+wide(names[c].name)+L"  "+std::to_wstring(int(std::lround(edit.gains[c]*100)))+L"%";SendMessageW(list,LB_ADDSTRING,0,reinterpret_cast<LPARAM>(s.c_str()));}SendMessageW(list,LB_SETCURSEL,selected,0);selection();}
    void selection(){int c=int(SendMessageW(list,LB_GETCURSEL,0,0));if(c>=0&&c<int(edit.gains.size()))SetWindowTextW(gainLabel,(L"선택 채널 "+std::to_wstring(int(std::lround(edit.gains[c]*100)))+L"%").c_str());}
    void capture(){
        if(edit.source.empty())throw std::runtime_error("Open game music first");
        SourceEdit candidate=edit;double* values[]={&candidate.begin,&candidate.end,&candidate.cutBegin,&candidate.cutEnd,&candidate.fadeIn,&candidate.fadeOut};
        for(int i=0;i<6;i++){wchar_t b[128];GetWindowTextW(GetDlgItem(hwnd,20+i),b,128);size_t count=0;*values[i]=std::stod(b,&count);if(count!=wcslen(b))throw std::runtime_error("Invalid time value");}candidate.track=int(SendMessageW(tracks,CB_GETCURSEL,0,0));candidate.validate();edit=std::move(candidate);
    }
    void startOpen(const std::filesystem::path&p){
        preview.reset();SourceEdit next;bool isProject=p.extension()==L".ngedit";if(isProject){if(std::filesystem::file_size(p)>131072)throw std::runtime_error("Project too large");std::ifstream f(p,std::ios::binary);next=SourceEdit::parse(std::string((std::istreambuf_iterator<char>(f)),{}));}else next.source=std::filesystem::absolute(p);
        cancel=false;progress=0;enabled(false);SetWindowTextW(state,L"음악 준비 중 · 창은 계속 사용할 수 있습니다. 다른 곡을 중복으로 준비하지 않습니다.");
        task=std::async(std::launch::async,[this,next,isProject,p]{Loaded r;r.edit=next;auto b=makeBackend(readMusicFile(next.source),next.source);if(cancel.load())throw std::runtime_error("Open cancelled");r.tracks=b->tracks();r.channels=b->channels();if(next.track>=int(r.tracks.size()))throw std::runtime_error("Invalid track");if(isProject){if(next.gains.size()!=r.channels.size())throw std::runtime_error("Source channel layout changed");}else{r.edit.gains.assign(r.channels.size(),1);auto ms=r.tracks[0].lengthMs;r.edit.end=ms>0?std::clamp(ms/1000.,.02,3600.):180.;}r.edit.validate();r.opened=true;return r;});
        pendingProject=isProject?p:std::filesystem::path{};
    }
    std::filesystem::path pendingProject;
    void save(){capture();auto p=project.empty()?choose(true):project;if(p.empty())return;saveSourceEdit(edit,p);project=p;saved=edit.serialize();SetWindowTextW(state,L"편집 저장 완료 · 이 프로젝트를 다시 열면 수정한 음량·구간·페이드를 복원합니다.");}
    void startPreview(){capture();preview.reset();auto e=edit;cancel=false;enabled(false);SetWindowTextW(state,L"수정된 음악을 준비하는 중…");task=std::async(std::launch::async,[this,e]{Loaded r;auto bytes=readMusicFile(e.source);auto core=makeEditedSource(e,&cancel);if(cancel.load())throw std::runtime_error("Preview cancelled");r.preview=std::make_unique<Player>();r.preview->openPrepared(std::move(core),identify(bytes));return r;});}
    void startExport(const std::filesystem::path&p){capture();preview.reset();auto e=edit;cancel=false;progress=0;enabled(false);SetWindowTextW(state,L"편집 결과 WAV 생성 중…");task=std::async(std::launch::async,[this,e,p]{exportSourceWav(e,p,cancel,progress);return Loaded{};});}
    void poll(){if(!task.valid()||task.wait_for(std::chrono::seconds(0))!=std::future_status::ready){if(task.valid()&&progress>0)SetWindowTextW(state,(L"편집 결과 WAV 생성 중  "+std::to_wstring(progress.load())+L"%").c_str());return;}try{auto r=task.get();enabled(true);if(r.opened){edit=std::move(r.edit);trackInfo=std::move(r.tracks);names=std::move(r.channels);project=pendingProject;saved=edit.serialize();refresh();SetWindowTextW(state,L"음악을 열었습니다. 채널 음량과 구간을 수정한 다음 ‘편집 결과 듣기’ 또는 ‘WAV 내보내기’를 누르세요.");}else if(r.preview){preview=std::move(r.preview);preview->play();SetWindowTextW(state,L"수정된 음악 재생 중 · 다음 재생부터 새 편집 내용을 반영합니다.");}else SetWindowTextW(state,L"WAV 내보내기 완료 · 수정된 음량·삭제 구간·페이드를 반영했습니다.");}catch(const std::exception&e){enabled(true);SetWindowTextW(state,wide(e.what()).c_str());}if(closeAfter){closeAfter=false;close();}}
    void close(){if(busy()){cancel=true;closeAfter=true;return;}bool valid=true;try{if(!edit.source.empty())capture();}catch(...){if(MessageBoxW(hwnd,L"입력 중인 시간이 올바르지 않습니다. 편집을 버리고 닫을까요?",L"음악 편집",MB_YESNO)!=IDYES)return;valid=false;}if(!valid||confirm()){preview.reset();finished=true;DestroyWindow(hwnd);}}
    void command(int id,int notification){if(id==6){cancel=true;return;}if(id==7){close();return;}if(busy())return;
        if(id==11&&notification==LBN_SELCHANGE){selection();return;}if(id==10&&notification==CBN_SELCHANGE){preview.reset();SetWindowTextW(state,L"트랙을 바꿨습니다. 다음 재생·내보내기에 반영합니다.");return;}if(notification!=BN_CLICKED)return;
        switch(id){case 1:if(confirm()){auto p=choose(false);if(!p.empty())startOpen(p);}break;case 2:save();break;case 3:{auto p=choose(true,true);if(!p.empty())startExport(p);}break;case 4:startPreview();break;case 5:preview.reset();break;case 12:case 13:case 14:{int c=int(SendMessageW(list,LB_GETCURSEL,0,0));if(c>=0&&c<int(edit.gains.size())){preview.reset();auto&g=edit.gains[c];g=id==14?(g>0?0:1):std::clamp(g+(id==12?-.1f:.1f),0.f,2.f);rows();}}break;}
    }
    static LRESULT CALLBACK proc(HWND h,UINT m,WPARAM w,LPARAM l){auto*s=reinterpret_cast<SourceWindow*>(GetWindowLongPtrW(h,GWLP_USERDATA));if(m==WM_NCCREATE){s=static_cast<SourceWindow*>(reinterpret_cast<CREATESTRUCTW*>(l)->lpCreateParams);s->hwnd=h;SetWindowLongPtrW(h,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(s));}if(!s)return DefWindowProcW(h,m,w,l);try{switch(m){case WM_COMMAND:s->command(LOWORD(w),HIWORD(w));return 0;case WM_TIMER:s->poll();return 0;case WM_CLOSE:s->close();return 0;case WM_DESTROY:s->finished=true;return 0;}}catch(const std::exception&e){MessageBoxW(h,wide(e.what()).c_str(),L"음악 편집",MB_ICONERROR);}return DefWindowProcW(h,m,w,l);}
};
void snapshot(HWND h,const std::filesystem::path&p){
 RECT r{};GetClientRect(h,&r);HDC dc=GetDC(h),mem=CreateCompatibleDC(dc);BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=r.right;info.bmiHeader.biHeight=-r.bottom;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;void*pixels=nullptr;auto bm=CreateDIBSection(dc,&info,DIB_RGB_COLORS,&pixels,nullptr,0);auto old=SelectObject(mem,bm);FillRect(mem,&r,GetSysColorBrush(COLOR_WINDOW));
 struct Paint {HWND parent;HDC dc;};Paint paint{h,mem};
 EnumChildWindows(h,[](HWND child,LPARAM param)->BOOL{auto&p=*reinterpret_cast<Paint*>(param);if(GetParent(child)!=p.parent)return TRUE;RECT r{};GetWindowRect(child,&r);MapWindowPoints(nullptr,p.parent,reinterpret_cast<POINT*>(&r),2);int state=SaveDC(p.dc);SetViewportOrgEx(p.dc,r.left,r.top,nullptr);IntersectClipRect(p.dc,0,0,r.right-r.left,r.bottom-r.top);SendMessageW(child,WM_PRINT,reinterpret_cast<WPARAM>(p.dc),PRF_CLIENT|PRF_NONCLIENT|PRF_ERASEBKGND|PRF_CHILDREN);RestoreDC(p.dc,state);return TRUE;},reinterpret_cast<LPARAM>(&paint));BITMAPFILEHEADER header{};header.bfType=0x4d42;header.bfOffBits=sizeof(header)+sizeof(BITMAPINFOHEADER);header.bfSize=header.bfOffBits+r.right*r.bottom*4;std::ofstream f(p,std::ios::binary);f.write(reinterpret_cast<char*>(&header),sizeof(header));f.write(reinterpret_cast<char*>(&info.bmiHeader),sizeof(info.bmiHeader));f.write(static_cast<char*>(pixels),r.right*r.bottom*4);SelectObject(mem,old);DeleteObject(bm);DeleteDC(mem);ReleaseDC(h,dc);
}
HWND create(SourceWindow&s,HWND owner){auto instance=GetModuleHandleW(nullptr);WNDCLASSW c{};c.hInstance=instance;c.lpszClassName=L"NintendoSourceEdit140";c.lpfnWndProc=SourceWindow::proc;c.hCursor=LoadCursor(nullptr,IDC_ARROW);c.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);RegisterClassW(&c);s.owner=owner;auto h=CreateWindowW(c.lpszClassName,L"Nintendo Music Studio 1.4.9 · 게임 음악 편집",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,CW_USEDEFAULT,CW_USEDEFAULT,930,795,owner,nullptr,instance,&s);if(!h)throw std::runtime_error("Could not create source editor");s.init();return h;}
}
void runSourceEditor(HWND owner,const std::filesystem::path&initial){SourceWindow s;auto h=create(s,owner);EnableWindow(owner,FALSE);ShowWindow(h,SW_SHOWNORMAL);if(!initial.empty())try{s.startOpen(initial);}catch(const std::exception&e){SetWindowTextW(s.state,wide(e.what()).c_str());}MSG msg{};while(!s.finished&&GetMessageW(&msg,nullptr,0,0)>0){if(!IsDialogMessageW(h,&msg)){TranslateMessage(&msg);DispatchMessageW(&msg);}}EnableWindow(owner,TRUE);SetForegroundWindow(owner);}
void sourceEditorSelftest(const std::filesystem::path&dir){
    SourceWindow s;auto h=create(s,nullptr);auto wait=[&]{for(int i=0;i<3000&&s.busy();i++){s.poll();Sleep(10);}if(s.busy())throw std::runtime_error("Source editor task timeout");};
    try{s.startOpen(dir/L"UI test.nsf");wait();if(s.names.empty())throw std::runtime_error("Source editor open failed");s.command(12,BN_CLICKED);SetWindowTextW(GetDlgItem(h,20),L"0.05");SetWindowTextW(GetDlgItem(h,21),L"0.3");SetWindowTextW(GetDlgItem(h,25),L"0.05");s.project=dir/L"UI source.ngedit";s.save();s.startExport(dir/L"UI edited.wav");wait();if(std::filesystem::file_size(dir/L"UI edited.wav")!=56+12000*8)throw std::runtime_error("Source editor WAV export failed");s.startPreview();wait();if(!s.preview||!s.preview->playing())throw std::runtime_error("Edited preview failed");s.preview.reset();s.startOpen(s.project);wait();if(s.edit.begin!=.05||s.edit.end!=.3||s.edit.gains[0]>.91f)throw std::runtime_error("Source editor save/reopen failed");snapshot(h,dir/L"source-editor.bmp");DestroyWindow(h);}catch(...){DestroyWindow(h);throw;}
}
}
