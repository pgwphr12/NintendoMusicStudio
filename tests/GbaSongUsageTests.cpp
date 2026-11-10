#include "formats/GbaSongUsage.hpp"
#include "core/IMusicBackend.hpp"
#include <iostream>
#include <stdexcept>
using namespace ngmv;
void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
int wmain(int argc,wchar_t**argv){try{
 std::vector<uint8_t>r(8192);auto put=[&](size_t at,uint32_t v){for(int i=0;i<4;++i)r[at+i]=uint8_t(v>>(8*i));};
 auto read=[&](uint32_t at,size_t n)->const uint8_t*{return at<=r.size()&&n<=r.size()-at?r.data()+at:nullptr;};
 r[32]=1;put(36,256);put(40,2048);r[256]=0;r[268]=2;r[280]=4;
 r[2048]=0xbd;r[2049]=0;r[2050]=0xd0;r[2051]=60;r[2052]=100;r[2053]=0xb1;
 uint32_t mask=123;need(gbaSongPsgUsage(read,32,mask)&&mask==0,"PCM-only PSG hidden proof");
 // Late voice behind wait and pattern; pattern changes running VOICE status.
 r[2053]=0xb3;put(2054,3000);r[2058]=2;r[2059]=0xb2;put(2060,2048);
 r[3000]=0xb0;r[3001]=0xbd;r[3002]=1;r[3003]=0xb4;
 need(gbaSongPsgUsage(read,32,mask)&&mask==10,"Late/pattern/running-status channels lost");
 r[2058]=0xb9;mask=123;need(!gbaSongPsgUsage(read,32,mask)&&mask==123,"Unsupported command guessed");
 r[2058]=2;r[280]=0x20;need(!gbaSongPsgUsage(read,32,mask),"Unsupported rhythm guessed");
 r[280]=4;
 // Explicit keysplit includes late noise and wave, including independent extras.
 r[280]=0x40;put(284,4000);put(288,5000);for(int k=0;k<128;++k)r[5000+k]=k%2;
 r[4000]=3;r[4012]=4;need(gbaSongPsgUsage(read,32,mask)&&mask==14,"Keysplit child voices lost");
 r[4000]=0x40;put(4004,4000);put(4008,5000);need(gbaSongPsgUsage(read,32,mask)&&mask==10,"Nested split should skip note as MP2K does");
 r[280]=4;put(2054,8190);need(!gbaSongPsgUsage(read,32,mask),"Truncation accepted");
 std::cout<<"PASS PCM-only/late PSG/pattern return/running status/loop/unsupported/truncated; no audio-intro heuristic\n";
 for(int i=1;i<argc;++i){auto data=readMusicFile(argv[i]);auto b=makeBackend(data,argv[i]);uint64_t used=0;bool known=b->channelUsage(used);std::cout<<"GSF case="<<i<<" known="<<known<<" PSG="<<(used&15)<<" channels="<<b->channels().size()<<'\n';if(known){need((used&~uint64_t(15))==(UINT64_MAX&~uint64_t(15)),"PCM hidden");AudioBlock block;b->render(8192,block);uint64_t later=0;need(b->channelUsage(later)&&later==used,"Mask changes during playback");b->seek(1000);need(b->channelUsage(later)&&later==used,"Mask changes on seek");}need(data==readMusicFile(argv[i]),"Original changed");}
 return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
