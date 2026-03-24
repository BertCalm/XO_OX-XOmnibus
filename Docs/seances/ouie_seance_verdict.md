# Synth Seance Verdict: OUIE

**Engine:** OuieEngine ("The Hammerhead")
**Date:** 2026-03-20
**File:** `Source/Engines/Ouie/OuieEngine.h` (2026 lines)
**Score:** 8.5 / 10
**Accent:** Hammerhead Steel `#708090`
**Parameter Prefix:** `ouie_`
**Parameter Count:** 51 declared (14 Voice 1 algo, 14 Voice 2 algo, 7 filter, 8 amp env, 5 mod env, 6 LFO, 2 breath, 2 unison, 3 voice mode, 1 level, 4 macros, 1 voice mix)

---

## Ghost Panel Verdicts

### Robert Moog (1934-2005)
"The duophonic architecture is correct in its fundamentals: two independent signal paths, each with its own filter and envelope, converging at an interaction stage. The CytomicSVF per voice is a sound engineering choice. What concerns me is the ADSR implementation -- linear attack and quasi-exponential decay using a one-pole approach. A true exponential attack curve, or at minimum an RC charging model, would give the envelopes more natural response. The velocity-to-filter path (D001) is properly implemented and musically meaningful." **7.5/10**

### Don Buchla (1937-2016)
"The HAMMER axis is the crown jewel. A bipolar interaction control that sweeps from cross-FM and ring modulation (STRIFE) through neutrality to spectral blending and harmonic convergence (LOVE) -- this is a genuine performance control, not a parameter. The wavefolder algorithm correctly implements iterative reflection folding with soft saturation, and I appreciate the Buchla/Serge attribution in the comments. The 8-algorithm palette gives each voice genuine timbral independence. I would want the HAMMER to modulate more deeply at extremes -- the cross-FM scaling of 0.6 is conservative. Let the performer destroy the signal if they choose." **9/10**

### Dave Smith (1950-2022)
"The voice allocation is thoughtfully designed across three modes: Split, Layer, and Duo. The Duo mode -- where Voice 1 captures the newest note and Voice 2 inherits the previous -- is a classic duophonic behavior that Sequential instruments made famous. The glide implementation uses a proper exponential coefficient derived from sample rate. I note the FM algorithm now has user-controllable ratio (0.5-16.0) and index (0-10) per voice, which resolves the hardcoded FM concern from the previous review. The unison implementation is practical: up to 4 oscillators per voice with detuning, though unison voices 2-4 fall back to simplified waveforms for CPU efficiency. Pragmatic." **8/10**

### John Chowning (1934-)
"The FM algorithm implements classic 2-operator FM with carrier and modulator phases tracked independently. The ratio parameter now ranges from 0.5 to 16.0 with a skew factor, and the modulation index from 0 to 10 -- these are sensible ranges for musical FM. I observe that the modulator output is applied as a phase offset rather than a frequency offset, which is technically phase modulation rather than frequency modulation, but this is the standard practice (as in Yamaha DX-series) and produces equivalent spectra. The coupling input AudioToFM modulating the FM index from external engines is a creative extension of the FM paradigm into the fleet architecture." **8.5/10**

### Ikutaro Kakehashi (1930-2017)
"Fifty-one parameters is a substantial count for a duophonic engine, but the organization is logical: per-voice algorithm selection, per-voice filter, per-voice amp envelope, shared mod envelope, per-voice LFO, and four character macros. The voice mode selector (Split/Layer/Duo) is immediately understandable. The CURRENT macro controlling stereo spread and chorus depth is the kind of single-knob environment control that makes an instrument approachable. The breathing LFO satisfying D005 is properly autonomous. I wish the AMPULLAE macro had a more intuitive name for the general user, but the DSP routing -- velocity sensitivity and resonance boost -- is correct." **8/10**

### Vangelis (1943-2022)
"I play from the instrument, not from the parameter list. The HAMMER axis is the kind of control I would reach for in performance -- mod wheel sweeping from STRIFE to LOVE while holding two notes in Duo mode, the relationship between the voices shifting from metallic destruction to harmonic convergence. The aftertouch routing to HAMMER position (D006) is essential and correctly implemented. The mod wheel routing to algorithm parameter morphing adds a second performance axis. But I want more from the CURRENT macro -- a simple chorus at 0.8 Hz with a 5ms base delay is thin for an environment control. Where is the reverb? Where is the space?" **8.5/10**

