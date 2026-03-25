# Seance Records — OMBRE, ORCA, OCTOPUS

**Date:** 2026-03-19
**Evaluator:** Claude Code (Sonnet 4.6)
**Method:** Full source-code doctrine audit against D001–D006
**Sources:** `Source/Engines/Ombre/OmbreEngine.h`, `Source/Engines/Orca/OrcaEngine.h`, `Source/Engines/Octopus/OctopusEngine.h`

---

## OMBRE Seance — 2026-03-19

**Score:** 7.8/10
**D001:** PASS — velocity drives `opsisTransient` intensity (scales a saturation burst via `fastTanh`) and multiplies directly into output gain; harder hits produce richer harmonic transients, not just louder amplitude.
**D002:** PARTIAL — 1 LFO (rate 0.005–10 Hz, depth 0–1) modulates filter cutoff; mod wheel and aftertouch both wired; all 4 macros active in DSP. D002 requires a minimum of 2 LFOs; only one formal LFO parameter pair exists. The Oubli memory drift and transient system provide organic modulation texture but do not constitute a second user-controllable LFO.
**D003:** N/A — OMBRE is not a physically modeled engine.
**D004:** PASS — all 21 declared parameters are attached in `attachParameters()` and read in the `renderBlock` ParamSnapshot. No dead parameters found.
**D005:** PASS — `ombre_lfoRate` minimum is 0.005 Hz (well under the 0.01 Hz floor requirement); the LFO runs every block unconditionally.
**D006:** PASS — mod wheel (CC#1) sweeps blend toward pure Opsis (perception) live; channel pressure (aftertouch) deepens Oubli interference haunting, both wired in the MIDI event loop.

**Strengths:**
- The dual-narrative architecture (Oubli/Opsis) is conceptually unified and mechanically sound — every parameter maps to a distinct sonic narrative role.
- Decay-on-read memory reconstruction (O(1) per read head via age-based attenuation) is an elegant DSP solution that avoids buffer traversal while producing physically coherent ghost echoes.
- Aftertouch integration is semantically meaningful: pressure deepens memory interference, so the player's hand controls how much the past haunts the present — not just filter brightness.

**Concerns:**
- D002 shortfall: only one LFO parameter pair (`ombre_lfoRate` / `ombre_lfoDepth`). The LFO targets only filter cutoff; there is no second LFO for pitch, blend, or memory drift modulation. Adding a second LFO targeting blend or memory drift would complete D002 and deepen the engine's narrative expressiveness.
- The memory buffer is not reset on note-on by design ("the ghost of the last note haunts this one"), which is correct conceptually but means the first note played after a silence starts cold while subsequent notes accumulate ghosting — players may experience inconsistency in live performance.
- Stereo spread implementation uses `oubliOut` as the pan modulation signal directly, which means frequency content of the memory output controls panning in real time. This can produce erratic imaging artifacts when memory grain sizes are small.

**Verdict:** OMBRE is a thoughtfully designed, doctrinally sound engine with one clear gap (D002 second LFO) and strong expressive wiring. The dual-narrative concept is executed with genuine craft — the Oubli memory buffer, interference cross-feed, and mod wheel/aftertouch semantics all serve the same poetic premise. Recommended for production with the addition of a second LFO.

---

## ORCA Seance — 2026-03-19

**Score:** 8.1/10
**D001:** PASS — `velFilterBoost = voice.velocity * 0.35f` is added to formant intensity every sample loop; harder hits push the five-band formant network further open, producing more vocal, harmonically complex character — not just amplitude scaling.
**D002:** PASS — two independent LFOs (lfo1: 0.01–30 Hz, 5 shapes; lfo2: 0.01–30 Hz, 5 shapes) plus a modulation envelope, mod wheel wired, all 4 macros active. Minimum requirement met.
**D003:** PASS (conditional) — the wavetable is built from inharmonic partial series that model orca vocalizations (sine-through-metallic-harmonic morphing); the 5-band formant network uses frequencies cited from actual orca vocal tract anatomy (F1=270 Hz, F2=730 Hz, F3=2300 Hz, F4=3500 Hz, F5=4500 Hz). The echolocation subsystem uses a resonant comb filter pinged at click rates that approximate cetacean biosonar. D003 compliance is earned — the biology is the architecture.
**D004:** PASS — all declared parameters are attached and consumed in the render loop. The HUNT macro drives five simultaneous DSP targets (filter cutoff, resonance, formant intensity, echo resonance, crush mix) — no dead endpoints found.
**D005:** PASS — both LFO rate parameters have a minimum of 0.01 Hz, satisfying the breathing floor requirement.
**D006:** FAIL — mod wheel (CC#1) is wired and scans wavetable position. However, **there is no aftertouch (channel pressure) handler** in the MIDI event loop. The MIDI processing block handles note-on, note-off, all-notes-off, and CC#1 only. Aftertouch data is silently discarded.

**Strengths:**
- The APEX HUNT macro is a fleet-level standout: a single parameter simultaneously drives filter cutoff, resonance, formant intensity, echo resonance, crush mix, and sub-bass level. This is a B-level blessing candidate — the engine moves as a coordinated predatory unit.
- The procedural orca wavetable (64 frames, 2048 samples, inharmonic stretch with formant-like mid-partial boost and late-frame breath noise) is substantive DSP work — not a stock wavetable scan.
- Countershading band-split bitcrushing (clean low belly, decimated high dorsal) is a genuinely novel processing concept that maps directly to orca anatomy. The per-voice `crushHold` state correctly implements sample-rate reduction without shared state contamination.

**Concerns:**
- D006 aftertouch failure: no channel pressure handling. For an apex predator engine, aftertouch should have an obvious semantic home — pressure could control breach sub-bass displacement, echolocation urgency (click rate), or hunt intensity. The absence is a clean fix.
- The breach sidechain compressor is applied to the entire output buffer after the render loop rather than per-voice, which means the compressor sees a mix of all voices simultaneously. This is architecturally intentional (the breach displaces the whole acoustic space) but means the compressor behavior is polyphony-dependent in ways the player cannot anticipate.
- LFO2 default depth is 0.0 — in the default state, the second LFO makes no contribution to audio. Presets should bake in non-zero LFO2 depth for the engine to breathe out of the box.

**Verdict:** ORCA is a production-ready engine with exceptional conceptual coherence and a single clear doctrine failure (D006 aftertouch absent). The biology-to-DSP mapping is rigorous — formant frequencies, inharmonic stretching, and echolocation pinging all trace to genuine orca anatomy and behavior. Fix aftertouch and ORCA scores in the 8.5 range.

---

## OCTOPUS Seance — 2026-03-19

**Score:** 8.3/10
**D001:** PASS — velocity drives two simultaneous timbre paths: `velCutoffBoost = voice.velocity * 4000.0f` opens the main filter, and `velChromaBoost = voice.velocity * 0.3f * 3000.0f` boosts chromatophore filter frequency. Harder hits produce both a brighter core timbre and a more aggressive skin-shift response.
**D002:** PASS — 2 standard LFOs (lfo1: 0.01–30 Hz, 5 shapes; lfo2: 0.01–30 Hz, 5 shapes) plus 8 independent arm LFOs running at prime-ratio-related rates (phi, sqrt(5), pi, etc.), a modulation envelope, mod wheel wired, all 4 macros active. D002 is decisively met.
**D003:** N/A — OCTOPUS is not a physically modeled engine (biological inspiration is behavioral/structural, not acoustically modeled).
**D004:** PASS — all declared parameters are attached in `attachParameters()` and consumed in the render loop. The arm system reads `pArmCount`, `pArmSpread`, `pArmBaseRate`, `pArmDepth` and distributes them across 8 targets. No dead parameters found.
**D005:** PASS — `octo_armBaseRate` minimum is 0.005 Hz (below the 0.01 Hz floor); `octo_lfo1Rate` / `octo_lfo2Rate` minimum is 0.01 Hz. The arms run continuously at ultra-low rates when spread is enabled.
**D006:** FAIL — mod wheel (CC#1) is wired and intensifies chromatophore depth (up to +0.4, causing aggressive filter morph). However, **there is no aftertouch (channel pressure) handler** in the MIDI event loop. Channel pressure data is silently discarded. The engine's comment block at the `modWheelAmount_` declaration explicitly describes the mod wheel function (D006 compliance) but makes no mention of aftertouch.

**Strengths:**
- The 8-arm polyrhythmic modulation system using irrational-ratio rates (golden ratio, pi, sqrt(5), etc.) guarantees the engine never phase-locks — a generative modulation architecture that will produce genuinely non-repeating texture without any external sequencing. This is a fleet-level innovation and a strong B-level blessing candidate.
- The Ink Cloud freeze buffer is a semantically precise design: velocity above threshold triggers a noise burst that mutes the dry signal proportionally as it decays — the octopus defense mechanism expressed as audio. The `dryMute` calculation (`1.0f - smoothedInkMix * voice.inkCloud.frozenGain * 0.8f`) is a clean implementation of this idea.
- The Sisters S-014 bug note in comments (coupling accumulators reset after render loop, not before) shows active maintenance awareness — this is a real coupling correctness fix that would have caused couplingPitchMod to always be zero.

**Concerns:**
- D006 aftertouch failure: no channel pressure handling. Semantically, aftertouch on OCTOPUS should trigger the chromatophore system — pressure should induce a camouflage response, morphing the filter topology. This is the most natural mapping in the fleet and its absence is notable.
- The chromaFilter is set to `LowPass` mode at initialization and re-set to `LowPass` every sample in the chromatophore processing block (`voice.chromaFilter.setMode(CytomicSVF::Mode::LowPass)`) — the filter type never actually changes despite `chromaMorphTarget` controlling LP/BP/HP/Notch blend. The morph is achieved by mixing the LP output against the dry signal (a crude approximation of HP/BP). A true morphing SVF would need separate LP, BP, HP outputs or mode switching between samples. The current implementation is functional but not as texturally rich as described.
- The Ink Cloud noise buffer fill uses a simple first-order IIR smoothing (`prev * 0.3f`) which may not produce the "dark, saturated" noise quality described in comments at all densities — at low density values the noise may be too quiet to register as a genuine defense burst.

**Verdict:** OCTOPUS is the fleet's most modulation-dense engine — 10+ LFO sources, velocity double-wiring, and a biologically-grounded arm system that generates genuinely non-repeating polyrhythmic texture. One clear doctrine failure (D006 aftertouch) and one implementation note (chromaFilter morph approximation) keep it from a perfect score. Fix aftertouch and OCTOPUS challenges ORCA for fleet leadership.

---

## Summary Table

| Engine | Score | D001 | D002 | D003 | D004 | D005 | D006 |
|--------|-------|------|------|------|------|------|------|
| OMBRE | 7.8/10 | PASS | PARTIAL | N/A | PASS | PASS | PASS |
| ORCA | 8.1/10 | PASS | PASS | PASS | PASS | PASS | FAIL |
| OCTOPUS | 8.3/10 | PASS | PASS | N/A | PASS | PASS | FAIL |

## Priority Fixes

| Priority | Engine | Fix |
|----------|--------|-----|
| P1 | ORCA | Add aftertouch handler — map channel pressure to hunt intensity or breach sub level |
| P1 | OCTOPUS | Add aftertouch handler — map channel pressure to chromatophore morph depth |
| P2 | OMBRE | Add second LFO targeting blend or memory drift to satisfy D002 minimum |
| P3 | OCTOPUS | Evaluate chromaFilter morph implementation — true SVF multi-mode vs. LP-mix approximation |

---

*Seance conducted 2026-03-19. See `Docs/seance_cross_reference.md` for full fleet history.*
