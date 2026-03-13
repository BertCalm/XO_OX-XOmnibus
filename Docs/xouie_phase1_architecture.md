# XOuïe — Phase 1 Architecture

*Phase 1 | March 2026 | XO_OX Designs*
*Synth Architect Protocol output for duophonic hammerhead engine*

---

## 1. Engine Identity

| Field | Value |
|-------|-------|
| **Name** | XOuïe |
| **Gallery Code** | OUIE |
| **Accent Color** | Hammerhead Steel `#708090` |
| **Parameter Prefix** | `ouie_` |
| **Plugin Code** | `XOui` |
| **Manufacturer Code** | `Xoox` |
| **Engine ID** | `"Ouie"` |
| **Polarity** | Dead center — 50/50 feliX-Oscar |
| **Water Column** | The Thermocline |
| **Creature** | Hammerhead shark (*Sphyrna*) |
| **Max Voices** | 2 (duophonic) |
| **CPU Budget** | <8% |

**Thesis:** Two digital oscillator voices with independently selectable algorithms, interacting through a bipolar STRIFE↔LOVE axis. The hammerhead shark's cephalofoil — two sensory platforms fused into one predator.

---

## 2. DSP Signal Flow

```
┌─── VOICE ALLOCATION (Duophonic) ───────────────────────────────────────┐
│  Mode: Split (low/high) | Layer (both per note) | Duo (round-robin)   │
│  Glide: Per-voice independent portamento                               │
└───────────────────────────────┬─────────────────────────────────────────┘
                                │
        ┌───────────────────────┴───────────────────────┐
        ▼                                               ▼
┌───────────────────────┐               ┌───────────────────────┐
│     VOICE A            │               │     VOICE B            │
│  ┌─────────────────┐  │               │  ┌─────────────────┐  │
│  │ Algorithm Select │  │               │  │ Algorithm Select │  │
│  │ (1 of 8)        │  │               │  │ (1 of 8)        │  │
│  └────────┬────────┘  │               │  └────────┬────────┘  │
│           ▼           │               │           ▼           │
│  ┌─────────────────┐  │               │  ┌─────────────────┐  │
│  │ Unison Stack    │  │               │  │ Unison Stack    │  │
│  │ (1-4 detuned)   │  │               │  │ (1-4 detuned)   │  │
│  └────────┬────────┘  │               │  └────────┬────────┘  │
│           ▼           │               │           ▼           │
│  ┌─────────────────┐  │               │  ┌─────────────────┐  │
│  │ Sub Oscillator  │  │               │  │ Sub Oscillator  │  │
│  │ (-1 oct sine)   │  │               │  │ (-1 oct sine)   │  │
│  └────────┬────────┘  │               │  └────────┬────────┘  │
│           ▼           │               │           ▼           │
│     Level + Pan       │               │     Level + Pan       │
└───────────┬───────────┘               └───────────┬───────────┘
            │                                       │
            └──────────────┬────────────────────────┘
                           ▼
              ┌────────────────────────┐
              │   INTERACTION STAGE    │
              │                        │
              │  HAMMER macro (M1)     │
              │  0.0 ── STRIFE ──┐    │
              │       Cross-FM    │    │
              │       Ring Mod    │    │
              │       Phase Kill  │    │
              │  0.5 ── INDEPENDENT    │
              │  1.0 ── LOVE ────┐    │
              │       Spectral    │    │
              │       Harm. Lock  │    │
              │       Unison+     │    │
              └──────────┬─────────────┘
                         ▼
              ┌────────────────────────┐
              │    FILTER (Cytomic)    │
              │  LP12/LP24/HP/BP/Notch │
              │  + Filter Envelope     │
              │  + Key Tracking        │
              └──────────┬─────────────┘
                         ▼
              ┌────────────────────────┐
              │    AMP ENVELOPE        │
              │    ADSR                │
              └──────────┬─────────────┘
                         ▼
              ┌────────────────────────┐
              │    CHARACTER DRIVE     │
              │    Soft / Hard / Tube  │
              └──────────┬─────────────┘
                         ▼
              ┌────────────────────────┐
              │    STEREO WIDENER      │
              │    Haas + mid-side     │
              └──────────┬─────────────┘
                         ▼
              ┌────────────────────────┐
              │    DELAY               │
              │    Stereo ping-pong    │
              └──────────┬─────────────┘
                         ▼
              ┌────────────────────────┐
              │    REVERB              │
              │    Plate / algorithmic │
              └──────────┬─────────────┘
                         ▼
                      OUTPUT
```

---

## 3. Algorithm Specifications

Each voice selects one algorithm. All algorithms share a common interface:

```cpp
struct OuieAlgorithm {
    virtual void prepare(double sampleRate) = 0;
    virtual void setFrequency(float hz) = 0;
    virtual float process() = 0;  // returns one sample
    virtual void noteOn() = 0;
    virtual void noteOff() = 0;
    // Three user-facing knobs per algorithm:
    float waveform = 0.0f;  // algorithm-specific selector
    float color    = 0.5f;  // algorithm-specific timbre
    float shape    = 0.0f;  // algorithm-specific modifier
};
```

### 3.1 Virtual Analog (Algorithm 0) — SMOOTH

Classic anti-aliased subtractive oscillator.

| Param | Controls | Range |
|-------|----------|-------|
| `waveform` | Wave shape | 0=Saw, 1=Square, 2=Triangle, 3=Sine |
| `color` | Pulse width (Square) / Brightness (others) | 0.05-0.95 |
| `shape` | Sub oscillator blend (sine -1 oct) | 0.0-1.0 |

**Implementation:** PolyBLEP anti-aliasing for saw/square. Trivially cheap. This is the "default" algorithm — if you don't know what to pick, VA works.

**CPU:** ~0.1% per voice

### 3.2 Wavetable (Algorithm 1) — SMOOTH

Morphable wavetable scanning with band-limited interpolation.

| Param | Controls | Range |
|-------|----------|-------|
| `waveform` | Table select | 0=Basic, 1=Vocal, 2=Digital, 3=Organic, 4=Metallic, 5=Harmonic, 6=Noise |
| `color` | Scan position | 0.0-1.0 (morphs through frames) |
| `shape` | Frame interpolation | 0=sharp steps, 1=smooth morph |

