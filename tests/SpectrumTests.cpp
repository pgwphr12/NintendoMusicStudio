#include "visualizer/SpectrogramHistory.hpp"
#include "visualizer/Spectrum.hpp"
#include "visualizer/AudioTap.hpp"
#include "visualizer/SpectrumCurve.hpp"
#include <iostream>
#include <thread>
#include <limits>
#include <stdexcept>
using namespace ngmv;
static int checks=0;
static void check(bool v){++checks;if(!v)throw std::runtime_error("Spectrum check "+std::to_string(checks));}
static size_t peak(const SpectrumAnalyzer::Levels& v){return size_t(std::max_element(v.begin(),v.end())-v.begin());}
int main(){try{
 SpectrumAnalyzer analyzer;SpectrumAnalyzer::Samples samples{};
 analyzer.analyze(samples,48000);for(auto v:analyzer.left)check(v==0);
 for(int rate:{16000,22050,32000,44100,48000,96000}){
  for(size_t i=0;i<SpectrumAnalyzer::Frames;++i){samples[i*2]=.5f*std::sin(float(i)*2*3.14159265359f*440/rate);samples[i*2+1]=.3f*std::sin(float(i)*2*3.14159265359f*3000/rate);}
  analyzer.analyze(samples,rate);
  auto l=peak(analyzer.left),r=peak(analyzer.right);
  auto dl=std::max_element(analyzer.detailLeft.begin(),analyzer.detailLeft.end())-analyzer.detailLeft.begin();
  auto dr=std::max_element(analyzer.detailRight.begin(),analyzer.detailRight.end())-analyzer.detailRight.begin();
  check(std::abs(20*std::pow(1000.,double(dl)/255)-440)<40);
  check(std::abs(20*std::pow(1000.,double(dr)/255)-3000)<180);
  for(float v:analyzer.detailLeft)check(std::isfinite(v)&&v>=0&&v<=1);
  check(SpectrumAnalyzer::edge(l)<510&&SpectrumAnalyzer::edge(l+1)>360);
  check(SpectrumAnalyzer::edge(r)<3300&&SpectrumAnalyzer::edge(r+1)>2700);
  check(analyzer.left[l]>.5f&&analyzer.right[r]>.5f);
  check(analyzer.right[l]<.05f&&analyzer.left[r]<.05f);
  for(auto v:analyzer.left)check(std::isfinite(v)&&v>=0&&v<=1);
 }
 // Anti-phase stereo must not disappear through mono cancellation.
 for(size_t i=0;i<SpectrumAnalyzer::Frames;++i){samples[i*2]=.2f*std::sin(float(i)*2*3.14159265359f*1000/48000);samples[i*2+1]=-samples[i*2];}
 analyzer.analyze(samples,48000);check(*std::max_element(analyzer.left.begin(),analyzer.left.end())>.5f);
 for(size_t i=0;i<analyzer.left.size();++i)check(std::abs(analyzer.left[i]-analyzer.right[i])<.00001f);
 samples.fill(std::numeric_limits<float>::quiet_NaN());analyzer.analyze(samples,48000);for(auto v:analyzer.left)check(v==0);
 AudioTap tap;samples.fill(1);check(tap.snapshot<SpectrumAnalyzer::Frames>(samples)==0);check(samples[0]==0);
 std::array<float,512> block{};for(size_t i=0;i<256;i++){block[i*2]=.25f;block[i*2+1]=-.25f;}
 tap.push(block.data(),256);check(tap.snapshot<SpectrumAnalyzer::Frames>(samples)==256);check(samples[0]==0);check(samples.back()==-.25f);
 std::atomic<bool> done{false};std::thread writer([&]{for(int i=0;i<10000;i++)tap.push(block.data(),256);done=true;});
 while(!done){tap.snapshot<SpectrumAnalyzer::Frames>(samples);for(float v:samples)if(v!=0&&v!=.25f&&v!=-.25f)std::terminate();}
 writer.join();check(tap.snapshot<SpectrumAnalyzer::Frames>(samples)>0);tap.reset();check(tap.snapshot<SpectrumAnalyzer::Frames>(samples)==0);
 SpectrumMotion motion;SpectrumAnalyzer::Levels goal{};goal.fill(1);
 for(int i=0;i<60;i++)motion.step(goal,goal,1./60.);float a=motion.left[0];
 SpectrumMotion high;for(int i=0;i<144;i++)high.step(goal,goal,1./144.);check(std::abs(a-high.left[0])<.0001f);
 goal.fill(0);for(int i=0;i<240;i++)motion.step(goal,goal,1./60.);check(motion.left[0]<.0001f);
 SpectrumAnalyzer::Levels l{};l[10]=1;
 for(int sample=0;sample<=1000;++sample){float f=sample/1000.f;float level=spectrumCurve(l,f);check(std::isfinite(level)&&level>=0&&level<=1);}
 check(spectrumCurve(l,10.f/71.f)>.999f);check(spectrumCurve(l,-1)==l.front());check(spectrumCurve(l,2)==l.back());
 SpectrogramHistory history;SpectrumAnalyzer::Detail dl{},dr{};dl[100]=1;dr[200]=.5f;
 history.push(dl,dr,0);check(history.size()==1&&history.column(0)->left[100]==255);
 history.push(dl,dr,80);check(history.size()==5&&history.column(2)->right[200]==128);
 history.push(dl,dr,240);check(history.column(8)==nullptr&&history.column(12));
 for(uint64_t t=260;t<20000;t+=20)history.push(dl,dr,t);
 check(history.size()==640&&!history.column(0)&&history.column(history.latestSlot()));
 history.push(dl,dr,1);check(history.size()==1&&history.latestSlot()==0);
 dl.fill(std::numeric_limits<float>::quiet_NaN());history.push(dl,dr,40000);
 check(history.size()==1);for(auto v:history.column(2000)->left)check(v==0);
 history.clear();check(history.size()==0&&!history.column(2000));
 std::cout<<"PASS spectrum "<<checks<<" checks: stereo/anti-phase/silence/NaN/6 rates/lock-free tap/concurrent wrap/reset/60-144Hz motion\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
