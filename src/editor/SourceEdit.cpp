#include "SourceEdit.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <windows.h>
namespace ngmv::editor {
namespace {
void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
uint64_t frame(double seconds){return uint64_t(std::llround(seconds*InternalRate));}
void word(std::ostream&o,uint32_t n,int count){for(int i=0;i<count;i++)o.put(char(n>>(8*i)));}
std::filesystem::path temporary(const std::filesystem::path&p){auto r=p;r+=L".ngms-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64())+L".tmp";return r;}
void replace(const std::filesystem::path&tmp,const std::filesystem::path&p){need(MoveFileExW(tmp.c_str(),p.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0,"Could not replace destination; existing file retained");}
void destination(const SourceEdit&e,const std::filesystem::path&p){
    auto a=std::filesystem::absolute(p).lexically_normal().wstring(),b=std::filesystem::absolute(e.source).lexically_normal().wstring();
    std::error_code ec;bool same=std::filesystem::equivalent(p,e.source,ec);
    need(!same&&CompareStringOrdinal(a.c_str(),int(a.size()),b.c_str(),int(b.size()),TRUE)!=CSTR_EQUAL,"Choose a different destination; source music is preserved");
}
class EditedSource final:public IMusicBackend {
    SourceEdit edit;std::unique_ptr<IMusicBackend> core;
    std::vector<TrackInfo> info;std::vector<ChannelInfo> names;
    uint64_t position=0;uint32_t mask=0;bool cutSkipped=false;std::atomic<bool>*cancel=nullptr;
    uint64_t total()const{return frame(edit.end)-frame(edit.begin)-(frame(edit.cutEnd)-frame(edit.cutBegin));}
    uint64_t sourcePosition(uint64_t at)const{auto result=frame(edit.begin)+at;if(edit.cutEnd>edit.cutBegin&&result>=frame(edit.cutBegin))result+=frame(edit.cutEnd)-frame(edit.cutBegin);return result;}
    void discardTo(uint64_t target){AudioBlock b;while(target){need(!cancel||!cancel->load(),"Operation cancelled");auto n=size_t(std::min<uint64_t>(target,4096));core->render(n,b);target-=n;}}
public:
    explicit EditedSource(const SourceEdit&e,std::atomic<bool>*c):edit(e),cancel(c){
        edit.validate();core=makeBackend(readMusicFile(edit.source),edit.source);
        need(edit.track<int(core->tracks().size()),"Invalid source track");
        names=core->channels();need(edit.gains.empty()||edit.gains.size()==names.size(),"Source channel layout changed; reload the music");
        if(edit.gains.empty())edit.gains.assign(names.size(),1.f);
        info={core->tracks()[edit.track]};info[0].lengthMs=int(std::ceil(double(total())*1000/InternalRate));
        // Constructor already selected track zero. Avoid a second GBA core rebuild.
        if(edit.track)core->selectTrack(edit.track);core->continuousEnd(true);discardTo(frame(edit.begin));
    }
    const std::vector<TrackInfo>&tracks()const override{return info;}
    const std::vector<ChannelInfo>&channels()const override{return names;}
    void selectTrack(int n)override{need(n==0,"Edited source has one track");seek(0);}
    void continuousEnd(bool)override{}
    void muteMask(uint32_t value)override{mask=value;}
    void seek(int ms)override{need(ms>=0,"Invalid edit seek");position=std::min<uint64_t>(uint64_t(ms)*InternalRate/1000,total());cutSkipped=edit.cutEnd>edit.cutBegin&&position>=frame(edit.cutBegin)-frame(edit.begin);core->selectTrack(edit.track);core->continuousEnd(true);discardTo(sourcePosition(position));}
    void render(size_t frames,AudioBlock&out)override{
        need(frames<=8192,"Audio block too large");out.mix.assign(frames,{});out.voices.assign(names.size(),std::vector<Stereo>(frames));size_t at=0;
        bool silent=true;for(size_t c=0;c<names.size();c++)if(edit.gains[c]>0&&!(c<32&&(mask&(uint32_t(1)<<c)))){silent=false;break;}
        while(at<frames&&position<total()){
            uint64_t n=std::min<uint64_t>(frames-at,total()-position);
            if(edit.cutEnd>edit.cutBegin){auto boundary=frame(edit.cutBegin)-frame(edit.begin);if(position==boundary&&!cutSkipped){discardTo(frame(edit.cutEnd)-frame(edit.cutBegin));cutSkipped=true;}else if(position<boundary)n=std::min(n,boundary-position);}
            AudioBlock block;core->render(size_t(n),block);need(block.mix.size()==n&&block.voices.size()==names.size(),"Invalid decoder channel block");
            for(size_t i=0;i<n;i++){
                auto mix=block.mix[i];double gain=1;
                if(edit.fadeIn>0)gain=std::min(gain,double(position+i)/std::max<uint64_t>(1,frame(edit.fadeIn)));
                if(edit.fadeOut>0)gain=std::min(gain,double(total()-position-i-1)/std::max<uint64_t>(1,frame(edit.fadeOut)));
                for(size_t c=0;c<names.size();c++){
                    need(block.voices[c].size()==n,"Invalid decoder voice length");auto v=block.voices[c][i];float g=(c<32&&(mask&(uint32_t(1)<<c)))?0:edit.gains[c];
                    // Keep the authoritative mix (including effects) and apply only the edited delta.
                    mix.l+=v.l*(g-1);mix.r+=v.r*(g-1);out.voices[c][at+i]={float(v.l*g*gain),float(v.r*g*gain)};
                }
                out.mix[at+i]=silent?Stereo{}:Stereo{float(mix.l*gain),float(mix.r*gain)};
            }
            position+=n;at+=size_t(n);
        }
    }
};
}
double SourceEdit::duration()const{return end-begin-(cutEnd-cutBegin);}
void SourceEdit::validate()const{
    need(!source.empty()&&source.u8string().size()<32768&&source.u8string().find_first_of("\r\n")==std::string::npos,"Invalid source path");
    for(double x:{begin,end,cutBegin,cutEnd,fadeIn,fadeOut})need(std::isfinite(x)&&x>=0&&x<=3600,"Times must be between 0 and 3600 seconds");
    need(track>=0&&end>begin&&duration()>=.02,"Invalid source range");
    need(cutBegin==cutEnd||(cutBegin>=begin&&cutEnd>cutBegin&&cutEnd<=end),"Cut must be inside the retained range");
    need(fadeIn<=duration()&&fadeOut<=duration(),"Fade exceeds edited duration");
    need(gains.size()<=64,"Too many channels");for(float x:gains)need(std::isfinite(x)&&x>=0&&x<=2,"Channel gain must be 0–200%");
}
std::string SourceEdit::serialize()const{validate();std::ostringstream o;o<<std::setprecision(17)<<"NGME 1\n"<<std::quoted(source.u8string())<<'\n'<<track<<' '<<begin<<' '<<end<<' '<<cutBegin<<' '<<cutEnd<<' '<<fadeIn<<' '<<fadeOut<<'\n'<<gains.size()<<'\n';for(auto g:gains)o<<g<<' ';return o.str()+'\n';}
SourceEdit SourceEdit::parse(const std::string&s){need(s.size()<131072,"Edit project too large");std::istringstream in(s);std::string magic,path;int version,count;SourceEdit e;need(bool(in>>magic>>version)&&magic=="NGME"&&version==1,"Unsupported edit project");need(bool(in>>std::quoted(path)>>e.track>>e.begin>>e.end>>e.cutBegin>>e.cutEnd>>e.fadeIn>>e.fadeOut>>count)&&count>=0&&count<=64,"Invalid edit project");e.source=std::filesystem::u8path(path);e.gains.resize(count);for(auto&g:e.gains)need(bool(in>>g),"Incomplete channel gains");in>>std::ws;need(in.eof(),"Unexpected project data");e.validate();return e;}
std::unique_ptr<IMusicBackend> makeEditedSource(const SourceEdit&e,std::atomic<bool>*cancel){return std::make_unique<EditedSource>(e,cancel);}
void saveSourceEdit(const SourceEdit&e,const std::filesystem::path&p){destination(e,p);auto tmp=temporary(p);try{std::ofstream f(tmp,std::ios::binary);auto s=e.serialize();f.write(s.data(),s.size());f.close();need(bool(f),"Could not write project");replace(tmp,p);}catch(...){std::error_code ec;std::filesystem::remove(tmp,ec);throw;}}
void exportSourceWav(const SourceEdit&e,const std::filesystem::path&p,std::atomic<bool>&cancel,std::atomic<int>&progress){
    e.validate();destination(e,p);progress=0;need(!cancel,"Export cancelled");auto core=makeEditedSource(e,&cancel);auto total=frame(e.end)-frame(e.begin)-(frame(e.cutEnd)-frame(e.cutBegin));auto tmp=temporary(p);
    try{std::ofstream f(tmp,std::ios::binary);need(bool(f),"Could not create WAV");f.write("RIFF",4);word(f,uint32_t(48+total*8),4);f.write("WAVEfmt ",8);word(f,16,4);word(f,3,2);word(f,2,2);word(f,48000,4);word(f,384000,4);word(f,8,2);word(f,32,2);f.write("fact",4);word(f,4,4);word(f,uint32_t(total),4);f.write("data",4);word(f,uint32_t(total*8),4);
        for(uint64_t at=0;at<total;){need(!cancel,"Export cancelled");size_t n=size_t(std::min<uint64_t>(4096,total-at));AudioBlock block;core->render(n,block);for(auto s:block.mix){need(std::isfinite(s.l)&&std::isfinite(s.r),"Non-finite audio");f.write(reinterpret_cast<const char*>(&s.l),4);f.write(reinterpret_cast<const char*>(&s.r),4);}need(bool(f),"WAV write failed; check free space");at+=n;progress=int(at*100/total);}
        f.close();need(bool(f),"Could not finish WAV");need(!cancel,"Export cancelled");replace(tmp,p);progress=100;
    }catch(...){std::error_code ec;std::filesystem::remove(tmp,ec);throw;}
}
}