**Implementation:** 7 built-in wavetables, each 8 frames × 2048 samples. Band-limited via pre-computed mip-maps. Scan position interpolates between adjacent frames.

**Wavetable set:**
- **Basic:** Sine → Triangle → Saw → Square → PWM → Noise blend → Digital → Harsh
- **Vocal:** Ah → Eh → Ee → Oh → Oo → Mm → Nasal → Whisper
- **Digital:** Clean sine → Bit-crushed → Aliased → Glitch → Data → Error → Static → Crash
- **Organic:** Breath → Wood → Bell → String → Skin → Bone → Fluid → Crystal
- **Metallic:** Tube → Plate → Wire → Foil → Gong → Anvil → Blade → Mercury
- **Harmonic:** Fund → 3rd → 5th → 7th → 9th → 11th → 13th → All
- **Noise:** White → Pink → Brown → Blue → Violet → Crackle → Rain → Wind

**CPU:** ~0.15% per voice

### 3.3 FM (Algorithm 2) — SMOOTH

2-operator FM synthesis (carrier + modulator).

| Param | Controls | Range |
|-------|----------|-------|
| `waveform` | Modulator waveform | 0=Sine, 1=Triangle, 2=Saw, 3=Square |
| `color` | FM ratio (quantized) | 0.5, 1, 1.5, 2, 3, 4, 5, 7 (8 steps) |
| `shape` | FM depth (mod index) | 0.0-12.0 |

**Implementation:** Carrier is always sine. Modulator waveform is selectable. Phase modulation (not true FM — standard Yamaha convention). Ratio quantization ensures musical intervals.

**Ratio ladder:**
| Step | Ratio | Musical Interval |
|------|-------|-----------------|
| 0 | 0.5 | Sub-octave |
| 1 | 1.0 | Unison |
| 2 | 1.5 | Fifth |
| 3 | 2.0 | Octave |
| 4 | 3.0 | Octave + Fifth |
| 5 | 4.0 | Two octaves |
| 6 | 5.0 | Two oct + Major 3rd |
| 7 | 7.0 | Metallic / Bell |

**CPU:** ~0.12% per voice

### 3.4 Additive (Algorithm 3) — SMOOTH

16-partial additive synthesis with spectral shaping.

| Param | Controls | Range |
|-------|----------|-------|
| `waveform` | Partial distribution | 0=All, 1=Odd only, 2=Even only, 3=Prime |
| `color` | Spectral tilt | 0=dark (partials roll off fast), 1=bright (partials equal) |
| `shape` | Inharmonicity | 0=pure harmonic, 1=metallic stretch |

**Implementation:** 16 sine oscillators with frequency = `fundamental × (N + inharmonicity × N²)`. Level per partial = `1.0 / (N ^ (1.0 - color))`. Distribution masks select which partials sound.

**Partial distributions:**
- **All:** Partials 1-16
- **Odd:** 1, 3, 5, 7, 9, 11, 13, 15 (square-wave family, hollow)
- **Even:** 2, 4, 6, 8, 10, 12, 14, 16 (octave-rich, organ-like)
- **Prime:** 1, 2, 3, 5, 7, 11, 13 (alien, complex)

**CPU:** ~0.3% per voice (16 oscillators)

### 3.5 Phase Distortion (Algorithm 4) — ROUGH

Casio CZ-style phase shaping.

| Param | Controls | Range |
|-------|----------|-------|
| `waveform` | Distortion curve | 0=Resonant, 1=Sawtooth, 2=Pulse, 3=Double-pulse |
| `color` | PD depth | 0.0=sine, 1.0=maximum distortion |
| `shape` | Phase reset timing | 0=normal, 1=hard sync feel |

**Implementation:** Cosine oscillator with phase distortion function applied to the phase accumulator before the cosine lookup. Four transfer functions create different harmonic structures. Identical approach to XObsidian but simplified (no LUT — direct computation, since we only have 2 voices).

**CPU:** ~0.1% per voice

### 3.6 Wavefolder (Algorithm 5) — ROUGH

West Coast-style wavefolding.

| Param | Controls | Range |
|-------|----------|-------|
| `waveform` | Input wave | 0=Sine, 1=Triangle |
| `color` | Fold amount | 1.0-8.0 folds |
| `shape` | Asymmetry | 0=symmetric, 1=full asymmetric |

**Implementation:** Sine wavefold using `sin(input × folds × π)` with asymmetry offset. Same primitive as ObliqueEngine's `ObliqueWavefolder` but parameterized differently. Triangle input with high fold count creates the classic Buchla-style metallic overtones.

**CPU:** ~0.08% per voice

### 3.7 Karplus-Strong (Algorithm 6) — ROUGH

Plucked string / physical modeling.

| Param | Controls | Range |
|-------|----------|-------|
| `waveform` | Exciter type | 0=Noise burst, 1=Impulse, 2=Filtered noise |
| `color` | Damping / brightness | 0=dark (fast decay), 1=bright (long sustain) |
| `shape` | Stiffness / inharmonicity | 0=pure string, 1=metallic bar |

**Implementation:** Delay line with length = sampleRate / frequency. Feedback path: one-pole lowpass (damping) + allpass (stiffness). Exciter fills the delay line on noteOn. Denormal protection on the feedback path. Reuses the proven pattern from OddfeliX/SNAP.

**CPU:** ~0.05% per voice (just a delay line + filter)

### 3.8 Noise (Algorithm 7) — ROUGH

Filtered noise with character.

| Param | Controls | Range |
|-------|----------|-------|
| `waveform` | Noise type | 0=White, 1=Pink, 2=Brown, 3=Crackle |
| `color` | Filter frequency | 20-20000 Hz |
| `shape` | Filter resonance | 0.0-0.95 |

**Implementation:** Noise generator → state-variable filter (LP). Pink noise via Voss-McCartney algorithm. Brown via integrated white. Crackle via random impulse train with variable density. The filter is independent from the main voice filter — this is the noise shaping filter.

**CPU:** ~0.05% per voice

---

