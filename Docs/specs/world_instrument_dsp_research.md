# World Instrument DSP Research — Collections Preparation

**Date**: 2026-03-16
**Project**: XO_OX Designs — XOmnibus Engine Design
**Target Collections**: Artwork/Color Collection, Travel/Water Collection
**Status**: R&D / Pre-Implementation

---

## Overview

This document provides acoustic physics analysis and DSP implementation guidance for 8 world instruments targeted for the Artwork/Color and Travel/Water collections. Each entry covers the physical sound production mechanism, timbre profile, JUCE/C++ modeling strategy, exposed parameters, coupling potential with existing XOmnibus engines, and XPN velocity mapping.

All DSP approaches assume the XOmnibus architecture: `juce::dsp::ProcessorChain`, per-voice DSP graphs, `AudioProcessorValueTreeState` parameter management, and the existing coupling infrastructure.

---

## 01 — Erhu (Chinese 2-String Fiddle)

**Collection**: Artwork/Color — Color Quad A | Accent: Oxblood `#800020`

### Physical Acoustic Mechanism
The erhu is a spike fiddle: two silk (historically) or metal-wound strings are bowed with a horsehair bow whose hairs pass *between* the strings — the bow cannot be removed during play. The resonator is a hexagonal or octagonal wooden cylinder with a python-skin membrane stretched over one face. This membrane is the primary radiating surface; the back of the resonator is open. Bow pressure and speed control the frictional excitation. The strings have no fingerboard — pitch is controlled by the player's fingers stopping the string in the air, producing a characteristic smooth, gliding portamento. Sympathetic resonance between the two strings contributes subtle harmonic coloring.

### Key Timbre Characteristics
- **Attack**: Extremely soft with bowing — near-zero if bow is already moving, or with a slight pre-bow "catch." Can be harsh if bow pressure is excessive (scratch tone).
- **Sustain**: Continuous while bowing. Highly expressive dynamic range within a note via bow pressure.
- **Harmonics**: Fundamental dominant. Strong 2nd and 3rd harmonics. Python skin membrane introduces a subtle inharmonic noise floor — the resonator does not have the clean wood-box response of a violin. Upper harmonics roll off faster than a Western violin.
- **Vibrato**: Fast, narrow, finger-controlled — characteristic Chinese bowing vibrato differs from Western violin vibrato in rate and depth.
- **Portamento**: Essential — gliding pitch transitions between notes are idiomatic, not ornamental.

### DSP Modeling Approach (JUCE/C++)

**Primary synthesis**: Physical modeling with a bowed string + resonator model.

```cpp
// Karplus-Strong extended with bow friction model
class ErhusStringVoice {
    // Delay line for string (length = sample_rate / pitch_hz)
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> stringDelay;

    // Bow friction: non-linear stick-slip model
    // f(v_rel) = bow_force * (sign(v_rel) - tanh(v_rel / v_bow))
    float bowFrictionModel(float relativeVelocity, float bowForce) {
        return bowForce * (std::tanh(relativeVelocity / 0.3f));
    }

    // Python skin membrane: bandpass resonator modeling membrane rattle
    juce::dsp::IIR::Filter<float> membraneResonator; // ~1200-2400 Hz bandpass
    juce::dsp::IIR::Filter<float> bodyFilter;         // ~300-800 Hz body resonance

    // Portamento glide
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> pitchGlide;
};
```

**Body resonance**: The cylindrical body + python skin introduces a strong resonance peak around 300–500 Hz and a membrane "crunch" band around 1.2–2.4 kHz. Model with two parallel IIR bandpass filters added back to the dry signal:

```cpp
// Body resonance: peak EQ at ~400 Hz, Q=3.0
auto bodyCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
    sampleRate, 400.0f, 3.0f, 4.0f  // +12dB peak
);
// Membrane "snake skin" presence: peak at ~1600 Hz, Q=2.5
auto membraneCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
    sampleRate, 1600.0f, 2.5f, 6.0f  // +18dB peak, distinctive erhu character
);
```

**Vibrato**: LFO (4–7 Hz, 10–30 cents depth) applied to pitch. Rate and depth mapped to separate parameters.

### Key Parameters to Expose
| Parameter | Range | Description |
|---|---|---|
| `erhu_bow_pressure` | 0.0–1.0 | Bow force — low = breathy/airy, high = full/scratchy |
| `erhu_bow_speed` | 0.0–1.0 | Bow velocity — controls brightness and harmonic content |
| `erhu_portamento` | 0–500ms | Pitch glide time between notes |
| `erhu_membrane_resonance` | 0.0–1.0 | Python skin presence (+dB at 1.6kHz) |
| `erhu_body_resonance` | 0.0–1.0 | Body cavity resonance (+dB at 400Hz) |
| `erhu_vibrato_rate` | 2–12 Hz | Vibrato speed |
| `erhu_vibrato_depth` | 0–50 cents | Vibrato width |
| `erhu_sympathetic` | 0.0–1.0 | Sympathetic string shimmer (high-frequency comb add) |

### Coupling Potential with XOmnibus Engines
- **DRIFT** (granular): Erhu bow noise layer extracted and granularized — produces textured legato pads
- **OPAL** (granular): Freeze on sustained vowel-like erhu tone → evolving pad
- **OVERDUB** (tape delay): Natural pairing — erhu through spring reverb + tape delay is a classic dub texture
- **ONSET**: Erhu staccato bowing → organic percussion transients for kit layers

