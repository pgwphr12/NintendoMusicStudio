#pragma once
#include "app/Settings.hpp"
#include "player/FolderPlaylist.hpp"
#include "player/Player.hpp"
#include "DisplayPacer.hpp"
#include "ChannelLayout.hpp"
#include "visualizer/SpectrumCurve.hpp"
#include "visualizer/Envelope.hpp"
#include "visualizer/SpectrogramHistory.hpp"
#include "visualizer/WaveformTail.hpp"
#include <filesystem>
#include <deque>
#include <chrono>
#include <windows.h>
namespace ngmv {
class Window {
    HWND hwnd_ = nullptr;
    HWND aboutWindow_ = nullptr;
    HWND spectrumChoiceWindow_ = nullptr;
    HWND hiddenChannelsWindow_ = nullptr;
    std::vector<std::string> hiddenChannelKeys_;
    std::vector<bool> hiddenChannels_;
    void refreshHiddenChannels();
    void updateHiddenChannels();
    void showHiddenChannels();
    void setChannelHidden(size_t,bool);
    void hideChannelAt(int,int);
    size_t hiddenChannelCount() const;
    HWND remoteWindow_ = nullptr, equalizerWindow_ = nullptr;
    void showRemote();
    void toggleRemote();
    void updateRemote();
    void showEqualizer();
    bool equalizerSync_=false;
    void updateEqualizer(bool force=false);
    void setRecording(bool);
    HDC bufferDC_ = nullptr;
    HBITMAP bufferBitmap_ = nullptr;
    HGDIOBJ bufferOld_ = nullptr;
    int bufferWidth_ = 0, bufferHeight_ = 0;
    struct WaveSegment {
        POINT from, to;
        COLORREF color;
    };
    std::vector<WaveSegment> segments_;
    std::vector<WaveformTracker> waveformTrackers_;
    std::vector<WaveformTransition> waveformTransitions_;
    std::chrono::steady_clock::time_point waveformPaintTime_{};
    WaveformTail waveformTail_;
    unsigned long *bufferPixels_ = nullptr;
    struct OverlayText {std::wstring value;int x,y,w,h,size;COLORREF color;bool bold;};
    std::vector<OverlayText> overlayText_;
    struct LabelPixel {size_t index;unsigned long ink,remain;};
    std::vector<LabelPixel> labelPixels_;
    size_t labelCacheKey_=0;
    int recordingLabelOverflow_=0;
    double lastPaintMs_=0,lastWavesMs_=0,lastLabelsMs_=0;
    void drawOverlayText(HDC);
    void drawWaves();
    void releaseBuffer();
    DisplayPacer framePacer_;
    Player player_;
    std::filesystem::path file_;
    FolderPlaylist playlist_;
    bool changing_ = false;
    std::deque<std::wstring> pendingFiles_;
    void selectFolder(size_t index, bool play);
    void nextTrack(int direction, bool automatic = false);
    bool recording_ = false, fullscreen_ = false;
    bool spectrumMode_=false;
    int spectrumStyle_=3;
    SpectrogramHistory spectrogram_;
    uint64_t spectrogramTimeMs_=0;
    std::vector<uint32_t> spectrogramRaster_;
    SpectrumAnalyzer spectrum_;
    SpectrumMotion spectrumMotion_;
    SpectrumAnalyzer::Samples spectrumSamples_{};
    ULONGLONG spectrumLastAnalysis_=0,spectrumLastPaint_=0,spectrumLastAudio_=0;
    uint64_t spectrumLastSample_=0;
    uint64_t spectrumEpoch_=0;
    float spectrumPeak_=0;
    void paintSpectrum(HDC);
    void updateSpectrum();
    void paintScrollingSpectrum(HDC,int,int);
    void showSpectrumChoices();
    void setSpectrumStyle(int);
    void loadVisualPreferences();
    void toggleSpectrum();
    bool autoplay_ = true;
    int windowSamples_ = 1024;
    int speedStep_ = 1;
    desktop::Settings preferences_;
    bool preferencesEnabled_ = false;
    std::string savedPreferences_;
    bool allVolumeDirty_ = false;
    void setAllChannelVolume(float volume);
    void rememberChannels();
    void restoreChannels();
    void savePreferences();
    RECT oldRect_{};
    std::wstring message_ = L"";
    double scale_ = 1;
    int offsetX_ = 0, offsetY_ = 0;
    int drag_ = -1;
    std::wstring screenshot_;
    static LRESULT CALLBACK procedure(HWND, UINT, WPARAM, LPARAM);
    LRESULT event(UINT, WPARAM, LPARAM);
    std::vector<size_t> visibleChannels() const;
    void paint(HDC, int, int);
    void renderFrame(HDC, int, int);
    void click(int, int);
    void mouseMove(int, int);
    void openDialog();
    void showAbout();
    bool open(const std::filesystem::path &);
    void tracksMenu();
    void rateMenu();
    void soundFontMenu();
    void updatePlayback();
    bool followVirtualMidi_=true,virtualMidiSeen_=false,virtualMidiAvailable_=false;
    ULONGLONG virtualMidiNextCheck_=0;
    unsigned virtualMidiReloads_=0;
    std::wstring virtualMidiLoaded_,virtualMidiPending_,virtualMidiFailed_,virtualMidiError_;
    void checkVirtualMidi();
    void saveMidiFontSelection(const std::filesystem::path&);

    void toggleFullscreen();
    void report(const std::exception &e);
    void capture();

  public:
    ~Window();
    int run(HINSTANCE, const std::filesystem::path &initial = {},
            const std::filesystem::path &screenshot = {}, int width = 1280, int height = 720,
            bool recording = false, bool playInitial = false);
};
std::wstring wide(const std::string &s);
} // namespace ngmv
