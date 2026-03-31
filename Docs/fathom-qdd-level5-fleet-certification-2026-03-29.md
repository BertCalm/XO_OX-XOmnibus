# FATHOM × QDD Level 5 — Pre-Launch Fleet Certification
## 76 Engines · 17,261 Presets · 20 Audit Agents · 2026-03-29
## Ringleader Authority · RAC All Hands

---

## VERDICT: NOT YET SHIP-READY — 47 CRITICAL findings

Fleet average: **8.1/10** | Ship-ready (≥9.0): **10 engines (13%)**

---

## COMPLETE FLEET SCORECARD

| Score | Engine | Criticals | Top Issue |
|-------|--------|-----------|-----------|
| 9.1 | ONSET | 0 | Fleet leader — clean |
| 9.1 | OWARE | 0 | W1 fixes verified |
| 9.1 | OPALINE | 0 | W1 fixes verified |
| 9.1 | OPTIC | 0 | Atomic writes block-rate (minor) |
| 9.1 | ORGANON | 0 | Spectral centroid FIXED |
| 9.1 | OXYTOCIN | 3 | NEW: 24x std::pow/sample in Thermal |
| 9.0 | OSTERIA | 0 | decomposeShore per-sample (minor) |
| 9.0 | OUROBOROS | 0 | All W1 fixes verified |
| 9.0 | OSTINATO | 0 | std::log in compressor (minor) |
| 9.0 | OXBOW | 0 | W1 fixes verified |
| 9.0 | OBSCURA | 0 | W1 fixes verified, benchmark engine |
| 8.9 | OBELISK | 0 | All W1 fixes verified |
| 8.9 | ORBITAL | 0 | Linear ADSR -> exponential |
| 8.8 | MORPH | 0 | Filter coeff block-rate (zipper) |
| 8.8 | OVERLAP | 0 | Duplicate param layout |
| 8.8 | OBRIX | 1 | std::log2/sample in harmonic field |
| 8.7 | OPERA | 0 | std::pow per-sample vibrato |
| 8.7 | OPCODE | 0 | W1 fixes verified |
| 8.7 | ORGANISM | 0 | Clean |
| 8.7 | OTIS | 0 | TonewheelCrosstalk 288 fastSin (ok) |
| 8.7 | OVERWORN | 1 | worn_concentrate dead param |
| 8.7 | OCHRE | 1 | noteOff *= 0.25 hard cut |
| 8.7 | ORPHICA | 0 | W1 fixes verified |
| 8.6 | FAT | 1 | crushRate hardcoded 44100 |
| 8.6 | ORIGAMI | 0 | IFFT cos/sin at hop-rate |
| 8.6 | OCELOT | 0 | 2 dead coupling params |
| 8.6 | ORACLE | 1 | std::tan/log per cycle at high pitch |
| 8.6 | OMEGA | 1 | std::exp per voice per sample |
| 8.6 | OVERWASH | 2 | 131K std::log2/block CPU BOMB |
| 8.6 | OVERGROW | 0 | Clean |
| 8.6 | DUB | 0 | Cleanest in batch |
| 8.6 | ODDFELLOW | 0 | Clean |
| 8.6 | SNAP | 2 | midiToHz per sample per voice |
| 8.5 | OMBRE | 1 | std::pow per voice per sample |
| 8.5 | ORBWEAVE | 0 | Verify CytomicSVF coeff path |
| 8.5 | OSPREY | 1 | cos/sin per sample for pan |
| 8.5 | OVERTONE | 0 | Mono output (by design) |
| 8.5 | OVERFLOW | 0 | Beat-mod aliasing risk |
| 8.5 | OTO | 0 | Clean |
| 8.5 | OAKEN | 0 | macroMovement dead |
| 8.5 | OLATE | 0 | Clean |
| 8.5 | OBSIDIAN | 1 | std::sqrt x4 per sample |
| 8.4 | OCTOPUS | 0 | Sucker filter per-sample |
| 8.4 | ORCHARD | 0 | 21 presets (thin) |
| 8.4 | OLEG | 0 | Clean |
| 8.4 | OCTAVE | 0 | Clean |
| 8.4 | OUIE | 1 | std::pow per sample per voice |
| 8.4 | OXALIS | 1 | semitonesToFreqRatio per sample |
| 8.3 | OFFERING | 1 | std::sin in Hat/Kick/Tom |
| 8.3 | OVEN | 1 | macroSpace dead |
| 8.3 | OSIER | 0 | filterEnv not released on noteOff |
| 8.3 | OGRE | 0 | CytomicSVF per-sample |
| 8.3 | OUTWIT | 0 | getDensity O(512) per sample |
| 8.2 | OCEANDEEP | 0 | DeepBioExciter constant recomputed |
| 8.2 | ONKOLO | 1 | std::exp x64/sample on key-off |
| 8.2 | ORCA | 1 | couplingBreachTrigger dead |
| 8.2 | OBLIQUE | 1 | Phaser 12 trig/sample + bounce pan |
| 8.1 | OVERCAST | 1 | SVF per-sample + macroMovement partial |
| 8.1 | OTTONI | 1 | std::sin x2 in sample loop |
| 8.0 | OLE | 0 | Linear release (soft notes cut short) |
| 7.8 | OASIS | 4 | 48x std::exp/sample + broken coupling |
| 7.8 | OCEANIC | 3 | 740 trig calls/sample at Poly4 |
| 7.8 | OPENSKY | 4 | Dead shimmerR + 6 dead mod params |
| 7.8 | OUTLOOK | 2 | Voice steal no crossfade (click) |
| 7.8 | OWLFISH | 3 | Dead coupling params + ArmorBuffer bug |
| 7.8 | OSMOSIS | 1 | std::exp per sample in membrane |
| 7.5 | OBIONT | 2 | 0 presets + 65x cos/sample |
| 7.4 | BITE | 2 | 16x std::exp + 16x std::pow/sample |
| 7.4 | OBBLIGATO | 3 | Linear noteOff + 12x std::sin |
| 7.4 | OPAL | 7 | 36x std::pow/sample + 18 dead params |
| 7.1 | OHM | 3 | 72x std::sin/sample at max MEDDLING |
| 7.1 | BOB | 1 | 8x std::tan/sample |
| 6.8 | DRIFT | 2 | 24 CytomicSVF coeff updates/sample |
| 6.8 | OUTFLOW | 2 | ZERO PRESETS |
| 6.8 | OBSIDIAN | 1 | 4x std::sqrt/sample |
| 6.5 | OVERWORLD | 1 | ALL DSP IS STUBS - outputs silence |
| 5.8 | OBLIQUE | 3 | 12 trig/sample phaser + steal click |
| 3.5 | OBIONT | 3 | 0 presets + 65x cos/sample + dead mods |

