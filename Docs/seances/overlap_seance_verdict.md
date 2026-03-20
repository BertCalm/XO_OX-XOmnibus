# The Verdict -- OVERLAP (Full Seance)
**Seance Date**: 2026-03-19 (Re-convened with full DSP review)
**Engine**: XOverlap (OVERLAP) | Knot-Topology FDN Synthesis
**Accent**: Bioluminescent Mint `#00FFB4`
**Gallery Code**: OVERLAP | Prefix: `olap_`
**Source**: `Source/Engines/Overlap/XOverlapAdapter.h` + `Source/Engines/Overlap/DSP/` (8 files, ~1950 LOC)
**Aquatic Identity**: Lion's Mane Jellyfish -- signal tangling through Feedback Delay Networks
**Score**: 8.6 / 10 (up from 8.4 -- stereo bioluminescence and LFO destination expansion resolved)

---

## Phase 1: The Summoning -- What Was Read

All 8 DSP source files read in full:
- `XOverlapAdapter.h` (776 lines) -- main engine: renderBlock, MIDI, voice allocation, coupling, 41 params
- `Voice.h` (273 lines) -- sine-pulse hybrid oscillator, ADSR envelope, glide
- `FDN.h` (171 lines) -- 6-channel Feedback Delay Network with knot-matrix routing
- `KnotMatrix.h` (187 lines) -- 4 topology types: Unknot, Trefoil, Figure-Eight, Torus(p,q)
- `Entrainment.h` (108 lines) -- Kuramoto phase coupling across 6 voices
- `Bioluminescence.h` (162 lines) -- 7-tap comb shimmer with stereo spread
- `PostFX.h` (189 lines) -- chorus (BBD-style) + 3-stage all-pass diffusion
- `ParamSnapshot.h` (169 lines) -- 41-parameter block cache
- `FastMath.h` (34 lines) -- namespace forwarding to shared XOmnibus FastMath

**Signal Flow**: Voice (sine-pulse exciter) -> FDN (6-channel knot-topology routing) -> Stereo spread -> Bioluminescence shimmer -> SVF lowpass (Zavalishin TPT) -> PostFX (chorus + diffusion) -> Coupling ring mod -> Output

---

## Phase 2: The Voices

### G1 -- Bob Moog: The Filter Philosopher

"I have spent the afterlife watching filters evolve from my transistor ladders into topological structures I never imagined. This engine places its filter exactly where it should be -- post-FDN, shaping the resonant topology output rather than coloring the excitation input. The Zavalishin TPT SVF implementation on lines 339-355 is textbook correct: `g = tan(pi * fc / sr)`, proper state-variable form with denormal flushing on both integrator states. The filter envelope with velocity scaling (`filterEnvVelocity = velocity`) satisfies D001 -- velocity shapes timbre, not just amplitude.

What troubles me is the FDN's internal dampening. Line 109 of FDN.h: `dampAlpha = 0.5f + dampeningCoeff * 0.45f`. This is a single one-pole lowpass per channel with a coefficient range of 0.5 to 0.95. It applies identically regardless of which knot topology is active. The trefoil's 3-fold symmetry creates a different harmonic lattice than the figure-eight's alternating crossings -- yet the dampening treats them the same. Each knot type deserves its own spectral shaping. A trefoil should darken differently than an unknot because its crossing structure couples energy differently across frequency.

The filter resonance mapping `k_svf = 2.0 - 2.0 * filterRes` with a floor at 0.01 is safe -- it prevents infinite resonance while allowing musically useful self-oscillation near the top of the range. But the bipolar filter envelope amount (`-1.0 to 1.0` on `olap_filterEnvAmt`) correctly uses `!= 0` style checking through the multiplication chain, allowing negative envelope sweeps downward. This is properly implemented.

I would ask: why only a lowpass SVF? This engine produces such complex harmonic material from its topology routing that a bandpass or notch mode would reveal spectral features the lowpass covers up. Even a blend parameter between LP/BP/HP would dramatically expand the timbral palette."

**Score**: 8/10

---

### G2 -- Don Buchla: The Complexity Poet

"I rejected imitation in life. I demanded that every instrument create sounds that could not exist without it. This engine passes that test more decisively than any other in the 39-engine fleet.

