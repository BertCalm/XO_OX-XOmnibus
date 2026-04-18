# Seance Verdict: Overbite
**Date:** 2026-04-12
**Convened by:** Claude (Ringleader session)
**Status:** FORMAL NUMERIC RE-SEANCE — supersedes "full approval (~9.2 est.)" on record
**Engine file:** `Source/Engines/Bite/BiteEngine.h`
**Parameter prefix:** `poss_` (frozen)
**Presets:** 478 total across 8 moods

---

## Context

Overbite (formerly XOppossum/Bite) carried a "full approval (~9.2 est.)" designation in the cross-reference with note "best expression in fleet (B008)." This seance provides the formal numeric score to replace the estimate and record the ghost panel verdict.

The engine was examined in full (122 parameters, all DSP stages, coupling implementation, preset library sample). No blockers found.

---

## Ghost Panel Summary

| Ghost | Score | Key Comment |
|-------|-------|-------------|
| **Moog** | 9.0 | "The CytomicSVF with four modes, key tracking, filter drive, and velocity-to-envelope at 10,000 Hz range is a filter system that breathes. The Gnash asymmetric stage mirrors what my ladder does to positive transients. I recognize this." |
| **Buchla** | 9.0 | "The 5 macros as first-class mod matrix sources — BELLY, BITE, SCURRY, TRASH, PLAY DEAD available as slots 11–15 in an 8-source matrix — this is a patch system with opinions. PLAY DEAD is the most interesting parameter name I have encountered in any synthesizer." |
| **Smith** | 9.5 | "122 frozen parameters. LRU voice stealing. 16-voice polyphony. 7 unison voices. Tempo-synced LFOs. Echo sync. MPE per-voice expression. This is the most complete voice architecture in the fleet. I would have shipped this." |
| **Kakehashi** | 9.0 | "BELLY is warm, BITE is aggressive, SCURRY is nervous, TRASH is dirty, PLAY DEAD fades away. You know what each macro does before you touch it. 478 presets across 8 moods confirm the accessibility. This is mastery of musical immediacy." |
| **Ciani** | 8.5 | "The Finish LowMono parameter — keeping sub frequencies mono while harmonics spread — is a sophisticated spatial decision. But the Space FX sums to mono before the Schroeder network. I want the shimmer to breathe stereo independently." |
| **Schulze** | 8.5 | "PLAY DEAD multiplies release up to 5× while closing the filter — decay to silence as a continuous control. That is temporal synthesis. The engine's identity is a character instrument, not a cosmic one, but it touches the long-form timescale with intention." |
| **Vangelis** | 9.5 | "Aftertouch adds BITE macro intensity +0.3. Mod wheel adds BITE macro depth +0.4. When you press harder the sound becomes more feral — gnash rises, resonance opens. This is not a parameter. This is an emotional instrument." |
| **Tomita** | 9.0 | "Five oscillator layers — OscA (4 waveforms), OscB (5 waveforms), Sub, Weight Engine (5 shapes/3 octaves), Noise (5 types) — plus 4 character processing stages. The timbral range covers electric bass to industrial noise to soft fuzz. This is an orchestra section." |

**Consensus Score: 9.1 / 10 — Tier 1 V1 PASS**

---

## Doctrine Compliance

| Doctrine | Status | Commentary |
|----------|--------|------------|
| **D001** Velocity Must Shape Timbre | **PASS** | `filtEnvAmt * filtEnvVal * voice.velocity * 10000.0f` at render time. Velocity scales filter envelope depth directly — the widest velocity→filter sweep in the fleet. Velocity also scales amplitude via `velGain`. |
| **D002** Modulation is the Lifeblood | **PASS** | 3 LFOs × 7 shapes (Sine/Tri/Saw/Square/S&H/Random/Stepped) with tempo sync, retrigger, start phase. 8-slot mod matrix: 16 sources (LFOs, 3 envelopes, velocity, note, aftertouch, mod wheel, all 5 macros) × 26 destinations (oscillators through FX). 3 ADSRs with Mod Envelope routing to 8 destinations. Richest modulation system in the fleet. |
| **D003** The Physics IS the Synthesis | **N/A** | Overbite is a character synthesis engine. No physical modeling claimed. |
| **D004** Dead Parameters Are Broken Promises | **PASS** | All 122 parameters read in ParamSnapshot and dispatched into DSP. Computation caches (exp, pow) confirmed — audio-thread safe. No dead parameters found. |
| **D005** An Engine That Cannot Breathe | **PASS** | BiteLFO `setRate()` clamps to floor 0.01 Hz. Three LFOs available at geological timescales. PLAY DEAD macro extends release up to 5× for temporal decay synthesis. |
| **D006** Expression Input Is Not Optional | **PASS** | Aftertouch → BITE macro intensity +0.3 (line 1568). Mod wheel (CC1) → BITE macro depth +0.4 (line 1568/1619). Pitch bend ±2st. All 5 macros as real-time controls. MPE per-voice expression via `MPEVoiceExpression`. |

---

## Sonic Identity

**Unique voice:** The fleet's character bass and dirt engine. Timbral range from warm sub-bass to industrial noise burst within a single engine. The 5-stage character chain (Fur → FilterDrive → Filter → Gnash → Trash) creates timbres that exist between electric bass, synth lead, and industrial percussion.

