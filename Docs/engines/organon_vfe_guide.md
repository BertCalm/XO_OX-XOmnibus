# Organon and the Free Energy Principle
## A Comprehensive Guide to Informational Dissipative Synthesis

**Engine:** ORGANON | XO-Organon
**Accent:** Bioluminescent Cyan `#00CED1`
**Creature:** The Deep-Sea Chemotroph
**Paradigm:** Informational Dissipative Synthesis
**Seance verdict:** 8/8 PASS — unanimous. "VFE metabolism publishable as a paper."

---

## Preface: What This Guide Is For

Organon received the only unanimous blessing from all eight ghost synthesists assembled in the XOceanus seance. Schulze, Buchla, Vangelis, Kakehashi, Tomita, Pearlman, Carlos, Smith — all eight passed it without qualification. The comment was precise: the Variational Free Energy metabolism is not an interesting gimmick or a branding metaphor. It is a genuine implementation of a neuroscience principle as a synthesis engine, and it could be published as an academic paper.

This guide exists to do three things: explain the science clearly enough that a developer can extend it; explain the synthesis clearly enough that a performer can play it; and document the vision clearly enough that the architecture decision is never reversed in the name of simplicity.

---

## Part I: The Neuroscience

### Karl Friston's Free Energy Principle (2010)

In 2010, Karl Friston at University College London proposed a unified theory of how biological brains maintain themselves: the **Free Energy Principle**. The core claim is remarkable in its simplicity: all living systems minimize *surprise*.

More precisely, Friston argues that any self-organizing system that maintains its structure over time must minimize its *variational free energy* — a mathematical quantity that upper-bounds the surprise (or unpredictability) of sensory inputs given the system's internal model of the world. Systems that cannot do this dissolve; systems that can, persist.

The formal statement is:

```
F = D_KL[q(s) || p(s|o)] + log p(o)
  = (accuracy term) + (complexity term)
```

Where:
- `q(s)` is the system's internal belief about hidden causes `s`
- `p(s|o)` is the true posterior probability of causes given observations `o`
- `D_KL` is the Kullback-Leibler divergence (a measure of how wrong the belief is)
- `log p(o)` is the log probability of the observation (a proxy for surprise)

In plain language: a system minimizes free energy by forming better predictions and adjusting its behavior to make surprising inputs less likely. This is why you don't flinch at a sound you've heard before, but startle at an unexpected one — your brain updates its model to minimize future surprise.

Friston's insight was that this applies not just to brains but to any system that models its environment: cells, immune systems, organisms, and — Organon proposes — synthesizers.

### Active Inference: The Action Side

The Free Energy Principle has a passive half (perception) and an active half (action). In **Active Inference** (Friston et al., 2016), the system not only updates its internal model to match observations — it also takes actions to make the environment conform to its predictions. A thermostat that adjusts the temperature to match its set-point is a minimal example. A mammal that seeks food when hungry is a richer one.

Organon implements the perception half in real time and sketches the action half through its metabolic behavior: when surprised, the organism burns energy faster (a kind of stress response) and shifts its spectral character — acting on its own synthesis to move toward a state it predicts will be less surprising.

### References

- Friston, K. (2010). The free-energy principle: A unified brain theory? *Nature Reviews Neuroscience*, 11(2), 127–138.
- Friston, K., FitzGerald, T., Rigoli, F., Schwartenbeck, P., O'Doherty, J., & Pezzulo, G. (2016). Active inference and learning. *Neuroscience & Biobehavioral Reviews*, 68, 862–879.
- van der Schaft, A., & Jeltsema, D. (2014). Port-Hamiltonian systems theory: An introductory overview. *Foundations and Trends in Systems and Control*, 1(2–3), 173–378.

---

## Part II: The Synthesis Paradigm

### The Paradigm Inversion

Every synthesizer ever built operates on a single assumption: the performer controls the instrument. You turn a knob, the timbre changes. You press a key, a sound begins. The instrument is passive; the performer is active.

Organon inverts this. The engine has an internal model of its acoustic environment — a prediction of what it expects to hear — and it continuously updates both its model and its synthesis behavior based on how well reality matches that prediction. The performer provides stimulus, but the engine decides how to respond. Over the course of a performance, Organon's internal model evolves: it learns its diet, learns the rhythm of notes, learns the spectral character of its coupling partners. A patch played for five minutes will sound different at minute five than at minute one, not because the player changed anything, but because the organism has adapted.

This is not an LFO. It is not envelope-following. It is not a random modulator. It is a continuous Bayesian belief update operating on a generative model of incoming audio.

### The Metabolism Metaphor (Not a Metaphor)

Organon's design document calls it a "deep-sea chemotroph" — an organism like tube worms at hydrothermal vents that survive without sunlight by metabolizing chemicals from the ocean floor. This is not decoration. The biological framing is structurally accurate:

| Biology | Organon |
|---|---|
| Ingestion — the organism consumes food | Audio enters via coupling input or internal noise |
| Catabolism — breaking food into usable energy | Shannon entropy analysis extracts information content |
| Metabolic economy — ATP pool | Free Energy pool tracking with VFE minimization |
| Anabolism — building structures from energy | 32-mode Port-Hamiltonian modal array driven by free energy |
| Starvation — organism dims when underfed | Note release triggers 4x metabolic cost; harmonics thin |
| Adaptation — organism learns its diet | Bayesian belief update of predicted entropy |
| Bioluminescence — organism glows more when healthy | Richer harmonics emerge when free energy is high |

The chemotroph that starves goes quiet. The chemotroph that feeds blooms. Organon is the same.

### Why This Is Audibly Distinct

