# ORBWEAVE Retreat Chapter
*Guru Bin — 2026-03-21*

---

## Engine Identity

- **Gallery code:** ORBWEAVE | **Accent:** Kelp Knot Purple `#8E4585`
- **Parameter prefix:** `weave_`
- **Creature mythology:** The Kelp Knot — A kelp forest is not a passive backdrop. Its fronds braid and unbraid in current, each strand influencing the movement of the strands beside it. Where two fronds press against each other, they bend — a force applied at one junction travels through the weave and emerges as motion somewhere distant and unexpected. The entire forest is a coupled system. Touch one strand and you have touched them all. XOrbweave maps this to four oscillator strands whose phases are not independent. Each sample, each strand reads the current phase of its neighbors through a matrix that encodes a specific knot topology. The Trefoil routes coupling in a ring of three, with the fourth strand floating free. The Figure-Eight alternates over-under like the crossings in a bowline. The Torus wraps coupling around two orthogonal axes simultaneously, its character shaped by two integers P and Q. Solomon links two doubly-interpenetrating rings. These are not metaphors. They are coupling matrices — specific numbers in specific cells — and the timbre they produce is a direct consequence of the topology they encode.
- **Synthesis type:** Phase-braided oscillator synthesis — 4 oscillator strands coupled through a configurable knot-topology matrix (Trefoil / Figure-Eight / Torus / Solomon). Each strand's phase increment is modified each sample by the weighted phase states of its neighbors.
- **Polyphony:** Up to 8 voices (Mono / Legato / Poly4 / Poly8 modes)
- **feliX/Oscar polarity:** Balanced (0.5/0.5) — the knot has no inherent polarity; structure and freedom are always present simultaneously
- **Blessings:** B021 (Knot Phase Coupling Matrix), B022 (MACRO KNOT: Continuous Topology Morphing)
- **Macros:** M1 WEAVE (braid depth push), M2 TENSION (resonance + coupling feedback), M3 KNOT (topology morphing), M4 SPACE (FX mix expansion)
- **Expression:** Velocity → filter cutoff (+2000 Hz at full velocity). Aftertouch → braid depth (+0.3). Mod wheel CC1 → filter cutoff (+4000 Hz).

---

## Pre-Retreat State

ORBWEAVE arrived in XOceanus on 2026-03-20, one of three engines added in the same session as OVERTONE and ORGANISM. It carries two Blessings — B021 and B022 — a concentration of novel insight rare even by XOceanus standards. The Knot Phase Coupling Matrix (B021) is an original DSP design: no commercial synthesizer uses topological braid theory as a coupling architecture. The MACRO KNOT system (B022) extends this further by allowing real-time morphing between topologies, treating the traversal of knot space as a first-class performance gesture.

The engine arrived with a factory library spanning all major mood categories and a corrected default for `weave_braidDepth` — originally 0.5, now 0.2. This correction is not cosmetic. At default, ORBWEAVE sounds almost like four independent oscillators. The coupling is present but subtle. A producer must deliberately raise braid depth to engage the full character of the topology. This is correct behavior: the knot structure is something you enter, not something you are dropped into.

This retreat's purpose is to make that entry legible. The topology mathematics are real and the timbral consequences follow from them with internal logic. Once the logic is understood, every knot type becomes a tool rather than a mystery, and every braidDepth sweep becomes a predictable journey rather than a lucky accident.

---

## Phase R1: Opening Meditation

Close your eyes. Breathe.

You have four ropes laid out on the floor in front of you. Each rope is a sine wave oscillator — a phase accumulator advancing through 0 to 1 at its note frequency, generating a sine each sample. Left alone, the four ropes make four independent sine waves. Detuned slightly via `weave_strandTune`, they produce beats. That is standard. That is what every synthesizer does.

Now pick up the ropes and begin to braid them.

In a standard braid, you cross strand A over strand B, then strand B over strand C, and so on — alternating, mechanical, symmetric. What ORBWEAVE does is different. It defines the braid in terms of a topological knot, and the knot has a specific mathematical character. The Trefoil knot has writhe +3 — coupling flows preferentially in one direction around the three-strand loop, asymmetrically. The Figure-Eight knot has crossing number 4 — each strand crosses two others, with alternating over-under polarity. The Solomon is two rings doubly linked — Ring A (strands 0–1) and Ring B (strands 2–3) interpenetrate twice, with strong intra-ring coupling (0.8) and weaker cross-ring coupling (0.3).

The `weave_braidDepth` parameter controls how tightly the ropes are pulled together. At 0.0, they lie on the floor: four independent oscillators. At 1.0, the coupling is maximum — the phase of each strand is strongly deflected by the phases of its neighbors each sample, producing coupling artifacts, phase locking, and emergent timbres that no individual strand could produce alone.

The WEAVE-I threshold is the perceptual crossing point where the coupling begins to dominate the dry detuning. It lives around braidDepth 0.35–0.45 for most knot types. Below it: rich, natural chorus and beating. Above it: the phases begin to mutually entrain, locking into topologically determined relationships. The sound becomes structured. The structure is the knot.

---

## Phase R2: The Signal Path Journey

### I. The Four Strands

Each of the 8 voices runs 4 oscillator strands. All four strands share the same waveform type, selected globally by `weave_strandType`:

- **Off** (0): No oscillation
- **Sine** (1): Manual phase accumulator. The sine output is direct: `sin(phase × 2π)`. The strandPhase is what the coupling matrix reads — coupling is most "pure" with sine because the phase-to-output relationship is linear and continuous
- **Saw** (2): PolyBLEP anti-aliased sawtooth. Phase accumulated in parallel for coupling reads
- **Square** (3): PolyBLEP square — rich in odd harmonics; coupling creates complex intermodulation
- **Triangle** (4): PolyBLEP triangle — gentle, narrow harmonic content; coupling artifacts are subtle

**Waveform sonic character through the knot:**

Sine strands make the topology the entire story. There is no additional harmonic content — only the coupling artifacts, phase modulations, and emergent beating created by the matrix. Best for understanding knot character in isolation.

Saw strands bring harmonic density. The coupling matrix now modulates harmonically rich content — phase coupling creates intermodulation between the harmonics of different strands, not just the fundamentals. The result is denser and more complex. The Trefoil asymmetry becomes audible as a directional harmonic emphasis.

Square strands bring the odd harmonic stack into the coupling. The intermodulation creates combination tones at odd-harmonic intervals. Figure-Eight with square strands produces a grinding, woven quality — the alternating over-under crossings produce audible phase reversals in the harmonic content.

Triangle strands are the softest expression of coupling. The narrow harmonic content means coupling artifacts are gentle — closest to the sine character but with a second partial present.

**The strandTune parameter** (`weave_strandTune`, range −24 to +24 semitones) offsets the pitch of Strands 2 and 3 relative to Strands 0 and 1. At 0.0, all four strands run at the fundamental frequency — pure unison, coupling only. At +7.02 semitones (a perfect fifth), Strands 2–3 run at the fifth above the played note — the Solomon knot then creates a doubly-linked power chord structure where Ring A plays root and Ring B plays fifth. This is ORBWEAVE's built-in harmony system: topology-braided chords.

