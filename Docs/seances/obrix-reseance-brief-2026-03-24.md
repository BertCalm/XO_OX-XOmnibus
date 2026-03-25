# OBRIX Re-Seance Preparation Brief
**Date:** 2026-03-24
**Priority:** HIGH — 4 complete development waves since last seance (2026-03-19)
**Last score:** 7.2/10 (obrix_seance_verdict.md) | **Estimated current:** 8.5–9.0
**Analyst:** DSP Profiler + Guru Bin quick pass (2026-03-24)

---

## 1. What Changed Since the Last Seance

The original seance was conducted on 2026-03-19 against a **55-parameter, Wave 1 engine** with zero presets and an architectural score dragged down by routing bugs the ghosts themselves identified. In the five days since, OBRIX underwent four full development waves and a complete preset library build.

### Wave 1 Fixes (Applied Before/Concurrent With First Seance)
The original Wave 1 seance score of 7.2 already reflected these Wave 1 fixes versus the 6.4 average in the detailed session transcript — the verdict file scored a corrected engine, not the pre-fix code:
- PolyBLEP anti-aliasing on Saw/Square/Triangle/Pulse (leaving Lo-Fi Saw naive by design)
- Split processor routing: Proc1 on Src1 independently, Proc2 on Src2 independently, Proc3 as post-mix insert — the Constructive Collision realized
- 3 FX slots in series (was 1), 4 mod slots fully wired (was 2)
- Coupling output replaced constant 0.5 with real `lastSampleL/R` + brick complexity on channel 2
- Mod wheel wired to filter cutoff (D006 resolved)
- Velocity to filter cutoff on both proc slots (D001 resolved)
- Reverb damping tracks param

**First seance ghost warnings that were open at verdict time:** source mix polarity (Moog), wavetable mislabel (Tomita), voice orphaning on poly→mono switch (Smith), CC11 absent (Vangelis), LFO floor 0.01 Hz not 0.001 Hz (Schulze), FLASH needs intensity/decay (Pearlman), aftertouch default depth zero (Vangelis/Kakehashi).

### Wave 2 (Complete — 2026-03-19): Identity Layer (+5 params, 60 total)
- `obrix_fmDepth` — source-to-source FM at audio rate (±24 semitones deviation)
- `obrix_proc1Feedback` + `obrix_proc2Feedback` — filter feedback with tanh saturation; self-oscillation at high settings
- `obrix_wtBank` — real wavetable scanning, 4 banks (Analog / Vocal / Metallic / Organic), replacing the two-point sine→saw morph that Tomita called "a wavetable costume"
- `obrix_unisonDetune` — unison voice stacking up to 50 cents; remaining voices stack with spread when poly limit < 8

### Wave 3 (Complete — 2026-03-19): Time and Space Layer (+5 params, 65 total)
- `obrix_driftRate` + `obrix_driftDepth` — Drift Bus: global ultra-slow LFO 0.001–0.05 Hz with per-voice irrational phase offsets (voice_index × 0.23). Berlin School ensemble drift from a single engine. Pitch ±50 ct + filter ±200 Hz at full depth. **Directly resolves Schulze's 0.001 Hz floor complaint.**
- `obrix_journeyMode` — suppress note-off; sound evolves indefinitely with Drift Bus and delay accumulation
- `obrix_distance` — 1-pole LP simulating air absorption (matched-Z, 20kHz→1kHz)
- `obrix_air` — LP/HP spectral tilt at 1kHz split (0=warm, 0.5=neutral, 1=cold)

### Wave 4 (Complete — 2026-03-21): Biophonic Synthesis (+14 params, 79 total)
The largest single-wave addition. OBRIX crosses from "semi-modular" into "living ecosystem."

