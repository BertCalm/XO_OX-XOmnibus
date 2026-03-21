# ORBWEAVE Retreat Chapter — The Kelp Knot
*Guru Bin — 2026-03-21*

---

## Engine Identity

- **Gallery code:** ORBWEAVE | **Accent:** Kelp Knot Purple `#8E4585`
- **Parameter prefix:** `weave_`
- **Creature mythology:** The Kelp Knot — oscillators braided together like kelp fronds in a tidal current, their phases coupled through the geometry of knots
- **feliX-Oscar polarity:** Ambivalent — ORBWEAVE is the fleet's most topologically rigorous engine. It can be feliX-bright (Sine strands through a (2,5) cinquefoil) or Oscar-deep (Solomon chord pads with low cutoff and slow LFOs). The knot type is the polarity dial.
- **Synthesis type:** Phase-braided oscillator synthesis — 4 oscillator strands per voice, phases coupled per-sample through a configurable 4×4 knot-topology matrix
- **Polyphony:** Up to 8 voices (Mono / Legato / Poly4 / Poly8)
- **Macros:** WEAVE (braid depth push), TENSION (resonance + coupling feedback), KNOT (topology morph to next type), SPACE (FX wet blend)

---

## Pre-Retreat State

**Seance score: 8.3/10.** Added to the fleet 2026-03-20. A prior retreat session (see `scripture/retreats/orbweave-retreat.md`) established the canonical 6 WEAVE scripture verses and remediated 147 ghost parameters. All six Doctrines satisfied. The `Docs/guru-bin-retreats/` chapter is the formal Awakening document for student distribution.

The engine is mathematically coherent and musically underexplored. The seance revealed the Default Trap in motion: braidDepth sitting at 0.5 where neither the coupled nor decoupled character is heard. The Torus P/Q parameters were almost exclusively (2,3). The KNOT macro — a topology morpher unique in the fleet — sat at 0.0 in half the library.

The engine is not being explored. It is being decorated.

---

## Phase R1: Opening Meditation

Close your eyes. Breathe.

Imagine four threads of kelp anchored at the ocean floor, rising toward the light. They start as separate strands. Then the current finds them. Each one begins to move — not arbitrarily, but according to its relationship to its neighbors. The first strand bends toward the second. The second follows the third. The third loops around the first. The fourth watches, tethered loosely to the whole.

This is not vibrato. This is not chorus. This is not detuning. Each strand's frequency does not shift independently — each strand's frequency is *influenced per sample* by the current phase of its neighbor through a matrix of coupling coefficients that depends on the shape of a mathematical knot.

The four knot types in ORBWEAVE are not tonal modes or filter characters. They are four different theories about how oscillators can be related to each other. The Trefoil says: three strands form a directed ring, and the fourth floats free. The Figure-Eight says: every strand crosses two others in alternating over-under pattern. The Torus says: two axes of winding define the coupling geometry, and you choose the winding numbers. The Solomon says: two pairs form separate rings, weakly linked at the crossing.

When you play ORBWEAVE, you are not choosing a timbre. You are choosing a topology. And the topology becomes sound.

Sit with that. The kelp breathes. The braid tightens. The current changes nothing about the notes you play — and changes everything about what those notes become.

---

## Phase R2: The Signal Path Journey

### I. The Strand Frequencies — Four Voices in One Voice

Every MIDI note produces a single voice with four internal oscillators. Their frequencies are not independent — they are derived from the same base note through `weave_strandTune`:

- Strand 0: base note (root)
- Strand 1: base note + strandTune semitones (the interval above)
- Strand 2: base note − strandTune semitones (the interval below)
- Strand 3: base note + (strandTune × 2) semitones (the interval displaced by two steps)

At strandTune = 0, all four strands are at the same pitch. They are indistinguishable until the knot coupling separates them. At strandTune = 7.02 (the perfect fifth), strands 1 and 3 sit at the fifth and the octave+fifth — the four strands are now a chord voice built into the single MIDI note. Every note you play is already a chord. The knot determines how that chord's internal voices speak to each other.

This is the strand architecture's deepest insight: **the four strands are not an oscillator stack. They are the voices of a topological system.** The number 4 is chosen because the Solomon knot requires exactly two pairs. Four is the minimum for full topology expression.

### II. The Knot Phase Coupling — The Novel DSP

Every sample, before the strands output audio, the engine computes coupling offsets:

```
phaseOffset[i] = sum(matrix[i][j] × fastSin(strandPhase[j] × 2π) × braidDepth)
coupledFreq[i] = strandFreq[i] + phaseOffset[i] × 200 Hz
```

The coupling scale is 200 Hz — meaning a full sine swing (−1 to +1) from a neighbor strand can push or pull the target strand's instantaneous frequency by up to 200 Hz × braidDepth. At braidDepth = 0.0, there is no coupling — four independent oscillators, possibly detuned. At braidDepth = 1.0, the full 200 Hz influence is applied — the strands are maximally entangled.

The matrix coefficients encode the knot's geometric character:

**Trefoil** [crossing number 3, writhe +3]:
- Strand 0 is strongly pulled by Strand 1 (coefficient 0.7), weakly pulled against by Strand 3 (−0.3)
- Strand 1 is pulled by Strand 2
- Strand 2 is pulled by Strand 0, completing the ring
- Strand 3 is gently tethered to Strand 0 (0.2) — the "floating" fourth strand

The coupling flows in one direction around the three-strand ring. Energy circulates. Strand 3 drifts at the edge. The sound is asymmetric, alive with directional energy.

**Figure-Eight** [crossing number 4, all strands]:
- Coefficient pattern: over (+0.6), under (−0.4), alternating per row
- Every strand crosses two others — there are no free strands
- The alternating signs create interference: constructive on some phase relationships, destructive on others
- The result is more dense and anxious than the Trefoil — four-way mutual interference rather than a three-strand ring with an observer

**Torus** [parameterized by P/Q winding]:
- Base matrix: symmetric pairing (0.5 coefficients, adjacent-strand coupling)
- P/Q modulation applied asymmetrically: even-sum index pairs get `pqScale`, odd-sum pairs get `1 − pqScale×0.5`
- `pqScale = 0.5 + 0.5 × sin(P/Q × π)` — a coupling-asymmetry weight derived from the winding ratio
- (2,3) gives pqScale = 0.933. (2,5) gives 0.976. (5,8) gives 0.962. Each is a different spectral character.

