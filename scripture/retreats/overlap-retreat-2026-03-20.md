# OVERLAP Retreat Chapter
*Guru Bin — 2026-03-20*

---

## Engine Identity

- **Gallery code:** OVERLAP | **Accent:** Bioluminescent Cyan-Green `#00FFB4`
- **Parameter prefix:** `olap_`
- **Creature mythology:** The Lion's Mane Jellyfish — signal tangling through Feedback Delay Networks
- **feliX-Oscar polarity:** Deep Oscar — sustained, spatial, warm, emergent. The jellyfish does not strike; it drifts and envelops.
- **Synthesis type:** 6-voice knot-topology FDN synthesis with Kuramoto entrainment and bioluminescent shimmer
- **Polyphony:** 6 voices (each voice IS one channel of the FDN — polyphony restructures the resonant lattice)
- **Macros:** M1 KNOT, M2 PULSE, M3 ENTRAIN, M4 BLOOM

---

## Pre-Retreat State

**Seance score: 8.6/10.** Two Blessings: B017 (Knot-Topology Resonance, reaffirmed) and the KNOT macro recognized as fleet-best single-knob design. 41 canonical `olap_` parameters. All six Doctrines satisfied. Stereo bioluminescence resolved. LFO destinations expanded to include Bioluminescence and Entrainment. No dead parameters.

This engine earned its retreat. The ghosts spoke in rare agreement: OVERLAP occupies territory no other synthesizer occupies.

---

## Phase R1: Opening Meditation

Close your eyes. Breathe.

The jellyfish does not decide to move. It contracts and the water decides. The bell pulses and the current carries. There is no separation between the creature and its medium — the water column is the instrument and the instrument is the water column.

This is what OVERLAP understands that no other engine understands: the voice is not separate from the space. In every other synthesizer, you create a tone and then you put it in a room. In OVERLAP, the tone IS the room. Each voice enters the Feedback Delay Network and becomes part of the architecture of resonance. Playing a chord does not stack voices — it restructures the geometry of the space itself. C, E, and G are not three notes. They are three changes to the shape of the room you are standing in.

Sit with that. The jellyfish breathes. The room changes shape.

---

## Phase R2: The Signal Path Journey

### I. The Exciter — Sine-Pulse Hybrid

The journey begins with almost nothing. A sine wave, shaped by a pulse envelope controlled by `olap_pulseRate`. This is deliberate poverty. The exciter is not the voice — it is the breath that enters the bell. The jellyfish's contraction is simple. What happens after the contraction is not.

The Voice module combines sine output with a velocity-triggered pulse envelope. At low pulse rates, each voice produces slow, whale-song pulsations. At high rates (up to 8 Hz), the voices create a granular, clicking texture — bioluminescent flickers before the current catches them.

This exciter's simplicity is a blessing (the ghosts debated this). The topology IS the timbre. A complex exciter would obscure the knot structure. The sine-pulse hybrid lets the FDN speak without interference.

### II. The Feedback Delay Network — Where Topology Becomes Sound

Six delay lines. Six voices feeding six channels. A 6x6 routing matrix derived from mathematical knot theory.

Four knot types reconfigure how delay channels feed back into each other:

- **Unknot** — Identity matrix blended with tangle depth. Voices run in parallel, clean and independent. The jellyfish floats still.
- **Trefoil** — 3-fold symmetric circulant matrix. Each delay channel feeds into two others with equal weight. Energy circulates in a triangular pattern. The jellyfish begins to spiral.
- **Figure-Eight** — Alternating-sign circulant. Channels feed each other with opposing polarity. Constructive and destructive interference create anxious flutter. The tentacles twist against each other.
- **Torus T(p,q)** — Winding number ratios derived from torus knot parameters. The most harmonically locked topology. With `olap_torusP` and `olap_torusQ`, the performer selects the winding pattern — T(3,2) gives the trefoil's cousin, T(5,7) creates irrational harmonic ratios that never resolve. The jellyfish enters a current it will not leave for hours.

