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

*(To be filled as CPU stewardship truths are revealed)*

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
> OCEANIC has two envelopes: the Amp ADSR controls loudness; the Swarm ADSR controls the strength of the boid rules — the collective intelligence of the school. They are independent. At `swarmSustain=0`, the boid forces extinguish completely during sustain: the school loses its collective mind while the note holds at full amplitude. At `swarmAttack=2.5`, the school takes 2.5 seconds to cohere after note-on — the attack is scattered, raw, then resolves. No other engine in the fleet separates the *intelligence* of the sound from the *loudness* of the sound.

**Application:** Design OCEANIC presets around both envelopes, not just the Amp ADSR. The Swarm ADSR is the second compositional dimension.

#### OCE-II: The Boid Rules Are the Filter
*2026-03-15*
> OCEANIC has no filter. The spectral content of the swarm is entirely determined by the boid configuration. High `oceanic_separation` + low `oceanic_cohesion` = particles spread across the frequency spectrum = bright, complex, textural. High `oceanic_cohesion` = particles cluster near the MIDI attractor = narrow, tone-like, pitched. Low `oceanic_tether` = the MIDI note barely influences particle behavior = near-unpitched drift. The boid rules ARE the filter. This is the engine's fundamental design truth.

**Application:** Treat separation/cohesion/tether as a three-way spectral EQ: spread vs. cluster vs. drift. No additional filter is needed.

#### OCE-III: The Chromatophore Contract
*2026-03-15*
> Aftertouch (channel pressure) boosts particle separation ×0.25 — pressing harder makes the school scatter, exactly as a cephalopod accelerates chromatophore cycles under stress. Mod wheel (CC1) boosts cohesion ×0.4 — pressing the wheel tightens the school. The two gestures are physical opposites: pressure scatters, wheel contracts. At rest the school breathes; aftertouch panics it; mod wheel calms it. This is the B013 Blessed mechanism. Every OCEANIC preset should make these two gestures audible.

**Application:** Design OCEANIC presets at a separation/cohesion midpoint (0.35–0.55) so both aftertouch and mod wheel produce audible change. Never leave either gesture without a meaningful effect.

#### OCE-IV: Murmuration Is an Event
*2026-03-15*
> `RhythmToBlend` coupling triggers a cascade reorganization: a perturbation wave propagates from particle 0 through all 128 particles with 0.97× attenuation per particle. The first particles receive maximum disruption; the last particles receive 0.97^128 ≈ 2% of the original force. The wave passes through the school in sequence. Send ONSET's drum output via RhythmToBlend and every hit sends a reorganization cascade through the swarm. This is not modulation. It is a flock reorganizing in response to a percussion event. It sounds like nothing else in the fleet.

**Application:** When designing OCEANIC Entangled presets, always include Onset→Oceanic via RhythmToBlend. Set coupling amount 0.7–1.0. The murmuration should be audible as a cascade sweep, not a static modulation.
