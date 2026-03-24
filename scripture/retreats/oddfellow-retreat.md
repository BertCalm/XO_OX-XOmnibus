# ODDFELLOW Retreat Chapter
*Guru Bin — 2026-03-21*

---

## Engine Identity

- **Gallery code:** ODDFELLOW | **Accent:** Neon Night `#FF6B35`
- **Parameter prefix:** `oddf_`
- **Creature mythology:** XOddfellow is the Wurlitzer electric piano that lives in the night market — open after the restaurants close, serving sounds that are urgent and imperfect and exactly right. The reed vibrates with the grit of a busker economy: slightly warbled, slightly driven, always characterful.
- **Synthesis type:** Wurlitzer physical model — `WurliReedModel` (5 inharmonic partials), `WurliPreamp` (cascaded dual-tanh saturation, always-on 1.5× minimum drive), built-in tremolo, migration coupling via SpectralFingerprint
- **Polyphony:** 8 voices
- **Macros:** M1 CHARACTER, M2 MOVEMENT, M3 COUPLING, M4 SPACE

---

## Pre-Retreat State

XOddfellow scored 8.5/10 in the FUSION Quad Seance (2026-03-21). Source tradition test: PASS — "the road-worn Wurli." The `WurliReedModel` inharmonic partial ratios (1.0, 2.01, 3.03, 4.08, 5.15) are exactly right for reed clamping imperfection. The always-on minimum drive of 1.5× in `WurliPreamp` ensures the Wurlitzer can never be clean — a core design commitment.

**Key seance findings for retreat presets:**

1. `attackTransience` not populated in SpectralFingerprint — 5th-slot coupling cannot detect hard Wurli hits. Minor for standalone use.

2. Filter brightness default 4000 Hz is "too open" per the council — more like clean Fender Rhodes than road-worn Wurli. Retreat presets will set default around 3000–3500 Hz for authentic character.

3. Warble is monophonic — all voices have identical warble phase (same warblePhase advancing at 4.5 Hz). Chords produce "unison warble" instead of per-reed variation. Minor, noted. Accept this in retreat presets.

4. The dual-tanh cascaded preamp (`fastTanh(driven * 0.8f) * 0.7f + fastTanh(driven * 1.5f) * 0.3f`) creates a more complex saturation than a single tanh — musically the right model for cascaded preamp stages.

---

## Phase R1: Opening Meditation

Close your eyes. Breathe.

The night market is open. The other restaurants have closed. This stand is serving food at 2am to people who are still awake and still hungry — for food, for music, for something that will not apologize for existing. The Wurlitzer is on the stage at the back of the market. It has been there for six years. The wheels on its amplifier case are worn. The tremolo circuit has a slight irregularity at 4.5 Hz. The preamp was always slightly driven — this was not a problem to fix; it was the sound.

The reed is the story. A flat steel reed — like a tuning fork — struck by a felt hammer. The reed produces a tone dominated by the fundamental and warm harmonics, with a slight natural inharmonicity from the clamping. The clamping is imperfect: where the reed is held to the frame, the vibration is slightly irregular. This irregularity is the source of the warble — the natural frequency modulation that gives the Wurlitzer its characteristic slightly-unstable, breathing quality. The inharmonic partials (2.01, 3.03, 4.08, 5.15 rather than clean 2, 3, 4, 5) are the consequence of this clamping imperfection.

The preamp adds grit because it was always slightly driven. The built-in tremolo circuit was the Wurlitzer's most recognizable feature after the reed tone itself — the 200A tremolo at approximately 5.5 Hz was the sound of 1960s soul. Ray Charles played it. Joe Zawinul used it on "Mercy Mercy Mercy." The Wurlitzer's grit belongs to the cramped stage, the slightly-too-loud PA, the busker economy that cannot afford to be clean.

The source tradition test asks: must it sound broken enough? The answer is yes. A clean Wurlitzer is not a Wurlitzer. The imperfection is the instrument.

---

## Phase R2: The Signal Path Journey

### I. The Reed — `WurliReedModel`

Five partials with inharmonic ratios derived from reed clamping physics: `1.0, 2.01, 3.03, 4.08, 5.15`. The slight sharp bias in upper partials (2.01 instead of 2.0, 5.15 instead of 5.0) is the measured consequence of imperfect clamping. Amplitudes: `1.0, 0.5, 0.2, 0.08, 0.03` — fundamental heavy, warm second harmonic, rapidly decaying upper partials.

`oddf_reed` (0–1): reed stiffness. At low values, the reed is soft — warmer, more fundamental-dominated, more inharmonic (wider ratios). At high values, the reed is stiff — more upper partials, tighter inharmonicity.

The warble (`warbleDepth = (1.0f - reedStiffness) * 0.003f` at 4.5 Hz) is inversely proportional to stiffness: a softer reed warbles more. At minimum stiffness, maximum warble (±0.3% frequency deviation ≈ ±5 cents). This is correct: a looser clamping produces more instability.

### II. The Preamp — `WurliPreamp`

The dual cascaded tanh: `fastTanh(driven * 0.8f) * 0.7f + fastTanh(driven * 1.5f) * 0.3f`. The two stages model the Wurlitzer's cascaded preamp sections — each stage has its own saturation characteristic, and the combination creates a more complex harmonic distortion profile than a single-stage saturation.

The minimum drive of 1.5× means the Wurlitzer can never be fully clean. Even at `oddf_drive = 0.0`, the output passes through at 1.5× before saturation. This is the most important design decision in the engine. It is the sound of the road-worn instrument.

`oddf_drive` (0–1) scales the drive from 1.5× (minimum, always-on) to a maximum drive that produces significant saturation on all amplitude levels. At high drive: the harmonics bloom, the warmth becomes grit, the character intensifies.

