# Nintendo Music Studio 1.6.2

Windows x64 game-music, ordinary audio and MIDI player. This stable build consolidates test iterations1–12 and fixes mixed per-channel gains being overwritten on same-song stop/replay/seek.

Extract and run **NintendoMusicStudio.exe**; no sidecar DLL is needed. User media/settings and earlier builds are preserved.

- F6: waveform/spectrum. F7: modeless direct choice of bars, lines or scrolling; scrolling is the default type, saved choices persist.
- F9: recording view and separate remote. F11: fullscreen. F1: About.
- Saved common channel gain takes priority for a different song. Current mixed per-channel edits survive stop/play/seek; mute/master stay independent.
- MesenCE-style20band EQ, integer/direct gain entry, pitch/speed/reverb controls.

Supports NSF/NSFE, SPC, GBS, GSF,2SF,USF,VGM/VGZ and other supported game-music backends, ordinary audio and MIDI; not every chip/driver/file is supported. External MIDI direct output is separate from internal PCM visualizations/effects. User/system banks are referenced locally.

Direct ROM playback is limited to13verified GBA profiles and49SMB1-family NES NROM32 program fingerprints, not all ROMs. GBA PCM lanes are dynamic voice slots, not fixed instrument tracks. GB compatibility uses GB CPU execution with AGB APU, not native GBA ARM/FIFO execution.

See LICENSE, THIRD_PARTY_NOTICES.md, SOURCE.md, VALIDATION.md and RELEASE-NOTES-1.6.2.md. **NintendoMusicStudio-1.6.2-Source.zip** contains complete corresponding source, marked modifications, notices and relinking/build instructions. No game music/ROM/BIOS/user banks/settings are shipped. Unsigned executable. Earlier documentation is retained under docs/PRE-RELEASE-162-README.en.md. Future test labels use **1.6.3-1 beta**.
