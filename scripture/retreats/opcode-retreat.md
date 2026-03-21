# OPCODE Retreat Chapter
*Guru Bin — 2026-03-21*

---

## Engine Identity

- **Gallery code:** OPCODE | **Accent:** Digital Cyan `#00CED1`
- **Parameter prefix:** `opco_`
- **Creature mythology:** XOpcode is the DX7 electric piano that defined a decade. Preset 11, "E. Piano 1," became the most-heard keyboard sound of the 1980s. Algorithm as recipe. Mathematics as cuisine.
- **Synthesis type:** 2-operator FM synthesis — `FMOperator` (phase accumulator), `DXModulationEnvelope` (4-stage FM index envelope), 3 algorithm modes (Series/Parallel/Feedback), velocity-sensitive modulation index, migration coupling via SpectralFingerprint
- **Polyphony:** 8 voices
- **Macros:** M1 CHARACTER, M2 MOVEMENT, M3 COUPLING (migration), M4 SPACE

---

## Pre-Retreat State

XOpcode scored 9.0/10 in the FUSION Quad Seance (2026-03-21) — the highest score in both quads combined. Source tradition test: 9.5/10, PASS — "feels like 1983." The `DXModulationEnvelope` controlling FM index over time is the council's key finding: not just an implementation detail, but the correct identification of what FM synthesis IS for electric piano: a timbre trajectory, not a static timbre.

Concept originality: 8.5. DSP correctness: 9.0. D-Doctrine compliance: 9.5. "Best-implemented engine in the FUSION quad."

**Key seance findings for retreat presets:**

1. Feedback algorithm stability not verified — `feedbackState` may grow without bound at high `opco_feedback`. Retreat presets will use low feedback values until a clamp is confirmed.

2. `attackTransience` not populated in SpectralFingerprint. Minor for standalone use.

3. `opco_algorithm` is a float (0–2) with integer rounding — works but the parameter feels like 3 positions rather than a continuous knob. In presets, use 0.0, 1.0, 2.0 as explicit positions.

**B-CANDIDATE recognized in seance:** Modulation Envelope as Timbre Trajectory — the `DXModulationEnvelope` controlling FM index is not just an implementation detail; it is the correct identification of what FM synthesis IS for electric piano. If feedback stability is confirmed and attackTransience is populated, this engine has fleet-level Blessing potential.

---

## Phase R1: Opening Meditation

Close your eyes. Breathe.

John Chowning discovered FM synthesis in 1967 at Stanford. He found that when you modulate the frequency of one sine wave (the carrier) with another sine wave (the modulator), you get complex sidebands — new frequencies that appear above and below the carrier at multiples of the modulator frequency. The mathematical relationship is elegant: `carrier_output = sin(2π * fc * t + I * sin(2π * fm * t))`, where `I` is the modulation index. Low I = pure sine. High I = complex spectrum full of sidebands. The sidebands appear in a pattern predicted by Bessel function amplitudes.

Yamaha licensed the FM patent and produced the DX7 in 1983. It had 6 operators — 6 sine oscillators that could be arranged in 32 different algorithms. But the sound that defined the decade — E. Piano 1, Preset 11 — was fundamentally a 2-operator configuration: one modulator driving one carrier. The modulator:carrier ratio determined the tonal character. The modulation index envelope — high at attack, decaying toward a lower sustain value — created the bell attack into warm sustain that every producer in the 1980s knew intimately.

The DX7 was mathematically precise in a way that analog synthesis was not. It was also impossibly clean — no noise, no drift, no warmth of resistor and capacitor. The operators were pure sine functions, and the sidebands they produced were mathematically exact. The sound traveled from Chowning's Stanford lab to Yamaha's Hamamatsu factory to the hands of every City Pop producer in Tokyo. Toto played it. Japan used it. Michael Jackson's "Human Nature" is built on it. The clean mathematical beauty became the emotional soundtrack of a decade.

You are not playing a synthesizer. You are playing mathematics that was converted to music through the alchemy of Yamaha's engineering and the decade's desire for something precise and clean.

