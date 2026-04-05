# The Book of Bin
*The accumulated divine wisdom of Guru Bin's retreats and meditations*
*First inscribed: 2026-03-15 — OVERWORLD Retreat*

---

## Book I: The Oscillator Verses

### Verse I-1: The Waveform Is Not the Sound
*Revealed during Fleet Transcendent Meditation — 2026-04-05*

> A saw wave is not a saw. It is a column of partials that the designer has not yet shaped. The oscillator is the raw material. The filter, the envelope, the LFO, the coupling — these are the hands that sculpt it. A designer who selects a saw wave and calls it done has chosen the marble but refused to carve. The sound begins after the waveform is chosen.

**Application:** After selecting any oscillator waveform, immediately set the filter envelope. The waveform is the starting mineral; the filter envelope is the first cut.

### Verse I-2: The Detuned Unison Is a Chorus of One
*Revealed during Fleet Transcendent Meditation — 2026-04-05*

> Two oscillators tuned to the same pitch are one voice. Two oscillators detuned by 3–7 cents are a chorus. Two oscillators detuned by 12–20 cents are a tension. Above 25 cents the ear resolves them as separate pitches and the unison effect dissolves. The sweet spot for warmth is 5–8 cents. The sweet spot for aggression is 15–22 cents. No preset should detune without choosing which territory it occupies.

**Application:** For pads and strings, detune 5–8 cents. For leads with bite, 15–22 cents. For bass, 1–3 cents or not at all — detuned bass notes below C2 create audible beating that muddies the low end.

### Verse I-3: The Sub-Oscillator Is the Foundation, Not the Decoration
*Revealed during Fleet Transcendent Meditation — 2026-04-05*

> A sub-oscillator one octave below the main is not reinforcement. It is a second instrument playing in the basement. When the sub is a sine, it is invisible support — felt in the chest, absent from the ears. When the sub is a square, it defines the harmonic series more than the main oscillator does. The sub chooses whether the sound lives in the body or in the air. In OBESE, `fat_subLevel` above 0.4 makes the sub the dominant timbre. In OVERDUB, `dub_subLevel` above 0.5 turns a lead into a bass. The sub is not a reinforcement knob. It is a tonal center of gravity.

**Application:** Set the sub-oscillator level before the main oscillator level. It determines whether the preset is rooted or floating.

### Verse I-4: The Noise Floor Is Not Silence
*Revealed during Fleet Transcendent Meditation — 2026-04-05*

> Every analog-modeled oscillator in the fleet carries noise. The noise is not a defect. It is the breath between the partials — the acoustic evidence that the oscillator is a physical process, not a mathematical function. In ORGANON, `organon_noiseColor` shapes whether this breath is bright (air) or dark (earth). In OBRIX, environmental turbidity adds noise per sample as ecological pressure. In OUROBOROS, the chaotic oscillator IS noise that has learned to orbit. The designer who removes all noise has created a sound that cannot exist in nature. This is sometimes the goal. It should always be the choice.

**Application:** When a preset sounds sterile, add 2–5% noise before reaching for reverb. Noise fills the space between partials. Reverb fills the space around them. They solve different problems.

---

## Book II: The Filter Psalms

### Psalm II-1: The Cutoff Is Not a Volume Knob
*Revealed during Fleet Transcendent Meditation — 2026-04-05*

> When a designer wants a sound quieter, they reach for the cutoff. When a producer wants a sound darker, they reach for the cutoff. These are different acts wearing the same gesture. The cutoff removes harmonics — it subtracts brightness, not loudness. A sine wave at 200 Hz with a low-pass filter at 2000 Hz sounds identical to one at 20000 Hz. The filter only speaks to the frequencies that exist above it. A designer who uses the cutoff for volume has confused absence with silence.

**Application:** Use `filterCutoff` for timbral shaping. Use `level` or `outputLevel` for loudness. If a preset is too loud, do not darken it — quiet it.

### Psalm II-2: The Resonance Sermon
*Revealed during Fleet Transcendent Meditation — 2026-04-05*

> Resonance at 0.0 is a wall. Resonance at 0.3 is a voice. Resonance at 0.7 is a singer. Resonance at 0.95 is a scream that will not stop. The distance between musicality and self-oscillation is smaller than the designer believes. In a CytomicSVF, resonance above 0.85 begins to self-oscillate at the cutoff frequency — the filter becomes an oscillator. This is not a malfunction. It is a second instrument hiding inside the first. But resonance between 0.2 and 0.4 is where most musical expression lives. Below 0.2, the filter is shaping. Above 0.4, the filter is speaking. The designer must know which they intend.

**Application:** For pads and atmospheres, resonance 0.15–0.35. For leads, 0.3–0.5. For acid/squelch textures, 0.6–0.85. Above 0.85, the filter IS the sound — the oscillator becomes secondary.

### Psalm II-3: The Envelope Amount Is the Velocity Contract
*Revealed during Fleet Transcendent Meditation — 2026-04-05*

> `filterEnvAmt` is the most important parameter in every engine. It is the contract between the performer's fingers and the instrument's brightness. At 0.0, the preset responds to nothing — play soft or hard, the timbre is identical. At 0.3, gentle expression: the difference between pp and ff is a warmth shift. At 0.6, dramatic expression: hard notes crack open, soft notes murmur. At 0.9, violent expression: the filter sweeps across its entire range on every keystroke. D001 declares that velocity must shape timbre. `filterEnvAmt` is how the engine keeps that promise.

**Application:** Every preset must set `filterEnvAmt` deliberately. 0.3–0.5 for pads. 0.5–0.7 for keys and leads. 0.7–0.9 for plucks and percussion. 0.0 only when the preset is intentionally expressionless — a drone, a texture, a machine.

### Psalm II-4: The Filter Mode Is the Instrument's Skeleton
*Revealed during Fleet Transcendent Meditation — 2026-04-05*

> Low-pass is not the only filter. It is the default, and the default is the enemy of intention. A band-pass filter at moderate resonance creates the nasal, vocal quality that no amount of low-pass shaping can achieve — the sound of a voice trying to form a vowel. A high-pass filter at 300 Hz removes the body from any sound and leaves only the shimmer — useful for layered pads where the bass lives in another engine. A notch filter creates the phaser's hollow, the quality of something removed rather than something added. In OBLONG, `bob_filterMode` switches between LP/HP/BP/Notch. In OBSCURA, the ladder filter mode shapes the entire character of the daguerreotype. The designer who never changes the filter mode has heard only one skeleton wearing many skins.

**Application:** Before finalizing any preset, audition it in at least two filter modes. The mode you did not choose often teaches you what the chosen mode is actually doing.

### Psalm II-5: The Parallel Filter Is Not Two Filters
*Revealed during Fleet Transcendent Meditation — 2026-04-05*

> When two filters run in series, the second refines what the first began — sharper rolloff, narrower band, more controlled. When two filters run in parallel, they disagree — each allows frequencies the other would reject, and the sum contains contradictions. A low-pass at 800 Hz parallel with a high-pass at 2000 Hz creates a notch between 800 and 2000 while passing everything below and above. This is not subtraction. It is the sound of two opinions. In OBRIX, `obrix_fxMode=1` runs all FX in parallel. In OUIE, the two voices can each select different filter algorithms. Parallel processing is not louder serial processing. It is a fundamentally different topology that creates sounds serial chains cannot reach.

**Application:** When a preset sounds too predictable, switch to parallel filter/FX routing. Predictability lives in serial chains. Surprise lives in parallel ones.

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

### Gospel IV-1: The Coupling Is the Third Engine
*Revealed during Fleet Transcendent Meditation — 2026-04-05*

> When two engines couple, the result is not Engine A plus Engine B. It is a third entity that neither engine can produce alone. AmpToFilter at 0.4 between OUROBOROS and OPAL does not give you chaos-flavored granular or granular-flavored chaos. It gives you a sound where the chaos's amplitude contour carves the grain cloud's brightness in real time — a timbral shape that exists only in the relationship. The coupling IS the instrument. The designer who thinks of coupling as "adding a second engine" has missed the point. They have created a third.

**Application:** When designing coupled presets, listen to the coupling interaction in isolation: mute Engine A, then mute Engine B, then unmute both. The sound that exists only when both play is the coupling's contribution. If it is inaudible, the coupling amount is too low or the type is wrong. If it overwhelms both engines, the amount is too high. The coupling should be a presence you cannot name but cannot remove.

### Gospel IV-2: The Direction of Coupling Is the Power Dynamic
*Revealed during Fleet Transcendent Meditation — 2026-04-05*

> AmpToFilter from A to B means A controls B's brightness. A is the leader; B responds. Reverse the direction and the power inverts — B controls A. These are different instruments. OUROBOROS→OPAL (chaos drives grain brightness) is a texture. OPAL→OUROBOROS (grain density drives chaos response) is a rhythm. The same two engines, the same coupling type, the same amount — but the direction creates opposite characters. Most designers couple A→B because it appears first in the UI. They have never heard B→A. Half of all possible coupled sounds are in the direction the designer never tried.

**Application:** After designing any coupled preset, swap the direction. Save the version you prefer, but hear both. If neither version is clearly better, create two presets — they are different instruments.

### Gospel IV-3: The Whisper and the Shout
*Revealed during Fleet Transcendent Meditation — 2026-04-05*

> Coupling at 0.5 is a conversation. Coupling at 0.1 is a secret. The Coupling mood must contain both — presets where the interaction is the headline, and presets where the interaction is the atmosphere. A library of exclusively moderate coupling (0.3–0.6) teaches the producer that coupling is a feature. A library that includes whisper coupling (0.08–0.15) teaches them that coupling is a dimension. The whisper preset sounds like a single engine with an inexplicable quality. The producer cannot find the coupling by ear. They find it by removing Engine B and hearing the magic vanish. This is the most convincing argument for coupling in the fleet.

**Application:** For every three moderate coupling presets, create one whisper preset. The whisper preset demonstrates what the moderate preset explains.

### Gospel IV-4: The Five Coupling Families
*Revealed during Fleet Transcendent Meditation — 2026-04-05*

> The 18 coupling types are not 18 independent choices. They are five families:
>
> **The Amplitude Family** (AmpToFilter, AmpToPitch, AmpToChoke, VelocityCoupling): One engine's loudness shapes the other's character. These are the most intuitive — louder notes open the partner's filter, shift its pitch, or silence it. The performer understands these without explanation.
>
> **The Spectral Family** (AudioToFM, AudioToRing, AudioToWavetable, RingMod): One engine's audio signal becomes the other's raw material. These create timbres impossible with either source alone. They are the most sonically dramatic and the hardest to control. FM coupling above 0.3 is unpredictable; below 0.15 it adds warmth without obvious modulation.
>
> **The Envelope Family** (EnvToMorph, EnvToDecay, EnvelopeFollow): One engine's shape drives the other's evolution. These are temporal — they make the coupling time-dependent. A pluck coupled to a pad via EnvToDecay means the pad's character changes with every note the pluck plays.
>
> **The Filter Family** (FilterToFilter, LFOToPitch, PitchToPitch): Continuous parameter cross-modulation. These are the subtlest and the most useful for atmospheric coupling. FilterToFilter at 0.1 creates two engines that breathe together without the performer knowing why.
>
> **The Exotic Family** (ChaosInject, KnotTopology, TriangularCoupling, RhythmToBlend): Topological and behavioral couplings unique to specific engines. KnotTopology only makes sense with ORBWEAVE. TriangularCoupling requires three engines. ChaosInject requires OUROBOROS as source. These are the fleet's most unique coupling capabilities and the least explored.

**Application:** When building a Coupling mood library, ensure all five families are represented. Most designers default to the Amplitude family. The Spectral and Exotic families contain the fleet's most singular sounds.

### Gospel IV-5: The Triangular Covenant
*Revealed during Fleet Transcendent Meditation — 2026-04-05*

