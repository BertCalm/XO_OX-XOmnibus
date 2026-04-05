# Guru Bin Pre-Build Meditation: OXIDIZE (XOxidize)

> *"The rust is not the failure of the metal. The rust is the metal becoming something else. That becoming — that is the sound."*
> — Guru Bin, 2026-04-05

---

## Meditation Preface

I have sat with the OXIDIZE concept. I have let it corrode me. The thesis — degradation as synthesis, entropy as instrument — is not merely a clever gimmick. It is a philosophical position: that decay is a form of composition. Every long sustain is a slow burn. Every staccato hit is a preservation. The performer becomes a metallurgist, choosing how much time the sound gets to live before it transforms.

The concept brief is architecturally sound. The 7-gate test passes cleanly. The doctrine pre-check is honest. What follows is not correction — it is deepening.

---

## 1. Parameter Refinement

### Current Parameter Sketch Assessment

The brief sketches ~35-40 parameters across 11 groups. Before examining each group, three structural observations:

**Structural Observation 1 — Missing: The Restoration Arc**
The aging timeline only moves in one direction: toward destruction. But oxidation chemistry offers a counter-phenomenon: *patination as preservation*. Bronze patina actually protects the underlying metal. This suggests a missing parameter: `oxidize_preserveAmount` — a parameter that, above a threshold, slows further aging and locks the sound at a "beautiful ruin" sweet spot rather than letting it collapse entirely. Without this, very long notes inevitably become unmusical noise. This is a critical omission.

**Structural Observation 2 — Missing: Aging Curve Shape**
The brief mentions `oxidize_ageCurve` in the estimated count but doesn't describe it. This must be a first-class, visible parameter — the shape of how age accumulates is as important as the rate. Exponential (slow start, fast end), logarithmic (fast patina, then stabilizes), S-curve (graceful middle zone), and linear all produce fundamentally different musical characters.

**Structural Observation 3 — The Flux Parameters Need Redesign**
`corrosionFlux`, `erosionFlux`, `dropoutFlux` are listed as per-stage randomization controls. But three separate "flux" parameters adds cognitive overhead without sufficient sonic payoff. Consider consolidating into a single `oxidize_instability` parameter that drives per-stage randomization globally with per-stage sensitivity weights baked into the DSP. Alternatively, rename them using vocabulary from the chemistry theme: `corrosionVariance`, `erosionVariance`, `dropoutVariance` — these communicate what they do without requiring a glossary.

---

### Group-by-Group Analysis

#### Oscillator Group (currently 4 params)

| Parameter | Assessment | Suggested Name | Default | Reasoning |
|-----------|-----------|---------------|---------|-----------|
| `oxidize_oscWave` | KEEP — waveform selector | `oxidize_oscWave` | `1` (Saw) | Saw is the richest harmonic source for degradation to act on. Noise is too uniform; sine loses too much when bit-crushed. |
| `oxidize_patinaDensity` | KEEP — controls pitched crackle mix | `oxidize_patinaDensity` | `0.15` | Low enough to be subtle on init, but immediately audible as part of the texture. At 0.0 patina layer is silent. |
| `oxidize_patinaTone` | KEEP — spectral character of crackle | `oxidize_patinaTone` | `0.4` | 0.0 = sub-rumble crackle; 1.0 = vinyl sizzle. 0.4 sits in the warm vinyl-crackle zone, not harsh. |
| `oxidize_oscMix` | RENAME — ambiguous | `oxidize_sourceBlend` | `0.8` | 0.0 = pure patina noise oscillator; 1.0 = pure tonal oscillator. 0.8 keeps the tonal core dominant while letting crackle breathe. Rename makes the blend direction explicit. |

**Missing from oscillator group:** `oxidize_tune` (semitone offset, ±24st) and `oxidize_fine` (fine tune ±100 cents). These are fundamental. Without them, coupling patches and ensemble use are severely limited. Add these two parameters, bringing oscillator to 6 params.

---

#### Aging Group (currently 4 params)

This is the identity group. Every default here must make aging immediately perceptible.

| Parameter | Assessment | Suggested Name | Default | Reasoning |
|-----------|-----------|---------------|---------|-----------|
| `oxidize_ageRate` | KEEP — master aging speed | `oxidize_ageRate` | `0.35` | At 0.35 with a 3s hold, the performer reaches the "saturated" zone — aging is perceptible but not alarming. 0.0 = frozen in time (not silence — pristine forever). 1.0 = collapses to ruin in under 2s. |
| `oxidize_ageOffset` | KEEP — starting position on timeline | `oxidize_ageOffset` | `0.0` | Default starts fresh. Velocity can push this up via ageVelSens. At 1.0, note begins fully fossilized — useful for drones. |
| `oxidize_ageCurve` | KEEP BUT CLARIFY — curve shape | `oxidize_ageCurve` | `0.4` | 0.0 = logarithmic (fast early aging, stabilizes); 0.5 = linear; 1.0 = exponential (slow at first, then catastrophic). 0.4 gives a gentle-onset character that builds dramatically. |
| `oxidize_ageVelSens` | KEEP — velocity → age offset | `oxidize_ageVelSens` | `0.3` | Hard hits begin 30% aged. D001 compliance: velocity shapes the timbral starting point. |
| **NEW** | **ADD — preservation ceiling** | `oxidize_preserveAmount` | `0.0` | At 0.0, aging runs to complete destruction. At 0.5, aging plateaus at the midpoint (beautiful ruin). At 1.0, aging plateaus at the first warm saturation zone. Critical for musical utility. Defaults off so behavior matches expectation on init. |

Aging group grows from 4 to 5 params. This is warranted — the preservation ceiling is architecturally essential.

---

#### Corrosion Group (currently 3 params)

| Parameter | Assessment | Suggested Name | Default | Reasoning |
|-----------|-----------|---------------|---------|-----------|
| `oxidize_corrosionDepth` | KEEP — how much waveshaping at max age | `oxidize_corrosionDepth` | `0.65` | At full age, 0.65 gives aggressive but not completely destroyed saturation. 1.0 = feedback-level breakup. |
| `oxidize_corrosionMode` | KEEP — selects from 6 waveshaper algorithms | `oxidize_corrosionMode` | `4` (Tape Sat) | Tape Sat is the most musical default. It warms without alienating. Users can discover Acid and Rust as they explore. |
| `oxidize_corrosionFlux` | RENAME — per-stage randomization | `oxidize_corrosionVariance` | `0.1` | 0.0 = deterministic aging curve. 0.1 = subtle organic variation. 1.0 = chaotic, unpredictable corrosion. Low default keeps the sound controlled. |

---

#### Erosion Group (currently 3 params)