Three properties combine to make Organon's behavior irreducible to existing synthesis paradigms:

**1. Sound requires feeding.** At note-on, all 32 modal oscillators start at silence (x=0, v=0). The organism has no energy. It must metabolize signal — either from coupling partners or its internal noise substrate — before any harmonic content emerges. The bloom time is not a fixed attack envelope; it is determined by the rate of entropy accumulation in the free energy pool. High-entropy input (noise, complex audio) feeds the organism faster. Simple tonal input feeds it slowly. The player can hear this: a Hyper-Metabolic patch fed with ONSET drum hits will bloom in milliseconds; the same patch sustained alone will bloom over seconds as it digests its own noise.

**2. The engine has a history.** Each voice carries an independent free energy pool and VFE belief state. When legato mode migrates a voice to a new note, the free energy is preserved — the organism slides to the new pitch without dying. The belief state (`predictedEntropy`, `entropyVariance`, `adaptationGain`) accumulates across the life of the voice. An organism that has been playing for 30 seconds has a different metabolic character than one freshly spawned.

**3. Surprise has timbral consequences.** When the observed entropy diverges from the predicted entropy, VFE rises. This is not a silent internal update — it cascades into the synthesis:
- Metabolic rate increases (`effectiveRate *= 1 + surprise * 2`)
- Spectral character shifts (`effectiveIsotope += surprise * 0.15`)
- Catalyst effectiveness drops (`effectiveCatalyst *= adaptationGain`)
- Stereo image widens (`spread = surprise * 0.15`)
- Reverb send increases (`sendLevel += surprise * 0.3`)

A performer who plays unexpectedly after a long silence will hear the organism react: a brief destabilization of harmonics, a widening of the stereo field, an increase in reverb as the organism "sweats" — then a gradual settling as the belief model updates and the organism learns the new diet.

---

## Part III: Technical Implementation

### Architecture Overview

```
Per-Voice Signal Flow
═══════════════════════════════════════════════════════════════════

 ┌────────────────────┐
 │  INGESTION STAGE   │
 │                    │
 │  CouplingInput ────┤──► ring buffer (2048 samples, 46ms)
 │    (AudioToFM,     │
 │   AudioToWavetable)│
 │                    │      OR
 │  Internal Noise ───┤──► xorshift32 PRNG
 │  (no coupling)     │    ↓ bandpass filter (Cytomic SVF)
 │                    │    [enzyme selectivity, noise color → Q]
 └────────────────────┘
           │
           │ ingestedSample × signalFlux
           ▼
 ┌────────────────────────────────────────────┐
 │  CATABOLISM (Shannon Entropy Analysis)     │
 │                                            │
 │  Ring buffer: 256 samples (5.8ms)          │
 │  Histogram: 32 amplitude bins              │
 │  Control rate: every ~22 samples (2kHz)    │
 │                                            │
 │  H = -Σ p_i · log₂(p_i)                  │
 │  entropyValue = H / log₂(32)              │
 │  spectralCentroid = Σ(i · p_i) / Σ(p_i)  │
 └────────────────────────────────────────────┘
           │
           │ entropyValue, spectralCentroid
           ▼
 ┌────────────────────────────────────────────┐
 │  METABOLIC ECONOMY (VFE / Active Inf.)    │
 │                                            │
 │  Prediction error: (obsH - predH)²        │
 │  VFE: precision·error + complexity        │
 │  Belief update: predH += lr · error       │
 │  Variance: EMA of squared errors          │
 │  Adaptation gain: 1 - VFE·2 (smoothed)   │
 │                                            │
 │  Intake: entropy·flux·adaptGain·(1+rMod) │
 │  Cost:   metabolicRate·0.1·(1+VFEmod)    │
 │  freeEnergy += (intake - cost) · dt       │
 └────────────────────────────────────────────┘
           │
           │ freeEnergy, adaptationGain, surprise
           ▼
 ┌────────────────────────────────────────────┐
 │  ANABOLISM (Port-Hamiltonian Modal Array) │
 │                                            │
 │  32 modes, RK4 @ audio rate              │
 │  f_n = f_note · n^spread                 │
 │  spread ∈ [0.5, 2.0] ← isotopeBalance    │
 │                                            │
 │  F_n = catalystDrive·freeEnergy·         │
 │        entropy·gaussianWeight(n)          │
 │                                            │
 │  dv_n/dt = -ω_n²·x_n - γ·v_n + F_n      │
 │  γ = dampingCoeff · 20                   │
 └────────────────────────────────────────────┘
           │
           │ Σ x_n (summed modal displacement)
           ▼
 ┌────────────────────────────────────────────┐
 │  OUTPUT                                   │
 │  fastTanh soft clip                       │
 │  VFE stereo spread (±0.15)               │
 │  → outputCacheL/R (coupling source)      │
 │  → reverb send: membrane + surprise·0.3  │
 └────────────────────────────────────────────┘
```

### Section 1: The Noise Substrate (OrganonNoiseGen)

When no coupling partner feeds audio to the organism, it generates its own internal noise using the **xorshift32 PRNG** (Marsaglia, 2003):

```
state ^= state << 13
state ^= state >> 17
state ^= state << 5
output = state / 2^31
```

This produces white noise with period 2^32-1. Each voice's PRNG is seeded uniquely with `note × 7919 + time` — the prime 7919 decorrelates seeds across adjacent MIDI notes, so unison intervals do not produce correlated noise.

The noise is then filtered through a **Cytomic SVF bandpass** filter tuned to the Enzyme Selectivity frequency. The Noise Color parameter adjusts the filter Q (resonance), controlling the spectral tilt of the substrate: low Q produces a gentle, broadly colored band; high Q produces a narrow, resonant peak.

