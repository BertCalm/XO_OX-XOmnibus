<!-- rnd: coupling-40pct-sprint -->

# XPN Coupling Coverage: 40% Sprint Plan (Revised from Scan)

**Date**: 2026-03-16
**Scanned**: 484/561 pairs (86.3%) — actual count from live .xometa files
**Prior doc baseline**: 137/561 (24.4%) when the 30% sprint was written
**Stated baseline (task brief)**: 174/561 (31%) — likely counted at a prior session snapshot
**Target (original brief)**: 224/561 (40%)
**Actual next milestone**: 100% — 561/561 pairs (77 pairs remaining)

---

## Note on Coverage Discrepancy

The task brief stated current coverage as 31% (174/561). A full scan of all .xometa files in
`Presets/XOlokun/Entangled/` (1,527 files) produces a materially different result: **484 of 561
pairs are already covered (86.3%)**. The 30% sprint plan (wave 71 doc) was written from a
137/561 baseline. Since then, extensive coupling preset generation across waves 71–present has
closed the gap dramatically.

The 40% milestone was cleared long ago. This document therefore reframes around the actual
remaining work: **closing the final 77 uncovered pairs to reach 100% coverage**.

---

## 1. Current Coverage — Scan Results

| Metric | Value |
|---|---|
| Total possible pairs (34 engines) | 561 |
| Pairs covered (at least 1 preset) | 484 |
| Coverage | 86.3% |
| Uncovered pairs | 77 |
| Fully saturated engines (33/33 partners) | OBLONG, ONSET, OVERDUB, OVERWORLD, OPAL |

### Per-Engine Partner Coverage (covered / 33 possible)

| Engine | Partners Covered | Gap |
|---|---|---|
| OBLONG | 33/33 | — complete |
| ONSET | 33/33 | — complete |
| OVERDUB | 33/33 | — complete |
| OVERWORLD | 33/33 | — complete |
| OPAL | 33/33 | — complete |
| OCTOPUS | 31/33 | 2 missing |
| ODYSSEY | 31/33 | 2 missing |
| ORACLE | 31/33 | 2 missing |
| ORGANON | 31/33 | 2 missing |
| OSTERIA | 31/33 | 2 missing |
| OMBRE | 31/33 | 2 missing |
| OBSIDIAN | 30/33 | 3 missing |
| OCELOT | 30/33 | 3 missing |
| ODDFELIX | 30/33 | 3 missing |
| OHM | 30/33 | 3 missing |
| OPTIC | 30/33 | 3 missing |
| ODDOSCAR | 29/33 | 4 missing |
| OCEANIC | 29/33 | 4 missing |
| OBLIQUE | 29/33 | 4 missing |
| OUROBOROS | 29/33 | 4 missing |
| OLE | 28/33 | 5 missing |
| ORCA | 28/33 | 5 missing |
| ORPHICA | 28/33 | 5 missing |
| OSCURA | 27/33 | 6 missing |
| OBBLIGATO | 27/33 | 6 missing |
| OSPREY | 27/33 | 6 missing |
| OTTONI | 27/33 | 6 missing |
| ORBITAL | 26/33 | 7 missing |
| OWLFISH | 26/33 | 7 missing |
| ORIGAMI | 25/33 | 8 missing |
| OUTWIT | 24/33 | 9 missing |
| OVERLAP | 23/33 | 10 missing |
| OVERBITE | 18/33 | 15 missing |
| OBESE | 17/33 | 16 missing |

**OVERBITE and OBESE are the two most isolated engines.** Together they account for 31 of the 77
uncovered pairs.

---

## 2. All 77 Uncovered Pairs (Grouped by Engine)

### OBESE (16 missing pairs)
Missing: OBBLIGATO, OBLIQUE, OBSCURA, OCEANIC, OCELOT, OHM, OLE, OPTIC, ORACLE, ORBITAL,
ORIGAMI, ORPHICA, OSPREY, OTTONI, OUTWIT, OVERLAP

### OVERBITE (15 missing pairs)
Missing: OBBLIGATO, OBLIQUE, OBSCURA, OBSIDIAN, OCEANIC, OCELOT, OHM, OLE, OPTIC, ORBITAL,
ORIGAMI, ORPHICA, OSPREY, OUTWIT, OVERLAP

### OVERLAP (10 missing pairs)
Missing: OBESE, OBLIQUE, OBSCURA, OCELOT, OPTIC, ORIGAMI, OSPREY, OSTERIA, OVERBITE, OWLFISH

