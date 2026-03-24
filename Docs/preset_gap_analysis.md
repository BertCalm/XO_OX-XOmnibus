# XOlokun Preset Gap Analysis

**Generated:** 2026-03-19
**Total presets scanned:** 10,514
**Fleet target:** 150+ presets per engine
**Engines tracked:** 39

---

## Summary

| Status | Count |
|--------|-------|
| At or above target (150+) | 34 |
| Below target | 5 |
| **Critical (0 presets)** | **5** |

All 5 engines below target are newly shipped (2026-03-18 / 2026-03-19) and have no factory presets yet.

---

## Engines Below Target

| Engine | Gallery Code | Current Count | Target | Deficit | Priority |
|--------|-------------|--------------|--------|---------|----------|
| Obrix | OBRIX | 0 | 150 | **150** | P1 — Wave 4 (after param freeze) ⚠️ |
| Ostinato | OSTINATO | 0 | 150 | **150** | P1 — V1.2 |
| OpenSky | OPENSKY | 0 | 150 | **150** | P1 — V1.2 |
| OceanDeep | OCEANDEEP | 0 | 150 | **150** | P1 — V1.2 |
| Ouie | OUIE | 0 | 150 | **150** | P1 — V1.2 |

**All 5 are new engines that shipped with DSP but no preset library.** Use `/exo-meta` + `/guru-bin` to design and fill each engine's preset library. Target 150 per engine across all 7 moods.

> ⚠️ **OBRIX preset exception:** Do NOT author OBRIX presets yet. Waves 1–3 will add ~18 new parameters to OBRIX (FM depth, filter feedback, wavetables, unison, drift, journey mode, spatial). Presets written against the Wave 1 schema will break when Wave 2+ params are added. OBRIX preset authoring is Wave 4 work — after all parameters are frozen. Use `/exo-meta` on OSTINATO/OPENSKY/OCEANDEEP/OUIE first.

---

## Full Fleet Counts (All 39 Engines)

| Engine | Gallery Code | Preset Count | Status |
|--------|-------------|-------------|--------|
| Oblong | OBLONG | 859 | ✅ AT TARGET |
| Odyssey | ODYSSEY | 796 | ✅ AT TARGET |
| Organon | ORGANON | 695 | ✅ AT TARGET |
| OddfeliX | ODDFELIX | 645 | ✅ AT TARGET |
| Oracle | ORACLE | 616 | ✅ AT TARGET |
| Overdub | OVERDUB | 609 | ✅ AT TARGET |
| Origami | ORIGAMI | 603 | ✅ AT TARGET |
| Opal | OPAL | 593 | ✅ AT TARGET |
| OddOscar | ODDOSCAR | 586 | ✅ AT TARGET |
| Obese | OBESE | 577 | ✅ AT TARGET |
| Ouroboros | OUROBOROS | 549 | ✅ AT TARGET |
| Outwit | OUTWIT | 546 | ✅ AT TARGET |
| Ohm | OHM | 535 | ✅ AT TARGET |
| Overlap | OVERLAP | 521 | ✅ AT TARGET |
| Oblique | OBLIQUE | 513 | ✅ AT TARGET |
| Orphica | ORPHICA | 504 | ✅ AT TARGET |
| Obbligato | OBBLIGATO | 501 | ✅ AT TARGET |
| Overworld | OVERWORLD | 495 | ✅ AT TARGET |
| Onset | ONSET | 492 | ✅ AT TARGET |
| Obscura | OBSCURA | 489 | ✅ AT TARGET |
| Optic | OPTIC | 470 | ✅ AT TARGET |
| Ole | OLE | 471 | ✅ AT TARGET |
| Ottoni | OTTONI | 473 | ✅ AT TARGET |
| Orbital | ORBITAL | 432 | ✅ AT TARGET |
| Oceanic | OCEANIC | 413 | ✅ AT TARGET |
| Osprey | OSPREY | 397 | ✅ AT TARGET |
| Obsidian | OBSIDIAN | 392 | ✅ AT TARGET |
| Overbite | OVERBITE | 377 | ✅ AT TARGET |
| Ocelot | OCELOT | 362 | ✅ AT TARGET |
| Osteria | OSTERIA | 317 | ✅ AT TARGET |
| Owlfish | OWLFISH | 314 | ✅ AT TARGET |
| Ombre | OMBRE | 312 | ✅ AT TARGET |
| Orca | ORCA | 272 | ✅ AT TARGET |
| Octopus | OCTOPUS | 260 | ✅ AT TARGET |
| **Obrix** | **OBRIX** | **0** | ❌ DEFICIT 150 — Wave 4 only (param freeze pending) |
| **Ostinato** | **OSTINATO** | **0** | ❌ DEFICIT 150 |
| **OpenSky** | **OPENSKY** | **0** | ❌ DEFICIT 150 |
| **OceanDeep** | **OCEANDEEP** | **0** | ❌ DEFICIT 150 |
| **Ouie** | **OUIE** | **0** | ❌ DEFICIT 150 |

---

## Notes

- **Preset schema fixes applied 2026-03-19:** 63 hard FAIL presets fixed (32 missing `engines` field, 31 `schema_version` string → integer). All Ocelot presets had missing `engines` field; all Obscura/Oracle/Ouro/etc. had string `schema_version`.
- **Engine name normalization 2026-03-19:** 80 presets used legacy names `XOverlap`/`XOutwit` → corrected to canonical `Overlap`/`Outwit`.
- **Validation status post-fix:** 0 FAILs, 42% PASS, 58% WARN (warnings are coupling type name style issues, not blocking).
- **Entangled presets:** Multi-engine presets are counted once per engine they reference, so totals across all engines exceed 10,514.

---

## Next Steps for New Engines

Run preset generation for each of the 5 deficit engines:

```
/exo-meta → design 150 presets per engine across Foundation/Atmosphere/Entangled/Prism/Flux/Aether/Family
/guru-bin → refine sound quality
/preset-forge → convert refinement logs to .xometa files
/preset-qa → validate before committing
```

**Mood distribution target per engine (150 total):**
- Foundation: 25 | Atmosphere: 25 | Entangled: 20 | Prism: 25 | Flux: 25 | Aether: 20 | Family: 10