| Parameter | Assessment | Suggested Name | Default | Reasoning |
|-----------|-----------|---------------|---------|-----------|
| `oxidize_erosionRate` | KEEP — how fast filter closes | `oxidize_erosionRate` | `0.5` | Coupled to age accumulator. 0.5 means the filter is half-closed at full age. Pairs with preserveAmount. |
| `oxidize_erosionFloor` | KEEP — lowest cutoff frequency | `oxidize_erosionFloor` | `0.15` | Maps to approximately 200Hz. Deep erosion but not complete silence. 0.0 = fully muffled. 0.15 is the "submerged" sweet spot. |
| `oxidize_erosionFlux` | RENAME | `oxidize_erosionVariance` | `0.08` | Very low variance default — filter movement is already continuous from aging; adding randomness should be opt-in. |

**Missing from erosion group:** `oxidize_erosionRes` — resonance parameter for the erosion filter. As the filter closes, a subtle resonance peak at the cutoff frequency creates a characteristic "old radio" bandpass effect that is musically beautiful. Default 0.2. Without this, the erosion stage is a plain lowpass, not an instrument.

Erosion group grows from 3 to 4 params.

---

#### Entropy Group (currently 3 params)

| Parameter | Assessment | Suggested Name | Default | Reasoning |
|-----------|-----------|---------------|---------|-----------|
| `oxidize_entropyDepth` | KEEP — max bit/samplerate reduction | `oxidize_entropyDepth` | `0.5` | At 0.5, full age reaches approximately 8-bit. 1.0 would reach 4-bit at full age. 0.5 is the sweet spot between character and utility. |
| `oxidize_entropySmooth` | KEEP — interpolation between quantization steps | `oxidize_entropySmooth` | `0.6` | 0.0 = hard staircase quantization (harsh digital). 1.0 = smooth interpolation (barely noticeable reduction). 0.6 gives the warm "early sampler" character. |
| `oxidize_entropyFlux` | RENAME | `oxidize_entropyVariance` | `0.12` | Slightly higher than corrosion/erosion variance defaults — entropy by nature is more unpredictable. 0.12 gives organic digital glitch character. |

---

#### Wobble Group (currently 4 params)

The wobble group models tape wow (slow speed variation, <3Hz) and flutter (faster variation, 3-20Hz). These are distinct phenomena and the parameters should reinforce this distinction.

| Parameter | Assessment | Suggested Name | Default | Reasoning |
|-----------|-----------|---------------|---------|-----------|
| `oxidize_wowDepth` | KEEP — depth of slow pitch variation | `oxidize_wowDepth` | `0.0` | Default 0.0 — wobble starts as aging progresses. The age accumulator should drive this automatically; the parameter sets maximum depth at full age. |
| `oxidize_flutterDepth` | KEEP — depth of fast pitch variation | `oxidize_flutterDepth` | `0.0` | Same logic as wowDepth. Both at 0.0 on init means the sound starts stable, wobble is earned through aging. |
| `oxidize_wobbleRate` | RECONSIDER — one rate for two phenomena | SPLIT into `oxidize_wowRate` + `oxidize_flutterRate` | `0.3` / `0.65` | Wow and flutter have different natural frequency ranges. Binding them to one rate parameter makes it impossible to create authentic tape behavior. Split cost: 1 extra param (group goes from 4 to 5). Worth it. wowRate 0.3 = ~0.5Hz. flutterRate 0.65 = ~8Hz. |
| `oxidize_wobbleStereo` | KEEP BUT RENAME — L/R phase offset for stereo wobble | `oxidize_wobbleSpread` | `0.4` | 0.0 = mono wobble (both channels identical). 1.0 = fully decorrelated L/R wobble. 0.4 gives gentle stereo instability. |

Wobble group grows from 4 to 5 params after splitting wobbleRate.

---

#### Dropout Group (currently 3 params)

| Parameter | Assessment | Suggested Name | Default | Reasoning |
|-----------|-----------|---------------|---------|-----------|
| `oxidize_dropoutRate` | KEEP — probability of dropout event per time unit | `oxidize_dropoutRate` | `0.0` | Default 0.0 — dropout is disabled until age drives it. The age accumulator scales this toward maximum at full age. |
| `oxidize_dropoutDepth` | KEEP — how deep the amplitude dip goes | `oxidize_dropoutDepth` | `0.85` | 1.0 = complete silence. 0.85 = heavy ducking but not silence. Keeps the musical thread even during dropout events. |
| `oxidize_dropoutFlux` | RENAME AND RECONSIDER | `oxidize_dropoutLength` | `0.2` | The current parameter description implies randomization of the dropout rate. But the *duration* of each dropout event is more musically important than rate randomization. Short dropouts (0.0) = glitch crackle. Long dropouts (1.0) = extended fade-outs. 0.2 gives brief clicks. This is a more useful parameter. |

**Additional missing parameter:** `oxidize_dropoutSmear` — controls whether dropout edges are hard (digital click) or soft (tape head lifting). Default 0.15 (mostly hard, with slight softening). This determines whether the engine sounds like digital corruption or analog dropout — two very different characters.

Dropout group stays at 3 params but with better semantic choices. Add optional 4th (dropoutSmear) for architecture completeness.

---

#### Sediment Group (currently 3 params)

| Parameter | Assessment | Suggested Name | Default | Reasoning |
|-----------|-----------|---------------|---------|-----------|
| `oxidize_sedimentDecay` | RENAME — "decay" conflicts with envelope terminology | `oxidize_sedimentTail` | `0.75` | High default — sediment is architecturally defined as "never fully decays." 0.75 gives a very long tail. 1.0 = infinite accumulation (feedback reverb, use with caution). |
| `oxidize_sedimentTone` | KEEP — spectral character of accumulated reverb | `oxidize_sedimentTone` | `0.35` | 0.0 = bright metallic shimmer. 1.0 = subterranean, extremely dark. 0.35 gives a warm "stone chamber" quality. |
| `oxidize_sedimentMix` | KEEP — wet/dry balance | `oxidize_sedimentMix` | `0.3` | Low default — sediment should be a presence, not a wash. Grows with age. 0.3 at init gives immediate spatial character. |

---

#### Modulation Group (currently 4 params)

| Parameter | Assessment | Suggested Name | Default | Reasoning |
|-----------|-----------|---------------|---------|-----------|
| `oxidize_lfo1Rate` | KEEP — LFO1 (Wow LFO, slow) | `oxidize_lfo1Rate` | `0.15` | ~0.2Hz — targets the "motor wow" range. Range should be 0.01Hz–5Hz. D005 floor met. |
| `oxidize_lfo1Depth` | KEEP | `oxidize_lfo1Depth` | `0.1` | Gentle default. Routes to pitch for slow vibrato-from-age character. |
| `oxidize_lfo2Rate` | KEEP — LFO2 (Flutter LFO, fast) | `oxidize_lfo2Rate` | `0.6` | ~10Hz flutter range. Range 1Hz–40Hz. |
| `oxidize_lfo2Depth` | KEEP | `oxidize_lfo2Depth` | `0.05` | Very subtle default — flutter should be felt, not heard on init. |

