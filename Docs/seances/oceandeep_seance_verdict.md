# OCEANDEEP Seance Verdict

**Date:** 2026-03-17
**Engine:** OceandeepEngine (`deep_` prefix)
**Ghost Score:** 9.1 / 10

## Doctrine Compliance

| Doctrine | Status | Evidence |
|----------|--------|----------|
| D001 | PASS | `velCutoffBoost = currentVel * velCutoffAmt * 150.f` is applied to `finalCutoff` in the param snapshot. High velocity opens the darkness filter, brightening the abyssal tone. Velocity is also stored as `currentVel` and implicitly scales output amplitude via `ampEnvLevel`. |
| D002 | PASS | LFO1 (sine, 0.01–2 Hz, creature modulation → bio rate + darkness filter cutoff) + LFO2 (sine, 0.01–0.5 Hz, pressure wobble → pitch + compressor depth). Mod wheel (CC#1) → `modWheelCutoffMod` (lowers darkness filter by up to 200 Hz). Aftertouch (both poly and channel pressure) → `aftertouchPressureMod` → increases hydrostatic compression. 4 macros: PRESSURE, CREATURE, WRECK, ABYSS. All macros have confirmed DSP paths in renderBlock. |
| D003 | PASS | Three distinct physics models declared and implemented with rigor: (1) hydrostatic compressor — peak-sensing gain reduction with one-pole attack/release envelope, models water column pressure; (2) waveguide body — comb filter tuned to fundamental frequency with three body character modes (open water, cave, wreck) using fractional tuning offsets; (3) bioluminescent exciter — LFO zero-crossing triggers burst envelopes into a 2-pole bandpass-filtered noise source, modeling deep-sea bioluminescent pulse patterns. Darkness filter uses full 2-pole Butterworth via bilinear transform (Zölzer-style). All matched-Z coefficients used throughout. |
| D004 | PASS | All 25 `deep_` parameters are attached in `attachParameters()` and confirmed to have DSP paths in `renderBlock`. Macro blends are computed explicitly (`effectivePressure`, `effectiveBioMix`, `effectiveBodyMix`, `abyssFilterClose`, etc.) and flow into per-sample computation. No dead parameters found. |
| D005 | PASS | LFO1 floor: `nr(0.01f, 2.0f)` — 0.01 Hz satisfied. LFO2 floor: `nr(0.01f, 0.5f)` — 0.01 Hz satisfied. `deep_bioRate` also has a 0.01 Hz floor. Three independent modulation sources all breathe at ultra-slow rates. |
| D006 | PASS | CC#1 → darkness filter cutoff (closes filter, darkens tone). Channel pressure + poly aftertouch both received → `aftertouchPressureMod` → hydrostatic compression amount. Velocity → filter brightness (D001). Three distinct expression input channels confirmed active. |

## Panel Commentary

**Bob Moog:** "This is the engine I would have wanted to build for the bass register. The three-oscillator sub stack with independent sub-sub-octave frequency paths, each with subtle LFO2 detuning, creates that 'displacement before sound' low-end pressure. The hydrostatic compressor is a genuinely novel insert — I have not seen peak-sensing gain reduction framed as underwater physics before. It makes the parameter meaningful in a way that 'compressor amount' never does."

**Suzanne Ciani:** "The bioluminescent exciter is poetic engineering. Triggering burst envelopes from a slowly breathing LFO zero-crossing, then filtering the noise burst through a 2-pole bandpass, gives you exactly the quality of intermittent alien light in deep water. The CREATURE macro amplifying both bio rate and bio mix simultaneously is correct — more creature means both louder and faster pulsing. I approve."

**Oskar Sala:** "The waveguide body character modes are well-differentiated: open water, cave, and wreck each use distinct tuning offsets to shift the comb resonance. The fact that the WRECK macro progressively selects the body character above thresholds (>0.35 → cave, >0.7 → wreck) while also increasing feedback is sophisticated and completely audible. This engine earns its physics claim."

## Overall Verdict

PASS

OCEANDEEP is the strongest of the four V1 concept engines examined in this seance. It is fully doctrine-compliant across all six doctrines, with zero dead parameters, three confirmed expression input channels, and three rigorously implemented physical models. The monophonic bass architecture is appropriate for the identity. The SilenceGate is configured with a 500 ms hold to accommodate long bass tails. The per-block parameter snapshot is clean, coupling inputs are consumed and reset each block to prevent accumulation drift.

One architectural note for the preset writing phase: the darkness filter ceiling is 800 Hz — this is intentional and identity-defining, but preset designers should be briefed that OCEANDEEP's filter never fully opens. The ABYSS macro sweeps the cutoff down by up to 350 Hz, which means at high ABYSS the filter closes toward 50 Hz even if `deep_filterCutoff` is at maximum. This is correct behavior but may surprise preset writers expecting a fully open filter.

## Required Actions

None — doctrine-compliant.

**Recommended (non-blocking):** Notify preset designers that darkness filter range is intentionally 50–800 Hz and that ABYSS macro actively closes the filter. Document this in the sound design guide entry for OCEANDEEP.
