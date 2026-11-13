## 1.6.3-beta 8 — continuous visuals and low-clock OPL

Stop/pause drains the last waveform to silence. Horizontal history runs in all views without transport/style resets. Restored beta7 channel layout and added saved waveform-spacing zoom in the F12 remote (0.5×–8×). YM3526 native sample cadence preserves low-clock held texture; reference-video comparison retained as software evidence, not perfect Furnace/hardware parity.

## 1.6.3-beta7 — paired MIDI banks

Prefer exact localDLS,thenSF2,thenVMS;DLS2PCM region/articulation support; protect local banks from live VMS changes; allow valid note-less MIDI as silent tracks.85NSMB files verified. Beta6 smooth rendering retained.

## 1.6.3-beta6 smooth-render revision

Reduced matching/history cost, cached outlined labels, axis-aligned raster spans and time-based waveform transitions. No audio changes. Same user-requested beta6 label.

## 1.6.3-beta 6 — corrected label and waveform / GBA PSG revision

Supersedes the mistakenly named beta7 candidate; same label requested by user. Shape-matched waveform, conservative MP2K PSG exclusion with PCM retained. All beta7 visibility/remote changes included.

# Nintendo Music Studio1.6.3-beta 7

- AY8930·SN76489도 기본 채널 강제 표시 없이 사용 기록으로 표시합니다.
- 신고한1793424975458.vgm의 YM2612 1·2·3번은 실제 노트 기록이 없는데도 기본 채널 강제 표시 규칙으로 남아 있었습니다. YM2612 FM/DAC 사용 명령을 곡 전체에서 확인하도록 수정했습니다.
- 처음 몇 초를 무조건 제외하지 않습니다. 시작부터 나오는 짧은 실제 음은 유지하고, 뒤에서 쓰는 채널도 처음부터 표시합니다. 현재 무음·음소거·볼륨0으로 목록이 변하지 않습니다.
- 기본 채널 유지 예외는 NES의5개 APU 채널, Game Boy의4개, SNES의8개입니다. 다른 형식의 고정 표시 예외는 제거했습니다.
- 곡 전체 사용 정보나 확정된 데이터 끝이 있으면 이를 기준으로 숨깁니다. 단순 길이 태그만 있는 끝없는 실행형 음원·ROM, 분석 실패·시간 제한 등으로 미사용을 확정할 수 없으면 추측으로 숨기지 않습니다.
- 리모컨에 **모든 채널 표시** 체크 옵션을 추가했습니다. 켜면 자동·수동 숨김을 잠시 해제하며, 끄면 기존 숨김 상태로 돌아갑니다. 설정은 저장되고 음량·음소거는 바뀌지 않습니다.
- 베타6의 자유롭게 움직이는 파형, 녹화뷰 채널 이름과 별도 F12 리모컨은 유지합니다.

테스트본입니다. 모든 게임 드라이버의 사용 정보를 완전히 판정하는 단계는 아니며, 미확정 파일은 채널을 보존합니다. 표시 정책 수정이며 원본 음악·재생 소리를 임의로 제거하지 않습니다.


# Nintendo Music Studio1.6.3-beta 6

- YM3812/YM3526는 곡 전체의 사용 기록을 미리 읽어 미사용 채널을 숨깁니다. 뒤에서만 사용되는 채널도 처음부터 표시하며, 현재 무음·음소거·음량0이라는 이유로 숨기지 않습니다. 기존 NES/GB 기본 채널·GBA PCM 유지 규칙은 보존합니다.

- 최신 요청에 따라 파형의 왼쪽 끝이나 가운데를 중앙선에 고정하는 정렬을 제거했습니다. 실제 재생 구간 그대로 왼쪽 끝과 가운데가 자유롭게 변합니다.
- 소수 샘플 보간과 화면 픽셀 구간 적분은 유지합니다. 별도 가상 흔들림이나 고정된 파형 반복을 넣지 않았으며, 일시정지하면 파형도 멈춥니다.
- 정렬용 추가 과거 구간이 필요 없어 최대 표시 폭에서도 화면용 복사를4096샘플로 줄였습니다.
- 녹화뷰는 채널 이름을 유지하고 제목·시간·음량·상태·격자는 숨깁니다. 일반 화면과 F12 리모컨도 유지합니다.
- 재생 음질·음원 데이터·피치·속도와 기존 MIDI/VGM 호환 수정은 유지합니다.

