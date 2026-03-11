# XOscillum — Phase 1 Architecture Specification

**Engine:** XOscillum (Psychoacoustic Residue Pitch Synthesis)
**Short Name:** OSCIL
**Engine ID:** `"Oscillum"`
**Accent Color:** TBD (must not conflict with existing 12 engine colors)
**Max Voices:** 16
**CPU Budget:** <12% single-engine, <28% dual-engine
**Date:** 2026-03-11
**Status:** DRAFT — Phase 1 architecture for review

---

## 1. Product Identity

**Thesis:** "XOscillum is a psychoacoustic synth that makes you hear bass notes it never plays, by computing precise upper partials that force your brain to hallucinate the missing fundamental."

**Sound family:** Pad / Bass (phantom) / Texture / Experimental

**Unique capability:** Sound that exploits the listener's auditory neurology. The engine generates no fundamental frequency — only carefully calculated upper partials that trigger the brain's "missing fundamental" perception. Additionally, a Tartini Intermodulator stage generates physical combination tones inside the listener's inner ear, cementing an illusion that exists partially inside the listener's body.

**Personality in 3 words:** Ghostly, cerebral, illusory.

**Gallery gap filled:** No existing engine does additive synthesis targeted at psychoacoustic perception. All current engines generate the frequencies they intend you to hear. XOscillum generates frequencies designed to make you hear something else entirely.

**Historical lineage:**
- **Kawai K5000 (1996)** — Controlled 64 individual harmonics per voice. XOscillum uses additive power targeted at perceptual manipulation rather than spectral control.
- **Max Mathews & Bell Labs (1950s)** — Computer psychoacoustics research. Bypassing acoustic physics to directly stimulate the ear.
- **Georg von Békésy** — Nobel Prize research on cochlear mechanics. The physical basis for Tartini difference tones.

---

## 2. Signal Flow Architecture

```
┌──────────────────────────────────────────────────────────────┐
│                    XOscillum Voice                             │
│                                                               │
│  ┌──────────────────┐                                         │
│  │  MIDI Note In     │                                        │
│  │  (phantom target) │                                        │
│  └────────┬─────────┘                                         │
│           │                                                   │
│           ▼                                                   │
│  ┌──────────────────────────────────────┐                     │
│  │  RESIDUE PITCH SOLVER (control rate)  │                    │
│  │                                      │                     │
│  │  Target: f_target (from MIDI note)   │                     │
│  │  Compute partial frequencies:        │                     │
│  │    f_n = n * f_target                │                     │
│  │    n ∈ {2, 3, 4, ...} (skip fund.)  │                     │
│  │  Apply Odd/Even skew                 │                     │
│  │  Apply Inharmonicity stretch         │                     │
│  │  Compute formant amplitude envelope  │                     │
│  │  Update rate: ~94 Hz (per block)     │                     │
│  └────────┬─────────────────────────────┘                     │
│           │                                                   │
│           ▼                                                   │
│  ┌──────────────────────────────────────┐                     │
│  │  SIMD SINE BANK (audio rate)          │                    │
│  │                                      │                     │
│  │  32 partials max per voice           │                     │
│  │  Phase accumulators (double)         │                     │
│  │  Amplitude from formant envelope     │                     │
│  │  Phase jitter from Blur parameter    │                     │
│  │  4-wide SIMD: process 4 partials/op │                     │
│  └────────┬─────────────────────────────┘                     │
│           │                                                   │
│           ▼                                                   │
│  ┌──────────────────────────────────────┐                     │
│  │  TARTINI INTERMODULATOR (audio rate)  │                    │
│  │                                      │                     │
│  │  Polynomial waveshaping:             │                     │
│  │    out = in + drive * in^2           │                     │
│  │  Generates f_a - f_b difference      │                     │
│  │  tones in the listener's cochlea     │                     │
│  │  Soft clip to prevent DC explosion   │                     │
│  └────────┬─────────────────────────────┘                     │
│           │                                                   │
│           ▼                                                   │
│  ┌──────────────────────────────────────┐                     │
│  │  NEURAL DECAY (per-partial envelope)  │                    │
│  │                                      │                     │
│  │  Lower partials release faster       │                     │
│  │  (illusion fades from bottom up)     │                     │
│  │  Higher partials sustain longer      │                     │
│  │  Amp envelope: standard ADSR         │                     │
│  └────────┬─────────────────────────────┘                     │
│           │                                                   │
│           ▼                                                   │
│  ┌──────────────────────────────────────┐                     │
│  │  OUTPUT STAGE                         │                    │
│  │                                      │                     │
│  │  DC block (CytomicSVF HP @ 5Hz)     │                     │
│  │  Soft limiter (tanh)                 │                     │
│  │  Coupling sample cache               │                     │
│  └──────────────────────────────────────┘                     │
│                                                               │
└──────────────────────────────────────────────────────────────┘
```

