#include "UiLanguage.hpp"
#include <map>
#include <atomic>
#include <commctrl.h>
namespace ngmv { namespace {
std::atomic<bool> english{false};
std::map<HWND,std::wstring> captions;
bool editable(HWND h){wchar_t cls[32]{};GetClassNameW(h,cls,32);return _wcsicmp(cls,L"ComboBox")==0||(_wcsicmp(cls,L"EDIT")==0&&!(GetWindowLongPtrW(h,GWL_STYLE)&ES_READONLY));}
LRESULT CALLBACK captionLife(HWND h,UINT m,WPARAM a,LPARAM b,UINT_PTR id,DWORD_PTR){if(m==WM_NCDESTROY){captions.erase(h);RemoveWindowSubclass(h,captionLife,id);}return DefSubclassProc(h,m,a,b);}
void watch(HWND h){SetWindowSubclass(h,captionLife,0x4e4d534c,0);}
using Pair=std::pair<std::wstring_view,std::wstring_view>;
const Pair words[]={
{L"닫기",L"Close"},
{L"플레이어 정보 · 지원 범위",L"About · Supported formats"},
{L"음악을 불러오는 중…",L"Loading music…"},
{L"다음 음악을 불러오는 중…",L"Loading next song…"},
{L"불러오는 중…",L"Loading…"},
{L"음악 파일을 열어 주세요",L"Open a music file"},
{L"음악을 열어 주세요",L"Open music first"},
{L"스펙트럼 (F6)",L"Spectrum (F6)"},
{L"파형 (F6)",L"Waveform (F6)"},
{L"막대형 ▾ (F7)",L"Bars ▾ (F7)"},
{L"선형 ▾ (F7)",L"Curve ▾ (F7)"},
{L"가로흐름 ▾ (F7)",L"Scrolling ▾ (F7)"},
{L"플레이어 정보 (F1)",L"About (F1)"},
{L"파일 열기",L"Open"},
{L"이전",L"Previous"},
{L"다음",L"Next"},
{L"일시정지",L"Pause"},
{L"재생",L"Play"},
{L"정지",L"Stop"},
{L"반복",L"Repeat"},
{L"녹화뷰",L"Recording view"},
{L"자동재생 켜짐",L"Autoplay on"},
{L"자동재생 꺼짐",L"Autoplay off"},
{L"전체 음량",L"Master volume"},
{L"피치",L"Pitch"},
{L"속도",L"Speed"},
{L"같은 폴더 음색",L"Local sound bank"},
{L"Windows 기본",L"Windows default"},
{L"외부 MIDI (F8)",L"External MIDI (F8)"},
{L"VMS 음색 자동 (F8)",L"VMS auto bank (F8)"},
{L"Windows 기본 (F8)",L"Windows default (F8)"},
{L"반향 켜짐",L"Reverb on"},
{L"반향 꺼짐",L"Reverb off"},
{L"단위 0.01",L"Step 0.01"},
{L"단위 0.10",L"Step 0.10"},
{L"반향 강도",L"Reverb amount"},
{L"잔향 시간",L"Decay time"},
{L"채널이 숨겨져 있습니다. F12 리모컨의 숨긴 채널에서 복원하세요.",L"Channels are hidden. Restore them in the F12 remote."},
{L"이 곡에서 사용하는 채널이 없습니다.",L"No channels are used in this song."},
{L"내보내기",L"Export"},
{L"모든 채널 음량",L"All channel volume"},
{L"혼합",L"Mixed"},
{L"리모컨 (F12)",L"Remote (F12)"},
{L"EQ 설정",L"EQ settings"},
{L"F1 정보 · F9 녹화뷰 · F11 전체화면",L"F1 About · F9 Recording · F11 Fullscreen"},
{L"기본 앱 설정... (F10)",L"Default apps... (F10)"},
{L"여기에 저장",L"Save here"},
{L"준비 중…",L"Preparing…"},
{L"내보내기를 취소했습니다.",L"Export cancelled."},
{L"내보내기 방식",L"Export mode"},
{L"사용 중인 채널별 WAV + master.wav",L"Used channel WAVs + master.wav"},
{L"원본 믹스 WAV (효과 제외)",L"Original mix WAV (without effects)"},
{L"길이: 0회 반복 시 초",L"Seconds (0 repeats)"},
{L"기록된 루프 반복",L"Recorded loop repeats"},
{L"폴더 선택 후 내보내기",L"Select folder and export"},
{L"취소",L"Cancel"},
{L"Nintendo Music Studio · 내보내기",L"Nintendo Music Studio · Export"},
{L"녹화뷰 끄기 (F9)",L"Leave recording view (F9)"},
{L"녹화뷰 켜기 (F9)",L"Enter recording view (F9)"},
{L"반복 켜짐",L"Repeat on"},
{L"반복 꺼짐",L"Repeat off"},
{L"종류: 막대형… (F7)",L"Style: bars… (F7)"},
{L"종류: 선형… (F7)",L"Style: curve… (F7)"},
{L"종류: 가로흐름… (F7)",L"Style: scrolling… (F7)"},
{L"모든 채널 혼합",L"All channels: mixed"},
{L"전체 채널 (공통)",L"All channels (shared)"},
{L"선택 채널",L"Selected channel"},
{L"이전 곡",L"Previous song"},
{L"다음 곡",L"Next song"},
{L"20밴드 EQ…",L"20-band EQ…"},
{L"녹화뷰 전환 (F9)",L"Recording view (F9)"},
{L"파형 / 스펙트럼 (F6)",L"Waveform / Spectrum (F6)"},
{L"스펙트럼 종류 (F7)",L"Spectrum style (F7)"},
{L"숨긴 채널…",L"Hidden channels…"},
{L"모든 채널 표시",L"Show all channels"},
{L"개별 해제",L"Use shared value"},
{L"구 파형 움직임 (중앙 기준)",L"Legacy waveform (center reference)"},
{L"코르스코프 파형",L"Corrscope waveform"},
{L"언어",L"Language"},
{L"Nintendo Music Studio · 리모컨",L"Nintendo Music Studio · Remote"},
{L"원하는 표시를 선택하세요. 바로 적용됩니다.",L"Choose a style. Changes apply immediately."},
{L"막대형 — 주파수별 막대",L"Bars — frequency bands"},
{L"선형 — 부드러운 곡선",L"Curve — smooth frequency plot"},
{L"가로흐름 — 옆으로 흐르는 스펙트로그램",L"Scrolling — horizontal spectrogram"},
{L"스펙트럼 종류",L"Spectrum style"},
{L"EQ 사용",L"Enable EQ"},
{L"모두 0 dB로 초기화",L"Reset all to 0 dB"},
{L"Nintendo Music Studio · 20밴드 이퀄라이저",L"Nintendo Music Studio · 20-band Equalizer"},
{L"직접 숨긴 채널입니다. 선택한 채널을 다시 표시할 수 있습니다.",L"Manually hidden channels. Select one to restore it."},
{L"직접 숨긴 채널이 없습니다.",L"No channels are manually hidden."},
{L"소리·음량은 그대로 유지됩니다. 미사용 채널의 자동 숨김은 별개입니다.",L"Audio and volume stay unchanged. Automatic unused-channel hiding is separate."},
{L"선택 채널 복원",L"Restore selected"},
{L"모두 복원",L"Restore all"},
{L"숨긴 채널",L"Hidden channels"},
{L"VirtualMIDISynth 사용 중인 SF2 가져오기…",L"Import active VirtualMIDISynth SF2…"},
{L"Windows 기본 음색 · 채널별 파형 / 효과",L"Windows default bank · channel waveforms / effects"},
{L"SF2 / DLS 파일 선택 · 채널별 파형 / 효과…",L"Choose SF2 / DLS · channel waveforms / effects…"},
{L"같은 폴더 음색 자동 탐색", L"Search same-folder sound banks"},
    {L"VirtualMIDISynth 음색 변경 자동 반영",L"Follow VirtualMIDISynth bank changes"},
{L"외부 MIDI 직접 출력 · 파형 없음",L"Direct external MIDI output · no waveform"},
{L"Windows MIDI 장치 출력 · 내부 효과/파형 미적용",L"Windows MIDI output · no internal effects/waveforms"},
{L"Windows 기본 음색 · 채널별 파형",L"Windows default bank · channel waveforms"},
};
const Pair prefixes[]={
{L"숨긴 채널 (",L"Hidden channels ("},
{L"모든 채널 ",L"All channels "},
{L"마스터 ",L"Master "},
{L"파형 간격: ",L"Waveform spacing: "},
{L"완료: ",L"Finished: "},
{L"내보내기 실패: ",L"Export failed: "},
{L"내보내는 중: ",L"Exporting: "},
{L"오류: ",L"Error: "},
{L"음소거 · ",L"Muted · "},
{L"MIDI 음색: ",L"MIDI bank: "},
{L"VirtualMIDISynth SF2 가져옴: ",L"VirtualMIDISynth SF2 imported: "},
{L"VirtualMIDISynth 음색 적용: ",L"VirtualMIDISynth bank applied: "},
{L"기존 음색 유지 · ",L"Kept previous bank · "},
{L"저장 위치 선택 — 선택한 위치에 ",L"Select location — create folder: "},
};
const Pair suffixes[]={
{L" (개별)",L" (individual)"},
{L"번 채널",L" · channel"},
{L"개 WAV\n",L" WAV files\n"},
{L" 폴더를 만듭니다",L""},
};
}
bool uiEnglish(){return english.load();}
std::wstring translateUi(std::wstring_view source){
 if(!uiEnglish())return std::wstring(source);
 if(source.find(L"Nintendo Music Studio 1.6.3.1h\r\n\r\n")==0){
  std::wstring about=L"Nintendo Music Studio 1.6.3.1h\r\n\r\nMusic playback, waveforms/spectra, channel volume/mute and folder playback.\r\n\r\nNSF/NSFE, SPC, GBS, GSF, 2SF, USF, BCSTM/BCWAV, BRSTM/BNS, ADX, VGM/VGZ.\r\nAudio: MP3, WAV, OGG (Vorbis), FLAC. MIDI: MID/MIDI, format 0/1, SF2/DLS or Windows MIDI output.\r\nWii/GameCube: supported stream files; disc/ROM music extraction is not supported.\r\nNES ROM: verified Super Mario Bros. NROM32 programs. GBA ROM: verified Super Mario Advance 1-4 revisions; not universal ROM support.\r\n\r\nF6 Waveform/Spectrum, F7 Spectrum style, F9 Recording view, F12 Remote, F8 MIDI bank.\r\nThe remote includes legacy waveform and Corrscope waveform options.\r\n\r\nProgram/component terms: LICENSE-REVIEW.md and licenses/. Music rights are separate from software licenses.\r\nThis modeless window does not block playback or channel controls.\r\n";
  auto licenses=source.find(L"Embedded component notices / licenses:");if(licenses!=std::wstring_view::npos)about+=source.substr(licenses);return about;
 }
 for(auto &item:words)if(source==item.first)return std::wstring(item.second);
 std::wstring result(source);
 for(auto &item:prefixes)if(source.substr(0,item.first.size())==item.first){result=std::wstring(item.second)+std::wstring(source.substr(item.first.size()));break;}
 // Numeric/channel suffixes are UI strings only. Do not translate metadata.
 for(auto &item:suffixes){auto at=result.find(item.first);if(at!=std::wstring::npos)result.replace(at,item.first.size(),item.second);}
 if(source.substr(0,3)==L"피치 ")result=L"Pitch "+std::wstring(source.substr(3));
 if(source.substr(0,3)==L"속도 ")result=L"Speed "+std::wstring(source.substr(3));
 return result;
}
BOOL setUiText(HWND h,const wchar_t* source){if(!h)return FALSE;if(!captions.count(h))watch(h);captions[h]=source?source:L"";return SetWindowTextW(h,translateUi(captions[h]).c_str());}
BOOL setUiDlgText(HWND h,int id,const wchar_t* source){return setUiText(GetDlgItem(h,id),source);}
void setUiLanguage(bool value){english=value;for(auto it=captions.begin();it!=captions.end();){if(!IsWindow(it->first)){it=captions.erase(it);continue;}if(!editable(it->first))SetWindowTextW(it->first,translateUi(it->second).c_str());++it;}}
void rememberUiTree(HWND h){
 auto remember=[](HWND child){if(editable(child))return;if(!captions.count(child)){watch(child);int n=GetWindowTextLengthW(child);std::wstring text(size_t(n)+1,L'\0');GetWindowTextW(child,text.data(),n+1);text.resize(size_t(n));captions[child]=text;}SetWindowTextW(child,translateUi(captions[child]).c_str());};
 captions.erase(h);remember(h);EnumChildWindows(h,[](HWND child,LPARAM fn)->BOOL{(*reinterpret_cast<decltype(remember)*>(fn))(child);return TRUE;},reinterpret_cast<LPARAM>(&remember));
}
}
