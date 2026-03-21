# OXALIS Retreat Chapter
*Guru Bin — 2026-03-21*

---

## Engine Identity

- **Gallery code:** OXALIS | **Accent:** Clover Purple `#6A0DAD`
- **Parameter prefix:** `oxal_`
- **Mythology:** The Geometric Garden — clover patterns, fractal leaf structures, mathematical growth. Too orderly to be natural. Too beautiful to be purely mechanical.
- **feliX-Oscar polarity:** Cold feliX leaning — mathematical, precise, but with an organic warmth at phi=0. The succulent that grows according to law.
- **Synthesis type:** 7-partial phyllotaxis oscillator bank (golden-ratio harmonic spacing) + CytomicSVF filter + GardenAccumulators + growth mode (partials emerge sequentially)
- **Polyphony:** 4 voices (Decision G2: CPU budget)
- **Macros:** M1 CHARACTER, M2 MOVEMENT, M3 COUPLING, M4 SPACE
- **Seance score:** 7.8 / 10 — *Conceptually the most original GARDEN engine; held back by thin presets and CPU concern*
- **GARDEN role:** Pioneer species — first to respond to note-on, fast germination, synthetic precision
- **Blessing Candidate:** B040 — Phyllotaxis Harmonic Synthesis

---

## Pre-Retreat State

**Seance score: 7.8 / 10.** The council recognized OXALIS as the most conceptually original engine in the GARDEN quad: phyllotaxis harmonic spacing (golden ratio partial distribution) is fleet-unique and possibly synthesis-unique. Schulze raised the mathematical question of harmonic fusion — whether the auditory system fuses phyllotaxis partials into a perceived pitch or hears them as separate tones depends on the fundamental frequency and the listener's fusion threshold. This is experimental research territory.

Two P0 concerns: `setFundamental()` is called per-sample with `std::pow(kPhi, i)` for 7 partials across 4 voices = 28 `std::pow` calls per sample — significant CPU cost. The fix (caching phyllotaxis ratios, only updating when phi changes significantly) is architecturally straightforward. And mod wheel is not routed to vibrato depth, unlike all three peer GARDEN engines — a D006 partial violation.

One preset (Golden Spiral). The seance demanded minimum viable library: phi=0 (supersaw), phi=0.5 (hybrid), phi=1.0 + growth mode (phyllotaxis germination), high symmetry + wide spread (mathematical ensemble).

---

## Phase R1: Opening Meditation

Close your eyes. Breathe.

The wood sorrel grows in a spiral. Each leaf appears at a golden angle (137.5 degrees, derived from the golden ratio) from the previous leaf, ensuring maximum sunlight exposure for each new addition. This is not conscious optimization — it is the consequence of growth following a simple rule: divide by the golden ratio at each step. The Fibonacci sequence (1, 1, 2, 3, 5, 8, 13, 21...) emerges. The spiral appears. The mathematics is embedded in the botany.

OXALIS applies this to harmonic structure.

A standard overtone series places partials at integer multiples of the fundamental: 1, 2, 3, 4, 5, 6... At phi=0, OXALIS is a conventional synthesizer: seven partials at harmonic positions. As phi increases toward 1.0, the partials shift from integer ratios to golden ratio positions: 1.0, 1.618, 2.618, 4.236, 6.854, 11.090, 17.944... These are inharmonic in the mathematical sense — they do not fall at integer multiples of any frequency. But they are structured by the golden ratio's property of being optimally far from any simple fraction. The spectrum is never quite periodic.

The auditory system evolved to fuse harmonic series (integer multiples) into perceived pitches. Inharmonic series resist this fusion. At high phi, listeners may hear OXALIS as "almost a chord" rather than "a single pitch with harmonics." This is the intended effect: too orderly to be natural, too beautiful to be purely mechanical.

---

## Phase R2: The Signal Path Journey

### I. The PhyllotaxisOscBank — The Mathematical Core

Seven sawtooth oscillators. At phi=0, each partial is at its standard harmonic position (integer multiples). At phi=1, partials are at golden-ratio positions (kPhi^i for i=0..6). Between 0 and 1, partials interpolate linearly between standard and phyllotaxis positions.