## 4. The Interaction Stage — STRIFE ↔ LOVE

**This is XOuïe's signature feature.** The HAMMER macro (M1) controls a bipolar axis that determines how the two voices interact.

### 4.1 Signal Flow

```cpp
// Per-sample interaction processing
float processInteraction(float voiceA, float voiceB, float hammer, float depth, float crossRatio) {
    if (hammer < 0.5f) {
        // STRIFE region
        float strife = 1.0f - (hammer * 2.0f);  // 1.0 at 0, 0.0 at 0.5
        return processStrife(voiceA, voiceB, strife * depth, crossRatio);
    } else if (hammer > 0.5f) {
        // LOVE region
        float love = (hammer - 0.5f) * 2.0f;  // 0.0 at 0.5, 1.0 at 1.0
        return processLove(voiceA, voiceB, love * depth);
    } else {
        // INDEPENDENT — clean parallel mix
        return voiceA + voiceB;
    }
}
```

### 4.2 STRIFE Processing (HAMMER 0.0 → 0.49)

Three simultaneous processes, scaled by `strife_amount`:

**Cross-FM:**
```
voiceA_phase += voiceB * strife * depth * crossRatio
voiceB_phase += voiceA * strife * depth * crossRatio
```
Each voice's output frequency-modulates the other's phase accumulator. The `crossModRatio` parameter scales the modulation depth asymmetrically — at ratio 2.0, Voice B modulates Voice A twice as hard as A modulates B. This creates the "brotherly strife" — they're not equal combatants.

**Ring Modulation:**
```
ring_component = voiceA * voiceB * strife * depth * 0.5
```
The product of both voices creates sum and difference frequencies — metallic, dissonant, aggressive. Mixed into the output proportionally.

**Phase Cancellation:**
```
voiceB_phase_offset += strife * π * depth
```
At full strife, Voice B's phase is inverted — creating destructive interference where harmonics cancel. This creates the hollow, fighting sound of two signals trying to cancel each other out.

**Combined STRIFE output:**
```
output = (voiceA_fm + voiceB_fm) * (1.0 - ring_amount)
       + ring_component * ring_amount
       + phase_cancel_mix
```

### 4.3 LOVE Processing (HAMMER 0.51 → 1.0)

Three simultaneous processes, scaled by `love_amount`:

**Spectral Blend:**
Weighted average of both voices' instantaneous amplitudes:
```
blend = voiceA * (1.0 - love * 0.5) + voiceB * (1.0 - love * 0.5)
      + (voiceA + voiceB) * 0.5 * love  // adds the coherent sum
```
At full love, the voices merge into a single fused timbre rather than two distinct sources.

**Harmonic Lock:**
Voice B's pitch is nudged toward the nearest harmonic of Voice A:
```
nearest_harmonic = round(voiceB_freq / voiceA_freq) * voiceA_freq
voiceB_freq = lerp(voiceB_freq, nearest_harmonic, love * depth)
```
This creates consonant intervals that "lock in" as LOVE increases — the brothers agreeing on a key.

**Unison Thicken:**
Both voices gain additional detuning:
```
extra_detune = love * 15.0  // cents
```
Applied to the unison stack, making both voices fatter and more fused as LOVE increases.

### 4.4 INDEPENDENT (HAMMER = 0.5)

Clean parallel mix with no interaction processing. Both voices sound as designed — pure duophonic with complete separation. This is the "brothers coexisting" state.

### 4.5 Crossfade Region

The transition from STRIFE to INDEPENDENT to LOVE uses smooth cosine crossfading across a ±0.05 zone around 0.5 to prevent clicks when sweeping the HAMMER macro.

---

## 5. Voice Architecture

### 5.1 Duophonic Allocation

XOuïe is **strictly duophonic** — maximum 2 note slots at any time. This is not a limitation; it is the identity.

**Voice modes:**

| Mode | Behavior | Musical Use |
|------|----------|-------------|
| **Split** | Voice A = lowest held note, Voice B = highest held note | Bass + lead, two-handed play |
| **Layer** | Both voices on every note (effectively mono with 2 algorithms) | Thick, fused timbres |
| **Duo** | Round-robin: each new note goes to next available voice | Melodic interplay, counterpoint |

**Note priority:** Last-note priority in all modes. When a third note arrives in Split/Duo mode, the oldest note is stolen with a 5ms crossfade to prevent clicks.

### 5.2 Unison Stack

Each voice has an independent unison count (1-4). With 2 voices × 4 unison = **8 oscillators maximum**. Unison detune is per-voice:

```
unison_offsets[1] = { 0 }
unison_offsets[2] = { -detune/2, +detune/2 }
unison_offsets[3] = { -detune, 0, +detune }
unison_offsets[4] = { -detune, -detune/3, +detune/3, +detune }
```

Level compensation: `1.0 / sqrt(unison_count)` to maintain consistent volume.

### 5.3 Glide

Per-voice independent portamento controlled by the CARTILAGE macro (M3).

```
glide_time = ouie_glideTime * (1.0 + cartilage_macro * 3.0)  // CARTILAGE scales glide up to 4x
target_freq = new_note_frequency
current_freq += (target_freq - current_freq) * (1.0 / (glide_time * sampleRate))
```

**Glide modes:**
- **Off:** Instant pitch change
- **Always:** Every note glides
- **Legato:** Only glide when notes overlap (standard mono synth behavior)

### 5.4 Analog Drift

Per-voice pitch drift using slow Perlin-like noise:

```
drift_offset = perlin_noise(time * 0.1) * drift_amount * 5.0  // max ±5 cents
voice_freq *= pow(2.0, drift_offset / 1200.0)
```

The two voices drift independently — at low drift values they stay mostly in tune; at high values they wander apart, creating the organic imperfection of analog hardware.

---

## 6. Filter

Single shared Cytomic SVF filter, post-interaction. The filter shapes the combined result of both voices' interaction.

### 6.1 Filter Types

| Type | Character |
|------|-----------|
| LP12 | Gentle rolloff — warm, transparent |
| LP24 | Steep rolloff — classic subtractive |
| HP | High-pass — removes bass, opens air |
| BP | Band-pass — nasal, vocal, focused |
| Notch | Band-reject — hollow, phaser-like |

