# Seance Verdict: ONEIRIC
**Date:** 2026-04-21
**Seance Type:** First seance — initial doctrine audit (engine scaffolded, no DAW stage time yet)
**Score (provisional): 7.5 / 10**
**Status: scaffolded, compiles clean under -Wpedantic -Werror, presets seeded (6), awaiting first real playthrough**
**Blessing: deferred pending audition**

---

## Architecture
- Nonlinear-Schrödinger soliton synthesis: closed-form superposition of fundamental solitons ψ_k = A_k sech(A_k(x − v_k t − x_k)) · exp(i·phase) sampled at a moving observation probe
- Seven physics-forward mechanics, all audible:
  1. Bound-state selector (Single / 2-soliton 1:3 / 3-soliton 1:3:5 / Mixed) — intrinsic amplitude-ratio breathing
  2. Polarity switch (Bright focusing ↔ Dark defocusing) — envelope dips on CW background
  3. Moving observation probe x_obs(t) with user-controlled velocity v_p
  4. Peregrine rogue-wave event — aftertouch past threshold spawns rational breather (ψ_peak ≈ 3A)
  5. Modulational instability — held CW crystallises into soliton train after dwell time
  6. Lattice potential V(x) (Flat / Periodic / Barrier) — gradient nudges soliton velocity
  7. Dispersive shock attack — narrow transient soliton for chirped note-on
- Per-voice CytomicSVF, ADSR amp + filter envs, poly8, up to 4 solitons/voice
- 2 LFOs × 4 routable targets, 4-slot ModMatrix, 4 macros: CHARACTER / MOVEMENT / COUPLING / SPACE
- Identity: Oneiric | Accent: Phosphene Lavender #B8A0FF | Prefix: `oner_`

## Citations (D003)
- Zakharov & Shabat, "Exact theory of two-dimensional self-focusing…" Sov. Phys. JETP 34, 62 (1972)
- Agrawal, "Nonlinear Fiber Optics" (Academic Press, 5th ed., 2013)
- Peregrine, "Water waves, nonlinear Schrödinger equations and their solutions" J. Austral. Math. Soc. B 25, 16 (1983)
- Akhmediev & Ankiewicz, "Solitons, Nonlinear Pulses and Beams" (1997)

*(citation strength: exemplary — four primary sources spanning the foundational theorem, the practical reference, the rogue-wave special case, and the modern textbook)*

---

## Doctrine Compliance

| Doctrine | Status | Notes |
|----------|--------|-------|
| D001 | ✅ PASS (physics-native) | Velocity → `mAmplitude = ampBase * (1 + m1 * 0.6)` via macro; note-on velocity also directly sets per-soliton A. Taller solitons are narrower (sech collapses) and brighter (carrier rate = (A²−v²)/2 rises with A²). Timbre is velocity-mediated by the equation itself, not by a side-chained filter. |
| D002 | ✅ PASS | 2 LFOs (SolitonVelocity / Dispersion / ProbeSpeed / FilterCutoff targets) + 4-slot mod matrix (7 destinations) + mod wheel + aftertouch + 4 macros. All wired end-to-end. |
| D003 | ✅ PASS (exemplary) | Four primary-source citations in the header block. The engine is a direct implementation of the cited solutions, not a metaphor. |
| D004 | ✅ PASS | Every parameter reaches audio: nonlinearity γ wired via √γ amplitude rescaling (Zakharov-Shabat invariant transform); dispersion β₂ scales phase-rate time base; potential V(x) nudges soliton velocity via finite-difference gradient; attack-shock seeds an additional narrow soliton on note-on. |
| D005 | ✅ PASS | LFO floor 0.005 Hz. Modulational-instability hold timer (0.2–8 s) is slower than any LFO; bound-state breather period is keytracked and independent of LFO. Engine has three orthogonal slow-motion sources. |
| D006 | ✅ PASS (strong) | Velocity → amplitude and filter track. Mod wheel stored. Aftertouch triggers Peregrine rogue wave (threshold + hysteresis) and boosts level. Expression is not optional — it unlocks a timbral region. |

---

## Provisional Weaknesses / Open Questions
1. **Dark-mode envelope approximation.** Polarity=Dark flips the soliton envelope sign on a synthetic CW background rather than solving the defocusing NLS for a true `tanh` dark-soliton profile. Audible character is close but not physically exact. Upgrade path: add tanh-profile branch to `evaluateSoliton` when polarity=Dark.
2. **MI crystallisation uses dwell timer, not noise-seeded Akhmediev breather.** The real physics is stochastic; the implementation is deterministic. Consequence: MI emergence feels *scheduled* rather than *emergent*. Upgrade: seed with a small amplitude-modulation perturbation on the CW background with period ≈ gain-peak frequency.
3. **Dispersive shock is a narrow extra soliton, not an inverse-scattering decomposition.** True NLS step-function decomposition yields a cascade of N solitons with specific amplitude ladder; currently we spawn one. The perceptual difference is probably small; the physics gap is notable if anyone looks.
4. **CPU not yet profiled.** 4 solitons/voice × 8 voices = 32 solitons computed per sample. Each evaluation includes two `std::cos`, two `std::sin`, one `std::exp` (in sech), one `std::sqrt`. Rough estimate: 10–15% of a modern DAW's voice budget per Oneiric instance. Needs measurement.
5. **Coupling-route audit pending.** Engine declares AmpToFilter / AudioToFM / EnvToMorph / LFOToPitch but MegaCouplingMatrix does not yet route Outcrop → Oneiric (the signature wave-terrain → V(x) hook). Without that wiring, the gallery narrative doesn't connect.
6. **Peregrine breather lifecycle.** Currently spawned once and left to decay; no clear removal when it has fully died. Minor (soliton is deactivated at amplitude<0.005), but explicit ejection is cleaner.

## Audition Plan
- Play 6 seeded presets (Lucid Field, Phase Entangled, Dark Aquifer, Rogue Swell, Crystallize, Probe Orbit) on a 49-key keyboard.
- Specifically probe: bound-state breathing across octaves (does the period feel musically right?), Dark polarity tonal color, aftertouch Peregrine response (hysteresis feel), Crystallize MI emergence (is 2s dwell right?).
- Stress: poly8 with 3-soliton bound state + attack shock — expect peak CPU scenario.
- Coupling: once MegaCouplingMatrix wiring lands, route Outcrop.EnvToMorph → Oneiric.PotentialDepth and verify audible terrain-driven V(x) modulation.

## Preset Assessment
6 presets at launch across Atmosphere, Entangled, Shadow, Flux, Aether, Kinetic. Covers the engine's distinctive mechanics. Missing: a Foundation bass preset (Dark polarity could work), a Coupling-mood preset (awaiting MegaCouplingMatrix wiring).

## Open Debates
- **DB-ONER-01:** Is the dwell-timer MI acceptable as a first implementation, or should it be held until noise-seeded Akhmediev is implemented? (Recommendation: ship as-is, flag as TODO.)
- **DB-ONER-02:** Should the Peregrine trigger also respond to very-high mod-wheel values, not only aftertouch? Would give non-aftertouch controllers access to the signature gesture.

---

*Oneiric has honest physics, clean doctrine compliance, and a strong feature stack. Provisional score withholds the upper range because the exotic mechanics (bound states, MI, rogue wave, V(x)) have not yet been auditioned in context. A post-audition re-seance at ≈2 weeks of use is the right gate for a 9.0+ verdict.*
