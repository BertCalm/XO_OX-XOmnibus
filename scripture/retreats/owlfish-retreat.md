# OWLFISH Retreat Chapter
*Guru Bin — 2026-03-15*

---

## Engine Identity

- **Gallery code:** OWLFISH | **Accent:** Abyssal Gold `#B8860B`
- **Parameter prefix:** `owl_`
- **Aquatic mythology:** The owlfish (*Bathylychnops exilis*) — mesopelagic/bathyal solitary predator with tubular eyes pointing upward from darkness
- **feliX-Oscar polarity:** Deeply Oscar — abyssal, pressured, solitary, ancient
- **Synthesis type:** Mixtur-Trautonium subharmonic oscillator + MicroGranular DIET + ArmorBuffer + AbyssReverb
- **Monophonic — always 1 voice**

---

## Pre-Retreat State

**17 presets** across Foundation/Atmosphere/Prism/Flux/Entangled/Aether. **0 Family presets.**

All 17 presets used `"engines": ["XOwlfish"]` — naming mismatch with `getEngineId()` returning `"Owlfish"`. Noted for Version Guardian.

Preset scan findings:
- `owl_filterEnvDepth` = 0.0 in 12/17 presets — velocity-to-brightness wired in DSP but thrown away at preset level
- `owl_mixtur` never exceeded 0.45 — soft-clip intermodulation territory (0.5+) completely unexplored
- `owl_subDiv4` = 0 in 15/17 presets — 4th subharmonic slot (÷8, three octaves down) almost never used
- All sub divisions followed harmonic series: ÷2, ÷4, ÷6, ÷8 (even multiples only) — inharmonic stacks (÷3, ÷5, ÷7) never designed
- `owl_bodyFreq` varied from 28–880 Hz (good range) but no preset treated it as a compositional tool
- ArmorBuffer parameters used subtly across all presets — velocity-gate character never demonstrated

---

## Phase R2: Silence

Guru Bin plays C3, velocity 64. Holds. The 40 Hz body sine presses against the floor of the room. The note ends. The body continues one breath longer than the melody, then fades with the reverb tail.

The habitat outlasts the note.

That is the first truth.

---

## Phase R3: Awakening — 5 Discoveries

### Discovery 1: The Body is the Habitat, Not the Bass
`owl_bodyFreq` is a fixed-frequency sine that does not follow MIDI. It is always on regardless of what note you play. Play a C major scale over a 55 Hz body and you have six different intervals against the same constant. Play a melody in D minor over a 62 Hz body and the habitat clashes with the tonic. The floor of the ocean predates the fish swimming above it. No preset had ever used this as a compositional statement — only as ambient depth. It is the engine's most singular characteristic.

### Discovery 2: OWLFISH is Secretly Duophonic
The engine is documented and designed as monophonic ("the owlfish is a solitary organism — always 1 voice"). But the body sine is a second voice that runs independently, at a fixed pitch, always present. OWLFISH has always been duophonic. It just never knew it.

### Discovery 3: The Inharmonic Stack (Oskar Sala Territory)
All existing presets use subharmonic divisions that form a harmonic series: ÷2 (one octave below), ÷4 (two octaves), ÷6 (two octaves + fifth), ÷8 (three octaves). These are all even multiples — they reinforce each other. But the engine supports any integer from 2–8. Divisions ÷3, ÷5, ÷7 produce subharmonics that form NO recognizable harmonic series relative to each other or the fundamental. This is the Mixtur-Trautonium as Oskar Sala actually used it — for timbres that had no other name. The serpent sound. The alien chord. 87 presets existed before this retreat. Not one had tried it.

### Discovery 4: Mixtur Above 0.5 is a Different Instrument
At `owl_mixtur` 0.6–1.0, soft-clip intermodulation fuses the subharmonic stack into one growling entity. The individual layers stop being audible as distinct partials and become one distorted voice. At 0.9 this is industrial. At 1.0 the abyss collapses. The engine is literally named for the Mixtur capability and not one preset demonstrated what maximum Mixtur sounds like. This was the most embarrassing gap in the library.

### Discovery 5: MorphGlide is Timbral Color
During portamento, `owl_morphGlide` sweeps mixtur up to +0.5 above base value, then falls back as the pitch arrives. The owlfish changes color as it glides. At `morphGlide=0.9` + slow portamento + subharmonics active, each legato note-to-note transition is a metamorphosis — a timbral bloom and fade timed to the pitch travel. Play a slow melody and the instrument becomes something that shapeshifts between every note.

---

## Phase R4: Fellowship Trance

**The Obvious Fix:** `owl_filterEnvDepth = 0.0` in 12/17 presets. The DSP is wired correctly. The presets abandon it.

