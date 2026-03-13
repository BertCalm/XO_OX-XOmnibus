# XOcelot — Phase 1 Architecture Specification

**Engine:** XOcelot (Ecosystem Synthesis)
**Short Name:** OCELOT
**Engine ID:** `"Ocelot"`
**Parameter Prefix:** `ocelot_`
**Accent Color:** Ocelot Tawny `#C5832B`
**Max Voices:** 8
**CPU Budget:** <13% single-engine
**Date:** March 2026

---

## 1. Product Identity

**Thesis:** "XOcelot is a canopy-layered sample-mangling synthesizer that builds living soundscapes from four interacting strata — percussion floor, chopped understory, spectral canopy, and emergent creature calls — all cross-feeding through a typed ecosystem matrix."

**Sound family:** Texture / Groove / Organic Hybrid

**Unique capability:** Four-strata ecosystem synthesis where layers cross-pollinate in real time through a typed modulation matrix. The Floor drives rhythm into the Canopy. The Canopy filters down into the Understory. Creature calls emerge from spectral analysis of the combined output. The ecosystem IS the sound — not layering, but feedback between synthesis dimensions. Biome transformations (Jungle / Underwater / Winter) alter every stratum's character simultaneously.

**Personality in 3 words:** Lush, Prowling, Hypnotic

**Gallery gap filled:** No XOmnibus engine models *interaction between internal synthesis layers* as the primary sound-shaping mechanism. All existing engines synthesize along a single dimension (harmonic, granular, temporal, metabolic). OCELOT introduces strata, typed cross-feed, and emergent behavior as musical parameters.

---

## 2. Signal Flow Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│                       XOcelot Voice (×8)                            │
│                                                                     │
│  MIDI Note ──► Note/Vel/Gate                                        │
│                                                                     │
│  ┌──────────────────────────────────────────────────────────────┐   │
│  │  FLOOR — Physical-Modeled Percussion                         │   │
│  │                                                              │   │
│  │  Model select (berimbau/cuica/agogo/kalimba/pandeiro/log)    │   │
│  │  Tension ──► pitch character                                 │   │
│  │  Strike ──► brightness / contact point                       │   │
│  │  Damping ──► resonance decay                                 │   │
│  │  Pattern ──► rhythmic trigger grid (16 patterns)             │   │
│  │  [Biome: transforms model timbres — Underwater=submerged,    │   │
│  │   Winter=crystalline/fragile]                                │   │
│  └──────────────────┬───────────────────────────────────────────┘   │
│                     │ amplitude + timbre signals                    │
│  ┌──────────────────▼───────────────────────────────────────────┐   │
│  │  UNDERSTORY — Sample Mangler + Grain Chopper                 │   │
│  │                                                              │   │
│  │  Source: internal osc mix OR coupling bus (ocelot_couplLvl) │   │
│  │  Chop engine: 4-8 grains, rhythmic resequencing + swing      │   │
│  │  Bit-crush: 4-16 bit (12-bit = SP-1200 sweet spot)           │   │
│  │  Sample rate: 4kHz-44.1kHz reduction                         │   │
│  │  Tape warp: Mellotron flutter + wow + dropout                │   │
│  │  Dust: vinyl crackle responding to amplitude                 │   │
│  │  [Biome: Underwater=slow warp+blur, Winter=cold artifact]    │   │
│  └──────────────────┬───────────────────────────────────────────┘   │
│                     │ energy + pitch signals                        │
│  ┌──────────────────▼───────────────────────────────────────────┐   │
│  │  CANOPY — Spectral Additive Pad Synth                        │   │
│  │                                                              │   │
│  │  Complex oscillator with Buchla-style wavefold               │   │
│  │  8 partials: individual level + detune                       │   │
│  │  Spectral filter: frequency-domain filter (Floor-responsive) │   │
│  │  Shimmer: pitch-shifted (+1 oct) harmonic feedback           │   │
│  │  Breathe: slow amp mod (wind through canopy)                 │   │
│  │  [Biome: Underwater=deep dark rolling, Winter=sparse+wide]   │   │
│  └──────────────────┬───────────────────────────────────────────┘   │
│                     │ spectral peaks + amplitude signals            │
│  ┌──────────────────▼───────────────────────────────────────────┐   │
│  │  EMERGENT — Formant Creature Call Generator                  │   │
│  │                                                              │   │
│  │  Vocal tract model: 3 formant filters (F1/F2/F3)             │   │
│  │  Call patterns: rhythmic trigger firing formant sweeps       │   │
│  │  Species: bird trill / primate howl / insect drone / frog    │   │
│  │  Trigger source: MIDI / Floor amplitude / Canopy peaks       │   │
│  │  [Biome: Underwater=whale+dolphin+bubble, Winter=wolf+bird]  │   │
│  └──────────────────┬───────────────────────────────────────────┘   │
│                     │                                               │
│  ┌──────────────────▼───────────────────────────────────────────┐   │
│  │  ECOSYSTEM MIXER                                             │   │
│  │                                                              │   │
│  │  4-channel strata mix (Floor/Understory/Canopy/Emergent)     │   │
│  │  strataBalance: morphs between lower and upper pair          │   │
│  │  Stereo spread per stratum                                   │   │
│  └──────────────────┬───────────────────────────────────────────┘   │
│                     │                                               │
│  ┌──────────────────▼───────────────────────────────────────────┐   │
│  │  HUMIDITY STAGE                                              │   │
│  │                                                              │   │
│  │  Tape saturation (warm, asymmetric)                          │   │
│  │  Analog warmth (subtle HF roll-off)                          │   │
│  │  humidity=0: clean   humidity=1: thick, saturated            │   │
│  │  [Biome: Underwater="depth" (LP+chorus), Winter="frost"]     │   │
│  └──────────────────┬───────────────────────────────────────────┘   │
│                     │                                               │
│  ┌──────────────────▼───────────────────────────────────────────┐   │
│  │  AMP ENVELOPE (ADSR)                                         │   │
│  │                                                              │   │
│  │  Attack 1ms-8s / Decay 50ms-4s / Sustain 0-1 / Rel 50ms-8s  │   │
│  └──────────────────┬───────────────────────────────────────────┘   │
│                     │                                               │
│                     └──► Coupling cache (post-humidity, pre-FX)    │
└─────────────────────────────────────────────────────────────────────┘
                                 │ (voice sum, stereo)
