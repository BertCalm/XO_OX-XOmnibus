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

---

# ONKOLO — Second Retreat
*Guru Bin — 2026-03-23 | Expanding the library to 30 presets. Filling the Producers Guild critical gaps.*

---

## Phase R9: Parameter Refinements

| # | Parameter | Current Default | Recommended Default | Reason |
|---|-----------|-----------------|---------------------|--------|
| 1 | `onko_funk` | 0.5 | 0.6 | 0.5 is borderline clean. The Clavinet's identity IS the wah — 0.6 ensures the wah is clearly present at the init patch, matching the instrument's essential character. |
| 2 | `onko_pickup` | 0.7 | 0.72 | Seance-recommended 0.7 is good; 0.72 edges slightly more toward bridge, increasing the initial brightness. This is the sweet spot for the "cutting" Clavinet character that sits in a mix. |
| 3 | `onko_brightness` | 8000.0 | 7000.0 | 8000 Hz is very bright — the Clavinet is present but not harsh. 7000 Hz allows the auto-wah sweep to have more "room above" when it triggers. |
| 4 | `onko_clunk` | 0.5 | 0.55 | The clunk is the Clavinet's most distinctive physical characteristic. 0.55 makes it slightly more present at default — the mechanical click is part of the identity. |
| 5 | `onko_attack` | 0.001 | 0.001 | Correct — instant. Keep. |
| 6 | `onko_decay` | 0.3 | 0.4 | 0.3s is very short — even for a Clavinet. 0.4s allows held notes to ring slightly longer before the natural string decay takes over. |
| 7 | `onko_sustain` | 0.4 | 0.38 | Minor reduction — lower sustain means the string's physical decay becomes more audible, rather than being held at an artificial level. |
| 8 | `onko_release` | 0.15 | 0.12 | The Clavinet's key-release IS the damper — very fast is correct. 0.12s is even more mechanical. The key-off clunk plays regardless of release setting. |
| 9 | `onko_filterEnvAmt` | 0.6 | 0.65 | Increase to match the higher wah default. More filterEnvAmt means velocity contrast is more pronounced — hard notes trigger a stronger wah sweep. |
| 10 | `onko_lfo1Depth` | 0.0 | 0.0 | Correct — no pitch LFO on the Clavinet. The wah IS the modulation. |
| 11 | `onko_lfo2Depth` | 0.0 | 0.0 | Keep at zero — LFO2 affects wah depth per preset. |
| 12 | `onko_macroCharacter` | 0.0 | 0.4 | CHARACTER (funk/wah depth) should be at a perceptible starting position. 0.4 means the CHARACTER macro provides audible increase from default. |

---

## Phase R10: The Twenty Awakenings — Filling the Guild Gaps

*Presets 11–30. Guild-identified gaps: classic funk riff, wah machine, Afrobeat, slap clavinet, 70s fusion.*

---

### Preset 11: Parliament Funkadelic

**Mood:** Kinetic | **Guild gap:** Classic funk riff (Parliament/Stevie)

Bernie Worrell playing in Parliament — the Clavinet as a collective instrument, part of an ensemble that was moving together. The wah is full, the decay is percussive, the playing is rhythmic.

| Parameter | Value | Why |
|-----------|-------|-----|
| `onko_funk` | 0.9 | Very high wah — Parliament's Clavinet was always all-in |
| `onko_pickup` | 0.75 | Bridge-forward — bright and cutting |
| `onko_brightness` | 7000.0 | |
| `onko_clunk` | 0.6 | Present clunk — the mechanism is part of the rhythm |
| `onko_attack` | 0.001 | Instant |
| `onko_decay` | 0.35 | Very short — Parliament is all attack |
| `onko_sustain` | 0.3 | Low |
| `onko_release` | 0.1 | Very fast |
| `onko_filterEnvAmt` | 0.72 | Very high velocity |
| `onko_migration` | 0.0 | |
| `onko_lfo1Rate` | 0.5 | |
| `onko_lfo1Depth` | 0.0 | |
| `onko_lfo2Rate` | 4.0 | Fast LFO2 for rhythmic wah variation |
| `onko_lfo2Depth` | 0.3 | LFO2 modulates wah depth — rhythmic wah sweep |
| `onko_macroCharacter` | 0.6 | |
| `onko_macroMovement` | 0.9 | |
| `onko_macroSpace` | 0.4 | |