### Section 2: Shannon Entropy Analysis (EntropyAnalyzer)

The catabolism stage measures the **information content** of the ingested signal using Shannon entropy over a short-time amplitude histogram.

**Algorithm, per control tick (~2kHz):**

1. Read the most recent `windowSize` samples (256 default, 64 for high-frequency enzyme settings above 10kHz)
2. Quantize each sample from [-1,1] into one of 32 amplitude bins
3. Count bin occupancy to form histogram H[32]
4. Normalize: `p[i] = H[i] / windowSize`
5. Compute Shannon entropy: `E = -Σ p[i] · log₂(p[i])` for all p[i] > 0
6. Normalize to [0,1]: `entropyValue = E / log₂(32)` (log₂(32) = 5.0 is the theoretical maximum)
7. Compute spectral centroid: amplitude-weighted mean of bin indices

**What entropy means for sound:**
- `entropyValue = 1.0`: Perfectly uniform noise — maximum information content, maximum nutrition
- `entropyValue = 0.5`: Moderately complex signal (most real audio)
- `entropyValue ≈ 0.0`: DC or single-frequency sine wave — minimal information, near-starvation

This is why white noise feeds the organism faster than a sine wave, and why complex drum audio feeds it faster than a sustained pad. The engine is discriminating about signal richness in the strict Shannon sense.

The **spectral centroid** output tells the modal array where in the amplitude histogram the energy is concentrated. This biases which of the 32 modes receive driving force — the engine reconstructs a harmonic structure that mirrors the spectral center of mass of what it consumed.

### Section 3: Variational Free Energy Metabolism (MetabolicEconomy)

This is the core of the Friston implementation. Per control tick (~2kHz), the organism performs the following operations:

**Step 1 — Prediction Error (Surprise)**

```
predictionError = observedEntropy - predictedEntropy
surprise = predictionError²
```

`predictedEntropy` is the organism's current **belief** about what entropy value it expects. If it has been digesting white noise for several seconds, it predicts high entropy; a sudden shift to silence produces a large prediction error.

**Step 2 — Precision Weighting**

```
precision = 1 / (entropyVariance + 0.01)
weightedError = surprise × precision
```

When the organism is confident in its model (low `entropyVariance`), the same surprise produces a larger weighted error — confident predictions that are violated cause more "shock." This is the Bayesian interpretation of precision as inverse uncertainty.

**Step 3 — VFE Computation**

```
complexityPenalty = beliefChangeRate² × 0.1
VFE = weightedError + complexityPenalty
```

The complexity penalty penalizes rapid belief changes. This is the Occam's razor term in Friston's formulation — the organism should prefer simple, stable beliefs over rapid oscillation. Without this term, the organism would thrash between predictions in a way that produces erratic synthesis behavior.

**Step 4 — Bayesian Belief Update**

```
learningRate = clamp(metabolicRate × 0.05, 0.001, 0.2)
predictedEntropy += learningRate × predictionError
```

The organism updates its prediction toward the observed value. The learning rate scales with Metabolic Rate: a fast-metabolizing organism learns faster. At maximum metabolic rate (10Hz), the learning rate reaches 0.2 — the organism updates its belief by up to 20% per control tick, allowing rapid tracking of changing inputs. At minimum rate (0.1Hz), learning rate is 0.005 — the belief changes almost glacially.

**Step 5 — Variance and Adaptation Gain**

```
entropyVariance = entropyVariance × 0.98 + surprise × 0.02
adaptationGain = smooth(1 - VFE × 2, 0.97/0.03)
```

Variance tracks the organism's uncertainty using a slow exponential moving average. `adaptationGain` (smoothed to prevent timbral jumps) represents how settled the organism is: 1.0 when fully adapted, 0.2 when in a high-VFE adaptation state.

**The Metabolic Economy**

```
effectiveRate = metabolicRate × (1 + externalDecayMod) × (1 + surprise × 2)
intake = entropyValue × signalFlux × adaptationGain × (1 + externalRhythmMod)
cost = effectiveRate × 0.1

// Note-off starvation
if (noteReleased): cost × 4

freeEnergy += (intake - cost) × dt
freeEnergy = clamp(freeEnergy, 0.0, 1.0)
```

The `adaptationGain` on the intake side is crucial: a settled organism that has learned its diet extracts more energy from the same signal. An organism in an adaptation state (high VFE) redirects metabolic resources to belief-updating rather than harmonic synthesis — it temporarily "sounds thinner" while it adapts, then blooms again when settled.

The `surprise × 2` modulation on effective rate is the fight-or-flight response: a fully surprised organism metabolizes three times faster than its baseline rate, burning through reserves quickly before settling.

### Section 4: Port-Hamiltonian Modal Array (ModalArray)

The anabolism stage reconstructs harmonic content using **32 damped harmonic oscillators** formulated as port-Hamiltonian systems.

**Port-Hamiltonian Formulation**

For each mode n, the state is `z_n = [x_n, p_n]` (displacement, momentum). The dynamics are:

```
dz_n/dt = (J - R) · ∇H_n + B · u_n

where:
  H_n = 0.5 · (p_n² + ω_n² · x_n²)   Hamiltonian (energy)
  J = [0, 1; -1, 0]                    Skew-symmetric (energy conserving)
  R = [0, 0; 0, γ]                     Dissipation matrix
  B = [0; 1]                           Input port
  u_n = F_n(t)                         Driving force
```

