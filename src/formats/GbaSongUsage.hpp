// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <cstdint>
#include <set>
#include <vector>
#include <utility>
#include <tuple>
namespace ngmv {
// Conservative control-flow union for standard MP2K. Follows pattern calls and
// returns with their running status, both repeat edges, and every declared track. Unsupported
// commands/tones fail closed (all PSG visible), never use an intro audio sample.
template<class Read> bool gbaSongPsgUsage(Read read,uint32_t header,uint32_t&out,unsigned*failure=nullptr){
 auto fail=[&](unsigned line){if(failure)*failure=line;return false;};
 auto h=read(header,8);if(!h||!h[0]||h[0]>16||h[1])return fail(__LINE__);
 auto word=[](const uint8_t*p){return uint32_t(p[0])|uint32_t(p[1])<<8|uint32_t(p[2])<<16|uint32_t(p[3])<<24;};
 unsigned tracks=h[0];uint32_t bank=word(h+4);std::set<unsigned>voices;
 using State=std::tuple<uint32_t,unsigned,unsigned,std::vector<uint32_t>>;
 std::vector<State> todo;std::set<State>seen;
 for(unsigned i=0;i<tracks;++i){auto p=read(header+8+i*4,4);if(!p)return fail(__LINE__);todo.emplace_back(word(p),0,0,std::vector<uint32_t>{});}
 while(!todo.empty()){
  auto state=todo.back();todo.pop_back();if(!seen.insert(state).second)continue;if(seen.size()>262144)return fail(__LINE__);
  uint32_t pc=std::get<0>(state);unsigned running=std::get<1>(state);unsigned voice=std::get<2>(state);auto stack=std::get<3>(state);auto p=read(pc,1);if(!p)return fail(__LINE__);
  unsigned cmd=*p;if(cmd>=128){++pc;if(cmd>=0xbd)running=cmd;}else cmd=running;
  if(cmd<128)return fail(__LINE__);
  auto byte=[&]()->int{auto b=read(pc,1);if(!b)return -1;++pc;return *b;};
  auto target=[&]()->uint32_t{auto b=read(pc,4);if(!b)return 0;pc+=4;return word(b);};
  if(cmd<=0xb0){}
  else if(cmd==0xb1)continue;
  else if(cmd==0xb4){if(!stack.empty()){pc=stack.back();stack.pop_back();}}
  else if(cmd==0xb2){uint32_t dest=target();if(!dest)return fail(__LINE__);todo.emplace_back(dest,running,voice,stack);continue;}
  else if(cmd==0xb3){uint32_t dest=target();if(!dest||stack.size()>=3)return fail(__LINE__);stack.push_back(pc);pc=dest;}
  else if(cmd==0xb5){if(byte()<0)return fail(__LINE__);uint32_t dest=target();if(!dest)return fail(__LINE__);todo.emplace_back(dest,running,voice,stack);}
  else if((cmd>=0xba&&cmd<=0xc5)||cmd==0xc8){int v=byte();if(v<0)return fail(__LINE__);if(cmd==0xbd){voice=unsigned(v);voices.insert(voice);}}
  else if(cmd>=0xce){if(cmd>=0xcf)voices.insert(voice);unsigned limit=cmd==0xce?1:cmd==0xcf?2:3;for(unsigned i=0;i<limit;++i){auto b=read(pc,1);if(!b)return fail(__LINE__);if(*b>=128)break;++pc;}}
  else return fail(__LINE__); // MEMACC/XCMD and nonstandard engines are not guessed.
  todo.emplace_back(pc,running,voice,stack);
 }
 uint32_t mask=0;
 std::set<uint32_t>tones;size_t toneBudget=0;
 auto inspectTone=[&](auto&&self,uint32_t address,unsigned depth)->bool{
  if(++toneBudget>8192||depth>1)return false;if(tones.count(address))return true;
  auto t=read(address,12);if(!t)return false;unsigned type=t[0];
  if(depth==1&&(type&0xc0))return true; // MP2K rejects a nested split/rhythm note.
  if(type==0x40||type==0x80){
   uint32_t child=word(t+4);std::set<unsigned>keys;
   if(type==0x40){auto mapping=read(word(t+8),128);if(!mapping)return false;for(unsigned k=0;k<128;++k)keys.insert(mapping[k]);}
   else for(unsigned k=0;k<128;++k)keys.insert(k);
   for(auto k:keys)if(!self(self,child+k*12,depth+1))return false;
   tones.insert(address);return true;
  }
  if(type&~15u)return false;unsigned c=type&7;if(c>4)return false;if(c)mask|=1u<<(c-1);tones.insert(address);return true;
 };
 for(unsigned voice:voices)if(!inspectTone(inspectTone,bank+voice*12,0))return fail(__LINE__);
 out=mask;return true;
}
}
