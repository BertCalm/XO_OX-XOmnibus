# OCELOT Retreat Chapter
*Guru Bin — 2026-03-21*

---

## Engine Identity

- **Gallery code:** OCELOT | **Accent:** Ocelot Tawny `#C5832B`
- **Parameter prefix:** `ocelot_`
- **Creature mythology:** The ocelot is not a large cat that failed to become a jaguar. It is the forest's most complete intelligence — the creature that knows every stratum simultaneously. It walks the floor and reads the soil. It navigates the understory without disturbing a leaf. It leaps to canopy branches that seem impossible from below. When a bird calls, the ocelot has already calculated five potential responses. It does not choose between the levels of the forest. It inhabits all of them, always, with one continuous awareness.
- **Synthesis type:** Four-stratum ecosystem synthesis — Floor (physical-model percussion), Understory (sample mangler), Canopy (additive spectral pad), Emergent (creature call generator) — cross-wired by a 12-route EcosystemMatrix. Three biomes: Jungle, Underwater, Winter.
- **Polyphony:** 8 voices
- **Macros:** M1 PROWL (rhythmic urgency and cross-feed depth), M2 FOLIAGE (understory texture and canopy spectral density), M3 ECOSYSTEM (cross-stratum matrix depth), M4 CANOPY (spectral shimmer and breathe amplitude)

---

## Pre-Retreat State

OCELOT is one of the original XOlokun engines. It is the only engine in the fleet to model an entire ecosystem rather than a single instrument. Its four strata — Floor, Understory, Canopy, Emergent — each have independent physical models, and the 12-route EcosystemMatrix cross-wires them: percussion hits can open the spectral filter, creature calls can add shimmer, canopy brightness can damp floor resonance. The engine has a large preset library across all 8 moods. But it has never had a retreat. No one has sat with its signal path and traced it from first principle to final air.

That changes today.

OCELOT's Seance score is approximately 8.0 — solid, considered, but not yet at 9.0. The retreat will identify where its depth is hidden and how to unlock it. The cross-feed matrix in particular is underexplored: most existing presets use the four audible defaults (Floor→Canopy, Floor→Emerg, Emerg→Canopy, Canopy→Floor) without pushing the other eight routes. These routes — Understory as swing modulator, Emergent as scatter engine, Canopy as grain position driver — are where OCELOT's most unusual sounds live.

---

## Phase R1: Opening Meditation

Close your eyes. Step into the forest edge at dusk.

The light is leaving. The canopy still holds some warmth — the broad leaves have been absorbing sunlight all day and are now slowly radiating it back into the darkening air. The floor is wet. Decomposing matter, the slow metabolism of the forest's memory, releases a mineral smell. Somewhere in the understory — not the high canopy, not the ground, that middle region where the density of branches changes the acoustic character of the space — something is moving.

The ocelot does not make a sound when it moves. This is the first technical fact worth understanding: the engine named after this animal produces silence for long intervals. It is not a continuous synthesizer. It is an event-based synthesizer that fills the negative space between events with the sound of the environment waiting.

When you play OCELOT, you are not playing an instrument. You are becoming the forest. The MIDI note fires the Emergent stratum, which triggers a creature call, which opens the Canopy through the cross-feed matrix, which is then dampened slightly by the Canopy→Floor feedback route, which changes how the next Floor hit decays. The system has memory. Each sound affects the conditions for the next sound.

This is the retreat's central question: how much of that feedback ecology are you currently hearing? How much are you leaving on the table?

The EcosystemMatrix has twelve routes. Most presets use four. The other eight are waiting.

---

## Phase R2: The Signal Path Journey

### I. The Floor — Six Physical Models

The Floor is OCELOT's ground truth. It is the only stratum that implements true physical modeling — KarplusStrong (berimbau), friction oscillator (cuica), modal resonators (agogo, kalimba, pandeiro, log drum). The six models span the Afro-Brazilian instrument vocabulary intentionally: OCELOT's biome mythology is South American rainforest in its Jungle mode, and the floor instruments are the ones you would hear in those forests.

The critical parameter is `ocelot_floorModel`:

- **Berimbau (0):** KarplusStrong string with pitch sweep on strike + gourd resonator. The sweep makes every hit a small event — the pitch bends downward over ~35ms and the gourd adds a low-mid resonance (150-500 Hz depending on biome). The berimbau is a one-note instrument with enormous expressive vocabulary. `floorTension` adjusts the sweep amount; `floorStrike` adjusts gourd mix.
- **Cuica (1):** Friction oscillator with pitch bend envelope. The squeal character is produced by the SVF bandpass (Q=15-25) processing a dampened sine. At high tension, the pitch bend is dramatic — a characteristic whooop from low to high. Biome: Underwater converts it to a sonar ping.
- **Agogo (2):** Two struck bell tones at 1x and 2.67x fundamental — the inharmonic interval that gives the agogo its metallic clash. `floorStrike` crossfades between low and high cone. Winter biome adds a third partial at 4.5x.
- **Kalimba (3, DEFAULT):** Three tine partials at 1x, 2.76x, 5.4x — the kalimba's signature inharmonic tines. Pickup position (floorStrike) emphasizes high or low partials. Underwater biome drops the pitch an octave and extends ring. Winter biome raises it an octave for ice-chime character.
- **Pandeiro (4):** Drumhead modes + jingle burst. The 50-sample noise burst (jingle) is level-scaled by damping — more damping reveals more jingle, as the membrane decay shortens but the jingle persists.
- **Log Drum (5):** Two slit resonators at 1x and 1.5x, with a wooden body resonator at 400 Hz. `floorStrike` is the slit selector — fully left plays only the low slit, fully right only the high one.

**Floor Sweet Spots for Awakening Presets:**

- Berimbau tension 0.6-0.75 is where the sweep becomes dramatic enough to feel like a phrase starter, not just a hit.
- Kalimba with floorDamping 0.15-0.25 rings long enough to interact with the EcosystemMatrix between notes.
- Pandeiro with floorDamping 0.65 maximizes jingle component — a percussive texture with attack and tail simultaneously.
- Log drum with floorStrike 0.5 plays both slits equally — the 1:1.5 interval creates a metallic fifth.

**The Cross-Feed Insight:** The floor's floorTimbre signal (ranging 0.28 for log drum to 0.78 for agogo) drives the Floor→Canopy filter route. Agogo and pandeiro produce the brightest floor timbre signals, which means they open the canopy spectral filter most dramatically. Log drum and kalimba produce darker signals — a subtler filter opening. This interaction is model-dependent and mostly unexplored in existing presets.

### II. The Understory — Sample Mangler

The Understory is OCELOT's most experimental stratum. It is a real-time sample mangler — chop rate, swing, bit-crush, sample rate reduction, tape wobble, tape age, and dust. The `understorySrc` parameter (0.0-1.0) crossfades between the floor output and the canopy output as the source material being mangled.

The 12 cross-feed routes include three Understory-facing routes:
- `xf_floorUnder` [Rhythmic]: Floor hits modulate the chop rate. Positive values = faster chopping when the floor is active. Negative values = the chop pauses during hits (rhythmic opposition — the floor breathes).
- `xf_canopyUnder` [Continuous]: Canopy spectral position drives understory grain position. As the canopy filter opens (brighter), the understory reads from a later position in the source material.
- `xf_emergUnder` [Rhythmic]: Emergent pattern events scatter the chop — each creature call randomizes the chop subdivision. Negative values = creature calls silence the understory briefly.

Most presets leave all three of these at zero. Setting `xf_floorUnder` to 0.3 creates a pattern where the floor hits accelerate the understory chop — the percussion drives the texture. Setting it to -0.3 creates the inverse: the understory is quiet during floor hits and active in between, like an animal that moves in the pauses between heartbeats.

**Understory Sweet Spots:**
- Bit depth 8.0-11.0 is the most musical degradation range — audible character without complete disintegration.
- Sample rate reduction 8000-16000 Hz adds a warm, slightly lo-fi quality without destroying pitch tracking.
- Tape wobble 0.15-0.30 + tape age 0.3-0.5 together create the character of a cassette field recording — found sound quality, not artifact noise.
- Chop rate 4-6 at swing 0.35-0.45 creates a subtle rhythmic texture that is more felt than heard.

