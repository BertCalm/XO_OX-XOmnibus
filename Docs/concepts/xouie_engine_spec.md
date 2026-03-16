# XOuïe — Engine Specification

**Gallery Code:** OUIE
**Accent Color:** Hammerhead Steel `#708090`
**Parameter Prefix:** `ouie_`
**Phase:** 1 — Architecture
**Date:** March 2026
**Status:** Ready for DSP scaffold

---

## 1. Concept

XOuïe (French: *ouïe* — the sense of hearing; also the gill slits of a fish) is a duophonic synthesis engine — two independent voices in constant dialogue. Not polyphony. Not unison detune. Two voices with a defined *relationship*: cooperation, competition, conversation.

The central thesis: **two brothers, two algorithms, one knob decides if they harmonize or destroy each other.**

This knob is HAMMER — the STRIFE↔LOVE axis. At full STRIFE, Voice A and Voice B fight for frequency space via cross-FM, ring modulation, and phase cancellation. At full LOVE, they merge via spectral blending, harmonic locking, and unison stacking. In between: every shade of sibling relationship from grudging coexistence to fierce argument to tender harmony.

No existing XOmnibus engine does this. Every other engine is monophonic or polyphonic with identical voice structure. OUIE is the first engine where the *relationship between voices* is the primary instrument.

**Sound family:** Leads / Duophonic Texture / Harmonic / Experimental
**Max polyphony:** 2 voices (duophonic — this IS the identity, not a limitation)
**Per-voice unison:** Up to 4 oscillators (8 oscillators maximum total)
**CPU budget:** < 8%

---

## 2. Architecture

### Signal Path

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
          │   (HAMMER macro M1)    │
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

**Smooth Brothers** — Harmonically predictable, blend-friendly, LOVE-affinity:

| # | Algorithm | Character | LOVE compatibility |
|---|-----------|-----------|-------------------|
| 0 | **Virtual Analog** | Classic saw/square/pulse/triangle shapes | High — spectral blend natural |
| 1 | **Wavetable** | Morphable wavetable with smooth interpolation | High — wavetable crossfade merges easily |
| 2 | **FM (2-op)** | 2-operator FM, ratio/depth control, bell to metallic | Medium — at low ratios, love-stable |
| 3 | **Additive** | 16-partial additive, per-partial level/detune, organ-to-glass | High — frequency content explicitly defined |

**Rough Brothers** — Harmonically unpredictable, conflict-prone, STRIFE-affinity:

| # | Algorithm | Character | STRIFE compatibility |
|---|-----------|-----------|---------------------|
| 4 | **Phase Distortion** | Casio CZ-style waveform bending, glassy to aggressive | High — phase offsets produce instability |
| 5 | **Wavefolder** | West Coast wavefolding, gentle folds to serrated chaos | High — folds create unpredictable harmonics |
| 6 | **Karplus-Strong** | Plucked string / physical modeling, organic transients | Medium — short decays create rhythm arguments |
| 7 | **Noise** | Filtered noise with color control, texture and breath | Medium — noise has no fundamental to fight over |

**Default algorithm pairing:** Voice A = VA (0), Voice B = Wavefolder (5). One smooth brother, one rough brother — the signature OUIE sound at factory init.

### The Interaction Stage (Signature Feature)

The HAMMER macro is a bipolar axis from 0.0 (full STRIFE) to 1.0 (full LOVE):

| HAMMER | Interaction | DSP Operation | Sound Character |
|--------|-------------|---------------|-----------------|
| 0.0 | Full STRIFE | Cross-FM (A mods B, B mods A) + Ring Mod + Phase Cancel | Aggressive, metallic, dissonant — brothers fighting |
| 0.25 | Tension | Light cross-mod, interference patterns, mild ring | Tense, unstable — brothers arguing |
| 0.5 | Independent | No interaction, parallel summation | Clean duophonic — brothers coexisting |
| 0.75 | Sympathy | Spectral averaging, gentle harmonic lock, mild pitch pull | Warm — brothers agreeing |
| 1.0 | Full LOVE | Full spectral blend + harmonic lock + unison thicken | Fused, massive, consonant — brothers as one |

