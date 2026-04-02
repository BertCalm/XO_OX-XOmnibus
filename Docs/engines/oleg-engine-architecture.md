# XOleg Engine Architecture — "The Sacred Bellows"

**Chef Quad #3** | Baltic/Eastern Europe | Orthodox Gold `#C5A036`
**Parameter prefix:** `oleg_` | **Engine ID:** `Oleg`

## Overview

XOleg is a switchable multi-organ synthesizer with 4 models selectable via `oleg_organ`:

| Index | Model | Character | Key DSP |
|-------|-------|-----------|---------|
| 0 | **Bayan** | Commanding, concert, sacred | Cassotto resonance chamber (comb+allpass+LP) |
| 1 | **Hurdy-gurdy** | Medieval, droning, hypnotic | Buzz bridge waveshaper + 2 drone oscillators |
| 2 | **Bandoneon** | Melancholic, warm, tango | Bisonoric push/pull banks (velocity-switched) |
| 3 | **Garmon** | Raw, folk, dance-driven | Asymmetric duty cycle + aggressive nonlinearity |

## DSP Components

### OlegCassotto (Bayan-specific)
Simulates the wooden cassotto tone chamber. Short comb filter (~3ms = 300Hz body resonance) + 2 allpass diffusion stages + lowpass wall absorption filter. Controls: `oleg_cassottoDepth` (0-1).

### OlegBuzzBridge (Hurdy-gurdy signature)
Models the trompette/chien buzz bridge mechanism. BPF extraction (350Hz center) -> threshold-gated cubic soft-clip waveshaper -> rattle resonance BPF (600Hz). The buzz ONLY activates when `pressure > oleg_buzzThreshold`, simulating the player's wrist snap (coups de poignet). This is the defining sound of the engine.

### OlegReedOscillator (all models)
Multi-model oscillator topology:
- **Bayan**: 2 detuned oscillators (saw + pulse mix)
- **Hurdy-gurdy**: 1 melody (saw + wheel vibrato) + 2 drone oscillators (configurable intervals)
- **Bandoneon**: 2 banks — push (triangle+saw, brighter) vs pull (dual triangle, warmer). Velocity >= 0.5 = push, < 0.5 = pull.
- **Garmon**: Asymmetric square (0.35 duty) + detuned square + saw

### OlegBellowsEnvelope
Sustained organ envelope with bellows pressure dynamics. Linear attack, sustained pressure with breathing modulation, exponential release. Model-specific timings (Bayan = strong sustain, HurdyGurdy = slow spin-up, Garmon = snappy dance).

## Parameter Map (30 params)

| Parameter | Range | Default | Target |
|-----------|-------|---------|--------|
| `oleg_organ` | 0-3 | 0 | Model select |
| `oleg_buzz` | 0-1 | 0.3 | Buzz/saturation intensity |
| `oleg_brightness` | 200-20kHz | 6000 | Voice LP filter cutoff |
| `oleg_drone` | 0-1 | 0.0 | Drone string level (hurdy-gurdy) |
| `oleg_bellows` | 0-1 | 0.7 | Bellows pressure scaling |
| `oleg_detune` | 0-50ct | 15 | Oscillator detuning |
| `oleg_formant` | 0-1 | 0.5 | Model-specific formant resonance |
| `oleg_attack` | 1ms-1s | 20ms | Bellows attack time |
| `oleg_release` | 5ms-3s | 150ms | Bellows release time |
| `oleg_filterEnvAmt` | 0-1 | 0.3 | Filter envelope depth |
| `oleg_bendRange` | 1-24st | 2 | Pitch bend range |
| `oleg_glideTime` | 0-1s | 0 | Portamento |
| `oleg_cassottoDepth` | 0-1 | 0.5 | Bayan chamber depth |
| `oleg_buzzThreshold` | 0-1 | 0.4 | Buzz bridge activation threshold |
| `oleg_wheelSpeed` | 0-1 | 0.3 | Hurdy-gurdy wheel speed (vibrato) |
| `oleg_droneInterval1` | -24-0st | -7 | Drone 1 pitch (5th below) |
| `oleg_droneInterval2` | -24-0st | -12 | Drone 2 pitch (octave below) |
| `oleg_macroCharacter` | 0-1 | 0 | M1: brightness + formant + buzz |
| `oleg_macroMovement` | 0-1 | 0 | M2: LFO depth + bellows + wheel |
| `oleg_macroCoupling` | 0-1 | 0 | M3: coupling sensitivity + drone |
| `oleg_macroSpace` | 0-1 | 0 | M4: cassotto + release + detune |
| `oleg_lfo1Rate` | 0.005-20Hz | 0.5 | LFO1 rate (D002/D005) |
| `oleg_lfo1Depth` | 0-1 | 0.1 | LFO1 depth → pitch vibrato |
| `oleg_lfo1Shape` | 0-4 | 0 | LFO1 waveform |
| `oleg_lfo2Rate` | 0.005-20Hz | 1.0 | LFO2 rate (D002) |
| `oleg_lfo2Depth` | 0-1 | 0.0 | LFO2 depth → filter cutoff |
| `oleg_lfo2Shape` | 0-4 | 0 | LFO2 waveform |