**Solomon** [two doubly-linked rings]:
- Strands 0–1 = Ring A: strong intra-ring coupling (0.8), weak cross-ring coupling (0.3 to Ring B)
- Strands 2–3 = Ring B: strong intra-ring coupling (0.8), weak cross-ring coupling (0.3 to Ring A)
- When strandTune = 7.02, Ring A is at the root and Ring B is at the fifth — two tonally distinct rings, each with its own internal resonant character, gently linked

### III. Waveform × Topology Interaction — The Purity Principle

ORBWEAVE's coupling reads `fastSin(strandPhase)` regardless of waveform. For Sine strands, this equals the actual output — coupling is literal cross-modulation of the oscillator output. For Saw, Square, and Triangle, `strandPhase` is kept synchronized with the PolyBLEP oscillator but the waveforms diverge from sine.

This creates two genuinely different instruments:

- **Sine strands**: mathematically pure coupling. The coupling reads exactly what the oscillator outputs. This is correct knot-theory analog computation. The result is clear, inharmonic spectra shaped entirely by the topology. Use Sine when the knot structure should be the foreground.
- **Saw/Square/Triangle strands**: harmonic-rich coupling. The PolyBLEP waveforms add harmonic content that the coupling matrix does not see. The result is a textured, gritty sound where the topology provides movement and the waveform provides timbral weight. Use these when you want the knot to animate a harmonically dense source.

The simplest waveform is the most topologically rigorous. This is counterintuitive — and it is true.

### IV. The Torus P/Q Topology — Hidden Instrument Families

The Torus knot type contains multiple instruments:

| P | Q | pqScale | Character |
|---|---|---------|-----------|
| 2 | 3 | 0.933 | Trefoil cousin. Default. Slightly asymmetric coupling. |
| 2 | 5 | 0.976 | Cinquefoil. Near-maximum coupling asymmetry. 5-pointed star geometry. |
| 3 | 4 | 0.854 | Spiral. Lower pqScale than default — more balanced coupling. |
| 3 | 5 | 0.975 | Pentagram. 5-crossing winding. Rich interference structure. |
| 5 | 8 | 0.962 | Golden Torus. Fibonacci ratio approximates φ. Maximum spectral polarization. |
| 1 | 6 | 0.707 | The (1,6) unknot-like. Minimal asymmetry. Almost symmetric coupling. |

Each P/Q combination is a different coupling architecture. Most of the preset library uses (2,3). The cinquefoil and golden torus are the most sonically distinctive — almost no presets use them. They are instruments hiding inside the Torus knot type, waiting.

### V. The KNOT Macro — Topology Morphing

`macroKnot` linearly interpolates the entire 4×4 coupling matrix between `knotType` and `knotType+1` (with wrap-around at Solomon→Trefoil). At macroKnot = 0.5, every matrix element is halfway between two topologically distinct mathematical structures.

This creates a state that no mathematical knot theory describes. A coupling matrix that is the average of Trefoil and Figure-Eight coupling coefficients does not correspond to any named knot. It is a topological chimera — an entity between topologies.

**No other synthesizer in the fleet morphs between distinct mathematical topologies in real time.** Most synthesis parameters blend between two timbres (filter open/closed, wet/dry mix). The KNOT macro blends between two theories of oscillator relationship. The sound is not just between two timbres — it is between two kinds of logic.

The chimera state is often more interesting than either endpoint. Design presets where macroKnot = 0.5 is the *starting position*, not a swept extreme.

### VI. The Amp Envelope — Velocity and Filter Expression

The 2000 Hz/velocity scaling law shapes how expressive ORBWEAVE is:

- `filterCutoff` = 8000 Hz: velocity adds at most +2000 Hz (8000→10000). Barely perceptible.
- `filterCutoff` = 500 Hz: velocity adds up to +2000 Hz (500→2500). A 5× frequency transformation.

**WEAVE-VI in practice:** Design velocity-expressive presets with filterCutoff below 2000 Hz. The low-cutoff zone is where ORBWEAVE becomes a velocity instrument. The high-cutoff zone is where it becomes a consistent pad. This is not a bug — it is a design constraint that should be used intentionally.

Aftertouch pushes braidDepth by up to +0.3 — at the threshold boundary (braidDepth = 0.7 to 1.0), aftertouch transitions the knot from the detuned zone into full topology. If you want aftertouch to feel transformative rather than cosmetic, set braidDepth between 0.5 and 0.7 and let aftertouch drive the engine across the WEAVE-I threshold.

---

## Phase R3: The Five Meditations

### Meditation 1 — The Braid Depth Threshold (WEAVE-I)

Set braidDepth to 0.1. Play a chord. Listen.

Increase to 0.5. Play the chord again. Is anything different?

Increase to 0.8. Play the chord again.

At 0.1, you have four detuned oscillators. At 0.5, you have something slightly unusual. At 0.8, you have something impossible with standard synthesis — a spectral character that shifts dynamically per-sample based on the instantaneous phase relationships between strands. The braid depth threshold is approximately 0.65–0.7. Below this, ORBWEAVE sounds like a detuned synthesizer with an interesting name. Above it, ORBWEAVE sounds like ORBWEAVE.

**The lesson:** braidDepth is not a mix control. It is a threshold control. The choice is: below the threshold (detuned zone) or above it (topology zone). Both are valid. Neither is a compromise.

### Meditation 2 — The Sine Purity Experiment

Load any Saw-strand preset. Play a note. Listen.

Switch strandType to Sine. Play the same note at the same pitch and velocity.

The knot structure becomes transparent. With Saw strands, the harmonic content of the waveform is louder than the topology. With Sine strands, there is nothing else to hear — the topology is the entire sound.

**The lesson:** When you cannot hear the knot type, switch to Sine and listen again. Then switch back to Saw with knowledge of what you are shaping.

### Meditation 3 — The P/Q Archaeology

Set knotType to Torus. Set P=2, Q=3. Play a sustained note. Listen for two full seconds.

Change Q to 5 (keeping P=2). Listen again.

Change to P=5, Q=8. Listen again.

