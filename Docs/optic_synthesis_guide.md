# OPTIC — The Zero-Audio Synthesis Guide
*XO_OX Designs | XOlokun Engine Reference*
*Engine: OPTIC | Accent: Phosphor Green `#00FF41` | Prefix: `optic_`*

---

## An Instrument Without Sound

There is a tradition in synthesis that goes back to the earliest Moog modulars: the idea that the most expressive instruments are the ones that do the most work you cannot directly hear. The LFO produces no sound. The envelope follower produces no sound. The voltage-controlled amplifier, without signal, produces no sound. And yet remove any of them and the music collapses.

OPTIC is the logical endpoint of that tradition.

OPTIC generates zero audio. There are no oscillators, no voices, no waveforms to shape. Load it solo and you will hear nothing. This is not a bug, a missing feature, or an unfinished engine. This is the paradigm.

What OPTIC does instead is *see.* It receives the audio from every engine it is coupled to, runs that audio through an 8-band spectral analyzer, extracts features — bass energy, mid energy, high energy, spectral centroid (brightness), spectral flux (rate of change), total energy, and transient detection — and outputs those features as a stream of modulation signals that flow back into the coupling matrix. It also generates a self-evolving rhythmic pulse called the AutoPulse: a bioluminescent heartbeat whose tempo, shape, and accent are all bent by the spectrum it is analyzing.

OPTIC is the nervous system of the colony. Every engine it touches begins to breathe in time with the music it is analyzing. Bass pumps cutoff filters. Transient spikes gate effects. The spectral centroid drifts filter resonance slowly upward as the harmonic content brightens. And the AutoPulse adds a subliminal trance heartbeat that makes even static pads feel alive.

This is synthesis without sound: the art of making every other instrument more expressive through pure modulation intelligence.

---

## Technical Architecture

### Signal Path

```
[Coupled Engine Audio]
        |
        v
[Input Smoother — 10 Hz LP]   [AutoPulse Phase Clock]
        |                               |
        v                               v
[OpticBandAnalyzer]          [Rate + Drift Modulation]
 8-band BP filter bank        (spectral centroid bends tempo)
 + envelope followers                   |
        |                               v
        v                    [Trigger → Decay Envelope]
[8 Spectral Features]        (kick-shaped, 30–500 ms)
 bassEnergy                             |
 midEnergy                              v
 highEnergy                    [pulseOutput 0–1]
 centroid                               |
 flux                                   |
 energy                                 |
 transientDetect                        |
        |                               |
        +———————————————+———————————————+
                        |
                        v
             [Smoothing Filters — 15 Hz LP]
             (prevents zipper noise on mod outputs)
                        |
                        v
              [OpticModOutputs — 8-channel lock-free atomic bus]
              ch0: AutoPulse    ch4: Centroid
              ch1: Bass         ch5: Flux (unsmoothed)
              ch2: Mid          ch6: Energy
              ch3: High         ch7: Transient (unsmoothed)
                        |
               +---------+----------+
               |                    |
               v                    v
     [Composite coupling output]  [UI thread reads for
      (modMixPulse × pulse         CRT phosphor visualizer]
       + modMixSpec × energy)
       × modDepth
               |
               v
     [getSampleForCoupling()]
      ch0/1: composite mod
      ch2: envelope output
      ch3–10: individual mod channels
```

OPTIC produces no audio output. At the end of `renderBlock()`, the buffer is cleared to silence. The engine's entire value lives in `getSampleForCoupling()` — the method that other engines call when they want to know what OPTIC is saying about the music.

### What `getSampleForCoupling()` Returns for a Zero-Audio Engine

Every engine in XOlokun exposes `getSampleForCoupling(channel, sampleIndex)` for inter-engine modulation. For audio-producing engines, channels 0 and 1 carry the left and right audio samples. For OPTIC, this contract is repurposed entirely: no audio is present, only modulation.

