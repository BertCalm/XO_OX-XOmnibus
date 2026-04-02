# XOceanic — Master Build Spec

**Brand:** XO_OX Designs
**Instrument:** XOceanic
**Version:** 1.0
**Spec Date:** March 2026

---

## Section 1: Executive Summary

**Instrument Name:** XOceanic
**Brand:** XO_OX Designs

**Core Identity:** A string ensemble synth with a bioluminescent effects processor — the strings provide warmth and body, the chromatophore pedalboard reveals colors hiding inside them that you need *different eyes* to see. Arturia makes the Solina, Chase Bliss makes the pedal. Sold as a bundle. Themed as a freaky deep-sea creature.

**Primary Use Cases:**

1. Lush ensemble string pads — warm, immediate, organ-like harmonic coherence
2. Shimmering ambient textures — strings processed through spectral freeze, scatter, shimmer
3. Evolving atmospheric beds — chromatophore modulation makes the sound pulse and breathe
4. Coupling gateway — any engine's audio can enter the chromatophore pedalboard
5. Cinematic string washes — reverse + shimmer + freeze for film/game scoring

**Non-Goals:**

- NOT a lead synth — no per-voice articulation or fast envelopes
- NOT a bass synth — paraphonic architecture doesn't suit low-end precision
- NOT a polyphonic synth — it's paraphonic by design; chord notes blend, not stack
- NOT a standard FX engine — OVERDUB handles send/return; OCEANIC's effects *reveal*, not process

---

## Section 2: Product Pillars

### Pillar 1: The Ensemble Is the Instrument
The triple-BBD ensemble chorus IS the sound. Without it, you have organ stops. With it, you have the Solina — alive, shimmering, warm. The ensemble depth/rate must be expressive enough to be the primary sound-design tool.

### Pillar 2: Effects Reveal, Not Add
The chromatophore pedalboard doesn't "add reverb" or "add delay." Each module reveals hidden spectral content already present in the string signal — harmonics exposed by pitch-shifted reverb tails, frozen moments of harmonic complexity, time-scattered particles of the ensemble. The metaphor is literal: these are colors that were always there, needing different eyes.

### Pillar 3: Organic Pulsing
The Chromatophore Modulator ties the entire pedalboard together into a single living organism. Effects don't sit static — they breathe, pulse, shift, like cuttlefish skin. The separation parameter controls whether they breathe as one or independently.

### Pillar 4: Paraphonic Warmth
All notes share one filter and one envelope. Chords blend into a single timbral mass — you hear *one instrument playing a chord*, not *several instruments playing notes*. This is a feature, not a limitation. It's why Solina string pads feel fundamentally different from polyphonic pads.

### Pillar 5: Coupling as Revelation
In XOceanus, any engine's audio can enter the chromatophore pedalboard. OVERWORLD's chip audio scattered through SCATTER. ODYSSEY's Climax frozen by FREEZE. OVERBITE's bass warped by TIDE. The pedalboard reveals hidden colors in *any* engine — this is OCEANIC's gallery role.

---

## Section 3: Brand & Concept Translation

**Working Concept Line:** "Like deep-sea creatures whose beauty needs different eyes to see"

**Primary Identity Axis:** Warm String Body <-> Impossible Bioluminescent Color
- *Warm:* Pure Solina ensemble — vintage, organic, immediately beautiful
- *Bioluminescent:* Full chromatophore processing — spectral freeze, scatter, shimmer, reverse — colors that shouldn't exist

**Secondary Behaviors:**

1. **Ensemble Shimmer** — the triple-BBD chorus that makes everything alive
2. **Spectral Freeze** — capturing a moment's harmonic content and holding it indefinitely
3. **Chromatophore Pulse** — rhythmic, organic modulation of all pedalboard parameters
4. **Tidal Warp** — tape degradation and pitch shift in the delay feedback

---

## Section 4: Vibe Matrix

| Axis | Soft Pole | Hard Pole |
|------|-----------|-----------|
| **Body** | Warm, dense string bed | Thin, spectral shimmer |
| **Movement** | Still, frozen | Pulsing, breathing |
| **Processing** | Dry ensemble (pure Solina) | Full chromatophore (all pedals) |
| **Temporal** | Sustained, infinite | Scattered, rhythmic |
| **Space** | Close, intimate | Vast abyss |

---

## Section 5: Sonic Signature Traits

### Trait 1: Triple Ensemble
The triple-phase BBD chorus: 3 modulated delay lines at different rates, creating the Solina's signature shimmer. This is the irreducible core — without it, XOceanic is just an organ. With it, everything becomes warm velvet fog.

### Trait 2: Spectral Freeze
Capture any moment of the string signal and sustain it indefinitely. Unlike a simple loop, FREEZE captures the *harmonic content* at a point in time — the frozen chord sustains without rhythmic artifacts or loop boundaries.

### Trait 3: Chromatophore Pulse
The global organic modulation that makes all active pedalboard modules breathe together. Rate, depth, pattern, and inter-pedal separation create the feeling of a living creature's skin shifting color. This is XOceanic's equivalent of ODYSSEY's Tidal Pulse — but applied to effects, not oscillators.

### Trait 4: Shimmer Abyss
Pitch-shifted reverb tails that expose harmonics octaves above the source. A C major string chord produces shimmering E-G-C tails an octave or two higher — harmonics that were always present in the string signal, now revealed by the pitch-shifted reverb feedback network.

### Trait 5: Tidal Warp
The tape delay with pitch-shifted feedback. Each delay repeat degrades slightly (tape degradation highcut) and shifts pitch. A single chord becomes a descending or ascending cascade of increasingly degraded echoes — like sound sinking deeper into the ocean.

---

## Section 6: High-Level Audio Architecture

### Signal Flow

