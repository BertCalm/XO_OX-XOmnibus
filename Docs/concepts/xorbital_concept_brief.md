# XOrbital — Concept Brief

**Date:** March 2026
**Phase:** 0 — Ideation complete
**Status:** Approved for Phase 1 architecture

---

## Identity

- **Name:** XOrbital
- **Gallery code:** ORBITAL
- **Engine ID:** `"Orbital"` (for `getEngineId()`, preset `engines[]`, and `REGISTER_ENGINE`)
- **Parameter prefix:** `orb_`
- **Thesis:** XOrbital is an additive synthesis engine that builds sound from individual harmonic orbits — each partial a satellite in a living spectral system, directly visible and sculptable.
- **Sound family:** Spectral pad / Crystal lead / Organ / Evolving texture hybrid
- **Unique capability:** Direct harmonic-level control with spectral coupling — any other engine's audio can reshape XOrbital's harmonic spectrum in real time via the coupling bus. OVERWORLD's chip harmonics superimpose onto ORBITAL's partials. ONSET's kick sculpts which harmonics ring. No other synth platform offers cross-synthesis-method spectral coupling.

---

## Context — Why This Engine

Every existing XOceanus engine synthesizes in the **time domain** — oscillators, wavetables, FM operators, granular schedulers, sample playback. None gives the user direct access to individual harmonics. XOrbital fills the **spectral domain** gap — the first engine where you sculpt the frequency content directly, not through filters applied after the fact.

The design draws from three reference instruments:
- **Kawai K5000** — playability: formant filter, macro spectral shaping, grouped harmonic editing
- **Synclavier II** — structure: multi-layer partial timbres, FM hybrid, resynthesis
- **Harmor** — workflow: "subtractive" additive filtering, visual spectral editing

The goal is to solve the "complexity problem" — additive synthesis offers infinite control but is historically tedious to program. XOrbital solves this through spectral profiles, formant filters, and grouped editing, making 64 harmonics as playable as a single oscillator.

---

## Character

- **Personality in 3 words:** Luminous, Evolving, Precise
- **Engine approach:** Additive synthesis — 64-partial sine bank with spectral profile morphing, formant filtering, and per-partial FM
- **Why additive serves the character:**
  An orbital is defined by its energy level — specific, quantized, precise. Harmonics behave the same way: each partial occupies a specific frequency, a specific amplitude, a specific phase. Where other engines blur these distinctions (wavetables average, FM scatters, granular fragments), XOrbital preserves them. The character is in the *precision* — every harmonic individually addressable, every spectral change intentional.
- **The coupling thesis:**
  Alone, XOrbital produces pristine harmonic spectra — organs, bells, glass, evolving pads. But its real power is as a **spectral lens** for the XOceanus gallery. Other engines' audio, envelopes, and rhythms can reach *inside* ORBITAL's spectrum and sculpt individual harmonics. No filter or effect can do this — only additive synthesis exposes the harmonic structure directly. Coupling doesn't just modulate ORBITAL; it reveals spectral relationships between engines that are invisible in any other architecture.

---

## The XO Concept Test

1. **XO word:** XOrbital ✓ — Orbital: electron shell, celestial path, harmonic energy level. Physics metaphor directly maps to additive synthesis (partials at specific energy/frequency levels).
2. **One-sentence thesis:** "XOrbital is an additive synthesis engine that builds sound from individual harmonic orbits — each partial a satellite in a living spectral system, directly visible and sculptable." ✓
3. **Sound only this can make:** Real-time spectral coupling — OVERWORLD's NES pulse odd-harmonic fingerprint superimposed onto a 64-partial organ chord, with ONSET's kick envelope sculpting which harmonics decay fastest. Three synthesis methods interacting at the harmonic level. No DAW plugin or other XOceanus engine does this. ✓

---

## Gallery Gap Filled

| Existing engines | Synthesis domain |
|-----------------|-------------------|
| ODDFELIX, ODDOSCAR, ODYSSEY, OBLONG, OBESE, OVERWORLD | Time-domain harmonic (oscillators, wavetables, FM, samples, chip) |
| OVERDUB | Time-domain temporal (delays, tape echo) |
| ONSET | Time-domain percussive (transient synthesis) |
| OPAL | Time-domain granular (grain scheduling) |
| **ORBITAL** | **Spectral domain — direct harmonic control** |

No current engine gives users access to individual harmonics. ORBITAL introduces partial amplitudes, spectral profiles, and formant filtering as musical parameters rather than DSP internals.

---

## Core DSP Architecture (Phase 0 Sketch)

```
MIDI Note → Fundamental frequency
     │
     ▼
Partial Bank (64 partials per voice, up to 6 voices)
├── Profile A amplitudes (64-point harmonic spectrum)
├── Profile B amplitudes (64-point harmonic spectrum)
├── A↔B Morph (continuous blend: amp[k] = lerp(A[k], B[k], morphPos))
├── Per-partial FM (global FM index + ratio, applied per partial)
├── Inharmonicity (stretch tuning: ratio[k] = sqrt(1 + B·k²), cached at noteOn)
└── Per-group envelope (4 groups: partials 1-8, 9-16, 17-32, 33-64)
     │
     ▼
Spectral Envelope / Formant Filter (per-partial amplitude multiply — zero CPU cost)
├── Spectral tilt (brightness rolloff curve)
├── Odd/Even harmonic balance
├── Formant shape (vowel presets: A/E/I/O/U + resonance curves)
└── Formant frequency shift (transpose the spectral envelope)
     │
     ▼
Partial Sum (per-sample: 64 × fastSin → stereo with per-partial pan spread)
     │
     ▼
Post Filter (CytomicSVF — LP/BP/HP, on summed output, coeffs cached per block)
     │
     ▼
Amp Envelope (ADSR)
     │
     ▼
Character Stage (Saturator — adds analog warmth to pure sine spectrum)
     │
     ▼
Output Cache (outputCacheL/R vectors for per-sample coupling export)
     │
     ▼
Output (stereo)
```

### Voice Model

Each MIDI note triggers a 64-partial additive voice pitched to that note's fundamental. Up to 6 simultaneous voices (polyphonic). Voice stealing: oldest note.

### Partial Bank Details

- **Phase accumulators:** `double` precision per partial to prevent drift at low frequencies over sustained notes
- **Inharmonic ratios:** Precomputed at `noteOn()` as `float inharmRatio[64]`; `freq[k] = k × f0 × inharmRatio[k]`. No `sqrt()` in the render loop.
- **Profile morph:** At render time, `amp[k] = lerp(profileA[k], profileB[k], morphPos)`. No extra storage — computed inline.
- **FM modulation:** Optional per-partial: `phase[k] += phaseInc[k] + fmIndex × fastSin(fmPhase[k])`. FM phase runs at `k × f0 × fmRatio`.
- **Denormal protection:** `flushDenormal()` on every per-partial phase accumulator state, every feedback path.
- **Stereo spread:** Per-partial pan coefficient: `panL[k] = cos(panAngle[k])`, `panR[k] = sin(panAngle[k])`. Pan angles spread outward from center as partial index increases (controlled by M4 SPACE).

