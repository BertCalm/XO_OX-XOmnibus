# XOverbite — Concept Brief (Retrospective)

*Retrospective Phase 0 | March 2026 | XO_OX Designs*
*Enriching the first generation with the aquatic mythology*

---

## Identity

**XO Name:** XOverbite
**Gallery Code:** OVERBITE
**Accent Color:** Fang White `#F0EDE8`
**Parameter Prefix:** `poss_`
**Engine Dir:** `Source/Engines/Bite/`

**Thesis:** Bass-forward character synth -- plush weight meets feral bite. The anglerfish: a soft lure hiding fangs.

---

## Aquatic Identity

The anglerfish. Two thousand meters below the surface, in water so dark that the concept of color has no meaning, a point of light appears. Soft. Warm. Pulsing gently. It looks like safety. It is the most dangerous thing in the ocean. The anglerfish's bioluminescent lure -- the esca -- attracts prey with an irresistible glow, and when they get close enough, the jaw opens. The bite is instantaneous, disproportionate, and final. XOverbite is built on this exact duality: the Belly macro (warm sub weight, closed filter, Fur saturation, Weight Engine reinforcement) is the lure, and the Bite macro (aggressive OscB mix, Gnash asymmetric distortion, filter resonance push) is the jaw.

Five character stages form the anatomy of a predator. BiteFur is the anglerfish's textured skin -- pre-filter tanh saturation that adds plush even harmonics before the filter ever touches the signal. The CytomicSVF filter is the mouth, four modes (LP, BP, HP, Notch) with key tracking and envelope-driven movement. BiteChew is the jaw muscle -- post-filter soft-knee compression that adds sustain and body to the bass. BiteGnash is the teeth -- asymmetric waveshaping where the positive half clips harder than the negative, creating aggressive odd harmonics that cut through a mix the way fangs cut through prey. BiteTrash is the aftermath -- three dirt modes (Rust bitcrush, Splatter wavefold, Crushed hard-clip) that represent what happens after the bite. The signal path is an anatomy lesson in predation.

The Fang White accent is the anglerfish's teeth -- pale, bone-colored, the last thing prey sees. It is Oscar's white, not feliX's electric blue. The anglerfish is an Oscar creature: patient, deep-dwelling, ambush-hunting. But the Scurry macro (LFO rate multiplication, envelope time compression) reveals a nervous, twitching energy underneath the patience -- the anglerfish is always watching, always adjusting, and when it moves, it moves fast. Play Dead (extended release, ducked level, closed filter) is the anglerfish going dark: retracting the lure, becoming invisible, waiting for the next opportunity.

---

## Polarity

**Position:** The Deep -- two thousand meters down, where the lure is the only light
**feliX-Oscar balance:** 80% Oscar / 20% feliX -- patient and deep, but the Bite macro and Scurry reveal feral energy

---

## DSP Architecture (As Built)

