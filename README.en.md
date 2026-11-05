# Nintendo Music Studio1.5.11

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

## Run

Extract the ZIP to a new folder on Windows10/11 x64 and launch **NintendoMusicStudio.exe**. Keep **gme.dll** beside it. No Python, FFmpeg, Visual Studio or installation is needed to run. Supply your own music and keep required companion music libraries in their original directory layout. The executable is not digitally signed. This is an unofficial project, not affiliated with or endorsed by Nintendo.

## Playback

Supported inputs: NSF/NSFE, SPC, GBS, GSF/MINIGSF,2SF/MINI2SF, USF/MINIUSF, BCSTM/BCWAV, VGM/VGZ, MP3/WAV/OGG(Vorbis)/FLAC. VGM uses the SMS/Mega Drive path and an additional YM2612/YM3812/MSM6295/K051649/ES5506 path. All chips/combinations are not supported; unsupported combinations return errors. ES5506 is a whole-chip MIX lane. Composition/editing, project saving and WAV export are not included.

Open/drop files; select internal tracks; use previous/next, folder autoplay and repeat. Channels have English names, mute and0–200% gain; a common gain control sets all gains without changing mute/master. Pitch and speed are independent0.50–2.00 multipliers with default0.01/optional0.10 steps. Reverb defaults to76%/3.0s with a separate enable switch. Recording view hides volume controls and enlarges the clock. Waveform presentation follows the monitor at output rates44.1kHz and above.

GBA PCM VOICE lanes are live engine slots, not permanent instruments/MIDI tracks; not every driver has separate-voice support. Unsupported/inactive PSG visibility depends on actual supported observation. Waveforms represent decoder signals; large pitch/tempo adjustments may change transients/timbre.

## Keys

Ctrl+O: open. Space: play/pause. Left/right: track change. Click channel names: mute. F1: modeless information. F9: recording view. F10: optional default-app registration/settings. F11/Esc: fullscreen/restore. Plus/minus: waveform time scale. A: folder autoplay.

## Upgrade/settings

Extract to a separate folder; older running apps are not terminated/replaced. For compatibility, preferences remain at `%LOCALAPPDATA%/GameMusicPlayer/settings.ini`; this legacy storage name preserves existing gains/pitch/speed. The public display name/executable are Nintendo Music Studio/NintendoMusicStudio.exe. Original music and older Studio settings are not converted/deleted. Default-app registration requires choosing the prompt/F10 action; Windows remains responsible for changing the default choice.

## Distribution and source

Distribute the **matching runtime ZIP and complete source ZIP together**, preserving LICENSE, THIRD_PARTY_NOTICES.md and licenses/. Components have GPL/LGPL/BSD/zlib/MIT terms; full source and relinking instructions are in SOURCE.md. Software licensing does not grant music/ROM/sample rights. No user music, ROMs or sample banks are included. Identified ambiguous/noncommercial dependencies were replaced; N64 and the1.4.15-reference VGM sound fixes remain enabled. See LICENSE-REVIEW.md and LICENSE-INVENTORY.json for evidence, VALIDATION.md for test scope and CHANGELOG.md for changes. No all-games/all-chip or physical-hardware fidelity certification is implied.
