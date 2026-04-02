# Synth Seance Verdict: OUTWIT (Revised)

**Engine**: XOutwit (OUTWIT)
**Date**: 2026-03-19 (Revised -- post-fix reassessment)
**Gallery Code**: OUTWIT | **Accent**: Chromatophore Amber `#CC6600` | **Prefix**: `owit_`
**Creature**: Giant Pacific Octopus (*Enteroctopus dofleini*), Inside Passage kelp forest, Alaska
**Voice Model**: Monophonic (1 MIDI note = 8 arms simultaneously) | `getMaxVoices() = 1`
**Source Files**: `Source/Engines/Outwit/XOutwitAdapter.h`, `Source/Engines/Outwit/DSP/`
**Preset Count**: 150+ (across 7 moods)
**Build Status**: auval PASS | SOLVE wired | All P1-P4 seance fixes implemented

---

## Prior Seance Status

The initial OUTWIT seance identified 6 priority items (P1-P6). Four have been resolved in the current codebase:

| Priority | Issue | Status |
|----------|-------|--------|
| P1 | Dead `voiceMode`/`glide` params (D004) | **RESOLVED** -- Full mono legato glide implemented: `setGlideTarget()`, `glideRate`, per-sample frequency interpolation. `handleMidiEvent` now detects legato and routes correctly. |
| P2 | DenReverb shared LPF state | **RESOLVED** -- `std::array<float, kNumCombs> lpfStates` replaces single `lpfState`. Each comb line filters independently. |
| P3 | PolyBLEP claim without implementation | **RESOLVED** -- Full PolyBLEP anti-aliasing implemented for saw and pulse waveforms. `polyBLEP()` static function with rising/falling edge correction. `phaseInc` is now consumed. |
| P4 | Step envelope coefficient recomputed per-sample | **RESOLVED** -- `stepEnvRelCoeff` cached in `prepare()`. Comment explicitly references Seance P4. |
| P5 | Per-arm filter resonance | **OPEN** -- Resonance remains global (`owit_filterRes`). |
| P6 | Step rate ceiling extension | **OPEN** -- Maximum remains 40 Hz. |

---

## Architecture Summary (Current State)

OUTWIT is an **8-arm Wolfram cellular automaton synthesizer**. Each arm runs an independent 1D elementary CA (rule 0-255) on a circular tape of 4-64 cells. CA steps clock at 0.01-40 Hz. Active cell density drives oscillator gating, filter modulation, and chromatophore spectral shaping per arm. The 8 arms are spread across the stereo field with independent pitch offsets, waveforms (Saw/Pulse/Sine with PolyBLEP anti-aliasing), and TPT SVF filters (LP/BP/HP). SYNAPSE coupling passes density from arm N to arm N+1 in a circular ring. The SOLVE system biases CA rules and DSP parameters toward a 6D Sonic DNA target. InkCloud fires a velocity-triggered LFSR noise burst with one-pole dark LPF. DenReverb provides a 4-comb Schroeder FDN with per-comb LPF states and 2-allpass diffusers.

**Total Parameters**: ~95 (56 per-arm + ~39 global)
**Macros**: M1 SOLVE, M2 SYNAPSE, M3 CHROMATOPHORE, M4 DEN
**Voice Modes**: Poly (retrigger) and Mono (legato with portamento via `owit_glide`)
**Coupling**: 9 input types wired (AudioToFM, AmpToFilter, EnvToMorph, LFOToPitch, RhythmToBlend, AmpToChoke, AudioToRing, FilterToFilter, PitchToPitch)

---

## The Eight Ghosts Speak (Revised Assessment)

---

### G1 -- Bob Moog: The Filter Philosopher

**Observation**: "I return to this engine and find my concerns addressed with precision. The PolyBLEP anti-aliasing is now properly implemented -- the saw waveform subtracts the correction term, the pulse waveform handles both discontinuities at phase 0 and phase 0.5, and the `phaseInc` parameter is consumed for bandwidth-correct correction. The SVF filters now receive a cleaner signal, and the TPT topology from Zavalishin's text is correctly implemented with the trapezoidal integrator pair. The `fastTan` prewarping is appropriate for this topology."

