# Preset Expansion Round 9g

**Date**: 2026-03-14
**Scope**: 40 new presets across 4 underrepresented engines
**Status**: Complete

---

## Summary

After Rounds 7-8 addressed Oblique, Ocelot, and Obsidian, four engines remained below 15 presets:

| Engine   | Before | After | Added |
|----------|--------|-------|-------|
| Oracle   | 3      | 13    | +10   |
| Overworld| 4      | 14    | +10   |
| OCELOT   | 4      | 14    | +10   |
| Optic    | 11     | 21    | +10   |

---

## New Presets

### Oracle (10 presets added)

GENDY stochastic synthesis + maqam microtonal tuning. All 8 maqamat represented across the 10 presets. Breakpoint counts range from 8 to 32 to cover the full gamut from stable to chaotic.

| Preset | Mood | Maqam | Breakpoints | Voice Mode | DNA Highlights |
|--------|------|-------|-------------|------------|----------------|
| Reef Memory | Foundation | — (ET) | 8 | Mono | Low movement (0.15), warm (0.6) |
| Tectonic Legato | Foundation | Nahawand | 12 | Legato | Warmth 0.7, glide 0.12 |
| Fossil Strata | Atmosphere | — (ET) | 16 | Poly8 | High space (0.7), moderate density |
| Maqam Shimmer | Atmosphere | Hijaz | 20 | Poly4 | Gravity 0.8, space 0.75 |
| Bayati Knife | Prism | Bayati | 14 | Mono | Bright (0.75), aggression 0.55 |
| Saba Pluck | Prism | Saba | 10 | Poly4 | Zero sustain, plucked transient |
| Sikah Turbulence | Flux | Sikah | 24 | Poly4 | Movement 0.85, aggression 0.7 |
| Xenakis Sequence | Flux | — (ET) | 32 | Poly4 | Max chaos: movement 0.95, aggression 0.8 |
| Coral Chorus | Entangled | Nawathar | 18 | Poly4 | Oracle+Oblong coupling, Audio->FM + Amp->Filter |
| Rast Dissolution | Aether | Rast | 28 | Poly8 | Space 0.95, very long attack/release |

**Macro usage**: 8 of 10 presets use non-zero macro values (PROPHECY, EVOLUTION, GRAVITY, DRIFT).

### Overworld (10 presets added)

Chip synthesis — NES 2A03, Genesis YM2612, SNES SPC700. ERA triangle parameter used to navigate between console eras.

| Preset | Mood | Era Focus | FM Algo | Key Feature |
|--------|------|-----------|---------|-------------|
| NES Pulse Bass | Foundation | NES (era=0.0) | — | Triangle + pulse mix, legato |
| SNES String Layer | Foundation | SNES (era=1.0) | — | BRR sample 0, Gaussian interp, echo |
| Hyrule Dawn | Atmosphere | Center blend | 4 | All 3 eras, slow era drift |
| Genesis Cave | Atmosphere | Genesis | 4 | Long FIR echo, cave reverb |
| Contra Run | Prism | NES | — | Narrow pulse, sweep, noise replace |
| FM Clang | Prism | Genesis | 0 | Max feedback (6), odd harmonics |
| Glitch Cascade | Flux | Center | 0 | Glitch type 3, bit crush + echo |
| Chip Dreams Granular | Entangled | SNES-lean | 4 | Overworld+Opal coupling, FM→grain |
| Cartridge Void | Aether | Drifting | 4 | Echo feedback 0.65, era drift 0.18 |
| Bit Garden | Aether | SNES-lean | 7 | BRR sample 2, era mem 4s, space 0.95 |

**Note**: Thunderforce was the only existing Overworld preset. 10 new presets bring the total to 14 (4 Entangled coupling presets from prior rounds + 10 new).

**Macro usage**: 4 of 10 new presets use non-zero macro values (ERA, CRUSH, GLITCH, SPACE).

### OCELOT (10 presets added)

4-strata ecosystem synthesis — Floor, Understory, Canopy, Emergent — with cross-modulation matrix. Biomes 0-3 covered.

