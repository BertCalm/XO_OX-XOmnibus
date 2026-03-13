# XOsprey — Architecture Blueprint (Phase 1)

**Date:** March 2026
**Status:** Phase 1 — Architecture designed, scaffold built
**Engine location:** `Source/Engines/Osprey/OspreyEngine.h` (native XOmnibus engine, self-contained)
**Companion engine:** XOsteria (shared Shore System)

---

## 1. Architecture Pattern

**Self-contained inline header** — all DSP lives in `OspreyEngine.h` with embedded helper structs at the top. The engine shares the `ShoreSystem.h` data model with XOsteria but is otherwise self-contained. No external library delegation.

**Estimated header size:** ~1200–1400 lines.

### Helper Struct Hierarchy

```
ModalResonator           — 2-pole resonant filter modeling a tuned partial
CreatureFormant          — Formant sweep generator for creature voices
FluidEnergyModel         — Perlin-inspired turbulence model (swell + chop + noise)
OspreyADSR               — Per-voice amplitude envelope (ADSR)
OspreyLFO                — Per-voice modulation oscillator (5 shapes)
OspreyVoice              — Voice state: 16 resonators, 3 creature voices, envelopes
AllpassDelay             — Simple allpass delay for harbor verb
OspreyEngine             — SynthEngine impl: MIDI, coupling, params, render
```

### Shared Dependencies

```
Source/DSP/ShoreSystem/ShoreSystem.h  — Shore morphing, resonator profiles, creature data
Source/DSP/CytomicSVF.h               — TPT SVF for tilt filter, character stages
Source/DSP/FastMath.h                  — Fast math utilities
Source/Core/SynthEngine.h              — Engine interface
```

---

## 2. Identity (Locked)

| Field | Value |
|-------|-------|
| Gallery code | OSPREY |
| Engine ID | `"Osprey"` |
| Accent color | Azulejo Blue `#1B4F8A` |
| Parameter prefix | `osprey_` |
| Max voices | 8 (polyphonic, oldest-note stealing) |
| Max resonators per voice | 16 (3 instruments × 4 formants + 4 sympathetic) |
| Max creature voices per voice | 3 (bird, deep, ambient) |
| CPU budget | <10% single-engine @ 44100Hz, 512 block on M1 |
| DSP location | `Source/Engines/Osprey/OspreyEngine.h` (inline) |

---

## 3. Static Data — Shore System

The engine uses static data from `ShoreSystem.h`:

### 3.1 Shore Regions (5)

| Shore | Value | Swell Character | Resonator Family | Creature Voices |
|-------|-------|----------------|-----------------|----------------|
| Atlantic | 0.0 | Long rolling swells | Guitarra, Kora, Uilleann | Storm Petrel, Humpback, Foghorn |
| Nordic | 1.0 | Deep slow waves | Hardingfele, Langspil, Kulning | Arctic Tern, Beluga, Ice Crack |
| Mediterranean | 2.0 | Short choppy seas | Bouzouki, Oud, Ney | Med Gull, Dolphin Click, Cicada |
| Pacific | 3.0 | Vast gentle swells | Koto, Conch, Singing Bowl | Albatross, Pacific Whale, Reef |
| Southern | 4.0 | Warm rolling rhythm | Cavaquinho, Valiha, Gamelan | Tropicbird, Southern Whale, Rain |

### 3.2 Morphing

`osprey_shore` is continuous (0.0–4.0). `decomposeShore()` splits into two adjacent shore indices + interpolation fraction. All resonator profiles, creature voices, and fluid character are interpolated using the morphing utilities in ShoreSystem.h.

---

## 4. DSP Components

### 4.1 ModalResonator

A 2-pole resonant filter (modal synthesis):

```
excitation → [b0 * x - a1 * s1 - a2 * s2] → output
```

- Coefficients derived from: center frequency, bandwidth, gain, sample rate
- Decay rate: `r = exp(-π * bandwidth / sampleRate)`
- Cosine coefficient: `a1 = -2r * cos(2π * freq / sampleRate)`
- Radius squared: `a2 = r²`
- Input gain: `b0 = (1 - r²) * gain`
- Denormal protection on state variables `s1`, `s2`

Per voice: 12 primary resonators (3 instruments × 4 formants from ShoreResonators) + 4 sympathetic resonators driven by cross-resonator energy.

### 4.2 FluidEnergyModel

The core energy generator — models ocean turbulence as an energy source for resonators:

**Low sea state (0.0–0.3):** Smooth sinusoidal swell modulated by shore-specific period and depth.
```
energy = sin(swellPhase) * swellDepth * (1 - seaState)
```