### III. Tremolo

`oddf_tremRate` (0–10 Hz) and `oddf_tremDepth` (0–1). The tremolo circuit of the Wurlitzer 200A was approximately 5.5 Hz at medium depth. The tremolo is one of the defining Wurlitzer characteristics — it is not an effect but a built-in circuit.

**Tremolo rates by context:**
- 0.0: No tremolo — dry, more unusual than the standard Wurli sound
- 2–3 Hz: Very slow pulse — more hypnotic, R&B ballad territory
- 5–5.5 Hz: Authentic 200A standard rate
- 7–8 Hz: Fast, urgent — Wurlitzer as percussion

### IV. Migration

Same architecture as XOasis — `oddf_migration` (0.0–1.0) enables SpectralFingerprint coupling from Kitchen engines. At 0.0: pure Wurlitzer. At higher values: the reed character absorbs the Kitchen engine's spectral character.

---

## Phase R5: The Ten Awakenings — Preset Table

---

### Preset 1: Night Market Standard

**Mood:** Foundation | **Discovery:** The Wurlitzer as it should sound — gritty, pulsing, alive

| Parameter | Value | Why |
|-----------|-------|-----|
| `oddf_reed` | 0.45 | Medium stiffness — balanced between soft warmth and hard clarity |
| `oddf_drive` | 0.35 | Moderate drive — the preamp is working but not saturating |
| `oddf_brightness` | 3200.0 | Dark-warm — corrected from seance finding (was 4000) |
| `oddf_tremRate` | 5.5 | Authentic 200A rate |
| `oddf_tremDepth` | 0.45 | Medium depth — the pulse is present and characterful |
| `oddf_attack` | 0.005 | Natural reed attack |
| `oddf_decay` | 1.2 | Medium decay |
| `oddf_sustain` | 0.55 | Moderate sustain |
| `oddf_release` | 0.5 | |
| `oddf_filterEnvAmt` | 0.38 | Moderate velocity shaping |
| `oddf_migration` | 0.0 | Pure Wurlitzer |
| `oddf_lfo1Rate` | 0.05 | |
| `oddf_lfo1Depth` | 0.05 | |
| `oddf_macroCharacter` | 0.35 | |
| `oddf_macroMovement` | 0.4 | |
| `oddf_macroSpace` | 0.4 | |

**Why this works:** 5.5 Hz tremolo at moderate depth, darker filter (3200 Hz), moderate drive — the Wurlitzer as Ray Charles played it. Not clean. Not broken. Exactly right.

---

### Preset 2: Street Soul

**Mood:** Organic | **Discovery:** Maximum grit — the busker's Wurlitzer, well-traveled

| Parameter | Value | Why |
|-----------|-------|-----|
| `oddf_reed` | 0.3 | Soft reed — more warble, more fundamental |
| `oddf_drive` | 0.75 | High drive — the preamp is saturating |
| `oddf_brightness` | 2800.0 | Very dark — the drive adds its own harmonics |
| `oddf_tremRate` | 6.5 | Slightly fast — the busker plays with urgency |
| `oddf_tremDepth` | 0.65 | Deep tremolo |
| `oddf_attack` | 0.006 | |
| `oddf_decay` | 1.0 | Short-medium |
| `oddf_sustain` | 0.5 | |
| `oddf_release` | 0.4 | Fast — street music has pace |
| `oddf_filterEnvAmt` | 0.5 | High velocity — hard notes are very bright |
| `oddf_migration` | 0.0 | |
| `oddf_lfo1Rate` | 0.1 | |
| `oddf_lfo1Depth` | 0.12 | |
| `oddf_macroCharacter` | 0.6 | |
| `oddf_macroMovement` | 0.6 | |
| `oddf_macroSpace` | 0.35 | |

**Why this works:** Maximum grit: soft reed (more warble), high drive (saturating preamp), dark filter, deep fast tremolo. This is the Wurlitzer that has been on the road. The imperfection is the identity.

---

### Preset 3: Soul Ballad

**Mood:** Atmosphere | **Discovery:** The slow ballad Wurlitzer — warm, restrained, intimate

| Parameter | Value | Why |
|-----------|-------|-----|
| `oddf_reed` | 0.6 | Stiffer reed — less warble, more controlled |
| `oddf_drive` | 0.2 | Minimal additional drive — just the always-on minimum |
| `oddf_brightness` | 3800.0 | Moderate dark |
| `oddf_tremRate` | 2.5 | Slow — ballad tempo |
| `oddf_tremDepth` | 0.3 | Light — the tremolo supports without asserting |
| `oddf_attack` | 0.01 | Slightly slower attack — intimate feel |
| `oddf_decay` | 1.8 | |
| `oddf_sustain` | 0.62 | |
| `oddf_release` | 1.0 | |
| `oddf_filterEnvAmt` | 0.25 | Gentle velocity |
| `oddf_migration` | 0.0 | |
| `oddf_lfo1Rate` | 0.03 | |
| `oddf_lfo1Depth` | 0.06 | |
| `oddf_lfo2Rate` | 0.012 | |
| `oddf_lfo2Depth` | 0.04 | |
| `oddf_macroCharacter` | 0.3 | |
| `oddf_macroMovement` | 0.15 | |
| `oddf_macroSpace` | 0.55 | |

**Why this works:** A stiffer reed means less warble — more controlled, less urgent. Slow tremolo at light depth. The minimum drive still ensures it is a Wurlitzer, but the restraint is the character. This is the instrument as accompaniment: present but not dominant.

---

### Preset 4: Reed Bark

**Mood:** Flux | **Discovery:** Hard velocity + high drive — the Wurlitzer at its most aggressive

