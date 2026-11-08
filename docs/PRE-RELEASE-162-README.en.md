## Latest test1.6.2-12 — direct spectrum choices

F7, the header type button and recording remote now open a modeless choice window. Select bars, smooth lines or scrolling directly; playback and the main player remain responsive. The perspective waterfall has been removed. Scrolling is the default type; valid saved choices persist and the retired waterfall value migrates to scrolling. F6 still switches waveform/spectrum. No image feature. Earlier notes below describe historical versions.


## Previous test1.6.2-11 — horizontal scrolling spectrogram

F6 toggles waveforms/spectrum; F7 cycles bars, smooth lines, perspective waterfall and scrolling spectrogram. The fourth style follows the supplied video in a16:9 layout: Left above Right, time moves right-to-left,20Hz..20kHz logarithmic frequency runs bottom-to-top and brightness represents actual internal output. Bounded12.8s history freezes on pause and resets on seek/source change. No image feature. See RELEASE-NOTES-1.6.2-11.md. Earlier release records follow.


## Current test: 1.6.2-8

## Latest test1.6.2-10: spectrum styles, no image feature

F6 switches channel waveforms/spectrum. F7 cycles **bars, smooth lines and waterfall**. All styles use two horizontal rows (Left above Right), including recording view; the remote also changes the style. Waterfall displays actual FFT history as receding blue-grey curves, capped at32records with time-based motion/fade and resets on source seeks/restarts. View/style persist. Local image loading/decoding/display/path restoration has been removed; existing image files/settings are preserved but not used. This remains a test toward official1.6.2.


## Historical1.6.2-9 — image feature removed in1.6.2-10

F6 switches channel waveforms and spectrum. Without an image, two horizontal rows show Left above Right, both rising from their own baseline. F7 loads a local PNG/JPEG/BMP/GIF/TIFF: the aspect-fitted image is centered, with left/right spectrum bars alongside it. GIF/TIFF use the first frame. Delete or the image menu clears the view without deleting the source. Image paths and view mode persist; a missing image falls back to two rows. The F9 remote also has view/image controls.

A bounded lock-free tap observes actual internal PCM after effects, master gain and pitch/tempo. Stereo FFT analysis is capped around60Hz with time-based motion on the monitor-paced renderer. Direct external MIDI hardware output has no internal PCM to visualize. No images, music, ROMs or banks are bundled. This remains a test build toward the official1.6.2; hardware listening/VRR performance is not certified.

MesenCE-compatible Orfanidis/Butterworth EQ replaces the earlier peaking approximation. ±20dB integer steps, direct numeric entry, disabled controls, exact upstream reference comparison at48kHz. Full corresponding GPL3 component source and pinned provenance included. Scope and known15kHz-label/13kHz-filter behavior: RELEASE-NOTES-1.6.2-8.md. Earlier records follow.

## Current test: 1.6.2-7
Fixed overlapping EQ/reverb footer buttons. EQ now occupies its own right-hand slot with matching render/hit bounds. Audio and recording-remote behavior retained from1.6.2-6. See RELEASE-NOTES-1.6.2-7.md.

## Current test: 1.6.2-6
Persistent all-channel volume takes priority before first playback and across files/tracks. F9 opens a separate modeless remote. A20band40Hz–15kHz graphic EQ offers ±15dB/0.1dB controls, enable/reset and saved settings; default flat/off is exact PCM bypass. See RELEASE-NOTES-1.6.2-6.md. Earlier records follow.

## Current test: 1.6.2-5
Fixed SMA1 sparse-entry crashes: retain verified m4a slots without requiring PCM activity; synchronize transport channel states and reject inconsistent decoder buffers before indexing. See RELEASE-NOTES-1.6.2-5.md. Earlier records follow.

## Current test: 1.6.2-4
GBA ROM PCM slots are now separated through the same validated mixer taps as GSF (SMA1:6; SMA2–4:7, plus residual). GBS runs its GB CPU with the bundled GBA APU in AGB mode, native-only wave/FIFO extensions disabled. This is not full GB-ROM extraction or physical-hardware certification. See RELEASE-NOTES-1.6.2-4.md.

