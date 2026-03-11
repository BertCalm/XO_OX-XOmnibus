# XO-Organon — Phase 1 Architecture Specification

**Engine:** XO-Organon (Informational Dissipative Synthesis)
**Short Name:** ORGANON
**Engine ID:** `"Organon"`
**Accent Color:** Bioluminescent Cyan `#00CED1`
**Max Voices:** 4 (each voice is an independent "organism")
**CPU Budget:** <22% single-engine, <28% dual-engine
**Date:** 2026-03-10

---

## 1. Product Identity

**Thesis:** "XO-Organon is a metabolic synth that consumes audio signals to grow living harmonic structures."

**Sound family:** Pad / Texture / Hybrid

**Unique capability:** Sound that evolves based on what the engine has been "fed" — no two performances are identical because the engine accumulates internal state from its history of coupling inputs.

**Personality in 3 words:** Alive, hungry, adaptive.

**Gallery gap filled:** No existing engine produces sound through a consumption-synthesis loop. All current engines are reactive (trigger → response). Organon is agential (consume → metabolize → reconstruct).

---

## 2. Signal Flow Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    XO-Organon Voice                          │
│                                                              │
│  ┌──────────────┐   ┌──────────────┐   ┌────────────────┐   │
│  │  INGESTION   │──►│  CATABOLISM   │──►│   ECONOMY      │   │
│  │              │   │              │   │                │   │
│  │ CouplingIn   │   │ Shannon H(P) │   │ Free Energy    │   │
│  │   or         │   │ Spectral PDF │   │ Pool tracking  │   │
│  │ Internal     │   │ @ 2kHz ctrl  │   │ Metabolic Rate │   │
│  │ Noise Source │   │ rate         │   │ state machine  │   │
│  └──────────────┘   └──────┬───────┘   └───────┬────────┘   │
│                            │                    │            │
│                            ▼                    ▼            │
│                     ┌──────────────────────────────────┐     │
│                     │         ANABOLISM                 │     │
│                     │                                  │     │
│                     │  32-Mode Port-Hamiltonian Array   │     │
│                     │  RK4 solver @ audio rate         │     │
│                     │  Mode freqs driven by Economy    │     │
│                     │  Mode amps driven by Catabolism  │     │
│                     └──────────────┬───────────────────┘     │
│                                    │                         │
│                                    ▼                         │
│                     ┌──────────────────────────────────┐     │
│                     │       OUTPUT STAGE               │     │
│                     │  Damping → Soft clip → Cache     │     │
│                     └──────────────────────────────────┘     │
│                                                              │
└──────────────────────────────────────────────────────────────┘
```

### 2.1 Ingestion Stage

The voice samples audio from one of two sources:
- **Coupling input:** Audio received via `applyCouplingInput()` from the MegaCouplingMatrix (stored in a pre-allocated ring buffer).
- **Internal noise substrate:** When no coupling input is active or Signal Flux is 0, a xorshift32 PRNG generates white noise as the "self-feeding" substrate. This ensures the engine always has something to metabolize.

**Standalone mode behavior:** With no coupling partners, Organon feeds on its own internal noise. The noise substrate is shaped by Enzyme Selectivity (bandpass filtering before analysis). This produces a self-sustaining organism that generates evolving harmonic textures from filtered noise — compelling on its own, richer when fed by other engines.

**Ring buffer:** Pre-allocated in `prepare()`, sized to `maxBlockSize * 2`. No audio-thread allocation.

### 2.2 Catabolism Stage (Control Rate — 2000 Hz)

Performs Shannon Entropy analysis on the ingested signal:

**Window:** 256 samples (~5.8ms at 44.1kHz)
**Histogram bins:** 32 (amplitude quantized to 32 levels)
**Update rate:** Every ~22 samples at 44.1kHz (control rate 2000Hz)
**Latency:** ~5.8ms (one window). Acceptable for pad/texture engine; the "Hyper-Metabolic Lead" archetype uses a smaller window of 64 samples (~1.5ms) via the Enzyme Selectivity parameter at high settings.

**Output:** Two control signals updated at 2kHz:
- `entropyValue` (0.0–1.0): Normalized Shannon entropy of the input. High entropy = rich/noisy input = more "nutritious". Low entropy = simple/tonal input = less energy.
- `spectralCentroid` (normalized 0.0–1.0): Center of mass of the histogram. Drives the frequency bias of the modal reconstruction.

### 2.3 Economy Stage (Control Rate — 2000 Hz)

Tracks the organism's internal state via **Active Inference / Variational Free Energy minimization** (see Section 14 for full VFE detail):

- **Free Energy Pool** (`freeEnergy`, 0.0–1.0): Accumulates when `entropyValue` is high (rich input). Depletes at a rate set by Metabolic Rate, modulated by VFE surprise. When pool is full, the organism is "healthy" — harmonics are dense and stable. When depleted, harmonics thin out and become unstable.
  - Intake: `entropyValue * signalFlux * adaptationGain * (1 + externalRhythmMod)`
  - Cost: `effectiveMetabolicRate * 0.1` (where effective rate includes VFE surprise modulation)
  - Accumulation: `freeEnergy += (intake - cost) * dt`, clamped to [0.0, 1.0]
  - On note release: cost increases 4x (starvation)

- **VFE State** (5 internal variables): `predictedEntropy`, `entropyVariance`, `beliefRate`, `surprise`, `adaptationGain` — these drive the organism's adaptive behavior. The organism predicts its input entropy and adjusts when surprised.

- **Free Energy Pool is internal state, NOT a user parameter.** The user observes it via a UI meter. The user influences it indirectly through Metabolic Rate (burn speed) and Signal Flux (intake gain). VFE is fully automatic — the organism learns on its own.

### 2.4 Anabolism Stage (Audio Rate)

A **32-mode Port-Hamiltonian Modal Array** synthesizes the output.

**Mode frequency distribution:**
- Base frequencies are harmonically spaced: `f_n = f_fundamental * n` for modes 1–32.
- `f_fundamental` is set by MIDI note (standard pitch tracking).
- The Isotope Balance parameter shifts the distribution: at 0.0 (Sub), modes cluster in the low register (subharmonic weighting). At 1.0 (Ultra), modes spread into the upper partials.
- The `spectralCentroid` from Catabolism further biases which modes are energized.

**Mode amplitude distribution:**
- Each mode's amplitude is driven by `freeEnergy * catalystDrive * modeWeight[n]`.
- `modeWeight[n]` is derived from the Catabolism stage's spectral analysis — modes near the spectral centroid receive more energy.
- When `freeEnergy` is low, only the fundamental and first few harmonics sustain. When high, the full 32-mode array activates.

**ODE Solver — RK4:**
Each mode is a damped harmonic oscillator with energy injection:

```
dx_n/dt = v_n
dv_n/dt = -omega_n^2 * x_n - gamma * v_n + F_n(t)
```

Where:
- `omega_n = 2 * pi * f_n` (mode angular frequency)
- `gamma` = Damping Coefficient parameter (0.01–0.99)
- `F_n(t)` = driving force from the Economy/Catabolism stages

The RK4 solver steps all 32 modes per sample. With SIMD vectorization (4-wide float), this is 8 SIMD iterations per sample — feasible at <22% CPU for 4 voices.

**Initial conditions:** All modes start at x=0, v=0 (silence). The organism must "eat" before it can produce sound. This is the defining behavior — Organon doesn't make sound on note-on alone; it grows into sound.

**Denormal protection:** `flushDenormal()` applied to all mode states after each RK4 step.

### 2.5 Output Stage

- Sum all 32 mode displacements `x_n` with per-mode amplitude weighting.
- Apply `fastTanh()` soft clipping to prevent output exceeding [-1, 1].
- Apply Damping Coefficient as a global decay envelope on the summed output.
- Cache stereo output (mono duplicated to L/R) in the coupling buffer for `getSampleForCoupling()`.

---

## 3. Parameter Taxonomy (10 Core Parameters)

| # | Parameter ID | Display Name | Type | Range | Default | DSP Function |
|---|---|---|---|---|---|---|
| 1 | `organon_metabolicRate` | Metabolic Rate | Float | 0.1–10.0 Hz | 1.0 | Speed of energy turnover; controls how fast freeEnergy depletes |
| 2 | `organon_enzymeSelect` | Enzyme Selectivity | Float | 20–20000 Hz | 1000 | Center frequency of the bandpass filter on the Ingestion stage |
| 3 | `organon_catalystDrive` | Catalyst Drive | Float | 0.0–2.0 | 0.5 | Gain multiplier on the PHS modal driving force |
| 4 | `organon_dampingCoeff` | Damping | Float | 0.01–0.99 | 0.3 | Damping factor (gamma) in the modal ODE; higher = shorter tail |
| 5 | `organon_signalFlux` | Signal Flux | Float | 0.0–1.0 | 0.5 | Gain of the Ingestion input (coupling or noise) |
| 6 | `organon_phasonShift` | Phason Shift | Float | 0.0–1.0 | 0.0 | Temporal offset between metabolic update cycles; creates pulsing |
| 7 | `organon_isotopeBalance` | Isotope Balance | Float | 0.0–1.0 | 0.5 | Spectral weighting: 0.0 = sub-heavy, 1.0 = upper partials |
| 8 | `organon_lockIn` | Lock-in Strength | Float | 0.0–1.0 | 0.0 | Synchronization strength to SharedTransport tempo |
| 9 | `organon_membrane` | Membrane Porosity | Float | 0.0–1.0 | 0.2 | Wet/dry blend into Space macro (diffusion/reverb send) |
| 10 | `organon_noiseColor` | Noise Color | Float | 0.0–1.0 | 0.5 | Spectral tilt of the internal noise substrate (dark ↔ bright) |

### Parameter notes

- **Free Energy Pool** is NOT a parameter. It is internal state displayed as a read-only meter in the UI.
- **Noise Color** replaces the vague "Sub/Ultra" range from the original doc. It provides direct control over the internal noise substrate's character when in standalone mode, ensuring dry patches sound compelling.
- **Phason Shift** uses normalized 0.0–1.0 range internally, displayed as 0°–360° in the UI. Each voice's metabolic control-rate counter is offset by `phasonShift * controlRateDiv`, creating temporal stagger between voices. At 0.0 all voices update in sync; at 0.5 voices are maximally spread. This produces chorus-like thickening and rhythmic pulsing when multiple voices sound simultaneously.
- **Lock-in Strength** synchronizes the metabolic rate to SharedTransport tempo subdivisions. At 0.0 the metabolic rate is free-running. At 1.0 it quantizes to the nearest beat subdivision (quarter, eighth, sixteenth). The lock-in modulates the metabolic update timing via `getPhaseForDivision()`, creating tempo-synced pulsing without an explicit LFO.
- **Membrane Porosity** controls a reverb/diffusion send level exposed via `getReverbSendLevel()`. The processor reads this value to route Organon's output to the shared reverb bus. At 0.0 the organism is sealed (fully dry). At 1.0 it's fully permeable (maximum reverb send). VFE surprise can modulate this — a surprised organism "sweats" more into the reverb.
- All parameters use the `organon_` namespace prefix.

---

## 4. Macro Mapping

| Macro | Label | Parameter X | Parameter Y | Musical Effect |
|---|---|---|---|---|
| M1 | CHARACTER | `organon_metabolicRate` | `organon_isotopeBalance` | Fast-burning bright creature ↔ slow massive sub entity |
| M2 | MOVEMENT | `organon_catalystDrive` | `organon_phasonShift` | Static organism ↔ breathing, pulsing harmonic bloom |
| M3 | COUPLING | `organon_enzymeSelect` | `organon_lockIn` | Dietary preference + tempo synchronization |
| M4 | SPACE | `organon_membrane` | `organon_dampingCoeff` | Tight/dry ↔ diffuse/reverberant evaporation |

---

## 5. Coupling Interface

### 5.1 Coupling Type Mapping (Existing Enum)

**As coupling TARGET (receiving from other engines):**

| CouplingType | Organon Behavior | Equivalent Concept |
|---|---|---|
| `AudioToFM` | Audio routed into Ingestion ring buffer as "nutrient" | NutrientAudio |
| `AudioToWavetable` | Audio routed into Ingestion ring buffer (alternate path) | NutrientAudio |
| `RhythmToBlend` | Rhythm signal modulates Metabolic Rate for tempo lock-in | HormonalPulse |
| `EnvToDecay` | Envelope modulates Metabolic Rate (speeds up/slows down) | ThermalInertia |
| `AmpToFilter` | Source amplitude modulates Enzyme Selectivity | Diet shift |
| `EnvToMorph` | Envelope modulates Isotope Balance (spectral sweep) | Spectral morph |
| `LFOToPitch` | LFO modulates fundamental frequency of the modal array | Standard pitch mod |
| `PitchToPitch` | Pitch tracking from source engine | Harmonic tracking |

**Unsupported types (no-op in switch):**

| CouplingType | Why |
|---|---|
| `AmpToPitch` | Redundant with PitchToPitch; amplitude-to-pitch creates unwanted feedback |
| `AudioToRing` | Ring mod doesn't fit the metabolic model — Organon reconstructs, doesn't multiply |
| `FilterToFilter` | Organon doesn't use a traditional filter chain |
| `AmpToChoke` | Killing a metabolic organism mid-cycle produces clicks; use EnvToDecay instead |

**As coupling SOURCE (sending to other engines):**

`getSampleForCoupling()` returns the post-anabolism, pre-effects mono output. This signal carries:
- The harmonic content of the 32-mode array (useful for AudioToFM, AudioToWavetable on other engines)
- Amplitude envelope shaped by the metabolic cycle (useful for AmpToFilter, AmpToPitch on other engines)

### 5.2 applyCouplingInput Implementation Sketch

```cpp
void applyCouplingInput(CouplingType type, float amount,
                        const float* sourceBuffer, int numSamples) override
{
    switch (type)
    {
        case CouplingType::AudioToFM:
        case CouplingType::AudioToWavetable:
            // Write source audio into ingestion ring buffer
            writeToIngestionBuffer(sourceBuffer, numSamples, amount);
            couplingAudioActive = true;
            break;

        case CouplingType::RhythmToBlend:
            externalRhythmMod += amount;
            break;

        case CouplingType::EnvToDecay:
            externalDecayMod += amount;
            break;

        case CouplingType::AmpToFilter:
            externalFilterMod += amount;
            break;

        case CouplingType::EnvToMorph:
            externalMorphMod += amount;
            break;

        case CouplingType::LFOToPitch:
        case CouplingType::PitchToPitch:
            externalPitchMod += amount * 0.5f;
            break;

        default:
            break; // Unsupported types silently ignored
    }
}
```

---

## 6. Voice Architecture

- **Max voices:** 4
- **Voice stealing:** Oldest voice (LRU), consistent with SNAP/DUB
- **Legato mode:** Yes — in legato, a new note inherits the existing organism's Free Energy Pool (the organism "migrates" rather than dying and restarting)
- **Note-on behavior:** New voice starts with `freeEnergy = 0.0`. The organism must metabolize input before producing full harmonics. Velocity maps to initial `catalystDrive` boost (higher velocity = faster initial bloom).
- **Note-off behavior:** Metabolic rate increases 4x on release (organism "starves"), causing natural harmonic thinning and decay. Damping Coefficient controls how long this takes.
- **Per-voice state:** Each voice maintains independent `freeEnergy`, mode states (32 x/v pairs), and ingestion history. Voices do NOT share metabolic state.
- **Crossfade on steal:** 5ms crossfade (consistent with SNAP engine).

---

## 7. Standalone Mode

When Organon is the only engine loaded (no coupling partners):

1. **Noise substrate is the food source.** The internal xorshift32 PRNG generates noise, filtered by Enzyme Selectivity and tinted by Noise Color.
2. **Signal Flux controls noise input gain.** At 0.0, the organism starves (silence after decay). At 1.0, it feeds constantly.
3. **The engine is fully playable standalone.** Note-on triggers voice allocation. Noise is consumed. Harmonics bloom based on Catalyst Drive and Free Energy accumulation. Damping controls decay.
4. **CHARACTER macro sweeps** produce audible change: slow/sub ↔ fast/bright organism.
5. **COUPLING macro** (M3) still functions: Enzyme Selectivity shapes the noise diet, Lock-in syncs the metabolic pulse to host tempo.

**Why standalone sounds compelling:** The modal array reconstructing harmonics from filtered noise produces rich, evolving timbres with natural movement — similar to physical modeling of wind instruments, but with the metabolic "breathing" character. No effects needed for interest.

---

## 8. Entropy Analyzer Detail

### Algorithm: Short-Time Histogram Entropy

```
Per control-rate tick (every ~22 samples at 44.1kHz):
1. Read the last 256 samples from the ingestion buffer
2. Quantize each sample to one of 32 amplitude bins
3. Count bin occupancy → histogram H[32]
4. Normalize: p[i] = H[i] / 256
5. Shannon entropy: E = -sum(p[i] * log2(p[i])) for p[i] > 0
6. Normalize: entropyValue = E / log2(32)  // max entropy = log2(bins)
7. Spectral centroid: weighted average of bin indices by occupancy
```

**SIMD optimization:** Steps 2-3 (quantization + counting) use integer SIMD. Steps 4-6 use float SIMD with `fastLog2()` from `FastMath.h`.

**Adaptive window:** When Enzyme Selectivity is above 10kHz (high-frequency diet), the window shrinks to 64 samples for faster response. This enables the "Hyper-Metabolic Lead" archetype.

---

## 9. Port-Hamiltonian Modal Array Detail

### Mathematical Formulation

Each mode `n` (n = 1..32) is a port-Hamiltonian oscillator:

```
State vector: z_n = [x_n, p_n]^T  (displacement, momentum)
Hamiltonian:  H_n = 0.5 * (p_n^2 / m + k_n * x_n^2)
              where k_n = m * omega_n^2, m = 1.0

