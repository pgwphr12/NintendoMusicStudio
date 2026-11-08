# Nintendo Music Studio 1.6.2

- 지원되는 GBA/NES ROM에서 곡을 선택해 직접 재생할 수 있습니다. 검증된 슈퍼 마리오 어드밴스1~4의 개별 PCM 음성 표시를 개선했습니다.
- 마리오 어드밴스1의 특정 곡을 선택할 때 종료되던 문제를 수정했습니다.
- GBS에 GBA의 GB 호환 음원 모드를 적용했습니다.
- 모든 채널 음량의 공통 저장·우선 적용을 추가했습니다. 현재 곡의 채널별 음량이 서로 다른 **혼합 상태는 정지 후 재생하거나 탐색해도 유지**합니다. 다른 곡을 고르면 저장한 공통 음량을 우선 적용합니다.
- F9 녹화뷰용 별도 리모컨을 추가했습니다.
- MesenCE 방식의20밴드 EQ와 정수·직접 숫자 입력을 지원합니다.
- 파형 외에 **막대형·선형·가로흐름 스펙트럼**을 추가했습니다. F7 선택창에서 바로 고르며 기본 종류는 가로흐름입니다. F6로 파형/스펙트럼을 전환합니다.
-16:9 가로흐름은 위 Left·아래 Right의 두 줄로 표시합니다. 이미지 기능과 입체 워터폴은 제거했습니다.
- DLL을 따로 설치할 필요 없는 단일 EXE와 전체 대응 소스를 제공합니다.

ROM 직접 재생은 검증된 게임/판본에 한정되며 모든 ROM을 지원하는 것은 아닙니다. 게임·음악·ROM·사운드폰트는 포함하지 않습니다.


# Nintendo Music Studio 1.6.2-12 — 테스트본

- 입체 **워터폴을 제거**했습니다. 막대형·선형·가로흐름 세 종류만 제공합니다.
- **F7 / 상단 종류 버튼 / F9 리모컨의 종류 버튼**이 이제 **선택창**을 엽니다. 원하는 종류를 바로 고르면 즉시 적용됩니다. 여러 번 눌러 순환할 필요가 없습니다.
- 선택창을 열어도 음악과 메인 창을 계속 사용할 수 있습니다. 이미 열린 선택창은 재사용하고 닫기·Esc로 숨깁니다.
- 종류의 **기본값은 가로흐름**입니다. 사용자가 저장한 유효한 종류는 유지합니다. 이전 워터폴 설정은 가로흐름으로 바꿉니다.
- **F6**의 채널 파형/스펙트럼 전환,16:9 가로흐름과 녹화뷰를 유지합니다. 이미지 기능은 없습니다.

정식 버전1.6.2를 준비하는 테스트본입니다.


# Nintendo Music Studio 1.6.2-11 — 테스트본

- 영상의 구도를16:9로 보고 **가로흐름 스펙트로그램**을 추가했습니다. 위쪽Left·아래쪽Right 두 영역에서 새로운 무늬가 오른쪽에 나타나고 과거 소리는 왼쪽으로 흐릅니다.
- 세로축은20Hz~20kHz 로그 주파수, 밝기는 실제 내부 출력의 세기입니다. 저음은 아래, 고음은 위입니다. 기존 입체 워터폴과 다른 종류입니다.
- **F6**: 파형/스펙트럼. **F7**: 막대형 → 선형 → 워터폴 → 가로흐름. 상단 버튼/F9 리모컨에서도 선택하고 종류를 저장합니다.
- 약12.8초의 시간 기록을 제한된 메모리에 보관하고 화면 주사율에 맞춰 위치를 갱신합니다. 일시정지는 기록을 멈추고 곡 변경·탐색은 기록을 비웁니다. 긴 화면 중단 구간의 소리를 임의로 채우지 않습니다.
- 이미지 기능은 제거한 상태입니다. 기존 재생·채널 파형·EQ·반복 기능을 유지합니다.

현재는 테스트본이며 정식 버전은1.6.2가 목표입니다. 실제 모니터/VRR·물리 오디오 청감 전체를 검증했다는 의미는 아닙니다.


## 현재 테스트본 1.6.2-8

## 1.6.2-10 (2026-11-08, 테스트)

이미지 기능 제거. 막대형·선형·워터폴을F7과녹화리모컨에서선택하며위아래두줄표시와종류저장을지원합니다. 워터폴실제분석기록32개/시간기반갱신·소멸/곡·탐색초기화검증.


## 1.6.2-9 (2026-11-08, 테스트)

이미지 중앙 배치+좌우 스펙트럼, 무이미지 가로 두 줄, F6/F7/Delete 및 녹화 리모컨 조작, 이미지·보기 복원, 실제 스테레오 출력 FFT/시간 기반 움직임을 추가했습니다.

EQ를 MesenCE의 실제 필터·정수 조절·직접 숫자 입력 방식으로 맞췄습니다. 동일48kHz PCM 입력에서 원본 기준과 출력 샘플 차이0을 확인했습니다. [변경 사항](RELEASE-NOTES-1.6.2-8.md)에 비교 범위·원본 특성·라이선스를 정리했습니다. 아래는 이전 기록입니다.

