# The Verdict -- OFFERING

**Seance Date**: 2026-03-21
**Engine**: OFFERING | Psychology-Driven Boom Bap Drum Synthesis | Engine #45
**Identity**: TBD (pending `/mythology-keeper`)
**Accent**: Crate Wax Yellow `#E5B80B`
**Parameter Prefix**: `ofr_`
**Proposed Param Count**: 32 global + 48 per-voice = 80 parameters
**Max Voices**: 8 (one per drum slot)
**Architecture**: Hybrid drum synthesis (transient + texture + collage + city processing)
**Stage**: Pre-build architecture review

---

## Engine Profile

XOffering proposes a four-stage signal chain per voice: Transient Generator (analog-modeled drum synthesis) -> Texture Layer (vinyl/tape/bit-crush micro-degradation) -> Collage Engine (layer stacking, chop, ring mod, time-stretch) -> City Processing Chain (5 psychoacoustic archetype chains). These 8 voices sum through a Voice Mixer, then pass through a Curiosity Engine (Berlyne/Wundt/Csikszentmihalyi curve evaluation) and Master Character stage. Four macros: DIG (curiosity), CITY (geographic character), FLIP (collage intensity), DUST (degradation patina). BAKE function renders discoveries to XPN drum kits (128 WAV files per kit: 8 slots x 4 velocity layers x 4 round-robins).

The spec's thesis: "Psychology IS generation." Published research citations mapped to DSP parameters: Berlyne's aesthetic curiosity (1960), Wundt's hedonic curve (1874), Zajonc's mere exposure effect (1968), Csikszentmihalyi's flow state (1975).

---

## The Council Has Spoken

