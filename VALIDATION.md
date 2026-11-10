## Current beta7 MIDI validation

See docs/MIDI-LOCAL-1.6.3-beta-7.json.85/85 actual whole-song DLS render/seek/restart checks (80audible,5valid-empty) plus85SF2load/1s render.7initial and3final targeted CTest checks PASS, native local/VMS precedence PASS. No hardware listening. Previous evidence below is historical.

## Beta6 smooth revision

Latest evidence: docs/WAVE-SMOOTH-1.6.3-beta-6.json. Final6focused checks PASS;108synthetic refresh/shape/frequency cases, history/transition tests and native performance/reference comparison PASS. Previous evidence below is historical.

## Corrected beta6 validation

See RELEASE-NOTES-1.6.3-beta-6.md and docs/WAVE-GBA-1.6.3-beta-6-revision.json for fresh tests and preserved failed candidates. Prior beta7 evidence below is historical, not a fresh run of this revision.

# Test1.6.3-beta 6 validation

New YM cases cover reset-only, short genuine intro notes, later/partial-slot notes, DAC enable-only/center-only, real direct/stream DAC, fixed pre-play masks and unchanged controls. Generic finite-end/unknown/error analysis cases and native all-channels persistence/override checks are included. Details: docs/YM-USAGE-1.6.3-beta-7.json.

# Test1.6.3-beta 6 validation

YM3812/YM3526 primary and auxiliary channels now use a precomputed whole-song key/rhythm usage mask. Late notes are visible before playback; silence/mute/gain do not alter visibility. Decoder channels and real audio remain intact.

Free-running scope tests cover24 signed/DC/pulse/bass/harmonic phase sweeps at60/144Hz,256/2048/4096 sample windows. Left and middle must vary, samples must not be modified, and unchanged input must not invent animation. Existing silence/peaks/interpolation checks remain. See docs/WAVE-FREE-1.6.3-beta-6.json.

Same-version revision: user requested recording channel names retained and version unchanged. Fresh recording/name-only palette and ES/AY native UI3/3 PASS5.46s. Names are drawn as fitted outlined overlays; metadata remains hidden. Earlier waveform stability/audio evidence below is reused unchanged.

# Test1.6.3-beta 5 validation

Final9/9 tests PASS27.61s plus fresh independent remote and hidden-channel native tests. Recording canvas has only waveform/background colors and uses the former metadata/header/footer space; normal-view controls remain.

24 moving phase sweeps cover signed/positive/bass/harmonic/pulse signals,256/2048/4096 windows and60/144Hz presentation advances. Fractional stable curves are compared with beta4 display code. Silence/DC/short input, original peak retention, live noise changes, no input mutation and a scope benchmark are included. Details: docs/WAVE-STABILITY-1.6.3-beta-5.json.

# Test1.6.3-beta 4 validation

No-VMS cases cover stale saved SF2/external preferences, missing/invalid/corrupt registered bank candidates, real system DLS synthesis and channel samples, seek/mute, an owned DLS-only input and descriptive missing-all failures. Native UI uses own offscreen/dummy processes, without uninstalling the host driver. Details and remaining limits: docs/NO-VMS-1.6.3-beta-4.json.

# Test1.6.3-beta 3 validation

New chip checks: YM3526 standard and custom335582Hz tone contracts, SN76489 period260/stereo, stem recombination/mute/replay and34 bounded malformed/unsupported cases. Supplied slogic renders63.483s including final silence, and1794124986077 renders67.023s; seek500ms/replay/all-mute/dummy worker checks pass. Full-timeline render does not certify physical listening identity with the original Furnace project.

Final focused13/13 PASS17.35s; earlier focused4/4 includes own8pitch cases, generic/extended VGM regressions and ES/AY voice structure. Actual lobby10second stems show the expected octave change only in ES, YM2612/OKI byte-identical. Generic frequency contracts, wrong creator/serial/later ACTV negative cases, replay/seek/mute/stem sums are covered. See docs/ES-PITCH-1.6.3-beta-3.json for source rationale, failed diagnostic candidates and limits. No full Furnace project/WAV or physical listening identity claim.


