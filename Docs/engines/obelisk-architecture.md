# XObelisk Architecture — "The Cold Monolith"

*KITCHEN Quad #3 | Stone/Marble Prepared Piano | March 2026*

---

## Identity

| Property | Value |
|----------|-------|
| Engine ID | `Obelisk` |
| Parameter prefix | `obel_` |
| Accent color | Obsidian Slate `#4A4A4A` |
| Voices | 8 |
| Parameters | 23 (+ 4 macros + 6 LFO = 30 total with `obel_` prefix) |
| Material | Stone / Marble |
| Piano type | Prepared Piano (John Cage, 1938-1975) |
| CPU target | < 3.5% @ 44.1kHz, 8 voices, 512 buffer |

---

## Creature

XObelisk is a monolith of cold marble standing in salt water, half-submerged, half-reaching skyward. Its surface is perfectly smooth except where some hand — Cage's hand — has placed objects between the strings that run through its body like veins through stone. Strike it and the marble rings: pure, cold, inharmonic. Press a bolt into the strings and it rattles. Weave rubber through the nodes and the stone hums through muted teeth. Glass laid across the strings adds its own crystalline voice. Chain draped over them buzzes like a bridge made of bees.

---

## DSP Architecture

### Signal Flow

```
MIDI Note → Hammer Contact Model (Hunt-Crossley, stone alpha)
         → Modal Resonator Bank (16 modes, inharmonic stone ratios)
           → Preparation Modification (per-mode freq/Q/amp changes)
           → Bolt Rattle (AM on contacted modes)
         → Chain Buzzer (post-modal nonlinear contact)
         → HF Noise Shaper (stone radiation character)
         → Filter Envelope + LFO1 → CytomicSVF LP
         → Amplitude Envelope (decay coefficient)
         → Stereo Pan (pitch-mapped)
         → Output
```

### Core Components

#### 1. Modal Resonator Bank (16 modes)

Stone/marble partials are **inharmonic** — they do NOT follow integer multiples of the fundamental. The mode ratios are derived from plate vibration theory (Fletcher & Rossing 1998, Table 2.2) adapted for marble's Poisson ratio (nu = 0.27):

```
Mode:    1      2      3      4      5      6      7      8
Ratio:   1.00   2.76   5.40   8.93   13.34  18.64  24.81  31.87
         9      10     11     12     13     14     15     16
         39.81  48.63  58.34  68.93  80.40  92.75  106.0  120.1
```

Each mode is a 2nd-order IIR resonator:
```
y[n] = b0*x[n] + a1*y[n-1] - a2*y[n-2]
```

Per-mode Q: 200-1000 (extremely high — stone rings clearly). Q scales with density parameter and falls off with mode number.

**Dynamic mode pruning**: modes below -120 dB are skipped to save CPU.

#### 2. Preparation System (HERO FEATURE)

Five preparation types, each modifying the modal bank:

| Type | Effect | Physics |
|------|--------|---------|
| **None** | Pure stone — all modes ring freely | Baseline |
| **Bolt** | Adds mass at contact point. Affected modes shift DOWN in frequency (up to 15%). Creates amplitude-modulated rattle via `ObeliskBoltRattle`. | Mass loading: f_new = f_old / sqrt(1 + dm/m) |
| **Rubber** | Damps modes at contact point (up to 95% Q reduction). Creates spectral holes. | Distributed contact narrows vibration, dampens antinode energy |
| **Glass** | Adds high-frequency resonances above fundamental series. Slightly INCREASES Q (glass is rigid). | Glass modes couple into string response |
| **Chain** | Broadband buzz via nonlinear contact. Chain bounces against strings when displacement exceeds threshold. | Jawari bridge / sitar principle |

**Preparation position** (0.05 - 0.95) determines WHICH modes are affected:
- Position sensitivity for mode N: `sin^2(N * pi * position)`
- Position 0.5 (center): kills even modes, preserves odd
- Position 0.33 (1/3 point): kills every 3rd mode
- This is real Cage preparation physics

**Preparation depth** (0.0 - 1.0) determines HOW STRONGLY modes are modified.

#### 3. Hammer Contact Model (Hunt-Crossley)

Stone is hard — alpha ~ 3.5-4.0 (vs 2.5 for piano felt). Short contact time, broadband excitation. Always includes impact noise (stone is percussive on contact).

Contact pulse: half-sine + noise mix (0.2-0.7 noise ratio based on hardness).

#### 4. HF Noise Shaper

Stone radiates noise at impact that sits above the modal frequencies. This bandpass-filtered noise (centered ~6kHz) adds the "crack" and "mineral dust" character during the attack phase.

#### 5. Thermal Drift

Stone has minimal thermal expansion but temperature affects internal damping. Cold marble = higher Q, longer sustain. The thermal drift parameter provides slow, per-voice micro-detuning with individual personality seeds.

---

## Parameters (30 total)

