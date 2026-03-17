# XOmnibus — Copilot Instructions

## Product Identity

XOmnibus ("for all") is a free, open-source multi-engine synthesizer platform by **XO_OX Designs**.
It merges character instruments into one unified creative environment where engines couple, collide,
and mutate into sounds impossible with any single synth. **34 engines** are currently registered.

- **Engine modules (registered):** ODDFELIX, ODDOSCAR, OVERDUB, ODYSSEY, OBLONG, OBESE, ONSET, OVERWORLD, OPAL, ORBITAL, ORGANON, OUROBOROS, OBSIDIAN, OVERBITE, ORIGAMI, ORACLE, OBSCURA, OCEANIC, OCELOT, OPTIC, OBLIQUE, OSPREY, OSTERIA, OWLFISH, OHM, ORPHICA, OBBLIGATO, OTTONI, OLE, OVERLAP, OUTWIT, OMBRE, ORCA, OCTOPUS
- **Coupling:** Cross-engine modulation via MegaCouplingMatrix (12 coupling types)
- **PlaySurface:** 4-zone unified playing interface (Pad/Fretless/Drum modes)
- **Presets:** 2,550 factory presets in `.xometa` format, 7 mood categories, 6D Sonic DNA
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
| `Source/DSP/` | Shared DSP library |
| `Source/UI/` | Gallery Model UI components |
| `Source/Export/` | XPN export pipeline |
| `Presets/XOmnibus/{mood}/` | Factory presets by mood |
| `Tools/` | Python utilities (DNA, breeding, migration, export) |
| `Docs/` | All specification documents |

## Parameter Naming — Frozen Prefixes

Engine parameter IDs use a `{prefix}_{paramName}` format. **Prefixes are frozen and must never change.**

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

Legacy engine names (`Snap`, `Morph`, `Dub`, `Drift`, `Bob`, `Fat`, `Bite`) are resolved automatically
by `resolveEngineAlias()` in `PresetManager.h`. See `Docs/xomnibus_name_migration_reference.md`.

## Engine Registration

New engines use **dual registration**:
1. `REGISTER_ENGINE` macro in the `.cpp` stub (keyed by class name)
2. Manual `registerEngine()` call in `XOmnibusProcessor.cpp` (keyed by canonical ID)

## Preset System

- `.xometa` JSON files are the source of truth (version-controlled)
- 7 moods: Foundation, Atmosphere, Entangled, Prism, Flux, Aether, Family
- 4 macros: CHARACTER, MOVEMENT, COUPLING, SPACE
- 6D Sonic DNA: brightness, warmth, movement, density, space, aggression
- Naming: 2-3 words, evocative, max 30 chars, no duplicates, no jargon

## Design System

- **Gallery Model:** Warm white shell `#F8F6F3` frames engine accent colors
- **XO Gold:** `#E9C46A` — brand constant (macros, coupling strip, active states)
- **Typography:** Space Grotesk (display), Inter (body), JetBrains Mono (values)
- **Light mode default**, dark mode toggle

## Build Commands

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

## The 6 Engine Quality Doctrines

Every engine must satisfy all six doctrines:

| ID | Doctrine | Requirement |
|----|----------|-------------|
| D001 | Velocity Must Shape Timbre | Velocity drives filter brightness / harmonic content — not just amplitude |
| D002 | Modulation is the Lifeblood | Min: 2 LFOs, mod wheel/aftertouch, 4 working macros, 4+ mod matrix slots |
| D003 | The Physics IS the Synthesis | Rigor and citation required for any physically-modeled engine |
| D004 | Dead Parameters Are Broken Promises | Every declared parameter must affect audio output |
| D005 | An Engine That Cannot Breathe Is a Photograph | Every engine needs at least one LFO with rate floor ≤ 0.01 Hz |
| D006 | Expression Input Is Not Optional | Velocity→timbre + at least one CC (aftertouch / mod wheel / expression) |

## Adding New Engines

**Quick rules for XOmnibus-ready development:**
- Parameter IDs use `{shortname}_{paramName}` format from day one
- Presets use `.xometa` JSON format from day one
- DSP lives in inline `.h` headers (portable)
- DSP works without UI references (just parameters)
- M1-M4 macros produce audible change in every preset
- Define coupling compatibility early (which `CouplingType` enums you accept)

**Integration path:** Write a thin adapter implementing `SynthEngine` → `REGISTER_ENGINE()` → register in `XOmnibusProcessor.cpp` → copy presets → done.

See `Docs/xomnibus_new_engine_process.md` for the full process.

## Development Workflow

1. Read the master spec (`Docs/xomnibus_master_specification.md`) before making changes
2. Plan before coding — produce architecture + QA plan first
3. All engines must implement the `SynthEngine` interface
4. Parameter IDs are namespaced by engine (e.g., `snap_filterCutoff`, `dub_sendAmount`)
5. Run DSP stability checks after any engine modifications
6. Preserve existing parameter IDs and preset compatibility
7. For new engines, follow the process in `Docs/xomnibus_new_engine_process.md`
