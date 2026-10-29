#pragma once
#include "core/GbaInspection.hpp"
#include <cstring>
#include <new>
namespace ngmv {
// The pointer is the loaded emulator ROM, never a raw guest pointer. This
// bounded copy deliberately excludes mutable RAM and unknown wave encodings.
inline bool copyGbaRomSample(const uint8_t*rom,size_t romSize,const GbaSampleInfo&expected,
                             GbaSampleData&out,std::string&error){
    auto fail=[&](const char*message){error=message;return false;};
    uint32_t region=expected.address>>24,offset=expected.address&0x01ffffffu;
    if(region<8||region>13)return fail("GBA sample is not in ROM; mutable RAM sample capture is unsupported");
    if(!rom||offset>0x02000000u-16u||offset>romSize||romSize-offset<16)return fail("GBA ROM sample header is out of range");
    const uint8_t*p=rom+offset;
    auto u16=[](const uint8_t*q){return uint16_t(q[0])|uint16_t(q[1])<<8;};
    auto u32=[](const uint8_t*q){return uint32_t(q[0])|uint32_t(q[1])<<8|uint32_t(q[2])<<16|uint32_t(q[3])<<24;};
    GbaSampleInfo actual;actual.address=expected.address;actual.type=uint16_t(u16(p));actual.status=uint16_t(u16(p+2));actual.rawFrequency=u32(p+4);actual.loopStart=u32(p+8);actual.length=u32(p+12);actual.looped=(actual.status&0x4000)!=0;
    if(actual.type!=0||(actual.status&~0x4000u)||!actual.rawFrequency)
        return fail("Unsupported GBA ROM sample encoding or frequency");
    // Do not cross a 32MiB cartridge mirror boundary, even if a loader has a
    // larger backing allocation. Byte16 is the first PCM byte, not a header.
    if(!actual.length||actual.length>32u*1024u*1024u||
       actual.length>0x02000000u-offset-16u||actual.length>romSize-offset-16u||
       (actual.looped&&actual.loopStart>=actual.length))return fail("Invalid GBA ROM sample length or loop");
    if(expected.type!=actual.type||expected.status!=actual.status||expected.rawFrequency!=actual.rawFrequency||
       expected.loopStart!=actual.loopStart||expected.length!=actual.length||expected.looped!=actual.looped||
       !expected.readable||!expected.pcm8)return fail("GBA sample metadata no longer matches its ROM header");
    actual.readable=true;actual.pcm8=true;
    try{
        GbaSampleData candidate;candidate.info=actual;candidate.pcm.resize(actual.length);
        std::memcpy(candidate.pcm.data(),p+16,actual.length);
        out=std::move(candidate);error.clear();return true;
    }catch(const std::bad_alloc&){return fail("Not enough memory to copy GBA ROM sample");}
}
}
