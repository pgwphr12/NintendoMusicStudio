// SPDX-License-Identifier: GPL-2.0-or-later
#include "core/IMusicBackend.hpp"
#include "player/Player.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <thread>
using namespace ngmv;
static void need(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
static void equal(const AudioBlock& a, const AudioBlock& b) {
    need(a.mix.size()==b.mix.size(), "Different block sizes");
    for(size_t i=0;i<a.mix.size();++i)
        need(a.mix[i].l==b.mix[i].l && a.mix[i].r==b.mix[i].r,"N64 reset/seek is not deterministic");
}
int wmain(int argc, wchar_t** argv) {
    try {
        need(argc==2,"N64 test file required");
        std::filesystem::path file=argv[1];
        auto backend=makeBackend(readMusicFile(file),file);
        AudioBlock first,replay,atSeek,atRender,muted;
        backend->render(2048,first);
        double energy=0;for(auto v:first.mix) energy+=v.l*v.l+v.r*v.r;
        need(energy>1e-5,"N64 replacement produced silence");
        backend->selectTrack(0);backend->render(2048,replay);equal(first,replay);
        backend->seek(500);backend->render(512,atSeek);
        backend->selectTrack(0);
        for(size_t left=24000;left;){auto count=std::min(left,size_t(512));backend->render(count,replay);left-=count;}
        backend->render(512,atRender);equal(atSeek,atRender);
        double maxError=0;
        for(size_t i=0;i<atRender.mix.size();++i){
            Stereo sum{};for(auto& voice:atRender.voices){sum.l+=voice[i].l;sum.r+=voice[i].r;}
            maxError=std::max({maxError,double(std::abs(sum.l-atRender.mix[i].l)),double(std::abs(sum.r-atRender.mix[i].r))});
        }
        need(maxError<.001,"N64 separated voices do not reconstruct output");
        need(backend->channels().size()<32,"Channel mask overflow");
        backend->muteMask((1u<<backend->channels().size())-1);backend->render(512,muted);
        for(auto v:muted.mix)need(v.l==0&&v.r==0,"N64 all-mute failed");
        backend->muteMask(0);backend->seek(500);backend->render(512,replay);equal(atSeek,replay);
        Player player;player.open(file);player.play();
        std::this_thread::sleep_for(std::chrono::milliseconds(350));player.pause();
        need(player.seconds()>.15 && player.error().empty() && player.underruns()==0,"N64 dummy device playback failed");
        player.stop();player.play();std::this_thread::sleep_for(std::chrono::milliseconds(250));player.pause();
        need(player.seconds()>.1 && player.error().empty(),"N64 app replay failed");
        std::cout<<"N64 exact reset/seek, voice recombination, mute/unmute, dummy worker/replay PASS; lanes="<<backend->channels().size()<<" maxError="<<maxError<<'\n';
        return 0;
    } catch(const std::exception& e) {std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
