# FOUNDER'S FREQUENCY -- Week 3
## Coral Investigations
**Engine(s)**: OBLONG | **Mood**: Atmosphere | **Coupling**: None

---

### The Story

Daisuke is the Guild's keys player. Not a beat-maker, not a producer in the traditional sense -- a pianist who crossed over into electronic music and brings that real-time expressiveness to everything he touches. He asked for something he called "a patch that explores itself." He wanted to hold a chord and hear it change -- not randomly, not on a cycle, but as if the sound was curious about its own filter space and kept poking around in it.

That request sat with me for weeks. Most modulation is repetitive: an LFO goes up, comes down, goes up, comes down. Even randomized LFOs feel mechanical because they don't have behavior -- they're just noise shaped through a filter. What Daisuke wanted was an LFO with personality.

XOblongBob already had the answer built into its architecture. The CuriosityEngine is OBLONG's behavioral modulation system -- five distinct modes that don't just modulate parameters, they explore them. Coral Investigations uses Mode 2 (Investigate), which creates per-voice modulation that systematically probes the filter frequency space, pauses at resonant peaks, then moves on. It's not random. It's not cyclic. It's genuinely curious.

This is the preset that keyboard players fall in love with first.

### The Science

**The CuriosityEngine -- Five Behavioral LFO Modes**

Standard LFOs produce periodic waveforms -- sine, triangle, square, saw. You can randomize the parameters, but the behavior is still mechanical: oscillate at rate X between bounds Y and Z. The CuriosityEngine replaces this with behavioral algorithms where the LFO's output depends on its own history and the current state of the filter.

The five modes, each modeled on animal behavior:

- **Sniff** (Mode 0): Quick, short probes. The modulation output makes rapid small excursions from the current position, as if sniffing around a point of interest. High-frequency, low-amplitude exploration. Think of a dog investigating a scent -- many small samples of nearby space.

- **Wander** (Mode 1): Smooth, slow, undirected movement. The modulation drifts with no particular target, like a creature swimming aimlessly through open water. This is closest to a traditional random LFO, but with momentum -- the drift has inertia, so it doesn't reverse abruptly.

- **Investigate** (Mode 2): The mode used in this preset. A two-phase behavior: the modulation moves to a new frequency region (the "approach"), dwells there for a variable duration (the "investigation"), then moves to a different region. The dwell time is longer at resonant peaks -- the algorithm literally spends more time where things sound interesting. Technically, it's a random walk with attractor points at filter resonance frequencies.

- **Twitch** (Mode 3): Nervous, jittery modulation. Short, unpredictable jumps in filter space. The animal equivalent of a startled creature -- sharp, angular movements with sudden direction changes. Useful for glitchy, percussive textures.

- **Nap** (Mode 4): Nearly static. The modulation makes extremely slow, imperceptible movements. Over 30 seconds, you might hear the filter cutoff shift by a few hundred Hz. This is what D005 (the breathing doctrine) demanded -- every engine must have modulation slow enough to be felt over geological timescales. Nap mode is the gentlest possible version: the sound breathes, but barely.

In Coral Investigations, the CuriosityEngine is set to Investigate mode with a moderate rate. Each voice gets its own behavioral instance (per-voice modulation), so when you play a chord, each note investigates a different part of the filter space at its own pace. A C major triad doesn't just sustain -- it evolves into three independent timbral journeys that occasionally converge and then diverge again.

**The SnoutFilter -- Cytomic SVF with Character**

OBLONG's filter is the BobSnoutFilter, which wraps a Cytomic State Variable Filter (SVF) -- one of the most respected digital filter topologies in audio DSP. The Cytomic SVF (designed by Andrew Simper, documented in his 2013 papers) solves the problems that plague traditional digital filters: it doesn't blow up at high resonance, it maintains consistent bandwidth across the frequency range, and it can self-oscillate cleanly.

The SnoutFilter adds a "character" parameter that crossfades between the SVF's pure response and a slightly saturated version. At character 0, it's a textbook SVF. At character 1, the feedback path includes a soft tanh saturation that models the nonlinear behavior of analog filter circuits. This nonlinearity is what gives analog filters their "warmth" -- the filter resonance doesn't just ring, it compresses and adds subtle harmonics.

The CuriosityEngine's Investigate mode is modulating this filter's cutoff frequency. Because the SVF tracks smoothly across its range without digital artifacts, the filter movements produced by the behavioral LFO sound organic rather than stepped or glitchy.

**DustTape -- Tape Saturation as Color**

After the filter, the signal passes through BobDustTape -- a simplified tape saturation model. Real magnetic tape has a complex interaction between bias current, oxide particle alignment, and hysteresis (the tendency of magnetized material to retain its state). DustTape models the most audible aspect of this: the soft saturation curve that compresses peaks and rolls off high frequencies.

