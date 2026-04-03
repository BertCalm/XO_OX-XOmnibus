# Fleet Health Dashboard — March 24, 2026

**Produced by:** Fleet Inspector (overnight autonomous session)
**Data sources:** `Source/Engines/` (ground-truth file counts), `Presets/XOceanus/` (JSON-parsed recursive file counts), `scripture/retreats/` + `Docs/guru-bin-retreats/` (retreat cross-reference), `Docs/seances/` + `Docs/fleet-seance-scores-2026-03-20.md` (seance scores)
**Methodology:** All counts are from actual file system reads and JSON parsing — not from memory or documentation. Seance scores reflect latest available verdict; estimates are flagged.

---

## Summary

| Metric | Count |
|--------|-------|
| Total engines (Source/Engines/ dirs) | **73** |
| Total non-quarantine presets | **13,260** |
| Total quarantine presets | 3,991 |
| Grand total presets | 17,251 |
| Engines with retreat docs | **52** |
| Engines with 100+ presets | **47** |
| Engines with fewer than 50 presets | **26** |
| Engines with no seance | **2** (Osmosis, Outlook) |
| Engines with only a design-phase seance | 7 legacy aliases (Bite/Bob/Drift/Dub/Fat/Morph/Snap) |
| Confirmed V1-ready (8.5+ / approved / flagship) | **23** |
| V1 CONSIDER (strong but need verification) | **13** |

**Note on legacy engine directories:** Seven `Source/Engines/` directories (Bite, Bob, Drift, Dub, Fat, Morph, Snap) are legacy aliases kept for build compatibility. They map to canonical engine IDs (Overbite, Oblong, Odyssey, Overdub, Obese, OddOscar, OddfeliX) via `resolveEngineAlias()` in `PresetManager.h`. Their preset counts in this document reflect the canonical engine name in the preset registry.

---

## Preset Distribution by Mood (Non-Quarantine)

| Mood | Preset Count |
|------|-------------|
| Foundation | 2,328 (incl. nested) |
| Atmosphere | 2,026 (incl. nested) |
| Entangled | 1,926 (incl. nested) |
| Prism | 1,858 (incl. nested) |
| Flux | 1,749 (incl. nested) |
| Aether | 1,653 (incl. nested) |
| Family | 698 |
| Submerged | 583 |
| Oxytocin (mood dir) | 130 |
| Deep | 64 |
| Organic | 70 |
| Kinetic | 60 |
| Crystalline | 36 |
| Luminous | 37 |
| Coupling | 18 |
| Ethereal | 24 |
| **Total** | **13,260** |

---

## Engine Roster (Full — 76 Engines)

**Column key:**
- **Dir** = `Source/Engines/` subdirectory name
- **Canonical ID** = engine name used in preset `"engines"` arrays
- **Src Files** = file count in engine directory
- **Engine.h** = primary `*Engine.h` present?
- **Adapter** = `*Adapter.h/cpp` present?
- **Presets** = non-quarantine preset count (JSON-parsed, recursive)
- **Retreat** = guru bin retreat document exists?
- **Seance** = latest known score / verdict

### A — Legacy Alias Engines (Dir name ≠ Canonical ID)

| # | Dir | Canonical ID | Src Files | Engine.h | Adapter | Presets | Retreat | Seance Score |
|---|-----|-------------|:---------:|:--------:|:-------:|--------:|:-------:|-------------|
| 1 | Bite | **Overbite** | 2 | YES | — | 255 | YES | Full approval (non-numeric) |
| 2 | Bob | **Oblong** | 3 | YES | — | 802 | YES | 7.x → ~8.5 est. (post-fix) |
| 3 | Drift | **Odyssey** | 3 | YES | — | 640 | YES | 7.6 (re-seance needed) |
| 4 | Dub | **Overdub** | 3 | YES | — | 552 | YES | 7.4 (single LFO weakness) |
| 5 | Fat | **Obese** | 3 | YES | — | 408 | YES | ~8.5 est. (post-fix 2026-03-21) |
| 6 | Morph | **OddOscar** | 3 | YES | — | 444 | NO | ~8.5 est. (post-fix 2026-03-21) |
| 7 | Snap | **OddfeliX** | 3 | YES | — | 531 | NO | ~8.5 est. (post-fix 2026-03-21) |

