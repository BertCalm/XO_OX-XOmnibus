# XOxytocin Concept Brief

## ENGINE IDENTITY

**Name**: XOxytocin
**Gallery Code**: OXYTO
**One-Sentence Thesis**: A circuit-modeling synth where Sternberg's three components of love — Intimacy, Passion, Commitment — map to thermal resistance, voltage drive, and capacitance, creating 8 emergent circuit topologies that respond to how you play.
**Elevator Pitch**: XOxytocin models electrical circuits that fall in love. Three forces — warmth (RE-201 tape), drive (MS-20 filter), and stability (Moog ladder) — interact with different time constants, so the LENGTH of your touch determines the EMOTION of the sound. Staccato = infatuation. Legato = consummate love.

---

## CONCEPT → DSP BRIDGE

**Source Concept**: Sternberg's Triangular Theory of Love (1986) — three components (Intimacy, Passion, Commitment) combine into 8 types of love, each with distinct temporal dynamics.

**DSP Translation Table**:

| Concept Element | DSP Implementation | Parameter(s) |
|---|---|---|
| Intimacy (closeness, warmth) | NTC thermal resistance model — RE-201 three-stage warming (motor inertia → head alignment → tube bias) | `oxy_intimacy`, `oxy_warmth_rate` |
| Passion (drive, arousal) | Voltage drive/saturation — MS-20 Sallen-Key asymmetric clipping curve | `oxy_passion`, `oxy_drive_character` |
| Commitment (stability, memory) | Capacitance/energy storage — Moog 4-pole ladder resonance, reactive impedance | `oxy_commitment`, `oxy_resonance_depth` |
| Love type emergence | Cross-modulation matrix (Serge-inspired egalitarian routing) — I modulates P's saturation, P modulates C's charge rate, C modulates I's distribution | `oxy_cross_mod` |
| Temporal dynamics | Three envelopes with different time constants: Passion (fast attack/exp decay), Intimacy (slow sigmoid), Commitment (very slow ramp, high sustain) | `oxy_passion_rate`, `oxy_warmth_rate`, `oxy_commit_rate` |
| Touch-as-emotion (Buchla) | Note duration determines which components reach expression → determines love type | Performance mechanic, no dedicated param |
| Circuit aging | Component drift, degradation over time — tubes lose gain, caps leak, resistors drift with temperature | `oxy_circuit_age` |
| Relationship memory | Running averages of session behavior accumulate relationship state | `oxy_remember` (toggle) |

**Physics/Math Citations**:
- Sternberg, R.J. (1986). "A triangular theory of love." *Psychological Review*, 93(2), 119-135.
- NTC thermistor: R(T) = R₀ · exp(B(1/T - 1/T₀)) — Steinhart-Hart equation for thermal resistance
- MS-20 Sallen-Key: asymmetric soft clipping via tanh(x) + 0.1·tanh(3x) approximation
- Moog cascade: 4 matched RC stages, H(s) = 1/(1 + s/ωc)⁴ with resonance feedback
- Serge cross-modulation: circular modulation topology without hierarchy
- Capacitor charge: V(t) = V₀(1 - e^(-t/RC)) — commitment as stored energy

---

## SONIC TERRITORY

**Synthesis Method**: Circuit modeling (hybrid analog component simulation — thermal + saturation + reactive)
**Timbral Range**: Init = clean, cool, barely connected circuit hum → Fully modulated = rich, warm, screaming, deeply resonant — all three legends audible simultaneously
**Genre Targets**:
1. Lo-fi / Neo-Soul (warm, intimate, breathing textures)
2. Industrial / Experimental (passion-driven distortion, circuit degradation)
3. Ambient / Cinematic (slow relationship arcs, evolving emotional states)

**Emotional Range**: Cold isolation (Non-Love) → Bright infatuation → Warm companionship → Screaming passion → Deep consummate love
**Water Column Placement**: Bathypelagic (1000-4000m) — the depth where the *Praya dubia* colony drifts, self-illuminating in total darkness
**feliX-Oscar Polarity**: 35% feliX / 65% Oscar — more depth and sustain than transient, but passion component adds feliX energy

---

## GAP ANALYSIS RESULT

**Primary Gaps Filled**:
- Synthesis method: Circuit modeling (EMPTY in 71-engine fleet)
- Physical modeling domain: Electrical/circuit (EMPTY)
- Emotional range: Sensual/Intimate (EMPTY), Defiant/Rebellious (EMPTY)
- Timbral: Gritty + Evolving (GAP)
- Genre: Industrial/Noise, Lo-fi, Neo-Soul (ALL EMPTY)
- Coupling: Breath/Pressure sender → reconceived as TriangularCoupling (EMPTY)

