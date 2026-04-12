# XOceanus V1 Scope Revision
**Date:** 2026-03-23
**Status:** LIVING DOCUMENT — no hard release deadline; quality before ship
**Supersedes:** `Docs/plans/v1-launch-plan.md`, `Docs/v1-launch-plan-2026-03-20.md`
**Related:** `Docs/fleet-health-dashboard-2026-03-24.md` (detailed per-engine readiness)

---

## Philosophy

> "We don't need to run to a V1. I am looking to perfect everything currently built. Let's not be precious about the release since there is no hard deadline in place."

V1 ships when the fleet is ready — not on a calendar date. The purpose of this document is to define **what ships in V1** (engine selection, quality bar, exit criteria) so every downstream decision has a reference point. The scope is intentionally focused: a curated subset of the 76-engine fleet at the quality bar the product deserves.

---

## V1 Scope Target

**Formula:** OBRIX flagship + 6–8 FX engines + 20–25 curated synthesis engines = **28–34 total**

The full 76-engine fleet remains active in the personal build. V1 gating applies to public distribution only.

---

## Quality Bar for V1 Inclusion

An engine must meet **all** of the following to be included in V1:

| Criterion | Threshold |
|-----------|-----------|
| Seance score | ≥ 8.5 (or unanimous non-numeric approval from ghost council) |
| Preset depth | ≥ 100 factory presets (recommended) |
| All 6 Doctrines | D001–D006 fully resolved — no dead parameters, LFOs breathe, velocity shapes timbre, expression input wired |
| No unresolved P0 DSP bugs | Zero known audio-correctness failures |
| `frozenPrefixForEngine` entry | Parameter prefix registered in `PresetManager.h` |
| Seance verdict documented | Entry in `Docs/seance_cross_reference.md` |

---

## Confirmed V1-Ready Engines (21)

These engines meet all criteria as of this revision:

| # | Engine | Seance | Presets | Notes |
|---|--------|--------|--------:|-------|
| 1 | **Obrix** | 9.4 roadmap | 466 | FLAGSHIP — mandatory |
| 2 | **Obscura** | Unanimous | 143 | Physics-as-synthesis |
| 3 | **Offering** | 8.8 | 154 | Psychology-as-DSP (B038); Boom bap drums |
| 4 | **Onset** | Ahead of industry | 480 | XVC drum engine (B002) |
| 5 | **Opal** | Approved | 357 | Granular; coupling crown jewel |
| 6 | **Opera** | 8.85 | 156 | Kuramoto additive-vocal; OperaConductor (B035) |
| 7 | **Optic** | Revolutionary | 308 | Zero-audio paradigm (B005) |
| 8 | **Oracle** | 8.6 | 241 | GENDY + Maqam (B010) |
| 9 | **Orbital** | Approved | 262 | Group Envelope System (B001) |
| 10 | **Organon** | 8/8 PASS | 365 | Variational Free Energy metabolism (B011) |
| 11 | **Origami** | Approved | 216 | STFT fold synthesis |
| 12 | **Osprey** | Approved | 166 | ShoreSystem (B012) |
| 13 | **Osteria** | Production-grade | 234 | ShoreSystem shared (B012) |
| 14 | **Ostinato** | 8.7 | 229 | Modal membrane synthesis (B017); 96 rhythm patterns (B019) |
| 15 | **Oto** | 8.6 post-fix | 34 | Chef quad — preset expansion in progress |
| 16 | **Ouie** | 8.5 | 396 | HAMMER axis (B025); duophonic |
| 17 | **Ouroboros** | Production-ready | 346 | Chaos attractors; Leash (B003) |
| 18 | **Outwit** | 8.7 | 593 | Cellular automata stepsynth |
| 19 | **Oware** | 8.7 | 175 | Mallet physics — Chaigne 1997 (B032) |
| 20 | **Oxbow** | 9.0 | 175 | Chiasmus FDN entangled reverb synth |
| 21 | **Oxytocin** | 9.5 (fleet leader) | 130 | Circuit × love topology; Note Duration (B040) |

---

## Under Consideration (15)

