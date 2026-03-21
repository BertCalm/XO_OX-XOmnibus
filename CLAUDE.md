# XOmnibus — Claude Code Project Guide

## Product Identity

XOmnibus ("for all") is a free, open-source multi-engine synthesizer platform by **XO_OX Designs**.
It merges character instruments into one unified creative environment where engines couple, collide,
and mutate into sounds impossible with any single synth. **46 engines** are registered in XOmnibus
(5 Constellation family engines added 2026-03-14; OVERLAP + OUTWIT installed 2026-03-15; OMBRE, ORCA, OCTOPUS confirmed 2026-03-15, auval PASS; OSTINATO added 2026-03-18; OPENSKY added 2026-03-18; OCEANDEEP added 2026-03-18; OUIE added 2026-03-18; OBRIX added 2026-03-19; ORBWEAVE, OVERTONE, ORGANISM added 2026-03-20; OXBOW added 2026-03-20; OWARE added 2026-03-20; OPERA added 2026-03-21; OFFERING added 2026-03-21)
— see engine table below.

- **Engine modules (registered):** ODDFELIX, ODDOSCAR, OVERDUB, ODYSSEY, OBLONG, OBESE, ONSET, OVERWORLD, OPAL, ORBITAL, ORGANON, OUROBOROS, OBSIDIAN, OVERBITE, ORIGAMI, ORACLE, OBSCURA, OCEANIC, OCELOT, OPTIC, OBLIQUE, OSPREY, OSTERIA, OWLFISH, OHM, ORPHICA, OBBLIGATO, OTTONI, OLE, OVERLAP, OUTWIT, OMBRE, ORCA, OCTOPUS, OSTINATO, OPENSKY, OCEANDEEP, OUIE, OBRIX, ORBWEAVE, OVERTONE, ORGANISM, OXBOW, OWARE, OPERA, OFFERING
- **Coupling:** Cross-engine modulation via MegaCouplingMatrix (14 coupling types incl. KnotTopology)
- **PlaySurface:** 4-zone unified playing interface (Pad/Fretless/Drum modes)
- **Presets:** ~16,000+ factory presets in `.xometa` format, 8 mood categories (Foundation, Atmosphere, Entangled, Prism, Flux, Aether, Family, Submerged), 6D Sonic DNA
- **Formats:** AU, Standalone (macOS); AUv3, Standalone (iOS); VST3 (v2)
- **Design:** Gallery Model — warm white shell frames engine accent colors. Light mode default.

## Brand Rules

- All XO_OX instruments follow the **XO + O-word** naming convention
- Character over feature count — every feature must support a sonic pillar
- Dry patches must sound compelling before effects are applied
- Presets are a core product feature, not an afterthought
- Light mode is the primary presentation. Dark mode is a toggle.

## Architecture Rules

- **Never** allocate memory on the audio thread
- **Never** perform blocking I/O on the audio thread
- **Never** rename stable parameter IDs after release
- All DSP lives inline in `.h` headers; `.cpp` files are one-line stubs
- DSP modules must be testable in isolation
- Export systems must run on non-audio worker threads
- Denormal protection required in all feedback/filter paths
- Engine hot-swap uses 50ms crossfade to prevent clicks
- ParamSnapshot pattern: cache all parameter pointers once per block

## Engine Modules

