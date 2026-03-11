# XOverbite — Canonical Parameter Architecture
*Phase 1 deliverable | 122 parameters | All IDs frozen — never rename after this document*

---

## Parameter Prefix: `poss_`

Legacy origin: XOpossum → XOverbite rename. Prefix is frozen forever per project rules.

---

## Complete Parameter Table

### Oscillator A — Belly (4 waveforms: Sine, Triangle, Saw, Cushion Pulse)

| # | Parameter ID | Type | Range | Default | Description |
|---|-------------|------|-------|---------|-------------|
| 1 | `poss_oscAWaveform` | Choice | Sine, Triangle, Saw, Cushion Pulse | 0 (Sine) | Belly oscillator waveform |
| 2 | `poss_oscAShape` | Float | 0.0–1.0 | 0.5 | Per-waveform shape (PW for pulse, brightness for saw) |
| 3 | `poss_oscADrift` | Float | 0.0–1.0 | 0.05 | Analog pitch drift amount |
| 4 | `poss_oscMix` | Float | 0.0–1.0 | 0.3 | Blend: 0 = all OscA, 1 = all OscB |

### Oscillator B — Bite (5 waveforms: Hard Sync Saw, FM, Ring Mod, Noise, Grit)

| # | Parameter ID | Type | Range | Default | Description |
|---|-------------|------|-------|---------|-------------|
| 5 | `poss_oscBWaveform` | Choice | Hard Sync Saw, FM, Ring Mod, Noise, Grit | 0 (Hard Sync Saw) | Bite oscillator waveform |
| 6 | `poss_oscBShape` | Float | 0.0–1.0 | 0.5 | Per-waveform shape (sync ratio, FM index, ring balance) |
| 7 | `poss_oscBInstability` | Float | 0.0–1.0 | 0.0 | Pitch/phase instability amount |

### Oscillator Interaction

| # | Parameter ID | Type | Range | Default | Description |
|---|-------------|------|-------|---------|-------------|
| 8 | `poss_oscInteractMode` | Choice | Off, Soft Sync, Low FM, Phase Push, Grit Multiply | 0 (Off) | Cross-oscillator interaction type |
| 9 | `poss_oscInteractAmount` | Float | 0.0–1.0 | 0.0 | Interaction intensity |

### Sub Oscillator

| # | Parameter ID | Type | Range | Default | Description |
|---|-------------|------|-------|---------|-------------|
| 10 | `poss_subLevel` | Float | 0.0–1.0 | 0.3 | Sub oscillator amplitude |
| 11 | `poss_subOctave` | Choice | -1 Oct, -2 Oct | 0 (-1 Oct) | Sub oscillator octave |

### Weight Engine (low-frequency reinforcement)

| # | Parameter ID | Type | Range | Default | Description |
|---|-------------|------|-------|---------|-------------|
| 12 | `poss_weightShape` | Choice | Sine, Triangle, Saw, Square, Pulse | 0 (Sine) | Weight engine waveform |
| 13 | `poss_weightOctave` | Choice | -1 Oct, -2 Oct, -3 Oct | 1 (-2 Oct) | Weight engine octave |
| 14 | `poss_weightLevel` | Float | 0.0–1.0 | 0.0 | Weight engine level |
| 15 | `poss_weightTune` | Float | -100.0–100.0 | 0.0 | Weight engine fine tune (cents) |

### Noise Source

| # | Parameter ID | Type | Range | Default | Description |
|---|-------------|------|-------|---------|-------------|
| 16 | `poss_noiseType` | Choice | White, Pink, Brown, Crackle, Hiss | 0 (White) | Noise source type |
| 17 | `poss_noiseRouting` | Choice | Pre-Filter, Post-Filter, Parallel, Sidechain | 0 (Pre-Filter) | Noise routing mode |
| 18 | `poss_noiseLevel` | Float | 0.0–1.0 | 0.0 | Noise source level |
| 19 | `poss_noiseDecay` | Float | 0.001–2.0 | 0.1 | Noise amplitude decay time (s) |