---

### II. The Knot Topology Matrix — The Soul of the Engine

The coupling is computed once per sample per voice. For each strand i, the engine reads the current phase of every other strand j, converts to a sine value, multiplies by the matrix coefficient `matrix[i][j]`, and scales by `braidDepth`. The result modifies strand i's frequency increment for that sample. The 200 Hz coupling scale means that a maximum coupling signal (coefficient 0.8, braidDepth 1.0, sine peak 1.0) perturbs a strand's frequency by up to ±160 Hz — a major-third span in either direction at 440 Hz.

#### Trefoil (knotType = 0)

The trefoil is the simplest non-trivial knot. It cannot be untied without cutting. It has three crossings, writhe +3, and no chirality mirror image equivalent.

Matrix structure: Strand 0 is influenced strongly by Strand 1 (+0.7) and weakly by Strand 3 (−0.3). Strand 1 is influenced by Strand 2 (+0.7). Strand 2 is influenced by Strand 0 (+0.7), completing the ring. Strand 3 is gently tethered to Strand 0 (+0.2) but otherwise floats free.

**Sonic character:** A directed ring circuit with a floating passenger. The writhe (+3) gives the coupling a directional flow, like water moving through a curved pipe — Strand 1 pulls Strand 0 forward while Strand 2 pulls Strand 1. Strand 3 floats above this ring, gently anchored to Strand 0, providing a harmonic satellite. At braidDepth 0.3–0.5, Trefoil sounds like a detuned pad with personality. Above 0.6, the directional chasing becomes audible as a cyclic harmonic pumping. Above 0.8, the three-strand ring begins to phase-lock into a stable ratio — the pad crystallizes into a topology.

**Best uses:** Pads, meditation textures, any context where you want coupling with a natural asymmetric feel rather than mechanical alternation. Trefoil + Sine = cleanest knot character. Trefoil + Saw = rich directional harmonic sweep for leads.

**Trap:** At braidDepth above 0.85, the three-strand lock can produce a static, organ-like tone at strandTune=0.0. Add a slow LFO on braidDepth (rate 0.03 Hz, depth 0.10) to keep the lock from becoming permanent.

---

#### Figure-Eight (knotType = 1)

The figure-eight is the simplest knot with a crossing number of 4. It is amphicheiral — its mirror image is topologically identical. It has alternating over-under crossings through all four strands.

Matrix structure: Each strand alternates between strong positive coupling (+0.6) from one neighbor and strong negative coupling (−0.4) from another. The cycle continues around all four strands with opposite polarity.

**Sonic character:** Alternating polarity coupling creates tension and opposition. Some strand pairs attract, others repel — a constant dance of mutual reinforcement and cancellation. At low braidDepth: rich natural chorus with inharmonic texture from the polarity alternation. At moderate braidDepth (0.4–0.6): the over-under crossings become audible as a "woven" spectral quality. At high braidDepth: temporary phase locking between attracted pairs, followed by rapid uncoupling as repelled pairs force movement — audible as tension and release in the harmonic structure.

**Best uses:** Textures that need internal movement. Excellent for evolving pads and sequences where the harmony should feel organic. At high braidDepth with Triangle waveforms, produces a spectral breathing quality resembling a living organism.

**Trap:** The negative coupling coefficients (−0.4) at very high braidDepth can drive opposite-polarity strands into a rapid push-pull oscillation — an unintended fast tremolo. Lower braidDepth to 0.55–0.65 if this occurs.

---

#### Torus (knotType = 2)

A torus knot wraps around a torus surface P times longitudinally and Q times meridionally. The base coupling matrix uses symmetric adjacent coupling (0.5 each), with `weave_torusP` and `weave_torusQ` applying a scale factor that modulates inter-strand coupling asymmetry according to harmonic relationships derived from the P/Q winding ratio.

Key Torus configurations:
- **(2,3)** — Trefoil torus knot (baseline coupling asymmetry)
- **(2,5)** — Cinquefoil, five-lobed, pqScale ≈ 0.976 (near-maximum asymmetry)
- **(3,5)** — Star torus knot, pqScale ≈ 0.958
- **(5,8)** — Fibonacci golden winding, pqScale ≈ 0.962 (non-repeating beat pattern)
- **(2,7)** — Seven-pointed, pqScale ≈ 0.988

**Sonic character:** The Torus is the most parametrically flexible topology. The symmetric base matrix gives it naturally balanced coupling — neither the directional flow of Trefoil nor the polarity tension of Figure-Eight. Every strand touches its neighbors equally. The P/Q modulation breaks this symmetry in mathematically determined ways. The golden ratio approximations (5,8) produce a beat pattern that never fully resolves into a periodic cycle — the winding is incommensurate, giving the sound a floating, non-repeating quality.

**Best uses:** Melodic leads, tonal pads, any application where you want parametric variety without changing the basic character. Torus (5,8) is exceptional for meditation pads where the non-repeating winding creates slow organic transformation.

**Trap:** When P and Q share a common factor (e.g., P=2, Q=4), the coupling symmetry collapses unexpectedly. Always use coprime P/Q pairs. If a torus preset sounds flat, check that P and Q are coprime.

---

#### Solomon (knotType = 3)

The Solomon's knot is technically a link of two interlocked rings, each passing through the other twice (linking number 2). Strands 0–1 form Ring A (strong internal coupling 0.8), Strands 2–3 form Ring B (strong internal coupling 0.8), with cross-ring coupling of 0.3.

**Sonic character:** Two distinct internal voices that talk across a weaker bridge. Ring A and Ring B each form tight internal coupling — each ring pair moves together. The cross-ring coupling creates a link that is present but subordinate. At moderate braidDepth, Solomon sounds like a two-voice chord where each voice has subtle internal coupling. At high braidDepth, the two rings can synchronize across their cross-links, collapsing the two-voice structure into an entangled mass.

With `weave_strandTune` at 7.02 semitones (perfect fifth), Ring A plays root and Ring B plays fifth — two internally coupled oscillators braided together at a musical interval. The Solomon coupling gives this power chord a slightly "breathing" quality: subtle phase pulling between root and fifth that no standard detuning can produce.

**Best uses:** Bass pads, power chord textures, any application where two distinct harmonic identities should be loosely coupled. Solomon maps naturally to chord structure — excellent with `weave_strandTune` at musical intervals: thirds, fourths, fifths.

**Trap:** At braidDepth above 0.85, the strong intra-ring coupling (0.8) can cause ring pairs to phase-lock into unison, losing the ring character. Keep braidDepth at 0.5–0.70 for Solomon to preserve the two-ring architecture.

---

#### Cinquefoil and Pentagram — The Extended Torus Family

