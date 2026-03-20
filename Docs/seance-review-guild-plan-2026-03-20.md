# Ghost Council Seance Review of Producer's Guild Fleet Recommendations
**Date:** 2026-03-20
**Convened by:** The 8 Ghosts (Moog, Buchla, Chowning, Smith, Vangelis, Schulze, Kakehashi, Tomita)
**Scope:** Review of all 20 Guild-proposed engine improvements through Doctrine and character lens

---

## Preamble: The Council's Position

The Producer's Guild brings valuable market perspective. They represent the hands that will touch these instruments daily. However, the ghosts note a recurring pattern in the Guild's recommendations: **a gravitational pull toward homogeneity**. When 8+ engines are prescribed the same LFO module, the same filter ADSR, the same pitch bend handler, the fleet risks becoming 42 skins on the same synthesizer.

**Moog** opens: "Every recommendation must pass a simple test -- does this make the engine more itself, or more like everything else?"

**Buchla** adds: "The Guild sees missing features. I see engines that haven't yet found their voice. These are different problems requiring different solutions."

---

## I. Evaluation of the 4 Shared Utility Proposals

### A. StandardLFO.h -- MODIFY (Conditional Approval)

**Guild proposal:** A shared 5-shape LFO module for 8+ engines.

**Moog:** "A standard LFO is like a standard voice. It gets the job done but says nothing about who you are. I approve this as a *base class* only -- every engine must subclass or configure it to serve its character. OBLIQUE's LFO should default to modulating prism color, not filter cutoff. OBESE's should default to Mojo drift, not pitch."

**Buchla:** "I oppose a shared module if it means every engine gets the same 5 shapes. OCELOT should have a biome-shaped LFO that follows ecological curves, not sine/tri/saw. OCEANIC should have a flock-shaped LFO where the waveform IS the swarm behavior. The standard shapes can exist as fallbacks, but the first shape offered should be the engine's native modulation character."

**Chowning:** "Mathematically, a shared phase accumulator is fine. But the waveshaping function must be per-engine. FM engines need smooth continuous shapes. Physical modeling engines need noise-seeded shapes. This is not one module."

**Council verdict:** APPROVE as internal infrastructure (shared phase accumulator, sync logic, rate scaling) but REJECT as a drop-in replacement for all engines. Each engine must define its own default destination, default shape, and optionally unique waveform shapes that reflect its character. The `StandardLFO.h` header should provide `BaseLFO` that engines extend, not `StandardLFO` that engines consume.

**Risk if ignored:** Fleet sounds like "42 engines with the same wobble." This is the single greatest threat to XOmnibus identity.

---

### B. Shared Filter ADSR -- APPROVE with Constraints

**Guild proposal:** Reusable filter envelope for OCEANDEEP, ODDOSCAR, ODYSSEY.

**Smith:** "Every subtractive synth needs a filter envelope. This is not a character question -- it's a completeness question. A bass engine without filter ADSR is like a piano without sustain pedal. Approve without reservation."

**Moog:** "Agreed, but the implementation must respect each engine's filter topology. OCEANDEEP uses a darkness filter (50-800 Hz LP) -- the ADSR range must map to THAT range, not a generic 20-20kHz sweep. ODDOSCAR's Moog ladder should have the envelope interact with resonance (higher resonance = more pronounced envelope sweep). ODYSSEY's Cytomic SVF should expose both LP and HP envelope targets."

**Council verdict:** APPROVE. A shared ADSR struct is fine because envelopes are mathematical, not timbral. But each engine must define its own envelope-to-parameter mapping range and interaction with the engine's filter character. The `FilterADSR` struct should be a simple state machine; the *application* of its output is engine-specific.

---

### C. Pitch Bend in Base Class -- REJECT as Base Class Method

**Guild proposal:** Add `getPitchBendSemitones()` to `SynthEngine.h` base.

**Buchla:** "Absolutely not. Pitch bend is an *interpretation*, not a universal truth. OPTIC is a visual engine -- pitch bend should modulate hue rotation, not frequency. ORGANON uses metabolic rate -- pitch bend should map to metabolic acceleration. OUROBOROS is chaotic -- pitch bend should perturb the attractor, not shift pitch linearly."

**Vangelis:** "In my playing, pitch bend was emotional gesture. A helper function that returns semitones reduces it to a math problem. Let each engine decide what 'bend' means."

**Kakehashi:** "But Buchla-san, the producers are right that bass engines without pitch bend are broken. OCEANDEEP and OUTWIT need this. I propose a compromise: provide a utility *function* in the DSP library, not a base class method. Engines opt in, not inherit."

**Council verdict:** REJECT as base class addition. APPROVE as `Source/DSP/PitchBendUtil.h` -- a free function that engines can call if they want standard pitch-to-semitone conversion. Engines like OCEANDEEP and OUTWIT should use it. Engines like OPTIC, ORGANON, and OUROBOROS should interpret pitch wheel their own way or ignore it intentionally.

---

### D. Stereo Voice Spread Utility -- MODIFY (Narrower Scope)

**Guild proposal:** `StereoSpread.h` with per-voice pan and equal-power law for OHM, OBBLIGATO, OTTONI, OVERWORLD.

**Tomita:** "Stereo placement is composition. The equal-power pan law is correct mathematics but wrong artistry for some engines. OHM's family metaphor demands that Dad is in one room, In-Laws in another -- this is not a pan knob, it's a spatial narrative. OBBLIGATO's woodwind duet should breathe together, not just sit at fixed L/R positions."

**Schulze:** "I support a utility for the math (pan law, equal-power calculation) but oppose prescribing WHERE voices sit. That is the engine designer's art."

