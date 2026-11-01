#include "core/IMusicBackend.hpp"
#include "player/Player.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <thread>
using namespace ngmv;
static void need(bool b, const char *s) {
    if (!b)
        throw std::runtime_error(s);
}
static void put(std::vector<uint8_t> &b, size_t p, uint32_t v) {
    for (int i = 0; i < 4; ++i)
        b[p + i] = uint8_t(v >> (8 * i));
}
static std::vector<uint8_t> fixture() {
    std::vector<uint8_t> b(0x100);
    std::memcpy(b.data(), "Vgm ", 4);
    put(b, 8, 0x171);
    put(b, 0x34, 0xcc);
    put(b, 0x9c, 2000000);
    auto write = [&](int p, int r, int v) {
        b.insert(b.end(), {0xd2, uint8_t(p), uint8_t(r), uint8_t(v)});
    };
    for (int i = 0; i < 32; ++i)
        write(0, i, int(110 * std::sin(i * 6.283185307 / 32)));
    write(1, 0, 140);
    write(1, 1, 0);
    write(2, 0, 15);
    write(3, 0, 1);
    b.insert(b.end(), {0x61, 0x44, 0xac, 0x66});
    put(b, 4, uint32_t(b.size() - 4));
    put(b, 0x18, 44100);
    return b;
}
static double difference(const std::vector<Stereo> &a, const std::vector<Stereo> &b) {
    need(a.size() == b.size(), "PCM lengths");
    double e = 0;
    for (size_t i = 0; i < a.size(); ++i)
        e = std::max(e, double(std::max(std::abs(a[i].l - b[i].l), std::abs(a[i].r - b[i].r))));
    return e;
}
static void structural() {
    auto b = fixture();
    auto core = makeBackend(b);
    need(core->channels().size() == 5, "Own SCC channel count");
    AudioBlock out;
    core->render(4096, out);
    double e = 0;
    for (auto x : out.mix)
        e += x.l * x.l + x.r * x.r;
    need(e > 1, "Own SCC must produce real audio");
    // One complete 32-sample sine at SCC register140. Native synthesis runs
    // at clock/16, not the application's 48kHz output rate. Testing only
    // nonzero energy missed the low-pitched playback regression in16/17.
    core->selectTrack(0);
    unsigned rising=0;float previous=0;
    for(unsigned batch=0;batch<12;++batch){
        core->render(2000,out);
        for(auto x:out.mix){if(previous<=0&&x.l>0)++rising;previous=x.l;}
    }
    double expected=2000000./(16*141);
    need(std::abs(rising*2.-expected)<4,"SCC native-rate conversion changed musical pitch");
    std::cout<<"SCC native-rate sine: "<<rising*2.<<"Hz expected="<<expected<<"Hz PASS\n";
    auto multi=b;put(multi,0x50,4000000); // declared but silent OPL2
    auto mixed=makeBackend(multi);core->selectTrack(0);
    AudioBlock single,multiple;core->render(4096,single);mixed->render(4096,multiple);
    need(difference(single.mix,multiple.mix)<1e-7,"Silent extra chip changed VGM mix gain");
    unsigned negatives = 0;
    auto reject = [&](std::vector<uint8_t> bad) {
        bool threw = false;
        try {
            auto p = makeBackend(bad);
            p->render(64, out);
        } catch (const std::exception &) {
            threw = true;
        }
        need(threw, "Malformed/unsupported VGM accepted");
        ++negatives;
    };
    for (size_t n = 0x100; n < b.size(); ++n) {
        auto bad = b;
        bad.resize(n);
        put(bad, 4, uint32_t(n - 4));
        reject(bad);
    }
    {
        auto x = b;
        put(x, 0x9c, 0x40000000 | 2000000);
        reject(x);
    }
    {
        auto x = b;
        put(x, 0x44, 3000000);
        reject(x);
    }
    {
        auto x = b;
        put(x, 0x34, 0xffffffff);
        reject(x);
    }
    {
        auto x = b;
        put(x, 0xbc, 0xfffffffe);
        reject(x);
    }
    {
        auto x = b;
        put(x, 0x1c, 0xe5);
        reject(x);
    }
    {
        auto x = b;
        put(x, 0x14, 0xfffffff0);
        reject(x);
    }
    {
        auto x = b;
        x[0x100] = 0xff;
        reject(x);
    }
    {
        auto x = b;
        x.resize(0x100);
        x.insert(x.end(), {0x67, 0x66, 0x8b, 8, 0, 0, 0});
        x.resize(x.size() + 8);
        put(x, 0x107, 0xffffffff);
        x.insert(x.end(), {0x62, 0x66});
        put(x, 4, uint32_t(x.size() - 4));
        reject(x);
    }
    core->muteMask(31);
    core->seek(0);
    core->render(4096, out);
    for (auto x : out.mix)
        need(x.l == 0 && x.r == 0, "Own SCC all mute");
    std::cout << "Own SCC synthesis, " << negatives
              << " bounded malformed/unsupported cases, mute PASS\n";
}
static void licensedEsFixture(){
    std::vector<uint8_t>b(256);std::memcpy(b.data(),"Vgm ",4);put(b,8,0x171);put(b,0x34,0xcc);put(b,0xd0,0x80000000u|16000000);
    auto byte=[&](uint8_t r,uint8_t v){b.insert(b.end(),{0xbe,r,v});};
    auto reg=[&](uint8_t r,uint32_t value){for(int i=0;i<4;++i)byte(uint8_t(r*4+i),uint8_t(value>>(24-8*i)));};
    b.insert(b.end(),{0x67,0x66,0x90});size_t block=b.size();b.resize(block+12);put(b,block,8+256);put(b,block+4,4096);put(b,block+8,0x80000000u);
    for(int i=0;i<256;++i)b.push_back(uint8_t(int(110*std::sin(i*6.283185307/128))));
    reg(15,0x20);reg(1,0);reg(2,127u<<11);reg(3,0);
    reg(15,0);reg(1,0x800);reg(2,0xffff);reg(4,0xffff);reg(7,0xffff);reg(9,0xffff);reg(0,0x308);
    b.insert(b.end(),{0x61,0x44,0xac,0x66});put(b,4,uint32_t(b.size()-4));put(b,0x18,44100);
    auto backend=makeBackend(b);AudioBlock out;backend->render(4096,out);double energy=0;
    for(auto v:out.mix){need(std::isfinite(v.l)&&std::isfinite(v.r),"Licensed ES nonfinite");energy+=v.l*v.l+v.r*v.r;}
    need(backend->channels().size()==1 && energy>1,"Licensed ES did not synthesize real supplied sample ROM");
    need(difference(out.mix,out.voices[0])<1e-6,"ES bus tap differs from full mix");
    backend->muteMask(1);backend->seek(0);backend->render(4096,out);for(auto v:out.mix)need(v.l==0&&v.r==0,"Licensed ES mute");
    put(b,0xd0,16000000);bool rejected=false;try{auto unsupported=makeBackend(b);}catch(const std::exception&){rejected=true;}need(rejected,"ES5505 incorrectly reported as ES5506");
    std::cout<<"Licensed zlib ES5506 own ROM/host writes/real synthesis/bus/mute/model rejection PASS\n";
}
static void ownDacStream(){
    for(bool fast:{false,true}){
        std::vector<uint8_t>b(256);std::memcpy(b.data(),"Vgm ",4);put(b,8,0x171);put(b,0x34,0xcc);
        put(b,0x2c,7670454);put(b,0x50,3579545); // routes to the new multi-chip engine
        b.insert(b.end(),{0x52,0x2b,0x80,0x67,0x66,0});size_t length=b.size();b.resize(length+4);put(b,length,256);
        for(int i=0;i<256;++i)b.push_back(uint8_t(128+int(100*std::sin(i*6.283185307/64))));
        b.insert(b.end(),{0x90,0,2,0,0x2a,0x91,0,0,1,0,0x92,0});size_t frequency=b.size();b.resize(frequency+4);put(b,frequency,16000);
        if(fast)b.insert(b.end(),{0x95,0,0,0,1});
        else {b.insert(b.end(),{0x93,0,0,0,0,0,0x81,0,1,0,0});}
        b.insert(b.end(),{0x61,0x44,0xac,0x94,0,0x66});put(b,4,uint32_t(b.size()-4));put(b,0x18,44100);
        auto core=makeBackend(b);AudioBlock out;core->render(4096,out);double energy=0;for(auto v:out.voices[6])energy+=v.l*v.l+v.r*v.r;
        need(energy>1,"New VGM stream failed to drive actual YM2612 DAC");
    }
    std::cout<<"Own YM2612 DAC normal/fast stream start and loop: PASS\n";
}
static void actual(const std::filesystem::path &path, const std::filesystem::path &output) {
    auto bytes = readMusicFile(path);
    auto core = makeBackend(bytes, path);
    AudioBlock b;
    unsigned channels = unsigned(core->channels().size());
    std::cout << path.filename().u8string() << " channels=" << channels
              << " durationMs=" << core->tracks()[0].lengthMs << std::endl;
    std::vector<double> energy(channels);
    double mix = 0, peak = 0, sumError = 0;
    auto start = std::chrono::steady_clock::now();
    size_t total = size_t(core->tracks()[0].lengthMs + 1000) * 48;
    for (size_t n = 0; n < total;) {
        size_t frames = std::min<size_t>(4096, total - n);
        core->render(frames, b);
        n += frames;
        for (size_t i = 0; i < frames; ++i) {
            auto x = b.mix[i];
            need(std::isfinite(x.l) && std::isfinite(x.r), "Nonfinite VGM output");
            mix += x.l * x.l + x.r * x.r;
            peak = std::max(peak, double(std::max(std::abs(x.l), std::abs(x.r))));
            Stereo sum{};
            for (unsigned c = 0; c < channels; ++c) {
                auto v = b.voices[c][i];
                need(std::isfinite(v.l) && std::isfinite(v.r), "Nonfinite VGM stem");
                energy[c] += v.l * v.l + v.r * v.r;
                sum.l += v.l;
                sum.r += v.r;
            }
            sumError =
                std::max(sumError, double(std::max(std::abs(sum.l - x.l), std::abs(sum.r - x.r))));
        }
    }
    need(mix > 1, "Full VGM silent");
    need(sumError<.001,"Chip stems no longer recombine within core quantization tolerance");
    double seconds =
        std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    std::cout << "Full command timeline+1s rendered=" << double(total) / 48000
              << "s wall=" << seconds << "s peak=" << peak << " stemSumMax=" << sumError
              << std::endl;
    for (unsigned i = 0; i < channels; ++i)
        std::cout << core->channels()[i].name << " energy=" << energy[i] << '\n';
    core->selectTrack(0);
    core->render(4096, b);
    auto initial = b.mix;
    core->selectTrack(0);
    core->render(4096, b);
    need(difference(initial, b.mix) < 1e-6, "Replay changed VGM PCM");
    core->selectTrack(0);
    for (int i = 0; i < 6; ++i)
        core->render(4000, b);
    core->render(1024, b);
    auto after500 = b.mix;
    core->seek(500);
    core->render(1024, b);
    need(difference(after500, b.mix) < 1e-6, "VGM seek mismatch");
    core->muteMask((1u << channels) - 1);
    core->seek(500);
    for (int k = 0; k < 8; ++k)
        core->render(1024, b);
    for (auto s : b.mix)
        need(std::abs(s.l) < 1e-6 && std::abs(s.r) < 1e-6, "All-channel VGM mute");
    core->muteMask(0);
    core->seek(0);
    core->render(4096, b);
    need(difference(initial, b.mix) < 1e-6, "Unmute/rewind did not restore PCM");
    // Hidden dummy output: validates actual app worker/output timing, not listening.
    Player player;
    player.open(path);
    player.play();
    std::this_thread::sleep_for(std::chrono::milliseconds(600));
    player.pause();
    need(player.seconds() > .25 && player.error().empty(), "App worker did not play VGM");
    std::cout << "App dummy playback seconds=" << player.seconds()
              << " underruns=" << player.underruns() << "; replay/seek/mute PASS\n";
    need(readMusicFile(path) == bytes, "Original VGM changed");
}
int wmain(int argc, wchar_t **argv) {
    try {
        structural();
        licensedEsFixture();
        ownDacStream();
        if (argc > 1) {
            std::filesystem::path out = argv[1];
            std::filesystem::create_directories(out);
            for (int i = 2; i < argc; ++i)
                actual(argv[i], out);
        }
        std::cout << "PASS extended VGM\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
}