**STRIFE implementation:**
- Cross-FM: Voice A's output FM-modulates Voice B's oscillator pitch (and vice versa) at the `ouie_crossModRatio` ratio
- Ring modulation: Voice A × Voice B multiplication in the time domain
- Phase cancellation: Voice B's phase offset adjusted to maximize destructive interference with Voice A

**LOVE implementation:**
- Spectral blend: FFT-domain averaging of Voice A and Voice B spectra (or time-domain crossfade at lower cost)
- Harmonic lock: Voice B's fundamental is quantized toward the nearest chord tone relative to Voice A
- Unison thicken: Detuned copies of each voice at ±`ouie_unisonDetune` cents

**`ouie_interactionDepth`** scales the STRIFE/LOVE effect independently of HAMMER position. HAMMER sets the direction; depth sets how hard it pushes.

### Voice Allocation

Three modes configurable per preset:

| Mode | Behavior | Best for |
|------|----------|----------|
| **Duo** | True duophonic — each new note to next available voice | Melodic leads, two-voice counterpoint |
| **Split** | Voice A = low note, Voice B = high note | Bass + lead layering |
| **Layer** | Both voices on every note (monophonic with 2 algorithms) | Thick mono patches |

Voice stealing: last-note priority. When both voices occupied in Duo mode, the older note is stolen.

---

## 3. Parameter List (~90 parameters, `ouie_` prefix)

### Per-Voice Parameters (14 × 2 = 28 total)

Duplicate for Voice B — replace `voiceA` with `voiceB`:

| ID | Range | Default A | Default B | Description |
|----|-------|-----------|-----------|-------------|
| `ouie_voiceA_algorithm` | 0–7 | 0 (VA) | 5 (Wavefolder) | Synthesis algorithm selection |
| `ouie_voiceA_tune` | -24–+24 st | 0 | 0 | Coarse tuning in semitones |
| `ouie_voiceA_fine` | -100–+100 cents | 0 | 0 | Fine tuning in cents |
| `ouie_voiceA_level` | 0.0–1.0 | 1.0 | 1.0 | Voice output level |
| `ouie_voiceA_pan` | -1.0–+1.0 | -0.3 | +0.3 | Pan position |
| `ouie_voiceA_unison` | 1–4 | 1 | 1 | Unison voice count |
| `ouie_voiceA_unisonDetune` | 0–50 cents | 10 | 10 | Unison detune spread |
| `ouie_voiceA_waveform` | 0–7 | 0 | 0 | Algorithm-specific waveform shape |
| `ouie_voiceA_color` | 0.0–1.0 | 0.5 | 0.5 | Algorithm-specific timbre modifier |
| `ouie_voiceA_shape` | 0.0–1.0 | 0.0 | 0.5 | Algorithm-specific modulation amount |
| `ouie_voiceA_octave` | -2–+2 | 0 | 0 | Octave offset |
| `ouie_voiceA_sub` | 0.0–1.0 | 0.0 | 0.0 | Sub oscillator blend (-1 octave) |
| `ouie_voiceA_drift` | 0.0–1.0 | 0.05 | 0.05 | Analog-style pitch drift |
| `ouie_voiceA_envDepth` | 0.0–1.0 | 0.0 | 0.0 | Pitch envelope depth |

### Interaction Stage (4)

| ID | Range | Default | Description |
|----|-------|---------|-------------|
| `ouie_hammer` | 0.0–1.0 | 0.5 | STRIFE (0) ↔ LOVE (1) relationship axis |
| `ouie_interactionDepth` | 0.0–1.0 | 0.5 | How strongly HAMMER position is applied |
| `ouie_crossModRatio` | 0.5–8.0 | 1.0 | FM ratio for STRIFE cross-modulation |
| `ouie_blendMode` | 0–2 | 0 | LOVE blend type: frequency-split / spectral / unison |

### Filter (5)

| ID | Range | Default | Description |
|----|-------|---------|-------------|
| `ouie_filterCutoff` | 20–20000 Hz | 8000 | Filter cutoff frequency |
| `ouie_filterResonance` | 0.0–1.0 | 0.0 | Filter resonance |
| `ouie_filterType` | 0–4 | 1 | LP12 / LP24 / HP / BP / Notch |
| `ouie_filterEnvAmount` | -1.0–+1.0 | 0.3 | Filter envelope modulation amount |
| `ouie_filterKeyTrack` | 0.0–1.0 | 0.5 | Key-tracking amount |

