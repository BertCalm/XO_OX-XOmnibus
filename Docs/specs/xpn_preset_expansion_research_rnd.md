# XPN Preset Expansion R&D Spec
**Date**: 2026-03-16
**Scope**: Research and documentation for converting XOmnibus presets into MPC-native XPN expansions

---

## 1. The MPC Hardware Difference

DAW playback is a controlled environment — precise scheduling, unlimited CPU headroom, sub-millisecond latency management. The plugin does the heavy lifting; the producer adjusts parameters with a mouse.

MPC standalone inverts all of that. Processing budget is finite. Interaction is physical — pads, velocity, Q-Links. Timing comes from the producer's hands, not the DAW grid. This means a preset that sounds spectacular in Logic may feel inert and unresponsive on hardware.

The critical gap: most XOmnibus presets are tuned for DAW listening. They reward careful A/B comparison, parameter exploration, and CPU-hungry real-time modulation. XPN expansions must be tuned for MPC playing. The producer triggers a pad at velocity 82 and expects something musical to happen immediately. If the preset sounds identical at velocity 40 and velocity 110, it fails the MPC test.

---

## 2. Velocity Response Hierarchy

The seance doctrine D001 — velocity maps to timbre, not just volume — is the single most important principle for MPC usability.

A preset that only scales amplitude with velocity is a sample, not an instrument. A preset that opens a filter sweep from 800Hz at velocity 40 to 8kHz at velocity 127 is playable, expressive, and musical in a live session context.

For melodic and bass programs, the target is a continuous timbral arc across the velocity range: closed and dark at ghost velocities (1–40), midrange body at medium velocities (41–80), bright and present at hard velocities (81–110), pushed and saturated at accent velocities (111–127).

For drum programs, the gold standard is 4 discrete timbral layers:
- **Ghost** (1–40): dry, quiet, minimal transient
- **Medium** (41–80): full body, natural decay
- **Hard** (81–110): punchy, transient-forward
- **Accent** (111–127): saturated, possibly distorted, maximum character

Any XPN drum kit that does not have at least 3 timbral layers per voice is incomplete. Volume-only velocity is a fallback, not a feature.

---

## 3. What Translates from XOmnibus to MPC

**Direct translations (high confidence):**
- Single-engine Foundation and Atmosphere presets are the best candidates. They have clear sonic identities, no inter-engine dependencies, and stable parameter states that render consistently.

**Approximate translations (require baking decisions):**
- Entangled presets with live coupling cannot be reproduced dynamically on MPC. The coupled sound must be baked in at a chosen coupling state. Capture the most musically interesting moment of the coupling — usually mid-range coupling depth, not the extremes.
- ORACLE GENDY breakpoints are non-deterministic by design. Pick one compelling GENDY state per export — freeze the breakpoint positions, render, and document what state was captured in the program name.
- OUROBOROS chaos presets: same principle. Find a chaotic attractor state that is musically useful (melodic density, rhythmic periodicity) and freeze it. The export should represent the best five seconds of chaos, not the average.

---

## 4. Engine-Specific Translation Notes

**ONSET**: Direct 1:1 translation. Drum programs export with full velocity layer support. No baking required. The XPN format was designed with ONSET as the reference implementation.

**OPAL**: Export at grain density mid-point (not maximum — polyphony on MPC is constrained). Freeze interesting granular states before export. Each program should represent one specific textural identity, not a generic "granular pad."

**OBLONG**: Bass sweeps are best captured as 4 velocity layers at distinct filter positions: closed sub, mid open, full open, and driven. Label these explicitly. Do not export OBLONG as a static sample — the filter position IS the character.

**OVERWORLD**: Export each ERA (NES / Genesis / SNES) as a separate XPN program. One OVERWORLD preset becomes 3 programs. ERA crossfade cannot be reproduced on MPC — splitting by ERA is not a compromise, it is a feature.

**ORACLE**: Export 2–3 GENDY snapshots as separate programs. Name them by their spectral character (e.g. "ORACLE — Dense Harmonic", "ORACLE — Sparse Flutter") rather than by snapshot number.

**OVERDUB**: Render with effects baked in. The tape delay and spring reverb are core to OVERDUB's identity; they are not optional processing. Dry OVERDUB exports are not representative. Document baked FX in the program name.

**OCEANIC (when applicable)**: 4 velocity layers map to 4 coastline positions — deep water, mid-water, surf, shoreline. Each has a distinct spectral signature.

---

## 5. Pack Density Guidelines

| Engine Type | Programs per Pack | Notes |
|---|---|---|
| Drum (ONSET) | 8–16 kits | Each kit must have distinct character — not genre variations of the same sound |
| Bass (OBLONG) | 12–20 programs | Cover sub, mid, upper bass + playing styles (sustained, plucked, staccato) |
| Melodic/Pad (OPAL) | 16–24 programs | Cover all 4 DNA quadrants; avoid clustering in "safe" textural territory |
| Utility/FX | 8–12 programs | Extreme parameter positions are more useful than safe middle-ground exports |

Packs below these densities feel thin on MPC. Producers cycle through programs quickly during sessions — a 6-program pack is consumed in minutes. The target is enough material to support a full session without repetition.

---

## 6. The "Bake It" Principle

Any XOmnibus feature requiring live coupling, real-time modulation, or CPU-intensive DSP that cannot be reproduced on MPC hardware must be baked into the exported sample.

**Baking options, in order of preference:**
1. Choose a parameter snapshot that represents peak musical value
2. Render with effects chain active (reverb, delay, drive — whatever defines the sound)
3. Create multiple programs at distinct settings rather than one compromise average

When baking, document what was frozen in the program name. Examples:
- `OVERDUB Vessel — BAKED: tape+reverb`
- `ORACLE Drift State 2 — BAKED: GENDY mid-density`
- `OPAL Coastline — BAKED: grain=mid, pitch=+5`

The bake note is metadata for the producer, not an apology. A well-chosen baked state is more useful than an unbaked preset with 40 parameters the MPC cannot replicate.

---

*Refer to [xpn-tools.md](../xpn-tools.md) for export implementation details. Refer to seance doctrine files for D001–D008 velocity and timbral coverage standards.*