```
VOICE (up to 16 polyphony, configurable 1/2/4/8/16)
|
+-- BiteOscA ("Belly" -- 4 warm waveforms)
|     0: Sine (shape adds even harmonic fold-in)
|     1: Triangle (shape morphs toward saw)
|     2: Saw (shape controls warmth rolloff, PolyBLEP)
|     3: Cushion Pulse (shape controls pulse width, PolyBLEP)
|     Per-voice analog drift (0.37 Hz sine LFO, smoothed)
|
+-- BiteOscB ("Bite" -- 5 aggressive waveforms)
|     0: Hard Sync Saw (shape = slave ratio 1x-4x, syncs to OscA)
|     1: FM (2-op, shape = mod index, external FM input)
|     2: Ring Mod (shape = carrier sine->saw blend, ring with OscA)
|     3: Noise (shape = color white->brown, leaky integrator)
|     4: Grit (shape = bit depth 64->8 steps, quantized saw)
|     Instability (2.7 Hz pitch jitter for organic movement)
|
+-- Osc Interaction (4 modes)
|     1: Soft Sync, 2: Low FM, 3: Phase Push, 4: Grit Multiply
|
+-- BiteSubOsc (sine at -1 or -2 octaves)
+-- BiteWeightEngine (sub-harmonic reinforcement)
|     5 shapes (Sine/Tri/Saw/Square/Pulse), 3 octaves (-1/-2/-3)
|     Fine tune in cents
|
+-- BiteNoiseSource (5 types with amplitude decay)
|     White, Pink (3 leaky integrators), Brown (integrated),
|     Crackle (sparse impulses), Hiss (high-passed)
|     Routing: Pre-filter / Post-filter / Parallel / Sidechain
|
+-- Mix: OscA * (1-mix) + OscB * mix + Sub + Weight + Noise
|
+-- BiteFur (pre-filter tanh saturation -- plush even harmonics)
+-- Filter Drive (pre-filter tanh with adjustable gain)
+-- CytomicSVF (4 modes: LP, BP, HP, Notch)
|     Key tracking, filter envelope, mod envelope, LFO mod
|     Coupling: external AmpToFilter modulation
|
+-- BiteChew (post-filter soft-knee compressor -- sustain and body)
+-- BiteGnash (asymmetric waveshaper -- positive clips harder)
+-- BiteTrash (3 dirt modes)
|     1: Rust (bitcrush + sample-rate reduction)
|     2: Splatter (tri-fold waveshaping)
|     3: Crushed (hard clip with gain)
|
+-- BiteAdsrEnvelope x3 (amp, filter, mod)
+-- BiteLFO x3 (7 shapes: Sine/Tri/Saw/Sqr/S&H/Random/Stepped)
|     Per-LFO retrigger + start phase offset
|
+-- 5 Macros:
|     M1 BELLY: plush weight (closes filter, raises sub+fur+weight)
|     M2 BITE: feral aggression (opens OscB mix, drives gnash+reso)
|     M3 SCURRY: nervous energy (multiplies LFO rates, compresses envelopes)
|     M4 TRASH: dirt and destruction (raises trash amount + resonance)
|     M5 PLAY DEAD: decay to silence (extends release, ducks level, closes filter)
|
+-> Amp envelope * velocity -> Equal-power pan -> Stereo Output
```

**Glide:** Exponential portamento with glide mode (off/legato/always), per-voice tracking.
**Voice management:** Oldest-note stealing, 1-16 voices.
**Coupling:** Audio output (ch0/ch1), envelope level (ch2). Receives AmpToFilter (drum hits pump filter), AudioToFM (external audio FM-modulates OscB).

---

## Signature Sound

XOverbite's signature is the tension between the Belly and Bite macros -- two opposing forces in a single voice. At Belly=1/Bite=0, the engine produces round, warm, pillowy sub bass reinforced by the Weight Engine and softened by Fur saturation. At Belly=0/Bite=1, the same voice becomes a snarling, gnashing aggressive lead with asymmetric distortion and resonance push. In between is where the anglerfish lives: bass that is simultaneously plush and feral, inviting and dangerous. The five character stages (Fur, filter, Chew, Gnash, Trash) are stacked in series, and because each adds different harmonic content at different points in the signal chain, the order matters -- pre-filter warmth into post-filter aggression creates a sound that no single distortion stage can produce.

---

## Coupling Thesis

Overbite receives energy and transforms it into predation. Drum hits from Onset drive the Bite macro through AmpToFilter coupling, making the bass snarl on every snare -- the snare is the prey triggering the anglerfish's jaw reflex. Width from Obese drives the filter, creating the widest bass in the ocean. External audio from any engine can FM-modulate OscB through AudioToFM coupling, feeding the anglerfish's bite with the spectral content of other species. As a source, Overbite's bass output through Overdub's tape delay is the deepest dub imaginable -- whale song filtered through predator biology. He feeds from above and lurks below.

---

## Preset Affinity

| Foundation | Atmosphere | Entangled | Prism | Flux | Aether |
|-----------|-----------|-----------|-------|------|--------|
| High | Medium | High | Low | Medium | Low |

---

*XO_OX Designs | XOverbite -- the lure glows soft, the jaw opens wide, the deep takes everything*
