#include "GmeBackend.hpp"
#include "Midi.hpp"
#include "PsfBackend.hpp"
#include "GbaRom.hpp"
#include "NesRom.hpp"
#include "StreamBackend.hpp"
#include <array>
#include <cstring>
#include <algorithm>
#include <cwctype>
#include <fstream>
#include <stdexcept>
#include <zlib.h>
namespace ngmv {
std::unique_ptr<IMusicBackend> makeAudioFileBackend(const std::filesystem::path&);
Format identify(const std::vector<uint8_t> &b, const std::filesystem::path &path) {
    auto romExt=path.extension().wstring();std::transform(romExt.begin(),romExt.end(),romExt.begin(),[](wchar_t c){return std::towlower(c);});
    if(romExt==L".gba")return Format::GbaRom;
    if(romExt==L".nes"||(b.size()>=4&&!memcmp(b.data(),"NES\x1a",4)))return Format::NesRom;
    if((b.size()>=4 && !memcmp(b.data(),"MThd",4))||(b.size()>=12&&!memcmp(b.data(),"RIFF",4)&&!memcmp(b.data()+8,"RMID",4)))return Format::Midi;
    if((b.size()>=12 && (!memcmp(b.data(),"RIFF",4)||!memcmp(b.data(),"RF64",4)) && !memcmp(b.data()+8,"WAVE",4)) ||
       (b.size()>=4 && (!memcmp(b.data(),"OggS",4)||!memcmp(b.data(),"fLaC",4))) ||
       (b.size()>=3 && !memcmp(b.data(),"ID3",3)) ||
       (b.size()>=4 && b[0]==0xff && (b[1]&0xe0)==0xe0 && (b[1]&6) && (b[2]&0xf0)!=0xf0))return Format::AudioFile;
    if (b.size() > 64 && !memcmp(b.data(), "Vgm ", 4))
        return Format::Vgm;
    if (b.size() >= 64 && !memcmp(b.data(), "CSTM", 4))
        return Format::Bcstm;
    if (b.size() >= 64 && !memcmp(b.data(), "CWAV", 4))
        return Format::Bcwav;
    if (b.size() >= 128 && !memcmp(b.data(), "NESM\x1a", 5))
        return Format::Nsf;
    if (b.size() >= 4 && !memcmp(b.data(), "NSFE", 4))
        return Format::Nsfe;
    if (b.size() >= 0x10200 && !memcmp(b.data(), "SNES-SPC700 Sound File Data", 26))
        return Format::Spc;
    if (b.size() >= 112 && !memcmp(b.data(), "GBS", 3))
        return Format::Gbs;
    if (b.size() >= 16 && !memcmp(b.data(), "PSF", 3)) {
        if (b[3] == 0x21)
            return Format::Usf;
        if (b[3] == 0x22)
            return Format::Gsf;
        if (b[3] == 0x24)
            return Format::TwoSf;
    }
    // Signatures of known game formats take precedence. Ordinary audio may
    // start with padding or a partial frame; let the real decoder validate it.
    auto ext=path.extension().wstring();
    std::transform(ext.begin(),ext.end(),ext.begin(),[](wchar_t c){return std::towlower(c);});
    if(ext==L".mp3"||ext==L".wav"||ext==L".ogg"||ext==L".flac")return Format::AudioFile;
    throw std::runtime_error("Unsupported or truncated music file. Open NSF / NSFE / SPC / GBS / "
                             "GSF / 2SF / USF / BCSTM / BCWAV / VGM / VGZ; "
                             "MIDI format0/1 / MP3 / WAV / OGG Vorbis / FLAC; verified GBA / NES ROM profiles only.");
}
std::vector<uint8_t> readMusicFile(const std::filesystem::path &p) {
    std::ifstream f(p, std::ios::binary | std::ios::ate);
    if (!f)
        throw std::runtime_error("Cannot open music file.");
    auto n = f.tellg();
    if (n >= 64) {
        std::vector<uint8_t> header(64);
        f.seekg(0);
        f.read(reinterpret_cast<char *>(header.data()), 64);
        bool knownGame=!memcmp(header.data(),"NESM\x1a",5)||!memcmp(header.data(),"NSFE",4)||
                       !memcmp(header.data(),"MThd",4)||!memcmp(header.data(),"Vgm ",4)||!memcmp(header.data(),"GBS",3)||
                       !memcmp(header.data(),"PSF",3)||!memcmp(header.data(),"SNES-SPC700 Sound File Data",26);
        bool ordinary=false;try{ordinary=!knownGame && identify(header,p)==Format::AudioFile;}catch(const std::exception&){}
        if (ordinary || !memcmp(header.data(), "CSTM", 4) || !memcmp(header.data(), "CWAV", 4))
            return header;
    }
    if (n <= 0 || n > 64 * 1024 * 1024)
        throw std::runtime_error("Music file is empty or exceeds the 64 MB safety limit.");
    std::vector<uint8_t> b(static_cast<size_t>(n));
    f.seekg(0);
    if (!f.read(reinterpret_cast<char *>(b.data()), n))
        throw std::runtime_error("Incomplete file read.");
    if (b.size() >= 3 && b[0] == 0x1f && b[1] == 0x8b && b[2] == 8) {
        z_stream stream{};
        stream.next_in = b.data();
        stream.avail_in = unsigned(b.size());
        if (inflateInit2(&stream, 15 + 16) != Z_OK)
            throw std::runtime_error("Cannot initialize VGZ decoder.");
        std::vector<uint8_t> decoded;
        std::array<uint8_t, 65536> block{};
        int status = Z_OK;
        do {
            stream.next_out = block.data();
            stream.avail_out = unsigned(block.size());
            status = inflate(&stream, Z_NO_FLUSH);
            auto count = block.size() - stream.avail_out;
            if (decoded.size() + count > 64 * 1024 * 1024) {
                inflateEnd(&stream);
                throw std::runtime_error("VGZ exceeds the 64 MB decompression limit.");
            }
            decoded.insert(decoded.end(), block.begin(), block.begin() + count);
        } while (status == Z_OK);
        bool valid = status == Z_STREAM_END && stream.avail_in == 0;
        inflateEnd(&stream);
        if (!valid || decoded.size() <= 64 || memcmp(decoded.data(), "Vgm ", 4))
            throw std::runtime_error("Damaged VGZ or invalid VGM data (CRC/length).");
        return decoded;
    }
    return b;
}
std::unique_ptr<IMusicBackend> makeBackend(const std::vector<uint8_t> &b,
                                           const std::filesystem::path &path) {
    switch (identify(b,path)) {
    case Format::NesRom:
        return makeNesRomBackend(b);
    case Format::GbaRom:
        return makeGbaRomBackend(b);
    case Format::Midi:
        return makeMidiBackend(b,path);
    case Format::AudioFile:
        return makeAudioFileBackend(path);
    case Format::Vgm:
        return makeVgmBackend(b);
    case Format::Bcstm:
    case Format::Bcwav:
        return makeStreamBackend(path);
    case Format::Usf:
    case Format::Gsf:
    case Format::TwoSf:
        return makePsfBackend(path, identify(b,path));
    case Format::Nsf:
        return std::make_unique<NSFBackend>(b);
    case Format::Nsfe:
        return std::make_unique<NSFEBackend>(b);
    case Format::Spc:
        return std::make_unique<SPCBackend>(b);
    case Format::Gbs:
        return std::make_unique<GBSBackend>(b);
    }
    throw std::runtime_error("Unsupported format.");
}
} // namespace ngmv
