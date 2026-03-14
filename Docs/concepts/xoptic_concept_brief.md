# XOptic — Concept Brief (Retrospective)

*Retrospective Phase 0 | March 2026 | XO_OX Designs*
*Enriching the second generation with the aquatic mythology*

---

## Identity

**XO Name:** XOptic
**Gallery Code:** OPTIC
**Accent Color:** Phosphor Green `#00FF41`
**Parameter Prefix:** `optic_`
**Engine Dir:** `Source/Engines/Optic/`

**Thesis:** Audio-reactive visual modulation engine with AutoPulse — bioluminescent plankton at the surface, converting sound into rhythmic light and light back into modulation. The synesthete of the water column: hearing light, seeing sound.

---

## Aquatic Identity

Bioluminescent plankton at the surface. On certain nights, when waves break in warm tropical waters, the ocean glows — millions of dinoflagellates firing in synchronized flashes of phosphor green, each organism responding to the mechanical energy of the water's movement. Touch the surface and light erupts. Paddle through it and your wake becomes a luminous trail. This is XOptic: a creature that converts kinetic energy into light, and light into a signal that propagates through the colony.

The phosphor green accent is the exact color of bioluminescent plankton — nature's own LED, the same green as a CRT monitor's phosphor glow. This is not coincidence. XOptic is explicitly a Winamp-inspired audio-reactive visualizer that also modulates sound. It lives in the same conceptual space as the demoscene visualizers of the late 1990s, where audio became visual became rhythmic became hypnotic. The 8-band spectral analyzer is the plankton colony's distributed nervous system: sub, bass, lo-mid, mid, hi-mid, presence, brilliance, air — eight frequency bands mapped, tracked, and converted into modulation signals that flow back into the coupling matrix.

XOptic lives in feliX's shallows because bioluminescence is a surface phenomenon. It requires movement, energy, disturbance — the same qualities that define feliX's darting, transient-forward personality. But where feliX initiates, Optic responds. feliX is the neon tetra that darts through the plankton. Optic is the green flash that trails behind him. The AutoPulse mode is the colony achieving synchronization without external stimulus — a self-evolving trance rhythm that emerges from the spectral state of whatever audio is flowing through the system, breathing and shifting without player input, the plankton pulsing in the dark.

---

## Polarity

**Position:** Sunlit Shallows — the surface interface where light and water meet
**feliX-Oscar balance:** 70/30 feliX-leaning — reactive, luminous, rhythmic, but with the patience of a colony organism that pulses on its own metabolic clock

---

## DSP Architecture (As Built)

XOptic is unique in the XOmnibus gallery: it generates no audio of its own. It is a pure modulation engine — an analyzer and rhythm generator that processes incoming sound and outputs control signals through the coupling matrix. Zero voices. Zero oscillators. Pure signal intelligence.

**8-Band Spectral Analyzer (OpticBandAnalyzer):** Cascaded Cytomic SVF bandpass filter pairs split incoming audio into 8 frequency bands (Sub 20-80Hz, Bass 80-200Hz, Lo-Mid 200-500Hz, Mid 500-1kHz, Hi-Mid 1-4kHz, Presence 4-8kHz, Brilliance 8-16kHz, Air 16-20kHz). Each band's energy is tracked via rectification and envelope follower lowpass filters (~30Hz cutoff for smooth tracking). The analyzer computes spectral centroid (energy-weighted average band position, normalized 0-1) and spectral flux (rate of change of total energy) per sample — all without FFT overhead.

**AutoPulse (OpticAutoPulse):** A self-oscillating rhythm generator that produces trance-like pulse patterns without MIDI input. The pulse is shaped like a kick drum: exponential amplitude decay with configurable shape (sharp kick at 0, soft swell at 1). Parameters include rate (0.5-16Hz), swing, subdivision (whole/quarter/8th/16th), accent strength on downbeats, and an evolve parameter that feeds spectral analysis back into the pulse character. The spectral centroid drifts the rate, energy surges speed it up, and spectral flux adds stochastic accent variation. A soft drift LFO provides organic rate variation. The result is emergent trance patterns that breathe and shift in response to the audio they are analyzing.

**Modulation Output Bus (OpticModOutputs):** 8 lock-free atomic channels written on the audio thread, readable by the UI thread for visualization and by the coupling matrix for modulation routing: AutoPulse level, bass energy, mid energy, high energy, spectral centroid, spectral flux, total energy, and transient detection (flux threshold gate). All mod outputs are smoothed through dedicated Cytomic SVF lowpass filters to prevent zipper noise.

**Coupling Composite:** The final coupling output blends pulse-driven and spectrum-driven modulation signals via `optic_modMixPulse` and `optic_modMixSpec` parameters, scaled by `optic_modDepth`. This composite signal is cached per-sample for coupling reads. Extended coupling channels (3-10) expose individual mod outputs for granular routing.

**Input:** Receives audio from other engines via coupling (AudioToFM, AudioToRing, AudioToWavetable, FilterToFilter all accepted — any audio input is summed for spectrum analysis). AmpToFilter and AmpToPitch coupling modulate reactivity. RhythmToBlend coupling feeds into the analysis input.

**Visualizer Parameters:** Four visualization modes (Scope, Spectrum, Milkdrop, Particles) with feedback, speed, and intensity controls expose the spectral state to the UI layer.

---

## Signature Sound

XOptic has no sound of its own — and that is its signature. Its presence is felt through the behavior of every other engine it touches. When Optic is active and coupled, the entire XOmnibus patch begins to pulse and breathe in response to its own audio output. Bass energy from Obese drives filter sweeps on Odyssey. Transient spikes from Onset trigger envelope resets on Oblong. The AutoPulse adds a subliminal trance heartbeat that makes static pads feel alive. Optic is the nervous system of the colony — invisible, essential, the reason everything moves together.

---

## Coupling Thesis

Optic gives rhythmic modulation — pulse-driven and spectrum-driven control signals that make other engines breathe and flash. His 8-band analysis feeds back into the audio domain through the coupling matrix: spectral centroid can drive filter cutoff on any engine, bass energy can pump amplitude, transient detection can gate effects. He receives audio from any engine and converts it to rhythm and spectral intelligence. The AutoPulse creates a feedback loop: audio from the gallery feeds Optic's analyzer, the analyzer shapes the AutoPulse, the AutoPulse modulates other engines, and those engines produce new audio for the analyzer. The colony pulses. When coupled with Onset, Optic's transient detection creates drum-reactive modulation that makes every other engine duck, swell, or flash in time. When coupled with Odyssey's long evolving pads, Optic's spectral centroid tracking creates slowly drifting modulation that mirrors the harmonic content of the journey itself. He is the synesthete — hearing light, seeing sound.

---

## Preset Affinity

| Foundation | Atmosphere | Entangled | Prism | Flux | Aether |
|-----------|-----------|-----------|-------|------|--------|
| Medium | Low | High | High | High | Low |

---

*XO_OX Designs | XOptic — the colony pulses, and the ocean remembers the rhythm*
