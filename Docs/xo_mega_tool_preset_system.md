# XO_OX Mega-Tool Preset System Specification

> ⚠️ **SUPERSEDED — DO NOT USE**
> This document was replaced by `Docs/xomnibus_preset_spec_for_builder.md`.
> Kept for historical reference only. Content may be outdated or incorrect.

**Version:** 1.0
**Author:** XO_OX Designs
**Date:** 2026-03-08
**Status:** SPECIFICATION COMPLETE -- Ready for implementation
**Depends on:** `xo_mega_tool_dev_strategy.md` (Interface-First Hybrid), `xo_mega_tool_feasibility.md` (Hybrid A+D architecture), `mega_tool_preset_philosophy.md` (category system, naming, quality standards)

---

## 1. Design Goals

### 1.1 Backward Compatibility

Every existing preset from every shipped XO_OX instrument loads in the mega-tool without modification to the original file. The migration layer wraps legacy formats into the unified `.xomega` envelope at load time. Original files are never mutated.

### 1.2 Forward Extensibility

The `.xomega` format supports an arbitrary number of engine entries in its `engines` array. Adding a new XO_OX instrument to the mega-tool requires only: (1) implement the `SynthEngine` interface, (2) register a module ID, (3) define a parameter namespace prefix. No schema changes needed.

### 1.3 Universal Browser

One preset browser interface discovers every sound in the library regardless of which engine(s) produced it. A user searching for "dub bass" finds results from XOverdub, OddfeliX/OddOscar, XObese, and any cross-engine presets tagged accordingly. The browser never requires the user to know which engine a preset belongs to.

### 1.4 Export Flexibility

Presets can be bundled into MPC-compatible XPN expansions along any axis: by engine, by mood/vibe, by instrument type, by custom user selection, or as a complete library. Multi-engine presets are rendered to WAV before packaging since coupling DSP cannot be exported natively.

### 1.5 Mutation

The system supports generating new presets through parameter crossover between two parent presets, producing offspring that inherit characteristics from both parents with controlled randomization. This is computationally trivial (parameter manipulation only) but sonically generative.

### 1.6 Self-Contained Files

Every `.xomega` file contains all information needed to reconstruct the sound. No external dependencies, no references to other files, no sample paths. A preset shared between users works identically on any machine with the mega-tool installed.

---

## 2. Preset Schema: .xomega Format

### 2.1 File Format

- **Extension:** `.xomega`
- **Encoding:** UTF-8 JSON
- **MIME type:** `application/json`
- **Maximum file size:** 256 KB (enforced at save time)
- **Line endings:** LF (Unix-style)

### 2.2 Complete Schema Definition

```json
{
  "schema_version": "1.0",
  "format": "xomega",
  "name": "Dub Pressure",
  "author": "XO_OX Designs",
  "mood": "Entangled",
  "category": "Hybrid Kits",
  "tags": ["dub", "drums", "coupled", "dark"],
  "instrument_type": "drums",
  "vibe": "dark",
  "description": "XOnset drums coupled with XOverdub send/return. Kick pumps the dub delay.",
  "created": "2026-03-08T00:00:00Z",
  "modified": "2026-03-08T00:00:00Z",
  "tempo": 130,
  "macro_labels": ["MACHINE", "PUNCH", "SPACE", "MUTATE"],

  "engines": [
    {
      "module": "onset",
      "version": "1.0",
      "slot": 0,
      "active": true,
      "parameters": {
        "perc_v1_blend": 0.2,
        "perc_v1_pitch": 0,
        "perc_v1_decay": 0.6,
        "perc_v1_tone": 0.5,
        "perc_v1_snap": 0.4,
        "perc_v1_body": 0.7,
        "perc_v1_character": 0.1,
        "perc_v1_level": -6.0,
        "perc_v1_pan": 0.0,
        "perc_v1_env_shape": 0,
        "perc_master_level": -3.0,
        "perc_coupling_amount": 0.3
      }
    },
    {
      "module": "dub",
      "version": "1.0",
      "slot": 1,
      "active": true,
      "parameters": {
        "dub_sendLevel": 0.7,
        "dub_delayFeedback": 0.85,
        "dub_delayTime": 0.375,
        "dub_delayWear": 0.4,
        "dub_delayWow": 0.2,
        "dub_delayMix": 0.6,
        "dub_reverbSize": 0.5,
        "dub_reverbMix": 0.3,
        "dub_driveAmount": 1.5,
        "dub_masterVolume": 0.75
      }
    }
  ],

  "coupling": {
    "pairs": [
      {
        "source_slot": 0,
        "target_slot": 1,
        "type": "amp_to_filter",
        "amount": 0.4,
        "direction": "unidirectional"
      },
      {
        "source_slot": 0,
        "target_slot": 1,
        "type": "trigger_to_reset",
        "amount": 1.0,
        "direction": "unidirectional"
      }
    ]
  },

  "surface": {
    "mode": "drum",
    "scale": "chromatic",
    "root": "C",
    "octave": 3,
    "orbit_recipe": "SPACE DUB",
    "strip_mode": "dub_space"
  },

  "fx_rack": {
    "shared": true,
    "slots": [
      {
        "type": "delay",
        "enabled": true,
        "params": {
          "time": 375,
          "feedback": 0.6,
          "mix": 0.4,
          "wobble": 0.1
        }
      },
      {
        "type": "reverb",
        "enabled": true,
        "params": {
          "size": 0.7,
          "mix": 0.3,
          "damping": 0.4
        }
      }
    ]
  },

  "legacy": {
    "source_instrument": null,
    "source_preset_name": null,
    "source_format": null,
    "migration_version": null
  }
}
```

### 2.3 Field Reference

#### Top-Level Metadata

| Field | Type | Required | Default | Description | Constraints |
|-------|------|----------|---------|-------------|-------------|
| `schema_version` | string | YES | -- | Format version for migration | Semantic version string. Current: `"1.0"` |
| `format` | string | YES | -- | Format identifier | Must be `"xomega"` |
| `name` | string | YES | -- | Display name of the preset | 1-40 characters, unique within library scope |
| `author` | string | YES | `"XO_OX Designs"` | Creator attribution | 1-60 characters |
| `mood` | string | YES | -- | Primary mood category (Tier 1) | One of: `"Grounded"`, `"Floating"`, `"Entangled"`, `"Sharp"`, `"Broken"`, `"Ritual"` |
| `category` | string | NO | `null` | Secondary category for engine-specific grouping | Free-form string, max 30 characters |
| `tags` | string[] | YES | `[]` | Searchable tags (see Section 4) | Array of lowercase strings, 1-20 items, each 1-20 chars |
| `instrument_type` | string | NO | `"synth"` | Primary instrument role | One of: `"synth"`, `"drums"`, `"bass"`, `"pad"`, `"lead"`, `"keys"`, `"arp"`, `"sequence"`, `"drone"`, `"fx"`, `"vocal"`, `"percussion"` |
| `vibe` | string | NO | `null` | Sonic character descriptor | One of: `"dark"`, `"bright"`, `"warm"`, `"cold"`, `"aggressive"`, `"gentle"`, `"psychedelic"`, `"vintage"`, `"modern"`, `"experimental"`, `"cinematic"`, `"lo-fi"`, `"pristine"`, `"organic"`, `"synthetic"` |
| `description` | string | NO | `""` | Human-readable description | Max 200 characters |
| `created` | string | YES | -- | ISO 8601 creation timestamp | Format: `YYYY-MM-DDTHH:MM:SSZ` |
| `modified` | string | YES | -- | ISO 8601 last-modified timestamp | Format: `YYYY-MM-DDTHH:MM:SSZ` |
| `tempo` | number | NO | `null` | Suggested BPM (for sequenced presets) | 30-300, or `null` if tempo-agnostic |
| `macro_labels` | string[4] | NO | `null` | Custom labels for the 4 performance macros | Array of exactly 4 strings, each 1-12 chars |

#### Engine Entries (`engines` array)

| Field | Type | Required | Default | Description | Constraints |
|-------|------|----------|---------|-------------|-------------|
| `module` | string | YES | -- | Engine module identifier | One of registered module IDs (see Section 2.4) |
| `version` | string | YES | -- | Engine parameter schema version | Semantic version string |
| `slot` | integer | YES | -- | Engine slot position (rendering order) | 0-3 (max 4 active engines) |
| `active` | boolean | NO | `true` | Whether this engine produces audio | -- |
| `parameters` | object | YES | -- | Key-value map of namespaced parameter IDs to values | All keys must use the engine's registered prefix. Values are numbers (float or int) |

**Rules:**
- Single-engine presets have exactly one entry in `engines`.
- Multi-engine presets have 2-4 entries.
- Slot values must be unique within the preset.
- The `parameters` object only contains parameters that differ from engine defaults. Omitted parameters use the engine's compiled defaults. This keeps file sizes small.

#### Coupling Configuration (`coupling` object)

| Field | Type | Required | Default | Description | Constraints |
|-------|------|----------|---------|-------------|-------------|
| `coupling` | object | NO | `null` | Omit entirely for single-engine presets | -- |
| `coupling.pairs` | array | YES (if coupling present) | `[]` | Array of coupling pair definitions | 0-6 pairs (max for 4 engines) |
| `pairs[n].source_slot` | integer | YES | -- | Slot index of the modulation source engine | Must match a valid `engines[].slot` |
| `pairs[n].target_slot` | integer | YES | -- | Slot index of the modulation target engine | Must match a valid `engines[].slot`, must differ from source |
| `pairs[n].type` | string | YES | -- | Coupling modulation type | One of: `"amp_to_filter"`, `"lfo_to_pitch"`, `"env_to_morph"`, `"audio_to_fm"`, `"audio_to_ring"`, `"filter_to_filter"`, `"trigger_to_reset"`, `"amp_to_choke"`, `"rhythm_to_blend"`, `"env_to_decay"` |
| `pairs[n].amount` | number | YES | -- | Coupling intensity | 0.0-1.0 |
| `pairs[n].direction` | string | NO | `"unidirectional"` | Signal flow direction | One of: `"unidirectional"`, `"bidirectional"` |

#### PlaySurface Configuration (`surface` object)