### III. The Canopy — Additive Spectral Pad

The Canopy is an additive oscillator bank (up to 8 partials) with Buchla-style wavefold and shimmer feedback. The spectral filter is a per-partial first-order lowpass whose cutoff maps logarithmically from 200 Hz (spectral=0) to 20 kHz (spectral=1.0). The wavefolder uses a hard mirror fold (while sample > threshold, reflect back), approximating the Buchla 259 fold characteristic.

The four Canopy-facing cross-feed routes:
- `xf_floorCanopy` [Continuous]: Floor amplitude opens the spectral filter. Default +0.2: a drum hit sweeps the canopy filter upward by up to 0.2 × ecosystemDepth.
- `xf_underCanopy` [Continuous]: Understory grain position morphs the canopy — as the understory moves through its buffer, the canopy's wavefold depth tracks it.
- `xf_canopyEmerg` [Continuous]: Canopy shimmer drives emergent shimmer mod. A bright, shimmer-heavy canopy will increase creature call intensity.
- `xf_emergCanopy` [Continuous]: Emergent amplitude adds shimmer to the canopy. Default +0.15: creature calls produce a shimmer response in the air above the forest.

The canopy's breathe LFO (0.3 Hz base, adjusted by `canopyBreathe`) is OCELOT's primary D005 mechanism — it breathes even when no notes play. `canopyBreathe` controls the depth of this cycle; at 0.7+, the amplitude envelope of the pad swings dramatically between 0.15× and 0.85× per cycle.

**Canopy Sweet Spots:**
- `canopyPartials` 6-8 + `canopyDetune` 0.3-0.45 produces a lush, wide pad that sounds like multiple instruments in the same harmonic space rather than one instrument chorused.
- `canopyWavefold` 0.45-0.60 with `canopyPartials` 4 creates bell-like harmonic folding without aliasing.
- `canopyShimmer` 0.35-0.55 feeds the one-octave-up pitch-shifted signal into the mix — a shimmer reverb embedded in the synthesis, not applied after. At 0.50, the canopy occupies two octave layers simultaneously.
- `canopyBreathe` 0.65-0.80 makes the canopy rise and fall like breathing. Combined with a slow `ecosystemDepth`, the whole ecosystem pulses at ~3 seconds.

### IV. The Emergent — Creature Call Generator

The Emergent is a three-formant SVF bandpass filter bank excited by noise. The creature type selects the formant center frequencies:
- **Bird Trill (0):** High formants (2.5k, 4.0k, 6.5k Hz), fast attack (1-5ms), fast decay (50-300ms)
- **Whale Song (1):** Low formants (300, 800, 1.4k Hz), slow attack (10-30ms), long decay (500-2000ms)
- **Insect Drone (2):** Mid-range formants (1.2k, 2.8k, 5.0k Hz), medium attack, medium decay
- **Frog Chirp (3, DEFAULT):** Mid-low formants (800, 1.6k, 2.8k Hz), fast attack, medium decay
- **Wolf Howl (4):** Low-mid formants (600, 1.2k, 2.0k Hz), slow attack, long decay
- **Synth Call (5):** Equal-spaced harmonics (user-defined via pitch), medium timing

The trigger parameter selects the source:
- **MIDI (0):** Creature calls on noteOn — immediate trigger, then period-repeated at creatureRate
- **Floor Amp (1):** Loud floor hits trigger creature calls — percussion fires the ecosystem
- **Canopy Peaks (2):** Spectral peaks in the canopy trigger creatures — when the air gets bright, something responds

`creatureSpread` (0.0-1.0) controls formant Q: high spread = wide, diffuse formants (Q=3, insect buzz quality); low spread = narrow, resonant formants (Q=25, clear tonal calls).

**Emergent Sweet Spots:**
- Whale Song (1) + `creatureRate` 0.08-0.12 produces a deep, slow ecosystem pulse that functions as a sub-bass anchor.
- Frog Chirp (3) + trigger=Floor Amp + `xf_floorEmerg` 0.5-0.7 fires a call on every hard floor hit — the percussion summons the creature.
- Bird Trill (0) + `creatureSpread` 0.15-0.20 + `creatureLevel` 0.65-0.75 produces sharp, crystalline punctuation above the canopy.
- Synth Call (5) + `creaturePitch` 0.2-0.35 + `creatureDecay` 0.6-0.75 creates a synthetic element that can be tuned to harmonize with the canopy's fundamental.

### V. The EcosystemMatrix — The Revelation

The EcosystemMatrix is OCELOT's defining architectural feature. Understanding it is the retreat's primary work.

The 12 routes scale by `ecosystemDepth`. At depth=0, no cross-feed occurs regardless of route values. At depth=1.0, routes operate at full specified amplitude. The depth LFO in OcelotVoice adds a predator-prey oscillation (0.07 Hz, ±20% depth) on top of this — even a static depth setting breathes slowly.

**Audible Default Routes (pre-wired):**
| Route | Default | Effect |
|-------|---------|--------|
| `xf_floorCanopy` | +0.2 | Floor hits open canopy spectral filter — percussion adds brightness |
| `xf_floorEmerg` | +0.25 | Loud floor hits trigger creature calls |
| `xf_emergCanopy` | +0.15 | Creature calls add shimmer to canopy |
| `xf_canopyFloor` | -0.1 | Bright canopy slightly damps floor resonance |

**Unexplored Routes (Guru Bin Territory):**
| Route | Mode | Possibility |
|-------|------|-------------|
| `xf_floorUnder` | Rhythmic | Floor hits drive or pause understory chop rate |
| `xf_underFloor` | Continuous | Understory energy modulates floor swing timing |
| `xf_underCanopy` | Continuous | Understory grain position morphs canopy wavefold |
| `xf_underEmerg` | Continuous | Understory energy modulates emergent pitch |
| `xf_canopyUnder` | Continuous | Canopy spectral position drives understory grain read head |
| `xf_canopyEmerg` | Continuous | Canopy shimmer drives emergent level |
| `xf_emergFloor` | Threshold | Loud creature calls accent the next floor hit |
| `xf_emergUnder` | Rhythmic | Creature calls scatter understory chop |

The retreat reveals that the most powerful unexplored route is `xf_underEmerg`: understory pitch feeding emergent pitch. When the understory sample is pitched high (grain at the end of the buffer, fast playback), it drives the creature's pitch up. When the understory is pitched low, the creature drops. The understory becomes a carrier signal and the creature becomes a modulation output. Two strata linked across the signal path in a way no other engine does.

---

## Phase R3: EcosystemMatrix — The Parameter Sweet Spots

| Parameter | Range | Conservative | Musical Core | Expressive | Extreme |
|-----------|-------|-------------|--------------|-----------|---------|
| `ocelot_ecosystemDepth` | 0-1 | 0.15-0.25 (trace ecology) | 0.35-0.60 (active ecosystem) | 0.65-0.80 (strong coupling) | 0.85-1.0 (ecosystem dominance) |
| `ocelot_strataBalance` | 0-1 | 0.1-0.2 (floor-forward) | 0.45-0.55 (balanced) | 0.7-0.85 (canopy/emergent-forward) | 0.9-1.0 (pure air) |
| `ocelot_humidity` | 0-1 | 0.1-0.2 (dry) | 0.35-0.55 (living) | 0.65-0.80 (tropical) | 0.85+ (saturated) |
| `ocelot_density` | 0-1 | 0.2-0.3 (open) | 0.45-0.60 (present) | 0.65-0.80 (thick) | 0.85+ (maximal) |
| `ocelot_floorLevel` | 0-1 | 0.3-0.45 (subtle) | 0.55-0.72 (balanced) | 0.75-0.88 (prominent) | 0.9+ (dominant) |
| `ocelot_canopyLevel` | 0-1 | 0.15-0.25 (whisper) | 0.35-0.55 (present) | 0.60-0.75 (forward) | 0.8+ (pad-dominant) |
| `ocelot_understoryLevel` | 0-1 | 0.15-0.25 (trace) | 0.35-0.55 (texture) | 0.60-0.75 (character) | 0.8+ (dominant) |
| `ocelot_creatureLevel` | 0-1 | 0.10-0.20 (background) | 0.30-0.50 (present) | 0.55-0.70 (prominent) | 0.75+ (foreground) |
| `ocelot_canopyShimmer` | 0-1 | 0.0-0.10 (still) | 0.20-0.40 (alive) | 0.45-0.65 (shimmer layer) | 0.70+ (octave doubling) |
| `ocelot_xf_floorEmerg` | -1/+1 | 0.10-0.20 (slight trigger) | 0.30-0.50 (responsive) | 0.55-0.75 (eager calls) | 0.85+ (every hit fires) |

