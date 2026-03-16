# World Instrument DSP Research — Collections Preparation

**Date**: 2026-03-16
**Project**: XO_OX Designs — XOmnibus Engine Design
**Target Collections**: Artwork/Color Collection, Travel/Water Collection
**Status**: Research — feeding directly into engine design briefs

---

## Overview

Eight world instruments have been selected as the voice cores for engines in the Artwork/Color and Travel/Water collections. This document establishes the physical acoustic mechanism of each instrument, identifies the key timbre characteristics that define its identity, proposes a JUCE/C++ DSP modeling approach, defines the critical parameters to expose, assesses coupling potential with existing XOmnibus engines, and specifies the XPN velocity mapping strategy for drum kit export.

All DSP must satisfy the 6 XOmnibus Doctrines. D001 (velocity shapes timbre) and D003 (physics IS the synthesis) are the most demanding constraints for physical modeling work.

---

## 1. Erhu

### Physical Acoustic Mechanism
The erhu is a two-string Chinese bowed fiddle. Strings are silk or metal-wound silk, tuned a fifth apart (D4–A4). The resonator is a small hexagonal or octagonal wooden tube covered at one end with python skin, which acts as a membrane radiator rather than a cavity resonator. Unlike Western bowed instruments, the bow hair is threaded between the two strings — the player cannot separate bow contact from the strings by lifting. Bow pressure and bow speed jointly determine tone, with very light pressure producing harmonics and heavy pressure producing a rough, nasal fundamental. There is no fingerboard; the left hand stops strings by pressing from the side, enabling microtonal inflection and portamento.

### Key Timbre Characteristics
- Nasal, bright, emotionally direct fundamental
- Extremely wide dynamic range from near-silence to penetrating projection
- Continuous pitch inflection — portamento is idiomatic, not ornamental
- Python skin membrane adds a subtle, dry buzz overtone beneath the main tone
- Harmonics readily available with light bow touch — ethereal, flute-like flageolet tones
- Vibrato is wide and slow compared to Western violin; often delayed or omitted on short notes

### DSP Modeling Approach (JUCE/C++)
**Bowing physics:** Use a simplified Helman-McIntyre bow model. A sawtooth-like Helmholtz motion waveform is generated whose duty cycle and harmonic content are controlled by two parameters: `bow_pressure` (0–1) and `bow_speed` (0–1). At low pressure, the waveform approaches a sine wave with prominent second harmonic. At high pressure, it shifts toward a clipped, rough waveform with strong odd harmonics.

```cpp
// Simplified bow model — frequency-domain shaping
float computeBowedTimbre(float pressure, float speed, float fundamentalHz, float sampleRate) {
    // Helmholtz motion spectral rolloff: pressure controls slope
    float rolloff = 1.0f - (0.7f * pressure);  // 0.3 to 1.0
    // Generate harmonics with rolloff
    float out = 0.0f;
    for (int k = 1; k <= 16; ++k) {
        float amp = std::pow(rolloff, k - 1) / k;
        out += amp * std::sin(2.0f * juce::MathConstants<float>::pi * k * fundamentalHz * phase);
    }
    return out * speed;
}
```

**Python skin resonator:** A `BiquadFilterNode` bandpass centered around 2.5–3.5 kHz with moderate Q (2–4) adds the characteristic membrane buzz. Modulate the bandpass center frequency slightly with a slow LFO (0.3–0.8 Hz) to simulate the slight acoustic inconsistency of natural skin.

**Portamento:** Glide time parameter with exponential curve (matches finger slide physics). Rate: 0–200 ms per semitone.

**String resonance:** A 2-voice unison with very slight detuning (0–3 cents) models the two-string sympathetic relationship.

### Key Parameters to Expose
| Parameter | ID | Range | Notes |
|---|---|---|---|
| Bow Pressure | `erhu_bowPressure` | 0–1 | Core timbre control — D001 velocity target |
| Bow Speed | `erhu_bowSpeed` | 0–1 | Maps to loudness + brightness |
| Skin Resonance | `erhu_skinResonance` | 0–1 | Membrane buzz intensity |
| Portamento Time | `erhu_portamento` | 0–500 ms | Exponential glide |
| Vibrato Depth | `erhu_vibratoDepth` | 0–100 cents | Wide, slow by default |
| Vibrato Rate | `erhu_vibratoRate` | 0.5–8 Hz | Default ~4 Hz |
| Harmonic Emphasis | `erhu_harmonicShift` | 0–1 | 0 = rich fundamental, 1 = flageolet |
| String Detuning | `erhu_unison` | 0–12 cents | Sympathetic string thickness |

### Coupling Potential
- **OVERDUB** (dub_): Erhu's nasal fundamental feeds beautifully into spring reverb. The tape delay with moderate feedback creates the sense of a long echo in a stone chamber.
- **OPAL** (opal_): Grain size tuned to 40–80 ms captures individual bow strokes and stretches them into evolving pads — transforms erhu into atmospheric texture.
- **ORGANON** (organon_): Metabolic rate coupling maps Erhu's expression dynamics to Organon's synthesis metabolism, producing biological-feeling phrase evolution.
- **DRIFT** (drift_): Era crossfade between traditional silk-string timbre and modern metal-string erhu textures.

