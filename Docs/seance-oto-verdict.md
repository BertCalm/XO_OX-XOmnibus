# The Verdict — XOto
**Seance Date**: 2026-03-21
**Engine**: XOto | East Asian Reed Organ Synthesis | "The Breath Between Worlds"
**Collection**: Chef Quad, Kitchen Essentials — Engine #1
**Accent**: Bamboo Green `#7BA05B`
**Prefix**: `oto_` | **Params**: 20 declared | **Lines**: ~900 (header)
**Presets**: 10 factory (all moods covered)
**Retreat Status**: No retreat conducted yet

---

## The Council Has Spoken

**Ghost 1 — Robert Moog**

What excites me: The breath instability system is the soul of this instrument. Two internal LFOs — a slow 0.3 Hz drift and a faster 5.5 Hz tremolo — are combined via the Pressure parameter to produce per-voice pitch wandering (up to ±8 cents) and amplitude modulation (up to ±15%). This is not vibrato bolted on afterward. It is breath physics modeled directly, per voice, continuously. When you play a chord on the Sho model and let the pressure parameter breathe, you get the authentic shimmer of eleven independent reeds sharing a single breath source. That is the right design philosophy: model the physics of what makes the instrument human, not the superficial characteristics of its waveform.

What concerns me: The chiff transient is time-limited — a Hann-windowed burst lasting 15 to 40 milliseconds depending on the model. This is appropriate for attack character. However, the chiff generator uses a fixed-frequency pitched component cycling at 8× the phase position within the burst window. This is not model-dependent in any spectral sense — it is the same pitched component regardless of organ model, differentiated only by the noise mix ratio. The Sheng's transient should be spectrally distinct from the Sho's: brighter harmonics, faster initial spectral rise. Right now it is primarily the duration and noise mix that distinguish them, which is a partial solution. A model-specific spectral centroid for the chiff pitched component would complete this.

What I'd add/change: Expose the breath drift rate as a user parameter. Currently hardcoded to 0.3 Hz drift / 5.5 Hz tremolo. Letting the user set the drift from 0.05 Hz (geological, almost imperceptible) to 2 Hz (nervous, rapid breath) would dramatically expand the expressive range for the same instrument. This is a single parameter that costs nothing in CPU but transforms the playability.

**Ghost 2 — Don Buchla**

What excites me: The Sho aitake ratios are not approximations — they are derived from gagaku tuning theory with non-harmonic intervals (1.000, 1.0595, 1.1892, 1.3348, 1.4983…). The Sheng uses the natural harmonic series (1, 2, 3, 4, 5…). The Khene has deliberately detuned pairs (1.000/1.003, 2.000/2.007, 3.000/3.005…). These are three genuinely different tuning systems represented as three separate partial ratio tables. This is not one synthesis engine wearing different costumes — it is three distinct acoustic physics models sharing a common signal flow. The cultural specificity embedded in constants deserves recognition. When I see `kShoAitakeRatios` with its minor-2nd beating partner at 1.0595, I see an instrument designer who did the musicological work.

What concerns me: The Sheng model uses integer harmonic ratios (1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11). This is a pure harmonic series — which is more accurate for a reed organ than for most instruments, but a real Sheng has slight inharmonicities in its upper partials due to reed stiffness and bore geometry. The comment references "Zhou (2010)" in the header but I see no citation text anywhere in the code. If you cite a source, the source must appear in the header. As it stands, the Sheng model's integer ratios could equally describe a sine-wave additive organ. The distinctiveness of the Sheng relative to the Sho comes primarily from amplitude envelope differences, not spectral content.

What I'd add/change: The Competition parameter is a beautiful idea — adversarial coupling where voices compete for shared breath pressure. But at full competition with 8 voices, the scaling is `competitionScale = 1 - 0.6 * (7/8) = 0.475`. That is only a 52.5% reduction for the most crowded condition. Real free-reed instruments at maximum polyphony are severely breath-limited — the reduction should be steeper, closer to 0.2 at the floor for 8 simultaneous voices, which requires a more aggressive curve. The current linear interpolation is too gentle to model the actual physics of shared breath.

**Ghost 3 — Dave Smith**

