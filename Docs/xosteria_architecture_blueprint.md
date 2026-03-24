# XOsteria — Architecture Blueprint (Phase 1)

**Date:** March 2026
**Status:** Phase 1 — Architecture designed, scaffold built
**Engine location:** `Source/Engines/Osteria/OsteriaEngine.h` (native XOlokun engine, self-contained)
**Companion engine:** XOsprey (shared Shore System)

---

## 1. Architecture Pattern

**Self-contained inline header** — all DSP lives in `OsteriaEngine.h` with embedded helper structs at the top. The engine shares the `ShoreSystem.h` data model with XOsprey but is otherwise self-contained. No external library delegation.

**Estimated header size:** ~1200–1500 lines.

### Helper Struct Hierarchy

```
QuartetRole              — Enum: Bass, Harmony, Melody, Rhythm
QuartetChannel           — Per-quartet-member resonator bank + shore position + memory
ElasticCoupling          — Spring-force model connecting quartet voices
TavernRoom               — FDN reverb modeling tavern acoustics
MurmurGenerator          — Filtered noise for crowd/conversation texture
OsteriaADSR              — Per-voice amplitude envelope (ADSR)
OsteriaLFO               — Per-voice modulation oscillator (5 shapes)
OsteriaVoice             — Voice state: 4 quartet channels, envelope, crossfade
OsteriaEngine            — SynthEngine impl: MIDI, coupling, params, render
```

### Shared Dependencies

```
Source/DSP/ShoreSystem/ShoreSystem.h  — Shore morphing, resonator profiles, tavern data, rhythm data
Source/DSP/CytomicSVF.h               — TPT SVF for formant filtering, character stages
Source/DSP/FastMath.h                  — Fast math utilities
Source/Core/SynthEngine.h              — Engine interface
```

---

## 2. Identity (Locked)

| Field | Value |
|-------|-------|
| Gallery code | OSTERIA |
| Engine ID | `"Osteria"` |
| Accent color | Porto Wine `#722F37` |
| Parameter prefix | `osteria_` |
| Max voices | 8 (polyphonic, oldest-note stealing) |
| Quartet channels per voice | 4 (Bass, Harmony, Melody, Rhythm) |
| Formant filters per channel | 4 (CytomicSVF in bandpass mode) |
| Max simultaneous resonators | 128 (8 voices × 4 channels × 4 formants) |
| CPU budget | <12% single-engine @ 44100Hz, 512 block on M1 |
| DSP location | `Source/Engines/Osteria/OsteriaEngine.h` (inline) |

---

## 3. Static Data — Shore System

The engine uses static data from `ShoreSystem.h`:

### 3.1 Per-Shore Instrument Mapping to Quartet Roles

| Shore | Bass [0] | Harmony [1] | Melody [2] | Rhythm |
|-------|----------|-------------|------------|--------|
| Atlantic | Guitarra Portuguesa | Kora | Uilleann Pipes | Bodhran |
| Nordic | Hardingfele | Langspil | Kulning | Sámi Drum |
| Mediterranean | Bouzouki | Oud | Ney | Darbuka |
| Pacific | Koto | Conch | Singing Bowl | Taiko |
| Southern | Cavaquinho | Valiha | Gamelan | Djembe |

The Bass voice uses instrument [0] formants, Harmony uses [1], Melody uses [2]. Rhythm uses ShoreRhythm pulse data + noise excitation.

### 3.2 Tavern Acoustics

Each shore has a TavernCharacter defining: room size, RT60, absorption, warmth, murmur brightness, reflection density. The tavern shore position (`osteria_tavernShore`) is independent of quartet positions — the room doesn't move with the musicians.

---

## 4. DSP Components

### 4.1 QuartetChannel

Each of the 4 quartet members is a resonator-based voice channel:

**Formant Resonators:** 4 CytomicSVF filters in bandpass mode, tuned to the current shore's instrument formant frequencies. Coefficients update at control rate based on shore position.

**Excitation Model:**
- Bass: fundamental sine + sub-octave partial. Low-pass filtered noise for body.
- Harmony: mid-register partials (3rd, 5th, octave). Paired detuned oscillators for shimmer.
- Melody: upper partials with formant emphasis. Highest brightness.
- Rhythm: noise burst + sharp transient envelope. Pulse rate from ShoreRhythm.

**Shore Position:** Each channel has an independent `shorePos` (0.0–4.0) that determines its timbral character via `decomposeShore()` + `morphResonator()`.

