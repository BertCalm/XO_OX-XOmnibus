# Otrium — Seance Re-Verdict (post-fix)

**Date:** 2026-05-01 (re-seance, same calendar day)
**Subject type:** FX chain (`Source/DSP/Effects/OtriumChain.h`, ~370 lines post-fix)
**Position:** Pack 1 — Sidechain Creative · ChainID `31` · prefix `otrm_` (FROZEN)
**Spec:** `Docs/specs/2026-04-27-fx-pack-1-sidechain-creative.md` §2 (post 2026-05-01 hygiene strike)
**Shipping PR:** #1474 (5-commit follow-through to merged #1355)
**Prior verdict:** `Docs/seances/otrium_seance_2026-05-01.md` — 6.4/10 PROVISIONAL

> **Re-seance trigger.** Per the prior verdict's recommendation 7, this re-seance was authored after items 1, 2, 5 of the prior recommendations landed (topology wiring, syncMode wiring, demo presets). Item 4 (spec hygiene) and items 3 + 6 (DNA tilt + clampSlot self-protection API) also landed in the same PR even though only 1, 2, 5 were strictly gating. Item 7 = this doc.

---

## Ghost Panel Summary (Δ from prior verdict)

| Ghost | Prior | Now | Δ | Key Comment |
|-------|-------|-----|---|-------------|
| Moog | 6 | **8.5** | +2.5 | "The hand is on the control. Topology rotates the triangle four ways and the sync knob does what its label promises. The bypassed parameters are no longer bypassed." |
| Buchla | 8 | **9.0** | +1.0 | "The Cyclical drift bug — frequency-detune masquerading as rotation — was caught before merge. The fix uses a separate drift-phase accumulator that does not reset on LFO wrap. This is the kind of architectural correction you only get when somebody is paying attention. Keep paying attention." |
| Smith | 7 | **8.5** | +1.5 | "All thirteen parameters are now bound and audible. APVTS hookup is clean. PartnerAudioBus + DNAModulationBus is shaping up as the right scaffold for the Pack 1 family. Two of three chains still have placeholder DSP — Pack 1 is not closed until Oblate and Oligo follow." |
| Kakehashi | 4 | **8.0** | +4.0 | "Five presets across four moods and four topologies. The audition is finally legible: load *Triangular Throb*, hold a chord, hear the matrix demo. The chain that exists to demonstrate the matrix to first-time users now actually demonstrates it. The format embeds chain config under an engine block — that is a workaround, not a schema. Fix the schema before Phase 2 ships nine packs of FX presets." |
| Ciani | 6 | **7.5** | +1.5 | "The Cyclical drift makes the triangle move through space rather than just oscillate in place. Better. The Path-A pivot still collapses three audio paths to one mean-gain VCA — Otrium ducks one stereo bus, not three independent partners. The spatiality is in the envelopes, not the audio. Acceptable, documented, but worth revisiting if a future chain wants to publish three discrete wet outputs." |
| Schulze | 8 | **9.0** | +1.0 | "Sync mode at 0.5 cycles per beat at 60 BPM gives a 0.5 mHz pump — slower than the Free-mode floor, *intentionally*. The comment on line 117 makes the trade explicit. This is the long-form patience the doctrine demanded, now with tempo-locking." |
| Vangelis | 5 | **7.0** | +2.0 | "DNA tilt now averages aggression across the three actual partners instead of a single hardcoded slot. The chain is finally listening to the engines it is ducking. Aftertouch still routes via host CC, not chain-internal — fine for FX, but the player is still one trust-step removed." |
| Tomita | 7 | **8.5** | +1.5 | "The orchestration is audible. Five concrete demonstrations of distinct topologies, two of them tempo-locked. *Three-Way Pad* in Equilateral / Sync at 1 cycle per beat is the cinematic premise realized. Ship it." |

**Consensus Score: 8.25 / 10** — *Approved · D004 fully resolved, demo-unblocked, four distinct topologies audible, tempo-sync delivered.*

(Computed: average of 8.5, 9.0, 8.5, 8.0, 7.5, 9.0, 7.0, 8.5 = 66.0 / 8 = 8.25.)

---

## Doctrine Compliance (re-evaluated)