### XPN Velocity Mapping
| Velocity range | Bow pressure | Sonic result |
|---|---|---|
| 1–40 | Low (0.1–0.3) | Barely touching bow, breathy harmonics |
| 41–80 | Medium (0.3–0.6) | Full, expressive tone — primary playable range |
| 81–110 | High (0.6–0.85) | Full bright tone with bow crunch |
| 111–127 | Extreme (0.85–1.0) | Scratch tone / col legno effect |

---

## 02 — Didgeridoo

**Collection**: Artwork/Color — Color Quad A | Accent: Onyx `#353839`

### Physical Acoustic Mechanism
The didgeridoo is an end-blown aerophone: a hollowed eucalyptus log (termites excavate the interior) with a beeswax mouthpiece. Sound is produced by buzzing lips against the mouthpiece (similar to a trombone mouthpiece but more diffuse). The tube is typically 1–1.5 meters long, producing a fundamental in the 60–80 Hz range. Circular breathing — inhaling through the nose while exhaling through cheeks — sustains a continuous drone. Overtone cycling produces the characteristic "didj talk" by modifying vocal tract resonance (changing vowel shapes with the mouth, tongue, and throat). Harmonics 2, 3, 4, and 5 are actively controlled.

### Key Timbre Characteristics
- **Attack**: Drone onset is gradual — lip buzz ramping into the tube resonance
- **Sustain**: Continuous indefinitely via circular breathing
- **Harmonics**: Fundamental (60–80 Hz) + strong odd harmonics. Even harmonics present but weaker (closed-end tube behavior). Overtone series: ~70 Hz, ~140 Hz, ~210 Hz, ~280 Hz, ~350 Hz
- **Vocal tract modulation**: Formant filtering — vowel shapes (OO, AH, EE) create moving bandpass resonances up to ~3 kHz
- **Drone + rhythm**: Rhythmic patterns are produced by stopping the drone momentarily or by rapid vocal tract changes — "tooting" rhythms

### DSP Modeling Approach (JUCE/C++)

**Core synthesis**: Waveguide tube model + formant filter bank.

```cpp
// Closed-open cylindrical waveguide (didgeridoo approximates closed-open)
class DidgeridooVoice {
    // Two delay lines: one for each wave direction (bidirectional waveguide)
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> tubeForward;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> tubeBackward;

    // Lip reed model: non-linear pressure-flow relationship
    float lipReedModel(float mouthPressure, float tubePressure) {
        float deltaPressure = mouthPressure - tubePressure;
        return std::tanh(deltaPressure * 3.0f) * 0.5f + 0.5f;
    }

    // Formant filter bank for vocal tract (3 formants, moving targets)
    // F1: 200-800 Hz (jaw opening)
    // F2: 800-2400 Hz (tongue front-back)
    // F3: 2000-3500 Hz (lip rounding)
    juce::dsp::IIR::Filter<float> formant1, formant2, formant3;
    juce::SmoothedValue<float> f1Target, f2Target, f3Target; // vowel morph
};
```

**Circular breathing simulation**: LFO-modulated gain envelope that never reaches zero. The slight variation in breath pressure during circular breathing creates a subtle ~2–4 Hz amplitude modulation.

**Overtone cycling**: Pre-mapped formant positions for 5 vowel shapes, interpolated via a morph parameter:
```cpp
// Vowel formant targets (Hz): OO, OH, AH, EH, EE
const float F1_VOWELS[] = {300, 450, 700, 600, 300};
const float F2_VOWELS[] = {870, 800, 1200, 1800, 2300};
```

### Key Parameters to Expose
| Parameter | Range | Description |
|---|---|---|
| `didj_drone_pitch` | 50–100 Hz | Root fundamental frequency |
| `didj_breath_pressure` | 0.0–1.0 | Lip buzz intensity — tone fullness |
| `didj_vowel_morph` | 0.0–1.0 | Formant position: OO (0) → EE (1) |
| `didj_overtone_select` | 1–5 | Harmonic partial emphasis |
| `didj_tongue_rhythm` | 0.0–1.0 | Rhythmic gate articulation rate |
| `didj_breath_cycle` | 0.5–4.0 Hz | Circular breathing modulation rate |
| `didj_tube_material` | 0.0–1.0 | Loss coefficient — 0=termite-hollowed (rough, resonant), 1=smooth |

### Coupling Potential with XOmnibus Engines
- **OCEAN DEEP**: Didgeridoo sub-drone as sub-bass oscillator — the OO formant position is functionally a vowel-formant sub-bass
- **OHM**: Didgeridoo overtone cycling + OHM's harmonic jam mode — spectral overtone interactions
- **OVERLAP** (FDN reverb): Didgeridoo in a cave-like FDN produces the traditional performance resonance
- **OBBLIGATO**: Wind coupling axis — didgeridoo as the "sub wind" voice in a wind ensemble layer

### XPN Velocity Mapping
| Velocity range | Breath pressure | Vowel position | Sonic result |
|---|---|---|---|
| 1–40 | Very low | OO (0.0) | Sub-bass drone, nearly inaudible formant |
| 41–80 | Medium | OH→AH (0.2–0.5) | Full drone with opening vowel |
| 81–110 | High | AH→EH (0.5–0.7) | Bright, active formant movement |
| 111–127 | Maximum | EE (1.0) | Full overtone cycling, bright scream |

