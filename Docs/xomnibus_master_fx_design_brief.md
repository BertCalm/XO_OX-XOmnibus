# XOlokun Master FX Tract — Design Brief

**Status:** Implemented
**Author:** Claude Code
**Date:** 2026-03-12

---

## 1. Current State

The Master FX chain (`Source/Core/MasterFXChain.h`) is a fixed 3-stage post-mix processor:

| Stage | Module | Parameters | Character |
|-------|--------|------------|-----------|
| 1. Tape Saturation | `Saturator.h` (Tape mode) | `master_satDrive` | Warmth, harmonic glue |
| 2. Space Reverb | `LushReverb.h` (Schroeder-Moorer) | `master_reverbSize`, `master_reverbMix` | Global ambience |
| 3. Bus Compressor | `Compressor.h` (soft knee, parallel) | `master_compRatio`, `master_compAttack`, `master_compRelease`, `master_compMix` | Output glue |

**7 total parameters**, wired to APVTS. UI: 3-knob strip (SAT DRIVE, REVERB MIX, COMP GLUE) + ADV popup (4 knobs). This is functional but minimal — it covers "warm and glued" but can't produce the spatial, rhythmic, or textural effects that define modern sound design.

---

## 2. Design Goals

Expand the Master FX tract from a 3-stage utility chain into a **6-stage creative instrument** that can produce sounds impossible with the engines alone. Each new stage draws from specific hardware inspiration while fitting the XOlokun character-first philosophy.

**Guiding principles:**
- Master FX are part of the instrument sound, not a mixing board
- Dry patches must sound compelling first — FX enhance, never mask
- Every parameter must map meaningfully to at least one macro (M1-M4)
- New stages must be bypassable at zero CPU when inactive (drive/mix = 0)
- Total chain must stay under **15% CPU** on M1 at 44.1kHz stereo

---

## 3. Hardware Reference Analysis

### 3.1 Meris (Air Sprite / Delay Pro / Flavor Pro lineage)

**Key takeaways for XOlokun:**
- **Pitch-shifted reverb tails** (shimmer) via granular pitch processing in the reverb diffusion network
- **Modulated multi-tap delay** with per-tap pitch shift and filtering — not just echo, but a melodic instrument
- **Analog-modeled saturation** with "flavor" character switching (clean boost → tape → tube → transformer)
- **Internal modulation bus** where effects cross-modulate (delay time modulated by reverb envelope, etc.)
- **Micro-looping** as a reverb/delay hybrid — captures tiny grains and smears them

**What to steal:** Shimmer reverb mode, multi-character saturation switching, internal cross-modulation concept.

### 3.2 OBNE (Old Blood Noise Endeavors)

**Key takeaways for XOlokun:**
- **Signal splitting and recombination** — effects that process two paths differently and blend (Dark Star, Sunlight)
- **Textural reverb** with built-in modulation (chorus/vibrato/tremolo baked into the reverb algorithm)
- **Dwell control** — how long the input feeds the effect before it decays (not just mix, but input saturation of the effect itself)
- **Reverse and swell** algorithms — time-reversed grains mixed with forward signal

**What to steal:** Dwell as a parameter concept, textural reverb with integrated modulation, reverse grain processing.

### 3.3 Walrus Audio

**Key takeaways for XOlokun:**
- **Polyphonic octave generation** (Polychrome) — pitch detection feeding octave-shifted parallel paths
- **Analog bucket-brigade emulation** (Julia) — chorus/vibrato with BBD character (clock noise, limited bandwidth)
- **Swell/auto-volume** effects (Slö) — slow-attack ambient pad creation from any input

**What to steal:** BBD-character chorus for the modulation stage, swell/auto-volume as a dynamics option.

### 3.4 Chase Bliss

**Key takeaways for XOlokun:**
- **Per-parameter LFO/envelope/random modulation** — every knob has its own modulation source (Mood, Automatone)
- **Digital engine + analog signal path** — DSP controls analog circuits (relevant philosophy for hybrid sound)
- **Micro-looper grain capture** — Mood MKII's granular looper captures and manipulates tiny audio fragments
- **Ramping** — smooth transitions between two parameter states on a clock-synced LFO