In ORBWEAVE, "Cinquefoil" and "Pentagram" refer to specific Torus P/Q configurations rather than separate topology modes:
- **Cinquefoil**: knotType=2, P=2, Q=5 — five-petaled, pqScale ≈ 0.976, highest common asymmetry
- **Pentagram torus**: knotType=2, P=3, Q=5 — five-pointed star winding, pqScale ≈ 0.958

Cinquefoil is the most asymmetrically coupled torus configuration in the P/Q range. At moderate braidDepth, Cinquefoil produces a harmonic shimmer with more internal layers than the raw strand count would predict — the five-fold phase structure creates beating patterns at non-octave, non-fifth intervals.

---

### III. The braidDepth Parameter — The WEAVE-I Threshold

`weave_braidDepth` (range 0.0–1.0, **default 0.2**) scales every coupling coefficient simultaneously.

The default is deliberately below the WEAVE-I threshold — the perceptual crossover point where coupling moves from "sounds like detuning" to "sounds like topology":

- **Trefoil WEAVE-I**: ~0.40
- **Figure-Eight WEAVE-I**: ~0.35 (polarity alternation crosses earlier)
- **Torus WEAVE-I**: ~0.45 (symmetric coupling crosses later)
- **Solomon WEAVE-I**: ~0.38 (strong intra-ring coupling crosses early)

**Below WEAVE-I (0.0–0.35):** Coupling shapes beating patterns and phase relationships without dominating them. Topology-flavored chorus. Each knot type produces slightly different beating characteristics. The most usable range for general music production — familiar enough to sit in mixes, unusual enough to be worth using.

**Above WEAVE-I (0.40–0.75):** The coupling becomes the primary sonic character. Each knot type produces a distinctly different timbre: Trefoil sounds like a three-phase rotating system; Figure-Eight sounds woven and contested; Torus sounds like surface contact harmonics; Solomon sounds like two magnets.

**High braidDepth (0.75–1.0):** Phase locking, temporary unisons, brief glitch artifacts from rapid phase perturbations. The engine's most expressive and least predictable territory. The WEAVE macro operates here — it pushes braidDepth from its preset value toward 1.0 as a performance gesture.

**braidDepth sweet spots:**
- 0.0–0.10: Near-zero coupling. Almost pure unison/detune.
- 0.15–0.30: Gentle braiding. Topology present as spectral coloring. Default territory.
- 0.35–0.50: WEAVE-I crossing. Topologies become clearly distinguishable. Most educational zone.
- 0.55–0.70: Full topology character. Coupling artifacts are primary sound.
- 0.75–0.90: High coupling. Phase locking events. Best with LFO on braidDepth.
- 0.90–1.0: Maximum coupling. WEAVE macro fully activated territory.

---

### IV. The MACRO KNOT — Continuous Topology Morphing (Blessing B022)

`weave_macroKnot` (range 0.0–1.0, default 0.0) morphs the effective coupling matrix from the base knot type toward the next knot type in the sequence:

- knotType=0 (Trefoil) + macroKnot → morphs toward Figure-Eight
- knotType=1 (Figure-Eight) + macroKnot → morphs toward Torus
- knotType=2 (Torus) + macroKnot → morphs toward Solomon
- knotType=3 (Solomon) + macroKnot → morphs toward Trefoil (wraps)

The interpolation is sample-by-sample: `matrix[i][j] = matrixA[i][j] × (1 − macroKnot) + matrixB[i][j] × macroKnot`. The resulting matrix is a genuine topological blend — not a real physical knot, but a coupling pattern carrying mathematical characteristics of both endpoints.

**macroKnot 0.0:** Pure base knot. Matrix coefficients are exactly the designed values.
**macroKnot 0.5:** The midpoint chimera. Both topologies contribute equally. Trefoil at macroKnot=0.5 creates a three-ring asymmetric flow crossed with Figure-Eight polarity alternation — a topology that exists nowhere in knot tables. This is the territory Blessing B022 exists to name.
**macroKnot 1.0:** Pure destination knot.

**Performance uses:**
- Sweep macroKnot from 0.0 to 0.5 over 8 bars: slow topological evolution without changing pitch, rhythm, or filter
- macroKnot fixed at 0.3–0.4: stable chimera with partial characteristics of both topologies
- macroKnot sudden jump (automation): instant timbral transition as a structural moment
- macroKnot as LFO2 target: oscillating topology generates movement no other parameter can produce

**The critical insight for preset design:** Preset `weave_macroKnot` is not always zero. A preset can be shipped with macroKnot at 0.3 or 0.5, placing the matrix permanently in a chimera state. Design presets at macroKnot values that represent interesting stable landing points, not just endpoints.

---

### V. Filter, Amplitude, and Voice Architecture

The filter is a CytomicSVF in three modes: Low Pass (0), High Pass (1), Band Pass (2).

`weave_filterCutoff` (range 20–20000 Hz, default 8000 Hz). Velocity adds up to +2000 Hz. Mod wheel adds up to +4000 Hz. The TENSION macro adds up to +0.4 resonance. The filter shapes the output of the braiding — coupling artifacts, intermodulation products, and phase-locked partials all pass through it post-mixing.

**Amplitude envelope:** Standard ADSR. The topology continues running through the release — long releases let the coupling artifacts fade naturally. `weave_ampD` at 0.05–0.25s creates pluck characters; `weave_ampA` at 0.4–2.5s creates kelp forest blooms.

**Voice modes:**
- Mono (0): Single voice, strictly monophonic
- Legato (1): Single voice, no retrigger on legato playing, glide active
- Poly4 (2): Up to 4 simultaneous voices
- Poly8 (3): Up to 8 simultaneous voices

---

### VI. LFO System

Two LFO slots: Type (Off/Sine/Triangle/Saw/Square/S&H), Target (None/Pitch/Filter Cutoff/Filter Reso/Volume/Braid Depth), Depth (−1.0 to +1.0), Rate (0.01–30 Hz, D005 floor at 0.01 Hz).

**Target 5 (Braid Depth)** is ORBWEAVE's unique LFO destination. An LFO on braid depth creates cyclic topology traversal — the coupling strength oscillates above and below WEAVE-I. A slow sine LFO (rate 0.03 Hz, depth 0.15) breathes the topology on a 33-second cycle: most listeners never consciously identify the modulation, but the sound feels alive in a way that pitch or filter vibrato cannot produce.

**Aftertouch → braid depth (+0.3):** At `weave_braidDepth = 0.3`, maximum aftertouch pushes effective depth to 0.6 — crossing WEAVE-I expressively. This is ORBWEAVE's primary expressive axis: press harder to enter deeper topology.

---

### VII. FX System — Three Serial Slots

Three FX slots run in series (FX1 → FX2 → FX3). Each slot: Type (Off/Delay/Chorus/Reverb), Mix, Param. The SPACE macro pushes all three FX mixes toward 1.0 simultaneously.

