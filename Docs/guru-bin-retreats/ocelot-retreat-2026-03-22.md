# Guru Bin Retreat — OCELOT (XOcelot)
*Retreat conducted: 2026-03-22*
*Engine accent: Ocelot Tawny `#C5832B`*
*Parameter prefix: `ocelot_`*
*Seance score: ~8.5/10 (post D002/D004 fix pass 2026-03-21)*
*The Flock convened for a full-day immersion*

---

## Phase R1: Pilgrimage — What We Found

### Architecture: The Four-Stratum Ecosystem

OCELOT is unique in the fleet: it is not one synthesizer. It is **four synthesizers living in the same body**, communicating through a routing matrix the engine calls the EcosystemMatrix.

**The Four Strata:**

1. **Floor** (OcelotFloor) — Six physical percussion models: Berimbau (KarplusStrong + gourd resonator + pitch sweep), Cuica (friction oscillator + SVF bandpass), Agogo (struck two-tone bell, inharmonic 2.67x), Kalimba (three-partial tine modal, inharmonic 2.76x + 5.4x, DEFAULT), Pandeiro (membrane + jingle burst), Log Drum (slit resonator + wooden body). Every model is pre-allocated. No audio-thread allocation.

2. **Understory** (OcelotUnderstory) — A grain chopper and sample mangler running on a 65536-sample ring buffer (~1.5s at 44.1k). Pipeline: internal oscillator (saw/sine blend) → chop gate (mute 30% of segments deterministically via floorPattern hash) → bit crusher → tape warp → vinyl dust. Can receive external audio via coupling instead of internal oscillator.

3. **Canopy** (OcelotCanopy) — A spectral additive pad of up to 8 partials with Buchla-style wavefold, shimmer feedback (read at 0.5x speed = +1 octave, hard-capped at 0.88 feedback), breathe LFO, and a per-partial log spectral filter (200Hz–20kHz).

4. **Emergent** (OcelotEmergent) — Three SVF bandpass formant filters in parallel over noise excitation, producing creature calls from a table of 6 types: Bird (800/2500/5000 Hz), Whale (80/220/540 Hz), Insect (1800/3800/7600 Hz), Frog (350/1000/2200 Hz), Wolf (260/720/1600 Hz), Synth (user-controlled via pitch). Auto-periodic or threshold-triggered.

### The EcosystemMatrix: 12 Typed Routes

The matrix is the central innovation. Every block, output signals from the four strata are collected into `StrataSignals` and routed through 12 typed cross-feed paths into `StrataModulation`. Three route types:

- **Continuous** (8 routes): linear bipolar scalar. Negative amount inverts.
- **Threshold** (2 routes): sigmoid gate (k=8). **Negative amount = INVERSE TRIGGER** (fires on silence, not peaks). This is the hidden gem of the engine.
- **Rhythmic** (2 routes): stepped. Negative = rhythmic opposition (fast source = slow destination).

Default pre-wired routes give the ecosystem audible behavior from first touch:
- Floor→Canopy +0.2: every drum hit opens the spectral filter
- Floor→Emergent +0.25: loud hits trigger creature calls
- Emergent→Canopy +0.15: creature calls add shimmer
- Canopy→Floor -0.1: bright canopy subtly damps floor resonance

The master `ecosystemDepth` scales all 12 mod outputs. It also receives from aftertouch (+0.3 sensitivity) and mod wheel (+0.35), and is continuously modulated by a 0.07 Hz autonomous LFO (predator-prey cycle, ~14-second period) per the D005 fix.

### Biome System: Three Full Ecosystem Profiles

Three biomes (Jungle, Underwater, Winter) each transform all four strata simultaneously via 11 interpolated `BiomeProfile` fields:

| Field | Jungle | Underwater | Winter |
|---|---|---|---|
| floorDampingBase | 0.0 | +0.3 (submerged resonance) | -0.2 (brittle) |
| floorBrightnessBase | +0.1 | -0.5 (dark) | +0.4 (crystalline) |
| understoryWobbleBase | 0.0 | +0.4 (viscous) | +0.15 (cold mechanism) |
| understoryBitShift | 0.0 | +2.0 (softer) | -1.5 (crunchier) |
| canopyPartialTilt | 0.0 | -0.4 (lows dominate) | +0.3 (highs dominate) |
| canopyBreathePeriod | 1.0x | 3.0x (tidal, slow) | 0.5x (wind gusts, fast) |
| emergentPitchRange | 0.5 | 0.8 (whale sweep wide) | 0.35 (wolf howl narrow) |
| emergentDecayBase | 0.0 | +0.4 (long tails) | +0.2 (carries far) |
| reverbPreDelayMs | 20ms | 60ms (sound travels slower) | 10ms (snow absorbs) |
| reverbDiffusion | 0.75 | 0.92 (ocean cavern) | 0.55 (sparse) |

