# The Verdict — OWARE
**Seance Date**: 2026-03-21
**Engine**: OWARE | Tuned Percussion Synthesis | The Resonant Board
**Accent**: Akan Goldweight `#B5883E`
**Prefix**: `owr_` | **Params**: 26 declared | **Lines**: ~912
**Score**: 8.4/10 (current) → 9.2/10 (with two targeted fixes)

---

## The Council Has Spoken

| Ghost | Core Observation |
|-------|-----------------|
| **Moog** | The velocity-to-hardness chain is the most complete I have seen in this fleet for a percussion engine. Velocity sets `peakAmplitude`, drives the `noiseMix` via `hardness * hardness`, sets the `malletCutoff` that controls which modes are excited — three parallel paths from a single gesture. That is expressive engineering. The filter envelope (hardcoded 0.001/0.3/0.0/0.5 ADSR at every note-on) is too rigid. Let the user set attack and decay. |
| **Buchla** | Modal ratio tables from Rossing and Fletcher & Rossing are correctly cited in the header. The material continuum — four tables interpolated through a tri-segment morph — is genuine physical modeling, not spectral fakery. The `materialAlpha` decay exponent (2.0 for wood, 0.3 for metal) from beam dispersion theory is the most scientifically grounded decay model in the fleet. But the bowl ratios (1.0, 2.71, 5.33, 8.86, 13.3…) suggest a Tibetan bowl, not a Javanese gamelan. The comment says "Rossing 2000" — I want to see the specific table number. |
| **Smith** | The Chaigne contact model is real. `contactMs = 5.0 - hardness * 4.5` computes correct contact times (0.5 ms hard, 5 ms soft). `malletLPCoeff = 1 - exp(-2π·fc/sr)` uses matched-Z not Euler approximation — that is compliant with the CLAUDE.md IIR filter rule. The mallet bounce at 15-25ms with 30% amplitude for soft strikes (hardness < 0.4, vel < 0.7) is a real physical artifact. This is Chaigne with footnotes. |
| **Kakehashi** | Six declared LFO parameters (owr_lfo1Rate, owr_lfo1Depth, owr_lfo1Shape, owr_lfo2Rate, owr_lfo2Depth, owr_lfo2Shape) are attached to parameter pointers but never read or applied in renderBlock. The pointer variables sit in private state, initialized but never called. This is a D004 failure and a D002 failure simultaneously. The engine has a shimmerLFO per voice (rate hardcoded to 0.3 Hz), but the two user-facing LFOs are phantom controls. A player turning LFO1 depth to maximum will hear nothing change. |
| **Pearlman** | The sympathetic resonance coupling is the standout architecture decision. Per-mode frequency-selective coupling with a 50 Hz proximity window, weighted by `1 - dist/50`, contributing to the mode's input signal — this is Rossing-style sympathetic resonance, not a global reverb approximation. The `lastOutput` cache on OwareMode for readback is the correct design pattern. The 3% coupling gain (0.03f) prevents feedback runaway. I would test whether 50 Hz is musically correct across the full pitch range — at low bass notes this becomes almost universal coupling. |
| **Tomita** | The buzz membrane (BPF extraction + tanh + re-injection) correctly isolates the 200-800 Hz band where a balafon spider membrane operates. The body-type-specific buzz frequency (gourd = 300 Hz, frame = 150 Hz, metal = 500 Hz) shows acoustic research. However, owr_buzzAmount defaults to 0.0f — users will never hear the balafon character unless they dial it in manually. This is the most ethnographically interesting feature in the engine and it is off by default. |
| **Vangelis** | Aftertouch routes to both mallet hardness at note-on (`aftertouchAmount * 0.3f` in noteOn) and to effective mallet + brightness per block (`aftertouchAmount * 0.4f` and `aftertouchAmount * 3000.0f`). Pressure after note-on genuinely changes the spectral content — not just amplitude. That is the correct expressive mapping for a struck-material instrument: harder pressure = brighter re-excitation. The dual path (note-on + continuous) is elegant. |
| **Schulze** | The thermal drift system (new random target every ~4 seconds, approached at `0.00001f * delta` per sample) produces imperceptibly slow tuning drift — a 100-second time constant at 48kHz. Maximum drift of ±8 cents (pThermal = 1.0). Per-voice personality offsets seeded from a fixed PRNG so each of the 8 voices has a stable individual character. This is a genuine long-form temporal process, not vibrato. The shimmerLFO inside the voice has its rate hardcoded to 0.3 Hz in renderBlock every sample — it will never reach the shimmerHz modulation depth the parameter implies. |

