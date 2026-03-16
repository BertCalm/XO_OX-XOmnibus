# XOxide Engine — Complete Technical Specification

**XO_OX Designs | 2026-03-16**
**Status: Concept Phase — Canonical Spec. Ready for Phase 1 scaffold.**
**Engine ID:** OXIDE
**Parameter Prefix:** `oxide_`
**Accent Color:** Rust Iron `#B7410E`
**Aquatic Creature:** Pistol Shrimp (Alpheid)
**Source Documents:** `xoxide_technical_design.md` + `xoxide_competitive_analysis.md`

---

## 1. Identity

### 1.1 What XOxide Is

XOxide is a continuous 2D character shaper. It is XOmnibus's "Flavor Pro" — a character processor that covers saturation, harmonic excitation, dynamic shaping, sub enhancement, stereo field, and transient design in a single unified instrument. Drop it into any slot alongside a synthesis engine, couple the synthesis engine's output as `AudioToBuffer`, and XOxide becomes that engine's character stage.

The thesis is this: **character is a position in space, not a recipe.** Every other character processor — Decapitator, Radiator, AIR Flavor Pro — asks you to select ingredients. You pick a mode, you turn a drive knob, you add or subtract something nameable. The mental model is additive chemistry: choose your reagents, combine them, done.

XOxide rejects this model. When a sound feels warm and deep and rounded, that is not because you dialed in "tube saturation + LA-2A opto + sub enhancement" as three independent decisions. Those dimensions cohere. They are facets of a single character philosophy. XOxide is the first tool that operationalizes this insight as a navigable 2D plane.

**The compressed pitch for a sell sheet:** *The first character processor where the middle of the dial is as intentional as the extremes.*

### 1.2 The 2D Plane

The XY pad is the primary interaction. Every point on the plane is an intentional, musically coherent character philosophy.

**X axis — feliX ↔ Oscar:**
- feliX (X = -1.0): Bright, transient-forward, precise. Attack emphasis, digital/odd-harmonic saturation, FET compression, crystalline air, Haas stereo width.
- Oscar (X = +1.0): Warm, sustain-forward, rounded. Sustain emphasis, tube/even-harmonic saturation, opto compression, sub enhancement, focused stereo.
- Center (X = 0.0): Tape character. Program-dependent compression. Balanced transient. Unity stereo. The middle of the dial is not a compromise between feliX and Oscar — it is its own character. It is where the Studer A800 lives.

**Y axis — Surface ↔ Depth:**
- Surface (Y = 0.0): Nearly transparent polish. All six stages operate at such low levels that the processing is felt rather than heard. The character is present but imperceptible to conscious tracking.
- Depth (Y = 1.0): Heavy transformation. The six stages operate at full character for the current X position. This is where the sound changes.
- The Y axis scales the *magnitude* of transformation; the X axis determines its *direction*.

**Six stages fire simultaneously from the same XY position.** The XY pad is not a controller for any one stage — it defines a character philosophy that all stages enact together.

```
Input Gain → [1] Transient Shaper → [2] Saturation → [3] Compression
           → [4] Sub Enhancement → [5] Stereo Field → [6] Exciter/Air
           → Output Gain → Dry/Wet
```

All six stages are individually bypassable for power-user override. Individual stage controls are available but secondary — the XY pad is the instrument.

### 1.3 Aquatic Creature: The Pistol Shrimp

The pistol shrimp generates a cavitation bubble hotter than the surface of the sun by snapping its claw. The snap produces a shockwave, a flash of light (sonoluminescence), and a temperature briefly exceeding 8,000 Kelvin. It is the most violent character transformation in the ocean, achieved through a single precise gesture.

XOxide is the pistol shrimp of the XOmnibus fleet. A single gesture — dragging the XY cursor — unleashes coordinated transformation across six processing stages simultaneously. The result is not the sum of six effects. It is one character, made from one snap.

**feliX expression:** The shockwave — precise, devastating, crystalline. Maximum transient violence, hard digital saturation, FET brick-wall limiting, Haas widening, odd harmonic shimmer.

**Oscar expression:** The heat of the plasma bubble — warm, consuming, transformative. Heavy tube saturation, deep opto leveling, sub enhancement, focused warmth that rounds and thickens.

**The center:** Tape. The analog machine that neither punches nor transforms but *saturates with character* — the Studer A820, the Ampex 351, the neutral ground between feliX and Oscar.

### 1.4 The Middle of the Dial

The killer hook of XOxide: "The first character processor where the middle of the dial is as intentional as the extremes."

At X = 0.0 (center), Y = 0.5 (moderate depth), XOxide applies:
- **Transient:** Balanced — no attack emphasis, no sustain emphasis. Transients pass through at natural dynamics.
- **Saturation:** Tape character — Jiles-Atherton reduced hysteresis with HF soft saturation and frequency-dependent behavior. Simultaneously contributes odd and even harmonics in musically appropriate proportions. Neither digital grit nor tube warmth; the specific organic non-linearity of magnetic tape.
- **Compression:** Program-dependent — attack slows for loud transients, stays fast for quiet passages. Ratio 2.5:1. Neither FET punch nor opto leveling; a vintage VCA responding intelligently to the program material.
- **Sub:** Silent (below the 0.3 Y threshold). The sub stage does not interfere with neutral-X processing.
- **Stereo:** Unity width. No Haas delay, no narrowing. The stereo field is untouched at center X.
- **Exciter:** Trace harmonic generation above 8 kHz. 2nd + 3rd harmonic blend in balanced proportion. Barely perceptible; present as subtle presence definition.

