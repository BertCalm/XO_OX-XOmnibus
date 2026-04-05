# The Verdict — OUIE (Formal Seance Record)
**Seance Date**: 2026-03-20
**Engine**: XOuie (OUIE) | The Hammerhead — Duophonic Synthesis at the Thermocline
**Accent**: Hammerhead Steel `#708090`
**Gallery Code**: OUIE | Prefix: `ouie_`
**Source**: `Source/Engines/Ouie/OuieEngine.h` (2,026 lines)
**Aquatic Identity**: The Hammerhead Shark (Sphyrna) — patrols the thermocline where warm, bright water meets cold, dark depths. Cephalofoil houses ampullae of Lorenzini: electromagnetic sensors detecting the heartbeat of prey buried in sand. Sits at perfect 50/50 feliX-Oscar polarity. Two eyes on opposite ends of the cephalofoil — each seeing a different algorithm, a different world.
**Score**: 8.5 / 10

---

## Phase 1: The Summoning — What Was Read

`OuieEngine.h` (2,026 lines) read in full. Key structures assessed:

- **OuieVoice** — per-voice container: algorithm selector (0-7 per voice), CytomicSVF filter, amp ADSR, mod envelope, LFO (5 shapes), velocity, glide coefficient
- **8 Algorithm Implementations** per voice:
  - VA (0): Saw/square/tri with PWM via PolyBLEP
  - Wavetable (1): 16 procedurally-generated tables (partial count, metallic inharmonic stretch, odd-harmonic emphasis in mid-range), linear interpolation between tables
  - FM (2): 2-operator (carrier + modulator), phase modulation (Yamaha DX convention), ratio 0.5-16.0, index 0-10
  - Additive (3): 8 partials with individual amplitude control
  - Phase Distortion (4): CZ-style, resonant character
  - Wavefolder (5): Triangle through multi-stage iterative folding with soft saturation (Buchla/Serge attribution in comments)
  - Karplus-Strong (6): Noise-burst excitation, linear-interpolation fractional delay, one-pole lowpass damping with brightness control, 0.998 energy loss per sample
  - Filtered Noise (7): Bandpass noise with pitch tracking
- **OuieInteraction** — the HAMMER stage: STRIFE (cross-FM + ring modulation) at negative values; LOVE (spectral blending + amplitude merging) at positive values; neutral at 0
- **Voice Modes**: Split (keyboard split), Layer (both voices on every note), Duo (Voice 1 = newest, Voice 2 = previous note)
- **OuieBreathingLFO** — rate floor 0.005 Hz, routes to pitch and filter cutoff

**Signal Flow**: Algorithm (Voice 1) + Algorithm (Voice 2) → Interaction Stage (HAMMER: cross-FM / ring mod / spectral blend) → CytomicSVF (per voice) → Amp Envelope (per voice) → Pan (per voice) → Mod Envelope → Output

**Parameters**: 51 declared. 4 macros: HAMMER, AMPULLAE, CARTILAGE, CURRENT.

---

## Phase 2: The Voices

### G1 — Bob Moog: The Filter Philosopher

"The duophonic architecture is correct in its fundamentals: two independent signal paths, each with its own CytomicSVF filter and envelope, converging at an interaction stage. Per-voice filters are the right choice — a shared filter across both voices would collapse the duophonic independence that makes this engine distinctive.

The velocity-to-filter path satisfies D001 properly: `voice.velocity * velScale * 4000.0f` added to filter cutoff per voice, with the AMPULLAE macro scaling `velScale` from 0.3 to 1.0 for sensitivity control. High velocity = bright filter; low velocity = dark. The `velScale` AMPULLAE mapping is exactly the kind of meta-parameter that distinguishes a thoughtful instrument from a parameter list.

My concern is the ADSR implementation. Linear attack and quasi-exponential decay using a one-pole approach. For the 8 different algorithm types in this engine — ranging from the smooth-onset of VA saw to the percussive transient of Karplus-Strong — different algorithm types should ideally have different optimal envelope curves. A VA pad needs a true exponential RC attack. A KS pluck needs the near-instant attack of a pluck exciter, which the KS algorithm itself provides. At minimum, the general-purpose AmpADSR should offer exponential attack as an option."