### Formant Filter

Not a traditional audio filter — a spectral envelope applied as amplitude multipliers per partial. Computationally free (just 64 multiplies, which are already part of the amplitude computation).

- **Vowel presets:** A, E, I, O, U — stored as 64-point amplitude curves matching human vocal formant frequencies
- **Custom curve:** User-definable 64-point spectral envelope (UI: grouped editing interface)
- **Formant shift:** Transpose the entire spectral envelope up/down in frequency (shifts which partials are emphasized)
- **Odd/Even balance:** Multiply even partials by a scalar (0 = odd-only square-ish, 1 = all equal, 2 = even-emphasized)

### Per-Group Envelopes

Partials are grouped for envelope control (editing 64 individual envelopes is the complexity problem — groups solve it):

- **Group 1:** Partials 1–8 (fundamental region — body)
- **Group 2:** Partials 9–16 (mid harmonics — character)
- **Group 3:** Partials 17–32 (upper harmonics — brightness)
- **Group 4:** Partials 33–64 (air harmonics — shimmer)
- **Alternate grouping modes:** Odd/Even, Custom ranges (UI feature)

Each group has independent Attack and Decay times. All groups share the global ADSR's Sustain and Release. This means: upper partials can have a sharp attack and fast decay (bell-like) while lower partials sustain (organ-like). One preset parameter set creates dramatically evolving timbres.

### CPU Budget

- 64 partials × 6 voices = 384 fastSin calls/sample
- fastSin: ~12-15 ops each → ~5,760 ops/sample for sine bank
- FM (when active): doubles sine calls → ~11,520 ops/sample
- Phase updates, amplitude multiplication, stereo panning: ~2,000 ops/sample
- Per-group envelopes: ~500 ops/sample
- Post filter (1× CytomicSVF): ~50 ops/sample
- ADSR + Saturator: ~200 ops/sample
- **Total estimate:** ~14,000 ops/sample → ~620M ops/sec at 44.1kHz
- **Single core at 3GHz:** ~20% utilization (with FM active)
- **Without FM:** ~10% utilization
- **Margin:** Within 15% budget without FM; FM is an optional feature that users enable per-preset

---

## Parameter Namespace

All parameter IDs use `orb_` prefix. Key parameters:

| ID | Range | Description |
|----|-------|-------------|
| `orb_profileA` | enum (0-7) | Harmonic profile A preset (Sawtooth, Square, Triangle, Bell, Organ, Vocal, Glass, Custom) |
| `orb_profileB` | enum (0-7) | Harmonic profile B preset |
| `orb_morph` | 0–1 | Profile A↔B blend position |
| `orb_brightness` | 0–1 | Spectral tilt (rolloff rate of upper partials) |
| `orb_oddEven` | 0–1 | Odd/Even harmonic balance (0.5 = equal) |
| `orb_formantShape` | enum (0-6) | Formant preset (Off, A, E, I, O, U, Custom) |
| `orb_formantShift` | -24–+24 st | Formant frequency shift in semitones |
| `orb_inharm` | 0–1 | Inharmonicity coefficient B (0 = pure harmonic, 1 = bell-stretched) |
| `orb_fmIndex` | 0–1 | FM modulation depth per partial (0 = off) |
| `orb_fmRatio` | 0.5–8.0 | FM modulator frequency ratio |
| `orb_groupAttack1` | 0.001–4s | Group 1 (partials 1-8) attack time |
| `orb_groupDecay1` | 0.01–4s | Group 1 decay time |
| `orb_groupAttack2` | 0.001–4s | Group 2 (partials 9-16) attack time |
| `orb_groupDecay2` | 0.01–4s | Group 2 decay time |
| `orb_groupAttack3` | 0.001–2s | Group 3 (partials 17-32) attack time |
| `orb_groupDecay3` | 0.01–2s | Group 3 decay time |
| `orb_groupAttack4` | 0.001–1s | Group 4 (partials 33-64) attack time |
| `orb_groupDecay4` | 0.01–1s | Group 4 decay time |
| `orb_filterCutoff` | 20–20000 Hz | Post-synthesis SVF filter cutoff |
| `orb_filterReso` | 0–1 | Post filter resonance |
| `orb_filterType` | enum (0-2) | LP / BP / HP |
| `orb_stereoSpread` | 0–1 | Per-partial stereo pan spread (0 = mono center, 1 = full spread) |
| `orb_saturation` | 0–1 | Character stage saturation amount |
| `orb_ampAttack` | 0.001–8s | Global ADSR attack |
| `orb_ampDecay` | 0.05–4s | Global ADSR decay |
| `orb_ampSustain` | 0–1 | Global ADSR sustain |
| `orb_ampRelease` | 0.05–8s | Global ADSR release |
| `orb_volume` | -inf–+6dB | Engine output level |

*Full parameter list refined in Phase 1 architecture.*

---

## Macro Mapping (M1–M4)

| Macro | Label | Controls | Behavior |
|-------|-------|----------|----------|
| M1 | SPECTRUM | `orb_brightness` + `orb_oddEven` | 0 = dark fundamental only. 0.5 = warm balanced. 1 = all harmonics blazing. The "what harmonics are present" dial. |
| M2 | EVOLVE | `orb_morph` + group envelope speeds (inverse) | 0 = Profile A, static, fast envelopes. 1 = Profile B, evolving, slow envelopes. The "how the spectrum changes over time" dial. |
| M3 | COUPLING | `orb_formantShift` + coupling input depth | Controls how external engines influence the spectrum. At 0, ORBITAL is isolated. At 1, coupling fully reshapes the harmonic profile. |
| M4 | SPACE | `orb_stereoSpread` + reverb send | 0 = mono, intimate. 1 = partials spread across the stereo field with spatial depth. "How wide the orbits are." |

All 4 macros produce audible, significant change at every point in their range in every preset.

---

## Coupling Interface Design

### ORBITAL as Target (receiving from other engines)