The center position sounds like audio through a well-maintained vintage tape machine at moderate drive — the specific sound of analog that producers call "vibe" or "warmth" without being able to articulate what exactly is being added. It is not neutral. It is the most useful character position for mix bus work. Many users will never leave it.

---

## 2. Historical Homage

XOxide's six-stage chain traces a specific lineage:

**SPL Transient Designer (Wolfgang Fuchs, 1998) — Stage 1:**
The first hardware device to make attack and sustain independently controllable. Fuchs's dual-envelope splitting principle (fast follower vs. slow follower, attack signal = fast − slow) is the foundational insight of Stage 1. feliX mode is the SPL pushing attack; Oscar mode is the SPL pulling sustain. XOxide's transient shaper is a direct implementation of Fuchs's architecture.

**Neve 1073 Input Transformer (Rupert Neve, 1970) — Stage 2, Oscar side:**
The Neve 1073's transformer introduces even-harmonic enrichment and low-frequency phase rotation that is audible as "warmth" and "weight." Neve's insight: the transformer is not noise. It is character. XOxide's tube model in Stage 2 uses an asymmetrically biased tanh waveshaper that produces the 1073's specific harmonic signature — 2nd harmonic dominant, soft knee, gentle onset drive. Full Oscar at Stage 2 is the 1073 homage.

**Studer A820 Tape Transport (Studer, 1986) — Stage 2, center:**
The A820 is the reference for professional tape recording character. Its hysteresis curve produces simultaneous odd and even harmonics with HF soft saturation that "warms" transients while "softening" harsh high frequencies. XOxide's tape model in Stage 2 uses a simplified Jiles-Atherton hysteresis with non-linear gain — the center of the dial is the A820 at moderate drive.

**Ampex 351 (Ampex, 1956) — Stage 2, feliX/center:**
The Ampex 351's replay electronics gave early rock and jazz recordings their signature "sparkle" — odd harmonic content from Class A circuitry and the electronics' interaction with tape. The feliX-lean of XOxide's tape character borrows the 351's specific forward presence, where odd harmonics extend the high-frequency intelligibility rather than softening it.

**LA-2A Electro-Optical Leveler (Teletronix / Bill Putnam, 1962) — Stage 3, Oscar side:**
The LA-2A's opto-electric leveling is the defining "glue" compression character: program-dependent attack (the photoresistor's non-linear response to light), slow release (200–2000 ms), soft knee (6–12 dB), and a specific warmth from the T4B electroluminescent panel that no other compressor replicates exactly. XOxide's Oscar compressor mode is the LA-2A homage — the opto-style behavior, not a hardware emulation.

**UREI 1176 FET Limiter (Universal Audio / Bill Putnam, 1967) — Stage 3, feliX side:**
The 1176 defines FET-style compression: fast attack (0.1–1 ms), fast release, hard knee, high ratio, the "all buttons in" aggressive over-compression mode. The 1176 is punchy where the LA-2A is smooth. XOxide's feliX compressor mode is the 1176 homage — the FET character, not a hardware emulation. feliX/Depth at Stage 3 is the all-buttons-in moment.

**Aural Exciter (Aphex Systems / Kurt Knoppel, 1975) — Stage 6:**
The Aural Exciter's insight: high-frequency harmonic generation above a crossover adds perceived presence and definition without boosting actual high-frequency content. Knoppel's approach (extracting audio above the crossover, distorting it to generate harmonics, mixing back only the harmonic difference) is exactly Stage 6's implementation. feliX mode generates odd harmonics (crystalline shimmer); Oscar mode generates even harmonics (warm air). The harmonic isolation technique is direct Aural Exciter homage.

**XOxide's synthesis:** No single historical tool attempted a unified 2D character surface where all six stages are orchestrated simultaneously by position. The Neve warmth, LA-2A leveling, Aphex air, and SPL transient attack are not separate tools chosen in sequence — they are facets of a single character gesture. XOxide is the instrument that those six tools, together, were trying to be.

---

## 3. feliX / Oscar Modes

### feliX Mode (X = -1.0)

**Transient:** Maximum attack emphasis (+1.0), sustain slightly cut (-0.5). Percussive, punchy, transient-forward. SPL Transient Designer pushing attack.

**Saturation:** Hard clip with optional fold-back: `y = clamp(drive * x, -1, 1)`. Odd harmonics dominant (3rd, 5th, 7th). Optional bit reduction (8–16 bit) for lo-fi texture at extreme depths. Ampex 351 / digital distortion character.

**Compression:** FET style. Fast attack (0.1–1 ms), fast release (50–200 ms), hard knee (0–2 dB). At Y = 1.0: 20:1 limiting territory, "all buttons in" 1176 character.

**Sub:** Silent. Sub enhancement is an Oscar-exclusive stage.

**Stereo:** Wide. Side channel boosted, Haas micro-delay (1–25 ms) on one channel, perceived width up to 1.8× unity.

**Exciter:** Crystalline odd harmonics (3rd harmonic: `y = x - x³/3`). No post-harmonics rolloff. Pure shimmer and definition from the crossover frequency upward.

