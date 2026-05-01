# Otrium — Seance Verdict

**Date:** 2026-05-01
**Subject type:** FX chain (`Source/DSP/Effects/OtriumChain.h`, 262 lines)
**Position:** Pack 1 — Sidechain Creative · ChainID `31` · prefix `otrm_` (FROZEN)
**Spec:** `Docs/specs/2026-04-27-fx-pack-1-sidechain-creative.md` §2
**Shipping PR:** #1355 (merged 2026-04-30)
**First seance** — no prior verdict.

> **Note on protocol scope.** The seance methodology was designed for engines in `Source/Engines/`; Otrium is an FX chain. The criteria are adapted: no init patch, no `getSampleForCoupling()`, no MIDI handling at the chain layer (host-routed). What stays in scope: doctrine compliance, sonic intent, demo-preset pipeline, coupling-source publishing, spec-vs-implementation drift.

---

## Ghost Panel Summary

| Ghost | Score | Key Comment |
|-------|-------|-------------|
| Moog | 6 | "The phase-stagger is real and the followers are tuned with respect — but a sidechain mixer is judged by the hand on the control, and there is no hand here yet." |
| Buchla | 8 | "Triangular cross-rotation, partner audio routed by primitive identity, LFO floor at one-thousandth Hz. This is West Coast thinking on an East Coast feature. The `topology` knob is decorative — fix that and you have a pillar." |
| Smith | 7 | "PartnerAudioBus is a clean, deterministic, lock-free design — exactly the right shape. APVTS hookup is correct. Two parameters declared and not bound is unprofessional." |
| Kakehashi | 4 | "Zero presets. The chain that exists *to demonstrate the MegaCouplingMatrix to first-time users* ships with nothing for first-time users to load. This is the central failure." |
| Ciani | 6 | "Three partners pumping in a triangle is the most spatial idea in Pack 1. The output is collapsed to a single mean-gain VCA — the spatiality the spec promised lives in the envelopes, not the audio paths." |
| Schulze | 8 | "Pump rate floors at 0.001 Hz. A triangle of partner engines breathing over hours. This is the long-form patience the doctrine demanded." |
| Vangelis | 5 | "DNA tilt is two layers removed from the player. The chain hears no MIDI; the player must trust the host to route aftertouch to a parameter the spec said would respond directly. Faith, not feedback." |
| Tomita | 7 | "Cinematic premise — three colliding voices, none louder than another. The orchestration is on paper. The audition cannot happen without presets." |

**Consensus Score: 6.4 / 10** — *Provisional · DSP-validated, demo-blocked, two declared params dead*

---

## Doctrine Compliance

| Doctrine | Status | Commentary |
|----------|--------|-----------|
| D001 velocity → timbre | **PASS (indirect)** | FX chains are downstream of voicing; velocity arrives via DNA aggression → `dnaTilt` → depth. Spec-aligned. |
| D002 modulation | **PASS (weak)** | 3 envs + master LFO + DNA + couplingDepth blend. But **coupling sources `otrm.envA/B/C`, `phaseAngle`, `totalDuck` are not published** — spec §2 listed these and there is no publishing mechanism. The chain consumes coupling but does not emit it. |
| D003 physics | **N/A** | Control FX. |
| D004 dead params | **FAIL** | `otrm_topology` (Equilateral / Isoceles / Chaotic / Cyclical) and `otrm_syncMode` (Free / Sync) are declared, *not cached*, and have no audible effect (`OtriumChain.h:220-221` explicitly defer them). Two of thirteen = 15 % dead. |
| D005 must breathe | **PASS** | `pumpRate` range `0.001f, 40.0f` (`OtriumChain.h:171`). Floor satisfied. |
| D006 expression | **PASS (host-routed)** | Aftertouch / mod-wheel route to any `otrm_*` via the host's CC matrix. No chain-layer MIDI, by design for FX. |

---

## Sonic Identity

**Unique voice:** A 3-partner phase-staggered ducker is not done elsewhere in the fleet — `Compressor` / `MultibandCompressor` are 1-source. The wildcard *is* the triangle. Audibly distinct from any single-sidechain pump.

**Implementation vs. spec — significant drift:**

- Spec §2 stage 4: "**VCA Bank — 3 sidechain compressors** with cross-routed sidechains."
- Implementation `OtriumChain.h:147-155`: one VCA per stereo channel driven by `(gainA + gainB + gainC) / 3`. The three "paths" are collapsed into a single mean-gain VCA on the chain's stereo input.
- Audible result: the input is one stereo signal, so 3 separate paths and 1 path with mean-gain are mathematically equivalent **as long as the input is the same for all three**. The spec's intent — that A/B/C be three distinct audio sources from partners — is *not realised*: the chain's stereo input is the upstream FX bus, not three engine slots. The partner audio is read into envelope followers only; partner audio is never routed into the wet path.
- This is the architectural pivot the PR description called "Path A": Otrium pulls partner *envelopes* but ducks its own input. That's a clean simplification, but it means **"Three-Way Pad" (3 atmosphere engines)** as a preset cannot work the way the spec described — only the one engine routed *through* Otrium is in the audio path; the other two contribute envelopes only.

