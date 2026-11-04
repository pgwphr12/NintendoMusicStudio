# 1.5.1 timbre validation candidate

User reports creator-provided1793424975458.vgm has a different timbre from the official lobby MP3. Song identity was established separately; that did not prove renderer accuracy. Only VGM was provided by the creator, with no matching Furnace project/reference render. The website MP3 may have different original mix settings, so it is not treated as a bit-exact master specification.

## Confirmed host-path corrections

The old adapter summed20-bit debug voice taps.1.5.1 reads actual6stereo output buses, respecting routing, word-clock precision, output saturation and the prior-frame output schedule. An explicitly marked read-only accessor in the zlib header exposes serial precision without mutating chip state. Disabled/unconfigured output now stays silent; unsupported>20-bit shifts return errors before reaching the core. The ES VGM reference default volume unit is0x20/0x100 (1/8); the old generic unity default was inappropriate. Relative volume now multiplies that default; absolute volume overrides it. No per-filename settings, EQ, fitted gains or reference audio are embedded in playback.

Own fixtures initialize W_ST/W_END/internal mode like actual Furnace dumps. They check real tone, disabled bus silence,12/16-bit normalized gain, relative unity/double/absolute gain, unsupported width errors, mute/replay and ES5505 rejection. The first old fixture omitted internal output-mode setup and produced silence on the real bus; it was corrected rather than reverting to a fake debug sum.

## Measured scope

At pitch/speed1.00 and reverb off, actual final1.5.1 render over20seconds improves official MP3 waveform correlation0.756484 ->0.782903, with only0.25ms latency alignment. The candidate does NOT fully match the MP3. Independent chip probes found YM3438-mode substitution did not improve the result, so YM2612 remains. An OKI level change toward Furnace scaling improves a diagnostic fit but is NOT applied: there is insufficient evidence for a universal fix and it could alter previously accepted files. Remaining reference/engine/mix differences stay open.

The first12seconds of suppliedSCC/OPL/OKI VGM remain bit-identical to corrected18baseline. That file has noES5506. N64/other cores and independent controls are unchanged. Final candidate is a listening/accuracy validation build, not a claim of complete creator-intent reproduction.

## Reference implementation evidence

- https://github.com/tildearrow/furnace/blob/592e7fd387e7bb5f0c908452c60632c8d37cdc39/src/engine/platform/es5506.cpp : actual acquire path uses lout/rout; initialization configures16-bit serial output/internal clocks.
- https://github.com/tildearrow/furnace/blob/592e7fd387e7bb5f0c908452c60632c8d37cdc39/src/engine/platform/genesis.cpp : selectable chip variants, not automatically recoverable from an unspecified project.
- https://github.com/ValleyBell/libvgm/blob/master/player/vgmplayer.cpp : conventional default VGM chip-volume reference. No player/resampler implementation is copied or restored.

Full measurement metadata is VGM-TIMBRE-151.json. Actual audio/user assets remain private. Original files/settings and published1.5packages are preserved; no external contact/upload.