| Coupling Type | What XOrbital Does | Musical Effect |
|---------------|-------------------|----------------|
| `AudioToWavetable` | Maps source audio amplitude to partial amplitude offsets — source energy at harmonic frequencies boosts corresponding partials | **The killer feature.** Any engine's harmonic fingerprint superimposes onto ORBITAL's spectrum. OVERWORLD's chip DNA transfers to additive harmonics. |
| `AmpToFilter` | Source amplitude → formant filter shift | Drum hits shift the formant vowel. Crescendos brighten the spectrum. Rhythmic spectral animation. |
| `EnvToMorph` | Source envelope → Profile A↔B morph position | Another engine's dynamics directly animate ORBITAL's timbral evolution. Build from dark to bright over a phrase. |
| `AudioToFM` | Source audio becomes FM modulator for ORBITAL's partials | External audio FM-bends individual harmonics. ODDFELIX click on bell attack = "glassy crunch." |
| `AudioToRing` | Ring modulation: ORBITAL output × source audio | Creates metallic, inharmonic sidebands around each partial. ODDFELIX ring × ORBITAL bell = new territory. |
| `RhythmToBlend` | Source rhythm pattern animates harmonic group enables | Rhythmic gating of harmonic groups — upper partials pulse with the beat. Spectral rhythm. |
| `LFOToPitch` | Source LFO → fundamental pitch offset | Standard vibrato from external engine. |
| `PitchToPitch` | Source pitch → fundamental pitch offset | Harmony — another engine's pitch offsets ORBITAL's fundamental. |
| `EnvToDecay` | Source envelope → per-group decay times | Percussion sculpts how fast bell harmonics ring down. ONSET → ORBITAL: kick shortens upper partial decay. |

**Primary coupling:** `AudioToWavetable` — spectral coupling is the reason ORBITAL exists in XOceanus. Every engine feeding into ORBITAL creates a unique "engine DNA transfer" voice.

### ORBITAL as Source (sending to other engines)

`getSampleForCoupling()` returns: post-filter, post-saturation output from `outputCacheL[sampleIndex]` / `outputCacheR[sampleIndex]`. Channel 2: envelope level for `AmpToFilter`/`AmpToChoke` routes.

Uses the per-sample `outputCacheL/R` vector pattern (like ODDFELIX/ODDOSCAR), not `lastSampleL/R`, enabling tight per-sample coupling for AudioToFM and AudioToRing routes.

Best receiving engines:
- **OPAL** — ORBITAL's pure harmonic output scattered through time = spectral granular
- **OVERDUB** — Clean additive through tape delay = antique sine organ. Mathematical precision corrupted by analog memory.
- **ODDOSCAR** — ORBITAL's harmonics as FM source for ODDOSCAR's wavetable oscillator = hybrid character

### Coupling types ORBITAL should NOT receive
- `AmpToChoke` — choking kills all harmonics simultaneously (no musical use; you want per-group decay control instead, which `EnvToDecay` provides)
- `FilterToFilter` — ORBITAL's formant filter operates in the spectral domain, not the time domain; a filter-to-filter route doesn't translate meaningfully

---

## Top 3 Coupling Pairings

### 1. OVERWORLD × ORBITAL — "Chip DNA Transfer"
- **Route:** OVERWORLD audio → ORBITAL via `AudioToWavetable`
- **Musical effect:** NES pulse waves have strong odd harmonics (1, 3, 5, 7). When mapped to ORBITAL's partial amplitudes, the chip harmonic fingerprint superimposes onto the additive spectrum. Result: an additive organ with chip DNA — 8-bit character rendered through 64 precisely controlled harmonics.
- **Why it's special:** Two "harmonic precision" engines interacting at the spectral level. Neither engine alone can produce these timbres. No other synth platform offers this cross-synthesis-method spectral coupling. This pairing should lead the marketing narrative.

### 2. ONSET × ORBITAL — "Rhythm Sculpts Spectrum"
- **Route:** ONSET envelope → ORBITAL `AmpToFilter` + ONSET audio → ORBITAL `AudioToFM`
- **Musical effect:** Every kick opens the spectral formant. Every hat brightens upper partials. The kick's transient click FM-bends bell partials on the attack only. Result: bells and organs that respond to groove — the rhythm sculpts the timbre, not just the amplitude.
- **Why it's special:** Percussion directly shapes harmonic content. In a subtractive synth, a sidechain just pumps volume. In ORBITAL, it reshapes *which frequencies are present*.

### 3. OPAL × ORBITAL — "Spectral Granular"
- **Route:** ORBITAL output → OPAL grain buffer via `AudioToWavetable`
- **Musical effect:** OPAL scatters ORBITAL's precise sine partials through time. Individual harmonics arrive at different moments, creating beating and phasing phenomena unique to granulated additive signals. Pure additive sine waves are the ideal granular source because each harmonic is cleanly separable.
- **Why it's special:** A synthesis method that essentially doesn't exist — granular decomposition of individually addressed harmonics. The precision of additive meets the chaos of granular.

### Honorable Mentions
- **OVERDUB × ORBITAL** — "Antique Sine Organ": ORBITAL's mathematical precision corrupted by OVERDUB's tape wow/flutter. Tape modulates timing between phase-coherent partials, creating a living tremolo distinct from vibrato.
- **ODDOSCAR × ORBITAL** — "Glass with a Wooden Body": ODDOSCAR's Moog-filtered output as FM modulator for ORBITAL's upper partials. Warm, resonant FM source creates Synclavier-like glass tones.
- **ODDFELIX × ORBITAL** — "Glassy Crunch": ODDFELIX's Karplus-Strong pluck audio FM-modulating ORBITAL's bell partials. Transient crunch dissolving into sustained shimmer.

---

## Visual Identity

- **Accent color:** Emission Coral `#FF6B6B`
  - The color of hydrogen-alpha spectral emission (656nm) — created when electrons change orbitals
  - Directly connects the accent color to the engine name's physics metaphor
  - Maximally distinct from all 9 existing engine colors (no warm red/coral in the gallery)
  - Reads clearly on the warm white Gallery Model shell
- **Material/texture:** Orbital diagrams — concentric ellipses at different energy levels, electron probability clouds
- **Panel character:** Clean, precise, scientific. Concentric ring motifs. Partial amplitudes visualized as radial bar graphs around a central fundamental.
- **Icon concept:** Concentric orbital rings with dots (partials) at different radii and brightness levels. The Bohr model of the atom, but for sound.

---

## Mood Affinity

| Mood | Affinity | Why |
|------|----------|-----|
| Foundation | Medium | Organs, spectral bass, clean keys — the solid ground |
| Atmosphere | **High** | Evolving spectral pads, formant drones, slow morphs |
| Entangled | **High** | Coupling showcases — spectral coupling is ORBITAL's superpower |
| Prism | **Very High** | Crystal bells, bright spectral leads, FM glass — ORBITAL's home turf |
| Flux | Medium | Animated spectra, rapid morph, extreme inharmonicity |
| Aether | **High** | Pure sine clouds, frozen harmonics, suspended single-partial tones |

Primary moods: Prism, Atmosphere, Entangled, Aether.

---

## Preset Strategy (Phase 0 Sketch)

**150 presets at v1.0** — coupling-heavy (40% of presets are cross-engine):