### Oscar Mode (X = +1.0)

**Transient:** Attack softened (-0.3), sustain fully boosted (+1.0). Smooth, legato, sustain-forward. SPL Transient Designer pulling sustain.

**Saturation:** Asymmetrically biased tanh: `y = tanh(drive * x + bias) - tanh(bias)`. Bias ≈ 0.2–0.4 introduces second harmonic dominance. Even harmonics (2nd, 4th). Soft knee, gentle onset. Neve 1073 transformer character.

**Compression:** Opto style. Program-dependent attack (fast for loud, slow for quiet). Slow release (200–2000 ms), soft knee (6–12 dB). Auto-makeup gain. LA-2A leveling character.

**Sub:** Octave-down pitch tracking. Pure sine at `f0/2`, mixed at up to 35% through a 80 Hz LP filter. Amplitude gated by the input's sub-band RMS (20–80 Hz) to prevent mud from sub-poor sources. Active above Y = 0.3.

**Stereo:** Focused. Mid channel emphasized, side reduced. Width 0.7× unity. Mono-compatible warmth. Correlation increases — the stereo image tightens toward center.

**Exciter:** Warm even harmonics (2nd harmonic: half-wave rectification approach). Post-harmonics LP rolloff softens the air into warmth. Harmonics extend deeper into the midrange as Y increases.

### The Center (X = 0.0)

**Saturation:** Tape character. Simplified Jiles-Atherton hysteresis: `tape_state += k * (x - tape_state)` where `k = f(input_level)` — faster for high inputs. HF soft saturation. Combined odd + even harmonics. Studer A820 / Ampex 351 character.

**Compression:** Program-dependent, balanced attack/release, 2.5:1 ratio, moderate knee. VCA character.

**Transient:** Balanced. Slight attack lift at feliX lean, slight sustain lift at Oscar lean. At exact center: natural dynamics.

**Stereo:** Unity width. No modification.

**Exciter:** 2nd + 3rd harmonic blend at moderate mix. Crossover frequency at midpoint between feliX and Oscar values.

---

## 4. Parameter List

### 4.1 Core / Navigation (5)

| ID | Type | Range | Default | Description |
|----|------|-------|---------|-------------|
| `oxide_x` | Float | -1.0 → +1.0 | 0.0 | feliX (-1) ↔ Oscar (+1) |
| `oxide_y` | Float | 0.0 → 1.0 | 0.0 | Surface (0) ↔ Depth (1) |
| `oxide_inputGain` | Float | -24 → +24 dB | 0.0 | Pre-chain input level |
| `oxide_outputGain` | Float | -24 → +24 dB | 0.0 | Post-chain output level |
| `oxide_dryWet` | Float | 0.0 → 1.0 | 1.0 | Dry/wet blend (parallel path) |

### 4.2 Global LFO — XY Position Modulation (5)

| ID | Type | Range | Default | Description |
|----|------|-------|---------|-------------|
| `oxide_lfoRate` | Float | 0.01 → 20 Hz | 0.25 | LFO rate for XY position modulation |
| `oxide_lfoDepth` | Float | 0.0 → 1.0 | 0.0 | LFO depth (0 = off, 1 = full plane sweep) |
| `oxide_lfoShape` | Choice | Sine / Triangle / S&H / Chaos | Sine | LFO waveform. Chaos = Lorenz attractor. |
| `oxide_lfoXAmount` | Float | -1.0 → +1.0 | 0.5 | How much LFO displaces X (bipolar) |
| `oxide_lfoYAmount` | Float | -1.0 → +1.0 | 0.5 | How much LFO displaces Y (bipolar) |

### 4.3 Stage Bypasses (6)

| ID | Default | Description |
|----|---------|-------------|
| `oxide_bypassTransient` | false | Bypass transient shaper stage |
| `oxide_bypassSaturator` | false | Bypass saturation stage |
| `oxide_bypassCompressor` | false | Bypass compression stage |
| `oxide_bypassSub` | false | Bypass sub enhancement stage |
| `oxide_bypassStereo` | false | Bypass stereo field stage |
| `oxide_bypassExciter` | false | Bypass exciter/air stage |

### 4.4 Stage Amounts — XY Override (6)

Default -1.0 = XY-driven. Any value ≥ 0 overrides the XY mapping for that stage, enabling per-stage manual control:

| ID | Range | Default |
|----|-------|---------|
| `oxide_transientAmount` | -1.0 (auto) → 1.0 | -1.0 |
| `oxide_saturatorAmount` | -1.0 (auto) → 1.0 | -1.0 |
| `oxide_compressorAmount` | -1.0 (auto) → 1.0 | -1.0 |
| `oxide_subAmount` | -1.0 (auto) → 1.0 | -1.0 |
| `oxide_stereoAmount` | -1.0 (auto) → 1.0 | -1.0 |
| `oxide_exciterAmount` | -1.0 (auto) → 1.0 | -1.0 |

### 4.5 Stage Advanced Controls (19)

#### Transient Shaper (3)
| ID | Range | Default |
|----|-------|---------|
| `oxide_transAttack` | 0.1 → 50 ms | XY-driven |
| `oxide_transRelease` | 10 → 500 ms | XY-driven |
| `oxide_transGain` | -12 → +12 dB | 0.0 |

