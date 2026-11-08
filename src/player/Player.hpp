#pragma once
#include "audio/OutputLevel.hpp"
#include "audio/Reverb.hpp"
#include "audio/GraphicEqualizer.hpp"
#include "audio/SdlOutput.hpp"
#include "channels/ChannelHistory.hpp"
#include "core/IMusicBackend.hpp"
#include <mutex>
#include <thread>
namespace ngmv {
class Player {
    StereoRing<> audio_;
    SdlOutput output_;
    OutputLevel level_;
    Reverb reverb_;
    GraphicEqualizer equalizer_;
    std::atomic<bool> eqEnabled_{false};
    std::array<std::atomic<int>,20> eqDb10_{};
    std::atomic<bool> reverbEnabled_{false};
    std::atomic<float> reverbAmount_{.76f};
    std::atomic<float> reverbDecay_{3.f};
    std::atomic<bool> dmcUsed_{false};
    std::atomic<uint64_t> usedChannels_{0};
    std::atomic<int> gbaPsgUsed_{-1};
    ChannelHistory history_;
    std::unique_ptr<IMusicBackend> backend_;
    std::thread worker_;
    std::atomic<bool> quit_{false};
    mutable std::mutex stateMutex_;
    std::vector<ChannelState> channelStates_;
    float commonChannelVolume_ = -1; // Unset preserves legacy per-channel defaults.
    std::string error_;
    std::atomic<bool> playing_{false};
    std::mutex deviceMutex_;
    int track_ = 0;
    uint64_t base_ = 0;
    std::atomic<bool> atEnd_{false};
    std::atomic<bool> repeat_{false};
    std::atomic<bool> continueAfterLength_{false};
    bool exactSourceEnd_=false;
    Format format_=Format::Nsf;
    uint64_t visibleMask_=UINT64_MAX;
    void configureVisibleChannels();
    std::atomic<uint64_t> sourceEndFrame_{UINT64_MAX};
    void haltWorker();
    void startWorker();
    void prefill();
    void produce();
    void restart(int track, int ms, bool play);

  public:
    Player() : output_(audio_) {}
    ~Player();
    void open(const std::filesystem::path &path);
    void openPrepared(std::unique_ptr<IMusicBackend> backend,Format format,uint64_t startFrame=0,const std::vector<ChannelState>& initialStates={});
    void select(int track);
    void play();
    void serviceDevice();
    void pause();
    void releaseDevice(){pause();output_.releaseDevice();}
    void stop();
    void seek(int milliseconds);
    void equalizerEnabled(bool v){eqEnabled_=v;}
    bool equalizerEnabled()const{return eqEnabled_.load();}
    int equalizerGain(size_t band)const{return band<20?eqDb10_[band].load():0;}
    void equalizerGain(size_t band,int db10){if(band<20)eqDb10_[band]=int(std::lround(std::clamp(db10,-200,200)/10.))*10;}
    float outputGain() const {
        return level_.gain();
    }
    bool loaded() const {
        return bool(backend_);
    }
    bool playing() const {
        return playing_;
    }
    int track() const {
        return track_;
    }
    int rate() const {
        return output_.rate();
    }
    const std::vector<TrackInfo> &tracks() const;
    const std::vector<ChannelInfo> &channels() const;
    double seconds() const {
        return double(base_ + output_.consumed()) / InternalRate;
    }
    bool ended() const;
    bool playbackBoundaryReached() const;
    void continueAfterLength(bool value) { continueAfterLength_ = value; }
    void repeat(bool b) {
        repeat_ = b;
    }
    bool repeat() const {
        return repeat_;
    }
    void rate(int rate) {
        output_.changeRate(rate);
    }
    void master(float v) {
        output_.master(v);
    }
    bool dmcUsed() const {
        return dmcUsed_.load();
    }
    bool channelVisible(size_t index)const{
        return index<channels().size() && index<64 && (visibleMask_&(uint64_t(1)<<index));
    }
    bool channelUsed(size_t index) const {
        return index < 64 && (usedChannels_.load() & (uint64_t(1) << index));
    }
    bool reverb() const {
        return reverbEnabled_.load();
    }
    void reverb(bool value) {
        reverbEnabled_ = value;
    }
    float reverbAmount() const {
        return reverbAmount_.load();
    }
    void reverbAmount(float value) {
        reverbAmount_ = std::clamp(value, 0.f, 1.f);
    }
    float reverbDecay() const {
        return reverbDecay_.load();
    }
    void reverbDecay(float value) {
        reverbDecay_ = std::clamp(value, .1f, 3.f);
    }
    float speed() const {
        return output_.speed();
    }
    void speed(float value) {
        output_.speed(value);
    }
    float master() const {
        return output_.master();
    }
    void channel(size_t index, ChannelState state);
    void channelVolumes(float volume);
    float commonChannelVolume() const { std::lock_guard<std::mutex> lock(stateMutex_); return commonChannelVolume_; }
    float pitch() const { return output_.pitch(); }
    void pitch(float value) { output_.pitch(value); }
    std::vector<ChannelState> states() const;
    std::vector<std::vector<float>> waveforms() const {
        return history_.interpolatedSnapshot(base_ + output_.visualConsumed());
    }
    uint64_t spectrumSamples(SpectrumAnalyzer::Samples& out)const{return output_.spectrumSamples(out);}
    uint64_t spectrumEpoch()const{return output_.spectrumEpoch();}
    std::string error() const;
    uint64_t underruns() const {
        return output_.underruns();
    }
};
} // namespace ngmv