| Category | Count | Character |
|----------|-------|-----------|
| Orbital Organs | 15 | Drawbar-style harmonic mixing — warm, immediate, playable. The K5000 heritage. |
| Crystal Structures | 15 | Bells, glass, chimes — inharmonic partials, sharp group attacks, sustained lows. |
| Formant Voices | 15 | Vocal-like formant filter shaping, choir textures, vowel morphs via M2. |
| Spectral Pads | 15 | Evolving A→B profile morphs, slow group envelopes, cinematic sustain. |
| Synclavier Glass | 10 | FM hybrid tones — glassy sparkle, pristine digital character, Synclavier homage. |
| Spectral Bass | 10 | Controlled low-harmonic content, clean fundamental, warm sub-territory. |
| Abstract Orbits | 10 | Extreme inharmonicity, spectral animation, experimental sound design. |
| **OVERWORLD × ORBITAL** | **12** | Chip DNA transfer — NES/Genesis/SNES harmonic fingerprints on additive spectrum |
| **ONSET × ORBITAL** | **12** | Rhythm sculpts spectrum — percussion envelopes shape harmonic content |
| **OPAL × ORBITAL** | **12** | Spectral granular — additive harmonics scattered through time |
| **OVERDUB × ORBITAL** | **8** | Antique sine organ — mathematical precision through tape delay |
| **ODDFELIX × ORBITAL** | **8** | Glassy crunch — Karplus-Strong + additive hybrid |
| **ODDOSCAR × ORBITAL** | **4** | Glass with wooden body — warm filtered FM on additive harmonics |
| **ODYSSEY × ORBITAL** | **4** | Odyssey spectrum — formant-on-formant, journey-driven spectral morph |

---

### Standalone Presets (90)

#### Orbital Organs (15)

| Preset Name | Mood | What It Does |
|-------------|------|-------------|
| "Helium Cathedral" | Atmosphere | Drawbar registration (partials 1,2,4,8), "oo" formant, breathing group envelopes. Warm, not clinical. |
| "Full Draw" | Foundation | All 8 drawbar partials at unity — the definitive Hammond-esque additive organ. Rich, full, no FM. |
| "Sine Chapel" | Aether | Pure fundamental + octave only, wide stereo spread, cathedral reverb. Minimal and sacred. |
| "Brass Section" | Foundation | Square-ish profile (odd harmonics dominant), bright formant, staccato group envelopes. Punchy organ brass. |
| "Jazz Club" | Atmosphere | Warm drawbar registration with 3rd harmonic emphasis, slight saturation, mellow formant. After-hours tone. |
| "Rotary Drive" | Prism | Organ harmonics with LFO-driven formant shift simulating Leslie cabinet doppler. Spinning, alive. |
| "Pipe Dream" | Aether | Open diapason registration — fundamental + 2nd + 4th partials, "oo" formant, long release. Church pipe organ. |
| "Neon Worship" | Prism | Bright registrations with 16' + 8' + 4' + 2' partials, "ah" formant, slight FM sparkle. Modern worship keys. |
| "Third Rail" | Foundation | Aggressive 3rd-harmonic organ with saturation, tight envelopes, punchy attack. Power organ for rock. |
| "Prayer Wheel" | Atmosphere | Slowly evolving odd/even balance via M2, "oh" formant, long sustain. Meditative cycling harmonics. |
| "Transistor" | Prism | Clicky organ attack (fast group 3-4, slow group 1-2), bright, tight, combo-organ character. |
| "Spectral Flute" | Aether | Fundamental-heavy with octave overtone, "oo" formant, breathy saturation. Pure, airy, solo voice. |
| "Drawbar Drift" | Flux | Random slow modulation of individual drawbar levels. The organ tunes itself. Generative. |
| "Warm Keys" | Foundation | Balanced harmonic profile, moderate saturation, gentle formant. An all-purpose additive keys preset. |
| "Odd Interval" | Prism | Only odd harmonics (1,3,5,7,9...) — square wave approximation via additive, pure and hollow. |

#### Crystal Structures (15)

| Preset Name | Mood | What It Does |
|-------------|------|-------------|
| "Frozen Bell Choir" | Prism | High inharmonicity, 6-voice cluster, A=bell/B=harmonic morph. The A/B morph demo. |
| "Glass Harmonics" | Prism | Pure harmonic series, fast attack on groups 3-4, slow decay. Struck glass, singing bowls. |
| "Tubular Bells" | Foundation | Strong fundamental with moderate inharmonicity, percussive group envelopes. Classic metal bell. |
| "Wind Chimes" | Atmosphere | High inharmonicity, sparse voicing, random-ish detuning between voices. Breeze through bells. |
| "Crystal Cave" | Aether | Sustained bell harmonics with long release, high stereo spread. Partials echoing in space. |
| "Temple Gong" | Foundation | Low fundamental, heavy inharmonicity, all groups slow attack. Deep resonant metal. |
| "Ice Prism" | Prism | Very bright spectral tilt, fast upper group decay, no saturation. Cold, sharp, digital. |
| "Music Box" | Prism | High register, moderate inharmonicity, fast attack, medium decay. Toy-like precision. |
| "Gamelan Ring" | Flux | Extreme inharmonicity, paired detuned voices, slow beating. Indonesian metal percussion. |
| "Steel Pan" | Prism | Medium inharmonicity, bright attack, warm sustain. Caribbean steel drum via additive. |
| "Vibraphone Ghost" | Atmosphere | Bell profile with "ah" formant overlay, slow tremolo via group envelope modulation. Haunted vibes. |
| "Struck Quartz" | Prism | Pure crystalline attack, very high partials dominant, fast decay. Piezoelectric snap. |
| "Bronze Patina" | Atmosphere | Warm bell with saturation, slightly detuned partials, long sustain. Aged, oxidized tone. |
| "Crotales" | Prism | Very high register bells, tight inharmonicity, short decay. Orchestral antique cymbals. |
| "Singing Bowl" | Aether | Low inharmonicity, long sustain, slow beating between closely spaced partials. Meditation tone. |

#### Formant Voices (15)