**Timbral Memory:** Circular buffer of 32 recent shore positions. At control rate, the current resonator coefficients are blended with a weighted average of historical positions:
```
effectiveFormant[i] = lerp(currentFormant[i], memoryFormant[i], memoryAmount)
```

### 4.2 ElasticCoupling

Spring-force model connecting the 4 quartet channels:

1. Compute centroid = mean of all 4 shore positions
2. For each channel: `force = springConstant * (centroid - channel.shorePos)`
3. If `|channel.shorePos - centroid| > stretchLimit`: apply nonlinear force increase: `force *= 1.0 + (distance - stretchLimit)²`
4. Apply force as velocity: `channel.shoreVelocity += force * dt`
5. Apply damping: `channel.shoreVelocity *= damping`
6. Integrate: `channel.shorePos += channel.shoreVelocity * dt`
7. Clamp shore position to [0.0, 4.0]

The elastic model creates natural ensemble dynamics — voices that drift apart experience increasing tension, overshoot when snapping back, and settle with inertia.

### 4.3 TavernRoom

Feedback delay network (4 delays) modeling tavern acoustics:

- 4 delay lines with prime-number lengths (scaled by room size from TavernCharacter)
- Householder mixing matrix for energy distribution between delays
- LP filter in feedback path for frequency-dependent absorption
- Warmth control adds low-shelf boost in feedback path
- Pre-allocated fixed-size buffers (max 4096 samples per delay)

### 4.4 MurmurGenerator

Generates tavern background texture:

- White noise source (PRNG)
- 2 formant filters (CytomicSVF bandpass) at ~300Hz and ~2500Hz (speech formant region)
- Formant brightness adjusted by per-shore `murmurBrightness`
- Slow random modulation of formant positions (~0.5Hz) for natural crowd movement
- Output scaled by `osteria_murmur` parameter

### 4.5 Session Delay

Simple delay line with LP filter in feedback path:
- Max 22050 samples (500ms at 44.1kHz)
- Delay time derived from rhythm: `60.0 / (pulseRate * 120)` seconds (rhythmic subdivision)
- Feedback ~0.4, LP filter in feedback for warmth
- Stereo ping-pong option

### 4.6 Chorus

Modulated short delay (5–15ms range):
- Sine LFO modulating delay time
- Rate ~0.5Hz, depth ~3ms
- Creates paired-string shimmer (guitarra portuguesa beating)

---

## 5. Voice Architecture

### 5.1 Note-On Flow

1. Find free voice (or steal oldest via LRU)
2. If stealing: initiate 5ms crossfade on old voice
3. Initialize PRNG with note-dependent seed
4. Compute target frequency from MIDI note
5. For each quartet channel:
   a. Set shore position from corresponding parameter
   b. Decompose shore, morph resonator profile
   c. Configure 4 formant filters from profile:
      - Bass: formants centered around fundamental
      - Harmony: formants centered in mid-register
      - Melody: formants centered in upper register
      - Rhythm: formants from ShoreRhythm
   d. Set pan position based on role and ensWidth
6. Set amp envelope parameters and trigger noteOn
7. Initialize glide coefficient

### 5.2 Render Loop (per sample, per voice)

```
1. Voice stealing crossfade check
2. Glide (portamento) frequency update
3. Amplitude envelope tick
4. If envelope idle → deactivate voice
5. Control-rate update (every controlRateDiv samples):
   a. Update elastic coupling forces between quartet channels
   b. Integrate shore positions (apply forces, damping, clamping)
   c. Re-morph resonator profiles for each channel's current shore
   d. Update formant filter coefficients
   e. Apply timbral memory blending
   f. Record shore position to memory buffer
6. Audio-rate: for each quartet channel (Bass, Harmony, Melody, Rhythm):
   a. Generate excitation:
      - Bass: sine(freq) + 0.5*sine(freq/2) + filtered noise
      - Harmony: sine(freq*1.5) + sine(freq*2) with slight detune
      - Melody: sine(freq*2) + sine(freq*3) + sine(freq*4) with detuning
      - Rhythm: noise * transient envelope at ShoreRhythm pulse rate
   b. Run excitation through 4 formant bandpass filters
   c. Sum formant outputs with formant gains
   d. Apply channel level and pan
   e. Apply sympathy: mix small amount of other channels' output
7. Sum quartet channel outputs
8. DC blocker → soft limiter → ampEnv × velocity × fadeGain
9. Accumulate into stereo mix
```