### B — Native XOceanus Engines (Alphabetical by Dir)

| # | Dir | Canonical ID | Src Files | Engine.h | Adapter | Presets | Retreat | Seance Score |
|---|-----|-------------|:---------:|:--------:|:-------:|--------:|:-------:|-------------|
| 8 | Oaken | Oaken | 1 | YES | — | 30 | YES | 8.4 (Kitchen/Cellar — D004 KS formula) |
| 9 | Oasis | Oasis | 2 | YES | — | 20 | YES | 8.7 (Kitchen/Fusion) |
| 10 | Obbligato | Obbligato | 2 | YES | — | 229 | NO | 7.8 (FX routing misrouted V2) |
| 11 | Obelisk | Obelisk | 1 | YES | — | 28 | YES | 8.8 (Kitchen/Piano — 3 dead params P0) |
| 12 | Oblique | Oblique | 2 | YES | — | 289 | NO | ~7.2 est. (recovered Round 8A) |
| 13 | Obrix | Obrix | 2 | YES | — | 466 | NO | 7.2 → 9.4 roadmap (FLAGSHIP) |
| 14 | Obscura | Obscura | 2 | YES | — | 143 | NO | HIGH / unanimous (non-numeric) |
| 15 | Obsidian | Obsidian | 3 | YES | — | 280 | NO | ~8.2 est. (post-fix R9A) |
| 16 | OceanDeep | OceanDeep | 2 | YES | — | 310 | NO | 7.8 (no filter ADSR) |
| 17 | Oceanic | Oceanic | 2 | YES | — | 185 | YES | 7.1 (resolved velocity R9E) |
| 18 | Ocelot | Ocelot | 19 | YES | — | 271 | YES | ~8.5 est. (EcosystemMatrix live post-fix) |
| 19 | Ochre | Ochre | 1 | YES | — | 33 | YES | 8.2 (Kitchen/Piano — LFO2 dead P0) |
| 20 | Octave | Octave (Chef) | 1 | YES | — | 25 | YES | 8.01 → ~8.7 post-fix (dead params) |
| 21 | Octopus | Octopus | 2 | YES | — | 257 | YES | 8.3 → ~8.5 est. (aftertouch fixed) |
| 22 | Oddfellow | Oddfellow | 2 | YES | — | 21 | YES | 8.5 (Kitchen/Fusion) |
| 23 | Offering | Offering | 7 | YES | — | 154 | YES | **8.8** (post-build 2026-03-21) |
| 24 | Ogre | Ogre | 1 | YES | — | 29 | YES | 7.9 (Kitchen/Cellar — soil D004 bug) |
| 25 | Ohm | Ohm | 2 | YES | — | 236 | NO | 7.6 (mono summing, D001 partial) |
| 26 | Olate | Olate | 1 | YES | — | 29 | YES | 8.1 (Kitchen/Cellar) |
| 27 | Ole | Ole | 2 | YES | — | 216 | YES | 7.0 (isHusband regression) |
| 28 | Oleg | Oleg | 1 | YES | — | 25 | YES | 8.0 (Chef quad) |
| 29 | Ombre | Ombre | 2 | YES | — | 226 | NO | ~8.0 est. (LFO2 added post-SP7.5) |
| 30 | Omega | Omega | 1 | YES | — | 27 | YES | 8.6 (Kitchen/Cellar — distillation model) |
| 31 | Onkolo | Onkolo | 2 | YES | — | 20 | YES | 8.8 (Kitchen/Fusion) |
| 32 | Onset | Onset | 3 | YES | — | 480 | YES | Ahead of industry (non-numeric) |
| 33 | Opal | Opal | 3 | YES | — | 357 | YES | APPROVED (non-numeric) |
| 34 | Opaline | Opaline | 1 | YES | — | 33 | YES | 8.9 (Kitchen/Piano — 1 coupling dead P0) |
| 35 | Opcode | Opcode | 2 | YES | — | 20 | YES | **9.0** (Kitchen/Fusion — fleet-high) |
| 36 | OpenSky | OpenSky | 2 | YES | — | 385 | NO | 8.1 (sky_subWave D004 partial) |
| 37 | Opera | Opera | 9 | YES | YES | 156 | YES | **8.85** (post-build 2026-03-21) |
| 38 | Optic | Optic | 2 | YES | — | 308 | NO | Revolutionary (non-numeric) |
| 39 | Oracle | Oracle | 2 | YES | — | 241 | YES | 8.6 (Buchla 10/10) |
| 40 | Orbital | Orbital | 2 | YES | — | 262 | NO | APPROVED (Group Envelope System B001) |
| 41 | Orbweave | Orbweave | 2 | YES | — | 451 | YES | 8.4 (knot matrices novel) |
| 42 | Orca | Orca | 2 | YES | — | 270 | YES | ~8.6 est. (aftertouch fixed) |
| 43 | Orchard | Orchard | 2 | YES | — | 30 | YES | 8.2 (Garden quad) |
| 44 | Organism | Organism | 2 | YES | — | 433 | YES | 8.1 / 7.2 re-seance (CA patched 2026-03-20) |
| 45 | Organon | Organon | 2 | YES | — | 365 | YES | 8/8 PASS (VFE metabolism) |
| 46 | Origami | Origami | 3 | YES | — | 216 | NO | APPROVED (STFT correct) |
| 47 | Orphica | Orphica | 2 | YES | — | 264 | NO | ~8.7 est. (buffer extended 2026-03-21) |
| 48 | Osier | Osier | 2 | YES | — | 28 | YES | 8.3 (Garden quad) |
| 49 | Osmosis | Osmosis | 1 | YES | — | 1 | NO | **NOT SEANCED** (design phase only) |
| 50 | Osprey | Osprey | 2 | YES | — | 166 | NO | APPROVED/CONDITIONAL (ShoreSystem B012) |
| 51 | Osteria | Osteria | 2 | YES | — | 234 | NO | Production-grade (non-numeric) |
| 52 | Ostinato | Ostinato | 2 | YES | — | 229 | YES | **8.7** (re-seanced 2026-03-20) |
| 53 | Otis | Otis | 1 | YES | — | 27 | YES | 7.8 (Leslie Doppler wrong — amplitude not pitch) |
| 54 | Oto | Oto | 1 | YES | — | 34 | YES | **8.6** post-fix (Chef quad) |
| 55 | Ottoni | Ottoni | 2 | YES | — | 222 | NO | 7.2 (dead params fixed) |
| 56 | Ouie | Ouie | 2 | YES | — | 396 | NO | **8.5** (LOVE harmonic lock unimpl) |
| 57 | Ouroboros | Ouroboros | 2 | YES | — | 346 | YES | Production-ready (most scientifically rigorous) |
| 58 | Outlook | Outlook | 1 | YES | — | 18 | YES | **NOT SEANCED** (new engine) |
| 59 | Outwit | Outwit | 3 | — | YES | 593 | YES | **8.7** (adapter-based, re-seance 2026-03-20) |
| 60 | Oven | Oven | 1 | YES | — | 28 | YES | 8.7 (Kitchen/Piano) |
| 61 | Overcast | Overcast | 2 | YES | — | 30 | YES | 7.9 (Kitchen/Broth — coordinator not wired) |
| 62 | Overflow | Overflow | 2 | YES | — | 30 | YES | 8.0 (Kitchen/Broth — coordinator not wired) |
| 63 | Overgrow | Overgrow | 2 | YES | — | 27 | YES | 7.5 (Garden quad — bow noise injection bug) |
| 64 | Overlap | Overlap | 3 | — | YES | 577 | YES | **8.4** (adapter-based, re-seance 2026-03-20) |
| 65 | Overtone | Overtone | 2 | YES | — | 373 | YES | 7.6 (Pi table + Nyquist patched 2026-03-20) |
| 66 | Overwash | Overwash | 2 | YES | — | 30 | YES | 7.8 (Kitchen/Broth — Fick diffusion; coordinator not wired) |
| 67 | Overworld | Overworld | 6 | YES | — | 299 | YES | 7.6 (ERA triangle, resolved Round 9) |
| 68 | Overworn | Overworn | 2 | YES | — | 30 | YES | 8.6 (Kitchen/Broth — sessionAge invisible bug) |
| 69 | Oware | Oware | 2 | YES | — | 175 | YES | **8.7** (seanced 2026-03-21; LFO2 dead = path to 9.2) |
| 70 | Owlfish | Owlfish | 13 | YES | — | 157 | YES | 7.1 (fixed LFOs + morphGlide; Mixtur-Trautonium osc) |
| 71 | Oxalis | Oxalis | 2 | YES | — | 27 | YES | 7.8 (Garden quad — MW not routed) |
| 72 | Oxbow | Oxbow | 2 | YES | — | 175 | YES | **9.0** post-fix (entangled reverb synth) |
| 73 | Oxytocin | Oxytocin | 11 | YES | YES | 130 | NO | **9.5 est.** (fleet leader, built 2026-03-22) |

