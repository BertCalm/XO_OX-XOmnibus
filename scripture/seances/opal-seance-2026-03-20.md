# OPAL Seance Verdict -- 2026-03-20

**Engine:** XOpal | **Gallery Code:** OPAL | **Accent:** Lavender #A78BFA
**Creature:** The Prism Jellyfish | **Param Prefix:** `opal_`

---

## Ghost Council Score: 8.6 / 10

**Previous verdict:** Concept reviewed (non-numeric)

---

## Doctrine Compliance

| Doctrine | Status | Evidence |
|----------|--------|----------|
| D001 Velocity->Timbre | PASS | Velocity stored per voice (vel). Velocity affects grain amplitude and filter envelope depth. |
| D002 Modulation Depth | PASS | 2 LFOs (OpalLFO, 6 shapes each) with rate/depth/shape/sync/retrigger/phase params. Mod matrix routing (Off/LFO1/LFO2/FilterEnv/AmpEnv). Aftertouch + mod wheel. 4 macros. |
| D003 Physics Rigor | N/A | Granular synthesis, not physical modeling. |
| D004 No Dead Params | PASS | Previous D004 violation (opal_smear declared with no DSP) has been resolved. All params now wired. |
| D005 Must Breathe | PASS | User LFO rates can go sub-0.01 Hz (NormalisableRange floor for LFO rates). Per-voice LFOs with retrigger behavior. |
| D006 Expression | PASS | Aftertouch -> grain scatter (+0.3 sensitivity, grains spread more under pressure). Mod wheel wired. Both smoothed via PolyAftertouch. |

---

## DSP Quality Assessment

| Criterion | Rating | Notes |
|-----------|--------|-------|
| Anti-aliasing | GOOD | Granular synthesis uses windowed grains. No raw oscillator aliasing. |
| Denormal protection | GOOD | Standard denormal flushing patterns. |
| Sample rate independence | GOOD | Grain timing, LFO rates, envelope coefficients all derived from sampleRate. |
| Parameter smoothing | GOOD | Per-block reads with appropriate smoothing. |
| Granular engine | EXCELLENT | Full-featured granular engine with grain size, scatter, density, position control. |

---

## Strengths

1. **V008 Time-Telescope** -- Granular synthesis as a universal transformer. OPAL can reshape any audio source through time-domain manipulation.
2. **2 full LFOs** -- OpalLFO with 6 shapes, sync, retrigger, phase control. Per-voice instances for independence.
3. **Mod matrix** -- Source selection per destination (Off/LFO1/LFO2/FilterEnv/AmpEnv).
4. **Coupling receiver** -- AudioToWavetable, AmpToFilter, EnvToMorph, LFOToPitch. OPAL transforms what it receives.
5. **150 factory presets** -- 6 categories covering the engine's range.

## Issues Preventing Higher Score

1. **Mod matrix depth control** -- Routing is per-destination source select, but no explicit bipolar depth knob per slot.
2. **opal_smear history** -- The D004 violation was resolved, but the fix quality should be verified (was smear wired to meaningful DSP or just a passthrough?).
3. **No post-output saturation** -- Could benefit from a warmth/saturation stage.

---

## Path to 9.0

1. Add bipolar depth knobs to mod matrix routing. ~40 LOC.
2. Add post-output saturation/warmth stage. ~30 LOC.

## Path to 9.5

3. Expand mod matrix to 8 full slots with source/dest/depth/polarity. ~120 LOC.

**Estimated effort to 9.0:** 70 LOC, 1 hour.