# Test1.6.3-beta 2 validation

Fresh native offscreen/dummy sessions verify normal/recording startup without remote, independent F9/F12/button toggles, child/EQ focus routing, autorepeat suppression, native close, reused window, responsive playback, and recording footer pixels containing no shortcut/button. Hidden-channel and remote/EQ regressions are fresh. See docs/REMOTE-1.6.3-beta-2.json. Unchanged audio/core matrices are reused with SHA provenance.


# Test1.6.3-1 beta validation

Fresh native own-process hidden-channel tests cover right-click hit testing, normal/recording views, selected/all restoration, hide-all safety, unaffected mute/gain, saved restart preference, subsong transport, ignored toolbar/spectrum clicks and cross-system isolation. See docs/HIDDEN-CHANNELS-1.6.3-1-beta.json. Unchanged audio/decoder/ROM evidence from stable1.6.2 is reused explicitly. Tests are offscreen/dummy audio, not physical listening certification.


# Nintendo Music Studio1.6.2 — release validation

Fresh focused mixed-gain regression includes old failure and fixed actual internal PCM comparisons; native UI checks stop/play/seek/new-song priority. Stable identity/standalone/choice/About smoke checks and package CRC/SHA/license/asset exclusions are documented in docs/RELEASE-1.6.2.json. Earlier unchanged decoder and spectrum evidence is reused explicitly. Dummy audio/offscreen tests are not physical listening/VRR certification. Historical records follow.


## Test1.6.2-12 changed-path validation

Fresh spectrum utility tests and own offscreen native UI verify direct selection/default/persistence/legacy migration/main and remote entry points/nonblocking playback/resource reuse. See docs/SPECTRUM-CHOICES-1.6.2-12.json. Earlier unchanged FFT/audio/decoder and fullscreen repeat evidence is reused from11; not rerun or claimed fresh. Dummy audio, no physical listening certification.


## Test1.6.2-11 changed-path validation

See docs/SCROLLING-SPECTROGRAM-1.6.2-11.json for fresh analyzer/history/native UI/fullscreen repeat tests and measured rendering. Automated UI uses own generated audio and dummy output. Earlier decoder/content matrices below are retained evidence, not newly rerun. No physical listening or all-monitor timing certification.


## Test1.6.2-8 — pinned MesenCE EQ compatibility
Upstream commit4b8a196: unmodified orfanidis_eq.h/Equalizer.cpp/.h Git blob identity checked.199680stereo frames at48kHz,24gain profiles plus cache/update/bypass/re-enable: exact PCM16 error0. Native EQ2sessions check range±20/integer1/page5/direct entries/clamping/blank/reset/disabled and persistence; remote2sessions pass. Focused3/3PASS0.61s includes common-volume and ROM crash state regression. Fresh user assets or all old decoder suites not claimed. See docs/MESENCE-EQ-1.6.2-8.json.

## Test1.6.2-7 — footer overlap
Fresh own-process native UI: reverb click does not open EQ, right-hand EQ opens independently, saved reverb remains enabled, main responsive. Footer screenshot visually checked. Audio graph unchanged; previous1.6.2-6 tests reused, not repeated. See docs/FOOTER-1.6.2-7.json.

## Test1.6.2-6 — common gain / recording remote /20band EQ
Fresh focused3/3PASS0.41s (graphic_equalizer_audio/common_channel_volume_priority/rom_channel_transition_stability).20independent center sine responses +6dB each, exact flat/off bypass, stereo isolation/stress/limiter. Common-volume first-render/backend-transition test and own native4-format persistence test. Modeless remote/EQ2process restart controls/pass; settings isolated, SDL dummy, no physical listening. Details docs/COMMON-VOLUME-1.6.2-6.json. Unchanged ROM/GBS content matrices retain prior checkpoint evidence.

