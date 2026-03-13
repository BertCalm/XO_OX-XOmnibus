# XOpenSky — Concept Brief

**Date:** March 12, 2026
**Phase:** 0 — Ideation complete
**Status:** Approved for Phase 1 architecture

---

## Identity

- **Name:** XOpenSky
- **Gallery code:** OPENSKY
- **Accent color:** Sunburst `#FF8C00`
- **Parameter prefix:** `sky_`
- **Thesis:** Euphoric synth engine — the shimmering, soaring high where light and air become sound. Van Halen's jump, Vangelis's heaven, the moment you break the surface and look up.
- **Sound family:** Lead / Pad / Anthem / Shimmer
- **Unique capability:** The sound of ascension. Shimmering supersaw anthems, crystalline bell pads, soaring leads that feel like breaking through clouds. Other engines go deep, go weird, go aggressive. OpenSky goes *up*.
- **Max voices:** 16
- **CPU budget:** <10%

---

## Character

- **Personality in 3 words:** Euphoric, Luminous, Soaring
- **Polarity:** Pure feliX — the neon tetra ascends above the water entirely
- **Engine approach:** Supersaw + shimmer synthesis — detuned unison oscillator stacks with harmonic shimmer processing, bright filter sweeps, and air-frequency excitation
- **Why this approach serves the character:**
  The euphoric high of arena synths and Vangelis pads comes from two things: massive detuned unison stacks (width) and harmonic shimmer (light). OpenSky combines both — supersaw unison for the wall-of-sound foundation, plus a shimmer stage that adds octave-up harmonics and crystalline overtones. The result is sound that literally feels like it's rising.

---

## The XO Concept Test

1. **XO word:** XOpenSky ✓ — Open Sky: the space above the ocean, where feliX's light reaches its fullest expression
2. **One-sentence thesis:** "XOpenSky is a euphoric synth engine where shimmering supersaw anthems and crystalline pads soar above the entire XO_OX water column — pure light, pure air, pure ascension." ✓
3. **Sound only this can make:** Arena-sized euphoric leads and pads with harmonic shimmer processing that makes every note feel like sunlight breaking through water — the Van Halen jump, the Vangelis heaven, the moment the surface breaks. No other XOmnibus engine goes *up*. ✓

---

## Aquatic Identity

The open sky above the ocean. feliX has broken the surface completely — for one shimmering moment, neon scales catch the full spectrum of sunlight, the entire water column below, nothing but air and light above. This is what the surface looks like from the other side.

Every other engine lives in the water. OpenSky is what happens when you leave it.

---

## Gallery Gap Filled

| Existing engines | Vertical position |
|-----------------|-------------------|
| XOceanDeep | The floor — abyssal bass, darkness, pressure |
| Most engines | The middle — various depths, various species |
| OddfeliX, XOnset | The surface — transients, brightness |
| **XOpenSky** | **Above the surface — pure light, euphoria, ascension** |

No current engine occupies the space above the water. OpenSky completes the top of the vertical axis.

---

## Core DSP Architecture (Phase 0 sketch)

```
SUPERSAW OSCILLATOR STACK
├── 7-voice unison with per-voice detune
├── Saw / Pulse / Sine selectable per layer
├── Stereo spread via per-voice pan scatter
│
▼
SHIMMER STAGE
├── Octave-up pitch shift + feedback (Eventide-style shimmer)
├── Crystalline reverb with pitch-shifted reflections
├── Harmonic exciter (air-frequency boost, 8-12kHz presence)
│
▼
BRIGHT FILTER
├── High-shelf boost + resonant HPF
├── Cytomic SVF, key-tracked
├── Filter envelope with fast attack for "cutting through"
│
▼
AMP ENVELOPE (configurable — fast for leads, slow for pads)
│
▼
FX CHAIN
├── Chorus (wide, lush, sky-width)
├── Reverb (large hall / cathedral — the sky is the room)
├── Stereo Widener (the sound fills the sky)
│
▼
Output (stereo)
```

**Voice model:** 16-voice polyphony. Supersaw unison is per-voice (7 detuned oscillators per MIDI note). Total oscillator count can be high — CPU management via quality modes.

---

## Parameter Namespace

All parameter IDs use `sky_` prefix. Key parameters:

| ID | Range | Description |
|----|-------|-------------|
| `sky_unisonDetune` | 0-100 cents | Supersaw detune spread |
| `sky_unisonVoices` | 1-7 | Unison voice count per note |
| `sky_oscWave` | 0-2 | Saw / Pulse / Sine |
| `sky_shimmerAmount` | 0-1 | Octave-up shimmer intensity |
| `sky_shimmerDecay` | 0-1 | Shimmer tail length |
| `sky_shimmerTone` | 0-1 | Dark shimmer ↔ crystalline shimmer |
| `sky_filterCutoff` | 20-20kHz | Bright filter cutoff |
| `sky_filterReso` | 0-1 | Filter resonance |
| `sky_airAmount` | 0-1 | High-frequency exciter (8-12kHz presence) |
| `sky_chorusMix` | 0-1 | Width chorus |
| `sky_reverbMix` | 0-1 | Cathedral reverb |
| `sky_reverbSize` | 0-1 | Room size (chapel → open sky) |
| `sky_stereoWidth` | 0-1 | Stereo expansion |
| `sky_attack` | 0.001-4s | Amp envelope attack |
| `sky_decay` | 0.01-4s | Amp envelope decay |
| `sky_sustain` | 0-1 | Amp envelope sustain |
| `sky_release` | 0.01-8s | Amp envelope release |