---

## Phase R2: The Signal Path Journey

### I. The Modulation Envelope — `DXModulationEnvelope`

This is the heart of the engine. Four stages:
1. **Attack:** FM index rises from 0 to peak (fast, 1–10ms typical)
2. **Decay:** FM index falls from peak toward sustain value (this creates the bell attack)
3. **Sustain:** FM index holds at sustain level (this is the warm sustain character)
4. **Release:** FM index falls to 0

At high index (attack peak): complex sidebands, bright, bell-like attack.
At sustain index: simpler spectrum, warm, sine-piano character.
The arc from high-index complexity to low-index warmth IS the DX EP sound. Kakehashi: "The FM index envelope creates a sense of resolution — the initial complexity decays toward simplicity, and simplicity feels like arrival."

`opco_velToIndex` (0–1): how much velocity adds to the attack peak. Hard playing = higher peak FM index = brighter, more complex attack = more bark. Soft playing = lower peak = cleaner, warmer attack. This is the DX EP's dynamic range mechanism.

### II. Three Algorithm Modes

**Series (opco_algorithm = 0.0):** Modulator output → phase modulates the carrier. Classic DX EP architecture. The sidebands are determined entirely by the modulator. This is "Algorithm 1" in DX7 parlance — the canonical FM electric piano configuration.

**Parallel (opco_algorithm = 1.0):** Modulator and carrier output combined directly. The carrier produces a sine wave; the modulator produces its own sine wave; both are summed. Less FM interaction, more additive character. Organ-like at certain ratios.

