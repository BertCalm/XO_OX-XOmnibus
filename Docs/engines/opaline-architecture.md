# XOpaline Engine Architecture

*Engine #47 | KITCHEN Quad — Glass/Porcelain | March 2026*

---

## Identity

| Field | Value |
|-------|-------|
| **Name** | XOpaline |
| **Engine ID** | `Opaline` |
| **Parameter Prefix** | `opal2_` |
| **Accent Color** | Crystal Blue `#B8D4E3` |
| **Material** | Borosilicate Glass / Porcelain |
| **Piano** | Celesta / Toy Piano / Glass Harp / Porcelain Cups |
| **Quad Position** | 4th of 4 (KITCHEN) |
| **Polyphony** | 8 voices |

## Material Physics

| Property | Value |
|----------|-------|
| Density (rho) | 2,230 kg/m^3 |
| Wave speed (c) | 5,640 m/s |
| Impedance (Z) | 12,577,200 |
| Mode Bandwidth | Very narrow (high Q — glass rings at exact frequencies) |
| Character | Fragile, crystalline, pure partials, quick decay then long ring |

Glass has the **lowest impedance** of the four KITCHEN materials. This means:
- Best energy crossing from exciter to body
- But narrowest mode bandwidth (highest Q resonators)
- Few partials, each very pure and long-lived at their exact frequency
- The material IS a natural bandpass filter

## The 5 Pillars

### 1. Instrument Selector
Four distinct instruments, each with unique modal ratio tables derived from acoustic literature:

| Instrument | Modal Character | Source |
|------------|----------------|--------|
| **Celesta** (0) | Nearly harmonic (1:2:3:4...) — pure, magical | Metal bars + resonator tubes |
| **Toy Piano** (1) | Slightly inharmonic — bright, lo-fi | Short stiff tines (beam equation) |
| **Glass Harp** (2) | Strongly inharmonic — ethereal shimmer | Circular plate/shell modes |
| **Porcelain Cups** (3) | Bell-like — moderately inharmonic | Struck ceramic (Rossing 2000) |

Each instrument also has distinct:
- Mode amplitude rolloff rate (celesta slowest, toy piano fastest)
- Base Q (glass harp highest at 500, toy piano lowest at 120)
- Excitation character (felt hammer vs. hard tine vs. friction vs. spoon tap)

### 2. Glass Modal Resonator Bank
16 second-order IIR resonators in parallel, following the Kitchen CPU Optimization Strategy:

```
Per resonator per sample: 2 multiplies + 2 adds + 2 state loads = ~5 ns
16 modes * 5 ns * 8 voices = 640 ns/sample
+ 1 HF noise SVF * 4 ns * 8 voices = 32 ns/sample
Total: ~672 ns/sample = ~3.0% CPU at 512-sample block, 44.1 kHz
```

**Dynamic pruning:** Modes below -140 dB amplitude (`1e-7f`) are skipped when the exciter is inactive. Glass rings long, so the threshold is lower than other engines.

**HF noise shaper:** A single CytomicSVF bandpass per voice adds stochastic high-frequency character above the modal bank, simulating the spectral envelope of upper partials without computing 48 additional resonators.

### 3. Fragility Mechanic (the defining feature)
The Creme Brulee concept implemented as DSP:

```
crackThreshold = 0.65
if (velocity * fragility > crackThreshold):
    crackIntensity = (velocity * fragility - threshold) / (1.0 - threshold)
    → Noise burst: 2-5ms broadband noise (the crack sound)
    → Modal detuning: ±25 cents * crackIntensity (broken glass rings differently)
    → Q reduction on upper modes (broken glass rings less purely)
    → Higher modes shift more (glass breaks unevenly)
```

This is NOT a bug. It is the character. Glass is beautiful BECAUSE it can break. Below the threshold, every note rings pure and crystalline. Above, the response introduces nonlinearity — subtle harmonic distortion, modal detuning, noise artifacts. Full-velocity strikes don't just get louder; they get dangerous.

