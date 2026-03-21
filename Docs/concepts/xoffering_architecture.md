# XOffering (OFFERING) — Architecture Spec

**Engine #45** | Phase 1: Architecture | Date: 2026-03-21
**One-Line Thesis:** Psychology-driven boom bap drum synthesis — living drums that dig through imaginary crates, filtered through the psychoacoustic DNA of hip hop's global cities.

---

## Identity

| Field | Value |
|-------|-------|
| Name | XOffering |
| Short Name | OFFERING |
| Engine ID | `Offering` |
| Parameter Prefix | `ofr_` |
| Plugin Code | `XOFR` |
| Accent Color | Crate Wax Yellow `#E5B80B` |
| Max Voices | 8 (one per drum slot) |
| Architecture Type | Hybrid drum synthesis (transient + texture + collage + city processing) |
| Aquatic Mythology | TBD (run `/mythology-keeper` after build) |

---

## Sonic Thesis

> "Inspiration, not imitation."

XOffering doesn't sample records. It synthesizes the *entire signal chain of crate digging* — from the original drum hit, through the vinyl medium, through the sampler's conversion, through the chopping and layering process. The drums are generated, not played back. The city processing chains are psychoacoustic archetypes informed by real scenes, not attempts to replicate them.

**The gap it fills:** XOmnibus has 44 engines. Zero are sample-archaeology synthesizers. ONSET does analog/digital drums. OSTINATO does modal percussion. OWARE does tuned percussion. XOffering does **boom bap drums as psychological process** — the act of discovering, degrading, and flipping sounds.

**Core innovation:** Published psychological research (Berlyne, Wundt, Zajonc, Csikszentmihalyi) translated into DSP parameters. The engine has aesthetic judgment without AI.

---

## Signal Flow

```
┌─────────────────────────────────────────────────┐
│                  PER VOICE (×8)                  │
│                                                  │
│  MIDI Trigger                                    │
│      │                                           │
│      ▼                                           │
│  ┌──────────────┐                                │
│  │  TRANSIENT    │ ← Analog-modeled drum synth   │
│  │  GENERATOR    │   (kick/snare/hat/perc/tom/   │
│  │               │    rim/clap/shaker)            │
│  └──────┬───────┘                                │
│         │                                        │
│         ▼                                        │
│  ┌──────────────┐                                │
│  │  TEXTURE      │ ← Stochastic micro-texture    │
│  │  LAYER        │   (vinyl, tape, bit, room)     │
│  │               │   Controlled by DUST macro     │
│  └──────┬───────┘                                │
│         │                                        │
│         ▼                                        │
│  ┌──────────────┐                                │
│  │  COLLAGE      │ ← Layer stacking, ring mod,   │
│  │  ENGINE       │   time-stretch, chop sim       │
│  │               │   Controlled by FLIP macro     │
│  └──────┬───────┘                                │
│         │                                        │
│         ▼                                        │
│  ┌──────────────┐                                │
│  │  CITY         │ ← Psychoacoustic processing   │
│  │  PROCESSING   │   chain (5 cities V1)          │
│  │  CHAIN        │   Controlled by CITY macro     │
│  └──────┬───────┘                                │
│         │                                        │
│         ▼                                        │
│  Per-Voice Output (level, pan)                   │
└─────────┬───────────────────────────────────────┘
          │
          ▼
   ┌──────────────┐
   │  VOICE MIXER  │ ← 8 voices summed
   └──────┬───────┘
          │
          ▼
   ┌──────────────┐
   │  CURIOSITY    │ ← Berlyne curve evaluation
   │  ENGINE       │   (pattern generation + scoring)
   │               │   Controlled by DIG macro
   └──────┬───────┘
          │
          ▼
   ┌──────────────┐
   │  MASTER       │ ← Tape saturation, final EQ,
   │  CHARACTER    │   output leveling
   └──────┬───────┘
          │
          ▼
   Master Output → getSampleForCoupling()
```

---

## Parameter List (32 parameters)

### Voice Parameters (per-voice, 8 slots)