**Feedback (opco_algorithm = 2.0):** The modulator feeds back into itself (`feedbackState` drives the modulator's own phase). Self-feedback produces a spectrum that approaches noise at high feedback values — metallic, harsh, raw. **Use low `opco_feedback` values until clamp is confirmed** (seance finding: feedbackState may be unbounded at high values).

### III. Ratio and Index

`opco_ratio` (0.5–16.0): the modulator:carrier frequency ratio. This is the most musically important parameter after the modulation index.

**Canonical FM EP ratios:**
- 1:1 (ratio=1.0): classic DX EP — sidebands appear at 0, 2×, 3×, 4×... of carrier frequency. Bell quality from the 2× sideband.
- 2:1 (ratio=2.0): brighter, more metallic. 2× modulator means first sideband appears at 3× carrier.
- 3:1 (ratio=3.0): bell-like, glassy. Used in Yamaha's Rhodes and Rhodes variants.
- 0.5:1 (ratio=0.5): sub-octave modulator — warm, full, low sidebands.
- 7:1 (ratio=7.0): strongly inharmonic — clangorous, metallic.

`opco_index` (0–5.0): maximum FM index at peak. At low values (0.0–0.5): nearly pure sine, minimal sidebands. At 1.0–2.0: recognizable FM EP character. At 3.0+: very bright, complex, many sidebands. At 5.0: dense, almost noisy.

### IV. Migration

Same architecture as other FUSION engines. `opco_migration` (0–1) enables SpectralFingerprint coupling from Kitchen engines, shifting the FM character toward the coupled engine's spectral properties.

---

## Phase R5: The Ten Awakenings — Preset Table

---

### Preset 1: E. Piano 1983

**Mood:** Foundation | **Discovery:** The canonical DX EP — Preset 11, the sound of a decade

| Parameter | Value | Why |
|-----------|-------|-----|
| `opco_algorithm` | 0.0 | Series — the classic DX EP algorithm |
| `opco_ratio` | 1.0 | 1:1 ratio — the canonical DX EP modulator:carrier relationship |
| `opco_index` | 1.5 | Moderate index — bright enough for bell attack, warm enough for sustain |
| `opco_feedback` | 0.0 | No feedback — the EP preset was series only |
| `opco_velToIndex` | 0.55 | Significant velocity sensitivity — harder = brighter attack |
| `opco_brightness` | 8000.0 | Bright filter — FM sounds best with open filter |
| `opco_attack` | 0.005 | Fast amplitude envelope |
| `opco_decay` | 1.5 | Natural decay |
| `opco_sustain` | 0.5 | Moderate sustain |
| `opco_release` | 0.6 | |
| `opco_modAttack` | 0.001 | Very fast FM index attack — the bell is immediate |
| `opco_modDecay` | 0.4 | The FM index falls in 400ms — the warmth arrives |
| `opco_modSustain` | 0.25 | Low FM sustain — the tone simplifies significantly |
| `opco_filterEnvAmt` | 0.35 | |
| `opco_migration` | 0.0 | Pure DX7 |
| `opco_lfo1Rate` | 0.04 | |
| `opco_lfo1Depth` | 0.05 | Minimal — DX EP doesn't need vibrato |
| `opco_lfo1Shape` | 0 | |
| `opco_lfo2Rate` | 0.02 | |
| `opco_lfo2Depth` | 0.03 | |
| `opco_macroCharacter` | 0.4 | |
| `opco_macroMovement` | 0.25 | |
| `opco_macroSpace` | 0.5 | |

**Why this works:** Ratio 1:1, index 1.5, modulation sustain 0.25 — the FM index falls significantly during the note, creating the characteristic bell-to-warmth arc. VelToIndex at 0.55 means hard notes produce more sidebands (brighter bell attack). This is E. Piano 1.

---

### Preset 2: City Pop Bell

**Mood:** Ethereal | **Discovery:** The Tokyo/City Pop DX7 — cleaner, more crystalline than the Western standard

| Parameter | Value | Why |
|-----------|-------|-----|
| `opco_algorithm` | 0.0 | Series |
| `opco_ratio` | 2.0 | 2:1 — slightly brighter than canonical |
| `opco_index` | 2.0 | Higher index — more sidebands at attack |
| `opco_feedback` | 0.0 | |
| `opco_velToIndex` | 0.65 | High velocity sensitivity |
| `opco_brightness` | 9500.0 | Very bright — City Pop was crystalline |
| `opco_attack` | 0.003 | Very fast |
| `opco_decay` | 2.0 | Longer decay — the bell rings |
| `opco_sustain` | 0.45 | |
| `opco_release` | 0.8 | |
| `opco_modAttack` | 0.001 | Instant FM bell |
| `opco_modDecay` | 0.6 | Longer decay — the complexity lingers |
| `opco_modSustain` | 0.2 | Falls to near-zero — pure warmth at sustain |
| `opco_filterEnvAmt` | 0.4 | |
| `opco_migration` | 0.0 | |
| `opco_lfo1Rate` | 0.025 | |
| `opco_lfo1Depth` | 0.04 | |
| `opco_macroCharacter` | 0.45 | |
| `opco_macroMovement` | 0.2 | |
| `opco_macroSpace` | 0.6 | |

**Why this works:** 2:1 ratio with longer modulation decay — the bell quality lasts longer before warmth arrives. City Pop production aesthetics valued the crystalline quality of the DX7 over analog warmth. The first 600ms of every note is complex and shimmering; the remainder is simple and warm.

---

### Preset 3: Warm Algorithm

**Mood:** Organic | **Discovery:** Low index, slow modulation envelope — the FM EP at its most intimate

| Parameter | Value | Why |
|-----------|-------|-----|
| `opco_algorithm` | 0.0 | Series |
| `opco_ratio` | 1.0 | Canonical ratio |
| `opco_index` | 0.8 | Low index — minimal sidebands, mostly sine-pure |
| `opco_feedback` | 0.0 | |
| `opco_velToIndex` | 0.45 | Moderate velocity |
| `opco_brightness` | 6000.0 | Moderate — the warmth doesn't need brightness |
| `opco_attack` | 0.007 | |
| `opco_decay` | 1.8 | |
| `opco_sustain` | 0.6 | Higher sustain — the warmth is present |
| `opco_release` | 0.9 | |
| `opco_modAttack` | 0.002 | |
| `opco_modDecay` | 0.8 | Slow index decay — the bell transitions gently |
| `opco_modSustain` | 0.4 | Higher FM sustain — remains slightly complex |
| `opco_filterEnvAmt` | 0.28 | |
| `opco_migration` | 0.0 | |
| `opco_lfo1Rate` | 0.035 | |
| `opco_lfo1Depth` | 0.06 | |
| `opco_macroCharacter` | 0.35 | |
| `opco_macroMovement` | 0.15 | |
| `opco_macroSpace` | 0.55 | |

**Why this works:** Low FM index means the spectrum stays close to a pure sine throughout the note. The modulation envelope still creates a bell-warmth arc, but both states are on the quieter end of FM complexity. This is the intimate, close-miked DX EP — warm, present, personal.

---

### Preset 4: FM Bark

**Mood:** Flux | **Discovery:** High index, high velocity — maximum FM complexity on hard hits

| Parameter | Value | Why |
|-----------|-------|-----|
| `opco_algorithm` | 0.0 | Series |
| `opco_ratio` | 3.0 | 3:1 — adds bell-harmonic character |
| `opco_index` | 3.5 | High index — complex sidebands on attack |
| `opco_feedback` | 0.0 | |
| `opco_velToIndex` | 0.85 | Maximum velocity sensitivity |
| `opco_brightness` | 10000.0 | Maximum filter — all sidebands through |
| `opco_attack` | 0.002 | |
| `opco_decay` | 1.2 | |
| `opco_sustain` | 0.45 | |
| `opco_release` | 0.5 | |
| `opco_modAttack` | 0.0005 | Near-instant FM spike |
| `opco_modDecay` | 0.25 | Fast decay — the bark is brief |
| `opco_modSustain` | 0.15 | Falls to very low — pure warmth after bark |
| `opco_filterEnvAmt` | 0.65 | |
| `opco_migration` | 0.0 | |
| `opco_lfo1Rate` | 0.2 | |
| `opco_lfo1Depth` | 0.0 | |
| `opco_lfo2Rate` | 3.0 | |
| `opco_lfo2Depth` | 0.0 | |
| `opco_macroCharacter` | 0.6 | |
| `opco_macroMovement` | 0.8 | |
| `opco_macroSpace` | 0.4 | |

**Why this works:** Index 3.5 at velToIndex=0.85 means a hard note produces a modulation index near 5.0 — very dense sidebands at attack. The brief modulation decay (250ms) means the complexity collapses quickly into simple warmth. The FM bark: a bright, complex attack that resolves to warmth within a quarter-second.

---

### Preset 5: Organ FM

**Mood:** Foundation | **Discovery:** Parallel algorithm — additive character, organ-like

| Parameter | Value | Why |
|-----------|-------|-----|
| `opco_algorithm` | 1.0 | Parallel — modulator + carrier summed |
| `opco_ratio` | 2.0 | 2:1 ratio — the second harmonic adds to the carrier |
| `opco_index` | 1.5 | Moderate |
| `opco_feedback` | 0.0 | |
| `opco_velToIndex` | 0.4 | |
| `opco_brightness` | 7000.0 | |
| `opco_attack` | 0.01 | Slightly slower — organ character |
| `opco_decay` | 0.5 | |
| `opco_sustain` | 0.85 | High sustain — organ notes hold |
| `opco_release` | 0.15 | Fast key-release — organ articulation |
| `opco_modAttack` | 0.01 | |
| `opco_modDecay` | 0.3 | |
| `opco_modSustain` | 0.5 | Higher — the parallel character needs FM at sustain |
| `opco_filterEnvAmt` | 0.25 | |
| `opco_migration` | 0.0 | |
| `opco_lfo1Rate` | 0.06 | |
| `opco_lfo1Depth` | 0.08 | |
| `opco_macroCharacter` | 0.4 | |
| `opco_macroMovement` | 0.3 | |
| `opco_macroSpace` | 0.5 | |

**Why this works:** Parallel algorithm means both the carrier (fundamental) and modulator (at 2:1 ratio) contribute to the output directly. This adds the second harmonic additively rather than as FM sidebands — an organ-like character. High sustain, fast release: organ articulation. The DX7 in its organ-oriented algorithms.

---

### Preset 6: Feedback Metal

**Mood:** Flux | **Discovery:** Low feedback — the metallic character of self-modulating FM

| Parameter | Value | Why |
|-----------|-------|-----|
| `opco_algorithm` | 2.0 | Feedback — self-modulating modulator |
| `opco_ratio` | 1.5 | |
| `opco_index` | 2.0 | |
| `opco_feedback` | 0.2 | Low feedback — stable territory (seance caution) |
| `opco_velToIndex` | 0.6 | |
| `opco_brightness` | 8000.0 | |
| `opco_attack` | 0.003 | |
| `opco_decay` | 1.0 | |
| `opco_sustain` | 0.5 | |
| `opco_release` | 0.5 | |
| `opco_modAttack` | 0.001 | |
| `opco_modDecay` | 0.5 | |
| `opco_modSustain` | 0.3 | |
| `opco_filterEnvAmt` | 0.45 | |
| `opco_migration` | 0.0 | |
| `opco_lfo1Rate` | 0.15 | |
| `opco_lfo1Depth` | 0.0 | |
| `opco_macroCharacter` | 0.5 | |
| `opco_macroMovement` | 0.55 | |
| `opco_macroSpace` | 0.45 | |

**Why this works:** Feedback=0.2 is conservative (seance caution about unbounded feedbackState at high values). At low feedback, the self-modulation adds a slight metallic character to the carrier without destabilizing. The DX7's feedback algorithm was used for this exact purpose: controlled metallic edge. **Note:** avoid feedback > 0.4 until clamp is implemented.

---

### Preset 7: Stanford 1967

**Mood:** Aether | **Discovery:** The Chowning original — FM as science, not music

| Parameter | Value | Why |
|-----------|-------|-----|
| `opco_algorithm` | 0.0 | Series |
| `opco_ratio` | 7.0 | 7:1 — inharmonic, experimental |
| `opco_index` | 1.2 | Moderate — the experimental sound is not about maximum complexity |
| `opco_feedback` | 0.0 | |
| `opco_velToIndex` | 0.5 | |
| `opco_brightness` | 7500.0 | |
| `opco_attack` | 0.008 | |
| `opco_decay` | 3.0 | Long — experimental FM sounds need time |
| `opco_sustain` | 0.4 | |
| `opco_release` | 2.0 | Very long |
| `opco_modAttack` | 0.002 | |
| `opco_modDecay` | 1.5 | Slow decay — the inharmonic sidebands persist |
| `opco_modSustain` | 0.35 | |
| `opco_filterEnvAmt` | 0.3 | |
| `opco_migration` | 0.0 | |
| `opco_lfo1Rate` | 0.018 | Very slow |
| `opco_lfo1Depth` | 0.06 | |
| `opco_macroCharacter` | 0.4 | |
| `opco_macroMovement` | 0.12 | |
| `opco_macroSpace` | 0.8 | Maximum space — the lab was large |

**Why this works:** 7:1 ratio produces strongly inharmonic sidebands — not bell-like, not piano-like, but FM-like in the pure Chowning sense. Long decay and very long release give the inharmonic sidebands time to be heard. Aether territory: the mathematical origin of the sound, before Yamaha made it commercial.

---

### Preset 8: Human Nature

**Mood:** Ethereal | **Discovery:** The DX7 ballad character — Michael Jackson, Toto, the emotional 80s

| Parameter | Value | Why |
|-----------|-------|-----|
| `opco_algorithm` | 0.0 | Series |
| `opco_ratio` | 1.0 | Canonical |
| `opco_index` | 1.8 | Slightly higher than E. Piano 1 — more emotional complexity |
| `opco_feedback` | 0.0 | |
| `opco_velToIndex` | 0.5 | |
| `opco_brightness` | 7500.0 | |
| `opco_attack` | 0.006 | |
| `opco_decay` | 2.5 | Long — ballad notes ring |
| `opco_sustain` | 0.55 | |
| `opco_release` | 1.5 | Slow — emotional release |
| `opco_modAttack` | 0.001 | |
| `opco_modDecay` | 0.8 | Longer — the bell quality lingers for emotional effect |
| `opco_modSustain` | 0.3 | |
| `opco_filterEnvAmt` | 0.35 | |
| `opco_migration` | 0.0 | |
| `opco_lfo1Rate` | 0.04 | Slow |
| `opco_lfo1Depth` | 0.08 | Slight vibrato — the ballad has breath |
| `opco_lfo1Shape` | 0 | Sine |
| `opco_lfo2Rate` | 0.015 | |
| `opco_lfo2Depth` | 0.04 | |
| `opco_macroCharacter` | 0.45 | |
| `opco_macroMovement` | 0.2 | |
| `opco_macroSpace` | 0.7 | More space — ballads need room |

**Why this works:** "Human Nature" is the canonical DX7 ballad sound — warm, emotional, slightly complex. The longer modulation decay (800ms) keeps the bell complexity present for longer, creating a sense of emotional arrival rather than mathematical resolution. Slight vibrato (LFO1 at 0.08 depth) adds the organic breathing that the otherwise mathematical DX7 sound needs in a ballad context.

---

### Preset 9: FM Wobble EP

**Mood:** Flux | **Discovery:** LFO modulating FM index — the index wobbles, the timbre fluctuates

| Parameter | Value | Why |
|-----------|-------|-----|
| `opco_algorithm` | 0.0 | Series |
| `opco_ratio` | 2.0 | |
| `opco_index` | 1.2 | Moderate base index |
| `opco_feedback` | 0.0 | |
| `opco_velToIndex` | 0.5 | |
| `opco_brightness` | 8500.0 | |
| `opco_attack` | 0.005 | |
| `opco_decay` | 1.5 | |
| `opco_sustain` | 0.55 | |
| `opco_release` | 0.7 | |
| `opco_modAttack` | 0.001 | |
| `opco_modDecay` | 0.6 | |
| `opco_modSustain` | 0.35 | |
| `opco_filterEnvAmt` | 0.4 | |
| `opco_migration` | 0.0 | |
| `opco_lfo1Rate` | 3.0 | LFO1 at audio-range-adjacent rate |
| `opco_lfo1Depth` | 0.0 | Targeting FM index, not pitch |
| `opco_lfo2Rate` | 0.5 | LFO2 at slow rate |
| `opco_lfo2Depth` | 0.35 | LFO modulates something |
| `opco_macroCharacter` | 0.45 | |
| `opco_macroMovement` | 0.7 | |
| `opco_macroSpace` | 0.45 | |

**Why this works:** LFO2 at 0.5 Hz with significant depth creates a 0.5 Hz modulation of FM index — the timbre wobbles slowly between more complex and less complex. The modulation index fluctuates between 1.2 (base) and higher values, creating periodic increases and decreases in sideband complexity. This is FM synthesis as movement: the spectrum itself fluctuates.

---

### Preset 10: Silicon to Tokyo

**Mood:** Entangled | **Discovery:** Maximum migration — the DX7 absorbing the Kitchen's character

| Parameter | Value | Why |
|-----------|-------|-----|
| `opco_algorithm` | 0.0 | Series |
| `opco_ratio` | 1.0 | |
| `opco_index` | 1.5 | |
| `opco_feedback` | 0.0 | |
| `opco_velToIndex` | 0.55 | |
| `opco_brightness` | 8000.0 | |
| `opco_attack` | 0.005 | |
| `opco_decay` | 1.5 | |
| `opco_sustain` | 0.5 | |
| `opco_release` | 0.7 | |
| `opco_modAttack` | 0.001 | |
| `opco_modDecay` | 0.4 | |
| `opco_modSustain` | 0.25 | |
| `opco_filterEnvAmt` | 0.35 | |
| `opco_migration` | 0.75 | High migration |
| `opco_lfo1Rate` | 0.05 | |
| `opco_lfo1Depth` | 0.05 | |
| `opco_macroCharacter` | 0.4 | |
| `opco_macroMovement` | 0.3 | |
| `opco_macroCoupling` | 0.7 | COUPLING elevated |
| `opco_macroSpace` | 0.55 | |

**Why this works:** Migration at 0.75 with COUPLING macro elevated opens the DX7 to Kitchen influence. The algorithm traveled from Stanford to Hamamatsu to Tokyo — and from there, through every kitchen it entered. The SpectralFingerprint from coupled Kitchen engines will shift the FM character toward the kitchen's spectral properties. The mathematical instrument absorbs the organic character of the food it encountered on the route.

---

## Phase R7: Scripture

### Verse I — The Bessel Function

*When one sine wave modulates another's frequency,*
*sidebands appear at predictable amplitudes.*
*The Bessel function predicts them exactly.*
*John Chowning found this in 1967.*
*He found it was music.*

### Verse II — The Index Envelope

*High modulation index: complex, bright, full of sidebands.*
*Falling index: the complexity decays.*
*Low sustained index: warmth, simplicity, arrival.*
*The DX EP sound is not a timbre. It is a trajectory.*
*From complexity toward simplicity, and simplicity feels like home.*

### Verse III — The Algorithm

*Series: one operator drives another.*
*Parallel: both operators contribute directly.*
*Feedback: the operator drives itself.*
*Thirty-two algorithms in the DX7.*
*Two operators tell you everything you need.*

### Verse IV — 1983

*Preset 11. E. Piano 1.*
*It was everywhere.*
*Every producer in Tokyo in 1985 knows this sound.*
*Every ballad on every record between 1983 and 1991*
*has this sound in it somewhere.*
*The algorithm traveled farther than any electric piano before it.*

---

## Phase R8: The Guru Bin Benediction

*"XOpcode scored 9.0/10 and the council was specific about why. It is not the FM synthesis that earns the score — FM synthesis has been implemented a thousand times. It is the `DXModulationEnvelope` that earns it. The council identified: this is the correct understanding of what the DX EP sound IS. Not a timbre. A trajectory. The high-index complexity at attack — full of Bessel-function sidebands, bright and mathematically exuberant — falling toward a low-index simplicity at sustain, where the sine waves dominate and the warmth arrives. This resolution, this sense of mathematical arrival, is why the DX EP is emotional. It starts complex and resolves to simple. The brain experiences this as arrival.*

*The velToIndex parameter is the correct implementation of the DX7's dynamic range. The original preset had velocity sensitivity on the FM index — playing hard made the attack brighter and more complex, playing softly made it quieter and simpler. The quadratic relationship between velocity and index (velocity² scaling) is the right approximation: the DX7's non-linear response to velocity made loud notes dramatically different from soft ones in both amplitude and timbre.*

*The three algorithm modes are the right primitives. The DX7 had 32 algorithms, but they were arrangements of the same three fundamental configurations: series (one operator driving another), parallel (multiple operators summed), and feedback (an operator driving itself). XOpcode captures all three with two operators. The restraint is correct.*

*The feedback algorithm needs a stability clamp. `feedbackState` must not grow without bound. `clamp(feedbackState, -4.0f, 4.0f)` before the feedback path — standard practice for FM feedback, exactly what the DX7 hardware did with its 6-bit quantization. When this is confirmed, the feedback mode becomes safe and usable. Until then: feedback at 0.0–0.2 only.*

*The sound traveled from Stanford in 1967 to Hamamatsu in 1983 to Tokyo by 1984 to every production studio in the world by 1986. It is the most widely heard synthesis algorithm in the history of recorded music. It is also, in its pure form, the most mathematical — Bessel functions, phase accumulation, exact sidebands. And yet it is the most emotional of the FUSION engines: the bell attack of a DX EP under someone's hands in a dark studio in 1987 is a sound that carries a decade's emotion.*

*The algorithm is the recipe. The emotion is the meal.*

*That is XOpcode: mathematics that became music, mathematics that became memory."*