| Field | Type | Required | Default | Description | Constraints |
|-------|------|----------|---------|-------------|-------------|
| `surface` | object | NO | `null` | Omit to use engine-default surface settings | -- |
| `surface.mode` | string | NO | `"pad"` | PlaySurface operating mode | One of: `"pad"`, `"fretless"`, `"drum"` |
| `surface.scale` | string | NO | `"chromatic"` | Scale constraint | One of: `"chromatic"`, `"major"`, `"minor"`, `"pentatonic"`, `"blues"`, `"dorian"`, `"mixolydian"`, `"harmonic_minor"`, `"whole_tone"`, `"diminished"` |
| `surface.root` | string | NO | `"C"` | Root note | One of: `"C"`, `"C#"`, `"D"`, `"D#"`, `"E"`, `"F"`, `"F#"`, `"G"`, `"G#"`, `"A"`, `"A#"`, `"B"` |
| `surface.octave` | integer | NO | `3` | Base octave | 0-7 |
| `surface.orbit_recipe` | string | NO | `null` | Named Orbit Path recipe | Free-form string per PlaySurface spec |
| `surface.strip_mode` | string | NO | `"dub_space"` | Performance Strip mode | One of: `"dub_space"`, `"filter_sweep"`, `"coupling"`, `"dub_siren"` |

#### Shared FX Rack (`fx_rack` object)

| Field | Type | Required | Default | Description | Constraints |
|-------|------|----------|---------|-------------|-------------|
| `fx_rack` | object | NO | `null` | Omit if engines own their own FX | -- |
| `fx_rack.shared` | boolean | NO | `true` | Whether FX are shared or per-engine | -- |
| `fx_rack.slots` | array | NO | `[]` | Ordered FX slot definitions | Max 6 slots |
| `slots[n].type` | string | YES | -- | Effect type | One of: `"delay"`, `"reverb"`, `"phaser"`, `"lofi"`, `"compressor"`, `"chorus"`, `"distortion"` |
| `slots[n].enabled` | boolean | NO | `true` | Whether this FX slot is active | -- |
| `slots[n].params` | object | YES | -- | Effect-specific parameter map | Keys and value ranges depend on `type` |

#### Legacy Migration Metadata (`legacy` object)

| Field | Type | Required | Default | Description | Constraints |
|-------|------|----------|---------|-------------|-------------|
| `legacy` | object | NO | `null` | Only present on migrated presets | -- |
| `legacy.source_instrument` | string | NO | `null` | Original instrument name | e.g., `"OddfeliX/OddOscar"`, `"XOverdub"` |
| `legacy.source_preset_name` | string | NO | `null` | Original preset name (if renamed) | -- |
| `legacy.source_format` | string | NO | `null` | Original file format | One of: `"xocmeta"`, `"json"`, `"juce_binary"` |
| `legacy.migration_version` | string | NO | `null` | Migration script version that created this file | Semantic version |

### 2.4 Registered Engine Module IDs

| Module ID | Engine | Namespace Prefix | Source Project | Param Count |
|-----------|--------|-----------------|----------------|-------------|
| `oddx` | OddfeliX | `oddx_` | OddfeliX/OddOscar | ~24 |
| `oddo` | OddOscar | `oddo_` | OddfeliX/OddOscar | ~20 |
| `dub` | XOverdub | `dub_` | XOverdub | 38 |
| `fat` | XObese | `fat_` | XObese | 45 |
| `poss` | XOverbite | `poss_` | XOverbite | 122 |
| `drift` | XOdyssey | `drift_` | XOdyssey | ~130 |
| `bob` | XOblong | `bob_` | XOblong | ~50 |
| `perc` | XOnset | `perc_` | XOnset | ~110 |

**Rule:** The module ID is the canonical key. The namespace prefix is `moduleID_` prepended to each internal parameter ID at the adapter boundary. Internal engine code never changes its parameter names.

### 2.5 Minimal Valid Presets

**Single-engine preset (minimum required fields):**

```json
{
  "schema_version": "1.0",
  "format": "xomega",
  "name": "Init Dub",
  "author": "XO_OX Designs",
  "mood": "Grounded",
  "tags": ["init", "dub"],
  "created": "2026-03-08T00:00:00Z",
  "modified": "2026-03-08T00:00:00Z",
  "engines": [
    {
      "module": "dub",
      "version": "1.0",
      "slot": 0,
      "parameters": {}
    }
  ]
}
```

An empty `parameters` object means "use all engine defaults." This is the "Init" preset pattern.

**Multi-engine preset with coupling:**

```json
{
  "schema_version": "1.0",
  "format": "xomega",
  "name": "Neural Rhythm",
  "author": "XO_OX Designs",
  "mood": "Entangled",
  "tags": ["coupled", "drums", "pad", "dub"],
  "instrument_type": "drums",
  "vibe": "dark",
  "created": "2026-03-08T00:00:00Z",
  "modified": "2026-03-08T00:00:00Z",
  "engines": [
    {
      "module": "onset",
      "version": "1.0",
      "slot": 0,
      "parameters": {
        "perc_v1_blend": 0.3,
        "perc_v1_snap": 0.5
      }
    },
    {
      "module": "oddo",
      "version": "1.0",
      "slot": 1,
      "parameters": {
        "oddo_filterCutoff": 2000.0,
        "oddo_bloom": 0.7
      }
    }
  ],
  "coupling": {
    "pairs": [
      {
        "source_slot": 0,
        "target_slot": 1,
        "type": "amp_to_filter",
        "amount": 0.35,
        "direction": "unidirectional"
      }
    ]
  }
}
```

---

## 3. Legacy Preset Migration

### 3.1 Migration Philosophy

Migration is **read-only wrapping**, never format conversion. The original preset file is never modified. At import time, the migration layer reads the legacy file, maps its parameters into the `.xomega` envelope, and writes a new `.xomega` file. The original is preserved for standalone builds.

### 3.2 Migration Pipeline Per Engine

#### OddfeliX/OddOscar (.xocmeta) to .xomega

**Source format:** JSON with `parameters` object containing 52 un-prefixed parameter IDs.

**Strategy:** OddfeliX/OddOscar is a dual-engine instrument. Each `.xocmeta` preset maps to a two-engine `.xomega` with `oddx` and `oddo` modules. Shared parameters (macros, effects, coupling, sequencer) are distributed to the appropriate section.

**Parameter Mapping Table:**

| Original ID | Target Module | Namespaced ID | Notes |
|-------------|---------------|---------------|-------|
| `xOscMode` | `oddx` | `oddx_oscMode` | |
| `xSnap` | `oddx` | `oddx_snap` | |
| `xDecay` | `oddx` | `oddx_decay` | |
| `xFilterCutoff` | `oddx` | `oddx_filterCutoff` | |
| `xFilterReso` | `oddx` | `oddx_filterReso` | |
| `xDetune` | `oddx` | `oddx_detune` | |
| `xLevel` | `oddx` | `oddx_level` | |
| `xPitchLock` | `oddx` | `oddx_pitchLock` | |
| `xUnison` | `oddx` | `oddx_unison` | |
| `xPolyphony` | `oddx` | `oddx_polyphony` | |
| `oMorph` | `oddo` | `oddo_morph` | |
| `oBloom` | `oddo` | `oddo_bloom` | |
| `oDecay` | `oddo` | `oddo_decay` | |
| `oSustain` | `oddo` | `oddo_sustain` | |
| `oRelease` | `oddo` | `oddo_release` | |
| `oFilterCutoff` | `oddo` | `oddo_filterCutoff` | |
| `oFilterReso` | `oddo` | `oddo_filterReso` | |
| `oDrift` | `oddo` | `oddo_drift` | |
| `oSub` | `oddo` | `oddo_sub` | |
| `oDetune` | `oddo` | `oddo_detune` | |
| `oLevel` | `oddo` | `oddo_level` | |
| `oPolyphony` | `oddo` | `oddo_polyphony` | |
| `couplingAmount` | coupling | Stored in `coupling.pairs[0].amount` | Maps to the X-to-O coupling pair |
| `masterBalance` | top-level | Stored as engine level ratio | Negative = X-heavy, positive = O-heavy |
| `macro1` | top-level | Mapped to macro_labels context | OddfeliX + OddOscar |
| `macro2` | top-level | Mapped to macro_labels context | Bloom |
| `macro3` | top-level | Mapped to macro_labels context | Coupling |
| `macro4` | top-level | Mapped to macro_labels context | Delay + Reverb |
| `delayEnabled` | `fx_rack` | `fx_rack.slots[0].enabled` | |
| `delayTime` | `fx_rack` | `fx_rack.slots[0].params.time` | |
| `delayFeedback` | `fx_rack` | `fx_rack.slots[0].params.feedback` | |
| `delayMix` | `fx_rack` | `fx_rack.slots[0].params.mix` | |
| `delayWobble` | `fx_rack` | `fx_rack.slots[0].params.wobble` | |
| `reverbEnabled` | `fx_rack` | `fx_rack.slots[1].enabled` | |
| `reverbDecay` | `fx_rack` | `fx_rack.slots[1].params.decay` | |
| `reverbSize` | `fx_rack` | `fx_rack.slots[1].params.size` | |
| `reverbMix` | `fx_rack` | `fx_rack.slots[1].params.mix` | |
| `reverbDamping` | `fx_rack` | `fx_rack.slots[1].params.damping` | |
| `phaserEnabled` | `fx_rack` | `fx_rack.slots[2].enabled` | |
| `phaserRate` | `fx_rack` | `fx_rack.slots[2].params.rate` | |
| `phaserDepth` | `fx_rack` | `fx_rack.slots[2].params.depth` | |
| `phaserMix` | `fx_rack` | `fx_rack.slots[2].params.mix` | |
| `lofiEnabled` | `fx_rack` | `fx_rack.slots[3].enabled` | |
| `lofiBitDepth` | `fx_rack` | `fx_rack.slots[3].params.bitDepth` | |
| `lofiRateReduce` | `fx_rack` | `fx_rack.slots[3].params.rateReduce` | |
| `lofiWowFlutter` | `fx_rack` | `fx_rack.slots[3].params.wowFlutter` | |
| `lofiMix` | `fx_rack` | `fx_rack.slots[3].params.mix` | |
| `compressorEnabled` | `fx_rack` | `fx_rack.slots[4].enabled` | |
| `compThreshold` | `fx_rack` | `fx_rack.slots[4].params.threshold` | |
| `compRatio` | `fx_rack` | `fx_rack.slots[4].params.ratio` | |
| `compAttack` | `fx_rack` | `fx_rack.slots[4].params.attack` | |
| `compRelease` | `fx_rack` | `fx_rack.slots[4].params.release` | |
| `seqPlaying` | top-level | `tempo` context | Sequencer state |
| `seqTempo` | top-level | `tempo` | |
| `seqSwing` | top-level | Stored in surface or engine params | |

**Mood mapping:** Uses existing `.xocmeta` `category` field:
- `"Grounded"` -> `"Grounded"`
- `"Floating"` -> `"Floating"`
- `"Entangled"` -> `"Entangled"`
- `"Deep Space"` -> `"Ritual"`

**Coupling reconstruction:** The `couplingAmount` value becomes:

```json
"coupling": {
  "pairs": [
    {
      "source_slot": 0,
      "target_slot": 1,
      "type": "amp_to_filter",
      "amount": <couplingAmount value>,
      "direction": "bidirectional"
    }
  ]
}
```