| Parameter | Value | Why |
|-----------|-------|-----|
| `oddf_reed` | 0.55 | Medium stiffness |
| `oddf_drive` | 0.9 | Near-maximum drive — the preamp is saturating heavily |
| `oddf_brightness` | 5500.0 | Bright — the drive needs high filter to express its harmonics |
| `oddf_tremRate` | 8.0 | Very fast — urgent, percussive |
| `oddf_tremDepth` | 0.7 | Deep pulse |
| `oddf_attack` | 0.003 | Very fast |
| `oddf_decay` | 0.8 | Short |
| `oddf_sustain` | 0.45 | |
| `oddf_release` | 0.3 | Fast |
| `oddf_filterEnvAmt` | 0.65 | Very high velocity |
| `oddf_migration` | 0.0 | |
| `oddf_lfo1Rate` | 0.3 | |
| `oddf_lfo1Depth` | 0.2 | |
| `oddf_macroCharacter` | 0.55 | |
| `oddf_macroMovement` | 0.85 | |
| `oddf_macroSpace` | 0.35 | |

**Why this works:** Near-maximum drive with fast tremolo and high velocity sensitivity creates an aggressive, percussive Wurlitzer. The dual-tanh saturation at high drive produces complex harmonics that the bright filter allows through. Hard notes are urgent and driven. Soft notes are still characterful.

---

### Preset 5: Warble Study

**Mood:** Prism | **Discovery:** Maximum warble — the reed as an unstable system

| Parameter | Value | Why |
|-----------|-------|-----|
| `oddf_reed` | 0.05 | Near-minimum stiffness — maximum warble |
| `oddf_drive` | 0.4 | Moderate drive |
| `oddf_brightness` | 3500.0 | Dark — the warble is in the fundamental |
| `oddf_tremRate` | 4.5 | Moderate tremolo |
| `oddf_tremDepth` | 0.5 | |
| `oddf_attack` | 0.007 | |
| `oddf_decay` | 1.5 | |
| `oddf_sustain` | 0.58 | |
| `oddf_release` | 0.7 | |
| `oddf_filterEnvAmt` | 0.32 | |
| `oddf_migration` | 0.0 | |
| `oddf_lfo1Rate` | 0.04 | |
| `oddf_lfo1Depth` | 0.07 | |
| `oddf_macroCharacter` | 0.25 | |
| `oddf_macroMovement` | 0.35 | |
| `oddf_macroSpace` | 0.45 | |

**Why this works:** Near-minimum reed stiffness produces maximum warble depth (±0.3% = ±5 cents at 4.5 Hz). The warble is the instrument's primary timbral feature — the reed vibrates with visible instability. A single sustained note will audibly waver. Prism territory: the spectrum shifts with the warble's rhythm.

---

### Preset 6: Mercy Mercy

**Mood:** Foundation | **Discovery:** Joe Zawinul's "Mercy Mercy Mercy" — the definitive Wurlitzer moment

| Parameter | Value | Why |
|-----------|-------|-----|
| `oddf_reed` | 0.5 | Standard stiffness |
| `oddf_drive` | 0.3 | Moderate — the recording was clean by Wurlitzer standards |
| `oddf_brightness` | 3400.0 | Warm — classic soul recording character |
| `oddf_tremRate` | 5.0 | Near-standard rate |
| `oddf_tremDepth` | 0.42 | Medium — characteristic tremolo from the 1966 Capitol recording |
| `oddf_attack` | 0.005 | |
| `oddf_decay` | 1.4 | |
| `oddf_sustain` | 0.58 | |
| `oddf_release` | 0.6 | |
| `oddf_filterEnvAmt` | 0.34 | |
| `oddf_migration` | 0.0 | |
| `oddf_lfo1Rate` | 0.04 | |
| `oddf_lfo1Depth` | 0.05 | |
| `oddf_macroCharacter` | 0.35 | |
| `oddf_macroMovement` | 0.38 | |
| `oddf_macroSpace` | 0.45 | |

**Why this works:** The 1966 Capitol recording of "Mercy Mercy Mercy" is the Platonic Wurlitzer moment. The soul parameter set — moderate drive, standard tremolo rate, warm filter — aims for that specific character. Named for the recording because the Wurlitzer's mythology cannot be separated from it.

---

### Preset 7: Night Market Closing

**Mood:** Atmosphere | **Discovery:** The last hour of the night market — the energy dropping, the warmth remaining

| Parameter | Value | Why |
|-----------|-------|-----|
| `oddf_reed` | 0.4 | Slightly soft — tiredness makes things warmer |
| `oddf_drive` | 0.25 | Light — the energy is lower |
| `oddf_brightness` | 3000.0 | Dark — late hours are darker |
| `oddf_tremRate` | 3.0 | Slower — the market is closing |
| `oddf_tremDepth` | 0.38 | |
| `oddf_attack` | 0.012 | Slightly slower |
| `oddf_decay` | 2.0 | Longer — there is time now |
| `oddf_sustain` | 0.62 | |
| `oddf_release` | 1.2 | |
| `oddf_filterEnvAmt` | 0.22 | Light — the dynamics are compressed at closing time |
| `oddf_migration` | 0.0 | |
| `oddf_lfo1Rate` | 0.025 | Very slow |
| `oddf_lfo1Depth` | 0.06 | |
| `oddf_macroCharacter` | 0.35 | |
| `oddf_macroMovement` | 0.15 | |
| `oddf_macroSpace` | 0.55 | |

**Why this works:** The same instrument, the same player, two hours later. The energy is lower. The warmth remains. The night market is closing and the Wurlitzer is still playing.

---

### Preset 8: Lo-Fi Reed

**Mood:** Organic | **Discovery:** The degraded recording — the Wurlitzer through a cassette chain