### Filter Block (4 modes: Burrow LP, Snarl BP, Wire HP, Hollow Notch)

| # | Parameter ID | Type | Range | Default | Description |
|---|-------------|------|-------|---------|-------------|
| 20 | `poss_filterCutoff` | Float | 20.0–20000.0 (skew 0.3) | 2000.0 | Filter cutoff frequency (Hz) |
| 21 | `poss_filterReso` | Float | 0.0–1.0 | 0.3 | Filter resonance |
| 22 | `poss_filterMode` | Choice | Burrow LP, Snarl BP, Wire HP, Hollow Notch | 0 (Burrow LP) | Filter mode |
| 23 | `poss_filterKeyTrack` | Float | 0.0–1.0 | 0.0 | Filter keyboard tracking amount |
| 24 | `poss_filterDrive` | Float | 0.0–1.0 | 0.0 | Pre-filter drive/saturation |

### Character Stages

| # | Parameter ID | Type | Range | Default | Description |
|---|-------------|------|-------|---------|-------------|
| 25 | `poss_furAmount` | Float | 0.0–1.0 | 0.0 | Fur — pre-filter soft saturation (plush warmth) |
| 26 | `poss_chewAmount` | Float | 0.0–1.0 | 0.0 | Chew — post-filter contour and gentle compression |
| 27 | `poss_chewFreq` | Float | 100.0–8000.0 (skew 0.3) | 1000.0 | Chew — contour center frequency (Hz) |
| 28 | `poss_chewMix` | Float | 0.0–1.0 | 0.5 | Chew — dry/wet mix |
| 29 | `poss_driveAmount` | Float | 0.0–1.0 | 0.0 | Internal overdrive amount |
| 30 | `poss_driveType` | Choice | Warm, Grit, Clip, Tube | 0 (Warm) | Internal overdrive character |
| 31 | `poss_gnashAmount` | Float | 0.0–1.0 | 0.0 | Gnash — post-filter asymmetric bite |
| 32 | `poss_trashMode` | Choice | Off, Rust, Splatter, Crushed | 0 (Off) | Trash — dirt mode |
| 33 | `poss_trashAmount` | Float | 0.0–1.0 | 0.0 | Trash — dirt amount |

### Amp Envelope

| # | Parameter ID | Type | Range | Default | Description |
|---|-------------|------|-------|---------|-------------|
| 34 | `poss_ampAttack` | Float | 0.001–5.0 (skew 0.4) | 0.005 | Amp envelope attack (s) |
| 35 | `poss_ampDecay` | Float | 0.001–5.0 (skew 0.4) | 0.3 | Amp envelope decay (s) |
| 36 | `poss_ampSustain` | Float | 0.0–1.0 | 0.8 | Amp envelope sustain level |
| 37 | `poss_ampRelease` | Float | 0.001–10.0 (skew 0.4) | 0.3 | Amp envelope release (s) |
| 38 | `poss_ampVelSens` | Float | 0.0–1.0 | 0.7 | Amp velocity sensitivity |

### Filter Envelope

| # | Parameter ID | Type | Range | Default | Description |
|---|-------------|------|-------|---------|-------------|
| 39 | `poss_filterEnvAmount` | Float | -1.0–1.0 | 0.3 | Filter envelope modulation depth |
| 40 | `poss_filterAttack` | Float | 0.001–5.0 (skew 0.4) | 0.005 | Filter envelope attack (s) |
| 41 | `poss_filterDecay` | Float | 0.001–5.0 (skew 0.4) | 0.3 | Filter envelope decay (s) |
| 42 | `poss_filterSustain` | Float | 0.0–1.0 | 0.0 | Filter envelope sustain level |
| 43 | `poss_filterRelease` | Float | 0.001–10.0 (skew 0.4) | 0.3 | Filter envelope release (s) |