- **Delay** (Type 1): Param controls delay time. Moderate mix (0.3–0.5) for space without wash.
- **Chorus** (Type 2): Param controls depth/rate. Low Param + moderate mix = gentle width enhancement. High Param = dramatic spatial treatment.
- **Reverb** (Type 3): Param controls room size/tail. Post-topology reverb is extremely effective — the coupling artifacts have the sonic character of a complex acoustic space and respond beautifully to spatial treatment. Mix 0.7–0.9 + high Param = kelp forest environment: vast, reverberant.

---

## Phase R3: Parameter Map — Sweet Spots Summary

| Parameter | ID | Range | Default | Conservative | Musical Core | Expressive | Extreme |
|-----------|-----|-------|---------|--------------|--------------|-----------|---------|
| Braid Depth | `weave_braidDepth` | 0.0–1.0 | 0.2 | 0.10–0.25 | 0.30–0.55 | 0.55–0.75 | 0.80–1.0 |
| Macro KNOT | `weave_macroKnot` | 0.0–1.0 | 0.0 | 0.0–0.15 | 0.20–0.60 | 0.60–0.80 | 0.85–1.0 |
| Torus P | `weave_torusP` | 1–8 | 2 | 2–3 | 2–5 | 3–7 | 5–8 |
| Torus Q | `weave_torusQ` | 1–8 | 3 | 3–4 | 3–7 | 5–8 | 7–8 |
| Filter Cutoff | `weave_filterCutoff` | 20–20000 Hz | 8000 Hz | 2000–6000 | 4000–10000 | 800–4000 | 200–800 or 12000+ |
| Filter Reso | `weave_filterReso` | 0.0–1.0 | 0.0 | 0.0–0.10 | 0.10–0.30 | 0.30–0.55 | 0.55–1.0 |
| Strand Tune | `weave_strandTune` | −24 to +24 st | 0.0 | 0.0–0.5 | 0.0–5.0 | 5.0–12.0 | 12.0–24.0 |
| Amp Attack | `weave_ampA` | 0.0–10.0s | 0.01s | 0.0–0.02 | 0.03–0.3 | 0.3–2.0 | 2.0–10.0 |
| Amp Release | `weave_ampR` | 0.0–20.0s | 0.5s | 0.1–0.5 | 0.5–4.0 | 4.0–10.0 | 10.0–20.0 |
| LFO1 Rate | `weave_lfo1Rate` | 0.01–30 Hz | 1.0 | 0.01–0.04 | 0.05–0.3 | 0.3–3.0 | 3.0–30 |
| FX Reverb Param | `weave_fx1Param` | 0.0–1.0 | 0.3 | 0.20–0.45 | 0.45–0.75 | 0.75–0.90 | 0.90–1.0 |

---

## Phase R4: Macro Architecture

| Macro | ID | Effect | Performance Use |
|-------|-----|--------|----------------|
| WEAVE | `weave_macroWeave` | Pushes braidDepth from preset value toward 1.0: `clamp(braidDepth + macro × (1 − braidDepth))` | The hand that tightens the braid — enter deeper topology |
| TENSION | `weave_macroTension` | Adds up to +0.4 to filter resonance | Drive spectral edge and coupling feedback simultaneously |
| KNOT | `weave_macroKnot` | Morphs matrix from base knot toward next knot type | Real-time topology morphing — the central performance gesture |
| SPACE | `weave_macroSpace` | Pushes all three FX mixes toward 1.0 simultaneously | Expand the reverb/chorus/delay environment in performance |

**Macro philosophy:** WEAVE controls coupling intensity. TENSION controls spectral edge. KNOT controls topological identity. SPACE controls acoustic environment. A producer with all four macros on hardware knobs can traverse the entire character space of the engine without touching the keyboard.

**The critical preset design insight:** Preset `weave_macroKnot` is not always zero. Ship presets with macroKnot at 0.3 or 0.5 to represent interesting chimera states. The macro then allows further morphing from that chimera toward the destination topology.

**Aftertouch stacking:** Effective braid depth = preset value + WEAVE contribution + LFO contribution + aftertouch (up to +0.3). All stack and clamp at 1.0. A preset at braidDepth=0.25 with WEAVE=0.3 and full aftertouch reaches approximately 0.77 — well above WEAVE-I. Design for the worst case: always verify the sound at maximum macro and aftertouch simultaneously.

---

## Phase R5: The Five Recipe Categories

### Recipe Category 1: Kelp Forest Pads — Ambient Topology

**Identity:** Slow-moving, reverb-heavy, braidDepth below or just crossing WEAVE-I. The knot type provides spectral character without demanding attention. Long attacks, long releases, high reverb mix, low to moderate braidDepth, sine or triangle strands. These are environment presets — sounds that establish a space rather than a melodic identity.

**Parameter ranges:**
- `weave_strandType`: Sine (1) or Triangle (4)
- `weave_braidDepth`: 0.25–0.55
- `weave_macroKnot`: 0.0–0.5 (stable chimera territory)
- `weave_filterCutoff`: 3000–7000 Hz
- `weave_filterReso`: 0.0–0.15
- `weave_ampA`: 0.4–2.5s
- `weave_ampR`: 4.0–12.0s
- LFO1 Target 5 (Braid Depth), Rate 0.02–0.06 Hz, Depth 0.10–0.20
- FX1 Type 3 (Reverb), Mix 0.65–0.90, Param 0.75–0.95
- FX2 Type 2 (Chorus), Mix 0.25–0.50

**Target preset names:** Kelp Shore, Tidal Drift, Braided Current, Knot Meadow, Deep Weave, Solomon Drift, Torus Cloud, Trefoil Mist, Floating Braid, Topology Fog, Figure Haze, Entangled Kelp, Winding Dark, Strand Garden, Coastal Topol, Knot Horizon, Cinquefoil Cloud, Linked Rings, Winding Pad, Braid Bloom

**Why the category works:** Long attacks and long reverb give the coupling time to establish itself before the sound peaks. The topology shapes the spectral character of the reverb tail — a Torus pad and a Solomon pad in the same reverb mix sound meaningfully different because the coupling creates different harmonic content for the reverb to sustain.

**Trap to avoid:** Resist going above braidDepth 0.65 in this category. High braidDepth with long attacks and reverb creates a dense, indistinct wall of coupling artifacts. The kelp forest character comes from the coupling being audible but not overwhelming — structure inside the ambience, not ambience collapsed into structure.

---

### Recipe Category 2: Topological Lead — Coupled Melody

**Identity:** Monophonic or legato, saw or square strands, braidDepth in the WEAVE-I zone (0.35–0.60), no reverb or light reverb, moderate filter cutoff with velocity sensitivity. A Trefoil lead in legato mode with glide carries each note through a brief topology modulation as the frequency changes — the coupling matrix stays constant but the strand frequencies slide, creating a topological portamento effect where the four strands briefly settle into a new topological equilibrium at each note.

