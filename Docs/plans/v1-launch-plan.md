# XOmnibus V1 Launch Plan

*Drafted: 2026-03-17 | Updated: 2026-03-17 (RAC review + owner ratification)*
*Goal: Ship a clean, confident, no-apologies V1*

---

## Philosophy

V1 ships what's **done and excellent** — not everything that's been imagined. Every engine that ships must pass all 6 Doctrines, have compelling presets, and be free of P0 audio bugs. Anything that isn't ready gets a clear post-V1 label and stops consuming V1 attention.

---

## Owner Decisions (Ratified 2026-03-17)

1. **Scope:** 4 concept engines (OSTINATO, OPENSKY, OCEANDEEP, OUIE) deferred to post-V1 release cadence. Supersedes 2026-03-15 "all V1" decision.
2. **Release model:** Tiered — V1.0 → V1.1 → V1.2 (see Execution Order).
3. **License:** MIT (committed to repo).
4. **Post-V1 cadence:** Bi-monthly free engine releases. Premium boutique engines (voice + custom FX in 5th slot) as paid products, potentially fundraiser-connected. OSTINATO is top premium candidate.
5. **OddfeliX/OddOscar:** Character names stay primary in UI. "Transient Designer" / "Morph Pad" as secondary functional descriptors in documentation only.

---

## PHASE 0 — Fix What's Broken (P0 Audio Bugs) ✅ COMPLETE

All 5 issues resolved on `v1-launch-prep` branch:

| # | Bug | Status |
|---|-----|--------|
| 1 | OBSIDIAN right channel filter bypass | ✅ Fixed (both channels filtered) |
| 2 | OSTERIA warmth filter left-only | ✅ Fixed (mixR now passes through warmth) |
| 3 | ORIGAMI STFT race condition | ✅ Fixed (activeVoiceCount atomic; hop counter audio-thread-only) |
| 4 | 5 dead parameters (D004) | ✅ All wired to DSP |
| 5 | Test target link error | ✅ juce_audio_formats linked |

**Gate:** ✅ All fixed. Build + auval verification pending.

---

## PHASE 1 — SRO Optimization (CPU Credibility)

A 34-engine synth that burns CPU while silent is not shippable.

| # | Task | Impact | Effort |
|---|------|--------|--------|
| 1 | **SilenceGate integration** — all 34 engines | Zero CPU when idle | M (templated, repetitive) |
| 2 | **FastMath fleet adoption** — replace per-sample std::sin/cos/tan | 30-50% DSP reduction in hot paths | M |
| 3 | **ControlRateReducer for coupling** — coupling at control rate, not audio rate | Major CPU savings in multi-engine patches | M |
| 4 | **SROAuditor dashboard** — real-time CPU budget visibility | User confidence; "pro-grade" signal | S |

**Governance notes (Architect):**
- SilenceGate must not break D001 (velocity at slow attack) — validate all 34 engines post-integration
- ControlRateReducer must maintain minimum CC update rate ≥ ~5ms for D006 compliance
- Coupling routes may need exempt lanes from ControlRateReducer to prevent zipper artifacts
- D005 drift floor must be re-validated after FastMath substitution

**Gate:** 4-engine patch idles at <2% CPU. Single-engine worst case <15%.

---

## PHASE 2 — Engine Roster Lock

### 34 engines SHIP AS-IS

All 34 registered engines are unique synthesis paradigms with no redundancy at the DSP level. Nothing gets cut from V1.

OddfeliX and OddOscar retain their character identities (feliX the neon tetra, Oscar the axolotl) as primary UI names. Functional descriptors ("transient design," "morph") appear as secondary labels in documentation and guides only — never replacing character names in the gallery.

### 4 concept engines — POST-V1 RELEASE CADENCE

OSTINATO, OPENSKY, OCEANDEEP, OUIE have zero DSP. They enter the bi-monthly free release cadence post-V1, with OSTINATO additionally being the top candidate for the first premium boutique engine (voice + custom FX in 5th slot).

**V2 narrative:** Announce all 4 at V1 launch as "coming next" — converts a scope cut into a roadmap moment.

---

## PHASE 3 — FX for V1 (V1.1 milestone)

### Ship: Aquatic FX Suite (The Aquarium)

The **6-stage Aquatic FX** (Fathom/Drift/Tide/Reef/Surface/Biolume) is the right V1 FX play:

- Design is COMPLETE — 22 parameters, signal flow defined, `aqua_` prefix assigned
- All algorithmic DSP (no convolution) — moderate build effort
- Brand-defining: "that sounds like XO_OX" regardless of active engine
- **Architecture decision needed:** Does Aquatic FX Suite replace or extend MasterFXChain? (Architect P1 flag)
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

### Defer: fXO_ Regional Effects + Prime Movers + Chord Machine → V2

**Note:** fXOsmosis and fXOneiric (boutique effects added on SRO branch) need a decision — either they ship as V1 content (requiring seance + mood categorization) or they defer with the rest of the fXO_ category.

---

## PHASE 4 — Preset Expansion (V1.2 milestone)

### Current state: 11,247 factory presets, 100% DNA coverage, 0 duplicates

With 11,247 presets, the 120/engine minimum target needs recalculation. Run per-engine count to identify actual gaps.

