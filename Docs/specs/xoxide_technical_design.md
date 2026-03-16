# XOxide — Technical Design Specification

**Date**: 2026-03-16
**Category**: Utility / Character Processor
**Param prefix**: `oxide_`
**Accent**: Rust Iron `#B7410E`
**Creature**: The pistol shrimp — generates a plasma cavitation bubble hotter than the surface of the sun; the most violent character transformation in the ocean, achieved through a single precise snap.

---

## 1. Identity

XOxide is a continuous 2D character shaper. It is XOmnibus's first utility engine that processes
audio rather than synthesising it. Drop it into any slot alongside a synthesis engine; couple the
synthesis engine's output as `AudioToBuffer` and XOxide becomes that engine's character stage.

**2D Plane:**
- **X axis** — feliX ↔ Oscar: bright/transient-forward/precise (left) ↔ warm/sustain-forward/rounded (right)
- **Y axis** — Surface ↔ Depth: subtle/transparent (top) ↔ heavy/transformative (bottom)

**Processing chain** (always in this order, each stage individually bypassable):

```
Input Gain → [1] Transient Shaper → [2] Saturation → [3] Compression
           → [4] Sub Enhancement → [5] Stereo Field → [6] Exciter/Air
           → Output Gain → Dry/Wet
```

All six stages are driven simultaneously from the same XY position. The XY pad IS the instrument;
individual stage controls are power-user overrides, not the primary interaction.

---

## 2. Parameter List

### 2.1 Core / Navigation

| ID | Type | Range | Default | Description |
|----|------|-------|---------|-------------|
| `oxide_x` | Float | -1.0 → +1.0 | 0.0 | feliX (-1) ↔ Oscar (+1) |
| `oxide_y` | Float | 0.0 → 1.0 | 0.0 | Surface (0) ↔ Depth (1) |
| `oxide_inputGain` | Float | -24 → +24 dB | 0.0 | Pre-chain input level |
| `oxide_outputGain` | Float | -24 → +24 dB | 0.0 | Post-chain output level |
| `oxide_dryWet` | Float | 0.0 → 1.0 | 1.0 | Dry/wet blend (parallel path) |

### 2.2 Global Modulation (XY LFO)

| ID | Type | Range | Default | Description |
|----|------|-------|---------|-------------|
| `oxide_lfoRate` | Float | 0.01 → 20 Hz | 0.25 | LFO rate for XY position modulation |
| `oxide_lfoDepth` | Float | 0.0 → 1.0 | 0.0 | LFO depth (0 = off, 1 = full plane sweep) |
| `oxide_lfoShape` | Choice | Sine/Triangle/S&H/Chaos | Sine | LFO waveform shape |
| `oxide_lfoXAmount` | Float | -1.0 → +1.0 | 0.5 | How much LFO displaces X (bipolar) |
| `oxide_lfoYAmount` | Float | -1.0 → +1.0 | 0.5 | How much LFO displaces Y (bipolar) |

### 2.3 Stage Bypasses

| ID | Type | Default | Description |
|----|------|---------|-------------|
| `oxide_bypassTransient` | Bool | false | Bypass transient shaper stage |
| `oxide_bypassSaturator` | Bool | false | Bypass saturation stage |
| `oxide_bypassCompressor` | Bool | false | Bypass compression stage |
| `oxide_bypassSub` | Bool | false | Bypass sub enhancement stage |
| `oxide_bypassStereo` | Bool | false | Bypass stereo field stage |
| `oxide_bypassExciter` | Bool | false | Bypass exciter/air stage |

### 2.4 Stage Amounts (XY-driven but overridable)

Each `_amount` param defaults to -1.0 meaning "XY-driven." Any value ≥ 0 overrides the
XY mapping for that stage, enabling per-stage manual control.

| ID | Type | Range | Default | Description |
|----|------|-------|---------|-------------|
| `oxide_transientAmount` | Float | -1.0 (auto) → 0.0 → 1.0 | -1.0 | Stage mix override |
| `oxide_saturatorAmount` | Float | -1.0 (auto) → 0.0 → 1.0 | -1.0 | Stage mix override |
| `oxide_compressorAmount` | Float | -1.0 (auto) → 0.0 → 1.0 | -1.0 | Stage mix override |
| `oxide_subAmount` | Float | -1.0 (auto) → 0.0 → 1.0 | -1.0 | Stage mix override |
| `oxide_stereoAmount` | Float | -1.0 (auto) → 0.0 → 1.0 | -1.0 | Stage mix override |
| `oxide_exciterAmount` | Float | -1.0 (auto) → 0.0 → 1.0 | -1.0 | Stage mix override |

### 2.5 Stage Advanced Controls

#### Transient Shaper
| ID | Type | Range | Default | Description |
|----|------|-------|---------|-------------|
| `oxide_transAttack` | Float | 0.1 → 50 ms | XY-driven | Attack emphasis time constant |
| `oxide_transRelease` | Float | 10 → 500 ms | XY-driven | Sustain envelope release time |
| `oxide_transGain` | Float | -12 → +12 dB | 0.0 | Attack gain staging |

#### Saturation
| ID | Type | Range | Default | Description |
|----|------|-------|---------|-------------|
| `oxide_satDrive` | Float | 0.0 → 1.0 | XY-driven | Input drive into waveshaper |
| `oxide_satBlend` | Float | -1.0 → +1.0 | XY-driven | Tube (Oscar, -1) ↔ Digital (feliX, +1) |
| `oxide_satCompensate` | Bool | true | Output level compensation |

