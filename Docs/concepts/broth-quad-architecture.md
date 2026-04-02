# BROTH Quad — Architecture Document

*XO_OX Designs | 2026-03-21 | Pads Instrument Family*

---

## Overview

The BROTH quad is a family of 4 cooperative pad engines with fundamentally different ontological relationships to time. They share environmental state (cooperative coupling) and age together as a system. Together they form the Pads family within the Kitchen Collection.

## The 4 Engines

### XOverwash — Diffusion Pad (Infusion)
- **Source**: `Source/Engines/Overwash/OverwashEngine.h`
- **Prefix**: `wash_` | **Color**: Tea Amber `#D4A76A`
- **Time scale**: SECONDS (3–30s diffusion arc)
- **Physics**: Fick's Second Law of Diffusion (analytical Gaussian, no FFT)
- **Voice count**: 8 polyphonic
- **Params**: 29 (9 core + 4 filter + 4 amp ADSR + 4 filt ADSR + 4 LFO + 1 level + 4 macros + 3 tone)
- **DSP chain**: 16-partial oscillator bank → analytical Gaussian diffusion spread → viscosity LP filter → amp envelope → stereo placement
- **Key behavior**: Notes are drops of ink in water. Harmonics spread outward from the fundamental over time. Multiple notes create interference fringes where diffusion fronts overlap.
- **Presets**: 10 (Dashi, Sencha Bloom, Ink Drop, Saffron Thread, Gradient Field, Chamomile, Watercolor, Kombu Dawn, Pigment Collision, Still Water)

### XOverworn — Erosion Pad (Reduction)
- **Source**: `Source/Engines/Overworn/OverwornEngine.h`
- **Prefix**: `worn_` | **Color**: Reduced Wine `#4A1A2E`
- **Time scale**: SESSION-LONG (30+ minutes)
- **Physics**: Reduction Integral with frequency-dependent evaporation
- **Voice count**: 8 polyphonic
- **Params**: 30 (9 core + 4 filter + 4 amp ADSR + 4 filt ADSR + 4 LFO + 1 level + 4 macros)
- **DSP chain**: 16-partial oscillator bank (richness-controlled) → ReductionState spectral envelope → Maillard distortion → umami fundamental boost → LP filter → amp envelope → stereo
- **Key behavior**: The most radical engine in XOceanus. ReductionState persists across the session. High frequencies evaporate first, then mids. Fundamentals concentrate (umami). Playing accelerates reduction. Quiet long tones add character without accelerating. Only "Start Fresh" resets.
- **ReductionState fields**: sessionAge, spectralMass[8], concentrateDark, umamiBed, volatileAromatics
- **Presets**: 10 (Fresh Stock, Hour One, Quick Fond, Demi-Glace, Consomme, Bone Broth, Volatile Bright, Reduction Drone, Mid-Session, Maillard Fire)

### XOverflow — Pressure Pad (Pressure Cooking)
- **Source**: `Source/Engines/Overflow/OverflowEngine.h`
- **Prefix**: `flow_` | **Color**: Steam White `#E8E8E8`
- **Time scale**: PHRASES (accumulation-based, not clock-based)
- **Physics**: Clausius-Clapeyron (pressure/temperature phase transitions)
- **Voice count**: 8 polyphonic
- **Params**: 28 (8 core + 3 filter + 4 amp ADSR + 4 filt ADSR + 4 LFO + 1 level + 4 macros)
- **DSP chain**: 16-partial oscillator bank → pressure-dependent strain modification (high-freq grating, low-freq tightening, beating) → over-pressure saturation → valve release (gradual/explosive/whistle) → LP filter → amp envelope → stereo
- **Key behavior**: Pressure accumulates from MIDI input density, velocity, and interval dissonance. At threshold: valve release event. Three valve types: gradual bleed, explosive burst + silence, pitched steam whistle. Over-pressure catastrophic state if player never releases. The pad has consequences.
- **PressureState fields**: pressure, temperature, strainLevel, valveOpen, valveTimer, overPressure
- **Presets**: 10 (Sealed Vessel, Steam Whistle, Explosion, Gentle Simmer, Pressure Drop, Iron Kettle, Boiling Point, Tea Kettle, Volcano, Contained Heat)