---

## INFRASTRUCTURE SCORES

| System | Score | Blockers |
|--------|-------|----------|
| Shared DSP Library | 8.9 | ControlRateReducer overwrite bug, ComplexOscillator denormal threshold |
| MegaCouplingMatrix | 8.5 | Audio-thread allocation |
| MasterFXChain | 7.5 | Null pointer crash risk (6 params) |
| ShaperRegistry | 3.0 | NEVER CALLED - Shapers are dead code |
| ObserveShaper | 5.5 | EQ logic fundamentally broken |
| OxideShaper | 6.5 | Tape mode is a no-op, NaN risk |
| Engine Registration | 7.0 | OASIS + OUTFLOW params missing from APVTS |
| Export Pipeline | 7.5 | No shaper parity |
| AI System | 8.0 | Clean |
| XOlokunProcessor | 8.0 | currentSampleRate data race |

---

## TIER 0: LAUNCH BLOCKERS (13 items)

1. OVERWORLD: All DSP files are stubs - outputs silence
2. OBIONT: 0 presets + CPU bomb + dead mod destinations
3. OUTFLOW: Zero presets
4. OKEANOS: Parameter prefix oasis_ collides with OASIS
5. ShaperRegistry: ObserveShaper/OxideShaper never process audio
6. ObserveShaper: EQ band accumulation broken (only band 6 survives)
7. OASIS params: addParameters() missing from APVTS
8. OUTFLOW params: addParameters() missing from APVTS
9. MasterFXChain: 6 parameter loads with no null check -> crash
10. OxideShaper: Lorenz attractor no NaN guard
11. OxideShaper: Tape mode HF rolloff is complete no-op
12. ObserveShaper: Iron emulation mono-only
13. XOlokunProcessor: currentSampleRate data race

## TIER 1: CPU BOMBS (~25 engines)

Per-sample transcendental calls that cause audio dropouts:

| Engine | Issue | Calls/Sample |
|--------|-------|-------------|
| OCEANIC | sqrt+pow+cos+sin in particle loop | 740 @Poly4 |
| OVERWASH | std::log2 per partial per voice | 131K/block |
| OASIS | std::exp in Rhodes partial loop | 48 |
| OHM | std::sin in FM operators | 72 max |
| OBIONT | std::cos in projection readout | 65 |
| OPAL | std::pow + std::cos per grain | 36 |
| OXYTOCIN | std::pow in Thermal model | 24 |
| DRIFT | CytomicSVF coeff in formant filter | 24 |
| BITE | std::exp + std::pow | 32 |
| OBLIQUE | Phaser 12 trig + bounce pan 2 | 14 |
| OBBLIGATO | std::sin in flutter | 12 |
| ONKOLO | std::exp in fast-damp on key-off | 64 |
| BOB | std::tan in SnoutFilter | 8 |
| OBRIX | std::log2 in harmonic field | varies |
| SNAP | midiToHz std::pow per sample | 8 |
| OUIE | std::pow per sample | 8 |
| OMBRE | std::pow per voice per sample | 8 |
| OMEGA | std::exp per voice per sample | 8 |
| OSPREY | std::cos+sin for pan | 8 |
| OXALIS | semitonesToFreqRatio | 4 |
| OBSIDIAN | std::sqrt x4 | 4 |

## TIER 2: DEAD PARAMETERS (~40 across 10 engines)
## TIER 3: NOTEOFF CLICKS (8 engines)
## TIER 4: DEAD COUPLING ROUTES (4 engines)

---

## FLEET STATISTICS

- Total engines scored: 76
- Ship-ready (>=9.0): 10 (13%)
- Polish needed (8.0-8.9): 40 (53%)
- Fix required (7.0-7.9): 18 (24%)
- Blocked (<7.0): 8 (10%)
- Fleet average: 8.1/10
- Total CRITICAL findings: 47
- Dead parameters: ~40 across 10 engines
- Engines with CPU bombs: ~25 (33%)
- Engines with noteOff click risk: 8 (11%)
- Engines with 0 presets: 3

---

*Generated by FATHOM x QDD Level 5 Fleet Certification*
*Ringleader Authority | RAC All Hands*
*20 parallel Sonnet audit agents | Full source read of 146 header files*
