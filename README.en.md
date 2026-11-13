## Current test build 1.6.3-beta 9

Direct stream playback now recognizes Wii BRSTM/BNS and CRI ADX files. The supplied folder's 99 files passed bounded open/render/seek checks. The remote waveform spacing slider offers a global value or per-channel overrides, with 1× steps through 32× and a 0.5× minimum. Repeat restarts at the normal track boundary; after playback ends, Play remains available to start again. No Wii/GameCube disc-image extraction or universal game-format coverage is claimed. See RELEASE-NOTES-1.6.3-beta-9.md.

### Earlier test notes

## Current test build 1.6.3-beta 7

Exact sibling DLS -> SF2 -> VMS configured font; existing Windows fallback without VMS. DLS2 PCM regions/articulation supported. Local fonts are protected from VMS live replacement. All85NSMB MIDIs tested:80audible and5valid empty originals. No supplied music/banks included. See RELEASE-NOTES-1.6.3-beta-7.md.

### Earlier beta6 notes

## Beta 6 smooth-render revision

Reduced waveform matching/history overhead, cached outlined label coverage, and refresh-time-based display transitions. Audio remains unchanged. Native1080p rendering benchmark improved; physical refresh/VRR remains unverified. Beta6 label retained.

## Corrected beta 6 (2026-11-10)

Real-shape waveform alignment reduces drift without fixing an edge to zero. Conservative whole-sequence MP2K analysis hides unused GBA PSG channels; PCM remains visible. Unsupported driver/commands stay visible. Show all channels remains available in the remote.

# Latest test1.6.3-beta 6

Whole-song YM2612 FM/DAC usage replaces forced base-YM visibility. Only NES5/GB4/SNES8 base exceptions remain. A saved remote Show all channels checkbox temporarily overrides automatic/manual hiding without changing audio controls. Unproven usage/unknown execution ends remain visible rather than guessing from an intro or live signal.

# Previous test1.6.3-beta 6

YM3812/YM3526 primary and auxiliary channels now use a precomputed whole-song key/rhythm usage mask. Late notes are visible before playback; silence/mute/gain do not alter visibility. Decoder channels and real audio remain intact.

The latest captured waveform now runs freely: neither the left edge nor midpoint is locked to a zero crossing. Fractional display interpolation, real peaks, pause behavior and recording channel names remain. No artificial motion is generated. See RELEASE-NOTES-1.6.3-beta-6.md.

# Previous test1.6.3-beta 5

Recording view now contains only the selected visualization: channel names retained; no track/time text, grid, controls or status overlay. Normal view and the independent F12 remote remain.

Display-only waveform stabilization: DC-relative Schmitt trigger, fractional alignment/integrated pixel buckets, consistent vertical midpoint and trigger headroom at4096 samples. Real peaks and noise/transient updates are retained; decoded/audio output samples are unchanged. See RELEASE-NOTES-1.6.3-beta-5.md.

# Previous test1.6.3-beta 4

Without VirtualMIDISynth, Windows default MIDI bank lookup now tries both registry views and validated standard system paths, skipping stale or corrupt registrations. Real internal synthesis/channel waveforms remain enabled. No Windows/user sound bank is bundled. See RELEASE-NOTES-1.6.3-beta-4.md; exact error on the reporting PC has not yet been captured.

# Previous test1.6.3-beta 3

Added bounded VGM1.72 YM3526 playback (slogic.vgm) and mixed SN76489 + ES5506 support (1794124986077.vgm). Both supplied files passed full-timeline rendering, stems/mute/replay/seek and offscreen player checks. Original music is not redistributed.

Recognized Furnace ES5506 reset exports now reproduce the tracker's configured native rate, correcting the supplied lobby VGM's octave loss. Exact creator metadata plus a strict initialization signature are required. Generic ES5506 pitch, VGM event timing, other chips and the source file remain unchanged. See RELEASE-NOTES-1.6.3-beta-3.md. Keep pitch/speed1.00x when comparing. Existing F12 remote and hidden-channel controls remain.


# Previous test1.6.3-beta 2

F12 or the normal-view **Remote (F12)** button immediately left of Reverb toggles the remote independently. F9 only changes recording view; no automatic remote open/close, including startup. The remote button and footer shortcuts are hidden in recording view. F12 works from the main and known auxiliary windows. Closing the remote leaves recording unchanged. F10 retains Default apps. Future test labels use X.Y.Z-beta N. Earlier F9 auto-open records below are historical.


# Previous test1.6.3-1 beta

Right-click a channel waveform cell to hide it without muting or changing gain. F9 recording remote → Hidden channels lists manually hidden channels and restores selected/all entries. Preferences persist by system/channel label and duplicate occurrence. Existing automatic unused-channel filtering remains independent; stereo spectra are unaffected. Previous stable1.6.2 is preserved. Earlier stable documentation follows.


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