**Why this works:** The highest wah depth (0.9) with the highest velocity sensitivity (0.72) — every note is a wah event. The LFO2 at 4 Hz modulating wah depth creates rhythmic wah variation: the wah envelope follows velocity AND has its own rate, creating the characteristic Parliament "chika-wika" texture. Very short decay (0.35s) means the rhythm is percussive. Parliament-Funkadelic is about density and rhythm, not sustain.

---

### Preset 12: Auto-Wah Machine

**Mood:** Flux | **Guild gap:** Wah machine — envelope-following filter as primary voice

This preset foregrounds the auto-wah mechanism as a compositional element rather than a timbre. The string is secondary; the filter sweep is primary.

| Parameter | Value | Why |
|-----------|-------|-----|
| `onko_funk` | 1.0 | Maximum wah |
| `onko_pickup` | 0.85 | Very bridge — the brightest string for maximum wah range |
| `onko_brightness` | 10000.0 | Maximum filter ceiling — the wah has full range |
| `onko_clunk` | 0.45 | |
| `onko_attack` | 0.001 | |
| `onko_decay` | 0.5 | Medium — the wah needs time to sweep |
| `onko_sustain` | 0.45 | |
| `onko_release` | 0.15 | |
| `onko_filterEnvAmt` | 0.9 | Near-maximum velocity-to-wah |
| `onko_migration` | 0.0 | |
| `onko_lfo1Rate` | 0.8 | |
| `onko_lfo1Depth` | 0.0 | |
| `onko_lfo2Rate` | 0.7 | LFO2 at 0.7 Hz — a slow 1.4-second wah modulation cycle |
| `onko_lfo2Depth` | 0.5 | Heavy LFO2 — the wah depth itself is sweeping |
| `onko_macroCharacter` | 0.65 | |
| `onko_macroMovement` | 1.0 | Maximum movement — this is a movement preset |
| `onko_macroSpace` | 0.45 | |

**Why this works:** Maximum wah (1.0) with maximum filter ceiling (10000 Hz) creates the full 400–6000 Hz sweep range. The LFO2 at 0.7 Hz modulating the wah depth at 50% means the wah's sensitivity itself cycles slowly — creating a breathing, rhythmic variation in how much wah each note triggers. Combined with velocity sensitivity at 0.9, every note is a unique wah event AND the wah depth changes over time. This is the auto-wah as an autonomous voice.

---

### Preset 13: Afrobeat Clavi

**Mood:** Organic | **Guild gap:** Afrobeat — Fela Kuti era Clavinet in Lagos

The Afrobeat Clavinet — the rhythm section instrument in Fela Kuti's Africa 70 — was brighter and more percussive than the American funk Clavinet. Less wah, more raw string. The rhythm was the arrangement.

| Parameter | Value | Why |
|-----------|-------|-----|
| `onko_funk` | 0.45 | Moderate wah — Afrobeat Clavinet was more raw than wah-heavy |
| `onko_pickup` | 0.78 | Bridge position — bright and present |
| `onko_brightness` | 8500.0 | Bright — Afrobeat requires presence in the ensemble |
| `onko_clunk` | 0.65 | Strong clunk — the mechanical element is the rhythm |
| `onko_attack` | 0.001 | Instant |
| `onko_decay` | 0.3 | Very short — the Afrobeat riff is entirely attack |
| `onko_sustain` | 0.25 | Very low — staccato |
| `onko_release` | 0.08 | Extremely fast |
| `onko_filterEnvAmt` | 0.6 | High velocity |
| `onko_migration` | 0.0 | |
| `onko_lfo1Rate` | 0.6 | |
| `onko_lfo1Depth` | 0.0 | |
| `onko_lfo2Rate` | 6.0 | Fast LFO2 — Afrobeat has high rhythmic density |
| `onko_lfo2Depth` | 0.2 | |
| `onko_macroCharacter` | 0.5 | |
| `onko_macroMovement` | 0.8 | High rhythmic movement |
| `onko_macroSpace` | 0.38 | Tighter — Afrobeat is dense |

