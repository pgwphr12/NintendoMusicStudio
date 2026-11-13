# Test beta 9 scope: GameCube and Wii stream files

## User request
The user selected GameCube and Wii music-file playback, excluding Switch and ROM/disc-image extraction. Beta 9 currently adds direct Wii BRSTM/BNS and CRI ADX stream playback, validated against the supplied local corpus. This is format-level coverage, not a claim that all games on either console are supported.

## Investigation order
1. Extend from the verified file set only when an actual sample is available for the additional GameCube/Wii format.
2. Keep direct streams distinct from game-disc images; image parsing/extraction is explicitly out of scope.
3. Keep unknown, encrypted, malformed, or unsupported content as an explicit error rather than reporting a successful empty soundtrack.
4. Verify stream listing, track selection, seeking, looping, waveform/stem output for each added format before claiming support.

## Constraints
- Do not bundle ROMs, GameCube/Wii disc images, extracted game audio, keys, or user-provided copyrighted assets.
- Do not modify source images or install drivers/system software.
- Preserve all existing audio backends and test with copies/fixtures only.
- State the exact supported stream formats; do not use “GameCube/Wii support” as a blanket claim for unverified titles/formats.
