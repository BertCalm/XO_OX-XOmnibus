# XOuroboros — XOlokun Integration Spec

## Engine Identity
- **Engine ID:** `"OuroborosEngine"`
- **XOlokun Short Name:** OUROBOROS
- **Accent Color:** Strange Attractor Red `#FF2D2D`
- **Max Voices:** 6
- **CPU Budget:** <22% single-engine, <28% in dual-engine preset

## Parameter Namespace
- **Prefix:** `ouro_`
- **Total params:** 8
- **Key params for coupling targets:**
  - `ouro_chaosIndex` — bifurcation parameter (0.0–1.0)
  - `ouro_damping` — LP smoothing of attractor output (0.0–1.0)
  - `ouro_theta` — 3D→2D projection X rotation (-π to π)
  - `ouro_phi` — 3D→2D projection Y rotation (-π to π)
  - `ouro_injection` — coupling audio displacement into ODE velocity (0.0–1.0)
  - `ouro_rate` — target pitch frequency (0.01–20000 Hz)
  - `ouro_leash` — free-running vs hard-synced blend (0.0–1.0)
  - `ouro_topology` — ODE system selector (0=Lorenz, 1=Rössler, 2=Chua, 3=Aizawa)

## Coupling Compatibility

### As Coupling Source (OUROBOROS → others)
| Channel | Content | Use Case |
|---------|---------|----------|
| 0 | Left projected audio | Standard audio coupling |
| 1 | Right projected audio | Standard audio coupling |
| 2 | Normalized dx/dt velocity [-1, 1] | Non-repeating complex LFO for filter, morph, pitch |
| 3 | Normalized dy/dt velocity [-1, 1] | Decorrelated complex LFO from ch2 |

### As Coupling Target (others → OUROBOROS)
| Coupling Type | What It Does | Musical Effect |
|---------------|-------------|----------------|
| `AudioToFM` | Audio injected into dx/dt, scaled by `ouro_injection` | Orbit perturbation — rhythmic chaos |
| `AudioToWavetable` | Audio injected into dy/dt, scaled by `ouro_injection` | Orthogonal orbit perturbation |
| `RhythmToBlend` | Rhythm signal modulates Chaos Index | Beat-synced bifurcation transitions |
| `EnvToDecay` | Envelope modulates Damping | Dynamic smoothing — sharp attack, warm sustain |
| `AmpToFilter` | Source amplitude modulates Damping | Amplitude-reactive warmth |
| `EnvToMorph` | Envelope modulates Theta | Envelope-driven harmonic rotation |
| `LFOToPitch` | LFO modulates Rate / step size | Standard pitch vibrato |
| `PitchToPitch` | Pitch tracking modulates Rate / step size | Harmonic tracking |

### Unsupported Coupling Types
| Type | Why |
|------|-----|
| `AudioToRing` | Ring mod doesn't map to ODE perturbation — the attractor IS the waveform |
| `FilterToFilter` | No traditional filter chain in signal path |
| `AmpToChoke` | Killing attractor mid-orbit resets state → click |
| `AmpToPitch` | Redundant with PitchToPitch; creates feedback loops with the Leash |

## Macro Mapping
| Macro | Label | Target Parameter(s) | Musical Effect |
|-------|-------|---------------------|----------------|
| M1 | CHARACTER | `ouro_chaosIndex` + `ouro_damping` | Serene stable loop ↔ screaming torn chaos |
| M2 | MOVEMENT | `ouro_theta` + `ouro_phi` | Walk around the sound sculpture |
| M3 | COUPLING | `ouro_injection` + `ouro_leash` | External perturbation depth + pitch tightness |
| M4 | SPACE | `ouro_damping` + `ouro_rate` | Warm slow drift ↔ bright fast orbit |

## Preset Count
- **Total:** 20 presets in `.xometa` format
- **By mood:**
  - Foundation: 4 (Strange Loop, Steady State, Warm Attractor, Gravity Well)
  - Atmosphere: 3 (Butterfly Effect, Fog Machine, Dark Matter)
  - Entangled: 4 (Event Horizon, Perturbation Engine, Symbiotic Chaos, Orbital Capture)
  - Prism: 3 (Feral Oscillator, Bifurcation Point, Double Scroll)
  - Flux: 3 (Phase Portrait, Orbit Decay, Pendulum Swing)
  - Aether: 3 (Deterministic Ghost, Final Orbit, Strange Attractor)

## Visual Identity
- **Accent color:** Strange Attractor Red `#FF2D2D`
- **Material/texture:** Glowing wireframe phase-space trajectory — the 3D attractor rendered live
- **Icon concept:** Ouroboros (serpent eating its tail) — circular, self-referential, infinite
- **Panel character:** Attractor visualization pulses and writhes with Chaos Index. Low chaos = clean loops; high chaos = tangled space-filling trajectories. The Leash visualized as the serpent's jaw — tighter grip = smaller, more regular loops.

## Registration

### Engine Adapter
OuroborosEngine implements `SynthEngine` directly — no separate adapter needed. The engine header lives at:
```
Source/Engines/Ouroboros/OuroborosEngine.h
```

### Registration File
```
Source/Engines/Ouroboros/OuroborosEngine.cpp
```
Contains:
```cpp
#include "OuroborosEngine.h"
REGISTER_ENGINE(OuroborosEngine)
```

### CMakeLists.txt Addition
Add to the engine adapters section:
```cmake
Source/Engines/Ouroboros/OuroborosEngine.h
Source/Engines/Ouroboros/OuroborosEngine.cpp
```

## Best Coupling Partners

| Partner | Route | Musical Effect |
|---------|-------|----------------|
| ODDFELIX | ODDFELIX → Ouroboros via `AudioToFM` | Percussive hits knock the orbit into new trajectories |
| ORGANON | Ouroboros → Organon via `AudioToFM` | Chaotic audio feeds the metabolic organism |
| ODYSSEY | ODYSSEY → Ouroboros via `EnvToMorph` | Long envelopes slowly rotate projection angle |
| ODDOSCAR | Ouroboros → OddOscar via ch2/3 `LFOToPitch` | Velocity vectors as complex LFOs for wavetable |
| ONSET | ONSET → Ouroboros via `AudioToFM` | Drum patterns as physical forces on the orbit |
| OBLONG | Ouroboros → Oblong via ch2/3 `EnvToMorph` | Chaotic velocity vectors as non-repeating morph source |

## Integration Checklist
- [x] Engine implements `SynthEngine` interface
- [x] All parameters use `ouro_` prefix
- [x] `getSampleForCoupling()` supports 4-channel output
- [x] `applyCouplingInput()` handles all supported coupling types
- [x] `createParameterLayout()` returns 8 properly namespaced parameters
- [x] 20 factory presets in `.xometa` format
- [x] CPU budget verified (<2% single-engine — well under 22% limit)
- [x] EngineProfiler integrated
- [x] Denormal protection in feedback paths
- [x] REGISTER_ENGINE macro in .cpp file
- [x] Added to CMakeLists.txt
- [x] Cross-engine coupling presets created (6 presets: Orbit Smash, Chaos Metabolism, Manifold Drift, Velocity Morph, Beat Force, Strange Bass)
