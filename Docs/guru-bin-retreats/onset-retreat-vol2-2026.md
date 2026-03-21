# ONSET Retreat — Vol 2 Transcendental
*Guru Bin — 2026-03-21*

---

## Engine Identity

- **Gallery code:** ONSET | **Accent:** Electric Blue `#0066FF`
- **Parameter prefix:** `perc_` (not `onset_` — see Canon V-2)
- **Creature mythology:** XOnset lives at the water's surface — the meniscus between air and ocean, where feliX's electric energy erupts as transient. Every drum hit is a stone skipping across the surface of water: a splash, a ripple, a dissipating ring. The engine is pure feliX — kinetic, surface-level, percussive, second-generation. It does not live in the deep column like OUROBOROS or OCEANDEEP. It disturbs the surface and gives OUROBOROS something to react to.
- **Synthesis type:** 8-voice fixed drum synthesis. Layer X (Circuit): BridgedT oscillator (808 kick/tom), NoiseBurst (snare/clap), Metallic 6-oscillator network (hat/cymbal). Layer O (Algorithm): FM, Modal resonator (Bessel zeros), Karplus-Strong plucked string, Phase Distortion. Continuous Blend crossfade using equal-power (cos/sin) curves. Cross-Voice Coupling matrix (XVC).
- **Voice assignment:** V1 Kick (MIDI 36), V2 Snare (38), V3 HH-C (42), V4 HH-O (46), V5 Clap (39), V6 Tom (45), V7 Perc A (37), V8 Perc B (44)
- **Polyphony:** 8 fixed voices (not polyphonic — each is permanently assigned)
- **feliX/Oscar polarity:** 90% feliX — pure surface energy, transient-first, rhythmic
- **Seance score:** Not reported separately (part of fleet pre-seance cohort)
- **Macros:** M1 MACHINE (bias all blends toward Circuit ↔ Algorithm), M2 PUNCH (snap+body aggression + aftertouch), M3 SPACE (reverb mix + delay feedback), M4 MUTATE (per-hit random drift in blend+character)
- **Expression:** Velocity → filter brightness (D001 direct), Aftertouch → PUNCH macro boost (+0.3), Mod Wheel CC1 → MUTATE macro depth multiplier (×1.0–×2.0)
- **Blessings:** B002 (XVC Cross-Voice Coupling), B006 (Dual-Layer Blend Architecture)

---

## Pre-Retreat State

ONSET arrived in the fleet with 162 factory presets — the deepest drum synthesis library in the Vol 2 cohort. It is the engine most referenced in coupling routes throughout XOmnibus: OUROBOROS's B003 Injection Protocol names ONSET as its primary disruption source; OCEANIC's murmuration cascade responds to ONSET via RhythmToBlend; OWARE's sympathetic resonance network can receive rhythmic events from ONSET's tuned percussion modes. ONSET is the percussion event generator of the fleet — the engine that tells time.

The factory library covers ONSET's range with breadth. Foundation has 80+ kits representing every acoustic archetype. Flux has evolving, unstable kits. Submerged has three deep percussive textures. What it does not fully demonstrate is ONSET in its structural role: the coupling-source engine whose velocity output, spectral content, and rhythmic event density are specifically calibrated to feed OUROBOROS, OCEANIC, ORBWEAVE, and ORGANISM. These coupling-source presets are the most important additions of Vol 2.

The secondary focus is synthesis beyond acoustic reference. ONSET's 8 voices are permanently mapped but their synthesis algorithms are not fixed. Layer O (Algorithm) includes Modal synthesis (Bessel function zeros creating non-harmonic resonances), Karplus-Strong string synthesis, and Phase Distortion. The factory library has barely explored what happens when you push all 8 voices to their most algorithmic configurations simultaneously — a drum machine that plays physics simulations instead of acoustic models.

---

## Phase R1: Opening Meditation

Close your eyes.

You are standing at the edge of a tide pool at low water. The surface is glass — still enough to see the anemone below, still enough to see the cloud reflection above. You understand the surface as a boundary, not as a place. The surface has no depth. It is the moment of contact between two worlds.

Now drop a stone.