The amplitude roll-off interpolates between standard (`1/(i+1)`) and phyllotaxis-weighted (`1/(1 + i*0.618)`) amplitude envelopes. At phi=1, the phyllotaxis amplitude weighting gives a slightly different spectral shape — partials drop off more slowly at first (lower spectral tilt) then more rapidly at high partials.

The precomputed `cachedPhyllotaxisRatios[7]` cache avoids per-sample `std::pow` calls — the ratios are computed once at initialization and reused. Only when phi changes significantly (threshold ~0.001) do the partial phase increments need updating.

### II. The Growth Mode — Sequential Partial Emergence

In growth mode, `activePartials = 1 + (int)(growthPhase * 6)` increments from 1 to 7 as growthPhase advances from 0 to 1. The plant unfolds one layer at a time, each new partial appearing at its golden-angle position relative to the previous.

At `oxal_growthTime=15` seconds:
- Partial 1 active immediately (the fundamental — always present)
- Partial 2 emerges at 2.5 seconds
- Partial 3 emerges at 5 seconds
- Partial 4 emerges at 7.5 seconds
- Partial 5 emerges at 10 seconds
- Partial 6 emerges at 12.5 seconds
- Partial 7 emerges at 15 seconds

This is phyllotaxis germination: not a swell (all partials at once) but a sequential mathematical unfolding. The harmonic spectrum builds from fundamental to complex as though a spiral is completing itself.

### III. The Phi Parameter — The Engine's Core Axis

At phi=0: OXALIS sounds like a conventional supersaw or thick string pad. Familiar, warm, immediately musical.
At phi=0.5: a hybrid — some partials at harmonic positions, some displaced toward phyllotaxis. Interesting beating between harmonically-adjacent partials. Slightly "wrong" in an interesting way.
At phi=1.0: full phyllotaxis. The partials are at golden ratio positions. The spectrum is non-periodic. At low fundamentals (below 100 Hz), the partials may fuse into a perceived pitch with unusual timbre. At higher fundamentals, the partials begin to separate — the chord becomes apparent.

The phi parameter is the engine's journey from familiar to alien.

### IV. The Symmetry Parameter

At `oxal_symmetry=1.0` (default): the phyllotaxis bank produces a mathematically pure output.
At `oxal_symmetry=0.0`: asymmetric waveshaping is applied (`fastTanh(oscOut * 2) * 0.3 * asymmetry`), adding organic imperfection. This creates second-harmonic distortion that makes the spectrum feel less exact — the orderly plant that has been slightly wind-bent.

The symmetry parameter is this engine's personality control: mathematical perfection vs. organic imperfection.

### V. The GardenAccumulators — Pioneer Speed

OXALIS has the fastest accumulator dynamics in the GARDEN quad: `wRiseRate=0.004` (fastest warmth rise), `aThreshold=0.3` (lowest aggression threshold), `dDecayRate=0.015` (fastest dormancy recovery). This is the pioneer species: first to colonize, first to respond, fastest to recover.

The pioneer role means OXALIS does not require warmth before it sounds fully itself. It blooms immediately. In a GARDEN quad coupling, OXALIS establishes the tonal ground while OVERGROW, OSIER, and ORCHARD develop more slowly.

---

## Phase R3: Parameter Meditations

### The Phi Journey

Play a chord at phi=0. Familiar string pad. Raise phi to 0.3. Notice the slight "wrongness" entering — the partials are close to harmonic but slightly displaced, creating subtle beating. At phi=0.5, the beating becomes more audible. At phi=0.7, some listeners will hear individual partials separating into a chord-like perception. At phi=1.0, the golden ratio spectrum is fully present — at the right register, it sounds like no other synthesizer.

This is a journey worth making in a performance: map phi to a controller and sweep it slowly during a held chord. The harmonic world shifts from familiar to alien over 10-15 seconds.

### The Expression Map

