#include "core/IMusicBackend.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace ngmv;
static void check(bool b,const char*m){if(!b)throw std::runtime_error(m);}
static double energy(const std::vector<Stereo>&v){double result=0;for(auto x:v){check(std::isfinite(x.l)&&std::isfinite(x.r),"Nonfinite chip output");result+=x.l*x.l+x.r*x.r;}return result;}
int main(int argc,char**argv){try{
    auto root=std::filesystem::u8path(argv[1]);AudioBlock b;
    for(auto name:{"es-many.vgm","es-high-rate.vgm","es-exact-rate.vgm"}){
        auto core=makeBackend(readMusicFile(root/name));core->render(8192,b);check(core->channels().size()==33,"ES32 voices + residual/other chip count");
        check(energy(b.mix)>1,"ES fixture silent");double error=0;unsigned active=0;for(size_t i=0;i<b.mix.size();i++){Stereo sum{};for(const auto&v:b.voices){sum.l+=v[i].l;sum.r+=v[i].r;}error=std::max(error,double(std::max(std::abs(sum.l-b.mix[i].l),std::abs(sum.r-b.mix[i].r))));}
        for(size_t c=0;c<32;c++)if(energy(b.voices[c])>0)++active;check(active>=5,"ES voices not separated");std::cout<<name<<" diagnostic sum_error="<<error<<"\n";check(error<.00002,"ES voice frame/precision/routing sum mismatch");
        auto first=b.mix;core->selectTrack(0);core->render(8192,b);for(size_t i=0;i<first.size();i++)check(first[i].l==b.mix[i].l&&first[i].r==b.mix[i].r,"ES replay changed mix");
        std::cout<<name<<" active="<<active<<" sum_error="<<error<<" PASS\n";
    }
    auto last=makeBackend(readMusicFile(root/"es-last.vgm"));last->render(4096,b);check(energy(b.voices[38])>1,"ES voice32 is missing above bit31");
    last->muteMask64((uint64_t(1)<<38)|127);last->seek(0);last->render(4096,b);check(energy(b.mix)==0&&energy(b.voices[38])>1,"64-bit ES voice32 mute failed or wave disappeared");
    last->muteMask64(0);last->seek(0);last->render(4096,b);check(energy(b.mix)>1,"ES high-index unmute failed");
    auto bus=makeBackend(readMusicFile(root/"es-direct-bus.vgm"));bus->render(512,b);check(energy(b.mix)>0,"Direct ES bus output lost");
    for(unsigned c=0;c<32;c++)check(energy(b.voices[c])==0,"Direct bus was invented as a voice");
    for(size_t i=0;i<b.mix.size();i++)check(std::abs(b.mix[i].l-b.voices[32][i].l)<1e-7&&std::abs(b.mix[i].r-b.voices[32][i].r)<1e-7,"ES direct bus residual mismatch");
    bus->muteMask64(uint64_t(1)<<32);bus->seek(0);bus->render(512,b);check(energy(b.mix)==0,"ES residual high-bit mute failed");
    for(auto pair:{std::pair<const char*,double>{"ay-own.vgm",2500},{"ay-classic.vgm",1250},{"ay-half.vgm",1250}}){
        auto core=makeBackend(readMusicFile(root/pair.first));check(core->channels().size()==3,"AY8930 three outputs lost");unsigned rising=0;float prev=0;double total=0,error=0;
        for(unsigned n=0;n<12;n++){core->render(2000,b);total+=energy(b.mix);for(size_t i=0;i<b.mix.size();i++){auto x=b.mix[i];if(prev<=0&&x.l>0)++rising;prev=x.l;Stereo sum{};for(const auto&v:b.voices){sum.l+=v[i].l;sum.r+=v[i].r;}error=std::max(error,double(std::abs(sum.l-x.l)));}}
        check(total>1&&std::abs(rising*2.-pair.second)<12,"AY8930 native clock/mode/half-clock pitch mismatch");check(error<1e-6,"AY channel sum mismatch");
        core->muteMask(7);core->seek(0);core->render(4096,b);check(energy(b.mix)==0,"AY all mute failed");std::cout<<pair.first<<" frequency="<<rising*2.<<" expected="<<pair.second<<" PASS\n";
    }
    auto a=makeBackend(readMusicFile(root/"ay-own.vgm")),n=makeBackend(readMusicFile(root/"ay-narrow.vgm"));AudioBlock c;a->render(8192,b);n->render(8192,c);double diff=0;for(size_t i=0;i<b.mix.size();i++)diff+=std::abs(b.mix[i].l-c.mix[i].l);check(diff>10,"AY8930 expanded duty ignored");
    auto bad=readMusicFile(root/"ay-own.vgm");bad[0x78]=0;bool rejected=false;try{makeBackend(bad);}catch(const std::exception&){rejected=true;}check(rejected,"AY8910 mislabelled as AY8930");
    std::cout<<"ES voice timing/routing/precision/64bit mute and AY8930 clock/duty/three lanes: PASS\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