**Why this works:** The very short decay (0.3s) and extremely fast release (0.08s) enforce staccato articulation — the Afrobeat Clavinet is percussive, not melodic. The strong clunk (0.65) makes the key-off noise audible as a rhythm element. The LFO2 at 6 Hz is in the range of the rhythmic density of the ensemble — not a slow sweep but a fast texture. This preset is designed for riff playing at high tempo.

---

### Preset 14: Slap Clavinet

**Mood:** Kinetic | **Guild gap:** Slap Clavinet — the most aggressive articulation

The slap technique on Clavinet — hitting the keys with force and releasing quickly, creating a percussive accent — requires high attack velocity sensitivity, very fast release, and the clunk set to maximum.

| Parameter | Value | Why |
|-----------|-------|-----|
| `onko_funk` | 0.75 | High wah — slaps always trigger the full wah |
| `onko_pickup` | 0.8 | Very bridge |
| `onko_brightness` | 8000.0 | |
| `onko_clunk` | 0.95 | Near-maximum clunk — the slap's release is the whole sound |
| `onko_attack` | 0.001 | Instant |
| `onko_decay` | 0.2 | Very short — slap is attack, nothing else |
| `onko_sustain` | 0.15 | Minimal — key is released immediately |
| `onko_release` | 0.06 | Near-instant release — the slap ends as fast as it begins |
| `onko_filterEnvAmt` | 0.85 | Maximum velocity response — the slap IS the dynamics |
| `onko_migration` | 0.0 | |
| `onko_lfo1Rate` | 0.3 | |
| `onko_lfo1Depth` | 0.0 | |
| `onko_lfo2Rate` | 8.0 | Very fast LFO2 — rhythmic texture at slap tempo |
| `onko_lfo2Depth` | 0.35 | |
| `onko_macroCharacter` | 0.7 | |
| `onko_macroMovement` | 1.0 | Maximum |
| `onko_macroSpace` | 0.35 | |

**Why this works:** Near-maximum clunk (0.95) means the key-off noise is prominent — the slap technique requires the audible mechanical noise. Very fast release (0.06s) means notes end immediately — the slap is instantaneous. Maximum velocity-to-wah (0.85) ensures every hard slap produces a dramatic wah event. The LFO2 at 8 Hz adds timbral movement at the tempo of aggressive playing. This is the Clavinet as a weapon.

---

### Preset 15: Hancock Fusion

**Mood:** Prism | **Guild gap:** 70s fusion Clavinet — Herbie Hancock in the Head Hunters context

Hancock used the Clavinet more sparingly than Stevie Wonder, but in the Head Hunters context the Clavinet was a texture in a complex ensemble. Less wah than pure funk, more movement, more unpredictable dynamics.

| Parameter | Value | Why |
|-----------|-------|-----|
| `onko_funk` | 0.65 | Medium-high wah |
| `onko_pickup` | 0.7 | Standard bridge |
| `onko_brightness` | 7500.0 | |
| `onko_clunk` | 0.5 | |
| `onko_attack` | 0.001 | |
| `onko_decay` | 0.45 | Slightly longer than pure funk |
| `onko_sustain` | 0.38 | |
| `onko_release` | 0.18 | |
| `onko_filterEnvAmt` | 0.65 | |
| `onko_migration` | 0.1 | Fusion is about instrument interactions |
| `onko_lfo1Rate` | 0.4 | |
| `onko_lfo1Depth` | 0.0 | |
| `onko_lfo2Rate` | 1.8 | Moderate LFO2 — fusion has slower wah modulation than Parliament |
| `onko_lfo2Depth` | 0.22 | |
| `onko_macroCharacter` | 0.55 | |
| `onko_macroMovement` | 0.7 | |
| `onko_macroCoupling` | 0.1 | |
| `onko_macroSpace` | 0.5 | More space than pure funk — fusion breathes |

**Why this works:** Slightly longer decay (0.45s) and more space (macroSpace 0.5) than pure funk creates the fusion aesthetic: the instrument has room to breathe. The LFO2 at 1.8 Hz vs Parliament's 4 Hz creates slower, more musical wah variation. Migration at 0.1 acknowledges the ensemble context of fusion. The result is a Clavinet that is funky but not relentlessly rhythmic.

---

### Preset 16: Nkolo Ancestral

**Mood:** Ethereal | **Extra depth:** Deep ancestry — the Clavinet remembering before diaspora