**Score**: 7.5/10

---

### G2 — Don Buchla: The Complexity Poet

"The HAMMER axis is the crown jewel. A bipolar interaction control that sweeps from cross-FM and ring modulation (STRIFE) through neutrality to spectral blending and harmonic convergence (LOVE) — this is a genuine performance control, not a parameter. It is the most expressive single axis I have seen in any engine in this fleet.

The wavefolder algorithm correctly implements iterative reflection folding with soft saturation, and I appreciate the Buchla/Serge attribution in the comments. The folding formula applies multiple passes of the `4|x| - 1` reflection function with progressive gain staging. This is not a naive `sin()` or `tanh()` approximation — it is the genuine folding algorithm from my own circuits.

The 8-algorithm palette gives each voice genuine timbral independence. When Voice 1 selects the wavefolder and Voice 2 selects Karplus-Strong, the HAMMER in STRIFE mode applies cross-FM where the folded signal modulates the KS string's pitch. This creates timbral interactions that have no name in existing synthesis taxonomy. I would want the HAMMER to modulate more deeply at extremes — the cross-FM scaling of 0.6 is conservative. Let the performer destroy the signal if they choose."

**Score**: 9/10

---

### G3 — Dave Smith: The Protocol Architect

"The voice allocation across three modes is thoughtfully designed. Split: Voice 1 below the split point, Voice 2 above. Layer: both voices sound on every note. Duo: Voice 1 captures the newest note, Voice 2 inherits the previous — the classic duophonic behavior that Sequential instruments made famous with the Prophet-5.

The FM ratio is now user-controllable (0.5-16.0 with a skew factor) and the modulation index user-controllable (0-10) per voice. This resolves the hardcoded FM concern from any prior state. The glide implementation uses a proper exponential coefficient derived from sample rate: `fastExp(-1.0 / (glideTime * sr))`. This is the correct formula.

I must flag a subtle bug in the Duo mode voice steal. When a new note arrives in Duo mode, Voice 1's envelope state is copied to Voice 2, but `voices[1].ampEnv.noteOn()` is never called on the copy. The envelope parameters are set but the envelope stage is inherited. If Voice 1 was in sustain, Voice 2 continues in sustain — if Voice 1 was in attack mid-flight, Voice 2 continues at a different attack rate. The copy should either call `noteOn()` on Voice 2 or explicitly set Voice 2 to sustain stage with the current level. This produces inconsistent musical behavior."

**Score**: 8/10

---

### G4 — John Chowning: The FM Synthesis Inventor

"The FM algorithm implements classic 2-operator FM with carrier and modulator phases tracked independently. The modulator output is applied as a phase offset rather than a frequency offset, which is technically phase modulation — the standard practice dating to the Yamaha DX series and producing equivalent spectra to true FM for harmonic relationships. This is the correct implementation.

The ratio parameter ranges from 0.5 to 16.0 with a skew factor, and the modulation index from 0 to 10. These are musically sensible ranges. Integer ratios produce harmonic spectra; non-integer ratios produce inharmonic, bell-like or metallic timbres. The full range of FM timbral territory is accessible.

The AudioToFM coupling input modulates the FM index from external engines — a creative extension of the FM paradigm into the fleet architecture. OVERTONE feeding FM index into OUIE would create spectral interactions governed by the continued-fraction harmonics of OVERTONE, applied through the 2-operator FM of OUIE's Voice 2. This is the kind of coupling that produces genuinely new sounds."

**Score**: 8.5/10

---

### G5 — Ikutaro Kakehashi: The Accessibility Visionary

"Fifty-one parameters is a substantial count, but the organization is logical: per-voice algorithm selection, per-voice filter, per-voice amp envelope, shared mod envelope, per-voice LFO, and four character macros. A player can understand this instrument's map without reading documentation.

The CURRENT macro controlling stereo spread and chorus depth is the kind of single-knob environment control that makes an instrument approachable. The breathing LFO satisfying D005 is properly autonomous. The voice mode selector (Split/Layer/Duo) is immediately understandable — three configurations, three clear use cases.