#### Compression
| ID | Type | Range | Default | Description |
|----|------|-------|---------|-------------|
| `oxide_compThreshold` | Float | -60 → 0 dBFS | XY-driven | Compression threshold |
| `oxide_compRatio` | Float | 1:1 → 20:1 | XY-driven | Compression ratio |
| `oxide_compAttack` | Float | 0.1 → 100 ms | XY-driven | Compressor attack |
| `oxide_compRelease` | Float | 10 → 2000 ms | XY-driven | Compressor release |
| `oxide_compKnee` | Float | 0.0 → 12 dB | XY-driven | Knee softness |
| `oxide_compMakeup` | Bool | true | Auto makeup gain |

#### Sub Enhancement
| ID | Type | Range | Default | Description |
|----|------|-------|---------|-------------|
| `oxide_subOctaveMix` | Float | 0.0 → 1.0 | XY-driven | Octave-down sine level |
| `oxide_subTracking` | Float | 1 → 500 ms | XY-driven | Pitch tracking speed |
| `oxide_subLowpass` | Float | 40 → 200 Hz | 80 Hz | LP rolloff for sub sine |

#### Stereo Field
| ID | Type | Range | Default | Description |
|----|------|-------|---------|-------------|
| `oxide_stereoWidth` | Float | 0.0 → 2.0 | XY-driven | M/S width (1.0 = unity) |
| `oxide_stereoHaas` | Float | 0.0 → 25 ms | XY-driven | Haas micro-delay time |
| `oxide_stereoMono` | Bool | false | Hard mono collapse (monitoring) |

#### Exciter / Air
| ID | Type | Range | Default | Description |
|----|------|-------|---------|-------------|
| `oxide_exciterFreq` | Float | 3000 → 16000 Hz | XY-driven | Exciter crossover frequency |
| `oxide_exciterOrder` | Choice | 2nd/3rd/4th | XY-driven | Harmonic order (even=Oscar, odd=feliX) |
| `oxide_exciterMix` | Float | 0.0 → 1.0 | XY-driven | Exciter wet level |

### 2.6 Macros

| ID | Label | Maps To |
|----|-------|---------|
| `oxide_macroCharacter` | CHARACTER (M1) | `oxide_x` directly |
| `oxide_macroCoupling` | COUPLING (M2) | `oxide_y` directly |
| `oxide_macroMovement` | MOVEMENT (M3) | `oxide_lfoDepth` + `oxide_lfoRate` (combined sweep) |
| `oxide_macroSpace` | SPACE (M4) | `oxide_dryWet` + `oxide_stereoWidth` blend |

### 2.7 Total Parameter Count

| Group | Count |
|-------|-------|
| Core / Navigation | 5 |
| LFO | 5 |
| Stage bypasses | 6 |
| Stage amounts (overrides) | 6 |
| Transient advanced | 3 |
| Saturation advanced | 3 |
| Compression advanced | 6 |
| Sub advanced | 3 |
| Stereo advanced | 3 |
| Exciter advanced | 3 |
| Macros | 4 |
| **Total** | **47** |

---

## 3. DSP Architecture

### 3.1 Reuse of Shared DSP Modules

XOxide reuses existing modules from `Source/DSP/Effects/` where possible. New code
is written only where the 2D character-shaping behaviour requires it.

| Stage | Shared Module | New Work Required |
|-------|--------------|-------------------|
| Transient Shaper | `TransientDesigner.h` | XY mapping layer only |
| Saturation | `Saturator.h` + `Corroder.h` | 3-way tube/tape/digital blend bus |
| Compression | `Compressor.h` | Program-dependent mode switch |
| Sub Enhancement | None | Pitch tracker + octave-down oscillator (new) |
| Stereo Field | `StereoSculptor.h` + `PsychoacousticWidth.h` | XY routing only |
| Exciter / Air | `HarmonicExciter.h` | Harmonic order selector |

### 3.2 Stage 1 — Transient Shaper

**Principle**: Dual-envelope splitting: fast follower (attack) vs. slow follower (sustain). The
ratio of fast/slow envelopes reveals whether a moment is an attack or a sustain tail. Gain-riding
based on this ratio allows either to be boosted or cut independently.

**Implementation**:

```
fast_env[n]  = α_fast  * |x[n]| + (1 - α_fast)  * fast_env[n-1]   // ~1-5 ms
slow_env[n]  = α_slow  * |x[n]| + (1 - α_slow)  * slow_env[n-1]   // ~30-200 ms
attack_sig   = max(0, fast_env - slow_env)   // present during attacks
sustain_sig  = max(0, slow_env - fast_env)   // present during tails

attack_gain  = 1 + (transAttackAmount  * attack_sig  * transGain)
sustain_gain = 1 + (transSustainAmount * sustain_sig * transGain)
y[n] = x[n] * attack_gain * sustain_gain
```

**XY mapping**:
- **X (feliX side)**: `transAttackAmount` → +1.0 (attack boosted), `transSustainAmount` → -0.5
  (sustain slightly cut = percussive, punchy)
- **X (Oscar side)**: `transAttackAmount` → -0.3 (attack softened), `transSustainAmount` → +1.0
  (sustain boosted = smooth, legato)
- **Y (Depth)**: Scales the magnitude of both amounts linearly from 0 (no effect at surface) to
  1.0 (full application at maximum depth)