### 4. Thermal Shock
Glass is the most thermally sensitive of the four KITCHEN materials. Rapid temperature changes cause differential expansion, creating stress that manifests as:
- Non-uniform detuning between partials (up to ±12 cents)
- Per-voice thermal personality (random offset so voices drift independently)
- Slow ~3-second cycle for temperature target changes
- One-pole IIR tracking with ~1-second time constant

This is the parameter that makes XOpaline feel alive when nobody is playing.

### 5. Crystalline Envelope (Pate de Verre)
The Pate de Verre concept: heat glass, work it in its brief window of plasticity, cool it. XOpaline's envelope matches:
- Attack: 1ms (instant excitation)
- Decay: 50-200ms velocity-scaled (the brief working window)
- Sustain: 0% (no working state — glass is either liquid or solid)
- Release: 300ms (crystallization)
- Note-off: immediate 60% amplitude drop (glass stops vibrating quickly)

## Parameters (27 total)

| Parameter | ID | Range | Default | Notes |
|-----------|-----|-------|---------|-------|
| Instrument | `opal2_instrument` | 0-3 | 0 | Celesta/ToyPiano/GlassHarp/Porcelain |
| Fragility | `opal2_fragility` | 0-1 | 0.3 | Crack threshold sensitivity |
| Hammer Hardness | `opal2_hammerHardness` | 0-1 | 0.4 | Exciter brightness |
| Brightness | `opal2_brightness` | 200-20000 Hz | 10000 | Filter cutoff |
| Damping | `opal2_damping` | 0-1 | 0.1 | Mode energy loss rate |
| Decay | `opal2_decay` | 0.1-15 s | 3.0 | Ring time |
| Body Size | `opal2_bodySize` | 0-1 | 0.5 | Pitch scaling (0.5x-2.0x) |
| Inharmonicity | `opal2_inharmonicity` | 0-1 | 0.0 | Beam stiffness stretch |
| Shimmer | `opal2_shimmerAmount` | 0-1 | 0.3 | Crystalline beating |
| Thermal Shock | `opal2_thermalShock` | 0-1 | 0.2 | Temperature sensitivity |
| HF Noise | `opal2_hfNoise` | 0-1 | 0.3 | Stochastic upper partials |
| Filter Env Amt | `opal2_filterEnvAmount` | 0-1 | 0.3 | Filter envelope depth |
| Crystal Drive | `opal2_crystalDrive` | 0-1 | 0.0 | Subtle waveshaping |
| Sustain Pedal | `opal2_sustainPedal` | 0-1 | 0.0 | Extend ring |
| Bend Range | `opal2_bendRange` | 1-24 st | 2 | Pitch bend range |
| Macro CHARACTER | `opal2_macroCharacter` | 0-1 | 0.0 | -> hammer + brightness |
| Macro MOVEMENT | `opal2_macroMovement` | 0-1 | 0.0 | -> fragility |
| Macro COUPLING | `opal2_macroCoupling` | 0-1 | 0.0 | -> coupling depth |
| Macro SPACE | `opal2_macroSpace` | 0-1 | 0.0 | -> shimmer |
| LFO1 Rate | `opal2_lfo1Rate` | 0.005-20 Hz | 0.3 | LFO1 rate |
| LFO1 Depth | `opal2_lfo1Depth` | 0-1 | 0.1 | LFO1 -> brightness |
| LFO1 Shape | `opal2_lfo1Shape` | 0-4 | 0 | Sin/Tri/Saw/Sq/S&H |
| LFO2 Rate | `opal2_lfo2Rate` | 0.005-20 Hz | 0.8 | LFO2 rate |
| LFO2 Depth | `opal2_lfo2Depth` | 0-1 | 0.0 | LFO2 -> shimmer/pitch |
| LFO2 Shape | `opal2_lfo2Shape` | 0-4 | 0 | Sin/Tri/Saw/Sq/S&H |