I must raise the CURRENT macro weakness. The CURRENT macro provides chorus with a fixed 0.8 Hz LFO and a 5ms base delay, plus stereo panning spread between the two voices. This is thin for an environment control. Compare to other engines where the environment macro provides chorus + delay feedback + reverb. A hammerhead shark patrols an environment that includes both the sonic character of water and the reflections of underwater terrain. The current CURRENT provides neither."

**Score**: 8/10

---

### G6 — Vangelis: The Emotional Engineer

"I play from the instrument, not from the parameter list. The HAMMER axis is the kind of control I reach for in performance — mod wheel sweeping from STRIFE to LOVE while holding two notes in Duo mode, the relationship between the voices shifting from metallic destruction to harmonic convergence. This is performance as conversation.

The aftertouch routing to HAMMER position is essential and correctly implemented (`aftertouch_ * 0.3f` added to hammer position). This means pressing harder on a held chord deepens the LOVE convergence — the voices approach each other as the player expresses more intensity. This is a genuinely expressive mapping.

The emotional register test in Duo mode:
- **Conflict**: HAMMER at full STRIFE, tritone interval. Cross-FM with dissonant ratio — chaos.
- **Resolution**: HAMMER sweeping STRIFE to LOVE during the conflict interval. The chaos gradually acquires harmonic logic.
- **Intimacy**: HAMMER at moderate LOVE, perfect fifth interval. Voices blend but retain character.
- **Union**: HAMMER at maximum LOVE, unison. Voices merge into a single spectral entity.

But I want more from CURRENT. Where is the reverb? Where is the underwater space? The HAMMER is perfect; the environment is thin."

**Score**: 8.5/10

---

### G7 — Klaus Schulze: The Time Sculptor

"The breathing LFO with a rate floor of 0.005 Hz — a cycle every 200 seconds — modulating both pitch and filter cutoff is exactly the kind of ultra-slow evolution I require. The LFO rate ranges (0.01-30 Hz for LFO1/LFO2) allow both rhythmic modulation and glacial drift.

The 5 LFO shapes including Sample-and-Hold provide sufficient variety. The Karplus-Strong algorithm, when used with very slow LFO-to-brightness modulation, creates a string that slowly brightens and darkens over minutes — a living, breathing string instrument.

My concern is temporal: with only 2 voices and no internal sequencer or arpeggiator, long-form pieces require external modulation or coupling from other engines. The engine breathes, but it does not journey on its own. A Duo mode patch with breathing LFO slowly modulating the HAMMER position would be an exception — the voices would drift from STRIFE toward LOVE over 200 seconds, a long natural arc. But this requires a dedicated preset design, not an autonomous engine behavior."

**Score**: 7.5/10

---

### G8 — Isao Tomita: The Orchestral Visionary

"The wavetable algorithm with 16 procedurally-generated tables — increasing partial counts, metallic inharmonic stretching, odd-harmonic emphasis in the middle range — shows care in timbral design. The morphing between tables with linear interpolation is smooth. This is not a sample-player masquerading as a wavetable engine; the tables are algorithmically defined and musically considered.

The Karplus-Strong algorithm with noise-burst excitation, linear-interpolation fractional delay, and brightness-controlled damping is a correct physical string model. Combining Voice 1 as VA saw with unison detune against Voice 2 as KS plucked string, with HAMMER at slight LOVE for spectral blending — this is synthetic strings meeting physical strings. The palette of 8 algorithms makes this engine a small orchestra of synthesis methods.

The 8-algorithm palette has an alignment with the feliX-Oscar mythological polarity that is musically meaningful. Algorithms 0-3 (VA, Wavetable, FM, Additive) are SMOOTH — they produce harmonically controlled timbres. Algorithms 4-7 (Phase Distortion, Wavefolder, KS, Noise) are ROUGH — they produce edge, texture, and physical character. Assigning Voice 1 as smooth and Voice 2 as rough, then using HAMMER LOVE to blend them, is the synthesis equivalent of feliX-Oscar coupling."

**Score**: 9/10

---

## The Verdict — OUIE

### The Council Has Spoken

