# Aquatic FX Suite — Design Specification

**Status:** V1.1 scope — design + skeleton phase (implementation sessions follow)
**Prefix:** `aqua_` for all parameters
**Location:** `Source/DSP/Effects/AquaticFXSuite.h`
**Integration point:** `Source/Core/MasterFXChain.h` — inserted as a named sub-chain
**Macro targets:** CHARACTER (M1), MOVEMENT (M3), COUPLING (M4), SPACE (M2)

---

## Overview

The Aquatic FX Suite is a 6-stage post-mix signal chain inspired by the physics of the ocean water
column. Each stage maps to one layer of the XO_OX aquatic mythology: pressure, currents, tides,
reef architecture, the air-water surface boundary, and bioluminescence. Together they form the
signature XO_OX master effects chain — distinct from the existing 22-stage MasterFXChain, which
handles broader creative processing. The Aquatic Suite is positioned BEFORE the MasterFXChain
brickwall limiter and AFTER the existing bus compressor (stage 19).

Signal order within the suite:

```
Input → [1] Fathom → [2] Drift → [3] Tide → [4] Reef → [5] Surface → [6] Biolume → Output
```

All 6 stages have independent mix controls and bypass at zero CPU when mix = 0.

---

## Stage 1 — Fathom (Pressure Compression)

**Metaphor:** Abyssal pressure — the ocean compresses everything uniformly at depth.
**Macro:** CHARACTER (M1) — maps `aqua_fathom_character` to band-gain balance.

### Parameters

| ID | Range | Default | Description |
|----|-------|---------|-------------|
| `aqua_fathom_mix` | 0.0 – 1.0 | 0.0 | Wet/dry blend |
| `aqua_fathom_depth` | 0.0 – 1.0 | 0.5 | OTT compression intensity |
| `aqua_fathom_lowGain` | 0.0 – 2.0 | 1.0 | Low-band gain trim |
| `aqua_fathom_midGain` | 0.0 – 2.0 | 1.0 | Mid-band gain trim |
| `aqua_fathom_highGain` | 0.0 – 2.0 | 1.0 | High-band gain trim |
| `aqua_fathom_lowXover` | 60 – 800 Hz | 200.0 | Low/mid crossover |
| `aqua_fathom_highXover` | 1000 – 12000 Hz | 3000.0 | Mid/high crossover |
| `aqua_fathom_character` | 0.0 – 1.0 | 0.5 | M1 macro: 0 = low-boosted (deep/dark), 1 = high-boosted (bright/present) |

### DSP Algorithm

Fathom reuses the existing `MultibandCompressor` class with Aquatic-specific parameter mapping.

1. Split signal into 3 bands via Linkwitz-Riley crossovers at `lowXover` and `highXover`.
2. Apply upward + downward compression per band (OTT style) at `depth`.
3. Apply per-band gain trim. The `character` macro morphs the band-gain balance:
   - character = 0.0: `lowGain *= 1.4`, `highGain *= 0.7` (deep ocean dark pressure)
   - character = 1.0: `lowGain *= 0.7`, `highGain *= 1.4` (surface-bright airy pressure)
   - character is lerped continuously; user band trims are applied additively on top.
4. Sum bands and apply `mix` blend against dry signal.

**Existing DSP to reuse:** `MultibandCompressor` — instantiate directly; the Fathom stage
is a thin parameter-mapping wrapper around it.

**New code needed:** Character macro morph (4 lines of lerp before calling `setBandGains()`).

---

## Stage 2 — Drift (Brownian Chorus)

**Metaphor:** Deep ocean currents — slow, unpredictable lateral movement that never repeats.
**Macros:** SPACE (M2) + MOVEMENT (M3) — rate and depth are dual-macro targets.

### Parameters

| ID | Range | Default | Description |
|----|-------|---------|-------------|
| `aqua_drift_mix` | 0.0 – 1.0 | 0.0 | Wet/dry blend |
| `aqua_drift_voices` | 2 – 6 (int) | 3 | Number of chorus voices |
| `aqua_drift_depth` | 0.0 – 40.0 ms | 8.0 | Maximum modulation depth (delay swing) |
| `aqua_drift_rate` | 0.001 – 2.0 Hz | 0.07 | Brownian walk speed (base rate) |
| `aqua_drift_diffusion` | 0.0 – 1.0 | 0.4 | How much the walk deviates per step |
| `aqua_drift_feedback` | 0.0 – 0.7 | 0.0 | Feedback amount (> 0 → flanger-ish shimmer) |
| `aqua_drift_stereoSpread` | 0.0 – 1.0 | 0.8 | Phase offset between L and R per voice |
| `aqua_drift_space` | 0.0 – 1.0 | 0.5 | M2 macro: 0 = narrow mono, 1 = wide stereo field |
| `aqua_drift_movement` | 0.0 – 1.0 | 0.5 | M3 macro: 0 = glacial drift, 1 = turbulent churn |

