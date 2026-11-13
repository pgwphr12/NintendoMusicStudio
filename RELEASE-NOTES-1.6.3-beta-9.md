# Nintendo Music Studio 1.6.3-beta 9

## Previous update: 1.6.3-beta 8

- While playback is paused or stopped, the visualization history continues to scroll and fades to silence; audio and playback time remain stopped. Horizontal history also continues across visualization modes, recording view, and fullscreen transitions.
- Restored the earlier channel-cell, label, and volume-control layout. The F12 remote's saved **Waveform spacing** control changes waveform shape from 0.5× to 8× without widening channel cells.
- Adjusted low-clock YM3526 playback to preserve the chip's sample interval. Development comparison scores are not a claim of exact match with the original audio or Furnace project.

## 1.6.3-beta 9

- Added direct stream-file playback for Wii `BRSTM` and `BNS`, CRI `ADX` game-audio files used by the supplied GameCube sample collection, and Nintendo 3DS `BCSTM`/`BCWAV`. Multi-channel files expose independently adjustable stereo track groups; `BCSTM` uses its explicit track/channel table when present. A 3DS file with one two-channel mix and no separate track groups now shows independent `LEFT` and `RIGHT` lanes. Those lanes represent stereo sides, not recovered instruments. Other formats retain their decoded grouping.
- Expanded the F12 remote's waveform spacing control to integer 1× steps through 32×, retaining 0.5× as the minimum. Select a channel to give it a saved per-file/per-track spacing override; **개별 해제** returns it to the common value. It changes the displayed sample window, not channel-cell width.
- Removed the optional **무한 재생** control. With ordinary Repeat enabled, the shared playback worker seeks each format back to its start at the decoded boundary while keeping playback and audio active. With Repeat off, audio becomes silent at EOF but the transport remains in its playing state, as in 1.6.2; Pause and Stop remain available.
- Choosing another song while paused/stopped starts it automatically. Opening any supported file, including drag-and-drop, also starts playback.
- The shared loop uses exact decoder end frames where available and preserves the source-time display by wrapping the elapsed clock. Checked across all 15 existing late-format UI fixtures, real local Wii BRSTM and BNS examples, and BCSTM/BCWAV fullscreen, windowed, and recording-view transitions. These checks use dummy audio; they do not prove physical audio output or every game-specific stream.
- Wii disc images and GameCube disc images are not opened or searched. This is stream-file playback only, not ROM music extraction or complete coverage of every game-specific audio format.
- Verified the user's local sample folder: 88 BRSTM, 2 BNS, and 9 ADX files opened, rendered bounded audio frames, and sought successfully. Five BRSTM cues begin with encoded silence in the first 170 ms; they were retained as valid files.
- Scanned all 198 owned `.bcstm`, `.brstm`, `.bns`, and `.adx` files: 47 expose multiple decodable track groups. All 47 were rendered by the bounded diagnostic, sought, checked for finite samples, and tested with all-lane mute. The remaining 151 files decode as one mono/stereo mix and do not contain separately addressable tracks through this decoder.
- No supplied game audio, ROM, or disc image is included in the packages.
