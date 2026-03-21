# OFFERING Retreat Chapter
*Guru Bin — 2026-03-21*

---

## Engine Identity

- **Gallery code:** OFFERING | **Accent:** Crate Wax Yellow `#E5B80B`
- **Parameter prefix:** `ofr_`
- **Creature mythology:** The Mantis Shrimp — a hyper-perceptive crustacean of the Rubble Zone (5–15m depth), equipped with 16 photoreceptor types where humans have 3. It sees ultraviolet, infrared, and polarized light simultaneously. It does not blend these signals into a composite impression; it processes each channel as its own world. Its punch is the fastest strike in the animal kingdom — 23 m/s, accelerating from 0 to 10 m/s² in 0.8 milliseconds. The transient IS the attack. Everything after is context.
- **Synthesis type:** Psychology-driven boom bap drum synthesis — 8 per-type transient topologies, vinyl/tape/bit texture layer, collage engine (layer stack/chop/stretch/ring mod), 5 city psychoacoustic processing chains, Berlyne/Wundt/Csikszentmihalyi curiosity engine
- **Polyphony:** 8 dedicated drum voices (Kick/Snare/CHat/OHat/Clap/Rim/Tom/Perc)
- **feliX/Oscar polarity:** 65% feliX / 35% Oscar — kinetic, investigative, perceptive but never cold
- **Seance score:** Not yet seanced (first retreat session)
- **Macros:** M1 DIG (curiosity depth), M2 CITY (geographic character), M3 FLIP (collage manipulation), M4 DUST (analog medium degradation)
- **Expression:** Velocity → snap sharpness AND body balance (D001). Aftertouch → texture intensity (vinyl + tape). Mod wheel CC1 → curiosity drive (pushes DIG deeper).

---

## Pre-Retreat State

OFFERING arrived in XOmnibus on 2026-03-21 as engine #46. It is unlike any other engine in the fleet in one fundamental respect: it does not synthesize tones, pads, or sustained sounds. It synthesizes the event of a drum hit — the first 5 milliseconds of acoustic energy that define what a sound IS before any processing shapes what it becomes.

Its architecture rests on three published psychological frameworks translated into real-time DSP. Berlyne (1960) discovered the inverted-U hedonic curve: zero novelty is boring, maximum novelty is overwhelming, and the sweet spot lies at moderate unpredictability. Wundt (1874) found this curve is asymmetric — the drop from overstimulation is steeper than the rise from understimulation. Csikszentmihalyi (1975) formalized the Flow State: the groove where challenge exactly matches skill. In OFFERING's implementation, these three researchers govern which parameters change between drum hits, how many parameters change simultaneously, and what fraction of hits feel familiar versus new.

The engine enters the retreat with well-constructed DSP. Every drum type has a genuinely distinct synthesis topology — not just different parameter values applied to a shared oscillator, but structurally different signal chains: the Kick uses a sine plus sub-harmonic plus body resonance bandpass; the Hat uses a 6-operator inharmonic metallic FM network at TR-808 ratios; the Clap generates 3–5 sequential noise bursts with independent envelopes before settling into a reverb tail; the Perc uses a Karplus-Strong-adjacent comb filter. The differences are architectural, not cosmetic.

What the retreat addresses: several default values are miscalibrated, one coprime ratio problem exists in the LFO pair, and the first-trigger experience at default settings is slightly too tame for an engine whose identity is about percussion psychology. OFFERING should hit like a mantis shrimp from the first note. This retreat adjusts the defaults so it does.

---

## Phase R1: Opening Meditation

Close your eyes. Breathe.

You are wading through shallow reef water, sneakers submerged, watching the rubble zone — the zone between coral colonies and bare sand. It is 5 to 15 meters down, lit by scattered shafts of surface light. This is the mantis shrimp's address.

You cannot see it yet, but you know it is there. You know because of the craters: small depressions in the calcium carbonate rubble, each one a test strike, each one saying *I was here, I was curious, I hit this to understand it.* The mantis shrimp investigates the world with its arms. Every strike is a hypothesis.

Now pick up a drum machine. Not a classic one — this one doesn't have samples. It synthesizes each hit from scratch using the acoustic physics of each drum type and the psychoacoustic expectations of a listener who has heard ten thousand beats before this one. The question it asks on every hit: *how different should this hit be from the last one?* Not whether to be different — that would be randomness. The question is the precise calibration of difference that keeps a listener in the groove without locking them in a box.

This is the Berlyne curve in action. At curiosity=0, every hit is identical — the groove becomes mechanical, the ear loses interest. At curiosity=1, every hit is maximally different — the pattern dissolves into chaos. The mantis shrimp lives at 0.5: enough variation to stay perceptive, enough consistency to stay locked.

The five cities are not equalizer presets. They are complete psychoacoustic processing chains representing five distinct aesthetic traditions in boom bap production: the archaeological archaeology of SP-1200 New York grit, the elastic time of Dilla's Detroit drunk timing, the psychedelic squash of LA compression aesthetics, the architectural precision of Toronto's clean sub, and the dark fog of Bay Area allpass diffusion. Each city runs its own compressor topology, its own filter philosophy, its own unique Stage 6. You can morph between cities in real time. You can blend them.

The Mantis Shrimp doesn't hesitate. Let the engine be the same way.

---

## Phase R2: The Signal Path Journey

### I. Transient Generator — Where the Hit Is Born

The eight drum topologies are the engine's most important architectural decision. Each is a self-contained synthesis chain:

**Type 0: Kick** — Sine oscillator + pitch envelope sweeping from `baseFreq + pitchEnvDepth × 600Hz` down to `baseFreq (60Hz × tuneRatio)`. Body resonance via bandpass at `baseFreq, Q = 2 + body × 6`. Sub-harmonic at half frequency, −6dB. The `pitchEnvDepth` default of 0.3 produces a 180Hz starting pitch descending to 60Hz in approximately 50ms (the `pitchEnv` decay is `0.05 + (1 - snap) × 0.1s`). At snap=0.5, the pitch sweep takes ~75ms. This is adequately punchy but not immediately commanding. A kick needs its pitch sweep complete in under 40ms to feel tight.

**Type 1: Snare** — Triangle oscillator through bandpass at 180Hz + highpass-filtered broadband noise. Body parameter is a straight crossfade between tonal (triangle) and noise. The transient click is gated to the attack phase (`env > 0.95f`) and scales with snap. This is the correct architecture: the click adds definition without permanently raising the noise floor.