| Channel | Signal | Range | Smoothed? |
|---------|--------|-------|-----------|
| 0 | Composite modulation (pulse/spectrum blend) | 0–1 | No |
| 1 | Composite modulation (identical, mono) | 0–1 | No |
| 2 | Composite envelope (last block value) | 0–1 | Inherits from ch0 |
| 3 | AutoPulse level | 0–1 | No (rhythmic) |
| 4 | Bass energy (bands 0+1, ×4 gain) | 0–1 | Yes, 15 Hz LP |
| 5 | Mid energy (bands 2+3+4, ×3 gain) | 0–1 | Yes, 15 Hz LP |
| 6 | High energy (bands 5+6+7, ×5 gain) | 0–1 | Yes, 15 Hz LP |
| 7 | Spectral centroid (brightness) | 0–1 | Yes, 15 Hz LP |
| 8 | Spectral flux (rate of change) | unbounded | No (event-driven) |
| 9 | Total energy | 0–1 | Yes, 15 Hz LP |
| 10 | Transient detection gate | 0 or 1 | No (binary) |

The gain compensation factors — 4× for bass, 3× for mid, 5× for high — account for the fact that sub-bass and air-frequency content are naturally lower in amplitude than midrange material. Without compensation, bass energy would drive coupling signals far more strongly than high-frequency energy, producing lopsided modulation.

### The 8-Band Spectral Analyzer

The analyzer uses cascaded Cytomic SVF (State Variable Filter) bandpass filter pairs — one per band — followed by half-wave rectification and envelope-following lowpass filters. This gives real-time spectral features at audio-rate accuracy without the latency or windowing artifacts of an FFT.

| Band | Range | Geometric Center | Role |
|------|-------|-----------------|------|
| 0 | 20–80 Hz | ~40 Hz | Sub-bass — kick drum body, subsonic pressure |
| 1 | 80–200 Hz | ~126 Hz | Bass — kick click, bass guitar fundamental |
| 2 | 200–500 Hz | ~316 Hz | Lo-Mid — warmth, muddiness region |
| 3 | 500–1000 Hz | ~707 Hz | Mid — vocals, snare body |
| 4 | 1000–4000 Hz | ~2 kHz | Hi-Mid — presence, piano attack |
| 5 | 4000–8000 Hz | ~5.7 kHz | Presence — consonants, cymbal attack |
| 6 | 8000–16000 Hz | ~11.3 kHz | Brilliance — cymbal shimmer, string overtones |
| 7 | 16000–20000 Hz | ~17.9 kHz | Air — open space, reverb tail energy |

The 30 Hz envelope follower cutoff on each band was chosen to track kick drum transients (~33 ms response) while smoothing out the carrier frequency ripple from the bandpass filter outputs. This makes per-band energy readings stable and usable as modulation sources rather than jittering RF-style readouts.

**Spectral centroid** is the energy-weighted average band index, normalized to [0, 1]. A centroid of 0 means all energy is concentrated in sub-bass (very dark). A centroid of 1 means energy is in the air band (extremely bright). In practice, most synthesized patches sit in the 0.2–0.6 range with slow movement across that span.

**Spectral flux** is the frame-to-frame change in total energy — effectively a derivative. Positive spikes indicate transient onsets (a kick drum hits, a snare snaps, a sharp attack fires). Negative values indicate decay phases. Flux is not smoothed so it can drive transient-responsive modulation: the value changes as fast as the audio does.

**Transient detection** is a binary flag (0.0 or 1.0) that fires when flux exceeds 0.05. This threshold was tuned to catch kick and snare onsets without false-triggering on the slower energy changes of pad motion or filter sweeps.

### The AutoPulse

The AutoPulse is a self-oscillating rhythm generator that produces trance-like pulse patterns without MIDI input. It operates independently of incoming audio, but can be bent by the spectral analysis through the `optic_pulseEvolve` parameter.

The pulse shape models a kick drum envelope: instant attack, exponential decay. At `optic_pulseShape = 0`, decay is 30 ms — a sharp transient, like a 909 kick. At `optic_pulseShape = 1`, decay stretches to 500 ms — a slow swell, like an LFO rising on a pad. The parameter space between these extremes covers all of the rhythmic weight vocabulary from staccato snap to languid breath.

When `optic_pulseEvolve` is above zero, three spectral features feed back into the pulse behavior:
- **Spectral centroid** shifts pulse rate by ±20%: brighter audio slightly accelerates the beat
- **Total energy** extends decay time by up to 50%: louder passages make pulses ring longer
- **Spectral flux** adds stochastic accent variation of ±30%: transients randomly boost or dip pulse amplitude

