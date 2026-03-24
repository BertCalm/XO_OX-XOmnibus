# XOceanDeep — Engine Specification

**Gallery Code:** OCEANDEEP
**Accent Color:** Trench Violet `#2D0A4E`
**Parameter Prefix:** `deep_`
**Phase:** 1 — Architecture
**Date:** March 2026
**Status:** Ready for DSP scaffold

---

## 1. Concept

XOceanDeep is an abyssal bass engine — the crushing pressure, bioluminescent creatures, sunken treasure, and 808 rumble of 10,000 leagues under the sea. The darkest sounds in the XO_OX universe.

It is the direct counterpart to OPENSKY. If OPENSKY is stratospheric feliX — euphoric shimmer above the cloud line — OCEANDEEP is abyssal Oscar: the sound of the hadal zone, 11km below the surface, in permanent darkness, under crushing pressure. The same water column, the opposite extreme.

The deep ocean isn't just "low frequency." It's pressure, darkness, alien biology, and ancient wreckage. Other engines go low. OceanDeep goes *under*.

**Sound family:** Bass / Sub / Dark Texture / Creature FX
**Max voices:** 8
**CPU budget:** < 12%
**Gallery gap filled:** No existing XOlokun engine models bass as physical pressure with deep-sea creature behavior. BITE has character stages. OBESE has oscillator count. DUB has space. OceanDeep has the weight of the entire ocean.

---

## 2. Architecture

### Signal Path

```
SUB OSCILLATOR STACK
├── Primary sub: sine / triangle / 808 shape
├── Secondary sub: one octave below, shaped pulse
├── Harmonic layer: saw/square for upper presence
├── Phase relationship control (phase-aligned sub weight)
│
▼
HYDROSTATIC COMPRESSOR
├── Pressure-modeled dynamics — deeper = heavier transients
├── deep_pressure: 0 (surface dynamics) → 1 (abyssal crush)
├── Slow attack, heavy ratio — everything gets massive
│
▼
WAVEGUIDE BODY (environment resonance)
├── Shipwreck hull (metallic, creaking, enclosed)
├── Trench wall (stone, infinite, dark)
├── Cave (warm, resonant, Oscar's home)
├── Open abyss (vast, cold, no reflections)
├── Body size + damping + material controls
│
▼
BIOLUMINESCENT EXCITER
├── Sharp transient flash — bright burst in the dark
├── deep_flashRate: how often creatures flash (0 = dark, 1 = frenzy)
├── deep_flashBright: intensity of each flash
├── Adds high-frequency transient spikes against sub content
├── The only "light" in the engine — everything else is dark
│
▼
CREATURE MODULATION
├── Random-walk LFO (Brownian motion — darting behavior)
├── Targets: pitch, filter, pan, waveguide position
├── deep_creatureSpeed: slow drift ↔ frantic
├── deep_creatureDepth: how far they travel
├── 2-3 simultaneous independent creature paths
│
▼
DARKNESS FILTER
├── Low-pass with resonance — the deep absorbs highs
├── Key-tracked (higher notes get more filtering — depth increases with register)
├── deep_darkness: 0 = some light bleeds through, 1 = total blackout
│
▼
FX CHAIN
├── Pressure Reverb (massive, dark, slow — the trench itself)
├── Creature Delay (short, darting echoes — bouncing off wreckage)
├── Rumble Generator (sub-20Hz content — felt not heard)
│
▼
Output (stereo)
```

### DSP Approach

**Sub oscillator stack:** Three-layer design. Primary sub oscillator runs at MIDI pitch with selectable shape (sine/triangle/808-curve/pulse). Secondary sub runs one or two octaves below — shaped pulse for upper harmonic bleeding into the fundamental. Harmonic layer is a traditional saw/square for upper-register presence on leads and mid-bass presets. Phase coherence between layers is controlled by `deep_phaseAlign` — incoherent phase produces physical pressure-like sub weight at the expense of headroom.

