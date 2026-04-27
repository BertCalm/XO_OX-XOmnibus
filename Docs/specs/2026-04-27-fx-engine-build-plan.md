# FX Engine Build Plan — Gap Closure + Wildcard Infusion

**Date:** 2026-04-27
**Author:** XO_OX Designs
**Status:** Design Spec — Ready for Phase 0 implementation
**Branch:** `claude/fx-engine-gap-analysis-rDtCJ`

---

## Table of Contents

1. [Motivation](#1-motivation)
2. [Strategy — Three Wildcard Primitives](#2-strategy--three-wildcard-primitives)
3. [Phase 0 — Wildcard Infrastructure](#3-phase-0--wildcard-infrastructure)
4. [Phase 1 — Wave 2 Validation](#4-phase-1--wave-2-validation)
5. [Phase 2 — Nine New FX Packs (with Wave 2 Retrofits)](#5-phase-2--nine-new-fx-packs-with-wave-2-retrofits)
6. [Phase 3 — Meta-Wildcards](#6-phase-3--meta-wildcards)
7. [Per-Pack Integration Checklist](#7-per-pack-integration-checklist)
8. [Timeline](#8-timeline)
9. [Open Questions](#9-open-questions)

---

## 1. Motivation

A thorough audit (2026-04-27) of `Source/DSP/Effects/`, `Source/Core/EpicChainSlotController.h`, and `Docs/engines.json` produced two findings:

- **Backlog of headers without seance:** 20 Wave 2 Epic Chains have implemented headers but `engines.json` status remains `designed`. Validation, not new code, is the cheapest path to shipped FX.
- **Category gaps with strong market legibility:** nine categories are underrepresented or missing — analog warmth (tape/spring/tube), classical reverb (plate/hall/IR), vintage modulation (flanger/univibe/rotary), multiband creative FX, sidechain creative processing, mastering pipeline, transparent pitch/time-stretch, lo-fi physical artifacts (vinyl/cassette), and binaural/HRTF spatial.

A naïve plan would ship nine new packs in isolation. That risks one-off cleverness per chain and a two-tier library (new chains "feel XO_OX," older chains do not). This spec instead extracts the XO_OX flavor into **three reusable primitives** built once and infused across every chain — new and existing.

---

## 2. Strategy — Three Wildcard Primitives

| # | Primitive | What it does | Doctrines exercised |
|---|---|---|---|
| 1 | **Coupling-driven parameters** | Replace internal LFO/rate knobs with matrix-routable targets driven by partner engines via `MegaCouplingMatrix` (incl. `TriangularCoupling`, `KnotTopology`) | D002, D006 |
| 2 | **DNA-as-modulator** | Promote the 6D Sonic DNA (brightness, warmth, movement, density, space, aggression) from preset metadata to live continuous mod source. M1 CHARACTER macro warps DNA in real time | D001, D002 |
| 3 | **Doctrine-extension** | Apply D001 (velocity → timbre) and D005 (must breathe, ≤0.01 Hz LFO) to new axes — time, age, geometry, space | D001, D005 |

Plus two meta-wildcards held in reserve for Phase 3:

- **Mood-as-modulator** — promote the 16 mood tags from browse metadata to a continuous warp dimension every chain can opt into
- **Knot-topology slot routing** — `EpicChainSlotController` self-organizes into parallel braids based on coupling depth

---

## 3. Phase 0 — Wildcard Infrastructure

**Goal:** ship the three primitives as reusable shared infrastructure before any new FX consumes them. Estimated 4–6 weeks.

### 3.1 New components

| Component | Path | Responsibility |
|---|---|---|
| `DNAModulationBus` | `Source/Core/DNAModulationBus.h` (new) | Exposes 6 normalized DNA scalars as mod sources; subscribes to M1 CHARACTER macro for real-time warp; published into `MegaCouplingMatrix` as routable sources |
| `MoodModulationBus` | `Source/Core/MoodModulationBus.h` (new) | Provides 16-mood one-hot + soft-blend continuous output; defaults to current preset's mood tag; opt-in target on every chain |
| `AgeCouplingTarget` | `Source/DSP/Effects/Aging.h` (new) | Standard 0–1 AGE scalar convention for chains modeling material wear; *never* auto-driven by wall-clock; coupled to time, partner LFO, or partner MOVEMENT macro by user choice |

### 3.2 Real-time safety

All three components are scalar/coefficient-update only. No allocation on the audio thread. Filter coefficient updates use parameter smoothing (`juce::SmoothedValue` or equivalent linear interpolation already used in `LushReverb.h`).

### 3.3 Doctrine compliance

- **D004 (no dead params):** every primitive must drive audible output in at least one consuming chain at ship time
- **D005 (must breathe):** `DNAModulationBus` and `MoodModulationBus` both expose ≤0.01 Hz drift LFO modes
- **D001 (velocity → timbre):** velocity is a pre-routed input to `DNAModulationBus` (modifies aggression/brightness)

### 3.4 First consumer

Pack 1 (Sidechain Creative — Triangular Coupling Pump) is the proof-of-life consumer. Phase 0 and Pack 1 ship together; building them in parallel surfaces API gaps before they harden.

---

## 4. Phase 1 — Wave 2 Validation

**Goal:** push 20 Wave 2 Epic Chains from `designed` → `implemented` in `engines.json`. Estimated 2–4 weeks. **Pure validation — no retrofit.** Retrofits ride along with Phase 2 packs (see §5).

### 4.1 Sequence

1. Run `/master-audit` across all 20 to surface bulk failures
2. For each chain: `/validate-engine` → `/synth-seance` → fix doctrine violations → flip status
3. Update `Docs/seances/seance_cross_reference.md` and `Docs/reference/engine-color-table.md`

### 4.2 Why no retrofit here

- Phase 0 buses don't exist yet
- Parameter prefixes are frozen; retrofit must be additive only — safe to defer
- Seance will *organically* force retrofits where chains fail D002/D005; that natural overlap is sufficient. Don't expand scope.

### 4.3 The 20 Wave 2 chains

| Engine | Prefix | Chain Class | Concept |
|---|---|---|---|
| Oubliette | `oubl_` | OublietteChain | Memory Slicer |
| Osmium | `osmi_` | OsmiumChain | Sub-Harmonic Collapse |
| Orogen | `orog_` | OrogenChain | Ringing Abyss |
| Oculus | `ocul_` | OculusChain | Sentient Grid |
| Outage | `outg_` | OutageChain | Lo-Fi Cinema |
| Override | `ovrd_` | OverrideChain | Digital Aggression |
| Occlusion | `occl_` | OcclusionChain | Spatiotemporal Collapse |
| Obdurate | `obdr_` | ObdurateChain | Oscillating Drone Wall |
| Orison | `oris_` | OrisonChain | Shattered Cathedral |
| Overshoot | `ovsh_` | OvershootChain | Error Cascade |
| Obverse | `obvr_` | ObverseChain | Reverse Gravity |
| Oxymoron | `oxym_` | OxymoronChain | Gated Choir |
| Ornate | `orna_` | OrnateChain | Granular Exciter |
| Oration | `orat_` | OrationChain | Post-Delay Auto-Wah |
| Offcut | `offc_` | OffcutChain | Crushed Error |
| Omen | `omen_` | OmenChain | Reverb-Driven PLL |
| Opus | `opus_` | OpusChain | Tomorrow's Microcosm |
| Outlaw | `outl_` | OutlawChain | Cybernetic Child |
| Outbreak | `outb_` | OutbreakChain | Glitch Contagion |
| Orrery | `orry_` | OrreryChain | Frozen Diamond |

---

## 5. Phase 2 — Nine New FX Packs (with Wave 2 Retrofits)

Each pack ships **3 new chains** plus selective additive retrofits to Wave 2 chains that share the same primitive. By end of Phase 2, every Wave 2 chain has been touched by at least one primitive — no two-tier outcome.

**Retrofit rule:** strictly additive. Existing param IDs are frozen. Retrofit adds new coupling targets, mod sources, and doctrine knobs only.

### Pack 1 — Sidechain Creative *(ships with Phase 0)*

| Chain | Concept | Primitive | Wildcard |
|---|---|---|---|
| TriPump | 3-engine ducking via `TriangularCoupling` | Coupling-driven | **The matrix demo** — phase-staggered loop impossible elsewhere |
| SpectralGate | Frequency-selective sidechain gate | Coupling-driven | Sidechain key driven by partner spectrum |
| FreqDuck | Spectral-band ducking | Coupling-driven | Per-band release time mapped to partner DNA |

**Retrofits:** Outbreak, Overshoot, Obverse — add coupling-driven cascade rate / mirror depth / contagion spread.

### Pack 2 — Analog Warmth

| Chain | Concept | Primitive | Wildcard |
|---|---|---|---|
| TapeEcho | BBD + tape saturation + wow/flutter | Doctrine-ext (AGE) | Patch-Age Accumulator (user-scrubbable, coupling-target) |
| SpringReverb | 3-spring tank model | Doctrine-ext (AGE) | Springs sag with humidity from partner MOVEMENT macro |
| TubeDrive | Triode/pentode bias model | Doctrine-ext (AGE) | Tube bias drifts with AGE coupling input |

**Retrofits:** Outage, Offcut — add AGE coupling target to lo-fi degradation.

### Pack 3 — Vintage Modulation

| Chain | Concept | Primitive | Wildcard |
|---|---|---|---|
| Flanger | Through-zero flanger w/ Alnico mag drive | Coupling-driven | Tethered-Animal Motor — internal LFO floor at ≤0.01 Hz, partner transients trigger ramps |
| Univibe | Photocell + 4-stage phase shifter | Coupling-driven | Same dual-state pattern |
| Rotary | Leslie 122 with horn + drum + foot ramp | Coupling-driven | Slow/fast switch driven by partner transient density |

**Retrofits:** Oration, Oxymoron, Outlaw — coupling-driven LFO/gate timing.

### Pack 4 — Reverb Classics

| Chain | Concept | Primitive | Wildcard |
|---|---|---|---|
| Plate | EMT 140 plate model | Doctrine-ext (D005 → space) | **Breathing Room** — modal frequencies become coupling outputs partners can tune to |
| Hall | Concert hall w/ early reflection bus | Doctrine-ext | Geometry warps via partner pitch |
| IRConvolver | User IR + library IRs | Doctrine-ext | IR length breathed via ≤0.01 Hz LFO |

**Retrofits:** Orison, Orogen, Occlusion, Orrery — modal frequencies expose as coupling sources.

### Pack 5 — Multiband Creative

| Chain | Concept | Primitive | Wildcard |
|---|---|---|---|
| MBSaturator | 3-band Linkwitz-Riley + per-band saturation | DNA-as-mod | **DNA-Aware Band Split** — crossover Hz follows brightness/density |
| MBReverb | Per-band reverb with independent decay | DNA-as-mod | Decay per band scales with space DNA |
| MBDelay | Per-band delay with independent feedback | DNA-as-mod | Feedback per band scales with movement DNA |

**Retrofits:** Obdurate, Ornate — DNA-aware drone tuning / grain selection.

### Pack 6 — Lo-Fi Physical

| Chain | Concept | Primitive | Wildcard |
|---|---|---|---|
| Vinyl | RIAA + crackle + wow | Doctrine-ext (AGE) + Coupling | **Generational Coupling Decay** — each pass between coupled engines compounds medium loss |
| Cassette | Type II tape + dropout + azimuth drift | Doctrine-ext + Coupling | Same generational mechanism |
| Shortwave | AM-band filter + atmospheric noise | Doctrine-ext + Coupling | Atmosphere driven by partner MOVEMENT |

**Retrofits:** Outage, Offcut (second pass) — generational decay coupling.

### Pack 7 — Pitch / Time

| Chain | Concept | Primitive | Wildcard |
|---|---|---|---|
| PitchProcessor | PSOLA + formant preserve | DNA-as-mod + Doctrine-ext | **Velocity Stretches Time** — D001 applied to grain length and retrigger density |
| TimeStretcher | Phase-vocoder stretch | Doctrine-ext | Stretch ratio coupled to partner velocity |
| FormantShifter | Independent formant control | DNA-as-mod | Formant offset scaled by warmth DNA |

**Retrofits:** Oubliette, Osmium — velocity-driven slice/sub-harmonic timing.

### Pack 8 — Mastering Pipeline

| Chain | Concept | Primitive | Wildcard |
|---|---|---|---|
| TransparentComp | Linear-phase mastering compressor | Mood-as-mod | Threshold/ratio shaped by mood tag |
| LinearPhaseEQ | 8-band linear-phase EQ | Mood-as-mod | Band gains shaped by mood (Shadow darkens, Luminous lifts) |
| LoudnessMaximizer | LUFS-aware true-peak limiter | Mood-as-mod | Target LUFS shaped by mood |

**Retrofits:** Opus, Omen — mood-aware microcosm/PLL synthesis.

### Pack 9 — Aquatic Spatial

| Chain | Concept | Primitive | Wildcard |
|---|---|---|---|
| BinauralField | HRTF-based 3D placement | Doctrine-ext (D003 → space) | **Aquatic HRTF** — diffraction & absorption modeled for water; pressure-depth parameter |
| BathyscapeSpatializer | Pressure-depth spatial field | Doctrine-ext | Depth modulates HF absorption per real underwater physics |
| DepthDoppler | Depth + velocity Doppler | Doctrine-ext | Underwater sound speed (1500 m/s) used for delay |

**Retrofits:** Oculus — pressure-aware grid spacing.

### Phase 2 totals

- **27 new chains**
- **20 Wave 2 chains retrofitted** (every one touched at least once)
- Combined library: **77 assignable chains** (30 existing + 27 new + 20 retrofitted)

---

## 6. Phase 3 — Meta-Wildcards

Held until Phase 2 ships. Both touch `EpicChainSlotController` and risk regressions; defer.

### 6.1 Knot-Topology Slot Routing

`EpicChainSlotController.h` learns to braid the 3 slots in parallel based on coupling depth between assigned chains. Uses `KnotTopology` primitive from `MegaCouplingMatrix`. Estimated 2 week architectural lift; needs full regression suite.

### 6.2 Mood-as-Modulator Fleet Promotion

Promote `MoodModulationBus` from Phase 0 prototype + Pack 8 consumer to a fleet-wide modulation source available to every chain. Most chains stay opt-out by default to preserve preset determinism.

---

## 7. Per-Pack Integration Checklist

Run for every new chain (per CLAUDE.md "Adding New Engines"):

### 7.1 Code

1. `Source/DSP/Effects/{ChainName}Chain.h` — DSP inline
2. `Source/Engines/{ChainName}/{ChainName}Engine.h` — `SynthEngine` adapter (if exposed as engine)
3. `Source/XOceanusProcessor.cpp` — `REGISTER_ENGINE`
4. `Source/Core/EpicChainSlotController.h` — chain enum + factory entry
5. `Source/Core/PresetManager.h` — `validEngineNames` + `frozenPrefixForEngine`

### 7.2 Content

1. `Docs/engines.json` (status: `implemented`) — single source of truth, then `python Tools/sync_engine_sources.py`
2. `CLAUDE.md` — engine count header + parameter prefix table row
3. `Docs/reference/engine-color-table.md` — color row
4. `Docs/specs/xoceanus_master_specification.md` §3.1 engine table — add row
5. `Docs/seances/seance_cross_reference.md` — add row after seance
6. ≥5 demo presets per chain across relevant moods

### 7.3 Quality gates

1. `/validate-engine` — D001–D006 pass
2. `/synth-seance` — formal review
3. ParamSnapshot pattern — cache pointers once per block
4. No audio-thread allocation, no blocking I/O
5. Mono → stereo expansion if not already stereo
6. Denormal protection in feedback/filter paths

### 7.4 Retrofit checklist (Wave 2 chains)

1. Strictly additive params only; never rename or remove
2. Add coupling target to chain's existing wildcard concept
3. Re-seance after retrofit
4. Update `engines.json` notes field

---

## 8. Timeline

| Quarter | Phase | Output |
|---|---|---|
| Q2 2026 | Phase 0 + Phase 1 + Pack 1 | 3 primitives shipped, 20 Wave 2 chains validated, 3 new chains shipped (Sidechain Creative), 3 Wave 2 retrofits |
| Q3 2026 | Packs 2–4 | 9 new chains (Analog Warmth, Vintage Mod, Reverb Classics), 11 retrofits, "complete instrument" market position |
| Q4 2026 | Packs 5–7 | 9 new chains (Multiband, Lo-Fi, Pitch/Time), all primitives applied to multiple categories |
| Q1 2027 | Packs 8–9 + Phase 3 | 6 new chains (Mastering, Aquatic Spatial), Knot-Topology routing, Mood-as-mod fleet promotion |

Per CLAUDE.md "Release Philosophy — The Deep Opens": no fixed cutoff. Quarters are sequencing, not deadlines. Ship when ready.

---

## 9. Open Questions

1. **DNA real-time mutability scope** — does M1 CHARACTER macro warp DNA in-place per voice, or only at preset-load? Per-voice is more powerful but raises CPU concerns. *Decision needed before Phase 0 finalizes.*
2. **Retrofit gating** — should each retrofit ship with re-seance (slow, thorough) or with a lighter regression check (fast, risk of D004 regressions)? *Recommend: re-seance for any chain that gains a new coupling primitive; regression-only for additive doctrine knobs.*
3. **Mastering mood-coupling default** — does the master bus default to "mood-aware" or "mood-neutral"? Mood-aware is the wildcard; mood-neutral preserves existing preset behavior. *Recommend: opt-in via single MASTER_MOOD_AWARE toggle, default off, surfaced in mastering pack tutorial.*
4. **Knot-Topology routing back-compat** — when Phase 3 lands, do existing presets get auto-braided? *Recommend: explicit per-preset opt-in, default serial.*
5. **AGE coupling default** — what does AGE default to when no coupling input is wired? *Recommend: 0 (factory-fresh) for predictability; users explicitly couple to time/LFO/partner for the wildcard behavior.*

---

## Cross-References

- Audit findings: this branch's `claude/fx-engine-gap-analysis-rDtCJ` chat history
- Existing FX architecture: `Docs/specs/2026-04-05-epic-chains-fx-design.md`
- Master FX brief: `Docs/specs/xoceanus_master_fx_design_brief.md`
- Engine source of truth: `Docs/engines.json`
- Doctrines: `CLAUDE.md` § The 6 Doctrines
- Coupling matrix: `Source/Core/MegaCouplingMatrix.h`
- FX slot controller: `Source/Core/EpicChainSlotController.h`
