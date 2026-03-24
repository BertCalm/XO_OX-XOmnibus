# XO-Obscura — Phase 1 Architecture Specification

**Engine:** XO-Obscura (Scanned Synthesis)
**Short Name:** OBSCURA
**Engine ID:** `"Obscura"`
**Parameter Prefix:** `obscura_`
**Accent Color:** Daguerreotype Silver `#8A9BA8`
**Max Voices:** 8
**CPU Budget:** <12% single-engine, <18% dual-engine
**Date:** 2026-03-11

---

## 1. Product Identity

**Thesis:** "XO-Obscura simulates a vibrating chain of masses and springs, then reads its shape as a waveform — the physical simulation IS the oscillator. Sound from Newtonian mechanics."

**Sound family:** Pad / Texture / Lead / Cinematic

**Unique capability:** A 1D mass-spring chain (128 masses) vibrates under Newtonian physics — each mass connected to its neighbors by springs with configurable stiffness, damping, and nonlinearity. An audio-rate scanner sweeps across the chain, reading the displacement of each mass as it passes. The resulting waveform is never static: because the chain is constantly vibrating and evolving under physical law, the output breathes, ripples, and transforms continuously without any modulation source. *The physics is the modulation.*

**Personality in 3 words:** Physical, evolving, cinematic.

**Gallery gap filled:** XOntara models sympathetic resonance (modal synthesis). ORGANON models biological metabolism. OBSCURA models something different: the *shape* of a vibrating object, read as a wavetable in real time. The physical simulation and the oscillator are the same thing — there is no separation between the model and the sound source. This produces timbres that evolve with the organic unpredictability of physical systems while remaining musically pitched and controllable.

---

## 2. Signal Flow Architecture

```
┌──────────────────────────────────────────────────────────────────────────┐
│                        XO-Obscura Voice                                   │
│                                                                          │
│  ┌─────────────────────────────────────────────────────────────────────┐ │
│  │  EXCITATION STAGE                                                   │ │
│  │                                                                     │ │
│  │  Impulse ──────────┐                                                │ │
│  │  Continuous Force ──┤──► Excitation Mixer ──► Force Input            │ │
│  │  Coupling Input ────┘                                               │ │
│  │                                                                     │ │
│  │  Excitation Position: where on the chain force is applied           │ │
│  │  Excitation Width: how many masses receive force (1 = pluck, N/2)   │ │
│  └──────────────────────────────────┬──────────────────────────────────┘ │
│                                      │                                   │
│  ┌──────────────────────────────────▼──────────────────────────────────┐ │
│  │  MASS-SPRING CHAIN (128 masses, Verlet integration)                 │ │
│  │                                                                     │ │
│  │  For each mass i (0 to N-1):                                        │ │
│  │    F_spring = k * (x[i+1] - 2*x[i] + x[i-1])                     │ │
│  │    F_damping = -d * v[i]                                            │ │
│  │    F_nonlinear = -k3 * (x[i+1] - x[i])^3                          │ │
│  │    F_external = excitation force at mass i                          │ │
│  │                                                                     │ │
│  │  Verlet integration (position-based, energy-stable):                │ │
│  │    x_new = 2*x - x_old + (F_total / mass) * dt²                   │ │
│  │                                                                     │ │
│  │  Boundary conditions: Fixed / Free / Periodic (configurable)        │ │
│  │  Update rate: Control rate (2–8 kHz) with linear interpolation      │ │
│  └──────────────────────────────────┬──────────────────────────────────┘ │
│                                      │                                   │
│  ┌──────────────────────────────────▼──────────────────────────────────┐ │
│  │  SCANNER (Audio rate)                                               │ │
│  │                                                                     │ │
│  │  Scanner position sweeps 0 → N → 0 → N at audio-rate frequency     │ │
│  │  Scanner speed = MIDI note frequency (V/Oct pitch control)          │ │
│  │                                                                     │ │
│  │  At each sample:                                                    │ │
│  │    scanPos = fmod(phase * N, N)                                     │ │
│  │    output = interpolate(chain_displacement, scanPos)                │ │
│  │                                                                     │ │
│  │  Scanner width: 1 (single-point read) to N/4 (averaged window)     │ │
│  │  Wider scanner = lower harmonics, smoother tone                     │ │
│  │  Narrower scanner = all harmonics, brighter tone                    │ │
│  └──────────────────────────────────┬──────────────────────────────────┘ │
│                                      │                                   │
│  ┌──────────────────────────────────▼──────────────────────────────────┐ │
│  │  OUTPUT STAGE                                                       │ │
│  │                                                                     │ │
│  │  DC Blocker ──► Soft Limiter ──► Stereo Panner                     │ │
│  │                                                                     │ │
│  │  Stereo: L reads chain forward (0→N), R reads backward (N→0)       │ │
│  │  Creates natural stereo from different "perspectives" on the chain  │ │
│  └──────────────────────────────────┬──────────────────────────────────┘ │
│                                      │                                   │
│                                      ▼                                   │
│                            Output Cache [L, R]                           │
│                                                                          │
└──────────────────────────────────────────────────────────────────────────┘
```

