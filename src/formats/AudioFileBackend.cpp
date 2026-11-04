// SPDX-License-Identifier: GPL-2.0-or-later
#include "core/IMusicBackend.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>
// Decoder only: SDL remains the sole playback device owner.
#define MA_NO_DEVICE_IO
#define MA_NO_ENGINE
#define MA_NO_RESOURCE_MANAGER
#define MA_NO_ENCODING
#define MA_NO_GENERATION
#ifdef _MSC_VER
#pragma warning(push,0)
#endif
#include "miniaudio/stb_vorbis.c"
// stb implementation leaves short channel-map macros; do not leak into Windows headers.
#undef L
#undef C
#undef R
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio/miniaudio.h"
#ifdef _MSC_VER
#pragma warning(pop)
#endif
namespace ngmv {
namespace {
// Wide-path Vorbis pull adapter for the pinned decoder. Upstream's wide-file
// fallback uses push mode, which cannot report duration and seeks by decoding.
// Keep FILE ownership inside stb; no whole-file PCM or compressed-file copy.
static ma_result vorbisWide(void*,const wchar_t*path,const ma_decoding_backend_config*config,
                            const ma_allocation_callbacks*alloc,ma_data_source**out){
    *out=nullptr;
    std::error_code ec;auto bytes=std::filesystem::file_size(path,ec);
    if(ec||bytes>0x7fffffffu)return MA_INVALID_FILE;
    auto*v=static_cast<ma_stbvorbis*>(ma_malloc(sizeof(ma_stbvorbis),alloc));
    if(!v)return MA_OUT_OF_MEMORY;
    auto result=ma_stbvorbis_init_internal(config,v);
    if(result!=MA_SUCCESS){ma_free(v,alloc);return result;}
    FILE*file=nullptr;if(_wfopen_s(&file,path,L"rb")||!file){ma_data_source_uninit(&v->ds);ma_free(v,alloc);return MA_INVALID_FILE;}
    v->stb=stb_vorbis_open_file(file,1,nullptr,nullptr); // closes FILE on failure too
    if(!v->stb){ma_data_source_uninit(&v->ds);ma_free(v,alloc);return MA_INVALID_FILE;}
    v->usingPushMode=MA_FALSE;
    result=ma_stbvorbis_post_init(v);
    if(result!=MA_SUCCESS){ma_stbvorbis_uninit(v,alloc);ma_free(v,alloc);return result;}
    *out=v;return MA_SUCCESS;
}
static ma_decoding_backend_vtable vorbisWideTable={nullptr,nullptr,vorbisWide,nullptr,ma_decoding_backend_uninit__stbvorbis};
class AudioFileBackend final:public IMusicBackend {
    struct Decoder {
        ma_decoder value{};bool initialized=false;
        ~Decoder(){if(initialized)ma_decoder_uninit(&value);}
    };
    std::unique_ptr<Decoder> decoder_=std::make_unique<Decoder>();
    std::vector<TrackInfo> tracks_;std::vector<ChannelInfo> channels_;
    std::array<float,8192*2> scratch_{};
    uint32_t mask_=0;ma_uint64 length_=0;bool manualEnd_=false;ma_uint64 position_=0,end_=0;bool endKnown_=false;
  public:
    explicit AudioFileBackend(const std::filesystem::path& path){
        auto config=ma_decoder_config_init(ma_format_f32,2,InternalRate);
        ma_decoding_backend_vtable*tables[]={&vorbisWideTable};
        config.ppCustomBackendVTables=tables;config.customBackendCount=1;
        if(ma_decoder_init_file_w(path.c_str(),&config,&decoder_->value)!=MA_SUCCESS)
            throw std::runtime_error("Cannot decode this audio file. Supported: MP3, WAV, Ogg Vorbis, FLAC. Ogg Opus is not supported.");
        decoder_->initialized=true;
        ma_format sourceFormat{};ma_uint32 sourceChannels=0,sourceRate=0;
        if(ma_data_source_get_data_format(decoder_->value.pBackend,&sourceFormat,&sourceChannels,&sourceRate,nullptr,0)!=MA_SUCCESS || !sourceChannels || sourceChannels>8 || !sourceRate || sourceRate>768000)
            throw std::runtime_error("Unsupported audio channel count or sample rate (up to8 source channels).");
        if(ma_decoder_get_length_in_pcm_frames(&decoder_->value,&length_)!=MA_SUCCESS || !length_ || length_>ma_uint64(InternalRate)*86400)
            throw std::runtime_error("Cannot read audio duration, or audio exceeds24hours.");
        if(ma_decoder_seek_to_pcm_frame(&decoder_->value,0)!=MA_SUCCESS)throw std::runtime_error("Cannot rewind audio decoder.");
        channels_=sourceChannels==1?std::vector<ChannelInfo>{{"PCM MONO"}}:std::vector<ChannelInfo>{{"PCM LEFT"},{"PCM RIGHT"}};
        TrackInfo info;info.title=path.stem().u8string();info.system="Audio file";
        info.comment=std::to_string(sourceRate)+" Hz / "+std::to_string(sourceChannels)+" source channel(s)"+(sourceChannels>2?" / stereo downmix":"");
        info.lengthMs=int((length_*1000+InternalRate-1)/InternalRate);tracks_.push_back(std::move(info));
    }
    const std::vector<TrackInfo>& tracks()const override{return tracks_;}
    const std::vector<ChannelInfo>& channels()const override{return channels_;}
    void selectTrack(int index)override{if(index!=0)throw std::runtime_error("Invalid audio track.");seek(0);}
    void seek(int ms)override{
        if(ms<0 || ms>tracks_[0].lengthMs)throw std::runtime_error("Audio seek outside duration.");
        if(ms==tracks_[0].lengthMs){manualEnd_=true;endKnown_=true;end_=ma_uint64(ms)*InternalRate/1000;return;}
        ma_uint64 frame=std::min(length_-1,ma_uint64(ms)*InternalRate/1000);
        if(ma_decoder_seek_to_pcm_frame(&decoder_->value,frame)!=MA_SUCCESS)throw std::runtime_error("Audio seek failed.");
        manualEnd_=false;position_=frame;endKnown_=false;
    }
    bool decodedEndFrame(uint64_t&frame)const override{if(!endKnown_)return false;frame=end_;return true;}
    void muteMask(uint32_t mask)override{mask_=mask;}
    void render(size_t frames,AudioBlock& out)override{
        if(frames>8192)throw std::runtime_error("Audio render block too large.");
        out.mix.assign(frames,{});out.voices.resize(channels_.size());for(auto&v:out.voices)v.assign(frames,{});
        if(manualEnd_||!frames)return;
        ma_uint64 count=0;auto result=ma_decoder_read_pcm_frames(&decoder_->value,scratch_.data(),frames,&count);
        if((result!=MA_SUCCESS&&result!=MA_AT_END)||count>frames)throw std::runtime_error("Audio decode failed.");
        position_+=count;
        if(count<frames||result==MA_AT_END){
            if(!position_)throw std::runtime_error("Audio file contains no decodable samples.");
            if(!endKnown_){end_=position_;endKnown_=true;}
        }
        for(size_t i=0;i<count;++i){
            float l=scratch_[i*2],r=scratch_[i*2+1];if(!std::isfinite(l)||!std::isfinite(r))throw std::runtime_error("Nonfinite audio sample.");
            if(channels_.size()==1){out.voices[0][i]={l,r};if(!(mask_&1))out.mix[i]={l,r};}
            else{out.voices[0][i]={l,0};out.voices[1][i]={0,r};out.mix[i]={mask_&1?0:l,mask_&2?0:r};}
        }
        // At EOF, zero-filled output keeps the waveform clock responsive.
    }
};
}
std::unique_ptr<IMusicBackend> makeAudioFileBackend(const std::filesystem::path&path){return std::make_unique<AudioFileBackend>(path);}
}