---

## 03 — Oud (Middle Eastern Short-Neck Lute)

**Collection**: Artwork/Color — Color Quad A | Accent: Ochre `#CC7722`
**Also relevant**: Travel/Water — Historical Set

### Physical Acoustic Mechanism
The oud is a bowl-back lute with a short, fretless neck. 11–13 strings in 5–6 courses (pairs or single). Bridge is "floating" — glued only by string tension to the soundboard. Three rose-shaped sound holes (rather than one round hole like a guitar) distribute vibration differently across the soundboard. The deep pear-shaped bowl back (assembled from dozens of thin wood staves) acts as a resonant cavity with distinctive warmth. No frets means microtonal inflections are idiomatic; maqam scales use quarter-tones. The mizrab (plectrum) is typically made from an eagle feather; modern substitutes are horn or plastic. Attack is sharp and defined; decay is long and resonant.

### Key Timbre Characteristics
- **Attack**: Sharp percussive transient from plectrum strike, fast (< 5ms) initial click
- **Sustain**: Long, warm, decay-dominated — less sustain than a guitar, more resonant body
- **Harmonics**: Warm, with bowl resonance adding low-mid emphasis (~150–400 Hz). Rose holes create diffuse radiation pattern — less direct than a guitar's round hole
- **Double course beating**: Two strings in unison tuned slightly apart produce natural beating/chorusing
- **Microtonal capability**: Portamento between pitches, quarter-tone ornaments

### DSP Modeling Approach (JUCE/C++)

**Core synthesis**: Plucked string model with bowl resonator.

```cpp
// Extended Karplus-Strong with coupled resonator
class OudStringVoice {
    // Excitation: sharp noise burst (plectrum) convolved with plectrum impulse
    // Duration: 0.5–2ms depending on plectrum thickness
    std::array<float, 96> plectrumImpulse; // pre-computed

    // String delay line
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> stringDelay;

    // Loss filter: 1st-order lowpass in the delay loop
    // cutoff determined by decay rate target
    juce::dsp::IIR::Filter<float> loopLossFilter;

    // Bowl resonator: modal synthesis approach
    // 3–5 dominant bowl modes identified by spectral analysis
    // Mode 1: ~180 Hz, Mode 2: ~340 Hz, Mode 3: ~600 Hz
    struct ModalResonator {
        float freq, amplitude, decay;
        float state = 0.0f;
    };
    std::array<ModalResonator, 5> bowlModes;

    // Double course: second string with slight detune
    float detuneAmount = 0.005f; // ~8 cents typical
};
```

**Rose hole diffusion**: The three sound holes create a more diffuse radiation pattern than a single round hole. Model as a slight comb filter (very short delay, ~0.3ms) added to the output:

```cpp
// Rose hole diffusion: 3 holes at slightly different positions
// Creates subtle notches in high frequency response
float roseHoleDiffusion(float input, float* delayBuffer, int& writePos) {
    const int delays[] = {13, 27, 41}; // samples at 44100 Hz (~0.3/0.6/0.9ms)
    float output = input;
    for (int d : delays)
        output += delayBuffer[(writePos - d + 64) & 63] * 0.15f;
    delayBuffer[writePos++ & 63] = input;
    return output * 0.6f;
}
```

### Key Parameters to Expose
| Parameter | Range | Description |
|---|---|---|
| `oud_plectrum_hardness` | 0.0–1.0 | Attack sharpness — 0=soft eagle feather, 1=hard plastic |
| `oud_string_decay` | 0.5–8.0s | Loop loss → sustain length |
| `oud_bowl_resonance` | 0.0–1.0 | Bowl mode coupling depth |
| `oud_double_course` | 0–20 cents | Chorus detune between course strings |
| `oud_portamento` | 0–200ms | Microtonal glide |
| `oud_course_count` | 5–6 | Active string courses |
| `oud_sympathetic` | 0.0–1.0 | Subtle cross-string resonance |

### Coupling Potential with XOmnibus Engines
- **OPAL**: Granular freeze on oud decay tail → lush evolving string pad
- **OUIE** (hammerhead duophonic — when built): Oud + Oud an interval apart, STRIFE↔LOVE axis
- **OVERDUB**: Oud melody through dub tape delay — deeply musical pairing historically (Middle Eastern dub exists)
- **ORPHICA**: Microsound harp coupling — both are plucked chordophones, interesting spectral interaction

### XPN Velocity Mapping
| Velocity range | Plectrum force | Sonic result |
|---|---|---|
| 1–40 | Gentle, near-string | Soft, warm, no transient click |
| 41–80 | Normal playing | Full tone, defined attack |
| 81–110 | Strong stroke | Bright, projecting, slight edge |
| 111–127 | Percussive slam | Heavy attack transient, full bowl ring |

---

## 04 — Guzheng (Chinese Zither)

**Collection**: Artwork/Color — Color Quad A | Accent: Orchid `#DA70D6`

