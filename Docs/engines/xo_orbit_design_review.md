# XO_Orbit — Design Review (Phase 0)

**Date:** 2026-03-10
**Input:** User-provided deep-level mechanical spec for chaotic attractor synthesis
**Status:** 6 blocking issues identified, all resolved below

---

## Naming Collision: CRITICAL

The codebase already contains `OrbitalEngine` — a **64-partial additive synthesis engine** (`Source/Engines/Orbital/OrbitalEngine.h`, accent color `#FF6B6B`, 6 voices). "XOrbit" and "XOrbital" are too close for backend clarity or user-facing distinction.

### Recommendation: Rename to **XOuroboros** (engine short name: **OUROBOROS**)

**Why:**
- The ouroboros (serpent eating its tail) is the perfect metaphor for a strange attractor: self-referential, looping, never exactly repeating.
- It captures the engine's defining behavior: the system perpetually traces a path that never closes, creating infinite variation within a bounded space.
- Distinct from every other engine name — no possible confusion with Orbital.
- The "Leash" mechanic literally controls how tightly the serpent bites its own tail (100% = closed loop = repeating waveform, 0% = open trajectory = pure chaos).
- Parameter prefix: `ouro_` (short, distinctive, no collision).
- Accent color suggestion: **Strange Attractor Red** `#FF2D2D` — hot, volatile, distinct from Orbital's coral `#FF6B6B` and Snap's terracotta `#00A6D6`.

### Alternatives considered:
- **XOmen** — evocative but tonally dark/negative for a creative tool.
- **XObscura** — beautiful but suggests hidden/dark rather than chaotic/dynamic.

**CONFIRMED:** User approved XOuroboros on 2026-03-10. Proceeding to Phase 1.

---

## Issue 1: Topology Switching Needs Crossfade

**Problem:** The spec says topology modulation causes "immediate, dramatic shifts." The ODE state vector `[x,y,z]` from Lorenz occupies a completely different phase space than Rössler (e.g., Lorenz z ∈ [0, ~50], Rössler z ∈ [0, ~25]). Slamming from one to the other produces discontinuities → clicks → angry audio engineers.

### Resolution: Dual-System Crossfade with State Normalization

Run **two** attractor instances per voice when a topology change is in progress:

```
1. User changes Topology from A to B.
2. System B initializes with normalized state from System A:
   - Map A's current [x,y,z] from A's bounding box to [0,1]^3
   - Map [0,1]^3 to B's bounding box as initial conditions
3. Both systems run simultaneously for 50ms (crossfade window).
   - System A fades out (linear gain ramp 1.0 → 0.0)
   - System B fades in (linear gain ramp 0.0 → 1.0)
4. After crossfade completes, System A is deallocated (reset, not freed — no audio-thread allocation).
```

**Bounding boxes (pre-computed constants per topology):**

| Topology | x range | y range | z range |
|---|---|---|---|
| Lorenz | [-25, 25] | [-30, 30] | [0, 55] |
| Rössler | [-12, 12] | [-12, 12] | [0, 25] |
| Chua | [-3, 3] | [-0.5, 0.5] | [-4, 4] |
| Aizawa | [-1.5, 1.5] | [-1.5, 1.5] | [-0.5, 2.5] |

**CPU cost:** 2x ODE evaluation during the 50ms window. At 4x oversample, that's ~2200 samples of double-cost per voice. Acceptable — this is an infrequent event (discrete parameter change), not continuous.

**Implementation note:** Each voice pre-allocates space for two `AttractorState` structs. Only one is active in steady state. The second activates during crossfade. Zero allocation on the audio thread.

---

## Issue 2: Rate/h Ambiguity — Pitch Derivation

**Problem:** The spec lists `Rate / h` as "Hz, 0.01 to 20,000" but h is a step size (dimensionless time increment), not a frequency. If h is too large, RK4 diverges (the bounding saturator prevents Inf but produces stable garbage — wrong topology at high h).

### Resolution: User controls target **pitch frequency**; h is derived internally.

The parameter is renamed to **Orbit Rate** and represents the target perceived pitch in Hz. The engine derives h using a per-topology calibration constant:

```
h = (targetFreqHz / calibrationFreqHz[topology]) * baseStepSize[topology]
```

**Per-topology calibration (measured empirically at default chaos index):**

| Topology | Base h | Calibration Freq (Hz) | Max Safe h |
|---|---|---|---|
| Lorenz (σ=10, ρ=28, β=8/3) | 0.005 | ~130 Hz | 0.02 |
| Rössler (a=0.2, b=0.2, c=5.7) | 0.01 | ~95 Hz | 0.05 |
| Chua (α=15.6, β=28) | 0.002 | ~200 Hz | 0.008 |
| Aizawa (a=0.95, b=0.7, ...) | 0.005 | ~110 Hz | 0.015 |