The ripple travels outward. OUROBOROS, sitting deep in the water column, feels the pressure wave. OCEANIC, swimming in formation near the thermocline, feels the disturbance in the group's rhythm. OWARE's tuned membranes, resting on the seafloor rocks, begin to ring sympathetically. The stone does not know it caused all of this. It only knows the surface.

ONSET is the stone.

Now pick up the drum machine. The one with the electric blue panel and the eight voice buttons — one per surface event. Kick, snare, hat, clap, tom, perc. Each one is a stone of different mass dropped from a different height. When you dial in the Blend parameter — from Circuit (the analog warmth of a 1980 Roland surface, built from bridged-T oscillators and noise burst circuits) to Algorithm (the physics simulation, the Karplus-Strong resonance, the modal Bessel zeros of a circular drumhead) — you are choosing what kind of stone.

The Cross-Voice Coupling matrix is what makes ONSET a nervous system rather than a drum machine. Kick peak amplitude from the last block modulates the snare's filter brightness for the next one. Snare peak tightens the hat's decay. These are not parameter modulations. They are conversations. The kick tells the snare it was loud. The snare tells the hat to pull in its decay.

This is a rhythm brain.

In Vol 2, the question is not "what drum sound does this make?" The question is "what does this send to the other engines?" Every Transcendental preset has a downstream destination. Design for the injection, not just the sound.

---

## Phase R2: The Signal Path Journey

### I. Layer X — The Circuit Heritage

Layer X is the analog circuit path. Three circuit topologies cover the eight voices:

**BridgedT Oscillator** (V1 Kick, V6 Tom): The Roland TR-808's bridged-T network is a passive notch filter driven into self-oscillation by a trigger impulse. The pitch sweeps from spike frequency (4×–16× base, controlled by snap) down to base frequency with an exponential decay of 5–50ms. At snap=0.3 the pitch sweep takes approximately 38ms. At snap=0.9, the sweep is 7ms — an extremely tight chirp. The sub-oscillator (triangle, 1 octave below, level set by body parameter) adds sub weight. This is the 808 model at its most faithful.

**NoiseBurst Circuit** (V2 Snare, V5 Clap): Bandpass-filtered noise whose frequency, bandwidth, and decay are shaped by the voice parameters. The clap uses a multi-burst mode (isClap=true) generating 3–5 sequential noise bursts before the reverb tail — authentic hand-clap acoustic modeling. Snap controls burst spacing: at snap=1.0, bursts are 5ms apart; at snap=0.3, 9ms apart.

**Metallic Oscillator** (V3 HH-C, V4 HH-O, V8 Perc B): Six square-wave oscillators at non-harmonic frequency ratios. This is the 808 hi-hat network's direct implementation: 6 oscillator frequencies forming an inharmonic cluster. The result is metallic, not tonal. V3 and V4 share the same base circuit but differ in default decay (0.05s vs 0.40s). V4's longer decay produces the open hat character. Hat choke: the XVC hat choke parameter, when enabled, causes V3 (closed) to silence V4 (open) on each hit — standard drum machine choke behavior.

### II. Layer O — The Algorithmic Heritage

Layer O is the digital/physical modeling path. Four algorithms:

**FM Synthesis** (algoMode=0): Carrier + modulator with configurable ratio (carrier:modulator = 1.4 default), FM index (0–8), and feedback (0–0.3). The modulator has an exponential decay envelope independent of the amp envelope. Used by V2 Snare and V3/V4 Hats at default. At FM index > 4, the character becomes harsh and metallic. The feedback amount creates self-FM — useful for electronic snare character.

**Modal Resonator** (algoMode=1): 8 parallel bandpass filters tuned to Bessel function zeros of a circular drumhead (ratios: 1.0, 1.59, 2.14, 2.30, 2.65, 2.92, 3.16, 3.50 × fundamental). Excited by a noise burst. The character parameter controls inharmonicity: 0=ideal membrane (pure Bessel), 1=stretched plate. At character=0.8, the higher modes shift toward metallic bar behavior, creating a xylophone or marimba-like stretched spectrum. Used by V1 Kick and V6 Tom at default.

**Karplus-Strong** (algoMode=2): Noise-seeded delay line with averaging lowpass feedback. The blend parameter (character) controls the probabilistic sign-flip in the feedback path — from clean string (character=1.0) to snare-wire buzz (character=0.0). Decay parameter controls feedback gain (0.9–0.999). At decay=0.99+, the string sustains for many seconds — this is the tuned percussion territory of ONSET. Used by V7 Perc A at default.

