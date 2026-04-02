# Preset Naming Elevation — Fleet-Wide

**Date:** March 2026
**Scope:** All XOceanus preset categories (Aether, Atmosphere, Foundation, Prism, Flux, Entangled)
**Tool:** `Tools/rename_weak_presets.py`

---

## Summary

74 preset names were elevated from functional/generic to evocative/poetic across 8 engines.

| Engine    | Names Elevated |
|-----------|---------------|
| Odyssey   | 36            |
| Oblong    | 14            |
| OddOscar  | 7             |
| Obese     | 5             |
| Overbite  | 6             |
| OddfeliX  | 3             |
| Overdub   | 2             |
| Overworld | 1             |
| **Total** | **74**        |

---

## What Was Changed

### Criteria for elevation

A name was flagged for elevation if it matched one of three weak patterns:

1. **Purely descriptive** — names that state the synth technique with no imagery: `Warm Pad`, `Ambient Bass`, `Glide Lead`, `FM Razor`, `Noise Bed`, `Cosmic Drone`
2. **Engine_Descriptor format** — functional compound names from early preset generation: `Bob_Slow_Pad`, `Drift_Warm_Pad`, `Morph_Resonant_Pad`, `Fat_Wide_Pad`
3. **No emotional or atmospheric content** — names that communicate nothing except the patch type

No file was renamed. Only the `"name"` field inside each `.xometa` JSON was updated. Filenames remain stable for reference integrity.

---

## 10 Best Before → After Transformations

| Before | After | Engine | Rationale |
|--------|-------|--------|-----------|
| Warm Pad | **Moog Shoreline** | OddfeliX | The familiar coast — Moog ladder warmth where the ocean meets land |
| Fat Bass | **Hadal Pressure** | Obese | The weight of the hadal zone pressing upward from maximum depth |
| Noise Bed | **Static Plankton** | Overbite | Sustained crackle noise — the ocean's background radiation, alive |
| Arp Lead | **Sequential Phosphorescence** | Odyssey | Each arpeggiated note lights up and fades like bioluminescence triggered in sequence |
| Cosmic Drone | **Infinite Delay Horizon** | Overdub | A dub drone in vast reverb — the echo arriving from beyond the event horizon |
| Morph_Resonant_Pad | **Eigenfrequency** | OddOscar | The frequency at which a thing vibrates alone — self-resonance as identity |
| Bob_Vapor_Pad | **Evaporation Zone** | Oblong | Sound evaporating at the water's surface — vaporous, dusty, almost gone |
| Portamento Glide | **Cetacean Arc** | Odyssey | The long glide of a whale's descent through water — portamento as animal movement |
| Detune Bass | **Thermocline Beating** | Odyssey | Detuned oscillators creating beats — the interference pattern where warm meets cold water |
| Morph_Full_Scan | **Tidal Traverse** | OddOscar | Fast bloom, mid-morph — actively scanning the full tidal cycle of wavetable states |

---

## Naming Vocabulary Established Per Engine

### Oblong (Bob) — Warm, Fuzzy, Analog
Identity: analog character, warmth, reef textures, temperate water

**Vocabulary:** Sargasso, Reef, Coral, Hearthside, Temperate Zone, Tide Margin, Evaporation Zone, Two Currents, Pressure Burst, Convergence

| Concept | Example |
|---------|---------|
| Warmth / home | Hearthside Current |
| Suspended mass | Sargasso Drift |
| Presence without intrusion | Tide Margin |
| Colony / polyphony | Reef Chorus |
| Convergence of tones | Two Currents Meeting |

---

### Odyssey (Drift) — Psychedelic, Shimmer, Familiar↔Alien
Identity: journey, boundary layers, bioluminescence, the water column as spectrum

**Vocabulary:** Thermocline, Littoral, Haze, Voyager, Pelagic, Phosphorescence, Upwelling, Neritic, Cetacean, Tidal Breathing, Benthic

| Concept | Example |
|---------|---------|
| Warm shimmer | Haze Thermocline |
| Bioluminescent arpeggio | Sequential Phosphorescence |
| Whale-like portamento | Cetacean Arc |
| PWM breathing | PWM Tide / Tidal Compression |
| Supersaw width | Superposed Horizon / Upwelling Stack |
| Sub-bass floor | Origin Point / Benthic Presence |
| Gliding bass | Pelagic Glide |

---

### Obese (Fat) — Massive, Abyssal, Maximum Density
Identity: depth, pressure, weight, hadal zone, sub-bass cosmology

