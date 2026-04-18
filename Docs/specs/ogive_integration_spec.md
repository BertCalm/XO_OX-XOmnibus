# XOgive Integration Specification

> Engine #86 — Scanned Glass Synthesis
> Gallery Code: OGIV | Accent: Selenium Ruby #9B1B30

## Identity

| Field | Value |
|-------|-------|
| Engine ID | Ogive |
| Display Name | XOgive |
| Gallery Code | OGIV |
| Parameter Prefix | ogv_ |
| Accent Color | #9B1B30 (Selenium Ruby) |
| Water Column | Twilight Zone (200-1000m) |
| Creature | Glass Sponge (hexactinellid / Euplectella aspergillum) |
| feliX-Oscar | 45/55 |
| Synthesis Method | Scanned glass synthesis (spring-mass surface + synthwave waveform init) |
| Max Voices | 8 |
| Seance Score | Pending |

## Concept

XOgive initializes a vibrating glass surface with classic synthwave waveforms (saw/pulse/tri/sine/noise), then lets glass physics (high Q, crystalline damping, nonlinear springs) evolve those shapes into shimmering, crystalline variants. A read-head scans the surface along an ogive (Gothic pointed arch) trajectory. The result: sounds with synthwave ancestry refracted through cathedral glass.

## Signal Flow

```
Synthwave Waveform Init (saw/pulse/tri/sine/noise)
  → Glass Surface (N spring-mass nodes, Verlet integration, cubic nonlinearity)
    → Note Trigger Excitation (velocity² energy, D001)
      → Ogive Scan Read-Head (cubic Hermite interpolation)
        → Surface Memory blend (0-100%, continuous retrigger↔continuous)
          → Neon Filter (CytomicSVF LP + tanh saturation)
            → Amplitude Envelope (StandardADSR)
              → Voice Sum
                → Juno-style Chorus (shared)
                  → Gated Reverb (4-line FDN, shared)
                    → DC Blocker → Output
```

## Parameter Table (29 parameters)

### Glass Surface
| ID | Label | Type | Range | Default | Description |
|----|-------|------|-------|---------|-------------|
| ogv_waveform | Waveform | Choice | Saw/Pulse/Tri/Sine/Noise | Saw | Initial surface shape — the synthwave DNA |
| ogv_wave_asym | Wave Asymmetry | Float | 0.1–0.9 | 0.5 | Waveform skew (saw ramp, pulse width, tri symmetry) |
| ogv_pane_count | Panes | Choice | 4/8/16/32/64 | 32 | Surface resolution — more panes = richer harmonics |
| ogv_tracery_stiff | Tracery | Float | 0.01–1.0 | 0.45 | Spring stiffness — low = rubber, high = crystal |
| ogv_lead_damp | Lead | Float | 0.001–0.2 | 0.02 | Boundary damping — low = infinite ring, high = quick decay |
| ogv_glass_q | Glass Q | Float | 1–200 | 60 | Resonance quality — defines the "glass-ness" |

### Scan
| ID | Label | Type | Range | Default | Description |
|----|-------|------|-------|---------|-------------|
| ogv_scan_rate | Scan Rate | Float | 0.01–20 Hz | 1.0 | Read-head speed |
| ogv_scan_pos | Scan Position | Float | 0.0–1.0 | 0.5 | Manual position (MW/expression target) |
| ogv_lancet_shape | Lancet | Float | 0.5–4.0 | 1.5 | Scan curvature: 1.0 = linear, >1 = Gothic arch |
| ogv_scan_mode | Scan Mode | Choice | Trigger/Free/Expr/Sync | Trigger | How the read head moves |
| ogv_sync_div | Sync Div | Choice | 1/16 – 4 bars | 1/4 | Beat division (Sync mode only) |
| ogv_surface_memory | Surface Memory | Float | 0.0–1.0 | 0.0 | Retrigger↔continuous blend (Dave Smith enhancement) |

### Neon Layer
| ID | Label | Type | Range | Default | Description |
|----|-------|------|-------|---------|-------------|
| ogv_neon_cutoff | Neon Cutoff | Float | 80–20k Hz | 4000 | Analog filter frequency |
| ogv_neon_reso | Neon Reso | Float | 0.0–1.0 | 0.3 | Filter resonance |
| ogv_neon_drive | Neon Drive | Float | 0.0–1.0 | 0.15 | tanh saturation — neon tube warmth |
| ogv_chorus_depth | Chorus | Float | 0.0–1.0 | 0.4 | Juno-style ensemble depth |
| ogv_chorus_rate | Chorus Rate | Float | 0.05–5.0 Hz | 0.3 | Chorus modulation speed |