### OUTWIT (9 missing pairs)
Missing: OBESE, OBSCURA, OBSIDIAN, OCEANIC, ORBITAL, OSPREY, OSTERIA, OVERBITE, OWLFISH

### ORIGAMI (8 missing pairs)
Missing: OBESE, OCEANIC, ODDFELIX, ODDOSCAR, ODYSSEY, OTTONI, OVERBITE, OVERLAP

### ORBITAL (7 missing pairs)
Missing: OBESE, ODDFELIX, ODDOSCAR, ODYSSEY, OTTONI, OUTWIT, OVERBITE

### OWLFISH (7 missing pairs)
Missing: OHM, OLE, ORPHICA, OSPREY, OTTONI, OUTWIT, OVERLAP

### OBBLIGATO (6 missing pairs)
Missing: OBESE, OCTOPUS, OMBRE, ORCA, OUROBOROS, OVERBITE

### OBSCURA (6 missing pairs)
Missing: OBESE, ODDFELIX, OTTONI, OUTWIT, OVERBITE, OVERLAP

### OSPREY (6 missing pairs)
Missing: OBESE, ORCA, OUTWIT, OVERBITE, OVERLAP, OWLFISH

### OTTONI (6 missing pairs)
Missing: OBESE, OBSCURA, ORBITAL, ORIGAMI, OUROBOROS, OWLFISH

### OLE (5 missing pairs)
Missing: OBESE, ORGANON, OUROBOROS, OVERBITE, OWLFISH

### ORCA (5 missing pairs)
Missing: OBBLIGATO, OBLIQUE, OCTOPUS, OMBRE, OSPREY

### ORPHICA (5 missing pairs)
Missing: OBESE, ORGANON, OUROBOROS, OVERBITE, OWLFISH

### ODDOSCAR (4 missing pairs)
Missing: OBSIDIAN, ORACLE, ORBITAL, ORIGAMI

### OCEANIC (4 missing pairs)
Missing: OBESE, ORIGAMI, OUTWIT, OVERBITE

### OBLIQUE (4 missing pairs)
Missing: OBESE, ORCA, OVERBITE, OVERLAP

### OUROBOROS (4 missing pairs)
Missing: OBBLIGATO, OLE, ORPHICA, OTTONI

### OBSIDIAN (3 missing pairs)
Missing: ODDOSCAR, OUTWIT, OVERBITE

### OCELOT (3 missing pairs)
Missing: OBESE, OVERBITE, OVERLAP

### ODDFELIX (3 missing pairs)
Missing: OBSCURA, ORBITAL, ORIGAMI

### OHM (3 missing pairs)
Missing: OBESE, OVERBITE, OWLFISH

### OPTIC (3 missing pairs)
Missing: OBESE, OVERBITE, OVERLAP

### OCTOPUS (2 missing pairs)
Missing: OBBLIGATO, ORCA

### ODYSSEY (2 missing pairs)
Missing: ORBITAL, ORIGAMI

### ORACLE (2 missing pairs)
Missing: OBESE, ODDOSCAR

### ORGANON (2 missing pairs)
Missing: OLE, ORPHICA

### OSTERIA (2 missing pairs)
Missing: OUTWIT, OVERLAP

### OMBRE (2 missing pairs)
Missing: OBBLIGATO, ORCA

---

## 3. Top 50 Priority Pairs

Ranked by **anchor score** (sum of existing partner-coverage counts for both engines in the pair).
High-anchor pairs connect already-well-represented engines and are easiest to write convincingly.

