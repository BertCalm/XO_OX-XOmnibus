# XOlokun V1 Technical Changelog

**Date Range:** March 2026 (2026-03-01 through 2026-03-20)
**Branch:** main
**Build Status:** AU + auval PASS (73 engines registered)
**Total Presets at V1:** ~21,918 factory presets

---

## Engine Additions

### New Engines — March 18-20 (Post-Prism Sweep Wave)

| Engine ID | Source | Commit | Notes |
|-----------|--------|--------|-------|
| OSTINATO | `Source/Engines/Ostinato/` | `1447849d2` | Communal drum circle; 12 instruments; physical modeling; `osti_` prefix; 2,212 source lines |
| OPENSKY | `Source/Engines/OpenSky/` | `a8bfe5b66` | Euphoric shimmer synth; supersaw + shimmer reverb + chorus + unison; `sky_` prefix; 1,542 source lines |
| OCEANDEEP | `Source/Engines/OceanDeep/` | `a8bfe5b66` | Deep-pressure submerged synthesis; `deep_` prefix; 884 source lines |
| OUIE | `Source/Engines/Ouie/` | `a8bfe5b66` | Duophonic hammerhead synth; 2 voices × 8 algorithms; STRIFE/LOVE interaction; `ouie_` prefix; 2,028 source lines |
| OBRIX | `Source/Engines/Obrix/` | `55b13f2b9` | Flagship modular brick engine; constructive collision architecture; Drift Bus + Journey + Spatial; `obrix_` prefix; 65 params (Wave 3); 1,425 source lines |
| ORBWEAVE | `Source/Engines/Orbweave/` | `3e710645a` | Topological knot coupling engine; Kelp Knot architecture; `weave_` prefix; 1,126 source lines |
| OVERTONE | `Source/Engines/Overtone/` | `58909006b` | Continued fraction spectral engine (Nautilus); `over_` prefix; 889 source lines |
| ORGANISM | `Source/Engines/Organism/` | `58909006b` | Cellular automata generative engine (Coral Colony); `org_` prefix; 954 source lines |

### New Engines — March 11-14 (Prism Sweep Expansion)

| Engine ID | Source | Commit | Notes |
|-----------|--------|--------|-------|
| OVERLAP | `Source/Engines/Overlap/` | `f4c1842a5` (build fix) | XOverlap port; stereo bioluminescence; PolyBLEP; KnotMatrix; FDN; `olap_` prefix |
| OUTWIT | `Source/Engines/Outwit/` | `f4c1842a5` (build fix) | XOutwit port; chromatophore timbre engine; `owit_` prefix |
| OMBRE | `Source/Engines/Ombre/` | confirmed in CLAUDE.md | Dual-narrative engine; memory/forgetting + perception; `ombre_` prefix |
| ORCA | `Source/Engines/Orca/` | confirmed in CLAUDE.md | Apex predator engine; wavetable + echolocation + breach; `orca_` prefix |
| OCTOPUS | `Source/Engines/Octopus/` | `f4c1842a5` (doctrine fixes) | Decentralized alien intelligence; arms + chromatophores + ink cloud; `octo_` prefix |
| OWLFISH | `Source/Engines/Owlfish/` | `e6a8772b0` | Monophonic subharmonic organism; Mixtur-Trautonium oscillator (Blessing B014); `owl_` prefix |
| OBLIQUE | `Source/Engines/Oblique/` | `3fe4dc554` | Prismatic bounce synth; RTJ × Funk × Tame Impala; `oblq_` prefix |
| OPTIC | `Source/Engines/Optic/` | `ac6514d35` | Visual modulation engine; AutoPulse trance mode; Winamp-style visualizer; `optic_` prefix |
| OCELOT | `Source/Engines/Ocelot/` | `57480b5e4` | Canopy-layered ecosystem synth; biome system (Underwater + Winter); `ocelot_` prefix |
| OSPREY | `Source/Engines/Osprey/` | `76e410ab3` | ShoreSystem turbulence-modulated resonator; 5-coastline cultural data; `osprey_` prefix |
| OSTERIA | `Source/Engines/Osteria/` | `09d5d9827` | Ensemble synthesis with elastic coupling; ShoreSystem shared; `osteria_` prefix |
| OPAL | `Source/Engines/Opal/` | `2887bfba0` | Granular synthesis engine; AudioToBuffer receiver; `opal_` prefix |
| OVERBITE | `Source/Engines/Bite/` | `2364f8538` | Bass-forward character synth (XOpossum port); Five-Macro BELLY/BITE/SCURRY/TRASH/PLAY DEAD system; `poss_` prefix (frozen) |

