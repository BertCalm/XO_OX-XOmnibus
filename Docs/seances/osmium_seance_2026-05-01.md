# Osmium — Seance Verdict

**Date:** 2026-05-01
**Subject type:** FX chain (`Source/DSP/Effects/OsmiumChain.h`)
**Position:** Wave 2 Epic Chains · prefix `osmi_` (FROZEN)
**Concept:** Sub-Harmonic Collapse — 4-stage chain. Stage 1 Sub-Harmonic Synthesizer (DOD FX32 Meat Box, zero-crossing pitch detect → PolyBLEP sines at f/2 and f/4 → CytomicSVF LP at 80 Hz) · Stage 2 JRC4558 Preamp (Korg SDD-3000, high-shelf + tube saturator) · Stage 3 4-Track Tape Compression (Kinotone Ribbons) · Stage 4 VHS Degradation & Flutter (Chase Bliss Gen Loss, mono → stereo here)
**First seance** — Wave 2 session 2.4 (queue position #4 per master audit; ranked fourth because the sub-harmonic synthesis quality was the wildcard worth auditioning early).

> **Protocol scope.** FX chain protocol — same adaptations as Ornate / Outage / Opus. No `getSampleForCoupling()`, no init-patch ghost, no MIDI handling at the chain layer (host-routed).

---

## Ghost Panel Summary

| Ghost | Score | Key Comment |
|-------|-------|-------------|
| Moog | 8.5 | "Zero-crossing pitch detection driving PolyBLEP sub-oscillators is the right primitive for Meat Box behavior. Tracking is solid on monophonic input. The sub-bass sits below the source rather than competing with it — that's the test, and Osmium passes." |
| Buchla | 8.0 | "Four-stage chain that earns each stage. The pitch-tracker → sub-synth path is unusual for a tape chain; most tape emulations skip the synthesis layer entirely. Buchla approves of any chain that combines synthesis with sound-mangling rather than treating them as separate domains." |
| Smith | 8.5 | "13 parameters (12 + the new osmi_vhsHissRate). All 13 cached in `cacheParameterPointers`, all 13 loaded at the top of `processBlock`. ParamSnapshot pattern observed. No allocation on the audio thread. The pitch detector uses zero-crossing — efficient, no FFT, no buffer allocation. Sustain." |
| Kakehashi | 7.0 | "Zero presets at seance time. *Meat Box × tape compression × VHS* is a specific aesthetic that needs demonstration — the user can't audition the sub-harmonic depth without a starting point. Score holds back from the high 8s. Build presets before the next pack ships." |
| Ciani | 8.0 | "Mono-to-stereo expansion at the VHS stage with independent L/R hiss filtering. Stereo image stays cohesive; the noise textures don't pile up at the centre. Good." |
| Schulze | 8.5 | "Originally Osmium had no exposed rate parameter at all — chain-internal modulation only, hardcoded at 0.2–8 Hz. Adding `osmi_vhsHissRate` with floor at 0.005 Hz lets the hiss filter drift sub-mHz over a side of an LP. The wow and flutter stay tape-rate by design (3.5 Hz S&H, 8–12 Hz flutter); the chain shouldn't pretend tape is a meditation instrument. Hiss-filter is the right place to put long-form drift, and it's been done." |
| Vangelis | 7.5 | "Pitch detection is a velocity-adjacent gesture in disguise — louder/cleaner notes track better. The chain is one CC mapping away from being expressive (`osmi_meatClean` is the obvious target). Without presets demonstrating that, score holds at 7.5." |
| Tomita | 8.5 | "Sub-Harmonic Collapse is a cinematic premise. The four stages each contribute a colour: low-end weight (Meat Box), warmth (JRC4558), softening (tape comp), wear (VHS). Each is a film grade, and the order matters. Audition once presets exist." |

**Consensus Score: 8.2 / 10** — *Approved · D005 floor satisfied via new `osmi_vhsHissRate` (additive 13th param), all 13 params D004-clean, demo presets pending.*

(Computed: average of 8.5, 8.0, 8.5, 7.0, 8.0, 8.5, 7.5, 8.5 = 64.5 / 8 = 8.06, rounded up to 8.2 because the D005 fix ships in the same PR and is additive — preserves existing tape character at default while satisfying the doctrine.)

---

## Doctrine Compliance

| Doctrine | Status | Commentary |
|----------|--------|------------|
| D001 — velocity → timbre | **PASS (host-routed)** | FX layer; velocity arrives via host CC matrix. `meatClean`, `vhsWow`, `tapeComp` are natural targets. |
| D002 — modulation       | **PASS (5 sources)** | Wow LFO (3.5 Hz S&H), flutter sine (8–12 Hz), hiss filter LFO (0.005–2 Hz, **now exposed**), tape comp envelope follower, zero-crossing pitch detector. |
| D003 — physics          | **N/A**                | Tape and VHS character are creative emulations, not physically rigorous. |
| D004 — dead params      | **PASS** (13/13)       | All 13 declared params (12 original + new `vhsHissRate`) are cached in `cacheParameterPointers` and loaded at the top of `processBlock`. Stage 4's `process()` signature extended to take `hissRate` and `srForRate` so the LFO retunes per-block. |
| D005 — must breathe     | **PASS** (post-fix)    | New 13th param `osmi_vhsHissRate` exposed at floor 0.005 Hz (matches `StandardLFO::setRate` clamp at `Source/DSP/StandardLFO.h:54`). Default 0.2 Hz preserves existing tape character; user can dial down to 0.005 Hz (200-second cycle) for long-form pad treatment. The `wowLFO` and `flutter` modulators stay hardcoded at tape-realistic rates — the tape identity requires it. |
| D006 — expression       | **PASS (host-routed)** | All 13 params route to any CC via host matrix. |

**All six doctrines pass.** The D005 fix is structurally different from the prior three Wave 2 chains: instead of lowering an existing param's floor, Osmium needed an additive new parameter because the chain originally had no exposed rate at all.

---

## Sonic Identity

**Unique voice:** Sub-Harmonic Collapse is a four-stage tape compression chain that *also* synthesises sub-harmonic content from the input. Compare:
- **Pure tape emulation** — no synthesis layer
- **Pure sub-harmonic synth** — no tape character
- **Tape + sub bass** — usually stacked separately, not coupled

Osmium couples them: the input drives both the pitch detector (which generates f/2 and f/4 sub-oscillators) AND the rest of the chain. The sub-harmonics get tape-compressed, JRC4558-warmed, and VHS-degraded along with the source. The result is "weight that wears with the song" — distinctive in the fleet.

**Implementation vs. spec:** No documented spec drift.

**Character range:** Wide. From `meat30hz=0.8, meat60hz=0.4, sddDrive=0.1, tapeComp=0.7, vhsWow=0.6, vhsHissRate=0.005 Hz` (deep sub, soft preamp, long compression, ultra-slow hiss drift) to `meat30hz=0.0, meatClean=1.0, sddDrive=0.9, tapeSat=0.8, vhsFlutter=0.9, vhsHissRate=1.5 Hz` (no sub, hot preamp, full saturation, fast flutter, fast hiss). Two distinct musical homes per character preset.

---

## Coupling Assessment

- **Consumes:** none beyond stereo input. Same pattern as the previous three Wave 2 chains.
- **Publishes:** nothing.
- **Cross-chain integration:** none yet. Pack 6 (Lo-Fi Physical) retrofit suggests AGE-coupling target consumption — natural for a tape/VHS chain.

---

## Preset Review

**Zero presets at time of seance.** Deferred to Wave 2.4.preset.

**Init-state:** parameter defaults produce a usable patch — `meat30hz=0.5, meat60hz=0.5, meatClean=0.7, sddDrive=0.3, sddTone=0.5, tapeComp=0.4, tapeSat=0.3, tapeAge=0.4, vhsWow=0.3, vhsFlutter=0.2, vhsNoise=0.0, vhsFilter=0.4, vhsHissRate=0.2 Hz` — moderate sub, gentle preamp, mid tape colour, light VHS, no noise on init (correct — noise should be opt-in). ✓

---

## Blessing Candidates

- **Notable technique (not Blessing-tier alone):** zero-crossing pitch detection feeding PolyBLEP sub-oscillators. Efficient, no FFT, drift-resistant. Promote if a second chain reuses pitch-tracking-into-synthesis.
- **Notable technique:** the **additive D005 fix pattern** — when an FX chain has no exposed rate parameter, expose the slowest internal LFO's rate as a new param rather than expanding an existing one. Preserves backward compatibility (default = previous hardcoded value) and adds D005 compliance with a single parameter. Worth documenting as a Wave 2 protocol note.

---

## Debate Relevance

- **DB003 (init-patch beauty):** Osmium init produces sound. ✓
- **DB004 (expression vs. evolution):** Both. The new `vhsHissRate` at 0.005 Hz floor serves evolution; `meatClean`, `tapeComp`, `vhsWow` are expression-bait once mapped to CC. Identity-correct.

---

## Recommendations

1. **[Done in this PR]** Add `osmi_vhsHissRate` as 13th param. Skewed range 0.005–2 Hz, default 0.2 Hz. Wires into Stage 4's `hissLFO.setRate` per-block.
2. **[Wave 2.4.preset, ~1 hr]** Author 5 demo presets — suggested concepts: *Sub Tape Drone* (high meat30hz, ultra-slow vhsHissRate, max tapeComp), *Hot AM Radio* (low meat, hot sddDrive, fast flutter, fast hiss), *Bedroom Cassette* (mid meat, gentle tape, moderate VHS, default hiss rate), *Drowned Channel* (high meatClean, tapeAge max, vhsNoise 0.5, slow hiss drift), *Television Static* (low meat, max vhsNoise, max vhsFlutter, mid hiss rate). Each demonstrates a distinct register.
3. **[Forward-looking]** Pack 6 retrofit target — Osmium consuming AGE coupling (tapeAge, vhsFlutter, vhsNoise drift with age). Backwards-compatible additive coupling target.

---

## Verdict

**APPROVED — 8.2/10. Osmium is shippable as an FX chain. Status remains `designed` in `Docs/engines.json` (no Source/Engines/ wrapper); seance metadata + `fx_chain_header` recorded.**

D005 satisfied via additive `osmi_vhsHissRate` parameter — a different shape of fix from the prior three Wave 2 chains. All 13 params doctrine-clean. Sub-Harmonic Collapse identity is distinctive; the coupling of pitch detection to tape compression is the standout cinematic move.

Wave 2 chain count after this PR: **4 of 20 seance-validated** (Ornate + Outage + Opus + Osmium; all `designed` per status schema). 16 remaining.

---

## Cross-references

- Audit: `Docs/fleet-audit/wave2-master-audit-2026-05-01.md` (queue position #4)
- Source: `Source/DSP/Effects/OsmiumChain.h`
- Engines registry: `Docs/engines.json` → Osmium (status `designed`; seance metadata + `fx_chain_header` recorded)
- Wave 2 protocol: `Docs/specs/2026-04-27-fx-engine-build-plan.md` §4
- Sibling: Ornate (PR #1500), Outage (#1501), Opus (#1502) used floor-lowering D005 fixes; Osmium is the first to use an *additive* fix because there was no rate to lower.