### Physical Acoustic Mechanism
The guzheng is a plucked zither with 21 strings (modern standard) over a long resonant board. Each string is supported by a movable ivory or plastic bridge (yan zhi), allowing the player to adjust tuning. The traditional pentatonic tuning is D-G-A-B-D for each octave span. The right hand plucks with fingernail extensions (義甲, yìjiǎ — plastic or tortoiseshell picks glued to each finger); the left hand presses strings to the left of the bridges to create bent pitches and vibrato. This produces the characteristic wailing, expressive quality. Multiple strings can be plucked simultaneously for rich chords. The soundboard is made of paulownia wood.

### Key Timbre Characteristics
- **Attack**: Sharp, bright pluck — faster decay than piano, slower than harpsichord
- **Sustain**: Moderate — damping from the player's touch is part of technique
- **Harmonics**: Bright, with strong 2nd, 3rd, and 4th harmonics. Bridge position (movable) affects tone
- **Bending**: Left-hand pressure on the string to the left of the bridge raises pitch by a semitone or more — characteristic sliding, wailing effect
- **Tremolo**: Rapid repetitive plucking of a single string — produces a sustained note effect

### DSP Modeling Approach (JUCE/C++)

**Core synthesis**: Multi-string plucked model with per-string pitch bending.

```cpp
// 21-string zither: polyphonic plucked string synthesis
// Each string: independent Karplus-Strong delay + bend modulation
class GuzhenVoicePool {
    static constexpr int NUM_STRINGS = 21;
    struct GuzString {
        juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> delay;
        juce::dsp::IIR::Filter<float> loopFilter;
        juce::SmoothedValue<float> pitchBend; // left-hand pressing
        float velocity;
        bool active = false;
    };
    std::array<GuzString, NUM_STRINGS> strings;

    // Soundboard resonance: paulownia wood — bright, relatively hollow
    // Peak: ~500 Hz, secondary ~1800 Hz
    juce::dsp::IIR::Filter<float> boardResonance1, boardResonance2;
};

// Tremolo: re-trigger same string rapidly (16th-32nd notes)
// Implement as a sub-oscillator that re-excites the delay line
class TremoloEngine {
    float rate = 8.0f;  // Hz, typically 6-14 Hz
    float depth = 0.8f;
    juce::dsp::Oscillator<float> tremoloLFO;
    // On each LFO peak: re-inject pluck excitation at low amplitude
};
```

**Bending model**: Left-hand pitch bend is not a simple pitch shift — it elongates the vibrating string section, which slightly changes string tension. Implement as smooth pitch glide up to +200 cents:

```cpp
// Bend: raise pitch by pressing. Maximum ~2 semitones for most strings
// juce::SmoothedValue interpolates pitch between base and bent position
string.pitchBend.setTargetValue(bendAmount * 200.0f); // cents
```

### Key Parameters to Expose
| Parameter | Range | Description |
|---|---|---|
| `guzheng_pluck_position` | 0.0–1.0 | Bridge proximity — 0=over bridge (bright), 1=far (mellow) |
| `guzheng_bend_depth` | 0–200 cents | Left-hand press maximum bend |
| `guzheng_bend_rate` | 0–500ms | Bend attack speed |
| `guzheng_tremolo_rate` | 4–16 Hz | Rapid repluck tremolo rate |
| `guzheng_tremolo_depth` | 0.0–1.0 | Tremolo intensity |
| `guzheng_decay` | 0.3–5.0s | String decay time |
| `guzheng_board_brightness` | 0.0–1.0 | Paulownia board resonance brightness |
| `guzheng_polyphony` | 1–21 | Simultaneous string voices |

### Coupling Potential with XOmnibus Engines
- **OPAL**: Granular guzheng — scatter pluck events in time → shimmering cloud texture
- **OVERWORLD** (chip synth): Cross-coupling — guzheng + SNES SPC700's soft attack creates an interesting hybrid timbre
- **ODYSSEY** (wavetable): Guzheng waveshaping — single pluck cycle as wavetable source
- **OHM**: Guzheng tremolo strings → harmonically rich drone, OHM's commune mode adds bass foundation

### XPN Velocity Mapping
| Velocity range | Technique | Sonic result |
|---|---|---|
| 1–40 | Light touch, far from bridge | Warm, mellow, minimal attack |
| 41–80 | Normal plucking | Full bright tone, natural decay |
| 81–110 | Firm pluck near bridge | Bright, metallic, longer ring |
| 111–127 | Aggressive pluck + bend | Maximum brightness + pitch wail on release |

---

## 05 — Handpan (Hang Drum)

**Collection**: Travel/Water — Leisure Set (Island Cultural × Island Music)

### Physical Acoustic Mechanism
The handpan (invented by PANArt in 2000 as the Hang) is a convex steel shell instrument. The Ding (top center) is a dome-shaped note field; surrounding it are 7–9 additional tone fields arranged in a circular pattern. Each tone field is tuned to produce a fundamental and (ideally) the octave and fifth as harmonics — a Helmholtz resonator effect where the shell acts as the resonant cavity. The bottom shell (gu) has a central hole (gu port) that controls bass response. Played by hand (fingertips, palms, wrist). The characteristic sound comes from the coupled vibration: plate vibration + air column resonance inside the shell + the specific geometry of the tone fields.