---

## 3. Parameter Architecture

All parameters use `oscil_` prefix per the `{shortname}_{paramName}` convention.

### 3.1 Core Parameters (8)

| Parameter ID | Display Name | Range | Unit | Mapping | Rate | Description |
|---|---|---|---|---|---|---|
| `oscil_phantomTarget` | Phantom Target | 0-127 | MIDI note | Linear | Audio | Root note the brain should hear. Driven by MIDI note input. Exposed for coupling override. |
| `oscil_residueDensity` | Residue Density | 2-32 | partials | Stepped | Control | Number of upper partials generated to create the illusion. 2 = fragile ghost. 32 = dense, convincing phantom. |
| `oscil_oddEvenSkew` | Odd/Even Skew | -1.0 to 1.0 | ratio | Linear | Control | Bias toward odd harmonics (-1 = hollow, square-like) or even harmonics (+1 = bright, saw-like). 0 = all partials. |
| `oscil_inharmonicity` | Inharmonicity | 0.0-1.0 | amount | Exp | Control | Stretches partial series away from harmonic ratios. 0 = pure illusion. 1 = metallic bells, shattered perception. |
| `oscil_tartiniDrive` | Tartini Drive | 0.0-1.0 | amount | Linear | Audio | Intermodulation intensity. Generates physical difference tones in the listener's inner ear. The illusion becomes physical. |
| `oscil_formantAnchor` | Formant Anchor | 200-8000 | Hz | Log | Control | Center frequency of the partial amplitude envelope. Low = dark, bassy phantom. High = bright, ethereal. |
| `oscil_neuralDecay` | Neural Decay | 0.01-8.0 | seconds | Log | Control | Release time for lower partials. Controls how quickly the phantom illusion fades after note release. |
| `oscil_blur` | Blur | 0.0-1.0 | amount | Linear | Audio | Audio-rate phase jitter on partial phases. 0 = crystalline. 1 = scattered, atmospheric. |

### 3.2 Envelope Parameters (4)

| Parameter ID | Display Name | Range | Unit |
|---|---|---|---|
| `oscil_ampAttack` | Attack | 0.001-2.0 | seconds |
| `oscil_ampDecay` | Decay | 0.001-4.0 | seconds |
| `oscil_ampSustain` | Sustain | 0.0-1.0 | level |
| `oscil_ampRelease` | Release | 0.01-8.0 | seconds |

### 3.3 Utility Parameters (3)

| Parameter ID | Display Name | Range | Description |
|---|---|---|---|
| `oscil_level` | Level | 0.0-1.0 | Output volume |
| `oscil_polyphony` | Polyphony | 1, 2, 4, 8, 16 | Max simultaneous voices |
| `oscil_pitchBendRange` | Bend Range | 0-24 | Semitones for pitch bend |

### 3.4 Macro Mapping