A secondary 14-second drift LFO adds ±3% organic rate variation that prevents the pulse from sounding mechanical even without any audio input.

The 16-channel lock-free atomic bus uses `std::memory_order_relaxed` for all reads and writes. This is correct because each channel is an independent float (no inter-channel ordering is required), UI reads are cosmetic (a stale frame is invisible), and coupling reads happen within the same audio callback that writes the values.

---

## The 16 Parameters

### Analysis Section

| Parameter | Range | Default | Function |
|-----------|-------|---------|----------|
| `optic_reactivity` | 0–1 | 0.7 | How sensitively OPTIC responds to incoming audio. At 0, the analyzer barely tracks changes. At 1, it mirrors every spectral shift immediately. |
| `optic_inputGain` | 0–4 | 1.0 | Input gain before the analyzer. Boost for quiet sources; reduce if the modulation outputs are saturating at 1.0 constantly. |

**Sweet spot:** `reactivity` around 0.5–0.8 gives readable tracking without excessive jitter. For percussive sources (ONSET), lean toward 0.8–1.0. For slow pads (ODYSSEY), 0.4–0.6 lets the centroid drift smoothly rather than jumping.

### AutoPulse Section

| Parameter | Range | Default | Function |
|-----------|-------|---------|----------|
| `optic_autoPulse` | Off/On | Off | Enables the self-oscillating rhythm generator |
| `optic_pulseRate` | 0.5–16 Hz | 2.0 Hz | Base pulse frequency. 0.5 = once every 2 seconds; 16 = very fast flutter. Typical trance: 1–4 Hz. |
| `optic_pulseShape` | 0–1 | 0.2 | 0 = sharp kick (30 ms decay), 1 = soft swell (500 ms decay) |
| `optic_pulseSwing` | 0–1 | 0.0 | Swing amount — attenuates even subdivisions by up to 30%, creating triplet-feel lazy groove |
| `optic_pulseEvolve` | 0–1 | 0.5 | Spectral feedback: how much the analyzed audio bends rate, decay, and accent |
| `optic_pulseSubdiv` | 0–1 | 0.0 | Subdivision: 0 = whole beat, 0.15+ = quarter, 0.35+ = 8th notes, 0.6+ = 16th notes |
| `optic_pulseAccent` | 0–1 | 0.7 | Accent strength on downbeats vs. subdivisions. 0 = flat, 1 = strong downbeat contrast |

**Performance note:** `optic_pulseRate` has a skewed NormalisableRange (factor 0.35) so the lower half of the knob covers the musical range of 0.5–4 Hz (whole beats at 30–240 BPM), and the upper half covers 4–16 Hz (subdivisions and flutter effects). This makes the rate knob behave like a musical parameter rather than a linear frequency control.

### Modulation Output Section

| Parameter | Range | Default | Function |
|-----------|-------|---------|----------|
| `optic_modDepth` | 0–1 | 0.5 | Master output scale for the composite coupling signal. This is OPTIC's global volume knob. |
| `optic_modMixPulse` | 0–1 | 0.5 | How much the AutoPulse contributes to the composite coupling output |
| `optic_modMixSpec` | 0–1 | 0.5 | How much total spectral energy contributes to the composite coupling output |

**The composite formula:** `output = (pulse × modMixPulse + energy × modMixSpec) × modDepth`

This composite drives channels 0/1/2 of `getSampleForCoupling()`. For granular coupling to individual spectral features (bass, centroid, transient, etc.), use channels 3–10 directly from the MegaCouplingMatrix.

### Visualizer Section

| Parameter | Range | Default | Function |
|-----------|-------|---------|----------|
| `optic_vizMode` | Scope/Spectrum/Milkdrop/Particles | Milkdrop | Visualization style on OPTIC's engine panel |
| `optic_vizFeedback` | 0–1 | 0.6 | Frame-to-frame feedback in Milkdrop/Particles mode (longer trails) |
| `optic_vizSpeed` | 0.1–4 | 1.0 | Visual animation rate multiplier |
| `optic_vizIntensity` | 0–1 | 0.7 | Brightness of the phosphor green glow |

The visualizer parameters are purely cosmetic — they control the `OpticVisualizer` UI component and have no effect on the modulation outputs. However, they are real parameters stored in presets, making the visual character of OPTIC patches part of the preset identity.