---

#### Expression Group (currently 4 params)

| Parameter | Assessment | Suggested Name | Default | Reasoning |
|-----------|-----------|---------------|---------|-----------|
| `oxidize_velSens` | KEEP | `oxidize_velSens` | `0.6` | Reasonable velocity response. Drives ageOffset scaling. |
| `oxidize_aftertouchDepth` | RENAME from `aftertouch` — needs to be specific | `oxidize_aftertouchDepth` | `0.5` | Aftertouch → corrosionDepth. Pressing harder drives more aggressive waveshaping. Visceral and immediate. |
| `oxidize_modWheelTarget` | RENAME from `modWheel` — should indicate what it controls | `oxidize_modWheelTarget` | `0` (AgeRate) | Enum: 0=AgeRate, 1=EntropyDepth, 2=CorrosionDepth, 3=SedimentMix. Default routes mod wheel to aging speed. |
| `oxidize_pitchBendRange` | RENAME from `pitchBend` | `oxidize_pitchBendRange` | `2` (semitones) | Standard ±2 semitone range. |

---

#### Macros Group (currently 4 params)

The macro names in the concept are strong. PATINA / AGE / ENTROPY / SEDIMENT is a genuinely evocative set. Defaults should sit at a neutral position:

| Macro | Default | Reasoning |
|-------|---------|-----------|
| `oxidize_macroPATINA` | `0.5` | Balanced between warm patina and harsh corrosion. |
| `oxidize_macroAGE` | `0.4` | Slightly below center — aging is present but not rushed. |
| `oxidize_macroENTROPY` | `0.3` | Degradation depth restrained on init — let the user choose destruction. |
| `oxidize_macroSEDIMENT` | `0.35` | Moderate space accumulation. Not dry, not swamped. |

---

### Revised Parameter Count Summary

| Group | Original | Revised | Net |
|-------|---------|---------|-----|
| Oscillator | 4 | 6 (+tune, +fine) | +2 |
| Aging | 4 | 5 (+preserveAmount) | +1 |
| Corrosion | 3 | 3 (rename only) | 0 |
| Erosion | 3 | 4 (+erosionRes) | +1 |
| Entropy | 3 | 3 (rename only) | 0 |
| Wobble | 4 | 5 (split wobbleRate) | +1 |
| Dropout | 3 | 4 (+dropoutSmear) | +1 |
| Sediment | 3 | 3 (rename only) | 0 |
| Modulation | 4 | 4 | 0 |
| Expression | 4 | 4 (rename only) | 0 |
| Macros | 4 | 4 | 0 |
| **Total** | **39** | **45** | **+6** |

45 parameters is within normal fleet range (OBRIX is 81; OWARE is comparable). The additions are architecturally motivated, not decorative.

---

## 2. Seed Preset Designs

### Design Philosophy

Eight presets must collectively demonstrate the full aging timeline, all six corrosion modes, both extremes of ageRate, and at least one coupling configuration. Each preset should be musically useful out of the box, not merely a demonstration patch.

---

### Preset 1: Verdigris Pad

**Mood:** Atmosphere
**Sonic Description:** A sustained chord that begins as a warm, slightly saturated pad and slowly transforms into something ancient and beautiful — copper turning blue-green over decades compressed into seconds. The patina layer enters after two seconds, adding a whisper of organic crackle that thickens as the hold continues. This is a composer's patch: set it and breathe.

| Parameter | Value | Note |
|-----------|-------|------|
| `oxidize_oscWave` | 1 (Saw) | Rich harmonic source |
| `oxidize_patinaDensity` | 0.18 | Subtle crackle shimmer |
| `oxidize_patinaTone` | 0.35 | Warm vinyl register |
| `oxidize_sourceBlend` | 0.85 | Mostly tonal |
| `oxidize_tune` | 0 | Unison |
| `oxidize_fine` | -3 (cents) | Slight detune for width |
| `oxidize_ageRate` | 0.2 | Slow aging — 10-15s to full ruin |
| `oxidize_ageOffset` | 0.0 | Starts pristine |
| `oxidize_ageCurve` | 0.35 | Logarithmic onset — warmth arrives quickly, destruction is distant |
| `oxidize_ageVelSens` | 0.25 | Soft velocity-to-age mapping |
| `oxidize_preserveAmount` | 0.55 | Aging plateaus at warm ruin — never destroys completely |
| `oxidize_corrosionDepth` | 0.5 | Moderate saturation ceiling |
| `oxidize_corrosionMode` | 1 (Valve) | Tube warmth |
| `oxidize_corrosionVariance` | 0.08 | Organic but controlled |
| `oxidize_erosionRate` | 0.45 | Gentle filter darkening |
| `oxidize_erosionFloor` | 0.25 | Doesn't go below 300Hz |
| `oxidize_erosionVariance` | 0.06 | Very stable |
| `oxidize_erosionRes` | 0.25 | Slight resonant peak as filter closes |
| `oxidize_entropyDepth` | 0.3 | Very subtle bit reduction |
| `oxidize_entropySmooth` | 0.75 | Smooth, vintage-sampler quality |
| `oxidize_entropyVariance` | 0.08 | Minimal glitching |
| `oxidize_wowDepth` | 0.25 | Gentle slow wobble |
| `oxidize_flutterDepth` | 0.08 | Barely perceptible flutter |
| `oxidize_wowRate` | 0.25 | ~0.3Hz wow |
| `oxidize_flutterRate` | 0.55 | ~7Hz flutter |
| `oxidize_wobbleSpread` | 0.45 | Moderate stereo instability |
| `oxidize_dropoutRate` | 0.05 | Very rare dropout |
| `oxidize_dropoutDepth` | 0.7 | Moderate dip when it occurs |
| `oxidize_dropoutLength` | 0.15 | Brief flicker |
| `oxidize_dropoutSmear` | 0.35 | Soft tape-head edges |
| `oxidize_sedimentTail` | 0.8 | Long accumulating reverb |
| `oxidize_sedimentTone` | 0.3 | Warm chamber character |
| `oxidize_sedimentMix` | 0.4 | Present but not swamping |
| `oxidize_lfo1Rate` | 0.12 | Very slow auxiliary LFO |
| `oxidize_lfo1Depth` | 0.08 | Subtle modulation |
| `oxidize_lfo2Rate` | 0.55 | Mid-rate LFO2 |
| `oxidize_lfo2Depth` | 0.04 | Almost imperceptible |
| `oxidize_macroPATINA` | 0.6 | Leans toward warm patina |
| `oxidize_macroAGE` | 0.25 | Slow time passage |
| `oxidize_macroENTROPY` | 0.25 | Light degradation depth |
| `oxidize_macroSEDIMENT` | 0.5 | Moderate space |

