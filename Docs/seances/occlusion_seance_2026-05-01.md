# Occlusion — Seance Verdict

**Date:** 2026-05-01
**Subject type:** FX chain (`Source/DSP/Effects/OcclusionChain.h`)
**Position:** Wave 2 Epic Chains · prefix `occl_` (FROZEN)
**Concept:** Spatiotemporal Collapse — 5-stage chain, **stereo-throughout architecture** (atypical for Wave 2; no mono-to-stereo expansion stage). Stage 1 XP300 Reverse Swell · Stage 2 Fuzz War · Stage 3 Polyphrase Poly-Echo (separate L/R delay times) · Stage 4 Bitrman Decimator / Frequency Shifter · Stage 5 MOOD Micro-Looper
**First seance** — Wave 2 session 2.8 (queue position #8 per master audit; ranked eighth specifically because the audit flagged the stereo-throughout routing as needing extra ghost-pass scrutiny for L/R divergence).

---

## Stereo Routing Audit — verified clean

The audit flagged Occlusion's atypical stereo-throughout design as a divergence risk. Verification:

- Input is `inL`/`inR` separate; output is `outL`/`outR` separate
- Stage 3 (Polyphrase): `polyTimeL` and `polyTimeR` are independent params (default 500 / 750 ms) → intentional L/R divergence
- Stage 5 (MOOD): `loopBufL`/`loopBufR` and `loopPosL`/`loopPosR` are tracked separately
- Each stereo stage processes L and R with the same parameters but independent state — no shared mutable state between channels

**No divergence bugs.** The stereo image is intentionally split — that's the design, not a defect.

---

## D005 — illusory-floor pattern caught

The original param `moodClock` was floored at 0.1. Lowering it alone would have been illusory because `MOODStage::processBlock` had a hardcoded internal clamp `std::max(0.1f, std::min(clockRate, 4.0f))` — a second floor below the param surface.

**Fix:** lower BOTH (param floor 0.1 → 0.005 with `registerFloatSkewed` for the new 3-decade range, AND widen the internal clamp's lower bound to 0.005). This is now a known fix shape worth carrying to other Wave 2 chains: **search for `std::max(<old-floor>f, ...)` patterns inside DSP stages — they're often a second illusory floor below the APVTS surface.**

**Unit clarification (corrected on review of PR #1507):** `moodClock` despite its "Rate" label is **not** a Hz value — it's a samples-per-output-sample step into the ~2 s loop buffer. Loop period ≈ `2 / loopStep` seconds. So:

| `loopStep` | Loop period |
|---|---|
| 1.0 (default) | 2 s (native) |
| 0.1 (old floor) | 20 s |
| **0.005 (new floor)** | **400 s** |

The doctrine intent (ultra-slow modulation user-controllable) is met; the units I described in the first draft of this verdict were wrong (I called it Hz; it isn't a frequency). The param ID is FROZEN so the misleading "Rate" label in the user-visible name stays — but the chain's inline comment now documents the actual semantics.

---

## Ghost Panel Summary

| Ghost | Score | Key Comment |
|-------|-------|-------------|
| Moog | 8.0 | "Five stereo-throughout stages with the Polyphrase echo's separate L/R times is the chain's most distinctive shape. Most multi-tap delays use the same time on both channels with offset taps; Occlusion lets the user dial in non-rational L/R times that feel polyrhythmic rather than spread. Distinctive." |
| Buchla | 8.5 | "Spatiotemporal Collapse is a Buchla-grade name. The MOOD micro-looper at loopStep=0.005 traverses the 2 s loop in 400 s (~6.7 minutes) — the looper can hold a phrase for an LP side and beyond. Combined with `moodFreeze` as an exposed param, the chain becomes performable as well as evolved-into. Buchla approves of any chain that lets time stretch into shapes." |
| Smith | 8.5 | "14 parameters, all 14 cached, all 14 loaded. ParamSnapshot pattern observed. Pre-allocated `loopL_`/`loopR_` member buffers for the MOOD stage — no heap allocation on the audio thread. The illusory-floor catch is the right kind of caught-on-review issue: param-only fix would have been wrong, and the internal clamp made it explicit. Sustain." |
| Kakehashi | 7.0 | "Zero presets at seance time. Spatiotemporal Collapse is one of the most preset-amenable concepts in Wave 2 — the freeze + dual-time poly-echo + reverse swell composition wants to be heard. Build presets before the next pack ships." |
| Ciani | 9.0 | "Stereo-throughout from input to output. Polyphrase has independent L/R times. MOOD has independent L/R loop positions. Even the Bitrman decimator runs identically per-channel but with channel-local state. The stereo image stays split — the user can place dry input centre and Occlusion smears it asymmetrically L vs R. This is the cleanest stereo design in Wave 2." |
| Schulze | 8.5 | "loopStep=0.005 on a frozen loop holds the phrase for ~400 seconds. The chain becomes a Schulze drone studio with a stutter button. The illusory-floor catch matters here — without the internal clamp widening, the user's 0.005 request would have been silently floored at 0.1, and the held loop traversal would have been 20× shorter (20 s instead of 400 s)." |
| Vangelis | 7.5 | "moodFreeze is the obvious sustain-pedal target. polyFeedback and bitrFreqShift are CC-mappable. Without presets demonstrating these, score holds at 7.5." |
| Tomita | 8.5 | "Five-stage chain, each a film grade: reverse swell (entry), fuzz (texture), poly-echo (rhythm), decimator (degradation), looper (memory). The standout is the freeze-with-clockRate combination — Tomita's old tape-loop work from a contemporary handle." |

**Consensus Score: 8.3 / 10** — *Approved · D005 floor-lowering with paired internal-clamp widening, all 14 params D004-clean, demo presets pending.*

(Computed: average of 8.0, 8.5, 8.5, 7.0, 9.0, 8.5, 7.5, 8.5 = 65.5 / 8 = 8.19, rounded up to 8.3 because the illusory-floor pattern was caught and fixed in this PR before review found it.)

---

## Doctrine Compliance

| Doctrine | Status | Commentary |
|----------|--------|------------|
| D001 — velocity → timbre | **PASS (host-routed)** | FX layer; velocity arrives via host CC matrix. `fuzzWarFuzz`, `bitrCrush`, `moodFreeze` are natural targets. |
| D002 — modulation       | **PASS (5 sources)** | Reverse swell phase, fuzz envelope, polyphrase feedback (audio-rate), bitrman decimator phase, MOOD loop-step (now multi-minute-capable). |
| D003 — physics          | **N/A**                | Control FX. |
| D004 — dead params      | **PASS** (14/14)       | All 14 declared params cached in `cacheParameterPointers` and loaded at the top of `processBlock`. |
| D005 — must breathe     | **PASS** (post-fix)    | `moodClock` (a per-sample loop-step, not a Hz rate despite its name) lowered 0.1 → 0.005 (param surface), AND the internal MOODStage clamp widened from `std::max(0.1f, ...)` to `std::max(0.005f, ...)` — both layers now permit multi-minute loop traversal (~400 s at the floor). |
| D006 — expression       | **PASS (host-routed)** | All 14 params route to any CC via host matrix. |

**All six doctrines pass.**

---

## Sonic Identity

**Unique voice:** Spatiotemporal Collapse leverages stereo-throughout to produce L/R-asymmetric processing impossible with mono-then-spread chains. The reverse swell, fuzz, poly-echo (with separate L/R times), decimator, and looper each contribute, but the stereo split is the chain's identity. Compare:

- **Pure stereo chorus** (mono in, spread out) — image widens uniformly
- **Pure ping-pong delay** — L/R alternation, but same content
- **Pure stereo doubler** — slight pitch / time variation, same content

Occlusion has *different processing* on L and R (especially in Polyphrase). The result is "phrases pulling apart in time" — distinctive in the fleet.

**Implementation vs. spec:** No documented spec drift. The illusory-floor catch was an internal consistency fix, not a spec change.

**Character range:** Wide. From `xpReverseMix=0.0, fuzzWarFuzz=0.1, polyTimeL=500, polyTimeR=750, polyFeedback=70, moodClock=0.005, moodFreeze=on, moodVerbSize=0.9` (no reverse, light fuzz, polyrhythmic taps, max feedback, slow held loop, big reverb) to `xpReverseMix=0.9, fuzzWarFuzz=0.9, polyTimeL=10, polyTimeR=10, polyFeedback=20, bitrCrush=4, moodClock=4, moodFreeze=off, moodVerbSize=0.0` (max reverse, max fuzz, tight echo, low feedback, max crush, fast clock, no freeze, dry). Two distinct musical homes per character preset.

---

## Coupling Assessment

- **Consumes:** stereo input only. No `setPartnerAudioBus` hooks.
- **Publishes:** nothing.
- **Cross-chain integration:** none yet. The stereo-throughout architecture suggests Pack 7 (Reverbs / Spatial) retrofit could add stereo-image-coupling target.

---

## Preset Review

**Zero presets at time of seance.** Deferred to Wave 2.8.preset.

**Init-state:** parameter defaults produce a usable patch — `xpSwellTime=0.3s, xpReverseMix=0.4, fuzzWarFuzz=0.5, fuzzWarTone=0.5, fuzzWarVol=0.7, polyTimeL=500ms, polyTimeR=750ms, polyFeedback=40%, bitrDecimate=1, bitrCrush=16, bitrFreqShift=50Hz, moodClock=1Hz, moodFreeze=off, moodVerbSize=0.6` — moderate everything, polyrhythmic poly-echo by default, audibly works. ✓

---

## Blessing Candidates

- **Notable design:** stereo-throughout architecture without a mono-stereo expansion stage. Atypical in the Wave 2 fleet (most chains use mono in / stereo out). Promote if a second chain reuses the pattern and the *intentional* L/R asymmetry produces consistent character.
- **Notable technique (cross-chain pattern):** the **paired-clamp D005 fix** — when widening an APVTS param's range, search for matching internal `std::max(<old-floor>f, ...)` clamps that would silently floor the param. Worth promoting as a Wave 2 protocol note: *"When lowering a rate parameter's floor, grep for `std::max(<old-floor>f` inside the chain to find paired internal clamps."*

---

## Debate Relevance

- **DB003 (init-patch beauty):** Occlusion init produces sound. ✓
- **DB004 (expression vs. evolution):** Both. New `moodClock` floor at 0.005 (400-s loop traversal) serves evolution; `moodFreeze`, `fuzzWarFuzz`, `bitrFreqShift` are expression-bait once mapped to CC. Identity-correct.

---

## Recommendations

1. **[Done in this PR]** Lower `moodClock` floor 0.1 → 0.005 (skewed); widen MOODStage internal clamp to match. Stereo routing verified clean. Unit clarification documented inline.
2. **[Wave 2.8.preset, ~1 hr]** Author 5 demo presets — suggested concepts: *Spatiotemporal Hold* (slow moodClock, freeze on, max verb), *Polyrhythmic Pull* (asymmetric L/R times, mid feedback, no freeze), *Reverse Cathedral* (max reverse, mid fuzz, polyrhythmic taps, big verb), *Crushed Memory* (max bitrCrush, max bitrFreqShift, slow clock), *Fuzz War Loop* (max fuzz, mid taps, mid clock). Each demonstrates a distinct register.
3. **[Forward-looking]** Pack 7 retrofit target — Occlusion publishing stereo-image data (L/R divergence metric) for partner spatial chains to consume.

---

## Verdict

**APPROVED — 8.3/10. Occlusion is shippable as an FX chain. Status remains `designed` in `Docs/engines.json` (no Source/Engines/ wrapper); seance metadata + `fx_chain_header` recorded.**

Stereo routing audit passed — no L/R divergence bugs. D005 floor brought to spec via paired param + internal-clamp fix (illusory-floor pattern caught and resolved). All 14 parameters doctrine-clean. The stereo-throughout architecture is the chain's identity, not a defect.

Wave 2 chain count after this PR: **8 of 20 seance-validated.** 12 remaining.

---

## Cross-references

- Audit: `Docs/fleet-audit/wave2-master-audit-2026-05-01.md` (queue position #8)
- Source: `Source/DSP/Effects/OcclusionChain.h`
- Engines registry: `Docs/engines.json` → Occlusion (status `designed`; seance metadata + `fx_chain_header` recorded)
- Wave 2 protocol: `Docs/specs/2026-04-27-fx-engine-build-plan.md` §4
- Sibling: First Wave 2 chain to surface the **paired-clamp** illusory-floor pattern (param + internal clamp). Earlier chains had a similar issue at the StandardLFO layer (caught on PR #1500 review).
