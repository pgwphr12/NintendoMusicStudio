#include "Window.hpp"
#include "ChannelLayout.hpp"
#include <map>
#include <stdexcept>
namespace ngmv {
std::wstring wide(const std::string&);
size_t Window::hiddenChannelCount() const {
    return size_t(std::count(hiddenChannels_.begin(),hiddenChannels_.end(),true));
}
void Window::refreshHiddenChannels(){
    hiddenChannelKeys_.clear();hiddenChannels_.clear();
    const auto& channels=player_.channels();
    const auto system=player_.tracks().empty()?std::string():player_.tracks()[0].system;
    std::map<std::string,size_t> occurrence;
    for(const auto& channel:channels){
        // Duplicate labels stay independently addressable without storing paths.
        auto n=occurrence[channel.name]++;
        auto key=desktop::Settings::channelKey(system,channel.name)+".hidden."+std::to_string(n);
        hiddenChannelKeys_.push_back(key);
        hiddenChannels_.push_back(preferencesEnabled_&&preferences_.get(key,0,0,1)!=0);
    }
    updateHiddenChannels();updateRemote();
}
void Window::setChannelHidden(size_t channel,bool hidden){
    if(changing_||channel>=player_.channels().size()||channel>=hiddenChannels_.size())return;
    if(hidden)player_.showAllChannels(false);
    hiddenChannels_[channel]=hidden;
    preferences_.values[hiddenChannelKeys_[channel]]=hidden?1:0;
    // Display preference only: no backend mute/gain/history changes.
    drag_=-1;
    if(GetCapture()==hwnd_)ReleaseCapture();
    savePreferences();updateHiddenChannels();updateRemote();InvalidateRect(hwnd_,nullptr,FALSE);
}
void Window::hideChannelAt(int x,int y){
    if(changing_||spectrumMode_||!player_.loaded()||scale_<=0)return;
    const double lx=(x-offsetX_)/scale_,ly=(y-offsetY_)/scale_;
    const auto visible=visibleChannels();
    for(size_t slot=0;slot<visible.size();++slot){
        const auto cell=channelCell(slot,visible.size(),recording_);
        if(lx>=cell.x&&lx<cell.x+cell.width&&ly>=cell.y&&ly<cell.y+cell.height-8){
            setChannelHidden(visible[slot],true);return;
        }
    }
}
void Window::updateHiddenChannels(){
    if(!hiddenChannelsWindow_||!IsWindow(hiddenChannelsWindow_))return;
    auto list=GetDlgItem(hiddenChannelsWindow_,2700);
    const LRESULT selected=SendMessageW(list,LB_GETCURSEL,0,0);
    const LRESULT oldChannel=selected==LB_ERR?-1:SendMessageW(list,LB_GETITEMDATA,selected,0);
    SendMessageW(list,WM_SETREDRAW,FALSE,0);SendMessageW(list,LB_RESETCONTENT,0,0);
    int selectedRow=-1,count=0,extent=0;
    const auto& channels=player_.channels();
    for(size_t i=0;i<channels.size()&&i<hiddenChannels_.size();++i)if(hiddenChannels_[i]){
        auto label=wide(channels[i].name);
        auto row=SendMessageW(list,LB_ADDSTRING,0,reinterpret_cast<LPARAM>(label.c_str()));
        if(row==LB_ERR||row==LB_ERRSPACE)continue;
        SendMessageW(list,LB_SETITEMDATA,row,LPARAM(i));
        if(LRESULT(i)==oldChannel)selectedRow=int(row);
        extent=std::max(extent,int(label.size())*14+24);++count;
    }
    if(count)SendMessageW(list,LB_SETCURSEL,selectedRow<0?0:selectedRow,0);
    SendMessageW(list,LB_SETHORIZONTALEXTENT,extent,0);
    SendMessageW(list,WM_SETREDRAW,TRUE,0);InvalidateRect(list,nullptr,TRUE);
    const auto message=count?L"직접 숨긴 채널입니다. 선택한 채널을 다시 표시할 수 있습니다.":L"직접 숨긴 채널이 없습니다.";
    SetWindowTextW(GetDlgItem(hiddenChannelsWindow_,2703),message);
    EnableWindow(GetDlgItem(hiddenChannelsWindow_,2701),count!=0);
    EnableWindow(GetDlgItem(hiddenChannelsWindow_,2702),count!=0);
}
void Window::showHiddenChannels(){
    if(hiddenChannelsWindow_&&IsWindow(hiddenChannelsWindow_)){
        updateHiddenChannels();ShowWindow(hiddenChannelsWindow_,screenshot_.empty()?SW_SHOWNORMAL:SW_SHOWNOACTIVATE);
        if(screenshot_.empty())SetForegroundWindow(hiddenChannelsWindow_);return;
    }
    WNDCLASSW wc{};wc.hInstance=GetModuleHandleW(nullptr);wc.lpszClassName=L"NintendoMusicStudioHiddenChannels";
    wc.hCursor=LoadCursorW(nullptr,IDC_ARROW);wc.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_BTNFACE+1);
    wc.lpfnWndProc=[](HWND h,UINT m,WPARAM a,LPARAM b)->LRESULT{
        auto self=reinterpret_cast<Window*>(GetWindowLongPtrW(h,GWLP_USERDATA));
        if(m==WM_NCCREATE){self=static_cast<Window*>(reinterpret_cast<CREATESTRUCTW*>(b)->lpCreateParams);SetWindowLongPtrW(h,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(self));}
        if(m==WM_CREATE){
            auto control=[&](const wchar_t* cls,const wchar_t* text,DWORD style,int id,int x,int y,int w,int height){
                auto c=CreateWindowExW(0,cls,text,WS_CHILD|WS_VISIBLE|style,x,y,w,height,h,reinterpret_cast<HMENU>(INT_PTR(id)),GetModuleHandleW(nullptr),nullptr);
                SendMessageW(c,WM_SETFONT,reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),TRUE);return c;
            };
            control(L"STATIC",L"",0,2703,16,14,488,34);
            control(L"LISTBOX",L"",WS_BORDER|WS_VSCROLL|WS_HSCROLL|WS_TABSTOP|LBS_NOTIFY|LBS_NOINTEGRALHEIGHT,2700,16,50,488,190);
            control(L"STATIC",L"소리·음량은 그대로 유지됩니다. 미사용 채널의 자동 숨김은 별개입니다.",0,2704,16,250,488,34);
            control(L"BUTTON",L"선택 채널 복원",WS_TABSTOP|BS_DEFPUSHBUTTON,2701,16,294,170,32);
            control(L"BUTTON",L"모두 복원",WS_TABSTOP,2702,198,294,140,32);
            control(L"BUTTON",L"닫기",WS_TABSTOP,IDCANCEL,394,294,110,32);return 0;
        }
        if(!self)return DefWindowProcW(h,m,a,b);
        if(m==WM_COMMAND){
            int id=LOWORD(a);
            if(id==IDCANCEL){ShowWindow(h,SW_HIDE);return 0;}
            if(self->changing_)return 0;
            if((id==2701&&HIWORD(a)==BN_CLICKED)||(id==2700&&HIWORD(a)==LBN_DBLCLK)){
                auto list=GetDlgItem(h,2700);auto row=SendMessageW(list,LB_GETCURSEL,0,0);
                if(row!=LB_ERR){auto channel=SendMessageW(list,LB_GETITEMDATA,row,0);if(channel!=LB_ERR)self->setChannelHidden(size_t(channel),false);}return 0;
            }
            if(id==2702&&HIWORD(a)==BN_CLICKED){
                for(size_t i=0;i<self->hiddenChannels_.size();++i)if(self->hiddenChannels_[i]){
                    self->hiddenChannels_[i]=false;self->preferences_.values[self->hiddenChannelKeys_[i]]=0;
                }
                self->savePreferences();self->updateHiddenChannels();self->updateRemote();InvalidateRect(self->hwnd_,nullptr,FALSE);return 0;
            }
        }
        if(m==WM_CLOSE){ShowWindow(h,SW_HIDE);return 0;}
        if(m==WM_DESTROY){self->hiddenChannelsWindow_=nullptr;return 0;}
        return DefWindowProcW(h,m,a,b);
    };
    if(!RegisterClassW(&wc)&&GetLastError()!=ERROR_CLASS_ALREADY_EXISTS)throw std::runtime_error("Cannot create hidden channel list.");
    RECT owner{};GetWindowRect(remoteWindow_?remoteWindow_:hwnd_,&owner);RECT rect{0,0,520,342};
    AdjustWindowRectEx(&rect,WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,FALSE,WS_EX_TOOLWINDOW|WS_EX_CONTROLPARENT);
    hiddenChannelsWindow_=CreateWindowExW(WS_EX_TOOLWINDOW|WS_EX_CONTROLPARENT,wc.lpszClassName,L"숨긴 채널",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,
        owner.left+20,owner.top+20,rect.right-rect.left,rect.bottom-rect.top,hwnd_,nullptr,wc.hInstance,this);
    if(!hiddenChannelsWindow_)throw std::runtime_error("Cannot open hidden channel list.");
    updateHiddenChannels();ShowWindow(hiddenChannelsWindow_,screenshot_.empty()?SW_SHOWNORMAL:SW_SHOWNOACTIVATE);
}
}