| Parameter | Value | Why |
|-----------|-------|-----|
| `onko_funk` | 0.2 | Minimal wah — the thumb piano had no circuit |
| `onko_pickup` | 0.3 | Neck position — warm, fundamental-heavy, like wood resonance |
| `onko_brightness` | 3500.0 | Warm — the tine in a wooden box |
| `onko_clunk` | 0.85 | High clunk — the tine's return IS the sound |
| `onko_attack` | 0.001 | |
| `onko_decay` | 0.8 | Longer — the box resonates the tine longer |
| `onko_sustain` | 0.5 | |
| `onko_release` | 0.35 | The wooden box has residual resonance |
| `onko_filterEnvAmt` | 0.38 | |
| `onko_migration` | 0.2 | Strong coupling — the ancestor is present in the descendant |
| `onko_lfo1Rate` | 0.04 | |
| `onko_lfo1Depth` | 0.0 | |
| `onko_lfo2Rate` | 0.02 | |
| `onko_lfo2Depth` | 0.03 | Very slow — the resonance of time |
| `onko_macroCharacter` | 0.35 | |
| `onko_macroMovement` | 0.1 | |
| `onko_macroCoupling` | 0.2 | The ancestry is in the coupling |
| `onko_macroSpace` | 0.65 | |

**Why this works:** Neck position (0.3), minimal wah, warm filter, prominent clunk — the Clavinet heard through the lens of its thumb-piano ancestry. Migration at 0.2 acknowledges that the instrument carries its lineage. The slow filter drift (LFO2 at 0.02 Hz) is measured in minutes — the resonance of deep time.

---

### Preset 17: Clavi Solo

**Mood:** Foundation | **Extra depth:** Single-line melodic playing — the Clavinet as a horn

| Parameter | Value | Why |
|-----------|-------|-----|
| `onko_funk` | 0.7 | Present wah for melodic expressiveness |
| `onko_pickup` | 0.72 | |
| `onko_brightness` | 7200.0 | |
| `onko_clunk` | 0.5 | |
| `onko_attack` | 0.001 | |
| `onko_decay` | 0.6 | Slightly longer for melodic lines |
| `onko_sustain` | 0.5 | Moderate — melodic lines need some sustain |
| `onko_release` | 0.25 | |
| `onko_filterEnvAmt` | 0.6 | |
| `onko_migration` | 0.0 | |
| `onko_lfo1Rate` | 0.3 | |
| `onko_lfo1Depth` | 0.0 | |
| `onko_lfo2Rate` | 2.5 | Moderate LFO2 — melodic lines have slower wah variation |
| `onko_lfo2Depth` | 0.18 | |
| `onko_macroCharacter` | 0.5 | |
| `onko_macroMovement` | 0.55 | |
| `onko_macroSpace` | 0.5 | |

**Why this works:** Slightly longer decay (0.6s) and moderate sustain (0.5) allow single-note melodic lines to have shape — the note rings slightly before decaying, giving legato playing a natural feel. The wah at 0.7 means melodic lines have expressive brightness variations from velocity. The Clavinet as a lead instrument.

---

### Preset 18: Wah Off Bright

**Mood:** Foundation | **Extra depth:** Clean, bright, no wah — the Clavinet's raw pickup character

| Parameter | Value | Why |
|-----------|-------|-----|
| `onko_funk` | 0.0 | Zero wah — pure pickup |
| `onko_pickup` | 0.9 | Very bridge — maximum brightness |
| `onko_brightness` | 9000.0 | Very bright |
| `onko_clunk` | 0.55 | |
| `onko_attack` | 0.001 | |
| `onko_decay` | 0.45 | |
| `onko_sustain` | 0.38 | |
| `onko_release` | 0.15 | |
| `onko_filterEnvAmt` | 0.7 | Very high — without wah, filter velocity does all the work |
| `onko_migration` | 0.0 | |
| `onko_lfo1Rate` | 0.5 | |
| `onko_lfo1Depth` | 0.0 | |
| `onko_lfo2Rate` | 1.0 | |
| `onko_lfo2Depth` | 0.0 | |
| `onko_macroCharacter` | 0.4 | |
| `onko_macroMovement` | 0.5 | |
| `onko_macroSpace` | 0.45 | |

