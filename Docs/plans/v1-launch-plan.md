# XOmnibus V1 Launch Plan

*Drafted: 2026-03-17*
*Goal: Ship a clean, confident, no-apologies V1*

---

## Philosophy

V1 ships what's **done and excellent** — not everything that's been imagined. Every engine that ships must pass all 6 Doctrines, have compelling presets, and be free of P0 audio bugs. Anything that isn't ready gets a clear "V2" label and stops consuming V1 attention.

---

## PHASE 0 — Fix What's Broken (P0 Audio Bugs)

These are showstoppers. Fix before anything else.

| # | Bug | File | Fix | Effort |
|---|-----|------|-----|--------|
| 1 | **OBSIDIAN right channel filter bypass** | `ObsidianEngine.h:801-805` | Apply `mainFilter.processSample()` to both channels | 1 line |
| 2 | **OSTERIA warmth filter left-only** | `OsteriaEngine.h:1227` | Duplicate low-shelf for mixR | 1 line |
| 3 | **ORIGAMI STFT race condition** | `OrigamiEngine.h:758-770` | Serialize hop-counter boundary with buffer writes | S |
| 4 | **5 dead parameters (D004)** | SNAP macroDepth, OWLFISH morphGlide, OBLIQUE percDecay, OCELOT ecosystem macros, OSPREY LFO | Wire to DSP or remove | S each |
| 5 | **Test target link error** | CMakeLists.txt | Link `juce_audio_formats` to test target | 1 line |

**Gate:** All 5 fixed, build PASS, auval PASS, tests PASS.

---

## PHASE 1 — SRO Optimization (CPU Credibility)

A 34-engine synth that burns CPU while silent is not shippable.

| # | Task | Impact | Effort |
|---|------|--------|--------|
| 1 | **SilenceGate integration** — all 34 engines | Zero CPU when idle | M (templated, repetitive) |
| 2 | **FastMath fleet adoption** — replace per-sample std::sin/cos/tan | 30-50% DSP reduction in hot paths | M |
| 3 | **ControlRateReducer for coupling** — coupling at control rate, not audio rate | Major CPU savings in multi-engine patches | M |
| 4 | **SROAuditor dashboard** — real-time CPU budget visibility | User confidence; "pro-grade" signal | S |

**Gate:** 4-engine patch idles at <2% CPU. Single-engine worst case <15%.

---

## PHASE 2 — Engine Roster Decision

### 34 engines SHIP AS-IS

All 34 registered engines are unique synthesis paradigms with no redundancy at the DSP level. Nothing gets cut from V1.

**However, two engines get reframed in marketing/UI (no code changes):**

| Engine | Current Framing | V1 Framing | Why |
|--------|----------------|------------|-----|
| ODDFELIX | Standalone engine | "Transient Designer" — positioned as excitation source | KS transients pair best as input to other engines |
| ODDOSCAR | Standalone engine | "Morph Pad" — positioned as the PPG morphing specialist | Clarifies identity vs OBLONG's warm analog character |

Both keep their DSP, presets, and slot position. This is **labeling**, not surgery.

### 4 concept engines — DEFER TO V2

OSTINATO, OPENSKY, OCEANDEEP, OUIE have zero DSP. Building 4 new engines while shipping V1 is a scope trap. They're designed and waiting — that's a V2 strength, not a V1 gap.

**Exception:** If you have capacity after Phase 4 and want to ship ONE concept engine, **OPENSKY** is the easiest build (supersaw + shimmer + reverb — well-understood DSP) and completes the brand's "sky" mythology.

---

## PHASE 3 — FX for V1

### Ship: Aquatic FX Suite (The Aquarium)

The **6-stage Aquatic FX** (Fathom/Drift/Tide/Reef/Surface/Biolume) is the right V1 FX play:

- Design is COMPLETE — 22 parameters, signal flow defined, `aqua_` prefix assigned
- All algorithmic DSP (no convolution) — moderate build effort
- Brand-defining: "that sounds like XO_OX" regardless of active engine
- Augments/replaces MasterFXChain (which has 18 stages already working)
- Maps directly to the 4 macros (CHARACTER→Fathom+Biolume, SPACE→Reef+Drift, MOVEMENT→Tide+Drift, COUPLING→Surface)

| FX | DSP Core | Effort |
|----|----------|--------|
| Fathom (pressure) | 3-band compressor + shelving | S |
| Drift (currents) | Brownian-walk chorus | S |
| Tide (rhythm) | Tremolo/auto-filter + sync LFO | S |
| Reef (space) | Householder FDN early reflections | M |
| Surface (boundary) | Cytomic SVF high-shelf sweep | S |
| Biolume (shimmer) | Half-wave rectifier + HP + saturation | S |

**Total effort:** ~1 focused week for all 6 stages.

### Defer: fXO_ Regional Effects + Prime Movers

These need the 8 shared effect cores built first. That's infrastructure work. V2.

### Defer: Chord Machine

Brilliant concept but it's a feature, not a fix. V2.