The Port-Hamiltonian formulation (van der Schaft & Jeltsema, 2014) guarantees **passivity**: the system cannot generate energy that was not injected through the input port. This is critical for a synthesis engine with real-time driving forces — without the structural energy guarantee, the 32-mode array would require constant amplitude monitoring to prevent runaway instability. The port-Hamiltonian framework makes stability a mathematical property of the formulation rather than a runtime check.

Expanding to scalar ODEs (equivalent to the above):

```
dx_n/dt = v_n
dv_n/dt = -ω_n² · x_n - γ · v_n + F_n(t)
```

Solved with **4th-order Runge-Kutta (RK4)** per audio sample. The RK4 algorithm:

```
k1 = f(t, y)
k2 = f(t + h/2, y + h·k1/2)
k3 = f(t + h/2, y + h·k2/2)
k4 = f(t + h, y + h·k3)
y_next = y + h·(k1 + 2·k2 + 2·k3 + k4)/6
```

For 32 modes per sample at 44.1kHz, this is 32 × 4 = 128 function evaluations per audio sample. The arrays are 16-byte aligned (`alignas(16)`) for SIMD vectorization: 4-wide float SIMD reduces this to 32 iterations per sample.

**Mode Frequency Distribution**

Mode frequencies follow a power-law distribution controlled by Isotope Balance:

```
f_n = f_fundamental × n^spread
spread = 0.5 + isotopeBalance × 1.5   // maps [0,1] → [0.5, 2.0]
```

At `spread = 1.0` (balance ≈ 0.33): standard harmonic series (f, 2f, 3f, ...) — acoustic, familiar.
At `spread = 0.5` (balance = 0.0): compressed square-root spacing — subharmonic weighting, dark.
At `spread = 2.0` (balance = 1.0): squared spacing (f, 4f, 9f, 16f...) — inharmonic, metallic, alien.

All mode frequencies are clamped to 49% of Nyquist to prevent aliasing.

**Driving Force Distribution**

Each mode's driving force uses a Gaussian weight peaked at the spectral centroid:

```
modeWeight_n = exp(-0.5 · ((n/32 - spectralCentroid) / bandwidth)²)
bandwidth = 0.15 + 0.35 × isotopeBalance

F_n = catalystDrive × freeEnergy × entropyValue × modeWeight_n × adaptationGain
```

This means Organon's harmonic reconstruction mirrors the spectral center of mass of what it consumed: if the input signal has its energy in the midrange, the modes near the midrange are energized. The organism reconstructs its diet.

**Denormal Protection**

The exponential decay of `x_n` and `v_n` under damping produces subnormal IEEE 754 floats (values below ~1.18 × 10⁻³⁸) that can cause 10–100x CPU penalties on x86 processors. `flushDenormal()` is applied after every RK4 step:

```cpp
displacement[n] = flushDenormal(displacement[n]);
velocity[n] = flushDenormal(velocity[n]);
```

This is not optional. Without denormal protection, sustained silence with active voices can cause CPU spikes.

---

## Part IV: Performance Guide

### What the Engine Does Autonomously

Organon manages the following without any performer input:

- **Belief maintenance**: The `predictedEntropy` continuously tracks the statistical character of incoming audio. The organism always has an expectation, and always updates it.
- **Adaptation gain modulation**: When VFE is high (organism is surprised), synthesis quality drops slightly — the organism redirects resources to adaptation. This recovers automatically as VFE falls.
- **Free energy dynamics**: The pool fills when fed and depletes when starved. The rate of depletion is itself modulated by metabolic state. The performer cannot directly set the free energy pool value.
- **VFE effects on timbre**: Spectral shifts from surprise, stereo widening, reverb send increases — all happen automatically in response to the prediction/observation relationship.
- **Starvation decay**: On note release, metabolic cost increases 4x. The organism naturally fades as it depletes its reserves. Damping coefficient controls how long this takes, but the fade shape is metabolically determined, not a fixed ADSR envelope.

### What the Performer Controls

The performer has ten parameters that shape the organism's *character* and *environment*, not its moment-to-moment behavior:

**CHARACTER macro (M1):**
- `organon_metabolicRate`: How fast the organism burns and learns. Slow (0.1Hz) = glacial, accumulating, memory-like. Fast (10Hz) = reactive, volatile, forgetful.
- `organon_isotopeBalance`: Which part of the harmonic spectrum is energized. 0.0 = deep sub-weighting, 0.5 = acoustic midrange, 1.0 = metallic upper partials.

**MOVEMENT macro (M2):**
- `organon_catalystDrive`: The gain on the modal driving force. At zero, no modes are excited regardless of free energy. At 2.0, modes can self-excite at high energy levels.
- `organon_phasonShift`: Temporal offset between voices' metabolic cycles. Creates polyrhythmic pulsing from 0 (unison metabolism) to 1 (fully offset, maximum polyrhythm).

**COUPLING macro (M3):**
- `organon_enzymeSelect`: Center frequency of the ingestion bandpass. Controls which frequency band the organism can "digest." High settings with high-entropy input above 10kHz also trigger the fast 64-sample entropy window.
- `organon_lockIn`: Synchronization strength to DAW tempo. At 1.0, the metabolic rate quantizes to the nearest beat subdivision, creating tempo-locked pulsing.

**SPACE macro (M4):**
- `organon_membrane`: Reverb send level (base). This is further modulated by VFE surprise — surprised organisms send more to reverb.
- `organon_dampingCoeff`: Modal damping coefficient (gamma). Low = long resonant shimmer. High = percussive mute. This is the primary control over tail length.

**Direct parameters:**
- `organon_signalFlux`: Input gain on the ingested signal. At 0, the organism starves. At 1, it feeds at maximum rate. The primary volume-like control for the metabolic process.
- `organon_noiseColor`: Spectral tilt of the internal noise substrate (standalone mode only; coupling bypasses this). 0.0 = dark, rumbling. 1.0 = bright, hissing.

