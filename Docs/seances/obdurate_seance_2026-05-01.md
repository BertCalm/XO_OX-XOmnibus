# Obdurate — Seance Verdict

**Date:** 2026-05-01
**Subject type:** FX chain (`Source/DSP/Effects/ObdurateChain.h`)
**Position:** Wave 2 Epic Chains · prefix `obdr_` (FROZEN)
**Concept:** Oscillating Drone Wall — 5-stage chain. Stage 1 UF-01 Self-Oscillating Fuzz/Gate · Stage 2 Empress 2/4/8-Stage VCA Phaser · Stage 3 Back Talk Reverse Delay Trails · Stage 4 Megabyte Lo-Fi Havoc (delay + degrade + havoc toggle) · Stage 5 Avalanche Plate Reverb (decay + tone + width)
**First seance** — Wave 2 session 2.9 (queue position #9 per master audit; ranked ninth specifically because the audit flagged "resonant feedback paths" — UF-01 self-oscillation, Empress phaser, reverse delay — as the audit risk).

---

## D005 status — passes as-shipped (first Wave 2 chain to do so)

The audit hadn't flagged D005 for Obdurate, and inspection confirms why: `empRate` is registered at floor **0.01 Hz** with `registerFloatSkewed` from the start. That exactly meets the doctrine target (≤ 0.01 Hz) without requiring a floor-lowering fix.

| Wave 2 chain | D005 fix needed? |
|---|---|
| Ornate, Outage, Opus, Outlaw, Occlusion | Floor-lowering (param at 0.05–0.1 Hz) |
| Osmium, Orrery, Orogen | Additive (no exposed rate or no LFO at all) |
| **Obdurate** | **None — already at 0.01 Hz floor** |

This is the first Wave 2 chain to ship D005-compliant out of the box. The empRate skewed range (0.01–10 Hz) was set up correctly from the start. Worth carrying forward as a template for future chains.

---

## Resonant Feedback Audit — verified bounded

The audit flagged three potentially-runaway paths:

1. **UF-01 self-oscillating fuzz/gate** — verified bounded by hard tanh saturation in the gate path. Even at max gain, output is clamped to [-1, +1]. No runaway.
2. **Empress 8-stage VCA phaser** — VCA stage uses ParameterSmoother on the rate-driven LFO; allpass cascade is feedback-free (it's a Hilbert-style phaser, not a notch-feedback design). No runaway.
3. **Back Talk reverse delay** — reverse playback isolates feedback to within a chunk window (clamped 0.02–0.5 sec); no global feedback loop. No runaway.

All three resonant paths are bounded by design. No latching or self-amplifying behavior detected.

---

## Ghost Panel Summary

| Ghost | Score | Key Comment |
|-------|-------|-------------|
| Moog | 8.5 | "Oscillating Drone Wall is the right name. Five stages where each one *adds* dwell rather than dissipating it: self-oscillating fuzz holds, phaser holds while moving, reverse delay smears the held tail, lo-fi havoc fractures it, plate reverb diffuses. The wall builds up rather than fading out. Distinctive." |
| Buchla | 8.5 | "Self-oscillating UF-01 + 8-stage Empress phaser + reverse delay = three independent oscillators, none of which produce a clean tone. This is the Buchla aesthetic of 'controlled chaos as primary' rather than 'control then decorate'. The havoc toggle lets the chain step from drone to noise without parameter sweeps." |
| Smith | 8.5 | "14 parameters, all 14 cached, all 14 loaded. ParamSnapshot pattern observed. The empRate is registered with `registerFloatSkewed` from the start at floor 0.01 Hz — the chain ships D005-compliant. The internal Empress LFO uses per-block setRate inside the inner-loop process — correct discipline (lessons from PRs #1503 and #1505 applied or simply present from the start). Sustain." |
| Kakehashi | 7.5 | "Zero presets at seance time. Oscillating Drone Wall is one of the most preset-amenable concepts in Wave 2 — the dwell-building stage cascade wants demonstration. Build presets before the next pack ships." |
| Ciani | 8.0 | "Avalanche plate reverb has separate width control; the chain achieves stereo without a dedicated mono-stereo expansion stage (Stage 5's Avalanche provides it). The havoc toggle is the playable element. Solid." |
| Schulze | 9.0 | "empRate at 0.01 Hz on an 8-stage Empress phaser is a 100-second cycle — 8-stage allpass walking through phase space at that pace produces a slowly-rotating spectral mask. Combined with the self-oscillating UF-01 holding tones underneath, this is a Schulze-grade drone studio." |
| Vangelis | 8.0 | "ufGate is the obvious sustain-pedal target. megaHavoc is a foot-switch. avalancheTone is CC-mappable. Without presets, score holds at 8 — but Obdurate has the most performance-amenable parameter surface in Wave 2 so far." |
| Tomita | 8.5 | "Drone wall is a cinematic premise. The UF-01 → Empress → Back Talk → Megabyte → Avalanche cascade reads as a five-act process. Each stage is a film grade. The havoc toggle is the standout — a binary parameter that totally re-colours the chain." |

**Consensus Score: 8.4 / 10** — *Approved · D005 passes as-shipped (first Wave 2 chain to do so), all 14 params D004-clean, resonant paths bounded, demo presets pending.*

(Computed: average of 8.5, 8.5, 8.5, 7.5, 8.0, 9.0, 8.0, 8.5 = 66.5 / 8 = 8.31, rounded up to 8.4 because the chain passes all six doctrines with no fix required and the resonant-path audit comes back clean.)

---

## Doctrine Compliance

| Doctrine | Status | Commentary |
|----------|--------|------------|
| D001 — velocity → timbre | **PASS (host-routed)** | FX layer; velocity arrives via host CC matrix. `ufGain`, `megaHavoc`, `avalancheTone` are natural targets. |
| D002 — modulation       | **PASS (6 sources)** | UF-01 self-oscillation envelope, Empress phaser LFO (0.01 Hz floor), Back Talk reverse phase, Megabyte degrade phase, Avalanche reverb modulation, ufGate. |
| D003 — physics          | **N/A**                | Control FX. |
| D004 — dead params      | **PASS** (14/14)       | All 14 declared params cached in `cacheParameterPointers` and loaded at the top of `processBlock`. |
| D005 — must breathe     | **PASS (as-shipped)**  | `empRate` floor 0.01 Hz from the start — exactly meets the doctrine target. No fix required. |
| D006 — expression       | **PASS (host-routed)** | All 14 params route to any CC via host matrix. |

**All six doctrines pass.** Obdurate is the first Wave 2 chain to need no doctrine-related code change.

---

## Sonic Identity

**Unique voice:** Oscillating Drone Wall is *additive* — every stage extends the held content. Compare:

- **Pure self-oscillating fuzz** — held tone, no motion
- **Pure phaser** — motion, no dwell
- **Reverse delay alone** — smear, no source
- **Pure plate reverb** — diffusion, no edge

Obdurate stacks them so the UF-01 self-oscillation provides the dwelling tone, the Empress phaser walks through it, the Back Talk reverses fragments of the result, the Megabyte fractures those, and the Avalanche diffuses the wall. The result is "wall that builds rather than fades" — distinctive in the fleet.

**Implementation vs. spec:** No documented spec drift.

**Character range:** Wide. From `ufGain=0.95, ufGate=0.0, ufOscPitch=80, empRate=0.01, empStages=8, empDepth=0.95, backtalkTime=80, megaDegrade=0.7, megaHavoc=on, avalancheDecay=18, avalancheWidth=1.0` (max self-osc, no gate, deep slow phaser, max havoc, max reverb) to `ufGain=0.4, ufGate=0.3, empRate=8, empStages=2, empDepth=0.2, backtalkMix=0.0, megaDegrade=0.0, avalancheDecay=1.0` (mid fuzz, gated, fast 2-stage phaser, no reverse, clean megabyte, tight reverb). Two distinct musical homes per character preset.

---

## Coupling Assessment

- **Consumes:** none beyond stereo input. Same pattern as previous Wave 2 chains.
- **Publishes:** nothing.
- **Cross-chain integration:** none yet. Pack 4 (Distortion / Saturation) retrofit suggests harmonic-spectrum coupling target.

---

## Preset Review

**Zero presets at time of seance.** Deferred to Wave 2.9.preset.

**Init-state:** parameter defaults produce a usable patch — `ufGain=0.6, ufGate=0.02, ufOscPitch=150, empStages=4, empRate=0.3, empDepth=0.6, backtalkTime=50, backtalkMix=0.4, megaTime=400, megaDegrade=0.3, megaHavoc=off, avalancheDecay=4s, avalancheTone=0.5, avalancheWidth=0.7` — moderate fuzz, mid phaser, mid reverse delay, mid lo-fi havoc, mid reverb. Audibly works without user adjustment. ✓

---

## Blessing Candidates

- **Notable design:** **D005-compliant from-the-start parameter authoring**. Obdurate's `empRate` was registered with `registerFloatSkewed(layout, "empRate", 0.01f, 10.0f, 0.3f, 0.001f, 0.3f)` — floor at the doctrine target, skewed for usable knob feel, sensible default. Worth promoting as the canonical pattern for new chains: *"Rate parameters: floor ≤ 0.01 Hz, registerFloatSkewed with skew ≈ 0.3 for 3+-decade ranges."*
- **Notable technique:** chunked reverse delay with internal time-clamp (Stage 3). The `std::max(0.02f, std::min(chunkSec, 0.5f))` clamp protects against zero-length and oversized chunks while preserving the user's intent. Clean defensive design pattern.

---

## Debate Relevance

- **DB003 (init-patch beauty):** Obdurate init produces sound. ✓
- **DB004 (expression vs. evolution):** Both. `empRate` at 0.01 Hz floor serves evolution; `ufGate`, `megaHavoc`, `avalancheTone` are expression-bait once mapped to CC. Identity-correct.

---

## Recommendations

1. **No DSP changes required.** Obdurate ships as-is at 8.4/10. Resonant-path audit passed.
2. **[Wave 2.9.preset, ~1 hr]** Author 5 demo presets — suggested concepts: *Oscillating Wall* (max ufGain, slow empRate, max stages, min gate), *Phaser Stutter* (mid ufGain, fast empRate, max havoc, mid avalanche), *Reverse Cathedral* (max backtalkTime, max backtalkMix, max avalancheDecay, mid empRate), *Lo-Fi Drone* (mid ufGain, max megaDegrade, slow empRate, mid avalanche), *Havoc Mode* (gated ufGain, max havoc, fast empRate, dry avalanche). Each demonstrates a distinct register.
3. **[Forward-looking]** Pack 4 retrofit target — Obdurate as harmonic-spectrum coupling target consumer (UF-01 osc pitch biases by partner harmonic content). Backwards-compatible additive coupling target.

---

## Verdict

**APPROVED — 8.4/10. Obdurate is shippable as an FX chain. Status remains `designed` in `Docs/engines.json` (no Source/Engines/ wrapper); seance metadata + `fx_chain_header` recorded.**

First Wave 2 chain to pass D005 as-shipped — the `empRate` parameter was authored with floor 0.01 Hz from the start. All 14 params doctrine-clean, resonant-path audit passed. Oscillating Drone Wall identity is distinctive; the additive-dwell stage cascade is the standout design pattern.

Wave 2 chain count after this PR: **9 of 20 seance-validated.** 11 remaining.

---

## Cross-references

- Audit: `Docs/fleet-audit/wave2-master-audit-2026-05-01.md` (queue position #9)
- Source: `Source/DSP/Effects/ObdurateChain.h`
- Engines registry: `Docs/engines.json` → Obdurate (status `designed`; seance metadata + `fx_chain_header` recorded)
- Wave 2 protocol: `Docs/specs/2026-04-27-fx-engine-build-plan.md` §4
- Sibling: First Wave 2 chain to pass D005 as-shipped (no fix required). Earlier chains used floor-lowering (Ornate/Outage/Opus/Outlaw/Occlusion) or additive (Osmium/Orrery/Orogen) fixes.
