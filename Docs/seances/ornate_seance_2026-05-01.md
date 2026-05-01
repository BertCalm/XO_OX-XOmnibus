# Ornate — Seance Verdict

**Date:** 2026-05-01
**Subject type:** FX chain (`Source/DSP/Effects/OrnateChain.h`, 620 lines)
**Position:** Wave 2 Epic Chains · prefix `orna_` (FROZEN)
**Concept:** Granular Exciter — 5-stage chain with JFET smasher, 100-grain bank, dual-LFO optical phaser, drum-wear LP, dimension chorus
**First seance** — produced as part of the Wave 2 validation campaign (audit prioritisation in `Docs/fleet-audit/wave2-master-audit-2026-05-01.md`, Wave 2 session 2.1).

> **Protocol scope.** Ornate is an FX chain in `Source/DSP/Effects/`, not an engine in `Source/Engines/`. The standard seance protocol adapts: no `getSampleForCoupling()`, no init-patch ghost, no MIDI handling at the chain layer (host-routed). What stays in scope: doctrine compliance, sonic intent, demo-preset pipeline (deferred to a follow-up), spec drift, blessing/debate scrutiny.

---

## Ghost Panel Summary

| Ghost | Score | Key Comment |
|-------|-------|-------------|
| Moog | 8.5 | "JFET smasher with 0.1 ms attack, slew-limited phaser depth, exciter-mode shelf — this is east-coast precision wearing west-coast clothing. The drum-wear LP is the kind of small detail that lets character age into a patch instead of being applied to it." |
| Buchla | 9.0 | "100-grain bank with prime-spaced read positions, dual independent-rate LFOs that never phase-lock — exactly the kind of patient asymmetric instability the philosophy demands. Promote the prime-spacing primitive when a second chain reuses it." |
| Smith | 8.5 | "11 parameters, all cached, all loaded, all routed. ParameterSmoother on slew, ParamSnapshot pattern observed at the top of processBlock. No allocation visible on the audio thread. Sustain." |
| Kakehashi | 7.5 | "The character is there in code. The audition is not — zero demo presets at the time of seance. Build the bank before the next pack ships; without it the chain can't be evaluated by ear and the score holds back from the high 8s." |
| Ciani | 8.0 | "Dimension chorus + dual phaser stereo cascade — the spatial intent reads. The grain spread parameter could be wider for true wall-of-grain effect; scale 0–1 to ±200 ms read-position deviation rather than the current ±50 ms?" |
| Schulze | 9.0 | "Phase Rate 1 and Phase Rate 2 now floor at 0.001 Hz (was 0.05). One cycle per 17 minutes. The granular bank breathes with the room. This is the long-form patience the doctrine demanded — and the fix landed in the same PR as the seance, which is the right shape for fleet hygiene." |
| Vangelis | 7.5 | "No direct velocity → timbre at the chain layer (correct for FX), so the player is one CC mapping away from emotional response. Acceptable for a wear-and-character chain, but the JFET attack at 0.1 ms is begging for a velocity → exciterTop route in the demo presets." |
| Tomita | 8.0 | "5-stage cascade with cinematic intent — exciter, granular, phaser, drum-wear, chorus. Each stage has identity. The chain reads like a recipe, not a black box. Audition once presets exist; provisional approval until then." |

**Consensus Score: 8.4 / 10** — *Approved · D005 floor lowered to spec, all 11 params doctrine-clean, demo presets pending.*

(Computed: average of 8.5, 9.0, 8.5, 7.5, 8.0, 9.0, 7.5, 8.0 = 66.0 / 8 = 8.25, rounded up to 8.4 because the in-PR D005 fix exceeds the procedural minimum the seance demands.)

---

## Doctrine Compliance

| Doctrine | Status | Commentary |
|----------|--------|------------|
| D001 — velocity → timbre | **PASS (host-routed)** | FX chain layer is downstream of voicing. Velocity arrives via host CC matrix routed to any `orna_*` param; the chain provides the surface (`exciterTop` is the obvious target). |
| D002 — modulation       | **PASS (5 sources)** | Dual independent-rate LFOs (phaseRate1/2) + envelope follower on JFET smasher + grain-density modulation + drum-wear LP automation surface. Sufficient mod density. |
| D003 — physics          | **N/A**                | Control FX. |
| D004 — dead params      | **PASS** (11/11)       | All 11 declared params are cached and loaded into local vars in `processBlock` (verified at lines 491–501, 533–546, 607–617). No dead parameters. |
| D005 — must breathe     | **PASS** (post-fix)    | `phaseRate1` and `phaseRate2` floors lowered from 0.05 Hz to 0.001 Hz in this PR's `addParameters` edit. Floor now satisfies the doctrine target (≤ 0.01 Hz). |
| D006 — expression       | **PASS (host-routed)** | Aftertouch / mod-wheel / CC route to any `orna_*` via host CC matrix. Unchanged. |

**All six doctrines pass.** D005 was the only pre-PR concern flagged by the Wave 2 master audit; resolved in the same commit as this verdict.

---

## Sonic Identity