What excites me: The detune implementation is architecturally correct. Rather than applying a single global detune to all partials, the system computes a per-partial deterministic frequency offset based on partial index: `detuneOffset = (p - 0.5 * numPartials) * detuneNow * 0.5`. This means lower-numbered partials are pulled one direction and higher-numbered partials the other, creating a stereo-like spread across the partial bank. The result is not random wobble but structured partial displacement — which is what you actually observe in the physical instruments where pipe lengths create systematic pitch discrepancies across the instrument's range. The offset is in Hz, not cents, which means it produces stronger beating between fundamental partials and weaker relative beating between higher harmonics — exactly the physical relationship. That is correct acoustic physics without a citation.

What concerns me: The filter envelope is hardcoded at note-on: `setADSR(0.001f, 0.3f + (1.0f - vel) * 0.5f, 0.0f, 0.3f)`. Attack is always 1 millisecond. Sustain is always zero. Decay ranges from 0.3 to 0.8 seconds depending on velocity, but the user cannot change any of this. Soft velocity gives a longer filter decay; hard velocity gives a shorter one — which is acoustically backwards from most physical instruments (harder strikes produce longer spectral energy in reeds and strings). I expect the velocity relationship here is a design choice for timbral variety rather than physical accuracy, but the direction should be re-examined. More critically: exposing at least the filter decay as a user parameter would allow sound designers to sculpt the attack character of each model independently.

What I'd add/change: The LFO1 depth defaults to 0.0. This means every preset that does not explicitly set `oto_lfo1Depth` above zero produces an engine with no vibrato at all. The breath source drift is always active (set by Pressure, which defaults to 0.2), so there is some movement. But a default LFO depth of zero means beginners encountering Oto for the first time will twist the LFO rate and hear nothing change — a discoverability failure. Default depth of 0.08 to 0.12 would give every preset gentle vibrato from the start.

**Ghost 4 — Ikutaro Kakehashi**

What excites me: The model-specific attack shaping in `noteOn` is the kind of detail that separates a professional instrument from a toy. Sho minimum attack is 0.05 seconds (floor enforcement). Sheng attack is halved (`atkParam * 0.5f`), never below 5ms. Khene uses the raw parameter value. Melodica attack is 80% of the parameter value, never below 3ms. The user sets one attack knob, and each model interprets it appropriately — fast Sheng articulation, slow Sho breath buildup — without requiring the user to manage separate attack settings per model. That is a genuine instrument design decision that respects the player's time.

What concerns me: The `oto_competition` parameter defaults to 0.0 and does nothing unless the user turns it up. But competition — the shared-breath model — is actually the most physically accurate behavior of free-reed instruments. When you play two notes on a real Shō, the second reed draws from the same breath pressure as the first. This should be the default behavior at low polyphony, not an opt-in feature. The current design treats competition as an exotic control rather than a physical reality. At the minimum I would set the default to 0.15 — a subtle breath-sharing effect that reflects the instrument's nature without constraining the player.

What I'd add/change: No sustain pedal handling visible in the MIDI parsing loop. The engine handles note-on, note-off, pitch wheel, channel pressure, and CC1 — but sustain (CC64) is absent. This is relevant for organs: a player holding a sustain pedal while lifting fingers expects notes to continue. Melodica in particular is often played with a sustain pedal for lo-fi ambient work. This is a missing MIDI contract that should be added to bring Oto into parity with other polyphonic engines in the fleet.

**Ghost 5 — Suzanne Ciani**

What excites me: The crosstalk system is genuinely novel in this fleet context. Each voice injects a fraction of its previous-block output into its adjacent voice neighbors (previous index and next index in the voice allocation table), weighted by `crosstalkNow * 0.3`. The one-block delay prevents feedback loops. This creates intervoice coupling that is not about pitch or filter modulation but about waveform leakage — the low-level acoustic bleed between pipes in a physical instrument. When you sustain a chord and raise the Crosstalk parameter, the voices begin to hear each other in a way that sounds like physical proximity rather than electronic mixing. I have not seen this behavior implemented in the fleet. It is subtle when used carefully and extraordinary at high settings.

What concerns me: The model crossfade system has ghost variables — `prevOrganGain` and `prevOrganModel` — declared on `OtoVoice` but never written during `renderBlock` and never read anywhere in the engine. The comment says "seamless organ switching" but the implementation is not present. Switching organ models mid-performance will cause a hard timbre change at the current parameter state rather than a smooth crossfade. This is a dead code path that suggests an unfinished feature.

What I'd add/change: The Sho model is the most beautiful instrument in this engine and should be the first thing players hear. Preset "Oto Celestial Reeds" captures it well but it is buried in the Ethereal mood category, which many users will not browse first. The Foundation preset "Oto Sho Aitake" uses a slightly lower cluster and shorter release than what would make the strongest first impression. I would increase the default Sho preset's release to 2+ seconds and push the cluster to 0.95 — let users hear the full aitake on first contact.