```
LAYER A: STRING ENSEMBLE (Paraphonic)
═══════════════════════════════════════════════════════════

MIDI Note-On/Off → Note Gate Table (128 entries)
                           │
                           ▼
Divide-Down Oscillator Bank (all active notes)
├── Each note: bandlimited pulse/saw waveshape
├── Waveshape differs per registration stop
└── All notes always available, gated by MIDI
                           │
                           ▼
Registration Mixer (organ-style additive stops)
├── VIOLIN    (8')  — narrow pulse, bright, present
├── VIOLA     (8')  — wider pulse, darker, nasal
├── CELLO     (8')  — triangle-ish, warm, full body
├── BASS      (16') — one octave down, rounded
├── CONTRABASS(32') — two octaves down, fundamental only
├── HORN      (8')  — hollow square, brassy
│   Each stop has: level (0-1), stereo position (separation)
                           │
                           ▼
Brightness Control (pre-ensemble treble shaping)
                           │
                           ▼
Triple Ensemble Chorus (BBD simulation)
├── Line 1: 5-8ms delay, LFO @ 0.63 Hz, depth modulated
├── Line 2: 6-9ms delay, LFO @ 0.95 Hz, depth modulated
├── Line 3: 7-10ms delay, LFO @ 1.40 Hz, depth modulated
│   Mix: dry + 3 wet lines, normalized
│   (oceanic_ensemble controls wet/dry ratio)
│   (oceanic_ensRate scales all 3 LFO rates proportionally)
                           │
                           ▼
Paraphonic Filter (shared — all notes through ONE filter)
├── Lowpass (2-pole SVF, warm default)
├── Bandpass (for nasal string color)
├── Highpass (for thin ethereal strings)
│   Cutoff, resonance, type selectable
│   Envelope amount (bipolar) from shared ADSR
                           │
                           ▼
Paraphonic Amp Envelope (shared ADSR)
├── Retriggers on any new note-on
├── Releases on last note-off
├── All sounding notes affected identically
                           │
                           ▼
Analog Drift (per-oscillator slow pitch wander, 0.05-0.5Hz)
                           │
                           ▼
STRING OUTPUT ─────────────┬──→ [DRY PATH] ──→ Dry/Wet Mixer
                           │                            ↑
                           ▼                            │
LAYER B: CHROMATOPHORE PEDALBOARD                       │
═══════════════════════════════════════════             │
                           │                            │
(Optional: External coupling audio mixed in here)       │
                           │                            │
                           ▼                            │
┌─── FREEZE ──────────────────────────────────┐        │
│  Ring buffer capture (2s)                    │        │
│  freeze/release toggle                       │        │
│  Crossfade between live and frozen           │        │
│  oceanic_freezeMix, oceanic_freezeGain       │        │
└──────────────┬──────────────────────────────┘        │
               ▼                                        │
┌─── SCATTER ─────────────────────────────────┐        │
│  Granular micro-loop                         │        │
│  Grain size: 5-500ms                         │        │
│  Density: 1-80 grains/sec                    │        │
│  Pitch scatter: 0-12st                       │        │
│  Pan scatter: 0-1 (stereo width)             │        │
│  Window: Hann (default)                      │        │
│  Max 24 simultaneous grains                  │        │
│  oceanic_scatterSize/Density/Pitch/Pan/Mix   │        │
└──────────────┬──────────────────────────────┘        │
               ▼                                        │
┌─── TIDE ────────────────────────────────────┐        │
│  Warped tape delay                           │        │
│  Time: 10-2000ms                             │        │
│  Feedback: 0-110% (self-osc >100%)           │        │
│  Wow/flutter: Perlin noise pitch mod         │        │
│  Pitch shift: -12 to +12st in feedback loop  │        │
│  Tape degradation: LP in feedback (darkens)  │        │
│  tanh saturation in feedback loop            │        │
│  oceanic_tideTime/Feedback/Warp/Pitch/Mix    │        │
└──────────────┬──────────────────────────────┘        │
               ▼                                        │
┌─── ABYSS ───────────────────────────────────┐        │
│  Shimmer reverb (Dattorro plate + pitch)     │        │
│  Decay: 0.5s to infinite                     │        │
│  Pitch shift in feedback: +12st or +24st     │        │
│  Creates ascending harmonic tails            │        │
│  HF damping: frequency-dependent decay       │        │
│  Size: small plate to large hall             │        │
│  oceanic_abyssDecay/Shift/Damp/Size/Mix      │        │
└──────────────┬──────────────────────────────┘        │
               ▼                                        │
┌─── MIRROR ──────────────────────────────────┐        │
│  Reverse buffer                              │        │
│  Length: 50-2000ms                            │        │
│  Double-buffer with auto-crossfade           │        │
│  Blend: forward (0) ↔ reversed (1)           │        │
│  oceanic_mirrorLength/Blend/Mix              │        │
└──────────────┬──────────────────────────────┘        │
               ▼                                        │
Chromatophore Modulator (applied to all active pedals)  │
├── Rate: 0.1-10 Hz                                    │
├── Depth: 0-1 (modulation amount)                     │
├── Pattern: sine / tri / random-smooth / pulse / drift │
├── Separation: inter-pedal phase offset (0=sync, 1=free)│
│   Modulates each pedal's key parameter:              │
│   FREEZE gain, SCATTER density, TIDE time,           │
│   ABYSS decay, MIRROR blend                          │
               │                                        │
               ▼                                        │
         PEDALBOARD OUTPUT ──→ Dry/Wet Mixer ──→ MASTER OUTPUT
                                                    │
                                              Output Width
                                              Master Volume
                                              Soft Limiter
```

### Paraphonic vs. Global Processing

