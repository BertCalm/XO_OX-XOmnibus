# OCTAVE Retreat Chapter
*Guru Bin — 2026-03-21*

---

## Engine Identity

- **Gallery code:** OCTAVE | **Accent:** Bordeaux `#6B2D3E`
- **Parameter prefix:** `oct_`
- **Creature mythology:** The stone cathedral submerged at the continental shelf — four chambers, four centuries, four breaths of air. In the Romantic nave, Cavaillé-Coll's pipes exhale dark harmonic towers that take seconds to fill the space. In the Baroque positiv, chiff bursts precede transparent principals like light through leaded glass. In the Musette alcove, three reeds beat against each other with Parisian warmth. In the transistor crypt, Farfisa squares buzz with garage-rock immediacy. Octave is the Chef — classically trained, structurally precise, every note placed with music-theory intent.
- **Synthesis type:** Four-model organ synthesizer — additive partials (12-harmonic drawbar array), Baroque chiff transients, triple-reed beating, bandlimited PolyBLEP square wave. All four share the Chef parameter vocabulary (cluster, chiff, detune, buzz, pressure, crosstalk) weighted differently per model.
- **Polyphony:** 8 voices with chromatic stereo spread (note % 12 pan distribution)
- **Macros:** M1 CHARACTER (pressure + brightness + partial morph), M2 MOVEMENT (registration + bellows), M3 COUPLING (crosstalk + organ morph via couplingOrganMod), M4 SPACE (room depth)
- **Historical lineage:** Aristide Cavaillé-Coll (Saint-Sulpice, 1862), Arp Schnitger (Hamburg Baroque, 1693), Paolo Soprani (Castelfidardo accordion, 1863), Silvio Nascimbeni (Farfisa, 1964). Organ acoustics from Audsley (1905), Jaffe & Smith (1983). Accordion reed beating from Millot et al. (2001). Chiff transient model from Nolle (1979).

---

## Pre-Retreat State

**Seance score: 8.01 / 10** — fleet-below-average at seance, with two D004 violations flagged: `macroCoupling` was loaded but never applied, and `couplingOrganMod` was accumulated via `EnvToMorph` but immediately zeroed and discarded before reaching the synthesis loop. These have been fixed. `macroCoupling` now drives `effectiveCrosstalk` via `couplingOrganMod * 0.3`, and `couplingOrganMod` properly feeds the partial amplitude blend between Cavaillé-Coll and Baroque tables (`organMorphBlend`). Post-mix room resonance was moved from per-voice to two global instances (L/R), correcting the spatial model and reducing CPU.

Post-fix estimated score: **8.7 / 10** — fleet average.

This engine arrived with 22 presets across six moods. This retreat fills the remaining timbral territory: the extreme Cavaillé-Coll/Farfisa contrast that defines the instrument's range, the organ morph as a real-time gesture, and the coupling pathway that turns partial tables into a performance dimension.

The four organ models are not equally dramatic. Cavaillé-Coll is the deepest, the most spatially present, the most historically weighted. Farfisa is the antithesis: no room, no air, no patience. The space between them — the morphing partial blend — is what makes OCTAVE unique in the fleet. No other engine traverses 400 years of the same instrument family.

---

## Phase R1: Opening Meditation

Close your eyes. Breathe.

There is a church in Paris at the end of the Rue du Cherche-Midi. Sunday morning. The organist arrives at six to warm the instrument before the eight o'clock mass. The instrument in question is a Cavaillé-Coll — built in 1862, 4,000 pipes, five manuals. The organist does not sit down immediately. First, he opens the windchest, and the organ begins to breathe. The bellows fill. The pipes are already vibrating at inaudible amplitudes, sympathetically resonant with each other, with the stone, with the frequency of the city's morning traffic. Before the organist plays a single note, the instrument has already begun.

Now go one kilometer north and three decades later. A cellar venue, 1966. A Farfisa Compact rests on a folding table. The keyboard player plugs it in. There is no warm-up. The transistors do not need to breathe. The square wave exists the instant current flows. The vibrato begins at exactly 5.5 Hz, hardwired, the same as every Farfisa ever made. The first note is already fully formed before the key reaches its bottom.

These two instruments are made for the same keys. They produce sound when the same human fingers press the same physical mechanism. They share nothing else.

OCTAVE holds both instruments simultaneously — not as a crossfade between two similar things, but as a choice between two philosophies of what sound should be. The choice is yours. The COUPLING macro can blur the line between them, turning Romantic pipe partials toward Baroque brightness, toward transistor immediacy. The entire 400-year arc is traversable in one gesture.

Sit with that. What do you choose?

---

## Phase R2: The Signal Path Journey

### I. The Four Models — Synthesis Archetypes

