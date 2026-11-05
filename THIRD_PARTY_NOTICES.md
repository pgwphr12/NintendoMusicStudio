## AY8930 and ES observations (1.5.10)

AY8910/AY8930 standalone core: copyright Couriersud, explicit BSD-3-Clause in both source files, from official Furnace commit592e7fd387e7bb5f0c908452c60632c8d37cdc39/src/engine/platform/sound/ay8910.cpp and ay8910.h. Unmodified source, hashes, provenance and full BSD3 notice in third_party/furnace-ay8930 and licenses/AY8930-BSD-3-Clause.txt. No Furnace tracker code is linked.

ES5506 zlib source retains original copyright/license. Marked local1.5.10 changes add sidecar observations of original saturated bus deltas and read-only getters; no ALU/filter/envelope calculation is replaced. See UPSTREAM.json. Local1.5.11 adds a read-only programmed-voice mask used only by a separate metadata-trace core; live audio synthesis is unchanged.

## MIDI synthesis (1.5.9)

TinySoundFont, MIT: https://github.com/schellingb/TinySoundFont at853a0a171759f1ddba0de1442133a75912bbeffa. Complete pinned tsf.h and MIT license included in third_party/tinysoundfont and licenses/TinySoundFont-MIT.txt. Marked local modification bounds the final interpolation pair to the validated region; provenance/hashes in UPSTREAM.json. Own SMF parser and bounded DLS1 adapter are GPL2+. No Windows GM.DLS, user SF2/DLS samples or MIDI music is redistributed; only local read access. Internal synthesis is not identical to Windows MIDI Mapper or third-party device effects.

# Third-party notices

## Game_Music_Emu 0.6.5

Upstream: https://github.com/libgme/game-music-emu/tree/0.6.5

Used for NSF/NSFE/SPC/GBS/VGM music playback, metadata and actual voice mute/isolation APIs. Built as a separate, replaceable gme.dll with local FDS memory/bank modifications dated 2026-10-10. Complete modified files (Nsf_Emu.h, Nsf_Emu.cpp, nes_cpu_io.h) are in third_party/gme-fds-patch. third_party/apply_gme_fds.cmake checks source hashes and applies them. Modifications retain LGPL-2.1-or-later and original notices. The original archive is unchanged. Corresponding full source archive: third_party/game-music-emu-0.6.5.zip; SHA256 95444046148720dfa74ad47641dcb0232ec3e3bbd8ab5c6a013c0857da8b74bf. LGPL-2.1-or-later for the selected music-core modules; copyright Shay Green and upstream contributors. emu2413 (VRC7) uses MIT terms, copyright Mitsutaka Okazaki. Full archive retains individual notices and licenses.

The upstream archive also contains GPL-only components (such as the alternative MAME YM2612 core); those and non-target systems are not built. The archive's GPL text is included to preserve upstream notices, the new application now separately uses GPL-2.0-or-later. Consult per-file notices when changing the supported core selection. Do not replace gme.dll with a different architecture or incompatible API. Users may rebuild and replace the LGPL library for their own modified library use.

## SDL2 2.30.11

Upstream: https://github.com/libsdl-org/SDL/tree/release-2.30.11

Used for device audio callbacks and output device management; native Win32/GDI handles the UI. SDL2 is statically linked; zlib license, copyright Sam Lantinga and contributors. Full unmodified source: third_party/SDL-2.30.11.zip; SHA256 175eff804dface261f0ea239244da6c2f851b2b296d1ba70c42b6b710a90d6f5. LICENSE.txt is also copied to licenses/SDL2.txt.

## Application and fixtures

No Mesen/MesenCE source, UI or ROM is included. Reference video is used only for visual study and is not distributed. Own test music drivers are original generated diagnostic music, explicitly labeled as such; they are not Zelda music. The newly authored application and fixtures are licensed GPL-2.0-or-later; see LICENSE.

## GBA and DS music cores