## Doctrine Compliance

| Doctrine | Implementation |
|----------|---------------|
| D001 Velocity→Timbre | Velocity scales filter envelope sweep (velBright = vel^2 * 3000Hz) |
| D002 Modulation | 2 LFOs + mod wheel→wheel speed + aftertouch→pressure + 4 macros |
| D003 Physics | Cassotto (comb+AP), buzz bridge (Pignol & Music 2014), bisonoric banks |
| D004 No Dead Params | All 30 params wired to DSP |
| D005 Breathing | Per-voice BreathingLFO (~12s cycle, 0.08Hz), phase-staggered |
| D006 Expression | Velocity→timbre, aftertouch→pressure, mod wheel→wheel speed/bellows |

## Coupling Compatibility

| CouplingType | Behavior |
|-------------|----------|
| AmpToFilter | External amp → filter cutoff (±2000Hz) |
| LFOToPitch | External LFO → pitch (±2 semitones) |
| AmpToPitch | External amp → pitch |
| EnvToMorph | External env → buzz intensity |

## Initial Presets (10)

| Name | Mood | Model | Character |
|------|------|-------|-----------|
| Cathedral Breath | Foundation | Bayan | Warm cassotto, clean, foundational |
| Garmon Earth | Foundation | Garmon | Earthy, folk, grounded |
| Drone Vespers | Atmosphere | HurdyGurdy | Full drones, gentle buzz, medieval |
| Tango Midnight | Atmosphere | Bandoneon | Warm pull, melancholic, Buenos Aires |
| Trompette Fury | Flux | HurdyGurdy | Aggressive buzz, fast wheel, medieval fury |
| Garmon Dance | Flux | Garmon | Snappy, buzzy, folk dance |
| Piazzolla Ghost | Prism | Bandoneon | Bisonoric expression, tango slides |
| Amber Chapel | Aether | Bayan | Deep cassotto, ethereal, sacred space |
| Medieval Machine | Entangled | HurdyGurdy | Industrial buzz + drones, coupling-ready |
| Sunken Liturgy | Submerged | Bayan | Deep underwater organ, dark, prayerful |

## Key Design Decisions

1. **Buzz bridge is threshold-gated** — not always-on. The activation threshold mechanic (pressure > threshold) is what makes the hurdy-gurdy buzz feel authentic. The player controls when the chien lifts off.

2. **Bandoneon bisonoric via velocity** — real bandoneons produce different notes on push vs pull. Since we can't detect physical bellows direction, velocity becomes the proxy: soft = pull (darker), hard = push (brighter). This creates expressively different timbres from the same note.

3. **Cassotto is Bayan-only by default** — the other models don't have a cassotto chamber. The parameter still exists (useful for sound design hybridization), but defaults to 0 for non-Bayan models.

4. **Drone system is hurdy-gurdy-primary** — drones are most relevant for hurdy-gurdy (where they're physically separate strings), but the drone parameter is available for all models for creative sound design.

5. **Orthodox Gold `#C5A036`** — chosen to represent the Baltic amber and Orthodox iconographic gold that defines Oleg's spiritual-industrial identity.