### Key Timbre Characteristics
- **Attack**: Soft, bell-like — a fingertip tap has a ~10ms rise time, palm slower
- **Sustain**: Long, ethereal — 3–8 seconds at low dynamics, shorter when played firmly
- **Harmonics**: Fundamental + octave + perfect fifth as primary partials. Nearly harmonic due to intentional tuning. Very low inharmonicity compared to most struck metal
- **Intermodulation**: When two tone fields are struck simultaneously, their resonances interact through the shared shell — subtle combination tones
- **Ding note**: The central dome has the longest sustain and clearest tone; it is the "home" note

### DSP Modeling Approach (JUCE/C++)

**Core synthesis**: Modal synthesis — the most physically accurate and computationally efficient approach for tone fields.

```cpp
// Modal synthesis for each tone field
// Each field has 3 dominant modes: fundamental, octave, fifth
struct ToneFieldModal {
    struct Mode {
        float frequency;
        float amplitude;
        float decayTime; // seconds
        float phase;
        float state;     // current sample output
    };

    std::array<Mode, 3> modes; // fundamental, octave, fifth

    float process(float excitation) {
        float output = 0.0f;
        for (auto& mode : modes) {
            // Damped sinusoidal resonator
            float decay = std::exp(-1.0f / (mode.decayTime * sampleRate));
            mode.state = mode.state * decay +
                         excitation * mode.amplitude *
                         std::sin(mode.phase);
            mode.phase += juce::MathConstants<float>::twoPi * mode.frequency / sampleRate;
            output += mode.state;
        }
        return output;
    }
};

// Shell coupling: shared air cavity couples all tone fields
// Model as a low-frequency Helmholtz resonator (gu port)
// Resonant frequency: typically 60-120 Hz depending on handpan size
juce::dsp::IIR::Filter<float> shellResonator; // ~80 Hz, high Q
```

**Touch excitation model**: Different touches produce different excitation spectra:
```cpp
// Fingertip: short, bright impulse — rich in high harmonics
// Palm: softer, longer impulse — dominated by fundamental
float generateExcitation(float touchHardness, float touchArea) {
    float duration = juce::jmap(touchArea, 0.0f, 1.0f, 0.5f, 8.0f); // ms
    float brightness = juce::jmap(touchHardness, 0.0f, 1.0f, 0.3f, 1.0f);
    // Generate a short burst with brightness-controlled spectral slope
    return generateImpulse(duration * 0.001f * sampleRate, brightness);
}
```

### Key Parameters to Expose
| Parameter | Range | Description |
|---|---|---|
| `handpan_touch_hardness` | 0.0–1.0 | Fingertip (0) to rim knock (1) |
| `handpan_touch_position` | 0.0–1.0 | Center (0=sweet spot) to edge (1=brighter) |
| `handpan_decay_time` | 1.0–10.0s | Field resonance decay |
| `handpan_shell_coupling` | 0.0–1.0 | Cross-field interaction via shared shell |
| `handpan_gu_port` | 0.0–1.0 | Helmholtz bass resonance amount |
| `handpan_scale` | enum | D Kurd / Integral / Pygmy / etc. |
| `handpan_polyphony` | 1–9 | Simultaneous tone field voices |
| `handpan_damping` | 0.0–1.0 | Palm muting (simulates hand contact) |

### Coupling Potential with XOmnibus Engines
- **OVERLAP** (FDN reverb): Handpan in the Lion's Mane knot topology FDN — transforms the instrument into an infinite sustain ambient pad
- **OPAL** (granular): Handpan grain clouds — the long decay is perfect for granular stretch/freeze
- **ORPHICA** (microsound harp): Both are sustained resonant plate/string instruments — spectral sibling coupling
- **DRIFT**: Handpan with granular drift → water column texture for Aquarium atlas

### XPN Velocity Mapping
| Velocity range | Touch | Sonic result |
|---|---|---|
| 1–40 | Feather-light fingertip, center | Breathy fundamental, minimal overtones |
| 41–80 | Medium fingertip | Full tone, octave audible |
| 81–110 | Firm tap | All 3 harmonics active, slight shell ring |
| 111–127 | Rim knock | Percussive attack, metallic overtones, full shell excitation |

---

## 06 — Angklung (Indonesian/Sundanese Bamboo Rattle)

**Collection**: Travel/Water — Leisure Set (Island Cultural × Island Music)

### Physical Acoustic Mechanism
The angklung is a rattle made of 2–3 bamboo tubes, each cut and shaped to produce a single pitch when shaken. The tubes are suspended in a bamboo frame and slide freely. When shaken, the tubes strike the frame, producing a tone. Crucially: each tube is cut to a different length — typically the same pitch in different octaves (unison and octave, or unison/octave/fifth). The instrument produces exactly one pitch per angklung; an ensemble of angklungs is required for melodic playing. The mechanism is unique: the sound is produced by impact (tube hitting frame), not by plucking or bowing, giving it a percussive-sustain hybrid character.

### Key Timbre Characteristics
- **Attack**: Short impact transient, ~5ms, followed by ringing sustain
- **Sustain**: 0.5–2 seconds — bamboo has higher damping than metal
- **Harmonics**: The two/three tubes in octave/fifth relationship produce a natural chord on a single shake. Slightly inharmonic due to bamboo material properties
- **Rattle quality**: The impact excitation has a "woody click" transient (bamboo-on-bamboo impact noise) followed by the tonal ring
- **Shake rate**: Fast shaking produces tremolo effect; single shakes produce isolated notes

