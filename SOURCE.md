## Test1.6.3-beta 9 corresponding source

`Registry.cpp` signature-detects Wii RSTM/BRSTM, BNS (including IMD5-wrapped BNS), and CRI ADX, then routes them through the existing `StreamBackend`/vgmstream. `FolderPlaylist.cpp` includes `.brstm`, `.bns`, and `.adx`. The F12 remote provides global and per-file/per-track/per-channel waveform spacing, a common-value reset, and a saved metadata-length continuation toggle. True decoder EOF remains authoritative. No new dependency, disc extraction, or user assets.

# Test1.6.3-beta 8 corresponding source

WaveformTail, common UI spectrum servicing, beta7 channel geometry restoration and remote-controlled waveform zoom, native-rate OPL1 adapter, focused unit/native UI tests and reference-render helper. No new dependency. User music and downloaded reference audio remain excluded.

# Test1.6.3-beta 7 corresponding source

YM usage tracks timed key slots, DAC enable/data and actual stream starts; setup/reset alone is not note use. WholeTrackUsage wraps backends with known complete metadata or physical end boundaries, unmuted offline scans, reset/cache, a10s budget and conservative unknown fallback. PSF length tags alone do not prove an execution end. Remote option and console exceptions affect visibility only. No new dependency; all new source/tests included.

# Test1.6.3-beta 7 corresponding source

YM3812/YM3526 primary and auxiliary channels now use a precomputed whole-song key/rhythm usage mask. Late notes are visible before playback; silence/mute/gain do not alter visibility. Decoder channels and real audio remain intact.

Envelope.cpp no longer searches or pins crossings. It uses the newest real window and existing piecewise-linear pixel integration. Window.cpp returns to4096 display history frames; no need for extra trigger history. Decoder/mixer/output CPP code is unchanged. WaveStabilityTests.cpp now verifies free left/middle movement, actual newest offsets and deterministic pause rather than constant phase.

# Test1.6.3-beta 5 corresponding source

Owned Envelope.cpp/hpp implement stateless fractional trigger/raster sampling. Player.hpp exposes an optional display history length; Window.cpp requests additional headroom only at the maximum scope width and uses matching DC scale. Audio decode/mix/output implementations are unchanged. WaveStabilityTests.cpp compares with the owned previous display implementation in tests/LegacyEnvelope.cpp, which is test-only and not linked into the player.

# Test1.6.3-beta 4 corresponding source

MidiBackend.cpp validates Windows default DLS candidates before synthesis, expands registry paths from both views, and falls through to standard Windows locations. Isolated dummy-only test inputs simulate broken registrations without modifying the host registry or installing/removing drivers. MidiTests.cpp adds six fallback/diagnostic cases. Complete code and licenses are supplied; system gm.dls is excluded.

# Test1.6.3-beta 3 corresponding source

The existing GPL2+ OPL core now also builds YM3526. An unchanged, pinned BSD3 SN76496 core plus own functional configuration declarations supports mixed extended-chip files. Exact provenance, full source, and notices are bundled. FurnaceChipTests.cpp contains owned tone/stereo/negative fixtures; supplied music is excluded.

Owned FurnaceEsRate.hpp recognizes only tagged reset sequences. ExtendedVgmBackend.cpp passes the derived tracker divider to LicensedChips.cpp before first synthesis. The vendored ES5506 core is unchanged; no legacy MAME code is included. Native reference tools/media stay private and excluded. EsPitchTests.cpp generates its own8/16bit ROM fixtures. Upstream rationale and verification appear in docs/ES-PITCH-1.6.3-beta-3.json. Full source/licenses/build scripts remain included.


# Test1.6.3-beta 2 corresponding source

Owned native remote visibility is independent of recording state. F12 is forwarded only from known application tool windows; autorepeat is ignored. Footer remote button uses matching paint/hit bounds. Tool windows are explicitly destroyed at main teardown. Audio/player/decoder sources remain unchanged. Full source and notices accompany this beta.


# Test1.6.3-1 beta corresponding source