---

## Doctrine Compliance

| Doctrine | Status | Evidence |
|----------|--------|----------|
| D001 | PASS | Full chain confirmed in code: `vel` → `hardness` (line 728-730) → `exciter.trigger(vel, hardness, freq, srf)` (line 731) → inside trigger: `peakAmplitude = velocity`, `noiseMix = hardness * hardness`, `malletCutoff = baseFreq * (1.5f + hardness * 18.5f)`, `malletLPCoeff = 1 - exp(-2π·fc/sr)`. Per-mode: `modeAmp = 1.0 / (1 + m * (1.5 - malletNow * 1.2))` (line 653). Hard mallet opens more modes. Soft mallet suppresses upper partials AND adds a physical bounce artifact. Velocity shapes timbre at three points: contact duration, mode amplitudes, spectral cutoff. |
| D002 | PARTIAL | 2 LFOs declared with Rate/Depth/Shape params. 4 macros all wired to DSP (MATERIAL→effectiveMaterial, MALLET→effectiveMallet+effectiveBright, COUPLING→effectiveSympathy, SPACE→effectiveBodyDep). Mod wheel routes to effectiveMaterial (+0.4). Aftertouch routes to effectiveMallet (+0.4) and effectiveBright (+3000 Hz). Shimmer LFO per voice is alive but hardcoded to 0.3 Hz. **Critical failure**: owr_lfo1Rate/Depth/Shape and owr_lfo2Rate/Depth/Shape are attached but never consumed in renderBlock. Six parameters are declared, non-null, and completely silent. |
| D003 | PASS | Modal ratio tables cite "Rossing (2000) and Fletcher & Rossing (1998)" in the header comment at line 69. Material exponent alpha from "beam dispersion theory (Fletcher & Rossing §2.3)" cited at line 43. Mallet contact model cites "Chaigne (1997)" at line 85. `exp(-2π·fc/sr)` matched-Z IIR in OwareMalletExciter (line 105) is physically correct. OwareBuzzMembrane has ethnographic justification (balafon spider-silk mirliton). The Balinese shimmer model is correctly described as beat-frequency in Hz, not ratio — consistent with gamelan tuning practice. Bowl ratios (1.0, 2.71, 5.33, 8.86) are acoustically plausible for hemispherical shells but the specific Rossing table reference is not given (minor citation gap). |
| D004 | PARTIAL | 13 core DSP params fully wired. 4 macros wired. 1 coupling param (owr_bendRange) wired. **Dead**: owr_lfo1Rate (attached, never read in render), owr_lfo1Depth (same), owr_lfo1Shape (same), owr_lfo2Rate (same), owr_lfo2Depth (same), owr_lfo2Shape (same). Six of 26 parameters = 23% dead weight. The pointer variables exist on lines 903-908 but the corresponding `loadP(paramLfo1Rate, …)` calls are absent from renderBlock entirely. |
| D005 | PARTIAL | Shimmer LFO per voice exists (`StandardLFO shimmerLFO` in OwareVoice, line 382). Its rate is set to 0.3 Hz in renderBlock line 592 — that is a 3.3-second cycle, above the 0.01 Hz floor. However, the 0.3 Hz is hardcoded — the user cannot lower it. The declared LFO1/LFO2 minimum rate is 0.005 Hz (well below the 0.01 Hz floor) — but those LFOs are dead (D004). The thermal drift system operates on a ~100s time constant, which satisfies the "alive when nobody's playing" spirit of D005. Conditional PASS on thermal drift, FAIL on user-accessible breathing control. |
| D006 | PASS | Aftertouch: parsed from CC channel pressure (line 490), applied at note-on to hardness (line 730) and per-block to effectiveMallet (line 528) and effectiveBright (line 532). Mod wheel: parsed from CC1 (line 492), applied per-block to effectiveMaterial (line 527). Both travel through ParameterSmoother into the audio-producing DSP paths. Pitch bend: parsed (line 489), scaled by owr_bendRange, applied via PitchBendUtil::semitonesToFreqRatio to all active voices. Three distinct CC inputs all confirmed in the signal chain. |