### Envelopes (12)

| ID | Range | Default | Description |
|----|-------|---------|-------------|
| `ouie_ampAttack` | 0.001–10s | 0.005 | Amp envelope attack |
| `ouie_ampDecay` | 0.001–10s | 0.3 | Amp envelope decay |
| `ouie_ampSustain` | 0.0–1.0 | 0.8 | Amp envelope sustain |
| `ouie_ampRelease` | 0.001–30s | 0.5 | Amp envelope release |
| `ouie_filterAttack` | 0.001–10s | 0.01 | Filter envelope attack |
| `ouie_filterDecay` | 0.001–10s | 0.5 | Filter envelope decay |
| `ouie_filterSustain` | 0.0–1.0 | 0.4 | Filter envelope sustain |
| `ouie_filterRelease` | 0.001–30s | 0.5 | Filter envelope release |
| `ouie_modAttack` | 0.001–10s | 0.01 | Modulation envelope attack |
| `ouie_modDecay` | 0.001–10s | 0.3 | Modulation envelope decay |
| `ouie_modSustain` | 0.0–1.0 | 0.5 | Modulation envelope sustain |
| `ouie_modRelease` | 0.001–30s | 0.3 | Modulation envelope release |

### LFOs (8, 2 × 4)

| ID | Range | Default | Description |
|----|-------|---------|-------------|
| `ouie_lfo1Rate` | 0.01–50 Hz | 1.0 | LFO 1 rate |
| `ouie_lfo1Depth` | 0.0–1.0 | 0.0 | LFO 1 depth |
| `ouie_lfo1Shape` | 0–5 | 0 | Sine / Tri / Saw / Square / S&H / Random |
| `ouie_lfo1Sync` | 0–1 | 0 | Free / Tempo sync |
| `ouie_lfo2Rate` | 0.01–50 Hz | 3.0 | LFO 2 rate |
| `ouie_lfo2Depth` | 0.0–1.0 | 0.0 | LFO 2 depth |
| `ouie_lfo2Shape` | 0–5 | 2 | Sine / Tri / Saw / Square / S&H / Random |
| `ouie_lfo2Sync` | 0–1 | 0 | Free / Tempo sync |

### Voice Allocation (3)

| ID | Range | Default | Description |
|----|-------|---------|-------------|
| `ouie_voiceMode` | 0–2 | 2 | Voice mode: Split / Layer / Duo |
| `ouie_portamento` | 0.0–5.0s | 0.0 | Global portamento time |
| `ouie_portamentoCurve` | 0.0–1.0 | 0.5 | Portamento curve shape |

### FX Chain (8)

| ID | Range | Default | Description |
|----|-------|---------|-------------|
| `ouie_driveAmount` | 0.0–1.0 | 0.0 | Character drive amount |
| `ouie_driveType` | 0–2 | 0 | Drive type: Soft / Hard / Tube |
| `ouie_stereoWidth` | 0.0–1.0 | 0.3 | Output stereo width |
| `ouie_delayTime` | 0.01–2.0s | 0.375 | Delay time |
| `ouie_delayFeedback` | 0.0–0.95 | 0.3 | Delay feedback |
| `ouie_delayMix` | 0.0–1.0 | 0.15 | Delay wet/dry |
| `ouie_reverbSize` | 0.0–1.0 | 0.3 | Reverb room size |
| `ouie_reverbMix` | 0.0–1.0 | 0.15 | Reverb wet/dry |

**Total: ~91 parameters**

---

## 4. Macro Mapping (M1–M4)