## Doctrine Compliance

| Doctrine | Status | Implementation |
|----------|--------|----------------|
| D001 Velocity Shapes Timbre | PASS | Velocity -> hammer hardness -> upper mode excitation + filter env |
| D002 Modulation | PASS | 2 LFOs, mod wheel (brightness), aftertouch (hammer), 4 macros |
| D003 Physics IS Synthesis | PASS | Fletcher & Rossing 1998, Chaigne 1997, Rossing 2000 cited |
| D004 No Dead Parameters | PASS | All 27 parameters wired to DSP |
| D005 Breathing | PASS | LFO1 default 0.3 Hz, floor 0.005 Hz |
| D006 Expression Input | PASS | Velocity->timbre, aftertouch->hammer, mod wheel->brightness |

## KITCHEN Quad Contrast

| Engine | Material | Character | Impedance Z |
|--------|----------|-----------|-------------|
| XOven | Cast Iron | Massive, dark, slow | 36,000,000 |
| XOchre | Copper | Responsive, warm | 34,000,000 |
| XObelisk | Stone/Marble | Precise, ringing | 16,740,000 |
| **XOpaline** | **Glass** | **Fragile, crystalline, tiny** | **12,577,200** |

XOpaline is the lightest, brightest, most delicate. Where XOven traps energy in a massive resonant tank, XOpaline lets energy cross easily but concentrates it in extremely narrow modal peaks. XOven is a cast iron skillet; XOpaline is a porcelain ramekin. The contrast defines the quad.

## Coupling Behavior

- **AmpToFilter**: External amplitude modulates XOpaline's filter cutoff (±2000 Hz)
- **LFOToPitch**: External LFO drives XOpaline's pitch (±2 semitones)
- **AmpToPitch**: External amplitude displaces pitch
- **EnvToMorph**: External envelope morphs instrument selector

**KITCHEN-specific coupling potential:** When XOven couples to XOpaline, the cast iron's dense low-frequency modal cloud loads into XOpaline's sparse high-Q structure. XOpaline's pure glass partials ring with a low undertow they couldn't generate alone. The glass has been smoked in cast iron.

## Presets (10 factory)

| Name | Mood | Instrument | Character |
|------|------|-----------|-----------|
| Celesta Magic | Foundation | Celesta | Pure, magical, orchestral |
| Toy Piano LoFi | Foundation | Toy Piano | Bright, percussive, lo-fi |
| Glass Harp Ethereal | Aether | Glass Harp | Otherworldly, sustained shimmer |
| Porcelain Percussion | Foundation | Porcelain | Kitchen percussion, short ring |
| Creme Brulee | Flux | Glass Harp | Maximum fragility — cracks on hard hits |
| Frozen Cathedral | Atmosphere | Celesta | Maximum sustain, thermal drift, ice |
| Crystal Drive | Prism | Toy Piano | Driven harmonics, bright refractions |
| Thermal Fracture | Crystalline | Porcelain | Maximum thermal shock, dangerous |
| Music Box | Luminous | Celesta | Tiny body, sparkle, childhood |
| Shattered Glass | Kinetic | Glass Harp | Everything breaks — max fragility + drive |

## References

- Fletcher, N.H. & Rossing, T.D. (1998). *The Physics of Musical Instruments.* Springer.
- Chaigne, A. & Doutaut, V. (1997). "Numerical simulations of xylophones." JASA 101(1).
- Bilbao, S. (2009). *Numerical Sound Synthesis.* Wiley.
- Rossing, T.D. (2000). *Science of Percussion Instruments.* World Scientific.
- Kitchen CPU Optimization Strategy: `Docs/concepts/kitchen-cpu-optimization-strategy.md`
- Kitchen Quad Visionary: `Docs/concepts/kitchen-quad-visionary.md`