### XPN Velocity Mapping Strategy
- Velocity 1–40: Light bow, harmonics dominant, near-silence ghost notes
- Velocity 41–80: Normal bowing, nasal fundamental present, moderate skin resonance
- Velocity 81–110: Heavy pressure, rough overtones, maximum projection
- Velocity 111–127: Forced harmonics, string pressure peak, slight bow crunch
- Velocity scales `erhu_bowPressure` and `erhu_bowSpeed` simultaneously — velocity IS the bow stroke.

---

## 2. Didgeridoo

### Physical Acoustic Mechanism
The didgeridoo is an Australian Aboriginal aerophone: a naturally hollowed eucalyptus branch, 1–3 meters long, played with circular breathing. The fundamental pitch is determined by tube length (typically A1–G2 range). The player's lips buzz at the mouthpiece; the vocal tract acts as a secondary resonator. Circular breathing (inhaling through the nose while maintaining cheek air pressure) enables continuous drone. Overblowing produces a toot — one octave plus a fifth above the fundamental (the twelfth harmonic dominant in practice). Overtone cycling: the player shapes mouth/throat cavities to emphasize different harmonics during the drone, producing the characteristic wah-wah rhythmic pattern.

### Key Timbre Characteristics
- Continuous drone with rhythmic overblown accents and vocal tract formants
- Fundamental is enormous, sub-bass, saturated — almost synthesizer-like in its density
- Overtone cycling creates formant glides (wah effects) from ~200 Hz to ~2 kHz
- "Didge" texture: a combination of the fundamental buzz, lip flutter artifacts, and tube resonance
- Circular breathing transitions are nearly imperceptible in experienced players
- Breath pressure variations create amplitude modulation at 0.5–4 Hz

### DSP Modeling Approach (JUCE/C++)
**Tube resonance:** Model as a cylindrical waveguide using digital waveguide synthesis (Karplus-Strong extended). A delay line of length `sampleRate / fundamentalHz` samples with a one-pole lowpass loop filter simulates the tube. Loop filter coefficient controls brightness (`g = 0.97` for standard didgeridoo resonance).

```cpp
class DidgeridooWaveguide {
    juce::AudioBuffer<float> delayLine;
    float feedbackCoeff = 0.97f;  // tube energy loss
    float loopFilterState = 0.0f;

    float process(float excitation) {
        float output = delayLine.getSample(0, readPos);
        // One-pole loop filter
        loopFilterState = output * (1.0f - feedbackCoeff) + loopFilterState * feedbackCoeff;
        delayLine.setSample(0, writePos, excitation + loopFilterState * feedbackCoeff);
        return output;
    }
};
```

**Vocal tract formant shaping:** Two parallel formant filters (IIR bandpass, JUCE `IIRFilter`). Formant 1: 250–800 Hz (lip/throat cavity). Formant 2: 1200–2400 Hz (nasal cavity). Animate both with LFOs of different rates to create the characteristic overtone cycling. User parameter `overtoneCycle` controls the LFO depth; `cycling_rate` controls speed.

**Overblown toot:** A separate oscillator at fundamental × 3.0 (twelfth) triggered by a velocity threshold or explicit accent pad. ADSR with very fast attack (2 ms), 80 ms decay.

**Lip buzz excitation:** Low-frequency sawtooth (~60–120 Hz) mixed with filtered noise, scaled by breath pressure parameter.

### Key Parameters to Expose
| Parameter | ID | Range | Notes |
|---|---|---|---|
| Tube Length | `didge_tubeLength` | 1.0–3.0 m | Sets fundamental pitch via waveguide |
| Breath Pressure | `didge_breathPressure` | 0–1 | Core loudness + buzz density |
| Overtone Cycle Depth | `didge_overtoneCycle` | 0–1 | Wah depth — formant LFO amplitude |
| Cycling Rate | `didge_cyclingRate` | 0.5–8 Hz | Overtone LFO speed |
| Lip Buzz | `didge_lipBuzz` | 0–1 | Flutter artifact intensity |
| Toot Threshold | `didge_tootVelocity` | 0–127 | Velocity above which overblown toot fires |
| Formant 1 Center | `didge_formant1` | 200–900 Hz | Throat cavity vowel |
| Formant 2 Center | `didge_formant2` | 1000–2500 Hz | Nasal vowel peak |

### Coupling Potential
- **OBESE** (fat_): Saturation drive on didgeridoo output creates a wall-of-bass effect. The Mojo Control axis (analog warmth) is ideal for coloring the drone.
- **OUROBOROS** (ouro_): Feedback topology routing — didgeridoo drone as the source signal in a chaotic attractor creates evolving sub-bass mutation.
- **OCEANIC** (ocean_): The chromatophore modulator coupled to `didge_overtoneCycle` creates breathing ocean-floor textures.
- **OWLFISH** (owl_): Mixtur-Trautonium octave mixtures applied to the didgeridoo fundamental creates radical sub-octave layering.

