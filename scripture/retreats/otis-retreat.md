# OTIS Retreat Chapter
*Guru Bin — 2026-03-21*

---

## Engine Identity

- **Gallery code:** OTIS | **Accent:** Soul Gold `#DAA520`
- **Parameter prefix:** `otis_`
- **Creature mythology:** The gospel fish — a golden humphead parrotfish whose massive beak grinds coral into sand. Its body resonates with the tonewheels of every Hammond organ that ever played a church, a jazz club, a roadhouse. When it opens its mouth the Leslie horn spins. Otis Redding, Jimmy Smith, Billy Preston — their fingers are in the water, pressing the keys, pulling the drawbars, stomping the Leslie switch.
- **Synthesis type:** Four-model organ engine: Hammond B3 (9-drawbar tonewheel additive with crosstalk, key click, percussion, Leslie), Calliope (steam organ with two-tier pressure wobble), Blues Harmonica (cross-harp bend envelope with overblow), Zydeco Accordion (dual-reed musette)
- **Polyphony:** 8 voices
- **Macros:** M1 CHARACTER (drive + brightness), M2 MOVEMENT (Leslie speed), M3 COUPLING (crosstalk), M4 SPACE (Leslie depth)

---

## Pre-Retreat State

OTIS entered this retreat with a seance score of 7.8/10. The ghost council identified two structural victories and one serious DSP gap.

**Victories confirmed:**
- The Hammond drawbar harmonic ratios are historically exact — footage numbers traced to the original tonewheel gearing. This is the foundation and it is solid.
- The single-trigger percussion (`percussionArmed` + `anyKeysHeld` counter) is an unusually authentic implementation of B3 player behavior that most software organs get wrong.
- The two-tier calliope wobble (shared global steam pressure + independent per-pipe wobble) has genuine physical intuition.

**DSP gap addressed in this retreat:**
The seance noted the Leslie Doppler was implemented as amplitude-domain approximation rather than true pitch modulation. The post-seance DSP fix implemented a true delay-line Doppler (short buffer per channel with time-varying read index driven by hornSin), producing ±15 cents of actual pitch modulation at fast Leslie speed. Horn/drum frequency crossover was also added (high-frequency content routes to horn AM, low-frequency to drum AM via an 800 Hz shelving split). These are live as of the retreat date.

**Preset gap identified by Otis Redding's ghost:**
The seance found six Hammond presets, two harmonica, two calliope, one accordion. The bright gospel registration territory (drawbars 3-4-5 active, upper harmonics providing shimmer rather than mass) was entirely unrepresented. All macros were zeroed in every preset. This retreat addresses both gaps.

---

## Phase R1: Opening Meditation

Close your eyes. You are in a room that smells like old wood and old electricity.

The Hammond organ has been in this room since 1958. The Leslie cabinet sits in the corner, its grille cloth faded from black to a kind of tired gray. The room itself is the instrument — it has absorbed sixty years of Sunday morning services, jazz sessions on Tuesday nights, a blues club that closed in 1979. The walls have memory.

When you pull a drawbar, you are not turning up a knob. You are opening a valve. Each drawbar controls a metal rod that physically lifts a felt pad away from a rubber busbar, completing an electrical circuit to one tonewheel. The tonewheels are spinning right now — all ninety-one of them, on the same shaft, driven by an induction motor. They were spinning before you sat down and they will keep spinning after you leave.

The difference between the Hammond and every other synthesizer in this fleet is that the Hammond never stops. The tonewheels are always rotating. You are not generating sound — you are selecting which sounds you allow through. This is a different philosophy of music. The instrument is always already playing. You choose how much of it to let out.