---

## Coupling Manual

### What OPTIC Accepts as Input

OPTIC's `applyCouplingInput()` accepts any coupling type but treats them differently:

| Coupling Type | How OPTIC Uses It | Level Contribution |
|--------------|-------------------|-------------------|
| `AudioToFM` | Audio is analyzed for spectral content | Full signal (rectified block average) |
| `AudioToRing` | Same as AudioToFM | Full signal |
| `AudioToWavetable` | Same as AudioToFM | Full signal |
| `FilterToFilter` | Same as AudioToFM | Full signal |
| `AmpToFilter` | Adds amplitude-derived input | ×0.5 |
| `AmpToPitch` | Adds amplitude-derived input | ×0.5 |
| `RhythmToBlend` | Adds rhythm signal as input energy | ×0.3 |

In practice, use any audio-routing coupling type to feed OPTIC the source it should analyze. AudioToFM is the most semantically neutral choice — it simply means "send this engine's audio to OPTIC for analysis."

Multiple engines can feed OPTIC simultaneously. Their coupling input levels are summed. This allows OPTIC to analyze the full spectral content of a multi-engine patch rather than just one source.

### What OPTIC Sends as Output

OPTIC's 11 output channels span from a simple composite to individual spectral features:

| Channel | Signal | Best For | Coupling Target Examples |
|---------|--------|----------|--------------------------|
| 0/1 | Composite (pulse+spectrum blend) | General rhythmic modulation | Filter cutoff, amplitude, send level |
| 2 | Composite envelope (block value) | Slow-moving gates | VCA, reverb send |
| 3 | AutoPulse level | Kick-shaped rhythmic modulation | Filter cutoff pumping, tremolo |
| 4 | Bass energy | Low-end responsive control | Harmonic content, saturation drive |
| 5 | Mid energy | Vocal/presence responsive | Formant filter position, EQ |
| 6 | High energy | Air/shimmer responsive | Reverb size, chorus depth, shimmer amount |
| 7 | Spectral centroid (brightness) | Timbral brightness tracking | Filter cutoff, oscillator brightness |
| 8 | Spectral flux | Transient-responsive control | Attack time, chaos/randomness |
| 9 | Total energy | Overall loudness tracking | Sidechain compression-style pumping |
| 10 | Transient detection | Binary gate on onsets | Envelope resets, effect triggers |

### The Light Vocabulary: Translating the Metaphor

OPTIC's source code and concept brief speak in terms of light: *brightness*, *centroid*, *flash*, *luminous*, *phosphor*. These are not decorative — they map directly to synthesis parameters.

**Brightness → spectral centroid (ch7):** In optics, a bright light source has energy concentrated at high frequencies. In OPTIC, a high centroid value (near 1.0) means the analyzed audio is bright — full of high-frequency content. Route `centroid → optic_modMixSpec` and that brightness can literally drive the brightness of another engine's filter cutoff. A bright analyzed signal opens filters. A dark analyzed signal closes them. This creates *spectral mimicry*: the character of one engine shapes the tonal response of another.

**Flash → AutoPulse + transient detection (ch3, ch10):** A bioluminescent flash is an event — discrete, rhythmic, brief. The AutoPulse produces a continuous stream of these events at a musical rate (0.5–16 Hz). Transient detection produces them only when the analyzed audio contains a sudden energy onset. Route either to a filter cutoff for rhythmic pumping, to an LFO reset for synchronized sweep, or to a reverb send for gated reverb effects.

**Pulse → total energy (ch9) and composite (ch0):** The "pulse" of a bioluminescent organism follows the movement of the water around it. OPTIC's energy tracking mirrors this: total energy rises when the music swells and falls when it recedes. Route this to a reverb wet/dry to create a mix that opens up during louder passages — the musical equivalent of a creature glowing brighter in agitated water.

**Colony synchronization → multiple outputs routing to multiple engines:** No single output defines OPTIC's identity. The paradigm is that the entire gallery responds to OPTIC's analysis. Bass energy pumps OBESE's filter. Centroid drifts ODYSSEY's JOURNEY macro. Transient detection triggers ONSET fills. AutoPulse syncs the groove across all layers. When all of these are active simultaneously, the XOlokun patch becomes a single organism responding to its own sound — the colony synchronizing through luminous modulation.