### DSP Algorithm

The existing `MasterModulation` has a `Drift` mode but it uses a simple random-walk LFO on a
single delay line. Fathom Drift needs a proper Brownian multi-voice chorus.

**New implementation required.** Algorithm:

1. Maintain N delay lines (N = `voices`), each with its own Brownian walk state `(position, velocity)`.
2. Per sample, each voice's LFO advances: `velocity += randf() * diffusion * dt; velocity *= 0.999f; position += velocity`. This is an Ornstein-Uhlenbeck process — random walk with mean reversion.
3. Clamp `position` to `[-depth/2, +depth/2]`. Convert to samples: `delaySamples = basedelay + position * sr / 1000`.
4. Read from each delay line with linear interpolation at `delaySamples`.
5. Write input + `feedback * output` back into the delay line (HP-filtered feedback at 80 Hz to prevent mud — reuse the same 1-pole HPF pattern from `DubDelay`).
6. Sum all voices, normalize by `1/sqrt(N)`.
7. Spread voices across the stereo field using per-voice phase offsets driven by `stereoSpread`.
8. SPACE macro: scales `stereoSpread` (0 = all voices same position, 1 = maximum spread).
9. MOVEMENT macro: scales `rate` and `diffusion` simultaneously (0 → rate=0.001, diff=0.05; 1 → rate=2.0, diff=0.8).

**Denormal protection:** Add `1e-25f` DC offset to all delay line feedback paths (standard pattern used elsewhere in the codebase).

**Existing DSP to partially reuse:** Interpolation pattern from `MasterModulation`; HP feedback pattern from `DubDelay`.

---

## Stage 3 — Tide (Tremolo / Auto-Filter)

**Metaphor:** Tidal rhythm — cyclic rise and fall, the ocean breathing.
**Macro:** MOVEMENT (M3) — rate of the tidal LFO.

### Parameters

| ID | Range | Default | Description |
|----|-------|---------|-------------|
| `aqua_tide_mix` | 0.0 – 1.0 | 0.0 | Wet/dry blend |
| `aqua_tide_mode` | 0 = Tremolo, 1 = AutoFilter, 2 = Both | 0 | Processing mode |
| `aqua_tide_rate` | 0.01 – 8.0 Hz | 0.5 | LFO rate |
| `aqua_tide_depth` | 0.0 – 1.0 | 0.6 | LFO modulation depth |
| `aqua_tide_shape` | 0 = Sine, 1 = Triangle, 2 = S&H | 0 | LFO waveform |
| `aqua_tide_syncEnabled` | 0/1 | 0 | Tempo-sync LFO |
| `aqua_tide_syncDiv` | 0–7 (int) | 3 | Sync division: 1/32 1/16 1/8 1/4 1/2 1 2 4 bars |
| `aqua_tide_filterFreq` | 100 – 8000 Hz | 800.0 | Auto-filter center (mode 1/2) |
| `aqua_tide_filterRes` | 0.0 – 0.95 | 0.3 | Filter resonance (mode 1/2) |
| `aqua_tide_stereoPhase` | 0.0 – 180.0 deg | 0.0 | Phase offset L vs R (0 = sync, 90 = quadrature) |
| `aqua_tide_movement` | 0.0 – 1.0 | 0.5 | M3 macro: scales rate 0.01→8 Hz |

### DSP Algorithm

1. **LFO:** Computed per block, not per sample. Sine: `phase += 2π * rate / sr * numSamples`; advance and clamp to `[0, 2π]`. S&H: hold random value for `1/rate` seconds, then sample new value with linear slew (10ms).
2. **Stereo phase offset:** L and R compute their LFO values at `phase` and `phase + stereoPhase * π/180` respectively.
3. **Tremolo mode:** Amplitude modulation. `gain = 1.0 - depth * 0.5 * (1.0 + lfoValue)` — this sweeps gain from 1.0 down to `(1-depth)`, never to zero (preserves minimum 5% floor to avoid clicks).
4. **Auto-filter mode:** State-variable filter (SVF) with cutoff modulated by LFO. `cutoff = filterFreq * pow(2.0, depth * 2.0 * lfoValue)` — exponential sweep of ±2 octaves around center. Use a 2-pole SVF (same topology as Cytomic TPT for the Surface stage; see Stage 5 notes).
5. **Both mode:** Apply tremolo first, then auto-filter in series.
6. **Tempo sync:** When `syncEnabled`, replace `rate` with `bpm / 60.0 * syncDivisionMultiplier`. Division table: `{1/32, 1/16, 1/8, 1/4, 1/2, 1, 2, 4}`.
7. MOVEMENT macro directly maps `aqua_tide_movement` → `aqua_tide_rate` via `rate = 0.01 * pow(800.0, movement)`.