# Nintendo Music Studio 1.6.2-3 test build

## Test1.6.2-3: Initial NES ROM music

Open .nes in this same player.49 verified32KiB NROM program fingerprints in the SMB1 audio family, including World/Europe Rev0A/Korean and selected modifications. Matching a short code pattern alone is not acceptance: the complete PRG CRC must be registered. Other mappers, trainers, NES2.0 and unknown programs remain unsupported.

An own in-memory6502 shim invokes original sound code through the existing GME NES core. The original file/music data are preserved; no game reset/NMI/PPU/save is run and no extracted NSF is saved.14 original area/event commands include aliases/jingles, excluding stop. Four base APU lanes always show; area01/02 retain their real4011 DAC contribution in the DMC lane. No DPCM sample extraction claim. Region cadence defaults to the registered NTSC/PAL sound family. Short ROM seeks replay exact state; long seeks use the existing fast seek with at most1second of actual APU warmup. Existing NSF seeking is unchanged. No full-game-context/physical-console fidelity guarantee. Prior GBA13profiles remain; universal ROM support is still a long-term objective.

## Historical test1.6.2-2 GBA support

Open .gba in the same player, then choose Tracks or previous/next. Thirteen exact size/CRC profiles: SMA1 four profiles(319/321 IDs),SMA2 US/EU two(48 IDs),SMA3 US/EU two(40 IDs),SMA4 JP/US/EU/revision/palette-patched five(57 IDs). Nintendo drivers in2/3/4 use their original initialization, selection and frame-update routines, separately from SMA1 m4a. Original table flags and aliases are retained. No inferred sequence count, title or instrument metadata. Initial ID0 for2/3/4 and170 for1. Six hardware lanes4PSG+PCM A/B; no per-instrument PCM split in this ROM path. Other ROMs remain unsupported; universal ROM playback is a long-term objective. No ROM/save/config writes or bundled game assets. Prior test sections describe historical scope.

Test version1.6.2-3 targets official1.6.2; subsequent tests increment only the trailing iteration.

Test build only. Release packaging is prepared only when the user explicitly requests a distribution build.

## Historical test1.6.2-1: Initial GBA ROM song playback

Open .gba and select real song IDs through Tracks or previous/next in this same player. Only four locally verified Super Mario Advance1 revisions are supported (CRC32 c95824c4,69924cbd,1e4c6d6a,b993b92f);319/321 nonempty entries include sound effects. Song170 is the initial selection. Titles/durations are unavailable. Other games/revisions and NES/GB/SNES ROMs are explicitly unsupported.

An owned in-memory bootstrap executes the original ROM m4a driver in the existing GBA core. No MIDI inference, game boot, save access, external BIOS, on-disk patched ROM or extracted music file. This path exposes six hardware channels (Pulse1/2, Wave, Noise, PCM A/B), not individual sample instruments; existing GSF voice separation is retained. Local tests use offline/dummy audio and do not establish physical-console fidelity. User ROM/assets are excluded from all packages.

## 1.6.1h hotfix

Without VirtualMIDISynth, use only the Windows default bank, ignoring stale custom bank/direct-device choices for this run. Keep the old-style bank button in the same location, gray/disabled and labeled Windows default; mouse and F8 open no menu. Import/custom bank/follow/external MIDI options are restricted; internal default audio/waves/effects remain. Preserve saved selections on disk. Installed systems retain advanced controls. Presence is detected from MIDI device/executable, not just a running process or stale config; restart the player after installation/removal.

## 1.6 consolidated release

Includes all work after the public1.5.11 baseline: standalone statically linked EXE with embedded notices; large/compatible MIDI and SF2 handling; explicit enabled VirtualMIDISynth SF2 import; live saved-bank changes with position/pause/gain/mute preservation and old-bank rollback; clear internal waveform vs external MIDI output modes; fitted recording labels. MIDI labels are simplified to Channel 1..16, retaining original MIDI channel numbers; legacy gains/mutes migrate and new-name preferences take priority. See RELEASE-NOTES-1.6.md for the consolidated log. Internal synthesis uses the same SF2 samples, not captured/identical VMS/BASS output. Saved Apply/OK changes take ~1s to detect plus bank loading/seek time, with a possible playback pause. Existing format/event/memory/polyphony limits remain; no music/ROM/banks included. Historical hotfix details follow.