### XPN Velocity Mapping Strategy
- Velocity 1–30: Soft circular breath, fundamental only, minimal buzz
- Velocity 31–70: Normal drone with overtone cycling active
- Velocity 71–100: Heavy breath, formant cycling at maximum depth, audible lip flutter
- Velocity 101–127: Overblown toot fires, pitch jumps to twelfth overtone
- Velocity is mapped to `didge_breathPressure`; the toot threshold fires a secondary layer above velocity 101.

---

## 3. Oud

### Physical Acoustic Mechanism
The oud is a fretless, short-neck lute from the Middle East, North Africa, and Turkey. It has 5 or 6 courses (pairs of strings, with the top course often single), tuned in fourths (Turkish: C2–F2–B2b–E3b–A3b–D4). The body is a deep bowl resonator constructed from thin strips of bent wood, with one or more rose sound holes cut into the top plate. The top plate (cedar or spruce) is the primary acoustic radiator. The floating bridge transmits string vibration directly to the top. The absence of frets enables microtonal maqam (modal) scales — string bending is idiomatic. The plectrum (risha) is a flexible eagle feather or plastic strip, producing a bright, fast attack with immediate decay.

### Key Timbre Characteristics
- Fast, bright transient attack with moderate sustain and clean decay
- Deep bowl resonance adds a warm, rounded low-mid character (300–600 Hz)
- Rose sound hole pattern creates subtle diffraction coloration in upper mids
- Microtonal inflection — quarter-tone bends are idiomatic in maqam performance
- Double-course strings add chorus-like warmth from slight detuning between pairs
- Plectrum noise: a subtle percussive click at note onset from risha contact

### DSP Modeling Approach (JUCE/C++)
**String excitation:** Karplus-Strong with a noise burst initial excitation. Decay time controlled by a low-pass loop filter coefficient tuned to oud's characteristic 2–4 second sustain at mid-pitch. Plectrum click: a 2 ms noise burst at velocity-scaled amplitude, filtered to 2–8 kHz, mixed into the excitation.

```cpp
// Karplus-Strong with plectrum click
float excite(float velocity, float fundamentalHz, float sampleRate) {
    // Noise burst
    float noiseBurst = (juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f);
    // Plectrum click (high-passed noise)
    float clickAmp = velocity * 0.3f;
    return noiseBurst + (clickAmp * highPassClick(noiseBurst));
}
```

**Bowl resonance:** A series of 3–4 BiQuad resonant bandpass filters (modes of the bowl cavity) centered at approximately 280 Hz, 520 Hz, 900 Hz, and 1600 Hz, with Q = 8–15. These are summed with the direct string signal at about –12 dB relative to add warmth without muddying the attack.

**Double-course chorus:** A 2-voice unison with independent pitch LFOs (rate: 0.1–1 Hz, depth: 0–8 cents per voice) creates the warmth of paired strings.

**Microtonal pitch:** Pitch parameter has 1-cent resolution. Pitch bend range set to ±200 cents by default to allow maqam quarter-tones. `oud_microtonalBend` provides a dedicated ±100 cent offset.

### Key Parameters to Expose
| Parameter | ID | Range | Notes |
|---|---|---|---|
| Plectrum Brightness | `oud_plectrumBrightness` | 0–1 | Loop filter coefficient — sustain vs. bright |
| Bowl Resonance | `oud_bowlResonance` | 0–1 | Bowl filter mix level |
| Course Detuning | `oud_courseDetune` | 0–15 cents | Double-course width |
| Microtonal Bend | `oud_microtonalBend` | –100 to +100 cents | Maqam inflection |
| Plectrum Click | `oud_plectrumClick` | 0–1 | Risha attack artifact |
| Decay Time | `oud_decayTime` | 0.5–6 s | String sustain |
| Bowl Mode Tuning | `oud_bowlMode` | 0–1 | Shifts bowl resonance center frequencies |
| String Course Count | `oud_courses` | 5–6 | 5-course vs. 6-course tuning |

### Coupling Potential
- **ORACLE** (oracle_): GENDY stochastic synthesis plus maqam scale system — the oud's microtonal architecture and Oracle's scale system are natural partners. MAQAM_HIJAZ, MAQAM_RAST applied to oud excitation.
- **OUROBOROS** (ouro_): Feedback routing with oud as source produces extended string resonance that takes on topological character.
- **OPAL** (opal_): Granular processing of oud attacks creates shimmering, atomized plucks — highly characteristic textural preset category.
- **OSPREY** (osprey_): ShoreSystem cultural data (Mediterranean/MENA coastlines) pairs naturally with oud timbre and maqam scales.

### XPN Velocity Mapping Strategy
- Velocity 1–35: Soft fingertip touch, minimal click, warm fundamental
- Velocity 36–75: Normal risha strike, full bowl resonance, clear double-course chorus
- Velocity 76–105: Heavy plectrum attack, maximum click artifact, bright upper harmonics
- Velocity 106–127: Aggressive attack, slight string buzz, distortion artifacts at bowl resonance peaks
- Velocity scales `oud_plectrumBrightness` inversely (soft = warm, loud = bright) — critical for D001 compliance.

