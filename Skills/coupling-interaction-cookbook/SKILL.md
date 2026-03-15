# Skill: /coupling-interaction-cookbook

**Invoke with:** `/coupling-interaction-cookbook`
**Status:** LIVE
**Purpose:** Quick-reference guide to the best engine pairings in XOmnibus — proven routes, coupling type recommendations, and character descriptions for each pair.

---

## How to Use This Cookbook

1. **By source engine:** Find your rhythmic/tonal generator in the Source column. See what it drives best.
2. **By sound goal:** Use the "Sound Goal" quick-find table to jump to the right pair.
3. **By coupling type:** Use the CouplingType index to find all engines that support a given type.

For detailed preset construction, use the `/coupling-preset-designer` skill.
For coupling implementation status, see `Docs/coupling_audit.md`.

---

## Quick-Find by Sound Goal

| I Want… | Use This Pair | Coupling Type |
|---------|--------------|---------------|
| Drum that pumps a bass | ONSET → OVERBITE | `AmpToFilter` + `AudioToFM` |
| Pad that evolves with the beat | ONSET → OPAL | `RhythmToBlend` + `AmpToFilter` |
| Granular filter sweep | OPAL → OVERDUB | `AmpToFilter` (inverted duck) |
| Echo that responds to grains | OPAL → OVERDUB | `RhythmToBlend` |
| Psychedelic pad atomized into time | ODYSSEY → OPAL | `EnvToMorph` + `AmpToFilter` |
| Chip music in a time machine | OVERWORLD → OPAL | `AudioToWavetable` + `EnvToMorph` |
| Prophecy feeds a living organism | ORACLE → ORGANON | `AudioToFM` + `EnvToMorph` |
| Chaos drums the beat | OUROBOROS → ONSET | `AmpToFilter` + `EnvToDecay` + `RhythmToBlend` |
| Strange attractor as pad | OUROBOROS → ODYSSEY | `AmpToFilter` + `LFOToPitch` |
| Ring-modulated transient lead | ODDFELIX → ORBITAL | `AudioToRing` |
| Spectral chain injection | OBSCURA → any | `AudioToFM` (chain force) |
| Visual engine drives all | OPTIC → any | All types (analysis-only source) |
| Bass envelope gates a texture | OVERBITE → OVERDUB | `AmpToFilter` (ducking) |
| Warm pad brightens with velocity | ODYSSEY → OBLONG | `PitchToPitch` + `AmpToFilter` |
| Dub delay chases the LFO | ORBITAL → OVERDUB | `LFOToPitch` |
| Organ breathes like an organism | ORGANON → ODDOSCAR | `EnvToMorph` |

---

## Tier 1 — Score 5 Engines (Best Coupling Engines)

These engines have the deepest, most complete coupling implementations. Always try to include at least one score-5 engine in a coupling preset.

### ORBITAL (Score 5, FULL input + PROPER output)
- **Sends:** ch0/ch1 audio, ch2 envelope
- **Receives:** `AudioToWavetable`, `AudioToFM`, `AudioToRing`, `AmpToFilter`, `EnvToMorph`, `LFOToPitch`, `PitchToPitch`, `EnvToDecay`, `RhythmToBlend` — all 9 types
- **Best as source:** Melodic FM/wavetable content that other engines can ring-mod against
- **Best as target:** Anything that benefits from audio-rate modulation (FM, ring mod, wavetable injection)
- **Killer pair:** ODDFELIX → ORBITAL (`AudioToRing`) — neon tetra transient ring-mods the orbital partials

