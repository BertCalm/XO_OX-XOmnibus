# XOpal — Concept Brief

**Date:** March 2026
**Phase:** 0 — Ideation complete
**Status:** Approved for Phase 1 architecture

---

## Identity

- **Name:** XOpal
- **Gallery code:** OPAL
- **Thesis:** XOpal is a granular synthesis engine that fragments any sound into time-scattered particles — from smooth stretched clouds to shattered glass.
- **Sound family:** Texture / Pad / Ambient / FX hybrid
- **Unique capability:** Real-time granulation of the XOmnibus coupling bus — any other engine's audio can enter XOpal's grain buffer and be scattered through time. NES pulses fragmented. Climax blooms frozen. Bass clouds sustained. This doesn't exist elsewhere.

---

## Character

- **Personality in 3 words:** Iridescent, Fragmented, Suspended
- **Engine approach:** Granular synthesis — real-time grain scheduler over a ring-buffer source
- **Why granular serves the character:**
  An opal shifts color depending on how you look at it — the same light, scattered differently. Granular synthesis does exactly this to sound: the same audio fragment, scattered across pitch, time, and space. The character isn't in the oscillators; it's in the *scattering*.
- **The coupling thesis:**
  Alone, XOpal has its own oscillator bank as grain source. But its real voice is as a *transformer* — it takes a living engine's output (OVERWORLD, ODYSSEY, OBLONG) and applies time to it. Every XOmnibus engine becomes richer when it can be scattered.

---

## The XO Concept Test

1. **XO word:** XOpal ✓ — Opal: precious stone, iridescent shifting, layered mineral structure
2. **One-sentence thesis:** "XOpal is a granular synthesis engine that fragments any sound into time-scattered particles — from smooth stretched clouds to shattered glass." ✓
3. **Sound only this can make:** Real-time granulation of chip synthesis audio — NES pulses scattered through 300ms grain clouds with pitch scatter ±5 semitones. No DAW plugin or other synth in the XOmnibus gallery does this. ✓

---

## Gallery Gap Filled

| Existing engines | Synthesis dimension |
|-----------------|---------------------|
| ODDFELIX, ODDOSCAR, ODYSSEY, OBLONG, OBESE, OVERBITE, OVERWORLD | Harmonic (oscillators, wavetables, FM, samples) |
| OVERDUB | Temporal (delays, tape echo) |
| **OPAL** | **Granular time — the missing dimension** |

No current engine manipulates *time* as its primary synthesis mechanism. OPAL introduces freeze, stretch, scatter, and cloud density as musical parameters rather than effects.

---

## Core DSP Architecture (Phase 0 sketch)

```
Source Selection
├── Built-in: OSC bank (sine / saw / pulse / noise / two-osc)
└── Coupling: External audio from any engine (ring buffer write)
        │
        ▼
Grain Buffer (ring buffer, 4 seconds, 44100 × 4 × 4 bytes ≈ 700KB)
        │
        ▼
Grain Scheduler
├── Density: 1-120 grains/sec
├── Position: playhead within buffer (0-1) + scatter
├── Size: 10ms - 800ms
└── Window: Hann / Gaussian / Tukey / Rectangular
        │
        ▼
Grain Voices × 32 (maximum simultaneous)
├── Pitch shift: base note + scatter ±semitones
├── Pan: stereo scatter
└── Amplitude: per-grain envelope (window shape)
        │
        ▼
Cloud Mix (summed grain output, normalized)
        │
        ▼
Filter (Cytomic SVF — LP / BP / HP)
        │
        ▼
Character Stage (Shimmer: harmonic fold / Frost: cold limiting)
        │
        ▼
Amp Envelope (ADSR — typically slow, cloud-appropriate)
        │
        ▼
FX Chain
├── Freeze (loop a buffer region — freeze ratio)
├── Scatter Reverb (grain-informed room)
└── Smear (time-stretch without pitch change)
        │
        ▼
Output (stereo)
```

**Voice model:** Each MIDI note triggers a grain cloud pitched to that note. Up to 12 simultaneous clouds (polyphonic). The grain scheduler is shared; each cloud has its own pitch offset and envelope position.

**Grain count:** 32 simultaneous grains maximum (across all voices). Each grain is a short windowed overlap-add segment — cheap DSP per grain, cost scales with density.

---

## Parameter Namespace

All parameter IDs use `opal_` prefix. Key parameters:

| ID | Range | Description |
|----|-------|-------------|
| `opal_source` | 0-5 | Grain source: sine/saw/pulse/noise/osc2/coupling |
| `opal_grainSize` | 10-800ms | Grain window duration |
| `opal_density` | 1-120/s | Grains per second |
| `opal_position` | 0-1 | Playhead in grain buffer |
| `opal_posScatter` | 0-1 | Position randomization |
| `opal_pitchScatter` | 0-24st | Pitch randomization per grain |
| `opal_panScatter` | 0-1 | Stereo scatter width |
| `opal_window` | 0-3 | Grain window shape |
| `opal_freeze` | 0-1 | Buffer freeze ratio |
| `opal_couplingLevel` | 0-1 | External engine audio level into grain buffer |
| `opal_filterCutoff` | 20-20000Hz | Post-cloud filter |
| `opal_filterReso` | 0-1 | Filter resonance |
| `opal_shimmer` | 0-1 | Harmonic fold character stage |
| `opal_frost` | 0-1 | Cold limiting character stage |
| `opal_smear` | 0-1 | Time-stretch FX |
| `opal_reverbMix` | 0-1 | Scatter reverb wet level |
| `opal_ampAttack` | 0.001-8s | Cloud attack (often slow) |
| `opal_ampDecay` | 0.05-4s | Cloud decay |
| `opal_ampSustain` | 0-1 | Cloud sustain level |
| `opal_ampRelease` | 0.05-8s | Cloud release (often slow) |