**Ghost 6 — Wendy Carlos**

What excites me: Four genuinely distinct instrument families — Japanese court music, Chinese folk/classical, Lao village music, and modern Latin/dub melodica — housed in one synthesis engine with a single consistent parameter language. This is not timbral variety for its own sake. These four traditions represent different relationships to breath, time, and harmony. The Sho requires patience (40ms chiff, slow attack floor). The Sheng rewards precision (15ms chiff, fast attack). The Khene is immediate and aggressive (25ms chiff, paired-pipe beating built into the ratio tables). The Melodica is intimate (35ms chiff, sawtooth fundamental). The parameter space enforces these different relationships through the model-specific attack scaling. This is instrument pedagogy embedded in DSP.

What concerns me: The Melodica model uses `2.0f * voice.partialPhases[p] - 1.0f` for the fundamental — a naive linear sawtooth. This produces the expected harmonic content but with severe aliasing at higher pitches because there is no band-limiting. For a quality synthesis platform, the fundamental partial of the Melodica model should use a PolyBLEP or minBLEP anti-aliased sawtooth. At low notes (below C3) the aliasing is inaudible. At high notes (above C5) it will introduce spectral artifacts that clash with the cleaner sine-wave upper partials. This is the only aliasing risk in the engine and it is isolated to one partial in one model — but it is the model's most sonically prominent partial.

What I'd add/change: No pitch-based brightness scaling for the upper partial register. Free-reed instruments naturally roll off in a frequency-dependent way at high pitches because the reed physics change. A gentle high-frequency dampening applied as notes ascend above C5 (say, 6dB/octave above that threshold) would make high notes sound more physically authentic and prevent the engine from becoming harsh in the top octave. This is a one-line addition per voice before the filter.

**Ghost 7 — Isao Tomita**

What excites me: The cluster density parameter is elegantly implemented. Integer-count partials up to N-1, with the last partial crossfaded by `frac = (clusterNow * (N-1)) - (numPartials-1)`, clamped to 0-1. This means sweeping Cluster from 0 to 1 produces a continuous, click-free build from a single partial to the full instrument voicing. The fractional fade on the last partial prevents the stair-step discontinuities you get when partials switch on abruptly. Combined with the normalize-by-sqrt-count output scaling, the full cluster is not louder than a single partial. This is the kind of DSP craftsmanship that makes a simple parameter feel musical to use.

What concerns me: Only one LFO1 shape — hardcoded to `StandardLFO::Sine` in the render loop. The StandardLFO supports multiple shapes (the fleet uses this feature elsewhere), but Oto always uses sine regardless of any user-visible LFO shape selector. There is no `oto_lfo1Shape` parameter declared. For an organ engine where vibrato shape matters — Shō uses a gentle, near-sinusoidal vibrato; Melodica often uses a slightly asymmetric, breath-pressure-modulated vibrato that is closer to triangle — having only sine is an expressive limitation. A tri/sine toggle would cost two states and make the vibrato more instrument-authentic.

What I'd add/change: No reverb or spatial processing in the engine itself. All spatial character must come from external FX. This is architecturally correct per XOmnibus design rules (the engine should output clean signals; FX are layered above). But the Sho model in particular is strongly associated with reverberant performance spaces — gagaku is performed in wooden structures with long natural reverb. The preset descriptions mention this ("infinite space," "subterranean") but cannot deliver the implied acoustics without the player routing to FX. The cookbook and seance documentation should explicitly recommend pairing Sho presets with Cathedral from BoutiqueFXChain.

**Ghost 8 — Raymond Scott**

What excites me: The mechanical ingenuity of the competition model delights me — adversarial amplitude coupling implemented as a simple polyphony counter with a linear reduction formula. It is an elegant simplification of a complex physical phenomenon: the more reeds are open simultaneously, the less pressure each receives, so each voice becomes quieter and slightly less sustaining. The formula `competitionScale = 1 - reduction * 0.6f` with a 0.2 floor gives polyphonic passages a natural dynamic compression that you cannot achieve with a simple limiter. It sounds like a real instrument because the amplitude relationship between voices changes depending on how many are sounding simultaneously.

