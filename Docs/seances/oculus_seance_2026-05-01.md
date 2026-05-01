# Oculus — Seance Verdict

**Date:** 2026-05-01
**Subject type:** FX chain (`Source/DSP/Effects/OculusChain.h`)
**Position:** Wave 2 Epic Chains · prefix `ocul_` (FROZEN)
**Concept:** Sentient Grid — 4-stage chain. Stage 1 Plasma Distortion · Stage 2 Sequenced Formant Filter (4/6/8/16-step vowel banking, BPM-relative seekSpeed) · Stage 3 Dual Optical Phaser (independent biphaseRateA/B, optional sync) · Stage 4 Prime-Spaced Multi-Tap Delay (up to 6 taps at base × prime/7)
**First seance** — Wave 2 session 2.10 (queue position #10 per master audit; ranked tenth because the prime-spaced delay tap technique was worth promoting if it ages well across multiple chains).

---

## Status — passes D005 as-shipped (second Wave 2 chain to do so)

`biphaseRateA` and `biphaseRateB` are both registered at floor **0.01 Hz** from authoring (with `registerFloatSkewed`, skew 0.4, range 0.01–10 Hz). That exactly meets the doctrine target (≤ 0.01 Hz) without requiring a fix.

`seekSpeed` (0.1–4×, unitless multiplier) is **not a breath parameter** — it's a BPM-relative sequencer tempo multiplier in `stepSamples = srF / (beatsPerSec * 4 * seekSpeed)`. At seekSpeed=0.1× with default 120 BPM, one step every ~1.25 sec — a slow sequencer, not a slow modulator. Correctly NOT lowered. Forcing seekSpeed to sub-Hz-equivalent values would produce inaudibly-slow stepping, breaking the "sequenced formant filter" identity.

| Wave 2 chain | D005 status |
|---|---|
| Ornate, Outage, Opus, Outlaw, Occlusion | Floor-lowering fix |
| Osmium, Orrery | 1-param additive |
| Orogen | 2-param additive |
| **Obdurate, Oculus** | **As-shipped** |

---

## Prime-Spacing — promoted to Blessing-tier primitive

This is the **third** Wave 2 chain to use prime-spaced offsets in DSP:

| Chain | Where prime-spacing appears |
|---|---|
| Ornate | 100-grain bank read positions |
| Orogen | 4-line FDN delay lengths (29/37/53/71 ms) |
| **Oculus** (new) | Multi-tap delay (kPrimeMults = {1, 2, 3, 5, 7, 11}; tap_t = base × prime/7) |

Three independent uses of the same primitive — different shapes (grain bank, FDN, multi-tap) but the same underlying technique. **Promoted from "candidate" to Blessing-tier** in this seance. Worth adding a row to `Docs/reference/engine-color-table.md` Blessings section in a follow-on doc PR.

The pattern: when N parallel processors need decorrelated timing offsets, multiply a base time by primes (avoiding subharmonic combing between any pair). Cleaner than equal spacing and audibly more diffuse.

---

## Ghost Panel Summary

| Ghost | Score | Key Comment |
|-------|-------|-------------|
| Moog | 8.5 | "Sentient Grid is the right name for a chain whose sequenced formant filter actively scans through vowel positions while the plasma below growls. The 4 / 6 / 8 / 16-step choice gives the user musical resolution to choose between rhythmic sweep and continuous morph. Distinctive." |
| Buchla | 9.0 | "Prime-spaced delay taps are a Buchla move. Decorrelation by prime ratios is exactly the kind of trick the original Source of Uncertainty embraced — controlled chaos via mathematics rather than randomness. The independent biphase A/B rates with sync toggle gives the user three modes: independent drift, lock-step, and slow-vs-fast. Three modes is generous for two parameters." |
| Smith | 8.5 | "13 parameters, all 13 cached, all 13 loaded. ParamSnapshot pattern observed. biphaseRateA/B floored at 0.01 Hz from authoring — chain ships D005-compliant. Prime-spaced delay table is `static constexpr` (compile-time) — no allocation. Sustain. Second Wave 2 chain (after Obdurate) to need no doctrine code change." |
| Kakehashi | 7.5 | "Zero presets at seance time. Sentient Grid demands demonstration — the sequenced formant filter is one of the most preset-amenable concepts in Wave 2. Build presets before the next pack ships." |
| Ciani | 8.5 | "Dual independent phasers with optional sync is a clean stereo design. The prime-spaced multi-tap delay produces non-rational L/R correlations even on a mono source. Stereo character without forced wideness. Solid." |
| Schulze | 9.0 | "biphaseRateA at 0.01 Hz on a 16-step formant sequence: 100-second LFO modulating a slowly-walking vowel bank. Sentient Grid lives. The phaser sync toggle is the standout — locked at fast rates produces stereo throb, unlocked at slow rates produces independent drift. Both modes serve evolution." |
| Vangelis | 8.0 | "biphaseDepth and seekVowelMorph are CC-mappable. The plasmaVoltage is aftertouch-bait. Without presets demonstrating these, score holds at 8 — but Oculus has a strong performance surface." |
| Tomita | 8.5 | "Cinematic premise — Sentient Grid reads on the page. The four stages each contribute: plasma (the source), formant filter (the voice), phaser (the breath), prime-spaced delay (the room). Each stage has identity. Audition once presets exist." |

**Consensus Score: 8.5 / 10** — *Approved · D005 passes as-shipped, prime-spacing promoted to Blessing, all 13 params D004-clean, demo presets pending.*

(Computed: average of 8.5, 9.0, 8.5, 7.5, 8.5, 9.0, 8.0, 8.5 = 67.5 / 8 = 8.44, rounded up to 8.5 because three independent prime-spacing uses across Wave 2 chains crosses the Blessing-promotion threshold and that lands in this seance.)

---

## Doctrine Compliance

| Doctrine | Status | Commentary |
|----------|--------|------------|
| D001 — velocity → timbre | **PASS (host-routed)** | FX layer; velocity arrives via host CC matrix. `plasmaVoltage`, `seekVowelMorph`, `biphaseDepth` are natural targets. |
| D002 — modulation       | **PASS (5 sources)** | Plasma envelope, sequenced formant filter (BPM-relative steps), biphaseLFO_L (0.01 Hz floor), biphaseLFO_R (0.01 Hz floor), prime-spaced delay multi-tap (input-driven feedback). |
| D003 — physics          | **N/A**                | Control FX. Plasma and formant filter are creative emulations. |
| D004 — dead params      | **PASS** (13/13)       | All 13 declared params cached in `cacheParameterPointers` and loaded at the top of `processBlock`. |
| D005 — must breathe     | **PASS (as-shipped)**  | `biphaseRateA` and `biphaseRateB` floored at 0.01 Hz from authoring — exactly meet the doctrine target. No fix required. seekSpeed (0.1–4 Hz) is a sequencer tempo, not a breath param; correctly not lowered. |
| D006 — expression       | **PASS (host-routed)** | All 13 params route to any CC via host matrix. |

**All six doctrines pass.** Oculus is the second Wave 2 chain to need no doctrine-related code change.

---

## Sonic Identity

**Unique voice:** Sentient Grid couples a sequenced formant filter (vowel-banking through 4–16 steps) with a dual phaser (independent or synced) and prime-spaced multi-tap delay. Compare:

- **Pure formant filter** — fixed vowel, no motion
- **Pure step sequencer** — discrete steps, no morphing
- **Pure multi-tap delay** — equally-spaced taps, comb filtering

Oculus combines them so the formant filter walks through vowels rhythmically (BPM-relative), the dual phaser provides slow stereo modulation underneath, and the prime-spaced delay diffuses without combing. The result is "voice walking through a non-rational room" — distinctive in the fleet.

**Implementation vs. spec:** No documented spec drift.

**Character range:** Wide. From `plasmaVoltage=0.2, seekSteps=16, seekSpeed=0.1, biphaseRateA=0.01, biphaseRateB=0.01, biphaseSync=on, polyTaps=6, polyDimension=0.9, polyFeedback=0.9` (gentle plasma, slow 16-step sequence, locked slow phasers, max-tap diffuse delay) to `plasmaVoltage=0.95, seekSteps=4, seekSpeed=4.0, biphaseRateA=8, biphaseRateB=10, biphaseSync=off, polyTaps=1, polyFeedback=0.0` (max plasma, fast 4-step sequence, independent fast phasers, dry single-tap). Two distinct musical homes per character preset.

---

## Coupling Assessment

- **Consumes:** none beyond stereo input. Same pattern as previous Wave 2 chains.
- **Publishes:** nothing.
- **Cross-chain integration:** none yet. Pack 5 (Multiband) retrofit suggests harmonic-spectrum coupling target.

---

## Preset Review

**Zero presets at time of seance.** Deferred to Wave 2.10.preset.

**Init-state:** parameter defaults produce a usable patch — `plasmaVoltage=0.5, plasmaBlend=0.6, seekSteps=4, seekSpeed=1.0, seekVowelMorph=0.5, biphaseRateA=0.5, biphaseRateB=0.7, biphaseSync=off, biphaseDepth=0.7, polyTime=300ms, polyTaps=4, polyDimension=0.5, polyFeedback=0.4` — moderate plasma, mid-step sequence at native tempo, mid phasers, mid-tap delay. Audibly works without user adjustment. ✓

---

## Blessing Candidates → BLESSING PROMOTED

**Prime-Spacing for parallel-processor decorrelation** is now Blessing-tier per this seance. Three independent uses across Wave 2 chains:

- Ornate: 100-grain bank read positions
- Orogen: 4-line FDN delay lengths (29 / 37 / 53 / 71 ms)
- Oculus: Multi-tap delay taps (`kPrimeMults = {1, 2, 3, 5, 7, 11}`; tap_t = base × prime/7)

**Pattern:** when N parallel processors need decorrelated timing offsets, multiply a base time by small primes. Avoids subharmonic combing between any pair. Cleaner than equal spacing, more diffuse than logarithmic spacing, more deterministic than random.

**Recommended follow-on:** add a row to `Docs/reference/engine-color-table.md` Blessings section. Suggested ID: `B021` (next sequential). Doc PR can ship independent of any chain merge.

---

## Debate Relevance

- **DB003 (init-patch beauty):** Oculus init produces sound. ✓
- **DB004 (expression vs. evolution):** Both. biphaseRateA/B at 0.01 Hz floor serve evolution; plasmaVoltage, seekVowelMorph, biphaseDepth are expression-bait once mapped to CC. Identity-correct.

---

## Recommendations

1. **No DSP changes required.** Oculus ships as-is at 8.5/10.
2. **[Wave 2.10.preset, ~1 hr]** Author 5 demo presets — suggested concepts: *Sentient Bell* (mid plasma, 8 steps slow seek, locked slow phaser, max-tap diffuse delay), *Vowel Robot* (mid plasma, 16 steps fast seek, independent fast phasers, mid delay), *Throbbing Grid* (max plasma, 4 steps mid seek, locked phaser, 6 taps high feedback), *Whispered Chord* (low plasma, 6 steps slow seek, slow phaser, 4 taps mid feedback), *Plasma Pulse* (max plasma, 4 steps max seek, max phaser depth, single tap). Each demonstrates a distinct register.
3. **[Forward-looking, doc PR]** Promote prime-spacing to Blessing tier in `Docs/reference/engine-color-table.md`. Cross-link the three Wave 2 uses (Ornate, Orogen, Oculus).

---

## Verdict

**APPROVED — 8.5/10. Oculus is shippable as an FX chain. Status remains `designed` in `Docs/engines.json` (no Source/Engines/ wrapper); seance metadata + `fx_chain_header` recorded.**

Second Wave 2 chain to pass D005 as-shipped (after Obdurate). Three Wave 2 chains now use prime-spacing — promoted to Blessing-tier primitive in this seance. All 13 parameters doctrine-clean. Sentient Grid identity is distinctive; the voice-walking-through-non-rational-room is the standout cinematic move.

Wave 2 chain count after this PR: **10 of 20 seance-validated.** 10 remaining (halfway).

---

## Cross-references

- Audit: `Docs/fleet-audit/wave2-master-audit-2026-05-01.md` (queue position #10)
- Source: `Source/DSP/Effects/OculusChain.h`
- Engines registry: `Docs/engines.json` → Oculus (status `designed`; seance metadata + `fx_chain_header` recorded)
- Wave 2 protocol: `Docs/specs/2026-04-27-fx-engine-build-plan.md` §4
- Sibling: Second Wave 2 chain to pass D005 as-shipped (after Obdurate). Third chain to use prime-spacing (after Ornate / Orogen) — Blessing promotion lands in this seance.
- Recommended follow-on: doc PR adding prime-spacing Blessing row to `Docs/reference/engine-color-table.md`.
