# XOuïe — Concept Brief

*Phase 0 | March 2026 | XO_OX Designs*

---

## Identity

**XO Name:** XOuïe (French: *ouïe* — the sense of hearing; also the gill slits of a fish)
**Gallery Code:** OUIE
**Accent Color:** Hammerhead Steel `#708090`
**Parameter Prefix:** `ouie_`
**Creature:** Hammerhead shark (*Sphyrna*)

**Thesis:** Duophonic synthesis — two voices, two algorithms, one predator. Brotherly love and brotherly strife at the thermocline.

**One-liner:** *Two brothers. Two algorithms. One knob decides if they harmonize or destroy each other.*

---

## XO Concept Test

- **X in the name:** X**O**uïe — the O is there
- **Starts with XO:** Yes
- **O-word:** Ouïe — French for hearing/gills. Double meaning: the shark's breathing apparatus AND the raw auditory sense of twisting a cutoff and hearing digital engines breathe
- **Character, not feature:** The hammerhead shark — wide-set eyes as two independent sensory platforms that evolved to work together despite being apart. That's duophony in biology.
- **Not a clone:** No existing duophonic engine in the gallery. First engine at dead-center polarity.

---

## The Aquatic Identity

The hammerhead shark. A predator whose most distinctive feature — the cephalofoil, that impossibly wide T-shaped head — is literally two separate sensory platforms fused into one animal. Each eye sees a different angle. Each nostril tracks a different scent gradient. The ampullae of Lorenzini on that wide head detect the faintest electrical fields from buried prey.

XOuïe lives at the **thermocline** — the invisible boundary where warm surface water meets cold deep water. The hammerhead patrols this boundary, hunting across both worlds. It's the only engine at dead-center polarity: 50% feliX, 50% Oscar. Not because it's neutral — because it's *both at once*.

The two voices are the two halves of the cephalofoil. They can work in perfect concert (LOVE — binocular vision, triangulating prey with impossible precision) or they can disagree (STRIFE — two competing signals creating interference, confusion, aggression). The HAMMER macro is the animal's signature: wide-set eyes that see both sides of the thermocline simultaneously.

---

## Polarity

**Position:** The Thermocline — where warm meets cold
**feliX-Oscar balance:** Dead center, 50/50
**Character:** Neither surface nor deep. The boundary hunter. Patrols the line between bright and dark, transient and sustain, familiar and alien.

---

## Inspiration

- **Arturia MiniFreak** — Selectable digital oscillator algorithms per voice, duophonic architecture
- **Arturia MicroFreak** — The "little brother." The brotherly dynamic between these two real instruments mirrors XOuïe's internal duality
- **Hammerhead shark biology** — Cephalofoil (wide-set dual sensors), ampullae of Lorenzini (electroreception), cartilaginous skeleton (flexibility over rigidity)

---

## DSP Architecture

```
Voice A (Brother 1)              Voice B (Brother 2)
┌──────────────────┐             ┌──────────────────┐
│  Algorithm Select │             │  Algorithm Select │
│  ┌──────────────┐│             │┌──────────────┐  │
│  │ SMOOTH:      ││             ││ SMOOTH:      │  │
│  │  VA          ││             ││  VA          │  │
│  │  Wavetable   ││             ││  Wavetable   │  │
│  │  FM          ││             ││  FM          │  │
│  │  Additive    ││             ││  Additive    │  │
│  │ ROUGH:       ││             ││ ROUGH:       │  │
│  │  Phase Dist  ││             ││  Phase Dist  │  │
│  │  Wavefolder  ││             ││  Wavefolder  │  │
│  │  Karplus-St  ││             ││  Karplus-St  │  │
│  │  Noise       ││             ││  Noise       │  │
│  └──────────────┘│             │└──────────────┘  │
│  Tune / Detune    │             │ Tune / Detune    │
│  Level / Pan      │             │ Level / Pan      │
│  Unison (1-4)     │             │ Unison (1-4)     │
└────────┬─────────┘             └────────┬─────────┘
         │                                │
         └──────────┬─────────────────────┘
                    │
          ┌─────────▼──────────────┐
          │   INTERACTION STAGE    │
          │                        │
          │   STRIFE ◄──────► LOVE │
          │   (HAMMER macro)       │
          │                        │
          │   STRIFE modes:        │
          │    Cross-FM             │
          │    Ring Modulation      │
          │    Phase Cancellation   │
          │                        │
          │   LOVE modes:          │
          │    Spectral Blend       │
          │    Harmonic Lock        │
          │    Unison Thicken       │
          └─────────┬──────────────┘
                    │
          ┌─────────▼──────────────┐
          │      DUAL FILTER       │
          │  LP / HP / BP / Notch  │
          │  Cutoff + Resonance    │
          │  Env Amount + KeyTrack │
          └─────────┬──────────────┘
                    │
          ┌─────────▼──────────────┐
          │    AMP ENVELOPE        │
          │    ADSR                │
          └─────────┬──────────────┘
                    │
          ┌─────────▼──────────────┐
          │       FX CHAIN         │
          │  Character Drive →     │
          │  Stereo Widener →      │
          │  Delay →               │
          │  Reverb                │
          └─────────┬──────────────┘
                    │
                    ▼
                 OUTPUT
```