## Test1.6.2-5 — SMA1 sparse-entry crash
Actual old Player Song199 exit0xC0000005; fixed8song transitions/seek/stop PASS. Japan319+two revisions321each=961short starts with0layout changes/0decoder errors. Six affected commands compare to raw hardware mix with exact0error. Changing12/6/12slot own fixture16cycles and malformed layout rejection PASS. Focused3/3 (including fixture generation and core_audio_channels)11.47s. Native34transitions/recording view PASS(dummy). See docs/ROM-STABILITY-1.6.2-5.json. No whole-song/physical-hardware/all-file guarantee.

## Test1.6.2-4 — GBA ROM slot separation and GBS AGB-compatible APU
See docs/GBA-GB-1.6.2-4.json for fresh changed-path evidence, baseline reuse, initial failures and limitations. Real ROMs19files/13profiles, 8seconds each with fullmix error0; GBS12cases (1own+11local), four lanes and AGB mode checks; new native UI12lanes. No physical listening certification or repeated full baseline suite.

# Nintendo Music Studio test1.6.2-3 validation

New NES path:49 actual NROM32 PRG fingerprints,14 commands each(686 starts,96256frames/2.005s each),all finite/positive with14 distinct output fingerprints per program; aliases not asserted unique compositions. Replay,500ms seek(delta0),mute,graphics independence,driver mutation rejection and original readback PASS. Initial raw-GME seek/filter-history mismatch resolved only in new ROM adapter with short replay/long fast-seek+bounded warmup. NTSC/PAL60000ms seek/looping+rewind PASS; no timing guarantee. Focused own adapter/22-extension registration2/2 PASS0.08s. New own dummy/offscreen NES UI verifies auto-start,14entries,5source lanes,per-track visible5/5/4,next/previous/pause/F9 and real nonflat APU waveform; screenshot inspected. Full existing/user-confirmed suite and packaged GUI deliberately not repeated. Final UI changes after that snapshot are seek/error text only. No physical listening, universal mapper support or full-game-context claim. Details: docs/NES-ROM-1.6.2-3.json.

# Nintendo Music Studio test1.6.2-2 validation

Nine new actual ROM profiles PASS distinct songs0/1, replay/exact500ms seek/mute/invalid-ID/original-readback checks. SMA2/3/4 US catalog sweeps48+40+57=145 entries: first2seconds finite and nonconstant after removing mean/excluding first85ms (minimum AC energy65.7248/16.1138/219.136), not just idle DC and not full-song listening. Final runtime35/35 CTest PASS109.76s; subsequent test-only AC metric improvement ran the actual sweeps without player changes. Initial incorrect flag mask rejected valid original data; corrected per-profile allowed flag sets, originals preserved. Prior/user-confirmed ROM frontend reused; no fresh per-ROM GUI or packaged GUI rerun. Package --skip-ui is explicit in manifest. Physical audio, arbitrary ROMs, auxiliary banks and individual PCM instruments remain unverified/unsupported. See docs/ROM-MUSIC-1.6.2-2.json.

# Nintendo Music Studio 1.6.2-1 validation test build

Full35/35 CTest PASS108.97s before final ROM-only auto-start addition; final affected6/6 PASS17.94s. Own return-only emulated driver checks private bootstrap/body preservation and revision/ID errors. Four actual SMA1 ROM profiles list319/321 real IDs; songs170/171 produce distinct finite nonzero PCM, deterministic replay, exact500ms seek and all-channel mute; original buffers/file readback equal. Native own offscreen/dummy test verifies auto-start,321entries,6hardware lanes, previous/next/pause/recording and nonflat PCM A/B; screenshot visually inspected. Initial native no-autostart failure preserved and fixed. No claim that all entries were auditioned or other ROMs work. Details/limits: docs/ROM-MUSIC-1.6.2.json.

# Nintendo Music Studio 1.6.1h validation