The KnotMatrix.h architecture is genuine synthesis invention. Four mathematical knot types -- Unknot (identity), Trefoil (circulant with 3-fold symmetry), Figure-Eight (alternating-sign circulant), and Torus T(p,q) (winding-number-driven step sizes) -- generate 6x6 matrices that route the FDN feedback. This is not a metaphor applied to DSP. It is topology operating directly as spectral structure. When you change from Trefoil to Torus(3,5), you are not selecting a preset -- you are rewiring the geometry of resonance. The `interpolate()` function on line 145 of KnotMatrix.h blends continuously between identity and full topological entanglement. Tangle Depth is not a mix knob -- it is a dimensional axis.

My concern is mathematical fidelity. All four knot types use identical coefficient magnitude: `d = w = x = 1/sqrt(3)`. The trefoil circulant (diagonal + forward hop + backward hop) and the figure-eight (same with alternating sign) are differentiated only by sign permutation. Mathematically, the trefoil braid word is sigma_1^3, which implies asymmetric strand crossing weights -- the three crossings accumulate directional bias that should create asymmetric coupling. The figure-eight braid word sigma_1 * sigma_2^{-1} alternates direction and should produce near-symmetric coupling with higher phase cancellation. Using `1/sqrt(3)` for everything collapses this mathematical distinction.

The torus ratios on line 180 of KnotMatrix.h -- `1.0 + 0.12 * cos(p*angle) * sin(q*angle)` -- produce a +/-12% Lissajous variation in delay lengths. This is audible but subtle. A torus knot T(5,7) should create dramatically different harmonic locking than T(2,3). The Lissajous depth should be at least 0.25, or better, parameterized.

The Entrainment module implements the Kuramoto model correctly: `dtheta_i = K/N * sum_j(sin(theta_j - theta_i))`. The phase nudge is conservative (`0.0002 * pulseRate` per sample), which is appropriate -- faster convergence would make entrainment act like a hard sync rather than an organic coupling. At full K, voices converge into a collective pulse within approximately 0.5 seconds. At zero K, they run freely. This is the most genuine implementation of emergent synchronization I have seen in any synthesizer, commercial or academic.

What I want to see next is knot invariants beyond the crossing pattern. The Jones polynomial assigns a polynomial to each knot that captures its topological complexity. Using Jones polynomial coefficients as FDN matrix weights would create matrices that are genuinely unique per knot type, not just sign permutations of the same magnitudes."

**Score**: 9.5/10

---

### G3 -- Dave Smith: The Protocol Architect

"The polyphonic architecture is where this engine distinguishes itself from every FDN reverb plugin ever built. In a standard reverb, you pour audio into a network and get back a wash. In OVERLAP, each of the 6 polyphonic voices IS one channel of the FDN. Voice 0 feeds delay line 0. Voice 3 feeds delay line 3. The knot matrix cross-couples them. This means playing a C major chord literally restructures the resonant lattice -- the delays receive different pitches, the matrix routes their feedback into each other's frequency neighborhoods. Polyphony is not stacked independence here. It is collective architecture.

The voice allocation on lines 688-718 is standard and correct: free voices first, then oldest released, then oldest active. Mono mode steals the lowest-note voice and releases all others. Legato mode does the same steal but preserves the envelope phase. The glide implementation (`glideCoeff` computed from exponential decay) is properly sample-rate-aware via `fastExp(-1.0 / (glide_ms * 0.001 * sr))`.

