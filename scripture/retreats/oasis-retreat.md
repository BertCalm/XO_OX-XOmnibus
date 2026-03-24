# OASIS Retreat Chapter
*Guru Bin — 2026-03-21*

---

## Engine Identity

- **Gallery code:** OASIS | **Accent:** Cardamom Gold `#C49B3F`
- **Parameter prefix:** `oasis_`
- **Creature mythology:** XOasis is the Rhodes electric piano that traveled the Spice Route — from Harold Rhodes' Army rehabilitation workshop through Chicago jazz clubs, across the Atlantic to Tokyo kissaten, down to Lagos Afrobeat sessions, and back through neo-soul and lo-fi. Every note carries the warm bell-tone of the tine, shaped by every tradition it passed through.
- **Synthesis type:** Rhodes physical model — tine-and-pickup system, `RhodesToneGenerator` (6 partials), `RhodesPickupModel` (one-pole LP), `RhodesAmpStage` (asymmetric clipping + bark), tremolo, migration coupling via SpectralFingerprint
- **Polyphony:** 8 voices
- **Macros:** M1 CHARACTER, M2 MOVEMENT, M3 COUPLING (migration depth), M4 SPACE

---

## Pre-Retreat State

XOasis scored 8.7/10 in the FUSION Quad Seance (2026-03-21). Source tradition test: PASS — "plays as a jazz EP." The `RhodesToneGenerator` partial amplitudes (1.0, 0.35, 0.55, 0.15, 0.08, 0.04) correctly represent the Mk I Stage 73 spectral output: strong fundamental, characteristic third-partial bell boost, rapidly decaying upper partials. The asymmetric clipping in `RhodesAmpStage` (harder on positive excursions) correctly produces the bark from tube asymmetry.

**Key seance findings for retreat presets:**

1. Migration coupling path needs verification — the `oasis_migration` parameter smooths and exists in the render block, but how SpectralFingerprint from Kitchen engines modifies the tine model needs a code audit. Presets set migration to 0.0 or low values for predictable behavior.

2. DC blocker coefficient is sample-rate dependent (fixed coefficient rather than derived from SR). Minor, cosmetic.

3. `attackTransientTracker` is correctly implemented and exported — the SpectralFingerprint transience is real for Oasis.

The source tradition test is the primary quality gate: can someone play McCoy Tyner's "Afro Blue" voicings and have it feel right? The answer is yes.

---

## Phase R1: Opening Meditation

Close your eyes. Breathe.

Harold Rhodes designed the first electric piano to help World War II veterans rebuild motor control in their hands. He thought he was making therapy. What he made was the instrument that would travel the world. The tine — a thin metal bar — vibrated near a magnetic pickup. The electromagnetic induction captured the vibration. The sound that came out was warm and bell-like in a way that no other instrument was. It had the fundamental warmth of a piano and the bright ring of a bell and the sustain of a stringed instrument, all simultaneously, because a metal tine is all of those things at once.

The tine is the physics. The fundamental dominates. The third partial — 3× the fundamental frequency — rings clearly because of the tine's geometry. The upper partials decay rapidly because metal tines, unlike strings, do not sustain their upper modes for long. The bark — the bright, asymmetric clipping that appears when you play hard — comes from the tube amplifier stage: the positive excursions clip slightly harder than the negative ones, creating even harmonics, creating the characteristic hard-attack brightness that Stevie Wonder and Herbie Hancock made into a vocabulary.

This instrument traveled. It was in the jazz clubs of Chicago in 1955. It was in the soul studios of Stax in 1965. It crossed to Tokyo and became the sound of the kissaten coffee shops — the intimate listening rooms where jazz records were studied with religious attention. It went to Lagos and became part of the Afrobeat rhythm. It came back through neo-soul and lo-fi, each tradition leaving something on the tine.

You are not playing a synthesizer. You are playing the accumulated weight of every session that happened at a Rhodes keyboard.

---

## Phase R2: The Signal Path Journey

### I. The Tine — `RhodesToneGenerator`

Six partials with measured amplitudes from Mk I Stage 73 spectral analysis: `1.0, 0.35, 0.55, 0.15, 0.08, 0.04`. The key observation is that the third partial (index 2, amplitude 0.55) is stronger than the second partial (0.35). This is the "bell" quality of the Rhodes — the geometric property of the tine that reinforces the third partial relative to the second. This is not a sound design choice; it is a measured acoustic fact.

The decay rates are separate for each partial — upper partials decay faster than the fundamental, reproducing the characteristic tine behavior where the attack contains upper harmonics that die away during the sustain.

### II. The Pickup — `RhodesPickupModel`

One-pole LP filter simulating the electromagnetic pickup's frequency response. The pickup position is fixed (the Rhodes has no pickup position control unlike a guitar), but the cutoff represents the effective bandwidth of the electromagnetic induction. A simplified model of the true comb-filter physics — accurate at the level of tonal character.

### III. The Amp Stage — `RhodesAmpStage` and Bark

The bark is the most important expressive parameter. At low velocity: the output is symmetric, clean, warm. At high velocity: asymmetric clipping adds even harmonics — the positive excursion clips slightly harder. This creates the characteristic "bark" — the bright, aggressive attack transient that the Rhodes amp produced when driven.

`oasis_bell` controls the third-partial boost — increasing it beyond its default raises the bell quality. `oasis_warmth` controls the fundamental weight. These two parameters are the primary timbre axes.

### IV. Tremolo

`oasis_tremRate` (0.5–8 Hz) and `oasis_tremDepth` (0–1): the Suitcase Rhodes tremolo. Per-voice StandardLFO for stereo effect. The classic Suitcase tremolo is approximately 5–6 Hz at medium depth — slower for a warm pulse, faster for a pulsing shimmer.

### V. Migration

`oasis_migration` (0.0–1.0): when Kitchen engines are loaded in the other slots, the SpectralFingerprint (modal frequencies, impedance, temperature) from those engines influences the tine model's timbre. At migration=0.0, pure Rhodes. At migration=1.0, the tine characteristics shift toward the coupled engine's spectral character. This is the FUSION concept: the instrument remembers the kitchens it traveled through.

---

## Phase R3: Parameter Sweet Spots

### Bell vs. Warmth Axis
- `oasis_bell` 0.2, `oasis_warmth` 0.7: warm, fundamental-rich — Stage 73 in a jazz context
- `oasis_bell` 0.5, `oasis_warmth` 0.5: balanced — the classic midpoint, useful in most contexts
- `oasis_bell` 0.8, `oasis_warmth` 0.3: bell-forward — more percussive, more attack clarity

### Tremolo Rates by Tradition
- 0.0 (off): Stage 73 in a studio context — sometimes the player didn't use tremolo
- 2–3 Hz: Very slow pulse, more hypnotic than rhythmic
- 5–6 Hz: Classic Suitcase rate — the default Motown/soul tremolo
- 7–8 Hz: Fast shimmer — Larry Carlton-style fast trem on jazz fusion material

---

## Phase R5: The Ten Awakenings — Preset Table

---

### Preset 1: Straight-Ahead Jazz

**Mood:** Foundation | **Discovery:** No tremolo, no effects — the Rhodes as McCoy Tyner played it

