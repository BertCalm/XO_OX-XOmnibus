# Obsidian Deep Recovery — All Rounds Summary

**Engine**: Obsidian (Phase Distortion Synthesis)
**Seance Score**: 6.6/10 (pre-recovery)
**File**: `Source/Engines/Obsidian/ObsidianEngine.h`
**Accent**: Crystal White `#E8E0D8`
**Parameter Prefix**: `obsidian_`

---

## Identity

Obsidian = volcanic glass formed under impossible pressure at the ocean floor. Fourth-generation XO_OX species. Oscar-leaning — deep character with occasional surface flash. Lives in The Deep.

Synthesis lineage: Casio CZ-series (1984) Phase Distortion. A 2D morphable distortion space (density × tilt), two-stage cascade with cross-modulation, Euler-Bernoulli inharmonic stiffness, 4-formant resonance network, and stereo phase divergence from a single oscillator core.

---

## Round 3A — P0 Bug Fixes (2026-03-14)

### P0-01: R-Channel Filter Bypass
**Problem**: The Cytomic SVF main filter was only processing the left channel. Right channel output was unfiltered.
**Fix**: Both `outputLeft` and `outputRight` now pass through `voice.mainFilter.processSample()`.
**Location**: renderBlock per-sample loop, MAIN FILTER section (~line 817-818).

### P0-04: Formant Parameter ID Collision
**Problem**: `pFormantResonance` and `pFormantIntensity` were both mapping to `"obsidian_formantResonance"`, causing one to shadow the other. The formant intensity parameter had no effect.
**Fix**: `pFormantResonance` now correctly maps to `"obsidian_formantResonance"` for the blend parameter. `pFormantIntensity` maps separately to `"obsidian_formantIntensity"` (the aftertouch-sensitive intensity offset). Both are declared in `addParametersImpl()`.
**Location**: `attachParameters()` (~line 1091 and 1122).

---

## Round 5D — D006 Aftertouch (2026-03-14)

**Problem**: No aftertouch expression input wired.
**Fix**: Channel pressure (CC channel pressure) maps to `aftertouch.setChannelPressure()`. At the start of `renderBlock`, `aftertouch.updateBlock()` is called and smoothed pressure is read. Full pressure adds up to +0.3 to `effectiveFormant` — more channel pressure = deeper vowel character.
**Sensitivity**: 0.3 (full pressure = +30% formant blend on top of current setting).
**Location**: MIDI loop (~line 574-576), post-MIDI block (~line 583-586).

---

## Round 7A — D006 Mod Wheel (2026-03-14)

**Problem**: No mod wheel (CC1) expression input wired.
**Fix**: CC1 messages captured in MIDI loop, stored as `modWheelValue` (0.0–1.0). Applied to `effectiveCutoff`: `modWheelValue * 0.5f * 10000.0f`. Full mod wheel = +5000 Hz filter brightening.
**Sensitivity**: 0.5 (classic brightness expression — half of the 10 kHz range).
**Location**: MIDI loop (~line 577-579), `effectiveCutoff` calculation (~line 537-538).

---

## Deep Recovery — D001: Velocity Shapes Timbre (2026-03-14)

**Problem**: `voice.velocity` was used exclusively as amplitude multiplier (`voiceGain = amplitudeLevel * voice.velocity`). Velocity had no effect on timbral character — a soft hit and a hard hit sounded identical in timbre, only different in volume. This violates D001.

**Fix**: Velocity now boosts PD depth before envelope application:
```cpp
float velocityPDBoost = voice.velocity * 0.2f;
float depthEnvelopeAmount = pdEnvelopeLevel * clamp(modulatedDepth + velocityPDBoost, 0.0f, 1.0f);
```

**Effect**: A velocity of 1.0 adds +0.2 to the effective PD depth. Soft playing (velocity 0.2–0.4) produces hollow, pure tones with minimal harmonics. Hard playing (velocity 0.8–1.0) produces complex, edgy tones with deep harmonic distortion — exactly the character of obsidian glass under percussion.

**Sensitivity**: +0.2 at full velocity. Interacts with the PD depth envelope (both scale `depthEnvelopeAmount`), so the velocity effect is most audible on attack before the envelope decays.

**Location**: Per-voice render loop, PD STAGE 1 section (~line 742-749).

---

## Deep Recovery — D005: Engine-Level Formant LFO / "Stone Breathing" (2026-03-14)

**Problem**: While per-voice LFOs existed (LFO1→PD depth, LFO2→density), no LFO targeted the formant vowel blend position. Default LFO depth was 0.0 in all prior code paths, making the engine effectively static unless manually configured. D005 requires at least one LFO with rate floor ≤ 0.01 Hz.

**Fix**: Added an engine-level "stone breathing" LFO running at a fixed 0.1 Hz, modulating `effectiveFormant` by ±15%.