The `olap_tangleDepth` parameter interpolates between identity (no coupling) and full topological entanglement. At 0.0, six parallel delay lines — a reverb. At 1.0, a resonant lattice shaped by knot mathematics. The space between is where OVERLAP lives.

### III. Kuramoto Entrainment — The Collective Pulse

Six voices, each with an independent phase. The Kuramoto model nudges them toward synchrony: `dtheta_i = K/N * sum(sin(theta_j - theta_i))`. At `olap_entrain` = 0, voices pulse independently — six separate organisms. At entrain = 1.0, they converge into a single collective contraction — one organism.

The convergence is not instant. It is emergent. Voices approach synchrony, then a new note arrives and disrupts the lattice. The system re-converges. This is not an LFO. It is a mathematical process that surprises itself.

At moderate entrainment (0.3-0.6), voices continuously approach and retreat from phase alignment, creating rhythmic flutter that no parameter automates. This is the engine's most distinctive performance gesture — a breathing that arises from mathematics, not from modulation.

### IV. Bioluminescence — The Shimmer Layer

Seven comb taps at near-prime ratios (1.0, 1.31, 1.71, 2.09, 2.61, 3.19, 3.89), each modulated by an independent sine oscillator between 0.3 and 0.72 Hz. Odd taps pan left, even taps pan right. The result is a spatial shimmer that drifts through the stereo field like light scattered through deep water.

The `olap_bioluminescence` parameter controls shimmer depth. The `olap_brightness` parameter couples to it — brighter timbres shimmer more, darker timbres glow faintly. This is not an effect. It is the creature's light.

The seven modulators are autonomous. They do not respond to any parameter. They drift at their fixed rates, creating a shimmer pattern that never repeats within any human listening session. The jellyfish glows whether or not you are watching.

### V. The SVF Filter — Post-Topology Shaping

A Zavalishin TPT state-variable filter, placed after the FDN. This placement is critical — the filter shapes the resonant topology output, not the excitation input. The filter envelope with velocity scaling satisfies D001: velocity shapes timbre, not just amplitude. Negative envelope amounts sweep the filter downward — a closing of the bell after the initial contraction.

### VI. Post FX — Chorus and Diffusion

BBD-style chorus with 90-degree L/R phase offset creates stereo width at the close layer. Three-stage all-pass diffusion with prime delay lengths (113, 257, 397 samples) provides early reflection depth. Combined with the bioluminescence mid-layer, OVERLAP has a three-layer spatial architecture: close (chorus), mid (bioluminescence), far (diffusion). This is orchestral spatial thinking applied to a single engine.

### VII. Ocean Current — The Geological Drift

`olap_current` applies a slow sine-driven pitch bias to all voices. At `olap_currentRate` = 0.005 Hz, one full drift cycle takes over 3 minutes. The voices collectively shift by up to 2 semitones — a tidal force that moves the entire harmonic lattice. At minute 1, the patch is in one tonal neighborhood. At minute 5, it has drifted somewhere else. The performer did nothing. The current carried them.

---

## Phase R3: Parameter Meditations

### The Four Macros — Four Questions

| Macro | Question | What It Controls |
|-------|----------|-----------------|
| M1 KNOT | *What am I?* | Sweeps through all 4 topologies with scaled tangle depth across 7 breakpoints |
| M2 PULSE | *How do I sound?* | Pulse rate and voice spread jointly — excitation character |
| M3 ENTRAIN | *How do my voices relate?* | Entrainment coupling and feedback — collective vs. individual |
| M4 BLOOM | *What world am I in?* | Bioluminescence and filter opening — atmospheric presence |

The KNOT macro deserves meditation. It divides the 0-1 range into 7 zones: Unknot rising, Unknot plateau, Trefoil rising, Trefoil plateau, Figure-Eight rising, Figure-Eight plateau, Torus rising. A performer who has never heard the word "topology" turns this knob and discovers four distinct sonic territories. The plateaus between transitions give each topology room to breathe before the next begins.

