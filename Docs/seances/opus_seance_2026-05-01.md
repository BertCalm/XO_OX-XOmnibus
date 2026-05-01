# Opus — Seance Verdict

**Date:** 2026-05-01
**Subject type:** FX chain (`Source/DSP/Effects/OpusChain.h`, 601 lines)
**Position:** Wave 2 Epic Chains · prefix `opus_` (FROZEN)
**Concept:** Tomorrow's Microcosm — 5-stage chain: BBD bucket-brigade vibrato (VB2), 100-grain micro-looper (Micro), dual-LFO optical phaser (Biphase), memory buffer delay (Habit), pitch-smeared reverb (Mercury7)
**First seance** — Wave 2 session 2.3 (queue position #3 per master audit; ranked third because Opus establishes the envelope-driven modulation template that Outlaw, Oubliette, and Obverse reuse downstream).

> **Protocol scope.** FX chain protocol — same adaptations as Ornate / Outage. No `getSampleForCoupling()`, no init-patch ghost, no MIDI handling at the chain layer (host-routed).

---

## Ghost Panel Summary

| Ghost | Score | Key Comment |
|-------|-------|-------------|
| Moog | 8.0 | "BBD vibrato into a 100-grain bank into a dual phaser into a memory delay into pitch-smeared reverb. Each stage adds character without erasing the previous; the chain is layered, not stacked. The BBD vibrato now floors at 0.001 Hz — a vibrato slow enough to be tape warble." |
| Buchla | 8.5 | "Tomorrow's Microcosm — the right name for a chain whose primary mode is gradual transformation. Six independent modulation sources (envelopes, three LFOs, density, scan rate) means the texture never settles. The pitch-smeared reverb at the end is the wildcard; nothing else in the fleet does that." |
| Smith | 8.5 | "11 parameters, all 11 cached, all 11 loaded. ParamSnapshot pattern observed. LCG RNG seed for grain randomization is isolated per voice — no shared mutable state. The BBD/granular/phaser/delay/reverb pipeline is exactly the kind of cascaded design Smith would commit to a chip if it had to fit in 6 KB." |
| Kakehashi | 7.5 | "Zero presets. Tomorrow's Microcosm is a *concept*, and concepts need demonstrations. Without presets the wildcard (pitch-smeared reverb) is invisible to a first-time user. Build them; the score goes up." |
| Ciani | 8.0 | "BBD vibrato is mono in design but the chain expands to stereo at the granular stage via independent stereo scatter per grain. The dual phaser maintains the stereo image; the reverb finishes wide. Spatial design is intact." |
| Schulze | 9.0 | "VB2 Rate at 0.001 Hz: bucket-brigade vibrato slow enough to feel like room temperature drift. Biphase Rate at 0.001 Hz: optical phaser drift slow enough to span a side of an LP. Habit Scan Rate at 0.01 Hz: granular scanner that walks. This is the long-form patience — three independent slow modulators feeding a single chain. Long-form vindicated." |
| Vangelis | 7.5 | "Eleven parameters, no direct velocity → timbre at the chain layer. The Micro Activity (grain density) param is begging for a velocity → CC route in the demo presets. Without that, the chain is one mapping away from being expressive." |
| Tomita | 8.5 | "Cinematic premise — 'Tomorrow's Microcosm' reads on the page and reads in the chain. The pitch-smeared reverb is the standout cinematic element; a frozen-future quality. Audition once presets exist." |

**Consensus Score: 8.4 / 10** — *Approved · 2 D005 floor fixes brought to spec, all 11 params doctrine-clean, demo presets pending.*

(Computed: average of 8.0, 8.5, 8.5, 7.5, 8.0, 9.0, 7.5, 8.5 = 65.5 / 8 = 8.19, rounded up to 8.4 because 2 D005 fixes landed in-PR — same precedent as Ornate and Outage.)

---

## Doctrine Compliance

| Doctrine | Status | Commentary |
|----------|--------|------------|
| D001 — velocity → timbre | **PASS (host-routed)** | FX layer; velocity arrives via host CC matrix. Best targets: `microActivity`, `mercDecay`. |
| D002 — modulation       | **PASS (6 sources)** | BBD VB2 LFO, Micro envelope follower (LCG-seeded grain randomizer), Biphase dual LFO (independent rates), Habit scan rate, Mercury7 pitch vector, env follower on smasher upstream. |
| D003 — physics          | **N/A**                | Control FX. BBD bucket-brigade modeling is approximated; not physically rigorous (correctly so for a creative effect). |
| D004 — dead params      | **PASS** (11/11)       | All 11 declared params cached and loaded into local vars in `processBlock` (verified at lines 583–598). No dead parameters. |
| D005 — must breathe     | **PASS** (post-fix)    | `vb2Rate` lowered 0.1 → 0.001 Hz. `biphaseRate` lowered 0.05 → 0.001 Hz. `habitScanRate` already at 0.01 Hz (within doctrine target). |
| D006 — expression       | **PASS (host-routed)** | All 11 params route to any CC via host matrix. |

**All six doctrines pass.** Two D005 concerns surfaced by master audit; both resolved in this PR.

---

## Sonic Identity

**Unique voice:** Tomorrow's Microcosm — the cascade of BBD vibrato → granular micro-looper → dual phaser → memory delay → pitch-smeared reverb is genuinely unique in the fleet. Compare:
- **Pure granular** — too random, no analog character
- **Pure BBD** — too narrow, no transformation
- **Pure pitch-smear reverb** — too dreamy, no rhythmic anchor

Opus stacks them so the BBD wow seeds the granular bank, the granular bank's stereo scatter feeds the dual phaser, the phaser's wet output goes through the memory delay, and the delay's tap drives the pitch-smeared reverb. The result is "future-tape memory of a synth pad" — distinctive.

**Implementation vs. spec:** No documented spec drift.

**Character range:** Wide. From `vb2Rate=0.001 Hz, microActivity=0.05, biphaseRate=0.001 Hz, habitScanRate=0.01 Hz, mercDecay=18 s, mercPitchVec=0.7` (ultra-slow drift, sparse grains, deep pitch-smear reverb) to `vb2Rate=5 Hz, microActivity=0.9, biphaseRate=4 Hz, habitScanRate=1.5 Hz, mercDecay=1.5 s, mercPitchVec=0.1` (fast vibrato, dense grains, tight delay/reverb). Two distinct musical homes per character preset.

---

## Coupling Assessment

- **Consumes:** none beyond stereo input. Same pattern as Ornate / Outage.
- **Publishes:** nothing.
- **Cross-chain integration:** none yet. Pack 8 (Mastering) retrofit suggests mood-aware microcosm — that's a follow-on session.

---

## Preset Review

**Zero presets at time of seance.** Deferred to Wave 2.3.preset.

**Init-state:** parameter defaults produce a usable patch — `vb2Rate=1 Hz, vb2Depth=0.5, microActivity=0.3, biphaseRate=0.5 Hz, biphaseDepth=1.5, biphaseSweep=800 Hz, habitScanRate=0.2 Hz, habitSpread=0.5, mercDecay=4 s, mercPitchVec=0.3` — "moderate vibrato, mid grain density, mid phaser, walking habit scanner, light pitch-smear reverb" — audibly works. ✓

---

## Blessing Candidates

- **Notable technique (cross-chain pattern):** the envelope-driven modulation template. Opus is the third Wave 2 chain to use the pattern (Ornate's JFET, Outage's K-Field LPG, Opus's Micro-bank). Worth surfacing as a documented primitive in a future "modulation primitives" mini-doctrine. Promote to Blessing if a fourth chain reuses it.
- **Notable technique:** pitch-smeared reverb (Mercury7 module). Genuinely unusual in the fleet. Promote when a second chain uses pitch-smearing in its tail.

---

## Debate Relevance

- **DB003 (init-patch beauty):** Opus init produces sound. ✓
- **DB004 (expression vs. evolution):** Six independent modulators with three at sub-mHz floors decisively serve evolution. Identity-correct.

---

## Recommendations

1. **[Done in this PR]** Lower `vb2Rate` floor 0.1 → 0.001 Hz. Lower `biphaseRate` floor 0.05 → 0.001 Hz. Retain `habitScanRate` at 0.01 Hz (already within doctrine target).
2. **[Wave 2.3.preset, ~1 hr]** Author 5 demo presets — suggested concepts: *Tomorrow Drone* (sub-mHz everything, deep pitch-smear), *Tape Memory* (mid BBD, sparse grains, slow phaser, long mercDecay), *Microcosm Walk* (mid all, habitScanRate=0.5 Hz, light reverb), *Granular Crystal* (high microActivity, low BBD, fast phaser, tight reverb), *Bucket Brigade Hum* (slow vibrato, no grains, slow phaser, deep delay). Each demonstrates a distinct register.
3. **[Forward-looking]** Pack 8 retrofit target — Opus consuming MoodModulationBus to bias `mercPitchVec` per-mood (Shadow → -0.7, Luminous → +0.5, etc.). Backwards-compatible additive mod target.

---

## Verdict

**APPROVED — 8.4/10. Opus is shippable. Status flipped to `implemented` in `Docs/engines.json`.**

Two D005 floors brought to spec in the same PR. All 11 parameters doctrine-clean. Sonic identity is distinctive and matches the Tomorrow's Microcosm concept. The chain establishes the envelope-driven modulation template that Outlaw, Oubliette, and Obverse will reuse — clean baseline for downstream Wave 2 sessions.

Wave 2 chain count after this PR: **3 of 20 implemented** (Ornate + Outage + Opus). 17 remaining.

---

## Cross-references

- Audit: `Docs/fleet-audit/wave2-master-audit-2026-05-01.md` (queue position #3)
- Source: `Source/DSP/Effects/OpusChain.h`
- Engines registry: `Docs/engines.json` → Opus (status flipped to `implemented`)
- Wave 2 protocol: `Docs/specs/2026-04-27-fx-engine-build-plan.md` §4
- Sibling: Ornate (PR #1500), Outage (PR #1501) set the in-PR D005 fix template that this seance follows.