### DSP Modeling Approach (JUCE/C++)

**Core synthesis**: Impact synthesis (modal + noise burst) for the bamboo tube ring.

```cpp
class AngklungVoice {
    // Bamboo tube: two tubes per angklung at octave relationship
    struct BambooTube {
        float fundamentalHz;
        float decayTime;   // ~0.8s for bamboo
        float state = 0.0f;

        float process(float excitation) {
            // Bamboo is slightly inharmonic: stretch tuning factor ~1.02
            float inharmonicFundamental = fundamentalHz * 1.0f; // mild
            float decay = std::exp(-1.0f / (decayTime * sampleRate));
            state = state * decay + excitation;
            return state * std::sin(phase);
            // phase increments at inharmonicFundamental
        }
    };

    BambooTube lowerTube;  // fundamental pitch
    BambooTube upperTube;  // octave above

    // Impact noise: short burst of filtered noise (bamboo hit character)
    // Bandpass centered around 3-6 kHz, duration ~3ms
    juce::dsp::IIR::Filter<float> impactFilter;
    float impactEnvelope = 0.0f;

    // Shake rate: when shake_rate > 0, auto-retrigger at rate Hz
    float shakeRate = 0.0f; // Hz
    float shakePhase = 0.0f;
};
```

**Ensemble mode**: A full angklung gamelan requires 15–20 individual angklungs. In XPN context, map each note in the scale to a different pad:

```cpp
// 10-pad angklung kit: each pad = one angklung pitch
// Pentatonic: C D E G A (5 pitches × 2 octaves = 10 pads)
const float ANGKLUNG_PITCHES_HZ[] = {
    261.63f, 293.66f, 329.63f, 392.00f, 440.00f,  // lower octave
    523.25f, 587.33f, 659.25f, 784.00f, 880.00f    // upper octave
};
```

### Key Parameters to Expose
| Parameter | Range | Description |
|---|---|---|
| `angklung_shake_rate` | 0 (single), 2–15 Hz | Continuous shaking tremolo rate |
| `angklung_tube_count` | 2–3 | Number of tubes (2=unison+octave, 3=adds fifth) |
| `angklung_impact_hardness` | 0.0–1.0 | Impact click level (wooden click vs clean tone) |
| `angklung_bamboo_age` | 0.0–1.0 | 0=fresh bamboo (bright), 1=aged (warmer, darker) |
| `angklung_decay` | 0.3–2.5s | Tube ring decay time |
| `angklung_ensemble_mode` | bool | Spread pitches across 10 pads for melody mode |

### Coupling Potential with XOmnibus Engines
- **OTTONI** (triple brass): Angklung + brass — the attack transient of angklung clicks complements brass's slow attack. Tuned to the same tonic, creates a percussive gamelan-brass hybrid
- **OLE** (Afro-Latin): Angklung's rhythmic shaking pairs with OLE's polyrhythmic Latin framework
- **OVERWORLD**: Chip synth in Phrygian mode + angklung tuning — cross-cultural pixel art sound

### XPN Velocity Mapping
| Velocity range | Shake intensity | Sonic result |
|---|---|---|
| 1–40 | Gentle single shake | Soft click, gentle ring |
| 41–80 | Medium shake | Full impact, sustained ring |
| 81–110 | Fast continuous shaking | Tremolo effect, rich resonance |
| 111–127 | Maximum impact | Heavy click transient, full overtone chord |

---

## 07 — Sitar (North Indian Classical Lute)

**Collection**: Travel/Water — Historical Set (Historical Percussion × Synth Genres)

### Physical Acoustic Mechanism
The sitar has 6–7 main strings (played) and 11–13 sympathetic strings (taraf) that run beneath the main strings. The frets are curved (movable), allowing left-hand meend (bending) over a full tone or more. The jawari bridge is specifically designed to create a controlled buzzing (sawari) — the bridge surface is parabolicly curved, causing the string to graze across it, flattening the vibration envelope and creating a buzzy sustain. This buzz is the defining timbral feature of the sitar. The gourd body (tumba) acts as the resonator; a second smaller gourd may be added at the headstock. The mizrab (metal ring worn on the index finger) plucks the strings.

### Key Timbre Characteristics
- **Attack**: Sharp, immediate, with instant jawari buzz onset
- **Sustain**: Long with continuous jawari buzz throughout — envelope is almost flat for the buzz layer
- **Harmonics**: The jawari creates a rich, dense upper harmonic series from the fundamental through the 20th+ harmonic due to string grazing
- **Sympathetic strings**: Ring continuously whenever the main strings are played, tuned to the notes of the raga — creates a shimmering halo of pitched resonance
- **Meend (bending)**: Characteristic slurs up or down by pulling the string laterally — up to 2 whole tones on the bass strings

### DSP Modeling Approach (JUCE/C++)

**Jawari buzz model**: This is the most distinctive and technically challenging element.

