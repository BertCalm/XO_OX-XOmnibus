# CELLAR Quad — Architecture Document

*Built March 2026 | Bass Instrument Family with Gravitational Coupling*

---

## Overview

The CELLAR quad is the bass foundation of the Kitchen Collection — four engines spanning the complete bass synthesis spectrum from sub-bass (XOgre) through analog (XOlate) and acoustic (XOaken) to digital FM (XOmega). All four share MIDI-domain gravitational coupling (CPU decision C1: no FFT).

## Engine Summary

| Engine | Full Name | Synthesis | Accent Color | Prefix | Params | Presets |
|--------|-----------|-----------|-------------|--------|--------|---------|
| OGRE | XOgre (Root Cellar) | Dual osc (sine+tri) + sub-harmonic + tanh saturation | Deep Earth Brown `#4A2C0A` | `ogre_` | 27 | 10 |
| OLATE | XOlate (Aged Wine) | Dual osc (saw+pulse) + ladder filter + fermentation | Burgundy `#6B1A2A` | `olate_` | 30 | 10 |
| OAKEN | XOaken (Cured Wood) | Karplus-Strong + body resonator + curing model | Dark Walnut `#3D2412` | `oaken_` | 29 | 10 |
| OMEGA | XOmega (Distillation) | 2-operator FM + distillation model | Copper Still `#B04010` | `omega_` | 30 | 10 |

---

## XOgre — Sub Bass / Root Vegetables

**DSP Chain:**
1. **Dual PolyBLEP oscillators** (sine + triangle) with crossfade mix
2. **Sub-harmonic generator** — toggle-based frequency halver producing octave-down content, smoothed by one-pole LP at ~80 Hz
3. **rootDepth** mix — blends sub-harmonic with main oscillator output
4. **Density** — CytomicSVF LP emphasis on extreme sub-frequencies (40-60 Hz range)
5. **Tanh saturation** — thick waveshaping with drive control (1x-9x gain)
6. **Body resonance filter** — soil-character-aware CytomicSVF (clay=tight LP, sandy=wider, rocky=BP)
7. **Filter envelope** — velocity-scaled brightness sweep
8. **Tectonic LFO** — ultra-slow pitch modulation (0.005-0.5 Hz, up to 20 cents)

**Unique Parameters:**
- `ogre_rootDepth` — how deep below the fundamental the sub extends (sub-harmonic mix level)
- `ogre_soil` — filter character (0=clay, 0.5=sandy, 1.0=rocky)
- `ogre_tectonicRate` / `ogre_tectonicDepth` — extremely slow pitch drift (continental movement)
- `ogre_density` — infrasound sub-frequency emphasis

---

## XOlate — Analog Bass / Aged Wine

**DSP Chain:**
1. **Dual PolyBLEP oscillators** (saw + variable-width pulse) with crossfade mix
2. **FermentationIntegrator** — leaky integrator accumulating tanh saturation over note sustain (complexity grows from simple to rich)
3. **Vintage-era drive** — saturation character changes by analog era (transistor/Moog/303/modern)
4. **Ladder filter** (CytomicSVF LP) — cutoff + resonance with vintage-era modifiers
5. **Filter envelope** — velocity-scaled sweep with independent attack/decay
6. **Warmth filter** — CytomicSVF low shelf for tube/transistor character
7. **Terroir** — regional circuit flavor bias (West Coast dark, UK mid-forward, etc.)
8. **Session aging** — LP cutoff gently decreases over 20 minutes of play

**Unique Parameters:**
- `olate_vintage` — analog era position (0=early transistor, 0.25=Moog, 0.5=303, 0.75=modern)
- `olate_warmth` — tube vs transistor low-frequency character
- `olate_terroir` — regional circuit voice (West Coast cool / East Coast grit / UK mid / Japanese transparent)
- `olate_ageRate` — fermentation speed: how fast notes grow harmonically rich during sustain

---

## XOaken — Acoustic/Upright Bass / Cured Wood

**DSP Chain:**
1. **OakenExciter** — 3 playing styles:
   - Pluck (0): decaying noise burst proportional to string period
   - Bow (1): sustained LP-filtered noise with bow pressure control
   - Slap (2): sharp click + wider noise burst for percussive attack
2. **KarplusStrongString** — delay line with filtered feedback:
   - Fractional-delay interpolation for precise pitch
   - String type affects feedback LP coefficient (gut=warm, steel=bright, synthetic=brightest)
   - Damping coefficient controls string decay time
3. **OakenBodyResonator** — 3-mode BPF bank (CytomicSVF) at wood body frequencies:
   - Mode 1: ~180-220 Hz (varies with wood age)
   - Mode 2: ~520-600 Hz
   - Mode 3: ~1000-1200 Hz
   - Wood age shifts frequencies and Q (old wood = lower, more resonant)
4. **CuringModel** — slow LP cutoff reduction during sustain (12kHz toward 500Hz)
5. **Output brightness filter** with velocity/envelope/LFO modulation

**Unique Parameters:**
- `oaken_exciter` — playing style (0=pluck, 1=bow, 2=slap)
- `oaken_bowPressure` — arco energy injection level (light harmonic to heavy grinding)
- `oaken_stringTension` — gut (0) vs steel (0.5) vs synthetic (1.0) — harmonic series weighting
- `oaken_woodAge` — fresh bright (0) to old dark (1) wood character
- `oaken_curingRate` — how fast the note loses HF content during sustain