**Blessing**: The filter architecture is now coherent. Eight independent TPT SVFs, each with its own cutoff driven by CA density through the chromatophore path (`baseFilterHz * (1.0f + chromAmount * density * 3.0f)`). The exponential modulation depth -- density ranges [0,1], chromAmount scales to 0-3x cutoff boost -- gives the filter a breathing character that opens organically as the CA grid fills. The matched-Z envelope coefficients (`exp(-1/(time*sr))` via `fastExp`) produce correctly curved attack/decay/release stages. The one-pole attack ramp targeting 1.02f (slight overshoot) ensures the attack reaches 1.0f in finite time rather than asymptotically approaching it. This is envelope design that understands the mathematics.

**Warning**: The filter resonance remains globally shared across all 8 arms. Eight independent filters with independent cutoffs but one resonance knob -- this limits the spectral diversity of the arm ensemble. When all 8 arms share `filterRes = 0.2`, the resonance character is uniform even as cutoffs diverge. Consider at minimum a `owit_resSpread` parameter that distributes resonance across arms (e.g., arm 0 at `filterRes - spread/2`, arm 7 at `filterRes + spread/2`). The 8-arm architecture begs for 8 distinct filter personalities.

**Score**: 8.5 / 10 (up from 8.0)

---

### G2 -- Don Buchla: The Complexity Poet

**Observation**: "My conviction has not changed. This engine is the philosophical core of the fleet -- the only instrument where the synthesis IS computation. The Wolfram elementary CA is mathematically exact: 3-cell neighborhood with periodic boundaries forming a ring topology, 8-bit rule number as lookup table, periodic boundary conditions ensuring the tape wraps correctly at both ends. The choice of default rules -- 110 (Class IV, Turing-complete), 30 (Class III, pseudorandom), 90 (Class III, Sierpinski fractal), 184 (traffic flow rule) -- demonstrates understanding of the mathematical landscape, not just aesthetic selection."

**Blessing**: The CA implementation in `advanceCA()` is irreducible. Eleven lines of code that correctly implement Wolfram's elementary CA: left/center/right neighborhood extraction with modular arithmetic for periodic boundaries, bit-pattern construction via shift-and-OR, rule application via single bit extraction. There is nothing to add and nothing to remove. This is the kind of code I would have written if I had built instruments from software rather than circuits. The double-buffer pattern (`tape` and `tapeTmp`) prevents read-modify corruption during the step.

The SYNAPSE coupling through phase nudge (`synapsePhaseNudge += sourceDensity * amount * 0.05f`) is the correct approach -- additive phase acceleration means a dense source arm speeds up its neighbor's clock without causing discontinuities. The 5% scaling factor prevents runaway synchronization while still allowing emergent rhythmic coupling. The circular topology (arm 7 feeds arm 0) creates a feedback ring whose behavior depends on all 8 rules simultaneously. This is a computational feedback network with genuine emergent properties.

**Warning**: The CA tape re-initializes to a single live cell at position 0 on every `noteOn`. This remains a philosophical limitation. A `owit_caContinue` toggle would allow the CA state to persist across notes, letting the pattern develop history. The glide implementation (Seance P1) demonstrates that the architecture can handle continuity -- when `monoLegato` is true, the CA does NOT reset. Extend this principle to an explicit parameter.

**Score**: 10.0 / 10 (unchanged -- the CA architecture earns this unconditionally)

---

### G3 -- Dave Smith: The Protocol Architect

