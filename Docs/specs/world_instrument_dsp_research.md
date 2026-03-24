# World Instrument DSP Research — Collections Preparation

**Date**: 2026-03-16
**Author**: XO_OX R&D
**Purpose**: DSP modeling research for world instruments targeted at the Artwork/Color Collection and related V2 engines. Covers acoustic physics, timbre analysis, JUCE C++ implementation strategy, parameter design, engine coupling, and XPN velocity mapping for 8 instruments.

---

## 1. Erhu — Chinese 2-String Bowed Fiddle

### 1.1 Physical Acoustic Mechanism

The erhu consists of two silk (or modern steel-core) strings stretched across a small hexagonal or octagonal resonator body covered with python snakeskin on one face. The snakeskin membrane acts as the primary radiating surface — a coupled oscillator between string vibration transmitted via the bridge and the membrane's own modal response. The bow (horsehair, rosined) is threaded permanently between the two strings, meaning bow angle and pressure simultaneously control both strings even when only one is sounding. This design produces a continuous bowing gesture physically unlike Western bowed strings.

The acoustic coupling chain: bow friction → string self-sustained oscillation → bridge → snakeskin membrane → air column in the small resonator body → radiated sound. The membrane's low mass and high compliance gives the erhu its characteristic forward, nasal tone. There is no soundboard in the Western sense — the membrane IS the soundboard, and it has a very different frequency response profile (thinner high-frequency content, softer low-end rolloff) compared to spruce or maple.

Bow pressure (normal force) and bow speed interact nonlinearly via the Helmholtz stick-slip mechanism. At low pressure with high speed, the string "whistles" or enters flageolet-like modes. At high pressure with moderate speed, the tone "cracks" into a harsh overdriven state. The stable playing zone is a narrow band of pressure/speed ratio — and erhu players exploit the instability edges expressively. This is why the erhu is considered vocally expressive: it has a continuous timbral trajectory comparable to the human voice's glottal control.

### 1.2 Key Timbre Characteristics

- **Attack transient**: Bow initiation produces a soft attack with a pre-tone "scratch" component as the stick-slip regime establishes. No hard pluck transient — onset is 10–50ms of noisy excitation before pitched tone locks in.
- **Sustain character**: Remarkably stable fundamental with slowly evolving odd-harmonic emphasis. The snakeskin resonator introduces a slight "honk" around 800–1200 Hz — a formant peak that gives the erhu its nasal midrange quality. Vibrato is produced by finger pressure variation on the string (not lateral oscillation as in violin), which modulates both pitch and timbre simultaneously.
- **Harmonic content**: Predominantly odd harmonics (clarinet-like) due to the conical-ish bore coupling effect of the membrane resonator. Rich in harmonics 3, 5, 7, falling off sharply above harmonic 11–13. Less bright than violin at the same pitch. Sub-bass nearly absent.
- **Release**: Bow lift creates a fast, clean decay with no sympathetic string resonance. Very controllable release.

### 1.3 DSP Modeling Approach (JUCE C++)

**Primary synthesis: Extended Digital Waveguide (Smith III model)**

Use a bidirectional delay line model for the string. The key extension over basic Karplus-Strong is the bowing excitation model:

```cpp
// Bowing excitation (Hiller-Ruiz friction model)
float relativeVelocity = bowVelocity - stringVelocity; // velocity at bow contact point
float frictionForce = bowPressure * frictionCoeff * std::tanh(relativeVelocity / slewFactor);
float excitationSample = frictionForce - forceReflected;
```

The Hiller-Ruiz bowing physics model (1993) models the stick-slip boundary as a piecewise nonlinear function. In practice, a `tanh()` approximation of the friction curve captures the key behavior at audio rates without full physical accuracy. The key parameter is `relativeVelocity = bowVelocity - stringVelocity` sampled at the bow contact point on the delay line.

**Resonator: Parallel formant filter bank for snakeskin membrane**

The snakeskin body resonance is best captured as 4–6 second-order biquad bandpass filters in parallel, tuned to measured resonance peaks:
- Formant 1: ~400 Hz, Q ~3 (low-end body warmth)
- Formant 2: ~900 Hz, Q ~5 (the characteristic "honk")
- Formant 3: ~2200 Hz, Q ~4 (nasal brightness)
- Formant 4: ~4500 Hz, Q ~6 (shimmer/silk)

Implement as `juce::dsp::IIR::Filter` instances in a `juce::dsp::ProcessorChain` or manually via biquad coefficients for tighter control.

**Vibrato: Pitch + amplitude modulation coupled**

Erhu vibrato modulates both pitch (±30–80 cents) and amplitude (±5–15%) in phase. Use a single LFO signal applied to both the waveguide delay length and a VCA gain multiplier with a coupling coefficient parameter.

**Alternative lightweight approach**: For polyphonic contexts, replace the full waveguide with a noise-excited resonator using a `juce::dsp::StateVariableTPTFilter` in bandpass mode per voice, driven by a bowed-noise excitation signal (bandlimited noise shaped with a dynamic HP filter to simulate bow rosin spectrum). This is 60–70% perceptually accurate at a fraction of the CPU cost.

### 1.4 JUCE APVTS Parameters

| Parameter ID | Name | Range | Default | Notes |
|---|---|---|---|---|
| `erhu_bow_pressure` | Bow Pressure | 0.0–1.0 | 0.5 | Controls stick/slip regime. Macro target. |
| `erhu_bow_speed` | Bow Speed | 0.0–1.0 | 0.5 | Interacts with pressure for timbre. |
| `erhu_bow_position` | Bow Position | 0.05–0.5 | 0.12 | Sul tasto (0.5) to sul ponticello (0.05). |
| `erhu_vibrato_rate` | Vibrato Rate | 2.0–8.0 Hz | 5.0 | Finger vibrato speed. |
| `erhu_vibrato_depth` | Vibrato Depth | 0.0–1.0 | 0.3 | Pitch + amplitude modulation depth. |
| `erhu_body_resonance` | Body Resonance | 0.0–1.0 | 0.6 | Formant filter wet mix. |
| `erhu_string_decay` | String Decay | 0.1–4.0 s | 1.2 | Waveguide loss factor. |
| `erhu_brightness` | Brightness | 0.0–1.0 | 0.5 | High-formant emphasis. |
| `erhu_scratch` | Bow Scratch | 0.0–1.0 | 0.2 | Pre-tone noise excitation level. |
| `erhu_portamento` | Portamento | 0.0–500 ms | 30 ms | Glide time between notes. |

### 1.5 Coupling Potential with Existing XOlokun Engines

- **OPAL (Granular)**: Erhu sustain textures fed into OPAL's grain cloud creates ethereal string beds. Coupling axis: Erhu provides the melodic seed; OPAL smears it into atmosphere.
- **OVERDUB (Dub Synth)**: Tape delay on erhu creates the "echoing voice in a canyon" archetype. OVERDUB's spring reverb is physically appropriate (erhu + chamber resonance).
- **OVERLAP (FDN Reverb)**: The knot-topology FDN's dense reflections suit erhu's sustain character — preserves the formant structure while adding space.
- **OHM**: Erhu's odd-harmonic character and OHM's drone harmonics share spectral real estate — coupling creates a just-intonation drone texture.

### 1.6 XPN Velocity Mapping Strategy

Velocity on erhu maps to **bow pressure + bow speed combined**, which controls the stick-slip regime:

- **Velocity 1–40 (pianissimo)**: Low bow pressure, moderate speed → sul tasto tone, reduced brightness, slight "airy" scratch, vibrato depth minimal
- **Velocity 41–80 (mezzo)**: Balanced pressure/speed → full rich tone, vibrato active, body resonance prominent
- **Velocity 81–100 (forte)**: High pressure → ponticello brightness, increased harmonic content, slight edge/crunch in attack, faster vibrato onset
- **Velocity 101–127 (sforzando/accent)**: Bow attack spike → sharp transient noise burst before tone settles, maximum brightness, reduced body warmth (sul ponticello character)

Secondary velocity targets: `erhu_scratch` scales 0→0.4 from soft→loud. `erhu_brightness` scales 0.3→0.9. Vibrato depth scales 0.1→0.5.

---

## 2. Didgeridoo — Australian Circular-Breathing Drone

### 2.1 Physical Acoustic Mechanism

The didgeridoo is a naturally formed wooden tube (eucalyptus or bamboo), typically 1–1.5 meters long, with a conical or near-conical bore profile formed by the natural hollowing of the wood. The player's lips form a loose reed that excites the air column via buzzing, and critically, the shape of the vocal tract, cheeks, and tongue forms a secondary resonating cavity in series with the instrument bore.

This dual-resonator topology — vocal tract + instrument bore — is what creates the didgeridoo's characteristic timbral range. The player can shift formants dramatically by changing oral cavity shape (jaw position, tongue position, lip aperture), producing the "wah-wah" tonal cycling. Circular breathing (simultaneous exhale through the instrument while inhaling through the nose using cheek-stored air) sustains indefinitely, and the transition between cheek-push exhale and lung exhale creates rhythmic micro-dynamics that are fundamental to traditional playing style.

The bore's near-conical profile means odd AND even harmonics are present (unlike a cylindrical tube which strongly emphasizes odd harmonics). The lowest resonance (fundamental drone) is typically D2–A2 depending on tube length. Harmonics stack up to 8–12 partials in the audible range.

### 2.2 Key Timbre Characteristics

- **Attack transient**: Lip buzz onset is almost instantaneous (< 5ms) but with a low-frequency "thump" from the air column pressurizing. No sharp transient — more of a smooth low-frequency onset.
- **Sustain character**: Continuous fundamental drone with slowly cycling formant modulation (the "wah" shape). Rich in harmonics 2–8, with formant peaks sweeping across them. Highly dynamic timbre despite constant pitch.
- **Harmonic content**: Full harmonic series up to roughly harmonic 10–12. Even harmonics are present (conical bore), giving it more of a "saxophone" quality vs. clarinet. The vocal tract formants selectively amplify different harmonic bands as they shift.
- **Release**: Abrupt lip-stop release, sometimes with a pitched glottal consonant. Very short natural release.

### 2.3 DSP Modeling Approach (JUCE C++)

**Primary synthesis: Conical bore waveguide + LPC vocal tract model**

The conical bore can be approximated by a Kelly-Lochbaum waveguide with a series of expanding cylindrical sections (wave propagation junction model). For efficiency, a simpler approach is a single delay line with a bore-appropriate reflection filter (low-pass at the open end, near-total reflection at the lip end):

```cpp
// Conical bore: reflection gain tapers toward open end
float openEndReflection = -0.98f * lowPassFilter.process(outputSample);
float lipEndReflection = 0.98f * inputExcitation;
```

**Vocal tract: Linear Predictive Coding (LPC) filter**

A 10–12-pole LPC filter represents the vocal tract. In JUCE, this is a cascade of second-order IIR sections (5–6 biquads). The innovation: animate the LPC coefficients in real time by interpolating between stored "vocal shapes" (precomputed coefficient sets for different mouth/tongue positions). Key shapes to precompute and crossfade:

1. Neutral (flat spectrum)
2. "Ah" open vowel (boosted low formant)
3. "Oo" rounded vowel (shifted F1/F2 pattern)
4. "Ee" front vowel (high F2 emphasis)
5. "Wah" transition (F1 sweep from low to high)

**Circular breathing LFO:**

Model circular breathing as a rhythmic amplitude modulation applied to the pressure signal with a specific envelope shape:
- Lung exhale phase: constant amplitude ~1.0
- Cheek-push transition: brief amplitude dip (~0.85) with slight pitch flatness
- LFO period: 1–4 seconds (breathing rate), with subtle sub-LFO rhythmic pulsing (drone rhythm patterns)

**Lip excitation**: Bandlimited sawtooth or pulse wave at fundamental frequency, fed into the bore waveguide as the excitation signal. Lip tension controls fundamental pitch bend.

### 2.4 JUCE APVTS Parameters

| Parameter ID | Name | Range | Default | Notes |
|---|---|---|---|---|
| `didge_fundamental` | Fundamental | -24–0 semitones | 0 | Tube length offset (affects overtone spacing). |
| `didge_lip_tension` | Lip Tension | 0.0–1.0 | 0.5 | Pitch bend range and buzz character. |
| `didge_mouth_shape` | Mouth Shape | 0.0–1.0 | 0.5 | LPC vocal tract interpolation (Ah→Oo→Ee). |
| `didge_wah_rate` | Wah Rate | 0.1–4.0 Hz | 0.8 | Formant cycling speed. |
| `didge_wah_depth` | Wah Depth | 0.0–1.0 | 0.6 | Formant sweep amplitude. |
| `didge_breathe_rate` | Breathe Rate | 0.25–2.0 Hz | 0.5 | Circular breathing rhythm period. |
| `didge_breathe_depth` | Breathe Depth | 0.0–1.0 | 0.3 | Amplitude modulation of breathing cycle. |
| `didge_overtone_bloom` | Overtone Bloom | 0.0–1.0 | 0.4 | Harmonic emphasis on upper partials. |
| `didge_bore_resonance` | Bore Resonance | 0.0–1.0 | 0.7 | Waveguide feedback strength. |
| `didge_sub_drone` | Sub Drone | 0.0–1.0 | 0.3 | Subharmonic (octave-below) blend. |

### 2.5 Coupling Potential with Existing XOlokun Engines

- **ORGANISM (Concept/V2)**: Cellular automata generative rhythms driving didgeridoo breath patterns — the drone becomes a living, evolving entity.
- **OHM**: Natural pairing. OHM's just-intonation drone harmonics and didgeridoo's harmonic series overlap almost perfectly. Together: primal acoustic resonance system.
- **OBBLIGATO (Dual Wind)**: Bidirectional coupling — didgeridoo provides the root drone while OBBLIGATO layered winds provide the melodic motion above it.
- **OVERLAP**: FDN reverb on didgeridoo creates cave/canyon acoustics that suit the instrument's ceremonial context.

### 2.6 XPN Velocity Mapping Strategy

Velocity maps to **lung pressure and overtone activation**:

- **Velocity 1–40**: Low pressure, neutral mouth shape, minimal wah, sub drone audible
- **Velocity 41–80**: Standard playing pressure, wah cycling active, full harmonic series
- **Velocity 81–127**: High pressure → overtone bloom increased, faster wah cycling, lip tension rises slightly (subtle pitch rise), breathe depth decreases (more sustained pressure, less rhythmic modulation)

The sub drone (`didge_sub_drone`) inversely scales with velocity — louder playing suppresses the sub, quieter playing lets it bloom forward.