| Rank | Pair | Anchor Score | Character Brief |
|---|---|---|---|
| 1 | ODDOSCAR ↔ ORACLE | 60 | Morphing pads under GENDY stochastic pitch events |
| 2 | OBSIDIAN ↔ ODDOSCAR | 59 | Crystal silence meets axolotl regeneration |
| 3 | OCTOPUS ↔ ORCA | 59 | Chromatophore texture under apex hunt pressure |
| 4 | OLE ↔ ORGANON | 59 | Afro-Latin drama driven by metabolic rhythm |
| 5 | OMBRE ↔ ORCA | 59 | Memory-forgetting blur against predator tension |
| 6 | ORGANON ↔ ORPHICA | 59 | Metabolic resonance feeding microsound harp |
| 7 | OBBLIGATO ↔ OCTOPUS | 58 | Dual wind obligato over eight decentralized arms |
| 8 | OBBLIGATO ↔ OMBRE | 58 | Wind bond through dual-narrative shadow |
| 9 | OBLIQUE ↔ ORCA | 57 | Prismatic bounce with apex-predator pulse |
| 10 | OBSCURA ↔ ODDFELIX | 57 | Daguerreotype filter over neon tetra flutter |
| 11 | ODYSSEY ↔ ORBITAL | 57 | Granular drift feeding warm-red arpeggio |
| 12 | OLE ↔ OUROBOROS | 57 | Hibiscus drama cycling through strange-attractor chaos |
| 13 | ORPHICA ↔ OUROBOROS | 57 | Siren harp caught in self-referential loop |
| 14 | OBBLIGATO ↔ OUROBOROS | 56 | Wind obligato entangled with leash chaos |
| 15 | ODDFELIX ↔ ORBITAL | 56 | Neon tetra colour against warm-red orbital sweep |
| 16 | ODYSSEY ↔ ORIGAMI | 56 | Granular wavetable drift folding through paper geometry |
| 17 | OHM ↔ OWLFISH | 56 | Sage commune hum beneath Mixtur-Trautonium abyssal tone |
| 18 | OTTONI ↔ OUROBOROS | 56 | Triple brass GROW macro cycling through attractor chaos |
| 19 | OBBLIGATO ↔ ORCA | 55 | Obligato wind motif over deep-ocean hunt |
| 20 | ODDFELIX ↔ ORIGAMI | 55 | Neon tetra flutter through vermillion fold geometry |
| 21 | ODDOSCAR ↔ ORBITAL | 55 | Axolotl regeneration feeding warm-red orbit |
| 22 | ORCA ↔ OSPREY | 55 | Apex predator and shore hunter sharing coastline |
| 23 | OSTERIA ↔ OUTWIT | 55 | Porto wine tavern melody driven by octopus CA arms |
| 24 | OBSCURA ↔ OTTONI | 54 | Daguerreotype tarnish beneath patina brass |
| 25 | OBSIDIAN ↔ OUTWIT | 54 | Crystal stillness broken by Wolfram CA eruptions |
| 26 | OCEANIC ↔ ORIGAMI | 54 | Phosphorescent depth zone folded through paper geometry |
| 27 | ODDOSCAR ↔ ORIGAMI | 54 | Axolotl bloom unfolded through vermillion paper |
| 28 | OLE ↔ OWLFISH | 54 | Hibiscus drama over abyssal gold resonance |
| 29 | ORPHICA ↔ OWLFISH | 54 | Microsound harp resonating against Mixtur-Trautonium |
| 30 | OSTERIA ↔ OVERLAP | 54 | Tavern ostinato threaded through lion's mane FDN |
| 31 | OCEANIC ↔ OUTWIT | 53 | Phosphorescent depth zone shaped by CA arm logic |
| 32 | OCELOT ↔ OVERLAP | 53 | Tawny biome colour filtered through knot-topology reverb |
| 33 | OPTIC ↔ OVERLAP | 53 | Visual phosphor pulse modulating lion's mane FDN |
| 34 | ORBITAL ↔ OTTONI | 53 | Warm-red arpeggio GROW overlaid with triple brass sweep |
| 35 | OSPREY ↔ OWLFISH | 53 | Shore hunter and deep-water resonator sharing abyssal edge |
| 36 | OTTONI ↔ OWLFISH | 53 | Patina brass GROW over Mixtur-Trautonium low register |
| 37 | OBLIQUE ↔ OVERLAP | 52 | Prismatic bounce diffused through FDN infinite sustain |
| 38 | ORIGAMI ↔ OTTONI | 52 | Paper fold geometry as brass articulation scaffold |
| 39 | OBSCURA ↔ OUTWIT | 51 | Daguerreotype stillness interrupted by eight-arm CA |
| 40 | OSPREY ↔ OUTWIT | 51 | Shore system coastline processed through Wolfram CA |
| 41 | OBSCURA ↔ OVERLAP | 50 | Daguerreotype silver diffused through knot-topology reverb |
| 42 | ORBITAL ↔ OUTWIT | 50 | Warm-red arpeggio shaped by CA generative rhythm |
| 43 | OSPREY ↔ OVERLAP | 50 | Shore hunter embedded in lion's mane FDN space |
| 44 | OUTWIT ↔ OWLFISH | 50 | Eight-arm CA logic driving Mixtur-Trautonium harmonic series |
| 45 | OVERLAP ↔ OWLFISH | 49 | Knot-topology FDN resonating with abyssal gold partials |
| 46 | OBESE ↔ ORACLE | 48 | Hot-pink saturation modulating GENDY stochastic melody |
| 47 | OBSIDIAN ↔ OVERBITE | 48 | Crystal silence shattered by fang aggression |
| 48 | OCELOT ↔ OVERBITE | 48 | Tawny biome pressure against BITE-axis character |
| 49 | OHM ↔ OVERBITE | 48 | Sage commune hum under possum PLAY DEAD |
| 50 | OPTIC ↔ OVERBITE | 48 | Phosphor visual pulse locking to fang snap envelope |

