# XOnset — Architecture Blueprint (Phase 1 Locked)

**Date:** March 2026
**Status:** Phase 1 LOCKED — Parameter architecture confirmed, remaining phases defined
**Engine location:** `Source/Engines/Onset/OnsetEngine.h` (native XOlokun engine)

---

## 1. Current Implementation State

OnsetEngine.h: **1041 lines**, fully integrated in XOlokun gallery.

### What's Built

| Component | Status | Lines | Notes |
|-----------|--------|-------|-------|
| OnsetNoiseGen | ✅ Complete | 27 | xorshift32 PRNG |
| OnsetEnvelope | ✅ Complete | 82 | AD with dirty-flag coefficient caching |
| OnsetTransient | ✅ Complete | 134 | Pre-blend pitch spike + noise burst |
| BridgedTOsc | ✅ Complete | 187 | TR-808 kick/tom: decaying sine + pitch env + sub |
| NoiseBurstCircuit | ✅ Complete | 262 | 808/909 snare + clap (multi-burst) |
| MetallicOsc | ✅ Complete | 322 | 808-style 6-oscillator hat/cymbal |
| FMPercussion | ✅ Complete | 371 | 2-op FM with self-feedback |
| ModalResonator | ✅ Complete | 429 | 8-mode parallel BP resonator bank (membrane ratios) |
| KarplusStrongPerc | ✅ Complete | 483 | Delay line + averaging filter |
| PhaseDistPerc | ✅ Complete | 540 | CZ-inspired nonlinear phase accumulator |
| OnsetVoice | ✅ Complete | 687 | Full voice: LayerX + LayerO + blend + transient + filter + env |
| OnsetEngine | ✅ Complete | 1041 | SynthEngine impl, MIDI, coupling, params |

### What's Missing

| Component | Status | Spec Reference |
|-----------|--------|----------------|
| CrossVoiceCouplingMatrix (8×8) | ❌ Not built | Spec §6 — only hat choke hardcoded |
| OnsetStepSequencer | ❌ Not built | Spec §5.4, Integration §4 |
| 4 Macros (MACHINE/PUNCH/SPACE/MUTATE) | ❌ Not built | Integration §7 |
| Envelope shape selection (AD/AHD/ADSR) | ❌ Not built | Spec §3.3 — currently AD only |
| Character stage (grit/warmth) | ❌ Not built | Spec §3.2, Integration §2.3 step 5 |
| FX rack (delay/reverb/lofi) | ❌ Not built | Spec §3.2, Integration §2.3 step 6 |
| Factory presets (.xometa) | ❌ Not built | Integration §8 — 85 target |

---

## 2. Locked Parameter Architecture

### 2.1 Current Parameters (83 registered, working)

**Per-voice (×8 voices = 80 params):**

| ID Pattern | Type | Range | Default (varies by voice) |
|------------|------|-------|---------------------------|
| `perc_v{n}_blend` | Float | 0–1 | V1:0.2, V2:0.5, V3:0.7, V4:0.7, V5:0.4, V6:0.3, V7:0.6, V8:0.8 |
| `perc_v{n}_algoMode` | Choice | FM/Modal/K-S/PhaseDist | V1:Modal, V2:FM, V3:FM, V4:FM, V5:PD, V6:Modal, V7:K-S, V8:Modal |
| `perc_v{n}_pitch` | Float | -24 to +24 st | 0 |
| `perc_v{n}_decay` | Float | 0.01–8s (skewed 0.3) | V1:0.5, V2:0.3, V3:0.05, V4:0.4, V5:0.25, V6:0.4, V7:0.3, V8:0.35 |
| `perc_v{n}_tone` | Float | 0–1 | 0.5 |
| `perc_v{n}_snap` | Float | 0–1 | 0.3 |
| `perc_v{n}_body` | Float | 0–1 | 0.5 |
| `perc_v{n}_character` | Float | 0–1 | 0.0 |
| `perc_v{n}_level` | Float | 0–1 | 0.7 |
| `perc_v{n}_pan` | Float | -1 to +1 | 0.0 |

**Global (3 params):**

| ID | Type | Range | Default |
|----|------|-------|---------|
| `perc_level` | Float | 0–1 | 0.8 |
| `perc_drive` | Float | 0–1 | 0.0 |
| `perc_masterTone` | Float | 0–1 | 0.5 |

### 2.2 Parameters to Add (Phase 3–5)

**Per-voice additions (×8 = 8 params):**

