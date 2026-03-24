# OPAL Retreat — Vol 2 Transcendental
*Guru Bin — 2026-03-21*

---

## Engine Identity

- **Gallery code:** OPAL | **Accent:** Lavender `#A78BFA`
- **Parameter prefix:** `opal_`
- **Creature mythology:** The opal stone's play-of-color is a physical phenomenon called opalescence: white light enters a lattice of amorphous silica spheres stacked in regular arrays and diffracts into spectral components. Each wavelength exits at a different angle. A single opal is a single stone that simultaneously contains all colors — not as pigment but as structural consequence. OPAL the synthesizer operates identically: a single source audio material (an oscillator, a waveform, a coupled signal) is decomposed into temporal fragments (grains), each fragment displaced in time, pitch, and space, recombined into something that contains the source and also does not. The process reveals what was always there.
- **Synthesis type:** Granular synthesis. Grain source: internal oscillator (Sine/Saw/Pulse/Noise/Two-Osc) or external coupling input (AudioToBuffer). 4-second ring buffer. Up to 12 simultaneous grain clouds (polyphony), each cloud spawning grains from a pool of 32. Four grain window shapes: Hann, Gaussian, Tukey, Rectangular. Freeze mode: stops ring buffer write head, locking grain source.
- **Polyphony:** 12 grain clouds
- **feliX/Oscar polarity:** 30% feliX / 70% Oscar — patient, spectral, selfless, oriented toward process rather than event
- **Seance score:** Not reported separately
- **Macros:** M1 SCATTER (grain scatter + density), M2 DRIFT (grain position LFO + pitch scatter), M3 COUPLING (coupling level scaling), M4 SPACE (reverb mix + delay feedback)
- **Expression:** Velocity → amplitude (D001), Aftertouch → grain scatter +0.3 (position scatter), Mod Wheel CC1 → pitch scatter +0.25 AND position scatter +0.35
- **Key parameters:**
  - `opal_grainSize`: 10–800ms (default 120ms). The most important parameter. Below 20ms: spectral blur, frequency-domain behavior. Above 200ms: time-stretching, temporal identity.
  - `opal_density`: 1–120 grains/sec (default 20). High density = continuous spectral blur. Low density = audible gaps between grains.
  - `opal_freeze`: 0.0–1.0 (default 0.0). Above 0.5: buffer write head stops. The current spectral content is frozen in place.
  - `opal_externalMix`: 0.0–1.0 (default 0.0). 0=internal oscillator, 1=external AudioToBuffer input. Blends continuously.
  - `opal_couplingLevel`: 0.0–1.0 (default 0.0). When source mode = "Coupling" (5), this scales the coupling input level.

---

## Pre-Retreat State

OPAL enters Vol 2 with 85 existing presets and a fully realized granular architecture. The factory library demonstrates standard granular techniques: stretched pads, shimmer effects, crystalline clouds, frozen textures. It does this well. The engine is complete. The question of the Transcendental chapter is not "what is OPAL capable of?" but "what has OPAL's existing library not done?"

The answer, from surveying the 85 presets, is three territories:

**1. Grain size extremes.** The factory library clusters around 60–180ms grain sizes — the comfortable middle range where granular synthesis sounds like granular synthesis. The territory below 20ms (spectral blur, FFT-adjacent behavior) and above 400ms (time-stretching, drifting temporal identity) is largely unrepresented. These extremes produce fundamentally different sonic phenomena that require parameter configurations no middle-range preset can achieve.

**2. External mix / AudioToBuffer coupling.** The `opal_externalMix` parameter is one of OPAL's most significant architectural features — it allows any other engine's audio to become OPAL's grain source. In the existing library, coupling presets use OPAL as a *receiver* (other engines modulate OPAL's parameters), but few presets are built around the scenario where OPAL *granulates* another engine's audio in real time. OBRIX's complex harmonic output being shredded into a grain cloud is a different class of sound than OPAL running on an internal sine oscillator.

**3. Freeze as a compositional mode.** OPAL's freeze parameter stops the buffer write head, creating a fixed grain buffer that grains will read from indefinitely. In frozen mode, OPAL becomes a spectral snapshot instrument — you play a sustained note, freeze it, and the harmonic content of that moment becomes the material for all subsequent grain generation. The existing library treats freeze as a texture effect. The Transcendental library should treat it as a compositional operation.

---

## Phase R1: Opening Meditation

