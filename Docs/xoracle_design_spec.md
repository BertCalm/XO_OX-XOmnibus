# XO-Oracle — Phase 1 Architecture Specification

**Engine:** XO-Oracle (Stochastic GENDY + Maqam Microtonal Synthesis)
**Short Name:** ORACLE
**Engine ID:** `"Oracle"`
**Parameter Prefix:** `oracle_`
**Accent Color:** Prophecy Indigo `#4B0082`
**Max Voices:** 8
**CPU Budget:** <10% single-engine, <16% dual-engine
**Date:** 2026-03-11

---

## 1. Product Identity

**Thesis:** "XO-Oracle constructs waveforms from probability — using Iannis Xenakis's GENDY algorithm to build each cycle from stochastic breakpoints, constrained by mirror barriers and shaped by maqam microtonal intelligence."

**Sound family:** Texture / Lead / Experimental / Drone

**Unique capability:** Every other synthesizer engine uses deterministic oscillators — given identical parameters, they produce identical output. ORACLE generates waveforms through genuine stochastic processes: random walks in time and amplitude that construct each waveform cycle from scratch. No two cycles are identical. Combined with a maqam-aware microtonal tuning system, ORACLE produces sounds that are simultaneously ancient (maqam is centuries old) and genuinely alien (GENDY has never been commercialized).

**Personality in 3 words:** Prophetic, unstable, ancient.

**Gallery gap filled:** No existing engine uses stochastic waveform construction. All engines — even experimental ones like OUROBOROS (chaotic but deterministic ODE) and ORGANON (metabolic but state-determined) — produce output that is mathematically determined by their inputs. ORACLE introduces *genuine randomness* as a creative force. The waveform is not calculated; it is *composed by chance within constraints*.

---

## 2. Signal Flow Architecture

```
┌──────────────────────────────────────────────────────────────────────────┐
│                         XO-Oracle Voice                                    │
│                                                                           │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │  MAQAM TUNING SYSTEM                                                │  │
│  │                                                                     │  │
│  │  MIDI Note ──► Maqam Lookup ──► Microtonal Frequency               │  │
│  │                    │                                                │  │
│  │  Mode: Rast / Bayati / Saba / Hijaz / Sikah / Nahawand / etc.     │  │
│  │  Context: Ascending / Descending (different intervals)              │  │
│  │  Jins detection: automatic tetrachord transition                    │  │
│  └──────────────────────────────┬──────────────────────────────────────┘  │
│                                  │ Target frequency                       │
│  ┌──────────────────────────────▼──────────────────────────────────────┐  │
│  │  GENDY BREAKPOINT ENGINE                                            │  │
│  │                                                                     │  │
│  │  N Breakpoints (8–32) define one waveform cycle                     │  │
│  │  Each breakpoint: (time_offset, amplitude)                          │  │
│  │                                                                     │  │
│  │  Per cycle:                                                         │  │
│  │    For each breakpoint b:                                           │  │
│  │      b.time += random_walk(time_distribution, time_step_size)       │  │
│  │      b.amplitude += random_walk(amp_distribution, amp_step_size)    │  │
│  │      apply_mirror_barriers(b)                                       │  │
│  │                                                                     │  │
│  │  Time total normalized to target period (from Maqam frequency)      │  │
│  │  Breakpoints interpolated via cubic Hermite spline                  │  │
│  └──────────────────────────────┬──────────────────────────────────────┘  │
│                                  │                                        │
│  ┌──────────────────────────────▼──────────────────────────────────────┐  │
│  │  WAVEFORM INTERPOLATION                                             │  │
│  │                                                                     │  │
│  │  Cubic Hermite interpolation between breakpoints                    │  │
│  │  Audio-rate readout at Maqam-adjusted frequency                     │  │
│  │  Breakpoints updated once per waveform cycle                        │  │
│  │                                                                     │  │
│  │  Output: continuously evolving waveform                             │  │
│  └──────────────────────────────┬──────────────────────────────────────┘  │
│                                  │                                        │
│  ┌──────────────────────────────▼──────────────────────────────────────┐  │
│  │  POST-PROCESSING                                                    │  │
│  │                                                                     │  │
│  │  DC Blocker (5 Hz HPF) ──► Soft Limiter ──► Coupling Output         │  │
│  │                                                                     │  │
│  │  Coupling input: external audio perturbs breakpoint amplitudes      │  │
│  └──────────────────────────────┬──────────────────────────────────────┘  │
│                                  │                                        │
│                                  ▼                                        │
│                        Output Cache [L, R]                                │
│                                                                           │
└──────────────────────────────────────────────────────────────────────────┘
```

