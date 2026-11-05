#include "player/Player.hpp"
#include <iostream>
#include <stdexcept>
using namespace ngmv;
static void check(bool b,const char*s){if(!b)throw std::runtime_error(s);}
static unsigned count(const Player&p){unsigned n=0;for(size_t c=0;c<p.channels().size();c++)n+=p.channelVisible(c);return n;}
int wmain(int argc,wchar_t**argv){try{
    auto root=std::filesystem::path(argv[1]);Player p;
    p.open(root/"channels.nsfe");check(count(p)==4,"Basic NES channels must stay, unused tagged DMC must hide");
    p.open(root/"dmc-late.nsfe");check(p.channelVisible(4)&&!p.channelUsed(4),"Later DMC must be visible BEFORE its first note");
    p.seek(200);check(count(p)==5,"Seeking changed song-wide channel list");auto d=p.states()[4];d.mute=true;d.volume=0;p.channel(4,d);p.stop();check(count(p)==5,"Mute/volume/rewind hid a used channel");
    p.open(root/"channels.nsfe");check(!p.channelVisible(4),"Previous song's used channel leaked");p.select(1);check(count(p)==4,"Subsong usage did not reset");
    p.open(root/"channels.nsf");check(count(p)==p.channels().size(),"Unknown-length NSF was guessed unused");
    p.open(root/"chips/es-late.vgm");check(p.channels().size()==40&&count(p)==8,"ES whole-program mask/base YM lanes incorrect");check(p.channelVisible(38)&&!p.channelUsed(38),"Later ES voice must appear at load, not first audio");
    check(!p.channelVisible(37)&&!p.channelVisible(39),"Unused ES voice/residual visible");auto before=count(p);p.seek(1200);check(count(p)==before,"ES list changed during seek");
    p.open(root/"chips/es-many.vgm");check(count(p)==32&&!p.channelVisible(32),"ES32 voices/residual declaration mismatch");
    p.open(root/"chips/es-direct-bus.vgm");check(count(p)==1&&p.channelVisible(32),"Direct ES bus usage lost");
    p.open(root/"chips/unused-opl.vgm");check(count(p)==8&&!p.channelVisible(7),"Unused auxiliary OPL did not hide");
    p.open(root/"chips/ay-own.vgm");check(count(p)==3,"AY basic channels should remain fixed");
    for(auto name:{"channels.gbs","channels.spc","channels.minigsf","channels.mini2sf","channels.miniusf","channels.bcstm","channels.bcwav","midi/channels.mid","audio/tone.wav"}){
        p.open(root/name);check(count(p)==p.channels().size(),"A basic/GBA PCM/declared MIDI channel was hidden");std::cout<<name<<" visible="<<count(p)<<" PASS\n";
    }
    if(argc>2){p.open(std::filesystem::path(argv[2]));check(count(p)==17,"Actual lobby usage must be known as17lanes before playback");std::cout<<"Actual lobby at open: ";for(size_t i=0;i<p.channels().size();i++)if(p.channelVisible(i))std::cout<<p.channels()[i].name<<"; ";std::cout<<'\n';}
    std::cout<<"Whole-song pre-play late notes, stable pause/seek/mute/rewind, new song/subsong, basic lanes and GBA PCM: PASS\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