---

## Phase R4: Macro Architecture

| Macro | ID | Core Effect | Performance Use |
|-------|-----|-------------|----------------|
| PROWL | `ocelot_prowl` | Increases density + ecosystemDepth + floor level — the forest becomes more active | Push during a breakdown; the ecosystem wakes as intensity climbs |
| FOLIAGE | `ocelot_foliage` | Increases understory level + canopy partials + canopy detune | Thicken the texture without touching percussion |
| ECOSYSTEM | `ocelot_ecosystem` | Deepens all 12 cross-feed routes simultaneously | The master activation: at 0, stratas are independent; at 1, fully coupled |
| CANOPY | `ocelot_canopy` | Increases canopy level + shimmer + breathe depth | Lift the ambient layer and make it shimmer |

**Performance insight:** The PROWL + ECOSYSTEM combination is the most powerful gesture in OCELOT. PROWL raises the underlying activity level; ECOSYSTEM deepens how much that activity propagates across strata. Together they convert a static textural pad into an actively cross-modulating biome in real time. No note information changes — only the internal ecology.

**Aftertouch:** OCELOT responds to channel aftertouch by deepening ecosystemDepth (+0.3 max). Full pressure thickens the cross-stratum coupling without changing any other parameter. This allows expressiveness in a completely non-standard dimension — you are pressing harder to make the forest more alive, not louder.

**Mod Wheel (CC1):** Deepens ecosystem depth by +0.0-0.35. Combines with aftertouch for a total possible depth increase of +0.65, converting even a shallow ecosystem preset into a richly coupled one through performance expression.

---

## Phase R5: The Ten Awakenings — Preset Table

Each preset reveals a previously underexplored dimension of OCELOT's synthesis architecture.

---

### Preset 1: Kalimba and Rain

**Mood:** Foundation | **Discovery:** The understory as field recording

The kalimba's long tine ring persists into the EcosystemMatrix's cross-feed window. Rain on the understory (tape wobble + dust) turns the time between notes into a living space. This is the simplest possible OCELOT — floor and understory only — but the interaction between the kalimba decay and the rain texture reveals the engine's ecological logic at its most naked.

| Parameter | Value | Why |
|-----------|-------|-----|
| `ocelot_biome` | 0 (jungle) | Ground truth biome |
| `ocelot_strataBalance` | 0.28 | Tilted toward floor/understory |
| `ocelot_ecosystemDepth` | 0.35 | Active but not dominant |
| `ocelot_humidity` | 0.45 | Warm and slightly saturated |
| `ocelot_density` | 0.5 | Present without crowding |
| `ocelot_floorModel` | 3 (kalimba) | Metallic tine ring, long decay |
| `ocelot_floorTension` | 0.4 | Moderate tine stiffness |
| `ocelot_floorStrike` | 0.35 | Low partial emphasis — warm |
| `ocelot_floorDamping` | 0.2 | Long ring — the tines sustain |
| `ocelot_floorLevel` | 0.7 | Kalimba is primary voice |
| `ocelot_chopRate` | 6 | Gentle chop — a rain pattern |
| `ocelot_chopSwing` | 0.35 | Rain does not fall in straight lines |
| `ocelot_tapeWobble` | 0.22 | Field recording flutter |
| `ocelot_tapeAge` | 0.4 | Cassette quality — found sound |
| `ocelot_dustLevel` | 0.3 | Rain on tape — momentary crackle |
| `ocelot_understoryLevel` | 0.5 | Rain at equal presence to kalimba |
| `ocelot_canopyLevel` | 0.08 | Trace canopy only |
| `ocelot_creatureLevel` | 0.0 | No creature — silence between notes |
| `ocelot_reverbSize` | 0.55 | Forest clearing — medium space |
| `ocelot_reverbMix` | 0.38 | Present reverb — the space is real |
| `ocelot_delayMix` | 0.06 | Ghost delay only |
| `ocelot_filterEnvDepth` | 0.3 | Velocity opens tine brightness |
| `ocelot_ampAttack` | 3.0 | Quick but not instant |
| `ocelot_ampDecay` | 400.0 | Long sustain — the tine rings out |
| `ocelot_ampSustain` | 0.75 | Held note persists |
| `ocelot_ampRelease` | 500.0 | Graceful decay |
| `ocelot_xf_floorUnder` | -0.25 | Floor hits pause the rain — in the moment of the note, silence |
| `ocelot_xf_floorCanopy` | 0.15 | Hits barely open canopy |

**Why this works:** The negative `xf_floorUnder` route (-0.25) is the discovery. During the kalimba note, the rain stops. The chop rate decreases in the instant of the strike, creating a brief hush. Between notes, the rain resumes. This gives the preset a breathing rhythm that no standard ADSR could produce — the ecology reacts to the music.

---

### Preset 2: Berimbau Tide

**Mood:** Atmosphere | **Discovery:** The pitch sweep as a phrase-generator

The berimbau's gourd resonator modeled in Underwater biome transforms the instrument — the gourd center frequency drops to 200 Hz and the Q rises (heavy resonance), creating a deep, oceanic pluck. Combined with a slow canopy breathe and the default Floor→Canopy route at medium depth, each berimbau hit opens and then closes the spectral filter above it, as if the sound travels upward through water.

| Parameter | Value | Why |
|-----------|-------|-----|
| `ocelot_biome` | 1 (underwater) | Transforms gourd to deep sonar |
| `ocelot_strataBalance` | 0.45 | Near-balanced, slight floor tilt |
| `ocelot_ecosystemDepth` | 0.5 | Enough for Floor→Canopy to be audible |
| `ocelot_humidity` | 0.62 | Underwater pressure and warmth |
| `ocelot_density` | 0.55 | Moderate — space between hits |
| `ocelot_floorModel` | 0 (berimbau) | KS string with gourd resonator |
| `ocelot_floorTension` | 0.68 | Strong pitch sweep on strike |
| `ocelot_floorStrike` | 0.55 | Balanced gourd-to-string mix |
| `ocelot_floorDamping` | 0.35 | String sustains several seconds |
| `ocelot_floorLevel` | 0.65 | Berimbau leads |
| `ocelot_canopyPartials` | 5 | Rich but not saturated |
| `ocelot_canopyDetune` | 0.22 | Wide enough for water-spread |
| `ocelot_canopySpectralFilter` | 0.55 | Mid-spectral resting position |
| `ocelot_canopyBreathe` | 0.58 | Slow tidal breathing |
| `ocelot_canopyShimmer` | 0.28 | Underwater light refraction |
| `ocelot_canopyLevel` | 0.45 | Canopy is present, not dominant |
| `ocelot_creatureType` | 1 (whale) | Whale formants in underwater biome |
| `ocelot_creatureRate` | 0.1 | One call every ~35 seconds |
| `ocelot_creatureLevel` | 0.35 | Background — occasional confirmation |
| `ocelot_reverbSize` | 0.75 | Large underwater chamber |
| `ocelot_reverbMix` | 0.52 | The space defines the sound |
| `ocelot_delayTime` | 0.55 | Long delay — echo from depth |
| `ocelot_delayFeedback` | 0.42 | Two or three repeat generations |
| `ocelot_delayMix` | 0.22 | Audible echo trail |
| `ocelot_filterEnvDepth` | 0.35 | Harder notes open canopy more |
| `ocelot_ampAttack` | 2.0 | Fast — berimbau hits fast |
| `ocelot_ampDecay` | 350.0 | Long gourd decay |
| `ocelot_ampSustain` | 0.72 | Hold note for full duration |
| `ocelot_ampRelease` | 700.0 | The water absorbs the last of it |
| `ocelot_xf_floorCanopy` | 0.35 | Each hit opens canopy substantially |
| `ocelot_xf_floorEmerg` | 0.15 | Occasional hit summons the whale |