Transitions are smoothly interpolated over 200ms with `BiomeMorph`. Each biome also transforms specific model behaviors: Kalimba in Winter becomes ice chimes (2x frequency, Q×1.5). Cuica in Underwater becomes a sonar ping (less pitch bend, more resonance). Agogo in Winter adds a 4.5x third partial.

---

## Phase R2: Silence — The Init Voice

With all parameters at default, OCELOT sounds like:

A **kalimba** playing a sparse pattern (floorPattern=0), its tines (at the inharmonic 1x / 2.76x / 5.4x ratios) rising bright in the Jungle biome. Beneath it, a canopy of 4 additive partials breathes slowly (0.3 Hz breathe, 0.3 rate). Bird calls fire from the floor trigger route (floorEmerg=0.25) at medium velocity. A light reverb tail (0.6 size, 0.3 mix) holds the space. The ecosystem matrix is live at default: each kalimba strike brightens the canopy spectral filter slightly (+0.2 position), and creature calls add shimmer (+0.15). A 14-second predator-prey drift cycle modulates ecosystemDepth invisibly. The voice before the words is: *a courtyard in a tropical garden at dusk. Metal tines, distant birds, and a warm haze.*

---

## Phase R3: Awakening — What the Flock Found

### The Finger: Parameter Interactions That Are Unique

**1. The Inverse Threshold Route.** Setting `xfFloorEmerg` to a negative value (e.g., -0.55) causes the ecosystem to fire creature calls in the SILENCE between floor beats, not on the peaks. The sigmoid's source is inverted: `s = (1.0 - source)`. This is rare territory. No other engine in the fleet has a negative-polarity ecological trigger. It creates sounds that speak between the beats.

**2. Shimmer→Formant Feedback Loop.** Routing `xfEmergCanopy` high (e.g., 0.6) feeds creature call amplitude as shimmer into the canopy. Simultaneously, routing `xfCanopyEmerg` high (e.g., 0.55) feeds canopy spectral position into emergent formants. The result: every creature call adds shimmer; that shimmer changes spectral centroid; the changed centroid shifts the next call's formant position; which produces a different shimmer level; which changes spectral centroid again. A genuine feedback ecology that evolves differently every note.

**3. Biome-Aware Per-Model Physics.** Each floor model responds to biome differently. The Kalimba becomes ice chimes in Winter. The Cuica becomes a sonar ping in Underwater. The Agogo gains a 4.5x third partial in Winter. This is not a simple EQ tilt — the physical model parameters change.

**4. The ecosystemDepth Triple Source.** ecosystemDepth is simultaneously modulated by: the parameter value, aftertouch (+0.3), mod wheel (+0.35), and the 14-second autonomous predator-prey LFO (±20%). Maximum ecosystemDepth from all sources = 1.0 + 0.3 + 0.35 + clamp = well above base value, but always clamped. The mod wheel is the most powerful expression input in this engine.

**5. Deterministic Mute Patterns.** The Understory chop gate uses `floorPattern` as a seed to a deterministic hash for deciding which 30% of segments to mute. This means `floorPattern` controls the rhythmic character of the Understory without any randomness — the same preset always has the same mute pattern. Changing floorPattern changes the Understory groove.

### The Breath: LFO Character

The engine breathes through three autonomous rhythms:
- **Canopy breathe LFO** (0.3 Hz default / period multiplied per biome). Modulates amplitude of the entire canopy layer. In Winter (0.5x), it gusts. In Underwater (3.0x), it tides.
- **Ecosystem drift LFO** (0.07 Hz / ~14-second cycle). Modulates the full cross-feed depth. The ecosystem becomes more or less interactive over geological time.
- **Emergent auto-periodic** (creatureRate: 4.0s → 0.1s). The creatures have their own clock.

These three LFOs have no phase coupling. Their interaction creates organic polyrhythmic breath: the canopy breathes, the ecosystem pulses, and the creatures call — all on different clocks.

### The Tongue: Velocity Response

- **D001 filter envelope** is wired to the canopy spectral filter position. At `filterEnvDepth=0.55` and full velocity, the canopy opens by +0.165 spectral units (~+3300 Hz at mid position). PP playing produces a dark, muffled canopy. FF playing is bright and open. This is most audible on Cuica Lead.
- **Emergent** multiplies level by velocity (`envAmp * creatureLevel * baseVelocity`). Hard notes produce louder creature calls.
- **Floor** models all receive velocity as the initial excitation amplitude. Kalimba and Berimbau have very strong velocity response (harder hit = more overtones from modal synthesis).

