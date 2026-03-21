# XOchre — Copper Upright Piano Architecture

*KITCHEN Quad Engine #2 | March 2026 | XO_OX Designs*

---

## Identity

| Field | Value |
|-------|-------|
| **Engine Name** | XOchre |
| **Engine ID** | Ochre |
| **Material** | Copper |
| **Instrument** | Upright Piano |
| **Accent Color** | Copper Patina `#B87333` |
| **Parameter Prefix** | `ochre_` |
| **Polyphony** | 8 voices |
| **CPU Target** | <3.5% @ 8 voices, 512-sample block, 44.1kHz |
| **Character** | WARM and QUICK (vs XOven's MASSIVE and DARK) |

---

## Material Physics

### Copper Properties
- **Density**: rho = 8960 kg/m^3
- **Wave speed**: c = 3750 m/s
- **Impedance**: Z = rho x c = 33,600,000 Rayls
- **Thermal conductivity**: 401 W/mK (highest of the KITCHEN materials)
- **Internal damping**: Higher than cast iron -> faster energy dissipation

### Comparison to XOven (Cast Iron)
| Property | XOven (Cast Iron) | XOchre (Copper) | Consequence |
|----------|-------------------|-----------------|-------------|
| Impedance Z | 36,000,000 | 33,600,000 | Copper: ~7% lower -> better HF transmission |
| Wave speed c | 5,000 m/s | 3,750 m/s | Copper: lower -> HF penetrates body more |
| Internal damping | Low | Medium | Copper: faster decay, more intimate |
| Thermal conductivity | 80 W/mK | 401 W/mK | Copper: 5x faster thermal response |

### Synthesis Consequence
Copper is NOT just a brighter iron. It has:
1. **Similar mass** but **lower wave speed** -> HF energy enters the body more efficiently
2. **Higher internal damping** -> energy dissipates faster -> shorter sustain
3. **Higher thermal conductivity** -> faster response to temperature changes -> more expressive

The result: an instrument that is immediate, responsive, and warm. It rewards dynamic control and punishes laziness.

---

## DSP Architecture

### Signal Flow
```
MIDI Note On
    |
    v
[Hammer Model] -- Hunt-Crossley contact, upright geometry (from below)
    |              + Caramel saturation (velocity-driven waveshaping)
    |
    v
[16-Mode Modal Bank] -- Copper eigenfrequencies (plate model)
    |                    Q scaled by conductivity parameter
    |                    Dynamic mode amplitude per hardness
    |
    v
[HF Noise Shaper] -- Perceptual body character above 400Hz
    |                  (replaces 48 additional modes)
    |
    v
[Caramel Post-Sat] -- Gentle fastTanh waveshaping
    |                   (copper saucepan / caramelization mapping)
    |
    v
[Body Resonator] -- 3-mode upright body (Practice/Parlour/Studio)
    |
    v
[Decay Envelope] -- Exponential amplitude decay (copper-fast)
    |
    v
[LPF (Brightness)] -- CytomicSVF, modulated by filter envelope + LFO1
    |
    v
[Stereo Pan] -- Pitch-keyed stereo spread
    |
    v
[Sympathetic Network] -- 12-string shared (post-voice summing)
    |
    v
[Output]
```

### The 7 Pillars

#### 1. Upright Hammer Model
Hunt-Crossley contact model adapted for upright piano geometry:
- Hammer strikes from below (shorter throw than grand)
- Contact time: 0.5ms (hard) to 3ms (soft) -- faster than grand's 4-8ms
- Caramel saturation integrated into excitation: `fastTanh(driven) * caramelAmount`
- Noise component for felt texture, scaled by hardness^2

#### 2. Copper Modal Bank (16 modes)
16 IIR biquad resonators with copper-derived eigenfrequencies:
- Ratios from rectangular copper plate model (Bilbao 2009)
- Pre-computed amplitude weighting per mode (kCopperModeAmps)
- Q range: 40 (high conductivity) to 400 (low conductivity)
- Modes above index 4 get brightness boost from conductivity parameter

#### 3. Conductivity Control (`ochre_conductivity`)
The primary character parameter. Maps copper's thermal/acoustic conductivity:
- Higher = brighter transients, faster decay, more upper partials
- Lower = warmer, longer sustain, more contained
- Modulated by mod wheel (D006) and macro CHARACTER

#### 4. Caramel Saturation (`ochre_caramel`)
Copper saucepan -> caramelization mapping:
- Pre-resonator: velocity-driven waveshaping in hammer model
- Post-resonator: gentle `fastTanh` saturation
- Not distortion -- transformation. Sharp sucrose becomes complex amber.
- Macro MOVEMENT modulates caramel intensity

#### 5. Intimate Body (3 types)
Upright piano body with 3 OchreMode resonators per type:
- **Practice Room** (0): Dry, tight. Freqs: 150/420/950 Hz, low Q
- **Parlour** (1): Warm bloom, wooden cabinet. Freqs: 120/350/780 Hz, higher Q
- **Studio** (2): Balanced, professional. Freqs: 180/500/1100 Hz, medium Q

#### 6. Sympathetic Strings (12-string sparse)
Shared across voices (physically accurate: one soundboard):
- 12 resonators built from active note frequencies (fundamental + octave + fifth)
- Scaled to 60% of grand piano level (smaller soundboard)
- Rebuilt at note-on and note-off

#### 7. Copper Thermal Drift
Temperature modulates eigenfrequencies:
- Young's modulus: E(T) = E0 * (1 - 0.0003 * T)
- Copper drifts 5x faster than iron (higher thermal conductivity)
- Time constant: ~0.3s (vs iron's ~0.5s)
- New drift target every ~2.5s, max +/-12 cents
- Per-voice thermal personality for ensemble detuning

---

## Parameters (27 total)

### Core (11)
| Parameter ID | Range | Default | Description |
|-------------|-------|---------|-------------|
| `ochre_conductivity` | 0-1 | 0.5 | Copper conductivity (brightness vs warmth) |
| `ochre_hardness` | 0-1 | 0.4 | Hammer hardness |
| `ochre_bodyType` | 0-2 | 0 | Practice Room / Parlour / Studio |
| `ochre_bodyDepth` | 0-1 | 0.4 | Body resonance depth |
| `ochre_brightness` | 200-20000 Hz | 10000 | Post-modal LPF cutoff |
| `ochre_damping` | 0-1 | 0.4 | Damping factor |
| `ochre_decay` | 0.05-8.0 s | 1.2 | Decay time |
| `ochre_caramel` | 0-1 | 0.3 | Caramel saturation amount |
| `ochre_sympathy` | 0-1 | 0.2 | Sympathetic resonance level |
| `ochre_hfCharacter` | 0-1 | 0.3 | HF noise burst body character |
| `ochre_thermalDrift` | 0-1 | 0.4 | Thermal drift depth |

### Envelope / Pitch (2)
| Parameter ID | Range | Default | Description |
|-------------|-------|---------|-------------|
| `ochre_filterEnvAmount` | 0-1 | 0.4 | Filter envelope depth |
| `ochre_bendRange` | 1-24 st | 2 | Pitch bend range |

### Macros (4)
| Parameter ID | Macro | Routes |
|-------------|-------|--------|
| `ochre_macroCharacter` | CHARACTER | -> conductivity, hardness, brightness |
| `ochre_macroMovement` | MOVEMENT | -> caramel |
| `ochre_macroCoupling` | COUPLING | -> sympathy |
| `ochre_macroSpace` | SPACE | -> body depth |

### LFOs (6)
| Parameter ID | Range | Default | Target |
|-------------|-------|---------|--------|
| `ochre_lfo1Rate` | 0.005-20 Hz | 0.5 | LFO1 rate |
| `ochre_lfo1Depth` | 0-1 | 0.1 | LFO1 -> brightness (+/-3000 Hz) |
| `ochre_lfo1Shape` | 0-4 | 0 (Sine) | LFO1 waveform |
| `ochre_lfo2Rate` | 0.005-20 Hz | 1.0 | LFO2 rate |
| `ochre_lfo2Depth` | 0-1 | 0.0 | LFO2 depth (reserved V2) |
| `ochre_lfo2Shape` | 0-4 | 1 (Triangle) | LFO2 waveform |

---

## CPU Budget

```
16 modes x 5ns x 8 voices = 640 ns/sample
1 SVF noise shaper x 4ns x 8 voices = 32 ns/sample
12 sympathetic resonators x 5ns = 60 ns/sample (shared)
Caramel saturation (fastTanh) x 2ns x 8 voices = 16 ns/sample
Total: ~748 ns/sample = ~3.3% CPU @ 512-sample block, 44.1kHz
```

Well within the 5% per-engine target. SilenceGate with 300ms hold eliminates idle cost.

---

## Doctrine Compliance

| Doctrine | Status | Implementation |
|----------|--------|---------------|
| D001 Velocity -> Timbre | PASS | Velocity scales hammer hardness, filter env, caramel amount |
| D002 Modulation | PASS | 2 LFOs, mod wheel -> conductivity, aftertouch -> hardness, 4 macros |
| D003 Physics IS Synthesis | PASS | Copper material properties (density, wave speed, damping) derive all DSP parameters |
| D004 No Dead Params | PASS | All 27 parameters affect audio output |
| D005 Breathing | PASS | LFO1 rate floor 0.005 Hz (200s cycle), thermal drift autonomous |
| D006 Expression Input | PASS | Velocity -> timbre, aftertouch -> hardness, mod wheel -> conductivity |

---

## KITCHEN Quad Context

XOchre is the second of four KITCHEN piano engines:

| Engine | Material | Character | Status |
|--------|----------|-----------|--------|
| XOven | Cast Iron | MASSIVE and DARK | Designed, not built |
| **XOchre** | **Copper** | **WARM and QUICK** | **BUILT** |
| XObelisk | Marble/Granite | PRECISE and ETERNAL | Designed, not built |
| XOpaline | Borosilicate Glass | CRYSTALLINE and FRAGILE | Designed, not built |

### Coupling Predictions
- **XOchre <-> XOven**: T ~ 0.97 (near-identical impedance). Bronze piano timbre.
- **XOchre <-> XObelisk**: Copper warmth infuses stone's cold rigidity.
- **XOchre <-> XOpaline**: Copper's thermal response stabilizes glass's fragility.

---

## Presets (10 Factory)

| Preset | Mood | Character |
|--------|------|-----------|
| Practice Room | Foundation | Default copper upright, dry, honest |
| Warm Keys | Foundation | Parlour body, rich, singing |
| Bright Attack | Foundation | High conductivity, percussive |
| Studio Grand | Foundation | Balanced, professional |
| Copper Haze | Atmosphere | Heavy drift, sympathetic bloom |
| Patina Memory | Atmosphere | Aged copper, long decay, weathered |
| Caramel Glaze | Prism | Full caramel saturation, sweet |
| Sugar Work | Prism | Bright + precise, crystalline top |
| Thermal Shift | Flux | Maximum drift, evolving |
| Hammered Copper | Kinetic | Aggressive, metallic, percussive |

---

## Shared DSP Dependencies

All from `Source/DSP/`:
- `CytomicSVF.h` -- TPT state-variable filter
- `FastMath.h` -- flushDenormal, fastTanh, fastExp, fastPow2, fastSin, fastCos
- `StandardLFO.h` -- 5-shape LFO with D005 floor
- `FilterEnvelope.h` -- ADSR for filter modulation
- `GlideProcessor.h` -- Frequency-domain portamento
- `ParameterSmoother.h` -- One-pole zipper-free smoothing
- `VoiceAllocator.h` -- LRU voice stealing
- `PitchBendUtil.h` -- MIDI pitch wheel pipeline
- `SRO/SilenceGate.h` -- Zero-idle CPU bypass