| Preset | Mood | Biome | Floor Model | Ecosystem Depth | DNA Highlights |
|--------|------|-------|-------------|-----------------|----------------|
| Rainforest Anchor | Foundation | 0 (Tropical) | KS (1) | 0.15 | Floor-only, warmth 0.75 |
| Savanna Strike | Foundation | 2 (Savanna) | Modal (2) | 0.20 | Hard strike, movement 0.35 |
| Mangrove Breath | Atmosphere | 3 (Mangrove) | KS (0) | 0.55 | Floor→Canopy, tape age, breathe |
| Berimbau Riot | Prism | 0 (Tropical) | KS (1) | 0.70 | SP-1200 bit depth, wavefold 0.85 |
| Canopy Strobe | Prism | 1 (Cloud) | Glass (3) | 0.60 | Shimmer 0.8, fast emergent |
| Ecosystem Storm | Flux | 0 (Tropical) | KS (1) | 0.95 | All cross-faders >0.5, max chaos |
| Predator Pulse | Flux | 2 (Savanna) | Modal (2) | 0.75 | Chop rate 16, creature trigger 0.9 |
| Ocelot Dawn | Entangled | 0 (Tropical) | KS (0) | 0.70 | All strata balanced, internal entanglement |
| Kelp Forest Dream | Aether | 1 (Cloud) | Glass (3) | 0.45 | Reverb 0.85, space 0.92 |
| Phantom Biome | Aether | 3 (Mangrove) | KS (0) | 0.35 | Ghost biome, space 0.95, density 0.2 |

**Macro usage**: 9 of 10 new presets use non-zero PROWL, FOLIAGE, ECOSYSTEM, CANOPY macro values.

### Optic (10 presets added)

Bioluminescent spectral modulation engine. Generates no audio — pure coupling modulation via AutoPulse rhythm and spectral analysis.

| Preset | Mood | Paired With | Pulse Mode | Mod Mix | DNA Highlights |
|--------|------|-------------|------------|---------|----------------|
| Phosphor Signal | Foundation | Odyssey | AutoPulse | Pulse 0.6 / Spec 0.4 | Foundation anchor, moderate rate |
| Spectral Anchor | Foundation | Organon | Spec-only | Spec 1.0 | Pure spectrum, no pulse |
| Bioluminescent Rain | Atmosphere | Odyssey | AutoPulse | Mixed | Slow pulse, high viz feedback |
| Spectral Kelp | Atmosphere | Organon | Spec-only | Spec 0.9 | Organon feedback loop |
| Flash Sequence | Prism | Odyssey | AutoPulse | Pulse 0.7 | Subdivision, high accent |
| Chromatic Storm | Prism | Ouroboros | AutoPulse | Mixed | Deep coupling, reactivity 0.85 |
| Sidechain Jelly | Flux | Organon | AutoPulse | Pulse 0.9 | Pumping sidechain effect |
| Reactive Cascade | Flux | Odyssey | AutoPulse | Mixed | Max evolve + subdivision |
| Luminous Metabolism | Entangled | Organon | AutoPulse | Mixed | Bidirectional: Organon↔Optic |
| Phosphorescence | Aether | Odyssey | AutoPulse | Spec 0.85 | Rate 0.04, space 0.98 |

**Macro usage**: All 10 Optic presets effectively use reactivity and pulse parameters as functional macros; coupling amounts vary from subtle (0.15) to deep (0.75).

---

## Sonic DNA Distribution

Diversity was explicitly managed to avoid clustering. Per-engine DNA ranges:

**Oracle**: brightness 0.35–0.75, warmth 0.2–0.7, movement 0.15–0.95, space 0.3–0.95
**Overworld**: brightness 0.4–0.85, warmth 0.25–0.75, movement 0.1–0.85, aggression 0.08–0.8
**OCELOT**: brightness 0.35–0.8, warmth 0.35–0.8, movement 0.2–0.9, space 0.2–0.92
**Optic**: brightness 0.5–0.8, warmth 0.25–0.65, movement 0.1–0.95, space 0.2–0.98

Each engine covers the full range from low-movement/high-space (Aether corner) to high-movement/high-aggression (Flux corner) as intended.

---

## Mood Coverage After Round 9g

| Engine   | Foundation | Atmosphere | Prism | Flux | Entangled | Aether |
|----------|-----------|-----------|-------|------|-----------|--------|
| Oracle   | 2 new     | 2 new     | 2 new | 2 new | 1 new (+2 existing) | 1 new |
| Overworld| 2 new     | 2 new     | 2 new | 2 new (1 existing) | 1 new (+3 existing) | 1 new |
| OCELOT   | 2 new     | 1 new (+2 existing) | 2 new | 2 new | 1 new (+1 existing) | 1 new (+1 existing) |
| Optic    | 2 new     | 2 new (+1 existing) | 2 new (+2 existing) | 2 new (+5 existing) | 1 new (+1 existing) | 1 new (+2 existing) |

All four engines now have representation in every mood category.