**Alpha constants**: Derived from `exp(-1 / (sampleRate * timeMs / 1000))` — matched-Z, never
hardcoded sample counts.

### 3.3 Stage 2 — Saturation

Three saturation characters are continuously blended based on X position.

**Tube model (Oscar, X = +1.0)**:
- Even-harmonic emphasis using an asymmetrically biased `tanh` waveshaper
- Bias term shifts the operating point: `y = tanh(drive * x + bias) - tanh(bias)`
- `bias` ≈ 0.2–0.4 introduces second harmonic dominance
- Soft knee, gentle onset drive

**Tape model (center, X = 0.0)**:
- Simplified hysteresis: Jiles-Atherton reduced to a single-pole IIR with nonlinear gain
- `tape_state += k * (x - tape_state)` where `k = f(input_level)` — faster for high inputs
- Simultaneously contributes odd + even harmonics with gentle frequency-dependent saturation
  (HF loss at high drive, pre-emphasis/de-emphasis structure)

**Digital model (feliX, X = -1.0)**:
- Hard clip: `y = clamp(drive * x, -1, 1)` — odd harmonics dominant
- Optional fold-back: when `|drive * x| > 1`, reflect back: creates aggressive aliased texture
- Optional bit reduction: quantize to N bits (8–16 range) for lo-fi crunch

**Blend bus**:
```
blend_t = (oxide_x + 1.0) / 2.0          // 0 = feliX/digital, 1 = Oscar/tube
tube_out    = tube_process(x)
tape_out    = tape_process(x)
digital_out = digital_process(x)
y = lerp(lerp(digital_out, tape_out, blend_t * 2),
         lerp(tape_out, tube_out, (blend_t - 0.5) * 2),
         step(0.5, blend_t))
```

Output level compensation (when `oxide_satCompensate = true`): measure RMS before/after, apply
inverse gain correction over a 100 ms smoothed window.

### 3.4 Stage 3 — Compression

Two compressor characters blended by X position.

**feliX mode — FET style (X = -1.0)**:
- Fast attack (0.1–1 ms), fast release (50–200 ms)
- Harder knee (0–2 dB)
- Ratio can reach 20:1 (limiting territory)
- Models 1176-style "all buttons in" hyper-compression when Y = 1.0 (Depth)

**Oscar mode — Opto style (X = +1.0)**:
- Program-dependent attack: fast for loud transients, slow for quiet passages
  (approximated as: `attack_ms = base_attack / (1 + peak_env)`)
- Slow release (200–2000 ms), auto-makeup gain
- Softer knee (6–12 dB range)
- Models LA-2A optical levelling character

**XY mapping**:
- X position continuously interpolates attack/release/knee between FET and Opto curves
- Y position (Depth) maps to effective ratio: `ratio = 1 + (Y * (target_ratio - 1))`
  At surface (Y=0): ratio ≈ 1.2:1 (nearly transparent), at depth (Y=1): full ratio applies
- Threshold is auto-derived from input RMS when in XY mode:
  `threshold = measured_rms - headroom_factor` where `headroom_factor = lerp(12, 3, Y)`

**Implementation note**: Use the gain computer / ballistics architecture from the existing
`Compressor.h` module. The XY layer controls which parameter curves are active.

### 3.5 Stage 4 — Sub Enhancement

Only meaningfully active in the Oscar half of the plane (X > 0) and at depth (Y > 0.3).
At feliX values (X < -0.3) the stage is effectively muted by the XY mapping.

**Pitch tracking**: Zero-crossing detector feeding an IIR-smoothed period estimator.
```
period_est[n] = α_track * measured_period + (1 - α_track) * period_est[n-1]
```
`α_track` is derived from `oxide_subTracking` (1–500 ms). Fast tracking follows rapidly
changing pitches; slow tracking is appropriate for pads and drones.

**Octave-down oscillator**: A phase accumulator running at `f0 / 2` (one octave below detected
fundamental), outputting a pure sine. Mixed in at `oxide_subOctaveMix` level after a
`oxide_subLowpass` LP filter to suppress harmonics.

**Amplitude follower**: The sub oscillator's level is gated/shaped by the input's sub-band RMS
(20–80 Hz range from a dedicated bandpass). This ensures the octave-down oscillator only
speaks when real sub content is present, preventing mud from sub-poor sources.

**XY mapping**:
- X: Sub amount scales from 0 at X = -1.0 to full amount at X = +1.0 (Oscar exclusive)
- Y: `oxide_subOctaveMix` max value scales with Y: `max_sub_level = Y * 0.35`
  (caps at 35% to prevent muddy overwhelm at maximum depth)

### 3.6 Stage 5 — Stereo Field

Mid/side processing with two character extremes.

**feliX mode (X = -1.0) — Wide and airy**:
- Side channel level boosted: `S_out = S_in * widthFactor` where `widthFactor` up to 2.0
- Haas micro-delay on one channel (1–25 ms, no feedback) adds perceived width without
  comb filtering in mono (keep < 25 ms to stay above Haas fusion zone threshold)
- Implementation: `R_delayed = delay_line(R, haasTime)` on right channel only

**Oscar mode (X = +1.0) — Focused and warm**:
- Mid channel emphasized, side reduced: stereo image tightened toward center
- `width < 1.0` (0.6–0.9 range) compresses the stereo field
- Subtle correlation increase creates mono-compatible warmth