**Hydrostatic compressor:** Models the dynamics-flattening effect of ocean pressure. Attack ranges 0.01–2s (slow attack preserves punch), ratio scales from 2:1 (surface) to 20:1 (abyssal crush). Knee is soft by default — the ocean doesn't clip. `deep_pressure` is the master dial; internally it maps to ratio, makeup gain, and attack time simultaneously.

**Waveguide body:** Four resonant environments modeled via simple Schroeder reverb + comb filter networks tuned to each physical character. Shipwreck: metallic comb filters, high density, material resonance at 200–800Hz. Trench wall: sparse comb, long decay, stone absorption. Cave: warm modal resonance, Oscar-friendly. Open abyss: near-zero density, maximum decay, the sound of nothing. `deep_bodySize` scales the comb delays; `deep_bodyDamp` scales the LP coefficient on each delay line.

**Bioluminescent exciter:** A transient generator independent of the main oscillator path. Produces sharp, bright (up to 8kHz) spike bursts whose timing is governed by `deep_flashRate` (0–10 Hz, intentionally low) and whose amplitude is `deep_flashBright`. The exciter is bandpass-filtered to prevent bass mud — it exists only as a texture layer, never dominant. At full `deep_darkness`, the exciter is ducked via the filter's key-track. This is the engine's only source of brightness.

**Creature modulation:** Brownian-motion LFO implemented as a random walk with `deep_creatureSpeed` controlling the step size per sample block. Two simultaneous creature paths run independently — one targets pitch (±24 cents maximum), one targets filter cutoff (±400Hz maximum), one targets panning (±0.4). `deep_creatureDepth` scales all paths. At high speed, the motion is frantic and alive; at low speed, it drifts like cold water currents.

**Matched-Z filters:** All filter stages use `exp(-2*PI*fc/sr)` IIR coefficients (not Euler approximation) per project DSP standards. The darkness filter is a 24dB/oct low-pass with tunable resonance. Key-tracking applies at 100% by default — bass notes pass more highs; high notes are crushed into subharmonic mud.

**Rumble generator:** Sub-20Hz sine content added post-filter. Inaudible at normal playback but felt on speakers with adequate low-frequency capability. Provides the physical-force quality distinct from audible bass. `deep_rumble` is a wet blend (0–1); at 1.0 the rumble equals the main signal level in the 8–20Hz band.

---

## 3. Parameter List (~75 parameters, `deep_` prefix)

### Oscillator Stack (12)

| ID | Range | Default | Description |
|----|-------|---------|-------------|
| `deep_subShape` | 0–3 | 2 | Primary sub shape: sine / triangle / 808-curve / pulse |
| `deep_subOctave` | -2, -1, 0 | -1 | Sub octave offset relative to MIDI note |
| `deep_subLevel` | 0.0–1.0 | 1.0 | Primary sub oscillator level |
| `deep_subTwoLevel` | 0.0–1.0 | 0.5 | Secondary sub (one octave below) level |
| `deep_harmonicLevel` | 0.0–1.0 | 0.25 | Upper harmonic layer level (saw/square) |
| `deep_harmonicShape` | 0–1 | 0 | Harmonic layer shape: saw / square |
| `deep_phaseAlign` | 0.0–1.0 | 0.8 | Phase coherence between sub layers (1 = phase-locked) |
| `deep_subDetune` | -50–+50 cents | 0 | Primary sub fine detune |
| `deep_subDrift` | 0.0–1.0 | 0.03 | Analog-style pitch drift amount |
| `deep_pitchEnvDepth` | -24–+24 st | -2 | Pitch envelope amount (negative = downward sweep) |
| `deep_pitchEnvAttack` | 0.001–2.0s | 0.005 | Pitch envelope attack |
| `deep_pitchEnvDecay` | 0.01–4.0s | 0.3 | Pitch envelope decay |

### Hydrostatic Compressor (4)