**Why this works:** Bridge position (0.9) with no wah and high brightness — the rawest Clavinet character. The filterEnvAmt at 0.7 compensates for the absent wah by making velocity-to-brightness very sensitive. Hard notes are very bright; soft notes are noticeably darker. The bridge position creates a naturally cutting tone that sits in any mix without effects.

---

### Preset 19: Spice Route Thumb

**Mood:** Entangled | **Extra depth:** Maximum coupling — the Clavinet absorbing Kitchen character

| Parameter | Value | Why |
|-----------|-------|-----|
| `onko_funk` | 0.65 | |
| `onko_pickup` | 0.7 | |
| `onko_brightness` | 6500.0 | |
| `onko_clunk` | 0.5 | |
| `onko_attack` | 0.001 | |
| `onko_decay` | 0.5 | |
| `onko_sustain` | 0.42 | |
| `onko_release` | 0.2 | |
| `onko_filterEnvAmt` | 0.6 | |
| `onko_migration` | 0.8 | High migration |
| `onko_lfo1Rate` | 0.2 | |
| `onko_lfo1Depth` | 0.1 | Higher LFO under coupling influence |
| `onko_lfo2Rate` | 1.5 | |
| `onko_lfo2Depth` | 0.2 | |
| `onko_macroCharacter` | 0.5 | |
| `onko_macroMovement` | 0.5 | |
| `onko_macroCoupling` | 0.8 | COUPLING elevated for migration |
| `onko_macroSpace` | 0.5 | |

**Why this works:** The Clavinet on the Spice Route — the thumb piano's descendant absorbing the spectral character of the Kitchen instruments. Migration at 0.8 means the wah sweep and string character respond to the coupled Kitchen engines' SpectralFingerprint. The nkolo lineage continues its journey through new kitchens.

---

### Preset 20: Thumb Piano Modern

**Mood:** Luminous | **Extra depth:** Contemporary production using the Clavinet's ancestral character

| Parameter | Value | Why |
|-----------|-------|-----|
| `onko_funk` | 0.55 | Moderate wah |
| `onko_pickup` | 0.65 | Moderate bridge |
| `onko_brightness` | 6000.0 | |
| `onko_clunk` | 0.6 | Present clunk — the clunk is charming in modern production |
| `onko_attack` | 0.001 | |
| `onko_decay` | 0.55 | |
| `onko_sustain` | 0.42 | |
| `onko_release` | 0.2 | |
| `onko_filterEnvAmt` | 0.58 | |
| `onko_migration` | 0.0 | |
| `onko_lfo1Rate` | 0.25 | |
| `onko_lfo1Depth` | 0.0 | |
| `onko_lfo2Rate` | 3.0 | |
| `onko_lfo2Depth` | 0.15 | |
| `onko_macroCharacter` | 0.45 | |
| `onko_macroMovement` | 0.5 | |
| `onko_macroSpace` | 0.55 | |

**Why this works:** A balanced, contemporary preset that acknowledges both the Clavinet's funk heritage and its thumb-piano ancestry. Moderate settings across the board with the prominent clunk — a production choice to let the mechanical character be audible. This is for producers who want the Clavinet as a textured rhythmic element, not a pure funk machine.

---

## Phase R11: Second Scripture

### Verse V — The Key-Off Clunk

*The Knuth LCG generates noise.*
*The noise level scales with velocity.*
*A soft note produces a soft clunk.*
*A hard note produces a loud clunk.*
*The clunk is the damper returning.*
*It is not a mistake.*
*It is the instrument telling you the note has ended.*

### Verse VI — The Pickup Position

*At 0.0: warmth. The neck speaks.*
*At 1.0: brightness. The bridge speaks.*
*Between: the harmonic modes shift.*
*No two pickup positions are the same instrument.*
*Move the parameter one step.*
*Listen to what changes.*
*The instrument is a variable.*

### Verse VII — The Lineage

*Nkolo → kalimba → diaspora → gospel → funk → Clavinet D6.*
*Each step was a translation.*
*The translation preserved the essential thing:*
*percussive, pitched, controlled by the thumb.*
*The wah circuit was added in Trossingen.*
*The lineage was carried across the Atlantic.*
*The instrument returned to where the lineage began*
*in the hands of Bernie Worrell, Stevie Wonder, Parliament.*
*The clunk is the oldest part.*
*The tine returning to rest.*
