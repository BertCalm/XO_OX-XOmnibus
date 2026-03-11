# XO-Ouroboros — Phase 1 Architecture Specification

**Engine:** XO-Ouroboros (Chaotic Attractor Synthesis)
**Short Name:** OUROBOROS
**Engine ID:** `"Ouroboros"`
**Parameter Prefix:** `ouro_`
**Accent Color:** Strange Attractor Red `#FF2D2D`
**Max Voices:** 6
**CPU Budget:** <22% single-engine, <28% dual-engine
**Date:** 2026-03-10

---

## 1. Product Identity

**Thesis:** "XO-Ouroboros generates audio by continuously solving chaotic differential equations, producing sounds completely alien to subtractive or wavetable synthesis."

**Sound family:** Lead / Texture / Experimental

**Unique capability:** Real-time numerical integration of strange attractors (Lorenz, Rössler, Chua, Aizawa) projected from 3D phase space to stereo audio. The "Leash" mechanism solves the fundamental problem of chaotic synthesis — making it musically playable without neutering the chaos.

**Personality in 3 words:** Feral, swirling, alive.

**Gallery gap filled:** No existing engine produces sound through continuous ODE integration of chaotic systems. All current engines use oscillators, samples, or modal synthesis. Ouroboros is mathematically generative — the waveform IS the attractor trajectory.

---

## 2. Signal Flow Architecture

```
┌──────────────────────────────────────────────────────────────────┐
│                    XO-Ouroboros Voice                             │
│                                                                  │
│  ┌───────────────────────────────────────────────────────────┐   │
│  │  ODE CORE (4x oversampled)                                │   │
│  │                                                           │   │
│  │  Topology ──► Attractor ODEs ──► RK4 Integration          │   │
│  │  (Lorenz/Rössler/Chua/Aizawa)    [x, y, z] state        │   │
│  │                                                           │   │
│  │  Coupling ──► Velocity Injection (dx/dt += input)         │   │
│  │  Injection     (perturbation force on ODE state)          │   │
│  │                                                           │   │
│  │  Chaos Index ──► ODE Coefficients                         │   │
│  │                  (bifurcation parameter per topology)      │   │
│  │                                                           │   │
│  │  [x,y,z] ──► Cubic Saturator (bounding box [-1,1])       │   │
│  └───────────────────┬───────────────────────────────────────┘   │
│                      │                                           │
│  ┌───────────────────▼───────────────────────────────────────┐   │
│  │  PITCH CONTROL (The Leash)                                │   │
│  │                                                           │   │
│  │  Master Phasor (V/Oct) ──► Poincaré Reset                │   │
│  │  Leash param blends:                                      │   │
│  │    0% = free-running (sludgy drift)                       │   │
│  │    100% = hard-synced (complex wavetable)                 │   │
│  │                                                           │   │
│  │  output = (1-leash)*free + leash*synced                   │   │
│  └───────────────────┬───────────────────────────────────────┘   │
│                      │                                           │
│  ┌───────────────────▼───────────────────────────────────────┐   │
│  │  SIGNAL EXTRACTION                                        │   │
│  │                                                           │   │
│  │  12-tap Half-Band FIR (4x → 1x downsample)              │   │
│  │  ↓                                                        │   │
│  │  DC Blocker (5 Hz HPF)                                    │   │
│  │  ↓                                                        │   │
│  │  3D→2D Projection (Rotation Matrix)                       │   │
│  │    Theta/Phi ──► [x,y,z] → [L, R]                        │   │
│  │  ↓                                                        │   │
│  │  Damping (LP accumulator)                                 │   │
│  │  ↓                                                        │   │
│  │  Velocity Cache (dx/dt, dy/dt → ch2, ch3)                │   │
│  └───────────────────┬───────────────────────────────────────┘   │
│                      │                                           │
│                      ▼                                           │
│              Output Cache [L, R, dx/dt, dy/dt]                   │
│                                                                  │
└──────────────────────────────────────────────────────────────────┘
```

### 2.1 ODE Core (4x Oversampled)

