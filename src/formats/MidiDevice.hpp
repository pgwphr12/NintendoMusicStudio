#pragma once
#include "Midi.hpp"
#include <memory>
namespace ngmv {
// Transport clock is consumed audio frames, never render/prefetch time.
struct MidiPort {
 virtual ~MidiPort()=default;
 virtual unsigned preferred()=0;
 virtual void open()=0;
 virtual void close()=0;
 virtual void shortMessage(uint32_t)=0;
 virtual void longMessage(const std::vector<uint8_t>&)=0;
};
std::unique_ptr<MidiPort> windowsMidiPort();
class MidiDevice {
 const MidiSong& song_;std::unique_ptr<MidiPort> port_;
 bool opened_=false,playing_=false;unsigned preferred_=~0u;uint64_t last_=0;size_t next_=0;
 std::vector<ChannelState> states_;float master_=1;
 void silence();void chase(uint64_t);void send(uint32_t);
 public:
 MidiDevice(const MidiSong&s,std::unique_ptr<MidiPort> p):song_(s),port_(std::move(p)){}
 ~MidiDevice();
 void update(uint64_t frame,bool playing,const std::vector<ChannelState>& states,float master);
};
void midiDeviceOutput(bool);
void midiSelectOutput(unsigned choice);
bool midiDeviceOutput();
}