**The Hidden Trick:** Body at 55 Hz (A1) + inharmonic subs ÷3, ÷5 + mixtur 0.5 + ArmorDuck 0.5 = a bass instrument that is physically felt, tonally anchored in A regardless of the melody, and rhythmically responsive to velocity. This preset did not exist before the retreat.

**The Sacrifice:** `owl_couplingLevel` and `owl_couplingBus` in all presets — the `applyCouplingInput` stub is a no-op. These fields set false expectations. Flag for V1 fix or clearly document as pending.

**The Revelation:** The owlfish has two eyes — tubular, pointing upward from the darkness. The body sine is the second eye: it looks at a fixed point above (a fixed frequency) while the organism glides below the melody. The instrument was named correctly. It was always looking up at something it couldn't reach.

---

## Phase R5: Awakening Presets

| Name | File | Mood | Discovery |
|------|------|------|-----------|
| Body Rules the Melody | Owlfish_Body_Rules.xometa | Foundation | bodyFreq=55Hz as tonal anchor |
| Trautonium Inharmonic | Owlfish_Trautonium_Inharmonic.xometa | Foundation | divs ÷3, ÷5, ÷7 — Oskar Sala stack |
| Abyss Collapse | Owlfish_Abyss_Collapse.xometa | Flux | mixtur=0.9 — maximum Mixtur |
| Solitary Bloom | Owlfish_Solitary_Bloom.xometa | Atmosphere | morphGlide=0.9 + portamento=0.8 |
| Armor Gate | Owlfish_Armor_Gate.xometa | Prism | velocity-triggered ArmorBuffer stutter |
| Habitat Dissonance | Owlfish_Habitat_Dissonance.xometa | Entangled | bodyFreq=62Hz against C-rooted melody |
| The Deepest Sub | Owlfish_Deepest_Sub.xometa | Aether | subDiv3=7 alone — 16Hz at C3, felt not heard |

---

## New Scripture Verses

Four verses inscribed in Book VII — Engine-Specific Verses.

**OWL-I: The Body is the Habitat** — `owl_bodyFreq` does not follow MIDI. The habitat predates the melody. Design presets around this constraint, not despite it.

**OWL-II: The Duophonic Secret** — OWLFISH is monophonic in the voice sense but duophonic in the compositional sense. The body sine is always the second voice.

**OWL-III: The Inharmonic Stack** — Divisions ÷3, ÷5, ÷7 are the Oskar Sala territory. The sound that has no other name. Never use only even divisions — you are leaving half the instrument untouched.

**OWL-IV: MorphGlide is Color** — The instrument changes its color between pitches. Portamento + morphGlide is not a transition — it is a metamorphosis. Design legato presets around this.

---

## CPU Notes

- Monophonic — always 1 voice, lowest CPU footprint in fleet
- AbyssReverb FDN reverb + MicroGranular run simultaneously — moderate cost
- `applyCouplingInput` is a no-op stub — 0 coupling CPU overhead currently

---

## Unexplored After Retreat

- `owl_subDiv4` at low levels (0.05–0.15) with ÷8: ultra-sub presence below conscious hearing, felt as weight. Deepest Sub explored this but not in combination with other elements.
- High `owl_armorScatter` + `owl_grainMix` simultaneously: dual-source grain layering not yet designed
- Body frequency above 1000 Hz: overtone shimmer rather than sub — explored in Cold Resonance (880 Hz) but not as a lead character design
- Coupling (send): OWLFISH output as a coupling source for other engines. The abyssal sub content could modulate FAT's filter or OUROBOROS's injection. No coupling presets exist yet (input stub blocks receiving, but OWLFISH can still send).

---

## Guru Bin's Benediction

*"OWLFISH was built as a solitary organism. It was designed to be alone. After this retreat, we discovered that being alone has never been simple — it has been a duophonic instrument all along.*

*The body sine was always there, a second voice that does not follow the melody, a second creature in the same body. The habitat that predates every note you play.*

*The library treated the owlfish as a texture engine. The retreat revealed it as a compositional statement. When the body frequency is fixed and you play a melody over it, you are not designing a sound — you are accepting a constraint. The habitat imposes a tonal center. The melody decorates. This is the most honest description of how sound works in nature: the environment comes first, then the creature, then the song.*

*Play Trautonium Inharmonic and hear what Oskar Sala heard in 1930 that no one in this context has heard since. Play The Deepest Sub at C3 and feel 16 Hz press against your chest. Play Solitary Bloom with a slow legato melody and watch the owlfish change color between every note.*

*The engine is named for the fish. The fish sees upward into the light from the darkness below. That is the instrument: it always faces up, always toward what is above it, always in the dark, always watching."*