### OPTIC (Score 5, FULL input + PROPER multi-channel output)
- **Sends:** ch0/ch1 audio, ch2 composite envelope, ch3–10 individual modulation channels
- **Receives:** All types including `FilterToFilter` (sends to OPTIC's spectrum analyzer)
- **Special:** Visual-only engine — its output is analysis/visualization data, not audio. Best used as a modulation hub.
- **Killer pair:** Any engine → OPTIC (feeds visualizer), OPTIC → any (multi-channel mod)

### ORGANON (Score 5, FULL input + PROPER output)
- **Sends:** ch0/ch1 audio, ch2 envelope (metabolic)
- **Receives:** `AudioToFM` + `AudioToWavetable` (per-voice ingestion buffer), `RhythmToBlend`, `EnvToDecay`, `AmpToFilter`, `EnvToMorph`, `LFOToPitch`, `PitchToPitch`
- **Special:** Per-voice ingestion means source audio literally feeds each voice's metabolic cycle
- **Killer pair:** ORACLE → ORGANON — stochastic prophecy feeds the metabolic organism

### OUROBOROS (Score 5, FULL input + PROPER+ output)
- **Sends:** ch0/ch1 audio, ch2 attractor velocity X (dx/dt), ch3 attractor velocity Y (dy/dt)
- **Receives:** `AudioToFM` (dx/dt perturbation), `AudioToWavetable` (dy/dt perturbation), `RhythmToBlend`, `EnvToDecay`, `AmpToFilter`, `EnvToMorph`, `LFOToPitch`, `PitchToPitch`
- **Special:** ch2 and ch3 are derivative signals — faster-changing, more "nervous" than amplitude envelopes
- **Killer pair:** OUROBOROS → ONSET — strange attractor dynamics modulate percussion (chaos becomes rhythm)

### OSPREY (Score 5, FULL input + PROPER output)
- **Sends:** ch0/ch1 audio, ch2 peak envelope
- **Receives:** `AudioToFM` (sea state excitation), `AmpToFilter` (sea state), `EnvToMorph` (swell period), `LFOToPitch` (resonator tuning), `AudioToWavetable` (excitation replacement with RMS)
- **Killer pair:** ORGANON → OSPREY — organism metabolism modulates shore excitation

---

## Tier 2 — Score 4 Engines (Excellent)

### ONSET (Score 4, PARTIAL input)
- **Sends:** Per-sample audio buffer (unique — full audio, not just ch0/ch1)
- **Receives:** `AmpToFilter`, `EnvToDecay`, `RhythmToBlend`, `AmpToChoke`
- **Killer pairs:** → OVERBITE (rhythm drives bass), → OPAL (rhythm drives density)

### OBSCURA (Score 4, PARTIAL input)
- **Sends:** ch0/ch1 audio, ch2 envelope
- **Receives:** `AudioToFM` (chain force injection), `AmpToFilter` (stiffness), `RhythmToBlend` (impulse trigger)
- **Special:** Physical modeling chain can be *forced* by audio input — deeply physical

### OBSIDIAN (Score 4, PARTIAL input)
- **Sends:** ch0/ch1 audio, ch2 envelope
- **Receives:** `AudioToFM` (PD depth), `AmpToFilter` (cutoff), `EnvToMorph` (density+tilt), `RhythmToBlend` (PD depth)

### OCEANIC (Score 4, PARTIAL input)
- **Sends:** ch0/ch1 audio, ch2 envelope
- **Receives:** `AudioToFM` (particle velocity), `AmpToFilter` (cohesion), `RhythmToBlend` (murmuration cascade)

### ORACLE (Score 4, PARTIAL input)
- **Sends:** ch0/ch1 audio, ch2 envelope
- **Receives:** `AudioToFM` (breakpoint perturbation), `AmpToFilter` (barrier mod), `EnvToMorph` (distribution morph)
- **Killer pair:** → ORGANON (chaos feeds metabolism)

### ORIGAMI (Score 4, PARTIAL input)
- **Sends:** ch0/ch1 audio, ch2 envelope
- **Receives:** `AudioToWavetable` (source replace), `AmpToFilter` (fold depth), `EnvToMorph` (fold point), `RhythmToBlend` (freeze trigger)

### OSTERIA (Score 4, PARTIAL input)
- **Sends:** ch0/ch1 audio, ch2 envelope
- **Receives:** `AudioToWavetable` (quartet excitation), `AmpToFilter` (elastic tightness), `AudioToFM` (room excitation), `EnvToMorph` (shore drift)

---

## Tier 3 — Score 3 Engines (Solid)

All handle at least 3 coupling types. All output PROPER multi-channel data.

| Engine | Key Input Types | Key Output | Best Role |
|--------|----------------|------------|-----------|
| OVERBITE (BITE) | `AmpToFilter`, `AudioToFM`, `EnvToDecay` | ch2 envelope | Target for percussion |
| OBLONG (BOB) | `AmpToFilter`, `AmpToPitch`, `LFOToPitch`, `PitchToPitch` | ch2 envelope | Warm harmonic target |
| ODYSSEY (DRIFT) | `AmpToFilter`, `AmpToPitch`, `LFOToPitch`, `PitchToPitch`, `EnvToMorph` | ch2 envelope | Psychedelic morph target |
| OVERDUB (DUB) | `AmpToFilter` (inverted), `AmpToPitch`, `LFOToPitch`, `PitchToPitch` | ch2 envelope | Duck/swell target |
| OBESE (FAT) | `AmpToFilter`, `AmpToPitch`, `LFOToPitch`, `PitchToPitch` | ch2 envelope | Heavy sampler target |
| ODDOSCAR (MORPH) | `AmpToFilter` (inverted), `EnvToMorph` | ch2 LFO (0.3 Hz) | Wavetable morph target |
| OVERWORLD | `AmpToFilter`, `EnvToMorph` (→ERA X), `AudioToFM` (→ERA Y) | PROPER | ERA crossfade target |
| OPAL | `AudioToWavetable`, `AmpToFilter`, `EnvToMorph`, `LFOToPitch`, `RhythmToBlend`, `EnvToDecay` | SCALAR | Granular morph target |
| OBLIQUE | `AmpToFilter`, `AmpToPitch`, `LFOToPitch`, `PitchToPitch`, `EnvToDecay→filter`, `RhythmToBlend→filter` | ch2 envelope | Prismatic target |

---

## Tier 4 — Score 2 (Limited Input Types)

### ODDFELIX (SNAP) (Score 2)
- **Receives:** `AmpToPitch`, `LFOToPitch`, `PitchToPitch` only — pitch-related types only; no filter or morph reception
- **Sends:** ch2 envelope — sharp transients make excellent coupling SOURCES
- **Best role:** Source engine (driver), not target. Its score reflects limited *input* coupling types, not its value as a modulation source — ODDFELIX's transients are among the sharpest in the fleet.

---

## Score 1 — STUB Engines (Do Not Use as Targets)

| Engine | Why STUB | Can Be Source? |
|--------|----------|---------------|
| OCELOT | All coupling input void-cast | Yes (ch0/ch1/envelope output works) |
| OWLFISH | All coupling input void-cast | Yes (ch0/ch1/envelope output works) |

---

## The Proven Mythological Pairs

These 6 pairs from `Docs/coupling_preset_library.md` are field-tested and narrative-coherent:

| Pair | Type(s) | Mythology |
|------|---------|-----------|
| ONSET → OVERBITE | `AmpToFilter` + `AudioToFM` + `EnvToDecay` | Drum teaches predator how to move |
| OPAL → OVERDUB | `AmpToFilter` + `RhythmToBlend` | Grains dissolving into echo |
| ODYSSEY → OPAL | `EnvToMorph` + `AmpToFilter` + `LFOToPitch` | Familiar→Alien journey atomized |
| OVERWORLD → OPAL | `EnvToMorph` + `AmpToFilter` + `AudioToFM` | Console dissolved into time |
| ORACLE → ORGANON | `AudioToFM` + `AmpToFilter` + `EnvToMorph` | Organism eats the prophet's words |
| OUROBOROS → ONSET | `AmpToFilter` + `EnvToDecay` + `RhythmToBlend` | Strange attractor becomes drum |

---

## Coupling Type → Compatible Target Index

| CouplingType | Compatible Targets (confirmed implemented) |
|--------------|-------------------------------------------|
| `AmpToFilter` | OVERBITE, OBLONG, ODYSSEY, OVERDUB, OBESE, ODDOSCAR, OVERWORLD, OPAL, OBSIDIAN, OCEANIC, ORBITAL, OBLIQUE, OBSCURA, ORACLE, ORGANON, ORIGAMI, OSPREY, OSTERIA |
| `AmpToPitch` | OBLONG, ODYSSEY, OVERDUB, OBESE, ODDFELIX, ORBITAL |
| `LFOToPitch` | OBLONG, ODYSSEY, OVERDUB, OBESE, ODDFELIX, OPAL, ORBITAL, OBLIQUE, ORGANON, OSPREY |
| `EnvToMorph` | ODDOSCAR, ODYSSEY, OPAL, OVERWORLD (ERA X), OBSIDIAN, ORACLE, ORGANON, ORIGAMI, OSPREY, OSTERIA |
| `AudioToFM` | ORBITAL, OUROBOROS, OBSCURA, OBSIDIAN, OCEANIC, ORACLE, ORGANON, ORIGAMI, OSPREY, OSTERIA |
| `AudioToRing` | ORBITAL, OPTIC |
| `FilterToFilter` | OPTIC (analysis input only) |
| `AmpToChoke` | ONSET |
| `RhythmToBlend` | ONSET, OPAL, OBLIQUE, OBSCURA, OCEANIC, ORGANON, ORIGAMI, OUROBOROS, OSPREY |
| `EnvToDecay` | ONSET, OPAL, OBLIQUE, ORGANON, OUROBOROS |
| `PitchToPitch` | OBLONG, ODYSSEY, OVERDUB, OBESE, ODDFELIX, OPAL, ORBITAL, OBLIQUE, ORGANON |
| `AudioToWavetable` | OPAL, ORBITAL, OVERWORLD (ERA Y via AudioToFM), ORIGAMI, ORGANON, OSPREY |

---

## Common Mistakes to Avoid

1. **Two STUB engines** — OCELOT and OWLFISH cannot be targets. No coupling will occur.
2. **Single coupling type for audio-rate sources** — `AudioToFM` needs a strong source (ORACLE, OUROBOROS, OBSCURA). Weak sources produce no audible FM.
3. **Coupling amount too low** — Below 0.1, most couplings are inaudible. Start at 0.25 for testing.
4. **Forgetting M3 macro** — Every Entangled preset must wire M3 (COUPLING) to the coupling amount.
5. **OPAL as scalar source** — OPAL's `getSampleForCoupling` returns SCALAR (not per-sample buffer). Use for envelope-rate modulation only, not audio-rate FM.
6. **OVERDUB coupling direction** — OVERDUB handles `AmpToFilter` as an *inverted* duck (source amplitude = duck the filter). Unexpected if you want a boost.