---

## XOmega — FM/Digital Bass / Distillation

**DSP Chain:**
1. **2-operator FM synthesis:**
   - Carrier (sine via fastSin) + Modulator (sine with self-feedback)
   - Modulator output scales by mod index to produce phase modulation on carrier
   - 8 algorithm presets (ratio settings): unison, octave, fifth, sub-octave, m7, inharmonic, bell, custom
2. **DistillationModel** — mod index decays exponentially during note sustain:
   - Half-life configurable 2-20 seconds via distillRate
   - Complex FM spectrum simplifies toward pure carrier
   - Irreversible within the note — every note distills from complexity to purity
3. **Purity parameter** — at 1.0, mathematically clean FM. At 0.0:
   - Phase noise injection (subtle random modulation)
   - Frequency instability (slight random drift)
4. **Output LP filter** with velocity/envelope/LFO modulation

**Unique Parameters:**
- `omega_algorithm` — FM ratio preset (0-6: classic ratios, 7: custom using omega_ratio)
- `omega_ratio` — carrier:modulator frequency ratio (0.25-16.0)
- `omega_modIndex` — FM modulation depth (0-20)
- `omega_purity` — mathematical purity (1.0=clean, 0.0=noisy/unstable)
- `omega_distillRate` — how fast FM complexity reduces during sustain

---

## Gravitational Coupling — Shared Architecture

All 4 engines implement MIDI-domain gravitational coupling per CPU strategy decision C1:

```
Per voice:
  gravityMass accumulates during note sustain:
    mass(t) = min(1.0, mass(t-1) + dt * 0.1 * gravity_param)
    ~10 seconds to full mass at gravity=1.0
```

**What each engine broadcasts:**
- Current MIDI note (via active voice data)
- Gravity mass (accumulated weight)
- Gravity parameter value (user-controlled pull strength)

**How receiving engines use it (V2 — not implemented in V1):**
- Read active notes from CELLAR engine via SynthEngine interface
- Compute harmonic series from CELLAR fundamental (8 harmonics)
- Apply per-voice pitch displacement toward nearest harmonic at control rate
- Binary capture model: captured (within 30 cents * pull) or free

**CPU Cost:** ~3.1% per engine (standard subtractive/FM synthesis + ~0.1% gravity coupling)

---

## Doctrine Compliance

All 4 engines comply with all 6 doctrines:

| Doctrine | XOgre | XOlate | XOaken | XOmega |
|----------|-------|--------|--------|--------|
| D001 Velocity→Timbre | velocity scales brightness + saturation | velocity scales filter cutoff | velocity scales exciter energy + brightness | velocity scales FM brightness |
| D002 Modulation | 2 LFOs + tectonic LFO + mod wheel + aftertouch + 4 macros | 2 LFOs + mod wheel + aftertouch + 4 macros | 2 LFOs + mod wheel + aftertouch + 4 macros | 2 LFOs + mod wheel + aftertouch + 4 macros |
| D003 Physics IS Synthesis | N/A (not physically modeled) | N/A (analog emulation) | Karplus-Strong (Smith 1992) | Chowning FM (1973) |
| D004 No Dead Params | All params wired to DSP | All params wired to DSP | All params wired to DSP | All params wired to DSP |
| D005 Breathing | Tectonic LFO floor 0.005 Hz | LFO1/2 floor 0.005 Hz | LFO1/2 floor 0.005 Hz | LFO1/2 floor 0.005 Hz |
| D006 Expression | MW→drive, AT→body resonance | MW→cutoff, AT→resonance | MW→bow pressure, AT→damping | MW→mod index, AT→feedback |

---

## Shared DSP Utilities Used

All engines use the full shared DSP library:
- `CytomicSVF` — topology-preserving state-variable filter
- `PolyBLEP` — band-limited oscillators (XOgre, XOlate)
- `StandardLFO` — 5-shape LFO with D005-compliant rate floor
- `FilterEnvelope` — ADSR for filter/pitch modulation
- `GlideProcessor` — frequency-domain portamento
- `ParameterSmoother` — one-pole zipper-free parameter smoothing
- `VoiceAllocator` — LRU voice stealing
- `PitchBendUtil` — MIDI pitch wheel pipeline
- `SilenceGate` — zero-idle CPU bypass
- `FastMath` — denormal-safe fast approximations

---

## File Locations

| Component | Path |
|-----------|------|
| XOgre engine | `Source/Engines/Ogre/OgreEngine.h` |
| XOlate engine | `Source/Engines/Olate/OlateEngine.h` |
| XOaken engine | `Source/Engines/Oaken/OakenEngine.h` |
| XOmega engine | `Source/Engines/Omega/OmegaEngine.h` |
| Presets (40 total) | `Presets/XOceanus/{mood}/{Engine}_{Name}.xometa` |
| Visionary concept | `Docs/concepts/cellar-quad-visionary.md` |
| CPU strategy | `Docs/concepts/kitchen-cpu-optimization-strategy.md` |
| This architecture doc | `Docs/cellar-quad-architecture.md` |