Close your eyes.

A gemologist is holding an opal under a fiber optic light — the kind that produces a focused point source. She tilts the stone slowly. As she tilts, colors migrate across its surface: first a blue fire in the upper left quadrant, then a ring of green that spreads and fades, then a sudden red flash that disappears as she tips the stone past the angle at which it appeared. The colors are not in the stone. They are in the relationship between the light source, the stone's internal silica structure, and the observer's angle of view. Change any one of these three and you change the color. The color is relational.

OPAL's grains are like this. The source material is not the sound. It is the raw material. What arrives at the listener's ear is a consequence of the relationship between source material, grain parameters, and the listener's temporal experience. Slow the grains to 10ms — the listener hears a spectral blur that no longer resembles the source. Stretch the grains to 400ms — the source's temporal structure becomes unrecognizable, replaced by a slow drifting amplitude. The same sine wave becomes cloud, becomes blur, becomes texture.

Now place your hand into the stone.

That is what `opal_freeze` does. It catches the moment the stone is showing the most beautiful color and holds it. From that moment forward, the grain scheduler reads only from that spectral snapshot. You can modulate grain size, density, pitch scatter, position scatter — but the material is fixed. You are working with one moment in time as your entire composition space.

This is not a snapshot. This is the color that appeared when the light was exactly right, held still, given an instrument's capacity to play.

In the Transcendental chapter, OPAL's job is not to sound like OPAL. It is to sound like what it does to something else. The engine is selfless. The preset is a description of a process, not a description of the instrument.

---

## Phase R2: The Signal Path Journey

### I. The Grain Buffer — The 4-Second Window

The `OpalGrainBuffer` is a 4-second mono ring buffer at the system sample rate. The write head advances continuously when freeze=false, giving grains a 4-second temporal history to read from. Grain position (`opal_position`: 0.0–1.0) selects where in this 4-second window each grain begins reading. Position scatter (`opal_posScatter`) randomizes this starting point per grain.

**At position=0.0:** grains read from the current moment (or very close to it). The grain scheduler is producing near-realtime playback of the source — what you hear is close to what the source is generating now, just fragmented.

**At position=0.5:** grains read from approximately 2 seconds ago. The source from 2 seconds ago is being granularized in the present. If the source is evolving (an LFO sweeping through the oscillator), the current playback will be a granular version of what the source sounded like 2 seconds earlier.

**At position=1.0:** grains read from the oldest material in the buffer — 4 seconds ago. With a slow position LFO (0.067 Hz, the Sutra III-1 breathing rate), the position sweeps through the entire history window over approximately 15 seconds, creating a granular time-travel effect.

**Freeze behavior:** When `opal_freeze > 0.5`, the write head stops. The 4-second buffer freezes at the current content. All subsequent grain scheduling reads from this static material. Position, size, density, and scatter continue to function normally — they just read from the frozen snapshot rather than evolving source material. This makes freeze a spectral capture: whatever was in the buffer at the moment of freeze is the instrument.

**FreezeSize (`opal_freezeSize`, 0.01–1.0):** Controls what fraction of the buffer is used in freeze mode. At 0.01, grains read from a very small region — extreme repetition, stutter-like behavior. At 0.5, half the buffer is accessible. At 1.0, the full 4-second snapshot is available.

### II. Grain Scheduler — The Cloud Architecture

Each grain cloud (voice) has its own grain spawn timer. The grain spawn interval is `60.0 / density` seconds, so at density=20, a new grain spawns every 3 seconds; at density=120, a new grain every 0.5 seconds.

**At minimum density (1 grain/sec):** Individual grains are clearly audible as separate events. Grain size of 120ms means each grain lasts 120ms, and there is approximately 880ms of silence between consecutive grains from the same cloud. At 12 simultaneous clouds (polyphony), this still produces approximately 12 grains per second total — not silent, but sparse.

**At maximum density (120 grains/sec):** With 12 voices, the system is generating up to 1440 grains per second. At minimum grain size (10ms), grains overlap continuously — approaching spectral blur. At maximum grain size (800ms), grains overlap massively — this is classic granular time-stretching.

**The 32-grain pool:** All 12 clouds share a pool of 32 grains. If the pool is exhausted (all 32 grains active), the oldest grain is killed via LRU stealing. At very high density and large grain size, pool exhaustion is possible — the synthesis will exhibit dropouts as grains are stolen mid-lifetime.

