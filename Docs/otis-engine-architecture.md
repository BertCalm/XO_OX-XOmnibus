# XOtis Engine Architecture — "The Soul Organ"

**Engine #45** | Chef Quad: Otis (American) | Accent: Soul Gold `#DAA520`
Parameter prefix: `otis_` | 37 parameters | 8 voices

## Identity

XOtis is the gospel fish — a golden humphead parrotfish swimming through Sunday morning services. Its body resonates with the tonewheels of every Hammond organ that ever played a church, a jazz club, a roadhouse. Otis Redding, Jimmy Smith, Billy Preston — their fingers are in the water.

## The 4 Organ Models

### Model 0: Hammond B3 (Tonewheel Organ)

The flagship model — the most recognizable organ sound in the world.

**DSP Architecture:**
- **9-drawbar additive synthesis** using the exact Hammond footage ratios: 16', 5-1/3', 8', 4', 2-2/3', 2', 1-3/5', 1-1/3', 1'. Each drawbar controls one harmonic partial. The ratios follow the original Hammond tonewheel gearing (not exact integer harmonics — this is critical to the Hammond warmth).
- **Tonewheel crosstalk**: Adjacent tonewheels bleed into each other via `TonewheelCrosstalk`. Each drawbar's harmonic gets additive sine leakage from the 2 nearest tonewheels above and below, with micro-detuning from gear tooth mesh (~10 cents). This is the warmth and "sizzle" that separates a real B3 from a clean additive synth.
- **Key click**: `KeyClick` generates a 1-3ms burst of filtered noise on note-on, shaped by fast exponential decay. The Hammond B3 click was an unintended artifact that musicians demanded back.
- **Percussion**: `HammondPercussion` adds single-trigger harmonic emphasis (2nd or 3rd harmonic, selectable). Only the first note in a legato passage gets percussion — a critical B3 behavior tracked by `percussionArmed` and `anyKeysHeld`.
- **Tube amp overdrive**: Velocity-scaled `fastTanh` saturation applied per-voice. D001 compliance: harder playing = more grit.

### Model 1: Calliope (Steam Organ)

Genuinely unhinged — steam-driven chaos.

**DSP Architecture:**
- `CalliopePipe` per voice: near-sine oscillator with massive pitch instability from steam pressure variation.
- **Per-pipe wobble**: Each pipe has its own pressure wobble rate (1.5-5 Hz) and random detuning (+-15 cents). No two pipes drift in sync.
- **Global steam pressure**: Shared ~1.7 Hz sinusoidal pressure variation affects all pipes, plus per-pipe local wobble.
- **Binary-adjacent dynamics**: Calliopes are mechanically on/off, but we add subtle amplitude wobble from steam pressure for organic life.
- **Spectrum**: Near-sine fundamental + weak 2nd harmonic (15%) + trace 3rd (5%) — steam pipe character.
- Light Leslie applied (30% depth) for spatial movement.

### Model 2: Blues Harmonica (Cross-Harp)

Breath-driven emotional expression. The bend is not a pitch wheel — it is an ENVELOPE.

**DSP Architecture:**
- `BluesHarpVoice` per voice: oscillator with attack-triggered pitch bend envelope.
- **Bend envelope**: Note starts at `bendAmount` semitones above target pitch and decays exponentially to 0 over 30-100ms (velocity-dependent: harder = faster bend, more air pressure). D001: velocity determines bend depth.
- **Breath vibrato**: 5.5 Hz sine vibrato with depth controlled by mod wheel (D006 expression).
- **Overblows**: At velocity > 0.85, the 2nd partial activates at 30% amplitude (reed flip to next partial).
- **Breath noise**: Band-limited noise proportional to velocity and aftertouch breath pressure.
- **Harmonic spectrum**: Fundamental + 2nd + 3rd harmonics with breath-dependent balance (harder breath pushes energy into upper harmonics).

### Model 3: Zydeco Accordion (Diatonic Button)

Louisiana dance music. The rhythm IS the instrument.

**DSP Architecture:**
- `ZydecoReed` per voice: dual-reed model with musette beating.
- **Two reeds per note**: Detuned by 3-8 cents (random per note), creating characteristic musette beating.
- **Reed spectrum**: Additive odd harmonics (1st + 3rd + 5th + 7th) approximating the free-reed square-ish spectrum with harmonic rolloff.
- **Bellows snap attack**: 5-15ms exponential transient burst with broadband noise (the physical bellows opening/closing).
- **Bellows pressure**: Continuous control via aftertouch + mod wheel for dynamics.

## Shared Systems

All four models share these systems applied post-model:

### Filter (CytomicSVF)
- LowPass mode, per-voice
- D001: Velocity scales cutoff (0.5x to 1.0x range)
- Filter envelope (FilterEnvelope): 1ms attack, 300ms decay
- LFO2 modulates cutoff (+-3000 Hz at full depth)

### Amp Envelope (FilterEnvelope)
- Configurable ADSR (Attack 1ms-2s, Decay 10ms-5s, Sustain 0-1, Release 10ms-5s)
- Default: 5ms attack, 300ms decay, 80% sustain, 300ms release

### Leslie Speaker (LeslieSpeaker)
- Post-voice-mix stereo effect (applied after all voices are summed)
- Full depth on Hammond (Model 0), 30% depth on Calliope (Model 1), off for Harmonica/Accordion
- Three speeds: brake (0 Hz), slow/chorale (~0.7 Hz horn), fast/tremolo (~6.7 Hz horn)
- Physical inertia: ~1.5 second speed ramp between states
- Horn AM (+-6 dB) + drum AM (+-3 dB) + Doppler pitch mod (+-20 cents)
- 90-degree stereo offset between horn/drum + crossfeed

### LFOs (StandardLFO)
- LFO1: routed to pitch modulation (vibrato), 0.005-20 Hz, 5 shapes
- LFO2: routed to filter cutoff, 0.005-20 Hz, 5 shapes
- D005: rate floor 0.005 Hz (200-second cycle) for autonomous breathing

### Expression (D006)
- **Mod wheel (CC1)**: Leslie speed on Hammond, vibrato depth on Harmonica, bellows pressure on Accordion
- **Aftertouch**: Drive amount on Hammond, breath pressure on Harmonica/Accordion
- **Pitch bend**: Standard pitch wheel with configurable range (1-24 semitones)

## Parameters (37 total)

| Parameter | ID | Range | Default | Notes |
|-----------|----|-------|---------|-------|
| Organ Model | `otis_organ` | 0-3 | 0 | 0=Hammond, 1=Calliope, 2=Harmonica, 3=Accordion |
| Drawbar 16' | `otis_drawbar1` | 0-1 | 0.0 | Sub-octave |
| Drawbar 5-1/3' | `otis_drawbar2` | 0-1 | 0.0 | Sub-fifth |
| Drawbar 8' | `otis_drawbar3` | 0-1 | 1.0 | Unison (fundamental) |
| Drawbar 4' | `otis_drawbar4` | 0-1 | 0.0 | Octave |
| Drawbar 2-2/3' | `otis_drawbar5` | 0-1 | 0.0 | Octave+fifth |
| Drawbar 2' | `otis_drawbar6` | 0-1 | 0.0 | Two octaves |
| Drawbar 1-3/5' | `otis_drawbar7` | 0-1 | 0.0 | Two oct+M3 |
| Drawbar 1-1/3' | `otis_drawbar8` | 0-1 | 0.0 | Two oct+fifth |
| Drawbar 1' | `otis_drawbar9` | 0-1 | 0.0 | Three octaves |
| Leslie Speed | `otis_leslie` | 0-1 | 0.5 | 0=brake, 0.5=slow, 1.0=fast |
| Key Click | `otis_keyClick` | 0-1 | 0.5 | Mechanical click level |
| Percussion Level | `otis_percussion` | 0-1 | 0.5 | Harmonic percussion |
| Perc Harmonic | `otis_percHarmonic` | 0-1 | 0.0 | 0=2nd, 1=3rd |
| Perc Decay | `otis_percDecay` | 0-1 | 0.3 | 200-500ms |
| Crosstalk | `otis_crosstalk` | 0-1 | 0.3 | Tonewheel bleed |
| Brightness | `otis_brightness` | 200-20000 | 8000 | Filter cutoff (Hz) |
| Drive | `otis_drive` | 0-1 | 0.2 | Tube overdrive |
| Filter Env Amount | `otis_filterEnvAmount` | 0-1 | 0.3 | Filter envelope depth |
| Bend Range | `otis_bendRange` | 1-24 | 2 | Pitch bend (semitones) |
| Attack | `otis_attack` | 0.001-2 | 0.005 | Amp attack (sec) |
| Decay | `otis_decay` | 0.01-5 | 0.3 | Amp decay (sec) |
| Sustain | `otis_sustain` | 0-1 | 0.8 | Amp sustain level |
| Release | `otis_release` | 0.01-5 | 0.3 | Amp release (sec) |
| Bend Amount | `otis_bendAmount` | 0-5 | 2.0 | Harmonica bend depth (st) |
| Instability | `otis_instability` | 0-1 | 0.5 | Calliope steam chaos |
| Musette | `otis_musette` | 0-1 | 0.5 | Accordion reed detune |
| Macro CHARACTER | `otis_macroCharacter` | 0-1 | 0.0 | Drive + brightness |
| Macro MOVEMENT | `otis_macroMovement` | 0-1 | 0.0 | Leslie speed |
| Macro COUPLING | `otis_macroCoupling` | 0-1 | 0.0 | Crosstalk |
| Macro SPACE | `otis_macroSpace` | 0-1 | 0.0 | Leslie depth |
| LFO1 Rate | `otis_lfo1Rate` | 0.005-20 | 0.5 | Pitch vibrato rate |
| LFO1 Depth | `otis_lfo1Depth` | 0-1 | 0.0 | Pitch vibrato depth |
| LFO1 Shape | `otis_lfo1Shape` | 0-4 | 0 | Sine/Tri/Saw/Sq/S&H |
| LFO2 Rate | `otis_lfo2Rate` | 0.005-20 | 1.0 | Filter mod rate |
| LFO2 Depth | `otis_lfo2Depth` | 0-1 | 0.0 | Filter mod depth |
| LFO2 Shape | `otis_lfo2Shape` | 0-4 | 0 | Sine/Tri/Saw/Sq/S&H |

