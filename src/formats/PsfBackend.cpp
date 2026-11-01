#include "PsfBackend.hpp"
#include "GbaMixerTap.hpp"
#include "GbaRomSample.hpp"
#include "GbaPsgMirror.hpp"
#include "NintendoMixerTap.hpp"
#include "UsfVoiceTap.hpp"
#include <GBA.h>
#include <Gb_Apu.h>
extern "C" {
#include <state.h>
#include <usf.h>
}
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <deque>
#include <fstream>
#include <map>
#include <set>
#include <stdexcept>
#include <zlib.h>
namespace ngmv {
namespace {
uint32_t le(const uint8_t *p) {
    return p[0] | uint32_t(p[1]) << 8 | uint32_t(p[2]) << 16 | uint32_t(p[3]) << 24;
}
void need(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
constexpr size_t MaxRom = 256 * 1024 * 1024;
std::vector<uint8_t> inflateBody(const uint8_t *p, size_t n, size_t cap) {
    if (!n)
        return {};
    z_stream stream{};
    stream.next_in = const_cast<Bytef *>(p);
    stream.avail_in = unsigned(n);
    need(inflateInit(&stream) == Z_OK, "Cannot initialize PSF decompressor.");
    std::vector<uint8_t> out;
    std::array<uint8_t, 65536> block;
    int status;
    do {
        stream.next_out = block.data();
        stream.avail_out = unsigned(block.size());
        status = inflate(&stream, Z_NO_FLUSH);
        size_t count = block.size() - stream.avail_out;
        if (out.size() + count > cap) {
            inflateEnd(&stream);
            throw std::runtime_error("PSF expanded data exceeds limit.");
        }
        out.insert(out.end(), block.data(), block.data() + count);
    } while (status == Z_OK);
    bool ok = status == Z_STREAM_END && stream.avail_in == 0;
    inflateEnd(&stream);
    need(ok, "Invalid compressed PSF data.");
    return out;
}
struct Image {
    Format format;
    std::vector<uint8_t> rom, save;
    std::vector<std::vector<uint8_t>> usfSections;
    std::map<std::string, std::string> tags;
    std::set<std::filesystem::path> active;
    size_t budget = 0, fileCount = 0;
    uint32_t entry = 0;
    bool entrySet = false;
    void map(const std::vector<uint8_t> &body, bool saved) {
        if (body.empty())
            return;
        size_t header = format == Format::Gsf && !saved ? 12 : 8;
        need(body.size() >= header, "Truncated PSF memory map.");
        uint32_t offset = le(body.data() + (header == 12 ? 4 : 0));
        if (header == 12) {
            if (!entrySet) {
                entry = le(body.data());
                entrySet = true;
            }
            offset &= 0x1ffffff;
        }
        size_t count = le(body.data() + header - 4);
        size_t cap = saved ? 32 * 1024 * 1024 : (format == Format::Gsf ? 32 * 1024 * 1024 : MaxRom);
        need(count <= body.size() - header && offset <= cap && count <= cap - offset,
             "Invalid PSF memory map bounds.");
        auto &dest = saved ? save : rom;
        dest.resize(std::max(dest.size(), size_t(offset) + count));
        std::copy_n(body.data() + header, count, dest.data() + offset);
    }
    void load(const std::filesystem::path &path) {
        auto absolute = std::filesystem::weakly_canonical(path);
        need(fileCount++ < 64 && active.size() < 16, "PSF library chain is too large.");
        need(active.insert(absolute).second, "Circular PSF library reference.");
        std::ifstream f(absolute, std::ios::binary | std::ios::ate);
        if (!f)
            throw std::runtime_error("Missing PSF music/library file: " + path.u8string());
        auto size = f.tellg();
        need(size >= 16 && size <= 64 * 1024 * 1024, "Invalid PSF file size.");
        std::vector<uint8_t> bytes(static_cast<size_t>(size));
        f.seekg(0);
        need(bool(f.read(reinterpret_cast<char *>(bytes.data()), size)), "Incomplete PSF read.");
        need(!memcmp(bytes.data(), "PSF", 3) && bytes[3] == (format == Format::Gsf   ? 0x22
                                                             : format == Format::Usf ? 0x21
                                                                                     : 0x24),
             "PSF library type mismatch.");
        size_t reserved = le(bytes.data() + 4), compressed = le(bytes.data() + 8);
        need(reserved <= bytes.size() - 16 && compressed <= bytes.size() - 16 - reserved,
             "Truncated PSF sections.");
        need(crc32(0, bytes.data() + 16 + reserved, unsigned(compressed)) == le(bytes.data() + 12),
             "PSF compressed-data CRC mismatch.");
        std::map<std::string, std::string> local;
        size_t tagStart = 16 + reserved + compressed;
        if (bytes.size() - tagStart >= 5 && !memcmp(bytes.data() + tagStart, "[TAG]", 5)) {
            std::string text(reinterpret_cast<char *>(bytes.data() + tagStart + 5),
                             bytes.size() - tagStart - 5);
            size_t begin = 0;
            while (begin < text.size()) {
                auto end = text.find('\n', begin);
                if (end == std::string::npos)
                    end = text.size();
                auto line = text.substr(begin, end - begin);
                if (!line.empty() && line.back() == '\r')
                    line.pop_back();
                auto split = line.find('=');
                if (split != std::string::npos) {
                    auto key = line.substr(0, split);
                    for (auto &c : key)
                        c = char(std::tolower(static_cast<unsigned char>(c)));
                    local[key] = line.substr(split + 1);
                }
                begin = end + 1;
            }
        }
        auto library = [&](const std::string &key) {
            auto i = local.find(key);
            if (i != local.end()) {
                auto lib = std::filesystem::u8path(i->second);
                need(!lib.is_absolute(), "Absolute PSF library references are unsupported.");
                load(absolute.parent_path() / lib);
            }
        };
        library("_lib");
        auto body = inflateBody(bytes.data() + 16 + reserved, compressed, MaxRom + 12);
        budget += body.size() + reserved;
        need(budget <= MaxRom * 2, "PSF chain expanded data exceeds limit.");
        if (format == Format::Usf) {
            need(body.empty(), "USF executable section must be empty.");
            std::vector<uint8_t> section(bytes.begin() + 16, bytes.begin() + 16 + reserved);
            size_t at = 0;
            for (int kind = 0; !section.empty() && kind < 2; kind++) {
                need(section.size() - at >= 4, "Truncated USF section.");
                uint32_t id = le(section.data() + at);
                at += 4;
                if (id == 0)
                    continue;
                need(id == 0x34365253, "Unknown USF section marker.");
                while (true) {
                    need(section.size() - at >= 4, "Truncated USF chunk.");
                    size_t length = le(section.data() + at);
                    at += 4;
                    if (!length)
                        break;
                    need(section.size() - at >= 4, "Truncated USF offset.");
                    size_t offset = le(section.data() + at);
                    at += 4;
                    size_t cap = kind == 0 ? 64 * 1024 * 1024 : 0x80275c;
                    need(offset <= cap && length <= cap - offset && length <= section.size() - at,
                         "USF chunk exceeds memory bounds.");
                    if (kind == 1) {
                        save.resize(std::max(save.size(), offset + length));
                        std::copy_n(section.data() + at, length, save.data() + offset);
                    }
                    at += length;
                }
            }
            need(at == section.size(), "Trailing invalid USF section data.");
            usfSections.push_back(std::move(section));
        } else
            map(body, false);
        if (format == Format::TwoSf && reserved) {
            size_t at = 16, end = 16 + reserved;
            while (at < end) {
                need(end - at >= 12, "Truncated 2SF reserved block.");
                size_t count = le(bytes.data() + at + 4);
                need(count <= end - at - 12, "Invalid 2SF reserved block size.");
                if (!memcmp(bytes.data() + at, "SAVE", 4)) {
                    auto saved = inflateBody(bytes.data() + at + 12, count, 32 * 1024 * 1024);
                    budget += saved.size();
                    need(budget <= MaxRom * 2, "PSF chain expanded data exceeds limit.");
                    auto crc = le(bytes.data() + at + 8);
                    need(!crc || crc32(0, saved.data(), unsigned(saved.size())) == crc,
                         "2SF SAVE CRC mismatch.");
                    map(saved, true);
                }
                at += 12 + count;
            }
        }
        for (int n = 2; n <= 99; n++)
            library("_lib" + std::to_string(n));
        for (auto &tag : local)
            tags[tag.first] = tag.second;
        active.erase(absolute);
    }
    int number(const char *key, int fallback = 0) const {
        auto i = tags.find(key);
        if (i == tags.end())
            return fallback;
        try {
            return std::stoi(i->second);
        } catch (...) {
            return fallback;
        }
    }
    std::string tag(const char *key) const {
        auto i = tags.find(key);
        return i == tags.end() ? "" : i->second;
    }
    void finish() {
        if (format == Format::Usf) {
            need(save.size() >= 0x75c && le(save.data()) == 0x23d8a6c8 &&
                     (le(save.data() + 4) == 0x400000 || le(save.data() + 4) == 0x800000),
                 "USF chain lacks a valid Project64 music state.");
            return;
        }
        need(rom.size() >= (format == Format::Gsf ? 4 : 512),
             "PSF chain contains no playable image.");
        size_t padded = 1;
        while (padded < rom.size())
            padded *= 2;
        rom.resize(padded);
        if (format == Format::TwoSf)
            for (size_t at : {size_t(0x20), size_t(0x30)}) {
                size_t start = le(rom.data() + at), length = le(rom.data() + at + 12);
                need(start <= rom.size() && length <= rom.size() - start,
                     "Invalid 2SF CPU program bounds.");
            }
    }
};
struct GbaCore : GBASoundOut {
    GBASystem system;
    GbaMixerTap mixer;
    NintendoMixerTap nintendo;
    std::unique_ptr<GbaPsgMirror> psg;
    std::deque<Stereo> pending;
    GbaCore(const Image &image, int enabled,int voice=-1,uint32_t info=0,int voices=0,std::vector<NintendoVoicePool> pools={}) : nintendo(std::move(pools),voice) {
        system.cpuIsMultiBoot = (image.entry >> 24) == 2;
        need(CPULoadRom(&system, image.rom.data(), unsigned(image.rom.size())) > 0,
             "Cannot load GSF music image.");
        soundInit(&system, this);
        soundSetSampleRate(&system, InternalRate);
        soundReset(&system);
        if(enabled==0x30f&&!std::getenv("NGMV_GBA_CPU_PSG")){psg=std::make_unique<GbaPsgMirror>(system);system.viewerApuContext=psg.get();system.viewerApuEvent=&GbaPsgMirror::callback;}
        CPUInit(&system);
        CPUReset(&system);
        mixer.isolate=voice;mixer.info=info;mixer.voices=voices;system.viewerMixerContext=this;system.viewerMixerWrite=&GbaCore::filter;
        soundSetEnable(&system, enabled);
        soundResume(&system);
        if(enabled==0x30f)system.gb_apu->viewer_inspection(true);
    }
    ~GbaCore() override {
        soundShutdown(&system);
        CPUCleanUp(&system);
    }
    void write(const void *data, unsigned long bytes) override {
        auto samples = static_cast<const int16_t *>(data);
        for (size_t i = 0; i < bytes / 4; i++)
            pending.push_back({samples[i * 2] / 32768.f, samples[i * 2 + 1] / 32768.f});
    }
    void render(size_t frames, std::vector<Stereo> &out) {
        for (int attempts = 0; pending.size() < frames; attempts++) {
            need(attempts < 100, "GSF core did not produce audio.");
            CPULoop(&system, 250000);
        }
        need(!nintendo.fault,"Unsupported Nintendo mixer context/kernel change; no partial voice output");
        out.resize(frames);
        for (auto &v : out) {
            v = pending.front();
            pending.pop_front();
        }
    }
    static u32 filter(void*p,GBASystem*g,u32 address,u32 value,int width){auto&core=*static_cast<GbaCore*>(p);auto result=core.mixer.filter(g,address,value,width);return core.mixer.info?result:core.nintendo.filter(g,address,result,width);}
    void probe(){
        int limit=nintendo.candidates.empty()?50:200;
        for(int i=0;i<limit&&mixer.recognized<=32&&nintendo.recognized<=32;i++){CPULoop(&system,250000);if(!mixer.info)mixer.scan(&system);}
    }
};
using DsFrame = std::array<Stereo, 17>;
struct DsCore {
    NDS_state state{};
    std::deque<std::array<Stereo, 16>> tapped;
    std::deque<DsFrame> pending;
    double position = 0;
    explicit DsCore(Image &image) {
        if (state_init(&state)) {
            state_deinit(&state);
            throw std::runtime_error("Cannot allocate DS audio core.");
        }
        state.dwInterpolation = 4;
        state.initial_frames = std::clamp(image.number("_frames"), -1, 600);
        state.sync_type = std::clamp(image.number("_vio2sf_sync_type"), 0, 2);
        state.arm7_clockdown_level = std::clamp(
            image.number("_vio2sf_arm7_clockdown_level", image.number("_clockdown")), 0, 4);
        state.arm9_clockdown_level = std::clamp(
            image.number("_vio2sf_arm9_clockdown_level", image.number("_clockdown")), 0, 4);
        state_setrom(&state, image.rom.data(), unsigned(image.rom.size()), 0);
        state_loadstate(&state, image.save.data(), unsigned(image.save.size()));
        state.viewer_context = this;
        state.viewer_pcm = [](void *context, const float *pcm, unsigned frames) {
            auto &core = *static_cast<DsCore *>(context);
            for (unsigned i = 0; i < frames; i++) {
                std::array<Stereo, 16> frame{};
                for (size_t c = 0; c < 16; c++)
                    frame[c] = {pcm[i * 32 + c * 2], pcm[i * 32 + c * 2 + 1]};
                core.tapped.push_back(frame);
            }
        };
    }
    ~DsCore() {
        state_deinit(&state);
    }
    void fill() {
        std::array<int16_t, 2048> mix{};
        state_render(&state, mix.data(), 1024);
        need(tapped.size() >= 1024, "DS channel tap lost sample alignment.");
        for (size_t i = 0; i < 1024; i++) {
            DsFrame frame{};
            frame[0] = {mix[i * 2] / 32768.f, mix[i * 2 + 1] / 32768.f};
            auto voices = tapped.front();
            tapped.pop_front();
            std::copy(voices.begin(), voices.end(), frame.begin() + 1);
            pending.push_back(frame);
        }
    }
    DsFrame next() {
        while (pending.size() < 2)
            fill();
        auto &a = pending[0];
        auto &b = pending[1];
        DsFrame frame{};
        for (size_t c = 0; c < 17; c++)
            frame[c] = {float(a[c].l + (b[c].l - a[c].l) * position),
                        float(a[c].r + (b[c].r - a[c].r) * position)};
        position += 44100. / InternalRate;
        while (position >= 1) {
            pending.pop_front();
            position -= 1;
        }
        return frame;
    }
};
struct UsfCore {
    std::vector<uint8_t> state;
    UsfVoiceTap tap;
    std::deque<UsfVoiceTap::Frame> pending;
    double position = 0;
    int32_t rate = 0;
    explicit UsfCore(const Image &image) : state(usf_get_state_size()) {
        usf_clear(state.data());
        for (auto &section : image.usfSections) {
            if (section.empty())
                continue;
            if (usf_upload_section(state.data(), section.data(), section.size())) {
                usf_shutdown(state.data());
                throw std::runtime_error("Cannot upload USF music state.");
            }
        }
        usf_set_compare(state.data(), image.tags.count("_enablecompare") != 0);
        usf_set_fifo_full(state.data(), image.tags.count("_enablefifofull") != 0);
        usf_set_hle_audio(state.data(), 1);
        usf_set_voice_tap(state.data(), &tap.callbacks);
    }
    ~UsfCore() {
        usf_shutdown(state.data());
    }
    void fill() {
        std::array<int16_t, 2048> mix{};
        const char *error = usf_render(state.data(), mix.data(), 1024, &rate);
        if (error)
            throw std::runtime_error(error);
        need(rate >= 8000 && rate <= 192000, "Invalid USF output sample rate.");
        need(tap.pending.size() >= 1024, "N64 voice tap lost sample alignment.");
        for (size_t i = 0; i < 1024; i++) {
            auto frame = tap.pending.front();
            tap.pending.pop_front();
            need(std::abs(frame[0].l - mix[i * 2] / 32768.f) < .0001f &&
                     std::abs(frame[0].r - mix[i * 2 + 1] / 32768.f) < .0001f,
                 "N64 DMA tap lost sample alignment.");
            pending.push_back(frame);
        }
    }
    UsfVoiceTap::Frame next() {
        while (pending.size() < 2)
            fill();
        auto a = pending[0], b = pending[1];
        UsfVoiceTap::Frame value{};
        for (size_t c = 0; c < value.size(); ++c)
            value[c] = {float(a[c].l + (b[c].l - a[c].l) * position),
                        float(a[c].r + (b[c].r - a[c].r) * position)};
        position += double(rate) / InternalRate;
        while (position >= 1) {
            if (pending.size() < 2)
                fill();
            pending.pop_front();
            position -= 1;
        }
        return value;
    }
};
class PsfBackend final : public IMusicBackend {
    Image image_;
    std::vector<TrackInfo> tracks_;
    std::vector<ChannelInfo> channels_;
    std::vector<std::unique_ptr<GbaCore>> gba_;
    std::unique_ptr<DsCore> ds_;
    std::unique_ptr<UsfCore> usf_;
    uint32_t mask_ = 0;
    uint64_t clock_ = 0;
    int fadeStart_ = -1, fadeMs_ = 0;
    int gbaVoices_=0,nintendoVoices_=0;
    std::vector<NintendoVoicePool> nintendoPools_;
    int gbaVoiceCount()const{return gbaVoices_?gbaVoices_:nintendoVoices_;}
    uint32_t gbaMixerInfo_=0;
    bool continuousEnd_=false;
    bool gbaInspection_=false,gbaMixInspection_=false;
    static int milliseconds(const std::string &text) {
        if (text.empty())
            return -1;
        double seconds = 0;
        size_t start = 0;
        try {
            while (start < text.size()) {
                auto end = text.find(':', start);
                auto part = text.substr(start, end == std::string::npos ? end : end - start);
                size_t consumed = 0;
                double value = std::stod(part, &consumed);
                if (consumed != part.size() || value < 0)
                    return -1;
                seconds = seconds * 60 + value;
                if (end == std::string::npos)
                    break;
                start = end + 1;
            }
        } catch (...) {
            return -1;
        }
        return std::isfinite(seconds) && seconds < 86400 ? int(std::round(seconds * 1000)) : -1;
    }

  public:
    void continuousEnd(bool value) override {continuousEnd_=value;}
    bool prepareGbaInspection() override {
        if(image_.format!=Format::Gsf||!gbaVoices_)return false;
        gbaInspection_=true;gbaMixInspection_=false;selectTrack(0);return true;
    }
    bool prepareGbaMixerInspection() override {
        if(image_.format!=Format::Gsf||!gbaVoices_)return false;
        gbaInspection_=true;gbaMixInspection_=true;selectTrack(0);return true;
    }
    bool drainGbaMixFrames(std::vector<GbaMixFrame>&out,std::string&error) override {
        if(!gbaMixInspection_||gba_.empty()||!gba_[0]->mixer.mixCapture)return false;
        return gba_[0]->mixer.mixCapture->drain(out,error);
    }
    bool gbaPsgUsed(uint32_t&out) const override {if(gba_.empty()||!gba_[0]->system.gb_apu)return false;out=gba_[0]->system.gb_apu->viewer_psg_mask();return true;}
    bool gbaPsgActivity(uint32_t&out) const override {
        if(!gbaMixInspection_||gba_.empty()||!gba_[0]->system.gb_apu)return false;
        out=gba_[0]->system.gb_apu->viewer_psg_mask()|(gba_[0]->mixer.mixCapture&&gba_[0]->mixer.mixCapture->psgNoteSeen()?16u:0u);return true;
    }
    bool drainGbaEvents(std::vector<GbaVoiceEvent>&out,std::string&error) override {
        if(!gbaInspection_||gba_.empty()||!gba_[0]->mixer.capture)return false;
        return gba_[0]->mixer.capture->drain(out,error);
    }
    bool gbaEmulatedCycles(uint64_t&out) const override {
        if(!gbaInspection_||gba_.empty()||!gba_[0]->mixer.capture)return false;
        const auto&g=gba_[0]->system;out=g.viewerEventCycles+uint64_t(std::max(0,g.cpuTotalTicks));return true;
    }
    bool copyGbaSample(const GbaSampleInfo&info,GbaSampleData&out,std::string&error) const override {
        if(!gbaInspection_||gba_.empty()||!gba_[0]->mixer.capture)return false;
        const auto&g=gba_[0]->system;
        return copyGbaRomSample(g.rom,g.romSize>0?size_t(g.romSize):0,info,out,error);
    }
    PsfBackend(const std::filesystem::path &path, Format format) : image_{format} {
        need(!path.empty(), "GSF/2SF/USF requires a file path to resolve music libraries.");
        image_.load(path);
        image_.finish();
        fadeStart_ = milliseconds(image_.tag("length"));
        fadeMs_ = std::max(0, milliseconds(image_.tag("fade")));
        tracks_.push_back({image_.tag("title"), image_.tag("game"), image_.tag("artist"),
                           format == Format::Gsf   ? "Game Boy Advance"
                           : format == Format::Usf ? "Nintendo 64"
                                                   : "Nintendo DS",
                           image_.tag("comment"), fadeStart_ < 0 ? -1 : fadeStart_ + fadeMs_});
        if (format == Format::Gsf)
            for (auto name : {"PULSE 1", "PULSE 2", "WAVE", "NOISE", "PCM A", "PCM B"})
                channels_.push_back({name});
        else if (format == Format::Usf) {
            channels_.push_back({"LEFT"});
            channels_.push_back({"RIGHT"});
        } else
            for (int i = 1; i <= 16; i++)
                channels_.push_back({"CHANNEL " + std::to_string(i)});
        if(format==Format::Gsf){
            // Detection needs only the authoritative core. Previously all seven
            // full-ROM cores were built here and immediately thrown away.
#ifdef NGMV_GBA_LEGACY_BOOTSTRAP
            selectTrack(0); // Reproducible pre-1.4.0 startup, benchmark target only.
#else
            nintendoPools_=findNintendoVoicePools(image_.rom);
            gba_.push_back(std::make_unique<GbaCore>(image_,0x30f,-1,0,0,nintendoPools_));
#endif
            if(!std::getenv("NGMV_GBA_HARDWARE_ONLY"))gba_[0]->probe();
            if(std::getenv("NGMV_GBA_MIXER_DEBUG"))std::fprintf(stderr,"GBA mixer: layout=%d slots=%d RAMwrites=%llu recognized=%llu checks=%u\n",gba_[0]->mixer.info!=0,gba_[0]->mixer.voices,(unsigned long long)gba_[0]->mixer.writes,(unsigned long long)gba_[0]->mixer.recognized,gba_[0]->mixer.checks);
            if(std::getenv("NGMV_GBA_MIXER_DEBUG")&&gba_[0]->mixer.info){auto s=GbaMixerTap::ram(&gba_[0]->system,gba_[0]->mixer.info,24);std::fprintf(stderr,"GBA mixer fields: reverb=%u master=%u freq=%u samples=%u\n",s[5],s[7],GbaMixerTap::word(s+20),GbaMixerTap::word(s+16));}
            if(std::getenv("NGMV_GBA_MIXER_DEBUG")&&gba_[0]->mixer.info)std::fprintf(stderr,"GBA DMA offsets: A=%x B=%x\n",GbaMixerTap::word(gba_[0]->system.ioMem+0xbc)-gba_[0]->mixer.info,GbaMixerTap::word(gba_[0]->system.ioMem+0xc8)-gba_[0]->mixer.info);
            if(!std::getenv("NGMV_GBA_HARDWARE_ONLY")&&gba_[0]->mixer.recognized>32){gbaVoices_=gba_[0]->mixer.voices;gbaMixerInfo_=gba_[0]->mixer.info;channels_.resize(4);for(int i=0;i<gbaVoices_;i++)channels_.push_back({"PCM VOICE "+std::to_string(i+1)});channels_.push_back({"EFFECTS / OTHER"});}
            if(!std::getenv("NGMV_GBA_HARDWARE_ONLY")&&!gbaVoices_&&gba_[0]->nintendo.recognized>32&&!gba_[0]->nintendo.fault){auto profile=gba_[0]->nintendo.selected;nintendoVoices_=int(profile.count);nintendoPools_={profile};channels_.resize(4);for(int i=0;i<nintendoVoices_;i++)channels_.push_back({"PCM VOICE "+std::to_string(i+1)});channels_.push_back({"EFFECTS / OTHER"});}
            // Do not retain the probe's prefetched, unmuted output in either mode.
            selectTrack(0);
        } else selectTrack(0);
        if (usf_) {
            // Determine whether this game's microcode exposes envelope-state voice slots.
            // Preserve the probe samples so playback still starts at time zero.
            for (int i = 0; i < 8; ++i)
                usf_->fill();
            if (usf_->tap.used || usf_->tap.supported) {
                channels_.clear();
                for (size_t c = 0; c < UsfVoiceTap::Voices; ++c)
                    channels_.push_back({"CHANNEL " + std::to_string(c + 1)});
                channels_.push_back({"EFFECTS / OTHER"});
            }
        }
    }
    const std::vector<TrackInfo> &tracks() const override {
        return tracks_;
    }
    const std::vector<ChannelInfo> &channels() const override {
        return channels_;
    }
    void selectTrack(int index) override {
        need(index == 0, "PSF files contain one track per file.");
        gba_.clear();
        ds_.reset();
        usf_.reset();
        clock_ = 0;
        if (image_.format == Format::Gsf) {
            int bits[] = {1, 2, 4, 8, 0x100, 0x200};
            gba_.push_back(std::make_unique<GbaCore>(image_, 0x30f,-1,gbaMixerInfo_,gbaVoices_,nintendoVoices_?nintendoPools_:std::vector<NintendoVoicePool>{}));
            if(gbaVoiceCount()){if(!gba_[0]->psg)for(int c=0;c<4;c++)gba_.push_back(std::make_unique<GbaCore>(image_,bits[c]));for(int c=0;c<gbaVoiceCount();c++)gba_.push_back(std::make_unique<GbaCore>(image_,0x300,c,gbaMixerInfo_,gbaVoices_,nintendoVoices_?nintendoPools_:std::vector<NintendoVoicePool>{}));}
            else for (int c=gba_[0]->psg?4:0;c<6;c++)gba_.push_back(std::make_unique<GbaCore>(image_,bits[c]));
            if(gbaInspection_){gba_[0]->mixer.capture=std::make_unique<GbaVoiceCapture>();if(gbaMixInspection_){gba_[0]->mixer.mixCapture=std::make_unique<GbaMixCapture>();gba_[0]->system.gb_apu->viewer_inspection(true);}gba_[0]->system.viewerEventClock=true;}
        } else if (image_.format == Format::Usf)
            usf_ = std::make_unique<UsfCore>(image_);
        else
            ds_ = std::make_unique<DsCore>(image_);
        muteMask(mask_);
    }
    void muteMask(uint32_t mask) override {
        mask_ = mask;
        if (ds_)
            ds_->state.dwChannelMute = mask;
        if (!gba_.empty()&&!gbaVoiceCount()) {
            int bits[] = {1, 2, 4, 8, 0x100, 0x200};
            int enabled = 0;
            for (size_t c = 0; c < 6; c++)
                if (!(mask & (1u << c)))
                    enabled |= bits[c];
            soundSetEnable(&gba_[0]->system, enabled);
        }
    }
    void render(size_t frames, AudioBlock &out) override {
        need(frames <= 8192, "Audio block too large.");
        out.mix.resize(frames);
        out.voices.resize(channels_.size());
        for (auto &v : out.voices)
            v.resize(frames);
        if (usf_)
            for (size_t i = 0; i < frames; i++) {
                auto frame = usf_->next();
                if (channels_.size() == 2) {
                    out.voices[0][i] = {frame[0].l, 0};
                    out.voices[1][i] = {0, frame[0].r};
                    out.mix[i] = {(mask_ & 1) ? 0 : frame[0].l, (mask_ & 2) ? 0 : frame[0].r};
                } else {
                    out.mix[i] = {};
                    for (size_t c = 0; c < channels_.size(); ++c) {
                        out.voices[c][i] = frame[c + 1];
                        if (!(mask_ & (1u << c))) {
                            out.mix[i].l += frame[c + 1].l;
                            out.mix[i].r += frame[c + 1].r;
                        }
                    }
                    if (mask_ == 0)
                        out.mix[i] = frame[0]; // preserve the original clipped mix exactly
                }
            }
        else if (ds_)
            for (size_t i = 0; i < frames; i++) {
                auto frame = ds_->next();
                out.mix[i] = frame[0];
                for (size_t c = 0; c < 16; c++)
                    out.voices[c][i] = frame[c + 1];
            }
        else {
            gba_[0]->render(frames, out.mix);
            for (size_t c = 0; c < (gbaVoiceCount()?size_t(4+gbaVoiceCount()):6); c++){
                if(gba_[0]->psg&&c<4)gba_[0]->psg->render(int(c),frames,out.voices[c]);
                else gba_[c+1-(gba_[0]->psg?4:0)]->render(frames,out.voices[c]);
            }
            if(gbaVoiceCount()){
                for(size_t i=0;i<frames;i++){Stereo dry{};for(int c=0;c<4+gbaVoiceCount();c++){dry.l+=out.voices[c][i].l;dry.r+=out.voices[c][i].r;}out.voices.back()[i]={out.mix[i].l-dry.l,out.mix[i].r-dry.r};if(mask_){Stereo mix{};for(size_t c=0;c<out.voices.size();c++)if(!(mask_&(1u<<c))){mix.l+=out.voices[c][i].l;mix.r+=out.voices[c][i].r;}out.mix[i]=mix;}}
            }
        }
        // Fade tagged files in the original source-time domain, including isolated signals.
        for (size_t i = 0; i < frames; i++, clock_++)
            if (!continuousEnd_ && fadeStart_ >= 0 && double(clock_) * 1000 / InternalRate >= fadeStart_) {
                float gain =
                    fadeMs_
                        ? float(std::clamp(
                              1. - (double(clock_) * 1000 / InternalRate - fadeStart_) / fadeMs_,
                              0., 1.))
                        : 0;
                out.mix[i].l *= gain;
                out.mix[i].r *= gain;
                for (auto &v : out.voices) {
                    v[i].l *= gain;
                    v[i].r *= gain;
                }
            }
    }
    void seek(int ms) override {
        need(ms >= 0, "Invalid seek time.");
        selectTrack(0);
        size_t remaining = size_t(ms) * InternalRate / 1000;
        AudioBlock discarded;
        while (remaining) {
            size_t count = std::min(remaining, size_t(4096));
            render(count, discarded);
            remaining -= count;
        }
    }
};
} // namespace
std::unique_ptr<IMusicBackend> makePsfBackend(const std::filesystem::path &path, Format format) {
    return std::make_unique<PsfBackend>(path, format);
}
} // namespace ngmv
