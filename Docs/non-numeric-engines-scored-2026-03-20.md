# Non-Numeric Engines -- Formal Numeric Scoring

**Date:** 2026-03-20
**Scorer:** Ghost Council (Opus 4.6 session)
**Method:** Full source code review of each engine header, evaluated against 6 Doctrines + DSP quality criteria

---

## Summary Table

| # | Engine | Previous Verdict | Numeric Score | Needs DSP Work? | Primary Gap |
|---|--------|-----------------|---------------|-----------------|-------------|
| 1 | ORGANON | 8/8 PASS | **8.5** | Minor | No macros, no user LFO |
| 2 | OBSCURA | High / unanimous | **9.1** | No | No mod matrix (minor) |
| 3 | OPTIC | Revolutionary | **8.3** | Minor | No MIDI expression, no macros |
| 4 | ORBITAL | APPROVED | **8.7** | Minor | No dedicated user LFO |
| 5 | OUROBOROS | Production-ready | **9.0** | No | No macros, no post-filter |
| 6 | OVERBITE | Full approval | **9.2** | No | Mod matrix depth control |
| 7 | ONSET | Ahead of industry | **8.8** | Minor | No user LFO (breathing LFO fixed rate) |
| 8 | OSPREY | APPROVE/CONDITIONAL | **8.4** | Minor | Single LFO, fixed shape |
| 9 | OSTERIA | Production-grade | **8.2** | Yes | No user LFO, D005 borderline |
| 10 | OPAL | Concept reviewed | **8.6** | No | Mod matrix depth control |
| 11 | ORIGAMI | Not formally scored | **8.0** | Yes | STFT block constraint, no post-filter |

---

## Engines Already at 8.0+ (No Urgent DSP Needed)

All 11 engines score 8.0 or above. The fleet quality floor is high.

**Top tier (9.0+):**
- OVERBITE: 9.2 -- Best expression system in fleet (5 macros, 3 LFOs, 3 envelopes)
- OBSCURA: 9.1 -- Best physics + most complete feature set (2 LFOs, dual envelopes, 4 voice modes)
- OUROBOROS: 9.0 -- Best chaos synthesis, B003 Leash + B007 Velocity Coupling

**Strong (8.5-8.9):**
- ONSET: 8.8 -- B002 XVC is 3-5 years ahead. Needs user LFO control.
- ORBITAL: 8.7 -- B001 Group Envelopes. Needs dedicated LFO.
- OPAL: 8.6 -- Full granular engine with 2 LFOs. Mod matrix depth control missing.
- ORGANON: 8.5 -- B011 VFE is publishable. Needs macros + LFO.

**Solid (8.0-8.4):**
- OSPREY: 8.4 -- B012 ShoreSystem. Single LFO, fixed shape.
- OPTIC: 8.3 -- B005 Zero-Audio. No MIDI processing.
- OSTERIA: 8.2 -- B012 ShoreSystem. No user LFO, D005 borderline.
- ORIGAMI: 8.0 -- Spectral folding unique. STFT block constraint, no post-filter.

---

## Engines That Would Most Benefit from DSP Improvement

### Priority 1: Quick wins to reach 8.5+

| Engine | Current | Target | Fix | LOC |
|--------|---------|--------|-----|-----|
| OSTERIA | 8.2 | 8.5 | Add user-controllable LFO (rate/depth/shape) | ~60 |
| ORIGAMI | 8.0 | 8.5 | Add post-output SVF filter + scale hop size | ~65 |
| OPTIC | 8.3 | 8.5 | Add MIDI CC processing (aftertouch/modwheel) | ~40 |

### Priority 2: Quick wins to reach 9.0

| Engine | Current | Target | Fix | LOC |
|--------|---------|--------|-----|-----|
| ORGANON | 8.5 | 9.0 | Add 4 macros + breathing LFO + velocity->enzyme | ~85 |
| ORBITAL | 8.7 | 9.0 | Add 2 user LFOs with rate/depth/shape | ~80 |
| ONSET | 8.8 | 9.0 | Make breathing LFO rate user-controllable + add 2nd LFO | ~75 |
| OSPREY | 8.4 | 9.0 | Add LFO shape param + 2nd LFO | ~60 |
| OPTIC | 8.3 | 9.0 | Add MIDI + 4 macros | ~70 |

---

## Doctrine Violation Heatmap (these 11 engines)

| Doctrine | Engines with issues | Severity |
|----------|-------------------|----------|
| D001 | None | All 11 pass |
| D002 | ORGANON (no user LFO), OSTERIA (no user LFO), ONSET (fixed LFO), OSPREY (single fixed LFO), OPTIC (no MIDI) | Low-Medium |
| D003 | N/A for most; OBSCURA + OUROBOROS exemplary | N/A |
| D004 | None | All 11 pass |
| D005 | OSTERIA (borderline -- internal LFOs but no rate <= 0.01 Hz), ORGANON (VFE breathes but no discrete LFO) | Low |
| D006 | OPTIC (no MIDI processing at all) | Medium |

---

## Fleet Score Distribution (all engines with numeric scores)

| Score Range | Count | Engines |
|-------------|-------|---------|
| 9.0+ | 3 | OVERBITE (9.2), OBSCURA (9.1), OUROBOROS (9.0) |
| 8.5-8.9 | 6 | ONSET (8.8), ORBITAL (8.7), ORACLE (8.6), OPAL (8.6), ORGANON (8.5), OCTOPUS (8.3) |
| 8.0-8.4 | 5 | OSPREY (8.4), OPTIC (8.3), OSTERIA (8.2), ORCA (8.1), OSTINATO (8.0), ORIGAMI (8.0) |
| 7.5-7.9 | 3 | OMBRE (7.8), ODYSSEY (7.6), OVERWORLD (7.6) |
| 7.0-7.4 | 4 | OVERDUB (7.4), OBLIQUE (7.2), OBLONG (7.x), OCEANIC (7.1), OWLFISH (7.1) |
| 6.0-6.9 | 4 | ODDOSCAR (6.9), OBESE (6.6), OBSIDIAN (6.6), OCELOT (6.4) |
| < 6.0 | 1 | OBLIQUE (5.9 original, 7.2 post-recovery) |

**Fleet average (scored engines):** ~7.9/10
**Fleet median:** ~8.0/10

---

*Verdict files written to `scripture/seances/{engine}-seance-2026-03-20.md` for each of the 11 engines.*