### The Ear: Spectral Signature

OCELOT lives in the **mid-warm** range by default. The kalimba tines (2.76x / 5.4x inharmonic) place energy in the 500–3000 Hz range. The canopy 4-partial sine bank with 1/harmonic rolloff sits below 2kHz. Bird calls (800/2500/5000 Hz) provide the only significant high-frequency content. In Underwater biome, most of the sound drops below 1kHz. In Winter, energy rises to 4–8 kHz. It sits naturally **under** leads and vocals; use CANOPY macro to bring it forward.

### The Eye: Is biome the central gesture?

Yes, unambiguously. The biome parameter is the one parameter that transforms the entire synthesis personality simultaneously. More than any single stratum parameter, changing biome reveals what OCELOT truly is: an engine designed to model ecosystem-as-environment, not just instrument-as-sound. The 200ms crossfade means biome changes can be performed in real-time.

### The Bone: CPU Observations

The engine reports ~6% CPU at 44.1k on M1 (source: OcelotVoice.h comment). Per-sample cost is dominated by the Emergent SVF filters (3 formant filters × `fastTanh` saturation) and the Canopy partial bank (up to 8 partials × sin per sample). The `fastSin`/`fastTanh`/`fastPow2` SRO approximations in FastMath.h replace all `std::` trig calls. The biome crossfade (11-field linear interpolation) runs per-block, not per-sample. No CPU concerns.

---

## Phase R4: Deep Fellowship — Cross-Pollinated Discoveries

**From The Finger to The Bone:** The deterministic hash for Understory mute patterns (using `floorPattern` as seed) makes the Understory rhythmically coupled to the Floor without any matrix route. It's structural coupling: changing the floor model's pattern simultaneously changes the Understory's groove.

**From The Tongue to The Eye:** The biome changes the velocity response indirectly: Winter's higher Q kalimba (Q×1.5) means the same velocity produces more sustain. PP in Winter sounds different from PP in Jungle even if the filter envelope is identical.

**From The Breath to The Ear:** The three async LFOs ensure OCELOT is never spectrally static. Even a held note drifts: the canopy breathe changes amplitude; the ecosystem drift changes cross-feed depth (more flux in the matrix); the creatures pulse. At no two moments is the spectrum identical.

**The Shimmer Feedback Loop (confirmed):** The `Emergent→Canopy shimmer` + `Canopy→Emergent formant` dual route creates a feedback ecology with stable behavior (not runaway) because shimmer is capped at 0.88 and formant mod is only ±0.3 octaves. It enriches rather than destabilizes.

---

## Phase R5: The 10 Awakening Presets

| # | Name | Mood | Primary Character | Key Discovery Demonstrated |
|---|------|------|-------------------|---------------------------|
| 1 | Jungle Morning | Atmosphere | Lush pad, Jungle | Full 8-partial canopy + tidal breathe + bird calls from canopy peaks |
| 2 | Oceanic Drift | Submerged | Deep pad, Underwater | Whale creature calls + 3x breathe period + ocean cavern reverb |
| 3 | Frozen Canopy | Prism | Crystalline pad, Winter | Ice chime kalimba + high-partial tilt + wolf howl creature |
| 4 | Wood Stalker | Foundation | Textured lead, Berimbau | KarplusStrong pitch sweep + strong floor→canopy filter route |
| 5 | Agogo Pulse | Foundation | Rhythmic texture | Agogo bell polyrhythm + full Floor→Understory rhythmic route |
| 6 | Tape Jungle | Flux | Lo-fi rhythmic texture | Heavy Understory: bit crush + tape warp + Understory→Canopy morph |
| 7 | Biome Crossroads | Aether | Evolving atmosphere | All routes active at moderate level — designed for real-time biome switching |
| 8 | Creature Hour | Atmosphere | Emergent-forward | Canopy↔Emergent feedback loop: shimmer→formant→shimmer |
| 9 | Cuica Lead | Foundation | Textured lead, friction | Cuica friction oscillator + high tension SVF squeal |
| 10 | Silence Triggered | Flux | Rhythmic negative-space | Inverse threshold: creature calls fire in the silence between Pandeiro beats |
| 11 | Tawny Gold | Entangled | Holy grail | Full ecosystem balance: Berimbau + 7-partial canopy + full shimmer feedback loop |

*(11 written total — the 10 awakening + the holy grail as a standalone Entangled preset)*