### 5.3 Voice Stealing

LRU (oldest note) with 5ms crossfade — identical to OceanicEngine pattern.

---

## 6. Coupling Interface

### 6.1 Output (getSampleForCoupling)

| Channel | Returns |
|---------|---------|
| 0 | Left output (post-room) |
| 1 | Right output |
| 2 | Peak envelope level |

### 6.2 Input (applyCouplingInput)

| CouplingType | Effect | Accumulator |
|-------------|--------|-------------|
| AudioToWavetable | External audio shapes quartet character (added to excitation) | `couplingExcitationMod` |
| AmpToFilter | External amplitude modulates elastic tightness | `couplingElasticMod` |
| AudioToFM | External audio excites tavern room model | `couplingRoomExcitation` |
| EnvToMorph | External envelope drives shore drift on all channels | `couplingShoreDrift` |

### 6.3 Coupling Reject List

- `AmpToChoke` — kills all four voices simultaneously (musically destructive)
- `PitchToPitch` — overrides per-voice shore-derived pitch identity

### 6.4 The Diptych — OSPREY × OSTERIA

| Direction | Type | Effect |
|-----------|------|--------|
| OSPREY → OSTERIA | AudioToFM | Ocean turbulence bleeds into tavern room model |
| OSPREY → OSTERIA | AmpToFilter | Sea state modulates ensemble tightness |
| OSTERIA → OSPREY | AudioToFM | Quartet music excites ocean resonators |
| OSTERIA → OSPREY | AmpToFilter | Quartet dynamics modulate wave intensity |

---

## 7. Parameter Namespace (Locked)

All parameters use `osteria_` prefix. Version = 1.

### Quartet Parameters

| Parameter ID | Range | Default | Skew | Description |
|-------------|-------|---------|------|-------------|
| `osteria_qBassShore` | 0.0–4.0 | 0.0 | 1.0 | Bass voice shore position |
| `osteria_qHarmShore` | 0.0–4.0 | 0.0 | 1.0 | Harmony voice shore position |
| `osteria_qMelShore` | 0.0–4.0 | 0.0 | 1.0 | Melody voice shore position |
| `osteria_qRhythmShore` | 0.0–4.0 | 0.0 | 1.0 | Rhythm voice shore position |
| `osteria_qElastic` | 0.0–1.0 | 0.5 | 1.0 | Rubber band strength |
| `osteria_qStretch` | 0.0–1.0 | 0.5 | 1.0 | Max shore distance threshold |
| `osteria_qMemory` | 0.0–1.0 | 0.3 | 1.0 | Cross-pollination persistence |
| `osteria_qSympathy` | 0.0–1.0 | 0.3 | 1.0 | Inter-voice sympathetic resonance |

### Voice Balance

| Parameter ID | Range | Default | Description |
|-------------|-------|---------|-------------|
| `osteria_bassLevel` | 0.0–1.0 | 0.8 | Bass channel level |
| `osteria_harmLevel` | 0.0–1.0 | 0.7 | Harmony channel level |
| `osteria_melLevel` | 0.0–1.0 | 0.7 | Melody channel level |
| `osteria_rhythmLevel` | 0.0–1.0 | 0.6 | Rhythm channel level |
| `osteria_ensWidth` | 0.0–1.0 | 0.5 | Stereo spread |
| `osteria_blendMode` | 0.0–1.0 | 0.0 | Jazz (0) → Unison (1) |

### Tavern Parameters

| Parameter ID | Range | Default | Description |
|-------------|-------|---------|-------------|
| `osteria_tavernMix` | 0.0–1.0 | 0.3 | Room model intensity |
| `osteria_tavernShore` | 0.0–4.0 | 0.0 | Which shore's tavern |
| `osteria_murmur` | 0.0–1.0 | 0.2 | Conversation texture level |
| `osteria_warmth` | 0.0–1.0 | 0.5 | Proximity EQ |
| `osteria_oceanBleed` | 0.0–1.0 | 0.1 | Outside world seepage |

### Character Parameters

| Parameter ID | Range | Default | Description |
|-------------|-------|---------|-------------|
| `osteria_patina` | 0.0–1.0 | 0.2 | Harmonic aging |
| `osteria_porto` | 0.0–1.0 | 0.0 | Warm saturation |
| `osteria_smoke` | 0.0–1.0 | 0.1 | HF haze |

### Envelope Parameters

