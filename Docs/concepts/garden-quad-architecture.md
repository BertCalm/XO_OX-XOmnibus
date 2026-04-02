# GARDEN Quad — Architecture Document

*Built 2026-03-21 | 4 Engines | Strings Family with Evolutionary Coupling*

---

## Overview

The GARDEN quad is the Strings instrument family for XOceanus. Four engines spanning orchestral, solo, chamber, and synth strings, unified by evolutionary coupling — state that accumulates over session time and never automatically resets.

## Engines

| Engine | Short | Prefix | Color | Voices | DSP Core | Quad Role |
|--------|-------|--------|-------|--------|----------|-----------|
| XOrchard | ORCHARD | `orch_` | Harvest Gold `#DAA520` | 4 | 4-osc detuned saw ensemble + formant filter | Climax species |
| XOvergrow | OVERGROW | `grow_` | Vine Green `#3A5F0B` | 4 | Karplus-Strong string + runners + bow noise | Intermediate |
| XOsier | OSIER | `osier_` | Willow Green `#6B8E23` | 4 | 2-osc saw + per-role tone shaping + companion planting | Later intermediate |
| XOxalis | OXALIS | `oxal_` | Clover Purple `#6A0DAD` | 4 | Phyllotaxis harmonic bank (golden ratio spacing) | Pioneer species |

## Shared Infrastructure

### GardenAccumulators.h (`Source/DSP/GardenAccumulators.h`)

Three accumulator dimensions, updated once per block (not per-sample):

- **W (Warmth)**: Leaky integrator. Rises with sustained playing (+velocity, +note count). Decays toward floor (>0) during silence. Maps to: HF roll-off, inter-voice phase lock, vibrato smoothing.
- **A (Aggression)**: Rises with high velocity beyond threshold. Square-root decay (stress lingers). Maps to: bow pressure, vibrato irregularity, ensemble drift.
- **D (Dormancy)**: Rises during silence. Resets on note-on. Maps to: attack noise, pitch variance, vibrato stiffness.

### Session Seasons

Derived from W/A/D combination:

| Season | Condition | Character |
|--------|-----------|-----------|
| Spring | Default / low all | Bright, new, slightly uneven |
| Summer | High W, low A, low D | Peak expression, lush, warm |
| Fall | High A or W saturation | Rich but strained, deeper tones |
| Winter | High D | Dormant, cold, interior |

### Mycorrhizal Network (`GardenMycorrhizalNetwork`)

Event-queue based cross-voice state propagation (Decision G1):
- 6 channels for 4 voices (C(4,2) pairs)
- 32-event circular buffer per channel
- Memory: ~7.2 KB total (vs. 39.5 MB for naive audio delay lines)
- Propagation delay: 2-8 seconds (configurable)
- Transmits stress (A), warmth (W), not audio

### Growth Mode

Per-engine opt-in mode where note-on = seed. Harmonics germinate over time:
- Phase 1 (0-10%): Near silence, sub-harmonic presence
- Phase 2 (10-40%): Fundamental emerges
- Phase 3 (40-70%): Harmonics fill rapidly
- Phase 4 (70-100%): Full bloom

Growth Mode controlled by `{prefix}_growthMode` (on/off) and `{prefix}_growthTime` (5-60 seconds).

## Per-Engine Architecture

### XOrchard (Orchestral)
- **4 detuned PolyBLEP sawtooth oscillators** per voice (string section ensemble)
- **Formant-shaped filter** adds orchestral body resonance (BPF blended with main signal)
- **Concertmaster mechanism**: highest voice influences ensemble timing
- **Season override**: `orch_season` parameter for manual season control (-1 = auto)
- **D006**: Mod wheel → vibrato depth, Aftertouch → filter cutoff
- **Params**: 28 (ADSR, filter, detune, formant, vibrato, season, brightness, warmth, growth, bend, 4 macros, 2 LFOs)

### XOvergrow (Solo)
- **Karplus-Strong string model**: noise burst → tuned delay line → one-pole damping → feedback
- **Runner generator**: stressed notes spawn sub-harmonic sympathetic resonances at 2-8 second delay
- **Bow noise injection**: continuous per-sample noise excitation for bowed character
- **Wildness parameter**: controls pitch jitter, runner probability, unpredictability
- **D006**: Mod wheel → vibrato depth, Aftertouch → filter cutoff
- **Params**: 26 (string model, wildness, filter, ADSR, vibrato, growth, bend, 4 macros, 2 LFOs)

