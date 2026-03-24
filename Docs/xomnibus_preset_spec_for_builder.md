# XOlokun Preset System — Builder Specification

**For:** Architecture/Builder Agent
**From:** Preset Design Agent
**Date:** 2026-03-08
**Status:** All decisions locked, migration tooling complete

This document contains every decision the builder needs to implement the XOlokun preset system. No planning narrative — just specs.

---

## 1. Product Identity

- **Name:** XOlokun ("for all" — the collected works of XO_OX Designs)
- **Type:** Multi-engine synth platform merging 6 character instruments
- **Target:** 1000 factory presets at v1.0

## 2. Engines

| Engine | Identity | Params | Presets (migrated) |
|--------|----------|--------|--------------------|
| OddfeliX/OddOscar | Dual-engine coupled synth (Percussive X + Pad O) | 52 | 114 |
| XOverdub | Dub mixing desk (send/return FX chain) | 38 | 40 |
| XOdyssey | Psychedelic pad synth (Familiar→Alien journey) | ~130 | 198 |
| XOblong | Warm character synth (fuzzy textures, curious motion) | ~50 | 167 |
| XObese | Character-driven sampler engine | ~30 | 52 (pending) |
| XOnset | Percussive synthesis (Circuit↔Algorithm blend) | ~110 | 85 (planned) |
| XOmbre | Dual-narrative (memory/forgetting + perception) | 15 | 15 |

**Parameter namespacing:** Each engine prefixes its APVTS parameter IDs to avoid collisions. The `.xometa` format stores parameters under engine-keyed objects.

## 3. Preset Format: `.xometa`

Single source of truth. JSON files replacing all per-engine C++ presets and `.xocmeta` files.

**Schema:** `synth_playbook/docs/xometa_schema.json`

**Key fields:**

```json
{
  "schema_version": 1,
  "name": "Dub Pressure Machine",       // max 30 chars, unique across library
  "mood": "Entangled",                   // enum: Foundation|Atmosphere|Entangled|Prism|Flux|Aether|User
  "engines": ["OddfeliX/OddOscar", "XOnset"],   // 1-3 engines
  "author": "XO_OX",
  "version": "1.0.0",
  "description": "Kick pumps the pad engine, snare brightens the filter.",
  "tags": ["dub", "pump", "coupled"],    // min 3
  "macroLabels": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
  "couplingIntensity": "Deep",           // None|Subtle|Moderate|Deep
  "tempo": 130,                          // null if not tempo-dependent
  "dna": {                               // 6D sonic fingerprint (auto-computed)
    "brightness": 0.54, "warmth": 0.41,
    "movement": 0.09, "density": 0.16,
    "space": 0.35, "aggression": 0.08
  },
  "legacy": {                            // migration tracking
    "sourceInstrument": "OddfeliX/OddOscar",
    "sourceCategory": "Entangled",
    "sourcePresetName": null
  },
  "parameters": {                        // engine-keyed param overrides
    "OddfeliX/OddOscar": { "xFilterCutoff": 3000, "couplingAmount": 0.8 },
    "XOnset": { "kickDecay": 0.3, "blend": 0.5 }
  },
  "coupling": {                          // cross-engine mod routing
    "pairs": [{
      "engineA": "OddfeliX/OddOscar", "engineB": "XOnset",
      "type": "Amp->Filter", "amount": 0.4
    }]
  },
  "recipe": null,                        // optional soft reference to recipe name (max 25 chars)
  "parentPreset": null,                  // optional lineage — factory preset this derives from
  "sequencer": null                      // optional step sequencer data
}
```

**Coupling types (enum):** `Amp->Filter`, `Amp->Pitch`, `LFO->Pitch`, `Env->Morph`, `Audio->FM`, `Audio->Ring`, `Filter->Filter`, `Amp->Choke`, `Rhythm->Blend`, `Env->Decay`

## 4. Mood Categories (Browsing Taxonomy)

6 universal moods replace 42 per-engine categories.