### Mod Envelope

| # | Parameter ID | Type | Range | Default | Description |
|---|-------------|------|-------|---------|-------------|
| 44 | `poss_modEnvAmount` | Float | -1.0–1.0 | 0.0 | Mod envelope depth |
| 45 | `poss_modAttack` | Float | 0.001–5.0 (skew 0.4) | 0.01 | Mod envelope attack (s) |
| 46 | `poss_modDecay` | Float | 0.001–5.0 (skew 0.4) | 0.5 | Mod envelope decay (s) |
| 47 | `poss_modSustain` | Float | 0.0–1.0 | 0.0 | Mod envelope sustain level |
| 48 | `poss_modRelease` | Float | 0.001–10.0 (skew 0.4) | 0.5 | Mod envelope release (s) |
| 49 | `poss_modEnvDest` | Choice | OscA Shape, OscB Shape, Filter Cutoff, Fur, Gnash, Trash, Osc Mix, Weight Level | 2 (Filter Cutoff) | Mod envelope destination |

### LFO 1

| # | Parameter ID | Type | Range | Default | Description |
|---|-------------|------|-------|---------|-------------|
| 50 | `poss_lfo1Shape` | Choice | Sine, Triangle, Saw, Square, S&H, Random, Stepped | 0 (Sine) | LFO 1 shape |
| 51 | `poss_lfo1Rate` | Float | 0.01–50.0 (skew 0.3) | 1.0 | LFO 1 rate (Hz) |
| 52 | `poss_lfo1Depth` | Float | 0.0–1.0 | 0.0 | LFO 1 depth |
| 53 | `poss_lfo1Sync` | Choice | Off, On | 0 (Off) | LFO 1 tempo sync |
| 54 | `poss_lfo1Retrigger` | Choice | Off, On | 0 (Off) | LFO 1 note retrigger |
| 55 | `poss_lfo1Phase` | Float | 0.0–1.0 | 0.0 | LFO 1 start phase |

### LFO 2

| # | Parameter ID | Type | Range | Default | Description |
|---|-------------|------|-------|---------|-------------|
| 56 | `poss_lfo2Shape` | Choice | Sine, Triangle, Saw, Square, S&H, Random, Stepped | 0 (Sine) | LFO 2 shape |
| 57 | `poss_lfo2Rate` | Float | 0.01–50.0 (skew 0.3) | 2.0 | LFO 2 rate (Hz) |
| 58 | `poss_lfo2Depth` | Float | 0.0–1.0 | 0.0 | LFO 2 depth |
| 59 | `poss_lfo2Sync` | Choice | Off, On | 0 (Off) | LFO 2 tempo sync |
| 60 | `poss_lfo2Retrigger` | Choice | Off, On | 0 (Off) | LFO 2 note retrigger |
| 61 | `poss_lfo2Phase` | Float | 0.0–1.0 | 0.0 | LFO 2 start phase |

### LFO 3

| # | Parameter ID | Type | Range | Default | Description |
|---|-------------|------|-------|---------|-------------|
| 62 | `poss_lfo3Shape` | Choice | Sine, Triangle, Saw, Square, S&H, Random, Stepped | 0 (Sine) | LFO 3 shape |
| 63 | `poss_lfo3Rate` | Float | 0.01–50.0 (skew 0.3) | 0.5 | LFO 3 rate (Hz) |
| 64 | `poss_lfo3Depth` | Float | 0.0–1.0 | 0.0 | LFO 3 depth |
| 65 | `poss_lfo3Sync` | Choice | Off, On | 0 (Off) | LFO 3 tempo sync |
| 66 | `poss_lfo3Retrigger` | Choice | Off, On | 0 (Off) | LFO 3 note retrigger |
| 67 | `poss_lfo3Phase` | Float | 0.0–1.0 | 0.0 | LFO 3 start phase |