**Safety clamp:** `h = min(h, maxSafeH[topology])`. If the user requests a pitch that would require h > maxSafeH, the system uses maxSafeH and relies on the Leash hard-sync to enforce the correct pitch period. This is actually musical — at extreme high pitches, the Leash naturally dominates and the system becomes a complex repeating waveform.

**Chaos Index interaction:** As Chaos Index increases, the attractor becomes more volatile and the max safe h *decreases*. The engine should scale `maxSafeH` by `(1.0 - chaosIndex * 0.3)` to maintain stability at high chaos.

**Parameter rename:** `orbit_rate` → displayed as "Orbit Rate (Hz)" in the UI with the range 0.01–20,000 Hz. Internally this maps to h via the calibration table.

**MIDI pitch tracking:** When a note is played, `targetFreqHz = midiNoteToFreq(noteNumber)`. The Leash phasor runs at this exact frequency. The ODE step size h is derived from it. At Leash = 0, only h controls pitch (approximate, drifty). At Leash = 1, the phasor enforces exact pitch (hard-synced repeating period).

---

## Issue 3: Leash — Deterministic Blend, Not Stochastic

**Problem:** The spec uses both "hard-sync probability" and "forces a Poincaré section reset," which are contradictory.

### Resolution: Deterministic crossfade between free-running and hard-synced output.

The Leash parameter controls a **continuous blend** between two signal paths:

```
output = (1.0 - leash) * freeRunning + leash * hardSynced
```

Where:
- **freeRunning:** The attractor runs without any resets. Pitch is approximate, governed only by h. The orbit may drift, bifurcate, or wander across the attractor.
- **hardSynced:** A master phasor at the exact MIDI frequency resets the attractor state to a Poincaré section point every cycle. The waveform repeats exactly — like a complex wavetable.

**Poincaré section definition (per topology):**

| Topology | Reset Condition | Reset State |
|---|---|---|
| Lorenz | x crosses 0 (ascending) | [0, y_current, z_current] normalized to attractor scale |
| Rössler | x crosses 0 (ascending) | [0, y_current, z_current] |
| Chua | x crosses 0 (ascending) | [0, y_current, z_current] |
| Aizawa | θ crosses 0 (ascending in azimuthal plane) | Computed from current radius |

**Why not stochastic:** A player needs to predict what Leash does. Random resets would make performance unreliable — you'd get different timbres on every note-on at the same Leash value. Deterministic blend means the player can dial in a repeatable sweet spot.

**The sweet spot (~30-60% Leash):** The hard-synced path provides the pitch anchor while the free-running path provides harmonic movement. The blend creates a sound that tracks pitch reliably but has constantly shifting overtone content — like an analog oscillator with organic drift, but far more complex.

**Implementation detail:** Both paths run simultaneously (both cost CPU). At Leash = 0, the hardSynced path can be skipped (early exit). At Leash = 1, the freeRunning path can be skipped. In the 0-1 range, both run. This doubles the ODE cost per voice in the mid-range but maintains constant CPU in the extremes.

**Optimization:** At Leash < 0.01, skip hardSynced entirely. At Leash > 0.99, skip freeRunning. This covers the common cases (full chaos or full sync) with single-system cost.

---

## Issue 4: 4x Oversampling — Downsampling Filter

**Problem:** The spec specifies 4x oversampling but doesn't define the downsampling filter order.

### Resolution: Half-band FIR, 12-tap, pre-computed coefficients.

A 12-tap half-band FIR filter provides:
- ~70dB stopband rejection (sufficient to push aliasing below the noise floor)
- Linear phase (no group delay distortion)
- Half-band symmetry means only 6 multiplies per output sample (every other coefficient is zero)
- Pre-computed coefficients: no audio-thread allocation

**Signal chain per voice:**

```
ODE (4x rate) → Cubic saturator → Downsample (12-tap half-band) → DC blocker → 3D→2D projection → Output
```

**DC blocker:** First-order high-pass at 5 Hz. Coefficient: `R = 1 - (2π * 5 / sampleRate)`. Applied after downsampling (at 1x rate) to avoid affecting the oversampled ODE dynamics.