| Mood | Intent | UI Color |
|------|--------|----------|
| **Foundation** | Bass, drums, rhythmic anchors | Terracotta |
| **Atmosphere** | Pads, drones, washes, textures | Teal |
| **Entangled** | Cross-coupled, reactive — the XOlokun signature | Gold |
| **Prism** | Leads, keys, bells, melodic, articulate | Silver |
| **Flux** | Glitchy, unstable, experimental, lo-fi | Crimson |
| **Aether** | Cinematic, transcendent, ambient, spiritual | Indigo |

**Current library distribution (519 presets):**

| Mood | Count |
|------|-------|
| Foundation | 89 |
| Atmosphere | 129 |
| Entangled | 84 |
| Prism | 144 |
| Flux | 36 |
| Aether | 37 |

## 5. Macro System

4 macro knobs, unified behavior across all engines:

| Macro | Label | Intent |
|-------|-------|--------|
| M1 | CHARACTER | Each engine's defining parameter |
| M2 | MOVEMENT | Modulation depth across all active engines |
| M3 | COUPLING | Cross-engine interaction strength (XOlokun signature) |
| M4 | SPACE | Shared FX depth (reverb, delay, chorus) |

**Per-engine macro targets:**

| Macro | OddfeliX/OddOscar | XOverdub | XOdyssey | XOblong | XOnset | XOmbre |
|-------|-----------|----------|----------|------------|--------|--------|
| M1 | Snap+Morph | Send Level | JOURNEY | Curiosity | MACHINE | BLEND+INTERFERENCE |
| M2 | Bloom | Drive | BREATHE | Bob Mode | PUNCH | DRIFT+REACTIVITY |
| M3 | Coupling | Delay FB | BLOOM | Smear Mix | SPACE | INTERFERENCE+GRAIN |
| M4 | Delay+Rev | Reverb Mix | FRACTURE | Space Mix | MUTATE | DECAY+FILTER |

**Golden rule:** Every macro must produce an audible change across its full range in every preset.

## 6. Sonic DNA System

Every preset carries a 6D fingerprint for similarity search, morphing, and breeding:

| Dimension | Measures |
|-----------|----------|
| brightness | Filter openness, harmonics, shimmer |
| warmth | Saturation, drift, sub, low harmonics |
| movement | LFO depth, modulation, tidal, curiosity |
| density | Osc count, unison, detuning, layers |
| space | Reverb, delay, chorus wet signal |
| aggression | Drive, distortion, resonance, fracture |

**Features to implement in UI:**
- **Find Similar** — 5 nearest neighbors by Euclidean distance in DNA space
- **Find Opposite** — invert DNA vector, find closest match
- **Mood Map** — 2D PCA/t-SNE visualization of all presets
- **Preset Morphing** — parameter interpolation between two presets via single fader
- **Preset Breeding** — genetic crossover + DNA-aware mutation producing offspring

**DNA computation:** Auto-runs on preset save. Tool: `tools/compute_preset_dna.py`

## 7. Preset Loading Pipeline

```
.xometa file on disk
    ↓
PresetManager::loadPreset(path)
    ↓
Parse JSON → validate schema_version
    ↓
For each engine in "engines":
    ↓
    SynthEngine::loadParameters(parameters[engine])
    ↓
    Apply coupling pairs to CouplingMatrix
    ↓
    Set macro labels in UI
    ↓
    If sequencer != null: load sequencer data
    ↓
    Compute DNA if missing → inject into file
```

**Key constraint:** The `SynthEngine` interface (from feasibility study) must implement `loadParameters(dict)` accepting the engine-keyed parameter map from `.xometa`.

## 8. Cross-Engine Preset Rules

- **Single-engine:** 1 engine active, coupling disabled, M3 (COUPLING) controls engine-internal modulation
- **Dual-engine:** 2 engines active, coupling pairs define interaction, CPU budget <28%
- **Tri-engine:** 3 engines, hero presets only, CPU budget <55%
- **Coupling pairs** are directional: `engineA` modulates `engineB`
- **Decoupling** (coupling amount → 0) must produce a musically valid sound
- **No feedback runaway** — all coupling paths must be gain-limited

## 9. Preset Browser UX

```
[Mood tabs: Foundation | Atmosphere | Entangled | Prism | Flux | Aether]
    ↓
[Engine filter: All | OddfeliX/OddOscar | XOverdub | XOdyssey | XOblong | XOnset | Multi-Engine]
    ↓
[Search bar + tag filter]
    ↓
[Preset grid: name, mood badge, engine icon(s), DNA sparkline, favorite star]
```