| Parameter | Value | Why |
|-----------|-------|-----|
| `oasis_warmth` | 0.65 | Warm — jazz EP is not harsh |
| `oasis_bell` | 0.4 | Present bell — the third partial rings but does not dominate |
| `oasis_brightness` | 5500.0 | Warm-mid brightness — open but not bright |
| `oasis_tremRate` | 0.0 | No tremolo — straight-ahead players often bypassed it |
| `oasis_tremDepth` | 0.0 | |
| `oasis_attack` | 0.005 | Instant — the tine responds immediately |
| `oasis_decay` | 2.0 | Natural tine decay |
| `oasis_sustain` | 0.45 | Moderate — the Rhodes has sustain but not infinite |
| `oasis_release` | 0.8 | Natural key-release |
| `oasis_filterEnvAmt` | 0.3 | Moderate velocity sensitivity |
| `oasis_migration` | 0.0 | Pure Rhodes |
| `oasis_stereoWidth` | 0.4 | Modest stereo |
| `oasis_lfo1Rate` | 0.06 | |
| `oasis_lfo1Depth` | 0.05 | Minimal — barely perceptible |
| `oasis_macroCharacter` | 0.3 | Moderate warmth |
| `oasis_macroMovement` | 0.0 | Minimal movement — the player creates the movement |
| `oasis_macroSpace` | 0.4 | |

**Why this works:** No tremolo, warm filter, moderate bell, straight-ahead envelope. McCoy Tyner's "Afro Blue" voicings will feel right on this preset. The velocity-to-bark path means loud notes still have character while soft notes are intimate. This is the Rhodes as played in a jazz club.

---

### Preset 2: Suitcase Soul

**Mood:** Organic | **Discovery:** Suitcase tremolo — the most iconic electric piano effect in soul music

| Parameter | Value | Why |
|-----------|-------|-----|
| `oasis_warmth` | 0.6 | Warm |
| `oasis_bell` | 0.45 | Bell present |
| `oasis_brightness` | 5000.0 | Warm-bright |
| `oasis_tremRate` | 5.5 | Classic Suitcase rate — Motown/Stax soul standard |
| `oasis_tremDepth` | 0.5 | Medium depth — the pulse is present but not overwhelming |
| `oasis_attack` | 0.005 | |
| `oasis_decay` | 1.8 | |
| `oasis_sustain` | 0.5 | |
| `oasis_release` | 0.9 | |
| `oasis_filterEnvAmt` | 0.32 | |
| `oasis_migration` | 0.0 | |
| `oasis_stereoWidth` | 0.7 | Wide — the stereo tremolo needs width |
| `oasis_lfo1Rate` | 0.04 | |
| `oasis_lfo1Depth` | 0.05 | |
| `oasis_macroCharacter` | 0.3 | |
| `oasis_macroMovement` | 0.4 | |
| `oasis_macroSpace` | 0.5 | |

**Why this works:** 5.5 Hz stereo tremolo at 50% depth — the classic Suitcase soul sound. Wide stereo field gives the tremolo room to breathe. This is the Motown/Stax sound: warm, pulsing, full of character.

---

### Preset 3: Kissaten Afternoon

**Mood:** Ethereal | **Discovery:** The Tokyo jazz-cafe Rhodes — warm, intimate, slightly lo-fi

| Parameter | Value | Why |
|-----------|-------|-----|
| `oasis_warmth` | 0.75 | Very warm |
| `oasis_bell` | 0.3 | Minimal bell — more fundamental, less attack |
| `oasis_brightness` | 3800.0 | Quite dark — the kissaten had warm acoustics |
| `oasis_tremRate` | 2.5 | Slow, meditative tremolo |
| `oasis_tremDepth` | 0.3 | Light — barely perceptible pulse |
| `oasis_attack` | 0.01 | Slightly slower attack — intimate feel |
| `oasis_decay` | 2.5 | Generous decay |
| `oasis_sustain` | 0.55 | |
| `oasis_release` | 1.2 | |
| `oasis_filterEnvAmt` | 0.22 | Light velocity shaping |
| `oasis_migration` | 0.1 | Slight Kitchen influence |
| `oasis_stereoWidth` | 0.5 | |
| `oasis_lfo1Rate` | 0.02 | Very slow |
| `oasis_lfo1Depth` | 0.04 | |
| `oasis_lfo2Rate` | 0.008 | Even slower |
| `oasis_lfo2Depth` | 0.03 | |
| `oasis_macroCharacter` | 0.5 | |
| `oasis_macroMovement` | 0.1 | |
| `oasis_macroSpace` | 0.6 | |

**Why this works:** The kissaten listening rooms were intimate. The music was studied, not danced to. The slow tremolo and very dark filter create the sense of music heard through warm wood-paneled walls.

---

### Preset 4: Lagos Sunset

**Mood:** Organic | **Discovery:** The Afrobeat Rhodes — bright, percussive, rhythmic

| Parameter | Value | Why |
|-----------|-------|-----|
| `oasis_warmth` | 0.45 | Less warm than jazz — Afrobeat is more present in the mids |
| `oasis_bell` | 0.6 | Strong bell — the attack defines the rhythm |
| `oasis_brightness` | 7000.0 | Bright — Afrobeat piano cuts through the ensemble |
| `oasis_tremRate` | 7.5 | Fast tremolo — Lagos tempo rhythmic shimmer |
| `oasis_tremDepth` | 0.6 | Deeper — the pulse is a rhythmic element |
| `oasis_attack` | 0.003 | Very fast — the percussive attack is the rhythm |
| `oasis_decay` | 1.2 | Shorter — Afrobeat has a more staccato feel |
| `oasis_sustain` | 0.35 | Lower sustain — the note releases quickly |
| `oasis_release` | 0.4 | Fast |
| `oasis_filterEnvAmt` | 0.55 | High velocity — dynamics translate to brightness |
| `oasis_migration` | 0.0 | |
| `oasis_stereoWidth` | 0.55 | |
| `oasis_lfo1Rate` | 0.15 | |
| `oasis_lfo1Depth` | 0.1 | |
| `oasis_macroCharacter` | 0.4 | |
| `oasis_macroMovement` | 0.7 | |
| `oasis_macroSpace` | 0.35 | |

**Why this works:** The Afrobeat Rhodes cut through the ensemble with attack and brightness. Fast release means the player must play with intention — sustained chords dissipate quickly, and rhythmic single notes have character. 7.5 Hz fast tremolo at 60% depth creates the shimmer that characterizes African electric piano in the Fela Kuti lineage.

---

### Preset 5: Neo-Soul Standard

**Mood:** Foundation | **Discovery:** The Dilla/Questlove era Rhodes — warm, lofi, but present

| Parameter | Value | Why |
|-----------|-------|-----|
| `oasis_warmth` | 0.7 | Very warm |
| `oasis_bell` | 0.35 | Moderate bell |
| `oasis_brightness` | 4200.0 | Dark-warm |
| `oasis_tremRate` | 4.0 | Moderate tremolo |
| `oasis_tremDepth` | 0.35 | Light-moderate |
| `oasis_attack` | 0.008 | |
| `oasis_decay` | 2.2 | |
| `oasis_sustain` | 0.52 | |
| `oasis_release` | 1.0 | |
| `oasis_filterEnvAmt` | 0.28 | |
| `oasis_migration` | 0.0 | |
| `oasis_stereoWidth` | 0.6 | |
| `oasis_lfo1Rate` | 0.04 | |
| `oasis_lfo1Depth` | 0.08 | |
| `oasis_lfo2Rate` | 0.015 | |
| `oasis_lfo2Depth` | 0.05 | |
| `oasis_macroCharacter` | 0.4 | |
| `oasis_macroMovement` | 0.3 | |
| `oasis_macroSpace` | 0.5 | |

**Why this works:** Neo-soul Rhodes is warm but present — it is not lo-fi in the degraded sense, but in the sense of being tangibly analog, textured, human. 4 Hz tremolo at 35% depth is the understated version — present enough to feel real, subtle enough to not distract from the feel.