| Macro | Label | Controls | Aquatic Meaning |
|-------|-------|----------|-----------------|
| M1 | **HAMMER** | `ouie_hammer` + `ouie_interactionDepth` + `ouie_crossModRatio` | The cephalofoil — wide-set eyes choosing whether to see one world or two. The fundamental relationship dial. STRIFE=0, LOVE=1. |
| M2 | **AMPULLAE** | Velocity sensitivity + `ouie_modSustain` + `ouie_lfo1Depth` + `ouie_lfo2Depth` | Electroreceptors in the hammerhead's flat head — sensing the faintest electrical fields. The sensitivity and expressiveness dial. |
| M3 | **CARTILAGE** | `ouie_portamento` + `ouie_voiceA_drift` + `ouie_voiceB_drift` + `ouie_voiceA_envDepth` | Flexible skeleton — bending without breaking. The plasticity and movement dial. High CARTILAGE = voices glide, drift, and bend around each other. |
| M4 | **CURRENT** | `ouie_reverbMix` + `ouie_delayMix` + `ouie_stereoWidth` + `ouie_filterCutoff` | The thermocline current — the water around the shark. The space and environment dial. Low CURRENT = dry, close, interior. High CURRENT = immersed in the thermocline. |

---

## 5. feliX-Oscar Polarity

**Position:** Dead center — the thermocline. 50% feliX, 50% Oscar.

This is the only engine at the exact midpoint of the polarity axis. Not because OUIE is neutral — because it *simultaneously embodies both poles*.

**STRIFE = feliX:** Analytical, precise, separated. Cross-FM creates mathematically exact frequency relationships. Ring modulation produces sum-and-difference frequencies with mathematical clarity. Phase cancellation is a precise destructive interference calculation. STRIFE is feliX's rationality applied to conflict.

**LOVE = Oscar:** Warm, blended, harmonically rich. Spectral blending produces fused, indistinct sonority. Harmonic locking creates chord-tone warmth. Unison thickening adds the density and fullness that Oscar's bass engines possess at high frequency. LOVE is Oscar's warmth applied to relationship.

The hammerhead patrols the thermocline — the invisible boundary where warm surface water meets cold deep water. It's the only engine that inhabits both poles and moves between them in real time.

**feliX-Oscar balance:** 50% feliX / 50% Oscar — variable depending on HAMMER position.

---

## 6. Coupling Matrix

### OUIE as Target (receives from)

| Source Engine | Coupling Type | Musical Effect |
|-------------|---------------|----------------|
| OUROBOROS | `EnvToMorph` | Chaos envelope drives HAMMER toward STRIFE — the ouroboros destabilizes the brothers |
| ORACLE | `EnvToMorph` | Stochastic modulation drives interval selection — oracle chooses the interval |
| OHM | `AmpToFilter` | Commune axis amplitude mirrors LOVE state — the commune makes the shark harmonize |
| OCEANIC | `EnvToMorph` | Spectral separation drives algorithm morph — the thermocline bends around the shark |
| ONSET | `AmpToFilter` | Drum hits modulate filter — rhythmic predator, strike on beat |
| FAT | `AmpToFilter` | 13-oscillator mass drives filter — the ocean's largest creature |
| OPAL | `AudioToFM` | Grain particles FM-modulate Voice B — parasitic interference from scattered light |

### OUIE as Source (sends to)

| Target Engine | Coupling Type | Musical Effect |
|-------------|---------------|----------------|
| OVERDUB | `getSample` | Two-voice argument echoing endlessly through dub delay |
| OPAL | `AudioToWavetable` | Two-voice texture granulated into particles — predator dissolved into plankton |
| OVERBITE | `AmpToFilter` | Shark's attack drives anglerfish filter — apex predator triggers ambush predator |
| OCEANIC | `getSample` | Duophonic output spectrally separated by the thermocline |
| ODYSSEY | `getSample` | Duophonic texture as ODYSSEY's journey starting point |
| OCEANDEEP | `AmpToFilter` | Hammerhead's attack drives abyssal bass filter — descent from thermocline to floor |
| OPENSKY | `AmpToFilter` | LOVE-mode fused voice drives sky filter — harmony erupts through the surface |

### Signature Coupling Routes

**OUIE × OCEANDEEP — "Apex Dive"**
Duophonic leads over abyssal 808. The hammerhead descending from the thermocline to Oscar's floor. STRIFE mode preferred — the shark disagrees with the depths.

**OUIE × OPENSKY — "Thermal Rising"**
Two-voice texture erupting through the surface into euphoric shimmer. LOVE mode preferred — brothers rising together, fused into one light.