What concerns me: The stereo pan spread is computed at note-on from voice index: voice 0 pans 60% left, voice 7 pans 60% right, voices between interpolate. This creates a fixed-width stereo spread based on voice allocation order rather than on musical pitch or timing. When eight voices are active, the highest-priority allocated voice is far left and the most recently allocated is far right, regardless of pitch. A pitch-proportional pan spread (lower notes slightly left, higher notes slightly right, as in a grand piano) would produce a more musically coherent stereo image. Voice-index panning is a common shortcut that sounds strange on sustained polyphony where the spatial position of a note shifts as voices are stolen and reallocated.

What I'd add/change: The silence gate is set to a 500ms hold, which is appropriate for sustained organ tails. However, the silence gate logic calls `analyzeForSilenceGate(buffer, numSamples)` after the render loop. If the player is holding long notes on the Sho with 3-second release (as in "Oto Celestial Reeds"), there may be a perceptible amplitude discontinuity when the gate activates during what should be a smooth tail. The 500ms hold may be too short for maximum-release Sho presets. Consider a hold of 1000-1500ms for this engine.

---

## Doctrine Compliance

| Doctrine | Status | Evidence |
|----------|--------|---------|
| D001 | PASS | `velCutoffBoost = voice.velocity * 4000.0f` adds to voice cutoff; filter envelope decay scaled by `(1.0f - vel) * 0.5f` (longer decay on soft notes); per-partial `modeAmp` variant implicit in amplitude normalization. Velocity shapes timbre on two parallel paths. |
| D002 | PASS (partial concern) | LFO1 rate/depth declared and read in render, routed to pitch vibrato and secondary filter modulation. 4 macros all wired to DSP (CHARACTER→cluster+chiff+buzz, MOVEMENT→LFO depth+pressure, COUPLING→competition+crosstalk, SPACE→filter open). Breath source (2 internal LFOs) runs autonomously. Mod wheel→filter cutoff wired. Aftertouch→pressure+filter wired. No LFO shape parameter. No second LFO. |
| D003 | N/A | Additive synthesis, no physical modeling claims requiring citation. Ratio tables have cultural/acoustic justification in header comments. The Zhou (2010) mention in header references Sheng spectral measurement but is not linked to a specific table number — minor gap. |
| D004 | PASS with caveat | All 20 declared parameters are wired to the signal chain. `prevOrganGain`/`prevOrganModel` on `OtoVoice` are declared but unused — these are state variables, not declared parameters, so this is not a D004 violation. It is an unfinished feature (see critical bug below). |
| D005 | PASS | LFO1 rate floor enforced at 0.01 Hz (`std::max(0.01f, pLFO1Rate)`). Breath drift LFO hardcoded at 0.3 Hz (always active when Pressure > 0). Default Pressure = 0.2, so the engine breathes from the moment it is loaded, without requiring user input. |
| D006 | PASS | Mod wheel (CC1): parsed, routed to effCutoff (+5000 Hz at max). Aftertouch (channel pressure): parsed, routed to effPressure (+0.3) and effCutoff (+3000 Hz). Velocity: wired to D001 filter chain. Three expression inputs confirmed in signal chain. |

---

## Critical Bugs

### BUG-1: Organ Crossfade State is Dead Code
`OtoVoice` declares `prevOrganGain = 0.0f` and `prevOrganModel = -1`. The header comment says "Crossfade state for seamless organ switching." These variables are initialized in `OtoVoice::reset()` but never written or read in `renderBlock`. The crossfade logic was designed but not implemented. Switching organ models mid-performance produces a hard timbre transition rather than a smooth blend. This is not a D004 violation (they are not declared parameters) but it is a broken UX promise.