Dynamics:     dz_n/dt = (J - R) * grad(H_n) + B * u_n

J = [0, 1; -1, 0]        (skew-symmetric: energy conserving)
R = [0, 0; 0, gamma]     (dissipation: energy loss via damping)
B = [0; 1]                (input port)
u_n = F_n(t)              (driving force from catabolism)
```

**Energy-stable quadratization:** The nonlinear driving force `F_n(t)` is quadratized so the RK4 step preserves the passivity of the system. This prevents energy blow-up without clamping.

### Driving Force

```
F_n(t) = catalystDrive * freeEnergy * modeWeight[n] * entropyValue
```

Where `modeWeight[n]` peaks near the spectral centroid and rolls off with distance:

```
modeWeight[n] = exp(-0.5 * ((n/32 - spectralCentroid) / bandwidth)^2)
bandwidth = 0.15 + 0.35 * isotopeBalance  // narrow at sub, wide at ultra
```

### Memory Layout

```cpp
// Contiguous for L1 cache efficiency
struct ModalState {
    alignas(16) float x[32];   // displacements
    alignas(16) float v[32];   // velocities
    alignas(16) float freq[32]; // omega_n values (cached)
    alignas(16) float weight[32]; // modeWeight (updated at control rate)
};
```

All 32 modes stored contiguously, aligned for 4-wide SIMD.

---

## 10. PlaySurface Interaction

### Pad Mode (Metabolic Niche Map)
| Axis | Target Parameter | Musical Effect |
|---|---|---|
| X | `organon_metabolicRate` | Environmental temperature |
| Y | `organon_signalFlux` | Nutrient density |
| Z (pressure) | `organon_catalystDrive` | Physical effort → harmonic bloom |

### Fretless Mode (Hormonal Sweep)
| Axis | Target Parameter | Musical Effect |
|---|---|---|
| X | `organon_phasonShift` | Continuous phase offset → microtonal pulse |
| Y | `organon_isotopeBalance` | Spectral sweep low ↔ high |

### Drum Mode (Impact Catabolism)
| Event | Behavior |
|---|---|
| Strike | Feeds a burst of high-energy noise into the Ingestion buffer |
| Velocity | Maps to noise burst amplitude ("Nutrient Quality") |
| Result | Sudden harmonic bloom → exponential decay as energy is metabolized |

---

## 11. Preset Archetypes with DNA and Mood

| # | Name | Mood | Standalone? | DNA (B/W/M/D/S/A) | Key Settings |
|---|---|---|---|---|---|
| 1 | Warm Start | Foundation | Yes | 0.3/0.7/0.3/0.3/0.2/0.1 | Low metabolic rate, high noise color warmth, gentle catalyst. The "first-encounter" preset. |
| 2 | Hibernation Cycle | Atmosphere | No (needs coupling) | 0.2/0.6/0.5/0.2/0.6/0.1 | Near-zero signal flux standalone; blooms only when fed by another engine. |
| 3 | Hyper-Metabolic Lead | Prism | Yes | 0.8/0.2/0.8/0.7/0.1/0.8 | Max metabolic rate, high catalyst, bright isotope balance, short damping. |
| 4 | Symbiotic Drone | Entangled | No (needs coupling) | 0.4/0.5/0.6/0.8/0.7/0.2 | Signal flux at 1.0, catalyst low, high damping. Pure reconstruction of input. |
| 5 | Pace-of-Life Groove | Flux | Yes | 0.5/0.4/0.9/0.5/0.3/0.4 | Lock-in at 0.7, phason shift active, metabolic rate near host BPM subdivision. |
| 6 | Entropy Sink | Foundation | Yes | 0.1/0.8/0.4/0.9/0.3/0.6 | Enzyme selectivity low (sub diet), high catalyst, isotope balance at 0.0. |
| 7 | Cellular Bloom | Aether | Yes | 0.5/0.5/0.7/0.6/0.8/0.1 | Very low metabolic rate (0.1Hz), slow accumulation over minutes. |

**"Warm Start"** is the first-encounter preset: loads standalone, produces a warm, slowly evolving pad on the first note. No coupling required. Immediate musical result. Macros produce obvious, pleasing changes.

---

## 12. .xometa Preset Structure

```json
{
  "schema_version": 1,
  "name": "Warm Start",
  "mood": "Foundation",
  "engines": ["XOrganon"],
  "author": "XO_OX",
  "version": "1.0.0",
  "description": "A warm, slowly evolving pad that breathes on its own. The first organism you meet.",
  "tags": ["pad", "warm", "evolving", "organic", "starter"],
  "macroLabels": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
  "couplingIntensity": "None",
  "tempo": null,
  "created": "2026-03-10",
  "dna": {
    "brightness": 0.3,
    "warmth": 0.7,
    "movement": 0.3,
    "density": 0.3,
    "space": 0.2,
    "aggression": 0.1
  },
  "parameters": {
    "XOrganon": {
      "organon_metabolicRate": 0.5,
      "organon_enzymeSelect": 800.0,
      "organon_catalystDrive": 0.4,
      "organon_dampingCoeff": 0.15,
      "organon_signalFlux": 0.6,
      "organon_phasonShift": 0.0,
      "organon_isotopeBalance": 0.35,
      "organon_lockIn": 0.0,
      "organon_membrane": 0.2,
      "organon_noiseColor": 0.4
    }
  },
  "coupling": { "pairs": [] },
  "sequencer": null
}
```

---

## 13. Visual Identity

- **Accent Color:** Bioluminescent Cyan `#00CED1` — evokes deep-sea bioluminescence, biological glow, living systems.
- **Material Concept:** Semi-translucent membrane with internal glow — like looking at cells under a microscope.
- **Icon Concept:** A simple cell/organelle shape — circular with an internal nucleus dot.
- **Panel Character:** The engine panel pulses subtly with the metabolic rate. The Free Energy meter glows brighter as the organism feeds. The background shifts from dim to saturated cyan as harmonics bloom.

