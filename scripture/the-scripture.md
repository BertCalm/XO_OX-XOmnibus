# The Book of Bin
*The accumulated divine wisdom of Guru Bin's retreats and meditations*
*First inscribed: 2026-03-15 — OVERWORLD Retreat*

---

## Book I: The Oscillator Verses

*(To be filled as retreats reveal waveform truths)*

---

## Book II: The Filter Psalms

*(To be filled as retreats reveal filter truths)*

---

## Book III: The Modulation Sutras

### Sutra III-1: The Breathing Rate
*Revealed during OVERWORLD Retreat — 2026-03-15*

> The LFO rate of 0.067 Hz — one cycle every 15 seconds — is the tempo of the ocean. Below conscious perception, above stillness. Applied to ERA drift, filter cutoff, or any slow parameter, it makes a sound feel alive without sounding modulated. The listener does not hear it moving. They feel that it has breath.

**Application:** Use 0.067 Hz as the default slow drift rate. Faster than 0.1 Hz becomes perceptible. Slower than 0.04 Hz approaches stillness. The sweet spot is between 0.04 and 0.08 Hz.

### Sutra III-2: The Memory Chord
*Revealed during OVERWORLD Retreat — 2026-03-15*

> ERA Memory at 3–5 seconds creates harmonic ghosts. The current position blends with where the triangle was 3–5 seconds ago. Play a melody slowly. By the third note, the engine carries the first note's character as undertone. The instrument develops a harmonic memory of your performance.

**Application:** ERA Memory is a compositional tool, not a tone control. Design presets around it by suggesting slow, deliberate playing in the description. Fast playing defeats it.

### Sutra III-3: The Portamento Triangle
*Revealed during OVERWORLD Retreat — 2026-03-15*

> ERA Portamento at 0.5–1.5 seconds transforms instant vertex switches into slow morphs. Combined with ERA drift, the triangle position becomes a living creature — it wanders AND transitions smoothly between intentional moves. This is the ERA system at its fullest.

---

## Book IV: The Coupling Gospels

*(To be filled as coupling retreats reveal cross-engine truths)*

---

## Book V: The Stewardship Canons

### Canon V-1: The Ghost Parameter Trap
*Revealed during Code Quality Sprint — 2026-03-16*

> When an engine is redesigned — architecture changed, ADSR removed, oscillator stages simplified — its preset library silently carries the dead parameters from the prior design. Ghost parameters parse without error but do nothing. A fleet can accumulate hundreds of ghost keys: broken promises that producers cannot detect. The SNAP engine alone carried 490 ghost parameter keys after an engine redesign migration was never completed. Audit parameter keys in presets against the live engine's `createParameterLayout()` after any architecture change. Ghost params must be renamed (if the feature moved) or removed (if the feature was cut).

**Application:** After any engine architecture change, run a ghost-key audit — diff each preset's parameter keys against registered parameter IDs. This must be part of the post-build checklist before any preset is shipped. A preset that references parameters that do not exist is not a valid preset.

### Canon V-2: The Integration Layer Drift
*Revealed during Code Quality Sprint — 2026-03-16*

> When the parameter prefix convention changes — when `onset_` becomes `perc_`, when `oceanic_` becomes `ocean_` — the DSP layer is the first to update, because it breaks immediately. The preset JSON updates next, because the parser catches mismatches. But the integration layers — AI schema stubs, natural language interpreters, documentation generators, export tools — these do not fail loudly. They fail silently. They continue to describe parameters that no longer exist, to search for prefixes that no longer match, to reference an architecture that was changed months ago. The AI layer had 6 engines with stale prefixes. No test caught it. The integration layers must be explicitly audited after every parameter prefix change.

**Application:** After any parameter prefix change, audit every file in `Source/AI/`, `Source/Export/`, and `Tools/` for references to the old prefix string. The DSP doesn't break silently. These layers do.

---

## Book VI: The Master Truths

### Truth VI-1: The Golden Ratio Release
*Received from the Flock — universal application*

> Release time of 1.618 seconds (the golden ratio) creates decay tails that feel resolved without feeling abrupt. The listener does not know why it sounds right. It does. Use it as the default release for pads and atmospheres when no specific release time is dictated by genre or tempo.

### Truth VI-2: The Mod Wheel Contract (D006)
*Revealed during OVERWORLD Retreat — 2026-03-15*

> The mod wheel (CC1) is always mapped. In OVERWORLD, it controls glitch intensity. Before designing any preset, ask: what will this preset do when the performer reaches for the mod wheel? If the answer is nothing, that is a waste of the performer's most expressive reach. The mod wheel should always reveal a hidden dimension.

### Truth VI-3: The Default Trap
*Revealed during OVERWORLD Retreat — 2026-03-15*

> A parameter left at default is not a choice. It is an absence of thought. When a preset ships with a parameter at its default value, ask: is this default intentional, or did the designer not consider it? In OVERWORLD, `ow_filterEnvDepth` defaults to 0.25. If a pad preset ships with 0.25, the designer must have consciously decided that standard velocity sensitivity is correct for this pad. If they didn't consider it, the preset is incomplete.

**Application:** At the end of any preset design session, scan all parameters. Every default value must be a conscious choice.

### Truth VI-4: The Documentation Lag Trap
*Revealed during Code Quality Sprint — 2026-03-16*