---

## 3. Oud — Middle Eastern/North African Short-Neck Lute

### 3.1 Physical Acoustic Mechanism

The oud is a fretless short-neck lute with 11 or 12 strings in courses (pairs, except for the lowest bass string which may be single). The body is a deep bowl shape constructed from many thin wooden staves (typically walnut, maple, or rosewood), giving it exceptional internal volume relative to its face area. The soundboard is spruce or cedar with two or three decorative rose sound holes. The bridge is floating (not glued), meaning string energy transfers to the soundboard via bridge rocking motion — a subtly different coupling than a fixed bridge.

The floating bridge introduces a characteristic "live" quality: the bridge can shift slightly under heavy playing, and its rocking motion creates a subtle high-frequency "shimmer" sometimes called "buzz" in Arabic music contexts (similar in concept to the jawari of sitar or sawari of shamisen, though less pronounced). The deep bowl body creates a long, reverberant internal cavity with strong low-mid resonance (peaking around 200–400 Hz).

The fretless neck allows microtonal inflections — maqam scales use intervals between equal-tempered notes (quarter tones, neutral thirds). This is fundamental to the oud's identity in Arabic, Turkish, and Persian music.

### 3.2 Key Timbre Characteristics

- **Attack transient**: Sharp, defined pluck transient (the "click" of the plectrum or finger on string). Shorter attack than guitar due to shorter scale length and higher string tension relative to body mass. The attack includes both the string excitation and an immediate body "knock" from the floating bridge impact.
- **Sustain character**: Moderate sustain with a characteristic warm mid-body resonance. Notes bloom briefly (5–15ms) before the sustain phase settles. The bowl resonance creates a "singing" quality in the mid-range. Damping is relatively fast compared to guitar — oud notes speak clearly but don't sustain forever.
- **Harmonic content**: Rich and balanced — fundamentals are strong, harmonics 2–5 prominent, upper harmonics (6–12) present but rolling off gently. The bowl resonance emphasizes a broad region around 250–600 Hz. Less treble than a steel-string guitar, more warmth than a classical guitar.
- **Release**: Natural decay following string damping physics. The floating bridge can add a slight "ringing" after note cutoff.

### 3.3 DSP Modeling Approach (JUCE C++)

**Primary synthesis: Karplus-Strong with extended pluck model**

Standard KS provides the string behavior. Key additions for oud:

1. **Pluck position**: Parameterize the "comb" filter notch in the KS algorithm by pluck position (0.05 = near bridge, 0.5 = mid-string), which controls which harmonics are suppressed.
2. **Pluck hardness**: Control the noise burst length and spectral content of the excitation signal (short burst = bright pluck, longer burst = softer finger tone).

**Body resonance: Parallel bandpass resonator bank OR convolution IR**

Option A (Algorithmic): 8–10 parallel second-order bandpass filters with tuned resonance peaks modeling the bowl cavity modes. Key tunings:
- 180 Hz (fundamental cavity mode)
- 310 Hz (primary bowl resonance)
- 520 Hz (secondary bowl resonance)
- 900 Hz (bridge coupling mode)
- 1800 Hz (soundboard mode)
- 3200 Hz (high soundboard mode)

Option B (Convolution): Short IRs (20–50ms) captured from real oud body taps. Use `juce::dsp::Convolution` with a short kernel for efficiency.

**Floating bridge buzz: Subtle FM**

Model the floating bridge shimmer as a very low-index FM modulation (modulation index 0.02–0.08) applied to the string signal, with the modulator frequency tracking the fundamental at a ratio of ~7.3–8.1 (inharmonic). This adds the characteristic live "shimmer" without sounding like deliberate vibrato.

**Microtonal support**: Expose a `oud_pitch_quantize` parameter with options: Off / Equal Temp / Maqam Rast / Maqam Bayati / Maqam Hijaz.

### 3.4 JUCE APVTS Parameters

| Parameter ID | Name | Range | Default | Notes |
|---|---|---|---|---|
| `oud_pluck_position` | Pluck Position | 0.05–0.5 | 0.12 | Sul ponticello to sul tasto. |
| `oud_pluck_hardness` | Pluck Hardness | 0.0–1.0 | 0.5 | Pick vs. finger, excitation burst length. |
| `oud_body_resonance` | Body Resonance | 0.0–1.0 | 0.65 | Bowl resonator wet mix. |
| `oud_bridge_buzz` | Bridge Buzz | 0.0–1.0 | 0.15 | Floating bridge FM shimmer depth. |
| `oud_string_decay` | String Decay | 0.2–3.0 s | 0.8 | KS damping coefficient. |
| `oud_warmth` | Warmth | 0.0–1.0 | 0.6 | Low-mid body resonance emphasis. |
| `oud_course_detune` | Course Detune | 0.0–20 cents | 3 cents | Course string pair detuning (chorus). |
| `oud_portamento` | Portamento | 0–300 ms | 0 | Glide for maqam inflections. |
| `oud_maqam` | Maqam Scale | Off/Rast/Bayati/Hijaz/Saba | Off | Pitch quantization mode. |
| `oud_attack_click` | Attack Click | 0.0–1.0 | 0.4 | Plectrum transient emphasis. |

### 3.5 Coupling Potential with Existing XOlokun Engines

- **OPAL**: Granular processing of oud sustain creates Middle Eastern ambient textures. The oud's warm mid-range feeds beautifully into grain clouds.
- **OTTONI (Triple Brass)**: Unexpected coupling: oud melodic lines against brass pads creates orchestral Arabic fusion textures.
- **OVERDUB (Tape Delay)**: Tape delay on oud is historically accurate — many classic Arabic recordings used tape echo. The warmth of OVERDUB's analog character suits oud's timbre.
- **OLE (Afro-Latin)**: Oud's Andalusian heritage and OLE's Afro-Latin character share historical musical lineage (Arabic → Moorish Spain → Flamenco). Coupling axis: the Mediterranean-Atlantic bridge.

### 3.6 XPN Velocity Mapping Strategy

Velocity maps to **pluck hardness and attack transient prominence**:

- **Velocity 1–30**: Soft finger tone — minimal attack click, high pluck position (sul tasto), long decay, low bridge buzz
- **Velocity 31–70**: Pick playing — moderate attack click, mid pluck position, standard decay
- **Velocity 71–100**: Hard pick attack — prominent attack click, lower pluck position (slightly brighter), course detune increases slightly
- **Velocity 101–127**: Forte stroke — maximum attack click, bridge buzz peaks, shortest decay settling time (the "smack" character of Arabic forte playing)

---

## 4. Guzheng — Chinese 21-String Zither

### 4.1 Physical Acoustic Mechanism

The guzheng is a horizontal zither approximately 160cm long, with 21 strings (traditionally silk, now typically steel or nylon-coated steel) stretched over a slightly curved soundboard (paulownia wood). Each string passes over a movable bridge (nut), allowing players to re-tune individual strings for different modes or perform "bend and release" techniques by pressing the string to the left of the bridge before or after plucking to raise pitch.

The characteristic technique involves the right hand plucking strings while the left hand presses on the string's other side of the bridge to apply pitch bends — this creates the "crying" inflection fundamental to guzheng expression. The default tuning is pentatonic (do-re-mi-sol-la), though players retune bridges to access heptatonic or chromatic notes.