*Full parameter list defined in Phase 1 architecture.*

---

## Macro Mapping (M1-M4)

| Macro | Label | Controls | Behavior |
|-------|-------|----------|----------|
| M1 | SCATTER | `opal_grainSize` + `opal_density` inverse | Small: shimmer (tiny dense grains). Large: stretched clouds. The texture dial. |
| M2 | DRIFT | `opal_posScatter` + `opal_pitchScatter` + `opal_panScatter` | 0=laser-focused. 1=total dissolution. The spread dial. |
| M3 | COUPLING | `opal_couplingLevel` + `opal_freeze` | Controls how much external audio enters and how much of it is frozen. The portal. |
| M4 | SPACE | `opal_reverbMix` + `opal_smear` | Spatial expansion. How big the room is around the cloud. |

All 4 macros produce audible, significant change at every point in their range in every preset.

---

## Coupling Interface Design

### OPAL as Target (receiving from other engines)

| Coupling Type | What XOpal Does | Musical Effect |
|---------------|----------------|----------------|
| `AudioToWavetable` | Writes source audio into grain buffer | Any engine's sound becomes OPAL's grain source — the core coupling |
| `AmpToFilter` | Source amplitude → filter cutoff | Drum hits open OPAL's filter — rhythmic cloud filtering |
| `EnvToMorph` | Source envelope → grain size | Attack shapes → grain size variation |
| `LFOToPitch` | Source LFO → pitch scatter | Cross-engine modulation of the scatter width |

**Primary coupling:** `AudioToWavetable` — this is the reason OPAL exists in XOmnibus. Every engine feeding into OPAL creates a unique "engine through time" voice.

### OPAL as Source (sending to other engines)

`getSampleForCoupling()` returns: post-filter cloud output, stereo, normalized ±1.

Good receiving engines:
- **OVERDUB** — grain cloud through dub echo. Granular dub.
- **ODDOSCAR** — cloud amplitude modulates wavetable morph position
- **OVERBITE** — cloud density drives filter cutoff. Breathing bass.

### Coupling types OPAL should NOT receive
- `AmpToChoke` — choking a cloud kills the entire texture (no musical use)
- `PitchToPitch` — grain pitch scatter already provides pitch variation; external pitch coupling creates confusion

---

## Visual Identity

- **Accent color:** Iridescent Lavender `#A78BFA`
  - Distinguishes from ODYSSEY's dark violet `#7B2D8B`
  - Suggests precious stone quality, light refraction, crystalline
- **Material/texture:** Thin slices of precious opal — the layered mineral structure, the fire inside
- **Gallery panel character:** Translucent layers. The panel surface should feel like looking through glass at depth behind it. Subtle parallax if animation budget allows.
- **Icon concept:** A fractal grain cloud — concentric scatter of dots at various opacities, suggesting both the opal stone and the granular particle cloud

---

## Mood Affinity

| Mood | Affinity | Why |
|------|----------|-----|
| Foundation | Low | Granular is rarely "stable" — that's not its strength |
| Atmosphere | **High** | Cloud synthesis is the definitive atmospheric tool |
| Entangled | **High** | Coupling-focused presets are OPAL's killer app |
| Prism | Medium | Bright shimmer presets can be Prism-adjacent |
| Flux | **High** | Scatter + drift presets feel unstable and evolving |
| Aether | **High** | Frozen, suspended, near-silent cloud pads |

Primary moods: Atmosphere, Entangled, Flux, Aether.

---

## Preset Strategy (Phase 0 sketch)

**100 presets at v1.0:**

| Category | Count | Character |
|----------|-------|-----------|
| Smooth Clouds | 20 | Large grains, low scatter, atmospheric pads |
| Shimmer Textures | 20 | Small grains, high density, bright airy shimmer |
| Frozen Moments | 15 | High freeze, slow movement, suspended pads |
| Scattered Glass | 15 | High pitch scatter, rhythmic density, percussive texture |
| Coupling Showcases | 20 | Designed for use with specific engine pairs (OVERWORLD×OPAL, ODYSSEY×OPAL, etc.) |
| Deep Drift | 10 | Max scatter, position wander, generative/ambient |

---

## Decision Gate: Phase 0 → Phase 1

- [x] Concept brief written
- [x] XO word feels right (XOpal — opal metaphor is authentic to the sound)
- [x] Gallery gap is clear (no time-domain synthesis engine exists)
- [x] At least 2 coupling partner ideas exist (OVERWORLD, ODYSSEY, OVERBITE, OVERDUB)
- [x] Excited about the sound
- [x] Unique capability defined (chip audio granulation)

**→ Proceed to Phase 1: Architect**
*Invoke: `/new-xo-engine phase=1 name=XOpal identity="Granular synthesis engine that fragments any sound into time-scattered particles" code=XOpl`*

---

*XO_OX Designs | Engine: OPAL | Accent: #A78BFA | Prefix: opal_*