Each voice slot has a fixed drum type but the synthesis parameters shape the character.

| Parameter ID | Range | Default | Description |
|-------------|-------|---------|-------------|
| `ofr_voiceType` | 0-7 (int) | varies per slot | Drum type: 0=Kick, 1=Snare, 2=ClosedHat, 3=OpenHat, 4=Clap, 5=Rim, 6=Tom, 7=Perc |
| `ofr_voiceTune` | -24 to +24 (semitones) | 0 | Pitch offset from base tuning |
| `ofr_voiceDecay` | 0.001-2.0 (seconds) | 0.3 | Amplitude decay time |
| `ofr_voiceBody` | 0.0-1.0 | 0.5 | Tonal body vs. noise balance (0=pure noise, 1=pure tone) |
| `ofr_voiceLevel` | 0.0-1.0 | 0.8 | Per-voice output level |
| `ofr_voicePan` | -1.0 to 1.0 | 0.0 | Per-voice stereo position |

*Note: These are accessed per-voice via index. In the APVTS they register as `ofr_v0_tune`, `ofr_v0_decay`, etc. through `ofr_v7_tune`, `ofr_v7_decay`. That's 48 voice params (6 × 8). For clarity, the spec lists them once.*

### Transient Generator

| Parameter ID | Range | Default | Description |
|-------------|-------|---------|-------------|
| `ofr_transientSnap` | 0.0-1.0 | 0.5 | Transient attack sharpness (0=soft, 1=hard click) |
| `ofr_transientPitch` | 0.0-1.0 | 0.3 | Pitch envelope depth on attack (the "boom" in boom bap) |
| `ofr_transientSat` | 0.0-1.0 | 0.15 | Transient saturation amount |

### Texture Layer (DUST)

| Parameter ID | Range | Default | Description |
|-------------|-------|---------|-------------|
| `ofr_dustVinyl` | 0.0-1.0 | 0.2 | Vinyl crackle intensity (stochastic impulse noise) |
| `ofr_dustTape` | 0.0-1.0 | 0.1 | Tape hiss + saturation (pink noise + soft clip) |
| `ofr_dustBits` | 4-16 (int) | 16 | Bit depth reduction (SP-1200 = 12, MPC60 = 12, clean = 16) |
| `ofr_dustSampleRate` | 8000-48000 (Hz) | 48000 | Sample rate reduction (SP-1200 = 26040, MPC3000 = 44100) |
| `ofr_dustWobble` | 0.0-1.0 | 0.05 | Pitch/speed wobble simulating motor drift |

### Collage Engine (FLIP)

| Parameter ID | Range | Default | Description |
|-------------|-------|---------|-------------|
| `ofr_flipLayers` | 1-4 (int) | 1 | Number of stacked transient variations per trigger |
| `ofr_flipChop` | 0.0-1.0 | 0.0 | Chop simulation — rhythmic gating of the transient (0=off, 1=aggressive chop) |
| `ofr_flipStretch` | 0.5-2.0 | 1.0 | Time-stretch factor (< 1 = compressed, > 1 = stretched) |
| `ofr_flipRingMod` | 0.0-1.0 | 0.0 | Inter-layer ring modulation depth (creates metallic/crushed textures) |

### City Processing Chain

| Parameter ID | Range | Default | Description |
|-------------|-------|---------|-------------|
| `ofr_cityMode` | 0-4 (int) | 0 | City archetype: 0=NewYork, 1=Detroit, 2=LosAngeles, 3=Toronto, 4=BayArea |
| `ofr_cityBlend` | 0.0-1.0 | 0.0 | Morph between current city and next city (for continuous morphing) |
| `ofr_cityIntensity` | 0.0-1.0 | 0.5 | How much the city processing affects the sound (0=bypass, 1=full character) |

### Curiosity Engine (DIG — Berlyne Psychology)