The soundboard resonance is critical — the flat paulownia board has strong radiation efficiency and a characteristic "woody" resonant character with prominent modes in the 300–800 Hz range. The strings are relatively long (up to 115cm for bass strings), giving long sustain with slow natural decay.

### 4.2 Key Timbre Characteristics

- **Attack transient**: Sharp, bright pluck transient with fingernail or plectrum (guizhao, artificial nails worn on right-hand fingers). The attack has a distinctive "nail click" component followed immediately by the pitched string. Attack time: <3ms.
- **Sustain character**: Long, singing sustain on treble strings. Bass strings decay faster due to higher string mass and stronger coupling to the soundboard. The characteristic "bend and release" creates a note that either rises to pitch (press before pluck) or falls from pitch (press and release after pluck).
- **Harmonic content**: Bright, with strong fundamental and harmonics 2–6. Steel string brightness gives more treble than the erhu. Harmonics 7–10 are present but less prominent. The wooden soundboard emphasizes mid-range around 400–700 Hz.
- **Release**: Natural exponential decay. Players sometimes "stop" strings for staccato.

### 4.3 DSP Modeling Approach (JUCE C++)

**Primary synthesis: Karplus-Strong with bend modulation**

The bend technique requires real-time control over the KS delay line length (pitch):

```cpp
// Bend modulation: press left of bridge raises pitch
float bendSemitones = bendAmount * bendRange; // bendRange: 0–2 semitones typical
float targetDelay = sampleRate / (midiNoteToFreq(noteNumber) * std::pow(2.0f, bendSemitones / 12.0f));
currentDelay += (targetDelay - currentDelay) * lerpCoeff; // smooth glide
```

The bend envelope shape is asymmetric: fast attack (press before pluck) with slow release, or immediate drop (pluck then release). Expose as a dedicated bend envelope with pre-pluck/post-pluck mode.

**Multi-string architecture**: 21 simultaneous string synthesis is CPU-intensive. Solution: voice pool of 8–12 KS voices with intelligent voice stealing. Each voice maintains its own bend state independently.

**Soundboard: Parallel resonator bank**

6–8 bandpass resonators tuned to measured guzheng soundboard modes, applied as a shared post-processing stage (not per-voice). The soundboard is global, not per-string.

**Pentatonic quantization**: A scale-lock processor that maps any MIDI input to the nearest note in the active pentatonic (or heptatonic) mode. Expose as a toggle with mode selection.

**Glissando / cascading arpeggiation**: A built-in arpeggiator mode that plays scale runs up/down the string register. Implemented as a rapid-fire trigger of sequential notes with per-note velocity shaping.

### 4.4 JUCE APVTS Parameters

| Parameter ID | Name | Range | Default | Notes |
|---|---|---|---|---|
| `gzheng_pluck_position` | Pluck Position | 0.05–0.5 | 0.1 | Bridge proximity. |
| `gzheng_nail_hardness` | Nail Hardness | 0.0–1.0 | 0.6 | Guizhao nail attack brightness. |
| `gzheng_bend_amount` | Bend Amount | 0.0–2.0 st | 0.0 | Left-hand press pitch raise. |
| `gzheng_bend_mode` | Bend Mode | Pre/Post/Auto | Auto | Pre-pluck rise or post-pluck fall. |
| `gzheng_bend_time` | Bend Time | 10–500 ms | 80 ms | Glide time for bend. |
| `gzheng_string_decay` | String Decay | 0.5–8.0 s | 2.5 | KS damping. |
| `gzheng_soundboard_res` | Soundboard | 0.0–1.0 | 0.55 | Resonator wet mix. |
| `gzheng_scale_lock` | Scale Lock | Off/Penta/Hepta | Penta | Pitch quantization. |
| `gzheng_scale_root` | Scale Root | C–B | C | Root note for scale lock. |
| `gzheng_gliss_speed` | Gliss Speed | 20–500 ms | 80 ms | Glissando cascade rate. |
| `gzheng_string_count` | String Voices | 4–12 | 8 | Active polyphony. |

### 4.5 Coupling Potential with Existing XOlokun Engines

- **OPAL**: Guzheng's long sustain tails feed beautifully into granular processing. The bend-and-release technique creates pitch-gliding grains that sound uniquely alive.
- **ONSET**: Guzheng's pluck transient paired with ONSET's percussion creates a hybrid "plucked percussion" texture. Coupling axis: ONSET handles the attack impulse, guzheng provides the pitched sustain.
- **OVERLAP**: The FDN's long tail complements guzheng's natural sustain. Together they create an extended reverberant space that suits the instrument's hall performance context.
- **OHM**: Pentatonic scale lock + OHM's just-intonation drone creates a self-consistent tonal world with no dissonant notes possible.

### 4.6 XPN Velocity Mapping Strategy

Velocity maps to **nail hardness and soundboard coupling intensity**:

- **Velocity 1–35**: Soft pluck — long decay, minimal nail click, soundboard resonance prominent, bend depth limited
- **Velocity 36–75**: Standard technique — balanced nail/body mix, moderate decay, full bend range available
- **Velocity 76–100**: Hard pluck — bright nail click prominent, faster initial decay (strings move more), slight increase in harmonic content
- **Velocity 101–127**: Forte pluck — maximum nail attack, short sustain before decay settles, reduced body resonance (directional energy)

Secondary: `gzheng_gliss_speed` can be velocity-triggered — high velocities on sustained notes can auto-trigger a brief downward glissando cascade.

---

## 5. Handpan — Steel Percussion (Hang Drum)

### 5.1 Physical Acoustic Mechanism

The handpan (generic term; Hang is a registered trademark of PANArt) consists of two hemispherical steel shells joined at the rim. The top shell (ding side) has a central dome (the "ding" note) surrounded by 7–9 tone fields — indented areas that each produce a specific pitch. The bottom shell has a central opening (the "gu" hole) that acts as a Helmholtz resonator, adding the characteristic low "breath" undertone.

Each tone field is a steel membrane with a specific geometry that produces three coupled vibration modes: the fundamental, an octave above, and a compound fifth (compound mode at ~2.5x the fundamental). These three modes are tuned to be in specific harmonic ratios — the octave and fifth/octave relationships give the handpan its characteristic "shimmering" sustain quality where overtones blend harmonically.

The Helmholtz resonator (gu hole) adds a very slow-decaying bass frequency tuned roughly to the instrument's root — this creates the "breathing" low-end bloom when the instrument is struck. The entire structure is a coupled vibroacoustic system, and the nitrided steel material provides very long sustain (5–15 seconds for upper notes) with a distinctive metallic shimmer during decay.

### 5.2 Key Timbre Characteristics

- **Attack transient**: A soft "thwack" from hand contact — low-frequency impact with a very fast rise to the pitched modes. The attack is much softer than a steel drum or drum kit hit. The fundamental and octave mode emerge within 5–10ms of the strike.
- **Sustain character**: Very long, bell-like sustain. The three coupled modes (fundamental, octave, fifth) create a shimmering interference pattern during decay as they decay at slightly different rates. The Helmholtz resonator adds a slow bass bloom on sustained notes.
- **Harmonic content**: Inharmonic — the overtones are NOT integer multiples of the fundamental (modal synthesis territory). Primary modes: f0, ~2f0, ~2.5f0. Secondary modes: ~3.5f0, ~4.2f0. These inharmonic partials give the "magical" character.
- **Release**: Extremely slow natural decay (up to 15s). Players use hand damping to cut notes.