**OUIE × OUIE — "Cephalofoil" (self-coupling)**
When a duophonic engine couples with itself, the result is 4 interacting algorithms in a 2×2 interaction matrix. Voice A of instance 1 fights with Voice B of instance 2. Voice B of instance 1 harmonizes with Voice A of instance 2. Two hammerheads circling each other.

**OUIE × OHM — "Commune Harmony"**
OHM's COMMUNE axis amplitude drives OUIE's HAMMER toward LOVE. When the commune is warm, the shark becomes harmonious.

---

## 7. Creature Identity

**The Hammerhead Shark** (*Sphyrna*)

The hammerhead's most distinctive feature — the cephalofoil, the impossibly wide T-shaped head — is literally two independent sensory platforms fused into one animal. Each eye sees a different angle. Each nostril tracks a different scent gradient. The ampullae of Lorenzini on that wide head detect the faintest electrical fields from buried prey.

This is duophony in biology. Two sensory systems, independently evolved, forced to cooperate inside one skull. Sometimes they agree — binocular vision, triangulating prey with impossible precision. Sometimes they detect contradictory information — one nostril says left, one nostril says right. The animal must decide.

**XOuïe lives at the thermocline.** The hammerhead patrols this invisible boundary where warm surface water meets cold deep water. Neither surface nor deep. Not feliX, not Oscar — both at once.

**The two eyes are in LOVE.** The two nostrils are in STRIFE.

---

## 8. Historical Homage

**Don Buchla's 258 Complex Waveform Generator** — Two-oscillator dialogue was the conceptual foundation of the 258. Buchla understood that a synthesizer with two voices in defined relationship creates a musical organism, not just a doubled sound source. OUIE's interaction stage is Buchla's two-oscillator philosophy with explicit STRIFE/LOVE polarity.

**Arturia MiniFreak** — The MiniFreak's selectable digital oscillator algorithms per voice is the direct technical ancestor of OUIE's per-voice algorithm selection. The MiniFreak demonstrated that a duophonic architecture with non-identical voices is both musically powerful and CPU-feasible.

**Arturia MicroFreak (the "little brother")** — The brotherly dynamic between MicroFreak and MiniFreak mirrors OUIE's internal sibling dynamic. One voice smooth, one rough. Both instruments shaped by the same designer philosophy yet with different characters.

**Morton Subotnick's Silver Apples of the Moon** — The first electronic music composition to explore two Buchla voices as characters in a conversation rather than a two-voice chord. OUIE's HAMMER at 0.5 is two voices coexisting the way Subotnick's two voices coexisted — aware of each other without merging.

**Roland Juno-106 dual-DCO architecture** — LOVE mode's unison-thicken behavior is structurally analogous to the Juno's dual-DCO design: two oscillators at slightly different tuning creating warmth through beating. OUIE brings this classic technique into the interaction stage as a resolved outcome of LOVE.

---

## 9. Aquatic Mythology Position

The thermocline. The invisible horizontal boundary in the ocean where warm surface water meets cold deep water. Light penetrates here — but not fully. This is neither feliX's sky nor Oscar's floor. It is the in-between zone: the most biologically productive layer of the ocean, where currents mix and nutrients concentrate.

The hammerhead patrols this boundary. Not because it's trapped here, but because the thermocline is where prey concentrates. The shark's wide-set eyes see both the sunlit water above and the darker water below simultaneously. The HAMMER macro is this: which direction is the shark looking?

**Depth zone:** Zone 4-5 — Mesopelagic to Bathypelagic transition
**Water column position:** Mid-column, thermocline boundary
**Counterpart:** None — OUIE is the only engine at dead-center polarity. OVERDUB is nearby (Oscar-leaning at 40/60) but OUIE is the first true center.

---

## 10. Preset Strategy (150 factory presets)