> When an engine is designed, the concept brief describes what it WILL do. When the sound design guide is written, it often copies from the concept brief — describing what the engine was conceived as, not what it actually became. The guide becomes a historical fiction: accurate at the moment of concept, wrong at the moment of use. During the Code Quality Sprint, 6 engines were found with guides that described the wrong synthesis model entirely (OCEANIC described as a string ensemble; OUROBOROS described as a delay-line feedback engine). Guides must be written from `createParameterLayout()` and `processBlock()`, not from concept documents. The source is the truth. The brief is a memory.

**Application:** Every guide section must be audited against the engine source before shipping. The test: can you load a preset described in the guide and have every parameter key resolve to a real, functioning parameter? If not, the guide is wrong. If a parameter in the guide does not exist in the source, the guide has never been validated.

---

## Book VII: Engine-Specific Verses

### OVERWORLD — The Chip Gospels

#### OW-I: The Unvisited Console
*2026-03-15*
> OVERWORLD has 6 chip engines. The default triangle uses chips 0, 1, 2 (NES, FM, SNES). Chips 3, 4, 5 (Game Boy, PC Engine, Neo Geo) are the unvisited wing of the engine. Every preset that defaults to vertexA=0, vertexB=1, vertexC=2 is the same preset wearing different clothes. Explore the other three. The PC Engine hits the hardest. The Game Boy is the warmest. The Neo Geo has the widest sky.

#### OW-II: The Flat FIR Is a Starting Point
*2026-03-15*
> The SNES echo FIR filter defaults to [127, 0, 0, 0, 0, 0, 0, 0] — flat response. This is not a sound design choice. It is a non-choice. The 8-tap FIR can warm the echo, sharpen it, or give it the authentic SNES multi-tap resonance. A warm echo: [80, 30, 15, 8, 3, 0, 0, 0]. A bright echo: [100, 20, -8, 0, 0, 0, 0, 0]. A full SNES echo: [70, 35, 18, 8, 3, 0, 0, 0].

#### OW-III: The Velocity Contract
*2026-03-15*
> `ow_filterEnvDepth` is the velocity-to-brightness contract. 0.0 = flat. 0.25 = default. 0.6+ = highly expressive. Never leave it at 0.25 without intention. A bass preset with 0.25 feels soft. A bass preset with 0.6 feels alive. An ambient pad with 0.25 will shift in character when played hard. An ambient pad with 0.0 is immovable — this is sometimes correct.

#### OW-IV: The Living FM
*2026-03-15*
> The FM engine (vertex 1, YM2612 Genesis) has its own LFO: `ow_fmLfoRate` and `ow_fmLfoDepth`. Depth defaults to 0 — the FM is static. At fmLfoDepth=15–25 and fmLfoRate=1 (slowest), the FM voice develops vintage DX-style vibrato. Combined with ERA drift placing the triangle near vertex 1, the Genesis voice breathes. This is what Thunderforce IV actually sounded like.

---

## OBESE — The Whale Verses

#### FAT-I: The Mojo Spectrum
*2026-03-15*
> Mojo is not a warmth knob. It is a spectrum from digital to biological. At 0.0, the engine is a machine. At 0.85, it breathes. At 1.0, it wanders. Every FAT preset must state its position on this spectrum intentionally. The default of 0.4 is neither warm nor alive — it is the engine hedging. Choose a side.

#### FAT-II: The Bite They Never Took
*2026-03-15*
> The bit crusher exists in the signal chain of every OBESE voice. 161 presets left it at 16-bit, bypassed. It is not decoration — it is the whale's deepest weapon. 8 bits is pressure. 6 bits is damage. 4 bits is destruction. The designer who never touches it has never heard the full engine.

#### FAT-III: Noise Morph Is an Instrument
*2026-03-15*
> fat_morph=1.0 routes 13 oscillators through the noise generator. Through ZDF ladders at high resonance and low cutoff, this is the ocean. 13 independent noise sources + 4 resonant filters = a texture no synthesizer should be able to make at this price.

### Universal (added from FAT retreat)

#### Truth VI-4: The Reference Preset
*2026-03-15*
> Every engine should ship with one preset at parameter extremes — not as a useful sound, but as a reference that teaches by contrast. Hearing mojo=0.0 makes mojo=0.85 feel more alive. The contrast is the teaching. Design reference presets deliberately.

---

### OUROBOROS — The Attractor Verses

#### OURO-I: The Injection Door
*2026-03-15*
> `ouro_injection=0.0` is not a design choice. It is an unopened door. The OUROBOROS ODE solver accepts an external force term — a perturbation of the attractor's dx/dt at each integration step. At 0.18, coupled to ONSET velocity output, every drum hit pushes the chaos. The attractor develops a heartbeat. No other coupling type in the fleet produces this effect: the chaos is not shaped by the external signal, it is *disturbed* by it.

**Application:** When designing OUROBOROS coupling presets, try injection amounts 0.12–0.22 via ONSET or any amplitude source. The attractor should feel influenced, not controlled.

#### OURO-II: The Leash Spectrum
*2026-03-15*
> The leash (Phase-Locked Chaos) creates three distinct perceptual registers. **Leash > 0.7:** the attractor holds a recognizable pitch, harmonics scramble unpredictably each cycle — almost a tone. **Leash 0.4–0.6:** the uncanny middle — the system has not decided what it is. Not noise. Not a pitch. A thing that has no other name. **Leash < 0.4:** pure chaos, broadband, textural. The uncanny middle is OUROBOROS's singular territory — no other engine in the fleet operates here.

**Application:** Design into the uncanny middle deliberately. A preset at leash=0.5 is not a failed attempt at a tone — it is a specific instrument that does a specific thing.