| Task | Details |
|------|---------|
| **Per-engine audit** | Count presets per engine, identify any below 120 |
| **Aquatic FX presets** | 20-30 presets showcasing the new FX suite across multiple engines |
| **Coupling showcase refresh** | Verify existing Entangled presets still sound right after SRO changes |
| **Init patch audit** | Every engine's init patch must sound compelling dry (DB003) |

**Gate:** Every engine ≥120 presets. Aquatic FX has dedicated showcase presets.

---

## PHASE 5 — Documentation & Ship Prep (Parallel with Phases 1-4)

| Task | Details | Effort | Status |
|------|---------|--------|--------|
| **MIT License** | Committed to repo | S | ✅ DONE |
| **README.md** | User-facing: what is XOmnibus, how to build, how to contribute | M | In progress |
| **CONTRIBUTING.md** | PR standards, code style, fnm requirement | M | In progress |
| **SECURITY.md** | Vulnerability reporting process | S | In progress |
| **CODE_OF_CONDUCT.md** | Contributor Covenant | S | In progress |
| **GitHub issue templates** | Bug report + feature request | S | In progress |
| **Master spec rewrite** | Accurate 34-engine table | M | Pending |
| **Sound design guide completion** | 4 Constellation engines missing from unified guide | M | Pending |
| **CLAUDE.md accuracy pass** | Preset count corrected (11,247). Verify all other counts. | S | In progress |
| **Security audit** | Git history for secrets, firebase-debug.log, no .env | M | Pending |
| **11 missing engine identity cards** | Aquatic mythology gap | M | Pending |

---

## What V1 Does NOT Ship

| Item | Why Deferred | When |
|------|-------------|------|
| OSTINATO | Premium boutique candidate (voice + FX 5th slot) | Post-V1 bi-monthly cadence |
| OPENSKY, OCEANDEEP, OUIE | Zero DSP — design-only | Post-V1 bi-monthly cadence |
| fXO_ regional effects | Need shared cores infrastructure | V2 |
| Prime Movers | Phase 2 architecture | V2 |
| Chord Machine | Feature, not fix | V2 |
| Synesthesia Engine | PARKED (P001) | Needs planning session |
| Theorem engines (OVERTONE, KNOT, ORGANISM) | Pi Day concepts | V2+ |
| Kitchen Essentials / Collections | Paid expansion platform | V2 ($39-49) |
| Dynamic oversampling | Phase 2 SRO | V2 |

---

## V1 Launch Manifest

```
ENGINES:           34 (all registered, all unique paradigms)
PRESETS:           11,247 factory (target: every engine ≥120)
MOODS:             8 (Foundation, Atmosphere, Entangled, Prism, Flux, Aether, Family, Submerged)
MACROS:            4 (CHARACTER, MOVEMENT, COUPLING, SPACE)
COUPLING TYPES:    12 (MegaCouplingMatrix)
MAX SLOTS:         4 simultaneous engines
FX:                Aquatic FX Suite (6 stages) + MasterFXChain (18 stages)
FORMATS:           AU + Standalone (macOS), AUv3 + Standalone (iOS), VST3
DOCTRINES:         6/6 resolved fleet-wide
LICENSE:           MIT
```

---

## Execution Order — Tiered Release

```
WEEKS 1-2:  PHASE 0 ✅ (already done)
            PHASE 1 — SilenceGate + FastMath fleet rollout
            PHASE 5 — Community infrastructure (README, CONTRIBUTING, etc.)
            PHASE 5 — Security audit (git history, no secrets)
            Site: Record audio demos, fix Patreon URL, build Download page

WEEKS 3-4:  PHASE 1 — ControlRateReducer + SROAuditor
            PHASE 5 — Documentation pass (master spec, sound design guides)
            Site: Index refresh, engine gallery, Getting Started page

WEEK 5:    Integration testing, hardware MPC validation, auval
            Launch-ready gate review

WEEK 6:    ═══ V1.0 LAUNCH ═══
            Repo public, download live, TIDE TABLES live
            Community seeding (Reddit, KVR, Discord, forums)

WEEKS 7-8:  Post-launch monitoring + hotfix protocol
            MACHINE GUN REEF push
            Community Challenge #1

WEEKS 9-10: PHASE 3 — Aquatic FX Suite build + integration
            ═══ V1.1 UPDATE ═══

WEEKS 11-14: PHASE 4 — Preset expansion (if needed per audit)
             Content cadence (Field Guide, Signal)
             ═══ V1.2 UPDATE ═══ (content complete)
```

**Total: ~6 weeks to V1.0 launch. ~14 weeks to content-complete V1.2.**

---

## Hard Gates (from Board of Directors)

Nothing ships without these cleared:

1. **Security** — Git history clean, no secrets, npm audit clean
2. **License** — ✅ MIT committed
3. **Community infrastructure** — README, CONTRIBUTING, SECURITY, CODE_OF_CONDUCT, issue templates
4. **Audio demos** — Minimum 5 hero preset recordings on XO-OX.org
5. **Preset silence audit** — All presets verified non-silent
6. **Engine count accuracy** — All materials say "34 engines" (accurate — all registered with DSP)
7. **Patreon URL** — Fixed from placeholder across all pages

---

*This plan ships 34 engines, 11,247+ presets, branded Aquatic FX, zero P0 bugs, SRO-optimized performance, and MIT-licensed open source. Premium boutique engines and bi-monthly free releases sustain momentum post-launch. Everything else is V2 — designed, documented, and waiting.*