GBA: viogsf/VBA-M from https://github.com/xbmc/audiodecoder.gsf at b0c965cfa5c1e6242925f23acdba391aaf4517c7. Original archive audiodecoder.gsf-b0c965cfa5c1e6242925f23acdba391aaf4517c7.zip, SHA256 6fc66b01855bde6286ad991efefea28e391abb4b7711c101ab52434659c8037b. Full compiled core source in third_party/viogsf; original notices and per-file GPL/LGPL terms retained. Application builds the core only, without Kodi APIs.

DS: vio2sf/DeSmuME from https://github.com/xbmc/audiodecoder.2sf at 039eeb7de76bf28d20c64ab4ef778982e70c639d. Original archive audiodecoder.2sf-039eeb7de76bf28d20c64ab4ef778982e70c639d.zip, SHA256 c455496f2d71b830f4e3369a5885a0e7c31ab6de33cc277214d237455f424a44. GPL-2.0-or-later; full compiled source in third_party/vio2sf. Local modifications dated 2026-10-10 add pre-mute channel PCM capture to SPU.cpp, SPU.h and state.h, and remove GCC-specific SSE flags from its CMakeLists.txt. The original archive remains intact.

zlib 1.3.1: https://github.com/madler/zlib/tree/v1.3.1, zlib license. Full source third_party/zlib; original archive zlib-1.3.1.zip SHA256 50b24b47bf19e1f35d2a21ff36d2a366638cdf958219a66f30ce0861201760e6. Used by the new bounded PSF container loader.

The user selected GPL-2.0-or-later for the combined application on 2026-10-10. See LICENSE. Complete corresponding application and core sources, patches, build files and notices are included in the separate corresponding source ZIP. The original first V1 package predates this licensing choice.

## Nintendo 64 and 3DS

lazyusf2 from the official Cog distribution, https://github.com/losnoco/Cog commit0ac68cc947a66fc88c130324a593f9bfca698d1a, Frameworks/lazyusf2/lazyusf2. The component-specific Acknowledgements/Acknowledgements.json entry explicitly declares GPL-2.0-or-later. This is also published at https://cog.losno.co/acknowledgements/. Original authors include Hacktarux, Richard Goedeken, Bobby Smiles, Ari64 and zilmar; USF integration maintained by Christopher Snowhill. Full source is third_party/lazyusf, with pinned LICENSE-EVIDENCE.json and hashed acknowledgement excerpt. Per-file CC0 RSP LLE and X-Scale BSD-2-Clause CIC exceptions remain; see licenses/lazyusf-CC0.txt and lazyusf2-CIC-BSD-2-Clause.txt. GPL2 full text is licenses/lazyusf2-Cog-GPL-2.txt. Complete source permits rebuilding/relinking. Local changes: Windows x64 rounding API, own read-only HLE voice/DMA observer callbacks, cached-interpreter CMake. No old Project64-restricted CPU source or archive is included.

vgmstream from https://github.com/vgmstream/vgmstream at 7dc938fa2f210943b37c7b6511852b516ef432ab. Archive vgmstream-7dc938fa2f210943b37c7b6511852b516ef432ab.zip SHA256 7618962679274a1d3b7da886d82365628ad15af55be8937db2f6df322643606f. Full compiled source third_party/vgmstream and COPYING are included. ISC-style license plus per-file notices. External codec libraries and player plugins are disabled; only native BCSTM/BCWAV header dispatch is exposed by the application. Build-generated version_auto.h is included with the source tree.

The application CMake configuration enables UTF-8 compilation for libvgmstream and its upstream VGM_STDIO_UNICODE path support for streamfile_stdio.c on MSVC. Upstream source files are unchanged.

V1.1 modifications dated 2026-10-10: SPU_Mix in vio2sf SPU.cpp now skips the terminal one-shot fetch position before waveform capture or mixing; see docs/DS-STABILITY.md. Original upstream notices and pinned archives are retained.

