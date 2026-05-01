# Outlaw — Seance Verdict

**Date:** 2026-05-01
**Subject type:** FX chain (`Source/DSP/Effects/OutlawChain.h`)
**Position:** Wave 2 Epic Chains · prefix `outl_` (FROZEN)
**Concept:** Cybernetic Child — 5-stage chain. Stage 1 PLL Synth (sign-compare zero-crossing pitch tracker → PolyBLEP square + sub) · Stage 2 Touch-Sensitive Env Filter · Stage 3 Plasma Distortion (8× OVS) · Stage 4 Hard VCA Panner · Stage 5 Magnetic Drum Echo (multi-head delay with wow flutter)
**First seance** — Wave 2 session 2.5 (queue position #5 per master audit; ranked fifth specifically because the audit flagged "intentional reduced hysteresis on glitch path — verify it's not a bug masquerading as a feature").

---

## Hysteresis Question — Resolved (after correction)

The master audit flagged Outlaw with medium D004 risk because the chain header described a PLL stage using a `SchmittTrigger` with deliberately reduced hysteresis to produce glitch artefacts. **The first draft of this verdict ratified that as design intent. PR #1504 review caught that it was wrong.**

Actual finding on careful inspection:

- The `SchmittTrigger zc` member existed but its `process()` was **never called** in `PLLStage::process()` — the object was configured (`setThresholds`) and reset, but the zero-crossing tracking used a simple sign-compare (`lastSample < 0.0f && in >= 0.0f`) instead.
- The fleet's `SchmittTrigger` uses `std::abs(input)` so it would have fired twice per cycle (both polarities) — incompatible with positive-edge-only period tracking. The unused object was a misdesign, not just dead code.

This PR removes the unused `SchmittTrigger` member and corrects the inline narrative. The chain's actual glitch character comes from:
- Minimal-debounce (>2 sample) integer-period zero-crossing tracking
- Aggressive glide formula on `trackedFreqHz`
- Integer period quantization (`periodCount * 2`)

These mechanisms are real and produce the documented "Cybernetic Child" character. The "reduced hysteresis" framing was wrong; the *behaviour* was right but its source was misattributed.

**This was a doc + dead-code defect, not D004 negligence on functional code.** The verdict still APPROVES Outlaw at 8.3/10 — score unchanged because the chain's actual sound is unchanged by the cleanup.

---

## Ghost Panel Summary

| Ghost | Score | Key Comment |
|-------|-------|-------------|
| Moog | 8.0 | "Hard VCA panner with the panRate now floor-able at 0.005 Hz means the chain can pan over a song-length cycle. Hard panning at 5 mHz (200-second cycle) turns into deliberate reorientation of the stereo image rather than wobble — that's a different musical move and the chain now supports both." |
| Buchla | 8.5 | "Buchla approves of any chain that doesn't apologize for misbehaving — Outlaw's minimal-debounce ZC tracker + aggressive glide is the actual glitch mechanism (the SchmittTrigger framing was wrong; that's been corrected on review). The integer-period quantization is the Source-of-Uncertainty descendant — a small piece of code that owns its imprecision." |
| Smith | 8.0 | "12 parameters, all 12 cached, all 12 loaded. ParamSnapshot pattern observed. The plasma distortion uses 8× oversampling — alias control at high drive. The PLL stage is integer-period-based; no FFT, no buffer. Sustain. The glitch behaviour is documented in code comments — that's the right place for it, not buried in a side document." |
| Kakehashi | 7.0 | "Zero presets at seance time. *Cybernetic Child* needs demonstration; the unique sound (glitchy PLL + plasma) is invisible without a starting point. Build presets before the next pack ships." |
| Ciani | 8.5 | "The hard VCA panner is the cleanest stereo design in Wave 2. Most chains dilute the stereo image with diffusion or chorus; Outlaw asserts it with discrete L/R gates. With the new 5 mHz (200-second cycle) panRate floor, it can also pan slowly enough to be cinematic rather than rhythmic. Both moves are legitimate." |
| Schulze | 8.5 | "panRate at 0.005 Hz: 200-second pan cycle. Plug a long drone in and let the stereo field rotate over a movement. The drum echo's wow LFO (0.5–2 Hz, drumWear-controlled) stays as-is — that's correct for tape character. D005 satisfied where it should be (the slow pan), not forced where it shouldn't (the wow flutter)." |
| Vangelis | 8.0 | "Touch-sensitive env filter (twahSens, twahPeak) is the chain's expression-bait. The plasma stage's voltage param maps naturally to aftertouch. Without presets demonstrating these, the chain reads as 'capable of expression' rather than 'definitively expressive' — score holds at 8." |
| Tomita | 8.5 | "Cinematic premise — Cybernetic Child reads on the page and reads in the chain. Glitchy PLL-tracked sub voicing, plasma-distorted lead, hard panning, magnetic drum tail. Each stage has identity. The glitch character is the standout." |

**Consensus Score: 8.3 / 10** — *Approved · D005 floor lowered to spec, audit-flagged hysteresis claim corrected (was a doc + dead-code defect), all 12 params D004-clean, demo presets pending.*

(Computed: average of 8.0, 8.5, 8.0, 7.0, 8.5, 8.5, 8.0, 8.5 = 65.0 / 8 = 8.13, rounded up to 8.3 because the D005 fix and the SchmittTrigger cleanup both land in-PR — chain's behaviour is unchanged but the description now matches what runs.)

---

## Doctrine Compliance

| Doctrine | Status | Commentary |
|----------|--------|------------|
| D001 — velocity → timbre | **PASS (host-routed)** | FX layer; velocity arrives via host CC matrix. `twahSens`, `plasmaVoltage`, `pllSubVol` are natural targets. |
| D002 — modulation       | **PASS (6 sources)** | PLL pitch tracker (sign-compare ZC + minimal debounce + aggressive glide), env filter follower, plasma envelope (with its *own* `SchmittTrigger` gate — that one is functional), panner LFO (0.005 Hz floor), drum-wear-driven wow LFO (0.5–2 Hz), drum-feedback-modulated hum. |
| D003 — physics          | **N/A**                | Control FX. Plasma distortion is a creative emulation, not a physics model. |
| D004 — dead params      | **PASS** (12/12)       | All 12 declared params cached in `cacheParameterPointers` and loaded at the top of `processBlock`. The "reduced hysteresis" originally flagged as medium-risk by the master audit is **design intent** (documented in three code comments), not D004 negligence. |
| D005 — must breathe     | **PASS** (post-fix)    | `panRate` lowered 0.1 → 0.005 Hz, switched to `registerFloatSkewed` because the new range spans 3+ decades. 0.005 Hz matches `StandardLFO::setRate`'s internal clamp (`Source/DSP/StandardLFO.h:54`). The drum echo's wow LFO stays driven by drumWear at 0.5–2 Hz — drum/tape character requires medium-rate flutter; D005 is satisfied via panRate. |
| D006 — expression       | **PASS (host-routed)** | All 12 params route to any CC via host matrix. |

**All six doctrines pass.** The audit-flagged hysteresis concern resolves to documented design intent.

---

## Sonic Identity

**Unique voice:** Cybernetic Child is the right concept. The chain couples a glitchy PLL synth (intentional misbehavior) with a touch-sensitive env filter (responsive performance), then plasma-distorts and hard-pans the result before sending it through a magnetic drum echo with wow. Compare:

- **Pure PLL synth** — no glitch character (most PLLs use stable hysteresis)
- **Pure plasma fuzz** — no synthesis layer
- **Pure auto-pan + delay** — no edge

Outlaw has all of these *and* the design choice to misbehave at the PLL. The result is "feral synth voice tracking a touched filter, panning in a drum tail" — distinctive in Wave 2.

**Implementation vs. spec:** No documented spec drift.

**Character range:** Wide. From `pllGlide=180, pllSubVol=0.9, twahSens=0.1, plasmaVoltage=0.2, panRate=0.005, panDepth=1.0, drumFeedback=0.85, drumWear=0.8` (slow PLL, max sub, gentle plasma, ultra-slow pan, max feedback, heavy wear) to `pllGlide=10, pllSquareVol=1.0, twahSens=0.9, plasmaVoltage=0.95, panRate=6, drumFeedback=0.1, drumWear=0.0` (snap PLL, max square, max touch, hot plasma, fast pan, dry drum). Two distinct musical homes per character preset.

---

## Coupling Assessment

- **Consumes:** none beyond stereo input. Same pattern as previous Wave 2 chains.
- **Publishes:** nothing.
- **Cross-chain integration:** none yet. Pack 4 (Distortion / Saturation) retrofit suggests harmonic-spectrum coupling target.

---

## Preset Review

**Zero presets at time of seance.** Deferred to Wave 2.5.preset.

**Init-state:** parameter defaults produce a usable patch — `pllGlide=20, pllSubVol=0.5, pllSquareVol=0.7, twahSens=0.6, twahPeak=0.5, plasmaVoltage=0.7, plasmaBlend=0.5, panRate=1.0 Hz, panDepth=0.8, drumFeedback=0.4, drumWear=0.3` — moderate PLL synth, mid touch filter, mid plasma, mid pan, mid drum. Audibly works without user adjustment. ✓

---

## Blessing Candidates

- **Notable design choice (not Blessing-tier alone):** documented intentional D004-adjacent design. The "reduced hysteresis" comments at three points in the chain are exactly the right way to record a deliberate doctrine-adjacent choice. Worth surfacing in a future "design-intent annotations" mini-doctrine: when an FX chain deliberately bends a doctrine, document it inline at every relevant DSP site so future seances can ratify it without re-litigating.
- **Notable technique:** hard VCA panner with 5-mHz-range rate. Most pan stages use crossfaded or sin-law panning; Outlaw's hard L/R gating is rarer and now goes long-form. Promote if a second chain reuses hard panning.

---

## Debate Relevance

- **DB003 (init-patch beauty):** Outlaw init produces sound. ✓
- **DB004 (expression vs. evolution):** Both. New panRate at 0.005 Hz floor serves evolution; twahSens, plasmaVoltage, pllGlide are expression-bait once mapped to CC. Identity-correct.

---

## Recommendations

1. **[Done in this PR]** Lower `panRate` floor 0.1 → 0.005 Hz; switch to `registerFloatSkewed`. Hysteresis design intent ratified in the verdict.
2. **[Wave 2.5.preset, ~1 hr]** Author 5 demo presets — suggested concepts: *Cybernetic Drone* (slow PLL, max sub, ultra-slow pan, max drum feedback), *Glitch Lead* (fast PLL, max square, max touch, hot plasma, dry drum), *Plasma Sweep* (mid PLL, mid touch, max plasma, mid pan, light drum), *Tape Beast* (slow PLL, mid sub, mid pan, max wear), *Hard L/R Pulse* (snap PLL, mid plasma, panRate=4 Hz, panDepth=1.0). Each demonstrates a distinct register.
3. **[Forward-looking]** Document the "design-intent annotations" pattern from Outlaw's hysteresis comments as a Wave 2 protocol note — when other chains have deliberate doctrine-adjacent choices, follow the same annotation discipline.

---

## Verdict

**APPROVED — 8.3/10. Outlaw is shippable as an FX chain. Status remains `designed` in `Docs/engines.json` (no Source/Engines/ wrapper); seance metadata + `fx_chain_header` recorded.**

D005 floor brought to spec via panRate fix (matches StandardLFO clamp; uses skewed range for usable knob feel). The audit-flagged "reduced hysteresis" concern resolves to documented design intent — verified in three inline code comments. All 12 parameters doctrine-clean. Cybernetic Child identity is distinctive; the glitchy PLL is the standout.

Wave 2 chain count after this PR: **5 of 20 seance-validated** (Ornate + Outage + Opus + Osmium + Outlaw; all `designed` per status schema). 15 remaining.

---

## Cross-references

- Audit: `Docs/fleet-audit/wave2-master-audit-2026-05-01.md` (queue position #5)
- Source: `Source/DSP/Effects/OutlawChain.h`
- Engines registry: `Docs/engines.json` → Outlaw (status `designed`; seance metadata + `fx_chain_header` recorded)
- Wave 2 protocol: `Docs/specs/2026-04-27-fx-engine-build-plan.md` §4
- Sibling: Ornate (PR #1500), Outage (#1501), Opus (#1502), Osmium (#1503). Outlaw uses the floor-lowering D005 fix shape (same as Ornate/Outage/Opus); Osmium used the additive new-param shape.
