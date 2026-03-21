# XOven Engine Architecture — Cast Iron Concert Grand

*Kitchen Collection Engine #1 | March 2026*

---

## Identity

| Property | Value |
|----------|-------|
| Engine ID | `Oven` |
| Parameter prefix | `oven_` |
| Accent color | Cast Iron Black `#2C2C2C` |
| Material | Cast Iron (rho=7200 kg/m3, c=5100 m/s, Z=36.72 MRayl) |
| Piano model | Concert Grand (Steinway D-like) |
| Voices | 8, LRU voice stealing |
| Parameters | 27 |
| CPU target | ~3.2% at 44.1kHz/512 buffer |

---

## DSP Architecture

### Signal Flow

```
MIDI Note → Hammer Model → Modal Resonator Bank (16 IIR) → + HF Noise Fill
                                                            ↓
                                                     Bloom Envelope
                                                            ↓
                                                    Amp Envelope × Output
                                                            ↓
                                                    Output Filter (SVF LP)
                                                            ↓
                                                    Voice Mix (stereo pan)
                                                            ↓
                                               Sympathetic Network (post-mix, shared)
                                                            ↓
                                                       Stereo Output
```

### 1. Hunt-Crossley Hammer Model

Nonlinear contact excitation based on Hunt & Crossley (1975). Velocity maps to both hammer hardness AND contact time:

- **Soft velocity** (felt hammer): 6-8ms contact, sine pulse, low-passed. Only excites lower modes.
- **Hard velocity** (hard hammer): 1-2ms contact, sine + noise ("Maillard char"), full spectrum excited.
- **Micro-rebound**: Cast iron's slow body creates a subtle secondary impulse at 20-35ms (thermal mass reflection).

This implements D001 (Velocity Must Shape Timbre) at the physics level.

### 2. Modal Resonator Bank (16 modes)

16 second-order IIR biquad resonators in parallel, each representing a body mode of the cast iron frame. Decision K1 from the CPU strategy document.

**Mode frequencies**: Derived from cast iron stiffness dispersion:
```
f_n = f_1 * n * sqrt(1 + B*n^2)   where B ~= 0.0004
```

**Q factors**: Cast iron's high impedance (36.72 MRayl) means extreme energy retention:
- Base Q: 150-600 (density parameter)
- Upper modes: Q decreases with mode index (radiation loss)
- Body resonance parameter: additional Q boost

**Amplitude weighting**: Audsley spectral envelope — 1/n rolloff with formant boost at modes 3-5.

**Dynamic pruning**: Modes below amplitude threshold are skipped (saves CPU on sustained notes where upper modes have decayed).

### 3. HF Noise Fill

Above the 16th mode ceiling, shaped noise matches the spectral slope. One CytomicSVF bandpass per voice. The noise is triggered by the hammer and decays with the note. Negligible CPU cost.

### 4. Sympathetic String Network (post-mix, shared)

Decision K2: NOT per-voice. One shared set of 12 resonators driven by the summed voice output. Physically accurate (the soundboard is shared).

- Responds to harmonically related frequencies (octave, 5th, sub-octave)
- Controlled by damper pedal (CC64): pedal up = full engagement
- Cast iron sympathetic scaling: 0.6 (high impedance = sympathetics accumulate)
- Updates active string set on note-on/note-off

### 5. Thermal Drift

Temperature parameter drives slow eigenfrequency detuning:
- E(T) = E0 * (1 - 0.0003*T) per the Visionary spec
- ~0.8 cents per 10 degrees C, max +-5 cents
- Per-voice thermal personality: +-3 cents variance (cast iron non-uniformity)
- New drift target every ~6 seconds (massive thermal mass = slow change)

### 6. Bloom Envelope

Cast iron thermal mass metaphor: optional 0-200ms slow attack ramp. When bloom > 0, the note fades in rather than appearing instantly. The iron has to warm up before it speaks.

---

## Doctrine Compliance

| Doctrine | Implementation |
|----------|---------------|
| D001 | Velocity → hammer hardness → contact time → mode excitation spectrum (Hunt-Crossley) |
| D002 | 2 LFOs (rate/depth/shape), mod wheel, aftertouch, 4 macros (CHARACTER/MOVEMENT/COUPLING/SPACE) |
| D003 | Modal synthesis with cast iron material constants, Hunt-Crossley citation, Audsley envelope, Chaigne references |
| D004 | All 28 parameters wired to DSP (verified: density→Q, temperature→drift, bloom→attack, sympathetic→network, competition→stub) |
| D005 | LFO rate floor 0.005 Hz (via StandardLFO shared utility) |
| D006 | Mod wheel → brightness, aftertouch → body resonance + sustain, CC64 → damper pedal/sympathetic |