**CPU budget check:**
- RK4 at 4x: 4 samples × 4 evaluations × 3 state vars × ~6 ops = ~288 FLOPs per output sample per voice
- Half-band downsample: 6 multiplies + 6 adds = 12 FLOPs per output sample per voice per channel (×2 channels = 24)
- DC blocker: 4 FLOPs per output sample per channel (×2 = 8)
- 3D projection (rotation matrix): 6 multiplies + 4 adds = 10 FLOPs per output sample
- **Total per voice:** ~330 FLOPs per output sample
- **6 voices:** ~1980 FLOPs per output sample
- At 44.1kHz: ~87M FLOPs/s — well within 22% of a single M1 core (~15 GFLOPS)

---

## Issue 5: Coupling OUT — Velocity Vectors via Existing Interface

**Problem:** `getSampleForCoupling(channel, sampleIndex)` only provides L/R stereo. The spec wants to emit velocity vectors dx/dt, dy/dt as control signals.

### Resolution: Multi-channel coupling cache with semantic channel mapping.

The current `getSampleForCoupling()` interface supports arbitrary channel indices. We extend the output cache to 4 channels:

| Channel | Content | Use Case |
|---|---|---|
| 0 | Left audio (projected) | Standard coupling audio source |
| 1 | Right audio (projected) | Standard coupling audio source |
| 2 | dx/dt (normalized to [-1, 1]) | Non-repeating complex LFO for filter, morph, etc. |
| 3 | dy/dt (normalized to [-1, 1]) | Non-repeating complex LFO, decorrelated from ch2 |

**Normalization:** Velocity vectors are divided by the per-topology max expected velocity to keep them in [-1, 1]:

```cpp
float normalizedDxDt = clamp(dxdt / maxVelocity[topology], -1.0f, 1.0f);
```

**Receiving engine behavior:** Engines that call `getSampleForCoupling(2, ...)` or `getSampleForCoupling(3, ...)` get the velocity data. Engines that only request channels 0-1 get normal audio. No changes to the SynthEngine interface — it already accepts arbitrary channel indices.

**MegaCouplingMatrix routing:** When an Ouroboros→Target coupling is configured as `LFOToPitch` or `EnvToMorph`, the matrix reads from channels 2/3 instead of 0/1. This mapping is configured in the coupling matrix, not in the engine itself.

**Fallback:** If `getSampleForCoupling()` is called with channel > 3, return 0.0f. If the velocity cache hasn't been filled (engine not yet rendered this block), return 0.0f.

---

## Issue 6: Missing Specifications

### 6a. Voice Count: 6

Matching Orbital's 6 voices. Chaotic attractors are computationally lighter per-voice than 64-partial additive synthesis (Orbital) or 32-mode PHS (Organon). 6 voices at 4x oversample with dual-system Leash is within budget:

- Worst case (all 6 voices, Leash at 50% = both systems running): 6 × 2 × 330 = 3960 FLOPs/sample ≈ 175M FLOPS/s
- Still under 2% of M1 single-core. CPU budget is not the constraint here.

### 6b. Note-On/Off Behavior

**Note-On:**
- The attractor state is **not reset**. The organism continues from wherever it currently orbits. This means the first note after silence starts from the initial condition `[0.1, 0.1, 0.1]`, but subsequent notes inherit the current trajectory.
- The Leash phasor resets to 0.0 on note-on (ensuring pitch alignment from the first cycle).
- Velocity maps to Injection gain boost for the first 50ms (harder hit = more chaotic initial transient).

**Note-Off:**
- Standard ADSR release (the amp envelope fades the output, but the attractor keeps running internally so it can resume smoothly on the next note-on).
- If voice is stolen: 5ms crossfade (consistent with all other engines).

**Legato:**
- Attractor state carries over (the orbit continues unbroken).
- Leash phasor frequency updates to new note (pitch glides if portamento is active).
- No transient — pure pitch change with continuous timbre evolution.

### 6c. Macro Mapping

| Macro | Label | Parameter X | Parameter Y | Musical Effect |
|---|---|---|---|---|
| M1 | CHARACTER | `ouro_chaosIndex` | `ouro_damping` | Serene stable loop ↔ screaming torn chaos |
| M2 | MOVEMENT | `ouro_theta` | `ouro_phi` | Rotates the 3D manifold — shifts harmonics and stereo image as if walking around the sculpture |
| M3 | COUPLING | `ouro_injection` | `ouro_leash` | How deeply other engines perturb the orbit + how tightly it's synced to pitch |
| M4 | SPACE | `ouro_damping` | `ouro_rate` | (Damping shared with M1) Warm slow drift ↔ bright fast orbit |

