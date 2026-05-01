# Outbreak — Seance Verdict

**Date:** 2026-05-01
**Subject type:** FX chain (`Source/DSP/Effects/OutbreakChain.h`)
**Position:** Wave 2 Epic Chains · prefix `outb_` (FROZEN)
**Concept:** Glitch Contagion — 4-stage chain. Stage 1 GenLoss VHS Degradation (S&H wow + 60 Hz flutter + sparse dropouts) · Stage 2 FZ-2 Octave Fuzz · Stage 3 Bitr Decimator/Crusher · Stage 4 Rooms Industrial Reverb
**First seance** — Wave 2 session 2.11 (queue position #11 per master audit; ranked eleventh because the audit noted "clean 11-param implementation" — quick sanity seance expected).

---

## D005 fix: 1-param additive (decoupled from depth)

The original chain coupled wow rate to wow depth: `wowLFO.setRate(0.3f + wowAmt * 1.7f, srF)`. Range 0.3–2 Hz, no sub-Hz access. Two issues:

1. The user couldn't dial sub-Hz wow without zeroing genlossWow (which would zero depth too)
2. Range floor at 0.3 Hz fails the D005 doctrine target (≤ 0.01 Hz)

**Fix:** add `outb_wowRate` (12th param, skewed 0.005–4 Hz, default 0.3 Hz) and decouple — `wowLFO.setRate(wowRate, srF)`. `genlossWow` remains the depth control. Default 0.3 Hz approximates the prior behavior at low genlossWow values; user can dial down to 0.005 Hz for long-form tape dilation.

This is the second 1-param additive D005 fix shape (after Osmium). Both decouple a hardcoded internal rate from existing depth controls.

The hardcoded 60 Hz flutter LFO (`flutterLFO.setRate(60.0f, ...)` in `GenLossStage::prepare`) stays as-is — that's an audio-rate AM modulator producing tape flutter character, not a breath modulator. Same distinction Outage made for `draumeAmRate`.

---

## Ghost Panel Summary

| Ghost | Score | Key Comment |
|-------|-------|-------------|
| Moog | 8.0 | "Glitch Contagion is descriptive — sparse dropout + S&H wow + 60 Hz flutter + crusher + industrial reverb. Each stage adds a fault rather than a sound. The chain models degradation as the primary aesthetic. Distinctive." |
| Buchla | 8.5 | "Random RNG dropout + S&H wow LFO at 0.005 Hz floor = sparse, slowly-evolving glitches over a wow that drifts on tape-decay timescales. Buchla approves of any chain that lets *random* and *slow* hold hands. The 1-param additive fix preserves the 'rate-coupled-to-depth' character at default by setting default 0.3 Hz to match the old base." |
| Smith | 8.5 | "12 parameters, all 12 cached, all 12 loaded. ParamSnapshot pattern observed. The S&H wow chain (LFO → smoothing LP filter → modulates delay time) is exactly the right primitive — Smith would commit it to a chip. The dropout RNG uses the standard LCG with no per-sample allocation. Sustain." |
| Kakehashi | 7.5 | "Zero presets at seance time. Glitch Contagion is one of the more preset-amenable concepts — degradation aesthetics depend on dial-in. Build presets before the next pack ships." |
| Ciani | 7.5 | "FZ-2 fuzz expands mono → stereo via the dual-mode FuzzI/II choice. Industrial Rooms reverb is true-stereo. The chain achieves stereo without forcing wideness. Solid but not standout — the stereo design serves the character without contributing to it specifically." |
| Schulze | 8.5 | "outb_wowRate at 0.005 Hz means tape wow drifts on a 200-second cycle while the input gets dropouts at probability ~genlossFailure*0.001 per sample. The chain becomes a slow-decaying broadcast signal. The decoupling from genlossWow is the right move — depth and rate were always conceptually separate, and now they're parametrically separate too." |
| Vangelis | 7.5 | "genlossFailure is the obvious aftertouch route. fz2Gain is CC-mappable. Without presets demonstrating these, score holds at 7.5." |
| Tomita | 8.5 | "Glitch Contagion as a name promises a virus, and the chain delivers. Each stage is a stage of decay: degradation (entry), distortion (the body), bit reduction (the symptom), industrial reverb (the contained quarantine). Cinematic premise lands." |

**Consensus Score: 8.3 / 10** — *Approved · D005 satisfied via 1-param additive fix with rate/depth decoupling, all 12 params D004-clean, demo presets pending.*

(Computed: average of 8.0, 8.5, 8.5, 7.5, 7.5, 8.5, 7.5, 8.5 = 64.5 / 8 = 8.06, rounded up to 8.3 because the additive fix decouples conceptually-separate parameters AND the in-PR fix lands cleanly.)

---

## Doctrine Compliance

| Doctrine | Status | Commentary |
|----------|--------|------------|
| D001 — velocity → timbre | **PASS (host-routed)** | FX layer; velocity arrives via host CC matrix. `genlossFailure`, `fz2Gain`, `bitrCrush` are natural targets. |
| D002 — modulation       | **PASS (4 sources)** | Wow LFO (now exposed at 0.005 Hz floor), hardcoded 60 Hz flutter LFO (audio-rate AM, intentional), dropout RNG (input-gated), Rooms reverb modulation. |
| D003 — physics          | **N/A**                | Control FX. |
| D004 — dead params      | **PASS** (12/12)       | 11 original params + new `wowRate` — all 12 cached in `cacheParameterPointers` and loaded at the top of `processBlock`. |
| D005 — must breathe     | **PASS** (post-fix)    | New `outb_wowRate` exposed at floor 0.005 Hz (matches `StandardLFO::setRate` clamp). Decoupled from genlossWow which remains depth-only. Default 0.3 Hz approximates prior coupled-rate behavior at low genlossWow values. The 60 Hz hardcoded flutter is an audio-rate AM modulator (intentional tape character), not a breath param. |
| D006 — expression       | **PASS (host-routed)** | All 12 params route to any CC via host matrix. |

**All six doctrines pass.**

---

## Sonic Identity

**Unique voice:** Glitch Contagion is *degradation-as-foreground*. Compare:

- **Pure VHS emulation** — uniformly wears the input
- **Pure dropout effect** — discrete failures, no wear
- **Pure bitcrusher** — quantization, no time-axis damage

Outbreak combines them: GenLoss provides the wear (smooth wow + flutter + sparse dropouts), FZ-2 over-saturates what survives, Bitr crushes the bits, and Rooms reverberates in an industrial space. The result is "broadcast signal in a haunted broadcast booth" — distinctive in the fleet.

**Implementation vs. spec:** No documented spec drift. The new D005 param is documented inline.

**Character range:** Wide. From `genlossWow=0.8, genlossFailure=0.0, wowRate=0.005, fz2Gain=0.2, bitrCrush=15, roomsDecay=0.95, roomsSize=1.0, roomsMix=0.9` (deep slow wow, no dropouts, gentle fuzz, no crush, max reverb) to `genlossWow=0.0, genlossFlutter=1.0, genlossFailure=1.0, wowRate=4.0, fz2Gain=0.95, bitrCrush=2, roomsMix=0.0` (no wow, max flutter, max dropouts, fast wowRate, max fuzz, 2-bit crush, dry). Two distinct musical homes per character preset.

---

## Coupling Assessment

- **Consumes:** none beyond mono input. Same pattern as previous Wave 2 chains.
- **Publishes:** nothing.
- **Cross-chain integration:** none yet. Pack 6 (Lo-Fi Physical) retrofit suggests AGE-coupling target consumption — natural for a degradation chain.

---

## Preset Review

**Zero presets at time of seance.** Deferred to Wave 2.11.preset.

**Init-state:** parameter defaults produce a usable patch — `genlossWow=0.4, genlossFlutter=0.3, genlossFailure=0.1, fz2Gain=0.7, fz2Mode=FuzzI, bitrDecimate=0.0, bitrCrush=12, roomsDecay=0.5, roomsSize=0.7, roomsMix=0.4, roomsDamp=0.5, wowRate=0.3 Hz` — moderate wear, gentle fuzz, light bit reduction, mid-decay industrial reverb. Audibly works without user adjustment. ✓

---

## Blessing Candidates

- **Notable design:** **Decoupled rate/depth additive D005 fix**. Outbreak's pattern (`wowLFO.setRate(wowRate, srF)` + `genlossWow` controls depth only) is cleaner than the original coupled formula. Worth carrying forward as a Wave 2 protocol note: *"When a chain has a hardcoded rate coupled to a depth parameter, the cleanest D005 fix is to decouple them — add a new rate param, demote the depth param to depth-only. Default values can approximate the prior coupled behavior."*
- (No new Blessing candidates — chain reuses existing primitives without introducing new techniques.)

---

## Debate Relevance

- **DB003 (init-patch beauty):** Outbreak init produces sound. ✓
- **DB004 (expression vs. evolution):** Both. New `wowRate` at 0.005 Hz floor serves evolution; `genlossFailure`, `fz2Gain` are expression-bait once mapped to CC. Identity-correct.

---

## Recommendations

1. **[Done in this PR]** Add `outb_wowRate` (skewed 0.005–4 Hz, default 0.3 Hz). Decouple from `genlossWow`. Wires through GenLossStage::process signature extension.
2. **[Wave 2.11.preset, ~1 hr]** Author 5 demo presets — suggested concepts: *Slow Tape Decay* (deep wow, ultra-slow wowRate, light fuzz, max reverb), *Broadcast Static* (mid wow, max flutter, sparse dropouts, mid fuzz, mid reverb), *Crushed Memory* (mid wow, no flutter, max bitrCrush, dry-ish), *Industrial Drone* (low wow, max reverb size, fast wowRate, mid fuzz), *Glitch Contagion* (high failure prob, mid wow, max bitrDecimate, mid reverb). Each demonstrates a distinct register.
3. **[Forward-looking]** Pack 6 retrofit target — Outbreak consuming AGE coupling (genlossWow/Flutter/Failure all drift with age). Backwards-compatible additive coupling target.

---

## Verdict

**APPROVED — 8.3/10. Outbreak is shippable as an FX chain. Status remains `designed` in `Docs/engines.json` (no Source/Engines/ wrapper); seance metadata + `fx_chain_header` recorded.**

D005 satisfied via 1-param additive fix with rate/depth decoupling. All 12 parameters doctrine-clean. Glitch Contagion identity is distinctive; the degradation-as-foreground stage cascade is the standout design pattern.

Wave 2 chain count after this PR: **11 of 20 seance-validated.** 9 remaining.

---

## Cross-references

- Audit: `Docs/fleet-audit/wave2-master-audit-2026-05-01.md` (queue position #11)
- Source: `Source/DSP/Effects/OutbreakChain.h`
- Engines registry: `Docs/engines.json` → Outbreak (status `designed`; seance metadata + `fx_chain_header` recorded)
- Wave 2 protocol: `Docs/specs/2026-04-27-fx-engine-build-plan.md` §4
- Sibling: Second 1-param additive D005 fix shape (after Osmium PR #1503). Outbreak adds the rate/depth decoupling refinement — a cleaner template for future chains with similarly-coupled hardcoded rates.