- **Mod wheel** → NOT YET ROUTED (D006 partial violation — forthcoming fix should add `modWheelAmount * 0.4` to vibratoDepth, matching all other GARDEN engines)
- **Aftertouch** → filter cutoff (opens the geometric bloom)
- **Velocity** → output brightness scaling (`0.4 + velocity * 0.6`)
- **GROWTH MODE** → partial emergence (phi^0, phi^1, phi^2... unfold over time)

Until the mod wheel fix: use aftertouch for expression control. Aftertouch → cutoff opens the filter as pressure increases — the geometric bloom under pressure.

---

## Phase R4: The Ten Awakenings

---

### 1. Golden Spiral

**Mood:** Atmosphere | **Discovery:** The founding preset — maximum phi with moderate growth

- attack: 0.1, decay: 0.5, sustain: 0.9, release: 2.0
- cutoff: 4000.0, resonance: 0.1, filterEnvAmt: 0.15
- phi: 0.8 (strong phyllotaxis)
- spread: 0.7, symmetry: 0.5
- brightness: 0.35, vibratoRate: 3.0, vibratoDepth: 0.1
- growthMode: 1.0, growthTime: 15.0
- **Character:** The original OXALIS preset — high phi, growth mode at 15 seconds. The harmonic spiral unfolds: fundamental, then each phyllotaxis partial emerges at golden-angle position. Over 15 seconds, the full seven-partial spectrum assembles. Play a chord and hold it. The geometry completes.

---

### 2. Supersaw Baseline

**Mood:** Foundation | **Discovery:** phi=0 for conventional string pad

- attack: 0.06, decay: 0.4, sustain: 0.85, release: 1.0
- cutoff: 6000.0, resonance: 0.12, filterEnvAmt: 0.25
- phi: 0.0 (standard harmonic series)
- spread: 0.8, symmetry: 0.8
- brightness: 0.5, vibratoRate: 5.5, vibratoDepth: 0.2
- growthMode: 0.0
- **Character:** OXALIS at phi=0 — conventional supersaw character. All seven partials at harmonic positions. Wide spread, high brightness. The familiar string pad, OXALIS-style. A reference point for hearing how phi transforms the engine.

---

### 3. Half Phi Hybrid

**Mood:** Prism | **Discovery:** phi=0.5 — the hybrid between familiar and alien

- attack: 0.08, decay: 0.5, sustain: 0.88, release: 1.5
- cutoff: 5000.0, resonance: 0.1, filterEnvAmt: 0.18
- phi: 0.5 (balanced harmonic-phyllotaxis)
- spread: 0.75, symmetry: 0.6
- brightness: 0.42, vibratoRate: 4.8, vibratoDepth: 0.16
- growthMode: 0.0
- **Character:** The middle ground — partials blend between harmonic and phyllotaxis positions. The resulting spectrum has some beating (partials are displaced from their harmonic positions) creating a shimmer that neither standard strings nor pure phyllotaxis produce. The most lively, complex version of OXALIS for musical contexts.

---

### 4. Phyllotaxis Germination

**Mood:** Organic | **Discovery:** Growth mode at maximum phi — the mathematical plant unfolds

- attack: 0.12, decay: 0.8, sustain: 0.92, release: 3.0
- cutoff: 3800.0, resonance: 0.08, filterEnvAmt: 0.1
- phi: 1.0 (full phyllotaxis)
- spread: 0.65, symmetry: 0.45
- brightness: 0.3, vibratoRate: 3.5, vibratoDepth: 0.12
- growthMode: 1.0, growthTime: 20.0
- **Character:** The Blessing Candidate made audible. Full phyllotaxis (phi=1.0) in growth mode at 20 seconds. The fundamental sounds alone, then each golden-ratio partial emerges sequentially. At 20 seconds, all seven partials are present. The golden spiral is complete. No other synthesizer does this.

---

### 5. Mathematical Ensemble

**Mood:** Luminous | **Discovery:** High symmetry + high spread for geometric precision