테스트본입니다. 중앙 고정 방식 대신 자연스럽게 흐르는 방식이므로, 재생 중 파형이 이동하고 모양이 달라지는 것은 의도된 동작입니다.


# Nintendo Music Studio1.6.3-beta 5

- 녹화뷰는 채널 이름은 유지하고 곡 제목·시간·음량·상태 안내·격자를 숨깁니다. 파형과 채널 이름만 표시합니다. 위아래 빈 정보 공간도 제거했습니다. 일반 화면과 독립 F12 리모컨은 유지합니다.
- 파형이 계속 왼쪽으로 밀리거나 덜덜 떨리는 표시를 개선했습니다.
- 0만 기준으로 찾던 정렬을 파형의 중심과 히스테리시스 기준으로 바꾸고, 소수 샘플 위치까지 맞춥니다. 한쪽으로 치우친 칩 출력과 낮은 음에도 적용됩니다.
- 표시 폭4096에서도 정렬용 이전 구간을 확보합니다. 프레임마다 달라지는 평균값으로 파형이 위아래로 흔들리는 현상도 줄였습니다.
- 샘플 극값을 보존하며 화면용 선을 보간합니다. 무음은 평평하게, 소음과 변화하는 음은 계속 갱신합니다. 화면을 고정하거나 지난 파형을 반복해서 보여주는 방식은 아닙니다.
- 재생 샘플·음질·피치·속도·모니터 주사율에 따른 갱신은 유지합니다. MIDI 기본 음원과 Furnace VGM 호환 수정도 유지합니다.

테스트본입니다. 여러 악기가 섞이거나 음색·음높이가 변하는 신호는 원래 파형도 달라지므로 모든 음악을 정지 그림처럼 고정하는 기능은 아닙니다.


# Nintendo Music Studio1.6.3-beta 4

- VirtualMIDISynth가 없는 PC에서 Windows 기본 MIDI 음원을 찾는 처리를 보강했습니다.
- 예전 음원 경로가 없거나 손상되어 있어도 다른 Windows 기본 경로의 gm.dls를 확인합니다. 32/64비트 등록 경로와 환경변수가 들어간 경로도 처리합니다.
- 기본 음원으로 내부 합성하므로 채널별 파형과 플레이어 효과를 유지합니다. VirtualMIDISynth가 없을 때 F8/음원 선택 버튼을 제한하는 기존 동작도 유지합니다.
- 기본 음원이 모두 없거나 읽을 수 없을 때는 MIDI 파일 자체의 형식 오류와 구분되는 안내를 표시합니다. Windows 음원이나 사용자 사운드폰트를 배포본에 넣지 않습니다.
- beta3의 Furnace VGM/ES5506 음정 호환 처리는 유지합니다.

이번 신고의 정확한 오류 문구는 확인하지 못했습니다. 재현 가능한 기본 음원 경로 실패를 수정한 테스트본이며, 신고 PC에서의 최종 확인은 남아 있습니다. 기본 음원 파일 자체가 없는 Windows 환경에는 이 파일을 생성하거나 대체 음원을 몰래 설치하지 않습니다.


# Nintendo Music Studio1.6.3-beta 3