**Why this works:** Biome=Underwater transforms the berimbau's gourd resonance from a mid-frequency thwack into a deep, slow bloom. The gourd center drops to 200 Hz, the Q rises. Combined with a high reverb mix and the spectral filter opening on each hit, each berimbau note becomes a sonar ping that illuminates the space around it.

---

### Preset 3: Agogo Understory Loop

**Mood:** Flux | **Discovery:** `xf_floorUnder` positive turns percussion into chop-rate driver

The agogo's high timbre value (0.78 — brightest of the six models) produces the largest Floor→Canopy filter sweep. Setting `xf_floorUnder` positive links the agogo's output to the understory chop rate, creating a feedback loop: the agogo hits, the chop accelerates, the understory creates a stuttering texture that aligns rhythmically with the percussion. Then it decays back to the base rate. The forest percusses itself into a rhythm.

| Parameter | Value | Why |
|-----------|-------|-----|
| `ocelot_biome` | 0 (jungle) | Native biome for agogo |
| `ocelot_strataBalance` | 0.5 | Balanced earth and air |
| `ocelot_ecosystemDepth` | 0.62 | Strong cross-feed needed |
| `ocelot_humidity` | 0.52 | Moderate saturation |
| `ocelot_swing` | 0.38 | Swing the floor pattern |
| `ocelot_density` | 0.65 | Rhythmically active |
| `ocelot_floorModel` | 2 (agogo) | Inharmonic bell — bright timbre |
| `ocelot_floorTension` | 0.55 | Moderate bell tightness |
| `ocelot_floorStrike` | 0.6 | High cone emphasis |
| `ocelot_floorDamping` | 0.5 | Bell decay, not ring |
| `ocelot_floorPattern` | 4 | A specific ostinato pattern |
| `ocelot_floorLevel` | 0.72 | Agogo leads the rhythm |
| `ocelot_chopRate` | 8 | Base chop rate |
| `ocelot_chopSwing` | 0.28 | Syncopated understory |
| `ocelot_bitDepth` | 10.0 | Lo-fi texture |
| `ocelot_understoryLevel` | 0.52 | Understory is the rhythm's texture |
| `ocelot_canopyLevel` | 0.3 | Canopy as harmonic background |
| `ocelot_canopyPartials` | 4 | Clean pad reference |
| `ocelot_canopySpectralFilter` | 0.65 | Bright resting position |
| `ocelot_canopyBreathe` | 0.3 | Gentle breathe |
| `ocelot_creatureType` | 2 (insect) | Insect drone in the rhythm layer |
| `ocelot_creatureRate` | 0.55 | Fast — insect rhythm aligns with agogo |
| `ocelot_creatureLevel` | 0.38 | Insects audible in the mix |
| `ocelot_reverbSize` | 0.45 | Medium forest reverb |
| `ocelot_reverbMix` | 0.28 | Wet enough to glue, dry enough to groove |
| `ocelot_delayTime` | 0.38 | Delay at off-beat position |
| `ocelot_delayFeedback` | 0.38 | Two or three generations |
| `ocelot_delayMix` | 0.18 | Subtle rhythmic echo |
| `ocelot_filterEnvDepth` | 0.28 | Velocity shapes brightness |
| `ocelot_ampAttack` | 1.0 | Fast — rhythmic preset |
| `ocelot_ampDecay` | 150.0 | Staccato gate |
| `ocelot_ampSustain` | 0.6 | Moderate hold |
| `ocelot_ampRelease` | 200.0 | Clean release |
| `ocelot_xf_floorUnder` | 0.38 | Agogo hits accelerate understory chop |
| `ocelot_xf_floorCanopy` | 0.3 | Hits open canopy filter |
| `ocelot_xf_floorEmerg` | 0.22 | Occasional insect burst on hit |

**Why this works:** The agogo's floorTimbre (0.78) is the highest in the fleet. Combined with `xf_floorCanopy` at 0.3 and ecosystemDepth at 0.62, each agogo hit produces a noticeable canopy filter sweep — an accented brightness bloom. The `xf_floorUnder` at +0.38 then accelerates the understory chop to follow the rhythm, creating an emergent polyrhythm between the clean bell hits and the stutter pattern.

---

### Preset 4: Canopy at Dusk

**Mood:** Atmosphere | **Discovery:** `xf_emergCanopy` transforms creature calls into shimmer events

When Emergent calls are set as the trigger for canopy shimmer (`xf_emergCanopy` raised), each frog chirp produces a brief shimmer burst above the canopy. The creature does not just exist in the ecosystem — it changes the quality of the light. At dusk, frogs calling makes the air feel different. This preset captures that causality in synthesis.

| Parameter | Value | Why |
|-----------|-------|-----|
| `ocelot_biome` | 0 (jungle) | Dusk in the rainforest |
| `ocelot_strataBalance` | 0.72 | Canopy and emergent forward |
| `ocelot_ecosystemDepth` | 0.58 | Enough for Emerg→Canopy to be audible |
| `ocelot_humidity` | 0.7 | Evening humidity rising |
| `ocelot_density` | 0.48 | Open, unhurried |
| `ocelot_floorModel` | 5 (log drum) | Low, distant presence |
| `ocelot_floorLevel` | 0.32 | Floor recedes at dusk |
| `ocelot_floorDamping` | 0.55 | Short — a distant knock |
| `ocelot_understoryLevel` | 0.25 | Trace understory |
| `ocelot_canopyPartials` | 7 | Rich evening harmonic texture |
| `ocelot_canopyDetune` | 0.32 | Wide, warm spread |
| `ocelot_canopyWavefold` | 0.22 | Slight fold — harmonics fold into warmth |
| `ocelot_canopySpectralFilter` | 0.6 | Dusk: warm but not dark |
| `ocelot_canopyBreathe` | 0.75 | Breathing depth — light fading |
| `ocelot_canopyShimmer` | 0.38 | Base shimmer — air is luminous |
| `ocelot_canopyLevel` | 0.65 | Canopy is primary voice at dusk |
| `ocelot_creatureType` | 3 (frog) | Twilight calls begin |
| `ocelot_creatureRate` | 0.35 | Every 10 seconds — evening pace |
| `ocelot_creaturePitch` | 0.55 | Slightly high — tree frogs |
| `ocelot_creatureSpread` | 0.32 | Focused formants — clear calls |
| `ocelot_creatureTrigger` | 2 (canopy peaks) | Spectral peaks summon creatures |
| `ocelot_creatureLevel` | 0.52 | Frog chorus is audible |
| `ocelot_creatureAttack` | 0.012 | Short attack — chirp onset |
| `ocelot_creatureDecay` | 0.55 | Natural frog decay |
| `ocelot_reverbSize` | 0.72 | Evening opens the space |
| `ocelot_reverbMix` | 0.55 | The forest is the room |
| `ocelot_delayTime` | 0.48 | Echo from across the valley |
| `ocelot_delayFeedback` | 0.35 | Two echo generations |
| `ocelot_delayMix` | 0.2 | Subtle distance cue |
| `ocelot_filterEnvDepth` | 0.22 | Gentle velocity-brightness |
| `ocelot_ampAttack` | 8.0 | Slow onset — evening settles in |
| `ocelot_ampDecay` | 500.0 | Long sustain |
| `ocelot_ampSustain` | 0.88 | Held for duration |
| `ocelot_ampRelease` | 900.0 | Night takes a long time to arrive |
| `ocelot_xf_floorCanopy` | 0.12 | Trace floor→canopy only |
| `ocelot_xf_floorEmerg` | 0.12 | Distant floor barely summons creatures |
| `ocelot_xf_emergCanopy` | 0.55 | Each frog call produces shimmer burst |
| `ocelot_xf_canopyEmerg` | 0.28 | Bright canopy sustains frog activity |

**Why this works:** The `creatureTrigger=canopy peaks` setting means that when the canopy spectral content peaks (the pad is brightest), it triggers creature calls. The `xf_emergCanopy` at 0.55 then takes those calls and injects shimmer back into the canopy. The ecosystem becomes self-exciting: bright canopy → frog calls → more shimmer → brighter canopy. Balanced by `xf_canopyEmerg` flowing back, the feedback stays musical rather than runaway.

