## Nintendo Music Studio 1.6.4

Game Boy high-frequency wave playback now uses the actual active wave-table average, fixing attenuation of NR32 PCM speech such as Pikachu voices. The legacy option is shortened to “Legacy waveform”; behavior is unchanged. See RELEASE-NOTES-1.6.4.md.

- English is the default when no language preference exists. Saved Korean/English choices remain unchanged.
- All formats use master0–100% and common/individual channels0–200%; the MIDI500% extension has been removed. Older excessive gain settings are capped.
- Manual **SF2/DLS selection** works without VirtualMIDISynth and persists across restarts. Only VMS import/follow actions require VMS.
- Corrscope results ease at each monitor-paced paint, independently of output sample rate; trigger/audio remain unchanged.
- The F8 menu has a persistent **Search same-folder sound banks** option, on by default (DLS then SF2). Turn it off to use your selected bank. Selecting a bank manually turns it off; it can be enabled again. Toggling reloads the current MIDI from the beginning.

Corrscope display scaling reduces differences between songs, retaining the NSF 4x reference and applying a single gain to the trace. Held envelopes preserve note releases and silence; shape, timing, DC offset, audio and trigger inputs stay intact. The remote opens and reopens within the monitor work area, clear of the taskbar.

The remote now offers **Corrscope waveform** below the legacy option, using a native translation of the actual corrscope 0.11.1 default correlation/period/pitch-tracking code on a separate worker. The middle **Language** selector switches Korean/English immediately and saves the choice. Existing standard and legacy waveform paths stay separate.

Standard waveform motion is restored to stable 1.6.3. Legacy keeps its original integer rising-edge trigger and pixel buckets, with the reference search moved to the center and a small free range rather than a fixed zero-height center. Tool window overlaps, DPI sizing and duplicated text rendering were also fixed.

Export uses the modern Windows folder picker, starting beside the source music. Results go into a song-named subfolder, with a numeric suffix if it already exists. Used-channel WAV exports now include the decoder's original `master.wav`, with the same duration and loops. Export remains dry 48kHz stereo PCM16. See RELEASE-NOTES-1.6.3.1h.md.

## Current test build 1.6.3-beta 10

After a non-repeating track reaches EOF, the audio becomes silent while the waveform history continues scrolling with silence; the transport remains in its playing state. Repeat still loops at the track boundary. Selecting another track, opening a supported file, or dropping one into the player starts playback. See RELEASE-NOTES-1.6.3-beta-10.md.

### Earlier test notes

## Nintendo Music Studio 1.6.3

The remote can select the 1.6.2 waveform motion. Export creates individual audible channel WAVs or a dry mix; output is 48 kHz stereo PCM16. Recorded loop repeats are supported for VGM/VGZ and supported game audio streams. VGM loop segments containing data blocks and recorded loops in other formats are not yet supported. User audio and sound banks are not included. See RELEASE-NOTES-1.6.3.md for details.

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
