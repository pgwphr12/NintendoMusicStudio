#include "core/IMusicBackend.hpp"
#include <chrono>
#include <iostream>
#include <algorithm>
#include <cmath>
namespace ngmv {std::unique_ptr<IMusicBackend> makePsfBackendBaseline(const std::filesystem::path&,Format);}
int wmain(int argc,wchar_t**argv){try{
 if(argc!=2)throw std::runtime_error("Usage: gba_load_benchmark local-GSF");
 std::filesystem::path p=argv[1];auto bytes=ngmv::readMusicFile(p);std::vector<double>oldTimes,newTimes;std::vector<ngmv::Stereo>original;
 for(int pass=0;pass<6;pass++){
  bool old=pass%2==0;auto start=std::chrono::steady_clock::now();auto core=old?ngmv::makePsfBackendBaseline(p,ngmv::Format::Gsf):ngmv::makeBackend(bytes,p);
  double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();(old?oldTimes:newTimes).push_back(ms);
  ngmv::AudioBlock block;core->render(4096,block);if(original.empty())original=block.mix;else for(size_t i=0;i<original.size();i++)if(original[i].l!=block.mix[i].l||original[i].r!=block.mix[i].r)throw std::runtime_error("Startup optimization changed original audio");
  std::cout<<(old?"old":"new")<<" load_ms="<<ms<<" channels="<<core->channels().size()<<'\n';
 }
 std::sort(oldTimes.begin(),oldTimes.end());std::sort(newTimes.begin(),newTimes.end());std::cout<<"median_old_ms="<<oldTimes[1]<<" median_new_ms="<<newTimes[1]<<" unchanged_audio=PASS\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