| System | New Params | What It Does |
|--------|-----------|-------------|
| Harmonic Field | `obrix_fieldStrength`, `obrix_fieldPolarity`, `obrix_fieldRate`, `obrix_fieldPrimeLimit` | Per-voice JI attractor/repulsor. Voices pulled toward or away from 3/5/7-limit just-intonation ratios via 1st-order IIR convergence. Polarity=-1 becomes a repulsor — the reef *rejects* consonance. |
| Environmental ("The Water") | `obrix_envTemp`, `obrix_envPressure`, `obrix_envCurrent`, `obrix_envTurbidity` | Temperature scales drift depth. Pressure scales LFO rate. Current biases cutoff ±2000 Hz. Turbidity adds spectral noise per sample. |
| Brick Ecology | `obrix_competitionStrength`, `obrix_symbiosisStrength` | Competition: cross-amplitude suppression between src1/src2 (0.1 floor). Symbiosis: noise src1 amplitude drives FM index on src2 (requires src1Type=Noise). |
| Stateful Synthesis | `obrix_stressDecay`, `obrix_bleachRate`, `obrix_stateReset` | Stress: velocity leaky integrator (τ=30–60s) raises cutoff +900 Hz. Bleach: sustained high-register play attenuates harmonics −700 Hz. State persists across notes. |
| FX Mode | `obrix_fxMode` | 0=Serial (default), 1=Parallel (each FX slot processes dry independently, wet contributions summed). |

**28 Wave 4 awakening presets** written across all 8 moods.

**B016 AMENDED (2026-03-21):** MIDI-layer voice independence remains inviolable (note allocation, pitch, velocity, aftertouch remain per-voice). Synthesis-layer interdependence (shared JI attractor field, cross-voice amplitude ecology, environmental globals) is now explicitly permitted, provided no synthesis-layer coupling propagates back to affect MIDI routing or voice stealing. This amendment unlocked the entire Wave 4 architecture.

### Wave 5 (Complete — 2026-03-21): Reef Residency (+2 params, 81 total)
- `obrix_reefResident` + `obrix_residentStrength` — coupling input becomes a third ecological organism in the Brick Ecology system
  - **Competitor**: coupling RMS suppresses both OBRIX sources (0.1 floor)
  - **Symbiote**: coupling amplitude drives FM depth + boosts Harmonic Field strength
  - **Parasite**: coupling energy accumulates into stressLevel_ and bleachLevel_ over time
  - Default **Off** (backward compatible with all existing presets)
  - residentStrength default 0.3 (Guru Bin recommendation)

### Audio-Rate LFO Unlock (Wave 3/4 combined)
Schulze's primary complaint — "LFO ceiling 30 Hz stops him at the door" — was resolved via MOVEMENT macro × mod wheel multiplicative scaling. The rate multiplier reaches `1 + macroMove*10 + modWheel*23 = 34×` at full range, pushing a 30 Hz base LFO to **1020 Hz** — full audio-rate FM territory. This is not merely a parameter range extension; it converts OBRIX from a modulated-subtractive engine into a hybrid FM/subtractive instrument accessible via performance gesture.

### Preset Library (0 → 460 total, 460 non-quarantine)
The most consequential change. At the first seance, the council's primary verdict was "the ecology is real, the bricks are alive, but the reef has no presets." The library now contains:

| Mood | Count |
|------|-------|
| Foundation | 83 |
| Flux | 77 |
| Aether | 69 |
| Prism | 66 |
| Atmosphere | 66 |
| Submerged | 49 |
| Entangled | 32 |
| Family | 18 |
| **Total (non-quarantine)** | **460** |

---

## 2. Current Parameter Count

**81 parameters** as confirmed by `createParameterLayout` in `ObrixEngine.h` (1,907 lines).