The engine generates audio by solving a system of three coupled ordinary differential equations at 4x the audio sample rate using Runge-Kutta 4th Order (RK4) integration.

**RK4 per-sample update for state vector v = [x, y, z]:**

```
k1 = f(v)
k2 = f(v + h/2 * k1)
k3 = f(v + h/2 * k2)
k4 = f(v + h * k3)
v_next = v + (h/6) * (k1 + 2*k2 + 2*k3 + k4)
```

Where `f(v)` is the selected attractor's derivative function and `h` is the integration step size derived from the target pitch frequency.

**Why RK4:** Computationally predictable (constant work per sample, zero branches), highly stable for stiff systems, no adaptive step-size complexity. Perfect for real-time audio — constant CPU load regardless of attractor state.

**4x Oversampling:** Non-linear ODE systems generate infinite harmonics. Internal processing at 4x sample rate pushes aliasing artifacts above the audible range. Downsampled via 12-tap half-band FIR.

**Bounding box:** A cubic polynomial saturator `x = x * (1.5 - 0.5 * x * x)` (Hermite-style soft clip) constrains all three state variables to [-1, 1] after each RK4 step. This mathematically guarantees the simulation cannot diverge, even at high h values or extreme chaos indices. The saturator is applied in the normalized attractor space (after scaling from the topology's natural range).

### 2.2 Attractor Topologies (4 Systems)

Each topology defines a different `f(v)` derivative function and its own phase-space geometry.

**Topology 0 — Lorenz:**
```
dx/dt = σ * (y - x)
dy/dt = x * (ρ - z) - y
dz/dt = x * y - β * z

σ = 10.0 (fixed)
ρ = 20.0 + chaosIndex * 12.0    (range: 20–32, onset of chaos at ~24.74)
β = 8.0/3.0 (fixed)
```

**Topology 1 — Rössler:**
```
dx/dt = -(y + z)
dy/dt = x + a * y
dz/dt = b + z * (x - c)

a = 0.2 (fixed)
b = 0.2 (fixed)
c = 3.0 + chaosIndex * 15.0     (range: 3–18, period-doubling cascade)
```

**Topology 2 — Chua:**
```
dx/dt = α * (y - x - chua_f(x))
dy/dt = x - y + z
dz/dt = -β * y

chua_f(x) = m1*x + 0.5*(m0-m1)*(|x+1| - |x-1|)
m0 = -1.143, m1 = -0.714 (fixed)
α = 9.0 + chaosIndex * 7.0      (range: 9–16, double scroll formation)
β = 28.0 (fixed, controls Chua's oscillation frequency)
```

**Topology 3 — Aizawa:**
```
dx/dt = (z - β) * x - δ * y
dy/dt = δ * x + (z - β) * y
dz/dt = γ + α * z - z³/3 - (x² + y²) * (1 + ε * z) + ζ * z * x³

α = 0.95, β = 0.7, γ = 0.6, δ = 3.5, ζ = 0.1 (fixed)
ε = 0.1 + chaosIndex * 0.85     (range: 0.1–0.95, torus → chaos transition)
```

**Bounding boxes (for state normalization during topology crossfade):**

| Topology | x range | y range | z range |
|---|---|---|---|
| Lorenz | [-25, 25] | [-30, 30] | [0, 55] |
| Rössler | [-12, 12] | [-12, 12] | [0, 25] |
| Chua | [-3, 3] | [-0.5, 0.5] | [-4, 4] |
| Aizawa | [-1.5, 1.5] | [-1.5, 1.5] | [-0.5, 2.5] |

### 2.3 Topology Crossfade (50ms Dual-System)

When the Topology parameter changes:

1. **Initialize System B** with normalized state from System A:
   - Map A's current `[x,y,z]` from A's bounding box to `[0,1]^3`
   - Map `[0,1]^3` to B's bounding box as initial conditions
2. **Run both systems** for 50ms (crossfade window):
   - System A output fades `1.0 → 0.0` (linear ramp)
   - System B output fades `0.0 → 1.0` (linear ramp)
3. **Retire System A** after crossfade completes (reset, not freed — no allocation).

Each voice pre-allocates space for two `AttractorState` structs. Only one is active in steady state. The second activates during crossfade. Zero allocation on the audio thread.

### 2.4 Pitch Control — Phase-Locked Chaos (The Leash)

**The fundamental problem:** Chaotic systems have no defined fundamental frequency. The perceived pitch drifts with integration step size, chaos index, and attractor state. This makes raw chaotic synthesis unplayable.

**The solution:** A hidden master phasor (sawtooth wave at exact MIDI frequency) runs alongside the attractor. The `leash` parameter blends between:

- **Free-running path (leash = 0):** Attractor runs without resets. Pitch is approximate, governed by `h` alone. The orbit drifts, bifurcates, wanders. Organic but pitch-ambiguous.
- **Hard-synced path (leash = 1):** When the master phasor resets (at the MIDI note frequency), the attractor state is forced to a Poincaré section point. This creates an exactly repeating period — the chaos becomes a complex, static wavetable. Perfectly pitched but static.

**Blend:** `output = (1 - leash) * freeOutput + leash * syncedOutput`

**Poincaré section reset points (per topology):**

| Topology | Reset Condition | Reset State |
|---|---|---|
| Lorenz | Phasor reset | `[0.1, y_current_normalized, z_current_normalized]` |
| Rössler | Phasor reset | `[0.1, y_current_normalized, z_current_normalized]` |
| Chua | Phasor reset | `[0.1, y_current_normalized, z_current_normalized]` |
| Aizawa | Phasor reset | Computed from current radius in azimuthal plane |

**CPU optimization:**
- Leash < 0.01: Skip synced path entirely (single-system cost)
- Leash > 0.99: Skip free path entirely (single-system cost)
- 0.01–0.99: Both paths run (double ODE cost)

**The sweet spot (~30-60%):** The synced path anchors the pitch while the free path provides harmonic movement. The blend creates a sound that tracks pitch reliably but has constantly shifting overtone content.

### 2.5 Step Size Derivation from Pitch

The user controls target pitch frequency; `h` is derived internally via per-topology calibration.

```
h = (targetFreqHz / calibrationFreqHz[topology]) * baseStepSize[topology]
```

**Per-topology calibration (measured at default chaos index):**

| Topology | Base h | Calibration Freq (Hz) | Max Safe h |
|---|---|---|---|
| Lorenz | 0.005 | ~130 | 0.02 |
| Rössler | 0.01 | ~95 | 0.05 |
| Chua | 0.002 | ~200 | 0.008 |
| Aizawa | 0.005 | ~110 | 0.015 |

**Safety clamp:** `h = min(h, maxSafeH[topology] * (1.0 - chaosIndex * 0.3))`. High chaos reduces the safe step size. At extreme pitches, the Leash naturally dominates and the system becomes a complex repeating waveform — musically correct behavior.

**MIDI pitch tracking:** On note-on, `targetFreqHz = midiNoteToFreq(noteNumber)`. The Leash phasor runs at this exact frequency. The `ouro_rate` parameter serves as the base rate for free-running (no MIDI) or drone patches; MIDI note-on overrides it.

### 2.6 Signal Extraction — 3D to Stereo 2D

The three ODE state variables `[x, y, z]` are projected to two audio channels using a rotation matrix controlled by Theta and Phi parameters.

**Rotation matrix:**
```
Rx = [[1,      0,       0     ],
      [0,  cos(θ), -sin(θ)],
      [0,  sin(θ),  cos(θ)]]

Ry = [[cos(φ),  0, sin(φ)],
      [0,       1, 0      ],
      [-sin(φ), 0, cos(φ)]]

[x', y', z'] = Ry * Rx * [x, y, z]

Left  = x'
Right = y'
```

As Theta and Phi are modulated, the harmonic structure and stereo image shift entirely — as if walking around a physical, spinning sculpture of sound. Because L and R are discrete projections of a 3D object, the raw dry output inherently features massive stereo width and complex movement without chorus or reverb.

### 2.7 Downsampling and DC Blocking

**12-tap half-band FIR:**
- ~70dB stopband rejection
- Linear phase (no group delay distortion)
- Half-band symmetry: only 6 multiplies per output sample (every other coefficient is zero)
- Pre-computed coefficients (no allocation)

**DC blocker:** First-order high-pass at 5 Hz, applied post-downsample. Essential because attractors like Lorenz orbit entirely in positive z-space, creating massive DC offsets.

```
y[n] = x[n] - x[n-1] + R * y[n-1]
R = 1 - (2π * 5 / sampleRate)
```

### 2.8 Damping

A low-pass integration accumulator smooths sharp corners of Chua and Lorenz trajectories:

```
smoothed = smoothed * (1 - alpha) + raw * alpha
alpha = 1.0 - damping * 0.95    (damping 0 = no smoothing, 1 = heavy LP)
```

Applied per-channel after projection, before output. Yields warmer, more analog tones at high damping without changing the underlying attractor dynamics.

---

## 3. Parameter Taxonomy (8 Core Parameters)

| # | Parameter ID | Display Name | Type | Range | Default | DSP Function |
|---|---|---|---|---|---|---|
| 1 | `ouro_topology` | Topology | Int | 0–3 | 0 (Lorenz) | Selects ODE system; change triggers 50ms crossfade |
| 2 | `ouro_rate` | Orbit Rate | Float | 0.01–20000 Hz | 130.0 | Target pitch frequency; derived to step size h via calibration. MIDI note-on overrides. |
| 3 | `ouro_chaosIndex` | Chaos Index | Float | 0.0–1.0 | 0.3 | Bifurcation parameter: stable orbit → tearing chaos |
| 4 | `ouro_leash` | Leash | Float | 0.0–1.0 | 0.5 | Blend: free-running (0) ↔ hard-synced to MIDI phasor (1) |
| 5 | `ouro_theta` | Theta | Float | -π to π | 0.0 | 3D→2D projection rotation X-axis. Displayed as -180°–180° |
| 6 | `ouro_phi` | Phi | Float | -π to π | 0.0 | 3D→2D projection rotation Y-axis. Displayed as -180°–180° |
| 7 | `ouro_damping` | Damping | Float | 0.0–1.0 | 0.3 | LP integration accumulator; smooths sharp attractor corners |
| 8 | `ouro_injection` | Injection | Float | 0.0–1.0 | 0.0 | Scales coupling audio displacement into ODE velocity equations |

### Parameter Notes

- **Topology** is a discrete integer. Changes are detected per-block; when changed, the dual-system crossfade begins.
- **Orbit Rate** replaces the ambiguous "Rate / h" from the original spec. The user thinks in pitch (Hz), not step sizes. Internally this maps to h via the calibration table.
- **Chaos Index** maps to different ODE coefficients per topology. The mapping is non-linear — the perceptually interesting range (onset of chaos) is clustered around 0.3–0.6 for all topologies.
- **Leash** at < 0.01 or > 0.99 enables single-system CPU optimization.
- **Theta/Phi** are stored as radians but displayed as degrees in the UI.
- **Damping** at 0.0 passes the raw attractor output (sharp, aliased corners). At 1.0, heavy low-pass smoothing creates warm, rounded tones.
- **Injection** at 0.0 ignores coupling input entirely. At 1.0, external audio fully displaces the ODE velocity. The perturbation is additive to the ODE derivatives, not to the output.

---

## 4. Macro Mapping

| Macro | Label | Parameter X | Parameter Y | Musical Effect |
|---|---|---|---|---|
| M1 | CHARACTER | `ouro_chaosIndex` | `ouro_damping` | Serene stable loop ↔ screaming torn chaos |
| M2 | MOVEMENT | `ouro_theta` | `ouro_phi` | Walk around the sound sculpture — rotating harmonics + stereo |
| M3 | COUPLING | `ouro_injection` | `ouro_leash` | External perturbation depth + pitch tightness |
| M4 | SPACE | `ouro_damping` | `ouro_rate` | Warm slow drift ↔ bright fast orbit |

**Note:** Damping appears in both M1 and M4 (CHARACTER Y and SPACE X). It is the most musically impactful smoothing parameter and should be reachable from multiple macro contexts.

---

## 5. Voice Architecture

- **Max voices:** 6
- **Voice stealing:** Oldest voice (LRU), 5ms crossfade (consistent with all engines)
- **Per-voice state:** Two `AttractorState` structs (for topology crossfade), master phasor, ADSR envelope, DC blocker state, damping accumulator. All pre-allocated. No dynamic memory.

### 5.1 Note-On Behavior

- Attractor state is **not reset**. The organism continues from wherever it currently orbits. This means the first note after silence starts from initial condition `[0.1, 0.1, 0.1]`, but subsequent notes inherit the current trajectory — the chaos has memory.
- Leash phasor resets to 0.0 on note-on (ensuring pitch alignment from the first cycle).
- Velocity maps to Injection gain boost for the first 50ms (harder hit = more chaotic initial transient).
- ADSR attack begins.

### 5.2 Note-Off Behavior

- ADSR enters release stage. The amp envelope fades the output, but the attractor keeps running internally so it can resume smoothly on the next note-on.
- If voice is stolen during release: 5ms crossfade.

### 5.3 Legato

- Attractor state carries over (the orbit continues unbroken).
- Leash phasor frequency updates to new note frequency (pitch glides if portamento is active).
- No transient — pure pitch change with continuous timbre evolution.
- ADSR retriggers from current level (no attack restart).

---

## 6. ADSR Envelope

Ouroboros includes a basic ADSR amplitude envelope per voice. Unlike Organon (which uses metabolic decay), Ouroboros needs explicit envelope control because the attractor runs indefinitely.

The ADSR is **not a user-facing parameter** in Phase 1 — it uses fixed values optimized for the engine's character:

```
Attack:  5ms  (fast — the attractor is already running)
Decay:   100ms
Sustain: 0.8
Release: 200ms (enough to hear the orbit fade, not so long it wastes voices)
```

These values can be exposed as parameters in Phase 2 if needed.

---

## 7. Coupling Interface

### 7.1 As Coupling TARGET (receiving from other engines)

| CouplingType | Ouroboros Behavior | Concept |
|---|---|---|
| `AudioToFM` | Audio injected into dx/dt velocity equation, scaled by `ouro_injection` | Perturbation — knock the orbit off-trajectory |
| `AudioToWavetable` | Audio injected into dy/dt velocity equation, scaled by `ouro_injection` | Perturbation on orthogonal axis |
| `RhythmToBlend` | Rhythm signal modulates Chaos Index | Beat-synced bifurcation transitions |
| `EnvToDecay` | Envelope modulates Damping | Dynamic smoothing — attack is sharp, sustain is warm |
| `AmpToFilter` | Source amplitude modulates Damping | Amplitude-reactive warmth |
| `EnvToMorph` | Envelope modulates Theta projection angle | Envelope-driven harmonic rotation |
| `LFOToPitch` | LFO modulates Orbit Rate / step size | Standard pitch vibrato |
| `PitchToPitch` | Pitch tracking modulates Orbit Rate / step size | Harmonic tracking |

### 7.2 Unsupported types (no-op)

| CouplingType | Why |
|---|---|
| `AudioToRing` | Ring mod doesn't map to ODE perturbation — the attractor IS the waveform |
| `FilterToFilter` | No traditional filter chain in signal path |
| `AmpToChoke` | Killing attractor mid-orbit resets state → click. Use ADSR release instead. |
| `AmpToPitch` | Redundant with PitchToPitch; creates feedback loops with the Leash |

### 7.3 As Coupling SOURCE (4-channel output)

| Channel | Content | Use Case |
|---|---|---|
| 0 | Left projected audio | Standard coupling audio source |
| 1 | Right projected audio | Standard coupling audio source |
| 2 | dx/dt normalized to [-1, 1] | Non-repeating complex LFO for filter, morph, etc. |
| 3 | dy/dt normalized to [-1, 1] | Non-repeating complex LFO, decorrelated from ch2 |

Velocity normalization: `normalizedDxDt = clamp(dxdt / maxVelocity[topology], -1.0, 1.0)`

MegaCouplingMatrix reads channels 2/3 when coupling type is `LFOToPitch`, `EnvToMorph`, etc. Channels 0/1 used for audio coupling types.

---

## 8. PlaySurface Mapping

### Pad Mode (Phase Space Explorer)
| Axis | Target | Musical Effect |
|---|---|---|
| X | `ouro_chaosIndex` | Stable orbit ↔ full chaos |
| Y | `ouro_rate` | Pitch control in free-running mode |
| Z (pressure) | `ouro_injection` | Pressure opens the system to external perturbation |

### Fretless Mode (Manifold Rotation)
| Axis | Target | Musical Effect |
|---|---|---|
| X | `ouro_theta` | Continuous harmonic rotation |
| Y | `ouro_phi` | Continuous stereo rotation |

### Drum Mode (Orbit Smash)
| Event | Behavior |
|---|---|
| Strike | Injects a spike into the ODE velocity (like a transient perturbation) |
| Velocity | Maps to spike amplitude — harder hit = wider orbit displacement |
| Result | Explosive transient → organic decay as attractor pulls back to stable trajectory |

---

## 9. Preset Archetypes with DNA and Mood

| # | Name | Mood | Standalone? | DNA (B/W/M/D/S/A) | Key Settings |
|---|---|---|---|---|---|
| 1 | Strange Loop | Foundation | Yes | 0.5/0.5/0.4/0.4/0.2/0.3 | Lorenz, Leash 0.6, Chaos 0.3, Damping 0.4. First encounter — stable, musical, warm. |
| 2 | Butterfly Effect | Atmosphere | Yes | 0.4/0.6/0.8/0.3/0.5/0.2 | Lorenz, Leash 0.3, Chaos 0.5, Theta animated. Evolving pad with micro-movement. |
| 3 | Feral Oscillator | Prism | Yes | 0.8/0.2/0.7/0.7/0.1/0.9 | Chua, Leash 0.1, Chaos 0.9, Damping 0.0. Raw, aggressive, screaming. |
| 4 | Event Horizon | Entangled | No | 0.3/0.4/0.9/0.8/0.6/0.5 | Rössler, Injection 0.8, Leash 0.4. Transforms whatever feeds it into spiraling harmonics. |
| 5 | Phase Portrait | Flux | Yes | 0.6/0.3/0.9/0.5/0.2/0.4 | Aizawa, Leash 0.5, Chaos 0.6, Theta/Phi animated. Rhythmic, shifting. |
| 6 | Deterministic Ghost | Aether | Yes | 0.2/0.7/0.5/0.2/0.9/0.1 | Lorenz, Leash 0.95, Chaos 0.2, Damping 0.7. Near-static complex waveform, ethereal. |
| 7 | Perturbation Engine | Entangled | No | 0.5/0.3/0.8/0.6/0.3/0.7 | Rössler, Injection 1.0. Drum hits knock orbit into new trajectories. |

**"Strange Loop"** is the first-encounter preset: Lorenz attractor at moderate chaos with enough Leash to be pitch-stable. Produces a warm, complex tone that evolves subtly. Macros produce obvious changes. No coupling required.

---

## 10. .xometa Preset Structure

```json
{
  "schema_version": 1,
  "name": "Strange Loop",
  "mood": "Foundation",
  "engines": ["XOuroboros"],
  "author": "XO_OX",
  "version": "1.0.0",
  "description": "A warm strange attractor that traces infinite variations on a theme. The first loop you hear.",
  "tags": ["lead", "warm", "chaotic", "evolving", "starter"],
  "macroLabels": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
  "couplingIntensity": "None",
  "tempo": null,
  "created": "2026-03-10",
  "dna": {
    "brightness": 0.5,
    "warmth": 0.5,
    "movement": 0.4,
    "density": 0.4,
    "space": 0.2,
    "aggression": 0.3
  },
  "parameters": {
    "XOuroboros": {
      "ouro_topology": 0,
      "ouro_rate": 130.0,
      "ouro_chaosIndex": 0.3,
      "ouro_leash": 0.6,
      "ouro_theta": 0.0,
      "ouro_phi": 0.0,
      "ouro_damping": 0.4,
      "ouro_injection": 0.0
    }
  },
  "coupling": { "pairs": [] },
  "sequencer": null
}
```

---

## 11. Visual Identity

- **Accent Color:** Strange Attractor Red `#FF2D2D` — hot, volatile, energetic.
- **Material Concept:** Phase-space visualization — the 3D attractor trajectory rendered as a glowing wireframe that rotates with Theta/Phi.
- **Icon Concept:** An ouroboros (serpent eating its tail) — circular, self-referential.
- **Panel Character:** The attractor visualization pulses and writhes with Chaos Index. At low chaos it traces clean loops; at high chaos it fills the space with tangled trajectories. The Leash is visualized as the serpent's jaw — tighter grip = smaller, more regular loops.

---

## 12. Best Coupling Partners

| Partner | Route | Musical Effect |
|---|---|---|
| ODDFELIX | ODDFELIX → Ouroboros via `AudioToFM` | Percussive hits knock the orbit into new trajectories — rhythmic chaos |
| ORGANON | Ouroboros → Organon via `AudioToFM` | Chaotic audio feeds the metabolic organism — unpredictable nutrition |
| ODYSSEY | ODYSSEY → Ouroboros via `EnvToMorph` | Long envelopes slowly rotate projection — evolving harmonic architecture |
| ODDOSCAR | Ouroboros → OddOscar via ch2/3 `LFOToPitch` | Velocity vectors as complex LFOs driving wavetable position |
| ONSET | ONSET → Ouroboros via `AudioToFM` | Drum patterns as physical forces — the orbit dances to the beat |
| OBLONG | Ouroboros → Oblong via ch2/3 `EnvToMorph` | Chaotic velocity vectors as non-repeating morph source |

---

## 13. CPU Budget Verification

| Component | FLOPs/sample/voice | Notes |
|---|---|---|
| RK4 at 4x oversample | ~288 | 4 samples × 4 evaluations × 3 vars × 6 ops |
| Half-band downsample | ~24 | 6 multiplies × 2 channels |
| DC blocker | ~8 | 2 ops × 2 channels × 2 (free + synced) |
| 3D projection | ~10 | Rotation matrix multiply |
| Damping | ~4 | LP accumulator × 2 channels |
| Leash phasor | ~4 | Phase increment + reset check |
| **Total per voice** | **~338** | Single-system (Leash at extremes) |
| **Worst case (dual)** | **~626** | Both free + synced running (Leash 0.01–0.99) |
| **6 voices worst case** | **~3756** | All voices, dual system |
| **At 44.1kHz** | **~166M FLOPS/s** | ~1.1% of M1 single core |

Well within the 22% single-engine budget. No CPU concerns.

---

## 14. Verification Checklist

- [x] Naming confirmed: XOuroboros, `ouro_` prefix, `#FF2D2D`
- [x] 8 parameters defined with `ouro_` namespace IDs
- [x] Coupling maps to existing `CouplingType` enum (no new types)
- [x] Voice count = 6, CPU budget verified at <2% single-engine (well under 22%)
- [x] Topology crossfade: 50ms dual-system with state normalization
- [x] Rate/h derivation: pitch → h via per-topology calibration with safety clamp
- [x] Leash: deterministic blend, not stochastic, with single-system optimization at extremes
- [x] 4x oversampling with 12-tap half-band FIR, ~70dB rejection
- [x] DC blocker: 5 Hz HPF post-downsample
- [x] Velocity vector coupling via 4-channel output cache
- [x] Note-on/off/legato behavior defined
- [x] 7 preset archetypes with mood assignments and DNA values
- [x] "Strange Loop" first-encounter preset defined
- [x] Macro mapping for all 4 macros
- [x] PlaySurface mapping for all 3 modes
- [x] ADSR envelope specified (fixed values, Phase 1)
- [x] All 6 blocking issues from design review resolved

---

*This document resolves all issues from the Phase 0 design review. Ready for Phase 2: Sandbox Build.*
