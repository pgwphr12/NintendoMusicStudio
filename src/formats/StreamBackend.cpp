#include "StreamBackend.hpp"
extern "C" {
#include <libvgmstream.h>
}
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <deque>
#include <fstream>
#include <stdexcept>
#include <utility>
namespace ngmv {
namespace {
class StreamBackend final : public IMusicBackend {
    std::unique_ptr<libvgmstream_t, decltype(&libvgmstream_free)> core_{nullptr, libvgmstream_free};
    std::vector<ChannelInfo> channels_;
    std::vector<TrackInfo> tracks_;
    std::deque<std::array<float, 30>> pending_;
    std::vector<float> scratch_;
    std::vector<Stereo> routing_;
    std::vector<std::vector<size_t>> voiceSources_;
    std::vector<size_t> sourceVoice_;
    size_t sourceChannelCount_ = 0;
    bool groupStereoTracks_ = false;
    bool splitTwoChannelStereo_ = false;
    bool splitStereoSides_ = false;
    bool explicitTrackGroups_ = false;
    double position_ = 0;
    int rate_ = 0;
    uint32_t mask_ = 0;
    uint64_t endFrames_=UINT64_MAX;
    std::string system_;
    void routing(const std::filesystem::path &path, size_t channels) {
        const float groups = float((channels + 1) / 2);
        routing_.resize(channels);
        for (size_t c = 0; c < channels; ++c)
            routing_[c] = channels == 1 || (channels % 2 && c + 1 == channels)
                              ? Stereo{1 / groups, 1 / groups}
                          : c % 2 ? Stereo{0, 1 / groups}
                                  : Stereo{1 / groups, 0};
        // BCSTM track tables identify stereo pairs and mono stems. Do not assume
        // alternating L/R for a mono drum stem following several stereo pairs.
        std::ifstream file(path, std::ios::binary);
        std::array<uint8_t, 64> header{};
        if (!file.read(reinterpret_cast<char *>(header.data()), header.size()) ||
            std::memcmp(header.data(), "CSTM", 4))
            return;
        auto u32 = [](const uint8_t *p) {
            return uint32_t(p[0]) | uint32_t(p[1]) << 8 | uint32_t(p[2]) << 16 |
                   uint32_t(p[3]) << 24;
        };
        unsigned sections = header[16] | unsigned(header[17]) << 8;
        uint32_t offset = 0, size = 0;
        for (unsigned i = 0; i < std::min(sections, 3u); ++i)
            if (header[20 + i * 12] == 0 && header[21 + i * 12] == 0x40) {
                offset = u32(header.data() + 24 + i * 12);
                size = u32(header.data() + 28 + i * 12);
            }
        if (size < 32 || size > 65536)
            return;
        std::vector<uint8_t> info(size);
        file.seekg(offset);
        if (!file.read(reinterpret_cast<char *>(info.data()), size))
            return;
        auto ref = [&](size_t base, size_t field) -> size_t {
            if (field + 4 > size)
                return size;
            return base + uint64_t(u32(info.data() + field));
        };
        size_t table = ref(8, 20);
        if (table + 4 > size)
            return;
        unsigned count = u32(info.data() + table);
        if (!count || count > 30 || table + 4 + count * 8 > size)
            return;
        std::vector<Stereo> candidate(channels);
        std::vector<bool> assigned(channels);
        std::vector<std::vector<size_t>> trackGroups;
        for (unsigned t = 0; t < count; ++t) {
            size_t track = ref(table, table + 8 + t * 8);
            if (track + 12 > size)
                return;
            size_t indices = ref(track, track + 8);
            if (indices + 4 > size)
                return;
            unsigned n = u32(info.data() + indices);
            if (n < 1 || n > 2 || indices + 4 + n > size)
                return;
            float gain = info[track] / (127.f * count);
            float left = std::min(1.f, (127 - info[track + 1]) / 63.f);
            float right = std::min(1.f, info[track + 1] / 64.f);
            std::vector<size_t> trackChannels;
            for (unsigned i = 0; i < n; ++i) {
                size_t c = info[indices + 4 + i];
                if (c >= channels || assigned[c])
                    return;
                assigned[c] = true;
                trackChannels.push_back(c);
                candidate[c] = n == 1 ? Stereo{gain * left, gain * right}
                               : i    ? Stereo{0, gain * right}
                                      : Stereo{gain * left, 0};
            }
            trackGroups.push_back(std::move(trackChannels));
        }
        if (std::all_of(assigned.begin(), assigned.end(), [](bool a) { return a; })) {
            routing_ = std::move(candidate);
            if (groupStereoTracks_) {
                voiceSources_ = std::move(trackGroups);
                explicitTrackGroups_ = true;
            }
        }
    }
    void fill() {
        scratch_.resize(1024 * sourceChannelCount_);
        if (libvgmstream_fill(core_.get(), scratch_.data(), 1024) < 0)
            throw std::runtime_error("Cannot decode 3DS audio stream.");
        for (size_t i = 0; i < 1024; i++) {
            std::array<float, 30> frame{};
            std::copy_n(scratch_.data() + i * sourceChannelCount_, sourceChannelCount_, frame.data());
            pending_.push_back(frame);
        }
    }

