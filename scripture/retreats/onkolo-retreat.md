# ONKOLO Retreat Chapter
*Guru Bin — 2026-03-21*

---

## Engine Identity

- **Gallery code:** ONKOLO | **Accent:** Kente Gold `#FFB300`
- **Parameter prefix:** `onko_`
- **Creature mythology:** XOnkolo is the Clavinet that remembers the thumb piano. Named for nkolo — a Central African ancestor of the kalimba — because the lineage is the instrument. The rubber pad strikes the string, the magnetic pickup captures the percussive slap, and the whole thing is so funky it makes you move involuntarily.
- **Synthesis type:** Clavinet physical model — `ClaviStringModel` (8 harmonics, odd-harmonic emphasis), pickup position comb filter, key-off clunk (LCG noise burst), `AutoWahEnvelope` (2ms attack, 100ms release, 400–6000 Hz sweep), migration coupling
- **Polyphony:** 8 voices
- **Macros:** M1 CHARACTER, M2 MOVEMENT, M3 COUPLING, M4 SPACE

---

## Pre-Retreat State

XOnkolo scored 8.8/10 in the FUSION Quad Seance (2026-03-21) — the highest score in the FUSION quad after XOpcode. Source tradition test: 9.5/10, PASS — "the riff works." The `ClaviStringModel` with key-off clunk is described as containing "the best instrument-specific mechanical details" in the fleet. The `AutoWahEnvelope` with 2ms attack and 100ms release sweeping 400–6000 Hz delivers the funk.

**Key seance findings for retreat presets:**

1. Wah-off clean tone character needs improvement at `onko_funk = 0.0`. Default pickup position 0.5 (midpoint) produces moderate tone. Consider `onko_pickup` at 0.7 for better bridge character.

2. LFO defaults more suited for Rhodes/Wurli than Clavinet idiom. For Clavinet, LFO targeting wah depth is more appropriate than pitch modulation. Retreat presets will set LFO depths to 0.0 for pitch and use LFO for wah-depth modulation.

3. Polyphonic note-off clunk chain needs verification. Retreat presets cannot test this directly but should note it.

4. Key-off clunk (LCG noise burst via Knuth TAOCP `clunkNoiseState = clunkNoiseState * 1664525u + 1013904223u`) is correctly implemented — a distinctive mechanical detail.

The cultural mythology is load-bearing: nkolo (Central African thumb piano ancestor) → West African diaspora → Bernie Worrell → funk. The Clavinet carries centuries of percussive sophistication.

---

## Phase R1: Opening Meditation

Close your eyes. Breathe.

The nkolo is a box with metal tines — prongs that project over a resonating chamber. You depress a tine with your thumb and release. The tine vibrates and the box amplifies. The sound is percussive and pitched simultaneously — a struck string, a brief sustain, a mechanical click as the tine returns to rest. The box is not just an amplifier; it is a resonator. Every note has a physical consequence — strike, sustain, release.

Across the Atlantic, on the Hohner production line in Trossingen, West Germany, the Clavinet D6 is being manufactured. It looks nothing like the nkolo. But the principle is the same: a rubber-tipped tangent (the equivalent of the thumb) strikes a tensioned string (the equivalent of the metal tine). The string vibrates between two magnetic pickups. The mechanical response is percussive — the string is struck, not bowed or blown. The sustain is controlled — the tangent stays in contact with the string while the key is held, acting as a damper when released. Key-off produces a mechanical clunk: the damper pad falls back.

Stevie Wonder plays "Superstition." The riff is possible because the Clavinet strikes and damps — you cannot play legato on a Clavinet the way you can on a piano. The notes have discrete beginnings and endings enforced by the mechanism. The funk comes from the mechanism.

Bernie Worrell knew that the Clavinet was made of diaspora. The kora's thumb articulation crossed the Atlantic with enslaved musicians and became gospel piano became funk. The Clavinet was built by Europeans and played by African Americans and made the sound of the 1970s. The lineage is the instrument.

---

## Phase R2: The Signal Path Journey