| Category | Count | Character |
|----------|-------|-----------|
| Brotherly Love | 20 | Harmonious duophonic textures — HAMMER=1.0, spectral blend, harmonic lock |
| Brotherly Strife | 20 | Aggressive cross-mod — HAMMER=0.0, ring mod, FM interference, dissonant leads |
| Thermocline | 20 | Balanced exploration — HAMMER=0.5, subtle interaction, clean duophony |
| Predator | 18 | Aggressive leads and bass — the shark hunting, sharp filter sweeps, high attack |
| Deep Scan | 15 | Ambient AMPULLAE-focused — sensitive, velocity-responsive, electroreceptive |
| Cartilage | 12 | Portamento and drift presets — CARTILAGE macro demonstrates flexible movement |
| Coupling Showcases | 15 | Cross-engine duets: OCEANDEEP/OPENSKY/OHM/OUROBOROS |
| Hero Presets | 8 | Best-of showpieces (included in counts above) |
| Cephalofoil Series | 12 | Self-coupling and multi-instance patches — for advanced users |
| Algorithm Pairs | 10 | Systematic exploration of all 28 possible V×V algorithm combinations |

### Hero Preset Concepts

1. **Cephalofoil** — VA + Wavefolder, HAMMER=0.5, both voices panned ±0.4. The signature sound. Two brothers coexisting.
2. **Apex** — FM + Phase Dist, HAMMER=0.0, aggressive cross-mod. Maximum STRIFE. The attack.
3. **Schooling** — Wavetable + Wavetable, HAMMER=1.0, unison 4+4. Brothers as one massive wall.
4. **Electroreception** — KS + Noise, AMPULLAE=1.0, velocity-responsive plucked + breath.
5. **Cartilage** — Additive + FM, CARTILAGE=1.0, gliding duophonic lines bending around each other.
6. **Thermocline Drift** — VA + Additive, LFO slowly sweeping HAMMER, alternating love and strife.
7. **Gill Breath** — Noise + Wavefolder, filter LFO mimicking breathing rhythm.
8. **Brothers** — VA (Voice A, smooth) + Phase Dist (Voice B, rough), HAMMER=0.3. Tension without full conflict.

---

## 11. Visual Identity

- **Accent color:** Hammerhead Steel `#708090` — the first neutral-cool grey in the gallery. Every other engine has a vibrant or saturated accent. OUIE's neutrality is intentional: it occupies the center, takes no side.
- **UI concept:** The cephalofoil as the central interaction display. Voice A displayed left (the left eye of the hammerhead). Voice B displayed right (the right eye). The HAMMER macro as the central axis connecting them — at STRIFE, waveforms collide and interfere with visual glitch artifacts; at LOVE, waveforms merge and harmonize with smooth blend animation.
- **Unique visual feature:** The interaction visualizer shows STRIFE↔LOVE state in real time. At full STRIFE, waveforms visually crash into each other. At full LOVE, they superimpose into one coherent image.

---

## 12. Gallery Niche

| Dimension | OUIE Position | Nearest Neighbor | Gap Filled |
|-----------|---------------|------------------|------------|
| Polyphony | Duophonic (2 voices) | All others: 8-16 voice poly | First duophonic |
| Polarity | Dead center (50/50) | OVERDUB (~40/60) | First true center |
| Algorithm | Selectable per voice (8 options × 2) | Fixed algorithm per engine | First per-voice multi-algorithm |
| Accent | Neutral steel grey | All others: vibrant/saturated | First neutral tone |
| Interaction | Bipolar internal STRIFE↔LOVE | Coupling matrix (external only) | First internal voice interaction engine |

---

## 13. Phase 1 Build Readiness

### Scaffold Checklist

- [ ] Create `Source/Engines/Ouie/` directory structure
- [ ] Write `OuieEngine.h` implementing `SynthEngine` interface
- [ ] Voice A synthesis: 8 algorithm selectors (VA / Wavetable / FM / Additive / PhaseDistortion / Wavefolder / KarplusStrong / Noise)
- [ ] Voice B synthesis: identical algorithm pool, independent selection
- [ ] Interaction stage: STRIFE path (cross-FM + ring mod + phase cancel), LOVE path (spectral blend + harmonic lock + unison)
- [ ] HAMMER parameter routes smoothly between STRIFE and LOVE
- [ ] Dual filter: shared LP/HP/BP/Notch post-interaction
- [ ] Three amp/filter/mod envelopes
- [ ] Two LFOs with rate ≥ 0.01 Hz (D005 compliance)
- [ ] Voice allocation: Duo / Split / Layer modes
- [ ] FX chain: drive → stereo width → delay → reverb
- [ ] All ~91 `ouie_` parameters wired to APVTS
- [ ] Four macros (HAMMER/AMPULLAE/CARTILAGE/CURRENT) confirm audible
- [ ] Register in `EngineRegistry.h` → add to `CLAUDE.md` engine table
- [ ] 150 factory presets in `.xometa` format

