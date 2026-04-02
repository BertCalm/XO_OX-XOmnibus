# XOceanic — Concept Brief

**Date:** March 2026
**Phase:** Complete — Standalone built, AU passes auval, 34 factory presets
**Status:** Final

---

## Identity

- **Name:** XOceanic
- **Gallery code:** OCEANIC
- **Accent color:** Phosphorescent Teal `#00B4A0`
- **Parameter prefix:** `oceanic_`
- **Thesis:** XOceanic is a string ensemble synth paired with a bioluminescent effects processor — the strings provide warmth and body, the pedalboard reveals colors hiding inside them that you need *different eyes* to see.
- **Sound family:** Pad / String / Texture / FX hybrid
- **Unique capability:** Paraphonic divide-down string synthesis (Solina architecture) fused with a Chase Bliss-style experimental pedalboard where each effect module is designed to expose hidden harmonics and spectral content. The effects don't *add* color — they *reveal* colors already present in the string sound, like deep-sea creatures whose beauty exists in wavelengths beyond normal perception.

---

## Character

- **Personality in 3 words:** Luminous, Submerged, Alive
- **Engine approach:** Paraphonic string ensemble (divide-down oscillators + triple-phase BBD ensemble chorus) → experimental effects chain (chromatophore pedalboard)
- **Why this dual architecture serves the character:**
  The Solina String Ensemble's magic was never the oscillators — it was the *ensemble effect*. Three BBD (bucket-brigade delay) lines with slightly different LFO rates create a shimmering, living quality that no standard chorus matches. That's the creature. The Chase Bliss-inspired pedalboard is the *weird ocean eyes* — each module reveals a different spectral dimension hiding inside the string sound. Together: warm string body + impossible bioluminescent color.

---

## The XO Concept Test

1. **XO word:** XOceanic ✓ — Oceanic: vast, deep, teeming with invisible life, unexplored colors
2. **One-sentence thesis:** "XOceanic is a string ensemble synth where a bioluminescent pedalboard reveals impossible colors hiding inside warm analog strings — like deep-sea creatures whose beauty needs different eyes to see." ✓
3. **Sound only this can make:** A lush Solina string pad where spectral freeze captures a single harmonic moment, pitch-shifted reverb exposes overtones 3 octaves up as shimmering tails, and granular scatter breaks the sustained chord into time-displaced particles — all pulsing with chromatophore-like modulation that makes the colors *breathe*. No other engine combines paraphonic ensemble with spectral-revealing effects. ✓

---

## Gallery Gap Filled

| Existing engines | Synthesis dimension |
|-----------------|---------------------|
| ODDFELIX, ODDOSCAR, OBLONG | Standard polyphonic (one voice per note) |
| ODYSSEY | Psychedelic poly (journey/climax as organizing principle) |
| OBESE | Massive poly (13 osc per voice, width) |
| OVERBITE | Bass character (belly/bite duality) |
| OVERWORLD | Chip synthesis (NES/Genesis/SNES) |
| OPAL | Granular (time as synthesis dimension) |
| ONSET | Drum synthesis (percussive) |
| OVERDUB | Dub FX routing (send/return architecture) |
| **OCEANIC** | **Paraphonic string ensemble + spectral-revealing effects** |

**Two unique gaps filled at once:**
1. **Paraphonic architecture** — every other engine is fully polyphonic. XOceanic's divide-down topology creates a fundamentally different harmonic interaction where all notes share the same ensemble chorus, filter, and amp path. Chords *blend* rather than stack.
2. **Effects as revelation** — every other engine treats FX as post-processing. XOceanic's pedalboard is a co-equal instrument designed to expose hidden spectral content, not just color it.

---

## Core Synthesis Architecture (Phase 0 sketch)

### Layer A: String Ensemble (The Creature)

Inspired by the ARP Solina String Ensemble (1972):

