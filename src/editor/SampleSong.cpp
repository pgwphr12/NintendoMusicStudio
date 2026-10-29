#include "SampleSong.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <stdexcept>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
namespace ngmv::editor {
namespace {
void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
double value(const SampleInstrument&s,double position){
    const auto&pcm=*s.pcm;const double size=double(pcm.size());
    if(position>=size){if(!s.loop)return 0;position=s.loopStart+std::fmod(position-s.loopStart,size-s.loopStart);}
    auto index=size_t(position);double fraction=position-double(index);
    auto next=index+1;double a=pcm[index]/128.,b=next<pcm.size()?pcm[next]/128.:s.loop?pcm[s.loopStart]/128.:0.;
    return a+(b-a)*fraction;
}
}
SampleSongRenderer::SampleSongRenderer(const Song&s):song_(s){
    song_.validate();need(song_.sampled,"This renderer requires a sampled Song");
    total_=uint64_t(std::llround(song_.seconds()*InternalRate));
    for(size_t i=0;i<song_.notes.size();i++){
        auto&n=song_.notes[i];auto&instrument=song_.instruments[n.channel];auto&sample=song_.samples[n.sample<0?instrument.sample:n.sample];
        double tickFrames=60.*InternalRate/(song_.bpm*song_.resolution);
        Scheduled entry{uint64_t(std::llround(n.step*tickFrames)),uint64_t(std::llround((n.step+n.length)*tickFrames)),i,{}};
        double base=sample.sampleRate/InternalRate*std::pow(2.,(n.pitch-sample.rootKey)/12.);
        if(!n.controls||n.controls->empty())entry.segments.push_back({0,n.sourceOffset,base,1,1});
        else{
            double position=n.sourceOffset;
            for(auto&c:*n.controls){
                uint64_t frame=uint64_t(std::llround(c.at*tickFrames));if(frame>=entry.end-entry.begin&&frame!=0)break;
                if(!entry.segments.empty()){auto&previous=entry.segments.back();position=previous.position+double(frame-previous.frame)*previous.increment;}
                entry.segments.push_back({frame,position,base*c.rate,c.left,c.right});
            }
        }
        schedule_.push_back(std::move(entry));
    }
    std::stable_sort(schedule_.begin(),schedule_.end(),[](auto&a,auto&b){return a.begin<b.begin;});
}
void SampleSongRenderer::seek(uint64_t at){
    position_=std::min(at,total_);cursor_=0;active_.clear();
    while(cursor_<schedule_.size()&&schedule_[cursor_].begin<position_){if(schedule_[cursor_].end>position_)active_.push_back(cursor_);cursor_++;}
}
void SampleSongRenderer::render(size_t count,std::vector<Stereo>&out,uint64_t mask,std::vector<std::vector<Stereo>>*voices){
    need(count<=8192,"Sample render block too large");out.assign(count,{});if(voices)voices->assign(song_.instruments.size(),std::vector<Stereo>(count));
    uint64_t end=std::min(total_,position_+count);
    while(cursor_<schedule_.size()&&schedule_[cursor_].begin<end){if(schedule_[cursor_].end>position_)active_.push_back(cursor_);cursor_++;}
    for(auto index:active_){
        auto&e=schedule_[index];auto&n=song_.notes[e.note];auto&instrument=song_.instruments[n.channel];
        if(instrument.mute||(mask&(uint64_t(1)<<n.channel)))continue;
        auto&sample=song_.samples[n.sample<0?instrument.sample:n.sample];
        // Fixed headroom; no AGC/clipping/normalization that would undo edits.
        double gain=.25*instrument.volume/15.*n.velocity/15.;
        double left=gain*(instrument.pan>0?1-instrument.pan/100.:1),right=gain*(instrument.pan<0?1+instrument.pan/100.:1);
        uint64_t at=std::max(position_,e.begin),stop=std::min(end,e.end);
        auto segment=std::upper_bound(e.segments.begin(),e.segments.end(),at-e.begin,[](uint64_t frame,const Segment&s){return frame<s.frame;});--segment;
        while(at<stop){
            auto next=segment+1;uint64_t boundary=next==e.segments.end()?stop:std::min(stop,e.begin+next->frame);
            for(;at<boundary;at++){double source=segment->position+double(at-e.begin-segment->frame)*segment->increment;double x=value(sample,source);auto&dest=out[size_t(at-position_)];float l=float(x*left*segment->left),r=float(x*right*segment->right);dest.l+=l;dest.r+=r;if(voices){auto&v=(*voices)[size_t(n.channel)][size_t(at-position_)];v.l+=l;v.r+=r;}}
            if(at<stop)segment=next;
        }
    }
    position_=end;active_.erase(std::remove_if(active_.begin(),active_.end(),[&](size_t i){return schedule_[i].end<=end;}),active_.end());
}
void exportSampleSongWav(const Song&s,const std::filesystem::path&output,std::atomic<bool>&cancel,std::atomic<int>&progress){
    need(!cancel.load(),"WAV export cancelled");SampleSongRenderer renderer(s);uint64_t bytes=renderer.frames()*8;
    need(bytes<=0xffffffffu-48u,"Sample WAV exceeds RIFF size limit");
    static std::atomic<uint64_t>serial{0};auto tmp=output;tmp+=L".sample-tmp-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(++serial);
    try{
        std::ofstream f(tmp,std::ios::binary|std::ios::trunc);need(bool(f),"Cannot create sample WAV temporary file");
        auto word=[&](uint32_t v,int n){for(int i=0;i<n;i++)f.put(char(v>>(i*8)));};
        f.write("RIFF",4);word(uint32_t(bytes)+48,4);f.write("WAVEfmt ",8);word(16,4);word(3,2);word(2,2);word(InternalRate,4);word(InternalRate*8,4);word(8,2);word(32,2);f.write("fact",4);word(4,4);word(uint32_t(renderer.frames()),4);f.write("data",4);word(uint32_t(bytes),4);
        progress=0;std::vector<Stereo>block;static_assert(sizeof(Stereo)==8,"WAV requires packed stereo float32");
        while(renderer.position()<renderer.frames()){
            need(!cancel.load(),"WAV export cancelled");auto n=size_t(std::min<uint64_t>(4096,renderer.frames()-renderer.position()));renderer.render(n,block);f.write(reinterpret_cast<const char*>(block.data()),std::streamsize(n*8));need(bool(f),"Could not write sample WAV");progress=int(renderer.position()*99/renderer.frames());
        }
        f.close();need(bool(f),"Could not finish sample WAV");need(!cancel.load(),"WAV export cancelled");
        need(MoveFileExW(tmp.c_str(),output.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0,"Could not replace sample WAV; original destination retained");progress=100;
    }catch(...){std::error_code error;std::filesystem::remove(tmp,error);throw;}
}
}
