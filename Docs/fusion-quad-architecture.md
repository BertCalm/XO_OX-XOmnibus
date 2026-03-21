# FUSION Quad — Architecture Document

*March 2026 | XO_OX Designs | 4 Electric Piano Engines*

---

## Overview

The FUSION Quad is the electric piano instrument family within the Kitchen Collection. Four engines, each modeling a distinct electric piano tradition, each carrying a cultural migration route. These engines are designed for the 5th-slot mechanic — they become available when all 4 Kitchen engines are loaded, unlocking through the conditional slot UI animation.

### The 4 Engines

| Engine | Source Instrument | Prefix | Color | Accent Hex | Params |
|--------|------------------|--------|-------|------------|--------|
| XOasis | Rhodes/Tine EP | `oasis_` | Cardamom Gold | `#C49B3F` | 23 |
| XOddfellow | Wurlitzer/Reed EP | `oddf_` | Neon Night | `#FF6B35` | 22 |
| XOnkolo | Clavinet/Pickup Keys | `onko_` | Kente Gold | `#FFB300` | 21 |
| XOpcode | DX7/FM EP | `opco_` | Digital Cyan | `#00CED1` | 26 |

All engines share: 8 voices, 4 macros (CHARACTER/MOVEMENT/COUPLING/SPACE), 2 LFOs, migration parameter, full D001-D006 doctrine compliance.

---

## Shared Architecture

### SpectralFingerprint Cache (152 bytes)

The FUSION quad's 5th-slot coupling works through metadata exchange rather than audio routing. Each engine continuously exports a `SpectralFingerprint` struct containing:

```
modalFrequencies[8]   — Top 8 sounding frequencies (Hz)
modalAmplitudes[8]    — Amplitude of each frequency [0,1]
impedanceEstimate     — Material hardness/softness [0,1]
temperature           — Energy/activity level [0,1]
spectralCentroid      — Brightness measure (Hz)
activeVoiceCount      — Number of sounding voices
fundamentalFreq       — Most prominent fundamental (Hz)
rmsLevel              — Current output RMS
harmonicDensity       — Harmonic vs inharmonic ratio [0,1]
attackTransience      — Recent transient energy (decays ~50ms)
padding[2]            — Reserved for future fields
```

Total: 38 floats = 152 bytes. CPU cost: effectively zero (populated from existing voice state).

### Migration Parameter

Each engine has a `{prefix}_migration` parameter [0,1] that controls how deeply the engine absorbs influences from coupled engines via the Spectral Fingerprint. At 0%, pure source tradition. At 100%, maximum cultural fusion.

### Doctrine Compliance

| Doctrine | Implementation |
|----------|---------------|
| D001 (Velocity Shapes Timbre) | All engines: velocity drives filter brightness + engine-specific timbral response |
| D002 (Modulation is Lifeblood) | 2 LFOs, mod wheel, aftertouch, 4 macros per engine |
| D003 (Physics IS Synthesis) | Rhodes: tine+pickup model. Wurli: reed+preamp. Clav: string+pickup. FM: operator physics |
| D004 (No Dead Parameters) | Every declared parameter wired to audio output |
| D005 (Engine Must Breathe) | LFO1 rate floor 0.005 Hz (200-second cycle) |
| D006 (Expression Input Required) | Mod wheel + aftertouch mapped per engine |

---

## Engine 1: XOasis (Rhodes / Tine EP)

### DSP Architecture

```
Hammer → RhodesToneGenerator (6-partial additive) → RhodesPickupModel (proximity LP)
→ RhodesAmpStage (asymmetric tube saturation) → CytomicSVF (LP) → Tremolo → Output
```

**RhodesToneGenerator**: 6-partial additive synthesis with physically-derived partial ratios. Partials 1-6 at harmonic ratios with the 3rd partial boosted (the characteristic "bell"). Each partial has independent decay rates — upper partials decay faster, matching tine physics.

**RhodesPickupModel**: One-pole LP simulating pickup proximity effect. `oasis_bell` controls pickup position (tip=bright, base=warm).

