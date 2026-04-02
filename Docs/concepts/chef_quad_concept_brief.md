# Chef Quad — Concept Brief

**Status:** Early concept
**Theme:** Four world-class seafood chefs in competition — bespoke organs with boutique synth wildcards
**Naming convention:** Regionally relevant human O-names with double meanings

---

## The Quad

| Engine | Chef | Region | Boutique Wildcard | Quadrant | Prefix |
|--------|------|--------|-------------------|----------|--------|
| **XOto** | Oto (音, Japanese for "sound") | East Asia | Teenage Engineering | NE | `oto_` |
| **XOctave** | Octave (French given name + musical interval) | Western Europe | Arturia | SW | `oct_` |
| **XOleg** | Oleg (Latvian/Russian, means "holy, sacred") | Baltic/Eastern Europe | Erica Synths | NW | `oleg_` |
| **XOtis** | Otis (American — Otis Redding, soul, Hammond B3) | Americas | The Oddballs | SE | `otis_` |

## Each Chef's Identity

- **Oto** — sound itself, elemental. Disciplined minimalism.
- **Octave** — music theory, structural. Classically trained precision.
- **Oleg** — the sacred, spiritual. Holy organs and industrial edge.
- **Otis** — soul, emotional. Gospel fire and wildcard chaos.

## The 4×4×4 Combinatorial Engine

Each chef is a configurable engine with three independent axes:

```
         CHEF ENGINE
        /     |     \
    ORGAN    FX    WILDCARD
    (1of4) (1of4)  (1of4)
```

**Per chef:** 4 organs × 4 FX recipes × 4 wildcards = **64 configurations**
**Four chefs:** 256 total configurations before coupling

### Organ Territory (4 per region — LOCKED)

| Chef | Organ 1 | Organ 2 | Organ 3 | Organ 4 |
|------|---------|---------|---------|---------|
| **Oto** | Shō (ethereal cluster sustain, 11-note aitake) | Sheng (bright, articulate, melodic) | Khene (raw buzz-drone, rhythmic) | Melodica (breath-controlled, plastic-body resonance) |
| **Octave** | Cavaillé-Coll romantic pipe organ (dark, symphonic) | Baroque positiv organ (chiff transient, transparent) | French musette accordion (triple-reed beating) | Farfisa (transistor buzz, filtered squares) |
| **Oleg** | Bayan (powerful cassotto resonance, clean) | Hurdy-gurdy (melody + drone + buzz bridge) | Bandoneon (bisonoric push/pull, tango) | Garmon (raw Russian diatonic accordion) |
| **Otis** | Hammond B3 (additive + tonewheel crosstalk + Leslie) | Calliope (steam whistle, binary on/off, unhinged) | Blues harmonica (cross-harp bending, breath-driven) | Zydeco accordion (Louisiana raw, dance-driven) |

**Selection rationale (March 2026 research audit):**
- Zero overlap across all four chefs
- Each chef's four organs span a timbral spectrum (ethereal↔raw, precise↔chaotic)
- Melodica moved from Otis→Oto (Japanese-manufactured: Suzuki, Yamaha Pianica)
- Barrel organ dropped from Oleg (limited timbral interest), replaced with Bandoneon (German origin, Baltic/EE folk use)
- Pump organ removed from both Oleg and Otis (was duplicated), replaced with Garmon and Zydeco accordion
- Blues harmonica added to Otis (quintessentially American reed instrument)

### Shared Organ Parameter Vocabulary

Research across all 16 organs surfaced six synthesis parameters that recur across traditions. These form the **shared DSP vocabulary** — every chef's organ gets interpreted through these axes, weighted differently per instrument:

| Parameter | Source Instruments | Synthesis Role |
|-----------|-------------------|---------------|
| Cluster density | Shō aitake, Sheng parallel 5ths | How many voices stack in unison/near-unison |
| Chiff transient | Baroque organ onset burst | Brief harmonic burst before steady-state tone |
| Unison detune | Musette triple-reed interference | Beating from slightly detuned copies |
| Buzz intensity | Hurdy-gurdy trompette, Khene raw reed | Rattle/distortion mapped to velocity/pressure |
| Pressure instability | Calliope steam drift, bellows instruments | Irregular pitch/amplitude drift from air source |
| Crosstalk/leakage | Hammond tonewheel bleed | Adjacent voice bleed as deliberate warmth |

### Boutique Wildcard Groups (4 per region)

| Chef | Wildcard Influences |
|------|-------------------|
| **Oto** | OP-1, OP-XY, Riddim, Pocket Operators |
| **Octave** | MicroFreak, MiniBrute, MatrixBrute, BruteFactor |
| **Oleg** | Syntrx, Pērkons, Bullfrog, Black series |
| **Otis** | Telepathic, Critter & Guitari, Torso Electronics, OXI |

### FX Recipes (4 per region — TBD)

Each chef's signature processing chains — their "cuisine." To be designed.

## Competition Coupling

When two or more chefs occupy slots simultaneously, they **compete** rather than blend:
- Each chef brings their current loadout (organ + FX + wildcard)
- The output is the *tension* between them, not a cooperative merge
- Dominant signal depends on the matchup
- Chefs enter "warrior mode" — giving everything for their region

---

*Part of XOceanus by XO_OX Designs*