### Algorithm Pool (8 algorithms in 2 clusters)

**Smooth Brothers** — Harmonically predictable, blend-friendly:
1. **Virtual Analog** — Classic subtractive oscillator shapes (saw, square, pulse, triangle)
2. **Wavetable** — Morphable wavetable scanning with smooth interpolation
3. **FM** — 2-operator FM with ratio/depth control, bell-like to metallic
4. **Additive** — 16-partial additive with per-partial level/detune, organ-to-glass

**Rough Brothers** — Harmonically unpredictable, conflict-prone:
5. **Phase Distortion** — Casio CZ-style waveform bending, glassy to aggressive
6. **Wavefolder** — West Coast-style wavefolding, gentle folds to serrated chaos
7. **Karplus-Strong** — Plucked string/physical modeling, organic transients
8. **Noise** — Filtered noise with color control, texture and breath

### The Interaction Stage (Signature Feature)

The HAMMER macro is a bipolar axis:

| HAMMER Position | Interaction | Sound Character |
|----------------|-------------|-----------------|
| 0.0 (Full STRIFE) | Cross-FM + Ring Mod + Phase Cancel | Aggressive, metallic, dissonant — brothers fighting |
| 0.25 | Light cross-mod, interference patterns | Tense, unstable — brothers arguing |
| 0.5 (INDEPENDENT) | No interaction, parallel output | Clean duophonic — brothers coexisting |
| 0.75 | Spectral averaging, gentle harmonic lock | Warm, sympathetic — brothers agreeing |
| 1.0 (Full LOVE) | Full spectral blend + unison thicken | Fused, massive, consonant — brothers as one |

The STRIFE side uses the same FM/ring mod primitives that already exist in XOlokun DSP.
The LOVE side uses spectral averaging and detuned unison stacking — proven techniques.

---

## Parameters (~65 canonical)

### Per-Voice (×2 = 28 total)
| Parameter | Range | Default |
|-----------|-------|---------|
| `ouie_voiceN_algorithm` | 0-7 (VA, WT, FM, Add, PD, WF, KS, Noise) | 0 (VA) / 5 (WF) |
| `ouie_voiceN_tune` | -24 to +24 semitones | 0 |
| `ouie_voiceN_fine` | -100 to +100 cents | 0 |
| `ouie_voiceN_level` | 0.0-1.0 | 1.0 |
| `ouie_voiceN_pan` | -1.0 to +1.0 | -0.3 / +0.3 |
| `ouie_voiceN_unison` | 1-4 voices | 1 |
| `ouie_voiceN_unisonDetune` | 0-50 cents | 10 |
| `ouie_voiceN_waveform` | Algorithm-dependent | 0 |
| `ouie_voiceN_color` | 0.0-1.0 (algorithm-specific timbre) | 0.5 |
| `ouie_voiceN_shape` | 0.0-1.0 (algorithm-specific mod) | 0.0 |
| `ouie_voiceN_octave` | -2 to +2 | 0 / 0 |
| `ouie_voiceN_sub` | 0.0-1.0 (sub oscillator blend) | 0.0 |
| `ouie_voiceN_drift` | 0.0-1.0 (analog-style pitch drift) | 0.05 |
| `ouie_voiceN_envDepth` | 0.0-1.0 (pitch envelope depth) | 0.0 |

### Interaction Stage (4)
| Parameter | Range | Default |
|-----------|-------|---------|
| `ouie_hammer` | 0.0-1.0 (STRIFE at 0, LOVE at 1) | 0.5 |
| `ouie_interactionDepth` | 0.0-1.0 | 0.5 |
| `ouie_crossModRatio` | 0.5-8.0 (FM ratio for STRIFE) | 1.0 |
| `ouie_blendMode` | 0-2 (frequency split / spectral / unison) | 0 |