---

## Coupling Patches: Five Demonstrations

### Patch 1: The Sidechain Ghost

**Engines:** OPTIC + OBESE + any kick/drum source

**Setup:**
1. Load ONSET (or OBLONG with a kick-like patch) in slot A
2. Load OBESE in slot B
3. Load OPTIC in slot C
4. Route: ONSET → OPTIC (AudioToFM), OPTIC → OBESE (composite, ch0)
5. Set OBESE's M3 (COUPLING) to target `fat_filterCutoff`

**OPTIC settings:** `autoPulse=Off`, `reactivity=0.9`, `modMixPulse=0.0`, `modMixSpec=1.0`, `modDepth=0.6`

**What happens:** ONSET's kick drums feed OPTIC's analyzer. Each kick hit creates a spike in bass energy and total energy. The composite output delivers that spike to OBESE's filter cutoff, creating a ducking/pumping effect without any sidechain compression. The result is more musical than traditional sidechain because it's spectrally aware: the pump depth varies with how hard the kick hits, and the spectral centroid slightly modulates the filter character on each pump.

**The ghost aspect:** The kick drum is *heard* through its effect on the bass synth, not through direct audio routing. OPTIC becomes the invisible mediator between rhythm and tone — present everywhere, heard nowhere.

---

### Patch 2: The Trance Heartbeat

**Engines:** OPTIC + ODYSSEY (or any pad engine)

**Setup:**
1. Load ODYSSEY in slot A with a slow evolving pad preset
2. Load OPTIC in slot B
3. Route: ODYSSEY → OPTIC (AudioToFM)
4. Route: OPTIC → ODYSSEY ch3 (AutoPulse) → `odyssey_detune`
5. Route: OPTIC → ODYSSEY ch7 (centroid) → `odyssey_filterCutoff`

**OPTIC settings:** `autoPulse=On`, `pulseRate=1.0 Hz`, `pulseShape=0.6`, `pulseEvolve=0.7`, `modDepth=0.4`

**What happens:** OPTIC analyzes ODYSSEY's pad as it evolves. The AutoPulse at 1 Hz (one beat per second, 60 BPM) creates a slow rhythmic detune sweep — the pad's unison width breathing in and out like a chest. The spectral centroid tracking means that as ODYSSEY's JOURNEY macro makes the pad brighter, the filter also opens — the pad brightens spectrally and tonally in parallel. The evolve parameter means the pulse rhythm itself is affected by the pad's energy, creating an emergent loop: the pad shapes the pulse, the pulse shapes the pad, and they breathe together.

**The revelation:** Neither engine is static. The pad is not just playing a note — it is interacting with its own modulator through OPTIC's analysis loop.

---

### Patch 3: The Spectral Compass

**Engines:** OPTIC + any three engines

**Setup:**
1. Load any three engines in slots A, B, C
2. Load OPTIC in slot D
3. Route all three engines → OPTIC (AudioToFM)
4. Route OPTIC ch7 (centroid) to a different parameter on each engine:
   - Engine A: filter cutoff
   - Engine B: reverb size
   - Engine C: a harmonic parameter (FM ratio, formant position, etc.)

**OPTIC settings:** `autoPulse=Off`, `reactivity=0.6`, `modMixSpec=1.0`, `modDepth=0.5`

**What happens:** OPTIC receives the summed spectral content of all three engines. As the combined audio evolves — different notes played, different filters opening — the spectral centroid slowly shifts. When the combined patch is bright, all three modulation targets move upward simultaneously. When the patch is dark (bass notes, low filters), they all move down. This creates a *unified tonal response* across three independent engines — they don't know they are coupled to each other, but they behave as if they share a nervous system.

**The compass metaphor:** The centroid is a compass needle pointing from dark to bright. Every engine in the patch navigates by the same reading.

---

### Patch 4: The Transient Broom

**Engines:** OPTIC + ODDFELIX (or ONSET) + ODYSSEY

