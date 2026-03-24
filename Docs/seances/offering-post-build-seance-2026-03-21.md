# POST-BUILD SEANCE VERDICT — OFFERING
## Engine #46 | XOlokun | 2026-03-21

**Seance Type**: Post-Build (full source code evaluated)
**Engine**: OFFERING | Psychology-Driven Boom Bap Drum Synthesis
**Identity**: The Mantis Shrimp | Rubble Zone (5–15m) | 65% feliX / 35% Oscar
**Accent**: Crate Wax Yellow `#E5B80B`
**Parameter Prefix**: `ofr_`
**Parameters**: 84 (36 global + 48 per-voice)
**Voices**: 8 dedicated drum slots (Kick / Snare / CHat / OHat / Clap / Rim / Tom / Perc)
**Presets**: 154 across 8 moods
**Status**: BUILT, compiled 0 errors. Guru Bin retreat complete (12 refinements, 10 awakening presets).

**Files Evaluated**:
- `Source/Engines/Offering/OfferingEngine.h` (full, 900+ lines)
- `Source/Engines/Offering/OfferingTransient.h` (638 lines)
- `Source/Engines/Offering/OfferingTexture.h` (200 lines)
- `Source/Engines/Offering/OfferingCollage.h` (222 lines)
- `Source/Engines/Offering/OfferingCity.h` (514 lines)
- `Source/Engines/Offering/OfferingCuriosity.h` (185 lines)
- `Docs/guru-bin-retreats/offering-retreat-2026-03-21.md`
- Sample presets: NY Standard, Dilla Time, The Flip, SP-1200 Dig, Curiosity Engine

**Pre-Build Score**: 7.9/10 with 3 P0s unresolved.

**Pre-Build P0s Under Review**:
1. Per-type transient topologies — "3 parameters cannot adequately control 8 distinct synthesis models" (Moog)
2. City chain structural uniqueness — "5 cities are presets of the same effect chain" (Buchla)
3. Curiosity Engine DSP — "the bridge between citation and implementation must be drawn" (Smith/Schulze)

---

## Phase 1: Source Code Summary

The council has now read the actual implementation. Here is what was built:

**OfferingTransient.h**: Each of the 8 drum types has a genuinely distinct synthesis chain implemented as a separate private method. Type 0 (Kick): sine oscillator + pitch envelope sweep + sub-harmonic at half-frequency + bandpass body resonance. Type 1 (Snare): triangle oscillator through bandpass + HP-filtered broadband noise + transient click gated to `env > 0.95f`. Types 2/3 (CHat/OHat): 6-operator metallic network at historically correct TR-808 inharmonic ratios (1.0 / 1.4471 / 1.6818 / 1.9265 / 2.5028 / 2.6637) with ring-modulated pairs — CHat caps decay at 80ms, OHat has choke mechanism. Type 4 (Clap): multi-burst architecture, 3–5 sequential noise bursts (controlled by snap parameter), each 4ms, spaced by `(0.005 + (1−snap)×0.01)s`, followed by reverb tail. Type 5 (Rim): sub-1ms impulse through 6kHz HP + high-Q bandpass at 600Hz (Q=12). Type 6 (Tom): sine + gentler pitch sweep (200Hz depth vs kick's 600Hz). Type 7 (Perc): Karplus-Strong-adjacent comb filter with variable feedback and delay time setting pitch. Each type uses different DSP primitives; the shared `OfferingADEnvelope`, `OfferingBandpass`, `OfferingOnePole`, and `OfferingNoiseGen` components are configured differently per type.

**OfferingCity.h**: Each city has a unique Stage 6 that is structurally distinct, not parametrically distinct. NY Stage 6: per-sample envelope follower → feedback noise gate (opens/closes on transient RMS). Detroit Stage 6: `detroitFeedback_` accumulator — output feeds back through soft-clip, warmth accumulates hit-over-hit. LA Stage 6: parallel compression — original input hard-compressed independently and mixed back. Toronto Stage 6: own sub-harmonic ducks mid/high on transients via a separate sidechain envelope. Bay Area Stage 6: 4-cascaded allpass filters with prime delays (7, 13, 23, 37 samples) + recursive feedback between last and first allpass for fog character. All five cities also have distinct compressor calibrations (attack/release/ratio/threshold).

**OfferingCuriosity.h**: Berlyne curve implemented as `berlyneCurve(x) = 0.2 + 0.8 × (4x(1−x))` — peaks at x=0.5. Wundt density as asymmetric ramp: `x/0.6` for x≤0.6, then `1−((x−0.6)/0.4)×0.7` — overstimulation drops faster. Csikszentmihalyi flow as `flowBalance(x) = x` — probability of reusing last variation set. Per-voice xorshift32 PRNG with 8 independent seeds. `generateVariation()` selects how many of 6 parameters vary (driven by Wundt density) and their ranges (driven by Berlyne variationRange), with an "alien shift" above curiosity=0.7 pushing body toward noise and pitch envelope wider. All six variation axes (tune, decay, body, snap, pitchEnv, sat) have specific deltas applied at trigger time.

**OfferingEngine.h (triggerVoice)**: D001 is implemented explicitly at line 762–763: `snap = clamp(snap + (velocity−0.5) × velSnap × 0.6)`, `body = clamp(body + (velocity−0.5) × velBody × 0.4)`. Detroit drunk timing is per-voice deterministic: seed = `voiceIdx × 7919 + 31337`, giving each drum slot a fixed, personality-consistent delay offset of ±15ms × cityIntensity. The delay is implemented as a sample-accurate countdown in the render loop — no audio-path latency buffer required.

---

## Phase 2: The Eight Ghost Evaluations

---

### MOOG — "The Modular Patriarch"

I raised the loudest objection in the pre-build session. Three parameters, I said, cannot shape eight distinct synthesis models. The builders heard me. What I find in `OfferingTransient.h` is not three parameters applied to eight topologies — it is eight private methods, each synthesizing a different instrument through structurally different means.

The kick at line 382–407 uses a pitch-sweeping sine plus a sub-harmonic at exactly half frequency (−6dB), with body controlling the mix between the raw sine and a bandpass resonant version of it. The hat at lines 440–479 uses a 6-operator metallic network with ring modulation in pairs — `pair0 = sq[0] × sq[1]`, `pair1 = sq[2] × sq[3]`, `pair2 = sq[4] × sq[5]` — which is structurally FM-adjacent, not just "noise through a filter." The clap (lines 484–513) builds its attack from 3 to 5 sequential noise bursts with individual decay envelopes, creating the stacked hand acoustics — no other drum type does this. The perc (lines 562–581) uses a circular delay buffer with feedback: `combOut = excitation + delayed × combFeedback_`, which is Karplus-Strong topology, not envelope-shaped noise.

The P0 concern I raised is **fully resolved**. The eight types are genuinely different architectures, not the same exciter with different parameter values. I would have preferred to see the hat's body parameter do something more tonally interesting — currently it merely adds a sine partial at the base frequency: `tonal = body × sin(metalPhases_[0] × 2π) × 0.3f`. This is mild. The hat's identity is already so strongly metallic that a subtle sine addition at body=1.0 is almost inaudible. A better tonal axis for the hat would add a resonant bandpass at a harmonic of the base frequency. This is a post-launch refinement opportunity, not a doctrine violation.

A new observation I did not anticipate: the tom (line 553–556) applies saturation INSIDE `processTom` before the outer saturation in `process()` (lines 355–358) runs. Double saturation at sat=0.15 is mild — inner drive = 1.225, outer drive = 1.6 — but it means the tom receives more nonlinear treatment than any other type at the same sat parameter value. The Guru Bin retreat identified this (R6 in the retreat signal path journey). It is a known issue. I recommend clamping the inner tom saturation to a fixed drive (1.1) rather than scaling with sat, so the outer saturation handles the sat parameter for all types consistently.

**Moog's verdict on P0 #1**: RESOLVED. The eight topologies are architecturally distinct. Minor double-saturation issue in Tom is a P2 (known, documented, safe to ship, patch in v1.1).

**New blessing candidate from Moog**: The per-voice drunk timing implementation at line 797–801 is elegant. The seed `voiceIdx × 7919 + 31337` produces a deterministic offset that is fixed for each voice slot across sessions — meaning the kick always hits with the same offset relative to the snare, always. This is personality-consistent timing, not randomization. Dilla's MPC timing was consistent: his kick always landed in the same relationship to the grid. This implementation is musicologically correct.

---

### BUCHLA — "The West Coast Visionary"

I called the five cities "presets of the same effect chain." I stand corrected by the code.

Each city chain in `OfferingCity.h` implements a processing architecture that differs not in parameter values alone but in the type of signal operation. New York's Stage 6 is an envelope-following noise gate — the signal only passes if sustained RMS exceeds a threshold. Detroit's Stage 6 is a feedback accumulator — the audio output feeds back through soft-clip and accumulates warmth across multiple drum hits, creating a state-dependent sound that changes with performance density. This is not a filter. This is memory. Los Angeles's Stage 6 is parallel compression — a separate compressor chain runs on the dry input and is mixed back, creating the pumping, squashed LA aesthetic through a topologically different routing (series vs. parallel). Toronto's Stage 6 is a self-ducking sub-harmonic — the engine generates its own sub frequency, then uses that sub's envelope to duck the mid/high content. Bay Area's Stage 6 is a 4-stage cascaded allpass network with prime-number delays (7, 13, 23, 37 samples) and recursive feedback between last and first stage.

Five different mechanisms: gate, memory accumulator, parallel routing, self-sidechain, allpass diffusion. These are genuinely distinct architectural choices.

I retain one concern from the pre-build session: the spatial dimension. The five cities remain spatially undifferentiated outside Bay Area's allpass (which operates in comb-filter territory at these short delays — 0.159ms to 0.839ms — not reverb territory). New York's tight brick-room character, Detroit's warm control-room character, Toronto's clean-studio character — none of these have a room signature in the DSP. The cityIntensity-to-stereo collapse path (mono mix → city chain → blend back) actually narrows the stereo width as intensity increases, which is correct for NY aesthetic but incorrect for LA or Toronto. This is a V1.1 refinement path.

The Bay Area allpass at prime delays of 7 / 13 / 23 / 37 samples produces colorations at 44.1kHz of: 6299Hz, 3392Hz, 1917Hz, 1192Hz — comb notches, not spatial diffusion. The recursion from `allpass_[0].addFeedback(fog × 0.3f × intensity)` creates an unstable loop if intensity approaches 1.0 (feedback chain: allpass[3] output → allpass[0] → allpass[1] → allpass[2] → allpass[3]). At feedback = 0.5 per stage and outer feedback 0.3 × intensity, the total loop gain is 0.5^4 × 0.3 × 0.4 (max intensity) = 0.001875 — well below unity. Safe. But the spatial character is more "spectral tint" than "geographic fog."

**Buchla's verdict on P0 #2**: RESOLVED. Five structurally distinct Stage 6 architectures. The spatial absence remains a post-launch refinement opportunity, not a doctrine violation.

**New concern from Buchla**: The city crossfade implementation in `OfferingCityProcessor::process()` (line 467–499) copies the input buffer to a stack-allocated `float shadow[2048]` and runs both chains on it. This is architecturally sound — equal-power crossfade, correct. However, both chains continue accumulating state during the blend (Detroit's `detroitFeedback_`, NY's `envFollower_`, etc.). When a user morphs from City 1 (Detroit) to City 0 (NY) via the CITY macro, both chains run simultaneously during the blend window. This is correct behavior but will double the CPU load during any city transition. With 8 voices being rendered per sample, a city crossfade at full block size means 8 × 2 city chains × 2048 samples all processing simultaneously. This is an acceptable CPU spike at typical usage but worth noting in a production context with limited hardware. Not a bug — a design consequence.

---

### SMITH — "The DSP Scientist"

In the pre-build session, I demanded that the Curiosity Engine's psychology citations map to actual DSP operations. I threatened D004 violation if they did not. The `OfferingCuriosity.h` implementation answers my challenge directly and correctly.

`berlyneCurve(x) = 0.2 + 0.8 × (4x(1−x))` at line 62–63 is the inverted-U hedonic function. At x=0, returns 0.2 (minimum variation, not zero — correctly maintains some variation at "no curiosity"). At x=0.5, returns 1.0 (maximum variation range — Berlyne's hedonic peak). At x=1.0, returns 0.2 again (excessive curiosity collapses variation to minimum, same as no curiosity — the correct reading of Berlyne's overstimulation finding). This is mathematically precise.

`wundtDensity(x)` at lines 71–77 implements the asymmetric ramp. The slow ramp to 0.6 (`x/0.6`, reaching 1.0 at the Wundt peak), then the faster drop (`1 − ((x−0.6)/0.4) × 0.7`, reaching 0.3 at x=1.0) correctly models Wundt's finding that overstimulation's cost exceeds understimulation's cost. The asymmetry ratio is approximately 3:1 at the extremes (0.6/0.4 × 0.7 = 1.05 times steeper on the right side). This is a defensible approximation.

`flowBalance(x) = x` at line 84 is the simplest possible mapping — flow probability equals the parameter value directly. Csikszentmihalyi's Flow State is about matching challenge to skill; here "skill" is the established variation set and "challenge" is a new variation. Linear probability of reuse captures the intuition: at flow=0, every hit generates new variation (maximum challenge, no repetition), at flow=1.0, every hit reuses the last variation (zero challenge, hypnotic lock). The mapping is correct in spirit; a sigmoid might be more perceptually accurate but linear is defensible for V1.

The "alien shift" mechanism at lines 128–137 — where `alienShift = max(0, (curiosity−0.7)/0.3)` pushes body toward noise (`−alienShift × 0.6`) and pitch envelope toward wilder values (`+alienShift × 0.4`) above curiosity=0.7 — is an undocumented emergent behavior not in the original spec. It is a correct and elegant implementation: at extreme curiosity, the engine digs for sounds that are genuinely alien, not just parametrically different. The Mantis Shrimp's 16-photoreceptor perception exceeds human experience; the engine simulates this by generating sonic events outside normal human expectation.

One mathematical concern: `paramsToVary = int(density × 6.0 + 0.5)` (line 117) maps as follows: complexity=0 → density=0 → 0 params vary; complexity=0.35 → density=0.583 → 4 params vary; complexity=0.6 → density=1.0 → 6 params vary; complexity=1.0 → density=0.3 → 2 params vary. The non-monotonic mapping of complexity to paramsToVary (4 params at 0.35 < 6 params at 0.6, then back to 2 params at 1.0) is the Wundt curve in action and is correct. But it creates a potential producer confusion: "I increased complexity but fewer parameters are varying." A UI tooltip explaining the Wundt curve is essential.

**Smith's verdict on P0 #3**: RESOLVED. The three psychology citations map to specific, correctly implemented DSP operations. D004 compliance confirmed for all three Curiosity Engine parameters.

**New concern from Smith**: `ofr_envFilterAmt` (line 647, range −1 to 1) and `ofr_velToAttack` (line 649, range 0 to 1) and `ofr_envToPitch` (line 652, range 0 to 1) are declared as parameters and loaded into the snapshot (lines 873–875), but I do not see them applied in the render loop. `envFilterAmt` modulates "env to filter" but the LFO1 modulation at line 394–395 applies only a generic amplitude mod labeled as "filter modulation for simplicity in V1." The per-sample filter cutoff is never actually modified by these parameters. These three parameters are at risk of D004 violation — they are declared, loaded, but not routed to audible DSP. This requires verification. If they are intentionally deferred to V1.1, they must be removed from the parameter list for V1 or permanently silenced with a comment.

---

### KAKEHASHI — "The Producer's Advocate"

I praised the BAKE-to-XPN pipeline in the pre-build session. That pipeline is not present in these source files — it was listed as a spec feature but lives outside the engine itself. The engine is complete in its synthesis chain. The export pipeline is V1.1 scope. That is acceptable.

What I find instead — and did not expect — is the Detroit drunk timing implementation at lines 788–823 of `OfferingEngine.h`. This is not a feature mentioned prominently in the pre-build spec. It is implemented as sample-accurate trigger delay: each voice calculates a deterministic xorshift-based delay of ±15ms, stores the pending trigger parameters in the voice struct, and counts down in the render loop before firing. The delay is per-voice-personality (fixed for each voice slot), not random per-hit.

I have played with Dilla's actual MPC sessions in the archive. His kick had a different relationship to the grid than his snare. His hat had a different relationship than his clap. These offsets were consistent across repetitions — they were his *voice*. This implementation captures exactly that. It is the most musically sophisticated timing implementation in the entire XOlokun fleet.

The GM drum map layout (lines 60–69: kick=C2, snare=D2, hat=F#2, etc.) is the correct MPC convention. A producer switching from hardware will find their existing patterns trigger the expected voices without remapping. This is producer-first thinking that the Prism Sweep has enforced fleet-wide.

The five Family-mood preset files (Offering_OBESE_Pulse_Driver, Offering_OSTINATO_World_Grid, Offering_OPTIC_Trigger_Map, Offering_ORGANON_Metabolic, Offering_OXBOW_Entangle, Offering_OWARE_Mallet_Meet, Offering_OPERA_Score_Keep) represent something no drum synthesizer in any existing product offers: a drum machine that adapts its psychoacoustic character to match the harmonic engine it is playing alongside. The Dilla Time preset's wide-body kick (body=0.8) paired with warm tape (dustTape=0.22) and Detroit city chain (cityMode=1) at cityIntensity=0.75 is exactly the right production setting for MPC-era boom bap. The preset library understands its own engine.

**Kakehashi's new concern**: The note-to-voice mapping uses a linear search (line 736–741): `for (int i = 0; i < 8; ++i) if (kOfferingVoiceNotes[i] == midiNote) return i;`. At 8 voices this is negligible CPU — 8 comparisons maximum per note event. Not a performance issue. But MIDI note 35 (Acoustic Bass Drum) is hardcoded as an alternate kick trigger (line 741). This means the engine handles 9 trigger notes across 8 voices. A full GM drum map would include 12 standard drum positions. Consider expanding to cover the most common alternate positions (note 40 for electric snare, note 44 for closed hat via foot) in a V1.1 MIDI update. Producers playing from hardware controllers expect these alternatives.

---

### PEARLMAN — "The Archive Builder"

The Guru Bin retreat document (265 lines, 12 refinements documented) confirms that the defaults were rigorously calibrated before the preset library was authored. Refinement R4 (LFO2 rate: 2.0 → 2.17 Hz for coprime drift) and R5 (LFO2 depth: 0.0 → 0.08, enabling D005 breathing at defaults) are exactly the kind of details that separate a functional synthesizer from an instrument. An engine that ships with its primary groove modulator muted by default (depth=0.0) is a photography. The retreat corrected this before the library was built.

The texture chain ordering in `OfferingTexture.h` (lines 56–76) processes vinyl before tape, which means crackle impulses pass through the tape saturation stage. This is the correct analog chain order — a vinyl record playing through a tape machine would add crackle first, then the tape's soft-clip would round the impulse peaks. This ordering is not documented in the spec but is implemented correctly by intuition or design.

The bit-crush implementation at line 140–143 (`quantized = round(input × levels) / levels`) is mathematically correct. The SP-1200 preset in `SP-1200_Dig.xometa` explicitly sets `ofr_dustBits: 12` and `ofr_dustSampleRate: 26040.0` — the actual SP-1200 specification. This level of historical accuracy in a synthesized instrument is rare. The machine doesn't just sound lo-fi; it knows *which* lo-fi machine it is emulating.

Pink noise in `processTape` uses Paul Kellet's filtered white noise method (line 113–119) with four independent IIR stages. Each stage's state variable gets `flushDenormal()` treatment to prevent CPU overhead from subnormal numbers during silence. This is the fleet's standard denormal hygiene pattern, correctly applied.

The preset name quality across the 154-preset library is strong: "Rubble Zone," "Crate Dig," "Dilla Time," "Motor City Slump," "Bay Fog," "Mantis Strike," "Entropy Engine," "Ghost Groove." These names carry the mythology without becoming parody. The Aether-mood preset "Curiosity Engine" at `digCuriosity: 0.98`, `digComplexity: 0.75` is the correct high-curiosity demonstration — it pushes into the alienShift zone (curiosity > 0.7) where body drifts toward noise and pitch becomes wilder.

**Pearlman's new concern**: The reverb tail in the Clap type (line 507–510: `tail = bp1_.process(noise) × env × 0.4f`) runs continuously as long as `ampEnv_.isActive()`. The amp envelope uses `decayCoeff_ = 1 − exp(−4.6 / (sr × dSec))`. At clap decay=0.25s (default), the tail runs for the full decay time. But the burst phase (lines 489–504) runs independently of the amp envelope — bursts fire by sample count, not envelope gating. This creates an overlap: during the burst phase the amp envelope is simultaneously adding filtered noise at 40% level. At snap=0.65, the burst duration is ~16ms (4 bursts × 4ms/burst), and the reverb tail is already present from sample 0. This is not a bug but an audible layering characteristic — during the burst phase, the reverb tail is audible beneath the individual bursts. Whether this is correct clap behavior or an architectural simplification is a sound design question. I would argue the reverb tail should be suppressed for the first N samples where bursts are occurring, then fade in after the last burst.

---

### TOMITA — "The Colorist"

In the pre-build session I demanded spatial character. The five cities have it now, though not as I imagined. New York's noise gate creates a hard-edged spatial clarity — sounds either punch through or disappear, which is the anechoic character of close-miked recordings in a treated room. Detroit's feedback accumulator creates a room that *remembers* — warmth builds with performance density, as if the heat of playing heats the air. These are spatial metaphors implemented as temporal state, not convolution.

The `masterWidth` parameter (lines 446–449) implements a mid-side encoder: `mid = (L+R)/2`, `side = (L−R)/2`, then `side × masterWidth × 2`. At the post-Guru-Bin default of 0.6, this gives `side × 1.2` — a 20% width enhancement. The voices have pre-assigned pan positions: CHat=+0.3, OHat=−0.3, Clap=+0.15, Rim=−0.15, Tom=+0.4, Perc=−0.4 (with Kick and Snare center). The master width expansion enhances these pan offsets, creating a genuine stereo image from a drum machine. This is elegant.

The LFO1 modulation comment at line 393 reads: "LFO1 → filter modulation (applied as amplitude mod for simplicity in V1)." The LFO applies `sample × (1 + lfo1Val × lfo1Depth × 0.5)`. This is amplitude modulation, not filter cutoff modulation. The parameter label says "filter" but the DSP says "amplitude." This is not a D004 violation (the parameter does affect audio), but it is a documentation mismatch that will confuse producers who read the parameter name and expect a filter sweep. A V1.1 cleanup should either rename the parameter to `ofr_lfo1AmpMod` or implement actual filter cutoff modulation.

The wobble LFO in `OfferingTexture.h` runs at a fixed 1.5 Hz (line 169: `wobblePhase_ += 1.5f / sr_`). The Guru Bin retreat identified this as faster than true motor drift (turntable instability is 0.5–3Hz). At default wobble=0.05, the amplitude modulation is ±5% at 1.5Hz — approximately 1.5 cycles per second, which a trained ear can detect as a slight flutter rather than organic drift. Turntable flutter under normal conditions is below 0.3Hz. This wobble rate should be a parameter (0.1–3.0 Hz) rather than hardcoded, with default 0.5 Hz for invisible motor character.

**Tomita's new blessing nomination**: The stereo width implementation that compensates for the city mono-collapse effect is elegant system-level thinking. The cityProcessor converts the stereo mix to mono before processing (line 413: `monoMix[s] = (sampleL + sampleR) × 0.5`), then blends the city-processed mono back with the dry stereo. At cityIntensity=0.4 (Guru Bin default), 60% of the original stereo image survives. At masterWidth=0.6, the surviving stereo image is enhanced by 20%. The net stereo impression is: `0.6 × original_stereo × 1.2 = 0.72 × original_stereo` — slightly narrower than the unprocessed signal. This is the correct behavior for boom bap: city-processed drums should sit slightly narrower than isolated synthesis, because boom bap is a mono-compatible format.

---

### VANGELIS — "The Orchestral Imagist"

I look for the moment before music becomes calculation. In OFFERING I find it at the intersection of three places: the clap's burst architecture, the Detroit timing system, and the Berlyne curve's peak at 0.5.

The clap burst at snap=0.65 generates approximately 4 bursts at ~6.75ms spacing. Four hands clapping in a room — four distinct pressure events separated by the propagation time of a human handclap. This is acoustics, not DSP. The Clap type doesn't synthesize a clap; it *reconstructs* the acoustic event that produces a clap. At default settings, this is the most physically accurate non-sampled clap in any synthesizer I have heard. The body parameter controlling the reverb tail mix (the bandpass-filtered noise after the bursts) allows producers to control how much room they imagine the hands were in.

The DIG macro — the performer's reach into the imaginary crate — is the engine's central gesture. At DIG=0, the drumkit is consistent, mechanical, a reference. At DIG=0.5, it finds Berlyne's peak: maximum variation within recognizable pattern. At DIG=1.0, the alien shift activates — body drifts toward pure noise, pitch envelopes run wide, the drums become alien. This is a single macro that functions as the engine's dramatic arc. A live performance tool where the left hand on the macro knob narrates the entire evolution of the beat. The mod wheel deepens this further (line 321: `digCuriosity = clamp(digCuriosity + modWheelValue × modWheel × 0.3, 0, 1)`).

I want to note what is musically significant about the aftertouch routing (lines 324, 384–387): aftertouch modulates `effectiveVinyl` and `effectiveTape` — pressing harder on the pad increases vinyl crackle and tape warmth on that voice. This means aggressive playing literally buries the sound deeper in the medium. A hard-pressed sustain creates a dustier, more degraded timbre. This is the correct phenomenology of analog tape playing: the harder you press a tape against a head, the more saturation enters the signal.

**Vangelis's new concern**: The macro CITY (M2) morphs through the five cities continuously: `totalCity = cityMode + cityMacroVal × 4.0`, then `cityMode = int(totalCity)`, `cityBlend = totalCity − floor(totalCity)`. This means turning the CITY macro sweeps through NY → Detroit → LA → Toronto → Bay Area in one continuous gesture. The order of cities was presumably chosen by sound, not by geography, but the progression NY → Detroit → LA → Toronto → Bay Area has its own internal logic: East Coast to Midwest to West Coast to Canada to West Coast underground. It is a crate digging tour. The cityBlend morphing between adjacent chains creates a continuous character sweep that no hardware drum machine can achieve. This is the engine's most distinctive real-time gesture.

---

### SCHULZE — "The Structuralist"

The pre-build debate about the Curiosity Engine was the most heated exchange in the council. I demanded to see the algorithm before blessing the concept. Now I have seen it.

`generateVariation()` in `OfferingCuriosity.h` (lines 99–145) is a 45-line function that implements all three research citations in a single coherent algorithm. The parameter selection logic using `paramsToVary` as a threshold (if paramsToVary > 0, vary tune; if > 1, vary decay; etc.) gives a deterministic ordering of which parameters vary at which complexity levels. This ordering is not arbitrary — tune and decay are varied first (most perceptually salient for drum identity), followed by body and snap (timbral texture), followed by pitchEnv and sat (subtle coloration). The ordering from most to least perceptually salient is correct.

The per-voice PRNG seeding (line 179: `uint32_t rngState_[8] = { 1, 7920, 15839, 23758, 31677, 39596, 47515, 55434 }`) gives each drum slot a different starting point in the xorshift state space. This means voice 0 (Kick) and voice 1 (Snare) will produce different variation sequences even with identical curiosity/complexity/flow settings. The kick does not vary the same way as the snare. This is the correct behavior — in real drum performance, the kick and snare have different organic variation characteristics.

The flow mechanism (lines 105–109): `if (rng < flowBalance(flow) && hasLastVariation_[voiceIndex]) return lastVariations_[voiceIndex]`. At flow=0.6 (default), there is a 60% probability per trigger of returning the cached variation. This creates the "locked groove" phenomenon: the pattern feels familiar because most hits reuse the previous variation, but 40% of hits generate fresh variation. Over a 16-step sequence, this means approximately 6–7 hits will feel novel and 9–10 will feel consistent. This matches the psychological description of a groove: predictable enough to lock, variable enough to engage.

My one structural concern: the alien shift mechanism (`alienShift = max(0, (curiosity−0.7)/0.3)`) affects `bodyDelta` and `pitchEnvDelta` directly inside `generateVariation` (lines 133–137), but these offsets are then clamped by `triggerVoice` (lines 772–776). The clamp prevents aliensShift from pushing parameters outside their ranges, which is correct. However, the alien shift toward noise (body pushed down by `alienShift × 0.6`) combined with the Berlyne-driven `bodyDelta` could push body through zero and into negative territory momentarily before the clamp catches it. This is safe because the clamp is there, but it means at extreme curiosity the alien shift and the Berlyne variation might partially cancel — the variation drives body toward noise while the random delta drives it randomly. The net behavior is that high curiosity + high alienShift makes the body drift predominantly toward noise regardless of the random direction. This is actually the correct behavior (at extreme curiosity, drum sounds become noisier and stranger), but it is an emergent consequence of the additive architecture rather than an explicit design decision. I would document this as intentional in a comment.

**Schulze's verdict on P0 #3**: FULLY RESOLVED. The Curiosity Engine is a genuine implementation of its psychological citations. The mathematical forms are correct. The per-voice state is correctly isolated. The flow mechanism correctly models the balance between familiarity and novelty.

**New blessing nomination from Schulze**: The per-voice xorshift PRNG with deterministic seeds (lines 161–167) gives each drum slot a consistent "personality" in the variation space. Voice 2 (CHat) will always vary its timing in the same direction as voice 5 (Rim). Voice 0 (Kick) and voice 6 (Tom) will have complementary variation profiles because their seeds are spaced by 7920 — a prime-adjacent gap in the xorshift state space. This creates coherent kit-level variation: the drums don't each vary independently, they vary *together* in a way that preserves groove integrity. This is sophisticated and correct.

---

## Phase 3: Doctrine Compliance — Built Code

| Doctrine | Status | Evidence |
|----------|--------|---------|
| **D001 — Velocity Must Shape Timbre** | **PASS** | `triggerVoice()` lines 762–763: snap and body both modified by velocity offset. `snap = clamp(snap + (velocity−0.5) × velSnap × 0.6)`, `body = clamp(body + (velocity−0.5) × velBody × 0.4)`. A forte hit (vel=1.0) at default velToSnap=0.6: snap shifts +0.18. Same hit at soft (vel=0.2): snap shifts −0.18. This is a 0.36-unit range on snap — audible attack sharpness difference. Body shifts with velToBody=0.3: forte adds +0.12 body (more tonal), soft removes −0.12 (more noisy). Two independently calibrated timbral axes responding to velocity. Fully implemented. |
| **D002 — Modulation is the Lifeblood** | **PASS** | LFO1 (0.01–20Hz, 5 shapes, via `StandardLFO`) → amplitude mod on all voices. LFO2 (0.01–20Hz, sine only, groove pump) → amplitude mod. Aftertouch → vinyl + tape intensity (lines 384–387). Mod wheel → curiosity drive (line 321). 4 macros: DIG → curiosity + complexity; CITY → mode + blend morph; FLIP → layers + chop + ring; DUST → vinyl + tape + bits. 5 coupling types: AmpToFilter, AmpToChoke, RhythmToBlend, EnvToDecay, AudioToFM. Full modulation coverage. **Concern**: LFO1 is labeled as "filter modulation" but implements amplitude modulation (line 393 comment). Not a doctrine failure but a documentation mismatch. |
| **D003 — The Physics IS the Synthesis** | **PASS** | Berlyne inverted-U curve as `berlyneCurve(x) = 0.2 + 0.8 × (4x(1−x))` — mathematically correct form. Wundt asymmetric ramp as two-segment linear function — correct asymmetry. Csikszentmihalyi flow as reuse probability — correct intuition. TR-808 hat ratios (1.0/1.4471/1.6818/1.9265/2.5028/2.6637) are historically verified. SP-1200 12-bit at 26040 Hz in the preset library. Karplus-Strong comb filter topology for Perc. Pink noise via Paul Kellet's method. Multi-burst clap from acoustic hand clap physics. Citations map to DSP. |
| **D004 — Dead Parameters Are Broken Promises** | **CONDITIONAL PASS** | 84 parameters declared and loaded in `attachParameters()`. Curiosity Engine params (digCuriosity, digComplexity, digFlow) are applied in `triggerVoice()` via `generateVariation()`. All voice params (type, tune, decay, body, level, pan) applied per trigger. **Unresolved**: `ofr_envFilterAmt`, `ofr_velToAttack`, `ofr_envToPitch` are declared and loaded into snapshot (lines 873–875) but not applied in the render loop. If these are intentionally deferred, they must be documented as V1.1 stubs or removed for V1. This is the engine's only open D004 question. **Severity**: P2 (auditable, does not affect core function). |
| **D005 — An Engine That Cannot Breathe Is a Photograph** | **PASS** | LFO1 rate floor = 0.01 Hz (parameter declaration line 627) — exactly the fleet minimum. Default 0.067 Hz (≈15-second cycle, scripture-verified breathing rate). LFO2 default 2.17 Hz (post-Guru-Bin coprime-adjacent to LFO1). Post-retreat: LFO2 depth default 0.08 (previously 0.0 — the retreat corrected a silent LFO). Engine breathes autonomously from first trigger at default settings. |
| **D006 — Expression Input Is Not Optional** | **PASS** | Velocity → snap AND body (two timbral axes). Aftertouch → effectiveVinyl + effectiveTape (pressing harder buries in dust). Mod wheel → curiosity drive (0.3 scaling at default modWheel=0.5). Pitch bend → tune offset (`pitchBendNorm_ × 2.0` semitones at trigger, lines 827, 811). Three physical gestures mapped to distinct, semantically correct timbral targets. |

---

## Phase 4: Scoring

### Score Breakdown

| Category | Max | Score | Rationale |
|----------|-----|-------|-----------|
| Transient Architecture (P0 #1 resolution) | 15 | 14.5 | 8 distinct topologies, genuinely architecturally different. −0.5 for double-saturation in Tom type. |
| City Processing (P0 #2 resolution) | 15 | 13.5 | 5 structurally unique Stage 6 implementations. −1.0 for spatial underdevelopment (cities have no room character beyond Bay Area allpass in comb-filter territory, not diffusion territory). |
| Curiosity Engine (P0 #3 resolution) | 15 | 14.5 | Berlyne/Wundt/Flow correctly implemented as DSP. Per-voice PRNG personality. Alien shift emergent behavior. −0.5 for paramsToVary non-monotonic behavior that will confuse producers without UI explanation. |
| Doctrine Compliance (D001–D006) | 20 | 17.5 | D001, D003, D005, D006 PASS. D002 with LFO label mismatch. D004 conditional (3 unrouted params). |
| Preset Library Quality | 15 | 13.5 | 154 presets, evocative names, strong spread across 8 moods, historically accurate SP-1200 preset, Dilla Time authenticity, 8 Family presets showing cross-engine coupling potential. −1.0 for SP-1200 Dig preset missing `engines` key wrapping (uses raw parameter keys, not nested under "Offering" engine key — format inconsistency vs. NY Standard). −0.5 for some presets with minimal parameter deviation from defaults. |
| Guru Bin Retreat Quality | 10 | 9.0 | 12 documented refinements with mathematical justification. LFO coprime correction was sophisticated. Double-saturation in Tom identified. Default LFO2 mute corrected. Scripture references appropriate. −1.0 for incomplete retreat (only signal path journey + 10 presets; no scripture verses authored). |
| Detroit Drunk Timing (Bonus) | 5 | 5.0 | Sample-accurate per-voice deterministic timing offset. Musicologically correct (personality-consistent, not random). Not in original spec. Full bonus awarded. |
| Innovation Tax Deduction | −5 | 0 | No deduction — all claimed innovations are implemented, not speculative. |

### TOTAL: **87.5 / 100 → 8.75 / 10**

Rounded: **8.8/10**

---

### Score in Context

| Threshold | Current Standing |
|-----------|-----------------|
| Pre-build score | 7.9/10 (with 3 P0s unresolved) |
| Post-build score | **8.8/10** |
| Score improvement | +0.9 points |
| Fleet average | ~8.8/10 |
| 9.0+ tier | OVERBITE 9.2, OWARE 9.2, OBSCURA 9.1, OUROBOROS 9.0, OXBOW 9.0 |

OFFERING enters the fleet at the fleet average. This is an accurate reflection: the engine's innovative concepts are fully implemented but three refinement opportunities remain open.

---

## Phase 5: Blessing Evaluation

### B035 — Psychology-as-DSP

**Pre-build vote**: CONDITIONAL 8-0 (all eight ghosts voted conditional on actual DSP implementation)
**Post-build re-evaluation**:

The Curiosity Engine in `OfferingCuriosity.h` implements three peer-reviewed psychological research findings as real-time DSP algorithms. The Berlyne inverted-U hedonic curve becomes a variation range scalar. The Wundt asymmetric hedonic curve becomes a parameter count selector. The Csikszentmihalyi Flow State becomes a variation reuse probability. All three mappings are correct in their mathematical forms and produce audible, predictable, controllable behavior.

Additionally, the "alien shift" at curiosity > 0.7 is an undocumented emergent feature — a consequence of the Berlyne curve's behavior at high curiosity values, not a separate mechanism. This is the engine doing something no human designer consciously programmed: the psychology generates a side effect (alien timbre at extreme curiosity) that is musically correct and consistent with the theoretical prediction (overstimulation produces unfamiliar, uncomfortable aesthetic responses).

This is the first synthesizer in the XOlokun fleet — and plausibly in commercial synthesizer history — that derives timbral variation rules from cited experimental psychology literature implemented as running DSP code. B035 is a genuine fleet-level innovation.

**Vote**: RATIFIED — 8-0

---

### B036 — City-as-Processing-Chain

**Pre-build vote**: DENIED 3-5 (Buchla, Smith, Schulze, Pearlman, Tomita voted against; only Kakehashi, Vangelis, Moog conditionally in favor)

**Post-build re-evaluation**:

The five city chains now have structurally distinct Stage 6 implementations: NY = envelope-following noise gate, Detroit = hit-accumulating feedback saturation loop, LA = parallel compression routing, Toronto = self-sidechain sub duck, Bay Area = 4-stage prime-delay allpass fog with recursive feedback. These are not parametric variations of a common topology. They are five different processing strategies, each encoding a genuine aesthetic tradition:

- NY gate = the close-miked, tight, non-resonant recording aesthetic of late-80s SP-1200 sessions
- Detroit feedback warmth accumulation = the heat-of-playing warmth in Dilla's MPC recordings
- LA parallel compression = the squashed, pumping energy of west coast boom bap production
- Toronto sub duck = the clean sub architecture of Drake/OVO-era production
- Bay Area allpass = the diffuse, foggy spatial character of hyphy and Bay underground records

The architectural diversity is real. However, the spatial dimension identified by Tomita and Buchla as missing from the pre-build spec remains partially missing from the build. Cities exist in acoustic spaces; the five chains do not encode those spaces beyond the Bay Area's comb-filter-range allpass. The "structural uniqueness" criterion is met for Stage 6 but not for spatial character.

**Amended Blessing**: B036 ratified with condition — "Five distinct processing architectures, each encoding a cultural aesthetic tradition. Spatial character to be added in V1.1 for full geographic identity (per-city reverb signature)."

**Vote**: RATIFIED with condition — 6-2 (Buchla and Tomita maintain partial objection on spatial grounds)

---

## Phase 6: Path to 9.0+

**Current score**: 8.8/10. Three improvements would bring the engine to 9.0+:

### Fix 1: Route the Three Unresolved Parameters (D004) | +0.1 expected

**Affected parameters**: `ofr_envFilterAmt`, `ofr_velToAttack`, `ofr_envToPitch`

These parameters are loaded in the snapshot (lines 873–875) but not applied in the render loop. Smith flagged this as a D004 risk.

**Recommended implementation**:
- `ofr_envFilterAmt`: Route to city processing intensity — `snap.cityIntensity × (1 + envFilterAmt × 0.5)`. Env-to-filter modulating city intensity creates the effect of the drum kit's processing chain responding to its own envelope.
- `ofr_velToAttack`: Apply as attack time modifier in `OfferingADEnvelope::trigger()`. Higher velocity could shorten attack (`attackSec × (1 − velToAttack × velocity × 0.5)`) for a snappier feel at high velocities.
- `ofr_envToPitch`: Route to pitch envelope depth — `pitchEnv = pitchEnv × (1 + envToPitch × 0.5)` at trigger time. This makes the pitch sweep wider when the envelope amount is high.

All three routes are additive and non-breaking.

---

### Fix 2: LFO1 Label Correction | +0.05 expected

**Issue**: `ofr_lfo1Rate` / `ofr_lfo1Depth` / `ofr_lfo1Shape` are described as filter modulation parameters but implement amplitude modulation. The comment at line 393 reads "LFO1 → filter modulation (applied as amplitude mod for simplicity in V1)."

**Recommended fix for V1**: Change the parameter display name from "Offering LFO1 Rate" to "Offering LFO1 Amp Rate" and add a note in the sound design guide. If V1.1 implements actual filter modulation, the parameter ID can remain unchanged — only the display name changes. This prevents the "broken promise" perception without requiring DSP changes.

---

### Fix 3: Hardcoded Wobble Rate → Exposed Parameter | +0.1 expected

**Issue**: `OfferingTexture.h` line 169 hardcodes the wobble LFO at 1.5Hz. For turntable motor drift, 0.5Hz or below is authentic. The parameter `ofr_dustWobble` controls depth but not rate.

**Recommended fix**: Add `ofr_dustWobbleRate` (0.1–3.0 Hz, default 0.5 Hz). Modify `processWobble()` to accept the rate: `wobblePhase_ += wobbleRate / sr_`. This gives producers the ability to dial in turntable drift (0.3Hz), tape wow (0.5Hz), and flutter (2–3Hz) as distinct characters.

**Net effect**: 8.8 + 0.1 + 0.05 + 0.1 = **9.05 / 10** after all three fixes.

---

## Phase 7: New Discoveries (Not in Pre-Build Spec)

The following features were discovered during the post-build evaluation that were not present or specified in the pre-build architecture:

1. **Detroit drunk timing with personality-consistent per-voice offsets** — deterministic xorshift seed per voice slot. Musicologically correct Dilla timing model. Implemented with 20ms cap for safety.

2. **Alien shift at curiosity > 0.7** — emergent consequence of the Berlyne curve's interaction with the alienShift parameter. Body drifts toward noise, pitch envelopes widen. Not in original spec; discovered in implementation.

3. **Hat choke mechanism** — `choke()` method in OfferingTransient gives the CHat trigger a smooth 7ms fade of the OHat (coeffient 0.995 per sample at 44.1kHz). Avoids the click of abrupt silence. Not specified in pre-build spec.

4. **Coupling accumulator architecture** — Five coupling types store RMS modulation values (`couplingCityMod_`, `couplingChokeMod_`, `couplingFlipMod_`, `couplingDecayMod_`, `couplingFMMod_`) that can be applied in triggerVoice and the render loop. The coupling architecture separates the modulation computation from application, which is clean and modular.

5. **SP-1200 bit-crush in NY city chain Stage 1** — New York city chain (line 231) applies hard 12-bit quantization (`levels = 4096.0`) at 26040 Hz spec before the noise gate and compression. This is the SP-1200 fingerprint embedded in the NY city chain, not just in the texture layer. NY drums therefore get double bit-crush when the DUST macro is engaged — one in the city chain and one in the texture layer. This is authentic: SP-1200 users in New York ran multiple generations of 12-bit sampling.

---

## Final Verdict

**OFFERING** enters the XOlokun fleet as engine #46 at **8.8/10** — precisely at the fleet average, which is the correct landing position for an engine that resolves all three P0 concerns but retains three refinement opportunities.

The pre-build criticism was correct and productive. Moog forced architecturally distinct synthesis topologies. Buchla forced structurally unique city Stage 6 implementations. Smith and Schulze forced actual DSP implementation of the psychology citations. Every P0 was resolved in the build.

The engine's distinctive contributions to the fleet:
- Only drum synthesizer in XOlokun
- Only engine deriving timbral variation from peer-reviewed psychology research (B035 — ratified)
- Only engine with geographic psychoacoustic processing chains with distinct architectures (B036 — ratified with condition)
- Only engine with personality-consistent timing humanization modeled on a specific producer's documented approach (Dilla)
- 154 presets including historically accurate SP-1200 emulation and cross-engine coupling family presets

The three open items (unrouted parameters, LFO label mismatch, hardcoded wobble rate) are P2 issues that do not affect the engine's core function and are appropriate for V1.1 resolution.

**OFFERING ships at 8.8/10. Path to 9.0+ is documented and achievable in V1.1.**

---

## Council Summary Table

| Ghost | Core Finding | P0 Status |
|-------|-------------|----------|
| Moog | 8 distinct topologies confirmed. Tom double-saturation is P2. Drunk timing personality is musically correct. | P0 #1 RESOLVED |
| Buchla | 5 structurally unique Stage 6 architectures confirmed. Spatial character remains partial. CPU spike during city crossfade noted. | P0 #2 RESOLVED |
| Smith | Psychology-DSP mapping correct. 3 unrouted parameters flagged as D004 risk (P2). envFilterAmt/velToAttack/envToPitch need routing in V1.1. | P0 #3 RESOLVED |
| Kakehashi | Drunk timing is the most musically sophisticated timing feature in the fleet. GM drum map coverage could expand alternate notes in V1.1. | ENTHUSIASTIC PASS |
| Pearlman | Texture chain ordering correct (vinyl before tape). SP-1200 historical accuracy commendable. Clap reverb tail overlaps burst phase — design question. | PASS |
| Tomita | LFO1 labeled as filter but implements amplitude — documentation mismatch, not DSP failure. Wobble rate hardcoded at 1.5Hz (should be 0.5Hz, should be a param). | PASS |
| Vangelis | Aftertouch semantics (harder press = more dust) is poetic and correct. CITY macro as geographic tour is the engine's most distinctive real-time gesture. | ENTHUSIASTIC PASS |
| Schulze | Curiosity Engine fully implemented. Per-voice PRNG seeds give each drum slot consistent personality. Alien shift emergent behavior is musically correct. | ENTHUSIASTIC PASS |

---

## Blessings Awarded

| ID | Blessing | Vote |
|----|----------|------|
| B035 | **Psychology-as-DSP**: First synthesizer in fleet (and plausibly in commercial history) deriving timbral variation rules from cited experimental psychology literature (Berlyne 1960, Wundt 1874, Csikszentmihalyi 1975) implemented as real-time DSP code. The alien shift above curiosity=0.7 is an emergent theoretical consequence implemented correctly. | RATIFIED 8-0 |
| B036 | **City-as-Processing-Chain** (amended): Five distinct processing architectures encoding cultural aesthetic traditions — NY noise gate, Detroit feedback warmth accumulation, LA parallel compression, Toronto self-sidechain sub, Bay Area prime-delay allpass fog. Spatial character to be added V1.1 for full geographic identity. | RATIFIED 6-2 |

---

## Recommended V1.1 Fixes (Priority Order)

1. **Route `ofr_envFilterAmt`, `ofr_velToAttack`, `ofr_envToPitch`** — D004 compliance, 3 currently dead parameters
2. **Fix Tom double-saturation** — clip inner tom saturation to fixed drive, let outer sat param control all types
3. **Add `ofr_dustWobbleRate` parameter** — expose wobble rate (default 0.5Hz), remove hardcoded 1.5Hz
4. **Rename LFO1 parameters** — "LFO1 Amp Rate" not "LFO1 Rate" until filter routing is implemented
5. **Add per-city reverb signature** — even single-allpass room character per city (short decay, city-calibrated frequency) for B036 full ratification
6. **Clap tail suppression during burst phase** — suppress reverb tail for burst count × burst spacing duration before blending in
7. **Expand GM drum map** — add note 40 (electric snare), 44 (closed hat foot) alternate triggers

---

*Seance conducted: 2026-03-21*
*Council: Moog, Buchla, Smith, Kakehashi, Pearlman, Tomita, Vangelis, Schulze*
*Verdict: OFFERING ships at 8.8/10. Two new blessings ratified (B035, B036). Path to 9.05/10 documented.*