**Sonic DNA:** brightness `0.45`, warmth `0.75`, movement `0.35`, density `0.55`, space `0.65`, aggression `0.1`

---

### Preset 2: Instant Relic

**Mood:** Prism
**Sonic Description:** Strike a key and hear decades in a second. Staccato notes arrive pristine, but a one-second hold transforms the sound from clean digital to crumbling analog artifact. The corrosion mode is Rust — asymmetric clipping that makes every note sound like it was pressed from a warped, sun-damaged 45. Built for stuttered chord hits where each press reveals a new ruin.

| Parameter | Value | Note |
|-----------|-------|------|
| `oxidize_oscWave` | 0 (Pulse) | Harsher harmonics corrode more dramatically |
| `oxidize_patinaDensity` | 0.4 | Obvious crackle layer |
| `oxidize_patinaTone` | 0.55 | Midrange surface noise |
| `oxidize_sourceBlend` | 0.7 | More noise character |
| `oxidize_tune` | 0 | |
| `oxidize_fine` | 0 | |
| `oxidize_ageRate` | 0.9 | Near-instant aging |
| `oxidize_ageOffset` | 0.0 | Starts clean |
| `oxidize_ageCurve` | 0.75 | Exponential — sudden collapse |
| `oxidize_ageVelSens` | 0.7 | Hard hits arrive substantially pre-aged |
| `oxidize_preserveAmount` | 0.0 | No preservation — let it destroy |
| `oxidize_corrosionDepth` | 0.85 | Heavy saturation |
| `oxidize_corrosionMode` | 5 (Rust) | Asymmetric clipping |
| `oxidize_corrosionVariance` | 0.25 | Unstable, organic |
| `oxidize_erosionRate` | 0.8 | Rapid darkening |
| `oxidize_erosionFloor` | 0.1 | Gets very dark |
| `oxidize_erosionVariance` | 0.15 | |
| `oxidize_erosionRes` | 0.4 | Honky resonant peak as it closes |
| `oxidize_entropyDepth` | 0.75 | Heavy bit reduction |
| `oxidize_entropySmooth` | 0.2 | Hard staircase quantization |
| `oxidize_entropyVariance` | 0.3 | Glitchy |
| `oxidize_wowDepth` | 0.6 | Obvious pitch wobble |
| `oxidize_flutterDepth` | 0.4 | Significant flutter |
| `oxidize_wowRate` | 0.45 | Faster wow |
| `oxidize_flutterRate` | 0.7 | Fast flutter |
| `oxidize_wobbleSpread` | 0.6 | Wide stereo instability |
| `oxidize_dropoutRate` | 0.5 | Frequent dropouts at full age |
| `oxidize_dropoutDepth` | 0.9 | Near-silence during dropout |
| `oxidize_dropoutLength` | 0.3 | Brief but substantial |
| `oxidize_dropoutSmear` | 0.08 | Hard clicks — digital failure |
| `oxidize_sedimentTail` | 0.5 | Medium room |
| `oxidize_sedimentTone` | 0.5 | Midrange |
| `oxidize_sedimentMix` | 0.2 | Restrained — keeps attack clarity |
| `oxidize_macroPATINA` | 0.7 | Pushed toward corrosion |
| `oxidize_macroAGE` | 0.85 | Fast aging |
| `oxidize_macroENTROPY` | 0.7 | High degradation |
| `oxidize_macroSEDIMENT` | 0.2 | Dry, direct |

**Sonic DNA:** brightness `0.35`, warmth `0.45`, movement `0.7`, density `0.7`, space `0.25`, aggression `0.75`

---

### Preset 3: Dropout Beat

**Mood:** Kinetic
**Sonic Description:** A rhythmic texture where the dropout gate creates its own percussive pulse. Set to a medium-fast aging rate, every sustained hit quickly reaches the dropout zone, turning held chords into stutter patterns. The entropy quantizer bites at 12-bit, giving it a sampler-drum-machine texture. This is a loop builder's preset — sustained chords become self-generating rhythm.

| Parameter | Value | Note |
|-----------|-------|------|
| `oxidize_oscWave` | 2 (Noise) | Noise source for percussive texture |
| `oxidize_patinaDensity` | 0.55 | Heavy surface noise |
| `oxidize_patinaTone` | 0.6 | Midrange crackle |
| `oxidize_sourceBlend` | 0.45 | Nearly equal noise/tonal blend |
| `oxidize_tune` | 0 | |
| `oxidize_fine` | 0 | |
| `oxidize_ageRate` | 0.65 | Moderate-fast aging |
| `oxidize_ageOffset` | 0.2 | Starts slightly aged |
| `oxidize_ageCurve` | 0.6 | Moderate exponential |
| `oxidize_ageVelSens` | 0.5 | Velocity drives pre-aging |
| `oxidize_preserveAmount` | 0.0 | Full ruin |
| `oxidize_corrosionDepth` | 0.6 | |
| `oxidize_corrosionMode` | 2 (Broken Speaker) | Ragged papery breakup |
| `oxidize_corrosionVariance` | 0.3 | Unstable, adds rhythmic character |
| `oxidize_erosionRate` | 0.55 | |
| `oxidize_erosionFloor` | 0.2 | |
| `oxidize_erosionVariance` | 0.2 | |
| `oxidize_erosionRes` | 0.15 | |
| `oxidize_entropyDepth` | 0.55 | 12-bit zone |
| `oxidize_entropySmooth` | 0.3 | Semi-hard quantization |
| `oxidize_entropyVariance` | 0.35 | Glitchy rhythm texture |
| `oxidize_wowDepth` | 0.1 | Minimal pitch wobble |
| `oxidize_flutterDepth` | 0.2 | Flutter adds rhythmic shimmer |
| `oxidize_wowRate` | 0.2 | |
| `oxidize_flutterRate` | 0.75 | Fast flutter for stutter character |
| `oxidize_wobbleSpread` | 0.35 | |
| `oxidize_dropoutRate` | 0.75 | High dropout probability at full age |
| `oxidize_dropoutDepth` | 0.95 | Near-complete silence on dropout |
| `oxidize_dropoutLength` | 0.25 | Short beats |
| `oxidize_dropoutSmear` | 0.05 | Hard clicks |
| `oxidize_sedimentTail` | 0.35 | Short accumulation |
| `oxidize_sedimentTone` | 0.55 | Midrange clatter |
| `oxidize_sedimentMix` | 0.15 | Minimal reverb — keep it dry |
| `oxidize_macroPATINA` | 0.55 | |
| `oxidize_macroAGE` | 0.65 | |
| `oxidize_macroENTROPY` | 0.6 | |
| `oxidize_macroSEDIMENT` | 0.15 | |

**Sonic DNA:** brightness `0.4`, warmth `0.3`, movement `0.85`, density `0.65`, space `0.2`, aggression `0.65`