## 현재 테스트본 1.6.2-7
하단 EQ/반향 버튼 겹침을 수정하고 표시·클릭 영역을 분리했습니다. [변경 사항](RELEASE-NOTES-1.6.2-7.md). 아래 이전 기능 기록은 보존합니다.

## 현재 테스트본 1.6.2-6
모든 채널 음량의 공통 저장·우선 적용, F9 별도 리모컨,40Hz~15kHz20밴드 EQ를 추가했습니다. [변경 사항](RELEASE-NOTES-1.6.2-6.md)에 사용법과 검사 범위를 정리했습니다. 아래는 이전 기록입니다.

## 현재 테스트본 1.6.2-5
마리오 어드밴스 1의 특정 곡 선택 시 종료되는 문제를 수정했습니다. 확인된 PCM 슬롯을 고정 유지하고 곡 전환 시 채널 설정과 버퍼 범위를 검사합니다. 자세한 내용은 [변경 사항](RELEASE-NOTES-1.6.2-5.md)을 참고하세요. 아래는 이전 기록입니다.

## 현재 테스트본 1.6.2-4
GBA ROM 개별 PCM 음성 분리 누락 수정 및 GBS의 GBA GB 호환 음원 모드를 추가했습니다. 상세 범위와 한계는 [변경 사항](RELEASE-NOTES-1.6.2-4.md)을 참고하세요. 이전 버전 기록은 아래에 보존합니다.

# Nintendo Music Studio 테스트본 1.6.2-3

- NES ROM(.nes) 음악 재생을 추가했습니다. 슈퍼 마리오 브라더스1 계열의 검증한 NROM32 프로그램49종을 지원합니다. 모든 NES 게임 지원은 아닙니다.
- 원본 드라이버의 배경음악·이벤트 음악14개 선택 항목을 Tracks에 표시합니다. 동일 악보를 참조하는 항목/짧은 징글을 포함하며 정지 명령은 제외합니다.
- 원래6502 사운드 코드를 메모리에서 실행하고 NTSC/PAL 판본의 주기를 구분합니다. ROM·게임 화면·세이브를 실행하거나 수정하지 않습니다.
- 기본4APU 채널은 항상 표시하고, 원래DAC 출력을 사용하는 곡은 DMC 채널도 처음부터 표시합니다.
- 새 NES 경로의 탐색은 원래 드라이버 상태를 재생해 복원합니다. 긴 위치로 이동할 때 시간이 더 들 수 있습니다. 기존 NSF 탐색은 변경하지 않았습니다.
- 기존 GBA13개 프로필은 유지합니다. 원본 ROM/음악은 테스트 파일에 포함하지 않습니다.

# Nintendo Music Studio 테스트본 1.6.2-2

- 슈퍼 마리오 어드밴스2·3·4 ROM 음악 재생을 추가했습니다. 검증한9개 판본을 더해 총13개 판본을 지원합니다.
- 2편48개,3편40개,4편57개 원본 곡 번호를 표시합니다. 징글과 같은 시퀀스를 참조하는 별도 ID도 포함합니다.
- Nintendo 전용 음악 드라이버의 초기화·곡 선택·프레임 갱신을 원본 ROM에서 실행합니다. 원본 플래그/데이터를 그대로 두며 MIDI나 추측한 악기로 바꾸지 않습니다.
- 원본 읽기 전용,6개 하드웨어 채널(4PSG+PCM A/B),기존1편/GSF/MIDI/오디오 기능은 유지합니다. 악기별 PCM 분리나 모든 ROM 지원이 완성됐다는 뜻은 아닙니다.
- 테스트 번호만 올렸으며 대상 정식 버전은1.6.2입니다.

# Nintendo Music Studio 1.6.2-1 테스트본

- GBA ROM 직접 열기와 곡 선택 재생을 추가했습니다. 첫 지원은 검증된 슈퍼 마리오 어드밴스 1의 4개 ROM 판본입니다.
- 원본 곡 번호로319~321개 항목을 표시하며 배경음악과 효과음을 포함합니다. 처음에는 Song170을 재생합니다.
- 원래 ROM 음악 드라이버를 에뮬레이션하며 재생·탐색·채널 파형·음량·음소거를 지원합니다. 이 경로는4PSG+PCM A/B의6하드웨어 채널입니다.
- NES/GB/SNES 및 다른 GBA 게임의 ROM 직접 재생은 아직 지원하지 않습니다. 기존 GSF 등 음악 파일 지원은 유지합니다.
- ROM을 수정하거나 추출 파일을 저장하지 않으며, ROM·음악 데이터·BIOS는 배포하지 않습니다.

# V1.6.1h 핫픽스

- VirtualMIDISynth 미설치 PC에서는 Windows 기본 음색만 사용
- 미설치 시 음색 버튼을 기존 스타일의 회색 Windows 기본 표시로 비활성화하고 F8도 제한
- 기존에 저장된 SF2/외부 MIDI 선택은 미설치 실행에서 무시하며 기본 파형·효과 유지
- 설치된 PC의 기존 사운드폰트 기능은 유지

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