**What to steal:** Per-parameter sequenced modulation (the Torso S-4 connection), ramping concept for macro automation.

### 3.5 Torso S-4

**Key takeaways for XOlokun:**
- **Step-sequenced modulation of effect parameters** — 16 steps, each step sets a target value for multiple parameters
- **Euclidean rhythm generation** for parameter triggers — sparse, musical rhythmic patterns
- **Per-step probability and randomization** — controlled chaos
- **Clock-synced** to host tempo with division/multiplication

**What to steal:** This is the most transformative reference. A **sequenced modulation engine** for the Master FX chain turns static effects into rhythmic, evolving textures. This is the "Torso dimension" — the Master FX become a performance instrument.

---

## 4. Proposed New Stages

The expanded chain grows from 3 → 6 stages. Three new DSP modules are proposed:

### 4.1 Stage 2 (NEW): Stereo Delay — `MasterDelay.h`

**Inspiration:** Meris Delay Pro, DubDelay.h (existing), Chase Bliss Tonal Recall

**Signal flow:**
```
Input → Pre-filter (HP/LP) → Delay Line L/R → Feedback → Saturation in feedback → Output mix
                                    ↓
                              Ping-pong crossfeed
                              Tap tempo / BPM sync
```

**Parameters (6):**

| Parameter ID | Range | Default | Description |
|-------------|-------|---------|-------------|
| `master_delayTime` | 1–2000 ms | 375 ms | Delay time (or BPM-synced division) |
| `master_delayFeedback` | 0.0–0.95 | 0.3 | Feedback amount (hard-capped at 0.95) |
| `master_delayMix` | 0.0–1.0 | 0.0 | Wet/dry (0 = bypass) |
| `master_delayPingPong` | 0.0–1.0 | 0.5 | Mono ↔ full ping-pong |
| `master_delayDamping` | 0.0–1.0 | 0.3 | HF roll-off per feedback iteration |
| `master_delaySync` | 0–8 (enum) | 0 | Off, 1/1, 1/2, 1/4, 1/8, 1/16, dotted, triplet |

**Key design decisions:**
- Feedback path includes a one-pole LP filter (damping) + subtle tape saturation (re-use Saturator in Tape mode) for warmth accumulation — each repeat gets darker and more saturated, like the Meris/dub aesthetic
- Smooth time interpolation (crossfade between old/new delay length) to avoid zipper artifacts on tempo changes
- Sync reads BPM from host transport via `juce::AudioPlayHead`
- When `master_delayMix` = 0, delay line is not written/read (zero CPU)

**CPU estimate:** ~2% at 44.1kHz (single delay line + feedback filter per channel)

### 4.2 Stage 4 (NEW): Modulation FX — `MasterModulation.h`

**Inspiration:** Walrus Julia (BBD chorus), Chase Bliss Warped Vinyl (wow/flutter), OBNE (textural modulation)

**Signal flow:**
```
Input → Modulated delay line (0.5–40ms) → Feedback → Mix with dry
              ↑
         LFO (sine/tri/S&H/drift)
         Rate + Depth control
         Mode switch: Chorus / Flanger / Ensemble / Drift
```

**Parameters (5):**

| Parameter ID | Range | Default | Description |
|-------------|-------|---------|-------------|
| `master_modRate` | 0.05–10.0 Hz | 0.8 Hz | LFO rate |
| `master_modDepth` | 0.0–1.0 | 0.0 | Modulation depth (0 = bypass) |
| `master_modMix` | 0.0–1.0 | 0.0 | Wet/dry blend |
| `master_modMode` | 0–3 (enum) | 0 | Chorus, Flanger, Ensemble, Drift |
| `master_modFeedback` | 0.0–0.85 | 0.0 | Flanger resonance / ensemble thickening |

**Mode descriptions:**
- **Chorus:** Classic 2-voice BBD-style chorus. Short delay (5–15ms), sine LFO, no feedback. Warm and wide.
- **Flanger:** Short delay (0.5–5ms), triangle LFO, feedback for comb filtering. Jet-engine sweep.
- **Ensemble:** 3-voice chorus with phase-offset LFOs (0°/120°/240°). Thick, orchestral widening. Inspired by Roland Dimension D.
- **Drift:** Random walk LFO (smoothed S&H) with slow rate. Produces analog tape wow/flutter. Inspired by Warped Vinyl and Chase Bliss.