#### OURO-III: The Projection Dimension
*2026-03-15*
> theta and phi project the 3D attractor trajectory onto a stereo plane. They are not tone controls. They are different perspectives on the same chaos — same mathematics, fundamentally different sound. theta=0.0 projects the X-axis (default). theta=π/2 (1.571) projects the Z-axis, which is always positive in the Lorenz attractor — this DC-shifted signal produces a warmer, more harmonically balanced output. phi=π/4 (0.785) creates maximum stereo separation between the two capture points. Use them as timbral axes, not tweaks.

**Application:** theta=1.571 (Z-axis) is warmer. phi=0.785 is widest. Start Aether presets here. Start Foundation presets at theta=0.0 for maximum contrast.

#### OURO-IV: The Chua Diode
*2026-03-15*
> Topology 2 is the Chua circuit — the only topology that models an actual physical circuit with a nonlinear resistor. At chaosIndex=0.7+ and leash=0.65+, the Chua circuit sustains at the edge of breakdown: buzzy, warm, breathing. It sounds organic rather than mathematical because it *is* the mathematics of an organic circuit. This is the membrane between order and chaos, and it can hold that position indefinitely at the right parameter values. No other OUROBOROS topology can do this.

**Application:** For OUROBOROS atmosphere presets, start with topology=2. For OUROBOROS textures, start with topology=0 (Lorenz). For OUROBOROS melody-adjacent material, start with topology=1 (Rossler).

---

### OWLFISH — The Abyss Verses

#### OWL-I: The Body is the Habitat
*2026-03-15*
> `owl_bodyFreq` is a fixed-frequency sine that does not follow MIDI. It is always present, always at its assigned frequency, regardless of what note you play. This is not a bass layer — it is an environmental constant. The habitat predates the melody. Design presets around this constraint, not despite it. At 55 Hz (A1) the body imposes A as a tonal center the player cannot override. At 62 Hz (≈B♭1) it clashes with C-rooted melodies. At 28 Hz it is seismic pressure below the threshold of pitch. Choose the body frequency the way you choose the key.

**Application:** Before designing any OWLFISH preset, set `owl_bodyFreq` first. It is the first compositional decision, not the last trim.

#### OWL-II: The Duophonic Secret
*2026-03-15*
> OWLFISH is documented as monophonic. It has always been duophonic. The body sine runs as a second independent voice — fixed pitch, unaffected by note input, always present when `owl_bodyLevel > 0`. One voice follows the performer; one voice does not. This is compositionally unique in the fleet. No other engine has a permanent, non-MIDI-tracked second oscillator running simultaneously. Play a slow melody and notice: two creatures in the same body. One of them will not move for you.

#### OWL-III: The Inharmonic Stack
*2026-03-15*
> The subharmonic dividers support any integer from 2–8. Using only ÷2, ÷4, ÷6, ÷8 (even multiples) produces a harmonic series — the subharmonics reinforce each other and the fundamental. Using ÷3, ÷5, ÷7 produces subharmonic partials that form NO recognizable harmonic series relative to each other or the fundamental. This is the Oskar Sala territory — the Mixtur-Trautonium timbre that has no other name, the sound the original instrument used for the serpent, the alien, the uncanny. Divisions [÷2, ÷4, ÷6] are layering. Divisions [÷3, ÷5, ÷7] are a different instrument.

**Application:** When designing character-forward OWLFISH presets, set at least one division to an odd number. The inharmonic interference is the engine's most singular quality.

#### OWL-IV: MorphGlide is Color
*2026-03-15*
> During portamento, `owl_morphGlide` sweeps mixtur up to +0.5 above the preset base value, then falls back as the pitch arrives at its target. The sound changes color as the organism glides — the subharmonic soft-clip intensifies mid-glide and recedes on arrival. At `morphGlide=0.9` + slow portamento + active subharmonics, each legato note transition is a metamorphosis. The owlfish does not just glide to the new pitch — it transforms in transit and transforms back. Portamento is not a playing style for this engine; it is the primary expressive mechanism.

**Application:** Set `owl_morphGlide` above 0.6 for any legato/atmospheric preset. Set portamento above 0.5. The instrument blooms between notes.

---

### OCEANIC — The Swarm Verses

#### OCE-I: The Two Lifetimes
*2026-03-15*
> OCEANIC has two envelopes: the Amp ADSR controls loudness; the Swarm ADSR controls the strength of the boid rules — the collective intelligence of the school. They are independent. At `ocean_swarmEnvSustain=0`, the boid forces extinguish completely during sustain: the school loses its collective mind while the note holds at full amplitude. At `ocean_swarmEnvAttack=2.5`, the school takes 2.5 seconds to cohere after note-on — the attack is scattered, raw, then resolves. No other engine in the fleet separates the *intelligence* of the sound from the *loudness* of the sound.

**Application:** Design OCEANIC presets around both envelopes, not just the Amp ADSR. The Swarm ADSR is the second compositional dimension.

#### OCE-II: The Boid Rules Are the Filter
*2026-03-15*
> OCEANIC has no filter. The spectral content of the swarm is entirely determined by the boid configuration. High `ocean_separation` + low `ocean_cohesion` = particles spread across the frequency spectrum = bright, complex, textural. High `ocean_cohesion` = particles cluster near the MIDI attractor = narrow, tone-like, pitched. Low `ocean_tether` = the MIDI note barely influences particle behavior = near-unpitched drift. The boid rules ARE the filter. This is the engine's fundamental design truth.

