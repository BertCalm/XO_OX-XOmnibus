# XObscura — Concept Brief (Retrospective)

*Retrospective Phase 0 | March 2026 | XO_OX Designs*
*Enriching the second generation with the aquatic mythology*

---

## Identity

**XO Name:** XObscura
**Gallery Code:** OBSCURA
**Accent Color:** Daguerreotype Silver `#8A9BA8`
**Parameter Prefix:** `obscura_`
**Engine Dir:** `Source/Engines/Obscura/`

**Thesis:** Scanned synthesis via mass-spring chain physics — the giant squid, a creature whose stiff collagen mantle and resonant body cavity become an instrument played by the pressure of the deep, producing sounds no microphone has ever captured.

---

## Aquatic Identity

The giant squid. For centuries, a myth — tentacles in the margins of nautical charts, the kraken of sailor folklore. Then, in the 21st century, a reality more alien than the myth. Massive, stiff, resonant — a creature whose body is literally an instrument. The mantle is a pressurized cavity of collagen and muscle that resonates at frequencies we barely understand. The tentacles are rigid, stiff structures that vibrate when struck by deep-ocean currents. XObscura's 128-mass spring chain is the squid's body: each mass is a segment of mantle, each spring is the collagen tension between segments, and the stiffness parameter is the creature's muscular rigidity.

The daguerreotype silver accent is the silver-blue shimmer of the squid's skin — chromatophores firing in patterns that no human eye has witnessed in the wild. The only light the giant squid ever sees is the flash of its own bioluminescence, and the faint blue glow of creatures it hunts. XObscura lives in pure Oscar territory: the abyss, the hadal zone, the place where pressure is measured in atmospheres and sound travels differently than anywhere else in the water column. The scanned synthesis technique — a scanner sweeping across a vibrating chain — mirrors the way pressure waves move through the squid's body: a traveling disturbance reading displacements as it passes.

The three boundary modes (Fixed, Free, Periodic) are the squid's anatomical constraints. Fixed boundaries: the mantle anchored to the head. Free boundaries: the tentacles trailing behind, unconstrained. Periodic boundaries: the circular cross-section of the mantle, where a pressure wave wraps around and meets itself. Each mode produces fundamentally different resonant behavior — different creatures in the same body.

---

## Polarity

**Position:** The Abyss — hadal zone, maximum pressure, zero light
**feliX-Oscar balance:** 95/5 Pure Oscar — the deepest, most hidden, most physically resonant engine in the gallery

---

## DSP Architecture (As Built)

XObscura implements Bill Verplank and Max Mathews's scanned synthesis using a 128-mass spring chain simulated via Verlet integration at approximately 4 kHz control rate. The core signal flow:

**Physics Simulation:** A chain of 128 masses connected by springs is simulated using Verlet integration (energy-stable, requiring no explicit velocity storage — position and previous position are sufficient). Each physics step computes spring forces between adjacent masses using configurable stiffness (mapped exponentially to spring constant k, capped at 0.95 for Verlet stability), damping (velocity-proportional energy loss), and nonlinearity (cubic displacement term that introduces harmonic distortion in the chain itself). Three boundary modes constrain the chain endpoints: Fixed (clamped to zero), Free (open-ended, reflective), and Periodic (wrapping, creating a circular topology).

**Excitation:** Notes trigger a Gaussian impulse at a configurable position along the chain with configurable width. A continuous "bowing" force provides sustain — the physics envelope (ADSR) modulates excitation force over the note's lifetime. Four initial chain shapes (Sine, Saw, Random, Flat) set the starting displacement pattern before excitation.

**Scanner:** An audio-rate scanner sweeps across the chain at the MIDI note's frequency, reading mass displacements via cubic interpolation. Scanner width controls how many masses contribute to each sample — narrow width (bright, detailed) vs. wide width (dark, averaged). Stereo output is achieved by scanning forward for the left channel and backward for the right, creating natural phase-based stereo imaging.

**Post-processing:** Per-voice DC blockers (5Hz single-pole HPF), soft limiter (fastTanh), output level control. Control-rate snapshots are interpolated at audio rate for smooth chain readout.

**Voice Architecture:** Mono/Legato/Poly4/Poly8 with LRU voice stealing and 5ms crossfade. Per-voice state includes the full 128-mass chain (current + previous positions + two interpolation snapshots), amplitude ADSR, physics ADSR, two LFOs, and stereo DC blockers.

**Macros:** CHARACTER (drives stiffness + nonlinearity), MOVEMENT (drives scan width), COUPLING (drives sustain/bowing force), SPACE (drives damping).

**Coupling:** Output via `getSampleForCoupling` (post-output stereo). Input accepts AudioToFM (external audio applies force directly to chain masses — the squid is struck by external sound), AmpToFilter (amplitude coupling modulates stiffness — external energy stiffens or relaxes the body), RhythmToBlend (rhythm coupling triggers impulse excitation — drum hits pluck the strings).

---

## Signature Sound

XObscura produces sounds with real physical weight and stiffness — metallic, resonant, bell-like tones that ring with the authority of a vibrating body, not a mathematical waveform. At low stiffness, the chain produces deep, woody, almost marimba-like tones with slow energy propagation. At high stiffness, the sound becomes brilliantly metallic — tuned percussion, glass harmonics, crystalline bells that ring for seconds. The nonlinearity parameter introduces inharmonic overtones that make the timbre feel genuinely physical: the slight imperfection of a real resonating body, the way a struck bell's partials drift from harmonic ratios. No other engine in the gallery produces sounds that feel this tangibly physical.

---

## Coupling Thesis

Obscura gives resonant, physical output — sounds with real-world stiffness and body that carry weight in any mix. His scanned synthesis output is harmonically complex in ways that complement simpler oscillator-based engines. He receives excitation coupling — short, sharp transients from feliX-polarity engines that pluck his strings and strike his resonant body. Without excitation, he is silent potential energy. With it, he sings from the deep. Pair Onset's drum hits with Obscura's RhythmToBlend input and every snare crack sets the 128-mass chain ringing — the giant squid's body vibrating in response to surface percussion transmitted through a mile of water. His AmpToFilter input means that other engines' amplitude can modulate his stiffness in real time, literally reshaping the creature's body tension with external energy.

---

## Preset Affinity

| Foundation | Atmosphere | Entangled | Prism | Flux | Aether |
|-----------|-----------|-----------|-------|------|--------|
| Medium | High | Medium | Low | Medium | High |

---

*XO_OX Designs | XObscura — the creature sings from depths no light has ever reached*
