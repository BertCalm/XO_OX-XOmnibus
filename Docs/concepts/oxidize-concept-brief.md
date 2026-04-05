# OXIDIZE — Concept Brief

## Engine Identity

| Field | Value |
|-------|-------|
| **Name** | OXIDIZE (XOxidize) |
| **Parameter Prefix** | `oxidize_` |
| **Plugin Code** | `Oxdz` |
| **Thesis** | Degradation as synthesis — play the sound of entropy itself |
| **Producer Pitch** | A synth where every note ages. Staccato = pristine. Sustain = beautifully ruined. The longer you hold, the more the sound corrodes, crackles, wobbles, and fossilizes. The degradation isn't an effect — it's the instrument. |
| **Accent Color** | Verdigris `#4A9E8E` — the blue-green patina of oxidized copper |

## Source Concept: Chemistry of Oxidation

Oxidation — the irreversible chemical process of material transforming through exposure to oxygen and time. Rust. Patina. Tape oxide. Vinyl groove wear. Circuit board corrosion.

### Key Elements → DSP Mapping

| Element | Description | DSP Implementation | Parameters |
|---------|-------------|-------------------|------------|
| **Patina** | Beautiful degradation — copper turns verdigris | Colored noise oscillator — pitched crackle/hiss as synthesis source | `oxidize_patinaDensity`, `oxidize_patinaTone` |
| **Corrosion** | Destructive degradation — iron rusts, circuits fail | Waveshaping curve that evolves with note age | `oxidize_corrosionDepth`, `oxidize_corrosionMode` (6 modes) |
| **Entropy** | Irreversible disorder increase | Bit/sample-rate reduction that increases with note age | `oxidize_entropyDepth`, `oxidize_entropySmooth` |
| **Erosion** | Gradual detail loss — grooves wear smooth | Filter whose cutoff drops with note age | `oxidize_erosionRate`, `oxidize_erosionFloor` |
| **Sediment** | Accumulated layers — debris builds up | Reverb that accumulates, never fully decays | `oxidize_sedimentDecay`, `oxidize_sedimentTone` |
| **Dropout** | Sudden failure — tape head loses contact | Probability-based amplitude gate, probability ↑ with age | `oxidize_dropoutRate`, `oxidize_dropoutDepth` |

## The Killer Feature: AGING (Note Age as Synthesis Parameter)

Every aspect of the sound transforms based on how long the note has been held:

```
Note Age:    0s        0.5s       2s         5s         15s        30s+
             │         │          │          │          │          │
Corrosion:   Clean ──→ Warm ───→ Saturated → Broken ──→ Destroyed
Erosion:     Bright ─→ Present ─→ Warm ────→ Muffled ─→ Submerged
Entropy:     Hi-fi ──→ 16-bit ──→ 12-bit ──→ 8-bit ───→ 4-bit
Wobble:      Stable ─→ Gentle ──→ Seasick ─→ Warped ──→ Melting
Patina:      Silent ─→ Whisper ─→ Crackle ─→ Hiss ────→ Storm
Dropout:     Never ──→ Rare ────→ Occasional → Frequent → Constant
Sediment:    Dry ────→ Room ────→ Hall ─────→ Cathedral → Infinite
```

- `oxidize_ageRate` — master control: how fast time passes for the sound
- `oxidize_ageOffset` — where on the aging timeline the note begins (velocity mappable: hard hits = pre-aged)

Extends Blessing B040 (OXYTOCIN: Note Duration as Synthesis Parameter) into degradation domain.

## Signal Flow

```
                    ┌─── Note Age Accumulator ───┐
                    │  (drives all aging params)  │
                    └──────────┬──────────────────┘
                               │
Patina Oscillator ─────────────┤
(pitched noise/crackle)        │
         +                     ▼
Basic Oscillator ───→ [Corrosion Waveshaper] ──→ [Erosion Filter]
(saw/pulse/noise)     (age-driven curve)         (cutoff drops w/ age)
                                                        │
                                                        ▼
                                              [Entropy Quantizer]
                                              (bits/rate reduce w/ age)
                                                        │
                                                        ▼
                                              [Wobble Generator]
                                              (wow + flutter, depth ↑ w/ age)
                                                        │
                                                        ▼
                                              [Dropout Gate]
                                              (probability ↑ w/ age)
                                                        │
                                                        ▼
                                              [Sediment Reverb]
                                              (accumulates, never fully decays)
                                                        │
                                                        ▼
                                                    [VCA] ──→ Output
                                                              + Coupling
```

## Vocabulary

