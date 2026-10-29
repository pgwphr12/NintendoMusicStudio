#include "formats/GbaRomSample.hpp"
#include "core/IMusicBackend.hpp"
#include <array>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <stdexcept>
using namespace ngmv;
static void require(bool b,const char*s){if(!b)throw std::runtime_error(s);}
static void unit(){
 std::array<uint8_t,128>rom{};
 auto put=[&](int o,uint32_t v,int n=4){for(int j=0;j<n;j++)rom[o+j]=uint8_t(v>>(j*8));};
 put(32,0x40000000);put(36,12345678);put(40,2);put(44,8);
 const std::array<uint8_t,8>pcm={0,127,128,255,1,254,64,192};std::copy(pcm.begin(),pcm.end(),rom.begin()+48);
 GbaSampleInfo info;info.address=0x08000020;info.status=0x4000;info.rawFrequency=12345678;info.loopStart=2;info.length=8;info.looped=info.readable=info.pcm8=true;
 GbaSampleData copy;std::string error="old";auto original=rom;
 require(copyGbaRomSample(rom.data(),rom.size(),info,copy,error)&&error.empty(),"Own ROM sample copy failed");
 require(copy.pcm.size()==8&&copy.pcm[0]==0&&copy.pcm[1]==127&&copy.pcm[2]==-128&&copy.pcm[3]==-1&&copy.pcm[7]==-64,"Signed PCM or data offset incorrect");
 require(copy.info.rawFrequency==12345678&&copy.info.loopStart==2&&copy.info.looped&&rom==original,"Metadata or source modified");
 rom[48]=99;require(copy.pcm[0]==0,"Sample copy aliases source memory");rom=original;
 int checks=0;
 auto reject=[&](GbaSampleInfo bad,size_t size=128){GbaSampleData out;out.info.address=99;out.pcm={-3,4};error="";require(!copyGbaRomSample(rom.data(),size,bad,out,error)&&!error.empty()&&out.info.address==99&&out.pcm==std::vector<int8_t>({-3,4}),"Failure altered caller sample or succeeded");checks++;};
 for(uint32_t address:{0u,0x02000020u,0x03000020u,0x07000020u,0x0e000020u,0xffffffffu}){auto bad=info;bad.address=address;reject(bad);}
 for(size_t size:{size_t(0),size_t(31),size_t(32),size_t(47),size_t(48),size_t(55)})reject(info,size);
 auto bad=info;bad.length++;reject(bad);bad=info;bad.rawFrequency++;reject(bad);bad=info;bad.status=0;reject(bad);bad=info;bad.loopStart=3;reject(bad);bad=info;bad.type=1;reject(bad);bad=info;bad.looped=false;reject(bad);bad=info;bad.readable=false;reject(bad);bad=info;bad.pcm8=false;reject(bad);
 for(int region:{8,10,12}){auto mirror=info;mirror.address=uint32_t(region<<24)|0x20u;require(copyGbaRomSample(rom.data(),rom.size(),mirror,copy,error)&&copy.info.address==mirror.address&&copy.pcm.size()==8,"Cartridge mirror failed");checks++;}
 for(int region:{9,11,13}){auto mirror=info;mirror.address=uint32_t(region<<24)|0x20u;reject(mirror);}
 put(32,1,2);reject(info);put(32,0,2);put(34,0x2000,2);reject(info);put(34,0x4000,2);put(36,0);reject(info);put(36,12345678);put(40,8);reject(info);put(40,2);put(44,0);reject(info);put(44,0xffffffff);reject(info);put(44,8);
 require(rom==original,"Negative fixture did not restore ROM");
 // Exact final-byte range is valid; one byte less backing is not.
 require(copyGbaRomSample(rom.data(),56,info,copy,error),"Exact sample boundary rejected");
 require(!copyGbaRomSample(nullptr,rom.size(),info,copy,error),"Null ROM accepted");
 std::vector<uint8_t>large(0x02000000u+64u);
 bad=info;bad.address=0x09fffff8u;require(!copyGbaRomSample(large.data(),large.size(),bad,copy,error),"Header crossed mirror boundary");
 bad.address=0x09ffffe8u;std::copy_n(original.data()+32,24,large.data()+0x01ffffe8u);
 require(copyGbaRomSample(large.data(),large.size(),bad,copy,error),"Exact mirror boundary sample rejected");
 large[0x01ffffe8u+12]=9;bad.length=9;require(!copyGbaRomSample(large.data(),large.size(),bad,copy,error),"PCM crossed mirror boundary");
 std::cout<<"PASS ROM ownership/signed-data/header/mirror/range/output-canary checks="<<checks+5<<"\n";
}
static void actual(const std::filesystem::path&p){
 auto bytes=readMusicFile(p);auto baseline=makeBackend(bytes,p);auto backend=makeBackend(bytes,p);GbaSampleData unsupported;unsupported.info.address=88;unsupported.pcm={7};std::string error="canary";
 require(!backend->copyGbaSample({},unsupported,error)&&error=="canary"&&unsupported.pcm==std::vector<int8_t>({7})&&unsupported.info.address==88,"Sample API active without opt-in");
 require(backend->prepareGbaInspection(),"Actual engine not recognized");backend->continuousEnd(true);baseline->continuousEnd(true);
 std::map<uint32_t,GbaSampleData>owned;int notes=0;double maxError=0;
 for(int i=0;i<563;i++){
  AudioBlock reference,block;baseline->render(512,reference);backend->render(512,block);
  require(reference.mix.size()==block.mix.size(),"Actual reference size changed");
  for(size_t j=0;j<block.mix.size();j++)maxError=std::max(maxError,double(std::max(std::abs(reference.mix[j].l-block.mix[j].l),std::abs(reference.mix[j].r-block.mix[j].r))));
  std::vector<GbaVoiceEvent>events;require(backend->drainGbaEvents(events,error),"Actual event drain failed");
  for(auto&e:events)if(e.kind==GbaVoiceEventKind::NoteOn){
   GbaSampleData sample;require(backend->copyGbaSample(e.sample,sample,error),error.c_str());require(sample.pcm.size()==e.sample.length&&sample.info.address==e.sample.address,"Actual sample size/identity mismatch");
   auto found=owned.find(e.sample.address);if(found==owned.end())owned.emplace(e.sample.address,std::move(sample));else require(found->second.pcm==sample.pcm,"Immutable sample changed");notes++;
  }
 }
 require(notes>=20&&owned.size()>=2,"Actual sample coverage too small");require(maxError==0,"ROM sample access changed original mix");
 auto before=owned;require(backend->prepareGbaInspection(),"Actual reset failed");
 for(auto&entry:owned){GbaSampleData sample;require(backend->copyGbaSample(entry.second.info,sample,error)&&sample.pcm==entry.second.pcm,"Reset changed sample");}
 backend.reset();size_t sampleBytes=0;for(auto&entry:owned){require(entry.second.pcm==before.at(entry.first).pcm,"Sample lifetime tied to decoder");sampleBytes+=entry.second.pcm.size();}
 std::cout<<"PASS actual six-second notes="<<notes<<" owned-ROM-samples="<<owned.size()<<" bytes="<<sampleBytes<<" original_mix_max_error="<<maxError<<" repeat/reset/decoder-destruction; no sample files written\n";
}
int wmain(int argc,wchar_t**argv){try{unit();if(argc>1){auto path=std::filesystem::path(argv[1])/L"channels.gsf";auto backend=makeBackend(readMusicFile(path),path);GbaSampleData out;out.pcm={5};std::string error="canary";require(!backend->copyGbaSample({},out,error)&&error=="canary"&&out.pcm==std::vector<int8_t>({5}),"Unrecognized engine sample API mutated output");}if(argc>2)actual(argv[2]);return 0;}catch(const std::exception&e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
