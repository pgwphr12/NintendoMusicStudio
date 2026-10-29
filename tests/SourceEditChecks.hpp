#pragma once
#include "editor/SourceEdit.hpp"
#include <cstring>
static void checkSourceEdit(const std::filesystem::path&input,const std::filesystem::path&out){
 using namespace ngmv;using namespace ngmv::editor;
 auto need=[](bool b,const char*s){if(!b)throw std::runtime_error(s);};
 auto bytes=readMusicFile(input);auto raw=makeBackend(bytes,input);raw->continuousEnd(true);AudioBlock original;std::vector<Stereo>mix;std::vector<Stereo>voice;
 for(int i=0;i<12;i++){raw->render(4000,original);mix.insert(mix.end(),original.mix.begin(),original.mix.end());voice.insert(voice.end(),original.voices[0].begin(),original.voices[0].end());}
 SourceEdit e;e.source=std::filesystem::absolute(input);e.begin=.1;e.end=.9;e.cutBegin=.25;e.cutEnd=.5;e.fadeIn=.05;e.fadeOut=.05;e.gains.assign(raw->channels().size(),1);e.gains[0]=.25f;
 auto p=out/(input.filename().wstring()+L".ngedit");saveSourceEdit(e,p);std::ifstream project(p,std::ios::binary);auto restored=SourceEdit::parse(std::string((std::istreambuf_iterator<char>(project)),{}));need(restored.serialize()==e.serialize(),"Source edit save/reload");
 std::atomic<bool>cancel{false};std::atomic<int>progress{0};auto wav=out/(input.filename().wstring()+L".edited.wav");exportSourceWav(restored,wav,cancel,progress);need(progress==100&&std::filesystem::file_size(wav)==56+26400*8,"Edited WAV length/header");
 std::ifstream f(wav,std::ios::binary);char header[56];f.read(header,56);need(header[20]==3&&!std::memcmp(header+48,"data",4),"Edited WAV float format");
 std::vector<Stereo>pcm(26400);f.read(reinterpret_cast<char*>(pcm.data()),pcm.size()*sizeof(Stereo));for(size_t i=0;i<pcm.size();i++){size_t source=4800+i+(i>=7200?12000:0);double g=std::min(1.,std::min(double(i)/2400,double(pcm.size()-i-1)/2400));auto expected=Stereo{float((mix[source].l+voice[source].l*(-.75f))*g),float((mix[source].r+voice[source].r*(-.75f))*g)};need(std::abs(expected.l-pcm[i].l)<1e-6f&&std::abs(expected.r-pcm[i].r)<1e-6f,"WAV did not apply source edits at correct times");}
 auto edited=makeEditedSource(e);edited->seek(150);AudioBlock b;edited->render(64,b);for(int i=0;i<64;i++)need(std::abs(b.mix[i].l-pcm[7200+i].l)<1e-6,"Seek at cut boundary skipped twice");
 auto muted=e;std::fill(muted.gains.begin(),muted.gains.end(),0);auto silent=makeEditedSource(muted);silent->render(4096,b);for(auto s:b.mix)need(s.l==0&&s.r==0,"Muted edit retained residual output");
 // Cancelling must preserve a previous successful output, not publish a partial WAV.
 cancel=true;bool rejected=false;try{exportSourceWav(e,wav,cancel,progress);}catch(...){rejected=true;}need(rejected&&std::filesystem::file_size(wav)==56+26400*8,"Cancelled export overwrote destination");cancel=false;
 rejected=false;try{exportSourceWav(e,input,cancel,progress);}catch(...){rejected=true;}need(rejected&&readMusicFile(input)==bytes,"Source overwrite guard");
 auto bad=e;bad.cutBegin=0;bad.cutEnd=.2;rejected=false;try{bad.validate();}catch(...){rejected=true;}need(rejected,"Invalid cut accepted");
 std::cout<<input.filename().u8string()<<" edit/project/WAV/cut-seek/cancel/source-preservation PASS\n";
}