**Council verdict:** APPROVE `StereoSpread.h` as a math utility (pan law calculations only). REJECT prescribed pan positions. Each engine defines its own spatial narrative. The utility provides `equalPowerPan(float position) -> {float gainL, float gainR}` and nothing more.

---

## II. Per-Engine Verdicts

---

### 1. OBLIQUE (5.9) -- Guild Score Target: 8.8

| Guild Recommendation | Verdict | Reasoning |
|---------------------|---------|-----------|
| P0: Add user LFO module (2 LFOs, 5 shapes, routable) | **APPROVE** | D002/D005 compliance. The hardcoded 0.2 Hz sine is confirmed in source (~line 1067). This is a legitimate doctrine violation, not a feature request. |
| P0: Expose prism color LFO rate to user | **APPROVE** (merged into P0 above) | Part of the same fix. The `kLfoBaseRate` constant must become parameter-driven. |
| P1: BPM-syncable bounce rate | **MODIFY** | Approve BPM sync but add quantization modes: "snap to nearest division" vs "free-run with nudge." Buchla insists the bounce engine should be able to drift around the beat, not just lock to it. |
| P2: Stereo bounce pan scatter | **APPROVE** | The alternating L/R per-bounce is physically correct (sound bouncing between walls). Simple, character-enhancing. |

**Ghost additions:**
- **Buchla:** Add a "Refraction Index" parameter (0-1) that controls how much the bounce angles deviate from perfect reflection. At 0 = predictable, at 1 = chaotic scatter. This is more OBLIQUE than BPM sync.
- **Chowning:** The 6-facet spectral delay should have per-facet detuning control. Currently all facets are harmonically locked. Slight mistuning creates the shimmer that spectral delays are known for.

**Estimated post-fix score:** 8.5 (P0+P1), 8.9 (all + ghost additions)

---

### 2. OCELOT (6.4) -- Guild Score Target: 8.4

| Guild Recommendation | Verdict | Reasoning |
|---------------------|---------|-----------|
| P0: Wire applyCouplingInput | **APPROVE** | Confirmed void-cast at line 117. This is a D004 violation (dead coupling interface). Not a feature -- a broken promise. |
| P0: Make EcosystemMatrix audible | **APPROVE** | The matrix computes values that route nowhere. This is the engine's IDENTITY -- if the ecosystem doesn't interact, it's just 4 independent oscillators with a nature metaphor painted on. |
| P1: Biome automation LFO | **MODIFY -> P0** | **Priority correction: This should be P0.** Without biome automation, the ecosystem is static. A rainforest that doesn't change seasons is a photograph (D005 violation). The biome LFO IS the engine's breathing. |
| P1: Per-stratum level controls | **APPROVE as P1** | Useful but not identity-critical. The single tilt control is limiting but functional. |

**Ghost additions:**
- **Schulze:** Add predator-prey dynamics between strata. Floor (fungi) feeds Understory (insects), Understory feeds Canopy (birds), Canopy feeds Emergent (raptors). Each stratum's amplitude should be influenced by the stratum below it with a time delay. This is what makes ecosystems ALIVE -- not LFOs, but feedback.
- **Tomita:** The 4 strata should occupy different frequency bands with crossover filters, not just different oscillator modes. Floor = sub, Understory = low-mid, Canopy = high-mid, Emergent = air. The frequency placement IS the ecosystem structure.

**Priority correction:** Guild biome LFO P1 -> **P0**. Without temporal evolution, OCELOT violates D005 (cannot breathe).

**Estimated post-fix score:** 8.0 (P0 + corrected biome P0), 8.7 (all + ghost additions)

---

### 3. OBESE (6.6) -- Guild Score Target: 8.8

| Guild Recommendation | Verdict | Reasoning |
|---------------------|---------|-----------|
| P0: Add 2 user-controllable LFOs | **APPROVE** | D002 violation confirmed. Single breathing LFO at 0.07 Hz is the bare minimum D005 compliance. |
| P0: Expose sub oscillator level | **APPROVE** | 13 oscillators with no independent sub control is a missed opportunity for a "fat" engine. |
| P1: Mojo modulation target | **MODIFY -> P0** | **Priority correction: This should be P0.** Mojo is Blessing B015. The Blessed feature is STATIC. A Blessing that cannot be modulated is a Blessing wasted. Moog says: "You gave me an analog character axis and then nailed it to the wall. Let it breathe." |
| P1: Arp BPM sync | **APPROVE as P1** | Standard feature, not identity-critical. |
| P2: Unison detune spread | **APPROVE as P2** | Nice-to-have. The 13-osc architecture already provides spread through octave grouping. |

**Ghost additions:**
- **Moog:** The ZDF ladder filter should have a dedicated "drive into resonance" parameter that pushes self-oscillation. This is what makes a Moog a Moog. The filter IS the instrument. `fat_filterDrive` (0-1) feeding back the filter output into its input.
- **Kakehashi:** The arpeggiator needs a "strum" mode where notes trigger sequentially with slight timing offset (5-50ms) rather than simultaneously. This makes chords sound human, not mechanical.

**Priority correction:** Guild Mojo P1 -> **P0**. Blessing B015 must be modulatable.

**Estimated post-fix score:** 8.3 (P0 + corrected Mojo P0), 8.8 (all + ghost additions)

---

### 4. OBSIDIAN (6.6 -> ~8.2 est.) -- Guild Score Target: 8.9