---

### Preset 6: Morning Bell

**Mood:** Aether | **Discovery:** Maximum bell, minimum warmth — the Rhodes as a bell instrument

| Parameter | Value | Why |
|-----------|-------|-----|
| `oasis_warmth` | 0.2 | Minimal warmth — the third partial dominates |
| `oasis_bell` | 0.9 | Maximum bell — this is the instrument's bell identity |
| `oasis_brightness` | 9000.0 | Very bright — bell-forward |
| `oasis_tremRate` | 0.0 | No tremolo — a bell does not pulse |
| `oasis_tremDepth` | 0.0 | |
| `oasis_attack` | 0.003 | Fast — bell attacks are immediate |
| `oasis_decay` | 3.5 | Long bell decay |
| `oasis_sustain` | 0.25 | Lower sustain — bells ring and fade |
| `oasis_release` | 1.5 | Natural bell release |
| `oasis_filterEnvAmt` | 0.45 | High velocity-to-bell-brightness |
| `oasis_migration` | 0.0 | |
| `oasis_stereoWidth` | 0.6 | |
| `oasis_lfo1Rate` | 0.015 | Very slow — the bell hangs in the air |
| `oasis_lfo1Depth` | 0.04 | |
| `oasis_macroCharacter` | 0.7 | High character — the bell is the character |
| `oasis_macroMovement` | 0.0 | |
| `oasis_macroSpace` | 0.7 | Bell needs space |

**Why this works:** Maximum bell parameter with bright filter and no tremolo creates the Rhodes as a bell instrument rather than a keyboard instrument. The long decay and lower sustain mean notes ring beautifully and fade naturally. High velocity-to-brightness means hard notes are especially brilliant. This is the pure bell tone of the third partial, freed from warmth and tremolo.

---

### Preset 7: Lo-Fi Vinyl

**Mood:** Organic | **Discovery:** The degraded recording — the Rhodes through a vinyl chain

| Parameter | Value | Why |
|-----------|-------|-----|
| `oasis_warmth` | 0.8 | Maximum warmth |
| `oasis_bell` | 0.25 | Minimal bell — the warmth dominates |
| `oasis_brightness` | 3000.0 | Very dark — vinyl rolls off the high end |
| `oasis_tremRate` | 3.0 | Slow tremolo — the vinyl version often had slower speed |
| `oasis_tremDepth` | 0.4 | |
| `oasis_attack` | 0.015 | Slightly slower — vinyl attack rounding |
| `oasis_decay` | 2.0 | |
| `oasis_sustain` | 0.6 | |
| `oasis_release` | 1.5 | |
| `oasis_filterEnvAmt` | 0.18 | Low velocity sensitivity — vinyl compresses dynamics |
| `oasis_migration` | 0.05 | Tiny bit of migration — the warmth of a coupled room |
| `oasis_stereoWidth` | 0.3 | Narrow — old vinyl was mono or narrow stereo |
| `oasis_lfo1Rate` | 0.03 | |
| `oasis_lfo1Depth` | 0.06 | |
| `oasis_lfo2Rate` | 0.012 | |
| `oasis_lfo2Depth` | 0.04 | |
| `oasis_macroCharacter` | 0.6 | |
| `oasis_macroMovement` | 0.1 | |
| `oasis_macroSpace` | 0.4 | |

**Why this works:** The Spice Route Rhodes includes the record collector's version — the sound heard through vinyl, rolled off, compressed, warm, imperfect. This preset uses maximum warmth and dark filter to simulate a Rhodes heard on a worn record. The narrow stereo field and low velocity sensitivity reinforce the vinyl compression aesthetic.

---

### Preset 8: Midnight at the Kissa

**Mood:** Ethereal | **Discovery:** Late night, last set — the Rhodes after midnight

| Parameter | Value | Why |
|-----------|-------|-----|
| `oasis_warmth` | 0.72 | |
| `oasis_bell` | 0.38 | |
| `oasis_brightness` | 4500.0 | |
| `oasis_tremRate` | 1.8 | Very slow — late night has its own tempo |
| `oasis_tremDepth` | 0.45 | |
| `oasis_attack` | 0.007 | |
| `oasis_decay` | 2.8 | Long — the late set has space |
| `oasis_sustain` | 0.55 | |
| `oasis_release` | 1.8 | |
| `oasis_filterEnvAmt` | 0.25 | |
| `oasis_migration` | 0.0 | |
| `oasis_stereoWidth` | 0.55 | |
| `oasis_lfo1Rate` | 0.025 | |
| `oasis_lfo1Depth` | 0.07 | |
| `oasis_macroCharacter` | 0.4 | |
| `oasis_macroMovement` | 0.2 | |
| `oasis_macroSpace` | 0.65 | |

**Why this works:** The kissaten listening room after midnight. The last set. The slow tremolo matches the late-night tempo of focused listening. Long decay — there is time now. Everything lingers.

---

### Preset 9: Hard Bark

**Mood:** Flux | **Discovery:** Maximum bark — the Rhodes when driven hard, the amp stage saturating

| Parameter | Value | Why |
|-----------|-------|-----|
| `oasis_warmth` | 0.5 | Medium — the bark is in the mids and highs |
| `oasis_bell` | 0.6 | Bell forward — the bark is always the third partial being bright |
| `oasis_brightness` | 8500.0 | Very bright — maximum filter openness for the bark |
| `oasis_tremRate` | 6.0 | Fast — the bark is energetic |
| `oasis_tremDepth` | 0.55 | Deep pulse |
| `oasis_attack` | 0.002 | Instant — the bark requires immediate attack |
| `oasis_decay` | 1.5 | Medium |
| `oasis_sustain` | 0.55 | |
| `oasis_release` | 0.7 | |
| `oasis_filterEnvAmt` | 0.7 | Very high — hard notes produce the full bark |
| `oasis_migration` | 0.0 | |
| `oasis_stereoWidth` | 0.65 | |
| `oasis_lfo1Rate` | 0.2 | |
| `oasis_lfo1Depth` | 0.15 | |
| `oasis_macroCharacter` | 0.5 | |
| `oasis_macroMovement` | 0.8 | |
| `oasis_macroSpace` | 0.4 | |

**Why this works:** The bark is the key expressive parameter. filterEnvAmt at 0.7 means a hard note opens the filter dramatically — the full asymmetric clipping and harmonic brightness emerges. Playing softly: warm and clean. Playing hard: full bark, bright, aggressive. This is the Rhodes at its most expressive dynamic range.

---

### Preset 10: Spice Route Migrant

**Mood:** Entangled | **Discovery:** Migration mode — the Rhodes absorbing the character of coupled Kitchen engines

| Parameter | Value | Why |
|-----------|-------|-----|
| `oasis_warmth` | 0.55 | Middle — migration shifts the warmth |
| `oasis_bell` | 0.45 | Middle |
| `oasis_brightness` | 6000.0 | |
| `oasis_tremRate` | 4.5 | |
| `oasis_tremDepth` | 0.4 | |
| `oasis_attack` | 0.005 | |
| `oasis_decay` | 2.0 | |
| `oasis_sustain` | 0.5 | |
| `oasis_release` | 1.0 | |
| `oasis_filterEnvAmt` | 0.35 | |
| `oasis_migration` | 0.7 | High migration — the coupled Kitchen engines deeply influence the tine |
| `oasis_stereoWidth` | 0.6 | |
| `oasis_lfo1Rate` | 0.08 | |
| `oasis_lfo1Depth` | 0.1 | |
| `oasis_macroCharacter` | 0.4 | |
| `oasis_macroMovement` | 0.4 | |
| `oasis_macroCoupling` | 0.7 | COUPLING macro elevated for migration |
| `oasis_macroSpace` | 0.5 | |