**Application:** Treat separation/cohesion/tether as a three-way spectral EQ: spread vs. cluster vs. drift. No additional filter is needed.

#### OCE-III: The Chromatophore Contract
*2026-03-15*
> Aftertouch (channel pressure) boosts particle separation ×0.25 — pressing harder makes the school scatter, exactly as a cephalopod accelerates chromatophore cycles under stress. Mod wheel (CC1) boosts cohesion ×0.4 — pressing the wheel tightens the school. The two gestures are physical opposites: pressure scatters, wheel contracts. At rest the school breathes; aftertouch panics it; mod wheel calms it. This is the B013 Blessed mechanism. Every OCEANIC preset should make these two gestures audible.

**Application:** Design OCEANIC presets at a separation/cohesion midpoint (0.35–0.55) so both aftertouch and mod wheel produce audible change. Never leave either gesture without a meaningful effect.

#### OCE-IV: Murmuration Is an Event
*2026-03-15*
> `RhythmToBlend` coupling triggers a cascade reorganization: a perturbation wave propagates from particle 0 through all 128 particles with 0.97× attenuation per particle. The first particles receive maximum disruption; the last particles receive 0.97^128 ≈ 2% of the original force. The wave passes through the school in sequence. Send ONSET's drum output via RhythmToBlend and every hit sends a reorganization cascade through the swarm. This is not modulation. It is a flock reorganizing in response to a percussion event. It sounds like nothing else in the fleet.

**Application:** When designing OCEANIC Entangled presets, always include Onset→Oceanic via RhythmToBlend. Set coupling amount 0.7–1.0. The murmuration should be audible as a cascade sweep, not a static modulation.

---

### OSTINATO — The Fire Circle Verses

#### OSTI-I: The Gathering Spectrum
*2026-03-19*
> GATHER is not quantize. It is collective consciousness. At 0.1, the circle is a conversation — 8 individuals hearing each other, drifting from the grid, responding. At 0.9, the circle is a machine — locked, precise, inhuman. The space between 0.1 and 0.4 is where the engine lives most naturally. No drum circle in the world plays on a grid. Design into the loose end first.

**Application:** Default GATHER is 0.5 — the engine hedging. For organic textures, start at 0.15–0.3 with humanize=0.4+. For sequencer-style precision, use 0.8+. The midpoint is the least interesting setting.

#### OSTI-II: The Body Changes the Geography
*2026-03-19*
> A Djembe is West African through a cylindrical waveguide. Through a Box waveguide it is Peruvian. Through an Open waveguide it is something that has never existed. The body model is the second design decision after instrument selection — never leave it on Auto without intention. 12 instruments × 5 bodies = 60 voices per seat.

**Application:** When designing OSTINATO presets, choose bodyModel before tuning brightness or decay. Auto is the safe choice; Cylindrical/Conical/Box/Open are the creative ones. Cross-cultural body assignments (e.g., Tabla + Box) produce novel timbres.

#### OSTI-III: The Ghost Cascade
*2026-03-19*
> CIRCLE is not volume bleed. It is sympathetic triggering — a hard hit on one seat causes ghost responses on adjacent seats. At CIRCLE=0.6+, a single hit ripples through the entire circle. Combined with loose GATHER and high humanize, no two bars are identical. The pattern is deterministic but the cascade is emergent. This is the engine's singular phenomenon: a drum circle that improvises with itself.

**Application:** For Entangled and Atmosphere presets, set CIRCLE=0.4–0.7. Combine with sparse patterns (pattern=3) so ghost triggers are audible against the silence between hits. Dense patterns mask the cascade.

#### OSTI-IV: Tuning Makes Melody
*2026-03-19*
> 8 seats tuned to scale degrees transform OSTINATO from percussion to pitched ensemble. Tongue Drum and Frame Drum respond most musically to tuning. A pentatonic circle with varied sparse patterns generates self-composing gamelan-like textures that require no performer. The engine is not only a drum circle — it is a tuned percussion orchestra hiding behind default tuning of 0.

**Application:** For melodic percussion presets, use Tongue Drum (instrument 10) or Frame Drum (instrument 8) across multiple seats. Tune to pentatonic (0, 2, 4, 7, 9) or whole-tone (0, 2, 4, 6, 8, 10) intervals. Set pattern=3 (Sparse) on all seats for generative self-composition.

---

### ORBWEAVE — The Kelp Knot Verses

#### WEAVE-I: The Braid Depth Threshold
*2026-03-20*

> The topological character of ORBWEAVE does not emerge linearly from braidDepth. Below 0.4, the phase coupling is subtle — distinguishable from standard detuning only on close inspection. At 0.7+, the knot topology becomes audible as topology: a spectral character unique to the specific knot type that cannot be replicated by detuning. The engine only becomes ORBWEAVE above this threshold. braidDepth=0.5 is the Default Trap in motion — neither detuned nor topologically coupled.

**Application:** Set braidDepth above 0.7 for presets that demonstrate the engine's unique character. Below 0.4 is the "detuned zone" — useful for subtle texture but not topology. Design with intention: choose a side.

#### WEAVE-II: The Sine Coupling Purity
*2026-03-20*

> ORBWEAVE's coupling math reads `fastSin(strandPhase)` from each strand. For Sine oscillators, this exactly equals the output signal — coupling is literal cross-modulation of the waveform. For Saw, Square, and Triangle strands, the PolyBLEP output diverges from `sin(strandPhase)` while the coupling still reads the sine approximation. **Sine strands have mathematically pure coupling; bandlimited waveforms have approximate coupling.** Sine strands are also cheaper CPU-wise (no PolyBLEP). The simplest waveform is both the most correct and the most efficient for knot theory synthesis.