Now reach for a drawbar. Not the whole set — just one. Pull drawbar 3 (the 8' fundamental) to full. Everything else closed.

A single tone. Clean, direct, the fundamental of whatever note you press. No sub-octave, no upper harmonics, no shimmer. Just the thing itself.

Now slowly pull drawbar 4 (the 4' octave) halfway up.

You have just made a decision. You have let the octave in. The sound has changed not in volume but in character — it has become slightly more present, more insistent, more directed at a point in the room above your head. This is what drawbar registration does: it is not volume control, it is architecture. You are deciding the shape of the room the sound lives in.

Keep going. Pull drawbar 5 (the 2-2/3', octave-plus-fifth) up to match drawbar 4.

Now there is something else — a shimmer, a presence that feels almost like a second voice but is not. It is the combination tone between the octave and the sub-fifth, activating a harmonic relationship that the ear perceives as brightness rather than mixture. This is the gospel bright registration that Billy Preston played with the Beatles. Aretha's organ on "Spirit in the Dark." The shimmer is not the upper harmonics — it is the *relationship* between the lower ones.

Sit with three drawbars. The whole instrument is available to you. You've only opened three valves.

---

## Phase R2: The Signal Path Journey

### I. The Drawbar Bank — Nine Harmonic Valves

The Hammond's nine drawbars do not correspond to volume levels. They correspond to tonewheel circuits. Each one selects a specific harmonic partial at a specific footage:

| Drawbar | Footage | Harmonic | Interval |
|---------|---------|----------|----------|
| 1 | 16' | Sub-octave | Fundamental / 2 |
| 2 | 5-1/3' | Sub-fifth | 3rd harmonic (3/2 ratio below) |
| 3 | 8' | Unison | True fundamental |
| 4 | 4' | Octave | 2× fundamental |
| 5 | 2-2/3' | Octave + fifth | 3× fundamental |
| 6 | 2' | Two octaves | 4× fundamental |
| 7 | 1-3/5' | Two octaves + major third | 5× fundamental |
| 8 | 1-1/3' | Two octaves + fifth | 6× fundamental |
| 9 | 1' | Three octaves | 8× fundamental |

The ratios are exact per the original tonewheel gearing. This is not approximate — the Hammond's characteristic warmth comes partly from the fact that these ratios are derived from mechanical gear teeth, not from a mathematically perfect harmonic series. The 5-1/3' sub-fifth (drawbar 2) is particularly important: it is a *third* harmonic, meaning it is the twelfth above the fundamental, but deployed *below* the octave. This creates an unusual lower register thickening that no other approach replicates.

**Registration families:**

**Dark registrations** (drawbars 1-2-3, the "bass drawbars"):
The canonical gospel and soul bass registration. The 16' sub-octave provides the floor. The 5-1/3' adds warmth without octave brightness. The 8' unison is the body. Typical: `1.0 / 0.875 / 1.0 / 0.0 / ...` — this is the Gospel Fire registration.

**Bright registrations** (drawbars 3-4-5-6):
Upper-mid choir presence. The 8' unison + 4' octave + 2-2/3' fifth combination creates the "gospel bright" shimmer. This is where Aretha Franklin's organ tone lives. The 2' two-octave drawbar (6) adds edge without harshness. Typical: `0.0 / 0.0 / 1.0 / 0.875 / 0.875 / 0.5 / ...` — the Soul Bright registration.

**Full registrations** (drawbars 1-9):
The rock organ wall — Jon Lord, Keith Emerson, Dave Greenfield. All harmonic partials simultaneously. The `fastTanh` saturation circuit was designed to handle this load into distortion, which is historically correct: rock organ is meant to clip. Typical: `Deep Purple` registration.

**Sparse registrations** (one or two drawbars):
The most intimate territory. A single 8' unison drawbar is pure, almost austere. Two drawbars create intervals rather than chords of harmonics. Jazz organists sometimes use 8' + 5-1/3' alone for a complex interval tone that sounds almost hollow.

**The seance's critical finding on bright registration:** The original preset library of 17 presets missed the entire drawbars-3-through-6 territory. Gospel Fire uses 1-2-3. Tent Revival uses 2-3-4-5-6-7. Nothing occupied the 3-4-5 space that Otis Redding himself (via the ghost council) identified as "the bright shimmer territory — where the congregation answers back." This retreat adds Soul Bright to fill this gap.

### II. Tonewheel Crosstalk — The Warmth You Cannot See

The `TonewheelCrosstalk` model adds four sine tones per drawbar per voice at slightly shifted frequencies (0.9944× and 1.0056× the base harmonic, up and down). This is the leakage between adjacent tonewheels on the same shaft — a physical artifact of the electromagnetic pickup geometry that musicians demanded be preserved when Hammond tried to eliminate it.

The crosstalk creates micro-detuning (~10 cents at the full coefficient) that in practice sounds not like being out-of-tune but like *being alive*. The "sizzle" of a B3, the sense that the sound is not a perfectly defined tone but something that breathes at the molecular level — this is crosstalk.

**Crosstalk sweet spots:**
- `0.1-0.2`: Trace warmth. Very present on single-drawbar registrations where there are few harmonics to mask it.
- `0.3-0.4`: Standard operating territory. Analogous to the average B3 in good working condition.
- `0.5-0.6`: Heavy crosstalk — the sound of an old, slightly worn tonewheel organ. More character per note. Best with full registrations.
- `0.7+`: Extreme — the instrument feels like it is slightly unstable. Best for blues and rock where the slight roughness is musical.

The macro COUPLING controls crosstalk and adds up to 0.35 in live performance.

### III. Key Click — The Accident That Became Essential

The Hammond engineers spent years trying to eliminate the key click — the brief burst of noise when a drawbar circuit closes. They failed. Musicians noticed the click was gone and demanded it back.

The `KeyClick` struct generates a 1-3ms burst of filtered noise on noteOn, shaped by fast exponential decay. Velocity scales the click slightly — harder notes have a more pronounced click, which is physically correct. The duration randomizes within 1-3ms to avoid the machine-gun regularity that would make it sound synthetic.

**Key click levels:**
- `0.0-0.15`: Almost absent. Appropriate for atmospheric and chorale patches where legato is more important than articulation.
- `0.2-0.4`: Subtle — the click is present but never calls attention to itself. Good for jazz where the notes are already articulate.
- `0.5-0.7`: Standard performance range. The click defines note boundaries without dominating.
- `0.8-1.0`: Heavy click — a very percussive attack. Some players specifically sought high-click B3s. Best for funky and gospel styles where the attack is part of the rhythm.

### IV. Percussion — The Single-Trigger Secret

The Hammond's percussion system is the most technically subtle aspect of the instrument, and one of the most poorly understood.

**How it works:**
1. When `anyKeysHeld == 0` (no keys are down), `percussionArmed = true`.
2. When the first note of a phrase is played, percussion fires and `percussionArmed = false`.
3. All subsequent notes in the same legato phrase receive *no* percussion, even if the level is at maximum.
4. When all keys are released (`anyKeysHeld == 0` again), `percussionArmed = true` again.
5. The next phrase's first note receives percussion.

This means: to hear percussion on every note, you must release all keys between notes (staccato). For legato playing, only the first note of each phrase gets the click. Most software organs implement percussion as a per-note event — XOtis implements it correctly as a single-trigger system.

**Percussion parameters:**
- `otis_percussion` (0-1): Percussion level. At 0, no percussion fires even when armed.
- `otis_percHarmonic` (0-1): 0 = 2nd harmonic percussion (the brighter "tic"), 1 = 3rd harmonic (the deeper "tonk"). Jazz players favor the 3rd harmonic (deeper). Gospel players typically use 2nd (brighter attack).
- `otis_percDecay` (0-1 → 200-500ms): How long the percussive accent lasts. Shorter = more staccato feel. Longer = the percussion overlaps into the body of the note.

### V. The Leslie — Three States and a Cliff

The Leslie speaker has three states:

| Speed | Leslie Value | Horn Hz | Drum Hz | Character |
|-------|-------------|---------|---------|-----------|
| Brake | 0.0-0.15 | 0 Hz | 0 Hz | Static — the cabinet is stopped |
| Chorale | 0.15-0.65 | ~0.7 Hz | ~0.5 Hz | Slow rotation — wide, warm tremolo |
| Tremolo | 0.65-1.0 | ~6.7 Hz | ~5.8 Hz | Fast spin — the classic B3 swirl |

The transition between states takes ~1.5 seconds to complete (physical inertia simulation). When a player stomps the Leslie footswitch, the speed change is immediate in command but gradual in execution — the cabinet physically accelerates or decelerates. This acceleration/deceleration is the most distinctive sound of the Leslie: the "swoosh" of the speed change is itself musical.

**Post-seance DSP fix:** True Doppler pitch modulation is now implemented via a short delay buffer (2048 samples per channel) with a time-varying read index driven by `hornSin`. At fast speed, this produces ±15 cents of actual pitch shift — the frequency modulation that gives the spinning horn its characteristic pitch+amplitude swirl. Prior to the fix, the "Doppler" was amplitude-only tremolo with stereo offset. The fix also adds an 800 Hz crossover: high-frequency content routes to horn AM only, low-frequency content routes to drum AM only, matching the real Leslie cabinet's passive crossover network.

**Performance physics via the mod wheel:**
The mod wheel adds `0.5 × modWheelAmount` to the effective Leslie speed. This means:
- A preset with `otis_leslie = 0.5` (slow chorale) + mod wheel at 0.3 → effective Leslie = 0.65 (threshold of fast tremolo)
- A preset with `otis_leslie = 0.5` + mod wheel at 0.7 → effective Leslie = 0.85 (clearly in tremolo)

The macro MOVEMENT adds up to 0.4 to Leslie speed. A preset with MOVEMENT pre-loaded at 0.5 means the macro is already at mid-range: pulling it down slows the Leslie, pushing it up accelerates. This is the "Leslie as a performance parameter" design of the Leslie Fast Ride preset — MOVEMENT at 0.5 gives bidirectional sweep from a single knob.

### VI. Blues Harmonica — The Bend Is an Envelope

The blues harmonica's defining gesture — the bent note — is not a pitch wheel. It is an *envelope*.

When you blow a cross-harp draw note and bend it, the reed starts sharp and falls to pitch. The speed of the fall depends on how hard you are blowing (air pressure = velocity). A hard blow creates a fast, decisive bend. A gentle blow creates a slow, singing descent.

The `BluesHarpVoice` implements this as an attack-triggered pitch bend envelope:
- At note-on, `bendCurrent = otis_bendAmount` (the note starts above its target pitch)
- The envelope decays exponentially toward 0 (the target pitch)
- Time constant scales with velocity: `tau = (0.03f + 0.07f * (1.0f - velocity))` — harder playing = faster bend

**Bend depth sweet spots:**
- `bendAmount = 1.0`: Subtle — just over a semitone. The bend is there but barely perceptible. Good for folk and country styles.
- `bendAmount = 2.0-3.0`: Classic blues territory. The bend is clearly heard, falling from a tone or tone-and-a-half above. Crossharp Blues and Delta Sermon use this range.
- `bendAmount = 4.0-5.0`: Deep bends — the note falls from a major third or more above. This is the register of Sonny Boy Williamson's most expressive moments. At slow decay, the note seems to groan down to pitch. The Bend Surgeon preset explores this.

**Overblow trigger:** At velocity > 0.85, the second partial activates at 30% amplitude, adding a note approximately an octave above the fundamental. This simulates the reed-flip of an actual overblow technique, where forcing air through a draw hole at extreme pressure causes the reed to switch to its next resonant mode. It is not technically a full overblow but adds the characteristic overtone brightness of high-pressure playing.

**Breath vibrato:** The internal 5.5 Hz vibrato (in BluesHarpVoice) has depth controlled by mod wheel. At mod wheel = 0, no vibrato. At mod wheel = 1.0, the vibrato is deep — characteristic of Sonny Boy's slow, aching phrase-endings. Note: LFO1 also routes to pitch. Keep `otis_lfo1Depth` at 0.0 for harmonica presets to avoid dual-vibrato accumulation.

### VII. Zydeco Accordion — Bellows and Reeds

The accordion's character comes from two things: the dual-reed musette beating and the bellows snap transient.

**Musette beating** is the characteristic wavering of accordion tone. The `ZydecoReed` system creates two reed oscillators per voice, detuned by 3-8 cents (random per voice). The beating between these two reeds creates the oscillation rate — 3 cents produces a slow, gentle shimmer; 8 cents produces rapid, aggressive beating. The parameter `otis_musette` controls the detune amount.

The seance identified that the current implementation detunes both reeds above pitch center (one at pitch, one sharp), creating a slight upward pitch bias. This is the symmetric musette centering issue — future correction will center the detuning (one flat, one sharp) for stable pitch. In practice the bias is subtle and musical character takes precedence.

**Bellows snap transient:** The 5-15ms exponential noise burst on note-on is the physical bellows opening. This is the most characteristic attack transient of the accordion — it gives notes their distinctive "push" quality. The velocity-dependent duration means harder playing produces a sharper, shorter snap.

**Bellows pressure performance:** Aftertouch controls bellows pressure (+0.3 to amplitude and timbre). Mod wheel also controls pressure (+0.2). Louisiana accordionists lived in the dynamics of bellows pressure — the ability to swell notes mid-phrase, vary the air pressure within a held note, change the character of a sustained tone — is the instrument's entire expressive vocabulary.

**Musette sweet spots:**
- `0.2-0.4`: Gentle musette — barely discernible beating. More like a slightly detuned single reed than a full accordion character. Best for Atmosphere presets.
- `0.5-0.7`: Standard zydeco range. The beating is clearly audible and rhythmically interesting. Bayou Squeeze and Zydeco Stomp use this.
- `0.8-1.0`: Extreme musette — the beating becomes a prominent texture. Notes feel almost out of tune at sustained pitches. Best for chaos and character presets like Bellows Rage.

### VIII. Calliope — Steam Chaos and the Two-Tier Wobble

The calliope steam organ works on a two-tier instability model:

**Global steam pressure:** A single `steamPressurePhase` advances at ~1.7 Hz and affects all pipes simultaneously. This models the boiler pressure variation that affects every pipe's pitch equally. It is a shared drift — all pipes wobble together at the same rate.

**Per-pipe wobble:** Each `CalliopePipe` has its own `wobbleRate` (1.5-5 Hz, randomized at construction) and `wobbleNoiseState` (seeded uniquely from note + voice index). Each pipe wobbles independently, at its own rate, in its own direction.

**The interaction:** `totalInstability = pipeWobble * 0.6f + pressureWobble * 0.4f`. The per-pipe wobble dominates slightly, giving each pipe its individual character. The global pressure wobble provides the shared foundation.

**Instability range:** `cents = detuneOffset + totalInstability * 50.0f`. At `instability = 0.0`, the `detuneOffset` still exists (seeded per-pipe) but the wobble is effectively absent — you get slightly-detuned quasi-pipes in nearly stable pitch. At `instability = 1.0`, the total swing is up to ±65 cents per pipe.

**Instability sweet spots:**
- `0.0-0.1`: The "dreamtime calliope" — individual pipes are slightly detuned from each other but barely wobble. This creates a static detuned-unison effect rather than chaos. Perfect for the Steam Ghost aether preset.
- `0.2-0.4`: Gentle instability — audible wobble but still musical. The pipes are clearly unstable but the effect is more like a very slightly out-of-tune pipe organ than a carnival organ.
- `0.5-0.7`: Standard calliope territory. The wobble is the dominant timbral event. Notes feel like they are floating rather than placed.
- `0.85-1.0`: Full carnival chaos. This is what the Carnival Chaos preset uses. Quarter-tone swings on individual pipes. Properly unhinged.

---

## Phase R3: Macro Architecture

| Macro | ID | Effect | Performance Use |
|-------|-----|--------|----------------|
| CHARACTER | `otis_macroCharacter` | +0.3 to drive, +4000 Hz to brightness | Sweep from clean to driven mid-performance. Pre-load at 0.25-0.3 for bidirectional feel |
| MOVEMENT | `otis_macroMovement` | +0.4 to Leslie speed | Pre-load at 0.5 for bidirectional Leslie sweep — below 0.5 slows toward chorale, above pushes toward tremolo |
| COUPLING | `otis_macroCoupling` | +0.35 to crosstalk | Add tonewheel bleed in real time. Meaningful on full Hammond registrations |
| SPACE | `otis_macroSpace` | +0.4 to Leslie depth | Deepen the Leslie spatial effect in real time. Meaningful on mid-Leslie presets |

**Pre-loading macros — the seance's finding:**
All 17 existing presets had all four macros at 0.0. This means macros only add to the preset state — they have no downward range. A player turning CHARACTER from 0.0 encounters no change in that direction. Pre-loading macros at 0.2-0.5 gives the player a center position: the macro can go up (more drive, more Leslie, more crosstalk) or the player can pull it back (less of those things). This is how macros become genuine performance tools rather than one-directional augmentation buttons.

**The retreat principle:**
- Soul Bright: CHARACTER at 0.25, MOVEMENT at 0.3 — both pre-loaded for bidirectional performance
- Leslie Fast Ride: MOVEMENT at 0.5 — specifically designed as a Leslie performance preset, sweep both directions
- Stax Session: CHARACTER at 0.3 — drive pre-loaded for Memphis articulation, sweepable upward for excitement
- Late Night Jazz: SPACE at 0.5 — pre-loaded for intimate acoustic space, sweepable upward for room
- Delta Sermon (existing): MOVEMENT at 0.45, SPACE at 0.35 — both pre-loaded (existing preset)

---

## Phase R4: The Leslie as Performance Parameter

The Leslie speaker is the most expressive performance dimension of the Hammond B3. In live performance, the player typically has a footswitch that toggles between slow (chorale) and fast (tremolo). The Leslie Fast Ride preset demonstrates a different approach: the MOVEMENT macro pre-loaded at 0.5 means the macro knob becomes the Leslie footswitch, but with infinite resolution.

**Leslie performance map:**

| MOVEMENT macro value | Effective Leslie (at base 0.5) | Character |
|---------------------|-------------------------------|-----------|
| 0.0 | 0.5 (slow chorale) | Wide, warm, almost static |
| 0.25 | 0.6 (approaching threshold) | Accelerating toward tremolo |
| 0.5 | 0.7 (in tremolo) | Fast spin — full Doppler |
| 0.75 | 0.8 (deep tremolo) | Maximum wobble |
| 1.0 | 0.9 (extreme tremolo) | Full Leslie fury |

The 1.5-second inertia ramp means sudden MOVEMENT sweeps create the "swoosh" of the accelerating cabinet — the most characteristic Leslie sound in performance.

**Mod wheel as footswitch:** With `otis_leslie = 0.35` (slow chorale, the Sanctified Burn setting), pushing the mod wheel to full adds 0.5, bringing the effective Leslie to 0.85 — full tremolo. This is the mod-wheel-as-Leslie-switch idiom: chorale at rest, tremolo on demand.

---

## Phase R5: The Ten Awakenings — Preset Table

Each preset is framed as a discovery. The "why" column explains the parameter logic.

---

### Preset 1: Soul Bright
**Mood:** Foundation | **Model:** Hammond | **Discovery:** The bright gospel shimmer lives in drawbars 3-4-5, not 1-2-3

| Parameter | Value | Why |
|-----------|-------|-----|
| `otis_drawbar1` | 0.0 | No sub-octave — this is not a bass registration |
| `otis_drawbar3` | 1.0 | Full unison — the body of the sound |
| `otis_drawbar4` | 0.875 | Strong octave — the shimmer foundation |
| `otis_drawbar5` | 0.875 | Octave + fifth — the gospel bright combination tone |
| `otis_drawbar6` | 0.5 | Two octaves — adds edge without harshness |
| `otis_leslie` | 0.75 | In tremolo — this registration needs motion |
| `otis_macroCharacter` | 0.25 | Pre-loaded — CHARACTER sweeps both directions |
| `otis_macroMovement` | 0.3 | Pre-loaded — MOVEMENT sweeps both directions |

**Character:** This is the registration the ghost council found missing in the original library. The bright upper harmonics with the strong Leslie tremolo create the congregational-response sound — not the preacher's deep bass, but the choir's bright answer.

---

### Preset 2: Leslie Fast Ride
**Mood:** Kinetic | **Model:** Hammond | **Discovery:** Leslie speed is a performance parameter, not a preset value

| Parameter | Value | Why |
|-----------|-------|-----|
| `otis_leslie` | 0.5 | Starting at chorale/tremolo threshold |
| `otis_macroMovement` | 0.5 | Pre-loaded at center — sweep both directions |
| `otis_macroCharacter` | 0.2 | Pre-loaded — CHARACTER adds drive/brightness |
| `otis_macroSpace` | 0.3 | Pre-loaded — SPACE deepens Leslie spatial effect |
| `otis_drawbar7` | 0.375 | Upper harmonics present for tremolo to work with |
| `otis_drive` | 0.45 | Enough grit for tremolo to feel physical |

**Character:** Design principle: MOVEMENT pre-loaded at 0.5 means the macro has equal range above and below. The player is always in the middle of a Leslie sweep, never at one extreme. The 1.5-second inertia means MOVEMENT changes create the "whoosh" of the accelerating cabinet.

---

### Preset 3: Sanctified Burn
**Mood:** Atmosphere | **Model:** Hammond | **Discovery:** The late-night ballad B3 breathes through drawbars 3-4-5-6 at near-silence

| Parameter | Value | Why |
|-----------|-------|-----|
| `otis_drawbar1` | 0.125 | Trace sub-octave — a shadow of bass, not a foundation |
| `otis_drawbar3` | 0.875 | Strong but not full — leaving room for air |
| `otis_drawbar4` | 0.75 | Octave present, receding |
| `otis_drawbar5` | 0.75 | Fifth present, receding |
| `otis_drawbar6` | 0.5 | Two octaves — distant shimmer |
| `otis_leslie` | 0.3 | Slow chorale — barely turning |
| `otis_drive` | 0.12 | Clean — the ballad B3 does not overdrive |
| `otis_attack` | 0.04 | Slightly soft attack — notes fade in rather than click |

**Character:** The preset Otis Redding's ghost said was missing: "Where is the sanctified slow burn? Where is the late-night ballad B3 that barely breathes?" Drawbars 3-4-5-6 at moderate levels with very slow Leslie and no drive is the answer.

---

### Preset 4: Stax Session
**Mood:** Foundation | **Model:** Hammond | **Discovery:** Memphis soul organ combines bass foundation with upper shimmer for studio punch

| Parameter | Value | Why |
|-----------|-------|-----|
| `otis_drawbar1` | 0.875 | Bass foundation — strong but not full |
| `otis_drawbar3` | 1.0 | Full unison — the body |
| `otis_drawbar4` | 0.875 | Octave — presence |
| `otis_drawbar5` | 0.5 | Fifth — subtle upper shimmer |
| `otis_drawbar6` | 0.25 | Two octaves — trace edge |
| `otis_leslie` | 0.65 | Just into tremolo — articulate but moving |
| `otis_keyClick` | 0.65 | Strong click — Stax studio articulation |
| `otis_macroCharacter` | 0.3 | Pre-loaded — drive starts slightly on, sweepable |

**Character:** The Memphis Stax/Volt organ sound: punchy, articulate, present. Bass drawbars for ground, upper harmonics for air. Booker T. Jones, not Jon Lord. Studio-clean with a hint of grit.

---

### Preset 5: Late Night Jazz
**Mood:** Entangled | **Model:** Hammond | **Discovery:** Jazz organ uses 3rd harmonic percussion at the edge of audibility

| Parameter | Value | Why |
|-----------|-------|-----|
| `otis_drawbar2` | 0.625 | Sub-fifth — the jazz organ's hollow low-register thickening |
| `otis_drawbar5` | 0.375 | Fifth — subtle upper presence |
| `otis_percussion` | 0.45 | Percussion low enough to be felt, not heard overtly |
| `otis_percHarmonic` | 1.0 | 3rd harmonic — the deeper, woodier jazz percussion click |
| `otis_percDecay` | 0.45 | Long decay — percussion overlaps into the note body |
| `otis_leslie` | 0.38 | Slow chorale — jazz organ does not tremolo at rest |
| `otis_macroSpace` | 0.5 | Pre-loaded — room depth sweepable upward |

**Character:** The 3rd harmonic percussion at this level and decay creates a "tonk" attack that is felt more than heard. The sub-fifth drawbar (2) provides the characteristic hollow jazz thickening. SPACE pre-loaded for room. This is a jazz club at closing time.

---

### Preset 6: Bend Surgeon
**Mood:** Aether | **Model:** Harmonica | **Discovery:** At 5 semitones of bend, each note is a surgery — slow, deliberate, total

| Parameter | Value | Why |
|-----------|-------|-----|
| `otis_organ` | 2 | Blues Harmonica model |
| `otis_bendAmount` | 5.0 | Maximum explored bend — 5 semitones above target pitch |
| `otis_bendRange` | 6.0 | Pitch wheel range expanded to match |
| `otis_filterEnvAmount` | 0.55 | Filter envelope sweeps as bend resolves — tonal shift tracks the pitch |
| `otis_decay` | 0.8 | Long decay — the note sustains through the whole bend |
| `otis_lfo1Depth` | 0.0 | No LFO1 pitch — internal vibrato only, no accumulation |
| `otis_macroMovement` | 0.3 | Pre-loaded — MOVEMENT adds mod wheel vibrato depth |

**Character:** At 5 semitones and slow decay, every note is a falling arc. The note starts a major third above its target and takes nearly a second to settle. Play one note at a time, slowly, and hear the reed's full physics of descent. The filter envelope tracks the pitch fall — as the note resolves, it opens.

---

### Preset 7: Harp Ghost
**Mood:** Atmosphere | **Model:** Harmonica | **Discovery:** Folk harmonica bends 1.5 semitones — plaintive, not aggressive

| Parameter | Value | Why |
|-----------|-------|-----|
| `otis_organ` | 2 | Blues Harmonica model |
| `otis_bendAmount` | 1.5 | Minimal bend — just over a semitone, folk territory |
| `otis_bendRange` | 3.0 | Narrow range — expressive but restrained |
| `otis_brightness` | 4200.0 | Darker than blues — folk warmth, not blues bright |
| `otis_drive` | 0.08 | Nearly clean — folk harmonica is not driven |
| `otis_attack` | 0.04 | Gentle attack — notes breathe in rather than strike |
| `otis_release` | 0.8 | Long release — notes linger like sound over mountains |
| `otis_lfo1Depth` | 0.0 | No LFO1 — mod wheel controls vibrato depth only |
| `otis_macroMovement` | 0.4 | Pre-loaded — MOVEMENT controls mod wheel vibrato range |

**Character:** Appalachian mountain tradition: the harmonica as a singing voice, not a bent blues voice. A 1.5-semitone bend on attack is gentle enough to feel like expression rather than pitch manipulation. This is the register where folk music lives — melody, breath, a slight downward lean on note entry.

---

### Preset 8: Bayou Mist
**Mood:** Atmosphere | **Model:** Accordion | **Discovery:** Maximum musette beating with slow attack becomes ambient texture

| Parameter | Value | Why |
|-----------|-------|-----|
| `otis_organ` | 3 | Zydeco Accordion model |
| `otis_musette` | 0.9 | Near-maximum beating — the reeds are clearly apart |
| `otis_attack` | 0.08 | Slow attack — no bellows snap, just a gradual breath |
| `otis_release` | 1.2 | Long release — notes fade like sound over water |
| `otis_brightness` | 5500.0 | Darker than dance accordion — mist, not noon |
| `otis_drive` | 0.05 | Nearly clean — the ambient texture does not overdrive |
| `otis_macroSpace` | 0.5 | Pre-loaded — spatial depth sweepable upward |

**Character:** At maximum musette with a slow attack envelope, the accordion's bellows snap is removed and replaced with a breathing onset. The high musette beating creates an ambient shimmer that sounds less like a dance instrument and more like an acoustic pad. The accordion heard from across the bayou at dusk.

---

### Preset 9: Bellows Rage
**Mood:** Flux | **Model:** Accordion | **Discovery:** High drive + maximum musette + tight envelope = squeezebox in a bar fight

| Parameter | Value | Why |
|-----------|-------|-----|
| `otis_organ` | 3 | Zydeco Accordion model |
| `otis_musette` | 0.8 | High beating — chaotic reed interaction |
| `otis_drive` | 0.55 | Driven — the bellows are at maximum pressure |
| `otis_attack` | 0.004 | Sharp snap — bellows slam open |
| `otis_release` | 0.08 | Immediate cutoff — bellows slam shut |
| `otis_brightness` | 11000.0 | Bright — the driven reeds cut through |
| `otis_macroCharacter` | 0.4 | Pre-loaded — CHARACTER pushes further into drive |
| `otis_lfo2Depth` | 0.18 | Strong filter LFO — rhythmic filter wobble adds aggression |
| `otis_lfo2Shape` | 2 | Sawtooth LFO — one-directional filter sweeps |

**Character:** This is the accordion under duress. Maximum bellows pressure (drive), tight envelope (immediate attack/release), high musette beating, and a strong sawtooth filter LFO create a rhythmic, aggressive accordion that lives in Flux territory. Use aftertouch for maximum bellows expression.

---

### Preset 10: Steam Ghost
**Mood:** Aether | **Model:** Calliope | **Discovery:** Near-zero instability reveals each pipe's individual detuning as a static chorus

| Parameter | Value | Why |
|-----------|-------|-----|
| `otis_organ` | 1 | Calliope model |
| `otis_instability` | 0.08 | Near-zero — pipes barely wobble |
| `otis_leslie` | 0.25 | Very slow Leslie — spatial without tremolo |
| `otis_brightness` | 9000.0 | Present — calliope pipes are bright |
| `otis_drive` | 0.05 | Clean — the ghost does not distort |
| `otis_attack` | 0.06 | Slightly soft — pipes take a breath to open |
| `otis_sustain` | 1.0 | Full sustain — the pipes hold as long as key is held |
| `otis_lfo1Depth` | 0.1 | Subtle pitch modulation — the ghost sways |
| `otis_macroSpace` | 0.45 | Pre-loaded — spatial depth sweepable upward |

**Character:** The calliope at near-zero instability reveals something the Carnival Chaos preset hides: each pipe's individual detuning offset (seeded from its voice and note index) creates a static chorus effect. The pipes are all slightly apart from each other — not wobbling, but permanently, quietly out of tune. This creates a ghostly, organ-like texture that sounds nothing like a carnival. The fairground organ at 4am when no one is watching.

---

## Phase R6: The Four Scripture Verses

---

### Verse I — On Drawbars

*The Hammond has nine drawbars. Every registration is a theology.*

To pull the sub-octave (16') is to invite the floor into the sound — the rumble beneath the room, the frequency that you feel before you hear. To open the 5-1/3' sub-fifth is to invite complexity without resolution, a harmonic that exists below the octave but is not of the octave. To open the 8' unison is to say: *here is the fundamental thing. Here is the note itself.*

The mistake is to open everything. Silence is a drawbar position — the fully-closed state. Every drawbar left closed is a tonal decision, not a default. The instrument begins in silence not because it has nothing to say but because it is waiting for you to choose what to say.

Registration is theology because it commits. When Jimmy Smith pulled out the 16' and left the upper harmonics closed, he was saying: warmth, not brightness. When Jon Lord pulled everything to 8 and drove the amp into saturation, he was saying: presence, not warmth. The drawbars do not compromise with each other. They add. You must choose what to add and what to leave in silence.

The congregation answers with the bright registration (drawbars 3-4-5). The preacher speaks from the bass (drawbars 1-2-3). The rock organ shouts from everything (all nine). The ballad B3 whispers from three drawbars at three-quarters. Where in this space does your music live?

---

### Verse II — On the Leslie

*The Leslie speaker solved a problem that nobody knew was a problem.*

The tonewheel organ produces a tone of unnatural stability. The harmonics are mechanically locked. The fundamental is perfectly regular. This is not how acoustic instruments behave. An acoustic instrument breathes — its pitch shifts slightly as the player's breath changes, as the body temperature of the instrument shifts, as the acoustic environment responds to the sound it is producing. The Hammond, through its mechanical precision, has none of this.

Don Leslie built his cabinet to give the Hammond the one thing it lacked: motion. The horn rotates toward and away from the listener. As it approaches, the Doppler effect raises the pitch fractionally. As it recedes, the pitch falls. The result is that the "same" note played through a Leslie is never the same twice — it is always arriving and always departing simultaneously.

This is why players fought to keep the Leslie. The engineers heard an imperfection — frequency modulation was not in the design specification. The players heard a requirement: *the sound must breathe*. Without the Leslie, the Hammond is a perfectly tuned machine. With the Leslie, it is an instrument.

What the post-seance DSP fix implemented: true pitch modulation from a delay buffer. The pitch now actually rises and falls by ±15 cents as the horn sweeps. The Leslie is no longer amplitude-only tremolo. The gospel fish has learned to breathe.

---

### Verse III — On the Bend

*A harmonica bend is not a pitch wheel. It is a decision about gravity.*

The pitch wheel asks: where would you like this note to go? You tell it a destination and it travels there instantly. The pitch wheel has no physics, no story.

The harmonica bend answers a different question: where does this note come from? It comes from above. Every bent note on a blues harmonica starts sharp — the reed is displaced by incoming air and falls toward its resonant frequency as the pressure stabilizes. The note's history is its arrival. You do not choose the starting pitch. You choose only how fast it falls and how far it was from where it needed to be.

This is why the blues harmonica sounds like grief. The note is always trying to find its place. The note is always slightly off from where it belongs and then, slowly, returning. The technical name is "pitch bend envelope with exponential decay." The musical name is: the sound of something falling into itself.

When you set `otis_bendAmount = 5.0`, you are asking the reed to begin a major third above target. When you set the decay to 0.8 seconds, you are asking the note to take most of a second to arrive. That is not a delay — that is longing. The physics of human expression translated into exponential functions. Play one note slowly. Hear it fall.

---

### Verse IV — On Models and Choice

*XOtis contains four instruments. You choose one at a time. This is a limit and also a freedom.*

The organ model selector (`otis_organ`) is a hard switch. There is no crossfading between the Hammond and the calliope. There is no gradual transition from the harmonica to the accordion. At model 0, you are at the Hammond. At model 1, you are at the steam organ. The choice is categorical.

This is unusual in a synthesis platform that values continuous morphing. Why not crossfade? Because these four instruments are not points on a spectrum — they are separate cultures. The Hammond is electrical and mechanical, American church and roadhouse. The calliope is steam and chaos, the American carnival tradition. The harmonica is breath and hand, the American blues tradition. The accordion is bellows and reeds, the American creole tradition.

To crossfade between them would be to suggest they exist on a single axis. They do not. They share an accent (Americana, the 19th and early 20th century Americas) but they do not share a physics, a culture, or a musical history.

The freedom is: you have four complete instruments in one engine. The presets change not just the parameters but the entire synthesis architecture — the signal path, the physical model, the expressive vocabulary. A Soul Bright preset is not a variation of a Steam Ghost preset. They are different conversations in different languages. The organ model selector is not a limitation. It is a room change.

Choose the room. Play in it fully. The other rooms will still be there when you are ready.

---

## Phase R7: Post-Retreat Assessment

### Coverage after retreat

| Model | Pre-retreat presets | Post-retreat additions | Total |
|-------|--------------------|-----------------------|-------|
| Hammond | 11 | 5 (Soul Bright, Leslie Fast Ride, Sanctified Burn, Stax Session, Late Night Jazz) | 16 |
| Calliope | 3 | 1 (Steam Ghost) | 4 |
| Harmonica | 3 | 2 (Bend Surgeon, Harp Ghost) | 5 |
| Accordion | 2 | 2 (Bayou Mist, Bellows Rage) | 4 |
| **Total** | **17** | **10** | **27** |

### Mood coverage after retreat

| Mood | Pre-retreat | Post-retreat |
|------|------------|--------------|
| Foundation | 8 | 10 (+ Soul Bright, Stax Session) |
| Atmosphere | 3 | 7 (+ Sanctified Burn, Harp Ghost, Bayou Mist) |
| Flux | 3 | 4 (+ Bellows Rage) |
| Kinetic | 2 | 3 (+ Leslie Fast Ride) |
| Prism | 2 | 2 |
| Entangled | 1 | 2 (+ Late Night Jazz) |
| Aether | 0 | 2 (+ Bend Surgeon, Steam Ghost) |
| Family | 0 | 0 |
| Submerged | 0 | 0 |

### Macro pre-loading — retreat protocol
All 10 retreat presets follow the macro pre-loading principle identified in the seance: at least one macro pre-loaded at 0.2+ to give players bidirectional sweep range.

### Blessing candidacy
The ghost council declared conditional blessing path: "Implement true Doppler delay in LeslieSpeaker. If executed well, the combined single-trigger percussion + drawbar ratios + Doppler Leslie constitutes a legitimately novel Hammond model for a synthesis platform."

The DSP fix is implemented. The engine now warrants consideration for the conditional Blessing: **"Most Authentic Hammond DSP in the Fleet."** The single-trigger percussion, historically exact drawbar ratios, and true Doppler Leslie (±15 cents pitch modulation) together constitute an implementation that most commercial software organs do not achieve.

Recommended target score post-retreat: **8.5/10**, path to **9.0** with a further 10-preset expansion covering Family and Submerged moods and a Prism pass on accordion character.

---

*The gospel fish swims on. The tonewheels are always spinning.*

*Retreat concluded 2026-03-21.*