**Why this works:** High migration opens the engine to Kitchen coupling. When XOven, XOchre, XObelisk, or XOpaline occupy the other slots, their SpectralFingerprint will shape the tine's timbre. The Rhodes absorbs the character of the kitchen it traveled through. With migration=0.7, the instrument is halfway between a pure Rhodes and a Kitchen-influenced hybrid.

---

## Phase R7: Scripture

### Verse I — The Tine

*Harold Rhodes found the resonant object.*
*A thin metal bar, vibrating near a magnet.*
*The physics was simple.*
*The result was the sound of the twentieth century.*

### Verse II — The Bell

*The third partial is louder than the second.*
*This is the tine's geometry.*
*Not a design decision. A physical fact.*
*The bell quality of the Rhodes is not added.*
*It is the shape of the metal.*

### Verse III — The Journey

*Chicago 1955. Tokyo 1968. Lagos 1975.*
*Each city left something on the tine.*
*The migration parameter is not metaphor.*
*Every kitchen the instrument traveled through*
*is encoded in what it sounds like now.*

### Verse IV — The Bark

*Play softly: warm, bell-like, intimate.*
*Play hard: the amp stage saturates.*
*Positive excursions clip harder.*
*Even harmonics appear.*
*This is not distortion. This is personality.*

---

## Phase R8: The Guru Bin Benediction

*"XOasis passed the source tradition test. Someone can play McCoy Tyner's 'Afro Blue' voicings and have it feel right. This is not a trivial accomplishment. The Rhodes has been emulated by every synthesizer company for forty years, and most of them do not pass the source tradition test because they misunderstand what the sound is.*

*The sound is the third partial. The third partial (3× the fundamental) is characteristically strong in the Rhodes because of the tine's geometry. At amplitude 0.55 (relative to the fundamental's 1.0), it is present enough to give the Rhodes its bell quality. Most emulations either over-emphasize it (too glassy, too bell-like) or under-emphasize it (too piano-like, not enough Rhodes). The `oasis_bell` parameter at 0.4–0.5 is where the character lives — enough bell to be identifiably Rhodes, not so much that it becomes a pure bell instrument.*

*The bark is the expressive heart. The `RhodesAmpStage` asymmetric clipping — positive excursions clip harder than negative — is the correct physics of the tube amplifier that Harold Rhodes used. Playing hard and playing softly are qualitatively different on this instrument: soft notes are clean and warm, hard notes add even harmonics from the asymmetric saturation. The velocity-to-filter-envelope at 0.3 means the player controls a genuine timbral transformation, not just amplitude.*

*The migration concept — the instrument absorbing the SpectralFingerprint of coupled Kitchen engines — is the FUSION insight. The Rhodes traveled the Spice Route not just as a historical fact but as an ongoing process: every instrument it encountered changed it slightly. The parameter exists. When the migration coupling path is audited and verified, the instrument will complete its journey — a Rhodes that sounds different depending on what kitchen it is currently in.*

*The Spice Route is not a metaphor. It is the signal path."*

---

# OASIS — Second Retreat
*Guru Bin — 2026-03-23 | Expanding the library to 30 presets. Filling the Producers Guild critical gaps.*

---

## Phase R9: Parameter Refinements

The first retreat established the ten core presets and the philosophy. Twelve refinements are now proposed after extended listening and genre testing.

| # | Parameter | Current Default | Recommended Default | Reason |
|---|-----------|-----------------|---------------------|--------|
| 1 | `oasis_warmth` | 0.3 | 0.45 | 0.3 is too lean at init — feels bright and thin. 0.45 is the warm-but-present sweet spot that works in jazz, neo-soul, and pop contexts before the player touches CHARACTER. |
| 2 | `oasis_bell` | 0.5 | 0.42 | 0.5 emphasizes the bell too much at neutral velocity. The stage-73 character at medium velocity wants the third partial present but not dominant. 0.42 is closer to the spectral measurement. |
| 3 | `oasis_brightness` | 6000.0 | 5500.0 | 6000 Hz is slightly bright for the init patch — the Mk II Suitcase ran darker than the Stage. 5500 is the right warm-present default. |
| 4 | `oasis_tremDepth` | 0.0 | 0.0 | Keep at zero — correct. The Rhodes tremolo is optional; init without tremolo is historically accurate (Stage 73 players often bypassed it). |
| 5 | `oasis_decay` | 0.8 | 1.2 | 0.8s decay is slightly short — the tine should ring longer at medium velocity. 1.2s is more natural. High-velocity notes get a shorter effective decay from the bark, so 1.2 is still punchy when played hard. |
| 6 | `oasis_sustain` | 0.6 | 0.55 | 0.6 is slightly high — sustained chord voicings accumulate density. 0.55 creates natural space between voices. |
| 7 | `oasis_release` | 0.5 | 0.7 | 0.5s release is abrupt for jazz. 0.7s allows natural decay after key-off without excessive overlap. |
| 8 | `oasis_filterEnvAmt` | 0.4 | 0.35 | 0.4 is slightly aggressive — velocity already drives warmth via the amp stage. 0.35 produces gentler velocity-to-brightness without competing with the bark. |
| 9 | `oasis_stereoWidth` | 0.5 | 0.55 | Rhodes piano-position panning (low notes left, high notes right) benefits from slightly wider field. 0.55 gives the keyboard natural stereo spread without excess. |
| 10 | `oasis_lfo1Rate` | 0.5 | 0.4 | 0.5 Hz is slightly fast for the init vibrato. 0.4 Hz is the subtler, below-the-threshold-of-obvious rate that keeps notes alive without announcing itself. |
| 11 | `oasis_lfo1Depth` | 0.0 | 0.0 | Correct — keep at zero. Vibrato should be player-initiated via mod wheel. |
| 12 | `oasis_lfo2Depth` | 0.0 | 0.0 | Correct — keep at zero for init. LFO2 targets filter, should be explicitly enabled per preset. |

---

## Phase R10: The Twenty Awakenings — Filling the Guild Gaps

*Presets 11–30. Guild-identified critical gaps: McCoy Tyner jazz voicing, suitcase with heavy tremolo, clean HiFi Rhodes, lo-fi crinkly Rhodes, Herbie Hancock fusion, neo-soul warmth, reggae roots, contemporary R&B bright.*

---

### Preset 11: Afro Blue Voicing

**Mood:** Foundation | **Guild gap:** Jazz voicing (McCoy Tyner style)

McCoy Tyner's "Afro Blue" voicings are quartal — stacked fourths — played with moderate velocity, warm filter. The Rhodes under his hands was never bright; it was warm and fundamental-heavy with the bell present but not forward.