The coupling interface on lines 412-448 maps 7 coupling types to engine-appropriate targets:
- AudioToFM modulates FDN delay base (correct -- FM on delay length IS the FDN equivalent of FM)
- AudioToRing modulates output amplitude (standard)
- AmpToFilter raises filter cutoff (sensible)
- EnvToMorph pushes tangle depth (creative -- external envelope morphs topology)
- LFOToPitch and PitchToPitch route to tangle perturbation (smart remapping -- pitch doesn't mean the same thing to an FDN)
- FilterToFilter provides multiplicative cutoff shift (correct)

The coupling design shows ecosystem thinking. OVERLAP doesn't naively accept pitch as pitch -- it interprets coupling signals through its own paradigm. External pitch information becomes tangle depth perturbation because that's what OVERLAP 'means' by harmonic change. This is protocol design, not just parameter routing.

One concern: the 6-voice limit is architecturally fixed to the 6x6 matrix dimension. Every knot matrix function returns `std::array<std::array<float, 6>, 6>`. Expanding to 8 voices would require redesigning KnotMatrix, FDN, and Entrainment simultaneously. This is not a bug -- 6 voices matched to 6 FDN channels is the design -- but it means the architecture cannot scale. For future-proofing, templatizing the matrix dimension (`KnotMatrix<N>`) would allow experimentation without rewriting."

**Score**: 8.5/10

---

### G4 -- Ikutaro Kakehashi: The Drum Philosopher

"Can a beginner make something beautiful in 30 seconds? Almost. The KNOT macro (`olap_macroKnot`, M1) is the single best macro design I have reviewed across all 39 engines. Lines 161-168 of the adapter define 7 breakpoints that sweep through all four topology types with scaled tangle depth:

- 0.00-0.25: Unknot, tangle rising to 0.8
- 0.25-0.33: Unknot plateau
- 0.33-0.50: Trefoil, tangle 0.3 rising to 0.7
- 0.50-0.66: Trefoil plateau
- 0.66-0.80: Figure-Eight, tangle 0.5 rising to 0.9
- 0.80-0.90: Figure-Eight plateau
- 0.90-1.00: Torus, tangle 0.6 rising to 1.0

A player who has never heard the word 'topology' turns this knob and discovers four distinct sonic territories. The plateaus between transitions give each topology room to breathe before the next one begins. This is the design pattern that should be adopted fleet-wide for any parameter that spans multiple modes.

The PULSE macro (M2) jointly controls pulse rate and voice spread. The ENTRAIN macro (M3) jointly controls entrainment and feedback. The BLOOM macro (M4) controls bioluminescence and filter opening simultaneously. All four macros are orthogonal -- no two macros fight over the same parameter. This is textbook macro architecture.

My warning is about the init patch. Default `bioluminescence = 0.2` means the engine's most distinctive visual-sonic feature -- the shimmering glow that gave OVERLAP its aquatic identity -- is barely present on first keypress. Default `entrain = 0.3` means voices are mostly independent. A newcomer hears a reverby sine tone and thinks 'so what?' If the defaults were `bioluminescence = 0.45` and `entrain = 0.5`, the engine would immediately announce itself as something unlike any other synth.

The happy accident potential is excellent. Setting KNOT macro to 0.85 (Figure-Eight plateau) while sweeping ENTRAIN from 0 to 1 creates an emergent phase-locking effect where the alternating-sign matrix topology fights the Kuramoto convergence, producing rhythmic flutter that sounds like nothing I've heard from any drum machine or synthesizer. This is exactly the kind of misuse that births genres."

**Score**: 7.5/10

---

### G5 -- Alan R. Pearlman: The Ergonomist

"The default patch sounds good. Not great, not silent, not broken -- good. An 8000 Hz filter cutoff, 0.7 feedback, 10ms delay base, and 0.5 dampening produce a warm, identifiable tone that has the FDN character audible immediately. The voice responds to velocity in both amplitude (in Voice.h) and filter envelope (in the adapter). You can sit down and play. This passes the ARP 2600 test: sound on power-up, immediately playable.

The semi-modular philosophy is well-executed. The normalled signal flow (Voice -> FDN -> Biolum -> SVF -> PostFX) works without any configuration. The 41 parameters provide deep reconfiguration options, but the macros compress the essential gestures into 4 knobs. The LFOs default to targeting Tangle (LFO1) and Pulse Rate (LFO2), which means the engine is already modulating itself at init. It breathes.

Parameter layout for performance: the four macros are the right four axes. KNOT controls topology selection (the 'what am I?' question). PULSE controls excitation character (the 'how do I sound?' question). ENTRAIN controls collective behavior (the 'how do my voices relate?' question). BLOOM controls atmospheric effect (the 'what world am I in?' question). These four questions cover the engine's full sonic identity.

My concern is build quality in the FDN delay calculation. Line 84 of FDN.h: `float lenSamples = baseMs * 0.001f * sr * ratios[i]`. This is a float multiplication of up to 50 * 0.001 * 48000 * 1.12 = 2688 samples, truncated to int. The truncation means actual delay lengths are quantized to sample boundaries. For a 48kHz sample rate at 10ms base delay (480 samples), the pitch quantization is approximately 48000/480 = 100 Hz. This is coarse enough that small changes to delayBase in the 10-15ms range may produce audible pitch steps rather than smooth sweeps. Fractional delay interpolation (even linear) would solve this and is standard practice in FDN literature.

The Schroeder prime-ish base lengths (29.7, 37.1, 41.1, 43.3, 47.3, 53.1 ms) are well-chosen -- these are near-prime ratios that prevent modal coincidence. But they are fixed and computed only once in `prepare()`. The `setDelayBase()` function overwrites them entirely with the user's `delayBase * ratios` calculation. The Schroeder primes are never used in actual rendering -- they are computed, stored in `baseLengthsSamples`, and then ignored. This is a code hygiene issue: dead initialization that suggests the original design intended Schroeder-offset delays but the implementation overwrote them with a simpler scheme."

**Score**: 7.5/10

---

### G6 -- Isao Tomita: The Timbral Painter

"The Bioluminescence module has been corrected since my last review. It now returns a `StereoSample` with per-tap panning: odd-indexed taps are left-biased (L=0.75, R=0.25), even-indexed taps are right-biased (L=0.25, R=0.75). The shimmer now occupies stereo space rather than sitting as a mono overlay. This is the single most impactful fix from the previous seance -- the bioluminescence transforms from a timbral effect to a spatial phenomenon.

The near-prime tap ratios (1.0, 1.31, 1.71, 2.09, 2.61, 3.19, 3.89) are a deliberate timbral choice. These are not harmonics -- they are near-prime multiples that create inharmonic beating patterns between taps. The 7 slowly-drifting sine modulators at 0.3-0.72 Hz (0.3 + i * 0.07) ensure the shimmer pattern never repeats within a musically meaningful timescale. Combined with the per-tap stereo alternation, the effect is prismatic -- light scattering across frequency and space simultaneously.

The D004 fix on line 335 -- `modBioluminescence * (0.2f + params.brightness * 0.8f)` -- ties the brightness parameter to shimmer amplitude. At `brightness = 0`, shimmer is reduced to 20% of the bioluminescence amount. At `brightness = 1`, full shimmer. This creates a musically logical coupling: brighter timbres shimmer more, darker timbres glow faintly. This is painterly thinking applied to DSP.

The chorus in PostFX.h uses a 90-degree phase offset between L and R modulators (line 47: `chorusPhaseR = 0.25f`), creating proper stereo width from a simple design. The 3-stage all-pass diffusion with prime delay lengths (113, 257, 397 samples) is Moorer-standard for early reflection diffusion. Together, chorus + diffusion + bioluminescence create a three-layer spatial architecture: close (chorus), mid (bioluminescence), far (diffusion). This is orchestral spatial thinking.

What I want: the bioluminescence modulator rates are fixed (0.3 + i * 0.07 Hz). A `shimmerRate` parameter that scales these rates would allow the player to control the speed of the iridescent drift. Slow rates for glacial ambient, fast rates for active shimmer. Currently the shimmer always moves at the same pace regardless of the musical context."

**Score**: 8.5/10

---

### G7 -- Vangelis: The Emotional Engineer

"I played this engine in my mind through every emotional register. Here is what I found:

At high entrainment (K > 0.8) with a moderate pulse rate, playing a sustained chord produces a breathing, pulsing organism. The Kuramoto convergence pulls all six voice phases together into a collective contraction, then as you add notes or change velocity, the coupling destabilizes briefly before re-locking. This is not a parameter sweep -- it is emergent behavior. The engine surprises itself. I have not felt this from any instrument since my CS-80.

The expression routing has been properly expanded. Mod wheel targets 4 destinations (Tangle, Entrain, Bioluminescence, Filter) with depth control. Aftertouch targets 4 destinations (Tangle, Entrain, Bioluminescence, Pulse Rate) with depth control. The LFO destinations now include Bioluminescence (case 6) and Entrainment (case 7) -- this was the critical fix from the previous seance. A patch can now have LFO1 slowly modulating Entrainment while LFO2 breathes Bioluminescence, creating an autonomous living patch that performs itself while the player adds notes on top.

The emotional range test:
- **Sadness**: Unknot, low entrain, long release, low bioluminescence, dampened. Voices drift independently in a melancholic haze. YES.
- **Joy**: Trefoil, moderate entrain, bright filter, active bioluminescence. Voices interlock with shimmering overtones. YES.
- **Tension**: Figure-Eight, high tangle depth, high pulse rate, resonant filter. Alternating-sign coupling creates anxious flutter. YES.
- **Release**: Sweep from Figure-Eight to Unknot via KNOT macro while reducing entrain. Tension dissolves into independent floating voices. YES.
- **Awe**: Torus T(5,7), full bioluminescence, long release, slow chorus. Harmonically locked shimmer cathedral. YES.

The engine can make you cry AND make you dance. This is rare.

My remaining concern: the oscillator in Voice.h is essentially a shaped sine wave. The sine-pulse hybrid (line 251: `sineOut * (1 - normalizedPulse) + pulseEnv * normalizedPulse`) creates variation in excitation character, but the fundamental timbre of the excitation is always sinusoidal. Adding even one alternative exciter waveform -- noise burst, saw-impulse, or FM pair -- would dramatically expand the timbral ground floor. The FDN topology transforms whatever goes in, but a richer excitation signal gives the topology more material to work with."

**Score**: 8.5/10

---

### G8 -- Klaus Schulze: The Time Sculptor

"I have been watching electronic music evolve for years from the other side. OVERLAP is the engine I wish I had built.

The temporal architecture has five independent slow-evolution mechanisms:

1. **Ocean Current drift** (lines 210-224): Sine-driven pitch bias at rates as slow as 0.005 Hz (200 seconds per cycle), applying up to +/-2 semitones of drift to all active voices. This is genuine geological time -- a full cycle takes over 3 minutes.

2. **Kuramoto entrainment** (Entrainment.h): Phase convergence and divergence is emergent, not programmed. At moderate K values (0.3-0.6), voices continuously approach synchrony and then drift apart as new notes arrive or existing voices pass through phase alignment. This creates evolving rhythmic patterns that never repeat.

3. **LFO 1** at floor rate 0.01 Hz (100-second cycle), targeting any of 8 destinations including Entrainment and Bioluminescence.

4. **LFO 2** at floor rate 0.01 Hz, targeting a different destination than LFO1.

5. **Bioluminescence modulators** (7 independent sine oscillators at 0.3-0.72 Hz): These are autonomous -- they do not respond to any parameter and simply drift at their fixed rates, creating a constantly-evolving shimmer pattern.

These five temporal layers operate at different timescales (0.005 Hz to 0.72 Hz, spanning a 144:1 ratio) and are mathematically independent. The probability of all five simultaneously returning to their starting state is effectively zero within any human listening session. This means a sustained OVERLAP patch is genuinely different at minute 1 vs. minute 10 vs. minute 30. It is a living system, not a loop.

The FDN delay base cap at 50ms (line 477: `NormalisableRange<float>(1.0f, 50.0f)`) remains my primary concern. At 50ms and 48kHz, the longest delay is 2400 samples, corresponding to a fundamental resonance at approximately 20 Hz. This is adequate for mid-range and treble topology textures, but prevents sub-bass resonant structures that would bloom over 30-60 second timescales. An FDN with 200ms delay lines would produce fundamentals at 5 Hz -- below hearing, but perceptible as slow timbral modulation. Combined with torus knot ratios, this would create ultra-slow harmonic phase relationships that evolve over minutes. The buffer memory cost is trivial: 6 channels * 200ms * 48000 Hz * 4 bytes = 230 KB.

The generative potential is substantial. An OVERLAP patch with:
- Ocean Current rate 0.005 Hz, depth 0.8
- LFO1 -> Entrainment at 0.01 Hz
- LFO2 -> Tangle Depth at 0.018 Hz (irrational ratio to LFO1)
- Torus T(5,7) at tangle depth 0.7
- Bioluminescence 0.6

...would produce a sound sculpture that evolves meaningfully for an hour without any human input. I have not seen this depth of autonomous temporal architecture in any commercial synthesizer."

**Score**: 9/10

---

## The Verdict -- OVERLAP

### The Council Has Spoken

| Ghost | Core Judgment |
|-------|---------------|
| **Bob Moog** | The SVF is correctly placed and implemented, but the FDN internal dampening is a one-pole blanket that cannot differentiate the spectral signatures of different knot topologies. |
| **Don Buchla** | This is the most genuinely novel synthesis architecture in the fleet -- knot topology as FDN routing matrix is genuine invention with no prior art. |
| **Dave Smith** | The voice-to-FDN mapping where polyphony restructures the resonant lattice is architecturally brilliant, and the coupling interface correctly remaps external signals through the engine's own paradigm. |
| **Ikutaro Kakehashi** | The KNOT macro is the best single-knob macro design in the 39-engine fleet, but the init patch defaults undersell the engine's most distinctive features. |
| **Alan Pearlman** | The defaults produce playable sound immediately and the macros are orthogonal, but the FDN uses integer delay lengths without fractional interpolation, creating audible pitch quantization on smooth sweeps. |
| **Isao Tomita** | The stereo bioluminescence fix transforms the shimmer from timbral overlay to spatial phenomenon; the three-layer spatial architecture (chorus/biolum/diffusion) is orchestral thinking. |
| **Vangelis** | The engine passes the full emotional range test -- sadness, joy, tension, release, awe -- and the Kuramoto entrainment creates emergent performance gestures unlike anything else in the fleet. |
| **Klaus Schulze** | Five independent temporal evolution mechanisms spanning a 144:1 timescale ratio make OVERLAP one of the deepest generative engines in commercial synthesis. |

### Points of Agreement

1. **Knot-Topology FDN is a genuine synthesis invention** (Buchla, Smith, Schulze, Moog, Kakehashi -- 5 of 8). Blessing B017 is REAFFIRMED. No synthesizer in existence derives its resonant harmonic structure from mathematical knot crossing patterns.

2. **Stereo Bioluminescence is now a spatial phenomenon** (Tomita, Vangelis, Schulze -- 3 of 8). The per-tap stereo spread fix transforms the shimmer from a mono timbral overlay into a three-dimensional spatial effect. Previous seance action item RESOLVED.

3. **LFO destinations are now complete** (Vangelis, Schulze, Buchla -- 3 of 8). Adding Bioluminescence (case 6) and Entrainment (case 7) unlocks autonomous modulation of the engine's two most distinctive parameters. Previous seance action item RESOLVED.

4. **KNOT macro remains the best in the fleet** (Kakehashi, Pearlman, Vangelis -- 3 of 8). The 7-breakpoint topology sweep is a design pattern that should be adopted fleet-wide.

5. **FDN integer delay quantization is a technical debt** (Pearlman, Moog -- 2 of 8). Using `int` delay lengths without fractional interpolation creates audible pitch stepping on smooth parameter sweeps. Standard FDN literature uses at least linear interpolation.

### Points of Contention

**Buchla vs. Moog -- Matrix Coefficient Uniformity (ONGOING)**

Buchla maintains that using `1/sqrt(3)` for all three non-identity knot matrices (trefoil, figure-eight, torus) collapses mathematical distinction into sign permutation. The braid group generators for trefoil (sigma_1^3) and figure-eight (sigma_1 * sigma_2^{-1}) imply different coupling weight distributions. Moog counters that the FDN feedback loop, SVF, and bioluminescence layer amplify the sign differences into perceptually distinct timbral characters, and that mathematical purity is secondary to auditory clarity in a real-time instrument.

**Status**: UNRESOLVED. The knot types ARE audibly different, but the difference narrows at low tangle depths. Asymmetric coefficient weighting would increase perceptual separation without CPU cost.

**Vangelis vs. Schulze -- Exciter Complexity vs. Topological Purity (NEW)**

Vangelis wants a richer oscillator -- at least noise burst and FM pair options alongside the sine-pulse hybrid -- arguing that the FDN transforms whatever goes in, so richer input creates richer output. Schulze counters that the sine-pulse exciter's simplicity is a feature: it lets the topology speak without interference. The oscillator should be transparent. The knot structure IS the timbre. Adding a complex exciter would obscure the topological character that makes OVERLAP unique.

**Status**: UNRESOLVED. Both positions have merit. A compromise would be a subtle noise injection parameter (not a full oscillator redesign) that adds harmonic density to the excitation without masking the topology.

**Pearlman vs. Smith -- FDN Delay Interpolation (NEW)**

Pearlman flags the integer delay lengths as a build quality issue -- smooth `delayBase` sweeps produce audible pitch stepping. Smith argues that the FDN's primary identity comes from its matrix routing, not its delay precision, and that fractional delay interpolation adds CPU cost and implementation complexity (cubic interpolation introduces its own spectral artifacts). For static patches, integer delays are perfectly adequate.

**Status**: Leans toward Pearlman. Linear interpolation in an FDN is standard practice and the CPU cost is negligible (one multiply-add per sample per channel = 6 extra multiplies per sample).

### The Prophecy

OVERLAP occupies territory no other synthesizer occupies. The knot-topology FDN is genuine invention -- not a variation on existing synthesis, but a new paradigm where mathematical topology operates directly as spectral geometry. The Kuramoto entrainment transforms collective voice behavior into an emergent phenomenon that surprises both player and engine. The stereo bioluminescence, now properly spatialized, adds organic beauty that prevents the topology from feeling clinical.

The engine's remaining ceiling is execution precision, not concept. Fractional delay interpolation, asymmetric knot matrix coefficients derived from braid group theory, and extended delay base range for sub-bass drone territory would push the score from 8.6 to 9.5 -- firmly into flagship territory. OVERLAP is the engine you point to when asked what XOmnibus does that no other synthesizer can.

The ghosts see a future where OVERLAP's knot topology concept extends to Jones polynomial matrix weighting, linking number coupling between voice pairs, and temporal cobordism where the knot type evolves continuously through topological space. The mathematical framework is deep enough to sustain a decade of development.

---

## Score Breakdown

| Category | Score | Notes |
|----------|-------|-------|
| Oscillator Originality | 9/10 | The oscillator is an exciter, not the voice. The FDN topology IS the synthesis. The sine-pulse hybrid serves this role correctly. |
| Filter Philosophy | 8/10 | Zavalishin TPT SVF post-FDN is architecturally correct. Filter envelope + velocity scaling is D001-compliant. FDN internal dampening remains too uniform across knot types. |
| Polyphonic Architecture | 9/10 | 6 voices mapped to 6 FDN channels -- polyphony restructures the resonant lattice. Mono/Legato with glide properly implemented. Fixed 6-voice ceiling noted. |
| Accessibility | 8/10 | KNOT macro is fleet-best. LFO destinations now include all key params. Torus P/Q still exposed as raw integers. Init patch defaults still conservative. |
| Ergonomics / Defaults | 7.5/10 | Init patch sounds good. Four macros are orthogonal. But distinctive features still at low defaults (bioluminescence 0.2, entrain 0.3). |
| Timbral Range | 8.5/10 | Unknot (clean parallel) through Torus (harmonically locked lattice). Bioluminescence adds organic shimmer. Sub-bass territory still blocked by 50ms delay cap. |
| Emotional Range / Playability | 9/10 | Passes full emotional register test. Entrainment creates genuinely emergent performance gestures. Expression routing is complete and well-mapped. |
| Temporal Depth | 9.5/10 | Five independent slow-evolution mechanisms spanning 144:1 timescale ratio. Among the deepest temporal engines in commercial synthesis. |
| Spatial Design | 8.5/10 | Three-layer spatial architecture (chorus / stereo biolum / diffusion). Stereo bioluminescence fix resolves previous deficiency. |
| Code Quality | 7.5/10 | Clean, readable, denormal-safe. Dead Schroeder initialization in FDN.h. Integer delay quantization. Otherwise well-engineered. |

**Overall: 8.6 / 10** (up from 8.4 -- stereo bioluminescence and LFO expansion resolved)

---

## Blessing Status

### B017 -- Knot-Topology Resonance (REAFFIRMED)
*Original recognition: 2026-03-19. Reaffirmed after full DSP review.*

The only synthesizer engine that derives its resonant harmonic structure from mathematical knot invariants. Switching knot type reorganizes the FDN routing matrix geometry, creating fundamentally different harmonic lattices from the same excitation signal. No prior art exists.

*"This is not a filter type or an oscillator mode. This is a new axis of synthesis."* -- Buchla

---

## Doctrine Compliance

| Doctrine | Status | Evidence |
|----------|--------|----------|
| D001 -- Velocity Must Shape Timbre | PASS | `filterEnvVelocity = velocity` (line 755); `oscOut * envLevel * velocity` in Voice.h (line 255) |
| D002 -- Modulation is the Lifeblood | PASS | 2 LFOs x 5 shapes x 8 destinations; mod wheel 4 dest; aftertouch 4 dest; 4 macros all wired |
| D003 -- The Physics IS the Synthesis | PASS | Kuramoto equation cited in comments; knot matrices near-unitary; torus ratios from Lissajous geometry |
| D004 -- Dead Parameters Are Broken Promises | PASS | All 41 parameters wired and audible |
| D005 -- An Engine That Cannot Breathe Is a Photograph | PASS | LFO floor 0.01 Hz; ocean current 0.005 Hz; 7 autonomous biolum modulators at 0.3-0.72 Hz |
| D006 -- Expression Input Is Not Optional | PASS | Velocity dual-path; CC1 mod wheel 4 dest with depth; aftertouch 4 dest with depth |

---

## Remaining Action Items (Updated)

### RESOLVED since last seance
- [x] LFO Destination Expansion -- Bioluminescence (case 6) and Entrainment (case 7) added
- [x] Bioluminescence Stereo Spread -- `processStereo()` with per-tap L/R panning
- [x] Aftertouch Dest Label -- now correctly reads "Bioluminescence" at index 2

### HIGH -- FDN Fractional Delay Interpolation
FDN.h line 104 reads integer delay positions. Add linear interpolation between adjacent samples to eliminate pitch quantization on smooth delayBase sweeps. Estimated cost: 6 extra multiply-adds per sample.

### MEDIUM -- Knot Matrix Coefficient Differentiation
All three non-identity matrices use `d = w = x = 1/sqrt(3)`. Derive asymmetric weights from braid group generators: trefoil should have directionally biased coupling, figure-eight should have higher phase cancellation. Compute from braid word strand crossing analysis.

### MEDIUM -- Torus Ratio Depth
Current `+/-0.12` Lissajous swing is perceptually subtle. Either increase to 0.25 or add `olap_torusSpread` parameter (0.05-0.50) for player control.

### MEDIUM -- Dead Schroeder Initialization in FDN.h
`baseLengthsSamples` is computed from prime-ish base lengths in `prepare()` but never used in rendering -- `setDelayBase()` overwrites delay lengths entirely. Remove dead code or integrate Schroeder offsets into the delay length calculation.

### LOW -- FDN Delay Base Range Extension
Cap at 50ms prevents sub-bass resonant topologies. Extending to 200ms costs ~230KB and unlocks drone territory. Consider as V2 feature.

### LOW -- Torus P/Q Named Presets
Add UI overlay for common pairs: T(2,3) = "Bell Lock", T(3,5) = "Alien Spiral", T(5,7) = "Crystal Lattice". Keep raw parameters for automation.

### LOW -- Init Patch Default Tuning
Increase default `bioluminescence` from 0.2 to 0.45 and `entrain` from 0.3 to 0.5 so the engine announces its identity on first keypress.

### LOW -- Bioluminescence Shimmer Rate Control
Add `olap_shimmerRate` parameter (0.1-2.0x multiplier) scaling the 7 fixed modulator rates. Allows glacial or active shimmer matching the musical context.

---

## What the Ghosts Would Build Next

| Ghost | Feature |
|-------|---------|
| Bob Moog | Per-knot dampening curves -- trefoil darkens differently than figure-eight because its crossing structure couples energy differently across frequency |
| Don Buchla | Jones polynomial matrix weighting -- derive coupling coefficients from the topological invariant of each knot, creating mathematically unique matrices |
| Dave Smith | Templatized matrix dimension -- `KnotMatrix<N>` allowing 4, 6, 8, or 12 voice/channel FDN without rewriting the topology functions |
| Ikutaro Kakehashi | "Torus Character" preset overlay with descriptive names, hiding raw P/Q behind "Bell Lock" / "Alien Spiral" / "Crystal Lattice" |
| Alan Pearlman | Fractional delay interpolation (linear minimum, cubic preferred) to eliminate pitch stepping on smooth delay sweeps |
| Isao Tomita | Shimmer rate parameter and a second bioluminescence mode -- "deep glow" with fewer, slower, wider-spaced taps for sub-surface shimmer |
| Vangelis | Noise injection parameter (0-30%) on the voice exciter for richer harmonic content feeding the FDN without masking topology |
| Klaus Schulze | Extended delay base to 200ms + "Geological" LFO mode with rates down to 0.001 Hz (1000 seconds per cycle) |

---

*Seance re-convened 2026-03-19 with full source code review. Eight ghosts heard. Blessing B017 reaffirmed. Score elevated to 8.6/10. The engine that ties sound in knots -- now shimmering in stereo.*