**Setup:**
1. Load ODDFELIX in slot A (percussive snaps)
2. Load ODYSSEY in slot B (ambient pad, heavy reverb)
3. Load OPTIC in slot C
4. Route: ODDFELIX → OPTIC (AudioToFM)
5. Route: OPTIC ch10 (transient detection) → ODYSSEY reverb send `odyssey_fxReverbSize` (or equivalent)
6. Invert the coupling polarity so transient events *reduce* reverb size momentarily

**OPTIC settings:** `autoPulse=Off`, `reactivity=1.0`, `modDepth=0.7`

**What happens:** Every snap from ODDFELIX registers as a transient in OPTIC's analyzer. That binary trigger (0 or 1) fires to ODYSSEY's reverb, momentarily tightening the space. Between snaps, the reverb swells back out to full size. The result is a gated reverb effect that is not periodic but percussion-driven — the reverb breathes in reaction to the snaps, not a clock.

**The broom metaphor:** Each transient sweeps the reverb tail shorter, then the room fills back up. OPTIC is the broom that only moves when ODDFELIX snaps.

---

### Patch 5: The Thirty-Minute Drift

**Engines:** OPTIC + OCEANDEEP + OVERDUB

**Setup:**
1. Load OCEANDEEP in slot A (deep evolving bass/texture)
2. Load OVERDUB in slot B (tape delay + spring reverb)
3. Load OPTIC in slot C
4. Route: OCEANDEEP → OPTIC (AudioToFM), OVERDUB → OPTIC (AudioToFM)
5. Route: OPTIC ch9 (total energy) → OVERDUB `dub_sendAmount`
6. Route: OPTIC ch7 (centroid) → OCEANDEEP macro CHARACTER
7. Set OPTIC `pulseEvolve=1.0`, `pulseRate=0.5 Hz`, `pulseShape=1.0` (softest swell)

**OPTIC settings:** `autoPulse=On`, `pulseRate=0.5 Hz`, `pulseShape=1.0` (slow swell), `pulseEvolve=1.0`, `modDepth=0.3`

**What happens:** At half a hertz, the AutoPulse beats once every two seconds — a slow oceanic breathing. With `pulseShape=1.0`, the pulse is a 500 ms swell rather than a kick snap. With `pulseEvolve=1.0`, OCEANDEEP's spectral content fully controls the pulse character: louder passages make the swell longer, brighter passages slightly accelerate it. The total energy drives OVERDUB's dub send — when OCEANDEEP is loudest, the delay deepens; when it recedes, the delay dries up. The centroid tracks OCEANDEEP's harmonic evolution and steers its CHARACTER macro in real time.

Set this running. Walk away for thirty minutes. Return to a completely different patch.

This is the Schulze prophecy: OPTIC as a performance in time — a slow-motion composer reshaping everything it touches across a timescale no hand automation could manage.

---

## Performance Guide: Playing an Instrument You Cannot Hear

### The Fundamental Reframe

Every other instrument in the gallery answers the question "what note am I playing?" OPTIC answers a different question: "how is everything else moving?"

Playing OPTIC is an act of curation and shaping, not melody or rhythm. When you reach for an OPTIC parameter in performance, you are changing how the rest of the patch responds to itself, not adding a new voice to the mix.

### Performance Parameters

**`optic_reactivity` as the sensitivity dial:** This is the most expressive real-time OPTIC control. At low values (0.1–0.3), OPTIC barely tracks the audio — the modulation outputs become slow, smooth signals that change over many seconds. At high values (0.8–1.0), OPTIC tracks every transient and spectral movement, producing fast reactive modulation. Sweep this during a performance to change how tightly the coupled engines track the music.

**`optic_modDepth` as the coupling master:** This is OPTIC's volume in the modulation domain. At 0, coupled engines receive no signal and behave autonomously. At 1.0, OPTIC's composite output drives modulation targets at full range. Sweep this in and out to transition between sections where engines are independent versus fully coupled.

**`optic_pulseRate` + `optic_pulseSubdiv` for rhythmic control:** These two together define the rhythmic character of the AutoPulse. `pulseRate` sets the base tempo; `pulseSubdiv` multiplies it into subdivisions. Modulating `pulseRate` during a performance changes the groove in the way a tempo change does — but because OPTIC drives other engines rather than producing sound directly, the tempo change feels more like a current changing direction than a machine speeding up.