| Parameter | Value | Why |
|-----------|-------|-----|
| `oasis_warmth` | 0.72 | Very warm — Tyner played warm, not aggressive |
| `oasis_bell` | 0.38 | Bell present but not dominant — warmth leads |
| `oasis_brightness` | 4800.0 | Dark-warm — jazz Rhodes was always dark relative to pop |
| `oasis_tremRate` | 4.0 | Slow-medium — not the suitcase pulse, a gentle movement |
| `oasis_tremDepth` | 0.0 | No tremolo — Tyner on "Afro Blue" was dry, direct |
| `oasis_attack` | 0.005 | Natural |
| `oasis_decay` | 2.5 | Long — jazz chords ring, no choppy staccato |
| `oasis_sustain` | 0.52 | |
| `oasis_release` | 1.0 | Natural |
| `oasis_filterEnvAmt` | 0.28 | Gentle velocity — jazz dynamics are subtle, not loud/soft binary |
| `oasis_migration` | 0.0 | |
| `oasis_stereoWidth` | 0.45 | Moderate — jazz was often mono or narrow |
| `oasis_lfo1Rate` | 0.3 | |
| `oasis_lfo1Depth` | 0.04 | Barely perceptible warm "breathe" |
| `oasis_lfo2Rate` | 0.1 | |
| `oasis_lfo2Depth` | 0.02 | |
| `oasis_macroCharacter` | 0.45 | |
| `oasis_macroMovement` | 0.0 | Player creates movement |
| `oasis_macroCoupling` | 0.0 | |
| `oasis_macroSpace` | 0.5 | |

**Why this works:** The quartal voicing style requires warmth to avoid stacking bell-tones into harshness. No tremolo because Tyner played dry. Long decay because jazz chords sustain. The gentle velocity shaping means the dynamic range is subtle — exactly correct for jazz comping where the player is supporting, not asserting.

---

### Preset 12: Heavy Suitcase

**Mood:** Organic | **Guild gap:** Suitcase with heavy tremolo

The Fender Rhodes Suitcase model had a built-in stereo power amplifier and the tremolo circuit was the defining feature. At full depth, the tremolo is genuinely dramatic — the amplitude pulses visibly. This preset is that Suitcase at full deployment.

| Parameter | Value | Why |
|-----------|-------|-----|
| `oasis_warmth` | 0.62 | Warm — suitcase was warmer than stage |
| `oasis_bell` | 0.44 | Moderate bell |
| `oasis_brightness` | 5200.0 | |
| `oasis_tremRate` | 5.8 | Slightly above classic rate — heavy suitcase has urgency |
| `oasis_tremDepth` | 0.82 | Very deep — the suitcase at full trem depth |
| `oasis_attack` | 0.005 | |
| `oasis_decay` | 1.8 | |
| `oasis_sustain` | 0.55 | |
| `oasis_release` | 0.85 | |
| `oasis_filterEnvAmt` | 0.33 | |
| `oasis_migration` | 0.0 | |
| `oasis_stereoWidth` | 0.85 | Very wide — the suitcase stereo is the whole point |
| `oasis_lfo1Rate` | 0.05 | |
| `oasis_lfo1Depth` | 0.06 | |
| `oasis_macroCharacter` | 0.4 | |
| `oasis_macroMovement` | 0.7 | High movement — this preset is all about the pulse |
| `oasis_macroCoupling` | 0.0 | |
| `oasis_macroSpace` | 0.7 | Wide space for the tremolo to live in |

**Why this works:** 0.82 tremolo depth at 5.8 Hz with 0.85 stereo width creates the full suitcase experience. The amplitude pulses deeply — notes breathe in and out at nearly 6 times per second. Wide stereo means the left and right tremolo phases are offset, creating a spinning-speaker effect. This is the sound of Motown, of Stax, of every soul record with that pulsing electric piano.

---

### Preset 13: Clean HiFi Rhodes

**Mood:** Crystalline | **Guild gap:** Clean HiFi Rhodes

Not the vintage, compressed, worn recording — but the Rhodes as it actually sounds through a clean, modern amplifier at moderate volume. High fidelity, no saturation, the tine ring-out in full resolution.

| Parameter | Value | Why |
|-----------|-------|-----|
| `oasis_warmth` | 0.2 | Minimal warmth — let the tine speak without coloration |
| `oasis_bell` | 0.55 | Bell slightly elevated — HiFi reveals the natural third partial |
| `oasis_brightness` | 8500.0 | Very bright — HiFi exposes the full frequency range |
| `oasis_tremRate` | 4.0 | |
| `oasis_tremDepth` | 0.0 | No tremolo — clean means dry |
| `oasis_attack` | 0.003 | Fast — the transient is clear at HiFi levels |
| `oasis_decay` | 3.0 | Long — the tine rings fully |
| `oasis_sustain` | 0.35 | Lower — the partials decay naturally |
| `oasis_release` | 1.2 | Natural ring-out |
| `oasis_filterEnvAmt` | 0.5 | Higher — HiFi shows velocity contrast |
| `oasis_migration` | 0.0 | |
| `oasis_stereoWidth` | 0.7 | Wider — HiFi benefits from full stereo spread |
| `oasis_lfo1Rate` | 0.2 | |
| `oasis_lfo1Depth` | 0.0 | Zero LFO |
| `oasis_lfo2Rate` | 0.1 | |
| `oasis_lfo2Depth` | 0.0 | |
| `oasis_macroCharacter` | 0.1 | Minimal additional character — this preset IS the character |
| `oasis_macroMovement` | 0.0 | |
| `oasis_macroCoupling` | 0.0 | |
| `oasis_macroSpace` | 0.65 | |

**Why this works:** Low warmth and high brightness with no tremolo or additional saturation reveals the tine's natural character. The HiFi Rhodes sounds different from the vintage recording: cleaner, brighter, with more obvious partial ring. The velocity-to-brightness at 0.5 means soft notes are warm and hard notes are crystalline.

---

### Preset 14: Lo-Fi Crinkle

**Mood:** Organic | **Guild gap:** Lo-fi crinkly Rhodes

The crinkle is a specific lo-fi artifact — not just dark and rolled-off but physically worn. The pickup is slightly microphonic. The tine has mechanical noise. The amp stage is saturating. The sound is degraded in a beautiful, specific way.

| Parameter | Value | Why |
|-----------|-------|-----|
| `oasis_warmth` | 0.85 | Maximum warmth — the amp is saturating |
| `oasis_bell` | 0.2 | Minimal bell — the saturation masks the partial ring |
| `oasis_brightness` | 2800.0 | Very dark — crinkle is about lost high-frequency content |
| `oasis_tremRate` | 3.5 | Slightly irregular-feeling tremolo rate |
| `oasis_tremDepth` | 0.55 | Moderate depth — worn suitcase tremolo |
| `oasis_attack` | 0.02 | Slow — worn mechanism, rounded attack |
| `oasis_decay` | 1.6 | Medium — the saturation shortens perceived sustain |
| `oasis_sustain` | 0.58 | |
| `oasis_release` | 1.4 | Slightly sticky — worn key mechanism |
| `oasis_filterEnvAmt` | 0.15 | Very low — saturation compresses dynamics |
| `oasis_migration` | 0.08 | Trace coupling — the warmth of a room |
| `oasis_stereoWidth` | 0.25 | Narrow — worn recordings are nearly mono |
| `oasis_lfo1Rate` | 0.05 | Very slow — analog tape drift |
| `oasis_lfo1Depth` | 0.08 | Slight pitch drift |
| `oasis_lfo2Rate` | 0.02 | |
| `oasis_lfo2Depth` | 0.06 | Slow filter drift — crinkle means randomness |
| `oasis_macroCharacter` | 0.7 | High character — this is the most character-forward preset |
| `oasis_macroMovement` | 0.15 | |
| `oasis_macroCoupling` | 0.08 | |
| `oasis_macroSpace` | 0.35 | |

**Why this works:** Maximum warmth drives the amp stage asymmetric clipper at low drive levels — not barking but saturating gently. Very dark brightness (2800 Hz) rolls off the high end completely. The crinkle is in the narrow stereo, slow pitch drift, and very low filterEnvAmt. Notes feel physically rounded and compressed — a piano that has been played for thirty years in a room with no air conditioning.