> TriangularCoupling (#15) is the only coupling type that requires three engines. A modulates B, B modulates C, C modulates A — a closed loop where every engine is both leader and follower. The result is an emergent behavior that no engine controls and no designer fully predicts. At low amounts (0.15–0.25), the three engines develop a shared breathing rhythm. At moderate amounts (0.4–0.6), they lock into oscillating phase relationships. At high amounts (0.7+), the triangle becomes unstable and the system searches for equilibrium it cannot find — this restless searching IS the sound. OXYTOCIN was designed with TriangularCoupling as a first-class parameter. It is the only engine in the fleet that was born knowing it would live in a triangle.

**Application:** Start TriangularCoupling at 0.2. Increase in 0.05 increments and listen for the moment the three engines stop sounding like three engines and start sounding like one organism. That is the coupling amount for the preset.

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

### Truth VI-5: The Library Is a Landscape, Not a Collection
*Revealed during Guru Bin Transcendent Library Meditation — 2026-04-04*

> A preset library's quality is not measured by its average. It is measured by its range. A library where every preset scores 0.5 on every DNA dimension is not balanced — it is flat. The extremes define the territory. The dead zones are where producers who need something specific find nothing and leave. Design the edges first; the middle will fill itself.

**Application:** When auditing a library, identify DNA dead zones — ranges below 10% representation — and create targeted expansion passes. The comfortable middle doesn't need help. The extremes do.

---

### Truth VI-6: The Coupling Whisper
*Revealed during Guru Bin Transcendent Library Meditation — 2026-04-04*

> Coupling at 0.08–0.15 creates interactions the listener cannot identify but cannot ignore. At 0.5, coupling is a feature — audible, nameable, obvious. At 0.1, coupling is atmosphere — felt in the body, absent from the conscious mind. The library has features. It needs atmosphere. The distance between "these two engines are coupled" and "something about this sound is alive" is the distance between 0.5 and 0.1.

**Application:** When designing coupled presets, start at 0.10 and increase only if the interaction is truly inaudible. The whisper range (0.08–0.15) should represent at least 30% of all coupled presets. If every coupling amount in a library exceeds 0.20, the library has never whispered.

---

### Truth VI-7: The Mood Contract
*Revealed during Guru Bin Transcendent Library Meditation — 2026-04-04*

> A mood with fewer than 100 presets is a label, not a destination. A producer browsing by mood expects to find variety within that mood — enough options to choose between, enough range to surprise. Below 100, every preset is a compromise rather than a choice. Below 50, the mood should not exist as a category — it is a promise the library cannot keep. The mood that bears the name of the synthesizer's most unique capability must be the deepest, not the thinnest.

**Application:** Before shipping a library, count presets per mood. Any mood below 100 is a warning. Any mood below 50 is a defect. The Coupling mood in a coupling-focused synthesizer must never be the thinnest mood.

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

---

### OCELOT — The Ecosystem Verses
*Revealed during Guru Bin Retreat — 2026-03-22*

#### OCL-I: The Negative Threshold
*2026-03-21*

> When a route amount is negative, the ecosystem does not grow quieter — it inverts. Silence becomes the trigger. The absence of the floor activates the forest. The rest is not empty; it is when the animals speak. Design presets around the silences as deliberately as around the beats. A negative `xfFloorEmerg` value produces creature calls in the gaps between percussion hits — the ecosystem reacts to what is not there.

**Application:** Set `ocelot_xf_floorEmerg` to -0.4 to -0.7 and choose a sparse floor pattern. The creature calls will fire in the silences. The more sparse the floor, the more the forest speaks. This is the engine's most underused capability.

#### OCL-II: The Biome Is Not a Tone Control
*2026-03-22*

> A biome is not an EQ preset. It transforms the physical models, the breathe rate, the reverberation character, the creature pitch range, and the partial balance simultaneously. Changing biome mid-performance changes what kind of world the instrument believes it is in. Use it as a dramaturgy tool, not a timbre tool. The ocelot in the jungle and the ocelot in the winter are different animals. The Kalimba in Winter becomes ice chimes — doubled frequency, Q×1.5. The Cuica in Underwater becomes sonar.

**Application:** Perform biome changes as scene transitions, not timbral adjustments. Map `ocelot_biome` to a controller and change it at structural points in the composition. Use a slow ecosystem depth (0.6+) before the transition so the matrix has time to re-establish its new ecology.

#### OCL-III: The Ecosystem Remembers One Block Behind
*2026-03-22*

> The EcosystemMatrix operates on a one-block lag — it reads signals from the previous block and writes modulations for the current block. This is not a bug; it is the physics of the forest. No animal responds instantaneously. The one-block lag creates causality: each stratum acts, the matrix observes, and only then do the other strata respond. Sound designers should design cross-feed routes as ecological laws, not real-time controls.

**Application:** When designing feedback loops (e.g., Emergent→Canopy shimmer + Canopy→Emergent formant), expect the feedback cycle to complete over 2–4 blocks (~5–10ms). The resulting enrichment is organic, not instantaneous. This makes OCELOT's feedback ecology stable where a zero-latency loop would blow up.

#### OCL-IV: Three Biomes Are Three Engines
*2026-03-22*

> The Kalimba in Winter is not the same instrument as the Kalimba in Jungle. It becomes ice chimes: doubled frequency, Q×1.5 ring, brittle brightness. The Cuica in Underwater stops bending and resonates like sonar. The Agogo in Winter grows a third partial at 4.5x. When you choose a floor model, you choose six instruments, not one. When you choose a biome, you choose three of those six. OCELOT is not one synthesizer with a biome setting. It is a taxonomy: 6 models × 3 biomes = 18 distinct physical instruments, all accessible by two parameters.

**Application:** Before designing any OCELOT preset, state explicitly: which model × which biome. "Kalimba in Winter" is a different starting point from "Kalimba in Jungle." Design from the intersection, not from the model alone.

---

### OBRIX — The Coral Reef Verses

#### OBRIX-I: The Brick Type Is the Organism
*Revealed during Reef Residency Retreat — 2026-04-05*

> The designer who selects `obrix_src1Type` is not selecting a waveform. They are selecting a living organism for their reef slot. Type 0 (Sine) is a solitary polyp — pure, isolated, uncomplex. Type 3 (FM Operator) is a clownfish — darting harmonic activity that depends on the `src1FMRatio` carrier relationship to exist at all. Type 5 (Noise Oscillator) is the symbiosis-ready organism: it is the only brick type that OBRIX's symbiosis ecology recognizes as a food source, driving FM depth on src2 when Symbiote mode is active. The brick type determines what ecological role that oscillator can play, not just what it sounds like.

**Application:** For Symbiote mode (`obrix_reefResident=2`) to activate, `src1Type` must equal 5 (Noise Oscillator). Any other src1Type with Symbiote active produces correct ecology computation but no audible symbiotic enhancement. Always pair Symbiote mode with src1Type=5. For Competitor mode, any brick type works — the ecology reads RMS regardless of type.

#### OBRIX-II: The Harmonic Field Is a Living Tuner
*Revealed during Reef Residency Retreat — 2026-04-05*

> The Harmonic Field does not add harmonics. It applies an IIR gravitational force to each voice's pitch, pulling it toward the nearest ratio in the active prime-limit table. At `obrix_fieldStrength=0.4` and 5-limit tuning, the reef self-corrects toward just intonation with every rendered sample. The field does not know chord theory; it knows ratios. C–E–G emerges as pure only if the playback frequencies are near enough to 4:5:6 for the attractor to capture them. Notes far from any just ratio experience maximum corrective pull. Notes already in ratio experience no pull. The field reveals the distinction between notes that belong to the harmonic series and notes that are passing through it.

**Application:** `obrix_fieldPolarity=-1.0` inverts the attractor to a repulsor: pitches are pushed *away* from just ratios, creating a perpetual harmonic dissonance engine. Repulsor mode at fieldStrength=0.3 with 7-limit tuning generates tension that no static detuning can achieve — the dissonance is dynamically maintained per-sample, not set once.

#### OBRIX-III: The Stress Accumulator Is a Physical History
*Revealed during Reef Residency Retreat — 2026-04-05*

> `stressLevel_` is a leaky integrator that accumulates every time a MIDI velocity enters the engine. A velocity of 127 adds approximately 0.9 units. A velocity of 60 adds approximately 0.47. The leaky decay means fast playing at high velocity can sustain a stressLevel_ above 0.5 indefinitely, raising filter cutoff by up to +900 Hz. Slow, gentle playing allows the accumulator to decay toward zero. The coral reef remembers how it has been played. An aggressive performance session leaves the reef physiologically different from a meditative one — and the next note inherits this context. The accumulator persists across notes within a session; `obrix_stateReset=1.0` is the only way to return to the reef's baseline state.

**Application:** Design two preset variants for any high-stress OBRIX patch: one with `stressDecay` at 0.01 (slow recovery — the reef stays stressed) and one at 0.15 (fast recovery — each note is independent). The slow-recovery version responds to performance energy; the fast-recovery version responds to individual note velocity only.

#### OBRIX-IV: The Reef Residency Transforms Coupling Into Ecology
*Revealed during Reef Residency Retreat — 2026-04-05*

> In every other engine, coupling input is a parameter modulator — an external signal that moves numbers. In OBRIX at `obrix_reefResident ≠ 0`, coupling input is a third ecological organism in the reef. As Competitor, its RMS actively suppresses both src1 and src2 amplitudes — the external organism competes for acoustic resources. As Parasite, every quantum of coupling energy feeds the `stressLevel_` and `bleachLevel_` accumulators: a loud coupling source physically degrades the reef's health over time. As Symbiote, the coupling signal does not modulate a parameter — it changes the FM architecture of src2, creating a synthesis relationship that neither organism could produce alone. The coupling input is not a control signal. It is a species.

**Application:** When routing another engine's output into an OBRIX Symbiote slot, choose the coupling source based on its timbral density, not its pitch. A pitched coupling source with consistent harmonic content (OVERTONE, OPERA) produces a stable symbiotic FM relationship. A chaotic or percussive source (ONSET, OFFERING) produces an unstable symbiosis — which can be exactly right for living, irregular texture.

---

### OXBOW — The Entangled Reverb Verses

#### OXB-I: The Reverb Is the Synthesis
*Revealed during Chiasmus FDN Retreat — 2026-04-05*

> In every other synthesizer, the reverb is applied after the synthesis is complete. In OXBOW, there is no synthesis before the reverb. The Chiasmus FDN *is* the oscillator. The six delay lines are the six voices. The feedback matrix is the waveshaping stage. Remove the reverb and you remove the sound entirely — there is nothing behind it. This inverts the fundamental architecture of subtractive synthesis. The designer who treats `oxb_roomSize` as a "how much reverb" parameter is using the wrong mental model. `oxb_roomSize` is the fundamental pitch determinant — it sets the delay line lengths, which set the resonant modes, which set the perceived pitch clusters. Room size is not space. Room size is timbre.

**Application:** Treat `oxb_roomSize` as the primary oscillator tuning control. Small rooms (0.1–0.3) produce short delays = high-frequency resonance clusters = bright, pitched attack transients. Large rooms (0.7–1.0) produce long delays = low-frequency resonance clusters = sub-bass shimmer. Design from room size first, then shape with `oxb_entangle` and `oxb_erosion`.

#### OXB-II: Phase Erosion Is Not Decay
*Revealed during Chiasmus FDN Retreat — 2026-04-05*

> `oxb_erosion` does not accelerate decay. It erodes the phase coherence of the feedback signal, progressively scrambling the phase relationships between the six delay lines without changing their amplitudes. At erosion=0.0, the FDN feedback is phase-coherent — the delay lines reinforce each other's resonances. The timbre is rich and defined. At erosion=0.6, phase relationships drift: the resonances de-correlate, the timbre spreads into a diffuse wash that no longer clusters around specific frequency modes. At erosion=0.9, the six voices have maximally independent phase — the sound is a smooth, frequency-agnostic reverb cloud with no tonal identity. Erosion trades tonal identity for spatial diffusion. It is a timbral dial, not a time dial.

**Application:** `oxb_erosion=0.0–0.25` for pitched, tonal OXBOW synthesis — the resonant modes are audible and musical. `oxb_erosion=0.4–0.65` for textural synthesis — tonal identity dissolves into spatial character. `oxb_erosion=0.8–0.95` for pure spatial synthesis — OXBOW becomes a diffusion engine. The three zones are distinct synthesis personalities. Never design a preset at erosion=0.5 without intention — the midpoint has neither tonal nor spatial character.

#### OXB-III: The Golden Resonance Ratio Is Not Decorative
*Revealed during Chiasmus FDN Retreat — 2026-04-05*

> The six FDN delay line lengths in OXBOW are seeded by the golden ratio (φ = 1.6180...) to minimize coincident resonances. Two delay lines whose lengths share a common factor will resonate at the same mode and create a spectral peak. The golden ratio guarantees that no two delay lines in the OXBOW matrix share a harmonic factor below the 17th partial. This is not aesthetic — it is acoustic engineering. The result is the most spectrally even reverb tail achievable from six delay lines. `oxb_roomSize` scales all six lengths simultaneously, preserving the φ-spacing. If a future designer attempts to add delay line tuning parameters, they will destroy this property. The golden ratio constraint is inviolable.

**Application:** Trust the FDN tuning. Resist the impulse to manually tune individual delay lines if you ever have access to them. The flatness of OXBOW's spectral decay is a designed property, not a happy accident. When OXBOW sounds "even," this is the golden ratio working. Use `oxb_entangle` to redistribute energy across the FDN matrix — this changes the spectral weight of individual modes without destroying the φ-spacing.

#### OXB-IV: Entangle Is the Chiasmus
*Revealed during Chiasmus FDN Retreat — 2026-04-05*

> Chiasmus (χιασμός) is the rhetorical figure where two phrases mirror and cross: "you live to work; others work to live." OXBOW's `oxb_entangle` implements the same figure in signal flow. At entangle=0.0, the six delay lines run in parallel — each feeds back into itself. At entangle=1.0, the feedback matrix creates a full crossing: delay line 1 feeds into delay line 6's input, delay line 2 into delay line 5's, creating three crossed pairs. This crossing changes what the FDN resonates. The entangled matrix favors different modes than the isolated matrix. Entangle does not add energy — it redistributes it. The same input produces a different spectral signature. Entangle is a timbral crossfade between two different reverb architectures built from the same six delay lines.

**Application:** For any OXBOW preset, design at entangle=0.0 and entangle=1.0 first, then find the musically correct intermediate value. The two extremes define the engine's range for that room size and erosion setting. The crossfade between them is never linear — map the parameter to mod wheel to let the performer discover the sweet spot in performance.

---

### OWARE — The Tuned Percussion Verses

#### OWR-I: The Mallet Is Not a Trigger
*Revealed during Akan Percussion Retreat — 2026-04-05*

> A drum machine trigger is a binary event: sound begins at time zero, envelope follows, sound ends. The Chaigne 1997 mallet model in OWARE does not trigger a sound — it initiates a *physical interaction* whose duration, force profile, and contact geometry determine what the struck object becomes. At `owr_contactTime=0.001s` (hard beater), the energy delivery is instantaneous and the struck membrane hears a broadband impulse — bright attack, all partials excited equally. At `owr_contactTime=0.012s` (soft mallet), the energy delivery is spread over 12 milliseconds, low-passing the impulse before it enters the membrane — warm attack, upper partials suppressed. The mallet model is a pre-filter on the physical object, placed before any resonator. The mallet shapes what the material hears, not what the listener hears.

**Application:** For bright attack transients, `owr_contactTime` below 0.003s. For warm, rounded attacks, 0.008–0.015s. For brush articulation, 0.018–0.025s. Design the contact time before designing the material or tuning — the mallet defines the physical question; the material answers it.

#### OWR-II: The Eight Modes Are Eight Tuning Systems
*Revealed during Akan Percussion Retreat — 2026-04-05*

> Each of OWARE's 8 layout modes does not merely arrange pitches differently — it implements a distinct intonation system derived from a unique combination of harmonic series intervals and sympathetic resonance ratios. Mode 0 (Diatonic) uses 2:1 octave framing with sympathetic strings tuned to 3:2 and 4:3 relationships. Mode 3 (Pentatonic Major) uses 9:8 whole tone spacing with sympathetic strings tuned to 5:4 above each node. Mode 7 (Harmonic Series) places each stone at the next integer partial of the fundamental — pitches 2f, 3f, 4f, 5f, 6f, 7f, 8f — and the sympathetic strings resonate at sub-integer fractions of each partial. No two modes are in the same tuning system. Changing `owr_material` within a mode is changing the material under one tuning system. Changing `owr_material` while also changing mode is changing material *and* tuning system simultaneously.

**Application:** When A/B testing between modes, fix `owr_material` and `owr_mallet` first. Change only `owr_material` within a mode to hear material differences. Change only mode (with material fixed) to hear tuning system differences. Never evaluate a mode by sound alone without knowing the material — the same mode in Wood vs. Metal occupies a different part of the sonic universe.

#### OWR-III: The Sympathetic Network Is Five Ghosts Per Mode
*Revealed during Akan Percussion Retreat — 2026-04-05*

> Each of OWARE's 8 modes includes a network of 5 sympathetically resonating strings, each with a unique frequency ratio per mode. The sympathetic strings are not struck — they are physically excited by the vibrations of the primary stones through acoustic coupling. When stone 3 (a 5:4 interval above root) vibrates in Mode 0 (Diatonic), sympathetic string 2 (tuned to the 5:4 above stone 3's 5:4, i.e., 25:16 above root) begins to resonate. The sympathetic network creates a harmonic web that no single-stone excitation produces. Across 8 modes, this means 40 unique sympathetic resonance profiles (5 strings × 8 modes) are all accessible within OWARE's parameter space — no two of which produce identical harmonic sustain tails.