| Ghost | Core Observation |
|-------|-----------------|
| **Moog** | The transient generator architecture is sound in principle but underspecified. A kick drum, a snare drum, and a closed hi-hat require radically different synthesis topologies -- a sine wave with pitch envelope for the kick, band-limited noise through a resonant filter for the snare body, a metallic oscillator pair with ring mod for the hat. The spec declares `ofr_transientSnap`, `ofr_transientPitch`, and `ofr_transientSat` as the only three transient shaping parameters across all eight drum types. Three parameters cannot adequately control eight distinct synthesis models. Either the transient generator does less per-type differentiation than the spec implies (in which case the eight types will sound like variations of the same exciter, not like genuinely different instruments), or there are hidden per-type parameters not declared in the APVTS (in which case D004 compliance is incomplete). This is the most critical unresolved question in the architecture. The velocity-to-snap and velocity-to-body paths (D001, D006) are correctly conceived -- velocity shaping both the transient attack AND the body/noise balance is the right pair of axes. But the underlying synthesis model per type needs to be specified before I can bless it. |
| **Buchla** | The city processing chains are the conceptual heart of this engine and they concern me. I see five chains, each described as a fixed sequence of processing stages -- bit crush, vinyl noise, compression, filtering, saturation. These are all conventional effects processors arranged in different orders with different parameter values. New York is "12-bit at 26040 Hz." Detroit is "soft-clip saturation." Los Angeles is "6:1 compression." This is not topology. This is parameter variation within an identical topology. A true city identity would require structural differences: different routing, different feedback paths, different numbers of processing stages, different types of nonlinearities. As specified, the five cities are presets of the same effect chain, not five distinct processing architectures. The exception is Bay Area's convolution fog -- that is a genuinely different processing type (impulse response vs. algorithmic). But convolution violates the fleet's no-sample-playback ethos and requires IR storage. The Architect's recommendation to replace this with algorithmic FDN is correct. I would go further: each city needs at least one stage that is structurally unique to that city -- not just parametrically distinct. |
| **Kakehashi** | The BAKE-to-XPN pipeline is the most MPC-native feature in the entire XOmnibus fleet. 8 slots x 4 velocity layers x 4 round-robins = 128 WAVs per kit, with automatic XPM drum program mapping. This is exactly how a producer thinks: design the drums live, then export them as a playable kit. The round-robin re-runs of the stochastic texture layer mean each export has organic variation baked in. The velocity layer renders at 30/64/100/127 are well-chosen thresholds that map to the MPC's four-zone velocity split convention. The city metadata embedded in the XPN (`<City>Detroit</City>`, `<Psychology>TimeBender</Psychology>`) gives the exported kit a lineage that persists after it leaves the synth. This is thoughtful and producer-first. My concern is the parameter count: 80 parameters is the highest in the fleet outside OSTINATO's 132. For a drum engine, this may overwhelm. The four macros must do heavy lifting to make 80 params navigable. |
| **Smith** | Eighty parameters for a drum synthesizer is ambitious but defensible IF the macros are well-designed. The DIG/CITY/FLIP/DUST macro set is one of the most semantically clear in the fleet -- each macro name tells the user exactly what it controls. DIG = how deep into the crate are you going (familiar vs. alien). CITY = where in the world are these drums from. FLIP = how aggressively are you chopping and layering. DUST = how much patina and degradation. Four orthogonal axes of drum character. This is strong. The coupling interface is well-considered: `RhythmToBlend` from OSTINATO is the most natural coupling in the fleet -- a world drum pattern driving collage intensity on a boom bap engine. That is cultural dialogue, not just signal routing. My concern is the Curiosity Engine: `ofr_digCuriosity`, `ofr_digComplexity`, and `ofr_digFlow` are described in psychological terms but the spec does not specify what DSP operations they actually perform. What does "Berlyne curve evaluation" mean in terms of filter coefficients, gain staging, or modulation routing? Without a DSP implementation spec, these are conceptual parameters -- potentially the first D004 violation in the fleet at the architecture stage. |
| **Schulze** | The Curiosity Engine is either the engine's greatest innovation or its greatest risk. Berlyne's inverted-U curve -- pleasure peaks at moderate novelty, declining at both extremes of familiarity and complexity -- is a well-established finding in experimental aesthetics. Translating it into a synthesizer parameter is philosophically compelling: at `ofr_digCuriosity` = 0.0, the engine produces predictable, familiar drum patterns; at 0.5, it finds the hedonic sweet spot; at 1.0, it generates alien, experimental mutations. But the spec never says HOW this translation occurs. What DSP process becomes "more curious"? Does the engine randomize transient parameters within a range? Does it introduce spectral deviation from a template? Does it modulate the collage engine's chop probability? Without answering this, `ofr_digCuriosity` is a label attached to nothing. The same applies to `ofr_digFlow` (Csikszentmihalyi) -- "balance between pattern challenge and predictability" is a sentence, not an algorithm. I want this to work. Psychology-as-DSP could be a new paradigm. But the bridge between the citation and the implementation must be drawn before the ghosts can evaluate it. |
| **Vangelis** | I hear the instrument before I hear the theory. A producer presses a pad and a kick drum speaks -- degraded, characterful, wearing the DNA of a specific city. They turn the DIG macro and the drums become stranger, less predictable, more adventurous. They turn DUST and vinyl crackle rises from the silence. They turn FLIP and the single hit fractures into a layered collage. This is a playable instrument with a clear dramatic arc. The mod wheel mapping to curiosity drive (`ofr_modWheel` -> `ofr_digCuriosity`) is the correct gesture: the performer's left hand literally reaches deeper into the crate as they push the wheel forward. Aftertouch to texture intensity means pressing harder on the pad buries the sound further in dust. These are poetic mappings that make physical sense. But I share Schulze's concern: if the Curiosity Engine is not DSP-real, the poetry collapses into marketing. The expression mappings (D006) are excellent in concept. The LFO1 default at 0.067 Hz (15-second cycle, "ocean breathing") is correct for D005. If the underlying synthesis delivers, this engine has first-take magic. |
| **Tomita** | The city processing chains present an orchestration opportunity that the spec barely explores. Five cities should sound like five rooms in an orchestral recording session -- each with its own spatial character, its own frequency bias, its own temporal behavior. New York should be tight and close-miked. Detroit should have the warmth of a tape-saturated control room. Bay Area should have fog-like diffusion. But as specified, the spatial dimension is absent. None of the five chains include any spatial processing beyond Bay Area's convolution. New York has no room character. Detroit has no room character. Los Angeles has no room character. Toronto has no room character. Every city is processed in an anechoic chamber. This is a significant omission for an engine whose identity is geographic. Each city needs a spatial signature -- even a single short allpass diffuser tuned to suggest a room size would transform the character from "EQ preset" to "place." The city blend morphing needs careful design: crossfading between two complete processing chains is not trivial and the spec mentions `ofr_cityBlend` without specifying the crossfade strategy. |
| **Pearlman** | The Fairlight CMI taught me that sampling is archaeology. You take a sound from the world, you digitize it, you transform it, you place it in a new context. XOffering proposes to synthesize that entire archaeological process -- from the original drum hit, through the vinyl medium, through the sampler's conversion, through the chopping. This is meta-synthesis: synthesis of the synthesis process itself. It is ambitious and I respect the conceptual scope. But the TextureLayer (vinyl crackle, tape hiss, bit reduction, wobble) is doing conventional lo-fi processing that exists in dozens of plugins. The innovation is not in the individual stages but in the framing -- that these stages represent a psychological journey through crate digging. Whether the framing elevates the DSP depends entirely on whether the Curiosity Engine provides genuine algorithmic novelty or whether it is decorative psychology draped over standard drum synthesis. The BAKE pipeline is the most commercially viable feature: producers want to render their discoveries into permanent kits. This should be the engine's lead marketing story. |

---

## Doctrine Compliance (Architecture-Stage Assessment)