### The Note-On Event

Note-on does not immediately produce sound. What happens:

1. A voice is allocated (or the oldest voice is stolen with a 5ms crossfade)
2. The voice's modal array is reset to silence (x=0, v=0)
3. The VFE belief state is reset to defaults (`predictedEntropy = 0.5`, `adaptationGain = 1.0`)
4. Initial free energy is set to `velocity × 0.15` — higher velocity provides a 15% energy head start, allowing faster initial bloom without bypassing the growth curve
5. The noise PRNG is seeded with `note × 7919 + time`
6. The organism begins metabolizing

Sound emerges over the first few hundred milliseconds as free energy accumulates. This is the engine's defining signature: the arrival is never instantaneous.

### Legato Migration

In legato play, the new note inherits the existing voice's free energy. The organism does not die and restart — it migrates to the new pitch, preserving its metabolic reserves and adaptation state. Only the modal array is reset to retune to the new fundamental. This means a legato phrase on a well-fed organism will maintain timbral density across pitch changes; the organism carries its history to each new note.

### Working With the Engine

**Feed it before you need it.** If playing with coupling partners, route an active engine (ONSET drum hits, ODDFELIX transients, OVERDUB delay tails) to Organon via AudioToFM before the performance moment you want to use. The organism builds free energy reserves from the feeding.

**Understand the latency of surprise.** When you change the diet (switching coupling partners, changing Enzyme Selectivity, adjusting Signal Flux), the VFE belief update has a time constant set by Metabolic Rate. At 1Hz default, the organism takes roughly a second to adapt to the new diet. At 10Hz, it adapts in 100–200ms.

**Low Catalyst Drive + high free energy = subtle evolution.** The modal modes will be lightly driven, producing a quiet, slowly evolving texture. Sweeping Catalyst Drive up while the organism is well-fed causes a sudden harmonic bloom.

**Phason Shift as a chord thickener.** With 3–4 voices held simultaneously and Phason Shift above 0.3, each voice metabolizes at a different phase of the metabolic cycle. The result is a chorus-like thickening where individual voices swell in and out of phase — organic and non-periodic in a way no traditional LFO chorus can replicate.

**Lock-in for rhythmic textures.** At Lock-in Strength 1.0 with a tempo-synced DAW, the metabolic pulse quantizes to the nearest beat subdivision. Combined with Phason Shift, this creates tempo-locked polyrhythmic pulsing. The MOVEMENT macro (Catalyst Drive + Phason Shift) is the primary performance control for this behavior.

**The organism that starved beautifully.** Low Signal Flux (0.1–0.2) with high Catalyst Drive and moderate Damping produces an organism that is perpetually near-starved but occasionally surges. When you hold a note, free energy slowly accumulates and modes start to emerge — then the cost exceeds intake and modes thin back out. This creates a long, slow pulse with no LFO.

---

## Part V: Parameter Reference

All 10 Organon parameters use the `organon_` prefix in the APVTS.

| Parameter ID | Display Name | Range | Default | Description |
|---|---|---|---|---|
| `organon_metabolicRate` | Metabolic Rate | 0.1–10.0 Hz (log) | 1.0 | Speed of energy turnover. Controls how fast free energy depletes AND how fast the VFE belief model learns. |
| `organon_enzymeSelect` | Enzyme Selectivity | 20–20000 Hz (log) | 1000 | Bandpass center frequency for the ingestion filter. Controls the organism's dietary spectrum. Also sets entropy analysis window size (>10kHz → 64-sample fast window). |
| `organon_catalystDrive` | Catalyst Drive | 0.0–2.0 | 0.5 | Gain multiplier on the modal driving force. At 0: no sound regardless of free energy. At 2.0: intense, potentially self-exciting at high energy. Modulated by adaptation gain (VFE). |
| `organon_dampingCoeff` | Damping | 0.01–0.99 | 0.3 | Modal damping coefficient (gamma, mapped 0.2–19.8). Low: long resonant shimmer. High: short, percussive. |
| `organon_signalFlux` | Signal Flux | 0.0–1.0 | 0.5 | Input gain on the ingested signal. Primary control for starvation/feeding rate. At 0: silence after decay. At 1: maximum feeding. |
| `organon_phasonShift` | Phason Shift | 0.0–1.0 (displayed 0°–360°) | 0.0 | Temporal offset between voices' metabolic update cycles. Creates polyrhythmic pulsing. 0 = unison, 1 = maximally spread. |
| `organon_isotopeBalance` | Isotope Balance | 0.0–1.0 | 0.5 | Mode frequency spread exponent [0.5, 2.0]. 0.0 = subharmonic compression, 0.33 = natural harmonic series, 1.0 = metallic/inharmonic spread. |
| `organon_lockIn` | Lock-in Strength | 0.0–1.0 | 0.0 | Synchronization to SharedTransport tempo. 0 = free-running metabolism. 1 = quantized to nearest beat subdivision (16th, 8th, quarter, half, whole). |
| `organon_membrane` | Membrane Porosity | 0.0–1.0 | 0.2 | Base reverb send level. Further modulated by average VFE surprise across voices (up to +30%). |
| `organon_noiseColor` | Noise Color | 0.0–1.0 | 0.5 | Spectral tilt of internal noise substrate (standalone only). 0.0 = dark/rumbling, 0.5 = white, 1.0 = bright/hissing. |

**Internal state (read-only, not parameters):**