```
Master Clock (high frequency, divide-down)
        │
        ├── ÷2  → Octave registers
        ├── ÷3  → Fifth intervals
        ├── ÷4, ÷8, ÷16... → Lower octaves
        │
        ▼
Registration Mixer (organ-style stops)
├── VIOLIN     (8')  — bright, present
├── VIOLA      (8')  — darker, nasal
├── CELLO      (8')  — warm, full
├── BASS       (16') — low, fundamental
├── CONTRABASS (32') — sub-bass depth
├── HORN       (8')  — brassy, hollow
        │
        ▼
Ensemble Chorus (THE secret sauce)
├── BBD Line 1: ~6ms delay, LFO at 0.63 Hz
├── BBD Line 2: ~7ms delay, LFO at 0.95 Hz
├── BBD Line 3: ~8ms delay, LFO at 1.40 Hz
│   (3 modulated delay lines with specific phase offsets)
│   (This is what makes it a Solina, not just "string pad")
        │
        ▼
Paraphonic Filter (shared across all notes)
├── Lowpass (warm Solina default)
├── Bandpass (nasal string color)
├── Highpass (thin, ethereal)
        │
        ▼
Paraphonic Amp Envelope (shared ADSR)
        │
        ▼
String Output → [to Pedalboard input]
```

**Paraphonic key detail:** All pressed notes share ONE filter and ONE amp envelope. When you play a chord, the filter sweeps all notes together. When you release, they all decay together. This creates the Solina's signature: chords feel like *one instrument*, not stacked voices. A new note retriggers the envelope for *all* sounding notes.

**Divide-down key detail:** Rather than independent oscillators per voice (expensive, standard poly), a single high-frequency master oscillator is digitally divided to produce all pitches. This is why the Solina could play unlimited simultaneous notes with minimal hardware — and why chords have that distinctive "organ-like" coherence.

### Layer B: Chromatophore Pedalboard (The Eyes)

Chase Bliss-inspired experimental effects designed *for* string sounds:

```
String Output
        │
        ▼
┌─────────────────────────────────────┐
│    CHROMATOPHORE PEDALBOARD         │
│                                     │
│  ┌─────────┐  Spectral Freeze      │
│  │ FREEZE  │  Captures a moment's  │
│  │         │  harmonic content and  │
│  │         │  sustains it infinitely│
│  └────┬────┘                        │
│       ▼                             │
│  ┌─────────┐  Granular Scatter      │
│  │ SCATTER │  Micro-looping with    │
│  │         │  clock modulation,     │
│  │         │  density control       │
│  └────┬────┘                        │
│       ▼                             │
│  ┌─────────┐  Warped Tape Delay     │
│  │  TIDE   │  Pitch-shifted feedback│
│  │         │  Wow/flutter, self-osc │
│  │         │  tape degradation      │
│  └────┬────┘                        │
│       ▼                             │
│  ┌─────────┐  Shimmer Reverb        │
│  │ ABYSS   │  Pitch-shifted tails   │
│  │         │  (+12st, +24st octave) │
│  │         │  Infinite decay mode   │
│  └────┬────┘                        │
│       ▼                             │
│  ┌─────────┐  Reverse Buffer        │
│  │ MIRROR  │  Reverse playback      │
│  │         │  with crossfade blend  │
│  │         │  Pre-delay into reverb │
│  └────┬────┘                        │
│       ▼                             │
│  Chromatophore Modulator            │
│  (Rhythmic, organic pulsing of all  │
│   pedal parameters — like cuttlefish│
│   skin shifting color. Rate, depth, │
│   and pattern controls.)            │
│                                     │
└──────────┬──────────────────────────┘
           ▼
     Output (stereo)
```

**Pedal chain is serial** — each module feeds the next, building complexity. Any module can be bypassed. The *order* matters: freezing → scattering a frozen moment → delaying the scattered particles → reverberating the delayed scatter → reversing the reverb tail. Each step reveals a deeper layer of the string's hidden harmonics.