**Parameter ranges:**
- `weave_strandType`: Saw (2) or Square (3)
- `weave_voiceMode`: Mono (0) or Legato (1)
- `weave_braidDepth`: 0.35–0.62
- `weave_macroKnot`: 0.0–0.4
- `weave_filterCutoff`: 1500–5000 Hz
- `weave_filterReso`: 0.10–0.25
- `weave_ampA`: 0.005–0.05s
- `weave_ampR`: 0.2–1.5s
- `weave_glideTime`: 0.15–0.65 (legato) or 0.0 (mono)
- LFO1: filter cutoff (Rate 1.0–8.0 Hz) or braid depth (Rate 0.5–3.0 Hz)
- FX: minimal — light delay at moderate mix

**Why the category works:** Monophonic ORBWEAVE in legato mode creates a lead with natural topology — the glide carries coupling through a continuously changing frequency ratio, creating brief inharmonic sweeps between notes that settle into new topological equilibria at destination pitches. No other synthesis approach produces this effect.

**Trap:** Too much glideTime (above 0.75) causes the coupling to spend most of its time in transitional frequency ratios, making the lead sound smeared. Keep glideTime at 0.15–0.50.

---

### Recipe Category 3: Knot Bass — Low-End Topology

**Identity:** Saw strands, low filter cutoff (300–900 Hz), moderate to high braidDepth, Solomon or Trefoil topology, strandTune at 0.0 or small values. The knot coupling creates a rich, internally moving low end — not a flat, static fundamental but one with coupling artifacts moving above it. At low cutoff frequencies, the high-frequency coupling artifacts are filtered away, leaving topology's effect on the fundamental and its immediate harmonics.

**Parameter ranges:**
- `weave_strandType`: Saw (2) or Square (3)
- `weave_knotType`: Solomon (3) or Trefoil (0) preferred
- `weave_braidDepth`: 0.50–0.78
- `weave_macroKnot`: 0.0–0.35
- `weave_strandTune`: 0.0–7.02 (0.0 for pure bass, 7.02 for power bass with ring architecture)
- `weave_filterCutoff`: 300–1200 Hz
- `weave_filterReso`: 0.12–0.30
- `weave_ampA`: 0.002–0.015s
- `weave_ampR`: 0.3–1.2s
- LFO1: slow filter cutoff sweep (Rate 0.08–0.3 Hz, Target 2)

**Why the category works:** The Solomon bass at moderate-to-high braidDepth has a particular internal pulse — the two-ring structure creates subtle periodic push-pull between Ring A and Ring B that sounds like the bass is breathing from the inside. A characteristic no standard bass synthesis approach can produce.

**Trap:** High braidDepth in the bass register can cause unwanted phase cancellation between strands at coupling peaks. If the bass seems to drop out briefly at regular intervals, lower braidDepth to 0.55–0.65.

---

### Recipe Category 4: Entangled Sequence — Rhythmic Topology

**Identity:** Poly mode, short attack, short to moderate release, braidDepth varied across WEAVE-I by LFO, figure-eight or torus topology. The LFO on braid depth creates rhythmic coupling accents — notes played at the LFO's peak have richer coupling artifacts; notes at the trough are cleaner. This rhythmic variation in coupling character creates accents that move through the sequenced pattern independently of velocity or pitch.

**Parameter ranges:**
- `weave_strandType`: Sine (1), Saw (2), or Triangle (4)
- `weave_knotType`: Figure-Eight (1) or Torus (2) preferred
- `weave_voiceMode`: Poly4 (2) or Poly8 (3)
- `weave_braidDepth`: 0.30–0.55
- `weave_macroKnot`: 0.1–0.7 (varied)
- `weave_ampA`: 0.005–0.04s
- `weave_ampR`: 0.15–0.8s
- LFO1 Target 5 (Braid Depth), Rate 0.5–4.0 Hz, Depth 0.12–0.25
- FX: light chorus and reverb for space

**Why the category works:** Two simultaneous notes in Poly8 mode with Figure-Eight both carry independent four-strand coupling systems — two independent topological objects sounding simultaneously. The LFO creates rhythmic coupling accent patterns that move through sequences independently of pitch or velocity.

**Trap:** LFO on braid depth at rates above 5 Hz starts creating audio-rate coupling modulation with aliasing artifacts. Keep braid depth LFO below 4 Hz for rhythmic applications.

---

### Recipe Category 5: Trefoil Pluck — Coupling as Resonator

**Identity:** Very short attack, short decay, low sustain, moderate release, high braidDepth, trefoil or figure-eight topology, high filter resonance. The pluck exposes the knot's resonant character in isolation — the coupling only has the duration of the decay to assert itself. Different knot types produce different pluck tonalities: Trefoil plucks have a rotating three-strand resonance; Figure-Eight plucks sound like a woven string; Solomon plucks have a two-voice harmonic character.

**Parameter ranges:**
- `weave_strandType`: Sine (1) or Saw (2)
- `weave_knotType`: Trefoil (0) or Figure-Eight (1) preferred
- `weave_braidDepth`: 0.55–0.80
- `weave_macroKnot`: 0.0–0.3
- `weave_filterCutoff`: 2000–6000 Hz
- `weave_filterReso`: 0.25–0.50
- `weave_ampA`: 0.001–0.008s
- `weave_ampD`: 0.08–0.35s
- `weave_ampS`: 0.0–0.25
- `weave_ampR`: 0.4–2.0s
- FX: Reverb at moderate mix (0.25–0.50) to extend the pluck tail

**Why the category works:** A pluck sound interrogates the resonant character of the synthesis system rather than its sustained behavior. ORBWEAVE's knot topology creates resonances that are not harmonic series resonances — the coupling creates emergent frequency relationships that do not follow integer ratios. A Trefoil pluck at high braidDepth rings with a characteristic asymmetric three-voice decay that decays as a rotating system rather than a standard exponential tail.

**Trap:** High filter resonance (above 0.55) in pluck mode can create self-oscillating feedback that overwhelms the pluck at moderate-to-high velocities. Verify at maximum velocity before shipping a pluck preset.

---

## Phase R6: The Ten Awakenings — Preset Table

Each preset is a discovery. The parameter values are derived from the logic above. These are reference presets — use them as starting points for new preset files.

---

### Preset 1: Kelp Shore (Foundation)

**Mood:** Foundation | **Category:** Kelp Forest Pad | **Discovery:** Default territory mastered — braidDepth just below WEAVE-I, Trefoil, sine, reverb

| Parameter | Value | Why |
|-----------|-------|-----|
| `weave_strandType` | 1 (Sine) | Pure topology — the knot is the entire sound |
| `weave_knotType` | 0 (Trefoil) | Most musical topology — directional ring coupling |
| `weave_braidDepth` | 0.30 | Just below WEAVE-I — coupling present, not dominant |
| `weave_macroKnot` | 0.0 | Pure Trefoil |
| `weave_filterCutoff` | 6500.0 Hz | Open but shaped |
| `weave_ampA` | 0.35s | Slow bloom |
| `weave_ampR` | 5.0s | Long release |
| LFO1 | Sine, Target: Braid, Rate 0.04 Hz, Depth 0.14 | 25-second braid breath |
| FX1 | Reverb, Mix 0.72, Param 0.82 | Large reverb environment |