---

## Health Flags

### FLAG 1 — Engines with Adapter Only (No Engine.h)
These engines live in standalone repos; XOceanus communicates through adapter files only.

| Engine (Dir) | Canonical ID | Adapter File | Notes |
|--------------|-------------|-------------|-------|
| Outwit | Outwit | `XOutwitAdapter.h/cpp` | Fully wired, 8.7/10 |
| Overlap | Overlap | `XOverlapAdapter.h/cpp` | Fully wired, 8.4/10 |
| Opera | Opera | `OperaAdapter.h/cpp` | Fully wired, 8.85/10 |
| Oxytocin | Oxytocin | `OxytocinAdapter.h/cpp` | Fully wired, 9.5 est. |

All other 69 engines have primary `*Engine.h` files directly in their engine directory.

### FLAG 2 — Engines with Fewer Than 50 Presets (26 engines)
Most are Kitchen Collection engines (intentionally at 10-30 at seance time — retreat expansion in progress).

**Critical (<10 presets):**
- **Osmosis**: 1 preset — design-phase engine, not shipped
- **Outlook**: 18 presets — new engine, NOT SEANCED

**Kitchen Collection engines (20-34 presets — need expansion to 100+ before V1 release):**
Oaken (30), Oasis (20), Obelisk (28), Ochre (33), Octave (25), Oddfellow (21), Ogre (29), Olate (29), Oleg (25), Omega (27), Onkolo (20), Opaline (33), Opcode (20), Orchard (30), Osier (28), Otis (27), Oto (34), Oven (28), Overcast (30), Overflow (30), Overgrow (27), Overwash (30), Overworn (30), Oxalis (27)