| Parameter ID | Range | Default | Skew | Description |
|-------------|-------|---------|------|-------------|
| `osteria_attack` | 0.001–4.0 | 0.05 | 0.3 | Attack time |
| `osteria_decay` | 0.05–4.0 | 0.3 | 0.3 | Decay time |
| `osteria_sustain` | 0.0–1.0 | 0.7 | 1.0 | Sustain level |
| `osteria_release` | 0.05–8.0 | 1.0 | 0.3 | Release time |

### FX Parameters

| Parameter ID | Range | Default | Description |
|-------------|-------|---------|-------------|
| `osteria_sessionDelay` | 0.0–1.0 | 0.2 | Delay mix |
| `osteria_hall` | 0.0–1.0 | 0.2 | Reverb size |
| `osteria_chorus` | 0.0–1.0 | 0.1 | Paired-string shimmer |
| `osteria_tape` | 0.0–1.0 | 0.0 | Lo-fi warmth |

### Macro Parameters

| Parameter ID | Range | Default | Label | Controls |
|-------------|-------|---------|-------|----------|
| `osteria_macroCharacter` | 0.0–1.0 | 0.0 | CHARACTER | blendMode + shore convergence |
| `osteria_macroMovement` | 0.0–1.0 | 0.0 | MOVEMENT | elastic(inv) + stretch + drift |
| `osteria_macroCoupling` | 0.0–1.0 | 0.0 | COUPLING | sympathy + memory + coupling |
| `osteria_macroSpace` | 0.0–1.0 | 0.0 | SPACE | tavernMix + hall + oceanBleed |

---

## 8. Macro Implementation

### M1 — CHARACTER

```cpp
float effectiveBlend = clamp(pBlendMode + macroChar * 0.8f, 0.0f, 1.0f);
// At blend=1 (unison): converge all shores toward centroid
float convergence = macroChar;
for each channel:
    targetShore = lerp(channel.targetShore, centroid, convergence);
```

At 0: maximum cultural diversity (voices spread across shores).
At 1: unified ensemble (all voices converge on same shore).

### M2 — MOVEMENT

```cpp
float effectiveElastic = clamp(pElastic - macroMove * 0.7f, 0.0f, 1.0f);  // inverse!
float effectiveStretch = clamp(pStretch + macroMove * 0.4f, 0.0f, 1.0f);
// Plus: increase shore drift rate
```

At 0: tight ensemble locked together.
At 1: voices wander freely, stretching rubber bands.

### M3 — COUPLING

```cpp
float effectiveSympathy = clamp(pSympathy + macroCoup * 0.5f, 0.0f, 1.0f);
float effectiveMemory = clamp(pMemory + macroCoup * 0.5f, 0.0f, 1.0f);
```

At 0: parallel monologues. At 1: deep conversation + heavy memory.

### M4 — SPACE

```cpp
float effectiveTavern = clamp(pTavernMix + macroSpace * 0.6f, 0.0f, 1.0f);
float effectiveHall = clamp(pHall + macroSpace * 0.5f, 0.0f, 1.0f);
float effectiveBleed = clamp(pOceanBleed + macroSpace * 0.5f, 0.0f, 1.0f);
```

At 0: open air. At 1: deep inside the pub.

---

## 9. Memory Allocation

All allocation happens in `prepare()`:
- `outputCacheL`, `outputCacheR`: `std::vector<float>` sized to maxBlockSize
- All voice state, quartet channels, formant filters: fixed-size arrays (no heap)
- TavernRoom delay buffers: fixed-size arrays (compile-time, 4 × 4096 samples)
- Session delay buffers: fixed-size arrays (2 × 22050 samples)
- Chorus buffers: fixed-size arrays (2 × 2048 samples)

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
┌──────────────────────────────────────────────────┐
│  Per Voice (×8)                                   │
│                                                   │
│  Elastic Coupling Engine                         │
│  ├── Compute centroid of 4 shore positions       │
│  ├── Apply spring forces per channel             │
│  ├── Integrate shore velocities                  │
│  └── Clamp positions to [0, 4]                   │
│                                                   │
│  Quartet (4 channels)                            │
│  ┌─────────────────────────────────────────────┐ │
│  │ BASS: sine(f) + sine(f/2) + noise           │ │
│  │ → 4 formant BPFs (shore-morphed)            │ │
│  │ → level × pan(-0.3)                         │ │
│  ├─────────────────────────────────────────────┤ │
│  │ HARMONY: sine(1.5f) + sine(2f) detuned      │ │
│  │ → 4 formant BPFs (shore-morphed)            │ │
│  │ → level × pan(-0.1)                         │ │
│  ├─────────────────────────────────────────────┤ │
│  │ MELODY: sine(2f) + sine(3f) + sine(4f)     │ │
│  │ → 4 formant BPFs (shore-morphed)            │ │
│  │ → level × pan(+0.2)                         │ │
│  ├─────────────────────────────────────────────┤ │
│  │ RHYTHM: noise × transient envelope          │ │
│  │ → 4 formant BPFs (shore-morphed)            │ │
│  │ → level × pan(+0.4)                         │ │
│  └─────────────────────────────────────────────┘ │
│                                                   │
│  Sympathy crossfeed between channels             │
│  Timbral memory blending                         │
│                                                   │
│  Mix → DC Blocker → Soft Limiter                 │
│  → Amp ADSR × Velocity × Crossfade              │
└──────────────────────────────────────────────────┘
    │
    ▼