---

## 4. Suggested Tools — Three Waves to 100%

### Wave A — OBESE + OVERBITE Sweep (31 pairs, closable in one focused session)

These two engines account for 16 + 15 = 31 of the 77 uncovered pairs. A single targeted tool batch
closes the largest single gap in the matrix. Write one coupling tool per OBESE partner and one per
OVERBITE partner.

**OBESE tools** (16 presets, each a new pair):
- OBESE ↔ OBBLIGATO — fat saturation with obligato wind bond
- OBESE ↔ OBLIQUE — mojo modulation through prismatic bounce
- OBESE ↔ OBSCURA — hot-pink distortion under daguerreotype grain
- OBESE ↔ OCEANIC — phosphorescent bass depth zone
- OBESE ↔ OCELOT — biome texture with fat saturation floor
- OBESE ↔ OHM — sage commune hum over hot-pink sub
- OBESE ↔ OLE — Afro-Latin drive with fat mojo axis
- OBESE ↔ OPTIC — visual pulse shaping fat filter response
- OBESE ↔ ORACLE — GENDY stochastic melody over fat pad
- OBESE ↔ ORBITAL — warm-red arpeggio with fat sub support
- OBESE ↔ ORIGAMI — paper fold geometry through fat saturation
- OBESE ↔ ORPHICA — microsound harp over fat low register
- OBESE ↔ OSPREY — shore blend with fat bass anchor
- OBESE ↔ OTTONI — triple brass GROW with fat sub pulse
- OBESE ↔ OUTWIT — CA arm logic shaping fat filter depth
- OBESE ↔ OVERLAP — FDN reverb tail behind fat distortion

**OVERBITE tools** (15 presets, shared pairs already noted where overlap exists):
- OVERBITE ↔ OBBLIGATO — obligato wind over possum fang snap
- OVERBITE ↔ OBLIQUE — prismatic bounce against BITE-axis aggression
- OVERBITE ↔ OBSCURA — daguerreotype silver under fang release
- OVERBITE ↔ OBSIDIAN — crystal space shattered by OVERBITE attack
- OVERBITE ↔ OCEANIC — chromatophore depth with fang snap transient
- OVERBITE ↔ OCELOT — tawny biome and BITE axis
- OVERBITE ↔ OHM — sage commune stillness under possum PLAY DEAD
- OVERBITE ↔ OLE — hibiscus DRAMA macro vs fang character
- OVERBITE ↔ OPTIC — phosphor visual pulse with fang envelope shaping
- OVERBITE ↔ ORBITAL — warm-red arpeggio punctuated by BITE snap
- OVERBITE ↔ ORIGAMI — paper fold geometry under fang articulation
- OVERBITE ↔ ORPHICA — microsound harp pluck with OVERBITE snap transient
- OVERBITE ↔ OSPREY — shore blend with fang aggression
- OVERBITE ↔ OUTWIT — CA arms triggering BITE envelope events
- OVERBITE ↔ OVERLAP — FDN infinite sustain behind fang snap attack

**Wave A total: 31 new pairs → 515/561 (91.8%)**

---

### Wave B — OVERLAP + OUTWIT + OWLFISH Spine (22 pairs)

These three engines share deep thematic connections (topology, CA, abyssal resonance) and have
dense mutual gaps. A "spine" tool writes pairs across all three in a single coordinated pass.

**OVERLAP remaining** (after Wave A — 9 pairs):
- OVERLAP ↔ OBSCURA
- OVERLAP ↔ OCELOT
- OVERLAP ↔ OPTIC
- OVERLAP ↔ ORIGAMI
- OVERLAP ↔ OSPREY
- OVERLAP ↔ OSTERIA
- OVERLAP ↔ OWLFISH
- OVERLAP ↔ OBLIQUE
- OVERLAP ↔ OBESE (covered in Wave A)