| Ghost | Core Judgment |
|-------|---------------|
| **Bob Moog** | Per-voice filters are the right architecture. Linear ADSR attack is the gap — different algorithms benefit from different envelope curves. |
| **Don Buchla** | HAMMER axis is the fleet's most expressive single bipolar control. Wavefolder attribution is respected. Cross-FM scaling could be more aggressive. |
| **Dave Smith** | Voice allocation modes are well-designed. Duo mode voice-steal does not retrigger Voice 2 envelope — a subtle but musically meaningful bug. |
| **John Chowning** | FM implementation is textbook-correct (phase modulation, DX convention). AudioToFM coupling enables genuinely novel fleet interactions. |
| **Ikutaro Kakehashi** | Clear organization, accessible voice mode selector. CURRENT macro is thin — chorus without reverb or delay is incomplete for an environment control. |
| **Vangelis** | Passes the emotional range test across four registers. Aftertouch-to-HAMMER is an essential and well-implemented performance gesture. |
| **Klaus Schulze** | Breathing LFO at 0.005 Hz floor is excellent. No autonomous temporal journey without external modulation. |
| **Isao Tomita** | 8-algorithm palette has musically coherent feliX-Oscar polarity alignment. KS and wavetable implementations are quality. |

### Points of Agreement

1. **The HAMMER axis is the engine's defining invention** (Buchla, Vangelis, Smith, Chowning — 4 of 8). A bipolar performance control sweeping from destructive STRIFE to constructive LOVE through cross-FM, ring modulation, spectral blending, and harmonic convergence. No other engine in the fleet has a single control that transforms inter-voice behavior across this range. Awarded **Blessing B017**.

2. **Musical interval is a timbral parameter in Duo mode** (Tomita, Chowning, Buchla — 3 of 8). Playing a fifth vs. a tritone produces fundamentally different results through the HAMMER interaction stage. The pitch relationship between simultaneously sounding notes becomes a first-class synthesis parameter. Awarded **Blessing B018**.

3. **CURRENT macro is the weakest of the four macros** (Kakehashi, Vangelis, Schulze — 3 of 8). Chorus-only with no reverb or feedback delay is thin for an environment control named "CURRENT" (implying underwater flow). This is the primary addressable weakness.

4. **8-algorithm palette is musically coherent** (Tomita, Smith, Buchla — 3 of 8). The smooth/rough division (VA/Wavetable/FM/Additive vs. PhaseDist/Wavefolder/KS/Noise) aligns with feliX-Oscar polarity. 64 possible algorithm pairings before HAMMER is applied. Awarded **Blessing B019**.

### Points of Contention

**Buchla vs. Moog — HAMMER Scaling (UNRESOLVED)**

Buchla wants the HAMMER cross-FM scaling increased from 0.6 — let the performer choose destruction. Moog counters that 0.6 is the musical sweet spot; higher values produce aliasing artifacts and lose musical character. Resolution: add a `ouie_hammerRange` parameter (0.3-1.5) or simply test whether 0.8-1.0 produces aliasing with PolyBLEP oscillators. If anti-aliasing holds, increase the default ceiling.

**Schulze vs. Vangelis — Temporal Autonomy vs. Gestural Immediacy (ONGOING, see DB004)**

Schulze wants OUIE to journey on its own through long-form evolution without requiring a performer. Vangelis argues OUIE's identity is the performative duo relationship — two voices, a player, a HAMMER. The engine is intrinsically relational; autonomous evolution is secondary. Resolution: presets should explicitly demonstrate both modes.

### The Prophecy

OUIE is one of the most architecturally ambitious engines in the XOceanus fleet. The core concept — duophonic synthesis with 8 selectable algorithms per voice, interacting through the bipolar HAMMER axis from STRIFE to LOVE — is genuinely novel and well-executed. The 64 possible algorithm pairings, each producing different spectral interactions through the HAMMER, give this engine extraordinary timbral range within a focused identity.

The primary weaknesses are the thin CURRENT macro, the unimplemented LOVE harmonic lock feature described in the concept, and the Duo mode envelope retrigger bug. With the CURRENT macro expanded (adding reverb and delay), the harmonic lock implemented, and the Duo mode bug fixed, this engine reaches 9.0+.