| Macro | Label | Target | Musical Effect |
|---|---|---|---|
| M1 | CHARACTER | `oscil_tartiniDrive` | From ghostly pure phantom → physically biting intermodulation tones |
| M2 | MOVEMENT | `oscil_inharmonicity` (LFO) | From pure harmonic illusion → shifting metallic bell textures |
| M3 | COUPLING | Cross-engine coupling depth | How strongly the phantom pitch anchors connected engines |
| M4 | SPACE | `oscil_formantAnchor` | Shift spectral peak: deep dark phantom ↔ bright ethereal shimmer |

**Golden rule verified:** All 4 macros produce audible change across their full range in every preset archetype.

---

## 4. DSP Implementation Details

### 4.1 Residue Pitch Solver (Control Rate)

Runs once per audio block (~94 Hz at 512 samples / 48kHz).

```cpp
// Per-block calculation
void updatePartials(float targetFreq, float density, float skew, float inharm, float formantHz) {
    int numPartials = static_cast<int>(density);
    int partialIndex = 0;

    for (int n = 2; n <= numPartials + 1 && partialIndex < MaxPartials; ++n) {
        // Skip based on odd/even skew
        bool isOdd = (n % 2 != 0);
        if (skew < 0.0f && !isOdd) continue;  // negative skew = odd only
        if (skew > 0.0f && isOdd) continue;    // positive skew = even only
        // At skew == 0, include all

        // Frequency with inharmonicity stretch
        float stretchFactor = 1.0f + inharm * (n - 1) * 0.01f;
        partials[partialIndex].freq = targetFreq * n * stretchFactor;

        // Amplitude from formant envelope (Gaussian centered on formantHz)
        float logRatio = std::log2(partials[partialIndex].freq / formantHz);
        partials[partialIndex].amp = std::exp(-logRatio * logRatio * 2.0f);

        ++partialIndex;
    }
    activePartialCount = partialIndex;
}
```

### 4.2 SIMD Sine Bank (Audio Rate)

New shared DSP module: `Source/DSP/SineBank.h`

Processes 4 partials simultaneously using SIMD:

```cpp
// Per-sample: 4-wide SIMD sine evaluation
inline void renderSample(float* output, int numPartials) {
    float sum = 0.0f;

    for (int i = 0; i < numPartials; i += 4) {
        // Load 4 phases
        auto phases = vld1q_f32(&phaseAccumulators[i]);
        // Load 4 amplitudes
        auto amps = vld1q_f32(&partialAmps[i]);

        // Fast sine approximation (from FastMath.h, vectorized)
        auto sines = fastSin4(phases);

        // Multiply by amplitudes
        auto weighted = vmulq_f32(sines, amps);

        // Horizontal sum
        sum += vaddvq_f32(weighted);

        // Advance phases
        auto increments = vld1q_f32(&phaseIncrements[i]);
        phases = vaddq_f32(phases, increments);

        // Wrap phases to [0, 2π)
        // ... (modulo operation)

        vst1q_f32(&phaseAccumulators[i], phases);
    }
    *output = sum;
}
```

**Note:** ARM NEON intrinsics shown. x86 build uses SSE/AVX equivalents via platform abstraction in `FastMath.h`.

### 4.3 Tartini Intermodulator (Audio Rate)

Simple polynomial waveshaping that generates combination tones:

```cpp
inline float tartiniProcess(float input, float drive) {
    // Quadratic term generates f_a ± f_b combination tones
    float squared = input * input;
    float out = input + drive * squared;
    // Soft clip to prevent DC explosion from squared term
    return FastMath::fastTanh(out);
}
```

The combination tones (f_a - f_b = f_target) physically reinforce the phantom fundamental in the listener's cochlea.

### 4.4 Neural Decay (Per-Partial Envelope)

Lower partials decay faster than higher ones, causing the phantom illusion to dissolve from the bottom up:

```cpp
float getPartialRelease(int partialIndex, float baseRelease) {
    // Lower partials (closer to missing fundamental) decay faster
    float scaleFactor = 1.0f + static_cast<float>(partialIndex) * 0.3f;
    return baseRelease * scaleFactor;
}
```

### 4.5 Memory Layout