**Key design decisions:**
- Modulated delay line uses linear interpolation (adequate for short delays at these rates)
- BBD character: optional subtle noise injection + bandwidth limiting (LP at 8kHz in feedback path)
- Ensemble mode uses 3 independent delay lines with offset LFOs — most expensive mode
- When `master_modDepth` = 0, bypass completely (zero CPU)

**CPU estimate:** ~2.5% at 44.1kHz (Ensemble mode with 3 voices)

### 4.3 Sequenced Modulation Engine — `MasterFXSequencer.h`

**Inspiration:** Torso S-4, Chase Bliss Automatone, Meris internal mod bus

This is not a new audio stage — it's a **parameter modulation layer** that can rhythmically animate any Master FX parameter. It turns the entire chain into a performance instrument.

**Architecture:**
```
Host Clock → Clock Divider → Step Counter (1–16 steps)
                                    ↓
                              Per-step: target values for up to 4 FX params
                              Interpolation: snap / glide / S&H
                              Probability per step
                                    ↓
                              Modulation output → additive offset to target params
```

**Parameters (8):**

| Parameter ID | Range | Default | Description |
|-------------|-------|---------|-------------|
| `master_seqEnabled` | 0/1 | 0 | Sequencer on/off |
| `master_seqRate` | 0–8 (enum) | 3 | Clock division (1/1 through 1/32, triplet, dotted) |
| `master_seqSteps` | 1–16 | 8 | Active step count |
| `master_seqDepth` | 0.0–1.0 | 0.5 | Overall modulation amount |
| `master_seqSmooth` | 0.0–1.0 | 0.3 | Step interpolation (0=snap, 1=full glide) |
| `master_seqTarget1` | 0–N (enum) | 0 | First target parameter |
| `master_seqTarget2` | 0–N (enum) | 0 | Second target parameter |
| `master_seqPattern` | 0–7 (enum) | 0 | Built-in patterns (see below) |

**Built-in patterns** (eliminates 16×4 per-step parameters for v1):

| ID | Name | Description |
|----|------|-------------|
| 0 | **Pulse** | Alternating high/low (classic sidechain pump) |
| 1 | **Ramp Up** | Linear ramp 0→1 over step count |
| 2 | **Ramp Down** | Linear ramp 1→0 over step count |
| 3 | **Triangle** | Up then down (sweeping filter feel) |
| 4 | **Euclidean 3** | 3 hits in N steps (Euclidean distribution) |
| 5 | **Euclidean 5** | 5 hits in N steps |
| 6 | **Random Walk** | Brownian motion (new random offset per step) |
| 7 | **Scatter** | Probability-based triggers (sparse, musical) |

**Targetable parameters** (enum list for `seqTarget1`/`seqTarget2`):

| Value | Target | Effect |
|-------|--------|--------|
| 0 | None | No modulation |
| 1 | `master_satDrive` | Rhythmic drive pulsing |
| 2 | `master_delayFeedback` | Dub-style feedback swells |
| 3 | `master_delayMix` | Delay throws on beats |
| 4 | `master_reverbMix` | Reverb swells/ducks |
| 5 | `master_modDepth` | Chorus/flanger depth cycling |
| 6 | `master_modRate` | Modulation speed sweep |
| 7 | `master_compMix` | Pump/breathe dynamics |

**Key design decisions:**
- Sequencer runs on the audio thread but updates once per block (not per sample) — minimal CPU
- Step values are computed from pattern algorithms, not stored (no 16×4 parameter explosion)
- Smooth parameter applies one-pole smoothing filter between step transitions
- Clock derived from `AudioPlayHead` PPQ position — sample-accurate sync
- When disabled (`seqEnabled` = 0), zero CPU overhead
- v2 could add user-editable per-step values via UI grid, but v1 ships with algorithmic patterns

**CPU estimate:** ~0.1% (block-rate parameter computation only, no audio processing)

---

## 5. Expanded Chain Architecture