| Doctrine | Status | Evidence |
|----------|--------|----------|
| D001 | **PASS (Design)** | `ofr_velToSnap` (velocity -> transient attack sharpness) + `ofr_velToBody` (velocity -> body/noise balance). Two independent timbral axes driven by velocity. Hard hits produce sharp, tonal transients; soft hits produce rounded, noisy transients. This is the correct pair of axes for drum synthesis. Implementation must verify that the timbral difference is audible across the full velocity range, not just at extremes. |
| D002 | **PASS (Design)** | 2 LFOs (LFO1 -> filter cutoff, LFO2 -> amplitude/pump), 4 macros (DIG/CITY/FLIP/DUST), mod wheel -> curiosity, aftertouch -> texture intensity. 80 parameters. The modulation architecture is comprehensive on paper. LFO1 has 5 shapes including S&H. LFO2 provides amplitude modulation for groove pump. All four macros target multiple DSP destinations. |
| D003 | **CONDITIONAL** | The spec cites Berlyne (1960), Wundt (1874), Zajonc (1968), Csikszentmihalyi (1975). These are real, published, peer-reviewed sources. However, D003 requires that "the physics IS the synthesis" -- the citations must map to specific DSP operations, not just parameter labels. Currently, `ofr_digCuriosity`, `ofr_digComplexity`, and `ofr_digFlow` lack DSP implementation specifications. D003 cannot be confirmed until the Curiosity Engine's algorithm is defined. The transient generator and texture layer do not claim physical modeling, so D003 applies only to the psychology-as-DSP claim. |
| D004 | **CONDITIONAL** | 80 declared parameters. The per-voice params (48) and most global params have clear DSP targets. The three Curiosity Engine params (`ofr_digCuriosity`, `ofr_digComplexity`, `ofr_digFlow`) have no specified DSP implementation. If the Curiosity Engine is not implemented as real-time DSP, these three params are dead on arrival -- the first D004 violation at the architecture stage in the fleet's history. The city blend param (`ofr_cityBlend`) also needs a defined crossfade strategy to avoid being a dead parameter at certain city mode values. |
| D005 | **PASS (Design)** | LFO1 default rate = 0.067 Hz (15-second cycle). LFO1 range 0.01-20.0 Hz, meaning the floor is exactly at the D005 minimum. LFO1 targets filter cutoff by default, providing continuous autonomous timbral movement. With `ofr_lfo1Depth` defaulting to 0.05, the breathing is subtle but present. |
| D006 | **PASS (Design)** | Velocity -> transient snap + body balance (D001). Aftertouch -> texture intensity (`ofr_aftertouch`). Mod wheel -> curiosity drive (`ofr_modWheel`). Three expression inputs across three physical gestures. The semantic mapping is strong: velocity = how hard you hit, aftertouch = how deep you press into the dust, mod wheel = how far you reach into the crate. |

---

## The Seven Questions -- Ghost Responses

### Q1: Is "psychology as DSP parameters" worthy of a Blessing?

