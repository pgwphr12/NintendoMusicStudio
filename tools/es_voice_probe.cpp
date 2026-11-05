#include "core/IMusicBackend.hpp"
#include <array>
#include <fstream>
#include <iostream>
int wmain(int argc,wchar_t**argv){try{
 if(argc<3||argc>4)return 2;
 unsigned seconds=argc==4?std::stoul(argv[3]):12;
 if(!seconds||seconds>600)return 2;
 auto core=ngmv::makeBackend(ngmv::readMusicFile(argv[1]),argv[1]);
 std::ofstream out(std::filesystem::path(argv[2]),std::ios::binary);
 for(auto& c:core->channels())std::cout<<c.name<<'\n';
 ngmv::AudioBlock b;
 for(size_t left=size_t(48000)*seconds;left;){size_t n=std::min(left,size_t(1024));core->render(n,b);left-=n;
  for(size_t i=0;i<n;++i){std::array<ngmv::Stereo,4> row{};row[0]=b.mix[i];
   for(size_t c=0;c<b.voices.size();++c){auto group=core->channels()[c].name.find("YM2612")!=std::string::npos?1:core->channels()[c].name.find("MSM6295")!=std::string::npos?2:3;row[group].l+=b.voices[c][i].l;row[group].r+=b.voices[c][i].r;}
   out.write(reinterpret_cast<const char*>(row.data()),sizeof(row));
  }
 }
 return out?0:3;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
