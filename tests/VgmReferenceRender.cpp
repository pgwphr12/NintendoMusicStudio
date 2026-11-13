// SPDX-License-Identifier: GPL-2.0-or-later
#include "core/IMusicBackend.hpp"
#include <fstream>
#include <iostream>
#include <cmath>
namespace ngmv {std::unique_ptr<IMusicBackend>makeExtendedVgmBackend(const std::vector<uint8_t>&);}
int wmain(int argc,wchar_t**argv){try{
 if(argc<4||argc>5)return 2;
 auto bytes=ngmv::readMusicFile(argv[1]);
 if(argc==5){unsigned clock=unsigned(std::stoul(argv[4]));for(unsigned i=0;i<4;i++)bytes.at(0x54+i)=uint8_t(clock>>(8*i));}
 auto core=ngmv::makeExtendedVgmBackend(bytes);core->selectTrack(0);
 size_t total=size_t(std::stod(argv[3])*48000);std::ofstream out(std::filesystem::path(argv[2]),std::ios::binary);if(!out)return 3;
 ngmv::AudioBlock block;double peak=0;
 for(size_t done=0;done<total;){size_t count=std::min<size_t>(1024,total-done);core->render(count,block);if(block.mix.size()!=count)return 4;for(auto s:block.mix){if(!std::isfinite(s.l)||!std::isfinite(s.r))return 4;peak=std::max(peak,double(std::max(std::abs(s.l),std::abs(s.r))));}out.write((const char*)block.mix.data(),std::streamsize(count*sizeof(ngmv::Stereo)));done+=count;}
 out.close();if(!out)return 3;std::cout<<"frames="<<total<<" channels="<<core->channels().size()<<" peak="<<peak<<"\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