**OUTWIT remaining** (after Wave A — 8 pairs):
- OUTWIT ↔ OBSCURA
- OUTWIT ↔ OBSIDIAN
- OUTWIT ↔ OCEANIC
- OUTWIT ↔ ORBITAL
- OUTWIT ↔ OSPREY
- OUTWIT ↔ OSTERIA
- OUTWIT ↔ OWLFISH
- OUTWIT ↔ OBESE (covered in Wave A)

**OWLFISH remaining** (7 pairs):
- OWLFISH ↔ OHM
- OWLFISH ↔ OLE
- OWLFISH ↔ ORPHICA
- OWLFISH ↔ OSPREY
- OWLFISH ↔ OTTONI
- OWLFISH ↔ OUTWIT (also counted above — 1 shared pair)
- OWLFISH ↔ OVERLAP (also counted above — 1 shared pair)

Netting out double-counted pairs: **22 net new pairs in Wave B → 537/561 (95.7%)**

---

### Wave C — Scatter Fill (24 remaining pairs)

After Waves A and B, 24 isolated pairs remain across: ORBITAL, ORIGAMI, OBBLIGATO, ORCA,
OUROBOROS, OCTOPUS, OMBRE, OLE, ORGANON, ORPHICA cross-pairs.

Group these into 4–5 thematic tools:

| Tool | Pairs Covered |
|---|---|
| Orbital/Origami cross-engine | ORBITAL ↔ {ODDFELIX, ODDOSCAR, ODYSSEY, OTTONI, OUTWIT} + ORIGAMI ↔ {OCEANIC, ODDFELIX, ODDOSCAR, ODYSSEY, OTTONI} = 10 pairs |
| Orca/Obbligato scatter | ORCA ↔ {OBBLIGATO, OBLIQUE, OCTOPUS, OMBRE, OSPREY} + OBBLIGATO ↔ {OCTOPUS, OMBRE, OUROBOROS} = 7 pairs |
| Ouroboros closes | OUROBOROS ↔ {OBBLIGATO, OLE, ORPHICA, OTTONI} = 4 pairs |
| Organon/Oracle closes | ORGANON ↔ {OLE, ORPHICA} + ORACLE ↔ {OBESE, ODDOSCAR} = 4 pairs (OBESE covered Wave A) |
| Obsidian/Obscura closes | OBSIDIAN ↔ {ODDOSCAR, OUTWIT} + OBSCURA ↔ {ODDFELIX, OUTWIT} (OUTWIT Wave B) = 3 pairs net |

**Wave C total: ~24 net pairs → 561/561 (100%)**

---

## 5. 50% Milestone — Historical Context

For reference, the 30% sprint doc (wave 71 baseline, 137/561) projected 5–6 wave batches to
reach 50%. That milestone (281 pairs) was long since passed. The current 86.3% represents
approximate wave 110–120 territory.

**Key milestones already cleared:**

| Milestone | Pairs | Status |
|---|---|---|
| 30% | 168 | Complete |
| 40% | 224 | Complete |
| 50% | 281 | Complete |
| 60% | 337 | Complete |
| 70% | 393 | Complete |
| 80% | 449 | Complete |
| 86.3% | 484 | Current state |
| 91.8% | 515 | After Wave A (OBESE + OVERBITE) |
| 95.7% | 537 | After Wave B (OVERLAP + OUTWIT + OWLFISH) |
| 100% | 561 | After Wave C (scatter fill) |

**Projected sessions to 100%:** 3 focused tool waves. Each wave is manageable in a single session.

---

## 6. Hard Ceiling Note

The 30% sprint doc noted a hard ceiling of 78.6% (441/561) due to 4 concept engines without DSP
(OSTINATO, OPENSKY, OCEANDEEP, OUIE). That ceiling no longer applies — the 34-engine roster
used here is the full registered and installed set. The ceiling is 100%.

The actual remaining constraint is creative, not architectural: OBESE and OVERBITE coupling presets
require careful parameter mapping because both engines have high-saturation character that can
overwhelm quieter partners. Each preset needs a gain-staged mix that lets both engines be audible.

---

## 7. Quality Gate

The 30% sprint doc recommended a human curation pass at 50% to tag stubs. That window has passed.
Recommended action: once 100% coverage is reached, run a single-session quality audit pass with
the following criteria per pair:

1. Both engines are audible in the mix (neither buried)
2. Coupling direction produces perceptible modulation response
3. All 4 macros produce audible change
4. Velocity shapes timbre (D001 compliance)
5. Preset has a unique, evocative name (no "STUB" suffix)

---

*Next action: run Wave A tools (OBESE sweep + OVERBITE sweep), validate new pair count via gap scan, then proceed to Wave B.*
