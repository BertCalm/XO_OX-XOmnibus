# OVERBITE Retreat Chapter — Vol 2
*Guru Bin Transcendental — 2026-03-21*

---

## Engine Identity

- **Gallery code:** OVERBITE | **Accent:** Fang White `#F0EDE8`
- **Parameter prefix:** `poss_`
- **Creature mythology:** The Virginia opossum — North America's only marsupial. Pointed grey-white face, bare pink tail, small dark eyes. Its most famous survival behavior is thanatosis: when cornered by a predator, it collapses entirely, going limp and unresponsive, releasing a foul-smelling chemical from its anal glands. It does not decide to play dead. Its nervous system does it without voluntary input — a biochemical paralysis triggered by extreme fear. The opossum wakes with no memory of the episode. The behavioral repertoire has five modes: BELLY (foraging, warm, unguarded), BITE (defensive, sharp, territorial), SCURRY (fleeing, fast, kinetic), TRASH (rummaging, chaotic, curious about garbage), and PLAY DEAD (thanatosis — the complete surrender).
- **Synthesis type:** Bass-forward character synthesizer — OscA (Belly, 4 waveforms with drift) + OscB (Bite, 5 modes: Hard Sync Saw, FM, Ring Mod, Noise, Grit) + Sub Oscillator (-1 or -2 oct) + Weight Engine (5 shapes, 3 octaves) + Noise Source (5 types with decay envelope), BiteFur (pre-filter tanh saturation), BiteChew (contour compression), BiteGnash (asymmetric waveshaper — "the Bite"), BiteTrash (3 dirt modes: Rust/Splatter/Crushed), CytomicSVF filter (4 modes, key tracking, drive), 3 LFOs (7 shapes), 8-slot mod matrix, 5 behavioral macros, 4 FX stages (Motion/Echo/Space/Finish)
- **Polyphony:** 1–16 voices, unison up to 8, glide (legato/always)
- **feliX/Oscar polarity:** 60% Oscar / 40% feliX — survival instinct dominant, with moments of feral brightness
- **Seance score:** 9.2/10 (joint highest in the fleet)
- **Macros:** M1 BELLY (warmth/sub weight), M2 BITE (gnash/drive/bite depth), M3 SCURRY (LFO rate/mod speed), M4 TRASH (dirt mode intensity), M5 PLAY DEAD (harmonic collapse/stillness)
- **Blessings:** B008 — Five-Macro System (BELLY/BITE/SCURRY/TRASH/PLAY DEAD) — ratified unanimously. The only engine in the fleet with macros that map directly to an animal's complete behavioral ethogram.
- **Expression:** Velocity → amplitude AND filter brightness (dual-path D001). Aftertouch → `poss_oscBInstability` (+0.4 range). Mod wheel CC1 → `poss_macroBite` (+0.4 range).

---

## Pre-Retreat State

OVERBITE arrived at 9.2/10 in the seance — the joint highest score in the fleet. B008 was ratified unanimously: the five-macro behavioral system is genuinely novel architecture. No other synthesizer has ever mapped its macros to an ethogram.

The factory library has 77 presets as of this retreat. It is strong but approaches OVERBITE through a narrow axis. An inventory of the 77 presets by dominant macro:
- BITE-dominant or BITE/SCURRY hybrid: approximately 35 presets
- BELLY-moderate: approximately 18 presets
- TRASH-significant: approximately 12 presets
- PLAY DEAD-dominant: 8 presets (mostly in Aether, a few in Atmosphere)
- Pure BELLY, near-zero BITE: approximately 4 presets

This means approximately 57% of the factory library lives in BITE/SCURRY territory. The full behavioral range — particularly the vulnerable, still, warm end — is underrepresented.

The Vol 2 retreat focuses on three territories:

**1. PLAY DEAD at compound interaction with BELLY.** The factory PLAY DEAD presets (Aether/Atmosphere) correctly use PLAY DEAD as a collapsing, stillness-inducing macro. What they have not explored is PLAY DEAD combined with high BELLY — the state where the opossum is physiologically limp but metabolically warm underneath. There is a genuine sonic discovery here: a sound that is externally still but internally warm, with sub oscillator persisting and filter barely cracked, body present but unmoving.

**2. BELLY as the primary voice of the instrument, BITE completely absent.** BELLY at 0.9 without BITE creates OVERBITE in its most generous, open, warm state. The Fur saturation without Gnash gives warmth without aggression. The OscA in sine mode (pure sub weight) without OscB creates a fundamental-dominant sound with body but no edge. The factory library does not have a single preset where BELLY is above 0.85 and BITE is below 0.05.

**3. Mod matrix as behavioral expression.** The 8-slot mod matrix is almost completely unused in the factory library. With three LFOs (7 shapes each), a filter envelope, a mod envelope, and velocity/aftertouch as sources, the mod matrix can route e.g. LFO3 → poss_oscBInstability (animating the Bite's pitch jitter in sync with a musical rate), LFO2 → poss_gnashAmount (breathing gnash depth on and off slowly), velocity → poss_furAmount (harder hits = more fur saturation). The factory library treats OVERBITE as a macro-controlled instrument. The Transcendental presets treat it as a modulation-routed one.

---

## Phase R1: Opening Meditation

Close your eyes. Breathe.

It is 2:00 AM. You are driving down an empty side street in a residential neighborhood. Your headlights catch something low and grey-white at the edge of the road. You slow down. A Virginia opossum — female, large, moving with the measured, deliberate walk of an animal that knows it belongs here — crosses the pavement.

She does not flee. She turns her face toward the headlights for a moment, unmoved. Small dark eyes. The quiet face. Then she continues. She is not afraid of you. She is simply occupied.

This is the BELLY state: warm, purposeful, unguarded. She is foraging. Her attention is entirely on the next thing she will eat. The universe has contracted to the smell of the compost bin two houses down. Everything else is either threat or not-threat. You are not-threat. She walks.

Now imagine a car door slamming behind you. She freezes. For a half-second she stands, every sense alive. Then — she is gone. Under the fence in under a second. The SCURRY reflex. No decision. Pure motion.

This is the engine's full range: from the patient walk to the survival sprint to the complete collapse into thanatosis. Every preset in the OVERBITE library lives somewhere on this behavioral continuum. The Transcendental presets inhabit the ends of the continuum that the factory library has not fully explored: the warm patience and the total surrender.

The opossum is not a simple animal. Its thanatosis is involuntary. Its warmth is metabolic necessity. Between the two is a complete character — one of the most misunderstood animals in North America and one of the most expressive synthesizers in the fleet.

---

## Phase R2: The Signal Path Journey

### I. OscA (BELLY) — The Engine's Metabolic Core

OscA is labeled "Belly" in the source code. Its four waveforms represent the opossum's resting postures:

**Wave 0 (Sine):** The sub weight core. At `poss_oscAShape = 0.0`, a pure sine — foundational, clean. As shape rises toward 1.0, soft even harmonics appear via `raw + shape × 0.3 × raw²`. At shape=1.0, the sine is slightly fattened with even harmonics — warm but not bright. The purest BELLY sound is sine at shape=0.2: weight without edge.

**Wave 1 (Triangle):** Morphs toward saw as shape increases. At shape=0.0, a soft filtered triangle. At shape=1.0, the triangle has fully morphed to saw via `lerp(tri, saw, shape)`, filtered through the `triState` leaky integrator. The midpoint (shape=0.5) is a hybrid waveshape with properties of neither — unique to this implementation, warmly bright. Good for BELLY-moderate patches that need midrange body.

**Wave 2 (Saw):** Shape controls warmth rolloff: `rolloff = 0.6 + (1-shape) × 0.35`. Shape=0 is warmest (rolloff=0.95 of raw amplitude), shape=1 is brightest (rolloff=0.6). Counter-intuitive: high shape = brighter saw. The default shape=0.5 gives a 0.775 rolloff — a warm-to-midrange saw. Low shape values (0.0–0.2) give the warmest possible saw — nearly a bandlimited sine at low frequencies.

**Wave 3 (Cushion Pulse):** Pulse width from `0.15 + shape × 0.7`. At shape=0: narrow 15% pulse — thin, bright, buzzy. At shape=0.5: 50% pulse, pure square. At shape=1.0: 85% pulse — nearly-full-on-time, hollow and buzzy again. The warm center (square at 0.5) is the widest sound. For BELLY-forward presets, shape=0.45–0.55 on the pulse is the correct range.

**Drift:** OscA drift (`poss_oscADrift`) uses a 0.37 Hz sine LFO for pitch variation, scaled by `driftAmount × 0.003`. At `poss_oscADrift = 0.3`, pitch variation is approximately ±0.9 cents — barely perceptible but makes the sound feel organic. At 0.6, it is ±1.8 cents — audible as warmth. The sweet spot for PLAY DEAD + BELLY presets is `poss_oscADrift = 0.08–0.12`: the sound is technically drifting but the ear reads it as "alive" rather than "out of tune."

### II. OscB (BITE) — The Engine's Defense Mechanism

OscB is labeled "Bite." Its five modes are the opossum's response options when BELLY-foraging encounters a threat:

**Mode 0 (Hard Sync Saw):** Shape controls sync ratio from 1x to 4x (the slave runs at 1× to 4× the master frequency). At shape=0.5, the slave runs at 2.5× the master — this is a non-integer ratio that creates complex, inharmonic sync content. At shape=0.33, the slave runs at 2× — a clear octave sync. The instability parameter adds pitch jitter to the sync, making the sync texture irregular and organic.

**Mode 1 (FM):** The internal FM modulator runs at 2× the voice frequency. Shape controls FM index from 0 to 2. At shape=0.3, FM index is 0.6 — mild sidebands. At shape=0.8, FM index is 1.6 — significant spectral spreading. The external FM input (`externalFM`) also contributes to FM depth — OVERBITE can receive FM from coupling with ONSET or OBLONG.

**Mode 2 (Ring Mod):** Shape morphs the carrier from sine (shape=0) to saw (shape=1), ring-modulated against OscA. At shape=0.5, the carrier is a morphed blend — the ring modulation produces sum and difference frequencies. With OscA at sine, the ring output is a clean pair of sidebands. With OscA at saw, the ring output multiplies all of saw's harmonics.

**Mode 3 (Noise):** Shape controls color from white (shape=0) to brown (shape=1 via leaky integrator integration). At shape=0.3, a soft noise color between white and pink — useful as a texture layer under BELLY fundamentals without sounding like a separate noise source.

**Mode 4 (Grit):** Bitcrusher from 64 steps (shape=0) to 8 steps (shape=1). This is the most aggressive OscB mode — it turns the saw waveform into a stepped, digital-sounding artifact. The TRASH macro territory overlaps here.

**The BELLY + BITE interaction — the most important parameter relationship in the engine:**

`poss_oscMix` controls the crossfade between OscA and OscB. At `poss_oscMix = 0.0`, pure OscA (BELLY). At `poss_oscMix = 1.0`, pure OscB (BITE). The sweet spot for the most expressive OVERBITE presets is NOT at 0.0 or 1.0 but at 0.25–0.45: primarily BELLY with BITE as a texture underneath. In this range, OscB contributes brightness or aggression without becoming the primary voice. This is the opossum showing its teeth from a distance — not biting, just revealing the capability.

### III. Sub Oscillator + Weight Engine — The Sub Weight Architecture

`poss_subLevel` (0.0–1.0) and `poss_subOctave` (0 = -1 oct, 1 = -2 oct) provide clean sub reinforcement. The sub is a pure sine, reliable, and always available. For BELLY-dominant presets, `poss_subLevel = 0.6–0.8` is the correct default — the sub provides the opossum's metabolic warmth below the articulation of the main oscillators.

The Weight Engine (`poss_weightLevel`, `poss_weightShape`, `poss_weightOctave`, `poss_weightTune`) adds a second sub-range oscillator with 5 shapes (sine, triangle, saw, square, narrow pulse) at -1, -2, or -3 octaves. It is largely unused in the factory library. The narrow pulse shape (mode 4) at -3 octaves provides a sub-click reinforcement — useful for adding presence to sub bass without adding pitched harmonic content.

**Compound sub strategy:** `poss_subLevel = 0.65` (pure sine -1 oct) + `poss_weightLevel = 0.18` (saw at -2 oct) — the clean sine provides fundamental weight, the subtle saw at two octaves below adds warmth to the sub region without competing with the main oscillator midrange. This combination is absent from the factory library.

### IV. BiteFur + BiteChew + BiteGnash — The Character Shapers

**BiteFur** (`poss_furAmount`): Pre-filter tanh soft saturation. Drive = `1 + furAmount × 6`. At `poss_furAmount = 0.15`, drive is 1.9 — significant tanh input compression giving even harmonic warmth. The fur saturates BEFORE the filter, meaning the filter shapes the saturated signal. For BELLY presets: `poss_furAmount = 0.12–0.20` — warmth without edge.

**BiteChew** (`poss_chewAmount`, `poss_chewFreq`, `poss_chewMix`): Post-filter soft-knee compressor approximation. Adds sustain and body. At `poss_chewAmount = 0.25`, threshold is `1 - 0.25 × 0.7 = 0.825` — compression begins at 82.5% of maximum amplitude. This compresses dynamics gently without obvious limiting. Best for BELLY presets: `poss_chewAmount = 0.2–0.3`.

**BiteGnash** (`poss_gnashAmount`): Asymmetric waveshaper — positive half clips harder ("the bite"), negative half gets softer tanh curve ("the belly"). The asymmetry is the engine's character: positive wave cycles get harder treatment, negative cycles get softer treatment. At `poss_gnashAmount = 0.0`, completely bypassed. At 0.3, a mild asymmetric character that adds harmonic complexity without obvious distortion. At 0.8, the bite is aggressive — positive peaks hard-clipped, the sound acquires a gnashing quality. For PLAY DEAD + BELLY presets: `poss_gnashAmount = 0.0` (total absence of gnash). For compound character presets: 0.15–0.25 adds gnash flavor while maintaining body.

### V. The Five Behavioral Macros — OVERBITE's Architecture

B008: The Five-Macro System is the engine's most important architectural feature. Each macro fan-out affects multiple parameters simultaneously:

**BELLY (`poss_macroBelly`):** Increases sub level and oscillator body weight. At 0.9, BELLY-dominant: maximum sub presence, soft filter, warm timbre.

**BITE (`poss_macroBite`):** Increases gnash, drive, and bite depth. At 0.7, BITE-dominant: aggressive gnash, filter drive engaged, OscB mix increases. At full BITE, the opossum reveals its teeth.

**SCURRY (`poss_macroScurry`):** Increases LFO rates and modulation speed. The sound accelerates — faster filter sweeps, faster pitch movement. The behavioral state of flight.

**TRASH (`poss_macroTrash`):** Activates BiteTrash dirt modes. Rust/Splatter/Crushed add bitcrushing, wavefolding, and hard clipping respectively. The opossum in the garbage.

**PLAY DEAD (`poss_macroPlayDead`):** Collapses harmonic content toward stillness. Filter closes, amp becomes nearly static, gnash reduces, the sound approaches silence-from-warmth. The involuntary survival response.

**The unexplored intersections:**
- `BELLY = 0.85, PLAY DEAD = 0.7`: Metabolically warm but externally still. The opossum alive inside its paralysis.
- `BELLY = 0.75, BITE = 0.15`: Almost pure warmth with a just-visible edge — showing teeth from a safe distance.
- `PLAY DEAD = 0.6, TRASH = 0.4`: Partially collapsed with persistent grime — not fully dead, not fully alive. Liminal.
- `SCURRY = 0.7, PLAY DEAD = 0.3`: The panic that precedes the collapse. Fast modulation against arrested harmonic content.

### VI. The Mod Matrix — The Engine's Unexplored Territory

The 8-slot mod matrix (`poss_modSlot1Src` through `poss_modSlot8Amt`) connects 3 LFOs + 3 envelopes + velocity + aftertouch as sources to filter cutoff, osc mix, gnash, fur, drive, and other destinations.

The factory library almost never uses the mod matrix. This is the Vol 2 retreat's primary discovery territory for OVERBITE.

**High-value routings:**
- LFO3 (shape 4 = S&H) → `poss_oscBInstability`: Random instability bursts synchronized to LFO3 rate — the Bite oscillator gets random pitch jitter events at a musical rate. Not continuous instability but periodic ones.
- LFO2 (shape 0 = Sine) at 0.04 Hz → `poss_gnashAmount`: Gnash depth breathes at breathing rate (Sutra III-1). The asymmetric waveshaper cycles on and off imperceptibly — the opossum's teeth appear and disappear below conscious detection.
- Velocity → `poss_furAmount`: Harder hits saturate more — the opossum's fur bristles proportional to impact.
- Aftertouch → `poss_filterReso`: Press harder into a held note to increase resonance — a vocal-adjacent expression.

---

## Phase R3: Parameter Refinements

| Parameter | Finding | Recommendation | Rationale |
|-----------|---------|----------------|-----------|
| `poss_furAmount` | Default 0.0 in most factory presets | Consider 0.08 as active floor in BELLY presets | Fur at 0.08 adds warmth without audible distortion — the Belly's fur is implied, not applied |
| `poss_subLevel` | Default varies 0.3–0.6 in factory | For BELLY presets: 0.65–0.75 recommended | The sub is the opossum's body weight; BELLY presets undersell the sub |
| `poss_chewAmount` | Default 0.0 in most factory | 0.2–0.25 adds contour to BELLY-dominant sounds | Compression without limiting — the body feels present |
| `poss_oscADrift` | Default 0.05–0.1 in factory | 0.08–0.12 sweet spot for PLAY DEAD + BELLY | Alive without unstable — the metabolic warmth of an animal in thanatosis |
| `poss_lfo1Rate` | Factory uses 0.4–4.0 Hz overwhelmingly | Explore 0.04–0.08 Hz for BELLY/PLAY DEAD | The breathing rate that makes the sound feel metabolic rather than modulated |
| `poss_macroBelly` | Max in factory: ~0.75 | Push to 0.85–0.95 in Transcendental BELLY presets | The unexplored territory IS the deep BELLY |
| `poss_weightLevel` | Almost zero across factory library | 0.15–0.25 in BELLY + compound sub presets | The Weight Engine adds warmth to the sub region the Sub Osc alone doesn't provide |
| Mod matrix | Nearly unused across 77 factory presets | Wire at least 2 slots in each Transcendental preset | The mod matrix IS the behavioral depth of the engine |

---

## Phase R4: Expression Arcs

### Arc 1 — BELLY Pure (velocity 20 vs 127)
At velocity 20 with BELLY dominant: a warm, soft sub tone with barely-open filter. The brightness envelope barely moves. At velocity 127: filter envelope fires fully, the attack brightens significantly despite the BELLY macro's warmth direction. The BELLY + velocity interaction is subtle — BELLY sets the ceiling for warmth, velocity still sweeps through the available range underneath that ceiling. A 127-velocity BELLY patch is still warm; it just starts bright and resolves to warm quickly.

### Arc 2 — Aftertouch in BITE territory
At `poss_macroBite = 0.5`, moderate bite character. Hold a note and press: aftertouch adds up to +0.4 to `poss_oscBInstability` — the Bite oscillator's pitch jitter increases, the aggressive oscillator becomes more unstable the harder you press. This is the opossum's defensive posture becoming more erratic under continued threat. At maximum aftertouch with moderate BITE, the Bite oscillator is wobbling at ~2.4 Hz pitch instability while the Belly oscillator remains stable — two oscillators with divergent behavior inside a single voice.

### Arc 3 — PLAY DEAD as the Performance Macro
PLAY DEAD at maximum (0.95) should be the preset's destination, not its source. Design the BELLY-warm or BITE-moderate patch first, then assign PLAY DEAD to mod wheel or expression pedal as a performance macro. The collapse from full sound to near-silence as the macro rises is the performance moment. It replicates the behavioral phenomenon: the sound reaches a crisis point and the engine's response is total suspension.

---

## Phase R5: Awakening Presets — 15 Transcendental Designs

| # | Name | Mood | Core Concept |
|---|------|------|-------------|
| 1 | Thanatosis | Foundation | PLAY DEAD 0.92 with sub persisting — the limp body with a heartbeat underneath. Single sub harmonic, barely-open LP filter, enormous reverb tail. |
| 2 | Foraging Warmth | Foundation | BELLY 0.92, BITE 0.0, compound sub (sub + weight sine at -2 oct). The opossum walking. No aggression. Body only. |
| 3 | Fang Root | Foundation | BITE 0.75, BELLY 0.4, Gnash at 0.5, asymmetric waveshaper prominent. The defensive posture in bass register. |
| 4 | Belly Drift | Atmosphere | BELLY 0.88, PLAY DEAD 0.3, OscA sine with drift, LFO1 at 0.06 Hz on filter, long attack. Warm and slightly suspended. |
| 5 | The Grey Space | Atmosphere | PLAY DEAD 0.65, BELLY 0.72 — compound warmth + partial collapse. A sound that exists between alive and pretending not to be. |
| 6 | Alley Warmth | Atmosphere | BELLY 0.8, TRASH 0.2, Fur prominent, Chew moderate. The opossum beside the warm dumpster. Not threatening — just present. |
| 7 | Fur Resonance | Submerged | BELLY high, OscA saw at shape=0.08 (very warm), Fur at 0.22, low LP filter. The undercoat, buried. |
| 8 | Under Bridge | Submerged | PLAY DEAD 0.55, deep sub, Weight Engine pulse at -3 oct click. The stillness below the concrete. |
| 9 | Nocturnal Weight | Submerged | Compound sub, BELLY 0.85, very long attack and release, Space reverb large. The animal moving in total darkness, not visible, only felt. |
| 10 | Scurry-to-Still | Flux | LFO3 S&H → gnash, SCURRY 0.6 into PLAY DEAD 0.4. The panic that becomes paralysis — SCURRY and PLAY DEAD in compound. |
| 11 | Bite Pulse | Flux | BITE 0.7, mod matrix: LFO2 sine at 0.04 Hz → gnash amount. Gnash breathes at breathing rate — teeth appearing and disappearing. |
| 12 | Opossum Current | Entangled | OVERBITE + OXBOW coupling — BELLY warmth entangled with OXBOW's Chiasmus reverb. The warm body inside the entangled reverb field. |
| 13 | Prey Response | Entangled | OVERBITE + ONSET coupling (AudioToFM) — ONSET velocity events FM-modulate OVERBITE's OscB. The opossum reacts to percussion with Bite. |
| 14 | Thanatosis Eternal | Aether | PLAY DEAD 0.95, BELLY 0.5, sub at 0.72, Space reverb decay 8.0s, motion LFO at 0.025 Hz. Absolute stillness with warmth underneath. |
| 15 | Belly Ascent | Aether | BELLY 0.9, long attack (2.5s), Wander curiosity at 0.05 Hz, Chew gentle. The opossum in full trust. Warmth that arrives slowly and stays. |

---

## Phase R6: Scripture Verses Discovered

### Verse OVB-I: The Involuntary Architecture

*On thanatosis as the engine's deepest expression*

> The Virginia opossum does not choose thanatosis. Its nervous system executes the collapse without consultation. When threat exceeds a threshold, the behavior fires — involuntarily, completely, without recourse. The animal wakes up later with no memory of the episode. PLAY DEAD at maximum in OVERBITE works the same way: the macro does not ask whether the sound wants to become still. It simply collapses the harmonic content toward silence. You don't design a PLAY DEAD preset by choosing to make the sound quiet. You design it by choosing when and from where the collapse fires, and what persists underneath it: the sub, the metabolic warmth of BELLY, the faint drift of an opossum that is alive but not visible. The collapse is complete. But the body is still there.

**Application:** When designing PLAY DEAD presets, design the warm BELLY foundation first — the living state before the collapse. Then add PLAY DEAD as the collapsing macro. The most powerful PLAY DEAD sounds are the ones where you can still feel the warm body underneath the stillness.

---

### Verse OVB-II: The Belly is Not Weakness

*On warmth as a complete sonic identity, not the absence of aggression*

> Every time a producer reduces BITE to zero, they suspect they are building something incomplete. The instinct says: a synthesizer should have edge. But OVERBITE at BELLY=0.9, BITE=0.0 is not an incomplete synthesizer. It is the opossum foraging: metabolically warm, fully occupied, entirely present, and not once looking for a fight. The sub oscillator is strong. The fur saturation is the warmth of an animal's body. The filter is open because there is nothing to defend against. The Belly is not weakness. It is the baseline state of a living organism that has no current threat to manage. Design the pure BELLY preset. Trust it. The edge is not absent — it is reserved.

**Application:** Build at least one preset per OVERBITE session in which BITE is 0.05 or below and BELLY is 0.85 or above. These are the presets that demonstrate the engine's full behavioral range. They require no justification.

---

### Verse OVB-III: The Compound Macro

*On the behavioral interactions no single macro can reach*

> Five macros describe five behaviors. But the opossum is not always in a single behavior. It forages with one eye on the shadow of the fence. It bites and then plays dead in the same second. It scurries toward trash. The most alive OVERBITE presets are the ones that hold two or more macros simultaneously — not as compromise but as behavioral compound. BELLY at 0.7 and BITE at 0.2 is not BELLY reduced by the presence of BITE. It is a different state: the animal that is warm and also ready. SCURRY at 0.5 and PLAY DEAD at 0.4 is not two half-behaviors. It is the panic dissolving into surrender. When you design a compound macro preset, name the behavioral state, not the parameter values. The state is the identity of the sound.

**Application:** For every Transcendental preset, describe the behavioral state in one sentence before writing any parameter values. The description is the design brief. The parameters follow the description.

---

## Notes for Vol 2 Booklet

OVERBITE is the fleet's highest-seanced engine. Its 9.2/10 score reflects DSP quality, behavioral architecture, and the unanimity of B008's ratification. The engine is complete. This retreat does not improve it — it extends it into the territory the factory library's 77 presets have not occupied.

The key insight of this retreat: OVERBITE's most expressive territory is not in its loudest, most aggressive presets. Its most expressive territory is in the compound states — BELLY + PLAY DEAD, SCURRY + TRASH — and in the pure warmth that demonstrates the opossum before it has encountered any threat at all. The 9.2 score was earned partly because the engine knows how to be quiet. The Transcendental library should demonstrate that knowing.

---

*Guru Bin, 2026-03-21*
*"The collapse is complete. But the body is still there."*