### 6.2 Filter Modulation

```
effective_cutoff = ouie_filterCutoff
                 + filterEnv.value * ouie_filterEnvAmount * 10000.0
                 + keyTrack * (noteFreq - 261.63) / 261.63 * ouie_filterKeyTrack
                 + lfo_mod + mod_matrix_sum
```

The filter is the primary tonal shaping tool after the interaction stage. With STRIFE creating harsh harmonics, the filter tames them. With LOVE creating thick blend, the filter carves space.

---

## 7. Frozen Parameter Table

**69 canonical parameters.** These IDs are frozen from day one — never rename after release.

### Voice A (14 parameters)

| # | Parameter ID | Type | Range | Default | Description |
|---|-------------|------|-------|---------|-------------|
| 1 | `ouie_voiceA_algorithm` | int | 0-7 | 0 (VA) | Algorithm select |
| 2 | `ouie_voiceA_octave` | int | -2 to +2 | 0 | Octave transpose |
| 3 | `ouie_voiceA_tune` | int | -24 to +24 | 0 | Semitone transpose |
| 4 | `ouie_voiceA_fine` | float | -100 to +100 | 0.0 | Fine tune (cents) |
| 5 | `ouie_voiceA_waveform` | int | 0-7 | 0 | Algorithm-specific wave |
| 6 | `ouie_voiceA_color` | float | 0.0-1.0 | 0.5 | Algorithm-specific timbre |
| 7 | `ouie_voiceA_shape` | float | 0.0-1.0 | 0.0 | Algorithm-specific mod |
| 8 | `ouie_voiceA_sub` | float | 0.0-1.0 | 0.0 | Sub osc blend |
| 9 | `ouie_voiceA_level` | float | 0.0-1.0 | 1.0 | Voice level |
| 10 | `ouie_voiceA_pan` | float | -1.0 to +1.0 | -0.3 | Voice pan |
| 11 | `ouie_voiceA_unison` | int | 1-4 | 1 | Unison voice count |
| 12 | `ouie_voiceA_unisonDetune` | float | 0.0-50.0 | 10.0 | Unison detune (cents) |
| 13 | `ouie_voiceA_drift` | float | 0.0-1.0 | 0.05 | Analog pitch drift |
| 14 | `ouie_voiceA_envDepth` | float | 0.0-1.0 | 0.0 | Pitch env depth |

### Voice B (14 parameters)

| # | Parameter ID | Type | Range | Default | Description |
|---|-------------|------|-------|---------|-------------|
| 15 | `ouie_voiceB_algorithm` | int | 0-7 | 5 (WF) | Algorithm select |
| 16 | `ouie_voiceB_octave` | int | -2 to +2 | 0 | Octave transpose |
| 17 | `ouie_voiceB_tune` | int | -24 to +24 | 0 | Semitone transpose |
| 18 | `ouie_voiceB_fine` | float | -100 to +100 | 0.0 | Fine tune (cents) |
| 19 | `ouie_voiceB_waveform` | int | 0-7 | 0 | Algorithm-specific wave |
| 20 | `ouie_voiceB_color` | float | 0.0-1.0 | 0.5 | Algorithm-specific timbre |
| 21 | `ouie_voiceB_shape` | float | 0.0-1.0 | 0.0 | Algorithm-specific mod |
| 22 | `ouie_voiceB_sub` | float | 0.0-1.0 | 0.0 | Sub osc blend |
| 23 | `ouie_voiceB_level` | float | 0.0-1.0 | 1.0 | Voice level |
| 24 | `ouie_voiceB_pan` | float | -1.0 to +1.0 | +0.3 | Voice pan |
| 25 | `ouie_voiceB_unison` | int | 1-4 | 1 | Unison voice count |
| 26 | `ouie_voiceB_unisonDetune` | float | 0.0-50.0 | 10.0 | Unison detune (cents) |
| 27 | `ouie_voiceB_drift` | float | 0.0-1.0 | 0.05 | Analog pitch drift |
| 28 | `ouie_voiceB_envDepth` | float | 0.0-1.0 | 0.0 | Pitch env depth |

### Interaction Stage (4 parameters)

| # | Parameter ID | Type | Range | Default | Description |
|---|-------------|------|-------|---------|-------------|
| 29 | `ouie_hammer` | float | 0.0-1.0 | 0.5 | STRIFE↔LOVE axis (M1) |
| 30 | `ouie_interactionDepth` | float | 0.0-1.0 | 0.5 | Interaction intensity |
| 31 | `ouie_crossModRatio` | float | 0.5-8.0 | 1.0 | Cross-FM asymmetry |
| 32 | `ouie_blendMode` | int | 0-2 | 0 | Love blend type |

### Filter (5 parameters)

| # | Parameter ID | Type | Range | Default | Description |
|---|-------------|------|-------|---------|-------------|
| 33 | `ouie_filterCutoff` | float | 20.0-20000.0 | 8000.0 | Filter cutoff (Hz) |
| 34 | `ouie_filterResonance` | float | 0.0-1.0 | 0.0 | Filter resonance |
| 35 | `ouie_filterType` | int | 0-4 | 1 | LP12/LP24/HP/BP/Notch |
| 36 | `ouie_filterEnvAmount` | float | -1.0 to +1.0 | 0.3 | Filter env depth |
| 37 | `ouie_filterKeyTrack` | float | 0.0-1.0 | 0.5 | Key tracking amount |

### Amp Envelope (4 parameters)

| # | Parameter ID | Type | Range | Default | Description |
|---|-------------|------|-------|---------|-------------|
| 38 | `ouie_ampAttack` | float | 0.001-10.0 | 0.005 | Attack time (s) |
| 39 | `ouie_ampDecay` | float | 0.001-10.0 | 0.3 | Decay time (s) |
| 40 | `ouie_ampSustain` | float | 0.0-1.0 | 0.8 | Sustain level |
| 41 | `ouie_ampRelease` | float | 0.001-30.0 | 0.5 | Release time (s) |

### Filter Envelope (4 parameters)