These are different instruments. The P/Q parameter is not a tone control — it is a geometry selector. The cinquefoil (2,5) has a five-pointed star winding. The golden torus (5,8) approximates the irrational golden ratio, creating coupling that never settles into a periodic pattern.

**The lesson:** Before you finalize any Torus preset, test at least three P/Q combinations. The default (2,3) is rarely the most interesting.

### Meditation 4 — The Solomon Chord Architecture

Set knotType to Solomon. Set strandTune to 7.02. Set braidDepth to 0.78.

Play a single MIDI note. What do you hear?

You hear two simultaneous pitch identities — the root and the fifth — each with its own internal resonant character. Ring A (strands 0–1) couples internally at 0.8 strength and bleeds into Ring B (strands 2–3) at 0.3. The two rings are harmonically distinct but not independent. They share a crossing.

**The lesson:** The Solomon knot is the chord pad topology. When you play a single note into Solomon, you are playing an implied chord structure with built-in internal dynamics. The other three knots are single-pitch topologies. Solomon is a two-pitch system by design.

### Meditation 5 — The Velocity Architecture Experiment

Set filterCutoff to 600 Hz. Set filterReso to 0.3. Play at minimum velocity (pianissimo). Listen.

Play at maximum velocity (fortissimo). Listen.

The difference should be dramatic — 600 Hz versus 2600 Hz of filter cutoff. The same note becomes a completely different instrument at different velocities.

Now increase filterCutoff to 8000 Hz. Repeat the velocity experiment.

The difference collapses. Both velocities sound similar. The 2000 Hz offset is small relative to 8000 Hz.

**The lesson:** Velocity expression lives in the low-cutoff zone. If a preset is meant to be velocity-expressive, its filterCutoff must be low enough for 2000 Hz to matter. If a preset should be velocity-neutral (consistent pads, predictable leads), set filterCutoff above 6000 Hz.

---

## Phase R4: The Four Macros — Four Questions

| Macro | Question it asks | What it controls |
|-------|-----------------|-----------------|
| WEAVE | *How deeply are we bound?* | Pushes braidDepth toward 1.0 from its current value. `effBraidDepth = braidDepth + WEAVE × (1 − braidDepth)`. Never reduces coupling — only increases it. |
| TENSION | *How tightly does the knot hold?* | Adds up to 0.4 of filter resonance. Boosts the standing resonance of the low-pass filter, making the coupling's spectral peaks sharper and more prominent. |
| KNOT | *What kind of knot are we?* | Blends the current knotType's coupling matrix toward knotType+1. At 0.5, maximum chimera. At 1.0, the next topology in sequence. |
| SPACE | *Where in the world are we?* | Scales FX wet amounts from current values toward 1.0. Also modulates chorus LFO rate and reverb input gain — more SPACE = wider, longer, more diffuse. |

### WEAVE — Performance Wisdom

WEAVE never decreases the braidDepth. It only pushes it upward. This means WEAVE is a one-way control: it can only entangle, never separate. Design presets with braidDepth in the 0.3–0.5 range and use the WEAVE macro to dynamically push into the topology zone (0.7+) during performance. The performer's left hand is the entanglement dial. Pulling back on WEAVE does not untangle — it stops the push. Use this asymmetry deliberately.

### TENSION — When to Use It

TENSION adds resonance to the filter. At macroTension = 1.0, filter resonance increases by +0.4. This is substantial — at filterReso = 0.0, TENSION brings the filter to 0.4 resonance. The coupling peaks become self-resonant ridges. Use TENSION when you want the knot's spectral peaks to ring — for leads, for accents, for moments of harmonic emphasis. Use TENSION sparingly for pads and consistently-voiced textures where you do not want spectral peaks to dominate.

### KNOT — The Fleet's Most Unique Single Control

There is no equivalent of the KNOT macro in any other synthesizer in the fleet or in most commercial synthesizers. It sweeps through mathematically distinct topologies in real time. The motion from Trefoil to Figure-Eight is not a timbre crossfade — it is a transition between two theories of oscillator relationship.

At macroKnot = 0.0: pure knotType, pure topology.
At macroKnot = 0.5: chimera state — between two topologies, at neither.
At macroKnot = 1.0: the next knotType, fully.

The chimera at 0.5 is an instrument that does not exist in mathematics. It exists only in ORBWEAVE. Name the chimera state. Give it its own identity. The macroKnot = 0.5 point deserves its own preset documentation — it is not a transition, it is a destination.

### SPACE — Spatial Architecture

SPACE controls how environmental ORBWEAVE sounds. At 0.0, the knot coupling is the spatial architecture — the phase interference between strands creates apparent stereo width from a purely monophonic coupling process. At 1.0 (with active FX slots), the coupling texture sits inside a defined room. The best ORBWEAVE presets choose: either the knot IS the space (no FX, SPACE=0), or the space enhances the knot (FX active, SPACE used as a room-size control).

---

## Phase R5: Coupling Wisdom

ORBWEAVE accepts coupling inputs through four routes:

| Coupling Type | ORBWEAVE's Interpretation |
|--------------|--------------------------|
| AudioToFM | Adds to couplingPitchMod — modulates the effective pitch of all strands via external audio |
| AmpToFilter | Adds to couplingCutoffMod — external amplitude controls filter brightness |
| LFOToPitch | Same pathway as AudioToFM at 0.5× scaling |
| AmpToPitch | Adds to couplingPitchMod at 0.3× scaling |
| AmpToChoke | Triggers note-off on all active voices — external amplitude gates ORBWEAVE |

The most powerful coupling route is **AmpToFilter from a rhythmic engine (ONSET, OBRIX) to ORBWEAVE**. The drum hits pump the filter on ORBWEAVE's voices — the knot coupling's spectral peaks flash bright with each hit and decay into the coupling texture. This turns ORBWEAVE into a rhythmically animated topology — the knot breathes to the kick drum.

The **AmpToChoke** coupling creates an inverse gate: ORBWEAVE sounds only in the spaces between another engine's loud moments. Route ONSET's amplitude to ORBWEAVE via AmpToChoke at high depth — ORBWEAVE speaks in the gaps between drum hits. A counterpoint instrument driven by the inverse of a rhythmic gate.

**ORBWEAVE as coupling source:** Its audio output contains phase coupling artifacts — sidebands, aliased partials, sub-harmonics — that are rich modulation material when routed to another engine via AudioToFM. The coupling output has more spectral energy in unexpected frequency regions than a standard oscillator, making it a particularly interesting FM source.