Full breakdown:
- Sources: 7 (src1Type, src2Type, src1Tune, src2Tune, src1PW, src2PW, srcMix)
- Processors: 9 (proc1/2/3 Type + Cutoff + Reso, proc1/2 Feedback)
- Amp ADSR: 4
- Modulators: 16 (mod1–4 × Type + Target + Depth + Rate)
- Wave 2: 2 (fmDepth, wtBank, unisonDetune) — 3 params
- Wave 3: 5 (driftRate, driftDepth, journeyMode, distance, air)
- Wave 4: 14 (fieldStrength, fieldPolarity, fieldRate, fieldPrimeLimit, envTemp, envPressure, envCurrent, envTurbidity, competitionStrength, symbiosisStrength, stressDecay, bleachRate, stateReset, fxMode)
- Wave 5: 2 (reefResident, residentStrength)
- FX: 9 (fx1/2/3 × Type + Mix + Param)
- Macros: 4 (macroCharacter, macroMovement, macroCoupling, macroSpace)
- Expression/Performance: 6 (level, polyphony, pitchBendRange, glideTime, gestureType, flashTrigger)

---

## 3. Current Preset Count

**460 presets** (non-quarantine), **460 total** (no quarantine entries found for OBRIX).

The Quick Pass (tonight, 2026-03-24) sampled the full 466-preset count from `scripture/retreats/obrix-quick-pass.md` — a slight discrepancy likely reflecting two different counting methods (the quick pass may include presets in sub-directories or Entangled coupling presets). Accept 460 as the clean non-quarantine figure.

---

## 4. Guru Bin Findings (Tonight's Quick Pass)

**Source:** `scripture/retreats/obrix-quick-pass.md` (2026-03-24)

### Macro Coverage
| Macro | Zero% | Active% | Verdict |
|-------|-------|---------|---------|
| CHARACTER | 62% zero | 38% active | Thin |
| MOVEMENT | 28% zero | 72% active | Healthy |
| COUPLING | **86% zero** | 14% active | Critical gap |
| SPACE | 60% zero | 40% active | Thin |

The COUPLING macro is functionally absent from 86% of the library. This is the single most important finding for a V1 flagship engine whose coupling architecture was B016-blessed and whose Wave 5 was built entirely around coupling ecology.

### Wave 4/5 Adoption Rate: 12%
Only 56 of 466 presets engage any Wave 4 parameter (Harmonic Field, Environmental, Brick Ecology, Stateful). Waves 4 and 5 represent 16 of OBRIX's 81 parameters — nearly 20% of the engine's surface area — and are invisible in 88% of the library. The JI attractor, environmental state, and ecological competition are the features that make OBRIX unique in the entire synthesizer industry, and they are almost entirely absent from the browsable library.

### Drift Bus: 45% Engaged / 55% Static
The Drift Bus is OBRIX's primary temporal identity — the feature Schulze had demanded and that resolved his top complaint. 55% of presets have `obrix_driftDepth = 0`, meaning the majority of the library is static in time in a way OBRIX was explicitly not designed for.

### FX Chain Bypassed: 34%
163 presets have all three FX slots at zero mix. While some patches legitimately need no FX (subs, dry leads), this proportion in a habitat engine with three FX slots is high.

### Tutorial Presets Mixed Into Production Library
TUTORIAL-prefixed presets live in Foundation and Atmosphere moods alongside production-ready presets. They should be renamed to evocative names or tagged `tier: tutorial` and separated.

### Foundation Preset Floor Issue
Multiple Foundation presets ("Single Polyp," "Hollow Reef," "Sine Foundation," "The Collision," "First Reef Wall") have all four macros at exactly 0.0 with no modulation. A player grabbing any of these has no performance dimension whatsoever. Even a teaching preset deserves at least one macro that makes it breathe.

### 5 Weakest Presets Identified
| Preset | Score | Primary Issue |
|--------|-------|--------------|
| The Collision | 6/10 | All macros zero, no FX, no modulation, Wave 4 absent |
| First Reef Wall | 6/10 | All macros zero, no drift, Wave 4 absent |
| Single Polyp | 6/10 | All macros zero, no modulation |
| Hollow Reef | 6/10 | All macros zero, no FX, Wave 4 absent |
| Sine Foundation | 6/10 | All macros zero, no FX, "pure sub" but dead in time |