This is the best single-knob macro design in the fleet. Every engine with multi-mode parameter sweeps should study this pattern.

### The Expression Routes

- **Mod wheel** targets Tangle, Entrainment, Bioluminescence, or Filter with adjustable depth
- **Aftertouch** targets Tangle, Entrainment, Bioluminescence, or Pulse Rate with adjustable depth
- **Velocity** scales amplitude and filter envelope simultaneously — harder notes are brighter

The mod wheel controlling Entrainment is the most powerful expression mapping in the fleet. The performer's left hand controls the boundary between six individual minds and one collective consciousness. At CC1=0, six organisms. At CC1=127, one organism. No other instrument gives a performer this axis.

### The LFO Destinations — Eight Targets

Both LFOs can target: Tangle, Dampening, Pulse Rate, Delay Base, Filter Cutoff, Spread, Bioluminescence, Entrainment. The inclusion of Bioluminescence and Entrainment (added during the seance cycle) unlocks autonomous living patches — LFO1 slowly modulating entrainment while LFO2 breathes bioluminescence. The engine performs itself while the player adds notes on top.

---

## Phase R4: Coupling Wisdom

OVERLAP does not naively accept coupling signals. It interprets them through its own paradigm:

| Coupling Type | OVERLAP's Interpretation |
|--------------|--------------------------|
| AudioToFM | Modulates FDN delay base — FM on delay length IS the FDN equivalent of FM |
| AudioToRing | Ring modulates output amplitude |
| AmpToFilter | Raises filter cutoff proportionally |
| EnvToMorph | Pushes tangle depth — external envelope morphs topology |
| LFOToPitch | Tangle depth perturbation — pitch doesn't mean the same thing to an FDN |
| PitchToPitch | Same remapping as LFOToPitch |
| FilterToFilter | Multiplicative cutoff shift |

The EnvToMorph mapping is the deepest. When another engine's envelope modulates OVERLAP, it morphs the topology itself — the knot structure breathes with the external engine's note events. Couple OVERLAP with ONSET (drums), route EnvToMorph, and the drum hits physically reshape the resonant lattice. The jellyfish dances to a drummer it cannot see.

---

## Phase R5: The Ten Awakenings

Ten preset concepts for the Awakening tier — hero presets that announce what OVERLAP is.

---

### 1. Bioluminescent Drift

**Mood:** Atmosphere | **Discovery:** The shimmer IS the instrument

- Unknot topology, tangle depth 0.1 (barely entangled)
- Bioluminescence 0.7, brightness 0.8 — the shimmer dominates
- Ocean Current 0.6 at rate 0.008 Hz — 2-minute drift cycle
- LFO1 -> Bioluminescence at 0.03 Hz, depth 0.4
- Release 6.0s, filter cutoff 12000 Hz
- **Character:** A drifting cloud of shimmering light. The topology is minimal — this preset foregrounds the bioluminescence layer as the primary timbral identity. Notes hang and glow. The current carries them.

---

### 2. Trefoil Cathedral

**Mood:** Aether | **Discovery:** Topology as sacred geometry

- Trefoil knot, tangle depth 0.7
- Entrain 0.6 — voices lock into collective pulse
- Diffusion 0.7, chorus mix 0.35, chorus rate 0.04
- Filter cutoff 6000 Hz, resonance 0.2
- Attack 0.3s, decay 2.0s, sustain 0.8, release 4.0s
- **Character:** Play a sustained chord. The three-fold symmetry creates harmonic reinforcement in triadic patterns. Voices converge through entrainment into a pulsing, breathing cathedral of overtones. This is where OVERLAP sounds most like a pipe organ designed by mathematicians.

---

### 3. Torus Lock

**Mood:** Foundation | **Discovery:** Harmonic locking through winding numbers