┌────────────────────────────────▼────────────────────────────────────┐
│  FX CHAIN (shared, post-voice-sum)                                  │
│                                                                     │
│  Canopy Reverb ── dense diffuse (LushReverb.h) ── size + pre-delay  │
│  Tape Delay ────── DubDelay.h ── time + feedback + HF roll-off      │
│  Vinyl Hiss ─────── noise floor, amplitude-scaled                   │
└─────────────────────────────────────────────────────────────────────┘
```

---

## 3. The Ecosystem Cross-Feed Matrix

The defining architectural feature. A 4×4 matrix with 12 active routes, each typed at compile time. Users see only a single bipolar amount knob per route (-1.0 to +1.0).

### Route Type Taxonomy

| Type | Routes | Baked curve | Amount controls | Negative amount |
|------|--------|-------------|-----------------|-----------------|
| **Continuous** | filter, morph, damp, formant, pitch, grain pos, shimmer, swing | Linear scalar | Depth of influence | Inverts polarity |
| **Threshold** | trigger, accent | Sigmoid (soft knee, k=8) | Threshold sensitivity (closer to 0 = fires more easily) | Inverse threshold: silence triggers |
| **Rhythmic** | chop rate, scatter | Stepped/quantized | Intensity of shift (range of chop division swept) | Rhythmic opposition (sparse when dense) |

### Route Table (frozen)

```
              TO:
              Floor         Understory      Canopy          Emergent
FROM:
Floor         —             CHOP RATE       FILTER          TRIGGER
                            [Rhythmic]      [Continuous]    [Threshold]

Understory    SWING         —               MORPH           PITCH
              [Continuous]                  [Continuous]    [Continuous]

Canopy        DAMP          GRAIN POS       —               FORMANT
              [Continuous]  [Continuous]                    [Continuous]

Emergent      ACCENT        SCATTER         SHIMMER         —
              [Threshold]   [Rhythmic]      [Continuous]