---

## Parameters (27 total)

### Modal Bank (3)
- `oven_brightness` — output filter cutoff (200-16000 Hz)
- `oven_bodyResonance` — Q factor boost for modal resonators (0-1)
- `oven_hfAmount` — HF noise fill level (0-1)

### Hammer (1)
- `oven_hardness` — hammer felt/steel blend (0=soft felt, 1=hard) → contact time + noise mix

### Material (2)
- `oven_density` — cast iron density parameter → Q factor range (0-1)
- `oven_temperature` — thermal drift intensity (0-1) → eigenfrequency detuning

### Sympathetic (1)
- `oven_sympathetic` — sympathetic string network depth (0-1)

### Cast Iron Character (2)
- `oven_bloomTime` — slow attack bloom (0=instant, 1=200ms thermal mass fade-in)
- `oven_sustainTime` — base sustain time in seconds (0.5-15)

### Filter (1)
- `oven_filterEnvAmt` — filter envelope → brightness modulation (0-1)

### Amp ADSR (4)
- `oven_ampAttack`, `oven_ampDecay`, `oven_ampSustain`, `oven_ampRelease`

### Pitch (1)
- `oven_bendRange` — pitch bend range in semitones (1-24)

### Macros (4)
- `oven_macroCharacter` — CHARACTER → hardness + brightness
- `oven_macroMovement` — MOVEMENT (available for coupling)
- `oven_macroCoupling` — COUPLING → sympathetic depth
- `oven_macroSpace` — SPACE → body resonance

### LFOs (6)
- `oven_lfo1Rate`, `oven_lfo1Depth`, `oven_lfo1Shape` — LFO1 → brightness modulation
- `oven_lfo2Rate`, `oven_lfo2Depth`, `oven_lfo2Shape` — LFO2 → body resonance modulation

### Competition (2)
- `oven_competition` — adversarial coupling stub (Kitchen quad V2)
- `oven_couplingResonance` — coupling resonance control (Kitchen quad V2)

---

## Presets (10)

| Preset | Mood | Character |
|--------|------|-----------|
| Midnight Grand | Foundation | Definitive XOven: dark, massive, sustained |
| Steak Au Poivre | Foundation | Hard hammer sear → dark sustain braise |
| Iron Cathedral | Atmosphere | Bloom + max sympathetic = cathedral wash |
| Forged Bright | Prism | Maximum hardness, all modes excited |
| Thermal Drift | Flux | Temperature at max, LFOs modulating |
| Infinite Sustain | Aether | Max density, near-infinite ring |
| Sunken Steinway | Submerged | 800 Hz brightness, max bloom, underwater |
| Bourguignon | Deep | Slow bloom, braised over hours |
| Hammer Dance | Kinetic | Short percussive, max velocity sensitivity |
| Living Iron | Organic | Both LFOs breathing, thermal personality |

---

## Kitchen Quad Context

XOven is the first of four Kitchen Collection engines:

| Engine | Material | Impedance Z | Character |
|--------|----------|-------------|-----------|
| **XOven** | Cast Iron | 36.72 MRayl | Dark, massive sustain, energy trapped |
| XOchre | Copper | 34.05 MRayl | Warm, responsive, immediate dynamics |
| XObelisk | Marble | 16.74 MRayl | Pure modes, prepared piano objects |
| XOpaline | Glass | 12.58 MRayl | Crystalline, narrow bandwidth, fragile |

Coupling between Kitchen engines uses the transmission coefficient:
```
T = 4*Z1*Z2 / (Z1 + Z2)^2
```

XOven → XOpaline: T ~= 0.38 (38% energy transfer — strong coupling, dramatic contrast).

---

## Shared DSP Utilities Used

- `StandardLFO` — both voice LFOs
- `FilterEnvelope` — filter envelope + amp envelope
- `CytomicSVF` — voice output filter + HF noise shaper
- `GlideProcessor` — voice frequency glide
- `ParameterSmoother` — all 7 smoothed parameters
- `VoiceAllocator` — LRU voice stealing
- `PitchBendUtil` — pitch wheel processing
- `SilenceGate` — 500ms hold, zero-idle bypass
- `FastMath` — fastPow2, fastExp, fastSin, fastCos, flushDenormal
