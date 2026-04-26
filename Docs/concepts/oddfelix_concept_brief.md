# OddfeliX — Concept Brief (Retrospective)

*Retrospective Phase 0 | March 2026 | XO_OX Designs*
*Enriching the first generation with the aquatic mythology*

---

## Identity

**XO Name:** OddfeliX
**Gallery Code:** ODDFELIX
**Accent Color:** Neon Tetra Blue `#00A6D6`
**Parameter Prefix:** `snap_`
**Engine Dir:** `Source/Engines/OddfeliX/`

**Thesis:** Percussive transient synthesis -- the snap, the click, the dart of neon through water.

---

## Aquatic Identity

feliX himself. The neon tetra -- a flash of iridescent blue darting through sunlit shallows. He is the X in XO_OX, the initiator, the first spark of movement in the water column. His body is small but impossibly bright, a sliver of electric pigment that catches light at every angle. When a school of neon tetras changes direction, there is a snap -- a collective pivot so fast it registers as percussion. That is the sound OddfeliX makes.

Every note begins with a pitch sweep downward: up to 24 semitones crashing to the target pitch in a quarter of a second. This is the snap of a tail changing direction, the crack of surface tension breaking as a fish darts sideways. The Karplus-Strong mode is the pluck of a fishing line, taut and shimmering. The FM mode is the metallic flash of scales catching sunlight. The Sine+Noise mode is the splash itself -- pure tone colliding with turbulence.

feliX lives at the surface where light is brightest and movement is fastest. He does not sustain. He does not pad. He strikes and vanishes, leaving only the memory of blue in the water. He is pure feliX polarity -- no Oscar in him at all. Speed, transience, brilliance. The neon tetra does not rest.

---

## Polarity

**Position:** The Surface -- sunlit shallows where light is brightest
**feliX-Oscar balance:** 100/0 -- Pure feliX

---

## DSP Architecture (As Built)

The SnapEngine is an 8-voice percussive synthesizer with three oscillator modes, a pitch snap sweep, and a filter cascade.

**Signal flow per voice:**

```
Oscillator (Sine+Noise | FM | Karplus-Strong)
  x Unison (1/2/4 sub-voices with detune + pan scatter)
    |
    v
Tanh Waveshaper (snap-driven saturation)
    |
    v
HPF -> BPF Filter Cascade (Cytomic SVF)
    |
    v
Decay Envelope (no sustain -- percussive only)
    |
    v
Output (stereo via unison pan scatter)
```

**Oscillator modes:**
- **Sine+Noise (0):** PolyBLEP sine carrier plus high-frequency saw as noise source, noise scaled by snap amount. The fundamental splash.
- **FM (1):** 2:1 ratio FM with snap controlling modulation depth (x4). Metallic, bell-like transients.
- **Karplus-Strong (2):** 4096-sample delay-line excitation with damped feedback. Snap controls damping inversely -- higher snap means brighter, shorter plucks. Each unison sub-voice gets its own KS delay line.

**Key DSP details:**
- Pitch sweep: starts at `baseMidi + 24 * snap` semitones, glides down to target over ~250ms. This is the signature "snap" -- a percussive pitch drop.
- Pitch lock mode: locks all notes to C4, making it a pure percussion instrument.
- Unison: 1, 2, or 4 sub-voices with symmetric detune offsets and stereo pan scatter.
- Filter cascade: HPF into BPF, both Cytomic SVF, sculpting the transient character.
- Decay-only envelope: no attack, no sustain. Level starts at 1.0 and decays linearly. Pure percussive character.
- Voice stealing: LRU with 5ms crossfade.
- Polyphony: configurable 1/2/4/8 voices.

**Parameters (10 total):** `snap_oscMode`, `snap_snap`, `snap_decay`, `snap_filterCutoff`, `snap_filterReso`, `snap_detune`, `snap_level`, `snap_pitchLock`, `snap_unison`, `snap_polyphony`.

---

## Signature Sound

OddfeliX sounds like percussion made from light. The pitch snap sweep gives every note a downward crack that is instantly recognizable -- not a drum, not a synth, but the sound of something darting. With Karplus-Strong mode and high snap, it produces shimmering metallic plucks that ring and decay like a struck wire. With FM mode, it creates bell-like transients with harmonic complexity that changes on every velocity. It is always brief, always bright, always moving.

---

## Coupling Thesis

feliX gives transients. His envelope output (channel 2) drives other engines' filters, chokes their tails, pumps their dynamics. A feliX snap into OddOscar's AmpToFilter makes the pad breathe in rhythm with the percussion. A feliX snap into Overdub's echo chamber creates self-generating rhythmic patterns.

He receives sparingly -- pitch modulation (AmpToPitch, LFOToPitch, PitchToPitch) that makes his darting less predictable. A slow LFO from Oscar drifting feliX's pitch by half a semitone makes the percussion feel organic, alive. He is the initiator. He strikes first, and the ecosystem responds.

---

## Preset Affinity

| Foundation | Atmosphere | Entangled | Prism | Flux | Aether |
|-----------|-----------|-----------|-------|------|--------|
| High | Low | High | High | Medium | Low |

---

*XO_OX Designs | OddfeliX -- a flash of neon in sunlit shallows, gone before you name the color*