| Parameter | Value | Why |
|-----------|-------|-----|
| `oddf_reed` | 0.35 | Soft reed — more character |
| `oddf_drive` | 0.5 | Medium drive |
| `oddf_brightness` | 2500.0 | Very dark — cassette high-frequency rolloff |
| `oddf_tremRate` | 5.5 | Standard |
| `oddf_tremDepth` | 0.5 | |
| `oddf_attack` | 0.015 | Slow — cassette tape rounding |
| `oddf_decay` | 1.3 | |
| `oddf_sustain` | 0.55 | |
| `oddf_release` | 0.8 | |
| `oddf_filterEnvAmt` | 0.2 | Low — cassette compresses dynamics |
| `oddf_migration` | 0.05 | Trace migration — warmth of the room |
| `oddf_lfo1Rate` | 0.03 | |
| `oddf_lfo1Depth` | 0.05 | |
| `oddf_macroCharacter` | 0.45 | |
| `oddf_macroMovement` | 0.3 | |
| `oddf_macroSpace` | 0.4 | |

**Why this works:** The Wurlitzer heard through a cassette recording. Very dark filter (2500 Hz), slow attack (tape rounding), low velocity sensitivity (cassette compression). The imperfection that the Wurlitzer already has is multiplied by the imperfection of the recording medium.

---

### Preset 9: Fast Tremolo Funk

**Mood:** Flux | **Discovery:** Fast tremolo as percussion — the Wurlitzer rhythmic element

| Parameter | Value | Why |
|-----------|-------|-----|
| `oddf_reed` | 0.55 | |
| `oddf_drive` | 0.55 | |
| `oddf_brightness` | 4500.0 | |
| `oddf_tremRate` | 9.0 | Near-maximum — the tremolo becomes a rhythmic texture |
| `oddf_tremDepth` | 0.75 | Very deep — the pulse is strong |
| `oddf_attack` | 0.003 | Very fast |
| `oddf_decay` | 0.9 | |
| `oddf_sustain` | 0.5 | |
| `oddf_release` | 0.35 | |
| `oddf_filterEnvAmt` | 0.55 | |
| `oddf_migration` | 0.0 | |
| `oddf_lfo1Rate` | 0.25 | |
| `oddf_lfo1Depth` | 0.15 | |
| `oddf_macroCharacter` | 0.5 | |
| `oddf_macroMovement` | 0.9 | |
| `oddf_macroSpace` | 0.4 | |

**Why this works:** At 9 Hz and 75% depth, the tremolo becomes a 9-per-second amplitude modulation — not quite a pitch effect, not quite a rhythmic effect, but a timbral percussive texture. The Wurlitzer becomes a rhythmic instrument.

---

### Preset 10: Spice Route Reed

**Mood:** Entangled | **Discovery:** Migration mode — the Wurlitzer absorbing Kitchen character

| Parameter | Value | Why |
|-----------|-------|-----|
| `oddf_reed` | 0.5 | |
| `oddf_drive` | 0.4 | |
| `oddf_brightness` | 3500.0 | |
| `oddf_tremRate` | 5.0 | |
| `oddf_tremDepth` | 0.42 | |
| `oddf_attack` | 0.006 | |
| `oddf_decay` | 1.5 | |
| `oddf_sustain` | 0.55 | |
| `oddf_release` | 0.7 | |
| `oddf_filterEnvAmt` | 0.35 | |
| `oddf_migration` | 0.65 | High migration |
| `oddf_lfo1Rate` | 0.06 | |
| `oddf_lfo1Depth` | 0.08 | |
| `oddf_macroCharacter` | 0.4 | |
| `oddf_macroMovement` | 0.35 | |
| `oddf_macroCoupling` | 0.65 | COUPLING elevated for migration |
| `oddf_macroSpace` | 0.45 | |

**Why this works:** The Wurlitzer on the Spice Route, absorbing kitchen character from coupled engines. The reed's inharmonic ratios will shift toward the Kitchen engine's spectral fingerprint at high migration. The night market instrument knows every kitchen it has played in.

---

## Phase R7: Scripture

### Verse I — The Reed

*The flat steel reed is clamped at one end.*
*The clamping is imperfect.*
*The imperfection produces inharmonic partials.*
*2.01, not 2. 5.15, not 5.*
*The Wurlitzer sounds like a Wurlitzer because the clamping was imperfect.*

### Verse II — The Preamp

*The minimum drive is 1.5 times.*
*The output is always saturated, slightly.*
*This is not a mistake. This is the design.*
*A clean Wurlitzer is not a Wurlitzer.*
*The grit is the identity.*

### Verse III — Night Market

*Open after the restaurants close.*
*Serving what the restaurants did not.*
*The instrument's character comes from the hours.*
*After midnight everything is more honest.*
*The grit, the warble, the urgency — these are night things.*

### Verse IV — The 4.5 Hz

*The warble is 4.5 cycles per second.*
*±5 cents of frequency deviation.*
*This is not vibrato. It is reed instability.*
*The reed is not controlled — it is tolerated.*
*The player does not modulate the pitch. The reed does.*

---

## Phase R8: The Guru Bin Benediction

*"XOddfellow passes the source tradition test because it understood the question. The question is not 'can it produce the right frequency content?' The question is: 'Must it sound broken enough?' The answer is yes, and the answer is built into the architecture.*

*The always-on 1.5× minimum drive is the most important design decision in the engine. Every Wurlitzer that was played on records was being slightly driven through its preamp. This was not accidental — it was the circuit being used in the range it was designed for. The dual-tanh cascade produces a more complex saturation curve than a single stage because the Wurlitzer's preamp had multiple cascaded stages. The math is in the signal path.*