**Paraphonic (shared across all active notes):**
- All oscillators (divide-down bank — always running, gated by MIDI)
- Registration stop mixer
- Brightness control
- Triple ensemble chorus
- Filter (ONE shared SVF)
- Amp envelope (ONE shared ADSR)
- Analog drift (per-oscillator slow wander)

**Global (runs once):**
- Entire chromatophore pedalboard (FREEZE → SCATTER → TIDE → ABYSS → MIRROR)
- Chromatophore modulator
- Dry/wet mixer
- Output width
- Master volume + limiter

**Nothing is per-voice in the traditional sense.** The paraphonic architecture means there are no independent voices — all notes blend through shared processing. This is fundamentally different from every other XO_OX engine.

---

## Section 7: Oscillator Architecture — Divide-Down

### Historical Context

The original ARP Solina (1972) used a single high-frequency master oscillator, divided by integer ratios to produce every pitch across the keyboard. This meant:
- Unlimited polyphony at near-zero cost
- All notes phase-locked to the master clock
- Chords have a distinctive "organ-like" coherence — notes blend rather than stack

### DSP Implementation

In digital, we don't need a literal master clock. Instead:

```
For each of 128 MIDI notes:
  - Maintain a phase accumulator (0-1) at the note's frequency
  - Each registration stop applies a different waveshaping function to the phase
  - The note is gated (on/off) by MIDI note state with a short anti-click fade
```

**Waveshapes per stop (applied to raw phase):**

| Stop | Waveshape | Character | Octave |
|------|-----------|-----------|--------|
| VIOLIN | Narrow pulse (duty ~0.15) | Bright, reedy, cutting | 8' (concert pitch) |
| VIOLA | Wide pulse (duty ~0.35) | Darker, hollow, nasal | 8' |
| CELLO | Triangle with soft rounding | Warm, full, fundamental | 8' |
| BASS | Rounded sawtooth (soft edges) | Deep, supporting | 16' (octave down) |
| CONTRABASS | Near-sine (filtered triangle) | Sub-like fundamental | 32' (two octaves down) |
| HORN | Square (duty 0.5) | Hollow, brassy, organ-like | 8' |

**Anti-aliasing:** Each waveshape uses PolyBLEP correction at transitions (pulse/square edges) to prevent aliasing at high frequencies. Triangle/sine shapes don't need correction.

**Note gating:** When a MIDI note-on arrives, the note's gate opens with a 2ms linear ramp (anti-click). On note-off, the gate closes with a 5ms ramp. The phase accumulator runs continuously — only the gate opens and closes.

**Stereo separation:** Each stop can be panned slightly across the stereo field via `oceanic_separation`. At 0, all stops are centered. At 1, stops are spread: Violin hard left, Horn hard right, others distributed between.

---

## Section 8: Triple Ensemble Chorus

### The Solina's Secret

The Solina's ensemble effect used 3 analog BBD (bucket-brigade delay) lines with asymmetric LFO modulation. The 3-phase relationship creates a shimmering, living quality that standard 2-voice chorus cannot match.

### DSP Implementation

3 modulated delay lines in parallel, mixed with the dry signal:

```
Input ──┬──→ BBD Line 1 (delay: 5-8ms, LFO: sine @ base_rate × 0.63)
        ├──→ BBD Line 2 (delay: 6-9ms, LFO: sine @ base_rate × 0.95)
        ├──→ BBD Line 3 (delay: 7-10ms, LFO: sine @ base_rate × 1.40)
        │
        └──→ Dry signal
                │
                ▼
Output = dry × (1 - depth) + (line1 + line2 + line3) × depth / 3
```

**Parameters:**
- `oceanic_ensemble` (0-1): wet/dry depth. 0 = pure dry organ. 1 = full Solina shimmer.
- `oceanic_ensRate` (0.1-3.0 Hz): base LFO rate. Scales all 3 lines proportionally. Default: 1.0 (gives 0.63, 0.95, 1.40 Hz).

**BBD simulation detail:**
- Each delay line uses linear interpolation for smooth modulation
- Delay range per line: `base_delay ± depth × 3ms`
- The 3 LFO rates are irrational ratios of each other (0.63, 0.95, 1.40) to prevent phase-locking — this is what creates the constantly-moving shimmer
- Optional: slight high-frequency loss per delay line to simulate BBD bucket leakage (subtle LP at ~12kHz)

---

## Section 9: Paraphonic Filter

### Design

One Cytomic SVF filter shared across all sounding notes. The filter processes the summed output of all registration stops after ensemble chorus.

**Modes:**
| Mode | Type | Character |
|------|------|-----------|
| WARM | 2-pole lowpass | Classic Solina warmth, rolling off highs |
| NASAL | 2-pole bandpass | Hollow, vocal string character |
| THIN | 2-pole highpass | Ethereal, ghostly upper harmonics only |

**Parameters:**
- `oceanic_filterCutoff`: 20-20000 Hz (default: 8000 Hz — fairly open)
- `oceanic_filterReso`: 0-1 (Q range: 0.5 to 8.0, self-oscillation at max)
- `oceanic_filterType`: 0 = WARM, 1 = NASAL, 2 = THIN

**Envelope modulation:** The shared ADSR envelope modulates filter cutoff with bipolar depth:
- `oceanic_filterEnvDepth`: -1 to +1 (negative = filter closes on attack)

---

## Section 10: Paraphonic Amp Envelope

### Design

One ADSR envelope shared across all notes.

**Behavior:**
- **Note-on (any key):** Envelope retriggers from current position (not from zero — prevents clicks on legato playing)
- **Note-off (last key):** Envelope enters release stage
- **New note while sustaining:** Envelope holds at sustain level (no retrigger when adding notes to a chord)
- **Retrigger on new note while released:** Envelope restarts attack from current amplitude (smooth restart)