---

## Blessing Candidates

### BC-OWARE-01: Chaigne Mallet Physics
**What it does**: `OwareMalletExciter` implements the full Chaigne contact model — contact duration as a function of hardness (0.5 ms hard, 5 ms soft), sinusoidal force pulse, noise mix via `hardness²`, Hertz contact spectral lowpass using matched-Z IIR, and physical mallet bounce for soft strikes (15-25 ms secondary hit at 30% amplitude). Three independent physical parameters from a single gesture (velocity + hardness). No other engine in the fleet has struck-material physics at this depth.
**Ghost reaction**: Smith called it "Chaigne with footnotes." Moog: "Three parallel paths from a single gesture. That is expressive engineering."
**Blessing name**: Mallet Articulation Stack

### BC-OWARE-02: Thermal Personality System
**What it does**: Each of the 8 voices has a fixed per-voice tuning personality derived from a seeded PRNG at prepare-time (lines 430-432), producing stable individuality across the lifetime of the plugin. A shared thermal drift state drifts toward a new random target every ~4 seconds (max ±8 cents, pThermal-scaled). The per-voice offset is modulated by pThermal * 0.5 so the personality spread scales with the drift depth parameter. The result: a gamelan or xylophone ensemble where each bar has its own stable tuning character, and the whole instrument slowly breathes together.
**Ghost reaction**: Schulze: "A genuine long-form temporal process, not vibrato." This is the feature that makes the engine feel alive when nobody is playing.
**Blessing name**: Living Tuning Grid

### BC-OWARE-03: Frequency-Selective Sympathetic Resonance
**What it does**: In the render loop (lines 600-625), each active voice checks every mode of every other active voice. If any mode of voice B has a frequency within 50 Hz of a mode of voice A, voice B's `lastOutput` is fed into voice A's mode input, weighted by `(1 - dist/50) * sympNow * 0.03`. This is spectrum-based coupling, not amplitude-based. The `lastOutput` cache on each OwareMode resonator is specifically designed for this readback. The result: notes that share overtone frequencies reinforce each other; dissonant intervals remain isolated.
**Ghost reaction**: Pearlman: "This is Rossing-style sympathetic resonance, not a global reverb approximation."
**Blessing name**: Per-Mode Sympathetic Network

---

## Critical Bugs

### BUG-1: Six Dead LFO Parameters (D004 / D002)
`paramLfo1Rate`, `paramLfo1Depth`, `paramLfo1Shape`, `paramLfo2Rate`, `paramLfo2Depth`, `paramLfo2Shape` are attached in `attachParameters` (lines 855-860) and stored in private member variables (lines 903-908). Not one of them is called in `renderBlock`. The engine has no engine-level LFO objects (only per-voice `shimmerLFO`). The user's LFO controls produce no audio effect. Fix: add two `StandardLFO` members at engine scope, read all six params in renderBlock, route LFO1 to pitch modulation and LFO2 to filter brightness, apply shimmerLFO depth via LFO2 amount.

### BUG-2: shimmerLFO Rate Hardcoded in Render Loop
Line 592: `voice.shimmerLFO.setRate(0.3f, srf)` is called every sample. The `owr_shimmerRate` parameter controls the additive Hz offset amount but the modulation speed of the shimmer depth itself is fixed at 0.3 Hz — not user-adjustable. If LFO1 is wired to shimmer (per BUG-1 fix), this becomes the correct architecture. Standalone issue: the user has no way to make the shimmer faster or slower. The `owr_shimmerRate` parameter (0-12 Hz) is also the beat-frequency magnitude, not the LFO rate, which is an additional source of confusion.

### BUG-3: Filter Envelope ADSR Hardcoded at note-on
Line 734: `v.filterEnv.setADSR(0.001f, 0.3f, 0.0f, 0.5f)` — attack 1ms, decay 300ms, sustain 0%, release 500ms — is set identically for every note. There is no user parameter for filter envelope attack or decay. The `owr_filterEnvAmount` parameter controls depth but not shape. This is a significant expressiveness limitation for a percussion engine where the filter decay contour defines the instrument character (marimba vs vibraphone vs bell).