| ID Pattern | Type | Range | Default | Purpose |
|------------|------|-------|---------|---------|
| `perc_v{n}_envShape` | Choice | AD/AHD/ADSR | 0 (AD) | Envelope mode selection |

**Macros (4 params):**

| ID | Type | Range | Default | Targets |
|----|------|-------|---------|---------|
| `perc_macro_machine` | Float | 0–1 | 0.5 | All 8 `v{n}_blend` simultaneously |
| `perc_macro_punch` | Float | 0–1 | 0.5 | All `v{n}_snap` + `v{n}_body` |
| `perc_macro_space` | Float | 0–1 | 0.0 | Reverb send + delay feedback |
| `perc_macro_mutate` | Float | 0–1 | 0.0 | Randomize blend + character ±20% |

**Cross-Voice Coupling (XVC) (6 params):**

| ID | Type | Range | Default | Route |
|----|------|-------|---------|-------|
| `perc_xvc_kick_to_snare_filter` | Float | 0–1 | 0.15 | V1 amp → V2 filter cutoff |
| `perc_xvc_snare_to_hat_decay` | Float | 0–1 | 0.10 | V2 amp → V3 decay tighten |
| `perc_xvc_kick_to_tom_pitch` | Float | 0–1 | 0.0 | V1 amp → V6 pitch duck |
| `perc_xvc_snare_to_perc_blend` | Float | 0–1 | 0.0 | V2 trigger → V7 blend shift |
| `perc_xvc_hat_choke` | Bool | on/off | on | V3 trigger chokes V4 |
| `perc_xvc_global_amount` | Float | 0–1 | 0.5 | Scales all XVC routing |

**FX (8 params):**

| ID | Type | Range | Default | Purpose |
|----|------|-------|---------|---------|
| `perc_fx_delay_time` | Float | 0.01–1.0s | 0.3 | Echo time |
| `perc_fx_delay_feedback` | Float | 0–0.95 | 0.3 | Echo feedback |
| `perc_fx_delay_mix` | Float | 0–1 | 0.0 | Echo wet/dry |
| `perc_fx_reverb_size` | Float | 0–1 | 0.4 | Room size |
| `perc_fx_reverb_decay` | Float | 0–1 | 0.3 | Reverb tail |
| `perc_fx_reverb_mix` | Float | 0–1 | 0.0 | Reverb wet/dry |
| `perc_fx_lofi_bits` | Float | 4–16 | 16 | Bit reduction |
| `perc_fx_lofi_mix` | Float | 0–1 | 0.0 | LoFi wet/dry |

**Character stage (2 params):**

| ID | Type | Range | Default | Purpose |
|----|------|-------|---------|---------|
| `perc_char_grit` | Float | 0–1 | 0.0 | Post-mix saturation warmth |
| `perc_char_warmth` | Float | 0–1 | 0.5 | Tonal character (bright↔warm) |

**Total after all phases: 83 + 8 + 4 + 6 + 8 + 2 = 111 parameters**

---

## 3. Voice Configuration (Locked)

| Voice | Name | Circuit Type | Default Algo | MIDI Note | Base Freq | Default Blend | Clap |
|-------|------|-------------|-------------|-----------|-----------|---------------|------|
| V1 | Kick | BridgedT (0) | Modal (1) | C1 (36) | 55 Hz | 0.2 (X-heavy) | No |
| V2 | Snare | NoiseBurst (1) | FM (0) | D1 (38) | 180 Hz | 0.5 (blended) | No |
| V3 | HH-Closed | Metallic (2) | FM (0) | F#1 (42) | 8000 Hz | 0.7 (O-heavy) | No |
| V4 | HH-Open | Metallic (2) | FM (0) | A#1 (46) | 8000 Hz | 0.7 (O-heavy) | No |
| V5 | Clap | NoiseBurst (1) | PhaseDist (3) | D#1 (39) | 1200 Hz | 0.4 (slight X) | Yes |
| V6 | Tom | BridgedT (0) | Modal (1) | A1 (45) | 110 Hz | 0.3 (slight X) | No |
| V7 | Perc A | BridgedT (0) | K-S (2) | C#1 (37) | 220 Hz | 0.6 (slight O) | No |
| V8 | Perc B | Metallic (2) | Modal (1) | G#1 (44) | 440 Hz | 0.8 (O-heavy) | No |

Extended MIDI mapping: Note 35 → V1 (bass drum alternate).

---

## 4. Signal Chain (Per Voice — Current)

