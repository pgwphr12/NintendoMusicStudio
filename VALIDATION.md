# Nintendo Music Studio1.5.9 validation

Fresh full CTest28/28 PASS85.72s. Final3/3 targeted tests PASS0.82s add16-channel exact stem recombination and existing same-EXE-path registration-consent refresh under isolated HKCU. MIDI parser tempo/running-status/SMPTE, malformed SMF/SF2, own sine PCM SF2/DLS, native Windows default bank, program/drums, replay/seek/mute/EOF tested. Native UI shows actual separate synthesized channel waveforms with system and selected Unicode SF2/DLS banks; snapshot forced Play disabled. MIDI button repeat wraps twice. Earlier supplied FD piano.mid read-only first10s and5sseek PASS (2channels,263200ms including2s release tail). All UI uses dummy audio; no physical listening claim. Windows MIDI Mapper waveform equivalence is not claimed. No system/user bank or music assets shipped. Evidence and known limits: docs/MIDI-159.json.

# Nintendo Music Studio1.5.8 validation

Fresh targeted CTest7/7 PASS31.05s: fixtures, ordinary waveform, actual EOF repeat, recording geometry, fullscreen repeat, fullscreen DS auto-advance, and ordinary open/start. New4-format open test disables the snapshot harness forced Play so only shared Window.open can start playback; folder autoplay is off, each clock advances, and Space pauses. With only that harness suppression and the old open behavior, MP3 remains paused (expected baseline failure). Decoder and audio core unchanged from1.5.7. Native tests use dummy audio, not physical listening.

# Nintendo Music Studio1.5.7 validation

Fresh targeted CTest 5/5 PASS (24.91s): fixture generation, core audio/channel geometry for 4/8/11/16/18/31 lanes, recording waveform native image, fullscreen repeat and fullscreen DS autoplay. Recording image verifies waveform reaches x1887 inside the x32..1888 channel cell, retains x1888..1919 outer margin, and raises the label near the row top. Image visually inspected. Audio is dummy; no physical listening claim. Audio decoding and transport source unchanged from1.5.6.

# Nintendo Music Studio1.5.6 validation

Fresh24/24CTest PASS80.63s. Existing decoder/game/MP3/EOF repeat and recording controls retained. New tests use own offscreen3840x2160 popup windows driven by the actual F11/F9 handlers, with idle timer1 delivery deliberately disabled. Repeat succeeds in fullscreen -> fullscreen recording -> fullscreen normal -> windowed -> fullscreen again. DS auto advances01->02->10 while fullscreen under the same timer block. Diagnostic timer-only build stops at5002ms without a wrap; current build wraps near2005ms. Unmodified155's ordinary4K trial did wrap once on this machine: exact user hardware symptom is not claimed reproduced solely by resolution. Windows documents WM_TIMER as idle-priority; the fault injection isolates that dependency. Primary source: https://learn.microsoft.com/en-us/windows/win32/winmsg/wm-timer

Recording image verifies left name-area overlap and right-edge pixels only in recording mode. Normal view preserved. Shared decoder/audio/third-party C/C++ sources hash-identical to155. No original music/settings changed, display mode switched, or user process operated. Dummy audio and own native UI testing; no physical listening/high-refresh hardware certification. Details: docs/FULLSCREEN-156.json. Older151..155 evidence remains historical.