```cpp
// Jawari buzz: simulated by waveshaping the string signal
// The bridge-grazing creates clipping-like distortion that enriches harmonics
// Key: soft waveshaping (not hard clip) that saturates the envelope uniformly
float jawariWaveshaper(float input, float jawariAmount) {
    // Modified tanh saturation — creates buzz without sounding like overdrive
    // jawariAmount: 0.0 = clean (no buzz), 1.0 = full traditional buzz
    float saturation = 1.0f + jawariAmount * 8.0f;
    return std::tanh(input * saturation) / std::tanh(saturation);
}

// Sympathetic strings: 13 resonators tuned to raga scale degrees
// Each resonator is a simple damped oscillator with very long decay (~10s)
// Excited by whatever frequency is closest to their tuning
class SympatheticStringBank {
    static constexpr int N_STRINGS = 13;
    struct SympString {
        float tuningHz;
        float state = 0.0f;
        float decay; // very long, ~8-12s

        float process(float input) {
            // Bandpass around tuningHz, Q=30 (narrow resonance)
            // Input energy near tuningHz excites this string
            float excitation = narrowBandpass(input, tuningHz);
            state = state * decay + excitation * 0.01f;
            return state;
        }
    };
    std::array<SympString, N_STRINGS> strings;
};
```

**Meend (string bending)**: Lateral string pull — implement as smooth pitch glide:
```cpp
// Sitar bending: can reach +400 cents (major third) on bass strings
// Pitch modulation applied to delay line length
juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> meendBend;
meendBend.reset(sampleRate, 0.1f); // 100ms glide default
```

### Key Parameters to Expose
| Parameter | Range | Description |
|---|---|---|
| `sitar_jawari_amount` | 0.0–1.0 | Buzz intensity — 0=clean pluck, 1=full traditional jawari |
| `sitar_jawari_character` | 0.0–1.0 | Buzz texture — 0=smooth, 1=raspy |
| `sitar_sympathetic_level` | 0.0–1.0 | Taraf string resonance in mix |
| `sitar_sympathetic_tuning` | enum | Raga selection: Yaman/Bhairav/Kafi/Todi/Bhimpalasi |
| `sitar_meend_range` | 0–400 cents | Maximum bend depth |
| `sitar_meend_speed` | 10–500ms | Bend glide time |
| `sitar_gourd_resonance` | 0.0–1.0 | Tumba body resonance depth |
| `sitar_plectrum_position` | 0.0–1.0 | Near bridge (bright) to mid-string (warm) |

### Coupling Potential with XOmnibus Engines
- **OPAL** (granular): Granularize the sympathetic string shimmer → evolving pad texture that is physically derived from sitar resonance
- **OVERDUB** (tape delay + spring reverb): Classic psychedelic sitar sound — George Harrison era. The jawari buzz through spring reverb is extremely musical
- **DRIFT**: Sitar with granular drift → Indian classical meets ambient drone
- **OPENSKY** (when built): Pure feliX mode — sitar sympathetic strings through OPENSKY's shimmer = transcendent texture

### XPN Velocity Mapping
| Velocity range | Mizrab force | Sonic result |
|---|---|---|
| 1–40 | Feather touch | Delicate, sympathetics dominant, barely any buzz |
| 41–80 | Normal plucking | Full jawari buzz, sympathetics ringing |
| 81–110 | Strong stroke | Bright, projecting, full harmonic density |
| 111–127 | Maximum force | Attack transient + full buzz + all sympathetics activated |

---

## 08 — Shamisen (Japanese 3-String Lute)

**Collection**: Travel/Water — Historical Set

### Physical Acoustic Mechanism
The shamisen has three silk strings stretched over a nearly square frame resonator covered with cat, dog, or synthetic skin on both top and bottom (both faces are resonating membranes, unlike a guitar). The neck is unfretted and very long relative to the body. The most distinctive feature is **sawari** (障り): a small groove cut into the nut of the first string (or the bridge) causes the string to buzz when played. This controlled buzz (analogous to sitar jawari, but higher and brighter) is an intentional feature of the sound — not a defect. The bachi (plectrum) is large, fan-shaped, and struck against the skin as much as the string, creating a "skin hit" attack sound distinct from a pure string pluck.

### Key Timbre Characteristics
- **Attack**: Dual transient — (1) bachi-on-skin hit (thwack, 2–5ms) + (2) string pluck (10–15ms rise)
- **Sustain**: Medium-short — skin membrane damps more quickly than wood soundboard
- **Harmonics**: Bright, cutting. Sawari enriches upper harmonics, similar to sitar jawari but with a different spectral character — more metallic, less full
- **Skin resonance**: Both membrane faces vibrate; the combination creates a complex radiation pattern with strong fundamentals and a distinctive hollow mid-frequency dip
- **Range**: Shamisen has three main tunings: honchoshi (standard), niagari (raised 2nd string), sansagari (lowered 3rd string)

### DSP Modeling Approach (JUCE/C++)

**Core synthesis**: Dual excitation (skin impact + string pluck) + sawari waveshaping.