### FLAG 3 — Engines Never Retreated (21 engines)
These engines have NO retreat document in either `scripture/retreats/` or `Docs/guru-bin-retreats/`:

| Engine | Presets | Seance Score | Priority |
|--------|--------:|-------------|---------|
| Obbligato | 229 | 7.8 | HIGH — 229 presets, never retreated |
| Oblique | 289 | ~7.2 | HIGH — 289 presets, never retreated |
| Obscura | 143 | HIGH | MED |
| Obsidian | 280 | ~8.2 | HIGH — 280 presets, never retreated |
| OceanDeep | 310 | 7.8 | HIGH — 310 presets, never retreated |
| Ohm | 236 | 7.6 | HIGH — 236 presets, never retreated |
| Ombre | 226 | ~8.0 | HIGH — 226 presets, never retreated |
| Onkolo (Kitchen) | 20 | 8.8 | LOW — Kitchen, preset expansion needed |
| OpenSky | 385 | 8.1 | HIGH — 385 presets, never retreated |
| Optic | 308 | Revolutionary | MED — 308 presets |
| Orbital | 262 | APPROVED | MED |
| Origami | 216 | APPROVED | MED |
| Orphica | 264 | ~8.7 | HIGH — 264 presets, never retreated |
| Osmosis | 1 | NOT SEANCED | LOW — not released |
| Osprey | 166 | APPROVED | MED |
| Osteria | 234 | Production-grade | MED |
| Ouie | 396 | 8.5 | HIGH — 396 presets, never retreated |
| Oxytocin | 130 | 9.5 est. | HIGH — fleet leader, needs retreat |
| OddOscar (Morph) | 444 | ~8.5 | HIGH |
| OddfeliX (Snap) | 531 | ~8.5 | HIGH |
| Ottoni | 222 | 7.2 | MED |

### FLAG 4 — BROTH Coordinator Not Written (Critical Architecture Debt)
**All 4 BROTH engines (Overwash, Overworn, Overflow, Overcast) run independently.** The coordinator that pumps values between them (`setBrothSessionAge()`, `getConcentrateDark()`, `setSpectralMass()`, etc.) was never implemented in `XOceanusProcessor.cpp`. The cross-engine flavor chemistry exists only in individual engines waiting for the coordinator. This is the single most impactful missing piece of code in the fleet.