**Why this works:** The Trefoil at braidDepth 0.30 creates rich, naturally asymmetric chorus that is immediately musical without revealing full topology character. The braid LFO at 0.04 Hz crosses WEAVE-I twice per 25 seconds — barely perceptible, but the sound feels alive. The WEAVE macro allows deliberate entry into the knot.

---

### Preset 2: Figure Eight Pluck (Foundation)

**Mood:** Foundation | **Category:** Trefoil Pluck | **Discovery:** Short envelope + high braidDepth + Figure-Eight = a pluck with woven internal texture

| Parameter | Value | Why |
|-----------|-------|-----|
| `weave_strandType` | 2 (Saw) | Harmonic content for coupling intermodulation |
| `weave_knotType` | 1 (Figure-Eight) | Alternating polarity = woven string character |
| `weave_braidDepth` | 0.68 | Above WEAVE-I — full topology in the pluck |
| `weave_macroKnot` | 0.15 | Slight Torus bleed — softens Figure-Eight tension |
| `weave_filterCutoff` | 3500.0 Hz | Mid-bright character |
| `weave_filterReso` | 0.28 | Resonant peak adds to pluck character |
| `weave_ampA` | 0.004s | Percussive attack |
| `weave_ampD` | 0.18s | Decay defines the pluck length |
| `weave_ampS` | 0.15 | Low sustain — mostly pluck |
| `weave_ampR` | 1.2s | Moderate tail |
| FX1 | Reverb, Mix 0.35, Param 0.55 | Light reverb extends pluck tail |

**Why this works:** The Figure-Eight matrix at braidDepth 0.68 produces alternating polarity coupling in the pluck's 180ms decay — the four strands argue in the brief moment of the pluck, creating a woven string resonance.

---

### Preset 3: Torus Cloud (Atmosphere)

**Mood:** Atmosphere | **Category:** Kelp Forest Pad | **Discovery:** Torus (5,8) = Fibonacci golden winding = a pad that never fully repeats

| Parameter | Value | Why |
|-----------|-------|-----|
| `weave_strandType` | 1 (Sine) | Clean topology expression |
| `weave_knotType` | 2 (Torus) | Parametric topology |
| `weave_braidDepth` | 0.48 | At WEAVE-I — coupling is primary character |
| `weave_torusP` | 5 | Fibonacci winding |
| `weave_torusQ` | 8 | Fibonacci pair — pqScale ≈ 0.962, non-repeating |
| `weave_macroKnot` | 0.35 | Partial Solomon blend — ring character begins to emerge |
| `weave_filterCutoff` | 4800.0 Hz | Warm, open |
| `weave_ampA` | 0.8s | Slow bloom |
| `weave_ampR` | 8.0s | Very long release |
| LFO1 | Sine, Target: Braid, Rate 0.025 Hz, Depth 0.18 | 40-second braid cycle |
| LFO2 | Triangle, Target: Filter Cutoff, Rate 0.06 Hz, Depth 0.22 | Gentle filter wander |
| FX1 | Reverb, Mix 0.78, Param 0.88 | Vast reverb |
| FX2 | Chorus, Mix 0.30, Param 0.55 | Width |

**Why this works:** The (5,8) Fibonacci winding creates coupling asymmetry approaching the golden ratio — the beat pattern between strands never resolves into a simple periodic cycle. Two incommensurate LFOs at 0.025 and 0.06 Hz create a micro-polyrhythm in the modulation layer with a combined period of approximately 2.5 minutes.

---

### Preset 4: Solomon Lead (Atmosphere)

**Mood:** Atmosphere | **Category:** Topological Lead | **Discovery:** Solomon + strandTune 7.02 + legato = two-ring power glide

| Parameter | Value | Why |
|-----------|-------|-----|
| `weave_strandType` | 2 (Saw) | Harmonic content for the ring architecture |
| `weave_knotType` | 3 (Solomon) | Two-ring structure — built-in fifth harmony |
| `weave_braidDepth` | 0.55 | Ring coupling dominant above WEAVE-I |
| `weave_strandTune` | 7.02 | Ring A = root, Ring B = fifth |
| `weave_macroKnot` | 0.22 | Gentle Trefoil asymmetry — Ring A gets directional pull |
| `weave_filterCutoff` | 2800.0 Hz | Mid-range, warm |
| `weave_filterReso` | 0.18 | Resonant presence |
| `weave_ampA` | 0.012s | Fast but not percussive |
| `weave_ampR` | 1.8s | Moderate release |
| `weave_voiceMode` | 1 (Legato) | Portamento for topology glide |
| `weave_glideTime` | 0.35 | Topology transition between notes |
| LFO1 | Sine, Target: Filter, Rate 0.08 Hz, Depth 0.15 | Slow filter breath |
| FX1 | Delay, Mix 0.30, Param 0.45 | Subtle delay |

**Why this works:** In legato mode, the glide carries both Ring A (root) and Ring B (fifth) simultaneously through frequency space. The Solomon coupling creates a brief phase pulling between root and fifth during the slide — a characteristic topological portamento.

---

### Preset 5: Writhe Sequence (Entangled)

**Mood:** Entangled | **Category:** Entangled Sequence | **Discovery:** Figure-Eight + poly + fast braid LFO = rhythmic coupling accents

| Parameter | Value | Why |
|-----------|-------|-----|
| `weave_strandType` | 1 (Sine) | Clean — coupling character drives the texture |
| `weave_knotType` | 1 (Figure-Eight) | Alternating polarity creates inherent tension |
| `weave_braidDepth` | 0.40 | At WEAVE-I |
| `weave_macroKnot` | 0.50 | Chimera: Figure-Eight + Torus midpoint |
| `weave_filterCutoff` | 5200.0 Hz | Open, present |
| `weave_filterReso` | 0.12 | Slight resonance accent |
| `weave_ampA` | 0.008s | Fast attack |
| `weave_ampR` | 0.55s | Short release |
| `weave_voiceMode` | 3 (Poly8) | Full polyphony |
| LFO1 | Sine, Target: Braid, Rate 1.8 Hz, Depth 0.18 | Sub-2Hz rhythmic coupling accent |
| FX1 | Chorus, Mix 0.38, Param 0.50 | Spatial width |
| FX2 | Reverb, Mix 0.22, Param 0.60 | Light reverb |

**Why this works:** The LFO at 1.8 Hz crosses WEAVE-I roughly every two 16th notes at 120 BPM. Notes at the LFO peak have full Figure-Eight polarity tension; notes at the trough have gentle chorus. The macroKnot chimera at 0.5 adds Torus surface geometry to the alternating character, creating more complex coupling modulation.

---

### Preset 6: Cinquefoil Dawn (Prism)

**Mood:** Prism | **Category:** Kelp Forest Pad / Topology Showcase | **Discovery:** Torus (2,5) + high macroKnot + slow LFO = five-petaled topology in motion