*The warble from the inharmonic reed ratios deserves separate recognition. The ratios 1.0, 2.01, 3.03, 4.08, 5.15 are not stylistic choices — they are measurements. Reed clamping imperfection produces exactly this kind of mild inharmonicity, biased toward sharp upper partials. The warble depth inversely proportional to reed stiffness means that a softer reed (less clamping force) warbles more — physically correct. At minimum stiffness, the warble is ±5 cents at 4.5 Hz. This is audible as instability, as breath, as the instrument being alive in a way that a perfect instrument is not.*

*The night market is the right mythology. The Wurlitzer has always belonged to the venues that were improvised rather than designed: the cramped stage, the slightly-too-loud PA, the instrument that had been transported too many times and repaired too many times and was still somehow playing. The imperfection accumulated with the miles. The character deepened.*

*The road-worn Wurlitzer is not an inferior instrument. It is an instrument that has been lived in.*

*That is XOddfellow: an instrument that has been lived in."*

---

# ODDFELLOW — Second Retreat
*Guru Bin — 2026-03-23 | Expanding the library to 30 presets. Filling the Producers Guild critical gaps.*

---

## Phase R9: Parameter Refinements

| # | Parameter | Current Default | Recommended Default | Reason |
|---|-----------|-----------------|---------------------|--------|
| 1 | `oddf_reed` | 0.5 | 0.45 | 0.5 is slightly stiff — the Wurlitzer's character comes from its softness and inharmonicity. 0.45 produces slightly more warble and a more characterful reed. |
| 2 | `oddf_drive` | 0.3 | 0.35 | 0.3 is borderline clean-for-a-Wurlitzer. 0.35 ensures the minimum drive is clearly in the preamp saturation range — authentically imperfect. |
| 3 | `oddf_brightness` | 4000.0 | 3400.0 | 4000 Hz is too bright — confirmed seance finding. The authentic Wurlitzer recording character is darker. 3400 Hz is the correct warm-dark default. |
| 4 | `oddf_tremRate` | 5.5 | 5.5 | Keep — the 200A standard rate is correct. |
| 5 | `oddf_tremDepth` | 0.4 | 0.45 | 0.4 is slightly low for the classic tremolo character — 0.45 is the sweet spot where the pulse is clearly present without being overwhelming. |
| 6 | `oddf_decay` | 0.6 | 0.9 | 0.6s is short — the Wurlitzer actually has more sustain than this suggests. 0.9s is more representative. |
| 7 | `oddf_sustain` | 0.5 | 0.52 | Minor — 0.52 produces slightly fuller held chords. |
| 8 | `oddf_release` | 0.4 | 0.5 | 0.4s feels abrupt. 0.5s is more natural for the reed decay. |
| 9 | `oddf_filterEnvAmt` | 0.5 | 0.42 | 0.5 is slightly high — competing with the preamp saturation's own timbral effect. 0.42 balances velocity sensitivity against preamp character. |
| 10 | `oddf_lfo1Rate` | 0.5 | 0.4 | Same reasoning as XOasis — 0.4 Hz is the subtler rate below perceptual threshold. |
| 11 | `oddf_lfo1Depth` | 0.0 | 0.0 | Correct — no default vibrato. |
| 12 | `oddf_lfo2Depth` | 0.0 | 0.0 | Correct — no default LFO2. Wurlitzer's instability comes from the reed warble, not LFO-driven modulation. |

---

## Phase R10: The Twenty Awakenings — Filling the Guild Gaps

*Presets 11–30. Guild-identified gaps: ABBA ballad, Ray Charles block chords, vintage broadcast, roadhouse grit, late night diner, contemporary pop.*

---

### Preset 11: ABBA Slow Dance

**Mood:** Atmosphere | **Guild gap:** ABBA ballad

The ABBA Wurlitzer — "Knowing Me Knowing You," "The Winner Takes It All" — is the instrument at its most romantic. Moderate drive, slow tremolo, warm filter, slightly slower than the standard soul rate. It is the Wurlitzer saying: this matters.

| Parameter | Value | Why |
|-----------|-------|-----|
| `oddf_reed` | 0.55 | Slightly stiffer — more controlled, less warble, more composed |
| `oddf_drive` | 0.25 | Minimal drive — ABBA's production was clean by Wurlitzer standards |
| `oddf_brightness` | 4200.0 | Moderate-warm — Swedish pop was cleaner than American soul |
| `oddf_tremRate` | 3.5 | Slower than soul standard — ballad pace |
| `oddf_tremDepth` | 0.55 | Medium-deep — the tremolo is emotional, not rhythmic |
| `oddf_attack` | 0.008 | Slightly rounded — emotional rather than percussive |
| `oddf_decay` | 1.8 | Long — the ballad sustains |
| `oddf_sustain` | 0.65 | High — held chords are the language of the ballad |
| `oddf_release` | 1.2 | Natural |
| `oddf_filterEnvAmt` | 0.28 | Gentle velocity — pop ballad dynamics are subtle |
| `oddf_migration` | 0.0 | |
| `oddf_lfo1Rate` | 0.04 | |
| `oddf_lfo1Depth` | 0.07 | Very slight — emotional breath |
| `oddf_lfo2Rate` | 0.015 | |
| `oddf_lfo2Depth` | 0.04 | |
| `oddf_macroCharacter` | 0.3 | |
| `oddf_macroMovement` | 0.3 | |
| `oddf_macroSpace` | 0.65 | Generous space — ABBA used reverb generously |

**Why this works:** The slower tremolo rate (3.5 Hz vs standard 5.5) with deeper depth (0.55) creates an emotional, sighing pulse rather than a rhythmic one. Higher sustain (0.65) means chord voicings ring fully. The minimal additional drive keeps it from becoming gritty — the ABBA Wurlitzer had refinement.

---

### Preset 12: Ray Charles Block Chords