---

## 4. Guzheng

### Physical Acoustic Mechanism
The guzheng is a Chinese zither with 21 movable bridges and strings tuned to a pentatonic scale. Strings are steel-core with nylon or silk winding, tuned across a 4-octave range. Movable bridges (yima) allow retuning to different modes by repositioning. The player wears finger picks (yijia — plastic nails) on the right hand for plucking, while the left hand performs bending technique (pressing the string to the left of the bridge, raising pitch by 1–3 semitones). The soundboard is paulownia wood. The instrument's natural reverb and long sustain are defining characteristics. Glissando (sweep across multiple strings) is idiomatic.

### Key Timbre Characteristics
- Bright, bell-like attack with long, clear sustain
- Left-hand bending: pitch rises then releases — the characteristic ornament of guzheng performance
- Glissando sweep: multiple string pitches in rapid arpeggio
- Harmonics readily available (light touch at 1/2 or 1/4 string length)
- Steel strings give a brighter, more metallic quality than silk strings
- Natural reverb from the large soundboard creates a hall-like decay

### DSP Modeling Approach (JUCE/C++)
**String model:** Karplus-Strong with a longer delay line than the oud (guzheng has more sustain). Loop filter coefficient: ~0.995 for the characteristic long decay. Initial excitation: bright filtered noise burst, 3–5 ms duration.

**Bending technique:** An envelope-driven pitch modulation post-excitation. The bend envelope rises to +150 cents over 60–150 ms (left-hand press), holds briefly, then releases back to 0. This is independent of standard pitch bend — triggered by a dedicated MIDI CC or a secondary hit on the same pad.

```cpp
// Left-hand bend envelope
void triggerBend(float semitones, float attackMs, float releaseMs) {
    bendTarget = semitones * 100.0f;  // convert to cents
    bendAttackSamples = (attackMs / 1000.0f) * sampleRate;
    bendReleaseSamples = (releaseMs / 1000.0f) * sampleRate;
    bendPhase = BendPhase::Attack;
}
```

**Soundboard reverb:** A short convolution reverb (or algorithmic plate reverb with `roomSize` = 0.3, `dampening` = 0.6) applied post-string at –18 dB creates the natural hall bloom without overwhelming the attack.

**Glissando:** A polyphonic trigger sequence plays 8–21 notes in rapid succession (timing: 15–40 ms apart), ascending or descending, all using the same string model.

**Harmonics mode:** When `guzheng_harmonicTouch` > 0.5, the loop filter is opened and the fundamental is attenuated, emphasizing overtones — produces the clear, glassy harmonic tone.

### Key Parameters to Expose
| Parameter | ID | Range | Notes |
|---|---|---|---|
| Sustain Length | `guzheng_sustain` | 1–12 s | Loop filter coefficient control |
| Bend Depth | `guzheng_bendDepth` | 0–300 cents | Left-hand press range |
| Bend Attack | `guzheng_bendAttack` | 20–300 ms | Left-hand press speed |
| Glissando Speed | `guzheng_glissSpeed` | 10–80 ms | Time between notes in sweep |
| Soundboard Bloom | `guzheng_boardBloom` | 0–1 | Plate reverb send amount |
| Harmonic Touch | `guzheng_harmonicTouch` | 0–1 | Flageolet mode intensity |
| String Brightness | `guzheng_brightness` | 0–1 | Loop filter opening — steel vs. silk |
| Pick Attack | `guzheng_pickAttack` | 0–1 | Initial excitation sharpness |

### Coupling Potential
- **OPAL** (opal_): Granular guzheng creates cascading, shimmering glissandi frozen in time — one of the most natural couplings in the entire XOmnibus fleet.
- **DRIFT** (drift_): Era crossfade between traditional silk-string guzheng and modern steel-string electric guzheng textures.
- **OBLIQUE** (oblq_): Prism color cycling applied to guzheng harmonics creates psychedelic arpeggiated patterns.
- **ORGANON** (organon_): Metabolic coupling routes the guzheng's bend envelope as a modulation source into Organon's synthesis rate.

### XPN Velocity Mapping Strategy
- Velocity 1–30: Fingertip gentle pluck, long decay, bell-like
- Velocity 31–65: Normal pick attack, full sustain, soundboard bloom active
- Velocity 66–95: Heavy pick, maximum brightness, slight pick noise
- Velocity 96–127: Aggressive attack triggers automatic left-hand bend — the iconic bending ornament fires automatically at high velocity
- Velocity scales `guzheng_brightness` and `guzheng_pickAttack`. The automatic bend trigger at velocity 96+ is the key D001 timbre-from-velocity feature.

---

## 5. Handpan