### 5.3 DSP Modeling Approach (JUCE C++)

**Primary synthesis: Modal synthesis with inharmonic partial bank**

Modal synthesis is the correct approach. Each tone field is modeled as a bank of parallel exponentially-decaying sinusoidal resonators:

```cpp
struct ModalMode {
    float frequency;   // f0 * inharmonicRatio
    float amplitude;   // relative mode strength
    float decayRate;   // mode-specific T60 time
    // Second-order resonator (biquad bandpass with very high Q)
    juce::dsp::IIR::Filter<float> resonatorFilter;
};

// Tone field: 5–7 modes with empirically measured inharmonic ratios
// PANArt Hang-style ratios (approximate):
//   f0 * 1.000, 2.003, 2.497, 3.521, 4.187, 5.830, 7.123
//   amplitudes:  1.00,  0.60,  0.40,  0.20,  0.12,  0.06,  0.03
```

The exact inharmonic ratios vary by handpan maker and note position — expose them as tuning parameters for different "maker presets."

**Helmholtz resonator: Long-decay comb filter tuned to root**

```cpp
// Helmholtz resonator: slowly decaying low-frequency component
float helmholtzFreq = instrumentRoot * 0.5f; // sub-root region
float combDelay = sampleRate / helmholtzFreq;
float helmholtzOutput = combFilter.process(excitation) * helmholtzLevel * std::exp(-time / helmholtzDecay);
```

**Metallic shimmer: Feedback delay network**

A small FDN (4x4) with tuned delay times (1–15ms) creates the metallic shimmer and cross-modal coupling characteristic of steel. Very short delay times produce dense metallic diffusion without audible echo.

**Strike excitation model**: Short noise burst (2–5ms), bandlimited to 50–800 Hz, simulates the hand-contact impulse.

### 5.4 JUCE APVTS Parameters

| Parameter ID | Name | Range | Default | Notes |
|---|---|---|---|---|
| `hpan_strike_hardness` | Strike Hardness | 0.0–1.0 | 0.4 | Hand strike force, excitation bandwidth. |
| `hpan_strike_position` | Strike Position | 0.0–1.0 | 0.5 | Center vs. edge of tone field. |
| `hpan_inharmonicity` | Inharmonicity | 0.0–1.0 | 0.5 | Mode ratio spread (0=harmonic, 1=max spread). |
| `hpan_fund_decay` | Fund Decay | 1.0–15.0 s | 6.0 | Fundamental T60 time. |
| `hpan_shimmer_decay` | Shimmer Decay | 0.5–8.0 s | 3.0 | Upper mode decay time. |
| `hpan_helmholtz_level` | Helmholtz Level | 0.0–1.0 | 0.35 | Gu hole Helmholtz resonance depth. |
| `hpan_helmholtz_freq` | Helmholtz Freq | 0.5–2.0x | 0.5x | Helmholtz frequency relative to root. |
| `hpan_metallic_shimmer` | Metallic Shimmer | 0.0–1.0 | 0.4 | FDN metallic diffusion depth. |
| `hpan_mode_coupling` | Mode Coupling | 0.0–1.0 | 0.3 | Cross-mode energy transfer. |
| `hpan_maker_style` | Maker Style | PANArt/Saraz/Yishama/Free | PANArt | Inharmonic ratio preset. |

### 5.5 Coupling Potential with Existing XOlokun Engines

- **OPAL**: Handpan's long inharmonic sustain + OPAL's granular processing creates shimmering ambient clouds. A very high-value coupling.
- **OVERLAP**: The knot-topology FDN's metallic diffusion character is almost custom-made for handpan — OVERLAP's own metallic decay extends the handpan's sustain in a physically coherent way.
- **OTTONI (Triple Brass)**: Handpan's inharmonic partials against brass chorals creates a modern "sacred music" texture.
- **ORACLE**: ORACLE's stochastic GENDY engine against handpan modal sustain creates an alien-yet-organic atmosphere. Both instruments resist equal temperament — natural partners.

### 5.6 XPN Velocity Mapping Strategy

Velocity maps to **strike hardness and mode activation**:

- **Velocity 1–25**: Gentle tap — only fundamental and octave modes excited, Helmholtz resonator prominent (the "breath" dominates), very long soft decay
- **Velocity 26–60**: Normal playing — fundamental through 4th mode excited, balanced mix, standard decay times
- **Velocity 61–90**: Stronger hit — modes 1–6 excited, metallic shimmer increases, Helmholtz slightly suppressed, slight shortening of fundamental decay
- **Velocity 91–127**: Hard rim strike — all modes excited plus high-frequency transient, maximum shimmer, strike position shifts toward edge (more inharmonic), Helmholtz barely audible

Note: Handpan rarely plays very loud — velocity sensitivity should be compressed at the top end. Velocity 127 still sounds musical, not struck.

---

## 6. Angklung — Indonesian Bamboo Rattle

### 6.1 Physical Acoustic Mechanism

The angklung is a bamboo musical instrument originating from West Java (Sundanese culture). Each angklung frame holds two or three bamboo tubes cut to produce a specific pitch. The tubes are held in a frame and can slide vertically, and when the frame is shaken or rattled, the tubes strike internal stops within the frame — the impact excites the air columns in the bamboo tubes.

Each tube is cut to a specific length that determines its resonant frequency. The frame typically holds two tubes tuned in octaves (or sometimes octave + fifth), so each angklung inherently produces a two-note "chord" — the fundamental and its octave — creating automatic doubling that gives ensembles their characteristic full, rich sound. In an angklung ensemble, each player holds one frame (one pitch), and melodies emerge from collective coordination — a profoundly social instrument.

The bamboo material has specific acoustic properties: relatively high damping (shorter sustain than metal), warm inharmonic content due to bamboo's density and cylindrical cut geometry, and a characteristic woody "clunk" at tube impact that precedes the pitched resonance. This makes each note a two-part sound: impact transient + tuned resonance.

### 6.2 Key Timbre Characteristics

- **Attack transient**: A distinct woody "knock" (3–8ms) from tube impact, followed immediately by the pitched air column resonance. The knock is not purely inharmonic — it contains energy at the tube's resonance frequencies but with higher bandwidth (more noise content).
- **Sustain character**: Moderate sustain (0.5–2s) with gentle decay. The octave doubling creates a natural chorus/shimmer. Multiple sequential shakes produce a rhythmic rattling pattern with overlapping decays.
- **Harmonic content**: Tube resonators produce a fundamental with harmonics 2, 3, 4 — less harmonic content than metal pipes due to bamboo's higher internal damping. The octave tube adds a clean octave layer. Overall spectrum is warm and mid-forward.
- **Release**: Fast natural decay. No sympathetic resonance between frames.

### 6.3 DSP Modeling Approach (JUCE C++)

**Primary synthesis: Inharmonic tube resonator (two parallel instances)**

Model each angklung as two resonator instances (fundamental tube + octave tube):

```cpp
struct BambooTube {
    float frequency;
    // Q is lower for bamboo (~8–15) vs metal (~20–50) due to higher damping
    juce::dsp::IIR::Filter<float> resonatorBP;

    void setFrequency(float f, float sampleRate) {
        auto coeffs = juce::dsp::IIR::Coefficients<float>::makeBandPass(sampleRate, f, 10.0f);
        resonatorBP.coefficients = coeffs;
    }
};

BambooTube fundamentalTube, octaveTube;
// Optional third tube (fifth) for three-tube frames
```