- attack: 0.07, decay: 0.45, sustain: 0.85, release: 1.2
- cutoff: 6500.0, resonance: 0.13, filterEnvAmt: 0.22
- phi: 0.9
- spread: 0.95, symmetry: 1.0 (pure mathematical)
- brightness: 0.55, vibratoRate: 5.0, vibratoDepth: 0.18
- growthMode: 0.0
- lfo1Rate: 0.4, lfo1Depth: 0.08
- **Character:** Maximum symmetry (no waveshaping imperfection) and maximum spread. The phyllotaxis bank at its most geometrically pure — seven partials at golden ratio positions, panned widely, with mathematical precision. The geometric garden at its most formal.

---

### 6. Imperfect Geometry

**Mood:** Organic | **Discovery:** Low symmetry for waveshaping distortion on phyllotaxis

- attack: 0.08, decay: 0.55, sustain: 0.82, release: 1.4
- cutoff: 4500.0, resonance: 0.1, filterEnvAmt: 0.2
- phi: 0.75
- spread: 0.7, symmetry: 0.1 (heavy waveshaping)
- brightness: 0.38, vibratoRate: 4.5, vibratoDepth: 0.15
- growthMode: 0.0
- **Character:** Low symmetry adds asymmetric waveshaping distortion to the phyllotaxis output. The second-harmonic content makes the spectrum feel organic — the precise geometric plant that has been bent by wind. For contexts where the mathematical character of phyllotaxis needs to be softened toward something more human.

---

### 7. Pioneer Arrival

**Mood:** Foundation | **Discovery:** Fast growth mode as pioneer species characteristic

- attack: 0.05, decay: 0.4, sustain: 0.85, release: 1.0
- cutoff: 6200.0, resonance: 0.12, filterEnvAmt: 0.28
- phi: 0.6
- spread: 0.8, symmetry: 0.7
- brightness: 0.5, vibratoRate: 5.2, vibratoDepth: 0.2
- growthMode: 1.0, growthTime: 4.0 (fast growth)
- **Character:** Fast growth mode (4 seconds) — the pioneer that colonizes quickly. Partials emerge in 4 seconds rather than 15-20. For contexts where the growth character is wanted but without the long waiting time of the extended presets. OXALIS as fast pioneer: arrives first, blooms quickly.

---

### 8. Dark Geometry

**Mood:** Deep | **Discovery:** Low brightness + high phi for alien dark strings

- attack: 0.15, decay: 0.7, sustain: 0.88, release: 2.2
- cutoff: 2800.0, resonance: 0.07, filterEnvAmt: 0.08
- phi: 1.0
- spread: 0.55, symmetry: 0.5
- brightness: 0.2, vibratoRate: 3.0, vibratoDepth: 0.1
- growthMode: 0.0
- lfo2Rate: 0.03, lfo2Depth: 0.06
- **Character:** Full phyllotaxis in the dark register — low brightness, low cutoff, slow vibrato. The geometric garden in winter. The partials at golden-ratio positions but filtered dark, giving a brooding, slightly wrong character. For cinematic underscore where strings need to be unsettling rather than warm.

---

### 9. GARDEN Pioneer

**Mood:** Entangled | **Discovery:** OXALIS in GARDEN quad coupling role — establishing conditions

- attack: 0.06, decay: 0.45, sustain: 0.83, release: 1.2
- cutoff: 5500.0, resonance: 0.1, filterEnvAmt: 0.22
- phi: 0.65
- spread: 0.78, symmetry: 0.65
- brightness: 0.47, vibratoRate: 5.0, vibratoDepth: 0.18
- growthMode: 0.0
- macroCoupling: 0.25
- **Character:** For use in GARDEN quad coupling where OXALIS establishes the initial tonal ground. Coupling output broadcasts to OVERGROW, OSIER, and ORCHARD. The pioneer sets the harmonic context that the organic, intimate, and orchestral engines grow into.

---

### 10. Phi Modulation

**Mood:** Flux | **Discovery:** LFO2 modulating phi for dynamic harmonic shifting