### Physical Acoustic Mechanism
The handpan (PANArt Hang and subsequent makers) is a convex steel idiophone: two hemispherical steel shells are glued together. The top shell has a central dome (ding, the lowest note) and 7–9 tone fields hammered into the surface. Each tone field is tuned to produce a fundamental, an octave, and a compound fifth (partial at 3× the fundamental). These three partials lock in a Helmholtz resonance — the enclosed air chamber reinforces the octave and fifth simultaneously, producing an unusually pure, bell-like sound with fast attack and 4–8 second sustain. Played with fingers and palm; heavier strikes emphasize higher partials.

### Key Timbre Characteristics
- Bell-like clarity with simultaneous fundamental + octave + fifth partials
- Extremely strong Helmholtz resonance — the body cavity provides a unique bloom
- Metallic shimmer in upper partials (3rd–6th harmonic) from steel shell vibration
- Natural compression: louder strikes produce more upper partials but the fundamental barely grows — the instrument has an inherently compressed dynamic range
- Wet-under-hands muting: palm contact after strike creates immediate mute
- Overtone ring: notes ring sympathetically off adjacent tone fields

### DSP Modeling Approach (JUCE/C++)
**Additive synthesis with partial locking:** Three sine oscillators per voice (fundamental, octave, compound fifth) with amplitude ratios approximately 1.0 : 0.6 : 0.35. Each has an independent ADSR. Fundamental: slow attack (5 ms), long decay. Octave: faster attack (2 ms), shorter decay. Fifth: fastest attack (1 ms), shortest decay — this creates the characteristic "bling" then "bloom" envelope shape.

```cpp
struct HandpanVoice {
    juce::ADSR fund, octave, fifth;
    float fundFreq, octaveFreq, fifthFreq;  // ratio 1:2:3

    float process() {
        return fund.getNextSample() * std::sin(fundPhase)
             + octave.getNextSample() * 0.6f * std::sin(octavePhase)
             + fifth.getNextSample() * 0.35f * std::sin(fifthPhase);
    }
};
```

**Helmholtz chamber resonance:** A bandpass filter centered at ~1.2× the fundamental (the enclosed air body resonance) with Q = 8, applied to all voices in the bank simultaneously. This creates the characteristic bloom that makes all notes on the instrument feel "connected."

**Steel shell shimmer:** Filtered noise burst (bandpass 4–8 kHz) at note onset, 30 ms duration, scaled by strike velocity. Gives the metallic "ping" quality.

**Sympathetic ring:** Each active voice adds a small amount of its fundamental back into adjacent pitch neighbors via a shared resonance bus, implementing cross-string coupling.

### Key Parameters to Expose
| Parameter | ID | Range | Notes |
|---|---|---|---|
| Partial Balance | `hpan_partialBalance` | 0–1 | 0 = pure fundamental, 1 = partial-rich |
| Chamber Resonance | `hpan_chamberResonance` | 0–1 | Helmholtz bloom intensity |
| Steel Shimmer | `hpan_shimmer` | 0–1 | Metallic onset noise |
| Decay Time | `hpan_decayTime` | 2–10 s | Ring duration |
| Mute Speed | `hpan_muteSpeed` | 0–1 | Palm mute release time (fast = staccato) |
| Sympathetic Ring | `hpan_sympatheticRing` | 0–1 | Cross-field resonance coupling |
| Shell Tuning | `hpan_shellTuning` | 0–1 | Ding vs. tone field emphasis |
| Strike Character | `hpan_strikeCharacter` | 0–1 | Fingertip vs. palm center spectrum |

### Coupling Potential
- **OPAL** (opal_): Granular freezing of handpan decays creates shimmering, infinite sustain pads — one of the most marketable XOmnibus preset types.
- **OVERDUB** (dub_): Spring reverb applied to handpan bell tones creates a deeply resonant stone cave character.
- **ORBITAL** (orb_): Group envelope system applied to handpan — the "orbital" bloom shape matches handpan's natural partial timing perfectly.
- **ORPHICA** (orph_): Microsound harp coupled with handpan creates a layered metallic plucked soundscape.

### XPN Velocity Mapping Strategy
- Velocity 1–25: Fingertip light tap, fundamental only, very long decay
- Velocity 26–60: Normal finger strike, all three partials present, full chamber bloom
- Velocity 61–90: Firm palm strike, maximum shimmer, sympathetic ring active
- Velocity 91–127: Heavy strike, upper partials dominate, steel shimmer at peak, brief fundamental compression
- Velocity scales `hpan_partialBalance` and `hpan_shimmer`. Note: the fundamental barely scales — this matches the instrument's natural compression curve and is intentional for D001 compliance (timbre changes, not just volume).

---

## 6. Angklung

### Physical Acoustic Mechanism
The angklung is a West Javanese (Sundanese) bamboo instrument. Each angklung unit consists of two or three bamboo tubes tuned to the same pitch class but different octaves (unison, octave, or unison-plus-fifth). The tubes are suspended in a bamboo frame; shaking the frame causes the tubes to knock against a cross-bar, producing a short, bright tone. The rattling produces a characteristic "shake burst" envelope: very fast attack, immediate decay, followed by secondary smaller hits as tubes continue to rattle. In ensemble performance, each player holds one angklung and shakes on cue — the group collectively plays melodies.