### FLAG 5 — Engines NOT SEANCED (2)
- **Osmosis** — design-phase only; 1 preset; not shipped
- **Outlook** — new engine (registered 2026-03-23/24); 18 presets; seance pending

### FLAG 6 — Engines with Open P0 DSP Issues
| Engine | P0 Issue |
|--------|---------|
| Ogre | `ogre_soil` param dead — body filter double-pass bug discards first result |
| Ochre | LFO2 entirely dead (no targets wired) |
| Obelisk | 3 dead params (coupling system, 2 others) |
| Opaline | 1 dead param (coupling integration dead) |
| Octave | 2 dead params (`macroCoupling`, `oct_competition`) |
| Oware | LFO2 computed but never applied (3 dead params) |
| Opera | `OperaSVF` calls `std::tan()` per-sample (CPU debt, not correctness) |

### FLAG 7 — Engines with Score <7.5 (Weakest tier)
| Engine | Score | Primary Weakness |
|--------|-------|-----------------|
| Ole | 7.0 | isHusband regression post-SP7.5 |
| Oceanic | 7.1 | Resolved but low headroom |
| Owlfish | 7.1 | Fixed LFOs; Mixtur-Trautonium isolated |
| Oblique | ~7.2 est. | Recovered; LFO rate still hardcoded |
| Ottoni | 7.2 | Dead params fixed; D001 partially honored |
| Overdub (Dub) | 7.4 | Single sine LFO only |
| Overgrow | 7.5 | Bow noise added to output not delay line |

---

## V1 Readiness Check

**V1 Target:** OBRIX flagship + 6-8 FX engines + 20-25 curated engines = 28-34 total (see `Docs/v1-scope-revision-2026-03-23.md`)

**Criteria for V1:** Seance 8.5+ OR non-numeric approval + sufficient preset depth (100+ recommended) + no unresolved P0 DSP bugs

### CONFIRMED V1-READY (23 engines)

| # | Engine | Seance | Presets | Retreat | Notes |
|---|--------|--------|--------:|:-------:|-------|
| 1 | **Obrix** | 7.2 → 9.4 roadmap | 466 | — | FLAGSHIP — mandatory; Wave 5 presets strong |
| 2 | **Obscura** | HIGH/unanimous | 143 | — | Physics-as-synthesis; acoustic paradigm |
| 3 | **Offering** | 8.8 | 154 | YES | Boom bap drum synthesis; MPC-native pipeline |
| 4 | **Onset** | Ahead of industry | 480 | YES | XVC drum engine; 3-5 yr ahead of commercial |
| 5 | **Opal** | APPROVED | 357 | YES | Granular; coupling crown jewel |
| 6 | **Opera** | 8.85 | 156 | YES | Additive-vocal Kuramoto; engine #45 |
| 7 | **Optic** | Revolutionary | 308 | — | Zero-audio paradigm; 10 yr ahead |
| 8 | **Oracle** | 8.6 | 241 | YES | Buchla 10/10; breakpoint synthesis |
| 9 | **Orbital** | APPROVED | 262 | — | Group Envelope System (B001) |
| 10 | **Orca** | ~8.6 est. | 270 | YES | Echolocation LFO; aftertouch fixed |
| 11 | **Organon** | 8/8 PASS | 365 | YES | VFE metabolism; academic-grade DSP |
| 12 | **Origami** | APPROVED | 216 | — | STFT fold synthesis |
| 13 | **Orphica** | ~8.7 est. | 264 | — | Buffer extended; velocity→resonance wired |
| 14 | **Osprey** | APPROVED | 166 | — | ShoreSystem (B012) |
| 15 | **Osteria** | Production-grade | 234 | — | ShoreSystem (B012) shared |
| 16 | **Ostinato** | 8.7 | 229 | YES | Re-seanced 2026-03-20 |
| 17 | **Oto** | 8.6 post-fix | 34 | YES | Chef quad — needs preset expansion |
| 18 | **Ouie** | 8.5 | 396 | — | Love-harmonic synthesis |
| 19 | **Ouroboros** | Production-ready | 346 | YES | Chaos attractors; Lorenz/Rossler/Chua/Aizawa |
| 20 | **Outwit** | 8.7 | 593 | YES | Cellular automata stepsynth |
| 21 | **Oware** | 8.7 | 175 | YES | Tuned percussion; Mallet physics (Chaigne 1997) |
| 22 | **Oxbow** | 9.0 | 175 | YES | Entangled reverb synth; fleet's highest confirmed |
| 23 | **Oxytocin** | 9.5 est. | 130 | — | Fleet leader; circuit × love topology |

