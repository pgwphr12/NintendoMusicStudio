#include "Window.hpp"
#include "ChannelLayout.hpp"
#include "app/WindowsIntegration.hpp"
#include "visualizer/Envelope.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <commdlg.h>
#include <cstdio>
#include <fstream>
#include <future>
#include <map>
#include <shellapi.h>
#include <windowsx.h>
namespace ngmv {
std::wstring wide(const std::string &s) {
    if (s.empty())
        return {};
    int n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s.data(), int(s.size()), nullptr, 0);
    UINT cp = CP_UTF8;
    if (!n) {
        cp = CP_ACP;
        n = MultiByteToWideChar(cp, 0, s.data(), int(s.size()), nullptr, 0);
    }
    std::wstring out(n, 0);
    MultiByteToWideChar(cp, 0, s.data(), int(s.size()), out.data(), n);
    return out;
}
// GDI objects are reused on the UI thread and released when the thread exits.
struct DrawingResources {
    std::map<COLORREF, HBRUSH> brushes;
    std::map<std::pair<COLORREF, int>, HPEN> pens;
    std::map<std::pair<int, bool>, HFONT> fonts;
    ~DrawingResources() {
        for (auto &entry : brushes)
            DeleteObject(entry.second);
        for (auto &entry : pens)
            DeleteObject(entry.second);
        for (auto &entry : fonts)
            DeleteObject(entry.second);
    }
    HBRUSH brush(COLORREF color) {
        auto &value = brushes[color];
        if (!value)
            value = CreateSolidBrush(color);
        return value;
    }
    HPEN pen(COLORREF color, int width) {
        auto &value = pens[{color, width}];
        if (!value)
            value = CreatePen(PS_SOLID, width, color);
        return value;
    }
    HFONT font(int size, bool bold) {
        auto &value = fonts[{size, bold}];
        if (!value)
            value = CreateFontW(-size, 0, 0, 0, bold ? FW_SEMIBOLD : FW_NORMAL, FALSE, FALSE, FALSE,
                                DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
        return value;
    }
};
static thread_local DrawingResources drawing;
static void fill(HDC dc, RECT r, COLORREF color) {
    FillRect(dc, &r, drawing.brush(color));
}
static void text(HDC dc, const std::wstring &s, int x, int y, int w, int h, int size,
                 COLORREF color, bool bold = false) {
    auto old = SelectObject(dc, drawing.font(size, bold));
    SetTextColor(dc, color);
    SetBkMode(dc, TRANSPARENT);
    RECT r{x, y, x + w, y + h};
    DrawTextW(dc, s.c_str(), -1, &r, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    SelectObject(dc, old);
}
static void line(HDC dc, int x1, int y1, int x2, int y2, COLORREF color, int width = 1) {
    auto old = SelectObject(dc, drawing.pen(color, width));
    MoveToEx(dc, x1, y1, nullptr);
    LineTo(dc, x2, y2);
    SelectObject(dc, old);
}
static std::wstring channelLabel(const std::string &name, bool) {
    return wide(name);
}
static std::wstring time(double s) {
    int n = std::max(0, int(s));
    wchar_t b[40];
    swprintf_s(b, L"%02d:%02d", n / 60, n % 60);
    return b;
}
static void button(HDC dc, const wchar_t *s, int x, int y, int w, bool active = false) {
    fill(dc, {x, y, x + w, y + 50}, active ? RGB(31, 76, 76) : RGB(28, 34, 46));
    text(dc, s, x + 16, y, w - 28, 50, 20, active ? RGB(101, 237, 204) : RGB(214, 220, 235));
}
static void volumeButton(HDC dc, const wchar_t *label, int x, int y, int size = 32) {
    fill(dc, {x, y, x + size, y + size - 2}, RGB(28, 34, 46));
    text(dc, label, x + size / 4, y, size * 3 / 4, size - 2, size * 5 / 8, RGB(214, 220, 235));
}
void Window::report(const std::exception &e) {
    message_ = wide(e.what());
    MessageBoxW(hwnd_, message_.c_str(), L"Nintendo Music Studio", MB_OK | MB_ICONERROR);
}
void Window::showAbout() {
    if (aboutWindow_ && IsWindow(aboutWindow_)) {
        ShowWindow(aboutWindow_, screenshot_.empty() ? SW_SHOWNORMAL : SW_SHOWNOACTIVATE);
        if (screenshot_.empty())
            SetForegroundWindow(aboutWindow_);
        return;
    }
    auto instance = reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(hwnd_, GWLP_HINSTANCE));
    WNDCLASSW wc{};
    wc.hInstance = instance;
    wc.lpszClassName = L"NintendoMusicStudioInfo";
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpfnWndProc = [](HWND h, UINT message, WPARAM a, LPARAM b) -> LRESULT {
        auto self = reinterpret_cast<Window *>(GetWindowLongPtrW(h, GWLP_USERDATA));
        if (message == WM_NCCREATE) {
            self = static_cast<Window *>(reinterpret_cast<CREATESTRUCTW *>(b)->lpCreateParams);
            SetWindowLongPtrW(h, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        if (message == WM_CREATE) {
            auto font = GetStockObject(DEFAULT_GUI_FONT);
            HWND body = CreateWindowExW(
                WS_EX_CLIENTEDGE, L"EDIT",
                L"Nintendo Music Studio 1.5.7\r\n\r\n재생 · 파형 · 채널 음량/음소거 · 폴더 "
                L"재생\r\n\r\n"
                L"NSF/NSFE · SPC · GBS · GSF · 2SF · USF · BCSTM/BCWAV · VGM/VGZ\r\n"
                L"일반 오디오: MP3 · WAV · OGG(Vorbis) · FLAC\r\n"
                L"추가 VGM: YM2612 · YM3812 · MSM6295 · K051649 · ES5506\r\n"
                L"ES5506은 칩 전체 MIX 버스입니다. 모든 칩/조합의 지원 완료를 뜻하지 "
                L"않습니다.\r\n\r\n"
                L"프로그램 라이선스와 구성별 조건: LICENSE-REVIEW.md / licenses 폴더.\r\n"
                L"음악 파일의 권리는 프로그램 코드와 별개입니다.\r\n\r\n"
                L"이 창을 열어 둔 상태에서도 본창의 재생·음량·채널 조작을 계속할 수 있습니다.",
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_MULTILINE | ES_READONLY | WS_VSCROLL, 12,
                12, 610, 260, h, reinterpret_cast<HMENU>(1), nullptr, nullptr);
            SendMessageW(body, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
            HWND close =
                CreateWindowW(L"BUTTON", L"닫기", WS_CHILD | WS_VISIBLE | WS_TABSTOP, 522, 285, 100,
                              32, h, reinterpret_cast<HMENU>(IDCANCEL), nullptr, nullptr);
            SendMessageW(close, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
            return 0;
        }
        if (message == WM_SIZE) {
            int width = LOWORD(b), height = HIWORD(b);
            MoveWindow(GetDlgItem(h, 1), 12, 12, std::max(40, width - 24),
                       std::max(40, height - 68), TRUE);
            MoveWindow(GetDlgItem(h, IDCANCEL), std::max(12, width - 112),
                       std::max(52, height - 44), 100, 32, TRUE);
            return 0;
        }
        if (message == WM_CLOSE || (message == WM_COMMAND && LOWORD(a) == IDCANCEL)) {
            DestroyWindow(h);
            return 0;
        }
        if (message == WM_DESTROY && self) {
            self->aboutWindow_ = nullptr;
            return 0;
        }
        return DefWindowProcW(h, message, a, b);
    };
    if (!RegisterClassW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
        throw std::runtime_error("Cannot register player information window.");
    RECT owner{};
    GetWindowRect(hwnd_, &owner);
    aboutWindow_ =
        CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_CONTROLPARENT, wc.lpszClassName,
                        L"플레이어 정보 · 지원 범위", WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                        owner.left + 24, owner.top + 48, 660, 370, hwnd_, nullptr, instance, this);
    if (!aboutWindow_)
        throw std::runtime_error("Cannot open player information.");
    ShowWindow(aboutWindow_, screenshot_.empty() ? SW_SHOWNORMAL : SW_SHOWNOACTIVATE);
    if (screenshot_.empty())
        SetFocus(GetDlgItem(aboutWindow_, IDCANCEL));
}
struct ChangingScope {
    bool &value;
    explicit ChangingScope(bool &flag) : value(flag) {
        value = true;
    }
    ~ChangingScope() {
        value = false;
    }
};
// Only a separate, unpublished decoder is touched by this worker. The active
// player and all HWND operations remain on the UI thread.
static std::unique_ptr<IMusicBackend> prepareMusic(HWND window, const std::filesystem::path &path) {
    auto task =
        std::async(std::launch::async, [path] { return makeBackend(readMusicFile(path), path); });
    bool quit = false;
    int exitCode = 0;
    while (task.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready) {
        MSG msg{};
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                quit = true;
                exitCode = int(msg.wParam);
                continue;
            }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        MsgWaitForMultipleObjects(0, nullptr, FALSE, 15, QS_ALLINPUT);
    }
    try {
        auto result = task.get();
        if (quit)
            PostQuitMessage(exitCode);
        return IsWindow(window) ? std::move(result) : nullptr;
    } catch (...) {
        if (quit)
            PostQuitMessage(exitCode);
        if (!IsWindow(window))
            return nullptr;
        throw;
    }
}
bool Window::open(const std::filesystem::path &p) {
    if (changing_)
        return false;
    try {
        ChangingScope guard(changing_);
        auto playlist = FolderPlaylist::scan(p, identify(readMusicFile(p),p));
        rememberChannels();
        message_ = L"음악을 불러오는 중…";
        drag_ = -1;
        InvalidateRect(hwnd_, nullptr, FALSE);
        auto prepared = prepareMusic(hwnd_, p);
        if (!prepared)
            return false;
        player_.openPrepared(std::move(prepared), identify(readMusicFile(p),p));
        restoreChannels();
        file_ = p;
        SetWindowTextW(hwnd_,
                       (L"Nintendo Music Studio · 1.5.7 — " + file_.filename().wstring()).c_str());
        playlist_ = std::move(playlist);

        drag_ = -1;
        message_.clear();

        InvalidateRect(hwnd_, nullptr, FALSE);
        return true;
    } catch (const std::exception &e) {
        report(e);
        return false;
    }
}
void Window::selectFolder(size_t index, bool play) {
    if (changing_ || index >= playlist_.files.size())
        return;
    ChangingScope guard(changing_);
    const auto path = playlist_.files[index];
    rememberChannels();
    message_ = L"다음 음악을 불러오는 중…";
    drag_ = -1;
    InvalidateRect(hwnd_, nullptr, FALSE);
    auto prepared = prepareMusic(hwnd_, path);
    if (!prepared)
        return;
    player_.openPrepared(std::move(prepared), identify(readMusicFile(path),path));
    restoreChannels();
    playlist_.current = index;
    file_ = path;
    SetWindowTextW(hwnd_, (L"Nintendo Music Studio · 1.5.7 — " + file_.filename().wstring()).c_str());

    drag_ = -1;
    message_.clear();
    if (play)
        player_.play();
    InvalidateRect(hwnd_, nullptr, FALSE);
}
void Window::nextTrack(int direction, bool automatic) {
    if (changing_ || !player_.loaded())
        return;
    if (playlist_.active()) {
        if (automatic && playlist_.current + 1 >= playlist_.files.size())
            return;
        auto size = playlist_.files.size();
        size_t next =
            direction > 0 ? (playlist_.current + 1) % size : (playlist_.current + size - 1) % size;
        selectFolder(next, automatic || player_.playing());
    } else if (!automatic && !player_.tracks().empty()) {
        int size = int(player_.tracks().size());
        player_.select((player_.track() + size + direction) % size);
    }
}
void Window::openDialog() {
    wchar_t path[32768] = {};
    OPENFILENAMEW d{};
    d.lStructSize = sizeof d;
    d.hwndOwner = hwnd_;
    d.lpstrFilter =
        L"음악 파일 "
        L"(*.nsf;*.nsfe;*.spc;*.gbs;*.gsf;*.minigsf;*.2sf;*.mini2sf;*.usf;*.miniusf;*."
        L"bcstm;*.bcwav;*.vgm;*.vgz;*.mp3;*.wav;*.ogg;*.flac)\0*.nsf;*.nsfe;*.spc;*.gbs;*.gsf;*.minigsf;*.2sf;*.mini2sf;*."
        L"usf;*.miniusf;*.bcstm;*.bcwav;*.vgm;*.vgz;*.mp3;*.wav;*.ogg;*.flac\0모든 파일\0*.*\0";
    d.lpstrFile = path;
    d.nMaxFile = 32768;
    d.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (GetOpenFileNameW(&d))
        open(path);
}
void Window::tracksMenu() {
    if (!player_.loaded())
        return;
    auto menu = CreatePopupMenu();
    int index = 0;
    if (playlist_.active()) {
        for (const auto &path : playlist_.files) {
            auto title = path.stem().wstring();
            AppendMenuW(menu, MF_STRING | (size_t(index) == playlist_.current ? MF_CHECKED : 0),
                        1000 + index, title.c_str());
            ++index;
        }
    } else
        for (const auto &t : player_.tracks()) {
            std::wstring title =
                std::to_wstring(index + 1) + L"  " +
                (t.title.empty() ? L"Track " + std::to_wstring(index + 1) : wide(t.title));
            AppendMenuW(menu, MF_STRING | (index == player_.track() ? MF_CHECKED : 0), 1000 + index,
                        title.c_str());
            index++;
        }
    POINT p;
    GetCursorPos(&p);
    auto chosen = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_NONOTIFY, p.x, p.y, 0, hwnd_, nullptr);
    DestroyMenu(menu);
    if (chosen >= 1000) {
        if (playlist_.active())
            selectFolder(chosen - 1000, player_.playing());
        else
            player_.select(chosen - 1000);
    }
}
void Window::rateMenu() {
    auto menu = CreatePopupMenu();
    const int rates[] = {16000, 22050, 32000, 44100, 48000, 96000};
    for (int i = 0; i < 6; i++)
        AppendMenuW(menu, MF_STRING | (rates[i] == player_.rate() ? MF_CHECKED : 0), i + 1,
                    (std::to_wstring(rates[i]) + L" Hz").c_str());
    POINT p;
    GetCursorPos(&p);
    auto n = TrackPopupMenu(menu, TPM_RETURNCMD, p.x, p.y, 0, hwnd_, nullptr);
    DestroyMenu(menu);
    if (n >= 1 && n <= 6)
        player_.rate(rates[n - 1]);
}
void Window::updatePlayback(){
    // Both display messages and the idle WM_TIMER fallback service transport.
    // UI-thread work only; never a paint/audio callback or concurrent restart.
    if(changing_ || !player_.playing())return;
    const bool scheduledEnd=player_.playbackBoundaryReached();
    const bool advance=player_.repeat() || (autoplay_ && playlist_.active() &&
                                          playlist_.current+1<playlist_.files.size());
    if(!player_.ended() && !(scheduledEnd && advance))return;
    const bool repeat=player_.repeat();
    player_.pause();
    if(repeat){player_.stop();player_.play();}
    else if(autoplay_ && playlist_.active() && playlist_.current+1<playlist_.files.size())nextTrack(1,true);
    else message_=L"Track ended";
}
void Window::toggleFullscreen() {
    fullscreen_ = !fullscreen_;
    if (fullscreen_) {
        GetWindowRect(hwnd_, &oldRect_);
        SetWindowLongPtrW(hwnd_, GWL_STYLE, WS_POPUP | WS_VISIBLE);
        MONITORINFO m{sizeof m};
        GetMonitorInfoW(MonitorFromWindow(hwnd_, MONITOR_DEFAULTTONEAREST), &m);
        const bool isolated=!screenshot_.empty() && desktop::automatedSession();
        SetWindowPos(hwnd_, isolated ? nullptr : HWND_TOP, isolated ? -20000 : m.rcMonitor.left,
                     isolated ? -20000 : m.rcMonitor.top,
                     m.rcMonitor.right - m.rcMonitor.left, m.rcMonitor.bottom - m.rcMonitor.top,
                     SWP_FRAMECHANGED | (isolated ? SWP_NOACTIVATE|SWP_NOZORDER : 0));
    } else {
        SetWindowLongPtrW(hwnd_, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE);
        SetWindowPos(hwnd_, nullptr, oldRect_.left, oldRect_.top, oldRect_.right - oldRect_.left,
                     oldRect_.bottom - oldRect_.top, SWP_FRAMECHANGED | SWP_NOZORDER |
                     ((!screenshot_.empty() && desktop::automatedSession()) ? SWP_NOACTIVATE : 0));
    }
}
std::vector<size_t> Window::visibleChannels() const {
    std::vector<size_t> indices;
    const auto &channels = player_.channels();
    const bool dmcUsed = player_.dmcUsed();
    for (size_t i = 0; i < channels.size(); ++i)
        if ((channels[i].name != "DMC" || dmcUsed) && player_.channelVisible(i))
            indices.push_back(i);
    return indices;
}
void Window::paint(HDC dc, int width, int height) {
    segments_.clear();
    overlayText_.clear();
    segments_.reserve(8 * 1576 * 2);
    fill(dc, {0, 0, width, height}, RGB(6, 9, 14));
    scale_ = std::min(width / 1920., height / 1080.);
    offsetX_ = int((width - 1920 * scale_) / 2);
    offsetY_ = int((height - 1080 * scale_) / 2);
    int saved = SaveDC(dc);
    SetGraphicsMode(dc, GM_ADVANCED);
    XFORM xf{FLOAT(scale_), 0, 0, FLOAT(scale_), FLOAT(offsetX_), FLOAT(offsetY_)};
    SetWorldTransform(dc, &xf);
    fill(dc, {0, 0, 1920, 1080}, RGB(12, 16, 24));
    if (changing_) {
        text(dc, recording_ ? L"Loading…" : L"불러오는 중…", 42, 22, 1810, 62, 46,
             RGB(235, 240, 249));
        RestoreDC(dc, saved);
        return;
    }
    const auto &tracks = player_.tracks();
    TrackInfo info;
    if (player_.loaded() && size_t(player_.track()) < tracks.size())
        info = tracks[player_.track()];
    std::wstring title =
        player_.loaded() ? (info.title.empty() ? L"Track " + std::to_wstring(player_.track() + 1)
                                               : wide(info.title))
                         : (recording_ ? L"Open a music file" : L"음악 파일을 열어 주세요");
    text(dc, title, 42, 22, recording_ ? 1810 : 1400, 62, 46, RGB(235, 240, 249), true);
    if (!recording_)
        button(dc, L"플레이어 정보 (F1)", 1495, 22, 375);
    std::wstring game =
        info.game.empty()
            ? (file_.empty()
                   ? L"NSF · NSFE · SPC · GBS · GSF · 2SF · USF · BCSTM · BCWAV · VGM · VGZ"
                   : file_.stem().wstring())
            : wide(info.game);
    text(dc,
         game + L"  /  " + wide(info.system) +
             (info.author.empty() ? L"" : L"  /  " + wide(info.author)),
         44, 89, 1600, 32, 22, RGB(150, 166, 189));
    text(dc,
         time(player_.seconds()) + L" / " +
             (info.lengthMs > 0 ? time(info.lengthMs / 1000.) : L"--:--"),
         recording_ ? 44 : 1625, recording_ ? 125 : 91, recording_ ? 1800 : 260,
         recording_ ? 72 : 30, recording_ ? 60 : 24, RGB(232, 206, 122), recording_);
    if (!recording_) {
        button(dc, L"파일 열기", 42, 141, 155);
        button(dc, L"Tracks", 212, 141, 155);
        button(dc, L"이전", 382, 141, 115);
        button(dc, player_.playing() ? L"일시정지" : L"재생", 512, 141, 150, player_.playing());
        button(dc, L"정지", 677, 141, 115);
        button(dc, L"다음", 807, 141, 115);
        button(dc, L"반복", 937, 141, 115, player_.repeat());
        button(dc, L"녹화뷰", 1067, 141, 115);
        button(dc, autoplay_ ? L"자동재생 켜짐" : L"자동재생 꺼짐", 1197, 141, 160, autoplay_);
        button(dc, (std::to_wstring(player_.rate()) + L" Hz").c_str(), 1372, 141, 147);
    }
    if (!recording_) {
        text(dc, recording_ ? L"Volume" : L"전체 음량", 1550, 131, 140, 28, 18, RGB(154, 168, 190));
        volumeButton(dc, L"−", 1550, 163);
        line(dc, 1594, 178, 1798, 178, RGB(48, 59, 77), 5);
        line(dc, 1594, 178, 1594 + int(204 * player_.master()), 178, RGB(80, 203, 180), 5);
        volumeButton(dc, L"+", 1810, 163);
        text(dc, std::to_wstring(int(std::round(player_.master() * 100))) + L"%", 1850, 159, 65, 35,
             18, RGB(196, 208, 225));
    }
    if (!recording_) {
        wchar_t speed[64];
        swprintf_s(speed, L"피치 %.2f×", player_.pitch());
        text(dc, speed, 42, 205, 165, 50, 22, RGB(196, 208, 225));
        button(dc, L"−", 212, 205, 70);
        button(dc, L"+", 292, 205, 70);
        button(dc, L"1.00", 372, 205, 80);
        swprintf_s(speed, L"속도 %.2f×", player_.speed());
        text(dc, speed, 472, 205, 165, 50, 22, RGB(196, 208, 225));
        button(dc, L"−", 642, 205, 70);
        button(dc, L"+", 722, 205, 70);
        button(dc, L"1.00", 802, 205, 80);
        button(dc, player_.reverb() ? L"반향 켜짐" : L"반향 꺼짐", 1240, 1028, 180,
               player_.reverb());
        button(dc, speedStep_ == 1 ? L"단위 0.01" : L"단위 0.10", 917, 205, 200);
        text(dc, L"반향 강도", 1140, 205, 130, 50, 20, RGB(154, 168, 190));
        volumeButton(dc, L"−", 1276, 215);
        line(dc, 1320, 230, 1450, 230, RGB(48, 59, 77), 5);
        line(dc, 1320, 230, 1320 + int(130 * player_.reverbAmount()), 230, RGB(80, 203, 180), 5);
        volumeButton(dc, L"+", 1462, 215);
        text(dc, std::to_wstring(int(std::round(player_.reverbAmount() * 100))) + L"%", 1505, 205,
             75, 50, 20, RGB(196, 208, 225));
        text(dc, L"잔향 시간", 1590, 205, 105, 50, 20, RGB(154, 168, 190));
        volumeButton(dc, L"−", 1700, 215);
        wchar_t decay[32];
        swprintf_s(decay, L"%.1f s", player_.reverbDecay());
        text(dc, decay, 1745, 205, 75, 50, 20, RGB(196, 208, 225));
        volumeButton(dc, L"+", 1830, 215);
    }
    const auto &channels = player_.channels();
    auto waves = player_.waveforms();
    auto states = player_.states();
    const COLORREF colors[] = {RGB(247, 205, 87),  RGB(126, 222, 190), RGB(137, 180, 249),
                               RGB(222, 146, 232), RGB(250, 155, 106), RGB(181, 211, 119),
                               RGB(126, 213, 228), RGB(247, 163, 181)};
    auto visible = visibleChannels();

    auto labelText=[&](const std::wstring&s,int x,int y,int w,int h,int size,COLORREF color,bool bold=false){
        if(recording_)overlayText_.push_back({s,x,y,w,h,size,color,bold});
        else text(dc,s,x,y,w,h,size,color,bold);
    };
    size_t count = visible.size();
    for (size_t i = 0; i < count; i++) {
        size_t c = visible[i];
        if (c >= waves.size() || c >= states.size() || c >= channels.size())
            continue;
        auto cell = channelCell(i, count, recording_);
        int y = cell.y, lane = cell.height;
        fill(dc, {cell.x, y, cell.x + cell.width, y + lane - 8},
             i % 2 ? RGB(16, 22, 32) : RGB(18, 24, 35));
        float peak = 0;
        for (float v : waves[c])
            peak = std::max(peak, std::abs(v));
        COLORREF color = states[c].mute ? RGB(96, 103, 119) : colors[c % 8];
        std::wstring label = channelLabel(channels[c].name, recording_);
        if (cell.compact && channels[c].name == "EFFECTS / OTHER")
            label = L"FX / OTHER";
        if (cell.compact && !recording_)
            label += L" " + std::to_wstring(int(std::round(states[c].volume * 100))) + L"%";
        if (cell.compact && states[c].mute)
            label += L" MUTED";
        labelText(label, cell.labelX(),
             y + (recording_ ? 2 : cell.compact ? 2
                  : lane < 100 ? 5
                               : 15),
             cell.split ? 140 : 205,
             cell.compact ? 18
             : recording_ ? (lane < 100 ? 26 : 30)
             : lane < 100 ? 30
                          : 36,
             cell.compact ? 13 : std::min(cell.split ? 20 : 24, lane / 4), color, true);
        if (!cell.compact) {
            std::wstring status;
            if (recording_) {
                if (states[c].mute)
                    status = L"MUTED";
            } else
                status = (states[c].mute ? L"음소거 · " : L"") +
                         std::to_wstring(int(std::round(states[c].volume * 100))) + L"%";
            if (!status.empty())
                labelText(status, cell.labelX(), y + (recording_ ? (lane < 100 ? 30 : 36) : lane < 100 ? 35 : 48), cell.split ? 140 : 178,
                     lane < 100 ? 14 : 22, lane < 100 ? 12 : 15, RGB(118, 136, 158));
        }
        if (!recording_) {
            volumeButton(dc, L"−", cell.labelX(), cell.volumeY(), cell.buttonSize());
            int sliderY = cell.volumeY() + cell.buttonSize() / 2;
            line(dc, cell.sliderLeft(), sliderY, cell.sliderRight(), sliderY, RGB(47, 59, 78), 3);
            line(dc, cell.sliderLeft(), sliderY,
                 cell.sliderLeft() +
                     int((cell.sliderRight() - cell.sliderLeft()) * states[c].volume / 2),
                 sliderY, color, 3);
            volumeButton(dc, L"+", cell.plusX(), cell.volumeY(), cell.buttonSize());
        }
        int x0 = cell.waveLeft(recording_), x1 = cell.waveRight(recording_), center = y + (lane - 8) / 2;
        float amp = (lane - 22) * .44f;
        line(dc, x0, center, x1, center, RGB(44, 56, 73));
        for (int x = x0; x < x1; x += int((x1 - x0) / 8))
            line(dc, x, y + 12, x, y + lane - 18, RGB(29, 39, 54));
        auto env = envelope(waves[c], size_t(x1 - x0), size_t(windowSamples_));

        float mean = 0;
        for (float v : waves[c])
            mean += v;
        mean /= float(std::max(size_t(1), waves[c].size()));
        float gain = peak > .0001f ? std::min(64.f, .65f / peak) : 1;
        int previous = center;
        for (size_t x = 0; x < env.size(); x++) {
            auto point = env[x];
            auto py = [&](float v) {
                return center - int(std::clamp((v - mean) * gain, -1.f, 1.f) * amp);
            };
            int low = py(point.minimum), high = py(point.maximum), mid = py(point.center);
            if (low != high)
                segments_.push_back({{x0 + LONG(x), low}, {x0 + LONG(x), high}, color});
            if (x)
                segments_.push_back({{x0 + LONG(x) - 1, previous}, {x0 + LONG(x), mid}, color});
            previous = mid;
        }
    }
    if (info.lengthMs > 0 && !recording_) {
        int x = 42 + int(std::clamp(player_.seconds() * 1000 / info.lengthMs, 0., 1.) * 1840);
        line(dc, 42, 1016, 1882, 1016, RGB(42, 52, 69), 5);
        line(dc, 42, 1016, x, 1016, RGB(74, 196, 173), 5);
    }
    if (!recording_) {
        float group = states.empty() ? 1.f : states.front().volume;
        bool mixed = std::any_of(states.begin(), states.end(),
                                 [&](const auto &v) { return std::abs(v.volume - group) > .001f; });
        text(dc, L"모든 채널 음량", 42, 1034, 175, 40, 20, RGB(154, 168, 190));
        volumeButton(dc, L"−", 222, 1038);
        line(dc, 264, 1054, 468, 1054, RGB(48, 59, 77), 5);
        line(dc, 264, 1054, 264 + int(102 * group), 1054, RGB(80, 203, 180), 5);
        volumeButton(dc, L"+", 480, 1038);
        text(dc, mixed ? L"혼합" : std::to_wstring(int(std::lround(group * 100))) + L"%", 530, 1034,
             95, 40, 20, RGB(196, 208, 225));
        text(dc, L"F1 정보 · F9 녹화뷰 · F11 전체 화면", 660, 1034, 565, 40, 18,
             RGB(112, 130, 153));
    }
    if (!player_.error().empty())
        text(dc, (recording_ ? L"Error: " : L"오류: ") + wide(player_.error()), 42, 1048, 1800, 28,
             18, RGB(245, 131, 131));
    RestoreDC(dc, saved);
}
void Window::mouseMove(int x, int y) {
    if (drag_ < 0)
        return;
    int lx = int((x - offsetX_) / scale_);
    if (recording_)
        return;
    if (drag_ == 102)
        player_.channelVolumes(std::clamp(2.f * (lx - 264) / 204.f, 0.f, 2.f));
    else if (drag_ == 100)
        player_.master(std::clamp((lx - 1594) / 204.f, 0.f, 1.f));
    else if (drag_ == 101)
        player_.reverbAmount(std::clamp((lx - 1320) / 130.f, 0.f, 1.f));
    else {
        auto states = player_.states();
        if (size_t(drag_) < states.size()) {
            auto visible = visibleChannels();
            auto found = std::find(visible.begin(), visible.end(), size_t(drag_));
            if (found == visible.end())
                return;
            auto cell = channelCell(size_t(found - visible.begin()), visible.size(), recording_);
            states[drag_].volume = std::clamp(2.f * (lx - cell.sliderLeft()) /
                                                  (cell.sliderRight() - cell.sliderLeft()),
                                              0.f, 2.f);
            player_.channel(drag_, states[drag_]);
        }
    }
    InvalidateRect(hwnd_, nullptr, FALSE);
}
void Window::click(int x, int y) {
    if (changing_)
        return;
    int lx = int((x - offsetX_) / scale_), ly = int((y - offsetY_) / scale_);
    try {
        if (!recording_ && ly >= 22 && ly < 72 && lx >= 1495 && lx < 1870) {
            showAbout();
            return;
        }
        if (!recording_ && ly >= 163 && ly < 193 && lx >= 1550 && lx < 1842) {
            if (lx < 1582)
                player_.master((std::lround(player_.master() * 100) - 5) / 100.f);
            else if (lx >= 1810)
                player_.master((std::lround(player_.master() * 100) + 5) / 100.f);
            else if (lx >= 1594 && lx < 1798) {
                drag_ = 100;
                SetCapture(hwnd_);
                mouseMove(x, y);
            }
            InvalidateRect(hwnd_, nullptr, FALSE);
            return;
        }
        if (!recording_ && ly >= 1028 && ly < 1078) {
            auto states = player_.states();
            float value = states.empty() ? 1.f : states.front().volume;
            if (lx >= 222 && lx < 254)
                player_.channelVolumes((std::lround(value * 100) - 5) / 100.f);
            else if (lx >= 480 && lx < 512)
                player_.channelVolumes((std::lround(value * 100) + 5) / 100.f);
            else if (lx >= 264 && lx <= 468) {
                drag_ = 102;
                SetCapture(hwnd_);
                mouseMove(x, y);
            } else if (lx >= 1240 && lx < 1420)
                player_.reverb(!player_.reverb());
            InvalidateRect(hwnd_, nullptr, FALSE);
            return;
        }
        if (!recording_ && ly >= 205 && ly < 255) {
            if (lx >= 212 && lx < 282)
                player_.pitch((std::lround(player_.pitch() * 100) - speedStep_) / 100.f);
            else if (lx >= 292 && lx < 362)
                player_.pitch((std::lround(player_.pitch() * 100) + speedStep_) / 100.f);
            else if (lx >= 372 && lx < 452)
                player_.pitch(1);
            else if (lx >= 642 && lx < 712)
                player_.speed((std::lround(player_.speed() * 100) - speedStep_) / 100.f);
            else if (lx >= 722 && lx < 792)
                player_.speed((std::lround(player_.speed() * 100) + speedStep_) / 100.f);
            else if (lx >= 802 && lx < 882)
                player_.speed(1);
            else if (lx >= 917 && lx < 1117)
                speedStep_ = speedStep_ == 1 ? 10 : 1;
            else if (ly >= 215 && ly < 245) {
                if (lx >= 1276 && lx < 1308)
                    player_.reverbAmount((std::lround(player_.reverbAmount() * 100) - 5) / 100.f);
                else if (lx >= 1462 && lx < 1494)
                    player_.reverbAmount((std::lround(player_.reverbAmount() * 100) + 5) / 100.f);
                else if (lx >= 1700 && lx < 1732)
                    player_.reverbDecay((std::lround(player_.reverbDecay() * 10) - 1) / 10.f);
                else if (lx >= 1830 && lx < 1862)
                    player_.reverbDecay((std::lround(player_.reverbDecay() * 10) + 1) / 10.f);
                else if (lx >= 1320 && lx <= 1450) {
                    drag_ = 101;
                    SetCapture(hwnd_);
                    mouseMove(x, y);
                }
            }
            InvalidateRect(hwnd_, nullptr, FALSE);
            return;
        }
        if (!recording_ && ly >= 141 && ly < 191) {
            if (lx >= 42 && lx < 197)
                openDialog();
            else if (lx >= 212 && lx < 367)
                tracksMenu();
            else if (lx >= 382 && lx < 497 && player_.loaded())
                nextTrack(-1);
            else if (lx >= 512 && lx < 662) {
                if (player_.playing())
                    player_.pause();
                else
                    player_.play();
            } else if (lx >= 677 && lx < 792)
                player_.stop();
            else if (lx >= 807 && lx < 922 && player_.loaded())
                nextTrack(1);
            else if (lx >= 937 && lx < 1052)
                player_.repeat(!player_.repeat());
            else if (lx >= 1067 && lx < 1182)
                recording_ = true;
            else if (lx >= 1197 && lx < 1357)
                autoplay_ = !autoplay_;
            else if (lx >= 1372 && lx < 1519)
                rateMenu();
            return;
        }
        if (!recording_ && player_.loaded() && ly >= 1004 && ly <= 1029) {
            auto &t = player_.tracks()[player_.track()];
            if (t.lengthMs > 0)
                player_.seek(int(std::clamp((lx - 42) / 1840., 0., 1.) * t.lengthMs));
            return;
        }
        auto states = player_.states();
        auto visible = visibleChannels();
        for (size_t slot = 0; slot < visible.size(); ++slot) {
            auto cell = channelCell(slot, visible.size(), recording_);
            if (ly < cell.y || ly >= cell.y + cell.height - 8 || lx < cell.x ||
                lx >= cell.waveLeft() - 12)
                continue;
            size_t i = visible[slot];
            if (i >= states.size())
                return;
            if (!recording_ && ly >= cell.volumeY() && ly < cell.volumeY() + cell.buttonSize()) {
                if (lx >= cell.labelX() && lx < cell.labelX() + cell.buttonSize()) {
                    states[i].volume =
                        std::clamp((std::lround(states[i].volume * 100) - 5) / 100.f, 0.f, 2.f);
                    player_.channel(i, states[i]);
                } else if (lx >= cell.plusX() && lx < cell.plusX() + cell.buttonSize()) {
                    states[i].volume =
                        std::clamp((std::lround(states[i].volume * 100) + 5) / 100.f, 0.f, 2.f);
                    player_.channel(i, states[i]);
                } else if (lx >= cell.sliderLeft() && lx < cell.sliderRight()) {
                    drag_ = int(i);
                    SetCapture(hwnd_);
                    mouseMove(x, y);
                }
            } else if (ly - cell.y < (cell.compact ? 20 : cell.height < 100 ? 36 : 51)) {
                states[i].mute = !states[i].mute;
                player_.channel(i, states[i]);
            }
            InvalidateRect(hwnd_, nullptr, FALSE);
            return;
        }

    } catch (const std::exception &e) {
        report(e);
    }
}
Window::~Window() {
    framePacer_.stop();
    releaseBuffer();
}
void Window::rememberChannels() {
    if (!preferencesEnabled_ || !player_.loaded())
        return;
    const auto states = player_.states();
    const auto &channels = player_.channels();
    const std::string system = player_.tracks().empty() ? "" : player_.tracks()[0].system;
    for (size_t i = 0; i < channels.size() && i < states.size(); ++i) {
        auto key = desktop::Settings::channelKey(system, channels[i].name);
        preferences_.values[key + ".volume"] = int(std::lround(states[i].volume * 100));
        preferences_.values[key + ".mute"] = states[i].mute ? 1 : 0;
    }
}
void Window::restoreChannels() {
    if (!preferencesEnabled_ || !player_.loaded())
        return;
    const auto &channels = player_.channels();
    const std::string system = player_.tracks().empty() ? "" : player_.tracks()[0].system;
    for (size_t i = 0; i < channels.size(); ++i) {
        auto key = desktop::Settings::channelKey(system, channels[i].name);
        ChannelState state;
        state.volume = preferences_.get(key + ".volume", 100, 0, 200) / 100.f;
        state.mute = preferences_.get(key + ".mute", 0, 0, 1) != 0;
        player_.channel(i, state);
    }
}
void Window::savePreferences() {
    if (!preferencesEnabled_ || changing_)
        return;
    rememberChannels();
    auto &v = preferences_.values;
    v["controlsRevision"] = 1415;
    v["master"] = int(std::lround(player_.master() * 100));
    v["speed"] = int(std::lround(player_.speed() * 100));
    v["pitch"] = int(std::lround(player_.pitch() * 100));
    v["speedStep"] = speedStep_;
    v["rate"] = player_.rate();
    v["reverb"] = player_.reverb();
    v["reverbAmount"] = int(std::lround(player_.reverbAmount() * 100));
    v["reverbDecay"] = int(std::lround(player_.reverbDecay() * 10));
    v["repeat"] = player_.repeat();
    v["autoplay"] = autoplay_;
    v["recording"] = recording_;
    v["windowSamples"] = windowSamples_;
    if (!fullscreen_ && !IsIconic(hwnd_) && !IsZoomed(hwnd_)) {
        RECT rect{};
        GetClientRect(hwnd_, &rect);
        v["width"] = rect.right;
        v["height"] = rect.bottom;
    }
    if (!fullscreen_ && !IsIconic(hwnd_))
        v["maximized"] = IsZoomed(hwnd_) != FALSE;
    auto text = preferences_.text();
    if (text != savedPreferences_ && preferences_.save(desktop::Settings::path()))
        savedPreferences_ = std::move(text);
}
void Window::releaseBuffer() {
    if (bufferDC_) {
        SelectObject(bufferDC_, bufferOld_);
        DeleteObject(bufferBitmap_);
        DeleteDC(bufferDC_);
    }
    bufferDC_ = nullptr;
    bufferBitmap_ = nullptr;
    bufferPixels_ = nullptr;
    bufferOld_ = nullptr;
    bufferWidth_ = bufferHeight_ = 0;
}
void Window::renderFrame(HDC dc, int width, int height) {
    if (!bufferDC_ || bufferWidth_ != width || bufferHeight_ != height) {
        releaseBuffer();
        bufferDC_ = CreateCompatibleDC(dc);
        BITMAPINFO bitmap{};
        bitmap.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bitmap.bmiHeader.biWidth = width;
        bitmap.bmiHeader.biHeight = -height;
        bitmap.bmiHeader.biPlanes = 1;
        bitmap.bmiHeader.biBitCount = 32;
        bitmap.bmiHeader.biCompression = BI_RGB;
        void *pixels = nullptr;
        bufferBitmap_ = CreateDIBSection(dc, &bitmap, DIB_RGB_COLORS, &pixels, nullptr, 0);
        bufferPixels_ = static_cast<unsigned long *>(pixels);
        if (!bufferDC_ || !bufferBitmap_) {
            if (bufferBitmap_)
                DeleteObject(bufferBitmap_);
            if (bufferDC_)
                DeleteDC(bufferDC_);
            bufferDC_ = nullptr;
            bufferBitmap_ = nullptr;
            paint(dc, width, height);
            for (const auto &segment : segments_)
                line(dc, int(segment.from.x * scale_) + offsetX_,
                     int(segment.from.y * scale_) + offsetY_, int(segment.to.x * scale_) + offsetX_,
                     int(segment.to.y * scale_) + offsetY_, segment.color,
                     std::max(1, int(2 * scale_)));
            drawOverlayText(dc);
            return;
        }
        bufferOld_ = SelectObject(bufferDC_, bufferBitmap_);
        bufferWidth_ = width;
        bufferHeight_ = height;
    }
    paint(bufferDC_, width, height);
    // Complete background/text GDI work before touching the same DIB memory.
    GdiFlush();
    drawWaves();
    drawOverlayText(bufferDC_);
    BitBlt(dc, 0, 0, width, height, bufferDC_, 0, 0, SRCCOPY);
}
void Window::drawOverlayText(HDC dc){
    if(overlayText_.empty())return;
    int saved=SaveDC(dc);SetGraphicsMode(dc,GM_ADVANCED);
    XFORM xf{FLOAT(scale_),0,0,FLOAT(scale_),FLOAT(offsetX_),FLOAT(offsetY_)};SetWorldTransform(dc,&xf);
    for(const auto&v:overlayText_){
        for(auto offset:{POINT{-1,0},POINT{1,0},POINT{0,-1},POINT{0,1}})
            text(dc,v.value,v.x+offset.x,v.y+offset.y,v.w,v.h,v.size,RGB(6,9,14),v.bold);
        text(dc,v.value,v.x,v.y,v.w,v.h,v.size,v.color,v.bold);
    }
    RestoreDC(dc,saved);
}
void Window::drawWaves() {
    // Rasterize the actual min/max envelope directly: no per-segment kernel/GDI calls.
    const int thickness = std::max(1, int(std::round(2 * scale_)));
    for (const auto &segment : segments_) {
        int x = int(segment.from.x * scale_) + offsetX_;
        int y = int(segment.from.y * scale_) + offsetY_;
        const int endX = int(segment.to.x * scale_) + offsetX_;
        const int endY = int(segment.to.y * scale_) + offsetY_;
        const int dx = std::abs(endX - x), dy = -std::abs(endY - y);
        const int stepX = x < endX ? 1 : -1, stepY = y < endY ? 1 : -1;
        int error = dx + dy;
        const unsigned long color = (GetRValue(segment.color) << 16) |
                                    (GetGValue(segment.color) << 8) | GetBValue(segment.color);
        for (;;) {
            const int left = std::clamp(x - thickness / 2, 0, bufferWidth_);
            const int right = std::clamp(x - thickness / 2 + thickness, 0, bufferWidth_);
            const int top = std::clamp(y - thickness / 2, 0, bufferHeight_);
            const int bottom = std::clamp(y - thickness / 2 + thickness, 0, bufferHeight_);
            for (int row = top; row < bottom; ++row)
                std::fill(bufferPixels_ + size_t(row) * bufferWidth_ + left,
                          bufferPixels_ + size_t(row) * bufferWidth_ + right, color);
            if (x == endX && y == endY)
                break;
            const int twice = 2 * error;
            if (twice >= dy) {
                error += dy;
                x += stepX;
            }
            if (twice <= dx) {
                error += dx;
                y += stepY;
            }
        }
    }
}
void Window::capture() {
    RECT r;
    GetClientRect(hwnd_, &r);
    int w = r.right, h = r.bottom;
    auto dc = GetDC(hwnd_);
    auto mem = CreateCompatibleDC(dc);
    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = w;
    info.bmiHeader.biHeight = -h;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    void *bits = nullptr;
    auto bmp = CreateDIBSection(dc, &info, DIB_RGB_COLORS, &bits, nullptr, 0);
    auto old = SelectObject(mem, bmp);
    wchar_t benchmark[2];
    if (GetEnvironmentVariableW(L"NGMV_RENDER_BENCHMARK", benchmark, 2)) {
        for (int i = 0; i < 10; ++i)
            renderFrame(mem, w, h);
        GdiFlush();
        auto resourcesBefore = GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS);
        std::vector<double> durations;
        for (int i = 0; i < 120; ++i) {
            auto begin = std::chrono::steady_clock::now();
            renderFrame(mem, w, h);
            GdiFlush();
            durations.push_back(
                std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - begin)
                    .count());
        }
        auto resourcesAfter = GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS);
        double total = 0;
        for (double ms : durations)
            total += ms;
        std::sort(durations.begin(), durations.end());
        std::ofstream report(std::filesystem::path(screenshot_ + L".render.csv"));
        report << "width,height,frames,mean_ms,p95_ms,gdi_before,gdi_after\n"
               << w << ',' << h << ",120," << total / durations.size() << ',' << durations[113]
               << ',' << resourcesBefore << ',' << resourcesAfter << '\n';
    }
    renderFrame(mem, w, h);
    GdiFlush();
    BITMAPFILEHEADER file{};
    file.bfType = 0x4d42;
    file.bfOffBits = sizeof(file) + sizeof(BITMAPINFOHEADER);
    file.bfSize = file.bfOffBits + w * h * 4;
    std::ofstream f(std::filesystem::path(screenshot_), std::ios::binary);
    f.write(reinterpret_cast<char *>(&file), sizeof file);
    f.write(reinterpret_cast<char *>(&info.bmiHeader), sizeof(BITMAPINFOHEADER));
    f.write(static_cast<char *>(bits), w * h * 4);
    SelectObject(mem, old);
    DeleteObject(bmp);
    DeleteDC(mem);
    ReleaseDC(hwnd_, dc);
}
LRESULT CALLBACK Window::procedure(HWND w, UINT m, WPARAM a, LPARAM b) {
    auto *self = reinterpret_cast<Window *>(GetWindowLongPtrW(w, GWLP_USERDATA));
    if (m == WM_NCCREATE) {
        self = static_cast<Window *>(reinterpret_cast<CREATESTRUCTW *>(b)->lpCreateParams);
        self->hwnd_ = w;
        SetWindowLongPtrW(w, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    }
    try {
        return self ? self->event(m, a, b) : DefWindowProcW(w, m, a, b);
    } catch (const std::exception &e) {
        if (self)
            self->report(e);
        return 0;
    }
}
LRESULT Window::event(UINT m, WPARAM a, LPARAM b) {
    switch (m) {
    case WM_APP + 74:
        // Read-only scalar observations for our isolated offscreen regression.
        // No pointers, audio data or user settings are exposed by this hook.
        if (!screenshot_.empty() && desktop::automatedSession()) {
            switch (a) {
            case 0:
                return player_.loaded();
            case 1:
                return player_.playing();
            case 2:
                return LRESULT(player_.seconds() * 1000);
            case 3:
                return LRESULT(std::lround(player_.master() * 100));
            case 5:
                return LRESULT(std::lround(player_.speed() * 100));
            case 6:
                return LRESULT(std::lround(player_.pitch() * 100));
            case 7:
                return LRESULT(std::lround(player_.reverbAmount() * 100));
            case 8:
                return LRESULT(std::lround(player_.reverbDecay() * 10));
            case 9:
                return speedStep_;
            case 14: return fullscreen_;
            case 15: return recording_;
            case 16: return player_.repeat();
            case 12:
                return LRESULT(player_.underruns());
            case 13:
                return !player_.error().empty();
            case 10: {
                auto states = player_.states();
                return states.empty() ? 0 : LRESULT(std::lround(states[0].volume * 100));
            }
            case 11: {
                auto states = player_.states();
                return std::all_of(states.begin(), states.end(), [&](const auto &v) {
                    return states.empty() || v.volume == states[0].volume;
                });
            }
            case 4: {
                auto states = player_.states();
                return !states.empty() && states[0].mute;
            }
            }
        }
        return 0;
    case DisplayPacer::Message:
        framePacer_.acknowledge();
#ifndef NGMV_TEST_TIMER_ONLY_BASELINE
        updatePlayback();
#endif
        if (!IsIconic(hwnd_) && (player_.playing() || changing_))
            InvalidateRect(hwnd_, nullptr, FALSE);
        return 0;
    case WM_COPYDATA: {
        std::wstring path;
        if (pendingFiles_.size() >= 32 ||
            !desktop::decodeRequest(reinterpret_cast<COPYDATASTRUCT *>(b), path))
            return FALSE;
        pendingFiles_.push_back(std::move(path));
        PostMessageW(hwnd_, desktop::OpenQueuedFile, 0, 0);
        return TRUE;
    }
    case desktop::OpenQueuedFile:
        if (changing_) {
            SetTimer(hwnd_, 3, 100, nullptr);
            return 0;
        }
        if (!pendingFiles_.empty()) {
            auto path = std::move(pendingFiles_.front());
            pendingFiles_.pop_front();
            ShowWindow(hwnd_, IsIconic(hwnd_) ? SW_RESTORE : SW_SHOW);
            if (!SetForegroundWindow(hwnd_)) {
                FLASHWINFO flash{sizeof(FLASHWINFO), hwnd_, FLASHW_TRAY, 3, 0};
                FlashWindowEx(&flash);
            }
            try {
                if (!path.empty() && open(path))
                    player_.play();
            } catch (const std::exception &e) {
                report(e);
            }
            if (!pendingFiles_.empty())
                PostMessageW(hwnd_, desktop::OpenQueuedFile, 0, 0);
        }
        return 0;
    case desktop::AskDefaultApps:
        try {
            desktop::defaultAppsPrompt(hwnd_, false, recording_);
        } catch (const std::exception &e) {
            report(e);
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((a & 0xfff0) == desktop::DefaultAppsCommand) {
            try {
                desktop::defaultAppsPrompt(hwnd_, true, recording_);
            } catch (const std::exception &e) {
                report(e);
            }
            return 0;
        }
        break;
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(hwnd_, &ps);
        RECT r;
        GetClientRect(hwnd_, &r);
        if (r.right && r.bottom) {
            try {
                renderFrame(dc, r.right, r.bottom);
            } catch (...) {
                EndPaint(hwnd_, &ps);
                throw;
            }
        }
        EndPaint(hwnd_, &ps);
        return 0;
    }
    case WM_TIMER:
        if (a == 4) {
            savePreferences();
            return 0;
        }
        if (a == 3) {
            KillTimer(hwnd_, 3);
            PostMessageW(hwnd_, desktop::OpenQueuedFile, 0, 0);
            return 0;
        }
        try {
            updatePlayback();
            if (a == 2) {
                KillTimer(hwnd_, 2);
                capture();
                DestroyWindow(hwnd_);
                return 0;
            }
        } catch (const std::exception &e) {
            report(e);
        }
        if (a != 1)
            InvalidateRect(hwnd_, nullptr, FALSE);
        return 0;
    case WM_SIZE:
        InvalidateRect(hwnd_, nullptr, FALSE);
        return 0;
    case WM_LBUTTONDOWN:
        click(GET_X_LPARAM(b), GET_Y_LPARAM(b));
        return 0;
    case WM_MOUSEMOVE:
        mouseMove(GET_X_LPARAM(b), GET_Y_LPARAM(b));
        return 0;
    case WM_LBUTTONUP:
        drag_ = -1;
        ReleaseCapture();
        return 0;
    case WM_CAPTURECHANGED:
        drag_ = -1;
        return 0;
    case WM_DROPFILES: {
        auto drop = reinterpret_cast<HDROP>(a);
        wchar_t p[32768];
        DragQueryFileW(drop, 0, p, 32768);
        DragFinish(drop);
        open(p);
        return 0;
    }
    case WM_KEYDOWN:
        if (changing_)
            return 0;
        try {
            if (a == VK_F1)
                showAbout();
            else if (a == 'O' && (GetKeyState(VK_CONTROL) & 0x8000))
                openDialog();
            else if (a == VK_SPACE) {
                if (player_.playing())
                    player_.pause();
                else
                    player_.play();
            } else if (a == 'A')
                autoplay_ = !autoplay_;
            else if (a == VK_F9) {
                recording_ = !recording_;
                drag_ = -1;
                ReleaseCapture();
                ModifyMenuW(GetSystemMenu(hwnd_, FALSE), desktop::DefaultAppsCommand,
                            MF_BYCOMMAND | MF_STRING, desktop::DefaultAppsCommand,
                            recording_ ? L"Default apps... (F10)" : L"기본 앱 설정... (F10)");
            } else if (a == VK_F10)
                desktop::defaultAppsPrompt(hwnd_, true, recording_);
            else if (a == VK_F11)
                toggleFullscreen();
            else if (a == VK_ESCAPE && fullscreen_)
                toggleFullscreen();
            else if (a == VK_OEM_PLUS)
                windowSamples_ = std::max(256, windowSamples_ / 2);
            else if (a == VK_OEM_MINUS)
                windowSamples_ = std::min(4096, windowSamples_ * 2);
            else if (a == VK_LEFT && player_.loaded())
                nextTrack(-1);
            else if (a == VK_RIGHT && player_.loaded())
                nextTrack(1);
        } catch (const std::exception &e) {
            report(e);
        }
        InvalidateRect(hwnd_, nullptr, FALSE);
        return 0;
    case WM_GETMINMAXINFO: {
        auto p = reinterpret_cast<MINMAXINFO *>(b);
        p->ptMinTrackSize = {800, 490};
        p->ptMaxTrackSize = {8192, 8192};
        return 0;
    }
    case WM_CLOSE:
        DestroyWindow(hwnd_);
        return 0;
    case WM_DESTROY:
        if (aboutWindow_ && IsWindow(aboutWindow_))
            DestroyWindow(aboutWindow_);
        framePacer_.stop();
        savePreferences();
        RemovePropW(hwnd_, desktop::InstanceProperty);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd_, m, a, b);
}
int Window::run(HINSTANCE instance, const std::filesystem::path &initial,
                const std::filesystem::path &screenshot, int width, int height, bool recording,
                bool playInitial) {
    SetProcessDPIAware();
    player_.continueAfterLength(true);
    recording_ = recording;
    preferencesEnabled_ = (screenshot.empty() ||
                           GetEnvironmentVariableW(L"NGMV_SETTINGS_TEST_FILE", nullptr, 0) > 0) &&
                          (!desktop::automatedSession() ||
                           GetEnvironmentVariableW(L"NGMV_SETTINGS_TEST_FILE", nullptr, 0) > 0);
    if (preferencesEnabled_) {
        preferences_.load(desktop::Settings::path());
        player_.master(preferences_.get("master", 100, 0, 100) / 100.f);
        player_.speed(preferences_.get("speed", 100, 50, 200) / 100.f);
        player_.pitch(preferences_.get("pitch", 100, 50, 200) / 100.f);
        const bool currentDefaults = preferences_.get("controlsRevision", 0, 0, 9999) >= 1415;
        speedStep_ = currentDefaults && preferences_.get("speedStep", 1, 1, 10)==10 ? 10 : 1;
        player_.reverb(preferences_.get("reverb", 0, 0, 1) != 0);
        player_.reverbAmount((currentDefaults ? preferences_.get("reverbAmount", 76, 0, 100) : 76) /
                             100.f);
        player_.reverbDecay((currentDefaults ? preferences_.get("reverbDecay", 30, 1, 30) : 30) /
                            10.f);
        player_.repeat(preferences_.get("repeat", 0, 0, 1) != 0);
        autoplay_ = preferences_.get("autoplay", 1, 0, 1) != 0;
        recording_ = recording || preferences_.get("recording", 0, 0, 1) != 0;
        int samples = preferences_.get("windowSamples", 2048, 256, 4096);
        if ((samples & (samples - 1)) == 0)
            windowSamples_ = samples;
        width = preferences_.get("width", width, 800, 8192);
        height = preferences_.get("height", height, 490, 8192);
        // A previously available output rate may fail on a different device.
        try {
            player_.rate(preferences_.get("rate", 96000, 16000, 96000));
        } catch (const std::exception &) { /* Keep the output's existing default. */
        }
    }
    WNDCLASSW wc{};
    wc.hInstance = instance;
    wc.lpszClassName = desktop::WindowClass;
    wc.lpfnWndProc = procedure;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hIcon = static_cast<HICON>(LoadImageW(instance, MAKEINTRESOURCEW(101), IMAGE_ICON,
                                             GetSystemMetrics(SM_CXICON),
                                             GetSystemMetrics(SM_CYICON), LR_SHARED));
    RegisterClassW(&wc);
    RECT r{0, 0, width, height};
    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, FALSE);
    hwnd_ = CreateWindowW(wc.lpszClassName, L"Nintendo Music Studio · 1.5.7", WS_OVERLAPPEDWINDOW,
                          screenshot.empty() ? CW_USEDEFAULT : -20000,
                          screenshot.empty() ? CW_USEDEFAULT : -20000, r.right - r.left,
                          r.bottom - r.top, nullptr, nullptr, instance, this);
    if (!hwnd_)
        throw std::runtime_error("Cannot create window.");
    SendMessageW(hwnd_, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(wc.hIcon));
    auto smallIcon =
        LoadImageW(instance, MAKEINTRESOURCEW(101), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON),
                   GetSystemMetrics(SM_CYSMICON), LR_SHARED);
    SendMessageW(hwnd_, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(smallIcon));
    if (screenshot.empty()) {
        if (!SetPropW(hwnd_, desktop::InstanceProperty,
                      reinterpret_cast<HANDLE>(desktop::instanceToken())))
            throw std::runtime_error("Cannot initialize file handoff.");
        auto menu = GetSystemMenu(hwnd_, FALSE);
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(menu, MF_STRING, desktop::DefaultAppsCommand,
                    recording_ ? L"Default apps... (F10)" : L"기본 앱 설정... (F10)");
    }
    DragAcceptFiles(hwnd_, TRUE);
    ShowWindow(hwnd_, preferencesEnabled_ && preferences_.get("maximized", 0, 0, 1)
                          ? SW_SHOWMAXIMIZED
                          : SW_SHOW);
    if (!screenshot.empty()) {
        SetWindowPos(hwnd_, nullptr, 0, 0, r.right - r.left, r.bottom - r.top,
                     SWP_NOZORDER | SWP_NOMOVE);
    }
    SetTimer(hwnd_, 1, 16, nullptr);
    framePacer_.start(hwnd_);
    if (preferencesEnabled_)
        SetTimer(hwnd_, 4, 1000, nullptr);
    if (!initial.empty() && open(initial) && playInitial)
        player_.play();
    if (screenshot.empty())
        PostMessageW(hwnd_, desktop::AskDefaultApps, 0, 0);
    if (!screenshot.empty()) {
        if (player_.loaded())
            player_.play();
        screenshot_ = screenshot.wstring();
        unsigned delay = 1200;
        if (desktop::automatedSession()) {
            wchar_t value[16]{};
            DWORD size = GetEnvironmentVariableW(L"NGMV_TEST_SNAPSHOT_MS", value, 16);
            if (size && size < 16)
                delay = unsigned(std::clamp(_wtoi(value), 1200, 20000));
        }
        SetTimer(hwnd_, 2, delay, nullptr);
    }
    const bool dropPlaybackTimer=!screenshot_.empty() && desktop::automatedSession() &&
        GetEnvironmentVariableW(L"NGMV_TEST_DISABLE_PLAYBACK_TIMER",nullptr,0)>0;
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        // Fault-injection only in explicitly isolated snapshot tests: verify
        // transport still advances with no idle timer delivery at all.
        if(dropPlaybackTimer && msg.hwnd==hwnd_ && msg.message==WM_TIMER && msg.wParam==1)continue;
        if (aboutWindow_ && (msg.hwnd == aboutWindow_ || IsChild(aboutWindow_, msg.hwnd))) {
            if (msg.message == WM_KEYDOWN && msg.wParam == VK_F1) {
                showAbout();
                continue;
            }
            if (IsDialogMessageW(aboutWindow_, &msg))
                continue;
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return int(msg.wParam);
}
} // namespace ngmv