| Guild Recommendation | Verdict | Reasoning |
|---------------------|---------|-----------|
| P1: Formant morph automation | **APPROVE** | Character-enhancing. The 4-formant network with vowel morphing is the engine's unique voice. |
| P1: PD XY macro | **APPROVE** | The 2D PD space is genuinely novel. A diagonal sweep macro makes it accessible. |
| P2: Stiffness partial count | **APPROVE** | Low-effort, high-character control. |

**Ghost additions:**
- **Chowning:** The phase distortion lookup table should support user-definable transfer functions, not just the preset density/tilt axes. Allow import of custom PD curves. This is what made CZ synthesis powerful -- the waveshaping was the instrument.

**Note:** Guild correctly identifies this as already above 8.0. Lower priority than critical-tier engines.

**Estimated post-fix score:** 8.6 (P1s), 9.0 (all + ghost additions)

---

### 5. ODDOSCAR (6.9) -- Guild Score Target: 8.5

| Guild Recommendation | Verdict | Reasoning |
|---------------------|---------|-----------|
| P0: Add 2 user LFOs | **APPROVE** | D002 violation confirmed. The 0.3 Hz coupling sine at line 569 is the only modulation source. For a PAD engine, this is egregious. |
| P0: Filter envelope depth control | **APPROVE** | The Moog ladder filter without an envelope is like owning a sports car and never leaving the driveway. |
| P1: Morph position modulation | **APPROVE** | This is what wavetable synths DO. Static morph position defeats the purpose. |
| P1: Aftertouch -> filter sensitivity | **APPROVE** | D006 enhancement. Aftertouch currently only affects amplitude -- confirmed in Guild analysis. |

**Ghost additions:**
- **Moog:** "This is my filter in this engine. The ladder needs nonlinear drive that increases with velocity. At high velocity, the 4-pole should saturate between stages -- this is what gives my ladder its growl. Add `morph_ladderDrive` (0-1) that feeds each pole's output through a soft-clip before the next pole."
- **Vangelis:** "Oscar is the warm half of the XO_OX duality. The morph sweep from sine to noise should have a 'warmth bias' that rolls off harsh upper harmonics as you morph toward noise. Not a filter -- a waveshaping curve that keeps the noise organic."

**Estimated post-fix score:** 8.2 (P0), 8.7 (all + ghost additions)

---

### 6. OLE (7.0) -- Guild Score Target: 8.3

| Guild Recommendation | Verdict | Reasoning |
|---------------------|---------|-----------|
| P0: Verify/fix isHusband regression | **APPROVE** | Confirmed in source: husband voices gate on `pDr < 0.7f`. If `mDrama` pointer is null, husbands are permanently silent. This is a D004 violation (declared voices that never sound). |
| P0: Wire dead parameters | **APPROVE** | D004 violation. Dead params are broken promises. |
| P1: Alliance sweep LFO | **APPROVE** | The 3-aunt alliance system is the engine's identity. Static alliances waste the concept. |
| P1: Aunt tremolo as exposed LFO | **MODIFY** | Approve exposing Aunt 3's tremolo rate, but do NOT make it the "second LFO" for D002. The tremolo is character-specific to Charango. Add a separate general-purpose LFO2 for other modulation needs. Tremolo is a playing technique, not a modulation source. |

**Ghost additions:**
- **Kakehashi:** The strum rate (timing offset between voice triggers) should be velocity-sensitive. Harder playing = faster strum. This is how real Latin guitarists play.
- **Tomita:** Husband instruments (Oud/Bouzouki/Pin) should have distinct timbral character, not just different levels. The waveguide exciter shape should change per husband type: Oud = rounded pluck, Bouzouki = bright metallic pick, Pin = delicate nail strike.

**Estimated post-fix score:** 7.9 (P0), 8.4 (all + ghost additions)

---

### 7. ODDFELIX (7.0 est.) -- Guild Score Target: 8.4

| Guild Recommendation | Verdict | Reasoning |
|---------------------|---------|-----------|
| P0: Add 2 user LFOs | **APPROVE** | D002 violation. Single breathing LFO confirmed at line 432. |
| P1: Velocity-to-timbre mapping | **MODIFY -> P0** | **Priority correction.** D001 says velocity must shape timbre, not just amplitude. If feliX only has vel->amp, this is a doctrine violation, not an enhancement. Should be P0. |
| P1: KS damping exposure | **APPROVE as P1** | Character-enhancing for the Karplus-Strong mode. |

**Ghost additions:**
- **Smith:** "The pitch-snap sweep is the engine's signature. The sweep RATE should be velocity-sensitive -- harder hits have faster pitch snaps (think acoustic drum physics: harder strike = faster membrane settling). Add `snap_pitchVelSens` (0-1)."
- **Chowning:** "The FM mode needs exposed modulator ratio as a user parameter, not just index depth. Ratio determines the spectral fingerprint. Fixed ratio means one timbre per mode."

**Priority correction:** Guild vel->timbre P1 -> **P0** (D001 violation).

**Estimated post-fix score:** 8.2 (corrected P0s), 8.6 (all + ghost additions)

---

### 8. OCEANIC (7.1) -- Guild Score Target: 8.4

| Guild Recommendation | Verdict | Reasoning |
|---------------------|---------|-----------|
| P0: Dynamic movement macro | **APPROVE** | The swarm engine without a coherence/scatter axis is a missed identity opportunity. This IS what the engine should do. |
| P1: User-triggerable murmuration | **APPROVE** | Excellent -- makes the unique feature accessible. Wire to CC, not just coupling. |
| P1: Sub-flock ratio tuning | **MODIFY** | Approve but limit range to 0.25-8.0 (not 0.5-4.0). The extreme ratios (very low = sub-octave beating, very high = crystalline harmonics) are where the swarm sounds most alien. Buchla: "Never artificially limit a parameter range. Let the performer find the edges." |