### Family Engine Stubs (Constellation Group)

Added 2026-03-13–14 as stub engines (registered, no DSP):
- OBBLIGATO (`obbl_`), OHM (`ohm_`), OLE (`ole_`), ORPHICA (`orph_`), OTTONI (`otto_`)
- `FamilyWaveguide.h` shared physical modeling DSP added for XOrphica Pentagon architecture

---

## OBRIX Flagship — Multi-Wave Build Log

OBRIX was built across 3 iterative waves in a single session (2026-03-18–19):

| Wave | Commit | Content |
|------|--------|---------|
| Wave 1 | `de89586de` | Initial brick architecture; PolyBLEP source; constructive collision routing; 4 modulation slots; 3 FX slots; FLASH macro |
| Wave 1 lock-in | `6b9f5a465` | Documentation, seance prep, preset constraints established |
| Wave 2 | `7fecdff44` | FM synthesis; filter feedback; real wavetable import; unison detune; 60 params |
| Wave 3 | `b4f2e0e31` | Drift Bus; Journey parameter; Spatial field; 65 params total; Fab Five score 8.9/10 |
| DSP embed | `38dc70bf3` | Overlap/Outwit/Overworld DSP copied into XOlokun — sibling repo dependencies removed |

---

## FX Chain Additions

### Aquatic FX Suite (V1.1) — `Source/DSP/Effects/AquaticFXSuite.h`
**Commit:** `217f4c209` (2026-03-18)
- 6 water-themed master effects: Reef, Fathom, Drift, Tide + 2 additional
- Master FX slot; plugs into XOlokunProcessor post-mix chain

### Mathematical FX Chain — `Source/DSP/Effects/MathFXChain.h`
**Commit:** `cf6ab75c0` (2026-03-18)
- 4 math-driven processors: Entropy, Voronoi, Quantum, Attractor
- Part of XOM-FX-CORE designation

### Boutique FX Chain — `Source/DSP/Effects/BoutiqueFXChain.h`
**Commit:** `b60a30377` (2026-03-18)
- 4 environment generators: Anomaly, Archive, Cathedral, Submersion
- Part of XOM-FX-BOUTIQUE designation

**Total new FX parameters across 3 chains:** 63

---

## SDK — Phase 1

**Commit:** `8f7cd9fa6` (2026-03-18)
**Location:** `SDK/include/xolokun/`

- `SynthEngine.h` — JUCE-free engine interface
- `CouplingTypes.h` — 13 coupling type definitions
- `EngineModule.h` — base module contract
- Minimal engine template for third-party developers
- No JUCE dependency in SDK headers

---

## DSP / Audio Fixes