| Short Name | Source Instrument | Accent Color |
|-----------|------------------|-------------|
| ODDFELIX | OddfeliX (feliX the neon tetra) | Neon Tetra Blue `#00A6D6` |
| ODDOSCAR | OddOscar (Oscar the axolotl) | Axolotl Gill Pink `#E8839B` |
| OVERDUB | XOverdub | Olive `#6B7B3A` |
| ODYSSEY | XOdyssey | Violet `#7B2D8B` |
| OBLONG | XOblong | Amber `#E9A84A` |
| OBESE | XObese | Hot Pink `#FF1493` |
| ONSET | XOnset | Electric Blue `#0066FF` |
| OVERWORLD | XOverworld | Neon Green `#39FF14` |
| OPAL | XOpal | Lavender `#A78BFA` |
| ORGANON | XOrganon | Bioluminescent Cyan `#00CED1` |
| OUROBOROS | XOuroboros | Strange Attractor Red `#FF2D2D` |
| OBSIDIAN | XObsidian | Crystal White `#E8E0D8` |
| ORIGAMI | XOrigami | Vermillion Fold `#E63946` |
| ORACLE | XOracle | Prophecy Indigo `#4B0082` |
| OBSCURA | XObscura | Daguerreotype Silver `#8A9BA8` |
| OCEANIC | XOceanic | Phosphorescent Teal `#00B4A0` |
| OCELOT | XOcelot | Ocelot Tawny `#C5832B` |
| OVERBITE | XOverbite | Fang White `#F0EDE8` |
| ORBITAL | XOrbital | Warm Red `#FF6B6B` |
| OPTIC | XOptic | Phosphor Green `#00FF41` |
| OBLIQUE | XOblique | Prism Violet `#BF40FF` |
| OSPREY | XOsprey | Azulejo Blue `#1B4F8A` |
| OSTERIA | XOsteria | Porto Wine `#722F37` |
| OWLFISH | XOwlfish | Abyssal Gold `#B8860B` |
| OHM | XOhm | Sage `#87AE73` |
| ORPHICA | XOrphica | Siren Seafoam `#7FDBCA` |
| OBBLIGATO | XObbligato | Rascal Coral `#FF8A7A` |
| OTTONI | XOttoni | Patina `#5B8A72` |
| OLE | XOlé | Hibiscus `#C9377A` |
| OVERLAP | XOverlap | Bioluminescent Cyan-Green `#00FFB4` |
| OUTWIT | XOutwit | Chromatophore Amber `#CC6600` |
| OMBRE | XOmbre | Shadow Mauve `#7B6B8A` |
| ORCA | XOrca | Deep Ocean `#1B2838` |
| OCTOPUS | XOctopus | Chromatophore Magenta `#E040FB` |
| OSTINATO | XOstinato | Firelight Orange `#E8701A` |
| OPENSKY | XOpenSky | Sunburst `#FF8C00` |
| OCEANDEEP | XOceanDeep | Trench Violet `#2D0A4E` |
| OUIE | XOuïe | Hammerhead Steel `#708090` |
| OBRIX | XObrix | Reef Jade `#1E8B7E` |
| ORBWEAVE | XOrbweave | Kelp Knot Purple `#8E4585` |
| OVERTONE | XOvertone | Spectral Ice `#A8D8EA` |
| ORGANISM | XOrganism | Emergence Lime `#C6E377` |
| OXBOW | XOxbow | Oxbow Teal `#1A6B5A` |
| OWARE | XOware | Akan Goldweight `#B5883E` |
| OPERA | XOpera | Aria Gold `#D4AF37` |
| OFFERING | XOffering | Crate Wax Yellow `#E5B80B` |

### Engine ID vs Parameter Prefix

Engine IDs (used in preset `"engines"` arrays, `"parameters"` keys, UI, and coupling routes)
were renamed to O-prefix convention. **Parameter prefixes are frozen and never change:**

| Engine ID | Parameter Prefix | Example |
|-----------|-----------------|---------|
| OddfeliX | `snap_` | `snap_filterCutoff` |
| OddOscar | `morph_` | `morph_morph` |
| Overdub | `dub_` | `dub_oscWave` |
| Odyssey | `drift_` | `drift_oscA_mode` |
| Oblong | `bob_` | `bob_fltCutoff` |
| Obese | `fat_` | `fat_satDrive` |
| Overbite | `poss_` | `poss_biteDepth` |
| Onset | `perc_` | `perc_noiseLevel` |
| Overworld | `ow_` | `ow_era` |
| Opal | `opal_` | `opal_grainSize` |
| Orbital | `orb_` | `orb_brightness` |
| Organon | `organon_` | `organon_metabolicRate` |
| Ouroboros | `ouro_` | `ouro_topology` |
| Obsidian | `obsidian_` | `obsidian_depth` |
| Origami | `origami_` | `origami_foldPoint` |
| Oracle | `oracle_` | `oracle_breakpoints` |
| Obscura | `obscura_` | `obscura_stiffness` |
| Oceanic | `ocean_` | `ocean_separation` |
| Ocelot | `ocelot_` | `ocelot_biome` |
| Optic | `optic_` | `optic_pulseRate` |
| Oblique | `oblq_` | `oblq_prismColor` |
| Osprey | `osprey_` | `osprey_shoreBlend` |
| Osteria | `osteria_` | `osteria_qBassShore` |
| Owlfish | `owl_` | `owl_filterCutoff` |
| Ohm | `ohm_` | `ohm_macroMeddling` |
| Orphica | `orph_` | `orph_pluckBrightness` |
| Obbligato | `obbl_` | `obbl_breathA` |
| Ottoni | `otto_` | `otto_macroGrow` |
| Ole | `ole_` | `ole_macroDrama` |
| Ombre | `ombre_` | `ombre_blend` |
| Orca | `orca_` | `orca_huntMacro` |
| Octopus | `octo_` | `octo_armDepth` |
| Ostinato | `osti_` | `osti_macroGather` |
| OpenSky | `sky_` | `sky_macroRise` |
| OceanDeep | `deep_` | `deep_macroPressure` |
| Ouie | `ouie_` | `ouie_macroHammer` |
| Overlap | `olap_` | `olap_knotDepth` |
| Outwit | `owit_` | `owit_armDepth` |
| Obrix | `obrix_` | `obrix_src1Type` |
| Orbweave | `weave_` | `weave_knotDepth` |
| Overtone | `over_` | `over_cfDepth` |
| Organism | `org_` | `org_ruleSet` |
| Oxbow | `oxb_` | `oxb_entangle` |
| Oware | `owr_` | `owr_material` |
| Opera | `opera_` | `opera_drama` |
| Offering | `ofr_` | `ofr_digCuriosity` |

