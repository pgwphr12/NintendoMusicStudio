#include "Song.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <locale>
#include <iomanip>
#include <map>
#include <sstream>
#include <stdexcept>
namespace ngmv::editor {
static void require(bool b,const char *s){if(!b)throw std::runtime_error(s);}
static bool overlaps(const Note&a,const Note&b){return a.channel==b.channel&&a.step<b.step+b.length&&b.step<a.step+a.length;}
static bool validName(const std::string&s){return s.size()<=120&&s.find_first_of("\r\n")==std::string::npos;}
void Song::validate()const{
    require(bpm>=30&&bpm<=300&&bars>=1&&bars<=512,"Tempo/bars outside supported range");
    require(validName(title),"Invalid title");
    require(resolution>=4&&resolution<=960&&resolution%4==0,"Invalid tick resolution");
    require(notes.size()<=32768,"Too many notes");
    require(sampled?(instruments.size()>=1&&instruments.size()<=64):(instruments.size()==4&&samples.empty()),"Invalid song channel/profile layout");
    require(samples.size()<=256,"Too many sample instruments");
    size_t sampleBytes=0;
    for(auto&s:samples){
        require(validName(s.name)&&s.rootKey>=0&&s.rootKey<=127&&std::isfinite(s.sampleRate)&&s.sampleRate>=100&&s.sampleRate<=384000,"Invalid sample name/root/rate");
        require(s.pcm&&!s.pcm->empty()&&s.pcm->size()<=8u*1024u*1024u,"Invalid PCM data");
        sampleBytes+=s.pcm->size();require(sampleBytes<=8u*1024u*1024u,"Sample bank exceeds 8 MiB");
        require(!s.loop||s.loopStart<s.pcm->size(),"Invalid sample loop");
    }
    auto sampleIndex=[&](int n){return n>=0&&size_t(n)<samples.size();};
    for(auto&i:instruments){require(i.duty>=0&&i.duty<=3&&i.volume>=0&&i.volume<=15&&i.pan>=-100&&i.pan<=100&&validName(i.name),"Invalid instrument");require(sampled?sampleIndex(i.sample):(i.sample==-1&&i.pan==0&&i.name.empty()),"Invalid channel sample or NES-only metadata");}
    std::vector<std::vector<Note>>lanes(instruments.size());std::vector<std::pair<int,int>>edges;size_t controlPoints=0;
    for(auto&n:notes){
        require(n.channel>=0&&size_t(n.channel)<instruments.size()&&n.step>=0&&n.step<=ticks()&&n.length>0&&n.length<=ticks()-n.step&&n.velocity>=1&&n.velocity<=15&&n.duty>=-1&&n.duty<=3,"Invalid note position");
        require(n.pitch>=(sampled?0:n.channel==3?0:24)&&n.pitch<=(sampled?127:n.channel==3?15:95),"Invalid pitch");
        require(sampled?(n.sample==-1||sampleIndex(n.sample)):n.sample==-1,"Invalid note sample");
        require(std::isfinite(n.sourceOffset)&&n.sourceOffset>=0,"Invalid source offset");
        if(sampled){auto&sample=samples[n.sample<0?instruments[n.channel].sample:n.sample];require(n.sourceOffset<sample.pcm->size(),"Source offset exceeds sample");}
        else require(n.sourceOffset==0&&(!n.controls||n.controls->empty()),"Sample automation cannot be compiled as base NES NSF");
        if(n.controls){
            require(n.controls->size()<=8192,"Too many controls on one note");controlPoints+=n.controls->size();require(controlPoints<=262144,"Song control point limit exceeded");
            double prior=-1;
            for(auto&c:*n.controls){require(std::isfinite(c.at)&&c.at>=0&&c.at<=512.*4*resolution&&(prior<0?c.at==0:c.at>prior),"Invalid control position/order");
                require(std::isfinite(c.rate)&&c.rate>=0&&c.rate<=4096&&std::isfinite(c.left)&&c.left>=0&&c.left<=16&&std::isfinite(c.right)&&c.right>=0&&c.right<=16,"Invalid control rate or stereo gain");prior=c.at;}
        }
        lanes[n.channel].push_back(n);edges.emplace_back(n.step,1);edges.emplace_back(n.step+n.length,-1);
    }
    if(!sampled)for(auto&lane:lanes){std::sort(lane.begin(),lane.end(),[](auto&a,auto&b){return a.step<b.step;});for(size_t i=1;i<lane.size();i++)require(!overlaps(lane[i-1],lane[i]),"Each NES channel is monophonic; notes overlap");}
    else{std::sort(edges.begin(),edges.end());int active=0;for(auto e:edges){active+=e.second;require(active<=256,"Sample song exceeds 256 simultaneous notes");}}
}
void Song::put(Note n){
    Song candidate=*this;
    if(!sampled)candidate.notes.erase(std::remove_if(candidate.notes.begin(),candidate.notes.end(),[&](auto&x){return overlaps(x,n);}),candidate.notes.end());
    candidate.notes.push_back(n);candidate.validate();*this=std::move(candidate);
}
std::string Song::serialize()const{
    validate();bool automated=sampled&&std::any_of(notes.begin(),notes.end(),[](const Note&n){return n.sourceOffset!=0||(n.controls&&!n.controls->empty());});std::ostringstream o;o.imbue(std::locale::classic());o<<std::setprecision(17)<<"NGMS "<<(sampled?(automated?4:3):2)<<'\n'<<resolution<<' '<<bpm<<' '<<bars<<' '<<int(pal)<<' '<<std::quoted(title)<<'\n';
    if(sampled)o<<instruments.size()<<' '<<samples.size()<<'\n';
    for(auto&i:instruments){o<<i.duty<<' '<<i.volume<<' '<<int(i.mute);if(sampled)o<<' '<<i.sample<<' '<<i.pan<<' '<<std::quoted(i.name);o<<'\n';}
    static const char hex[]="0123456789abcdef";
    for(auto&s:samples){o<<std::quoted(s.name)<<' '<<s.rootKey<<' '<<s.sampleRate<<' '<<int(s.loop)<<' '<<s.loopStart<<' '<<s.pcm->size()<<' ';for(auto x:*s.pcm){auto v=uint8_t(x);o<<hex[v>>4]<<hex[v&15];}o<<'\n';}
    o<<notes.size()<<'\n';for(auto&n:notes){o<<n.channel<<' '<<n.step<<' '<<n.length<<' '<<n.pitch<<' '<<n.velocity<<' '<<n.duty;if(sampled)o<<' '<<n.sample;if(automated)o<<' '<<n.sourceOffset<<' '<<(n.controls?n.controls->size():0);o<<'\n';if(automated&&n.controls)for(auto&c:*n.controls)o<<c.at<<' '<<c.rate<<' '<<c.left<<' '<<c.right<<'\n';}return o.str();
}
Song Song::parse(const std::string&s){
    require(s.size()<64u*1024u*1024u,"Project file too large");std::istringstream in(s);in.imbue(std::locale::classic());std::string magic;int version,region;Song r;
    require(bool(in>>magic>>version)&&magic=="NGMS"&&(version==1||version==2||version==3||version==4),"Unsupported project format");
    require(version>=3||s.size()<1024u*1024u,"Legacy project file too large");r.sampled=version>=3;
    if(version>=2)require(bool(in>>r.resolution),"Missing tick resolution");
    require(bool(in>>r.bpm>>r.bars>>region>>std::quoted(r.title))&&(region==0||region==1),"Invalid project header");r.pal=region!=0;
    int sampleCount=0;if(r.sampled){int tracks;require(bool(in>>tracks>>sampleCount)&&tracks>=1&&tracks<=64&&sampleCount>=1&&sampleCount<=256,"Invalid sample layout");r.instruments.resize(tracks);}
    for(auto&i:r.instruments){int mute;require(bool(in>>i.duty>>i.volume>>mute)&&(mute==0||mute==1),"Invalid instrument record");i.mute=mute!=0;if(r.sampled)require(bool(in>>i.sample>>i.pan>>std::quoted(i.name)),"Invalid sampled channel record");}
    size_t sampleBytes=0;
    auto nibble=[](char c){return c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:-1;};
    for(int j=0;j<sampleCount;j++){
        SampleInstrument sample;int loop;uint32_t count;std::string hex;
        require(bool(in>>std::quoted(sample.name)>>sample.rootKey>>sample.sampleRate>>loop>>sample.loopStart>>count>>hex)&&(loop==0||loop==1),"Invalid sample record");sample.loop=loop!=0;
        require(count>0&&count<=8u*1024u*1024u&&hex.size()==size_t(count)*2,"Invalid PCM hex length");sampleBytes+=count;require(sampleBytes<=8u*1024u*1024u,"Sample bank exceeds 8 MiB");
        auto pcm=std::make_shared<std::vector<int8_t>>(count);for(size_t k=0;k<count;k++){int a=nibble(hex[k*2]),b=nibble(hex[k*2+1]);require(a>=0&&b>=0,"Invalid PCM hex digit");uint8_t byte=uint8_t(a*16+b);std::memcpy(pcm->data()+k,&byte,1);}sample.pcm=std::move(pcm);r.samples.push_back(std::move(sample));
    }
    int count;require(bool(in>>count)&&count>=0&&count<=32768,"Invalid note count");
    size_t controlPoints=0;
    for(int i=0;i<count;i++){Note n;require(bool(in>>n.channel>>n.step>>n.length>>n.pitch>>n.velocity),"Incomplete note record");if(version>=2)require(bool(in>>n.duty),"Missing note duty");if(r.sampled)require(bool(in>>n.sample),"Missing note sample");
        if(version==4){int points;require(bool(in>>n.sourceOffset>>points)&&points>=0&&points<=8192,"Invalid control count");controlPoints+=size_t(points);require(controlPoints<=262144,"Song control point limit exceeded");
            if(points){auto controls=std::make_shared<std::vector<NoteControl>>(size_t(points));for(auto&c:*controls)require(bool(in>>c.at>>c.rate>>c.left>>c.right),"Incomplete control point");n.controls=std::move(controls);}}
        r.notes.push_back(std::move(n));}
    in>>std::ws;require(in.eof(),"Unexpected trailing project data");r.validate();return r;
}
Song Song::demo(){
    Song s;s.title="First light";s.bpm=128;
    const int melody[]={72,76,79,76,74,77,81,77,76,79,83,79,74,77,79,71};
    for(int i=0;i<16;i++)s.put({0,i*4,3,melody[i],13});
    for(int i=0;i<8;i++)s.put({1,i*8,6,60+(i%4==1?5:i%4==2?7:0),8});
    for(int i=0;i<16;i++)s.put({2,i*4,3,48+(i/4==1?5:i/4==2?7:0),15});
    for(int i=0;i<32;i++)s.put({3,i*2,1,i%4==2?5:2,i%4==2?9:5});
    s.instruments[0].duty=1;s.instruments[1].duty=2;s.instruments[1].volume=7;s.instruments[3].volume=7;return s;
}
struct Asm {
    std::vector<uint8_t>b;std::map<std::string,int>labels;
    struct Fix{size_t at;std::string name;bool relative;};std::vector<Fix>fix;
    void emit(std::initializer_list<int>v){for(int x:v)b.push_back(uint8_t(x));}
    void label(const char*s){labels[s]=0x8000+int(b.size());}
    void jump(int op,const char*s,bool rel=false){emit({op,0});if(!rel)emit({0});fix.push_back({b.size()-(rel?1:2),s,rel});}
    void store(int a,int v){emit({0xa9,v,0x8d,a&255,a>>8});}
    void finish(){for(auto&f:fix){int v=labels.at(f.name);if(f.relative){int d=v-int(0x8000+f.at+1);require(d>=-128&&d<=127,"Driver branch too long");b[f.at]=uint8_t(d);}else{b[f.at]=v&255;b[f.at+1]=v>>8;}}require(b.size()<4096,"Driver too large");b.resize(4096,0xea);}
};
std::vector<uint8_t> compileNsf(const Song&s){
    s.validate();require(!s.sampled,"Sample instruments cannot be compiled as base NES NSF");const double cpu=s.pal?1662607.:1789773.;const int periodUs=s.pal?20000:16667;const double fps=1e6/periodUs;
    int frames=int(std::ceil(s.seconds()*fps));require(frames<=250000,"Song exceeds compiler limit");
    std::array<std::vector<size_t>,4> order;std::array<size_t,4>cursor{};
    for(size_t i=0;i<s.notes.size();i++)order[s.notes[i].channel].push_back(i);
    for(auto&lane:order)std::sort(lane.begin(),lane.end(),[&](size_t a,size_t b){return s.notes[a].step<s.notes[b].step;});
    std::vector<uint8_t>stream;std::map<int,int>cache;int last[4]={-1,-1,-1,-1};
    for(int f=0;f<=frames;f++){
        std::vector<std::pair<int,int>>writes;
        auto reg=[&](int a,int v,bool force=false){if(force||!cache.count(a)||cache[a]!=v){writes.push_back({a,v});cache[a]=v;}};
        double step=(f/fps)*s.bpm*s.resolution/60.;
        for(int ch=0;ch<4;ch++){
            int index=-1;if(f<frames&&!s.instruments[ch].mute){auto&lane=order[ch];while(cursor[ch]<lane.size()&&step>=s.notes[lane[cursor[ch]]].step+s.notes[lane[cursor[ch]]].length)cursor[ch]++;if(cursor[ch]<lane.size()&&step>=s.notes[lane[cursor[ch]]].step)index=int(lane[cursor[ch]]);}
            const Note*n=index<0?nullptr:&s.notes[index];bool attack=index!=last[ch];int volume=n?(s.instruments[ch].volume*n->velocity+7)/15:0;
            int duty=n&&n->duty>=0?n->duty:s.instruments[ch].duty;
            if(ch<2){int a=0x4000+ch*4;reg(a,(duty<<6)|0x30|volume);if(n&&volume){double hz=440*std::pow(2.,(n->pitch-69)/12.);int p=std::clamp(int(std::lround(cpu/(16*hz)-1)),8,2047);reg(a+2,p&255);reg(a+3,0xf8|(p>>8),attack);}}
            else if(ch==2){reg(0x4008,n&&volume?0xff:0x80);if(n&&volume){double hz=440*std::pow(2.,(n->pitch-69)/12.);int p=std::clamp(int(std::lround(cpu/(32*hz)-1)),2,2047);reg(0x400a,p&255);reg(0x400b,0xf8|(p>>8),attack);}}
            else{reg(0x400c,0x30|volume);if(n&&volume){reg(0x400e,(duty&1?0x80:0)|n->pitch);reg(0x400f,0xf8,attack);}}
            last[ch]=index;
        }
        if(f==frames)reg(0x4015,0);
        require(writes.size()<255,"Frame write overflow");stream.push_back(uint8_t(writes.size()));
        for(auto [a,v]:writes){stream.push_back(a&255);stream.push_back(a>>8);stream.push_back(v);}
    }
    stream.push_back(255);require(stream.size()<255*4096,"NSF bank limit exceeded");
    Asm a;a.label("init");for(auto [z,v]:std::vector<std::pair<int,int>>{{2,0},{3,0xa0},{4,1},{6,0}})a.emit({0xa9,v,0x85,z});
    for(auto [ad,v]:std::vector<std::pair<int,int>>{{0x5ffa,1},{0x4015,15},{0x4017,0x40},{0x4001,8},{0x4005,8},{0x4000,0x30},{0x4004,0x30},{0x4008,0x80},{0x400c,0x30},{0x4010,0},{0x4011,0}})a.store(ad,v);
    a.emit({0x60});a.label("play");a.emit({0xa5,6});a.jump(0xd0,"return",true);a.jump(0x20,"read");a.emit({0xc9,255});a.jump(0xf0,"end",true);a.emit({0x85,5,0xc9,0});a.jump(0xf0,"return",true);
    a.label("loop");a.jump(0x20,"read");a.emit({0x85,0});a.jump(0x20,"read");a.emit({0x85,1});a.jump(0x20,"read");a.emit({0xa0,0,0x91,0,0xc6,5});a.jump(0xd0,"loop",true);a.label("return");a.emit({0x60});a.label("end");a.emit({0xa9,1,0x85,6,0x60});
    a.label("read");a.emit({0xa0,0,0xb1,2,0xe6,2});a.jump(0xd0,"read_return",true);a.emit({0xe6,3,0x48,0xa5,3,0xc9,0xb0});a.jump(0xd0,"restore",true);a.emit({0xa9,0xa0,0x85,3,0xe6,4,0xa5,4,0x8d,0xfa,0x5f});a.label("restore");a.emit({0x68});a.label("read_return");a.emit({0x60});a.finish();
    std::vector<uint8_t>data(128,0);data[0]='N';data[1]='E';data[2]='S';data[3]='M';data[4]=0x1a;data[5]=1;data[6]=data[7]=1;
    auto word=[&](int o,int v){data[o]=v&255;data[o+1]=v>>8;};word(8,0x8000);word(10,a.labels.at("init"));word(12,a.labels.at("play"));
    for(size_t i=0;i<std::min(size_t(31),s.title.size());i++)data[14+i]=uint8_t(s.title[i]);
    std::string author="Nintendo Music Editor";std::copy(author.begin(),author.end(),data.begin()+46);
    word(110,periodUs);word(120,20000);data[114]=1;data[122]=s.pal?1:0;
    data.insert(data.end(),a.b.begin(),a.b.end());data.insert(data.end(),stream.begin(),stream.end());while((data.size()-128)%4096)data.push_back(255);return data;
}
}