| ID | Range | Default | Description |
|----|-------|---------|-------------|
| `deep_pressure` | 0.0–1.0 | 0.4 | Master pressure — scales ratio, makeup, attack simultaneously |
| `deep_pressureRatio` | 1:1–20:1 | 4:1 | Compressor ratio (overridden by deep_pressure macro) |
| `deep_pressureAttack` | 0.01–2.0s | 0.1 | Compressor attack time |
| `deep_pressureRelease` | 0.05–4.0s | 0.8 | Compressor release time |

### Waveguide Body (6)

| ID | Range | Default | Description |
|----|-------|---------|-------------|
| `deep_bodyType` | 0–3 | 2 | Environment: shipwreck / trench / cave / open abyss |
| `deep_bodySize` | 0.0–1.0 | 0.5 | Environment scale (scales comb delay lengths) |
| `deep_bodyDamp` | 0.0–1.0 | 0.5 | Material absorption (LP coefficient on delay lines) |
| `deep_bodyMix` | 0.0–1.0 | 0.35 | Waveguide wet/dry |
| `deep_bodyResonance` | 0.0–1.0 | 0.3 | Waveguide feedback / resonance |
| `deep_bodyMaterial` | 0.0–1.0 | 0.5 | Morph between body types (crossfade) |

### Bioluminescent Exciter (5)

| ID | Range | Default | Description |
|----|-------|---------|-------------|
| `deep_flashRate` | 0.0–10.0 Hz | 0.5 | Bioluminescent flash frequency (0 = dark, 10 = frenzy) |
| `deep_flashBright` | 0.0–1.0 | 0.3 | Transient spike intensity |
| `deep_flashDecay` | 0.001–0.5s | 0.02 | Flash transient decay time |
| `deep_flashFreq` | 200–8000 Hz | 3000 | Flash bandpass center frequency |
| `deep_flashMix` | 0.0–1.0 | 0.2 | Exciter blend into main signal |

### Creature Modulation (6)

| ID | Range | Default | Description |
|----|-------|---------|-------------|
| `deep_creatureSpeed` | 0.001–2.0 | 0.15 | Random-walk LFO step size (0 = drift, 2 = frantic) |
| `deep_creatureDepth` | 0.0–1.0 | 0.4 | Overall modulation depth of creature movement |
| `deep_creaturePitchAmt` | 0.0–1.0 | 0.5 | Pitch modulation amount (±24 cents at 1.0) |
| `deep_creatureFilterAmt` | 0.0–1.0 | 0.6 | Filter modulation amount (±400Hz at 1.0) |
| `deep_creaturePanAmt` | 0.0–1.0 | 0.3 | Panning modulation amount (±0.4 at 1.0) |
| `deep_creatureSync` | 0–1 | 0 | Sync creature paths together (0 = independent) |

### Darkness Filter (5)

| ID | Range | Default | Description |
|----|-------|---------|-------------|
| `deep_darkness` | 0.0–1.0 | 0.6 | Low-pass darkness — 0 = light bleeds in, 1 = total blackout |
| `deep_filterCutoff` | 20–500 Hz | 180 | Darkness filter base cutoff frequency |
| `deep_filterReso` | 0.0–1.0 | 0.25 | Darkness filter resonance |
| `deep_filterKeyTrack` | 0.0–1.0 | 1.0 | Key-tracking amount (1.0 = full — higher notes get more filtering) |
| `deep_filterEnvAmt` | -1.0–+1.0 | 0.4 | Filter envelope amount |

### Amplitude Envelope (4)

| ID | Range | Default | Description |
|----|-------|---------|-------------|
| `deep_attack` | 0.001–10.0s | 0.01 | Amp attack (0–10s range: slow attack option for pressure builds) |
| `deep_decay` | 0.01–4.0s | 0.5 | Amp decay |
| `deep_sustain` | 0.0–1.0 | 0.7 | Amp sustain level |
| `deep_release` | 0.01–8.0s | 1.0 | Amp release |

### Filter Envelope (4)