**Medium sea state (0.3–0.7):** Swell + surface chop (higher frequency noise injection).
```
energy = swell + seaState * chopNoise * chopAmount
```

**High sea state (0.7–1.0):** Layered Perlin-style noise — 4 octaves of noise at different frequencies, spectral cascade modeling energy transfer from large to small scales.
```
energy = Σ(noise[octave] * weight[octave]) * turbulenceGain
```

The fluid model runs once per sample globally (not per-voice). Each voice reads the same energy stream but applies it through differently-tuned resonators.

### 4.3 CreatureFormant

Formant sweep generator for creature voices (bird calls, whale songs, ambient textures):

- 3 CytomicSVF filters in bandpass mode, each tracking a formant frequency
- Sweep phase advances at rate derived from creature's sweepMs
- During sweep: formant frequencies interpolate from startFreqs to endFreqs
- During gap: silence, countdown, random re-trigger
- Creature amplitude modulated by `osprey_creatureDepth` and sea state
- Higher sea state increases creature trigger probability

### 4.4 OspreyVoice

Per-MIDI-note state:
- 16 ModalResonators tuned to MIDI note pitch × resonator profile partials
- 3 CreatureFormant generators
- OspreyADSR amplitude envelope
- Glide/portamento state
- DC blocker (1-pole highpass ~5Hz)
- Voice-stealing crossfade (5ms)
- Control-rate decimation counter (~2kHz)
- Per-voice PRNG (LCG: `rng = rng * 1664525 + 1013904223`)

### 4.5 Harbor Verb

Simple allpass delay chain (4 stages):
- Pre-allocated fixed-size delay buffers (up to ~4096 samples)
- Prime-number delay lengths scaled by room size
- Feedback coefficient from harborVerb parameter
- LP filter in feedback path for natural decay

### 4.6 Post-Processing Chain

```
Voice Mix → Tilt Filter (SVF LP/HP) → Foam (softClip overdrive)
  → Brine (bit reduction) → Hull (LP resonance body)
  → Fog (LP rolloff) → Harbor Verb (allpass chain)
  → Output + Coupling Cache
```

---

## 5. Voice Architecture

### 5.1 Note-On Flow

1. Find free voice (or steal oldest via LRU)
2. If stealing: initiate 5ms crossfade on old voice
3. Initialize PRNG with note-dependent seed
4. Compute target frequency from MIDI note
5. Configure 16 resonators:
   - For each of 3 instruments in current shore: 4 formant resonators
   - Resonator freq = MIDI note freq × (formant ratio / base instrument freq)
   - Bandwidth and gain from shore-morphed ResonatorProfile
   - 4 additional sympathetic resonators at harmonic intervals (2nd, 3rd, 5th, octave)
6. Configure creature voices from shore-morphed CreatureVoice data
7. Set amp envelope parameters and trigger noteOn
8. Initialize glide coefficient

### 5.2 Render Loop (per sample, per voice)

```
1. Voice stealing crossfade check
2. Glide (portamento) frequency update
3. Amplitude envelope tick
4. If envelope idle → deactivate voice
5. Control-rate update (every controlRateDiv samples):
   a. Update resonator coefficients (shore morphing + pitch tracking)
   b. Update creature formant targets
   c. Apply coherence (scale resonator freq scatter)
6. Audio-rate:
   a. Read fluid energy value (global)
   b. Excite each resonator with: fluidEnergy * resonatorGain * coherenceModulation
   c. Sum resonator outputs with per-resonator pan scatter
   d. Process creature formant generators
   e. Mix creatures into resonator output at creatureDepth level
   f. DC blocker
   g. Soft limiter (fastTanh)
   h. Apply ampEnvelope * velocity * fadeGain
7. Accumulate into stereo mix
```

### 5.3 Voice Stealing

LRU (oldest note) with 5ms crossfade — identical to OceanicEngine pattern.

---

## 6. Coupling Interface

### 6.1 Output (getSampleForCoupling)

| Channel | Returns |
|---------|---------|
| 0 | Left output (post-processing, pre-reverb) |
| 1 | Right output |
| 2 | Peak envelope level |

### 6.2 Input (applyCouplingInput)

| CouplingType | Effect | Accumulator |
|-------------|--------|-------------|
| AudioToFM | External audio adds to resonator excitation energy | `couplingExcitationMod` |
| AmpToFilter | External amplitude modulates effective sea state | `couplingSeaStateMod` |
| EnvToMorph | External envelope modulates swell period | `couplingSwellMod` |
| LFOToPitch | External LFO modulates resonator tuning offset | `couplingPitchMod` |
| AudioToWavetable | External audio replaces fluid excitation source | `couplingExternalExcitation` flag |