**Existing DSP to reuse:** `MasterModulation` has LFO infrastructure to reference. The SVF for auto-filter mode should be a new 2-pole TPT SVF (shared with Stage 5 Surface).

---

## Stage 4 — Reef (Householder FDN Early Reflections)

**Metaphor:** Coral reef architecture — dense, irregular surfaces scatter sound into complex early reflections.
**Macro:** SPACE (M2) — room scale and reflection density.

### Parameters

| ID | Range | Default | Description |
|----|-------|---------|-------------|
| `aqua_reef_mix` | 0.0 – 1.0 | 0.0 | Wet/dry blend |
| `aqua_reef_size` | 0.0 – 1.0 | 0.5 | Room scale: maps delay matrix lengths |
| `aqua_reef_decay` | 0.0 – 1.0 | 0.4 | Feedback coefficient (0 = single bounce, 1 = long tail) |
| `aqua_reef_damping` | 0.0 – 1.0 | 0.5 | High-frequency damping per reflection |
| `aqua_reef_density` | 0.0 – 1.0 | 0.6 | Early reflection density (controls allpass diffusion) |
| `aqua_reef_predelay` | 0 – 50 ms | 8.0 | Pre-delay before first reflection |
| `aqua_reef_width` | 0.0 – 1.0 | 0.8 | Stereo decorrelation width |
| `aqua_reef_space` | 0.0 – 1.0 | 0.5 | M2 macro: 0 = tight room, 1 = vast cavern |

### DSP Algorithm

Reef is a **4-channel Feedback Delay Network (FDN)** with a Householder mixing matrix. This
produces dense early reflections without the colored resonance of comb-based reverbs.

**Why Householder:** The Householder matrix `H = I - 2/N * ones_matrix` guarantees energy
preservation and uniform eigenvalue distribution. For N=4: `H[i][j] = (i==j ? 1 : 0) - 0.5`.
This means: `out[i] = in[i] - 0.5 * sum(in)`. No per-coefficient tuning required.

**Implementation:**

1. Pre-delay line: ring buffer of `predelay * sr / 1000` samples per channel.
2. Four delay lines with lengths derived from prime-number offsets scaled by `size`:
   - Base lengths (at 44.1kHz): `{1237, 1601, 1913, 2237}` samples (prime numbers, chosen to minimize common harmonics). Scale linearly with `sampleRate / 44100.0` and `size` in range `[0.3, 3.0]`.
3. Per sample:
   a. Read from each delay line at its current position.
   b. Apply 1-pole LP damping: `state = state + dampCoeff * (in - state)` where `dampCoeff = 1 - damping * 0.7`.
   c. Apply Householder mix: `mixed[i] = delayed[i] - 0.5 * sum(delayed)`.
   d. Apply feedback coefficient `decay` (clamped to 0.95): write `input + decay * mixed[i]` into delay line.
4. Allpass diffusion: chain 2 series allpass filters per channel (`density` controls allpass coefficient g ∈ [0.1, 0.6]).
5. Stereo output: mix channels 0,2 → L; channels 1,3 → R; apply M/S width via `width`.
6. M2 macro `space`: morphs `size` from 0.1→1.0 and `decay` from 0.1→0.8.

**Denormal protection:** Add `1e-25f` to all delay line writes.

**Existing DSP to reuse:** Delay line write/read pattern from `LushReverb`; allpass structure from `LushReverb`. The FDN mixing matrix is new.

---

## Stage 5 — Surface (Cytomic SVF High-Shelf Sweep)

**Metaphor:** The air-water boundary — the surface tension that separates the aquatic from the aerial world. High frequencies bleed across the surface differently depending on angle/coupling.
**Macro:** COUPLING (M4) — sweep position controlled by coupling amount from other engines.

### Parameters