**Parameters:**
- `oceanic_attack`: 0.001-4s (default: 0.05s — Solina was fairly instant)
- `oceanic_decay`: 0.05-4s (default: 0.3s)
- `oceanic_sustain`: 0-1 (default: 0.8 — Solina sustains well)
- `oceanic_release`: 0.05-8s (default: 0.5s — smooth fade)

---

## Section 11: Chromatophore Pedalboard — Module Details

### 11.1 FREEZE — Spectral Buffer Hold

**Concept:** Capture a moment of the string signal and sustain it indefinitely. Not a simple loop — a crossfaded buffer hold that sustains the *timbral character* at the freeze point.

**Implementation:**
- 2-second ring buffer continuously recording input
- On freeze trigger: mark the current write position, loop a 50-200ms region around it
- Crossfade (10ms Hann window) at loop boundaries for seamless sustain
- On unfreeze: crossfade back to live input over 20ms

**Parameters:**
| ID | Range | Default | Description |
|----|-------|---------|-------------|
| `oceanic_freezeMix` | 0-1 | 0 | Wet/dry. 0=live, 1=fully frozen |
| `oceanic_freezeGain` | 0-1 | 0.8 | Frozen signal level |

**CPU:** Negligible — ring buffer write + crossfaded read.

### 11.2 SCATTER — Granular Micro-Loop

**Concept:** Break the input into tiny grains and scatter them across pitch, time, and stereo space. Like the string sound shattering into particles.

**Implementation:**
- Input feeds a 1-second grain source buffer (ring buffer)
- Grain scheduler triggers grains at `density` rate
- Each grain: read a `size` ms window from the source buffer, apply Hann envelope, optionally pitch-shift, pan randomly within `panScatter` width
- Max 24 simultaneous grains (overlap-add mixing)
- Pitch shift via playback rate change (simple, introduces time compression at extreme shifts)

**Parameters:**
| ID | Range | Default | Description |
|----|-------|---------|-------------|
| `oceanic_scatterSize` | 5-500ms | 80ms | Grain window duration |
| `oceanic_scatterDensity` | 1-80/s | 15/s | Grains per second |
| `oceanic_scatterPitch` | 0-12st | 0st | Pitch randomization range (±) |
| `oceanic_scatterPan` | 0-1 | 0.5 | Stereo scatter width |
| `oceanic_scatterMix` | 0-1 | 0 | Wet/dry |

**CPU:** Moderate — scales with density × size. 24 grains × Hann window + pitch resampling.

### 11.3 TIDE — Warped Tape Delay

**Concept:** A tape delay where the tape is warped, the heads are wobbling, and each repeat shifts pitch — like sound sinking into the deep.

**Implementation:**
- Circular delay buffer (max 2 seconds at 44100Hz = 88200 samples)
- Modulated read head: Perlin noise at ~2Hz modulates read position for wow/flutter
- Feedback loop: output → tanh saturation → lowpass filter (tape degradation) → optional pitch shift → back to buffer
- Pitch shift in feedback: per-repeat pitch change via fractional delay line read offset
- Self-oscillation above 100% feedback (tanh prevents explosion)

**Parameters:**
| ID | Range | Default | Description |
|----|-------|---------|-------------|
| `oceanic_tideTime` | 10-2000ms | 375ms | Delay time |
| `oceanic_tideFeedback` | 0-1.1 | 0.4 | Feedback (>1.0 = self-oscillation) |
| `oceanic_tideWarp` | 0-1 | 0.15 | Wow/flutter depth |
| `oceanic_tidePitch` | -12 to +12st | 0 | Per-repeat pitch shift |
| `oceanic_tideDegradation` | 0-1 | 0.3 | Tape HF loss per repeat (LP cutoff in feedback) |
| `oceanic_tideMix` | 0-1 | 0 | Wet/dry |

**CPU:** Low — one modulated delay line + feedback DSP.

### 11.4 ABYSS — Shimmer Reverb

**Concept:** A reverb where the feedback network includes a pitch shifter. Each time the signal recirculates, it shifts up an octave (or two). The result: ascending harmonic tails that reveal overtones hiding in the original string chord.

**Implementation:**
- Dattorro plate reverb (4 allpass + 2 delay lines per channel)
- Pitch shifter inserted in the feedback network (granular pitch shift, +12 or +24 semitones)
- Frequency-dependent decay: HF damping LP in feedback loop
- Infinite mode: feedback = 1.0 (signal sustains indefinitely, useful with FREEZE)

**Parameters:**
| ID | Range | Default | Description |
|----|-------|---------|-------------|
| `oceanic_abyssDecay` | 0.5-30s | 3.0s | Reverb decay time (30 = near-infinite) |
| `oceanic_abyssShift` | 0/12/24 | 12 | Shimmer pitch shift interval (semitones) |
| `oceanic_abyssDamp` | 0-1 | 0.5 | HF damping (0=bright tails, 1=dark tails) |
| `oceanic_abyssSize` | 0-1 | 0.7 | Room size (diffusion amount) |
| `oceanic_abyssMix` | 0-1 | 0 | Wet/dry |

**CPU:** Moderate — Dattorro plate is well-studied and efficient. The pitch shifter in feedback adds ~2% overhead.

### 11.5 MIRROR — Reverse Buffer

**Concept:** Record input into a buffer, play it back reversed, crossfade between forward and reversed signal. Creates pre-echoes and backwards swells — sounds arriving before they happen.

**Implementation:**
- Double-buffer system: while one buffer records, the other plays backwards
- Buffer length sets the reversal period
- Auto-crossfade (10ms Hann) at buffer swap boundaries
- Blend parameter: 0 = all forward (bypass), 1 = all reversed