- `slogic.vgm`의 **VGM1.72 / YM3526(OPL)** 재생을 추가했습니다. 기록된335,582Hz 클록을 그대로 사용합니다.
- `1794124986077.vgm`의 **SN76489 + ES5506 조합**을 지원합니다. 이 파일도 확인된 Furnace 초기화 규칙에 따라 ES5506 속도를 처리합니다.
- 두 실제 파일을 각각 끝까지 렌더링하고 채널 합산·전체 음소거·되감기·탐색을 검사했습니다. 원본 파일과 사용자 설정은 덮어쓰지 않습니다.
- 특정 **Furnace에서 내보낸 VGM의 ES5506 음정이 한 옥타브 낮아지는 차이**를 수정했습니다.
- Furnace는 설정된 채널 수로 내부 재생 속도를 정하고, 초기화 중32채널 값을 추가로 기록합니다. 이전 내보내기에서는 이 초기화 쌍이 잘못된 레지스터 페이지에 남는 경우도 있습니다. 해당 VGM을 하드웨어 레지스터 값만으로 재생하면 Furnace 원본과 속도가 달라집니다.
- **Furnace 제작 태그와 확인된 초기화 순서가 모두 일치**할 때만 원래 설정의 속도를 재현합니다. 일반 ES5506에 일괄 +12반음을 적용하지 않습니다. 원본 VGM 파일도 수정하지 않습니다.
- 제공된 로비 VGM의 ES5506은32채널 속도31,250Hz 대신 원래16채널 속도62,500Hz로 처리합니다. VGM 이벤트 시간·곡 템포·다른 칩은 유지합니다.
- 8/16비트 샘플·일반 파일 음정 유지·채널 합산·음소거·탐색을 검사했습니다. 실제 로비 VGM 앞10초에서 ES5506 음정 변화와 YM2612/OKI 출력 불변을 확인했습니다.
- F12 리모컨, 반향 왼쪽 버튼, 녹화뷰 안내 숨김 및 채널 숨김 기능은 유지합니다.

수정 근거: https://github.com/tildearrow/furnace/blob/592e7fd387e7bb5f0c908452c60632c8d37cdc39/src/engine/platform/es5506.cpp (reset/setFlags)

현재는 테스트본입니다. 원작자의 Furnace 프로젝트/WAV와 전체 음색이 완전히 동일하다는 검증은 아직 아닙니다. 피치·속도1.00×에서 확인하세요.


# Nintendo Music Studio1.6.3-beta 2

- **리모컨을 녹화뷰에서 분리**했습니다. F9는 녹화뷰만 전환하며 리모컨을 자동으로 열거나 닫지 않습니다.
- **F12**로 리모컨을 열고 닫습니다. 리모컨·EQ·종류 선택창·숨긴 채널·정보 창에 포커스가 있어도 작동합니다. F10의 기존 기본 앱 설정 기능은 유지합니다.
- 일반 화면의 **반향 버튼 바로 왼쪽**에 **리모컨 (F12)** 버튼을 추가했습니다.
- 녹화뷰에서는 리모컨 버튼과 하단 단축키 안내를 숨깁니다. F12는 녹화뷰에서도 사용할 수 있습니다.
- 리모컨의 녹화뷰 켜기/끄기 버튼은 보기 모드만 바꾸고 리모컨을 닫지 않습니다. X·Esc·F12로 리모컨을 닫아도 녹화뷰 상태는 유지됩니다.
- 녹화뷰로 시작하더라도 리모컨은 자동으로 나타나지 않습니다. 리모컨 표시 상태는 앱을 재실행할 때 복원하지 않습니다.
- 테스트 버전 표기를 **1.6.3-beta 2**처럼 `-beta 번호` 형식으로 변경했습니다. 기존 파일은 보존합니다.

채널 숨김·복원, 소리·EQ·재생 기능은 유지됩니다. 이번 파일은 테스트본이며 정식1.6.2는 보존합니다.


# Nintendo Music Studio1.6.3-1 beta

- **채널 영역을 우클릭하면 해당 채널을 숨깁니다.** 일반 화면과 F9 녹화뷰의 채널 파형 모두 지원합니다.
- **F9 리모컨 → 숨긴 채널 (개수)…**에서 목록을 확인하고 선택 채널·전체 채널을 복원할 수 있습니다. 목록에서 더블클릭해도 복원합니다.
- 숨김은 화면에만 적용합니다. 소리·음소거·채널 음량·공통 음량은 바꾸지 않습니다.
- 같은 시스템·채널 이름의 숨김 설정을 저장하고 곡 이동·정지·재생·앱 재실행 후에도 유지합니다. 같은 이름이 중복되면 발생 순서로 구별하며 다른 시스템에는 적용하지 않습니다.
- 직접 숨긴 채널의 복원은 기존 미사용 채널 자동 숨김을 강제로 해제하지 않습니다. 스펙트럼은 전체 좌·우 출력이므로 개별 채널 숨김 대상이 아닙니다.
- 전부 숨겨도 재생은 계속되며 리모컨에서 되돌릴 수 있습니다.

정식1.6.2는 보존합니다. 이번 파일은 새 기능 테스트본1.6.3-1 beta입니다.


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