| Internal Variable | Description | Observable Via |
|---|---|---|
| `freeEnergy` | Metabolic reserves [0,1] | UI meter (bioluminescent glow) |
| `predictedEntropy` | Organism's current belief about expected input entropy | — |
| `entropyVariance` | Uncertainty in that prediction | — |
| `adaptationGain` | Settledness [0.2, 1.0] | Timbral density |
| `surprise` | Squared prediction error | Stereo widening, reverb swell |
| `variationalFreeEnergy` | Total VFE | Metabolic behavior |

---

## Part VI: Preset Archetypes

Seven factory archetypes exercise different aspects of the engine:

### Warm Start (Foundation) — Standalone
**DNA:** brightness 0.3, warmth 0.7, movement 0.3, density 0.3, space 0.2, aggression 0.1
**Use:** The first-encounter preset. Loads alone, produces a warm evolving pad within 3 seconds of the first note. Low metabolic rate (0.5Hz), gentle catalyst (0.4), dark-warm noise color. The organism is slow and contemplative.
**Key settings:** `metabolicRate=0.5`, `enzymeSelect=800`, `catalystDrive=0.4`, `dampingCoeff=0.15`, `signalFlux=0.6`, `isotopeBalance=0.35`

### Hibernation Cycle (Atmosphere) — Requires coupling
**DNA:** brightness 0.2, warmth 0.6, movement 0.5, density 0.2, space 0.6, aggression 0.1
**Use:** Near-zero signal flux standalone — the organism is dormant. Feed it another engine (ODDOSCAR pads, OVERDUB tails) and it blooms into slow harmonic growth. Remove the feed; it dims back to near-silence.

### Hyper-Metabolic Lead (Prism) — Standalone
**DNA:** brightness 0.8, warmth 0.2, movement 0.8, density 0.7, space 0.1, aggression 0.8
**Use:** Maximum metabolic rate (10Hz), high catalyst (1.8), bright isotope balance, short damping. The organism burns fast and learns fast. Velocity directly shapes bloom time (high velocity = near-immediate harmonic density). The fast 64-sample entropy window (enzyme >10kHz) enables rapid transient tracking.

### Symbiotic Drone (Entangled) — Requires coupling
**DNA:** brightness 0.4, warmth 0.5, movement 0.6, density 0.8, space 0.7, aggression 0.2
**Use:** Signal flux at 1.0, catalyst low, high damping. The organism fully reconstructs whatever it is fed, with no self-oscillation — pure metabolic transformation of the partner engine's output.

### Pace-of-Life Groove (Flux) — Standalone
**DNA:** brightness 0.5, warmth 0.4, movement 0.9, density 0.5, space 0.3, aggression 0.4
**Use:** Lock-in at 0.7, phason shift active, metabolic rate tuned to a beat subdivision. The organism's metabolic pulse locks to host tempo, producing rhythmic harmonic throbbing without a step sequencer.

### Entropy Sink (Foundation) — Standalone
**DNA:** brightness 0.1, warmth 0.8, movement 0.4, density 0.9, space 0.3, aggression 0.6
**Use:** Enzyme selectivity tuned to subsonic frequencies, isotope balance at 0.0 (maximum subharmonic compression). The organism digests the lowest frequency content and reconstructs dense, dark sub-harmonic structures.

### Cellular Bloom (Aether) — Standalone
**DNA:** brightness 0.5, warmth 0.5, movement 0.7, density 0.6, space 0.8, aggression 0.1
**Use:** Metabolic rate at 0.1Hz (minimum). The organism accumulates energy extremely slowly — a 5-minute performance is a legitimate arc. The 32-mode array fills in over many minutes of continuous feeding, creating a genuinely temporal composition.

---

## Part VII: Sound Design Philosophy

### Work With, Not Against

The central tension for Organon performers is the impulse to control. With every other synthesizer, more control = more predictability = better results. With Organon, the attempt to micromanage the engine fights its fundamental character. The organism is not a modular patch you wire up; it is an entity you feed and observe.

The productive relationship with Organon resembles working with prepared piano or extended technique acoustic instruments: you set conditions, you provide input, and you listen to what emerges. When the organism surprises you — and it will — that surprise is the feature.

### Macro as Environment-Setting

Think of the four macros not as timbre controls but as environmental conditions:

- **CHARACTER** sets the organism's metabolic archetype — is this a slow, massive deep-sea organism or a fast, reactive surface creature?
- **MOVEMENT** controls how aggressively it synthesizes once fed — the catalyst drive and whether multiple voices pulse together or against each other
- **COUPLING** determines dietary preference — what frequency range it can digest, whether it entrains to external rhythm
- **SPACE** controls how much metabolic energy leaks into the shared acoustic environment

The macros define the organism's nature. The performer's notes and coupling connections define the environment. The synthesis emerges from the interaction.

### Coupling as Ecology

Organon is described in the XOceanus aquatic mythology as the chemotroph of the XOceanus ecosystem — an organism that converts the chemical energy of other engines' outputs into harmonic structures. This framing yields a productive design vocabulary:

- **ODDFELIX → Organon**: Percussive hits feed the organism bursts of high-entropy energy. Each drum strike triggers a metabolic spike, creating rhythmic harmonic blooms that decay as the organism starves between hits.
- **ODDOSCAR → Organon**: Lush wavetable pads provide a slow-entropy, consistent feed. The organism builds up dense harmonic content over minutes, mirroring and metabolizing the character of the pad.
- **OVERDUB → Organon**: Delay tails become recycled nutrients. The organism feeds on its acoustic environment in a feedback ecology — the reverb tails of its own output can be routed back as food.
- **ONSET → Organon**: Drum synthesis engine hits create irregular, high-intensity feeding bursts. The XVC (Cross-Voice Coupling) outputs from ONSET create rhythmic metabolic spikes synchronized to the drum pattern.
- **ODYSSEY → Organon (EnvToMorph)**: Odyssey's long JOURNEY envelopes sweep Organon's Isotope Balance over time — the spectral character of the organism co-evolves with a Climax event.

