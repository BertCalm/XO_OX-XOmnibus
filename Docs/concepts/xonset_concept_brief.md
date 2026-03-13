# XOnset — Concept Brief (Retrospective)

*Retrospective Phase 0 | March 2026 | XO_OX Designs*
*Enriching the first generation with the aquatic mythology*

---

## Identity

**XO Name:** XOnset
**Gallery Code:** ONSET
**Accent Color:** Electric Blue `#0066FF`
**Parameter Prefix:** `onset_`
**Engine Dir:** `Source/Engines/Onset/`

**Thesis:** Algorithmic drum synthesis -- FM, modal, Karplus-Strong, phase distortion. Splashes on the surface, precise and electric.

---

## Aquatic Identity

Surface splashes. The sharp percussion of water breaking -- a fish leaping clear of the surface, a tail slapping the water to stun prey, rain striking the ocean in a downpour. Every drum voice in XOnset is a different kind of impact event, and the engine's dual-layer architecture (Layer X circuit models + Layer O algorithmic synthesis) mirrors the physics of impact: the initial collision (transient) and the resonant aftermath (body). The kick is a body entering water from height -- the BridgedT oscillator's pitch envelope sweeps down like displacement pressure. The snare is a wave breaking on rock -- the NoiseBurst circuit's dual-sine body with high-passed noise burst. The hi-hats are rain -- the MetallicOsc's six inharmonic square waves through bandpass filtering, the bright patter of water droplets on the ocean surface.

The Cross-Voice Coupling system is schooling behavior made audible. In a school of fish, each individual reacts to the movement of its neighbors -- a flash of silver triggers a chain reaction, a single darting fish redirects the entire school. XOnset's XVC works the same way: the peak amplitude of each voice can modulate the parameters of other voices through an eight-by-eight coupling matrix. When the kick fires, it can pump the snare's filter. When the hat triggers, it can choke the open hat. This is not a sequencer -- it is an ecosystem of rhythmic organisms reacting to each other in real time.

The electric blue accent is the color of surface water in direct sunlight -- the brightest, most energetic blue in the ocean. It is feliX's color, darkened slightly for authority. XOnset lives where feliX lives, at the surface where energy is highest and every sound is an event, not a process. Where Oscar's engines sustain, XOnset strikes. Where Oscar breathes, XOnset snaps.

---

## Polarity

**Position:** The Surface -- where energy is highest and sounds are sharpest
**feliX-Oscar balance:** 90% feliX / 10% Oscar -- pure percussion, but the modal resonator and Karplus-Strong have sustain and body

---

## DSP Architecture (As Built)

```
8 FIXED VOICES (drum kit layout)
|
V1: Kick     (BridgedT circuit  + algo select, MIDI note 36, base 55 Hz)
V2: Snare    (NoiseBurst circuit + algo select, MIDI note 38, base 180 Hz)
V3: CH Hat   (Metallic circuit   + algo select, MIDI note 42, base 8000 Hz)
V4: OH Hat   (Metallic circuit   + algo select, MIDI note 46, base 8000 Hz)
V5: Clap     (NoiseBurst circuit + algo select, MIDI note 39, base 1200 Hz, clap mode)
V6: Tom      (BridgedT circuit   + algo select, MIDI note 45, base 110 Hz)
V7: Perc A   (BridgedT circuit   + algo select, MIDI note 37, base 220 Hz)
V8: Perc B   (Metallic circuit   + algo select, MIDI note 44, base 440 Hz)
|
PER VOICE:
|
+-- LAYER X (Circuit Models -- the "body")
|   +-- BridgedTOsc: TR-808-style decaying sine + pitch envelope + sub triangle
|   +-- NoiseBurstCircuit: dual-sine body + HP-filtered noise burst (clap=3 bursts)
|   +-- MetallicOsc: 6-oscillator 808 metallic (square waves at inharmonic ratios
|       through dual bandpass + highpass CytomicSVF)
|
+-- LAYER O (Algorithmic Synthesis -- the "character")
|   +-- FMPercussion: 2-operator FM, self-feedback, decaying mod envelope
|   +-- ModalResonator: 8-mode parallel CytomicSVF bandpass bank
|       (membrane ratios, inharmonicity control, noise excitation)
|   +-- KarplusStrongPerc: delay line + averaging filter + noise burst excitation
|   +-- PhaseDistPerc: Casio CZ-inspired DCW envelope + 3 wave shapes
|
+-- Equal-power crossfade blend (Layer X <-> Layer O)
+-- OnsetTransient: pitch spike + noise burst (pre-blend click/snap)
+-- CytomicSVF per-voice filter (LP, tone-scaled cutoff)
+-- OnsetEnvelope: AD / AHD / ADSR percussion envelope
|
+-> Per-voice level * velocity -> Stereo pan -> Voice mix
|
MASTER CHAIN:
|
+-- OnsetCharacterStage (tanh saturation "grit" + LP "warmth")
+-- OnsetDelay (dark tape-style, LP in feedback path, 2-second max)
+-- OnsetReverb (4 comb + 2 allpass diffusers, Freeverb topology)
+-- OnsetLoFi (bitcrush + sample-rate reduction)
+-- Master filter (CytomicSVF LP)
|
+-> Stereo output
```

**Per-voice parameters (x8):** pitch, decay, tone, snap, body, character, blend, algo mode, level, pan, env shape (88 per-voice params total).
**Global parameters:** 4 macros (MACHINE, PUNCH, SPACE, MUTATE), 6 XVC, 2 character, 8 FX. 111 total.
**Coupling:** Audio output + per-voice peak amplitudes for XVC. Receives AmpToChoke, pattern modulation.

---

## Signature Sound

XOnset is the only engine in XOmnibus that models percussion from physics rather than sampling it. The dual-layer architecture -- analog circuit models (BridgedT, NoiseBurst, Metallic) blended with algorithmic synthesis (FM, Modal, Karplus-Strong, Phase Distortion) -- means every drum hit exists on a continuum between "familiar analog machine" and "alien synthesized percussion." The blend knob per voice is the key: at zero, you get an 808 kick; at one, you get an FM percussion hit tuned to the same frequency; in between, you get something that has never existed in hardware. The Cross-Voice Coupling system then lets these eight voices react to each other, creating kit-wide behaviors that evolve with every hit.

---

## Coupling Thesis

Onset is the pulse. He drives everything -- amplitude coupling from drum hits pumps filters, triggers envelopes, gates other engines across the gallery. His eight voices are eight independent coupling sources, each with its own peak amplitude available for cross-engine modulation. The kick drives Overbite's Bite macro, making the bass snarl on every downbeat. The snare opens Oblong's filter, blooming the coral with every backbeat. The hat chokes through Overdub's delay, creating rhythmic echo patterns. Onset receives sparingly -- pattern modulation and density control from external sources can reshape his rhythm, but his precision must not be killed. He is the school of fish: fast, coordinated, reacting to each other before any individual can think.

---

## Preset Affinity

| Foundation | Atmosphere | Entangled | Prism | Flux | Aether |
|-----------|-----------|-----------|-------|------|--------|
| High | Low | High | Medium | High | Low |

---

*XO_OX Designs | XOnset -- eight splashes on the surface, each one a different kind of breaking water*