---

## Phase R6: The Awakening Preset Table

Ten presets for the Awakening tier. Gold star quality. Each teaches one thing the engine does that no other engine does.

---

### 1. Bare Topology

**Mood:** Foundation | **Knot:** Trefoil | **Discovery:** The reference state — what coupling sounds like in isolation

| Parameter | Value | Why |
|-----------|-------|-----|
| `weave_strandType` | 1 (Sine) | Purest coupling math — reads strandPhase exactly as oscillator output |
| `weave_strandTune` | 0.0 | All four strands at identical pitch — coupling creates spectral separation, not interval spreading |
| `weave_knotType` | 0 (Trefoil) | Three-strand directed ring — the simplest named knot |
| `weave_braidDepth` | 0.80 | Above the WEAVE-I threshold (0.65–0.7) — topology audible as topology |
| `weave_torusP/Q` | 2/3 | Default; not relevant at Trefoil |
| `weave_filterCutoff` | 6000 Hz | Bright enough to hear coupling partials; not so bright it exposes aliasing |
| `weave_filterReso` | 0.05 | Minimal — let the coupling be the only spectral shaping |
| `weave_filterType` | 0 (LP) | Standard |
| `weave_ampA` | 0.01 | Fast attack — immediate entry |
| `weave_ampD` | 0.5 | Medium decay |
| `weave_ampS` | 0.75 | Sustain at 75% — audible hold |
| `weave_ampR` | 1.5 | Long release — hear coupling decay naturally |
| `weave_lfo1Type` | 0 (Off) | No modulation — static topology reference |
| `weave_lfo2Type` | 0 (Off) | — |
| `weave_fx1Type` | 0 (Off) | No FX — bare topology in the room |
| `weave_fx2Type` | 0 (Off) | — |
| `weave_fx3Type` | 0 (Off) | — |
| `weave_macroWeave` | 0.0 | Macro demonstrates coupling push when swept |
| `weave_macroTension` | 0.0 | — |
| `weave_macroKnot` | 0.0 | — |
| `weave_macroSpace` | 0.0 | — |
| `weave_voiceMode` | 3 (Poly8) | Polyphonic — hear coupling per voice |
| `weave_pitchBendRange` | 2 | Standard |
| `weave_glideTime` | 0.0 | — |

**Sonic character:** A pure sustained tone whose partials shift slightly between notes — not vibrato, not detuning. The topology creates a spectral character that is unique to this engine. Play intervals and observe how the coupling between strands interacts with the interval between notes. This is the reference preset — the minimal demonstration of what ORBWEAVE is.

**Macro use:** Sweep WEAVE from 0 to 1.0 while holding a note. Hear the coupling engage as braidDepth approaches 1.0.

---

### 2. Solomon Chord Pad

**Mood:** Atmosphere | **Knot:** Solomon | **Discovery:** Each MIDI note produces a two-ring chord with internal dynamics

| Parameter | Value | Why |
|-----------|-------|-----|
| `weave_strandType` | 1 (Sine) | Pure coupling between the two rings |
| `weave_strandTune` | 7.02 | Perfect fifth — Ring A at root, Ring B at fifth |
| `weave_knotType` | 3 (Solomon) | The doubly-linked two-ring topology |
| `weave_braidDepth` | 0.78 | Above threshold — intra-ring coupling audible |
| `weave_filterCutoff` | 4500 Hz | Warm — reduces bright coupling harmonics |
| `weave_filterReso` | 0.12 | Slight resonance amplifies coupling peaks in the mid-range |
| `weave_filterType` | 0 (LP) | — |
| `weave_ampA` | 0.35 | Slow attack — pads bloom |
| `weave_ampD` | 0.6 | Medium decay |
| `weave_ampS` | 0.82 | High sustain — the chord rings |
| `weave_ampR` | 3.5 | Very long release — ring pairs decay independently |
| `weave_lfo1Type` | 1 (Sine) | — |
| `weave_lfo1Target` | 5 (Braid Depth) | LFO gently breathes the coupling strength |
| `weave_lfo1Depth` | 0.15 | Subtle — audible ring variation without instability |
| `weave_lfo1Rate` | 0.09 | Below 0.1 Hz — ocean-breath tempo |
| `weave_lfo2Type` | 0 (Off) | — |
| `weave_fx1Type` | 3 (Reverb) | Space for the rings to exist in |
| `weave_fx1Mix` | 0.32 | Moderate reverb |
| `weave_fx1Param` | 0.55 | Medium-long reverb tail |
| `weave_macroWeave` | 0.0 | WEAVE pushes into tighter ring coupling on performance |
| `weave_macroTension` | 0.0 | TENSION accents the ring resonances |
| `weave_macroKnot` | 0.0 | KNOT morphs toward a hybrid Solomon-Trefoil state |
| `weave_macroSpace` | 0.0 | SPACE widens the reverb |
| `weave_voiceMode` | 3 (Poly8) | — |
| `weave_glideTime` | 0.0 | — |

**Sonic character:** Each note is a chord. Ring A at the root breathes with Ring B at the fifth, the two rings coupling gently (0.3 cross-link) while each ring has its own internal resonance (0.8 intra-ring). The slow LFO on braid depth creates a living ring structure — coupling is slightly stronger and weaker over the 11-second LFO cycle. Playing open fifth intervals in the left hand while adding top voice melody creates a three-layer texture: the implicit chord from strandTune, the harmonic content from coupling, and the melody.

**Macro use:** WEAVE increases coupling depth into a more tightly interlocked ring structure. TENSION adds resonance to the coupling peaks, making the ring architecture more prominent. KNOT begins morphing the Solomon matrix toward Trefoil — the two rings lose their independence and begin to form a single loop.

---

### 3. Cinquefoil Lead

**Mood:** Prism | **Knot:** Torus (2,5) | **Discovery:** The cinquefoil — five-pointed star geometry, near-maximum coupling asymmetry