**Mono safety**:
- `oxide_stereoMono` hard-collapses to `(L+R)/2` for checking
- All Haas delay times kept ≤ 25 ms (JASA 1951 Haas effect boundary)
- M/S encoding: `M = (L+R)/√2`, `S = (L-R)/√2`; apply width; decode back:
  `L = (M+S)/√2`, `R = (M-S)/√2`

**XY mapping**:
- X maps `oxide_stereoWidth`: feliX → 1.8, Oscar → 0.7, center → 1.0
- X maps `oxide_stereoHaas`: feliX → up to 15 ms, Oscar → 0 ms
- Y scales the magnitude of the deviation from unity (center = no effect regardless of X at Y=0)

### 3.7 Stage 6 — Exciter / Air

Harmonic generation above the `oxide_exciterFreq` crossover. Audio above the crossover is
split off via a high-shelf HP path, saturated to generate harmonics, then mixed back.

**feliX mode (X = -1.0) — Crystalline odd harmonics**:
- `oxide_exciterOrder = 3rd` (cubic distortion): `y = x - (x³/3)` — adds third harmonic
- No LP rolloff on generated harmonics — full spectral content
- Produces shimmer, presence, and definition at high frequencies

**Oscar mode (X = +1.0) — Warm even harmonics**:
- `oxide_exciterOrder = 2nd` (half-wave rectification approach): `y = (x + |x|) / 2`
  followed by normalisation — generates second harmonic dominance
- Post-harmonics LP rolloff (e.g., 12 kHz 6 dB/oct shelf) softens the air into warmth

**Implementation**:
```
hp_signal  = highpass_filter(x, oxide_exciterFreq)   // 2nd-order Butterworth
dry_hp     = hp_signal
saturated  = harmonic_stage(hp_signal, order)         // order selected by XY
harmonic_add = saturated - dry_hp                     // isolate added harmonics only
y = x + oxide_exciterMix * harmonic_add
```

Isolating the harmonic difference prevents the exciter from changing the fundamental level.

**XY mapping**:
- X continuously interpolates harmonic order: feliX = pure odd (3rd), center = 2nd+3rd blend,
  Oscar = pure even (2nd)
- Y maps `oxide_exciterMix` from 0 at surface to 0.6 max at full depth
- Y also lowers `oxide_exciterFreq` as depth increases (deeper transformation extends
  further into the midrange): `freq = lerp(12000, 4000, Y)`

---

## 4. XY → Parameter Mapping Table

The table below defines the complete mapping from normalized (x ∈ [-1,+1], y ∈ [0,1]) to
stage parameters. All interpolations are linear unless noted.

### Legend
- `x_n` = (oxide_x + 1) / 2  → 0 = full feliX, 1 = full Oscar
- `y`   = oxide_y             → 0 = Surface, 1 = Depth
- `α`   = x_n (Oscar weight)
- `β`   = 1 - x_n (feliX weight)

### Transient Shaper

| Parameter | feliX (x_n=0) | Center (x_n=0.5) | Oscar (x_n=1) | Y scaling |
|-----------|--------------|-----------------|--------------|-----------|
| Attack amount | +1.0 (boost) | +0.3 | -0.3 (cut) | × y |
| Sustain amount | -0.5 (cut) | 0.0 | +1.0 (boost) | × y |
| Attack time | 1 ms | 3 ms | 8 ms | — |
| Release time | 50 ms | 100 ms | 200 ms | — |

### Saturation

| Parameter | feliX | Center | Oscar | Y scaling |
|-----------|-------|--------|-------|-----------|
| Drive (effective) | 0.6 | 0.3 | 0.7 | × (0.3 + y × 0.7) |
| Blend (type) | -1.0 (digital) | 0.0 (tape) | +1.0 (tube) | — |
| Max THD | ~3% | ~1.5% | ~4% | × y |

Note: At Y=0 (Surface), saturation drive is low enough that THD is perceptually negligible
regardless of X position. Depth is required for significant character change.

### Compression

| Parameter | feliX | Center | Oscar | Y scaling |
|-----------|-------|--------|-------|-----------|
| Attack mode | FET 0.3 ms | 2 ms | Opto 20 ms | — |
| Release mode | Fast 80 ms | 250 ms | Program-dep 800 ms | — |
| Knee | 0.5 dB | 3 dB | 9 dB | — |
| Ratio | 4:1 | 2.5:1 | 1.8:1 | Scale to × y² (quadratic depth) |
| Threshold offset | -6 dB rel RMS | -3 dB | -1.5 dB | — |

Ratio uses quadratic Y scaling so the surface is nearly transparent and depth is required to
engage real compression. At Y=0 ratio is 1.1:1 for all X positions.

### Sub Enhancement

| Parameter | feliX | Center | Oscar | Y scaling |
|-----------|-------|--------|-------|-----------|
| Sub gate | 0.0 (silent) | 0.0 | 1.0 | × max(0, y - 0.3) / 0.7 |
| Octave mix | 0.0 | 0.0 | y × 0.35 | — |
| Tracking speed | — | — | lerp(20ms, 200ms, y) | — |

Sub is gated below Y=0.3 at all X positions. Between Y=0.3 and Y=1.0 it ramps linearly
from 0 to full active.

### Stereo Field

| Parameter | feliX | Center | Oscar | Y scaling |
|-----------|-------|--------|-------|-----------|
| Width | 1.0 + y × 0.8 | 1.0 | 1.0 - y × 0.3 | (magnitude built in) |
| Haas delay | y × 15 ms | 0 | 0 | — |
| Side boost (dB) | y × 3 dB | 0 | 0 | — |
| Mid boost (dB) | 0 | 0 | y × 2 dB | — |

