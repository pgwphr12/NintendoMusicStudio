// Own bounded read-only local driver diagnostics; output is private, not packaged.
#define makePsfBackend makePsfBackendDriverProbe
#include "../src/formats/PsfBackend.cpp"
#include "formats/NintendoMixerTap.hpp"
#include <unordered_map>
#include <fstream>
#include <iostream>
struct DriverTrace {
 struct Entry {uint64_t count=0;uint32_t low=0xffffffff,high=0;int width=0;std::set<std::pair<int,uint32_t>>pointers;std::array<unsigned,32>ptrCount{};};
 std::map<std::pair<uint32_t,bool>,Entry> pcs;uint64_t writes=0;
 static u32 callback(void*p,GBASystem*g,u32 address,u32 value,int width){auto&t=*static_cast<DriverTrace*>(p);uint32_t a=ngmv::GbaMixerTap::word(g->ioMem+0xbc),b=ngmv::GbaMixerTap::word(g->ioMem+0xc8);auto nearby=[&](uint32_t x){unsigned region=x>>24;return (region==2||region==3)&&uint64_t(address)+8192>=x&&uint64_t(address)<=uint64_t(x)+8192;};
  if(!nearby(a)&&!nearby(b))return value;t.writes++;auto key=std::make_pair(g->reg[15].I,g->armState);if(t.pcs.size()>=4096&&!t.pcs.count(key))return value;auto&e=t.pcs[key];e.count++;e.low=std::min(e.low,address);e.high=std::max(e.high,address);e.width|=width;
  for(int r=0;r<15;r++){auto v=g->reg[r].I;unsigned region=v>>24;if((region==2||region==3||(r==14&&region>=8&&region<=13))&&e.ptrCount[r]<16){if(e.pointers.emplace(r,v).second)e.ptrCount[r]++;}}
  auto stack=ngmv::GbaMixerTap::ram(g,g->reg[13].I,64);if(stack)for(int i=0;i<16;i++){auto v=ngmv::GbaMixerTap::word(stack+i*4);unsigned region=v>>24;int r=16+i;if((region==2||region==3)&&e.ptrCount[r]<16){if(e.pointers.emplace(r,v).second)e.ptrCount[r]++;}}
  return value;
 }
};
int wmain(int argc,wchar_t**argv){try{if(argc!=3)throw std::runtime_error("source-GSF private-output-prefix");std::filesystem::path path=argv[1],out=argv[2];std::filesystem::create_directories(out.parent_path());ngmv::Image image;image.format=ngmv::Format::Gsf;image.load(path);image.finish();auto pools=ngmv::findNintendoVoicePools(image.rom);for(auto pool:pools)std::cout<<"pool="<<std::hex<<pool.base<<" count="<<std::dec<<pool.count<<'\n';ngmv::GbaCore core(image,0x30f);DriverTrace trace;core.system.viewerMixerContext=&trace;core.system.viewerMixerWrite=&DriverTrace::callback;std::vector<ngmv::Stereo>audio;for(int i=0;i<563;i++)core.render(512,audio);
 std::vector<std::pair<std::pair<uint32_t,bool>,DriverTrace::Entry>>entries(trace.pcs.begin(),trace.pcs.end());std::sort(entries.begin(),entries.end(),[](auto&a,auto&b){return a.second.count>b.second.count;});std::ofstream log(out.string()+".txt");log<<"writes="<<trace.writes<<" PCs="<<entries.size()<<" DMA_A="<<std::hex<<ngmv::GbaMixerTap::word(core.system.ioMem+0xbc)<<" DMA_B="<<ngmv::GbaMixerTap::word(core.system.ioMem+0xc8)<<'\n';
 for(size_t i=0;i<std::min(size_t(24),entries.size());i++){auto&k=entries[i].first;auto&e=entries[i].second;log<<"pc="<<std::hex<<k.first<<" arm="<<k.second<<" count="<<std::dec<<e.count<<" dest="<<std::hex<<e.low<<".."<<e.high<<" widths="<<e.width<<" ptr=";for(auto rp:e.pointers)log<<std::dec<<rp.first<<":"<<std::hex<<rp.second<<",";log<<'\n';uint32_t begin=(k.first&~3u)-32;auto code=ngmv::GbaMixerTap::ram(&core.system,begin,96);if(!code&&(begin>>24)>=8&&(begin>>24)<=13&&(begin&0x1ffffffu)+96<=core.system.romSize)code=core.system.rom+(begin&0x1ffffffu);if(code){std::ofstream raw(out.string()+"-code-"+std::to_string(i)+".bin",std::ios::binary);raw.write(reinterpret_cast<const char*>(code),96);}}
 std::set<uint32_t>callers;for(auto&entry:entries)for(auto rp:entry.second.pointers)if(rp.first==14&&(rp.second>>24)>=8&&(rp.second>>24)<=13)if(callers.size()<24)callers.insert(rp.second);
 for(auto lr:callers){uint32_t address=(lr&~1u)-256,offset=address&0x1ffffffu;if(offset+512<=core.system.romSize){std::ofstream raw(out.string()+"-caller-"+std::to_string(lr)+".bin",std::ios::binary);raw.write(reinterpret_cast<const char*>(core.system.rom+offset),512);}}
 std::cout<<"PASS bounded RAM-write metadata PCs="<<entries.size()<<" writes="<<trace.writes<<" private code windows only; no sample values/ROM dump/source changes\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