**Parameters:**
| ID | Range | Default | Description |
|----|-------|---------|-------------|
| `oceanic_mirrorLength` | 50-2000ms | 500ms | Reverse buffer length |
| `oceanic_mirrorBlend` | 0-1 | 0 | Forward ↔ reversed crossfade |
| `oceanic_mirrorMix` | 0-1 | 0 | Wet/dry |

**CPU:** Low — buffer read/write + crossfade.

---

## Section 12: Chromatophore Modulator

### Concept

A global LFO that modulates key parameters across all active pedalboard modules. This is what makes the pedalboard feel *alive* — a single organism pulsing with color, not 5 static effects.

### Modulation Targets

| Pedal | Modulated Parameter | Effect |
|-------|-------------------|--------|
| FREEZE | `freezeGain` | Frozen signal fades in and out rhythmically |
| SCATTER | `scatterDensity` | Grain density pulses — sparse↔dense |
| TIDE | `tideTime` | Delay time wobbles — pitch modulation of echoes |
| ABYSS | `abyssDecay` | Reverb tail length breathes |
| MIRROR | `mirrorBlend` | Forward/reverse ratio shifts |

### Phase Separation

The `oceanic_chromSeparation` parameter offsets each pedal's modulation phase:
- At 0: all 5 pedals modulate in sync (they breathe together)
- At 1: each pedal's phase is offset by 72° (360°/5) — they ripple like a wave

This creates the cuttlefish chromatophore effect — at low separation, the creature pulses as one; at high separation, colors ripple across the skin.

### Parameters

| ID | Range | Default | Description |
|----|-------|---------|-------------|
| `oceanic_chromRate` | 0.1-10 Hz | 0.8 Hz | Modulation rate |
| `oceanic_chromDepth` | 0-1 | 0.3 | Modulation depth (0=static, 1=full range) |
| `oceanic_chromPattern` | 0-4 | 0 | Shape: sine / triangle / random-smooth / pulse / drift |
| `oceanic_chromSeparation` | 0-1 | 0.3 | Phase offset between pedals |

### Implementation

```
For each audio block:
  chromatophore_phase += chromRate / sampleRate × blockSize

  for each pedal (i = 0..4):
    pedal_phase = chromatophore_phase + (i / 5.0) × chromSeparation
    mod_value = applyPattern(pedal_phase, chromPattern) × chromDepth
    pedal.modulate(mod_value)
```

Pattern shapes:
- **Sine:** smooth, organic breathing
- **Triangle:** linear ramp up/down — mechanical but warm
- **Random-smooth:** filtered white noise — unpredictable drift
- **Pulse:** on/off with smoothing — rhythmic gating effect
- **Drift:** Perlin noise — organic, non-repeating wander

---

## Section 13: Complete Parameter List

### Layer A: String Ensemble (20 parameters)