**Character range:** Excellent on paper (Equilateral / Isoceles / Chaotic / Cyclical via skew + topology). In practice, only `phaseSkew` varies the triangle — `topology` is dead. Range collapses from 4 distinct routings to "skew angle".

---

## Coupling Assessment

- **Consumes:** partner audio via `PartnerAudioBus` (3 slots, mono) ✓ · DNA `Aggression` axis on slot 0 ✓
- **Publishes:** **nothing.** Spec §2 listed `otrm.envA/B/C`, `phaseAngle`, `totalDuck` as coupling sources. No emission mechanism exists in the chain or in `MegaCouplingMatrix`. Spec calls this a "matrix demo" — the demo can't reflect back into the matrix.
- **Subtle issue:** `dnaTilt` reads only `dnaBus_->get(0, Aggression)` (`OtriumChain.h:118`). Spec said "partner aggression DNA tilts duck spectrum" — should read from `idxA/B/C`, not slot 0. Hardcodes the wrong slot.
- **Self-routing:** defaults `partnerA/B/C_idx = 0/1/2`. If Otrium sits on engine slot 0, partner A reads slot 0's pre-FX engine output, which is also Otrium's input. Not a bug (`PartnerAudioBus` publishes before FX), but subtly self-feeding. Worth a clamping rule: exclude the slot Otrium itself is attached to.

---

## Preset Review

**Zero presets.** Spec §8 calls for ≥5: Triangular Throb · Three-Way Pad · Bass Triangle · Chaos Topology · Stereo Triangle. None exist. Per Kakehashi: this is a launch blocker for the chain's stated purpose.

**Init-patch concern (Kakehashi DB003):** chain default state is `pumpDepth=0.5`, `mix=1.0`, `pumpRate=1 Hz`, partners 0/1/2 — first load *will* sound (provided partner slots have signal). Not blank. Reasonable default. ✓

---

## Blessing Candidates

- **Provisional B-XX: PartnerAudioBus pattern** — the bus itself is a clean, lock-free architectural primitive that opens the door for Oblate, Oligo, and any future cross-engine FX chain. **Not yet a Blessing — promote when ≥2 chains consume it.** Currently only Otrium does.
- **No Otrium-specific Blessing yet.** The triangular phase-stagger is the right idea but the dead `topology` param and the missing coupling-source publishing leave it half-built.

---

## Debate Relevance

- **DB003 (init-patch beauty vs. blank canvas):** Otrium can't have a "beautiful init" — it's an FX chain. The init is dependent on partner content. Position it as "blank canvas for partner audio."
- **DB004 (expression vs. evolution):** Otrium leans hard toward evolution (long pump rates, partner-driven envelopes) and weakly toward expression (no direct velocity / AT, only DNA-routed). Identity-correct.

---

## Recommendations (priority-ordered)

1. **[D004 blocker, ~30 LOC]** Wire `otrm_topology`. Suggested mapping: Equilateral = 120° fixed skew · Isoceles = `phaseSkew` user-controlled · Chaotic = stochastic per-LFO-period skew · Cyclical = engages `syncMode`. This unlocks the second dead param.
2. **[D004 blocker, ~50 LOC]** Wire `otrm_syncMode`. Use the `bpm` / `ppqPosition` already passed into `processBlock` (currently ignored at line 75). When Sync, quantize `pumpRate` to beat divisions (1/16 → 32 bars).
3. **[Coupling correctness, ~5 LOC]** Change line 118 `dnaBus_->get(0, ...)` to read from `idxA/B/C` partners, not slot 0. Either (a) average the three partners' aggression, or (b) sum them weighted by per-partner envelope contribution.
4. **[Spec compliance, design call]** Either publish `otrm.envA/B/C` + `totalDuck` as coupling sources (requires a `getCouplingSample`-style hook on chains, none exists today), **or** strike them from the spec. Recommend striking — Pack 1's PR description already pivoted to "Path A" and the matrix-publishing direction is closed off.
5. **[Demo unblock, ~1 hr, mechanical]** Author the 5 spec'd presets in `Presets/XOceanus/Coupling/`. Without these the chain's existence cannot be evaluated by ear and Kakehashi's score doesn't move.
6. **[Self-routing nicety, ~3 LOC]** In `clampSlot`, optionally accept the host slot index and rotate self-references to the next available partner. Prevents the "partner A is myself" foot-gun.
7. **[Re-seance gate]** Re-evaluate after items 1, 2, 5 land. Target: 8.0+. Item 4 is a separate spec hygiene fix.

---

## Verdict

**APPROVED CONDITIONAL — do not author dependent presets until items 1, 2 land.**

The DSP that exists is correct, the architecture (`PartnerAudioBus` + `DNAModulationBus`) is clean, and the doctrine floor is satisfied except for D004. But two declared parameters are dead and the demo-preset deficit is total. This is exactly the gate D2 was meant to enforce: **don't ship Triangular Throb / Three-Way Pad on a chain whose `topology` knob doesn't yet route differently.**

The spec → implementation drift on the "VCA Bank" stage (3 paths → 1 mean-gain VCA) is acceptable as Path A but should be documented in the chain header so the next reader doesn't expect three audio paths.