```cpp
class ShamisenVoice {
    // Excitation 1: bachi-skin impact
    // Short noise burst, filtered to ~500-2000 Hz (skin thwack character)
    // Duration: 3ms, amplitude proportional to velocity
    juce::dsp::IIR::Filter<float> skinImpactFilter; // bandpass 500-2000Hz

    // Excitation 2: string pluck
    // Karplus-Strong delay with loop filter
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> stringDelay;
    juce::dsp::IIR::Filter<float> loopFilter;

    // Sawari model: identical mechanism to sitar jawari
    // But character differs: shamisen sawari is brighter, more metallic
    float sawariWaveshaper(float input, float amount) {
        // Use asymmetric saturation: positive half clips harder
        // This produces the metallic, buzzy character of shamisen sawari
        if (input > 0.0f)
            return std::min(input, amount * 2.0f) + (input - amount * 2.0f) *
                   (1.0f - amount) * 0.3f; // soft clip
        else
            return input * (1.0f - amount * 0.2f); // gentle reduction
    }

    // Skin membrane: both faces — model as two coupled resonators
    // Front membrane: ~300 Hz primary resonance (skin over wooden frame)
    // Back membrane: slightly detuned, ~280 Hz → beating effect
    juce::dsp::IIR::Filter<float> frontMembrane, backMembrane;

    // Mix: skin impact (30%) + string pluck (70%) at default settings
    float skinMixAmount = 0.3f;
};
```

**Tuning system**: Three standard tunings affect all string intervals:
```cpp
enum class ShamisenTuning {
    Honchoshi,  // D  G  D  (unison, perfect fourth, octave)
    Niagari,    // D  A  D  (unison, perfect fifth, octave)
    Sansagari   // D  G  C  (unison, perfect fourth, minor seventh)
};
```

### Key Parameters to Expose
| Parameter | Range | Description |
|---|---|---|
| `shamisen_sawari_amount` | 0.0–1.0 | Buzz intensity — 0=clean, 1=full traditional sawari |
| `shamisen_bachi_weight` | 0.0–1.0 | Bachi impact on skin — skin hit vs pure pluck |
| `shamisen_skin_tension` | 0.0–1.0 | Membrane tightness — 0=loose/deep, 1=tight/bright |
| `shamisen_tuning` | enum | Honchoshi / Niagari / Sansagari |
| `shamisen_string_decay` | 0.2–3.0s | String sustain length |
| `shamisen_both_membranes` | 0.0–1.0 | Back membrane coupling (adds hollow character) |
| `shamisen_silk_texture` | 0.0–1.0 | Silk string roughness (adds subtle noise floor) |
| `shamisen_neck_size` | enum | Hosozao (thin) / Chuzao (medium) / Futozao (thick) |

### Coupling Potential with XOmnibus Engines
- **ONSET**: Shamisen bachi-skin hit as a percussion layer — the skin impact transient is genuinely percussive and sits well in a drum kit
- **OLE** (Afro-Latin): Shamisen + OLE's rhythm section — Japanese folk + Afro-Cuban creates a collision texture that is unexpected and musical
- **OVERWORLD**: NES/Famicom + shamisen — historical Japanese aesthetic alignment, chip music + traditional instrument
- **OBBLIGATO** (dual wind): Shamisen as melodic voice over OBBLIGATO's sustained wind texture — creates a folk-like composition layer

---

## Cross-Instrument DSP Notes

### Shared Components (Reusable Across All 8 Engines)

**1. Inharmonic string model** (Erhu, Oud, Guzheng, Sitar, Shamisen):
All five share the same extended Karplus-Strong pattern. Recommend a shared `PluckedStringVoice` base class in XOmnibus `Source/Engines/Shared/`:

```cpp
// Source/Engines/Shared/PluckedStringVoice.h
class PluckedStringVoice {
protected:
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> stringDelay;
    juce::dsp::IIR::Filter<float> loopFilter;
    juce::SmoothedValue<float> pitchGlide;
    juce::SmoothedValue<float> pitchBend;
    float loopDecay;

    virtual float excite(float velocity, float hardness) = 0; // subclass provides plectrum impulse
    virtual float processLoop(float input) { return loopFilter.processSample(input * loopDecay); }
};
```

**2. Jawari/sawari waveshaper** (Sitar, Shamisen — and applicable to Erhu's bow scratch):
Shared waveshaper function. Difference: sitar uses symmetric tanh, shamisen uses asymmetric positive-clip.

**3. Sympathetic resonator bank** (Sitar, Erhu's secondary string):
Shared `SympatheticBank` class: N narrow bandpass IIR filters, each representing one sympathetic string. Input = primary string output, fed into all bandpass filters simultaneously.

**4. Modal synthesis kernel** (Handpan, Angklung):
Both instruments are well-modeled by modal synthesis. Shared `ModalVoice` struct with configurable mode count, frequencies, amplitudes, and decay times.

### Build Priority Recommendation
Given collection phase targets, recommended implementation order:

1. **Handpan** — Modal synthesis only, no complex bow/buzz physics. Fastest to build correctly.
2. **Angklung** — Simple impact + ring, ensemble pad mode is a single parameter.
3. **Oud** — Karplus-Strong with bowl resonator, no unusual DSP.
4. **Guzheng** — Same base as Oud, adds bend model.
5. **Shamisen** — Dual excitation + sawari, uses shared components from above.
6. **Sitar** — Adds sympathetic bank (complex) + jawari + meend.
7. **Erhu** — Bow physics is the hardest real-time physical model here.
8. **Didgeridoo** — Waveguide + formant bank, depends on waveguide infrastructure.

---

*Document prepared for XO_OX Designs — Artwork/Color and Travel/Water collection pre-production. Build implementation to begin after OVERLAP and OUTWIT seances complete.*
