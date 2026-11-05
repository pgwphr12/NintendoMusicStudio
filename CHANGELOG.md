#1.5.11 — fixed whole-song channel visibility

- Determine optional channel use before playback, including late DMC and ES voices; no first-sound appearance or playback-time hiding.
- Keep console base channels and GBA PCM visible; retain MIDI declared note channels and ordinary mono/stereo.
- Scan complete extended VGM commands and finite tagged NSF/NSFE length+fade; cache tagged subsong results.
- Retain unverified optional channels for unknown lengths or tagged analyses over one hour.
- Preserve pause/mute/seek/repeat lists and existing synthesis, mix, effects and controls.

#1.5.10 — ES5506 voices and AY8930

- Split ES5506 into32 actual voice lanes plus explicit non-voice bus residual; preserve original mix and serial/interpolation timing.
- Capture one unmuted ES core for all voice lanes; skip unused filters in the main mix path.
- Extend channel mute/activity masks to64bits; use2/3/4columns for large chip combinations.
- Add single AY8930 VGM expanded-mode synthesis,3lanes and stereo-mask command, using pinned BSD3 core.
- Preserve MIDI, custom banks and existing player controls.

#1.5.9 — MIDI and sound banks

- Add MIDI(.mid/.midi) format0/1, active-channel waveforms and existing playback effects/controls.
- Read Windows default DLS bank; F8 selects a custom SF2/DLS or restores the default. Persist choice.
- Register all20 supported game/audio/MIDI extensions without overwriting Windows defaults. Refresh existing consented registration.
- Include MIT TinySoundFont source/notices and mark the interpolation bounds patch; do not bundle music or banks.

#1.5.8 — start ordinary audio on open

- Start MP3/WAV/Ogg Vorbis/FLAC immediately after opening, including file dialog, drop and initial-file paths.
- Preserve the separate folder auto-advance setting and manual pause.

#1.5.7 — recording cell alignment

- End recording waveforms at each channel cell boundary, retaining outer and column gaps.
- Raise recording channel labels and tighten their text bounds.
- Preserve fullscreen repeat scheduling and existing audio behavior.

#1.5.6 — fullscreen repeat and recording edge

- Service repeat/folder advance from display-paced UI messages, retaining the timer fallback and guarded UI-thread restarts.
- Extend recording waveforms to right screen/column edges; normal layout unchanged.
- Add F11/F9/fullscreen transitions and DS autoplay checks under deliberate idle-timer starvation. Audio/decoder source unchanged from1.5.5.

#1.5.5 — compatibility, repeat and recording view

- Fix rejection of playable MP3 files whose first bytes do not match the narrow signature check; validate with the decoder.
- Repeat ordinary audio at actual decoded EOF, including overstated duration headers.
- Extend recording-view waveforms left behind channel names, rendering labels above the waves.
- Add actual Repeat-button regressions and later-format/representative game music coverage. Preserve original music/settings.

#1.5.4 — ordinary audio waveforms

- Add MP3/WAV/Ogg Vorbis/FLAC playback and live stereo/mono waveforms.
- Share existing controls, recording view and mixed ordinary-audio folder navigation.
- Decode files on demand, support Unicode paths/direct seeking; no external codecs needed.
- Preserve game-music playback and prior ES5506 fixes. Add own format/seek/mute/replay/EOF/UI tests.

#1.5.3 — ES5506 interpolation validation candidate

- Replace sub48kHz ES5506 linear interpolation with32-tap causal band-limited conversion, preserving treble and suppressing imaging.
- Preserve1.5.2 chip levels, ES decoder/filter and other chip paths. Use exact fractional native-rate calculation.
- Own passband/DC/stereo/silence tests and actual VGM comparisons; exact creator timbre remains unverified.

#1.5.2 — ES5506 level regression fix

- Remove the additional1/8 ES5506 attenuation introduced in1.5.1; restore main melody level.
- Keep actual ES serial output/routing/precision fixes and explicit VGM volume overrides.
- Preserve other chips, existing settings and old releases. Exact reference timbre remains unverified.

# Nintendo Music Studio — changes

## 1.5.1 —2026-11-02 timbre validation candidate

- ES5506 configured output buses/precision/routing now replace debug voice summation.
- Correct ES default VGM calibration and relative/absolute volume overrides.
- Unsupported serial width errors and disabled-output/word precision tests added.
- Preserve SCC rate/gain fixes, N64, previous files and user settings.
- Full reference-timbre identity remains unverified; compare at pitch/speed1.00/reverb off.


## 1.5 —2026-11-01 public package

- Public name Nintendo Music Studio1.5; executable NintendoMusicStudio.exe; Windows product/version/dialogs/default-app identity updated.
- Preserves1.4.18 playback code, including SCC native-rate/downsampling and chip-count gain fixes verified against1.4.15.
- Preserves N64 playback/voice taps through licensed Cog-pinned lazyusf2 and the licensed VGM replacements.
- Preserves independent pitch/speed0.01 steps,76%/3.0s reverb defaults, bulk channel gain, recording view and modeless F1 fix.
- English channel labels, existing GBA voice/visibility and monitor-paced waveform behavior retained. Universal GBA/other-chip support is not claimed.
- Player-only: composition/editing/project/WAV-export paths remain removed.
- Complete matching source, license notices, rebuild/relink instructions, SHA256 manifests and bounded/self-authored regression tests included.
- Legacy settings location retained to preserve prior preferences. Older releases/original music are preserved.

## 1.4.18 — sound regression correction

SCC generation at clock/16 was incorrectly treated as48kHz in16/17, lowering pitch; fixed native-rate conversion and native-cell interval averaging. Removed arbitrary sqrt(chip-count) mix attenuation. Whole supplied VGM compared to15: SCC correlation0.9999999994, whole mix RMS difference~0.05%. Added pitch/silent-extra-chip gain regression checks.

## 1.4.17 — N64 license replacement

Replaced old Project64-restricted N64 tree/archive with specifically GPL-declared official Cog lazyusf2 source, preserving original exceptions and read-only voice taps. N64 remains enabled.

## 1.4.16 — VGM license replacements

Replaced unassigned VGM framework/resampler with independent app code, extra undeclared YM2612 with LGPL Nuked OPN2, and old noncommercial ES5506 with zlib vgsound_emu. The16/17sound regression was corrected in18.

## 1.4.15 and earlier player work

Independent pitch/speed, settings/default adjustments, bulk channel gain, recording clock/hidden volume controls, modeless F1, additional tested VGM chips, GBA inspection/individual PCM voices and presentation improvements. The previous editing/composition implementation was removed during the player refocus.