### Stone Body (6)
| ID | Name | Range | Default | Description |
|----|------|-------|---------|-------------|
| `obel_density` | Stone Density | 0-1 | 0.5 | Controls mode Q (denser = higher Q = longer ring) |
| `obel_stoneTone` | Stone Tone | 0-1 | 0.5 | Cold (0) emphasizes upper inharmonics; Warm (1) emphasizes fundamental |
| `obel_brightness` | Brightness | 200-20000 Hz | 6000 | LPF cutoff (skirt) |
| `obel_damping` | Damping | 0-1 | 0.2 | Scales decay time down |
| `obel_decay` | Decay | 0.1-15 s | 3.0 | Base decay time |
| `obel_hfNoise` | HF Stone Noise | 0-1 | 0.3 | Impact noise amount |

### Hammer (1)
| ID | Name | Range | Default | Description |
|----|------|-------|---------|-------------|
| `obel_hardness` | Hammer Hardness | 0-1 | 0.5 | D001: velocity + aftertouch add to this |

### Preparation System (3) — THE HERO
| ID | Name | Range | Default | Description |
|----|------|-------|---------|-------------|
| `obel_prepType` | Prep Type | 0-4 | 0 | None / Bolt / Rubber / Glass / Chain |
| `obel_prepPosition` | Prep Position | 0.05-0.95 | 0.5 | Where on the string (determines which modes affected) |
| `obel_prepDepth` | Prep Depth | 0-1 | 0.5 | How firmly inserted (intensity of modification) |

### Expression (3)
| ID | Name | Range | Default | Description |
|----|------|-------|---------|-------------|
| `obel_filterEnvAmount` | Filter Env Amount | 0-1 | 0.4 | Filter envelope depth |
| `obel_thermalDrift` | Thermal Drift | 0-1 | 0.2 | Slow micro-detuning |
| `obel_bendRange` | Bend Range | 1-24 st | 2 | Pitch bend range |

### Macros (4)
| ID | Label | Targets |
|----|-------|---------|
| `obel_macroStone` | STONE | Density + brightness |
| `obel_macroPrep` | PREPARATION | Prep depth |
| `obel_macroCoupling` | COUPLING | (reserved for coupling matrix) |
| `obel_macroSpace` | SPACE | (reserved for reverb/space) |

### LFOs (6)
| ID | Target | Default |
|----|--------|---------|
| `obel_lfo1Rate/Depth/Shape` | Brightness | 0.3 Hz, 0.0, Sine |
| `obel_lfo2Rate/Depth/Shape` | Prep depth | 0.8 Hz, 0.0, Sine |

---

## Expression Mapping (D006)

| Input | Target | Amount |
|-------|--------|--------|
| Velocity | Hammer hardness | +0.4 |
| Velocity | HF noise frequency | +4000 Hz |
| Aftertouch | Hammer hardness | +0.3 |
| Aftertouch | Brightness | +2000 Hz |
| Mod wheel | Stone tone (cold/warm) | +0.5 |
| Pitch bend | Pitch (via PitchBendUtil) | configurable |

---

## Presets (10)

| Name | Mood | Prep | Description |
|------|------|------|-------------|
| Cold Marble | Foundation | None | Pure unprepared stone. The baseline. |
| Bolt Rattle | Foundation | Bolt | Steel bolt at midpoint. Classic Cage preparation. |
| Rubber Mute | Foundation | Rubber | Rubber at 1/3 point. Spectral holes. |
| Glass Ring | Prism | Glass | Glass rod. Crystalline high-frequency overtones. |
| Chain Buzz | Flux | Chain | Chain buzz. Sitar-like nonlinearity. |
| Monolith Drone | Atmosphere | None | Maximum sustain. Geological timescales. |
| Sonatas Interludes | Prism | Bolt | Cage's masterwork configuration. |
| Living Stone | Flux | Rubber | LFO2 modulates prep depth. Breathing preparation. |
| Frozen Cathedral | Aether | Glass | Extreme decay + glass + thermal drift. Sacred. |
| Kitchen Counter | Flux | Chain | Maximum aggression. Chain + hard hammer. |

---

## CPU Budget

```
16 modes * 5ns * 8 voices = 640 ns/sample
1 SVF (LP filter) * 4ns * 8 voices = 32 ns/sample
1 SVF (HF noise) * 4ns * 8 voices = 32 ns/sample (only during attack)
1 SVF (chain buzz) * 4ns * active voices = ~16 ns/sample
Bolt rattle AM: ~2ns * affected modes = ~10 ns/sample
Total: ~730 ns/sample = ~3.2% CPU @ 44.1kHz, 512 buffer, 8 voices
```

Within the 5% per-engine budget, with headroom for coupling.

---

## References

- Cage, J. (1961). *Silence: Lectures and Writings.* Wesleyan UP.
- Bilbao, S. (2009). *Numerical Sound Synthesis.* Wiley.
- Fletcher, N.H. & Rossing, T.D. (1998). *The Physics of Musical Instruments.* 2nd ed. Springer.
- Chaigne, A. & Kergomard, J. (2016). *Acoustics of Musical Instruments.* Springer.
- Woodhouse, J. (2004). "Plucked guitar transients." *Acustica* 90.