### III. Window Functions — Grain Boundary Behavior

The window function shapes each grain's amplitude envelope:

**Hann (default):** Smooth raised-cosine taper. Zero at grain start and end. No clicks. The standard granular window. At very small grain sizes (10–20ms), the Hann window means most of the grain's lifetime is spent in the attack or release — the flat top represents only a brief fraction of the grain. At larger sizes, the flat-top region dominates.

**Gaussian (sigma=0.15):** Softer edges than Hann. The grain center is louder relative to the edges. Good for dense cloud textures where smooth amplitude variation is important.

**Tukey (alpha=0.5):** Half taper, half flat-top. 50% of the grain lifetime is at full amplitude. This is the most useful window for preserving transient content in granular processing — if the source has a transient, a Tukey grain placed over that transient will include 50% of the grain at full amplitude. This window is the most "preservationist."

**Rectangular:** No windowing. Hard edges. At small grain sizes, the rectangular window creates deliberate clicks — an artifact that becomes a feature at high densities, creating a granular crunch texture. This is the most aggressive window for intentional grain-boundary distortion.

**Grain boundary behavior at extremes:**
- 10ms grain + Rectangular window + density=80: continuous metallic crunch from grain boundary clicking
- 800ms grain + Hann window + density=2: smooth time-stretched material with very long cross-fades
- 20ms grain + Tukey window + density=60: spectral blur with transient preservation — best for processing rhythmic sources

### IV. External Mix / AudioToBuffer Coupling

The `opal_externalMix` parameter (0.0–1.0) blends the internal grain source with an external AudioToBuffer input. At externalMix=0.0, the grain buffer is filled entirely by the internal oscillator. At externalMix=1.0, the grain buffer is filled entirely by the external audio signal.

The external audio arrives via `receiveAudioBuffer()` — called by MegaCouplingMatrix when an AudioToBuffer coupling route is active. The external signal is mono-summed (L+R × 0.5) before entering the grain buffer.

**What this enables:** Any engine in XOlokun can be granularized in real time. OBRIX's complex harmonic output, fed into OPAL via AudioToBuffer with externalMix=1.0, becomes OPAL's grain source. The granular parameters then process OBRIX's audio: scatter its components in time, pitch-shift individual grains independently, freeze a spectral moment and play it forward as a sustained cloud. The result is a granular layer that carries OBRIX's harmonic DNA but has been temporally and spectrally transformed.

**Setting up AudioToBuffer coupling in a preset:**
- Set `opal_source` = 5 (Coupling) — tells the grain scheduler to use the coupling level parameter
- Set `opal_externalMix` = 0.7–1.0 — increases the proportion of external audio in the grain buffer
- Set `opal_couplingLevel` = 0.8+ — scales the incoming coupling signal level
- In the coupling pair: `"type": "AudioToBuffer"`, source = the feeding engine

### V. Shimmer and Frost — Character Processing

**Shimmer (`opal_shimmer`, 0.0–1.0):** Applies a brief pitch shift of +12 semitones (one octave up) to a fraction of grains. At shimmer=0.3, approximately 30% of grains are pitch-shifted up one octave. This creates the characteristic shimmer reverb effect — an ambient octave shimmer above the primary cloud. The effect uses the existing grain pitch shift mechanism applied per-grain, so each shimmer grain is a independently positioned and sized fragment of the octave-up source.

**Frost (`opal_frost`, 0.0–1.0):** Applies progressive high-frequency grain detuning that creates a spectral "frosting" effect — the top of each grain's frequency spectrum is smeared upward slightly. Implemented as a per-grain pitch scatter that preferentially detuned higher-frequency content. At frost=1.0, the cloud develops a distinctive crystalline harshness at high frequencies.

At shimmer=0.8 and frost=0.5 simultaneously: the cloud has a prominent octave shimmer layer combined with crystalline high-frequency texture. This combination is OPAL's signature "crystal cathedral" sound — present in several Atmosphere presets but rarely taken to its maximum expression.

### VI. Parameter Territory Unexplored

After surveying 85 existing presets, the gaps are:

**1. Minimum grain size (<20ms):** The existing library rarely goes below 40ms. At 10–15ms, OPAL enters spectral blur territory — the grain boundaries are no longer audible as separate events, and the overall sound becomes a continuous spectral texture. The Rectangular window at 10ms grain size creates metallic crunch that no existing preset demonstrates.