| Parameter ID | Range | Default | Description |
|-------------|-------|---------|-------------|
| `ofr_digCuriosity` | 0.0-1.0 | 0.5 | Berlyne curve position: 0=familiar/predictable, 0.5=sweet spot, 1=alien/experimental |
| `ofr_digComplexity` | 0.0-1.0 | 0.4 | Wundt hedonic complexity: controls variation density in generated patterns |
| `ofr_digFlow` | 0.0-1.0 | 0.6 | Csikszentmihalyi flow balance: pattern predictability vs. surprise |

### Expression & Modulation

| Parameter ID | Range | Default | Description |
|-------------|-------|---------|-------------|
| `ofr_velToSnap` | 0.0-1.0 | 0.5 | Velocity → transient snap amount (D001: velocity shapes timbre) |
| `ofr_velToBody` | 0.0-1.0 | 0.3 | Velocity → body/noise balance (harder hit = more tonal) |
| `ofr_lfo1Rate` | 0.01-20.0 (Hz) | 0.067 | LFO1 rate (default = ocean breathing, Sutra III-1) |
| `ofr_lfo1Depth` | 0.0-1.0 | 0.05 | LFO1 depth → filter cutoff |
| `ofr_lfo1Shape` | 0-4 (int) | 0 | LFO1 shape: 0=Sine, 1=Tri, 2=Saw, 3=Square, 4=S&H |
| `ofr_lfo2Rate` | 0.01-20.0 (Hz) | 2.0 | LFO2 rate (default = groove pump) |
| `ofr_lfo2Depth` | 0.0-1.0 | 0.0 | LFO2 depth → amplitude (for swing/pump) |
| `ofr_aftertouch` | 0.0-1.0 | 0.3 | Aftertouch → texture intensity |
| `ofr_modWheel` | 0.0-1.0 | 0.5 | Mod wheel → curiosity drive (twist the dig deeper) |

### Master

| Parameter ID | Range | Default | Description |
|-------------|-------|---------|-------------|
| `ofr_masterLevel` | 0.0-1.0 | 0.75 | Master output level |
| `ofr_masterWidth` | 0.0-1.0 | 0.5 | Stereo width (0=mono, 1=wide) |

**Total: 32 global params + 48 per-voice params = 80 params**

---

## The Five City Processing Chains (V1)

Each city is a complete psychoacoustic processing chain applied after the collage engine.

### City 0: New York (The Archaeologist)

> *"Find beauty in degradation."* — SP-1200 grit heritage

| Stage | DSP | Parameters |
|-------|-----|-----------|
| 1. Bit Crush | 12-bit quantization at 26040 Hz (SP-1200 exact spec) | Fixed at city values when selected |
| 2. Vinyl Noise | Impulse noise model (dust hits at Poisson-distributed intervals) | Density scales with cityIntensity |
| 3. Tight Swing | Micro-timing displacement: 60-70% swing (Premier/RZA pocket) | Fixed groove template |
| 4. High-Pass | Gentle HP at 60Hz (the "thin" NYC boom bap low end) | Fixed |
| 5. Compression | Fast attack, medium release, 4:1 ratio (punchy) | Fixed |

**Psychological archetype:** The Archaeologist finds beauty in degradation. Processing emphasizes the dust, the grit, the aliasing artifacts that become the texture.

### City 1: Detroit (The Time-Bender)

> *"Time is not a grid. Time is a feeling."* — Dilla's legacy

| Stage | DSP | Parameters |
|-------|-----|-----------|
| 1. Analog Saturation | Soft-clip saturation model (warm, not harsh) | Amount scales with cityIntensity |
| 2. Drunk Timing | Per-voice micro-timing displacement: ±15ms random offset per trigger (the Dilla "drunk" feel) | Drift amount scales with cityIntensity |
| 3. Soft Transients | Transient softener: 2ms attack smoothing | Fixed |
| 4. Warm Filter | Gentle LP at 12kHz (remove digital harshness) | Fixed |
| 5. Tape Compression | Slow attack, slow release, 2:1 (glue, not punch) | Fixed |

**Psychological archetype:** The Time-Bender. Perception of time is malleable. Dilla proved you could make a grid feel human by making it imperfect. The processing introduces intentional temporal drift.

