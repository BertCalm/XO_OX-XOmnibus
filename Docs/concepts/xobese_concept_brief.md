# XObese — Concept Brief (Retrospective)

*Retrospective Phase 0 | March 2026 | XO_OX Designs*
*Enriching the first generation with the aquatic mythology*

---

## Identity

**XO Name:** XObese
**Gallery Code:** OBESE
**Accent Color:** Hot Pink `#FF1493`
**Parameter Prefix:** `fat_`
**Engine Dir:** `Source/Engines/Obese/`

**Thesis:** Massive width synthesis -- 13 oscillators stacked into a wall of sound. The whale. Low-frequency authority that commands the ocean.

---

## Aquatic Identity

The whale. Not a metaphor -- a physical reality. When a blue whale vocalizes at 188 decibels, the sound travels through the entire ocean basin. Every creature for a thousand miles hears it, not as a distant echo but as a pressure wave that moves through their body. XObese does the same thing to a mix. Thirteen oscillators -- one sub, four groups of three -- each tuned at root, +12 semitones, and -12 semitones with per-oscillator analog drift. The sound does not sit in the mix. It displaces everything else. The hot pink accent color is the belly of a right whale surfacing -- unexpected beauty on something that massive, a reminder that the largest animal on Earth is also one of the most graceful.

The Mojo system is the whale's biology. Each of the thirteen oscillators receives independent random-walk pitch drift of up to three cents, modeled on the fact that no biological system produces perfectly stable frequency. At Mojo zero, XObese is a digital supersaw -- precise, clinical, impressive but lifeless. At Mojo one, it breathes. The drift is subtle enough that the ear hears it as warmth rather than detuning, the same way whale song sounds "warm" even though it is mathematically complex. The four ZDF Ladder Filters -- one per oscillator group, each with proper tan() pre-warp, inter-stage soft-clip, and Nyquist-aware resonance scaling -- are the whale's vocal tract, shaping the raw oscillator mass into something that can be musical rather than merely overwhelming.

The arpeggiator is the whale's rhythm -- the cyclical patterns of diving and surfacing, the repeated song phrases that can last hours. Five patterns (Up, Down, UpDown, Random, AsPlayed) across five rates with octave expansion up to three octaves. The saturation stage adds even harmonics through asymmetric waveshaping with a DC blocker, and the bitcrusher provides sample-rate reduction with TPDF dithering for lo-fi degradation. These are not effects bolted onto a synth. They are the distortion that happens when something that large tries to fit into a recording.

---

## Polarity

**Position:** Open Water -- the whale belongs to no reef, no shore, no single depth
**feliX-Oscar balance:** 70% Oscar / 30% feliX -- massive and sustained, but the arpeggiator and the sheer energy of 13 oscillators give it kinetic power

---

## DSP Architecture (As Built)

```
VOICE (up to 6 polyphony -- CPU-limited by 13 oscillators per voice)
|
+-- FatMorphOsc x1 (Sub oscillator -- triangle, configurable octave -2/-1/0)
|     Per-oscillator FatMojoDrift (xorshift32 random walk, +/-3 cents)
|
+-- FatMorphOsc x12 (4 groups of 3 oscillators)
|     Each osc: morphable waveform (Sine -> Saw -> Square -> Noise)
|     Inline PolyBLEP anti-aliasing on all discontinuities
|     Per-osc FatMojoDrift for analog character
|     Group layout:
|       Group 1 (oscs 1-3): root, +12st, -12st -> Filter 1 -> center pan
|       Group 2 (oscs 4-6): root, +12st, -12st -> Filter 2 -> left pan
|       Group 3 (oscs 7-9): root, +12st, -12st -> Filter 3 -> right pan
|       Group 4 (oscs 10-12): root, +12st, -12st -> Filter 4 -> stereo wide
|
+-- FatLadderFilter x4 (ZDF 4-pole ladder lowpass per group)
|     tan() pre-warp, Nyquist resonance scaling
|     Inter-stage soft-clip (fastTanh between poles)
|     Drive, cutoff, resonance (shared coefficients computed once per voice)
|     Key tracking, filter envelope modulation
|
+-- FatSaturation (asymmetric waveshaper + DC blocker)
|     Even harmonics via dual tanh shaping
|     Gain-compensated output
|
+-- FatBitcrusher (sample-rate reduction + bit-depth quantization)
|     TPDF dithering for clean lo-fi degradation
|     Block-constant level precomputation
|
+-- FatEnvelope (amp ADSR -- linear attack, exponential decay/release)
+-- FatEnvelope (filter ADSR)
|
+-- FatArpeggiator (pattern-based, internal tempo)
|     5 patterns: Up, Down, UpDown, Random, AsPlayed
|     5 rates: 1/4, 1/8, 1/8T, 1/16, 1/32
|     1-3 octave expansion, configurable gate length
|     Sustain pedal support
|
+-> Per-group stereo pan (constant-power) -> Mix -> Amp Envelope -> Output
```

**Glide:** Exponential portamento, mono/legato voice modes.
**Voice management:** Oldest-note stealing, up to 6 voices (CPU budget for 13 oscs).
**Coupling:** Audio output (ch0/ch1), envelope level (ch2). Receives PitchMod, FilterMod.

---

## Signature Sound

XObese is the only engine in XOceanus that stacks thirteen oscillators per voice across four stereo-panned groups, each with its own ZDF ladder filter. The result is a wall of sound that has physical width -- not stereo widening as an effect, but genuine multi-source stereo content. The Mojo drift system gives the wall organic movement: at high Mojo values, the thirteen oscillators drift independently like a choir that never quite locks into perfect unison, producing a shimmering, living mass. Combined with the morphable waveform (continuous Sine-to-Saw-to-Square-to-Noise blend), XObese can produce everything from vast, warm pads to crushing industrial leads, but it always sounds massive.

---

## Coupling Thesis

Obese gives amplitude -- raw, massive amplitude that drives other engines' filters, chokes their output, fills their frequency spectrum. When XObese output couples to another engine's filter via AmpToFilter, every note opens the receiving engine wide. His sheer spectral density means even quiet notes produce a full-band signal. He receives pitch and filter modulation that shapes his enormous sound into something that breathes -- without external modulation sculpting him, he is a wall. With coupling, he is an ocean. The most dramatic pairing is Obese into Overdub's tape delay: the whale song echoing through the thermocline, degrading beautifully over distance.

---

## Preset Affinity

| Foundation | Atmosphere | Entangled | Prism | Flux | Aether |
|-----------|-----------|-----------|-------|------|--------|
| High | Medium | High | Medium | Low | Low |

---

*XO_OX Designs | XObese -- the whale, thirteen harmonics traveling for miles, hot pink belly surfacing*