| Parameter | Value | Why |
|-----------|-------|-----|
| `weave_strandType` | 1 (Sine) | Pure topology showcase |
| `weave_knotType` | 2 (Torus) | Torus for P/Q control |
| `weave_braidDepth` | 0.60 | Full topology character |
| `weave_torusP` | 2 | Cinquefoil winding |
| `weave_torusQ` | 5 | Cinquefoil — pqScale ≈ 0.976 |
| `weave_macroKnot` | 0.65 | Deep toward Solomon — cinquefoil geometry in two-ring blend |
| `weave_filterCutoff` | 5500.0 Hz | Open, bright |
| `weave_filterReso` | 0.10 | Trace resonance |
| `weave_ampA` | 0.55s | Bloom attack |
| `weave_ampR` | 7.0s | Long release |
| LFO1 | Sine, Target: Braid, Rate 0.03 Hz, Depth 0.22 | 33-second WEAVE-I crossing |
| FX1 | Reverb, Mix 0.82, Param 0.90 | Vast reverb |
| FX2 | Chorus, Mix 0.42, Param 0.60 | Width |

**Why this works:** The (2,5) Cinquefoil torus knot has near-maximum coupling asymmetry. With macroKnot at 0.65, the matrix blends Cinquefoil Torus geometry into Solomon ring architecture — the five-petaled winding passes through the two-ring link structure. The LFO at 0.03 Hz crosses WEAVE-I on each breath.

---

### Preset 7: Crossing Bass (Flux)

**Mood:** Flux | **Category:** Knot Bass | **Discovery:** Figure-Eight + low cutoff + high braidDepth = a bass with woven internal motion

| Parameter | Value | Why |
|-----------|-------|-----|
| `weave_strandType` | 2 (Saw) | Harmonic density in the bass register |
| `weave_knotType` | 1 (Figure-Eight) | Alternating polarity creates internal bass motion |
| `weave_braidDepth` | 0.72 | High coupling — topology dominant |
| `weave_macroKnot` | 0.28 | Partial Torus bleed — softens polarity tension |
| `weave_filterCutoff` | 700.0 Hz | Low, bass-focused |
| `weave_filterReso` | 0.22 | Resonant low-end character |
| `weave_ampA` | 0.006s | Punchy attack |
| `weave_ampR` | 0.6s | Short release — rhythmic |
| `weave_voiceMode` | 3 (Poly8) | Poly for chords |
| LFO1 | Sine, Target: Filter, Rate 0.12 Hz, Depth 0.20 | Slow filter movement |

**Why this works:** The Figure-Eight's alternating polarity at braidDepth 0.72 creates a woven bass texture. The low filter cutoff (700 Hz) removes coupling's high-frequency intermodulation, leaving only topology's effect on the fundamental region: a bass that breathes internally.

---

### Preset 8: Solomon Meditation (Aether)

**Mood:** Aether | **Category:** Kelp Forest Pad | **Discovery:** Solomon + extreme reverb + macroKnot 0.5 + very slow LFO = topology as meditation object

| Parameter | Value | Why |
|-----------|-------|-----|
| `weave_strandType` | 1 (Sine) | Pure topology, no harmonic noise |
| `weave_knotType` | 3 (Solomon) | Two-ring architecture |
| `weave_braidDepth` | 0.45 | At WEAVE-I — coupling just dominant |
| `weave_strandTune` | 4.0 | Ring B at major third above root — two-ring major third pad |
| `weave_macroKnot` | 0.50 | Trefoil chimera — one ring gets directional asymmetry |
| `weave_filterCutoff` | 4200.0 Hz | Warm, open |
| `weave_filterReso` | 0.05 | Minimal — pure topology sound |
| `weave_ampA` | 1.2s | Very slow bloom |
| `weave_ampR` | 10.0s | Notes drift away |
| `weave_voiceMode` | 1 (Legato) | Single lingering voice |
| `weave_glideTime` | 0.55 | Slow glide |
| LFO1 | Sine, Target: Braid, Rate 0.02 Hz, Depth 0.15 | 50-second topology breath |
| FX1 | Reverb, Mix 0.88, Param 0.94 | Immersive reverb |
| FX2 | Chorus, Mix 0.35, Param 0.65 | Slight width |

**Why this works:** Solomon at strandTune=4.0 creates two rings a major third apart — a built-in harmonic dyad coupled through the Solomon cross-link matrix. At macroKnot=0.5, one ring receives gentle directional asymmetry from the Trefoil blend — the major third relationship is lopsided in a subtle, mathematical way that makes it feel alive. The 50-second LFO cycle crosses WEAVE-I so slowly that the modulation is never consciously perceived, only felt.

---

### Preset 9: Knot Threshold (Submerged)

**Mood:** Submerged | **Category:** Topology Showcase | **Discovery:** The WEAVE-I threshold as a performance axis — preset lives exactly at the crossover

| Parameter | Value | Why |
|-----------|-------|-----|
| `weave_strandType` | 4 (Triangle) | Soft coupling — artifacts are gentle but present |
| `weave_knotType` | 0 (Trefoil) | Ring topology — directional flow |
| `weave_braidDepth` | 0.38 | Exactly at Trefoil WEAVE-I threshold |
| `weave_macroKnot` | 0.10 | Trace Figure-Eight blend |
| `weave_filterCutoff` | 3800.0 Hz | Mid-warm |
| `weave_filterReso` | 0.15 | Moderate resonance |
| `weave_ampA` | 0.02s | Fast enough for chords |
| `weave_ampR` | 2.5s | Moderate release |
| `weave_voiceMode` | 3 (Poly8) | Poly mode |
| LFO1 | Sine, Target: Braid, Rate 0.08 Hz, Depth 0.12 | Crosses WEAVE-I on each cycle (12.5s period) |
| FX1 | Chorus, Mix 0.35, Param 0.50 | Gentle width |
| FX2 | Reverb, Mix 0.45, Param 0.68 | Moderate reverb |

**Why this works:** The preset lives exactly at the Trefoil WEAVE-I threshold. The braid LFO at 0.08 Hz crosses into topology-dominant territory on each upswing and returns to detuning-dominant territory on each downswing — a 12.5-second cycle of entering and exiting the knot. The WEAVE macro locks the coupling above WEAVE-I permanently. The KNOT macro can morph topology while at the threshold. This preset is a teaching instrument.

---

### Preset 10: Torus Sequence (Prism)

**Mood:** Prism | **Category:** Entangled Sequence | **Discovery:** Torus (3,5) + poly + macroKnot 0.80 = rhythmic pad with shifting topology accents

