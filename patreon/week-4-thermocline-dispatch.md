# FOUNDER'S FREQUENCY -- Week 4
## Thermocline Dispatch
**Engine(s)**: OVERDUB + ODYSSEY | **Mood**: Entangled | **Coupling**: Amp->Filter + LFO->Pitch

---

### The Story

Tomoko is the Guild's dub specialist. She grew up on King Tubby, Lee "Scratch" Perry, and Scientist -- the Jamaican engineers who turned the mixing desk into an instrument. She also listens to Boards of Canada, Tame Impala, and Floating Points. Her request was specific: "I want a pad that sounds like it was recorded to tape in a Jamaican studio in 1975 and then sent through a psychedelic time machine."

That request perfectly describes the collision between two XO_OX engines. OVERDUB is our dub engine -- tape delay, spring reverb, and drive, built from the thermocline down. ODYSSEY is our psychedelic pad engine -- Voyager Drift, Climax bloom, the open ocean journey from familiar to alien. Put them together with coupling and you get Thermocline Dispatch: a pad that degrades beautifully over time, drifting and echoing through layers of warm tape coloration, each repeat darker and more distant than the last.

The name comes from our aquatic mythology. The thermocline is where warm surface water meets cold deep water -- the boundary zone where sound bends and refracts. In the XO_OX water column, OVERDUB and ODYSSEY both live near this boundary. This preset is what happens when they meet there.

### The Science

**OVERDUB's Tape Delay -- Hermite Interpolation and Wow Modulation**

The core of OVERDUB's delay is the DubTapeDelay class. Unlike a simple digital delay (which reads from a buffer at a fixed offset), tape delay introduces continuous pitch modulation because the tape speed is never perfectly constant.

We model this with two modulation sources:

- **Wow**: A 0.3 Hz sine oscillator that slowly bends the effective delay time. In real tape machines, "wow" is caused by irregularities in the capstan -- the metal shaft that pulls the tape. At 0.3 Hz, the pitch bends are slow enough to sound musical rather than broken. The modulation depth is set to 0.2% of the delay time -- subtle, but audible as a gentle pitch waver on repeats.

- **Flutter**: A faster modulation (~45 Hz) from smoothed noise, simulating the higher-frequency vibrations of tape transport mechanisms. Flutter adds the characteristic "shimmer" to tape delay repeats. The smoothing prevents it from sounding like digital noise.

The critical technical detail is the **Hermite interpolation** on the delay buffer read. When the wow and flutter modulate the delay time, the read position lands between samples. Linear interpolation (connecting adjacent samples with a straight line) introduces high-frequency artifacts. Hermite interpolation uses four neighboring samples to construct a smooth cubic curve, which is mathematically closer to what an analog tape would produce. The difference is subtle on a single repeat but cumulative -- after 6 repeats, linear interpolation sounds harsh, while Hermite stays warm.

**Feedback Filtering -- Each Repeat Darkens**

In the feedback path (where the delay output feeds back into the input), OVERDUB applies a bandpass filter. This is the "each repeat darkens" behavior that defines dub delay. The bandpass removes both high frequencies (the brightness fades, like sound traveling through water) and low frequencies (the sub energy dissipates, preventing the feedback from building into a destructive rumble).

The bandpass center frequency is set around 800 Hz with moderate Q. Each time the signal passes through the feedback loop, the bandpass removes more high and low content. After 3-4 repeats, only the midrange survives -- a ghostly, telephone-like version of the original note. This is exactly what King Tubby's Roland Space Echo did, and it's why dub delay sounds like memories fading.

**OVERDUB's Spring Reverb -- 6-Stage Allpass with Coprime Delays**

The DubSpringReverb uses a 6-stage allpass chain. An allpass filter passes all frequencies at equal amplitude but shifts their phase -- the perceived effect is time smearing without frequency coloring. Chain six of them together and you get a dense, diffuse reverb.

The key design choice is **coprime delay lengths**. Each of the 6 allpass stages has a different buffer length, and those lengths are chosen to be coprime (their greatest common divisor is 1). Why coprime? Because when delay lines share common factors, their echoes align periodically, creating audible metallic ringing. Coprime lengths ensure the echoes never align, producing a diffuse, organic reverb tail.

The left and right channels use slightly offset delay lengths for true stereo decorrelation. The result isn't just stereo -- it's dimensional. Sounds placed through this reverb have depth and width without the phasey artifacts of simple stereo wideners.

**ODYSSEY's Voyager Drift -- Per-Voice Random Walk**

The second engine in this preset is ODYSSEY, contributing its signature Voyager Drift. This is a per-voice seeded random walk: each voice gets its own pseudo-random number generator (xorshift32, seeded uniquely per voice), producing a smooth random modulation signal.

DriftVoyagerDrift works by generating random target values and smoothly interpolating toward them at a rate controlled by the drift rate parameter. The smoothing coefficient uses an exponential moving average (`1 - e^(-2*pi*rate/sr)`), which produces natural-feeling movement -- fast at first, then settling gradually as it approaches the target. When a new target is reached, a fresh random value is generated and the process repeats.

Because each voice has its own seed, a held chord produces multiple independent drift signals. In Thermocline Dispatch, these drift signals modulate both pitch (creating micro-detuning between voices) and filter cutoff (creating timbral variation). The psychedelic quality comes from this: each voice is on its own journey, and the chord as a whole shifts and breathes in ways that never repeat.

