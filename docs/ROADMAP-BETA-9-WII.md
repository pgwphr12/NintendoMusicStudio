# Candidate scope: test beta 9 — Wii game music

## User request
Add Wii game support around test beta 9. This is a roadmap item only; beta 8 does not claim Wii support.

## Investigation order
1. Inventory the player's existing Wii-era stream/container decoders and distinguish direct audio files from game-disc images.
2. Identify which Wii music assets are feasible to discover/read from user-provided game images, and which require formats or filesystem/container work not yet implemented.
3. Build small owned fixtures for any format parsing and keep unknown, encrypted, malformed, or unsupported content as an explicit error rather than reporting a successful empty soundtrack.
4. Verify song listing, track selection, seeking, looping, waveform/stem output, and source image immutability before claiming support.

## Constraints
- Do not bundle ROMs, Wii disc images, extracted game audio, keys, or user-provided copyrighted assets.
- Do not modify source images or install drivers/system software.
- Preserve all existing audio backends and test with copies/fixtures only.
- State clearly whether the iteration supports direct BRSTM-like stream files, game-image extraction, or both; do not use “Wii support” as a blanket claim without that evidence.