```
Signal Flow (6 stages):

Engine Mix Bus
    ↓
┌─ Stage 1: Tape Saturation ──── [EXISTING] Saturator.h (Tape mode)
│   params: master_satDrive
│
├─ Stage 2: Stereo Delay ──────── [NEW] MasterDelay.h
│   params: master_delayTime, delayFeedback, delayMix,
│           delayPingPong, delayDamping, delaySync
│
├─ Stage 3: Space Reverb ──────── [EXISTING] LushReverb.h
│   params: master_reverbSize, master_reverbMix
│
├─ Stage 4: Modulation FX ──────── [NEW] MasterModulation.h
│   params: master_modRate, modDepth, modMix, modMode, modFeedback
│
├─ Stage 5: Bus Compressor ──── [EXISTING] Compressor.h (parallel)
│   params: master_compRatio, compAttack, compRelease, compMix
│
└─ Stage 6: Sequenced Mod ──── [NEW] MasterFXSequencer.h (non-audio)
    params: master_seqEnabled, seqRate, seqSteps, seqDepth,
            seqSmooth, seqTarget1, seqTarget2, seqPattern
    → modulates stages 1-5 parameters rhythmically
```

**Stage ordering rationale:**
1. **Saturation first** — adds harmonics that subsequent effects can work with
2. **Delay second** — processes the saturated signal; reverb washes delay tails naturally
3. **Reverb third** — catches both dry and delayed signal in its diffusion network
4. **Modulation fourth** — adds movement to the already spatialized signal (chorus on reverb tails = lush)
5. **Compressor last** — glues everything together, controls dynamic range of the full chain
6. **Sequencer is non-audio** — modulates parameters of stages 1-5

---

## 6. Performance Budget

| Stage | Module | CPU @ 44.1kHz stereo (M1) | CPU @ 96kHz | Notes |
|-------|--------|--------------------------|-------------|-------|
| 1 | Saturator | 0.5% | 1.0% | Per-sample tanh, DC block |
| 2 | MasterDelay | 2.0% | 3.5% | Delay line + feedback filter + interpolation |
| 3 | LushReverb | 3.5% | 6.0% | 8 combs + 4 allpasses, most expensive stage |
| 4 | MasterModulation | 2.5% | 4.0% | Ensemble mode (3 voices) worst case |
| 5 | Compressor | 0.5% | 1.0% | Per-sample envelope + gain |
| 6 | FXSequencer | 0.1% | 0.1% | Block-rate only, negligible |
| **Total** | | **9.1%** | **15.6%** | Under 15% target at 44.1kHz |

**Zero-cost bypass rule:** Every new stage checks its mix/depth/enable param at the top of processBlock. If the control value is ≤ 0.001f, the stage returns immediately. No delay lines are written, no LFOs advance. This means a preset using only reverb + compressor (the current default) has identical CPU to today's implementation.

---

## 7. Parameter Summary

### New parameters (19 total new, 26 total with existing 7)

| Parameter ID | Type | Range | Default |
|-------------|------|-------|---------|
| `master_delayTime` | float | 1–2000 ms | 375 |
| `master_delayFeedback` | float | 0.0–0.95 | 0.3 |
| `master_delayMix` | float | 0.0–1.0 | 0.0 |
| `master_delayPingPong` | float | 0.0–1.0 | 0.5 |
| `master_delayDamping` | float | 0.0–1.0 | 0.3 |
| `master_delaySync` | choice | Off,1/1,1/2,1/4,1/8,1/16,dotted,triplet | Off |
| `master_modRate` | float | 0.05–10.0 Hz | 0.8 |
| `master_modDepth` | float | 0.0–1.0 | 0.0 |
| `master_modMix` | float | 0.0–1.0 | 0.0 |
| `master_modMode` | choice | Chorus,Flanger,Ensemble,Drift | Chorus |
| `master_modFeedback` | float | 0.0–0.85 | 0.0 |
| `master_seqEnabled` | bool | 0/1 | 0 |
| `master_seqRate` | choice | 1/1..1/32,triplet,dotted | 1/4 |
| `master_seqSteps` | int | 1–16 | 8 |
| `master_seqDepth` | float | 0.0–1.0 | 0.5 |
| `master_seqSmooth` | float | 0.0–1.0 | 0.3 |
| `master_seqTarget1` | choice | None,SatDrive,DelayFB,...| None |
| `master_seqTarget2` | choice | None,SatDrive,DelayFB,...| None |
| `master_seqPattern` | choice | Pulse,RampUp,...,Scatter | Pulse |