1.5.12h4: follow saved VirtualMIDISynth enabled-bank changes while the app is open. Import enables following; F8 toggles it, manual custom banks disable it. Poll500ms and require two stable observations (~1s detection); VMS Apply/OK must save its config. Preserve source position, playing/paused state, master and channel gain/mute. Pause briefly during background load/seek preparation; not a seamless crossfade. Missing/corrupt new fonts retain the old decoder; file size/mtime detects same-path edits. With multiple active fonts keep the currently selected one if still enabled, otherwise ask selection via F8. No layer mixing or reproduction of other VMS effect/mixer settings. Large banks/dense MIDI seek may take longer.

1.5.12h3: F8 explicitly imports an enabled SF2 from VirtualMIDISynth. Choose one if several are enabled; disabled entries are excluded. Validate/select it for internal per-channel synthesis and persist its path, without copying/moving the font or editing VMS config. Reload current MIDI from the start. Show imported filename. Missing config/enabled entries/files fail visibly with no DLS substitution. Reimport after changing VMS banks. Multi-bank layering and identical BASS/VirtualMIDISynth synthesis are not claimed.

1.5.12h2: F8 Windows default bank now explicitly uses internal synthesis with per-channel waves/effects; external MIDI direct output is a separate no-waveform option. A present VirtualMIDISynth config takes priority over the mapper preferred-ID query, preventing silent GS/DLS fallback. Only absence of VMS config falls back to system DLS; ambiguous/missing enabled fonts raise a visible error. Display the actual selected bank filename in the track-info line. Reopen/reselect for bank changes; no VMS config/sample modifications or audio identity claims. Prior large-MIDI and standalone fixes retained.

1.5.12h hotfix: default to internal MIDI synthesis for actual per-channel waveform/effects. One-time migrate the prior route default; subsequent explicit F8 device choice persists. Auto bank selection detects preferred VirtualMIDISynth #1 and reads its single enabled SF2, without modifying its config/bank. Custom selected banks take precedence. Recheck on file load/F8 auto bank. This is local synthesis using the same samples, not captured VirtualMIDISynth audio or identical BASS effects/filter/mixer behavior. Multiple layered banks/device IDs/remapping are not reproduced; select a bank manually with F8 when automatic selection is unsupported.

Raise bounded MIDI event capacity from1M to16M and merge already ordered tracks with a small track-head heap instead of a large stable-sort temporary allocation. Provided3files each6.26M..6.49M events fully render with the actual active bank;64MB input/2h duration/malformed-data guards remain. Existing256voices per MIDI channel is a synthesis limit; extreme polyphony may omit voices. No supplied MIDI/SF2 files are redistributed. Single EXE retained.

1.5.12: statically linked standalone EXE, no sidecar gme.dll or temporary extraction. Embedded component notices are available in About. Windows system libraries, a MIDI device and user/system banks remain external. Default MIDI mode sends events to Windows MIDI_MAPPER; a changed preferred device reopens at current transport state. F8 selects device output or internal system/SF2/DLS synthesis (restarts current file). Device output has silent waveform lanes and bypasses internal reverb/pitch; it is not loopback capture. Tempo, channel/master gain and mute use MIDI timing/controllers. A tool must actually update Windows' preferred output, not only private settings. Third-party driver audio/latency is not certified.

Compatibility adds bounded RIFF/RMID SMF unwrap, complete track boundaries without EOT, metadata-crossing running status and internal GM/first-preset fallback for absent banks/programs. Device mode forwards vendor SysEx. Recording labels use the cell width and measured font fitting. The user's failing file has not yet been supplied, so its specific fix is unconfirmed. SMF2, damaged/truncated tracks and >2h files still reject.