- Torus T(5,3), tangle depth 0.85
- Entrain 0.8 — strong collective convergence
- Pulse rate 1.2 Hz, spread 0.9
- Bioluminescence 0.3, brightness 0.5
- Filter cutoff 4000 Hz, filter env amount 0.5, filter env decay 0.8s
- **Character:** The most harmonically locked state OVERLAP can achieve. The T(5,3) winding creates a lattice where delay ratios form near-integer relationships. Voices entrain tightly. The result is a thick, resonant bass tone with overtone clarity — the topology acting as a harmonic filter. Play single bass notes. Let the lattice ring.

---

### 4. Figure-Eight Anxiety

**Mood:** Flux | **Discovery:** Alternating-sign coupling creates tension

- Figure-Eight knot, tangle depth 0.9
- Entrain 0.4 — voices fight between convergence and divergence
- Pulse rate 3.5 Hz, spread 0.6
- Filter cutoff 10000 Hz, resonance 0.35
- LFO1 -> Tangle at 0.15 Hz, depth 0.3
- LFO2 -> Entrainment at 0.08 Hz, depth 0.25
- **Character:** The alternating-sign matrix creates constructive and destructive interference simultaneously. Voices push and pull. The moderate entrainment tries to lock them but the topology fights back. The result is an anxious, fluttering texture — a jellyfish caught in a riptide. Play staccato chords. Feel the tension.

---

### 5. Kuramoto Breath

**Mood:** Atmosphere | **Discovery:** Entrainment as emergent rhythm

- Unknot topology, tangle depth 0.3
- Entrain 0.7 — the primary axis of this preset
- Pulse rate 0.4 Hz — slow, deep pulsation
- Bioluminescence 0.5, brightness 0.6
- LFO1 -> Entrainment at 0.02 Hz, depth 0.4
- Release 5.0s, filter cutoff 6000 Hz
- **Character:** The LFO slowly sweeps entrainment. At the peaks, all six voices converge into one collective breath. At the troughs, they drift apart into independent organisms. The pulse is slow enough to feel as a tidal rhythm — contraction, expansion, contraction. Hold a chord and listen to the breathing. This is not modulation. It is emergence.

---

### 6. Deep Current

**Mood:** Submerged | **Discovery:** Ocean Current as compositional force

- Trefoil knot, tangle depth 0.5
- Ocean Current 0.9 at rate 0.005 Hz — 200-second drift cycle, nearly 2 semitones of pitch wander
- Entrain 0.3, feedback 0.85
- Delay base 40ms — long resonant tails
- Filter cutoff 3000 Hz, dampening 0.7
- Release 8.0s
- **Character:** Play a single note and wait. For three minutes, the pitch drifts through a full cycle — not a vibrato, not an LFO, but a geological force. The trefoil topology colors the drift with its circulant overtone structure. The long feedback creates trails that overlap with the shifting pitch, building a layered sediment of harmonic history. This is OVERLAP at its most Schulze — a sound sculpture that evolves for an hour.

---

### 7. Pulse Swarm

**Mood:** Flux | **Discovery:** High pulse rate as texture

- Unknot topology, tangle depth 0.5
- Pulse rate 6.0 Hz — fast granular pulsation
- Entrain 0.2 — mostly independent
- Spread 1.0 — full stereo field
- Bioluminescence 0.15
- Filter cutoff 14000 Hz, filter env amount 0.6, filter env decay 0.2s
- Attack 0.01s, decay 0.3s, sustain 0.5, release 1.0s
- **Character:** Six voices pulsing independently at 6 Hz, spread across the stereo field. Each note adds another flickering organism to the swarm. The fast pulse rate creates a granular, bioluminescent texture — a cloud of fireflies rather than a single jellyfish. Rapid note runs create cascading swarms. This is OVERLAP at its most energetic.

---

### 8. Topological Crossfade

**Mood:** Prism | **Discovery:** The KNOT macro as performance axis