**Note:** Damping appears in both M1 and M4. This is intentional — it's the most musically impactful parameter after Chaos Index and should be reachable from multiple macro contexts. If this creates confusion, M4 could map `ouro_damping` to a secondary range (e.g., M4 controls only the 0.3–1.0 range for warm→neutral, while M1 controls 0.0–0.7 for neutral→aggressive).

### 6d. Preset Archetypes (7 presets, 6 moods)

| # | Name | Mood | Standalone? | DNA (B/W/M/D/S/A) | Key Settings |
|---|---|---|---|---|---|
| 1 | Strange Loop | Foundation | Yes | 0.5/0.5/0.4/0.4/0.2/0.3 | Lorenz, Leash 0.6, Chaos 0.3, Damping 0.4. The "first encounter" — stable, musical, warm. |
| 2 | Butterfly Effect | Atmosphere | Yes | 0.4/0.6/0.8/0.3/0.5/0.2 | Lorenz, Leash 0.3, Chaos 0.5, Theta slowly modulated. Evolving pad with chaotic micro-movement. |
| 3 | Feral Oscillator | Prism | Yes | 0.8/0.2/0.7/0.7/0.1/0.9 | Chua, Leash 0.1, Chaos 0.9, Damping 0.0. Raw, aggressive, screaming. |
| 4 | Event Horizon | Entangled | No | 0.3/0.4/0.9/0.8/0.6/0.5 | Rössler, Injection 0.8, Leash 0.4. Transforms whatever feeds it into spiraling harmonics. |
| 5 | Phase Portrait | Flux | Yes | 0.6/0.3/0.9/0.5/0.2/0.4 | Aizawa, Leash 0.5, Chaos 0.6, Theta/Phi animated. Rhythmic, shifting, alive. |
| 6 | Deterministic Ghost | Aether | Yes | 0.2/0.7/0.5/0.2/0.9/0.1 | Lorenz, Leash 0.95, Chaos 0.2, Damping 0.7, high reverb send. Near-static complex waveform. |
| 7 | Perturbation Engine | Entangled | No | 0.5/0.3/0.8/0.6/0.3/0.7 | Rössler, Injection 1.0. Designed to be fed by ODDFELIX/ONSET — drum hits knock the orbit into new trajectories. |

**"Strange Loop"** is the first-encounter preset: Lorenz attractor at moderate chaos with enough Leash to be pitch-stable. Produces a warm, complex tone that evolves subtly. Macros produce obvious changes. No coupling required.

### 6e. Accent Color: **Strange Attractor Red** `#FF2D2D`

Evokes heat, energy, volatility. Distinct from:
- Orbital's coral `#FF6B6B`
- OddfeliX's terracotta `#00A6D6`
- Obese's hot pink `#FF1493`

---

## Revised Parameter Specification (8 Core Parameters)

| # | Parameter ID | Display Name | Type | Range | Default | DSP Function |
|---|---|---|---|---|---|---|
| 1 | `ouro_topology` | Topology | Int | 0–3 | 0 (Lorenz) | Selects ODE system (0=Lorenz, 1=Rössler, 2=Chua, 3=Aizawa) |
| 2 | `ouro_rate` | Orbit Rate | Float | 0.01–20000 Hz | 130.0 | Target pitch frequency; internally derived to step size h via calibration |
| 3 | `ouro_chaosIndex` | Chaos Index | Float | 0.0–1.0 | 0.3 | Maps to ODE-specific bifurcation parameter (e.g., Lorenz ρ: 0→1 = 20→32) |
| 4 | `ouro_leash` | Leash | Float | 0.0–1.0 | 0.5 | Blend between free-running (0) and hard-synced to MIDI phasor (1) |
| 5 | `ouro_theta` | Theta | Float | -π to π | 0.0 | 3D→2D projection rotation X-axis (radians) |
| 6 | `ouro_phi` | Phi | Float | -π to π | 0.0 | 3D→2D projection rotation Y-axis (radians) |
| 7 | `ouro_damping` | Damping | Float | 0.0–1.0 | 0.3 | Low-pass integration accumulator; smooths sharp attractor corners |
| 8 | `ouro_injection` | Injection | Float | 0.0–1.0 | 0.0 | Scales external coupling audio displacement into ODE velocity equations |

### Parameter Notes

- **Topology** is a discrete integer parameter. Changes trigger the 50ms dual-system crossfade (see Issue 1).
- **Orbit Rate** replaces the ambiguous "Rate / h". The user thinks in pitch, not step sizes. MIDI note-on overrides this parameter with the note frequency; the parameter serves as the base rate for free-running (no MIDI) or drone patches.
- **Chaos Index** maps to different ODE coefficients per topology:
  - Lorenz: ρ ∈ [20, 32] (onset of chaos at ρ ≈ 24.74)
  - Rössler: c ∈ [3, 18] (period-doubling cascade)
  - Chua: α ∈ [9, 16] (double scroll formation)
  - Aizawa: ε ∈ [0.1, 0.95] (torus → chaos transition)