### File Locations

- `Presets/XOmnibus/Atmosphere/Ocelot_JungleMorning.xometa`
- `Presets/XOmnibus/Submerged/Ocelot_OceanicDrift.xometa`
- `Presets/XOmnibus/Prism/Ocelot_FrozenCanopy.xometa`
- `Presets/XOmnibus/Foundation/Ocelot_WoodStalker.xometa`
- `Presets/XOmnibus/Foundation/Ocelot_AgogoPulse.xometa`
- `Presets/XOmnibus/Flux/Ocelot_TapeJungle.xometa`
- `Presets/XOmnibus/Aether/Ocelot_BiomeCrossroads.xometa`
- `Presets/XOmnibus/Atmosphere/Ocelot_CreatureHour.xometa`
- `Presets/XOmnibus/Foundation/Ocelot_CuicaLead.xometa`
- `Presets/XOmnibus/Flux/Ocelot_SilenceTriggered.xometa`
- `Presets/XOmnibus/Entangled/Ocelot_TawnyGold.xometa`

---

## Phase R6: Engine Scripture — Verse Candidates

### VERSE: The Negative Threshold
**TRUTH:** When a route amount is negative, the ecosystem does not grow quieter — it inverts. Silence becomes the trigger. The absence of the floor activates the forest. The rest is not empty; it is when the animals speak. Design presets around the silences as deliberately as around the beats.
**DOMAIN:** Modulation

### VERSE: The Biome Is Not a Tone Control
**TRUTH:** A biome is not an EQ preset. It transforms the physical models, the breathe rate, the reverberation character, the creature pitch range, and the partial balance simultaneously. Changing biome mid-performance changes what kind of world the instrument believes it is in. Use it as a dramaturgy tool, not a timbre tool. The ocelot in the jungle and the ocelot in the winter are different animals.
**DOMAIN:** Oscillator

### VERSE: The Ecosystem Remembers One Block Behind
**TRUTH:** The EcosystemMatrix operates on a one-block lag — it reads signals from the previous block and writes modulations for the current block. This is not a bug; it is the physics of the forest. No animal responds instantaneously. The one-block lag creates causality: each stratum acts, the matrix observes, and only then do the other strata respond. Sound designers should design cross-feed routes as ecological laws, not real-time controls.
**DOMAIN:** Coupling

### VERSE: Three Biomes Are Three Engines
**TRUTH:** The Kalimba in Winter is not the same instrument as the Kalimba in Jungle. It becomes ice chimes: doubled frequency, Q×1.5 ring, brittle brightness. The Cuica in Underwater stops bending and resonates like sonar. The Agogo in Winter grows a third partial at 4.5x. When you choose a floor model, you choose six instruments, not one. When you choose a biome, you choose three of those six.
**DOMAIN:** Oscillator

---

## Phase R7: Summary of Key Discoveries

**The engine's unique identity:**
OCELOT is the only engine in the fleet that models an ecological *relationship* between synthesis layers rather than treating them as independent signal paths. The EcosystemMatrix is its central invention. No other engine has continuous, threshold, and rhythmic typed routes between four dedicated synthesis layers. No other engine has negative-polarity inverse threshold triggering. No other engine has biome-aware per-model physical transformations.

**The overlooked power:**
The inverse threshold route (`xfFloorEmerg < 0`) is completely unexplored in the Init preset and virtually unknown to users. It produces sounds that speak in negative space — the rest is the signal.

**The highest emotional peak:**
The shimmer→formant feedback ecology (Emergent→Canopy shimmer + Canopy→Emergent formant). When both routes are high and ecosystemDepth is deep, the ecosystem becomes a true feedback organism, evolving differently every note.

**The missing conversation:**
The Log Drum (model 5) is the darkest floor model (brightness proxy 0.28). In Underwater biome (oceanic thud: heavier body resonance), it becomes the lowest-frequency voice in the entire OCELOT palette. Almost no existing presets explore this territory.

**The producer moment:**
One note of **Tawny Gold** (Entangled), held for 30 seconds. The Berimbau pitch sweep peaks and decays. The canopy opens as the floor trigger sends the spectral filter up. A bird call fires, adding shimmer. The shimmer shifts the formant of the next call. The 14-second ecosystem drift LFO begins its first wave. By bar 3, something is different from bar 1, and the producer cannot explain why. They do not stop playing.

---

*Guru Bin Retreat — OCELOT — 2026-03-22*
*The Flock: The Finger, The Breath, The Tongue, The Ear, The Eye, The Bone*
*11 presets deposited. 4 scripture verse candidates submitted.*