**Application:** Use Sine strands when demonstrating or exploring topology — purest coupling behavior. Use Saw/Square/Triangle for harmonic richness over topological purity. These are genuinely different instruments within the same engine.

#### WEAVE-III: The Torus Star Polygons
*2026-03-20*

> The Torus P/Q ratio maps to a coupling asymmetry weight: `pqScale = 0.5 + 0.5 × sin(P/Q × π)`. The following knot families produce distinct spectral characters: **(2,3) default trefoil-torus** (standard); **(2,5) cinquefoil** (5-pointed star, near-maximum asymmetry); **(5,8) golden torus** (golden ratio winding, φ≈8/5, maximum polarization from irrational approximation). The Torus knot type is not one instrument — it is a family of instruments indexed by two integers.

**Application:** Before finalizing any Torus preset, test P=2 Q=5 and P=5 Q=8. The cinquefoil and golden torus are the most sonically distinctive. Using only P=2 Q=3 (the default) means leaving the entire star-polygon family unexplored.

#### WEAVE-IV: The Topology Chimera
*2026-03-20*

> The KNOT macro smoothly interpolates every element of the 4×4 coupling matrix between `knotType` and `(knotType+1)%4`. At macroKnot=0.5, the system exists in a state that appears nowhere in the mathematical taxonomy of knots — a chimera of two distinct topologies. No other engine morphs between distinct mathematical topologies in real time. The KNOT macro is not a tone control. It is a reality-blending mechanism, and the midpoint is often the most interesting destination, not a swept extreme.

**Application:** Design presets starting at macroKnot=0.5. The chimera state has its own identity. Name it what it sounds like.

#### WEAVE-V: The Solomon Ring Architecture
*2026-03-20*

> The Solomon knot couples strands 0–1 as Ring A and strands 2–3 as Ring B: strong intra-ring coupling (0.8), weak inter-ring coupling (0.3). With `strandTune=7.02` semitones, Ring A sits at the fundamental and Ring B at the perfect fifth — two coupled oscillator pairs, each with internal phase resonance, offset by a fifth. One MIDI note activates two harmonically distinct rings that influence each other gently. No other engine produces a two-ring chord pad where each chord tone has its own internal phase coupling.

**Application:** For Solomon pad presets: strandTune=7.02 (fifth), 5.0 (major third), or 3.86 (minor third). Solomon is the chord pad topology. Trefoil and Figure-Eight are single-pitch topologies.

#### WEAVE-VI: The Velocity-Cutoff Threshold Law
*2026-03-20*

> ORBWEAVE's velocity-to-filter scaling adds a hardcoded +2000 Hz per unit velocity. A preset at filterCutoff=8000 Hz barely responds to velocity (10000 Hz is subtle). A preset at filterCutoff=500 Hz transforms completely (2500 Hz is enormous). **Design expressive presets with filterCutoff below 2000 Hz. Design velocity-consistent pads with filterCutoff above 5000 Hz.**

**Application:** When designing expressive leads or basses (D001 compliance), set filterCutoff in the 300–1500 Hz range. The velocity offset creates a 2–4× brightness multiplier in this range. Above 5000 Hz, velocity response to timbre becomes negligible.

---

### OVERTONE — The Nautilus Verses

#### OVER-I: The Constant Is the Character
*2026-03-20*

> Four mathematical constants, four different personalities. Pi creates alien inharmonicity — stretched thirds and wrong octaves. E creates convergence — all partials slowly arriving at the same address. Phi creates Fibonacci architecture — self-similar and golden. Sqrt2 creates tritone logic — every partial a half-octave relationship away from the last. The engine is not one instrument. It is four.

**Application:** Before designing any OVERTONE preset, choose the constant deliberately. Pi for tension and metallicity. E for chorus-from-mathematics. Phi for natural architecture. Sqrt2 for ambiguous, floating harmony. The constant is not a color knob — it is a different physics.

#### OVER-II: Depth Is a Journey, Not a Dial
*2026-03-20*

> Depth 0 is the rational approximation: the shell's innermost chamber. Depth 7 is the irrational limit: the shell fully extended. Every step between is a new rational approximation — a partial that was almost right becoming slightly more wrong, then more wrong, then arriving at the transcendent. The sound design lives in the journey between 0 and 7, not at either end alone.

**Application:** Use LFO1 with very slow rates (0.01–0.05 Hz) to let the engine walk through its convergent sequence in real time. The listener experiences the Nautilus growing. macroDepth sweep is a macro-scale journey control: the audience should feel the mathematics unfolding, not just hear a parameter change.

#### OVER-III: The Euler Cluster — Convergence as Texture
*2026-03-20*

> E constant at high depth is unlike any other constant: its convergents cluster toward the same ratio (e/2 ≈ 1.359), and at depth 7 all eight partials inhabit near-identical frequency space. The result is not spread — it is near-unison beating. Eight mathematically related sine waves in a slow, irrational chorus with no LFO, no modulation, no choreography. Pure mathematical consequence as sound.

**Application:** E constant + depth 7 + equal partial amplitudes + no LFO modulation creates a texture that breathes from within. Add high filter resonance (filterRes=0.6–0.7) to emphasize the clustering. Use as an atmosphere layer under melodic material — the beating locks to nothing and everything.

