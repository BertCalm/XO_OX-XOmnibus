# Preset Expansion Round 10h

**Date**: 2026-03-14
**Scope**: 20 new presets — inaugural OSTERIA and OSPREY sets
**Status**: Complete

---

## Summary

Round 9G expanded Oracle, Overworld, OCELOT, and Optic to ≥10 presets each. This round targeted the four engines called out for inspection:

| Engine     | Before | After | Added |
|------------|--------|-------|-------|
| Organon    | 152    | 152   | 0 — already above threshold |
| Ouroboros  | 50+    | 50+   | 0 — already above threshold |
| Osteria    | 0      | 10    | +10 inaugural presets |
| Osprey     | 0      | 10    | +10 inaugural presets |

**20 new presets total.** OSTERIA and OSPREY were the last two engines with zero XOlokun-native presets. That gap is now closed.

---

## Engine Context

Both OSTERIA and OSPREY share the **ShoreSystem** (Blessing B012) — a constexpr cultural data model encoding five coastal regions (Atlantic, Nordic, Mediterranean, Pacific, Southern) as instrument spectral fingerprints. Neither engine owns the data; both read the same coordinate space.

- **OSPREY** (Azulejo Blue `#1B4F8A`): Surface-zone turbulence-modulated resonator synthesis. The ocean heard from above.
- **OSTERIA** (Porto Wine `#722F37`): Ensemble synthesis with elastic coupling and timbral memory. The human answer to the ocean — the tavern where the fisherman goes after the sea.

---

## Osteria Presets (10 new)

All presets use the `osteria_` parameter prefix with the `"Osteria"` engine key.
Macro labels: CHARACTER, MOVEMENT, COUPLING, SPACE.

| Preset Name | File | Mood | Shore | Macro Values | DNA Highlights |
|-------------|------|------|-------|-------------|----------------|
| Tavern Oak | `Osteria_Tavern_Oak.xometa` | Foundation | Atlantic (0.2) | None | warmth 0.8, low movement 0.25 |
| Guitarra Portuguesa | `Osteria_Guitarra_Portuguesa.xometa` | Foundation | Atlantic (0.0–0.1) | None | plucked: sustain 0.0, bright 0.65 |
| Mediterranean Dusk | `Osteria_Mediterranean_Dusk.xometa` | Atmosphere | Mediterranean (2.0) | Char 0.4, Space 0.35 | warmth 0.7, space 0.6, Oud/Ney voices |
| Nordic Fjord Song | `Osteria_Nordic_Fjord_Song.xometa` | Atmosphere | Nordic (1.0) | Space 0.55 | memory 0.75, space 0.8, sparse |
| Rembetika Fire | `Osteria_Rembetika_Fire.xometa` | Prism | Mediterranean (2.0) | Char 0.65, Mov 0.4 | porto 0.75, aggression 0.6 |
| Gamelan Shard | `Osteria_Gamelan_Shard.xometa` | Prism | Southern (4.0) | Char 0.5, Mov 0.7 | elastic 0.85, inharmonic, bright 0.8 |
| Kora Metabolism | `Osteria_Kora_Metabolism.xometa` | Flux | Atlantic (0.0–0.3) | Char 0.3, Mov 0.6, Space 0.2 | sympathy 0.65, movement 0.7 |
| Pacific Argument | `Osteria_Pacific_Argument.xometa` | Flux | Pacific (3.0) | Mov 0.65, Space 0.15 | elastic 0.9, stretch 0.9, tension |
| Shore Dialogue | `Osteria_Shore_Dialogue.xometa` | Entangled | Atlantic (0.2) | Char 0.3, Coup 0.35, Space 0.25 | OSTERIA+OSPREY pairing, Amp→Filter |
| Last Table | `Osteria_Last_Table.xometa` | Aether | Mediterranean→Nordic blend | Space 0.7 | memory 0.9, release 5.0s, space 0.85 |

**Macro usage**: 8 of 10 presets have at least one non-zero macro value.

**Shore coverage**: Atlantic (3 presets), Mediterranean (3), Nordic (1), Pacific (2), Southern (1) — all five coastlines represented.

### Osteria DNA Distribution

| DNA Axis | Min | Max | Notes |
|----------|-----|-----|-------|
| brightness | 0.25 | 0.80 | Plucked presets push high; memory presets pull low |
| warmth | 0.30 | 0.80 | Atlantic and Mediterranean dominate warm end |
| movement | 0.15 | 0.75 | Foundation anchors low; Flux presets push high |
| density | 0.25 | 0.75 | Tavern presets dense; Aether/Nordic sparse |
| space | 0.10 | 0.85 | Prism presets tight; Aether/Nordic wide |
| aggression | 0.03 | 0.60 | Rembetika Fire and Gamelan Shard are the high end |

---

## Osprey Presets (10 new)

All presets use the `osprey_` parameter prefix with the `"Osprey"` engine key.
Macro labels: CHARACTER, MOVEMENT, COUPLING, SPACE.

