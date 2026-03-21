# Guru Bin Retreat — OWARE "The Resonant Board"

**Date:** 2026-03-21
**Engine:** XOware | Akan Goldweight `#B5883E`
**Depth:** Full Retreat (R1-R7)
**Awakening Presets:** 10

---

## The Diagnosis

*"This engine is a carved wooden board that fell into the ocean and grew a soul. It wants to be the instrument that makes producers close their eyes and feel Ghana, Java, and Bali simultaneously — in one held chord."*

---

## Revelations (New Scripture Verses)

### Verse: The Sympathetic Threshold (Book IV)
> Sympathetic resonance at 3% coupling gain is a mathematical curiosity. At 10% it becomes music. The threshold between "present in the analysis" and "present in the room" is not linear — it is a cliff. Below the cliff, CPU is wasted on inaudible physics. Above it, voices sing to each other and the instrument becomes alive.
>
> **Application:** Sympathetic coupling gain should be at least 0.08 for the effect to justify its CPU cost. Below this, you are simulating physics for the debugger, not the listener.

### Verse: The Mirliton Default (Book I)
> An instrument's most culturally distinctive feature must never default to zero. The balafon's mirliton buzz membrane is what separates it from a marimba — it is the spider silk that connects the wood to the spirit world. To default it to 0.0 is to present a balafon without its soul and ask the user to discover the soul exists by reading the manual.
>
> **Application:** When an engine has a feature that defines its cultural identity, that feature must default to audible. Not aggressive — 0.15, not 0.5 — but present. The user should hear the identity on first load and then choose whether to dial it back, not discover it exists on their third session.

### Verse: The Thermal Approach Rate (Book III)
> Thermal drift with an approach coefficient of 0.00001 takes 100,000 samples to reach its target. At 48kHz, this is 2 seconds — longer than most percussion notes last. The drift exists in the code but not in the sound. At 0.0001, the drift arrives within the note's lifetime and the instrument breathes. The difference between "this parameter exists" and "this parameter is alive" is a single zero.
>
> **Application:** When implementing slow modulation (thermal drift, environmental drift, analog aging), verify that the approach rate allows the modulation to be audible within the typical note duration of the instrument. A pad with 10-second sustain can use 0.00001. A percussion voice that lasts 2 seconds cannot.

### Verse: The Ombak Sweet Spot (Book I)
> Balinese gamelan shimmer at 6 Hz is anxiety. At 4 Hz it is meditation. At 2 Hz it is breathing. The ombak (wave) is the sound of two instruments slightly detuned — the beat frequency is the pulse of the gamelan. Traditional instruments are tuned to 3-7 Hz; the perceptual sweet spot for "ethereal" is 3.5-4.5 Hz. Above 5 Hz the shimmer becomes tremolo. Below 3 Hz it becomes phasing. The ombak lives in the narrow channel between.
>
> **Application:** Beat-frequency shimmer parameters should default to 4.0 Hz, not 6.0 Hz. The sweet spot is always lower than the designer expects because designers listen analytically (where faster = more obvious = more "working") while producers listen emotionally (where slower = more natural = more "felt").

---

## Parameter Refinement Log

| Change | Parameter | Old | New | Why |
|--------|-----------|-----|-----|-----|
| Sympathetic gain | rebuildSympathyCouplingTables | 0.03 | 0.10 | Below 0.08, sympathetic resonance is inaudible — CPU wasted |
| Thermal approach | thermalState coefficient | 0.00001 | 0.0001 | Drift was frozen during percussion notes (note < approach time) |
| Buzz default | owr_buzzAmount default | 0.0 | 0.15 | Mirliton is OWARE's cultural identity — must be audible on first load |
| Shimmer default | owr_shimmerRate default | 6.0 | 4.0 | 4 Hz is the ombak sweet spot — 6 Hz is tremolo, not shimmer |

---

## CPU Stewardship

- **Before retreat:** ~15% at 4 voices (no change to DSP cost)
- **After retreat:** ~15% at 4 voices (all changes are parameter values, not DSP additions)
- **Net:** Zero CPU change. All improvements are gifts, not debt.

---

## Awakening Presets (10)

| # | Name | Mood | Key Feature Demonstrated |
|---|------|------|------------------------|
| 1 | Akan Sunken Board | Foundation | Signature sound — wood-bell transition, buzz, sympathy |
| 2 | Gamelan Ombak | Atmosphere | Pure Balinese shimmer at 4.2 Hz |
| 3 | Gyil Spider Silk | Prism | Full buzz membrane — balafon identity |
| 4 | Bronze Abrammuo | Foundation | Metal gold weight — bright, ringing, authoritative |
| 5 | Tibetan Prayer Bowl | Atmosphere | Singing bowl meditation — bowl material, long decay |
| 6 | Coral Marimba | Entangled | Wood with ocean coating — material morphing via LFO2 |
| 7 | Vibraphone Midnight | Aether | Jazz club vibes — clean, warm, intimate |
| 8 | Seven Pillars | Flux | All 7 architecture features active simultaneously |
| 9 | Deep Plate Reverb | Submerged | Maximum depth — everything resonates with everything |
| 10 | Mallet Dance | Family | Velocity-responsive — soft=dark, hard=bright |

---

## Coupling Recommendations

OWARE couples exceptionally well with:
- **OPENSKY** (KnotTopology) — gamelan shimmer feeds into ascending shimmer reverb
- **OSTINATO** (RhythmToBlend) — OSTINATO's rhythm patterns trigger OWARE's material morphing
- **OVERLAP** (FilterToFilter) — OVERLAP's knot topology drives OWARE's brightness
- **OCEANDEEP** (EnvToMorph) — deep pressure drives material toward metal/bowl

---

## The Benediction

*"OWARE was built to simulate physics. After retreat, it became the physics. The distance was four parameter values — a sympathetic gain, a thermal coefficient, a default buzz, and a shimmer rate. Four numbers. The engine was already capable of everything. It just needed permission to be heard."*