**Nearest Neighbors**:
1. XObese (saturation) — but XObese is about fatness, XOxytocin is about relationship between forces
2. XOverbite (wavefolder) — similar harmonic generation, but XOverbite lacks temporal dynamics and emotional framework
3. XOverdub (vintage warmth) — shares the "warm circuit" territory but XOverdub is monophonic vintage recreation, XOxytocin is a fundamentally new topology

**Creative Intersection**: Electrical engineering × Psychology of Love — neither domain alone produces this engine

---

## INTERNAL VOCABULARY

| Term | Meaning | Used In |
|---|---|---|
| Bonding | The act of circuit components forming a connection — warmth flowing between elements | Param names, preset names |
| Infatuation | Bright, harsh, unstable circuit state — high voltage, no thermal coupling | Preset category, love type |
| Companionate | Deep, warm, stable circuit — low drive, high warmth + storage | Preset category, love type |
| Consummate | Full circuit engagement — all three forces at peak expression | Preset category, love type |
| Warmup | The RE-201-style thermal development — cold circuit becoming warm | Parameter group, performance concept |
| Scream | MS-20-style self-oscillation when passion exceeds linear operation | Extreme preset names |
| Fuse | When commitment locks into a saved state (Remember mode checkpoint) — the colony's configuration frozen as a preset | Feature name |
| Circuit Age | Component degradation — tubes losing gain, caps leaking, drift accumulating | Parameter, preset dimension |
| Triangle | The real-time barycentric display showing I/P/C balance | UI element |
| Remember | Session-accumulated relationship state — the circuit's memory of you | Mode name |
| First Date | Reset state — the circuit before any relationship develops | Mode/preset concept |
| Lineage | The legendary circuits (RE-201, MS-20, Moog, Serge, Buchla) embedded in the DSP | Documentation, Field Guide |

---

## COUPLING DESIGN

**Coupling Role**: BILATERAL — sends emotional state, receives modulation that affects the love balance

**Best Coupling Partners**:
1. XOpera (OPERA) — vocal formants + love emotions = the most expressive coupling in the fleet. Opera's Kuramoto sync receiving XOxytocin's passion drive.
2. XOxbow (OXBOW) — golden ratio resonance receiving commitment's stability = infinitely patient beauty
3. XObese (OBESE) — saturation + passion = the loudest love in the fleet. Raw, overwhelming, unforgettable.

**Coupling Type Preferences**: NEW — TriangularCoupling (proposed 16th type). Spectral band encoding: low-freq = Commitment (slow/stable), mid-freq = Intimacy (warm), high-freq transients = Passion (fast/bright). Backward compatible — unaware engines hear complex audio.

**Coupling Sample**: `getSampleForCoupling()` returns a composite signal with three spectral bands encoding the current love state (I/P/C levels). The coupling signal IS the relationship.

---

## PARAMETER SKETCH

**Target Count**: 26 parameters
**Namespace Prefix**: `oxy_`

**Macro Mapping**:
- M1 (CHARACTER): Triangle position — morphs between love types via barycentric coordinates
- M2 (MOVEMENT): Temporal speed — how fast I/P/C evolve within a note
- M3 (COUPLING): Love shared — coupling output intensity and TriangularCoupling balance
- M4 (SPACE): Distance — intimate closeness vs. separation (reverb/delay character)

**Key Parameters**:

| ID | Name | Range | Default | What It Does |
|---|---|---|---|---|
| oxy_intimacy | Intimacy | 0.0-1.0 | 0.3 | Thermal coupling strength — RE-201 warmth depth |
| oxy_passion | Passion | 0.0-1.0 | 0.5 | Voltage drive — MS-20 saturation intensity |
| oxy_commitment | Commitment | 0.0-1.0 | 0.2 | Capacitive storage — Moog ladder resonance depth |
| oxy_warmth_rate | Warmth Rate | 0.01-2.0s | 0.3s | How fast intimacy develops (thermal time constant) |
| oxy_passion_rate | Passion Rate | 0.001-0.1s | 0.005s | How fast passion fires (attack speed) |
| oxy_commit_rate | Commit Rate | 0.1-5.0s | 1.0s | How slowly commitment builds (charge time) |
| oxy_cross_mod | Cross-Modulation | 0.0-1.0 | 0.4 | Depth of Serge-style circular cross-modulation between I/P/C |
| oxy_circuit_age | Circuit Age | 0.0-1.0 | 0.0 | Component degradation — from factory-new to vintage-worn |
| oxy_remember | Remember | 0/1 | 0 | Toggle session memory — circuit accumulates relationship state |
| oxy_feedback | Feedback | 0.0-0.95 | 0.3 | Signal return through the circuit — relationship reinforcement |
| oxy_pitch | Pitch | -24 to +24 st | 0 | Base pitch offset |
| oxy_detune | Detune | 0-100 cents | 0 | Voice detuning |
| oxy_cutoff | Filter Cutoff | 20-20000 Hz | 8000 | Base filter frequency (modulated by love state) |
| oxy_attack | Attack | 0.001-2.0s | 0.01 | Amplitude envelope attack |
| oxy_decay | Decay | 0.01-5.0s | 0.5 | Amplitude envelope decay |
| oxy_sustain | Sustain | 0.0-1.0 | 0.7 | Amplitude envelope sustain |
| oxy_release | Release | 0.01-10.0s | 0.5 | Amplitude envelope release |
| oxy_lfo_rate | LFO Rate | 0.01-20 Hz | 1.0 | Modulation LFO rate (D005: floor at 0.01 Hz) |
| oxy_lfo_depth | LFO Depth | 0.0-1.0 | 0.0 | LFO modulation depth |
| oxy_lfo_shape | LFO Shape | 0-4 | 0 | Sine/Tri/Saw/Square/S&H |
| oxy_mod_wheel | Mod Wheel Dest | 0-3 | 1 | MW → Intimacy / Passion / Commitment / Cross-Mod |
| oxy_aftertouch | Aftertouch Dest | 0-3 | 0 | AT → Intimacy / Passion / Commitment / Cross-Mod |
| oxy_velocity_curve | Velocity Map | 0-2 | 0 | Vel → Passion peak (default) / All three / Intimacy |
| oxy_output | Output Level | -inf to +6dB | 0dB | Master output |
| oxy_pan | Pan | -1.0 to 1.0 | 0.0 | Stereo position |
| oxy_voices | Voices | 1-8 | 4 | Polyphony |

---

## DRAMATIC ARC

**Act 1 (Init)**: A cool, barely-alive circuit. You hear a thin oscillation — potential energy waiting to flow. The triangle display shows a tiny, collapsed shape. No warmth. No drive. No storage. The sound of a circuit that hasn't met anyone yet.

**Act 2 (Exploration)**: Turn up Intimacy — the RE-201 warmth bleeds in, filters open gradually, harmonics enrich. Add Passion — the MS-20 drive saturates, the sound gets bright and urgent. Push Commitment — the Moog depth locks in, resonance stabilizes. The triangle grows and shifts shape. Each combination reveals a different love type with a different sonic character. The user discovers that PLAYING STYLE matters — short notes sound different from long notes.

**Act 3 (Mastery)**: The user realizes they can play the triangle like an instrument. Fast legato runs that bloom from infatuation to romance. Held chords that settle into companionate warmth. Aggressive staccato that spits infatuation. They turn on Remember mode and the circuit LEARNS them — the tenth minute sounds different from the first. They couple XOxytocin to XOpera and hear the vocal engine literally fall in love. They Save a "Fuse" point — a relationship checkpoint that starts every session in love. The synthesizer becomes a mirror.

---

## DOCTRINE PRE-CHECK

| Doctrine | Requirement | Self-Assessment | Notes |
|---|---|---|---|
| D001 | Velocity shapes timbre | PASS | Velocity maps to Passion peak amplitude — harder hits = more intense saturation. Optional mapping to all three or Intimacy only. |
| D002 | Sufficient modulation sources | PASS | 1 LFO (5 shapes, D005 floor), 3 love envelopes (I/P/C with independent rates), ADSR amplitude env, cross-modulation matrix, mod wheel + aftertouch + expression |
| D003 | Physics grounds the DSP | PASS | Steinhart-Hart NTC model, Sallen-Key topology, Moog cascade equations, capacitor charge formula, Sternberg's psychological framework |
| D004 | Every parameter wired to audio | PASS | All 26 parameters directly affect the circuit's audio output or modulation routing |
| D005 | Engine breathes | PASS | LFO floor 0.01 Hz, Commitment rate up to 5.0s, Remember mode creates evolution across minutes |
| D006 | Expression input mapped | PASS | Velocity → passion peak, Aftertouch → configurable (I/P/C), Mod wheel → configurable, Expression pedal → M4 distance |

---

## CONCEPT TEST (7 CHECKS)