New owned src/ui/HiddenChannels.cpp integrates display-only preferences and a native modeless restore list. Audio/player/decoder code is unchanged from stable1.6.2. Both CMake UI targets include the new file. Full source/notices/tests accompany the standalone beta. No new dependencies, user media or settings are redistributed. Previous build details follow.


# Nintendo Music Studio1.6.2 — stable corresponding source

Same source tree as test12 plus current-song mixed-gain retention and release identity/docs. Complete dependencies, notices, owned tests and build scripts are included. Test labels going forward must include beta; see docs/VERSIONING.md. Historical build instructions/records follow.


## Test1.6.2-12 corresponding source

Owned native choice window in ExtraWindows.cpp replaces type cycling. Perspective-history storage/rendering removed; SpectrumCurve.hpp retains only the bounded line interpolation helper. Existing spectrum type IDs0/1/3 stay compatible; retired2 migrates to3. No audio/decoder/FFT/history backend changes. Full sources, tests and notices included; no new third-party dependencies or user assets.


## Test1.6.2-11 corresponding source

Owned SpectrogramHistory.hpp and SpectrumView.cpp implement the bounded scrolling display. Spectrum.hpp adds a256bin log-frequency projection of the same FFT; original72band mapping and audio callback/decoder code remain unchanged. No video, reference frames, user music, ROM, bank or settings are included. New test/evidence files are bundled; no additional third-party dependency.


## 현재 테스트본 1.6.2-8
EQ를 MesenCE의 실제 필터·정수 조절·직접 숫자 입력 방식으로 맞췄습니다. 동일48kHz PCM 입력에서 원본 기준과 출력 샘플 차이0을 확인했습니다. [변경 사항](RELEASE-NOTES-1.6.2-8.md)에 비교 범위·원본 특성·라이선스를 정리했습니다. 아래는 이전 기록입니다.

## 현재 테스트본 1.6.2-7
하단 EQ/반향 버튼 겹침을 수정하고 표시·클릭 영역을 분리했습니다. [변경 사항](RELEASE-NOTES-1.6.2-7.md). 아래 이전 기능 기록은 보존합니다.

## 현재 테스트본 1.6.2-6
모든 채널 음량의 공통 저장·우선 적용, F9 별도 리모컨,40Hz~15kHz20밴드 EQ를 추가했습니다. [변경 사항](RELEASE-NOTES-1.6.2-6.md)에 사용법과 검사 범위를 정리했습니다. 아래는 이전 기록입니다.

## 현재 테스트본 1.6.2-5
마리오 어드밴스 1의 특정 곡 선택 시 종료되는 문제를 수정했습니다. 확인된 PCM 슬롯을 고정 유지하고 곡 전환 시 채널 설정과 버퍼 범위를 검사합니다. 자세한 내용은 [변경 사항](RELEASE-NOTES-1.6.2-5.md)을 참고하세요. 아래는 이전 기록입니다.

## 현재 테스트본 1.6.2-4
GBA ROM 개별 PCM 음성 분리 누락 수정 및 GBS의 GBA GB 호환 음원 모드를 추가했습니다. 상세 범위와 한계는 [변경 사항](RELEASE-NOTES-1.6.2-4.md)을 참고하세요. 이전 버전 기록은 아래에 보존합니다.

# Nintendo Music Studio 1.6.2-3 — complete corresponding source test build

NES ROM adapter: src/formats/NesRom.cpp/.hpp and NesRomProfiles.hpp contain only owned shim code and program fingerprints. Original PRG/audio/CHR are read from local user ROMs and never shipped. Internal NSF memory images use the existing static GME; no new dependency or sidecar library. tests/NesRomTests.cpp generates its own6502 signal fixture; optional real-ROM arguments remain read-only.

Nintendo-driver profiles in GbaRom.cpp extend original-driver execution to SMA2/3/4; all drivers/data remain in user ROMs, with no additional library. Optional per-frame game music update executes before the original PCM mixer; m4a branch unchanged.

ROM music support uses own src/formats/GbaRom.cpp bootstrap and existing viogsf; no additional core/library or game driver is bundled. Supported ROM revisions are identified by full size/CRC; unsupported revisions fail explicitly.