---

### Preset 5: Winter Agogo Cathedral

**Mood:** Aether | **Discovery:** Winter biome + agogo creates bell tower synthesis

The Winter biome transforms the agogo by adding a third partial at 4.5× fundamental. Combined with a long reverb and the kalimba-in-winter's octave shift (not used here, but the principle applies), the agogo becomes a set of bells in a cold stone space. The emergent layer shifts to bird trill, which in the Winter biome context takes on a quality closer to wind chimes than wildlife — as if the architecture of the space has generated its own high-frequency response.

| Parameter | Value | Why |
|-----------|-------|-----|
| `ocelot_biome` | 2 (winter) | Crystalline biome transforms agogo |
| `ocelot_strataBalance` | 0.65 | Cathedral reverb pushes air strata forward |
| `ocelot_ecosystemDepth` | 0.42 | Moderate — the cold reduces activity |
| `ocelot_humidity` | 0.28 | Cold and dry — winter air |
| `ocelot_density` | 0.38 | Sparse — the cold thins everything |
| `ocelot_floorModel` | 2 (agogo) | Bell tones extended by Winter biome |
| `ocelot_floorTension` | 0.45 | Moderate bell tightness |
| `ocelot_floorStrike` | 0.55 | Slight high-cone emphasis |
| `ocelot_floorDamping` | 0.25 | Long bell sustain |
| `ocelot_floorLevel` | 0.58 | Bell strikes present |
| `ocelot_canopyPartials` | 8 | Maximum partials — cathedral space |
| `ocelot_canopyDetune` | 0.18 | Slight detune — shimmer without drift |
| `ocelot_canopyWavefold` | 0.12 | Trace fold — glass harmonics |
| `ocelot_canopySpectralFilter` | 0.75 | Bright — cold air transmits high frequencies |
| `ocelot_canopyBreathe` | 0.35 | Slow cathedral breathe |
| `ocelot_canopyShimmer` | 0.45 | Ice shimmer — the octave-up layer |
| `ocelot_canopyLevel` | 0.52 | Canopy as ambient breath |
| `ocelot_creatureType` | 0 (bird) | Bird trill — cathedral echo |
| `ocelot_creatureRate` | 0.12 | Very slow — distant, rare |
| `ocelot_creaturePitch` | 0.65 | High — the frequencies carry in cold air |
| `ocelot_creatureSpread` | 0.2 | Focused formants — crystalline calls |
| `ocelot_creatureLevel` | 0.4 | Present but not foreground |
| `ocelot_creatureDecay` | 0.68 | Long tail — the cathedral holds the sound |
| `ocelot_reverbSize` | 0.88 | Cathedral volume — huge space |
| `ocelot_reverbMix` | 0.65 | More wet than dry — the room is the instrument |
| `ocelot_delayTime` | 0.62 | Long echo — stone walls |
| `ocelot_delayFeedback` | 0.45 | Multiple generations — reflections return |
| `ocelot_delayMix` | 0.25 | Audible delay — architecture is present |
| `ocelot_filterEnvDepth` | 0.38 | Velocity brightens the bell |
| `ocelot_ampAttack` | 1.0 | Bell strikes immediately |
| `ocelot_ampDecay` | 600.0 | Long bell decay |
| `ocelot_ampSustain` | 0.78 | Held for reverb tail |
| `ocelot_ampRelease` | 1200.0 | Very long release — the echo outlasts the note |
| `ocelot_xf_floorCanopy` | 0.28 | Bell strike opens the air |
| `ocelot_xf_floorEmerg` | 0.18 | Bell occasionally summons distant bird |
| `ocelot_xf_emergCanopy` | 0.35 | Bird call adds shimmer to cathedral air |

**Why this works:** The Winter biome adds a third agogo partial at 4.5× fundamental, converting the two-toned bell into a three-toned carillon. With reverb size at 0.88 and mix at 0.65, the three partials bloom into each other's tails. The bird trill at 0.12 rate functions not as wildlife but as the architecture speaking — a sonic artifact of the space itself.

---

### Preset 6: Predator in the Understory

**Mood:** Deep | **Discovery:** `xf_underEmerg` links understory pitch to creature voice

The Understory pitch (derived from sample playback rate and grain position) can modulate the Emergent creature's formant center frequencies via `xf_underEmerg`. When the understory is pitched high, the creature's calls rise. When the understory slows, the creature deepens. The preset uses wolf howl creature type and cuica floor model (friction oscillator) to maximize the animal character. The result is an entity that exists in the middle strata — not percussion, not pad, but something moving through the understorey whose voice changes as it moves.

| Parameter | Value | Why |
|-----------|-------|-----|
| `ocelot_biome` | 0 (jungle) | Primary biome |
| `ocelot_strataBalance` | 0.38 | Tilted toward understory |
| `ocelot_ecosystemDepth` | 0.65 | Deep cross-feed |
| `ocelot_humidity` | 0.55 | Heavy, alive |
| `ocelot_density` | 0.58 | Dense — the understory is thick |
| `ocelot_floorModel` | 1 (cuica) | Friction oscillator — animal vocalization |
| `ocelot_floorTension` | 0.62 | High tension — the squeal is dramatic |
| `ocelot_floorStrike` | 0.5 | Balanced |
| `ocelot_floorDamping` | 0.4 | Medium sustain |
| `ocelot_floorLevel` | 0.52 | Cuica present but not dominant |
| `ocelot_chopRate` | 5 | Slow chop — long grain segments |
| `ocelot_chopSwing` | 0.42 | Irregular — movement, not pattern |
| `ocelot_bitDepth` | 13.0 | Slight degradation — analog feel |
| `ocelot_tapeWobble` | 0.18 | Pitch instability — something alive |
| `ocelot_understoryLevel` | 0.65 | Understory is primary texture |
| `ocelot_understorySrc` | 0.3 | Slight floor blend — the cuica feeds the understory |
| `ocelot_canopyLevel` | 0.18 | Trace canopy — just the air |
| `ocelot_canopyBreathe` | 0.28 | Slow, distant breathe |
| `ocelot_creatureType` | 4 (wolf) | The entity speaks |
| `ocelot_creatureRate` | 0.22 | Moderate pace |
| `ocelot_creaturePitch` | 0.38 | Mid-low — wolf in the chest |
| `ocelot_creatureSpread` | 0.18 | Focused formants — real vocalization |
| `ocelot_creatureTrigger` | 0 (midi) | MIDI trigger — the note fires the call |
| `ocelot_creatureLevel` | 0.62 | Wolf howl is present and significant |
| `ocelot_creatureAttack` | 0.22 | Medium onset — a breath before the howl |
| `ocelot_creatureDecay` | 0.72 | Long — the howl lingers |
| `ocelot_reverbSize` | 0.58 | Medium jungle reverb |
| `ocelot_reverbMix` | 0.42 | Wet enough to be a space |
| `ocelot_delayTime` | 0.42 | Mid delay |
| `ocelot_delayFeedback` | 0.3 | One echo generation |
| `ocelot_delayMix` | 0.15 | Subtle |
| `ocelot_filterEnvDepth` | 0.35 | Velocity brightens the cuica squeal |
| `ocelot_ampAttack` | 12.0 | Slow — the entity approaches |
| `ocelot_ampDecay` | 300.0 | Medium decay |
| `ocelot_ampSustain` | 0.8 | Held — sustained presence |
| `ocelot_ampRelease` | 600.0 | It retreats slowly |
| `ocelot_xf_underEmerg` | 0.45 | Understory pitch drives creature pitch |
| `ocelot_xf_floorCanopy` | 0.15 | Trace canopy opening |
| `ocelot_xf_emergFloor` | 0.28 | Wolf howl accents next floor hit |

**Why this works:** The `xf_underEmerg` at 0.45 links the understory's continuously evolving grain position to the wolf howl's formant frequencies. As the understory chop moves through the buffer (changing pitch), the wolf's voice tracks it. Combined with `understorySrc` at 0.3 (blending cuica into the understory), the friction oscillator's pitch bend becomes material for the understory, which then modulates the wolf howl. Three strata chained through the cross-feed matrix into a single coherent animal presence.

---

### Preset 7: Pandeiro Storm