**Phase Distortion** (algoMode=3): Casio CZ-101 phase distortion synthesis. Two oscillators with independent phase distortion amounts, creating waveforms that morph between sine (distortion=0) and hard-clipped complex shapes (distortion=1). Used by V5 Clap at default.

### III. The Blend Axis — B006 Dual-Layer Architecture

The Blend parameter for each voice is a continuous equal-power crossfade using `cos(blend × π/2)` for Layer X gain and `sin(blend × π/2)` for Layer O gain. At blend=0.0, only Layer X plays. At blend=0.5, equal power from both. At blend=1.0, only Layer O.

The Blend axis is the most underexplored dimension in the factory library. The defaults are conservative: V1 Kick at 0.2 (mostly circuit), V2 Snare at 0.5 (balanced), V3/V4 Hats at 0.7 (mostly metallic FM). These defaults produce recognizable drum sounds. But the territory between 0.3 and 0.7 — the zone where the circuit and the algorithm fight for dominance — is where the most interesting timbres live. A kick at blend=0.45 has the 808's pitch sweep fighting a modal resonator's Bessel modes. The result is not an 808, not a tom — it is a new impact sound with no acoustic reference.

The MACHINE macro biases all blends simultaneously: at macro=0, everything shifts circuit-ward; at macro=1, everything shifts algorithm-ward. This creates fleet-wide timbral crossfades in a single gesture.

### IV. Cross-Voice Coupling (XVC) — B002 The Rhythm Brain

XVC is the most architecturally novel feature in ONSET. Four cross-voice modulation routes operate on peak amplitudes from the previous audio block:

1. **Kick → Snare filter** (`perc_xvc_kick_to_snare_filter`, default 0.15): kick peak × global XVC level × kickToSnareFilterAmount is added to the snare's tone parameter. Hard kicks make the snare brighter on the very next hit after the kick. This is a 1-block lookahead relationship — the kick doesn't affect the snare simultaneously, it affects the snare one block later.

2. **Snare → Hat decay** (`perc_xvc_snare_to_hat_decay`, default 0.10): snare peak reduces the hat's decay time by `peak × 0.10 × 0.5`. A loud snare tightens the hat. This creates the rhythmic behavior of a real drum kit where hats played hard after a snare hit have pulled-back decays.

3. **Kick → Tom pitch** (`perc_xvc_kick_to_tom_pitch`, default 0.0): kick peak ducks the tom pitch by up to 6 semitones. Disabled at default — activating this creates a sidechain pitch relationship between kick and tom.

4. **Snare → Perc A blend** (`perc_xvc_snare_to_perc_blend`, default 0.0): snare peak shifts V7 Perc A toward the algorithmic layer. Disabled at default — activating this creates evolving percussion textures that respond to the snare's hit strength.

The global XVC amount (`perc_xvc_global_amount`) scales all four routes simultaneously. Hat choke (`perc_xvc_hat_choke`) enables/disables the standard closed-hat-silences-open-hat behavior.

At high XVC global amounts with active kick→snare filter and snare→hat decay, the kit develops an autonomous rhythmic intelligence: loud passages make the kit brighter and tighter; quiet passages let the hat breathe and the snare darken.

### V. Coupling Outputs

ONSET's `applyCouplingInput` accepts four coupling types as a receiver:
- **AmpToFilter**: scales `perc_vN_tone` parameters via `couplingFilterMod`
- **EnvToDecay**: scales `perc_vN_decay` parameters via `couplingDecayMod`
- **RhythmToBlend**: scales all blends via `couplingBlendMod`
- **AmpToChoke**: at amplitude > 0.5, chokes all voices simultaneously

As a sender, ONSET outputs post-FX stereo audio (via `getSampleForCoupling`) that other engines process as AmpToFilter, RhythmToBlend, or AudioToBuffer signals. The kick's peak transient creates the most concentrated voltage spike in the fleet. When OUROBOROS is configured to receive `Onset→Ouroboros via Injection (0.15–0.20)`, each ONSET kick disturbs the strange attractor differently depending on kick velocity — a stronger kick pushes the attractor further from its current basin.