### Key Timbre Characteristics
- Very fast attack, immediately decaying pitched transient
- Multiple tubes per unit create a narrow cluster chord (unison + octave or fifth) on every hit
- Bamboo material: bright, woody, with an upper partial cluster around 2–4 kHz
- Shake burst: the main hit plus 2–4 secondary smaller hits at ~50–100 ms spacing
- No sustain — the instrument is inherently percussive-pitched
- Ensemble context: 15–20 players create a staccato melodic texture that no single player can produce alone

### DSP Modeling Approach (JUCE/C++)
**Cluster tone:** Three sine oscillators at the fundamental, 2× (octave), and 3× (fifth). Amplitude ratios: 1.0 : 0.45 : 0.25. Each has an extremely short ADSR (attack: 1 ms, decay: 80–200 ms, sustain: 0, release: 30 ms). No sustain phase.

**Shake burst:** A burst sequencer fires the main hit plus 2–4 additional hits at 40–120 ms intervals, each at 60% of the previous velocity. This is implemented as a scheduled trigger queue, not a physical model.

```cpp
void triggerShakeBurst(float velocity) {
    schedule(0.0f, velocity);
    schedule(55e-3f, velocity * 0.6f);
    schedule(110e-3f, velocity * 0.36f);
    schedule(165e-3f, velocity * 0.22f);  // optional 4th hit
}
```

**Bamboo body resonance:** A comb filter approximation using a short delay (2–5 ms feedback loop) with `feedbackCoeff = 0.3` emphasizes the woody, hollow formant around 1.5–2.5 kHz. One `IIRFilter` bandpass per voice, centered at ~2.2 kHz, Q = 3.

**Ensemble mode:** When `angklung_ensembleDepth > 0`, additional ghost voices at ±1 and ±2 semitones from the fundamental are triggered at reduced velocity (30–50%), simulating adjacent ensemble players slightly early or late.

### Key Parameters to Expose
| Parameter | ID | Range | Notes |
|---|---|---|---|
| Shake Burst Count | `angklung_burstCount` | 1–4 | Number of secondary hits per shake |
| Burst Spacing | `angklung_burstSpacing` | 30–150 ms | Time between secondary hits |
| Bamboo Body | `angklung_bambooBody` | 0–1 | Comb filter woody resonance |
| Tube Cluster Width | `angklung_clusterWidth` | 0–1 | Unison, octave+unison, octave+fifth |
| Decay Time | `angklung_decayTime` | 40–300 ms | How long each tube rings |
| Ensemble Depth | `angklung_ensembleDepth` | 0–1 | Ghost voices simulating full ensemble |
| Pitch Spread | `angklung_pitchSpread` | 0–15 cents | Detuning between tubes |
| Brightness | `angklung_brightness` | 0–1 | Upper partial emphasis |

### Coupling Potential
- **ONSET** (perc_): Drum machine integration — angklung rhythmic patterns layered with ONSET's percussion synthesis creates a full Javanese gamelan-influenced groove.
- **OBLONG** (bob_): The XOblong character synth's aggressive filter attack complements angklung's sharp transient burst.
- **OSTERIA** (osteria_): Shore system cultural data from Island/Southeast Asia coastlines pairs naturally with angklung's Sundanese heritage.
- **OLE** (ole_): Afro-Latin rhythmic coupling — angklung bursts over OLE's clave patterns creates cross-cultural polyrhythm.

### XPN Velocity Mapping Strategy
- Velocity 1–30: Single tube strike, minimal burst, soft bamboo ring
- Velocity 31–65: Normal shake, 2-hit burst, full cluster chord
- Velocity 66–95: Heavy shake, 3-hit burst, maximum bamboo body resonance
- Velocity 96–127: Aggressive shake, 4-hit burst, ensemble mode partially engaged, pitch spread at maximum
- Velocity scales `angklung_burstCount` (stepped), `angklung_decayTime`, and `angklung_brightness`.

---

## 7. Sitar

### Physical Acoustic Mechanism
The sitar is a North Indian plucked string instrument with 6–7 main playing strings, 11–13 sympathetic strings (taraf) running beneath the frets, and a large gourd resonator. The frets are curved (not flat), allowing the player to pull the string sideways across the fret and raise the pitch by a whole tone — this is the characteristic meend (glide) technique. The most distinctive sonic feature is *jawari*: the bridge is shaped with a slight curvature that causes the string to graze the bridge at a specific point in its vibration, creating a controlled buzz. This buzz is acoustically similar to sitar's instrument identity. The mizrab (plectrum ring worn on the index finger) produces the initial pluck. Sympathetic strings resonate in response to the main strings, creating the characteristic cloud of overtones beneath each note.

### Key Timbre Characteristics
- Jawari buzz: a nasal, buzzing quality beneath every note — the signature of the instrument
- Sympathetic string halo: a cloud of gently ringing pitches that bloom after each note
- Meend (glide): smooth pitch glides up to a whole tone, executed by pulling across the curved fret
- Bright, metallic attack from the mizrab
- The gourd resonator adds a warm, rounded low-mid bloom (similar to oud bowl)
- Sustained notes have a double-envelope character: the initial pluck, then a slower secondary bloom from sympathetics

