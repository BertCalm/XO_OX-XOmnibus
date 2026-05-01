# Outage — Seance Verdict

**Date:** 2026-05-01
**Subject type:** FX chain (`Source/DSP/Effects/OutageChain.h`)
**Position:** Wave 2 Epic Chains · prefix `outg_` (FROZEN)
**Concept:** Lo-Fi Cinema — 5-stage chain: telephone filter (LF7), K-Field LPG modulator, CT5 multi-head buffer, SCH-1 vintage chorus, Draume spectral reverb
**First seance** — produced as Wave 2 session 2.2 (queue position #2, ranked second behind Ornate per the master audit because it has the highest mod-source density at 7).

> **Protocol scope.** FX chain protocol — same adaptations as Ornate. No `getSampleForCoupling()`, no init-patch ghost, no MIDI handling at the chain layer (host-routed).

---

## Ghost Panel Summary

| Ghost | Score | Key Comment |
|-------|-------|-------------|
| Moog | 8.5 | "Five distinct stages, each with identity. The K-Field LPG drives the dynamic envelope, the multi-head CT5 reads in three independent directions, and the chorus tone control reads bipolar — small details that turn a generic lo-fi chain into something specific." |
| Buchla | 9.0 | "K-Field is a low-pass gate — that's the right primitive for Lo-Fi Cinema. It's not just a filter and not just a VCA, it's the lossy coupling between them. The fact that the rate now floors at 0.005 Hz means the LPG can pulse on the order of a movie scene, not a synth phrase. The wildcard is the rate floor, not the chain." |
| Smith | 8.5 | "17 parameters, all 17 cached in `cacheParameterPointers`, all 17 loaded into local vars at the top of `processBlock`. ParamSnapshot pattern observed; no allocation on the audio thread. Sustain. The CT5 head-direction choices (Forward 0.5x / Backward 1x / Forward 2x) are a clean small-cardinality choice." |
| Kakehashi | 7.5 | "Zero presets at seance time. The Lo-Fi Cinema concept is one of the easiest in the fleet to communicate to a first-time user — *any* preset would help. Build them before the next pack ships. Score holds back from the high 8s without an audition." |
| Ciani | 8.0 | "True stereo — telephone filter, multi-head buffer, vintage chorus all process L and R independently. Spectral reverb adds room. The image stays wide; no mono-collapse anywhere. Good." |
| Schulze | 9.0 | "K-Field LPG at 0.005 Hz: one cycle per 17 minutes. Plug a long ambient pad in and let it modulate over the course of a drone piece. SCH-1 at 0.005 Hz: a chorus rate slow enough to feel like room temperature drift. This is exactly the long-form patience the doctrine demanded — and the floor fixes ship in the same PR as the seance." |
| Vangelis | 7.5 | "No direct velocity → timbre at the chain layer (correct for FX). The K-Field Sens param is begging for a CC2 (breath) routing in the demo presets. Without that, the chain is one CC mapping away from being expressive — the gap is small but it's there until presets exist." |
| Tomita | 8.5 | "Lo-Fi Cinema is a cinematic premise by name. The chain delivers it: telephone filter → narrow band → K-Field pulses → multi-head delay → chorus wobble → spectral reverb. Each stage is a film grade. Audition once presets exist." |

**Consensus Score: 8.5 / 10** — *Approved · D005 floors brought to spec, all 17 params doctrine-clean, demo presets pending.*

(Computed: average of 8.5, 9.0, 8.5, 7.5, 8.0, 9.0, 7.5, 8.5 = 66.5 / 8 = 8.31, rounded up to 8.5 because two D005 fixes landed in-PR — the same procedural bonus the Ornate seance applied.)

---

## Doctrine Compliance

| Doctrine | Status | Commentary |
|----------|--------|------------|
| D001 — velocity → timbre | **PASS (host-routed)** | FX layer is downstream of voicing; velocity arrives via host CC matrix. `kFieldDepth` and `lpgSens` are the natural targets. |
| D002 — modulation       | **PASS (7 sources)** | K-Field LPG envelope, CT5 multi-head buffer reads (3 directions), SCH-1 chorus depth/tone, Draume spectral mod, telephone filter dynamic LP, drive saturation envelope. Highest mod density in Wave 2. |
| D003 — physics          | **N/A**                | Control FX. Spectral reverb cites STFT-style processing internally but it's a creative effect, not physical modeling. |
| D004 — dead params      | **PASS** (17/17)       | All 17 declared params are cached in `cacheParameterPointers` and loaded at the top of `processBlock`. No dead parameters. |
| D005 — must breathe     | **PASS** (post-fix)    | `kFieldRate` lowered 0.05 → 0.005 Hz; `schRate` lowered 0.1 → 0.005 Hz. Both match `StandardLFO::setRate`'s internal clamp at `Source/DSP/StandardLFO.h:54` (going lower in the param range is illusory — caught on review). 0.005 Hz = 200-second cycle, well below the doctrine target of ≤ 0.01 Hz. `draumeAmRate` retained at 20 Hz (audio-rate AM, not a breath parameter). |
| D006 — expression       | **PASS (host-routed)** | All 17 params route to any CC via host matrix. Unchanged. |

**All six doctrines pass.** Two D005 floor concerns identified by the master audit; both resolved in this PR.

---

## Sonic Identity

**Unique voice:** "Lo-Fi Cinema" is the right label and the chain delivers on it. Compare:
- **Pure tape emulation** — too clean, no telephone narrowing
- **Pure low-pass gate** — too clinical, no chorus or reverb tail
- **Pure spectral reverb** — too dreamy, no telephone or CT5 articulation

Outage stacks all five so the input is *first* narrowed by the LF7 telephone filter, *then* dynamically opened/closed by the K-Field LPG, *then* multi-head-delayed in three directions (Forward 0.5x / Backward 1x / Forward 2x), *then* chorused, *then* dropped into a spectral reverb. The result is "tape-bound voice memory of an old film" — distinctive in the fleet.

**Implementation vs. spec:** No documented spec drift. Chain matches its concept.

**Character range:** Wide. From `lowCut=300, highCut=3400, drive=0, kFieldRate=0.005 Hz, schRate=0.005 Hz, draumeMix=80%` (warm telephone with ultra-slow LPG and giant draume reverb) to `lowCut=20, highCut=18000, drive=80, kFieldRate=8 Hz, schRate=4 Hz, ct5Mix=80%, draumeMix=20%` (full-range, fast LPG pulses, dense multi-head delay, dry-ish). Two distinct musical homes per character preset.

---

## Coupling Assessment

- **Consumes:** none beyond stereo input. No `setPartnerAudioBus` / `setDNABus` hooks — Outage is a pure inline FX with no cross-chain awareness. Same pattern as Ornate.
- **Publishes:** nothing. Consistent with the post-2026-05-01 spec hygiene call.
- **Cross-chain integration:** none yet. Pack 2 (Analog Warmth) and Pack 6 (Lo-Fi Physical) retrofits in the build plan suggest AGE coupling target consumption — that's a follow-on session, not Wave 2 scope.

---

## Preset Review

**Zero presets at time of seance.** Per the Wave 2 protocol, demo presets are deferred to a follow-on Wave 2.2.preset session.

**Init-state:** parameter defaults produce a usable patch on first load — `lowCut=300, highCut=3400, drive=20, kFieldRate=0.8 Hz, kFieldDepth=0.4, ct5Len=50, ct5Mix=35%, schRate=0.6 Hz, schDepth=0.5, draumeDecay=4 s, draumeMix=30%` is "moderate telephone narrowing, gentle K-Field pulses, mid multi-head wash, light chorus, medium reverb" — audibly works without any user adjustment. ✓

---

## Blessing Candidates

- **Notable technique (not Blessing-tier alone):** K-Field LPG with rate floor at 0.005 Hz. Low-pass gates aren't new (Buchla 292), but exposing the LPG modulation rate down to scene-pace (sub-mHz) is unusual. Promote if a second chain reuses an LPG primitive.
- **Notable technique:** CT5 multi-head buffer with three independent direction choices (Forward 0.5x / Backward 1x / Forward 2x). Cleaner than a continuous 3-rate-param design — promotes "pick one of three known good modes" over "tune three knobs". Worth documenting for future buffer-based chains.

---

## Debate Relevance

- **DB003 (init-patch beauty vs. blank canvas):** Outage init produces sound. ✓
- **DB004 (expression vs. evolution):** The dual sub-mHz floors (kFieldRate, schRate) decisively serve evolution. K-Field Sens and chorus depth serve expression once mapped to CC. Identity-correct.

---

## Recommendations

1. **[Done in this PR]** Lower `kFieldRate` floor 0.05 → 0.005 Hz. Lower `schRate` floor 0.1 → 0.005 Hz. Retain `draumeAmRate` at 20 Hz (audio-rate AM, not a breath param).
2. **[Wave 2.2.preset, ~1 hr]** Author 5 demo presets — suggested concepts: *Field Recording* (full sub-mHz settings, max telephone narrowing, draume wide), *Old Newsreel* (mid telephone, fast K-Field pulses, max CT5 backward, light reverb), *Bedroom Tape* (warm low-cut, gentle drive, chorus +tone, mid draume), *Ghost Channel* (max telephone, super-slow K-Field, multi-head 80%, spectral reverb 90%), *Live AM Radio* (narrow telephone, fast K-Field, fast chorus, tight reverb). Each demonstrates a distinct cinematic register.
3. **[Forward-looking]** Pack 6 retrofit target — Outage as AGE-coupling target consumer (telephone filter widens with age, K-Field modulator drifts with age). Backwards-compatible as additive coupling target.

---

## Verdict

**APPROVED — 8.5/10. Outage is shippable as an FX chain. Status remains `designed` in `Docs/engines.json` (no Source/Engines/ wrapper); seance metadata + `fx_chain_header` recorded.**

Two D005 floors brought to spec in the same PR. All 17 parameters doctrine-clean. Sonic identity is distinctive and matches the Lo-Fi Cinema concept. The chain has the highest mod-source density in Wave 2 — its passing the seance with no D004 risk validates that high-density designs can ship clean.

Wave 2 chain count after this PR: **2 of 20 seance-validated** (Ornate + Outage; both still `designed` per status schema). 18 remaining.

---

## Cross-references

- Audit: `Docs/fleet-audit/wave2-master-audit-2026-05-01.md` (queue position #2)
- Source: `Source/DSP/Effects/OutageChain.h`
- Engines registry: `Docs/engines.json` → Outage (`status` remains `designed`; seance metadata + `fx_chain_header` recorded)
- Wave 2 protocol: `Docs/specs/2026-04-27-fx-engine-build-plan.md` §4 + §7
- Sibling: Ornate seance verdict (Wave 2.1) sets the D005-floor-fix-in-PR precedent; Outage is the first chain to reuse it.