### 2.1 Mass-Spring Chain — The Physics Core

The heart of OBSCURA is a 1D chain of N masses (default 128) connected by springs. Each mass has a displacement value `x[i]` that represents how far it has moved from its rest position. The chain vibrates according to Newtonian mechanics:

**Spring force (Hooke's law):**
```
F_spring[i] = k * (x[i+1] - 2*x[i] + x[i-1])
```
Where `k` is the spring stiffness. Higher stiffness = faster vibration, brighter tone. This is the discrete version of the wave equation — the chain behaves like a vibrating string.

**Damping force:**
```
F_damping[i] = -d * v[i]
```
Where `d` is the damping coefficient and `v[i]` is the velocity (estimated from position difference between timesteps). Higher damping = faster decay, duller tone.

**Nonlinear spring force (cubic stiffness):**
```
F_nonlinear[i] = -k3 * (x[i+1] - x[i])³
```
Cubic stiffness makes the springs stiffer at large displacements — like a real metal wire that resists stretching more as it's pulled further. This produces inharmonic overtones (the overtone series stretches upward, similar to OBSIDIAN's Euler-Bernoulli stiffness). At high values, the chain produces metallic, bell-like tones.

**Verlet integration:**

```cpp
void updateChain(float* x, float* x_old, int N, float k, float d, float k3,
                 float dt2, const float* excitation)
{
    for (int i = 1; i < N - 1; ++i)  // Skip boundary masses
    {
        float dx_left = x[i] - x[i-1];
        float dx_right = x[i+1] - x[i];

        // Linear spring
        float f = k * (x[i+1] - 2.0f * x[i] + x[i-1]);

        // Nonlinear spring (cubic)
        f += k3 * (dx_right * dx_right * dx_right - dx_left * dx_left * dx_left);

        // Damping (velocity from Verlet positions)
        float v = (x[i] - x_old[i]) / std::sqrt(dt2);
        f -= d * v;

        // External excitation
        f += excitation[i];

        // Verlet step
        float x_new = 2.0f * x[i] - x_old[i] + f * dt2;
        x_old[i] = x[i];
        x[i] = x_new;
    }
}
```