- KNOT macro at 0.5 (Trefoil plateau)
- All other macros at 0.5 — a centered starting point
- LFO1 -> Tangle at 0.05 Hz, depth 0.5
- LFO2 -> Pulse Rate at 0.03 Hz, depth 0.3
- Bioluminescence 0.4, diffusion 0.5
- **Character:** The performer's preset. KNOT macro sweeps from Unknot through Trefoil through Figure-Eight to Torus in one gesture. Start at the left (clean, parallel voices), sweep through the middle (spiraling, fluttering), arrive at the right (harmonically locked lattice). The LFOs keep the texture evolving between sweeps. This preset teaches the engine: one knob, four worlds.

---

### 9. Entangled Choir

**Mood:** Entangled | **Discovery:** Polyphony restructures the lattice

- Trefoil knot, tangle depth 0.65
- Entrain 0.55
- Voice mode: Poly (6 voices)
- Spread 0.8
- Bioluminescence 0.45, brightness 0.65
- Filter cutoff 7000 Hz, resonance 0.15
- Attack 0.15s, decay 1.5s, sustain 0.75, release 3.0s
- Chorus mix 0.25, diffusion 0.4
- **Character:** Play a slow chord progression. Each new note restructures the FDN lattice — the room changes shape with every chord. Voices partially entrain, creating a choir that breathes in near-unison. The shimmer layer adds spatial depth. This is OVERLAP's answer to a string section — six voices sharing one resonant space, each voice changing the space for all the others.

---

### 10. Midnight Torus

**Mood:** Aether | **Discovery:** Torus T(7,5) as generative instrument

- Torus T(7,5), tangle depth 0.75
- Entrain 0.5
- Pulse rate 0.25 Hz — very slow contraction
- Ocean Current 0.5 at rate 0.01 Hz
- Bioluminescence 0.6, brightness 0.4
- LFO1 -> Entrainment at 0.01 Hz, depth 0.35
- LFO2 -> Bioluminescence at 0.018 Hz, depth 0.3 (irrational ratio to LFO1)
- Filter cutoff 5000 Hz, dampening 0.6, feedback 0.88
- Release 7.0s
- **Character:** Five independent temporal evolution mechanisms — Ocean Current, Kuramoto entrainment, LFO1, LFO2, and the seven bioluminescence modulators — all at different rates with irrational ratios. The probability of all five returning to their starting state simultaneously is effectively zero. Play one chord and walk away. The patch will be different at minute 1, minute 10, minute 30. This is OVERLAP as generative installation — a living system that breathes on its own clock. The jellyfish does not need you. It drifts regardless.

---

## Phase R6: New Scripture Verses

Four verses to be inscribed in the Book of OVERLAP.

**OLAP-I: The Room That Plays Itself** — In every other synthesizer, the voice is separate from the space. In OVERLAP, the voice IS the space. Each note enters the Feedback Delay Network and becomes a channel of resonance. Playing a chord restructures the geometry of the room. C, E, and G are not three notes — they are three changes to the shape of the place you are standing in. Design patches that honor this: the note is the beginning, not the destination.

**OLAP-II: The Jellyfish's Contraction** — The sine-pulse exciter is deliberately simple. It is the breath that enters the bell, not the sound that leaves it. The topology IS the timbre. A complex exciter would obscure the knot structure. Trust the FDN to do the work. Shape the excitation only enough to give it character — pulse rate for rhythm, brightness for shimmer coupling. Then let the topology speak.

**OLAP-III: Entrainment Is Not Modulation** — At moderate coupling (0.3-0.6), the Kuramoto model creates rhythmic gestures that no LFO can produce. Voices approach synchrony, destabilize when new notes arrive, and re-converge along paths that depend on the current phase distribution. This is emergence, not automation. Never set entrain to 0.0 or 1.0 and walk away. The space between is where the engine breathes.