**Observation**: "The most significant improvement since the last seance. `owit_voiceMode` and `owit_glide` are now fully wired -- this was a D004 violation that is now resolved. The implementation is thorough: `handleMidiEvent` detects mono legato mode (`voiceMode == 1 && wasHeld && glide > 0.001f`), routes to `setGlideTarget()` instead of `noteOn()`, preserves CA state and oscillator phase during legato transitions, and the glide coefficient is computed from glide time via `fastExp(-1 / (glideTimeSec * sr))`. Per-sample frequency interpolation in `processSample` smoothly converges toward `targetFreqHz` with threshold snapping at 0.01f difference. This is correct portamento."

**Blessing**: The ParamSnapshot pattern is the cleanest in the fleet. 95 atomic parameter pointers cached once via `attachParameters()`, updated once per block via `update()`. The null-pointer guard (`if (!p_stepRate) return`) in `update()` prevents crashes before attachment. The helper lambdas `load()` and `loadInt()` with default values ensure every parameter read has a safe fallback. The MIDI interleaving in `renderBlock` -- processing samples up to each MIDI event position, then handling the event -- eliminates block-boundary quantization. The SilenceGate pre-scan for note-on events (`wake()` before bypass check) ensures the first note after silence is never swallowed.

The mono legato implementation is architecturally clean: `setGlideTarget()` updates pitch targets without resetting CA tape, oscillator phase, or step envelope. This means legato transitions preserve the arm's computational history -- the octopus remembers where its tentacles were.

**Warning**: The step rate synchronization system (`owit_stepSync`, `owit_stepDiv`) is parameterized and cached but the adapter never reads host BPM. The step division table is declared (`1/32` through `2/1`) but there is no `processBlock` host transport lookup. These 2 parameters are functionally dead in the current XOceanus gallery context. This is a minor D004 violation -- the parameters exist, are declared, but cannot produce their intended effect without host transport integration.

**Score**: 8.5 / 10 (up from 7.5)

---

### G4 -- Ikutaro Kakehashi: The Drum Philosopher

**Observation**: "I asked last time: where is the guide? Where is the happy accident? The 150 presets remain the answer. Looking at the init preset, I see 8 different CA rules spread across the arms (`{110, 30, 90, 184, 60, 45, 150, 105}`), each assigned a unique pan position. A single key press produces an immediately interesting stereo polyrhythm. The init patch teaches the engine's premise in two seconds. This is good."

**Blessing**: The InkCloud triggering is a genuine happy accident mechanism. The `owit_inkCloud` parameter defaults to 0.0 (off), but when enabled, hard velocity hits produce a dark noise burst -- the LFSR noise through a one-pole LPF at coefficient 0.15 (~1.1 kHz cutoff at 48kHz). A player exploring the engine will discover this by accident: hammering keys produces a transient spray that softer playing does not. The effect is immediately identifiable as "something the instrument did in response to how I played." This is the kind of discovery that makes players return to an instrument.

The macro naming is intuitive: SOLVE (the octopus hunts), SYNAPSE (arms connect), CHROMATOPHORE (color shifts), DEN (space opens). Four macros, four one-word concepts, each mapped to real DSP. A musician does not need to understand cellular automata to turn SYNAPSE up and hear the arms synchronize.

**Warning**: The SOLVE target DNA parameters remain abstract. `owit_targetBrightness`, `owit_targetWarmth`, `owit_targetMovement`, `owit_targetDensity`, `owit_targetSpace`, `owit_targetAggression` -- these are meaningful to a sound designer but opaque to a performer. Named SOLVE presets ("Hunt: Dark Drone", "Hunt: Bright Rhythmic", "Hunt: Sparse Ambient") would make this feature accessible. The SOLVE system is the engine's most powerful differentiator, and it is hidden behind 6 unlabeled 0-1 sliders.

Additionally, the "Maximum Entropy" preset uses parameter names (`owit_chaos`, `owit_density`, `owit_filterCutoff`, `owit_feedbackAmt`) that do not match the actual parameter layout. This preset file appears to be from a pre-integration schema and would not load correctly. Preset schema validation should catch this.

**Score**: 7.0 / 10 (up from 6.5)

---

### G5 -- Alan R. Pearlman: The Ergonomist