| # | Check | Result |
|---|---|---|
| 1 | XO + O-word name | YES — Oxytocin (the bonding hormone) |
| 2 | One-sentence thesis clear | YES — "Circuit modeling where love's three forces create 8 emergent topologies" |
| 3 | Unique sound | YES — no circuit modeling engine in the fleet; temporal love dynamics are unprecedented |
| 4 | Coupling potential (1-10) | 9/10 — TriangularCoupling is a fleet-first, coupling output carries emotional metadata |
| 5 | All 6 doctrines PASS | YES — all 6 PASS |
| 6 | Vocabulary 6+ terms | YES — 12 terms defined |
| 7 | Dramatic arc | YES — cold circuit → discovery of love types → mirror/memory mastery |

**GATE: ALL 7 PASS. Coupling 9/10. Proceeding to Phase 0.5.**

---

## ACCENT COLOR

**Hex**: #9B5DE5
**Name**: Synapse Violet
**Reasoning**: The intersection of bioluminescent deep-sea blue and neural chemistry violet — oxytocin is a neuropeptide, and its receptor pathways fire in the violet range. The siphonophore colony's bioluminescent pulses are blue-violet in the bathypelagic zone: not the turquoise of the surface but the deeper violet of total-pressure dark. Circuit Rose (#C9717E, the original color) was retired when the anglerfish mythology was replaced by the deep-sea siphonophore — the warm-copper read of Circuit Rose belonged to the surface; Synapse Violet belongs to the deep.

**Fleet conflict check (2026-03-22)**: Distinct from ORACLE (Prophecy Indigo #4B0082, H=270° lower value), OBLIQUE (Prism Violet #BF40FF, H=280° higher saturation/brightness), and ORBWEAVE (Kelp Knot Purple #8E4585, H=293° more red-purple). No collision. Synapse Violet reads as electric-deep rather than mystic-dark or prism-bright.

**Previous color (retired)**: Circuit Rose `#C9717E` — retired 2026-03-22 with anglerfish mythology replacement.

---

## LEGEND LINEAGES

| Legend | Component | What Lives in the DSP |
|---|---|---|
| Roland Space Echo RE-201 (1974) | Intimacy warmth model | Three-stage thermal warming: motor inertia → head alignment → tube bias shift |
| Korg MS-20 (1978) | Passion saturation curve | Sallen-Key asymmetric clipping, self-oscillation at extremes |
| Moog Ladder (1965) | Commitment resonance | 4-pole cascade depth, unwavering resonant lock |
| Serge Modular | Cross-modulation architecture | Egalitarian circular routing — every component is both source and processor |
| Buchla Touch | Performance mechanic | Note duration determines emotional state — how you touch determines what you hear |

---

## AQUATIC MYTHOLOGY

**Creature**: Deep-sea Siphonophore (*Praya dubia*, Blainville's physonect)
**Zone**: Bathypelagic (1000–4000m)
**feliX-Oscar**: 35% feliX / 65% Oscar — depth and sustain dominant, but the Passion bioluminescent pulse is unmistakably feliX
**Mythology**: *Praya dubia* is not a single animal but a colony of fused zooids — nectophores (Passion/propulsion), gastrozooids (Intimacy/feeding and warmth), gonozooids (Commitment/reproduction and future). Each zooid is specialized and individually helpless; together they produce a colony 40+ meters long — among the longest animals on Earth — self-illuminating in total darkness via blue-violet bioluminescent pulses. Colony behavior emerges from individual specialization with no central nervous system. Sternberg's three components of love map directly to the three zooid types: this is not metaphor, it is body plan.

**Mythological relationships**:
- OVERBITE (anglerfish, same zone): convergent depth, divergent role — lone predator vs. colonial superorganism. The original XOxytocin anglerfish mythology was reassigned to OVERBITE (its rightful owner) per Architect C1 collision ruling.
- OXBOW (same depth range): patience at depth — golden ratio geometry and colonial architecture are both ancient, distributed, and self-similar
- ORCA (apex predator): pod intelligence meets colonial intelligence — two models of coordinated depth
- ORPHICA (surface siphonophore, same phylum): same biology, different zone — the surface colony in the light vs. the deep colony generating its own

**Full mythology**: `Docs/concepts/mythology-xoxytocin.md`

**Previous creature (retired)**: Deep-sea Anglerfish (Melanocetus johnsonii) — retired 2026-03-22. Anglerfish mythology (bioluminescent lure, male-female fusion/permanent bonding) belongs to OVERBITE (OVERBITE: "The anglerfish. Deep water predator with a bioluminescent lure — plush, glowing, inviting — until the bite.").
