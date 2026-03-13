# XOblique — Concept Brief (Retrospective)

*Retrospective Phase 0 | March 2026 | XO_OX Designs*
*Enriching the second generation with the aquatic mythology*

---

## Identity

**XO Name:** XOblique
**Gallery Code:** OBLIQUE
**Accent Color:** Prism Violet `#BF40FF`
**Parameter Prefix:** `oblq_`
**Engine Dir:** `Source/Engines/Oblique/`

**Thesis:** Prismatic bounce synthesis — dual oscillators through a wavefolder and 6-tap spectral delay prism, with bouncing-ball percussive ricochets and psychedelic phaser swirl. Light hitting water at an angle, refracting into a spectrum of color. Run the Jewels meets Funk meets Tame Impala.

---

## Aquatic Identity

Light hitting water at an angle. Every swimmer has seen it: sunlight entering a swimming pool refracts at the surface, bounces off the floor, the walls, the water itself — and the pool floor shimmers with dancing caustic patterns, white light fractured into shifting prismatic geometry. XOblique is that refraction made audible. Sound enters the engine at one angle — a simple oscillator tone, a punchy transient — and bounces off internal surfaces, each reflection picking up a different spectral color, until what emerges is something far more complex and vivid than what went in.

The prism violet accent is the most refracted wavelength in the visible spectrum. When white light passes through a glass prism, violet bends the most — it is the color that travels the furthest from its original trajectory. This is XOblique's personality: every sound that enters is bent, refracted, scattered into spectral components that land in unexpected places across the stereo field. The 6-tap ObliquePrism delay is literally a sonic prism: six facets, each filtered at a different frequency (sub through brilliance, mapped to the light spectrum from red to violet), each delayed by a Fibonacci-ratio time offset, each panned to a different stereo position. Sound enters as white light. It exits as a rainbow.

The bouncing-ball generator (ObliqueBounce) extends the refraction metaphor into rhythm. A ball of light dropped between two mirrors doesn't just bounce — it accelerates, each reflection closer to the last, each impact dimmer than the one before, until the bounces converge into a blur of diminishing clicks. This is how light behaves in a house of mirrors: ricocheting, multiplying, losing energy at each surface. XOblique lives in feliX's shallows because refraction is a surface phenomenon — it happens where light meets water, where air meets glass, where the medium changes. The deeper you go, the less light there is to refract. Oblique needs brightness to work with.

---

## Polarity

**Position:** Sunlit Shallows — the surface refraction zone where light bends at the water interface
**feliX-Oscar balance:** 70/30 feliX-leaning — bright, energetic, rhythmically bold, but the phaser swirl and spectral delay carry Oscar's psychedelic patience

---

## DSP Architecture (As Built)

XOblique implements a prismatic bounce synthesis architecture combining subtractive synthesis, wavefolding, spectral delay, percussive physics, and psychedelic phase modulation. The signal flow:

**Dual Oscillator:** Two PolyBLEP oscillators (OscA primary, OscB detuned for stereo width) generate the "light beam" — the sustained tonal foundation. Five waveform modes: Sine, Saw, Square, Pulse, Triangle. OscB is detuned by a configurable amount (0-50 cents) for natural width without chorus. Both oscillators sum to mono for voice processing.

**Wavefolder (ObliqueWavefolder):** A two-stage sine wavefolder adds harmonic grit inspired by El-P's Run the Jewels production aesthetic. At low fold amounts, gentle saturation warms the tone. At high fold, a second fold pass generates metallic overtones and complex harmonic content. Output is soft-clipped via fastTanh to prevent runaway levels. The fold amount parameter is the "angle of incidence" — how hard the light beam hits the first surface.

**Voice Filter:** Per-voice Cytomic SVF lowpass filter with cutoff and resonance control. Shapes the folded waveform before the envelope, controlling how much of the wavefolder's harmonic content reaches the prism stage.

**Bounce Engine (ObliqueBounce):** Per-voice bouncing-ball rhythm generator triggered on every note-on. Models a ball dropping with exponentially decreasing intervals (gravity ratio per bounce) and exponentially decreasing velocity (damping per bounce). Each bounce fires a short sine-tone click at a configurable frequency (200-8000Hz) with ~3ms exponential decay. Parameters include rate (initial interval 20-500ms), gravity (interval shrink 0.3-0.95), damping (velocity decay 0.3-0.95), max bounces (2-16), swing (offsets even-numbered bounces), and click tone frequency. The bounce output rides on top of the sustained beam as percussive fragments ricocheting off mirrors.