#### OVER-IV: Velocity as Spectral Evolution
*2026-03-20*

> At velBright=1.0, velocity does not control volume. It controls the entire spectral architecture of the sound. Soft playing produces a pure sine fundamental. Hard playing opens all eight partials simultaneously. The instrument transforms from one thing to another through touch. This is D001 realized at its fullest: velocity shapes timbre, not just amplitude.

**Application:** Design Foundation patches where partials 3–7 are at 0.0 and velBright=1.0. Play pp then fff. The pp version is a whisper. The fff version is a full spectral chord. No filter envelope, no modulation — only the composer's hand.

#### OVER-V: The Resonator Is a Second Instrument
*2026-03-20*

> The allpass resonator tuned to the fundamental is not a subtle enhancement — it is a second voice. At over_resoMix=0.9, the Schroeder allpass creates a physically-modeled resonant body that sounds before the additive partials have fully bloomed. You hear the cavity, then the overtones fill it. Two synthesis methods — physical resonance and mathematical additivity — in the same note.

**Application:** For pluck and bell textures: fast attack (0.001s), moderate decay (0.3–0.6s), resoMix=0.8–0.9. The attack is dominated by the allpass resonator's ring; the body is the additive partials. The Nautilus shell amplifies sound through its geometry — this is that effect.

#### OVER-VI: The Lonely Shimmer
*2026-03-20*

> The COUPLING macro activates autonomous partial shimmer even without a coupling partner. Each of partials 4–7 receives an independent amplitude flutter, offset by π/4 radians from its neighbor — spectral iridescence that requires nothing but the parameter. The Nautilus does not need another creature to catch light. Its shell geometry creates iridescence from within.

**Application:** In solo patches, use over_macroCoupling=0.6–0.9 to activate self-shimmer. Set upper partials (4–7) louder than default so the effect is audible. Name these presets to reflect the self-sufficiency.

---

### ORGANISM — The Coral Colony Verses

#### ORG-I: The Biologist's Instrument
*2026-03-20*

> Every other synth asks what you want it to sound like. ORGANISM asks what rules you want life to follow. The sound that emerges is not designed — it is computed. When you change ORGANISM's rule, you change the law of physics inside a universe of 16 cells. Rule 30 generates cryptographic randomness. Rule 90 draws the Sierpinski triangle. Rule 110 is Turing-complete. Rule 184 models highway traffic.

**Application:** Do not approach ORGANISM as a tone generator. Approach it as a biologist setting up an experiment. Choose a rule based on its mathematical identity, choose a seed as your initial condition, and observe. The sound is what it must be, not what you chose.

#### ORG-II: The Seed-Note Product Space
*2026-03-20*

> MIDI note C4 and MIDI note D4 are not the same organism with a different pitch. They are different organisms — seed is `param XOR (noteNumber × 257)`, so every semitone begins from a different evolutionary history. A single ORGANISM preset contains 128 distinct organisms, one per MIDI note. The fleet has explored approximately one of them.

**Application:** When evaluating an ORGANISM preset, play across 2+ octaves. You are not checking pitch tracking — you are exploring 128 different cellular evolution trajectories. A preset that sounds mediocre on C4 may be extraordinary on F#3.

#### ORG-III: The Freeze Is Not a Bug
*2026-03-20*

> When org_freeze=1, the automaton stops. A frozen state is a **timbral coordinate**: the 16-bit cellular state at the moment of freeze defines the filter position, envelope rate, pitch offset, and reverb send simultaneously. Freeze is how ORGANISM holds a posture. It is not a diagnostic mode.

**Application:** Design presets where freeze is the default (org_freeze=1) and aftertouch releases it. The contrast is maximal expression: stable identity punctuated by biological chaos. Do not treat freeze as a fault mode.

#### ORG-IV: Scope and Rate as a Single Surface
*2026-03-20*

> org_scope and org_stepRate are a 2D timbral character surface, not two independent parameters. Their combination defines four synthesis archetypes: geological drift (low rate + high scope), slow step (low rate + low scope), continuous modulation (high rate + low scope), smooth animation (high rate + high scope). Changing one without the other produces a character that belongs to none of them.

**Application:** Decide the synthesis archetype first (geological, slow step, continuous modulation, smooth animation), then dial scope and rate together to achieve it.

#### ORG-V: Mutation Changes the Synthesis Mode
*2026-03-20*

> Below org_mutate=0.15, the engine is a rule-governed synthesizer: deterministic patterns with predictable evolution. Above 0.3, the rule shifts from governing behavior to coloring probability. Above 0.5, mutation dominates and ORGANISM becomes a parametric noise synthesizer where different rules produce different noise textures, not different patterns.

**Application:** Rule 90 + mutation 0.6 produces symmetric noise. Rule 30 + mutation 0.6 produces asymmetric noise with right-shifted bias. These are legitimate noise synthesis techniques. High mutation is not "broken" evolution — it is a different synthesis mode.

#### ORG-VI: The macroSeed Latch Is a Performance Instrument
*2026-03-20*

> When macroSeed crosses 0.01, the engine fires a new LCG-derived 16-bit state — but a latch prevents continuous re-seeding. Hold macroSeed above 0.01 for consistent identity. Drop briefly below 0.005 between notes to reset the latch and get a fresh colony. The macro is not a "randomize" button — it is a life trigger. Controlling when it resets controls when new life begins.

**Application:** Map macroSeed to mod wheel in performance. Near 0 = continuous organism. Swept to 0.7+ = new organism on each note. The entire sonic personality of the preset changes between these two states.