```

### Route Semantics

| Parameter | What source sends | What destination receives | Musical result |
|-----------|------------------|--------------------------|----------------|
| `ocelot_xf_floorUnder` | Floor amplitude (0-1) | Chop rate division shift | Louder hit = faster chop rhythm |
| `ocelot_xf_floorCanopy` | Floor amplitude (0-1) | Spectral filter frequency | Percussion opens canopy brightness |
| `ocelot_xf_floorEmerg` | Floor amplitude (0-1) | Creature call trigger gate | Hard hit fires a creature call |
| `ocelot_xf_underFloor` | Understory energy (0-1) | Floor timing humanization | Chop density adds swing to percussion |
| `ocelot_xf_underCanopy` | Understory grain energy | Canopy wavefold depth | Dusty chop makes the pad fold harder |
| `ocelot_xf_underEmerg` | Understory pitch centroid | Creature base pitch offset | SP-1200 formant tunes the creature |
| `ocelot_xf_canopyFloor` | Canopy spectral centroid | Floor damping time | Pad brightness controls percussion ring |
| `ocelot_xf_canopyUnder` | Canopy amplitude | Understory grain read position | Pad swell shifts which section gets chopped |
| `ocelot_xf_canopyEmerg` | Canopy spectral peaks | Formant filter tuning | Pad harmonics tune creature calls |
| `ocelot_xf_emergFloor` | Emergent call amplitude | Floor next-hit accent level | Creature call accents the next drum hit |
| `ocelot_xf_emergUnder` | Emergent pattern rhythm | Understory scatter timing | Creature rhythm patterns drive the chop |
| `ocelot_xf_emergCanopy` | Emergent amplitude | Canopy shimmer depth | Calls animate the shimmer feedback |

**Negative amount semantic:**
- Continuous routes: inverts polarity (loud Floor → closes Canopy filter, like animals going quiet)
- Threshold routes: inverse trigger (silence fires the call, not amplitude peak)
- Rhythmic routes: opposition (dense source = sparse destination rhythm)

---

## 4. Biome System Architecture

`ocelot_biome` is a 3-position enum (0=Jungle, 1=Underwater, 2=Winter). When it changes, a 200ms parameter crossfade interpolates all biome-dependent values. The crossfade operates on the *biome character tables* — not the audio buffer — so it is zero-cost on the audio thread at steady state.

### Biome Character Tables (compile-time constants, per stratum)

Each stratum has a `BiomeProfile` struct with timbral constants that modulate at biome transition:

```cpp
struct BiomeProfile {
    float floorDampingBase;     // base damping shift
    float floorBrightnessBase;  // EQ tilt on Floor output
    float understoryWobbleBase; // tape wobble intensity
    float understoryBitDepthBase; // bit depth floor offset
    float canopyPartialBalance; // high vs low partial tilt
    float canopyBreathePeriod;  // breathe LFO period (seconds)
    float emergentPitchRange;   // creature pitch sweep range
    float emergentDecayBase;    // call tail length
    float humidityCharacter;    // saturation curve shape (warm/depth/frost)
    float reverbPreDelay;       // reverb pre-delay (ms)
    float reverbDiffusion;      // reverb density
};
```

Biome transitions: linear crossfade on BiomeProfile values over 200ms. Implemented as a `BiomeMorph` object per voice that holds `current` and `target` profiles and interpolates via `biomePhase` (0-1).

---

## 5. Frozen Parameter List — 62 Parameters

### Global (6)
| ID | Type | Range | Description |
|----|------|-------|-------------|
| `ocelot_biome` | Int | 0–2 | Biome: Jungle / Underwater / Winter |
| `ocelot_strataBalance` | Float | 0–1 | Floor+Understory (0) ↔ Canopy+Emergent (1) |
| `ocelot_ecosystemDepth` | Float | 0–1 | Master cross-feed intensity |
| `ocelot_humidity` | Float | 0–1 | Saturation / tape warmth / depth |
| `ocelot_swing` | Float | 0–1 | Global rhythmic humanization |
| `ocelot_density` | Float | 0–1 | Overall layer activity level |

### Cross-Feed Matrix (12)
| ID | Type | Range | Route |
|----|------|-------|-------|
| `ocelot_xf_floorUnder` | Float | −1–1 | Floor → Understory chop rate [Rhythmic] |
| `ocelot_xf_floorCanopy` | Float | −1–1 | Floor → Canopy filter [Continuous] |
| `ocelot_xf_floorEmerg` | Float | −1–1 | Floor → Emergent trigger [Threshold] |
| `ocelot_xf_underFloor` | Float | −1–1 | Understory → Floor swing [Continuous] |
| `ocelot_xf_underCanopy` | Float | −1–1 | Understory → Canopy morph [Continuous] |
| `ocelot_xf_underEmerg` | Float | −1–1 | Understory → Emergent pitch [Continuous] |
| `ocelot_xf_canopyFloor` | Float | −1–1 | Canopy → Floor damp [Continuous] |
| `ocelot_xf_canopyUnder` | Float | −1–1 | Canopy → Understory grain pos [Continuous] |
| `ocelot_xf_canopyEmerg` | Float | −1–1 | Canopy → Emergent formant [Continuous] |
| `ocelot_xf_emergFloor` | Float | −1–1 | Emergent → Floor accent [Threshold] |
| `ocelot_xf_emergUnder` | Float | −1–1 | Emergent → Understory scatter [Rhythmic] |
| `ocelot_xf_emergCanopy` | Float | −1–1 | Emergent → Canopy shimmer [Continuous] |

### Floor (8)
| ID | Type | Range | Description |
|----|------|-------|-------------|
| `ocelot_floorModel` | Int | 0–5 | berimbau / cuica / agogo / kalimba / pandeiro / log drum |
| `ocelot_floorTension` | Float | 0–1 | String/skin tension (pitch character) |
| `ocelot_floorStrike` | Float | 0–1 | Strike position (brightness) |
| `ocelot_floorDamping` | Float | 0–1 | Resonance decay time |
| `ocelot_floorPattern` | Int | 0–15 | Rhythmic pattern selector |
| `ocelot_floorLevel` | Float | 0–1 | Floor stratum output level |
| `ocelot_floorPitch` | Float | 0–1 | Pitch offset for tuned models (kalimba/agogo) |
| `ocelot_floorVelocity` | Float | 0–1 | Velocity sensitivity |

### Understory (9)
| ID | Type | Range | Description |
|----|------|-------|-------------|
| `ocelot_chopRate` | Int | 1–32 | Chop divisions (tempo-synced) |
| `ocelot_chopSwing` | Float | 0–1 | Chop timing humanization |
| `ocelot_bitDepth` | Float | 4–16 | Bit depth (12 = SP-1200) |
| `ocelot_sampleRate` | Float | 4000–44100 | Sample rate reduction (Hz) |
| `ocelot_tapeWobble` | Float | 0–1 | Mellotron flutter + wow |
| `ocelot_tapeAge` | Float | 0–1 | Tape degradation (hiss, dropout, saturation) |
| `ocelot_dustLevel` | Float | 0–1 | Vinyl / dust noise amount |
| `ocelot_understoryLevel` | Float | 0–1 | Understory stratum output level |
| `ocelot_understorySrc` | Float | 0–1 | 0=internal osc, 1=coupling bus (blend) |

### Canopy (8)
| ID | Type | Range | Description |
|----|------|-------|-------------|
| `ocelot_canopyWavefold` | Float | 0–1 | Buchla-style wavefolder depth |
| `ocelot_canopyPartials` | Int | 1–8 | Active harmonic count |
| `ocelot_canopyDetune` | Float | 0–1 | Inter-partial detuning (shimmer spread) |
| `ocelot_canopySpectralFilter` | Float | 0–1 | Spectral filter position (normalized 20Hz–20kHz) |
| `ocelot_canopyBreathe` | Float | 0–1 | Wind-like amplitude modulation rate |
| `ocelot_canopyShimmer` | Float | 0–1 | Pitch-shifted (+1oct) harmonic feedback level |
| `ocelot_canopyLevel` | Float | 0–1 | Canopy stratum output level |
| `ocelot_canopyPitch` | Float | 0–1 | Fine tune relative to note (±24 cents) |

### Emergent (8)
| ID | Type | Range | Description |
|----|------|-------|-------------|
| `ocelot_creatureType` | Int | 0–3 | bird trill / primate howl / insect drone / frog chirp |
| `ocelot_creatureRate` | Float | 0–1 | Call frequency (normalized → 0.1–20Hz or tempo-synced) |
| `ocelot_creaturePitch` | Float | 0–1 | Base formant pitch |
| `ocelot_creatureSpread` | Float | 0–1 | Formant bandwidth (narrow=focused, wide=breathy) |
| `ocelot_creatureTrigger` | Int | 0–2 | Source: MIDI / Floor amplitude / Canopy peaks |
| `ocelot_creatureLevel` | Float | 0–1 | Emergent stratum output level |
| `ocelot_creatureAttack` | Float | 0–1 | Formant sweep attack speed |
| `ocelot_creatureDecay` | Float | 0–1 | Call tail length |

### FX (5)
| ID | Type | Range | Description |
|----|------|-------|-------------|
| `ocelot_reverbSize` | Float | 0–1 | Canopy reverb room size (dense, diffuse) |
| `ocelot_reverbMix` | Float | 0–1 | Reverb wet level |
| `ocelot_delayTime` | Float | 0–1 | Tape delay time (tempo-syncable) |
| `ocelot_delayFeedback` | Float | 0–1 | Delay feedback with HF roll-off |
| `ocelot_delayMix` | Float | 0–1 | Delay wet level |

### Amp Envelope (4)
| ID | Type | Range | Description |
|----|------|-------|-------------|
| `ocelot_ampAttack` | Float | 0.001–8s | Attack time |
| `ocelot_ampDecay` | Float | 0.05–4s | Decay time |
| `ocelot_ampSustain` | Float | 0–1 | Sustain level |
| `ocelot_ampRelease` | Float | 0.05–8s | Release time |

### Coupling Input (2)
| ID | Type | Range | Description |
|----|------|-------|-------------|
| `ocelot_couplingLevel` | Float | 0–1 | External audio blend into Understory chop buffer |
| `ocelot_couplingBus` | Int | 0–3 | Which stratum coupling audio targets (default: Understory) |

**Total: 62 parameters** — all frozen with `ocelot_` prefix.

---

## 6. Voice Architecture

- **Max voices:** 8
- **Voice stealing:** Quietest active voice (ecosystem textures benefit from graceful fadeout, not abrupt cut)
- **Legato:** Yes — for Canopy pad lines and Floor percussion rolls. Legato re-triggers Floor pattern without full voice reset.
- **Mono mode:** Available (for Floor-focused bass grooves where single-note playing is intentional)
- **Note-to-voice mapping:** Each MIDI note activates one voice. All 4 strata fire simultaneously per voice. The Emergent layer's trigger source (`ocelot_creatureTrigger`) determines whether creature calls are per-MIDI-note or amplitude-driven.

---

## 7. DSP Modules Required

### New (XOcelot-specific)
| Module | Location | Description |
|--------|----------|-------------|
| `OcelotFloor.h` | engine/Floor/ | Physical model switcher — 6 Karplus-Strong / resonator variants |
| `OcelotUnderstory.h` | engine/Understory/ | Grain chopper + bit-crush + tape warp pipeline |
| `OcelotCanopy.h` | engine/Canopy/ | Additive osc (8 partials) + wavefold + shimmer |
| `OcelotEmergent.h` | engine/Emergent/ | 3-formant vocal tract + call pattern generator |
| `EcosystemMatrix.h` | engine/ | Cross-feed router — typed dispatch (12 routes) |
| `BiomeMorph.h` | engine/ | 200ms parameter crossfade for biome transitions |
| `OcelotVoice.h` | engine/ | Voice containing all 4 strata + matrix + biome |
| `OcelotVoicePool.h` | engine/ | 8-voice pool with quietest-stealing |
| `OcelotParamSnapshot.h` | engine/ | Cached `std::atomic<float>*` reads, once per block |

### Reused from `Source/DSP/`
| Module | Used for |
|--------|---------|
| `Saturator.h` | Humidity stage saturation |
| `LushReverb.h` | Canopy reverb (FX chain) |
| `DubDelay.h` | Tape delay (FX chain) |
| `FastMath.h` | Fast tanh / wavefold approximations |

---

## 8. CPU Budget Analysis

| Component | Per-voice estimate | 8-voice total |
|-----------|-------------------|---------------|
| Floor (resonator model, active note) | ~0.10% | ~0.80% |
| Understory (4-8 grains + bit-crush + tape) | ~0.35% | ~2.80% |
| Canopy (8-partial additive + wavefold + shimmer) | ~0.50% | ~4.00% |
| Emergent (3-formant + pattern trigger) | ~0.12% | ~0.96% |
| EcosystemMatrix (12 mod routes, per-block math) | ~0.05% | 0.05% (shared) |
| BiomeMorph (active transition only) | ~0.01% | 0.01% |
| Amp envelope + voice overhead | ~0.05% | ~0.40% |
| **Voice subtotal** | **~1.13%** | **~9.02%** |
| FX (LushReverb + DubDelay + hiss) | — | ~2.50% |
| **Grand total (worst case, all 8 voices + FX)** | — | **~11.5%** |

✅ Within <13% budget. Dual-engine pairing (e.g., OCELOT + OVERDUB) projects to ~13-15% — within the <28% dual-engine envelope.

**CPU risk areas:**
- Canopy shimmer feedback path: clamp feedback coefficient <0.9 to prevent runaway
- Understory grain count: cap at 8 simultaneous grains per voice (not OPAL's full 32-grain cloud)
- 8 voices all active simultaneously: rare in practice; 4-5 voices typical in use

---

## 9. Macro Mapping (M1–M4)

| Macro | Label | Target Parameters | Behavior |
|-------|-------|------------------|----------|
| M1 | **PROWL** | `ocelot_strataBalance` (primary) + `ocelot_floorModel` crossfade + `ocelot_humidity` | Character dial. Low = floor-heavy percussive groove. Mid = balanced ecosystem. High = canopy-dominant lush pads with creature calls floating above. The ocelot moving from forest floor to treetops. |
| M2 | **FOLIAGE** | `ocelot_density` + `ocelot_canopyPartials` + `ocelot_chopRate` + `ocelot_creatureRate` | Movement dial. Low = sparse, minimal, slow breathing. High = dense, rapid, teeming with activity. How thick is the jungle. |
| M3 | **ECOSYSTEM** | `ocelot_ecosystemDepth` (primary) + `ocelot_couplingLevel` (when coupled) | Interaction dial. Low = independent layers, clean separation. High = full cross-feed, layers driving each other. When coupled externally, also scales coupling amount. M3 is BOTH internal and external coupling in one gesture. |
| M4 | **CANOPY** | `ocelot_reverbSize` + `ocelot_reverbMix` + `ocelot_delayMix` + `ocelot_canopyShimmer` | Space dial. Low = intimate, close, dry. High = vast overhead space, shimmer, echoes between trees. How high is the canopy. |

All 4 macros produce audible, significant change at every point in their range in every preset. Gate requirement: verified before proceeding to Phase 2.

---

## 10. XOmnibus Coupling Interface

### As Coupling Source

`getSampleForCoupling()` returns: **post-humidity, pre-FX mono mix** (all four strata blended through ecosystem mixer and humidity stage, before reverb/delay). Stereo — left channel = `channel 0`, right = `channel 1`. Normalized ±1.

**Musical value of this output:** Ecosystem output captures the rhythmic + textural + spectral blend of the full jungle. The berimbau attacks, the chop grain textures, the canopy shimmer, and creature call transients all ride in this signal. Any engine receiving it gets the full ecosystem character.

### As Coupling Target — Supported Types

| Coupling Type | What XOcelot does | Musical effect |
|--------------|------------------|----------------|
| `AudioToWavetable` | Writes source audio into Understory chop buffer (scaled by `ocelot_couplingLevel`) | **Primary route.** Any engine's output gets chopped, bit-crushed, tape-warped. The Aesop Rock treatment applied to anything. |
| `AmpToFilter` | Source amplitude drives `ocelot_canopySpectralFilter` | External dynamics open/close canopy brightness — drum hits from ONSET make the jungle shimmer |
| `RhythmToBlend` | Source rhythm pattern shifts `ocelot_strataBalance` | ONSET's kick pattern shifts emphasis between Floor and Canopy — the jungle breathes with the beat |
| `AudioToFM` | Source audio FM-modulates Canopy complex oscillator | External engine adds harmonic complexity to lush pad layer |
| `EnvToMorph` | Source envelope sweeps `ocelot_creaturePitch` | External dynamics control what the Emergent layer "says" |
| `LFOToPitch` | Source LFO modulates Floor `ocelot_floorTension` | Cross-engine vibrato on berimbau/cuica |
| `EnvToDecay` | Source envelope controls `ocelot_floorDamping` | External dynamics control percussion resonance |

### Unsupported Coupling Types

| Type | Why |
|------|-----|
| `AmpToChoke` | Choking an ecosystem kills everything unnaturally — no musical use |
| `PitchToPitch` | Floor models have fixed pitch tables; external pitch coupling conflicts with physical model behavior |
| `FilterToFilter` | XOcelot's spectral filter is ecosystem-driven; external filter coupling conflicts with the cross-feed matrix |
| `AudioToRing` | Ring modulation of the full ecosystem mix creates confusing phase artifacts |

---

## 11. Hero Preset Archetypes (8 presets for Phase 2 first-encounter)

| Name | Biome | Focus | M1 | M2 | M3 | M4 | DNA (bright/warm/move/dense/space/aggr) |
|------|-------|-------|----|----|----|----|----------------------------------------|
| Berimbau Dawn | Jungle | Floor+Understory | 0.20 | 0.45 | 0.15 | 0.30 | 0.45/0.75/0.65/0.55/0.35/0.20 |
| Coral Cathedral | Underwater | Canopy+Emergent | 0.80 | 0.25 | 0.10 | 0.85 | 0.25/0.45/0.30/0.45/0.90/0.05 |
| Dusty Kalimba | Jungle | Understory-heavy | 0.30 | 0.70 | 0.20 | 0.25 | 0.60/0.70/0.80/0.65/0.30/0.35 |
| Full Ecosystem | Jungle | All strata, high xf | 0.50 | 0.80 | 0.85 | 0.50 | 0.55/0.65/0.85/0.85/0.50/0.30 |
| Frozen Canopy | Winter | Canopy+Emergent sparse | 0.75 | 0.20 | 0.15 | 0.80 | 0.70/0.20/0.25/0.30/0.80/0.05 |
| Jungle Pulse | Jungle | Coupling-ready (ONSET) | 0.35 | 0.75 | 0.65 | 0.40 | 0.50/0.60/0.90/0.70/0.40/0.45 |
| Glass Marimba | Underwater | Floor (kalimba→glass) | 0.15 | 0.50 | 0.10 | 0.65 | 0.75/0.40/0.55/0.40/0.65/0.10 |
| Creature Surge | Jungle | Emergent-driven feedback | 0.55 | 0.90 | 0.90 | 0.45 | 0.55/0.60/0.95/0.75/0.45/0.55 |

**First-encounter preset:** `Berimbau Dawn` — Floor-forward, warm, immediately musical, non-threatening. The user understands "jungle percussion" before ecosystem depth is introduced.

---

## 12. Real-Time Safety Notes

- **No memory allocation on audio thread.** All voice/grain buffers pre-allocated in `prepareToPlay()`.
- **Denormal protection:** `ScopedNoDenormals` at top of `renderBlock()`. `flushDenormal()` on all shimmer feedback paths, tape wobble state, and formant filter state variables.
- **EcosystemMatrix routes:** Per-block arithmetic only (no per-sample branches in Continuous routes). Threshold routes use soft knee evaluated once per block, not per sample.
- **Biome transitions:** `BiomeMorph` increments `biomePhase` on audio thread (cheap linear interp). No lock, no allocation.
- **ParamSnapshot:** All `std::atomic<float>*` reads cached once per block in `OcelotParamSnapshot`. Zero atomic reads inside per-sample loops.
- **Shimmer feedback clamp:** `ocelot_canopyShimmer` coefficient clamped to max 0.88 regardless of user value. Prevents runaway even at M4=1.0.

---

## 13. Phase 1 Decision Gate

- [x] Full spec written
- [x] 62 parameter IDs frozen with `ocelot_` prefix
- [x] Cross-feed matrix typed (12 routes, 3 route types, negative-amount semantics locked)
- [x] Macro mapping defined (PROWL/FOLIAGE/ECOSYSTEM/CANOPY)
- [x] Coupling interface designed (7 supported types, 4 unsupported with reasons)
- [x] Signal flow diagram complete
- [x] Voice architecture defined (8 voices, quietest-stealing, legato)
- [x] CPU budget verified (<13%, within <28% dual-engine envelope)
- [x] Hero preset archetypes defined (8 presets, DNA values assigned)
- [x] DSP module list defined (9 new + 4 reused from Source/DSP/)
- [x] Biome system architecture defined (BiomeMorph, 200ms crossfade, per-stratum profiles)
- [x] Real-time safety requirements documented

**→ Proceed to Phase 2: Scaffold + Build**
*Invoke: `/new-xo-project name=XOcelot identity="Canopy-layered sample-mangling synthesizer — four interacting strata with biome system" code=XOcl`*

---

*XO_OX Designs | Engine: OCELOT | Accent: #C5832B | Prefix: ocelot_*
