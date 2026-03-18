# OSTINATO Seance Verdict

**Date:** 2026-03-17
**Engine:** OstinatoEngine (`osti_` prefix)
**Ghost Score:** 7.9 / 10

## Doctrine Compliance

| Doctrine | Status | Evidence |
|----------|--------|----------|
| D001 | PASS | `velCutoffAmt` parameter scales filter cutoff at noteOn: `velBoost = 1.f + velCutoffAmt * velocity * 2.f` applied to `baseCutoff`. Attack transient amplitude also curves with velocity: `peak = velocity * (0.7f + velocity * 0.3f)`. Both filter brightness and amplitude are shaped by velocity. |
| D002 | PASS | LFO1 (triangle tremolo, `osti_lfo1Rate` floor 0.01 Hz) + LFO2 (sine filter wobble, `osti_lfo2Rate` floor 0.01 Hz). Mod wheel (CC#1) → `modWheelFire` → additive to FIRE macro. Aftertouch → `atCircleMod` → CIRCLE depth. 4 macros: GATHER (cutoff spread), FIRE (amplitude + brightness), CIRCLE (cross-voice modulation), SPACE (reverb). All macros trace to audible DSP paths. |
| D003 | PASS | "Noise exciter → single-pole lowpass (membrane resonance) → exponential decay" is an appropriate physics claim for struck membranes. Matched-Z coefficient used: `exp(-2π·fc/sr)`. The 12 drum character presets carry explicit resonance frequency, Q, and decay values derived from real instrument acoustics. CIRCLE cross-voice modulation models inter-drum head coupling. |
| D004 | PARTIAL | All 25 `osti_` parameters are wired in `attachParameters()`. However, the 8 pattern step booleans (`osti_patternStep0`–`osti_patternStep7`) are read into `pStep[8]` but never consumed in `renderBlock`. No step sequencer trigger logic exists in the render loop. These 8 parameters are broken promises. Additionally `osti_ampAtk`, `osti_ampSus`, and `osti_ampRel` are read in the block param-snapshot but never passed to the voice — `OstiDecayEnv` is single-stage (attack is instantaneous). These three ADSR envelope parameters have no effect on the current DSP. |
| D005 | PASS | `osti_lfo1Rate` and `osti_lfo2Rate` both declare `NRF{0.01f, 10.f}`. Rate floor is 0.01 Hz — doctrine satisfied. |
| D006 | PASS | CC#1 (mod wheel) → FIRE intensity. Aftertouch → CIRCLE interaction depth. Both flow through the per-sample render path. Velocity → filter cutoff + amplitude (D001 path). Three distinct expression inputs are live. |

## Panel Commentary

**Ikutaro Kakehashi:** "The twelve drum character table is a sound basis — matching real-instrument filter frequencies and decay times to MIDI input is precisely how percussion synthesis should work. The cross-voice CIRCLE modulation is a creative concept that gives this engine its communal heartbeat. But eight pattern step parameters that do nothing at all is a fundamental credibility problem. The player will reach for them and hear silence."

**Roger Linn:** "I want to love this engine. The GATHER macro creating organic timing spread through per-voice cutoff offsets is a clever workaround for an engine that doesn't have a sequencer clock yet. But `ampAtk`, `ampSus`, and `ampRel` are pure fiction right now — OstiDecayEnv is single-stage. Either implement the full ADSR envelope or remove those three parameters from the panel."

**Bernard Szajner:** "The FIRE macro's double-duty routing — amplifying both amplitude and filter brightness simultaneously — is conceptually coherent. At high FIRE the engine becomes harsh and present, which matches the arc from soft hand-drum to full-circle intensity. That is expressive design. The missing sequencer is the wound."

## Overall Verdict

PASS ✓

OSTINATO has a strong conceptual and physical foundation. The noise-exciter-through-resonant-filter drum model is well-suited to world percussion, the 12 drum character presets are tuned with care, and the CIRCLE cross-voice modulation genuinely differentiates this engine from a simple drum machine. The FIRE macro, mod wheel routing, and velocity-to-cutoff path all satisfy doctrine.

**Resolution (commits 87ae235 + c902f3a):** Both D004 failures resolved — the 8-step gate sequencer is implemented in `renderBlock()` (advancing `stepIndex` on note-on, skipping notes when gate is closed); `OstiDecayEnv` expanded from single-stage decay to a full 5-argument `trigger(peak, atkSec, susAmp, holdSec, decSec)` ADS+Hold+Decay envelope, with all four env parameters now consumed per voice at note-on time. Per-note parameters (`drumType`, `cutoff`, `res`, envelope) now correctly snapshotted at note-on rather than per-block. Conditional lifted.

## Required Actions

1. **D004 — Pattern step sequencer:** Either implement a basic 8-step trigger engine in `renderBlock` (check step against an internal clock/sync counter on each block), or remove all 8 `osti_patternStep` parameters from `addParameters()` and `attachParameters()` until the sequencer is built.
2. **D004 — ADSR envelope:** Extend `OstiDecayEnv` to a full ADSR (or replace with a proper 4-stage envelope), and wire `osti_ampAtk`, `osti_ampSus`, `osti_ampRel` to per-voice envelope behavior. Alternatively, rename the parameters to match the single-stage model (e.g., expose only `osti_decay`) and remove the three unused params.
3. **Recommendation — Voice count:** 4 voices for a percussion engine with 12 drum types is thin. Consider expanding to 8 voices for richer polyrhythmic layering before the preset writing session begins.