Full34/34 CTest PASS112.02s on final build. Isolated absent-VMS case ignores stale custom-bank/native-output selection, renders the Windows default bank and shows waveforms; existing-style gray Windows 기본 button and F8 are inert. Installed cases retain SF2/DLS controls and live bank reload. Detection is at startup; actual MIDI device or installed executable required, not stale configuration alone. Tests use own generated inputs, simulated presence and dummy audio; no host uninstall, user configuration change or physical listening. Initial unpublished About-initialization placement failed two UI tests; fixed main-run initialization before full rerun. Details: docs/NO-VMS-1.6.1h.json.

# Nintendo Music Studio 1.6 validation

Consolidated public build after user-reported GitHub1.5.11. Numeric PE1.6.0.0 and public1.6. Full34/34 CTest PASS110.67s before final label/migration change; final affected9/9 PASS29.30s. All16 MIDI names Channel1..16 (including drum Channel10), synthesis retained, old MIDI volume/mute migration and new-name override precedence PASS. Native live-bank tests preserve play/pause/position/master/channel controls and keep old decoder for missing/corrupt banks, recover and notice same-path edits. Final packaged EXE-only/import/About/controls/visibility/MIDI/live tests verify the shipped artifact. Tests dummy audio/owned inputs; no physical listening or external publishing. Complete corresponding source, notices and consolidated release notes included; assets excluded. Details: docs/RELEASE-1.6.json. Older evidence remains historical.

# Nintendo Music Studio1.5.12h4 validation

Full34/34 CTest PASS108.60s before final debounce-reset/short-label refinements. Final affected3/3 PASS8.90s. Owned live-bank UI verifies playing position preservation, paused position stability, master73/channel mute+gain55 preserved, corrupt-bank failure keeps playing, missing bank keeps old decoder, recovery and same-path content edit, saved new path, resume.4successful swaps; controls initially supplied before new prefill. Final packaged regression runs the same live test on distributed EXE. Dummy audio/owned config and generated fonts only; no physical listening or user VMS config change. Detection uses saved config (~1s) plus load/seek and may pause audio briefly; no crossfade. Prior h3 explicit-import actual8bit.sf2 render/seek evidence retained. Details: docs/VMS-LIVE-1512h4.json.

# Nintendo Music Studio1.5.12h3 validation

Fresh affected MIDI/About6/6 PASS13.33s. Own active/disabled config and multiple-enabled import-list tests pass; existing font validation/synthesis/persistence tests retained. Actual explicit import resolves8bit.sf2 (6,744,228bytes) and provided Basics MIDI renders10s energy36971.1/seek5s367.705,10695events/239898ms, no external output. User config/SF2 read-only, original paths referenced rather than copies. h2 full33/33 retained for unchanged paths; packaged standalone/UI/MIDI checks cover h3. Limits and evidence: docs/VMS-IMPORT-1512h3.json.

# Nintendo Music Studio1.5.12h2 validation

Full33/33 CTest PASS107.94s before final button-label shortening only. Shared menu policy explicitly tests Windows-default choice3 ->internal and external choice4 ->device. Automatic active VMS bank and non-silent ambiguity rejection pass. User current Basics In Behaviour MIDI10,695events/239,898ms passes10srender/5sseek with12,697,366-byte SF2; actual own UI shows bank filename Nintendo64.sf2 and a nonflat lane within its own waveform cell. Native UI tests dummy audio; no external MIDI notes or user app/config modifications. Packaged EXE-only/import/About/controls/visibility/MIDI tests cover final button wording. Internal synthesis is not captured VirtualMIDISynth output. Details: docs/MIDI-DEFAULT-1512h2.json.

# Nintendo Music Studio1.5.12h validation