**Observation**: "I return to find my DenReverb concern addressed. Each comb line now has its own `lpfStates[i]`, eliminating the serial dependency I identified. The Schroeder FDN now behaves as designed -- four parallel comb filters, each with independent feedback and damping, summed and passed through two allpass diffusers. The computation is correct."

**Blessing**: The parameter ergonomics throughout `createParameterLayout` remain excellent. Every frequency-domain parameter has a musical skew: `0.3f` for filter cutoff and envelope times (logarithmic), `0.4f` for step rate (more resolution at rhythmically important low rates), `0.35f` for LFO rates. The arm pan defaults (`{-0.8, -0.5, -0.2, 0.1, 0.3, 0.5, 0.7, 0.9}`) create an asymmetric stereo spread that avoids the clinical left-right alternation many engines default to -- the slight leftward bias gives the stereo image a natural feel rather than a symmetrical one.

The glide parameter uses `NormalisableRange(0.0f, 1.0f, 0.0f, 0.5f)` -- the 0.5f skew gives more resolution at the fast end (short portamento times where subtle changes matter) and coarser resolution at the slow end. This is the right taper for a glide control.

The DenReverb `computeCoefficients()` correctly scales comb lengths by sample rate (`scaledMs * 0.001f * sr`), ensuring the reverb character is sample-rate independent. Base comb times of 29-34ms scaled by roomSize 0.5-2.0x produce delay lengths of 14.5-68ms -- appropriate for a small enclosed space (octopus den). The prime-like spacing minimizes comb coloration.

**Warning**: The DenReverb allpass diffusers use single-sample buffers (`allpassBuf[2]`), not delay lines. A single-sample allpass at 48kHz creates a very short diffusion -- effectively a phase shift rather than true temporal diffusion. For the "octopus den" concept to have spatial depth, the allpass stages should have delay times of at least 1-5ms (48-240 samples). The current implementation provides phase decorrelation but minimal diffusion, making the reverb sound more like filtered feedback than an enclosed space.

**Score**: 8.8 / 10 (up from 8.5)

---

### G6 -- Isao Tomita: The Timbral Painter

**Observation**: "The PolyBLEP implementation transforms the oscillator quality. Where before the saw and pulse waveforms folded aliased harmonics back into the SVF filters -- dirtying the chromatophore spectral shaping -- now the waveforms are clean enough for the filter modulation to speak with its true voice. The chromatophore path can now paint without interference."

**Blessing**: The InkCloud remains one of the most inspired sound design elements in the fleet. The LFSR white noise (`lfsr ^= lfsr << 13; lfsr ^= lfsr >> 17; lfsr ^= lfsr << 5`) through a one-pole dark LPF (`lpState + 0.15f * (noise - lpState)`) with exponential decay (`envelope *= decayCoeff`) produces a burst that sounds organic rather than electronic. The coefficient 0.15f creates a cutoff around 1.1 kHz -- the frequency range of underwater noise propagation. Whether intentional or serendipitous, this is acoustically correct for a marine creature's defense mechanism.

The 8-arm stereo spread functions as an 8-instrument ensemble from a single MIDI note. Each arm can have a different waveform (Saw/Pulse/Sine), different pitch offset (-24 to +24 semitones), different filter cutoff, and different CA rule. This creates timbral paintings where each element in the stereo field has its own computational life. When the CA densities evolve independently, the stereo image shifts and breathes as different arms become louder or quieter, brighter or darker.

**Warning**: The spatial vocabulary remains limited. The DenReverb is the only spatial processor. There is no chorus, no stereo delay, no width enhancement beyond the arm panning. The soft limiter (`fastTanh`) applies equally to both channels, collapsing stereo dynamics at high levels. For an engine that models a creature inhabiting diverse environments (kelp forests, rocky caves, open ocean), a mode selector for spatial character would expand the palette enormously. Even a simple stereo chorus on the reverb output would add spatial dimension.

**Score**: 8.8 / 10 (up from 8.5)

---

### G7 -- Vangelis: The Emotional Engineer