1.5.11: decide the whole-song channel list before playback and keep it fixed through pause, mute, zero gain, seek and repeat. Base NES4/GB4 and other console base channels remain visible, including GBA PSG and PCM. MIDI retains its song-wide note channel list; ordinary mono/stereo lanes remain. Extended VGM scans the entire command stream for ES voices/residual, SCC/OKI and auxiliary OPL2/AY8930; late voices appear from the start. Primary chip lanes remain fixed. This is programmed channel use, not inferred instrument separation or proof of non-silent sample contents. Audio synthesis and mix are unchanged.

Tagged NSF/NSFE checks optional DMC/expansion channels over the entire length plus fade and caches per-subsong results. Unknown-length files and analysis durations beyond one hour conservatively retain unverified optional channels; no arbitrary preview is treated as whole-song proof. Initial finite-track analysis can take time.

1.5.10: ES5506 exposes32 real voice contributions plus an explicit BUS RESIDUAL for non-voice bus output. Legacy ES5506 MIX gain/mute provides a fallback for new lanes; saved individual settings take priority. Original serial precision, saturation order, frame delay, interpolation and audible mix retained. Voices are reusable chip slots, not permanent instruments.64-bit mute masks and2/3/4-column layouts handle chips beyond32lanes (up to64). Add single AY8930 VGM(type3),3separate lanes using the explicitly BSD3 Furnace core, expanded duty/envelopes/noise and VGM stereo mask, fixed20Hz DC block. Other AY/YM models, dual AY and YM2203 SSG remain unsupported; native analog volume is approximate. MIDI/SF2/DLS and20extension registration remain included.

1.5.9: MIDI(.mid/.midi, SMF0/1) with real per-channel synthesized waveforms, tempo/program changes, controllers, drums, seeking and repeat. Uses the Windows DirectMusic GMFilePath bank (system gm.dls fallback), or an SF2/DLS bank selected with **MIDI sound bank / F8**. Selection persists; changing banks restarts the current MIDI. Registration includes all20 game/audio/MIDI extensions; prior registration consent refreshes this list while preserving Windows UserChoice.

MIDI uses internal TinySoundFont synthesis through the existing gain/pitch/tempo/reverb pipeline. The Windows bank's samples are used locally; this is **not bit-identical to Microsoft GS Wavetable Synth or MIDI Mapper**. VirtualMIDISynth-specific settings are not imported; choose its SF2 explicitly. PCM SF2 and mono PCM8/16 DLS1 are supported; use a GM bank. SF3/SFZ, compressed/DLS2 banks, SMF2, custom SF2 modulators and some DLS routes/aftertouch/vendor SysEx are unsupported or limited. Two-hour MIDI limit and2s release tail. Seeking replays synthesis and can be slow far into a long song. No system/user bank or MIDI assets are bundled.

1.5.8: opening MP3/WAV/Ogg Vorbis/FLAC starts playback immediately through the shared file-open path. Folder auto-advance remains a separate setting; manual pause remains available.

1.5.7: recording waveforms end at their channel cell boundary, retaining outer margins and the gap between columns. Channel labels sit nearer the row top with tighter text bounds. Fullscreen repeat scheduling and audio decoding remain unchanged.

1.5.6: service repeat/folder advance from coalesced display-paced UI messages as well as the idle timer fallback, preventing transport checks from waiting for idle timer delivery during fullscreen rendering. Preserve UI-thread/track-loading guards. Recording waveforms reach the right screen/column edge; labels stay above the wave. Normal margins unchanged. Own offscreen F11/F9 transitions and DS autoplay tests pass with timer1 delivery deliberately disabled. Decoder/audio code remains unchanged from1.5.5; no physical fullscreen certification is implied.

Changes: route ordinary files with recognized audio extensions to the validating decoder when initial signatures are inconclusive, while known game signatures retain priority. All13provided mission.mp3 files open/play/seek/mute/restart; no original modification. Ordinary audio repeats at actual decoded EOF rather than overstated metadata duration. Recording-view waveforms extend under the name area; labels draw above waves. Native button-repeat checks cover later GBA/DS/N64/3DS/VGM paths, with actual representative GBA/DS/N64 files checked separately. No inferred loop boundary for unknown-length emulated programs.

