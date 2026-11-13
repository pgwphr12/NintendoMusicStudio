# Nintendo Music Studio 1.6.3-beta 10

## Waveform after a song ends

- With Repeat off, the song's audio ends normally and the transport remains in its active playing state.
- The waveform history keeps moving after EOF as silence flows through it, instead of freezing on the final audio samples.
- This is display-only history; no sound is added and decoded audio is unchanged.
- Repeat remains a separate setting: when enabled, playback loops at the song boundary.

## Automatic playback

- Selecting another track starts it even when the previous track was paused or stopped.
- Opening a supported file or dropping one onto the player starts playback.

## Verification status

The beta 10 executable is built from the corresponding source. No runtime UI check or test suite was run for this build, so verify EOF waveform movement and automatic playback on your files before treating this test version as stable.