**Smart Collections:**

| Collection | Filter Logic |
|-----------|-------------|
| Start Here | Curated 10 best-of |
| Hero Presets | Top 20 flagships |
| Deep Coupling | couplingIntensity == "Deep" |
| Sequencer Patterns | sequencer != null |
| New This Version | created >= currentVersion |
| User Presets | author != "XO_OX" |
| Favorites | user-starred |

**First launch:** Auto-load a hero cross-engine preset. Mood tabs visible. "Start Here" collection prominent.

## 10. 1000-Preset Library Architecture

| Layer | Count | Description |
|-------|-------|-------------|
| Migrated Singles | 588 | Existing presets, one engine each |
| Impossible Sounds | 40 | Coupling-dependent flagships |
| Sessions | 90 | 15 production palettes × 6 presets each |
| Evolution Presets | 50 | M1 sweep tells a sonic story |
| Tempo Collections | 75 | 5 genre packs × 15 sync-ready presets |
| Deconstructed Heroes | 80 | 20 heroes × 4 variants (full/A solo/B solo/dry) |
| Living Presets | 32 | Generative, never-repeating ambient instruments |
| Reimagined Classics | 25 | MVP hits rebuilt with cross-engine coupling |
| XO Duality | 20 | Brand philosophy: opposing forces creating unity |
| **TOTAL** | **1000** | |

## 11. Naming Conventions

- 2-3 words, evocative/tactile, max 30 characters
- No technical jargon ("Amber Tide" not "LP Filter Pad")
- No numbers ("Sub Pressure" not "Bass 3")
- No duplicates across entire library (10 known collisions resolved)
- Cross-engine names hint at both worlds ("Velvet Cosmos" = Bob warmth + Odyssey space)
- Lexicon: Nature, Sensation, Science, Culture, Objects

## 12. Quality Standards

Every preset must pass:
- Sounds compelling dry (before shared FX)
- Velocity response is audible
- All 4 macros produce audible change
- CPU within engine budget
- No clicks/pops during macro sweeps
- Name follows conventions
- Minimum 3 searchable tags

Cross-engine presets additionally:
- Coupling is audible
- Decoupled version still sounds good
- No feedback runaway
- CPU <28% (dual) or <55% (tri)

## 13. MPC Export Strategy

For MPC integration, XOlokun renders presets as multi-sampled keygroup programs:
- 21 notes per preset (every minor 3rd, C1-C6)
- `KeyTrack=True`, `RootNote=0` (MPC convention)
- Empty layers: `VelStart=0` to prevent ghost triggering
- Package as `.xpn` expansion by mood category

## 14. Migration Status

| Source | Presets | Status | Tool |
|--------|---------|--------|------|
| OddfeliX/OddOscar | 114 | Done | `migrate_xocmeta_to_xometa.py` |
| XOverdub | 40 | Done | `extract_cpp_presets.py` |
| XOdyssey | 198 | Done | `extract_cpp_presets.py` |
| XOblong | 167 | Done | `extract_cpp_presets.py` |
| XObese | 52 | Pending | Manual export + migration |
| XOnset | 85 | Spec only | N/A |
| **Total in .xometa** | **519** | **All fingerprinted with DNA** | |

## 15. File Locations

| Asset | Path |
|-------|------|
| Philosophy doc | `synth_playbook/docs/mega_tool_preset_philosophy.md` |
| This spec | `synth_playbook/docs/xolokun_preset_spec_for_builder.md` |
| .xometa schema | `synth_playbook/docs/xometa_schema.json` |
| .xometa examples | `synth_playbook/docs/xometa_examples.json` |
| Migrated presets | `Presets/XOlokun/{mood}/*.xometa` |
| DNA generator | `tools/compute_preset_dna.py` |
| Preset breeder | `tools/breed_presets.py` |
| C++ extractor | `tools/extract_cpp_presets.py` |
| OddfeliX/OddOscar migrator | `tools/migrate_xocmeta_to_xometa.py` |
| XPM fixer | `tools/fix_xobese_xpms.py` |
| TODO tracker | `synth_playbook/docs/mega_tool_preset_todo.md` |