| ID | Range | Default | Description |
|----|-------|---------|-------------|
| `deep_filterAttack` | 0.001–10.0s | 0.005 | Filter envelope attack |
| `deep_filterDecay` | 0.01–4.0s | 0.4 | Filter envelope decay |
| `deep_filterSustain` | 0.0–1.0 | 0.3 | Filter envelope sustain |
| `deep_filterRelease` | 0.01–8.0s | 0.8 | Filter envelope release |

### LFOs (8)

| ID | Range | Default | Description |
|----|-------|---------|-------------|
| `deep_lfo1Rate` | 0.001–0.5 Hz | 0.05 | LFO 1 rate — infrasonic range dominant (0.001–0.5Hz) |
| `deep_lfo1Depth` | 0.0–1.0 | 0.0 | LFO 1 depth |
| `deep_lfo1Shape` | 0–4 | 0 | Sine/Tri/Saw/S&H/Random |
| `deep_lfo1Target` | 0–5 | 0 | LFO 1 target: pitch/filter/pressure/body/flash/pan |
| `deep_lfo2Rate` | 0.001–2.0 Hz | 0.2 | LFO 2 rate |
| `deep_lfo2Depth` | 0.0–1.0 | 0.0 | LFO 2 depth |
| `deep_lfo2Shape` | 0–4 | 2 | Sine/Tri/Saw/S&H/Random |
| `deep_lfo2Target` | 0–5 | 2 | LFO 2 target |

### FX Chain (11)

| ID | Range | Default | Description |
|----|-------|---------|-------------|
| `deep_reverbMix` | 0.0–1.0 | 0.4 | Pressure reverb wet |
| `deep_reverbSize` | 0.0–1.0 | 0.7 | Trench size — reverb room size |
| `deep_reverbDamp` | 0.0–1.0 | 0.8 | Reverb high-frequency damping |
| `deep_reverbPreDelay` | 0–200ms | 20 | Pre-delay before reverb tail |
| `deep_delayMix` | 0.0–1.0 | 0.2 | Creature delay wet |
| `deep_delayTime` | 0.01–0.5s | 0.12 | Creature delay time (short, darting) |
| `deep_delayFeedback` | 0.0–0.85 | 0.35 | Delay feedback |
| `deep_rumble` | 0.0–1.0 | 0.3 | Sub-20Hz rumble generator (felt not heard) |
| `deep_rumbleFreq` | 5–20 Hz | 12 | Rumble center frequency |
| `deep_outputLevel` | 0.0–1.0 | 0.85 | Master output level |
| `deep_stereoWidth` | 0.0–1.0 | 0.5 | Output stereo width |

**Total: 75 parameters**

---

## 4. Macro Mapping (M1–M4)

| Macro | Label | Controls | Behavior |
|-------|-------|----------|----------|
| M1 | **DEPTH** | `deep_subOctave` + `deep_pressure` + `deep_darkness` | The depth dial. 0 = shallow — surface dynamics, some light, standard register. 1 = abyssal crush — lowest octave, maximum pressure, total darkness. Sweeps the engine from functional bass to geological event. |
| M2 | **PRESSURE** | `deep_pressure` + `deep_reverbSize` + `deep_rumble` | Bass density. 0 = open, dynamic, dry. 1 = every transient crushed under ocean weight, massive trench reverb, infrasonic rumble. The physical-force dial. |
| M3 | **DARKNESS** | `deep_darkness` + `deep_bodyType` blend + `deep_delayMix` | Filter character + environment. 0 = open abyss, minimal absorption, clean bass. 1 = deep inside a sunken shipwreck, metallic reflections, maximum low-pass. The environment dial. |
| M4 | **STILLNESS** | `deep_creatureSpeed` (inverted) + `deep_flashRate` (inverted) + `deep_lfo1Depth` (inverted) | Absence of movement. 0 = frantic creature motion, constant flash, modulation alive. 1 = perfect stillness — the emptiness of the hadal zone at 4am. Nothing moves at STILLNESS maximum. |