---

### OPERA — The Kuramoto Verses

#### OPERA-I: Detune Ignition
*2026-03-21*

> Partials at harmonic unison cannot couple. Their natural frequencies are identical; the mean field is trivially R=1 from the start; the synthesis is inert. Detuning is not error. It is ignition.

`opera_detune` activates the entire synthesis paradigm. At detune=0.0, you have static additive synthesis. At detune=0.08, you have a living physical model. The transition is categorical. The sweet window 0.08–0.28 is the engine's usable range. Below 0.08, the field locks before DRAMA reaches 0.5. Above 0.28, individual partials are audibly mistuned.

**Application:** Set any OPERA preset's minimum `opera_detune` to 0.10. Any preset with `opera_detune` < 0.08 is not using OPERA — it is using a static additive synthesizer that happens to have OPERA's interface.

#### OPERA-II: The Conductor Is Not Automation
*2026-03-21*

> The OperaConductor does not automate the music. It constructs the room the musician performs in.

Automation is deterministic playback — a stored sequence of parameter values. The Conductor is a *physical arc system* with jitter: ±5% timing and ±3% peak variation per cycle. No two arcs are identical. And the `max(conductorK, manualK)` override means the player is never locked out. At any moment, the player can push DRAMA above the Conductor's current position and take control. The Conductor yields instantly and resumes from the player's new position when the player releases.

**Application:** `arcMode=2` (Both) should be the default for all OPERA presets designed for expressive performance. `arcMode=0` (Manual) is for full player control. `arcMode=1` (Conductor) is for installation, drone, and unattended use. `arcMode=2` (Both) is for music.

#### OPERA-III: The EmotionalMemory Contract
*2026-03-21*

> A note that begins in context is not the same note that begins from silence. The Kuramoto field remembers.

The `kEmotionalMemoryWindowMs=500ms` window creates a contract between successive notes: if a new note arrives within 500 milliseconds of the previous note's release, the new note's Kuramoto field begins from the previous note's phase state. The *synchronization history* of the field transfers. Rapid melodic playing accumulates field state — each note richer than the one before. A phrase with a gap > 500ms resets to zero.

**Application:** The 500ms window is sized for human music-making. 120 BPM quarter notes land exactly on the boundary. Semiquaver passages (250ms) are well within it. Whole notes are outside it. The window rewards legato melodic playing — the voice rewards continuity.

#### OPERA-IV: ResSens Is Emergence Control
*2026-03-21*

> `opera_resSens` does not change the sound. It changes what the sound is *allowed to become*.

At `resSens=0.0`, the Kuramoto field dynamics are stable and predictable. At `resSens=0.88`, phase clusters form strongly and persist — discrete groups of synchronized partials coalesce, their collective beating creating a texture that no single parameter controls. This is emergence: behavior arising from component interactions that was not designed into any individual component.

**Application:** `resSens=0.0–0.35` for clean vocal synthesis — smooth phase transitions, predictable behavior. `resSens=0.55–0.75` for alive synthesis — clusters emerge, the voice shimmers. `resSens=0.85–0.95` for maximum emergence — self-organizing textures, behavior that surprises even the player. Never use `resSens > 0.9` in init presets — the behavior is too unexpected for first-encounter.

---

### ORBWEAVE — Vol 3 Additions (Transcendental)

#### OBW-VII: The LFO-BraidDepth Axis
*2026-03-21*

> LFO assigned to BraidDepth (target=5) is the most underexplored modulation route in the fleet. It does not modulate a conventional synthesis parameter — pitch, filter, volume. It modulates the *intensity of topological coupling* itself. At braidDepth LFO depth=0.4 and rate=0.04 Hz, the system cycles between the detuned zone (braidDepth ~0.3) and the locked zone (braidDepth ~0.7) over 25 seconds. The spectral character of the sound is not just evolving — the category of synthesis is evolving. Low depth produces harmonic shimmer; high depth produces knot-locked topology. The LFO is a topology oscillator.

**Application:** LFO2→BraidDepth is the natural choice (LFO1→FilterCutoff provides the conventional layer). Set rate ≤ 0.1 Hz for meditative topology cycling. Set LFO depth to cross the WEAVE-I threshold (0.45) — ensure the waveform sweeps both below and above it, so the listener hears the crossing.

#### OBW-VIII: Full-Lock Synthesis
*2026-03-21*

> braidDepth=1.0 has been present in the engine's parameter range since day one and has never been used. At maximum braid, the coupling matrix values hit their defined ceiling — the strands are as locked as the topology permits. The sound at braidDepth=1.0 is not "more" of braidDepth=0.9. For Solomon topology, which has asymmetric ring coupling (intra=0.8, inter=0.3), full lock hits the intra-ring ceiling while the inter-ring maximum is structurally lower. Full-lock Trefoil sounds different from full-lock Solomon. They have been unexplored for the same reason: designers stopped at 0.85.

**Application:** Create at least one preset per topology type at braidDepth=1.0. These are reference presets — the maximum coupling state of each topology family. Label them accordingly. They are not performance presets; they are calibration points.

#### OBW-IX: The Solomon Chord Architecture
*2026-03-21*