### Klaus Schulze (1947-2022)
"The breathing LFO with a rate floor of 0.005 Hz -- a cycle every 200 seconds -- modulating both pitch and filter cutoff is exactly the kind of ultra-slow evolution I require. The LFO rate ranges (0.01-30 Hz for LFO1/LFO2) allow both rhythmic modulation and glacial drift. The 5 LFO shapes including Sample-and-Hold provide sufficient variety. My concern is temporal: with only 2 voices and no internal sequencer or arpeggiator, long-form pieces require external modulation or coupling from other engines. The engine breathes, but it does not yet journey on its own." **7.5/10**

### Isao Tomita (1932-2016)
"The wavetable algorithm with 16 procedurally generated tables -- each with increasing partial counts, metallic inharmonic stretching, and odd-harmonic emphasis in the middle range -- shows care in timbral design. The morphing between tables with linear interpolation is smooth. The Karplus-Strong algorithm with noise-burst excitation, linear-interpolation fractional delay, and brightness-controlled damping is a correct physical string model. I would orchestrate Voice 1 as VA saw with unison detune against Voice 2 as KS plucked string, the HAMMER at slight LOVE for spectral blending -- synthetic strings meeting physical strings. The palette of 8 algorithms makes this engine a small orchestra of methods." **9/10**

---

## Blessings

| ID | Blessing | Description |
|----|----------|-------------|
| B017 | **HAMMER Interaction Axis** | A bipolar performance control (-1 STRIFE to +1 LOVE) that continuously morphs the relationship between two voices through cross-FM, ring modulation, hard sync, spectral blending, and harmonic convergence. No other engine in the fleet has a single control that transforms inter-voice behavior across this range. Mod wheel and aftertouch both route to HAMMER position, making it a true performance axis. |
| B018 | **Interval-as-Parameter** | The duophonic Duo mode inherently makes the musical interval between two notes a timbral parameter. Playing a fifth vs. a tritone produces fundamentally different results through the HAMMER interaction stage. The interval shapes the cross-FM sidebands, the ring mod sum/difference tones, and the harmonic lock quantization. No other engine in the fleet treats the pitch relationship between simultaneously sounding notes as a first-class synthesis parameter. |
| B019 | **8-Algorithm Palette** | Each voice independently selects from 8 synthesis algorithms spanning VA, wavetable, FM, additive, phase distortion, wavefolding, Karplus-Strong, and filtered noise. This gives 64 possible voice-pair combinations before the HAMMER interaction is applied. The smooth/rough division (feliX-side algorithms 0-3, Oscar-side algorithms 4-7) aligns with the XO_OX mythology. |

---

## Concerns

| Priority | Issue | Detail |
|----------|-------|--------|
| P1 | **CURRENT macro is underwhelming** | The CURRENT macro ("Environment") only provides a simple chorus effect with fixed 0.8 Hz rate and ~5ms base delay, plus stereo voice panning. No reverb, no delay feedback, no depth variation. This is the weakest of the 4 macros by a significant margin. Compare to other engines where the environment macro provides chorus + delay + reverb. Recommend adding at least a simple feedback delay and a basic reverb (even a simple allpass diffuser). |
| P1 | **Unison voices 2-4 lose algorithm character** | When unison count > 1, voices 2-4 fall back to raw saw oscillators (VA mode), plain sine (FM/Additive/PhaseDist modes), simplified wavefolder, or silence (KS/Noise). This means the timbral identity of the selected algorithm is diluted by generic waveforms in unison mode. Unison with Karplus-Strong or Filtered Noise produces only the primary voice's output. |
| P1 | **LOVE harmonic lock is not implemented** | The header comments describe LOVE as including "Harmonic lock: quantize Voice 2 pitch to Voice 1 harmonics" and "Soft unison: voices converge in pitch with chorus-like detune," but the actual OuieInteraction::process() only implements spectral blending (weighted average) and merging at deep LOVE values. The harmonic lock and pitch convergence features described in the concept are absent from the DSP. |
| P2 | **Duo mode voice-steal does not retrigger Voice 2 envelope** | In Duo mode, when a new note arrives, Voice 1's state is copied to Voice 2, but `voices[1].ampEnv.noteOn()` is never called on the copy. The envelope parameters are set via `setParams()` but the envelope stage is inherited from the copy. If Voice 1 was in sustain, Voice 2 continues in sustain -- but if Voice 1 was in attack, Voice 2 continues mid-attack at a potentially different rate. Should retrigger or explicitly transition. |
| P2 | **Mod envelope is "shared" but per-voice** | The mod envelope ADSR parameters (A/D/S/R/Depth) are shared (one set of params), but each voice has its own `modEnv` instance that processes independently. The "shared" label in the parameter section is slightly misleading. This is not a bug -- the behavior is correct -- but could confuse preset designers who expect a global mod envelope. |
| P2 | **LFO1/LFO2 shape changes require noteOn** | The LFO shape is set in `noteOn()` via `voice.lfo.setShape()` but not updated during `renderBlock()`. Changing the LFO shape parameter while a note is held has no effect until the next noteOn. Rate and depth are read per block, but shape is latched. |
| P2 | **Chorus buffer not cleared on reset()** | The `chorusBufferL`/`chorusBufferR` arrays and `chorusWritePos`/`chorusPhase` are not cleared in `reset()`. This could produce a brief burst of stale audio when re-enabling CURRENT after a reset. |