### The Organism That Eats Itself

Route Organon's output (via `getSampleForCoupling()`) back into its own `AudioToFM` input. At moderate catalyst drive, this creates a self-sustaining feedback loop where the organism feeds on its own harmonic reconstruction — a self-referential metabolism that can sustain indefinitely at a fixed energy level. Damping Coefficient controls whether this is stable (high damping, self-limited) or can crescendo (low damping, energy accumulates).

This is the synthesis equivalent of autopoiesis — self-production. Combined with OUROBOROS (the self-consuming recursive feedback engine, Strange Attractor Red `#FF2D2D`), this creates a feedback ecology unique in the XOceanus galaxy.

---

## Part VIII: Long-Term Memory — Vision V006

*Prophesied by Schulze and Buchla, March 2026.*

Organon's VFE metabolism currently adapts within a single performance session. The internal state — `predictedEntropy`, `entropyVariance`, `beliefChangeRate`, `adaptationGain` — resets between plugin loads.

**The vision** (V006): serialize the VFE model state to disk between sessions. Allow the organism to accumulate experience across multiple performances. After 100 hours of playing by a specific musician, Organon's `predictedEntropy` and `entropyVariance` would reflect that musician's specific performance patterns — their tendency toward dense or sparse playing, their preference for high or low entropy input, their characteristic note durations. The organism would respond differently to that musician than to anyone else, because it has learned their musical personality.

No electronic instrument has achieved this. Acoustic instruments develop a relationship with their players over decades — the violin that "opens up," the piano that "knows your touch." These effects emerge from physical modification of materials. Organon offers the first principled computational mechanism for an electronic analog: genuine long-term musical memory via accumulated Bayesian belief updating.

**Current state**: The VFE state is per-session. The architecture fully supports serialization — the relevant state is five float values per voice. No blocking I/O is required on the audio thread; serialization happens on plugin save.

**Implementation direction**: On plugin save, write VFE state to a JSON sidecar alongside the preset. On plugin load, optionally restore the previous VFE state rather than resetting to defaults. Provide a "Reset Memory" button. Default to cumulative accumulation.

**Why this is not yet built**: The design decision — whether musical memory should be per-preset or global-per-player, how to handle multiple players on the same system, how to present the memory state in the UI — has not been resolved. The architecture is ready; the design is pending.

---

## Part IX: Coupling Exports and Integration

### What Organon Sends

`getSampleForCoupling()` returns the post-anabolism, pre-soft-clip mono output from the current block. This carries:

- The summed displacement of all active modes (the synthesized harmonic content)
- Amplitude envelope shaped by the metabolic cycle — the organism's free energy level is embedded in the signal's dynamics

**Useful coupling targets for Organon's output:**
- `AudioToFM` on any engine: Organon's harmonically rich, slowly-evolving output becomes an FM carrier or modulator — the evolved harmonic structure modulates another engine's timbre
- `AudioToWavetable` on OPAL: Organon's output becomes the wavetable input for granular fragmentation — living harmonics become scattered particles
- `AmpToFilter` on any engine: Organon's amplitude envelope (metabolically shaped, not a fixed ADSR) modulates another engine's filter cutoff

### Metabolic State as Coupling Signal

The three VFE readouts are available to the engine layer for future coupling integration:

```cpp
getSurprise()         // Squared prediction error [0, ~1] — spikes when diet changes
getVFE()              // Total variational free energy — high when adapting
getAdaptationGain()   // Settledness [0.2, 1.0] — low when organism is learning
getPredictedEntropy() // Organism's current belief about expected entropy
```

A future coupling route could expose `getSurprise()` as an `AmpToChoke` or `RhythmToBlend` output — the organism's surprise events would trigger or modulate other engines. This would create a genuinely novel signal: not amplitude, not LFO, not envelope, but **metabolic surprise** as a modulation source.

### Coupling Inputs Accepted

| CouplingType | Organon Behavior |
|---|---|
| `AudioToFM` | Audio → ingestion ring buffer (primary feeding mechanism) |
| `AudioToWavetable` | Audio → ingestion ring buffer (alternate feeding path) |
| `RhythmToBlend` | External rhythm → metabolic rate modulation (intake multiplier) |
| `EnvToDecay` | Envelope → metabolic cost modulation (effective rate multiplier) |
| `AmpToFilter` | Amplitude → enzyme selectivity offset (±2kHz) |
| `EnvToMorph` | Envelope → isotope balance offset (±0.3) |
| `LFOToPitch` | LFO → fundamental frequency offset (±10 semitones) |
| `PitchToPitch` | Pitch → fundamental frequency tracking |

---

## Part X: Developer Notes

### CPU Budget

Target: <22% single-engine, <28% dual-engine (Apple Silicon M-series).

The primary cost center is the RK4 integration: 32 modes × 4 stages × 2 state variables per audio sample. At 44.1kHz with 4 active voices, this is 32 × 4 × 2 × 44100 × 4 ≈ 45M float operations per second. With 4-wide SIMD this reduces to ~11M vectorized operations. Modern Apple Silicon handles this comfortably within budget.

Secondary cost: Shannon entropy computation at 2kHz (32-bin histogram over 256 samples, log2 per nonzero bin). This is negligible relative to the RK4 cost.