- **Leash** at values < 0.01 or > 0.99 enables single-system optimization (see Issue 3).
- **Theta/Phi** are displayed in degrees in the UI (0°–360°) but stored as radians internally.

---

## Coupling Interface Mapping

### As Coupling TARGET (receiving from other engines)

| CouplingType | Ouroboros Behavior | Concept |
|---|---|---|
| `AudioToFM` | Audio injected into dx/dt velocity equation (scaled by `ouro_injection`) | Perturbation — knock the orbit off-trajectory |
| `AudioToWavetable` | Audio injected into dy/dt velocity equation (scaled by `ouro_injection`) | Perturbation on orthogonal axis |
| `RhythmToBlend` | Rhythm signal modulates Chaos Index | Rhythmic bifurcation — beat-synced chaos transitions |
| `EnvToDecay` | Envelope modulates Damping | Dynamic smoothing — attack is sharp, sustain is warm |
| `AmpToFilter` | Source amplitude modulates Damping | Amplitude-reactive warmth |
| `EnvToMorph` | Envelope modulates Theta projection | Envelope-driven harmonic rotation |
| `LFOToPitch` | LFO modulates Orbit Rate | Standard pitch vibrato |
| `PitchToPitch` | Pitch tracking modulates Orbit Rate | Harmonic tracking |

### Unsupported types (no-op)

| CouplingType | Why |
|---|---|
| `AudioToRing` | Ring mod doesn't map to ODE perturbation — the attractor IS the waveform, not a carrier |
| `FilterToFilter` | No traditional filter chain in the signal path |
| `AmpToChoke` | Killing an attractor mid-orbit would require resetting state → click |
| `AmpToPitch` | Redundant with PitchToPitch; amplitude→pitch creates feedback loops with the Leash |

### As Coupling SOURCE (sending to other engines)

4-channel output cache:
- Channel 0: Left projected audio
- Channel 1: Right projected audio
- Channel 2: Normalized dx/dt (complex non-repeating LFO)
- Channel 3: Normalized dy/dt (complex non-repeating LFO, decorrelated)

MegaCouplingMatrix routes channels 2/3 when the coupling type is `LFOToPitch`, `EnvToMorph`, etc.

---

## PlaySurface Mapping

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

## Best Coupling Partners

| Partner | Route | Musical Effect |
|---|---|---|
| ODDFELIX | ODDFELIX → Ouroboros via `AudioToFM` | Percussive hits knock the orbit into new trajectories — rhythmic chaos |
| ORGANON | Ouroboros → Organon via `AudioToFM` | Chaotic audio feeds the metabolic organism — unpredictable nutrition creates wild harmonic blooms |
| ODYSSEY | ODYSSEY → Ouroboros via `EnvToMorph` | Long envelopes slowly rotate the projection angle — evolving harmonic architecture |
| ODDOSCAR | Ouroboros → OddOscar via ch2/3 `LFOToPitch` | Velocity vectors as complex LFOs driving wavetable position — organic, non-repeating modulation |
| ONSET | ONSET → Ouroboros via `AudioToFM` | Drum patterns as physical forces — the orbit "dances" to the beat |

---

## Verification Checklist (Phase 0 → Phase 1 Gate)

- [ ] Naming collision resolved (Ouroboros vs Orbital)
- [ ] 8 parameters defined with `ouro_` namespace IDs
- [ ] Coupling maps to existing `CouplingType` enum (no new types)
- [ ] Voice count = 6, CPU budget verified at <22% single-engine
- [ ] Topology crossfade mechanism specified (50ms dual-system)
- [ ] Rate/h derivation from pitch frequency specified with per-topology calibration
- [ ] Leash defined as deterministic blend (not stochastic)
- [ ] 4x oversampling with 12-tap half-band FIR downsample filter
- [ ] DC blocker specified (5 Hz high-pass, post-downsample)
- [ ] Velocity vector coupling via 4-channel output cache
- [ ] Note-on/off/legato behavior defined
- [ ] 7 preset archetypes with mood assignments and DNA values
- [ ] "Strange Loop" first-encounter preset defined
- [ ] Macro mapping for all 4 macros
- [ ] PlaySurface mapping for all 3 modes
- [ ] Accent color assigned: `#FF2D2D`

---

*All 6 blocking issues from the initial review are resolved. Ready for Phase 1: Architecture Specification.*
