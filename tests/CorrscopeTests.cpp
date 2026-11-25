#include "visualizer/Corrscope.hpp"
#include "ui/ToolLayout.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <cmath>
#include <thread>
#include <chrono>
using namespace ngmv;
int main(int argc,char**argv){try{
 if(argc==2){std::ifstream file(argv[1],std::ios::binary);auto read=[&](auto& value){file.read(reinterpret_cast<char*>(&value),sizeof(value));if(!file)throw std::runtime_error("Truncated reference input");};uint32_t cases;read(cases);
  for(uint32_t c=0;c<cases;++c){uint32_t size,count;read(size);read(count);if(size>1000000||count>10000)throw std::runtime_error("Fixture too big");std::vector<float> data(size);file.read(reinterpret_cast<char*>(data.data()),size*4);CorrscopeTrigger trigger;
   for(uint32_t i=0;i<count;++i){int64_t pos;read(pos);auto r=trigger.process(data,0,pos);std::cout<<c<<','<<i<<','<<r.sample<<','<<r.period<<'\n';}
  }return 0;
 }
 for(auto work:std::vector<RECT>{{0,0,1920,1040},{0,40,1920,1080},{60,0,1920,1080},{-1920,0,0,1040}}){
  RECT requested{work.right-100,work.bottom-100,work.right+440,work.bottom+470};auto placed=clampToolRect(requested,work);
  if(placed.left<work.left||placed.top<work.top||placed.right>work.right||placed.bottom>work.bottom||placed.right-placed.left!=540||placed.bottom-placed.top!=570)throw std::runtime_error("Tool window extends under taskbar");
 }
 CorrscopeDisplayScale normal;for(int i=0;i<120;++i)if(std::abs(normal.update(.1625f,.1625f,1./60)-4)>1e-6)throw std::runtime_error("NSF 4x reference changed");
 for(float amplitude:{.01f,.03f,.10f,.20f,.40f}){CorrscopeDisplayScale scale;float gain=4;for(int i=0;i<120;++i)gain=scale.update(amplitude,amplitude,1./60);if(std::abs(amplitude*gain-.65f)>.0001f)throw std::runtime_error("Different chip levels do not reach common display height");}
 CorrscopeDisplayScale quiet;float gain=4;for(int i=0;i<120;++i)gain=quiet.update(.01f,.01f,1./60);
 if(quiet.update(0,0,.1)!=gain)throw std::runtime_error("Silence pumps gain");
 const float before=gain;for(int i=0;i<10;++i)gain=quiet.update(.002f,.002f,1./60);if(std::abs(gain-before)>.001f)throw std::runtime_error("Note release envelope was normalized away");
 if(std::abs((.004f*gain)/(.001f*gain)-4)>1e-6||.01f*gain<=0)throw std::runtime_error("Waveform ratios/DC changed");
 if(quiet.update(.8f,.8f,1./60)*.8f>.950001f)throw std::runtime_error("Waveform clipped");
 CorrscopeDisplayScale at60,at144;float a=4,b=4;for(int i=0;i<600;++i)a=at60.update(.01f,.01f,1./60);for(int i=0;i<1440;++i)b=at144.update(.01f,.01f,1./144);if(std::abs(a-b)>.001f)throw std::runtime_error("Gain depends on refresh rate");
 std::vector<float> silence(6000);CorrscopeTrigger trigger;auto r=trigger.process(silence,0,3000);if(r.period!=0||r.sample!=3000)throw std::runtime_error("Silence trigger unstable");
 std::vector<float> tone(6144);for(size_t i=0;i<tone.size();++i)tone[i]=float(.3*std::sin(i*.1));
 {CorrscopeWorker one,many;one.submit({tone},{1024},1,0);many.submit(std::vector<std::vector<float>>(8,tone),std::vector<size_t>(8,1024),255,0);
  for(int i=0;i<500&&(!one.completed()||!many.completed());++i)std::this_thread::sleep_for(std::chrono::milliseconds(2));
  auto a=one.result(),b=many.result();if(a.size()!=1||b.size()!=8)throw std::runtime_error("Parallel worker did not publish all lanes");
  for(const auto&lane:b){if(lane.size()!=a[0].size())throw std::runtime_error("Parallel lane width changed");for(size_t i=0;i<lane.size();++i)if(lane[i].center!=a[0][i].center)throw std::runtime_error("Parallelization changed actual waveform points");}
  many.submit(std::vector<std::vector<float>>(8,tone),std::vector<size_t>(8,1024),255,.01);many.reset();many.submit({silence},{1024},1,.02);
  for(int i=0;i<500&&!many.completed();++i)std::this_thread::sleep_for(std::chrono::milliseconds(2));if(many.result().size()!=1)throw std::runtime_error("Reset published retired lane data");
 }
 CorrscopeWorker worker;worker.reset();worker.submit({silence},{1024},1,0);worker.reset();
 std::cout<<"PASS common chip height, held note envelopes, NSF 4x reference, silence/DC/ratios, refresh rate and work-area placement\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