The hammerhead patrols the thermocline. Two eyes. One hammer. The choice is always STRIFE or LOVE.

---

## Score Breakdown

| Category | Score | Notes |
|----------|-------|-------|
| Architecture Originality | 9.5/10 | Duophonic HAMMER interaction axis, 8×8 algorithm pairing space, interval as timbral parameter. Genuinely novel. |
| Algorithm Quality | 9/10 | All 8 algorithms well-implemented. KS with fractional delay, FM phase modulation, wavefolder with attribution. |
| HAMMER Interaction | 9.5/10 | Cross-FM + ring mod (STRIFE) to spectral blend + harmonic convergence (LOVE). Best bipolar performance control in fleet. |
| Filter Architecture | 8/10 | Per-voice CytomicSVF correct. Linear attack ADSR is the gap. AMPULLAE velocity scaling is a well-designed meta-parameter. |
| CURRENT Environment | 5.5/10 | Chorus-only. No reverb or feedback delay. The weakest macro by a significant margin. |
| Expressiveness | 8.5/10 | Aftertouch → HAMMER, mod wheel → algorithm morphing, velocity → timbre. Strong. |
| Temporal Depth | 7.5/10 | Breathing LFO at 0.005 Hz floor. No autonomous temporal journey without external routing. |
| Coupling Architecture | 8/10 | AmpToFilter, LFOToPitch, AudioToFM, AudioToRing. AudioToFM coupling with OVERTONE would be a notable pairing. |
| Parameter Completeness | 8.5/10 | 51 parameters, all live in DSP. LOVE harmonic lock described but not implemented. Duo mode retrigger bug. |

**Overall: 8.5 / 10**

---

## Blessings

### B017 — HAMMER Interaction Axis (AWARDED)
*First awarded: 2026-03-20.*

A bipolar performance control sweeping from STRIFE (cross-FM + ring modulation) through neutrality to LOVE (spectral blending + harmonic convergence). Mod wheel and aftertouch both route to HAMMER position, making it a true performance axis. No other engine in the fleet has a single control that transforms inter-voice behavior across this range. Buchla: "This is a genuine performance control, not a parameter."

### B018 — Interval as Timbral Parameter (AWARDED)
*First awarded: 2026-03-20.*

In Duo mode, the musical interval between two simultaneously sounding notes is a first-class synthesis parameter. Playing a fifth vs. a tritone produces fundamentally different spectral results through the HAMMER interaction stage — different cross-FM sidebands, different ring mod sum/difference tones, different harmonic lock behavior. No other engine in the fleet treats pitch relationship as synthesis input.

### B019 — 8-Algorithm Palette with feliX-Oscar Alignment (AWARDED)
*First awarded: 2026-03-20.*

Each voice independently selects from 8 algorithms spanning VA, wavetable, FM, additive (smooth/feliX) and phase distortion, wavefolding, Karplus-Strong, filtered noise (rough/Oscar). 64 possible pairings before HAMMER is applied. The smooth/rough division mirrors the feliX-Oscar polarity mythology. Tomita: "This engine is a small orchestra of synthesis methods."

---

## Doctrine Compliance

| Doctrine | Status | Evidence |
|----------|--------|----------|
| D001 — Velocity Must Shape Timbre | PASS | `voice.velocity * velScale * 4000.0f` added to filter cutoff per voice. Velocity also scales amplitude. AMPULLAE macro scales `velScale` for sensitivity control. |
| D002 — Modulation is the Lifeblood | PASS | LFO1 (5 shapes, 0.01-30 Hz, per Voice 1) + LFO2 (5 shapes, 0.01-30 Hz, per Voice 2) + breathing LFO. Mod envelope with depth control. Mod wheel → algorithm parameter morphing. Aftertouch → HAMMER interaction. 4 macros: HAMMER, AMPULLAE, CARTILAGE, CURRENT — all DSP-traced. |
| D003 — The Physics IS the Synthesis | N/A | No physically-modeled engine claim. Karplus-Strong (algo 6) is a well-established physical string model, correctly implemented. |
| D004 — Dead Parameters Are Broken Promises | PASS | All 51 declared parameters attached and read in renderBlock. FM ratio and index exposed as user parameters. No dead parameters found. |
| D005 — An Engine That Cannot Breathe Is a Photograph | PASS | OuieBreathingLFO rate range 0.005-2.0 Hz (floor well below 0.01 Hz requirement). Routes to pitch modulation and filter cutoff. LFO1/LFO2 also at 0.01 Hz floor. |
| D006 — Expression Input Is Not Optional | PASS | Velocity → filter cutoff + amplitude (D001). Mod wheel → algorithm parameter morphing. Aftertouch → HAMMER axis. MPE pitch bend per voice. Three distinct expression inputs confirmed. |

