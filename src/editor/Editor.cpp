#include "Song.hpp"
#include "Editor.hpp"
#include "RipWindow.hpp"
#include "SourceWindow.hpp"
#include "NoteImportWindow.hpp"
#include "NoteImport.hpp"
#include "GbaImport.hpp"
#include "SampleBackend.hpp"
#include "SampleSong.hpp"
#include <cstring>
#include <tuple>
#include <future>
#include "player/Player.hpp"
#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>
#include <shellapi.h>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
namespace ngmv::editor {
static std::wstring wide(const std::string&s){int n=MultiByteToWideChar(CP_UTF8,0,s.data(),int(s.size()),nullptr,0);std::wstring w(n,0);MultiByteToWideChar(CP_UTF8,0,s.data(),int(s.size()),w.data(),n);return w;}
static std::string utf8(const std::wstring&s){int n=WideCharToMultiByte(CP_UTF8,0,s.data(),int(s.size()),nullptr,0,nullptr,nullptr);std::string r(n,0);WideCharToMultiByte(CP_UTF8,0,s.data(),int(s.size()),r.data(),n,nullptr,nullptr);return r;}
static void atomicWrite(const std::filesystem::path&p,const std::vector<uint8_t>&b){
    auto tmp=p;tmp+=L".editor-tmp-"+std::to_wstring(GetCurrentProcessId());
    try{std::ofstream f(tmp,std::ios::binary|std::ios::trunc);f.write(reinterpret_cast<const char*>(b.data()),std::streamsize(b.size()));f.close();if(!f)throw std::runtime_error("Could not write file");
        if(!MoveFileExW(tmp.c_str(),p.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw std::runtime_error("Could not replace file; existing file retained");
    }catch(...){std::error_code e;std::filesystem::remove(tmp,e);throw;}
}
struct Editor {
    HWND window=nullptr;Song song=Song::demo();Player player;std::filesystem::path project,preview;
    std::optional<Song> savedSong=song;std::vector<Song>undo,redo;int channel=0,page=0,low=48;bool dragging=false,loop=true;Note draft,originalDraft;int selected=-1,dragMode=0,dragStep=0,dragPitch=0;
    std::filesystem::path importSource;std::future<void>wavJob;std::atomic<bool>cancelWav{false};std::atomic<int>wavProgress{0};
    int unit()const{return song.resolution/4;}
    double scale=1;int ox=0,oy=0;bool testing=false,finished=false;std::function<void(const std::filesystem::path&)> showViewer;std::filesystem::path pendingViewer;bool pendingSampleViewer=false;std::function<void(const Song&)>showSampleViewer;std::wstring status=L"음표를 드래그해 길이를 정하세요. 오른쪽 클릭은 삭제입니다.";
    std::map<int,RECT>buttons;std::map<COLORREF,HBRUSH>brushes;std::map<int,HFONT>fonts;
    const COLORREF bg=RGB(13,17,27),panel=RGB(21,27,40),muted=RGB(133,148,172),ink=RGB(226,234,247),mint=RGB(91,232,195);
    std::array<COLORREF,4>colors{RGB(90,229,195),RGB(244,188,111),RGB(151,168,253),RGB(238,139,169)};
    ~Editor(){cancelWav=true;if(wavJob.valid())wavJob.wait();player.releaseDevice();std::error_code e;if(!preview.empty()){std::filesystem::remove(preview,e);std::filesystem::remove(preview.parent_path(),e);}for(auto&[k,v]:brushes)DeleteObject(v);for(auto&[k,v]:fonts)DeleteObject(v);}
    HBRUSH brush(COLORREF c){auto&b=brushes[c];if(!b)b=CreateSolidBrush(c);return b;}
    void fill(HDC dc,RECT r,COLORREF c){FillRect(dc,&r,brush(c));}
    void text(HDC dc,const std::wstring&s,RECT r,int size,COLORREF c){auto&f=fonts[size];if(!f)f=CreateFontW(-size,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");auto old=SelectObject(dc,f);SetBkMode(dc,TRANSPARENT);SetTextColor(dc,c);DrawTextW(dc,s.c_str(),-1,&r,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);SelectObject(dc,old);}
    void button(HDC dc,int id,const std::wstring&s,int x,int y,int w,bool on=false){RECT r{x,y,x+w,y+36};buttons[id]=r;fill(dc,r,on?RGB(31,73,67):RGB(33,42,58));r.left+=10;text(dc,s,r,16,on?mint:ink);}
    bool dirty()const{
        if(!savedSong)return true;const auto&a=song;const auto&b=*savedSong;
        if(std::tie(a.bpm,a.bars,a.resolution,a.pal,a.title,a.sampled)!=std::tie(b.bpm,b.bars,b.resolution,b.pal,b.title,b.sampled)||a.instruments.size()!=b.instruments.size()||a.samples.size()!=b.samples.size()||a.notes.size()!=b.notes.size())return true;
        for(size_t i=0;i<a.instruments.size();i++){auto&x=a.instruments[i];auto&y=b.instruments[i];if(std::tie(x.duty,x.volume,x.mute,x.sample,x.pan,x.name)!=std::tie(y.duty,y.volume,y.mute,y.sample,y.pan,y.name))return true;}
        for(size_t i=0;i<a.samples.size();i++){auto&x=a.samples[i];auto&y=b.samples[i];if(std::tie(x.name,x.rootKey,x.sampleRate,x.loop,x.loopStart,x.pcm)!=std::tie(y.name,y.rootKey,y.sampleRate,y.loop,y.loopStart,y.pcm))return true;}
        for(size_t i=0;i<a.notes.size();i++){auto&x=a.notes[i];auto&y=b.notes[i];if(std::tie(x.channel,x.step,x.length,x.pitch,x.velocity,x.duty,x.sample,x.sourceOffset,x.controls)!=std::tie(y.channel,y.step,y.length,y.pitch,y.velocity,y.duty,y.sample,y.sourceOffset,y.controls))return true;}
        return false;
    }
    bool noise()const{return !song.sampled&&channel==3;}
    int pitchMin()const{return song.sampled?0:noise()?0:24;}
    int pitchMax()const{return song.sampled?127:noise()?15:95;}
    int lowMin()const{return song.sampled?0:24;}
    int lowMax()const{return song.sampled?91:59;}
    void focusTrack(){channel=std::clamp(channel,0,int(song.instruments.size())-1);low=std::clamp(low,lowMin(),lowMax());}
    void setSample(int sample){
        if(!song.sampled||sample<0||sample>=int(song.samples.size()))return;
        int at=selected;Song candidate=song;
        if(at>=0&&at<int(candidate.notes.size())){auto&n=candidate.notes[at];n.sample=sample;if(n.sourceOffset>=candidate.samples[sample].pcm->size())n.sourceOffset=0;}
        else{candidate.instruments[channel].sample=sample;for(auto&n:candidate.notes)if(n.channel==channel&&n.sample<0&&n.sourceOffset>=candidate.samples[sample].pcm->size())n.sourceOffset=0;}
        candidate.validate();remember();song=std::move(candidate);selected=at;status=L"샘플 악기를 변경했습니다 · 새 악기보다 긴 시작 위치는 0으로 복원됩니다.";
    }
    void chooseSample(){HMENU menu=CreatePopupMenu();for(size_t i=0;i<song.samples.size();i++)AppendMenuW(menu,MF_STRING,UINT_PTR(2000+i),wide(song.samples[i].name).c_str());POINT at;GetCursorPos(&at);int picked=TrackPopupMenu(menu,TPM_RETURNCMD|TPM_NONOTIFY,at.x,at.y,0,window,nullptr);DestroyMenu(menu);if(picked>=2000)setSample(picked-2000);}
    void changed(){focusTrack();InvalidateRect(window,nullptr,FALSE);SetWindowTextW(window,(L"Nintendo Music Studio 1.4.9 · 공통 음악 편집 — "+wide(song.title)+(dirty()?L" *":L"")).c_str());}
    void remember(){selected=-1;undo.push_back(song);if(undo.size()>100)undo.erase(undo.begin());redo.clear();player.stop();}
    bool confirm(){return testing||!dirty()||MessageBoxW(window,L"저장하지 않은 변경 사항을 버릴까요?",L"프로젝트",MB_YESNO|MB_ICONQUESTION)==IDYES;}
    std::filesystem::path dialog(bool save,int format=0){wchar_t path[32768]{};OPENFILENAMEW f{};f.lStructSize=sizeof(f);f.hwndOwner=window;f.lpstrFile=path;f.nMaxFile=32768;f.lpstrFilter=format==2?L"WAV (*.wav)\0*.wav\0\0":format==1?L"NES music (*.nsf)\0*.nsf\0\0":save?L"Music project (*.ngms)\0*.ngms\0\0":L"Project / game music\0*.ngms;*.nsf;*.nsfe;*.gsf;*.minigsf\0\0";f.lpstrDefExt=format==2?L"wav":format==1?L"nsf":L"ngms";f.Flags=OFN_EXPLORER|OFN_NOCHANGEDIR|OFN_PATHMUSTEXIST|(save?OFN_OVERWRITEPROMPT:OFN_FILEMUSTEXIST);return (save?GetSaveFileNameW(&f):GetOpenFileNameW(&f))?std::filesystem::path(path):std::filesystem::path{};}
    void save(){auto p=project.empty()?dialog(true):project;if(p.empty())return;Song copy=song;copy.title=utf8(p.stem().wstring());auto data=copy.serialize();atomicWrite(p,{data.begin(),data.end()});song=copy;project=p;savedSong=song;status=L"프로젝트를 저장했습니다.";}
    void load(const std::filesystem::path&p){auto ext=p.extension().wstring();std::transform(ext.begin(),ext.end(),ext.begin(),::towlower);if(ext==L".nsf"||ext==L".nsfe"||ext==L".gsf"||ext==L".minigsf"){importMusic(p);return;}selected=-1;if(std::filesystem::file_size(p)>=64u*1024u*1024u)throw std::runtime_error("Project exceeds 64 MiB");std::ifstream f(p,std::ios::binary);if(!f)throw std::runtime_error("Could not open project");std::string s((std::istreambuf_iterator<char>(f)),{});Song candidate=Song::parse(s);player.stop();song=std::move(candidate);project=p;savedSong=song;undo.clear();redo.clear();page=0;status=L"프로젝트를 열었습니다.";changed();}
    void exportNsf(){if(song.sampled){status=L"샘플 악기는 기본 NES NSF로 변환할 수 없습니다. 프로젝트 또는 WAV로 저장하세요.";return;}auto p=dialog(true,true);if(p.empty())return;atomicWrite(p,compileNsf(song));status=L"NSF 내보내기 완료 · 현재 곡 전체를 NES 채널로 연주합니다.";}
    void adopt(Song candidate){candidate.validate();remember();song=std::move(candidate);project.clear();savedSong.reset();page=0;selected=-1;for(auto&n:song.notes)if(n.channel==channel){low=std::clamp(n.pitch-12,lowMin(),lowMax());break;}status=song.sampled?L"GBA 원본 PCM 음표·샘플 · 필터/리버브·정확한 출력 지연 차이 있음 · PSG/일부 엔진 미지원":L"원곡 음표를 같은 피아노롤에 불러왔습니다 · 엔벨로프·슬라이드 재현에는 차이가 있습니다.";changed();}
    void importMusic(const std::filesystem::path&source={}){if(!confirm())return;player.releaseDevice();auto imported=runNoteImportDialog(window,source,song.bpm);if(imported)adopt(std::move(*imported));}
    void exportWavTo(const std::filesystem::path&p){player.pause();auto copy=song;cancelWav=false;wavProgress=0;wavJob=std::async(std::launch::async,[this,copy,p]{exportSongWav(copy,p,cancelWav,wavProgress);});status=L"현재 피아노롤의 수정된 음표·음색을 WAV로 생성 중…";}
    void exportWav(){auto p=dialog(true,2);if(!p.empty())exportWavTo(p);}
    void pollWav(){if(!wavJob.valid())return;if(wavJob.wait_for(std::chrono::seconds(0))!=std::future_status::ready){status=L"WAV 생성 중  "+std::to_wstring(wavProgress.load())+L"%";return;}try{wavJob.get();status=L"편집한 음표·음색을 반영한 WAV 저장 완료";}catch(const std::exception&e){status=wide(e.what());}changed();}
    void keyNote(WPARAM key,bool shift=false,bool control=false){if(selected<0||selected>=int(song.notes.size()))return;auto n=song.notes[selected];int index=selected;if(key==VK_DELETE){remember();song.notes.erase(song.notes.begin()+index);changed();return;}
        if(control&&(key==VK_UP||key==VK_DOWN))n.velocity=std::clamp(n.velocity+(key==VK_UP?1:-1),1,15);
        else if(key==VK_UP||key==VK_DOWN)n.pitch=std::clamp(n.pitch+(key==VK_UP?1:-1),pitchMin(),pitchMax());
        else if(key==VK_LEFT||key==VK_RIGHT){int delta=key==VK_RIGHT?unit():-unit();if(shift)n.length=std::clamp(n.length+delta,1,song.ticks()-n.step);else n.step=std::clamp(n.step+delta,0,song.ticks()-n.length);}else return;
        Song candidate=song;candidate.notes.erase(candidate.notes.begin()+index);candidate.put(n);remember();song=std::move(candidate);selected=int(song.notes.size())-1;changed();}
    void preparePreview(){player.stop();if(preview.empty()){wchar_t dir[MAX_PATH];if(!GetTempPathW(MAX_PATH,dir))throw std::runtime_error("Could not access temporary folder");auto own=std::filesystem::path(dir)/(L"NintendoMusicEditor-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64()));std::filesystem::create_directory(own);preview=own/L"preview.nsf";}atomicWrite(preview,compileNsf(song));}
    void play(){if(player.playing()){player.stop();status=L"재생 정지";}else{if(song.sampled){player.openPrepared(makeSampleBackend(song),Format::SampledSong);}else{preparePreview();player.open(preview);}player.master(.65f);player.play();status=song.sampled?L"편집한 음표·샘플로 미리 듣는 중 · WAV와 같은 연주":L"실제 NSF로 미리 듣는 중 · 변경 후 재생하면 새 연주가 반영됩니다.";}}
    void viewer(){if(song.sampled){player.releaseDevice();pendingSampleViewer=true;finished=true;ShowWindow(window,SW_HIDE);status=L"편집한 샘플 곡을 파형 화면에 보냈습니다.";return;}preparePreview();pendingViewer=preview;finished=true;ShowWindow(window,SW_HIDE);status=L"현재 곡을 같은 프로그램의 파형 화면에 보냈습니다.";}
    void dispatchViewer(){if(pendingSampleViewer&&showSampleViewer)showSampleViewer(song);else if(!pendingViewer.empty()&&showViewer)showViewer(pendingViewer);}
    void command(int id){if(wavJob.valid()){if(id==27)cancelWav=true;return;}
        switch(id){
        case 1:if(confirm()){player.stop();song=Song{};project.clear();savedSong=song;undo.clear();redo.clear();page=0;}break;
        case 2:if(confirm()){auto p=dialog(false);if(!p.empty())load(p);}break;
        case 3:save();break;case 4:exportWav();break;case 5:viewer();break;
        case 6:if(confirm()){remember();song=Song::demo();project.clear();savedSong.reset();page=0;}break;
        case 7:play();break;case 8:loop=!loop;break;
        case 9:if(!undo.empty()){selected=-1;player.stop();redo.push_back(song);song=undo.back();undo.pop_back();page=std::min(page,(song.bars-1)/4);}break;
        case 10:if(!redo.empty()){selected=-1;player.stop();undo.push_back(song);song=redo.back();redo.pop_back();page=std::min(page,(song.bars-1)/4);}break;
        case 11:if(song.bpm>30){remember();song.bpm=std::max(30,song.bpm-5);}break;
        case 12:if(song.bpm<300){remember();song.bpm=std::min(300,song.bpm+5);}break;
        case 13:if(song.bars>1&&MessageBoxW(window,L"마지막 마디와 그 안의 음표를 삭제할까요?",L"마디 줄이기",MB_YESNO)==IDYES){remember();song.bars--;song.notes.erase(std::remove_if(song.notes.begin(),song.notes.end(),[&](auto&n){return n.step+n.length>song.ticks();}),song.notes.end());page=std::min(page,(song.bars-1)/4);}break;
        case 14:if(song.bars<512){remember();song.bars++;}break;
        case 15:if(!song.sampled){remember();song.pal=!song.pal;}break;
        case 16:if(song.sampled){chooseSample();break;}if(channel!=2){if(selected>=0&&selected<int(song.notes.size())){auto n=song.notes[selected];int at=selected;remember();song.notes[at].duty=((n.duty>=0?n.duty:song.instruments[channel].duty)+1)%4;selected=at;}else{remember();song.instruments[channel].duty=(song.instruments[channel].duty+1)%4;}}break;
        case 17:if(song.instruments[channel].volume){remember();song.instruments[channel].volume--;}break;
        case 18:if(song.instruments[channel].volume<15){remember();song.instruments[channel].volume++;}break;
        case 19:remember();song.instruments[channel].mute=!song.instruments[channel].mute;break;
        case 20:page=std::max(0,page-1);break;case 21:page=std::min((song.bars-1)/4,page+1);break;
        case 22:low=std::max(lowMin(),low-12);break;case 23:low=std::min(lowMax(),low+12);break;
        case 24:remember();song.notes.erase(std::remove_if(song.notes.begin(),song.notes.end(),[&](auto&n){return n.channel==channel;}),song.notes.end());break;
        case 25:importMusic(importSource);importSource.clear();break;
        case 26:exportNsf();break;case 27:cancelWav=true;break;
        case 28:selected=-1;channel=std::max(0,(channel/4-1)*4);break;case 29:selected=-1;channel=std::min(int(song.instruments.size())-1,(channel/4+1)*4);break;
        default:if(id>=30&&id<94&&id-30<int(song.instruments.size())){selected=-1;channel=id-30;low=channel==2&&!song.sampled?36:48;for(auto&n:song.notes)if(n.channel==channel){low=std::clamp(n.pitch-12,lowMin(),lowMax());break;}}break;
        }changed();
    }
    void paint(HDC dc,int width,int height){
        fill(dc,{0,0,width,height},bg);scale=std::min(width/1280.,height/800.);ox=int((width-1280*scale)/2);oy=int((height-800*scale)/2);
        SetGraphicsMode(dc,GM_ADVANCED);XFORM transform{float(scale),0,0,float(scale),float(ox),float(oy)};SetWorldTransform(dc,&transform);buttons.clear();
        text(dc,L"NINTENDO  /  MUSIC STUDIO",{24,14,700,49},26,mint);text(dc,L"EDITOR  ·  1.4.9",{1040,14,1256,49},16,muted);
        button(dc,26,song.sampled?L"NSF 미지원":L"NSF 출력",640,20,125);button(dc,25,L"원곡 음표 가져오기",780,20,220);
        text(dc,wide(song.title)+(dirty()?L"  • 저장되지 않음":L""),{24,46,950,73},16,muted);
        button(dc,1,L"새 곡",24,84,76);button(dc,2,L"열기",108,84,76);button(dc,3,L"저장",192,84,76);button(dc,4,L"WAV 내보내기",276,84,146);button(dc,5,L"파형 뷰어",430,84,124);button(dc,6,L"예제",562,84,76);
        button(dc,9,L"되돌리기",660,84,110);button(dc,10,L"다시 실행",778,84,110);button(dc,8,L"반복",908,84,80,loop);button(dc,7,player.playing()?L"■ 정지":L"▶ 재생",1000,84,256,player.playing());
        fill(dc,{24,140,214,738},panel);text(dc,L"CHANNELS",{38,148,202,178},14,muted);
        const wchar_t*names[]={L"01   사각파 1",L"02   사각파 2",L"03   삼각파",L"04   노이즈"};int first=song.sampled?(channel/4)*4:0;
        for(int i=first;i<std::min(first+4,int(song.instruments.size()));i++)button(dc,30+i,song.sampled?std::to_wstring(i+1)+L"  "+(song.instruments[i].name.empty()?L"샘플 트랙":wide(song.instruments[i].name)):names[i],36,187+(i-first)*46,166,channel==i);
        if(song.instruments.size()>4){button(dc,28,L"◀ 트랙",36,369,78);button(dc,29,L"트랙 ▶",122,369,80);}
        text(dc,L"음색 · 선택 음표 / 트랙",{38,407,202,431},13,muted);int displayedDuty=song.instruments[channel].duty;if(selected>=0&&selected<int(song.notes.size())&&song.notes[selected].duty>=0)displayedDuty=song.notes[selected].duty;
        std::wstring duty;if(song.sampled){int sample=selected>=0&&selected<int(song.notes.size())&&song.notes[selected].sample>=0?song.notes[selected].sample:song.instruments[channel].sample;duty=wide(song.samples[size_t(sample)].name);}else duty=channel<2?std::wstring(L"듀티 ")+std::array<const wchar_t*,4>{L"12.5%",L"25%",L"50%",L"75%"}[displayedDuty]:channel==2?L"고정 삼각파":displayedDuty%2?L"짧은 노이즈":L"긴 노이즈";button(dc,16,duty,36,433,166);
        text(dc,L"볼륨  "+std::to_wstring(song.instruments[channel].volume)+L" / 15",{38,470,202,499},16,ink);button(dc,17,L"−",36,504,76);button(dc,18,L"+",124,504,78);button(dc,19,L"음소거",36,556,166,song.instruments[channel].mute);button(dc,24,L"채널 비우기",36,610,166);
        text(dc,selected>=0&&selected<int(song.notes.size())?L"선택 음표  음량 "+std::to_wstring(song.notes[selected].velocity)+L"/15":song.sampled?L"샘플 트랙 · 여러 음 동시 연주":L"한 채널에 한 음씩 연주",{36,672,205,708},12,muted);
        text(dc,L"BPM  "+std::to_wstring(song.bpm),{236,140,364,176},17,ink);button(dc,11,L"−",364,140,40);button(dc,12,L"+",410,140,40);text(dc,std::to_wstring(song.bars)+L" 마디",{470,140,560,176},17,ink);button(dc,13,L"−",560,140,40);button(dc,14,L"+",606,140,40);button(dc,15,song.sampled?L"PCM 샘플":song.pal?L"PAL · 50Hz":L"NTSC · 60Hz",670,140,158);button(dc,20,L"◀",856,140,48);button(dc,21,L"▶",910,140,48);button(dc,22,L"옥타브 −",990,140,126);button(dc,23,L"옥타브 +",1124,140,132);
        const int gx=290,gy=220,gw=966,gh=480,count=noise()?16:37;double rh=double(gh)/count,cw=double(gw)/64;
        for(int i=0;i<4;i++)text(dc,std::to_wstring(page*4+i+1)+L" 마디",{gx+i*gw/4,184,gx+(i+1)*gw/4,214},15,muted);
        const wchar_t*pitchNames[]={L"C",L"C#",L"D",L"D#",L"E",L"F",L"F#",L"G",L"G#",L"A",L"A#",L"B"};
        for(int row=0;row<count;row++){int pitch=noise()?15-row:low+36-row;int pc=pitch%12;bool black=pc==1||pc==3||pc==6||pc==8||pc==10;int y=gy+int(row*rh);fill(dc,{236,y,286,gy+int((row+1)*rh)-1},noise()?panel:black?RGB(24,31,45):RGB(43,54,71));std::wstring label=noise()?std::to_wstring(pitch):std::wstring(pitchNames[pc])+std::to_wstring(pitch/12-1);if(noise()||!black)text(dc,label,{241,y,285,gy+int((row+1)*rh)},11,ink);fill(dc,{gx,y,gx+gw,gy+int((row+1)*rh)-1},black?RGB(19,25,37):RGB(25,33,47));}
        for(int s=0;s<=64;s++){int x=gx+int(s*cw);fill(dc,{x,gy,x+(s%16==0?2:1),gy+gh},s%16==0?RGB(93,108,132):s%4==0?RGB(55,69,90):RGB(34,44,60));}
        auto drawNote=[&](const Note&n,bool ghost){int row=noise()?15-n.pitch:low+36-n.pitch;double a=std::max(double(n.step)/unit()-page*64,0.),b=std::min(double(n.step+n.length)/unit()-page*64,64.);if(row<0||row>=count||b<=a)return;RECT r{gx+int(a*cw)+2,gy+int(row*rh)+1,std::max(gx+int(a*cw)+4,gx+int(b*cw)-1),gy+int((row+1)*rh)-1};fill(dc,r,ghost?RGB(231,241,250):colors[channel%4]);};
        for(size_t i=0;i<song.notes.size();i++)if(song.notes[i].channel==channel&&!(dragging&&dragMode&&int(i)==selected))drawNote(song.notes[i],int(i)==selected);if(dragging)drawNote(draft,true);
        int valid=std::clamp(song.ticks()/unit()-page*64,0,64);if(valid<64)fill(dc,{gx+int(valid*cw),gy,gx+gw,gy+gh},RGB(11,15,23));
        if(player.playing()){double step=player.seconds()*song.bpm/15.-page*64;if(step>=0&&step<64){int x=gx+int(step*cw);fill(dc,{x,gy,x+2,gy+gh},RGB(255,255,255));}}
        text(dc,L"드래그: 입력 / 이동 / 끝부분 길이 · 방향키: 음높이·위치 · Shift+좌우: 길이 · Ctrl+위아래: 음량 · Delete: 삭제",{236,710,1256,740},14,muted);text(dc,status,{24,752,1060,786},15,ink);if(wavJob.valid())button(dc,27,L"WAV 취소",1080,750,176);
        XFORM identity{1,0,0,1,0,0};SetWorldTransform(dc,&identity);
    }
    bool point(int x,int y,int&step,int&pitch){x=int((x-ox)/scale);y=int((y-oy)/scale);if(x<290||x>=1256||y<220||y>=700)return false;step=(page*64+(x-290)*64/966)*unit();int row=(y-220)*(noise()?16:37)/480;pitch=noise()?15-row:low+36-row;return step<song.ticks();}
    void down(int x,int y,bool right){POINT p{int((x-ox)/scale),int((y-oy)/scale)};if(!right)for(auto&[id,r]:buttons)if(PtInRect(&r,p)){command(id);return;}if(wavJob.valid())return;int step,pitch;if(!point(x,y,step,pitch))return;
        if(right){auto it=std::find_if(song.notes.begin(),song.notes.end(),[&](auto&n){return n.channel==channel&&n.pitch==pitch&&step>=n.step&&step<n.step+n.length;});if(it!=song.notes.end()){int index=int(it-song.notes.begin());remember();song.notes.erase(song.notes.begin()+index);changed();}}
        else{double tick=(page*64+(p.x-290)*64./966)*unit();auto it=std::find_if(song.notes.begin(),song.notes.end(),[&](auto&n){return n.channel==channel&&n.pitch==pitch&&tick>=n.step&&tick<std::max(double(n.step+n.length),n.step+unit()*.2);});
            selected=it==song.notes.end()?-1:int(it-song.notes.begin());dragMode=0;dragStep=step;dragPitch=pitch;
            if(selected>=0){draft=*it;originalDraft=draft;double edge=290+(double(it->step+it->length)/unit()-page*64)*966/64;dragMode=p.x>=edge-5?2:1;}else draft={channel,step,unit(),pitch,15};dragging=true;if(!testing)SetCapture(window);changed();}}
    void move(int x,int y){if(!dragging)return;int step,pitch;if(point(x,y,step,pitch)){if(dragMode==1){draft.step=std::clamp(originalDraft.step+step-dragStep,0,song.ticks()-draft.length);draft.pitch=std::clamp(originalDraft.pitch+pitch-dragPitch,pitchMin(),pitchMax());}else draft.length=std::clamp(step-draft.step+unit(),1,song.ticks()-draft.step);changed();}}
    static LRESULT CALLBACK proc(HWND h,UINT m,WPARAM w,LPARAM l){Editor*e=reinterpret_cast<Editor*>(GetWindowLongPtrW(h,GWLP_USERDATA));if(m==WM_NCCREATE){e=static_cast<Editor*>(reinterpret_cast<CREATESTRUCTW*>(l)->lpCreateParams);e->window=h;SetWindowLongPtrW(h,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(e));}if(!e)return DefWindowProcW(h,m,w,l);
        try{switch(m){
        case WM_PAINT:{PAINTSTRUCT p;HDC dc=BeginPaint(h,&p);RECT r;GetClientRect(h,&r);HDC back=CreateCompatibleDC(dc);HBITMAP bm=CreateCompatibleBitmap(dc,r.right,r.bottom);auto old=SelectObject(back,bm);e->paint(back,r.right,r.bottom);BitBlt(dc,0,0,r.right,r.bottom,back,0,0,SRCCOPY);SelectObject(back,old);DeleteObject(bm);DeleteDC(back);EndPaint(h,&p);return 0;}
        case WM_ERASEBKGND:return 1;case WM_SIZE:e->changed();return 0;
        case WM_APP+25:e->command(25);return 0;
        case WM_LBUTTONDOWN:e->down(GET_X_LPARAM(l),GET_Y_LPARAM(l),false);return 0;case WM_RBUTTONDOWN:if(e->wavJob.valid())return 0;e->down(GET_X_LPARAM(l),GET_Y_LPARAM(l),true);return 0;
        case WM_MOUSEMOVE:e->move(GET_X_LPARAM(l),GET_Y_LPARAM(l));return 0;
        case WM_LBUTTONUP:if(e->dragging){e->dragging=false;if(!e->testing)ReleaseCapture();if(e->dragMode&&e->draft.step==e->originalDraft.step&&e->draft.length==e->originalDraft.length&&e->draft.pitch==e->originalDraft.pitch){e->changed();return 0;}int at=e->selected;Song candidate=e->song;if(e->dragMode&&at>=0)candidate.notes.erase(candidate.notes.begin()+at);candidate.put(e->draft);e->remember();e->song=std::move(candidate);e->selected=int(e->song.notes.size())-1;e->changed();}return 0;
        case WM_CAPTURECHANGED:e->dragging=false;return 0;
        case WM_MOUSEWHEEL:if(!e->noise())e->low=std::clamp(e->low+(GET_WHEEL_DELTA_WPARAM(w)>0?1:-1),e->lowMin(),e->lowMax());e->changed();return 0;
        case WM_KEYDOWN:if(e->wavJob.valid())return 0;if(w==VK_DELETE||w==VK_UP||w==VK_DOWN||w==VK_LEFT||w==VK_RIGHT)e->keyNote(w,(GetKeyState(VK_SHIFT)&0x8000)!=0,(GetKeyState(VK_CONTROL)&0x8000)!=0);else if(w==VK_SPACE)e->command(7);else if(w=='S'&&(GetKeyState(VK_CONTROL)&0x8000))e->command(3);else if(w=='O'&&(GetKeyState(VK_CONTROL)&0x8000))e->command(2);else if(w=='Z'&&(GetKeyState(VK_CONTROL)&0x8000))e->command(9);else if(w=='Y'&&(GetKeyState(VK_CONTROL)&0x8000))e->command(10);return 0;
        case WM_TIMER:e->pollWav();if(e->player.playing()&&e->player.seconds()>=e->song.seconds()){e->player.stop();if(e->loop){e->player.seek(0);e->player.play();}}InvalidateRect(h,nullptr,FALSE);return 0;
        case WM_GETMINMAXINFO:reinterpret_cast<MINMAXINFO*>(l)->ptMinTrackSize={960,640};return 0;
        case WM_CLOSE:if(e->wavJob.valid()){e->cancelWav=true;return 0;}e->player.stop();e->finished=true;ShowWindow(h,SW_HIDE);return 0;
        case WM_DESTROY:e->player.stop();e->finished=true;return 0;
        }}catch(const std::exception&ex){MessageBoxW(h,wide(ex.what()).c_str(),L"음악 에디터",MB_OK|MB_ICONERROR);}return DefWindowProcW(h,m,w,l);
    }
    void snapshot(const std::filesystem::path&path){HDC dc=GetDC(window),mem=CreateCompatibleDC(dc);BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=1280;info.bmiHeader.biHeight=-800;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;void*pixels=nullptr;auto bm=CreateDIBSection(dc,&info,DIB_RGB_COLORS,&pixels,nullptr,0);auto old=SelectObject(mem,bm);paint(mem,1280,800);BITMAPFILEHEADER header{};header.bfType=0x4d42;header.bfOffBits=sizeof(header)+sizeof(BITMAPINFOHEADER);header.bfSize=header.bfOffBits+1280*800*4;std::ofstream f(path,std::ios::binary);f.write(reinterpret_cast<char*>(&header),sizeof(header));f.write(reinterpret_cast<char*>(&info.bmiHeader),sizeof(info.bmiHeader));f.write(static_cast<char*>(pixels),1280*800*4);SelectObject(mem,old);DeleteObject(bm);DeleteDC(mem);ReleaseDC(window,dc);}
    void selftest(const std::filesystem::path&dir){
        auto require=[](bool b,const char*s){if(!b)throw std::runtime_error(s);};testing=true;
        auto verifySampleWav=[&](const std::filesystem::path&p){SampleSongRenderer expected(song);std::ifstream wav(p,std::ios::binary);require(bool(wav)&&std::filesystem::file_size(p)==56+expected.frames()*8,"Shared UI WAV size");wav.seekg(56);std::vector<Stereo>block,readback(4096);while(expected.position()<expected.frames()){auto count=size_t(std::min<uint64_t>(4096,expected.frames()-expected.position()));expected.render(count,block);wav.read(reinterpret_cast<char*>(readback.data()),std::streamsize(count*8));require(bool(wav)&&!std::memcmp(block.data(),readback.data(),count*8),"UI WAV differs from current edited Song");}};
        command(1);SendMessageW(window,WM_LBUTTONDOWN,0,MAKELPARAM(310,400));SendMessageW(window,WM_MOUSEMOVE,MK_LBUTTON,MAKELPARAM(380,400));SendMessageW(window,WM_LBUTTONUP,0,MAKELPARAM(380,400));
        require(song.notes.size()==1&&song.notes[0].length>=5,"Piano roll drag failed");auto length=song.notes[0].length;
        command(9);require(song.notes.empty(),"Undo failed");command(10);require(song.notes.size()==1&&song.notes[0].length==length,"Redo failed");
        SendMessageW(window,WM_RBUTTONDOWN,0,MAKELPARAM(310,400));require(song.notes.empty(),"Erase failed");command(9);require(song.notes.size()==1,"Erase undo failed");
        command(12);require(song.bpm==125,"Tempo change failed");command(19);require(song.instruments[0].mute,"Mute failed");command(19);
        std::filesystem::create_directories(dir);project=dir/L"UI test.ngms";save();auto original=song.serialize();load(project);require(song.serialize()==original&&!dirty(),"Save/load UI roundtrip failed");
        atomicWrite(dir/L"UI test.nsf",compileNsf(song));
        player.releaseDevice();
        std::atomic<bool>cancel{false};std::atomic<int>progress{0};require(!testNoteImportDialog(dir/L"UI test.nsf",125,.6,true),"Import cancel changed model");auto nesImported=testNoteImportDialog(dir/L"UI test.nsf",125,.6,false);require(bool(nesImported),"NES import dialog worker");adopt(std::move(*nesImported));
        require(!song.notes.empty()&&song.resolution==120,"Shared note import failed");selected=0;int oldPitch=song.notes[0].pitch;keyNote(VK_UP);require(song.notes[0].pitch==oldPitch+1,"Imported pitch edit failed");keyNote(VK_RIGHT);keyNote(VK_RIGHT,true);keyNote(VK_UP,false,true);
        project=dir/L"Imported edit.ngms";save();auto edited=song.serialize();load(project);require(song.serialize()==edited,"Shared imported project save/reopen failed");
        exportWavTo(dir/L"Imported edited.wav");for(int i=0;i<3000&&wavJob.valid();i++){pollWav();Sleep(10);}require(!wavJob.valid()&&wavProgress==100&&std::filesystem::exists(dir/L"Imported edited.wav"),"Shared piano-roll WAV export failed");snapshot(dir/L"shared-editor.bmp");
        play();Sleep(180);require(player.loaded()&&player.playing()&&player.seconds()>0,"NSF preview failed");player.stop();
        Song sampled;sampled.sampled=true;sampled.resolution=960;sampled.instruments.resize(8);for(auto&i:sampled.instruments)i.sample=0;
        sampled.samples.push_back({"Own sampled instrument",60,48000,true,0,std::make_shared<const std::vector<int8_t>>(std::initializer_list<int8_t>{0,64,0,-64})});
        sampled.samples.push_back({"Own second instrument",60,24000,true,0,std::make_shared<const std::vector<int8_t>>(std::initializer_list<int8_t>{64,-64,32,-32})});
        sampled.notes.push_back({7,0,1920,60,12});sampled.notes[0].sourceOffset=.5;sampled.notes[0].controls=std::make_shared<const std::vector<NoteControl>>(std::initializer_list<NoteControl>{{0,1,1,1},{480,2,.5,1}});
        player.releaseDevice();adopt(sampled);command(37);require(channel==7&&!noise(),"Sample tracks were mapped to NES noise");selected=0;
        keyNote(VK_UP);keyNote(VK_RIGHT);keyNote(VK_RIGHT,true);keyNote(VK_DOWN,false,true);setSample(1);auto editedSample=song.serialize();
        require(song.notes[0].pitch==61&&song.notes[0].velocity==11&&song.notes[0].sample==1&&song.notes[0].sourceOffset==.5,"Shared sampled musical edit failed");
        command(9);require(song.notes[0].sample==-1,"Sample instrument undo failed");command(10);require(song.serialize()==editedSample,"Sample instrument redo failed");
        project=dir/L"Own sampled edit.ngms";save();auto savedSample=song.serialize();load(project);require(!dirty()&&song.serialize()==savedSample,"Sampled project screen save/reopen failed");command(37);snapshot(dir/L"sampled-editor.bmp");
        play();Sleep(180);require(player.playing()&&player.seconds()>0&&player.channels().size()==8&&player.error().empty(),"Sampled Player preview failed");player.stop();
        exportWavTo(dir/L"Own sampled edited.wav");for(int i=0;i<3000&&wavJob.valid();i++){pollWav();Sleep(10);}require(!wavJob.valid()&&wavProgress==100&&std::filesystem::exists(dir/L"Own sampled edited.wav"),"Sampled piano-roll WAV failed");verifySampleWav(dir/L"Own sampled edited.wav");
        bool handedOff=false;auto retainedSample=song.serialize();showSampleViewer=[&](const Song&snapshot){require(snapshot.serialize()==retainedSample,"Waveform handoff changed Song");Player wave;wave.openPrepared(makeSampleBackend(snapshot),Format::SampledSong);wave.play();Sleep(100);require(wave.channels().size()==8&&wave.playing()&&wave.error().empty(),"Sampled waveform Player handoff");wave.releaseDevice();handedOff=true;};viewer();dispatchViewer();require(handedOff&&song.serialize()==retainedSample,"Sampled waveform handoff did not retain common editor Song");showSampleViewer={};pendingSampleViewer=false;finished=false;
        auto large=sampled;large.samples[0].pcm=std::make_shared<const std::vector<int8_t>>(600000,int8_t(8));auto largeText=large.serialize();atomicWrite(dir/L"Own large project.ngms",{largeText.begin(),largeText.end()});load(dir/L"Own large project.ngms");require(largeText.size()>1024u*1024u&&!dirty(),"Large sampled project was still capped at1MiB");auto begin=std::chrono::steady_clock::now();for(int i=0;i<1000;i++)require(!dirty(),"Bank-pointer dirty comparison");auto dirtyMicros=std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-begin).count();std::ofstream(dir/L"large-project-ui-metrics.json")<<"{\"project_bytes\":"<<largeText.size()<<",\"dirty_checks\":1000,\"elapsed_microseconds\":"<<dirtyMicros<<"}\n";
        auto retainedProject=song.serialize();atomicWrite(dir/L"Invalid project.ngms",{'N','G','M','S',' ','9','9'});bool rejected=false;try{load(dir/L"Invalid project.ngms");}catch(...){rejected=true;}require(rejected&&song.serialize()==retainedProject&&!dirty(),"Malformed project replaced current sampled Song");
        command(1);require(!song.sampled&&channel<4,"New NES song retained invalid sampled channel");command(9); // New-song deliberately clears undo.
        wchar_t actual[32768]{};if(GetEnvironmentVariableW(L"NGMV_TEST_GBA_SOURCE",actual,32768)){
            player.releaseDevice();require(!testNoteImportDialog(actual,120,6,true),"GBA import cancel replaced Song");auto gbaImported=testNoteImportDialog(actual,120,6,false);require(bool(gbaImported),"GBA import dialog worker");adopt(std::move(*gbaImported));require(song.sampled&&song.notes.size()>=20,"Actual GBA shared piano-roll adoption failed");channel=0;selected=0;keyNote(VK_UP);keyNote(VK_RIGHT);keyNote(VK_RIGHT,true);keyNote(VK_DOWN,false,true);setSample((song.notes[0].sample+1)%int(song.samples.size()));
            project=dir/L"Private actual edited.ngms";save();auto actualSaved=song.serialize();load(project);require(song.serialize()==actualSaved&&!dirty(),"Actual GBA UI save/reopen failed");play();Sleep(180);require(player.playing()&&player.channels().size()==6&&player.error().empty(),"Actual GBA edited preview failed");player.stop();exportWavTo(dir/L"Private actual edited.wav");for(int i=0;i<3000&&wavJob.valid();i++){pollWav();Sleep(10);}require(!wavJob.valid()&&wavProgress==100,"Actual GBA UI WAV timeout/failure");verifySampleWav(dir/L"Private actual edited.wav");snapshot(dir/L"private-actual-editor.bmp");
        }
        std::ofstream(dir/L"ui-test-result.json")<<"{\"pass\":true,\"piano_roll_drag\":true,\"erase\":true,\"undo_redo\":true,\"tempo_mute\":true,\"save_reload\":true,\"actual_NSF_preview_dummy_device\":true,\"shared_piano_roll_import_note_edit_save_WAV\":true,\"sampled_piano_roll_instrument_undo_project_preview_WAV\":true,\"sampled_waveform_Player_handoff\":true,\"large_sample_project_and_bank_pointer_dirty_check\":true,\"external_windows_or_input_touched\":false}\n";
    }
};
static Editor* persistentEditor=nullptr;
bool confirmMusicEditorExit(HWND owner){return !persistentEditor||!persistentEditor->dirty()||MessageBoxW(owner,L"작곡 화면에 저장하지 않은 변경 사항이 있습니다. 버리고 종료할까요?",L"Nintendo Music Studio",MB_YESNO|MB_ICONQUESTION)==IDYES;}
}
int ngmv::editor::runMusicEditor(HINSTANCE instance,HWND owner,const std::filesystem::path&image,bool snapshot,std::function<void(const std::filesystem::path&)> callback,bool analyze,const std::filesystem::path&initialMusic,std::function<void(const Song&)>sampleCallback){
    static std::unique_ptr<Editor> persistent;
    std::unique_ptr<Editor> isolated;if(!owner)isolated=std::make_unique<Editor>();else if(!persistent)persistent=std::make_unique<Editor>();Editor&e=owner?*persistent:*isolated;
    if(owner)persistentEditor=&e;
    e.finished=false;e.pendingViewer.clear();e.pendingSampleViewer=false;e.showViewer=callback;e.showSampleViewer=sampleCallback;bool test=!snapshot&&!image.empty();
    WNDCLASSW c{};c.lpfnWndProc=Editor::proc;c.hInstance=instance;c.lpszClassName=L"NintendoMusicEditorWindow";c.hCursor=LoadCursor(nullptr,IDC_ARROW);RegisterClassW(&c);
    HWND h=e.window;if(!h||!IsWindow(h))h=CreateWindowW(c.lpszClassName,L"Nintendo Music Studio 1.4.9 · 공통 음악 편집",WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,1280,840,owner,nullptr,instance,&e);if(!h)return 1;
    if(test){HDC dc=GetDC(h);e.paint(dc,1280,800);ReleaseDC(h,dc);try{e.selftest(image);DestroyWindow(h);return 0;}catch(const std::exception&x){std::filesystem::create_directories(image);std::ofstream(image/L"ui-test-error.txt")<<x.what();DestroyWindow(h);return 3;}}
    if(snapshot){HDC dc=GetDC(h),mem=CreateCompatibleDC(dc);BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=1280;info.bmiHeader.biHeight=-800;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;void*pixels=nullptr;auto bm=CreateDIBSection(dc,&info,DIB_RGB_COLORS,&pixels,nullptr,0);auto old=SelectObject(mem,bm);e.paint(mem,1280,800);BITMAPFILEHEADER file{};file.bfType=0x4d42;file.bfOffBits=sizeof(file)+sizeof(BITMAPINFOHEADER);file.bfSize=file.bfOffBits+1280*800*4;std::ofstream out(image,std::ios::binary);out.write(reinterpret_cast<char*>(&file),sizeof(file));out.write(reinterpret_cast<char*>(&info.bmiHeader),sizeof(info.bmiHeader));out.write(static_cast<char*>(pixels),1280*800*4);SelectObject(mem,old);DeleteObject(bm);DeleteDC(mem);ReleaseDC(h,dc);DestroyWindow(h);return out?0:2;}
    if(owner){EnableWindow(owner,FALSE);ShowWindow(owner,SW_HIDE);}ShowWindow(h,SW_SHOWNORMAL);SetTimer(h,1,33,nullptr);e.changed();if(analyze){e.importSource=initialMusic;PostMessageW(h,WM_APP+25,0,0);}MSG msg{};while(!e.finished&&GetMessageW(&msg,nullptr,0,0)>0){TranslateMessage(&msg);DispatchMessageW(&msg);}
    KillTimer(h,1);ShowWindow(h,SW_HIDE);e.player.releaseDevice();if(owner){EnableWindow(owner,TRUE);ShowWindow(owner,SW_SHOWNORMAL);SetForegroundWindow(owner);}e.dispatchViewer();return 0;
}