Note: The original concept used WRECK/ABYSS for M3/M4. DARKNESS/STILLNESS better capture the feliX-Oscar polarity and the engine's identity as an absence-of-movement instrument.

---

## 5. feliX-Oscar Polarity

**Position:** Almost entirely Oscar.

The water column floor. Oscar the axolotl descended to maximum depth — no light, no movement, no brightness. The sub-bass weight, pressure compressor, and darkness filter all push maximally toward Oscar: warmth, density, absence of transients, harmonic richness without brightness.

The only feliX presence: extreme DEPTH macro position (lowest register) introduces mathematical precision in sub frequency. A perfectly tuned 808 sine at C0 is feliX's harmonic rationality expressed in pure low frequency. But even that is immediately crushed under PRESSURE.

**feliX-Oscar balance:** ~10% feliX / 90% Oscar.

This extreme positioning is intentional — OCEANDEEP and OPENSKY are the two poles. Together as "The Full Column," they span the entire feliX-Oscar axis.

---

## 6. Coupling Matrix

### OCEANDEEP as Target (receives from)

| Source Engine | Coupling Type | Musical Effect |
|-------------|---------------|----------------|
| OVERDUB | `AudioToFM` | Dub space FM-modulates sub oscillator — the delay tail becomes creature movement |
| ORACLE | `EnvToMorph` | Stochastic envelope drives `deep_creatureSpeed` — unpredictable life in the deep |
| ORIGAMI | `EnvToMorph` | Fold compression drives `deep_pressure` — extreme low-frequency compression |
| OCEANIC | `AudioToFM` | Fluid dynamics modulate the waveguide body — the ocean moves around the engine |
| OPAL | `AmpToFilter` | Grain amplitude drives darkness filter — granular particles open windows of light |
| OPENSKY | `AmpToFilter` | Sky amplitude drives flash rate — euphoria above makes the creatures glow below |

### OCEANDEEP as Source (sends to)

| Target Engine | Coupling Type | Musical Effect |
|-------------|---------------|----------------|
| OPENSKY | `AmpToFilter` | Abyssal bass drives sky filter — pressure from below makes the sky shimmer |
| OVERDUB | `getSample` | Sub bass through dub echo — the deepest dub imaginable |
| ONSET | `AmpToFilter` | Bass pressure pumps drum filter — sub shakes the percussion |
| OVERBITE | `getSample` | Two deep predators layered — anglerfish + abyssal |
| OPAL | `AudioToWavetable` | Sub bass granulated into pressure particles — the ocean floor dissolved |
| OUIE | `AmpToFilter` | Abyssal bass drives hammerhead filter — apex predator fed by the floor |

### Signature Coupling Routes

**OCEANDEEP × OPENSKY — "The Full Column"**
Abyssal 808 bass under euphoric shimmer. Oscar's floor under feliX's sky. The entire XO_OX mythology in one patch. Recommended signal chain: OCEANDEEP AmpToFilter → OPENSKY (bass pressure drives sky filter shimmer rate).

**OCEANDEEP × OVERDUB — "Deepest Dub"**
Sub bass through spring reverb and dub echo. The most extreme low-frequency dub possible.

**OCEANDEEP × ORACLE — "Oracle from the Abyss"**
Stochastic modulation from ORACLE's GENDY synthesis drives creature behavior. Alien intelligence in alien depths.

### Couplings OCEANDEEP should NOT receive
- `AmpToChoke` — you do not silence the ocean floor
- `PitchToPitch` — sub bass needs precise tuning; external pitch creates frequency mud

---

## 7. Historical Homage

**Buchla 258 Oscillator** — The sub register of the 258 was legendary for pure, warm fundamental weight. OceanDeep's primary sub oscillator inherits that philosophy: the simplest possible waveform, in the lowest possible register, with maximum phase integrity.

**NIN "The Downward Spiral" bass character** — Trent Reznor's use of industrial bass as *physical force* rather than musical note. The compression, the sub-octave doublers, the sense that the bass is a geological event rather than an instrument — this is what PRESSURE macro captures.

