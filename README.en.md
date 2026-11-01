# Nintendo Music Studio 1.5

A Windows game-music player with per-channel waveforms, mute and volume controls. This public package incorporates the internal player improvements and1.4.18 VGM pitch/gain regression fixes. It does not include composition or music editing.

## Run

Extract the ZIP to a new folder on Windows10/11 x64 and launch **NintendoMusicStudio.exe**. Keep **gme.dll** beside it. No Python, FFmpeg, Visual Studio or installation is needed to run. Supply your own music and keep required companion music libraries in their original directory layout. The executable is not digitally signed. This is an unofficial project, not affiliated with or endorsed by Nintendo.

## Playback

Supported inputs: NSF/NSFE, SPC, GBS, GSF/MINIGSF,2SF/MINI2SF, USF/MINIUSF, BCSTM/BCWAV, VGM/VGZ. VGM uses the SMS/Mega Drive path and an additional YM2612/YM3812/MSM6295/K051649/ES5506 path. All chips/combinations are not supported; unsupported combinations return errors. ES5506 is a whole-chip MIX lane. MP3, composition/editing, project saving and WAV export are not included.

Open/drop files; select internal tracks; use previous/next, folder autoplay and repeat. Channels have English names, mute and0–200% gain; a common gain control sets all gains without changing mute/master. Pitch and speed are independent0.50–2.00 multipliers with default0.01/optional0.10 steps. Reverb defaults to76%/3.0s with a separate enable switch. Recording view hides volume controls and enlarges the clock. Waveform presentation follows the monitor at output rates44.1kHz and above.

GBA PCM VOICE lanes are live engine slots, not permanent instruments/MIDI tracks; not every driver has separate-voice support. Unsupported/inactive PSG visibility depends on actual supported observation. Waveforms represent decoder signals; large pitch/tempo adjustments may change transients/timbre.

## Keys

Ctrl+O: open. Space: play/pause. Left/right: track change. Click channel names: mute. F1: modeless information. F9: recording view. F10: optional default-app registration/settings. F11/Esc: fullscreen/restore. Plus/minus: waveform time scale. A: folder autoplay.

## Upgrade/settings

Extract to a separate folder; older running apps are not terminated/replaced. For compatibility, preferences remain at `%LOCALAPPDATA%/GameMusicPlayer/settings.ini`; this legacy storage name preserves existing gains/pitch/speed. The public display name/executable are Nintendo Music Studio/NintendoMusicStudio.exe. Original music and older Studio settings are not converted/deleted. Default-app registration requires choosing the prompt/F10 action; Windows remains responsible for changing the default choice.

## Distribution and source

Distribute the **matching runtime ZIP and complete source ZIP together**, preserving LICENSE, THIRD_PARTY_NOTICES.md and licenses/. Components have GPL/LGPL/BSD/zlib/MIT terms; full source and relinking instructions are in SOURCE.md. Software licensing does not grant music/ROM/sample rights. No user music, ROMs or sample banks are included. Identified ambiguous/noncommercial dependencies were replaced; N64 and the1.4.15-reference VGM sound fixes remain enabled. See LICENSE-REVIEW.md and LICENSE-INVENTORY.json for evidence, VALIDATION.md for test scope and CHANGELOG.md for changes. No all-games/all-chip or physical-hardware fidelity certification is implied.