### Mod Matrix (8 slots)

Each slot: source → destination with bipolar amount.

**Sources:** Off, LFO 1, LFO 2, LFO 3, Amp Env, Filter Env, Mod Env, Velocity, Note, Aftertouch, Mod Wheel, Macro Belly, Macro Bite, Macro Scurry, Macro Trash, Macro Play Dead

**Destinations:** Off, OscA Shape, OscA Drift, OscB Shape, OscB Instability, Osc Mix, Osc Interact, Sub Level, Weight Level, Noise Level, Filter Cutoff, Filter Reso, Filter Drive, Fur, Chew, Drive, Gnash, Trash, Amp Level, Pan, FX Motion Rate, FX Motion Depth, FX Echo Time, FX Echo Feedback, FX Space Size, FX Space Decay

| # | Parameter ID | Type | Range | Default | Description |
|---|-------------|------|-------|---------|-------------|
| 68 | `poss_modSlot1Src` | Choice | (sources) | 0 (Off) | Mod matrix slot 1 source |
| 69 | `poss_modSlot1Dst` | Choice | (destinations) | 0 (Off) | Mod matrix slot 1 destination |
| 70 | `poss_modSlot1Amt` | Float | -1.0–1.0 | 0.0 | Mod matrix slot 1 amount |
| 71 | `poss_modSlot2Src` | Choice | (sources) | 0 (Off) | Mod matrix slot 2 source |
| 72 | `poss_modSlot2Dst` | Choice | (destinations) | 0 (Off) | Mod matrix slot 2 destination |
| 73 | `poss_modSlot2Amt` | Float | -1.0–1.0 | 0.0 | Mod matrix slot 2 amount |
| 74 | `poss_modSlot3Src` | Choice | (sources) | 0 (Off) | Mod matrix slot 3 source |
| 75 | `poss_modSlot3Dst` | Choice | (destinations) | 0 (Off) | Mod matrix slot 3 destination |
| 76 | `poss_modSlot3Amt` | Float | -1.0–1.0 | 0.0 | Mod matrix slot 3 amount |
| 77 | `poss_modSlot4Src` | Choice | (sources) | 0 (Off) | Mod matrix slot 4 source |
| 78 | `poss_modSlot4Dst` | Choice | (destinations) | 0 (Off) | Mod matrix slot 4 destination |
| 79 | `poss_modSlot4Amt` | Float | -1.0–1.0 | 0.0 | Mod matrix slot 4 amount |
| 80 | `poss_modSlot5Src` | Choice | (sources) | 0 (Off) | Mod matrix slot 5 source |
| 81 | `poss_modSlot5Dst` | Choice | (destinations) | 0 (Off) | Mod matrix slot 5 destination |
| 82 | `poss_modSlot5Amt` | Float | -1.0–1.0 | 0.0 | Mod matrix slot 5 amount |
| 83 | `poss_modSlot6Src` | Choice | (sources) | 0 (Off) | Mod matrix slot 6 source |
| 84 | `poss_modSlot6Dst` | Choice | (destinations) | 0 (Off) | Mod matrix slot 6 destination |
| 85 | `poss_modSlot6Amt` | Float | -1.0–1.0 | 0.0 | Mod matrix slot 6 amount |
| 86 | `poss_modSlot7Src` | Choice | (sources) | 0 (Off) | Mod matrix slot 7 source |
| 87 | `poss_modSlot7Dst` | Choice | (destinations) | 0 (Off) | Mod matrix slot 7 destination |
| 88 | `poss_modSlot7Amt` | Float | -1.0–1.0 | 0.0 | Mod matrix slot 7 amount |
| 89 | `poss_modSlot8Src` | Choice | (sources) | 0 (Off) | Mod matrix slot 8 source |
| 90 | `poss_modSlot8Dst` | Choice | (destinations) | 0 (Off) | Mod matrix slot 8 destination |
| 91 | `poss_modSlot8Amt` | Float | -1.0–1.0 | 0.0 | Mod matrix slot 8 amount |