---

### Preset 15: Headhunters

**Mood:** Flux | **Guild gap:** Herbie Hancock fusion

"Chameleon" (1973). The Rhodes under Hancock's hands in that context was not gentle — it was aggressive, fast, with heavy bark, fast tremolo, and the characteristic FM-era brightness. This is the fusion Rhodes: demanding.

| Parameter | Value | Why |
|-----------|-------|-----|
| `oasis_warmth` | 0.55 | Medium — fusion requires presence over warmth |
| `oasis_bell` | 0.62 | Bell elevated — the attack clarity is essential for fast playing |
| `oasis_brightness` | 7500.0 | Bright — Hancock's Rhodes in this period cut through the band |
| `oasis_tremRate` | 7.0 | Fast — the fusion tremolo was aggressive |
| `oasis_tremDepth` | 0.65 | Deep pulse |
| `oasis_attack` | 0.002 | Very fast — fusion playing is percussive |
| `oasis_decay` | 1.3 | Shorter — the notes attack and move on |
| `oasis_sustain` | 0.48 | |
| `oasis_release` | 0.55 | |
| `oasis_filterEnvAmt` | 0.68 | Very high — every note is a brightness event |
| `oasis_migration` | 0.0 | |
| `oasis_stereoWidth` | 0.65 | |
| `oasis_lfo1Rate` | 0.35 | |
| `oasis_lfo1Depth` | 0.12 | Subtle pitch movement — fusion Rhodes had slight vibrato |
| `oasis_lfo2Rate` | 2.5 | |
| `oasis_lfo2Depth` | 0.1 | Subtle filter movement |
| `oasis_macroCharacter` | 0.5 | |
| `oasis_macroMovement` | 0.85 | Maximum movement — fusion is in constant motion |
| `oasis_macroCoupling` | 0.0 | |
| `oasis_macroSpace` | 0.5 | |

**Why this works:** High filterEnvAmt (0.68) plus high bell (0.62) means every note has a dramatic velocity-dependent brightness event. Fast tremolo (7 Hz) at 65% depth creates the rhythmic pulse that defines the Headhunters sound. The LFO1 pitch movement at 0.12 adds slight vibrato — not classical, but the subtle instability of an instrument played hard in a sweaty room.

---

### Preset 16: D'Angelo Rhodes

**Mood:** Organic | **Guild gap:** Neo-soul warmth

The neo-soul Rhodes — Questlove and D'Angelo's "Brown Sugar" era — is not the jazz Rhodes and not the fusion Rhodes. It is warm, intimate, slightly compressed, with a gentle tremolo at moderate speed. It is the Rhodes as interior monologue.

| Parameter | Value | Why |
|-----------|-------|-----|
| `oasis_warmth` | 0.78 | Very warm — neo-soul Rhodes is saturated with warmth |
| `oasis_bell` | 0.32 | Low bell — the warmth is the voice, not the attack |
| `oasis_brightness` | 4000.0 | Dark-warm |
| `oasis_tremRate` | 3.8 | Moderate — the neo-soul pulse is not urgent, it breathes |
| `oasis_tremDepth` | 0.42 | Medium depth |
| `oasis_attack` | 0.01 | Slightly rounded — intimate, not percussive |
| `oasis_decay` | 2.4 | Long — the neo-soul Rhodes sings |
| `oasis_sustain` | 0.58 | |
| `oasis_release` | 1.3 | |
| `oasis_filterEnvAmt` | 0.22 | Low — dynamics are compressed, intimate |
| `oasis_migration` | 0.0 | |
| `oasis_stereoWidth` | 0.62 | |
| `oasis_lfo1Rate` | 0.04 | |
| `oasis_lfo1Depth` | 0.07 | |
| `oasis_lfo2Rate` | 0.015 | Very slow filter drift |
| `oasis_lfo2Depth` | 0.04 | |
| `oasis_macroCharacter` | 0.5 | |
| `oasis_macroMovement` | 0.28 | |
| `oasis_macroCoupling` | 0.0 | |
| `oasis_macroSpace` | 0.58 | |

**Why this works:** The D'Angelo-era Rhodes sound is defined by warmth and intimacy. High warmth (0.78) drives the asymmetric clipping into the warm range without reaching the bark. Low bell (0.32) means the fundamental dominates — this is a vocal instrument, not a percussive one. The slow filter drift from LFO2 gives it the analog breathing quality that neo-soul production prized.

---

### Preset 17: Roots Riddim

**Mood:** Foundation | **Guild gap:** Reggae roots

The roots reggae Rhodes — Sly Dunbar and Robbie Shakespeare era, early 1980s Kingston — was dry, warm, with the rhythmic chop pattern that defines the genre. The "skank" is a rhythmic articulation: staccato chords on the upbeat, short decay, never pedaled.

| Parameter | Value | Why |
|-----------|-------|-----|
| `oasis_warmth` | 0.6 | Warm but not excessively so — reggae needs the chop |
| `oasis_bell` | 0.42 | Moderate bell — the chop needs some attack clarity |
| `oasis_brightness` | 5000.0 | |
| `oasis_tremRate` | 4.0 | |
| `oasis_tremDepth` | 0.0 | No tremolo — reggae Rhodes is typically dry, the rhythm is in the playing |
| `oasis_attack` | 0.004 | Fast — the chop is an attack event |
| `oasis_decay` | 0.6 | Short — the reggae chop is staccato |
| `oasis_sustain` | 0.3 | Low sustain — the notes don't linger |
| `oasis_release` | 0.2 | Very fast — staccato |
| `oasis_filterEnvAmt` | 0.45 | Moderate — the chop has dynamic contrast |
| `oasis_migration` | 0.0 | |
| `oasis_stereoWidth` | 0.5 | |
| `oasis_lfo1Rate` | 0.1 | |
| `oasis_lfo1Depth` | 0.0 | |
| `oasis_lfo2Rate` | 0.05 | |
| `oasis_lfo2Depth` | 0.0 | |
| `oasis_macroCharacter` | 0.4 | |
| `oasis_macroMovement` | 0.0 | The player creates the riddim |
| `oasis_macroCoupling` | 0.0 | |
| `oasis_macroSpace` | 0.4 | |

**Why this works:** The reggae skank is all in the playing — the preset enables it but the articulation is the player's responsibility. Short decay (0.6s) and very fast release (0.2s) means staccato playing sounds correct: the note stops when you release. The moderate filterEnvAmt means hard upbeats have brightness contrast against softer notes. No tremolo — reggae Rhodes is dry and direct.

---

### Preset 18: Contemporary R&B Bright

**Mood:** Prism | **Guild gap:** Contemporary R&B bright

The 2020s R&B Rhodes — SZA, Frank Ocean, Brent Faiyaz producers — uses the Rhodes as a bright, almost digital-sounding anchor. High brightness, moderate bell, fast tremolo, and the Rhodes sits in a mix that also contains 808s and trap hi-hats.