V1.2 modifications (2026-10-10): viogsf GBA.h/Sound.cpp use a 16-kernel source-rate PCM reconstruction bank. LazyUSF usf.h/usf.c, audio.c and rsp_hle/{hle_internal.h,alist.c,alist_audio.c,alist_naudio.c} add read-only dry envelope-slot taps and buffer/DMA observers. Historical1.2 archives are preserved in prior releases;1.4.17 uses the new N64 source described above. Full current modified sources and notices are included.

## V1.3 Sega changes · 2026-10-10

VGM is enabled with the LGPL-2.1-or-later GENS YM2612 implementation, copyright Stéphane Dallongeville and Shay Green. The alternative GPL MAME and Nuked implementations are not compiled. YM2413 uses the bundled MIT emu2413 by Mitsutaka Okazaki; licenses/emu2413-MIT.txt preserves its notice. Local LGPL interface modifications replace the empty YM2413 stub and separate SMS FM/rhythm/PSG and Genesis FM/DAC/PSG voices. Complete modified Vgm_Emu.cpp, Ym2413_Emu.cpp and Ym2413_Emu.h are in third_party/gme-sega-patch; third_party/apply_gme_sega.cmake verifies original/patched hashes. Vgm_Emu.cpp retains original notices. The original libgme archive is unchanged. The application validates VGM command/PCM bounds and decompresses VGZ using zlib. Distribute the matching source ZIP alongside the runtime ZIP; see SOURCE.md.


## V1.4.8 GBA source-state inspection · 2026-10-30

Local Gb_Apu.h/.cpp modifications add default-off read-only sticky PSG enabled/DAC/envelope/routing observation for explicit offline import. Oscillator stepping, samples and serialized register state are unchanged. Original LGPL notices remain; complete corresponding modified source is included. GbaMixCapture separately observes standard-m4a CGB START writes, including muted notes, to reject unsupported PSG imports rather than silently dropping them.


V1.4.10 local modifications (2026-10-31): viogsf GBA.h/Sound.cpp add null-default read-only APU register/volume/frame observer callbacks; original synthesis remains unchanged. Full corresponding modified sources and original GPL/LGPL notices retained; GbaPsgMirror is new application GPL code.


## 1.4.17 VGM license replacements

The old unassigned libvgm player/utils/SoundEmu/Resampler code, its selected undeclared Gens YM2612 copy and the old noncommercial-MAME ES5506 adaptation are absent from this source tree and runtime build. Their historical releases remain separate; they are not relicensed by this change.

The new bounded VGM scheduler/metadata reader/DAC-stream handling and causal resampling adapters are independent application GPL-2.0-or-later code in src/formats/ExtendedVgmBackend.cpp and LicensedChips.cpp/.hpp. Minimal functional C ABI declarations under third_party/licensed-chips are independently authored; no old libvgm framework/header implementations are carried over.

Selected cores retain explicit original notices, with receipts in third_party/licensed-chips/UPSTREAM.json:

- fmopl.c: GPL-2.0-or-later, Jarek Burczynski/Tatsuyuki Satoh. Only OPL2 is compiled; local change guards the unused Y8950 header include. Full GPL2 text is supplied.
- k051649.c: BSD-3-Clause, Bryan McPhail.
- okim6295.c: BSD-3-Clause, Mirko Buffoni/Aaron Giles; okiadpcm.c/.h: BSD-3-Clause, Andrew Gardner/Aaron Giles. Full BSD notice is licenses/selected-chips-BSD-3-Clause.txt.
- ym3438.c and ym3438_int.h: LGPL-2.1-or-later, Alexey Khokholov (Nuke.YKT), Nuked OPN2. Original license retained. licenses/Nuked-OPN2-LGPL-2.1.txt is supplied. The complete corresponding library/application source and CMake relinking build are supplied to meet LGPL2.1 section6a; no unsupported conversion of its source notices is asserted.
- vgsound_emu ES5506: zlib, cam900 and contributors. Pinned Furnace mirror592e7fd387e7bb5f0c908452c60632c8d37cdc39, subtree extern/vgsound_emu-modified/vgsound_emu. Original synthesis code is retained;1.5.2adds a marked read-only serial precision getter in es5506.hpp. The own adapter now reads actual configured output buses; the old debug voice sum is no longer used. Original LICENSE and per-file zlib headers retained, also licenses/vgsound-ES5506-zlib.txt.