#### Saturation (3)
| ID | Range | Default | Notes |
|----|-------|---------|-------|
| `oxide_satDrive` | 0.0 → 1.0 | XY-driven | Input drive into waveshaper |
| `oxide_satBlend` | -1.0 → +1.0 | XY-driven | Tube (Oscar, +1) ↔ Digital (feliX, -1) |
| `oxide_satCompensate` | Bool | true | Output level compensation |

#### Compression (6)
| ID | Range | Default |
|----|-------|---------|
| `oxide_compThreshold` | -60 → 0 dBFS | XY-driven |
| `oxide_compRatio` | 1:1 → 20:1 | XY-driven |
| `oxide_compAttack` | 0.1 → 100 ms | XY-driven |
| `oxide_compRelease` | 10 → 2000 ms | XY-driven |
| `oxide_compKnee` | 0.0 → 12 dB | XY-driven |
| `oxide_compMakeup` | Bool | true |

#### Sub Enhancement (3)
| ID | Range | Default |
|----|-------|---------|
| `oxide_subOctaveMix` | 0.0 → 1.0 | XY-driven |
| `oxide_subTracking` | 1 → 500 ms | XY-driven |
| `oxide_subLowpass` | 40 → 200 Hz | 80 Hz |

#### Stereo Field (3)
| ID | Range | Default | Notes |
|----|-------|---------|-------|
| `oxide_stereoWidth` | 0.0 → 2.0 | XY-driven | M/S width (1.0 = unity) |
| `oxide_stereoHaas` | 0.0 → 25 ms | XY-driven | Haas micro-delay time |
| `oxide_stereoMono` | Bool | false | Hard mono collapse (monitoring) |

#### Exciter / Air (3)
| ID | Range | Default |
|----|-------|---------|
| `oxide_exciterFreq` | 3000 → 16000 Hz | XY-driven |
| `oxide_exciterOrder` | 2nd / 3rd / 4th | XY-driven |
| `oxide_exciterMix` | 0.0 → 1.0 | XY-driven |

### 4.6 Macros (4)

| ID | Label | Primary Mapping | Secondary Mapping |
|----|-------|-----------------|-------------------|
| `oxide_macroCharacter` | **GRIT** (M1) | `oxide_x` full range | `oxide_satBlend` echoes X |
| `oxide_macroCoupling` | **WARMTH** (M2) | `oxide_y` full range | `oxide_compRatio` echoes Y at Depth |
| `oxide_macroMovement` | **CHAOS** (M3) | `oxide_lfoDepth` 0→1 + `oxide_lfoRate` | LFO shape morphs Sine→Chaos as M3 increases |
| `oxide_macroSpace` | **BLEND** (M4) | `oxide_dryWet` 0→1 | `oxide_stereoWidth` 1.0→1.4 simultaneously |

**Note on macro labels:** The default labels (GRIT, WARMTH, CHAOS, BLEND) communicate the character dimension each macro controls. Sound designers may rename per preset in the `.xometa` preset file.

### 4.7 Total Parameter Count

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

## 5. The Lorenz LFO — Chaos as Character

### 5.1 Why Chaos Beats a Standard LFO

A standard LFO is periodic. It returns to the same position after every cycle. A character processor driven by a periodic LFO produces a predictable groove — every two bars, the sound returns to identical character. This is mechanical.

The Lorenz attractor is a deterministic dynamical system that never repeats. At any musically useful rate, its period is effectively infinite. The character evolution it drives sounds like organic drift: it moves in one direction, reverses, wanders, doubles back — all within bounded territory.

For XOxide specifically:
- **Bounded:** The attractor stays within bounded state space. Projected onto the XY plane, character never escapes to saturated extremes unexpectedly.
- **Ergodic:** Over long time periods, the attractor visits all allowed regions of the XY plane. A sine LFO only ever visits the same arc.
- **Self-similar but non-periodic:** The trajectory returns to the vicinity of previous positions but never exactly repeats. The producer's ear interprets this as organic, non-mechanical evolution.
- **Smooth:** The Lorenz trajectory in XY space is continuous and differentiable. Character transitions feel like natural evolution, not switching. S&H jumps; Lorenz glides.

A pad processed through XOxide at Chaos LFO mode with moderate depth sounds like it was played through a vintage hardware chain where the tube is warming up, the tape transport is slightly irregular, and the compressor is reacting to the room. It sounds alive because it is alive — the character is non-stationary in the same way a live analog chain is non-stationary.

### 5.2 The Lorenz System

The Lorenz attractor is defined by three coupled differential equations:

```
dx/dt = σ(y - x)
dy/dt = x(ρ - z) - y
dz/dt = xy - βz
```

Three parameters control the attractor's shape and behavior:

**σ (Sigma) — Correlation Rate:**
Controls how quickly `x` responds to changes in `y`. Musically: sensitivity and responsiveness of character evolution.
- Low σ (3–6): slow, wide, lazy sweeps. Character moves with broad gestures.
- High σ (15–20): tight, nervous movement. Character responds quickly and changes direction often.
- Default musical value: 10.0