The implementation is straightforward: a tanh waveshaper (modeling the tape's saturation curve) followed by a one-pole low-pass filter (modeling the tape's natural high-frequency loss). Simple, but the combination of soft saturation + gentle HF rolloff is exactly what "tape warmth" means in engineering terms. It's the same reason vinyl records and cassette tapes have that warm quality -- the medium itself is a gentle compressor and a low-pass filter.

In Coral Investigations, DustTape is set to moderate intensity. It softens the filter movements produced by the CuriosityEngine, preventing any harsh resonant peaks from becoming painful. The tape saturation acts like a safety net under the behavioral modulation -- the sound can explore freely because the tape catches any harshness.

### How to Use It

**1. Ambient Keys (Tempo: any, Key: any, Sustain pedal: ON)**
Hold a chord with the sustain pedal. Let the CuriosityEngine investigate. Over 10-20 seconds, each note in the chord will evolve independently. The pad is never the same twice. Record a 2-minute hold and chop it into sections -- each section has its own character because the behavioral LFO was in a different phase of investigation.

**2. Neo-Soul Rhodes Replacement (BELLY high, moderate velocity)**
Turn M1 (CHARACTER) up to 0.65. The DustTape warmth and SnoutFilter character combine to produce something that sits in the same frequency band as a Rhodes electric piano, but with organic movement that a Rhodes can never achieve. Play 9th chords in the middle register. The CuriosityEngine adds life without being distracting.

**3. Soundtrack Underscoring (PLAY with INVESTIGATE mode, low velocity)**
Play pianissimo in the C3-C5 range. The investigation mode is sensitive to velocity -- softer playing produces slower, more cautious explorations. The sound becomes environmental: not a pad, not a melody, but a presence. Layer under dialogue or narration.

**4. Live Performance -- Mode Switching**
Map M2 (MOVEMENT) to a footswitch or expression pedal. At low values, the CuriosityEngine runs in near-Nap territory (slow, meditative). As you push M2 up, the investigation rate increases -- the sound becomes more active, more searching. In a live set, start a section with gentle investigation and build energy by increasing the rate. The audience hears the sound "wake up."

**5. Textural Sound Design (M3 high, M2 high)**
Push both COUPLING and MOVEMENT macros above 0.6. The filter exploration becomes dramatic -- wide sweeps with long dwells at resonant peaks. Every note becomes a mini composition. Record individual notes and use them as one-shot samples in a drum machine or sampler.

### Macro Guide

| Macro | What It Does | Sweet Spot |
|-------|-------------|-----------|
| M1 (CHARACTER) | SnoutFilter character blend -- crossfades between clean SVF and saturated analog response. Also scales DustTape amount. | 0.45 -- warm analog character without obvious saturation |
| M2 (MOVEMENT) | CuriosityEngine investigation rate. At 0, near-Nap. At 1, rapid, energetic exploration. | 0.30 -- slow enough to feel organic, fast enough to hear movement within a bar |
| M3 (COUPLING) | Filter modulation depth from CuriosityEngine. How far the investigation sweeps. At 0, tiny micro-movements. At 1, full-range filter sweeps. | 0.50 -- wide enough to hear timbral variety, narrow enough to stay musical |
| M4 (SPACE) | Blend of internal reverb/delay. OBLONG's reverb uses short allpass chains tuned for small-room intimacy. | 0.25 -- a touch of room. Above 0.5, the pad gets washy. Coral Investigations is meant to be relatively dry. |

### Pair It With

- **"Coupled Bounce"** (Week 1) -- The rhythmic coupling foundation underneath Coral Investigations' wandering textures creates a complete production. The locked kick-bass below, the exploratory keys above. Two presets, one track.

- **ODYSSEY "Deep Meridian" or "Velvet Drift"** -- Layer an ODYSSEY atmosphere preset an octave above Coral Investigations. The Voyager Drift in ODYSSEY and the CuriosityEngine in OBLONG create two independent modulation systems that never sync up -- this is how you get sounds that evolve forever without repeating.

- **ONSET percussion at low coupling** -- A light ONSET pattern with Amp->Filter coupling into OBLONG at 0.15 amount. The drum hits subtly nudge the CuriosityEngine's investigation trajectory. The exploration isn't random anymore -- it's reacting to the rhythm, like a creature responding to distant sounds.

### The Download

This preset is included in the **Founder's Signature Vol. 1** XPN pack -- 5 presets, free, for XOceanus and MPC.
[Download link placeholder]

---
*Next week: Thermocline Dispatch -- where dub delay meets psychedelic drift.*
*Founder's Frequency is a free weekly series from XO_OX Designs.*
