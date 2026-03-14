# D006 Mod Wheel Fixes

**Date:** 2026-03-14
**Doctrine:** D006 — Expression Input Is Not Optional
**Round:** Prism Sweep 6F/6G (follows aftertouch round 5D)

## Summary

The D006 expression audit (seance round) found only 2 of 23 engines responded to mod wheel (CC1).
Round 5D wired aftertouch (channel pressure) into 5 engines. This round adds CC1 mod wheel support
to 8 engines — 7 newly wired and 1 confirmed pre-existing.

## Before / After

| Engine | Before | After (mod wheel destination) | Sensitivity |
|--------|--------|-------------------------------|-------------|
| **Snap** (OddfeliX) | No CC1 response | BPF resonance (+ring/peak) | 0.4 |
| **Orbital** | No CC1 response | Spectral morph drift rate (faster LFO) | 0.3 |
| **Obsidian** | No CC1 response | Filter cutoff brightening (+10kHz range) | 0.5 |
| **Origami** | No CC1 response | STFT fold depth (+more spectral processing) | 0.3 |
| **Oracle** | No CC1 response | Maqam gravity (stronger scale attraction) | 0.4 |
| **Oblique** | No CC1 response | Prism color spread (+more spectral color) | 0.3 |
| **Fat** (XObese) | No CC1 response | Mojo analog axis boost | 0.5 |
| **Morph** (OddOscar) | **Already wired** — morph position sweep | (unchanged; maps 0–127 → 0–3.0 morph offset) | — |

## Implementation Details

### Pattern

Each engine follows the same pattern:

1. Member variable: `float modWheelValue = 0.0f;` added alongside the `PolyAftertouch aftertouch` member.
2. MIDI loop capture: `else if (message.isController() && message.getControllerNumber() == 1) modWheelValue = message.getControllerValue() / 127.0f;` added after the `isChannelPressure()` handler.
3. DSP application: `modWheelValue` applied to the target parameter at block-rate (alongside existing macro and coupling modulation).

### Per-Engine Details

#### Snap — BPF Resonance
- File: `Source/Engines/Snap/SnapEngine.h`
- Application: `const float modWheelResonance = std::min(1.0f, effectiveResonance + modWheelValue * 0.4f);`
- Passed to both `highPassFilter` and `bandPassFilter` `setCoefficients()` calls.
- Effect: More ring and peak character in feliX's signature BPF stack. Extreme wheel = near self-oscillation on the BPF.

#### Orbital — Spectral Morph Drift Rate
- File: `Source/Engines/Orbital/OrbitalEngine.h`
- Application: `const double spectralDriftRate = 0.03 + modWheelValue * 0.3;`
- Replaces the hardcoded 0.03 Hz D005 LFO rate in the `spectralDriftPhase` increment.
- Effect: At full wheel, spectral drift runs at 0.33 Hz (11× faster), creating animated harmonic movement between partial profiles A and B.

#### Obsidian — Filter Cutoff
- File: `Source/Engines/Obsidian/ObsidianEngine.h`
- Application: added `+ modWheelValue * 0.5f * 10000.0f` to the `effectiveCutoff` computation.
- Effect: Classic filter brightening expression — full wheel opens the filter up to +5kHz above the base cutoff. Works alongside coupling and macro filter offsets.

#### Origami — STFT Fold Depth
- File: `Source/Engines/Origami/OrigamiEngine.h`
- Application: `effectiveFoldDepth = clamp(effectiveFoldDepth + atPressure * 0.3f + modWheelValue * 0.3f, 0.0f, 1.0f);`
- Combined with existing aftertouch fold depth modulation. Both add up to 0.3.
- Effect: More spectral folding and shimmer in the STFT processor. Full wheel + full pressure = fold depth pushed 0.6 above the base value (clamped to 1.0).

#### Oracle — Maqam Gravity
- File: `Source/Engines/Oracle/OracleEngine.h`
- Application: `effectiveGravity = clamp(effectiveGravity + modWheelValue * 0.4f, 0.0f, 1.0f);`
- Applied after the MIDI loop (post-aftertouch processing), modifying the already-computed `effectiveGravity`.
- Effect: Wheel pulls pitches progressively into the selected maqam scale. With gravity=0 on the panel and a maqam selected, the wheel becomes a real-time "scale intensity" control — from pure 12-TET to full maqam tuning.

#### Oblique — Prism Color Spread
- File: `Source/Engines/Oblique/ObliqueEngine.h`
- Application: `prismParams.colorSpread = std::min(1.0f, prismColorSpread + modWheelValue * 0.3f);`
- Applied per-block in the post-voice FX chain when constructing `ObliquePrism::Params`.
- Effect: More spectral color and shimmer in Oblique's prismatic delay core. Full wheel spreads the 6 delay facets further across the frequency spectrum.

#### Fat (XObese) — Mojo Analog Axis
- File: `Source/Engines/Fat/FatEngine.h`
- Application: `const float effectiveMojo = clamp(mojo + atPressure * 0.3f + modWheelValue * 0.5f, 0.0f, 1.0f);`
- Combined with existing aftertouch Mojo modulation (aftertouch=0.3, wheel=0.5, can sum to +0.8).
- CC1 capture added to both the arpeggiator MIDI path and the direct MIDI path.
- Effect: Classic Moog-style expression — wheel increases analog drift and soft-clip saturation on all 12 oscillators simultaneously. This is Blessing B015 (Mojo Control) made fully expressive.

#### Morph (OddOscar) — Already Wired
- File: `Source/Engines/Morph/MorphEngine.h`
- Pre-existing: `modWheelMorphOffset = static_cast<float>(controllerValue) / 127.0f * 3.0f;`
- Maps wheel to morph position sweep (0–3.0 range), which sweeps through the scan buffer.
- No changes made. Documented here for completeness.

## Files Modified

```
Source/Engines/Snap/SnapEngine.h      — CC1 → BPF resonance (+0.4)
Source/Engines/Orbital/OrbitalEngine.h — CC1 → spectral drift rate (0.03→0.33 Hz)
Source/Engines/Obsidian/ObsidianEngine.h — CC1 → filter cutoff (+5kHz at full)
Source/Engines/Origami/OrigamiEngine.h — CC1 → fold depth (+0.3)
Source/Engines/Oracle/OracleEngine.h   — CC1 → maqam gravity (+0.4)
Source/Engines/Oblique/ObliqueEngine.h — CC1 → prism color spread (+0.3)
Source/Engines/Fat/FatEngine.h         — CC1 → mojo analog boost (+0.5)
```

## D006 Expression Coverage After This Round

| Engine | Velocity→Timbre | Aftertouch | Mod Wheel |
|--------|----------------|------------|-----------|
| Snap | Yes (D001) | Yes (round 5D) | **Yes (this round)** |
| Orbital | Yes (D001) | Yes (round 5D) | **Yes (this round)** |
| Obsidian | Yes (D001) | Yes (round 5D) | **Yes (this round)** |
| Origami | Yes (D001) | Yes (round 5D) | **Yes (this round)** |
| Oracle | Yes (D001) | Yes (round 5D) | **Yes (this round)** |
| Morph | Yes (D001) | No | Yes (pre-existing) |
| Oblique | Yes (D001) | No | **Yes (this round)** |
| Fat | Yes (D001) | Yes (pre-existing) | **Yes (this round)** |

All 8 engines now satisfy D006 for mod wheel.
Remaining engines without mod wheel: all others (Osprey, Osteria, Ouroboros, Organon, Oceanic, Ocelot, Optic, Overworld, Owlfish, etc.). These can be addressed in a subsequent sweep.