**RhodesAmpStage**: Asymmetric tube saturation via `fastTanh`. Positive excursions clip harder than negative (even harmonic generation = the "bark"). `oasis_warmth` controls drive depth.

**Tremolo**: Per-voice `StandardLFO` with stereo width parameter for the suitcase effect.

### Key Parameters

- `oasis_warmth` — Tube amp warmth (clean → warm → bark)
- `oasis_bell` — Tine attack brightness / pickup position
- `oasis_tremRate` / `oasis_tremDepth` — Built-in Rhodes vibrato circuit

### Expression Mapping

- Mod wheel → warmth (+0.4 range)
- Aftertouch → brightness (+3000 Hz range)
- Velocity → filter brightness (D001) + bell boost (quadratic)

---

## Engine 2: XOddfellow (Wurlitzer / Reed EP)

### DSP Architecture

```
Hammer → WurliReedModel (5-partial with warble) → WurliPreamp (always-on tube drive)
→ CytomicSVF (LP) → Tremolo → Output
```

**WurliReedModel**: 5-partial additive with slightly inharmonic ratios (reed clamping imperfection: 1.0, 2.01, 3.03, 4.08, 5.15). Includes per-voice warble from a slow (~4.5 Hz) frequency modulation — softer reed = more warble.

**WurliPreamp**: Never truly clean (minimum drive 1.5x). Dual `fastTanh` stages for odd-harmonic emphasis. `oddf_drive` controls saturation depth.

**Tremolo**: Identical architecture to Oasis but with faster default rate and deeper default depth.

### Key Parameters

- `oddf_reed` — Reed stiffness (soft=warm+warble, stiff=bright+buzzy)
- `oddf_drive` — Preamp drive (the Wurli is never clean — this controls how dirty)
- `oddf_tremRate` / `oddf_tremDepth` — Wurlitzer's signature tremolo

### Expression Mapping

- Mod wheel → drive (+0.5 range)
- Aftertouch → reed stiffness (+0.3 range)

---

## Engine 3: XOnkolo (Clavinet / Pickup Keys)

### DSP Architecture

```
Tangent Strike → ClaviStringModel (8-harmonic with pickup comb filter)
→ AutoWahEnvelope (envelope-following BPF) → CytomicSVF (LP) → Output
```

**ClaviStringModel**: 8-harmonic additive with odd-harmonic emphasis (string struck at center point). Pickup position (`onko_pickup`) applies comb filtering — at bridge (1.0) all harmonics pass; at neck (0.0) alternating harmonics are attenuated. Key-off produces a noise burst "clunk" from the damper pad. Decay is fast when key is released (damper simulation).

**AutoWahEnvelope**: Envelope-following bandpass filter. Fast attack (2ms), slow release (100ms). Velocity and envelope combine to sweep the BPF center frequency from 400-5400 Hz. `onko_funk` controls depth.

### Key Parameters

- `onko_funk` — Auto-wah depth (0=clean, 1=maximum funk envelope)
- `onko_pickup` — Pickup position (0=neck/warm, 1=bridge/bright)
- `onko_clunk` — Key-off mechanical noise level

### Expression Mapping

- Mod wheel → funk depth (+0.5 range)
- Aftertouch → pickup brightness (+0.3 range)
- Velocity → filter brightness (quadratic for aggressive response)

---

## Engine 4: XOpcode (DX7 / FM EP)

### DSP Architecture

```
FMOperator (modulator) → phase modulation → FMOperator (carrier) → CytomicSVF (LP) → Output
                ↑
        DXModulationEnvelope (controls FM index over time)
```

**FMOperator**: Phase accumulator oscillator using `fastSin`. Supports external phase modulation input. Two operators: carrier (the pitch you hear) and modulator (the harmonic character).

**DXModulationEnvelope**: 4-stage envelope controlling FM index. Fast attack → exponential decay to sustain level. This is what creates the "bell attack into warm sustain" character of the DX EP.