**Mood:** Foundation | **Guild gap:** Ray Charles block chord comping

Ray Charles played the Wurlitzer with a confident heaviness — block chords, wide voicings, the piano tradition recontextualized through the electric reed. The touch was strong, the tone warm and slightly driven. "What'd I Say" — that piano is a Wurlitzer at medium velocity with the tremolo doing the work.

| Parameter | Value | Why |
|-----------|-------|-----|
| `oddf_reed` | 0.5 | Standard — Ray Charles was a complete technician, not interested in extreme settings |
| `oddf_drive` | 0.45 | Medium-high — the "What'd I Say" Wurlitzer was definitely driven |
| `oddf_brightness` | 3600.0 | Warm-dark — Atlantic Records had a warm studio sound |
| `oddf_tremRate` | 5.0 | Near-standard |
| `oddf_tremDepth` | 0.5 | Full medium tremolo |
| `oddf_attack` | 0.005 | |
| `oddf_decay` | 1.5 | |
| `oddf_sustain` | 0.6 | Higher — block chords need sustain |
| `oddf_release` | 0.8 | |
| `oddf_filterEnvAmt` | 0.4 | Moderate — the dynamics of block chord playing |
| `oddf_migration` | 0.0 | |
| `oddf_lfo1Rate` | 0.06 | |
| `oddf_lfo1Depth` | 0.06 | Subtle — Ray Charles's style was confident, not ornamented |
| `oddf_macroCharacter` | 0.45 | |
| `oddf_macroMovement` | 0.5 | |
| `oddf_macroSpace` | 0.5 | |

**Why this works:** Medium-high drive (0.45) gives the driven character without excessive distortion. Standard tremolo settings (5.0 Hz / 50% depth) are the bread-and-butter Wurlitzer character. High sustain (0.6) means block chord voicings — widely spaced notes struck simultaneously — ring with their full tonal weight. The character is confident and declarative.

---

### Preset 13: Vintage Broadcast

**Mood:** Deep | **Guild gap:** Vintage broadcast — the Wurlitzer on a 1960s television sound stage

The vintage broadcast Wurlitzer — heard through a mono television speaker or AM radio — is a completely different animal from the studio recording version. Narrow stereo, dark filter, minimal dynamics (the limiter is always on), and the tube character of the broadcast chain.

| Parameter | Value | Why |
|-----------|-------|-----|
| `oddf_reed` | 0.4 | Slightly soft — the broadcast chain compresses everything anyway |
| `oddf_drive` | 0.55 | Medium-high — the broadcast limiter pushes the preamp harder |
| `oddf_brightness` | 2400.0 | Very dark — AM radio rolled off above 5kHz; this is already dark |
| `oddf_tremRate` | 5.5 | Standard |
| `oddf_tremDepth` | 0.35 | Lower — broadcast limiting compressed the tremolo |
| `oddf_attack` | 0.02 | Slow — tape and broadcast had slow transients |
| `oddf_decay` | 1.2 | |
| `oddf_sustain` | 0.55 | |
| `oddf_release` | 0.7 | |
| `oddf_filterEnvAmt` | 0.12 | Very low — broadcast compression kills dynamics |
| `oddf_migration` | 0.0 | |
| `oddf_lfo1Rate` | 0.03 | |
| `oddf_lfo1Depth` | 0.04 | |
| `oddf_macroCharacter` | 0.5 | |
| `oddf_macroMovement` | 0.2 | |
| `oddf_macroSpace` | 0.2 | Very narrow — mono broadcast |

**Why this works:** The very dark filter (2400 Hz) represents the AM radio bandwidth limitation. Very low velocity sensitivity (0.12) represents broadcast compression — everything sounds the same volume. Narrow stereo (0.2) is the mono speaker character. The result is the Wurlitzer as heard through technology — imperfect, warm, immediately recognizable from a distant era.

---

### Preset 14: Roadhouse Grit

**Mood:** Kinetic | **Guild gap:** Roadhouse grit — Steppenwolf / ZZ Top territory

The Wurlitzer in a roadhouse context — not soul or R&B but rock, where the instrument was driven hard, the tremolo was fast, and the grit was the whole point. This is the Wurlitzer as a guitar player would play it: with no deference.

| Parameter | Value | Why |
|-----------|-------|-----|
| `oddf_reed` | 0.35 | Soft reed — grit comes from warble + drive combination |
| `oddf_drive` | 0.82 | Very high — roadhouse means driven hard |
| `oddf_brightness` | 5500.0 | Bright — rock requires presence to cut through |
| `oddf_tremRate` | 8.5 | Very fast — aggressive rock tremolo |
| `oddf_tremDepth` | 0.72 | Very deep — the tremolo is a weapon |
| `oddf_attack` | 0.003 | Fast |
| `oddf_decay` | 0.9 | Short — roadhouse doesn't sustain, it attacks |
| `oddf_sustain` | 0.45 | |
| `oddf_release` | 0.4 | Fast |
| `oddf_filterEnvAmt` | 0.62 | Very high velocity sensitivity |
| `oddf_migration` | 0.0 | |
| `oddf_lfo1Rate` | 0.4 | |
| `oddf_lfo1Depth` | 0.18 | Aggressive pitch movement |
| `oddf_lfo2Rate` | 3.5 | Fast drive modulation |
| `oddf_lfo2Depth` | 0.25 | |
| `oddf_macroCharacter` | 0.65 | High character |
| `oddf_macroMovement` | 0.95 | Maximum movement |
| `oddf_macroSpace` | 0.45 | |

**Why this works:** High drive (0.82) at the low reed stiffness (0.35) creates a combination of warble instability and preamp saturation — the Wurlitzer as a distorted instrument. Very fast, deep tremolo (8.5 Hz / 72%) creates a machine-like intensity. The LFO2 drive modulation at 3.5 Hz creates rhythmic grit variation. This is for producers who want the Wurlitzer to be aggressive, not polite.