### City 2: Los Angeles (The Collage Artist)

> *"Identity is assembled from fragments."* — Madlib's crate philosophy

| Stage | DSP | Parameters |
|-------|-----|-----------|
| 1. Heavy Compression | Fast attack, fast release, 6:1 ratio (squashed, pumping) | Amount scales with cityIntensity |
| 2. Tape Saturation | Heavier tape model than Detroit (more color, more harmonics) | Fixed |
| 3. Psychedelic Pitch | Random pitch micro-drift ±5 cents per trigger (records on warped vinyl) | Drift scales with cityIntensity |
| 4. Layer Boost | Collage layer count +1 (LA aesthetic = more layers) | Fixed |
| 5. Low-End Warmth | Gentle bass shelf boost at 80Hz | Fixed |

**Psychological archetype:** The Collage Artist. Gestalt psychology — the whole is different from the sum of its parts. Processing emphasizes layering, transformation, and the psychedelic quality of pitch instability.

### City 3: Toronto (The Architect)

> *"Structure reveals beauty."* — Bauhaus precision meets boom bap

| Stage | DSP | Parameters |
|-------|-----|-----------|
| 1. Clean Sub | Sub-harmonic generator adds clean fundamental at -12dB | Amount scales with cityIntensity |
| 2. Tight High-Pass | HP at 40Hz (remove mud, keep clean low end) | Fixed |
| 3. Precision Transient | Transient sharpener: enhances attack clarity | Amount scales with cityIntensity |
| 4. Minimal Noise | Vinyl/tape noise reduced by 50% vs. other cities | Fixed |
| 5. Clean Compression | Medium attack, medium release, 3:1 (transparent punch) | Fixed |

**Psychological archetype:** The Architect. Bauhaus — form follows function. Processing emphasizes clarity, precision, and controlled power. Less dust, more punch.

### City 4: Bay Area (The Alchemist)

> *"Transform base material into gold."* — Dark sample alchemy

| Stage | DSP | Parameters |
|-------|-----|-----------|
| 1. Dark Filter | LP at 8kHz (roll off brightness, create fog) | Cutoff scales with cityIntensity |
| 2. Convolution Fog | Small room impulse response (subtle, dark spatial character) | Wet amount scales with cityIntensity |
| 3. Shimmer Degradation | Subtle chorus at very slow rate (0.1Hz) creating lo-fi shimmer | Depth scales with cityIntensity |
| 4. Tape Stop | Occasional micro tape-stop artifact (subtle pitch drop) | Probability scales with cityIntensity |
| 5. Dark Compression | Slow attack, slow release, 3:1 (dark glue) | Fixed |

**Psychological archetype:** The Alchemist. Transformation of base material into something precious. Processing emphasizes darkness, fog, and the slow reveal of hidden beauty.

---

## Macro Mapping

| Macro | Name | Primary Target | Secondary Targets |
|-------|------|---------------|-------------------|
| M1 | **DIG** | `ofr_digCuriosity` (0→1 = familiar→alien) | Also scales `ofr_digComplexity` at 50% depth |
| M2 | **CITY** | `ofr_cityMode` + `ofr_cityBlend` (morphs through cities) | Also adjusts `ofr_cityIntensity` slightly |
| M3 | **FLIP** | `ofr_flipLayers` + `ofr_flipChop` + `ofr_flipRingMod` | Collage intensity as unified gesture |
| M4 | **DUST** | `ofr_dustVinyl` + `ofr_dustTape` + `ofr_dustBits` | Degradation/patina as unified gesture |

**Macro behavior verification (D004):**
- M1 (DIG) at 0: predictable, classic boom bap. At 1: experimental, alien drum mutations. **Clear timbral change.**
- M2 (CITY) at 0: New York grit. At 1: Bay Area fog. **Clear character shift.**
- M3 (FLIP) at 0: single clean hit. At 1: layered, chopped, ring-modded collage. **Clear complexity change.**
- M4 (DUST) at 0: clean digital. At 1: buried in vinyl crackle and tape hiss. **Clear texture change.**