| Parameter | Value | Why |
|-----------|-------|-----|
| `weave_strandType` | 2 (Saw) | Harmonic richness — the cinquefoil's asymmetry is more audible with harmonic content |
| `weave_strandTune` | 0.0 | All strands at root — coupling creates the spectral identity |
| `weave_knotType` | 2 (Torus) | — |
| `weave_braidDepth` | 0.82 | High coupling — cinquefoil at full strength |
| `weave_torusP` | 2 | — |
| `weave_torusQ` | 5 | Cinquefoil winding. pqScale = 0.976 — near-maximum asymmetry |
| `weave_filterCutoff` | 3500 Hz | D001 active: velocity maps 3500→5500 Hz with full velocity swing |
| `weave_filterReso` | 0.25 | Prominent resonance — coupling peaks more audible |
| `weave_filterType` | 0 (LP) | — |
| `weave_ampA` | 0.005 | Snappy attack — lead response |
| `weave_ampD` | 0.35 | Fast decay |
| `weave_ampS` | 0.60 | Moderate sustain |
| `weave_ampR` | 0.4 | Short release — staccato capable |
| `weave_lfo1Type` | 1 (Sine) | — |
| `weave_lfo1Target` | 2 (Filter Cutoff) | Classic filter modulation, but the coupling is doing deeper modulation simultaneously |
| `weave_lfo1Depth` | 0.25 | Moderate filter LFO — harmonic animation |
| `weave_lfo1Rate` | 0.45 | Mid-rate — noticeable but not frenetic |
| `weave_lfo2Type` | 0 (Off) | — |
| `weave_fx1Type` | 1 (Delay) | Short delay for lead definition |
| `weave_fx1Mix` | 0.18 | Subtle echo — lead emphasis |
| `weave_fx1Param` | 0.35 | Medium delay time |
| `weave_macroWeave` | 0.0 | Performance: pushes deeper into cinquefoil coupling |
| `weave_macroTension` | 0.0 | Performance: adds resonance — transitions toward self-oscillation character |
| `weave_macroKnot` | 0.0 | Performance: morphs cinquefoil-Torus toward Solomon |
| `weave_macroSpace` | 0.0 | — |
| `weave_voiceMode` | 3 (Poly8) | — |
| `weave_pitchBendRange` | 7 | Wider bend range for lead expression |

**Sonic character:** The (2,5) cinquefoil has a five-pointed star cross-section on the torus surface, creating a coupling asymmetry (pqScale = 0.976) that is noticeably different from the default (2,3). Fast notes at low velocity have a tight, percussive cinquefoil click. Hard strikes at high velocity open the filter into a bright, inharmonic lead tone. The LFO animates the filter over the coupling's static spectral character. This is a topology you can feel under your fingers — faster playing yields different coupling phase relationships than slow playing.

**Macro use:** TENSION brings up resonance — the cinquefoil's coupling peaks become self-resonant. At TENSION = 0.6 or above, the coupling peaks begin to sing on sustained notes.

---

### 4. Kelp Forest Pad

**Mood:** Atmosphere | **Knot:** Solomon | **Discovery:** Dual LFO on braid + cutoff creates a breathing spatial texture

| Parameter | Value | Why |
|-----------|-------|-----|
| `weave_strandType` | 1 (Sine) | Pure coupling — minimal timbre, maximum topology |
| `weave_strandTune` | 5.0 | Major third — Ring A at root, Ring B at major third. Softer than fifth. |
| `weave_knotType` | 3 (Solomon) | Two-ring architecture for the pad's chord structure |
| `weave_braidDepth` | 0.72 | Just above WEAVE-I threshold — topology audible, not overwhelming |
| `weave_filterCutoff` | 5200 Hz | Open filter — pads need to breathe |
| `weave_filterReso` | 0.08 | Minimal resonance — no peaks cutting through the pad |
| `weave_filterType` | 0 (LP) | — |
| `weave_ampA` | 0.55 | Slow attack — the forest takes time to appear |
| `weave_ampD` | 0.8 | Long decay |
| `weave_ampS` | 0.88 | Very high sustain — pads should hang |
| `weave_ampR` | 4.5 | Very long release — notes leave trails |
| `weave_lfo1Type` | 1 (Sine) | — |
| `weave_lfo1Target` | 5 (Braid Depth) | The coupling breathes |
| `weave_lfo1Depth` | 0.22 | Moderate — audible breathing |
| `weave_lfo1Rate` | 0.067 | 15-second cycle — ocean breath (from WEAVE-III) |
| `weave_lfo2Type` | 1 (Sine) | — |
| `weave_lfo2Target` | 2 (Filter Cutoff) | Separate filter drift — asynchronous with coupling breath |
| `weave_lfo2Depth` | 0.18 | Subtle filter movement |
| `weave_lfo2Rate` | 0.11 | 9-second cycle — out of phase with braid LFO creates polyrhythmic breathing |
| `weave_fx1Type` | 3 (Reverb) | — |
| `weave_fx1Mix` | 0.52 | Deep reverb — the forest has distance |
| `weave_fx1Param` | 0.72 | Long reverb tail |
| `weave_fx2Type` | 2 (Chorus) | Spreading the pad width |
| `weave_fx2Mix` | 0.22 | Moderate chorus width |
| `weave_fx2Param` | 0.4 | Medium chorus depth |
| `weave_macroWeave` | 0.0 | Performance: deepens coupling into a tighter ring structure |
| `weave_macroTension` | 0.0 | Performance: adds resonance to the ring coupling peaks |
| `weave_macroKnot` | 0.0 | Performance: morphs the forest into a different topology |
| `weave_macroSpace` | 0.0 | Performance: opens the reverb and chorus wet amounts |
| `weave_voiceMode` | 3 (Poly8) | — |

**Sonic character:** A slow-moving, spatially rich pad where the coupling and filter breathe at different rates (15s vs. 9s), creating a polyrhythmic expansion and contraction with a 45-second combined period before the pattern repeats. The Solomon structure at a major third interval creates a harmonic pad that implies a chord within each note. The deep reverb puts the coupling texture inside an imagined space. Close your eyes and there are four strands of kelp in a forest, each with its own current.

---

### 5. Figure-Eight Bass

**Mood:** Foundation | **Knot:** Figure-Eight | **Discovery:** Mono + glide through the alternating over/under topology creates a unique portamento character

