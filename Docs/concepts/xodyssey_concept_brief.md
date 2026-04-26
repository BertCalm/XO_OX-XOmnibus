# XOdyssey — Concept Brief (Retrospective)

*Retrospective Phase 0 | March 2026 | XO_OX Designs*
*Enriching the first generation with the aquatic mythology*

---

## Identity

**XO Name:** XOdyssey
**Gallery Code:** ODYSSEY
**Accent Color:** Violet `#7B2D8B`
**Parameter Prefix:** `odyssey_`
**Engine Dir:** `Source/Engines/Odyssey/`

**Thesis:** Evolving pad synthesis with Voyager drift and psychedelic character processing -- the open ocean journey from familiar shores to alien depths.

---

## Aquatic Identity

The open ocean. No reef, no shore, no floor visible -- just the vast middle water where currents carry you from the familiar into the unknown. This is the pelagic zone, the largest habitat on Earth, where the only landmarks are the shifting gradients of light, temperature, and pressure. Creatures of the open ocean do not navigate by landmarks. They navigate by change itself -- the slow warming of water as a current shifts, the deepening blue as the continental shelf falls away, the subtle chemical signatures that say "you are far from home now."

XOdyssey is that journey made audible. The Voyager Drift is the current -- a smooth random walk on pitch and filter that ensures you never hear the same note the same way twice. It is not chaos. It is the organized unpredictability of a living ocean, where every molecule is in motion but the overall system has shape and direction. The drift rate and depth parameters are the strength and speed of the current. At low values, the journey is gentle -- a slow rocking that could be the surface swell above a deep reef. At high values, the journey is disorienting -- a riptide pulling you into unfamiliar harmonic territory.

The three oscillator modes (Classic, Supersaw, FM) are three species of open-water creature encountered on the voyage. Classic PolyBLEP is the familiar -- clean waveforms from the shore you know. Supersaw is the school of fish, seven detuned voices swimming in formation, creating a wall of width and movement. FM is the deep-sea bioluminescent flash -- metallic, alien, harmonically complex in ways that feel extraterrestrial. Dual oscillators (A and B, independently selectable) mean you can layer the familiar with the alien in a single voice. The Haze Saturation is the thermocline warmth rising from below. The Formant Filter is the voice of the ocean itself -- vowel shapes morphing through ah, eh, ee, oh, oo, as if the water is trying to speak. The Prism Shimmer is sunlight refracting through the surface far above, adding harmonics that shimmer like light through water. Violet is the color of the deepest visible light -- the last wavelength before the ocean swallows everything.

---

## Polarity

**Position:** Open Water -- the pelagic zone, between surface and deep
**feliX-Oscar balance:** 40/60 -- Oscar-leaning, but with feliX's restless movement

---

## DSP Architecture (As Built)

The DriftEngine is an 8-voice psychedelic pad synthesizer ported from the standalone XOdyssey instrument. It features dual multi-mode oscillators, three character stages, dual filters, Voyager Drift, and a full modulation system.

**Signal flow per voice:**

```
Osc A (Classic | Supersaw | FM) x Level
  + Osc B (Classic | Supersaw | FM) x Level
  + Sub Oscillator (sine, -1 octave of A)
  + Noise (xorshift32 PRNG)
    |
    v
Haze Saturation (pre-filter tanh warmth, variable drive + makeup gain)
    |
    v
Filter A: Cytomic SVF Low-Pass (12 or 24 dB/oct cascade)
  with Envelope mod + LFO mod + Drift mod + Coupling mod + CC1
    |
    v
Filter B: Formant Filter (3 parallel SVF bandpasses)
  Vowel morph: ah -> eh -> ee -> oh -> oo
    |
    v
Prism Shimmer (full-wave rectification + one-pole LP tone control)
    |
    v
ADSR Envelope x Velocity
    |
    v
Drift-based Stereo Pan -> Output
```

**Oscillator modes (per osc, independently selectable for A and B):**
- **Classic (0):** PolyBLEP with 4 waveforms (Sine, Triangle, Saw, Pulse/PWM). The familiar shore.
- **Supersaw (1):** 7-voice detuned sawtooth with inline PolyBLEP anti-aliasing. Staggered initial phases for immediate stereo width. Detune spreads voices symmetrically in 3 pairs around center.
- **FM (2):** 2-operator FM (modulator -> carrier). Depth controls modulation index (x5). Shape parameter maps to harmonic ratio (1:1 through 4:1).

**Voyager Drift (ported from XOdyssey):**
- Smooth random walk: xorshift32 PRNG generates new target values, one-pole LP smoothing interpolates between them.
- Rate parameter controls how often new targets are generated.
- Depth parameter scales the output range.
- Applied to pitch (up to +/-0.5 semitones), filter cutoff (subtle octave shift), and stereo pan position.
- Per-voice seeded RNG ensures each voice drifts independently -- a chord becomes a living organism.