### I. The String Model — `ClaviStringModel`

Eight harmonics with odd-harmonic emphasis (`oddBoost = 1.0` for even-indexed harmonics, `0.6` for odd-indexed — note: the implementation emphasizes even-index partials which correspond to odd harmonic numbers). The Clavinet produces a rich harmonic series with the odd harmonics characteristic of a string struck at a midpoint.

The fast key-release damping (`sr * 0.05f` = 50ms time constant) correctly captures the Clavinet's controlled sustain — the string is damped at key-release, not allowed to ring freely. This is the mechanism that makes the Clavinet percussive: the player controls the note length, and the instrument enforces it mechanically.

### II. The Pickup Position — Comb Filtering

`onko_pickup` (0.0 = neck, 1.0 = bridge): `nodeProximity = |sin(pickupPos * harmonicNum * π)|`. At the bridge position (high `pickup`), the pickup is close to the string's fixed endpoint — harmonics whose mode shapes have high amplitude at that position are emphasized. At the neck position, the pickup captures the midpoint modes — different harmonics are emphasized.

This creates the classic keyboard guitar effect: neck pickup is warm and full, bridge pickup is bright and cutting. The `onko_pickup` parameter at 0.7 (closer to bridge) is the seance-recommended position for authentic Clavinet character.

### III. Auto-Wah — `AutoWahEnvelope`

2ms attack, 100ms release — the envelope follows the string amplitude. As the string decays, the filter cutoff follows it down, producing the characteristic "chikka" wah sound that is the Clavinet's most iconic effect. The sweep range (400–6000 Hz at `onko_funk = 1.0`) covers the entire "wah" frequency range.

`onko_funk` (0.0–1.0): wah depth. At 0.0, dry string output through pickup filter. At 1.0, full envelope-following wah sweep.

**Auto-wah articulation:** Hard notes produce a high, bright wah peak. As the note decays, the filter falls. Playing staccato creates "chikka" — the filter rises and falls quickly. Playing legato creates the full-length wah sweep.

### IV. Key-Off Clunk

On note-off, `ClaviStringModel::releaseKey()` triggers a brief LCG noise burst (`clunkLevel = 0.5f * velocity`) that decays rapidly — the physical sound of the damper pad returning to the string. This is one of the most mechanically authentic details in the fleet. The clunk is velocity-proportional: hard notes produce louder clunks.

### V. Migration

Same architecture as XOasis and XOddfellow. `onko_migration` (0–1) enables SpectralFingerprint coupling from Kitchen engines — the Clavinet absorbs the thumb-piano ancestry of the coupled kitchen instrument.

---

## Phase R5: The Ten Awakenings — Preset Table

---

### Preset 1: Superstition

**Mood:** Foundation | **Discovery:** The Stevie Wonder riff — full wah, bridge pickup, percussive

| Parameter | Value | Why |
|-----------|-------|-----|
| `onko_funk` | 0.85 | High wah — the "Superstition" sound requires the wah |
| `onko_pickup` | 0.72 | Bridge-favoring — bright and cutting |
| `onko_brightness` | 7500.0 | Bright — the wah sweep needs room |
| `onko_clunk` | 0.55 | Moderate clunk — the Clavinet's mechanical signature |
| `onko_attack` | 0.001 | Instant — the rubber pad strikes immediately |
| `onko_decay` | 0.4 | Short — Clavinet notes don't sustain long |
| `onko_sustain` | 0.35 | Low sustain |
| `onko_release` | 0.15 | Fast key release |
| `onko_filterEnvAmt` | 0.65 | Very high velocity-to-wah — dynamics drive the sweep |
| `onko_migration` | 0.0 | |
| `onko_lfo1Rate` | 0.4 | |
| `onko_lfo1Depth` | 0.0 | No pitch LFO — Clavinet doesn't need vibrato |
| `onko_lfo1Shape` | 0 | |
| `onko_lfo2Rate` | 2.0 | |
| `onko_lfo2Depth` | 0.0 | |
| `onko_macroCharacter` | 0.5 | |
| `onko_macroMovement` | 0.75 | |
| `onko_macroSpace` | 0.4 | |