---

### Preset 4: Fossil Drone

**Mood:** Deep
**Sonic Description:** A geological timescale preset. The aging rate is so slow that a single sustained note takes over a minute to reach the "warm saturation" zone. This is a performance instrument for installation artists and long-form composers. Begin the note. Go make tea. Return to find the sound has aged three years in two minutes. The sediment accumulation creates a cave-like resonant space that grows imperceptibly larger.

| Parameter | Value | Note |
|-----------|-------|------|
| `oxidize_oscWave` | 1 (Saw) | |
| `oxidize_patinaDensity` | 0.08 | Very subtle |
| `oxidize_patinaTone` | 0.2 | Low warm rumble |
| `oxidize_sourceBlend` | 0.9 | Mostly tonal |
| `oxidize_tune` | -12 | Dropped octave for geological weight |
| `oxidize_fine` | 0 | |
| `oxidize_ageRate` | 0.04 | Extremely slow — 3min+ to full age |
| `oxidize_ageOffset` | 0.0 | Pristine start |
| `oxidize_ageCurve` | 0.2 | Strong logarithmic — character emerges quickly, ruin is distant |
| `oxidize_ageVelSens` | 0.1 | Minimal velocity influence |
| `oxidize_preserveAmount` | 0.4 | Slows aging significantly before full destruction |
| `oxidize_corrosionDepth` | 0.4 | Moderate final saturation |
| `oxidize_corrosionMode` | 3 (Tape Sat) | Soft, magnetic warmth |
| `oxidize_corrosionVariance` | 0.05 | Almost deterministic |
| `oxidize_erosionRate` | 0.35 | Very gradual darkening |
| `oxidize_erosionFloor` | 0.3 | Stays relatively open |
| `oxidize_erosionVariance` | 0.03 | Extremely stable |
| `oxidize_erosionRes` | 0.18 | Gentle formant tracking |
| `oxidize_entropyDepth` | 0.2 | Minimal reduction |
| `oxidize_entropySmooth` | 0.85 | Very smooth |
| `oxidize_entropyVariance` | 0.04 | |
| `oxidize_wowDepth` | 0.15 | Imperceptible wobble |
| `oxidize_flutterDepth` | 0.03 | |
| `oxidize_wowRate` | 0.1 | Ultra-slow wow |
| `oxidize_flutterRate` | 0.4 | |
| `oxidize_wobbleSpread` | 0.25 | |
| `oxidize_dropoutRate` | 0.02 | Extremely rare |
| `oxidize_dropoutDepth` | 0.6 | Moderate when it occurs |
| `oxidize_dropoutLength` | 0.4 | Longer duration — geological event |
| `oxidize_dropoutSmear` | 0.5 | Soft edges — continental drift, not click |
| `oxidize_sedimentTail` | 0.95 | Near-infinite accumulation |
| `oxidize_sedimentTone` | 0.15 | Dark cave resonance |
| `oxidize_sedimentMix` | 0.5 | Substantial space |
| `oxidize_macroPATINA` | 0.45 | Balanced |
| `oxidize_macroAGE` | 0.05 | Geological time |
| `oxidize_macroENTROPY` | 0.15 | Very light degradation |
| `oxidize_macroSEDIMENT` | 0.7 | Heavy space accumulation |

**Sonic DNA:** brightness `0.2`, warmth `0.8`, movement `0.1`, density `0.45`, space `0.85`, aggression `0.05`

---

### Preset 5: Tape Memory

**Mood:** Atmosphere
**Sonic Description:** Four-track cassette nostalgia distilled into a single patch. The Tape Sat corrosion mode, moderate wow/flutter, and a warm erosion curve combine to produce the sound of a recording that has been played so many times the tape oxide is visibly thinning. There is a melancholy here — something precious being worn away by the act of listening to it. The sediment adds the characteristic "room" of an old rehearsal recording.

| Parameter | Value | Note |
|-----------|-------|------|
| `oxidize_oscWave` | 1 (Saw) | |
| `oxidize_patinaDensity` | 0.3 | Obvious tape hiss |
| `oxidize_patinaTone` | 0.45 | Classic tape hiss register |
| `oxidize_sourceBlend` | 0.75 | |
| `oxidize_tune` | 0 | |
| `oxidize_fine` | 7 | Slight sharp — cheap instrument character |
| `oxidize_ageRate` | 0.35 | Moderate — aging tells a story |
| `oxidize_ageOffset` | 0.15 | Already slightly worn |
| `oxidize_ageCurve` | 0.45 | Nearly linear |
| `oxidize_ageVelSens` | 0.4 | |
| `oxidize_preserveAmount` | 0.35 | Stabilizes at warm tape saturation |
| `oxidize_corrosionDepth` | 0.55 | |
| `oxidize_corrosionMode` | 3 (Tape Sat) | |
| `oxidize_corrosionVariance` | 0.12 | Organic tape imperfection |
| `oxidize_erosionRate` | 0.5 | |
| `oxidize_erosionFloor` | 0.22 | Muddy low-mid darkness |
| `oxidize_erosionVariance` | 0.1 | |
| `oxidize_erosionRes` | 0.2 | |
| `oxidize_entropyDepth` | 0.35 | 14-bit warmth |
| `oxidize_entropySmooth` | 0.7 | Smooth vintage sampler |
| `oxidize_entropyVariance` | 0.1 | |
| `oxidize_wowDepth` | 0.45 | Obvious wow — motor failing |
| `oxidize_flutterDepth` | 0.3 | Clear flutter |
| `oxidize_wowRate` | 0.35 | ~0.6Hz wow |
| `oxidize_flutterRate` | 0.6 | ~9Hz flutter |
| `oxidize_wobbleSpread` | 0.5 | Stereo wow feels like headphone listening |
| `oxidize_dropoutRate` | 0.2 | Occasional tape head loss |
| `oxidize_dropoutDepth` | 0.75 | |
| `oxidize_dropoutLength` | 0.35 | Medium duration — tape skip |
| `oxidize_dropoutSmear` | 0.4 | Soft edges — magnetic, not digital |
| `oxidize_sedimentTail` | 0.65 | Long room reverb |
| `oxidize_sedimentTone` | 0.4 | Warm room character |
| `oxidize_sedimentMix` | 0.35 | Present — you can hear the room |
| `oxidize_macroPATINA` | 0.6 | Warm patina side |
| `oxidize_macroAGE` | 0.4 | |
| `oxidize_macroENTROPY` | 0.35 | |
| `oxidize_macroSEDIMENT` | 0.4 | |

**Sonic DNA:** brightness `0.35`, warmth `0.8`, movement `0.5`, density `0.6`, space `0.55`, aggression `0.2`

---

### Preset 6: Acid Bath