Windows x64, C++17, Visual Studio2022 Desktop development with C++, CMake3.24+ and Python3. Dependencies are bundled as pinned source archives/trees with notices; no proprietary music is required. Composition/editor sources are absent. Python/Pillow are used only for build/test/package workflows, not runtime.

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DBUILD_TESTING=ON
cmake --build build --config Release --parallel 4
python -m pip install Pillow
ctest --test-dir build -C Release --output-on-failure
python tools/audit_licenses.py build
python tools/package_player.py --build build --output dist
```

The legacy build target NintendoGameMusicViewer outputs **build/bin/Release/NintendoMusicStudio.exe**; GME is statically linked, with no sidecar DLL. Project/solution name is NintendoMusicStudio. The packaging CLI uses caller-provided paths and requires no private workspace. Test fixtures are generated by bundled own test scripts; no user songs/ROMs/banks are needed. Private-path ExtendedVgmTests/N64ReplacementTests options are optional and music is never packaged. Windows UI tests isolate settings/registration and operate only their own processes.

# # Modify/relink

All corresponding application and used-library source/headers, local patches, build scripts and licenses are included. Modify `third_party/licensed-chips/emu/cores/ym3438.c`, then rebuild targets licensed_vgm_chips and NintendoGameMusicViewer to relink Nuked OPN2; the complete-source route follows LGPL2.1 section6a. GME is a separate replaceable gme.dll; the pinned archive and patch application scripts are supplied. Default static MSVC runtime avoids bundling toolchain DLLs. No executable JIT or restricted old N64 framework is required.

N64 is the official Cog-specific GPL2+ lazyusf2 distribution at0ac68cc947a66fc88c130324a593f9bfca698d1a, subtree Frameworks/lazyusf2/lazyusf2. Exact component notice, original CC0/BSD exceptions, source hashes and local Windows/voice-observer changes are retained in third_party/lazyusf. Rebuild lazyusf and NintendoGameMusicViewer to relink. The old noncommercial archive is excluded.

Additional VGM playback uses own bounded scheduling/minimal core declarations, explicit GPL/BSD/LGPL chip cores and zlib ES5506.18's SCC native-rate/area averaging and mix-gain fixes are preserved byte-for-byte in1.5. Signalsmith Stretch/Linear are pinned MIT headers with source/notice metadata. See THIRD_PARTY_NOTICES.md, LICENSE-REVIEW.md, LICENSE-INVENTORY.json and licenses/. Distribute the complete corresponding source ZIP with the runtime ZIP and retain notices.

1.5.1: ES5506 host integration reads actual output buses and word precision. One explicitly marked read-only accessor was added to es5506.hpp; original zlib header/notice retained. Original synthesis algorithms are unchanged. No old restricted resampler/core is restored.


1.5.2 level correction: ES5506 extra default attenuation from1.5.1 is removed; real serial buses remain. Historical1.5.1 correlation is not a chip-balance validation. See docs/VGM-LEVEL-152.json.


1.5.3 adds own src/audio/ChipInterpolator.hpp and passband/DC/stereo/silence tests in ExtendedVgmTests.cpp. No new external dependency or altered third-party synthesis. Complete candidate evidence: docs/VGM-INTERPOLATION-153.json.


1.5.4 ordinary-audio decoding: own AudioFileBackend.cpp compiles unmodified miniaudio0.11.23 + bundled stb_vorbis as decoder-only code. MIT option chosen, original notices retained. Own wide-path Vorbis pull adapter reports length and enables direct seeks without copying whole compressed files. UI and playback continue using SDL exclusively. Own synthetic compressed fixture vectors are embedded in tests/audio_fixture_data.json; no FFmpeg is required by CTest or runtime. Developers can regenerate the described vectors with FFmpeg.


1.5.5: Registry path-aware fallback routes ordinary extensions to real decoder validation, preserving known game signatures. IMusicBackend optional decodedEndFrame and AudioFileBackend report actual PCM EOF; Player playbackBoundaryReached uses this for ordinary audio, and metadata for emulated formats. Recording-only wave start extends into labels, deferred outlined labels draw after the waveform. New native button/matrix/recording tests are included; unknown-length emulated loop boundaries are not invented.


1.5.6 UI-only service: Window::updatePlayback is called by DisplayPacer::Message and WM_TIMER; no audio-thread or painting restart. Recording ChannelLayout accepts recording-aware right edge. Native tests can explicitly drop timer1 only in own automated snapshot sessions; normal launches cannot activate this fault injection. Optional timer_only_baseline is a diagnostic build target using the old timer-only path, not included in runtime packages. Full evidence: docs/FULLSCREEN-156.json.


1.5.9 MIDI: own bounded SMF0/1 parser (MidiParser.cpp), DLS1 adapter and SF2 table preflight (MidiBank.inl), MIDI synth backend using pinned TinySoundFont MIT source with marked interpolation bounds patch. No Windows/other bank data is included; tests generate self-authored sine SF2/DLS. F8 selection uses a separate local midi-soundfont.txt next to the existing INI, leaving integer preferences compatible. DLLs, drivers and global MIDI settings are not installed or modified.


1.5.11 ES observation: marked sidecar arrays in third_party/vgsound-es5506 record each voice contribution after the original per-bus saturation. Read-only accessors expose the prior completed frame. Owned adapter applies the original serial quantization and same sample conversion; explicit residual covers direct bus writes. No ALU/filter/envelope/sample-decoding calculation changed. One unmuted core emits all voice lanes; main core skips unused voice interpolators. AY8930: unmodified BSD3 Furnace ay8910.cpp/.h, pinned origin and original hashes in third_party/furnace-ay8930/UPSTREAM.json, only the standalone sound core copied. No tracker/player framework, game music or ROM data is bundled.

1.5.11 visibility evidence: docs/VISIBILITY-1511.json. Whole-song metadata API and native query tests are included; no music assets required to generate the late-note fixtures.

1.5.12 links LGPL GME statically. This full GPL2+ application source and pinned LGPL library sources/patches/CMake build permit rebuilding/relinking with modified GME; no proprietary object or private asset is required. Public redistribution must provide corresponding source and preserve all component notices. Runtime-only EXE includes notices via generated RCDATA; source ZIP includes the complete license texts and template. Windows MIDI uses winmm MIDI_MAPPER; tests inject an owned fake port and never install/reconfigure drivers.

1.5.12h: own MidiAutoBank.cpp reads only the selected device configuration and local SF2; no CoolSoft executable/DLL/code is linked or redistributed. Dense parser uses O(events log tracks) track-head merge with16M-event/64MB/2h bounds. Hotfix evidence: docs/MIDI-HOTFIX-1512h.json.

1.6 is the public consolidation after1.5.11, with complete current sources and historical evidence retained. Numeric CMake version1.6.0, PE version1.6.0.0, public label1.6. No external publishing is performed by the packaging tool.


## 1.6.2-9 spectrum source

Owned implementation: `src/visualizer/AudioTap.hpp`, `Spectrum.hpp`, `src/ui/SpectrumView.cpp` and `VisualImage.cpp/.hpp`. Windows Imaging Component and GDI are system APIs; no new bundled dependency. FFT and image lifetime checks live in `tests/SpectrumTests.cpp`, `tests/ui_spectrum.py`. Package excludes user image/music assets and local settings.


## 1.6.2-10 current spectrum source

`VisualImage.cpp/.hpp` and WIC/msimg32 dependencies were removed. `SpectrumView.cpp` now renders two rows in3styles; `SpectrumTrail.hpp` contains the bounded UI-owned history and curve interpolation. Image paths are no longer read or written. Prior1.6.2-9 description above is historical.

## 1.6.3-beta 9 Wii/GameCube stream files

`Registry.cpp` signature-detects Wii RSTM/BRSTM, BNS (including IMD5-wrapped BNS), and CRI ADX, then routes them through the existing `StreamBackend`/vgmstream. `FolderPlaylist.cpp` includes `.brstm`, `.bns`, and `.adx`. `Window::setWaveformZoomStep` drives the persistent remote 0.5×–32× control in whole 1× steps. No disc-image/ROM extraction, user assets, or new dependency is included. Local corpus evidence is in `docs/GAMECUBE-WII-1.6.3-beta-9.json`.

