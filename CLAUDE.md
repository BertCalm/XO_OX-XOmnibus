# XOmnibus — Claude Code Project Guide

## Product Identity

XOmnibus ("for all") is a free, open-source multi-engine synthesizer platform by **XO_OX Designs**.
It merges character instruments into one unified creative environment where engines couple, collide,
and mutate into sounds impossible with any single synth. **40 engines** are registered in XOmnibus
(26 original; +5 Constellation 2026-03-14; +OVERLAP+OUTWIT 2026-03-15; +OMBRE+ORCA+OCTOPUS 2026-03-15;
+OPENSKY+OSTINATO+OCEANDEEP+OUIE V1 concept engines 2026-03-17; +OVERTONE+ORGANISM V2 Theorem engines 2026-03-17)
— see engine table below.

- **Engine modules (registered):** ODDFELIX, ODDOSCAR, OVERDUB, ODYSSEY, OBLONG, OBESE, ONSET, OVERWORLD, OPAL, ORBITAL, ORGANON, OUROBOROS, OBSIDIAN, OVERBITE, ORIGAMI, ORACLE, OBSCURA, OCEANIC, OCELOT, OPTIC, OBLIQUE, OSPREY, OSTERIA, OWLFISH, OHM, ORPHICA, OBBLIGATO, OTTONI, OLE, OVERLAP, OUTWIT, OMBRE, ORCA, OCTOPUS, OPENSKY, OSTINATO, OCEANDEEP, OUIE, OVERTONE, ORGANISM
- **Coupling:** Cross-engine modulation via MegaCouplingMatrix (13 coupling types: 12 original + KnotTopology)
- **PlaySurface:** 4-zone unified playing interface (Pad/Fretless/Drum modes)
- **Presets:** 19,578+ factory presets in `.xometa` format, 8 mood categories (incl. Family, Submerged), 6D Sonic DNA
- **Formats:** AU, Standalone (macOS); AUv3, Standalone (iOS); VST3 (v2)
- **Design:** Gallery Model — warm white shell frames engine accent colors. Light mode default.

## Brand Rules

- All XO_OX instruments follow the **XO + O-word** naming convention
- Effect engines use the **fXO_ + O-word** naming convention (e.g., `fXO_Obsession`, `fXO_Orbit`)
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
| OMBRE | XOmbre | Shadow Mauve `#7B6B8A` |
| ORCA | XOrca | Deep Ocean `#1B2838` |
| OCTOPUS | XOctopus | Chromatophore Magenta `#E040FB` |

### V1 Concept Engines — DSP Build Pending (approved concepts, no source code yet)

These engines have identity cards and design specs but zero source code. They are not yet registered
in XOmnibus. They ARE V1 scope (decision 2026-03-14) — do not defer to V2. Scaffold and DSP build
are next up after current Opus sessions resume.

| Short Name | Source Instrument | Accent Color |
|-----------|------------------|-------------|
| OSTINATO | XOstinato | Firelight Orange `#E8701A` |
| OPENSKY | XOpenSky | Sunburst `#FF8C00` |
| OCEANDEEP | XOceanDeep | Trench Violet `#2D0A4E` |
| OUIE | XOuïe | Hammerhead Steel `#708090` |

### Phase 2 Architecture — Effects Ecosystem & Prime Movers (approved 2026-03-17)

**Prime Mover engines:** A new send-only engine class that drives a 4-slot effects chain. Prime Movers generate source audio and feed it into the coupling matrix but never receive coupling input. `MaxSlots` remains 4 for generators; the Prime Mover occupies a separate dedicated slot. Candidates: `XO_Origin` (multi-osc), `XO_Overture` (sample player), `XO_Oscillograph` (wavetable+FM), `XO_Ouverture` (mic/line input).

**fXO_ effects engines:** Built on 6-8 shared effect cores (`ReverbCore`, `DelayCore`, `SaturationCore`, `ModulationCore`, `FilterCore`, `SpatialCore`, `PitchCore`, `DynamicsCore`). Regional engines (e.g., `fXO_Outpost` for Midwest, `fXO_Overcast` for Nordic) are thin adapters that configure 2-3 shared cores into a curated chain. Effects extend `SynthEngine` with optional audio input (Path B). Full design: `Skills/sro-optimizer/SKILL.md` §Phase 2 Roadmap.

**Dynamic oversampling:** Early Phase 2 — `OversamplingManager` component triggers 4×→2×→1× reduction based on `SROAuditor` budget alarm.

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
| `Source/UI/OpticVisualizer/OpticVisualizer.h` | Winamp-style audio-reactive visualizer |
| `Docs/xomnibus_sound_design_guides.md` | Sound design guide (40/40 engines — complete fleet coverage) |
| `Source/DSP/` | Shared DSP library |
| `Source/DSP/SRO/` | SRO framework (SilenceGate, ControlRateReducer, LookupTable, SROAuditor) |
| `Source/UI/` | Gallery Model UI components |
| `Source/Export/` | XPN export pipeline |
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

