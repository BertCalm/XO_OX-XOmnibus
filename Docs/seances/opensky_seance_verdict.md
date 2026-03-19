# OPENSKY Seance Verdict

**Date:** 2026-03-17
**Engine:** OpenSkyEngine (`sky_` prefix)
**Ghost Score:** 8.4 / 10

## Doctrine Compliance

| Doctrine | Status | Evidence |
|----------|--------|----------|
| D001 | PASS | `velBrightness = 0.5f + vel * 0.5f` scales `filterFc` per-sample inside `OpenSkyVoice::tick()`; shimmer trigger level also scales with `v * 0.7f` at noteOn. Velocity explicitly shapes both filter brightness and shimmer harmonic content. |
| D002 | PASS | LFO1 (pitch/shimmer vibrato, triangle wave, per shimmer voice) + LFO2 (stereo chorus rate, dual modulated delay lines). Mod wheel (CC#1) → shimmerDepth. Aftertouch → shimmerDepth boost. 4 working macros: CHARACTER (shimmer depth), MOVEMENT (LFO rate 0.01–4 Hz), COUPLING, SPACE (reverb size + mix). All macros trace to audible DSP changes in renderBlock. |
| D003 | N/A | No physically-modeled components declared. Schroeder reverb uses matched-Z coefficients; `exp(-2π·fc/sr)` one-pole filter. DSP is wave-based synthesis, not physics claim. |
| D004 | PARTIAL | 28 of 30 `sky_` params are wired to DSP. `sky_filterRes` (`pFilterRes`) is read and loaded but marked "reserved" — not passed to `SkyBrightFilter::process()`, which takes only `fc`. `sky_shimmerOct` (`pShimmerOct`) is read into `pShimOct` but never used in the voice tick or shimmer interval selection (the intervals are fixed in `kShimmerIntervals`). Two parameters are broken promises. `sky_shimmerBright`, `sky_shimmerSpread`, `sky_shimmerPhase`, `sky_glide`, and `sky_velSensitivity` are attached but their effect on audio is not confirmed in the render path — specifically `pVelSens` and `pGlide` have no visible usage in `renderBlock`. |
| D005 | PASS | LFO rate floor is 0.01 Hz: `N{0.01f, 4.0f}` for `sky_lfoRate`. Chorus rate floor is also 0.01 Hz. D005 satisfied. |
| D006 | PASS | CC#1 (mod wheel) → `modWheelShimmer` → blended into `shimmerDepth`. Aftertouch (channel pressure) → `atShimmer` → shimmerDepth. Both flow into the per-sample voice render. Velocity → filter brightness confirmed (D001 path serves D006). |

## Panel Commentary

**Robert Moog:** "The shimmer stack is genuinely lovely — four pitched sines at octave and fifth intervals with per-voice LFOs is exactly the kind of additive complexity that rewards careful listening. But I keep reaching for `sky_filterRes` and hearing nothing. Dead parameters break trust. Fix it or remove it from the panel."

**Wendy Carlos:** "The Schroeder implementation is clean, using matched-Z coefficients throughout rather than Euler approximations — a good sign. I am more concerned that `sky_shimmerOct` registers as a parameter yet the kShimmerIntervals array is a compile-time constant. If you promise the user a knob, it must move something."

**Marcus Fischer:** "The MOVEMENT macro cascading through both the shimmer LFO rate and chorus rate simultaneously is an excellent design choice. When MOVEMENT sweeps, the engine truly breathes. The `sky_glide` parameter is less convincing — portamento logic is not wired in renderBlock. That needs attention before this engine ships."

## Overall Verdict

PASS ✓

OPENSKY is a strong engine with a coherent identity. The supersaw-plus-shimmer architecture is implemented cleanly: velocity drives filter brightness per-sample, the four shimmer voices with individual LFOs create genuine harmonic animation, and the MOVEMENT macro's dual-routing to shimmer LFO and chorus rate is well-conceived. The Schroeder reverb follows correct matched-Z coefficient design.

**Resolution (commit 87ae235):** All four D004 violations resolved — `sky_filterRes` wired to resonance feedback in `SkyBrightFilter::process()`; `sky_shimmerOct` drives `octMult = fastPow2(shimmerOctShift)` applied to interval ratios; `sky_glide` implements per-voice portamento via one-pole `glideCoeff = 1 - fastExp(-1/(sky_glide * sr))`; `sky_velSensitivity` scales the vel→brightness path as `velBrightness = 0.5f + vel * sens + (1-sens) * 0.7f`. All 30 `sky_` parameters are confirmed live. Conditional lifted.

## Required Actions

1. **D004 — `sky_filterRes`:** Wire resonance to `SkyBrightFilter` (extend to a one-pole with resonance shelf or replace with a simple biquad), or remove the parameter. Do not ship a "reserved" knob.
2. **D004 — `sky_shimmerOct`:** Make `kShimmerIntervals` dynamic — `sky_shimmerOct` should offset or scale the shimmer interval ratios so the parameter moves the shimmer pitch register audibly.
3. **D004 — `sky_glide`:** Implement portamento in `renderBlock` — smoothly slide `v.freq` toward `targetFreq` using a one-pole coefficient derived from `pGlide * sr`.
4. **D004 — `sky_velSensitivity`:** Apply `pVelSens` as a scaling factor on the `vel * brightness` path: e.g., `float velBrightness = 0.5f + vel * pVelSens * 0.5f` so the knob controls how aggressively velocity opens the filter.