At center (X=0) the stereo field stage has no effect at any depth.

### Exciter / Air

| Parameter | feliX | Center | Oscar | Y scaling |
|-----------|-------|--------|-------|-----------|
| Frequency | lerp(12k, 8k, y) | lerp(10k, 6k, y) | lerp(8k, 4k, y) | (built in) |
| Harmonic order | 3rd (odd) | 2nd+3rd blend | 2nd (even) | — |
| Mix | 0.0 + y × 0.5 | 0.0 + y × 0.35 | 0.0 + y × 0.4 | (built in) |
| Post LP rolloff | none | 14 kHz | 10 kHz | lowered by y |

### Summary: What Each Quadrant Does

| Quadrant | feliX/Surface | feliX/Depth | Oscar/Surface | Oscar/Depth |
|----------|--------------|-------------|--------------|-------------|
| Transient | Slight attack lift | Strong attack emphasis, sustain cut | Slight sustain lift | Strong sustain emphasis, attack softened |
| Saturation | Trace digital grit | Aggressive hard clip fold-back | Trace tube warmth | Deep tube saturation with even harmonics |
| Compression | Nearly transparent FET | Hard limiting, fast pumping | Nearly transparent opto | Deep LA-2A levelling |
| Sub | Silent | Silent | Silent | Octave-down sine tracking |
| Stereo | Unity | Wide + Haas widening | Unity | Slightly narrowed/focused |
| Exciter | Trace odd shimmer | Full crystalline odd harmonics from 4 kHz up | Trace warm air | Full warm even harmonics from 4 kHz up |

---

## 5. LFO System

The LFO modulates the XY position, creating automatic character evolution.

**Shapes**:
- **Sine**: Smooth circular path through the 2D plane (ellipse when X/Y amounts differ)
- **Triangle**: Ping-pong path between two character extremes
- **S&H**: Stochastic jumps — samples new target position at each clock, glides via a
  `oxide_lfoShape`-controlled glide time (conceptually similar to track-and-hold)
- **Chaos**: Lorenz attractor projected onto XY space, producing organic unpredictable wandering
  while staying bounded. Use a simplified discrete-time Lorenz (σ=10, ρ=28, β=8/3) downsampled
  to audio control rate (every 64 samples).

**LFO rate** follows D005 doctrine: minimum 0.01 Hz (100-second sweep), maximum 20 Hz
(tremolo territory).

**LFO target**: The LFO displaces the current XY position additively, clamped to the plane
boundaries:
```
x_driven = clamp(oxide_x + lfo_out * oxide_lfoXAmount * oxide_lfoDepth, -1, +1)
y_driven = clamp(oxide_y + lfo_out * oxide_lfoYAmount * oxide_lfoDepth,  0,  1)
```

---

## 6. XOmnibus Integration

### 6.1 Engine Registration

XOxide registers as `"Oxide"` in `EngineRegistry.h` with `REGISTER_ENGINE(OxideEngine)`.
Parameter prefix: `oxide_`. Engine ID string: `"Oxide"`.

### 6.2 Utility Engine Classification

XOxide is an **FX processor** — it generates no audio of its own. Two design constraints follow:

1. `renderBlock()` must have an audio input path. XOmnibus's architecture passes the mixed
   output of other active slots as the `AudioToBuffer` coupling signal into XOxide's input
   ring buffer, or the user manually routes a specific slot.
2. `getMaxVoices()` returns 0. XOxide has no voice engine and ignores MIDI note events.
   It remains responsive to MIDI CC (mod wheel → `oxide_y`, expression → `oxide_dryWet`).

**Distinguishing from synthesis engines in the UI**: XOxide's Gallery card displays an "FX"
badge in the engine accent color. The PlaySurface hides the pad grid and shows the 2D
XY pad as the primary control surface instead.

### 6.3 Coupling Accepted

| Coupling Type | Role |
|--------------|------|
| `AudioToBuffer` | Primary: receives continuous stereo audio from another slot's output |
| `AmpToFilter` | Sidechain input: external amplitude modulates `oxide_y` (auto-duck behavior) |
| `LFOToPitch` | Clock sync: external LFO locks `oxide_lfoRate` to host BPM via this type |

### 6.4 Coupling Emitted

| Coupling Type | What XOxide Sends |
|--------------|-------------------|
| `AudioToBuffer` | Processed (oxidized) audio downstream, e.g. into OPAL grain buffer |
| `AmpToFilter` | Post-compression gain reduction value as modulation signal |
| `EnvToMorph` | Detected input transient envelope as modulation (can drive ODYSSEY morph, etc.) |

### 6.5 Normalled Defaults

When a synthesis engine is in Slot 1 and XOxide is in Slot 2, the normalled default route is:
- Slot 1 `AudioToBuffer` → Slot 2 (XOxide) at amount 1.0

This means placing XOxide adjacent to any synthesis engine "just works" with no manual routing.

### 6.6 Macros

| Macro | Label | Primary Mapping | Secondary Mapping |
|-------|-------|----------------|-------------------|
| M1 | CHARACTER | `oxide_x` full range | `oxide_satBlend` (echoes X automatically) |
| M2 | COUPLING | `oxide_y` full range | `oxide_compRatio` (echoes Y at Depth) |
| M3 | MOVEMENT | `oxide_lfoDepth` 0→1 + `oxide_lfoRate` 0.1→8 Hz simultaneously | LFO shape morphs Sine→Chaos as M3 increases |
| M4 | SPACE | `oxide_dryWet` 0→1 | `oxide_stereoWidth` 1.0→1.4 simultaneously |