**Three Algorithm Modes**:
- Series (0): Modulator → Carrier. Classic DX EP.
- Parallel (1): Modulator + Carrier. Organ-like.
- Feedback (2): Modulator feeds back into itself. Metallic, harsh.

### Key Parameters

- `opco_algorithm` — FM routing (0=series, 1=parallel, 2=feedback)
- `opco_ratio` — Modulator:carrier frequency ratio (0.5-16.0)
- `opco_index` — FM modulation depth (0-5)
- `opco_feedback` — Self-modulation amount (feedback mode)
- `opco_velToIndex` — How much velocity increases FM index

### Expression Mapping

- Mod wheel → FM index (+0.8 range)
- Aftertouch → brightness (+4000 Hz range)
- Velocity → FM index via dedicated `velToIndex` parameter

---

## CPU Budget

Per the Kitchen CPU Optimization Strategy:

| Configuration | Budget | Estimate |
|--------------|--------|----------|
| Single FUSION engine | < 5% | ~2-3% (additive/FM, no modal resonators) |
| 4 Kitchen + 1 FUSION | < 25% | ~17% total (SFC eliminates audio routing) |

FUSION engines are significantly lighter than Kitchen engines (no modal resonator banks, no sympathetic networks). The Spectral Fingerprint Cache adds ~0% overhead per Kitchen engine (just copying existing voice state into a struct).

---

## Preset Library (40 Presets)

| Engine | Count | Moods Covered |
|--------|-------|---------------|
| XOasis | 10 | Foundation, Atmosphere, Kinetic, Organic, Flux, Ethereal, Prism, Luminous, Deep |
| XOddfellow | 10 | Foundation, Kinetic, Organic, Flux, Atmosphere, Ethereal, Prism, Luminous, Deep, Crystalline |
| XOnkolo | 10 | Foundation, Kinetic, Organic, Flux, Atmosphere, Ethereal, Prism, Deep, Luminous, Crystalline |
| XOpcode | 10 | Foundation, Crystalline, Kinetic, Atmosphere, Organic, Flux, Ethereal, Prism, Deep, Luminous |

Each engine has a "signature" preset in Foundation mood that passes the Source Tradition Test.

---

## Source Tradition Tests

1. **XOasis** — "Midnight at the Kissa": Play McCoy Tyner's "Afro Blue" voicings. The tine should ring with a warm bell character, the warmth should feel like tube saturation, the sustain should feel musical.

2. **XOddfellow** — "Night Market Grit": Play Ray Charles-style block chords. The reed should warble, the preamp should grit, the tremolo should pulse. It should sound like it's been on the road.

3. **XOnkolo** — "Superstition Slap": Play the "Superstition" riff. The bridge pickup should cut, the attack should be percussive, the auto-wah should sweep on hard hits, the key-off should clunk.

4. **XOpcode** — "E Piano 1": Play a root-position C major chord. The bell attack should be crystalline, the decay should be smooth, the sustain should be warm. It should sound like 1983.

---

## Files

| Path | Purpose |
|------|---------|
| `Source/Engines/Oasis/OasisEngine.h` | Rhodes tine EP engine |
| `Source/Engines/Oasis/OasisEngine.cpp` | One-line stub |
| `Source/Engines/Oddfellow/OddfellowEngine.h` | Wurlitzer reed EP engine |
| `Source/Engines/Oddfellow/OddfellowEngine.cpp` | One-line stub |
| `Source/Engines/Onkolo/OnkoloEngine.h` | Clavinet pickup keys engine |
| `Source/Engines/Onkolo/OnkoloEngine.cpp` | One-line stub |
| `Source/Engines/Opcode/OpcodeEngine.h` | DX7 FM EP engine |
| `Source/Engines/Opcode/OpcodeEngine.cpp` | One-line stub |
| `Docs/concepts/fusion-quad-visionary.md` | Full concept document |
| `Docs/concepts/kitchen-cpu-optimization-strategy.md` | CPU budget framework |

---

*Built 2026-03-21 | XO_OX Designs*