### 6.3 Coupling Reject List

- `AmpToChoke` — the ocean doesn't stop
- `PitchToPitch` — resonator tuning is MIDI-controlled; external pitch creates mud

---

## 7. Parameter Namespace (Locked)

All parameters use `osprey_` prefix. Version = 1.

### Core Parameters

| Parameter ID | Range | Default | Skew | Description |
|-------------|-------|---------|------|-------------|
| `osprey_shore` | 0.0–4.0 | 0.0 | 1.0 | Coastline region selector (continuous) |
| `osprey_seaState` | 0.0–1.0 | 0.2 | 1.0 | Calm → storm master control |
| `osprey_swellPeriod` | 0.5–30.0 | 8.0 | 0.3 | Wave rhythm in seconds |
| `osprey_windDir` | 0.0–1.0 | 0.5 | 1.0 | Spectral tilt bias |
| `osprey_depth` | 0.0–1.0 | 0.3 | 1.0 | Surface vs. subsurface energy |
| `osprey_resonatorBright` | 0.0–1.0 | 0.5 | 1.0 | High partial emphasis |
| `osprey_resonatorDecay` | 0.01–8.0 | 1.0 | 0.3 | Resonator ring time |
| `osprey_sympathyAmount` | 0.0–1.0 | 0.3 | 1.0 | Cross-resonator coupling |
| `osprey_creatureRate` | 0.0–1.0 | 0.2 | 1.0 | Creature trigger rate |
| `osprey_creatureDepth` | 0.0–1.0 | 0.3 | 1.0 | Creature voice intensity |
| `osprey_coherence` | 0.0–1.0 | 0.7 | 1.0 | Partial correlation |

### Character Parameters

| Parameter ID | Range | Default | Skew | Description |
|-------------|-------|---------|------|-------------|
| `osprey_foam` | 0.0–1.0 | 0.0 | 1.0 | HF saturation (whitecap) |
| `osprey_brine` | 0.0–1.0 | 0.0 | 1.0 | Bit reduction (salt crystal) |
| `osprey_hull` | 0.0–1.0 | 0.2 | 1.0 | Body resonance (the boat) |
| `osprey_filterTilt` | 0.0–1.0 | 0.5 | 1.0 | Dark (0) → Bright (1) |
| `osprey_harborVerb` | 0.0–1.0 | 0.2 | 1.0 | Reverb mix |
| `osprey_fog` | 0.0–1.0 | 0.1 | 1.0 | HF rolloff + smear |

### Envelope Parameters

| Parameter ID | Range | Default | Skew | Description |
|-------------|-------|---------|------|-------------|
| `osprey_ampAttack` | 0.0–8.0 | 0.5 | 0.3 | Attack time |
| `osprey_ampDecay` | 0.05–8.0 | 1.0 | 0.3 | Decay time |
| `osprey_ampSustain` | 0.0–1.0 | 0.7 | 1.0 | Sustain level |
| `osprey_ampRelease` | 0.05–12.0 | 2.0 | 0.3 | Release time |

### Macro Parameters

| Parameter ID | Range | Default | Label | Controls |
|-------------|-------|---------|-------|----------|
| `osprey_macroCharacter` | 0.0–1.0 | 0.0 | SEA STATE | seaState + coherence(inv) + foam |
| `osprey_macroMovement` | 0.0–1.0 | 0.0 | MOVEMENT | swellPeriod + creatureRate + sympathy |
| `osprey_macroCoupling` | 0.0–1.0 | 0.0 | COUPLING | depth + creatureDepth + coupling amount |
| `osprey_macroSpace` | 0.0–1.0 | 0.0 | SPACE | harborVerb + fog + tideDelay |

---

## 8. Macro Implementation

### M1 — SEA STATE (Character)

```cpp
float effectiveSeaState = clamp(pSeaState + macroChar * 0.8f, 0.0f, 1.0f);
float effectiveCoherence = clamp(pCoherence - macroChar * 0.5f, 0.0f, 1.0f);
float effectiveFoam = clamp(pFoam + macroChar * 0.4f, 0.0f, 1.0f);
```

At 0: glassy calm harbor, locked harmonics, no foam.
At 1: full storm, decorrelated partials, spray and whitecap.

### M2 — MOVEMENT

```cpp
float effectiveSwellPeriod = lerp(pSwellPeriod, pSwellPeriod * 0.3f, macroMove);
float effectiveCreatureRate = clamp(pCreatureRate + macroMove * 0.5f, 0.0f, 1.0f);
float effectiveSympathy = clamp(pSympathy + macroMove * 0.4f, 0.0f, 1.0f);
```

