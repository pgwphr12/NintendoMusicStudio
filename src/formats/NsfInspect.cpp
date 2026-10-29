// SPDX-License-Identifier: LGPL-2.1-or-later
// Own read-only wrapper, compiled inside the LGPL GME library.
#include "Music_Emu.h"
#include "Nsf_Emu.h"
#include "gme.h"
#include "Multi_Buffer.h"
// Only explicitly requested by the offline note importer, before restarting
// its private emulator. Normal playback retains the upstream 50ms buffer.
blargg_err_t Classic_Emu::ngmv_inspection_buffer(){auto error=buf->set_sample_rate(sample_rate(),5);if(!error)buf->clock_rate(clock_rate_);return error;}
extern "C" __declspec(dllexport) int ngmv_nsf_inspection_mode(Music_Emu*emu){
 if(!emu||(emu->type()!=gme_nsf_type&&emu->type()!=gme_nsfe_type))return 0;
 return static_cast<Nsf_Emu*>(emu)->ngmv_inspection_buffer()?0:1;
}
extern "C" __declspec(dllexport) int ngmv_nsf_inspect(Music_Emu* emu,int* state,int size){
 if(!emu||!state||size<40)return 0;
 if(emu->type()!=gme_nsf_type&&emu->type()!=gme_nsfe_type)return 0;
 auto* n=static_cast<Nsf_Emu*>(emu);n->apu_()->ngmv_inspect_state(state);
 state[38]=(n->header().speed_flags&3)==1;state[39]=n->header().chip_flags;return 1;
}