| Preset Name | Mood | What It Does |
|-------------|------|-------------|
| "Braids" | Atmosphere | Profile A="ah", Profile B="ee", slow EVOLVE morph. Stacked voices = choir. |
| "Vowel Shift" | Flux | Formant morphs A→E→I→O→U via M2, fast modulation. Speaking spectrum. |
| "Cathedral Choir" | Aether | "Ah" formant, 6-voice unison detuned, wide spread, long reverb. Ethereal vocal pad. |
| "Throat Singer" | Atmosphere | Extreme formant emphasis on partials 2-6, tight "oh" formant, slight FM. Overtone singing. |
| "Whisper Field" | Aether | Very bright spectral tilt through "ee" formant, wispy, high stereo spread. Gossamer voices. |
| "Robot Choir" | Flux | Sharp formant transitions (no morph smoothing), metallic "ah", FM sparkle. Android vocals. |
| "Nasal Drone" | Atmosphere | Narrow "eh" formant, strong nasal resonance peak, sustained. Hurdy-gurdy character. |
| "Open Mouth" | Foundation | Wide "ah" formant, all harmonics present, natural warmth. The most vocal additive tone. |
| "Closed Throat" | Foundation | Tight "ee" formant, suppressed lower harmonics, slightly nasal. Constrained vocal quality. |
| "Spectral Siren" | Flux | Formant shift sweeping continuously via LFO, "oh" formant, eerie rising and falling. |
| "Choir Loft" | Atmosphere | Multi-voice "oo" with staggered group envelopes, wide stereo. Distant vocal ensemble. |
| "Formant Bass" | Foundation | Low register with "oh" formant, fundamental emphasis, warm saturation. Vocal bass tone. |
| "Glass Soprano" | Prism | High register, "ee" formant, FM sparkle, bright spectral tilt. Crystalline vocal lead. |
| "Resonant Body" | Atmosphere | Custom formant curve emphasizing body resonances (partials 3-8), warm, wooden. Acoustic body tone. |
| "Vowel Pad" | Atmosphere | Very slow A↔B morph between "ah" and "oo", long envelope, wide spread. Evolving vocal texture. |

#### Spectral Pads (15)

| Preset Name | Mood | What It Does |
|-------------|------|-------------|
| "Slow Orbit" | Atmosphere | Sawtooth→Bell morph over 12 seconds, slow group envelopes, maximum stereo spread. The demo sound. |
| "Aurora" | Aether | Bright upper partials slowly fading in (group 3-4 slow attack), fundamental immediate. Northern lights. |
| "Deep Field" | Aether | Only first 8 partials, very long attack and release, sub-heavy. Hubble deep space. |
| "Phase Space" | Flux | Rapid A↔B morph creating beating between profiles, slight inharmonicity. Quantum uncertainty. |
| "Thermal Glow" | Atmosphere | Warm spectral tilt, saturation, "oh" formant, long sustain. Radiant thermal pad. |
| "Zero Kelvin" | Aether | No saturation, pure sine, fundamental + 5th partial only, wide spread. Absolute cold. |
| "Spectral Dust" | Flux | Very high partials only (group 4), fast random group 4 envelopes, scattered. Particulate shimmer. |
| "Harmonic Cloud" | Atmosphere | All 64 partials at equal amplitude, slow global ADSR, wide spread. Dense harmonic mass. |
| "Event Horizon" | Aether | Fundamental only with extremely slow profile morph introducing upper partials over 30 seconds. Approaching singularity. |
| "Stellar Nursery" | Atmosphere | Bright but warm, moderate inharmonicity, slow group stagger, medium spread. New stars forming. |
| "Dark Matter" | Aether | Sub-fundamental emphasis, only odd partials, no high content, deep. Invisible mass. |
| "Solar Wind" | Flux | Bright spectral tilt with rapid group 4 envelope modulation, moving, energetic. Charged particles. |
| "Nebula Drift" | Atmosphere | Profile A=organ, Profile B=glass, very slow morph, formant shift wandering. Cosmic gas cloud. |
| "Cosmic String" | Prism | Single sustained note with all 64 partials creating dense beating patterns. Vibrating spacetime. |
| "Time Dilation" | Aether | Extremely slow everything — attack, morph, release. 20-second envelope. Relativity in sound. |

#### Synclavier Glass (10)

| Preset Name | Mood | What It Does |
|-------------|------|-------------|
| "The Glass Machine" | Prism | FM at 60%, ratio 2.0, slight stretch tuning. Fast upper group attack. The Synclavier tribute. |
| "Digital Ice" | Prism | High FM index, ratio 3.0, no inharmonicity. Pure digital sparkle, cold and precise. |
| "Fairlight Ghost" | Atmosphere | Low FM index, slow attack, "ah" formant. Ethereal 80s digital pad. |
| "Crystal Method" | Prism | FM ratio 1.5, Profile A=sawtooth, bright spectral tilt, snappy groups. Pop lead tone. |
| "New England" | Atmosphere | Classic Synclavier patch — FM sparkle on sustained additive bed, moderate morph. The original. |
| "Digital Marimba" | Prism | FM ratio 4.0, high inharmonicity, fast decay. Metallic mallet percussion. |
| "Additive Piano" | Foundation | Low FM, slight inharmonicity (B≈0.0003), fast attack all groups, natural piano-like decay. |
| "Glass Harp" | Prism | High FM, bright formant, plucky groups 3-4, sustained groups 1-2. Crystalline harp. |
| "Silicon Dreams" | Atmosphere | Moderate FM, slow morph from glass to organ, evolving formant. 80s sci-fi pad. |
| "Sparkle Lead" | Prism | High FM index, ratio 2.0, tight envelopes, bright. Cutting lead with digital shimmer. |

#### Spectral Bass (10)

| Preset Name | Mood | What It Does |
|-------------|------|-------------|
| "Sub Orbital" | Foundation | Fundamental + 2nd harmonic only, no FM, no inharmonicity, clean sub. Pure additive bass. |
| "Harmonic Floor" | Foundation | First 8 partials, moderate spectral tilt, "oh" formant, warm saturation. Solid bass foundation. |
| "Sine Bass" | Foundation | Pure fundamental sine, tight envelope, slight saturation. The simplest, cleanest bass. |
| "Growl" | Flux | Odd harmonics dominant, rapid formant shift via LFO, aggressive saturation. Spectral growl. |
| "Organ Pedal" | Foundation | 16' organ pedal registration, drawbar bass, long sustain, deep. Church organ low end. |
| "Mallet Bass" | Foundation | Bell profile with fast decay, low register, moderate inharmonicity. Marimba-like bass. |
| "Spectral Pluck" | Foundation | Fast attack all groups, medium decay, bright tilt. Additive bass pluck. |
| "Undertow" | Atmosphere | Fundamental with slow formant shift, long release, sub-heavy. Deep ocean current. |
| "Digital Thumb" | Foundation | Profile A=square, fast attack, tight decay, "oh" formant. Thumb-bass via additive. |
| "Warm Fuzz" | Foundation | High saturation on clean additive, 8 partials, warm spectral tilt. Fuzzy analog-ish bass. |

#### Abstract Orbits (10)

| Preset Name | Mood | What It Does |
|-------------|------|-------------|
| "Probability Cloud" | Flux | Random inharmonicity per note, random formant per note, no two notes alike. Quantum chaos. |
| "Spectral Fracture" | Flux | Extreme inharmonicity (B=1.0), all 64 partials, dense beating. Shattered spectrum. |
| "Metallic Breath" | Flux | Rapid group envelope cycling creating rhythmic metallic texture. Machine respiration. |
| "Partial Eclipse" | Aether | Upper partials slowly block lower partials via inverse group envelopes. Shadow across harmonics. |
| "Odd Attractors" | Flux | Only odd partials with non-harmonic FM ratios, creating strange tonal gravity. Mathematical chaos. |
| "Formant Cascade" | Flux | Rapid cycling through all 5 vowel formants. Speaking in tongues, spectral waterfall. |
| "Deorbiting" | Flux | Harmonics descending in amplitude from top to bottom over time. Spectral re-entry. |
| "Standing Wave" | Aether | Specific partial ratios creating resonance nodes, sustained, minimal. Room-mode simulation. |
| "Sine Swarm" | Flux | 64 slightly detuned near-unison partials creating dense beating. A cloud of sine bees. |
| "The Fundamental" | Aether | Pure sine. Just the fundamental. The origin of all sound. Elegant silence with one tone. |