All buffers pre-allocated in `prepare()`:

| Buffer | Size | Purpose |
|---|---|---|
| `phaseAccumulators[32]` | 128 bytes per voice | Phase state for each partial |
| `phaseIncrements[32]` | 128 bytes per voice | Frequency → phase increment (cached per block) |
| `partialAmps[32]` | 128 bytes per voice | Amplitude weights (cached per block) |
| `partialFreqs[32]` | 128 bytes per voice | Frequencies (cached per block) |
| `couplingBuffer[maxBlockSize]` | ~2KB | Cached output for `getSampleForCoupling()` |

**Total per voice:** ~514 bytes + coupling buffer. At 16 voices: ~10KB. Negligible.

---

## 5. Coupling Compatibility

### 5.1 Emissions (what XOscillum sends)

| CouplingType | Signal | Musical Effect |
|---|---|---|
| `PitchToPitch` | Phantom target frequency | **THE signature coupling.** Anchors other engines to a root note that XOscillum never physically plays. A SNAP percussion tuned to a note that exists only in the listener's brain. |
| `FilterToFilter` | Formant envelope center | Spectral emphasis from XOscillum's partial distribution applied to another engine's filter. |

### 5.2 Accepts (what XOscillum receives)

| CouplingType | Effect on XOscillum |
|---|---|
| `AudioToFM` | External audio FM-modulates upper partial phases. Tears apart the harmonic relationships, causing the phantom illusion to flutter or shatter. |
| `EnvToMorph` | External envelope drives `oscil_residueDensity`. Morphs the illusion from fragile (2 partials) to dense (32 partials). |
| `LFOToPitch` | External LFO modulates phantom target pitch. Creates vibrato on a note that doesn't exist. |
| `AmpToFilter` | External amplitude modulates formant anchor. Louder playing shifts the spectral peak. |

### 5.3 Coupling Priority Pairings

| Pair | Type | Musical Result |
|---|---|---|
| OSCIL + SNAP | PitchToPitch | Percussion tuned to a phantom root — drums in a key that no oscillator plays |
| OSCIL + DRIFT | PitchToPitch + FilterToFilter | Psychedelic pad anchored to a hallucinated fundamental, formant shaping |
| OSCIL + BOB | PitchToPitch | Warm texture orbiting a ghost note |
| OSCIL + DUB | FilterToFilter | Dub echo chain with spectral emphasis from partial distribution |
| OSCIL + MORPH | EnvToMorph (from MORPH) | Wavetable envelope morphs the partial density — illusion breathes |
| OSCIL + OUROBOROS | AudioToFM (from OUROBOROS) | Chaos engine FM-modulates the phantom partials — controlled destruction of perception |

---

## 6. PlaySurface Mapping

| Mode | X Axis | Y Axis | Pressure (Z) |
|---|---|---|---|
| **Pad** (default) | `oscil_formantAnchor` | `oscil_inharmonicity` | `oscil_tartiniDrive` |
| **Fretless** | Continuous pitch (phantom target) | `oscil_inharmonicity` | `oscil_tartiniDrive` |
| **Drum** | Not mapped (not a percussive engine) | — | — |

---

## 7. CPU Budget Analysis

| Component | Cost per voice per sample | Notes |
|---|---|---|
| Residue solver | ~0 (control rate) | Runs once per block, negligible |
| Sine bank (32 partials) | 8 SIMD ops | 4-wide: 32/4 = 8 vector multiply-adds |
| Tartini intermodulator | 3 ops | multiply + add + tanh |
| Neural decay envelopes | 32 multiplies | Per-partial amplitude scaling |
| DC block + soft clip | 5 ops | CytomicSVF HP + tanh |
| **Total per voice per sample** | ~48 ops | |

**At 16 voices, 48kHz:** 16 × 48 × 48000 = ~36.8M ops/second. Well within M1 budget (~12% single core).