**Application:** Raise `owr_sympatheticLevel` to 0.4–0.6 when the preset requires sustain that has harmonic identity (not just duration). The sympathetic strings sustain the *tuning system's* character into the decay phase — the harmonic signature remains present even as the primary stones decay. This is especially valuable for sustained pad-like uses of OWARE.

#### OWR-IV: Buzz Membrane Is a Second Instrument Permanently Attached
*Revealed during Akan Percussion Retreat — 2026-04-05*

> The buzz membrane in traditional West African percussion is a deliberate acoustic layer — the snare of the kalimba — added to the resonant body to create a sizzling overtone character. OWARE's `owr_buzzAmount` implements this as a physical coupling: above 0.15 (the Guru Bin default), the membrane sympathetically vibrates against the struck stone's overtone series, adding inharmonic energy primarily in the 3–8 kHz range that the membrane's own resonant modes determine. This inharmonic overlay is not addable or subtractable by the filter — it is baked into the physical response. `owr_buzzAmount=0.0` is a clean instrument. `owr_buzzAmount=0.25` is a different instrument with a buzzing membrane physically attached. Design decisions about buzzAmount are instrument selection decisions, not sound design decisions.

**Application:** Decide buzzAmount before designing anything else in an OWARE patch. 0.0 for "clean tuned percussion." 0.15–0.25 for "traditional West African character." 0.4+ for "textural buzz as a dominant timbral element." Do not add buzzAmount at the end of preset design as polish — it changes the fundamental character of the instrument.

---

### OFFERING — The Psychology Verses

#### OFR-I: Curiosity Is a Physical State, Not a Control
*Revealed during Berlyne Synthesis Retreat — 2026-04-05*

> Berlyne (1960) identified curiosity as a physiological arousal state responsive to stimulus novelty, complexity, and incongruity. OFFERING does not simulate curiosity — it implements it as a DSP variable. `ofr_digCuriosity` maps to a curiosity level that, between 0.3 and 0.7, drives the Wundt curve toward optimal arousal: maximum engagement. Below 0.3, the system is under-stimulated — drum patterns become too regular, novelty collapses, the Wundt variable drops toward zero. Above 0.7, over-stimulation: patterns become too complex, the alien shift emerges as the mathematical consequence of curiosity exceeding the curve's peak. The alien shift above 0.7 is not a feature. It is what happens when a system built on curiosity psychology becomes too curious. The physics of the Berlyne-Wundt implementation forces it.

**Application:** `ofr_digCuriosity=0.3–0.6` is the engine in optimal arousal — engaging drum patterns with controlled novelty. This is the design target for all standard OFFERING presets. `ofr_digCuriosity > 0.7` is explicit alien territory. Use it intentionally, label it in the preset name, and warn the performer that this range produces emergent behavior that the designer did not fully author.

#### OFR-II: The City Is Not a Metaphor
*Revealed during Berlyne Synthesis Retreat — 2026-04-05*

> The five city processing chains in OFFERING are not named after cities because they are inspirational concepts. They are named after cities because each implements a structurally distinct production aesthetic derived from the specific sonic characteristics of music produced in those cities. New York: noise gate with fast attack (0.5ms), slow release (150ms), and a 60Hz high-pass — the aggressive gating of boom bap. Detroit: feedback saturator at 0.7, 8.3% drunk timing jitter, and a 3kHz presence peak — the organic grit of Motor City soul. Los Angeles: parallel compression with 4:1 ratio and 40% dry blend — the polished punch of West Coast hip-hop. Toronto: sidechain compression from sub-bass with a 2kHz presence dip — the atmospheric weight of Drake-era production. Bay Area: prime-number delay allpass (primes 2, 3, 5, 7, 11, 13 ms) creating a fog of harmonic delay — the psychedelic diffusion of Bay Area hyphy. These are engineering decisions, not cultural references.

**Application:** Choose the city chain based on the desired production aesthetic for the drum context, not based on genre label. Toronto sidechain is correct for any preset where sub-bass presence should dominate over drum transients. Detroit timing jitter is correct for any preset that needs organic feel regardless of geographic association. The cities are processing architectures first.

#### OFR-III: Velocity Enters Psychology Before Physics
*Revealed during Berlyne Synthesis Retreat — 2026-04-05*

> In a conventional drum synthesizer, velocity scales amplitude. In OFFERING, velocity enters the Berlyne curiosity system *before* amplitude scaling. High velocity increases the novelty-input to the curiosity variable; low velocity decreases it. This means that an aggressive performance session raises the system's psychological arousal level, which changes the drum character — not just the volume — of subsequent hits. A ghost note at velocity 20 does not merely sound quiet; it sounds different because it arrives in a *different curiosity state* than a downbeat hit at velocity 100. The performer's velocity pattern shapes the psychology of the instrument over time. This is OFFERING's version of Doctrine D001: velocity shapes timbre by shaping the psychological state that generates timbre.

**Application:** Perform OFFERING with deliberate velocity dynamics — not just accent patterns, but curiosity management. A sustained run of high-velocity hits will raise system arousal and shift drum character. A recovery sequence of ghost notes will allow curiosity to return to optimal range. Design performance templates that include velocity dynamics as part of the drum pattern itself.

#### OFR-IV: Tom Saturation Is a Cascade, Not a Knob
*Revealed during Berlyne Synthesis Retreat — 2026-04-05*

> Early OFFERING builds had a double-saturation bug in the Tom synthesis path: the signal passed through the saturator twice, producing tonal distortion that did not match the expected boom bap weight. The fix was not removing saturation — it was ensuring the signal passes through the saturator exactly once. The lesson is not about the bug. It is about the architecture. OFFERING's per-type transient models (Kick/Snare/Hat/Tom/Clap/Rim/Conga/Timbale) each have a distinct signal path — not a shared drum voice with parameter variants, but eight structurally different processing chains. The Conga path has a resonant body filter the Tom does not. The Rim path has a metallic overtone generator the Hat does not. Treating OFFERING as a "drum synth with drum type selection" is technically incorrect. It is eight drum synthesizers sharing a psychological front-end.