| ID | Range | Default | Description |
|----|-------|---------|-------------|
| `aqua_surface_mix` | 0.0 – 1.0 | 0.0 | Wet/dry blend |
| `aqua_surface_freq` | 200 – 16000 Hz | 4000.0 | Shelf pivot frequency |
| `aqua_surface_gain` | -18 – +18 dB | 0.0 | Shelf gain (boost/cut above pivot) |
| `aqua_surface_resonance` | 0.0 – 1.0 | 0.3 | SVF resonance (0 = smooth shelf, 1 = resonant peak at shelf) |
| `aqua_surface_mode` | 0 = HighShelf, 1 = LowShelf, 2 = Bell | 0 | Filter topology |
| `aqua_surface_sweep` | 0.0 – 1.0 | 0.5 | Sweep position: maps to freq range |
| `aqua_surface_sweepRange` | 0 = Narrow (1 oct), 1 = Wide (4 oct) | 1 | Sweep range |
| `aqua_surface_coupling` | 0.0 – 1.0 | 0.0 | M4 macro: coupling amount drives sweep position |

### DSP Algorithm

Surface uses a **Cytomic-style Topology-Preserving Transform (TPT) State Variable Filter** — the
same mathematical foundation used in Ableton's filters, iZotope Ozone, and the Vital synth.

**TPT SVF structure** (Andy Simper / Cytomic 2012):

```
g = tan(π * fc / sr)
k = 2 * resonance  // where resonance ∈ [0, 1] maps to [0.02, 2.0]
a1 = 1.0 / (1.0 + g * (g + k))
a2 = g * a1
a3 = g * a2

// Per sample:
v3 = input - ic2
v1 = a1 * ic1 + a2 * v3
v2 = ic2 + a2 * ic1 + a3 * v3
ic1 = 2 * v1 - ic1
ic2 = 2 * v2 - ic2

// Outputs:
HP = v3 - k * v1 - v2
BP = v1
LP = v2
Notch = HP + LP
Bell = input + (2 * resonance_gain - k) * v1  // requires separate gain param
HighShelf = HP + sqrt(gain_linear) * BP + gain_linear * LP
LowShelf = gain_linear * (HP + sqrt(gain_linear) * BP) + LP
```

**Sweep:** `freq = baseFreq * pow(2.0, (sweep - 0.5) * sweepOctaves)` where `sweepOctaves` = 1 (Narrow) or 4 (Wide). The `coupling` macro maps linearly to `sweep`.

**Smoothing:** `freq` and `gain` are 1-pole smoothed at 20ms time constant to prevent zipper noise from COUPLING modulation. Use the standard one-pole: `smoothed += smoothCoeff * (target - smoothed)`.

