# OBSCURA Seance Verdict -- 2026-03-20

**Engine:** XObscura | **Gallery Code:** OBSCURA | **Accent:** Daguerreotype Silver #8A9BA8
**Creature:** The Giant Squid | **Param Prefix:** `obscura_`

---

## Ghost Council Score: 9.1 / 10

**Previous verdict:** High / unanimous (non-numeric)

---

## Doctrine Compliance

| Doctrine | Status | Evidence |
|----------|--------|----------|
| D001 Velocity->Timbre | PASS | Velocity scales Gaussian impulse amplitude (0.3 + 0.7 * velocity). Harder strikes excite more chain modes = brighter timbre. Physics-correct: larger displacement drives cubic nonlinear spring harder. |
| D002 Modulation Depth | PASS | 2 LFOs (5 shapes each: Sine/Tri/Saw/Square/S&H), rates 0.01-30 Hz. LFO1->scan width, LFO2->excite position. Mod wheel->sustain/bowing force. Aftertouch->spring stiffness. 4 macros (CHARACTER/MOVEMENT/COUPLING/SPACE). |
| D003 Physics Rigor | PASS | Verlet integration (Stormer-Verlet, symplectic). 128-mass spring chain. Cubic nonlinear springs. 3 boundary modes (Fixed/Free/Periodic). Stability limit k*dt^2 < 1.0. Verplank/Mathews scanned synthesis lineage cited. |
| D004 No Dead Params | PASS | All 29 parameters affect output. Dual envelopes (amp + physics), voice modes, glide, init shape, LFOs, macros all wired. |
| D005 Must Breathe | PASS | LFO1 rate minimum 0.01 Hz (100-second cycle). Both LFOs can go sub-0.01 Hz. |
| D006 Expression | PASS | Aftertouch->spring stiffness (+0.25, brighter timbre). Mod wheel CC1->sustain/bowing force (+0.4). Both smoothed via PolyAftertouch. |

---

## DSP Quality Assessment

| Criterion | Rating | Notes |
|-----------|--------|-------|
| Anti-aliasing | GOOD | Scanner produces aliasing-free output via cubic Hermite interpolation (C1 continuous). Chain physics run at control rate with linear snapshot interpolation. |
| Denormal protection | EXCELLENT | Explicit denormal threshold (1e-15) per chain mass per voice. flushDenormal on chainPrevious. DC blocker feedback path flushed. |
| Sample rate independence | EXCELLENT | Control rate derived from sampleRate / kPhysicsControlRate. DC blocker coefficient computed from sampleRate. Parameter smoothing coefficient uses sampleRate. |
| Parameter smoothing | GOOD | 5ms time constant exponential smoothing on stiffness, damping, nonlinearity, scan width, sustain force. Per-sample smoothing in render loop. |
| Memory safety | GOOD | Fixed arrays (128 masses * 4 snapshots = 512 floats per voice). No heap allocation on audio thread. |

---

## Strengths

1. **D003 exemplar** -- The most physically rigorous engine in the fleet alongside OUROBOROS. Verlet integration is the correct choice for mass-spring systems. Cubic nonlinearity adds amplitude-dependent brightness, matching real struck objects.
2. **Dual envelope system** -- Amplitude + physics envelopes give independent control of sound level and chain excitation, enabling percussive bodies with sustained tails.
3. **Comprehensive modulation** -- 2 LFOs, dual envelopes, aftertouch, mod wheel, 4 macros. D002 fully satisfied.
4. **Stereo imaging** -- Forward/backward scanner creates natural stereo width from chain asymmetry.
5. **Voice modes** -- Mono/Legato/Poly4/Poly8 with LRU stealing and crossfade.

## Issues Preventing Higher Score

1. **No mod matrix** -- LFO destinations are hardcoded (LFO1->scan width, LFO2->excite position). No user-assignable routing.
2. **Physics control rate** -- 4 kHz control rate means ~11-sample interpolation gaps. At 96 kHz this becomes ~24 samples, potentially audible as granularity in fast chain movements.
3. **No filter** -- No post-output SVF filter. The only timbral shaping is physics-based (stiffness, scan width, nonlinearity). A post-filter with velocity envelope would add a familiar synthesis control point.

---

## Path to 9.5

1. Add a post-output CytomicSVF filter with velocity-scaled envelope depth (~60 LOC).
2. Add simple mod matrix (4 slots: source->destination pairs) (~120 LOC).

## Path to 10.0

3. Increase physics simulation rate at higher sample rates (e.g., 8 kHz at 96 kHz audio rate).
4. Add per-voice random phase offset on chain initialization for richer unison.

**Estimated effort to 9.5:** 180 LOC, 2 hours.