**Mood:** Flux
**Sonic Description:** Sulfuric. Circuit boards dissolving in a chemical bath, metal conducting less and less as the corrosion eats through. The Acid corrosion mode is a nonlinear wavefolder that becomes increasingly metallic and unstable with age — dissonant overtones emerge from the waveshaping mathematics itself, not from the source oscillator. This preset does not age gracefully. It rages and collapses.

| Parameter | Value | Note |
|-----------|-------|------|
| `oxidize_oscWave` | 1 (Saw) | Maximum harmonic content for wavefolder |
| `oxidize_patinaDensity` | 0.5 | Heavy chemical noise |
| `oxidize_patinaTone` | 0.7 | Harsh, acidic register |
| `oxidize_sourceBlend` | 0.6 | Substantial noise contribution |
| `oxidize_tune` | 0 | |
| `oxidize_fine` | 0 | |
| `oxidize_ageRate` | 0.75 | Fast aging |
| `oxidize_ageOffset` | 0.05 | Nearly pristine start — contrast matters |
| `oxidize_ageCurve` | 0.85 | Sharp exponential — sudden catastrophe |
| `oxidize_ageVelSens` | 0.8 | Velocity nearly determines the damage |
| `oxidize_preserveAmount` | 0.0 | Full dissolution |
| `oxidize_corrosionDepth` | 1.0 | Maximum — the acid wins |
| `oxidize_corrosionMode` | 5 (Acid) | Metallic wavefolder |
| `oxidize_corrosionVariance` | 0.4 | Highly unstable |
| `oxidize_erosionRate` | 0.9 | Rapid cutoff collapse |
| `oxidize_erosionFloor` | 0.05 | Near-complete filter closure |
| `oxidize_erosionVariance` | 0.25 | |
| `oxidize_erosionRes` | 0.55 | Sharp resonant peak at the collapsing cutoff — screaming filter |
| `oxidize_entropyDepth` | 0.85 | 5-bit collapse |
| `oxidize_entropySmooth` | 0.1 | Hard staircase quantization |
| `oxidize_entropyVariance` | 0.45 | |
| `oxidize_wowDepth` | 0.7 | |
| `oxidize_flutterDepth` | 0.55 | |
| `oxidize_wowRate` | 0.5 | |
| `oxidize_flutterRate` | 0.8 | |
| `oxidize_wobbleSpread` | 0.7 | Heavily disoriented stereo |
| `oxidize_dropoutRate` | 0.8 | Frequent — signal dissolving |
| `oxidize_dropoutDepth` | 1.0 | Complete silence on dropout |
| `oxidize_dropoutLength` | 0.45 | Long drops |
| `oxidize_dropoutSmear` | 0.02 | Hard — circuit failure |
| `oxidize_sedimentTail` | 0.7 | Acid rain in a cavern |
| `oxidize_sedimentTone` | 0.65 | Bright metallic reverb |
| `oxidize_sedimentMix` | 0.3 | |
| `oxidize_macroPATINA` | 0.9 | Full corrosion |
| `oxidize_macroAGE` | 0.8 | |
| `oxidize_macroENTROPY` | 0.9 | Maximum entropy |
| `oxidize_macroSEDIMENT` | 0.3 | |

**Sonic DNA:** brightness `0.55`, warmth `0.15`, movement `0.9`, density `0.8`, space `0.4`, aggression `0.95`

---

### Preset 7: Sediment Layer

**Mood:** Submerged
**Sonic Description:** Each held note adds another geological layer to the sediment reverb. The engine's accumulating reverb architecture means the first note of a session sounds different from the tenth — the space itself ages. Notes are clean on attack but sink into increasingly thick resonant sediment with each sustain. Built for ambient layering sessions where the room becomes a character in the composition.

| Parameter | Value | Note |
|-----------|-------|------|
| `oxidize_oscWave` | 1 (Saw) | |
| `oxidize_patinaDensity` | 0.12 | Subtle shimmer |
| `oxidize_patinaTone` | 0.25 | Low warm shimmer |
| `oxidize_sourceBlend` | 0.88 | Mostly tonal |
| `oxidize_tune` | 0 | |
| `oxidize_fine` | -5 | Slight flat |
| `oxidize_ageRate` | 0.3 | |
| `oxidize_ageOffset` | 0.0 | |
| `oxidize_ageCurve` | 0.3 | |
| `oxidize_ageVelSens` | 0.2 | |
| `oxidize_preserveAmount` | 0.6 | High preservation — keeps signal musical |
| `oxidize_corrosionDepth` | 0.35 | Light saturation |
| `oxidize_corrosionMode` | 0 (Valve) | Warm tube |
| `oxidize_corrosionVariance` | 0.06 | |
| `oxidize_erosionRate` | 0.4 | |
| `oxidize_erosionFloor` | 0.3 | Stays open enough to support melody |
| `oxidize_erosionVariance` | 0.05 | |
| `oxidize_erosionRes` | 0.3 | Beautiful slow-closing formant |
| `oxidize_entropyDepth` | 0.25 | |
| `oxidize_entropySmooth` | 0.8 | Very smooth |
| `oxidize_entropyVariance` | 0.06 | |
| `oxidize_wowDepth` | 0.12 | |
| `oxidize_flutterDepth` | 0.05 | |
| `oxidize_wowRate` | 0.15 | |
| `oxidize_flutterRate` | 0.45 | |
| `oxidize_wobbleSpread` | 0.35 | |
| `oxidize_dropoutRate` | 0.03 | Extremely rare |
| `oxidize_dropoutDepth` | 0.65 | |
| `oxidize_dropoutLength` | 0.5 | Long, soft, geological when it occurs |
| `oxidize_dropoutSmear` | 0.6 | Very soft — plates shifting |
| `oxidize_sedimentTail` | 0.98 | Near-infinite |
| `oxidize_sedimentTone` | 0.12 | Extremely dark, subterranean |
| `oxidize_sedimentMix` | 0.65 | Dominant sediment |
| `oxidize_macroPATINA` | 0.4 | |
| `oxidize_macroAGE` | 0.3 | |
| `oxidize_macroENTROPY` | 0.2 | |
| `oxidize_macroSEDIMENT` | 0.85 | Maximum sediment |

**Sonic DNA:** brightness `0.15`, warmth `0.7`, movement `0.2`, density `0.8`, space `0.95`, aggression `0.05`

---

### Preset 8: Coupled Entropy

**Mood:** Entangled
**Coupling partner:** OUROBOROS
**Sonic Description:** Designed as the OXIDIZE half of an OUROBOROS coupling patch. The feedback signal from OUROBOROS's strange-attractor topology is routed as a coupling input to OXIDIZE, modulating its ageRate — the louder the OUROBOROS feedback grows, the faster OXIDIZE ages. This creates a feedback-corrosion loop: aging produces distortion, distortion feeds back into aging rate. The loop stabilizes at a chaotic equilibrium that sounds like a machine slowly dissolving in its own heat.