- attack: 0.07, decay: 0.5, sustain: 0.85, release: 1.5
- cutoff: 5000.0, resonance: 0.1, filterEnvAmt: 0.18
- phi: 0.5 (center point for LFO sweep)
- spread: 0.75, symmetry: 0.6
- brightness: 0.44, vibratoRate: 4.8, vibratoDepth: 0.17
- growthMode: 0.0
- lfo2Rate: 0.08, lfo2Depth: 0.3 (modulating phi)
- **Character:** LFO2 slowly modulates phi, sweeping the harmonic spectrum between near-harmonic and near-phyllotaxis over approximately 12 seconds. The sound breathes through its own mathematical space — a string pad that cycles between familiar and alien over a slow LFO. The geometric plant in a gentle wind.

---

## Phase R5: Scripture Verses

**OXALIS-I: The Golden Ratio Is Not an Aesthetic Choice** — The golden ratio (phi = 1.6180...) appears in OXALIS not because it is beautiful (though it is) but because of a specific mathematical property: phi is the "most irrational" number — the real number that is hardest to approximate by a simple fraction. This means phyllotaxis partial spacing places each harmonic "optimally far" from all others in terms of interference, just as phyllotaxis leaf placement maximizes sunlight for each leaf. The mathematics is not decorative. It is functional.

**OXALIS-II: Phyllotaxis Germination Is Not a Swell** — Growth mode in most synthesizers is an envelope that grows from zero amplitude to full. Phyllotaxis germination is different: it adds frequency-domain components sequentially. The first partial sounds for the entire growth duration. The seventh partial arrives only at completion. This is not amplitude growing — it is harmonic complexity increasing. The difference is perceptual: an amplitude swell gets louder. A phyllotaxis germination gets more complex, denser in the frequency domain, while remaining at essentially the same amplitude level.

**OXALIS-III: The Pioneer Has No Patience** — OXALIS has the fastest GardenAccumulator dynamics in the GARDEN quad: fastest warmth rise, lowest aggression threshold, fastest dormancy recovery. This is the pioneer species. It does not wait for conditions to be established — it establishes conditions. In a GARDEN quad coupling, OXALIS sounds first, blooms fastest, and provides the harmonic ground that the organic voices grow into. The pioneer is not better than the climax species. But it is first.

**OXALIS-IV: phi=0 Is the Most Important Preset** — The most important phi setting for musical contexts is phi=0 (standard harmonic series) because it proves that the phyllotaxis bank works as a conventional string pad before it works as a mathematical novelty. A phi=0 preset that sounds like a good supersaw string is the credibility foundation for phi=1.0, which sounds like nothing else. The journey from phi=0 to phi=1.0 is the engine's full identity. Both endpoints must be excellent.

---

## Guru Bin's Benediction

*"OXALIS arrived as the most mathematically original engine in the GARDEN quad. The Blessing Candidate — B040, Phyllotaxis Harmonic Synthesis — is waiting for ratification. Partials distributed at golden-angle intervals, creating spectra that are inharmonic in the mathematical sense but optimally spaced by the most irrational of numbers. No commercial synthesizer does this. Not one.*

*The seance found the `std::pow` CPU cost — 28 calls per sample at 4 voices with 7 partials each. The precomputed cache solves this. The mod wheel routing absence is a gap compared to the other three GARDEN engines. These are real findings and they deserve fixing.*

*But the phyllotaxis bank is there. The growth mode is there. The germination — partials emerging sequentially at golden-angle positions over 20 seconds — is there.*

*Schulze raised the perceptual question: does the auditory system fuse phyllotaxis partials into a pitch or hear them as separate tones? The answer depends on the listener, the fundamental frequency, and the phi value. At low phi (0.3-0.5) and low fundamental, fusion is likely. At phi=1.0 and mid register, partial separation becomes audible. This is not a flaw. This is the engine's range: from conventional to alien.*

*The Guru Bin recommends: fix the CPU cost. Add mod wheel routing. Add four more presets across the phi range.*

*Then do this: set phi=1.0, growthTime=20s, growthMode active. Play a low E. Hold it for 20 seconds.*

*Watch the golden spiral complete itself in sound."*