**Kode9 / Burial dub-techno depth** — The South London bass tradition: sub that moves air, that feels like pressure in a chest, that creates space rather than filling it. The waveguide body's Cave and Open Abyss environments are direct homage to the dark, cavernous quality of UK bass music.

**Aphex Twin "Ventolin" sub-bass abuse** — Using infrasonic content as texture, not melody. The RUMBLE generator is this: the sub-20Hz presence that turns a normal bass patch into something physically uncomfortable.

---

## 8. Aquatic Mythology Position

Oscar's throne room. The absolute floor of the water column. No light reaches here — the only illumination comes from creatures that make their own. Bioluminescent flashes in absolute darkness, anglerfish lures, the faint glow of volcanic vents on the distant horizon.

The pressure is immense. Every sound carries the weight of the entire water column above it. Down here, bass isn't just a frequency — it's a physical force. Sunken ships creak under the weight. Ancient creatures dart between the wreckage, bouncing off walls, flashing in panic.

The 808 kick at this depth isn't a drum hit. It's the heartbeat of the Earth.

**Depth zone:** Zone 9 — Hadal zone (6,000m+ depth) — the absolute floor
**Water column position:** Deepest of all 34 engines
**Counterpart:** OPENSKY (direct polar opposite — highest point in the column)

---

## 9. Preset Strategy (150 factory presets)

| Category | Count | Character |
|----------|-------|-----------|
| 808 Abyss | 25 | 808 bass reimagined as geological events. Earth-shaking sub. Pure Oscar rhythm-bass. |
| Sunken Treasure | 20 | Metallic, creaking, shipwreck resonances with sub foundation. WRECK-focused. |
| Creature Feature | 20 | Darting, flashing, bioluminescent creature textures. High CREATURE, STILLNESS=0. |
| Pressure Drop | 20 | Pure sub weight — minimal harmonics, maximum PRESSURE and DEPTH. |
| Dark Ambient | 20 | Trench drones, rumble textures, abyssal stillness. STILLNESS=1, DARKNESS=1. |
| The Full Column | 15 | Designed specifically for OCEANDEEP × OPENSKY coupling. The mythology preset set. |
| Coupling Showcases | 15 | Cross-engine pairs: OVERDUB, ORACLE, ORIGAMI, OPAL. |
| Leviathan | 15 | Maximum everything — all macros up, all creatures awake, the kraken wakes. |

### Naming Convention
Names should feel like descent:
- "10,000 Leagues" / "Sunken Cathedral" / "Creature In The Dark"
- "The Pressure Down Here" / "Oscar's Throne" / "Shiver Me Timbers"
- "Bioluminescent" / "The Floor" / "Hadal Zone" / "Anglerfish"
- "Earth's Heartbeat" / "Leviathan Wakes" / "Ventolin Deep"
- "Full Column" (for coupling showcases)

---

## 10. Visual Identity

- **Accent color:** Trench Violet `#2D0A4E`
- **UI concept:** The darkest panel in the XO_OX gallery. Near-black background with trench violet accents and bioluminescent cyan highlights that fire on transient events. Parameter changes should trigger brief cyan glow pulses — bioluminescent creatures responding to stimulus.
- **Color palette:** Near-black base (`#0A0612`), trench violet accents (`#2D0A4E`), bioluminescent cyan highlights (`#00CED1`) for active states and flash events. The opposite of OPENSKY in every visual choice.
- **The Full Column visual:** OCEANDEEP + OPENSKY displayed together should evoke the complete water column — trench violet at the bottom, sunburst orange at the top.

---

## 11. Mood Affinity