Verlet integration is chosen over RK4 because:
- It is unconditionally energy-stable for Hamiltonian systems (the chain won't explode)
- It preserves phase-space volume (Symplectic property)
- It requires only 2 position arrays (no explicit velocity storage)
- It is second-order accurate — good enough for audio-rate physics

**Boundary conditions:**

| Mode | Effect | Musical Character |
|------|--------|------------------|
| Fixed | Masses at ends held at x=0 | String-like (standing waves, harmonic series) |
| Free | Masses at ends move freely | Bar-like (free vibration, inharmonic series) |
| Periodic | Chain wraps around (mass N-1 connects to mass 0) | Ring-like (circular symmetry, odd harmonics) |

### 2.2 Excitation Stage

The chain needs energy to vibrate. Three excitation sources:

**Impulse:** At note-on, a Gaussian-shaped displacement is applied to the chain at a configurable position. This is the "pluck" — a transient that sets the chain vibrating. The impulse width determines the initial spectral content: narrow impulse = all modes excited (bright). Wide impulse = only low modes (dark).

```cpp
void applyImpulse(float* x, int N, float position, float width, float amplitude)
{
    int center = static_cast<int>(position * N);
    float sigma = width * N * 0.25f;
    for (int i = 0; i < N; ++i)
    {
        float dist = static_cast<float>(i - center);
        x[i] += amplitude * std::exp(-0.5f * (dist * dist) / (sigma * sigma));
    }
}
```

**Continuous force:** A sustained force applied to one or more masses, simulating bowing or wind excitation. Creates sustained tones that evolve as the chain finds its vibrating equilibrium under continuous energy input.

**Coupling input:** External audio from the MegaCouplingMatrix applied as force to the chain masses. The external signal *physically shakes the chain* — ONSET's drum hits literally inject energy into the spring network, OBSIDIAN's crystal harmonics become vibrating force patterns on the chain.

### 2.3 Scanner — The Wavetable Reader

The scanner sweeps across the chain at audio rate, reading mass displacements as output samples. The scanner's sweep rate determines the pitch:

```
Scanner frequency = MIDI note frequency
Scanner period = 1 / frequency samples
Samples per sweep = sampleRate / frequency
```

At each audio sample, the scanner's position is computed:

```cpp
float scan(const float* chain, int N, float phase, float width)
{
    float pos = phase * N;  // position along chain [0, N)

    if (width <= 1.0f)
    {
        // Single-point read with cubic interpolation
        return cubicInterpolate(chain, N, pos);
    }
    else
    {
        // Averaged window read
        float sum = 0.0f;
        int halfWidth = static_cast<int>(width * 0.5f);
        int center = static_cast<int>(pos);
        for (int i = -halfWidth; i <= halfWidth; ++i)
        {
            int idx = (center + i + N) % N;
            sum += chain[idx];
        }
        return sum / (2.0f * halfWidth + 1.0f);
    }
}
```

**Scanner width is a critical timbral parameter:**
- Width = 1 (single point): Reads every detail of the chain shape. All vibration modes audible. Bright, rich, complex.
- Width = N/4 (quarter-chain average): Averages over many masses. Only the lowest vibration modes audible. Warm, fundamental-heavy, flute-like.
- Sweeping scanner width creates a filter-like effect but produced entirely by the physics of what you're measuring, not by any DSP filter.

### 2.4 Stereo Output

The L channel reads the chain forward (scanner sweeps 0 → N → 0). The R channel reads backward (scanner sweeps N → 0 → N). This creates natural stereo from two "microphone positions" observing the same vibrating object from opposite ends. The stereo image widens when the chain vibrates asymmetrically and collapses to mono when it vibrates symmetrically.

---

## 3. Parameter Taxonomy

### 3.1 Core Parameters (8)

| ID | Parameter | Range | Curve | Rate | Description |
|----|-----------|-------|-------|------|-------------|
| `obscura_stiffness` | Spring Stiffness | 0.0–1.0 | Exponential | Control | Hooke's law spring constant k. Low = slow vibration (dark, evolving). High = fast vibration (bright, tonal). CHARACTER macro target. |
| `obscura_damping` | Damping | 0.0–1.0 | Exponential | Control | Energy dissipation rate. Low = long sustain. High = quick decay. |
| `obscura_nonlinear` | Nonlinearity | 0.0–1.0 | Exponential | Control | Cubic spring stiffness k3. 0 = perfectly harmonic (string). 1 = deeply inharmonic (metallic, bell-like). |
| `obscura_excitePos` | Excitation Position | 0.0–1.0 | Linear | Control | Where on the chain excitation force is applied. 0 = end, 0.5 = center. Center excites odd modes. End excites all modes. |
| `obscura_exciteWidth` | Excitation Width | 0.0–1.0 | Linear | Control | Width of excitation force. Narrow = bright pluck. Wide = dark bow. MOVEMENT macro target. |
| `obscura_scanWidth` | Scanner Width | 0.0–1.0 | Exponential | Control | Scanner reading window. 0 = single-point (all harmonics). 1 = wide average (fundamental only). SPACE macro target. |
| `obscura_boundary` | Boundary Mode | 0–2 | Stepped | Control | Fixed (0), Free (1), Periodic (2). Fundamentally changes the vibration character. |
| `obscura_sustain` | Continuous Force | 0.0–1.0 | Linear | Audio | Sustained excitation level. 0 = impulse only (pluck). 1 = constant force (bowed). COUPLING macro target. |

### 3.2 Macro Mapping

| Macro | Primary Target | Secondary Target | Musical Effect |
|-------|---------------|-----------------|----------------|
| CHARACTER (M1) | `obscura_stiffness` | `obscura_nonlinear` | Dark evolving texture → bright metallic tone |
| MOVEMENT (M2) | `obscura_exciteWidth` | `obscura_excitePos` | Narrow pluck → wide bow, moving across chain |
| COUPLING (M3) | `obscura_sustain` | Coupling input gain | Impulse-only → sustained + external engine force |
| SPACE (M4) | `obscura_scanWidth` | `obscura_damping` + reverb send | All harmonics bright → fundamental only with slow decay |

### 3.3 Envelope & Modulation Parameters

| ID | Parameter | Type | Description |
|----|-----------|------|-------------|
| `obscura_ampAttack` | Amp Attack | Time | 0ms–10s |
| `obscura_ampDecay` | Amp Decay | Time | 0ms–10s |
| `obscura_ampSustain` | Amp Sustain | Level | 0–1 |
| `obscura_ampRelease` | Amp Release | Time | 0ms–20s |
| `obscura_physEnvAttack` | Physics Env Attack | Time | Controls excitation intensity envelope |
| `obscura_physEnvDecay` | Physics Env Decay | Time | |
| `obscura_physEnvSustain` | Physics Env Sustain | Level | |
| `obscura_physEnvRelease` | Physics Env Release | Time | |
| `obscura_lfo1Rate` | LFO 1 Rate | Hz | 0.01–30 Hz |
| `obscura_lfo1Depth` | LFO 1 Depth | Level | |
| `obscura_lfo1Shape` | LFO 1 Shape | Enum | Sine / Triangle / Saw / Square / S&H |
| `obscura_lfo2Rate` | LFO 2 Rate | Hz | 0.01–30 Hz |
| `obscura_lfo2Depth` | LFO 2 Depth | Level | |
| `obscura_lfo2Shape` | LFO 2 Shape | Enum | |

### 3.4 Voice Parameters

| ID | Parameter | Description |
|----|-----------|-------------|
| `obscura_voiceMode` | Voice Mode | Mono / Legato / Poly4 / Poly8 |
| `obscura_glide` | Glide Time | Portamento (0–2s) |
| `obscura_initShape` | Initial Shape | Sine / Saw / Random / Flat — chain displacement at note-on |

---

## 4. The Ghosts in OBSCURA

### Ghost 1: The Telharmonium (Thaddeus Cahill, 1897) — The Scanned Generator

**The instrument:** The Telharmonium (also called the Dynamophone) was the first instrument to generate audio electrically. Designed by Thaddeus Cahill in Washington, D.C., and built in Holyoke, Massachusetts, it weighed 200 tons, occupied an entire floor of a building at 39th and Broadway in Manhattan, and generated sound by spinning massive tonewheels past electromagnetic pickups. The tone generators were precision-machined rotating shafts with mathematically shaped profiles — different shaft profiles produced different timbral qualities.

The Telharmonium was the first instrument where a physical mechanism was *scanned* to produce audio. The spinning tonewheel IS the vibrating object; the electromagnetic pickup IS the scanner. The concept is identical to OBSCURA's: a physical system (vibrating metal) read by a moving sensor (electromagnetic pickup) at audio rate.

Cahill's ambition was extraordinary: he intended to transmit Telharmonium performances through the telephone network to homes, restaurants, and hotels — streaming music in 1906. The project failed because the Telharmonium's powerful electromagnetic output bled into phone lines throughout Manhattan, disrupting telephone service. AT&T shut it down. All three versions of the Telharmonium were eventually scrapped. No recordings survive.

**How it lives in OBSCURA:** The Telharmonium is OBSCURA's philosophical ancestor — the first "scanned synthesis" instrument. Where Cahill scanned shaped metal with electromagnets, OBSCURA scans simulated masses with a digital scanner. The principle is identical across 130 years: a physical (or simulated physical) object, read at audio rate, produces the waveform. OBSCURA completes what Cahill started.

### Ghost 2: Bill Verplank & Max Mathews — The Scanned Synthesis Inventors (2000)

**The inventors:** In 1999–2000, at Interval Research Corporation (Paul Allen's Palo Alto think tank), interaction designer Bill Verplank and computer music pioneer Max Mathews formalized scanned synthesis as a technique. Mathews — the creator of MUSIC I through MUSIC V at Bell Labs in the 1950s–60s, universally recognized as the father of computer music — spent his final years exploring scanned synthesis as the next frontier of digital sound.

Their insight: separate the *physics* (which produces interesting, evolving shapes) from the *pitch* (which is determined by how fast you read the shape). In conventional physical modeling (e.g., Yamaha VL1), the physics and pitch are coupled — a string's vibration frequency IS its pitch. In scanned synthesis, the vibrating system runs at its own rate (slow, controllable, computationally cheap), and pitch is determined independently by the scanner speed. This decoupling makes scanned synthesis far more flexible and far cheaper than conventional physical modeling.

Verplank and Mathews presented the technique at ICMC 2000 (International Computer Music Conference). It was implemented in CSound as a set of opcodes. A handful of academic compositions used it. No commercial synthesizer ever adopted it. Mathews died in 2011. The technique has been dormant for over two decades.

**How it lives in OBSCURA:** OBSCURA is the direct commercial implementation of Verplank and Mathews's research. The mass-spring chain, the audio-rate scanner, the decoupling of physics rate from pitch — all are faithful to their original design. OBSCURA adds: configurable boundary conditions, nonlinear springs (for metallic timbres), multi-point excitation (for coupling input), and forward/backward scanning for stereo. Mathews never heard scanned synthesis in a polyphonic, MIDI-controlled, preset-driven context. OBSCURA realizes that vision.

### Ghost 3: The Sardinian Cantu a Tenore — The Living Chain

**The tradition:** In Sardinia's Barbagia region, four male voices create the cantu a tenore — one of the oldest continuously practiced vocal traditions in Europe (UNESCO Intangible Cultural Heritage, 2005). The four voices form a literal acoustic chain:

- **Bassu:** The lowest voice, producing a sustained drone. Anchors one end of the "chain."
- **Contra:** A throat-singing voice that produces a nasal drone harmonically related to the bassu. The second link.
- **Mesu boghe:** The "half-voice" — a middle-register voice that bridges the drones and the melody. The middle of the chain.
- **Boghe:** The lead voice, singing the melody. Floats at the other end.

When singing, the four voices create a vibrating acoustic system where each voice is coupled to its neighbors through the shared acoustic space. The bassu's resonance influences the contra; the contra's harmonics interact with the mesu boghe; the mesu boghe's register bridges to the boghe's melody. The composite sound — rich, resonant, deeply textured — is not the sum of four voices but the *vibration of the four-node chain they form*.

The overtone interactions between these voices produce spectral content that belongs to no individual singer — it exists only in the coupling between them. Listeners describe the experience as being "inside the sound" — the four voices create a physical vibrating field that the listener occupies.

**How it lives in OBSCURA:** The cantu a tenore IS a 4-mass chain with acoustic coupling. The bassu and boghe are the boundary masses. The contra and mesu boghe are the interior masses. The stiffness is the acoustic coupling between voices. The scanner is the listener, who "reads" the composite vibration. OBSCURA extends this to 128 masses, but the principle is identical: a chain of coupled resonators producing composite timbres from their collective vibration.

---

## 5. XOlokun Integration

### 5.1 MegaCouplingMatrix Compatibility

**Emits:**
- `CHAIN_SHAPE` — The current chain displacement as a normalized 64-point waveform. Other engines can use this as an evolving wavetable or modulation source.
- `CHAIN_ENERGY` — Total kinetic energy of the chain (sum of mass velocities squared), normalized to [0, 1]. Indicates how "active" the physics simulation currently is.
- `SCAN_POSITION` — Current scanner position (0–1) as a continuous modulation signal. Broadcasts the "reading point" on the chain.

**Accepts:**
- `AudioToFM` — External audio applied as force to chain masses. Literally shakes the physics simulation with external sound.
- `AmpToFilter` — External amplitude modulates spring stiffness. Loud = stiff springs (bright). Quiet = loose springs (dark).
- `RhythmToBlend` — External rhythmic triggers fire impulse excitation. Percussion triggers pluck the chain.
- `KINETIC_IMPACT` (from XOntara) — Transient impacts inject localized energy into the chain.

### 5.2 PlaySurface Interaction Model

**Pad Mode:**
- X-axis: `obscura_excitePos` — Where on the chain to apply force
- Y-axis: `obscura_stiffness` — Spring tension
- Pressure (Z): Excitation amplitude (harder = more energy)

**Fretless Mode:**
- X-axis: Continuous pitch (scanner frequency)
- Y-axis: `obscura_scanWidth` — Slide up = wider scanner (darker)
- Pressure (Z): `obscura_sustain` — Press harder = continuous force (bowing)

**Drum Mode:**
- X-axis: Pad assignment by boundary mode × excitation position (6 pads)
- Y-axis: `obscura_nonlinear` — Vertical = metallic intensity
- Pressure (Z): Impulse amplitude (velocity = pluck intensity)

---

## 6. Preset Archetypes

### 6.1 Steel String
`stiffness=0.6, damping=0.15, nonlinear=0.1, excitePos=0.3, exciteWidth=0.05, scanWidth=0.1, boundary=Fixed, sustain=0.0`

Plucked string with fixed boundaries — standing wave harmonics, moderate brightness, natural decay. Low nonlinearity gives slightly inharmonic character (like a real steel string). Impulse-only excitation. Warm, evolving, acoustic.

### 6.2 Glass Rod
`stiffness=0.8, damping=0.08, nonlinear=0.6, excitePos=0.5, exciteWidth=0.15, scanWidth=0.15, boundary=Free, sustain=0.0`

Free boundary conditions with high nonlinearity — deeply inharmonic, bell-like overtone series. Long sustain from low damping. Center excitation emphasizes odd modes. The glass rod that the Cristal Baschet couldn't play.

### 6.3 Bowed Chain
`stiffness=0.5, damping=0.2, nonlinear=0.2, excitePos=0.2, exciteWidth=0.3, scanWidth=0.2, boundary=Fixed, sustain=0.7`

Sustained excitation simulates bowing — continuous energy input creates a singing, evolving tone. The chain finds its natural vibration mode under constant force, producing a steady-state timbre that shifts subtly over time as the physics evolves. Cinematic, sustained, alive.

### 6.4 Tenore Drone
`stiffness=0.4, damping=0.1, nonlinear=0.3, excitePos=0.0, exciteWidth=0.5, scanWidth=0.3, boundary=Periodic, sustain=0.5`

Periodic boundaries (the chain forms a ring) with wide excitation and wide scanner. Low stiffness creates slow, evolving vibration. The composite tone has the resonant, "inside the sound" quality of Sardinian cantu a tenore — rich, warm, enveloping, with subtle internal motion.

### 6.5 Camera Obscura
`stiffness=0.3, damping=0.05, nonlinear=0.5, excitePos=LFO, exciteWidth=0.1, scanWidth=0.05, boundary=Fixed, sustain=0.3`

LFO-modulated excitation position moves the force source along the chain, exciting different modes in sequence. Very low damping means energy accumulates. High nonlinearity creates increasingly metallic overtones as the chain absorbs more energy. A slowly building, darkening, cinematic texture — like light moving through a camera obscura.

---

## 7. CPU Analysis

### 7.1 Physics Update Cost (Control Rate)

The mass-spring chain runs at control rate (2–8 kHz, configurable) to keep CPU manageable while maintaining physical accuracy.

| Component | Operations per Update | Notes |
|-----------|---------------------|-------|
| Spring force (128 masses) | 128 × 4 multiply + 128 × 3 add | Linear + nonlinear |
| Damping force (128 masses) | 128 × 2 multiply | Velocity × damping |
| Verlet step (128 masses) | 128 × 3 multiply + 128 × 3 add | Position update |
| Boundary enforcement | 2–6 ops | Depends on mode |
| **Total per physics step** | **~1280 multiply + ~768 add** | |

At 4 kHz control rate: ~8.2M ops/sec per voice.

### 7.2 Scanner Cost (Audio Rate)

| Component | Operations per Sample | Notes |
|-----------|----------------------|-------|
| Phase accumulator | 2 add | Standard V/Oct |
| Position calculation | 1 multiply | Phase × N |
| Cubic interpolation | 4 multiply + 4 add | Catmull-Rom |
| DC blocker | 2 multiply + 2 add | First-order IIR |
| Soft limiter | 1 tanh approx | Rational approximation |
| **Total per sample** | **~10 multiply + ~8 add** | |

At 44.1 kHz: ~0.79M ops/sec per voice.

### 7.3 Voice Budget

Per-voice total: ~9.0M ops/sec (physics + scanner)
8 voices: ~72M ops/sec
M1 single-core: ~3.2 GFLOPS
**CPU usage: ~2.3%** per engine instance

With full modulation and coupling overhead, OBSCURA should not exceed **12%** of a single core.

### 7.4 Memory

- Chain state (per voice): 128 × 4 bytes × 2 (current + old) = 1 KB
- Excitation buffer (per voice): 128 × 4 bytes = 512 bytes
- Interpolation cache: 128 × 4 bytes = 512 bytes
- 8 voices: ~16 KB total
- **Total: ~16 KB** — negligible

---

## 8. Implementation Notes

### 8.1 Physics Stability

Verlet integration is naturally energy-stable for Hamiltonian systems, but the addition of damping and nonlinear springs can potentially cause instability at extreme parameter values.

**Stability constraints:**
- Spring stiffness `k` must satisfy `k × dt² < 1.0` for the Verlet step to remain stable. At 4 kHz control rate, `dt = 0.00025`, so `k_max = 16,000,000`. The parameter range is mapped to keep `k` well below this.
- Nonlinear stiffness `k3` is limited to prevent energy injection at extreme displacements. A soft-clip on chain displacement (`tanh(x * 4.0) * 0.25`) ensures masses never exceed ±0.25 displacement.
- Damping `d` is always positive, ensuring energy can only be removed (never added) by the damping term.

### 8.2 Control Rate to Audio Rate Interpolation

The chain updates at control rate (2–8 kHz) but the scanner runs at audio rate (44.1–96 kHz). Linear interpolation between successive chain states provides smooth audio output:

```cpp
float interpolatedChain[N];
float alpha = controlRatePhase;  // [0, 1) between control ticks
for (int i = 0; i < N; ++i)
    interpolatedChain[i] = chain_prev[i] + alpha * (chain_current[i] - chain_prev[i]);
```

This adds negligible cost (128 multiply-adds per sample at audio rate, but only when the scanner is actually reading).

### 8.3 Thread Safety

- Chain arrays pre-allocated per voice in `prepare()`. No audio-thread allocation.
- Physics state is double-buffered: control-rate thread writes to back buffer, audio-rate thread reads from front buffer, atomic pointer swap at control rate.
- ParamSnapshot pattern for all parameter reads.

### 8.4 Denormal Protection

Chain displacement values are flushed to zero if magnitude < 1e-15f. This is critical because the chain can ring down to extremely small amplitudes over long periods, and the Verlet integration will continue computing on denormal values indefinitely.

```cpp
for (int i = 0; i < N; ++i)
{
    if (std::abs(x[i]) < 1e-15f) x[i] = 0.0f;
    if (std::abs(x_old[i]) < 1e-15f) x_old[i] = 0.0f;
}
```

---

*Architecture spec owner: XO_OX Designs | Engine: OBSCURA | Next action: Phase 1 — Verlet Physics Core + Parameter Architecture*