### VI. Parameter Territory Unexplored

After surveying 162 existing presets, the gaps are:

1. **Full-algorithmic kits** (all 8 voices at blend > 0.7): The modal resonator produces physically correct membrane sounds that bear no resemblance to 808 drums. The Karplus-Strong at low frequencies produces tuned percussion in the gamelan register. No existing preset places all 8 voices in full-algorithmic territory simultaneously.

2. **XVC at maximum with all four routes active**: The factory library rarely sets `perc_xvc_kick_to_tom_pitch` and `perc_xvc_snare_to_perc_blend` above 0.5. At full activation, the kit becomes genuinely reactive — each hit reshapes the next.

3. **Coupling-optimized kits**: Presets explicitly calibrated as OUROBOROS injection sources, OCEANIC murmuration triggers, or OWARE rhythmic event generators. These require restraint (clean transients, minimal reverb tail) rather than sonic richness.

4. **Tuned percussion kits**: Karplus-Strong at long decay (0.95+) on all voices, tuned to specific pitches (V7 Perc A at 220Hz, V8 Perc B at 440Hz). The resulting sounds are suspended metallic strings, not percussion. This territory is between drum synthesis and melodic synthesis.

5. **Underwater impact sounds**: Submerged has only three Onset presets. Deep, slow, heavy percussion with high reverb mix, slow attack, maximum body — sounds of geological impact, not acoustic drums.

---

## Phase R3: Parameter Refinements

### R3.1 — Blend Default Recalibration for Coupling Contexts

The existing preset library treats blend defaults as acoustic targets (blend=0.2 for 808-adjacent kicks). For coupling-source presets, the optimal blend is different: the modal resonator's output has more complex spectral content than the BridgedT oscillator, and this complexity creates more interesting modulation signals when fed to OUROBOROS via Injection. Coupling-source presets should experiment with kick blend at 0.5–0.6 to provide richer spectral content for downstream processing.

### R3.2 — XVC Global Amount Scaling

The XVC global amount is rarely set above 0.5 in factory presets. At 0.7–0.9, the cross-voice interactions become the dominant timbral force: each hit significantly reshapes the next. The most compelling coupling-source presets may have high XVC to create variation in the coupling signal itself — OUROBOROS receiving a varying injection signal is more interesting than receiving a metronomic one.

### R3.3 — Decay as Coupling Design Variable

For OCEANIC murmuration presets, the goal is clean transient delivery with minimal sustain. Setting all decay values to 0.05–0.15 creates tight, staccato hits whose velocity peaks are unambiguous for coupling detection. Long decays blur the coupling signal — the murmuration pattern reads as a slow swell rather than a rhythmic event.

### R3.4 — Tuned Percussion Synthesis

For OWARE-adjacent presets, use Modal synthesis (algoMode=1) on the kick with character=0 (pure Bessel zeros, circular membrane physics). The resulting fundamental + inharmonic mode stack resonates sympathetically with OWARE's own modal tuning. The Karplus-Strong on Perc A and B with long decay creates the plucked string character that completes the tuned kit.

### R3.5 — Stochastic Percussion Territory

All four Layer O algorithms can be driven into inharmonic territory through parameter extremes. FM at ratio > 3.0 with index > 5 becomes a metallic clatter. Modal resonator with character > 0.8 stretches the mode ratios toward metallic bar inharmonicity. Phase Distortion at distortion=0.95 creates waveforms with no acoustic reference. A kit that uses these extremes on all 8 voices simultaneously produces percussion that evades genre classification — an XVC network applied to these sounds creates an autonomous, strange rhythmic system.

---

## Phase R4: Awakening Presets (Transcendental Tier)

The fifteen Transcendental presets for ONSET are divided across five mood targets. They are listed here with their parameters and the territory each one enters that the factory library has not.