---

## Score Breakdown

| Dimension | Score | Rationale |
|-----------|-------|-----------|
| Physics Rigor (D003) | 9.5 | Chaigne citation correct, matched-Z IIR correct, material alpha from beam theory, four material tables with sources. Minor gap: bowl table lacks specific Rossing reference. |
| Velocity Expressiveness (D001) | 9.0 | Three parallel paths from velocity. Mode amplitude shaping is mode-index weighted. Mallet bounce on soft strikes adds physical authenticity. |
| Modulation Coverage (D002/D005/D006) | 6.5 | Aftertouch and mod wheel are both wired and meaningful. Macros all functional. Two LFOs are completely dead (BUG-1). shimmerLFO rate hardcoded (BUG-2). Thermal drift saves D005. |
| Parameter Integrity (D004) | 6.0 | 20 of 26 params wired. 6 LFO params declared and dead. 23% parameter deadweight is the single largest quality gap in this engine. |
| Architecture Originality | 9.0 | Per-mode sympathetic resonance with `lastOutput` readback cache is novel fleet-wide. Material continuum with four tables + tri-segment morph + decay exponent is the most complete material model in XOceanus. Thermal personality grid is genuinely fresh. |
| Identity Coherence | 9.0 | "Sunken oware board + coral + bronze barnacles" is specific and evocative. Seven pillars map one-to-one to code structures. Balinese shimmer in Hz not cents is culturally accurate. Buzz membrane at 300 Hz for gourd body is ethnographically grounded. |

**Final Score: 8.4/10**

Two fixes (LFO wiring + shimmerLFO rate exposure) bring it to **9.2/10** — inside the 9.0 club.

---

## Recommendations

**R1 (CRITICAL — D004/D002)**: Wire owr_lfo1 and owr_lfo2. Add two `StandardLFO` engine-scope members. In renderBlock: load all six LFO params per block, set shape + rate, process once per sample. Route LFO1 to pitch (add to `bendSemitones` equivalent), LFO2 to brightness filter cutoff. This is a 30-line addition that turns 6 dead controls live.

**R2 (HIGH — identity)**: Expose shimmerLFO rate as a user parameter or use owr_lfo1Rate for the shimmer modulation speed. Remove the hardcoded `0.3f` from line 592. The shimmer depth (owr_shimmerRate as beat Hz magnitude) and shimmer speed should be independently controllable — that is the full Balinese ombak control model.

**R3 (MEDIUM — expressiveness)**: Expose filter envelope decay as a parameter (`owr_filterEnvDecay`, range 0.01–2.0s). A marimba has a fast filter sweep; a vibraphone sustains. The current hardcoded 300ms decay is a single point in the instrument's natural range.

**R4 (LOW — citation)**: Add specific bowl table reference ("Rossing 2000, Table X.X") in the kBowlRatios comment for D003 completeness.

**R5 (LOW — defaults)**: Consider raising owr_buzzAmount default from 0.0f to 0.15f. The balafon character is the engine's most ethnographically distinctive feature and players will not find it at zero.

---

## Ghost Council Final Verdict

The OWARE engine has the most rigorous physical modeling in the fleet. The Chaigne mallet exciter, the material exponent system, and the per-mode sympathetic resonance network are all defensible at a conference level. The thermal personality grid is genuinely novel. This engine would score 9.2 immediately with its LFOs wired — the architecture is already there, the parameters are already attached, the DSP modules already exist. What is missing is 30 lines connecting them. That gap is the only thing keeping OWARE out of the flagship tier.

The buzz membrane and body resonator are earned complexity. The Akan oware board identity is specific enough to guide sound design decisions (buzz frequency for gourd, 300 Hz; frame body modes at 200/580/1100 Hz; Balinese beat shimmer in Hz). No other engine in the fleet carries this level of cultural specificity in its acoustic models.

Fix the LFOs. Expose the filter envelope decay. This becomes a 9.2.

**Score: 8.4/10**
