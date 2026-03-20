# ORGANON Seance Verdict -- 2026-03-20

**Engine:** XOrganon | **Gallery Code:** ORGANON | **Accent:** Bioluminescent Cyan #00CED1
**Creature:** The Deep-Sea Chemotroph | **Param Prefix:** `organon_`

---

## Ghost Council Score: 8.5 / 10

**Previous verdict:** 8/8 PASS (non-numeric)

---

## Doctrine Compliance

| Doctrine | Status | Evidence |
|----------|--------|----------|
| D001 Velocity->Timbre | PASS | Velocity scales initial free energy (0.15 * vel), which controls metabolic bloom rate and modal array intensity. Harder hits = faster/brighter harmonic growth. |
| D002 Modulation Depth | PARTIAL | No user-controllable LFO (no standard LFO with rate param). Mod wheel (CC1) wired (+3 Hz metabolicRate). Aftertouch wired (metabolicRate + signalFlux). 4 macros not present (Organon has 10 unique params, no standard M1-M4). |
| D003 Physics Rigor | PASS | Port-Hamiltonian modal array (van der Schaft & Jeltsema, 2014) with RK4 integration. Shannon entropy analysis. Variational Free Energy (Friston 2010). Fully cited. |
| D004 No Dead Params | PASS | All 10 parameters (metabolicRate, enzymeSelect, catalystDrive, dampingCoeff, signalFlux, phasonShift, isotopeBalance, lockIn, membrane, noiseColor) affect DSP output. |
| D005 Must Breathe | PARTIAL | No standard LFO with rate <= 0.01 Hz. However, the VFE adaptation system creates autonomous timbral evolution. Phason Shift creates per-voice metabolic pulsing. The organism *breathes* by design, but not via a discrete LFO. |
| D006 Expression | PASS | Aftertouch -> metabolicRate (+2.5 Hz) and signalFlux (+0.2). Mod wheel CC1 -> metabolicRate (+3.0 Hz). Both smoothed via PolyAftertouch. |

---

## DSP Quality Assessment

| Criterion | Rating | Notes |
|-----------|--------|-------|
| Anti-aliasing | GOOD | Modal array clamps modes to 49% Nyquist. No oscillator aliasing since output is modal synthesis (sine-based). |
| Denormal protection | EXCELLENT | flushDenormal on modal displacement/velocity, metabolic economy EMA paths, steal fade gain. Comprehensive. |
| Sample rate independence | GOOD | controlRateDivisor derived from sampleRate. Modal array inverseSampleRate computed from prepare(). Not hardcoded. |
| Parameter smoothing | ADEQUATE | Parameters read once per block (ParamSnapshot pattern), not smoothed per-sample. Acceptable for control-rate params. |
| Memory safety | GOOD | Fixed-size arrays (32 modes, 2048-sample ingestion buffer). No audio-thread allocation. |

---

## Strengths

1. **B011 Variational Free Energy** -- The VFE implementation is genuinely novel DSP. The organism predicts its input entropy and adjusts behavior, creating emergent timbral evolution.
2. **Coupling receiver design** -- Organon is the most sophisticated coupling receiver in the fleet. It literally metabolizes partner engine audio.
3. **RK4 modal synthesis** -- 4th-order Runge-Kutta on 32 modes with Port-Hamiltonian energy conservation. Numerically rigorous.
4. **Lock-in tempo sync** -- Metabolic rate can quantize to beat subdivisions via SharedTransport.

## Issues Preventing Higher Score

1. **No standard 4-macro system** -- Organon has 10 unique params but no M1-M4 (CHARACTER/MOVEMENT/COUPLING/SPACE). This breaks the XOmnibus gallery convention.
2. **No user-controllable LFO** -- The VFE system creates breathing, but there is no discrete LFO with a rate knob that can go to 0.01 Hz. D005 spirit is met but letter is not.
3. **No filter envelope** -- Velocity shapes initial energy but there is no explicit velocity-to-filter-brightness path. D001 is satisfied via the metabolic growth curve, but it is indirect.

---

## Path to 9.0

1. Add 4 standard XOmnibus macros (CHARACTER -> catalystDrive+dampingCoeff, MOVEMENT -> phasonShift+lockIn, COUPLING -> signalFlux+membrane, SPACE -> noiseColor+dampingCoeff). ~50 LOC.
2. Add a breathing LFO (0.01-0.5 Hz) that modulates isotopeBalance or enzymeSelect. ~30 LOC.
3. Add explicit velocity-to-enzymeSelect mapping (harder hits = brighter enzyme selectivity). ~5 LOC.

**Estimated effort:** 85 LOC, 1 hour.