| Preset Name | File | Mood | Shore | Sea State | Macro Values | DNA Highlights |
|-------------|------|------|-------|-----------|-------------|----------------|
| Atlantic Ground | `Osprey_Atlantic_Ground.xometa` | Foundation | Atlantic (0.0) | 0.15 calm | None | calm anchor, long swell 14s |
| Harbor Mouth | `Osprey_Harbor_Mouth.xometa` | Foundation | Mediterranean (2.0) | 0.10 sheltered | None | hull 0.7, harbor verb 0.55 |
| Salt Mist | `Osprey_Salt_Mist.xometa` | Atmosphere | Nordic (1.0) | 0.25 light | Space 0.4 | fog 0.55, brine 0.3, long decay |
| Pacific Night Dive | `Osprey_Pacific_Night_Dive.xometa` | Atmosphere | Pacific (3.0) | 0.20 | Space 0.5 | depth 0.7, creature rate 0.25 |
| Storm Petrel | `Osprey_Storm_Petrel.xometa` | Prism | Atlantic (0.0) | 0.85 storm | Char 0.6, Mov 0.7 | foam 0.6, coherence 0.3, aggression 0.75 |
| Shallow Turbulence | `Osprey_Shallow_Turbulence.xometa` | Prism | Southern (4.0) | 0.70 | Char 0.7, Mov 0.6 | depth 0.15, sympathy 0.8, aggression 0.8 |
| Tidal Friction | `Osprey_Tidal_Friction.xometa` | Flux | Mediterranean (2.0) | 0.55 active | Char 0.35, Mov 0.55 | creature 0.6 rate, short swell 6s |
| Wind Change | `Osprey_Wind_Change.xometa` | Flux | Atlantic blend (0.5) | 0.45 | Char 0.2, Mov 0.6 | windDir 0.9, sympathy 0.55 |
| Whale Song Coupling | `Osprey_Whale_Song_Coupling.xometa` | Entangled | Pacific (3.0) | 0.20 | Coup 0.5, Space 0.4 | OSPREY+Odyssey pairing, creature depth 0.65 |
| Open Water Dissolve | `Osprey_Open_Water_Dissolve.xometa` | Aether | Mediterranean blend (2.5) | 0.08 near-calm | Space 0.75 | depth 0.9, decay 7s, release 9s, space 0.95 |

**Macro usage**: 8 of 10 presets have at least one non-zero macro value.

**Sea state range**: 0.08 (near-still) to 0.85 (gale force) — full turbulence spectrum covered.

### Osprey DNA Distribution

| DNA Axis | Min | Max | Notes |
|----------|-----|-----|-------|
| brightness | 0.25 | 0.85 | Prism presets push to 0.85; Aether dissolves to 0.25 |
| warmth | 0.25 | 0.60 | Ocean engine skews cooler than Osteria by design |
| movement | 0.10 | 0.85 | Aether near-still; Storm/Shallow at max |
| density | 0.25 | 0.65 | Prism dense; Aether and Foundation sparse |
| space | 0.10 | 0.95 | Prism tight (storm = no reverb); Open Water at 0.95 |
| aggression | 0.02 | 0.80 | Gentlest ocean patch (0.02) to full gale (0.80) |

---

## Mood Coverage

| Engine  | Foundation | Atmosphere | Prism | Flux | Entangled | Aether |
|---------|-----------|-----------|-------|------|-----------|--------|
| Osteria | 2         | 2         | 2     | 2    | 1         | 1      |
| Osprey  | 2         | 2         | 2     | 2    | 1         | 1      |

Both engines now have representation in all six mood categories.

---

## Coupling Presets

Two multi-engine presets introduced:

1. **Shore Dialogue** (`Osteria_Shore_Dialogue.xometa`) — OSTERIA + OSPREY
   - The canonical ShoreSystem pairing: both engines on Atlantic shore 0.2
   - OSPREY's amplitude modulates OSTERIA's filter (Amp→Filter, 0.35)
   - oceanBleed 0.35 — tavern window left open

2. **Whale Song Coupling** (`Osprey_Whale_Song_Coupling.xometa`) — OSPREY + Odyssey
   - OSPREY Pacific deep-creature voice drives Odyssey pad filter
   - Coupling: Amp→Filter, 0.50 — strong biological envelope shapes the pad
   - Seance Blessing B012 (ShoreSystem) meeting Odyssey's interstellar character

---

## Fleet Status After Round 10h

| Engine    | Solo Presets | Total (incl. multi-engine) |
|-----------|-------------|---------------------------|
| Organon   | 119         | 152 |
| Ouroboros | 50          | 72  |
| Osteria   | 9           | 10  |
| Osprey    | 9           | 10  |

**Zero engines with 0 presets in the XOlokun fleet.** All 26 roadmap engines with implemented DSP now have at least one XOlokun-native preset.

---

*Round 10h complete. Round 10 (sonic DNA corner coverage) pending.*
