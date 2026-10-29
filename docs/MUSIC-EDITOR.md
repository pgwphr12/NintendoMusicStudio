# Nintendo Music Studio 1.4.2

F2 새 작곡과 F3 기존 곡 편집은 **같은 피아노롤·같은 프로젝트·같은 편집 기능**을 사용합니다. F3은 현재 재생 중인 곡을 음표 가져오기에 전달합니다. 음량/구간만 고치는 별도 화면을 주 편집 기능으로 열지 않습니다.

## 기존 곡을 같은 화면에서 수정

1. F3 또는 편집 화면의 **원곡 음표 가져오기**를 누릅니다. **열기**에서도 `.ngms`, `.nsf`, `.nsfe`를 고를 수 있습니다.
2. 원곡의 하위 트랙·가져올 길이·편집 격자의 BPM을 선택합니다. 변환 후 원곡 음표가 현재 피아노롤의 프로젝트가 됩니다. BPM은 원본에서 자동 복원한 템포가 아닙니다.
3. 음표를 드래그해 이동하고 끝부분을 드래그해 길이를 바꿉니다. 방향키는 선택 음표의 음높이/위치, Shift+좌우는 길이, Ctrl+위아래는 음량, Delete/오른쪽 클릭은 삭제입니다. 빈 곳을 드래그하면 새 음표를 넣습니다. 한 NES 채널의 겹치는 음표는 교체됩니다.
4. 선택 음표의 **음색** 버튼으로 듀티/노이즈 종류를 바꿀 수 있습니다. 채널 볼륨·음소거·템포·마디와 되돌리기도 새 곡과 동일합니다.
5. **저장**은 수정 가능한 `.ngms` 프로젝트입니다. **WAV 내보내기**는 현재 수정된 음표와 음색을 실제로 연주한 48kHz 스테레오32-bit float WAV입니다. 위쪽 **NSF 출력**도 사용할 수 있습니다.

## 지원 범위와 현재 한계

음표 가져오기는 현재 NSF/NSFE의 기본 NES4채널에 한정됩니다. DPCM이나 확장 칩 활동을 만나면 불완전한4채널 곡으로 대신하지 않고 거절합니다. GBA/SPC/GBS 등은 아직 이 공통 피아노롤로 원래 음표·악기를 가져오지 못합니다. 모든 형식 지원 완료가 아닙니다.

가져온 프로젝트는120틱/박자와 음표별 듀티를 저장합니다. 가져오기 전용 에뮬레이터의 출력 버퍼를5ms로 낮춰 상태 관측을 음표 시각에 가깝게 맞춥니다. 게임 드라이버의 프레임 간격, 엔벨로프, 비브라토/슬라이드, 반복음, 원래 악기 정의는 완전히 복원되지 않아 원곡과 차이가 있을 수 있습니다. 원본 트래커 프로젝트 자체를 복원한 것으로 간주하지 않습니다. 원본 파일은 수정하지 않습니다.

기존 버전1 `.ngms`도 열 수 있습니다. 새 버전2는 세밀한 시간과 음표별 음색을 보존합니다. 기본 편집 격자는16분음표이며 가져온 음표는 더 세밀한 시각을 유지합니다. WAV 취소·실패 시 기존 목적 파일을 보존합니다.

GBA 중복 초기화 제거·로딩 중 창 응답 개선과 개별 PCM 표시 기능은1.4.0에서 유지됩니다. [GBA-MIXER.md](GBA-MIXER.md)에 지원 엔진 한계가 있습니다. 메인 재생은 곡 메타데이터 종료 뒤에도 파형을 갱신합니다.

실행 파일은 `NintendoGameMusicViewer.exe` 하나이며 gme.dll이 함께 필요합니다. Visual Studio2022/CMake로 빌드합니다. GPL/LGPL 및 포함 의존성 라이선스를 유지하고 실행본/전체 소스를 별도 ZIP으로 제공합니다. 사용자의 게임 음원은 배포하지 않습니다.


1.4.2 야간 개발 단계: 표준 m4a GBA의 실제 PCM 음표·악기 헤더 메타데이터를 읽는 비공개 검사 경로를 추가했습니다. 공통 프로젝트/샘플 악기 재생 및 GBA 음표 가져오기 UI는 아직 연결하지 않았습니다. F3에서 GBA를 완성된 피아노롤 편집으로 열 수 있다고 표시하지 않습니다. 현재 사용자 기능은 기존 NSF/NSFE 가져오기 범위를 유지합니다.