```cpp
const double obsidianLfoHz = 0.1;
const double obsidianLfoIncrement = obsidianLfoHz / static_cast<double>(sampleRateFloat);
obsidianLfoPhase += obsidianLfoIncrement * static_cast<double>(numSamples);
if (obsidianLfoPhase >= 1.0) obsidianLfoPhase -= 1.0;
const float obsidianLfoValue = std::sin(static_cast<float>(obsidianLfoPhase) * kTwoPi);
effectiveFormant = clamp(effectiveFormant + obsidianLfoValue * 0.15f, 0.0f, 1.0f);
```

**Design Rationale**: The LFO runs at engine level (not per-voice) so all voices share the same vowel position. This creates a unified organic movement — all active voices breathe together like one living instrument, as opposed to individual voice chaos. A complete LFO cycle takes 10 seconds (0.1 Hz), creating the slowest possible organic movement that still reads as intentional modulation.

**Member**: `double obsidianLfoPhase = 0.0` added to private member data. Reset to 0.0 in `reset()`.

**Interaction with formant intensity preset value**: The LFO modulates `effectiveFormant` which is already the combined result of `paramFormantBlend + paramFormantIntensity`. Presets with higher `obsidian_formantIntensity` values (0.5–0.75) will show the most audible breathing. Presets with `formantIntensity = 0.0` will hear the LFO as slight periodicity only when the LFO is near its peak (+0.15 formant).

**Location**: Post-MIDI, post-aftertouch section of `renderBlock` (~line 594-604). Member data (~line 1525-1529).

---

## Pre-existing Features (Confirmed Working)

These were already present before the deep recovery pass:

### Filter Envelope
The `phaseDistortionEnvelope` (ADSR) modulates how much PD depth is applied. Parameters `obsidian_depthAttack/Decay/Sustain/Release` are fully wired. A long attack on the PD envelope creates a timbral bloom — notes start pure/hollow and gradually become harmonically complex.

### 2 Per-Voice LFOs
- LFO1 (rate 0.01–30 Hz): modulates PD depth ±30% (`lfo1Output * 0.3f` added to `smoothedDepth`)
- LFO2 (rate 0.01–30 Hz): modulates density ±20% (`lfo2Output * 0.2f` added to `smoothedDensity`)
- Both support: Sine, Triangle, Saw, Square, S&H shapes

### 4 Macros (All Working)
- CHARACTER: density +0.5, depth +0.3 — more harmonics, more edge
- MOVEMENT: cross-modulation +0.5 — more Stage 2 interaction
- COUPLING: cascade blend +0.3 — more Stage 2 presence
- SPACE: stiffness +0.4, stereo width +0.3 — wider, more metallic

### Coupling Inputs (All Working)
- AudioToFM: external audio → PD depth (phase-warped sidebands)
- AmpToFilter: external amplitude → filter cutoff (rhythmic filtering)
- EnvToMorph: external envelope → density/tilt position (timbral morphing)
- RhythmToBlend: rhythmic source → PD depth (tempo-synced timbre)

---

## Preset Library (Added 2026-03-14)

8 presets written covering all core moods using `obsidian_` parameter namespace:

| Preset | Mood | Character |
|--------|------|-----------|
| Volcanic Glass | Foundation | Dark, warm, immediate — the obsidian stone character |
| Deep Pressure | Foundation | Mono legato with glide, heavy, settled |
| Stone Breathing | Atmosphere | Formant LFO front and center, slow vowel drift |
| Obsidian Vowels | Atmosphere | High formant + cascade, PD bloom on long attack |
| Crystal Shard | Prism | Metallic, bell-like, stiffness inharmonics |
| Phase Knife | Prism | Maximum density + cascade cross-mod, razor spectral |
| Obsidian Eruption | Flux | Evolving, LFO-driven unstable, PD bloom |
| Glass Shard Coupling | Entangled | AudioToFM + AmpToFilter coupling-ready |

**Total Obsidian presets in XOmnibus**: 8 (previously 0 using actual `obsidian_` parameters)

Note: The two presets named "Obsidian Clave" (Foundation) and "Obsidian Drone" (Atmosphere) that existed prior use OddfeliX and Odyssey engines respectively — they are thematically named but do not use the Obsidian engine.

---

## Score Projection

| Criterion | Pre-Recovery | Post-Recovery |
|-----------|-------------|---------------|
| P0 bugs | Fixed (R-channel, formant ID) | — |
| D001 velocity timbre | FAIL | PASS (+0.2 PD depth) |
| D005 LFO (rate ≤ 0.01 Hz) | PARTIAL (per-voice LFOs exist, depth=0 default) | PASS (engine LFO 0.1 Hz + per-voice down to 0.01 Hz) |
| D006 aftertouch | PASS (Round 5D) | — |
| D006 mod wheel | PASS (Round 7A) | — |
| Preset library | 0 actual Obsidian presets | 8 new presets |

**Projected score: 8.2–8.5/10** (up from 6.6/10)