**ρ (Rho) — Chaos Depth:**
The Rayleigh number. Controls how far the attractor strays from its center. Below ρ = 24.74, the system converges to a fixed point (not chaotic). At ρ = 28, the classic butterfly attractor emerges.
- Low ρ (near 24.74): character stays close to initial XY position. Barely any wandering.
- High ρ (35–50): butterfly wings expand. Character visits wide regions of the XY plane.
- Default musical value: 28.0

**β (Beta) — Dissipation Rate / Lobe Density:**
Controls trajectory density on the attractor surface. At canonical 8/3: well-distributed. Higher β: more time in the butterfly "wings," longer sustained character positions before switching lobes. Lower β: faster lobe transitions.
- Default musical value: 8/3 (≈2.667)
- Low β (1.5–2.0): faster lobe transitions, more frequent character switching between feliX-leaning and Oscar-leaning territory.

### 5.3 Musical Lorenz Presets

**Subtle Organic Movement** (σ=8.0, ρ=26.5, β=8/3, rate=0.08 Hz, depth=0.2, X-amount=0.5, Y-amount=0.3):
The character breathes imperceptibly. Most producers will not consciously hear the evolution but will feel the sound is "alive." Best for pads, drones, sustained textures. This is the preset that ships as the default Chaos behavior.

**Analogue Warmth Drift** (σ=10.0, ρ=28.0, β=8/3, rate=0.03 Hz, depth=0.35, X-amount=0.6, Y-amount=0.4):
Slow, wide sweeps through Oscar-warm territory. Suitable for long ambient compositions where character needs to evolve over 5+ minutes. Feels like a studio that warms up over the course of a session.

**Unpredictable Chaos** (σ=14.0, ρ=38.0, β=1.8, rate=0.4 Hz, depth=0.75, X-amount=0.8, Y-amount=0.7):
Aggressive, unpredictable character evolution. The sound never settles. Processing ranges from crystalline to warm to crushing and back. Experimental and performance-context.

### 5.4 Implementation

The Lorenz system integrates with fixed-step RK4 (Runge-Kutta 4th order) at audio control rate (every 64 samples at 48 kHz = ~750 Hz update rate). The attractor's x and y components are normalized to [-1, +1] using known attractor bounds for the given σ, ρ, β values:

```
x_driven = clamp(oxide_x + lorenz_x_norm * oxide_lfoXAmount * oxide_lfoDepth, -1, +1)
y_driven = clamp(oxide_y + lorenz_y_norm * oxide_lfoYAmount * oxide_lfoDepth,  0,  1)
```

The z component is not used — it does not project well onto the 2D plane.

---

## 6. XY → Parameter Mapping

The complete mapping from normalized (x ∈ [-1,+1], y ∈ [0,1]) to stage parameters. All interpolations are linear unless noted.

**Notation:** `x_n = (oxide_x + 1) / 2` → 0 = feliX, 1 = Oscar.

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

Note: Y axis uses a logarithmic curve in the 0.0–0.5 range (gentle Surface-to-midpoint gradient) and linear from 0.5–1.0 (dramatic midpoint-to-Depth gradient). Most musically useful character enhancement lives in the 0.2–0.6 Y range.

### Compression

| Parameter | feliX | Center | Oscar | Y scaling |
|-----------|-------|--------|-------|-----------|
| Attack mode | FET 0.3 ms | 2 ms | Opto 20 ms | — |
| Release mode | Fast 80 ms | 250 ms | Program-dep 800 ms | — |
| Knee | 0.5 dB | 3 dB | 9 dB | — |
| Ratio | 4:1 | 2.5:1 | 1.8:1 | × y² (quadratic — nearly transparent at Surface) |

At Y = 0, ratio is 1.1:1 for all X positions — essentially transparent.

### Sub Enhancement

| Parameter | feliX | Center | Oscar | Y scaling |
|-----------|-------|--------|-------|-----------|
| Sub gate | 0.0 (silent) | 0.0 | 1.0 | × max(0, y - 0.3) / 0.7 |
| Octave mix | 0.0 | 0.0 | y × 0.35 | — |

Sub is gated below Y = 0.3 at all X positions. Oscar-exclusive.

### Stereo Field

| Parameter | feliX | Center | Oscar | Y scaling |
|-----------|-------|--------|-------|-----------|
| Width | 1.0 + y × 0.8 | 1.0 | 1.0 - y × 0.3 | (built in) |
| Haas delay | y × 15 ms | 0 | 0 | — |
| Mid boost (dB) | 0 | 0 | y × 2 dB | — |

At center X, the stereo field stage has no effect at any depth.

### Exciter / Air

| Parameter | feliX | Center | Oscar | Y scaling |
|-----------|-------|--------|-------|-----------|
| Frequency | lerp(12k, 8k, y) + lerp(10k, 5k, x_n) combined | — | — | (built in) |
| Harmonic order | 3rd (odd) | 2nd+3rd blend | 2nd (even) | — |
| Mix | 0.0 + y × 0.5 | 0.0 + y × 0.35 | 0.0 + y × 0.4 | (built in) |

Exciter frequency is influenced by both X and Y: feliX biases upward (pure air), Oscar biases downward (midrange warmth), deeper Y lowers the crossover into midrange territory.

### Quadrant Summary