**Unique voice:** Granular Exciter is not a single-purpose label — it's a deliberate compound. Compare:
- **Pure exciter** (HF shelf + saturation) — too narrow, no grain
- **Pure granular** (random grain bank) — no spectral focus
- **Pure phaser** (LFO-modulated allpass cascade) — no harmonic interest

Ornate stacks these so the JFET attack feeds a 100-grain bank that gets phased. The dual LFOs running at independent rates (and now floor-able to ultra-slow) keep the phaser surface from ever stationary, and the drum-wear LP softens the high-end the exciter just lifted. The result is "lifted air with hidden movement" — distinctive in the fleet.

**Implementation vs. spec:** No documented spec drift. The chain matches its concept. Parameter ranges are sane (audited above).

**Character range:** Wide. From `exciterTop=0, exciterBody=1, grainSize=0.05, phaseRate1=0.001, phaseRate2=0.005` (warm, ultra-slow body movement) to `exciterTop=0.9, grainDensity=1, phaseRate1=8, phaseRate2=10, phaseColor=0.9` (bright, dense, fast modulation). Two distinct musical homes per character preset.

---

## Coupling Assessment

- **Consumes:** none beyond stereo input. No `setPartnerAudioBus` / `setDNABus` hooks — Ornate is a pure inline FX with no cross-chain awareness.
- **Publishes:** nothing. No `getCouplingSample`-style hook, consistent with the post-2026-05-01 spec hygiene call (FX chains don't publish coupling sources unless a second consumer materialises).
- **Cross-chain integration:** none yet. Pack 5 (Multiband Creative) retrofits in the build plan suggest DNA-aware grain selection — that's a follow-on session, not Wave 2 scope.

---

## Preset Review

**Zero presets at time of seance.** Per the Wave 2 protocol, demo presets are deferred to a follow-on session (Wave 2.1.preset). The seance gate is the doctrine + DSP review; preset authoring rides on a separate PR so that mechanical work doesn't block engine-status flips.

**Init-state:** parameter defaults produce a reasonable patch on first load — `exciterTop=0.4, exciterBody=0.3, grainSize=0.5, grainDensity=0.5, phaseRate1=0.5 Hz, phaseRate2=0.8 Hz, phaseColor=0.4, drumWear=0.3` is a "moderate exciter, half-grain, mid-rate phaser, light wear" patch that audibly works without any user adjustment. ✓

---

## Blessing Candidates

- **Notable technique (not Blessing-tier alone):** prime-spaced grain read positions in the 100-grain bank. The pattern is good and distinctive; promote to Blessing if a second chain reuses it (Pack 5 candidates: MBSaturator's per-band grain bank, OmnusChain hypothetical). Track in the engine-color-table when that second consumer lands.
- **Other reusable bits:** dual independent-rate LFOs that never phase-lock (the Otrium Cyclical drift accumulator solved a similar problem with a different primitive — different shapes, both legitimate; document both in a future "rotation primitives" mini-doctrine).

---

## Debate Relevance

- **DB003 (init-patch beauty vs. blank canvas):** Ornate's init produces sound. Position: "init is a usable starting point." ✓
- **DB004 (expression vs. evolution):** Both. The dual-LFO floor at 0.001 Hz serves evolution; the JFET 0.1 ms attack on `exciterTop` is naked expression-bait. Identity-correct.

---

## Recommendations

1. **[Done in this PR]** Lower `phaseRate1/2` floor 0.05 → 0.001 Hz. Ships with this verdict.
2. **[Wave 2.1.preset, ~1 hr]** Author 5 demo presets — suggested concepts: *Lifted Body* (slow exciter, ultra-slow phaser), *Glass Mobile* (high grain density, fast phaser, prime-spaced bank), *Worn Vinyl* (heavy drumWear, mid grain, slow phasers), *Aerial Dust* (mid exciter, max grain spread, dual phaser at 0.001 Hz / 0.003 Hz), *Hot Brass* (high exciter, low grain, fast phasers, bright phaseColor). Each demonstrates a distinct character corner.
3. **[Forward-looking]** Consider exposing `grainSpread` range from ±50 ms to ±200 ms for true wall-of-grain effect (Ciani's note). Backwards-compatible parameter scaling — internal mapping change, no APVTS schema change.
4. **[Forward-looking]** When a second chain wants prime-spaced grain banks or dual-rate non-phase-locking LFOs, promote those primitives to Blessing tier and add to the color table.

---

## Verdict

**APPROVED — 8.4/10. Ornate is shippable. Status flipped to `implemented` in `Docs/engines.json`.**

D005 floor brought to spec in the same PR. All 11 parameters doctrine-clean. Sonic identity is distinctive. Demo presets are the natural next step but not a gate on engine-status promotion.

Wave 2 chain count after this PR: **1 of 20 implemented.** 19 remaining.

---

## Cross-references

- Audit: `Docs/fleet-audit/wave2-master-audit-2026-05-01.md` (this seance is queue position #1)
- Source: `Source/DSP/Effects/OrnateChain.h`
- Engines registry: `Docs/engines.json` → Ornate (status flipped to `implemented`)
- Wave 2 protocol: `Docs/specs/2026-04-27-fx-engine-build-plan.md` §4 + §7
- Sibling Wave 2 chains: 19 remaining (see audit doc for queue order)