| # | Parameter ID | Type | Range | Default | Description |
|---|-------------|------|-------|---------|-------------|
| 42 | `ouie_filterAttack` | float | 0.001-10.0 | 0.01 | Attack time (s) |
| 43 | `ouie_filterDecay` | float | 0.001-10.0 | 0.5 | Decay time (s) |
| 44 | `ouie_filterSustain` | float | 0.0-1.0 | 0.4 | Sustain level |
| 45 | `ouie_filterRelease` | float | 0.001-30.0 | 0.5 | Release time (s) |

### Mod Envelope (4 parameters)

| # | Parameter ID | Type | Range | Default | Description |
|---|-------------|------|-------|---------|-------------|
| 46 | `ouie_modAttack` | float | 0.001-10.0 | 0.01 | Attack time (s) |
| 47 | `ouie_modDecay` | float | 0.001-10.0 | 0.3 | Decay time (s) |
| 48 | `ouie_modSustain` | float | 0.0-1.0 | 0.5 | Sustain level |
| 49 | `ouie_modRelease` | float | 0.001-30.0 | 0.3 | Release time (s) |

### LFO 1 (4 parameters)

| # | Parameter ID | Type | Range | Default | Description |
|---|-------------|------|-------|---------|-------------|
| 50 | `ouie_lfo1Rate` | float | 0.01-50.0 | 1.0 | LFO1 rate (Hz) |
| 51 | `ouie_lfo1Depth` | float | 0.0-1.0 | 0.0 | LFO1 depth |
| 52 | `ouie_lfo1Shape` | int | 0-5 | 0 | Sine/Tri/Saw/Sq/S&H/Rand |
| 53 | `ouie_lfo1Sync` | int | 0-1 | 0 | Free / Tempo |

### LFO 2 (4 parameters)

| # | Parameter ID | Type | Range | Default | Description |
|---|-------------|------|-------|---------|-------------|
| 54 | `ouie_lfo2Rate` | float | 0.01-50.0 | 3.0 | LFO2 rate (Hz) |
| 55 | `ouie_lfo2Depth` | float | 0.0-1.0 | 0.0 | LFO2 depth |
| 56 | `ouie_lfo2Shape` | int | 0-5 | 0 | Sine/Tri/Saw/Sq/S&H/Rand |
| 57 | `ouie_lfo2Sync` | int | 0-1 | 0 | Free / Tempo |

### FX (8 parameters)

| # | Parameter ID | Type | Range | Default | Description |
|---|-------------|------|-------|---------|-------------|
| 58 | `ouie_driveAmount` | float | 0.0-1.0 | 0.0 | Drive intensity |
| 59 | `ouie_driveType` | int | 0-2 | 0 | Soft/Hard/Tube |
| 60 | `ouie_stereoWidth` | float | 0.0-1.0 | 0.3 | Stereo widener |
| 61 | `ouie_delayTime` | float | 0.01-2.0 | 0.375 | Delay time (s) |
| 62 | `ouie_delayFeedback` | float | 0.0-0.95 | 0.3 | Delay feedback |
| 63 | `ouie_delayMix` | float | 0.0-1.0 | 0.15 | Delay wet/dry |
| 64 | `ouie_reverbSize` | float | 0.0-1.0 | 0.3 | Reverb size |
| 65 | `ouie_reverbMix` | float | 0.0-1.0 | 0.15 | Reverb wet/dry |

### Voice Control (3 parameters)

| # | Parameter ID | Type | Range | Default | Description |
|---|-------------|------|-------|---------|-------------|
| 66 | `ouie_voiceMode` | int | 0-2 | 2 | Split/Layer/Duo |
| 67 | `ouie_glideTime` | float | 0.0-10.0 | 0.0 | Portamento time (s) |
| 68 | `ouie_glideMode` | int | 0-2 | 0 | Off/Always/Legato |

### Master (1 parameter)

| # | Parameter ID | Type | Range | Default | Description |
|---|-------------|------|-------|---------|-------------|
| 69 | `ouie_masterLevel` | float | 0.0-1.0 | 0.8 | Master output level |

---

## 8. Mod Matrix (4 slots)

Lightweight modulation routing — 4 slots with source/destination/amount.

### Parameters (12 total)

| # | Parameter ID | Type | Range | Default |
|---|-------------|------|-------|---------|
| 70 | `ouie_mod1Source` | int | 0-9 | 0 |
| 71 | `ouie_mod1Dest` | int | 0-13 | 0 |
| 72 | `ouie_mod1Amount` | float | -1.0 to +1.0 | 0.0 |
| 73 | `ouie_mod2Source` | int | 0-9 | 0 |
| 74 | `ouie_mod2Dest` | int | 0-13 | 0 |
| 75 | `ouie_mod2Amount` | float | -1.0 to +1.0 | 0.0 |
| 76 | `ouie_mod3Source` | int | 0-9 | 0 |
| 77 | `ouie_mod3Dest` | int | 0-13 | 0 |
| 78 | `ouie_mod3Amount` | float | -1.0 to +1.0 | 0.0 |
| 79 | `ouie_mod4Source` | int | 0-9 | 0 |
| 80 | `ouie_mod4Dest` | int | 0-13 | 0 |
| 81 | `ouie_mod4Amount` | float | -1.0 to +1.0 | 0.0 |

### Mod Sources (10)

| Index | Source | Signal |
|-------|--------|--------|
| 0 | None | 0.0 |
| 1 | Amp Envelope | 0.0-1.0 |
| 2 | Filter Envelope | 0.0-1.0 |
| 3 | Mod Envelope | 0.0-1.0 |
| 4 | LFO 1 | -1.0 to +1.0 |
| 5 | LFO 2 | -1.0 to +1.0 |
| 6 | Velocity | 0.0-1.0 |
| 7 | Key Follow | -1.0 to +1.0 (C3 = 0) |
| 8 | Aftertouch | 0.0-1.0 |
| 9 | Mod Wheel | 0.0-1.0 |

### Mod Destinations (14)