---

## Coupling Interface

### What XOffering Sends (getSampleForCoupling)
Returns the master output (post-city-processing, pre-master-level). This means coupled engines receive the full characterized drum sound including city DNA.

### What XOffering Receives

| CouplingType | Effect |
|-------------|--------|
| `AmpToFilter` | External amplitude modulates the city processing intensity |
| `AmpToChoke` | External amplitude chokes drum voices (sidechain duck) |
| `RhythmToBlend` | External rhythm pattern modulates the FLIP amount (drums become more collaged on beat) |
| `EnvToDecay` | External envelope modulates voice decay times |
| `AudioToFM` | External audio FM-modulates the transient generator (creates alien drum timbres) |
| `KnotTopology` | Bidirectional entanglement (XOffering drums ↔ partner engine) |

### Best Coupling Partners

| Partner | CouplingType | Why |
|---------|-------------|-----|
| OSTINATO | `RhythmToBlend` | Cultural ground truth (OSTINATO patterns) drives collage intensity. Tradition meets exploration. |
| ONSET | `AmpToChoke` | ONSET's circuit drums sidechain-duck XOffering's boom bap. Layer both drum engines. |
| OXBOW | `KnotTopology` | Entangled reverb creates spatial boom bap. XOffering's dry drums ↔ OXBOW's wet cathedral. |
| OPAL | `AudioToFM` | Granular textures FM-modulate drum transients. Creates otherworldly boom bap. |
| OPENSKY | `AmpToFilter` | OPENSKY's shimmer controlled by XOffering's drum dynamics. Boom bap drives euphoria. |

---

## BAKE → XPN Pipeline

XOffering is designed backwards from the MPC. The BAKE function renders discoveries into static XPN drum kits.

### BAKE Process
1. **Generate**: Use current parameter state to synthesize a full kit (8 drum slots)
2. **Render**: Each slot rendered as WAV at 4 velocity layers (vel 30, 64, 100, 127)
3. **Variation**: Each velocity layer gets 4 round-robin variations (re-run stochastic texture layer)
4. **Total**: 8 slots × 4 velocities × 4 round-robins = 128 WAV files per kit
5. **Map**: Auto-generate XPM drum program mapping (16-pad layout, velocity zones)
6. **Package**: Bundle as XPN-ready directory structure

### XPM Rules (Critical)
- `KeyTrack` = `True` (samples transpose across zones)
- `RootNote` = `0` (MPC auto-detect convention)
- Empty layer `VelStart` = `0` (prevents ghost triggering)

### City Metadata in XPN
Each baked kit carries its city identity:
```xml
<City>Detroit</City>
<Psychology>TimeBender</Psychology>
<Curiosity>0.55</Curiosity>
```
This metadata lets the MPC producer know the DNA of the kit they're loading.

---

## Doctrine Compliance (Pre-Build Verification)

| Doctrine | How XOffering Satisfies |
|----------|----------------------|
| D001: Velocity → Timbre | `ofr_velToSnap` + `ofr_velToBody`: velocity changes transient sharpness AND body/noise balance. Not just volume. |
| D002: Modulation is Lifeblood | 2 LFOs (filter + amplitude), 4 macros (DIG/CITY/FLIP/DUST), mod wheel → curiosity, aftertouch → texture. 80 params. |
| D003: Physics IS Synthesis | Psychology IS generation. Published citations: Berlyne (1960), Wundt (1874), Zajonc (1968), Csikszentmihalyi (1975). |
| D004: No Dead Parameters | Every param audibly affects output. City modes are complete processing chains, not labels. |
| D005: Engine Breathes | LFO1 default rate = 0.067 Hz (ocean breathing, Sutra III-1). Autonomous filter modulation. |
| D006: Expression Not Optional | Velocity → snap + body. Aftertouch → texture intensity. Mod wheel → curiosity drive. |

---

## Psychological Citations