### 2.1 GENDY Breakpoint Engine — The Core

GENDY (GENération DYnamique) was invented by Iannis Xenakis in 1991 as part of his lifelong project to apply mathematical formalism to musical composition. The algorithm constructs waveforms from N breakpoints — discrete (time, amplitude) pairs that define the shape of one waveform cycle. Each cycle, every breakpoint undergoes a random walk: its time offset and amplitude are perturbed by a random step drawn from a configurable probability distribution.

**Breakpoint structure:**

```cpp
struct GENDYBreakpoint
{
    float timeOffset;    // Relative time position within cycle [0, 1]
    float amplitude;     // Amplitude at this point [-1, 1]
};
```

**Per-cycle update:**

```cpp
void updateBreakpoints(GENDYBreakpoint* bp, int N, float timeStep, float ampStep,
                       Distribution timeDist, Distribution ampDist)
{
    for (int i = 0; i < N; ++i)
    {
        // Random walk in time
        float dt = sampleDistribution(timeDist) * timeStep;
        bp[i].timeOffset += dt;

        // Random walk in amplitude
        float da = sampleDistribution(ampDist) * ampStep;
        bp[i].amplitude += da;

        // Mirror barriers — elastic reflection at boundaries
        bp[i].timeOffset = mirrorBarrier(bp[i].timeOffset, 0.0f, 1.0f);
        bp[i].amplitude = mirrorBarrier(bp[i].amplitude, -1.0f, 1.0f);
    }

    // Normalize time offsets to maintain cycle period
    sortByTime(bp, N);
    normalizeTimeSpan(bp, N);
}
```

**Mirror barriers:** When a breakpoint's random walk pushes it past a boundary (amplitude beyond [-1, 1] or time beyond [0, 1]), the value is *reflected* back — like a billiard ball bouncing off a wall. This ensures breakpoints stay within valid ranges while maintaining the momentum of the random walk. The barrier elasticity is parameterized: hard barriers (sharp reflection) vs. soft barriers (gradual deceleration near boundaries).

**Probability distributions:** Two distributions are available for the random walks:

1. **Cauchy distribution:** Heavy-tailed — most steps are small, but occasional large jumps occur. Produces waveforms that are mostly stable with sudden dramatic changes. *The sound of prophecy — calm surface, sudden revelation.*

2. **Logistic distribution:** Medium-tailed — smoother than Cauchy, more predictable evolution. Produces waveforms that change continuously and gradually. *The sound of flowing water — constant gentle motion.*

A morphable blend between these distributions is the engine's primary timbre control.

### 2.2 Waveform Interpolation

The N breakpoints define the skeleton of the waveform. Cubic Hermite spline interpolation fills in the samples between breakpoints, producing a smooth, band-limited waveform:

```cpp
float interpolate(float phase, const GENDYBreakpoint* bp, int N)
{
    // Find surrounding breakpoints
    int i = findSegment(phase, bp, N);
    int j = (i + 1) % N;

    // Hermite basis
    float t = (phase - bp[i].timeOffset) / (bp[j].timeOffset - bp[i].timeOffset);
    float t2 = t * t;
    float t3 = t2 * t;

    // Cubic Hermite interpolation
    float h00 = 2*t3 - 3*t2 + 1;
    float h10 = t3 - 2*t2 + t;
    float h01 = -2*t3 + 3*t2;
    float h11 = t3 - t2;

    // Tangents estimated from neighboring breakpoints
    float m0 = estimateTangent(bp, N, i);
    float m1 = estimateTangent(bp, N, j);

    return h00 * bp[i].amplitude + h10 * m0
         + h01 * bp[j].amplitude + h11 * m1;
}
```

Cubic Hermite interpolation is chosen over linear interpolation because:
- It produces C1-continuous waveforms (continuous first derivative), reducing high-frequency content
- It matches the smooth, organic quality of the GENDY aesthetic
- It adds negligible CPU cost (4 multiplies, 6 adds per sample)

### 2.3 Maqam Tuning System

The maqam system replaces Western 12-tone equal temperament with a context-dependent microtonal tuning system. Maqam is not a scale — it is a set of behavioral rules:

**Jins (tetrachord) structure:** Each maqam is built from two jins (tetrachords) — a lower jins and an upper jins. The intervals within each jins are fixed for that maqam, but the *transition* between lower and upper jins can involve microtonal inflections.

**Ascending vs. descending intervals:** In many maqamat (plural of maqam), the intervals differ depending on whether the melody is ascending or descending. For example, in Maqam Rast:
- Ascending: C - D - E♭↑ - F (where ♭↑ means a quarter-tone above E♭)
- Descending: F - E♭ - D - C (E♭ without the quarter-tone raise)

This context-dependent tuning is implemented as a lookup table that takes (MIDI note, ascending/descending flag) and returns a frequency in Hz.

**Supported Maqamat:**

| Maqam | Lower Jins | Upper Jins | Character |
|-------|-----------|-----------|-----------|
| Rast | Rast (W, W, ½W, W) | Rast | Neutral, foundational |
| Bayati | Bayati (½W, ¾W, W, W) | Nahawand | Melancholic, introspective |
| Saba | Saba (½W, ¾W, ½W, W) | Hijaz | Dark, dramatic |
| Hijaz | Hijaz (½W, 1½W, ½W, W) | Rast | Bright, exotic |
| Sikah | Sikah (¾W, W, W, ½W) | Sikah | Ethereal, floating |
| Nahawand | Nahawand (W, ½W, W, W) | Hijaz | Western minor-adjacent |
| Kurd | Kurd (½W, W, W, W) | Nahawand | Dark, stable |
| Ajam | Ajam (W, W, ½W, W) | Ajam | Western major-adjacent |

(W = whole tone, ½W = semitone, ¾W = three-quarter tone, 1½W = augmented second)

**Equal temperament override:** When maqam mode is disabled, ORACLE reverts to standard 12-TET tuning.

---

## 3. Parameter Taxonomy

### 3.1 Core Parameters (8)