Denormal protection (`flushDenormal()` on every mode per sample) adds ~1% overhead but prevents 10–100x penalties from subnormal float handling in the modal decay paths.

### Memory Layout

All per-mode arrays use `alignas(16)` for SSE/NEON SIMD alignment:

```cpp
alignas(16) float displacement[32];  // x_n: modal output signal
alignas(16) float velocity[32];       // v_n: rate of change
alignas(16) float angularFrequency[32]; // omega_n (cached, updated on note-on)
alignas(16) float drivingWeight[32];  // F_n (updated at control rate)
```

Contiguous layout means the RK4 inner loop benefits from hardware prefetching: all 32 mode states fit within 512 bytes, well within L1 cache.

### Ingestion Ring Buffer

Each voice has a pre-allocated `ingestionBuffer[2048]` (float, ~8KB). Coupling audio is written by `applyCouplingInput()` with a power-of-2 mask for branchless wrap:

```cpp
voice.ingestionWritePosition = (voice.ingestionWritePosition + 1)
                                & (kIngestionBufferSize - 1);
```

2048 samples provides 46ms of latency headroom at 44.1kHz — sufficient for block-misaligned coupling reads without allocation.

### Parameter Architecture

All 10 parameters use the ParamSnapshot pattern: all `std::atomic<float>*` pointers are read once at the start of `renderBlock()` and cached as `const float` locals. This eliminates per-sample atomic reads and ensures parameter coherence within a block.

### Adding VFE Serialization (V006 Implementation Path)

To serialize VFE state, add to `OrganonVoice`:

```cpp
struct VFECheckpoint {
    float predictedEntropy;
    float entropyVariance;
    float beliefChangeRate;
    float adaptationGain;
    float freeEnergy;
};

VFECheckpoint serializeVFE() const noexcept {
    return { economy.getPredictedEntropy(),
             economy.getEntropyVariance(),
             economy.getBeliefChangeRate(),
             economy.getAdaptationGain(),
             economy.getFreeEnergy() };
}
```

Write to JSON on plugin save; optionally restore on load. The audio thread never touches serialized state directly — all reads/writes happen in the message thread.

---

## Appendix: Glossary

**Active Inference**: The action half of the Free Energy Principle — the system takes actions to make its environment conform to its predictions, rather than only updating its beliefs.

**Adaptation Gain**: Internal variable [0.2, 1.0] representing how settled the organism is. High (near 1.0) when VFE is low; low (near 0.2) when organism is surprised. Modulates catalyst effectiveness and energy intake.

**Catabolism**: The breaking-down stage — Shannon entropy analysis of ingested audio signal. By analogy to cellular catabolism (breaking food into ATP).

**Complexity Penalty**: The Occam's razor term in VFE computation — penalizes rapid belief changes to prevent model instability.

**Damping Coefficient (gamma)**: The dissipation term in the Port-Hamiltonian modal ODE. Controls how quickly mode oscillations decay after driving force is removed.

**Enzyme Selectivity**: The bandpass center frequency of the Ingestion filter — controls which frequency band of audio the organism can digest. High-frequency settings also shorten the entropy analysis window.

**Free Energy**: Internal metabolic reserve [0,1]. Rises when fed high-entropy signal; depletes at a rate set by metabolic rate. Directly controls modal driving force amplitude. NOT a user parameter.

**Informational Dissipative Synthesis**: The synthesis paradigm of Organon — sound emerges from a metabolic cycle of consuming, analyzing, and reconstructing audio information.

**Isotope Balance**: Mode frequency distribution exponent, mapped [0.5, 2.0]. Controls whether the harmonic spectrum is compressed downward, natural, or spread inharmonically upward.

**Membrane Porosity**: Reverb send level — how much metabolic energy leaks into the shared acoustic environment. Modulated by VFE surprise.

**Metabolic Rate**: The rate constant controlling free energy depletion AND the learning rate of the VFE belief model. These are coupled: fast metabolism = fast learning = fast forgetting.

**Modal Array**: 32 damped harmonic oscillators (Port-Hamiltonian formulation) solved with RK4 per audio sample. The anabolism stage — building harmonic structures from metabolic energy.

**Phason Shift**: Temporal offset between multiple voices' metabolic update cycles. Named after quasicrystal phason modes (structural shifts at constant energy). Creates polyrhythmic metabolism.

**Port-Hamiltonian System**: A formulation of dynamical systems that explicitly separates energy-conserving dynamics (J), dissipation (R), and input ports (B). Guarantees passivity without runtime amplitude monitoring.

**Precision**: In Bayesian terms, `1 / variance` — how confident the organism is in its prediction. High precision means small surprises are amplified; low precision means the organism is uncertain and less reactive.

**Prediction Error (Surprise)**: `(observedEntropy - predictedEntropy)²` — the squared difference between what the organism expected and what it observed. The fundamental quantity that VFE minimization seeks to reduce.

**Signal Flux**: Input gain on ingested signal — the primary control for starvation/feeding rate.

**Shannon Entropy**: `H = -Σ p_i · log₂(p_i)`. A measure of information content — how unpredictable a signal is. Maximum for uniform distributions (white noise); minimum for single-valued (DC or sine wave).

**Variational Free Energy (VFE)**: In Friston's framework, a tractable upper bound on the surprise of observations. Minimized by updating beliefs and, in active inference, by taking actions to make observations more predictable.

---

*Document version: 1.0.0 — 2026-03-14*
*Engine: XO-Organon (ORGANON)*
*Blessing B011: VFE metabolism publishable as a paper — all 8 ghost synthesists, unanimous*
*Vision V006: Long-term musical memory — Schulze, Buchla*