```
MIDI Trigger
    │
    ├──► Layer X (Circuit)                    ┐
    │    ├─ BridgedTOsc (kick/tom)            │
    │    ├─ NoiseBurstCircuit (snare/clap)    │    ┌────────────┐
    │    └─ MetallicOsc (hat/cymbal)          ├───►│   BLEND    │
    │                                          │    │ cos/sin    │
    ├──► Layer O (Algorithm)                   │    │ equal-pwr  │
    │    ├─ FMPercussion (2-op + feedback)     │    └─────┬──────┘
    │    ├─ ModalResonator (8-mode bank)       │          │
    │    ├─ KarplusStrongPerc (delay line)     │          ▼
    │    └─ PhaseDistPerc (CZ-style)          ┘    OnsetTransient
    │                                                (snap inject)
    │                                                     │
    │                                                     ▼
    │                                              Voice Filter
    │                                              (Cytomic SVF LP)
    │                                                     │
    │                                                     ▼
    └──────────────────────────────────────────►   Amp Envelope
                                                   (AD, vel-scaled)
                                                         │
                                                         ▼
                                                   Voice Output
```

## 5. Signal Chain (Global — Current + Planned)

```
Voice 1 ──┐
Voice 2 ──┤
Voice 3 ──┤                                    [PHASE 3]
Voice 4 ──┼──► Voice Mixer ──► Cross-Voice ──► Character Stage
Voice 5 ──┤   (level, pan)    Coupling 8×8     (grit/warmth)
Voice 6 ──┤                                         │
Voice 7 ──┤                                    [PHASE 4]
Voice 8 ──┘                                    FX Rack ──► Master Out
                                               (delay/reverb/lofi)
                                                     │
                                                     ▼
                                               Coupling Buffer
                                               (for MegaCouplingMatrix)
```

---

## 6. Coupling Interface (Locked)

### ONSET as Source (ONSET → other engines)

`getSampleForCoupling()` returns: post-mixer, post-drive, post-masterTone stereo output.

| Target | Type | Musical Effect |
|--------|------|----------------|
| Any melodic | `AmpToFilter` | Kick envelope → melodic filter cutoff (dub pump) |
| OVERDUB | Audio send | Master output → OVERDUB send/return chain |
| ODYSSEY | `AmpToFilter` | Hat triggers → Prism Shimmer depth |
| ODDFELIX | Trigger reset | Kick trigger → ODDFELIX envelope reset (locked groove) |

### ONSET as Target (other engines → ONSET)

| Source | Type | What ONSET Does |
|--------|------|-----------------|
| Any | `AmpToFilter` | Accumulates into `couplingFilterMod` → all voice tone params |
| ODDOSCAR | `EnvToDecay` | Accumulates into `couplingDecayMod` → all voice decay params |
| OVERBITE | `RhythmToBlend` | Accumulates into `couplingBlendMod` → all voice blend params |
| Any | `AmpToChoke` | If >0.5, chokes all voices (panic) |
| OBESE | `LFOToPitch` | Adds pitch mod to all voices |

### Types ONSET should NOT receive

| Type | Why |
|------|-----|
| `AudioToFM` | Drum voices don't have FM carrier inputs from external audio |
| `AudioToWavetable` | No wavetable in ONSET |
| `PitchToPitch` | Per-voice pitch is hit-specific, not continuously tracked |

---

## 7. Macro Mapping (Phase 3)

| Macro | Label | Internal Targets | Behavior |
|-------|-------|-----------------|----------|
| M1 | MACHINE | All 8 `v{n}_blend` | 0=pure Circuit (808/909), 1=pure Algorithm (FM/Modal/K-S/PD). THE XOnset identity knob. |
| M2 | PUNCH | All `v{n}_snap` + `v{n}_body` | 0=soft pillowy hits, 1=aggressive transients with heavy sub |
| M3 | SPACE | `fx_reverb_mix` + `fx_delay_feedback` | 0=bone dry, 1=drowned in dub echo |
| M4 | MUTATE | Randomize `v{n}_blend` + `v{n}_character` ±20% | 0=stable kit, 1=evolving drift per hit |

All 4 macros produce audible change at every position in every preset.

---

## 8. Identity (Locked)

| Field | Value |
|-------|-------|
| Gallery code | ONSET |
| Engine ID | `"Onset"` |
| Accent color | Electric Blue `#0066FF` |
| Parameter prefix | `perc_` |
| Max voices | 8 (fixed — never reduced) |
| CPU budget | <15.5% @ 8 voices, all active, 44100Hz, 512 block |
| DSP location | `Source/Engines/Onset/OnsetEngine.h` (inline, native XOlokun) |

