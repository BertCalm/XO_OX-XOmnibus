# Collections Build Preparation Research

*March 2026 | 68 engines across 3 collections | V2 Paid Expansion Packs for XOmnibus*

---

## Table of Contents

1. [Shared Architecture Research](#1-shared-architecture-research)
2. [Collection 1: Kitchen Essentials (24 engines)](#2-collection-1-kitchen-essentials)
3. [Collection 2: Travel / Water / Vessels (20 engines)](#3-collection-2-travel--water--vessels)
4. [Collection 3: Artwork / Color (24 engines)](#4-collection-3-artwork--color)
5. [Cross-Collection Concerns](#5-cross-collection-concerns)
6. [Build Sequence Recommendation](#6-build-sequence-recommendation)

---

## 1. Shared Architecture Research

### 1.1 The Engine Adapter Pattern

Every XOmnibus engine implements the `SynthEngine` interface (`Source/Core/SynthEngine.h`). The contract:

```
prepare(sampleRate, maxBlockSize)   -- allocate buffers
renderBlock(buffer, midi, numSamples) -- RT-safe audio render
getSampleForCoupling(ch, idx)       -- O(1) cached sample return
applyCouplingInput(type, amt, buf, n) -- receive cross-engine mod
createParameterLayout()             -- namespaced param IDs
attachParameters(apvts)             -- cache raw param pointers
getEngineId() / getAccentColour() / getMaxVoices()
```

**Observed patterns from existing engines:**

- **ObbligatoEngine** (dual wind physical model): All DSP inline in `.h`. Voice struct with `prepare/reset/noteOn/noteOff`. Parameters read once per block via `pParamName->load()`. BOND macro uses an 8-stage interpolation table for emotional modulation. FX chains are per-engine (not shared). ~40 params.

- **OpalEngine** (granular): 86 frozen params with `opal_` prefix. Param IDs defined as `inline constexpr const char*` in a namespace block. xorshift32 PRNG for RT-safe randomization. Uses shared DSP modules (`CytomicSVF`, `PolyBLEP`, `FastMath`). 6-slot mod matrix.

- **ObscuraEngine** (scanned synthesis): 128-mass spring chain via Verlet integration. Control-rate physics (~4kHz) with interpolation to audio rate. Detailed architecture comment block at top of file. Inline ADSR struct.

**Key takeaway for collections:** Each engine is self-contained in `Source/Engines/{ShortName}/`. DSP lives entirely in `.h` headers. A `.cpp` stub contains only the `REGISTER_ENGINE` macro. No shared state between engines except through the coupling matrix.

### 1.2 Engine Registration Process

1. Create `Source/Engines/{ShortName}/{ShortName}Engine.h` (all DSP inline)
2. Create `Source/Engines/{ShortName}/{ShortName}Engine.cpp` containing only:
   ```cpp
   #include "{ShortName}Engine.h"
   REGISTER_ENGINE({ShortName}Engine)
   ```
3. Add source files to `CMakeLists.txt`
4. Engine appears in `EngineRegistry` automatically at static init time
5. Copy `.xometa` presets to `Presets/XOmnibus/{mood}/`
6. Run `Tools/compute_preset_dna.py` for Sonic DNA fingerprinting
7. Update `CLAUDE.md` engine table, master spec, and design system docs

**For collections (V2 paid packs):** The registration mechanism is identical. The only difference is distribution -- collection engines would be conditionally compiled or loaded via a license/unlock mechanism (TBD).

### 1.3 Parameter ID Budget

Based on existing engines:
- **Opal**: 86 params (granular, the largest)
- **Onset**: 111 params (8-voice percussion, the absolute max)
- **Obbligato**: ~40 params (physical model)
- **Obscura**: ~45 params (scanned synthesis)

The APVTS merges all engine layouts. With 34 engines currently registered, that is approximately **34 x ~60 avg = ~2,040 params** already in the tree. Adding 68 more engines at ~50 params each adds ~3,400 params.

**Performance concern:** JUCE's `AudioProcessorValueTreeState` stores params in a flat `std::vector` with string-keyed lookup via `std::unordered_map`. The cached pointer pattern (read raw `std::atomic<float>*` per block) means runtime cost is O(1) per read. The cost is at construction time and memory footprint only.

**Recommendation:** Keep collection engines to **40-60 params each**. The 3-axis Voice/FX/Wildcard system means many "configurations" are achieved via param values, not additional params. Target 50 params per engine as the sweet spot.

### 1.4 The 5th Slot Architecture

The current `XOmnibusProcessor` has `MaxSlots = 4` with a fixed-size array:
```cpp
std::array<std::shared_ptr<SynthEngine>, MaxSlots> engines;
std::array<CrossfadeState, MaxSlots> crossfades;
std::array<juce::AudioBuffer<float>, MaxSlots> engineBuffers;
std::array<juce::MidiBuffer, MaxSlots> slotMidi;
```

**To enable a 5th slot:**
1. Change `MaxSlots` to 5 (trivial -- all arrays resize at compile time)
2. Update `MegaCouplingMatrix` to handle 5-slot routing
3. Update `ChordMachine` to distribute MIDI to 5 slots
4. Update UI gallery layout to accommodate a 5th tile
5. Add unlock logic: detect when slots 1-4 contain one engine from each quad/set, then enable slot 5

**Risk assessment:** Low. The slot system is array-based with a compile-time constant. Changing `MaxSlots = 5` propagates everywhere automatically. The coupling matrix already uses slot indices, so adding a 5th slot just extends the routing table. CPU budget becomes the real constraint -- 5 engines at 28% each is 140%, which exceeds a single core.

**Recommendation:** The 5th slot should impose constraints: either (a) the Fusion/Sable Island engine is lightweight (<10% CPU), or (b) the 5th slot reduces the other 4 engines to a "thinned" voice count. Alternatively, the 5th slot could be a special mode that replaces 2 of the 4 slots with a single fusion engine, keeping the total at 4 active engines.

### 1.5 Preset System for Collections

Presets use `.xometa` JSON format:
```json
{
  "schema_version": 1,
  "name": "Preset Name",
  "mood": "Foundation",
  "engines": ["EngineId"],
  "parameters": { "EngineId": { "prefix_param": value } },
  "macroLabels": ["M1", "M2", "M3", "M4"],
  "coupling": { "pairs": [] }
}
```

Collection presets would include:
- **Single-engine presets**: Standard per-engine presets (150+ per engine target)
- **Intra-quad coupling presets**: e.g., two Kitchen organs coupled (Entangled mood)
- **Cross-collection coupling presets**: e.g., Kitchen organ + Travel brass (if both collections owned)

### 1.6 Shared DSP Library Available

Existing modules in `Source/DSP/` available to all engines:
- `CytomicSVF.h` -- state-variable filter (LP/HP/BP/Notch)
- `PolyBLEP.h` -- anti-aliased oscillators
- `ComplexOscillator.h` -- FM/waveshaping oscillator
- `WavetableOscillator.h` + `WavetableScanMode.h` -- wavetable playback
- `FamilyWaveguide.h` -- physical modeling primitives (delay lines, damping, body resonance, sympathetic strings, air jet/reed exciters, organic drift)
- `FastMath.h` -- fast approximations for sin/cos/exp/tanh
- `Effects/` -- shared FX modules
- `ShoreSystem/` -- 5-coastline cultural data system (used by Osprey/Osteria)
- `EngineProfiler.h` -- CPU profiling

**Critical for collections:** The `FamilyWaveguide.h` module already contains physical modeling primitives (delay lines, reed exciters, air jet models, body resonance, sympathetic string banks). Kitchen and Travel engines can reuse these extensively.

---

## 2. Collection 1: Kitchen Essentials

### 2.1 Multi-Sample Synthesis Best Practices by Instrument Family

#### Organ (CHEF Quad)
- **Synthesis approach:** Additive synthesis (drawbar model) is superior to sampling for organs. Each drawbar is a sine harmonic at a fixed ratio. Leslie speaker simulation via amplitude/frequency modulation.
- **Key considerations:** 9 drawbar harmonics (sub-fundamental through 8th), percussion (2nd/3rd harmonic click), key click transient, vibrato/chorus scanner. No velocity layers needed (organs are velocity-insensitive). Rotary speaker sim is the defining FX.
- **DSP approach:** Pure synthesis. No samples needed. Additive oscillator bank + rotary speaker model (dual horn: treble rotor at 40/340 RPM, bass rotor at 40/80 RPM with crossover at ~800Hz).

#### Piano (KITCHEN Quad)
- **Synthesis approach:** Hybrid physical model + synthesis. Full sampling is impractical in a synth engine. Use waveguide string models excited by filtered noise bursts (hammer model).
- **Key zones:** Minimum 4 zones (bass, tenor, alto, treble) with crossfades. Real pianos have ~88 unique timbres, but 12-16 modeled zones with interpolation is sufficient.
- **Velocity layers:** 4 minimum (pp, mp, mf, ff). Velocity should control hammer hardness (brightness), not just amplitude. Soft = more fundamental, loud = more upper partials.
- **Sustain pedal:** Must model sympathetic resonance -- all undamped strings resonate when pedal is held. This is the piano's "reverb."
- **Round robin:** Not critical for piano (unlike drums). Instead, model micro-variation via slight detuning randomization per note-on.
- **Release samples:** Model the damper contact as a short filtered noise burst on note-off.

#### Electric Piano (FUSION Quad)
- **Synthesis approach:** FM synthesis for Rhodes/Wurlitzer character. The Rhodes sound is fundamentally a 2-operator FM patch (modulator:carrier ratio near 1:1 with velocity-dependent modulation index).
- **Key zones:** 3-4 zones. Bass notes have more fundamental, treble notes have more "bell" character.
- **Velocity:** Modulation index scales with velocity -- soft = pure sine, hard = rich overtones with "bark."
- **Tremolo/vibrato:** Essential. Wurlitzer tremolo is amplitude modulation; Rhodes has subtle vibrato.
- **FX dependency:** Chorus, phaser, and overdrive are integral to the electric piano sound. The Juno-60 chorus on a Rhodes is arguably more important than the Rhodes itself.

#### Strings (GARDEN Quad)
- **Synthesis approach:** Physical modeling (bowed string waveguide) or wavetable with formant shaping.
- **Key zones:** Violin (G3-E7), Viola (C3-E6), Cello (C2-A5), Bass (E1-G4). Each instrument has distinct formant regions.
- **Velocity layers:** 3+ layers mapping to bow pressure (Sul tasto = gentle, normale, Sul ponticello = aggressive).
- **Playing techniques that matter:** Legato transitions (portamento), pizzicato (plucked), tremolo, spiccato (bouncing bow). Each is a different excitation model.
- **Ensemble modeling:** Multiple detuned voices with slight timing offsets create section sound. 4-6 voices per "player," 4-16 "players" per section.
- **Release:** Strings have a natural decay that varies with bow pressure and position.

#### Bass (CELLAR Quad)
- **Synthesis approach:** Hybrid subtractive + waveguide. Electric bass = plucked string waveguide with pickup model. Synth bass = standard subtractive.
- **Key zones:** 2-3 zones (low E1-A1 has fundamentally different character than higher register).
- **Velocity:** Controls pluck intensity / filter envelope depth. Ghost notes (very soft) are musically important.
- **Round robin:** 2-4 round robins critical for fingered bass (repeated note realism).
- **Techniques:** Fingered, picked, slapped, muted -- each is a different excitation + damping model.
- **Sub-bass:** Dedicated sub-oscillator (sine or triangle) common below ~80Hz.

#### Pads (BROTH Quad)
- **Synthesis approach:** Wavetable, granular, or multi-oscillator with slow modulation.
- **No key zones needed** -- pads are typically pitch-invariant in timbre.
- **No velocity layers** -- pad dynamics come from envelope and filter, not sample selection.
- **Slow envelopes:** Attack 0.5-5s, release 2-10s. The envelope IS the sound.
- **Modulation density:** Multiple LFOs at different rates (0.01-2 Hz) creating evolving texture.
- **Stereo width:** Critical. Chorus, unison detuning, and stereo spread define pad character.

### 2.2 JUCE Implementation Patterns for Kitchen Engines

Based on existing XOmnibus engines, each Kitchen engine should follow this structure:

```
Source/Engines/{Name}/
  {Name}Engine.h      -- All DSP inline (~500-1500 lines)
  {Name}Engine.cpp    -- REGISTER_ENGINE macro only
```

**Voice architecture per quad:**

| Quad | Voice Count | Stealing | Rationale |
|------|------------|----------|-----------|
| CHEF (Organ) | 12-16 poly | None (infinite sustain) | Organs need full polyphony |
| KITCHEN (Piano) | 8 poly | Oldest | Piano needs sustain overlap |
| FUSION (EP) | 8 poly | Oldest | Similar to piano |
| GARDEN (Strings) | 6 poly (x4 unison per voice) | Oldest | Section modeling |
| CELLAR (Bass) | 4 poly | Oldest | Bass is typically monophonic |
| BROTH (Pads) | 6 poly (x4 unison) | Quietest | Pads need voice layering |

**3-axis param design (Voice x FX Recipe x Wildcard):**
- `{prefix}_voice` (0-3): Selects instrument variant within the quad
- `{prefix}_fxRecipe` (0-3): Selects culinary FX chain
- `{prefix}_wildcard` (0-3): Selects boutique synth "spice"
- These 3 params give 64 configurations per engine. The DSP reads these to branch behavior.

### 2.3 Boutique Synth Company Research -- The Wildcard Axis

Each Kitchen engine's wildcard axis is flavored by a boutique synth maker's character. Here is how each maker's philosophy translates to DSP:

#### Moog (East Coast Patriarch)
- **Sonic signature:** Rich, fat, warm. The Moog ladder filter (24dB/oct LP with resonance) is the defining circuit.
- **DSP translation:** Cascade of 4 one-pole filters with resonance feedback. Emphasize low-end warmth, slight saturation at resonance. Oscillators with subtle analog drift (random walk on pitch, +-5 cents).
- **Apply to Kitchen as:** "Comfort food" -- everything through a warm Moog filter becomes richer, fatter, more indulgent.

#### Buchla (West Coast Pioneer)
- **Sonic signature:** Complex, evolving, alien. Waveshaping and FM over filtering. The Low-Pass Gate (LPG) -- a vactrol-based combined filter/VCA that creates organic "pluck" transients.
- **DSP translation:** Wavefolder (Chebyshev polynomials), ring modulation, voltage-controlled waveshaping. LPG model: coupled LP filter + VCA with slow vactrol response (10ms attack, 100ms+ release).
- **Apply to Kitchen as:** "Molecular gastronomy" -- familiar instruments deconstructed through waveshaping into something unexpected.

#### Make Noise (Modern West Coast)
- **Sonic signature:** Chaotic, textural, patch-programmable. Stochastic modulation, self-patching feedback.
- **DSP translation:** Random walk modulators, sample-and-hold with slew, feedback paths with soft limiting. The "Wogglebug" concept: filtered noise as a modulation source with adjustable smoothness.
- **Apply to Kitchen as:** "Fermentation" -- controlled chaos that transforms the base ingredient over time.

#### Teenage Engineering (Nordic Design)
- **Sonic signature:** Lo-fi charm, constraint as aesthetic, playful immediacy. The OP-1's tape machine, the Pocket Operator's 8-bit crunch.
- **DSP translation:** Bit crushing, sample rate reduction, tape wow/flutter emulation, quantized pitch steps. Deliberate "imperfection" processing.
- **Apply to Kitchen as:** "Street food" -- rough, immediate, charming. Lo-fi processing on high-fidelity instruments.

#### Elektron (Swedish Precision)
- **Sonic signature:** Surgical, sequenced, parameter-locked. Real-time parameter recording per step.
- **DSP translation:** Per-step parameter modulation (p-locks), probability-based parameter variation, conditional trigs (1:2, 3:4 patterns).
- **Apply to Kitchen as:** "Precision plating" -- exact, measured, technically perfect. Step-sequenced parameter changes create rhythmic instrument variations.

#### Sequential / Dave Smith (American Polysynth)
- **Sonic signature:** Lush unison, polymod (oscillator cross-modulation), Curtis filter warmth.
- **DSP translation:** Thick unison detuning (4-8 voices with spread), oscillator sync, cross-modulation (Osc A modulating Osc B pitch/pulse width).
- **Apply to Kitchen as:** "Layer cake" -- stacked, rich, each layer contributing to a unified whole.

**Recommendation:** Assign 4 of these 6 makers to each quad's wildcard slots. Different quads can use different subsets, ensuring variety across the collection.

---

## 3. Collection 2: Travel / Water / Vessels

### 3.1 Genre FX Chains -- Canonical Signal Paths

#### Hip Hop Eras (SAIL set)

| Era | Canonical Gear | DSP Chain |
|-----|---------------|-----------|
| **Old School (1979-86)** | Roland TR-808, LinnDrum, DMX, basic 2-track | Drum machine saturation -> tight room verb (< 0.5s) -> mono sum below 300Hz -> high-cut at 10kHz |
| **Boom Bap (1986-96)** | Akai MPC60/SP-1200, vinyl, SSL console | 12-bit sample degradation -> vinyl noise + wow -> sidechain comp (ghost kick) -> lo-pass at 12kHz -> short room |
| **New Jack Swing (1987-95)** | TR-808 + LinnDrum + DX7, gated reverb | FM bell synthesis layer -> gated reverb (snare) -> wide stereo chorus -> bright shelf EQ (+3dB >8kHz) |
| **Trap (2012+)** | 808 sub-bass, hi-hat machines, dark reverb | Hard clipper -> aggressive sidechain -> hall reverb (dark, 3s+) -> stereo widener -> sub boost below 60Hz |

#### Dance Music Eras (INDUSTRIAL set)

| Era | Canonical Gear | DSP Chain |
|-----|---------------|-----------|
| **Disco (1974-82)** | Nile Rodgers' guitar, Tom Moulton edits, strings | Auto-phaser -> studio plate reverb (2s) -> light tape comp (4:1) -> stereo auto-pan (synced) |
| **House (1984-95)** | Roland TR-909, TB-303, Frankie Knuckles | Sidechain pump (4-on-floor, 200ms release) -> warm saturation -> spring reverb -> HPF 100Hz |
| **Jungle/DnB (1991-98)** | Amen break, Reese bass, timestretching | Timestretch artifacts -> asymmetric distortion -> sub-bass layer (sine at -1 oct) -> rapid auto-pan |
| **Trance (1998-2006)** | Supersaw (JP-8000), sidechain, shimmer | Supersaw chorus (7-voice unison detune) -> sidechain pump (long release, 400ms) -> shimmer verb -> hi-shelf +4dB |

#### Island Music Eras (LEISURE set)

| Era | Canonical Gear | DSP Chain |
|-----|---------------|-----------|
| **Roots/Dub (1968-85)** | King Tubby's studio, RE-201 Space Echo | Spring reverb (long, dark) -> tape delay (dotted 8th, 60% FB, modulated) -> high-cut sweep -> sub-bass boost |
| **Dancehall (1985+)** | Casio MT-40 (Sleng Teng), digital riddims | Digital distortion -> short room -> synth stab layer -> hard limiter |
| **Reggaeton (2004+)** | Dem Bow riddim, Latin trap bass | Sub-harmonic synth -> wide stereo delay (ping-pong) -> bright plate reverb -> bass boost +6dB < 80Hz |
| **Pacific/Polynesian** | Log drums, slack-key guitar, nose flute | Gentle chorus (warm) -> room reverb (natural, 1.5s) -> mono sum below 200Hz -> subtle tape saturation |

#### Synth Eras (HISTORICAL set)

| Era | Canonical Gear | DSP Chain |
|-----|---------------|-----------|
| **Synthpop (1978-86)** | Juno-60, SH-101, DMX, Vince Clarke | BBD chorus (Juno stereo mode) -> warm saturation -> gentle high-cut 12kHz -> short room |
| **Chiptune (1983+)** | NES 2A03, Game Boy, SID chip | Bit-crush to 4-bit -> sample rate reduction to 22kHz -> simple 1-pole LP -> hard pan L/R |
| **Industrial (1976-95)** | Metal percussion, Einsturzende Neubauten | Heavy distortion (tube + fuzz) -> ring modulator -> short metallic reverb -> aggressive gate |
| **Electro (1982-88)** | TR-808, Kraftwerk, Egyptian Lover | Tight compression (8:1) -> short bright reverb -> stereo flanger -> high-shelf boost |

### 3.2 Instrument Physical Modeling Approaches

#### SAIL Set -- Woodwinds

| Instrument | DSP Approach | Key Characteristics |
|------------|-------------|-------------------|
| **Flute / Bansuri / Ney** | Air-jet exciter + cylindrical waveguide. Model the jet deflection angle and edge-tone generation. | Open cylinder, overblowing at octave. Breath noise is a feature, not a bug. Ney has a breathy, airy quality from the oblique embouchure. Bansuri has a warmer tone from bamboo body resonance. Formant: ~1-2kHz broad peak. |
| **Clarinet** | Single-reed model + cylindrical bore waveguide. Non-linear reed function (hyperbolic tangent). | Closed cylinder, overblows at twelfth (not octave). Rich odd harmonics. Register break at Bb4. Very agile -- fastest woodwind for runs. Chalumeau register is dark and warm. |
| **Oboe / Duduk** | Double-reed model (higher stiffness, narrower aperture) + conical bore. | Conical bore creates both odd and even harmonics. Nasal, penetrating timbre. Duduk has a wider reed and softer tone. Formant peaks around 1kHz and 3kHz give the "nasal" quality. |
| **Saxophone** | Single-reed + conical bore. Wider reed than clarinet. | Conical bore like oboe but single reed. Registers from soprano to baritone span 3+ octaves of fundamental. Subtone technique (relaxed embouchure) for jazz. Growl = vocal cord vibration modulating reed. |

**Shared DSP resources:** `FamilyWaveguide.h` already provides `AirJetExciter` and `ReedExciter` classes, plus `FamilyDelayLine`, `FamilyDampingFilter`, and `FamilyBodyResonance`. The SAIL engines can build directly on these.

#### INDUSTRIAL Set -- Brass

| Instrument | DSP Approach | Key Characteristics |
|------------|-------------|-------------------|
| **Trombone / Tuba** | Lip-reed model (mass-spring oscillator) + cylindrical-to-conical bore + bell radiation filter. | Slide position changes tube length continuously (portamento). Muting modeled as additional filter section. Tuba has very long bore -- lowest brass. Growl possible. |
| **Trumpet / Cornet** | Lip-reed + cylindrical bore + bell. 3 valves select tube length combinations. | Bright, commanding. Harmon mute (Miles Davis) is a band-pass filter at ~1.5kHz. Cornet has conical bore -- warmer than trumpet. Valve click is a transient feature. |
| **French Horn** | Lip-reed + very long conical bore (3.7m uncoiled). Hand-stopping modeled as variable mute. | Most reverberant brass due to bore length. Bell points backward -- reflected sound is the natural presentation. Hand stopping shifts pitch and filters aggressively. |
| **Flugelhorn** | Lip-reed + wide conical bore. Similar fingering to trumpet. | Widest, warmest brass tone. Fewer upper harmonics than trumpet. The "dark trumpet" -- Chet Baker, Art Farmer. |

**DSP approach:** Lip-reed models use a mass-spring system where lip vibration frequency is controlled by MIDI note. The non-linear coupling between lip oscillation and bore pressure creates the characteristic brass tone. JUCE implementation: waveguide delay line for the bore, non-linear lip model in the excitation loop, radiation filter for the bell.

#### LEISURE Set -- Island Cultural Instruments

| Instrument | DSP Approach | Key Characteristics |
|------------|-------------|-------------------|
| **Steel Pan** | Modal synthesis (Chladni pattern modes). Each note area has 4-8 modes with specific frequency ratios and decay rates. | Inharmonic spectrum -- partials are NOT harmonic series. The "singing" quality comes from closely-spaced modes beating. Velocity affects which modes are excited. Different pan types (tenor, double second, cello) have different mode spacings. |
| **Ukulele** | Karplus-Strong string model with nylon string character (heavy damping, warm fundamental). | 4 strings, re-entrant tuning (high G). Short sustain. Strumming model: sequential triggers with ~5ms per string. Slack-key: open tuning with specific string resonances. |
| **Kalimba / Mbira** | Tuned bar model -- stiff bar vibration with attached resonator (gourd or box). | Inharmonic spectrum from bar stiffness: f2 ~= 5.4 * f1 (not 2x). The buzz from bottle-cap rattles (mbira) is a noise modulation on the resonator. Portable, meditative, circular patterns. |
| **Marimba / Balafon** | Bar percussion model -- struck bar with tubular resonator tuned to fundamental. | Wide pitch range (A2-C7). Lower notes are deeply resonant (long tubes). Balafon adds gourd resonators with spider-web membranes that create a buzzing sympathetic sound. Roll technique: rapid alternating strikes. |

**DSP approach:** Modal synthesis is ideal for all four instruments. Pre-compute mode frequencies and decay rates per instrument type, excite with a short noise/impulse burst shaped by velocity. The `FamilyBodyResonance` and `FamilySympatheticBank` classes in `FamilyWaveguide.h` provide starting points.

#### HISTORICAL Set -- Historical Percussion

| Instrument | DSP Approach | Key Characteristics |
|------------|-------------|-------------------|
| **Taiko / War Drum** | Membrane model -- 2D waveguide or modal synthesis with large diameter. | Very low fundamental (30-80Hz for odaiko). Long sustain. Rim hits have a sharp, high crack. Playing dynamics from pianissimo to thunderous fortissimo. Multiple strike positions (center, off-center, rim). |
| **Frame Drum / Bodhran** | Thin membrane model with hand-damping. | Pitch-bendable by hand pressure on back of skin. Short sustain when damped, ringing when open. Bodhran uses a stick (tipper) -- double-ended for rolls. |
| **Gong / Singing Bowl** | Modal synthesis with strong inharmonic partials and very long decay. | Gong: hundreds of vibration modes, some with negative group velocity ("shimmer"). Build-up effect -- gong gets louder after initial strike before decaying. Singing bowl: rubbing excitation creates sustained tone at specific mode. |
| **Gamelan** | Interlocking modal synthesis. Pairs of instruments slightly detuned to create "ombak" (beating). | Bronze percussion with specific tuning systems (slendro: 5-tone, pelog: 7-tone). NOT 12-TET. The beating between paired instruments is the defining sonic feature. Damping by hand is rhythmically essential. |

### 3.3 Cross-Set Coupling Design

| Coupling Route | Engine Pair | Parameter Cross-Modulation | Sonic Character |
|---------------|-------------|---------------------------|-----------------|
| **Trade Wind** (Sail x Industrial) | Woodwind <-> Brass | Breath pressure -> lip tension. Air jet noise -> brass bell resonance. | The transition from sail to steam. Flute air feeding brass pressure. Gradual blend from reed to lip. |
| **Regatta** (Sail x Leisure) | Woodwind <-> Island | Woodwind pitch -> steel pan mode selection. Embouchure -> kalimba pluck brightness. | Competition becoming celebration. The clarinet's agility driving the steel pan's joyful modes. |
| **Expedition** (Sail x Historical) | Woodwind <-> Percussion | Breath rhythm -> drum trigger pattern. Drum envelope -> reed tension modulation. | The voyage. Flute melody driving rhythmic patterns on historical drums. Drum accents bending woodwind pitch. |
| **Port Call** (Industrial x Leisure) | Brass <-> Island | Brass amplitude -> island instrument tremolo rate. Island rhythm -> brass mute position. | Shore leave. Trombone dynamics controlling steel pan shimmer speed. Steel pan patterns opening and closing brass mutes. |
| **Convoy** (Industrial x Historical) | Brass <-> Percussion | Brass staccato -> drum choke/trigger. War drum amplitude -> brass dynamics (swells). | Wartime. Trumpet calls triggering taiko hits. Drum rolls driving brass crescendos. |
| **Pilgrimage** (Leisure x Historical) | Island <-> Percussion | Kalimba pattern -> gamelan interlocking. Gong swell -> island instrument reverb wet amount. | Tourism meeting history. The comfort of leisure instruments gradually absorbed by the weight of ancient percussion. |

---

## 4. Collection 3: Artwork / Color

### 4.1 World Instruments -- DSP Characteristics

#### Color Quad A (Oxblood/Onyx/Ochre/Orchid)

| Instrument | Key Acoustic Features | DSP Approach |
|------------|----------------------|-------------|
| **Erhu** (Chinese bowed) | Two strings (D4, A4). Snakeskin resonator creates pronounced formant pairs at ~400Hz and ~1500Hz that resemble human vowels. Vibrato is fast and wide (~6Hz, +-50 cents). Portamento between notes is idiomatic. | Bowed string waveguide with dual formant resonator. Excitation: rosin-stick-slip friction model. The snakeskin resonator is the defining timbral feature -- model as 2 coupled membrane-cavity resonances. Use `FamilyWaveguide.h` bowed string components. |
| **Didgeridoo** | Fundamental drone ~70Hz (fixed by tube length). Formant bands at ~500Hz and ~1500-2200Hz controlled by mouth shape. Circular breathing creates continuous drone. Overtone singing adds pitched harmonics above drone. | Waveguide tube model with variable vocal tract filter. Two coupled resonators: the tube (fixed length) and the mouth (variable volume). Lip buzz excitation. Formant filter sweeps mapped to a "vowel" parameter. Harmonics via overblowing or vocal addition. |
| **Oud** | 11-13 strings, fretless. Warm, dark tone from large round body. Ornamental techniques: tremolo (rapid plectrum), vibrato, slides between microtones. Maqam scales use quarter-tones. | Karplus-Strong with large body resonator model. Fretless pitch requires continuous pitch interpolation. Tremolo modeled as rapid re-excitation. Body resonance is large and warm (formant ~200-400Hz). Support microtonal tuning. |
| **Guzheng** | 21 strings, movable bridges. Pitch bending by pressing string beyond bridge. Rapid arpeggios, harmonics, tremolo. Bright, resonant, metallic. | Multi-string Karplus-Strong with bridge coupling. Pitch bend via string tension modulation. Sympathetic resonance between strings is prominent. Bright attack transient from nail pluck. 2500+ year instrument -- oldest in this set. |

#### Color Quad B (Ottanio/Oma'oma'o/Ostrum/Oni)

| Instrument | Key Acoustic Features | DSP Approach |
|------------|----------------------|-------------|
| **Handpan** | Steel instrument, Helmholtz resonance. Each note area has a fundamental + 2-3 overtones (octave, fifth, third). Very specific tuning. Warm, bell-like sustain. | Modal synthesis with specific partial ratios per note. Helmholtz body resonance adds warmth. Hand damping is essential. Crossfade between finger-tip (warm) and finger-pad (bright) strikes. Similar to steel pan but with more controlled, harmonic spectrum. |
| **Angklung** | Bamboo tubes in frame, shaken to produce sound. Each angklung plays one note. Ensemble of 20+ players creates melodies. | Resonant tube model excited by random impulse train (shaking). The randomness of shaking creates the organic, living quality. Ensemble mode: multiple slightly detuned instances with independent random timing. |
| **Sitar** | Long-necked lute with sympathetic strings. The characteristic "buzz" comes from the jawari bridge -- a wide, curved bridge that allows the string to graze against it. 13 sympathetic strings resonate with played notes. | Waveguide string + jawari bridge model. The bridge is modeled as a conditional contact nonlinearity -- when string amplitude drops below a threshold, it contacts the bridge and creates buzz harmonics. Sympathetic string bank (13 strings) tuned to raga. |
| **Shamisen** | 3 strings, struck with large bachi plectrum. Buzzing sound (sawari) from string contact with neck. Skin membrane body. Percussive attack. | Karplus-Strong + membrane resonator. The sawari buzz is similar to sitar's jawari -- conditional nonlinearity at the nut. Large plectrum creates percussive "snap" transient. Tsugaru style: very aggressive, almost percussive playing. |

### 4.2 Color Science to DSP Mapping

Research on synesthesia and chromesthesia (sound-to-color perception) reveals consistent cross-modal correspondences that can drive DSP parameter mapping:

#### Hue -> Frequency/Timbre
| Color Property | Audio Parameter | Mapping Rationale |
|---------------|----------------|-------------------|
| **Hue** (0-360 degrees) | Filter cutoff position / spectral centroid | Higher frequency light (blue/violet) maps to brighter, higher-frequency sounds. Lower frequency light (red/orange) maps to darker, warmer sounds. Research confirms pitch class maps to hue in chromesthesia. |
| **Saturation** | Harmonic richness / distortion amount | High saturation = rich, harmonically dense sound. Low saturation = pure, sine-like tone. Maps to "colorfulness" in both domains. |
| **Brightness/Value** | Amplitude / octave register | Brighter colors = louder or higher-register sounds. Dark colors = quiet or bass register. Brightness maps to both loudness and pitch height. |
| **Complementary color** | Opposing filter shape or detuning | Complementary colors create maximum visual contrast. In audio: complementary timbres (e.g., if primary is LP filtered, complement is HP filtered). |
| **Warm colors** (red/orange/yellow) | Lower spectral centroid, more fundamental | Warm colors = warm sound. More low-frequency energy, less high-frequency content. |
| **Cool colors** (blue/green/violet) | Higher spectral centroid, more upper partials | Cool colors = bright, clear sound. More high-frequency energy, crystalline quality. |

#### Color Science FX Chains (from Artwork overview)
The overview defines 4 FX types drawn from color science and water physics:

1. **Refraction FX**: Spectral splitting (like a prism). Multi-band processing where each band is delayed/pitched differently. Creates "rainbow" spectral spread.
2. **Absorption FX**: Frequency-dependent damping (like light absorbed by water). Progressive high-frequency loss with depth. The deeper the parameter, the darker the sound.
3. **Diffusion FX**: Scattering (like light in fog). Dense multi-tap delay with randomized times. Allpass chains. Turns point sources into ambient textures.
4. **Reflection FX**: Mirror/echo (like light bouncing off surfaces). Clean stereo delay with specific reflection patterns. Complementary-color processing on the wet signal.

#### Research-Backed Synesthetic Mappings

From Frontiers in Psychology (2025): Timbral complexity maps to color saturation. MFCCs (Mel-frequency cepstral coefficients) and chroma vectors are the strongest predictors of synesthetic color associations. Lower octaves map to lower color saturation; higher octaves to higher saturation. Sound energy maps to visual scale/size.

From Nature Scientific Reports: Musical pitch classes show consistent "rainbow hue" mapping in chromesthesia -- C maps to red/white, D to yellow, E to green/yellow, F to green/blue, G to blue, A to blue/violet, B to violet/red.

### 4.3 Art Style to Synthesis Mapping

| Art Style | Visual Characteristics | DSP Translation |
|-----------|----------------------|-----------------|
| **Street Art** (raw, urban) | Rough textures, spray paint bleed, stencil edges, bold color blocks, improvised surfaces | Bit crushing, noise injection, hard clipping, rough filter sweeps. Sudden parameter jumps (stencil edges). Lo-fi processing. Graffiti = the sound that shouldn't be there but is. |
| **Cave Painting** (ancient, minimal) | Earth tones, charcoal, hand prints, sparse composition, firelight flicker | Minimal partials (1-3 harmonics), amplitude tremolo (firelight), room reverb (cave acoustics with long RT60), very sparse arrangement. The space between notes matters more than the notes. |
| **Fine Art** (controlled, masterful) | Smooth gradients, precise brushwork, balanced composition, glazing technique | Smooth envelopes, precise filter control, clean transitions, subtle vibrato. Layered processing (glazing = multiple subtle FX stages building depth). High-resolution timbral control. |
| **Digital Art** (processed, algorithmic) | Pixel artifacts, procedural generation, glitch aesthetics, vector precision, generative patterns | Granular processing, algorithmic modulation (cellular automata, L-systems), spectral processing, precise quantization. The computer as creative partner, not imitator. |

### 4.4 The Showmen -- Sonic Signatures

#### George Clinton (Parliament-Funkadelic)
- **Sonic DNA:** Triple-layer Moog synth bass (the "Flashlight" bass that replaced electric bass), psychedelic guitar feedback, collective improvisation, polyrhythmic layering, extreme tonal range (whisper to scream).
- **DSP translation:** Massive unison bass (3 detuned oscillators, heavy LP filter), feedback distortion path, probability-based parameter variation (the "collective" unpredictability), extreme dynamic range. Moog ladder filter is essential. G-funk = P-Funk's Moog riffs + 90s production.
- **Macro concept:** FUNK (controls bass weight and filter funk) / COSMIC (controls spacey FX depth) / COLLECTIVE (controls voice spread and chaos) / MOTHERSHIP (controls overall processing intensity).

#### Doug Henning (Magic/Illusion)
- **Sonic DNA:** Misdirection, reveal, transformation. Not a musician but a performer whose art was making the impossible seem effortless.
- **DSP translation:** Spectral morphing (one timbre transforms into another), hidden layers that emerge (previously masked partials revealed by filter sweeps), "now you see it" parameter automation that creates timbral surprises. The "trick" is a preset that sounds like one instrument and gradually reveals it's been another all along.
- **Macro concept:** MISDIRECT (controls which spectral elements are hidden) / REVEAL (controls the transformation speed) / VANISH (controls decay/fade character) / WONDER (controls overall "impossible" parameter combinations).

#### Bob Fosse (Choreography/Dance)
- **Sonic DNA:** Isolation (individual body parts moving independently), angular precision, jazz hands (sharp rhythmic accents), minimal movement with maximum impact, black costumes with white gloves (contrast).
- **DSP translation:** Rhythmic parameter gating (notes that appear and disappear with choreographic precision), sharp transients with controlled sustain, contrast between sparse and dense, syncopated modulation patterns. Every parameter movement is deliberate and rhythmically locked.
- **Macro concept:** ISOLATE (controls voice independence) / PRECISION (controls rhythmic quantization tightness) / CONTRAST (controls dynamic range between sparse/dense) / JAZZ HANDS (controls accent/transient intensity).

#### Eddie Van Halen (Guitar Innovation)
- **Sonic DNA:** The "brown sound" -- saturated but clear distortion with thick mids and articulate highs. Two-handed tapping (rapid hammer-ons creating impossible melodic lines), harmonic squeals (pinch harmonics), Variac-powered Marshall (reduced voltage for smoother breakup), MXR Phase 90 phaser, dive bombs with Floyd Rose tremolo bar.
- **DSP translation:** Asymmetric tube saturation (Variac model: lower bias point creates different even/odd harmonic ratios), tapping = rapid legato note changes with no re-excitation, pitch bend with extreme range (Floyd Rose: +-2 octaves), phaser as always-on color. The innovation is in the PLAYING, not just the tone.
- **Macro concept:** BROWN (controls saturation character and mid emphasis) / TAP (controls legato/hammer-on behavior) / DIVE (controls pitch bend range and return speed) / ERUPTION (controls overall intensity and harmonic density).

### 4.5 The Aesthetes -- Artistic Philosophies as DSP

#### Kehinde Wiley (Ornamental Portraiture)
- **Artistic DNA:** Heroic-scale portraits of people of color against intricate William Morris floral patterns. The subject is hyper-realistic; the background is decorative and overwhelming. Subverts classical European painting tropes.
- **DSP translation:** A clear, prominent melodic voice (the "portrait") against a dense, ornamental textural background (the "wallpaper"). The relationship between foreground clarity and background density is the synthesis parameter. Sidechain compression where the melody ducks the texture. Ornamental = dense modulation patterns that frame but never obscure the primary voice.
- **Macro concept:** PORTRAIT (controls foreground voice prominence) / ORNAMENT (controls background texture density) / SCALE (controls overall grandeur/reverb) / SUBVERT (controls how much the background threatens to overtake the portrait).

#### Luis Barragan (Architectural Minimalism + Color)
- **Artistic DNA:** Minimal geometric forms, bold saturated color planes, controlled light. Emotion and serenity over formalism. "Any work of architecture which does not express serenity is a mistake." Color is structural, not decorative -- a pink wall IS the architecture.
- **DSP translation:** Minimal oscillator count (1-2), clean waveforms, large empty spaces (silence and reverb). Color = a single bold timbral characteristic (one strong formant, one resonant frequency) that defines the entire sound. No clutter. The filter IS the sound, like Barragan's colored walls ARE the architecture.
- **Macro concept:** SERENITY (controls overall calmness -- smooths transients, lengthens envelopes) / WALL (controls the single dominant resonant color) / LIGHT (controls brightness/spectral tilt -- Barragan used light as a material) / GEOMETRY (controls the precision of pitch quantization and rhythm).

#### Pedro Almodovar (Emotional Cinema)
- **Artistic DNA:** Saturated reds (passion, blood, fire, Spanish culture), melodrama, extreme emotion played sincerely, women's stories, kitsch elevated to art. Every frame is a color composition. "I use red because it's the color of blood and passion."
- **DSP translation:** Saturated, warm distortion (the sonic equivalent of Almodovar's reds). Expressive vibrato and pitch bending (melodrama). Wide dynamic range from whisper to scream. Reverb as emotional space (intimate scenes are dry, dramatic scenes are drenched). Everything is felt intensely.
- **Macro concept:** PASSION (controls saturation/distortion warmth) / DRAMA (controls dynamic range and envelope intensity) / INTIMATE (controls reverb size -- small = close, large = cinematic) / KITSCH (controls how "over the top" the processing becomes -- embracing excess).

#### Madlib (Sampling/Beat Culture)
- **Artistic DNA:** Blind sampling -- grabs records without looking, finds the loop by feel. Micro-chopping: taking tiny fragments and reassembling them into new rhythms. Genre-agnostic: samples Brazilian jazz, Bollywood, Ethiopian funk, prog rock. Speed shifting: pitching samples up or down to change character while keeping feel. "When I make beats, I just pick a record up and go."
- **DSP translation:** Granular sampling with randomized position. Time-stretching with intentional artifacts. Pitch shifting as a creative tool (not correction). Lo-fi processing (vinyl crackle, sample rate reduction). The "Madlib approach" = the engine should sound like it's sampling from itself, constantly rearranging its own output.
- **Macro concept:** CRATE (controls sample source/position -- the "record digging" parameter) / CHOP (controls granular slice size and rearrangement) / FLIP (controls pitch shift and time stretch amount) / DUSTY (controls lo-fi processing -- vinyl noise, bit depth, wow/flutter).

---

## 5. Cross-Collection Concerns

### 5.1 Parameter Prefix Collision Check

Proposed prefixes for all 68 engines must not collide with existing fleet prefixes. Current prefixes in use:

```
snap_ morph_ dub_ drift_ bob_ fat_ poss_ onset_ ow_ opal_ orb_ organon_
ouro_ obsidian_ origami_ oracle_ obscura_ ocean_ ocelot_ optic_ oblq_
osprey_ ost_ owl_ ohm_ orph_ obbl_ otto_ ole_ ombre_ orca_ octo_
olap_ owit_
```

**Kitchen (24 engines):** Each of the 6 quads has 4 engines. Example prefixes:
- CHEF quad: `sous_`, `exec_`, `pastry_`, `garde_` (or shorter: `chef1_` through `chef4_`)
- Use 3-5 char prefixes to keep param IDs manageable

**Travel (20 engines):** Prefixes from overview:
- `oxy_`, `obeam_`, `oarl_`, `ossia_` (Sail)
- `oxide_`, `ordn_`, `oilrig_`, `outre_` (Industrial)
- `oshun_`, `outrig_`, `oriole_`, `okav_` (Leisure)
- `orage_`, `ormada_`, `origin_`, `obelus_` (Historical)
- `outlier_`, `ossuary_`, `outcry_`, `onetree_` (Sable Island)

**Collision check:** `origin_` is close to `origami_` but distinct. `oxide_` is close to nothing. `oshun_` is close to nothing. No collisions detected. The `ocean_` prefix (used by Oceanic engine) vs `okav_` -- safe.

**Artwork (24 engines):** Prefixes need defining. Must avoid all of the above.

### 5.2 CPU Budget

With 34 engines currently and 68 being added (102 total), only 4-5 are loaded simultaneously. CPU budget per engine:
- Single engine preset: < 25% of one core
- Dual engine preset: < 28% each (56% total)
- Triple engine: < 18% each (54% total)
- Quad engine: < 14% each (56% total)

**Physical modeling engines** (Kitchen pianos, Travel woodwinds/brass) tend to be CPU-intensive. Budget 20-30% for these. Simpler engines (additive organ, modal percussion) can be lighter at 10-15%.

### 5.3 Collection Unlock / Licensing

TBD but architecturally: engines could be compiled into the main binary but disabled without a license key, or distributed as separate dynamic libraries loaded at runtime. The former is simpler for JUCE (just a boolean gate in `EngineRegistry`); the latter requires JUCE's `DynamicLibrary` support.

---

## 6. Build Sequence Recommendation

### Phase 1: Prototype One Engine Per Collection (Week 1)
Build one engine from each collection to validate the 3-axis architecture:
1. **Kitchen:** One CHEF organ engine (additive synthesis -- simplest to implement)
2. **Travel:** One SAIL woodwind engine (reuses existing `FamilyWaveguide.h`)
3. **Artwork:** One Color Quad A engine (erhu -- reuses bowed string waveguide)

This validates: Voice/FX/Wildcard param switching, preset format, coupling compatibility, CPU budget.

### Phase 2: Complete One Full Quad/Set (Weeks 2-3)
Build all 4 engines for one quad from each collection:
1. **Kitchen CHEF quad** (4 organ engines)
2. **Travel SAIL set** (4 woodwind engines)
3. **Artwork Color Quad A** (4 world string instruments)

This validates: Intra-quad coupling, preset variety across a quad, wildcard axis differentiation.

### Phase 3: Remaining Engines (Weeks 4-8)
Build remaining engines by collection, prioritizing:
1. Quads that share DSP approaches (e.g., all physical models together)
2. The 5th-slot engines last (Fusion, Sable Island, Arcade -- depend on understanding the quads they fuse)

### Phase 4: Presets and Polish (Weeks 9-10)
- 150 presets per engine (target: 10,200 presets across all 68 engines)
- Cross-engine coupling presets
- Sonic DNA computation
- Seance evaluation per engine

### Estimated Total: 10 weeks for 68 engines

This is aggressive. Each engine at ~500-1500 lines of DSP + 150 presets = ~2 days per engine average. 68 engines x 2 days = 136 days of work. With parallel builds and DSP reuse across quads, 10 weeks is achievable with daily sessions.

---

## Sources

### Physical Modeling & DSP
- [CCRMA Physical Audio Signal Processing](https://ccrma.stanford.edu/~jos/pasp/)
- [Julius O. Smith III - Physical Modeling Synthesis Update](https://ccrma.stanford.edu/~jos/pmupd/pmupd.pdf)
- [Frontiers - Sound Synthesis through Physical Modeling (2025)](https://www.frontiersin.org/journals/signal-processing/articles/10.3389/frsip.2025.1715792/full)
- [JUCE Tutorial - String Model with Delay Lines](https://juce.com/tutorials/tutorial_dsp_delay_line/)
- [Modeling the Woodstock Gamelan for Synthesis](https://quod.lib.umich.edu/i/icmc/bbp2372.1999.447/3/--modeling-the-woodstock-gamelan-for-synthesis)

### Instrument Acoustics
- [Didgeridoo Acoustics - UNSW Physics](https://newt.phys.unsw.edu.au/jw/didjeridu.html)
- [A Review of Didjeridu Acoustics - AIP Publishing](https://pubs.aip.org/asa/poma/article/52/1/035004/3282844/A-review-of-didjeridu-didgeridoo-acoustics)
- [A Simple Model of the Erhu Soundbox - ResearchGate](https://www.researchgate.net/publication/329821281_A_simple_model_of_the_Erhu_soundbox)
- [Erhu - Wikipedia](https://en.wikipedia.org/wiki/Erhu)

### Synesthesia & Color-Sound Mapping
- [Mapping Sound to Color - Designing Sound](https://designingsound.org/2017/12/20/mapping-sound-to-color/)
- [Musical Pitch Classes Have Rainbow Hues - Nature Scientific Reports](https://www.nature.com/articles/s41598-017-18150-y)
- [Rainbows in My Ears - Frontiers in Psychology (2025)](https://www.frontiersin.org/journals/psychology/articles/10.3389/fpsyg.2025.1697918/full)
- [musicolors: Bridging Sound and Visuals (2025)](https://arxiv.org/html/2503.14220v1)
- [Voice-Evoked Color Prediction - PMC](https://pmc.ncbi.nlm.nih.gov/articles/PMC12110112/)
- [Chromesthesia - Wikipedia](https://en.wikipedia.org/wiki/Chromesthesia)

### Boutique Synth Makers
- [What is West Coast Synthesis? - Perfect Circuit](https://www.perfectcircuit.com/signal/what-is-west-coast-synthesis)
- [Don Buchla - Red Bull Music Academy](https://www.redbullmusicacademy.com/lectures/don-buchla-passing-the-acid-test/)

### Cultural Figures
- [George Clinton - Grateful Web](https://www.gratefulweb.com/articles/celebrating-sonic-sorcerer-george-clinton)
- [Eddie Van Halen Brown Sound - Legendary Tones](https://legendarytones.com/edward-van-halen-brown-sound/)
- [Kehinde Wiley - TheArtStory](https://www.theartstory.org/artist/wiley-kehinde/)
- [Luis Barragan - ArchDaily](https://www.archdaily.com/898028/how-luis-barragan-used-light-to-make-us-see-color)
- [Pedro Almodovar Filmmaking Style - Critic Film](https://www.criticfilm.com/pedro-almodovars-filmmaking-style/)
- [How Madlib Made Madvillainy - Loop Kitchen](https://loopkitchen.co.uk/blogs/loop-kitchen-blog/madlib-madvillainy-sampling-techniques)
- [Micro-Chopping Madlib - Medium](https://medium.com/micro-chop/micro-chopping-madlib-5d77b91a5ea5)
