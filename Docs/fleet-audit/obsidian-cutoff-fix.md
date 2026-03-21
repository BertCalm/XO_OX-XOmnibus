# Obsidian filterCutoff Domain Fix — 2026-03-21

## Problem

`obsidian_filterCutoff` had two incompatible domains across 279 Obsidian presets:

- **Correct domain (Hz)**: `NormalisableRange(20.0, 20000.0, 0.1, skew=0.3)` — engine uses Hz directly
- **Wrong domain (normalized)**: 28 presets stored [0,1] float values (e.g. 0.04, 0.35, 0.96)

The engine reads the raw Hz value and passes it directly to its filter calculation:
```cpp
float effectiveCutoff = clamp(paramFilterCutoff + ..., 20.0f, 20000.0f);
```
Normalized values would be clamped to 20 Hz (effectively silenced) since 0.04–0.96 << 20.

## Audit Results

| Category | Count |
|---|---|
| Total Obsidian presets (non-quarantine) | 279 |
| Presets with `obsidian_filterCutoff` present | 182 |
| Presets missing the parameter (engine default 8000 Hz applies) | 97 |
| Presets with correct Hz domain | 154 |
| **Presets with wrong normalized domain (fixed)** | **28** |

## Conversion Formula

JUCE `NormalisableRange` skew conversion (normalized → Hz plain value):

```
hz = 20.0 + (20000.0 - 20.0) * (norm ^ (1.0 / 0.3))
```

## Fixed Presets

All 28 presets updated. Values rounded to 1 decimal place.

| File | Old (norm) | New (Hz) | Mood |
|---|---|---|---|
| Obsidian_Void_Glass.xometa | 0.0400 | 20.4 | Aether |
| Obsidian_Obsidian_Waters.xometa | 0.1400 | 48.5 | Aether |
| Obsidian_Void_Harmonic.xometa | 0.2500 | 216.7 | Aether |
| Obsidian_Veil.xometa | 0.3100 | 422.8 | Atmosphere |
| lava_glass_curtain.xometa | 0.3400 | 568.1 | Entangled |
| lava_strike.xometa | 0.3600 | 683.1 | Entangled |
| dark_crystal_bloom.xometa | 0.3800 | 814.1 | Entangled |
| shard_lattice.xometa | 0.3900 | 885.9 | Entangled |
| Crystal Drone.xometa | 0.4280 | 1200.5 | Entangled |
| Black Mirror Water.xometa | 0.4300 | 1219.0 | Entangled |
| magma_assault.xometa | 0.4600 | 1521.3 | Entangled |
| dark_arm_slash.xometa | 0.4800 | 1750.1 | Entangled |
| obsidian_tendril.xometa | 0.5200 | 2279.1 | Entangled |
| Evolution_Glass_Cathedral.xometa | 0.5500 | 2743.6 | Entangled |
| White Tablecloth.xometa | 0.5510 | 2760.1 | Entangled |
| Obsidian_Resonance_Cascade.xometa | 0.6000 | 3660.0 | Flux |
| volcanic_swarm.xometa | 0.6600 | 5021.2 | Entangled |
| magma_frond.xometa | 0.6800 | 5544.5 | Entangled |
| shard_storm.xometa | 0.7000 | 6104.9 | Entangled |
| Obsidian_Crystal_Infinite.xometa | 0.7000 | 6104.9 | Aether |
| volcanic_veil.xometa | 0.7300 | 7018.5 | Entangled |
| Obsidian_Shattering.xometa | 0.7500 | 7678.3 | Flux |
| obsidian_predator.xometa | 0.7600 | 8024.0 | Entangled |
| Magma_Seam.xometa | 0.7900 | 9126.5 | Foundation |
| Obsidian_Pure_Elevation.xometa | 0.8200 | 10331.2 | Aether |
| Volcanic_Glass.xometa | 0.8700 | 12580.1 | Foundation |
| Obsidian_Glass_Heaven.xometa | 0.9000 | 14082.8 | Aether |
| Dark_Glass_Dome.xometa | 0.9600 | 17458.1 | Atmosphere |

## Verification

Post-fix scan confirmed zero normalized values remain across all 279 Obsidian presets.

## Notes

- 97 presets missing the parameter entirely are fine — JUCE will apply the engine default of **8000 Hz**
- Engine source not modified — only preset JSON files updated
- The skew=0.3 means the normalized scale is heavily log-weighted toward the low end; this is why even 0.04 normalized maps to only 20.4 Hz
- The 28 affected presets were disproportionately in the Entangled mood folder (18/28)