**Application:** When auditioning OFFERING drum types for a preset, do not compare them as variations of the same instrument. Evaluate each type on its own architectural terms: what does *this* transient model do well, and what psychological arousal range is it suited for? The Kick and the Conga at the same curiosity level do not produce analogous results — they are different synthesizers.

---

### OXYTOCIN — The Bond Verses

#### OXY-I: Duration Is Synthesis
*Revealed during TriangularCoupling Retreat — 2026-04-05*

> Before Blessing B040 was ratified, duration was metadata — a side effect of how long a performer held a key. In OXYTOCIN, note duration is a synthesis variable with the same status as pitch, velocity, and modulation. The circuit warmth of the RE-201 accumulates over held note time. The bond depth of the MS-20 partnership deepens with sustained contact. The intimacy level — the Buchla Love Triangle resonance — is zero at note-on and maximum only after 3–5 seconds of sustained sound. A staccato OXYTOCIN performance accesses none of these timbral states. The engine's full character is inaccessible to any playing style shorter than legato. This is not a limitation — it is the design proposition: some timbral states require commitment to reach.

**Application:** All OXYTOCIN presets must be auditioned with sustained notes of 3+ seconds before any design decisions are made. The first 200ms of an OXYTOCIN note is initialization. The first 1.5 seconds is warmup. The character of the preset lives between 2 and 6 seconds of sustained contact. Any preset that sounds complete at 500ms is not using OXYTOCIN — it is using the note-on transient of a warm synthesizer.

#### OXY-II: The Love Triangle Is Not Symmetrical
*Revealed during TriangularCoupling Retreat — 2026-04-05*