| Parameter | Value | Why |
|-----------|-------|-----|
| `weave_strandType` | 2 (Saw) | Bass needs harmonic weight |
| `weave_strandTune` | 0.0 | All strands at root — no chord spreading for a bass instrument |
| `weave_knotType` | 1 (Figure-Eight) | Alternating-sign crossing: over (+0.6), under (−0.4) per strand pair |
| `weave_braidDepth` | 0.85 | Full coupling — the figure-eight creates audible alternating interference |
| `weave_filterCutoff` | 400 Hz | D001 active — velocity opens filter from 400→2400 Hz. Powerful timbre arc. |
| `weave_filterReso` | 0.35 | Resonance at the cutoff creates a growling mid-peak |
| `weave_filterType` | 0 (LP) | — |
| `weave_ampA` | 0.005 | Snappy bass attack |
| `weave_ampD` | 0.45 | Decay shapes the punch |
| `weave_ampS` | 0.55 | Moderate sustain |
| `weave_ampR` | 0.4 | Short release — bass notes end cleanly |
| `weave_lfo1Type` | 0 (Off) | — |
| `weave_lfo2Type` | 0 (Off) | Static topology — no modulation on the bass coupling |
| `weave_fx1Type` | 0 (Off) | Dry bass — effects on the send bus |
| `weave_macroWeave` | 0.0 | Performance: pushes into maximum coupling |
| `weave_macroTension` | 0.0 | Performance: adds resonance peak for accent |
| `weave_macroKnot` | 0.0 | — |
| `weave_macroSpace` | 0.0 | — |
| `weave_voiceMode` | 0 (Mono) | Bass is monophonic |
| `weave_pitchBendRange` | 5 | Wide bend for bass slides |
| `weave_glideTime` | 0.15 | Short glide — bass portamento through topology |

**Sonic character:** Low filterCutoff makes velocity an expressive bass control: ghost notes at 30% velocity are dark and quiet (400 Hz), accents at 100% velocity are bright and cutting (2400 Hz). The Figure-Eight's alternating over/under coupling (+0.6/−0.4 per strand pair) creates a slightly nasal, growling tone quality that is different from standard bass synthesis — the alternating interference creates a throatier spectral character. Glide between notes passes the topology through intermediate phase states — the portamento slides through the Figure-Eight's crossing structure.

**Velocity use:** Ghost notes → dark and buried. Accents → topology snaps bright. The 2000 Hz arc is the most expressive at 400 Hz base cutoff in the entire engine.

---

### 6. Topology Drift

**Mood:** Flux | **Knot:** Torus → Solomon chimera | **Discovery:** The KNOT macro at 0.5 creates a topological chimera — an instrument between two mathematical structures

| Parameter | Value | Why |
|-----------|-------|-----|
| `weave_strandType` | 2 (Saw) | Harmonic content makes the chimera transitions audible |
| `weave_strandTune` | 3.86 | Minor third — slightly dark interval for the drift character |
| `weave_knotType` | 2 (Torus) | knotB = Solomon (knotType+1). Chimera is Torus←→Solomon halfway point. |
| `weave_braidDepth` | 0.68 | Just above threshold — topology active |
| `weave_torusP` | 3 | — |
| `weave_torusQ` | 4 | (3,4) spiral torus — pqScale = 0.854. Less asymmetric than (2,3). |
| `weave_filterCutoff` | 5000 Hz | — |
| `weave_filterReso` | 0.18 | Slight resonance |
| `weave_filterType` | 0 (LP) | — |
| `weave_ampA` | 0.04 | Fast attack |
| `weave_ampD` | 0.35 | — |
| `weave_ampS` | 0.72 | — |
| `weave_ampR` | 0.8 | — |
| `weave_lfo1Type` | 1 (Sine) | — |
| `weave_lfo1Target` | 5 (Braid Depth) | Coupling slowly breathes |
| `weave_lfo1Depth` | 0.12 | Subtle — braid variation |
| `weave_lfo1Rate` | 0.18 | 5-second cycle |
| `weave_fx1Type` | 2 (Chorus) | — |
| `weave_fx1Mix` | 0.22 | — |
| `weave_fx1Param` | 0.45 | — |
| `weave_macroWeave` | 0.65 | Pre-positioned: braid depth pushed up significantly |
| `weave_macroTension` | 0.15 | Light resonance already present |
| `weave_macroKnot` | 0.50 | **Pre-positioned at chimera state** — halfway between Torus and Solomon |
| `weave_macroSpace` | 0.15 | Light chorus active |
| `weave_voiceMode` | 2 (Poly4) | Moderate polyphony for the drift character |

**Sonic character:** This preset opens in the chimera state. The coupling matrix is the 50/50 average of (3,4)-Torus and Solomon matrix coefficients — a state that cannot be named by knot theory. The preset is designed to be swept: pull KNOT left toward 0.0 (pure (3,4) torus) or right toward 1.0 (pure Solomon) during performance. Sweeping KNOT while playing a sustained chord reveals the topological transition: the crossing character changes from symmetric-torus to two-ring-Solomon in real time.

**Macro use:** This is the only preset in the library designed with macroKnot = 0.5 as the starting position. The recommended performance: hold a chord, sweep KNOT slowly from left to right over 4 bars. The harmonic character shifts in a way that no filter sweep or oscillator parameter produces.

---

### 7. Golden Torus Bells

**Mood:** Prism | **Knot:** Torus (5,8) | **Discovery:** The golden ratio winding number creates coupling that never settles — irrational periodicity

| Parameter | Value | Why |
|-----------|-------|-----|
| `weave_strandType` | 1 (Sine) | Pure coupling — bell tones need spectral clarity |
| `weave_strandTune` | 0.0 | All strands at root — coupling creates the bell character |
| `weave_knotType` | 2 (Torus) | — |
| `weave_braidDepth` | 0.74 | Above threshold |
| `weave_torusP` | 5 | — |
| `weave_torusQ` | 8 | Golden torus — 8/5 approximates φ = 1.618. pqScale = 0.962. |
| `weave_filterCutoff` | 18000 Hz | Very bright — bell tones |
| `weave_filterReso` | 0.0 | No resonance — coupling creates its own spectral peaks |
| `weave_filterType` | 0 (LP) | Minimal filter involvement |
| `weave_ampA` | 0.003 | Instant attack — bell strike |
| `weave_ampD` | 1.2 | Long decay — bell ring |
| `weave_ampS` | 0.0 | No sustain — bell envelope |
| `weave_ampR` | 2.0 | Long release — bell reverberance |
| `weave_lfo1Type` | 0 (Off) | — |
| `weave_lfo2Type` | 0 (Off) | — |
| `weave_fx1Type` | 3 (Reverb) | — |
| `weave_fx1Mix` | 0.30 | Space for the bells |
| `weave_fx1Param` | 0.45 | Medium reverb |
| `weave_macroWeave` | 0.0 | WEAVE increases bell coupling depth — deeper φ-ratio interlocking |
| `weave_macroTension` | 0.0 | — |
| `weave_macroKnot` | 0.0 | KNOT morphs toward Solomon — bells become pad |
| `weave_macroSpace` | 0.0 | SPACE opens the reverb |
| `weave_voiceMode` | 3 (Poly8) | Chord bells |
| `weave_pitchBendRange` | 2 | — |

