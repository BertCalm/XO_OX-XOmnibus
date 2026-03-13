# XOmnibus — Claude Code Project Guide

## Product Identity

XOmnibus ("for all") is a free, open-source multi-engine synthesizer platform by **XO_OX Designs**.
It merges 9 character instruments into one unified creative environment where engines couple, collide,
and mutate into sounds impossible with any single synth.

- **Engine modules:** ODDFELIX, ODDOSCAR, OVERDUB, ODYSSEY, OBLONG, OBESE, ONSET, OVERWORLD, OPAL, ORBITAL, ORGANON, OUROBOROS, OBSIDIAN, OVERBITE, ORIGAMI, ORACLE, OBSCURA, OCEANIC, OPTIC, OBLIQUE
- **Coupling:** Cross-engine modulation via MegaCouplingMatrix (12 coupling types)
- **PlaySurface:** 4-zone unified playing interface (Pad/Fretless/Drum modes)
- **Presets:** 1000 factory presets in `.xometa` format, 6 mood categories, 6D Sonic DNA
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
| BITE | XOpossum | Moss Green `#4A7C59` |
| ORGANON | XOrganon | Bioluminescent Cyan `#00CED1` |
| OUROBOROS | XOuroboros | Strange Attractor Red `#FF2D2D` |
| OBSIDIAN | XObsidian | Crystal White `#E8E0D8` |
| ORIGAMI | XOrigami | Vermillion Fold `#E63946` |
| ORACLE | XOracle | Prophecy Indigo `#4B0082` |
| OBSCURA | XObscura | Daguerreotype Silver `#8A9BA8` |
| OCEANIC | XOceanic | Phosphorescent Teal `#00B4A0` |
| OVERBITE | XOverbite | Fang White `#F0EDE8` |
| ORBITAL | XOrbital | Warm Red `#FF6B6B` |
| OPTIC | XOptic | Phosphor Green `#00FF41` |
| OBLIQUE | XOblique | Prism Violet `#BF40FF` |
| OSTINATO | XOstinato | Firelight Orange `#E8701A` |
| OPENSKY | XOpenSky | Sunburst `#FF8C00` |
| OCEANDEEP | XOceanDeep | Trench Violet `#2D0A4E` |

### Engine ID vs Parameter Prefix

Engine IDs (used in preset `"engines"` arrays, `"parameters"` keys, UI, and coupling routes)
were renamed to O-prefix convention. **Parameter prefixes are frozen and never change:**

| Engine ID | Parameter Prefix | Example |
|-----------|-----------------|---------|
| OddfeliX | `snap_` | `snap_filterCutoff` |
| OddOscar | `morph_` | `morph_scanPos` |
| Overdub | `dub_` | `dub_sendAmount` |
| Odyssey | `odyssey_` | `odyssey_detune` |
| Oblong | `bob_` | `bob_fltCutoff` |
| Obese | `fat_` | `fat_satDrive` |
| Overbite | `poss_` | `poss_biteDepth` |
| Onset | `onset_` | `onset_noiseLevel` |
| Overworld | `era_` | `era_engineA` |
| Opal | `opal_` | `opal_grainSize` |
| Orbital | `orbital_` | `orbital_partialTilt` |
| Organon | `organon_` | `organon_entropy` |
| Ouroboros | `ouroboros_` | `ouroboros_feedback` |
| Obsidian | `obsidian_` | `obsidian_pdDepth` |
| Origami | `origami_` | `origami_foldPoint` |
| Oracle | `oracle_` | `oracle_breakpoints` |
| Obscura | `obscura_` | `obscura_stiffness` |
| Oceanic | `oceanic_` | `oceanic_separation` |
| Optic | `optic_` | `optic_pulseRate` |
| Oblique | `oblq_` | `oblq_prismColor` |
| Ostinato | `osti_` | `osti_seatN_instrument` |
| OpenSky | `sky_` | `sky_shimmerAmount` |
| OceanDeep | `deep_` | `deep_pressure` |

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
| `Source/UI/OpticVisualizer/OpticVisualizer.h` | Winamp-style audio-reactive visualizer |
| `Docs/xomnibus_sound_design_guides.md` | Sound design guide for all 20 engines |
| `Source/DSP/` | Shared DSP library |
| `Source/UI/` | Gallery Model UI components |
| `Source/Export/` | XPN export pipeline |
| `Presets/XOmnibus/{mood}/` | Factory presets by mood |
| `Tools/` | Python utilities (DNA, breeding, migration, export) |
| `Docs/` | All specification documents |

## Preset System

- `.xometa` JSON files are the source of truth (version-controlled)
- 6 moods: Foundation, Atmosphere, Entangled, Prism, Flux, Aether
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

## Development Workflow

1. Read the master spec (`Docs/xomnibus_master_specification.md`) before making changes
2. Plan before coding — produce architecture + QA plan first
3. All engines must implement the `SynthEngine` interface
4. Parameter IDs are namespaced by engine (e.g., `snap_filterCutoff`, `dub_sendAmount`)
5. Run DSP stability checks after any engine modifications
6. Preserve existing parameter IDs and preset compatibility
7. For new engines, follow the process in `Docs/xomnibus_new_engine_process.md`
