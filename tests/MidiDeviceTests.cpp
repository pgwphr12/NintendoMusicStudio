#include "formats/MidiDevice.hpp"
#include <stdexcept>
#include <iostream>
using namespace ngmv;
struct Fake final:MidiPort{unsigned id=1,opens=0,closes=0;std::vector<uint32_t> messages;std::vector<std::vector<uint8_t>> longs;
 unsigned preferred()override{return id;}void open()override{opens++;}void close()override{closes++;}void shortMessage(uint32_t m)override{messages.push_back(m);}void longMessage(const std::vector<uint8_t>&b)override{longs.push_back(b);}};
void check(bool b,const char*m){if(!b)throw std::runtime_error(m);}
int main(){try{MidiSong song;song.used=1;song.events={{0,0x28c0},{0,0x643c90},{4800,0x3c80},{9600,0x644090},{14400,0x4080}};song.endFrame=24000;
 auto sink=std::make_unique<Fake>();auto*p=sink.get();MidiDevice out(song,std::move(sink));std::vector<ChannelState>states(1);
 out.update(0,false,states,1);check(!p->opens,"Paused prefetch opened output");out.update(0,true,states,1);check(p->opens==1&&p->messages.back()==0x643c90,"Initial program/note lost");
 size_t n=p->messages.size();out.update(4799,true,states,1);check(p->messages.size()==n,"Future event sent by prefetch");out.update(4800,true,states,1);check(p->messages.back()==0x3c80,"Note off scheduled late");
 out.update(10000,true,states,1);check(p->messages.back()==0x644090,"Second note missing");out.update(10000,false,states,1);check(p->messages.back()==0x78bf,"Pause left stuck notes");out.update(11000,true,states,1);check(p->messages.back()==0x644090,"Resume lost held note");
 p->id=2;out.update(11500,true,states,1);check(p->opens==2&&p->closes==1&&p->messages.back()==0x644090,"Mapper change failed to reopen/chase");
 states[0].mute=true;out.update(11600,true,states,1);check((p->messages.back()&0xffff)==0x7b0&&((p->messages.back()>>16)&127)==0,"Mute not applied through CC7");
 states[0].mute=false;out.update(100,true,states,.5f);check(p->messages.back()==0x643c90,"Seek failed to restore active note");bool gain=false;for(auto m:p->messages)if(m==0x3207b0)gain=true;check(gain,"Master gain missing");
 out.update(15000,true,states,.5f);check(p->messages.back()==0x4080,"End note off missing");
 MidiSong reset;reset.used=1;reset.events={{0,0xff},{100,0x643c90}};auto rp=std::make_unique<Fake>();auto* r=rp.get();MidiDevice resetOut(reset,std::move(rp));resetOut.update(0,true,states,.5f);check(r->longs.size()==1&&r->messages.back()==0x3207b0,"GM reset bypassed master volume");
 std::cout<<"Owned fake MIDI port: no hardware notes; consumed-clock timing, prefetch, pause/resume, seek, mapper switch, mute/gain PASS\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