**Rattle excitation: Noise burst with micro-timing spread**

Each shake event triggers a short noise burst (5–15ms) band-limited to the tube frequency region. Stack 2–4 of these with slight timing randomization to simulate the double/triple impact of a natural shake:

```cpp
void triggerShake(float velocity) {
    float delay1 = 0.0f;
    float delay2 = juce::Random::getSystemRandom().nextFloat() * 5.0f + 3.0f; // 3–8ms offset in ms
    scheduleImpulse(delay1, velocity);
    scheduleImpulse(delay2 * sampleRate / 1000.0f, velocity * 0.7f);
}
```

**Octave/fifth doubling chorus**: The two tubes naturally produce detuning from manufacturing variation (±5–15 cents between nominal octave and perfect 2:1 ratio). A slight fractional frequency offset on the octave tube creates a subtle "living" quality.

**Ensemble simulation**: An optional mode that stacks 3–5 detuned versions of the two-tube model to simulate an angklung section playing the same note.

### 6.4 JUCE APVTS Parameters

| Parameter ID | Name | Range | Default | Notes |
|---|---|---|---|---|
| `ankl_shake_force` | Shake Force | 0.0–1.0 | 0.5 | Impact velocity and noise burst energy. |
| `ankl_shake_rhythm` | Shake Rhythm | 0.0–1.0 | 0.4 | Internal rattle timing spread. |
| `ankl_tube_damping` | Tube Damping | 0.0–1.0 | 0.6 | Bamboo resonator Q (0=high Q, 1=heavily damped). |
| `ankl_octave_detune` | Octave Detune | 0–20 cents | 5 cents | Manufacturing variation in octave tube. |
| `ankl_fifth_level` | Fifth Level | 0.0–1.0 | 0.0 | Third tube (fifth) blend. |
| `ankl_tube_decay` | Tube Decay | 0.2–2.0 s | 0.8 | Air column resonance decay time. |
| `ankl_knock_level` | Knock Level | 0.0–1.0 | 0.5 | Impact transient vs. pitched resonance blend. |
| `ankl_ensemble_size` | Ensemble Size | 1–5 | 1 | Number of layered frames. |
| `ankl_ensemble_spread` | Ensemble Spread | 0.0–1.0 | 0.3 | Detuning spread across ensemble. |
| `ankl_warmth` | Warmth | 0.0–1.0 | 0.5 | Low-pass character of bamboo material. |

### 6.5 Coupling Potential with Existing XOlokun Engines

- **ONSET**: Angklung's rhythmic rattle character pairs naturally with ONSET's percussion voices. Coupling axis: ONSET provides the rhythmic foundation while angklung adds melodic percussion color.
- **OLE (Afro-Latin trio)**: Angklung's Indonesian origin and OLE's Afro-Latin character both belong to non-Western percussion traditions — a "Global South Percussion" coupling with strong ensemble character.
- **OBBLIGATO**: Angklung's inherent octave doubling and OBBLIGATO's dual-wind structure both explore compound voicing — coupling creates layered texture with natural octave reinforcement.
- **ORGANISM (V2)**: Cellular automata driving angklung rattle patterns creates algorithmic gamelan — Sundanese music has deep mathematical structure.

### 6.6 XPN Velocity Mapping Strategy

Velocity maps to **shake force and rattle complexity**:

- **Velocity 1–30**: Gentle single tap — single impact, minimal rattle spread, full tube decay
- **Velocity 31–65**: Standard shake — double impact with natural spread, balanced knock/resonance mix
- **Velocity 66–95**: Vigorous shake — triple impact, knock level increases, slight increase in detuning (larger vibration amplitude)
- **Velocity 96–127**: Hard ensemble shake — maximum knock, ensemble layers activated (if ensemble size > 1), shorter decay, fifth level subtly introduced

---

## 7. Sitar — North Indian Plucked Lute

### 7.1 Physical Acoustic Mechanism

The sitar is one of the most acoustically complex plucked string instruments in the world. Its core structure: 6–7 main playing strings stretched over a long neck with 17–20 curved (movable) frets, a large gourd body as the primary resonator, and 11–13 sympathetic resonance strings (taraf) running beneath the main strings, not played directly but resonating freely when the main strings vibrate at their tuned frequencies.

The jawari bridge is the defining acoustic element: a curved, slightly convex bridge surface that the main strings just barely graze at rest. When a string vibrates, its displacement causes it to periodically make and break contact with the curved bridge surface, creating a sustained "buzz" or "shimmer" alongside the fundamental pitch. This jawari effect is carefully crafted by instrument makers and is the sitar's most recognizable characteristic. The jawari creates a rich spectrum of inharmonic overtones by distorting the string's natural decay.

The curved frets allow "meend" technique — pressing the string sideways across the curved fret raises the pitch continuously. This is not vibrato (which alternates pitch around center); meend is a deliberate pitch glide from one note to another, sometimes spanning a major third or more from a single fret position.

The sympathetic taraf strings are tuned to the notes of the raga being performed. When main strings are struck, the taraf strings vibrate in sympathy (acoustic resonance), creating a halo of pitch-matched resonance that gives sitar its characteristic "cloud of sound" quality even between notes.

### 7.2 Key Timbre Characteristics

- **Attack transient**: Crisp, defined pluck with the jawari buzz appearing almost instantly after the fundamental onset. The attack is dual-character: the clean pluck transient (< 5ms) followed by the buzzing sustain onset (5–20ms).
- **Sustain character**: Long, complex sustain defined by the jawari buzz and sympathetic string resonance. The taraf strings create a continuing halo of sympathetic sound that seems to extend the note beyond the main string's own decay. Overall envelope: sharp attack → buzzy sustain → long sympathetic tail.
- **Harmonic content**: Extremely rich due to jawari. The normally clean harmonic series of a plucked string is augmented by a dense cloud of inharmonic partials created by the bridge contact. The sympathetic strings add pitched but slightly detuned octaves/fifths/thirds from the scale, creating a dense spectral cluster.
- **Release**: Main string decays in 2–4s. Sympathetic strings continue ringing for 3–8s after the main string stops. This "halo" continuing after note release is the sitar's most important envelope characteristic.

### 7.3 DSP Modeling Approach (JUCE C++)

**Primary synthesis: KS string with jawari comb filter**

The jawari buzz is modeled as a feedback comb filter applied to the KS string output. The comb delay is tuned to create the characteristic inharmonic buzz:

```cpp
// Jawari bridge: comb filter that "buzzes" the string
float jawariDelay = currentStringPeriod * 0.017f; // ~1.7% of string period (empirically tuned)
float jawariGain = 0.85f; // feedback gain approaching but not reaching instability

// Apply asymmetric soft clipping: string only contacts bridge in one direction
float buzzCandidate = inputSample + jawariGain * delayLine.getDelayedSample(jawariDelay);
float output = (buzzCandidate > 0.0f) ? buzzCandidate : inputSample * 0.95f; // half-wave asymmetry
```

The key insight: jawari is asymmetric (the string contacts the bridge surface in one direction only). Model this with asymmetric soft clipping or half-wave processing of the comb output.

**Sympathetic string bank: 13 KS resonators in passive mode**

