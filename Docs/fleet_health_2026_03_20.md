# XOmnibus Fleet Health Dashboard

**Generated**: 2026-03-20
**Scope**: All 42 engine directories in `Source/Engines/`

---

## Engine Health Table

Sorted by preset count ascending (weakest coverage first).

| Engine (Gallery) | Source Dir | Source Lines | Presets | Seance | Registered |
|---|---|---:|---:|---|---|
| Orbweave | Orbweave | 1,126 | 0 | Yes | Yes |
| Ostinato | Ostinato | 2,212 | 172 | Yes | Yes |
| Obrix | Obrix | 1,425 | 187 | Yes | Yes |
| OceanDeep | OceanDeep | 884 | 280 | Yes | Yes |
| Overtone | Overtone | 889 | 296 | Yes | Yes |
| Organism | Organism | 954 | 324 | Yes | Yes |
| Owlfish | Owlfish | 2,353 | 351 | No | Yes |
| OpenSky | OpenSky | 1,542 | 354 | Yes | Yes |
| Ouie | Ouie | 2,028 | 369 | Yes | Yes |
| Octopus | Octopus | 1,572 | 378 | Yes | Yes |
| Ocelot | Ocelot | 3,180 | 413 | No | Yes |
| Orca | Orca | 1,463 | 421 | Yes | Yes |
| Ombre | Ombre | 946 | 428 | Yes | Yes |
| Bite (Overbite) | Bite | 2,462 | 442 | No | Yes |
| Osteria | Osteria | 1,975 | 449 | No | Yes |
| Osprey | Osprey | 2,197 | 453 | No | Yes |
| Oceanic | Oceanic | 1,473 | 486 | No | Yes |
| Obscura | Obscura | 1,925 | 494 | No | Yes |
| Obsidian | Obsidian | 1,651 | 517 | No | Yes |
| Ole | Ole | 378 | 517 | No | Yes |
| Ottoni | Ottoni | 464 | 522 | No | Yes |
| Orphica | Orphica | 626 | 547 | No | Yes |
| Obbligato | Obbligato | 508 | 548 | No | Yes |
| Orbital | Orbital | 1,633 | 552 | No | Yes |
| Ohm | Ohm | 591 | 577 | No | Yes |
| Overworld | Overworld | 1,303 | 577 | No | Yes |
| Optic | Optic | 1,033 | 600 | No | Yes |
| Ouroboros | Ouroboros | 1,551 | 623 | No | Yes |
| Oblique | Oblique | 1,695 | 624 | No | Yes |
| Onset | Onset | 2,211 | 638 | No | Yes |
| Obese | Fat | 1,572 | 642 | No | Yes |
| Origami | Origami | 1,960 | 657 | No | Yes |
| Overlap | Overlap | 2,065 | 660 | Yes | Yes |
| Opal | Opal | 3,337 | 672 | No | Yes |
| Oracle | Oracle | 1,837 | 672 | No | Yes |
| Outwit | Outwit | 1,879 | 676 | Yes | Yes |
| Overdub | Dub | 1,362 | 686 | No | Yes |
| OddOscar | Morph | 1,247 | 727 | No | Yes |
| Organon | Organon | 1,523 | 756 | No | Yes |
| OddfeliX | Snap | 1,071 | 792 | No | Yes |
| Oblong | Bob | 1,819 | 916 | No | Yes |
| Odyssey | Drift | 1,846 | 923 | No | Yes |

---

## Summary

| Metric | Value |
|---|---|
| **Total Engines** | 42 |
| **Total Presets** | 21,918 |
| **Total Source Lines** | 65,768 |
| **Registered Engines** | 42 / 42 (100%) |
| **Seance Verdicts** | 13 / 42 (31.0%) |
| **Engines with All 4 Checks** | 11 / 42 (26.2%) |

### Engines Below 100 Presets (Flagged for Expansion)

| Engine | Presets | Status |
|---|---:|---|
| **Orbweave** | 0 | CRITICAL -- zero presets, needs immediate attention |

### Engines Missing Seance Verdicts (29)

Owlfish, Ocelot, Bite, Osteria, Osprey, Oceanic, Obscura, Obsidian, Ole, Ottoni, Orphica, Obbligato, Orbital, Ohm, Overworld, Optic, Ouroboros, Oblique, Onset, Obese, Origami, Opal, Oracle, Overdub, OddOscar, Organon, OddfeliX, Oblong, Odyssey

### Overall Fleet Health Score

**26.2%** -- 11 of 42 engines have all four checkmarks (source lines > 0, presets >= 100, seance verdict, registered).

The primary gap is **seance coverage**: 29 engines have never been through a synth seance. All 42 engines are registered and have source code. Only Orbweave lacks presets entirely.

---

## Health Criteria

- **Source Lines**: Engine has DSP source code in `Source/Engines/`
- **Presets**: Count of `.xometa` and `.json` preset files referencing the engine (from `"engines"` or `"engine"` field)
- **Seance**: Verdict file exists in `Docs/seances/`
- **Registered**: `static bool registered_` entry in `Source/XOmnibusProcessor.cpp`