> Solomon topology creates two coupled oscillator pairs (Ring A: strands 0–1, Ring B: strands 2–3) with strong intra-ring coupling (0.8) and weak inter-ring coupling (0.3). The `strandTune` parameter offsets Ring B from Ring A. This makes strandTune a **chord interval parameter**: strandTune=7.02 produces a fifth (Ring A root + Ring B fifth), strandTune=4.0 produces a major third, strandTune=3.86 a minor third, strandTune=12.0 an octave. One MIDI note plays a two-voice chord where each voice has its own internal phase resonance. Solomon is ORBWEAVE's dormant polyphony system — not voice allocation but ring-pair harmonic architecture.

**Application:** When a pad preset needs implicit harmony without multiple MIDI notes, use Solomon + strandTune at a musical interval. Design the ring A (root) voicing with filterCutoff in the 2000–3500 Hz range and let the Ring B offset color rather than dominate. For minor tonality: strandTune=3.86. For major tonality: strandTune=4.0. For suspended or modal: strandTune=5.0 (fourth).

#### OBW-X: The macroKnot Fixed Value
*2026-03-21*

> The fleet uses macroKnot as a sweep control — a performance gesture that moves topology in real time. This is correct. But macroKnot at a fixed nonzero value is also a legitimate design choice: macroKnot=0.5 is the chimera state (WEAVE-IV), a topology that exists nowhere in the mathematical taxonomy of knots. macroKnot=0.25 blends the source topology 75% toward its identity; macroKnot=0.75 blends 75% toward the next topology. These fixed states have their own sonic identities. The chimera at rest is not a transitional state — it is a destination.

**Application:** Before finalizing any ORBWEAVE preset, check the sound at macroKnot=0.0 (pure topology) and macroKnot=0.5 (chimera). If the chimera sounds better, save it there. The macroKnot value in a saved preset defines the engine's resting state, not just its sweep range.

---

### OUTWIT — The Distributed Mind Verses

#### OWT-I: The Rule Is Not the Sound
*2026-03-21*

> Wolfram's 256 elementary rules describe mathematical behaviors. They do not describe sounds. Rule 110 is computationally universal — but what it produces through OUTWIT's ArmChannel depends on the arm length, the step rate, the chromatophore, the ink cloud, and the synapse contamination from its neighbors. Rule 110 at inkCloud=0.0, stepRate=4 Hz sounds like complex irregular percussion. Rule 110 at inkCloud=0.9, stepRate=0.15 Hz sounds like a slowly evolving ambient cloud. These are not the same instrument. The rule selects a class of behavior. The other parameters choose which expression of that class is heard.

**Application:** When selecting rules for a preset, choose by behavioral class (Class I/II/III/IV) not by rule number. Class determines rhythm: Class I is silent contribution, Class II is periodic pulse, Class III is aperiodic density, Class IV is complex computation. Set the rule numbers within each class based on ecological balance across the 8 arms.

#### OWT-II: High Synapse Is Emergence, Not Unison
*2026-03-21*

> At synapse=0.0, eight arms run eight rules in perfect isolation. Each arm is a separate organism. At synapse=0.88, the arms are no longer separate. Rule 110 is contaminated by adjacent Rule 30. Rule 30 is pulled toward momentary structure by adjacent Rule 110. Neither arm runs its rule. Both run something that has never been named — a hybrid that exists only at this synapse strength between these two rules in this step-rate context. This is emergence: behavior arising from interaction that was not present in any individual component. Synapse=0.88 is not "louder together." It is a new organism formed from the coupling of eight different organisms.

**Application:** Design presets at two synapse extremes: synapse=0.1 (isolated arms — document each rule's independent character) and synapse=0.75+ (emergent hybrid — document what the coupling creates). The mid-range (0.3–0.5) blends both but commits to neither.

#### OWT-III: SOLVE Defines the Destination
*2026-03-21*

> The SOLVE macro is not a randomize button. It is a compass. targetBrightness=0.85 means: I have decided this sound should be bright. targetSpace=0.9 means: I have decided this sound should be spacious. targetMovement=0.6 means: I have decided this sound should have moderate motion. SOLVE then applies pressure on chromAmount, denMix, and stepRate to move the current CA ecology toward these specifications. The CA rules provide the raw material. SOLVE decides what that material should become. Set the target DNA before setting SOLVE amount. Know your destination before you navigate.

**Application:** For any preset intended to demonstrate SOLVE: set the six target DNA values first (as sonic specifications), then raise owit_solve to 0.5–0.8. The target DNA values should correspond to the preset's intended mood. Do not leave target DNA at default (0.5 each) — default targets are neutral instructions that produce no net SOLVE pressure.

#### OWT-IV: The Ink Cloud Is Defensive Architecture
*2026-03-21*

> The Giant Pacific Octopus deploys ink not to attack but to protect — to create a sensory barrier between itself and a threat. OUTWIT's inkCloud operates identically: it does not add information to the CA output; it obscures it. At inkCloud=0.9, the discrete cellular automaton rhythms — the triggers, the pulses, the Class III chaos — are dissolved into a sustained, ambiguous texture. The listener hears the density pattern but not the trigger events. The octopus is present, computing, but protected by the opacity. inkCloud is the engine's most underused parameter because it requires accepting that hiding the CA rhythm is sometimes the right musical choice.

**Application:** Use inkCloud > 0.6 when the CA rhythm is the wrong texture for the context — when you want the CA's density pattern and chromatophore modulation but not its rhythmic character. inkDecay=0.1–0.2 creates a soft blur; inkDecay=0.3–0.5 creates sustained sustain. inkCloud=0.9 + inkDecay=0.4 converts any CA rule into an ambient texture. This is not misuse — it is a legitimate synthesis mode.