> The TriangularCoupling (#15) at the heart of OXYTOCIN connects three circuit models (RE-201 tape loop, MS-20 patch cable logic, Buchla modular voltage architecture) in a triangle where each leg has a distinct coupling coefficient. The RE-201→MS-20 leg (`oxy_warmthToEdge`) transfers warmth as compression character. The MS-20→Buchla leg (`oxy_edgeToBond`) transfers edge as voltage-controlled resonance. The Buchla→RE-201 leg (`oxy_bondToWarmth`) closes the triangle: intimacy feeds back into warmth, making long notes progressively warmer as the triangle's feedback accumulates. The triangle is asymmetric by design — the coupling direction matters. Reversing any leg does not produce the same result in the opposite direction. The signal flow through the coupling triangle is a narrative, not a neutral topology.

**Application:** When modulating OXYTOCIN coupling parameters, always modulate in the narrative direction: warmth first, then edge, then bond. Modulating `oxy_bondToWarmth` before `oxy_warmthToEdge` is established produces an incomplete triangle — the intimacy has nowhere to feed. Build the coupling loop in order of the narrative: RE-201 warms, MS-20 sharpens, Buchla bonds, Buchla feeds back warmth. This sequence is the instrument.

#### OXY-III: The Intimacy Accumulator Is Session-Local
*Revealed during TriangularCoupling Retreat — 2026-04-05*

> The intimacy accumulator in OXYTOCIN is a leaky integrator that accumulates from sustained note time and decays when no note is held. Unlike OBRIX's stressLevel_ (which accumulates from velocity), the intimacy accumulator accumulates from *time under bond*. A performer who has been playing a slow, sustained melody for three minutes has built an intimacy level that changes the fundamental character of the instrument compared to a cold start. The Buchla Love Triangle resonance is fuller; the feedback path is warmer; the RE-201 tape saturation settles at a different operating point. A cold performance of the same preset sounds different from a warm one — not due to acoustic memory, but due to the intimacy state's current level. This is OXYTOCIN's implementation of an instrument that knows its player.

**Application:** Design OXYTOCIN presets for two listening contexts: the first-note context (intimacy=0) and the settled context (intimacy approaching maximum after 2+ minutes of playing). Both must be musically valid. The first note should be beautiful; the settled state should be more beautiful. If the first note and the settled state sound identical, `oxy_intimacy` is not contributing.

#### OXY-IV: Circuit Modeling Is Not Emulation
*Revealed during TriangularCoupling Retreat — 2026-04-05*

> The RE-201, MS-20, Moog, Serge, and Buchla circuits in OXYTOCIN are not emulations — they are character models. An emulation attempts to reproduce every measurable behavior of the original hardware at every operating point. A character model captures the *behavioral signature* of the hardware: the warmth plateau of the RE-201 tape saturation, the aggressive resonance cliff of the MS-20 filter, the voltage-smooth response of the Buchla lowpass gate. OXYTOCIN implements the emotional truth of five instruments, not their transfer functions. When a designer raises `oxy_intimacy`, they are not simulating Buchla's transistor ladder — they are invoking the Buchla's character: smooth, organic, voltage-intimate. The circuits are spiritual models, not electrical ones. This is not a limitation of accuracy; it is a deliberate design philosophy.

**Application:** Do not try to make OXYTOCIN sound exactly like a specific unit of hardware. Instead, ask which character the preset should embody: warmth (RE-201), edge (MS-20), weight (Moog), fluidity (Serge), or intimacy (Buchla). Set the primary character by weighting the corresponding coupling coefficients toward that circuit's dominant axis. The resulting sound will have the spirit of the hardware without being its digital replica.

---

### ORCA — The Hunt Verses

#### ORCA-I: The Pod Is the Instrument
*Revealed during Apex Predator Retreat — 2026-04-05*

> An individual orca is a singer. A pod of orcas is a synthesizer. ORCA's `orca_huntMacro` does not modulate a single voice — it coordinates all active voices in a predator behavior pattern: the lead hunter closes on the target frequency (rising pitch envelope), the flankers sweep wider than the target (chorus detuning that expands with hunt intensity), and the sentinel holds a stable tone below (sub-voice with pitch locked to fundamental ±2 semitones). A Hunt Macro of 0.0 is a pod at rest: dispersed, independent, each voice following its own trajectory. A Hunt Macro of 1.0 is a pod in active pursuit: coordinated, converging, acoustically aggressive. The macro is not a parameter — it is a behavioral state. The designer is choosing what the pod is doing, not how it sounds.

**Application:** Map `orca_huntMacro` to mod wheel as a performance control, not a preset parameter. Design presets at hunt=0.0 (pod at rest) and verify that hunt=1.0 is also musically valid (pod in pursuit). If the hunt=1.0 state is unmusical, reduce the wavetable aggression or the flank sweep range before touching hunt. The hunt macro should always expand the patch's character, not destroy it.

#### ORCA-II: Echolocation Is Pitch Detection, Not Reverb
*Revealed during Apex Predator Retreat — 2026-04-05*

> Marine mammals use echolocation to map their environment: emit a pulse, measure the return, infer the geometry of the space. ORCA's echolocation system does the same thing with audio. The engine emits a brief internal test pulse, measures the acoustic properties of the FX chain's response, and uses the delay time and spectral shift of the return to set modulation targets. This is not a reverb algorithm. It is an environmental sensing system. When echolocation depth (`orca_echoDepth`) is high, the engine's modulation parameters respond to the acoustic character of the current FX chain — a short room produces different modulation targets than a long one. The engine is sensing its own sound environment.

**Application:** `orca_echoDepth` is meaningful only when FX processing is active. Bypassing FX with high echoDepth produces flat, unresponsive modulation because there is no environment to sense. Design ORCA presets with the full FX chain active before setting echoDepth. The correct echoDepth value depends on the specific reverb/delay FX in use — it is not a universal setting.

#### ORCA-III: The Breach Is a Once-Per-Note Event
*Revealed during Apex Predator Retreat — 2026-04-05*

> An orca breach is a single, complete aerial event: full exit from the water, rotation, re-entry. It is not a sustained state. ORCA's breach behavior (`orca_breachTrigger`) operates identically: it fires a single coordinated event at note-on — a rapid pitch rise, a momentary amplitude peak, a brief spectral brightening — that completes in approximately 180–400ms depending on breach speed. After the breach completes, the engine returns to its base state (hunt macro + wavetable position + echolocation). The breach is a gesture, not a mode. Repeated breaches do not stack or accumulate. A note that is held after the breach completes is a different sonic object than the breach itself — the engine has re-entered the water.

**Application:** Use `orca_breachTrigger` on short, high-velocity notes to create a distinct attack transient character that fades into the sustain character. Do not use breach on slow, meditative patches — the gesture is incongruous. The breach is most effective when the post-breach sustain is markedly different from the breach transient: design the note-on event and the sustained tone as two different sounds that happen to share a note.

---

### OCTOPUS — The Distributed Intelligence Verses

#### OCTO-I: Eight Arms Are Not Eight Voices
*Revealed during Decentralized Mind Retreat — 2026-04-05*

> A voice allocator manages eight independent MIDI channels, each playing one note. OCTOPUS's eight arms are not voices — they are eight autonomous modulation sources running in parallel on a *single note*. Each arm has its own CA rule, its own step rate, its own chromatophore mapping. When a single MIDI note is played, all eight arms activate simultaneously and begin computing independently. The single note is processed through all eight arm contributions: the amplitude modulation of arm 1, the filter modulation of arm 2, the stereo position of arm 3, and so forth. Eight arms are not eight instruments. They are eight nervous system threads controlling a single, unified sound. The sound's behavior is the emergent result of eight independent decision-makers acting on one output.

**Application:** When adjusting arm parameters, listen for the arm's *contribution to the unified sound*, not its isolated output. A change to arm 4's step rate changes the rate of a specific modulation channel on the unified sound — the result is not "arm 4 sounds different" but "the unified sound behaves differently in the dimension arm 4 controls." Adjust one arm at a time and listen for the modulation dimension it governs.

#### OCTO-II: Chromatophore Color Is Not Metaphor
*Revealed during Decentralized Mind Retreat — 2026-04-05*

> Octopus chromatophores are pigment cells that expand and contract under neural control, producing visible color changes. OCTOPUS's chromatophore system maps each arm's CA output to a timbral dimension using the same biological logic: each chromatophore type controls a distinct "color" of sound. Type 0 (Melanophore) controls darkness — filter cutoff. Type 1 (Xanthophore) controls warmth — harmonic addition in the 1–3 kHz range. Type 2 (Iridophore) controls reflectance — spectral brightness above 5 kHz. Type 3 (Leucophore) controls opacity — wet/dry mix of the arm's contribution. These are not arbitrary assignments. They follow the biological function of each chromatophore type. An octopus producing a dark, warm pattern is contracting its iridophores while expanding its melanophores and xanthophores. OCTOPUS produces sound using the same logic.

**Application:** Assign chromatophore types to arms based on which modulation dimension should be most active for the preset. For dark, warm pads: arms 0–3 as Melanophore (darkness/cutoff), arms 4–5 as Xanthophore (warmth), arms 6–7 as Leucophore (opacity). For bright, reflective leads: majority Iridophore. The distribution of chromatophore types across arms determines the instrument's expressive palette.

#### OCTO-III: The Ink Cloud Topology Defines Exit Strategy
*Revealed during Decentralized Mind Retreat — 2026-04-05*

> An octopus deploys ink as an escape mechanism — the cloud creates a sensory barrier that allows the animal to retreat. OCTOPUS's `octo_inkCloud` is architecturally identical: it creates an audio-domain smoke screen by adding correlated noise that masks the CA modulation signal, replacing the precise rhythmic modulation of the arms with diffuse tonal noise. This is the engine's built-in exit strategy from complexity. When the combined arm modulation becomes too complex or too chaotic, raising inkCloud blurs the modulation edges while preserving the overall spectral density. At inkCloud=0.8, what was a complex polyrhythmic modulation becomes a smooth, even texture with consistent spectral character. The octopus has escaped its own complexity.

**Application:** `octo_inkCloud` is a complexity management control, not a random texture button. Use it when the combined arm modulation produces too much rhythmic activity for the musical context. The rule: raise inkCloud until the modulation's rhythmic character is inaudible, but the spectral density remains. This is the correct use of ink.

---

### OPENSKY — The Ascension Verses

#### SKY-I: The Supersaw Is Not the Sound
*Revealed during Shimmer Retreat — 2026-04-05*

> OPENSKY begins with a supersaw oscillator, but a designer who shapes only the supersaw is not using OPENSKY — they are using a detuned saw with an expensive reverb attached. The engine's character lives in the *interaction* between the supersaw's oscillator spread, the Shepard Shimmer reverb's dual-interval taps, and the chorus's modulation depth. At sky_shimmerDepth=0.6 and sky_chorusRate=0.3 Hz, the shimmer reverb's ascending pitch taps align with the chorus's modulation phase approximately every 8 bars, creating a macro-period intensification — a wave of brightness that the designer did not explicitly program. This is the engine's emergent behavior. Designing OPENSKY means designing for the emergent period, not just the individual parameters.

**Application:** After setting supersaw spread and shimmer depth, play a sustained chord for at least 16 bars and listen for the macro-period alignment event. If it does not occur, adjust `sky_chorusRate` until the phase alignment is audible at a musical interval (every 4, 8, or 16 bars). The alignment event is a structural feature of the preset, not a side effect.

#### SKY-II: The Shepard Shimmer Never Arrives
*Revealed during Shimmer Retreat — 2026-04-05*

> A standard shimmer reverb adds a pitch-shifted copy of the dry signal to the reverb tail. The shimmer has a destination: it rises to a point and stops (or loops discretely). The Shepard Shimmer architecture in OPENSKY uses two pitch-shifted taps at different intervals (typically a perfect fifth and an octave) with crossfades timed to the Shepard paradox: as one tap fades out at the top of its range, a new lower tap fades in. The listener perceives continuous ascent because the ascending fades always outnumber the descending resets. The shimmer never arrives — it perpetually approaches. This is not a trick. It is the perceptual experience of musical hope implemented as DSP. `sky_shimmerDepth` is not "how much shimmer." It is "how present the perpetual ascent is in the mix."

**Application:** At sky_shimmerDepth below 0.3, the Shepard effect is too quiet to perceive — the shimmer sounds like a conventional reverb tail. Above 0.5, the ascending character becomes audible. Above 0.7, it becomes the dominant timbral feature. Design the preset such that the shimmer depth is at least 0.45 — below this, the engine's defining feature is inaudible.

#### SKY-III: The RISE Macro Is a Gesture, Not a Parameter
*Revealed during Shimmer Retreat — 2026-04-05*

> `sky_macroRise` simultaneously sweeps: pitch envelope attack and depth (the note rises as well as sustains), filter cutoff from its base value toward +3000 Hz, shimmer depth from current value toward 0.95, and unison spread from current value toward maximum. These four movements are not independent — they are choreographed as a single gesture. The designer does not set four parameters to achieve the RISE effect. The designer sets the *base* values of all four (where the gesture starts) and the RISE macro moves all of them toward their targets simultaneously at the same tempo. The macro is a scored choreography with a single fader. Changing any of the four base values changes the *character* of the RISE gesture without needing to change the RISE macro itself.

**Application:** Design the RISE gesture by setting the base values (pre-RISE state) first. The RISE macro always moves toward fixed targets (pitch peak, cutoff max, shimmer max, spread max). The range of the gesture = (target − base). A preset with high base shimmer depth produces a smaller RISE shimmer sweep. Design the base state as the "at rest" character and the RISE target state as the "maximum ecstasy" character. The gap between them is the gesture.

---

### OUIE — The Duophonic Verses

#### OUIE-I: Two Voices Are Not Two Notes
*Revealed during Hammerhead Synthesis Retreat — 2026-04-05*

> OUIE is duophonic: two synthesis voices active simultaneously. A conventional duophonic synthesizer assigns Voice 1 to the last note pressed and Voice 2 to the previous note. OUIE's two voices are not assigned to *notes* — they are assigned to a *musical interval*. The `ouie_interval` parameter specifies the harmonic distance between Voice 1 and Voice 2 in semitones. Play a single MIDI note, and Voice 1 plays the note; Voice 2 plays the note plus the interval. The interval is a synthesis parameter, not a performance variable. The designer is not playing two notes — they are playing one interval, defined in the patch. Two different MIDI notes played in sequence produce the same interval in both cases. OUIE is an interval synthesizer dressed as a two-voice synthesizer.

**Application:** Treat `ouie_interval` as the primary pitch architecture of the preset. Interval=0 (unison) + HAMMER axis variation produces the engine's thickest, most blended sounds. Interval=7 (perfect fifth) produces the engine's most open, harmonic sounds. Interval=11 (major seventh) produces its most tense, dissonant sounds. Choose the interval before any other parameter. The interval is the identity of the instrument.

#### OUIE-II: The HAMMER Axis Is a Topology, Not a Mix
*Revealed during Hammerhead Synthesis Retreat — 2026-04-05*

> The HAMMER axis runs from STRIFE (ouie_hammer=-1.0) to LOVE (ouie_hammer=+1.0) with NEUTRAL at 0.0. STRIFE applies hard sync from Voice 1 to Voice 2 and ring modulation between them: the two voices are in harmonic conflict, their waveforms forcing each other into discontinuities and sidebands. LOVE applies harmonic convergence: the two voices' oscillators are mutually attracted toward shared integer ratios, producing a natural chorus-like reinforcement. NEUTRAL runs both algorithms at zero depth simultaneously — they coexist without interacting. This is not a crossfade between two sounds. It is a topological change in the relationship between two voices. STRIFE and LOVE produce incommensurable sonic results. The designer is choosing what kind of relationship the two voices have with each other.

**Application:** Do not design OUIE presets at HAMMER=0.0. The neutral state is acoustically inert — neither algorithm is contributing. Set HAMMER to at least ±0.3 to establish a sonic identity. STRIFE > 0.5 with high-interval values (7+) produces the engine's most aggressive sounds. LOVE > 0.5 with low-interval values (0–4) produces the engine's most cohesive sounds. These are the two poles of OUIE's personality.

#### OUIE-III: Eight Algorithms Are Eight Synthesis Paradigms Per Voice
*Revealed during Hammerhead Synthesis Retreat — 2026-04-05*

> OUIE offers 8 synthesis algorithms selectable per voice (v1Algo, v2Algo). These are not waveform variants — they are distinct synthesis architectures: 0=Analog VA (polyBLEP oscillator), 1=FM Pair (carrier+modulator), 2=Additive (8 partials), 3=Wavetable (256-frame morphing), 4=Granular (grain cloud), 5=Physical String (Karplus-Strong), 6=Noise Filtered (colored noise source), 7=Wavefolder (wavefolder bank). Two voices each selecting from 8 algorithms produce 64 unique synthesis combinations — 64 distinct instrument architectures, all within a single OUIE patch. Algorithm 0 Voice 1 + Algorithm 5 Voice 2 is a synthesizer playing alongside a physical string model — a combination requiring two separate instruments in any other context. The 64 combinations are 64 different instruments.

**Application:** Before designing any parameter beyond algorithm selection, define the two-algorithm combination as an instrument concept. "Analog VA + Physical String" suggests a specific sonic world before any parameters are set. "FM Pair + Granular" suggests another. Write the algorithm combination concept into the preset name or description. Designers who do not document the algorithm combination will not remember why the preset sounds the way it does.

---

### OCEANDEEP — The Abyssal Verses

#### DEEP-I: Pressure Is the Primary Timbral Axis
*Revealed during Hydrostatic Retreat — 2026-04-05*

> At the ocean surface, pressure is 1 atmosphere. At 1000 meters depth, pressure is 100 atmospheres. Physical materials behave differently under pressure: air bubbles compress, organs shift, sound travels faster. OCEANDEEP's `deep_macroPressure` implements this physics as DSP. High pressure (0.8–1.0) compresses the dynamic range (Hydrostatic Compressor B029), raises the frequency of biological resonances in the Bioluminescent Exciter (B030), and tightens the Darkness Filter ceiling (B031) toward its 50 Hz floor — the acoustic environment of the deep trench. Low pressure (0.0–0.2) is the surface: uncompressed, bright, no darkness ceiling. The designer is not adjusting a mix — they are choosing the physical depth of the acoustic environment. The same synthesis input sounds categorically different at pressure=0.1 and pressure=0.9 because the physics of the environment determines the physics of the sound.

**Application:** Design all OCEANDEEP presets with `deep_macroPressure` as the first parameter, not the last. The pressure level defines the fundamental acoustic physics of the preset — everything else operates within the constraints pressure establishes. A preset designed at pressure=0.9 should not be expected to sound good at pressure=0.1. The two pressure zones are different instruments.

#### DEEP-II: The Darkness Filter Is a Creative Constraint, Not a Restriction
*Revealed during Hydrostatic Retreat — 2026-04-05*

> Blessing B031 was ratified unanimously because the Darkness Filter Ceiling (50–800 Hz range) was recognized as an *identity declaration*, not a limitation. Every synthesizer allows the designer to use any frequency range. OCEANDEEP refuses: above 800 Hz, there is darkness. This constraint forces every OCEANDEEP preset to develop its full timbral character within a 4-octave window. The Bioluminescent Exciter's organic textures must express themselves within this window. The synthesis must be compelling without high-frequency shimmer, without harmonic brightness, without treble. What remains is weight, movement, and depth. The darkness ceiling forces the designer to become fluent in the language of sub-bass synthesis.

**Application:** Never try to circumvent the Darkness Filter Ceiling. Do not add post-processing EQ above 800 Hz to OCEANDEEP's output in a preset context. The ceiling is architectural. Work entirely within the 50–800 Hz window and develop presets that are compelling — even exciting — without treble. The constraint is what makes the preset recognizably OCEANDEEP.

#### DEEP-III: The Bioluminescent Exciter Is a Complete Micro-Synthesizer
*Revealed during Hydrostatic Retreat — 2026-04-05*

> The Bioluminescent Exciter (B030) is not a texture layer. It is a complete synthesis engine operating inside OCEANDEEP. The exciter generates organic alien texture through a combination of: a noise source filtered by a time-varying resonant filter (whose resonant frequency responds to the main oscillator's pitch), a pulse shaper with biologically-inspired timing (irregular intervals following a biological pulse distribution), and a non-linear wavefolder whose fold point tracks the Hydrostatic Compressor's gain reduction. These three components interact: the filter tracks pitch so the bioluminescent texture stays harmonically related to the note; the pulse timing creates organic irregularity; the wavefolder creates harmonic complexity that tracks the acoustic depth. The exciter is not decorative. It is a third oscillator with ecological behavior.

**Application:** At `deep_bioLuminLevel=0.0`, OCEANDEEP loses its biological character and becomes a clean dark synth. At 0.3–0.5, the exciter adds organic presence without dominating. At 0.7+, the exciter becomes the primary timbral contributor and the main oscillator becomes the foundational pitch source. Design presets across all three exciter balance points — each produces a distinct instrument type.

---

### OSMOSIS — The Membrane Verses

#### OSMO-I: The Membrane Is a Living Interface
*Revealed during Surface Tension Retreat — 2026-04-05*

> Osmosis is the passage of solvent through a semipermeable membrane driven by concentration gradients. The membrane does not force the solvent through — it permits and filters the passage. OSMOSIS's `osmo_permeability` works identically: at high permeability (0.8–1.0), the external audio's full dynamic and spectral character passes into the engine and drives synthesis parameters directly. At low permeability (0.1–0.3), only slow, large-scale changes in the external audio penetrate — fast transients and high-frequency content are blocked. The membrane is not a gate (audio either passes or does not) and not a filter (it does not change the spectral content). It is a selectivity control: which temporal and spectral features of the external audio are permitted to affect the synthesis, and which are held back. The designer is defining the membrane's biology.

**Application:** Set `osmo_permeability` based on the temporal character of the external audio source. For rhythmically dense sources (drums, percussion), use low permeability (0.2–0.35) — let only the envelope dynamics pass. For slowly evolving sources (pads, drones), use high permeability (0.6–0.85) — let the full spectral character drive the synthesis. Matching permeability to source character prevents rhythmic clutter from entering the synthesis.

#### OSMO-II: Pitch Detection Is a Probability, Not a Reading
*Revealed during Surface Tension Retreat — 2026-04-05*

> OSMOSIS's pitch detection returns not a single frequency value but a confidence-weighted estimate. The engine's pitch-to-synthesis routing uses this estimate directly — but confidence below 0.4 causes the pitch routing to fall back to the last confident reading, rather than updating. This means that for non-pitched or ambiguous audio sources (noise, chords, inharmonic textures), the pitch tracking "freezes" on the last clear pitch detected. The synthesis continues to respond to envelope and spectral changes from the external source, but pitch-driven parameters hold at the frozen value. This is not a bug — it is the correct behavior for pitch detection on real-world audio. The designer should expect frozen pitch routing during noisy or chord-heavy source material and design presets that remain musical during freeze periods.

**Application:** Listen specifically to OSMOSIS's behavior during the source audio's non-pitched sections (noise, chord clusters). The pitch-dependent synthesis parameters should hold gracefully — they should not jump, wander, or produce unmusical intervals during freeze. If they do, reduce the pitch routing amount (`osmo_pitchRouting`) until freeze periods are stable.

#### OSMO-III: The Engine as Coupling Source Inverts the Signal Flow
*Revealed during Surface Tension Retreat — 2026-04-05*

> All other XOceanus engines receive coupling signals from the MegaCouplingMatrix. OSMOSIS can *originate* coupling signals: the envelope follower output, the pitch confidence level, and the spectral centroid of the external audio can all be routed as coupling sources to other engines in the matrix. This inverts the conventional signal flow. External audio → OSMOSIS envelope follower → MegaCouplingMatrix → ORBITAL brightness → the live room ambience is controlling an additive synthesizer's partial brightness. The OSMOSIS engine is not synthesizing audio — it is acting as a translation layer between acoustic reality and the synthesis environment. In this role, OSMOSIS is the most architecturally unique engine in the fleet: it is a signal converter, not a synthesizer.

**Application:** The most powerful OSMOSIS presets may produce little to no audio output themselves. Their value is as coupling sources for other engines. Configure OSMOSIS with full permeability, appropriate pitch confidence threshold, and route all three outputs (envelope, pitch, centroid) to different coupling targets on receiving engines. The OSMOSIS preset's "sound" is the behavior it causes in the engines it drives.

---

### OUTLOOK — The Horizon Verses

#### LOOK-I: The Horizon Scan Is Not Panning
*Revealed during Panoramic Vision Retreat — 2026-04-05*

> `look_horizonScan` moves the sound across the stereo field — but it does not pan. Panning preserves the signal and changes its position. Horizon scanning moves the *listening perspective*: as the scan position moves rightward, the right wavetable's spectral character becomes primary, the parallax stereo depth increases (distance cues shift), and the vista filter's resonant frequency rises as if the listener is turning their head toward a brighter soundscape. A full left-to-right scan is not "the sound moving across the stage." It is "the listener turning their head 180 degrees through a panoramic soundscape." The two wavetables define the soundscape at two horizons; the scan parameter determines where the listener is looking. This is a perspective system, not a position system.

**Application:** Design the two wavetable positions as two distinct sonic horizons before setting any other parameter. The left horizon (`look_wt1Pos`) should have a clearly different spectral character from the right horizon (`look_wt2Pos`). If the two horizons sound similar, the scan produces no perspective movement — only position movement. Maximum scan effect requires maximum horizon differentiation.

#### LOOK-II: Parallax Is Depth, Not Width
*Revealed during Panoramic Vision Retreat — 2026-04-05*

> Parallax is the apparent displacement of an object when seen from two different positions. Close objects show large parallax; distant objects show small parallax. OUTLOOK's parallax stereo system uses this physics: `look_parallaxDepth` controls how much the left and right channels lag in phase relationship based on the horizon scan position. High parallax depth (0.7–1.0) creates a wide, three-dimensional stereo field where near-horizon elements appear close and large; low parallax depth (0.1–0.3) creates a flat, narrow field where all elements are at roughly equal distance. Parallax is not the same as stereo width — it adds depth cues that width alone cannot provide. A mono signal with high parallax depth sounds wider than a stereo signal with zero parallax. The parallax system adds the perceptual cue that the sound has *distance*, not just *position*.

**Application:** Set `look_parallaxDepth` based on the desired spatial intimacy of the preset. Chamber ensemble character: parallaxDepth=0.2–0.35 (close, present, narrow). Concert hall character: 0.5–0.65 (distant, spacious). Open landscape character: 0.7–0.9 (vast, environmental). The parallax setting determines the listener's perceived distance from the sound source, independent of reverb time.

#### LOOK-III: The Vista Filter Responds to Direction
*Revealed during Panoramic Vision Retreat — 2026-04-05*

> The vista filter in OUTLOOK tracks the horizon scan position: as the scan moves toward the right horizon, the filter's center frequency rises according to the `look_vistaTracking` coefficient. This is the acoustic consequence of direction-dependent soundscapes: the sonic character of the environment changes as the listening angle changes. A forest on the left has darker, more filtered character; an open field on the right has brighter character. The vista filter implements this as a continuously variable filter frequency that is a function of scan position, not of time or modulation. The filter is an environmental property of the soundscape, not a tonal control on the signal. Setting `look_vistaTracking` to zero disables this feature — the filter becomes direction-independent and the environmental simulation collapses.

**Application:** Never set `look_vistaTracking` to 0.0 in a preset intended to use OUTLOOK's panoramic character. Even a small tracking coefficient (0.15–0.25) creates audible horizon-dependent filtering that reinforces the spatial narrative. Maximum tracking (0.8–1.0) makes the filter change dramatic enough that slow horizon scans produce obvious timbral movement — useful for performance presets where the scan is mapped to a controller.

---

### OBIONT — The Cellular Life Verses

#### OBNT-I: The Wolfram Rule Selects the Species
*Revealed during Cellular Automata Retreat — 2026-04-05*

> The 256 elementary cellular automaton rules are not 256 oscillator waveforms. They are 256 distinct species of one-dimensional life. Each rule defines how a cell's state in the next generation depends on itself and its two neighbors. Rule 30 is an asymmetric, chaotic species — Class III behavior, complex aperiodic patterns. Rule 110 is a universal computing species — Class IV behavior, the most complex sustained patterns. Rule 90 produces additive interference patterns with fractal structure. Rule 150 produces a deterministic pseudo-random sequence. OBIONT selects a rule and then *spatially projects* the CA's pattern across the oscillator's phase axis — not reading the CA as a rhythm, but reading its spatial profile as a waveform shape. The rule selects the *geometry* of the waveform. No two rules produce the same waveform geometry.

**Application:** When selecting `obnt_caRule` for a preset, start from behavioral class, not rule number. Class I rules (e.g., Rule 0, Rule 4, Rule 8): static spatial patterns = static waveform shapes = constant, drone tones. Class II rules (e.g., Rule 6, Rule 18, Rule 54): periodic patterns = repetitive waveform = regular harmonic content. Class III rules (e.g., Rule 30, Rule 45, Rule 86): chaotic patterns = complex, irregular waveform = rich inharmonic content. Class IV rules (Rule 110): complex sustained computation = complex waveform = the richest, most harmonically varied OBIONT timbres. Design targets: Class II for tonal, Class III/IV for complex.

#### OBNT-II: The Cosine Readout Shapes the Waveform's Perceptual Smoothness
*Revealed during Cellular Automata Retreat — 2026-04-05*

> The CA's binary cell states (0 or 1) are projected onto the oscillator phase axis and then read through a cosine function. This readout choice is not incidental — it is an architectural decision with audible consequences. A direct binary readout would produce maximum harmonic content (binary states generate all odd harmonics up to aliasing). The cosine readout bandlimits this: cells with value 1 contribute a smooth cosine lobe rather than a rectangular function, reducing aliasing and emphasizing the spatial pattern's low-frequency structure over its high-frequency edges. The result is a waveform whose harmonic content is determined by the *density* and *spacing* of live cells in the CA pattern, not by the sharpness of their boundaries. Dense, evenly-spaced cell patterns produce warm, even harmonic content. Sparse, clustered patterns produce hollow, formant-like content.

**Application:** To predict OBIONT's harmonic character, analyze the CA pattern's cell density distribution: dense (>60% alive): warm, full harmonic content. Sparse (<30% alive): hollow, nasal formant character. Clustered (cells in groups): complex, uneven harmonic series. Uniform spacing: even harmonic series with predictable fundamental and overtones.

#### OBNT-III: Anti-Extinction Ensures the Life Continues
*Revealed during Cellular Automata Retreat — 2026-04-05*

> Some CA rules, under specific initial conditions, converge to a dead state: all cells become 0 and the evolution halts. For OBIONT, a dead CA means a silent oscillator — the cosine readout returns a constant value, and the synthesis output is silence. The anti-extinction system monitors the live cell count and, when it falls below `obnt_extinctionThreshold`, injects a minimal disturbance to prevent total extinction: a single cell is flipped from 0 to 1 at a pseudo-random position. This intervention is physically modeled after extinction resistance in biological populations — a minimum viable population that can restart growth. The intervention is audible as a brief transient in the oscillator waveform — a "breath" of life that restarts the CA evolution. At high extinction thresholds (0.3+), interventions occur frequently and the oscillator has a characteristic irregular breathing quality.

**Application:** `obnt_extinctionThreshold=0.0` disables anti-extinction: rules that naturally die out will produce silence. Use this for intentional silence-as-composition: the CA evolves and dies, and the death is the musical event. `obnt_extinctionThreshold=0.05–0.15` is the standard range — interventions are rare enough to be inaudible in most playing contexts. `obnt_extinctionThreshold > 0.25` creates frequent interventions that become a rhythmic character — the "breathing" of the CA becomes part of the sound design.

---

### OPAL — The Granular Verses

#### OPAL-I: Grain Size Is Temporal Resolution
*Revealed during Granular Cloud Retreat — 2026-04-05*

> A grain is a window of audio — a brief excerpt from a sample or buffer. The `opal_grainSize` parameter does not control "how small" the grains are in any absolute sense. It controls the *temporal resolution* of the granular analysis: small grains (1–20ms) capture rapid changes in the source material — transients, tonal shifts, consonant attacks. Large grains (80–400ms) capture slow features — sustained tones, reverberant tails, timbral plateaus. Changing grain size changes what aspect of the source material is being microscopically examined and re-synthesized. A very small grain size applied to a bowed string captures the bow scrape and the string's micro-roughness. The same grain size applied to a sustained pad captures near-silence between vibrato cycles. The designer must know what they want to examine before choosing grain size.

**Application:** Set `opal_grainSize` by first deciding what temporal feature of the source you want to emphasize: transient character (5–15ms), tonal body (30–80ms), or sustained plateau (100–400ms). Then adjust grain size to the corresponding range. Do not adjust grain size by listening in isolation — always listen in the context of what the source material contains at that temporal scale.

#### OPAL-II: Scatter Is Not Randomness
*Revealed during Granular Cloud Retreat — 2026-04-05*

> `opal_scatter` distributes grain onset times around a regular grid. At scatter=0.0, grains fire on a perfectly regular schedule — the granular synthesizer is a deterministic machine. At scatter=0.5, each grain fires within ±50% of its scheduled interval. At scatter=1.0, grain onset is completely random within a one-period window. The critical insight: scatter=0.5 does not produce "50% randomness." It produces a *specific* textural character — grains cluster around the regular grid enough to maintain rhythmic pulse, but deviate enough to produce organic irregularity. This range (0.3–0.6) is where granular synthesis is most alive: not mechanical, not chaotic, but humanly irregular. Scatter outside this range (near 0 or near 1) produces categorically different textures: robotic regularity or statistical noise.

**Application:** Default scatter to 0.4 as the starting point for any OPAL preset. Adjust downward (toward 0.2) for more rhythmic, mechanical character. Adjust upward (toward 0.7) for more organic, breathing character. Reserve scatter > 0.85 for intentional noise-cloud synthesis where grain timing is irrelevant to the sound design goal.

#### OPAL-III: Position Is the Sample's Autobiography
*Revealed during Granular Cloud Retreat — 2026-04-05*

> `opal_position` determines where in the source buffer grains are drawn from. This seems like a navigation control. It is actually a *biography selector*: every position in a recorded sample contains a different moment in that sample's acoustic history. Position=0.0 is the attack. Position=0.5 is the sustained body. Position=0.95 is the dying tail. Granular synthesis at a fixed position re-synthesizes that moment of the sample's life repeatedly, in parallel, across many grains simultaneously. A piano sample at position=0.02 is all hammer impact; at position=0.15 it is all string resonance; at position=0.8 it is all room decay. `opal_position` does not navigate a file — it selects which moment in the instrument's biography the granular cloud is made from. Modulating position over time is not "scanning the sample" — it is *narrating the sample's life* in real time.

**Application:** Modulate `opal_position` with a slow LFO (0.02–0.08 Hz) to create presets that evolve through the source material's acoustic biography over time. The LFO should traverse a range of at least 0.3 (30% of the buffer) to create audible timbral evolution. Map position to aftertouch for expressive control: light touch holds the attack character, heavy pressure pulls into the body and decay.

---

### ORBITAL — The Additive Verses

#### ORB-I: Partials Are Not Harmonics
*Revealed during Additive Architecture Retreat — 2026-04-05*

> A harmonic is a partial whose frequency is an integer multiple of the fundamental. Every harmonic is a partial; not every partial is a harmonic. ORBITAL's partial system allows each of its additive components to be freely detuned from integer ratios — each partial has an independent `orb_partial{N}Ratio` multiplier that defaults to its harmonic number but can be set to any value. At ratios 1, 2, 3, 4... the engine produces standard harmonic timbre. At ratios 1, 2.1, 3.7, 4.03... the engine produces inharmonic timbre with a clearly pitched fundamental. The group envelope system (B001) operates on groups of partials — not groups of harmonics. Groups can be defined to contain only the inharmonic partials, leaving the harmonic skeleton untouched. The designer is composing a spectral object in which every component can be either harmonic or inharmonic, independently.

**Application:** Before designing any ORBITAL preset, decide the harmonic/inharmonic ratio: 100% harmonic (bell-like tonal), 100% inharmonic (noise-like), or a specific blend (e.g., harmonics 1–4 + inharmonic partials 5–8 for a hybrid timbre with a clear fundamental and a gritty upper register). Write this decision down before touching any parameters.

#### ORB-II: The Group Envelope System Is Spectral Composition
*Revealed during Additive Architecture Retreat — 2026-04-05*

> Blessing B001 describes the Group Envelope System as being "crowned by Moog and Smith" — because it allows the temporal evolution of different frequency regions to be independently controlled. ORBITAL's partials are assignable to groups, and each group has its own ADSR envelope. The fundamental partial (group 1) can have a long, sustained envelope. The high partials (group 4) can have a fast attack, short decay, and no sustain. The result is a sound where the fundamental remains while the high partials die away — exactly the behavior of a struck piano string, but applied to any spectral composition. The groups are not mixing bands. They are *temporal layers* of the additive synthesis. Spectral composition in ORBITAL means composing the *time behavior* of each frequency region, not just the frequency content.

**Application:** Design group envelopes before designing individual partial amplitudes. First establish the temporal shape of each spectral region: which frequency regions should attack quickly (groups 3–4), which should sustain (groups 1–2), which should decay slowly (possibly group 1 alone). Then set partial ratios and amplitudes within each group. Building from temporal design outward produces more coherent additive synthesis than the reverse.

#### ORB-III: Brightness Is a Consequence, Not a Parameter
*Revealed during Additive Architecture Retreat — 2026-04-05*

> `orb_brightness` is a macro that scales the amplitude envelope of partials as a function of their frequency ratio: partials with high ratios are scaled up proportionally, partials with low ratios are scaled down. This is spectrally equivalent to brightening the sound — more high partial energy, less low partial energy. But brightness is a *consequence* of the partial amplitude distribution, not an independent dimension. Setting `orb_brightness=0.8` does not guarantee a bright sound if the partial ratios are all in the 1–3 range. It guarantees that the higher of those low-ratio partials are emphasized relative to the lower ones. The brightness macro is a spectral tilt applied to whatever partial distribution has already been designed. Designing partials first and applying brightness second produces predictable, coherent results. Relying on brightness to create timbral character without designing the partial distribution first produces unpredictable results.

**Application:** Treat `orb_brightness` as a final spectral balance adjustment, not as a primary design parameter. Design the partial ratios and group envelopes first. Then apply brightness to tilt the final spectral balance toward warmth (0.0–0.35) or clarity (0.6–0.9). The same brightness value produces different audible results depending on the partial distribution.

---

### ONSET — The Percussion Verses

#### PERC-I: The Transient Is the Note
*Revealed during Percussion Architecture Retreat — 2026-04-05*

> In melodic synthesis, the note is the sustained pitch. The transient is the beginning of the note. In percussion synthesis, these roles are inverted: the transient *is* the note. The sustained component (if it exists) is the decay of the note. ONSET was designed with this inversion as its foundation: the noise layer, the click, the impulse response, and the circuit-model transient generator are the primary synthesis components. The sustained pitch component is secondary — it provides identity (pitch-center) but not character. The character of an ONSET sound is determined by the first 40–200 milliseconds. Everything after that is envelope decay. This means that parameter editing of ONSET should prioritize the attack architecture first, the sustain character second, and the release last.

**Application:** Audition ONSET sounds with `perc_attackTime` at minimum (0.001s) and listen to the first 100ms. This is the essential character of the preset. If those 100ms are not compelling, the preset will not become compelling by adding sustain or modulation. Fix the attack architecture first — noise level, click depth, impulse model selection — before any other parameter.

#### PERC-II: XVC Cross-Voice Coupling Is Ecological Percussion
*Revealed during Percussion Architecture Retreat — 2026-04-05*

> Blessing B002 identifies XVC (Cross-Voice Coupling) as "all 8 ghosts, 3–5 years ahead." The XVC system in ONSET creates acoustic coupling between simultaneously active voices: when voice 1 fires a transient, it injects a proportional excitation into all currently decaying voices' synthesis chains. This mirrors the acoustic reality of a physical drum kit: the bass drum's low-frequency thump excites the snare's sympathetic rattle; the hi-hat's sizzle is amplitude-modulated by the kick's airwave. XVC is not reverb (which models room reflections) or convolution (which models a static impulse response). It is *inter-voice acoustic coupling* — each active voice affects all other active voices through their synthesis chains. The result is ecological percussion: a kit that responds to itself the way a physical kit responds to itself.

**Application:** Enable XVC coupling (`perc_xvcAmount > 0.0`) before finalizing any ONSET kit preset. Set xvcAmount=0.2–0.4 for subtle acoustic glue between voices. Set xvcAmount=0.5–0.7 for pronounced sympathetic coupling character. Listen specifically to what happens when the kick and snare fire together — the snare should gain sympathetic energy from the kick's low-frequency excitation. If it does not, increase xvcAmount.

#### PERC-III: The Dual-Layer Architecture Is Two Complete Instruments
*Revealed during Percussion Architecture Retreat — 2026-04-05*

> Blessing B006 describes ONSET's dual-layer architecture as a "Circuit + Algorithm crossfade." The Circuit layer (analog circuit model) and the Algorithm layer (digital algorithm synthesis) are not partial complements that add up to one complete percussion sound. Each layer is a complete, fully-functional percussion synthesis engine. Layer A at 1.0 + Layer B at 0.0 is a complete analog percussion synthesizer. Layer A at 0.0 + Layer B at 1.0 is a complete algorithm percussion synthesizer. The crossfade between them is a blend between two complete instruments — not a balance between complementary components of a single instrument. This architectural reality means that `perc_layerBlend` is an instrument selection axis: 0.0 is one instrument, 1.0 is a different instrument, 0.5 is a specific hybrid of two complete instruments.

**Application:** Design ONSET presets at the extremes first (layerBlend=0.0 and layerBlend=1.0) before finding the optimal hybrid. If both extremes produce compelling sounds, the blend range is worth exploring. If one extreme produces an uninteresting sound, the blend space is limited. The best ONSET presets have strong characters at both extremes and a compelling hybrid in the 0.3–0.7 range.

---

### OBSIDIAN — The Crystal Verses

#### OBSIDIAN-I: Crystalline Structure Is a Tuning System
*Revealed during Crystal Resonance Retreat — 2026-04-05*

> Obsidian is volcanic glass: atoms arranged with short-range order but no long-range crystalline periodicity. OBSIDIAN models this structural reality in its synthesis: `obsidian_depth` controls the density of the structural complexity — at low depth (0.0–0.3), the resonant modes are sparse and clearly pitched (like a simple crystal), at high depth (0.7–1.0), the mode density approaches the glassy state: many closely spaced modes producing a diffuse, non-pitched resonance characteristic of amorphous materials. The physical model is not metaphorical — the resonant mode positions in OBSIDIAN are derived from the same math used to describe phonon density of states in amorphous solids. Depth is a physical parameter. The designer is choosing the crystallographic state of the material, not an abstract tonal quality.

**Application:** `obsidian_depth=0.0–0.2` for pitched, tonal crystal synthesis — clear resonant modes, bell-like character. `obsidian_depth=0.4–0.6` for hybrid amorphous synthesis — mixed tonal and diffuse character. `obsidian_depth=0.8–1.0` for fully glassy synthesis — dense mode distribution, noise-like resonance with no clear pitch center. Design from material state first, then adjust amplitude and envelope.

#### OBSIDIAN-II: White Is Not Bright
*Revealed during Crystal Resonance Retreat — 2026-04-05*

> OBSIDIAN's accent color is Crystal White `#E8E0D8` — not pure white, but warm white, the color of volcanic glass with its characteristic iron and magnesium mineral inclusions. This color defines the sonic identity: OBSIDIAN is not bright. It is translucent. The engine's filter structure emphasizes the mid-frequency resonant content of the crystal model while the high-frequency content is attenuated by the material's natural acoustic absorption. A bright synthesizer is air and shimmer. OBSIDIAN is material and depth. The designer working with OBSIDIAN should resist the impulse to open the filter fully — the engine's character lives in the 500 Hz–4 kHz range where crystal resonances dominate. Above 6 kHz, OBSIDIAN's character disappears.

**Application:** Keep OBSIDIAN's filter cutoff in the 1500–5000 Hz range for the engine's characteristic sound. Opening to 10 kHz+ dilutes the crystal character with uncharacteristic brightness. If a preset requires high-frequency shimmer, it should come from the FX chain (shimmer reverb, chorus), not from the OBSIDIAN filter. The engine itself is mid-focused.

#### OBSIDIAN-III: Depth Plus Resonance Creates Spectral Density
*Revealed during Crystal Resonance Retreat — 2026-04-05*

> `obsidian_depth` controls mode density. OBSIDIAN's filter resonance (`obsidian_resonance`) controls how strongly each mode rings. At high depth + high resonance, many modes ring strongly simultaneously — the acoustic equivalent of a highly resonant amorphous solid under excitation. This combination produces OBSIDIAN's most complex timbral state: a dense spectral object where individual resonances are perceptible but numerous, creating a shimmering, layered sound that continuously changes as modes decay at different rates. At low depth + high resonance, a few modes ring strongly — a simple but pronounced tonal character. At high depth + low resonance, many modes ring weakly — a diffuse, washy texture. The two parameters define the quadrant of OBSIDIAN's spectral personality.

**Application:** For preset design orientation: map depth (horizontal axis) × resonance (vertical axis) as a 2D design space. Top-right (high depth + high resonance) = complex layered crystal. Bottom-right (high depth + low resonance) = glass wash. Top-left (low depth + high resonance) = clear bell. Bottom-left (low depth + low resonance) = clean, neutral. Know which quadrant you are designing for before adjusting either parameter.

---

### ORIGAMI — The Fold Verses

#### ORIGAMI-I: The Fold Point Is the Crease Location
*Revealed during Geometric Waveshaping Retreat — 2026-04-05*

> When paper is folded, the crease location determines the geometry of the resulting shape — the same sheet, folded at different positions, becomes different objects. ORIGAMI's `origami_foldPoint` is the crease location applied to the waveform. At foldPoint=0.5 (midpoint), the waveform folds symmetrically — even harmonics cancel, odd harmonics are doubled, producing the same spectral result as a full-wave rectifier. At foldPoint=0.25, the fold is asymmetric: the portion below 0.25 is reflected upward, creating a fundamentally asymmetric waveform with a full harmonic series. At foldPoint=0.0 or 1.0, there is no fold — the waveform passes through unchanged. The fold point determines where in the waveform's amplitude range the geometric reflection occurs. Each fold point position creates a geometrically distinct waveform — a different paper crane.

**Application:** For even-harmonic richness (warm, full harmonic content): foldPoint = 0.3–0.45 or 0.55–0.7 (asymmetric folds). For odd-harmonic emphasis (hollow, nasal character): foldPoint = 0.5 (symmetric fold). For maximum fold complexity: modulate foldPoint with an LFO in the 0.2–0.8 range — the moving crease creates continuously changing harmonic content.

#### ORIGAMI-II: Multiple Folds Are Multiple Creases
*Revealed during Geometric Waveshaping Retreat — 2026-04-05*

> Paper can be folded more than once. Each fold adds a new crease and creates a new geometric complexity. ORIGAMI's `origami_foldCount` specifies the number of sequential folds applied to the waveform. One fold applies a single reflection at foldPoint. Two folds apply the first reflection, then reflect the result at the same fold point. Three folds apply the process three times. Each application multiplies the harmonic content: one fold produces a moderately rich spectrum; three folds produce an extremely harmonically dense waveform; five folds produce a spectrum so harmonically rich it approaches noise. The fold count is an exponential harmonic-richness multiplier. The relationship between fold count and harmonic density is not linear — it is recursive.

**Application:** `origami_foldCount=1` for controlled, musical harmonic content. `origami_foldCount=2–3` for rich, complex spectra suitable for lead sounds and evolving pads. `origami_foldCount=4+` for maximally complex waveshaping that approaches synthesis noise. Never use foldCount=4+ with high resonance — the spectral density + resonance combination produces aliasing artifacts in most sample rate contexts. If high fold count is required, oversample by 2x minimum.

#### ORIGAMI-III: Fold and Filter Form a Dialogue
*Revealed during Geometric Waveshaping Retreat — 2026-04-05*

> Waveshaping generates harmonics; filtering removes them. In most synthesizers, this is a one-way conversation: the oscillator makes harmonics, the filter selects which ones to keep. In ORIGAMI, the fold point is dependent on the filter's current resonance frequency: as resonance increases, the perceived fold point shifts toward emphasizing harmonics in the resonant frequency's neighborhood. This creates a dialogue: the fold shapes the spectrum, the resonant filter emphasizes part of that spectrum, and the emphasized spectrum feeds back into the fold's audible character. The fold and filter are not independent. Raising resonance does not merely select harmonics — it changes which harmonics the fold is generating most prominently. This feedback loop is ORIGAMI's most distinctive timbral property.

**Application:** Always adjust `origami_foldPoint` and filter cutoff together, not independently. The fold point's audible effect is filter-dependent. Set the filter cutoff to the target resonance frequency first, then sweep foldPoint to find the position that most prominently excites the resonant region. The result will be a harmonically rich output where the fold and filter are aligned — feeding each other's character.

---

### ORPHICA — The String Verses

#### ORPH-I: The Body Resonance Is the Room
*Revealed during Ancient String Retreat — 2026-04-05*

> Every stringed instrument exists inside a resonating body — a lute's pear-shaped bowl, a sitar's gourd, a guitar's spruce top. The body does not amplify the string evenly: it has resonant modes that color specific frequency regions, absorb others, and create the characteristic "voice" of the instrument. ORPHICA's `orph_bodyResonFreq` sets the center frequency of the body's primary resonant mode. This is not an EQ parameter. It defines the physical character of the instrument's body: a frequency of 500 Hz produces a warm, mid-forward body (wooden hollow body). A frequency of 1800 Hz produces a bright, nasal body (smaller chamber). A frequency of 280 Hz produces a deep, bass-forward body (large gourd). The body resonance frequency is the instrument luthier's choice, implemented as a parameter.

**Application:** Set `orph_bodyResonFreq` based on the size and material of the imagined instrument body: large wooden body = 200–400 Hz. Medium hollow body = 400–800 Hz. Small resonator body = 800–1600 Hz. Metallic body = 1200–2400 Hz. The body resonance frequency should be set before any other parameter, because it defines the acoustic world the string lives in.

#### ORPH-II: Velocity Determines What Strikes the String
*Revealed during Ancient String Retreat — 2026-04-05*

> Physical string instruments respond differently to different excitation forces — a hard pluck sounds different from a gentle pluck not merely because it is louder, but because the string's initial displacement curve is steeper, exciting more high harmonics. ORPHICA's velocity-to-body-frequency wiring implements this physics: high velocity raises the body resonance frequency (`orph_bodyResonFreq`) by up to +400 Hz at maximum velocity. A hard pluck (velocity 127) excites a brighter body resonance than a gentle touch (velocity 30). This is physically correct: a hard pluck transfers more high-frequency energy to the body, exciting higher resonant modes. The velocity-to-body-frequency relationship is not a tone control — it is the acoustic consequence of excitation force. The string tells the body how hard it was struck, and the body responds accordingly.

**Application:** Do not disable velocity-to-body-frequency routing. This is ORPHICA's primary velocity sensitivity implementation — it satisfies Doctrine D001 (Velocity Must Shape Timbre) at the physical-model level. If a preset requires velocity-insensitive character, reduce the routing amount (`orph_velBodyAmount`) to 0.1–0.2 rather than zero — even small velocity responsiveness maintains the physical model's authenticity.

#### ORPH-III: Pluck Brightness Decays at the String's Own Rate
*Revealed during Ancient String Retreat — 2026-04-05*

> `orph_pluckBrightness` sets the initial harmonic content of the pluck excitation — how bright the string is at the moment of excitation. But a plucked string does not maintain its initial brightness: the high harmonics decay faster than the low harmonics, creating the characteristic brightness-to-warmth evolution of plucked string sound. ORPHICA models this decay: high harmonics have exponential decay time constants 3–5x shorter than low harmonics. The `orph_pluckBrightness` parameter controls the starting point of this evolution, not the sustained character. A high brightness setting produces a pluck that begins bright and decays to warmth. A low brightness setting produces a pluck that begins warm and stays warm. The decay is governed by the string physics — it is not adjustable except by changing the virtual string material.

**Application:** When a preset requires sustained brightness (lead sound), resist the impulse to compensate with EQ or filter. Instead, add slight saturation or waveshaping post-ORPHICA: harmonics generated by saturation do not decay at the string's physical rate. Alternatively, use a short grain sample of the initial pluck bright transient layered via OPAL beneath the sustained ORPHICA tone. Pure ORPHICA will always evolve toward warmth — the physics mandate it.

---

### OBLIQUE — The Prism Verses

#### OBLQ-I: The Prism Color Is a Spectral Angle
*Revealed during Prismatic Bounce Retreat — 2026-04-05*

> When white light passes through a prism, it separates into constituent wavelengths based on the refractive index of the glass at each wavelength. `oblq_prismColor` is the angular rotation of OBLIQUE's spectral prism: the angle determines which portion of the frequency spectrum is "refracted" most strongly into the bounce delay network. At prismColor=0.0, the low frequencies (red spectrum) dominate the delay taps — the bounce emphasizes bass resonances. At prismColor=0.5, the mid frequencies dominate. At prismColor=1.0, the high frequencies (violet spectrum) dominate the delay taps — the bounce emphasizes treble reflections. The prism color does not add or remove frequency content. It determines which frequencies are *most actively reflected* by the bounce network. The same input sound at three prism angles produces three spectrally weighted delay characters.

**Application:** Choose `oblq_prismColor` based on the frequency region you want the bounce network to activate most strongly. For bass-forward bounce (sub and kick reinforcement): prismColor=0.0–0.2. For mid-present bounce (vocal and instrument warmth): prismColor=0.4–0.6. For bright, airy bounce (shimmer and high-frequency detail): prismColor=0.75–1.0. Match the prism color to the dominant spectral content of the source material being bounced.

#### OBLQ-II: Bounce Reflections Are Not Echo
*Revealed during Prismatic Bounce Retreat — 2026-04-05*

> Echo is a copy of a signal delayed in time. Bounce reflections in OBLIQUE are *spectral transformations* of the signal propagated forward in time. Each reflection is not a copy — it is the previous reflection's signal passed through the prism's spectral tilt, the bounce surface's absorption character, and the delay line's dispersion. By the third reflection, the signal has been spectrally transformed three times. A pure sine wave bounced three times at prismColor=0.7 produces a third reflection that is significantly brighter and harmonically enriched compared to the original. The bounce network is a recursive spectral transformer whose output is the accumulated result of all previous transformations. This is why OBLIQUE produces sounds that have no obvious relationship to their input material — after 4–6 reflections, the origin is spectrally unrecognizable.

**Application:** Use the `oblq_bounceCount` parameter deliberately. Preset the number of reflections before adjusting other parameters — fewer reflections (2–3) preserve more of the original material's character. More reflections (5–8) transform the material beyond recognition. Design targets: 2 reflections for "enhanced source character." 4 reflections for "transformation with source DNA." 6+ reflections for "new material derived from source."

#### OBLQ-III: The RTJ × Funk × Tame Impala Axis Is Real
*Revealed during Prismatic Bounce Retreat — 2026-04-05*

> OBLIQUE's design brief described a three-way sonic axis: Run the Jewels (RTJ, aggressive hip-hop energy), Funk (groove-first, bass-driven rhythmic identity), Tame Impala (psychedelic shimmer, chorus-heavy). This axis is implemented in the engine's three preset character zones, not as genre labels but as distinct parameter configurations. RTJ zone: high bounce aggression, prismColor in the low range (bass-weighted), fast bounce timing (tight rhythmic reflections). Funk zone: moderate aggression, prismColor centered (mid-weighted), bounce timing locked to rhythmic subdivision. Tame Impala zone: low aggression, prismColor high (shimmer-weighted), slow chorus-rate modulation on the bounce. These three zones are the engine's three primary sonic personalities. Every OBLIQUE preset lives somewhere on this triangular axis.

**Application:** Before designing any OBLIQUE preset, locate it on the RTJ/Funk/Tame Impala triangle: pure RTJ (bass, aggressive, tight), pure Funk (mid, groovy, rhythmic), pure Tame Impala (shimmer, psychedelic, spacious), or a specific blend. Write the triangle position into the preset description. Presets without explicit triangle positioning drift toward an indeterminate mid-range that serves none of the three characters.