### 6.7 prefixForEngine() Addition

In `XOmnibusEditor.h`, add case to `prefixForEngine()`:
```cpp
if (engineId == "Oxide") return "oxide_";
```

---

## 7. Factory Presets — 20 Concepts

All presets use `.xometa` format. DNA and mood assignments are noted below.

### Foundation Mood (near plane center, subtle)

**1. "Quiet Refinement"**
- X=0.0, Y=0.15 — pure center, almost no effect
- Transient: slight attack lift only | Sat: trace tape warmth | Comp: 1.2:1
- DNA: brightness=0.5, warmth=0.5, movement=0.1, density=0.2, space=0.5, aggression=0.1
- Use: mastering-style gentle enhancement, transparent polish

**2. "Steel Silk"**
- X=-0.2, Y=0.25 — slight feliX lean at low depth
- Transient: small attack boost | Exciter: subtle 3rd harmonic air above 10 kHz
- DNA: brightness=0.6, warmth=0.4, movement=0.1, density=0.3, space=0.5, aggression=0.2
- Use: adding definition to pads without brightness

**3. "Warm Glue"**
- X=+0.3, Y=0.3 — mild Oscar lean
- Comp: opto-style 1.5:1 ratio, slow attack | Sat: trace tube even harmonics
- DNA: brightness=0.4, warmth=0.6, movement=0.1, density=0.4, space=0.5, aggression=0.1
- Use: gluing a mix, slight vintage character

**4. "Still Water"**
- X=+0.5, Y=0.1 — moderate Oscar, minimal depth
- Stereo: slight width reduction to 0.85 (focused, mono-compatible)
- DNA: brightness=0.3, warmth=0.7, movement=0.0, density=0.3, space=0.4, aggression=0.0
- Use: mono-compatibility check, centering pads

### Atmosphere Mood (Oscar/Surface quadrant, gentle warmth)

**5. "Amber Morning"**
- X=+0.6, Y=0.4
- Sat: tube drive 0.3 | Comp: opto 1.8:1 slow | Exciter: warm even at 7 kHz
- DNA: brightness=0.4, warmth=0.8, movement=0.2, density=0.5, space=0.6, aggression=0.1
- Use: vintage warmth on synth pads, tape-machine character

**6. "Deep Swell"**
- X=+0.7, Y=0.6
- All stages active at moderate Oscar depth | Sub: octave mix 0.15 for bass voices
- DNA: brightness=0.3, warmth=0.8, movement=0.3, density=0.6, space=0.7, aggression=0.2
- Use: bass enhancement, pad thickening

**7. "Velvet Compress"**
- X=+0.5, Y=0.7
- Comp: opto 3:1, slow 30 ms attack, 1.2s release | Sat: tube | Transient: sustain+
- DNA: brightness=0.3, warmth=0.7, movement=0.1, density=0.7, space=0.6, aggression=0.1
- Use: levelling vocals/leads with character, not just dynamics

**8. "Analog Ghost"**
- X=+0.3, Y=0.5, LFO: Sine 0.03 Hz, depth 0.15, X-only
- Slowly breathes between center and mild Oscar — barely perceptible oscillation
- DNA: brightness=0.4, warmth=0.6, movement=0.3, density=0.5, space=0.6, aggression=0.1
- Use: subtle analog drift feeling, long-form pad evolution

### Prism Mood (dramatic, LFO active, moving between quadrants)

**9. "Pulse Prism"**
- X=0.0, Y=0.5, LFO: Triangle 0.5 Hz, depth 0.8, X-amount=1.0 Y-amount=0.0
- Sweeps feliX↔Oscar at 0.5 Hz — transient/sustain pumping alternation
- DNA: brightness=0.5, warmth=0.5, movement=0.8, density=0.6, space=0.6, aggression=0.5
- Use: rhythmic character morphing, trance-style oscillation

**10. "Depth Dive"**
- X=+0.4, Y=0.4, LFO: Sine 0.2 Hz, depth 0.7, Y-amount=1.0 X-amount=0.0
- Breathes in and out of depth — sub and compression rise and fall
- DNA: brightness=0.4, warmth=0.6, movement=0.7, density=0.6, space=0.6, aggression=0.3
- Use: pad automation, evolving bass presence

**11. "Crystal Storm"**
- X=-0.6, Y=0.6, LFO: Sine 0.15 Hz, depth 0.5, X-amount=0.3 Y-amount=0.6
- Strong feliX lean — exciter crystalline + FET compression + wide stereo, breathing in depth
- DNA: brightness=0.8, warmth=0.2, movement=0.6, density=0.5, space=0.8, aggression=0.6
- Use: metallic pads, cymbal-like sheen on sustained textures

**12. "Nautilus Path"**
- X=0.0, Y=0.0, LFO: Chaos 0.08 Hz, depth 0.9, X-amount=0.7 Y-amount=0.5
- Lorenz attractor traces an organic path through the entire plane
- DNA: brightness=0.5, warmth=0.5, movement=0.9, density=0.5, space=0.6, aggression=0.4
- Use: generative, unpredictable — never sounds the same twice

### Flux Mood (extreme, heavy processing, corners of the plane)