---

### Cross-Engine Coupling Presets (60)

#### OVERWORLD × ORBITAL — "Chip DNA Transfer" (12)

| Preset Name | Mood | Coupling Route | What It Does |
|-------------|------|---------------|-------------|
| "Chip Cathedral" | Entangled | OW audio→ORB `AudioToWavetable` | NES pulse odd harmonics superimposed on organ registration. The hero preset — 8-bit DNA on a 64-partial cathedral. |
| "Pulse Spectrum" | Entangled | OW audio→ORB `AudioToWavetable` | NES 25% duty cycle fingerprint on bright additive lead. Chip character, additive precision. |
| "Genesis Glass" | Entangled | OW audio→ORB `AudioToWavetable` | Genesis FM metallic harmonics mapped onto ORBITAL's bell profile. Two FM methods colliding. |
| "SNES Warmth" | Entangled | OW audio→ORB `AudioToWavetable` | SNES BRR Gaussian-smoothed audio softening ORBITAL's upper partials. Console warmth on additive. |
| "Era Spectrum" | Entangled | OW audio→ORB `AudioToWavetable` + OW ERA morph | As OVERWORLD's ERA sweeps NES→Genesis→SNES, ORBITAL's spectrum evolves through three chip fingerprints. |
| "Pixel Organ" | Entangled | OW audio→ORB `AudioToWavetable` | NES triangle wave emphasizing ORBITAL's fundamental and octave. 8-bit meets pipe organ. |
| "Glitch Harmonics" | Flux | OW audio→ORB `AudioToWavetable` + OW glitch | OVERWORLD's glitch events corrupt ORBITAL's harmonic profile. Pristine organ periodically shatters. |
| "Chiptune Choir" | Entangled | OW audio→ORB `AudioToWavetable` | NES pulse through ORBITAL's "ah" formant. Chip synthesis learning to sing. |
| "Blast Processing" | Flux | OW audio→ORB `AudioToFM` | Genesis audio as FM modulator on ORBITAL partials. Metallic chip sparkle on clean harmonics. |
| "Console Crossfade" | Entangled | OW env→ORB `EnvToMorph` | OVERWORLD's note envelope drives ORBITAL's A↔B morph. Chip dynamics animate spectral evolution. |
| "Retro Formant" | Entangled | OW audio→ORB `AudioToWavetable` | NES pulse harmonics driving ORBITAL's formant region. Vowel shapes from chip DNA. |
| "Level Select" | Prism | OW audio→ORB `AudioToWavetable` | Bright OVERWORLD arp spectral fingerprint on sustained ORBITAL pad. Menu screen energy. |

#### ONSET × ORBITAL — "Rhythm Sculpts Spectrum" (12)

| Preset Name | Mood | Coupling Route | What It Does |
|-------------|------|---------------|-------------|
| "Kick Spectrum" | Entangled | ONSET amp→ORB `AmpToFilter` | Every kick hit shifts ORBITAL's formant from dark to bright. Rhythmic spectral breathing. |
| "Hat Harmonics" | Entangled | ONSET amp→ORB `AmpToFilter` | Hi-hat pattern gates ORBITAL's upper partials. Ticking spectral clock. |
| "Drum Organ" | Entangled | ONSET audio→ORB `AudioToFM` | ONSET's full kit FM-modulating ORBITAL's organ. Each drum hit bends different harmonics. |
| "Snare Sparkle" | Entangled | ONSET audio→ORB `AudioToFM` | Snare noise burst as FM source on ORBITAL's glass preset. Transient crackle on crystal. |
| "Groove Cathedral" | Entangled | ONSET amp→ORB `AmpToFilter` + env→ORB `EnvToDecay` | Full drum pattern drives both formant shift AND harmonic decay rates. Rhythm IS the organ. |
| "Pulse Bell" | Prism | ONSET env→ORB `EnvToDecay` | ONSET's percussive envelope shortens ORBITAL bell decay. Every hit re-strikes the bell. |
| "Spectral Sidechain" | Foundation | ONSET amp→ORB `AmpToFilter` | Kick inverts ORBITAL's brightness — ducking in the frequency domain, not amplitude. |
| "Rhythmic Vowels" | Flux | ONSET rhythm→ORB `RhythmToBlend` | ONSET's pattern gates harmonic groups on/off rhythmically. Speaking drum spectrum. |
| "Transient Glass" | Prism | ONSET audio→ORB `AudioToFM` | Kick transient click FM-bends ORBITAL bell partials on attack only. Glassy crunch dissolving to shimmer. |
| "Envelope Sculptor" | Entangled | ONSET env→ORB `EnvToMorph` | ONSET's envelope drives ORBITAL's profile morph. Percussion shapes timbre trajectory. |
| "Cymbal Ring" | Entangled | ONSET audio→ORB `AudioToRing` | ONSET cymbal ring-modulated against ORBITAL harmonics. Metallic sideband cascade. |
| "Breath and Beat" | Atmosphere | ONSET amp→ORB `AmpToFilter` | Slow percussion pattern with wide ORBITAL formant sweep. Meditative rhythm-spectrum dialogue. |

#### OPAL × ORBITAL — "Spectral Granular" (12)

