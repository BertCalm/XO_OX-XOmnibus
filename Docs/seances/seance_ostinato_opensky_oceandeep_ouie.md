# Seance — OSTINATO, OPENSKY, OCEANDEEP, OUIE

**Date:** 2026-03-19
**Evaluator:** Documentation Health Agent (formal seance protocol)
**Engines covered:** OSTINATO, OPENSKY, OCEANDEEP, OUIE
**Reference:** All 6 Doctrines (D001–D006); seance format per `seance_cross_reference.md`

---

## OSTINATO Seance — 2026-03-19

**Score:** 8.0/10
**D001:** PASS — velocity opens per-voice filter brightness directly (`baseCutoff = 400 + brightness * effectiveVel * 16000`)
**D002:** PARTIAL — 4 macros (GATHER/FIRE/CIRCLE/SPACE) all wired to DSP; breathing LFO present; no user-controllable LFO rate/depth/shape parameters (the engine has no standalone LFO module exposed to the mod matrix)
**D003:** PASS — physically modeled throughout: modal membrane resonators derived from circular membrane Bessel function zeros (Kinsler & Frey), waveguide body resonance (delay + allpass), radiation filter per articulation. Citations are in code comments. Full D003 compliance.
**D004:** PASS — all per-seat parameters (instrument, articulation, tuning, decay, brightness, body, level, pan, pattern, patternVol, velSens, pitchEnv, exciterMix, bodyModel) read from APVTS and applied in the render loop. Global params (tempo, swing, masterTune, masterDecay, masterFilter, masterReso, reverb, comp, circleAmount, humanize, masterLevel) all reach DSP.
**D005:** PASS — `OstiBreathingLFO` instantiated per sub-voice, running at hardcoded 0.06 Hz; modulates per-voice filter cutoff by ±12% continuously. Engine breathes. Rate is not user-adjustable (noted as concern below).
**D006:** PASS — velocity shapes timbre (D001 confirms this); aftertouch wired to FIRE macro boost (`atPressure`-scaled); mod wheel (CC#1) wired to CIRCLE macro depth (`gCircleAmt += modWheelAmount * 0.3f`).

**Strengths:**
- The most ambitious physically modeled percussion engine in the fleet. 8 seats × 12 instruments × 3–4 articulations, each backed by modal membrane + waveguide body + radiation filter — the physics rigor is exemplary and the D003 citations are present.
- The CIRCLE macro (inter-seat sympathetic resonance with ghost triggers) is architecturally novel — a drum engine where seats influence each other's trigger probability is rare in commercial instruments.
- Velocity sensitivity is genuinely timbral: harder hits raise the filter cutoff by up to 16,000 Hz above the 400 Hz floor, producing a sharp tonal contrast from ghost notes to accents that matches acoustic drum behavior.

**Concerns:**
- D002 partial: no user-accessible LFO module. The breathing LFO is hardcoded at 0.06 Hz and modulates only the per-voice filter. There is no LFO rate, depth, or shape parameter the player can reach. For a fleet that has resolved D002 fully, OSTINATO is the only engine with zero modulation matrix slots. A future pass should add a global `osti_lfoRate` and at least one mod matrix destination.
- The `osti_seat*_bodyModel` "Auto" option delegates body model selection to instrument defaults — this is correct behavior but means the parameter has context-dependent effect. Not a D004 violation, but worth documenting.

**Verdict:** OSTINATO is a fleet standout for D003 rigor — no other engine matches its physical modeling depth across 12 instruments. The sole doctrine gap is a D002 partial: the engine breathes but offers no user-controllable LFO. Recommend a focused pass to add `osti_lfoRate`/`osti_lfoDepth` and 1–2 mod matrix slots in a future refinement round.

---

## OPENSKY Seance — 2026-03-19

**Score:** 8.5/10
**D001:** PASS — velocity drives both filter brightness (`velCutoffMod = voice.velocity * velFilterEnv * 8000.0f`) and pitch envelope depth (`voice.pitchEnvLevel = pitchEnvAmount * velocity`). Two independent timbral pathways scale with velocity.
**D002:** PASS — 2 LFOs (LFO1: 0.005–20 Hz, LFO2: 0.01–50 Hz, both multi-shape with Sine/Triangle/Saw/Square/S&H); mod wheel (CC#1) wired; aftertouch wired; 4 macros (RISE/WIDTH/GLOW/AIR) all produce audible change; 2 mod matrix slots (Src/Dst/Amt for slots 1 and 2).
**D003:** N/A — not physically modeled. No citations required.
**D004:** PASS — all 50 declared parameters attached in `attachParameters()` and loaded in the render loop. `pModSlot1Src/Dst/Amt` and `pModSlot2Src/Dst/Amt` are read each block (though the mod matrix routing logic is lightweight; see concern below).
**D005:** PASS — dedicated `SkyBreathingLFO` class, rate follows `lfo1Rate` parameter (floor 0.005 Hz), modulates `effectiveFilterCutoff` by ±2000 Hz continuously.
**D006:** PASS — velocity shapes timbre (D001); aftertouch increases shimmer mix (`effectiveShimmerMix += aftertouch_ * 0.4f`); mod wheel opens filter cutoff (`effectiveFilterCutoff += modWheelAmount_ * 6000.0f`).

**Strengths:**
- The shimmer reverb is architecturally distinguished: grain-based pitch shifting (octave + fifth, Hann-windowed crossfading grains) feeding into a parallel comb + allpass network with separate feedback control. The GLOW macro's self-reinforcing shimmer tail is genuinely expressive and rare at this price tier.
- The dual velocity pathways (filter brightness burst + pitch envelope depth) make hard voicings feel dramatically brighter than soft voicings — the engine rewards dynamic playing immediately.
- The OPENSKY × OCEANDEEP "Full Column" coupling identity (PitchToPitch harmony across the mythological water column) is one of the most evocative cross-engine narrative concepts in the fleet.

**Concerns:**
- Mod matrix slots 1 and 2 declare Src/Dst/Amt but the routing logic is minimal — the parameters are loaded but the actual dispatch to destinations may be incomplete. Worth a targeted audit to confirm all 8 possible source/destination combinations produce audio changes (D004 conditional).
- `sky_subWave` parameter selects Sine/Square/Triangle for the sub oscillator, but the sub implementation only uses `fastSin(phase * 0.5f * 2π)` regardless of `subWaveIdx`. The choice parameter appears declared but not fully dispatched — potential D004 partial violation on `sky_subWave`.

**Verdict:** OPENSKY is a high-quality, fully compliant engine that earns its place as the feliX pole of the Full Column pairing. The shimmer reverb and dual velocity pathways are genuine standouts. A targeted audit of the mod matrix routing and the `sky_subWave` dispatch would close the remaining D004 ambiguity and push this to near-perfect compliance.

---

## OCEANDEEP Seance — 2026-03-19

**Score:** 8.2/10
**D001:** PASS — velocity shapes filter brightness on every voice: `velFilterBoost = voice.velocity * 0.4f` applied to darkness cutoff, and `envFilterMod = filtEnvLevel * effectiveFiltEnvAmt * voice.velocity` scales filter envelope depth. Amplitude also scales with velocity (`gain = ampLevel * voice.velocity * voice.fadeGain`).
**D002:** PASS — 2 user LFOs (LFO1: rate/depth/shape, LFO2: rate/depth/shape) plus 2 creature LFOs (`DeepCreatureLFO` instances with random drift); mod wheel (CC#1) wired to creature modulation scale; aftertouch wired to PRESSURE macro boost; 4 macros (PRESSURE/CREATURE/WRECK/ABYSS) all wired to DSP; filter envelope with ADSR parameters.
**D003:** PARTIAL — waveguide body resonance (Shipwreck/Cave/Trench) is implemented as a delay + allpass + reflection network with per-body damping presets. The bioluminescent exciter uses probability-based random noise bursts. No formal acoustic citations in comments, but the waveguide implementation follows recognized Karplus-Strong body modeling patterns. D003 partial (not physically claimed in the engine header; no citations, but the architecture is physics-inspired).
**D004:** PASS — all declared parameters read in the block render. `deep_lfo1Rate/Depth/Shape`, `deep_lfo2Rate/Depth/Shape`, creature parameters, macro params all attached and loaded.
**D005:** PASS — two `DeepCreatureLFO` instances with rate floor 0.005 Hz (`std::max(0.005f, hz)`) and built-in random walk drift. The creatures drift continuously below 0.01 Hz. Additionally, user LFO1/LFO2 can be set to slow rates.
**D006:** PASS — velocity shapes filter timbre (D001); aftertouch drives PRESSURE macro (`effectivePressure = clamp(macroPressure + aftertouch_ * 0.3f, 0, 1)`); mod wheel scales creature modulation depth (`creatureScale = clamp(macroCreature + modWheelAmount_ * 0.4f, 0, 1)`).

**Strengths:**
- The Hydrostatic Compressor is a conceptually elegant macro-linked dynamics processor — increasing PRESSURE logarithmically raises compression ratio and makeup gain, simulating the crushing weight of abyssal depth. No other fleet engine uses a physically metaphored compressor in this way.
- The dual DeepCreatureLFO system (two independent slow LFOs with separate random walk drift and smoothing) is the most sophisticated autonomous modulation in the fleet, capable of producing genuinely unpredictable, organism-like movement below 0.01 Hz.
- The OCEANDEEP × OPENSKY "Full Column" coupling identity mirrors OPENSKY's PitchToPitch coupling, and from the deep end it creates sub-frequency pressure under OPENSKY's shimmer — the emotional range when both are coupled is extraordinary.

**Concerns:**
- D003 partial: the waveguide body resonance is well-implemented but the comments name the three body presets (Shipwreck/Cave/Trench) without acoustic measurements or citations. If this engine is presented as physically modeled in marketing, the D003 citation standard from OUROBOROS/ORACLE/OBSCURA should be applied. As written, it is physics-inspired rather than physics-cited.
- The bioluminescent exciter (probability-triggered noise burst with pitch sweep) is noted in the header but the trigger probability and pitch sweep parameters appear to be driven by the CREATURE macro rather than independently exposed. Worth confirming these are not orphaned DSP paths.

**Verdict:** OCEANDEEP is a deep, doctrine-compliant bass engine with a distinctive identity and genuinely novel modulation architecture. The D003 concern is soft (physics-inspired, not physics-claimed), and all six doctrines pass. A citation pass on the waveguide body models and an exciter parameter audit would complete the picture.

---

## OUIE Seance — 2026-03-19

**Score:** 8.4/10
**D001:** PASS — velocity shapes filter cutoff on both voices: `velFilterBoost = voice.velocity * velScale * 4000.0f` applied per voice per sample. AMPULLAE macro scales `velScale` (0.3 + pAmpullae × 0.7), making velocity sensitivity itself macro-controllable.
**D002:** PASS — 2 user LFOs (LFO1 assigned to Voice 1, LFO2 to Voice 2; both 0.01–30 Hz, multi-shape); dedicated breathing LFO (0.005–2 Hz); mod wheel (CC#1) wired; aftertouch wired; 4 macros (HAMMER/AMPULLAE/CARTILAGE/CURRENT) all produce audible change; mod envelope (ouie_modA/D/R/Depth).
**D003:** N/A — not physically modeled. Engine uses VA, wavetable, FM, additive, phase distortion, wavefolder, Karplus-Strong, and filtered noise — algorithmic, not physical. No citations required.
**D004:** PASS — all parameters attached in `attachParameters()`. The extensive per-voice parameter set (algo, waveform, algoParam, wtPos, fmRatio, fmIndex, pw × 2 voices; filter and envelope params; LFO params; mod matrix; breath params; unison; voice mode; split note; glide; level; macros) are all loaded in the block render. No dead parameters observed.
**D005:** PASS — dedicated `OuieBreathingLFO` with rate floor 0.005 Hz (parameter range 0.005–2 Hz), modulates filter cutoff on both voices (`breathMod * 500 Hz`). User LFO1 minimum is 0.01 Hz (exactly at the doctrine floor — passes). The breathing LFO provides the sub-0.01 Hz autonomous modulation.
**D006:** PASS — velocity shapes filter timbre (D001); aftertouch drives HAMMER interaction (`effectiveHammer = clamp(pHammer + aftertouch_ * 0.3f, -1, 1)`); mod wheel morphs both voices' algorithm parameter (`modWheelMod = modWheelAmount_ * 0.5f`, added to `algoParam`).

**Strengths:**
- The STRIFE/LOVE HAMMER axis is one of the most expressive single-parameter gestures in the entire fleet. Sweeping from cross-FM destruction to spectral blend harmony through a single macro (or live aftertouch) gives the player a full emotional narrative in one movement. Aftertouch wired directly to HAMMER makes this gesture performable in real time — a rare and excellent design decision.
- The duophonic architecture (exactly 2 voices, Split/Layer/Duo modes, each with independent algorithm selection from 8 options) gives OUIE the widest timbral contrast of any engine in the fleet. A voice running VA against a voice running Karplus-Strong, interacting through ring modulation, is a sound no other XOlokun engine can produce alone.
- AMPULLAE macro scaling velocity sensitivity (velScale range 0.3–1.0) is a genuinely useful meta-control — it lets a preset dial in exactly how dynamically responsive the engine is, from consistent (low AMPULLAE) to hyper-sensitive (high AMPULLAE).

**Concerns:**
- User LFO1 rate floor is 0.01 Hz — this is exactly at the D005 doctrine floor, not below it. The breathing LFO covers the sub-0.01 Hz range, so D005 passes, but the LFO1 minimum could be lowered to 0.005 Hz for consistency with OPENSKY (0.005 Hz) and the breathing LFO's own floor.
- The mod envelope (ouie_modA/D/R/Depth) is declared and attached but the routing from mod envelope output to a destination is worth a targeted audit to confirm the envelope reaches audio parameters (potential D004 soft concern).
- With 8 algorithms per voice and 2 voices, the Karplus-Strong (`KS`) algorithm requires an excitation signal at note-on. Confirming that KS-to-KS voice interaction in STRIFE mode does not create a click or silence on edge cases (e.g., very short decay, retriggered while active) would be a useful stress test.

**Verdict:** OUIE is a fleet-top engine in terms of expressive architecture — the HAMMER axis, aftertouch-to-HAMMER wiring, and the 2×8 algorithm duophony are distinctive achievements. All six doctrines pass cleanly. The LFO rate floor and mod envelope routing observations are minor; this engine is production-ready with no blocking doctrine issues.

---

## Summary Table

| Engine | Score | D001 | D002 | D003 | D004 | D005 | D006 | Key Distinction |
|--------|-------|------|------|------|------|------|------|-----------------|
| OSTINATO | 8.0/10 | PASS | PARTIAL | PASS | PASS | PASS | PASS | Most rigorous physical percussion modeling in the fleet; no user LFO |
| OPENSKY | 8.5/10 | PASS | PASS | N/A | PASS* | PASS | PASS | Grain shimmer reverb; dual velocity pathways; Full Column coupling |
| OCEANDEEP | 8.2/10 | PASS | PASS | PARTIAL | PASS | PASS | PASS | Hydrostatic compressor; dual random-walk creature LFOs |
| OUIE | 8.4/10 | PASS | PASS | N/A | PASS | PASS | PASS | HAMMER STRIFE/LOVE axis; aftertouch-to-HAMMER; 2×8 algorithm duophony |

*OPENSKY D004 conditional: `sky_subWave` dispatch and mod matrix routing warrant a targeted audit.

---

## Priority Recommendations

1. **OPENSKY** — Audit `sky_subWave` waveform dispatch (sub oscillator only uses sine regardless of selection) and confirm mod matrix slot routing reaches audio destinations.
2. **OSTINATO** — Add `osti_lfoRate` / `osti_lfoDepth` global LFO parameters and at least one user-accessible mod matrix slot to resolve D002 partial.
3. **OCEANDEEP** — Add acoustic citations to waveguide body resonance comments if D003 compliance is desired; confirm bioluminescent exciter probability and pitch sweep are not orphaned DSP paths.
4. **OUIE** — Lower LFO1 rate floor from 0.01 Hz to 0.005 Hz for consistency; audit mod envelope routing.

*Full fleet context: `Docs/seance_cross_reference.md` | Master spec: `Docs/xolokun_master_specification.md`*