## Doctrine Compliance

| Doctrine | Status | Implementation |
|----------|--------|----------------|
| D001 Velocity->Timbre | PASS | Velocity scales filter cutoff (0.5-1.0x), Hammond drive amount, harmonica bend depth |
| D002 Modulation | PASS | 2 LFOs, mod wheel, aftertouch, 4 macros, pitch bend |
| D003 Physics IS Synthesis | PASS | Hammond tonewheel gearing ratios, Leslie Doppler, harmonica reed bend physics, steam pressure wobble, bellows snap transient |
| D004 No Dead Params | PASS | All 37 parameters affect audio output |
| D005 Breathing | PASS | LFO rate floor 0.005 Hz (200-second cycle) |
| D006 Expression | PASS | Velocity->timbre, mod wheel (Leslie/vibrato/bellows), aftertouch (drive/breath), pitch bend |

## Coupling Compatibility

| CouplingType | As Receiver | Behavior |
|-------------|-------------|----------|
| AmpToFilter | Yes | External amplitude -> filter cutoff (+-3000 Hz) |
| LFOToPitch | Yes | External LFO -> pitch modulation (+-2 semitones) |
| AmpToPitch | Yes | External amplitude -> pitch modulation |
| AudioToFM | Yes | External audio -> frequency modulation |

## Initial Presets (10)

| Preset | Mood | Model | Character |
|--------|------|-------|-----------|
| Gospel Fire | Foundation | Hammond | Classic 888 gospel registration, fast Leslie |
| Jimmy Smith | Foundation | Hammond | Jazz organ, 3rd harmonic percussion |
| Deep Purple | Foundation | Hammond | Screaming rock, all drawbars, max drive |
| Chorale Hymn | Atmosphere | Hammond | Gentle flute stops, slow Leslie |
| Midnight Sermon | Atmosphere | Hammond | Warm bass drawbars, dark and reverent |
| Carnival Chaos | Flux | Calliope | Maximum steam instability |
| Steam Dream | Flux | Calliope | Gentle dreamy calliope |
| Crossharp Blues | Foundation | Harmonica | Raw blues with deep bends |
| Bayou Squeeze | Foundation | Accordion | Zydeco dance accordion |
| Tent Revival | Kinetic | Hammond | High-energy gospel, upper harmonics |
| Wailing Harp | Prism | Harmonica | Expressive with heavy vibrato |

## DSP Signal Flow

```
MIDI Note -> Voice Allocation (LRU, 8 voices)
  |
  v
Model Switch (0/1/2/3)
  |
  +-- Model 0: 9 drawbars -> additive tonewheel -> crosstalk -> key click -> percussion -> overdrive
  +-- Model 1: calliope pipe -> pressure wobble -> amplitude wobble
  +-- Model 2: harmonica reed -> bend envelope -> vibrato -> overblow -> breath noise
  +-- Model 3: dual reeds -> musette beating -> bellows snap -> bellows pressure
  |
  v
Filter Envelope -> CytomicSVF (LP, velocity-scaled)
  |
  v
Amp Envelope -> Per-voice output
  |
  v
Voice Mix (stereo pan spread)
  |
  v
Leslie Speaker (AM + Doppler + stereo rotation)
  |
  v
Output + Coupling Cache
```

## File Locations

- Engine: `Source/Engines/Otis/OtisEngine.h`
- Presets: `Presets/XOmnibus/{mood}/Otis_*.xometa`
- Architecture: `Docs/otis-engine-architecture.md` (this file)