| Fix | Commit | Detail |
|-----|--------|--------|
| OVERLAP stereo bioluminescence | `527aa9464` | Fixed mono collapse in Bioluminescence.h |
| OVERLAP PolyBLEP | `527aa9464` | Wired PolyBLEP anti-aliasing to output path |
| OUTWIT D004 dead param | `527aa9464` | `owit_armDepth` now routed to DSP |
| OUTWIT adapter: 5 seance findings | `85db8676a` | Pitch wheel, symmetric pans, 5 seance P0s |
| OVERLAP adapter: modFilterCutoffCache | `d9d6d762c` | Fixed stale cache bug in filter cutoff modulation |
| OCTOPUS D001 velocity→filter | `f4c1842a5` | Doctrine violation resolved |
| OCTOPUS LFO2 wired | `f4c1842a5` | Second LFO was disconnected |
| OCTOPUS AudioToRing applied | `f4c1842a5` | Coupling input now routes to ring modulator |
| OVERLAP/OUTWIT JUCE 8 include | `f4c1842a5` | `JuceHeader.h` → JUCE 8 include pattern; Source/ path added |
| OBRIX default source | `1fd9cbda1` | Changed init default from Sine → Saw (Producer's Guild P1) |
| Drift envelope setParams | `36639bf7f` | Moved to once-per-block — was running per-sample |
| PresetManager coupling validation | `0643ed15d` | Added validation for malformed coupling route arrays |
| DSP safety hardening | `79de022bf` | Denormal protection, null-pointer guards across 8 engines |
| OCEANDEEP filter env + pitch bend | `1f99ebb50` | P0 bugs from seance fixed |
| SilenceGate fleet | `fd8bec718` | SRO SilenceGate applied fleet-wide to prevent NaN blowup |
| SRO FastMath | `fd8bec718` | Fast approximation math (sin/exp) in hot DSP paths |
| Wave 1 Guru Bin quality pass | `c2202edd9` | OVERDUB + OBLONG sound quality improvements |
| Wave 1 audit cycle | `e702081a9` | Code quality + UI/UX fixes across 8 engines |

---

## Preset Library

### Scale

| Milestone | Date | Count |
|-----------|------|-------|
| Pre-sweep baseline | 2026-03-01 | ~2,369 presets (from release_readiness_12k.md) |
| Wave 4 deficit fill | 2026-03-19 | +750 presets (OSTINATO/OPENSKY/OCEANDEEP/OUIE/OBRIX — 150 each) |
| OVERTONE + ORGANISM | 2026-03-19 | +3,641 presets via cherry-pick (includes agent batch) |
| ORBWEAVE | 2026-03-19/20 | +297 presets (2 agents merged) |
| Overnight agent run | 2026-03-19 | +9 straggler presets |
| Straggler fills (OBRIX, OPENSKY, etc.) | 2026-03-19 | +various |
| **V1 total** | 2026-03-20 | **21,918 presets** |

### Quality Operations

| Operation | Commit | Detail |
|-----------|--------|--------|
| Author field normalization | `fb5031953` | 3,032 presets: "XO_OX" → "XO_OX Designs" |
| Case-duplicate fix | `6ef00a4ef` | Removed `Overworld_` variant, kept canonical `OVERWORLD_` |
| Round 12B deduplication | prior to this sprint | 57 duplicate names + 313 underscore violations resolved |
| DNA 100% fleet coverage | Prism Sweep | All 6 dimensions present on all presets |
| Mood coverage | Prism Sweep | 8 moods (Foundation, Atmosphere, Entangled, Prism, Flux, Aether, Family, Submerged) fully covered |

---

## Coupling System

| Change | Commit | Detail |
|--------|--------|--------|
| 13th coupling type: KnotTopology | `58909006b` | Bidirectional topological coupling (ORBWEAVE) |
| AudioToBuffer Phase 3 spec | `11e1986f3` | Phase 3 architecture document written; IAudioBufferSink interface defined |
| Round 13 coupling stubs | `11e1986f3` | Coupling input stubs advanced for Drift FX gap |
| OVERLAP adapter pitch wheel | `d9d6d762c` | Pitch wheel handler added |
| OUTWIT adapter pitch wheel | `85db8676a` | Pitch wheel handler added |
| Coupling preset library | Prism Sweep R8C | 18 coupling presets (6 pairs × 3 intensities) in Entangled mood |
| XVC demo presets | Prism Sweep R10D | 11 ONSET XVC demo kits |

---

## Tooling (Oxport / Tools/)

| Change | Commit | Detail |
|--------|--------|--------|
| Oxport sprint | `a6e9830e3` | 165 low-ROI tools archived; xpn_validator wired; 3 new tools; user guide written |
| Tool Doctrines T001–T006 | `74430ad50` | 6 doctrines ratified for Oxport/Kai tool suite |
| oxport_render tool | `ce056cdb2` | New render tool added + CMake wired |
| validate + batch subcommands | `289bdcf05` | oxport.py CLI gains `validate` and `batch` subcommands |
| Fleet Render Phase 1 | `b76177610` | MIDI + loopback WAV recording infrastructure |
| Q-Link name validation | `ad6b28db0` | Added Q-Link name length validation to QA pipeline |
| Python type hints + docstrings | `267b24389` | Type annotations added to Tools/*.py; hardcoded path audit |
| XPN class rename | `4f64b8b89` | `XPNExporter` → `XOriginate` (XO + O-word convention) |
| XOutshine engine | `75d4c3661` | C++ sample pack upgrader engine integrated |

---

## Build System

| Change | Commit | Detail |
|--------|--------|--------|
| OVERLAP/OUTWIT CMake include paths | `f4c1842a5` | Added `Source/` prefix to include paths; fixed JUCE 8 header |
| Register ORBWEAVE + OBRIX | `a689222b4` | Both engines registered in XOlokunProcessor.cpp |
| Fix missing addParameters() | `a689222b4` | 6 engines (OBRIX, OSTINATO, OUIE, OVERTONE, ORGANISM, ORBWEAVE) had missing `addParameters()` calls |
| DSP embedding | `38dc70bf3` | XOverlap/XOutwit/XOverworld DSP copied inline — zero sibling repo dependencies |
| Oscar Rive gating | per CLAUDE.md | `XO_HAS_RIVE` + `XO_HAS_OSCAR` CMake flags — builds cleanly without rive-cpp |
| SRO FastMath | `fd8bec718` | FastMath.h added to `Source/DSP/` |
| **Final build status** | 2026-03-23 | AU PASS + `auval -v aumu Xomn XoOx` PASS; 73 engines (OXYTOCIN + OUTLOOK added 2026-03-23); 8.9 MB bundle |

---

## Shaper Bus (Post-V1 Architecture — Phase 1 Spec)

**Commit:** `fd773bf3c` (2026-03-19)

- `ShaperEngine` interface defined
- XObserve (parametric EQ; Mantis Shrimp `#00CED1→#7B2D8B`) — architecture spec written
- XOxide (2D character shaper; Sea Urchin `#43B3AE`) — architecture spec written
- These are post-V1 utility engines; spec committed, no DSP yet

---

## Documentation

| Doc | Path |
|-----|------|
| Fleet health dashboard | `Docs/fleet_health_2026_03_20.md` |
| Post-completion checklist | `Docs/post_completion_checklist_2026_03_20.md` |
| OBRIX Fab Five report | `Docs/fab_five_obrix_2026_03_19.md` |
| OVERLAP synthesis guide | `Docs/overlap_synthesis_guide.md` |
| OUTWIT synthesis guide | `Docs/outwit_synthesis_guide.md` |
| Sweep report | `Docs/sweep_report_2026_03_20.md` |
| Sprint report | `Docs/sprint_report_2026-03-18.md` |
| Morning plan | `Docs/morning_plan_2026-03-20.md` |
| Docs INDEX | `Docs/INDEX.md` — 164+ files catalogued |
| SDK Phase 1 | `SDK/include/xolokun/` |

---

## Engine Registration Summary (V1 Final)

**73 engines registered** in `Source/XOlokunProcessor.cpp`

| Category | Engines |
|----------|---------|
| Original fleet (Prism Sweep) | ODDFELIX, ODDOSCAR, OVERDUB, ODYSSEY, OBLONG, OBESE, ONSET, OVERWORLD, OPAL, ORBITAL, ORGANON, OUROBOROS, OBSIDIAN, OVERBITE, ORIGAMI, ORACLE, OBSCURA, OCEANIC, OCELOT, OPTIC, OBLIQUE, OSPREY, OSTERIA, OWLFISH |
| Constellation (stubs) | OHM, ORPHICA, OBBLIGATO, OTTONI, OLE |
| March 15 additions | OVERLAP, OUTWIT, OMBRE, ORCA, OCTOPUS |
| March 18 additions | OSTINATO, OPENSKY, OCEANDEEP, OUIE, OBRIX |
| March 19-20 additions | ORBWEAVE, OVERTONE, ORGANISM |
| March 20 additions (Singularity) | OXBOW, OWARE |
| March 21 additions | OPERA, OFFERING, OSMOSIS |
| March 23 additions | OTO, OCTAVE, OLEG, OTIS, OVEN, OCHRE, OBELISK, OPALINE, OGRE, OLATE, OAKEN, OMEGA, ORCHARD, OVERGROW, OSIER, OXALIS, OVERWASH, OVERWORN, OVERFLOW, OVERCAST, OASIS, ODDFELLOW, ONKOLO, OPCODE (Kitchen Collection), OXYTOCIN, OUTLOOK |

---

## Known Deferred Items (Non-Blocking)

- AudioToBuffer Phase 3: full cycle detection graph, FREEZE state, `IAudioBufferSink` interface, UI slot assignment
- SNAP `AmpToFilter` — highest-value unimplemented coupling wire
- OCELOT + OWLFISH `applyCouplingInput` stubs
- Drift FX gap — 1,353 standalone XOdyssey FX params not yet exposed in DriftEngine adapter
- `juce::Font(name,size,style)` — 5 deprecation warnings in XOlokunEditor.h (pre-existing)
- `float→int` implicit conversion — 1 warning in MasterFXSequencer.h (pre-existing)
- Orbweave preset library: 0 presets on fleet dashboard (note: 297 presets were merged but may not be reflected in dashboard scan)

---

*Generated 2026-03-20. Source: git log --since=2026-03-01, Docs/release_readiness_12k.md, Docs/fleet_health_2026_03_20.md, CLAUDE.md.*