**The Coupling -- Amp->Filter + LFO->Pitch**

Two coupling routes bind OVERDUB and ODYSSEY:

1. **Amp->Filter** (OVERDUB -> ODYSSEY, amount: 0.30): OVERDUB's amplitude envelope modulates ODYSSEY's filter cutoff. When the tape delay produces a repeat, that repeat's energy opens ODYSSEY's filter slightly. The pad literally responds to the echoes -- each repeat brightens the pad for a moment, then the pad darkens again as the repeat fades. The pad breathes with the delay.

2. **LFO->Pitch** (ODYSSEY -> OVERDUB, amount: 0.12): ODYSSEY's internal LFO (running at a slow 0.15 Hz rate) modulates OVERDUB's delay time in tiny amounts. This adds an additional slow pitch wobble to the tape delay that's synchronized to ODYSSEY's internal modulation. The delay doesn't just wow on its own -- it sways with the pad's breathing.

The result is a feedback loop of influence: the delay shapes the pad, and the pad shapes the delay. Neither engine dominates. They converse.

### How to Use It

**1. Dub Pad Foundation (Key: Cm, Tempo: 70-80 BPM)**
Play a Cm7 chord and hold. Let the first 8 bars play out untouched -- just listen. You'll hear the pad establish itself (ODYSSEY's drift), the first delay repeats appear (OVERDUB's tape), and the coupling start to breathe. The sweet spot is after about 4 bars when the two engines have settled into their conversation. Record the full take and pick your favorite section.

**2. Ambient Dub Score (Key: any minor, Tempo: 60 BPM, Expression pedal: on)**
Map M4 (SPACE) to your expression pedal. Play sparse, sustained chords in a minor key. Push the pedal forward to increase the spring reverb -- the pad dissolves into space. Pull it back to dry the sound. The coupling means the reverb tail interacts with the filter opening, creating cascading timbral shifts as the space increases and decreases.

**3. Tame Impala-Style Shimmer (M2 at 0.55, pitch bend up 2 semitones)**
Increase MOVEMENT to add more Voyager Drift to both engines. Then hold a chord and slowly push the pitch bend wheel up. The Hermite-interpolated delay tracks the pitch change smoothly (no digital stepping), and the drift adds organic detuning around the bent pitch. This is the psychedelic shimmer that Kevin Parker builds with tape machines and studio magic -- but in a single preset.

**4. Cinematic Tension (Single notes, low register, M3 COUPLING at 0.7)**
Push the coupling intensity high and play single sustained notes below middle C. Each delay repeat now significantly affects the pad's filter, creating dramatic swells and retreats. The single notes prevent the pad from becoming muddy, and the high coupling creates an emotional intensity -- each echo changes everything. Use for film score tension scenes.

**5. Sound Design -- Freeze and Drift**
Play a chord and then immediately stop. Listen to the decay. The OVERDUB delay produces repeats that fade over 10-15 seconds, and ODYSSEY's drift continues modulating even as the volume decreases. The last 5 seconds of decay are pure sound design gold -- ghostly, drifting, distant. Sample these tails.

### Macro Guide

| Macro | What It Does | Sweet Spot |
|-------|-------------|-----------|
| M1 (CHARACTER) | OVERDUB tape saturation + drive amount. At 0, clean digital delay. At 1, heavily saturated tape with warm distortion on every repeat. | 0.40 -- warm tape character. Above 0.65, the repeats start crunching. |
| M2 (MOVEMENT) | Voyager Drift depth across both engines. Controls the amount of per-voice random walk on pitch and filter. | 0.35 -- gentle psychedelic drift. Above 0.55, things get noticeably wobbly (good for Tame Impala vibes). |
| M3 (COUPLING) | Master coupling intensity for both Amp->Filter and LFO->Pitch routes. | 0.35 (default) -- subtle conversation. At 0.6+, the engines become dramatically interdependent. |
| M4 (SPACE) | Spring reverb wet/dry blend. Scales OVERDUB's 6-stage allpass reverb. | 0.30 -- medium room. Below 0.1 is intimate. Above 0.6, the pad dissolves into pure space. |

### Pair It With

- **"Belly to Bite"** (Week 2) -- An OVERBITE bass line underneath Thermocline Dispatch creates a dub production skeleton. The bass is rooted and assertive (Belly), the pad is floating and ephemeral (Thermocline). Classic dub architecture: heavy low end, spacious midrange.

- **ONSET "Nudge and Growl"** (from the coupling preset library) -- A lightly coupled ONSET pattern at whisper intensity. The drum hits create subtle ripples in Thermocline Dispatch's coupling -- each kick slightly brightens the pad for a moment. Dub percussion that speaks to the pad rather than sitting on top of it.

- **OBLONG solo pads** -- Double the atmosphere by layering an OBLONG pad with DustTape engaged. Two tape-character engines creating a dense, warm mid-frequency bed. Pan Thermocline Dispatch slightly left, OBLONG slightly right, and the stereo field fills with living texture.

### The Download

This preset is included in the **Founder's Signature Vol. 1** XPN pack -- 5 presets, free, for XOceanus and MPC.
[Download link placeholder]

---
*Next week: Tidal Dialogue -- the most emotional preset in the collection. Drums that make pads breathe.*
*Founder's Frequency is a free weekly series from XO_OX Designs.*