OCTAVE's four synthesis models are not skins on the same DSP. They are distinct architectures that happen to share a parameter vocabulary.

**Cavaillé-Coll (oct_organ = 0):** Twelve drawbar partials whose amplitudes are sourced from Audsley (1905): `kCCPartialAmps[12] = {1.0, 0.8, 0.5, 0.4, 0.25, 0.2, 0.12, 0.1, 0.07, 0.06, 0.04, 0.03}`. The first four partials (8' register) are weighted by `lerp(0.5, 1.0, registration)`. The middle four (4') scale with `registration * kRegistration4ft`. The upper four (2') scale with `registration * kRegistration2ft`. This is a simplified three-rank scheme, not a true multi-stop drawbar, but the result is a dark, symphonic tone whose weight lives in the fundamental and first few harmonics. The slow attack (multiplied by 3× before applying model floors: minimum 50ms) is the engine's most historically faithful gesture — air genuinely takes time to fill a 16-foot bourdon pipe.

**Baroque Positiv (oct_organ = 1):** Twelve partials from `kBaroquePartialAmps[12] = {0.7, 1.0, 0.6, 0.8, 0.4, 0.5, 0.3, 0.25, 0.2, 0.15, 0.1, 0.08}` — note the emphasis on the 2nd and 4th harmonics over the fundamental. This is the principal chorus character of a Schnitger organ: bright, transparent, mixture-rich. The chiff generator fires at full weight (`chiffWeights[1] = 1.0`). Attack minimum floor is 5ms — fast but not instant. The chiff burst runs from 5ms at high chiff amounts to 30ms at low amounts; for the Baroque model, higher chiff = shorter burst, because Baroque chiff is a crisp transient, not a bloom.

**French Musette (oct_organ = 2):** Three detuned oscillators per voice. Center reed on-pitch; flanking reeds at `±(1 + detune * 8) Hz`. The reed waveform is a manually constructed odd-harmonic series: `sin(ph) + sin(3ph)*0.33 + sin(5ph)*0.15 + sin(7ph)*0.08` — matching the odd-harmonic profile of a free reed clamped at both ends. Bellows dynamics: `bellows = velocity * 0.6 + pressure * 0.4`. Aftertouch feeds `effectivePressure`, creating a natural correspondence between playing intensity and bellows force. The buzz path (`fastTanh(sample * (3 + buzzAmt * 8)) * buzzAmt * 0.3`) activates when `buzzAmt > 0.001` and `bellows > 0.5` — at high bellows and high buzz, the reed rattle becomes the dominant timbral character.

**Farfisa Compact (oct_organ = 3):** `PolyBLEP::Square` — bandlimited square wave, the only correct choice for transistor organ synthesis. Attack floor of 1ms makes it the most immediate instrument in the engine (and the fleet). Vibrato hardcoded at exactly 5.5 Hz (the Farfisa historical specification), with depth from `oct_detune * 0.015` — subtle, as the original circuit was. Octave-up tab simulation via a second phase accumulator (`partialPhases[0]`) blended at `regNow * 0.3`. No room resonance — correct. No chiff — correct.

### II. The Chef Parameters — Polymorphic Vocabulary

Six parameters whose meaning shifts across all four models:

**`oct_cluster`** (0–1): Voice-spread detuning applied in the render loop — `clusterCents = cluster * voiceOffset * 15.0` where `voiceOffset` is the normalized voice index (−1 to +1). At full cluster, voices 1 and 8 are ±15 cents apart, creating a natural ensemble width without an explicit unison mode. Meaningful on Cavaillé-Coll; unused in Musette or Farfisa (the detuning there comes from `oct_detune`).

**`oct_chiff`** (0–1): Active on models 0 and 1 only. The `OctaveChiffGenerator` is a half-sine windowed noise burst (Nolle 1979) with a first-order LP filter centered at `min(baseFreq * 3, sampleRate * 0.49)`. Duration is inversely proportional to chiff amount (5ms at amount=1.0, 30ms at amount=0). The chiff filter coefficient uses matched-Z: `1 - exp(-2π*fc/sr)` — the only correct method for a recursive filter coefficient (Euler approximation would shift the center frequency).

**`oct_detune`** (0–1): Musette — controls beat frequency (1–9 Hz). Farfisa — controls vibrato depth (0–1.5% pitch deviation). Unused on the two pipe organ models.

**`oct_buzz`** (0–1): Musette — activates tanh reed rattle at high bellows pressure. Farfisa — drives tanh saturation directly at 1–7× gain. Unused on pipe organs.

**`oct_pressure`** (0–1): All models — scales the bellows/wind amplitude. For Cavaillé-Coll, pressure affects partial amplitude scaling (`0.5 + pressure * 0.5`). For Musette, it is the bellows variable itself. Aftertouch feeds `effectivePressure` across all models.

**`oct_crosstalk`** (0–1): Voice-to-voice bleed — each active voice bleeds a small amount of the previous voice's filter output into its own synthesis. Models the acoustic phenomenon of pipes physically close together sharing a windchest. The COUPLING macro drives `effectiveCrosstalk`, turning polyphonic playing into a tighter, more unified acoustic body.

### III. The Organ Morph — The Newly Wired Pathway

The most important fix in this retreat: `couplingOrganMod` is now wired.

Previously, `CouplingType::EnvToMorph` accumulated `couplingOrganMod` (via `applyCouplingInput`) but the value was zeroed before reaching the synthesis loop — a silent discard. Now, `couplingOrganMod` feeds `organMorphBlend`:

```
organMorphBlend = clamp(abs(couplingOrganMod) * 1.5 + macroCharacter * 0.3, 0.0, 1.0)
morphToBright = (couplingOrganMod >= 0.0)
```

When `organMorphBlend > 0` and `morphToBright = true`, Cavaillé-Coll partial amplitudes blend toward Baroque's brighter table. When `morphToBright = false`, the blend deepens the CC partials (adds 30% more fundamental emphasis). This means a coupled envelope from another engine can drive the partial character of OCTAVE's pipe organ models in real time — the organ's harmonic content becomes a modulation destination.

The COUPLING macro (`oct_macroCoupling`) now directly increases `effectiveCrosstalk`. A macro at 1.0 adds 0.5 to the base crosstalk value, turning all 8 voices into an organically blended windchest rather than independent instruments.

**Performance use:** Set `oct_organ = 0`, `oct_macroCoupling = 0.0`. Play a chord. Now sweep COUPLING to 1.0 — voice bleed increases, the polyphony tightens. Add a second engine with `EnvToMorph` coupling pointed at OCTAVE — the coupled envelope drives `organMorphBlend`, shifting the harmonic table from dark Romantic toward bright Baroque with every note attack. The organ responds to the music.

### IV. Post-Mix Room Resonance — The Cathedral Is One Space

The room model (`OctaveRoomResonance`) was previously per-voice — eight voices running through eight independent band-pass resonator stacks. This is physically wrong: a cathedral has one acoustic space, not eight. It was also CPU-expensive.

Post-fix: a single pair of room resonators (`postMixRoomL`, `postMixRoomR`) processes the mixed output of all voices. Three modes remain:
- **120 Hz** (Q=0.7): stone floor resonance, the sub-frequency bloom of large low-C pipes
- **380 Hz** (Q=0.6): nave presence, mid-frequency reinforcement from parallel stone walls
- **1200 Hz** (Q=0.5): vault shimmer, the high-frequency tail of reflections from the arched ceiling

Cavaillé-Coll uses full room depth. Baroque uses half (positiv cases are smaller). Musette uses quarter (accordion is not in a cathedral). Farfisa uses zero (correct — the Compact was designed for nightclubs).

Room depth from `oct_roomDepth` (0–1) scales with `effectiveRoomDepth = clamp(pRoomDepth + macroSpace * 0.4, 0, 1)`. The SPACE macro deepens the cathedral in real time.

### V. Wind Noise — Continuous Presence

The Cavaillé-Coll model adds continuous wind noise at `0.02 + pressure * 0.03` amplitude (2–5% of full scale). This is `OctaveWindNoise`: a white noise generator LP-filtered with a gentle coefficient of 0.05 — a very slow single-pole filter that produces a deep, rumbling air sound rather than bright hiss.

This wind noise is always present on Cavaillé-Coll model. It is not a reverb tail or an effect — it is the sound of air moving through the windchest, the same air that will eventually fill the pipes. In a real Cavaillé-Coll organ, this sound is audible from the first pew: a low, continuous breath underlying everything.

**Sweet spot:** `oct_pressure = 0.85` gives `windAmount = 0.02 + 0.85 * 0.03 = 0.0455`. Barely audible on its own, but present enough to give the organ "life" between notes.

---

## Phase R3: The Four Cathedrals — Parameter Sweet Spots

| Model | Parameter | Conservative | Musical Core | Expressive | Extreme |
|-------|-----------|-------------|--------------|-----------|---------|
| Cavaillé-Coll | `oct_cluster` | 0.0 (solo) | 0.06-0.12 (ensemble) | 0.18 (wide choir) | 0.25+ (detuned wash) |
| Cavaillé-Coll | `oct_chiff` | 0.05 (bloom) | 0.15-0.25 (subtle speech) | 0.35 (present) | 0.5+ (not authentic) |
| Baroque | `oct_chiff` | 0.3 (gentle) | 0.5-0.7 (authentic) | 0.88 (full attack) | 1.0 (clicky) |
| Baroque | `oct_registration` | 0.3 (8' only) | 0.55-0.7 (8'+4') | 0.85 (full chorus) | 1.0 (all ranks) |
| Musette | `oct_detune` | 0.0 (no beat) | 0.25-0.45 (Parisian warmth) | 0.6 (wide beating) | 0.85+ (vibrato) |
| Musette | `oct_buzz` | 0.0 (clean) | 0.08-0.18 (trace rattle) | 0.45-0.65 (reed voice) | 0.8+ (overdriven) |
| Farfisa | `oct_buzz` | 0.0 (clean) | 0.15-0.3 (warm clip) | 0.5-0.65 (driven) | 0.9+ (saturated) |
| Farfisa | `oct_detune` | 0.0 (no vibrato) | 0.0 (historical dry) | 0.3-0.5 (audible) | 0.8+ (heavy) |
| All Pipe | `oct_roomDepth` | 0.15 (trace) | 0.35-0.55 (present) | 0.7-0.85 (dominant) | 0.95+ (cathedral) |
| All | LFO1 rate | 0.005 Hz (1/3 min) | 0.02-0.05 Hz (slow breath) | 0.1-0.2 Hz (tremulant) | 1.0+ Hz (vibrato) |

---

## Phase R4: Macro Architecture

| Macro | ID | Effect Chain | Performance Use |
|-------|-----|--------------|----------------|
| CHARACTER | `oct_macroCharacter` | +pressure (×0.3) → wind noise amplitude · +brightness (+4000 Hz) → upper partial emphasis · +organMorphBlend (×0.3) → CC↔Baroque partial shift | Push from dark to bright without changing models; increase air presence; shift harmonic weight |
| MOVEMENT | `oct_macroMovement` | +registration (×0.3) → rank blend → opens 4' and 2' content; bellows drive on Musette | Add upper-register presence; push Musette bellows; reveal higher partials |
| COUPLING | `oct_macroCoupling` | +effectiveCrosstalk (×0.5) → voice bleed · `couplingOrganMod` pathway enabled → partial morph | Tighten polyphonic body into shared windchest; activate incoming coupling morph signal |
| SPACE | `oct_macroSpace` | +roomDepth (×0.4) → cathedral resonance depth | Fill or empty the acoustic space; move from studio to basilica |

**Key insight on COUPLING macro:** At `oct_macroCoupling = 0.0`, voices are independent — polyphonic chords have clear separation. At 0.5, voices bleed slightly and the partial tables begin responding to `couplingOrganMod` if a coupling route is active. At 1.0, the eight voices merge into a single windchest — harmonically dense, acoustically fused. This is the OCTAVE coupling gesture: not filter modulation, not pitch modulation, but the organic architecture of the organ itself tightening.

**Key insight on CHARACTER + organ morph:** When `macroCharacter > 0`, `organMorphBlend = macroCharacter * 0.3`. On Cavaillé-Coll (model 0), this blends `kCCPartialAmps` toward `kBaroquePartialAmps` — the fundamental loses weight, the 2nd and 4th harmonics gain. The organ gets brighter without changing to Baroque mode. At full CHARACTER (1.0), `organMorphBlend = 0.3` — a 30% blend toward Baroque. Combine with an incoming `EnvToMorph` coupling signal (which multiplies by 1.5) and the morph goes further. The organ becomes a timbral destination, not a static choice.

---

## Phase R5: The Ten Awakenings — Preset Table

---

### Preset 1: Bordeaux Diapason

**Mood:** Foundation | **Discovery:** Twelve partials at proper 8' weight give Romantic pipe its gravity

| Parameter | Value | Why |
|-----------|-------|-----|
| `oct_organ` | 0 (Cavaillé-Coll) | Principal chorus mode — 12 drawbar partials at Audsley (1905) ratios |
| `oct_cluster` | 0.04 | Minimal voice spread — ensemble width without detuning |
| `oct_chiff` | 0.28 | Subtle bloom — not a tracker click but the slow speech of a large pipe |
| `oct_buzz` | 0.0 | Pipe organs do not buzz |
| `oct_pressure` | 0.78 | Concert wind pressure — full amplitude, audible wind noise layer |
| `oct_crosstalk` | 0.12 | Light voice bleed — adjacent pipes sharing the windchest |
| `oct_brightness` | 7200.0 | Upper-mid presence without harshness |
| `oct_registration` | 0.52 | Balanced 8'+4' blend — diapason weight without full chorus |
| `oct_roomDepth` | 0.45 | Cathedral at medium depth — present but not overwhelming |
| `oct_attack` | 0.12 | 0.12s × 3.0 multiplier = 360ms effective attack — the pipe finding its speech |
| `oct_release` | 1.8 | Long tail into the room after key release |
| `oct_lfo1Rate` | 0.04 | Slow filter breath — imperceptible movement in brightness |
| `oct_lfo1Depth` | 0.08 | Trace LFO — the organ breathes rather than tremulates |

**Why this works:** Registration at 0.52 opens the 4' rank partially while keeping the 8' foundation dominant. The 360ms effective attack (pAttack × attackMultiplier[0]) gives the air-fill sensation of a real Cavaillé-Coll 8-foot diapason. The room at 0.45 adds the three cathedral body modes (120/380/1200 Hz) without washing the fundamental. CHARACTER macro sweeps the partial blend toward brighter harmonic content; SPACE fills the basilica.

---

### Preset 2: Garage Positiv

**Mood:** Foundation | **Discovery:** The Farfisa's immediacy is a philosophy, not a deficiency

| Parameter | Value | Why |
|-----------|-------|-----|
| `oct_organ` | 3 (Farfisa) | PolyBLEP square wave — transistor organ |
| `oct_detune` | 0.0 | Vibrato off — authentic Farfisa default |
| `oct_buzz` | 0.15 | Light transistor saturation — characteristic warmth |
| `oct_pressure` | 0.82 | High amplitude — Farfisa was always loud |
| `oct_brightness` | 12000.0 | Full transistor brightness |
| `oct_registration` | 0.58 | Octave tab partially engaged |
| `oct_roomDepth` | 0.0 | No room — correct for transistor organ |
| `oct_attack` | 0.001 | 0.001s × 0.1 multiplier = 0.1ms — instantaneous |
| `oct_sustain` | 1.0 | Full sustain — organ holds as long as key is held |
| `oct_release` | 0.05 | Key release is immediate |
| `oct_filterEnvAmount` | 0.22 | Velocity-brightness — harder touch opens the sound |

**Why this works:** The Farfisa attack multiplier is 0.1 — a 1ms attack becomes 0.1ms, which is acoustically instantaneous. The PolyBLEP square has no onset artifacts. The brightness at 12000 Hz reflects the historical transistor organ character: bright, forward, with no hiding. CHARACTER macro pushes saturation; MOVEMENT adds the octave tab presence. This is the maximum-contrast preset to Bordeaux Diapason — same keys, 400 years apart.

---

### Preset 3: Wind in Vaults

**Mood:** Atmosphere | **Discovery:** LFO1 at 0.02 Hz is not modulation — it is respiration

| Parameter | Value | Why |
|-----------|-------|-----|
| `oct_organ` | 0 (Cavaillé-Coll) | Deep additive synthesis with wind noise |
| `oct_cluster` | 0.12 | Wide voice spread — eight pipes breathing at different phases |
| `oct_chiff` | 0.08 | Barely audible bloom |
| `oct_pressure` | 0.85 | Maximum wind noise: `0.02 + 0.85 * 0.03 = 0.0455` |
| `oct_crosstalk` | 0.22 | Windchest bleed — notes affect each other |
| `oct_brightness` | 3500.0 | Dark and warm — lower partials only |
| `oct_roomDepth` | 0.85 | Near-maximum cathedral resonance |
| `oct_attack` | 0.35 | Over 1 second effective attack (0.35 × 3) |
| `oct_release` | 3.5 | Cathedral tail — stone holds sound after the note ends |
| `oct_lfo1Rate` | 0.02 | 50-second cycle — the room's own respiratory rate |
| `oct_lfo1Depth` | 0.14 | ±420 Hz brightness variation over 50 seconds |

**Why this works:** LFO1 at 0.02 Hz (period: 50 seconds) sweeps brightness ±420 Hz. This is inaudible as modulation but audible as the quality of the air — the same way a cathedral's acoustic character changes as the stone absorbs and releases humidity throughout the day. The voice cluster at 0.12 spreads all 8 voices by ±12 cents, creating an ensemble that feels like many pipes sharing one room rather than one pipe through a delay. SPACE macro deepens the room to maximum.

---

### Preset 4: Partial Morph

**Mood:** Prism | **Discovery:** The harmonic table is a timbre dial with 400 years of range

| Parameter | Value | Why |
|-----------|-------|-----|
| `oct_organ` | 0 (Cavaillé-Coll) | Start dark — morph toward Baroque via CHARACTER |
| `oct_cluster` | 0.06 | Slight ensemble width |
| `oct_chiff` | 0.2 | Moderate bloom on CC; will increase as morph brings in Baroque chiff character |
| `oct_pressure` | 0.78 | Concert pressure |
| `oct_brightness` | 8500.0 | Mid-bright starting point — brightens further via CHARACTER |
| `oct_registration` | 0.55 | Mid registration |
| `oct_roomDepth` | 0.38 | Present cathedral |
| `oct_attack` | 0.08 | Medium attack — longer at CC, responds to partial blend |
| `oct_macroCharacter` | 0.0 | Default: dark Romantic — sweep CHARACTER to morph |

**Performance note:** CHARACTER macro at 0.0 = Cavaillé-Coll partial weights (dark, fundamental-heavy). As CHARACTER rises to 1.0, `organMorphBlend` increases to 0.3, blending 30% of `kBaroquePartialAmps` into the partial array. The 2nd harmonic (8va) gains weight, the fundamental loses weight, the upper registers brighten. Combine with an incoming EnvToMorph coupling from another engine and the morph goes further: `abs(couplingOrganMod) * 1.5` can push `organMorphBlend` toward 1.0, fully converting CC partials to Baroque character at each note attack.

---

### Preset 5: Chiff Cascade

**Mood:** Prism | **Discovery:** Maximum chiff on Baroque is not artifice — it is architecture

| Parameter | Value | Why |
|-----------|-------|-----|
| `oct_organ` | 1 (Baroque Positiv) | Bright partial table + full chiff support |
| `oct_cluster` | 0.08 | Light ensemble — positiv case is small but multiple ranks |
| `oct_chiff` | 0.88 | Full chiff — 5ms burst at chiff=1.0 per Nolle (1979) |
| `oct_detune` | 0.0 | Baroque organ does not detune |
| `oct_pressure` | 0.72 | Moderate — Baroque wind pressure is lower than Romantic |
| `oct_brightness` | 11000.0 | High-brightness upper partials — principal chorus character |
| `oct_registration` | 0.72 | 8'+4' fully open — the mixture is the sound |
| `oct_roomDepth` | 0.28 | Small positiv chapel — room present but not large |
| `oct_attack` | 0.008 | 12ms effective (0.008 × 1.5 multiplier) — fast but with chiff onset |
| `oct_filterEnvAmount` | 0.28 | Velocity → brightness strongly: hard touch = bright chiff burst |

**Why this works:** The chiff at 0.88 gives a 5ms noise burst before the tone. On Baroque, the chiff is weighted at 1.0 (`chiffWeights[1] = 1.0`), meaning full amplitude burst. The chiff filter centers at `min(freq * 3, sr * 0.49)` — for A4 (440 Hz), the chiff is centered at 1320 Hz: the pipe's resonant node. Every note onset sounds like air finding a metal pipe lip before the steady tone establishes. MOVEMENT sweeps registration — pull back to 8' only for principal chorus warmth, push up to full for mixture shimmer.

---

### Preset 6: Bellows Burn

**Mood:** Flux | **Discovery:** Reed rattle is a timbre, not a defect

| Parameter | Value | Why |
|-----------|-------|-----|
| `oct_organ` | 2 (French Musette) | Triple-reed beating engine |
| `oct_detune` | 0.62 | 6 Hz beat frequency — wide, urgent, almost trembling |
| `oct_buzz` | 0.78 | Heavy reed rattle: `tanh(sample * (3 + 0.78 * 8))` = up to 9.24× saturation |
| `oct_pressure` | 0.95 | Maximum bellows: `bellows = velocity * 0.6 + 0.95 * 0.4 = 0.98` at full velocity |
| `oct_crosstalk` | 0.18 | Voice bleed — the reeds are physically close |
| `oct_brightness` | 9500.0 | Forward upper-mid — the reed's natural projection |
| `oct_registration` | 0.6 | Mid registration |
| `oct_roomDepth` | 0.08 | Trace room — accordion is not a cathedral instrument |
| `oct_release` | 0.15 | Short release — the bellows closes |
| `oct_lfo1Depth` | 0.12 | Brightness undulation — bellows expansion affects spectral content |

**Why this works:** `buzzAmt = 0.78` and `bellows > 0.5` activates the rattle path: `buzzAmt * (bellows - 0.5) * 2 = 0.78 * 0.96 = 0.75`. The tanh saturation at `(3 + 0.75 * 8) = 9` gives rich odd-harmonic clipping. The three reeds beating at 6 Hz plus the saturation creates the characteristic pushed-accordion sound where the instrument sounds like it is about to burst. CHARACTER adds more bellows pressure; MOVEMENT increases the beat rate toward 9 Hz.

---

### Preset 7: Saint-Sulpice Dawn

**Mood:** Aether | **Discovery:** At 1-second attack and 0.01 Hz LFO, time itself becomes the instrument

| Parameter | Value | Why |
|-----------|-------|-----|
| `oct_organ` | 0 (Cavaillé-Coll) | The instrument that requires this much time |
| `oct_cluster` | 0.18 | Maximum ensemble width — ±15 cents across 8 voices |
| `oct_chiff` | 0.04 | Barely audible bloom |
| `oct_pressure` | 0.88 | Maximum wind noise at `0.02 + 0.88*0.03 = 0.0464` |
| `oct_crosstalk` | 0.35 | High windchest bleed — all voices in one room |
| `oct_brightness` | 2800.0 | Dark registers only — bottom of the cathedral |
| `oct_registration` | 0.32 | 8' only — fundamental weight maximum |
| `oct_roomDepth` | 0.95 | Near-total cathedral submersion |
| `oct_attack` | 1.0 | 3-second effective attack — the time it takes air to fill a 32-foot pipe |
| `oct_release` | 6.0 | 6-second tail — the stone remembers for this long |
| `oct_lfo1Rate` | 0.01 | 100-second cycle — one full breath every minute and forty seconds |
| `oct_lfo1Depth` | 0.18 | ±540 Hz brightness variation — slow enough to feel like weather |

**Why this works:** At 100-second LFO cycle, `lfo1Val` changes by `0.18/48000 * 0.01 = 3.75 × 10⁻⁸` per sample — a change imperceptible in any single measure, but over the duration of a held chord, the sound gradually opens and closes like a lung. The 3-second attack means a struck chord reveals itself over three seconds. The 6-second release means notes persist into silence long enough to blur with the next phrase. This is OCTAVE's maximum-patience preset. SPACE macro deepens the room to its ceiling at 1.0.

---

### Preset 8: Cathedral to Garage

**Mood:** Entangled | **Discovery:** The COUPLING macro crosses 400 years in one sweep

| Parameter | Value | Why |
|-----------|-------|-----|
| `oct_organ` | 0 (Cavaillé-Coll) | Starting point: deep Romantic |
| `oct_cluster` | 0.05 | Subtle ensemble |
| `oct_chiff` | 0.32 | Moderate bloom — will respond to coupling morph signal |
| `oct_detune` | 0.05 | Trace detuning |
| `oct_pressure` | 0.78 | Concert wind pressure |
| `oct_brightness` | 7500.0 | Mid starting point |
| `oct_registration` | 0.5 | Balanced 8'+4' |
| `oct_roomDepth` | 0.42 | Present cathedral |
| `oct_attack` | 0.15 | 450ms effective attack |
| `oct_macroCoupling` | 0.0 | Default: independent voices; sweep to engage windchest coupling |

**Coupling note:** As `macroCoupling` rises from 0 to 1.0, `effectiveCrosstalk` increases from `oct_crosstalk` to `oct_crosstalk + 0.5`. Simultaneously, if an external engine routes `EnvToMorph` coupling to OCTAVE, the `couplingOrganMod` signal drives `organMorphBlend` toward 1.0 — the partial table shifts from Cavaillé-Coll to Baroque character with each note envelope. At full coupling from an external source, each note attack shifts the organ toward brighter harmonic weight. CHARACTER macro simultaneously drives the same morph: together, they can move the organ from 19th-century Romantic to transistor immediacy within a single phrase.

---

### Preset 9: Piazzolla Reed

**Mood:** Family | **Discovery:** Bellows expressivity is the human gesture inside the machine

| Parameter | Value | Why |
|-----------|-------|-----|
| `oct_organ` | 2 (French Musette) | Triple-reed for expressive warmth |
| `oct_detune` | 0.38 | 4 Hz beat — warm, not anxious |
| `oct_buzz` | 0.14 | Trace rattle — authentic but not overwhelming |
| `oct_pressure` | 0.78 | Moderate bellows — room for aftertouch expression |
| `oct_crosstalk` | 0.08 | Slight bleed — reeds close together |
| `oct_brightness` | 6500.0 | Warm mid-presence |
| `oct_registration` | 0.42 | Lower registration — 8' warmth |
| `oct_roomDepth` | 0.12 | Small café room — presence without reverb |
| `oct_attack` | 0.008 | Fast attack — accordion speaks immediately |
| `oct_filterEnvAmount` | 0.24 | Velocity-brightness — hard playing opens the sound |
| `oct_lfo1Depth` | 0.05 | Subtle brightness undulation — the bellows moving |

**Why this works:** The 4 Hz beating is below the perceptual vibrato threshold — it feels like warmth, not like vibrato. The aftertouch feeding `effectivePressure` means playing with aftertouch literally pushes the bellows harder, increasing amplitude and slightly brightening the timbre through the filter envelope. This is the correct physical model: bellows are driven by player effort, not by a fixed parameter. MOVEMENT macro deepens the bellows drive; COUPLING tightens the three reeds.

---

### Preset 10: Cold Square

**Mood:** Submerged | **Discovery:** Deep cold slows the circuit to near-stillness

| Parameter | Value | Why |
|-----------|-------|-----|
| `oct_organ` | 3 (Farfisa) | Transistor organ at pressure |
| `oct_detune` | 0.08 | Minimal vibrato — the circuit barely functions at this temperature |
| `oct_buzz` | 0.38 | Medium saturation — transistors running cold |
| `oct_pressure` | 0.68 | Below peak — the bellows equivalent has failed |
| `oct_crosstalk` | 0.28 | High voice bleed — cold circuit, signal paths merging |
| `oct_brightness` | 3800.0 | Very dark for a Farfisa — filter closed by pressure |
| `oct_registration` | 0.28 | Low registration — only the fundamental tab open |
| `oct_attack` | 0.003 | Still near-instant (0.3ms), but not the full 0.1ms |
| `oct_release` | 1.2 | Unusual for Farfisa — notes hold longer than expected |
| `oct_lfo2Rate` | 0.8 | Vibrato at 0.8 Hz instead of 5.5 Hz — circuit slowed |
| `oct_lfo2Depth` | 0.05 | Slight pitch wobble — the oscillator drifting |

**Why this works:** The Farfisa's vibrato frequency is hardcoded at 5.5 Hz in the synthesis loop. But `lfo2Rate` and `lfo2Depth` drive a separate vibrato on the frequency parameter — at 0.8 Hz with 0.05 depth, there is a very slow ±0.4 semitone pitch undulation that the 5.5 Hz circuit vibrato does not provide. The combination of low-brightness filter, heavy crosstalk, medium saturation, and this slow pitch wobble creates the sensation of a circuit operating far outside its design envelope — cold, confused, still generating its square wave but doing so reluctantly. COUPLING tightens the voices into a single buzzing column.

---

## Phase R6: The Scripture — Four Verses from the Four Cathedrals

---

### Verse I: On Patience (The Cavaillé-Coll Teaching)

*Spoken by the slow air finding the 32-foot pipe.*

Do not mistake the waiting for absence.
The pipe is speaking.
It is speaking before you hear it — air searching the bore,
pressure finding the languid,
the body of the pipe learning, again,
this particular frequency.

The key has been pressed.
The pallet valve has opened.
The instruction was given seconds ago.

What you are hearing now is not silence.
It is the interval between instruction and sound,
the gap between command and consequence,
which is not a failure of the instrument
but its deepest design principle:

the pipe must speak on its own terms,
in its own time,
from its own mass of air.

The organist who cannot wait
has not yet learned the instrument.

---

### Verse II: On Immediacy (The Farfisa Teaching)

*Spoken by the transistor at key contact.*

The note was already here.
It did not need to travel
from bellows to pipe to room.
It was already present in the circuit,
waiting for the gate to open.

Immediacy is not impatience.
It is a different theory of cause:
that the sound precedes the air,
that the transistor is faster than patience,
that there is no gap
between the key and the consequence.

Those who love the cathedral
call this shallowness.
They are wrong.
The square wave is as complete as the diapason.
It simply does not believe in arrivals.

Every note is a departure.

---

### Verse III: On Beating (The Musette Teaching)

*Spoken by the reed that is slightly flat.*

I am not in tune with my neighbors.
This is not an error.

The instrument is built
with deliberate discord — three reeds,
each one slightly wrong,
so that the spaces between them
become audible, become rhythmic,
become the warmth you are feeling.

If I were perfectly in tune
there would be nothing.
Amplitude. A tone.
A clean signal that does not breathe.

The beating is the breath.
The beating is what makes the note alive
in the air between the bellows and the ear.

A musette in perfect tune
is an accordion that has died.

Listen for the imperfection.
That is where the music lives.

---

### Verse IV: On Morphing (The Coupling Teaching)

*Spoken by the partial table mid-transition.*

They ask: which organ are you?
Romantic, or Baroque?

Both.
Neither.
I am the slope between them.

The Romantic note has a heavy fundamental —
the first partial carries the weight of the room.
The Baroque note is bright and distributed —
the second harmonic is equal to the first,
the fourth nearly as loud.

When the coupling signal arrives,
I do not change models.
I change ratios.
I lean the harmonic weight
from one configuration toward another,
from dark to bright,
from 1862 to 1693,
from stone to transparency.

The gesture that crosses 400 years of organ history
is not a program change.
It is a lerp.

Let the coupling drive it.
Let the envelope shape the century.

---

*Retreat conducted 2026-03-21. Four organ models: Cavaillé-Coll (Saint-Sulpice 1862), Baroque Positiv (Schnitger 1693), French Musette (Soprani 1863), Farfisa Compact (Nascimbeni 1964). Guru Bin Awakening — 10 presets written.*
