// Owned bounded DLS1/SF2 adapter for the pinned TinySoundFont implementation.
// Windows sample banks are read locally and never copied to a release package.
struct BankChunk {size_t p=0,n=0;std::string id,type;std::vector<BankChunk> children;};
static uint16_t le16(const std::vector<uint8_t>&b,size_t p){if(p>b.size()||b.size()-p<2)throw std::runtime_error("Truncated sound bank.");return uint16_t(b[p]|b[p+1]<<8);}
static uint32_t le32(const std::vector<uint8_t>&b,size_t p){return uint32_t(le16(b,p))|(uint32_t(le16(b,p+2))<<16);}
static std::vector<BankChunk> bankChunks(const std::vector<uint8_t>&b,size_t lo,size_t hi,unsigned depth,unsigned&count){
    if(depth>8||hi>b.size())throw std::runtime_error("Invalid sound bank nesting.");std::vector<BankChunk> out;
    while(lo<hi){if(hi-lo<8||++count>100000)throw std::runtime_error("Invalid sound bank chunk table.");size_t n=le32(b,lo+4),p=lo+8;if(n>hi-p)throw std::runtime_error("Sound bank chunk exceeds file.");
        BankChunk c{p,n,std::string(reinterpret_cast<const char*>(b.data()+lo),4),{}, {}};
        if(c.id=="RIFF"||c.id=="LIST"){if(n<4)throw std::runtime_error("Invalid sound bank list.");c.type=std::string(reinterpret_cast<const char*>(b.data()+p),4);c.children=bankChunks(b,p+4,p+n,depth+1,count);}
        out.push_back(std::move(c));lo=p+n+(n&1);if(lo>hi)throw std::runtime_error("Missing sound bank padding.");
    }return out;
}
static const BankChunk* findChunk(const BankChunk&c,const char*id,const char*type=""){
    for(const auto&v:c.children)if(v.id==id&&(!*type||v.type==type))return &v;return nullptr;
}
static const BankChunk& requireChunk(const BankChunk&c,const char*id,size_t minimum=0,const char*type=""){
    auto p=findChunk(c,id,type);if(!p||p->n<minimum)throw std::runtime_error(std::string("Missing sound bank chunk: ")+id);return *p;
}
static void requireTable(const BankChunk&c,size_t stride,size_t minimum){if(c.n%stride||c.n/stride<minimum||c.n/stride>65535)throw std::runtime_error("Invalid SF2 table size.");}
static size_t validateSf2(const std::vector<uint8_t>&b,const BankChunk&root){
    const auto&sd=requireChunk(root,"LIST",0,"sdta");const auto&sm=requireChunk(sd,"smpl",4);
    if(sm.n%2||sm.n/2>64000000)throw std::runtime_error("Unsupported SF2 sample data.");
    const auto&pd=requireChunk(root,"LIST",0,"pdta");
    const auto&ph=requireChunk(pd,"phdr"),&pb=requireChunk(pd,"pbag"),&pm=requireChunk(pd,"pmod"),&pg=requireChunk(pd,"pgen");
    const auto&in=requireChunk(pd,"inst"),&ib=requireChunk(pd,"ibag"),&im=requireChunk(pd,"imod"),&ig=requireChunk(pd,"igen"),&sh=requireChunk(pd,"shdr");
    requireTable(ph,38,2);requireTable(pb,4,2);requireTable(pm,10,1);requireTable(pg,4,1);requireTable(in,22,2);requireTable(ib,4,2);requireTable(im,10,1);requireTable(ig,4,1);requireTable(sh,46,2);
    auto monotonic=[&](const BankChunk&table,size_t stride,size_t offset,size_t maximum){uint16_t prev=0;for(size_t i=0;i<table.n;i+=stride){auto x=le16(b,table.p+i+offset);if(x<prev||x>=maximum)throw std::runtime_error("Invalid SF2 table index.");prev=x;}};
    monotonic(ph,38,24,pb.n/4);monotonic(in,22,20,ib.n/4);
    monotonic(pb,4,0,pg.n/4);monotonic(pb,4,2,pm.n/10);monotonic(ib,4,0,ig.n/4);monotonic(ib,4,2,im.n/10);
    for(size_t i=0;i<pg.n;i+=4)if(le16(b,pg.p+i)==41&&le16(b,pg.p+i+2)>=in.n/22-1)throw std::runtime_error("SF2 instrument index outside table.");
    for(size_t i=0;i<ig.n;i+=4)if(le16(b,ig.p+i)==53&&le16(b,ig.p+i+2)>=sh.n/46-1)throw std::runtime_error("SF2 sample index outside table.");
    for(size_t i=0;i+46<sh.n;i+=46){auto p=sh.p+i;auto start=le32(b,p+20),end=le32(b,p+24),ls=le32(b,p+28),le=le32(b,p+32),rate=le32(b,p+36);auto type=le16(b,p+44);
        if(start>=end||end>sm.n/2||ls>le||le>sm.n/2||!rate||rate>768000||(type!=1&&type!=2&&type!=4))throw std::runtime_error("Invalid or unsupported SF2 sample header (PCM SF2 only).");}
    return sm.n/2;
}
struct DlsWave {unsigned offset=0,count=0,rate=0;const BankChunk* ws=nullptr;};
static void applyWsmp(tsf_region&r,const std::vector<uint8_t>&b,const BankChunk&w,unsigned count){
    if(w.n<20)throw std::runtime_error("Invalid DLS sample parameters.");auto p=w.p;unsigned size=le32(b,p),loops=le32(b,p+16);
    if(size<20||size>w.n||loops>1)throw std::runtime_error("Unsupported DLS sample loop.");
    r.pitch_keycenter=le16(b,p+4);r.tune=int16_t(le16(b,p+6));r.attenuation=-float(int32_t(le32(b,p+8)))/655360.f;
    if(r.pitch_keycenter>127)throw std::runtime_error("Invalid DLS root key.");
    if(loops){if(w.n-size<16||le32(b,p+size)<16||le32(b,p+size+4)!=0)throw std::runtime_error("Unsupported DLS loop type.");auto start=le32(b,p+size+8),length=le32(b,p+size+12);
        if(length<2||start>=count||length>count-start)throw std::runtime_error("DLS loop outside sample.");r.loop_mode=TSF_LOOPMODE_CONTINUOUS;r.loop_start=r.offset+start;r.loop_end=r.loop_start+length-1;}
}
static void dlsArt(tsf_region&r,const std::vector<uint8_t>&b,const BankChunk&parent,unsigned&unsupported){
    auto list=findChunk(parent,"LIST","lart");if(!list)return;
    for(const auto&a:list->children)if(a.id=="art1"){
        if(a.n<8)throw std::runtime_error("Truncated DLS articulation.");auto size=le32(b,a.p),n=le32(b,a.p+4);if(size<8||size>a.n||n>(a.n-size)/12)throw std::runtime_error("Invalid DLS articulation table.");
        for(unsigned i=0;i<n;i++){auto p=a.p+size+i*12;auto src=le16(b,p),ctrl=le16(b,p+2),dst=le16(b,p+4),transform=le16(b,p+6);float v=float(int32_t(le32(b,p+8)))/65536.f;
            if(transform){++unsupported;continue;}
            if(src==0&&ctrl==0){switch(dst){
                case 1:r.attenuation-=v/10.f;break;case 3:r.tune+=int(v);break;case 4:r.pan=std::clamp(v/1000.f,-.5f,.5f);break;
                case 0x104:r.freqModLFO=int(v);break;case 0x105:r.delayModLFO=v;break;
                case 0x206:r.ampenv.attack=v;break;case 0x207:r.ampenv.decay=v;break;case 0x209:r.ampenv.release=v;break;
                case 0x20a:r.ampenv.sustain=std::clamp(v/1000.f,0.f,1.f);break;
                case 0x30a:r.modenv.attack=v;break;case 0x30b:r.modenv.decay=v;break;case 0x30d:r.modenv.release=v;break;case 0x30e:r.modenv.sustain=std::clamp(v/1000.f,0.f,1.f);break;
                case 2:case 0x208:case 0x30c:break; // reserved DLS1 destinations
                default:++unsupported;break;}
            }else if(src==1&&ctrl==0&&dst==3)r.modLfoToPitch=int(v);
            else if(src==1&&ctrl==0&&dst==1)r.modLfoToVolume=int(v);
            else if(src==5&&ctrl==0&&dst==3)r.modEnvToPitch=int(v);
            else if(src==3&&ctrl==0&&(dst==0x207||dst==0x30b)){if(dst==0x207)r.ampenv.keynumToDecay=v/128.f;else r.modenv.keynumToDecay=v/128.f;}
            else if(src==1&&ctrl==129&&dst==3){++unsupported;} // modulation-wheel route not synthesized by TSF
            else ++unsupported;
        }
    }
}
static tsf* loadDls(const std::vector<uint8_t>&b,const BankChunk&root,unsigned&unsupported){
    const auto&pool=requireChunk(root,"LIST",0,"wvpl");const auto&pt=requireChunk(root,"ptbl",8);unsigned hs=le32(b,pt.p),cues=le32(b,pt.p+4);
    if(hs<8||hs>pt.n||!cues||cues>65535||cues>(pt.n-hs)/4)throw std::runtime_error("Invalid DLS wave pool table.");
    std::map<size_t,DlsWave> waves;std::vector<float> samples;
    for(const auto&w:pool.children)if(w.id=="LIST"&&w.type=="wave"){
        const auto&fmt=requireChunk(w,"fmt ",16),&data=requireChunk(w,"data",2);unsigned bits=le16(b,fmt.p+14),rate=le32(b,fmt.p+4);
        if(le16(b,fmt.p)!=1||le16(b,fmt.p+2)!=1||(bits!=8&&bits!=16)||!rate||rate>768000||data.n%(bits/8))throw std::runtime_error("DLS requires mono PCM8/16 samples.");
        unsigned count=unsigned(data.n/(bits/8)),offset=unsigned(samples.size());if(samples.size()+count+2>64000000)throw std::runtime_error("DLS sample limit exceeded.");
        for(unsigned i=0;i<count;i++)samples.push_back(bits==16?float(int16_t(le16(b,data.p+i*2)))/32768.f:(int(b[data.p+i])-128)/128.f);
        samples.push_back(0);samples.push_back(0);waves[w.p-8-(pool.p+4)]={offset,count,rate,findChunk(w,"wsmp")};
    }
    std::vector<DlsWave> table;for(unsigned i=0;i<cues;i++){auto it=waves.find(le32(b,pt.p+hs+i*4));if(it==waves.end())throw std::runtime_error("DLS wave pool offset not found.");table.push_back(it->second);}
    const auto&lins=requireChunk(root,"LIST",0,"lins");std::vector<tsf_preset> presets;std::vector<std::vector<tsf_region>> regions;
    for(const auto&ins:lins.children)if(ins.id=="LIST"&&ins.type=="ins "){
        const auto&head=requireChunk(ins,"insh",12),&lrgn=requireChunk(ins,"LIST",0,"lrgn");uint32_t bank=le32(b,head.p+4),program=le32(b,head.p+8);if(program>127)throw std::runtime_error("Invalid DLS program.");
        tsf_preset preset{};preset.preset=tsf_u16(program);preset.bank=tsf_u16((bank&0x80000000)?128:((bank>>8)&127)*128+(bank&127));
        snprintf(preset.presetName,sizeof(preset.presetName),"DLS %u",unsigned(program));std::vector<tsf_region> rr;
        for(const auto&reg:lrgn.children)if(reg.id=="LIST"&&reg.type=="rgn "){
            const auto&rh=requireChunk(reg,"rgnh",12),&link=requireChunk(reg,"wlnk",12);unsigned wi=le32(b,link.p+8);if(wi>=table.size())throw std::runtime_error("DLS sample reference outside table.");auto wave=table[wi];
            tsf_region v;tsf_region_clear(&v,TSF_FALSE);v.offset=wave.offset;v.end=wave.offset+wave.count;v.sample_rate=wave.rate;v.pitch_keycenter=60;v.group=le16(b,rh.p+10);
            unsigned low=le16(b,rh.p),high=le16(b,rh.p+2),vl=le16(b,rh.p+4),vh=le16(b,rh.p+6);if(low>high||high>127||vl>vh||vh>127)throw std::runtime_error("Invalid DLS key/velocity range.");
            v.lokey=uint8_t(low);v.hikey=uint8_t(high);v.lovel=uint8_t(vl);v.hivel=uint8_t(vh);
            if(auto ws=findChunk(reg,"wsmp"))applyWsmp(v,b,*ws,wave.count);else if(wave.ws)applyWsmp(v,b,*wave.ws,wave.count);
            // DLS sustain is a linear level; TSF's runtime uses a linear level too.
            v.ampenv.sustain=v.modenv.sustain=1;dlsArt(v,b,ins,unsupported);dlsArt(v,b,reg,unsupported);
            auto ampS=v.ampenv.sustain,modS=v.modenv.sustain;v.ampenv.sustain=v.modenv.sustain=0;
            tsf_region_envtosecs(&v.ampenv,TSF_TRUE);tsf_region_envtosecs(&v.modenv,TSF_FALSE);v.ampenv.sustain=ampS;v.modenv.sustain=modS;
            v.delayModLFO=v.delayModLFO<-11950?0:tsf_timecents2Secsf(v.delayModLFO);v.delayVibLFO=0;
            rr.push_back(v);
        }
        if(rr.empty()||rr.size()!=le32(b,head.p)||presets.size()>=4096)throw std::runtime_error("Invalid DLS region count.");presets.push_back(preset);regions.push_back(std::move(rr));
    }
    if(presets.empty())throw std::runtime_error("DLS contains no supported instruments.");
    std::unique_ptr<tsf,decltype(&tsf_close)> font(static_cast<tsf*>(calloc(1,sizeof(tsf))),tsf_close);if(!font)throw std::bad_alloc();
    font->presetNum=int(presets.size());font->presets=static_cast<tsf_preset*>(calloc(presets.size(),sizeof(tsf_preset)));if(!font->presets){font->presetNum=0;throw std::bad_alloc();}
    font->fontSamples=static_cast<float*>(malloc(samples.size()*sizeof(float)));if(!font->fontSamples)throw std::bad_alloc();memcpy(font->fontSamples,samples.data(),samples.size()*sizeof(float));
    for(size_t i=0;i<presets.size();i++){font->presets[i]=presets[i];auto&dst=font->presets[i];dst.regionNum=int(regions[i].size());dst.regions=static_cast<tsf_region*>(malloc(regions[i].size()*sizeof(tsf_region)));if(!dst.regions)throw std::bad_alloc();memcpy(dst.regions,regions[i].data(),regions[i].size()*sizeof(tsf_region));}
    return font.release();
}