| Quadrant | feliX/Surface | feliX/Depth | Oscar/Surface | Oscar/Depth |
|----------|--------------|-------------|--------------|-------------|
| Transient | Slight attack lift | Strong attack emphasis, sustain cut | Slight sustain lift | Strong sustain emphasis, attack softened |
| Saturation | Trace digital grit | Aggressive hard clip fold-back | Trace tube warmth | Deep tube saturation, even harmonics |
| Compression | Nearly transparent FET | Hard limiting, fast pumping | Nearly transparent opto | Deep LA-2A levelling |
| Sub | Silent | Silent | Silent | Octave-down sine tracking |
| Stereo | Unity | Wide + Haas widening | Unity | Slightly narrowed/focused |
| Exciter | Trace odd shimmer | Full crystalline odd harmonics from 4 kHz | Trace warm air | Full warm even harmonics from 4 kHz |

---

## 7. XPN Integration

### 7.1 Character Baking via xpn_complement_renderer.py

XOxide's processing chain can be applied to rendered XPN samples so producers retain character even on MPC hardware without the plugin running.

**Stages that bake cleanly (full fidelity):**
- Stage 1 (Transient): Attack/sustain shaping applies identically offline
- Stage 2 (Saturation): Tube, tape, and digital blend render identically offline — all waveshaper-based
- Stage 4 (Sub): Octave-down synthesis with pitch tracking bakes cleanly; the tracked fundamental at render time matches playback
- Stage 6 (Exciter): Harmonic generation applies identically offline

**Stages that cannot fully bake (dynamic content):**
- Stage 3 (Compression): Compressor response depends on playback dynamics. Baked samples have the dynamic shaping applied to the Oxport render signal, not future MIDI playback. Will sound over-processed if velocity-varied during MPC playback.
- Stage 5 (Stereo Field): Haas delay bakes fine. M/S width processing interacts with playback context dynamically. If `oxide_stereoWidth > 1.3`, flag for manual MPC mixer width application.

**Integration flow:**

```python
# xpn_complement_renderer.py
def render_oxide_character(sample_buffer, oxide_preset, sample_rate):
    """
    Apply XOxide character stages 1, 2, 4, 6 to a rendered sample buffer.
    Stage 3 (compression) and Stage 5 (stereo) are noted but not baked.
    """
    x = oxide_preset['oxide_x']
    y = oxide_preset['oxide_y']

    # Stage 1: Transient shaping
    buffer = apply_transient_shaper(buffer, x, y, sample_rate)

    # Stage 2: Saturation (tube/tape/digital blend)
    buffer = apply_saturation_blend(buffer, x, y, sample_rate)

    # Stage 4: Sub enhancement (if active)
    if y > 0.3 and x > 0.0:
        buffer = apply_sub_enhancement(buffer, x, y, sample_rate)

    # Stage 6: Exciter/air
    buffer = apply_exciter(buffer, x, y, sample_rate)

    # Log bake status for manifest
    notes = ['stages 1,2,4,6 baked']
    if oxide_preset.get('oxide_bypassCompressor', False) is False:
        notes.append('stage 3 (compression) not baked — apply at MPC mixer')
    if oxide_preset.get('oxide_stereoWidth', 1.0) > 1.3:
        notes.append('stage 5 (stereo width) not baked — apply at MPC mixer')

    return buffer, notes
```

The XPN bundle metadata includes the XOxide snapshot:

```json
{
  "xoxide_snapshot": {
    "preset_name": "Amber Morning",
    "oxide_x": 0.6,
    "oxide_y": 0.4,
    "baked_stages": [1, 2, 4, 6],
    "dynamic_stages_not_baked": [3, 5],
    "lorenz_active": false
  }
}
```

### 7.2 Velocity Sensitivity Integration

The Sonic DNA velocity curves map naturally onto XOxide's character space:

- **Vibe's musical velocity curve** (S-curve, soft at low velocities, exponential above 70%) → Y axis: low velocities stay near Surface, high velocities push toward Depth. Loud hits transform more deeply.
- **feliX velocity response:** At feliX X positions, the curve creates more transient attack boost at high velocities — correct, since loud hits should feel more percussive.
- **Oscar velocity response:** At Oscar X positions, the curve creates more warm/sustained character at low velocities — correct, since quiet intimate playing benefits from warmth.

Future `oxide_velocitySensitivity` parameter (V1.1 candidate): scales how much velocity modulates `oxide_y`. At 0.0: static depth. At 1.0: velocity fully determines depth. The first character processor where playing dynamics change timbre holistically.

---

## 8. Coupling Design

### 8.1 XOmnibus Integration

XOxide is an FX processor — it generates no audio of its own. `getMaxVoices()` returns 0. XOxide ignores MIDI note events but remains responsive to MIDI CC (mod wheel → `oxide_y`, expression → `oxide_dryWet`).

The PlaySurface hides the pad grid when XOxide is in a slot and shows the 2D XY pad as the primary control surface instead.

**Normalled default:** When a synthesis engine is in Slot 1 and XOxide is in Slot 2, the normalled default route is Slot 1 `AudioToBuffer` → Slot 2 at amount 1.0. Placing XOxide adjacent to any synthesis engine "just works" with no manual routing.

### 8.2 Coupling Accepted

| Coupling Type | Role |
|--------------|------|
| `AudioToBuffer` | Primary: receives continuous stereo audio from another slot's output |
| `AmpToFilter` | Sidechain: external amplitude modulates `oxide_y` (auto-duck behavior) |
| `LFOToPitch` | Clock sync: external LFO locks `oxide_lfoRate` to host BPM |

