#include "Player.hpp"
#include <chrono>
#include <cmath>
#include <stdexcept>
namespace ngmv {
Player::~Player() {
    try{pause();}catch(...){}
    output_.pause(true);
    haltWorker();
}
const std::vector<TrackInfo> &Player::tracks() const {
    static std::vector<TrackInfo> empty;
    return backend_ ? backend_->tracks() : empty;
}
const std::vector<ChannelInfo> &Player::channels() const {
    static std::vector<ChannelInfo> empty;
    return backend_ ? backend_->channels() : empty;
}
void Player::haltWorker() {
    quit_ = true;
    if (worker_.joinable())
        worker_.join();
}
void Player::startWorker() {
    quit_ = false;
    worker_ = std::thread(&Player::produce, this);
}
void Player::prefill() {
    const size_t target = std::min(
        size_t(14336), std::max(size_t(4096), size_t(std::ceil(1024. * InternalRate /
                                                              output_.rate() * output_.speed())) +
                                                 (output_.speed()!=1 || output_.pitch()!=1 ? 4096 : 1024)));
    for (int i = 0; i < 500 && audio_.available() < target && !atEnd_ && error().empty(); i++)
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    if (!error().empty())
        throw std::runtime_error(error());
}
void Player::open(const std::filesystem::path &path) {
    auto bytes = readMusicFile(path);
    auto format = identify(bytes,path);
    auto candidate = makeBackend(bytes, path);
    openPrepared(std::move(candidate),format);
}
void Player::openPrepared(std::unique_ptr<IMusicBackend> candidate,Format format,uint64_t startFrame,const std::vector<ChannelState>& initialStates) {
    if(!candidate)throw std::runtime_error("No prepared music");
    output_.pause(true);
    haltWorker();
    if(backend_)backend_->devicePlayback(0,false,states(),master());
    backend_ = std::move(candidate);
    format_=format;
    exactSourceEnd_=format==Format::AudioFile || format==Format::Midi;
    sourceEndFrame_=UINT64_MAX;
    bool fds = false;
    for (const auto &channel : backend_->channels())
        fds = fds || channel.name == "FDS WAVE";
    dmcUsed_ = false;
    usedChannels_ = 0;
    gbaPsgUsed_ = -1;
    level_.configure(format, fds);
    reverb_.reset();
    {
        std::lock_guard<std::mutex> lock(stateMutex_);
        channelStates_.assign(backend_->channels().size(), ChannelState{});
        if(initialStates.size()==channelStates_.size())channelStates_=initialStates;
        if(commonChannelVolume_>=0)for(auto& state:channelStates_)state.volume=commonChannelVolume_;
        error_.clear();
    }
    track_ = 0;
    configureVisibleChannels();
    base_ = startFrame;
    playing_ = false;
    atEnd_ = false;
    audio_.reset();
    output_.reset();
    history_.reset(channelStates_.size(),startFrame);
    startWorker();
    prefill();
}
void Player::restart(int track, int ms, bool play) {
    if (!backend_)
        return;
    pause();
    haltWorker();
    try {
        level_.reset();
        reverb_.reset();
        if (track != track_){
            dmcUsed_ = false;
            usedChannels_=0;
        }
        sourceEndFrame_=UINT64_MAX;
        const auto previousChannels = backend_->channels();
        const auto previousStates = states();
        backend_->selectTrack(track);
        {
            std::lock_guard<std::mutex> lock(stateMutex_);
            channelStates_.assign(backend_->channels().size(), ChannelState{});
            for(size_t i=0;i<channelStates_.size();i++) {
                if(commonChannelVolume_>=0)channelStates_[i].volume=commonChannelVolume_;
                const auto& name=backend_->channels()[i].name;
                if(i<previousChannels.size()&&i<previousStates.size()&&previousChannels[i].name==name)
                    channelStates_[i]=previousStates[i];
                else for(size_t j=0;j<previousChannels.size()&&j<previousStates.size();j++)
                    if(previousChannels[j].name==name){channelStates_[i]=previousStates[j];break;}
            }
        }
        { std::lock_guard<std::mutex> lock(stateMutex_);
          // Current-song stop/seek/replay preserves its mixed per-channel edits.
          // A different song still starts from the saved common gain.
          if(track!=track_&&commonChannelVolume_>=0)for(auto& state:channelStates_)state.volume=commonChannelVolume_; }
        configureVisibleChannels();
        gbaPsgUsed_ = -1;
        if (ms)
            backend_->seek(ms);
        track_ = track;
        base_ = uint64_t(ms) * InternalRate / 1000;
        audio_.reset();
        output_.reset();
        history_.reset(backend_->channels().size(), base_);
        atEnd_ = false;
        {
            std::lock_guard<std::mutex> lock(stateMutex_);
            error_.clear();
        }
        startWorker();
        prefill();
        if (play)
            this->play();
    } catch (...) {
        playing_ = false;
        throw;
    }
}
void Player::configureVisibleChannels(){
    uint64_t used=UINT64_MAX;
    if(!backend_||!backend_->channelUsage(used)){visibleMask_=UINT64_MAX;return;}
    // Base console channels remain fixed, even if silent throughout the song.
    if(format_==Format::Nsf||format_==Format::Nsfe||format_==Format::NesRom)used|=31;
    else if(format_==Format::Gbs)used|=15;
    else if(format_==Format::Spc)used|=255;
    else if(format_==Format::Gsf||format_==Format::GbaRom)used|=UINT64_MAX&~uint64_t(15); // PCM slots stay visible.
    visibleMask_=used;
}
void Player::select(int track) {
    if (!backend_ || track < 0 || track >= int(tracks().size()))
        return;
    restart(track, 0, playing_);
}
void Player::play() {
    if (!backend_)
        return;
    if (ended()) {
        restart(track_, 0, false);
    }
    prefill();
    output_.pause(false);
    playing_ = true;
    serviceDevice();
}
void Player::serviceDevice(){if(!backend_||!backend_->hasDeviceOutput())return;std::lock_guard<std::mutex> deviceLock(deviceMutex_);try{if(backend_)backend_->devicePlayback(base_+output_.consumed(),playing_,states(),master());}catch(const std::exception&e){output_.pause(true);playing_=false;std::lock_guard<std::mutex> lock(stateMutex_);error_=e.what();}}
void Player::pause() {
    output_.pause(true);
    playing_ = false;
    serviceDevice();
}
void Player::stop() {
    restart(track_, 0, false);
}
void Player::seek(int ms) {
    if (!backend_)
        return;
    int length = tracks()[track_].lengthMs;
    if (length > 0 && !continueAfterLength_)
        ms = std::min(ms, length - 1);
    restart(track_, std::max(0, ms), playing_);
}
void Player::channel(size_t i, ChannelState state) {
    std::lock_guard<std::mutex> lock(stateMutex_);
    if (i < channelStates_.size()) {
        state.volume = std::clamp(state.volume, 0.f, 2.f);
        channelStates_[i] = state;
    }
}
std::vector<ChannelState> Player::states() const {
    std::lock_guard<std::mutex> lock(stateMutex_);
    return channelStates_;
}
void Player::channelVolumes(float volume) {
    if(!std::isfinite(volume))return;
    std::lock_guard<std::mutex> lock(stateMutex_);
    commonChannelVolume_=std::clamp(volume,0.f,2.f);
    for(auto&state:channelStates_)state.volume=commonChannelVolume_;
}
std::string Player::error() const {
    std::lock_guard<std::mutex> lock(stateMutex_);
    return error_;
}
bool Player::ended() const {
    return atEnd_ && output_.drained();
}
bool Player::playbackBoundaryReached() const {
    if(!backend_)return false;
    if(exactSourceEnd_){auto end=sourceEndFrame_.load();return end!=UINT64_MAX && base_+output_.consumed()>=end;}
    auto length=tracks()[track_].lengthMs;
    return length>0 && seconds()*1000>=length;
}
void Player::produce() {
    try {
        AudioBlock block;
        uint64_t produced = base_;
        int length = tracks()[track_].lengthMs;
        backend_->continuousEnd(continueAfterLength_.load());
        while (!quit_) {
            serviceDevice();
            const size_t target=output_.speed()!=1 || output_.pitch()!=1 ? 14336 : 8192;
            if (audio_.available() > target - 512) {
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
                continue;
            }
            if (!continueAfterLength_ && (exactSourceEnd_ ? sourceEndFrame_.load()!=UINT64_MAX && produced>=sourceEndFrame_.load() : length > 0 && produced >= uint64_t(length) * InternalRate / 1000)) {
                atEnd_ = true;
                output_.finished();
                break;
            }
            size_t frames = 512;
            if (!continueAfterLength_ && !exactSourceEnd_ && length > 0)
                frames = size_t(
                    std::min(uint64_t(frames), uint64_t(length) * InternalRate / 1000 - produced));
            auto state = states();
            uint64_t mask = 0;
            bool unity = true;
            for (size_t i = 0; i < state.size(); i++) {
                if (state[i].mute && i < 64)
                    mask |= uint64_t(1) << i;
                if (state[i].mute && i >= 64) unity = false;
                if (state[i].volume != 1)
                    unity = false;
            }
            backend_->muteMask64(mask);
            backend_->render(frames, block);
            // Reject inconsistent decoder layouts before history/mixing can index
            // stale channel buffers. Never truncate voices to conceal the error.
            if(block.mix.size()!=frames || block.voices.size()!=state.size())
                throw std::runtime_error("Decoder audio/channel layout mismatch.");
            for(const auto& voice:block.voices)
                if(voice.size()!=frames)
                    throw std::runtime_error("Decoder channel frame count mismatch.");
            uint64_t decodedEnd=0;if(exactSourceEnd_ && backend_->decodedEndFrame(decodedEnd))sourceEndFrame_=decodedEnd;
            uint32_t psg=0;if(backend_->gbaPsgUsed(psg))gbaPsgUsed_=int(psg&15);
            history_.append(block);
            uint64_t active = usedChannels_.load();
            for (size_t c = 0; c < block.voices.size() && c < 64; ++c)
                if (!(active & (uint64_t(1) << c)))
                    for (auto sample : block.voices[c])
                        if (std::max(std::abs(sample.l), std::abs(sample.r)) > .0001f) {
                            active |= uint64_t(1) << c;
                            break;
                        }
            usedChannels_ = active;
            if (!dmcUsed_)
                for (size_t c = 0; c < backend_->channels().size(); ++c)
                    if (backend_->channels()[c].name == "DMC")
                        for (auto sample : block.voices[c])
                            if (std::max(std::abs(sample.l), std::abs(sample.r)) > .0001f) {
                                dmcUsed_ = true;
                                break;
                            }
            if (!unity)
                for (size_t f = 0; f < frames; f++) {
                    Stereo sample;
                    for (size_t c = 0; c < state.size(); c++)
                        if (!state[c].mute) {
                            sample.l += block.voices[c][f].l * state[c].volume;
                            sample.r += block.voices[c][f].r * state[c].volume;
                        }
                    block.mix[f] = sample;
                }
            reverb_.enabled(reverbEnabled_.load());
            reverb_.amount(reverbAmount_.load());
            reverb_.decay(reverbDecay_.load());
            std::array<float,20> eqGains{};for(size_t band=0;band<20;band++)eqGains[band]=eqDb10_[band].load()/10.f;
            equalizer_.beginBlock(eqEnabled_.load(),eqGains);
            for (auto &sample : block.mix)
                sample = level_.process(reverb_.process(equalizer_.process(sample)));
            if (!audio_.push(block.mix.data(), frames))
                throw std::runtime_error("Audio ring producer overflow.");
            produced += frames;
        }
    } catch (const std::exception &e) {
        std::lock_guard<std::mutex> lock(stateMutex_);
        error_ = e.what();
        atEnd_ = true;
        output_.finished();
    }
}
} // namespace ngmv