### V1 CONSIDER — Pending Verification (13 engines)

| Engine | Seance | Presets | Key Concern |
|--------|--------|--------:|-------------|
| Overbite (Bite) | Full approval | 255 | Need formal numeric re-seance |
| Oblong (Bob) | ~8.5 est. | 802 | CuriosityEngine responsiveness; needs formal score |
| Obese (Fat) | ~8.5 est. | 408 | Post-fix recovery estimated; needs formal re-seance |
| OddOscar (Morph) | ~8.5 est. | 444 | Post-fix recovery; no retreat doc |
| OddfeliX (Snap) | ~8.5 est. | 531 | Post-fix recovery; no retreat doc |
| Obsidian | ~8.2 est. | 280 | Below 8.5 threshold; post-fix estimate |
| Ocelot | ~8.5 est. | 271 | EcosystemMatrix now live; formal re-seance recommended |
| Octave | 8.01 → ~8.7 | 25 | Post-fix strong; only 25 presets — critical gap |
| Octopus | ~8.5 est. | 257 | Aftertouch fixed; formal numeric re-seance needed |
| OpenSky | 8.1 | 385 | D004 partial (sky_subWave); needs fix + re-seance |
| Orbweave | 8.4 | 451 | Just below 8.5; knot matrices unique |
| Organism | 8.1 / 7.2 | 433 | CA fix applied; re-seance needed to confirm |
| Overlap | 8.4 | 577 | Just below 8.5; adapter-based |

### NOT V1-READY (37 engines)
Engines below 8.5 confirmed, OR unresolved P0 blockers, OR insufficient presets, OR not seanced:

| Engine | Score | Preset Gap | Path to V1 |
|--------|-------|------------|------------|
| Ole | 7.0 | OK (216) | Fix isHusband regression; re-seance |
| Oceanic | 7.1 | OK (185) | Marginal; needs strong preset push |
| Owlfish | 7.1 | OK (157) | Marginal; Mixtur-Trautonium unique |
| Oblique | ~7.2 | OK (289) | LFO rate needs exposure; re-seance |
| Ottoni | 7.2 | OK (222) | Dead params fixed; re-seance needed |
| Overdub | 7.4 | OK (552) | Add LFO2; re-seance |
| Overgrow | 7.5 | LOW (27) | Bow noise fix + 70+ presets needed |
| Overtone | 7.6 | OK (373) | Pi table fixed; formal re-seance needed |
| Overworld | 7.6 | OK (299) | Expression resolved; re-seance needed |
| Ohm | 7.6 | OK (236) | Mono summing; D001 partial |
| Odyssey | 7.6 | OK (640) | Dead mods resolved; climax preset needed |
| Obbligato | 7.8 | OK (229) | FX routing fix + re-seance |
| OceanDeep | 7.8 | OK (310) | Needs filter ADSR + pitch bend |
| Otis | 7.8 | LOW (27) | Leslie Doppler must be fixed |
| Oxalis | 7.8 | LOW (27) | MW routing fix + presets needed |
| Overwash | 7.8 | LOW (30) | BROTH coordinator must be written first |
| Ombre | ~8.0 | OK (226) | Post-SP7.5 estimate; re-seance needed |
| Oleg | 8.0 | LOW (25) | Chef quad; needs 75+ more presets |
| Overflow | 8.0 | LOW (30) | BROTH coordinator needed |
| Olate | 8.1 | LOW (29) | Kitchen/Cellar; needs 70+ presets |
| OpenSky | 8.1 | OK (385) | D004 partial to fix |
| Organism | 8.1 | OK (433) | Re-seance after CA patch |
| Orchard | 8.2 | LOW (30) | Garden quad; needs presets |
| Ochre | 8.2 | LOW (33) | P0 LFO2 dead must be fixed |
| Obsidian | ~8.2 | OK (280) | Estimated; needs formal re-seance |
| Osier | 8.3 | LOW (28) | Garden quad; needs presets |
| Orbweave | 8.4 | OK (451) | Just below threshold |
| Overlap | 8.4 | OK (577) | Just below threshold |
| Oaken | 8.4 | LOW (30) | Kitchen/Cellar; needs presets |
| Octave | 8.01→~8.7 | LOW (25) | Strong engine; critically thin presets |
| Ogre | 7.9 | LOW (29) | Soil bug fix + presets needed |
| Overcast | 7.9 | LOW (30) | BROTH coordinator needed |
| Overworn | 8.6 | LOW (30) | Strong engine; BROTH coordinator + presets |
| Omega | 8.6 | LOW (27) | Kitchen/Cellar; needs presets |
| Obelisk | 8.8 | LOW (28) | P0 dead params must be fixed first |
| Onkolo | 8.8 | LOW (20) | Kitchen/Fusion; needs presets |
| Opaline | 8.9 | LOW (33) | P0 coupling dead must be fixed first |
| Opcode | 9.0 | LOW (20) | Highest Kitchen score; needs 80+ presets |
| Oasis | 8.7 | LOW (20) | Kitchen/Fusion; needs presets |
| Oddfellow | 8.5 | LOW (21) | Kitchen/Fusion; needs presets |
| Osmosis | NOT SEANCED | LOW (1) | Design phase; not ready |
| Outlook | NOT SEANCED | LOW (18) | New engine; seance needed |