### Defaults preserve backward compatibility
All new parameters default to bypass states (mix = 0, depth = 0, enabled = 0). Existing presets load identically.

---

## 8. Macro Mapping Recommendations

| Macro | Primary Target | Secondary Target | Rationale |
|-------|---------------|-----------------|-----------|
| **M4 (SPACE)** | `master_reverbMix` | `master_delayMix` | Space = ambience + echo |
| **M2 (MOVEMENT)** | `master_modDepth` | `master_seqDepth` | Movement = modulation + sequenced animation |
| **M1 (CHARACTER)** | `master_satDrive` | — | Character = harmonic density |
| **M3 (COUPLING)** | `master_compMix` | — | Coupling = glue between engines |

---

## 9. Preset Format Extension

The `.xometa` `"masterFX"` object grows to include new stages. Existing presets without new keys load defaults (backward compatible).

```json
"masterFX": {
  "satDrive": 0.15,
  "delayTime": 375,
  "delayFeedback": 0.35,
  "delayMix": 0.2,
  "delayPingPong": 0.8,
  "delayDamping": 0.4,
  "delaySync": 0,
  "reverbSize": 0.4,
  "reverbMix": 0.2,
  "modRate": 1.2,
  "modDepth": 0.3,
  "modMix": 0.25,
  "modMode": 2,
  "modFeedback": 0.1,
  "compRatio": 3.0,
  "compAttack": 15.0,
  "compRelease": 150.0,
  "compMix": 0.4,
  "seqEnabled": 1,
  "seqRate": 4,
  "seqSteps": 8,
  "seqDepth": 0.6,
  "seqSmooth": 0.4,
  "seqTarget1": 4,
  "seqTarget2": 2,
  "seqPattern": 3
}
```

---

## 10. UI Design

### 10.1 Expanded MasterFXSection Strip

The bottom strip grows from 3 knobs to a **scrollable/tabbed FX strip** with 6 sections:

```
┌─────────────────────────────────────────────────────────────────────┐
│ SAT       │ DELAY          │ REVERB     │ MOD        │ COMP   │ SEQ│
│ ○ Drive   │ ○ Time ○ FB    │ ○ Size     │ ○ Rate     │ ○ Glue │ ◉  │
│           │ ○ Mix          │ ○ Mix      │ ○ Depth    │        │ ●●●│
│   [ADV]   │   [ADV] [SYNC] │   [ADV]    │ [MODE][ADV]│  [ADV] │[ADV]│
└─────────────────────────────────────────────────────────────────────┘
                          68px height maintained
```

**Layout rules:**
- Each section shows 1-3 primary knobs + an ADV button for remaining params
- Section headers use engine accent color `#F8F6F3` (warm white) with XO Gold `#E9C46A` for active indicators
- SEQ section: small icon button (●●● step indicator) + enable toggle; ADV opens full sequencer panel
- On narrow screens: sections scroll horizontally

### 10.2 Sequencer ADV Panel

```
┌────────────────────────────────────────┐
│ SEQUENCER                    [PATTERN] │
│ ○ Rate  ○ Steps  ○ Depth  ○ Smooth    │
│                                        │
│ Target 1: [Reverb Mix    ▾]            │
│ Target 2: [Delay Feedback▾]            │
│                                        │
│ ■ □ ■ □ ■ □ □ ■  (pattern preview)    │
└────────────────────────────────────────┘
```

---

## 11. Implementation Plan

### Phase 1: MasterDelay (1 week)

1. Create `Source/DSP/Effects/MasterDelay.h`
   - Stereo delay line with interpolated read
   - Ping-pong crossfeed matrix
   - Feedback path: one-pole LP filter + Saturator (Tape mode, low drive)
   - BPM sync from AudioPlayHead
   - Crossfade on time changes (50ms)
