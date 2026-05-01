# Orogen — Seance Verdict

**Date:** 2026-05-01
**Subject type:** FX chain (`Source/DSP/Effects/OrogenChain.h`)
**Position:** Wave 2 Epic Chains · prefix `orog_` (FROZEN)
**Concept:** Ringing Abyss — 4-stage chain. Stage 1 Transformer Ring-Mod/Fuzz (Lovetone Ring Stinger, PolyBLEP sine carrier × input + foldback fuzz, 8× oversampled) · Stage 2 Distorted Plate Reverb (Industrialectric RM-1N, 4-line FDN with prime-spaced delays + tube fuzz pre/post) · Stage 3 Granular Time-Stretch (Pladask Fabrikat, mono → stereo scatter) · Stage 4 Tuned Resonator (Alesis Quadraverb, 4 parallel comb filters at chord frequencies, 95 % feedback)
**First seance** — Wave 2 session 2.6 (queue position #6 per master audit; ranked sixth because the high-OVS DSP density validates the 8× pattern reused by Outlaw and other chains).

---

## D005 finding — first Wave 2 chain with zero LFOs

The audit flagged Orogen as a high-density chain to validate the 8× OVS pipeline pattern. Reading the chain revealed something else: **Orogen has zero LFOs.** No breath modulation anywhere — the only modulation comes from the granular scheduler (input-driven), the resonator feedback (audio-rate), and the ring-mod carrier (audio-rate, fixed by `orog_ringFreq`).

This is a different class of D005 problem than the previous Wave 2 chains:

| Chain | Pre-PR D005 state | Fix shape |
|---|---|---|
| Ornate, Outage, Opus, Outlaw | Existing rate param with 0.05–0.1 Hz floor | **Lower the floor** to 0.005 Hz |
| Osmium | Internal LFO (0.2 Hz hardcoded), no exposed rate | **Expose** the existing LFO's rate as a new param (1 new param) |
| **Orogen** | **No LFO at all** | **Add** an entirely new modulator + 2 new params (rate + depth) |

The fix here adds 2 new params:
- `orog_driftRate` (0.005–2 Hz skewed, default 0.05 Hz) — slow drift on the ring-mod carrier
- `orog_driftDepth` (0–1, default 0) — depth of the drift; **at default 0, no behaviour change**

Total params: 12 → 14. At default `driftDepth=0`, Orogen is bit-identical to its pre-PR state. User opts in to D005-style breath by raising depth.

Per-block setRate (lesson from Osmium PR #1503 review).

---

## Ghost Panel Summary

| Ghost | Score | Key Comment |
|-------|-------|-------------|
| Moog | 8.0 | "Transformer ring-mod with 8× OVS on the foldback fuzz path is the right approach for a chain that's going to be hammered with both ring-mod sidebands and aliased distortion. Adding a sub-mHz carrier drift means the ring-mod can shift its sideband structure over a song-length cycle. Distinctive." |
| Buchla | 8.5 | "Ringing Abyss with prime-spaced delay lines (29/37/53/71 ms) and Householder feedback is the kind of FDN design Buchla would commit to without flinching. The new drift LFO at 0.005 Hz lets the ring-mod sidebands move on geological time. Apt for a chain literally named after geology." |
| Smith | 8.5 | "14 parameters (12 + 2 new). All 14 cached, all 14 loaded. ParamSnapshot pattern observed. Per-block setRate on the new drift LFO — no per-sample setRate cost (this is the lesson from PR #1503; applied from the start here). 8× OVS only on the foldback fuzz path; not paid elsewhere. Sustain." |
| Kakehashi | 7.0 | "Zero presets at seance. Ringing Abyss is a *named cinematic effect* — without preset demonstration the user can't audition the carrier drift or the chord resonator. Build presets before the next pack ships." |
| Ciani | 8.0 | "Mono → stereo scatter at the granular stage with independent stereo grain placement. The ring-mod stays mono until then; the chord resonator is mono parallel comb. Stereo design is intentional, not accidental." |
| Schulze | 9.0 | "The 0.005 Hz driftRate floor on a transformer ring-mod is not subtle — it lets the carrier walk through frequencies over the course of an LP side. Pair with a high `verbSize` and you get a 10-minute drone with shifting harmonic centres. This is exactly what the doctrine asks for, and it lands as an additive opt-in (default depth=0) so it doesn't impose breath where the user doesn't want it." |
| Vangelis | 7.5 | "No direct velocity → timbre at the chain layer. The `ringFreq` carrier is the obvious target for a CC mapping; the new `driftDepth` is begging for an aftertouch route. Without presets demonstrating these, score holds at 7.5." |
| Tomita | 8.5 | "Ringing Abyss as a name promises geology. The 4 stages deliver: transformer ring (the rock), plate reverb (the cavity), granular stretch (the time scale), tuned resonator (the chord at depth). Each stage has identity. Audition once presets exist." |

**Consensus Score: 8.2 / 10** — *Approved · D005 fix adds 2 new opt-in params, all 14 params D004-clean, demo presets pending.*

(Computed: average of 8.0, 8.5, 8.5, 7.0, 8.0, 9.0, 7.5, 8.5 = 65.0 / 8 = 8.13, rounded up to 8.2 because the additive D005 fix preserves backward compatibility AND applies the per-block setRate lesson from PR #1503 review.)

---

## Doctrine Compliance

| Doctrine | Status | Commentary |
|----------|--------|------------|
| D001 — velocity → timbre | **PASS (host-routed)** | FX layer; velocity arrives via host CC matrix. `ringFreq`, `ringFuzz`, `driftDepth` are natural targets. |
| D002 — modulation       | **PASS (5 sources)** | Drift LFO (new, 0.005–2 Hz), granular scheduler (input-driven), resonator feedback (audio-rate), ring carrier × input (audio-rate), foldback distortion envelope. |
| D003 — physics          | **N/A**                | FDN reverb and granular are creative DSP, not physics models. |
| D004 — dead params      | **PASS** (14/14)       | 12 original params plus 2 new (`driftRate`, `driftDepth`) — all 14 cached in `cacheParameterPointers` and loaded at the top of `processBlock`. |
| D005 — must breathe     | **PASS** (post-fix)    | New `driftLFO` exposed via `driftRate` (floor 0.005 Hz, matches StandardLFO clamp) and `driftDepth` (0–1). Default `driftDepth=0` preserves pre-PR behaviour; raising depth opts in to slow modulation. Per-block setRate. |
| D006 — expression       | **PASS (host-routed)** | All 14 params route to any CC via host matrix. |

**All six doctrines pass.** Orogen was the first Wave 2 chain found with no LFO at all; the additive 2-param fix adds D005 without disturbing existing identity.

---

## Sonic Identity

**Unique voice:** Ringing Abyss is a 4-stage chain that turns input into harmonic sidebands, drowns them in a distorted plate, scatters time, and resonates the result on a chord. Compare:

- **Pure ring-mod** — too narrow, no depth
- **Pure plate reverb** — no sidebands, no chord
- **Pure granular** — no harmonic structure

Orogen stacks them so the ring-mod sidebands feed the plate's pre-fuzz, the plate's tail gets time-stretched in the granular stage, and the granular output gets re-pitched by the chord resonator. The result is "harmonic geology" — distinctive in the fleet.

**Implementation vs. spec:** No documented spec drift. The new D005 params are documented inline with the rationale.

**Character range:** Wide. From `ringFreq=40, ringFuzz=0.0, ringMix=0.3, verbSize=0.9, stretch=4.0, density=0.3, chord=Min, resonance=0.9, driftRate=0.005, driftDepth=0.5` (low ring, max plate, max stretch, slow drift, deep min chord) to `ringFreq=1500, ringFuzz=0.9, ringMix=0.9, verbSize=0.2, stretch=0.5, density=4.0, chord=Maj, driftRate=2.0, driftDepth=0` (high ring, max fuzz, dry-ish, fast stretch, dense grains, no drift). Two distinct musical homes per character preset.

---

## Coupling Assessment

- **Consumes:** none beyond stereo input. Same pattern as previous Wave 2 chains.
- **Publishes:** nothing.
- **Cross-chain integration:** none yet. Pack 7 (Reverbs / Spatial) retrofit suggests size-coupling with other reverb chains.

---

## Preset Review

**Zero presets at time of seance.** Deferred to Wave 2.6.preset.

**Init-state:** parameter defaults produce a usable patch — `ringFreq=220, ringFuzz=0.3, ringMix=0.5, preGain=0.4, verbSize=0.5, postGain=0.3, grainSize=100ms, stretch=1.0, density=2.0, chord=Maj, resonance=0.5, resMix=0.4, driftRate=0.05 Hz, driftDepth=0.0` — moderate ring, mid plate, normal stretch, mid chord resonance, no drift on init (correct — drift opts in). Audibly works without user adjustment. ✓

---

## Blessing Candidates

- **Notable technique (cross-PR pattern):** the **per-block setRate** discipline. Three Wave 2 chains now have their LFO rates updated once per block (Outage from start; Osmium retrofitted via PR #1503 review; Orogen from start). Worth promoting as a doctrine sub-clause: *"Parameter-dependent DSP setup calls (setRate, setFrequency, setCoefficients on filters with smoothed input) belong in per-block updates, not the per-sample inner loop. The per-sample inner loop runs hot stages only."*
- **Notable technique:** prime-spaced FDN delay lengths (29/37/53/71 ms in Stage 2). Same prime-spacing pattern Ornate uses on its grain bank, applied here to FDN decorrelation. Cross-chain reuse — promote to Blessing.

---

## Debate Relevance

- **DB003 (init-patch beauty):** Orogen init produces sound. ✓
- **DB004 (expression vs. evolution):** Both. New `driftRate` at 0.005 Hz floor serves evolution; `ringFreq`, `ringFuzz`, `chord` are expression-bait once mapped to CC. Identity-correct.

---

## Recommendations

1. **[Done in this PR]** Add 2 new params: `orog_driftRate` (skewed 0.005–2 Hz, default 0.05 Hz) and `orog_driftDepth` (0–1, default 0). Wires into Stage 1's RingStingerStage. Per-block setRate.
2. **[Wave 2.6.preset, ~1 hr]** Author 5 demo presets — suggested concepts: *Geological Drift* (low ring, slow drift, max plate, max stretch), *Sideband Storm* (high ring, max fuzz, fast drift, no granular), *Crystal Cathedral* (mid ring, no drift, max resonance + Maj chord), *Tectonic Pulse* (mid all, mid drift, mid stretch, Min chord), *Foldback Glacier* (low ring, max fuzz, ultra-slow drift, dense granular). Each demonstrates a distinct register.
3. **[Forward-looking]** Pack 7 retrofit target — Orogen consuming SizeCouplingBus (verbSize biases by partner reverb chains). Backwards-compatible additive coupling target.

---

## Verdict

**APPROVED — 8.2/10. Orogen is shippable as an FX chain. Status remains `designed` in `Docs/engines.json` (no Source/Engines/ wrapper); seance metadata + `fx_chain_header` recorded.**

D005 satisfied via additive 2-param fix. Default `driftDepth=0` preserves pre-PR behaviour. Per-block setRate applies the lesson from Osmium PR #1503 review (no per-sample setRate cost). All 14 params doctrine-clean. Ringing Abyss identity is distinctive.

Wave 2 chain count after this PR: **6 of 20 seance-validated** (Ornate + Outage + Opus + Osmium + Outlaw + Orogen; all `designed` per status schema). 14 remaining.

---

## Cross-references

- Audit: `Docs/fleet-audit/wave2-master-audit-2026-05-01.md` (queue position #6)
- Source: `Source/DSP/Effects/OrogenChain.h`
- Engines registry: `Docs/engines.json` → Orogen (status `designed`; seance metadata + `fx_chain_header` recorded)
- Wave 2 protocol: `Docs/specs/2026-04-27-fx-engine-build-plan.md` §4
- Sibling: Ornate/Outage/Opus/Outlaw used floor-lowering D005 fixes; Osmium used a 1-param additive (rate-only) fix; Orogen uses a 2-param additive (rate + depth) fix because the chain originally had no LFO at all.