Legacy engine names (`Snap`, `Morph`, `Dub`, `Drift`, `Bob`, `Fat`, `Bite`)
are resolved automatically by `resolveEngineAlias()` in `PresetManager.h`.
See `Docs/xomnibus_name_migration_reference.md` for the full mapping and gotchas.

## Key Files

| Path | Purpose |
|------|---------|
| `Docs/xomnibus_master_specification.md` | **THE** single source of truth |
| `Docs/xomnibus_name_migration_reference.md` | Legacy → canonical engine name mapping |
| `Source/Core/SynthEngine.h` | Engine interface (all engines implement this) |
| `Source/Core/EngineRegistry.h` | Factory + 4-slot management |
| `Source/Core/MegaCouplingMatrix.h` | Cross-engine modulation |
| `Source/Core/PresetManager.h` | .xometa loading/saving |
| `Source/Engines/*/` | Engine adapter modules |
| `Source/Engines/Optic/OpticEngine.h` | Visual modulation engine + AutoPulse |
| `Source/Engines/Oblique/ObliqueEngine.h` | Prismatic bounce engine (RTJ x Funk x Tame Impala) |
| `Source/Engines/Ombre/OmbreEngine.h` | Dual-narrative engine (memory/forgetting + perception) |
| `Source/Engines/Orca/OrcaEngine.h` | Apex predator engine (wavetable + echolocation + breach) |
| `Source/Engines/Octopus/OctopusEngine.h` | Decentralized alien intelligence engine (arms + chromatophores + ink cloud) |
| `Source/Engines/OpenSky/OpenSkyEngine.h` | Euphoric shimmer synth (supersaw + shimmer reverb + chorus + unison) |
| `Source/Engines/Ouie/OuieEngine.h` | Duophonic hammerhead synth (2 voices x 8 algorithms + STRIFE/LOVE interaction) |
| `Source/Engines/Orbweave/OrbweaveEngine.h` | Topological knot coupling engine (Kelp Knot) |
| `Source/Engines/Overtone/OvertoneEngine.h` | Continued fraction spectral engine (Nautilus) |
| `Source/Engines/Organism/OrganismEngine.h` | Cellular automata generative engine (Coral Colony) |
| `Source/UI/OpticVisualizer/OpticVisualizer.h` | Winamp-style audio-reactive visualizer |
| `Docs/xomnibus_sound_design_guides.md` | Sound design guide (38 of 44 engines covered; OBRIX, ORBWEAVE, OVERTONE, ORGANISM, OXBOW, OWARE need entries) |
| `Source/DSP/` | Shared DSP library |
| `Source/UI/` | Gallery Model UI components |
| `Source/Export/` | XPN export pipeline |
| `Source/Engines/Obrix/ObrixEngine.h` | Modular brick synthesis engine (coral reef) |
| `Source/DSP/Effects/AquaticFXSuite.h` | Aquatic FX chain (Reef + Fathom + Drift + Tide) |
| `Source/DSP/Effects/MathFXChain.h` | Mathematical FX chain (Entropy + Voronoi + Quantum + Attractor) |
| `Source/DSP/Effects/BoutiqueFXChain.h` | Boutique FX chain (Anomaly + Archive + Cathedral + Submersion) |
| `Source/DSP/Effects/fXOnslaught.h` | Singularity FX: transient-reactive chorus → PM collapse |
| `Source/DSP/Effects/fXObscura.h` | Singularity FX: inverse-dynamic spectral degradation (chiaroscuro) |
| `Source/DSP/Effects/fXOratory.h` | Singularity FX: poetic meter-driven multi-tap delay |
| `Source/Engines/Oxbow/OxbowEngine.h` | Entangled reverb synth engine (Chiasmus FDN + phase erosion + golden resonance) |
| `Source/Engines/Oware/OwareEngine.h` | Tuned percussion (material continuum + mallet physics + sympathetic resonance + buzz membrane) |
| `Source/Engines/Opera/OperaAdapter.h` | Additive-vocal Kuramoto synchronicity engine (formant synthesis + phase transition + Conductor arcs) |
| `Source/Engines/Offering/OfferingEngine.h` | Psychology-driven boom bap drum synthesis (Berlyne curiosity + 5 city chains + per-type transient models) |
| `SDK/include/xomnibus/` | JUCE-free SDK headers for third-party engine development |
| `Presets/XOmnibus/{mood}/` | Factory presets by mood |
| `Tools/` | Python utilities (DNA, breeding, migration, export) |
| `Docs/` | All specification documents |