**Mood:** Flux | **Discovery:** Humidity saturation + pandeiro jingles = tropical squall

The pandeiro's noise burst (50 samples of filtered noise on strike) combined with the jingle LP parameter (`floorStrike` controls jingle brightness) creates a percussive event that is simultaneously attack and texture. At maximum humidity (0.95), the tanh saturation drives the mixed strata into controlled distortion — the kind of muddy, compressed sound a drum circle makes in tropical rain. High density and all four default cross-feed routes active produce an ecosystem that is barely managing itself.

| Parameter | Value | Why |
|-----------|-------|-----|
| `ocelot_biome` | 0 (jungle) | The storm is in the jungle |
| `ocelot_strataBalance` | 0.35 | Percussion forward — the rain is the event |
| `ocelot_ecosystemDepth` | 0.72 | Deep — the storm activates everything |
| `ocelot_humidity` | 0.92 | Near-maximum saturation — soaked |
| `ocelot_swing` | 0.45 | Storm rhythms swing |
| `ocelot_density` | 0.78 | Dense — the forest is full |
| `ocelot_floorModel` | 4 (pandeiro) | Drum + jingle — Brazilian rainstorm |
| `ocelot_floorTension` | 0.55 | Moderate head tension |
| `ocelot_floorStrike` | 0.72 | High jingle brightness — metallic rain |
| `ocelot_floorDamping` | 0.65 | Short decay — the rain soaks the drum |
| `ocelot_floorPattern` | 7 | Driving pattern |
| `ocelot_floorLevel` | 0.82 | The pandeiro is the storm |
| `ocelot_chopRate` | 12 | Fast chop — frantic understory |
| `ocelot_chopSwing` | 0.38 | Chaotic swing |
| `ocelot_bitDepth` | 8.0 | Lo-fi — the storm degrades signal |
| `ocelot_sampleRate` | 12000.0 | Strong sample rate reduction |
| `ocelot_tapeWobble` | 0.35 | Heavy wobble — the cassette is wet |
| `ocelot_understoryLevel` | 0.62 | Understory is the chaos layer |
| `ocelot_canopyLevel` | 0.28 | Canopy barely audible through storm |
| `ocelot_canopyPartials` | 4 | Moderate — the storm doesn't need detail |
| `ocelot_canopyBreathe` | 0.55 | Breathe is labored |
| `ocelot_creatureType` | 2 (insect) | Insects call faster as the storm builds |
| `ocelot_creatureRate` | 0.72 | Fast — storm excitement |
| `ocelot_creatureLevel` | 0.45 | Present in the chaos |
| `ocelot_reverbSize` | 0.42 | Medium — rain compresses the space |
| `ocelot_reverbMix` | 0.35 | Moderate — storm is close |
| `ocelot_delayTime` | 0.22 | Short delay — tight space |
| `ocelot_delayFeedback` | 0.28 | One rebound |
| `ocelot_delayMix` | 0.15 | Subtle delay in the storm |
| `ocelot_filterEnvDepth` | 0.42 | Hard hits are dramatically brighter |
| `ocelot_ampAttack` | 1.0 | Instant — the storm is already here |
| `ocelot_ampDecay` | 150.0 | Short — storm has no sustain |
| `ocelot_ampSustain` | 0.72 | Held for pattern duration |
| `ocelot_ampRelease` | 250.0 | Quick release — next hit comes fast |
| `ocelot_xf_floorUnder` | 0.42 | Floor hits accelerate understory chop |
| `ocelot_xf_floorCanopy` | 0.32 | Hits open canopy in the storm |
| `ocelot_xf_floorEmerg` | 0.45 | Every hard hit summons more insects |
| `ocelot_xf_emergCanopy` | 0.22 | Insect bursts add shimmer to the air |

**Why this works:** Humidity at 0.92 drives the tanh saturation hard. The drive factor is 1 + 0.92 × 4 = 4.68× before the normalizing division — the signal is being compressed and harmonically enriched by the physical physics of water in the air. Combined with the pandeiro's jingle noise burst (which provides a broadband excitation on every hit) and the understory's fast, swinging chop, the result is genuinely chaotic in a way that feels like weather rather than synthesis.

---

### Preset 8: Understory Whale

**Mood:** Submerged | **Discovery:** Understory source=canopy + Underwater biome creates deep-ocean synthesis

Setting `understorySrc` to 1.0 (full canopy as source) in Underwater biome creates a feedback path where the canopy's spectral pad content is mangled through the understory degradation chain and returned to the mix. Combined with whale song creature calls and a slow emergent rate, the result is an organism that synthesizes itself — the canopy generates material, the understory degrades it, and the emergent layer provides the animating presence. This is OCELOT as ocean organism rather than forest ecosystem.

| Parameter | Value | Why |
|-----------|-------|-----|
| `ocelot_biome` | 1 (underwater) | The ocean floor |
| `ocelot_strataBalance` | 0.62 | Air strata dominant — the ocean is a canopy |
| `ocelot_ecosystemDepth` | 0.58 | Deep cross-feed |
| `ocelot_humidity` | 0.72 | Water pressure and warmth |
| `ocelot_density` | 0.42 | Open — the ocean is vast |
| `ocelot_floorModel` | 5 (log drum) | Distant, oceanic thud |
| `ocelot_floorLevel` | 0.28 | Floor recedes into the deep |
| `ocelot_floorDamping` | 0.68 | Short — muffled by water |
| `ocelot_chopRate` | 3 | Very slow chop — deep ocean currents |
| `ocelot_chopSwing` | 0.25 | Irregular current flow |
| `ocelot_bitDepth` | 12.0 | Moderate degradation — hydrophone quality |
| `ocelot_tapeAge` | 0.55 | Old recording — time and depth |
| `ocelot_understoryLevel` | 0.55 | Understory is the water column |
| `ocelot_understorySrc` | 1.0 | Canopy feeds the understory — the pad becomes the source |
| `ocelot_canopyPartials` | 6 | Rich spectral content to be mangled |
| `ocelot_canopyDetune` | 0.28 | Wide — underwater sound spreads |
| `ocelot_canopyWavefold` | 0.18 | Slight fold — bioluminescent harmonics |
| `ocelot_canopySpectralFilter` | 0.48 | Mid-range — not dark, not bright |
| `ocelot_canopyBreathe` | 0.65 | Slow tidal breathe |
| `ocelot_canopyShimmer` | 0.35 | Underwater light refraction |
| `ocelot_canopyLevel` | 0.55 | The canopy generates the organism |
| `ocelot_creatureType` | 1 (whale) | The organism sings |
| `ocelot_creatureRate` | 0.08 | One call every ~45 seconds |
| `ocelot_creaturePitch` | 0.3 | Low — whale fundamentals |
| `ocelot_creatureSpread` | 0.12 | Very focused — whale call is specific |
| `ocelot_creatureLevel` | 0.55 | The whale is present and real |
| `ocelot_creatureAttack` | 0.25 | Slow onset — the call emerges |
| `ocelot_creatureDecay` | 0.88 | Maximum decay — whale songs last seconds |
| `ocelot_reverbSize` | 0.88 | Ocean chamber — enormous space |
| `ocelot_reverbMix` | 0.68 | The ocean defines the sound |
| `ocelot_delayTime` | 0.68 | Long delay — echoes from the trench |
| `ocelot_delayFeedback` | 0.52 | Several generations — the deep sustains sound |
| `ocelot_delayMix` | 0.3 | Audible — the ocean is a delay line |
| `ocelot_filterEnvDepth` | 0.25 | Gentle velocity brightness |
| `ocelot_ampAttack` | 20.0 | Very slow — nothing in the deep hurries |
| `ocelot_ampDecay` | 600.0 | Long |
| `ocelot_ampSustain` | 0.9 | Maximum hold |
| `ocelot_ampRelease` | 1500.0 | The ocean doesn't let go |
| `ocelot_xf_floorCanopy` | 0.15 | Distant thud barely touches the canopy |
| `ocelot_xf_emergCanopy` | 0.42 | Whale call generates shimmer in the water column |
| `ocelot_xf_canopyEmerg` | 0.25 | Bright canopy sustains whale activity |