**Ghost additions:**
- **Buchla:** Add "predator mode" -- an external signal (coupling input or macro) that causes the swarm to flee, dramatically increasing separation and velocity. The BBD chorus (B013) should intensify during flight. This is behavioral synthesis, not just parameter modulation.
- **Schulze:** Expose particle count as a parameter (8-128). Fewer particles = individual voices audible (melodic). More particles = dense swarm texture (drone). This changes the engine's fundamental character.

**Estimated post-fix score:** 8.0 (P0), 8.6 (all + ghost additions)

---

### 9. OWLFISH (7.1) -- Guild Score Target: 8.3

| Guild Recommendation | Verdict | Reasoning |
|---------------------|---------|-----------|
| P0: Verify LFO fixes wired | **APPROVE** | D004 audit. If params exist but don't affect DSP, they're broken promises. |
| P1: 4-voice polyphony | **REJECT** | **The Mixtur-Trautonium was monophonic.** This is D003 (physics IS the synthesis). The Trautonium was a monophonic instrument with subharmonic division -- adding polyphony violates the historical model. Instead: add a paraphonic mode where subharmonic divisions of a SINGLE voice create pseudo-polyphonic textures. This IS how the real instrument created chords -- through division, not duplication. |
| P1: Subharmonic ratio control | **APPROVE** | This is the instrument's core identity. Selectable undertone series position is essential. |
| P2: MorphGlide rate automation | **APPROVE** | Low-effort character enhancement. |

**Ghost additions:**
- **Moog (who knew Trautwein):** "The Trautonium's ribbon controller was its soul. Map aftertouch to subharmonic selection so the performer can 'slide' through the undertone series with finger pressure. This is more authentic than polyphony."
- **Tomita:** "The Sacrificial Armor (saturation) stage should have a 'tube' mode with asymmetric clipping -- even harmonics only. This is what gives the Trautonium its distinctive organ-like warmth."

**Priority correction:** Guild polyphony P1 -> **REJECT** (D003 violation). Replace with paraphonic subharmonic mode.

**Estimated post-fix score:** 7.8 (P0 + corrected recommendations), 8.4 (all + ghost additions)

---

### 10. OTTONI (7.2) -- Guild Score Target: 8.3

| Guild Recommendation | Verdict | Reasoning |
|---------------------|---------|-----------|
| P0: Verify D001 vel->brightness | **APPROVE** | Doctrine compliance check. If velocity only hits amplitude, this is a violation for a brass engine (real brass gets brighter when played louder). |
| P1: Layer mode (simultaneous ages) | **MODIFY** | Approve but rename to "Ensemble mode." Layer implies equal balance. In a real brass section, the mature instruments (Teen) carry the melody while young instruments (Toddler) provide color. The default balance should reflect this: Teen at 0.8, Tween at 0.6, Toddler at 0.3. User-adjustable. |
| P1: Reverb stereo spread | **APPROVE** | Mono reverb on a brass engine is a waste. Stereo FDN with different allpass lengths for L/R is standard and correct. |

**Ghost additions:**
- **Vangelis:** "The GROW macro should affect more than just which age plays. As GROW increases, the lip buzz exciter should become more complex -- overtone count increases, embouchure tightness increases. Growing up IS learning to control the instrument. The timbral complexity should mirror the maturity axis."
- **Smith:** "Add a mute/harmon parameter per age stage. Brass mutes are the most expressive timbral modifier in the brass family. A wa-wa mute on the Teen voice would be immediately useful for jazz and film scoring."

**Estimated post-fix score:** 7.9 (P0), 8.5 (all + ghost additions)

---

### 11. OBRIX (7.2) -- Guild Score Target: 9.0+

| Guild Recommendation | Verdict | Reasoning |
|---------------------|---------|-----------|
| P0: Ship 150 factory presets | **APPROVE** | The DSP is Waves 1-3 complete with 65 params. The score is depressed by demonstration, not capability. This is correct prioritization. |
| P1: LFO rate ceiling increase (30->100 Hz) | **APPROVE** | Audio-rate modulation is expected from a modular engine. Blessing B016 (Brick Independence) means each brick should have its own LFO capable of audio rate. |
| P1: BPM-synced delay FX | **APPROVE** | Standard production feature for a modular engine's delay effect. |
| P2: Default init patch improvement | **APPROVE** | Speaks to DB003 (init patch debate). A sine into an open filter is dead on arrival. Saw + LP filter at 2kHz is the correct default for a subtractive engine. Kakehashi wins this one. |

**Ghost additions:**
- **Buchla:** "Journey Mode (suppress note-off) should be the DEFAULT for at least 30% of the factory presets. This is OBRIX's differentiator -- infinite evolution. If all 150 presets are conventional note-on/note-off patches, the engine's unique character is buried."
- **Schulze:** "Add a 'freeze brick' function where individual bricks can be frozen (looping their current output buffer) while others continue evolving. This creates the temporal layering that generative music requires."

**Estimated post-fix score:** 8.5 (P0 presets), 9.2 (all + ghost additions)

---

### 12. OVERDUB (7.4) -- Guild Score Target: 8.5

| Guild Recommendation | Verdict | Reasoning |
|---------------------|---------|-----------|
| P0: Multi-shape LFO | **APPROVE** | D002 violation. Sine-only LFO at line 198. For a dub engine, this is unacceptable -- dub music lives on rhythmic modulation. |
| P0: Add second LFO | **APPROVE** | D002 requires 2+ LFOs. The spring reverb (B004) deserves its own modulation source. |
| P1: Tape delay BPM sync | **APPROVE** | Essential production feature. Dotted-eighth is the classic dub rhythm -- Guild correctly identifies this. |
| P1: Tape flutter depth control | **MODIFY** | Approve but expand: flutter should have BOTH rate and depth, AND the flutter should be applied in the delay FEEDBACK path (not the dry path). Each repeat gets progressively more warped. This is how real tape works. Guild correctly notes this but the implementation should emphasize the cumulative degradation. |

