// SPDX-License-Identifier: GPL-2.0-or-later
#include "WholeTrackUsage.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <map>
#include <stdexcept>
namespace ngmv {namespace {
class WholeTrackUsage final:public IMusicBackend {
 std::unique_ptr<IMusicBackend> core;
 struct Usage{bool known=false;uint64_t mask=0;};std::map<int,Usage> cache;
 Usage current;uint64_t mute_=0;int overrideMs;
 void discover(int track){
  auto found=cache.find(track);if(found!=cache.end()){current=found->second;return;}
  Usage u;uint64_t mask=0;if(core->channelUsage(mask))u={true,mask};
  else if(size_t(track)<core->tracks().size()){
   uint64_t frames=0;bool completeBoundary=overrideMs>0;
   if(completeBoundary)frames=uint64_t(overrideMs)*InternalRate/1000;else completeBoundary=core->decodedEndFrame(frames);
   size_t count=core->channels().size();
   if(completeBoundary&&frames>0&&frames<=uint64_t(InternalRate)*3600&&count>0&&count<=64&&!core->hasDeviceOutput()){
    uint64_t all=count==64?UINT64_MAX:((uint64_t(1)<<count)-1),left=frames;bool complete=false,valid=true;
    AudioBlock b;auto started=std::chrono::steady_clock::now();core->muteMask64(0);
    try{
     while(left){size_t n=size_t(std::min<uint64_t>(left,4096));core->render(n,b);if(core->channels().size()!=count||b.voices.size()!=count){valid=false;break;}
      for(size_t c=0;c<count;c++){if(b.voices[c].size()!=n){valid=false;break;}if(!(mask&(uint64_t(1)<<c)))for(auto v:b.voices[c])if(std::abs(v.l)>1e-7f||std::abs(v.r)>1e-7f){mask|=uint64_t(1)<<c;break;}}
      if(!valid)break;left-=n;uint32_t psg=0;bool hasPsg=core->gbaPsgUsed(psg);
      if(mask==all&&(!hasPsg||(psg&15)==15)){complete=true;break;}
      if(std::chrono::steady_clock::now()-started>std::chrono::seconds(10)){valid=false;break;}
     }
     complete=valid&&(complete||!left);
     if(complete){uint32_t psg=0;if(core->gbaPsgUsed(psg))mask=(mask&~uint64_t(15))|(psg&15);u={true,mask};}
    }catch(const std::bad_alloc&){throw;}catch(const std::exception&){u={};}
    // Always restore the real beginning. Never play the analyzed tail or let
    // analysis mute leak into normal playback. Decode errors remain observable
    // if actual playback reaches them; failed analysis never hides channels.
    core->selectTrack(track);core->muteMask64(mute_);
   }
  }
  cache.emplace(track,u);current=u;
 }
public:
 WholeTrackUsage(std::unique_ptr<IMusicBackend> p,int ms):core(std::move(p)),overrideMs(ms){discover(0);}
 const std::vector<TrackInfo>&tracks()const override{return core->tracks();}
 const std::vector<ChannelInfo>&channels()const override{return core->channels();}
 void selectTrack(int t)override{core->selectTrack(t);discover(t);core->muteMask64(mute_);}
 void seek(int ms)override{core->seek(ms);}
 void muteMask(uint32_t m)override{muteMask64(m);}
 void muteMask64(uint64_t m)override{mute_=m;core->muteMask64(m);}
 bool channelUsage(uint64_t&m)const override{if(!current.known)return false;m=current.mask;return true;}
 void render(size_t n,AudioBlock&b)override{core->render(n,b);}
 bool nesState(std::vector<int>&v)const override{return core->nesState(v);}
 bool prepareNesInspection()override{return core->prepareNesInspection();}
 bool prepareGbaInspection()override{return core->prepareGbaInspection();}
 bool prepareGbaMixerInspection()override{return core->prepareGbaMixerInspection();}
 bool drainGbaEvents(std::vector<GbaVoiceEvent>&v,std::string&e)override{return core->drainGbaEvents(v,e);}
 bool drainGbaMixFrames(std::vector<GbaMixFrame>&v,std::string&e)override{return core->drainGbaMixFrames(v,e);}
 bool gbaPsgActivity(uint32_t&v)const override{return core->gbaPsgActivity(v);}
 bool gbaPsgUsed(uint32_t&v)const override{return core->gbaPsgUsed(v);}
 bool gbaEmulatedCycles(uint64_t&v)const override{return core->gbaEmulatedCycles(v);}
 bool copyGbaSample(const GbaSampleInfo&i,GbaSampleData&d,std::string&e)const override{return core->copyGbaSample(i,d,e);}
 bool decodedEndFrame(uint64_t&v)const override{return core->decodedEndFrame(v);}
 void continuousEnd(bool v)override{core->continuousEnd(v);}
 bool hasDeviceOutput()const override{return core->hasDeviceOutput();}
 void devicePlayback(uint64_t f,bool p,const std::vector<ChannelState>&s,float m)override{core->devicePlayback(f,p,s,m);}
};}
std::unique_ptr<IMusicBackend> withWholeTrackUsage(std::unique_ptr<IMusicBackend>p,int ms){return std::make_unique<WholeTrackUsage>(std::move(p),ms);}
}