If `couplingAmount` is 0, the `coupling` field is omitted.

#### XOverdub (JSON/C++ presets) to .xomega

**Source format:** C++ factory preset arrays or JSON state with `schema_version`. 38 parameters.

**Parameter Mapping Table:**

| Original ID | Namespaced ID | Notes |
|-------------|---------------|-------|
| `oscWave` | `dub_oscWave` | |
| `oscOctave` | `dub_oscOctave` | |
| `oscTune` | `dub_oscTune` | |
| `oscPwm` | `dub_oscPwm` | |
| `oscSubLevel` | `dub_oscSubLevel` | |
| `oscNoiseLevel` | `dub_oscNoiseLevel` | |
| `oscDrift` | `dub_oscDrift` | |
| `oscLevel` | `dub_oscLevel` | |
| `filterMode` | `dub_filterMode` | |
| `filterCutoff` | `dub_filterCutoff` | |
| `filterResonance` | `dub_filterResonance` | |
| `filterEnvAmt` | `dub_filterEnvAmt` | |
| `envAttack` | `dub_envAttack` | |
| `envDecay` | `dub_envDecay` | |
| `envSustain` | `dub_envSustain` | |
| `envRelease` | `dub_envRelease` | |
| `pitchEnvDepth` | `dub_pitchEnvDepth` | |
| `pitchEnvDecay` | `dub_pitchEnvDecay` | |
| `lfoRate` | `dub_lfoRate` | |
| `lfoDepth` | `dub_lfoDepth` | |
| `lfoDest` | `dub_lfoDest` | |
| `sendLevel` | `dub_sendLevel` | |
| `returnLevel` | `dub_returnLevel` | |
| `dryLevel` | `dub_dryLevel` | |
| `driveAmount` | `dub_driveAmount` | |
| `delayTime` | `dub_delayTime` | |
| `delaySync` | `dub_delaySync` | |
| `delayFeedback` | `dub_delayFeedback` | |
| `delayWear` | `dub_delayWear` | |
| `delayWow` | `dub_delayWow` | |
| `delayMix` | `dub_delayMix` | |
| `reverbSize` | `dub_reverbSize` | |
| `reverbDamp` | `dub_reverbDamp` | |
| `reverbMix` | `dub_reverbMix` | |
| `voiceMode` | `dub_voiceMode` | |
| `voiceGlide` | `dub_voiceGlide` | |
| `masterVolume` | `dub_masterVolume` | |

**Note:** XOverdub's send/return FX are engine-owned (`ownsEffects() = true`). The delay/reverb/drive parameters stay inside the engine's `parameters` object, NOT in `fx_rack`. The `fx_rack` field is omitted for XOverdub presets.

**Mood mapping from XOverdub categories:**

| XOverdub Category | Mega-Tool Mood |
|-------------------|----------------|
| Dub Bass | Grounded |
| Dub Leads | Sharp |
| Dub Chords | Sharp |
| Log Drums | Grounded |
| Echo FX | Broken |
| Tape Chaos | Broken |
| Dub Atmospheres | Floating |
| Drone Machines | Ritual |

#### XObese (JSON) to .xomega

**Source format:** JSON with ~45 parameters.

**Strategy:** Wrap as single-engine `.xomega` with module ID `fat`. Apply `fat_` prefix to all parameter IDs.

**Representative Parameter Mapping:**

| Original ID | Namespaced ID | Notes |
|-------------|---------------|-------|
| `mojo` | `fat_mojo` | Signature saturation parameter |
| `fatness` | `fat_fatness` | Multi-oscillator spread |
| `filterCutoff` | `fat_filterCutoff` | |
| `filterRes` | `fat_filterRes` | |
| `oscLevel_1` through `oscLevel_13` | `fat_oscLevel_1` through `fat_oscLevel_13` | 13-oscillator unison |
| `attackTime` | `fat_attackTime` | |
| `decayTime` | `fat_decayTime` | |
| `sustainLevel` | `fat_sustainLevel` | |
| `releaseTime` | `fat_releaseTime` | |
| `masterLevel` | `fat_masterLevel` | |

**Mood mapping from XObese categories:**

| XObese Category | Mega-Tool Mood |
|-----------------|----------------|
| Bass | Grounded |
| Keys | Sharp |
| Pads | Floating |
| Leads | Sharp |
| FX | Broken |
| Textures | Floating |
| Rhythmic | Grounded |
| Init | Grounded |

#### XOverbite (JSON with schema_version) to .xomega

**Source format:** JSON with `schema_version`, 122 parameters.

**Strategy:** Wrap as single-engine `.xomega` with module ID `poss`. Apply `poss_` prefix.

**Representative Parameter Mapping:**

| Original ID | Namespaced ID | Notes |
|-------------|---------------|-------|
| `oscAWaveform` | `poss_oscAWaveform` | Primary oscillator waveform |
| `oscAShape` | `poss_oscAShape` | Oscillator A waveshape |
| `filterCutoff` | `poss_filterCutoff` | |
| `filterReso` | `poss_filterReso` | |
| `oscBWaveform` | `poss_oscBWaveform` | |
| `oscBShape` | `poss_oscBShape` | |
| `oscMix` | `poss_oscMix` | |
| `subLevel` | `poss_subLevel` | |
| `noiseLevel` | `poss_noiseLevel` | |
| `weightShape` | `poss_weightShape` | |
| `weightLevel` | `poss_weightLevel` | |
| `ampAttack` | `poss_ampAttack` | |
| `ampDecay` | `poss_ampDecay` | |
| `ampSustain` | `poss_ampSustain` | |
| `ampRelease` | `poss_ampRelease` | |
| `lfo1Rate` | `poss_lfo1Rate` | |
| `lfo1Depth` | `poss_lfo1Depth` | |

**Mood mapping:** XOverbite organizes by character spectrum (Plush to Aggressive). Map based on sonic role:
- Bass-forward presets -> `"Grounded"`
- Pad/texture presets -> `"Floating"`
- Aggressive/bitey presets -> `"Broken"` or `"Sharp"`
- Character-heavy presets -> `"Entangled"`

#### XOdyssey (JSON with schema_version) to .xomega

**Source format:** JSON with `schema_version`, ~130 parameters.

**Strategy:** Wrap as single-engine `.xomega` with module ID `drift`. Apply `drift_` prefix.

**Representative Parameter Mapping:**

| Original ID | Namespaced ID | Notes |
|-------------|---------------|-------|
| `journey` | `drift_journey` | Familiar-to-Alien macro |
| `breathe` | `drift_breathe` | Modulation depth macro |
| `bloom` | `drift_bloom` | Attack bloom macro |
| `fracture` | `drift_fracture` | Distortion/chaos macro |
| `oscA_wave` | `drift_oscA_wave` | |
| `oscA_level` | `drift_oscA_level` | |
| `oscB_wave` | `drift_oscB_wave` | |
| `oscB_level` | `drift_oscB_level` | |
| `filterA_cutoff` | `drift_filterA_cutoff` | Cytomic SVF LP |
| `filterA_res` | `drift_filterA_res` | |
| `filterB_cutoff` | `drift_filterB_cutoff` | Formant filter |
| `filterB_res` | `drift_filterB_res` | |
| `haze_amount` | `drift_haze_amount` | Pre-filter saturation |
| `prism_amount` | `drift_prism_amount` | Post-filter shimmer |
| `climax_threshold` | `drift_climax_threshold` | Per-preset JOURNEY position triggering climax bloom |
| `env1_attack` | `drift_env1_attack` | |
| `env1_decay` | `drift_env1_decay` | |
| `env1_sustain` | `drift_env1_sustain` | |
| `env1_release` | `drift_env1_release` | |
| `lfo1_rate` | `drift_lfo1_rate` | |
| `lfo1_depth` | `drift_lfo1_depth` | |
| `drift_amount` | `drift_drift_amount` | Voyager Drift mod source |
| `masterLevel` | `drift_masterLevel` | |

**Mood mapping from XOdyssey categories:**

| XOdyssey Category | Mega-Tool Mood |
|-------------------|----------------|
| Pads | Floating |
| Leads | Sharp |
| Keys | Sharp |
| Bass | Grounded |
| Textures | Floating or Broken (split by character) |

#### XOblong (JUCE binary state) to .xomega

**Source format:** JUCE `MemoryBlock` binary state via `getStateInformation()` / `setStateInformation()`.

**Strategy:** Load binary state into a temporary JUCE `AudioProcessorValueTreeState`, extract parameter values programmatically, write as `.xomega` with module ID `bob` and `bob_` prefix.

**This requires a C++ migration utility** since the binary state cannot be parsed outside of JUCE. The utility:
1. Instantiates `XOblongProcessor` headlessly
2. Calls `setStateInformation()` with the binary data
3. Reads all APVTS parameter values
4. Writes the `.xomega` JSON with `bob_` prefixed IDs

**Representative Parameter Mapping:**

| Original ID | Namespaced ID | Notes |
|-------------|---------------|-------|
| `warmth` | `bob_warmth` | Character warmth |
| `curiosity` | `bob_curiosity` | Movement/animation amount |
| `filterCutoff` | `bob_filterCutoff` | |
| `filterRes` | `bob_filterRes` | |
| `osc1Wave` | `bob_osc1Wave` | |
| `osc1Level` | `bob_osc1Level` | |
| `chorusMix` | `bob_chorusMix` | |
| `reverbMix` | `bob_reverbMix` | |
| `envAttack` | `bob_envAttack` | |
| `envDecay` | `bob_envDecay` | |
| `envSustain` | `bob_envSustain` | |
| `envRelease` | `bob_envRelease` | |
| `masterLevel` | `bob_masterLevel` | |

**Mood mapping from XOblong categories:**

| XOblong Category | Mega-Tool Mood |
|---------------------|----------------|
| WarmPads | Floating |
| DreamPads | Ritual |
| CuriousMotion | Entangled |
| SoftLeads | Sharp |
| WarmBass | Grounded |
| Textures | Floating |
| LoFiAtmospheres | Broken |
| Oddball | Broken |
| Plucks | Sharp |
| KeysAndBells | Sharp |
| Living | Entangled |

### 3.3 Migration Tool Architecture

Two migration paths are provided:

**Path A: Python migration script** (for JSON-based formats)

Location: `~/Documents/GitHub/OddfeliX/OddOscar/tools/preset_migrator/migrate.py`

```
Usage: python migrate.py --source <path> --format <xocmeta|json> --engine <module_id> --output <dir>
```

Handles: `.xocmeta` (OddfeliX/OddOscar), JSON (XOverdub, XObese, XOverbite, XOdyssey)

**Path B: C++ migration utility** (for JUCE binary state)