Open/drop MP3, WAV, Ogg Vorbis or FLAC and see live waveforms in the existing player. Stereo uses PCM LEFT/PCM RIGHT; mono uses PCM MONO. Playback, seeking, repeat, per-lane gains/mutes, pitch/speed, recording view and mixed ordinary-audio folder navigation share the existing controls.

Files are decoded as needed; no whole-track PCM preload and no external codec/FFmpeg installation. More than2 source channels are downmixed to stereo (up to8 input channels). These are output lanes, not vocal/instrument separation. Ogg Opus and M4A/AAC are unsupported;24-hour duration limit. Existing game formats and1.5.3 ES interpolation preserved. No original music/settings overwritten.

# # Run

Extract the ZIP to a new folder on Windows10/11 x64 and launch **NintendoMusicStudio.exe**. No sidecar DLL is required. No Python, FFmpeg, Visual Studio or installation is needed to run. Supply your own music and keep required companion music libraries in their original directory layout. The executable is not digitally signed. This is an unofficial project, not affiliated with or endorsed by Nintendo.

# # Playback

Supported inputs: NSF/NSFE, SPC, GBS, GSF/MINIGSF,2SF/MINI2SF, USF/MINIUSF, BCSTM/BCWAV, VGM/VGZ, MP3/WAV/OGG(Vorbis)/FLAC, MID/MIDI, and the verified GBA/NES ROM program profiles described above. VGM uses the SMS/Mega Drive path and an additional YM2612/YM3812/MSM6295/K051649/ES5506 path. All chips/combinations are not supported; unsupported combinations return errors. ES5506 exposes32voices and a BUS RESIDUAL lane; AY8930 exposes3channels. Composition/editing, project saving and WAV export are not included.

Open/drop files; select internal tracks; use previous/next, folder autoplay and repeat. Channels have English names, mute and0–200% gain; a common gain control sets all gains without changing mute/master. Pitch and speed are independent0.50–2.00 multipliers with default0.01/optional0.10 steps. Reverb defaults to76%/3.0s with a separate enable switch. Recording view hides volume controls and enlarges the clock. Waveform presentation follows the monitor at output rates44.1kHz and above.

GBA PCM VOICE lanes are live engine slots, not permanent instruments/MIDI tracks; not every driver has separate-voice support. Unsupported/inactive PSG visibility depends on actual supported observation. Waveforms represent decoder signals; large pitch/tempo adjustments may change transients/timbre.

# # Keys

Ctrl+O: open. Space: play/pause. Left/right: track change. Click channel names: mute. F1: modeless information. F9: recording view. F10: optional default-app registration/settings. F11/Esc: fullscreen/restore. Plus/minus: waveform time scale. A: folder autoplay.

# # Upgrade/settings

Extract to a separate folder; older running apps are not terminated/replaced. For compatibility, preferences remain at `%LOCALAPPDATA%/GameMusicPlayer/settings.ini`; this legacy storage name preserves existing gains/pitch/speed. The public display name/executable are Nintendo Music Studio/NintendoMusicStudio.exe. Original music and older Studio settings are not converted/deleted. Default-app registration requires choosing the prompt/F10 action; Windows remains responsible for changing the default choice.

# # Distribution and source

Distribute the **matching runtime ZIP and complete source ZIP together**, preserving LICENSE, THIRD_PARTY_NOTICES.md and licenses/. Components have GPL/LGPL/BSD/zlib/MIT terms; full source and relinking instructions are in SOURCE.md. Software licensing does not grant music/ROM/sample rights. No user music, ROMs or sample banks are included. Identified ambiguous/noncommercial dependencies were replaced; N64 and the1.4.15-reference VGM sound fixes remain enabled. See LICENSE-REVIEW.md and LICENSE-INVENTORY.json for evidence, VALIDATION.md for test scope and CHANGELOG.md for changes. No all-games/all-chip or physical-hardware fidelity certification is implied.