### DSP Modeling Approach (JUCE/C++)
**Jawari buzz:** The most important DSP challenge. Model as a waveshaping distortion with a subtle asymmetric clipping function applied after string synthesis, tuned to create the specific metallic buzz without gross distortion. An IIR bandpass filter (center: 2–5 kHz, Q = 2) applied to the distorted signal extracts the buzz band and mixes it back with the dry signal at –12 to –6 dB.

```cpp
float jawariProcess(float input, float jawariAmount) {
    // Asymmetric soft clip with buzz enhancement
    float clipped = std::tanh(input * (1.0f + jawariAmount * 4.0f)) / (1.0f + jawariAmount * 4.0f);
    float buzzBand = buzzBandpass.processSample(clipped - input);
    return input + buzzBand * jawariAmount;
}
```

**Sympathetic strings:** 11 voices at fixed pitches (the 12-note chromatic sympathetics are tuned to the raga scale in use). Each is a Karplus-Strong resonator with very long decay (8–15 s). They receive excitation from the main string output at –30 dB, simulating acoustic coupling. Total sympathetic resonance is mixed back at –20 to –12 dB.

**Meend glide:** Continuous pitch bend with exponential curve. `sitar_meendDepth` controls maximum glide range (0–200 cents). A dedicated bend LFO with retrigger-on-note-on simulates the characteristic pull-and-release ornament.

**Gourd resonance:** Same technique as oud bowl: 3–4 BiQuad bandpass modes at 200–600 Hz range, Q = 10–20.

### Key Parameters to Expose
| Parameter | ID | Range | Notes |
|---|---|---|---|
| Jawari Amount | `sitar_jawariAmount` | 0–1 | Core timbre — buzz intensity |
| Sympathetic Volume | `sitar_sympatheticVol` | 0–1 | Taraf string halo level |
| Sympathetic Decay | `sitar_sympatheticDecay` | 4–20 s | Taraf ring duration |
| Meend Depth | `sitar_meendDepth` | 0–200 cents | Glide range |
| Meend Speed | `sitar_meendSpeed` | 0–1 | Glide rate (fast = pluck, slow = sustained pull) |
| Gourd Resonance | `sitar_gourdResonance` | 0–1 | Body bloom level |
| Raga Scale | `sitar_ragaScale` | 0–11 | Root note for sympathetic tuning |
| String Brightness | `sitar_brightness` | 0–1 | Pluck spectral content |

### Coupling Potential
- **ORACLE** (oracle_): Maqam/raga scale system — Oracle's GENDY synthesis with raga tuning applied to sympathetic strings creates a generative Hindustani texture.
- **OPAL** (opal_): Granular sitar creates an endlessly sustaining, atomized sympathetic cloud — preset category: "Infinite Raga."
- **OVERDUB** (dub_): Tape delay with irregular timing creates a slap-back raga loop effect.
- **OUROBOROS** (ouro_): Feedback routing through sitar's sympathetic bus creates topological drone evolution.

### XPN Velocity Mapping Strategy
- Velocity 1–35: Soft fingertip touch, minimal jawari, sympathetics barely audible
- Velocity 36–70: Normal mizrab pluck, full jawari buzz, sympathetics at 50% volume
- Velocity 71–100: Heavy pluck, maximum jawari, sympathetics fully engaged
- Velocity 101–127: Aggressive attack, meend automatically bends up then releases, jawari at peak
- Velocity scales `sitar_jawariAmount` and `sitar_sympatheticVol` simultaneously — both must change with velocity for D001 compliance. The jawari buzz is the key timbre-from-velocity feature.

---

## 8. Shamisen

### Physical Acoustic Mechanism
The shamisen is a Japanese three-string lute with a square body covered on both sides by animal skin (traditionally cat or dog skin). The neck has no frets. Strings are silk or nylon. The instrument is played with a large paddle-shaped plectrum (bachi) in genres like nagauta and jiuta. The skin membrane resonator produces a short, sharp, percussive attack followed by moderate sustain. The most distinctive feature is *sawari*: a deliberate buzz produced by a small groove at the nut that causes the first string to lightly graze an adjacent surface. This is analogous to sitar jawari but produces a rougher, more percussive buzz quality. The bachi produces a loud, sharp snap attack when struck against the skin membrane.

### Key Timbre Characteristics
- Sawari buzz: rougher, more percussive than sitar jawari — a rattle-buzz rather than a metallic hum
- Sharp bachi snap: the plectrum hitting the skin creates an audible percussive slap, especially prominent in shamisen music
- Skin membrane resonance: short decay envelope from the membrane radiator, unlike a wooden soundboard
- Three strings in close intervals (open tuning varies: hon-choshi D-A-D, ni-agari D-A-E, san-sagari D-G-D)
- No frets — requires precise left-hand finger placement; microtonal inflection is possible but less idiomatic than sitar meend
- Dynamic range is wide: from near-inaudible ghost strokes to cutting, aggressive forte bachi attacks