---

## Doctrine Compliance

| Doctrine | Status | Evidence |
|----------|--------|----------|
| D001 — Velocity Must Shape Timbre | **PASS** | `voice.velocity * velScale * 4000.0f` added to filter cutoff per voice (line 1260). Velocity also scales amplitude: `ampLevel * voice.velocity * voice.fadeGain` (line 1267). AMPULLAE macro scales `velScale` from 0.3 to 1.0. Velocity shapes both timbre (filter brightness) and amplitude. |
| D002 — Modulation is the Lifeblood | **PASS** | LFO1 (5 shapes, 0.01-30 Hz, per Voice 1) + LFO2 (5 shapes, 0.01-30 Hz, per Voice 2) + Breathing LFO (D005). Mod envelope with depth control. Mod wheel -> algorithm parameter morphing. Aftertouch -> HAMMER interaction. 4 macros: HAMMER, AMPULLAE, CARTILAGE, CURRENT -- all with confirmed DSP paths. Coupling: AmpToFilter, LFOToPitch, AudioToFM, AudioToRing. |
| D003 — The Physics IS the Synthesis | **N/A** | No physically-modeled engine claim. The Karplus-Strong algorithm (algo 6) is a well-established physical string model with correct implementation: noise-burst excitation, linear-interpolation fractional delay, one-pole lowpass in feedback loop with brightness-controlled damping, and 0.998 energy loss per sample. |
| D004 — Dead Parameters Are Broken Promises | **PASS** | All 51 declared parameters are attached via `attachParameters()` and read in `renderBlock()` via `loadParam()`. FM ratio and index are now per-voice user parameters (resolved from previous review). Wavetable position is exposed per voice. No dead parameters found. |
| D005 — An Engine That Cannot Breathe Is a Photograph | **PASS** | Dedicated OuieBreathingLFO with rate range 0.005-2.0 Hz (floor well below 0.01 Hz requirement). Routes to both pitch modulation (`breathMod * 0.005f` frequency scaling) and filter cutoff (`breathMod * 500.0f` Hz offset). LFO1/LFO2 also have 0.01 Hz floor. |
| D006 — Expression Input Is Not Optional | **PASS** | Velocity -> filter cutoff + amplitude (D001 path). CC#1 mod wheel -> algorithm parameter morphing (`modWheelMod`). Channel pressure aftertouch -> HAMMER interaction axis (`aftertouch_ * 0.3f`). MPE pitch bend per voice. Three distinct expression inputs confirmed. |

---

## Final Consensus

**Score: 8.5 / 10**

OUIE is one of the most architecturally ambitious engines in the XOlokun fleet. The core concept -- duophonic synthesis with 8 selectable algorithms per voice, interacting through the bipolar HAMMER axis from STRIFE to LOVE -- is genuinely novel and well-executed. The 64 possible algorithm pairings, each producing different spectral interactions through the HAMMER, give this engine extraordinary timbral range within a focused identity. The FM parameters have been properly exposed since the last review, and all 51 parameters are live. Doctrine compliance is clean across all six checks. The primary weaknesses are the anemic CURRENT macro (chorus-only, no reverb or delay), the unimplemented harmonic lock feature described in the LOVE section of the concept, and the unison fallback to generic waveforms. The HAMMER interaction axis earns Blessing B017 as a genuinely novel performance control, and the emergent property of musical interval as timbral parameter in Duo mode earns B018. With the CURRENT macro expanded and the LOVE harmonic lock implemented, this engine would comfortably reach 9.0+.