| Preset Name | Mood | Coupling Route | What It Does |
|-------------|------|---------------|-------------|
| "Scattered Harmonics" | Atmosphere | ORB→OPAL `AudioToWavetable` | ORBITAL's 64 partials fed into OPAL's grain buffer. Individual harmonics scattered through time. |
| "Frozen Spectrum" | Aether | ORB→OPAL `AudioToWavetable` + OPAL freeze | ORBITAL chord frozen in OPAL's buffer. Sustained harmonic snapshot, grain-scattered. |
| "Grain Cathedral" | Atmosphere | ORB→OPAL `AudioToWavetable` | ORBITAL organ through dense OPAL grain cloud (120 grains/sec). Harmonic shimmer haze. |
| "Spectral Dust" | Aether | ORB→OPAL `AudioToWavetable` | Sparse OPAL grains (5/sec) picking individual ORBITAL partials. Harmonics as dust motes. |
| "Sine Scatter" | Flux | ORB→OPAL `AudioToWavetable` + pitch scatter | ORBITAL pure harmonics with OPAL pitch scatter ±12st. Clean tones becoming chaotic. |
| "Cloud Organ" | Atmosphere | ORB→OPAL `AudioToWavetable` | Large grain clouds (400ms) of ORBITAL organ. Smooth, smeared harmonics. Infinite sustain. |
| "Beating Grains" | Flux | ORB→OPAL `AudioToWavetable` | ORBITAL's closely-spaced upper partials granulated — creates complex beating and phasing. |
| "Bell Fragments" | Prism | ORB→OPAL `AudioToWavetable` | ORBITAL bell with high inharmonicity, OPAL scatter = shattered crystalline fragments. |
| "Reverse Spectrum" | Flux | ORB→OPAL `AudioToWavetable` + reverse grains | ORBITAL attack transients reversed by OPAL. Spectral swells that seem to anticipate the note. |
| "Harmonic Rain" | Atmosphere | ORB→OPAL `AudioToWavetable` + high density | Dense granulation of ORBITAL's upper partials. Individual harmonics falling like droplets. |
| "Glimmer Cloud" | Aether | ORB→OPAL `AudioToWavetable` | ORBITAL glass preset through OPAL with high position scatter. Sparkling, suspended particles. |
| "Spectral Freeze" | Aether | ORB→OPAL `AudioToWavetable` + OPAL freeze | Single ORBITAL note frozen indefinitely in OPAL. Living drone from a single keystroke. |

#### OVERDUB × ORBITAL — "Antique Sine Organ" (8)

| Preset Name | Mood | Coupling Route | What It Does |
|-------------|------|---------------|-------------|
| "Tape Cathedral" | Atmosphere | ORB→OVERDUB send | ORBITAL organ through OVERDUB's tape delay. Wow/flutter de-phases the harmonics. Vintage warmth. |
| "Echo Harmonics" | Atmosphere | ORB→OVERDUB send | ORBITAL bell delays — each echo loses upper partials through tape saturation. Decaying spectrum. |
| "Dub Spectrum" | Foundation | ORB→OVERDUB send + feedback | High OVERDUB feedback on ORBITAL bass. Self-oscillating harmonic delay. Spectral dub siren. |
| "Spring Glass" | Prism | ORB→OVERDUB spring reverb | ORBITAL glass through OVERDUB's spring model. Boingy crystalline textures. |
| "Worn Record" | Atmosphere | ORB→OVERDUB send + tape saturation | ORBITAL pad through maximum tape saturation. Harmonics compressed into warm analog glow. |
| "Ping Pong Orbits" | Prism | ORB→OVERDUB ping-pong | ORBITAL bell in stereo ping-pong delay. Harmonics bouncing between speakers. |
| "Delay Morph" | Entangled | OVERDUB amp→ORB `AmpToFilter` | OVERDUB's delay taps modulate ORBITAL's formant. Each echo shifts the vowel. |
| "Cassette Choir" | Atmosphere | ORB→OVERDUB send | ORBITAL choir formant through tape delay with heavy wow. Degraded beauty. |

#### ODDFELIX × ORBITAL — "Glassy Crunch" (8)

| Preset Name | Mood | Coupling Route | What It Does |
|-------------|------|---------------|-------------|
| "Pluck Bell" | Prism | ODDFELIX audio→ORB `AudioToFM` | ODDFELIX Karplus-Strong pluck FM-modulating ORBITAL bell partials. Transient crunch to sustained ring. |
| "String Spectrum" | Entangled | ODDFELIX env→ORB `EnvToMorph` | ODDFELIX's pluck envelope drives ORBITAL's A↔B morph. String attack shapes spectral evolution. |
| "Metallic Touch" | Prism | ODDFELIX audio→ORB `AudioToRing` | ODDFELIX ring-modulated against ORBITAL harmonics. Inharmonic metallic sidebands from a pluck. |
| "Click Glass" | Prism | ODDFELIX audio→ORB `AudioToFM` | ODDFELIX's filter click as brief FM burst on ORBITAL glass. Percussive sparkle on sustained shimmer. |
| "Harmonic Pluck" | Foundation | ODDFELIX audio→ORB `AudioToWavetable` | ODDFELIX's harmonic content mapped onto ORBITAL's partial amplitudes. Physical modeling meets additive. |
| "Resonant Orbit" | Entangled | ODDFELIX env→ORB `EnvToDecay` | ODDFELIX's envelope controls per-group decay in ORBITAL. Pluck articulation on spectral pad. |
| "Filtered Harmonics" | Atmosphere | ODDFELIX amp→ORB `AmpToFilter` | ODDFELIX's amplitude drives ORBITAL's formant shift. Pluck brightens the spectrum momentarily. |
| "Crunch Pad" | Atmosphere | ODDFELIX audio→ORB `AudioToFM` | Sustained ODDFELIX string FM-modulating a held ORBITAL pad. Warm crunch beneath clean harmonics. |

#### ODDOSCAR × ORBITAL — "Glass with Wooden Body" (4)

| Preset Name | Mood | Coupling Route | What It Does |
|-------------|------|---------------|-------------|
| "Moog Spectrum" | Entangled | ODDOSCAR audio→ORB `AudioToFM` | ODDOSCAR's Moog-filtered wavetable as FM modulator for ORBITAL's upper partials. Warm resonance on glass. |
| "Wavetable Harmonics" | Entangled | ODDOSCAR audio→ORB `AudioToWavetable` | ODDOSCAR's wavetable harmonic content shaping ORBITAL's partial amplitudes. Two harmonic worlds merged. |
| "Filter Breath" | Atmosphere | ODDOSCAR amp→ORB `AmpToFilter` | ODDOSCAR's filter sweep driving ORBITAL's formant position. Breathing in both spectral and timbral domains. |
| "Morphing Orbits" | Entangled | ODDOSCAR env→ORB `EnvToMorph` | ODDOSCAR's modulation envelope driving ORBITAL's profile morph. Wavetable dynamics animate additive timbre. |

#### ODYSSEY × ORBITAL — "Odyssey Spectrum" (4)

| Preset Name | Mood | Coupling Route | What It Does |
|-------------|------|---------------|-------------|
| "Journey Harmonics" | Entangled | ODYSSEY env→ORB `EnvToMorph` | ODYSSEY's Climax build envelope drives ORBITAL's A↔B morph. The journey from dark to bright harmonics. |
| "Formant Odyssey" | Entangled | ODYSSEY amp→ORB `AmpToFilter` | ODYSSEY's amplitude dynamics driving ORBITAL's formant shift. Two formant systems in dialogue. |
| "Spectral Voyage" | Atmosphere | ODYSSEY audio→ORB `AudioToWavetable` | ODYSSEY's wavetable/formant output mapped onto ORBITAL partials. Odyssey character rendered in additive. |
| "Climax Spectrum" | Entangled | ODYSSEY env→ORB `EnvToMorph` + ODYSSEY audio→ORB `AudioToFM` | Full ODYSSEY build: envelope morphs profile while audio FM-modulates partials. Maximum spectral drama. |

