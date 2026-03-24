# XOblongBob — Concept Brief (Retrospective)

*Retrospective Phase 0 | March 2026 | XO_OX Designs*
*Enriching the first generation with the aquatic mythology*

---

## Identity

**XO Name:** XOblongBob
**Gallery Code:** OBLONG
**Accent Color:** Amber `#E9A84A`
**Parameter Prefix:** `bob_`
**Engine Dir:** `Source/Engines/Bob/`

**Thesis:** Warm, character-driven synthesis -- evolving textures, fuzzy atmospheres, playful analog movement. Soft coral that breathes.

---

## Aquatic Identity

Living coral. Not the skeletal calcium carbonate that tourists mistake for rock, but the soft tissue itself -- the polyps, the mucus layer, the symbiotic algae that give coral its color. Touch a living coral and it feels warm, slightly yielding, faintly sticky. That is the XOblongBob sound. The Oblong Sine waveform -- a sine with a gentle second harmonic folded in -- is the polyp: simple in structure, complex in texture. The Velvet Saw and Cushion Pulse are the swaying arms that filter-feed from the current. Every waveform in this engine has been softened, rounded, given a tactile quality that makes it feel closer to organic tissue than electronic oscillation.

The BobSnoutFilter is the coral's relationship with the water passing over it. Four modes -- Snout LP, Snout BP, Snout Form, and Snout Soft -- each represent a different way the coral interacts with current. The formant mode is the coral mouth, shaping passing frequencies into vowel-like resonances. The Curiosity LFO system -- with its five behavior modes (Sniff, Wander, Investigate, Twitch, Nap) -- is the coral's nervous system, responding to stimulation with the slow, exploratory movements of an organism that has no brain but somehow still seems curious. Bob does not compute modulation. Bob sniffs, wanders, investigates, twitches, and naps.

The amber accent color is the coral itself at golden hour -- warm, honeyed, alive in the last light of the day. Bob is Oscar's architecture. He is the reef that every other species depends on, the structure that turns open water into habitat. Without the reef, the ocean is just water. Without Bob, XOlokun is just synthesizers.

---

## Polarity

**Position:** The Reef -- Oscar's home, where structure meets life
**feliX-Oscar balance:** 75% Oscar / 25% feliX -- warm and patient, but the Curiosity LFO system has a playful, darting quality

---

## DSP Architecture (As Built)

```
VOICE (up to 16 polyphony)
|
+-- BobOscA (Oblong Sine / Soft Triangle / Velvet Saw / Cushion Pulse)
|     Per-voice analog drift via xorshift32 PRNG random walk
|     Shape parameter modifies per-waveform character
|     Tune control with semitone offset + drift multiplier
|
+-- BobOscB (Soft Saw / Rounded Pulse / Triangle / Sub Harmonic)
|     Detune in cents, hard sync to OscA, FM from OscA
|
+-- BobTextureOsc (stereo noise/texture layer)
|     4 modes: Dust, Blanket, Static, Breath
|     Stereo width via mid/side decorrelation
|     Tone control, pitch-tracked band-pass noise
|
+-- Mix (OscA + OscB + Texture)
|
+-- BobSnoutFilter (TPT SVF, 4 character modes)
|     0: Snout LP (warm lowpass with resonance character tail)
|     1: Snout BP (nasal bandpass with soft-clip)
|     2: Snout Form (dual formant -- two parallel BPs, character crossfades)
|     3: Snout Soft (gentle 1-pole lowpass)
|     Drive pre-saturation, per-voice key tracking
|
+-- BobDustTape (tape saturation + HF rolloff)
|     Even harmonic soft saturation, LP head response
|     Amount control blends dry/wet
|
+-- BobAdsrEnvelope (amp -- linear attack, exponential decay/release)
+-- BobAdsrEnvelope (motion -- modulation envelope)
+-- BobCuriosityLFO (dual LFO + 5 curiosity behavior modes)
|     LFO1: Sine, Triangle, S&H, Smooth Random
|     LFO2: micro-motion (always smooth random, very slow)
|     Curiosity: Sniff, Wander, Investigate, Twitch, Nap
|
+-- BobMode macro (single knob fans to drift, character, texture, mod, FX)
|
+-> Amp envelope * Velocity -> Pan -> Stereo Output
```

**Glide:** Exponential portamento with configurable time, per-voice tracking.
**Voice management:** Oldest-note stealing, up to 16 voices.
**Coupling:** Audio output (ch0/ch1), envelope level (ch2). Receives AmpToFilter, PitchMod.

---

## Signature Sound

XOblongBob is the engine you reach for when a sound needs to feel warm in your hands. The combination of the Oblong Sine (a sine with its own second harmonic baked in), the Snout Filter's character saturation, and the DustTape stage creates textures that feel woolly, alive, slightly fuzzy -- like touching a sea anemone. The Curiosity LFO system means no two notes ever evolve the same way, giving sustained pads a quality of slow, organic investigation that pure LFO modulation cannot achieve.

---

## Coupling Thesis

Oblong gives warmth. His output softens whatever it touches -- amplitude coupling from Oblong makes aggressive engines gentler, like sound passing through coral structure. He receives well from bright, transient sources: feliX's snaps, Onset's drum hits, Overworld's chip edges. The contrast between his warmth and their sharpness creates the most musical pairings in the gallery. When Onset drives Bob's filter via AmpToFilter coupling, every drum hit blooms the coral open. He is the reef that other species shelter in.

---

## Preset Affinity

| Foundation | Atmosphere | Entangled | Prism | Flux | Aether |
|-----------|-----------|-----------|-------|------|--------|
| High | High | Medium | Medium | Medium | High |

---

*XO_OX Designs | XOblongBob -- the living reef, warm and yielding, architecture for every species that follows*