## Seance Findings

40 seances complete (2026-03-17) — all 40 registered engines covered. Constellation (OHM/ORPHICA/OBBLIGATO/OTTONI/OLE) + OVERLAP/OUTWIT/OMBRE/ORCA/OCTOPUS + OPENSKY/OSTINATO/OCEANDEEP/OUIE/OVERTONE/ORGANISM seances committed across multiple sessions. Full data in:
- Grand Survey: `Docs/xomnibus_landscape_2026.md`
- Knowledge tree: `~/.claude/skills/synth-seance/knowledge/index.md`
- Cross-reference: `Docs/seance_cross_reference.md`

### The 6 Doctrines

| ID | Doctrine | Summary |
|----|----------|---------|
| D001 | Velocity Must Shape Timbre | Velocity drives filter brightness / harmonic content — not just amplitude |
| D002 | Modulation is the Lifeblood | Min: 2 LFOs, mod wheel/aftertouch, 4 working macros, 4+ mod matrix slots |
| D003 | The Physics IS the Synthesis | Rigor and citation required for any physically-modeled engine |
| D004 | Dead Parameters Are Broken Promises | Every declared parameter must affect audio output |
| D005 | An Engine That Cannot Breathe Is a Photograph | Every engine needs at least one LFO with rate floor ≤ 0.01 Hz |
| D006 | Expression Input Is Not Optional | Velocity→timbre + at least one CC (aftertouch / mod wheel / expression) |

### The 15 Blessings

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

### The 4 Ongoing Debates

| ID | Tension | Status |
|----|---------|--------|
| DB001 | Mutual exclusivity vs. effect chaining (Buchla vs. field) | UNRESOLVED |
| DB002 | Silence as paradigm vs. accessibility (Schulze/Buchla vs. Kakehashi/Pearlman) | UNRESOLVED |
| DB003 | Init patch: immediate beauty vs. blank canvas (Vangelis/Kakehashi vs. Schulze) | UNRESOLVED |
| DB004 | Expression vs. Evolution: gesture vs. temporal depth (Vangelis vs. Schulze) | UNRESOLVED |

### Critical Fleet-Wide Findings

- **Seance score range**: 7.2/10 est. (OBLIQUE, recovered) to 8/8 PASS + 8.6/10 (ORGANON, ORACLE)
- **Preset expansion ongoing**: all engines now have at least 1 preset; thin coverage engines expanded in Rounds 8–11
- **D006 aftertouch coverage**: 23/23 engines have aftertouch (Optic intentionally exempt — visual engine)
- **D006 mod wheel coverage**: **22/22 MIDI-capable engines — FULLY RESOLVED** (Round 12C completed the last 7 engines)
- **D001 filter envelopes**: RESOLVED — all engines fleet-wide have velocity-scaled filter envelopes (Round 9E)
- **D004 dead params**: RESOLVED — all declared parameters wired to DSP (Round 3B)
- **D005 LFO breathing**: RESOLVED — all engines have autonomous modulation with rate floor ≤ 0.01 Hz (Round 5A + engine recoveries)

### Prism Sweep — COMPLETE (2026-03-14)

12-round progressive quality pass across all 26 original engines. **ALL 12 ROUNDS COMPLETE.**

- All 6 doctrines resolved fleet-wide (D001–D006)
- 22/22 engines with mod wheel | 23/23 engines with aftertouch (Optic intentionally exempt — visual engine)
- 19,578+ presets (as of 2026-03-17), 0 duplicates, 100% DNA coverage, health score ~92/100
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
| [sro-optimizer](Skills/sro-optimizer/SKILL.md) | `/sro-optimizer` | CPU audit, zero-idle bypass, LUT replacement, control-rate coupling optimization |
| synth-seance (`~/.claude/skills/`) | `/synth-seance` | Ghost council full engine quality evaluation |
| post-engine-completion-checklist (`~/.claude/skills/`) | `/post-engine-completion-checklist` | 5-point post-build audit |
| producers-guild (`~/.claude/skills/`) | `/producers-guild` | 12-specialist market/product review |

Full index: `Skills/README.md`

---

## Development Workflow

1. Read the master spec (`Docs/xomnibus_master_specification.md`) before making changes
2. Plan before coding — produce architecture + QA plan first
3. All engines must implement the `SynthEngine` interface
4. Parameter IDs are namespaced by engine (e.g., `snap_filterCutoff`, `dub_sendAmount`)
5. Run DSP stability checks after any engine modifications
6. Preserve existing parameter IDs and preset compatibility
7. For new engines, follow the process in `Docs/xomnibus_new_engine_process.md`