**Sonic character:** The (5,8) golden torus coupling weight (pqScale = 0.962) creates phase coupling that approximates an irrational winding ratio — no two oscillation cycles have exactly the same coupling relationship. Over time, the spectral character of sustained notes shifts almost imperceptibly, never settling into a fully periodic pattern. This is mathematically correct: the golden ratio is the most irrational number (least well approximated by rationals), creating the maximum variation in the coupling asymmetry between even and odd strand pairs. The result is a bell timbre with subtle, non-repeating shimmer — as if the bell were made of an alloy with irrational molecular structure.

**The φ insight:** The golden ratio appears in shell spirals, phyllotaxis, and optimal packing. Here it controls how oscillator pairs couple asymmetrically. The (5,8) torus is the closest integer approximation available in the P/Q range. It sounds different from (2,3). Play both. The difference is subtle but real — the golden torus has more long-term spectral variation.

---

### 8. Breathing Trefoil

**Mood:** Atmosphere | **Knot:** Trefoil | **Discovery:** Autonomous LFO on braid depth at very slow rate creates a living topology

| Parameter | Value | Why |
|-----------|-------|-----|
| `weave_strandType` | 1 (Sine) | Pure coupling — the breathing topology is the sound |
| `weave_strandTune` | 2.0 | Minor second — subtle interval spreading. Strands close but not unison. |
| `weave_knotType` | 0 (Trefoil) | Directed three-strand ring with floating fourth strand |
| `weave_braidDepth` | 0.45 | Below the WEAVE-I threshold on purpose — the LFO will breathe it above |
| `weave_filterCutoff` | 6500 Hz | — |
| `weave_filterReso` | 0.1 | Slight resonance |
| `weave_filterType` | 0 (LP) | — |
| `weave_ampA` | 0.30 | Slow attack — atmospheric |
| `weave_ampD` | 0.5 | — |
| `weave_ampS` | 0.78 | High sustain |
| `weave_ampR` | 3.0 | Long release |
| `weave_lfo1Type` | 1 (Sine) | — |
| `weave_lfo1Target` | 5 (Braid Depth) | — |
| `weave_lfo1Depth` | 0.38 | Strong modulation: braidDepth cycles from 0.45−0.38=0.07 to 0.45+0.38=0.83 |
| `weave_lfo1Rate` | 0.067 | 15-second cycle (≈ 0.067 Hz) — the ocean breath rate from retreat notes |
| `weave_lfo2Type` | 0 (Off) | — |
| `weave_fx1Type` | 3 (Reverb) | — |
| `weave_fx1Mix` | 0.40 | Deep reverb |
| `weave_fx1Param` | 0.60 | Long tail |
| `weave_macroWeave` | 0.0 | WEAVE raises the floor — minimum braid depth increases |
| `weave_macroTension` | 0.0 | — |
| `weave_macroKnot` | 0.0 | — |
| `weave_macroSpace` | 0.0 | SPACE opens the reverb |
| `weave_voiceMode` | 3 (Poly8) | — |

**Sonic character:** The braidDepth LFO depth of 0.38 with a center of 0.45 means the braid depth cycles from ~0.07 (decoupled zone) to ~0.83 (deep topology zone) over 15 seconds. The topology literally breathes: every 7.5 seconds, the knot structure emerges from near-silence and recedes again. When braidDepth is at the peak, Trefoil coupling is fully audible — the directed ring, the floating fourth strand. When at the trough, it sounds like four independent oscillators with a minor-second spread.

This is a different kind of breathing than vibrato or tremolo. The spectral character of the tone is changing, not just the pitch or volume. The Trefoil's topology emerges and retreats as if the knot were being tied and loosened.

---

### 9. Figure-Eight Drift

**Mood:** Aether | **Knot:** Figure-Eight → Solomon chimera | **Discovery:** Two LFOs at coprime rates create a non-repeating coupling texture

| Parameter | Value | Why |
|-----------|-------|-----|
| `weave_strandType` | 1 (Sine) | Pure coupling for the aether mood |
| `weave_strandTune` | 9.0 | Major sixth — wide interval, strands cover more tonal space |
| `weave_knotType` | 1 (Figure-Eight) | knotB = Torus. Figure-Eight chimera is the FE→Torus halfway state. |
| `weave_braidDepth` | 0.73 | Above threshold |
| `weave_torusP` | 2 | — |
| `weave_torusQ` | 3 | — |
| `weave_filterCutoff` | 7500 Hz | Bright — aether mood |
| `weave_filterReso` | 0.15 | — |
| `weave_filterType` | 0 (LP) | — |
| `weave_ampA` | 0.50 | Very slow attack — aether emerges |
| `weave_ampD` | 1.0 | Long decay |
| `weave_ampS` | 0.85 | — |
| `weave_ampR` | 5.5 | Very long release — notes float in space |
| `weave_lfo1Type` | 1 (Sine) | — |
| `weave_lfo1Target` | 5 (Braid Depth) | — |
| `weave_lfo1Depth` | 0.20 | Moderate braid breathing |
| `weave_lfo1Rate` | 0.13 | 7.7-second cycle |
| `weave_lfo2Type` | 1 (Sine) | — |
| `weave_lfo2Target` | 2 (Filter Cutoff) | Separate filter drift |
| `weave_lfo2Depth` | 0.22 | ±0.22 × 6000 Hz = ±1320 Hz filter drift |
| `weave_lfo2Rate` | 0.083 | 12-second cycle — 13 and 7.7 second periods are coprime-like, combining for ~100s total cycle |
| `weave_fx1Type` | 3 (Reverb) | — |
| `weave_fx1Mix` | 0.58 | Deep reverb for aether |
| `weave_fx1Param` | 0.78 | Very long reverb tail |
| `weave_fx2Type` | 2 (Chorus) | — |
| `weave_fx2Mix` | 0.28 | — |
| `weave_fx2Param` | 0.35 | — |
| `weave_macroWeave` | 0.0 | — |
| `weave_macroTension` | 0.0 | — |
| `weave_macroKnot` | 0.0 | KNOT sweeps from FE toward Torus |
| `weave_macroSpace` | 0.0 | SPACE opens the reverb and chorus |
| `weave_voiceMode` | 3 (Poly8) | — |