**`optic_pulseEvolve` as the feedback governor:** At 0, the AutoPulse is a fixed clock. At 1, the analyzed audio fully controls the pulse's character. Increasing this during a performance opens a feedback loop between the gallery's audio output and its own modulation source. At maximum evolve, the system becomes emergent — patterns arise that no single parameter controlled.

**`optic_modMixPulse` / `optic_modMixSpec` for source blending:** These two parameters define the character of the composite output. Full pulse, zero spectrum = purely rhythmic modulation (gates and pumps). Zero pulse, full spectrum = purely timbral modulation (brightness and energy following). Equal mix = both simultaneously. This transition can be swept during a performance to move between driving rhythms and gliding textural shifts.

### What "Playing" Looks Like

A performance with OPTIC active might look like:

1. **Intro:** `modDepth=0`, all engines independent. Establish the patch's natural sound.
2. **Build:** Slowly increase `modDepth` to 0.3–0.5. Coupled engines begin responding to each other's spectral content. The patch starts to move as a unit.
3. **Drop:** Enable AutoPulse, set `pulseRate` to the track's BPM / 60. The rhythmic coupling snaps into time. Bass starts pumping. Pads begin to breathe.
4. **Evolution:** Increase `pulseEvolve` to 0.7–1.0. The pulse feedback loop opens. The rhythm begins to drift and evolve in response to the audio it is modulating.
5. **Breakdown:** Cut `autoPulse=Off`, increase `reactivity` to maximum. OPTIC now mirrors every spectral event in real time — the patch becomes fully reactive without a rhythmic anchor.
6. **Outro:** Slowly reduce `modDepth` back toward 0. The engines decouple. Each returns to its own voice.

This is a complete structural arc controlled primarily through one engine that never makes a single sound.

---

## Vision V002: Where the Zero-Audio Paradigm Is Headed

Klaus Schulze, reviewing OPTIC in the 2026-03-14 seance, described it as "the prototype for a new category of instruments — meta-synths that don't make sound but reshape the sounds of everything around them."

His specific requests for future OPTIC development:

**Polyrhythmic ratios for AutoPulse:** Currently, `optic_pulseSubdiv` offers four subdivision levels. V002 envisions multiple independent pulse generators running at rational ratio relationships (3:4, 5:8, 7:12) — polyrhythmic modulation sources that create cross-rhythmic pumping patterns across different coupled engines simultaneously.

**Glacial decay times:** The AutoPulse currently decays in 30–500 ms. Schulze wants decay times measured in minutes — pulses that fire every few seconds and ring for 60 seconds or longer. This would enable OPTIC to function as a very slow macro controller: one "beat" per minute that shapes the entire gallery's character arc over the course of a performance.

**Spectral memory:** Currently, OPTIC analyzes only the present moment. A future OPTIC could track spectral state over longer windows — noticing that the last 30 seconds have been consistently bright and predicting or pre-shaping the next 30 seconds accordingly. This is synthesis with temporal intelligence.

**Cascade coupling:** Currently, OPTIC receives audio and outputs modulation. V002's prophecy points toward OPTIC receiving OPTIC outputs from another slot — creating a chain of meta-modulation where one OPTIC's pulse shapes another OPTIC's analysis parameters, which then modulates the gallery.

The Ongoing Debate (DB002) about zero-audio accessibility has a documented resolution path: OPTIC keeps its zero-audio identity permanently (Blessing B005, protected), but gains UX scaffolding — a first-load tutorial, visual coupling suggestions in the UI, and a "sonify" preview mode that temporarily converts control signals to audio for verification. The paradigm does not compromise; the confusion does not survive.

---

## Onboarding Path: Recommended First-Time User Experience

The seance noted the onboarding concern unanimously: a first-time user who loads OPTIC solo hears silence. The recommended path that preserves the zero-audio identity while eliminating confusion:

### Step 1: Read the Phosphor Green Signal
Load OPTIC in any slot. Notice the visualizer activates immediately — the Milkdrop-style display shows a flat, dark response. This confirms the engine is running. The phosphor green accent on the engine panel is the visual signature that OPTIC is present and processing.

### Step 2: Couple to One Engine First
Add any audio-producing engine in an adjacent slot. Route that engine → OPTIC using AudioToFM coupling. Watch the visualizer respond: the spectral display lights up with the analyzed audio's shape. This is OPTIC seeing.