Voice Sum (stereo)
    │
    ▼
Character Stage
├── Patina (gentle harmonic fold)
├── Porto (fastTanh saturation)
└── Smoke (LP filter HF haze)
    │
    ▼
Tavern Room Model (FDN reverb at tavernMix)
    │
    ▼
Murmur Generator (crowd texture at murmur level)
    │
    ▼
FX Chain
├── Session Delay (rhythmic delay)
├── Hall (allpass reverb)
├── Chorus (modulated delay shimmer)
└── Tape (LP + noise)
    │
    ▼
Output + Coupling Cache
```

---

## 11. Build Integration

### Files

| File | Purpose |
|------|---------|
| `Source/Engines/Osteria/OsteriaEngine.h` | Complete engine DSP (inline) |
| `Source/Engines/Osteria/OsteriaEngine.cpp` | One-line stub + REGISTER_ENGINE |
| `Source/DSP/ShoreSystem/ShoreSystem.h` | Shared shore data model |
| `Docs/concepts/xosteria_concept_brief.md` | Phase 0 concept brief |
| `Docs/xosteria_architecture_blueprint.md` | This document |

### Registration

```cpp
// OsteriaEngine.cpp
#include "OsteriaEngine.h"
REGISTER_ENGINE(OsteriaEngine)
```

---

## 12. The Diptych — Engine Relationship

```
     THE OCEAN                          THE SHORE
   ┌──────────────┐                 ┌──────────────┐
   │   OSPREY     │ ◄────────────► │   OSTERIA    │
   │              │   coupling      │              │
   │ Turbulence   │                 │ The Quartet  │
   │ Creature     │   ocean bleeds  │ The Tavern   │
   │ voices       │   into tavern   │ The Memory   │
   │ Fluid        │                 │ The Rubber   │
   │ dynamics     │   music calms   │ Bands        │
   │              │   the storm     │              │
   │ Azulejo Blue │                 │ Porto Wine   │
   └──────────────┘                 └──────────────┘
   Force. Vastness.                 Warmth. Community.
```

Shared: Shore System (5 coastlines, folk instrument families, creature voices, fluid character, tavern acoustics, rhythm patterns). Same data, different perspectives.

---

## 13. QA Checklist

- [ ] All DSP inline in .h header
- [ ] No allocation in renderBlock()
- [ ] No blocking I/O in renderBlock()
- [ ] Denormal protection on all filter state variables
- [ ] DC blocker on voice output
- [ ] Soft limiter (fastTanh) prevents clipping
- [ ] Voice stealing with 5ms crossfade
- [ ] getSampleForCoupling() is O(1) — returns cached sample
- [ ] Parameter IDs use `osteria_` prefix consistently
- [ ] All 4 macros produce audible change across full range
- [ ] Shore morphing per-channel produces continuous transitions
- [ ] Elastic coupling model is stable (no explosive oscillation)
- [ ] Timbral memory doesn't accumulate denormals
- [ ] Tavern room feedback < 1.0 (no runaway)
- [ ] Quartet channel balance produces musical output at defaults
- [ ] Session delay feedback < 1.0
- [ ] Output normalized — no steady-state clipping at default settings

---

*XO_OX Designs | Engine: OSTERIA | Accent: #722F37 | Prefix: osteria_*
*Companion: OSPREY | Accent: #1B4F8A | Prefix: osprey_*
*Together: The Diptych — ocean and shore, force and warmth, nature and humanity*