| ID | Parameter | Range | Curve | Rate | Description |
|----|-----------|-------|-------|------|-------------|
| `oracle_breakpoints` | Breakpoint Count | 8–32 | Stepped | Control | Number of GENDY breakpoints per waveform cycle. Low = simple waveforms. High = complex, noisy. CHARACTER macro target. |
| `oracle_timeStep` | Time Step Size | 0.0–1.0 | Exponential | Control | Maximum random walk step for breakpoint time offsets. Higher = more rhythmic instability. |
| `oracle_ampStep` | Amplitude Step Size | 0.0–1.0 | Exponential | Control | Maximum random walk step for breakpoint amplitudes. Higher = more timbral instability. MOVEMENT macro target. |
| `oracle_distribution` | Distribution Morph | 0.0–1.0 | Linear | Control | 0 = Cauchy (heavy-tailed, dramatic jumps). 1 = Logistic (smooth, gradual). Continuous blend between distributions. |
| `oracle_barrierElasticity` | Barrier Elasticity | 0.0–1.0 | Linear | Control | Mirror barrier hardness. 0 = hard reflection (sharp, aggressive). 1 = soft deceleration (smooth, organic). |
| `oracle_maqam` | Maqam Mode | 0–8 | Stepped | Control | 0 = 12-TET. 1–8 = Rast, Bayati, Saba, Hijaz, Sikah, Nahawand, Kurd, Ajam. |
| `oracle_gravity` | Maqam Gravity | 0.0–1.0 | Linear | Control | Strength of microtonal pull toward maqam-correct intervals. 0 = free (chromatic). 1 = strict maqam adherence. COUPLING macro target. |
| `oracle_drift` | Stochastic Drift | 0.0–1.0 | Exponential | Control | Overall rate of waveform evolution. 0 = frozen (breakpoints don't walk). 1 = maximum evolution speed. SPACE macro target. |

### 3.2 Macro Mapping

| Macro | Primary Target | Secondary Target | Musical Effect |
|-------|---------------|-----------------|----------------|
| CHARACTER (M1) | `oracle_breakpoints` | `oracle_distribution` | Simple, smooth waveforms → complex, jumpy stochastic forms |
| MOVEMENT (M2) | `oracle_ampStep` | `oracle_timeStep` | Frozen waveform → maximally evolving stochastic surface |
| COUPLING (M3) | `oracle_gravity` | Coupling input gain | Free chromatic → strict maqam adherence + external influence |
| SPACE (M4) | `oracle_drift` | `oracle_barrierElasticity` + reverb send | Still, dry → evolving, elastic, reverberant |

### 3.3 Envelope & Modulation Parameters

| ID | Parameter | Type | Description |
|----|-----------|------|-------------|
| `oracle_ampAttack` | Amp Attack | Time | 0ms–10s |
| `oracle_ampDecay` | Amp Decay | Time | 0ms–10s |
| `oracle_ampSustain` | Amp Sustain | Level | 0–1 |
| `oracle_ampRelease` | Amp Release | Time | 0ms–20s |
| `oracle_stochEnvAttack` | Stochastic Env Attack | Time | Controls stochastic intensity envelope |
| `oracle_stochEnvDecay` | Stochastic Env Decay | Time | |
| `oracle_stochEnvSustain` | Stochastic Env Sustain | Level | |
| `oracle_stochEnvRelease` | Stochastic Env Release | Time | |
| `oracle_lfo1Rate` | LFO 1 Rate | Hz | 0.01–30 Hz |
| `oracle_lfo1Depth` | LFO 1 Depth | Level | |
| `oracle_lfo1Shape` | LFO 1 Shape | Enum | Sine / Triangle / Saw / Square / S&H |
| `oracle_lfo2Rate` | LFO 2 Rate | Hz | 0.01–30 Hz |
| `oracle_lfo2Depth` | LFO 2 Depth | Level | |
| `oracle_lfo2Shape` | LFO 2 Shape | Enum | |

### 3.4 Voice Parameters

| ID | Parameter | Description |
|----|-----------|-------------|
| `oracle_voiceMode` | Voice Mode | Mono / Legato / Poly4 / Poly8 |
| `oracle_glide` | Glide Time | Portamento (0–2s). In maqam mode, glide follows maqam-correct pitch paths. |

---

## 4. The Ghosts in ORACLE

### Ghost 1: The UPIC (Iannis Xenakis, 1977) — The Drawn Sound

**The instrument:** At CEMAMu (Centre d'Études de Mathématique et Automatique Musicales) in Paris, Xenakis created the Unité Polyagogique Informatique du CEMAMu — a large electromagnetic drawing tablet connected to a PDP-11 minicomputer. Composers drew graphical curves on the tablet using a stylus; the system interpreted these drawings as pitch-time functions and synthesized the corresponding sound. Xenakis, who trained as an engineer under Le Corbusier, believed that visual form and sonic form were fundamentally equivalent — the same mathematical structures could produce architecture, visual art, and music.

The UPIC was the bridge between Xenakis's graphic scores (like *Metastasis*, 1954, whose string glissandi were drawn as ruled surfaces) and his later computational work (including GENDY). It demonstrated that waveforms could be *drawn* rather than calculated — composed as shapes rather than equations.

**How it lives in ORACLE:** The GENDY algorithm is Xenakis's final evolution of the UPIC concept. Where UPIC required the composer to draw every curve by hand, GENDY delegates the drawing to probability distributions — the algorithm "draws" the waveform through constrained random walks, with the composer controlling the character of the randomness rather than each individual shape. ORACLE is the UPIC that draws itself.

### Ghost 2: The Bazantar (Mark Deutsch, 2000) — The Sympathetic Cloud

**The instrument:** Mark Deutsch, a San Diego bassist, spent a decade designing the Bazantar — a modified 5-string acoustic bass fitted with 29 sympathetic drone strings running beneath the fingerboard and 4 bass drone strings. When the main strings are bowed, the sympathetic strings resonate in complex microtonal relationships — the instrument produces a cloud of overtones that extends far beyond the played note, creating what Deutsch calls "a one-man orchestra of drones."

The Bazantar's sympathetic strings are tuned to specific intervals from the Indian raga system — the instrument was designed to bridge Western bass technique with Indian microtonal aesthetics. It is perhaps the only acoustic instrument specifically engineered to produce the kind of microtonal resonance clouds that maqam systems describe theoretically.

**How it lives in ORACLE:** The Bazantar proves that microtonal sympathetic resonance produces extraordinary musical results in the hands of a skilled performer. ORACLE's maqam system creates a similar effect digitally — the "gravity" parameter pulls the stochastic waveform toward maqam-correct microtonal intervals, just as the Bazantar's sympathetic strings pull bowed notes toward sympathetic resonance. The Bazantar is acoustic proof that ORACLE's concept works.

### Ghost 3: The Ondes Martenot Diffuseurs (Maurice Martenot, 1928) — The Resonant Speakers

**The instrument:** The Ondes Martenot itself is well known — an early electronic instrument with a continuous-pitch ribbon controller. Less known are its three specialized speakers (diffuseurs), which Martenot considered integral to the instrument:

- **Diffuseur Principal (D1):** A conventional loudspeaker for direct sound.
- **Diffuseur Palme (D2):** A speaker whose cone was strung with metal springs and wires, creating sympathetic metallic resonance around the electronic tone. The palme produced halos of harmonics that made the electronic signal sound organic and alive.
- **Diffuseur Métallique (D3):** A speaker firing directly into a suspended metal gong, exciting it sympathetically. The gong's resonance colored the electronic tone with metallic, shimmering overtones.

The diffuseurs transformed a simple electronic oscillator into a rich, living sound through physical sympathetic resonance — acoustic post-processing that added complexity no electronic circuit of the era could produce.

**How it lives in ORACLE:** The diffuseurs are the acoustic ancestor of ORACLE's mirror barriers. Just as the palme and métallique speakers added resonant complexity by reflecting the electronic signal through physical resonant structures, ORACLE's mirror barriers add stochastic complexity by reflecting the breakpoint random walks through elastic boundaries. The barriers don't just constrain — they *contribute to the sound* by creating reflection patterns that add structure to the randomness.

### Ghost 4: Halim El-Dabh's Wire Recorder Experiments (1944) — The First Concrete

**The instrument:** In 1944, four years before Pierre Schaeffer's *Études de bruits* (1948, traditionally credited as the first musique concrète), Egyptian-American composer Halim El-Dabh recorded a zaar (healing/exorcism) ceremony at a street in Cairo using a wire recorder at the Middle East Radio studios. He then manipulated the recordings using the station's facilities: reverberation, voltage-controlled filtering, and re-recording at different speeds. The result, *The Expression of Zaar* (also called *Wire Recorder Piece*), is arguably the first piece of electroacoustic music — and it is almost entirely unknown in Western electronic music history.

El-Dabh went on to compose for the Columbia-Princeton Electronic Music Center (alongside Milton Babbitt and Vladimir Ussachevsky), created one of the earliest pieces for the Buchla 100 modular synthesizer, and composed the sound and light show for the Great Pyramids of Giza. Despite this extraordinary career, he is barely mentioned in standard histories of electronic music — a casualty of the field's Euro-American bias.

**How it lives in ORACLE:** El-Dabh's wire recorder experiments are proof that the maqam aesthetic sensibility — the cultural context in which he was immersed — produces radical electronic music when given access to technology. His manipulation of the zaar recordings used the same constrained-randomness principle that GENDY formalizes mathematically: take a complex source (the ceremony), apply transformations that introduce controlled unpredictability (speed changes, filtering), and constrain the result within cultural parameters (the zaar's ritual structure). ORACLE is the engine El-Dabh would have built if he'd had a DSP workstation instead of a wire recorder.

---

## 5. The Cultural Lens: Maqam

### The Modal System of the Arab World

Maqam (مقام, plural: maqamat) is the modal system underlying Arabic, Turkish, Persian, and Central Asian classical music — a framework for melodic organization that has been refined over centuries. Unlike Western scales (which are fixed pitch collections), maqam is a *behavioral system*: it defines not just which pitches are used but *how they are approached, departed, and ornamented*.

**Key principles:**

**Sayr (path):** Every maqam has a prescribed melodic path — a conventional direction of exploration. A performer in Maqam Bayati begins in the lower jins, establishes the tonic, explores upward to the upper jins, reaches a climax in the upper register, and then descends back to the tonic. This path is not rigid — a skilled performer departs from it creatively — but it provides the gravitational framework within which improvisation occurs.

This maps directly to GENDY's constrained random walks. The breakpoints "explore" the waveform space with random steps, but the mirror barriers and distribution parameters create a *sayr* — a conventional range within which the randomness operates. Low drift + hard barriers = strict sayr. High drift + soft barriers = free improvisation.

**Tarab (ecstasy):** The highest goal of maqam performance is *tarab* — a state of collective ecstasy where performer and audience achieve emotional unity through the music. Tarab occurs not from individual notes but from the *accumulation* of microtonal inflections, ornamental patterns, and emotional trajectory over extended improvisation (taqsim).

This maps to GENDY's temporal evolution. Over time, the accumulating random walks create a trajectory — the waveform has a history, a direction, a sense of emotional motion that emerges from the statistics of many small random steps. A long ORACLE note doesn't just sustain; it *journeys*.

**Quarter-tones and microtonal inflection:** Maqam uses intervals smaller than the Western semitone — most commonly quarter-tones (approximately 50 cents), though the exact tuning varies between regional traditions (the Turkish system uses 53 commas per octave; the Arab system uses 24 quarter-tones). These microtonal intervals are not ornamental — they are structural. Removing them destroys the maqam's identity.

ORACLE's maqam tuning system implements these microtonal intervals precisely, using lookup tables derived from historical tuning treatises and adjusted for compatibility with 12-TET when blended via the `oracle_gravity` parameter.

---

## 6. XOmnibus Integration

### 6.1 MegaCouplingMatrix Compatibility

**Emits:**
- `STOCHASTIC_FIELD` — The current GENDY breakpoint amplitudes as a normalized array. Other engines can use this as a modulation source with genuine randomness.
- `MAQAM_GRAVITY` — Current pitch deviation from 12-TET (in cents) as a continuous modulation signal. Other engines can use this to apply maqam-aware microtonal inflection to their own pitch.
- `WALK_VELOCITY` — The average magnitude of breakpoint random walk steps as a control signal (0 = frozen, 1 = maximum evolution). Indicates the engine's current level of stochastic activity.

**Accepts:**
- `AudioToFM` — External audio perturbs breakpoint amplitudes. The external signal literally reshapes the stochastic waveform.
- `AmpToFilter` — External amplitude modulates mirror barrier positions. Loud = wide barriers (more waveform range), quiet = narrow barriers (constrained, tonal).
- `EnvToMorph` — External envelope drives the distribution morph (Cauchy ↔ Logistic).
- `PITCH_GRAVITY` (from XOntara) — Topological pitch gravity modulates maqam gravity parameter, creating microtonal cross-reference between two different tuning systems.

### 6.2 PlaySurface Interaction Model

**Pad Mode:**
- X-axis: `oracle_distribution` — Cauchy (left) to Logistic (right)
- Y-axis: `oracle_ampStep` — Stochastic intensity
- Pressure (Z): `oracle_drift` — Pressing harder increases evolution speed

**Fretless Mode (Primary — designed for taqsim improvisation):**
- X-axis: Continuous pitch (following maqam tuning when active)
- Y-axis: `oracle_gravity` — Slide up = stricter maqam adherence
- Pressure (Z): `oracle_breakpoints` — Pressing harder increases breakpoint count (complexity)

**Drum Mode:**
- X-axis: Pad assignment by maqam mode (8 pads = 8 maqamat)
- Y-axis: `oracle_ampStep` — Vertical position controls stochastic intensity
- Pressure (Z): Velocity = breakpoint count

---

## 7. Preset Archetypes

### 7.1 The Prophecy
`breakpoints=12, timeStep=0.15, ampStep=0.2, distribution=0.3 (Cauchy-biased), barrier=0.4, maqam=0 (12-TET), gravity=0.0, drift=0.3`

Slowly evolving stochastic texture with occasional Cauchy-distributed dramatic jumps. The waveform breathes and shifts unpredictably, with sudden momentary changes that resolve back into the general flow. Sounds like a prophecy being spoken — mostly calm, punctuated by revelation.

### 7.2 Taqsim
`breakpoints=16, timeStep=0.1, ampStep=0.15, distribution=0.7 (Logistic-biased), barrier=0.6, maqam=2 (Bayati), gravity=0.8, drift=0.4`

Smooth stochastic evolution within Maqam Bayati's microtonal framework. The logistic distribution creates gradual, flowing change. High maqam gravity pulls pitch toward Bayati's characteristic quarter-tone intervals. Designed for fretless PlaySurface improvisation — the digital equivalent of an oud taqsim.

### 7.3 Wire Recording
`breakpoints=24, timeStep=0.3, ampStep=0.4, distribution=0.1 (heavy Cauchy), barrier=0.2, maqam=0 (12-TET), gravity=0.0, drift=0.7`

Aggressive stochastic waveform with heavy Cauchy tails — frequent dramatic jumps in both time and amplitude. Hard barriers create sharp reflections. High drift ensures constant evolution. Evokes the crackling, unstable quality of Halim El-Dabh's wire recorder manipulations.

### 7.4 Ancient Future
`breakpoints=20, timeStep=0.2, ampStep=0.25, distribution=0.5 (balanced), barrier=0.5, maqam=4 (Hijaz), gravity=0.6, drift=0.5`

Balanced stochastic evolution within Maqam Hijaz — the maqam most recognizable to Western ears (the augmented second interval). Medium gravity allows microtonal inflection while maintaining accessibility. Equal distribution blend creates a sound that feels simultaneously ancient (Hijaz intervals) and alien (GENDY waveforms).

### 7.5 Zaar Ceremony
`breakpoints=28, timeStep=0.35, ampStep=0.45, distribution=0.2 (Cauchy-heavy), barrier=0.3, maqam=3 (Saba), gravity=0.9, drift=0.8`

Maximum stochastic intensity within the dark, dramatic Maqam Saba. Heavy Cauchy distribution creates frequent dramatic breakpoint jumps — the waveform convulses and transforms. Very high maqam gravity creates strong microtonal pull, as if the randomness is trying to escape but the maqam keeps pulling it back into ritual structure. Named for El-Dabh's source material.

---

## 8. CPU Analysis

### 8.1 Per-Cycle Cost (Breakpoint Update)

| Component | Operations per Cycle | Notes |
|-----------|---------------------|-------|
| Random number generation (N breakpoints × 2) | N × 4 ops | xorshift64 PRNG + distribution shaping |
| Mirror barrier reflection | N × 6 ops | Conditional reflection per breakpoint |
| Sort breakpoints by time | N × log(N) × 4 ops | Insertion sort (nearly sorted, efficient) |
| Time normalization | N × 2 ops | Division + accumulate |
| **Total per cycle** | **~40N ops** | At N=32: ~1280 ops per cycle |

At A4 (440 Hz), that's 440 cycles/sec × 1280 ops = ~563,000 ops/sec — negligible.

### 8.2 Per-Sample Cost (Interpolation)

| Component | Operations per Sample | Notes |
|-----------|----------------------|-------|
| Segment finding | ~4 ops | Binary search in sorted breakpoint array |
| Cubic Hermite interpolation | 10 multiply + 8 add | Standard Hermite basis |
| DC blocker | 2 multiply + 2 add | First-order IIR |
| Soft limiter | 1 tanh approx | Rational approximation |
| **Total per sample** | **~26 ops** | |

### 8.3 Voice Budget

At 44.1kHz, 8 voices:
- Per-voice: ~26 ops/sample × 44100 = ~1.15M ops/sec + ~563K ops/sec (breakpoint updates) = ~1.71M ops/sec
- 8 voices: ~13.7M ops/sec
- M1 single-core throughput: ~3.2 GFLOPS
- **CPU usage: ~0.4%** per engine instance

Even with maqam lookup overhead and full modulation, ORACLE should not exceed **10%** of a single core. This makes it the second-lightest engine in V3 after OBSIDIAN.

### 8.4 Memory

- Per-voice breakpoint arrays: 32 × 2 × 4 bytes = 256 bytes
- Maqam tuning tables (8 maqamat × 128 notes × 4 bytes): 4 KB (shared)
- PRNG state: 8 bytes per voice
- **Total: ~6 KB** — negligible

---

## 9. Implementation Notes

### 9.1 Anti-Aliasing

GENDY waveforms can contain high-frequency content, especially with many breakpoints and high amplitude step sizes. Mitigation:

1. **Cubic Hermite interpolation** (already in the design) produces C1-continuous waveforms, naturally rolling off high-frequency content.
2. **Breakpoint smoothing:** When `oracle_breakpoints` > 20, apply a simple low-pass to the breakpoint amplitudes before interpolation (average each breakpoint's amplitude with its neighbors).
3. **2× oversampling:** Enabled when `oracle_breakpoints` > 24 AND `oracle_ampStep` > 0.7 (extreme settings). Adds ~50% cost but keeps aliasing inaudible.

### 9.2 PRNG Quality

The PRNG must produce statistically independent sequences for each breakpoint's time and amplitude walks. A single xorshift64 state per voice is seeded from the note number and velocity at note-on, ensuring:
- Different notes produce different stochastic evolution
- The same note at the same velocity produces reproducible (but not identical) evolution across sessions (seeded PRNG)
- No two voices share PRNG state

### 9.3 Thread Safety

- Breakpoint arrays pre-allocated per voice in `prepare()`. No audio-thread allocation.
- Maqam tuning tables loaded once, read-only on audio thread.
- ParamSnapshot pattern for all parameter reads.

### 9.4 Denormal Protection

Breakpoint amplitudes are clamped to `[-1.0f + 1e-7f, 1.0f - 1e-7f]` to prevent denormals in the mirror barrier reflection calculations. DC blocker state flushed if magnitude < 1e-15f.

---

*Architecture spec owner: XO_OX Designs | Engine: ORACLE | Next action: Phase 1 — GENDY Core + Parameter Architecture*
