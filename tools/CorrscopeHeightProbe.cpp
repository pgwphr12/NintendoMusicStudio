// Read-only headless display-height measurements. Writes metrics, never audio.
#include "core/IMusicBackend.hpp"
#include "channels/ChannelHistory.hpp"
#include "visualizer/Corrscope.hpp"
#include <fstream>
#include <iostream>
#include <cmath>
#include <iomanip>
using namespace ngmv;
static std::string quote(const std::string& s){std::string out="\"";for(unsigned char c:s){if(c=='"'||c=='\\')out+='\\';if(c<32)out+=' ';else out+=char(c);}return out+'"';}
struct Stats{std::vector<float> raw,ac,height;};
static float quantile(std::vector<float> v,double q){if(v.empty())return 0;size_t at=size_t((v.size()-1)*q);std::nth_element(v.begin(),v.begin()+at,v.end());return v[at];}
int wmain(int argc,wchar_t**argv){try{
 if(argc!=4)return 2;auto input=std::filesystem::path(argv[1]);int seconds=std::stoi(argv[3]);if(seconds<1||seconds>120)return 2;
 auto backend=makeBackend(readMusicFile(input),input);backend->selectTrack(0);const auto channels=backend->channels();
 ChannelHistory history;history.reset(channels.size());std::vector<CorrscopeDisplayScale> scales(channels.size());std::vector<Stats> stats(channels.size());AudioBlock block;
 for(int frame=0;frame<seconds*60;++frame){backend->render(800,block);if(block.voices.size()!=channels.size())throw std::runtime_error("Channel count changed");history.append(block);auto wave=history.snapshot(uint64_t(frame+1)*800,4096);
  for(size_t c=0;c<wave.size();++c){auto range=std::minmax_element(wave[c].begin(),wave[c].end());float peak=std::max(std::abs(*range.first),std::abs(*range.second)),ac=(*range.second-*range.first)*.5f;auto gain=scales[c].update(ac,peak,1./60);
   if(ac>.0001f&&frame>=60){stats[c].raw.push_back(peak);stats[c].ac.push_back(ac);stats[c].height.push_back(ac*gain);}
  }
 }
 std::ofstream out(std::filesystem::path(argv[2]),std::ios::binary);out<<std::setprecision(9)<<"{\"file\":"<<quote(input.filename().u8string())<<",\"tracks\":"<<backend->tracks().size()<<",\"tested_track\":0,\"seconds\":"<<seconds<<",\"channels\":[";
 for(size_t c=0;c<channels.size();++c){if(c)out<<',';auto&s=stats[c];out<<"{\"name\":"<<quote(channels[c].name)<<",\"active_frames\":"<<s.raw.size()<<",\"raw_p50\":"<<quantile(s.raw,.5)<<",\"raw_p90\":"<<quantile(s.raw,.9)<<",\"ac_p50\":"<<quantile(s.ac,.5)<<",\"height_p10\":"<<quantile(s.height,.1)<<",\"height_p50\":"<<quantile(s.height,.5)<<",\"height_p90\":"<<quantile(s.height,.9)<<'}';}
 out<<"]}";if(!out)throw std::runtime_error("Metrics write failed");std::cout<<"PASS "<<channels.size()<<" channels, "<<seconds<<" seconds; metrics only\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