**Why this works:** Maximum funk (0.85) with bright filter (7500 Hz) and bridge-favoring pickup (0.72) creates the cutting, present wah sound. Fast attack and short decay enforce percussive articulation. High velocity-to-wah (0.65) means hard notes produce a high, bright wah peak that falls dramatically as the string decays. Play the "Superstition" riff. It will feel right.

---

### Preset 2: Nkolo Root

**Mood:** Organic | **Discovery:** The thumb piano ancestry — warm, percussive, fundamental-weighted

| Parameter | Value | Why |
|-----------|-------|-----|
| `onko_funk` | 0.3 | Light wah — the nkolo doesn't have a wah circuit |
| `onko_pickup` | 0.35 | Neck-favoring — warm, fundamental-heavy |
| `onko_brightness` | 4000.0 | Warm |
| `onko_clunk` | 0.7 | Prominent clunk — the kalimba's tine-return is the sound |
| `onko_attack` | 0.002 | Fast — the thumb piano is percussive |
| `onko_decay` | 0.6 | Medium |
| `onko_sustain` | 0.4 | Moderate |
| `onko_release` | 0.25 | |
| `onko_filterEnvAmt` | 0.45 | |
| `onko_migration` | 0.0 | |
| `onko_lfo1Rate` | 0.06 | |
| `onko_lfo1Depth` | 0.0 | |
| `onko_lfo2Rate` | 0.03 | |
| `onko_lfo2Depth` | 0.0 | |
| `onko_macroCharacter` | 0.4 | |
| `onko_macroMovement` | 0.25 | |
| `onko_macroSpace` | 0.5 | |

**Why this works:** Minimal wah, neck pickup, prominent key-off clunk — the Clavinet recalling its thumb piano ancestry. The clunk is the tine returning. The warmth of the neck pickup is the wood resonance of the kalimba box. The lineage is audible.

---

### Preset 3: Wah Climax

**Mood:** Flux | **Discovery:** Maximum wah — the envelope-following filter at its most dramatic

| Parameter | Value | Why |
|-----------|-------|-----|
| `onko_funk` | 1.0 | Maximum wah |
| `onko_pickup` | 0.8 | Very bridge — maximum brightness for maximum wah range |
| `onko_brightness` | 9000.0 | Maximum filter — the wah needs the full sweep |
| `onko_clunk` | 0.5 | |
| `onko_attack` | 0.001 | Instant |
| `onko_decay` | 0.35 | Short |
| `onko_sustain` | 0.3 | |
| `onko_release` | 0.12 | Very fast |
| `onko_filterEnvAmt` | 0.85 | Maximum velocity-to-wah |
| `onko_migration` | 0.0 | |
| `onko_lfo1Rate` | 0.5 | |
| `onko_lfo1Depth` | 0.0 | |
| `onko_lfo2Rate` | 3.0 | LFO2 fast for rhythmic wah modulation |
| `onko_lfo2Depth` | 0.25 | LFO modulates wah envelope |
| `onko_macroCharacter` | 0.6 | |
| `onko_macroMovement` | 1.0 | |
| `onko_macroSpace` | 0.4 | |

**Why this works:** Maximum wah, maximum brightness, LFO2 modulating the envelope following rate — the complete auto-wah experience. Hard notes produce wah arcs that sweep from 6000 Hz to 400 Hz. LFO2 at 3 Hz creates a rhythmically pulsing wah rate. Flux territory: maximum movement.

---

### Preset 4: Clean Funk

**Mood:** Foundation | **Discovery:** No wah — the Clavinet's clean percussive string