**Architecture highlights:**
- **BiteOscB Interaction Modes**: Soft Sync (amplitude product), Low FM, Phase Push, Grit Multiply (ring mod quantized to 16 steps) — oscillators that interact rather than just mix
- **PLAY DEAD macro**: extends release ×5, ducks level 80%, closes filter -4kHz — decay to silence as a continuous performance gesture
- **5 macros as mod matrix sources**: macros 11–15 in the 8-slot matrix, making real-time macro positions part of the patch routing

**Dry patch quality:** Excellent. OscA sine + sub at defaults gives a warm, clean sub-bass immediately responsive to the filter. The character chain provides color without needing FX.

**Character range:** `filterCutoff: 800Hz, gnashAmount: 0.0` (plush, warm, round) vs. `trashMode: Crushed, trashAmount: 0.8, gnashAmount: 0.9, filterMode: HP` (industrial noise burst) — unrecognizably different timbres from the same engine.

---

## Preset Review

| Preset | Schema | Assessment |
|--------|--------|------------|
| **Gnash Bass** (Foundation) | NEW ✅ | Working poss_ schema. All 5 macros. BITE 0.7, gnashAmount 0.6. DNA accurate: aggression 0.75. Tight amp envelope delivers the name. |
| **Mod Wheel Fang** (Flux) | NEW ✅ | Demonstrates D006. Velocity→OscAShape, velocity→OscMix via mod matrix. Named presets that teach the engine — Kakehashi accessibility. |
| **Mood distribution** | — | Foundation(20), Prism(15), Atmosphere(14), Flux(13), Aether(12), Submerged(8), Family(7), Entangled(2). 478 total — excellent overall, but Entangled(2) is disproportionate. |

All reviewed presets use current poss_ prefix. No stale schema issue.

**Macro effectiveness:** All 5 macros produce distinct, audible, non-overlapping changes. Best macro design in fleet — confirmed B008.

---

## Coupling Assessment

**`getSampleForCoupling()`:** Stereo audio (ch0/1) + peak envelope (ch2).

**`applyCouplingInput()`:**
| Type | Effect |
|------|--------|
| `AmpToFilter` | Partner amplitude pumps filter cutoff |
| `AudioToFM` | **Per-sample buffer FM** — stores `const float* externalFMBuffer` pointer, reads sample-accurate in voice loop. Only buffer-based FM coupling in the fleet. |

The AudioToFM implementation is the most thorough in the fleet — true sample-rate audio FM, not a block-rate scalar approximation.

**Natural partners:** Onset (drum→filter pump), Orbital (group envelope modulation), Oracle (GENDY audio FM), Ouroboros (chaos attractor FM).

**Entangled gap:** 2 Entangled presets for 478 total is the biggest missed opportunity in the preset library.

---

## Blessings

**B008 — Five-Macro System** (previously awarded) ✅ Confirmed and scope extended: the 5 macros are mod matrix sources 11–15, not just blend controls. Macros-as-modulation-sources is the full realization of B008.

**B045 — BiteOscB Interaction Ecology** (NEW — proposed)
Four interaction modes between OscA and OscB go beyond standard mixing:
- **Soft Sync:** amplitude product of both oscillators — pseudo-sync FM
- **Low FM:** OscA → OscB frequency (gentle FM modulation)
- **Phase Push:** OscA nudges OscB phase (subtle phase interference)
- **Grit Multiply:** ring mod then quantize to 16 steps (unique industrial character)

No other fleet engine implements oscillator-to-oscillator interaction as a named mode choice. The Grit Multiply result is unavailable from any other synthesis path. Proposing B045.

---

## Debate Relevance

| Debate | Position |
|--------|----------|
| **DB003** Init patch | Sine + sub defaults = warm, inviting, immediately shapeable via macros. Sides with "immediate beauty." |
| **DB004** Expression vs. Evolution | Clearly sides with **Expression**. Aftertouch + mod wheel to BITE, velocity to filter, 5 performance macros. The engine is designed for playing. Evolution (PLAY DEAD, long-tail FX) is available but secondary. Overbite + OpenSky together demonstrate the full DB004 spectrum. |

---

## Recommendations

**Priority 1 — Verify before V1:**
- **PLAY DEAD in `macroLabels`**: All reviewed presets have 4 entries in `macroLabels` (BELLY/BITE/SCURRY/TRASH), missing PLAY DEAD. The macro is dispatched in DSP and present in the `macros` dict — but if the UI labels knobs from `macroLabels`, the 5th knob appears unlabeled. Verify against UI implementation. If unlabeled, extend all preset `macroLabels` arrays to 5 entries.

**Priority 2 — Post-seance improvement:**
- **Entangled preset expansion**: 2 Entangled is disproportionate for this coupling capability. Target: at minimum 15–20 Entangled presets demonstrating AudioToFM with Oracle/Ouroboros, AmpToFilter with Onset.

**Priority 3 — Post-V1:**
- Stereo Space FX: `BiteSpaceFX` sums to mono before Schroeder network. Stereo dual-network would address Ciani's critique and align with the fleet standard.
- Formally ratify B045 (BiteOscB Interaction Ecology).

---

## Score History

| Date | Score | Context |
|------|-------|---------|
| Pre-2026-04-12 | "Full approval (~9.2 est.)" | Partial seance; best macro system in fleet, no numeric score on record |
| **2026-04-12** | **9.1/10** | Formal re-seance. 122 params confirmed clean. All doctrines PASS. B045 proposed. V1 PASS — Tier 1. |