**Ghost additions:**
- **Vangelis:** "The spring reverb (B004) needs a 'crash' trigger -- a momentary impulse that overloads the spring, creating the classic spring reverb crash. Map it to velocity > 120 or a dedicated trigger parameter. Every dub producer knows this sound."
- **Moog:** "The drive stage should have switchable topology: soft clip vs. diode vs. tape saturation. Different saturation characters create fundamentally different dub flavors."

**Estimated post-fix score:** 8.1 (P0), 8.7 (all + ghost additions)

---

### 13. OHM (7.6) -- Guild Score Target: 8.5

| Guild Recommendation | Verdict | Reasoning |
|---------------------|---------|-----------|
| P0: Stereo voice spread | **APPROVE** | Mono summing wastes the 3-section family metaphor. The spatial arrangement IS the family dynamic. |
| P0: D001 vel->brightness | **APPROVE** | Doctrine violation. Half-honored (intensity not brightness) is not honored. |
| P1: SIDES macro as spatial LFO | **MODIFY** | Approve the concept but rename to "WANDER" or keep as "SIDES" -- the auto-panning should be irregular, not a clean LFO sine. Use the Dad waveguide's resonance as a modulation source for In-Law panning. The family influences each other's position, not a metronome. |
| P1: Grain density macro routing | **APPROVE** | COMMUNE macro should be compound. Single-target macros are D004-adjacent (weak macro = half-dead parameter). |

**Ghost additions:**
- **Kakehashi:** "The MEDDLING macro should introduce cross-talk between sections: Dad's waveguide output feeds into Obed's FM modulator, In-Law grain position modulates Dad's exciter strength. The family metaphor demands interaction, not just proximity."

**Estimated post-fix score:** 8.2 (P0), 8.6 (all + ghost additions)

---

### 14. ODYSSEY (7.6) -- Guild Score Target: 8.4

| Guild Recommendation | Verdict | Reasoning |
|---------------------|---------|-----------|
| P0: Climax system presets | **APPROVE** | The Climax system is the engine's unique selling point and was never demonstrated. This is not a code fix but a content fix. Correct prioritization. |
| P1: Bipolar filter envelope depth | **APPROVE** | Standard feature for any subtractive engine. Negative envelope depth is essential for experimental work. |
| P1: Cross-FM depth modulation | **APPROVE** | Now that the dead param is fixed, modulating it makes it alive. |
| P2: Drift-to-filter routing | **APPROVE** | Character-enhancing. Correlated pitch+filter drift is what makes analog synths sound organic. |

**Ghost additions:**
- **Schulze:** "The Climax system should have a DURATION parameter (10s to 300s) controlling the evolution timescale. A 5-minute Climax is fundamentally different from a 30-second one. Berlin School sequences evolve over minutes, not seconds."
- **Vangelis:** "VoyagerDrift amount should be velocity-responsive. Gentle playing = subtle drift. Hard playing = wild excursions. The performer's energy should feed the journey."

**Estimated post-fix score:** 8.1 (P0), 8.6 (all + ghost additions)

---

### 15. OVERWORLD (7.6) -- Guild Score Target: 8.4

| Guild Recommendation | Verdict | Reasoning |
|---------------------|---------|-----------|
| P0: ERA triangle LFO modulation | **APPROVE** | The ERA triangle (B009) is the engine's Blessed feature. A Blessed feature that cannot be modulated over time violates the spirit of Blessings. |
| P1: Glitch type selection | **APPROVE** | Exposing existing capability. If GlitchEngine supports multiple types internally, hiding them is a D004-adjacent issue. |
| P1: Chip-accurate portamento | **APPROVE** | **Particularly strong recommendation.** Discrete semitone steps for pitch slides is historically accurate AND sonically distinctive. This is D003 thinking applied to chip synthesis. |
| P2: Stereo output width per chip engine | **MODIFY** | The Guild's suggestion of NES=left, Genesis=center, SNES=right is arbitrary and historically inaccurate (NES had stereo-capable channels, SNES had true stereo). Instead: let the ERA position control stereo width. At a single vertex = mono (one console). Between vertices = widening stereo (console blend creates spatial interest). |

**Ghost additions:**
- **Kakehashi:** "Add a 'bit-accurate' toggle per console emulation. When enabled, output is quantized to the console's actual bit depth (NES=7-bit, Genesis=9-bit, SNES=16-bit). The quantization noise IS the character."
- **Tomita:** "The FIREcho delay should have console-accurate reverb models. NES reverb was short and metallic. SNES had long, lush hardware reverb. The FX character should follow the ERA position."

**Estimated post-fix score:** 8.1 (P0), 8.6 (all + ghost additions)

---

### 16. OVERTONE (7.6) -- Guild Score Target: 9.0

| Guild Recommendation | Verdict | Reasoning |
|---------------------|---------|-----------|
| P0: Implement true 8-voice polyphony | **APPROVE** | Declaring 8 voices and implementing 1 is a D004 violation. The most impactful single fix in the review. |
| P0: Verify Pi table patch | **APPROVE** | QA verification. Essential for a D003-adjacent engine (mathematical rigor). |
| P1: Constant type sweep (continuous 0-3) | **MODIFY** | Approve the axis but REJECT continuous crossfade between convergent tables. The mathematical constants are discrete entities -- Pi does not "become" E. Instead: use a morphing function that interpolates the PARTIAL RATIOS between two tables while maintaining mathematical meaning. `ratio[i] = lerp(piTable[i], eTable[i], blend)` is correct. Crossfading the AUDIO OUTPUT of two tables is not. |
| P1: Partial brightness macro | **APPROVE** | The spectral tilt approach (natural rolloff vs inverted) is acoustically correct and creates a dramatic sweep. |