---

### Preset 15: Late Night Diner

**Mood:** Organic | **Guild gap:** Late night diner — the Wurlitzer in the corner booth

The late night diner Wurlitzer — the jukebox that's been playing for fifteen years, the one with the slightly warped tremolo speed, the note that buzzes on the left-hand octave. Warm, imperfect, familiar.

| Parameter | Value | Why |
|-----------|-------|-----|
| `oddf_reed` | 0.32 | Soft — worn reed |
| `oddf_drive` | 0.38 | Moderate — the jukebox preamp is not new |
| `oddf_brightness` | 3000.0 | Dark |
| `oddf_tremRate` | 4.8 | Slightly off the standard — the tremolo pot has drifted |
| `oddf_tremDepth` | 0.52 | |
| `oddf_attack` | 0.012 | Slightly slow — worn mechanism |
| `oddf_decay` | 1.4 | |
| `oddf_sustain` | 0.58 | |
| `oddf_release` | 0.9 | |
| `oddf_filterEnvAmt` | 0.2 | Low — the diner doesn't ask for dynamics |
| `oddf_migration` | 0.06 | The warmth of a room full of people |
| `oddf_lfo1Rate` | 0.07 | |
| `oddf_lfo1Depth` | 0.1 | Subtle instability — a worn instrument |
| `oddf_lfo2Rate` | 0.04 | Slow filter drift |
| `oddf_lfo2Depth` | 0.06 | |
| `oddf_macroCharacter` | 0.5 | |
| `oddf_macroMovement` | 0.25 | |
| `oddf_macroSpace` | 0.45 | |

**Why this works:** The soft reed (0.32) with slight instability (LFO1 at 0.1 depth) recreates the worn instrument character — notes are slightly unsteady, which is correct for an instrument that has been played daily for years. Slightly off tremolo rate (4.8 Hz instead of 5.5) is intentional — the drift is the character. The result is a Wurlitzer that feels like a place rather than a studio session.

---

### Preset 16: Contemporary Pop Wurlitzer

**Mood:** Luminous | **Guild gap:** Contemporary pop — Taylor Swift / Olivia Rodrigo era Wurlitzer

The contemporary pop Wurlitzer is cleaner than any vintage Wurlitzer but unmistakably a reed instrument — present, warm, with a clean tremolo. It sits in a mix that also has 808s and sampled drums and still reads as analog.

| Parameter | Value | Why |
|-----------|-------|-----|
| `oddf_reed` | 0.65 | Stiffer — contemporary recordings benefit from tighter character |
| `oddf_drive` | 0.22 | Low — contemporary production tends toward cleaner (but the minimum drive still adds character) |
| `oddf_brightness` | 5000.0 | Present-warm — sits in a modern mix |
| `oddf_tremRate` | 5.0 | Standard |
| `oddf_tremDepth` | 0.42 | Moderate |
| `oddf_attack` | 0.005 | |
| `oddf_decay` | 1.5 | |
| `oddf_sustain` | 0.55 | |
| `oddf_release` | 0.7 | |
| `oddf_filterEnvAmt` | 0.45 | Good velocity response |
| `oddf_migration` | 0.0 | |
| `oddf_lfo1Rate` | 0.15 | |
| `oddf_lfo1Depth` | 0.05 | Subtle — contemporary doesn't want too much warble |
| `oddf_lfo2Rate` | 0.5 | |
| `oddf_lfo2Depth` | 0.06 | |
| `oddf_macroCharacter` | 0.3 | |
| `oddf_macroMovement` | 0.45 | |
| `oddf_macroSpace` | 0.6 | |

**Why this works:** Higher reed stiffness (0.65) means less warble — contemporary production is less forgiving of pitch instability. Lower drive (0.22) keeps it from getting gritty, but the always-on minimum drive ensures it's still clearly a Wurlitzer and not a piano. The brightness at 5000 Hz sits in the mix without fighting other elements.

---

### Preset 17: Zawinul Electric

**Mood:** Prism | **Extra depth:** Joe Zawinul's Weather Report era

Zawinul in Weather Report was using the Wurlitzer in a way that was neither jazz nor soul — it was a timbral voice in an ensemble of timbral voices, where the reed warble was a compositional element.

| Parameter | Value | Why |
|-----------|-------|-----|
| `oddf_reed` | 0.38 | Soft — warble is a compositional element here |
| `oddf_drive` | 0.48 | Moderate-high |
| `oddf_brightness` | 4200.0 | |
| `oddf_tremRate` | 6.0 | Fast |
| `oddf_tremDepth` | 0.58 | Deep |
| `oddf_attack` | 0.004 | |
| `oddf_decay` | 1.3 | |
| `oddf_sustain` | 0.52 | |
| `oddf_release` | 0.6 | |
| `oddf_filterEnvAmt` | 0.48 | |
| `oddf_migration` | 0.15 | Weather Report was about instruments responding to each other |
| `oddf_lfo1Rate` | 0.2 | |
| `oddf_lfo1Depth` | 0.12 | More LFO — jazz fusion uses vibrato |
| `oddf_lfo2Rate` | 1.2 | |
| `oddf_lfo2Depth` | 0.1 | Drive modulation for timbral movement |
| `oddf_macroCharacter` | 0.48 | |
| `oddf_macroMovement` | 0.65 | |
| `oddf_macroCoupling` | 0.15 | |
| `oddf_macroSpace` | 0.55 | |

**Why this works:** The Weather Report Wurlitzer was aggressive and mobile — the tremolo fast, the drive pushing, the reed contributing instability as a texture. Migration at 0.15 acknowledges that Weather Report was about ensemble chemistry — the Wurlitzer responded to the bass clarinet and synthesizer textures around it.