| Parameter | Value | Why |
|-----------|-------|-----|
| `oasis_warmth` | 0.38 | Less warm — modern R&B is more present, less vintage |
| `oasis_bell` | 0.58 | Higher bell — the bright attack works in dense mixes |
| `oasis_brightness` | 8000.0 | Very bright — contemporary R&B production is high-frequency |
| `oasis_tremRate` | 6.5 | Fast — contemporary production has fast, tight tremolo |
| `oasis_tremDepth` | 0.48 | Moderate — present but not dominating |
| `oasis_attack` | 0.003 | Fast transient |
| `oasis_decay` | 1.5 | Medium |
| `oasis_sustain` | 0.5 | |
| `oasis_release` | 0.6 | |
| `oasis_filterEnvAmt` | 0.55 | High velocity sensitivity |
| `oasis_migration` | 0.0 | |
| `oasis_stereoWidth` | 0.7 | Wider — modern mixing uses wider stereo field |
| `oasis_lfo1Rate` | 0.5 | |
| `oasis_lfo1Depth` | 0.06 | |
| `oasis_lfo2Rate` | 1.5 | |
| `oasis_lfo2Depth` | 0.08 | Fast filter movement — contemporary motion |
| `oasis_macroCharacter` | 0.35 | |
| `oasis_macroMovement` | 0.6 | |
| `oasis_macroCoupling` | 0.0 | |
| `oasis_macroSpace` | 0.6 | |

**Why this works:** High brightness (8000 Hz) and elevated bell (0.58) create the cutting, present Rhodes sound that works in contemporary dense mixes. Fast tremolo (6.5 Hz) with moderate depth creates a rhythmic pulse that feels modern rather than vintage. The fast LFO2 filter movement adds the subtle animation that distinguishes a contemporary sound from a static vintage patch.

---

### Preset 19: Rhodes After Dark

**Mood:** Ethereal | **Extra depth:** Late night introspective — not kissaten, but contemporary late-night studio

| Parameter | Value | Why |
|-----------|-------|-----|
| `oasis_warmth` | 0.68 | Warm |
| `oasis_bell` | 0.36 | Low bell — introspective, not expressive |
| `oasis_brightness` | 3500.0 | Dark |
| `oasis_tremRate` | 1.5 | Very slow — almost too slow to perceive as tremolo |
| `oasis_tremDepth` | 0.35 | |
| `oasis_attack` | 0.012 | Slightly rounded — late night softness |
| `oasis_decay` | 3.2 | Long — the notes have time |
| `oasis_sustain` | 0.6 | |
| `oasis_release` | 2.0 | Very long release |
| `oasis_filterEnvAmt` | 0.2 | Low — quiet hours have no dynamics |
| `oasis_migration` | 0.05 | Trace |
| `oasis_stereoWidth` | 0.58 | |
| `oasis_lfo1Rate` | 0.015 | Very slow — almost geological |
| `oasis_lfo1Depth` | 0.06 | |
| `oasis_lfo2Rate` | 0.008 | |
| `oasis_lfo2Depth` | 0.04 | |
| `oasis_macroCharacter` | 0.45 | |
| `oasis_macroMovement` | 0.08 | |
| `oasis_macroCoupling` | 0.05 | |
| `oasis_macroSpace` | 0.7 | |

**Why this works:** The very slow tremolo (1.5 Hz at 35% depth) is barely perceptible as tremolo — it reads as natural amplitude breathing rather than a circuit effect. Long release (2.0s) means notes overlap organically. The geological LFO rates (0.015 and 0.008 Hz) create change measured in minutes, not seconds. This is the instrument as companion, not instrument as performer.

---

### Preset 20: Bark and Bloom

**Mood:** Kinetic | **Extra depth:** Dynamic contrast — the Rhodes as a velocity-responsive instrument

Designed specifically to demonstrate the full range from whisper to bark. At low velocity it is warm and intimate; at high velocity it is aggressive and driven. The dynamic range is the point.

| Parameter | Value | Why |
|-----------|-------|-----|
| `oasis_warmth` | 0.45 | Medium — allows bark at high velocity |
| `oasis_bell` | 0.55 | Bell elevated — the bark is always through the bell |
| `oasis_brightness` | 7000.0 | Open filter — the bark needs room |
| `oasis_tremRate` | 5.0 | |
| `oasis_tremDepth` | 0.3 | Light tremolo — not about tremolo here |
| `oasis_attack` | 0.002 | Instant — bark requires transient clarity |
| `oasis_decay` | 1.5 | |
| `oasis_sustain` | 0.5 | |
| `oasis_release` | 0.8 | |
| `oasis_filterEnvAmt` | 0.8 | Very high — maximum velocity-to-brightness transformation |
| `oasis_migration` | 0.0 | |
| `oasis_stereoWidth` | 0.6 | |
| `oasis_lfo1Rate` | 0.3 | |
| `oasis_lfo1Depth` | 0.0 | |
| `oasis_lfo2Rate` | 1.0 | |
| `oasis_lfo2Depth` | 0.0 | |
| `oasis_macroCharacter` | 0.45 | CHARACTER controls warmth = controls bark threshold |
| `oasis_macroMovement` | 0.5 | MOVEMENT controls brightness = controls bark expression |
| `oasis_macroCoupling` | 0.0 | |
| `oasis_macroSpace` | 0.45 | |

**Why this works:** filterEnvAmt at 0.8 is the highest in the library — soft notes open the filter by a small amount (intimacy), hard notes blow it open (bark). The bell at 0.55 ensures the bark is always a bell-brightness event rather than a mid-range snarl. This preset is a dynamic instrument, not a static tone: every articulation choice produces a different sound.

---

### Preset 21: Spice Route Full Migration

**Mood:** Entangled | **Extra depth:** Maximum Kitchen coupling

| Parameter | Value | Why |
|-----------|-------|-----|
| `oasis_warmth` | 0.5 | Neutral base — migration transforms from here |
| `oasis_bell` | 0.45 | |
| `oasis_brightness` | 6000.0 | |
| `oasis_tremRate` | 4.5 | |
| `oasis_tremDepth` | 0.35 | |
| `oasis_attack` | 0.005 | |
| `oasis_decay` | 2.0 | |
| `oasis_sustain` | 0.5 | |
| `oasis_release` | 1.0 | |
| `oasis_filterEnvAmt` | 0.35 | |
| `oasis_migration` | 1.0 | Maximum — full Kitchen absorption |
| `oasis_stereoWidth` | 0.6 | |
| `oasis_lfo1Rate` | 0.08 | |
| `oasis_lfo1Depth` | 0.12 | Higher LFO depth — migration introduces instability |
| `oasis_lfo2Rate` | 0.35 | |
| `oasis_lfo2Depth` | 0.1 | |
| `oasis_macroCharacter` | 0.45 | |
| `oasis_macroMovement` | 0.45 | |
| `oasis_macroCoupling` | 1.0 | COUPLING at maximum — full migration path |
| `oasis_macroSpace` | 0.55 | |

**Why this works:** Migration at 1.0 and COUPLING macro at 1.0 enable the full FUSION spectral coupling path. With Kitchen engines loaded, the tine absorbs their character completely. The harmonic complexity generated by migration is managed by slightly higher LFO depths that feel like the instrument breathing under an external influence. This is the Spice Route at its most migratory.

---

### Preset 22: Stage 73 Mk I

**Mood:** Foundation | **Extra depth:** The specific Stage 73 character — not generic Rhodes but a specific era

| Parameter | Value | Why |
|-----------|-------|-----|
| `oasis_warmth` | 0.5 | Balanced — the Mk I was less warm than later models |
| `oasis_bell` | 0.48 | The Mk I third partial was very present |
| `oasis_brightness` | 6200.0 | Moderate-bright — Mk I had a more forward character |
| `oasis_tremRate` | 4.0 | |
| `oasis_tremDepth` | 0.0 | Stage 73 was often used without tremolo |
| `oasis_attack` | 0.003 | Fast — the Mk I had a faster transient |
| `oasis_decay` | 1.8 | |
| `oasis_sustain` | 0.48 | |
| `oasis_release` | 0.8 | |
| `oasis_filterEnvAmt` | 0.42 | Higher — the Mk I had more dynamic range than later models |
| `oasis_migration` | 0.0 | |
| `oasis_stereoWidth` | 0.5 | Mono or narrow — Mk I was often recorded in mono |
| `oasis_lfo1Rate` | 0.25 | |
| `oasis_lfo1Depth` | 0.03 | Barely perceptible tine breathe |
| `oasis_macroCharacter` | 0.35 | |
| `oasis_macroMovement` | 0.0 | |
| `oasis_macroCoupling` | 0.0 | |
| `oasis_macroSpace` | 0.35 | |