**Ghost additions:**
- **Chowning:** "This is a mathematics engine. Add a 'custom convergent' mode where the user provides a decimal number (0.001-99.999) and the engine computes its continued fraction expansion in real-time as the partial series. This turns the synth into a CALCULATOR of sound -- truly novel. No other synth does this."
- **Buchla:** "The 8 partials should be independently amplitude-controllable via a 'partial drawbar' interface (like a Hammond organ but with irrational ratios). This bridges the familiar (drawbar organ) with the novel (continued fraction spectra)."

**Estimated post-fix score:** 8.5 (P0), 9.2 (all + ghost additions)

---

### 17. OCEANDEEP (7.8) -- Guild Score Target: 8.9

| Guild Recommendation | Verdict | Reasoning |
|---------------------|---------|-----------|
| P0: Add filter ADSR | **APPROVE** | A bass synth without filter ADSR is fundamentally incomplete. Smith: "This is not optional. This is subtractive synthesis 101." |
| P0: Add pitch bend handler | **APPROVE** | A bass engine without pitch bend is unusable for performance. Use `PitchBendUtil.h` (see shared utility verdict above). |
| P1: Bioluminescent exciter controls | **APPROVE** | Character-enhancing. Rate/density/bandwidth expose the unique identity. |
| P1: Sidechain input for compressor | **APPROVE** | The hydrostatic compressor with external sidechain input is both production-useful AND character-consistent (external pressure affecting deep-sea compression). |

**Ghost additions:**
- **Moog:** "The 3-sine sub stack should have adjustable phase relationships. At 0 phase offset = maximum constructive interference (loudest sub). At 120 degrees offset = phase cancellation patterns that create movement. This is how real analog sub oscillators interact."
- **Smith:** "The waveguide body (comb filter) needs exciter position control. Where you 'strike' a resonant body determines the harmonic content. `deep_exciterPos` (0-1) controlling the comb filter tap position."

**Estimated post-fix score:** 8.5 (P0), 9.0 (all + ghost additions)

---

### 18. OMBRE (7.8 -> ~8.0 est.) -- Guild Score Target: 8.5

| Guild Recommendation | Verdict | Reasoning |
|---------------------|---------|-----------|
| P1: Grain head control parameters | **APPROVE** | Expanding the granular read system is character-consistent. Head count and spread are the engine's unique modulation axes. |
| P1: Decay rate as LFO destination | **APPROVE** | Excellent -- "memories dissolve rhythmically" is exactly the poetic-technical fusion this engine needs. |
| P1: Feedback saturation | **APPROVE** | Cumulative warmth through replay is physically accurate (tape degradation) and character-perfect. |

**Ghost additions:**
- **Schulze:** "Add a 'reminiscence bump' -- a parameter that makes the memory buffer's decay non-linear, preserving older memories better than recent ones (the psychological phenomenon where people remember early life more vividly). Implement as a decay curve that flattens at the buffer's oldest region."
- **Tomita:** "The 4 granular read heads should have independent pitch-shift capability. Memories recalled at different speeds/pitches create the dreamlike quality of actual memory recall."

**Note:** Already at ~8.0. All recommendations are enhancements, not doctrine fixes. Lower priority.

**Estimated post-fix score:** 8.5 (P1s), 8.9 (all + ghost additions)

---

### 19. OBBLIGATO (7.8) -- Guild Score Target: 8.6

| Guild Recommendation | Verdict | Reasoning |
|---------------------|---------|-----------|
| P0: D001 vel->brightness | **APPROVE** | Doctrine violation. Intensity-not-brightness confirmed. For woodwinds, harder blowing = brighter tone is physically accurate (D003). |
| P0: Fix FX chain routing | **APPROVE** | Broken routing is a functional bug, not a feature request. |
| P1: Layer mode stereo imaging | **APPROVE** | Brother A left, Brother B right is natural for a duet. Equal-power pan law is correct. |
| P1: Breath noise layer | **APPROVE** | Character-enhancing and physically accurate (D003). All wind instruments have breath noise. |

**Ghost additions:**
- **Chowning:** "The reed model (Brother B) should have a nonlinear pressure-to-amplitude curve that produces the characteristic 'honk' at high pressure. Real reed instruments have a threshold behavior -- below threshold, gentle tone; above threshold, the reed slaps and the timbre changes dramatically."
- **Vangelis:** "Add legato mode with breath continuity. When playing legato, the breath envelope should NOT reset between notes -- continuous airflow with pitch changes. This is how real woodwind players phrase."

**Estimated post-fix score:** 8.3 (P0), 8.7 (all + ghost additions)

---

### 20. OUTWIT (7.9) -- Guild Score Target: 8.8

| Guild Recommendation | Verdict | Reasoning |
|---------------------|---------|-----------|
| P0: Pitch wheel handler | **APPROVE** | Simple fix, small effort, clears 8.0. Use `PitchBendUtil.h`. |
| P1: Raise step rate to 200 Hz | **APPROVE** | **Particularly strong recommendation.** Audio-rate CA synthesis is genuinely novel. The anti-aliasing note is essential -- add 1-pole LP above 100 Hz step rate. This could be OUTWIT's breakthrough feature. |
| P1: Stereo Den reverb | **APPROVE** | Coprime delay lengths for L/R is acoustically correct. 8-arm panning (left to right) is character-consistent. |
| P1: BPM-synced step rate | **APPROVE** | Bridges experimental and rhythmic music. Correct musical thinking. |