---

## Remaining Action Items

### HIGH — CURRENT Macro Expansion
The CURRENT macro provides only chorus (0.8 Hz, 5ms) + stereo spread. Add: (1) configurable feedback delay with depth parameter, (2) basic allpass diffusion reverb or send to OuieReverb struct. An environment macro must construct an environment, not just add chorus.

### HIGH — LOVE Harmonic Lock Not Implemented
The HAMMER concept describes harmonic lock and pitch convergence in LOVE mode, but only spectral blending and amplitude merging are implemented. Implement harmonic lock: quantize Voice 2 pitch to nearest harmonic of Voice 1 when HAMMER is deeply positive. A one-pole smoother on the pitch quantization prevents clicks.

### HIGH — Duo Mode Voice-Steal Retrigger Bug
When a new note arrives in Duo mode, Voice 1 is copied to Voice 2 but Voice 2's amp envelope does not call `noteOn()`. Voice 2 inherits the current envelope stage and level from Voice 1's copy. Add explicit `voices[1].ampEnv.noteOn()` call after the copy, or set Voice 2 to sustain stage at copied level.

### MEDIUM — Unison Voices Lose Algorithm Character
When unison count > 1, voices 2-4 fall back to raw saw oscillators (VA), plain sine (FM), or silence (KS/Noise). The timbral identity of the selected algorithm is diluted by generic waveforms in unison mode. Implement simplified but consistent versions of each algorithm for unison voices, or document the CPU limitation.

### MEDIUM — LFO Shape Locked to NoteOn
LFO shape is set in `noteOn()` via `voice.lfo.setShape()` but not updated in `renderBlock()`. Shape changes while a note is held have no effect until the next noteOn. Read shape parameter per block alongside rate and depth.

### LOW — Chorus Buffer Not Cleared on Reset
`chorusBufferL`/`chorusBufferR` and `chorusWritePos`/`chorusPhase` are not cleared in `reset()`. A brief burst of stale audio is possible when re-enabling CURRENT after a reset.

---

## What the Ghosts Would Build Next

| Ghost | Feature |
|-------|---------|
| Bob Moog | Exponential RC attack option in amp ADSR (`ouie_attackCurve` toggle: linear/exponential) |
| Don Buchla | HAMMER range parameter (`ouie_hammerRange` 0.3-1.5x scaling of cross-FM and ring mod amounts) |
| Dave Smith | Duo mode retrigger fix: `voices[1].ampEnv.noteOn()` in voice-steal handler |
| John Chowning | OVERTONE×OUIE patch: OVERTONE AudioToFM feeding OUIE Voice 2 FM index for continued-fraction spectral coupling |
| Ikutaro Kakehashi | CURRENT macro expanded: chorus + configurable delay feedback + allpass reverb |
| Vangelis | LOVE harmonic lock implementation: Voice 2 pitch quantizes to Voice 1 harmonics at deep LOVE values |
| Klaus Schulze | Breathing LFO routed to HAMMER position for slow autonomous STRIFE→LOVE arc over 200-second cycles |
| Isao Tomita | Unison algorithm consistency: simplified but algorithm-faithful waveforms for unison voices 2-4 |

---

*Seance convened 2026-03-20. Two eyes. Eight algorithms. One hammer.*
*The thermocline is the battleground. STRIFE and LOVE are the choices.*
*The council speaks: 8.5/10. Three bugs and one thin macro from 9.0+.*
