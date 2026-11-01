#include "core/IMusicBackend.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <cmath>
#include <algorithm>
using namespace ngmv;
int wmain(int argc,wchar_t**argv){try{if(argc<3)throw std::runtime_error("private-report and readonly directories");std::ofstream report(std::filesystem::path(argv[1]),std::ios::binary);report<<"file\tchannels\tactive_PCM\tPCM_energy\tresult\n";int total=0,split=0,quiet=0,fallback=0,failed=0;
 for(int dir=2;dir<argc;dir++){std::vector<std::filesystem::path>files;for(auto&e:std::filesystem::directory_iterator(argv[dir]))if(e.path().extension()==L".minigsf")files.push_back(e.path());std::sort(files.begin(),files.end());for(auto&path:files){total++;try{auto backend=makeBackend(readMusicFile(path),path);bool separated=backend->channels().size()>6;std::vector<double>energy(backend->channels().size());AudioBlock b;for(int i=0;i<188;i++){backend->render(512,b);for(size_t c=0;c<b.voices.size();c++)for(auto v:b.voices[c]){if(!std::isfinite(v.l)||!std::isfinite(v.r))throw std::runtime_error("Non-finite stem");energy[c]+=v.l*v.l+v.r*v.r;}}double pcm=0;int active=0;for(size_t c=4;c<energy.size()-(separated?1:0);c++){pcm+=energy[c];active+=energy[c]>.0001;}std::string result=separated?"SPLIT":pcm<.0001?"NO_PCM_IN_WINDOW":"FALLBACK_PCM";if(separated)split++;else if(pcm<.0001)quiet++;else fallback++;report<<path.filename().u8string()<<'\t'<<backend->channels().size()<<'\t'<<active<<'\t'<<pcm<<'\t'<<result<<'\n';}
 catch(const std::exception&e){failed++;report<<path.filename().u8string()<<"\t0\t0\t0\tERROR "<<e.what()<<'\n';}report.flush();if(total%10==0)std::cout<<"progress="<<total<<" split="<<split<<" no_pcm="<<quiet<<" fallback="<<fallback<<" error="<<failed<<std::endl;}}
 std::cout<<"FINAL tracks="<<total<<" split="<<split<<" no_pcm_in_first2s="<<quiet<<" fallback="<<fallback<<" errors="<<failed<<"; readonly originals,2seconds per track not full-song proof\n";return failed?1:0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
