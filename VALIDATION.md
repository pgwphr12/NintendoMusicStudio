# Nintendo Music Studio 1.5 — public release validation

## Public package

Fresh Release x64 build succeeded in work/settings-build/studio150. Display title, modeless information, error/prompt branding, EXE filename, manifest, file version/product strings and Windows registration identity are Nintendo Music Studio1.5/NintendoMusicStudio.exe. Numeric resource/manifest version1.5.0.0. Prior INI location and prompt preference keys are intentionally retained for compatibility; control migration revision1415 is unchanged. No user app/settings were operated/changed.

All18CTest cases are covered by passing results. Initial full run passed17/18; native_folder_playlist initially failed because its old test sequence pressed Space after enabling autoplay, pausing a track intentionally still running beyond metadata length. Only the two test inputs were corrected to retain the playing state, and the targeted folder test/fixtures passed. Product audio/transport code was not changed for this test. Original failed log preserved as work/studio150-checks.log; corrected result work/studio150-playlist-final.log. Other17passing tests remain valid, with no product change afterward.

Audio/core sources298files SHA-identical to1.4.18, including SCC native-rate/area averaging and count-independent VGM gain corrections; N64 stays enabled. Full private-song18results below are reused as unchanged-source provenance, not claimed freshly rerun for1.5. Fresh1.5SCC pitch, silent-extra-chip gain, positive/negative VGM and N64 fixture tests pass. Dummy audio/offscreen tests are not physical listening/hardware verification.

## Complete source route

Fresh default configure from the supplied pinned archives succeeded without private source overrides. Its actual975runtime compile inputs all match the release build's hashes, and64GME headers match. This is source/configuration equivalence, not a claim of a second full clean compilation. Source package includes CMake/scripts, full relevant dependency source/patches/headers and original licenses. Portable packaging accepts --build/--output; CI/doc commands now target the public names. Snapshot builds/tests use only self-authored fixtures.

License inventory:13runtime projects/975translation units, unknown origins0, identified N64 restricted files0. Exact Cog component-specific GPL evidence and CC0/BSD exceptions retained. No old unassigned VGM/resampler/legacy noncommercial N64/ES source restored. No external upload/contact, legal certification or music-use permission is claimed.

Packaged player-only, About/F1 and independent-control UI checks run against the exact staged EXE. Both ZIPs carry matching README KO/EN, changelog, RELEASE.json, LICENSE/THIRD_PARTY_NOTICES, LICENSE-REVIEW/INVENTORY and licenses/. Source ZIP includes rebuild/relink instructions. CRC/SHA, required source/license files and user music/ROM/bank exclusion are verified by tools/package_player.py. Executable is unsigned and the product is unofficial/not Nintendo-affiliated. Prior packages remain untouched.

Logs/evidence: work/studio150-{build,checks,playlist-final,license-audit,source-configure,source-reproducibility,package}.log/json and outputs/NintendoMusicStudio-1.5-validation/. GitHub workflow was edited but not run remotely. No external publication occurs.

# Prior1.4.18 validation — retained audio baseline

User identifies1.4.15 as the correct-sounding baseline. The own16/17SCC adapter treated native clock/16 synthesis as48kHz, lowering pitch; own scheduler also reduced gain by sqrt(chip count).18 honors the reported native SCC rate, independently averages native sample-cell overlap when downsampling, and removes arbitrary chip-count attenuation. Chip volume tables/VGM modifier and user settings remain. No unlicensed framework/resampler/core was restored. Details: docs/VGM-SOUND-1418.md/.json.

## Direct reference comparison

Read-only supplied VGM, unmodified1.4.15 compiled libraries vs unmodified17 and final18. Before: early SCC peak528Hz ->226Hz; all buses about57.7% RMS. Final18: original528Hz restored. Whole79s SCC zero-lag correlation0.9999999994/RMS ratio1.0000035773; whole mix correlation0.9946569860/RMS ratio0.9995105911 (~0.05% difference). Final18 full trace SHA equals the tested correction candidate. This is measured PCM comparison, not physical listening or a whole-output byte-identity claim. The first rate-only/point-interpolation candidate was less accurate; final native-cell averaging is the verified implementation.

## Fresh18 checks

Release x64 build PASS. Eleven selected CTests PASS: fixture generation, settings, core audio/channels, modeless About, independent controls, independent pitch/tempo, N64 replacement, bounded VGM, Nintendo structure, presentation and monitor pacing. The actual freshly built gme.dll was copied before checks. New SCC sine pitch check measured888Hz vs expected886.525Hz (half-second crossing resolution), and a declared silent second chip leaves mix gain unchanged. Existing156malformed/unsupported cases, own ES5506 ROM/tone/mute/model rejection and YM2612 DAC normal/fast streams PASS.

Both private actual VGMs freshly rendered to end+1s under18:

-23lanes/78.350s: peak0.730525, voice recombination maximum1.78814e-7, wall21.7742s.
-12lanes/165.275s: peak0.729551, voice recombination maximum6.15045e-5, wall107.036s.

Both peaks <1(no clipped float output); finite audio, deterministic reset, seek500ms vs equivalent rendering, all-mute/unmute and dummy SDL worker0underruns PASS. Original bytes unchanged. Music/banks/raw traces are excluded from packages.

## Preserved scope

N64 replacement source byte-identical to17 across231files, verified against17 source ZIP. New18N64 fixture test passed; previous17actual3track/observer-off-on/exactseek findings remain provenance-reused, not claimed freshly rerun. Existing independent controls/modeless About/recording behavior pass. Previous17runtime/source ZIPs are preserved; the historical own library build directory was used for a private correction probe, not repackaged as17.

License audit:13runtime projects/975translation units, unknown origins0, identified restricted N64TUs0. Pinned official component declarations/CC0/BSD exceptions/full source/notices remain; no external contact or legal certification. Public1.5packaging follows below/above. README KO/EN, source and license notes updated.

Package script checks runtime/source CRC/SHA, source/license presence and game-asset exclusion, plus packaged player-only/About/controls tests with own offscreen processes/dummy audio. User apps/settings untouched. Logs: work/player1418-{build,checks,actual-vgm,license-audit,package}.log. Comparison evidence: work/vgm-sound-regression/; no raw audio traces bundled.