**Chromatophore Modulator:** The unifying concept. A global modulation source that rhythmically pulses parameters across all active pedals — not random, but *organic*. Like watching a cuttlefish's skin shift patterns. Controls:
- Rate (0.1–10 Hz)
- Depth (how much each pedal is affected)
- Pattern (sine / triangle / random-smooth / pulse / drift)
- Separation (how offset each pedal's modulation phase is — at max, each pulses independently; at min, they all breathe together)

---

## Parameter Namespace

All parameter IDs use `oceanic_` prefix. Key parameters (~65 estimated):

### String Ensemble (Layer A)
| ID | Range | Description |
|----|-------|-------------|
| `oceanic_violin` | 0-1 | Violin stop level (8') |
| `oceanic_viola` | 0-1 | Viola stop level (8') |
| `oceanic_cello` | 0-1 | Cello stop level (8') |
| `oceanic_bass` | 0-1 | Bass stop level (16') |
| `oceanic_contrabass` | 0-1 | Contrabass stop level (32') |
| `oceanic_horn` | 0-1 | Horn stop level (8') |
| `oceanic_ensemble` | 0-1 | Ensemble chorus depth |
| `oceanic_ensRate` | 0.1-3 Hz | Ensemble LFO rate multiplier |
| `oceanic_filterCutoff` | 20-20000 Hz | Paraphonic filter cutoff |
| `oceanic_filterReso` | 0-1 | Filter resonance |
| `oceanic_filterType` | 0-2 | LP / BP / HP |
| `oceanic_attack` | 0.001-4s | Amp envelope attack |
| `oceanic_decay` | 0.05-4s | Amp envelope decay |
| `oceanic_sustain` | 0-1 | Amp envelope sustain |
| `oceanic_release` | 0.05-8s | Amp envelope release |
| `oceanic_separation` | 0-1 | Registration voice spread (stereo width per stop) |
| `oceanic_drift` | 0-1 | Analog-style pitch drift amount |
| `oceanic_brightness` | 0-1 | Pre-ensemble high-frequency content |

### Pedalboard (Layer B)
| ID | Range | Description |
|----|-------|-------------|
| `oceanic_freezeMix` | 0-1 | Spectral freeze wet/dry |
| `oceanic_freezeGain` | 0-1 | Frozen signal level |
| `oceanic_scatterSize` | 5-500ms | Granular grain size |
| `oceanic_scatterDensity` | 1-80/s | Grains per second |
| `oceanic_scatterPitch` | 0-12st | Pitch scatter range |
| `oceanic_scatterMix` | 0-1 | Scatter wet/dry |
| `oceanic_tideTime` | 10-2000ms | Tape delay time |
| `oceanic_tideFeedback` | 0-1.1 | Delay feedback (self-osc >1.0) |
| `oceanic_tideWarp` | 0-1 | Wow/flutter amount |
| `oceanic_tidePitch` | -12 to +12st | Feedback pitch shift |
| `oceanic_tideMix` | 0-1 | Tide wet/dry |
| `oceanic_abyssDecay` | 0.5-inf | Shimmer reverb decay |
| `oceanic_abyssShift` | 0/12/24st | Shimmer pitch shift interval |
| `oceanic_abyssDamp` | 0-1 | High-frequency damping |
| `oceanic_abyssMix` | 0-1 | Abyss wet/dry |
| `oceanic_mirrorLength` | 50-2000ms | Reverse buffer length |
| `oceanic_mirrorBlend` | 0-1 | Forward/reverse crossfade |
| `oceanic_mirrorMix` | 0-1 | Mirror wet/dry |
| `oceanic_chromRate` | 0.1-10Hz | Chromatophore mod rate |
| `oceanic_chromDepth` | 0-1 | Chromatophore mod depth |
| `oceanic_chromPattern` | 0-4 | Mod pattern shape |
| `oceanic_chromSeparation` | 0-1 | Phase offset between pedals |
| `oceanic_pedalMix` | 0-1 | Overall pedalboard wet/dry |

*Full parameter list defined in Phase 1 architecture.*

---

## Macro Mapping (M1-M4)

| Macro | Label | Controls | Behavior |
|-------|-------|----------|----------|
| M1 | DEPTH | `oceanic_pedalMix` + `oceanic_ensemble` + `oceanic_chromDepth` | 0=pure dry Solina warmth. 1=full chromatophore processing. **The revealer.** How much hidden color do you want to see? |
| M2 | CURRENT | `oceanic_chromRate` + `oceanic_chromSeparation` + `oceanic_tideWarp` + `oceanic_scatterDensity` | Speed and complexity of the organic modulation. 0=still water. 1=churning deep current. |
| M3 | COUPLING | Reserved for XOceanus coupling amount. In standalone: crossfade between string ensemble and external audio (coupling input as grain/freeze source) | **The portal** — how much of another engine enters the pedalboard. |
| M4 | ABYSS | `oceanic_abyssMix` + `oceanic_abyssDecay` + `oceanic_mirrorMix` + `oceanic_separation` | Spatial depth. 0=close, intimate strings. 1=infinite shimmering abyss. |

All 4 macros produce audible, significant change at every point in their range in every preset.

---

## Coupling Interface Design

### OCEANIC as Coupling Target (receiving from other engines)

| Coupling Type | What XOceanic Does | Musical Effect |
|---------------|-------------------|----------------|
| `AudioToWavetable` | External audio enters pedalboard as secondary source (mixed with strings before FREEZE) | Any engine's sound processed through the chromatophore chain — THE killer coupling |
| `AmpToFilter` | Source amplitude → paraphonic filter cutoff | Drum hits or bass plucks sweep the string filter — rhythmic string breathing |
| `EnvToMorph` | Source envelope → ensemble chorus depth | Crescendos in other engines intensify the string ensemble shimmer |
| `LFOToPitch` | Source LFO → master pitch drift | Cross-engine organic pitch wander |

**Primary coupling:** `AudioToWavetable` — external engine audio enters the chromatophore pedalboard. ODYSSEY's Climax feeds into FREEZE. OVERWORLD's chip audio feeds into SCATTER. OVERBITE's bass feeds into TIDE. The pedalboard reveals hidden colors in *any* engine.

### OCEANIC as Coupling Source (sending to other engines)

`getSampleForCoupling()` returns: post-pedalboard stereo output (fully processed string + FX signal).

**Good receiving engines:**
- **OVERDUB** — Processed strings through dub delay. String ensemble → tape echo.
- **OPAL** — OCEANIC's warm string output → granulated through OPAL's time engine. String particles.
- **ODYSSEY** — OCEANIC's ensemble modulates JOURNEY position. Strings drive the psychedelic journey.
- **OBESE** — String output modulates OBESE's Mojo blend. Analog warmth from strings softens digital precision.

### Coupling types OCEANIC should NOT receive
- `AmpToChoke` — choking a string ensemble kills the pad character (no musical use)
- `AudioToRing` — ring modulation on string ensemble produces ugly metallic artifacts without musical value

---

## Visual Identity

- **Accent color:** Phosphorescent Teal `#00B4A0`
  - Between OddOscar's teal `#2A9D8F` and Organon's bioluminescent cyan `#00CED1`
  - Suggests deep-water phosphorescence — that eerie glow of living things in darkness
- **Material/texture:** Bioluminescent sea creature skin — semi-translucent, with color shifting and organic pulsing underneath
- **Gallery panel character:** The panel should feel *alive* — subtle shifting color underneath a dark translucent surface, like looking at a chromatophore through dark water. The pedalboard section should have distinct stomp-box outlines, each with its own subtle glow when active.
- **Icon concept:** A deep-sea creature silhouette (anglerfish? jellyfish? cuttlefish?) with phosphorescent spots — the creature *is* the instrument, the glowing spots are the pedals revealing hidden color

---

## Mood Affinity

| Mood | Affinity | Why |
|------|----------|-----|
| Foundation | Medium | Dry ensemble strings can be foundation-stable — warm, grounded, simple |
| Atmosphere | **High** | Processed strings through shimmer reverb + freeze = definitive atmosphere |
| Entangled | **High** | Coupling other engines through the chromatophore pedalboard = entanglement |
| Prism | **High** | Bright ensemble + scatter + shimmer = prismatic, refracting light |
| Flux | Medium | Chromatophore modulation creates movement, but strings resist chaos |
| Aether | **High** | Frozen strings in infinite shimmer reverb = pure aether |

Primary moods: Atmosphere, Entangled, Prism, Aether.

---

## Preset Strategy (Phase 0 sketch)

**120 presets at v1.0:**

| Category | Count | Character |
|----------|-------|-----------|
| **Pure Ensemble** | 20 | Dry Solina strings — warm, simple, registration variations. Foundation/Atmosphere. |
| **Bioluminescent Pads** | 25 | Strings through shimmer reverb + freeze. Glowing, suspended, ethereal. Atmosphere/Aether. |
| **Chromatophore Textures** | 20 | Full pedalboard engaged — shifting, pulsing, alive. Scatter + Tide + chromatophore mod. Prism/Flux. |
| **Deep Creatures** | 15 | Heavy processing, reverse buffers, self-oscillating tape delays. Strange, beautiful, alien. Prism/Aether. |
| **Spectral Freeze** | 15 | Freeze-focused — capturing moments of string harmonics and sustaining them. Aether. |
| **Coupling Showcases** | 25 | Designed for specific engine pairs. External audio through the chromatophore chain. Entangled. |

---

## Aesthetic / Musical DNA

### Reference Artists & Sounds
- **Solina side:** Vangelis (*Blade Runner*), Jean-Michel Jarre (*Oxygène*), Boards of Canada (warm analog strings), Tangerine Dream
- **Chase Bliss side:** Chase Bliss Mood (micro-looping, reverse), Chase Bliss Blooper (tape degradation, modulated loops), Hologram Microcosm (granular pedal processing)
- **Combined:** Stars of the Lid (processed strings as ambient music), Grouper (reverb as instrument), Tim Hecker (spectral manipulation of harmonic sources)

### The Aquatic Thread
- The string ensemble is the **creature** — warm-blooded, alive, swimming through the deep
- The pedalboard is the **chromatophores** — the color-shifting cells that reveal beauty invisible to surface eyes
- The chromatophore modulator is the **pulse** — the organic rhythm of the creature breathing, shifting, signaling
- The coupling bus is the **current** — other creatures' signals carried through the water, entering the chromatophore chain
- The output is the **bioluminescence** — what you see when you look with different eyes

---

## Decision Gate: Phase 0 → Phase 1

- [x] Concept brief written
- [x] XO word feels right (XOceanic — oceanic metaphor is authentic to the sound AND the brand's aquatic theme)
- [x] Gallery gap is clear (no paraphonic string ensemble exists; no "effects as revelation" exists)
- [x] At least 2 coupling partner ideas exist (ODYSSEY, OVERWORLD, OVERBITE, OPAL, OVERDUB)
- [x] Excited about the sound
- [x] Unique capability defined (Solina ensemble + Chase Bliss chromatophore pedalboard)

**→ Ready for Phase 1: Architect**
*Invoke: `/new-xo-engine phase=1 name=XOceanic identity="String ensemble synth with bioluminescent chromatophore pedalboard — reveals impossible colors hiding inside warm analog strings" code=XOcn`*

---

*XO_OX Designs | Engine: OCEANIC | Accent: #00B4A0 | Prefix: oceanic_*