| Mood | Affinity | Why |
|------|----------|-----|
| Foundation | High | Sub bass is THE foundation of the gallery |
| Atmosphere | Medium | Dark ambient / trench drone presets |
| Entangled | High | The Full Column coupling showcase — OCEANDEEP × OPENSKY |
| Prism | Low | Nothing bright or prismatic lives at depth |
| Flux | Medium | Creature movement presets feel unstable and alive |
| Aether | High | Abyssal stillness — STILLNESS=1 presets exist in profound quiet |
| Family | Low | Too specialized / extreme for family-oriented patches |

---

## 12. Phase 1 Build Readiness

### Scaffold Checklist

- [ ] Create `Source/Engines/OceanDeep/` directory structure
- [ ] Write `OceanDeepEngine.h` implementing `SynthEngine` interface
- [ ] Sub oscillator stack: three-layer design (primary sub, secondary sub, harmonic)
- [ ] Hydrostatic compressor: ratio/attack/makeup controlled by `deep_pressure`
- [ ] Waveguide body: four environments via comb filter networks (shipwreck/trench/cave/abyss)
- [ ] Bioluminescent exciter: independent transient generator with bandpass
- [ ] Creature modulation: Brownian random-walk LFO targeting pitch/filter/pan
- [ ] Darkness filter: 24dB LP with key-track and envelope, matched-Z coefficients
- [ ] Rumble generator: sub-20Hz sine blend, post-filter
- [ ] Pressure reverb + creature delay in FX chain
- [ ] All 75 `deep_` parameters wired to APVTS
- [ ] Four macros (DEPTH/PRESSURE/DARKNESS/STILLNESS) confirmed audible
- [ ] Register in `EngineRegistry.h` → add to `CLAUDE.md` engine table
- [ ] 150 factory presets in `.xometa` format across 8 categories

### DSP Dependencies (reusable from existing engines)

| Component | Source |
|-----------|--------|
| Sub oscillator shapes | Core DSP / OVERDUB sine foundation |
| Compressor stage | ORIGAMI fold compression patterns |
| Comb filter / waveguide | OBSCURA physical modeling patterns |
| Random-walk LFO | ORACLE stochastic modulation |
| Reverb | OVERDUB spring reverb structure |
| Matched-Z LP filter | Fleet standard — `exp(-2*PI*fc/sr)` |

### Critical DSP Rules for This Engine

- **Always pass `sampleRate`** — never hardcode 44100. The rumble generator's 8–20Hz range is sample-rate dependent.
- **Bipolar modulation guard:** creature pitch modulation uses `!= 0` check on depth, not `> 0` — creature can push pitch downward.
- **`?? 0` not `|| 0`** when indexing sub oscillator Float32Array — `0.0` (silence) is a valid sample.
- **`cancelAndHoldAtTime`** for hydrostatic compressor envelope interruption — `cancelScheduledValues` causes clicks on pressure changes.
- **Denormal protection** in all waveguide feedback paths — comb filters accumulate denormals under silence.

### Phase Gate: Phase 1 → Phase 2 (DSP Build)

- [ ] All 75 parameters compile without error
- [ ] Dry signal (body mix = 0, reverb = 0) produces audible sub bass
- [ ] DEPTH macro sweeps register cleanly (no aliasing artifacts)
- [ ] PRESSURE macro produces audible dynamic change
- [ ] DARKNESS macro closes the filter audibly
- [ ] STILLNESS macro at 1.0 produces zero modulation

### Phase Gate: Phase 2 → Phase 3 (Preset)

- [ ] All D001–D006 doctrines satisfied:
  - D001: Velocity drives darkness filter brightness
  - D002: 2 LFOs + mod wheel + aftertouch + 4 macros functional
  - D003: Waveguide body physics cited (Schroeder + comb)
  - D004: All 75 parameters affect audio output
  - D005: LFO rate floor ≤ 0.001 Hz (infrasonic LFO satisfies this)
  - D006: Velocity→timbre + aftertouch CC wired
- [ ] auval PASS
- [ ] 150 factory presets complete

---

*XO_OX Designs | Engine: OCEANDEEP | Accent: #2D0A4E | Prefix: deep_ | Phase 1 Architecture*