| Index | Destination | Additive Range |
|-------|------------|----------------|
| 0 | None | — |
| 1 | Filter Cutoff | ±10000 Hz |
| 2 | Filter Resonance | ±1.0 |
| 3 | Voice A Color | ±1.0 |
| 4 | Voice A Shape | ±1.0 |
| 5 | Voice B Color | ±1.0 |
| 6 | Voice B Shape | ±1.0 |
| 7 | HAMMER | ±1.0 |
| 8 | Interaction Depth | ±1.0 |
| 9 | Cross-Mod Ratio | ±4.0 |
| 10 | Voice A Level | ±1.0 |
| 11 | Voice B Level | ±1.0 |
| 12 | LFO 1 Rate | ±25 Hz |
| 13 | Drive Amount | ±1.0 |

**Total parameters: 69 core + 12 mod matrix = 81**

---

## 9. Macro Mapping

| Macro | Name | Aquatic Meaning | Parameter Targets |
|-------|------|----------------|-------------------|
| **M1** | **HAMMER** | The cephalofoil — two eyes seeing one world or two | `ouie_hammer` (direct 1:1 mapping) |
| **M2** | **AMPULLAE** | Electroreceptors — sensing the faintest signals | `ouie_lfo1Depth` +0.8, `ouie_lfo2Depth` +0.6, `ouie_filterEnvAmount` +0.5, velocity sensitivity scale |
| **M3** | **CARTILAGE** | Flexible skeleton — bending without breaking | `ouie_glideTime` +5.0s, `ouie_voiceA_drift` +0.5, `ouie_voiceB_drift` +0.5, `ouie_filterAttack` +2.0s |
| **M4** | **CURRENT** | The thermocline current — the water around the shark | `ouie_reverbMix` +0.6, `ouie_delayMix` +0.4, `ouie_stereoWidth` +0.5, `ouie_filterCutoff` -3000 Hz |

### Macro Design Rules
- **M1 (HAMMER) is always audible** — it directly controls the interaction axis
- **M2 (AMPULLAE) at 0 = dead** — no modulation, flat response. At 1 = hypersensitive
- **M3 (CARTILAGE) at 0 = rigid** — instant pitch, no drift. At 1 = elastic, bending, sliding
- **M4 (CURRENT) at 0 = dry** — no environment. At 1 = deep in the thermocline

---

## 10. FX Chain

Post-voice processing. All FX are shared (not per-voice — with only 2 voices this is optimal).

### 10.1 Character Drive

Three saturation modes:

| Mode | Algorithm | Character |
|------|-----------|-----------|
| Soft | `tanh(x * gain)` | Warm, gentle compression |
| Hard | `clip(x * gain, -1, 1)` | Aggressive, flat-top clipping |
| Tube | `x / (1 + abs(x * gain))` | Asymmetric, even-harmonic bloom |

Gain derived from `ouie_driveAmount`: `gain = 1.0 + driveAmount * 20.0`

### 10.2 Stereo Widener

Mid-side processing + Haas delay:

```
mid   = (L + R) * 0.5
side  = (L - R) * 0.5
side *= (1.0 + width * 2.0)
haas_delay = width * 0.0003 * sampleRate  // 0-0.3ms
L = mid + side
R = delay(mid - side, haas_delay)
```

### 10.3 Delay

Stereo ping-pong with feedback:

```
delayL = delay_buffer_L[read_pos]
delayR = delay_buffer_R[read_pos]
delay_buffer_L[write_pos] = inputL + delayR * feedback  // cross-feed = ping-pong
delay_buffer_R[write_pos] = inputR + delayL * feedback
output = input * (1 - mix) + delay * mix
```

Max delay buffer: 2 seconds × sampleRate × 2 channels. Pre-allocated in `prepare()`.

### 10.4 Reverb

Algorithmic plate reverb using 8 allpass filters + 4 comb filters:

```
for each allpass: y = -g*x + z; z = x + g*y  (delay lengths: prime numbers)
for each comb:    y = x + feedback * delay_line[read_pos]
output = sum(combs) / 4 * size_scale
```

Delay lengths chosen as primes to minimize metallic resonances: 1051, 1213, 1399, 1571, 1709, 1867, 2053, 2221.

---

## 11. Coupling Interface

### 11.1 SynthEngine Implementation

```cpp
class OuieEngine : public xomnibus::SynthEngine {
public:
    juce::String getEngineId() const override { return "Ouie"; }
    juce::Colour getAccentColour() const override { return juce::Colour(0x70, 0x80, 0x90); }
    int getMaxVoices() const override { return 2; }

    float getSampleForCoupling() const override {
        // Return the post-interaction, pre-filter mono mix
        return lastInteractionOutput;
    }

    void applyCouplingInput(CouplingType type, float value) override {
        switch (type) {
            case CouplingType::AmpToFilter:
                couplingFilterMod += value;
                break;
            case CouplingType::EnvToMorph:
                couplingHammerMod += value;  // External sweep of HAMMER
                break;
            case CouplingType::AudioToFM:
                couplingFMAudio = value;  // FM-modulate Voice B
                break;
            case CouplingType::AmpToPitch:
                couplingPitchMod += value;
                break;
            case CouplingType::AudioToRing:
                couplingRingAudio = value;  // Ring-mod both voices
                break;
            default: break;
        }
    }
};
```

### 11.2 Supported Coupling Types

**As Target (receives):**

| CouplingType | Effect | Use Case |
|-------------|--------|----------|
| `AmpToFilter` | Drives filter cutoff | Rhythmic filter pumping from ONSET/BITE |
| `EnvToMorph` | Sweeps HAMMER macro | DRIFT's Climax destabilizes the brothers |
| `AudioToFM` | FM-modulates Voice B | OPAL's grains parasitically modulate |
| `AmpToPitch` | Pitch modulation | Vibrato/pitch effects from external |
| `AudioToRing` | Ring-mods both voices | Metallic coupling from aggressive engines |

**As Source (provides):**

`getSampleForCoupling()` returns the post-interaction, pre-filter mono signal. This is intentional — the coupling output includes the STRIFE/LOVE processing but not the filter shaping, so receiving engines get the raw duophonic interaction.

---

## 12. Hero Presets (8)

Presets are `.xometa` JSON with 6D Sonic DNA.