### 8.3 Coupling Emitted

| Coupling Type | What XOxide Sends |
|--------------|-------------------|
| `AudioToBuffer` | Processed audio downstream (e.g., into OPAL grain buffer) |
| `AmpToFilter` | Post-compression gain reduction value as modulation signal |
| `EnvToMorph` | Detected input transient envelope (can drive ODYSSEY morph, etc.) |

### 8.4 Coupling with Chaos Engines

XOxide's drive amount accepts modulation from OUROBOROS (chaos engine) and ORACLE (stochastic engine):

- **OUROBOROS → AmpToFilter → oxide_y:** OUROBOROS's chaotic output drives XOxide's depth parameter. The Lorenz-based synthesis engine feeds into XOxide's Lorenz-based LFO character space. Chaos driving chaos — the character of the character shaper becomes as unpredictable as the source signal.
- **ORACLE → LFOToPitch → oxide_lfoRate:** ORACLE's stochastic output modulates the LFO rate, producing tempo-irregular character evolution. Every preset playing through this combination has a different rate profile each performance.
- **XOxide → EnvToMorph → ODYSSEY:** XOxide's detected transient envelopes become the morph driver for ODYSSEY. The physical character of the audio (attack intensity) drives the synthesis engine's timbral morph. Coupling in the reverse direction: the FX engine drives the synthesizer.

---

## 9. Macros

| Macro | Label | Primary Mapping | Secondary Mapping | Sweet Spot |
|-------|-------|-----------------|-------------------|------------|
| M1 | **GRIT** | `oxide_x` full range | `oxide_satBlend` echoes X | M1=0.35 (X=-0.3): feliX lean without digital harshness. The "punchy and defined" mix bus position. |
| M2 | **WARMTH** | `oxide_y` full range | `oxide_compRatio` echoes Y | M2=0.3 with M1=0.35: professional mix bus enhancer — transparent polish with character. |
| M3 | **CHAOS** | `oxide_lfoDepth` 0→1 + `oxide_lfoRate` | Shape morphs Sine→Chaos as M3 increases | M3=0.25: sound breathes but cycle is imperceptible. Maximum organicism for ambient work. |
| M4 | **BLEND** | `oxide_dryWet` 0→1 | `oxide_stereoWidth` 1.0→1.4 simultaneously | M4=0.6: parallel saturation at 60% wet — most useful for mix bus work, preserves dry transients. |

---

## 10. Factory Presets (20 concepts — 150 target)

### Foundation Mood (near plane center, subtle)

| Name | Position | Character |
|------|----------|-----------|
| **Quiet Refinement** | X=0.0, Y=0.15 | Near-passthrough. Trace tape warmth. Use: mastering-style gentle enhancement. |
| **Steel Silk** | X=-0.2, Y=0.25 | Small attack boost, trace digital grit, crystalline air above 10 kHz. Use: adding definition to pads without brightness. |
| **Warm Glue** | X=+0.3, Y=0.3 | Opto-style 1.5:1, trace tube even harmonics. Use: gluing a mix with slight vintage character. |
| **Still Water** | X=+0.5, Y=0.1 | Slight width reduction to 0.85. Mono-compatible warmth. |

### Atmosphere Mood (Oscar/Surface quadrant, gentle warmth)

| Name | Position | Character |
|------|----------|-----------|
| **Amber Morning** | X=+0.6, Y=0.4 | Vintage warmth — tube drive 0.3, opto 1.8:1, warm even harmonics at 7 kHz. |
| **Deep Swell** | X=+0.7, Y=0.6 | All stages at moderate Oscar depth. Sub octave mix 0.15 for bass voices. |
| **Velvet Compress** | X=+0.5, Y=0.7 | Opto 3:1, slow 30 ms attack, 1.2s release. Tube saturation. Use: levelling vocals with character. |
| **Analog Ghost** | X=+0.3, Y=0.5 + LFO Sine 0.03 Hz depth 0.15 X-only | Barely perceptible breathing — character lives and dies slowly. |

### Prism Mood (dramatic, LFO active, moving between quadrants)

| Name | Position | Character |
|------|----------|-----------|
| **Pulse Prism** | X=0.0, Y=0.5 + Triangle 0.5 Hz depth 0.8 X-amount=1.0 | Sweeps feliX↔Oscar at 0.5 Hz — transient/sustain pumping alternation. Rhythmic character morphing. |
| **Depth Dive** | X=+0.4, Y=0.4 + Sine 0.2 Hz Y-amount=1.0 | Breathes in and out of depth — sub and compression rise and fall. |
| **Nautilus Path** | X=0.0, Y=0.0 + Chaos 0.08 Hz depth 0.9 | Lorenz attractor traces organic path through entire plane. Never sounds the same twice. |
| **Crystal Storm** | X=-0.6, Y=0.6 + Sine 0.15 Hz | Strong feliX — transient attack hard, digital saturation, FET at 4:1, Haas width, crystalline harmonics. |

### Flux Mood (extreme, heavy processing, corners of the plane)

