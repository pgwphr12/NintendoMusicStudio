# VGM sound regression fixed in1.4.18

User reports supplied VGM sounded correct through1.4.15 and changed in1.4.17. The regression was introduced by the own licensed adapter/scheduler replacement in16;17 reused that path. It was not an unavoidable change of chip implementation: this file uses GPL OPL2/BSD OKI/BSD SCC, all retained core implementations. The previous positive tests checked nonzero audio but did not check the SCC musical pitch or match the old chip balance.

## Causes and fixes

1. SCC reports native generation rate clock/16 (111860Hz at this file's1789772Hz clock). The wrapper advanced one native sample for every48kHz output sample. Musical pitches fell by48000/111860; the observed early SCC peak528Hz became226Hz. LicensedChips.cpp now consumes the reported native rate. Downsampling averages the overlap of native sample cells across each output interval, independently authored; no old unassigned resampler was restored. Point-only interpolation was tested first but gave poorer high-frequency agreement, and is not the final downsampler.
2. The own scheduler divided total output by sqrt(declared chip count), reducing three-chip files to57.735% output. Chip count itself is not a gain control. The file's volume modifier and explicit chip volumes remain; the arbitrary count-based attenuation is removed. No user master/channel settings are reset.

## Evidence

Unmodified1.4.15 compiled libraries were linked only into an internal reference probe. Original music was read-only; no music or audio traces are shipped. Same12s and entire79s intervals were rendered as mix/OPL/OKI/SCC buses. Before-fix data came from unmodified17 libraries. Final18 full output SHA equals the verified correction candidate. Whole79s SCC waveform correlation0.9999999994 at zero lag; RMS ratio1.0000035773. Whole mix RMS ratio0.9995105911 (about0.05% difference), correlation0.9946569860. Other timing/core-rate conversion differences remain; no claim of byte-identical whole output or physical listening. Detailed metrics: VGM-SOUND-1418.json.

Own sine fixture expects SCC clock/(16*(period+1)), measured888Hz vs expected886.525Hz using half-second crossing count. It rejects the old low-pitch path. A declared silent second chip now leaves sample gain unchanged. Existing bounded malformed/mute/seek/replay checks remain. Licensed third-party cores and notices are unchanged. N64 replacement core is byte-identical to17 (231files), old releases retained.