At 0: barely breathing. At 1: churning, crying, ringing.

### M3 — COUPLING

```cpp
float effectiveDepth = clamp(pDepth + macroCoup * 0.5f, 0.0f, 1.0f);
float effectiveCreatureDepth = clamp(pCreatureDepth + macroCoup * 0.5f, 0.0f, 1.0f);
```

What's happening beneath the surface.

### M4 — SPACE

```cpp
float effectiveVerb = clamp(pHarborVerb + macroSpace * 0.6f, 0.0f, 1.0f);
float effectiveFog = clamp(pFog + macroSpace * 0.5f, 0.0f, 1.0f);
```

Close vs. vast open ocean.

---

## 9. Memory Allocation

All allocation happens in `prepare()`:
- `outputCacheL`, `outputCacheR`: `std::vector<float>` sized to maxBlockSize
- All voice state, resonators, creature formants: fixed-size arrays (no heap)
- FluidEnergyModel: inline state (no heap)
- AllpassDelay buffers: fixed-size arrays (compile-time)

**renderBlock() allocates zero bytes.**

---

## 10. Signal Chain Diagram

```
MIDI Input
    │
    ▼
Voice Allocation (8-voice poly, LRU stealing, 5ms crossfade)
    │
    ▼
┌─────────────────────────────────────────────────┐
│  Per Voice (×8)                                  │
│                                                  │
│  Shore Morph → Resonator Profiles               │
│  Shore Morph → Creature Voice Data              │
│  Shore Morph → Fluid Character                  │
│                                                  │
│  FluidEnergyModel (global) → energy             │
│                                                  │
│  Resonator Bank (16)                            │
│  ├── Instrument 1: 4 formant resonators         │
│  ├── Instrument 2: 4 formant resonators         │
│  ├── Instrument 3: 4 formant resonators         │
│  └── Sympathetic: 4 resonators                  │
│  └── Excited by: fluid energy × coherence       │
│                                                  │
│  Creature Voices (3)                            │
│  ├── Bird/Tern formant sweep                    │
│  ├── Whale/Deep formant arc                     │
│  └── Ambient texture                            │
│                                                  │
│  Mix → DC Blocker → Soft Limiter                │
│  → Amp ADSR × Velocity × Crossfade             │
└─────────────────────────────────────────────────┘
    │
    ▼
Voice Sum (stereo)
    │
    ▼
Tilt Filter (CytomicSVF LP/HP sweep)
    │
    ▼
Character Stage
├── Foam (softClip with HF emphasis)
├── Brine (bit reduction)
└── Hull (LP resonance body)
    │
    ▼
FX Chain
├── Fog (gentle LP rolloff)
└── Harbor Verb (4-stage allpass)
    │
    ▼
Output + Coupling Cache
```

---

## 11. Build Integration

### Files

| File | Purpose |
|------|---------|
| `Source/Engines/Osprey/OspreyEngine.h` | Complete engine DSP (inline) |
| `Source/Engines/Osprey/OspreyEngine.cpp` | One-line stub + REGISTER_ENGINE |
| `Source/DSP/ShoreSystem/ShoreSystem.h` | Shared shore data model |
| `Docs/concepts/xosprey_concept_brief.md` | Phase 0 concept brief |
| `Docs/xosprey_architecture_blueprint.md` | This document |

### Registration

```cpp
// OspreyEngine.cpp
#include "OspreyEngine.h"
REGISTER_ENGINE(OspreyEngine)
```

This registers the factory lambda in `EngineRegistry::instance()`. The engine is available for slot assignment immediately.

---

## 12. QA Checklist

- [ ] All DSP inline in .h header
- [ ] No allocation in renderBlock()
- [ ] No blocking I/O in renderBlock()
- [ ] Denormal protection on all filter state variables
- [ ] DC blocker on voice output
- [ ] Soft limiter (fastTanh) prevents clipping
- [ ] Voice stealing with 5ms crossfade
- [ ] getSampleForCoupling() is O(1) — returns cached sample
- [ ] Parameter IDs use `osprey_` prefix consistently
- [ ] All 4 macros produce audible change across full range
- [ ] Shore morphing produces continuous, artifact-free transitions
- [ ] Creature voices don't click (envelope on trigger/release)
- [ ] FluidEnergyModel is deterministic for same PRNG seed
- [ ] Harbor verb feedback < 1.0 (no runaway)
- [ ] Output normalized — no steady-state clipping at default settings

---

*XO_OX Designs | Engine: OSPREY | Accent: #1B4F8A | Prefix: osprey_*