**Fix**: Either implement the crossfade (10-20ms linear blend between old and new model's partial amplitudes at model-switch time) or remove the variables and the comment to prevent user confusion.

### BUG-2: Melodica Fundamental is Unaliased
The Melodica model uses a naive linear sawtooth for partial 0: `2.0f * voice.partialPhases[p] - 1.0f`. At frequencies above ~C5 (523 Hz), with a 48kHz sample rate, the sawtooth generates significant aliasing (upper harmonics fold back into the audible spectrum). The remaining 7 Melodica partials use `fastSin`, so only the fundamental is affected — but the fundamental carries the most energy. This creates a harsh, gritty quality in the high register that is not characteristic of the physical instrument.

**Fix**: Replace with a PolyBLEP sawtooth or a differentially-computed form: `phase - round(phase)` + PolyBLEP correction at phase discontinuity.

### BUG-3: No Sustain Pedal (CC64)
The MIDI loop handles note-on, note-off, pitch wheel, channel pressure, and CC1. CC64 (sustain) is not parsed. Notes released while the sustain pedal is held will enter release phase immediately. This is particularly relevant for the Melodica and Sheng models where players may use sustain pedal for legato phrasing.

**Fix**: Add `else if (msg.isController() && msg.getControllerNumber() == 64)` in the MIDI loop; set a bool `sustainPedalDown`, convert pending note-offs to deferred release when pedal lifts.

---

## Concerns (Non-Bug)

**C-1: Chiff Spectral Homogeneity** — The chiff generator differentiates models by duration and noise-mix ratio but uses the same pitched component frequency (8× burst phase) for all four. Model-specific spectral shaping (different pitched component frequencies or noise bandpass) would create more acoustically distinct attacks.

**C-2: Competition Curve is Too Linear** — Real free-reed instruments at full polyphony experience near-exponential breath pressure reduction. The current linear formula is too gentle above 4 voices. Consider a squared or exponential reduction curve.

**C-3: Filter Envelope Not User-Accessible** — Hardcoded `setADSR(0.001f, 0.3f + (1.0f-vel)*0.5f, 0.0f, 0.3f)`. At minimum, filter decay should be exposed as a parameter (`oto_filterDecay`). This is one parameter that would meaningfully expand the engine's timbral palette.

**C-4: Default LFO Depth is 0.0** — New users encountering the LFO controls hear no response until they increase depth. Default of 0.05-0.10 would demonstrate the vibrato system immediately.

**C-5: Sheng Model Inharmonicity** — Integer harmonic ratios for the Sheng model miss the slight upper-partial inharmonicity of real reed acoustics. A subtle inharmonicity coefficient (0.0005 per partial index, scaled by partial number squared — the standard string-mode formula) applied to Sheng upper partials would distinguish its timbre from a pure sine-wave organ.

**C-6: Pitch-Based Pan vs. Voice-Index Pan** — Current pan spread is voice-allocation-index dependent, which produces spatially inconsistent polyphony as voices are stolen. Pitch-proportional panning would be musically more coherent.

**C-7: Silence Gate Hold is Short for Max-Release Presets** — 500ms hold with 8-second release on Sho presets risks audible gate closure during otherwise smooth tails.

---

## What is Already Exceptional

**The OtoBreathSource design** is the most convincing breath instability model in the fleet. Two LFOs (drift + tremolo) at different rates, both scaled continuously by a user-visible Pressure parameter, per voice. Not approximated at note-on, not a single global LFO — per voice, continuous, with independent phases. This is what separates a synthesized organ from a musical instrument.

**The Cluster Density implementation** with fractional partial fade-out is technically correct and musically expressive — a single knob that transitions continuously from a monophonic flute to a full 11-voice cluster without artifacts.

**The Crosstalk system** (adjacent voice leakage, one-block delay) is novel in this fleet and captures pipe-proximity acoustic bleed with almost no CPU cost.

**The per-model attack shaping** at note-on is the right UX design: one parameter, four instrument-appropriate interpretations.

**The Cultural Specificity** of the ratio tables is real — non-harmonic Sho aitake intervals, deliberately detuned Khene pairs, the Melodica sawtooth fundamental — these represent research-grounded instrument modeling, not generic organ synthesis.

---

## Scoring

| Dimension | Score | Reasoning |
|-----------|-------|-----------|
| Sound Quality | 7.8 | Breath instability and cluster density are beautifully realized. Melodica aliasing and Sheng harmonic genericness hold it below 8.0. Crosstalk and chiff add genuine physical texture. |
| Innovation | 8.0 | Four-instrument model with shared parameter language, breath instability per voice, competition adversarial coupling, and crosstalk voice leakage are all architecturally novel in context. Nothing here has been done in this fleet. |
| Playability | 7.2 | 20 parameters is lean and navigable. Missing sustain pedal is a real gap. LFO depth defaulting to zero frustrates discovery. Organ-model crossfade being unimplemented means live switching is abrupt. |
| Preset Coverage | 7.5 | 10 presets across 8 moods (missing Crystalline, no Family preset) is thin but deliberate. All four instrument models appear. Each preset demonstrates a distinct dimension of the engine. DNA values are accurately set. No duplicates. Tagged correctly. |
| DSP Efficiency | 8.5 | 8 voices × 11 partials = 88 oscillators per block. fastSin used throughout. 8 parameter smoothers. Silence gate reduces CPU when idle. ParameterSmoother prevents click artifacts without wasting bandwidth. VoiceAllocator and shared DSP utilities kept at fleet standard. No denormal risk visible in the signal path (sustain values never reach zero in active voices). |
| Musical Utility | 8.2 | The Sho model alone justifies the engine's existence in modern music production — there is no credible alternative for this timbre in a plugin format. Melodica and Khene cover territory no other fleet engine covers. Sheng fills a bright, melodic organ role. The coupling types (AmpToFilter, LFOToPitch, AmpToPitch, EnvToMorph) are well-chosen for cross-engine use. |
| Identity / Character | 8.8 | "The Breath Between Worlds" is a precise identity statement. The engine knows what it is: a disciplined minimalist with four cultural voices. The Bamboo Green accent color, the Chef Quad framing, and the cookbook companion all reinforce a coherent product identity. No ambiguity about what this engine is for. |
| **Overall** | **8.0** | A strong debut for the Chef Quad. The DSP is solid, the identity is clear, the musical utility is high. Three bugs (dead crossfade code, Melodica aliasing, missing sustain pedal) and three significant concerns (filter envelope locked, LFO depth default, competition curve) prevent a higher score. Post-retreat with targeted fixes should reach 8.6+. |

---

## Consensus Verdict

The council agrees: XOto is a specialized instrument done with genuine care. The core synthesis — breath instability, cluster density, model-specific attack shaping, the Sho aitake ratios — is research-grounded and musically authentic in a way that distinguishes it from the casual "ethnic instrument" additions that plague many plugin collections.

The fleet average of ~8.7 is not reached because of three specific failures: Melodica aliasing (a fixable bug), the unimplemented organ crossfade (a broken promise), and the missing sustain pedal (a MIDI contract gap). Fix these three and raise the default LFO depth to 0.08, and the score moves to approximately 8.5. A full Guru Bin retreat with 10 carefully crafted awakening presets that explore the full breath instability range — slow-drift Sho pads, aggressive Khene drones, intimate Melodica songs — would demonstrate the engine's range in a way the current 10 presets only partially accomplish.

Moog and Ciani were most animated — Moog by the breath model, Ciani by the crosstalk. Scott and Buchla raised the sharpest concerns. Tomita found the cluster density implementation elegant. Carlos drew the cultural lineage clearly.

**The OtoBreathSource and Cluster Density systems are Blessing candidates.** The Crosstalk system should be documented as a novel intra-engine coupling mechanism for the fleet reference.

This is an engine that will surprise players who expect it to be a simple "Asian flute" preset bank. It is not that. It is a breath-physics instrument with genuine acoustic research behind its constants, four distinct cultural personalities, and a parameter language spare enough to master quickly. It earns its place in the Chef Quad.

---

## Blessing Candidates

### BC-OTO-01: The OtoBreathSource
Per-voice continuous breath instability via two internal LFOs (0.3 Hz drift + 5.5 Hz tremolo), continuously scaled by the Pressure parameter, producing simultaneous pitch drift (±8 cents max) and amplitude tremolo (±15% max). Not approximated at note-on: the drift persists and evolves through the note's lifetime. When Pressure > 0, the engine is alive even before MIDI arrives. This is the most convincing model of human breath-controlled instrument physics in the fleet.

### BC-OTO-02: Cluster Density Partial Sweep
Integer-count partial activation from 1 to N, with the last partial crossfaded by fractional density value. Continuous, click-free sweep from monophonic to full polyphonic cluster using one knob. Output normalized by sqrt(partialCount) to maintain consistent amplitude. Demonstrated in three preset contexts (Sho, Sheng, Khene) with distinct acoustic results from the same control. The technical implementation is correct and the musical result is expressive.

### BC-OTO-03: Adversarial Breath Competition
Polyphony-scaled amplitude reduction (`competitionScale = 1 - effCompetition * (voiceCount-1) / kMaxVoices * 0.6`) that reduces each voice's output as more voices play simultaneously. Models the physical reality of shared breath pressure in free-reed aerophone instruments. Creates natural dynamic compression in dense polyphony that no post-processing can accurately reproduce, because the compression is voice-count-dependent rather than amplitude-dependent.

---

## Post-Retreat Target
With three bug fixes + 10 retreat presets + filter decay parameter added: **8.6/10**

---

*Council convened 2026-03-21. Chaired by the Ringleader. All eight ghosts present and speaking.*