## 1.4.4 development: common sampled Song foundation

The existing Song type now also represents sampled compositions with1–64 named musical tracks, a bounded owned sample bank, per-note sample selection and polyphony. It is not a separate GBA editor model. NES projects still serialize as version2 and read versions1/2. Sampled projects use strict version3, embedding PCM8 hex with explicit known-Hz sample rate/root pitch/loop metadata, so save/reopen does not depend on a live emulator. Immutable shared sample vectors keep Song/undo snapshots from duplicating the bank. Limits:256 samples,8MiB total PCM,32768 notes,256 simultaneous notes. Changing bank data requires a replacement immutable sample vector.

The common exportSongWav now dispatches sampled Songs to a deterministic block sampler using linear interpolation, original one-shot/loop ranges, note position/length/pitch/velocity/sample, track level/pan/mute and fixed headroom. It retains overlap within the same musical track. Seeking and block sizes produce the same output. Samples are not normalized or clipped to hide level edits. The output is48kHz stereo float32 RIFF with cancellation and atomic replacement. This is an explicit-rate sampler, not original m4a ADSR/reverb/modulation emulation; original GBA frequency/timing conversion has not been verified and is NOT guessed.

Development boundary: GBA event-to-Song recording/import, sample project screen editing/preview and shared piano-roll instrument controls are still unfinished. The existing UI rejects version3 sampled projects before replacing the current project; it must not display a misleading four-channel NES view or compile sample tracks as NSF. Original NSF/NSFE editing/preview remains available. No GBA import completion is claimed. Tests use independently generated eight-sample waveforms, not game assets. Real game sample copies from1.4.3 remain private and are not embedded in public examples or packages.


## 1.4.7 development: common note sound automation

The SAME Song/Note now preserves an initial fractional source-sample offset and immutable per-note piecewise-constant rate/left/right multipliers. Points use relative musical ticks, start at0, and have strictly increasing finite times. Source phase accumulates across changes; a control point is not a new note/sample retrigger. Pitch and instrument edits multiply the sample's base increment; velocity/track gain/pan remain separate user controls. Rate0 explicitly holds source phase. Control gains can exceed1 within bounded headroom; no clipping or automatic normalization hides edits.

Automated sampled projects write NGMS4 with source offsets and all points; flat sampled projects continue to write3, NES continues to write2, and readers accept1/2/3/4. Wave data and immutable banks/controls remain owned across undo snapshots and file reopen. Limits:8192 points per note,262144 per Song,64MiB project text; times may extend beyond a temporarily shortened note up to the512-bar time horizon at the current resolution. Such trailing points remain stored and become audible again when length is extended. Tempo changes stretch musical point times, not sample pitch. Both note and point boundaries quantize to output sample frames; if multiple points land on one frame the last wins. Initial source offset must be inside the selected sample, and switching to a shorter instrument can require explicitly resetting it.

The common streaming renderer precomputes phase-continuous segments, so changing block size or seeking through rate boundaries produces identical samples. The SAME exportSongWav uses those segments, with48kHz float stereo output and existing atomic cancellation. Tests use only independently generated eight-byte waveforms: manually known fractional-phase/stereo output,8saved/reopened musical edits with exact WAV readback, tempo/move/pitch/resize/freeze/one-shot/rounded-boundary behavior, immutable undo, malformed/nonfinite/order/count/range rejection and versions1/2/3 compatibility. This is a generic sampler automation model, not a new GBA editor and not proof of original m4a audible timing/filter/reverb. Real GBA metadata-to-Song import and sampled piano-roll/preview remain unfinished.


# Nintendo Music Studio 1.4.8 — actual GBA common-model import checkpoint7

The backend now imports known standard-m4a forward PCM8 and fixed-rate PCM original logical MIDI notes, musical tracks, owned ROM samples and applied pitch/stereo envelopes into the EXISTING Song/Note/version4 project and common WAV renderer. Polyphony is retained, no forced NES4 mapping or separate editor/model. Fractional source phase is preserved; equal states coalesce without retriggering. Capture is cancellable and bounded by existing bank/track/note/control limits.