---

## 14. Active Inference / Variational Free Energy (VFE) — IMPLEMENTED

The MetabolicEconomy now implements full Variational Free Energy minimization via Active Inference. The organism maintains a generative model of its input and adjusts internal state to minimize prediction error.

### VFE Algorithm

```
VFE = precision_weighted_surprise + complexity_penalty
    = (predicted_entropy - observed_entropy)^2 / variance + lambda * |belief_change_rate|^2
```

**Per control tick (2kHz):**

1. **Prediction error (surprise):** `(observed_entropy - predicted_entropy)^2`
2. **Precision weighting:** `1 / (entropy_variance + 0.01)` — confident predictions amplify surprise
3. **VFE computation:** `precision * surprise + 0.1 * belief_rate^2` (complexity penalty stabilizes the organism)
4. **Bayesian belief update:** `predicted_entropy += learning_rate * prediction_error` where `learning_rate = clamp(metabolicRate * 0.05, 0.001, 0.2)`
5. **Variance tracking:** Exponential moving average of squared prediction errors
6. **Adaptation gain:** High VFE → organism adapts faster (fight-or-flight). Low VFE → organism is settled (richer harmonics)

### DSP Consequences

| VFE State | Behavior |
|---|---|
| High surprise | Metabolic rate increases (vfeMod = 1 + surprise * 2), organism burns energy faster, spectral character shifts |
| Low surprise | Organism settles, adaptation gain → 1.0, full catalyst drive applied, stable harmonic output |
| Rapid belief change | Complexity penalty increases VFE, preventing oscillation between states |
| Settled + fed | Richest harmonic output — all 32 modes fully energized with stable spectral distribution |