### Macros (5)

| # | Parameter ID | Type | Range | Default | Description |
|---|-------------|------|-------|---------|-------------|
| 92 | `poss_macroBelly` | Float | 0.0–1.0 | 0.0 | Belly — plush weight (reduces OscB, adds Sub+Fur, lowers filter) |
| 93 | `poss_macroBite` | Float | 0.0–1.0 | 0.0 | Bite — feral aggression (increases OscB, Gnash, resonance) |
| 94 | `poss_macroScurry` | Float | 0.0–1.0 | 0.0 | Scurry — nervous energy (LFO rate multiply, envelope compress) |
| 95 | `poss_macroTrash` | Float | 0.0–1.0 | 0.0 | Trash — dirt and destruction (Trash amount, resonance) |
| 96 | `poss_macroPlayDead` | Float | 0.0–1.0 | 0.0 | Play Dead — decay to silence (release extend, volume duck, filter close) |

### FX: Motion (Chorus/Doubler/Flange)

| # | Parameter ID | Type | Range | Default | Description |
|---|-------------|------|-------|---------|-------------|
| 97 | `poss_fxMotionType` | Choice | Plush Chorus, Uneasy Doubler, Oil Flange | 0 (Plush Chorus) | Motion effect type |
| 98 | `poss_fxMotionRate` | Float | 0.01–10.0 (skew 0.3) | 0.5 | Motion modulation rate (Hz) |
| 99 | `poss_fxMotionDepth` | Float | 0.0–1.0 | 0.0 | Motion modulation depth |
| 100 | `poss_fxMotionMix` | Float | 0.0–1.0 | 0.0 | Motion dry/wet mix |

### FX: Echo (Delay)

| # | Parameter ID | Type | Range | Default | Description |
|---|-------------|------|-------|---------|-------------|
| 101 | `poss_fxEchoType` | Choice | Dark Tape, Murky Digital, Short Slap, Ping | 0 (Dark Tape) | Echo algorithm |
| 102 | `poss_fxEchoTime` | Float | 0.01–2.0 (skew 0.5) | 0.3 | Echo delay time (s) |
| 103 | `poss_fxEchoFeedback` | Float | 0.0–0.95 | 0.3 | Echo feedback amount |
| 104 | `poss_fxEchoMix` | Float | 0.0–1.0 | 0.0 | Echo dry/wet mix |
| 105 | `poss_fxEchoSync` | Choice | Off, On | 0 (Off) | Echo tempo sync |

### FX: Space (Reverb)

| # | Parameter ID | Type | Range | Default | Description |
|---|-------------|------|-------|---------|-------------|
| 106 | `poss_fxSpaceType` | Choice | Burrow Room, Fog Chamber, Drain Hall | 0 (Burrow Room) | Space algorithm |
| 107 | `poss_fxSpaceSize` | Float | 0.0–1.0 | 0.3 | Space room size |
| 108 | `poss_fxSpaceDecay` | Float | 0.1–20.0 (skew 0.3) | 1.5 | Space decay time (s) |
| 109 | `poss_fxSpaceDamping` | Float | 0.0–1.0 | 0.5 | Space high-frequency damping |
| 110 | `poss_fxSpaceMix` | Float | 0.0–1.0 | 0.0 | Space dry/wet mix |

### FX: Finish (Master processing)

| # | Parameter ID | Type | Range | Default | Description |
|---|-------------|------|-------|---------|-------------|
| 111 | `poss_fxFinishGlue` | Float | 0.0–1.0 | 0.0 | Glue compression amount |
| 112 | `poss_fxFinishClip` | Float | 0.0–1.0 | 0.0 | Soft clip / limiter amount |
| 113 | `poss_fxFinishWidth` | Float | 0.0–2.0 | 1.0 | Stereo width (0=mono, 1=normal, 2=wide) |
| 114 | `poss_fxFinishLowMono` | Float | 0.0–1.0 | 0.0 | Low-frequency mono summing (bass focus) |