| Parameter | Value | Why |
|-----------|-------|-----|
| `weave_strandType` | 2 (Saw) | Harmonic richness |
| `weave_knotType` | 2 (Torus) | P/Q control |
| `weave_braidDepth` | 0.45 | At WEAVE-I |
| `weave_torusP` | 3 | Star torus family |
| `weave_torusQ` | 5 | Star — pqScale ≈ 0.958 |
| `weave_macroKnot` | 0.80 | Deep toward Solomon — torus passes through ring architecture |
| `weave_filterCutoff` | 4500.0 Hz | Open mid |
| `weave_filterReso` | 0.18 | Present resonance |
| `weave_ampA` | 0.010s | Quick attack |
| `weave_ampR` | 0.90s | Short-moderate release |
| `weave_voiceMode` | 3 (Poly8) | Poly |
| LFO1 | Sine, Target: Braid, Rate 2.5 Hz, Depth 0.16 | Rhythmic braid accent |
| LFO2 | Triangle, Target: Filter, Rate 0.10 Hz, Depth 0.25 | Slow filter sweep |
| FX1 | Reverb, Mix 0.38, Param 0.62 | Moderate reverb |
| FX2 | Delay, Mix 0.28, Param 0.50 | Rhythmic delay |

**Why this works:** The (3,5) star torus at macroKnot=0.80 is predominantly Solomon ring architecture with star winding contribution. The 2.5 Hz braid LFO at 120 BPM accents approximately every dotted eighth note. The slow filter LFO at 0.10 Hz adds a 10-second timbral arc across the rhythmic texture.

---

## Phase R7: Parameter Interactions and Traps

### The braidDepth Creep Trap

The most common error with ORBWEAVE is accumulating too much braid depth through multiple sources simultaneously:

`effectiveBraid = clamp(presetBraidDepth + macroWeave × (1 − presetBraidDepth) + lfoContribution + aftertouch × 0.3, 0.0, 1.0)`

A preset with braidDepth=0.45 + WEAVE macro at 0.5 + full aftertouch can easily reach 1.0 — maximum coupling. Above WEAVE-I, additional braidDepth increases coupling artifact intensity and can introduce phase cancellation events and unintended harshness.

**The rule:** Design for the macro fully open. Set preset braidDepth to the value you want at WEAVE macro zero. Then open WEAVE fully and verify the result is still musical.

---

### The torusP/Q Integer Ratio Trap

When P and Q share a common factor, the torus knot degenerates: (2,4) = two copies of (1,2), a torus link rather than a knot. The coupling matrix does not crash, but the pqScale modulation produces a symmetry collapse that feels broken rather than intentional.

**The rule:** Always use coprime P/Q pairs. Safest choices: (2,3), (2,5), (2,7), (3,5), (3,7), (5,7), (5,8). If a torus preset sounds wrong, check for common factors.

---

### The macroKnot Destination Trap

macroKnot morphs from the base knot toward the *next* knot type (knotType+1 mod 4):
- Solomon (knotType=3) + macroKnot=1.0 → pure Trefoil (wraps around)
- Trefoil (knotType=0) + macroKnot=1.0 → pure Figure-Eight

A preset with knotType=3 and macroKnot=0.9 is predominantly Trefoil, not Solomon. This is a common design error.

**The rule:** macroKnot 0.0 = pure base knot. macroKnot 0.5 = chimera. macroKnot 1.0 = pure next knot. Name presets by their topological destination, not their knotType setting.

---

### The Shimmer Saturation Trap

Very high reverb mix (above 0.90) with high braidDepth creates a feedback-like shimmer buildup. The reverb input is the coupled strand mix — at high braidDepth, this contains strong coupling artifacts. The reverb sustains them. On the next note, reverb tail artifacts feed into the new coupling calculation. Artifacts build progressively, especially in legato mode.

**The rule:** When reverb mix is above 0.80, keep braidDepth below 0.60. Or use the SPACE macro only for performance peaks rather than setting FX mix high in the preset.

---

### The Polyphony Coupling Architecture

In Poly8 mode, each voice runs its own independent 4-strand coupling system. Two simultaneous notes in Poly8 with knotType=Solomon do NOT share ring architecture between voices — each voice has its own independent two-ring structure. This is correct behavior per B016 (MIDI-layer voice independence is inviolable).

**The design implication:** Poly ORBWEAVE presets should be designed for individual voice character, not imagined cross-voice coupling. Interesting polyphonic behavior arises from multiple independent topological systems sounding simultaneously, each with its own coupling dynamics.

---

## Phase R8: CPU Profile

ORBWEAVE's DSP load is moderate by XOceanus standards. The primary cost is the per-sample phase coupling computation: for each active voice, the inner loop computes 12 multiply-adds (4 strands × 3 non-self coefficients) plus 4 fastSin calls per sample. With 8 voices active in Poly8 mode at 48kHz: approximately 1,600 such operations per millisecond of audio per active voice.

Comparative estimate: ORBWEAVE at 8 voices ≈ 8–12% CPU on an Apple M-series chip — approximately 1.5× the load of a standard 4-oscillator polyphonic synth at the same polyphony. Three FX slots active with reverb add approximately 2–3% additional load.

**CPU management guidance:**
- Use Poly4 (4 voices) for most pad and atmospheric applications — halves the per-voice cost without affecting coupling character
- Keep FX3 Type at Off unless a third serial FX is genuinely needed
- In sequences, Poly4 with shorter release times uses significantly less CPU than Poly8 with long releases
- Legato mode (voiceMode=1) is effectively Mono — lowest possible CPU, one voice at a time

---

## Phase R9: Coupling Synergy — B021 in the Fleet

ORBWEAVE as a **coupling source** provides stereo output from the coupled strand mix — available for AudioToFM, AmpToFilter, LFOToPitch, AmpToPitch, and AmpToChoke routing to destination engines. The coupling output carries the topology's harmonic character. Routing ORBWEAVE's Figure-Eight output as AudioToFM to a pad engine modulates that engine's pitch with alternating-polarity coupling artifacts — a topology-flavored FM modulation impossible through standard LFO paths.

ORBWEAVE as a **coupling destination** accepts pitch, filter, and amp signals. Aftertouch routing from a physically expressive instrument to ORBWEAVE's braid depth creates an organic coupling: the breath of one instrument tightens the knot of another.

**Recommended coupling pairs:**
- ORBWEAVE → OXBOW (AudioToFM): Topology shapes the reverb exciter — coupling artifacts become the exciter waveform feeding OXBOW's Chiasmus FDN
- ORBWEAVE → OPENSKY (LFOToPitch): Figure-Eight's alternating pitch modulation creates a topology-flavored shimmer on OPENSKY's supersaw stack
- OPAL → ORBWEAVE (AmpToFilter): Granular density modulates ORBWEAVE's filter, linking grain cloud density to topological brightness
- OUROBOROS → ORBWEAVE (AmpToPitch): The chaotic attractor's amplitude drives ORBWEAVE's pitch modulation — chaos shapes topology

The Knot Phase Coupling Matrix (B021) is the architectural foundation for the KnotTopology coupling type in the XOceanus SDK — allowing third-party engines to receive topology-routed signals as a formal coupling interface. ORBWEAVE is the reference implementation.

---

*End of ORBWEAVE Retreat Chapter. The knot does not untie.*