| Name | Position | Character |
|------|----------|-----------|
| **Iron Fist** | X=-1.0, Y=1.0 | Maximum feliX/Depth. Transient maximum, digital fold-back, FET 20:1, odd harmonics from 3 kHz. The pistol shrimp snap. |
| **Magma Core** | X=+1.0, Y=1.0 | Maximum Oscar/Depth. Sustain fully boosted, heavy tube drive, deep opto levelling, sub at 0.3 mix. |
| **Oxidize Heavy** | X=+0.2, Y=1.0 | Center/Depth. All six stages near-maximum, tape saturation dominant. Lo-fi tape destruction. |
| **Cavitation** | X=-0.9, Y=0.85 + Triangle 2.0 Hz depth 0.6 Y-amount=0.8 | Fast LFO pumps depth — stutter-like aggressive-to-subtle cycling at 2 Hz. EDM sidechain pumping effect. |
| **Event Horizon** | X=-1.0, Y=0.9 + Chaos 0.05 Hz Y-amount=1.0 | Locked at maximum feliX, chaos varies depth only. Evolving crystalline destruction. |
| **Black Velvet** | X=+0.9, Y=0.8 + Sine 0.03 Hz depth 0.3 X-amount=0.3 | Deep Oscar with slow barely-perceptible breathing on X axis. Deep, dark, warm transformation. |
| **Rust and Chrome** | X=0.0, Y=0.75 + S&H 0.12 Hz depth 0.7 X-amount=1.0 | S&H jumps to random X positions every ~8 seconds. Each jump re-characterizes the source differently. |
| **Pistol Snap** | X=-0.8, Y=0.9 + LFO off | Maximum transient emphasis + digital clip + FET brick-wall + crystalline exciter. No sustain, no warmth, no sub — pure impact. |

---

## 11. Doctrine Compliance

| Doctrine | Compliance |
|----------|-----------|
| D001: Velocity shapes timbre | PASS — `oxide_y` can be modulated by velocity via MIDI CC routing. Loud = deeper transformation. |
| D002: Modulation is lifeblood | PASS — 4 macros, Lorenz chaos LFO modulating XY position, coupling input auto-duck behavior |
| D003: Physics IS the synthesis | PASS — Jiles-Atherton hysteresis for tape (simplified but cited); Lorenz attractor (Lorenz 1963, cited); dual-envelope transient (Fuchs 1998) |
| D004: Dead parameters forbidden | PASS — 47 params, all wired; stage overrides default to XY-driven (-1.0) but are audible when manually set |
| D005: Engine must breathe | PASS — Lorenz LFO minimum rate 0.01 Hz; autonomous character modulation |
| D006: Expression input not optional | PASS — mod wheel → `oxide_y`, expression → `oxide_dryWet`, velocity → `oxide_y` optional |

---

## 12. Engine Registration

```cpp
REGISTER_ENGINE("XOxide", []() -> std::unique_ptr<SynthEngine> {
    return std::make_unique<OxideEngine>();
});

// getEngineId() → "Oxide"
// getAccentColour() → juce::Colour(0xFFB7410E)  // Rust Iron
// getMaxVoices() → 0 (FX engine, no voice concept)
// Parameter prefix: "oxide_"
```

Add case to `prefixForEngine()` in `XOmnibusEditor.h`:
```cpp
if (engineId == "Oxide") return "oxide_";
```

---

## 13. Build Sequencing

| Phase | Deliverable | Notes |
|-------|-------------|-------|
| Phase 0 | Concept (this document) | COMPLETE |
| Phase 1 | `OxideEngine.h` scaffold: SynthEngine interface, 47 params, dry-through passthrough | Standalone builds, silence output |
| Phase 2 | Transient Shaper (Stage 1) with XY mapping | Adapt `TransientDesigner.h` |
| Phase 3 | Saturation (Stage 2): tube/tape/digital blend bus | Adapt `Saturator.h` + `Corroder.h` |
| Phase 4 | Compression (Stage 3): FET/Opto blend | Adapt `Compressor.h` + program-dependent mode switch |
| Phase 5 | Sub Enhancement (Stage 4): pitch tracker + octave-down oscillator | New DSP |
| Phase 6 | Stereo Field (Stage 5): M/S width + Haas delay | Adapt `StereoSculptor.h` + `PsychoacousticWidth.h` |
| Phase 7 | Exciter / Air (Stage 6): harmonic order selector | Adapt `HarmonicExciter.h` |
| Phase 8 | LFO engine (Sine / Triangle / S&H / Lorenz RK4) | New DSP — Lorenz integration |
| Phase 9 | LFO → XY displacement routing | Wire LFO to XY with X/Y amount params |
| Phase 10 | XOmnibus integration: REGISTER_ENGINE, prefixForEngine | Normalled default route |
| Phase 11 | Factory presets (20 launch → 150 V1 target) | All 4 mood categories populated |
| Phase 12 | UI: XY pad with gradient, cursor, LFO trace, oxidation texture | Stage chain strip with mini meters and popovers |
| Phase 13 | Oxport: xpn_complement_renderer.py integration | Stages 1,2,4,6 bake path |
| Phase 14 | auval validation pass | |

---

*Authored: XO_OX Designs, 2026-03-16*
*Document type: Canonical engine specification. Supersedes standalone design documents as the definitive reference.*
*Source materials: `xoxide_technical_design.md` (DSP + XY mapping), `xoxide_competitive_analysis.md` (Vibe, competitive position + Lorenz analysis)*