These engines are strong candidates but require a formal re-seance, a preset push, or a targeted DSP fix before final confirmation:

| Engine | Seance | Presets | Blocker |
|--------|--------|--------:|---------|
| Overbite | Full approval | 255 | Needs formal numeric re-seance |
| Oblong | ~8.5 est. | 802 | CuriosityEngine responsiveness; needs formal score |
| Obese | ~8.5 est. | 408 | Post-fix recovery; needs formal re-seance |
| OddOscar | ~8.5 est. | 444 | Post-fix recovery; no retreat doc |
| OddfeliX | ~8.5 est. | 531 | Post-fix recovery; no retreat doc |
| Obsidian | ~8.2 est. | 280 | Below 8.5 threshold; post-fix estimate |
| Ocelot | ~8.5 est. | 271 | EcosystemMatrix now live; formal re-seance needed |
| Octave | 8.01 → ~8.7 | 25 | Post-fix strong; only 25 presets — critical gap |
| Octopus | ~8.5 est. | 257 | Aftertouch fixed; formal numeric re-seance needed |
| Orca | ~8.6 est. | 270 | Echolocation LFO strong; needs formal numeric seance on record |
| OpenSky | 8.5 | 385 | Needs formal numeric re-seance (8.1 score was partial; `sky_subWave` D004 was a false positive — parameter is fully dispatched); preset migration required (#1081) |
| Orbweave | 8.4 | 451 | Just below 8.5; Knot Phase Coupling (B021) unique |
| Organism | 8.1 / 7.2 | 433 | CA fix applied; re-seance to confirm |
| Orphica | ~8.7 est. | 264 | Buffer extended; velocity→resonance wired; needs formal numeric seance on record |
| Overlap | 8.4 | 577 | Just below 8.5; adapter-based |

---

## FX Engine Slots (6–8)

The companion FX engines (fXOnslaught, fXObscura, fXOratory, plus designed variants fXAdversary, fXResonance, fXGravity, fXGrowth, fXReduction, fXMigration) fill the FX slots. The singularity FX engines already in `Source/DSP/Effects/` are the primary candidates:

| FX Engine | File | Status |
|-----------|------|--------|
| fXOnslaught | `Source/DSP/Effects/fXOnslaught.h` | Built |
| fXObscura | `Source/DSP/Effects/fXObscura.h` | Built |
| fXOratory | `Source/DSP/Effects/fXOratory.h` | Built |
| AquaticFXSuite | `Source/DSP/Effects/AquaticFXSuite.h` | Built |
| MathFXChain | `Source/DSP/Effects/MathFXChain.h` | Built |
| BoutiqueFXChain | `Source/DSP/Effects/BoutiqueFXChain.h` | Built |

Final FX slot selection will be made when synthesis engine list is confirmed.

---

## Not V1 (Current Cycle)

Engines with confirmed scores below 8.5, unresolved P0 DSP blockers, or insufficient presets are deferred. They remain active in the personal build and will be promoted in a future update. See `Docs/fleet-health-dashboard-2026-03-24.md` for the full list and path-to-V1 for each.

---

## Exit Criteria (What "V1 Ready" Means)

Before any public distribution, the following must all be true:

- [ ] Every V1 synthesis engine has seance score ≥ 8.5 (numeric, on record)
- [ ] Every V1 engine has ≥ 100 factory presets (Oto and Octave need expansion)
- [ ] All 6 Doctrines resolved fleet-wide for V1 engines
- [ ] No unresolved P0 DSP bugs in any V1 engine
- [ ] Kitchen Collection preset expansion complete (all KC engines ≥ 100 presets)
- [ ] BROTH coordinator written in `XOceanusProcessor.cpp` (if any BROTH engine is in V1)
- [ ] Build PASS + auval PASS
- [ ] `Docs/seance_cross_reference.md` complete for all V1 engines
- [ ] `Docs/MANIFEST.md` reflects all V1 documentation

---

## Revision History

| Date | Change |
|------|--------|
| 2026-03-23 | Initial scope revision — derived from fleet-health-dashboard and v1-launch-plan-2026-03-20; no hard deadline established |