**Ghost additions:**
- **Buchla:** "The 256 Wolfram rules should be explorable via a macro that sweeps through rule space. Not linear (0-255) but organized by complexity class: Class 1 (uniform) -> Class 2 (periodic) -> Class 3 (chaotic) -> Class 4 (complex/edge-of-chaos). The sweep should follow Wolfram's classification, not numerical order."
- **Schulze:** "Add rule mutation: a parameter that introduces random bit-flips in the current rule at a controllable rate. The CA evolves its own rules over time. This is evolution, not just execution."

**Estimated post-fix score:** 8.2 (P0), 9.0 (all + ghost additions)

---

## III. Missing Engine: OCELOT P0 Regrade

The Guild lists OCELOT's `applyCouplingInput` as P0 and biome automation as P1. The ghosts unanimously override: **both are P0**. Without temporal evolution, OCELOT is a static 4-layer mixer with a rainforest label. The biome automation IS the engine's breathing (D005). Current score 6.4 reflects this fundamental gap.

---

## IV. Missing Engine: ORBWEAVE, OSTINATO, OPENSKY, OUIE, ORGANISM (Not in Guild Review)

The Guild reviewed 20 sub-8.0 engines. The following engines have recent seance verdicts but were not included in the Guild review. Noting for completeness:

- **ORBWEAVE** -- Seance completed 2026-03-20. If sub-8.0, should be added to the fix queue.
- **OSTINATO** -- Seance completed 2026-03-20. If sub-8.0, should be added.
- **OPENSKY** -- Seance completed 2026-03-20.
- **OUIE** -- Seance completed 2026-03-20.
- **ORGANISM** -- Seance completed 2026-03-20.

These engines should be cross-referenced against their seance verdicts to confirm whether they fall within Guild review scope.

---

## V. Final Prioritized Fix List (Guild + Seance Combined)

### Tier 0: Doctrine Violations (Must Fix for V1)

| # | Engine | Fix | Doctrine | Effort | Score Impact |
|---|--------|-----|----------|--------|-------------|
| 1 | OVERTONE | Implement 8-voice polyphony | D004 | Medium | 7.6 -> 8.5 |
| 2 | OCEANDEEP | Filter ADSR + pitch bend | D001/D006 | Small | 7.8 -> 8.5 |
| 3 | OCELOT | Wire coupling + biome automation (upgraded from P1) | D004/D005 | Medium | 6.4 -> 8.0 |
| 4 | ODDOSCAR | 2 user LFOs + filter envelope | D002 | Medium | 6.9 -> 8.2 |
| 5 | OBESE | 2 user LFOs + sub level + Mojo modulation (upgraded from P1) | D002/B015 | Medium | 6.6 -> 8.3 |
| 6 | OBLIQUE | 2 user LFOs with prism routing | D002/D005 | Medium | 5.9 -> 8.0 |
| 7 | ODDFELIX | 2 user LFOs + vel->timbre (upgraded from P1) | D001/D002 | Medium | 7.0 -> 8.2 |
| 8 | OVERDUB | Multi-shape LFO + second LFO | D002 | Small | 7.4 -> 8.1 |
| 9 | OLE | Fix husband regression + dead params | D004 | Small | 7.0 -> 7.9 |
| 10 | OHM | Stereo spread + vel->brightness | D001 | Small | 7.6 -> 8.2 |
| 11 | OBBLIGATO | Vel->brightness + FX routing fix | D001 | Small | 7.8 -> 8.3 |
| 12 | OUTWIT | Pitch wheel handler | D006 | Tiny | 7.9 -> 8.2 |
| 13 | OTTONI | Vel->brightness verification | D001 | Tiny | 7.2 -> 7.9 |
| 14 | OBRIX | Ship 150 factory presets | -- | Large | 7.2 -> 8.5 |

### Tier 1: Character Enhancement (Gets to 8.5+)

| # | Engine | Fix | Effort | Score Impact |
|---|--------|-----|--------|-------------|
| 15 | OBLIQUE | BPM-sync bounce + refraction index (ghost) | Medium | 8.0 -> 8.5 |
| 16 | OCELOT | EcosystemMatrix audible + stratum levels + predator-prey (ghost) | Large | 8.0 -> 8.7 |
| 17 | OCEANIC | Swarm macro + murmuration trigger + flock ratios | Medium | 7.1 -> 8.6 |
| 18 | OWLFISH | LFO verify + paraphonic subharmonic mode (ghost-corrected) + ratio control | Medium | 7.1 -> 8.4 |
| 19 | OTTONI | Ensemble mode + stereo reverb + mute param (ghost) | Medium | 7.9 -> 8.5 |
| 20 | ODYSSEY | Climax presets + bipolar filter env + Climax duration (ghost) | Medium | 7.6 -> 8.6 |
| 21 | OVERWORLD | ERA LFO + glitch types + chip portamento | Medium | 7.6 -> 8.6 |
| 22 | OVERTONE | Partial ratio morph + brightness tilt + custom convergent (ghost) | Medium | 8.5 -> 9.2 |
| 23 | OCEANDEEP | Bio exciter controls + sidechain + phase control (ghost) | Medium | 8.5 -> 9.0 |
| 24 | OUTWIT | Step rate 200Hz + stereo reverb + BPM sync + rule sweep (ghost) | Medium | 8.2 -> 9.0 |
| 25 | OVERDUB | BPM tape delay + flutter depth + spring crash (ghost) | Medium | 8.1 -> 8.7 |
| 26 | ODDOSCAR | Morph LFO + aftertouch->filter + ladder drive (ghost) | Medium | 8.2 -> 8.7 |
| 27 | OBESE | Arp BPM sync + filter drive (ghost) | Small | 8.3 -> 8.8 |
| 28 | OHM | WANDER spatial + grain density routing + cross-talk (ghost) | Medium | 8.2 -> 8.6 |
| 29 | OLE | Alliance LFO + separate LFO2 + strum velocity (ghost) | Medium | 7.9 -> 8.4 |
| 30 | OBBLIGATO | Stereo duet + breath noise + legato mode (ghost) | Medium | 8.3 -> 8.7 |