2. Add 6 parameters to APVTS (`master_delay*`)
3. Wire into `MasterFXChain.h` as Stage 2 (between Saturator and LushReverb)
4. Add delay knobs to `MasterFXSection` UI
5. Unit test: impulse response, feedback stability, sync accuracy

### Phase 2: MasterModulation (1 week)

1. Create `Source/DSP/Effects/MasterModulation.h`
   - Modulated delay line (linear interp) × 1–3 voices
   - LFO generator (sine, tri, S&H, drift/random walk)
   - 4 modes: Chorus, Flanger, Ensemble, Drift
   - Optional BBD character (LP + noise in feedback)
2. Add 5 parameters to APVTS (`master_mod*`)
3. Wire into `MasterFXChain.h` as Stage 4 (after LushReverb, before Compressor)
4. Add modulation knobs + mode selector to `MasterFXSection` UI
5. Unit test: LFO accuracy, mode switching, denormal safety

### Phase 3: MasterFXSequencer (1 week)

1. Create `Source/Core/MasterFXSequencer.h`
   - Clock sync from AudioPlayHead PPQ
   - Pattern generators (8 algorithms)
   - Smooth/glide interpolation between steps
   - Target routing to any `master_*` parameter via atomic pointer offset
2. Add 8 parameters to APVTS (`master_seq*`)
3. Wire into `MasterFXChain.h` — runs at block start before audio stages
4. Add sequencer section to `MasterFXSection` UI + ADV panel
5. Unit test: clock accuracy, pattern generation, target routing

### Phase 4: Integration & Polish (1 week)

1. Update `PresetManager.h` to read/write new `masterFX` keys
2. Update macro mapping defaults (M4 → delay+reverb, M2 → modulation)
3. Factory preset pass: add Master FX settings to flagship presets
4. CPU profiling: verify <15% budget on M1 @ 44.1kHz
5. Accessibility: knob labels, keyboard nav, screen reader descriptions
6. DSP stability sweep: denormal checks, feedback caps, DC blocking

---

## 12. Files to Create/Modify

### New files:
| File | Purpose |
|------|---------|
| `Source/DSP/Effects/MasterDelay.h` | Stereo delay with sync + feedback saturation |
| `Source/DSP/Effects/MasterModulation.h` | Chorus/Flanger/Ensemble/Drift processor |
| `Source/Core/MasterFXSequencer.h` | Step-sequenced parameter modulation engine |

### Modified files:
| File | Changes |
|------|---------|
| `Source/Core/MasterFXChain.h` | Add stages 2, 4, 6; reorder chain; add new param pointers |
| `Source/XOlokunProcessor.h` | Include new headers |
| `Source/XOlokunProcessor.cpp` | Register 19 new APVTS parameters; wire prepare/process/reset |
| `Source/UI/XOlokunEditor.h` | Expand MasterFXSection to 6-section strip; add SequencerPanel |
| `Source/Core/PresetManager.h` | Parse/save new masterFX keys in `.xometa` |
| `Docs/xolokun_master_specification.md` | Update §5.2 Master FX Rack |

---

## 13. Risk Assessment

| Risk | Mitigation |
|------|-----------|
| CPU budget exceeded at 96kHz | Ensemble mode can fall back to 2-voice; delay can use nearest-neighbor interpolation |
| Parameter ID explosion (26 total) | All namespaced `master_*`; grouped in APVTS tree; UI uses ADV panels to hide complexity |
| Preset compatibility break | All new params default to bypass; existing `.xometa` files load identically |
| Sequencer clock drift | Derive from PPQ position (absolute), not delta time (cumulative) |
| Zipper noise on delay time change | 50ms crossfade between old and new delay length |
| Feedback instability | Hard cap all feedback params; denormal flush in every feedback path |

---

## 14. Sonic Pillars Served

| New Stage | Sonic Pillar | Example Use |
|-----------|-------------|-------------|
| Delay | **space**, movement | Dub echoes, rhythmic throws, ping-pong spatial placement |
| Modulation | **movement**, warmth | Analog drift, ensemble widening, flanger sweeps |
| Sequencer | **movement**, aggression | Pumping sidechain, filter sweeps, rhythmic FX throws |

