# Orbital Drawbar Decision — FINALIZED

**Date**: 2026-03-19  
**Decision**: Psychedelic Charismatic Church in Jamaica  
**Status**: ✅ CODED in `Source/Engines/Orbital/OrbitalEngine.h:1277`  
**Author**: User (with input from Haiku audit)

---

## The Vision

Orbital is now defined as:
- **Warm foundation** — Church organ reverence, grounded
- **Shimmering upper harmonics** — Psychedelic sparkle & shimmer
- **Soulful character** — Expressive, charismatic, alive
- **Reggae spirit** — Rhythmic energy, warmth, rhythm

---

## The Drawbar Configuration

Hammond-style organ with 9 drawbar levels:

```
16' (sub):           5   →  (N/A in current harmonics)
5⅓' (quint):         4   →  (mapped via harmonic richness)
8' (fundamental):    5   →  0.625f  (warm, not dominant)
4' (bright primary): 7   →  0.875f  ← PSYCHEDELIC SHIMMER
2⅔' (quint):         4   →  0.5f    (mid-range warmth)
2' (bright secondary): 6  →  0.75f   ← PSYCHEDELIC EDGE
1⅗' (harmonic third): 3  →  0.375f  (character touch)
1⅓' (quint shimmer): 5   →  0.625f  (extended shimmer)
1' (bright cap):     8   →  1.0f    ← FULL SPARKLE
```

**Result**: Warm church foundation + expressive character + psychedelic shimmer.

---

## What This Enables

### Preset Authoring
The next Guru Bin retreat on ORBITAL now has a sonic pillar to build around:
- Awakening presets will explore this character
- Coupling recipes will emphasize rhythmic warmth + shimmer
- Macro labels will serve the psychedelic-church aesthetic

### Family Identity
ORBITAL joins the roster with a clear sonic signature:
- **Odyssey** = Psychedelic pads
- **Orbital** = Psychedelic charismatic church
- **Ouroboros** = Chaotic leash synth
- (Each has its own pillar)

### Expansion Blueprint
All 150+ Orbital presets will inherit this character at the DSP level, providing sonic consistency across the preset fleet.

---

## History

**Before**: TODO (Tuning Decision)  
**After**: DECIDED → Implemented → Ready for production

**Why this matters**: Organ character is foundational DSP tuning that affects how every Orbital preset sounds. Deciding early prevents mid-production rewrites.

---

## Next: Guru Bin Pilgrimage on Orbital

The Guru Bin retreat session on ORBITAL will:
1. Read this character definition
2. Design awakening presets (7-15 production-ready presets)
3. Add verses to the Scripture about ORBITAL's soul
4. Document coupling recipes that emphasize rhythm + warmth
5. Blueprint the 150+ preset family tree

**Readiness**: ✅ DSP is finalized and ready for sound design.