The necessary replacement changes synthesis/resampling implementations and output calibration. It preserves the supported command subset and tested channels/replay/seek/mute, not bit-identical PCM or universal hardware accuracy. ES5505 is rejected explicitly rather than synthesized as ES5506.

## N64 replacement —1.4.17

The previous root-CC0 interpretation was too broad; the old Project64 noncommercial/personal-use source has been removed, not relabeled. N64 remains enabled using the official Cog-specific GPL lazyusf2 distribution described above. The separate GitLab repository's missing root grant was not filled in by assumption. The pinned Cog component-specific declaration and actual distributed source are the evidence. No question/contact was posted, following the user's instruction. Original exceptions/attributions remain, and the own voice observer is ported only onto GPL HLE code. This does not grant rights to game music or ROM data.

## Independent pitch/tempo (1.4.15)

Signalsmith Stretch by Geraint Luff / Signalsmith Audio Ltd. and Signalsmith Linear by Signalsmith Audio: MIT. Pinned unmodified headers, source metadata and MIT notices are included in third_party/signalsmith/; license texts are also in licenses/Signalsmith-{Stretch,Linear}-MIT.txt. The own integration is src/audio/PitchTempo.cpp. No optional external FFT backend is enabled. MIT notices remain unchanged. The VGM/N64 replacements above remove the identified earlier licensing blockers. The current internal candidate is not a public release or legal certification.

## 1.4.18 sound correction

Own GPL adapter/scheduler changes restore SCC native-rate handling and remove chip-count attenuation. Native sample-cell overlap integration is independently authored. Licensed cores and original notices remain unchanged; no old libvgm resampler or restricted core is restored.

## Public1.5 package

Nintendo Music Studio1.5 retains the licensed replacement cores and18sound fixes. Display branding/version and Windows application identity changed; audio core source is unchanged from18. Full matching source/build inputs and all original component notices are distributed together. Old internal-version findings above are historical; current source/notice mapping is in LICENSE-INVENTORY.json.

## 1.5.2 timbre candidate

ES5506 real bus/serial precision normalization replace the former debug sum; normalized bus unity level is retained. Relative volume entries multiply the current default; absolute entries override it. Source changes are supplied and no original license is replaced. This is a validation candidate, not a claim of exact reproduction of an unavailable creator project.


1.5.2 level correction: ES5506 extra default attenuation from1.5.1 is removed; real serial buses remain. Historical1.5.1 correlation is not a chip-balance validation. See docs/VGM-LEVEL-152.json.


##1.5.3 interpolation candidate

Own GPL2+ ChipInterpolator.hpp replaces ES5506 sub48kHz host linear interpolation only. No external resampler added, no original synthesis/filter/decoder/license changed.32-tap Blackman-windowed sinc, fixed16-native-sample causal delay, normalized DC gain.1.5.2 chip levels retained.


##1.5.4 ordinary audio decoding

miniaudio0.11.23 (David Reid and contributors), chosen MIT option; unmodified miniaudio.h and original dual-license text retained. Builtin dr_wav/dr_mp3/dr_flac notices remain in the header. stb_vorbis (Sean Barrett and contributors), chosen MIT option; unmodified upstream extras/stb_vorbis.c includes original terms. Source/hash provenance: third_party/miniaudio/UPSTREAM.json. Runtime notices: licenses/miniaudio-MIT.txt and licenses/stb-vorbis-MIT.txt. Own decoder-only adapter and Unicode Vorbis pull hookup are project GPL2+ code; SDL remains device owner. No music assets included.