**13. "Iron Fist"**
- X=-1.0, Y=1.0 — full feliX/Depth
- Transient: maximum attack boost, sustain cut | Sat: hard digital fold-back
- Comp: FET limiting 20:1, 0.1 ms attack | Exciter: harsh odd harmonics from 3 kHz
- DNA: brightness=0.9, warmth=0.1, movement=0.0, density=0.9, space=0.7, aggression=0.9
- Use: drums, industrial percussion, aggressive stabs

**14. "Magma Core"**
- X=+1.0, Y=1.0 — full Oscar/Depth
- Transient: sustain fully boosted, attack suppressed (gates punches, swells tails)
- Sat: heavy tube drive 0.8 | Comp: deep opto levelling | Sub: octave-down at 0.3 mix
- DNA: brightness=0.1, warmth=0.9, movement=0.0, density=0.9, space=0.7, aggression=0.4
- Use: bass synthesis fattening, sustained pad transformation

**15. "Oxidize Heavy"**
- X=+0.2, Y=1.0 — center/Depth
- All six stages at near-maximum settings, tape saturation dominant
- DNA: brightness=0.5, warmth=0.6, movement=0.0, density=0.9, space=0.6, aggression=0.6
- Use: maximum character transformation, lo-fi effect

**16. "Cavitation"**
- X=-0.9, Y=0.85, LFO: Triangle 2.0 Hz, depth 0.6, X-amount=0.2 Y-amount=0.8
- Fast LFO pumps depth — stutter-like aggressive-to-subtle cycling at 2 Hz
- DNA: brightness=0.8, warmth=0.2, movement=0.9, density=0.8, space=0.7, aggression=0.8
- Use: aggressive sidechain pumping effect, EDM-style dynamic gating

**17. "Event Horizon"**
- X=-1.0, Y=0.9, LFO: Chaos 0.05 Hz, depth 0.4, X-amount=0.0 Y-amount=1.0
- Locked at maximum feliX; chaos LFO varies depth only
- DNA: brightness=0.9, warmth=0.1, movement=0.7, density=0.8, space=0.8, aggression=0.9
- Use: evolving crystalline destruction, experimental texture

**18. "Black Velvet"**
- X=+0.9, Y=0.8, LFO: Sine 0.03 Hz, depth 0.3, X-amount=0.3 Y-amount=0.0
- Deep Oscar with slow barely-perceptible breathing on X axis
- DNA: brightness=0.1, warmth=0.9, movement=0.4, density=0.8, space=0.5, aggression=0.2
- Use: deep, dark, warm transformation — lo-fi vinyl character

**19. "Rust and Chrome"**
- X=0.0, Y=0.75, LFO: S&H 0.12 Hz, depth 0.7, X-amount=1.0 Y-amount=0.3
- S&H jumps to random X positions every ~8 seconds; each jump is a distinct character snap
- DNA: brightness=0.5, warmth=0.5, movement=0.8, density=0.7, space=0.6, aggression=0.5
- Use: random character transformation — each jump re-characterizes the source differently

**20. "Pistol Snap"**
- X=-0.8, Y=0.9, LFO: off
- Maximum transient emphasis + digital clip fold-back + FET brick-wall + crystalline exciter
- No sustain, no warmth, no sub — pure impact
- DNA: brightness=0.8, warmth=0.1, movement=0.0, density=0.9, space=0.6, aggression=1.0
- Use: drums, claps, snaps — the pistol shrimp snap, maximum transient violence

---

## 8. UI Wireframe

### Layout Overview (800 × 600 reference frame)

```
┌─────────────────────────────────────────────────────────┐
│  XOXIDE                            [FX]  ████████ RUST  │
│  Character Shaper                                       │
├─────────────────────────────────────────────────────────┤
│  INPUT ▐█████████████░░░░░░░░░░░░░░░░░░░░▌ OUTPUT      │
│        -18dBFS                              +3 GR        │
├──────────────────────────┬──────────────────────────────┤
│                          │                              │
│   feliX ←──────────→ Oscar   SURFACE                   │
│                          │                              │
│   ┌──────────────────┐   │    ↑                        │
│   │                  │   │    │                        │
│   │     XY PAD       │   │    │   (Y axis label)       │
│   │                  │   │    │                        │
│   │    ●             │   │    ↓                        │
│   │   (cursor dot)   │   │   DEPTH                     │
│   │                  │   │                              │
│   └──────────────────┘   │                              │
│                          │  Plane bg: gradient          │
│  LFO: [●SINE] RATE [──]  │  feliX=blue/crisp           │
│        DEPTH [──]        │  Oscar=amber/warm            │
│        X×[─] Y×[─]       │  Surface=light, Depth=dark  │
├──────────────────────────┴──────────────────────────────┤
│  CHAIN   [TRANS] [SAT]  [COMP] [SUB]  [STERO] [EXCITE] │
│  ░░░░░░░░  ░░░░   ░░░░   ░░░░   ░░░   ░░░░░    ░░░░░░  │
│  (each stage: mini meter + bypass button + amt knob)    │
├─────────────────────────────────────────────────────────┤
│  M1 CHARACTER  M2 COUPLING  M3 MOVEMENT  M4 SPACE       │
│     [────●──]    [──●────]    [───●───]    [──●────]    │
├─────────────────────────────────────────────────────────┤
│  IN [────●] dB    DRY/WET [─────●] 100%    OUT [────●]  │
└─────────────────────────────────────────────────────────┘
```

### XY Pad Design