### Space
| ID | Label | Type | Range | Default | Description |
|----|-------|------|-------|---------|-------------|
| ogv_gate_time | Gate Time | Float | 0.05–2.0 s | 0.3 | Gated reverb threshold |
| ogv_nave_size | Nave Size | Float | 0.0–1.0 | 0.5 | Reverb room size (chapel → cathedral) |
| ogv_nave_mix | Nave Mix | Float | 0.0–1.0 | 0.25 | Reverb wet/dry |

### Modulation
| ID | Label | Type | Range | Default | Description |
|----|-------|------|-------|---------|-------------|
| ogv_lfo1_rate | LFO Rate | Float | 0.01–20 Hz | 0.5 | Primary LFO (D005: floor 0.01 Hz) |
| ogv_lfo1_shape | LFO Shape | Choice | Sin/Tri/Saw/Sq/S&H | Sin | LFO waveform |
| ogv_lfo1_depth | LFO Depth | Float | 0.0–1.0 | 0.35 | LFO modulation amount → scan position |
| ogv_env_attack | Env Attack | Float | 0.001–5.0 s | 0.01 | Mod envelope attack |
| ogv_env_decay | Env Decay | Float | 0.01–10.0 s | 0.5 | Mod envelope decay |
| ogv_env_sustain | Env Sustain | Float | 0.0–1.0 | 0.7 | Mod envelope sustain |
| ogv_env_release | Env Release | Float | 0.01–10.0 s | 0.8 | Mod envelope release |

### Performance
| ID | Label | Type | Range | Default | Description |
|----|-------|------|-------|---------|-------------|
| ogv_voice_mode | Voice Mode | Choice | Poly/Mono/Legato | Poly | Voice allocation mode |
| ogv_thermal | Thermal | Float | 0.0–1.0 | 0.0 | Schulze drift — spring constants shift across notes |

## Macro Mapping

| Macro | Label | Controls |
|-------|-------|----------|
| M1 | Glass (CHARACTER) | tracery_stiff ↑, glass_q ↑, wave_asym sweep |
| M2 | Scan (MOVEMENT) | scan_rate ↑, lancet_shape ↑, lfo1_depth → scan_pos |
| M3 | Tracery (COUPLING) | coupling amount, tracery_stiff ↓, lead_damp ↓ |
| M4 | Neon (SPACE) | neon_drive ↑, chorus_depth ↑, nave_mix ↑ |

## Expression Mapping (D006)

| Source | Target | Domain |
|--------|--------|--------|
| Velocity | Excitation energy (velocity², nonlinear) | Initialization |
| Aftertouch | Scan position offset | Spatial |
| Mod Wheel (CC1) | Scan position (absolute) | Navigational |
| Expression (CC11) | Neon drive | Processing |

## Coupling Contract

**Role**: BILATERAL (sender + receiver)

**getSampleForCoupling()**: Raw scanned surface output, PRE neon filter. Spectrally rich, evolving.

**applyCouplingInput()** receives:

| CouplingType | Effect | Safety |
|-------------|--------|--------|
| AudioToFM | Modulates scan rate | Standard |
| SpectralShaping | Adds excitation to glass surface | Clamped to ±1.0 × 0.25 |
| PhaseSync (receive only) | Nudges scan phase toward source | Clamped ±0.1 nodes/block |
| Default | AmpToFilter — modulates neon cutoff | Standard |

**Anti-patterns** (do NOT use):
- XOgive as PhaseSync SENDER (non-uniform velocity causes lurching)
- OXBOW bidirectional at equal depth (feedback runaway)
- OUROBOROS → XOgive via FrequencyModulation (use PhaseSync instead)

**Best Coupling Partners**:
1. OBSCURA → XOgive (SpectralShaping): "Cathedral Glass"
2. XOgive → OPERA (AudioToFM): "Neon Choir"
3. OXBOW ↔ XOgive (bidirectional, asymmetric): "Tidal Architecture"
4. OUROBOROS → XOgive (PhaseSync): "The Leash on Glass"
5. OXYTOCIN ↔ XOgive (FM + AmpToFilter): "Circuit Love Letter"
6. XOgive → ORPHICA (AudioToBuffer): "Frozen Granular Memory"

## Coupling Identity

> XOgive is the **slow cartographer** of the coupling ecosystem — its deterministic ogive trajectory traces a consistent, unhurried path across spectral territory, making it an ideal modulation source for engines needing long-form coherent drift and an ideal receiver for engines whose chaos needs a crystalline anchor point.