**Amplitude Envelope:** Per-voice ADSR with fast attack capability (down to 1ms) for percussive sounds or slow attack for pad-like swells. The envelope shapes the combined beam + bounce signal.

**Prism Delay (ObliquePrism):** The engine's signature effect — a 6-tap spectral delay that splits the stereo mix into six "facets," each a delay tap filtered at a different frequency and panned to a different stereo position. Facet frequencies map audio spectrum to light spectrum: Red (100Hz sub), Orange (300Hz body), Yellow (800Hz mid), Green (2kHz bite), Blue (5kHz presence), Violet (12kHz sparkle). Delay times are spread in Fibonacci ratios (1.0, 1.618, 2.0, 2.618, 3.236, 4.236) for rhythmically interesting patterns. The colorSpread parameter controls frequency convergence (at 0, all filters converge to 1kHz; at 1, full spectral separation). Feedback between facets with lowpass damping creates kaleidoscopic multiplication — sound fragments bouncing between mirrors, each reflection gaining a new spectral color while losing high-frequency energy.

**Phaser (ObliquePhaser):** A 6-stage allpass phaser applied post-prism for Tame Impala psychedelic swirl. LFO-modulated allpass cascade (Cytomic SVF allpass mode) creates sweeping notches across 200-4000Hz. Stereo processing with independent left/right cascades. Feedback with soft-clip protection for deep, resonant sweeps without instability. Applied after the prism delay for kaleidoscopic phase cancellation — the refracted light shimmers.

**Voice Architecture:** 8-voice polyphony with LRU voice stealing. Each voice carries independent dual oscillators, wavefolder state, filter, envelope, and bounce generator. Prism delay and phaser are shared (post-voice-mix) effects.

**Coupling:** Output via `getSampleForCoupling` (post-prism, post-phaser stereo). Input accepts AudioToFM (modulates oscillator frequency — external audio bends the light beam), AmpToFilter (amplitude coupling drives filter cutoff — external energy opens the prism), EnvToMorph (envelope coupling drives wavefold amount — external dynamics reshape the refraction angle).

---

## Signature Sound

XOblique sounds like light made audible — vibrant, bouncing, prismatic. The bouncing-ball clicks on top of sustained wavefolder tones create a unique percussive-melodic hybrid: funky bass lines with ricocheting ghost notes, leads that fire a burst of diminishing clicks on every note like a mirror ball catching a spotlight. The 6-tap prism delay spreads each note across the stereo field in spectrally colored echoes, and the phaser adds psychedelic swirl that makes the whole thing shimmer. At its funkiest, it sounds like Bootsy Collins' bass through a kaleidoscope. At its most psychedelic, it sounds like Kevin Parker's guitars refracting through a swimming pool. At its most aggressive, it sounds like El-P's productions bouncing off concrete walls. No other engine in the gallery combines percussive physics, spectral splitting, and psychedelic phase modulation into a single voice architecture.

---

## Coupling Thesis

Oblique gives colored, refracted output — sounds with unexpected harmonic content created by internal bouncing and spectral splitting. His post-prism output is rich in rhythmically scattered spectral fragments that make extraordinary coupling source material. Feed Oblique's output into Overdub's tape delay and the prismatic echoes degrade into dub rainbows. Feed it into Obscura's mass-spring chain and the spectral fragments excite different resonant modes along the chain. He receives direct, punchy input well: feliX's transient snaps modulate the wavefold amount (more energy = more harmonic refraction), Onset's drum hits drive the filter open (the prism catches the light), and Overdub's echo feeds back into the oscillator frequency for pitch-bending dub refractions. The more energy you put in, the more colors come out. He is the shallow-water prism — the point where simple white light becomes a spectrum.

---

## Preset Affinity

| Foundation | Atmosphere | Entangled | Prism | Flux | Aether |
|-----------|-----------|-----------|-------|------|--------|
| Medium | Low | Medium | High | High | Low |

---

*XO_OX Designs | XOblique — white light enters, a spectrum emerges*