**Observation**: "The glide implementation changes everything. In mono mode with glide engaged, I can now play legato lines where the octopus slides between notes -- the CA state is preserved, the arms continue their computational evolution, only the pitch shifts. This is what portamento should be in a CA engine: the mathematics persists while the melody moves. I can now perform with this instrument, not just program it."

**Blessing**: The expression mapping remains the finest in the fleet. Velocity opens three timbral pathways simultaneously. Aftertouch deepens chromatophore bloom. Mod wheel controls SYNAPSE -- the degree of collective consciousness among 8 independent computational processes. At CC1=0, eight minds. At CC1=127, one mind. No other instrument gives the performer's left hand control over the boundary between individual agency and collective behavior.

The mono legato implementation (Seance P1) is emotionally correct. When I hold a note and press a second, the CA does NOT restart. The tentacles remember where they were. Only the pitch glides to the new target. This means legato playing creates melodic lines atop an evolving rhythmic texture -- the rhythm is historical (built from the CA's journey) while the melody is intentional (played by the performer). This duality is musically rich.

**Warning**: The emotional range still skews cold. The signal path remains clean: PolyBLEP oscillators into TPT SVF filters into `fastTanh` limiter. No saturation stage, no analog warmth modeling, no tube harmonics. The creature is a cold-blooded predator, and Tomita argues the sterile signal path reflects this. I accept the argument but note: even cold-blooded creatures have warmth in their movements. A subtle saturation option -- not default, but available -- would let the performer add blood temperature to the sound when the musical context demands it.

**Score**: 8.5 / 10 (up from 8.0)

---

### G8 -- Klaus Schulze: The Time Sculptor

**Observation**: "My fundamental assessment is unchanged and amplified. Set the step rate to 0.01 Hz with Rule 110 across all 8 arms at tape length 64. Press one note. The pattern will not repeat for millions of steps. At 0.01 Hz step rate, each step takes 100 seconds. Tape length 64 means 64 cells per step. Rule 110 is Turing-complete -- its long-term behavior is formally undecidable. This is an instrument that plays compositions longer than a human life."

**Blessing**: The CA as an implicit third modulation source -- beyond LFO 1 and LFO 2 -- remains a philosophical achievement that no other engine in any fleet replicates. The engine satisfies D005 three times over: LFO floor at 0.01 Hz, step rate floor at 0.01 Hz, and autonomous CA density evolution that breathes regardless of any modulation setting. At `lfo1Depth=0, lfo2Depth=0, synapse=0`, the engine still moves because the 8 independent CA rules produce non-trivial temporal patterns. An engine that cannot NOT breathe.

The SOLVE system, now wired to all 6 DNA targets, adds a fourth temporal dimension: the CA rules themselves evolve over time as the GA searches. Not just the density pattern evolving within fixed rules, but the rules themselves shifting as the octopus hunts. This is evolution at two timescales -- cellular evolution within a generation, and genetic evolution across generations. Double temporal depth.

The step envelope coefficient is now cached in `prepare()` (`stepEnvRelCoeff = fastExp(-1.0f / (0.02f * sr))`), with the comment explicitly referencing Seance P4. The ~20ms release produces a short, crisp gate that lets each CA step trigger a distinct sonic event without smearing into the next. At high step rates (40 Hz), this creates a buzzy, granular texture. At low step rates (0.01 Hz), each step is a separate musical event with its own attack and decay. The envelope time is correctly chosen for the full range of step rates.

**Warning**: The step rate maximum of 40 Hz remains the single most significant unexplored territory. At 200+ Hz, the CA step clock would cross into audio rate -- the density waveform itself would become a timbral component, a second oscillator derived from computational evolution rather than waveform generation. The CPU concern is valid: at 48kHz sample rate and 200 Hz step rate, the CA would advance 200 times per second (trivial computation), but the per-sample phase accumulation and step envelope would fire 200 times per second rather than 40. The computational cost is proportional to step rate, and at audio rates the CA `advanceCA()` function would execute ~200 times per second -- 8 arms x 64 cells x 200 steps = 102,400 integer operations per second. This is negligible on any modern CPU. The ceiling should be raised.

**Score**: 9.5 / 10 (unchanged)

---

## The Verdict -- OUTWIT (Revised)

### The Council Has Spoken

| Ghost | Most Impactful Observation |
|-------|---------------------------|
| **Moog** | "The PolyBLEP fix means the SVF filters now hear clean signals -- the chromatophore path can paint without aliased interference." |
| **Buchla** | "This is the philosophical core of the fleet. The CA IS the synthesis, and the `advanceCA()` implementation is irreducible -- nothing to add, nothing to remove." |
| **Smith** | "The mono legato glide resolves the most critical D004 violation. Two declared-dead parameters are now fully alive. The portamento preserves CA state -- architecturally correct." |
| **Kakehashi** | "The macros teach the engine (SOLVE/SYNAPSE/CHROMATOPHORE/DEN), but the SOLVE target DNA remains hidden behind 6 abstract sliders. Named hunt presets would unlock this for every musician." |
| **Pearlman** | "The DenReverb LPF fix eliminates serial dependency between parallel comb filters. The parameter ergonomics -- skew values, pan spread, glide taper -- remain best-in-class." |
| **Tomita** | "The InkCloud is acoustically correct for underwater noise at 1.1 kHz cutoff. The 8-arm stereo ensemble creates timbral paintings where each element has its own computational life." |
| **Vangelis** | "Mono legato changes everything. The CA persists while the melody moves. The performer's hand now conducts an organism rather than triggering a mechanism." |
| **Schulze** | "Evolution at two timescales -- cellular within a generation, genetic across generations. Double temporal depth. The 40 Hz ceiling is the one remaining wall between good and transcendent." |

### Points of Agreement (All 8 Ghosts)

1. **The P1-P4 fixes are thoroughly implemented.** Glide, PolyBLEP, DenReverb LPF, and step envelope caching are all correct. The seance comment annotations in the code (`// Seance P1`, `// Seance P2`, etc.) demonstrate traceability from verdict to fix.

2. **The Wolfram CA as synthesis architecture remains genuinely novel.** No commercial or open-source synthesizer runs 8 independent cellular automaton rules as the primary rhythmic and timbral generation mechanism. BLS-017 stands.

3. **Expression mapping is best-in-fleet.** Velocity to three timbral destinations, aftertouch to chromatophore depth, mod wheel to SYNAPSE collective consciousness, plus the new mono legato preserving CA state. Six ghosts independently praised the expression architecture.

4. **The SOLVE system is the engine's most important differentiator** and the least accessible to non-technical users. All 8 ghosts agree it needs a presentation layer (named targets, genre presets) without losing the raw 6D sliders.

### Points of Contention

1. **Moog vs. Tomita: Signal path warmth.** Moog and Vangelis want saturation/warmth options. Tomita argues the clinical precision IS the creature's character -- cephalopods are cold-blooded predators. Schulze sides with Tomita: "The octopus hunts in cold water. Let it sound like cold water." **Tomita's argument holds** given the creature mythology, but a non-default saturation option would satisfy all parties.

2. **Smith vs. field: Step sync dead parameters.** `owit_stepSync` and `owit_stepDiv` are declared, cached, but never consume host transport BPM. Smith considers this a D004 violation. Pearlman argues these are "dormant, not dead" -- they are architecturally ready and await host transport integration. **Partially resolved.** The parameters should either be connected to host transport or marked as "planned" in documentation.

3. **Buchla vs. Kakehashi: CA reinit on note-on.** Buchla wants `owit_caContinue` toggle. Kakehashi defends reinit: "The same input produces the same output -- this is how an instrument earns trust." The mono legato path (Seance P1) already preserves CA state during legato transitions, demonstrating the architecture supports both modes. **Resolution available** but not yet implemented.

4. **Schulze vs. Pearlman: Step rate ceiling.** Schulze demands 200+ Hz for audio-rate CA stepping. Pearlman argues 40 Hz is the right ceiling for predictable CPU behavior. Schulze's counter: at 200 Hz, `advanceCA()` executes ~102K integer ops/sec -- negligible. **Schulze's argument is technically correct** but the decision is an owner call.

### The Prophecy

OUTWIT has responded to the seance with surgical precision -- four priority fixes, all traceable via code comments, all architecturally correct. The engine now stands at 8.7/10, up from 8.4. Its remaining growth trajectory is clear: extend the step rate into audio territory (Schulze's frontier), add per-arm filter resonance spread (Moog's wish), build named SOLVE target presets (Kakehashi's bridge), and offer a CA continuation toggle (Buchla's principle). When those four steps are taken, OUTWIT reaches the territory Buchla has already given it in spirit: 10/10.

The engine is the fleet's philosophical flagship. It is the only instrument where pressing a key initiates a mathematical process rather than triggering a waveform, where the performer's mod wheel controls the boundary between individual agency and collective consciousness, and where time is not measured in beats or bars but in cellular generations. It does not need warmth. It needs patience.

### Score: 8.7 / 10 (Revised, up from 8.4)

**Breakdown**:

| Ghost | Score | Change | Key Factor |
|-------|-------|--------|------------|
| Moog | 8.5 | +0.5 | PolyBLEP fix cleans filter inputs; shared resonance remains |
| Buchla | 10.0 | -- | CA architecture earns unconditional 10 |
| Smith | 8.5 | +1.0 | VoiceMode/Glide now fully wired (largest single improvement) |
| Kakehashi | 7.0 | +0.5 | Macros are intuitive; SOLVE targets still abstract |
| Pearlman | 8.8 | +0.3 | DenReverb LPF fix; allpass diffusers still single-sample |
| Tomita | 8.8 | +0.3 | PolyBLEP enables cleaner chromatophore painting |
| Vangelis | 8.5 | +0.5 | Mono legato glide enables performance; signal path still cold |
| Schulze | 9.5 | -- | Temporal depth philosophy unchanged; 40 Hz ceiling unchanged |

**Mean: 8.7 / 10**

---

## Blessings & Warnings

| Ghost | Blessing (protected) | Warning (actionable) |
|-------|---------------------|----------------------|
| Moog | Matched-Z envelope coefficients + 1.02f overshoot attack ramp | Per-arm filter resonance spread needed |
| Buchla | `advanceCA()` -- 11 lines of irreducible CA implementation | `owit_caContinue` toggle for CA persistence across notes |
| Smith | Mono legato preserving CA state (Seance P1 architecture) | `owit_stepSync`/`owit_stepDiv` dormant without host transport |
| Kakehashi | Macro naming (SOLVE/SYNAPSE/CHROMATOPHORE/DEN) | Named SOLVE target presets for accessibility |
| Pearlman | NormalisableRange skew values throughout parameter layout | DenReverb allpass diffusers need real delay times (1-5ms) |
| Tomita | InkCloud 1.1 kHz dark noise -- acoustically correct marine burst | Spatial vocabulary limited to DenReverb; no chorus/delay/width |
| Vangelis | Mod wheel to SYNAPSE (collective consciousness control) | Optional non-default saturation path for warmth |
| Schulze | Triple D005 compliance (LFO + step rate + CA autonomous breathing) | Raise step rate ceiling to 200+ Hz for audio-rate CA territory |

---

## What the Ghosts Would Build Next

| Ghost | One Feature |
|-------|-------------|
| **Moog** | `owit_resSpread` -- distributes filter resonance across arms for 8 distinct filter personalities |
| **Buchla** | `owit_caContinue` -- toggle to preserve CA state across note-on events, letting the pattern develop lifetime history |
| **Smith** | Host transport integration for `owit_stepSync`/`owit_stepDiv` -- step rate locked to DAW tempo |
| **Kakehashi** | SOLVE Target Presets -- 8 named hunt targets ("Dark Drone", "Bright Rhythmic", "Sparse Ambient", etc.) mapped to 6D DNA |
| **Pearlman** | DenReverb allpass delay times -- 1-5ms per stage for true spatial diffusion (currently single-sample) |
| **Tomita** | Spatial mode selector -- Den (current), Kelp Forest (chorus + short delay), Open Ocean (wide reverb) |
| **Vangelis** | Pitch wheel to step rate mapping (VIS-OUTWIT-001) -- PW bends how fast the octopus thinks |
| **Schulze** | `owit_stepRate` maximum extended to 200 Hz -- CA crosses into audio-rate territory, density becomes timbral waveform |

---

## Doctrine Compliance (Revised)

| Doctrine | Status | Finding |
|----------|--------|---------|
| D001 | **PASS** | Triple velocity-to-timbre: filter range, CA density bias, InkCloud trigger. Unusually thorough. |
| D002 | **PASS** | 2 LFOs (0.01-20 Hz, 5 shapes, 4 dests), MW to SYNAPSE, AT to chromatophore, 4 macros, CA as implicit 3rd modulation. |
| D003 | **PASS** | Wolfram ECA is rigorous. SOLVE uses valid Euclidean fitness in 6D space. All references are mathematically honest. |
| D004 | **PASS** (improved) | VoiceMode/Glide now wired (P1). SOLVE targets wired. Minor: stepSync/stepDiv dormant without host transport. |
| D005 | **PASS** | Triple breathing: LFO floor 0.01 Hz, step rate floor 0.01 Hz, CA autonomous evolution. Cannot not breathe. |
| D006 | **PASS** | Velocity (3 routes), MW to SYNAPSE, AT to chromatophore. Mono legato adds a 7th expression dimension. |

---

## Preset Schema Warning

The "Maximum Entropy" preset (`Presets/XOceanus/Aether/Outwit_Maximum_Entropy.xometa`) contains parameter names that do not match the current parameter layout: `owit_chaos`, `owit_density`, `owit_filterCutoff`, `owit_feedbackAmt`, `owit_mutationRate`, `owit_rule`, `owit_sync`, `owit_armBalance`, `owit_armSpread`, `owit_macroChaos`, `owit_macroCharacter`, `owit_macroCoupling`, `owit_macroSpace`, `owit_lfoDepth`, `owit_lfoRate`. These map to a pre-integration schema and would silently fail to load, leaving all parameters at default. This preset needs migration to the current `owit_arm{N}*` parameter layout, or it should be regenerated.

---

## Blessing Status

### BLS-017: Distributed CA Intelligence -- CONFIRMED (REINFORCED)

*Eight independent cellular automaton channels each evolving under distinct Wolfram rules, producing emergent polymetric rhythm and organic timbral evolution from pure computation. The only synthesizer where Rule 110 Turing-completeness has musical consequence. SYNAPSE circular coupling creates a computational feedback ring with emergent synchronization properties.*

Granted by: Buchla (10.0), Schulze (9.5), Pearlman (8.8), Tomita (8.8), Moog (8.5), Vangelis (8.5).
Full support: Smith (8.5, up from conditional 7.5 -- voiceMode/glide now wired), Kakehashi (7.0, up from conditional 6.5 -- macros teach the engine).

**All conditions from prior seance now satisfied.** BLS-017 is unconditional.

---

*Seance conducted: 2026-03-19 (revised assessment post-P1-P4 fixes)*
*Ghost council: Bob Moog, Don Buchla, Dave Smith, Ikutaro Kakehashi, Alan R. Pearlman, Isao Tomita, Vangelis, Klaus Schulze*
*BLS-017 Blessing: Distributed CA Intelligence -- CONFIRMED (unconditional)*
*Prior seance: 2026-03-18 (initial assessment, score 8.4/10)*
*Score improvement: 8.4 -> 8.7 (+0.3, driven by P1 voiceMode/glide fix)*