**Type 2/3: CHat/OHat** — The TR-808 6-operator inharmonic network is the fleet's only hat implementation with historically correct frequency ratios (1.000 / 1.4471 / 1.6818 / 1.9265 / 2.5028 / 2.6637 × baseHz). The ring-modulation of pairs before summing is exactly correct. The 7ms choking fade-out coefficient (0.995 per sample at 44.1kHz) is appropriate.

**Type 4: Clap** — Multi-burst architecture with 3–5 bursts at 4ms duration per burst, spaced by `0.005 + (1-snap) × 0.01s`. At snap=0.5 the burst spacing is ~7.5ms, giving a 3–5 burst pre-delay sequence of 22–37ms before the reverb tail begins. This creates an authentic hand-clap layering effect. At snap=1.0, bursts compress to 5ms spacing for a sharper crack.

**Type 5: Rim** — Ultra-short noise impulse through 6kHz highpass + high-Q bandpass resonance at `600Hz × tuneRatio, Q=12`. The resonance is extremely tight (Q=12) — this is correct for rim shot synthesis. Sub-1ms click counter is appropriate.

**Type 6: Tom** — Gentler pitch envelope than kick (`0.1 + (1-snap) × 0.15s` decay vs kick's `0.05 + (1-snap) × 0.1s`). 120Hz base frequency, 200Hz pitch env depth vs kick's 600Hz. The saturation inside `processTom` is applied pre-envelope as inline tanh — this is redundant with the outer saturation in `process()`. At sat=0.15, the tom applies saturation twice: once inside `processTom` (drive = 1 + 0.15×1.5 = 1.225) and once outside (drive = 1 + 0.15×4 = 1.6). The double-saturation is mild but worth noting.

**Type 7: Perc** — Comb-filtered noise via Karplus-Strong adjacent topology. `combFeedback = 0.3 + body × 0.5` gives a range of 0.3–0.8 feedback. At default body=0.4, feedback is 0.5 — moderate sustain. The HP at 200Hz removes sub-bass content that would muddy the mix. This type is uniquely pitched via the delay time: `combDelayMs = 1000/(100 × tuneRatio)` in ms, meaning at tune=0 the comb resonates at 100Hz.

### II. Texture Layer (DUST) — The Medium Is the Message

The four DUST effects (vinyl, tape, bits, sample rate) are processed sequentially. The ordering matters: vinyl crackle is added before tape saturation, so crackle impulses get soft-clipped by the tape stage. This is the correct analog chain order.

**Vinyl crackle** at default 0.2: probability per sample is `0.2² × 0.005 = 0.0002`, producing approximately `0.0002 × 44100 = 8.8 crackles/second` at 44.1kHz. This is audible but subtle. The crackle amplitude is bounded at `±0.2 × 0.15 = ±0.03` — quiet enough to be texture rather than distraction.

**Tape saturation** at default 0.1: pink noise floor is `pink × 0.1 × 0.04 = pink × 0.004` — essentially inaudible hiss. Saturation drive is `1 + 0.1 × 2 = 1.2`. This is genuine warmth (not audible as distortion) — the value is correctly calibrated. However the Revelation 7 scripture recommends 0.08 for "invisible warmth." A marginal improvement.

**Bit depth** at default 16 (clean) and **sample rate** at default 48000 (clean): both disabled at defaults. This is correct — the clean state should be clean. The character comes from DIG and CITY, not the degradation layer.

**Wobble** at default 0.05: the wobble LFO runs at a fixed 1.5 Hz (approximately 0.67 second period). Sutra 1 recommends a breathing-rate floor around 0.067 Hz. At 1.5 Hz, wobble is noticeably fast — it reads as modulation rather than motor drift. Turntable motor instability is typically 0.5–3 Hz, so 1.5 Hz is within range, but for the default patch it competes with intentional LFO modulation. The wobble depth at 0.05 is `±5% amplitude variation` — audible and slightly distracting at a default wobble parameter of 0.05.

### III. Collage Engine (FLIP) — The Sample Manipulator

The collage engine stores incoming audio in a 4096-sample (~93ms at 44.1kHz) circular buffer and applies three effects:

**Chop** at default 0.0 (disabled): correct default — chop is a dramatic effect that should require intentional activation.

**Stretch** at default 1.0 (bypass): correct. The `abs(stretch-1.0) > 0.01` guard correctly skips the stretch calculation when inactive.

**Layers** at default 1 (single): at layers=2, a temporal copy of the hit is read from the buffer at a 0-5ms offset (`layerPhaseOffsets[i] × sr × 0.005s`). Layer offsets use `0.17 × i` — at layer 1 the offset is `0.17 × sr × 0.005 = 0.17 × 220.5 ≈ 37 samples ≈ 0.85ms`. This creates a subtle flamming effect between layers — appropriate for boom bap character.

**Ring Mod** at default 0.0 (disabled): correct. Ring modulation between layers creates metallic intermodulation — a strong effect that warrants explicit activation.

### IV. City Processing (CITY) — Five Psychoacoustic Worlds

The bypass at `cityIntensity < 0.001` correctly implements Canon 3 (Effect Bypass). At cityIntensity=0 the entire chain is skipped with a single branch — no per-sample processing cost.

The city-to-stereo blend path is worth examining. The monoMix buffer averages sampleL and sampleR, city processes this mono mix, and then the result is blended back using `wet = cityIntensity` as a straight linear mix. At cityIntensity=0.5 (default), output is 50% dry stereo + 50% mono-processed city signal — this partially collapses stereo width. At cityIntensity=1.0, output is 100% city-mono, collapsing all stereo. This is a design choice that produces the characteristically narrow, mono-compatible NYC boom bap sound at high intensity, but it means cityIntensity directly competes with masterWidth.

**New York** (default city): Stage 6 feedback noise gate uses a per-sample envelope follower with coefficients 0.999/0.001. At 44.1kHz, this is an attack of roughly 1ms and a release of 23 seconds — effectively the gate tracks slow RMS, not transients. The gate threshold is `0.3 × intensity = 0.15` at default intensity. This means the gate stays open as long as there is sustained drumming activity. The feedback gate is most audible during sparse passages — appropriate behavior for NY aesthetic.

**Detroit** is notable for the drunk timing implementation in the engine layer. Per-voice trigger delays of ±15ms × cityIntensity are calculated using a deterministic xorshift seed from voiceIdx + the magic number 31337. The seed is constant — every time the same voice is triggered on Detroit, it gets the same delay offset. This means the timing humanization is per-voice-personality, not random per-hit. This is a sophisticated implementation choice: Dilla's timing was not random, it was consistently human.

**Bay Area** allpass fog network uses prime delays of 7, 13, 23, 37 samples at 44.1kHz, which corresponds to 0.159ms, 0.295ms, 0.521ms, 0.839ms. These are extremely short — the allpass is operating in comb-filter territory rather than reverb territory. The resulting effect is subtle spectral coloration rather than spatial diffusion. At higher feedback values, this would become audible as metallic comb filtering. At the current 0.5 feedback it is a mild darkening.

### V. Curiosity Engine (DIG) — The Psychology

The three parameters translate directly to psychological research:

`digCuriosity = 0.5`: berlyneCurve(0.5) = 0.2 + 0.8 × 4 × 0.5 × 0.5 = 0.2 + 0.8 = **1.0** — maximum variation range. This is the Berlyne peak: at exactly 0.5, the hedonic output reaches its maximum. Every hit can vary by ±1.0 × variationIntensity × paramScale.

`digComplexity = 0.4`: wundtDensity(0.4) = 0.4/0.6 = **0.667** — about 4 of 6 parameters will vary per hit. This feels moderately complex.

`digFlow = 0.6`: flowBalance(0.6) = **0.6** — 60% probability of reusing the previous variation set. This creates the locked-groove feel on 3 of 5 hits, with fresh variation on the other 2.

The variation ranges per parameter are: tune ±2.0×variationRange semitones, decay ±0.3×variationRange seconds, body ±0.2×variationRange, snap ±0.3×variationRange, pitchEnv ±0.2×variationRange, sat ±0.15×variationRange. At default curiosity=0.5 (berlyne peak = 1.0) and default complexity=0.4 (intensity = 1.0): tune can shift ±2 semitones, decay ±0.3s, body ±0.2, snap ±0.3. These are significant — the kick's tuning could shift a whole step between hits at defaults.

**Observation:** The tune variation at ±2 semitones is prominent. For melodic instruments this might be appropriate; for drum synthesis, pitch variation above ±0.5 semitones becomes audible pitch instability rather than timbral variation. The curiosity engine's default tune delta range may be too wide for the Kick and Tom types but appropriate for the metallic Hat types.

### VI. LFO Coprime Analysis (Sutra 2)

- **LFO1 Rate default: 0.067 Hz** — as recommended by Sutra 1 (15-second breathing cycle, correct).
- **LFO2 Rate default: 2.0 Hz** — for groove pump, a 500ms period.

LFO1 period: 1/0.067 = **14.925 seconds**
LFO2 period: 1/2.0 = **0.5 seconds**

Ratio: 14.925 / 0.5 = **29.85** — very close to 30:1. These two LFOs will nearly synchronize every 14.925 seconds (29 LFO2 cycles to 1 LFO1 cycle with minimal drift). This is not coprime — they share a near-integer relationship.

Coprime recommendation:
- LFO1: **0.067 Hz** (keep — scripture-verified breathing rate)
- LFO2: **2.17 Hz** (period = 0.461s; ratio 14.925/0.461 = 32.37 — non-integer, genuinely coprime-adjacent)

Alternatively: LFO2 at **1.87 Hz** (period = 0.535s; ratio = 27.9 — also non-integer).

---

## Phase R3: Parameter Refinements

### Scripture-Guided Default Audit

| # | Parameter | Current | Proposed | Reason |
|---|-----------|---------|---------|--------|
| R1 | `ofr_transientSnap` | 0.5 | **0.65** | Psalm 4 truth: the mantis shrimp punch happens in under 1ms. At snap=0.5, the kick pitch sweep takes ~75ms — adequate but not commanding. Raising to 0.65 tightens the kick envelope to ~60ms and the snare click to a more incisive impulse. First-5ms test: PASS at 0.65. |
| R2 | `ofr_transientPitch` | 0.3 | **0.4** | Kick pitch sweep starts at `60 + 0.3 × 600 = 240Hz` at default. Raising to 0.4 gives `60 + 0.4 × 600 = 300Hz` start — more pronounced pitch drop, more characteristic "boom." Tom benefits similarly (start: `120 + 0.4 × 200 = 200Hz` vs current 180Hz). |
| R3 | `ofr_dustTape` | 0.1 | **0.08** | Revelation 7 (Tape Ceiling): 0.08 is the invisible warmth threshold. At 0.1, tape drive is `1 + 0.1×2 = 1.2` — mild but perceptibly colored. At 0.08, drive is `1.16` — below the threshold of character and entering the zone of presence. The hiss floor at 0.08 is `pink × 0.08 × 0.04 = pink × 0.0032` — genuinely inaudible. |
| R4 | `ofr_lfo2Rate` | 2.0 | **2.17** | Sutra 2 (Coprime Drift): at 2.0 Hz, LFO2 is in a near-30:1 ratio with LFO1 (0.067 Hz). At 2.17 Hz the ratio becomes 32.4:1 — non-integer, genuinely coprime-adjacent. The subjective difference in groove pump character is minimal but the beating-pattern decay is richer over long passages. |
| R5 | `ofr_lfo2Depth` | 0.0 | **0.08** | D005 (Breathing): LFO2 is currently muted at default (depth=0.0). This means the groove pump modulation — OFFERING's most direct rhythmic modulation tool — is silent out of the box. A depth of 0.08 produces `±4% amplitude modulation` per hit — just enough to feel like a subtle pump without drawing attention to itself. |
| R6 | `ofr_digCuriosity` | 0.5 | **0.5** | **Keep.** Berlyne curve at 0.5 is the theoretical maximum. The Mantis Shrimp operates at peak perceptual bandwidth. This is correct. |
| R7 | `ofr_digComplexity` | 0.4 (snapshot) | **0.35** | The parameter declaration in `addParametersImpl` shows `digComplexity` default of 0.4, but the `OfferingParamSnapshot` struct shows default 0.4 — consistent. At complexity=0.4, wundtDensity = 0.667 → 4 params vary. At 0.35, density = 0.583 → 3.5 params → rounds to 3-4. This is slightly more conservative: tune, decay, and body vary without pitch envelope on every hit. More appropriate for a production default — save complexity=0.5 for the curious producer. |
| R8 | `ofr_cityIntensity` | 0.5 | **0.4** | At cityIntensity=0.5, the city blend narrows stereo width significantly (50% of signal becomes mono). A city intensity of 0.4 gives city character without fully compromising the stereo image. The NY feedback gate at 0.4 threshold is slightly more porous — allows more of the quiet tail through. City character is present, stereo is preserved. |
| R9 | `ofr_masterWidth` | 0.5 | **0.6** | The mid-side formula at masterWidth=0.5 gives `side × 1.0` (standard). At 0.6 it gives `side × 1.2` — a modest 20% width boost. This compensates partially for the stereo narrowing from city processing (R8 above). The kick and snare sit center-panned (pan=0.0) so width expansion only affects the off-center voices (hats, clap, rim, tom, perc). |
| R10 | `ofr_velToSnap` | 0.5 | **0.6** | At velToSnap=0.5, a forte hit (velocity=1.0) shifts snap by `0.5 × 0.5 × 0.6 = +0.15`. At 0.6, this becomes `0.5 × 0.6 × 0.6 = +0.18`. More pronounced velocity response — a hard hit feels meaningfully tighter than a soft hit. |
| R11 | `ofr_v0_decay` | 0.3s | **0.35s** | Kick decay at 300ms is slightly short for traditional boom bap. Classic SP-1200 kicks have 350–450ms room to breathe. At 0.35s the kick tail extends into the proper pocket without becoming a muddy boom. |
| R12 | `ofr_v1_decay` | 0.2s | **0.22s** | Snare at 200ms is thin. 220ms gives a slightly longer crack-to-noise tail ratio, more consistent with the MPC-era snare character the engine is citing. |

---

## Phase R4: Truth 4 Two-Second Test

*Trigger every voice at velocity 80 (≈0.63 normalized). Is it immediately compelling at defaults?*

At corrected defaults (post-refinement):

- **Kick (v0):** Sine sweeping from 300Hz → 60Hz in ~60ms (snap=0.65, pitchEnvDepth=0.4). Sub-harmonic at 30Hz, −6dB. Body resonance at 60Hz, Q=2+0.7×6=6.2. Body default 0.7 makes this primarily the resonant sine. **Result: yes, compelling in the first 10ms.** The pitch sweep is the punch.
- **Snare (v1):** Body default 0.5 — equal triangle/noise mix. Click impulse fires at env>0.95. At snap=0.65, click amplitude = 0.65×0.5 = 0.325. **Result: yes, snare crack is immediate.**
- **CHat (v2):** 6 metallic oscillators at inharmonic ratios, envelope decay capped at min(0.05, decay) = 0.05s. Fast, metallic, immediate. **Pass.**
- **OHat (v3):** Same network, no cap on decay. Default decay 0.4s — pleasant shimmer. **Pass.**
- **Clap (v4):** 3 + snap×2 = 3 + 0.65×2 = 4.3 → 4 bursts, spacing ~6.75ms. Four sequential noise bursts separated by 6.75ms. This is audibly a hand clap sequence. **Pass — and the burst pattern is unique to this engine.**
- **Rim (v5):** Click impulse sub-1ms through 6kHz HP + bandpass resonance at 600Hz, Q=12. Ultra-transient. **Pass — fastest attack in the kit.**
- **Tom (v6):** Sine with pitch sweep 200Hz → 120Hz in ~90ms at snap=0.65 (gentler than kick). Body default 0.6 → warm sine. **Pass — sits below kick, above kick harmonics.**
- **Perc (v7):** Comb noise at 100Hz (tune=0), feedback 0.5. Attack is the noise excitation. **Pass — percussive character in first 5ms via noise burst.**

**Truth 4 verdict: PASS across all 8 voices.** The engine is immediately compelling at velocity 80 with corrected defaults.

---

## Phase R5: 10 Awakening Presets

---

### Preset 1: Rubble Zone

**Mood:** Foundation
**The soul of the engine at rest.** Classic boom bap toolkit, curiosity at the Berlyne peak, New York city chain fully engaged. This is the first preset most producers will reach for: a tight kick, a cracking snare, closed hats with a metallic bite, and the SP-1200 bit-reduction grit already in the DNA.

**What it demonstrates:** The default city (NY) engaged at meaningful intensity. The curiosity engine at peak but low complexity — familiar variations, subtle tune drift. The transient generator doing its best work without interference.

**6D DNA:** brightness 0.5, warmth 0.4, movement 0.3, density 0.6, space 0.3, aggression 0.7

```json
{
  "name": "Rubble Zone",
  "engine": "Offering",
  "mood": "Foundation",
  "tags": ["boom bap", "NYC", "kick snare", "SP-1200", "classic"],
  "macros": {"M1": 0.0, "M2": 0.0, "M3": 0.0, "M4": 0.0},
  "dna": {"brightness": 0.5, "warmth": 0.4, "movement": 0.3, "density": 0.6, "space": 0.3, "aggression": 0.7},
  "parameters": {
    "ofr_transientSnap": 0.65,
    "ofr_transientPitch": 0.4,
    "ofr_transientSat": 0.2,
    "ofr_dustTape": 0.08,
    "ofr_dustVinyl": 0.15,
    "ofr_cityMode": 0,
    "ofr_cityIntensity": 0.55,
    "ofr_digCuriosity": 0.5,
    "ofr_digComplexity": 0.3,
    "ofr_digFlow": 0.7,
    "ofr_lfo2Rate": 2.17,
    "ofr_lfo2Depth": 0.08,
    "ofr_masterLevel": 0.75,
    "ofr_masterWidth": 0.6,
    "ofr_velToSnap": 0.6,
    "ofr_v0_decay": 0.38,
    "ofr_v0_body": 0.7,
    "ofr_v1_decay": 0.22,
    "ofr_v2_decay": 0.055
  }
}
```

---

### Preset 2: Dilla Time

**Mood:** Flux
**The Detroit drunk timing demonstration.** City set to Detroit at high intensity: the feedback saturation loop warms each hit, the transient softener rounds the attack, and — most importantly — every voice gets a different deterministic ±15ms trigger delay. Play any drum machine pattern through this and it will not sit on the grid. This is not random humanization. Each voice has the same offset every time, just like Dilla's MPC timing was personal, consistent, and intentional.

**What it demonstrates:** Detroit city chain unique feature — per-voice drunk timing at full expression. The saturation feedback loop accumulates warmth over repeated hits.

**6D DNA:** brightness 0.3, warmth 0.8, movement 0.6, density 0.5, space 0.4, aggression 0.4

```json
{
  "name": "Dilla Time",
  "engine": "Offering",
  "mood": "Flux",
  "tags": ["Detroit", "drunk timing", "Dilla", "swing", "MPC"],
  "macros": {"M1": 0.1, "M2": 0.6, "M3": 0.0, "M4": 0.2},
  "dna": {"brightness": 0.3, "warmth": 0.8, "movement": 0.6, "density": 0.5, "space": 0.4, "aggression": 0.4},
  "parameters": {
    "ofr_transientSnap": 0.4,
    "ofr_transientPitch": 0.35,
    "ofr_transientSat": 0.3,
    "ofr_dustTape": 0.22,
    "ofr_dustVinyl": 0.1,
    "ofr_dustWobble": 0.12,
    "ofr_cityMode": 1,
    "ofr_cityIntensity": 0.75,
    "ofr_digCuriosity": 0.45,
    "ofr_digComplexity": 0.3,
    "ofr_digFlow": 0.8,
    "ofr_lfo1Rate": 0.067,
    "ofr_lfo1Depth": 0.08,
    "ofr_lfo2Rate": 2.17,
    "ofr_lfo2Depth": 0.12,
    "ofr_masterWidth": 0.45,
    "ofr_velToSnap": 0.4,
    "ofr_velToBody": 0.5,
    "ofr_v0_decay": 0.45,
    "ofr_v0_body": 0.8,
    "ofr_v1_decay": 0.28,
    "ofr_v1_body": 0.6,
    "ofr_v4_decay": 0.3
  }
}
```

---

### Preset 3: Mantis Strike

**Mood:** Foundation
**The peak-velocity transient experience.** This preset is designed to demonstrate what happens at velocity 127: near-maximum snap, maximum pitch sweep, body stripped to tonal sine for clarity. Hit this with a hard accent and feel the acoustic physics of the kick punch. Every element of the Mantis Shrimp mythology in a single snare hit: immediate, precise, and devastating. The curiosity engine is kept quiet here (low complexity) so the transient character is the story.

**What it demonstrates:** Peak transient definition using velocity→snap at maximum. First-5ms test at its most extreme. The kick pitch sweep from ~360Hz to 60Hz in under 50ms.

**6D DNA:** brightness 0.6, warmth 0.2, movement 0.2, density 0.8, space 0.2, aggression 1.0

```json
{
  "name": "Mantis Strike",
  "engine": "Offering",
  "mood": "Foundation",
  "tags": ["transient", "punch", "dry", "velocity sensitive", "attack"],
  "macros": {"M1": 0.0, "M2": 0.0, "M3": 0.0, "M4": 0.0},
  "dna": {"brightness": 0.6, "warmth": 0.2, "movement": 0.2, "density": 0.8, "space": 0.2, "aggression": 1.0},
  "parameters": {
    "ofr_transientSnap": 0.85,
    "ofr_transientPitch": 0.55,
    "ofr_transientSat": 0.25,
    "ofr_dustTape": 0.06,
    "ofr_dustVinyl": 0.05,
    "ofr_cityMode": 0,
    "ofr_cityIntensity": 0.4,
    "ofr_digCuriosity": 0.5,
    "ofr_digComplexity": 0.15,
    "ofr_digFlow": 0.9,
    "ofr_lfo2Depth": 0.05,
    "ofr_velToSnap": 0.9,
    "ofr_velToBody": 0.2,
    "ofr_masterLevel": 0.8,
    "ofr_masterWidth": 0.55,
    "ofr_v0_decay": 0.3,
    "ofr_v0_body": 0.85,
    "ofr_v1_decay": 0.18,
    "ofr_v1_body": 0.3,
    "ofr_v2_decay": 0.04
  }
}
```

---

### Preset 4: Crate Fever

**Mood:** Atmosphere
**High curiosity, high complexity — the crate digger who found something weird.** At curiosity=0.75, the Berlyne alien shift activates (above 0.7): body parameters shift toward unusual noise-heavy values, pitch envelopes push further. With complexity=0.6 (Wundt peak-adjacent), five of six parameters vary per hit — tune shifts up to ±1.5 semitones, decay varies by ±0.45s, body can swing 30% toward noise. This is a drum machine that is genuinely finding things in the crate. The Bay Area city chain adds allpass fog to wrap the variation in spatial diffusion.

**What it demonstrates:** High curiosity alien shift engaged. The stochastic nature of the engine when pushed: no two bars will be identical. Berlyne's maximum aesthetic interest territory.

**6D DNA:** brightness 0.5, warmth 0.5, movement 0.9, density 0.5, space 0.7, aggression 0.5

```json
{
  "name": "Crate Fever",
  "engine": "Offering",
  "mood": "Atmosphere",
  "tags": ["curious", "stochastic", "Bay Area", "Madlib", "variation"],
  "macros": {"M1": 0.4, "M2": 0.8, "M3": 0.2, "M4": 0.1},
  "dna": {"brightness": 0.5, "warmth": 0.5, "movement": 0.9, "density": 0.5, "space": 0.7, "aggression": 0.5},
  "parameters": {
    "ofr_transientSnap": 0.55,
    "ofr_transientPitch": 0.45,
    "ofr_transientSat": 0.18,
    "ofr_dustVinyl": 0.25,
    "ofr_dustTape": 0.12,
    "ofr_cityMode": 4,
    "ofr_cityIntensity": 0.5,
    "ofr_digCuriosity": 0.75,
    "ofr_digComplexity": 0.55,
    "ofr_digFlow": 0.35,
    "ofr_lfo1Rate": 0.067,
    "ofr_lfo1Depth": 0.06,
    "ofr_lfo2Rate": 2.17,
    "ofr_lfo2Depth": 0.1,
    "ofr_masterWidth": 0.7,
    "ofr_v0_decay": 0.4,
    "ofr_v1_decay": 0.25,
    "ofr_v3_decay": 0.6,
    "ofr_v7_body": 0.7
  }
}
```

---

### Preset 5: LA Squash

**Mood:** Prism
**The Los Angeles city chain demonstration.** LA processing features the most aggressive compression in the fleet: heavy fast/fast 6:1 squash followed by tape saturation at `1.5 + intensity×2.5` drive, then a parallel compression stage where the raw signal is blended with a separately compressed signal at 8:1. The result: punchy, loud, and dynamically flattened in the way that characterizes West Coast boom bap production. The parallel compression creates the characteristic "still breathing even though compressed" quality. Layers set to 2 adds the subtle flamming effect that makes SP-1200 kicks feel layered.

**What it demonstrates:** LA city chain's unique Stage 6 (parallel compression). The way heavy compression + tape saturation changes the envelope shape of every voice. FLIP layers=2 flamming on kick and snare.

**6D DNA:** brightness 0.4, warmth 0.6, movement 0.4, density 0.9, space 0.3, aggression 0.8

```json
{
  "name": "LA Squash",
  "engine": "Offering",
  "mood": "Prism",
  "tags": ["LA", "compression", "Madlib", "parallel", "punchy"],
  "macros": {"M1": 0.0, "M2": 0.0, "M3": 0.3, "M4": 0.15},
  "dna": {"brightness": 0.4, "warmth": 0.6, "movement": 0.4, "density": 0.9, "space": 0.3, "aggression": 0.8},
  "parameters": {
    "ofr_transientSnap": 0.7,
    "ofr_transientPitch": 0.4,
    "ofr_transientSat": 0.22,
    "ofr_dustTape": 0.18,
    "ofr_dustVinyl": 0.12,
    "ofr_flipLayers": 2,
    "ofr_cityMode": 2,
    "ofr_cityIntensity": 0.65,
    "ofr_digCuriosity": 0.5,
    "ofr_digComplexity": 0.3,
    "ofr_digFlow": 0.65,
    "ofr_lfo2Rate": 2.17,
    "ofr_lfo2Depth": 0.1,
    "ofr_masterWidth": 0.5,
    "ofr_masterLevel": 0.78,
    "ofr_velToSnap": 0.55,
    "ofr_v0_decay": 0.4,
    "ofr_v0_body": 0.75,
    "ofr_v1_decay": 0.24,
    "ofr_v1_body": 0.55
  }
}
```

---

### Preset 6: Toronto Grid

**Mood:** Foundation
**The Toronto city chain: the Architect.** Toronto processing emphasizes transient precision (Stage 3 differential sharpener: `out + diff × intensity × 0.3`) and a sub-harmonic generator that ducks mid/high content on kick transients. The result is architecturally clean: the sub is separate from the transient, the transient is precisely defined, and the noise floor is reduced. This is the preset for producers who want to place drums in a mix without fighting for space — everything has its defined zone. OFFERING's city chains give it a sonic philosophy, not just a color.

**What it demonstrates:** Toronto sidechain sub duck — a structurally unique feature where the voice's own sub-harmonic ducks its mid/high content on transients. Clean, subtractive production character.

**6D DNA:** brightness 0.6, warmth 0.3, movement 0.2, density 0.7, space 0.4, aggression 0.5

```json
{
  "name": "Toronto Grid",
  "engine": "Offering",
  "mood": "Foundation",
  "tags": ["Toronto", "clean", "sub", "precise", "architectural"],
  "macros": {"M1": 0.0, "M2": 0.0, "M3": 0.0, "M4": 0.0},
  "dna": {"brightness": 0.6, "warmth": 0.3, "movement": 0.2, "density": 0.7, "space": 0.4, "aggression": 0.5},
  "parameters": {
    "ofr_transientSnap": 0.7,
    "ofr_transientPitch": 0.4,
    "ofr_transientSat": 0.12,
    "ofr_dustTape": 0.07,
    "ofr_dustVinyl": 0.08,
    "ofr_cityMode": 3,
    "ofr_cityIntensity": 0.55,
    "ofr_digCuriosity": 0.5,
    "ofr_digComplexity": 0.25,
    "ofr_digFlow": 0.75,
    "ofr_lfo2Rate": 2.17,
    "ofr_lfo2Depth": 0.07,
    "ofr_masterWidth": 0.65,
    "ofr_velToSnap": 0.65,
    "ofr_v0_decay": 0.35,
    "ofr_v0_body": 0.8,
    "ofr_v1_decay": 0.22,
    "ofr_v1_body": 0.45,
    "ofr_v2_decay": 0.06,
    "ofr_v5_decay": 0.045
  }
}
```

---

### Preset 7: The Flip

**Mood:** Entangled
**The Collage Engine at work.** Three layers of the hit stack with 0.85ms, 1.7ms flamming offsets. Chop at 0.3 applies rhythmic amplitude gating at `4 + 0.3×28 = 12.4 Hz` — approximately 12 gates per second, creating a stuttering gate above the drum hits. Ring modulation at 0.2 adds inter-layer metallic intermodulation — when the layers self-ring-modulate, sum and difference frequencies emerge. This is the preset for producers working with the "flip" methodology: take a single drum hit and make it three different sounds simultaneously. FLIP macro sweeps from two-layer flamming all the way to maximum chaos at M3=1.0.

**What it demonstrates:** FLIP collage engine with layer stacking, chop gating, and ring modulation simultaneously active. The interaction between the three collage effects creates emergent complexity from a single drum trigger.

**6D DNA:** brightness 0.6, warmth 0.4, movement 0.7, density 0.7, space 0.5, aggression 0.6

```json
{
  "name": "The Flip",
  "engine": "Offering",
  "mood": "Entangled",
  "tags": ["collage", "layers", "chop", "ring mod", "crate flip"],
  "macros": {"M1": 0.0, "M2": 0.0, "M3": 0.0, "M4": 0.0},
  "dna": {"brightness": 0.6, "warmth": 0.4, "movement": 0.7, "density": 0.7, "space": 0.5, "aggression": 0.6},
  "parameters": {
    "ofr_transientSnap": 0.6,
    "ofr_transientPitch": 0.4,
    "ofr_transientSat": 0.18,
    "ofr_dustVinyl": 0.15,
    "ofr_dustTape": 0.1,
    "ofr_flipLayers": 3,
    "ofr_flipChop": 0.3,
    "ofr_flipRingMod": 0.2,
    "ofr_cityMode": 0,
    "ofr_cityIntensity": 0.4,
    "ofr_digCuriosity": 0.5,
    "ofr_digComplexity": 0.35,
    "ofr_digFlow": 0.55,
    "ofr_lfo1Rate": 0.067,
    "ofr_lfo1Depth": 0.07,
    "ofr_lfo2Rate": 2.17,
    "ofr_lfo2Depth": 0.1,
    "ofr_masterWidth": 0.65,
    "ofr_v0_decay": 0.38,
    "ofr_v1_decay": 0.25,
    "ofr_v3_decay": 0.55
  }
}
```

---

### Preset 8: SP-1200 Dig

**Mood:** Submerged
**The SP-1200 mythology fully realized.** The SP-1200 famously operated at 12-bit depth and 26.04kHz sample rate — its grit was a function of the hardware, not an aesthetic choice, but producers fell in love with it anyway. This preset engages bit depth at 12, sample rate at 26040, and New York city chain at full intensity. Vinyl crackle at 0.35 adds the Poisson-distributed pops of a dusty record. Tape at 0.15 adds pink noise floor and saturation. The result is unmistakably SP-1200: everything sounds like it was sampled from a record, chopped, and re-pitched. DIG macro increases curiosity for increasingly wild variations; DUST macro deepens the degradation from warm lo-fi to destroyed.

**What it demonstrates:** Bit crush + sample rate reduction + vinyl crackle working together to recreate a historically specific hardware sonic character. The curiosity engine adds the sense of digging through a crate of vinyl.

**6D DNA:** brightness 0.3, warmth 0.7, movement 0.4, density 0.7, space 0.2, aggression 0.6

```json
{
  "name": "SP-1200 Dig",
  "engine": "Offering",
  "mood": "Submerged",
  "tags": ["SP-1200", "lo-fi", "bit crush", "vinyl", "sampler"],
  "macros": {"M1": 0.0, "M2": 0.0, "M3": 0.0, "M4": 0.0},
  "dna": {"brightness": 0.3, "warmth": 0.7, "movement": 0.4, "density": 0.7, "space": 0.2, "aggression": 0.6},
  "parameters": {
    "ofr_transientSnap": 0.6,
    "ofr_transientPitch": 0.42,
    "ofr_transientSat": 0.2,
    "ofr_dustVinyl": 0.35,
    "ofr_dustTape": 0.15,
    "ofr_dustBits": 12,
    "ofr_dustSampleRate": 26040.0,
    "ofr_dustWobble": 0.08,
    "ofr_cityMode": 0,
    "ofr_cityIntensity": 0.55,
    "ofr_digCuriosity": 0.5,
    "ofr_digComplexity": 0.3,
    "ofr_digFlow": 0.6,
    "ofr_lfo2Rate": 2.17,
    "ofr_lfo2Depth": 0.08,
    "ofr_masterWidth": 0.55,
    "ofr_v0_decay": 0.38,
    "ofr_v0_body": 0.7,
    "ofr_v1_decay": 0.22,
    "ofr_v3_decay": 0.5,
    "ofr_v6_decay": 0.4
  }
}
```

---

### Preset 9: City Crossfade

**Mood:** Prism
**The shadow-chain blend demonstration.** CityBlend set to 0.5 places the engine exactly between two city chains running in parallel, crossfaded with equal-power gain staging. At cityMode=1 (Detroit) + cityBlend=0.5, the engine runs Detroit processing on one copy of the signal and LA processing on another, then combines them at equal power. The result is a hybrid: Detroit's warmth and drunk timing character combined with LA's heavy compression and bass shelf boost. The CITY macro morphs between all 5 cities in sequence — at M2=0.5 you are in the LA→Toronto blend, at M2=1.0 you are in the Bay Area. This is the only preset in this library that uses two city chains simultaneously.

**What it demonstrates:** Shadow-chain blending. The city crossfade system running at its most audible — two complete processing chains at equal weight, creating a hybrid psychoacoustic character unavailable from any single city.

**6D DNA:** brightness 0.4, warmth 0.65, movement 0.45, density 0.8, space 0.45, aggression 0.65

```json
{
  "name": "City Crossfade",
  "engine": "Offering",
  "mood": "Prism",
  "tags": ["city blend", "Detroit", "LA", "hybrid", "crossfade"],
  "macros": {"M1": 0.1, "M2": 0.0, "M3": 0.0, "M4": 0.1},
  "dna": {"brightness": 0.4, "warmth": 0.65, "movement": 0.45, "density": 0.8, "space": 0.45, "aggression": 0.65},
  "parameters": {
    "ofr_transientSnap": 0.55,
    "ofr_transientPitch": 0.4,
    "ofr_transientSat": 0.22,
    "ofr_dustTape": 0.14,
    "ofr_dustVinyl": 0.18,
    "ofr_dustWobble": 0.07,
    "ofr_cityMode": 1,
    "ofr_cityBlend": 0.5,
    "ofr_cityIntensity": 0.6,
    "ofr_digCuriosity": 0.5,
    "ofr_digComplexity": 0.4,
    "ofr_digFlow": 0.6,
    "ofr_lfo1Rate": 0.067,
    "ofr_lfo1Depth": 0.06,
    "ofr_lfo2Rate": 2.17,
    "ofr_lfo2Depth": 0.1,
    "ofr_masterWidth": 0.55,
    "ofr_velToSnap": 0.6,
    "ofr_velToBody": 0.4,
    "ofr_v0_decay": 0.42,
    "ofr_v0_body": 0.75,
    "ofr_v1_decay": 0.25,
    "ofr_v4_decay": 0.28
  }
}
```

---

### Preset 10: Flow State

**Mood:** Aether
**Csikszentmihalyi at the wheel.** This preset demonstrates the Flow parameter at maximum: every hit reuses the previous variation set with 90% probability. The groove locks in. Combined with high curiosity (0.65, well into the Berlyne optimal zone) and low complexity (0.25 — only tune and decay vary), the engine produces a pattern that is familiar enough to feel like a groove but varied enough to feel alive. This is what Csikszentmihalyi meant: challenge matching skill means the drum pattern is just different enough to stay interesting without disrupting the listener's predictive model. The Bay Area city chain adds allpass diffusion — the groove floats.

**What it demonstrates:** Curiosity engine's flow parameter at maximum expression. The difference between flow=0.9 (this preset: nearly locked groove with subtle drift) and flow=0.0 (every hit completely fresh). The engine as a groove-sustaining machine.

**6D DNA:** brightness 0.4, warmth 0.5, movement 0.5, density 0.5, space 0.65, aggression 0.4

```json
{
  "name": "Flow State",
  "engine": "Offering",
  "mood": "Aether",
  "tags": ["flow", "locked groove", "hypnotic", "Bay Area", "psychology"],
  "macros": {"M1": 0.0, "M2": 0.0, "M3": 0.0, "M4": 0.0},
  "dna": {"brightness": 0.4, "warmth": 0.5, "movement": 0.5, "density": 0.5, "space": 0.65, "aggression": 0.4},
  "parameters": {
    "ofr_transientSnap": 0.6,
    "ofr_transientPitch": 0.38,
    "ofr_transientSat": 0.15,
    "ofr_dustTape": 0.09,
    "ofr_dustVinyl": 0.12,
    "ofr_dustWobble": 0.06,
    "ofr_cityMode": 4,
    "ofr_cityIntensity": 0.45,
    "ofr_digCuriosity": 0.65,
    "ofr_digComplexity": 0.25,
    "ofr_digFlow": 0.9,
    "ofr_lfo1Rate": 0.067,
    "ofr_lfo1Depth": 0.09,
    "ofr_lfo2Rate": 2.17,
    "ofr_lfo2Depth": 0.07,
    "ofr_masterWidth": 0.68,
    "ofr_velToSnap": 0.55,
    "ofr_velToBody": 0.45,
    "ofr_v0_decay": 0.4,
    "ofr_v0_body": 0.72,
    "ofr_v1_decay": 0.24,
    "ofr_v3_decay": 0.65
  }
}
```

---

## Phase R6: New Scripture — The Book of Bin, OFFERING Verses

---

### Psalm 12 (The Mantis Truth)
*On the nature of percussion synthesis*

The drum hit is not a sound. It is a decision.
The instrument does not produce audio — it commits to an acoustic event that cannot be undone.
Every transient is a hypothesis about what a hit should be.
The first millisecond is the word; everything after is the sentence that follows.
Design the word. The sentence will come.

*Application: the first 5ms of every drum voice must be immediately compelling without any processing downstream of the transient generator. The transient IS the drum.*

---

### Sutra 12 (The Curiosity Floor)
*On the calibration of variation*

Too much stillness and the ear forgets to listen.
Too much change and the mind cannot find the ground.
The Berlyne peak at 0.5 is not a compromise. It is the destination.
Variation range at maximum interesting is not the same as variation at maximum amplitude.
Let every hit land differently enough to say something.
Let every hit land similarly enough that the something can be heard.

*Application: curiosity=0.5 is the correct default. Do not move it toward 1.0 seeking excitement — you will break the groove. Do not move it toward 0.0 seeking stability — you will bore the listener. 0.5 is the mantis shrimp's perceptual resolution operating at full capacity.*

---

### Canon 12 (The City Is Not an Effect)
*On psychoacoustic processing philosophy*

The compressor is not a tool for controlling loudness.
The compressor is a cultural statement about what a transient should feel like.
New York believes transients should punch and gate.
Detroit believes transients should breathe and accumulate.
Los Angeles believes transients should be simultaneously loud and alive.
Toronto believes transients should be precisely placed and leave sub-bass room.
Bay Area believes transients should fog into the space around them.
These are not settings. They are philosophies.
Apply a city chain the way you choose a neighborhood to record in.

*Application: city processing is best used at cityIntensity between 0.35 and 0.65. Below 0.35 the city character is inaudible. Above 0.65 the stereo field collapses to mono and the city processing begins to dominate the drum timbre rather than inform it. The Rubble Zone for city intensity is 0.35–0.65.*

---

### Truth 14 (The Locked Groove)
*On the Flow State in percussion*

Csikszentmihalyi was not describing music. He was describing the brain's willingness to stay present.
In drumming, the Flow State is reached when the pattern is exactly complex enough to require attention without demanding interpretation.
A drum machine locked at flow=0.9 says: I have found a groove, and I will stay in it, and I will not let you leave.
A drum machine at flow=0.0 says: I will surprise you on every hit, and you will never find the ground.
Neither is a performance. Both are abstractions.
The Mantis Shrimp lives between them.
It has a groove. The groove shifts. The shift is earned.

*Application: flow=0.6–0.8 is the production sweet spot. At 0.6, 60% of hits reuse the previous variation — the groove exists but breathes. At 0.8, 80% reuse — the groove is locked, variations are rare events that stand out. Use flow=0.9+ only for intentionally hypnotic loops. Use flow=0.3 or below when building builds and drops where disruption is the point.*

---

## Phase R7: Closing Meditation

You have been in the rubble zone for a long time now. The Mantis Shrimp does not perform. It investigates.

Every drum hit in OFFERING is a test strike: sent out to understand the acoustic space, returned to the ear as information about what the drum world contains today. The curiosity engine remembers what it found on the last hit. The flow parameter decides whether today's hit is a new inquiry or a confirmation of the last one.

The five cities are five ways to answer the question: *what does a boom bap drum kit mean?* New York says it means archaeological grit and precise rhythm. Detroit says it means human timing and accumulated warmth. Los Angeles says it means aggressive compression and layered texture. Toronto says it means architectural clarity and defined sub. Bay Area says it means dark atmosphere and foggy diffusion.

You don't have to choose. You can blend.

The Mantis Shrimp has 16 photoreceptor types. It doesn't combine them into a single image. Each channel remains distinct: one for motion, one for ultraviolet, one for polarized light, one for depth. The richness comes not from mixing everything into a blended composite, but from maintaining independent perception channels that report simultaneously to a mind capable of acting on all of them at once.

OFFERING works the same way. The transient, the texture, the collage, the city, and the curiosity engine are not layered on top of each other. They run in sequence, each one operating on the output of the last, each one asking a different question about what this drum hit should be.

Trust the Berlyne curve. Trust the Wundt density. Trust the Flow balance. Trust that 0.5 is the correct curiosity setting, 0.35–0.65 is the correct city intensity range, and 60% flow reuse is the correct production default.

And then break all of it. That's why the macros are there.

---

*Retreat complete — 2026-03-21*
*OFFERING Engine #46 — XO_OX Designs*
*Crate Wax Yellow · #E5B80B · The Mantis Shrimp of the Rubble Zone*