  public:
    explicit StreamBackend(const std::filesystem::path &path, std::string system, bool groupStereoTracks,
                           bool splitTwoChannelStereo)
        : groupStereoTracks_(groupStereoTracks), splitTwoChannelStereo_(splitTwoChannelStereo),
          system_(std::move(system)) {
        using File = std::unique_ptr<libstreamfile_t, decltype(&libstreamfile_close)>;
        File file(libstreamfile_open_from_stdio(path.u8string().c_str()), libstreamfile_close);
        if (!file)
            throw std::runtime_error("Cannot open game audio stream.");
        libvgmstream_config_t config{};
        config.force_sfmt = LIBVGMSTREAM_SFMT_FLOAT;
        config.ignore_loop = true;
        config.ignore_fade = true;
        config.disable_config_override = true;
        core_.reset(libvgmstream_create(file.get(), 0, &config));
        if (!core_)
            throw std::runtime_error("Unsupported or damaged game stream codec.");
        const auto &info = *core_->format;
        if (info.channels < 1 || info.channels > 30 || info.sample_rate < 8000 ||
            info.sample_rate > 192000 || info.play_samples <= 0)
            throw std::runtime_error("Invalid game stream channels, rate or duration.");
        sourceChannelCount_ = size_t(info.channels);
        rate_ = info.sample_rate;
        if(uint64_t(info.play_samples)<=(UINT64_MAX-uint64_t(rate_)+1)/InternalRate)endFrames_=(uint64_t(info.play_samples)*InternalRate+rate_-1)/rate_;
        routing(path, info.channels);
        if (voiceSources_.empty()) {
            if (groupStereoTracks_) {
                for (size_t c = 0; c < sourceChannelCount_; c += 2) {
                    std::vector<size_t> group{c};
                    if (c + 1 < sourceChannelCount_)
                        group.push_back(c + 1);
                    voiceSources_.push_back(std::move(group));
                }
            } else {
                for (size_t c = 0; c < sourceChannelCount_; ++c)
                    voiceSources_.push_back({c});
            }
        }
        if (splitTwoChannelStereo_ && sourceChannelCount_ == 2 && voiceSources_.size() == 1 &&
            voiceSources_[0].size() == 2) {
            // Keep a simple 3DS stereo file inspectable as left/right when it
            // has no separately-addressable tracks. This is a channel view,
            // not a claim that the sides are independent musical stems.
            voiceSources_ = {{0}, {1}};
            explicitTrackGroups_ = false;
            splitStereoSides_ = true;
        }
        sourceVoice_.assign(sourceChannelCount_, SIZE_MAX);
        for (size_t lane = 0; lane < voiceSources_.size(); ++lane) {
            for (size_t source : voiceSources_[lane]) {
                if (source >= sourceChannelCount_ || sourceVoice_[source] != SIZE_MAX)
                    throw std::runtime_error("Invalid stream track channel mapping.");
                sourceVoice_[source] = lane;
            }
            std::string name;
            if (sourceChannelCount_ == 1)
                name = "MONO";
            else if (splitStereoSides_)
                name = lane == 0 ? "LEFT" : "RIGHT";
            else if (explicitTrackGroups_)
                name = "TRACK " + std::to_string(lane + 1);
            else if (voiceSources_.size() == 1)
                name = "STEREO MIX";
            else
                name = voiceSources_[lane].size() == 1 ? "MONO TRACK " : "STEREO TRACK ";
            if (voiceSources_.size() > 1 && !explicitTrackGroups_ && !splitStereoSides_)
                name += std::to_string(lane + 1);
            channels_.push_back({std::move(name)});
        }
        if (std::any_of(sourceVoice_.begin(), sourceVoice_.end(), [](size_t lane) { return lane == SIZE_MAX; }))
            throw std::runtime_error("Incomplete stream track channel mapping.");
        tracks_.push_back({path.stem().u8string(), "", "", system_,
                           info.codec_name,
                           int(std::min(int64_t(2147483647), info.play_samples * 1000 / rate_))});
    }
    const std::vector<TrackInfo> &tracks() const override {
        return tracks_;
    }
    const std::vector<ChannelInfo> &channels() const override {
        return channels_;
    }
    bool decodedEndFrame(uint64_t&v)const override{if(endFrames_==UINT64_MAX)return false;v=endFrames_;return true;}
    void selectTrack(int index) override {
        if (index != 0)
            throw std::runtime_error("Stream file contains one track.");
        seek(0);
    }
    void muteMask(uint32_t mask) override {
        mask_ = mask;
    }
    void seek(int ms) override {
        if (ms < 0)
            throw std::runtime_error("Invalid stream seek time.");
        libvgmstream_seek(core_.get(), int64_t(ms) * rate_ / 1000);
        pending_.clear();
        position_ = 0;
    }
    void render(size_t frames, AudioBlock &out) override {
        if (frames > 8192)
            throw std::runtime_error("Audio block too large.");
        out.mix.assign(frames, {});
        out.voices.resize(channels_.size());
        for (auto &voice : out.voices)
            voice.assign(frames, {});
        for (size_t i = 0; i < frames; i++) {
            while (pending_.size() < 2)
                fill();
            for (size_t c = 0; c < sourceChannelCount_; c++) {
                float value = float(pending_[0][c] + (pending_[1][c] - pending_[0][c]) * position_);
                if (!std::isfinite(value))
                    throw std::runtime_error("Non-finite stream audio.");
                Stereo voice{value * routing_[c].l, value * routing_[c].r};
                const size_t lane = sourceVoice_[c];
                out.voices[lane][i].l += voice.l;
                out.voices[lane][i].r += voice.r;
                if (!(mask_ & (1u << lane))) {
                    out.mix[i].l += voice.l;
                    out.mix[i].r += voice.r;
                }
            }
            position_ += double(rate_) / InternalRate;
            while (position_ >= 1) {
                if (pending_.size() < 2)
                    fill();
                pending_.pop_front();
                position_ -= 1;
            }
        }
    }
};
} // namespace
std::unique_ptr<IMusicBackend> makeStreamBackend(const std::filesystem::path &path, const std::string &system,
                                                 bool groupStereoTracks, bool splitTwoChannelStereo) {
    return std::make_unique<StreamBackend>(path, system, groupStereoTracks, splitTwoChannelStereo);
}
} // namespace ngmv