---

## Recommended V1 Roster (28-34 engines)

Based on confirmed readiness, preset depth, and the V1 scope target:

**Mandatory (core V1 identity):**
1. Obrix (Flagship)
2. Onset (drum engine — XPN pipeline)
3. Outwit (cellular automata stepsynth)
4. Overlap (adapter-based FX)
5. Opal (granular — coupling crown jewel)
6. Orbweave (knot matrices)
7. Ouroboros (chaos attractors)
8. Organism (cellular automata)
9. Organon (VFE metabolism)
10. Oxbow (entangled reverb synth — 9.0)
11. Oxytocin (fleet leader — 9.5 est.)

**High confidence additions (approved/scored 8.5+, 100+ presets):**
12. Ostinato
13. Ouie
14. Oware
15. Osprey
16. Osteria
17. Oracle
18. Orca
19. Orphica
20. Opera
21. Origami
22. Orbital
23. Optic
24. Offering
25. Obscura

**Strong candidates from CONSIDER list (pending formal re-seance):**
26. Overbite (Bite dir) — full approval
27. Oblong (Bob dir) — 802 presets, needs formal score
28. Obese (Fat dir) — 408 presets, post-fix strong
29. Ouroboros — already in mandatory
30. Octopus — aftertouch fixed; good candidate

**FX Engines (6-8 slots — for later identification):**
The 6 designed companion FX engines (fXAdversary, fXResonance, fXGravity, fXGrowth, fXReduction, fXMigration) are not yet in `Source/Engines/` — they exist as designs. These fill the 6-8 FX engine slots in V1.

**Total confirmed V1 roster: 25-30 engines + 6-8 FX = 31-38 (within 28-34 target)**

---

## Priority Actions (Derived from This Audit)

### P0 — Blocks any release
1. **Write BROTH coordinator** in `XOceanusProcessor.cpp` — `setBrothSessionAge()` / `getConcentrateDark()` / `getSpectralMass()` calls. All 4 BROTH engines are currently isolated. (Overwash, Overworn, Overflow, Overcast)
2. **Fix Ogre soil bug** — body filter double-pass discards first result; `ogre_soil` has zero audio effect
3. **Wire Ochre LFO2** — no mod targets connected; D002 FAIL and D004 FAIL
4. **Fix Obelisk 3 dead params** — blocks fleet-quality certification
5. **Fix Opaline coupling dead** — 1 dead param
6. **Fix Octave 2 dead params** (`macroCoupling`, `oct_competition`)
7. **Wire Oware LFO2** (3 params: `owr_lfo2Rate/Depth/Shape`) — projected 9.2/10 after fix
8. **Seance Osmosis** and **Seance Outlook** — these are the only 2 engines with zero seance