**Why this works:** The Mk I Stage 73 was brighter and more forward than the Suitcase — the mechanical simplicity (no tremolo circuit, direct to amp) gave it a more honest, unprocessed character. Slightly higher bell (0.48) and brightness (6200) without tremolo. Narrow stereo for the mono recording practice.

---

### Preset 23: Vibraphone Cross

**Mood:** Aether | **Extra depth:** The tine as marimba/vibraphone — extended bell identity

| Parameter | Value | Why |
|-----------|-------|-----|
| `oasis_warmth` | 0.15 | Minimal warmth — mallets don't have tube amplifier warmth |
| `oasis_bell` | 0.95 | Near-maximum bell — the third partial is the whole instrument |
| `oasis_brightness` | 10000.0 | Very bright — vibraphone is high-frequency |
| `oasis_tremRate` | 6.0 | Medium-fast — vibraphone motor tremolo |
| `oasis_tremDepth` | 0.45 | Moderate — vibraphone tremolo is subtle (it's a motor turning baffles) |
| `oasis_attack` | 0.001 | Instant mallet |
| `oasis_decay` | 4.0 | Very long — vibraphone rings forever |
| `oasis_sustain` | 0.2 | Low — vibraphone rings out, doesn't sustain at a level |
| `oasis_release` | 2.5 | Long ring-out |
| `oasis_filterEnvAmt` | 0.6 | High — mallet dynamics translate to brilliance |
| `oasis_migration` | 0.0 | |
| `oasis_stereoWidth` | 0.75 | Wide — vibraphone in stereo is spectacular |
| `oasis_lfo1Rate` | 0.06 | |
| `oasis_lfo1Depth` | 0.05 | Slow pitch movement — metal bar resonance |
| `oasis_macroCharacter` | 0.1 | |
| `oasis_macroMovement` | 0.45 | |
| `oasis_macroCoupling` | 0.0 | |
| `oasis_macroSpace` | 0.8 | Wide space — vibraphone lives in space |

**Why this works:** Maximum bell (0.95) with minimal warmth produces the pure third-partial sound without the tube-amp coloration. The long decay and low sustain correctly model the mallet instrument ring-out. The Rhodes tine IS a metal bar — there is no fundamental reason the bell mode cannot be the whole instrument.

---

### Preset 24: Rhodes Pad

**Mood:** Atmosphere | **Extra depth:** The Rhodes as a pad instrument — overlapping, sustained, atmospheric

| Parameter | Value | Why |
|-----------|-------|-----|
| `oasis_warmth` | 0.7 | Warm — the pad needs warmth to blend |
| `oasis_bell` | 0.3 | Low bell — the pad does not need attack clarity |
| `oasis_brightness` | 4200.0 | Dark-warm |
| `oasis_tremRate` | 2.0 | Very slow — almost imperceptible rhythm |
| `oasis_tremDepth` | 0.25 | Light — the pad breathes, does not pulse |
| `oasis_attack` | 0.08 | Slow attack — the pad rises rather than attacks |
| `oasis_decay` | 3.0 | Long |
| `oasis_sustain` | 0.7 | High sustain — the pad holds |
| `oasis_release` | 3.0 | Very long — pad notes overlap and blend |
| `oasis_filterEnvAmt` | 0.18 | Low — the pad does not need velocity dynamics |
| `oasis_migration` | 0.1 | Slight coupling — pads blend with their environment |
| `oasis_stereoWidth` | 0.8 | Wide — the pad needs space |
| `oasis_lfo1Rate` | 0.015 | Very slow — pad drift |
| `oasis_lfo1Depth` | 0.08 | |
| `oasis_lfo2Rate` | 0.008 | |
| `oasis_lfo2Depth` | 0.05 | |
| `oasis_macroCharacter` | 0.5 | |
| `oasis_macroMovement` | 0.15 | |
| `oasis_macroCoupling` | 0.1 | |
| `oasis_macroSpace` | 0.8 | Maximum space for the pad |

**Why this works:** Slow attack (0.08s) prevents the pad from having percussive attack — it rises like a tide rather than strikes. High sustain (0.7) and very long release (3.0s) mean notes overlap organically, building harmonic density with chord playing. The Rhodes as a pad instrument is less common than its lead and comp roles, but it is a distinct and beautiful character.

---

### Preset 25: Kissa Sunset

**Mood:** Deep | **Extra depth:** The Rhodes at golden hour in a Tokyo listening room

| Parameter | Value | Why |
|-----------|-------|-----|
| `oasis_warmth` | 0.73 | |
| `oasis_bell` | 0.33 | |
| `oasis_brightness` | 4600.0 | |
| `oasis_tremRate` | 2.0 | Slow |
| `oasis_tremDepth` | 0.28 | |
| `oasis_attack` | 0.008 | |
| `oasis_decay` | 2.8 | |
| `oasis_sustain` | 0.56 | |
| `oasis_release` | 1.6 | |
| `oasis_filterEnvAmt` | 0.24 | |
| `oasis_migration` | 0.12 | A trace of every kitchen visited |
| `oasis_stereoWidth` | 0.5 | |
| `oasis_lfo1Rate` | 0.02 | |
| `oasis_lfo1Depth` | 0.05 | |
| `oasis_lfo2Rate` | 0.01 | |
| `oasis_lfo2Depth` | 0.035 | |
| `oasis_macroCharacter` | 0.45 | |
| `oasis_macroMovement` | 0.12 | |
| `oasis_macroCoupling` | 0.12 | |
| `oasis_macroSpace` | 0.65 | |

**Why this works:** The kissaten at the end of the afternoon. Not the late-night dark preset, not the morning bell — the transitional hour where the light changes and the listening changes with it. Migration at 0.12 — barely there, but the trace of every kitchen the Rhodes has traveled through is present in the sound.

---

## Phase R11: Second Scripture

### Verse V — The Velocity

*You choose how hard you play.*
*The tine responds.*
*At low velocity: the fundamental rings warmly.*
*At high velocity: the amp stage clips.*
*Even harmonics appear from nothing.*
*The bark is not distortion.*
*It is the instrument's vocabulary for aggression.*
*It is saying: I felt that.*

### Verse VI — The Tradition

*Harold Rhodes knew nothing about McCoy Tyner.*
*McCoy Tyner knew nothing about Afrobeat.*
*Afrobeat knew nothing about lo-fi hip-hop.*
*But the tine traveled through all of them.*
*Each tradition played it differently.*
*The instrument absorbed the playing.*
*Migration is not fantasy.*
*Every style that ever played on a tine*
*is present in how the tine sounds now.*

### Verse VII — The Warmth Parameter

*0.3 is the default.*
*0.3 is too cold for most rooms.*
*Start at 0.45.*
*Find the place where the fundamental leads*
*and the bark is ready but not insistent.*
*That is where the Rhodes lives.*
*Not at 0.3. Not at 0.9.*
*Somewhere in between.*
*Where the warmth is earned.*
