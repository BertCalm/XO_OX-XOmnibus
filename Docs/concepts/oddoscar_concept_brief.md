# OddOscar — Concept Brief (Retrospective)

*Retrospective Phase 0 | March 2026 | XO_OX Designs*
*Enriching the first generation with the aquatic mythology*

---

## Identity

**XO Name:** OddOscar
**Gallery Code:** ODDOSCAR
**Accent Color:** Axolotl Gill Pink `#E8839B`
**Parameter Prefix:** `morph_`
**Engine Dir:** `Source/Engines/OddOscar/`

**Thesis:** Lush pad synthesis with continuous wavetable morphing -- the patient, breathing warmth of the reef.

---

## Aquatic Identity

Oscar himself. The axolotl -- pink gills breathing slowly in a coral cave, regenerating, evolving, never rushing. He is the O in XO_OX, the responder, the sustainer, the warm body of the water column that holds everything else together. Where feliX darts and vanishes, Oscar remains. Where feliX is a flash, Oscar is a tide.

His wavetable morph (Sine at 0.0, Saw at 1.0, Square at 2.0, Noise at 3.0) mirrors the axolotl's metamorphic ability -- the same creature, continuously transforming without ever losing its essential nature. A sine wave is the axolotl at rest, soft pink gills barely moving. A saw wave is the axolotl mid-regeneration, bristling with new tissue. A square wave is the axolotl in full display, bold and defined. Noise is the axolotl dissolving into the reef itself, becoming indistinguishable from the living coral. The morph parameter is metamorphosis made audible.

The Perlin noise drift on his filter is his breathing -- never quite the same cycle twice, never mechanical, always organic. The 4-pole Moog ladder filter is the warmth of the reef: resonant, self-oscillating when pushed, capable of singing on its own. Three detuned oscillators create the chorus width of a coral colony -- many identical polyps, slightly out of phase, producing a collective sound richer than any individual. The sub oscillator is Oscar's heartbeat, one octave below, the fundamental pulse that anchors the reef to the ocean floor.

---

## Polarity

**Position:** The Reef -- Oscar's home territory, where life is densest
**feliX-Oscar balance:** 0/100 -- The Reef (Oscar's home)

---

## DSP Architecture (As Built)

The MorphEngine is a 16-voice pad synthesizer built around a custom wavetable morph oscillator, a 4-pole Moog ladder filter, and Perlin noise drift modulation.

**Signal flow per voice:**

```
3x MorphOscillator (detuned, morph position 0.0-3.0)
  + Sub Oscillator (PolyBLEP sine, -1 octave)
    |
    v
4-pole Moog Ladder Filter (non-linear, tanh saturation per stage)
    |
    v
ADSR Envelope ("Bloom" attack control)
    |
    v
Drift-based Stereo Spread
    |
    v
Soft Clip (tanh) -> Output
```

**MorphOscillator:**
- 2048-sample wavetables, pre-built at construction: Sine, Saw, Square.
- Morph parameter (0.0-3.0) crossfades between four timbres: Sine(0) -> Saw(1) -> Square(2) -> Noise(3).
- Linear interpolation between adjacent tables, with noise crossfaded via RNG at the Square->Noise boundary.
- Phase-coherent -- all three detuned oscillators share the same morph position.

**MoogLadder filter:**
- 4-pole non-linear ladder with tanh saturation per stage.
- Self-oscillating at high resonance.
- Denormal protection in all 4 feedback stages.
- Coupling-responsive: AmpToFilter modulates cutoff (inverted "dub pump" effect), EnvToMorph shifts morph position from external envelopes.

**Perlin noise drift:**
- Hash-based smooth noise function generating slow, organic movement.
- Drives stereo spread (left/right balance shifts with drift value) and subtle pitch FM.
- Drift amount parameter scales the effect from imperceptible to obvious wandering.

**Key DSP details:**
- "Bloom" attack: the attack parameter is labeled Bloom, reflecting the slow opening of a pad. Range 0.001s to 10s.
- Full ADSR with sustain pedal (CC64) and mod wheel (CC1 sweeps morph position 0-3).
- 3 detuned oscillators create inherent chorus width -- detune range 0-50 cents.
- Soft clip (tanh) on the stereo output prevents hard clipping in dense pad stacking.
- 16-voice polyphony (configurable 1/2/4/8/16) with LRU voice stealing and 5ms crossfade.
- Internal LFO (0.3 Hz sine) available as coupling output on channel 2, providing slow modulation to other engines.

**Parameters (12 total):** `morph_morph`, `morph_bloom`, `morph_decay`, `morph_sustain`, `morph_release`, `morph_filterCutoff`, `morph_filterReso`, `morph_drift`, `morph_sub`, `morph_detune`, `morph_level`, `morph_polyphony`.

---

## Signature Sound

OddOscar sounds like warmth given voice. The Moog ladder filter gives every patch a liquid, resonant quality that no digital SVF can replicate -- it sings under the note, adding harmonics that feel grown rather than generated. The three detuned morph oscillators create a chorus that breathes on its own, and the Perlin drift ensures that no two cycles of a held chord are identical. At low morph positions, Oscar is pure, warm, oceanic. At high morph positions, he becomes textured, noisy, alive with the complexity of a living reef. He is the pad engine that other pads wish they were.

---

## Coupling Thesis

Oscar gives slow modulation. His 0.3 Hz internal LFO breathes like gills -- a sine wave so slow it feels tidal rather than rhythmic. This LFO, exposed on coupling channel 2, drives LFOToPitch on other engines, making their output wander gently. feliX receiving Oscar's LFO becomes a neon tetra caught in a slow current -- still darting, but with a drift that feels natural.

He receives energy from outside. AmpToFilter coupling from feliX or Onset creates an inverted "dub pump" on his filter cutoff -- when the transient hits, Oscar's filter ducks and then blooms back open. EnvToMorph coupling from external envelopes shifts his wavetable position, meaning other engines can literally change what Oscar sounds like. He absorbs the energy of other species and transforms it into warmth. He is the responder. He sustains.

---

## Preset Affinity

| Foundation | Atmosphere | Entangled | Prism | Flux | Aether |
|-----------|-----------|-----------|-------|------|--------|
| High | High | High | Medium | Medium | High |

---

*XO_OX Designs | OddOscar -- pink gills breathing in a coral cave, the same creature always transforming*
