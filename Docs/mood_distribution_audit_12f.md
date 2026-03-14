# XOmnibus Prism Sweep 12F — Mood Distribution Audit

**Date**: 2026-03-14
**Round**: 12F
**Total presets audited**: 1,685 (post-gap-fill)
**Moods**: Foundation, Atmosphere, Entangled, Prism, Flux, Aether

---

## Overall Mood Distribution

| Mood        | Count | Percentage |
|-------------|-------|-----------|
| Foundation  |   289 |     17.2% |
| Atmosphere  |   297 |     17.6% |
| Entangled   |   349 |     20.7% |
| Prism       |   327 |     19.4% |
| Flux        |   234 |     13.9% |
| Aether      |   189 |     11.2% |
| **TOTAL**   | **1,685** | **100%** |

**Observation**: The distribution is healthy at the top (Foundation/Atmosphere/Entangled/Prism all near 17–21%), with Flux and Aether intentionally thinner — Flux is performance-driven and Aether is drone/ambient, naturally representing a smaller library slice. No concern.

---

## Per-Engine Mood Distribution Table

| Engine     | Foundation | Atmosphere | Entangled | Prism | Flux | Aether | Total |
|------------|-----------|-----------|---------|-------|------|--------|-------|
| Obese      |    23     |    18     |    52   |  20   |  26  |   22   |  161  |
| Oblique    |     4     |     3     |     3   |  11   |   3  |    2   |   26  |
| Oblong     |    37     |    54     |    98   |  68   |  64  |   36   |  357  |
| Obscura    |     5     |     6     |     1   |   1   |   2  |    1   |   16  |
| Obsidian   |     2     |     2     |     1   |   2   |   1  |    2   |   10  |
| OddOscar   |    18     |    21     |    54   |  24   |  26  |   25   |  168  |
| OddfeliX   |    43     |    40     |    94   |  23   |  16  |   31   |  247  |
| Odyssey    |    63     |    97     |    61   | 109   |  23  |   33   |  386  |
| Onset      |     2     |     2     |    20   |   2   |   3  |    2   |   31  |
| Opal       |    19     |    21     |    24   |  20   |  17  |   18   |  119  |
| Optic      |     2     |     3     |     2   |   4   |   7  |    3   |   21  |
| Oracle     |     2     |     2     |     2   |   3   |   2  |    2   |   13  |
| Organon    |    26     |    29     |    40   |  20   |  18  |   16   |  149  |
| Osprey     |     3     |     3     |     2   |   2   |   2  |    2   |   14  |
| Osteria    |     2     |     2     |     2   |   2   |   3  |    2   |   13  |
| Ouroboros  |     7     |     6     |    30   |  11   |  12  |   13   |   79  |
| Overbite   |    12     |     7     |    19   |  10   |  16  |    7   |   71  |
| Overdub    |    35     |    20     |    58   |  32   |  32  |   14   |  191  |
| Overworld  |     9     |     9     |    18   |  10   |  10  |    7   |   63  |
| XOwlfish   |     6     |     3     |     2   |   2   |   2  |    2   |   17  |

---

## Engines with Skewed Distributions (Pre-Gap-Fill)

### Fully Missing Moods (0 presets)

| Engine  | Missing Moods              | Severity |
|---------|---------------------------|----------|
| Obscura | Entangled, Prism, Aether  | HIGH     |

### Low Representation (1 preset — effectively no presence)

| Engine   | Low Moods                        | Notes                                 |
|----------|----------------------------------|---------------------------------------|
| Oracle   | Entangled, Aether                | Small engine, needed foothold         |
| Osteria  | Entangled, Aether                | Small engine, needed foothold         |
| Osprey   | Aether                           | Missing ambient presence              |
| Onset    | Aether                           | Drum engine — sparse ambient expected |
| Obsidian | Entangled, Flux                  | Very thin library overall             |

### Entangled-Dominant (Onset)

| Engine | Dominant Mood | Count | % of Engine Total |
|--------|--------------|-------|-------------------|
| Onset  | Entangled    |  20   |       65%         |

This is expected and acceptable: Onset's primary value is in cross-engine drum coupling presets, which by definition land in Entangled. Its Foundation/Atmosphere/Prism/Flux/Aether presence (2 each) serves as a foothold, not a full tier.

---

## Gap-Fill Presets Written (12F)

