# Preset Conventions

Standards and patterns for XOlokun preset creation, naming, structure, and quality.

## .xometa Format

Presets are JSON files with the `.xometa` extension. They are version-controlled and considered a core product feature.

### Required Fields

```json
{
  "name": "Velvet Thunder",
  "version": "1.0",
  "mood": "Foundation",
  "author": "XO_OX",
  "engines": ["OddfeliX", "Odyssey", "Opal", null],
  "parameters": {
    "snap_filterCutoff": 0.65,
    "odyssey_detune": 0.12,
    "opal_grainSize": 0.3
  },
  "macros": {
    "M1_CHARACTER": { "targets": ["snap_filterCutoff", "odyssey_detune"], "curve": "linear" },
    "M2_MOVEMENT": { "targets": ["opal_grainSize"], "curve": "exponential" },
    "M3_COUPLING": { "targets": ["coupling_intensity"], "curve": "linear" },
    "M4_SPACE": { "targets": ["reverb_mix", "delay_send"], "curve": "linear" }
  },
  "coupling": {
    "routes": [
      { "source": "OddfeliX", "target": "Odyssey", "type": "phase_mod", "intensity": 0.4 }
    ]
  },
  "dna": {
    "brightness": 0.6,
    "warmth": 0.7,
    "movement": 0.4,
    "density": 0.5,
    "space": 0.3,
    "aggression": 0.2
  },
  "tags": ["pad", "warm", "evolving"]
}
```

### Engine Slots

- 4 slots, indexed 0-3
- Use engine ID names (O-prefix convention): `"OddfeliX"`, `"Odyssey"`, etc.
- `null` for empty slots
- Legacy names (`Snap`, `Morph`, etc.) are resolved by `resolveEngineAlias()`

### Parameter Keys

- Format: `{prefix}_{paramName}` — see CLAUDE.md for prefix table
- Parameter IDs are **frozen** — never rename after release
- Only include parameters that differ from defaults (sparse storage)

## Naming Conventions

| Rule | Example | Violation |
|------|---------|-----------|
| 2-3 words | "Velvet Thunder" | "The Great Big Velvet Thunder Pad" |
| Max 30 chars | "Crystal Lattice" | "Crystalline Harmonic Lattice Resonance v2" |
| Evocative, not technical | "Midnight Bloom" | "FM Pad with LFO Mod" |
| No engine names | "Solar Wind" | "OddfeliX Pad" |
| No jargon | "Breathing Glass" | "Bandlimited PWM Sweep" |
| No duplicates | — | Same name as existing preset |
| Title Case | "Frozen Lake" | "frozen lake" or "FROZEN LAKE" |

### Mood-Appropriate Tone

| Mood | Naming Tone | Examples |
|------|-------------|---------|
| Foundation | Solid, grounded, elemental | "Iron Core", "Bedrock Pulse", "Granite" |
| Atmosphere | Spacious, ethereal, environmental | "Cloud Atlas", "Misty Harbor", "Thin Air" |
| Entangled | Complex, intertwined, chaotic | "Quantum Tangle", "Neural Web", "Symbiosis" |
| Prism | Bright, colorful, refractive | "Split Light", "Rainbow Engine", "Spectral" |
| Flux | Moving, changing, unstable | "Undertow", "Phase Drift", "Morphing Tide" |
| Aether | Vast, cosmic, transcendent | "Stellar Dust", "Void Echo", "Nebula Song" |

## Macro Requirements

All four macros must produce **audible change** in every preset:

| Macro | Purpose | Must Affect |
|-------|---------|------------|
| M1 CHARACTER | Timbral identity | Filter, oscillator shape, harmonic content |
| M2 MOVEMENT | Temporal animation | LFO depth/rate, modulation, grain parameters |
| M3 COUPLING | Cross-engine interaction | Coupling route intensity, ring mod depth |
| M4 SPACE | Spatial dimension | Reverb, delay, stereo width, diffusion |

**Test:** Sweep each macro 0→1 while others are at default. Each must produce a clearly audible, musically useful change.

## Sonic DNA Guidelines

### Value Ranges

All dimensions are 0.0–1.0. Avoid extremes (0.0 or 1.0) unless the preset genuinely sits at the boundary.

| Dimension | 0.0 | 0.5 | 1.0 |
|-----------|-----|-----|-----|
| Brightness | Very dark, muffled | Balanced | Extremely bright, fizzy |
| Warmth | Cold, thin, digital | Neutral | Very warm, saturated, analog |
| Movement | Completely static | Moderate modulation | Extremely animated |
| Density | Single thin voice | Moderate layering | Wall of sound |
| Space | Bone dry | Room reverb | Infinite hall |
| Aggression | Gentle, soft | Moderate edge | Harsh, distorted, angry |

### Mood → DNA Tendencies

| Mood | B | W | M | D | S | A |
|------|---|---|---|---|---|---|
| Foundation | 0.3-0.6 | 0.4-0.7 | 0.1-0.3 | 0.3-0.6 | 0.1-0.3 | 0.1-0.3 |
| Atmosphere | 0.3-0.5 | 0.4-0.6 | 0.2-0.5 | 0.2-0.5 | 0.6-0.9 | 0.0-0.2 |
| Entangled | 0.4-0.7 | 0.3-0.6 | 0.5-0.9 | 0.6-0.9 | 0.3-0.6 | 0.3-0.6 |
| Prism | 0.6-0.9 | 0.3-0.5 | 0.3-0.7 | 0.3-0.6 | 0.3-0.5 | 0.1-0.4 |
| Flux | 0.4-0.7 | 0.3-0.6 | 0.7-1.0 | 0.4-0.7 | 0.2-0.5 | 0.2-0.5 |
| Aether | 0.5-0.8 | 0.3-0.5 | 0.2-0.5 | 0.1-0.4 | 0.7-1.0 | 0.0-0.2 |

## Quality Rules

1. **Dry sound first** — Preset must sound compelling with space/effects at zero
2. **No silence** — Playing a note must produce sound immediately (no 5-second attack pad as default)
3. **No clipping** — Output must stay within safe range at all velocity levels
4. **No DC offset** — Sustained notes must not drift from center
5. **No excessive noise** — Noise floor should be inaudible when note is off
6. **Coupling adds, doesn't break** — With coupling at zero, the preset must still work
7. **Macros are safe** — Sweeping any macro 0→1 must not cause clipping, silence, or artifacts

## File Organization

```
Presets/XOlokun/
├── Foundation/
│   ├── Velvet_Thunder.xometa
│   └── Iron_Core.xometa
├── Atmosphere/
│   └── Cloud_Atlas.xometa
├── Entangled/
│   └── Quantum_Tangle.xometa
├── Prism/
│   └── Split_Light.xometa
├── Flux/
│   └── Undertow.xometa
└── Aether/
    └── Stellar_Dust.xometa
```

- Filename: snake_case version of preset name
- One preset per file
- Sorted into mood folders
- Target: ~167 presets per mood (1000 total ÷ 6 moods)

## Coupling in Presets

- At least one coupling route for any preset using 2+ engines
- Coupling intensity controlled by M3 macro
- Document unusual coupling configurations in comments (JSON doesn't support comments, so use a `"notes"` field)
- Bidirectional coupling must be flagged for stability review