| # | ID | Range | Default | Description |
|---|-----|-------|---------|-------------|
| 1 | `oceanic_violin` | 0-1 | 0.7 | Violin stop level (8') |
| 2 | `oceanic_viola` | 0-1 | 0.5 | Viola stop level (8') |
| 3 | `oceanic_cello` | 0-1 | 0.6 | Cello stop level (8') |
| 4 | `oceanic_bass` | 0-1 | 0.4 | Bass stop level (16') |
| 5 | `oceanic_contrabass` | 0-1 | 0.2 | Contrabass stop level (32') |
| 6 | `oceanic_horn` | 0-1 | 0.0 | Horn stop level (8') |
| 7 | `oceanic_ensemble` | 0-1 | 0.6 | Ensemble chorus depth |
| 8 | `oceanic_ensRate` | 0.1-3.0 | 1.0 | Ensemble LFO rate multiplier |
| 9 | `oceanic_filterCutoff` | 20-20000 | 6000 | Paraphonic filter cutoff (Hz) |
| 10 | `oceanic_filterReso` | 0-1 | 0.15 | Filter resonance |
| 11 | `oceanic_filterType` | 0-2 | 0 | Filter: Warm LP / Nasal BP / Thin HP |
| 12 | `oceanic_filterEnvDepth` | -1 to 1 | 0.2 | Filter envelope modulation depth |
| 13 | `oceanic_attack` | 0.001-4.0 | 0.05 | Amp envelope attack (s) |
| 14 | `oceanic_decay` | 0.05-4.0 | 0.3 | Amp envelope decay (s) |
| 15 | `oceanic_sustain` | 0-1 | 0.8 | Amp envelope sustain level |
| 16 | `oceanic_release` | 0.05-8.0 | 0.5 | Amp envelope release (s) |
| 17 | `oceanic_separation` | 0-1 | 0.3 | Registration stereo spread |
| 18 | `oceanic_drift` | 0-1 | 0.15 | Analog pitch drift amount |
| 19 | `oceanic_brightness` | 0-1 | 0.5 | Pre-ensemble treble shaping |
| 20 | `oceanic_tune` | -50 to 50 | 0 | Master fine tune (cents) |

### Layer B: Chromatophore Pedalboard (27 parameters)

| # | ID | Range | Default | Description |
|---|-----|-------|---------|-------------|
| 21 | `oceanic_freezeMix` | 0-1 | 0 | FREEZE wet/dry |
| 22 | `oceanic_freezeGain` | 0-1 | 0.8 | Frozen signal level |
| 23 | `oceanic_scatterSize` | 5-500 | 80 | SCATTER grain size (ms) |
| 24 | `oceanic_scatterDensity` | 1-80 | 15 | SCATTER grains per second |
| 25 | `oceanic_scatterPitch` | 0-12 | 0 | SCATTER pitch randomization (st) |
| 26 | `oceanic_scatterPan` | 0-1 | 0.5 | SCATTER stereo width |
| 27 | `oceanic_scatterMix` | 0-1 | 0 | SCATTER wet/dry |
| 28 | `oceanic_tideTime` | 10-2000 | 375 | TIDE delay time (ms) |
| 29 | `oceanic_tideFeedback` | 0-1.1 | 0.4 | TIDE feedback amount |
| 30 | `oceanic_tideWarp` | 0-1 | 0.15 | TIDE wow/flutter depth |
| 31 | `oceanic_tidePitch` | -12 to 12 | 0 | TIDE per-repeat pitch shift (st) |
| 32 | `oceanic_tideDegradation` | 0-1 | 0.3 | TIDE tape HF loss amount |
| 33 | `oceanic_tideMix` | 0-1 | 0 | TIDE wet/dry |
| 34 | `oceanic_abyssDecay` | 0.5-30 | 3.0 | ABYSS reverb decay (s) |
| 35 | `oceanic_abyssShift` | 0/12/24 | 12 | ABYSS shimmer pitch interval (st) |
| 36 | `oceanic_abyssDamp` | 0-1 | 0.5 | ABYSS HF damping |
| 37 | `oceanic_abyssSize` | 0-1 | 0.7 | ABYSS room size |
| 38 | `oceanic_abyssMix` | 0-1 | 0 | ABYSS wet/dry |
| 39 | `oceanic_mirrorLength` | 50-2000 | 500 | MIRROR buffer length (ms) |
| 40 | `oceanic_mirrorBlend` | 0-1 | 0 | MIRROR forward/reverse blend |
| 41 | `oceanic_mirrorMix` | 0-1 | 0 | MIRROR wet/dry |
| 42 | `oceanic_chromRate` | 0.1-10 | 0.8 | Chromatophore mod rate (Hz) |
| 43 | `oceanic_chromDepth` | 0-1 | 0.3 | Chromatophore mod depth |
| 44 | `oceanic_chromPattern` | 0-4 | 0 | Chromatophore mod shape |
| 45 | `oceanic_chromSeparation` | 0-1 | 0.3 | Chromatophore inter-pedal phase offset |
| 46 | `oceanic_pedalMix` | 0-1 | 0.3 | Master pedalboard wet/dry |
| 47 | `oceanic_couplingLevel` | 0-1 | 0 | External audio input level |

### Global (8 parameters)

| # | ID | Range | Default | Description |
|---|-----|-------|---------|-------------|
| 48 | `oceanic_masterVol` | 0-1 | 0.8 | Master output volume |
| 49 | `oceanic_outputWidth` | 0-1 | 0.7 | Stereo width (0=mono, 1=full) |
| 50 | `oceanic_macro1` | 0-1 | 0 | DEPTH macro |
| 51 | `oceanic_macro2` | 0-1 | 0 | CURRENT macro |
| 52 | `oceanic_macro3` | 0-1 | 0 | COUPLING macro |
| 53 | `oceanic_macro4` | 0-1 | 0 | ABYSS macro |
| 54 | `oceanic_dryLevel` | 0-1 | 0.7 | Pre-pedalboard dry signal level |
| 55 | `oceanic_octave` | -1/0/1 | 0 | Master octave shift |

**Total: 55 parameters**

---

## Section 14: Macro System

| Macro | Label | Primary Targets | Curve | Musical Intent |
|-------|-------|----------------|-------|----------------|
| M1 | DEPTH | `pedalMix` ↑, `ensemble` ↑, `chromDepth` ↑, `abyssMix` slight ↑ | ease_in_out | **The revealer.** How much hidden color do you want to see? 0=pure dry Solina warmth. 1=full chromatophore bioluminescence. |
| M2 | CURRENT | `chromRate` ↑, `chromSeparation` ↑, `tideWarp` ↑, `scatterDensity` ↑, `drift` ↑ | ease_in | **The motion.** 0=still deep water. 1=churning current. Controls speed and complexity of all organic modulation. |
| M3 | COUPLING | `couplingLevel` ↑, `freezeMix` context, `scatterMix` context | linear | **The portal.** In XOceanus: scales coupling input depth. In standalone: could control internal feedback routing. |
| M4 | ABYSS | `abyssMix` ↑, `abyssDecay` ↑, `mirrorMix` ↑, `outputWidth` ↑, `separation` ↑ | ease_out | **The space.** 0=close, intimate string ensemble. 1=infinite shimmering ocean abyss. |

### Macro Design Rules
- All 4 macros produce audible change across their full range in every preset
- Macros never invert each other (M1↑ doesn't undo M4↑)
- Macros are per-preset data (mappings stored in .xometa, not hardcoded)
- Default macro mappings above are the recommended starting point; preset designers may customize

---

## Section 15: Coupling Interface (XOceanus Integration)

### getSampleForCoupling()
Returns: post-pedalboard stereo output, cached per sample.

This is the fully processed signal — string ensemble + all active effects. Other engines receiving XOceanic's coupling output get the full "bioluminescent" character.

### applyCouplingInput() — Supported Types

| CouplingType | What XOceanic Does | Musical Effect |
|--------------|-------------------|----------------|
| `AudioToWavetable` | External audio mixed into pedalboard input (before FREEZE) at `couplingLevel` | **THE killer coupling.** Any engine's sound enters the chromatophore chain. |
| `AmpToFilter` | Source amplitude → `filterCutoff` modulation | Drum hits or bass plucks sweep the string filter |
| `EnvToMorph` | Source envelope → `ensemble` depth | External crescendos intensify the ensemble shimmer |
| `LFOToPitch` | Source LFO → master pitch drift | Cross-engine organic pitch wander |
| `FilterToFilter` | Source filter output → XOceanic filter cutoff | Filter tracking between engines |

### applyCouplingInput() — Rejected Types

| CouplingType | Why |
|--------------|-----|
| `AmpToChoke` | Choking a string ensemble kills the pad — no musical use |
| `AudioToRing` | Ring mod on strings = ugly metallic artifacts |
| `AudioToFM` | FM on divide-down oscillators produces noise, not musical results |
| `PitchToPitch` | Paraphonic — pitch is determined by note gating, not continuous pitch |

### Normalled Coupling Routes (Default Pairings)

| Partner | Route | Musical Intent |
|---------|-------|---------------|
| ODYSSEY → OCEANIC | `AudioToWavetable` @ 0.3 | Climax bloom enters the chromatophore chain — psychedelic strings |
| OVERWORLD → OCEANIC | `AudioToWavetable` @ 0.4 | Chip audio scattered through SCATTER — 8-bit string particles |
| ONSET → OCEANIC | `AmpToFilter` @ 0.25 | Drum hits breathe the string filter — rhythmic string pumping |
| OCEANIC → OVERDUB | `getSample` → send input | Processed strings through dub delay — shimmer + tape echo |
| OCEANIC → OPAL | `getSample` → grain buffer | String output granulated through OPAL's time engine |

---

## Section 16: Voice Architecture

| Field | Value |
|-------|-------|
| Architecture | **Paraphonic** — NOT polyphonic |
| Max simultaneous notes | 128 (all MIDI notes available) |
| Practical simultaneous notes | Unlimited (CPU is constant regardless of note count) |
| Voice stealing | N/A — no voices to steal |
| Legato | N/A — all notes share the same envelope |
| Portamento | Not applicable (paraphonic — notes don't glide) |
| getMaxVoices() return | 1 (for XOceanus CPU budgeting — it's one paraphonic "voice") |
| getActiveVoiceCount() return | Count of currently held MIDI notes |

### CPU Profile

| Component | Estimated CPU | Notes |
|-----------|--------------|-------|
| Oscillator bank (128 phase accumulators) | <0.5% | Trivial — just phase + waveshape lookup |
| Registration mixer (6 stops) | <0.2% | 6 multiplies per sample |
| Triple ensemble chorus (3 delay lines) | <0.5% | 3 modulated reads + interpolation |
| Paraphonic filter (1 SVF) | <0.2% | Single filter instance |
| Amp envelope | <0.1% | One ADSR |
| FREEZE | <0.1% | Ring buffer + crossfade |
| SCATTER | <1.5% | Grain scheduler, up to 24 grains |
| TIDE | <0.5% | Modulated delay + feedback DSP |
| ABYSS | <2.0% | Dattorro plate + pitch shifter |
| MIRROR | <0.3% | Double buffer + crossfade |
| Chromatophore mod | <0.1% | One LFO + 5 multiplies |
| **TOTAL** | **<6%** | Well within single-engine budget |

**Dual-engine estimate:** OCEANIC + any engine ≤ 21% (OCEANIC is extremely light)

---

## Section 17: Preset Strategy

### Categories

| Category | Count | Character | Moods |
|----------|-------|-----------|-------|
| **Pure Ensemble** | 20 | Dry Solina strings. Registration variations. Warm, simple, immediate. | Foundation, Atmosphere |
| **Bioluminescent Pads** | 25 | Strings through shimmer + freeze. Glowing, suspended. The "wow" presets. | Atmosphere, Aether |
| **Chromatophore Textures** | 20 | Full pedalboard. Scatter + Tide + Chromatophore mod. Shifting, alive. | Prism, Flux |
| **Deep Creatures** | 15 | Heavy processing. Reverse + self-osc delay. Strange, alien, beautiful. | Prism, Aether |
| **Spectral Freeze** | 15 | Freeze-focused. Captured harmonic moments sustained infinitely. | Aether |
| **Coupling Showcases** | 25 | External audio through chromatophores. Designed for specific engine pairs. | Entangled |
| **TOTAL** | **120** | | |

### Preset Naming Style
Evocative, aquatic, 2-3 words, max 30 chars. No jargon.

Examples:
- "Warm Current" (pure ensemble, gentle)
- "Phosphor Drift" (bioluminescent pad, slow shimmer)
- "Chromatophore" (full pedalboard, pulsing)
- "Hadal Zone" (deep creature, maximum processing)
- "Frozen Reef" (spectral freeze, crystalline)
- "Creature Signal" (coupling showcase, external audio)

### Hero Presets (First 10)

| # | Name | Character | Key Settings |
|---|------|-----------|-------------|
| 1 | Solina Sea | Pure vintage ensemble | Violin+Cello, high ensemble, no pedals |
| 2 | Phosphor Drift | Gentle shimmer pad | Ensemble + ABYSS low mix + slow chromatophore |
| 3 | Abyssal Choir | Huge frozen shimmer | FREEZE + ABYSS at max + chromatophore pulse |
| 4 | Tidal String | Rhythmic delay pad | Ensemble + TIDE with tempo sync + chromatophore |
| 5 | Scattered Light | Granular texture | Low ensemble + SCATTER high density + pan scatter |
| 6 | Mirror Current | Reverse swell pad | MIRROR high blend + ABYSS shimmer |
| 7 | Deep Creature | Full chromatophore | All pedals active, M1 DEPTH at 0.7 |
| 8 | Warm Contrabass | Sub-focused ensemble | Contrabass+Bass high, violin off, dark filter |
| 9 | Frozen Moment | Spectral capture | FREEZE mix 0.8 + very slow chromatophore drift |
| 10 | Bioluminescence | The showcase preset | Everything engaged, chromatophore pulsing, maximum color |

---

## Section 18: Implementation Roadmap

### Phase 0: Project Scaffold (1 hour)
- Invoke `/new-xo-project name=XOceanic identity="..." code=XOcn`
- Verify build compiles
- Set up parameter layout with all 55 IDs

**Must-Have:** Build system, parameter layout, APVTS, basic silence-outputting processor.

### Phase 1: Oscillator Bank + Registration (2-3 hours)
- 128 phase accumulators (always running)
- MIDI note gating with anti-click ramps
- 6 registration stop waveshapes (PolyBLEP for pulse/square)
- Registration mixer with level + stereo separation
- Brightness control
- Tune + octave

**Must-Have:** Play a chord, hear blended organ-like stops.

### Phase 2: Triple Ensemble Chorus (1-2 hours)
- 3 modulated delay lines
- LFO rate ratios (0.63, 0.95, 1.40 × base)
- Wet/dry control
- Rate control

**Must-Have:** The Solina transformation — organ → warm shimmering strings. This is the soul of the engine.

### Phase 3: Filter + Envelope + Drift (1-2 hours)
- Cytomic SVF (LP/BP/HP)
- Shared ADSR with retrigger behavior
- Filter envelope modulation
- Per-oscillator analog drift (slow Perlin noise pitch wander)

**Must-Have:** Filter sweeps affect all notes simultaneously. Envelope retriggers on new notes, releases on last note-off.

### Phase 4: Chromatophore Pedalboard (4-6 hours)
Build in order: FREEZE → TIDE → ABYSS → SCATTER → MIRROR

- **FREEZE:** Ring buffer, freeze/release, crossfade
- **TIDE:** Modulated delay + wow/flutter + feedback with saturation + degradation LP
- **ABYSS:** Dattorro plate reverb + pitch shifter in feedback network
- **SCATTER:** Grain scheduler, overlap-add, pitch/pan randomization
- **MIRROR:** Double buffer reverse playback + crossfade

Each module independently bypassable. Serial chain.

**Must-Have:** Each pedal transforms the string sound in its intended way. Dry/wet controls work.

### Phase 5: Chromatophore Modulator + Macros (1-2 hours)
- Chromatophore LFO (5 patterns)
- Per-pedal phase separation
- Macro routing (M1-M4)
- Master dry/wet, coupling level, output width, volume

**Must-Have:** Macros produce audible change. Chromatophore makes pedals pulse organically.

### Phase 6: Coupling + Presets + UI (3-4 hours)
- Coupling buffer (cache render output for `getSampleForCoupling`)
- `applyCouplingInput` for AudioToWavetable, AmpToFilter, EnvToMorph, LFOToPitch, FilterToFilter
- External audio mixed into pedalboard input
- 10 hero presets in `.xometa`
- Custom UI (or GenericEditor for v1)
- auval validation

**Must-Have:** Coupling works, hero presets sound compelling, plugin validates.

### Feature Priority

**Must-Have:**
- Divide-down oscillators + 6 stops
- Triple ensemble chorus
- Paraphonic filter + envelope
- All 5 pedalboard modules
- Chromatophore modulator
- Macros
- Coupling interface

**Strong Should-Have:**
- Custom UI
- 120 preset library
- All 55 parameters exposed
- Analog drift

**Nice-to-Have:**
- Tempo sync on TIDE delay time
- Tempo sync on chromatophore rate
- MIDI-triggered freeze (sustain pedal or CC)
- Alternative ensemble voicings (beyond Solina — e.g., Mellotron, Reed Organ)

**Avoid for Now:**
- Per-voice filters (would break paraphonic identity)
- Polyphonic mode toggle (would undermine the concept)
- Additional FX beyond the 5 pedalboard modules (OVERDUB handles general FX)
- Arpeggiator (not suited to string ensemble playing style)

---

## Section 19: Real-Time Safety

| Concern | Mitigation |
|---------|-----------|
| Memory allocation | All buffers (ring buffers, grain pool, delay lines, reverb) pre-allocated in `prepare()` |
| Blocking I/O | None in audio path. Preset loading on message thread only. |
| Denormals | Flush-to-zero in filter feedback paths (ensemble, TIDE feedback, ABYSS feedback) |
| NaN propagation | `juce::FloatVectorOperations::disableFlushToZero` not called. Check filter state vars per block. |
| DC offset | DC blocker after TIDE (asymmetric saturation + pitch shift can introduce DC) |
| Grain pool exhaustion | SCATTER hard-limits at 24 grains — new grain request when pool full = oldest grain released |
| Self-oscillation safety | TIDE feedback ≤ 1.1 with tanh in loop. ABYSS decay ≤ 30s. Both have soft limiters. |
| CPU spikes | No per-voice processing. All global. CPU is constant regardless of polyphony. |

---

## Section 20: Reusable DSP Modules

| Module | Also Used In | Notes |
|--------|-------------|-------|
| Cytomic SVF | OVERBITE, ODDFELIX, ONSET, ODYSSEY | Already in shared DSP library |
| PolyBLEP | OBESE, ODDFELIX, ODYSSEY | For pulse/square stop waveshapes |
| Dattorro Reverb | OBESE | ABYSS extends with pitch shifter in feedback |
| ParamSnapshot | All engines | Standard pattern |
| ADSR Envelope | All engines | Paraphonic retrigger behavior is unique |
| tanh Saturation | OBESE, OVERBITE, OVERDUB | In TIDE feedback loop |
| DC Blocker | OVERBITE, ONSET | After TIDE |
| Modulated Delay Line | OVERDUB (tape delay) | For ensemble chorus + TIDE |

**New DSP unique to XOceanic:**
- Divide-down oscillator bank (128 phase accumulators with note gating)
- Granular scatter engine (24-grain overlap-add scheduler)
- Reverse double-buffer (MIRROR)
- Shimmer pitch shifter (for ABYSS feedback network)
- Chromatophore modulator (multi-target phased LFO)

---

*XO_OX Designs | Engine: OCEANIC | Accent: #00B4A0 | Prefix: oceanic_ | 55 parameters*