**OLAP-IV: Five Clocks, One Organism** — Ocean Current (0.005 Hz), LFO1 (0.01 Hz), LFO2 (0.018 Hz), Entrainment (emergent), Bioluminescence modulators (0.3-0.72 Hz). Five temporal layers spanning a 144:1 timescale ratio. When these rates are irrational multiples of each other, the patch never repeats. A sustained OVERLAP chord is genuinely different at minute 1 vs. minute 30. This is the engine's deepest gift: time as a compositional axis that requires no performer.

---

## CPU Notes

- 6-voice FDN: moderate cost per voice (delay line + matrix multiply + one-pole dampening)
- Bioluminescence: 7 comb taps + 7 sine modulators — light but constant
- PostFX: chorus (BBD delay + modulator) + 3 allpass diffusers — constant overhead
- SVF filter: 2-channel TPT — negligible
- Kuramoto entrainment: 6-voice phase comparison per sample — light
- Most costly configuration: Torus T(7,5) at full tangle, 6 voices, high bioluminescence, long feedback

---

## Unexplored After Retreat

- **Alternative exciters**: Noise burst or FM pair injection alongside the sine-pulse hybrid — the debate between Vangelis and Schulze remains unresolved
- **Sub-bass drone territory**: Extending delay base beyond 50ms for fundamentals below 20 Hz — Schulze's frontier
- **Asymmetric knot coefficients**: Deriving matrix weights from braid group theory rather than uniform `1/sqrt(3)` — Buchla's request
- **Fractional delay interpolation**: Linear interpolation in the FDN for smoother delay sweeps — Pearlman's build quality note
- **Jones polynomial weighting**: Using knot invariants to generate unique per-topology matrix weights — the ghosts' future vision
- **Shimmer rate control**: A parameter to scale the bioluminescence modulator rates — Tomita's wish for temporal control of the shimmer

---

## Guru Bin's Benediction

*"OVERLAP arrived already blessed. The ghosts gave it 8.6 and a reaffirmed Blessing — B017, Knot-Topology Resonance, the only synthesis architecture in the fleet with no prior art in any synthesizer, commercial or academic.*

*What I found in the silence between the parameters is an engine that understands something most instruments never learn: the voice and the space are one thing. When you play a chord on OVERLAP, you are not stacking sounds into a room. You are reshaping the room with each note. The chord IS the architecture. The topology IS the timbre.*

*The KNOT macro is the finest single-knob design in the fleet. Seven breakpoints. Four topologies. One gesture. A player who has never heard the word 'trefoil' turns this knob and discovers that sound has geometry. That the shape of resonance can be unknotted, spiraling, alternating, or locked into the winding patterns of a torus. That is teaching through touch.*

*The Kuramoto entrainment is the revelation. At entrain 0.5, six voices approach synchrony and retreat, approach and retreat, creating rhythmic gestures that no modulation source generates. This is not an LFO. It is six mathematical minds negotiating consensus. The performer's mod wheel controls the boundary between individual agency and collective consciousness. No other instrument gives a human hand this axis.*

*The bioluminescence is the beauty. Seven autonomous shimmer modulators, alternating left and right in the stereo field, drifting at near-prime rates that never align. The jellyfish does not choose to glow. It glows because it is alive. The shimmer does not respond to your playing. It responds to its own internal rhythms. And yet — because brightness couples shimmer to timbral energy — when you play brighter, the glow intensifies. The creature responds to your touch without obeying it.*

*Five clocks. 144:1 timescale ratio. Zero probability of repetition within a human listening session. Play one chord. Wait an hour. The sound at minute 60 has never existed before and will never exist again.*

*The jellyfish does not need you. It drifts, it pulses, it glows regardless. But when you play — when you add your notes to its lattice, your chords to its topology, your gestures to its entrainment — the room changes shape around both of you.*

*That is OVERLAP. Not a synthesizer you play. A resonant space you inhabit.*

*The bell contracts. The current carries. The light glows in the deep."*
