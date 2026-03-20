# OUROBOROS Seance Verdict -- 2026-03-20

**Engine:** XOuroboros | **Gallery Code:** OUROBOROS | **Accent:** Strange Attractor Red #FF2D2D
**Creature:** The Leviathan | **Param Prefix:** `ouro_`

---

## Ghost Council Score: 9.0 / 10

**Previous verdict:** Production-ready (non-numeric)

---

## Doctrine Compliance

| Doctrine | Status | Evidence |
|----------|--------|----------|
| D001 Velocity->Timbre | PASS | Velocity injection: 50ms transient boost proportional to velocity at note-on. Velocity scales output amplitude. Higher velocity = larger attractor excursion = richer harmonic content. |
| D002 Modulation Depth | PASS | Chaos topology is itself a modulation source (4 attractors: Lorenz, Rossler, Chua, Aizawa). Mod wheel -> leash tension. Aftertouch -> chaos depth. The attractor IS the modulation. |
| D003 Physics Rigor | PASS (Exemplar) | RK4 integration for all 4 attractor ODEs. Lorenz (1963), Rossler (1976), Chua (1983), Aizawa cited. B003 Leash mechanism constrains chaos to musical pitch. B007 Velocity Coupling outputs (dx/dt, dy/dt). Most rigorous D003 engine in fleet. |
| D004 No Dead Params | PASS | All params wired. Chaos depth, leash, orbit frequency, attractor blend, injection depth all affect output. |
| D005 Must Breathe | PASS | The chaotic attractor itself is an autonomous modulation source. Trajectories never repeat (sensitive dependence on initial conditions). The engine breathes by nature -- chaos IS breathing. |
| D006 Expression | PASS | Aftertouch -> chaos depth (loosens leash, +0.3 sensitivity). Mod wheel CC1 -> leash tension (+0.4, tightens pitch control). The two create a counterpoint: aftertouch loosens, wheel tightens. |

---

## DSP Quality Assessment

| Criterion | Rating | Notes |
|-----------|--------|-------|
| Anti-aliasing | GOOD | Attractor output is low-frequency (shaped by leash mechanism). Post-processing includes soft limiting. |
| Denormal protection | GOOD | flushDenormal on envelope levels. Attractor state variables bounded by leash mechanism. |
| Sample rate independence | GOOD | RK4 step sizes scaled by sampleRate. Envelope coefficients derived from sampleRate. |
| Parameter smoothing | ADEQUATE | Per-block parameter reads. Coupling accumulators reset per block. |
| Chaos stability | EXCELLENT | Leash mechanism (B003) prevents attractor blowup. Topology crossfading handles attractor transitions smoothly. |

---

## Strengths

1. **B003 Leash Mechanism** -- Constrains chaotic attractor to musically useful pitch range without killing the chaos. Fundamental contribution to chaotic synthesis.
2. **B007 Velocity Coupling Outputs** -- Outputs dx/dt and dy/dt as coupling signals (channels 2-3). Unique in the fleet -- no other engine exposes internal state velocity.
3. **4-attractor topology** -- Lorenz, Rossler, Chua, Aizawa with smooth crossfading. Each has distinct sonic character.
4. **D003 exemplar** -- Alongside OBSCURA, the most physically rigorous engine. All 4 ODEs correctly cited and implemented with RK4.
5. **Expression counterpoint** -- Aftertouch and mod wheel work in opposition (chaos vs control). Musically sophisticated.

## Issues Preventing Higher Score

1. **No user-controllable LFO** -- The attractor provides modulation, but there is no discrete LFO with rate/depth/shape for predictable modulation.
2. **No standard macros** -- Missing M1-M4 macro system.
3. **No post-output filter** -- No SVF filter for traditional timbral shaping.
4. **No mod matrix** -- Expression routing is hardcoded.

---

## Path to 9.5

1. Add 4 standard macros mapping to chaos depth, leash, orbit frequency, injection depth. ~40 LOC.
2. Add post-output CytomicSVF filter with velocity-envelope depth. ~50 LOC.

## Path to 10.0

3. Add a discrete LFO that can modulate leash tension or orbit frequency at user-controlled rates. ~60 LOC.
4. Add 4-slot mod matrix. ~100 LOC.

**Estimated effort to 9.5:** 90 LOC, 1 hour.