**Vocabulary:** Hadal, Abyssal, Midnight Zone, Mesopelagic, Vent, Subterranean, Trench, Pressure Front

| Concept | Example |
|---------|---------|
| Maximum bass density | Hadal Pressure |
| Sub-only aether | Midnight Zone |
| Wide pad | Abyssal Horizon |
| Thick warm pad | Mesopelagic Bloom |
| Short impact | Vent Pulse |

---

### OddOscar (Morph) — Liminal, Transitional, Wavetable Scanning
Identity: the space between states, membrane, threshold, scanning

**Vocabulary:** Transitional, Eigenfrequency, Tidal Traverse, Littoral Migration, Dissolution, Spectral Crest, Membrane, Threshold

| Concept | Example |
|---------|---------|
| Wavetable scanning | Tidal Traverse |
| Spectral peak | Spectral Crest |
| Portamento migration | Littoral Migration |
| Self-resonance | Eigenfrequency |
| Short spectral pluck | Dissolution Strike |
| Pad at low morph | Transitional Membrane |

---

### OddfeliX — Moog Warmth, feliX Identity, Classic Character
Identity: the familiar shoreline, Moog ladder, metal bell, string resonance

**Vocabulary:** Moog, Shoreline, Karplus, Operator, Metallic Tide, Bell

| Concept | Example |
|---------|---------|
| Classic warm pad | Moog Shoreline |
| FM bell tone | Operator Bell |
| Acoustic string pluck | Karplus Resonance |

---

### Overbite — Bite, Crunch, Predator, Noise
Identity: feral energy, compression, hunting, benthic predator

**Vocabulary:** Predator, Benthic Snarl, Static Plankton, Notch Grotto, Triangle Abyss, Carrier Overtones

| Concept | Example |
|---------|---------|
| Noise pad | Static Plankton |
| Hollow notch pad | Notch Filter Grotto |
| Saw bass growl | Benthic Snarl |
| Triangle sub | Triangle Abyss |
| Biting lead | Predator Signal |
| FM overtones | Carrier Overtones |

---

### Overdub — Echo, Pressure Drop, Dub Space
Identity: reverb vastness, echo horizon, pressure drop, spatial delay

**Vocabulary:** Infinite Delay, Horizon, Dub Glissando, Pressure Drop

| Concept | Example |
|---------|---------|
| Long dub drone | Infinite Delay Horizon |
| Gliding dub lead | Dub Glissando |

---

### Overworld — Chip, Retro, Inharmonic
Identity: NES/SNES pixel sound, ring modulation, metallic partials

**Vocabulary:** Inharmonic, Shimmer, Pixel, Ring, Metallic

| Concept | Example |
|---------|---------|
| Ring mod lead | Inharmonic Shimmer |

---

## Naming Principles Applied

1. **Aquatic/oceanic zone references** — hadal, mesopelagic, benthic, littoral, pelagic, thermocline, neritic shelf, abyssal plain, tide margin, Sargasso

2. **Physical phenomena** — thermocline beating (interference), laminar flow (legato), tidal compression (PWM), upwelling (supersaw rising)

3. **Bioluminescence/living ocean** — static plankton, sequential phosphorescence, trigger particle, carrier overtones

4. **Mythological/historical** — Nereid's Bassline (ocean nymph), Sargasso Drift (mythologized gyre), Cetacean Arc (whale glide)

5. **Emotional/atmospheric states** — Evaporation Zone (vaporous), Dissolution Strike (crystallization then fade), Eigenfrequency (identity as resonance), Infinite Delay Horizon (vertigo/vastness)

6. **Engine-matched character** — Fat = depth/pressure vocabulary; Bob = warm/reef vocabulary; Drift = tidal/bioluminescent vocabulary; Morph = liminal/membrane vocabulary; Overbite = predator/crunch vocabulary

---

## Files Not Touched

The following categories had no weak names and were left entirely unchanged:
- All Drift/Climax presets (01_Singularity_Bloom etc.) — already evocative
- Flux, Entangled, Aether (non-generic names) — strong naming throughout
- Onset presets — technical coupling names intentional
- Multi-engine combination presets (Bob_Drift_Atmosphere etc.) — functional index names for cross-reference

---

## Tooling

The rename script is reusable for future audits:

```bash
# Preview without changes
python3 Tools/rename_weak_presets.py --dry-run

# Apply all elevations
python3 Tools/rename_weak_presets.py

# Filter to one engine
python3 Tools/rename_weak_presets.py --engine Odyssey --dry-run
```

To extend: add entries to the `NAME_MAP` dict in `Tools/rename_weak_presets.py`.
