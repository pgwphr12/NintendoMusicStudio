# Software license review — Nintendo Music Studio 1.5 /2026-11-01

The user requires N64 support and research only: no public question/contact was sent. The identified VGM and N64 licensing blockers have concrete source replacements. Nintendo Music Studio1.5 is prepared as a public runtime/source package at the user's request. No external upload/contact is performed. Earlier versions are preserved without retroactive clearance claims.

## Source replacements

| Removed dependency | Replacement and declared terms |
|---|---|
| Unassigned libvgm framework/player/utils/resampler | Own bounded VGM reader/scheduler/adapter, GPL2+ |
| Undeclared extra Gens YM2612 copy | Nuked OPN2, LGPL2.1+; full source and relink build |
| Noncommercial old MAME ES5506 | vgsound_emu ES5506, zlib; original used core unmodified |
| Project64 noncommercial/personal-use LazyUSF CPU | Cog-pinned lazyusf2, specific GPL2+ component notice, CC0/BSD exceptions |

The original restricted files are not relicensed. They and the old N64 archive are absent from the current shipping source and runtime dependency graph. N64 is enabled, not removed as a licensing workaround.

## Precise N64 grant evidence

Official Cog distribution: https://github.com/losnoco/Cog at0ac68cc947a66fc88c130324a593f9bfca698d1a. Used subtree: Frameworks/lazyusf2/lazyusf2. The pinned Acknowledgements/Acknowledgements.json explicitly labels **lazyusf2 (Mupen64Plus), GPL-2.0-or-later**. Official site: https://cog.losno.co/acknowledgements/. Cog's README identifies Christopher Snowhill as maintainer since2013; project contributions include kode54. Frameworks retain their own terms: the basis here is the named component declaration, not inheritance from Cog's generic application license or an unrelated mirror badge. The entry was initially added by another Cog contributor; do not claim a new personal permission message from the author.

The original JSON SHA256 and relevant unmodified entry/texts are retained in third_party/lazyusf/Cog-Acknowledgements.json; LICENSE-EVIDENCE.json pins origin, context and local changes. Original per-file GPL notices, CC0 RSP LLE and X-Scale BSD2 CIC notices remain. The separate official GitLab repository's missing root license remains a historical research finding; this build uses the specifically declared Cog distribution instead. Nothing was posted externally.

Windows x64 rounding and own read-only voice/DMA taps were ported. No noncommercial CPU algorithm was copied from the old core. Cached interpreter53 C units are built; all corresponding source/headers/build inputs and original notices are provided.

## Redistribution conditions and scope

Supply the matching complete source ZIP and notices with the executable. GPL2+ application/library sources and build files permit rebuilding. LGPL2.1 section6a complete source/relink path is provided for statically linked Nuked OPN2; gme.dll remains replaceable. Retain GPL/LGPL/BSD/zlib/MIT copyright/license/disclaimer texts. Third-party per-file exceptions remain operative. Music, ROMs, samples and platform trademarks are separate rights; no user assets are bundled. No rights-holder approval, legal certification or assurance for every jurisdiction is asserted.

Other compiled modules: GME LGPL2.1+/emu2413 MIT, SDL zlib, viogsf/vio2sf GPL/LGPL per-file, zlib, vgmstream ISC-style/per-file, Signalsmith MIT. LICENSE-INVENTORY.json follows the actual runtime MSBuild graph with file hashes and source/notice mappings. It is a provenance/known-blocker check, not a substitute for reading every applicable term. Selected BSD3 OKI/SCC and GPL2+ OPL2 retain original headers. See THIRD_PARTY_NOTICES.md, SOURCE.md and licenses/.

## Other primary evidence

- https://github.com/ValleyBell/libvgm/issues/129
- https://github.com/ValleyBell/libvgm/issues/43
- https://github.com/tildearrow/furnace/tree/592e7fd387e7bb5f0c908452c60632c8d37cdc39/extern/vgsound_emu-modified
- https://github.com/losnoco/Cog/blob/0ac68cc947a66fc88c130324a593f9bfca698d1a/Acknowledgements/Acknowledgements.json

Those libvgm issues were not treated as a blanket grant; the ambiguous dependency was removed. Validation and remaining playback limitations are documented in VALIDATION.md.

1.4.18 adds independent sample-rate/gain regression corrections. The17N64/license replacements are retained. No restricted framework/resampler/source was restored; no external contact occurred. The subsequent1.5public package preserves these corrections.