User3Armageddon files reproduce old1M-event failure. Fixed parser accepts6,487,738 /6,263,892 /6,393,452 events including cleanup; complete92.285/104.678/117.428s timelines with active9.94MB SF2 and5sseek PASS, offline wall26.64/23.82/29.46s. Actual configured-bank hidden UI shows nonflat lane indices7/11 within their own cells, automatic start and route migration PASS. Separate155.4MB Arachno bank10s/5sseek PASS. All tests dummy audio, no physical listening/external notes. Initial full33/33 PASS102.68s; after latest large-bank/inactive-loop changes final affected6/6 PASS12.97s. Own million-event and config encoding/enable/multiple/empty-loop fixtures, malformed and bounded-limit guards pass. Test process isolated from live user config; user assets/config not modified/distributed. Internal synthesis is not captured/identical VirtualMIDISynth audio. Native device output remains optional. Details: docs/MIDI-HOTFIX-1512h.json.

# Nintendo Music Studio1.5.12 validation

Full33/33 CTest PASS100.76s before the final GM-reset gain preservation. Final affected MIDI/visibility7checks PASS; a new About test assertion initially used the wrong cross-process text API, fixed to WM_GETTEXT and final notice/transport3/3 PASS5.30s. Fake port validates consumed clock, prefetch exclusion, pause/resume/seek, mapper switch, mute/master and GM-reset gain with no hardware notes. Read-only real mapper query returns4devices/preferred0/success0; changing an actual third-party output and physical listening are unverified. Prior provided FD piano.mid2channels263200ms passes internal10s render and5sseek. New own RMID/no-EOT/running-metadata/absent-bank/vendor-SysEx fixtures pass; invalid/truncated still reject. User's particular failing MIDI remains unspecified. Recording label long BUS RESIDUAL screenshot visually inspected; native fitting checks report no overflow. GME static library and980runtimeTUs all license-classified; full corresponding source permits relinking. Windows-system-only PE imports and EXE-only-folder packaged UI validation recorded by package tool. Component notices embedded/readable in About. Details and limits: docs/MIDI-STATIC-1512.json.

# Nintendo Music Studio1.5.11 validation

Full32/32 CTest PASS99.28s. Final6/6 targeted tests PASS19.60s after detecting any nonzero optional NSF PCM and fixing the test tool Unicode argv. Late DMC and ES voice32 appear before first sound, lists remain fixed through playback/pause/F9/seek/mute/zero gain/rewind, and song/subsong changes reset the list. NES/GB base lanes, GBA PCM, DS/SPC and other declared base buses retained. Unused auxiliary OPL test passes. Actual provided lobby VGM has17 lanes immediately at load (7YM2612,4OKI,6ES voices), no supplied assets modified or packaged. Own native UI captures visually inspected with8fixed lanes. Earlier narrow-argv provided-path failure retained, corrected wide-argv test PASS. Tests use dummy audio, not physical listening. Whole-stream VGM metadata analysis is separate from live audio synthesis; unknown-length NSF remains conservatively visible. Details: docs/VISIBILITY-1511.json.

# Nintendo Music Studio1.5.10 validation

Fresh full30/30 CTest PASS97.91s before the final legacy-settings fallback; final restart/migration UI3/3 PASS9.33s including individual-override precedence; final targeted4/4 PASS15.03s adds genuine direct ES bus output/residual mute. ES32/5/16active voices verify native rates below/above/equal48k, sum error at most1.43e-6; voice32 above global bit31 and residual bit32 mute tested; native click/volume at global channel38 and3column recording image pass. AY expanded/classic/half-clock pitch and duty variants pass. Provided ES165.275s and AY85.74s complete timelines, replay/seek/mute/unmute and original byte preservation pass with dummy output;6ESvoices active in provided lobby. Final optimized20s lobby mix/YM/OKI bit-identical to saved1.5.3mix baseline; ES stem sum delta5.96e-8. Full-song bit identity and physical listening are not claimed. Performance wall times are recorded for the pre-optimization whole-timeline test; unused voice filters are skipped in final main-mix path. Full evidence/limitations: docs/CHIP-VOICES-1510.json.

# Nintendo Music Studio1.5.9 validation