### Filter (5)
| Parameter | Range | Default |
|-----------|-------|---------|
| `ouie_filterCutoff` | 20-20000 Hz | 8000 |
| `ouie_filterResonance` | 0.0-1.0 | 0.0 |
| `ouie_filterType` | LP12/LP24/HP/BP/Notch | LP24 |
| `ouie_filterEnvAmount` | -1.0 to +1.0 | 0.3 |
| `ouie_filterKeyTrack` | 0.0-1.0 | 0.5 |

### Envelopes (12)
| Parameter | Range | Default |
|-----------|-------|---------|
| `ouie_ampAttack` | 0.001-10s | 0.005 |
| `ouie_ampDecay` | 0.001-10s | 0.3 |
| `ouie_ampSustain` | 0.0-1.0 | 0.8 |
| `ouie_ampRelease` | 0.001-30s | 0.5 |
| `ouie_filterAttack` | 0.001-10s | 0.01 |
| `ouie_filterDecay` | 0.001-10s | 0.5 |
| `ouie_filterSustain` | 0.0-1.0 | 0.4 |
| `ouie_filterRelease` | 0.001-30s | 0.5 |
| `ouie_modAttack` | 0.001-10s | 0.01 |
| `ouie_modDecay` | 0.001-10s | 0.3 |
| `ouie_modSustain` | 0.0-1.0 | 0.5 |
| `ouie_modRelease` | 0.001-30s | 0.3 |

### LFOs (×2 = 8)
| Parameter | Range | Default |
|-----------|-------|---------|
| `ouie_lfoN_rate` | 0.01-50 Hz | 1.0 / 3.0 |
| `ouie_lfoN_depth` | 0.0-1.0 | 0.0 |
| `ouie_lfoN_shape` | Sine/Tri/Saw/Square/S&H/Random | Sine |
| `ouie_lfoN_sync` | Free/Tempo | Free |

### FX (8)
| Parameter | Range | Default |
|-----------|-------|---------|
| `ouie_driveAmount` | 0.0-1.0 | 0.0 |
| `ouie_driveType` | Soft/Hard/Tube | Soft |
| `ouie_stereoWidth` | 0.0-1.0 | 0.3 |
| `ouie_delayTime` | 0.01-2.0s | 0.375 |
| `ouie_delayFeedback` | 0.0-0.95 | 0.3 |
| `ouie_delayMix` | 0.0-1.0 | 0.15 |
| `ouie_reverbSize` | 0.0-1.0 | 0.3 |
| `ouie_reverbMix` | 0.0-1.0 | 0.15 |

### Total: ~65 canonical parameters

---

## Macros

| Macro | Name | What It Drives | Aquatic Meaning |
|-------|------|---------------|-----------------|
| M1 | **HAMMER** | Interaction stage (STRIFE↔LOVE), cross-mod depth, blend mode | The cephalofoil — two eyes seeing one world or two |
| M2 | **AMPULLAE** | Velocity sensitivity, mod depth, expression amount, LFO depth | Electroreceptors — sensing the faintest signals |
| M3 | **CARTILAGE** | Portamento time, pitch drift, algorithm morph speed, filter env | Flexible skeleton — bending without breaking |
| M4 | **CURRENT** | Reverb mix, delay mix, stereo width, filter cutoff | The thermocline current — the water around the shark |

---

## Voice Architecture