8 presets written to close the identified gaps. All use accurate parameter schemas confirmed against existing library entries.

### Obscura — 3 presets (previously missing Entangled, Prism, Aether entirely)

| File | Mood | Concept |
|------|------|---------|
| `Entangled/Obscura_Chain_Tangle.xometa` | Entangled | OBSCURA physical chain couples to OddOscar MORPH — bowed resonances tangle with spectral blur |
| `Prism/Obscura_Modal_Refraction.xometa` | Prism | High stiffness, low damping — standing-wave modal peaks scatter bright inharmonic partials |
| `Aether/Obscura_Suspended_Filament.xometa` | Aether | Near-zero damping, very slow bowing force — chain hangs in near-vacuum, free boundary |

### Onset — 1 preset (Aether foothold)

| File | Mood | Concept |
|------|------|---------|
| `Aether/Onset_Ghost_Kit.xometa` | Aether | Ultra-slow decays on all 8 voices, extreme low levels — suggestion of drums, not drums |

### Osprey — 1 preset (Aether foothold)

| File | Mood | Concept |
|------|------|---------|
| `Aether/Osprey_Fog_Bank.xometa` | Aether | Near-zero seaState, maximum fog and harborVerb — the ocean disappears into itself |

### Osteria — 2 presets (Entangled and Aether footholds)

| File | Mood | Concept |
|------|------|---------|
| `Entangled/Osteria_Session_Drift.xometa` | Entangled | Tavern session couples to Odyssey drift — real conversation becoming a memory |
| `Aether/Osteria_Last_Call.xometa` | Aether | Maximum qMemory, minimum melody/rhythm — final chord dissolving into night sea |

### Oracle — 2 presets (Entangled and Aether footholds)

| File | Mood | Concept |
|------|------|---------|
| `Entangled/Oracle_Stochastic_Mesh.xometa` | Entangled | Oracle probability barriers re-gate OddfeliX density — stochastic and textural, entwined |
| `Aether/Oracle_Infinite_Barrier.xometa` | Aether | Maximum elasticity, minimum gravity — barriers never collapse, prophecy that never arrives |

---

## Post-Gap-Fill Gap Analysis

**Zero-count gaps remaining**: NONE — all 20 engines now have at least 1 preset in every mood.

**Remaining distribution concerns (advisory only)**:

| Engine   | Thin Moods (1–2 presets) | Action Needed |
|----------|--------------------------|---------------|
| Obscura  | Entangled(1), Prism(1), Aether(1) | Foothold established — expand in future round |
| Oracle   | All moods 2–3 each | Engine is new/small — acceptable |
| Osteria  | Most moods 2–3 each | Engine is new/small — acceptable |
| Osprey   | Entangled(2), Prism(2), Flux(2), Aether(2) | Even but thin — expand in future round |
| Obsidian | All moods 1–2 each | Niche engine — monitor |

---

## Structural Notes

- **Entangled** is the deepest mood (349 presets, 20.7%) — this is structurally sound. Multi-engine coupling presets are the library's richest creative zone.
- **Aether** at 189 (11.2%) is the thinnest mood. This is intentional — drone/ambient content is a narrower slice of the library. Consider a dedicated Aether expansion round if the catalog grows past 2,000 total.
- **Odyssey (Drift)** is the single most represented engine (386 presets, 22.9% of all) with a heavy Prism skew (109 presets = 28% of Odyssey). This reflects Drift's identity as a pad/spectral engine — acceptable, but a Flux expansion for Odyssey would improve balance.
- **Oblong (Bob)** second-largest (357 presets) with Entangled-heavy distribution (98 = 27%) — similarly appropriate to Bob's coupling-friendly identity.

---

## Next Recommended Actions

1. **Obscura Prism/Entangled/Aether expansion** — each has only 1 foothold; add 3–5 more in a focused Obscura expansion round
2. **Odyssey Flux expansion** — currently 23 Flux presets vs 109 Prism; add 10–15 performance-motion presets for better balance
3. **OddfeliX Prism** — 23 presets vs 94 Entangled; add solo-texture Prism presets that lean into FAT's spectral character
4. **Osprey/Osteria/Oracle full-tier expansion** — all three are sub-20 total presets; a dedicated small-engine expansion round would establish proper representation
5. **Aether-focused round** — if total library crosses 2,000, dedicate a sweep to bringing Aether to 15% or higher