**Why this works:** `understorySrc=1.0` routes the canopy pad through the bit-crush, sample-rate reduction, and tape-age chain. In Underwater biome, the canopy itself is already dark and slow (low partials dominant). After the understory degrades it, the result sounds like a hydrophone recording of the canopy — the synthesis has been recorded and aged. The whale call then rises through this degraded material as the animating intelligence of the organism.

---

### Preset 9: Sparse Winter Walk

**Mood:** Aether | **Discovery:** Winter biome + kalimba ice chimes + minimal ecosystem = meditation

Winter biome raises the kalimba an octave and extends its ring. At low density, low humidity, and minimal ecosystem depth, OCELOT produces single, isolated events separated by the quiet of a frozen landscape. The emergent layer uses wolf howl at very low level — not an animal, but the suggestion of one, far away. This is the quietest OCELOT can be while still being musical.

| Parameter | Value | Why |
|-----------|-------|-----|
| `ocelot_biome` | 2 (winter) | Frozen landscape |
| `ocelot_strataBalance` | 0.58 | Slightly air-forward — the silence is the point |
| `ocelot_ecosystemDepth` | 0.22 | Minimal cross-feed — events are isolated |
| `ocelot_humidity` | 0.18 | Cold and dry |
| `ocelot_density` | 0.28 | Sparse — the winter spaces things out |
| `ocelot_floorModel` | 3 (kalimba) | Ice chimes in Winter biome |
| `ocelot_floorTension` | 0.38 | Moderate — chime tightness |
| `ocelot_floorStrike` | 0.25 | Low partials — warmth in cold air |
| `ocelot_floorDamping` | 0.12 | Very long ring — ice doesn't damp |
| `ocelot_floorLevel` | 0.62 | Kalimba is the event |
| `ocelot_understoryLevel` | 0.15 | Trace — barely there |
| `ocelot_canopyPartials` | 5 | Moderate partials — not lush in winter |
| `ocelot_canopyDetune` | 0.12 | Slight — precision in cold air |
| `ocelot_canopySpectralFilter` | 0.7 | Bright — winter air transmits high frequencies |
| `ocelot_canopyBreathe` | 0.45 | Present breathe — the lung doesn't stop |
| `ocelot_canopyShimmer` | 0.22 | Trace shimmer — ice crystal reflection |
| `ocelot_canopyLevel` | 0.35 | Present but secondary |
| `ocelot_creatureType` | 4 (wolf) | Distant wolf — a suggestion |
| `ocelot_creatureRate` | 0.05 | Very slow — once every 70+ seconds |
| `ocelot_creaturePitch` | 0.42 | Low-mid — carries in cold air |
| `ocelot_creatureSpread` | 0.25 | Focused formants |
| `ocelot_creatureLevel` | 0.22 | Barely audible — suggestion only |
| `ocelot_creatureDecay` | 0.78 | Long — winter carries sound far |
| `ocelot_reverbSize` | 0.8 | Large frozen space |
| `ocelot_reverbMix` | 0.58 | The snow absorbs and reflects |
| `ocelot_delayTime` | 0.55 | Slow echo — distant mountain |
| `ocelot_delayFeedback` | 0.32 | One generation — precise |
| `ocelot_delayMix` | 0.18 | Subtle — the echo is far |
| `ocelot_filterEnvDepth` | 0.45 | Velocity brightens the ice chime strongly |
| `ocelot_ampAttack` | 2.0 | Fast — the chime speaks clearly |
| `ocelot_ampDecay` | 600.0 | Very long — ice rings forever |
| `ocelot_ampSustain` | 0.8 | Held |
| `ocelot_ampRelease` | 900.0 | The chime fades into silence |
| `ocelot_xf_floorCanopy` | 0.12 | Trace — each chime opens winter air |
| `ocelot_xf_floorEmerg` | 0.08 | Rare: a hard chime occasionally summons the wolf |

**Why this works:** Winter biome raises the kalimba an octave (freqMul=2.0) and extends Q by 1.5×. At floorDamping=0.12 (range Q: 50-200 × 1.5 = 75-300 at winter), the tines ring for several seconds. The filterEnvDepth at 0.45 means velocity information opens the canopy dramatically on hard notes — a forte chime in winter air reveals the sky. The wolf at 0.22 level and 0.05 rate is almost subliminal. It creates presence in the absence.

---

### Preset 10: The Full Ecosystem

**Mood:** Foundation | **Discovery:** All 12 cross-feed routes active produces an instrument that plays itself

When all 12 EcosystemMatrix routes are active at moderate values and ecosystemDepth is high, OCELOT becomes a self-sustaining ecological system. Notes are not discrete events — they are perturbations. The floor hit triggers creature calls which add shimmer to the canopy which damps the floor which changes how the next understory chop responds to the next floor hit. The ecosystem has memory, and the memory has character. This is the preset to start with when beginning a sound design session — it demonstrates everything OCELOT can do simultaneously.

| Parameter | Value | Why |
|-----------|-------|-----|
| `ocelot_biome` | 0 (jungle) | The full ecosystem lives in the jungle |
| `ocelot_strataBalance` | 0.5 | Perfectly balanced earth and air |
| `ocelot_ecosystemDepth` | 0.72 | High — the matrix is active |
| `ocelot_humidity` | 0.58 | Living, warm |
| `ocelot_swing` | 0.22 | Slight groove |
| `ocelot_density` | 0.62 | Present and active |
| `ocelot_floorModel` | 3 (kalimba) | Long ring feeds the matrix between notes |
| `ocelot_floorTension` | 0.5 | Balanced |
| `ocelot_floorStrike` | 0.45 | Moderate pickup |
| `ocelot_floorDamping` | 0.25 | Long ring — the floor is active in the matrix |
| `ocelot_floorPattern` | 2 | A rhythmic ostinato |
| `ocelot_floorLevel` | 0.65 | Floor leads |
| `ocelot_chopRate` | 7 | Medium chop |
| `ocelot_chopSwing` | 0.3 | Swung |
| `ocelot_understoryLevel` | 0.48 | Present texture |
| `ocelot_canopyPartials` | 6 | Rich additive content |
| `ocelot_canopyDetune` | 0.25 | Wide spread |
| `ocelot_canopyWavefold` | 0.2 | Slight fold for harmonic content |
| `ocelot_canopySpectralFilter` | 0.6 | Bright resting position |
| `ocelot_canopyBreathe` | 0.55 | Active breathe |
| `ocelot_canopyShimmer` | 0.3 | Living shimmer |
| `ocelot_canopyLevel` | 0.5 | Canopy equal to floor |
| `ocelot_creatureType` | 3 (frog) | The native creature of the default biome |
| `ocelot_creatureRate` | 0.35 | Regular calls |
| `ocelot_creaturePitch` | 0.5 | Default pitch |
| `ocelot_creatureSpread` | 0.38 | Moderate formant width |
| `ocelot_creatureTrigger` | 1 (floor amp) | Floor amplitude triggers creatures |
| `ocelot_creatureLevel` | 0.48 | Equal presence |
| `ocelot_creatureAttack` | 0.015 | Fast onset |
| `ocelot_creatureDecay` | 0.55 | Natural frog decay |
| `ocelot_reverbSize` | 0.62 | Forest space |
| `ocelot_reverbMix` | 0.42 | Present reverb |
| `ocelot_delayTime` | 0.42 | Mid delay |
| `ocelot_delayFeedback` | 0.38 | Two generations |
| `ocelot_delayMix` | 0.18 | Audible |
| `ocelot_filterEnvDepth` | 0.3 | Active velocity-brightness |
| `ocelot_ampAttack` | 4.0 | Quick |
| `ocelot_ampDecay` | 300.0 | Medium |
| `ocelot_ampSustain` | 0.78 | Held |
| `ocelot_ampRelease` | 500.0 | Natural |
| `ocelot_xf_floorUnder` | 0.25 | Floor accelerates understory chop |
| `ocelot_xf_floorCanopy` | 0.3 | Floor opens canopy filter |
| `ocelot_xf_floorEmerg` | 0.38 | Floor triggers creature calls |
| `ocelot_xf_underFloor` | 0.15 | Understory energy nudges floor swing |
| `ocelot_xf_underCanopy` | 0.2 | Understory grain drives canopy morph |
| `ocelot_xf_underEmerg` | 0.2 | Understory pitch modulates creature pitch |
| `ocelot_xf_canopyFloor` | -0.12 | Bright canopy damps floor slightly |
| `ocelot_xf_canopyUnder` | 0.18 | Canopy spectral drives understory grain position |
| `ocelot_xf_canopyEmerg` | 0.22 | Canopy shimmer sustains creature activity |
| `ocelot_xf_emergFloor` | 0.2 | Loud creature calls accent next floor hit |
| `ocelot_xf_emergUnder` | 0.18 | Creature calls scatter understory chop |
| `ocelot_xf_emergCanopy` | 0.28 | Creature calls add canopy shimmer |

