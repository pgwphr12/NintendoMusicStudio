#include "MidiDevice.hpp"
#include <windows.h>
#include <mmsystem.h>
#include <mmddk.h>
#include <algorithm>
#include <atomic>
#include <array>
#include <list>
#include <stdexcept>
namespace ngmv {
namespace {
std::atomic<bool> deviceDefault{false};
void check(MMRESULT r){if(r){char text[256]{};midiOutGetErrorTextA(r,text,256);throw std::runtime_error(std::string("Windows MIDI output: ")+text);}}
class WinPort final:public MidiPort {
 HMIDIOUT handle_=nullptr;
 struct Pending {MIDIHDR header{};std::vector<uint8_t> bytes;};std::list<Pending> pending_;
 void reap(){for(auto i=pending_.begin();i!=pending_.end();){if(i->header.dwFlags&MHDR_DONE){check(midiOutUnprepareHeader(handle_,&i->header,sizeof(MIDIHDR)));i=pending_.erase(i);}else ++i;}}
 public:
 ~WinPort(){try{close();}catch(...){}}
 unsigned preferred()override{midiOutGetNumDevs();DWORD id=UINT_MAX,flags=0;auto r=midiOutMessage(reinterpret_cast<HMIDIOUT>(UINT_PTR(MIDI_MAPPER)),DRVM_MAPPER_PREFERRED_GET,reinterpret_cast<DWORD_PTR>(&id),reinterpret_cast<DWORD_PTR>(&flags));return r==MMSYSERR_NOERROR?id:UINT_MAX;}
 void open()override{check(midiOutOpen(&handle_,MIDI_MAPPER,0,0,CALLBACK_NULL));}
 void close()override{if(!handle_)return;auto reset=midiOutReset(handle_);if(reset){/* A failed reset leaves driver ownership uncertain: retain buffers rather than free in-flight SysEx. */auto* retained=new std::list<Pending>();retained->splice(retained->end(),pending_);handle_=nullptr;check(reset);}for(auto&i:pending_)check(midiOutUnprepareHeader(handle_,&i.header,sizeof(MIDIHDR)));pending_.clear();auto old=handle_;handle_=nullptr;check(midiOutClose(old));}
 void shortMessage(uint32_t msg)override{check(midiOutShortMsg(handle_,msg));reap();}
 void longMessage(const std::vector<uint8_t>&bytes)override{
  reap();if(pending_.size()>=64)throw std::runtime_error("MIDI output SysEx queue is full.");pending_.emplace_back();auto&i=pending_.back();i.bytes=bytes;i.header.lpData=reinterpret_cast<char*>(i.bytes.data());i.header.dwBufferLength=DWORD(i.bytes.size());
  auto r=midiOutPrepareHeader(handle_,&i.header,sizeof(MIDIHDR));if(r){pending_.pop_back();check(r);}r=midiOutLongMsg(handle_,&i.header,sizeof(MIDIHDR));if(r){midiOutUnprepareHeader(handle_,&i.header,sizeof(MIDIHDR));pending_.pop_back();check(r);}
 }
};
}
void midiDeviceOutput(bool b){deviceDefault=b;}
bool midiDeviceOutput(){return deviceDefault;}
void midiSelectOutput(unsigned choice){if(choice<1||choice>4)throw std::runtime_error("Invalid MIDI output mode.");midiDeviceOutput(choice==4); }
std::unique_ptr<MidiPort> windowsMidiPort(){return std::make_unique<WinPort>();}
MidiDevice::~MidiDevice(){try{silence();}catch(...){}try{port_->close();}catch(...){}}
void MidiDevice::silence(){if(opened_)for(unsigned c=0;c<16;c++){port_->shortMessage(0x40b0u+c);port_->shortMessage(0x78b0u+c);}}
void MidiDevice::send(uint32_t m){
 if(m==0xff){port_->longMessage({0xf0,0x7e,0x7f,9,1,0xf7});for(unsigned c=0;c<16;c++)if(song_.used&(1<<c))send(0x6407b0u+c);return;}
 if(m&0x1000000){auto i=m&0xffffff;if(i>=song_.sysex.size())throw std::runtime_error("Invalid MIDI SysEx index.");port_->longMessage(song_.sysex[i]);return;}
 unsigned c=m&15,slot=0;for(unsigned i=0;i<c;i++)slot+=bool(song_.used&(1<<i));if(slot>=states_.size())return;
 auto state=states_[slot];if((m&0xf0)==0x90&&state.mute)return;
 if((m&0xf0)==0xb0&&((m>>8)&127)==7){unsigned v=unsigned(std::clamp(float((m>>16)&127)*master_*state.volume*(state.mute?0.f:1.f),0.f,127.f));m=(m&0xffff)|(v<<16);}
 port_->shortMessage(m);if((m&0xffff)==(0x79b0u+c))send(0x6407b0u+c);
}
void MidiDevice::chase(uint64_t frame){
 silence();std::array<std::array<int,128>,16>cc{},notes{};std::array<std::array<bool,128>,16>held{};std::array<int,16>program{},bend{};
 for(auto&c:cc)c.fill(-1);program.fill(-1);bend.fill(-1);next_=0;
 for(;next_<song_.events.size()&&song_.events[next_].frame<frame;next_++){
  auto m=song_.events[next_].message;if(m==0xff){send(m);for(auto&c:cc)c.fill(-1);for(auto&n:notes)n.fill(0);for(auto&h:held)h.fill(false);program.fill(-1);bend.fill(-1);continue;}
  if(m&0x1000000){send(m);continue;}unsigned c=m&15,a=(m>>8)&127,b=(m>>16)&127;
  switch(m&0xf0){case 0x90:if(b){notes[c][a]=b;held[c][a]=true;break;}[[fallthrough]];
   case 0x80:held[c][a]=false;if(cc[c][64]<64)notes[c][a]=0;break;
   case 0xc0:program[c]=a;break;case 0xe0:bend[c]=a|(b<<7);break;
   case 0xb0:cc[c][a]=b;if(a==64&&b<64)for(unsigned n=0;n<128;n++)if(!held[c][n])notes[c][n]=0;if(a==120||a==123){notes[c].fill(0);held[c].fill(false);}if(a==121){cc[c].fill(-1);cc[c][64]=0;for(unsigned n=0;n<128;n++)if(!held[c][n])notes[c][n]=0;}break;
  }
 }
 for(unsigned c=0;c<16;c++)if(song_.used&(1<<c)){
  port_->shortMessage(0x79b0u+c);for(unsigned a:{0u,32u})if(cc[c][a]>=0)send(0xb0+c+(a<<8)+(cc[c][a]<<16));
  if(program[c]>=0)send(0xc0+c+(program[c]<<8));
  for(unsigned a=1;a<120;a++)if(a!=32&&a!=7&&cc[c][a]>=0)send(0xb0+c+(a<<8)+(cc[c][a]<<16));
  send(0x7b0+c+((cc[c][7]<0?100:cc[c][7])<<16));if(bend[c]>=0)send(0xe0+c+((bend[c]&127)<<8)+((bend[c]>>7)<<16));
  for(unsigned n=0;n<128;n++)if(notes[c][n])send(0x90+c+(n<<8)+(notes[c][n]<<16));
 }
}
void MidiDevice::update(uint64_t frame,bool playing,const std::vector<ChannelState>&states,float master){
 if(!playing){if(playing_)silence();playing_=false;return;}
 unsigned preferred=port_->preferred();bool changed=states.size()!=states_.size()||master!=master_;if(!changed)for(size_t i=0;i<states.size();i++)if(states[i].mute!=states_[i].mute||states[i].volume!=states_[i].volume)changed=true;states_=states;master_=master;
 if(opened_&&preferred!=preferred_){silence();port_->close();opened_=false;}
 if(!opened_){port_->open();opened_=true;preferred_=preferred;changed=true;}
 if(!playing_||frame<last_||changed)chase(frame);
 while(next_<song_.events.size()&&song_.events[next_].frame<=frame)send(song_.events[next_++].message);
 last_=frame;playing_=true;
}
}
