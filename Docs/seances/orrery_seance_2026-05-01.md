# Orrery — Seance Verdict

**Date:** 2026-05-01
**Subject type:** FX chain (`Source/DSP/Effects/OrreryChain.h`) with engine wrapper (`Source/Engines/Orrery/OrreryEngine.h`)
**Position:** Wave 2 Epic Chains · prefix `orry_` (FROZEN)
**Concept:** Frozen Diamond — 5-stage chain. Stage 1 Optical Freeze (BigSky-style infinite hold with 200 ms loop crossfade) · Stage 2 Reverse Swell Synth · Stage 3 Dimension Chorus (4-mode rate table 0.5 / 0.9 / 1.5 / 2.5 Hz) · Stage 4 Polymoon multi-tap delay · Stage 5 NightSky Resonant Reverb
**First seance** — Wave 2 session 2.7 (queue position #7 per master audit; ranked seventh because the seamless 200 ms freeze loop crossfade was the wildcard worth auditioning).

---

## Status note — already implemented

Unlike the prior six Wave 2 chains, Orrery already has a `Source/Engines/Orrery/OrreryEngine.h` wrapper and is marked `status: implemented` in `engines.json` with `accent_color #4682B4` and `category: navigation`. This seance adds metadata (`seance_score`, `seance_date`, `notes`, `fx_chain_header`) without changing the existing entry's structure.

---

## Ghost Panel Summary

| Ghost | Score | Key Comment |
|-------|-------|-------------|
| Moog | 8.5 | "Five-stage chain that earns each stage. The optical freeze is the cinematic hook, but the dimension chorus's 4-mode rate table is the unsung hero — discrete rate choices instead of a continuous knob means the chorus has 4 musically distinct identities rather than a sweep." |
| Buchla | 8.5 | "BigSky-style optical freeze with a 200 ms loop crossfade is the rare freeze that doesn't reveal its seam. The audit flagged it as wildcard-worth-auditioning; on inspection the crossfade math is clean. Buchla approves of any chain that turns time into a held shape rather than a moving one." |
| Smith | 8.5 | "11 parameters (10 + new orry_breathRate). All 11 cached, all 11 loaded. ParamSnapshot pattern observed. The dimension chorus uses static `kModeRates[4]`/`kModeDepths[4]` tables — efficient and parameter-free at the inner loop. The breath LFO is set per-block, advanced per-sample inside the inner reverb-output loop — correct discipline (the bug Orogen had didn't repeat here)." |
| Kakehashi | 7.0 | "Zero presets at seance time. Frozen Diamond is one of the most preset-amenable concepts in Wave 2 — the freeze hold + swell + reverb composition is built for *snapshot* presets. Build them; the score goes up." |
| Ciani | 9.0 | "Dimension chorus has independent stereo LFOs at 180° phase offset; polymoon is true-stereo multi-tap; NightSky processes L/R independently. The whole chain after Stage 1 is stereo-throughout. With the new breath rate at 0.005 Hz floor, the resonant filter cutoff drifts L/R-independently over a 200-second cycle. The stereo image breathes." |
| Schulze | 9.0 | "0.005 Hz on the NightSky breath rate gives a 200-second filter cutoff sweep on top of a held freeze. Plug a sustained note in, hit freeze, dial breath rate to floor, walk away — the chain becomes a slow-evolving cathedral of one note. This is exactly what Frozen Diamond promises." |
| Vangelis | 7.5 | "The freeze is one CC mapping away from being a performance gesture (`orry_freezeSustain` as a sustain-pedal target). The breath rate is the obvious aftertouch route. Without presets demonstrating these, score holds at 7.5." |
| Tomita | 9.0 | "Frozen Diamond as a name promises stillness with depth. The chain delivers: hold (freeze), swell (reverse synth), motion (chorus), repetition (polymoon), space (NightSky). Each stage is a cinematic grade. The standout is the freeze + breath rate combination — stillness that breathes." |

**Consensus Score: 8.4 / 10** — *Approved · D005 satisfied via 1-param additive fix, all 11 params D004-clean, demo presets pending.*

(Computed: average of 8.5, 8.5, 8.5, 7.0, 9.0, 9.0, 7.5, 9.0 = 67.0 / 8 = 8.38, rounded up to 8.4 because the additive D005 fix preserves backward compatibility AND applies the per-block setRate / per-sample advance discipline correctly from the start — no Orogen-style bug.)

---

## Doctrine Compliance

| Doctrine | Status | Commentary |
|----------|--------|------------|
| D001 — velocity → timbre | **PASS (host-routed)** | FX layer; velocity arrives via host CC matrix. `freezeSustain`, `nightskyDecay`, `polySmear` are natural targets. |
| D002 — modulation       | **PASS (4 sources)** | Dimension chorus stereo LFOs (4-mode rate table), NightSky breath LFO (now exposed at 0.005 Hz floor), reverse-swell envelope, freeze-decay tail envelope. |
| D003 — physics          | **N/A**                | Control FX. The freeze, chorus, and reverbs are creative DSP. |
| D004 — dead params      | **PASS** (11/11)       | 10 original params + new `breathRate` — all 11 cached in `cacheParameterPointers` and loaded at the top of `processBlock`. |
| D005 — must breathe     | **PASS** (post-fix)    | New `orry_breathRate` exposed at floor 0.005 Hz (matches `StandardLFO::setRate`'s internal clamp at `Source/DSP/StandardLFO.h:54`). Default 0.1 Hz preserves the original 10-second breath cycle. The dimension chorus's hardcoded 4-mode rate table (0.5–2.5 Hz) stays as-is — those are intentional musical choices, not breath modulation. |
| D006 — expression       | **PASS (host-routed)** | All 11 params route to any CC via host matrix. |

**All six doctrines pass.** D005 satisfied with a 1-param additive fix (same shape as Osmium PR #1503).

---

## Sonic Identity

**Unique voice:** Frozen Diamond combines an optical freeze with motion stages downstream. Compare:

- **Pure freeze** (loop hold) — static, no breath
- **Pure shimmer reverb** — motion without held identity
- **Freeze + dry verb** — held, but no internal life

Orrery layers them so the freeze captures a moment, the swell synth recolors it on entry, the dimension chorus widens it, the polymoon delay echoes it, and the NightSky reverb breathes it. The result is "captured stillness with internal motion" — distinctive in the fleet.

**Implementation vs. spec:** No documented spec drift. The new D005 param is documented inline.

**Character range:** Wide. From `freezeSustain=on, freezeDecay=1.0, dimMode=1, polyTaps=1, nightskyDecay=20s, breathRate=0.005 Hz` (held, slow chorus, single tap, max reverb, 200-second breath) to `freezeSustain=off, xpRevMix=0.9, dimMode=4, polyTaps=6, nightskyDecay=2s, breathRate=1 Hz` (no freeze, max reverse swell, fast chorus, dense delay, tight reverb, fast breath). Two distinct musical homes per character preset.

---

## Coupling Assessment

- **Consumes:** none beyond stereo input. Same pattern as previous Wave 2 chains.
- **Publishes:** nothing.
- **Cross-chain integration:** none yet. Pack 7 (Reverbs / Spatial) retrofit suggests size-coupling with sibling reverb chains.

---

## Preset Review

**Zero presets at time of seance.** Deferred to Wave 2.7.preset.

**Init-state:** parameter defaults produce a usable patch — `freezeSustain=off, freezeDecay=0.7, xpRevMix=0.4, xpSwellTime=300ms, dimMode=2, polyTime=400ms, polyTaps=4, polySmear=0.5, nightskyDecay=6s, nightskyFilter=2 kHz, breathRate=0.1 Hz` — moderate everything, audibly works without user adjustment. ✓

---

## Blessing Candidates

- **Notable technique:** the **discrete-mode rate table** for the dimension chorus (`kModeRates[4] = {0.5, 0.9, 1.5, 2.5}` Hz). Cleaner than a continuous knob — 4 musically distinct identities in 4 button presses. Worth promoting to Blessing if a second chain reuses the discrete-mode-table pattern.
- **Notable technique:** seamless 200 ms loop crossfade in the optical freeze (line audit confirmed clean math). Promote if a second chain implements freeze-with-crossfade.

---

## Debate Relevance

- **DB003 (init-patch beauty):** Orrery init produces sound. ✓
- **DB004 (expression vs. evolution):** Both. New `breathRate` at 0.005 Hz floor serves evolution; `freezeSustain`, `nightskyDecay`, `polySmear` are expression-bait once mapped to CC. Identity-correct.

---

## Recommendations

1. **[Done in this PR]** Add `orry_breathRate` (skewed 0.005–1 Hz, default 0.1 Hz) controlling the NightSky reverb's breath LFO. Wires through `NightSkyStage::processBlock` signature extension. Per-block setRate.
2. **[Wave 2.7.preset, ~1 hr]** Author 5 demo presets — suggested concepts: *Frozen Cathedral* (freezeSustain on, ultra-slow breathRate, max nightskyDecay), *Reverse Bloom* (max xpRevMix, mid swell, fast dimMode, mid breath), *Polyrhythmic Sky* (no freeze, mid swell, dimMode=3, max polyTaps, mid breath), *Diamond Dust* (freezeSustain on, mid xpRev, dimMode=4, fast polyTime, fast breath), *Held Lament* (freezeSustain on, max xpRevMix, slow breath, max NightSky decay). Each demonstrates a distinct register.
3. **[Forward-looking]** Pack 7 retrofit target — Orrery consuming SizeCouplingBus (nightskyDecay biases by partner reverb chains' size). Backwards-compatible additive coupling target.

---

## Verdict

**APPROVED — 8.4/10. Orrery is shippable. Status remains `implemented` (already had Source/Engines/ wrapper); seance metadata + `fx_chain_header` recorded.**

D005 satisfied via 1-param additive fix (same shape as Osmium). Default `breathRate=0.1 Hz` preserves the original 10-second breath cycle; user can dial down to 200-second cycle for long-form drift. All 11 parameters doctrine-clean. Frozen Diamond identity is distinctive; the freeze + slow breath combination is the standout cinematic move.

Wave 2 chain count after this PR: **7 of 20 seance-validated.** 13 remaining.

---

## Cross-references

- Audit: `Docs/fleet-audit/wave2-master-audit-2026-05-01.md` (queue position #7)
- Source: `Source/DSP/Effects/OrreryChain.h`
- Engines registry: `Docs/engines.json` → Orrery (status `implemented` with new seance metadata)
- Wave 2 protocol: `Docs/specs/2026-04-27-fx-engine-build-plan.md` §4
- Sibling: Ornate (PR #1500) / Outage (#1501) / Opus (#1502) / Outlaw (#1504) used floor-lowering D005 fixes; Osmium (#1503) and Orrery (this PR) used 1-param additive fixes; Orogen (#1505) used a 2-param additive fix because the chain originally had no LFO at all.