## Preset System

- `.xometa` JSON files are the source of truth (version-controlled)
- 8 moods: Foundation, Atmosphere, Entangled, Prism, Flux, Aether, Family, Submerged
- 4 macros: CHARACTER, MOVEMENT, COUPLING, SPACE
- 6D Sonic DNA: brightness, warmth, movement, density, space, aggression
- Naming: 2-3 words, evocative, max 30 chars, no duplicates, no jargon

## Design System

- **Gallery Model:** Warm white shell `#F8F6F3` frames engine accent colors
- **XO Gold:** `#E9C46A` — brand constant (macros, coupling strip, active states)
- **Typography:** Space Grotesk (display), Inter (body), JetBrains Mono (values)
- **Light mode default**, dark mode toggle

## Build

```bash
# macOS build
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build

# iOS build
cmake -B build-ios -G Xcode \
  -DCMAKE_TOOLCHAIN_FILE=ios-toolchain.cmake \
  -DCMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM=YOUR_TEAM_ID
cmake --build build-ios --config Release
```

## XPN Export (MPC Compatibility)

3 critical XPM rules — no exceptions:
- `KeyTrack` = `True` (samples transpose across zones)
- `RootNote` = `0` (MPC auto-detect convention)
- Empty layer `VelStart` = `0` (prevents ghost triggering)

## Adding New Engines

New engines are designed as standalone instruments first, then integrated into XOmnibus.

**Invoke:** `/new-xo-engine` — walks through ideation, architecture, scaffold, and integration prep.

**Full process:** `Docs/xomnibus_new_engine_process.md`

**Quick rules for XOmnibus-ready standalone development:**
- Parameter IDs use `{shortname}_{paramName}` format from day one
- Presets use `.xometa` JSON format from day one
- DSP lives in inline `.h` headers (portable)
- DSP works without UI references (just parameters)
- M1-M4 macros produce audible change in every preset
- Define coupling compatibility early (which `CouplingType` enums you accept)

**Integration path:** Write a thin adapter implementing `SynthEngine` → `REGISTER_ENGINE()` → copy presets → done.

## OBRIX — Wave 5: Reef Residency (2026-03-21)

**81 params** (was 79). Coupling input becomes a third ecological organism in the Brick Ecology system.

| System | Params | Key Behavior |
|--------|--------|-------------|
| Reef Residency | reefResident, residentStrength | 4-mode coupling ecology (Off/Competitor/Symbiote/Parasite). Competitor: coupling RMS suppresses both sources (0.1 floor). Symbiote: coupling amplitude drives FM depth + boosts Harmonic Field. Parasite: coupling energy feeds stressLevel_ and bleachLevel_ accumulators over time. Default Off (backward compatible). residentStrength default 0.3 (Guru Bin). |

Previous Waves preserved — all existing behavior unchanged when reefResident=Off.

### Wave 4: Biophonic Synthesis (14 params: 65→79)