**Character stages (ported from XOdyssey):**
- **Haze Saturation:** Pre-filter soft tanh with variable drive gain (1x to 5x) and makeup compensation. Crossfaded with dry signal by amount parameter. Adds warmth without destroying dynamics.
- **Prism Shimmer:** Post-filter full-wave rectification generating even harmonics, with one-pole LP tone control. Tone parameter sweeps the LP cutoff from 500 Hz (dark shimmer) to 15.5 kHz (crystalline). Adds upper partials that feel like light refracting.

**Formant Filter (ported from XOdyssey's FilterB):**
- 3 parallel Cytomic SVF bandpasses tuned to vowel formant frequencies.
- 5 vowel presets: ah (730/1090/2440 Hz), eh (530/1840/2480), ee (270/2290/3010), oh (570/840/2410), oo (300/870/2240).
- Morph parameter crossfades between adjacent vowels, creating smooth vocal sweeps.
- Mix parameter blends formant-filtered signal with direct signal.
- EnvToMorph coupling from external engines can sweep the vowel position.

**Modulation:**
- ADSR envelope with exponential decay/release (same DriftAdsrEnvelope pattern as DubEngine).
- LFO (sine, routable to pitch, filter, or amplitude).
- CC1 mod wheel opens filter (+4000 Hz at full travel).
- Glide with exponential frequency slew (applied to both Osc A and B, with ratio preservation).

**Parameters (34 total):** `odyssey_oscA_mode`, `odyssey_oscA_shape`, `odyssey_oscA_tune`, `odyssey_oscA_level`, `odyssey_oscA_detune`, `odyssey_oscA_pw`, `odyssey_oscA_fmDepth`, `odyssey_oscB_mode`, `odyssey_oscB_shape`, `odyssey_oscB_tune`, `odyssey_oscB_level`, `odyssey_oscB_detune`, `odyssey_oscB_pw`, `odyssey_oscB_fmDepth`, `odyssey_subLevel`, `odyssey_noiseLevel`, `odyssey_hazeAmount`, `odyssey_filterCutoff`, `odyssey_filterReso`, `odyssey_filterSlope`, `odyssey_filterEnvAmt`, `odyssey_formantMorph`, `odyssey_formantMix`, `odyssey_shimmerAmount`, `odyssey_shimmerTone`, `odyssey_attack`, `odyssey_decay`, `odyssey_sustain`, `odyssey_release`, `odyssey_lfoRate`, `odyssey_lfoDepth`, `odyssey_lfoDest`, `odyssey_driftDepth`, `odyssey_driftRate`, `odyssey_level`, `odyssey_voiceMode`, `odyssey_glide`, `odyssey_polyphony`.

---

## Signature Sound

XOdyssey sounds like distance. The Voyager Drift ensures that every held chord slowly evolves, every sustained note wanders through micro-detunings that make the sound feel alive and unbounded. Layer Classic sine on Osc A with Supersaw on Osc B and the result is a pad that has both warmth and width -- a recognizable fundamental surrounded by a shimmering cloud of detuned harmonics. Run it through the Formant Filter with a slow morph sweep and the pad begins to speak -- vowel shapes emerging from the harmonic content like voices heard across open water. Add Haze Saturation for analog warmth and Prism Shimmer for crystalline upper partials, and the sound occupies a space no other engine can reach: simultaneously warm and bright, familiar and alien, grounded and drifting. It is the sound of a journey you cannot take back.

---

## Coupling Thesis

Odyssey is the traveler. She gives evolving envelopes and drifting modulation via her envelope output on coupling channel 2. This peak envelope, always shifting because of the Voyager Drift, makes receiving engines feel alive -- their filters open and close with the slow breathing of a pad that is never static.

She receives amplitude and envelope coupling that accelerates or redirects her journey. AmpToFilter from feliX or Onset creates rhythmic filter movement on her pads -- the percussion of the surface heard as filter pumps in the deep. LFOToPitch and AmpToPitch add external pitch modulation on top of her own drift, creating compound movement that feels oceanic in scale. EnvToMorph coupling sweeps her formant filter vowel position from outside, meaning another engine can literally change the words the ocean is speaking.

When coupled with Overdub, Odyssey's drifting pads gain history -- each echo slightly different from the last because the drift has moved between repetitions. When coupled with Oscar, their two drift systems create interference patterns like overlapping ripples. When coupled, she tells a story. Alone, she asks a question.

---

## Preset Affinity

| Foundation | Atmosphere | Entangled | Prism | Flux | Aether |
|-----------|-----------|-----------|-------|------|--------|
| Low | High | High | Medium | High | High |

---

*XO_OX Designs | XOdyssey -- the open ocean, where the current carries you past the point of return*