Location: `~/Documents/GitHub/OddfeliX/OddOscar/tools/preset_migrator/BinaryMigrator.cpp`

Handles: XOblong binary presets

Compiled as a JUCE console application that links against the target instrument's processor class headlessly.

### 3.4 Migration Validation

After migration, each `.xomega` file is validated against:

1. **Schema validation:** All required fields present, types correct, enums valid
2. **Parameter range check:** All parameter values within their declared ranges
3. **Round-trip test:** Load the `.xomega` in the mega-tool engine, snapshot parameters, compare against original preset values (must match within floating-point tolerance of 1e-6)
4. **Audio null test:** Render 4 bars from the original standalone build and from the mega-tool with the migrated preset. Difference must be silence (within -120 dBFS)

---

## 4. Category & Tagging System

### 4.1 Tier 1: Mood Categories (Required, Exclusive)

Every preset maps to exactly one mood. These are the top-level browsing categories.

| Mood | Color | Intent | Description |
|------|-------|--------|-------------|
| **Grounded** | Terracotta (#C8553D) | Rhythmic, percussive, bass-heavy, earthy | Sounds that anchor a track. Kicks, bass, stabs, rhythmic patterns. |
| **Floating** | Teal (#2A9D8F) | Atmospheric, evolving, lush, sustained | Sounds that fill space. Pads, drones, washes, textures. |
| **Entangled** | Gold (#D4A843) | Cross-coupled, reactive, dub pump | Sounds where engines modulate each other. The mega-tool signature. |
| **Sharp** | Silver (#B8C4CC) | Leads, keys, bells, melodic, articulate | Sounds that cut through. Solo voices, plucks, stabs, melodic lines. |
| **Broken** | Crimson (#8B1A1A) | Glitchy, chaotic, experimental, FX | Sounds that surprise. Tape chaos, fracture, codec errors, noise. |
| **Ritual** | Indigo (#2E1A47) | Cinematic, vast, ambient, spiritual | Sounds for meditation, film, ceremony. Deep space, drones. |

### 4.2 Engine Tags (Auto-Applied)

Applied automatically based on which module IDs appear in the preset's `engines` array. Users cannot add or remove these manually.

| Tag | Applied When |
|-----|-------------|
| `oddx` | `engines` contains module `"oddx"` |
| `oddo` | `engines` contains module `"oddo"` |
| `dub` | `engines` contains module `"dub"` |
| `fat` | `engines` contains module `"fat"` |
| `bite` | `engines` contains module `"bite"` |
| `drift` | `engines` contains module `"drift"` |
| `bob` | `engines` contains module `"bob"` |
| `onset` | `engines` contains module `"onset"` |

### 4.3 Vibe Tags (Author-Curated)

Set by the preset author via the `vibe` field. One vibe per preset maximum.

| Vibe | Sonic Character |
|------|----------------|
| `dark` | Low frequencies emphasized, minor tonality, filtered |
| `bright` | High frequencies present, open filters, shimmering |
| `warm` | Analog character, gentle saturation, mid-focused |
| `cold` | Thin, metallic, digital, sparse |
| `aggressive` | Heavy distortion, harsh transients, loud |
| `gentle` | Soft attack, low velocity sensitivity, quiet |
| `psychedelic` | Modulated, shifting, complex movement |
| `vintage` | Lo-fi, tape character, retro tones |
| `modern` | Clean, precise, contemporary production |
| `experimental` | Unconventional, boundary-pushing, unexpected |
| `cinematic` | Evocative, wide, suited for film/media |
| `lo-fi` | Degraded, crunchy, bit-reduced |
| `pristine` | Ultra-clean, unprocessed, transparent |
| `organic` | Natural, acoustic-like, living |
| `synthetic` | Obviously electronic, machine-made |

### 4.4 Complexity Tags (Auto-Calculated)

Computed at index time based on the preset's `engines` array length and `coupling` presence. Users cannot modify these.

| Tag | Condition |
|-----|-----------|
| `single-engine` | `engines.length == 1` |
| `dual-engine` | `engines.length == 2` |
| `multi-engine` | `engines.length >= 3` |
| `coupled` | `coupling` object is present and has at least one pair with `amount > 0` |
| `chained` | `engines.length >= 2` regardless of coupling (layered or split) |

### 4.5 Instrument Type Tags

Set via the `instrument_type` field. One type per preset.

| Type | Description |
|------|-------------|
| `synth` | General-purpose synthesizer sound |
| `drums` | Drum kit or percussion pattern |
| `bass` | Bass-register sound (designed for low end) |
| `pad` | Sustained, atmospheric, chord-capable |
| `lead` | Monophonic or prominent melodic voice |
| `keys` | Piano-like, organ-like, or keyboard instrument |
| `arp` | Arpeggiated pattern |
| `sequence` | Step-sequenced pattern |
| `drone` | Sustained single-note texture |
| `fx` | Sound effect, riser, impact, transition |
| `vocal` | Voice-like, formant-based |
| `percussion` | Individual percussion hit (not a full kit) |

### 4.6 User-Defined Tags

The `tags` array can contain any lowercase alphanumeric string (plus hyphens). This is where free-form descriptors live. Recommended tags to use consistently:

**Sonic character:** `dub`, `ambient`, `glitch`, `noise`, `distorted`, `clean`, `filtered`, `resonant`, `detuned`, `unison`

**Musical context:** `rhythmic`, `melodic`, `harmonic`, `atonal`, `percussive`, `sustained`, `evolving`, `static`

**Cultural reference:** `dubstep`, `techno`, `house`, `ambient`, `idm`, `jungle`, `dub`, `lo-fi-hip-hop`, `cinematic`, `game-audio`

**Technical descriptor:** `mono`, `poly`, `legato`, `glide`, `sidechain`, `sequenced`, `coupled`, `morphing`

### 4.7 Tag Cardinality Rules

- `mood`: Exactly 1 (required)
- `vibe`: 0 or 1 (optional)
- `instrument_type`: 0 or 1 (optional, defaults to `"synth"`)
- Engine tags: Auto-calculated, 1-4
- Complexity tags: Auto-calculated, 1-2
- User-defined `tags`: 1-20 items

---

## 5. Preset Browser UI

### 5.1 Layout

```
+-----------------------------------------------------------------------+
|  [Search: ______________] [Engine: v All] [Vibe: v All] [Type: v All] |  <- TOP BAR
|  [Complexity: v All]                                                   |
+-----------------------------------------------------------------------+
|  [Grounded] [Floating] [Entangled] [Sharp] [Broken] [Ritual] [All]   |  <- MOOD TABS
+------------------+----------------------------+-----------------------+
|                  |                            |                       |
|  CATEGORY TREE   |      PRESET LIST           |    PREVIEW PANEL      |
|                  |                            |                       |
|  v All Presets   | Name          | Engine | * |  Name: Dub Pressure   |
|  v Factory       | -----------  | ------ | - |  Author: XO_OX        |
|    v Grounded    | Dub Pressure | [O][D] | * |  Mood: Entangled      |
|    v Floating    | Neural Rhythm| [P][O] |   |  Engines: [onset][dub]|
|    v Entangled   | Fat Bite Bass| [F][B] | * |  Vibe: dark           |
|    v Sharp       | ...          | ...    |   |  Type: drums          |
|    v Broken      |              |        |   |                       |
|    v Ritual      |              |        |   |  "Kick pumps the dub  |
|  v User          |              |        |   |   delay. XOnset drums |
|    v My Presets   |              |        |   |   coupled with dub    |
|    v Mutations   |              |        |   |   send/return."       |
|  > Favorites     |              |        |   |                       |
|  > Recent        |              |        |   |  Tags: dub, drums,   |
|  > Start Here    |              |        |   |  coupled, dark        |
|  > Hero Presets  |              |        |   |                       |
|                  |              |        |   |  [Coupling Diagram]   |
|                  |              |        |   |   onset --[amp>flt]-- |
|                  |              |        |   |          --> dub      |
|                  |              |        |   |                       |
+------------------+----------------------------+-----------------------+
|  [< Prev]  "Dub Pressure" by XO_OX (Entangled)  [Next >]  [LOAD]    |  <- FOOTER
+-----------------------------------------------------------------------+
```

### 5.2 Panel Specifications

#### Left Panel: Category Tree

- **Width:** 180px fixed
- **Content:** Hierarchical tree with expand/collapse
- **Top-level nodes:**
  - **All Presets** -- total count badge
  - **Factory** -- expandable to mood sub-categories, each with count badge
  - **User** -- expandable to user-defined folders
  - **Favorites** -- virtual category (star-toggled presets)
  - **Recent** -- last 20 loaded presets (FIFO)
  - **Start Here** -- curated 10-preset intro collection
  - **Hero Presets** -- top 20 across all engines
- **Behavior:** Clicking a node filters the center panel. Multiple selections allowed with Cmd+Click.

#### Center Panel: Preset List

- **Content:** Scrollable list of presets matching current filters
- **Columns:**
  - **Name** (sortable A-Z, Z-A)
  - **Engine(s)** (icon badges, sortable)
  - **Date Modified** (sortable, hidden by default)
  - **Favorite Star** (toggle on click)
- **Row behavior:**
  - Single click: Select and preview (loads preset into preview buffer without committing to audio thread)
  - Double click: Load preset (commits to audio thread, replaces current state)
  - Right click: Context menu (Add to Favorites, Export, Duplicate, Delete [user only], Show in Finder)
- **Sorting:** Click column headers. Default sort: mood group, then alphabetical within group.

#### Right Panel: Preview Info

- **Width:** 220px fixed
- **Content:** Full metadata display for selected preset
- **Fields displayed:**
  - Preset name (large, bold)
  - Author
  - Mood badge (colored chip)
  - Engine badges (colored icons per module, showing module display name)
  - Vibe tag (if set)
  - Instrument type
  - Description text (multi-line, scrollable if needed)
  - Tags (horizontal chip list, clickable to filter)
  - Coupling diagram (if multi-engine): visual graph showing engine nodes and coupling lines with type/amount labels
- **Coupling diagram:** Simple node-and-edge visualization. Each engine is a colored circle (using its brand colour). Coupling pairs are drawn as lines with arrows showing direction, labeled with type abbreviation and amount percentage.

#### Top Bar: Search & Filters

- **Search field:** Full-text search across name, description, tags, author. Results ranked by relevance. Minimum 2 characters to trigger search.
- **Engine dropdown:** Filter by module ID. Options: All, OddfeliX/OddOscar, XOverdub, XObese, XOverbite, XOdyssey, XOblong, XOnset, Multi-Engine
- **Vibe dropdown:** Filter by vibe tag. Options: All, plus all 15 vibe values
- **Type dropdown:** Filter by instrument_type. Options: All, plus all 12 type values
- **Complexity dropdown:** Filter by complexity tag. Options: All, Single-Engine, Dual-Engine, Multi-Engine, Coupled

### 5.3 Special Features

#### Preview (Audition Without Committing)

Single-clicking a preset in the center list loads it into a preview buffer. The current audio state is preserved. A small "Preview" indicator appears in the footer. The user hears the previewed preset. Pressing Escape or clicking away reverts to the previously loaded preset. Pressing Enter or clicking LOAD commits the preview.

**Implementation:** The `PresetManager` maintains two states: `currentState` (committed) and `previewState` (tentative). Preview loads into `previewState` and swaps it to the audio thread via async message. Revert swaps `currentState` back.

#### Favorites

Each preset has a boolean `isFavorite` flag stored in `Favorites.json` (a flat array of preset file paths). Toggling the star in the preset list updates this file. The Favorites virtual category shows all starred presets across all moods and engines.

#### Recent

The last 20 loaded presets (by file path) are stored in `Recent.json`. Updated on every successful LOAD (not preview). Oldest entries are evicted when the list exceeds 20.

#### User Presets

- Saved via the preset browser's Save button
- User defines: name, mood, tags, description
- Stored in `User/` directory (flat or user-created subfolders)
- Can be edited, renamed, deleted
- Clearly visually separated from Factory presets (different background tint)

---

## 6. Preset Mutation System

### 6.1 Concept

Mutation generates new presets by combining the "DNA" of two parent presets through parameter crossover and random perturbation. It is computationally trivial -- pure parameter manipulation -- but produces sonically surprising results that neither parent produces alone.

### 6.2 Parameter Genome

Each engine's parameters are extracted into a normalized genome: an ordered array of `[paramID, value]` pairs where every value is normalized to the 0.0-1.0 range based on the parameter's declared min/max.

```
Genome structure:
{
  "engine": "dub",
  "genes": [
    { "id": "dub_oscWave", "value": 0.67, "mutable": false },
    { "id": "dub_filterCutoff", "value": 0.45, "mutable": true },
    { "id": "dub_filterResonance", "value": 0.3, "mutable": true },
    { "id": "dub_delayFeedback", "value": 0.6, "mutable": true },
    { "id": "dub_voiceMode", "value": 0.0, "mutable": false },
    ...
  ]
}
```

### 6.3 Mutable vs Immutable Parameters

Certain parameters must NOT be mutated because changing them produces an invalid or unmusical state rather than a variation.

**Immutable (mutable = false):**

| Parameter Type | Reason | Examples |
|---------------|--------|---------|
| Oscillator mode/wave selectors | Discrete choices, not continuous | `oscWave`, `oscMode`, `algo_mode` |
| Voice mode | Mono/poly/legato changes playing behavior | `voiceMode` |
| Sync toggles | Boolean switches, not gradients | `delaySync` |
| Envelope shape selectors | Discrete mode switches | `env_shape` |
| Polyphony count | Discrete integer | `polyphony` |
| Performance pad states | Momentary triggers, not preset state | `padFire`, `padSend` |

**Mutable (mutable = true) -- everything else:**

All continuous parameters (cutoff, resonance, level, time, amount, depth, rate, mix, drive, blend, decay, attack, sustain, release, etc.) are mutable.

### 6.4 Crossover Methods

#### Single-Point Crossover

Split the genome at a random index. Take genes 0..N from Parent A, genes N+1..end from Parent B.

```
Parent A: [A0, A1, A2, | A3, A4, A5, A6, A7]
Parent B: [B0, B1, B2, | B3, B4, B5, B6, B7]
                        ^
                   crossover point (random)

Child:    [A0, A1, A2,   B3, B4, B5, B6, B7]
```

Best for: Presets within the same engine (same genome structure).

#### Uniform Crossover

For each gene, independently choose from Parent A (50% probability) or Parent B (50% probability).

```
Parent A: [A0, A1, A2, A3, A4, A5, A6, A7]
Parent B: [B0, B1, B2, B3, B4, B5, B6, B7]
Coin:     [ A,  B,  B,  A,  B,  A,  A,  B]

Child:    [A0, B1, B2, A3, B4, A5, A6, B7]
```

Best for: Any combination, produces more varied results.

#### Weighted Crossover

Like uniform, but the probability is configurable. A slider controls the Parent A/B bias from 0% (pure B) to 100% (pure A).

```
Slider at 70% (favoring Parent A):
Each gene: 70% chance from A, 30% chance from B
```

Best for: When the user likes one parent more and wants to "season" it with the other.

### 6.5 Mutation (Post-Crossover Perturbation)

After crossover, apply random perturbation to each mutable gene:

```
For each mutable gene in child:
  if random() < mutation_rate:
    perturbation = random_gaussian(0, mutation_strength)
    gene.value = clamp(gene.value + perturbation, 0.0, 1.0)
```

**Configurable parameters:**

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| `mutation_rate` | 0.0-1.0 | 0.3 | Probability of mutating each gene (30% = about 1 in 3 parameters change) |
| `mutation_strength` | 0.0-0.5 | 0.15 | Standard deviation of perturbation (15% = subtle shifts, 50% = wild changes) |

### 6.6 Cross-Engine Mutation

When Parent A and Parent B use different engines, the child inherits the engine from the **primary parent** (left slot in the UI). The secondary parent's influence is applied through parameter analogy:

1. Identify "analogous" parameters across engines by semantic role:

| Semantic Role | Example Across Engines |
|--------------|----------------------|
| Filter cutoff | `dub_filterCutoff`, `snap_filterCutoff`, `poss_filterCutoff`, `drift_filterCutoff` |
| Filter resonance | `dub_filterReso`, `snap_filterReso`, `poss_filterReso`, `drift_filterReso` |
| Oscillator level | `dub_level`, `snap_level`, `fat_masterLevel` |
| Attack time | `dub_attack`, `snap_decay` (inverse mapping), `drift_attack` |
| Effect mix | `dub_delayMix`, `snap_delayMix` (via fx_rack) |
| Character macro | `dub_driveAmount`, `fat_mojo`, `poss_oscAShape`, `drift_driftDepth` |

2. For each analogous parameter pair, apply the secondary parent's normalized value with the configured crossover probability.

3. Parameters that have no analog in the other engine are left unchanged from the primary parent.

### 6.7 Mutation UI

```
+-----------------------------------------------------------------------+
|                          PRESET MUTATOR                                |
+-----------------------------------------------------------------------+
|                                                                       |
|  [Parent A: "Dub Pump"]          [Parent B: "Spooky Action"]          |
|  Mood: Entangled                 Mood: Entangled                      |
|  Engine: oddx + oddo             Engine: oddx + oddo                  |
|                                                                       |
|         [Parent A]  =====[Slider]=====  [Parent B]                    |
|                        70% / 30%                                      |
|                                                                       |
|  Method: [Uniform v]   Rate: [30% ---]   Strength: [15% ---]         |
|                                                                       |
|  +-----------------------------------------------------------+       |
|  |                                                           |       |
|  |   OFFSPRING: "Dub Pump x Spooky Action #3"                |       |
|  |   Mood: Entangled                                         |       |
|  |   Engine: oddx + oddo                                     |       |
|  |                                                           |       |
|  |   [PLAY]   [REROLL]   [SAVE]   [SAVE AS...]              |       |
|  |                                                           |       |
|  +-----------------------------------------------------------+       |
|                                                                       |
|  History: [#1] [#2] [#3*] [#4] [#5]    <- click to recall            |
|                                                                       |
+-----------------------------------------------------------------------+
```

**Workflow:**

1. User drags or selects two presets into the Parent A and Parent B slots.
2. User adjusts the A/B bias slider, crossover method, mutation rate, and strength.
3. User clicks REROLL to generate a new offspring. Each click produces a different random result.
4. User clicks PLAY to audition the offspring.
5. If the user likes it, SAVE commits it to `User/Mutations/` with auto-generated name.
6. History keeps the last 10 offspring so the user can compare and pick the best.

**Auto-naming convention for offspring:**

```
"{Parent A name} x {Parent B name} #{sequence_number}"
```

Example: `"Dub Pump x Spooky Action #3"`

The user can rename before saving.

---

## 7. Multi-Engine Preset Categories

### 7.1 Emergent Sound Categories

These categories exist ONLY through engine chaining and coupling. They represent sounds that no single engine can produce alone.

| Category | Engines Involved | Coupling | Description | Example Presets |
|----------|-----------------|----------|-------------|-----------------|
| **Pumped Pads** | Any melodic + `dub` | Amplitude sidechain from `dub` send/return | Melodic engine breathes with dub delay pump. The classic dub pad. | "Dub Pressure", "Breathing Reverb Pad" |
| **Morphing Drums** | `onset` + any melodic | `onset` amplitude -> melodic filter cutoff | Drum hits sculpt the pad's tonal character. Each kick brightens or darkens the pad. | "Neural Rhythm", "Membrane Choir" |
| **Fat Bite Bass** | `fat` + `bite` | `fat` 13-osc width feeds `bite` character stages | Massive unison width from XObese processed through XOverbite's Belly/Bite saturation. Sub-weight meets aggressive harmonics. | "Fat Bite Bass", "Obese Teeth" |
| **Psychedelic Rhythm** | `drift` + `onset` | `drift` Climax system triggers on `onset` pattern | XOdyssey's JOURNEY macro blooms on drum accents. The Climax effect is rhythmically triggered by percussion. | "Climax on the Beat", "Alien Drum Ritual" |
| **Dub Techno** | `oddx` (or `onset`) + `dub` | Percussive trigger -> delay feedback boost | Short percussive hits feed into XOverdub's tape delay at high feedback. Classic dub techno chord stab into infinite echo. | "Dub Techno Machine", "Infinite Chord" |
| **Alien Orchestra** | `drift` + any other + coupling | High bidirectional coupling between `drift` and target | XOdyssey's psychedelic modulation infects the target engine's parameters. Both engines drift in tandem. | "Third Mind Drift", "Alien Conversation" |
| **Living Texture** | 3+ engines | Light coupling (10-20%) across all pairs | Three or more engines with subtle mutual influence. No single engine dominates. The texture breathes, shifts, and evolves as coupling interactions compound. | "Full XO Orchestra", "Organism" |
| **Split Performance** | Any 2 engines | No coupling (or minimal) | One engine on low keys, another on high keys. Not coupled but allowing the performer to play two sounds from one keyboard. | "Bass + Lead Split", "Drums + Pad Split" |

### 7.2 Projected Multi-Engine Preset Counts

| Category | Target Count | Priority | Notes |
|----------|-------------|----------|-------|
| Pumped Pads | 15 | High | Core mega-tool demo sound |
| Morphing Drums | 15 | Highest | XOnset showcase |
| Fat Bite Bass | 8 | Medium | After both engines ship |
| Psychedelic Rhythm | 10 | Medium | After XOdyssey adapter |
| Dub Techno | 12 | High | MVP engine pair (oddx + dub) |
| Alien Orchestra | 8 | Medium | Requires XOdyssey |
| Living Texture | 10 | Low | Tri-engine, post-MVP |
| Split Performance | 7 | Medium | Keyboard splits |
| **Total new chained** | **85** | | |

### 7.3 Hero Chained Presets (The Demo Set)

The 20 most impressive cross-engine presets. These are what you play when someone asks "what does this thing do?"

Every hero chained preset must demonstrate:

1. **Audible coupling** -- you can hear the engines reacting to each other
2. **Macro control** -- at least one macro dramatically changes the coupling behavior
3. **Immediate musicality** -- sounds great with one finger on the keyboard
4. **Progressive reveal** -- more playing = more interaction = more interesting

---

## 8. XPN Export Integration

### 8.1 Export Architecture

The XPN export pipeline renders mega-tool presets as multi-sampled WAVs, then packages them using the validated `.xpn` format from the existing `xpn_exporter` tool.

**Key constraint:** Multi-engine presets with coupling CANNOT be decomposed into separate engine renders. The coupling IS the sound. They must be rendered with all engines active simultaneously.

### 8.2 Bundle Axes

| Bundle Mode | Description | MPC Expansion Name Pattern | Example |
|-------------|-------------|---------------------------|---------|
| **By Engine** | All presets from one engine | `com.xo-ox.<engine-name>` | `com.xo-ox.xoverdub` |
| **By Mood** | Cross-engine presets matching a mood category | `com.xo-ox.mega-<mood>` | `com.xo-ox.mega-grounded` |
| **By Vibe** | Cross-engine presets matching a vibe tag | `com.xo-ox.<vibe>-essentials` | `com.xo-ox.dark-essentials` |
| **By Type** | All presets of an instrument type | `com.xo-ox.<type>-collection` | `com.xo-ox.bass-collection` |
| **Full Pack** | Every preset across all engines | `com.xo-ox.mega-tool-v<version>` | `com.xo-ox.mega-tool-v1` |
| **Custom** | User-selected subset | User-defined | `com.xo-ox.my-live-set` |

### 8.3 Rendering Pipeline

```
1. PRESET SELECTION
   User selects presets to export via checkboxes in the export UI.

2. RENDER SETTINGS
   - Sample rate: 44100 or 48000 Hz (default: 44100)
   - Bit depth: 16 or 24 (default: 24)
   - Note range: C1-C6 (default), every minor 3rd = 21 notes
   - Velocity layers: 1, 2, or 4 (default: 1)
   - Render length: 2-8 seconds per note (default: 4s)
   - Tail: include release tail up to 2s (default: yes)

3. HEADLESS RENDER
   For each preset:
     For each note in range:
       For each velocity layer:
         a. Load preset into MegaToolProcessor (headless, no UI)
         b. Send MIDI Note On (note, velocity)
         c. Render audio for configured duration
         d. Send MIDI Note Off
         e. Render release tail
         f. Normalize, trim silence, fade out
         g. Save as WAV

4. WAV NAMING
   Convention: ENGINE__PRESET__NOTE__vLAYER.WAV
   Examples:
     dub__Sliding_Bass__C3__v1.WAV
     onset_dub__Dub_Pressure__C1__v1.WAV  (multi-engine: module IDs joined with _)

5. XPM GENERATION
   Generate MPC keygroup program files using xpn_exporter format.
   - One .xpm per preset
   - RootNote = MIDI + 1 (Convention 2)
   - KeyTrack = False
   - Application = MPC-V
   - Full PadNoteMap, PadGroupMap, FilterType/Cutoff

6. XPN PACKAGING
   Bundle WAVs + XPMs + metadata into .xpn expansion structure.
```

### 8.4 Export UI

```
+-----------------------------------------------------------------------+
|                        XPN EXPORT                                      |
+-----------------------------------------------------------------------+
|                                                                       |
|  BUNDLE BY:  (*) Engine  ( ) Mood  ( ) Vibe  ( ) Type  ( ) Custom    |
|                                                                       |
|  ENGINE FILTER: [v XOverdub]                                          |
|                                                                       |
|  +---------------------------------------------------------------+   |
|  | [ ] Select All                                                 |   |
|  | [x] Sliding Bass                               Grounded  dub  |   |
|  | [x] King Tubby                                  Sharp     dub  |   |
|  | [ ] Tape Meditation                             Floating  dub  |   |
|  | [x] Echo Spiral                                 Broken    dub  |   |
|  | ...                                                            |   |
|  +---------------------------------------------------------------+   |
|                                                                       |
|  RENDER SETTINGS                                                      |
|  Sample Rate:  [44100 v]  Bit Depth:  [24 v]                         |
|  Note Range:   [C1] to [C6]   (21 notes, every minor 3rd)            |
|  Velocity:     [1 layer v]                                            |
|  Duration:     [4 seconds]   Release Tail:  [x] Include              |
|                                                                       |
|  EXPANSION NAME: com.xo-ox.xoverdub                                  |
|                                                                       |
|  Estimated:  3 presets x 21 notes x 1 velocity = 63 WAVs             |
|  Disk space: ~126 MB (24-bit, 4s per note)                           |
|                                                                       |
|  [EXPORT]                           [Cancel]                          |
|                                                                       |
+-----------------------------------------------------------------------+
```

### 8.5 Multi-Engine Preset Handling

When exporting a multi-engine (coupled) preset to XPN:

1. All engines are loaded simultaneously
2. Coupling is active during render
3. The WAV file name uses joined module IDs: `onset_dub__Dub_Pressure__C1__v1.WAV`
4. The XPM file references these WAVs normally
5. The coupling interaction is "baked in" to the rendered audio

**Limitation:** The XPN version loses the real-time coupling interaction. The rendered version captures the coupling at the parameter values stored in the preset but cannot respond dynamically to playing style. This is documented in the expansion's metadata.

---

## 9. Preset File Organization

### 9.1 Directory Structure

```
~/Library/Application Support/XO_OX/MegaTool/
├── Factory/
│   ├── oddx_oddo/              # OddfeliX/OddOscar presets (always paired as 2-engine)
│   │   ├── Grounded/
│   │   │   ├── Amen Shatter.xomega
│   │   │   ├── Basement Stomp.xomega
│   │   │   └── ...
│   │   ├── Floating/
│   │   ├── Entangled/
│   │   └── Ritual/
│   ├── dub/                    # XOverdub presets (single-engine)
│   │   ├── Grounded/
│   │   ├── Sharp/
│   │   ├── Broken/
│   │   ├── Floating/
│   │   └── Ritual/
│   ├── fat/                    # XObese presets (single-engine)
│   │   ├── Grounded/
│   │   ├── Sharp/
│   │   ├── Floating/
│   │   └── Broken/
│   ├── bite/                   # XOverbite presets (single-engine)
│   │   ├── Grounded/
│   │   ├── Sharp/
│   │   ├── Floating/
│   │   └── Broken/
│   ├── drift/                  # XOdyssey presets (single-engine)
│   │   ├── Grounded/
│   │   ├── Floating/
│   │   ├── Sharp/
│   │   └── Ritual/
│   ├── bob/                    # XOblong presets (single-engine)
│   │   ├── Floating/
│   │   ├── Ritual/
│   │   ├── Entangled/
│   │   ├── Sharp/
│   │   ├── Grounded/
│   │   └── Broken/
│   ├── onset/                  # XOnset presets (single-engine)
│   │   ├── Grounded/
│   │   ├── Sharp/
│   │   ├── Entangled/
│   │   └── Broken/
│   └── chained/                # Multi-engine presets (new for mega-tool)
│       ├── Entangled/          # Coupled presets (largest category)
│       ├── Grounded/
│       ├── Floating/
│       ├── Sharp/
│       ├── Broken/
│       └── Ritual/
├── User/
│   ├── (user-defined folders or flat)
│   └── Mutations/              # Auto-generated from mutation system
├── Favorites.json              # Array of favorited preset file paths
├── Recent.json                 # Last 20 loaded preset file paths
└── SearchIndex.json            # Pre-built metadata index for fast browsing
```

### 9.2 File Naming Convention

- File name matches the preset's `name` field
- Spaces are preserved in file names (not replaced with underscores)
- Extension is `.xomega`
- File names must be unique within their directory
- Maximum file name length: 50 characters (including extension)
- Characters prohibited in file names: `/`, `\`, `:`, `*`, `?`, `"`, `<`, `>`, `|`

### 9.3 Search Index

`SearchIndex.json` is a flat array of metadata records, one per preset. It contains only the fields needed for browsing -- NOT the full parameter data. This enables the browser to load instantly without parsing every preset file.

```json
[
  {
    "path": "Factory/dub/Grounded/Sliding Bass.xomega",
    "name": "Sliding Bass",
    "author": "XO_OX Designs",
    "mood": "Grounded",
    "engines": ["dub"],
    "tags": ["dub", "bass", "glide", "warm"],
    "vibe": "warm",
    "instrument_type": "bass",
    "modified": "2026-03-08T00:00:00Z",
    "complexity": "single-engine"
  },
  ...
]
```

**Index rebuild triggers:**
- Application startup (full rebuild)
- Preset save (incremental add/update)
- Preset import (incremental add)
- Preset delete (incremental remove)
- Manual "Rebuild Index" button in settings (full rebuild)

---

## 10. Versioning & Compatibility

### 10.1 Schema Versioning

The `schema_version` field in every `.xomega` file tracks the format version. When the format evolves, migration functions transform older versions forward.

| Schema Version | Changes | Migration From Previous |
|----------------|---------|------------------------|
| `"1.0"` | Initial release. All fields defined in this spec. | N/A (first version) |
| `"1.1"` (future) | Example: add `sequencer` object for step pattern data | Copy all fields, add empty `sequencer` |
| `"2.0"` (future) | Example: restructure `coupling` to support per-sample routing | Transform `coupling.pairs` to new format |

**Migration rule:** The `PresetManager` always reads the `schema_version` first. If it is less than the current version, the migration chain executes: v1.0 -> v1.1 -> v2.0 -> ... -> current. Each step is a pure function: `migrate_N_to_N1(json) -> json`.

### 10.2 Engine Versioning

Each engine entry has its own `version` field tracking parameter schema changes within that engine.

| Engine | Version | Changes | Migration |
|--------|---------|---------|-----------|
| `dub` | `"1.0"` | Initial 38 parameters | N/A |
| `dub` | `"1.1"` (future) | Example: add `dub_reverbPreDelay` | Set `dub_reverbPreDelay` to default 0.0 |
| `onset` | `"1.0"` | Initial ~110 parameters | N/A |
| `oddx` | `"1.0"` | Initial ~24 parameters (mapped from OddfeliX/OddOscar) | N/A |
| `oddo` | `"1.0"` | Initial ~20 parameters (mapped from OddfeliX/OddOscar) | N/A |

**Engine migration rule:** If a preset's engine version is less than the current engine version, the engine-specific migration chain executes before the parameters are applied. New parameters receive their compiled default values. Removed parameters (if ever necessary) are silently ignored.

### 10.3 Compatibility Guarantees

1. **Never delete a parameter from the schema.** Deprecated parameters remain in the schema with their default values. The engine ignores them internally but they persist in saved files for backward compatibility.

2. **Never rename a parameter ID.** If a parameter's semantics change, create a new parameter with a new ID and deprecate the old one.

3. **Never change a parameter's value range.** If the range needs to expand, create a new parameter. The old parameter continues to work at the old range.

4. **Preset files are self-contained.** A `.xomega` file from version 1.0 must load correctly in version 3.0 (with migration). A file from version 3.0 loaded in version 1.0 gracefully ignores unknown fields.

5. **Unknown fields are preserved.** When a newer-version file is loaded by an older version, unknown top-level fields, unknown engine parameters, and unknown coupling types are preserved in memory and re-saved to disk. This prevents data loss when round-tripping through older software.

### 10.4 Preset Sharing

`.xomega` files can be shared between users via any file transfer mechanism (email, Dropbox, forums, etc.). The file is self-contained. The recipient:

1. Places the file in `~/Library/Application Support/XO_OX/MegaTool/User/`
2. The mega-tool detects the new file on next index rebuild (or immediately if file watching is active)
3. If the file references engines the user doesn't have installed/activated, the browser shows the preset as "unavailable" with a tooltip listing the missing engines

---

## 11. Implementation Notes

### 11.1 PresetManager Class

Location: `~/Documents/GitHub/OddfeliX/OddOscar/Source/Shared/Presets/PresetManager.h`

```cpp
namespace xo {

class PresetManager
{
public:
    // -- Lifecycle --
    PresetManager(const juce::File& presetRoot);
    void buildSearchIndex();          // Full index rebuild
    void rebuildIndexAsync();         // Background thread rebuild

    // -- Load / Save --
    bool loadPreset(const juce::File& file);                       // Full load -> audio thread
    bool previewPreset(const juce::File& file);                    // Preview (tentative)
    void commitPreview();                                           // Commit preview to current
    void revertPreview();                                           // Revert to previous state
    bool savePreset(const PresetState& state, const juce::File& file);
    bool saveUserPreset(const PresetState& state, const juce::String& name,
                        const juce::String& mood, const juce::StringArray& tags);

    // -- Browse --
    juce::Array<PresetMetadata> getAllPresets() const;
    juce::Array<PresetMetadata> filterPresets(const PresetFilter& filter) const;
    juce::Array<PresetMetadata> searchPresets(const juce::String& query) const;

    // -- Import / Export --
    bool importLegacyPreset(const juce::File& file, const juce::String& format,
                            const juce::String& engineModule);
    bool exportToXPN(const juce::Array<juce::File>& presets,
                     const XPNExportSettings& settings);

    // -- Mutate --
    PresetState mutatePresets(const PresetState& parentA, const PresetState& parentB,
                              const MutationSettings& settings);

    // -- Favorites / Recent --
    void toggleFavorite(const juce::File& file);
    bool isFavorite(const juce::File& file) const;
    juce::Array<PresetMetadata> getFavorites() const;
    juce::Array<PresetMetadata> getRecent() const;

private:
    juce::File presetRoot;
    juce::Array<PresetMetadata> searchIndex;
    juce::StringArray favoritePaths;
    juce::StringArray recentPaths;

    // -- Internal --
    PresetState parseXomega(const juce::File& file);
    juce::var   serializeXomega(const PresetState& state);
    PresetState migrateLegacy(const juce::var& json, const juce::String& format,
                              const juce::String& engineModule);
    PresetState migrateSchema(const PresetState& state);
};

} // namespace xo
```

### 11.2 JSON Parsing

All JSON operations use `juce::JSON::parse()` and `juce::JSON::toString()`. The `juce::var` type handles the dynamic JSON structure. No external JSON libraries are required.

**Parse flow:**
1. Read file to `juce::String`
2. `juce::JSON::parse(string)` -> `juce::var`
3. Validate required fields exist and have correct types
4. Extract metadata for search index (lightweight)
5. On full load: extract engine parameters, coupling, surface, fx_rack
6. Apply migration if `schema_version` < current
7. Apply engine migration if `engine.version` < current engine version

### 11.3 Lazy Loading

The browser never parses full preset files for display purposes. The search index contains only metadata. Full parameter parsing happens only when:

1. A preset is loaded (LOAD action)
2. A preset is previewed (single-click with preview mode active)
3. A preset is selected for mutation (parent slot assignment)
4. A preset is selected for XPN export (render pipeline)

This keeps browser startup time under 100ms even with 1000+ presets.

### 11.4 Thread Safety

| Operation | Thread | Mechanism |
|-----------|--------|-----------|
| Browse / search / filter | Message thread | Direct index access (read-only) |
| Index rebuild | Background thread | Builds new index, swaps atomically on message thread |
| Preset load (parse JSON) | Message thread | Parse -> build PresetState |
| Preset apply (to audio) | Message thread -> audio thread | `std::atomic` swap of parameter snapshot pointer |
| Preset preview | Message thread | Same as load, but saves current state for revert |
| Mutation | Message thread | Pure computation, no audio thread access |
| XPN render | Background thread | Headless processor instance, independent of live audio |
| Save to disk | Message thread | File I/O is fast (< 1ms for JSON write) |
| Favorites / Recent update | Message thread | Write to JSON files |

**Critical rule:** No preset loading code ever runs on the audio thread. The audio thread reads a `PresetState` snapshot that was prepared on the message thread and swapped in via atomic pointer exchange.

### 11.5 Search Index Performance

**Target:** < 50ms for any search/filter operation across 1000+ presets.

**Implementation:** The search index is a flat `juce::Array<PresetMetadata>` sorted by mood, then by name. Filtering creates a new array via linear scan with predicate. Full-text search uses case-insensitive substring matching across name, description, tags, and author fields.

For the projected library size (755-900 presets), linear scan is fast enough. If the library exceeds 2000 presets (community content scenario), switch to a trie-based index for text search.

### 11.6 Preset State Struct

```cpp
namespace xo {

struct EngineState
{
    juce::String moduleID;              // e.g., "dub"
    juce::String version;               // e.g., "1.0"
    int slot;                           // 0-3
    bool active;                        // true if producing audio
    juce::HashMap<juce::String, float> parameters;  // namespaced param -> value
};

struct CouplingPair
{
    int sourceSlot;
    int targetSlot;
    juce::String type;                  // e.g., "amp_to_filter"
    float amount;                       // 0.0-1.0
    juce::String direction;             // "unidirectional" or "bidirectional"
};

struct SurfaceState
{
    juce::String mode;                  // "pad", "fretless", "drum"
    juce::String scale;
    juce::String root;
    int octave;
    juce::String orbitRecipe;
    juce::String stripMode;
};

struct FXSlot
{
    juce::String type;                  // "delay", "reverb", etc.
    bool enabled;
    juce::HashMap<juce::String, float> params;
};

struct FXRackState
{
    bool shared;
    juce::Array<FXSlot> slots;
};

struct LegacyInfo
{
    juce::String sourceInstrument;
    juce::String sourcePresetName;
    juce::String sourceFormat;
    juce::String migrationVersion;
};

struct PresetState
{
    // Metadata
    juce::String schemaVersion;
    juce::String name;
    juce::String author;
    juce::String mood;
    juce::String category;
    juce::StringArray tags;
    juce::String instrumentType;
    juce::String vibe;
    juce::String description;
    juce::String created;
    juce::String modified;
    float tempo = 0.0f;                // 0 = not set
    juce::StringArray macroLabels;      // 4 entries, or empty

    // Engine state
    juce::Array<EngineState> engines;

    // Coupling
    juce::Array<CouplingPair> couplingPairs;

    // Surface
    SurfaceState surface;

    // FX
    FXRackState fxRack;

    // Legacy
    LegacyInfo legacy;

    // File reference (not serialized)
    juce::File sourceFile;
};

struct PresetMetadata
{
    juce::String path;                  // Relative to preset root
    juce::String name;
    juce::String author;
    juce::String mood;
    juce::StringArray engines;          // Module IDs
    juce::StringArray tags;
    juce::String vibe;
    juce::String instrumentType;
    juce::String modified;
    juce::String complexity;            // Auto-calculated
    bool isFavorite = false;
};

} // namespace xo
```

### 11.7 File Watching

On macOS, use `FSEvents` (via JUCE's `FileSystemWatcher` or native API) to monitor the preset root directory for changes. When a new `.xomega` file appears (e.g., user drops a file into the folder), automatically parse its metadata and add it to the search index.

On other platforms, fall back to periodic polling (every 5 seconds) or manual "Refresh" button.

---

## Appendix A: Complete .xomega JSON Schema (Formal)

```json
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://xo-ox.com/schemas/xomega-1.0.json",
  "title": "XO_OX Mega-Tool Preset",
  "description": "Unified preset format for the XO_OX Mega-Tool multi-engine synth platform",
  "type": "object",
  "required": ["schema_version", "format", "name", "author", "mood", "tags", "created", "modified", "engines"],
  "properties": {
    "schema_version": {
      "type": "string",
      "pattern": "^\\d+\\.\\d+$",
      "description": "Format version for migration"
    },
    "format": {
      "type": "string",
      "const": "xomega"
    },
    "name": {
      "type": "string",
      "minLength": 1,
      "maxLength": 40
    },
    "author": {
      "type": "string",
      "minLength": 1,
      "maxLength": 60
    },
    "mood": {
      "type": "string",
      "enum": ["Grounded", "Floating", "Entangled", "Sharp", "Broken", "Ritual"]
    },
    "category": {
      "type": ["string", "null"],
      "maxLength": 30
    },
    "tags": {
      "type": "array",
      "items": {
        "type": "string",
        "minLength": 1,
        "maxLength": 20,
        "pattern": "^[a-z0-9-]+$"
      },
      "minItems": 1,
      "maxItems": 20
    },
    "instrument_type": {
      "type": "string",
      "enum": ["synth", "drums", "bass", "pad", "lead", "keys", "arp", "sequence", "drone", "fx", "vocal", "percussion"],
      "default": "synth"
    },
    "vibe": {
      "type": ["string", "null"],
      "enum": [null, "dark", "bright", "warm", "cold", "aggressive", "gentle", "psychedelic", "vintage", "modern", "experimental", "cinematic", "lo-fi", "pristine", "organic", "synthetic"]
    },
    "description": {
      "type": "string",
      "maxLength": 200,
      "default": ""
    },
    "created": {
      "type": "string",
      "format": "date-time"
    },
    "modified": {
      "type": "string",
      "format": "date-time"
    },
    "tempo": {
      "type": ["number", "null"],
      "minimum": 30,
      "maximum": 300
    },
    "macro_labels": {
      "type": ["array", "null"],
      "items": {
        "type": "string",
        "minLength": 1,
        "maxLength": 12
      },
      "minItems": 4,
      "maxItems": 4
    },
    "engines": {
      "type": "array",
      "minItems": 1,
      "maxItems": 4,
      "items": {
        "type": "object",
        "required": ["module", "version", "slot", "parameters"],
        "properties": {
          "module": {
            "type": "string",
            "enum": ["oddx", "oddo", "dub", "fat", "bite", "drift", "bob", "onset"]
          },
          "version": {
            "type": "string",
            "pattern": "^\\d+\\.\\d+$"
          },
          "slot": {
            "type": "integer",
            "minimum": 0,
            "maximum": 3
          },
          "active": {
            "type": "boolean",
            "default": true
          },
          "parameters": {
            "type": "object",
            "additionalProperties": {
              "type": "number"
            }
          }
        }
      }
    },
    "coupling": {
      "type": ["object", "null"],
      "properties": {
        "pairs": {
          "type": "array",
          "maxItems": 6,
          "items": {
            "type": "object",
            "required": ["source_slot", "target_slot", "type", "amount"],
            "properties": {
              "source_slot": {
                "type": "integer",
                "minimum": 0,
                "maximum": 3
              },
              "target_slot": {
                "type": "integer",
                "minimum": 0,
                "maximum": 3
              },
              "type": {
                "type": "string",
                "enum": [
                  "amp_to_filter",
                  "lfo_to_pitch",
                  "env_to_morph",
                  "audio_to_fm",
                  "audio_to_ring",
                  "filter_to_filter",
                  "trigger_to_reset",
                  "amp_to_choke",
                  "rhythm_to_blend",
                  "env_to_decay"
                ]
              },
              "amount": {
                "type": "number",
                "minimum": 0.0,
                "maximum": 1.0
              },
              "direction": {
                "type": "string",
                "enum": ["unidirectional", "bidirectional"],
                "default": "unidirectional"
              }
            }
          }
        }
      }
    },
    "surface": {
      "type": ["object", "null"],
      "properties": {
        "mode": {
          "type": "string",
          "enum": ["pad", "fretless", "drum"],
          "default": "pad"
        },
        "scale": {
          "type": "string",
          "enum": ["chromatic", "major", "minor", "pentatonic", "blues", "dorian", "mixolydian", "harmonic_minor", "whole_tone", "diminished"],
          "default": "chromatic"
        },
        "root": {
          "type": "string",
          "enum": ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"],
          "default": "C"
        },
        "octave": {
          "type": "integer",
          "minimum": 0,
          "maximum": 7,
          "default": 3
        },
        "orbit_recipe": {
          "type": ["string", "null"]
        },
        "strip_mode": {
          "type": "string",
          "enum": ["dub_space", "filter_sweep", "coupling", "dub_siren"],
          "default": "dub_space"
        }
      }
    },
    "fx_rack": {
      "type": ["object", "null"],
      "properties": {
        "shared": {
          "type": "boolean",
          "default": true
        },
        "slots": {
          "type": "array",
          "maxItems": 6,
          "items": {
            "type": "object",
            "required": ["type", "params"],
            "properties": {
              "type": {
                "type": "string",
                "enum": ["delay", "reverb", "phaser", "lofi", "compressor", "chorus", "distortion"]
              },
              "enabled": {
                "type": "boolean",
                "default": true
              },
              "params": {
                "type": "object",
                "additionalProperties": {
                  "type": "number"
                }
              }
            }
          }
        }
      }
    },
    "legacy": {
      "type": ["object", "null"],
      "properties": {
        "source_instrument": { "type": ["string", "null"] },
        "source_preset_name": { "type": ["string", "null"] },
        "source_format": {
          "type": ["string", "null"],
          "enum": [null, "xocmeta", "json", "juce_binary"]
        },
        "migration_version": { "type": ["string", "null"] }
      }
    }
  },
  "additionalProperties": true
}
```

**Note:** `additionalProperties: true` at the top level ensures forward compatibility. Newer schema versions can add fields that older parsers silently preserve.

---

## Appendix B: Migration Checklist Per Engine

### OddfeliX/OddOscar (114 presets)

- [ ] Parse all 114 `.xocmeta` files from `~/Documents/GitHub/OddfeliX/OddOscar/Presets/Factory/`
- [ ] Split into `oddx` and `oddo` engine entries per preset
- [ ] Map `x`-prefixed parameters to `oddx_` namespace
- [ ] Map `o`-prefixed parameters to `oddo_` namespace
- [ ] Map FX parameters to `fx_rack` slots (delay, reverb, phaser, lofi, compressor)
- [ ] Map `couplingAmount` to `coupling.pairs[0]`
- [ ] Map `category` to `mood` (Grounded, Floating, Entangled, Deep Space -> Ritual)
- [ ] Preserve existing `tags`, `description`, `engineBalance`, `couplingIntensity`
- [ ] Set `macro_labels` to `["OddfeliX+OddOscar", "Bloom", "Coupling", "Delay+Reverb"]`
- [ ] Validate round-trip: load migrated preset, compare parameter values against original
- [ ] Audio null test: render 4 bars, diff against standalone render

### XOverdub (40 presets)

- [ ] Extract C++ factory preset definitions from source code
- [ ] Write JSON intermediary for each preset
- [ ] Wrap each as single-engine `.xomega` with module `"dub"`
- [ ] Apply `dub_` prefix to all 38 parameter IDs
- [ ] Assign mood tags based on XOverdub category
- [ ] Write descriptions and tags for all 40 presets
- [ ] Note: `fx_rack` is NOT used (XOverdub owns its effects)
- [ ] Validate round-trip and audio null test

### XObese (~52 presets)

- [ ] Parse existing JSON preset files
- [ ] Wrap each as single-engine `.xomega` with module `"fat"`
- [ ] Apply `fat_` prefix to all ~45 parameter IDs
- [ ] Assign mood tags based on XObese category
- [ ] Write descriptions and tags
- [ ] Note: XObese is a sampled instrument -- verify parameter semantics apply
- [ ] Validate and test

### XOverbite (~15 presets)

- [ ] Parse JSON preset files with `schema_version`
- [ ] Wrap each as single-engine `.xomega` with module `"poss"` (Overbite)
- [ ] Apply `poss_` prefix to all 122 parameter IDs
- [ ] Assign mood tags based on character spectrum
- [ ] Write descriptions and tags
- [ ] Validate and test

### XOdyssey (198 presets)

- [ ] Parse JSON preset files with `schema_version`
- [ ] Wrap each as single-engine `.xomega` with module `"drift"`
- [ ] Apply `drift_` prefix to all ~130 parameter IDs
- [ ] Assign mood tags based on XOdyssey category (Pads, Leads, Keys, Bass, Textures)
- [ ] Handle Textures split: ambient -> Floating, glitch/fracture -> Broken
- [ ] Write descriptions and tags for all 198 presets
- [ ] Validate and test

### XOblong (167 presets)

- [ ] Build C++ migration utility (BinaryMigrator)
- [ ] Load each binary state into headless processor
- [ ] Extract all APVTS parameter values
- [ ] Wrap each as single-engine `.xomega` with module `"bob"`
- [ ] Apply `bob_` prefix to all parameter IDs
- [ ] Assign mood tags based on XOblong category (11 categories)
- [ ] Write descriptions and tags for all 167 presets
- [ ] Validate and test

### XOnset (85 planned presets)

- [ ] No migration needed -- built fresh against `.xomega` format
- [ ] All presets authored directly as `.xomega` files
- [ ] Use `onset` module ID with `perc_` parameter prefix
- [ ] 6 categories map to moods: Circuit Kits -> Grounded, Algorithm Kits -> Sharp, Hybrid Kits -> Entangled, Coupled Kits -> Entangled, Morphing Kits -> Broken, XO Fusion Kits -> Entangled

---

## Appendix C: Preset Count Summary

| Source | Single-Engine | Multi-Engine (New) | Total |
|--------|--------------|-------------------|-------|
| OddfeliX/OddOscar | 114 | -- | 114 |
| XOverdub | 40 | -- | 40 |
| XObese | 52 | -- | 52 |
| XOverbite | ~15 | -- | ~15 |
| XOdyssey | 198 | -- | 198 |
| XOblong | 167 | -- | 167 |
| XOnset (planned) | 85 | -- | 85 |
| Chained presets (new) | -- | 85 | 85 |
| **Total** | **~671** | **85** | **~756** |

**Distribution by mood (projected):**

| Mood | Count | Percentage |
|------|-------|------------|
| Grounded | ~120 | 16% |
| Floating | ~165 | 22% |
| Entangled | ~150 | 20% |
| Sharp | ~180 | 24% |
| Broken | ~75 | 10% |
| Ritual | ~65 | 8% |
| **Total** | **~755** | **100%** |

---

## Appendix D: Glossary

| Term | Definition |
|------|-----------|
| **.xomega** | The unified preset file format for the XO_OX Mega-Tool. JSON-based, UTF-8 encoded. |
| **Coupling** | Cross-engine modulation where one engine's output modulates another engine's parameters. The XO_OX brand signature. |
| **Engine** | A self-contained synthesis module implementing the `SynthEngine` interface. Each XO_OX instrument becomes one engine. |
| **Module ID** | Short string identifier for an engine (e.g., `"dub"`, `"onset"`, `"drift"`). Used in preset files and parameter namespacing. |
| **Mood** | Top-level browsing category (Grounded, Floating, Entangled, Sharp, Broken, Ritual). Every preset has exactly one. |
| **Mutation** | Generating new presets through parameter crossover between two parent presets. |
| **Namespace prefix** | Engine-specific string prepended to all parameter IDs to prevent collisions (e.g., `dub_`, `oddx_`). |
| **Normalled routing** | Default coupling connections that make multi-engine presets sound musical without user patching. Inspired by Moog Matriarch. |
| **PlaySurface** | The unified interaction layer for note input, expression, and performance across all XO_OX instruments. |
| **Slot** | Engine position index (0-3) within a multi-engine preset. Determines rendering order. |
| **Vibe** | Sonic character descriptor tag (dark, bright, warm, etc.). Optional, max one per preset. |
| **XPN** | Akai MPC expansion format. Presets are rendered to WAV and packaged with XPM keygroup programs. |

---

*CONFIDENTIAL -- XO_OX Internal Design Document*
