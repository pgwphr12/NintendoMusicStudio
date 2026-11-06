# V1.6 업데이트 (V1.5.11 이후 통합)

- 별도 DLL이 필요 없는 단일 EXE로 변경하고 프로그램 안에 라이선스 안내 포함
- 대용량 MIDI 재생 제한 개선 및 트랙 병합 처리 최적화
- RMID 포장 MIDI, 종료 표식이 없는 정상 트랙, 일부 음색 선택·SF2 호환성 개선
- VirtualMIDISynth에서 활성화한 SF2 가져오기 및 사용 중인 음색 파일 이름 표시
- 실행 중 저장된 SF2 변경 자동 반영, 재생 위치·일시정지·음량·음소거 유지
- 새 음색을 불러오지 못하면 기존 음색 유지 및 오류 안내
- Windows 기본 음색에서도 내부 합성을 통해 채널별 파형·효과 사용
- 파형이 없는 외부 MIDI 직접 출력 모드를 별도 항목으로 구분
- MIDI 채널 이름을 Channel 1, Channel 2 형식으로 간소화하고 기존 음량·음소거 설정 유지
- 녹화뷰의 긴 채널 이름 잘림 개선

VirtualMIDISynth 연동은 같은 SF2를 내부 합성하는 방식입니다. SF2 변경 후 적용/확인으로 저장하면 약1초 내 감지하며 로딩 시간은 별도입니다. 큰 음색 전환 중에는 잠시 멈출 수 있습니다. 여러 음색의 적층이나 VirtualMIDISynth 자체 효과를 그대로 재현하는 기능은 아닙니다.

음악·ROM·사운드폰트는 포함하지 않습니다. 공개 시 실행본과 함께 대응 전체 소스와 라이선스 안내도 제공해 주세요.


# 1.5.12h4 — live VirtualMIDISynth soundfont changes

- Watch saved active SF2 changes (~1s detection) and same-path edits without restarting the app.
- Prepare the new bank at the current source position; retain playing/paused state, gain and channel mute.
- Keep old soundfont on missing/corrupt new banks and recover after a valid selection.
- Add F8 automatic-follow toggle; explicit import enables it, manual custom banks disable it.
- Retain explicit SF2 import, channel waveforms and standalone EXE. Large bank loading may pause briefly.

# 1.5.12h3 — explicit VirtualMIDISynth soundfont import

- Add F8 import of currently enabled VirtualMIDISynth SF2 files, with choice among multiple entries.
- Select/persist the imported file for internal synthesis and channel waveforms; restart current MIDI.
- Display actual filename and reject missing config/fonts instead of substituting Windows DLS.
- Preserve original files/config and standalone EXE; no copied/distributed user fonts.

# 1.5.12h2 — Windows default MIDI waveform fix

- Make F8 Windows default bank synthesize internally with channel waveforms/effects.
- Prefer configured VirtualMIDISynth SF2 independently of mapper preferred-ID responses; do not silently use DLS for invalid VMS settings.
- Separate external direct MIDI output and label its lack of waveforms explicitly.
- Show actual bank filename in track information; preserve standalone EXE and large-MIDI support.

# 1.5.12h — MIDI waveform and large-file hotfix

- Default to internal MIDI synthesis; reuse the single enabled SF2 from preferred VirtualMIDISynth #1 automatically.
- Preserve explicit custom banks and optional F8 Windows device output; no double playback.
- Fix the1M MIDI event cap for the3provided dense files; bounded16M capacity and ordered-track heap merge reduce sort work/memory.
- Verify complete render/seek of all3 files and actual channel waveforms with the configured bank.
- Retain standalone EXE and recording-label improvements. No user MIDI/SF2/config modification or redistribution.

# 1.5.12 — MIDI output and standalone EXE

- Link GME into the EXE; no sidecar DLL. Embed full component notices in About.
- Default to Windows MIDI output; F8 switches to internal synthesis/system or custom SF2/DLS.
- Restore MIDI state after pause/seek/preferred device changes; apply MIDI gain/mute and tempo.
- Accept RMID-wrapped MIDI, complete tracks without EOT and metadata-crossing running status; fall back for missing internal bank/program.
- Fit full recording-view channel names to their channel cells.
- Device output does not pass through internal waveform/effects; select internal synthesis for these.

# 1.5.11 — fixed whole-song channel visibility

- Determine optional channel use before playback, including late DMC and ES voices; no first-sound appearance or playback-time hiding.
- Keep console base channels and GBA PCM visible; retain MIDI declared note channels and ordinary mono/stereo.
- Scan complete extended VGM commands and finite tagged NSF/NSFE length+fade; cache tagged subsong results.
- Retain unverified optional channels for unknown lengths or tagged analyses over one hour.
- Preserve pause/mute/seek/repeat lists and existing synthesis, mix, effects and controls.

# 1.5.10 — ES5506 voices and AY8930

- Split ES5506 into32 actual voice lanes plus explicit non-voice bus residual; preserve original mix and serial/interpolation timing.
- Capture one unmuted ES core for all voice lanes; skip unused filters in the main mix path.
- Extend channel mute/activity masks to64bits; use2/3/4columns for large chip combinations.
- Add single AY8930 VGM expanded-mode synthesis,3lanes and stereo-mask command, using pinned BSD3 core.
- Preserve MIDI, custom banks and existing player controls.

# 1.5.9 — MIDI and sound banks