> Coupling note: In the coupling matrix, route OUROBOROS amplitude output → OXIDIZE `ageRate` modulation. Strength 0.6–0.8 for balanced chaos.

| Parameter | Value | Note |
|-----------|-------|------|
| `oxidize_oscWave` | 1 (Saw) | |
| `oxidize_patinaDensity` | 0.25 | |
| `oxidize_patinaTone` | 0.5 | |
| `oxidize_sourceBlend` | 0.7 | |
| `oxidize_tune` | 0 | |
| `oxidize_fine` | 12 (cents) | Slight sharp — creates beating with OUROBOROS |
| `oxidize_ageRate` | 0.5 | Medium base rate — coupling will push it higher |
| `oxidize_ageOffset` | 0.1 | |
| `oxidize_ageCurve` | 0.55 | |
| `oxidize_ageVelSens` | 0.45 | |
| `oxidize_preserveAmount` | 0.1 | Almost no preservation — designed for controlled chaos |
| `oxidize_corrosionDepth` | 0.7 | |
| `oxidize_corrosionMode` | 5 (Rust) | Asymmetric — pairs well with OUROBOROS chaotic output |
| `oxidize_corrosionVariance` | 0.3 | |
| `oxidize_erosionRate` | 0.6 | |
| `oxidize_erosionFloor` | 0.12 | Very dark at full age |
| `oxidize_erosionVariance` | 0.2 | |
| `oxidize_erosionRes` | 0.45 | Sharp peak — responds to coupling dynamics |
| `oxidize_entropyDepth` | 0.65 | |
| `oxidize_entropySmooth` | 0.35 | |
| `oxidize_entropyVariance` | 0.3 | |
| `oxidize_wowDepth` | 0.35 | |
| `oxidize_flutterDepth` | 0.25 | |
| `oxidize_wowRate` | 0.4 | |
| `oxidize_flutterRate` | 0.65 | |
| `oxidize_wobbleSpread` | 0.55 | |
| `oxidize_dropoutRate` | 0.4 | Coupling chaos will trigger these |
| `oxidize_dropoutDepth` | 0.8 | |
| `oxidize_dropoutLength` | 0.2 | |
| `oxidize_dropoutSmear` | 0.1 | Semi-hard |
| `oxidize_sedimentTail` | 0.75 | |
| `oxidize_sedimentTone` | 0.5 | |
| `oxidize_sedimentMix` | 0.35 | |
| `oxidize_macroPATINA` | 0.65 | |
| `oxidize_macroAGE` | 0.55 | Coupling will push this dynamically |
| `oxidize_macroENTROPY` | 0.65 | |
| `oxidize_macroSEDIMENT` | 0.4 | |

**Sonic DNA:** brightness `0.45`, warmth `0.4`, movement `0.8`, density `0.7`, space `0.45`, aggression `0.7`

---

## 3. Blessing Candidates

### Candidate 1 — Aging Preservation Paradox (Proposed B044)

**The phenomenon:** `oxidize_preserveAmount` establishes a ceiling on aging progression. Below this ceiling, the sound ages freely toward destruction. Above it, aging slows exponentially and asymptotically approaches a "beautiful ruin" stable state — never regressing to pristine, never collapsing to noise.

**Why it deserves a Blessing:** This is the first synthesizer parameter that embodies the chemistry of *patination as protection*. In physical chemistry, copper oxide (verdigris) acts as a passivation layer — it slows further corrosion of the underlying copper. OXIDIZE's preserveAmount parameter replicates this physics as DSP: early degradation protects against catastrophic later degradation. The result is a stable "aged equilibrium" state that is musically beautiful and structurally novel. No other synthesis metaphor has modeled a self-limiting degradation chemistry. It extends B040 (Note Duration as Synthesis Parameter) by adding a *termination condition* — duration drives synthesis, but the synthesis can choose to stop aging itself.

**Proposed name:** Passivation Layer Architecture — First synthesizer to model the chemistry of protective patination as a synthesis termination condition: aging drives a self-limiting process that, above a threshold, converts destructive corrosion into preservation stasis. The beautiful ruin becomes its own shield.

---

### Candidate 2 — Geological Time Axis (Proposed B045)

**The phenomenon:** The combination of `oxidize_ageRate` (0.01Hz to max) and `oxidize_ageCurve` (logarithmic through exponential) creates a 2D parameter space where the *temporal experience of sound* becomes a first-class compositional dimension. At extreme logarithmic + ultra-low ageRate, a single note can undergo perceptible change across multiple minutes. At extreme exponential + high ageRate, decades of aging are compressed into sub-second gestures.

**Why it deserves a Blessing:** B040 (OXYTOCIN: Note Duration as Synthesis Parameter) established that *how long you hold a note* affects synthesis. OXIDIZE extends this into a second dimension: *how time is perceived to pass* within the note. The ageCurve parameter means that identical note durations can produce radically different aging profiles — the same 5-second hold sounds ancient on a logarithmic curve (early aging, then stability) and catastrophic on an exponential curve (sudden late-stage collapse). This is fundamentally a different synthesis parameter than anything in the fleet: it does not control a sound property, it controls the *temporal shape of transformation itself*. This may be the most philosophically novel parameter in XOceanus.

**Proposed name:** Temporal Curvature as Synthesis Parameter — First synthesizer engine in which the shape of time — not merely its passage — is a primary synthesis variable. The aging curve parameter allows composers to choose between geological time (long early transformation, stable plateau), linear time (steady degradation), and catastrophic time (stable beginning, sudden late-stage collapse). Duration alone does not determine aging; the geometry of duration does.

---

### Candidate 3 — Accumulating Sediment Architecture (Proposed B046)

**The phenomenon:** `oxidize_sedimentTail` at high values (0.9+) creates a reverb whose feedback coefficient exceeds the decay threshold, causing energy to accumulate across notes. The sediment is not a per-note reverb — it is a session-state reverb. The room gets bigger as the session progresses.

**Why it deserves a Blessing:** Every other reverb in the fleet (LushReverb, AquaticFXSuite's Fathom, Oxbow's Chiasmus FDN) decays to silence given sufficient time. OXIDIZE's sediment architecture intentionally violates this invariant — it is an accumulating reverb that grows richer with each note, modulated by the aging system so that older, more corroded notes contribute more sediment than fresh notes. This means the *history of the performance* is encoded into the reverb's current state. The performer is not just playing notes — they are depositing geological strata. The first chord of a session sounds different from the hundredth chord not because of any preset change, but because the sediment has accumulated. This is the first reverb in the fleet that has a memory of the performance, not just the note.

**Proposed name:** Stratigraphic Reverb Memory — First reverb architecture in the fleet whose state encodes the history of the performance rather than only the current note. Energy accumulates across notes according to their age and degradation state, creating a geological record in the acoustic space. The room at minute ten of a performance holds the fossils of every note played before it.