| System | Params | Key Behavior |
|--------|--------|-------------|
| Harmonic Field | fieldStrength, fieldPolarity, fieldRate, fieldPrimeLimit | Per-voice JI attractor/repulsor. 3-limit/5-limit/7-limit ratio tables. IIR convergence toward nearest just ratio. fieldPolarity=-1 = repulsor mode. |
| Environmental | envTemp, envPressure, envCurrent, envTurbidity | Temperature scales drift depth. Pressure scales LFO rate. Current biases cutoff ±2000Hz. Turbidity adds engine-level noise per sample. |
| Brick Ecology | competitionStrength, symbiosisStrength | Competition: cross-amplitude suppression between src1/src2 (0.1 floor). Symbiosis: noise src1 amplitude → FM depth on src2 (requires src1Type=5). |
| Stateful Synthesis | stressDecay, bleachRate, stateReset | stressLevel_ leaky integrator (velocity accumulator → +900Hz cutoff). bleachLevel_ accumulates from high notes (−700Hz cutoff). stateReset clears both. |
| FX Mode | fxMode | 0=Serial (existing), 1=Parallel (each FX slot processes dry independently, wet contributions summed). |

**28 Wave 4 awakening presets** across all 8 moods. B016 AMENDED (synthesis-layer interdependence permitted, MIDI-layer inviolable).

## Seance Findings

**45 of 45 engines seanced** (2026-03-21) — All engines complete. OPERA: 8.7/10 (path to 9.0+ via init state fix + velocity pass — see `Docs/seances/opera-seance-2026-03-21.md`). OXBOW: ~8.2/10 pre-fix → 9.0 post-fix. OWARE: 8.4/10 initial → 9.2 post-fix (LFO1/LFO2 wired, shimmerRate param wired, buzzAmount 0.15 default, lfo1Depth 0.1 default). 22 engines received DSP fixes this session; fleet average ~8.8/10 (up from ~7.2; +0.1 after 5-engine D002/D004 fix pass 2026-03-21). Five engines at 9.0+ (OVERBITE 9.2, OWARE 9.2, OBSCURA 9.1, OUROBOROS 9.0, OXBOW 9.0). **2026-03-21 post-fix rescoring**: OBESE 6.6→~8.5 (LFO1 exposed, B015 Mojo now breathes), ODDOSCAR 6.9→~8.5 (LFO1 added, aftertouch→resonance wired), ODDFELIX ~7.0→~8.5 (LFO rates exposed, aftertouch-rate wired), OCELOT 6.4→~8.5 (biome crossfade was dead — setBiomeTarget() never called — now live), ORPHICA 8.0→~8.7 (buffer extended to 1s, velocity→body resonance frequency wired). All 44 previous engines at 8.0+ target. OPERA 8.7/10 with no DSP bugs. Full data in:
- Fleet scores: `Docs/fleet-seance-scores-2026-03-20.md`
- Post-fix rescoring: `Docs/post-fix-rescoring-2026-03-20.md`
- Producer's Guild review: `Docs/producers-guild-fleet-review-2026-03-20.md`
- Seance review of Guild plan: `Docs/seance-review-guild-plan-2026-03-20.md`
- Grand Survey: `Docs/xomnibus_landscape_2026.md`
- Cross-reference: `Docs/seance_cross_reference.md`
- Individual verdicts: `Docs/seances/` and `scripture/seances/`

### The 6 Doctrines

| ID | Doctrine | Summary |
|----|----------|---------|
| D001 | Velocity Must Shape Timbre | Velocity drives filter brightness / harmonic content — not just amplitude |
| D002 | Modulation is the Lifeblood | Min: 2 LFOs, mod wheel/aftertouch, 4 working macros, 4+ mod matrix slots |
| D003 | The Physics IS the Synthesis | Rigor and citation required for any physically-modeled engine |
| D004 | Dead Parameters Are Broken Promises | Every declared parameter must affect audio output |
| D005 | An Engine That Cannot Breathe Is a Photograph | Every engine needs at least one LFO with rate floor ≤ 0.01 Hz |
| D006 | Expression Input Is Not Optional | Velocity→timbre + at least one CC (aftertouch / mod wheel / expression) |

### The 37 Blessings