### Voice Control

| # | Parameter ID | Type | Range | Default | Description |
|---|-------------|------|-------|---------|-------------|
| 115 | `poss_polyphony` | Choice | 1, 2, 4, 8, 16 | 3 (8) | Maximum polyphony |
| 116 | `poss_glideTime` | Float | 0.0–2.0 (skew 0.5) | 0.0 | Portamento/glide time (s) |
| 117 | `poss_glideMode` | Choice | Off, Legato, Always | 0 (Off) | Glide mode |
| 118 | `poss_unisonVoices` | Choice | 1, 2, 3, 4, 5, 6, 7 | 0 (1) | Unison voice count |
| 119 | `poss_unisonDetune` | Float | 0.0–1.0 | 0.2 | Unison detune spread |
| 120 | `poss_unisonSpread` | Float | 0.0–1.0 | 0.5 | Unison stereo spread |

### Output

| # | Parameter ID | Type | Range | Default | Description |
|---|-------------|------|-------|---------|-------------|
| 121 | `poss_level` | Float | 0.0–1.0 | 0.7 | Engine output level |
| 122 | `poss_pan` | Float | -1.0–1.0 | 0.0 | Engine pan position |

---

## Signal Flow

```
OscA (Belly) ─┐
              ├─ OscMix ─┬─ Fur ─┬─ FilterBlock ─┬─ Chew ─ Drive ─ Gnash ─ Trash ─┐
OscB (Bite) ──┘          │       │               │                                   │
                          │       │               │                                   │
Sub ──────────────────────┘       │               │                                   │
Weight ───────────────────────────┘               │                                   │
Noise ────────────(routing)───────────────────────┘                                   │
                                                                                      │
┌─────────────────────────────────────────────────────────────────────────────────────┘
│
├─ Amp Envelope ─ Motion FX ─ Echo FX ─ Space FX ─ Finish ─ Level/Pan ─ OUT
│
├─ Mod Envelope ─ (mod matrix destinations)
├─ Filter Envelope ─ Filter Cutoff
├─ LFO 1/2/3 ─ (mod matrix destinations)
└─ Macros (Belly/Bite/Scurry/Trash/Play Dead) ─ (hardcoded macro routes + mod matrix)
```

---

## Macro Hardcoded Routes

| Macro | Increases | Decreases | Modulates |
|-------|-----------|-----------|-----------|
| **Belly** | Sub Level, Fur Amount, Weight Level | OscB level (via OscMix→0) | Filter Cutoff ↓ |
| **Bite** | OscB level (OscMix→1), Gnash Amount | — | Filter Reso ↑ |
| **Scurry** | LFO1/2/3 rates (multiply) | Envelope times (compress) | — |
| **Trash** | Trash Amount, Filter Reso | — | Trash mode cycles |
| **Play Dead** | Amp Release, Filter Release | Level (duck), Filter Cutoff | FX Space Mix ↑ |

---

## Phase Map

| Phase | Parameters Added | Count |
|-------|-----------------|-------|
| Phase 1 (this) | ALL 122 declared, defaults set | 122 |
| Phase 2 | OscA/B extended, Weight, Noise, Sub, Voice, Amp env, Interaction DSP | 1–19, 34–38, 115–120 |
| Phase 3 | Filter, Fur, Chew, Drive, Gnash, Trash DSP | 20–33 |
| Phase 4 | Mod env, LFO 1–3, Mod matrix, Macros DSP | 44–96 |
| Phase 5 | FX chain DSP | 97–114 |
| Phase 6 | Presets authored using all 122 | — |
| Phase 7 | UI mapped to all 122 | — |

---

*Document frozen. Parameter IDs in this table must never be renamed.*