### XOsier (Chamber)
- **2 detuned sawtooth oscillators** per voice (thinner than orchestral)
- **Named quartet roles**: Soprano (bright), Alto (warm), Tenor (neutral), Bass (dark)
- **Per-role tonal shaping**: each voice has distinct filter character via `QuartetRoleConfig`
- **Companion planting**: voices that play together develop harmonic affinity (pitch attractor)
- **Intimacy parameter**: controls companion planting strength
- **Traditional quartet panning**: S=left, A=center-left, T=center-right, B=right
- **D006**: Mod wheel → vibrato depth, Aftertouch → filter cutoff
- **Params**: 27 (ADSR, filter, detune, companion, intimacy, brightness, vibrato, growth, bend, 4 macros, 2 LFOs)

### XOxalis (Synth)
- **Phyllotaxis oscillator bank**: 7 partials at golden ratio intervals
- **Phi parameter**: blends between standard harmonic series (0) and phyllotaxis spacing (1)
- **Golden angle phase initialization**: partials start at 137.5 degree intervals
- **Symmetry parameter**: controls asymmetric waveshaping for organic vs mathematical feel
- **Growth Mode specialty**: partials emerge one by one at golden angle intervals
- **D006**: Mod wheel → phi amount, Aftertouch → filter cutoff
- **Params**: 27 (ADSR, filter, phi, spread, symmetry, brightness, vibrato, growth, bend, 4 macros, 2 LFOs)

## Doctrine Compliance

| Doctrine | Status |
|----------|--------|
| D001 Velocity→Timbre | All 4: velocity scales attack time + filter brightness |
| D002 Modulation | All 4: 2 LFOs + mod wheel + aftertouch + 4 macros |
| D003 Physics IS Synthesis | Overgrow: KS string model. Oware references. |
| D004 No Dead Params | All params wired to DSP |
| D005 Breathing | LFO1 floor 0.005 Hz on all engines |
| D006 Expression | Mod wheel + aftertouch wired on all 4 |

## CPU Budget (Decision G2)

All engines: 4 voices (not 8). Estimated per-engine:
- Orchard: ~3.5% (4 oscs × 4 voices + formant filter)
- Overgrow: ~2.8% (KS string + runner + bow noise)
- Osier: ~2.5% (2 oscs × 4 voices + companion planting)
- Oxalis: ~3.2% (7 partials × 4 voices + waveshaping)
- Accumulators/Mycorrhizal: ~0.03% each (block rate)
- Full quad simultaneous: ~12% total

## Color Conflict Resolution

- XOsier: Changed from Sage `#87AE73` to Willow Green `#6B8E23` (avoids XOhm conflict)
- XOxalis: Changed from Fractal Purple `#7B2D8B` to Clover Purple `#6A0DAD` (avoids XOdyssey conflict)

## Files Created

```
Source/DSP/GardenAccumulators.h          — Shared W/A/D accumulators + Mycorrhizal network
Source/Engines/Orchard/OrchardEngine.h   — XOrchard (28 params)
Source/Engines/Orchard/OrchardEngine.cpp — One-line stub
Source/Engines/Overgrow/OvergrowEngine.h — XOvergrow (26 params)
Source/Engines/Overgrow/OvergrowEngine.cpp
Source/Engines/Osier/OsierEngine.h       — XOsier (27 params)
Source/Engines/Osier/OsierEngine.cpp
Source/Engines/Oxalis/OxalisEngine.h     — XOxalis (27 params)
Source/Engines/Oxalis/OxalisEngine.cpp
Docs/concepts/garden-quad-architecture.md — This document
```

## Next Steps

1. Register all 4 engines in `EngineRegistry.h` and `XOceanusProcessor.cpp`
2. Update CLAUDE.md engine tables (4 new entries)
3. Create 10 presets per engine (40 total) across moods
4. Run Seance on each engine
5. Guru Bin retreats for parameter refinement
6. Cross-engine coupling presets (Entangled mood)