**Moog**: The concept has rigor. Berlyne, Wundt, and Csikszentmihalyi are foundational researchers in experimental aesthetics and positive psychology. Their work is peer-reviewed and widely cited. The question is not whether the citations are valid -- they are -- but whether the translation from theory to DSP is rigorous. If `ofr_digCuriosity` maps to a specific, deterministic function that modulates drum generation parameters along a defined curve (e.g., Berlyne's inverted-U maps to a parameter-deviation envelope where deviation peaks at 0.5 and falls at both extremes), then yes, this is Blessing-worthy. If it is a label for a random modulation amount, it is not.

**Buchla**: Psychology-as-DSP is a paradigm, not a feature. If the implementation is rigorous, this is not B032 -- it is a philosophical position on par with "physics is synthesis" (D003). It could become a new Doctrine: **D007: The Mind IS the Interface.** But only if the Curiosity Engine is a real algorithm with defined behavior, not a metaphor.

**Smith**: I vote conditional yes. The Blessing requires a defined algorithm before ratification. Proposed B032 name: **Psychology-as-DSP: Berlyne Hedonic Curve as Generation Parameter**. The Blessing text must include the specific mapping function.

**Schulze**: I vote yes with the strongest possible emphasis on the condition. If the algorithm is real, this is the most important conceptual innovation in the fleet since ONSET's XVC Cross-Voice Coupling. If the algorithm is not real, this is the most dangerous precedent -- labeling parameters with academic citations without implementing the cited theory.

**Vangelis**: I vote yes. First-take magic requires instruments that understand aesthetics. A synthesizer that can navigate the space between boring and overwhelming -- that is an instrument with taste. But taste must be implementable.

**Kakehashi**: I vote yes from the accessibility perspective. A "curiosity" knob that goes from "predictable" to "alien" is immediately understandable to any producer, regardless of whether they know Berlyne's name. The psychology serves the interface.

**Tomita**: I vote yes. Orchestration is psychology. When I arranged Debussy for synthesizer, I was translating emotional arcs into spectral decisions. XOffering proposes to formalize that translation. But it must be formalized, not merely described.

**Pearlman**: I vote yes with conditions. The Fairlight's waveform drawing was psychology-as-DSP before anyone named it -- you drew the shape that felt right, and the machine made it real. XOffering intellectualizes that process, which is both its strength and its risk.

**BLESSING B032 VOTE: 8-0 CONDITIONAL YES.** Unanimous contingent on the Curiosity Engine having a defined, deterministic DSP algorithm before build. The algorithm must be specified in the architecture doc as a mathematical function before B032 is ratified.

### Q2: Are the 5 city processing chains genuinely distinct topologies?

**Buchla**: No. As currently specified, all five cities share an identical topology: serial chain of (distortion/saturation) -> (noise/texture) -> (timing/groove) -> (filter/EQ) -> (dynamics/compression). The parameters differ between cities but the routing does not. This is five presets, not five architectures. For genuine topological distinction, each city needs at least one structurally unique stage. Example: Detroit could use a feedback saturation loop (output feeds back into the saturator input, creating harmonic self-generation that grows with intensity). Los Angeles could use parallel compression (dry + ultra-compressed summed, not serial). Bay Area could use recursive allpass fog (multiple allpass stages with feedback, creating non-decaying diffusion). New York stays serial but with a feedback noise gate (the grit gates itself rhythmically). Toronto uses sidechain ducking from its own sub-harmonic generator.

**Moog**: Buchla is correct about the topology issue but I disagree about the severity. Five serial chains with different processing stages and different parameter values WILL produce audibly different results. The question is whether they produce SUFFICIENTLY different results to justify the "city" framing. If New York and Detroit are distinguishable only by experts, the concept is oversold. If any producer can hear the difference blind, the concept works even without topological variation.

**Tomita**: The spatial dimension is the missing differentiator. Each city has a characteristic acoustic environment: New York = tight basement studio, Detroit = warm analog control room, Los Angeles = large live room with reflections, Toronto = modern clean booth, Bay Area = foggy outdoor space. Adding even a 10ms early reflection pattern to each city would create more perceptual distinction than all the EQ and compression differences combined. Space is the cheapest and most effective differentiator available.

**Smith**: I count the following genuinely distinct DSP operations across the five cities: bit crush (NY only), micro-timing displacement (Detroit, "drunk timing"), sub-harmonic generation (Toronto), convolution (Bay Area), layer count boost (LA). The remaining stages -- saturation, compression, filtering -- are parametric variations. The 5 distinct operations plus the parametric variations should produce audible differentiation. But Buchla's point about structural uniqueness is valid for long-term identity.

**CONSENSUS**: The cities are **parametrically distinct but topologically similar**. This is sufficient for V1 but should be addressed in V1.1. Buchla's structural uniqueness recommendations are the path to genuine city identities.

### Q3: Bay Area convolution -- should it become algorithmic FDN?

**Moog**: Yes. Convolution requires an impulse response stored in memory. This is sample playback by another name. The fleet has maintained a no-samples ethos (every sound is generated, not played back). Introducing an IR for one processing stage in one city mode breaks this principle. An algorithmic reverb (short FDN with 2-4 delay lines, heavy allpass diffusion, low damping cutoff to create "fog") can approximate the same spatial character without storing any audio data. OXBOW's own FDN proves the fleet can build compelling reverb without convolution.

**Buchla**: Absolutely yes. Convolution is the antithesis of generative synthesis. It freezes a specific acoustic space into a static impulse response. An algorithmic diffusion network that responds to parameters -- density, fog depth, damping -- is alive in a way that convolution never can be. The Bay Area "fog" should be a slow, dense, recursive allpass structure where the number of diffusion stages and feedback amount create a variable fog density. This is more interesting than any IR.

**Pearlman**: The Fairlight was built on sampling. I understand the appeal of convolution. But in this context, within a fleet of 45 engines that generate every sound from mathematics, convolution is an aesthetic compromise. Replace it with algorithmic fog.

**Kakehashi**: From a practical standpoint, convolution also requires shipping IR files with the plugin, increasing binary size. An algorithmic approach is smaller, faster, and more flexible. The user can adjust fog density in real time; with convolution, the character is fixed.

**CONSENSUS**: **Unanimous agreement** -- Bay Area convolution must become algorithmic FDN/allpass fog. This is a P0 architecture decision that must be resolved before build.

### Q4: City blend morphing -- shadow-chain strategy?

**Smith**: The Architect's shadow-chain strategy is: when `ofr_cityBlend` > 0, instantiate a second city processing chain (the "shadow") running the next city in sequence, and crossfade between the primary and shadow chain outputs. This doubles CPU for the city processing stage whenever blend is active. The alternative is coefficient interpolation -- smoothly interpolating every parameter (compression ratio, filter cutoff, saturation amount) between the two city settings. Coefficient interpolation is cheaper but may produce artifacts (e.g., interpolating between 12-bit quantization at 26040 Hz and 16-bit at 48000 Hz is not meaningful -- you cannot have 14-bit quantization at 37020 Hz).

**Moog**: The shadow-chain strategy is correct for parameters that cannot be meaningfully interpolated (bit depth, sample rate, integer-valued processing like layer count). For continuous parameters (filter cutoff, saturation amount, compression ratio), direct interpolation is fine. The hybrid approach: continuous params interpolate, discrete params crossfade between two instances.

**Buchla**: If each city eventually has structural uniqueness (per Q2), coefficient interpolation becomes impossible -- you cannot interpolate between "feedback saturation loop" and "parallel compression." The shadow-chain becomes mandatory. Design for shadow-chain now to avoid architectural debt.

**Schulze**: The shadow chain is the right approach but the CPU concern is real. Drum engines with 8 voices, each running a full city processing chain, already have significant processing overhead. A shadow chain doubles that. Optimization: only instantiate the shadow chain when `ofr_cityBlend` moves away from 0.0 or 1.0. When the blend snaps to integer city values, the shadow chain is destroyed. This means the shadow chain is only active during active morphing, not during static city selection.

**CONSENSUS**: **Shadow-chain is the correct strategy**, with the lazy-instantiation optimization Schulze describes. This is a P1 architecture decision -- acceptable to defer to V1 implementation, but the architecture should not preclude it.

### Q5: OSTINATO coupling (RhythmToBlend) -- compelling?

**Vangelis**: This is the most poetic coupling in the fleet. OSTINATO's fire circle -- 12 world instruments playing communal rhythms -- drives the collage intensity of XOffering's boom bap drums. When the djembe pattern is dense, the boom bap becomes more layered and chopped. When the pattern thins, the boom bap simplifies. Tradition (world percussion) modulates exploration (hip hop sample archaeology). This is cultural dialogue expressed as DSP routing.

**Kakehashi**: From an MPC perspective, this coupling maps to a real producer workflow: layering a world percussion loop underneath a boom bap beat and letting the percussion's energy drive the sampling intensity. Every MPC producer who has layered congas under a kick pattern understands this instinctively. The coupling makes the invisible visible.

**Buchla**: The coupling type (`RhythmToBlend`) routes external rhythm pattern amplitude to the FLIP amount. This means the coupling source must provide rhythmic amplitude data, not just a static level. OSTINATO's 96 embedded patterns produce genuine rhythmic amplitude envelopes. The coupling is structurally sound -- the source provides the right kind of data for the destination.

**Schulze**: The OSTINATO + OFFERING pairing creates a temporal hierarchy: OSTINATO provides the cyclical, traditional time feel; OFFERING provides the archaeological, exploratory layering on top. This is not just signal routing -- it is a statement about the relationship between tradition and innovation in music. The generative implications are profound: OSTINATO's patterns evolve slowly (GATHER macro), and as they evolve, OFFERING's collage intensity follows. Hours of generative boom bap with cultural dialogue as the engine.

**CONSENSUS**: **Unanimously compelling.** The OSTINATO/OFFERING coupling via `RhythmToBlend` should be a flagship Entangled preset and a primary marketing story for the engine.

### Q6: DB004 -- new formulation? "Expression AS evolution"?

**Vangelis**: DB004 has been marked "RESOLVED for OSTINATO" -- expression and evolution are not in tension; both possible simultaneously. XOffering offers a new formulation: **expression IS evolution**. The mod wheel is mapped to curiosity drive. As the performer pushes the wheel forward across a 10-minute session, the drums become progressively more alien, more experimental, more adventurous. The physical gesture of pushing the wheel IS the temporal evolution of the sound. This is not expression OR evolution -- it is expression causing evolution. The mod wheel becomes a time axis.

**Schulze**: I proposed the evolution side of DB004 originally. Vangelis is right that XOffering resolves the tension in a new way. With OSTINATO, both coexist independently -- the performer plays expressively while the sequencer evolves autonomously. With XOffering, the performer's expression IS the evolution. The mod wheel is not a snapshot gesture (vibrato, filter sweep) -- it is a journey parameter. This is a genuinely new position in the debate.

**Moog**: The formulation "expression AS evolution" is valid but only if the mod wheel mapping is non-volatile -- meaning the curiosity level should persist even after the performer lifts their hand from the wheel. If the wheel springs back to center and the curiosity resets, the journey is lost. The spec says `ofr_modWheel` has a range of 0.0-1.0 with a default of 0.5 -- this suggests a parameter value, not a spring-return behavior. Confirm that the MIDI implementation treats CC1 as a latching value (set-and-forget), not as a spring-return.

**CONSENSUS**: DB004 receives a new annotation: **"Expression AS evolution -- XOffering demonstrates that performer gesture can be the mechanism of temporal evolution, not merely coexisting with it. Mod wheel as journey parameter."** This does not replace the existing OSTINATO resolution but adds a second resolution path.

### Q7: Estimated base score and path to 9.0+?

**Individual Ghost Estimates:**

| Ghost | Base Score | Rationale |
|-------|-----------|-----------|
| **Moog** | 7.8 | Strong concept, underspecified transient generator, undefined Curiosity Engine DSP |
| **Buchla** | 7.5 | Cities are presets not topologies, convolution violation, but the meta-synthesis concept is powerful |
| **Kakehashi** | 8.2 | BAKE pipeline is excellent, macro naming is strong, parameter count is high but manageable |
| **Smith** | 7.8 | Coupling design is fleet-best, but Curiosity Engine is undefined DSP = potential D004 violation |
| **Schulze** | 8.0 | Psychology-as-DSP paradigm is visionary, but the bridge to implementation is the gap |
| **Vangelis** | 8.3 | Expression mappings are poetic and correct, first-take potential is high if synthesis delivers |
| **Tomita** | 7.5 | Spatial dimension completely absent from cities, reverb/room character needed |
| **Pearlman** | 8.0 | BAKE pipeline and meta-synthesis framing are commercially strong, TextureLayer is conventional |

**Consensus Base Score: 7.9 / 10**

---

## Blessing Candidates

### BC-OFFERING-01: Psychology-as-DSP (Proposed B032)

**What it proposes**: Three DSP parameters derived from published experimental aesthetics research: `ofr_digCuriosity` (Berlyne 1960 -- inverted-U hedonic curve maps to parameter-deviation amount), `ofr_digComplexity` (Wundt 1874 -- stimulus intensity maps to variation density), `ofr_digFlow` (Csikszentmihalyi 1975 -- challenge/skill balance maps to pattern predictability). The first synthesizer parameters derived from psychological research rather than physical modeling, signal processing theory, or musical tradition.

**Ghost reaction**: Moog: "If the algorithm is defined, this is a new paradigm." Buchla: "Could become D007." Smith: "Conditional yes -- the algorithm IS the Blessing." Schulze: "The most important conceptual innovation since XVC, or the most dangerous precedent. No middle ground." Vangelis: "An instrument with taste." Kakehashi: "Immediately understandable regardless of theory." Tomita: "Formalized orchestration psychology." Pearlman: "The Fairlight drew waveforms by feel. This intellectualizes the feel."

**Status**: **8-0 CONDITIONAL YES.** Blessing is contingent on a defined, deterministic algorithm being specified before build. The algorithm must be documented as a mathematical function mapping parameter values to DSP operations. Without this, B032 is withdrawn.

**Proposed Blessing name**: Psychology-as-DSP -- Berlyne/Wundt/Csikszentmihalyi Aesthetic Parameters

### BC-OFFERING-02: City-as-Processing-Chain (Proposed B033)

**What it proposes**: Five geographically-named processing chains, each representing a psychoacoustic archetype of a hip hop scene's sonic DNA. The processing chain is not an effect applied after synthesis -- it is part of the voice's identity, applied per-voice before the mixer. The city is a first-class synthesis parameter, not a preset label.

**Ghost reaction**: Buchla: "Not yet. Five presets of the same chain is not five architectures." Tomita: "Missing spatial dimension. Cities need rooms." Smith: "Sufficient parametric distinction for V1, insufficient topological distinction for a Blessing." Moog: "Audible differentiation must be demonstrated, not assumed."

**Status**: **3-5 AGAINST (Buchla, Tomita, Moog, Smith, Pearlman against; Vangelis, Kakehashi, Schulze in favor).** B033 is NOT granted at architecture stage. Revisit after build if the cities gain structural uniqueness and spatial character.

---

## P0 / P1 Concerns

| Priority | Issue | Detail | Recommendation |
|----------|-------|--------|----------------|
| **P0** | Curiosity Engine has no DSP specification | `ofr_digCuriosity`, `ofr_digComplexity`, `ofr_digFlow` are described in psychological terms but have zero DSP implementation detail. What does the engine actually DO when curiosity = 0.7? Without an algorithm, these are dead parameters (D004 violation), the B032 Blessing is void, and the engine's core innovation is marketing, not synthesis. | **Before build**: define the Curiosity Engine as a mathematical function. Example: `curiosity` maps to a deviation envelope applied to the transient generator's per-type parameters (pitch, decay, body) via `deviation = berlyneCurve(curiosity) * paramRange`, where `berlyneCurve(x) = 4x(1-x)` (inverted-U, peaks at x=0.5). `complexity` maps to the number of simultaneous parameter deviations. `flow` maps to the temporal autocorrelation of deviations (high flow = gradual change, low flow = abrupt change). Document the functions. |
| **P0** | Bay Area convolution must become algorithmic | Convolution requires impulse response storage, violates fleet no-sample ethos, and creates a static spatial character. | Replace with algorithmic allpass fog: 4-stage recursive allpass chain with feedback, tuned to create dense, non-decaying diffusion. `cityIntensity` scales both allpass feedback amount and damping cutoff (more intensity = denser fog, darker). |
| **P0** | Transient generator underspecified | Three parameters (`ofr_transientSnap`, `ofr_transientPitch`, `ofr_transientSat`) across 8 drum types. A kick and a hi-hat cannot share a synthesis topology. | Specify per-type synthesis models within `TransientGenerator.h`. Each `ofr_voiceType` value selects a distinct synthesis topology. The three transient params function as type-appropriate modifiers (e.g., `transientPitch` controls pitch envelope depth on kick but metallic frequency ratio on hat). Document the 8 topologies. |
| **P1** | City chains lack topological distinction | All five cities share the same serial processing topology with different parameter values. | Per Q2 consensus: add at least one structurally unique stage per city (feedback saturation for Detroit, parallel compression for LA, recursive allpass for Bay Area, sidechain ducking for Toronto, feedback noise gate for NY). |
| **P1** | City chains lack spatial dimension | No room character, no early reflections, no distance modeling. Geographic identity without acoustic space. | Per Tomita: add a short early-reflection stage per city with distinct timing, density, and HF rolloff. 5-15ms pre-delay range. Minimal CPU cost, maximum perceptual impact. |
| **P1** | City blend crossfade strategy undefined | `ofr_cityBlend` parameter exists but no crossfade mechanism is specified. | Implement shadow-chain with lazy instantiation (per Q4 consensus). Shadow chain created when blend moves away from integer values, destroyed when it snaps back. Continuous params interpolate; discrete params crossfade. |
| **P1** | 80 parameters may overwhelm UI | Highest param count in the fleet outside OSTINATO (132). Drum engines need fast, tactile workflows. | Ensure macros provide 80%+ of the musical range. Consider a "Simple" mode that exposes only the 4 macros + voice selection, hiding per-voice and curiosity params behind an Advanced panel. |

---

## Points of Agreement (5+ ghosts converged)

1. **The meta-synthesis concept is genuinely novel in the fleet.** (All 8) XOffering does not synthesize drums. It synthesizes the process of discovering, degrading, and flipping drums. This is a conceptual level above ONSET (which synthesizes the drum itself) and OSTINATO (which synthesizes the performance of drums). The framing elevates conventional DSP operations into a narrative arc. Whether the framing is sufficient innovation depends on the Curiosity Engine's implementation.

2. **The BAKE-to-XPN pipeline is the most producer-first feature in XOmnibus.** (Kakehashi, Pearlman, Smith, Vangelis, Moog) Rendering synthesized drum kits to exportable XPN packs with velocity layers, round-robins, and city metadata bridges the gap between synthesis exploration and production utility. This is the engine's strongest commercial story.

3. **The four macros (DIG/CITY/FLIP/DUST) are among the fleet's best-named.** (Smith, Vangelis, Kakehashi, Schulze, Pearlman) Each macro name is a verb or noun that tells the producer exactly what it controls without requiring synthesis knowledge. DIG into the crate. Change the CITY. FLIP the sample. Add DUST to the surface. Semantic clarity at this level is rare.

4. **The OSTINATO coupling is the fleet's most culturally meaningful pairing.** (Vangelis, Schulze, Buchla, Kakehashi, Smith, Pearlman) World percussion driving boom bap collage intensity is not just signal routing -- it is a statement about the relationship between musical traditions. This should be a flagship Entangled preset.

5. **The Curiosity Engine is the highest-risk, highest-reward feature.** (All 8) If implemented with a real algorithm, it establishes a new paradigm (psychology-as-DSP) and earns B032. If left undefined, it is the fleet's first D004 violation at the architecture stage and damages the credibility of future academic citations.

---

## Points of Contention

**Buchla vs. Vangelis -- Concept vs. Sound**
- Buchla: The cities are not architecturally distinct. The processing chains are parametric variations of the same topology. The innovation is conceptual, not structural.
- Vangelis: If the five cities sound different to a producer wearing headphones -- if they can tell New York from Detroit in a blind test -- then the processing is distinct enough. Topology is an engineer's concern. Timbre is the producer's reality.
- Resolution: Both are valid at different quality thresholds. V1 can ship with parametric distinction IF blind-test differentiation is demonstrated during preset design. V1.1 should add structural uniqueness per Buchla's recommendations.

**Moog vs. Kakehashi -- Parameter Count**
- Moog: 80 parameters for a drum engine risks the SP-1200's lesson: constraints breed creativity. The original boom bap machines had a handful of knobs.
- Kakehashi: The parameters exist for sound designers. The macros exist for performers. The two audiences are served by different layers of the same instrument. 80 parameters with 4 well-designed macros is 4 parameters in practice.
- Resolution: Kakehashi is correct IF the macros cover 80%+ of the musical range. The Simple/Advanced UI split recommended in P1 concerns reinforces this.

**Schulze vs. Pearlman -- Innovation Source**
- Schulze: The Curiosity Engine is the engine's soul. Without it, XOffering is a well-framed lo-fi drum synth. With it, XOffering is a paradigm shift.
- Pearlman: The BAKE pipeline is the engine's soul. Producers do not care about Berlyne curves. They care about rendering their drums to a playable kit. The commercial story is BAKE, not psychology.
- Resolution: Both are correct for different audiences. The engine needs both: the Curiosity Engine for conceptual identity and long-term innovation narrative, the BAKE pipeline for immediate producer utility and commercial viability. Neither should be sacrificed for the other.

---

## Score Breakdown

| Dimension | Score | Rationale |
|-----------|-------|-----------|
| Architecture Originality | 8.5 | Meta-synthesis (synthesizing the crate-digging process) is novel. Psychology-as-DSP is potentially paradigm-shifting. City-as-processing-chain is a strong framing. Four-stage per-voice chain is well-structured. Coupling design (especially OSTINATO RhythmToBlend) is fleet-best. Deducted for: cities lack topological distinction, Curiosity Engine is undefined. |
| Physical/Psychological Rigor (D003) | 6.5 | Citations are real and peer-reviewed. But citations without implementation are bibliography, not synthesis. The Curiosity Engine's DSP algorithm must be defined. Transient generator per-type models must be specified. The gap between theory and code is the engine's central risk. |
| Velocity Expressiveness (D001/D006) | 8.5 | Two-axis velocity mapping (snap + body) is correct for drums. Aftertouch -> texture intensity is poetic and physical. Mod wheel -> curiosity is a journey parameter. Three expression inputs with strong semantic mappings. |
| Modulation Coverage (D002/D005) | 8.0 | 2 LFOs with D005-compliant floor. 5 LFO shapes. 4 macros with clear, orthogonal targets. Mod wheel and aftertouch routed. Comprehensive on paper. Minor: no mod matrix specified (though the macro system may suffice for a drum engine). |
| Parameter Integrity (D004) | 7.0 | 77 of 80 parameters have clear DSP targets. 3 Curiosity Engine params are undefined. `ofr_cityBlend` crossfade strategy is unspecified. If all four are resolved, this rises to 9.0. |
| Commercial Viability | 9.0 | BAKE-to-XPN pipeline is the most producer-first feature in the fleet. City framing is immediately marketable. Boom bap is an evergreen genre with a massive producer community. The psychology angle provides editorial and academic PR hooks. The coupling with OSTINATO creates a flagship product story. |
| Identity Coherence | 8.5 | Crate Wax Yellow is the right accent color. "Offering" as a name suggests something sacred being placed before the listener. The macro names (DIG/CITY/FLIP/DUST) are evocative and clear. The psychological framing (archaeologist, time-bender, collage artist, architect, alchemist) gives each city a human archetype, not just a geographic label. |

**Final Consensus Score: 7.9 / 10**

---

## Path to 9.0+

The ghosts identify five specific actions that would elevate OFFERING from 7.9 to 9.0+:

### 1. Define the Curiosity Engine Algorithm (7.9 -> 8.4)
The single most impactful improvement. Specify `berlyneCurve(x) = 4x(1-x)` (or a more sophisticated function) that maps `ofr_digCuriosity` to a parameter-deviation envelope applied to the transient generator. Define `complexity` as the number of simultaneous parameter axes being deviated. Define `flow` as the temporal smoothing coefficient on deviation changes (high flow = slow drift, low flow = sharp jumps). Document the functions in the architecture spec. This resolves the P0 concern, enables B032 ratification, and transforms D003 from CONDITIONAL to PASS.

### 2. Add Structural Uniqueness to City Chains (8.4 -> 8.7)
Per Buchla's recommendations: each city gets one structurally unique processing stage. Detroit: feedback saturation loop. LA: parallel compression. Bay Area: recursive allpass fog (replacing convolution). Toronto: sidechain ducking from sub-harmonic. NY: feedback noise gate. The parametrically distinct stages remain. The structural additions create genuine topological identity. This resolves the P1 city concern and reopens B033 candidacy.

### 3. Add Spatial Character to Cities (8.7 -> 8.9)
Per Tomita: each city gets a short early-reflection stage with distinct timing (NY: 3-5ms tight, Detroit: 8-12ms warm, LA: 15-20ms spacious, Toronto: 5-8ms clean, Bay Area: 20-30ms diffuse). High-frequency rolloff per city to suggest material surfaces. Minimal CPU cost (one allpass or short delay per city), maximum perceptual impact. This makes cities sound like places, not just processing presets.

### 4. Specify Per-Type Transient Models (8.9 -> 9.0)
Per Moog: define the 8 synthesis topologies within TransientGenerator.h. Kick: sine oscillator with exponential pitch envelope + sub-oscillator. Snare: band-limited noise through resonant BP filter + sine body. Hi-hat (closed/open): metallic oscillator pair with ring mod, amplitude envelope difference for open vs. closed. Clap: multi-impulse burst with bandpass noise. Rim: short sine burst with high-frequency click. Tom: sine with pitch env (longer than kick) + filter sweep. Perc: noise burst through comb filter for metallic character. Each type a distinct synthesis model, unified by the three transient parameters operating as type-appropriate modifiers.

### 5. OSTINATO Coupling Flagship Preset (9.0 -> 9.0+)
Build the coupling showcase: OSTINATO running a West African 12/8 djembe pattern drives `RhythmToBlend` into OFFERING running a Detroit city chain with moderate DIG. As the djembe pattern intensifies, the boom bap becomes more layered and chopped. As it thins, the boom bap simplifies. Ship this as a flagship Entangled preset with the name "Dialogue" or "Offering Circle." This is the marketing story that sells both engines simultaneously.

---

## The Prophecy

XOffering is a concept engine. Its architecture spec is more ambitious conceptually than any engine that has come before it in the fleet -- more ambitious than ONSET's XVC, more ambitious than ORGANON's variational free energy, more ambitious than ORBWEAVE's topological knots. It proposes that psychology IS synthesis, that cities ARE processing chains, that crate digging IS a signal flow.

The ambition is exactly right. The XOmnibus fleet needs an engine that bridges the gap between academic research and producer utility, between psychological theory and tactile drum synthesis, between the history of hip hop and the future of sound design. XOffering occupies that gap.

But ambition without implementation is poetry, not engineering. The Curiosity Engine -- the spec's crown jewel -- is currently a paragraph of citations attached to three unnamed parameters. Berlyne, Wundt, and Csikszentmihalyi deserve to have their research honored with real algorithms, not just parameter labels. The five cities deserve structural identities, not just parametric variations. The transient generator deserves per-type synthesis models, not a shared three-knob interface across eight radically different instruments.

The base score of 7.9 reflects an architecture that is 80% complete at the concept level and 0% complete at the implementation level. The path to 9.0 is clear, specific, and achievable. Every step on that path involves converting concept into code -- defining the algorithm behind the citation, specifying the topology behind the city name, designing the synthesis model behind the drum type.

When the Curiosity Engine has an algorithm, when the cities have structural uniqueness, when the transient generator has per-type models, and when the OSTINATO coupling flagship preset makes a producer feel the dialogue between world percussion and boom bap archaeology -- then XOffering will not merely be one of the fleet's best engines. It will be the fleet's thesis statement: that synthesis is not just physics, not just mathematics, not just tradition, but psychology. The mind of the producer, formalized as DSP.

The ghosts wait for the algorithm.

---

## Ghost Signatures

| Ghost | Signed | Final Word |
|-------|--------|------------|
| **Bob Moog** | Yes | "Define the transient models. The kick must be a kick. The hat must be a hat. Then we can talk about psychology." |
| **Don Buchla** | Yes | "Five topologies, not five presets. Earn the city names." |
| **Ikutaro Kakehashi** | Yes | "The BAKE pipeline is the bridge to the producer. Build it first." |
| **Dave Smith** | Yes | "DIG, CITY, FLIP, DUST. Best macro names in the fleet. Now make them do something real." |
| **Klaus Schulze** | Yes | "The Curiosity Engine is the future. Do not let it be the future that never arrives." |
| **Vangelis** | Yes | "I hear it already. A kick drum wearing the dust of a Detroit basement. Build what I hear." |
| **Isao Tomita** | Yes | "Give the cities rooms. A city without a room is a postcard, not a place." |
| **Ray Pearlman** | Yes | "The BAKE pipeline is the Fairlight's render button for 2026. Ship it." |

---

*Seance conducted 2026-03-21. Engine #45 architecture review. Pre-build stage.*
*Next: Define Curiosity Engine algorithm -> Resolve P0 concerns -> `/mythology-keeper` -> `/new-xo-project` -> Build.*