## Guru Bin Blessings (4 candidates)

| Blessing | Values | Significance |
|----------|--------|-------------|
| The Venetian Frequency | Q=144, stiffness=0.55 | Cristallo glass resonance — 1.5 semitone offset |
| The Gothic Corridor | lancet_shape=2.618 | Golden ratio arch — uncanny forward motion |
| The Lead Line | lead_damp=0.014, panes=32 | Ring-down = 16th note at 120 BPM — structural rhythm |
| The Neon Eucharist | drive=0.33, reso=0.67 | 1:2 ratio reinforces 3rd harmonic of saturation |

## DSP Implementation Notes

- **Verlet integration** with ±4.0 magnitude clamp (prevents NaN at high Q + stiffness)
- **Cubic spring nonlinearity**: F = kx + k₂x³ (D001: velocity → timbre, not just amplitude)
- **Cubic Hermite interpolation** for fractional scan positions (Architect Contract #2)
- **Fixed boundary conditions**: pos[0] = pos[N-1] = 0 (clamped plate model)
- **SilenceGate bypasses Verlet loop entirely** for inactive voices (CPU critical)
- **Key tracking on damping**: higher notes damp faster (noteFreq/440.0 multiplier)
- **Thermal drift**: Brownian walk in spring constants, accumulates across note boundaries
- **Surface Memory**: captures smoothed parameter value atomically at note-on (thread safety)
- **Saturation**: tanh-family (odd harmonics) for neon tube character

## Shared DSP Usage

StandardLFO, StandardADSR, CytomicSVF, FastMath, VoiceAllocator, ModMatrix<4>, ParameterSmoother, PitchBendUtil, GlideProcessor, DCBlocker, SilenceGate, Saturator (Tube mode)

## Preset Library (8 seed presets)

| Name | Mood | Archetype |
|------|------|-----------|
| First Light | Foundation | Init |
| Rose Window | Crystalline | Signature |
| Fracture Point | Shadow | Extreme |
| Neon Tracery | Kinetic | Synthwave Anchor |
| Cold Cathedral | Deep | Darkwave Anchor |
| Orbital Rosette | Ethereal | Cinematic Anchor |
| Vespers | Atmosphere | Breath |
| OBSCURA Incident | Coupling | Coupling Demo |

Target: 136 presets across 11 categories (Synthwave 22, Darkwave 18, Cinematic 18, Ambient 15, Chillwave 12, Experimental 12, Game Audio 10, Vaporwave 8, Expression 8, Coupling 8, Init+Utility 5).

## Review Council Scores

| Council | Gate | Score |
|---------|------|-------|
| Synth Seance | 8.0+ required | 8.6/10 — PASS |
| Producers Guild | 5+ excited | 10 excited — PASS |
| Guru Bin | 5+ viable presets | 8 seed presets — PASS |
| Architect | 5 provinces | CONDITIONAL PASS (11 contracts, all resolved) |
| Coupling Cookbook | — | 6 recipes, 3 anti-recipes |

## Files

| Path | Purpose |
|------|---------|
| Source/Engines/Ogive/OgiveEngine.h | Engine implementation (1,532 lines) |
| Source/Engines/Ogive/OgiveEngine.cpp | One-line stub |
| Presets/XOceanus/Foundation/First_Light.xometa | Init preset |
| Presets/XOceanus/Crystalline/Rose_Window.xometa | Signature preset |
| Presets/XOceanus/Shadow/Fracture_Point.xometa | Extreme preset |
| Presets/XOceanus/Kinetic/Neon_Tracery.xometa | Synthwave anchor |
| Presets/XOceanus/Deep/Cold_Cathedral.xometa | Darkwave anchor |
| Presets/XOceanus/Ethereal/Orbital_Rosette.xometa | Cinematic anchor |
| Presets/XOceanus/Atmosphere/Vespers.xometa | Breath preset |
| Presets/XOceanus/Coupling/Obscura_Incident.xometa | Coupling demo |

## Vocabulary

| Term | Meaning |
|------|---------|
| Pane | A node in the scanned glass surface |
| Lancet | The pointed-arch scan trajectory |
| Tracery | Spring network connecting glass panels |
| Rosette | Circular scan mode (rose window) |
| Lead | Damping at node boundaries (lead came) |
| Nave | Central resonance / output mixing |
| Neon | The analog warmth/saturation layer |
| Arcade | Row of arches / 80s game room (preset bank) |