---

### Preset Library Summary

| Mood | Count | Primary Source |
|------|-------|---------------|
| Foundation | 22 | Organs, bass, percussive bells, ONSET coupling |
| Atmosphere | 36 | Evolving pads, formant voices, OPAL/OVERDUB coupling |
| Entangled | 38 | Cross-engine coupling (all pairings) |
| Prism | 28 | Crystal bells, FM glass, bright leads, ODDFELIX coupling |
| Flux | 18 | Animated spectra, experimental, glitch coupling |
| Aether | 18 | Pure tones, frozen harmonics, OPAL freeze coupling |
| **Total** | **150** | |

**Coupling impact:** 60 presets (40%) are coupling showcases — the highest ratio of any XOceanus engine. This reflects ORBITAL's unique position as a **spectral lens** that reveals hidden relationships between synthesis methods. Every engine in the gallery sounds different when coupled with ORBITAL because the coupling operates at the harmonic level, not just at the audio level.

**Gallery impact:** ORBITAL adds 38 Entangled presets to the gallery — nearly doubling the existing coupling-focused preset count. This reinforces XOceanus's core thesis that engines coupled together produce sounds impossible with any single synth.

### Five Signature Presets

| Preset Name | Category | What It Demonstrates |
|-------------|----------|---------------------|
| "Helium Cathedral" | Orbital Organs | Drawbar registration, "oo" formant, breathing envelopes. Proves additive can be warm, not clinical. |
| "Frozen Bell Choir" | Crystal Structures | High inharmonicity, 6-voice cluster, A/B morph. Demonstrates the morph system. |
| "Chip Cathedral" | OVERWORLD × ORBITAL | NES pulse → ORBITAL via AudioToWavetable. Chip DNA on organ harmonics. **The coupling hero preset.** |
| "Groove Cathedral" | ONSET × ORBITAL | Full drum pattern drives formant + decay. Rhythm IS the organ. **The rhythm-spectrum demo.** |
| "Scattered Harmonics" | OPAL × ORBITAL | 64 partials fed into OPAL grains. Individual harmonics scattered through time. **The spectral granular demo.** |

### The Killer Demo Sound

A single sustained C-E-G chord on a clean 64-partial preset. Moderate inharmonicity (B ≈ 0.0003). Formant set to bright "ah." M2 EVOLVE at 50% causing slow automatic morph from Profile A (sawtooth harmonics) to Profile B (bell harmonics) over 8 seconds. No reverb. No coupling. Just the spectrum unfolding.

**Why this works in 10 seconds:** The listener hears sustained notes, then notices the *timbre* is changing — not pitch, not amplitude, but the harmonic content itself. The "ah" formant glows through as a resonance. The notes gradually ring-ify as the bell profile emerges. The question "what did you do?" is the hook.

**Demo sequence:** Start with "Slow Orbit" solo/dry (10 sec). Cut to "Chip Cathedral" (10 sec). Cut to "Groove Cathedral" (10 sec). Cut to "Scattered Harmonics" (10 sec). Four sounds in 40 seconds = standalone + three coupling paradigms.

---

## Feature Scope

### v1 — Core (ship with XOceanus)
- 64-partial additive bank, 6 voices polyphonic
- 2 spectral profiles (A/B) with continuous morph
- Formant filter (spectral envelope with vowel presets)
- Spectral tilt + odd/even balance
- FM hybrid (per-partial FM with global index/ratio)
- Inharmonicity model (bell/piano stretch tuning)
- 4-group envelope system (partials 1-8, 9-16, 17-32, 33-64)
- Post-synthesis CytomicSVF filter
- Saturator character stage
- Full coupling interface (9 coupling types accepted)
- 150 factory presets (90 standalone + 60 cross-engine coupling showcases across 7 pairings)

### v2 — Aspirational (post-launch)
- **Resynthesis:** WAV → harmonic analysis (offline FFT process) → populate profiles A/B from real audio
- **Image synthesis:** Import 2D image → Y=frequency, X=time, brightness=amplitude → animate spectral profiles
- **Real-time spectrogram:** UI visualization of all 64 partials in real time (per-voice or summed)
- **Custom formant curves:** User-drawable 64-point spectral envelope (replaces preset vowels)
- **3rd spectral profile (C):** Triangle interpolation with X/Y pad navigation
- **Per-partial phase offsets:** Store phase data in profiles for richer A/B morph timbral variation

---

## Implementation Notes (for Phase 1 Architecture)

### Blocking prerequisites before preset authoring
- Add `"Orbital"` and `"XOrbital"` to `validEngineNames` in `Source/Core/PresetManager.h`

### Architecture decisions locked at Phase 0
- `getSampleForCoupling()` uses `outputCacheL/R` vector pattern (not `lastSampleL/R`) — enables per-sample tight coupling
- Inharmonic ratios precomputed at `noteOn()` and cached — no `sqrt()` in render loop
- Post filter on summed output (not per-voice), `setCoefficients()` called once per block
- `flushDenormal()` on every per-partial phase accumulator
- 6 voices (not 8) to maintain CPU headroom with FM active

### Critical files for implementation
| File | Role |
|------|------|
| `Source/Core/SynthEngine.h` | Interface contract (10 pure virtuals) |
| `Source/Core/EngineRegistry.h` | `REGISTER_ENGINE(OrbitalEngine)` |
| `Source/Core/PresetManager.h` | Add to `validEngineNames` |
| `Source/Engines/Opal/OpalEngine.h` | Reference pattern (closest structural analog) |
| `Source/DSP/FastMath.h` | `fastSin`, `flushDenormal`, `fastExp`, `midiToFreq` |
| `Source/DSP/CytomicSVF.h` | Post-synthesis filter |
| `Source/DSP/Effects/Saturator.h` | Character stage |
| `CMakeLists.txt` | Add `Source/Engines/Orbital/OrbitalEngine.cpp` |

---

## Decision Gate: Phase 0 → Phase 1

- [x] Concept brief written
- [x] XO word feels right (XOrbital — orbital physics metaphor maps directly to additive synthesis)
- [x] Gallery gap is clear (no spectral-domain synthesis engine exists)
- [x] At least 2 coupling partner ideas exist (OVERWORLD, ONSET, OPAL, OVERDUB, ODDFELIX, ODDOSCAR)
- [x] Unique capability defined (spectral coupling — harmonic DNA transfer between engines)
- [x] Excited about the sound

**→ Proceed to Phase 1: Architecture**
*Invoke: `/new-xo-engine phase=1 name=XOrbital identity="Additive synthesis engine that builds sound from individual harmonic orbits" code=ORBITAL`*

---

*XO_OX Designs | Engine: ORBITAL | Accent: #FF6B6B | Prefix: orb_*