13 KS string resonators tuned to the raga scale, each with a very small coupling factor (they receive energy from the main string, not from direct excitation):

```cpp
// Sympathetic string coupling: passive resonance only
const float sympatheticCoupling = 0.003f; // very small
for (int i = 0; i < 13; i++) {
    sympatheticStrings[i].inject(mainStringOutput * sympatheticCoupling);
    sympatheticOutput += sympatheticStrings[i].process();
}
```

Each sympathetic string has its own decay time and detuning (±10 cents from ideal), mimicking the slight tuning imprecision of real taraf strings.

**Meend: Portamento on KS delay length**

```cpp
// Meend: smooth glide controller
float targetPeriod = sampleRate / targetFrequency;
currentPeriod += (targetPeriod - currentPeriod) * meendRate; // exponential smoothing
```

Expose meend range (0–4 semitones), meend speed, and meend curve (linear vs. exponential).

**Raga scale library**: Pre-tune sympathetic strings to common ragas (Yaman, Bhairav, Bhairavi, Kafi, Khamaj). A `sitar_raga` parameter selects the sympathetic string tuning set.

### 7.4 JUCE APVTS Parameters

| Parameter ID | Name | Range | Default | Notes |
|---|---|---|---|---|
| `sitar_pluck_position` | Pluck Position | 0.05–0.4 | 0.1 | Tone control, harmonic content. |
| `sitar_jawari_depth` | Jawari Depth | 0.0–1.0 | 0.6 | Comb filter buzz intensity. |
| `sitar_jawari_buzz` | Jawari Buzz | 0.0–1.0 | 0.5 | Buzz asymmetry (inharmonic density). |
| `sitar_sympathetic_level` | Sympathetic Level | 0.0–1.0 | 0.5 | Taraf string resonance volume. |
| `sitar_sympathetic_decay` | Sympathetic Decay | 1.0–10.0 s | 5.0 | Taraf string sustain time. |
| `sitar_meend_range` | Meend Range | 0.0–4.0 st | 2.0 | Maximum pitch glide range. |
| `sitar_meend_speed` | Meend Speed | 10–500 ms | 100 ms | Glide time per semitone. |
| `sitar_raga` | Raga | Yaman/Bhairav/Bhairavi/Kafi/Free | Yaman | Sympathetic string tuning. |
| `sitar_string_decay` | String Decay | 1.0–5.0 s | 2.5 | Main string T60. |
| `sitar_body_warmth` | Body Warmth | 0.0–1.0 | 0.5 | Gourd body resonance. |

### 7.5 Coupling Potential with Existing XOlokun Engines

- **OPAL**: The sympathetic string halo feeds naturally into granular processing — OPAL's grain clouds can extend the taraf resonance into infinite sustain. This is the highest-value coupling in this document.
- **OVERLAP (FDN Reverb)**: The sitar's natural acoustic space (Indian classical concert halls) is long and reverberant. OVERLAP extends the sympathetic resonance appropriately.
- **OUTWIT (CA Synthesis)**: 8-arm cellular automata driving sitar's sympathetic string tuning creates a generative raga system — the sympathetic bank's tuning evolves algorithmically. High concept, high reward.
- **OHM**: Just intonation drone + sitar raga tuning — this is musically correct and deeply resonant. Indian classical music is built on drone + scale relationships.

### 7.6 XPN Velocity Mapping Strategy

Velocity maps to **pluck intensity, jawari depth, and sympathetic activation**:

- **Velocity 1–25**: Very soft pluck (da ra technique) — minimal jawari, sympathetic strings gently activated, long sustain, high pluck position
- **Velocity 26–60**: Standard playing — balanced jawari, full sympathetic resonance, standard decay
- **Velocity 61–90**: Strong pluck — jawari depth increases (buzz "blooms" more aggressively), sympathetic level peaks, pluck position slightly lowered (brighter)
- **Velocity 91–127**: Forte stroke — maximum jawari buzz at onset, all sympathetic strings ring, slight meend-like pitch inflection at attack onset (string bends slightly under high pluck force), faster initial decay

The sympathetic strings always ring regardless of velocity — they are velocity-modulated in level but never fully silent. This is acoustically accurate: the taraf strings always resonate freely.

---

## 8. Shamisen — Japanese 3-String Plucked Lute

### 8.1 Physical Acoustic Mechanism

The shamisen is a three-string fretless lute with a square body (dou) covered on both faces with animal skin (traditionally dog or cat skin, now often synthetic). The neck is long and relatively thin. Three silk strings (or nylon/tetron modern equivalents) are played with a large flat plectrum (bachi) held in the right hand.

The skin membrane resonator distinguishes the shamisen from other lutes. The stretched skin responds to string vibration transmitted through the bridge (koma), which sits directly on the face skin. The skin acts as a 2D membrane oscillator with its own modal patterns (circular membrane modes), contributing to the shamisen's specific timbre. Different body styles (futozao, chuzao, hosozao — thick/medium/thin neck) have different skin tension and koma heights, producing distinctly different sounds.

The sawari mechanism is the key timbral signature: the first string (ichi-no-ito) rests against a specially shaped nut (sawari-yama) at the pegbox, which creates a deliberate buzz on that string when played at open pitch. This buzz is analogous to the sitar's jawari. The term "sawari" means "touch/contact" and describes the string's deliberate grazing contact with the nut.

The bachi (plectrum) is large and flat, producing a distinctive "scratch-thwack" transient that is one of the most recognizable sounds in Japanese music — a broad attack including both string excitation and skin membrane impact, as the bachi often strikes the skin face directly as part of technique.

### 8.2 Key Timbre Characteristics

- **Attack transient**: The bachi attack is sharp and broad — a combination of the string pluck transient and the bachi-to-skin contact sound. The attack has 2–3ms of broadband noise before the pitched string onset.
- **Sustain character**: Moderate sustain (0.5–1.5s) with the sawari buzz prominent on the first string. The skin membrane contributes a "papery," slightly metallic character to the mid-range sustain. The body is smaller than an oud or sitar — less resonant, more direct.
- **Harmonic content**: Moderate harmonic content. Fundamental strong, harmonics 2–4 present, rapid rolloff above harmonic 6. The sawari adds inharmonic partials on the first string. The skin membrane contributes its own formant peaks (circular membrane modes at ~1.6x, 2.1x, 2.65x, 3.16x the fundamental membrane frequency).
- **Release**: Relatively fast decay. Players use right-hand muting or bachi lifting for articulation.

### 8.3 DSP Modeling Approach (JUCE C++)

**Primary synthesis: KS string + circular membrane modal synthesis**

Three KS string voices. The body resonance is modeled using circular membrane modal synthesis:

```cpp
// Circular membrane modes (J_mn Bessel function zeros normalized to fundamental)
// Mode ratios: 1.0, 1.593, 2.136, 2.295, 2.653, 2.917, 3.155...
static const float membraneRatios[] = { 1.000f, 1.593f, 2.136f, 2.295f, 2.653f, 2.917f };
static const float membraneAmps[]   = { 1.000f, 0.350f, 0.200f, 0.150f, 0.100f, 0.070f };
// Membrane modes decay ~5x faster than string
float membraneModeDecay = stringDecay * 0.2f;
```

**Sawari buzz: Detuned parallel string**

The sawari is modeled as a second KS string for the first string, detuned by 3–8 cents from the main string, mixed at a low level:

```cpp
// Sawari: parallel slightly-detuned KS string on string 1
float sawariDetune = 5.0f; // cents
float sawariPeriod = stringPeriod * std::pow(2.0f, -sawariDetune / 1200.0f);
// sawari KS runs in parallel, mixed at sawariLevel
float mixedString1 = mainString1 + sawariLevel * sawariKS.process(excitation);
```

This creates a beating interference pattern between the two slightly detuned strings — the same physical mechanism as real sawari buzz.

**Bachi attack transient**: The bachi strike has a specific broad, scratchy character. Model as a short noise burst (5–8ms) LP filtered at 3–5 kHz blended with the KS excitation:

```cpp
float bachiNoise = noiseGenerator.nextSample() * attackEnvelope.process();
float lpNoise = bachiLPFilter.process(bachiNoise); // 3kHz LP for scratch character
float excitation = lpNoise * bachiLevel + pluckImpulse * (1.0f - bachiLevel);
```

**Skin membrane coupling**: A 6-mode parallel resonator bank representing the circular membrane modes, applied as a post-processing stage on the combined string output.

### 8.4 JUCE APVTS Parameters

| Parameter ID | Name | Range | Default | Notes |
|---|---|---|---|---|
| `sham_bachi_force` | Bachi Force | 0.0–1.0 | 0.5 | Strike velocity, transient level. |
| `sham_bachi_scratch` | Bachi Scratch | 0.0–1.0 | 0.4 | Noise-to-pluck ratio in excitation. |
| `sham_sawari_level` | Sawari Level | 0.0–1.0 | 0.4 | Buzz depth on first string. |
| `sham_sawari_detune` | Sawari Detune | 1–15 cents | 5 cents | Buzz string detuning interval. |
| `sham_membrane_level` | Membrane Level | 0.0–1.0 | 0.45 | Skin membrane resonance depth. |
| `sham_membrane_tension` | Membrane Tension | 0.0–1.0 | 0.5 | Skin tightness (shifts membrane modes). |
| `sham_string_decay` | String Decay | 0.2–2.0 s | 0.7 | String T60. |
| `sham_body_style` | Body Style | Futozao/Chuzao/Hosozao | Chuzao | Neck/body style preset. |
| `sham_portamento` | Portamento | 0–200 ms | 0 | Fretless glide. |
| `sham_string_spacing` | String Spacing | 0.0–1.0 | 0.5 | Chord voicing spread (all 3 strings). |

### 8.5 Coupling Potential with Existing XOlokun Engines

- **ONSET**: The shamisen's percussive bachi attack and ONSET's drum voices create a hybrid drum-melodic instrument. Coupling axis: ONSET handles kick/snare, shamisen handles melodic hits.
- **OUTWIT (CA Synthesis)**: Cellular automata patterns driving shamisen string triggering creates algorithmic koto-like compositions — a generative Japanese music system.
- **OVERDUB (Tape Delay)**: Shamisen + tape delay evokes Japanese experimental music (Takemitsu, Yuasa). The organic decay of the shamisen is enhanced by OVERDUB's warm tape echo.
- **OPAL**: Granular processing of shamisen skin membrane tone creates unusual textural pads — the membrane's inharmonic modal content yields interesting grain material.

### 8.6 XPN Velocity Mapping Strategy

Velocity maps to **bachi force, sawari intensity, and membrane activation**:

- **Velocity 1–30**: Soft finger pluck (no bachi contact with skin) — minimal scratch, sawari subtle, membrane level low, long decay
- **Velocity 31–65**: Standard bachi technique — bachi scratch prominent, sawari level moderate, membrane activated, standard decay
- **Velocity 66–90**: Strong bachi hit — maximum scratch, sawari at peak buzz depth, membrane loudly activated, bachi-contact skin component audible
- **Velocity 91–127**: Forte strike — full bachi attack with skin impact, sawari may slightly overdrive (a known performance artifact on loud shamisen), membrane rings loudly before settling

On the sawari specifically: `sham_sawari_level` scales from 0.15 at velocity 1 to 0.65 at velocity 127 — the buzz is always present on string 1 but grows in prominence with force.

---

## Priority Engines for V2 Collections

The following ranking evaluates each instrument across two axes: **DSP complexity** (total engineering effort, novel algorithm requirements) and **XOlokun coupling potential** (how well the engine's character meshes with existing engines to produce valuable new textures).

| Rank | Instrument | DSP Complexity | Coupling Potential | Recommend First? |
|---|---|---|---|---|
| 1 | **Sitar** | High | Very High | YES |
| 2 | **Handpan** | Medium-High | Very High | YES |
| 3 | **Erhu** | High | High | YES |
| 4 | **Guzheng** | Medium | High | — |
| 5 | **Shamisen** | Medium | Medium-High | — |
| 6 | **Didgeridoo** | Medium-High | Medium | — |
| 7 | **Oud** | Medium | Medium-High | — |
| 8 | **Angklung** | Low-Medium | Medium | — |

### Recommendation: Build Sitar, Handpan, and Erhu First

**Sitar** is the highest-priority engine. The sympathetic string bank is a unique DSP architecture that no existing XOlokun engine replicates. Its coupling with OPAL (granular sympathetic extension), OHM (drone + raga), and OUTWIT (generative raga via cellular automata) is extremely high-value. The jawari comb filter and meend portamento are novel algorithms that will differentiate the engine strongly. Sitar also anchors the Artwork Collection's Color Quad A (Erhu / Didgeridoo / Oud / Guzheng cluster), suggesting a V2 South/East Asian instrument release cohort.

**Handpan** is the second priority. Modal synthesis of inharmonic partials is an underrepresented algorithm in XOlokun — most existing engines use waveguides or subtractive synthesis. The handpan engine would be the first true modal synthesis engine in the fleet, opening a new DSP category. Its coupling with OVERLAP (FDN metallic diffusion) is near-perfect, and the instrument has massive contemporary appeal in ambient and electronic music. The relatively contained parameter set means faster development despite the novel algorithm. The `hpan_maker_style` preset system also creates a natural preset differentiation axis (PANArt vs. Saraz vs. free-tuned).

**Erhu** is third. The Hiller-Ruiz bowing physics model is the most complex single algorithm in this document, but the erhu's coupling potential with OPAL, OVERDUB, and OVERLAP makes it a high-return investment. The erhu's vocal expressiveness and continuous timbral trajectory will enable expressive melodic playing that no current XOlokun engine provides — this is a genuine gap in the fleet's voice roster. It anchors the "bowed string" voice archetype required by the Collections, and its snakeskin membrane formant bank connects architecturally with OBBLIGATO's wind formant modeling.

**Deferred to V3 consideration**: Didgeridoo (the 10-12 pole LPC vocal tract model is the second-most complex algorithm in this set; better to tackle after bowing physics is solved in Erhu) and Angklung (lowest coupling potential and narrowest appeal in isolation — a stronger future option is a single "Bamboo Ensemble" engine combining angklung, bamboo flute, and bamboo percussion into one architecture, maximizing the material's character without spending a full engine slot on rattle alone).

---

*Document version 1.0 — 2026-03-16. Prepared for XO_OX Collections V2 planning.*
*Cross-reference: `Docs/concepts/artwork_collection_overview.md`, `Docs/concepts/travel_water_collection_overview.md`, open-engines-status in project memory.*