**Existing DSP to reuse:** None directly — the TPT SVF is a new implementation. However the
smoothing pattern is used throughout the codebase. This SVF should also be shared with Stage 3
(Tide's auto-filter mode) as a reusable inner struct.

---

## Stage 6 — Biolume (Shimmer Saturation)

**Metaphor:** Bioluminescence — creatures that generate light through chemical reactions. The audio
equivalent: high-frequency shimmer emerging from the signal's own energy via non-linear excitation.
**Macro:** CHARACTER (M1) — shimmer intensity and harmonic brightness.

### Parameters

| ID | Range | Default | Description |
|----|-------|---------|-------------|
| `aqua_biolume_mix` | 0.0 – 1.0 | 0.0 | Wet/dry blend |
| `aqua_biolume_threshold` | -60 – 0 dB | -18.0 | Excitation threshold |
| `aqua_biolume_drive` | 0.0 – 1.0 | 0.4 | Rectifier drive (pre-saturation gain) |
| `aqua_biolume_hpFreq` | 2000 – 12000 Hz | 4000.0 | HP filter frequency on rectified signal |
| `aqua_biolume_shimmer` | 0.0 – 1.0 | 0.5 | Parallel blend of shimmer layer |
| `aqua_biolume_satMode` | 0 = Soft, 1 = Hard, 2 = Fold | 0 | Saturation curve on shimmer layer |
| `aqua_biolume_spread` | 0.0 – 1.0 | 0.6 | Stereo spread of shimmer (decorrelate L/R) |
| `aqua_biolume_character` | 0.0 – 1.0 | 0.5 | M1 macro: 0 = subtle glow, 1 = full bioluminescent burst |

### DSP Algorithm

Biolume generates a shimmer layer through **half-wave rectification + high-pass filtering +
soft saturation**, then adds it in parallel with the dry signal.

**Signal flow:**

1. **Full-wave rectification:** `rect = |input|` — this produces even harmonics (2nd, 4th).
2. **Gate:** Only process if `level_dB(input) > threshold`. Below threshold, shimmer output = 0 (prevents noise floor excitation). Use the same fast dB detection as `MultibandCompressor`.
3. **Drive:** `rect *= dbToGain(drive * 24.0)` — up to +24dB pre-gain.
4. **HP filter:** 2-pole HP at `hpFreq` — removes low-frequency content from the rectified signal so only the shimmer frequency band is added back. Use the same 1-pole cascade HP as the existing crossover in `MultibandCompressor` (HP = input - LP).
5. **Saturation:**
   - Soft: `tanh(rect)` (use `fastTanh` from `FastMath.h` if available)
   - Hard: `clamp(rect, -1, 1)`
   - Fold: `foldback(rect, 1.0)` — `if (rect > 1.0) rect = 2.0 - rect; if (rect < -1.0) rect = -2.0 - rect`
6. **Stereo spread:** L shimmer uses rectified L; R shimmer uses rectified R. Apply an allpass phase rotation on one channel: `y = g * x + state; state = x - g * y` with g ≈ 0.7 for decorrelation.
7. **Shimmer mix:** `output = dry + shimmer * shimmerLayer`. This is additive, not substitutive — bioluminescence adds energy, doesn't replace.
8. **CHARACTER macro:** scales `drive` (0→0.1, 1→1.0) and `shimmer` (0→0.1, 1→0.9) simultaneously.

**Existing DSP to reuse:** Fast math helpers from `FastMath.h`; rectifier + HP pattern is new but minimal; saturation curves borrowed from `Saturator.h` topology.

---

## Macro Summary

| Macro | Stage | Parameter Controlled |
|-------|-------|---------------------|
| CHARACTER (M1) | Fathom | `aqua_fathom_character` — band-gain balance dark↔bright |
| CHARACTER (M1) | Biolume | `aqua_biolume_character` — shimmer drive + blend |
| SPACE (M2) | Drift | `aqua_drift_space` — stereo spread width |
| SPACE (M2) | Reef | `aqua_reef_space` — room size + decay |
| MOVEMENT (M3) | Drift | `aqua_drift_movement` — Brownian walk speed |
| MOVEMENT (M3) | Tide | `aqua_tide_movement` — tidal LFO rate |
| COUPLING (M4) | Surface | `aqua_surface_coupling` — high-shelf sweep position |

The suite intentionally has two stages per macro (except COUPLING, which is singular). This
creates macro "depth" — turning CHARACTER past 0.5 saturates both Fathom's tonal balance AND
Biolume's shimmer simultaneously, producing the characteristic XO_OX layered response.

---

## Integration into MasterFXChain

The Aquatic FX Suite is instantiated as a single member of `MasterFXChain`:

```cpp
AquaticFXSuite aquaticFX;
```

It is called in `MasterFXChain::processBlock()` after stage 19 (Bus Compressor) and before
stage 20 (Brickwall Limiter):

```
... → Bus Compressor → [AquaticFXSuite] → Brickwall Limiter → DC Blocker
```

Parameters are registered in `XOceanusProcessor::createParameterLayout()` alongside existing
master FX parameters, using the `aqua_` prefix throughout.

The suite's `prepare(sampleRate, samplesPerBlock)` is called from `MasterFXChain::prepare()`.
Its `processBlock(L, R, numSamples, bpm)` receives raw pointers and BPM (needed for Tide sync).

---

## Implementation Checklist (for build sessions)

- [ ] Implement TPT SVF inner struct (shared by Tide + Surface)
- [ ] Implement Fathom: MultibandCompressor wrapper + character morph
- [ ] Implement Drift: Brownian multi-voice chorus (Ornstein-Uhlenbeck LFOs)
- [ ] Implement Tide: tremolo + SVF auto-filter + tempo sync
- [ ] Implement Reef: 4-channel Householder FDN
- [ ] Implement Surface: TPT SVF high-shelf + COUPLING macro sweep
- [ ] Implement Biolume: rectifier + HP + saturation shimmer layer
- [ ] Register all `aqua_` parameters in `createParameterLayout()`
- [ ] Cache `aqua_` param pointers in `MasterFXChain::cacheParameterPointers()`
- [ ] Add `aquaticFX.prepare()` call in `MasterFXChain::prepare()`
- [ ] Wire `aquaticFX.processBlock()` into `MasterFXChain::processBlock()`
- [ ] Add Aquatic Suite section to MasterFXSequencer target enum
- [ ] Write unit isolation test (processBlock with silence = silence out)
- [ ] auval regression pass after integration