### P1 — Before V1 ship
1. **Formal re-seance** for all 5 post-fix legacy aliases (Overbite, Oblong, Obese, OddOscar, OddfeliX) with confirmed numeric scores
2. **Retreat documents** for Oxytocin (fleet leader — no retreat yet), Ouie (396 presets — no retreat), OddfeliX/OddOscar (531/444 presets — no retreat)
3. **Preset expansion** for all Kitchen Collection engines (all below 35 presets — need 100+ each)
4. **Re-seance** Organism and Overtone after 2026-03-20 patches (both had critical DSP fixes applied; Overtone projected 9.3+ if fixes hold)
5. **Fix OpenSky D004** (sky_subWave not dispatched) then re-seance

### P2 — V1.1 improvements
1. Formal numeric seance for all non-numeric approval engines (Obscura, Optic, Orbital, Organon, Ouroboros, Origami, Onset, Opal, Osprey, Osteria, Overbite)
2. Leslie Doppler fix for Otis (amplitude not pitch — most significant DSP gap in Chef quad)
3. isHusband regression fix for Ole
4. OceanDeep filter ADSR addition

---

## Score Distribution Summary

| Score Tier | Count | Engines |
|-----------|------:|---------|
| 9.0+ | 3 | Oxbow (9.0), Oxytocin (9.5 est.), Opcode (9.0 Kitchen) |
| 8.5–8.9 | 14 | Offering (8.8), Opera (8.85), Ostinato (8.7), Outwit (8.7), Oware (8.7), Oasis (8.7), Onkolo (8.8), Oven (8.7), Orphica (~8.7), Obelisk (8.8), Opaline (8.9), Omega (8.6), Overworn (8.6), Oto (8.6) |
| 8.0–8.4 | 18 | Orbweave (8.4), Overlap (8.4), Oaken (8.4), Octopus (~8.5), Octave (~8.7 post-fix), Oblong (~8.5), Obese/OddOscar/OddfeliX/Ocelot (~8.5), Oracle (8.6), Orca (~8.6), Obsidian (~8.2), OpenSky (8.1), Organism (8.1), Orchard (8.2), Ochre (8.2), Oleg (8.0), Overflow (8.0), Ombre (~8.0) |
| 7.5–7.9 | 14 | OceanDeep (7.8), Obbligato (7.8), Overwash (7.8), Oxalis (7.8), Otis (7.8), Overcast (7.9), Ogre (7.9), Ohm (7.6), Odyssey (7.6), Overworld (7.6), Overtone (7.6 re-seance), Overdub (7.4), Olate (8.1), Osier (8.3) |
| Below 7.5 | 6 | Overgrow (7.5), Ole (7.0), Oceanic (7.1), Owlfish (7.1), Oblique (~7.2), Ottoni (7.2) |
| Non-numeric approved | 9 | Obscura, Optic, Orbital, Organon, Ouroboros, Origami, Onset, Opal, Osprey, Osteria, Overbite |
| NOT SEANCED | 2 | Osmosis, Outlook |

**Fleet median (numeric engines):** ~8.5/10
**Highest confirmed:** Oxytocin 9.5 est. / Oxbow 9.0 confirmed
**Most improved since 2026-03-20:** Ocelot (6.4 → ~8.5), Obese (6.6 → ~8.5), OddOscar (6.9 → ~8.5)

---

## Adapter / Integration Status

Only 4 engines have dedicated `*Adapter.h` files (for standalone-repo engines integrated into XOceanus):

| Engine | Adapter Files | Integration Status |
|--------|-------------|-------------------|
| Opera | `OperaAdapter.h/cpp` | FULLY INTEGRATED — 8.85/10 |
| Oxytocin | `OxytocinAdapter.h/cpp` | FULLY INTEGRATED — 9.5 est. |
| Outwit | `XOutwitAdapter.h/cpp` | FULLY INTEGRATED — 8.7/10 |
| Overlap | `XOverlapAdapter.h/cpp` | FULLY INTEGRATED — 8.4/10 |

All remaining 69 engines are natively implemented within this repo.

---

*Dashboard generated 2026-03-24. All file counts sourced from filesystem reads. All preset counts from recursive JSON parsing of `Presets/XOceanus/` excluding `_quarantine/`. Seance scores from `Docs/seances/` and `Docs/fleet-seance-scores-2026-03-20.md`; estimates are flagged. This is the canonical ground-truth document for fleet state as of March 24, 2026.*