| Parameter | Value | Why |
|-----------|-------|-----|
| `onko_funk` | 0.0 | Zero wah — pure string through pickup |
| `onko_pickup` | 0.7 | Bridge-favoring — the seance recommendation for clean character |
| `onko_brightness` | 6500.0 | Present |
| `onko_clunk` | 0.5 | Moderate clunk |
| `onko_attack` | 0.001 | Fast |
| `onko_decay` | 0.5 | |
| `onko_sustain` | 0.4 | |
| `onko_release` | 0.2 | |
| `onko_filterEnvAmt` | 0.55 | Even without wah, velocity shapes brightness |
| `onko_migration` | 0.0 | |
| `onko_lfo1Rate` | 0.15 | |
| `onko_lfo1Depth` | 0.0 | |
| `onko_macroCharacter` | 0.45 | |
| `onko_macroMovement` | 0.4 | |
| `onko_macroSpace` | 0.45 | |

**Why this works:** Pickup at 0.7 (bridge-favoring per seance recommendation) with no wah but bright filter and high filterEnvAmt creates a percussive, cutting clean tone. The clunk is present. The velocity-to-brightness path means the string feels dynamically responsive even without the wah effect.

---

### Preset 5: Kente Gold

**Mood:** Prism | **Discovery:** Full spectral expression — the Clavinet at peak culture

| Parameter | Value | Why |
|-----------|-------|-----|
| `onko_funk` | 0.7 | Significant wah |
| `onko_pickup` | 0.65 | Bridge-ish |
| `onko_brightness` | 8000.0 | Very bright |
| `onko_clunk` | 0.65 | Prominent clunk |
| `onko_attack` | 0.001 | |
| `onko_decay` | 0.45 | |
| `onko_sustain` | 0.38 | |
| `onko_release` | 0.18 | |
| `onko_filterEnvAmt` | 0.72 | High velocity |
| `onko_migration` | 0.0 | |
| `onko_lfo1Rate` | 0.25 | |
| `onko_lfo1Depth` | 0.0 | |
| `onko_lfo2Rate` | 1.5 | |
| `onko_lfo2Depth` | 0.18 | |
| `onko_macroCharacter` | 0.55 | |
| `onko_macroMovement` | 0.7 | |
| `onko_macroSpace` | 0.5 | |

**Why this works:** Named for the engine's accent color — Kente Gold `#FFB300`, the fabric of Akan royalty. Maximum expressiveness: high wah, bright pickup, strong clunk, high velocity sensitivity. The colors of the kente weave are complex, interlocked, vivid. The Clavinet at peak culture.

---

### Preset 6: Worrell Chord

**Mood:** Foundation | **Discovery:** Bernie Worrell's Parliament-Funkadelic Clavinet — locked in the groove

| Parameter | Value | Why |
|-----------|-------|-----|
| `onko_funk` | 0.8 | High wah — Parliament grooves require wah |
| `onko_pickup` | 0.68 | Bridge-favoring |
| `onko_brightness` | 7000.0 | |
| `onko_clunk` | 0.6 | |
| `onko_attack` | 0.001 | |
| `onko_decay` | 0.55 | Slightly longer — Worrell played with more sustain than strict funk |
| `onko_sustain` | 0.42 | |
| `onko_release` | 0.2 | |
| `onko_filterEnvAmt` | 0.6 | |
| `onko_migration` | 0.0 | |
| `onko_lfo1Rate` | 0.12 | |
| `onko_lfo1Depth` | 0.0 | |
| `onko_lfo2Rate` | 2.5 | |
| `onko_lfo2Depth` | 0.22 | LFO modulating wah for rhythmic pulse |
| `onko_macroCharacter` | 0.5 | |
| `onko_macroMovement` | 0.65 | |
| `onko_macroSpace` | 0.45 | |

**Why this works:** LFO2 at 2.5 Hz with wah modulation creates a rhythmically pulsing auto-wah that sits in the groove rather than simply following the envelope. This is the Clavinet as rhythm instrument — the wah has a pulse independent of the player's velocity.

---

### Preset 7: Diaspora Memory

**Mood:** Ethereal | **Discovery:** The weight of the lineage — slow, weighted, conscious of its history