Fresh full CTest28/28 PASS85.72s. Final3/3 targeted tests PASS0.82s add16-channel exact stem recombination and existing same-EXE-path registration-consent refresh under isolated HKCU. MIDI parser tempo/running-status/SMPTE, malformed SMF/SF2, own sine PCM SF2/DLS, native Windows default bank, program/drums, replay/seek/mute/EOF tested. Native UI shows actual separate synthesized channel waveforms with system and selected Unicode SF2/DLS banks; snapshot forced Play disabled. MIDI button repeat wraps twice. Earlier supplied FD piano.mid read-only first10s and5sseek PASS (2channels,263200ms including2s release tail). All UI uses dummy audio; no physical listening claim. Windows MIDI Mapper waveform equivalence is not claimed. No system/user bank or music assets shipped. Evidence and known limits: docs/MIDI-159.json.

# Nintendo Music Studio1.5.8 validation

Fresh targeted CTest7/7 PASS31.05s: fixtures, ordinary waveform, actual EOF repeat, recording geometry, fullscreen repeat, fullscreen DS auto-advance, and ordinary open/start. New4-format open test disables the snapshot harness forced Play so only shared Window.open can start playback; folder autoplay is off, each clock advances, and Space pauses. With only that harness suppression and the old open behavior, MP3 remains paused (expected baseline failure). Decoder and audio core unchanged from1.5.7. Native tests use dummy audio, not physical listening.

# Nintendo Music Studio1.5.7 validation

Fresh targeted CTest 5/5 PASS (24.91s): fixture generation, core audio/channel geometry for 4/8/11/16/18/31 lanes, recording waveform native image, fullscreen repeat and fullscreen DS autoplay. Recording image verifies waveform reaches x1887 inside the x32..1888 channel cell, retains x1888..1919 outer margin, and raises the label near the row top. Image visually inspected. Audio is dummy; no physical listening claim. Audio decoding and transport source unchanged from1.5.6.

# Nintendo Music Studio1.5.6 validation

Fresh24/24CTest PASS80.63s. Existing decoder/game/MP3/EOF repeat and recording controls retained. New tests use own offscreen3840x2160 popup windows driven by the actual F11/F9 handlers, with idle timer1 delivery deliberately disabled. Repeat succeeds in fullscreen -> fullscreen recording -> fullscreen normal -> windowed -> fullscreen again. DS auto advances01->02->10 while fullscreen under the same timer block. Diagnostic timer-only build stops at5002ms without a wrap; current build wraps near2005ms. Unmodified155's ordinary4K trial did wrap once on this machine: exact user hardware symptom is not claimed reproduced solely by resolution. Windows documents WM_TIMER as idle-priority; the fault injection isolates that dependency. Primary source: https://learn.microsoft.com/en-us/windows/win32/winmsg/wm-timer

Recording image verifies left name-area overlap and right-edge pixels only in recording mode. Normal view preserved. Shared decoder/audio/third-party C/C++ sources hash-identical to155. No original music/settings changed, display mode switched, or user process operated. Dummy audio and own native UI testing; no physical listening/high-refresh hardware certification. Details: docs/FULLSCREEN-156.json. Older151..155 evidence remains historical.


## 1.6.2-9 spectrum stabilization

See `docs/SPECTRUM-1.6.2-9.json`:682FFT/tap/motion checks,4current core tests,4native spectrum sessions,5fullscreen repeat phases,3modeless About entries and2remote/EQ sessions. All use owned fixtures/processes, isolated settings and dummy audio.64image replacements show bounded first-use GDI growth and no further batch growth;120frame1920x1080 rendering checks record stable GDI counts. Earlier full ROM/music matrices are reused, not newly certified.


## 1.6.2-10 spectrum styles / image removal

1698unit checks PASS;3owned stereo UI sessions cover allstyles,60stylechanges,recording remote,pause/resume,persistence and stable GDIover120frames.5fullscreen/recording repeat phases PASS. Eight audio/FFT files are byte-identical to1.6.2-9source ZIP; earlier audio/EQ/ROM checks reused explicitly. Image loader/setting access and extra system image links removed. See docs/SPECTRUM-STYLES-1.6.2-10.json.