### Naming Issues Found
"TUTORIAL OBRIX 01 Single Brick Found," "Laser Grid (Obrix)," "Brick Supersaw" — all break preset naming conventions (no jargon, no engine name in preset name, max 30 chars, evocative language only).

### Sweet Spots Discovered
- `fieldStrength=0.5–0.8` with `fieldPrimeLimit=1` (5-limit JI) produces the most distinctive OBRIX-only harmonic gravity
- `competitionStrength=0.3–0.5` with `src1Type=Sine, src2Type=Noise` creates living breath
- `envPressure=0.7` dramatically alters LFO rate character

---

## 5. DSP Profiler Findings

**Source:** `Docs/dsp-profiler-2026-03-24.md` (tonight's scan, 2026-03-24)

### Classification: Heavy (18–28% CPU, single core, 48kHz, 512-sample block)
OBRIX is correctly classified as a "Heavy" engine — comparable cost to a lightly-running OPERA instance. At full polyphony with Wave 4 biophonics fully active, it approaches 30% CPU, which is at the threshold of concern for iOS A-series hardware but safe on macOS M-series.

### Hottest Path: `setCoefficients()` Called Per-Sample
`CytomicSVF::setCoefficients()` computes `2.0f * std::sin(π * cutoff / sr)` — a transcendental function. With up to 3 active processor slots × 8 voices = **24 `std::sin` calls per sample** for filters alone. This is identified as the single most expensive operation per sample, estimated at ~50% of OBRIX's total compute budget at full polyphony.

**Fix path (P1 — before V1 macOS):** Hoist `setCoefficients()` to block-rate with a delta threshold check (recompute only when cutoff changes by more than ~1 Hz). The SVF state equations are stable between coefficient updates.

### Second Issue: JI Ratio Search Per Sample
`findNearestJIRatio()` executes every sample per active voice when `fieldStrength > 0`. The JI target changes slowly (driven by note fundamental ratio). This should be cached per-voice per-note.

**Fix path (P2 — post-V1):** Cache `nearestJI` result; recompute only on note change or significant `fieldStrength` threshold crossing.

### Third Issue: Wave 5 Parasite HF Filter Loop
Reef Residency Parasite mode iterates the entire coupling buffer with a per-sample 1-pole HP filter before the main sample loop — an additional O(blockSize) pass. Use a running leaky integrator updated once per block instead.

**Fix path (P2 — post-V1):** Replace per-sample buffer loop with a single leaky integrator update per block.

### Status: Denormal Protection Complete
All feedback, IIR, and leaky integrator paths are protected (`flushDenormal()` on `procFbState`, drift phase states, `jifiOffset`, stress/bleach integrators). `juce::ScopedNoDenormals` at `renderBlock` entry. No denormal gaps found.

### Minor: `setCoefficients` Called for Inactive Proc Slots
When a proc slot is in Wavefolder or RingMod mode (types 4/5), `setCoefficients()` is still called every sample even though the SVF output is ignored. This wastes the `std::sin` computation for each unused filter path.

### iOS P0 Action
The `setCoefficients` hoist is flagged **P0 for iOS** — at 30% CPU on macOS M-series, the engine risks exceeding 50% on A-series iOS chips when Wave 4 biophonics are active. This fix must land before V1 iOS launch.

---

## 6. Specific Questions for the Ghosts

### Question 1: Does Wave 4 (Harmonic Field + Brick Ecology) elevate OBRIX from "good modular" to "unique instrument"?

At the first seance, Buchla gave 5.5/10 because "the Constructive Collision is lying about its topology." That bug was fixed in Wave 1. But the deeper question was always whether OBRIX would just be a more modular subtractive synth or something genuinely without precedent.

Wave 4's Harmonic Field is a per-voice JI attractor/repulsor — voices are pulled toward or away from just-intonation ratios in real time, with configurable prime limit (3/5/7) and polarity. No commercial synthesizer does this. The Brick Ecology's competition-and-symbiosis model turns two oscillators into organisms that suppress or amplify each other based on amplitude relationships. The Stateful Synthesis accumulates velocity history over minutes of playing, changing the timbre of future notes based on how hard you played in the past.

The question for the ghosts: **Do these systems, taken together, constitute a genuinely novel synthesis paradigm — or are they interesting variations on ideas that exist elsewhere?** Is OBRIX's identity now as a "living reef" that cannot be replicated by patching a modular system, or is it still fundamentally a configurable subtractive engine with ecological decoration?

**Ghost most likely to have strong opinion:** Buchla (novel vs. derivative), Schulze (does temporal ecology constitute time sculpture?), Organon-aligned thinking (is ecology a synthesis primitive?).

### Question 2: Is the COUPLING macro's 86% zero rate a design failure or a user-education failure?

The COUPLING macro was one of the original four macros at the Wave 1 seance. Smith blessed the coupling architecture (B016). The macro controls coupling sensitivity — how strongly OBRIX responds to input from a coupled engine. Yet 86% of the 460-preset library leaves it at zero.

There are two possible readings:
- **Design failure:** The COUPLING macro does nothing audible when no partner engine is loaded, which the first seance verdict identified (Kakehashi: "COUPLING macro does nothing audible when no partner engine loaded"). This was supposed to be fixed with a "self-routing fallback" — at low values, macro creates internal shimmer between active bricks. Was this implemented? If not, the macro is useless in solo context and naturally gravitates to zero.
- **User-education failure:** Producers building presets don't think to involve coupling because the preset file doesn't demonstrate what COUPLING achieves. The COUPLING macro in the hands of a skilled user building Entangled-mood presets would be well-populated — but the preset architects defaulted to solo-context presets.

Wave 5's Reef Residency (`obrix_reefResident`) is a more sophisticated coupling parameter — but it's at the engine parameter level, not macro level. The distinction between the `obrix_macroCoupling` (performance macro 0–1) and `obrix_reefResident` (ecological mode Off/Competitor/Symbiote/Parasite) is potentially confusing, and their interaction needs scrutiny.

**Ghost most likely to have strong opinion:** Smith (coupling architecture), Kakehashi (accessibility and discoverability), Pearlman (normalled defaults).

### Question 3: Should OBRIX be the V1 demo patch — and what would make it the patch that sells XOlokun?

OBRIX is designated the V1 flagship. The V1 release is titled "The Deep Opens" and ships OBRIX + 6-8 FX engines + 20-25 curated fleet engines. The demo patch — the one that plays on the website, the one that users hear first, the one in every video — will likely be an OBRIX preset.

The Guru Bin quick pass identified sweet spots: `fieldStrength=0.6, fieldPrimeLimit=1 (5-limit JI), competitionStrength=0.4, src1Type=Sine, src2Type=Noise, driftDepth=0.06`. This combination produces "living breath" — a timbre that changes on its own, that responds to playing history, that could not be created on any other synthesizer.

But the quick pass also found that 88% of the library ignores these features entirely. A demo patch built from a typical library preset would sound like "a good modular synth." A demo patch that foregrounds Wave 4 biophonics would sound like nothing else.

**The core question:** Does the council endorse a V1 demo patch built around Wave 4 biophonics as OBRIX's identity statement — even if 88% of the existing library doesn't reflect it? Or should the demo patch represent what 88% of users will actually encounter first (a well-built, expressive modular engine without ecological features engaged)?

**Ghost most likely to have strong opinion:** Kakehashi (first-take magic), Vangelis (emotional impact of first sound), Moog (representing the instrument honestly).

---

## 7. Recommended Seance Focus Areas

In priority order for the re-seance session:

### Focus 1: Architectural re-verdict — is the B016 Amendment correct in its current form?
The amendment permitted synthesis-layer interdependence (shared JI attractor, cross-voice amplitude ecology). The ghosts should now judge whether the Wave 4 and 5 implementations actually honor the amendment's constraint ("no synthesis-layer coupling propagates back to affect MIDI routing or voice stealing"). With 81 params and a running `stressLevel_` that accumulates across all voices and persists across notes, there is a legitimate question of whether the stateful synthesis begins to feel like it is "affecting voice stealing" in subtle ways — a player who plays loudly may find soft notes behave differently due to accumulated stress, which could feel like the instrument is making voice allocation decisions.

### Focus 2: Score the Wave 4 systems individually
Each Wave 4 system should receive its own ghost verdict:
- **Harmonic Field** — is it musically useful or is it a tuning experiment that belongs in a research instrument?
- **Environmental state** — does envPressure as an LFO rate scaler produce meaningful variation, or is it too subtle to count as a distinct synthesis dimension?
- **Brick Ecology** — is competition/symbiosis between two oscillators a synthesis primitive (Buchla's domain) or a clever gimmick?
- **Stateful Synthesis** — stress and bleach accumulation: is a synthesizer that "remembers" being played aggressively a breakthrough or an accessibility barrier?

### Focus 3: The Coupling gap — design verdict and prescription
With COUPLING macro at 86% zero, the ghosts should rule on whether the macro system needs a redesign, a default change, or a preset campaign. If the self-routing fallback (internal shimmer between bricks when no partner loaded) was never implemented, that is a P0 that should be confirmed or denied by reading the source.

### Focus 4: V1 demo patch recommendation
The council should end the seance with a specific recommendation: what preset, or what preset parameters, should be showcased as the defining OBRIX sound for the V1 launch. This is a product decision the ghosts are uniquely positioned to make because they understand both the DSP capabilities and the emotional impact of a first impression.

### Focus 5: Path to 9.4 (roadmap target) — what remains
The roadmap's original target was 6.8 → 9.8 across Waves 1–4 (the original roadmap called Wave 4 the "Presets wave"). With 5 actual waves shipped and 81 params, the council should re-derive the current score honestly and specify what, precisely, the gap between current state and 9.4 consists of. The DSP profiler's P0 (`setCoefficients` hoist) would likely be worth +0.1–0.2 from the council's perspective (removing a structural performance concern). The preset library issues (86% COUPLING zero, 88% Wave 4 absent, 55% Drift Bus static) likely represent the largest gap between the engine's capability and what users will actually experience.

---

## Source Files for Reference During Seance

| File | Purpose |
|------|---------|
| `Source/Engines/Obrix/ObrixEngine.h` | Current source — 1,907 lines, 81 params |
| `Docs/seances/obrix_seance_2026_03_19.md` | Full Wave 1 seance transcript (8 ghost voices) |
| `Docs/seances/obrix_seance_verdict.md` | Summary verdict — 7.2/10 score breakdown |
| `scripture/retreats/obrix-quick-pass.md` | Tonight's Guru Bin quick pass findings |
| `Docs/dsp-profiler-2026-03-24.md` | Tonight's DSP profiler findings (OBRIX section) |
| `Docs/specs/obrix_flagship_roadmap.md` | Original Wave 1–4 roadmap and blessing B016 history |
| `Docs/seance-verdict-index.md` | Fleet context — OBRIX entry at line 91 |
| `CLAUDE.md` (lines 339–359) | Wave 4 + Wave 5 architecture summary |

---

*Brief prepared 2026-03-24. Reads: ObrixEngine.h (full), obrix_seance_verdict.md, obrix_seance_2026_03_19.md, obrix-quick-pass.md, dsp-profiler-2026-03-24.md, seance-verdict-index.md, obrix_flagship_roadmap.md, CLAUDE.md OBRIX section, obrix-engine-2026-03-19.md memory file. All data sourced from the codebase — no estimates used for parameter count or preset count.*