| Doctrine | Status | Δ | Commentary |
|----------|--------|---|------------|
| D001 velocity → timbre | **PASS (indirect)** | — | Unchanged — DNA aggression → dnaTilt → depth. Spec-aligned. |
| D002 modulation | **PASS (strong)** | ↑ | 3 envs + master LFO + drift phase (Cyclical) + DNA-averaged-per-partner + couplingDepth blend. Coupling sources struck from spec (item 4) — clean accounting; the chain is a pure consumer, not a publisher. |
| D003 physics | **N/A** | — | Control FX. |
| D004 dead params | **PASS** | ↑↑ | 13/13 parameters audible. `otrm_topology` routes 4 distinct phase strategies (Equilateral / Isosceles / Chaotic / Cyclical). `otrm_syncMode` reinterprets `pumpRate` as cycles-per-beat using the host `bpm` previously discarded. Was the only failing doctrine in the prior verdict. |
| D005 must breathe | **PASS** | — | `pumpRate` range `0.001f, 40.0f` enforces the floor in Free mode. Sync mode can dip below the floor at low tempos — documented as intentional in the source comment. The doctrine is about the existence of the slow path, not the ceiling at which it stops. |
| D006 expression | **PASS (host-routed)** | — | Aftertouch / mod-wheel route to any `otrm_*` via host CC matrix. Unchanged. |

**All six doctrines pass.** This was the gate the prior verdict held the chain against.

---

## Sonic Identity (re-evaluated)

**Unique voice:** A 3-partner phase-staggered ducker with 4 audible topology variants is genuinely not done elsewhere in the fleet. Compare:

- **Compressor / MultibandCompressor** — 1 sidechain source, no phase stagger
- **OnrushChain** — single transient swell, not partner-driven
- **OmnistereoChain** — stereo width, not coupling-driven

The wildcard *is* the triangle, and the triangle now has four shapes plus tempo-sync.

**Implementation vs. spec — drift status:**

- Spec §2 stage 4: "VCA Bank — 3 sidechain compressors." Implementation: 1 mean-gain VCA on the chain's stereo input ("Path A" pivot).
- **Status:** acknowledged in prior verdict, documented in chain header, not regressed in this PR. Re-evaluation: this is the right call for v1. Three-discrete-output paths require pulling partner audio *into* the wet path, not just the envelopes. That's a deeper architectural change appropriate for a Phase 2 follow-on, not a bug fix.

**Character range:** ↑↑. From "skew angle only" to:

| Topology | Behaviour |
|----------|-----------|
| Equilateral | Forced 120°/240° — `phaseSkew` ignored, perfect triangle |
| Isosceles | User `phaseSkew` between A↔B and B↔C |
| Chaotic | Per-partner offsets re-randomised per LFO wrap (deterministic LCG, reproducible) |
| Cyclical | Slow continuous drift via separate phase accumulator (1/8 LFO rate, doesn't reset on wrap, 2× drift on C) |

Plus orthogonal Free / Sync mode.

---

## Coupling Assessment (re-evaluated)

- **Consumes:** partner audio via `PartnerAudioBus` (3 slots, mono) ✓ · DNA `Aggression` axis on `idxA/B/C` partners (averaged) ✓ — **prior bug fixed (item 3)**
- **Publishes:** **nothing — by spec.** §2 was edited to strike `otrm.envA/B/C`, `otrm.phaseAngle`, `otrm.totalDuck` (item 4). Otrium is now formally a coupling consumer, not a publisher. The `getCouplingSample`-style hook for chains is deferred until a second chain needs it.
- **Self-routing:** `setHostEngineSlot()` API added (item 6). Currently dormant — `EpicChainSlotController` is a master-bus router, not per-engine, so there is no host engine slot to wire today. The protection rotates partner indices forward when the host slot is set; sits ready for any future per-engine FX routing.

---

## Preset Review (re-evaluated)

**5 presets shipped** in `Presets/XOceanus/Coupling/`:

| Preset | Topology | Sync | Engines | Demonstrates |
|--------|----------|------|---------|--------------|
| Coupling_Triangular_Throb | Equilateral | Free / 1.2 Hz | Onset, Overbite, Octopus | Even three-way pump, fast attack |
| Coupling_Three_Way_Pad | Equilateral | Sync / 1 cycle/beat | Obsidian, OceanDeep, Opal | Tempo-locked slow rotation, long release |
| Coupling_Bass_Triangle | Isosceles | Free / 2 Hz | Ogre, Oaken, Omega | Narrow 75° skew → focused swing |
| Coupling_Chaos_Topology | Chaotic | Free / 1.5 Hz | Onkolo, Ouroboros, Oxbow | Per-cycle re-randomization, deterministic |
| Coupling_Stereo_Triangle | Cyclical | Sync / 0.5 cycles/beat | OpenSky, Oracle, Obsidian | Slow continuous drift across cycles |

Each preset isolates one topology + sync combination so the audition is legible. Together they cover the full character range.

**Preset format note:** Otrium chain config (`slot1_chain`, `otrm_*`) is embedded under the first listed engine's parameter block as a workaround for the engine-keyed `.xometa` schema. Documented in each preset's `description`. **Action item for Phase 2:** before nine packs of FX presets ship, extend the `.xometa` schema with a first-class `controllerParams` or `fxSlots` section so chain-prefixed parameters live in their own namespace. This is **not** a blocker for Pack 1 — `applyPreset()` resolves full APVTS IDs as-is, so the workaround is correct, just semantically loose.

---

## Blessing Candidates

- **B-XX-provisional: PartnerAudioBus pattern** — *still provisional.* Promote when Oligo and/or Oblate also consume it (Wave 0 sessions 0.3 + 0.4 in `Docs/plans/2026-05-01-fx-engine-multi-session-plan.md`).
- **Notable technique (not Blessing-tier alone):** the Cyclical drift-phase accumulator pattern — separate state that runs in parallel to the main LFO and does not reset on wrap. This is the right primitive for "rotating geometry in audio" and is reusable. Note it for the engine-color-table when the second chain needs it.

---

## Debate Relevance

- **DB003 (init-patch beauty vs. blank canvas):** Otrium's defaults still produce sound when partners have signal. Re-seance position unchanged: "blank canvas for partner audio."
- **DB004 (expression vs. evolution):** Cyclical + Sync now decisively serves *evolution*. Expression remains weakly served (no chain-layer MIDI, by design for FX). Identity-correct.

---

## Recommendations (post-re-seance)

The prior verdict's 7-item list is now resolved or formally deferred:

| # | Prior Status | Now |
|---|---|---|
| 1 | `topology` wiring | ✅ done (#1474) |
| 2 | `syncMode` wiring | ✅ done (#1474) |
| 3 | `dnaTilt` reads partners | ✅ done (#1474, this re-seance commit) |
| 4 | strike or implement coupling sources | ✅ struck from spec (#1474) |
| 5 | 5 demo presets | ✅ shipped (#1474) |
| 6 | `clampSlot` self-protection | ✅ API in place; dormant by design (#1474) |
| 7 | re-seance | ✅ this doc |

**New recommendations (forward-looking):**

1. **[Phase 2 hygiene, ~50 LOC]** Extend the `.xometa` schema with a `controllerParams` or `fxSlots` section so chain-prefixed parameters have a proper home. The current engine-block embed is correct but loose. Land before Pack 2 demo presets ship.
2. **[Pack 1 closure]** Implement real Oligo DSP (Wave 0 session 0.3) and real Oblate DSP (Wave 0 session 0.4) per `Docs/plans/2026-05-01-fx-engine-multi-session-plan.md`. PartnerAudioBus Blessing promotion follows.
3. **[Future]** If a second consumer ever needs `getCouplingSample`-style chain-side coupling publishing, revisit the spec §2 strike. The strike was an economic call, not a permanent design exclusion.

---

## Verdict

**APPROVED — 8.25/10. Pack 1 chain Otrium is shippable.**

D004 is fully resolved. Five presets demonstrate the four topologies and both sync modes. The Cyclical-drift bug was caught and fixed before merge. PartnerAudioBus + DNAModulationBus + averaged-partner DNA tilt is the correct shape for the Pack 1 family.

The chain's headline character — *the matrix demo, audible* — is now true.

The remaining caveats are forward-looking: Pack 1 is not closed until Oligo and Oblate ship real DSP. The seance gate is satisfied for Otrium specifically.

---

## Cross-references

- Prior verdict: `Docs/seances/otrium_seance_2026-05-01.md`
- PR: #1474 (5 commits — D004 fix, Copilot review fixes, items 3/4/6, plan doc, presets)
- Spec (post-strike): `Docs/specs/2026-04-27-fx-pack-1-sidechain-creative.md` §2
- Multi-session plan: `Docs/plans/2026-05-01-fx-engine-multi-session-plan.md`
- Source: `Source/DSP/Effects/OtriumChain.h`