| ID | Blessing | Engine |
|----|----------|--------|
| B001 | Group Envelope System — crowned by Moog + Smith | ORBITAL |
| B002 | XVC Cross-Voice Coupling — all 8 ghosts, 3-5 years ahead | ONSET |
| B003 | Leash Mechanism — chaotic system with a leash | OUROBOROS |
| B004 | Spring Reverb — Vangelis + Tomita praised the metallic splash | OVERDUB |
| B005 | Zero-Audio Identity — synthesis without sound | OPTIC |
| B006 | Dual-Layer Blend Architecture — Circuit + Algorithm crossfade | ONSET |
| B007 | Velocity Coupling Outputs — velocity as coupling signal | OUROBOROS |
| B008 | Five-Macro System (BELLY/BITE/SCURRY/TRASH/PLAY DEAD) — all 8 ghosts | OVERBITE |
| B009 | ERA Triangle — 2D timbral crossfade, Buchla/Schulze/Vangelis/Pearlman | OVERWORLD |
| B010 | GENDY Stochastic Synthesis + Maqam — Buchla gave 10/10 | ORACLE |
| B011 | Variational Free Energy Metabolism — unanimous; publishable as paper | ORGANON |
| B012 | ShoreSystem — 5-coastline cultural data shared across engines | OSPREY + OSTERIA |
| B013 | Chromatophore Modulator — praised by Buchla, Schulze, Kakehashi, Tomita | OCEANIC |
| B014 | Mixtur-Trautonium Oscillator — unanimous praise, genuinely novel | OWLFISH |
| B015 | Mojo Control — orthogonal analog/digital axis | OBESE |
| B016 | Brick Independence (AMENDED 2026-03-21) — MIDI-layer voice independence is inviolable (note allocation, pitch, velocity, aftertouch remain per-voice). Synthesis-layer interdependence (shared JI attractor field, cross-voice amplitude ecology, environmental globals) is explicitly permitted, provided no synthesis-layer coupling propagates back to affect MIDI routing or voice stealing. | OBRIX |
| B017 | Modal Membrane Synthesis with Academic Citation — Bessel zeros sourced from physics papers | OSTINATO |
| B018 | Circular Topology Coupling (CIRCLE) — adjacent seats trigger sympathetic ghost responses | OSTINATO |
| B019 | 96 Hand-Authored World Rhythm Patterns — 8 instruments × 8 patterns, ethnomusicologically sourced | OSTINATO |
| B020 | Live Override with Graceful Yield — machine suppresses for one bar when live MIDI arrives | OSTINATO |
| B021 | Knot Phase Coupling Matrix — topological routing via Trefoil/Figure-Eight/Torus/Solomon matrices | ORBWEAVE |
| B022 | MACRO KNOT: Continuous Topology Morphing — real-time traversal of topological space as gesture | ORBWEAVE |
| B023 | Shepard Shimmer Architecture — dual-interval reverb creates endlessly ascending shimmer tail | OPENSKY |
| B024 | RISE Macro: Single-Gesture Ascension — simultaneously sweeps pitch env, filter, shimmer | OPENSKY |
| B025 | HAMMER Interaction Axis — bipolar axis from STRIFE (ring mod / hard sync) to LOVE (harmonic convergence) | OUIE |
| B026 | Interval-as-Parameter — musical interval between two voices is a first-class timbral parameter | OUIE |
| B027 | 8-Algorithm Palette — each voice selects independently from 8 synthesis algorithms (64 combinations) | OUIE |
| B028 | Continued Fraction Convergent Synthesis — timbre derived from rational approximations to π, e, φ, √2 | OVERTONE |
| B029 | Hydrostatic Compressor — pressure as environment, not utility; simulates depth-of-water physics | OCEANDEEP |
| B030 | Bioluminescent Exciter — complete micro-synthesis engine generating organic alien texture | OCEANDEEP |
| B031 | Darkness Filter Ceiling — creative restriction 50–800 Hz; constraint as identity declaration | OCEANDEEP |
| B032 | Mallet Articulation Stack — Chaigne 1997 contact-time synthesis creates physical pluck/strike/brush modes with academic citation | OWARE |
| B033 | Living Tuning Grid — 8-mode layout where each mode creates a distinct intonation field via harmonic series + sympathetic resonance combination | OWARE |
| B034 | Per-Mode Sympathetic Network — 5 sympathetically resonating strings with unique ratios per mode (5×8 = 40 unique resonance profiles) | OWARE |
| B035 | OperaConductor: Autonomous Dramatic Arc Architecture — 4 arc shapes, configurable arc time/peak, ±5%/±3% jitter, graceful player override via max(conductorK, manualK). First synthesizer in the fleet with autonomous narrative intent. Blessed by Kakehashi, Pearlman, Vangelis, Smith. | OPERA |
| B036 | Coherence-Driven Spatial Panning — stereo field is a live readout of Kuramoto order parameter R: locked partials spread wide, chaotic partials collapse to center. The stereo field breathes with the dramatic arc. Blessed by Tomita, Buchla. | OPERA |
| B037 | EmotionalMemory: Phase Persistence Across Note Boundaries — partial phases stored at note-off, recalled within 500ms window. The Kuramoto field wakes up knowing where it was. Named after Vangelis in source code. Blessed by Vangelis. | OPERA |