Together with the existing stages (saturation → warmth/aggression, reverb → space, compressor → density/glue), the expanded chain covers all 6 Sonic DNA dimensions.

---

## 15. V2 Features (Implemented)

These features from the hardware research were incorporated directly into the v1 implementation:

### 15.1 Diffusion (MasterDelay)
**Source:** AIR Delay Pro — continuous control that smears delay taps into reverb territory.
**Implementation:** 4 series allpass filters (prime-length delays: 113, 167, 229, 313 samples) applied to the wet delay signal. `master_delayDiffusion` parameter [0..1] controls allpass feedback gain. At 0 = clean discrete echoes. At 1 = delay washes into reverb-like smear.

### 15.2 Autoclear (MasterDelay)
**Source:** AIR Delay Pro — delay tails clear on new note onset.
**Implementation:** `MasterDelay::triggerAutoclear()` called from note-on handler. Performs a 5ms fade-out on the most recent portion of the delay buffer, preventing buildup between phrases. Essential for synth use where massive delay settings can pile up.

### 15.3 Tape Saturation in Feedback (MasterDelay)
**Source:** Meris Delay Pro, DubDelay heritage — each repeat gets darker and more saturated.
**Implementation:** Feedback path includes: HP filter (60Hz) → one-pole LP damping → Saturator (Tape mode, low drive 0.15). Each echo iteration accumulates subtle harmonic warmth while losing high-frequency content. Classic dub delay character.

### 15.4 Drift Mode (MasterModulation)
**Source:** Chase Bliss Warped Vinyl — analog tape wow/flutter via random walk LFO.
**Implementation:** `Mode::Drift` uses a smoothed random walk LFO instead of periodic sine/triangle. New random targets are generated at intervals scaled by rate. One-pole smoothing creates organic, non-repeating pitch drift. BBD lowpass (8kHz) adds analog character.

### 15.5 Envelope Follower (MasterFXSequencer)
**Source:** Torso S-4, AIR Sprite — input dynamics drive parameter modulation.
**Implementation:** `master_seqEnvFollow` enables an envelope follower (50ms attack, 200ms release) that tracks input RMS. The follower output cross-modulates the sequencer depth — louder playing increases modulation intensity. `master_seqEnvAmount` [0..1] controls how much the envelope affects depth.

### 15.6 Cross-Modulation via Sequencer Targets (MasterFXSequencer)
**Source:** Torso S-4 — modulators modulating each other's parameters.
**Implementation:** The sequencer can target any Master FX parameter (saturation drive, delay feedback/mix, reverb mix, modulation depth/rate, compressor mix). With envelope follower enabled, the input dynamics modulate the sequencer depth, which modulates the target parameter — creating a 2-level modulation chain. This is the simplest form of cross-modulation that delivers the most musical result.

### 15.7 Total Parameter Count

With v2 features, the final count is **28 parameters** (7 original + 21 new):
- Stage 1 (Saturation): 1 param
- Stage 2 (Delay): 7 params (time, feedback, mix, ping-pong, damping, diffusion, sync)
- Stage 3 (Reverb): 2 params
- Stage 4 (Modulation): 5 params (rate, depth, mix, mode, feedback)
- Stage 5 (Compressor): 4 params
- Stage 6 (Sequencer): 9 params (enabled, rate, steps, depth, smooth, target1, target2, pattern, envFollow, envAmount)

---

## 16. Implementation Status

| Component | File | Status |
|-----------|------|--------|
| MasterDelay DSP | `Source/DSP/Effects/MasterDelay.h` | **COMPLETE** |
| MasterModulation DSP | `Source/DSP/Effects/MasterModulation.h` | **COMPLETE** |
| MasterFXSequencer | `Source/Core/MasterFXSequencer.h` | **COMPLETE** |
| MasterFXChain (6-stage) | `Source/Core/MasterFXChain.h` | **COMPLETE** |
| APVTS Parameters (28) | `Source/XOlokunProcessor.cpp` | **COMPLETE** |
| Transport sync (PPQ/BPM) | `Source/XOlokunProcessor.cpp` | **COMPLETE** |
| UI (6-section strip) | `Source/UI/XOlokunEditor.h` | **COMPLETE** |