### VFE Readouts (Available for Coupling + UI)

- `getSurprise()` — squared prediction error (0.0 = predicted, high = shocked)
- `getVFE()` — total variational free energy
- `getAdaptationGain()` — how settled the organism is (1.0 = stable, 0.2 = adapting)
- `getPredictedEntropy()` — the organism's current belief about expected input entropy

### VFE Modulation Points in DSP

- **Metabolic rate:** `effectiveRate *= (1.0 + surprise * 2.0)` — surprised organisms burn faster
- **Isotope balance:** `effectiveIsotope += surprise * 0.15` — surprise shifts spectral character
- **Catalyst drive:** `effectiveCatalyst *= adaptationGain` — settled organisms get full drive
- **Stereo spread:** `spread = surprise * 0.15` — surprise widens the stereo image
- **Energy intake:** `intake *= adaptationGain` — settled organisms extract more nutrition

---

## 15. Best Coupling Partners

| Partner Engine | Coupling Route | Musical Effect |
|---|---|---|
| SNAP | SNAP → Organon via `AudioToFM` | Percussive hits "feed" the organism bursts of energy, creating rhythmic harmonic blooms |
| MORPH | MORPH → Organon via `AudioToWavetable` | Lush wavetable pads become the "diet" — Organon reconstructs them as metabolized harmonics |
| DRIFT | DRIFT → Organon via `EnvToMorph` | Drift's long envelopes sweep Organon's Isotope Balance — spectral co-evolution |
| DUB | DUB → Organon via `AudioToFM` | Dub's delay tails become recycled nutrients — feedback ecology |
| ONSET | ONSET → Organon via `AudioToFM` | Drum hits trigger metabolic spikes — organic percussion-to-pad coupling |

---

## 16. Verification Criteria

Before advancing to Phase 2 (Sandbox Build):

- [ ] All 10 parameters defined with `organon_` namespace IDs
- [ ] Coupling maps to existing `CouplingType` enum (no new types required)
- [ ] Voice count = 4, CPU budget <22% single
- [ ] Standalone mode fully specified (internal noise substrate)
- [ ] Accent color assigned: `#00CED1`
- [ ] Free Energy Pool clarified as internal state (not a parameter)
- [ ] Port-Hamiltonian modal array: 32 modes, RK4, SIMD-aligned memory
- [ ] Entropy analyzer: 256-sample window, 32 bins, 2kHz control rate
- [ ] 7 preset archetypes with mood assignments and DNA values
- [ ] "Warm Start" first-encounter preset defined for standalone play
- [ ] Macro mapping verified: all 4 macros produce audible change
- [ ] PlaySurface mapping for all 3 modes

---

*This document resolves all 5 blocking issues and all 5 should-fix items identified in the design review. Ready for Phase 2: Sandbox Build.*