- **Polyphony:** 2 (duophonic — this IS the engine's identity)
- **Per-voice unison:** 1-4 oscillators for thickness within each voice
- **Voice allocation:** Last-note priority with configurable behavior:
  - **Split:** Voice A = low note, Voice B = high note
  - **Layer:** Both voices on every note (effectively monophonic with 2 algorithms)
  - **Duo:** True duophonic — each new note goes to the next available voice
- **Glide:** Per-voice independent portamento (CARTILAGE macro)
- **Max voices:** 2 × 4 unison = 8 oscillators max
- **CPU budget:** <8%

---

## Coupling Matrix

### As Target (receives from)
| Source Engine | Coupling Type | Musical Effect |
|-------------|---------------|----------------|
| DRIFT | EnvToMorph | Climax bloom sweeps HAMMER from love to strife — the journey destabilizes the brothers |
| ONSET | AmpToFilter | Drum hits modulate filter — rhythmic predator, strike on every beat |
| FAT | AmpToFilter | Whale mass drives filter depth — the ocean's largest creature pushing the shark |
| OPAL | AudioToFM | Grain particles FM-modulate Voice B — parasitic interference from scattered light |
| OCEANIC | EnvToMorph | Spectral separation drives algorithm morph — the thermocline bending around the shark |

### As Source (sends to)
| Target Engine | Coupling Type | Musical Effect |
|-------------|---------------|----------------|
| DUB | getSample | Duophonic output through dub echo — two brothers' argument echoing endlessly |
| OPAL | AudioToWavetable | Two-voice texture granulated into particles — predator dissolved into plankton |
| BITE | AmpToFilter | Shark's attack drives anglerfish's bite — apex predator meets ambush predator |
| OCEANIC | getSample | Two-voice output spectrally separated — the thermocline dissected into its layers |
| DRIFT | getSample | Duophonic texture as Odyssey's starting point — the hunt becomes the journey |

### Signature Couplings
- **OUIE × OCEANDEEP** — *Apex Dive* — duophonic leads over abyssal 808, the hammerhead descending to Oscar's floor
- **OUIE × OPENSKY** — *Thermal Rising* — two-voice texture erupting through the surface into euphoric shimmer
- **OUIE × OUIE** (self-coupling) — *Cephalofoil* — when a duophonic engine couples with itself, you get 4 interacting algorithms

---

## Preset Strategy (80 factory presets)

| Category | Count | Character |
|----------|-------|-----------|
| **Brotherly Love** | 15 | Harmonious duophonic textures — spectral blend, harmonic lock, warm fusion |
| **Brotherly Strife** | 15 | Aggressive cross-mod — ring mod, FM interference, dissonant tension |
| **Thermocline** | 12 | Balanced exploration — HAMMER at center, subtle interaction, clean duophony |
| **Predator** | 12 | Aggressive leads and bass — the shark hunting, sharp filter sweeps |
| **Deep Scan** | 10 | Ambient, AMPULLAE-focused — sensitive, responsive, electroreceptive |
| **Coupling Showcases** | 8 | Cross-engine duets showcasing OUIE's unique position in the column |
| **Hero Presets** | 8 | Best-of-category showpieces (included in counts above) |

### Hero Preset Concepts
1. **Cephalofoil** — VA + Wavefolder, HAMMER at 0.5, both voices panned wide. The signature sound.
2. **Apex** — FM + Phase Dist, STRIFE at max, aggressive cross-mod lead. The attack.
3. **Schooling** — Wavetable + Wavetable, LOVE at max, unison 4+4. Massive harmonic wall.
4. **Electroreception** — KS + Noise, high AMPULLAE, velocity-responsive plucked + breath texture.
5. **Cartilage** — Additive + FM, high CARTILAGE, gliding duophonic lines that bend around each other.
6. **Thermocline Drift** — VA + Additive, HAMMER modulated by LFO, slow sweep between love and strife.
7. **Gill Breath** — Noise + Wavefolder, filter LFO mimicking breathing, the ouïe itself.
8. **Brothers** — VA (Voice A, smooth brother) + Phase Dist (Voice B, rough brother), HAMMER at 0.3. Tension.

---

## Visual Identity

- **Accent:** Hammerhead Steel `#708090` — first neutral-cool tone in the gallery
- **Character:** Predatory, understated, industrial-organic
- **UI concept:** The cephalofoil shape as the central interaction display — Voice A on the left eye, Voice B on the right eye, the HAMMER macro controlling how the two halves connect
- **Unique element:** The interaction visualizer shows the STRIFE↔LOVE state — at STRIFE, waveforms collide and interfere; at LOVE, they merge and harmonize

---

## Gallery Niche

| Dimension | XOuïe's Position | Nearest Neighbor | Gap Filled |
|-----------|-----------------|------------------|------------|
| Polyphony | Duophonic (2 voices) | All others: 8-16 voice poly | First duophonic |
| Polarity | Dead center (50/50) | XOverdub (thermocline) | First true center |
| Algorithm | Selectable per voice (8 options) | Fixed per engine | First multi-algorithm |
| Accent | Neutral steel grey | All others: vibrant/saturated | First neutral tone |
| Interaction | Bipolar STRIFE↔LOVE | Coupling matrix (external) | First internal voice interaction |

---

## Technical Notes

- 8 algorithm implementations can share infrastructure with existing engines: VA from core DSP, FM from XOrbital, KS from OddfeliX, Phase Dist from XObsidian, Wavefolder from XOrigami
- Interaction stage is ~100 lines of DSP — cross-FM and ring mod are trivial, spectral blend uses existing FFT
- Duophonic voice allocation is simpler than polyphonic — no voice stealing complexity
- CPU budget <8% makes this the lightest engine in the gallery alongside OVERWORLD
- The `ouie_` prefix avoids collision with all existing prefixes

---

## Invoke Phase 1

```
/new-xo-engine phase=1 name=XOuïe identity="Duophonic synthesis — two voices, two algorithms, one predator. Brotherly love and brotherly strife at the thermocline." code=XOui
```

---

*XO_OX Designs | XOuïe — the hammerhead at the thermocline | feliX + Oscar, seen from two angles at once*