Actual read-only Super Mario Advance Overworld six-second test imports87 notes/6 tracks/6 samples/4778 sample bytes/1659 applied snapshots/1201 control points/3 capture-boundary tails. Project save/reopen and six independent pitch/move/length/velocity/instrument/mute edits each produce changed audio and exact edited common-WAV readback. Own metadata tests check known stereo samples, velocity/master/envelope not multiplied twice, original MIDI, fixed/variable rates, lifetime/phase/unsupported failures and cancellation.

Initial nonzero-PSG-stem rejection failed on DAC/filter residue. The preserved failure was replaced by opt-in actual oscillator enabled/DAC/envelope/routing state plus sticky original sequencer CGB START observation, including muted written PSG notes. Own tests cover pulse/noise/AGB75-percent-wave/history/default-off, zero-envelope bias exclusion and original CGB START. Active or written PSG notes are rejected explicitly; no partial PCM-only success. Normal audio remains unchanged and original full-mix comparison is rerun.

This is an internal development release. Sampled piano-roll/Player preview and import dialog are still unconnected; sampled project UI guard remains. Last user-ready1.4.1 and all prior releases preserved. Timeline uses observed emulated mixer-lock times; exact DAC delay, filtering, reverb and original output fidelity remain uncalibrated. Unknown drivers, RAM/compressed/reverse samples, timer1 routing and irregular/discontinuous captures fail. No universal GBA support or listening/hardware validation claim.

Fresh core/compiler/studio/mixer/sample/project/WAV tests and packaged hidden UI use dummy audio; private bank/project/WAV stay in a separate private workspace test folder. Runtime has one EXE; separate complete source ZIP includes changed core, own import and PSG tests, licenses. CRC/SHA and exact-byte own-GSF whitelist exclude user music, banks, libraries and ROMs.


# Nintendo Music Studio 1.4.9 — shared GBA piano roll and preview checkpoint8

Recognized standard-m4a forward ROM PCM8/fixed8 GSF/MINIGSF now imports through the existing cancellable background note-import dialog into the SAME persistent Song/Note editor used for NES import and composition. No standalone app or waveform-analysis substitute. The piano roll supports1–64 sampled musical tracks with paged selection, overlapping notes and MIDI0–127; selected note/track sample selection, note movement/length/pitch/velocity, mute/volume, undo/redo, project3/4 save/reopen and common edited WAV are connected. New NES song and old project1/2 remain supported.

Preview uses a prepared backend around the EXACT SampleSongRenderer used by WAV, with one-pass per-track waveform voices, phase-continuous controls and seek. It never compiles samples as fake NES NSF. Edited sampled music can return to the existing main waveform screen through an in-memory Song snapshot while the persistent editor retains its project. Original playback and edited preview stay distinct. The legacy32-bit mask cannot wrap onto tracks32–63; higher-track mute uses actual per-track voices. A sample change explicitly resets an initial offset if the new sample is shorter.

Paint/dirty checks compare immutable bank/control identities and model fields instead of serializing megabytes on every mouse movement. Bounded project load supports64MiB with validation before replacing the current Song. Hidden own-data tests cover8track editing/sample choice/undo/reopen/preview/WAV/waveform Player handoff, malformed load retention, new-NES channel reset,1.2MiB project and1000 dirty checks. A separate64track test checks exact sampler/preview samples, per-track sum, seek, end, mask no-wrap, real dummy callback/pause/high-track mute/history. Actual private Overworld six-second import uses the production dialog worker and cancellation, edits five musical properties in the same editor, save/reopen/preview/WAV and a private screenshot. These are dummy audio tests, not listening/hardware validation.

Known subset only: GBA PSG notes/activity (including muted sequencer START), custom drivers, RAM/compressed/reverse samples, timer1 routing and irregular captures are rejected. Imported dry mixer-lock timing is not calibrated exact original DAC delay, filtering or reverb. Capture boundaries may truncate tails; import a suitable range. All game formats are not universally editable, and saving a modified GSF/ROM is not implemented. Large project parsing/save and heavy model edits still have synchronous work despite the dirty-check improvement. Main waveform layouts with many tracks may be compact.

1.4.1 and all previous releases are preserved. One-EXE runtime and separate full corresponding source include modified LGPL/GPL core/licenses and own tests, CRC/SHA and exact-byte own-GSF whitelist. Private game music/banks/projects/WAV/screenshots and ROMs remain outside packages.