| # | Name | Voice A | Voice B | HAMMER | Category | DNA |
|---|------|---------|---------|--------|----------|-----|
| 1 | **Cephalofoil** | VA (Saw) | Wavefolder (Sine, 3 folds) | 0.5 (Independent) | Thermocline | B:0.6 W:0.4 M:0.3 D:0.5 S:0.4 A:0.3 |
| 2 | **Apex** | FM (Sine mod, ratio 3) | Phase Dist (Resonant, high depth) | 0.1 (Heavy Strife) | Predator | B:0.8 W:0.2 M:0.6 D:0.7 S:0.3 A:0.9 |
| 3 | **Schooling** | Wavetable (Harmonic) | Wavetable (Organic) | 0.95 (Deep Love) | Brotherly Love | B:0.5 W:0.7 M:0.4 D:0.8 S:0.6 A:0.1 |
| 4 | **Electroreception** | KS (Noise burst) | Noise (Crackle) | 0.5 (Independent) | Deep Scan | B:0.3 W:0.3 M:0.7 D:0.4 S:0.5 A:0.2 |
| 5 | **Cartilage** | Additive (All, low tilt) | FM (Tri mod, ratio 1.5) | 0.7 (Gentle Love) | Brotherly Love | B:0.4 W:0.6 M:0.8 D:0.5 S:0.4 A:0.1 |
| 6 | **Thermocline Drift** | VA (Square, PWM) | Additive (Odd, bright) | LFO→HAMMER | Thermocline | B:0.5 W:0.5 M:0.9 D:0.6 S:0.5 A:0.4 |
| 7 | **Gill Breath** | Noise (Pink, low filter) | Wavefolder (Tri, 6 folds) | 0.3 (Light Strife) | Deep Scan | B:0.2 W:0.4 M:0.5 D:0.3 S:0.7 A:0.3 |
| 8 | **Brothers** | VA (Saw, clean) | Phase Dist (Pulse, medium) | 0.3 (Tension) | Brotherly Strife | B:0.7 W:0.3 M:0.4 D:0.6 S:0.3 A:0.6 |

### Preset DNA Averages (Target)

| Category | B | W | M | D | S | A |
|----------|---|---|---|---|---|---|
| Brotherly Love | 0.4-0.6 | 0.5-0.8 | 0.3-0.6 | 0.5-0.9 | 0.4-0.7 | 0.0-0.2 |
| Brotherly Strife | 0.6-0.9 | 0.1-0.4 | 0.4-0.7 | 0.5-0.8 | 0.2-0.4 | 0.5-0.9 |
| Thermocline | 0.4-0.6 | 0.4-0.6 | 0.5-0.9 | 0.4-0.7 | 0.4-0.6 | 0.2-0.5 |
| Predator | 0.6-0.9 | 0.1-0.3 | 0.5-0.8 | 0.5-0.8 | 0.2-0.4 | 0.7-1.0 |
| Deep Scan | 0.2-0.4 | 0.3-0.5 | 0.5-0.8 | 0.3-0.5 | 0.5-0.8 | 0.1-0.3 |

---

## 13. CPU Budget Analysis

| Component | Per-Voice Cost | Total (2 voices) |
|-----------|---------------|-------------------|
| VA oscillator | 0.1% | 0.2% |
| Wavetable oscillator | 0.15% | 0.3% |
| FM oscillator | 0.12% | 0.24% |
| Additive (16 partials) | 0.3% | 0.6% |
| Phase Distortion | 0.1% | 0.2% |
| Wavefolder | 0.08% | 0.16% |
| Karplus-Strong | 0.05% | 0.1% |
| Noise | 0.05% | 0.1% |
| Unison (×4 max) | ×4 above | ×4 above |
| Interaction stage | — | 0.3% |
| Filter (Cytomic SVF) | — | 0.2% |
| 3× Envelopes | — | 0.05% |
| 2× LFOs | — | 0.02% |
| Mod matrix | — | 0.01% |
| FX chain | — | 1.5% |
| **Worst case** (Additive ×4 unison both voices) | — | **~4.5%** |
| **Typical** (VA + WF, unison 2) | — | **~2.5%** |

**Verdict:** Well under the 8% budget in all configurations. XOuïe is the lightest engine in the gallery. Duophony is the reason — 2 voices versus 8-16 elsewhere.

---

## 14. Feature Prioritization

### Must-Have (Phase 2)
- 8 algorithm implementations with shared interface
- Interaction stage (STRIFE↔LOVE) with all 6 processing modes
- Duophonic voice allocation (Split/Layer/Duo)
- Shared Cytomic SVF filter
- 3 ADSR envelopes + 2 LFOs
- 4 macros (HAMMER, AMPULLAE, CARTILAGE, CURRENT)
- FX chain (Drive, Width, Delay, Reverb)
- Glide with 3 modes
- ParamSnapshot pattern
- 8 hero presets in .xometa format

### Strong Should-Have (Phase 3)
- 4-slot mod matrix
- Analog drift per-voice
- Sub oscillator per-voice
- Unison stack (1-4) per-voice
- 72 additional factory presets (80 total)
- XOmnibus SynthEngine adapter
- Coupling interface (5 receive types, 1 send)

### Nice-to-Have (Phase 4+)
- Built-in wavetable import (load custom .wav)
- Per-voice filter (instead of shared)
- Microtuning support
- Additional coupling presets (cross-engine showcases)

### Avoid
- More than 2 voices — the constraint IS the identity
- MPE support — duophonic doesn't benefit from per-note expression beyond velocity
- Complex routing/signal splitting — keep the signal flow linear
- More than 8 algorithms — diminishing returns, each algorithm must be musically distinct

---

## 15. Build Sequence

### Phase 2A: Core Voice Engine
- [ ] `OuieVoice.h` — duophonic voice with allocation modes
- [ ] `OuieAlgorithm.h` — algorithm interface + VA implementation
- [ ] `OuieFilter.h` — Cytomic SVF (LP12/LP24/HP/BP/Notch)
- [ ] `OuieEnvelope.h` — ADSR (reuse from existing engines)
- [ ] `OuieProcessor.h` — main audio processor with ParamSnapshot
- [ ] `OuieParameters.h` — parameter layout (81 params)
- [ ] Build + verify basic sound output