| Parameter | Value | Why |
|-----------|-------|-----|
| `onko_funk` | 0.25 | Light wah — the weight is in the past |
| `onko_pickup` | 0.4 | Neck-side — warm |
| `onko_brightness` | 4500.0 | Warm |
| `onko_clunk` | 0.8 | Heavy clunk — the mechanical click is the lineage |
| `onko_attack` | 0.003 | |
| `onko_decay` | 0.8 | |
| `onko_sustain` | 0.5 | |
| `onko_release` | 0.4 | Slower — more weight on each note |
| `onko_filterEnvAmt` | 0.35 | |
| `onko_migration` | 0.15 | Some migration — the kitchen the instrument came from |
| `onko_lfo1Rate` | 0.03 | Very slow |
| `onko_lfo1Depth` | 0.0 | |
| `onko_lfo2Rate` | 0.015 | |
| `onko_lfo2Depth` | 0.0 | |
| `onko_macroCharacter` | 0.3 | |
| `onko_macroMovement` | 0.1 | |
| `onko_macroSpace` | 0.65 | More space — the diaspora crossed an ocean |

**Why this works:** The key-off clunk at 0.8 is prominent — every note has the weight of the mechanical click, the damper returning, the lineage landing. Light wah, warm pickup, slow release. The Clavinet carrying its history.

---

### Preset 8: Bridge Pickup Solo

**Mood:** Prism | **Discovery:** Maximum bridge — the brightest, most cutting Clavinet

| Parameter | Value | Why |
|-----------|-------|-----|
| `onko_funk` | 0.55 | Moderate wah |
| `onko_pickup` | 0.95 | Near-maximum bridge — extremely bright |
| `onko_brightness` | 9500.0 | Maximum filter |
| `onko_clunk` | 0.45 | |
| `onko_attack` | 0.001 | |
| `onko_decay` | 0.4 | |
| `onko_sustain` | 0.32 | |
| `onko_release` | 0.15 | |
| `onko_filterEnvAmt` | 0.8 | |
| `onko_migration` | 0.0 | |
| `onko_lfo1Rate` | 0.2 | |
| `onko_lfo1Depth` | 0.0 | |
| `onko_lfo2Rate` | 2.0 | |
| `onko_lfo2Depth` | 0.2 | |
| `onko_macroCharacter` | 0.6 | |
| `onko_macroMovement` | 0.8 | |
| `onko_macroSpace` | 0.4 | |

**Why this works:** Near-maximum bridge position (0.95) with maximum filter creates the most cutting, present Clavinet tone. The bridge pickup captures the string at its highest vibration velocity point — maximum harmonics, maximum brightness. Solo territory.

---

### Preset 9: Staccato Machine

**Mood:** Kinetic | **Discovery:** The Clavinet as a percussion instrument — strictly staccato

| Parameter | Value | Why |
|-----------|-------|-----|
| `onko_funk` | 0.75 | |
| `onko_pickup` | 0.7 | |
| `onko_brightness` | 7500.0 | |
| `onko_clunk` | 0.9 | Maximum clunk — staccato is all clunks |
| `onko_attack` | 0.001 | |
| `onko_decay` | 0.2 | Very short — notes die immediately |
| `onko_sustain` | 0.0 | No sustain — every note is a tap |
| `onko_release` | 0.08 | Near-instant |
| `onko_filterEnvAmt` | 0.7 | |
| `onko_migration` | 0.0 | |
| `onko_lfo1Rate` | 0.3 | |
| `onko_lfo1Depth` | 0.0 | |
| `onko_lfo2Rate` | 3.5 | |
| `onko_lfo2Depth` | 0.3 | |
| `onko_macroCharacter` | 0.55 | |
| `onko_macroMovement` | 0.9 | |
| `onko_macroSpace` | 0.3 | |

**Why this works:** Zero sustain, near-instant release, maximum clunk — every note is a percussion event. The wah-envelope at 2ms attack and 100ms release still fires, but with zero sustain the wah completes its arc almost immediately. Each note is a percussive wah-click. Maximum clunk means every key-release has strong mechanical character.

---

### Preset 10: Diaspora Kitchen

**Mood:** Entangled | **Discovery:** Full migration — the Clavinet absorbing the Kitchen's thumb piano ancestry