### DSP Dependencies (reusable from existing engines)

| Component | Source |
|-----------|--------|
| VA oscillator | Core DSP / OddfeliX snap_ subtractive |
| Wavetable scanning | XOdyssey drift_ wavetable |
| 2-operator FM | XOrbital orb_ FM core |
| Phase distortion | XObsidian obsidian_ CZ-style |
| Wavefolder | XOrigami origami_ fold stage |
| Karplus-Strong | XOwlfish owl_ plucked string |
| Noise generator | Core DSP noise floor |
| Additive synthesis | New — 16-partial additive, reference XOrganon for partial management |
| Ring modulation | Available in XOuroboros ouro_ path |
| Spectral blend | New — FFT-domain or time-domain crossfade |
| Matched-Z LP filter | Fleet standard |

### Critical DSP Rules for This Engine

- **Per-voice algorithm state:** Voice A and Voice B must maintain fully independent DSP state. No shared wavetable position, no shared FM phase, no shared KS buffer.
- **Interaction stage is post-oscillator, pre-filter.** STRIFE/LOVE operates on the raw oscillator outputs before filtering — this preserves the fight/merge at the fundamental level.
- **Bipolar HAMMER modulation:** `ouie_hammer` uses `!= 0` checks, not `> 0`. HAMMER at exactly 0.5 produces no interaction; both below and above 0.5 produce different effects.
- **Cross-FM feedback guard:** Cross-FM (A mods B, B mods A simultaneously) can produce runaway feedback. Clamp the FM depth to prevent audio blowup at extreme STRIFE.
- **Duophonic voice allocation is simpler than polyphonic** — no voice stealing algorithm needed beyond last-note priority between 2 slots.
- **Algorithm switching:** Crossfade algorithm transitions over ~10ms to prevent clicks when `ouie_voiceN_algorithm` changes.
- **`cancelAndHoldAtTime`** on filter envelope interruption — rapid note stealing creates filter clicks without this.
- **`?? 0` not `|| 0`** when reading Voice A/B level — `0.0` is a valid (silent) voice level.

### Phase Gate: Phase 1 → Phase 2 (DSP Build)

- [ ] Both voices produce independent audio at HAMMER=0.5
- [ ] HAMMER sweep from 0.0 to 1.0 produces audible transition from STRIFE to LOVE
- [ ] All 8 algorithm types produce audio in Voice A
- [ ] All 8 algorithm types produce audio in Voice B
- [ ] STRIFE at 1.0 produces audible cross-FM/ring-mod artifacts
- [ ] LOVE at 1.0 produces fused/blended output

### Phase Gate: Phase 2 → Phase 3 (Preset)

- [ ] All D001–D006 doctrines satisfied:
  - D001: Velocity drives filter brightness (AMPULLAE macro routes velocity → filter depth)
  - D002: 2 LFOs + mod wheel + aftertouch + 4 macros confirmed functional
  - D003: Physics citations — KS physical model, FM frequency ratios documented
  - D004: All ~91 parameters affect audio (algorithm-inactive params must still route to something)
  - D005: LFO rate floor ≤ 0.01 Hz
  - D006: Velocity→timbre + aftertouch CC wired
- [ ] D004 note: `ouie_voiceN_waveform` / `color` / `shape` vary per active algorithm. When algorithm changes, these parameters must re-map. Document the mapping in engine header.
- [ ] auval PASS
- [ ] 150 factory presets complete

### V1 Scope Note

OUIE is confirmed V1 scope (decision 2026-03-14). It is not registered in `EngineRegistry.h` yet. The target is to complete scaffold and DSP build in the same Opus session as OSTINATO, OPENSKY, and OCEANDEEP.

---

*XO_OX Designs | Engine: OUIE | Accent: #708090 | Prefix: ouie_ | Phase 1 Architecture*
*"Two brothers. Two algorithms. One knob decides if they harmonize or destroy each other."*
