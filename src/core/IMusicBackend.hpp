#pragma once
#include "Models.hpp"
#include "GbaInspection.hpp"
#include <filesystem>
#include <memory>
namespace ngmv {
class IMusicBackend {
  public:
    virtual ~IMusicBackend() = default;
    virtual const std::vector<TrackInfo> &tracks() const = 0;
    virtual const std::vector<ChannelInfo> &channels() const = 0;
    virtual void selectTrack(int index) = 0;
    virtual void seek(int milliseconds) = 0;
    virtual void muteMask(uint32_t mask) = 0;
    virtual void muteMask64(uint64_t mask) { muteMask(uint32_t(mask)); }
    // Whole-track usage, never a playback-time activity latch. False means
    // unknown: the UI must preserve optional channels rather than guess.
    virtual bool channelUsage(uint64_t&) const { return false; }
    virtual void render(size_t frames, AudioBlock &out) = 0;
    // Optional read-only base NES APU state, not a reconstructed tracker project.
    virtual bool nesState(std::vector<int>&) const { return false; }
    virtual bool prepareNesInspection(){return false;}
    // Offline standard-m4a PCM voice metadata only. This is not a complete
    // score importer and does not include PSG notes. Inspection and sample
    // access must be called on the decoder owner thread, between render calls.
    virtual bool prepareGbaInspection(){return false;}
    virtual bool prepareGbaMixerInspection(){return false;}
    virtual bool drainGbaEvents(std::vector<GbaVoiceEvent>&,std::string&) {return false;}
    virtual bool drainGbaMixFrames(std::vector<GbaMixFrame>&,std::string&) {return false;}
    virtual bool gbaPsgActivity(uint32_t&) const {return false;}
    virtual bool gbaPsgUsed(uint32_t&) const {return false;}
    // Actual emulated execution boundary; prefetch can lead audible playback.
    virtual bool gbaEmulatedCycles(uint64_t&) const {return false;}
    // Explicit offline, owned ROM-only PCM8 copy; RAM samples are unsupported.
    // Unsupported backends leave both caller outputs unchanged.
    virtual bool copyGbaSample(const GbaSampleInfo&,GbaSampleData&,std::string&) const {return false;}
    // Decoder-owner-thread query: actual absolute output-frame EOF, if known.
    // Compressed headers may overstate duration. This does not infer silence.
    virtual bool decodedEndFrame(uint64_t&) const {return false;}
    virtual void continuousEnd(bool) {}
    virtual bool hasDeviceOutput()const {return false;}
    virtual void devicePlayback(uint64_t,bool,const std::vector<ChannelState>&,float) {}
};
Format identify(const std::vector<uint8_t> &bytes, const std::filesystem::path &path = {});
std::vector<uint8_t> readMusicFile(const std::filesystem::path &path);
std::unique_ptr<IMusicBackend> makeBackend(const std::vector<uint8_t> &bytes, const std::filesystem::path &path = {});
} // namespace ngmv