### Step 3: Enable the AutoPulse
Turn `optic_autoPulse` to On. Set `optic_pulseRate` to 2 Hz. The visualizer begins pulsing rhythmically — phosphor green flashes at two beats per second. The engine is now producing a rhythm signal. But you still hear nothing new. OPTIC has not added audio to the mix.

### Step 4: Route OPTIC's Output
Add a second audio engine. Route OPTIC (composite, ch0) → that engine's filter cutoff parameter using AmpToFilter coupling. Slowly increase `optic_modDepth`. The second engine's filter begins pumping in time with the AutoPulse. You hear the OPTIC engine for the first time — not as a sound, but as a change in another engine's sound.

This is the paradigm.

### Step 5: Explore Spectral Routing
Replace the first engine (the analysis source) with ONSET or ODDFELIX. Route their transient-heavy audio to OPTIC. Watch the visualizer show the spectral flash of each kick and snap. Route OPTIC ch10 (transient detection) to an effect parameter on a pad engine. Each drum hit shapes the pad's response.

### Summary
The progression is: see → analyze → pulse → modulate → explore. Each step takes about thirty seconds. By step 5, the user understands that OPTIC is not a broken synth but a different kind of instrument — one that speaks only through the behavior of everything it touches.

---

## Quick Reference

### Parameter Summary

| Parameter | ID | Range | Default | Section |
|-----------|-----|-------|---------|---------|
| Reactivity | `optic_reactivity` | 0–1 | 0.7 | Analysis |
| Input Gain | `optic_inputGain` | 0–4 | 1.0 | Analysis |
| AutoPulse | `optic_autoPulse` | Off/On | Off | AutoPulse |
| Pulse Rate | `optic_pulseRate` | 0.5–16 Hz | 2.0 | AutoPulse |
| Pulse Shape | `optic_pulseShape` | 0–1 | 0.2 | AutoPulse |
| Pulse Swing | `optic_pulseSwing` | 0–1 | 0.0 | AutoPulse |
| Pulse Evolve | `optic_pulseEvolve` | 0–1 | 0.5 | AutoPulse |
| Pulse Subdivision | `optic_pulseSubdiv` | 0–1 | 0.0 | AutoPulse |
| Pulse Accent | `optic_pulseAccent` | 0–1 | 0.7 | AutoPulse |
| Mod Depth | `optic_modDepth` | 0–1 | 0.5 | Mod Output |
| Mod Mix Pulse | `optic_modMixPulse` | 0–1 | 0.5 | Mod Output |
| Mod Mix Spectrum | `optic_modMixSpec` | 0–1 | 0.5 | Mod Output |
| Viz Mode | `optic_vizMode` | Scope/Spectrum/Milkdrop/Particles | Milkdrop | Visualizer |
| Viz Feedback | `optic_vizFeedback` | 0–1 | 0.6 | Visualizer |
| Viz Speed | `optic_vizSpeed` | 0.1–4 | 1.0 | Visualizer |
| Viz Intensity | `optic_vizIntensity` | 0–1 | 0.7 | Visualizer |

### Coupling Channel Quick Reference

| Ch | Signal | Smooth? | Best Use |
|----|--------|---------|---------|
| 0/1 | Composite output | No | General rhythmic modulation |
| 2 | Composite envelope | Inherits | Gate-style control |
| 3 | AutoPulse | No | Rhythmic pumping |
| 4 | Bass energy | Yes | Low-frequency responsive |
| 5 | Mid energy | Yes | Presence/vocal responsive |
| 6 | High energy | Yes | Air/shimmer responsive |
| 7 | Centroid | Yes | Timbral brightness tracking |
| 8 | Flux | No | Transient-responsive |
| 9 | Total energy | Yes | Loudness tracking |
| 10 | Transient gate | No | Binary event triggering |

### Compatible Coupling Input Types

AudioToFM, AudioToRing, AudioToWavetable, FilterToFilter (full signal), AmpToFilter, AmpToPitch (×0.5 gain), RhythmToBlend (×0.3 gain).

---

*The silence IS the identity. OPTIC does not make sound. It makes everything else move.*

*XO_OX Designs | XOptic — the colony pulses, and the ocean remembers the rhythm*