- Add MIDI(.mid/.midi) format0/1, active-channel waveforms and existing playback effects/controls.
- Read Windows default DLS bank; F8 selects a custom SF2/DLS or restores the default. Persist choice.
- Register all20 supported game/audio/MIDI extensions without overwriting Windows defaults. Refresh existing consented registration.
- Include MIT TinySoundFont source/notices and mark the interpolation bounds patch; do not bundle music or banks.

# 1.5.8 — start ordinary audio on open

- Start MP3/WAV/Ogg Vorbis/FLAC immediately after opening, including file dialog, drop and initial-file paths.
- Preserve the separate folder auto-advance setting and manual pause.

# 1.5.7 — recording cell alignment

- End recording waveforms at each channel cell boundary, retaining outer and column gaps.
- Raise recording channel labels and tighten their text bounds.
- Preserve fullscreen repeat scheduling and existing audio behavior.

# 1.5.6 — fullscreen repeat and recording edge

- Service repeat/folder advance from display-paced UI messages, retaining the timer fallback and guarded UI-thread restarts.
- Extend recording waveforms to right screen/column edges; normal layout unchanged.
- Add F11/F9/fullscreen transitions and DS autoplay checks under deliberate idle-timer starvation. Audio/decoder source unchanged from1.5.5.

# 1.5.5 — compatibility, repeat and recording view

- Fix rejection of playable MP3 files whose first bytes do not match the narrow signature check; validate with the decoder.
- Repeat ordinary audio at actual decoded EOF, including overstated duration headers.
- Extend recording-view waveforms left behind channel names, rendering labels above the waves.
- Add actual Repeat-button regressions and later-format/representative game music coverage. Preserve original music/settings.

# 1.5.4 — ordinary audio waveforms

- Add MP3/WAV/Ogg Vorbis/FLAC playback and live stereo/mono waveforms.
- Share existing controls, recording view and mixed ordinary-audio folder navigation.
- Decode files on demand, support Unicode paths/direct seeking; no external codecs needed.
- Preserve game-music playback and prior ES5506 fixes. Add own format/seek/mute/replay/EOF/UI tests.

# 1.5.3 — ES5506 interpolation validation candidate

- Replace sub48kHz ES5506 linear interpolation with32-tap causal band-limited conversion, preserving treble and suppressing imaging.
- Preserve1.5.2 chip levels, ES decoder/filter and other chip paths. Use exact fractional native-rate calculation.
- Own passband/DC/stereo/silence tests and actual VGM comparisons; exact creator timbre remains unverified.

# 1.5.2 — ES5506 level regression fix

- Remove the additional1/8 ES5506 attenuation introduced in1.5.1; restore main melody level.
- Keep actual ES serial output/routing/precision fixes and explicit VGM volume overrides.
- Preserve other chips, existing settings and old releases. Exact reference timbre remains unverified.

# Nintendo Music Studio — changes

# # 1.5.1 —2026-11-02 timbre validation candidate

- ES5506 configured output buses/precision/routing now replace debug voice summation.
- Correct ES default VGM calibration and relative/absolute volume overrides.
- Unsupported serial width errors and disabled-output/word precision tests added.
- Preserve SCC rate/gain fixes, N64, previous files and user settings.
- Full reference-timbre identity remains unverified; compare at pitch/speed1.00/reverb off.


# # 1.5 —2026-11-01 public package

- Public name Nintendo Music Studio1.5; executable NintendoMusicStudio.exe; Windows product/version/dialogs/default-app identity updated.
- Preserves1.4.18 playback code, including SCC native-rate/downsampling and chip-count gain fixes verified against1.4.15.
- Preserves N64 playback/voice taps through licensed Cog-pinned lazyusf2 and the licensed VGM replacements.
- Preserves independent pitch/speed0.01 steps,76%/3.0s reverb defaults, bulk channel gain, recording view and modeless F1 fix.
- English channel labels, existing GBA voice/visibility and monitor-paced waveform behavior retained. Universal GBA/other-chip support is not claimed.
- Player-only: composition/editing/project/WAV-export paths remain removed.
- Complete matching source, license notices, rebuild/relink instructions, SHA256 manifests and bounded/self-authored regression tests included.
- Legacy settings location retained to preserve prior preferences. Older releases/original music are preserved.

# # 1.4.18 — sound regression correction

SCC generation at clock/16 was incorrectly treated as48kHz in16/17, lowering pitch; fixed native-rate conversion and native-cell interval averaging. Removed arbitrary sqrt(chip-count) mix attenuation. Whole supplied VGM compared to15: SCC correlation0.9999999994, whole mix RMS difference~0.05%. Added pitch/silent-extra-chip gain regression checks.

# # 1.4.17 — N64 license replacement

Replaced old Project64-restricted N64 tree/archive with specifically GPL-declared official Cog lazyusf2 source, preserving original exceptions and read-only voice taps. N64 remains enabled.

# # 1.4.16 — VGM license replacements

Replaced unassigned VGM framework/resampler with independent app code, extra undeclared YM2612 with LGPL Nuked OPN2, and old noncommercial ES5506 with zlib vgsound_emu. The16/17sound regression was corrected in18.

# # 1.4.15 and earlier player work

Independent pitch/speed, settings/default adjustments, bulk channel gain, recording clock/hidden volume controls, modeless F1, additional tested VGM chips, GBA inspection/individual PCM voices and presentation improvements. The previous editing/composition implementation was removed during the player refocus.