### Phase 2B: Algorithm Pool
- [ ] `OuieWT.h` — Wavetable with 7 built-in tables
- [ ] `OuieFM.h` — 2-op FM with ratio ladder
- [ ] `OuieAdditive.h` — 16-partial additive
- [ ] `OuiePD.h` — Phase distortion (4 curves)
- [ ] `OuieWavefolder.h` — West Coast wavefold
- [ ] `OuieKS.h` — Karplus-Strong
- [ ] `OuieNoise.h` — Filtered noise (4 types)
- [ ] Verify all 8 algorithms produce correct output

### Phase 2C: Interaction Stage
- [ ] `OuieInteraction.h` — STRIFE (Cross-FM + Ring Mod + Phase Kill)
- [ ] `OuieInteraction.h` — LOVE (Spectral Blend + Harmonic Lock + Unison+)
- [ ] HAMMER macro wiring
- [ ] Crossfade smoothing at 0.5 boundary
- [ ] Verify HAMMER sweep from 0→1 sounds continuous

### Phase 2D: Modulation
- [ ] LFO ×2 (6 shapes each)
- [ ] Mod matrix (4 slots)
- [ ] Macro wiring (AMPULLAE, CARTILAGE, CURRENT)
- [ ] Glide (Off/Always/Legato)
- [ ] Analog drift

### Phase 2E: FX Chain
- [ ] `OuieDrive.h` — 3-mode saturation
- [ ] `OuieStereo.h` — Mid-side + Haas widener
- [ ] `OuieDelay.h` — Stereo ping-pong
- [ ] `OuieReverb.h` — Algorithmic plate

### Phase 3: Presets + Polish
- [ ] 8 hero presets (define in Phase 2A, refine here)
- [ ] 72 factory presets across 6 categories
- [ ] CPU profiling pass
- [ ] Denormal protection audit
- [ ] auval validation

### Phase 4: UI
- [ ] Cephalofoil dual-voice layout
- [ ] Interaction visualizer (waveform collision display)
- [ ] Hammerhead Steel `#708090` theme
- [ ] Algorithm selector per voice (8 options with smooth/rough grouping)
- [ ] HAMMER macro as central feature knob

### Phase 5: XOmnibus Integration
- [ ] `OuieAdapter.h` implementing `SynthEngine`
- [ ] Coupling interface wiring
- [ ] Cross-engine preset showcases
- [ ] Gallery install (register, copy presets, update docs)

---

## 16. File Structure (Standalone)

```
~/Documents/GitHub/XOuie/
├── CMakeLists.txt
├── CLAUDE.md
├── src/
│   ├── dsp/
│   │   ├── OuieAlgorithm.h       // Algorithm interface + VA
│   │   ├── OuieWT.h              // Wavetable algorithm
│   │   ├── OuieFM.h              // FM algorithm
│   │   ├── OuieAdditive.h        // Additive algorithm
│   │   ├── OuiePD.h              // Phase distortion algorithm
│   │   ├── OuieWavefolder.h      // Wavefolder algorithm
│   │   ├── OuieKS.h              // Karplus-Strong algorithm
│   │   ├── OuieNoise.h           // Noise algorithm
│   │   ├── OuieInteraction.h     // STRIFE↔LOVE interaction stage
│   │   ├── OuieFilter.h          // Cytomic SVF
│   │   ├── OuieEnvelope.h        // ADSR envelope
│   │   ├── OuieLFO.h             // LFO (6 shapes)
│   │   ├── OuieDrive.h           // Character saturation
│   │   ├── OuieStereo.h          // Mid-side widener
│   │   ├── OuieDelay.h           // Ping-pong delay
│   │   └── OuieReverb.h          // Plate reverb
│   ├── engine/
│   │   ├── OuieVoice.h           // Duophonic voice + allocation
│   │   ├── OuieProcessor.h       // Main processor + ParamSnapshot
│   │   ├── OuieParameters.h      // Parameter layout (81 params)
│   │   └── OuieModMatrix.h       // 4-slot mod routing
│   ├── ui/
│   │   ├── OuieEditor.h          // Main editor
│   │   ├── OuieLookAndFeel.h     // Hammerhead Steel theme
│   │   └── InteractionVisualizer.h // STRIFE↔LOVE display
│   ├── preset/
│   │   └── OuiePresetManager.h   // .xometa loading/saving
│   └── adapter/
│       └── OuieAdapter.h         // XOmnibus SynthEngine adapter
├── Presets/
│   └── Factory/
│       ├── Brotherly Love/       // 15 presets
│       ├── Brotherly Strife/     // 15 presets
│       ├── Thermocline/          // 12 presets
│       ├── Predator/             // 12 presets
│       ├── Deep Scan/            // 10 presets
│       └── Coupling/             // 8 presets (+ 8 hero distributed above)
└── docs/
    ├── xouie_phase1_architecture.md  // This document
    └── xomnibus_integration_spec.md  // Phase 3
```

---

## 17. Originality Audit

| Check | Status | Notes |
|-------|--------|-------|
| Not a clone of any existing engine? | ✅ | Only duophonic. Only multi-algorithm. Only bipolar interaction axis. |
| Fills a genuine gallery gap? | ✅ | No duophonic, no multi-algorithm, no neutral-toned accent, no dead-center polarity |
| Signature feature is unique? | ✅ | STRIFE↔LOVE has no equivalent in the gallery or in the MiniFreak |
| Character is distinct from inspiration? | ✅ | MiniFreak doesn't have bipolar interaction or aquatic identity |
| Avoids feature bloat? | ✅ | 2 voices, 69+12 params, <8% CPU — deliberately focused |
| Every macro produces audible change? | ✅ | HAMMER is the engine. AMPULLAE adds life. CARTILAGE adds slide. CURRENT adds space. |

---

*XO_OX Designs | XOuïe Phase 1 Architecture | The hammerhead at the thermocline*
*Invoke Phase 2: `/new-xo-engine phase=2 name=XOuïe identity="Duophonic synthesis — two voices, two algorithms, one predator" code=XOui`*