### DSP Modeling Approach (JUCE/C++)
**Sawari buzz:** Similar to sitar jawari but with a different waveshaper character. Shamisen sawari is produced mechanically by a groove — model as a rectified distortion (half-wave or full-wave with noise injection) rather than asymmetric soft clip:

```cpp
float sawariProcess(float input, float sawariAmount) {
    // Half-wave rectification creates a rough, buzzy quality
    float rectified = std::max(0.0f, input) * (1.0f + sawariAmount);
    float buzz = highpassFilter.processSample(rectified - input);
    return input + buzz * sawariAmount * 0.5f;
}
```

**Bachi snap:** A membrane percussion model: a brief (~8 ms) filtered noise burst (bandpass 500 Hz–3 kHz) simulating the skin impact, mixed with the string synthesis at the note onset. `shamisen_bachiSnap` controls this layer's volume.

**Skin membrane decay:** Apply an envelope-controlled lowpass filter sweep post-attack: filter opens wide at note onset (mimicking membrane's bright initial radiation) then closes over 80–200 ms to a darker, focused sustain.

**String model:** Three-voice Karplus-Strong with decay times tuned to shamisen's shorter sustain (1–3 s). Each voice at a slightly different pitch (open tuning) contributes simultaneously.

**Polyphonic tuning modes:** `shamisen_tuning` selects from hon-choshi, ni-agari, or san-sagari open tuning presets, affecting the base pitch of voices 2 and 3.

### Key Parameters to Expose
| Parameter | ID | Range | Notes |
|---|---|---|---|
| Sawari Amount | `sham_sawariAmount` | 0–1 | Core timbre — buzz character |
| Bachi Snap | `sham_bachiSnap` | 0–1 | Membrane percussion snap at onset |
| Skin Decay | `sham_skinDecay` | 0.5–4 s | Membrane sustain duration |
| Tuning Mode | `sham_tuning` | 0–2 | Hon-choshi / ni-agari / san-sagari |
| String Brightness | `sham_brightness` | 0–1 | Pluck spectral content |
| Membrane Bloom | `sham_membraneBloom` | 0–1 | Lowpass filter sweep depth |
| Sawari Roughness | `sham_sawariRoughness` | 0–1 | Noise injection into sawari buzz |
| Attack Sharpness | `sham_attackSharpness` | 0–1 | Bachi strike speed (slow = soft, fast = snap) |

### Coupling Potential
- **ONSET** (perc_): Shamisen bachi snap maps directly to ONSET's percussive layer — combine for a hybrid taiko/shamisen attack.
- **OBESE** (fat_): Saturation drive on shamisen output extends the sawari buzz into overdriven territory — a modern electric-shamisen character.
- **ORACLE** (oracle_): Pentatonic and Japanese scales applied within Oracle's GENDY framework creates generative shamisen-inspired melodic patterns.
- **OBLIQUE** (oblq_): Prism color modulation of the shamisen's skin membrane decay creates a psychedelic, fragmented shamisen texture.

### XPN Velocity Mapping Strategy
- Velocity 1–30: Fingernail soft stroke, minimal bachi snap, subdued sawari
- Velocity 31–65: Normal bachi stroke, full skin membrane bloom, sawari present
- Velocity 66–95: Heavy bachi, maximum snap layer, sawari at full intensity
- Velocity 96–127: Aggressive bachi, percussive snap dominates, membrane resonance peaks, slight string distortion
- Velocity scales `sham_bachiSnap` and `sham_sawariAmount` simultaneously. The bachi snap layer's prominence shift from subtle to dominant is the primary D001 timbre-from-velocity transform.

---

## Cross-Instrument Design Notes

### Shared DSP Infrastructure
Six of the eight instruments use Karplus-Strong as their core string/membrane model. A shared `KarplusStrongVoice` base class should be implemented in `Source/DSP/` with instrument-specific parameters (loop filter type, excitation shape, decay range). This avoids code duplication across six engines and simplifies parameter naming.

### Buzz/Jawari Abstraction
Both sitar and shamisen require a buzz/sawari distortion stage. A shared `BuzzStage` DSP module with configurable waveshaper type (asymmetric soft clip for sitar-style, half-wave rectification for shamisen-style) should live in `Source/DSP/BuzzStage.h`.

### Bowl/Body Resonance Pattern
Oud, guzheng, sitar, and shamisen all use similar body resonance modeling (series of bandpass filters simulating cavity modes). A `BodyResonator` DSP class with configurable mode count, center frequencies, and Q values should be abstracted into `Source/DSP/BodyResonator.h`.

### D003 Compliance Note
All eight instruments have documented physical acoustic mechanisms with measurable parameters. Engine design briefs must cite these mechanisms explicitly and ensure that parameter names in the UI reference the physical source (e.g., "Jawari Buzz" not "Distortion Amount", "Bow Pressure" not "Timbre"). D003 requires the physics to be named, not hidden.

---

*End of document — world_instrument_dsp_research.md*