- **Size**: ~360 × 280 px — the dominant UI element
- **Background gradient**: Four-corner color map:
  - Top-left (feliX/Surface): pale electric blue `#E0F4FF`
  - Top-right (Oscar/Surface): pale amber `#FFF3E0`
  - Bottom-left (feliX/Depth): saturated steel blue `#1A3A5C`
  - Bottom-right (Oscar/Depth): saturated rust orange `#5C2A0A`
  - Bilinear interpolation between corners as X/Y changes
- **Cursor dot**: 12 px circle, XO Gold `#E9C46A` fill, white ring, drop shadow
- **LFO path trace**: when LFO is active, a ghost trail shows the recent position history
  (last 2 seconds, fading transparency, in the accent rust color)
- **Axis labels**: "feliX" left edge, "Oscar" right edge, "SURFACE" top, "DEPTH" bottom —
  Space Grotesk, small caps, color-coded to the gradient

### Metering Strip (top bar)

- Input level: horizontal bar, green → amber → red, left-aligned, shows peak hold
- Output level: same layout, right-aligned
- Gain reduction: center display, shows GR in dB as the compression/saturation engages
- Water-level metaphor: bars fill bottom-to-top like a water column rising with signal level,
  consistent with the XO_OX aquatic visual language

### Stage Chain Strip (below pad)

Six stage tiles in a horizontal strip, each tile containing:
- Mini waveform/level meter (2-bar, input vs. output level for that stage)
- Stage name label
- Bypass toggle (illuminated when active, dark when bypassed)
- Amount knob (small, 20 px — defaults to "XY" mode indicated by an orbit icon;
  clicking pulls it into manual override mode, breaking the XY link)
- Expanded view (click stage tile to open a 200 × 120 px popover with all advanced controls
  for that stage — avoids cluttering the main view)

### Oxidation Visual

As `oxide_y` (Depth) increases, subtle texture overlays the XY pad:
- Y < 0.3: clean, glassy gradient
- Y = 0.5: faint rust/patina texture pattern (CSS-style grain overlay at ~20% opacity)
- Y = 1.0: heavy rust texture at ~60% opacity, slightly desaturates the gradient underneath

This communicates visually that "depth = transformation = chemical change" without interfering
with cursor readability.

---

## 9. Historical Homage

**The processing chain lineage**:
- Transient Shaper: SPL Transient Designer (1998, Wolfgang Fuchs) — first hardware transient
  processor, introduced the concept of attack/sustain as independent controls
- Saturation: Neve 1073 (1970, Rupert Neve) — tube/transformer harmonic enrichment as musical
  tool. Studer A820 (1986) — tape hysteresis as instrument character
- Compression: LA-2A (1962, Teletronix, Bill Putnam) — opto-electric leveling. 1176 (1967,
  Universal Audio, Bill Putnam) — FET peak limiting with all-buttons-in mode
- Sub Enhancement: Waves MaxxBass (2001) — psychoacoustic bass enhancement via harmonic
  generation, derived from research by Yoichi Ando on missing-fundamental perception
- Stereo Field: Orban Optimod (1975, Robert Orban) — M/S processing for broadcast consistency.
  Haas effect (Helmut Haas, 1951) — perceptual fusion zone for stereo width
- Exciter: Aural Exciter (1975, Aphex Systems, Kurt Knoppel) — high-frequency harmonic
  generation as "presence" tool, revolutionised mixing practice

**XOxide's synthesis**: What no single historical tool attempted — a unified 2D character surface
where all six stages are orchestrated simultaneously by position. The XY pad is not a controller
for any one stage; it defines a character *philosophy* that all stages enact together. The Neve
warmth, LA-2A levelling, Aphex air, and SPL transient attack are not separate tools chosen in
sequence — they are facets of a single character gesture.

---

## 10. Build Path

### Phase 1 — DSP Foundation
1. Scaffold `Source/Engines/Oxide/OxideEngine.h` and `.cpp`
2. Implement dry-through passthrough (AudioToBuffer input → stereo output)
3. Wire `oxide_x`, `oxide_y`, `oxide_inputGain`, `oxide_outputGain`, `oxide_dryWet`
4. Stage bypass controls all wired, all stages in passthrough state

### Phase 2 — Stage DSP (one stage per commit)
5. Transient Shaper (adapt TransientDesigner.h + XY mapping)
6. Saturation (adapt Saturator.h + Corroder.h for blend bus)
7. Compression (adapt Compressor.h + program-dependent mode switch)
8. Sub Enhancement (new pitch tracker + oscillator)
9. Stereo Field (adapt StereoSculptor.h + PsychoacousticWidth.h)
10. Exciter/Air (adapt HarmonicExciter.h + order selector)

### Phase 3 — LFO and Modulation
11. LFO engine (Sine/Triangle/S&H/Lorenz Chaos)
12. LFO → XY displacement routing

### Phase 4 — XOmnibus Integration
13. `REGISTER_ENGINE(OxideEngine)` in EngineRegistry.h
14. Add `"oxide_"` case to `prefixForEngine()` in XOmnibusEditor.h
15. Normalled default route: adjacent synthesis engine → AudioToBuffer → OxideEngine

### Phase 5 — Presets and Polish
16. Write all 20 factory presets in `.xometa` format
17. UI: XY pad with gradient, cursor, LFO trace, oxidation texture
18. Stage chain strip with mini meters and popovers
19. auval validation pass

**Estimated parameter count**: 47 (within comfortable single-engine range)
**Target fleet preset count**: 20 factory presets at launch (expandable to 50+ in V1.1)