| Theory | Author | Year | Engine Implementation |
|--------|--------|------|----------------------|
| Aesthetic Curiosity (Hedonic Gap) | Daniel Berlyne | 1960 | `ofr_digCuriosity`: the distance between expected and actual stimulus. Moderate novelty = maximum pleasure. |
| Hedonic Curve | Wilhelm Wundt | 1874 | `ofr_digComplexity`: pleasure as a function of stimulus intensity. Too simple = boredom, too complex = anxiety. |
| Mere Exposure Effect | Robert Zajonc | 1968 | V2: Familiarity accumulator — engine reinforces elements the producer doesn't change. |
| Flow State | Mihaly Csikszentmihalyi | 1975 | `ofr_digFlow`: balance between pattern challenge and predictability. When matched = groove. |

---

## V1 vs V2 Scope

### V1 (Build Now)
- 8 drum voices with full transient + texture + collage + city signal chain
- 5 city processing chains (NY, Detroit, LA, Toronto, Bay Area)
- 4 macros: DIG, CITY, FLIP, DUST
- 32 global + 48 per-voice = 80 parameters
- Berlyne curiosity + Wundt complexity + flow state as params
- LFO1/LFO2, velocity mapping, aftertouch, mod wheel
- Manual triggering (producer plays the drums)
- BAKE function for XPN export
- 150+ presets (target)

### V2 (Future — Global Boroughs + Intelligence)
- 5 international city additions: London, Tokyo, Berlin, Sao Paulo, Seoul
- Parent→Dialect morphing (2D city space)
- Cultural psychology DSP: mono no aware, gambiarra, Sachlichkeit, make-do-and-mend, ppalli ppalli
- Mere Exposure accumulator (engine learns from producer interaction)
- Autonomous pattern generation (generative digging)
- BAKE → XPN pipeline with velocity layers + round-robin
- Academic paper: "Psychology-Driven Drum Synthesis"

---

## File Structure (Standalone → XOmnibus)

```
~/Documents/GitHub/XOffering/
├── Source/
│   ├── DSP/
│   │   ├── TransientGenerator.h     — Per-type drum synthesis (kick/snare/hat/etc.)
│   │   ├── TextureLayer.h           — Vinyl/tape/bit/room micro-texture
│   │   ├── CollageEngine.h          — Layer stacking, chop, stretch, ring mod
│   │   ├── CityProcessor.h          — 5 city processing chains
│   │   ├── CuriosityEngine.h        — Berlyne/Wundt/Flow DSP
│   │   ├── OfferingVoice.h          — Single drum voice (transient→texture→collage→city)
│   │   └── OfferingEngine.h         — Master engine (8 voices + curiosity + master)
│   ├── Adapter/
│   │   └── OfferingAdapter.h        — SynthEngine interface wrapper
│   └── UI/
│       └── OfferingEditor.h         — Standalone UI
├── Presets/
│   └── *.xometa                     — Factory presets
├── CLAUDE.md                        — Project guide
├── CMakeLists.txt
└── Docs/
    └── xomnibus_integration_spec.md
```

---

## Accent Color Rationale

**Crate Wax Yellow `#E5B80B`** — the color of old vinyl center labels (Motown, Def Jam, Stones Throw, Fat Beats). Not gold (that's XO Gold `#E9C46A`). Not amber (that's OBLONG `#E9A84A`). This is the specific yellow of a Technics needle dropping onto a 1974 Brazilian jazz record at 3am in a basement in Queens. It's the color of discovery.

---

## Next Steps

1. **Seance** — Ghost council review of the architecture before build (`/synth-seance`)
2. **Mythology** — Aquatic identity assignment (`/mythology-keeper`)
3. **Scaffold** — `/new-xo-project` with name=XOffering, code=XOFR
4. **Build** — Phase 2: TransientGenerator → TextureLayer → CollageEngine → CityProcessor → CuriosityEngine → Voice → Master
5. **Presets** — `/exo-meta` for initial 150 presets, then `/preset-audit-checklist` for 9.0+ quality gate
6. **Retreat** — `/guru-bin` refinement on the V1 presets
7. **Integrate** — Phase 3-4: Adapter → XOmnibus registration → engine #45
