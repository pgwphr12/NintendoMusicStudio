#include "PitchTempo.hpp"
#include "Resampler.hpp"
#include "signalsmith-stretch.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
namespace ngmv {
struct PitchTempo::State {
    signalsmith::stretch::SignalsmithStretch<float> stretch{1415};
    OutputResampler resampler;
    StereoRing<8192> stage;
    std::array<std::array<float, 8192>, 2> in{};
    std::array<std::array<float, 512>, 2> out{};
    std::array<Stereo, 512> interleaved{};
    std::array<double, 8192> positions{};
    uint64_t written = 0, read = 0, sourceRead = 0, base = 0;
    double position = 0, fraction = 0;
    bool active = false, padded = false;
    int rate = 96000;
    State() {
        stretch.configure(2, 2048, 256);
        resampler.rate(rate);
    }
    template <size_t N> void take(StereoRing<N> &source, int count) {
        int actual = int(std::min(size_t(count), source.available()));
        for (int i = 0; i < count; ++i) {
            Stereo v{};
            if (i < actual)
                source.peek(i, v);
            in[0][i] = v.l;
            in[1][i] = v.r;
        }
        source.consume(actual);
        sourceRead += actual;
    }
    bool generate(StereoRing<> &source, double speed, double pitch, bool finished) {
        const double limit = finished ? double(sourceRead + source.available())
                                      : std::numeric_limits<double>::infinity();
        if (position >= limit) {
            if (!padded) {
                interleaved.fill({});
                stage.push(interleaved.data(), 32);
                for (int i = 0; i < 32; ++i)
                    positions[(written++) % 8192] = limit;
                padded = true;
            }
            return false;
        }
        int frames = 512;
        if (finished)
            frames = std::min(frames, int(std::ceil((limit - position) / speed)));
        double exact = frames * speed + fraction;
        int input = int(exact);
        if (!finished && source.available() < size_t(input))
            return false;
        fraction = exact - input;
        take(source, input);
        const float *inputs[] = {in[0].data(), in[1].data()};
        float *outputs[] = {out[0].data(), out[1].data()};
        stretch.setTransposeFactor(float(pitch));
        stretch.process(inputs, input, outputs, frames);
        for (int i = 0; i < frames; ++i) {
            interleaved[i] = {out[0][i], out[1][i]};
            position = std::min(limit, position + speed);
            positions[(written++) % 8192] = position;
        }
        return stage.push(interleaved.data(), frames);
    }
};
PitchTempo::PitchTempo() : state_(std::make_unique<State>()) {}
PitchTempo::~PitchTempo() = default;
void PitchTempo::rate(int r) {
    state_->resampler.rate(r);
    state_->rate = r;
}
void PitchTempo::reset() {
    auto &s = *state_;
    s.stretch.reset();
    s.resampler.reset();
    s.stage.reset();
    s.written = s.read = s.sourceRead = s.base = 0;
    s.position = s.fraction = 0;
    s.active = s.padded = false;
    consumed = underruns = 0;
}
bool PitchTempo::drained(const StereoRing<> &source, bool finished) const {
    auto &s = *state_;
    return finished && (s.active ? s.padded && s.stage.available() <= 32 : source.available() <= 2);
}
void PitchTempo::pull(StereoRing<> &source, float *output, size_t frames, float master,
                      double speed, double pitch, bool finished) {
    auto &s = *state_;
    speed = std::clamp(speed, .5, 2.);
    pitch = std::clamp(pitch, .5, 2.);
    if (!s.active && speed == 1 && pitch == 1) {
        s.resampler.pull(source, output, frames, master);
        consumed = s.resampler.consumed;
        underruns = s.resampler.underruns;
        return;
    }
    if (!s.active) {
        int preload = s.stretch.outputSeekLength(float(speed));
        if (!finished && source.available() < size_t(preload + 1024)) {
            std::fill(output, output + frames * 2, 0.f);
            underruns += frames;
            return;
        }
        s.base = consumed;
        s.resampler.reset();
        s.sourceRead = 0;
        s.stretch.setTransposeFactor(float(pitch));
        s.take(source, preload);
        const float *inputs[] = {s.in[0].data(), s.in[1].data()};
        s.stretch.outputSeek(inputs, preload);
        s.active = true;
    }
    size_t done = 0;
    while (done < frames) {
        size_t count = std::min(size_t(64), frames - done);
        size_t needed = size_t(std::ceil(count * double(InternalRate) / s.rate)) + 32;
        while (s.stage.available() < needed) {
            if (!s.generate(source, speed, pitch, finished))
                break;
        }
        if (s.stage.available() < 2) {
            std::fill(output + done * 2, output + frames * 2, 0.f);
            if (!finished)
                underruns += frames - done;
            break;
        }
        size_t possible =
            size_t(std::floor(double(s.stage.available() - 1) * s.rate / InternalRate));
        count = std::min(count, possible);
        if (!count) {
            std::fill(output + done * 2, output + frames * 2, 0.f);
            if (!finished)
                underruns += frames - done;
            break;
        }
        auto before = s.resampler.consumed;
        s.resampler.pull(s.stage, output + done * 2, count, master);
        auto used = s.resampler.consumed - before;
        if (used) {
            s.read += used;
            consumed = s.base + uint64_t(s.positions[(s.read - 1) % 8192]);
        }
        done += count;
    }
}
} // namespace ngmv