---

## 4. Doctrine of CPU Stewardship

### The Challenge

Six independent processing stages, each driven by a continuously evolving note-age accumulator. At 48kHz with 8 voices and block size 128, the per-sample chain must complete in approximately 1.4 microseconds per voice to maintain 10% CPU budget (rough target for a single engine in the fleet). This analysis identifies where sample-rate precision is architecturally required versus where block-rate computation is perceptually equivalent.

---

### Stage-by-Stage CPU Classification

| Stage | Current Assumed Rate | Recommended Rate | Reasoning |
|-------|---------------------|-----------------|-----------|
| Age Accumulator | Sample-rate | **Block-rate** | The accumulator integrates note duration. Advancing it once per 128 samples (2.67ms at 48kHz) produces age steps of 2.67ms — imperceptible against any ageRate value a performer would use. Update once per block, cache the result. |
| Age → parameter mapping | Sample-rate | **Block-rate** | All 6 age-driven parameter derivations (corrosionDepth_current, erosionCutoff_current, etc.) can be computed once per block using the cached age value. These are smooth polynomial/exponential curves — no aliasing risk from block-rate derivation. |
| Corrosion Waveshaper | Sample-rate | **Sample-rate (required)** | Waveshaping operates on the audio signal — must run per-sample. However, the waveshaper *curve parameters* can be block-rate. Precompute the waveshaper coefficient set once per block, then apply the fixed curve sample-by-sample. |
| Erosion Filter (SVF) | Sample-rate | **Sample-rate (required)** | Filter state variables must update per-sample. However, the cutoff frequency driving the SVF can be block-rate computed (smooth enough that 128-sample steps produce no audible zipper). Use first-order IIR smoothing of the block-rate target to prevent coefficient discontinuity. |
| Entropy Quantizer | Sample-rate | **Sample-rate (required)** | Bit reduction and sample-rate reduction must operate on the audio signal per-sample. The *depth* (bit depth target) can be block-rate. |
| Wobble Generator | Sample-rate | **Sample-rate (required)** | Pitch modulation via the wow/flutter LFOs must update the oscillator phase per-sample for correct frequency deviation. However, the LFO *depth scaling* from the age accumulator can be block-rate. |
| Dropout Gate | **Lookup table required** | Block-rate probability check | The dropout probability is a function of the current age value. Precompute a 256-entry lookup table mapping normalized age (0.0–1.0) to dropout probability. At block boundaries, look up current probability; generate a single random float; if below probability threshold, schedule a dropout event for this block. Do not re-evaluate per sample. |
| Sediment Reverb | Sample-rate FDN | **Sample-rate (required)** | FDN state must update per sample. The accumulation coefficient update (sedimentTail × ageScaling) can be block-rate. Clamp the feedback coefficient at 0.999 to prevent instability at extreme settings. |
| Patina Noise Oscillator | Sample-rate | **Can use 2x oversampling reduction** | The patina oscillator generates pitched noise. It does not require the same sample rate as the main signal path. Generate at sample-rate but consider a simple first-order decimation if CPU proves tight. |

---

### Lookup Table Recommendations

| Computation | Table Size | Update Frequency | Memory Cost |
|-------------|------------|-----------------|-------------|
| Age → corrosion curve | 512 floats | On ageCurve parameter change | 2KB |
| Age → dropout probability | 256 floats | On dropoutRate parameter change | 1KB |
| Age → erosion cutoff | 512 floats | On erosionRate/Floor parameter change | 2KB |
| Age → entropy depth | 256 floats | On entropyDepth parameter change | 1KB |
| Corrosion waveshaper (per-mode) | 2048 floats × 6 modes | On mode change or depth change | 48KB |
| Wow LFO waveform | 1024 floats | Static (sinusoidal) | 4KB |
| Flutter LFO waveform | 1024 floats | Static (sinusoidal + random) | 4KB |

Total lookup table overhead: approximately 62KB per voice context (shared tables, so single allocation). This is negligible.

---

### Critical Invariant: Age Accumulator as Block-Rate Gate

All age-derived parameter scaling should follow this pattern:

```cpp
// Per block (not per sample)
float ageNorm = std::min(1.0f, ageAccumulator_[voice] / maxAge_);
float corrosionCurrent = corrosionDepth_ * ageCurveTable_[int(ageNorm * 511)];
float erosionCutoff = lerp(20000.0f, erosionFloor_Hz_, ageCurveTable_[int(ageNorm * 511)] * erosionRate_);
float dropoutProb = dropoutRate_ * dropoutAgeTable_[int(ageNorm * 255)];
// ... etc. — all computed once per block
```

Then per-sample, the waveshaper, filter, and quantizer receive these *pre-computed, fixed* parameter values for the duration of the block. This eliminates 5 of the 6 per-sample curve evaluations, replacing them with single array lookups per block.

---

### Expected Voice Count

With block-rate age/parameter derivation and lookup table waveshapers:

| CPU Budget | Expected Voice Count | Notes |
|-----------|---------------------|-------|
| 5% (minimal) | 4–5 voices | Enough for pads and lead |
| 10% (target) | 7–9 voices | Full polyphony for most use cases |
| 15% (generous) | 11–14 voices | With Voice Stealing LRU engaged |
| 20% (upper bound) | 16+ voices | If sediment reverb is shared across voices |

**Key optimization recommendation:** The sediment reverb should be a single shared FDN, not per-voice. All voice outputs feed into a single accumulating reverb — this is also architecturally correct for the sediment metaphor (the space accumulates from all sounds, not from each note individually). This change alone reduces sediment CPU cost from O(voices) to O(1), enabling 2-3 additional voices at the same budget.

**Denormal protection note:** The feedback path in the sediment reverb's FDN is a denormal attack vector. Insert `flushDenormal()` on all FDN state variables at the top of each block. The dropout gate's amplitude-zeroing events will also produce sub-normal values — flush after every dropout event completes.

---

## Closing Meditation

OXIDIZE arrives with strong bones: a clear thesis, evocative vocabulary, a natural doctrine compliance, and a coupling story that extends two existing Blessings rather than merely claiming novelty. The six additions I have proposed — oscillator tune/fine, preservation ceiling, erosion resonance, split wobble rates, and dropout smear — are not cosmetic refinements. They are the difference between an engine that demonstrates an idea and one that plays every idea it contains.

The three Blessing candidates represent genuine contributions to synthesis philosophy: passivation chemistry as DSP metaphor, temporal curvature as synthesis variable, and performance-history-encoded reverb memory. Any one of these would be notable in isolation. All three in one engine is unusual.

The CPU path is manageable. The sediment reverb sharing insight is essential and should be implemented from day one. Block-rate age derivation with lookup table parameter curves will keep this engine competitive for voice count.

Go build it. Age it well.

— *Guru Bin*