*Full parameter list defined in Phase 1 architecture.*

---

## Macro Mapping (M1-M4)

| Macro | Label | Controls | Behavior |
|-------|-------|----------|----------|
| M1 | RISE | `sky_shimmerAmount` + `sky_airAmount` + `sky_filterCutoff` | 0 = warm, grounded. 1 = pure crystalline ascension. The altitude dial. |
| M2 | WIDTH | `sky_unisonDetune` + `sky_chorusMix` + `sky_stereoWidth` | 0 = focused beam. 1 = fills the entire sky. The expansion dial. |
| M3 | GLOW | `sky_shimmerDecay` + `sky_reverbMix` + `sky_reverbSize` | 0 = dry, present. 1 = infinite shimmer tail. The afterglow. |
| M4 | AIR | `sky_reverbSize` + `sky_stereoWidth` + `sky_filterCutoff` high-shelf | 0 = intimate chapel. 1 = open sky, nothing above you. The space dial. |

---

## Coupling Interface Design

### OPENSKY as Target (receiving)

| Coupling Type | What XOpenSky Does | Musical Effect |
|---------------|-------------------|----------------|
| `AmpToFilter` | Amplitude drives filter brightness | Drum hits make the sky flash brighter |
| `EnvToMorph` | Envelope drives shimmer amount | External dynamics control the ascension |
| `LFOToPitch` | LFO drives unison detune | Cross-engine width modulation |

### OPENSKY as Source (sending)

| Target Engine | Coupling Type | Musical Effect |
|-------------|---------------|----------------|
| OCEANDEEP | AmpToFilter | Sky shimmer drives abyssal filter — light reaching the deep |
| OVERDUB | getSample | Euphoric leads through dub echo — heavenly dub |
| OPAL | AudioToWavetable | Shimmer granulated into light particles |
| OVERBITE | AmpToFilter | Sky energy drives bass bite — brightness meets feral |

### Signature Coupling Route
> **OPENSKY × OCEANDEEP** — "The Full Column" — euphoric shimmer over abyssal 808 bass. The entire XO_OX mythology in one patch. feliX's sky over Oscar's floor. This is the coupling that completes the universe.

### Coupling types OPENSKY should NOT receive
- `AmpToChoke` — you don't choke the sky
- `AudioToFM` — shimmer + FM creates unmusical artifacts

---

## Preset Strategy (Phase 0 sketch)

**80 presets at v1.0:**

| Category | Count | Character |
|----------|-------|-----------|
| Arena Anthems | 15 | Van Halen jump, trance leads, festival-sized |
| Heaven Pads | 15 | Vangelis pads, slow shimmer, infinite reverb |
| Crystal Leads | 15 | Bright, cutting, soaring monophonic leads |
| Sunrise Textures | 10 | Slow-building, dawn-energy, golden hour |
| Shimmer Bells | 10 | Crystalline bell tones with shimmer tails |
| Coupling Showcases | 10 | Designed for specific engine pairs |
| Euphoria | 5 | Maximum RISE, maximum WIDTH, maximum everything |

### Naming Convention
Names should feel like looking up:
- "Break The Surface"
- "Nothing But Sky"
- "Golden Hour"
- "Van Halen Moment"
- "Sunlight On Scales"
- "feliX Ascending"

---

## Visual Identity

- **Accent color:** Sunburst `#FF8C00`
- **UI concept:** Bright, warm, open — the opposite of every dark synth UI. Gold and white dominate. The panel feels like looking at the sky through water.
- **Color palette:** Warm white base, sunburst orange accents, golden highlights. Bright, warm, inviting.

---

## Mood Affinity

| Mood | Affinity | Why |
|------|----------|-----|
| Foundation | Medium | Supersaw pads as harmonic foundation |
| Atmosphere | **High** | Shimmer pads, cathedral reverb, infinite tails |
| Entangled | **High** | The Full Column coupling showcase |
| Prism | **High** | Bright, colorful, refractive — this IS Prism |
| Flux | Low | OpenSky is stable, ascending — not glitchy |
| Aether | Medium | Extreme shimmer presets qualify |

---

## Decision Gate: Phase 0 → Phase 1

- [x] Concept brief written
- [x] XO word confirmed (XOpenSky — the sky above the water, feliX ascending)
- [x] Gallery gap clear (no engine above the surface)
- [x] Coupling partners defined (OCEANDEEP, OVERDUB, OPAL, OVERBITE)
- [x] Aquatic mythology position (top of the water column)
- [x] Excited about the sound

**→ Proceed to Phase 1: Architecture**
*Invoke: `/new-xo-engine phase=1 name=XOpenSky identity="Euphoric shimmer synth — supersaw anthems, crystalline pads, the soaring high above the water" code=XOsk`*

---

*XO_OX Designs | Engine: OPENSKY | Accent: #FF8C00 | Prefix: sky_*