### XOvercast — Crystallization Pad (Flash Freeze)
- **Source**: `Source/Engines/Overcast/OvercastEngine.h`
- **Prefix**: `cast_` | **Color**: Ice Blue `#B0E0E6`
- **Time scale**: INSTANT (no evolution between triggers)
- **Physics**: Wilson's Nucleation Theory (spectral peak identification + propagation)
- **Voice count**: 8 polyphonic
- **Params**: 24 (9 core + 2 filter + 4 amp ADSR + 4 LFO + 1 level + 4 macros)
- **DSP chain**: Spectral peak identification (nucleation sites) → crystallization propagation window (20–200ms with crackling) → frozen spectrum lock → lattice harmonic snap → LP filter → amp envelope → fixed stereo
- **Key behavior**: The anti-pad. Does NOT evolve between triggers. On note-on: captures spectral state, nucleates crystal from peaks, propagates outward during crystallization window (audible crackling). Then locks. Holds indefinitely until next trigger. Three transition modes: instant (no gap), crystal (50–200ms crackling), shatter (break + silence + reform).
- **CrystalState fields**: peakFreqs[16], peakAmps[16], crystalProgress[16], isFrozen, freezeTimer
- **Presets**: 10 (Flash Freeze, Pure Crystal, Frost Bite, Shatter Glass, Slow Ice, Diamond Dust, Black Ice, Crackle Glass, Permafrost, Snowflake)

---

## Cooperative Coupling (BROTH Shared State)

All 4 engines share environmental state through XOverworn's ReductionState:

| Consumer | XOverworn Export | Effect |
|----------|-----------------|--------|
| XOverwash | `sessionAge` | Increases viscosity (diffusion slows) |
| XOverflow | `concentrateDark` | Lowers pressure threshold (concentrated broth builds pressure faster) |
| XOvercast | `spectralMass` | Fewer/darker nucleation sites (dark ice) |

### API Methods

```cpp
// XOverworn exports (read by other BROTH engines):
float getSessionAge() const;
float getConcentrateDark() const;
float getTotalSpectralMass() const;

// XOverwash reads:
void setBrothSessionAge(float age);

// XOverflow reads:
void setBrothConcentrateDark(float dark);

// XOvercast reads:
void setBrothSpectralMass(float mass);
```

The coupling is environmental, not signal-based. The four engines orbit around XOverworn's memory. As the session ages, ALL four engines change character together. The BROTH quad ages in concert.

---

## Doctrine Compliance

| Doctrine | WASH | WORN | FLOW | CAST |
|----------|------|------|------|------|
| D001: Velocity→Timbre | Brightness | Reduction accel | Pressure input | Crystal density |
| D002: Modulation | 2 LFO + MW + AT + 4M | 2 LFO + MW + AT + 4M | 2 LFO + MW + AT + 4M | 2 LFO + MW + AT + 4M |
| D003: Physics | Fick's Law | Reduction Integral | Clausius-Clapeyron | Wilson Nucleation |
| D004: No Dead Params | All wired | All wired | All wired | All wired |
| D005: Breathing | BreathingLFO 0.008Hz | BreathingLFO 0.007Hz | BreathingLFO 0.009Hz | BreathingLFO 0.006Hz |
| D006: Expression | AT→viscosity, MW→diff | AT→heat, MW→richness | AT→pressure, MW→vessel | AT→freeze, MW→purity |

---

## CPU Budget

All 4 engines use analytical Gaussian diffusion (no FFT). Per the Kitchen CPU optimization strategy:
- Per engine: ~1.5–2.5% CPU (16 partials x 8 voices x sine + filter)
- Quad total: ~6–10% (well under 18% quad budget)
- With BROTH coupling overhead: negligible (shared state is float reads, not DSP)

---

## File Manifest

```
Source/Engines/Overwash/
  OverwashEngine.h          (engine header)
  Presets/Foundation/        (2 presets: Dashi, Still Water)
  Presets/Atmosphere/        (4 presets)
  Presets/Prism/             (1 preset)
  Presets/Aether/            (1 preset)
  Presets/Flux/              (1 preset)

Source/Engines/Overworn/
  OverwornEngine.h          (engine header)
  Presets/Foundation/        (2 presets)
  Presets/Atmosphere/        (3 presets)
  Presets/Flux/              (2 presets)
  Presets/Prism/             (1 preset)
  Presets/Aether/            (1 preset)
  Presets/Submerged/         (1 preset)

Source/Engines/Overflow/
  OverflowEngine.h          (engine header)
  Presets/Foundation/        (2 presets)
  Presets/Atmosphere/        (3 presets)
  Presets/Flux/              (4 presets)
  Presets/Prism/             (1 preset)

Source/Engines/Overcast/
  OvercastEngine.h          (engine header)
  Presets/Foundation/        (2 presets)
  Presets/Prism/             (3 presets)
  Presets/Atmosphere/        (1 preset)
  Presets/Flux/              (2 presets)
  Presets/Aether/            (1 preset)
  Presets/Submerged/         (1 preset)
```

Total: 4 engine headers + 40 presets + this architecture doc.