**Sonic character:** The Figure-Eight's four-strand mutual interference is animated by two LFOs at approximately 7.7s and 12s cycles. Their periods are not integer multiples of each other (~7.7 × 1.56 ≈ 12), so the combined pattern repeats approximately every 7.7 × 12 / gcd ≈ 92 seconds. In practice, no two measures of this preset sound the same — the braid depth and filter cutoff drift asynchronously across a ~90-second combined cycle. An aether instrument by definition. Hold a chord for a minute and listen to the patch evolve.

---

### 10. Knot Theory at Low Velocity

**Mood:** Foundation | **Knot:** Trefoil | **Discovery:** Low filterCutoff + velocity scaling creates a complete timbral arc from whisper to declaration

| Parameter | Value | Why |
|-----------|-------|-----|
| `weave_strandType` | 2 (Saw) | Harmonic weight for the dark timbre |
| `weave_strandTune` | 0.0 | All strands at root — velocity should move the timbre, not the intervals |
| `weave_knotType` | 0 (Trefoil) | — |
| `weave_braidDepth` | 0.77 | Above threshold |
| `weave_filterCutoff` | 350 Hz | Maximum velocity expressivity: 350→2350 Hz arc |
| `weave_filterReso` | 0.42 | High resonance at the low cutoff — the coupling peaks ring inside the filter resonance |
| `weave_filterType` | 0 (LP) | — |
| `weave_ampA` | 0.01 | Fast attack |
| `weave_ampD` | 0.5 | Medium decay |
| `weave_ampS` | 0.65 | — |
| `weave_ampR` | 0.8 | — |
| `weave_lfo1Type` | 0 (Off) | Velocity expression is the performance dimension — no LFO needed |
| `weave_lfo2Type` | 0 (Off) | — |
| `weave_fx1Type` | 0 (Off) | Dry — the timbre arc is the demonstration |
| `weave_macroWeave` | 0.0 | WEAVE increases coupling |
| `weave_macroTension` | 0.0 | TENSION adds resonance to the filter peak |
| `weave_macroKnot` | 0.0 | — |
| `weave_macroSpace` | 0.0 | — |
| `weave_voiceMode` | 3 (Poly8) | — |
| `weave_pitchBendRange` | 2 | — |

**Sonic character:** At pianissimo velocity: dark, muffled, with a barely-audible resonance peak at 350 Hz. The topology is present but obscured. At mezzo-forte: filter opens to ~1350 Hz, resonance peak audible and prominent, topology clearly visible in the spectrum. At fortissimo: filter at 2350 Hz, the Trefoil coupling's spectral character fully exposed in the mid-range. The player's touch velocity is a direct control over how much of the knot they expose.

**Teaching use:** This is the WEAVE-VI teaching preset. Play scales at three velocity levels. Listen to the same pitch become three different instruments under velocity. Then explain: the velocity maps to 2000 Hz of filter offset. At low cutoff, 2000 Hz is everything.

---

## Phase R7: The Six Laws (Quick Reference)

| Law | Rule | Application |
|-----|------|-------------|
| WEAVE-I | BraidDepth below 0.65 = detuned zone. Above 0.7 = topology zone. Choose a side. | Never park braidDepth at 0.5 without intention. |
| WEAVE-II | Sine strands = mathematically pure coupling. Saw/Square/Triangle = approximate coupling. | Use Sine to hear the knot. Use Saw/Sq/Tri to add harmonic weight. |
| WEAVE-III | Torus P/Q is a geometry selector, not a tone control. Test (2,5) and (5,8) before finalizing any Torus preset. | Never default to (2,3) without trying at least two other P/Q pairs. |
| WEAVE-IV | KNOT macro 0.5 = topological chimera — a state that no mathematical knot theory describes. | The chimera midpoint is a destination, not a transition. |
| WEAVE-V | Solomon at strandTune=7.02: each MIDI note is two coupled rings at root and fifth. The chord pad topology. | For Solomon pads, always consider strandTune 7.02, 5.0, or 3.86. |
| WEAVE-VI | Velocity → +2000 Hz. Expressive at cutoff below 2000 Hz. Negligible above 6000 Hz. | Match cutoff range to desired velocity response before finalizing. |

---

## Sister Cadence's Notes

- The `Docs/guru-bin-retreats/` directory was empty before this chapter — this is the inaugural formal Guru Bin entry for ORBWEAVE.
- The `scripture/retreats/orbweave-retreat.md` (2026-03-20) contains the original seance retreat with 6 WEAVE verses and 9 awakening preset concepts. This chapter (2026-03-21) is the student-facing version with 10 fully specified presets and pedagogical structure.
- Three moods are underrepresented in the current ORBWEAVE library: Submerged (2 presets), Aether (fewer than 5), and Family (coupling presets). Flag for next expansion pass.
- The Solomon + strandTune chord architecture has no dedicated Transcendental tier content yet. The two-ring chord behavior, combined with cross-engine coupling, is the strongest candidate for a deep-dive Transcendental chapter.
- The KNOT macro as a topology morpher is the most unique single control in the entire fleet. Document it prominently in marketing copy and in the Field Guide post scheduled for this engine.
- The golden torus (5,8) and cinquefoil (2,5) P/Q configurations are the least used in the preset library and the most sonically distinctive. Consider a focused mini-preset pass (10 presets) that exhausts all meaningful P/Q combinations: (2,3), (2,5), (2,7), (3,4), (3,5), (3,7), (4,5), (4,7), (5,7), (5,8).
