# SPDX-License-Identifier: LGPL-2.1-or-later
# Add read-only inspection to the built LGPL NES APU; source archive unchanged.
set(path "${GME_SOURCE_DIR}/gme/Nes_Apu.h")
file(READ "${path}" content)
if(NOT content MATCHES "ngmv_inspect_state")
 set(anchor "\tNes_Apu();")
 string(FIND "${content}" "${anchor}" found)
 if(found LESS 0)
  message(FATAL_ERROR "NES APU inspection anchor missing")
 endif()
 set(method [=[
 // NGMV read-only, caller owns 38 integers. No emulation state is changed.
 void ngmv_inspect_state(int* p) const {
  for(int i=0;i<5;i++)for(int j=0;j<4;j++)p[i*4+j]=oscs[i]->regs[j];
  p[20]=osc_enables;
  for(int i=0;i<5;i++){p[21+i]=oscs[i]->length_counter;p[31+i]=oscs[i]->period();}
  p[26]=square1.volume();p[27]=square2.volume();p[28]=15;p[29]=noise.volume();p[30]=0;
  p[36]=triangle.linear_counter;p[37]=1;
 }
]=])
 string(REPLACE "${anchor}" "${method}\n${anchor}" content "${content}")
 file(WRITE "${path}" "${content}")
endif()
# State30 is a read-only DMC activity flag (previously reserved zero).
file(READ "${path}" content)
set(previous "${content}")
string(REPLACE "p[30]=0;" "p[30]=dmc.silence?0:1;" content "${content}")
if(NOT previous STREQUAL content)
 file(WRITE "${path}" "${content}")
endif()
set(classic "${GME_SOURCE_DIR}/gme/Classic_Emu.h")
file(READ "${classic}" content)
if(NOT content MATCHES "ngmv_inspection_buffer")
 string(REPLACE "void set_buffer( Multi_Buffer* ) override;" "void set_buffer( Multi_Buffer* ) override;\n blargg_err_t ngmv_inspection_buffer();" content "${content}")
 file(WRITE "${classic}" "${content}")
endif()