---

## 9. Remaining Build Phases

### Phase 3 — Cross-Voice Coupling + Macros

Add to `OnsetEngine.h`:

1. **CrossVoiceCouplingMatrix** — 8×8 matrix with 6 coupling types
   - Default normalled: hat choke (already done), kick→snare filter (15%), snare→hat decay (10%)
   - 6 `perc_xvc_` parameters for user control
   - Process between voice render and voice mixer (inside `renderBlock()`)

2. **MacroMapper** — 4 macros with eased curves
   - `perc_macro_machine`: applies offset to all `v{n}_blend`
   - `perc_macro_punch`: applies offset to all `v{n}_snap` + `v{n}_body`
   - `perc_macro_space`: drives FX sends (wired in Phase 4)
   - `perc_macro_mutate`: per-block random drift on blend + character

3. **Per-voice envelope shape** — `perc_v{n}_envShape` choice param
   - Extend `OnsetEnvelope` with AHD (hold stage) and ADSR modes

**Gate:** MACHINE macro smoothly morphs entire kit. XVC kick→snare filter is audible.

### Phase 4 — FX + Character Stage

4. **Character stage** — post-mixer, pre-FX
   - `perc_char_grit`: soft tanh saturation (reuse pattern from OVERBITE's FurStage)
   - `perc_char_warmth`: one-pole LP on saturated signal

5. **FX rack** — serial: Delay → Reverb → LoFi
   - Delay: dark tape echo (similar to OVERDUB's architecture)
   - Reverb: tight room (drums don't need hall — max 0.8s decay)
   - LoFi: bitcrush + sample rate reduction
   - All with mix controls, default 0 (dry)

6. Wire SPACE macro to FX params

**Gate:** FX chain preserves transient punch. Bass integrity test passes.

### Phase 5 — Presets + Polish

7. **10 hero presets first:**
   - 808 Reborn (circuit-heavy, classic sub kick)
   - Membrane Theory (modal resonator showcase)
   - Quantum Kit (max blend — both layers audible)
   - Living Machine (MACHINE macro sweep from analog to digital)
   - Time Stretch (K-S heavy, metallic sustain)
   - Dub Pressure (coupled with OVERDUB engine — echo throws)
   - Future 909 (blend 0.5 on snare/hat — hybrid)
   - Particle Drums (OPAL granulates ONSET output)
   - Analog Heart (all blend=0 — pure circuit)
   - Mutant Factory (MUTATE macro cranked — evolving kit)

8. **Fill to 85 presets** across 6 categories:
   - Circuit Kits (15) — pure Layer X
   - Algorithm Kits (15) — pure Layer O
   - Hybrid Kits (20) — blend zone
   - Coupled Kits (15) — designed for cross-engine use
   - Morphing Kits (10) — MACHINE macro sweeps
   - Fusion Kits (10) — multi-engine presets

9. CPU optimization pass (NEON for modal resonators, inactive voice skip)

**Gate:** All presets load/save in .xometa. CPU <15.5% with all voices active.

---

## 10. QA Annotations (Already in Code)

The existing implementation includes QA markers:

| Tag | Fix | Location |
|-----|-----|----------|
| QA C1 | Blend gains precomputed per block (hoist trig) | OnsetEngine::renderBlock L808-818 |
| QA C2 | Pan gains precomputed per block (hoist sqrt) | OnsetEngine::renderBlock L816-818 |
| QA C3 | Single log instead of 8 pows for modal inharm | ModalResonator::trigger L393 |
| QA C4 | Denormal protection on FM feedback path | FMPercussion::process L360 |
| QA C5 | Early-out when PD envelope silent | PhaseDistPerc::process L511 |
| QA C6 | Clear coupling buffer prevents stale tail reads | OnsetEngine::renderBlock L787 |
| QA I1 | Master tone as LP filter on output | OnsetEngine::renderBlock L781-784 |
| QA I3 | Reset filter state on voice retrigger | OnsetVoice::triggerVoice L624 |
| QA W1 | Excite decay coefficient fix | ModalResonator::trigger L404 |
| QA W2 | Pitch decay coefficient formula fix | BridgedTOsc::trigger L155, NoiseBurstCircuit L209/212 |

---

*Phase 1: LOCKED. Next action: Phase 3 — Cross-Voice Coupling + Macros.*
*Process: `/new-xo-engine` | Engine: ONSET | Accent: #0066FF | Prefix: perc_*