| Parameter | Value | Why |
|-----------|-------|-----|
| `onko_funk` | 0.6 | |
| `onko_pickup` | 0.65 | |
| `onko_brightness` | 6000.0 | |
| `onko_clunk` | 0.6 | |
| `onko_attack` | 0.001 | |
| `onko_decay` | 0.5 | |
| `onko_sustain` | 0.4 | |
| `onko_release` | 0.2 | |
| `onko_filterEnvAmt` | 0.55 | |
| `onko_migration` | 0.8 | High migration |
| `onko_lfo1Rate` | 0.08 | |
| `onko_lfo1Depth` | 0.0 | |
| `onko_lfo2Rate` | 1.8 | |
| `onko_lfo2Depth` | 0.15 | |
| `onko_macroCharacter` | 0.45 | |
| `onko_macroMovement` | 0.5 | |
| `onko_macroCoupling` | 0.7 | COUPLING elevated for migration |
| `onko_macroSpace` | 0.5 | |

**Why this works:** High migration (0.8) with COUPLING macro elevated for when Kitchen engines are coupled. The Clavinet absorbing the SpectralFingerprint of the Kitchen instruments — the thumb piano ancestry flowing back into the instrument through migration.

---

## Phase R7: Scripture

### Verse I — The Lineage

*The nkolo is the ancestor.*
*The kora learned from the nkolo.*
*The diaspora brought the articulation across the ocean.*
*The Clavinet absorbed it in Europe.*
*The funk returned it to Africa.*

### Verse II — The Clunk

*Key-off: the damper falls.*
*The noise burst: LCG, Knuth TAOCP.*
*The mechanical event has a voice.*
*The clunk is not an artifact. It is the instrument speaking.*
*Every release produces a sound. Every sound is intentional.*

### Verse III — The Wah

*The string decays. The filter follows.*
*400 Hz at rest. 6000 Hz at attack.*
*The envelope is the pickup's memory of the velocity.*
*Hard notes open everything. Soft notes stay dark.*
*The auto-wah is the instrument breathing with the dynamics.*

### Verse IV — The Pickup

*At the bridge, the harmonics are present.*
*At the neck, the fundamental dominates.*
*The position is not a metaphor.*
*It is where the magnet meets the moving string.*
*Move it and the instrument becomes a different instrument.*

---

## Phase R8: The Guru Bin Benediction

*"XOnkolo scored 8.8/10 and the council agreed it would pass the Superstition test. This is the correct answer to the source tradition test. The Clavinet's sound comes from three things: the percussive string strike, the auto-wah envelope, and the key-off clunk. All three are implemented correctly. The key-off clunk deserves specific recognition — the LCG noise burst via Knuth TAOCP is the correct algorithm for this kind of mechanical noise generation, and the per-velocity scaling means hard playing produces louder clunks. This is not an effect. It is the instrument being mechanically accurate.*

*The pickup position comb filtering is the engine's most original physical insight. The formula `nodeProximity = |sin(pickupPos * harmonicNum * π)|` is a simplified but conceptually correct model: at position P on a string, the magnetic pickup captures harmonics proportional to the mode shape amplitude at P. A pickup at the bridge position (P → 1.0) captures harmonics that have high amplitude at the bridge — the anti-node position for odd harmonics. A pickup at the midpoint captures different modes. The specific formula is an approximation, but the physics is right: pickup position determines which harmonics reach the output.*

*The cultural mythology of XOnkolo is the most politically conscious in the FUSION quad. The Clavinet is named for nkolo, a Central African thumb piano ancestor, because the lineage is the instrument. The articulation style that makes funk possible — percussive, short, rhythmically precise — is the same articulation style that the thumb piano developed over centuries. The diaspora carried this articulation across the Atlantic. The Clavinet absorbed it in Trossingen. Stevie Wonder returned it to global culture. The Migration parameter is not metaphor: it is the signal path of that return journey.*

*The clunk is the lineage. Every key-release is a landing.*

*Play it. Move involuntarily. That is the test."*