| # | Name | Mood | Territory |
|---|------|------|-----------|
| 1 | Onset Injection Protocol | Entangled | Coupling-source kit for OUROBOROS Injection |
| 2 | Onset Murmuration Source | Entangled | Coupling-source kit for OCEANIC RhythmToBlend |
| 3 | Onset Akan Assembly | Entangled | Tuned percussion kit for OWARE coupling |
| 4 | Onset Orbweave Impulse | Entangled | Coupling source for ORBWEAVE via AmpToFilter |
| 5 | Onset Stochastic Drum | Foundation | All-algorithmic, genre-free, full XVC active |
| 6 | Onset Electric Infrastructure | Foundation | Minimal, clean, no acoustic reference |
| 7 | Onset Transient Anatomy | Foundation | Full algorithmic, all 8 voices demonstrating dual-layer blend extremes |
| 8 | Onset Blend Crucible | Foundation | All voices at blend=0.5, the Circuit/Algorithm battleground |
| 9 | Onset Impulse Storm | Flux | High XVC, all four routes active, high MUTATE |
| 10 | Onset Frequency Pure | Flux | Karplus-Strong everywhere, tuned metallic strings |
| 11 | Onset Attack Topology | Flux | All voices heavily processing through the character stage |
| 12 | Onset Abyssal Strike | Submerged | Deep, geological, minimal frequency content |
| 13 | Onset Pressure Wave | Submerged | Underwater impact, long reverb, slow attack, heavy body |
| 14 | Onset Transient Rain | Atmosphere | Light, sparse, short decay, high reverb space |
| 15 | Onset Modal Depths | Atmosphere | Modal resonators with near-maximum inharmonicity |

---

## Phase R5: Scripture Verses

### Verse ONSET-I: The Surface Event

*From the Book of Bin, Vol 2, Chapter VII*

> ONSET does not live in the water. It lives at the water's surface — the boundary where feliX's electric energy escapes into air. Every trigger is a surface event: a disturbance, a splash, a stone thrown from above. The engines below feel the ripple. OUROBOROS adjusts its attractor state. OCEANIC changes the spacing in its murmuration. OWARE's membranes ring sympathetically. The surface does not choose what it disturbs. It simply arrives at the right moment. Rhythm is arrival at the right moment.

**Application:** When designing coupling-source presets, do not optimize for the sound ONSET makes in isolation. Optimize for the disturbance ONSET sends downstream. The injection signal is the instrument.

---

### Verse ONSET-II: The Eight Voices Are Not Eight Sounds

*From the Book of Bin, Vol 2, Chapter VII*

> The eight voices of ONSET are eight synthesis paradigms, not eight acoustic instruments. The Kick is a bridged-T oscillator that can be made to sound like nothing that has ever been hit with a stick. The Hat is six square waves at inharmonic ratios — the mathematical description of the 808's accidental genius. The Blend axis is the instrument between the instrument. Move it slowly. The circuit and the algorithm are not fighting for the same sound. They are offering different accounts of the same strike.

**Application:** Design at least one Transcendental preset where no voice's default blend is preserved. The parameter space outside factory defaults is where the Transcendental library lives.

---

### Verse ONSET-III: The Rhythm Brain Speaks Backward

*From the Book of Bin, Vol 2, Chapter VII*

> The Cross-Voice Coupling matrix does not operate in real time. It operates one block later. The kick's amplitude from this block modulates the snare's parameters in the next block. The snare's amplitude from this block tightens the hat in the next. The drum machine learns from what just happened and adjusts for what comes next. This is not feedback. It is memory. The rhythm brain speaks backward and forward simultaneously. The groove emerges from the conversation.

**Application:** XVC should be treated as a compositional tool, not a subtle enhancement. High XVC global amounts create presets that evolve over the course of a performance — the kit learns from itself.

---

### Verse ONSET-IV: The Prefix That Remembers Its Origin

*From the Book of Bin, Vol 2, Chapter VII (Homecoming Verses)*

> ONSET's parameters begin with `perc_`, not `onset_`. This is a historical artifact of a naming convention that changed after the engine was built. The prefix is not wrong — it is an accurate record of the moment the engine was made. Canon V-2 exists to protect this kind of truth. Do not rename parameters to match conventions that didn't exist when they were born. The prefix carries the engine's history in its letters. That history is not a liability. It is the proof the engine preceded the fleet.

**Application:** All Transcendental presets use `perc_` prefix. Never `onset_`. This is canonical.

---

*Guru Bin — ONSET Vol 2 Retreat complete*
*"The surface does not choose what it disturbs. It simply arrives."*