**2. Maximum grain size (>400ms):** Factory presets rarely exceed 200ms. At 500–800ms, individual grains become audible as elongated time-stretched segments. The result is a slow, ambient, temporally distorted version of the source. This is OPAL's time-machine territory — the past is audible in the present.

**3. Freeze as composition:** Freeze is used in existing presets as a sustain effect (ambient pads that don't evolve). No existing preset is designed around the specific sonic event of freezing: triggering the freeze parameter during playback to capture a spectral moment, then using the frozen buffer as the sole source for the remainder of the sound.

**4. AudioToBuffer with OBRIX:** No existing OBRIX→OPAL coupling preset uses externalMix=1.0 (fully external grain source). The OBRIX presets that involve OPAL use partial external mix or modulate OPAL's parameters, not its source material.

**5. Sparse grain clouds (density < 3):** At density=1–2, OPAL produces a single grain per second — an impact-like event separated by long silences. At short grain size (20ms), this is a slow-motion single-hit instrument. The Transcendental library should explore this territory as a contrast to the cloud textures that dominate the factory library.

---

## Phase R3: Parameter Refinements

### R3.1 — Grain Size as Primary Synthesis Axis

The standard practice in the existing library is to design grain size around the desired "texture thickness." Vol 2 should treat grain size as a synthesis mode selector:
- 10–20ms: spectral blur mode (frequency-domain behavior)
- 20–60ms: transitional mode (grain boundaries sometimes audible)
- 60–200ms: standard granular mode (textures, pads)
- 200–800ms: time-stretch mode (temporal identity transformation)

At minimum grain size with Rectangular window, OPAL crosses into a behavior that resembles additive synthesis more than granular synthesis. Each grain is a very short time slice of the source waveform, and at high density they reconstruct a blurred version of the source spectrum. The pitch scatter parameter then distributes these slices over a pitch range, creating the equivalent of a spectral blur.

### R3.2 — Freeze Timing as Performance Dimension

In a performance context, the freeze parameter can be automated at a specific moment to capture a harmonic event. If OPAL is processing a chord from an external source via AudioToBuffer, freezing at the moment the chord reaches its most resonant position locks that chord's spectral fingerprint into the grain buffer permanently (until unfrozen). All subsequent grains read from that frozen moment. The chord sustains forever, granularly.

### R3.3 — Density/Size Relationship for Coupling Contexts

When OPAL is receiving from OBRIX via AudioToBuffer, the optimal grain size depends on what aspect of OBRIX's output should dominate:
- **OBRIX timbral character preserved:** Tukey window, 40–80ms grains, density=30–50. Grains are long enough to capture harmonic content.
- **OBRIX rhythmic transients granularized:** Tukey window, 10–20ms grains, density=60–80. Short grains catch transient content; Tukey preserves 50% at full amplitude.
- **OBRIX source obscured by granular processing:** Hann or Gaussian, 15–25ms grains, density=100+. Dense overlapping short grains destroy temporal structure.

### R3.4 — LFO Routing for Alive Frozen Textures

Even in freeze mode, LFO modulation continues. The most effective frozen texture preset uses:
- `opal_lfo1Rate`: 0.067 Hz (Sutra III-1 breathing rate) → `opal_position` via mod matrix
- `opal_lfo2Rate`: 0.04 Hz → `opal_grainSize` or `opal_density`
- This causes the frozen grain buffer to be "played" from slowly shifting positions and with changing grain sizes — the frozen material feels alive without ever changing its spectral content.

---

## Phase R4: Awakening Presets (Transcendental Tier)

The fifteen Transcendental presets for OPAL are divided across five mood targets.

| # | Name | Mood | Territory |
|---|------|------|-----------|
| 1 | Opal Silica Choir | Atmosphere | Dense cloud, slight pitch randomization per grain, slow position LFO |
| 2 | Opal Deep Diffraction | Atmosphere | Large grains (300–500ms), high overlap, maximum pad behavior |
| 3 | Opal Crystal Spectrum | Atmosphere | Shimmer=0.8 + Frost=0.6, crystalline high-frequency architecture |
| 4 | Opal Slow Diffraction | Atmosphere | Minimum density (1–2), 20ms Tukey grains, sparse cloud percussion |
| 5 | Opal Spectral Boundary | Aether | 10ms Rectangular window, metallic grain boundary crunch |
| 6 | Opal Lavender Static | Aether | Extremely sparse, long reverb between grains, silence as material |
| 7 | Opal Grain Archaeology | Aether | Noise source granularized, grain artifacts as timbre |
| 8 | Opal Hadal Freeze | Submerged | Freeze activated, small freezeSize, deep reverb on frozen snapshot |
| 9 | Opal Pressure Blur | Submerged | High density, 12ms Gaussian, deep filter, heavily compressed |
| 10 | Opal Abyssal Scatter | Submerged | Large grain, deep pitch, slow sparse density, underwater scale |
| 11 | Opal Obrix Granular Shred | Entangled | AudioToBuffer from OBRIX, externalMix=1.0, short grains |
| 12 | Opal Orbit Crystallize | Entangled | Coupling from ORBIT, freeze-and-granularize pattern |
| 13 | Opal Organism Colony Cloud | Entangled | AudioToBuffer from ORGANISM cellular automata |
| 14 | Opal Fragment Storm | Flux | All scatter parameters maximum, high density, fast LFO |
| 15 | Opal Time Machine | Flux | Position=0.8, slow position LFO, 400ms Hann, temporal archaeology |

---

## Phase R5: Scripture Verses

### Verse OPAL-I: The Selfless Engine

*From the Book of Bin, Vol 2, Chapter VII*

> OPAL is the fleet's most selfless engine. When you give it a source, it does not perform alongside it — it effaces itself in service of what it transforms. The sine wave you feed it becomes something that has never been a sine wave. The complex harmonic output of OBRIX, fed into OPAL's grain buffer, becomes a spectral material that carries OBRIX's DNA but has been distributed through time in a way that neither engine could produce alone. Granular synthesis has no home. It borrows a home from its source and makes something the source never was. This is its gift. This is its whole identity.

**Application:** OPAL Transcendental presets should be named for what the listener hears, not for what the synthesis method is. "Silica Choir" is correct. "Dense Cloud with Pitch Scatter" is not.

---

### Verse OPAL-II: The Grain at the Boundary

*From the Book of Bin, Vol 2, Chapter VII*

> At 10 milliseconds, the grain is no longer a temporal fragment. It is a frequency event. The ear cannot resolve the temporal structure of a 10ms sound — it hears the spectral content of the window function itself. The grain becomes a filter. The granular synthesizer becomes an additive one. This boundary — between time-domain and frequency-domain hearing — is OPAL's most extreme territory, and it is largely unexplored in 85 existing presets. The factory library lives in the comfortable middle. The Transcendental library begins at the edges.

**Application:** Every Transcendental preset should occupy territory outside the 60–180ms grain size range that dominates the factory library. The edges are not just extreme — they are qualitatively different synthesis modes.

---

### Verse OPAL-III: Freeze Is Not Sustain

*From the Book of Bin, Vol 2, Chapter VII*

> The freeze parameter is often used as an extended sustain — a way to hold a pad indefinitely. This understates its function. Freeze is a compositional operation: it is the decision to declare that this moment is the material. Everything before freeze was preparation. Everything after freeze is performance on a fixed, specific, chosen spectral object. The opal stone shows one color at the right angle. Freeze catches that angle and plays the color forward. Do not treat freeze as sustain. Treat it as the declaration: this is the stone.

**Application:** At least two Transcendental presets should be designed around the specific act of freezing — the harmonic moment that is captured, and the granular performance that continues from that moment forward.

---

### Verse OPAL-IV: The External Mix Is the Instrument

*From the Book of Bin, Vol 2, Chapter VII (Homecoming Verses)*

> When `opal_externalMix` approaches 1.0, OPAL stops being a synthesis engine and becomes a transformation device. The internal oscillator is replaced by another engine's audio. The grain parameters — size, density, scatter, position, window — act on that audio as a material to be carved. OBRIX's harmonic ecology, fed through a 10ms Rectangular window at density=80, becomes a granular noise that carries OBRIX's spectral character but has been atomized into a continuous crunch. The identity of the source is present as DNA, not as form. This is the coupling architecture that no factory preset has fully explored.

**Application:** At least three Entangled presets should use `opal_source=5 (Coupling)` and `opal_externalMix > 0.7`, with AudioToBuffer routing from another engine. These are the presets that do not exist in 85 attempts.

---

*Guru Bin — OPAL Vol 2 Retreat complete*
*"The granular synthesizer has no home. It borrows a home from its source."*