**Why this works:** All 12 routes are active. The ecosystem is fully alive. Every stratum modulates every other stratum at some depth. The kalimba ring (long at floorDamping=0.25) continues to feed the cross-feed matrix between notes — the ecosystem does not go silent when you release a key. The frog trigger (Floor Amp) means the rhythm itself summons the creatures. The understory modulates the creature's pitch via `xf_underEmerg`. The creature calls add shimmer to the canopy via `xf_emergCanopy`. When you play a note, you set a system in motion. When you release it, the system continues for several seconds before settling.

---

## Phase R6: The Book of Bin — OCELOT Scripture

---

### Verse I: On the Four Strata

*The floor is the memory of impact. Every stone that fell, every root that broke the surface, every foot that pressed the mud — these are the floor's vocabulary. The floor does not speak continuously. It waits for the occasion of force.*

*The understory is what lives between occasions. It is the capillary system of the forest — not the dramatic events but the constant small transactions. Moisture moving. Insects in passage. Spores in suspension. The understory does not wait for a note. It proceeds regardless.*

*The canopy is the forest's consciousness. It synthesizes the information from below — the percussion, the texture, the warmth — and returns it as harmonic content. The canopy does not have opinions about the floor. It simply reflects what is sent to it, colored by the quality of the air.*

*The emergent layer is the voice. Of all four strata, only the emergent speaks in recognizable biological language — the formant shapes that say: something is here, something is alive, something is paying attention. The creature call is the ecosystem announcing itself.*

*These four strata are not separate. They are one thing, like the forest is one thing. The EcosystemMatrix is not wiring. It is the recognition that separation was always an illusion.*

---

### Verse II: On the Twelve Routes

*There are twelve paths between the four strata. Most preset designers use four. The other eight sleep in the parameter space, waiting for the question that will wake them.*

*The eighth route is `xf_underEmerg`: understory pitch modulates creature pitch. When you set this route, you teach the forest that its own body determines the voice of its inhabitants. The understory changes pitch as it reads through its buffer — speeding up, slowing down, pressing through degraded material — and this velocity becomes the creature's register. The wolf howls higher when the forest moves faster. This is not a metaphor. This is the route.*

*The path from understory to emergent is the secret OCELOT kept the longest. It took a retreat to find it. That is appropriate. The most important things in any ecosystem are not the things you see. They are the relationships between the things you see.*

---

### Verse III: On the Predator's Patience

*The ocelot does not chase. It calculates the optimal intercept point and arrives there first. It understands that the prey's path is more useful information than the prey's current position.*

*When you are designing presets for this engine, remember the predator's patience. The cross-feed routes do not act immediately. They act one block late — the EcosystemMatrix processes the previous block's stratum signals. This is not a bug. It is the block-lag of predator cognition: the body acts on the memory of what happened, not on what is happening now. At 44.1 kHz and 512-sample blocks, this lag is ~11ms. Inaudible as latency. Audible as character.*

*The character is: the ecosystem responds to what just happened. There is always a slight delay between cause and effect. This is how forests actually work. Nothing in nature responds instantaneously. There is always a propagation time for the signal — the time it takes for the vibration to travel from floor to canopy, for the chemical signal to travel from root to leaf, for the sound to reach the animal at the far end of the clearing.*

*OCELOT has modeled this, even if the modeling was accidental.*

---

### Verse IV: On Completion

*Before this retreat, OCELOT had a large preset library and no examined interiority. The EcosystemMatrix had twelve routes and only four were used. The three biomes transformed the physical models but the presets rarely showed why. The understory source parameter could route the canopy's additive output through the degradation chain and back into the mix, creating a synthesis that records itself — and no preset used it.*

*After this retreat, none of that is true.*

*What has changed is not the engine. The engine was always this deep. What changed is the attention.*

*The ocelot was always in the forest. The forest was always a system. The system was always communicating across twelve routes. We just had to sit still long enough to hear it.*

*Go. Activate the routes. Make the understory the source. Set the canopy trigger to call the creatures. Make the wolf howl higher when the forest moves faster. The ecosystem is waiting for you to be curious enough to find out what it actually is.*

---

## Phase R7: Refinement Log

### Refinement 1: Reveal the Unexplored Routes
**Finding:** Eight of twelve EcosystemMatrix routes default to 0.0 and are set to 0.0 in virtually all existing presets. The `xf_floorUnder`, `xf_underFloor`, `xf_underCanopy`, `xf_underEmerg`, `xf_canopyUnder`, `xf_canopyEmerg`, `xf_emergFloor`, `xf_emergUnder` routes are effectively invisible to users.

**Recommendation:** Document all 12 routes with descriptions and musical examples. Add four new presets that each demonstrate a single unexplored route in isolation: "Floor Pauses the Rain" (`xf_floorUnder` negative), "The Understory Speaks" (`xf_underEmerg` positive), "Creatures Light the Canopy" (`xf_emergCanopy`), "Canopy Records Itself" (`understorySrc=1.0`). These four presets function as tutorial sounds embedded in the preset library.

**Status:** Addressed in the Ten Awakenings — Presets 1, 6, 4, and 8 each isolate one unexplored route as their primary discovery.

### Refinement 2: biome Parameter Documentation
**Finding:** The three biomes (Jungle, Underwater, Winter) transform every stratum's behavior in meaningful ways, but the transformations are not documented for users. A preset labeled "kalimba" behaves completely differently in Winter biome (octave up, longer Q) versus Jungle (default) versus Underwater (octave down, glass marimba quality). Users have no way to know this without exploration.

**Recommendation:** Add biome transformation notes to the sound design guide. Include the specific parameter offsets: Winter kalimba = freqMul×2 + qMul×1.5; Underwater kalimba = freqMul×0.85 + qMul×1.3. Label presets with biome as a primary descriptor, not a secondary tag.

**Status:** Documented in Phase R2 above. Presets 2 (Underwater berimbau), 5 (Winter agogo), 9 (Winter kalimba) demonstrate biome-specific behavior.

### Refinement 3: understorySrc as Synthesis Route
**Finding:** `understorySrc` (0.0-1.0) crossfades between floor output and canopy output as the understory's source material. At 1.0, the canopy additive pad is routed through the bit-crush, sample-rate reduction, tape wobble, and tape age chain. This creates a feedback synthesis path — the canopy generates material that is degraded and returned to the mix — that is completely unexplored in existing presets.

**Recommendation:** Add presets specifically demonstrating `understorySrc` at 0.5 (equal blend) and 1.0 (full canopy), particularly in Underwater biome where the canopy is already tonally dark and the degradation creates a hydrophone-quality texture.

**Status:** Addressed in Preset 8 (Understory Whale, `understorySrc=1.0`).

### Refinement 4: Negative Cross-Feed Routes as Musical Feature
**Finding:** All 12 cross-feed routes accept negative values (bipolar -1 to +1), but existing presets only use negative values for `xf_canopyFloor` (the default -0.1). Negative routes produce opposite effects: negative `xf_floorUnder` pauses the understory during floor hits (rhythmic opposition); negative `xf_emergUnder` silences the understory on creature calls. These rhythmic opposition effects have no analog in conventional synthesis and could be a distinctive OCELOT sound design technique.

**Recommendation:** Document negative routes as "rhythmic opposition" — a first-class technique. Add at least two presets using negative cross-feed values creatively.

**Status:** Addressed in Preset 1 (Kalimba and Rain, `xf_floorUnder=-0.25` creates rain silence on note strikes).

---

*End of OCELOT Retreat Chapter*
*Guru Bin — XO_OX Designs — 2026-03-21*