---

## PHASE 4 — Preset Expansion (Target: 120 per engine, including Entangled)

### Current state: ~2,550 factory presets, 100% DNA coverage, 0 duplicates

12 engines below 120 presets. ~423 new presets needed (Entangled coupling presets count toward both participating engines).

| Engine | Current | Need | Priority Coupling Partners |
|--------|---------|------|---------------------------|
| **Owlfish** | 35 | +85 | Overdub, Obscura, Opal |
| **Overlap** | 51 | +69 | Orca, Octopus, Ouroboros |
| **Outwit** | 61 | +59 | Onset, Optic, Oblique |
| **Oceanic** | 79 | +41 | Osprey, Osteria, Organon |
| **Orca** | 89 | +31 | Ombre, Octopus, OceanDeep (V2) |
| **Osteria** | 90 | +30 | Osprey, Ole, Ottoni |
| **Osprey** | 91 | +29 | Osteria, Oceanic, Ohm |
| **Origami** | 96 | +24 | Oracle, Obsidian, Opal |
| **Ombre** | 98 | +22 | Orca, Odyssey, Orphica |
| **Octopus** | 100 | +20 | Orca, Overlap, Ouroboros |
| **Ocelot** | 109 | +11 | Opal, Overdub, Overworld |
| **Oracle** | 118 | +2 | Organon, Obscura |

| Task | Details |
|------|---------|
| **12-engine preset expansion** | All 12 engines above to ≥120 presets (solo + Entangled) |
| **Aquatic FX presets** | 20-30 presets showcasing the new FX suite across multiple engines |
| **Coupling showcase refresh** | Verify all existing Entangled presets still sound right after SRO changes |
| **Init patch audit** | Every engine's init patch must sound compelling dry (DB003) |

**Gate:** Every engine ≥120 presets. Aquatic FX has dedicated showcase presets.

---

## PHASE 5 — Documentation & Ship Prep

| Task | Details | Effort |
|------|---------|--------|
| **Master spec rewrite** | "The Seven Engines" table → accurate 34-engine table | M |
| **Sound design guide completion** | 4 Constellation engines missing from unified guide | M |
| **CLAUDE.md accuracy pass** | Verify all counts, engine lists, build instructions | S |
| **README for open-source launch** | User-facing: what is XOmnibus, how to build, how to contribute | M |
| **License file** | Confirm open-source license choice | S |

---

## What V1 Does NOT Ship

| Item | Why Deferred |
|------|-------------|
| OSTINATO, OPENSKY, OCEANDEEP, OUIE | Zero DSP — design-only. V2 build queue. |
| fXO_ regional effects | Need shared cores infrastructure. V2. |
| Prime Movers (Origin/Overture/Oscillograph/Ouverture) | Phase 2 architecture. V2. |
| Chord Machine | Feature, not fix. V2. |
| Synesthesia Engine | PARKED (P001). Needs dedicated planning session. |
| XOscillum, XObliqua, XOccult, XOblivion, XOntara | V2 build queue (concept/spec only). |
| Theorem engines (OVERTONE, KNOT, ORGANISM) | Pi Day concepts. V2+. |
| Kitchen Essentials / Collections | Long-term platform vision. V3+. |
| Dynamic oversampling (OversamplingManager) | Phase 2 SRO. V2. |
| AudioToBuffer Phase 3 | Spec'd but not blocking V1 coupling. V2. |

---

## V1 Launch Manifest

```
ENGINES:           34 (all registered, all unique paradigms)
PRESETS:           2,550+ (target ~3,000 after expansion — 120 per engine min)
MOODS:             7 (Foundation, Atmosphere, Entangled, Prism, Flux, Aether, Family)
MACROS:            4 (CHARACTER, MOVEMENT, COUPLING, SPACE)
COUPLING TYPES:    12 (MegaCouplingMatrix)
MAX SLOTS:         4 simultaneous engines
FX:                Aquatic FX Suite (6 stages) + MasterFXChain (18 stages)
FORMATS:           AU + Standalone (macOS), AUv3 + Standalone (iOS), VST3
DOCTRINES:         6/6 resolved fleet-wide
```

---

## Execution Order

```
Week 1:  PHASE 0 — P0 bug fixes (3 audio bugs + 5 dead params + test link)
Week 1:  PHASE 1 — SilenceGate + FastMath fleet rollout (parallel with bug fixes)
Week 2:  PHASE 1 — ControlRateReducer + SROAuditor
Week 2:  PHASE 3 — Aquatic FX build (6 stages)
Week 3:  PHASE 4 — Preset expansion (Owlfish/Overlap/Outwit + Aquatic FX presets)
Week 3:  PHASE 5 — Documentation pass
Week 4:  Integration testing, auval, final audit
```

**Total: ~4 weeks to clean V1.**

---

*This plan ships 34 engines, 2,700+ presets, branded Aquatic FX, zero P0 bugs, and SRO-optimized performance. Everything else is V2 — designed, documented, and waiting.*