| Term | Meaning | Use In |
|------|---------|--------|
| Patina | Beautiful, warm degradation | Macro M1, noise layer name |
| Corrosion | Harsh, destructive degradation | Waveshaper stage name |
| Fossil | Extreme aged state | Preset category |
| Erosion | Gradual detail loss | Filter behavior name |
| Sediment | Accumulated artifact layers | Reverb name |
| Dropout | Sudden amplitude failure | Gate behavior name |
| Verdigris | Blue-green color of oxidized copper | Accent color, preset name |
| Flux | Per-module randomization | Modulation parameter suffix |
| Tarnish | Surface-level degradation | Preset name, light corrosion |
| Relic | Preserved ancient artifact | Preset category |

## Macro Mapping

| Macro | Name | Controls |
|-------|------|----------|
| M1 CHARACTER | PATINA | Patina↔Corrosion balance (warm aging vs. harsh destruction) |
| M2 MOVEMENT | AGE | Age rate — how fast time passes for the sound |
| M3 COUPLING | ENTROPY | Global degradation depth across all stages |
| M4 SPACE | SEDIMENT | Accumulating reverb amount + dropout probability |

## Coupling Design

| Role | Behavior |
|------|----------|
| Source | `getSampleForCoupling()` returns the degraded signal |
| Destination | Coupling signal modulates ageRate (louder = faster aging) OR replaces oscillator (OXIDIZE becomes degradation processor) |
| CouplingType Support | AmplitudeModulation, FrequencyModulation, SpectralShaping |

### Best Coupling Partners
- OUROBOROS — feedback loops that age and decay infinitely
- OXYTOCIN — dual note-duration engines = compound temporal synthesis
- OSMOSIS — external audio through the degradation pipeline
- OPENSKY — shimmer that gradually corrodes into texture
- OWARE — tuned percussion that fossilizes into metallic sediment

## Corrosion Modes (6 Waveshaper Algorithms)

1. **Valve** — Tube saturation, smooth even harmonics
2. **Transformer** — Magnetic core saturation, low-mid thickening
3. **Broken Speaker** — Cone distortion, ragged papery breakup
4. **Tape Sat** — Magnetic tape compression, gentle 3rd harmonic
5. **Rust** — Asymmetric clipping that grows more asymmetric with age
6. **Acid** — Extreme nonlinear wavefolder, metallic and unstable

## Doctrine Pre-Check

| Doctrine | Status | Implementation |
|----------|--------|---------------|
| D001 | PASS | Velocity → initial age offset + erosion filter brightness |
| D002 | PASS | 2 LFOs (wow + flutter), mod wheel → age rate, aftertouch → corrosion depth, 4 macros, 4+ mod matrix slots |
| D003 | N/A | Chemistry-inspired metaphor, not physical model |
| D004 | PASS | Every parameter wired to audio by design |
| D005 | PASS | Wow LFO rate floor 0.01 Hz, aging itself is continuous autonomous modulation |
| D006 | PASS | Velocity → age offset + filter, aftertouch → corrosion, mod wheel → age rate |

## 7-Gate Concept Test

| # | Gate | Result |
|---|------|--------|
| 1 | XO Name | PASS — OXIDIZE, no conflicts |
| 2 | Thesis | PASS — "A synth where notes age" |
| 3 | Unique Sound | PASS — No fleet engine does time-dependent degradation synthesis |
| 4 | Coupling Potential | PASS — 8/10, natural partners identified |
| 5 | Doctrine Pre-Check | PASS — All 6 pass or N/A |
| 6 | Vocabulary | PASS — 10 evocative terms |
| 7 | Dramatic Arc | PASS — Init warmth → explore aging extremes → master coupling entropy |

## Phase 2 Plan: fXOxide (Singularity FX)

After the engine ships, extract the 6 degradation stages into fXOxide — a Singularity FX that processes any engine's audio through the aging pipeline. The FX version replaces "note age" with an envelope follower — quiet signals age faster (silence corrodes), loud signals stay fresh (energy preserves).

## Estimated Parameter Count

~35-40 parameters:
- Oscillator (4): waveform, patinaDensity, patinaTone, oscMix
- Aging (4): ageRate, ageOffset, ageCurve, ageVelSens
- Corrosion (3): corrosionDepth, corrosionMode, corrosionFlux
- Erosion (3): erosionRate, erosionFloor, erosionFlux
- Entropy (3): entropyDepth, entropySmooth, entropyFlux
- Wobble (4): wowDepth, flutterDepth, wobbleRate, wobbleStereo
- Dropout (3): dropoutRate, dropoutDepth, dropoutFlux
- Sediment (3): sedimentDecay, sedimentTone, sedimentMix
- Modulation (4): lfo1Rate, lfo1Depth, lfo2Rate, lfo2Depth
- Expression (4): velSens, aftertouch, modWheel, pitchBend
- Macros (4): macroPATINA, macroAGE, macroENTROPY, macroSEDIMENT