**Comparison to existing engines:**
- SNAP: ~15% (PolyBLEP + FM + filter + envelope)
- MORPH: ~15% (wavetable + ladder filter + envelope)
- XOscillum: ~12% (sine bank + intermodulator + envelope)

Fits comfortably within the established per-engine budget.

---

## 8. Preset Archetypes (10 Heroes)

| # | Name | Character | Key Settings |
|---|---|---|---|
| 1 | **Ghost Bass** | Pure phantom fundamental, zero Tartini, maximum density | density=32, tartini=0, formant=400Hz |
| 2 | **Phantom Organ** | Church-like, odd harmonics, formant mid-range | skew=-0.8, formant=800Hz, blur=0.1 |
| 3 | **Tartini Bite** | Aggressive physical difference tones | tartini=0.9, density=16, formant=2000Hz |
| 4 | **Bell Garden** | Metallic partials, long neural decay | inharm=0.8, decay=6s, formant=3000Hz |
| 5 | **Vanishing Point** | Phase-scattered spectra, atmospheric | blur=1.0, density=24, formant=1200Hz |
| 6 | **Neural Drift** | MOVEMENT macro sweeps inharmonicity | inharm modulated by M2, density=20 |
| 7 | **Psychoacoustic Drop** | Coupled with DRIFT, phantom sub anchors pad | PitchToPitch coupling, density=32 |
| 8 | **Inner Ear** | Tartini creates tones inside listener's ear canal | tartini=0.95, density=8, formant=4000Hz |
| 9 | **Partial Eclipse** | Only lowest partials audible, dark | formant=200Hz, density=6, blur=0.3 |
| 10 | **The Missing Note** | Coupled with SNAP, phantom pitch between percussion | PitchToPitch to SNAP, density=16 |

---

## 9. Build Phases (following `Docs/xomnibus_new_engine_process.md`)

### Phase 0: Ideation — COMPLETE (this document + Volume 2 source)

### Phase 1: Architecture — THIS DOCUMENT
- Parameter IDs locked with `oscil_` prefix
- Signal flow designed
- Coupling compatibility defined
- CPU budget validated
- **Gate:** Document reviewed and approved

### Phase 2: Sandbox Build
- Create standalone XOscillum project
- Implement Residue Pitch Solver
- Implement SIMD Sine Bank (`Source/DSP/SineBank.h` — shared module)
- Implement Tartini Intermodulator
- Implement Neural Decay envelopes
- Wire up ADSR + output stage
- Build 10 hero presets
- **Gate:** Audible phantom fundamental perception confirmed. Macros produce change. CPU within budget.

### Phase 3: Integration Prep
- Write `XOscillumAdapter.h` implementing `SynthEngine` interface
- Verify `PitchToPitch` coupling emission works
- Verify `AudioToFM` coupling acceptance works
- Test coupling with SNAP, DRIFT, BOB
- Write integration spec

### Phase 4: Gallery Install
- Copy DSP headers to `XO_OX-XOmnibus/Source/Engines/Oscillum/`
- `REGISTER_ENGINE(XOscillumAdapter)`
- Copy presets to XOmnibus Presets directory
- Run `compute_preset_dna.py`
- Design 10 cross-engine Entangled presets
- Update gallery docs, CLAUDE.md engine table

---

## 10. Open Questions

1. **Accent color** — Needs assignment. Suggested: a color that evokes "invisible/phantom." Perhaps a pale spectral blue, ghost white, or UV-adjacent purple. Must not conflict with existing 12 colors.
2. **Voice count** — 16 voices at 32 partials each = 512 simultaneous sine evaluations. Confirm this is acceptable for iPad target. Fallback: cap at 8 voices on mobile.
3. **SineBank.h reuse** — This shared module will also serve XOntara (gravitational pitch) and potentially XOGRAMA (720-voice optical) if ever built. Design the API for reuse from day one.
4. **Tartini perceptual validation** — The difference tone effect is amplitude-dependent and varies between listeners. Need to tune the polynomial coefficients empirically with real audio monitoring.