### The 5 Ongoing Debates

| ID | Tension | Status |
|----|---------|--------|
| DB001 | Mutual exclusivity vs. effect chaining (Buchla vs. field) | EXPANDED — OSTINATO introduces behavioral chaining as a new axis; fleet-level unresolved |
| DB002 | Silence as paradigm vs. accessibility (Schulze/Buchla vs. Kakehashi/Pearlman) | UNRESOLVED |
| DB003 | Init patch: immediate beauty vs. blank canvas (Vangelis/Kakehashi vs. Schulze) | RESOLVED for percussion — immediate beauty wins (OSTINATO seance 2026-03-20) |
| DB004 | Expression vs. Evolution: gesture vs. temporal depth (Vangelis vs. Schulze) | RESOLVED for OSTINATO — expression AND evolution are not in tension; both possible simultaneously |
| DB005 | Autonomy vs. Agency — machine Conductor arc vs. player expressive control (Kakehashi named; OPERA is the first engine where this tension is architecturally explicit). Current resolution: Conductor + Manual/Both mode is correct; debate is about *degree* not *principle*. | OPEN |

### Critical Fleet-Wide Findings

- **Seance score range (post 2026-03-21 fixes)**: ~8.0 (ORIGAMI) to 9.2 (OVERBITE / OWARE). Fleet avg ~8.8. **All 45 engines seanced. OPERA 8.7/10 (no DSP bugs, 3 UX fixes for 9.0+).** Five engines at 9.0+ (OVERBITE 9.2, OWARE 9.2, OBSCURA 9.1, OUROBOROS 9.0, OXBOW 9.0). 5 additional engines at ~8.5 after 2026-03-21 D002/D004 fixes (OBESE, ODDOSCAR, ODDFELIX, OCELOT, ORPHICA ~8.7).
- **Preset expansion ongoing**: all engines now have at least 1 preset; thin coverage engines expanded in Rounds 8–11
- **D006 aftertouch coverage**: 23/23 engines have aftertouch (Optic intentionally exempt — visual engine)
- **D006 mod wheel coverage**: **22/22 MIDI-capable engines — FULLY RESOLVED** (Round 12C completed the last 7 engines)
- **D001 filter envelopes**: RESOLVED — all engines fleet-wide have velocity-scaled filter envelopes (Round 9E)
- **D004 dead params**: RESOLVED — all declared parameters wired to DSP (Round 3B)
- **D005 LFO breathing**: RESOLVED — all engines have autonomous modulation with rate floor ≤ 0.01 Hz (Round 5A + engine recoveries)

### Prism Sweep — COMPLETE (2026-03-14)

12-round progressive quality pass across all 24 original engines. **ALL 12 ROUNDS COMPLETE.**

- All 6 doctrines resolved fleet-wide (D001–D006)
- 22/22 engines with mod wheel | 23/23 engines with aftertouch (Optic intentionally exempt — visual engine)
- ~15,500 presets (was 10,028 at sweep completion), 0 duplicates, 100% DNA coverage, health score ~92/100
- Build PASS + auval PASS
- **Full history**: `Docs/prism_sweep_final_report.md` | Master index: `Docs/prism_sweep_index.md`

---

## Skill Library

Reusable skill guides live in `Skills/` — invoke the relevant one before starting each task type.