---

### Preset 18: Reed Without Tremolo

**Mood:** Foundation | **Extra depth:** The Wurlitzer's reed character without the circuit effect

The tremolo is so associated with the Wurlitzer that its absence is striking — and reveals the reed's character unmediated. Warm, buzzy, slightly inharmonic, surprisingly present.

| Parameter | Value | Why |
|-----------|-------|-----|
| `oddf_reed` | 0.5 | Standard |
| `oddf_drive` | 0.4 | Medium |
| `oddf_brightness` | 3800.0 | |
| `oddf_tremRate` | 5.5 | |
| `oddf_tremDepth` | 0.0 | No tremolo — the reed speaks for itself |
| `oddf_attack` | 0.006 | |
| `oddf_decay` | 1.6 | |
| `oddf_sustain` | 0.6 | |
| `oddf_release` | 0.85 | |
| `oddf_filterEnvAmt` | 0.38 | |
| `oddf_migration` | 0.0 | |
| `oddf_lfo1Rate` | 0.03 | Very slow — reed natural drift |
| `oddf_lfo1Depth` | 0.06 | |
| `oddf_lfo2Rate` | 0.015 | |
| `oddf_lfo2Depth` | 0.04 | |
| `oddf_macroCharacter` | 0.4 | |
| `oddf_macroMovement` | 0.0 | No imposed movement — the reed IS the movement |
| `oddf_macroSpace` | 0.5 | |

**Why this works:** No tremolo reveals that the Wurlitzer's reed warble (from the inharmonic ratios and the 4.5 Hz natural frequency modulation) provides its own internal animation. The slow LFO1 at very low depth adds the analog drift that a completely static patch would lack, without imposing a circuit rhythm. This is the Wurlitzer as a reed instrument, not a circuit effect.

---

### Preset 19: Broadcast Soul Revival

**Mood:** Luminous | **Extra depth:** Neo-soul recreating the vintage broadcast character

| Parameter | Value | Why |
|-----------|-------|-----|
| `oddf_reed` | 0.42 | |
| `oddf_drive` | 0.52 | |
| `oddf_brightness` | 3200.0 | Dark |
| `oddf_tremRate` | 4.5 | Slightly slow — the revival era slows things down |
| `oddf_tremDepth` | 0.48 | |
| `oddf_attack` | 0.01 | |
| `oddf_decay` | 1.7 | |
| `oddf_sustain` | 0.58 | |
| `oddf_release` | 1.0 | |
| `oddf_filterEnvAmt` | 0.32 | |
| `oddf_migration` | 0.0 | |
| `oddf_lfo1Rate` | 0.04 | |
| `oddf_lfo1Depth` | 0.07 | |
| `oddf_macroCharacter` | 0.48 | |
| `oddf_macroMovement` | 0.3 | |
| `oddf_macroSpace` | 0.5 | |

**Why this works:** The soul revival approach — a dark filter and slightly slower tremolo — creates the sense of referring to the vintage broadcast character rather than copying it. This is how neo-soul uses the Wurlitzer: as a historical citation, not a historical re-enactment.

---

### Preset 20: Maximum Warble

**Mood:** Flux | **Extra depth:** The reed fully released — maximum inharmonic instability

| Parameter | Value | Why |
|-----------|-------|-----|
| `oddf_reed` | 0.0 | Minimum stiffness — maximum inharmonicity and warble |
| `oddf_drive` | 0.45 | |
| `oddf_brightness` | 3500.0 | |
| `oddf_tremRate` | 5.0 | |
| `oddf_tremDepth` | 0.5 | |
| `oddf_attack` | 0.006 | |
| `oddf_decay` | 1.4 | |
| `oddf_sustain` | 0.55 | |
| `oddf_release` | 0.7 | |
| `oddf_filterEnvAmt` | 0.38 | |
| `oddf_migration` | 0.0 | |
| `oddf_lfo1Rate` | 0.05 | |
| `oddf_lfo1Depth` | 0.08 | LFO adds to the reed's own instability |
| `oddf_lfo2Rate` | 0.03 | |
| `oddf_lfo2Depth` | 0.05 | |
| `oddf_macroCharacter` | 0.25 | Let the instability lead |
| `oddf_macroMovement` | 0.4 | |
| `oddf_macroSpace` | 0.5 | |

**Why this works:** Reed stiffness at minimum (0.0) means warble depth is at its maximum (±5 cents at 4.5 Hz). The inharmonic partials are at their maximum deviation — 5.15 instead of 5.0 for the 5th partial. The instrument sounds genuinely unstable, breathing with its own character. For producers who want the Wurlitzer's imperfection as a texture, this is the extreme case.

---

## Phase R11: Second Scripture

### Verse V — The Imperfection

*The clamping was imperfect.*
*The ratios are 2.01, not 2.*
*This is not a rounding error.*
*This is the reed's nature.*
*The Wurlitzer sounds like itself*
*because the engineering was approximate.*
*In sound, approximation is character.*

### Verse VI — The Preamp

*Every preamp has a minimum.*
*The Wurlitzer's minimum is 1.5 times.*
*At rest, it is already driven.*
*There is no clean Wurlitzer.*
*There is only the degree of grit.*
*The minimum is 1.5.*
*The starting point is already character.*

### Verse VII — The Night Market Never Closes

*Somewhere right now it is after midnight.*
*The restaurants are closed.*
*The stand in the corner of the market is open.*
*The Wurlitzer is playing.*
*The imperfection is on full display.*
*The warble is audible.*
*The preamp is driven.*
*The tremolo is running.*
*This is the instrument as itself:*
*not managed, not cleaned,*
*but expressed.*