### Tier 2: Polish (Gets to 9.0)

| # | Engine | Fix |
|---|--------|-----|
| 31 | OBLIQUE | Stereo bounce scatter + per-facet detune (ghost) |
| 32 | OBESE | Unison detune spread + strum arp (ghost) |
| 33 | OBSIDIAN | Formant morph + PD XY macro + partial count |
| 34 | OVERWORLD | Bit-accurate toggle + console-accurate reverb (ghost) |
| 35 | OMBRE | Grain head control + decay LFO dest + feedback sat + reminiscence (ghost) |
| 36 | OBRIX | LFO rate 100Hz + BPM delay + init patch |
| 37 | OUTWIT | Rule complexity sweep (ghost) + rule mutation (ghost) |

---

## VI. Estimated Final Scores (After All Approved Fixes)

| Engine | Current | After Tier 0 | After Tier 1 | After Tier 2 |
|--------|---------|-------------|-------------|-------------|
| OBLIQUE | 5.9 | 8.0 | 8.5 | 8.9 |
| OCELOT | 6.4 | 8.0 | 8.7 | 8.7 |
| OBESE | 6.6 | 8.3 | 8.8 | 9.0 |
| OBSIDIAN | ~8.2 | 8.2 | 8.2 | 9.0 |
| ODDOSCAR | 6.9 | 8.2 | 8.7 | 8.7 |
| OLE | 7.0 | 7.9 | 8.4 | 8.4 |
| ODDFELIX | ~7.0 | 8.2 | 8.6 | 8.6 |
| OCEANIC | 7.1 | 7.8* | 8.6 | 8.6 |
| OWLFISH | 7.1 | 7.8 | 8.4 | 8.4 |
| OTTONI | 7.2 | 7.9 | 8.5 | 8.5 |
| OBRIX | 7.2 | 8.5 | 9.0 | 9.2 |
| OVERDUB | 7.4 | 8.1 | 8.7 | 8.7 |
| OHM | 7.6 | 8.2 | 8.6 | 8.6 |
| ODYSSEY | 7.6 | 8.1 | 8.6 | 8.6 |
| OVERWORLD | 7.6 | 8.1 | 8.6 | 8.6 |
| OVERTONE | 7.6 | 8.5 | 9.2 | 9.2 |
| OCEANDEEP | 7.8 | 8.5 | 9.0 | 9.0 |
| OMBRE | ~8.0 | 8.0 | 8.5 | 8.9 |
| OBBLIGATO | 7.8 | 8.3 | 8.7 | 8.7 |
| OUTWIT | 7.9 | 8.2 | 9.0 | 9.0 |

*OCEANIC P0 alone gets to 7.8 because the swarm macro is an enhancement, not a doctrine fix. D001-D006 were resolved in prior sweeps. Tier 1 is where the real character emerges.

**Fleet average after Tier 0:** 8.1 (up from 7.2)
**Fleet average after Tier 1:** 8.6
**Fleet average after Tier 2:** 8.7
**Engines below 8.0 after Tier 0:** 3 (OCEANIC 7.8, OLE 7.9, OTTONI 7.9) -- down from 20
**Engines below 8.0 after Tier 1:** 0

---

## VII. Council Closing Statement

**Moog:** "The Guild sees what's missing. We see what must be protected. Every fix should make the engine more itself. If you can't explain how a change serves the engine's identity, don't make it."

**Buchla:** "I count 37 items in the fix list. Most are good. But I warn against the assembly-line mentality. Don't implement StandardLFO in 8 engines in one sitting. Implement each engine's modulation system as a separate act of design, informed by that engine's character."

**Chowning:** "The mathematical engines (OVERTONE, OUROBOROS, ORGANON) must maintain rigor. The Guild's recommendations are producer-friendly but must not compromise the mathematical identity. Custom convergent mode for OVERTONE is the right kind of ambition."

**Kakehashi:** "I appreciate the Guild's focus on accessibility. Many of these engines have powerful DSP that nobody can reach because controls are hidden or absent. Opening the doors is not dilution -- it's invitation."

**Vangelis:** "The most important recommendation in this entire document is the Climax system presets for ODYSSEY. Features without demonstration are invisible. Ship the presets."

**Schulze:** "Time is the missing dimension in half these engines. Biome evolution, memory decay, climax crescendo, CA rule mutation -- these are all ways of giving engines a relationship with time. An instrument that doesn't change over time is not an instrument. It's a snapshot."

**Smith:** "Fix the doctrine violations first. D001, D002, D004 -- these are not negotiable. Everything else is optimization."

**Tomita:** "Finally: the spring reverb crash for OVERDUB, the mute for OTTONI, the console-accurate reverb for OVERWORLD -- these are the details that separate a professional instrument from a student project. Do not skip them."

---

*Seance review complete. The ghosts are dismissed. Implementation may proceed under the prioritized fix list above.*
