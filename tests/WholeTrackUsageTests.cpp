#include "formats/WholeTrackUsage.hpp"
#include <iostream>
#include <stdexcept>
using namespace ngmv;
void need(bool v,const char*s){if(!v)throw std::runtime_error(s);}
class OwnBackend:public IMusicBackend {
 std::vector<TrackInfo>t;std::vector<ChannelInfo>c{{"early"},{"silent"},{"late"}};uint64_t at=0;uint32_t m=0;bool bad;
public:
 OwnBackend(int ms,bool fail=false):bad(fail){TrackInfo i;i.lengthMs=ms;t.push_back(i);}
 const std::vector<TrackInfo>&tracks()const override{return t;}const std::vector<ChannelInfo>&channels()const override{return c;}
 bool decodedEndFrame(uint64_t&v)const override{if(t[0].lengthMs<0)return false;v=uint64_t(t[0].lengthMs)*48;return true;}
 void selectTrack(int)override{at=0;}void seek(int ms)override{at=uint64_t(ms)*48;}void muteMask(uint32_t v)override{m=v;}
 void render(size_t n,AudioBlock&b)override{if(bad&&at+n>24000)throw std::runtime_error("Owned failure");b.mix.assign(n,{});b.voices.assign(3,std::vector<Stereo>(n));for(size_t i=0;i<n;i++,at++){if(at<480)b.voices[0][i]={.25f,.25f};if(at>=43200&&at<45600)b.voices[2][i]={.1f,.1f};for(unsigned c=0;c<3;c++)if(!(m&(1<<c))){b.mix[i].l+=b.voices[c][i].l;b.mix[i].r+=b.voices[c][i].r;}}}
};
int main(){try{
 auto known=withWholeTrackUsage(std::make_unique<OwnBackend>(1000));uint64_t mask=0;need(known->channelUsage(mask)&&mask==5,"Late/offline channel mask failed");AudioBlock b;known->render(8,b);need(b.mix[0].l==.25f,"Offline scan leaked tail into playback");known->muteMask(7);known->selectTrack(0);need(known->channelUsage(mask)&&mask==5,"Mute/replay changed usage");known->render(8,b);need(b.mix[0].l==0,"Mute not restored");
 auto unknown=withWholeTrackUsage(std::make_unique<OwnBackend>(-1));mask=123;need(!unknown->channelUsage(mask)&&mask==123,"Unknown length guessed unused");unknown->render(8,b);need(b.mix[0].l==.25f,"Unknown input advanced by analysis");
 auto failed=withWholeTrackUsage(std::make_unique<OwnBackend>(1000,true));need(!failed->channelUsage(mask),"Failed prefix scan presented as whole track");bool threw=false;try{failed->render(25000,b);}catch(const std::runtime_error&){threw=true;}need(threw,"Actual decoder failure was swallowed");
 std::cout<<"PASS whole-track early/late/silent, start reset, mute/cache, unknown/failed analysis safety, real playback errors\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