| Skill | Invoke | When to Use |
|-------|--------|-------------|
| [coupling-preset-designer](Skills/coupling-preset-designer/SKILL.md) | `/coupling-preset-designer` | Creating Entangled mood presets with cross-engine coupling |
| [coupling-interaction-cookbook](Skills/coupling-interaction-cookbook/SKILL.md) | `/coupling-interaction-cookbook` | Quick lookup: which engines pair best, what types are supported |
| [mod-matrix-builder](Skills/mod-matrix-builder/SKILL.md) | `/mod-matrix-builder` | Adding 8-slot mod matrix to any engine (D002/D005/D006) |
| [preset-architect](Skills/preset-architect/SKILL.md) | `/preset-architect` | Writing any new `.xometa` preset |
| [engine-health-check](Skills/engine-health-check/SKILL.md) | `/engine-health-check` | Quick D001–D006 doctrine check on any engine |
| [dna-designer](Skills/dna-designer/SKILL.md) | `/dna-designer` | Assigning accurate 6D Sonic DNA to presets |
| [xpn-export-specialist](Skills/xpn-export-specialist/SKILL.md) | `/xpn-export-specialist` | Full XPN/MPC export pipeline |
| [midi-daw-audit](Skills/midi-daw-audit/SKILL.md) | `/midi-daw-audit` | MIDI/DAW compatibility audit (CC thread safety, MIDI learn, APVTS exposure, state persistence, MPE, pitch bend, sustain, transport sync, plugin contract) |
| [sro-optimizer](Skills/sro-optimizer/SKILL.md) | `/sro-optimizer` | CPU optimization via Spectral Resonance Objects |
| [preset-auditor](Skills/preset-auditor/SKILL.md) | `/preset-auditor` | Preset quality gate — DNA, macros, D004, naming |
| [coupling-debugger](Skills/coupling-debugger/SKILL.md) | `/coupling-debugger` | Diagnose broken or inaudible coupling routes |
| [master-audit](Skills/master-audit/SKILL.md) | `/master-audit` | Fleet-wide QA orchestration — pre-release health check |
| [repo-audit](Skills/repo-audit/SKILL.md) | `/repo-audit` | Repo hygiene + doc currency — stale counts, broken IDs, governance gaps |
| synth-seance (`~/.claude/skills/`) | `/synth-seance` | Ghost council full engine quality evaluation |
| post-engine-completion-checklist (`~/.claude/skills/`) | `/post-engine-completion-checklist` | 5-point post-build audit |
| producers-guild (`~/.claude/skills/`) | `/producers-guild` | 12-specialist market/product review |
| [preset-audit-checklist](Skills/preset-audit-checklist/SKILL.md) | `/preset-audit-checklist` | 7-phase Guru-informed preset audit — path to 9.0+ libraries (supersedes ad-hoc preset QA) |
| [skill-friction-detective](Skills/skill-friction-detective/SKILL.md) | `/skill-friction-detective` | Meta-skill: detect friction/failures across all skills, then repair and link |

Full index: `Skills/README.md`

---

## Adding a New Engine — CLAUDE.md Checklist

When a new engine is registered, update **all four** of these sections in CLAUDE.md:

1. **Product Identity header** — add the engine name to the `**Engine modules (registered):**` comma list and update the parenthetical note with the date added
2. **Engine Modules table** — add a row: `| SHORTNAME | Source Instrument | Accent Color `#RRGGBB` |`
3. **Parameter Prefix table** — add a row: `| EngineName | \`prefix_\` | \`prefix_exampleParam\` |`
4. **Key Files table** (if the engine has a notable architecture worth linking) — add `| Source/Engines/Name/NameEngine.h | Brief description |`

Then update these external files:
- `Docs/xomnibus_master_specification.md` section 3.1 engine table (add row)
- `Docs/seance_cross_reference.md` (add seance row after the seance is run)
- `Source/XOmnibusProcessor.cpp` (register the engine)
- `Source/Core/PresetManager.h` (add to `validEngineNames` and `frozenPrefixForEngine`)

Full process: `Docs/xomnibus_new_engine_process.md`

---

## Documentation Index

- Full doc inventory: `Docs/MANIFEST.md`
- Documentation governance: `Docs/GOVERNANCE.md`
- Documentation improvement plan: `Docs/documentation_health_plan.md`
- Master specification: `Docs/xomnibus_master_specification.md`
- Discovery index: `Docs/INDEX.md`

---

## Development Workflow

1. Read the master spec (`Docs/xomnibus_master_specification.md`) before making changes
2. Plan before coding — produce architecture + QA plan first
3. All engines must implement the `SynthEngine` interface
4. Parameter IDs are namespaced by engine (e.g., `snap_filterCutoff`, `dub_sendAmount`)
5. Run DSP stability checks after any engine modifications
6. Preserve existing parameter IDs and preset compatibility
7. For new engines, follow the process in `Docs/xomnibus_new_engine_process.md`
