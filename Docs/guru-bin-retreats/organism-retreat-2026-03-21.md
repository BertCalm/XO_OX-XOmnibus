# ORGANISM Retreat — Guru Bin Deep Meditation
*2026-03-21 | Engine ID: Organism | Accent: Emergence Lime `#C6E377`*
*Creature: Coral Colony | Mechanism: 1D Elementary Cellular Automaton*
*Tier: Awakening — 10 gold presets*

---

## Arrival

The Coral Colony is the only XOmnibus engine that does not synthesize sound. It computes it.

Every other engine in the fleet responds to human gestures: raise a knob, change a timbre. ORGANISM does something different. You specify a rule — a single 8-bit number — and an initial condition — a 16-bit seed. Then you release the note and let mathematics do the rest. The colony evolves according to universal laws that have nothing to do with music. From those laws, music emerges. You are not a sound designer here. You are a biologist who has learned to set traps.

The Guru Bin retreat on 2026-03-20 catalogued the library's gaps. This retreat goes deeper: into the mathematics behind each curated rule, into the specific interactions between rule character and oscillator waveform, into the structural role of scope and step rate as a two-axis synthesis character surface. The questions the 2026-03-21 retreat answers are:

1. Which Wolfram rules produce the most musical patterns, and why?
2. How does rule selection interact with oscillator waveform choice?
3. What cell count and history depth create interesting evolution without chaos?
4. How does the state history visualization drive sound?
5. What are the sweet spots for each of the 4 macros?

Ten Awakening presets emerge from these answers. Each one demonstrates something specific. None of them is a decoration.

---

## The Meditation: Five Questions

---

### Question I — Which Wolfram Rules Are Most Musical?

The eight curated rules in `kCuratedRules[8] = { 30, 90, 110, 184, 150, 18, 54, 22 }` are not a random sample. Wolfram's taxonomy divides rules into four behavioral classes, and these eight rules span all of them. Understanding the class of each rule is the foundation of musical ORGANISM design.

**Rule 110 — Turing-Complete, Class IV**
Rule 110 is the most famous cellular automaton aside from Conway's Game of Life. Wolfram and Cook proved it Turing-complete in 2004: given the right initial condition, Rule 110 can simulate any computation. Its patterns are neither purely periodic nor purely random — they produce persistent structures ("gliders") that travel, collide, and generate new structures from their collisions. In ORGANISM's 16-cell circular world, Turing completeness is not practically relevant, but the Class IV behavior manifests: the filter, pitch, and envelope outputs never fully settle. There is always a perturbation somewhere. This is the engine's default and most accessible entry point — perpetual movement, never chaos.

*Most musical for:* Leads, generative sequences, anything where you want the engine to keep surprising you without collapsing into noise.

**Rule 90 — Sierpinski Triangle, Class III**
Rule 90 is the additive rule: the new cell equals the XOR of its two neighbors (ignoring center). Starting from a single live cell, Rule 90 generates the Sierpinski triangle — a fractal structure with exact self-similarity at every scale. In a circular 16-cell array, the Sierpinski pattern wraps and produces a characteristic alternating-density rhythm: regions of high cell activity separated by structured silence. The filter and envelope outputs driven by cells 0–3 and 4–7 oscillate with more regularity than Rule 110 — there is a fractal periodicity that the ear can track.

*Most musical for:* Rhythmic arpeggiation effects, resonant filter sweeps (the regular density peaks drive Q-ringing at predictable rhythmic intervals), percussive generative patterns.

**Rule 30 — Pseudorandom, Class III**
Wolfram uses Rule 30 as the core of Mathematica's `Random[]` function. Starting from a single live cell, it produces output that passes all standard statistical tests for randomness. In ORGANISM, Rule 30 with a deterministic seed produces deterministic pseudo-noise — the filter, pitch, and envelope cells change every step, but the changes have no discernible pattern. This sounds like continuous organic flutter: nothing repeats, nothing stabilizes, but it never sounds truly chaotic because the maximum pitch range is only ±6 semitones. Rule 30 is ORGANISM in "natural noise" mode.

*Most musical for:* Ambient textures, noise-colored pads, situations where the automaton should add organic irregularity rather than discernible patterns.

**Rule 184 — Traffic Flow, Class II**
Rule 184 is the traffic flow rule: it models particles (live cells) moving rightward along the tape, blocking at collisions. Sparse initial seeds produce particles that drift rightward and pile up at slower particles ahead. Dense initial seeds produce traffic jams — regions of immobility flanked by movement. In ORGANISM's circular 16-cell world, particles cycle continuously. The filter cells (0–3) trace the passage of density waves: long stable periods punctuated by brief activity bursts as a cluster passes through. This creates a rhythmic gating character that sounds like a very slow tremolo being interrupted by occasional rhythmic events.

*Most musical for:* Bass textures with embedded rhythmic density variation, situations where you want long stable periods with irregular interruptions.

**Rule 150 — Symmetric XOR, Class III**
Rule 150 is a symmetric generalization of Rule 90: new cell = XOR of left, center, and right. This produces symmetric patterns from asymmetric seeds — the tape develops bilateral symmetry that collapses and re-emerges. The density outputs tend toward regular 50% population averages with a characteristic "flip-flopping" between symmetric and asymmetric states. More regular than Rule 30, less structured than Rule 90.

*Most musical for:* Mid-range textures that oscillate between order and disorder, background modulation layers.

**Rule 18 — Isolated Gliders, Class III**
Rule 18 produces widely separated live cells from most initial seeds — it is a "sparsifying" rule. Dense initial states rapidly collapse toward sparse, isolated live cells that advance at fixed speeds. In ORGANISM, this means the filter, envelope, and pitch outputs tend toward 0 (dark, quiet, unmodulated) with brief periodic peaks. With a fixed period seed like seed=42, the output is nearly periodic — resembling a slow LFO with occasional double pulses.

*Most musical for:* Presets where the automaton provides rhythmic punctuation rather than continuous modulation — sharp periodic events against long silence. Strong sub-oscillator character emerges when cells are mostly silent with brief dense flashes.

**Rule 54 — Complex Class IV**
Rule 54 is another Class IV rule — one of only a handful confirmed complex. It produces long-transient behavior: initial conditions take many generations to settle (or never settle) into recognizable patterns. In ORGANISM's 16 cells, the transient period of a note-on can last many seconds before a quasi-periodic pattern develops. This makes Rule 54 an excellent rule for note-length-dependent evolution: a short note sounds chaotic; a sustained note eventually develops structure. The colony "matures."

*Most musical for:* Sustained pads and evolving textures where patience is rewarded. ORGANISM as a time-based narrative instrument.

**Rule 22 — Complex Class III**
Rule 22 produces nearly random-appearing outputs from sparse seeds but is notably denser than Rule 30 — average population sits around 50% rather than asymptoting to sparse. This gives ORGANISM more consistently "occupied" filter and envelope outputs, producing a sustained, busy texture. The density is high enough that the smoothing (scope averaging) produces a relatively stable base modulation with subtle variation.

*Most musical for:* Drone textures where the automaton adds subtle modulation to a continuous tone. The busier cellular activity keeps the filter and envelope outputs near their midpoints, producing a warm, mildly evolving pad character.

---

### Question II — Rule Selection and Oscillator Waveform Interaction

The cellular automaton modulates filter cutoff, amplitude rate, pitch offset, and reverb amount. The oscillator provides the raw harmonic material that these modulations shape. Different waveforms respond to CA modulation differently because they have different harmonic structures.

**Saw oscillator (default) + high-complexity rules (30, 110, 54):**
The sawtooth contains all harmonics (partials 1, 2, 3, 4...) in a 1/n amplitude envelope. When the filter cutoff sweeps (driven by cells 0–3), all these harmonics participate — the sweep sounds full-spectrum and rich. Complex rules that produce wide filter modulation ranges (Rule 30 producing full random excursions, Rule 110 producing structured modulation) interact maximally with a saw source because there is always harmonic content available at any cutoff position. **Recommendation: Saw + Rules 30/110/54 = maximum timbral range.**

**Square oscillator + periodic rules (90, 184):**
The square wave contains only odd harmonics (1, 3, 5...). When a periodic rule like Rule 90 drives the filter in quasi-regular cycles, the odd-harmonic content creates a nasal, woodwind-like quality as the filter sweep crosses odd-harmonic peaks and troughs at predictable intervals. Rule 184's traffic rhythm against a square source sounds like a gated pulse sequence — the density waves produce natural on/off character that matches the square wave's own on/off topology. **Recommendation: Square + Rules 90/184 = harmonic-periodic interaction, quasi-woodwind or gated character.**

**Triangle oscillator + sparse rules (18, 150):**
The triangle contains very weak upper harmonics (rolling off at 1/n² vs. saw's 1/n). This means filter modulation has less timbral impact — the triangle is already naturally dark and lacks the higher harmonics that the filter would normally cut. Sparse rules like Rule 18 produce long periods of low filter cutoff anyway; a triangle oscillator in this context produces a sine-like tone with occasional brief bright accents. This combination is the most subtle in the collection — ORGANISM as a barely-there presence. **Recommendation: Triangle + Rules 18/22 = subtle background color, near-sine character with occasional harmonic flashes.**

**Sub oscillator level interaction:**
The sub oscillator is always a square wave one octave below, mixed by `org_subLevel`. At high sub levels (0.7+), the fundamental becomes dominant. Rules with high average cell density (22, 150) keep the filter relatively open, allowing the sub's square-wave character to remain present. Rules with sparse density (18) push the filter low, turning the sub into a pure low-frequency thump. The sub level effectively determines whether the bass register is a constant foundation (high sub + dense rule) or an occasional punctuation (high sub + sparse rule).

---

### Question III — Cell Count and History Depth: The Scope/Rate Surface

ORGANISM has a fixed 16-cell automaton — the cell count is not a user parameter. But the effective number of generational samples that drive modulation is controlled by `org_scope` (1–16 generations of moving average). Combined with `org_stepRate` (0.5–32 Hz), this creates a two-dimensional control surface whose corners define four fundamentally different instruments:

```
                        LOW STEP RATE (0.5–2 Hz)

              Geological Drift               Crystalline Steps
              scope=16, rate=0.5            scope=1, rate=0.5
              One generation per 2 seconds.  One generation per 2 seconds.
              16-generation average.          Raw output.
              Nearly static modulation.      Discrete, slow timbral events.
              ORGANISM as a static timbre.   ORGANISM as a slow arpeggiator.

     HIGH ◄─────────────────────────────────────────────────────► LOW
     SCOPE                                                        SCOPE

              Continuous Smooth Modulator   Granular Chaos
              scope=16, rate=32             scope=1, rate=32
              32 gen/sec, averaged over 16. 32 gen/sec, no averaging.
              Smooth, fast LFO character.   Maximum variation, near noise.
              ORGANISM as a complex LFO.    ORGANISM as a spectral disruptor.

                        HIGH STEP RATE (28–32 Hz)
```

**The musically sweet spots within this surface:**

- **scope=4, rate=4–8 Hz:** The default-adjacent zone. Four-generation averaging with moderate step rate produces recognizable rhythm while smoothing out single-step chaos. Rules with defined periodic character (90, 184) produce discernible repeating patterns at these settings.

- **scope=8, rate=2–4 Hz:** The "dreaming" zone. Eight-generation averaging produces slow drift where the source rule's statistical character emerges without the rapid variation. Rule 110 at these settings sounds like a gentle, slow-breathing evolution — unpredictable but never startling.

- **scope=1, rate=16–24 Hz:** The "flicker" zone. No averaging, moderate-to-fast step rate. Individual cellular states drive all parameters with no smoothing lag. The one-pole smoothing constant (kSmoothCoeff=0.005f) provides just enough audio-rate anti-aliasing to prevent clicking, but the modulation signal is genuinely discontinuous. This is ORGANISM as a rhythmic disruptor.

- **scope=16, rate=0.5–1 Hz:** The "geological" zone. 16-generation averaging at minimum step rate means the modulation barely moves within a 30-second period. ORGANISM becomes a slowly-morphing static timbre. Use freeze=true to snapshot one generation for true stasis; use this zone for presets that evolve only over minutes.

**Interaction with evolution:**
At very low scope (1–2) with high step rate (28–32 Hz), the averaging period is too short to suppress single-step variation — every CA step produces an immediate, unaveraged timbral event. This is chaos-adjacent territory. The saving grace: pitch output is quantized to semitones (`std::round(rawSemitones)`), so even in maximum chaos mode the pitch output stays on-grid. The melodic identity survives the chaos; the timbral identity does not.

---

### Question IV — How the State History Visualization Drives Sound

The `OrgScopeHistory` circular buffer is the engine's critical innovation. Without it, raw CA-to-audio mapping would produce abrupt parameter jumps at each step — clicking, chaotic, unmusical. With it, the moving average of recent states creates a continuous signal that tracks the CA's statistical tendency rather than its instantaneous value.

**What this means for sound design:**

Each of the four cell groups (0–3, 4–7, 8–11, 12–15) maps to a different modulation destination. The scope averaging smooths all four simultaneously. This has different sonic consequences per destination:

*Cells 0–3 → Filter cutoff (200–8000 Hz range, offset ±80% of baseCutoff):*
The filter response to scope averaging is the most audible. At scope=1, filter changes are immediate — each CA step drives an abrupt cutoff shift. At scope=16, the filter follows a slow moving average that sounds like a natural, organic modulation curve. **Design implication:** For filter-forward presets (high filterRes, wide baseCutoff range), use scope ≥ 6 to prevent the filter from sounding mechanical. For rhythmic filter effects, use scope=1–3.

*Cells 4–7 → Amplitude envelope rate (×3 slowdown to ×0.33 speedup):*
The envelope rate modulation is the most subtle destination. When cells 4–7 average high, envelopes complete faster; when they average low, envelopes breathe more slowly. At high scope values, this produces a gentle "alive" quality to the envelope — like a living organism that sometimes breathes faster. At low scope, each step can radically alter the envelope speed. **Design implication:** The amplitude rate modulation is most musical at scope=4–8, where it produces a measurable but not dramatic envelope variation. Below scope=3, envelope rates can stutter to near-maximum or minimum on consecutive steps, producing uncontrolled rhythmic artifacts.

*Cells 8–11 → Pitch offset (±6 semitones, quantized):*
Pitch is always quantized to semitones regardless of scope. This means scope averaging affects how quickly the pitch changes between semitone values, not whether it quantizes. At scope=1, the pitch jumps immediately to the nearest semitone every step. At scope=16, the pitch changes very rarely — the averaging produces a near-constant pitch that occasionally drifts by a semitone when a long sequence of cellular states consistently favor one direction. **Design implication:** Pitch modulation by the automaton is most musical at scope=6–12 for slow-drift effects, or scope=1–2 for rapid semitone-sequencing effects. The middle (scope=3–5) produces the most tonally ambiguous territory: pitch changes often enough to be heard but not fast enough to sound like arpeggiation.

*Cells 12–15 → Reverb mix (additive 0–0.3 on top of base reverbMix):*
The reverb send is the most forgiving destination — changes in reverb amount are perceptually lagged and never sound abrupt. Even at scope=1, the reverb modulation sounds natural because the reverb tail itself is a temporal averaging mechanism. **Design implication:** Reverb cells benefit most from low scope values (1–4), which allow maximum CA variation to reach the send level. This creates a spatial effect where denser colony states produce more reverb — the organism "blooms" spatially when it is most active.

---

### Question V — Macro Sweet Spots

**M1: RULE (org_macroRule, 0–1 → curated rules index 0–7)**

The curated rules in index order: 30, 90, 110, 184, 150, 18, 54, 22.

Macro position 0.0 = Rule 30 (pseudorandom).
Macro position ~0.14 = Rule 90 (Sierpinski fractal).
Macro position ~0.29 = Rule 110 (Turing-complete, default).
Macro position ~0.43 = Rule 184 (traffic flow).
Macro position ~0.57 = Rule 150 (symmetric XOR).
Macro position ~0.71 = Rule 18 (sparsifying gliders).
Macro position ~0.86 = Rule 54 (complex transient).
Macro position 1.0 = Rule 22 (dense Class III).

**Sweet spots:** 0.29 (Rule 110 — default, immediate movement), 0.14 (Rule 90 — rhythmic fractal), 0.43 (Rule 184 — traffic pulse).

**Performance gesture:** Sweep RULE from 0.0 to 1.0 over 8 beats. The colony passes through pseudorandom → fractal → Turing-complete → traffic → symmetric → sparse → complex transient → dense. This sweep is a complete tour of Wolfram's computational universe, made audible.

**Mod wheel interaction:** The mod wheel adds ±2 rule positions to the macro-selected position. This is a "micro-morph" within the neighborhood of the current rule. At macro=0.29 (Rule 110), mod wheel up blends toward Rule 184 (traffic); mod wheel down blends toward Rule 90 (fractal). This is more nuanced than it sounds — it creates hybrid rule behaviors that do not exist as standalone rules.

**M2: SEED (org_macroSeed, 0–1 → latch trigger)**

SEED is not a continuous control — it is a trigger with a latch. Once raised above 0.01, it fires a new LCG-derived 16-bit state into the automaton (once). Once lowered below 0.005, the latch resets. The next raise triggers another new state.

**Sweet spots:** 0.0 (no re-seeding — stable organism identity), 0.7–1.0 (generous latch zone — reliable re-trigger on each gesture).

**Performance gesture:** Hold SEED at 0.0 for the first 4 bars (consistent colony identity). Raise briefly to 0.8, lower, raise, lower — each crossing injects a new colony. Each new colony from the same rule produces different modulation trajectories. One rule, infinite organisms.

**Critical insight from the retreat:** When macroSeed stays above 0.01 between notes, the latch is already engaged when the next note arrives. The organism continues from its previous state rather than starting fresh. This is the "continuity" mode — one colony persists across multiple notes. Lowering SEED to 0 between notes enables the note-on XOR seed calculation, giving each note its own colony variant.

**M3: COUPLING (org_macroCoupling, 0–1)**

COUPLING serves two functions: it scales the engine's receive sensitivity for incoming coupling signals (`recvScale = 0.5 + macroCoupling * 0.5f`), and it adds a subtle autonomous mutation boost (`effectiveMutate += macroCoupling * 0.01f`).

**Sweet spots:** 0.0 (isolated colony, consistent behavior), 0.5 (half receive sensitivity — moderate coupling receive), 1.0 (maximum coupling receive + maximum autonomous mutation boost).

**At solo use (no coupled engine):** The autonomous mutation at COUPLING=1.0 adds only 0.01 probability per cell per step — barely perceptible in isolation. But over many generations (hundreds of steps), this low mutation rate causes the colony's statistical character to slowly drift. A colony running at COUPLING=1.0 for 30 seconds will have diverged from its original trajectory in ways not achievable by the base rule alone. COUPLING=1.0 is the "entropy" setting for solo use.

**Best coupling partners (from organism-retreat.md):** OVERTONE (spectral) — ORGANISM's CA rhythm drives OVERTONE's partial selection. The CA's rhythmic density variations create harmonic sequences that OVERTONE makes musically coherent.

**M4: MUTATE (org_macroMutate, 0–1)**

Mutation probability per cell per step. This stacks additively with `org_mutate` and aftertouch (`aftertouchVal * 0.3`).

**At 0.0:** Deterministic rule-governed evolution. The colony's future is entirely determined by its current state and the rule. Given the same seed and rule, the colony will always produce the same evolution.

**At 0.1–0.3:** Low mutation. The colony occasionally receives random bit flips that kick it into unexplored regions of its state space. The rule still dominates, but the colony never repeats exactly. This is the "biological noise" zone — the equivalent of cosmic ray mutations in real DNA.

**At 0.5:** High mutation. At this level, approximately 8 of the 16 cells are randomly flipped each generation after the rule is applied. The rule no longer governs the colony — it provides a statistical bias in a fundamentally stochastic process. Different rules produce different noise textures: Rule 90's symmetric character at high mutation produces a roughly symmetric noise spectrum. Rule 110's complex character produces a less symmetric noise texture.

**At 1.0:** Maximum mutation. Every cell has a 100% probability of being flipped. This negates the rule entirely — the evolution is pure random bit noise. The colony becomes a 16-bit random number generator running at `stepRate` Hz, producing full-range random modulation of all four destinations.

**Sweet spots:** 0.0 (deterministic — for structured patches), 0.08–0.15 (biological noise — organic imperfection without chaos), 0.5 (stochastic mode — noise synthesis with rule-colored texture), 1.0 (maximum chaos — academic/experimental territory).

**Aftertouch interaction:** Aftertouch adds `aftertouchVal * 0.3` to effective mutation. This means: at macroMutate=0, pressing harder provides 0–30% mutation. At macroMutate=0.7, maximum aftertouch caps total mutation at 1.0. Design presets with macroMutate at low values (0.0–0.2) so aftertouch has a meaningful expressive range.

---

## Awakening Preset Table

Ten presets, each demonstrating a specific insight from the five meditations above. Each preset is complete, playable from the first note, and demonstrates something not already demonstrated in the existing library.

| # | Name | Rule | Seed | oscWave | Key Parameters | Discovery |
|---|------|------|------|---------|----------------|-----------|
| 1 | Coral Law | 110 | 42 | saw | scope=4, rate=6, sub=0.4, res=0.35 | Rule 110: default organism, Turing-complete base |
| 2 | Sierpinski Gate | 90 | 1024 | square | scope=3, rate=8, res=0.65, sub=0.25 | Rule 90 fractal + square wave harmonic-periodic interaction |
| 3 | Traffic Thump | 184 | 2048 | saw | scope=6, rate=3, sub=0.72, atk=0.08 | Rule 184 traffic density + dominant sub |
| 4 | Geological | 110 | 42 | tri | scope=16, rate=0.5, lfo1=0.01, sub=0.3 | Low rate + max scope = geological drift |
| 5 | Flicker Colony | 30 | 32767 | saw | scope=1, rate=24, res=0.55, sub=0.2 | scope=1 + high rate = granular CA chaos |
| 6 | Resonant Fractal | 90 | 512 | square | scope=1, rate=12, res=0.78, cutoff=1800 | Fractal rule + resonant filter = emergent melody |
| 7 | Stochastic Organism | 30 | 16384 | saw | scope=4, rate=4, mutate=0.5 | High mutation + Rule 30 = noise synthesis mode |
| 8 | Rule 54 Maturation | 54 | 100 | saw | scope=8, rate=2, atk=0.5, rel=3.0 | Class IV long transient — colony matures over note duration |
| 9 | Sparse Light | 18 | 777 | tri | scope=8, rate=1, sub=0.85, cutoff=400 | Rule 18 sparsifying + triangle + deep sub = rhythmic punctuation |
| 10 | Aftertouch Biome | 110 | 42 | square | scope=5, rate=5, mutate=0.0, macroMutate=0.0 | Aftertouch controls full mutation range: press for chaos |

---

## Preset Specifications

---

### 1. Coral Law
*The reference organism. Rule 110, seed 42. The engine's soul stated plainly.*

Rule 110 from seed 42 is the engine's default identity and the correct entry point for any Guru Bin practitioner. The Turing-complete Class IV behavior produces continuous, non-repeating structural evolution. Scope=4 and rate=6 Hz sits in the "recognizable rhythm" zone: individual CA steps are discernible but not mechanical. The saw oscillator maximizes the filter modulation's timbral impact — every cutoff event moves through a full harmonic spectrum.

This preset is not about novelty. It is about demonstrating the foundation. The coral colony as it was designed to be: alive from the first note, never repeating, never collapsing.

```json
{
  "schema_version": 1,
  "name": "Coral Law",
  "mood": "Foundation",
  "engines": ["Organism"],
  "author": "Guru Bin",
  "version": "1.0.0",
  "tier": "awakening",
  "description": "Rule 110 from seed 42 — the default organism. Turing-complete Class IV. Never repeats, never collapses.",
  "tags": ["foundation", "rule-110", "reference", "generative"],
  "macroLabels": ["RULE", "SEED", "COUPLING", "MUTATE"],
  "couplingIntensity": "None",
  "tempo": null,
  "dna": {
    "brightness": 0.55,
    "warmth": 0.5,
    "movement": 0.6,
    "density": 0.4,
    "space": 0.25,
    "aggression": 0.2
  },
  "parameters": {
    "Organism": {
      "org_rule": 110.0,
      "org_seed": 42,
      "org_stepRate": 6.0,
      "org_scope": 4,
      "org_mutate": 0.0,
      "org_freeze": 0,
      "org_oscWave": 0,
      "org_subLevel": 0.4,
      "org_filterCutoff": 3200.0,
      "org_filterRes": 0.35,
      "org_velCutoff": 0.55,
      "org_ampAtk": 0.02,
      "org_ampDec": 0.3,
      "org_ampSus": 0.72,
      "org_ampRel": 0.8,
      "org_lfo1Rate": 0.4,
      "org_lfo1Depth": 0.15,
      "org_lfo2Rate": 0.25,
      "org_lfo2Depth": 0.18,
      "org_reverbMix": 0.18,
      "org_macroRule": 0.29,
      "org_macroSeed": 0.0,
      "org_macroCoupling": 0.0,
      "org_macroMutate": 0.0
    }
  }
}
```

---

### 2. Sierpinski Gate
*Rule 90's fractal density rhythm, channeled through a square wave's odd harmonic ladder.*

Rule 90 is the XOR rule — each new cell equals the XOR of its two neighbors. Starting from seed 1024 (a single burst of activity on the high-bit side), the Sierpinski triangle fractal pattern begins immediately. In 16 cells with circular wrap, the fractal wraps and the density output (cells 0–3) oscillates with quasi-periodic character.

The square wave contains only odd harmonics. When the fractal-regular filter sweep crosses odd harmonic peaks at predictable rhythmic intervals, a woodwind-like harmonic progression emerges. This is the resonant fractal interaction identified in Question II. Filterres=0.65 (Q≈8.0) provides audible resonant ringing without instability.

Scope=3 preserves enough of the fractal regularity to hear its periodicity without averaging it into continuous drift. Rate=8 Hz puts one CA generation every 125ms — musically, roughly quarter-note territory at 120 BPM.

```json
{
  "schema_version": 1,
  "name": "Sierpinski Gate",
  "mood": "Prism",
  "engines": ["Organism"],
  "author": "Guru Bin",
  "version": "1.0.0",
  "tier": "awakening",
  "description": "Rule 90 Sierpinski fractal with square wave. Harmonic sweeps follow the fractal rhythm. Woodwind character.",
  "tags": ["prism", "rule-90", "fractal", "harmonic", "woodwind"],
  "macroLabels": ["RULE", "SEED", "COUPLING", "MUTATE"],
  "couplingIntensity": "None",
  "tempo": null,
  "dna": {
    "brightness": 0.65,
    "warmth": 0.35,
    "movement": 0.65,
    "density": 0.45,
    "space": 0.3,
    "aggression": 0.3
  },
  "parameters": {
    "Organism": {
      "org_rule": 90.0,
      "org_seed": 1024,
      "org_stepRate": 8.0,
      "org_scope": 3,
      "org_mutate": 0.0,
      "org_freeze": 0,
      "org_oscWave": 1,
      "org_subLevel": 0.25,
      "org_filterCutoff": 2400.0,
      "org_filterRes": 0.65,
      "org_velCutoff": 0.6,
      "org_ampAtk": 0.012,
      "org_ampDec": 0.25,
      "org_ampSus": 0.68,
      "org_ampRel": 0.7,
      "org_lfo1Rate": 0.8,
      "org_lfo1Depth": 0.12,
      "org_lfo2Rate": 0.5,
      "org_lfo2Depth": 0.15,
      "org_reverbMix": 0.2,
      "org_macroRule": 0.14,
      "org_macroSeed": 0.0,
      "org_macroCoupling": 0.0,
      "org_macroMutate": 0.0
    }
  }
}
```

---

### 3. Traffic Thump
*Rule 184 traffic jam density waves + dominant sub oscillator = rhythmic bass organism.*

Rule 184 is the traffic flow rule: live cells (particles) move rightward and block at collisions. Seed 2048 provides moderate initial density — some particles, enough open road for movement, eventual traffic jams. The density waves that pass through cells 0–3 create periodic filter openings. Against a high sub level (0.72), these filter openings punch through the low-end mix like a kick drum sidechain without requiring one.

Scope=6 averages 6 generations — long enough to suppress single-cell noise, short enough to track the traffic density rhythm. Rate=3 Hz (one generation every ~333ms) sits near triplet-quarter territory at 90 BPM, giving the traffic rhythm a natural groove feel.

The slow attack (0.08s) prevents the note from immediately slamming into the sub oscillator — the colony builds. High sub at 0.72 dominates the low frequencies, with the saw providing mid and upper content only when the filter opens.

```json
{
  "schema_version": 1,
  "name": "Traffic Thump",
  "mood": "Foundation",
  "engines": ["Organism"],
  "author": "Guru Bin",
  "version": "1.0.0",
  "tier": "awakening",
  "description": "Rule 184 traffic density waves drive filter over a dominant sub. Bass organism with embedded rhythm.",
  "tags": ["foundation", "rule-184", "bass", "rhythmic", "sub"],
  "macroLabels": ["RULE", "SEED", "COUPLING", "MUTATE"],
  "couplingIntensity": "None",
  "tempo": null,
  "dna": {
    "brightness": 0.3,
    "warmth": 0.75,
    "movement": 0.55,
    "density": 0.6,
    "space": 0.15,
    "aggression": 0.45
  },
  "parameters": {
    "Organism": {
      "org_rule": 184.0,
      "org_seed": 2048,
      "org_stepRate": 3.0,
      "org_scope": 6,
      "org_mutate": 0.0,
      "org_freeze": 0,
      "org_oscWave": 0,
      "org_subLevel": 0.72,
      "org_filterCutoff": 1800.0,
      "org_filterRes": 0.28,
      "org_velCutoff": 0.65,
      "org_ampAtk": 0.08,
      "org_ampDec": 0.4,
      "org_ampSus": 0.65,
      "org_ampRel": 1.2,
      "org_lfo1Rate": 0.3,
      "org_lfo1Depth": 0.1,
      "org_lfo2Rate": 0.2,
      "org_lfo2Depth": 0.12,
      "org_reverbMix": 0.12,
      "org_macroRule": 0.43,
      "org_macroSeed": 0.0,
      "org_macroCoupling": 0.0,
      "org_macroMutate": 0.0
    }
  }
}
```

---

### 4. Geological
*Maximum scope, minimum step rate. The colony evolves over geological time.*

The extreme corner of the scope/rate surface: scope=16 (maximum averaging) and rate=0.5 Hz (one generation every 2 seconds). At 44.1 kHz, that is 88,200 samples between CA steps. With 16 generations of averaging, a single modulation cycle takes over 32 seconds. The filter, pitch, and reverb outputs barely move on any human timescale shorter than a minute.

The triangle oscillator is chosen deliberately. At near-static modulation, the timbral character is determined almost entirely by the oscillator waveform's intrinsic harmonic content. Triangle's 1/n² harmonic rolloff produces a soft, sine-adjacent tone that drifts almost imperceptibly. Both LFOs are set to 0.01 Hz floor — 100-second periods. This preset validates D005 fully: the engine breathes, but it breathes like a planet.

This preset is a meditation device. Play a note. Wait.

```json
{
  "schema_version": 1,
  "name": "Geological",
  "mood": "Atmosphere",
  "engines": ["Organism"],
  "author": "Guru Bin",
  "version": "1.0.0",
  "tier": "awakening",
  "description": "Maximum scope, minimum rate. The colony evolves on geological time. Both LFOs at 100-second period.",
  "tags": ["atmosphere", "slow", "drone", "meditation", "d005"],
  "macroLabels": ["RULE", "SEED", "COUPLING", "MUTATE"],
  "couplingIntensity": "None",
  "tempo": null,
  "dna": {
    "brightness": 0.3,
    "warmth": 0.7,
    "movement": 0.05,
    "density": 0.3,
    "space": 0.6,
    "aggression": 0.0
  },
  "parameters": {
    "Organism": {
      "org_rule": 110.0,
      "org_seed": 42,
      "org_stepRate": 0.5,
      "org_scope": 16,
      "org_mutate": 0.0,
      "org_freeze": 0,
      "org_oscWave": 2,
      "org_subLevel": 0.3,
      "org_filterCutoff": 2800.0,
      "org_filterRes": 0.22,
      "org_velCutoff": 0.4,
      "org_ampAtk": 1.5,
      "org_ampDec": 2.0,
      "org_ampSus": 0.8,
      "org_ampRel": 4.5,
      "org_lfo1Rate": 0.01,
      "org_lfo1Depth": 0.3,
      "org_lfo2Rate": 0.01,
      "org_lfo2Depth": 0.25,
      "org_reverbMix": 0.45,
      "org_macroRule": 0.29,
      "org_macroSeed": 0.0,
      "org_macroCoupling": 0.0,
      "org_macroMutate": 0.0
    }
  }
}
```

---

### 5. Flicker Colony
*Scope=1, rate=24 Hz. The granular chaos corner. Raw unaveraged CA at near-audio rate.*

The opposite extreme from Geological. No scope averaging, 24 generations per second. The one-pole smoothing constant (kSmoothCoeff=0.005f) barely keeps up with the incoming changes — the filter, pitch, and envelope destinations receive near-instantaneous parameter jumps at 24 Hz, creating a granular, flickering quality. Rule 30 is used because its pseudorandom character produces the widest modulation range — maximum chaos from a deterministic rule.

Seed 32767 (0x7FFF, all lower 15 bits set) produces maximum initial density. This puts cells 4–7 (amplitude rate) at maximum average, accelerating envelope rates from the start. The first few seconds of a note are extremely chaotic; as the pseudorandom evolution continues, brief quasi-periodic windows occasionally emerge.

Filterres=0.55 adds moderate Q ringing that turns each filter jump into a brief resonant ping — the flickering filter behaves like a granular resonator array.

```json
{
  "schema_version": 1,
  "name": "Flicker Colony",
  "mood": "Flux",
  "engines": ["Organism"],
  "author": "Guru Bin",
  "version": "1.0.0",
  "tier": "awakening",
  "description": "Scope=1 + rate=24Hz. No averaging, near-audio-rate chaos. Rule 30 pseudorandom. Granular filter flicker.",
  "tags": ["flux", "chaos", "granular", "rule-30", "fast"],
  "macroLabels": ["RULE", "SEED", "COUPLING", "MUTATE"],
  "couplingIntensity": "None",
  "tempo": null,
  "dna": {
    "brightness": 0.6,
    "warmth": 0.3,
    "movement": 0.95,
    "density": 0.5,
    "space": 0.2,
    "aggression": 0.65
  },
  "parameters": {
    "Organism": {
      "org_rule": 30.0,
      "org_seed": 32767,
      "org_stepRate": 24.0,
      "org_scope": 1,
      "org_mutate": 0.0,
      "org_freeze": 0,
      "org_oscWave": 0,
      "org_subLevel": 0.2,
      "org_filterCutoff": 3500.0,
      "org_filterRes": 0.55,
      "org_velCutoff": 0.5,
      "org_ampAtk": 0.01,
      "org_ampDec": 0.15,
      "org_ampSus": 0.6,
      "org_ampRel": 0.4,
      "org_lfo1Rate": 2.5,
      "org_lfo1Depth": 0.08,
      "org_lfo2Rate": 1.8,
      "org_lfo2Depth": 0.1,
      "org_reverbMix": 0.2,
      "org_macroRule": 0.0,
      "org_macroSeed": 0.0,
      "org_macroCoupling": 0.0,
      "org_macroMutate": 0.0
    }
  }
}
```

---

### 6. Resonant Fractal
*Rule 90 + high Q + low cutoff base = emergent melody from resonance sweeps.*

This preset demonstrates the most counterintuitive ORGANISM discovery: that melodic content can emerge from a single-oscillator system with no pitch sequencer, purely through high-Q filter resonance interacting with the CA-driven cutoff modulation.

Rule 90's quasi-periodic density output drives the filter cutoff in rhythmically regular sweeps. With filterRes=0.78 (Q≈9.4), the resonant peak is audible as a tonal event at whatever frequency the filter is currently tuned to. Each CA step that crosses through the filter's current resonant region rings the Q peak. The quasi-periodic timing of Rule 90's density output creates quasi-periodic melody: not random notes, not a programmed sequence, but emergent pitch content arising from the interaction between the fractal rule and the resonant filter.

Cutoff base at 1800 Hz places the Q peak in a singable range. Scope=1 ensures the filter tracks each CA step immediately, maximizing the melody's rhythmic articulation. Square wave for its odd-harmonic content — the resonant sweeps excite odd harmonic peaks at double the rate of a sawtooth.

```json
{
  "schema_version": 1,
  "name": "Resonant Fractal",
  "mood": "Prism",
  "engines": ["Organism"],
  "author": "Guru Bin",
  "version": "1.0.0",
  "tier": "awakening",
  "description": "Rule 90 fractal density drives a high-Q filter. Melody emerges from resonance sweeps. No sequencer.",
  "tags": ["prism", "rule-90", "resonance", "emergent-melody", "filterQ"],
  "macroLabels": ["RULE", "SEED", "COUPLING", "MUTATE"],
  "couplingIntensity": "None",
  "tempo": null,
  "dna": {
    "brightness": 0.7,
    "warmth": 0.35,
    "movement": 0.75,
    "density": 0.4,
    "space": 0.35,
    "aggression": 0.5
  },
  "parameters": {
    "Organism": {
      "org_rule": 90.0,
      "org_seed": 512,
      "org_stepRate": 12.0,
      "org_scope": 1,
      "org_mutate": 0.0,
      "org_freeze": 0,
      "org_oscWave": 1,
      "org_subLevel": 0.2,
      "org_filterCutoff": 1800.0,
      "org_filterRes": 0.78,
      "org_velCutoff": 0.7,
      "org_ampAtk": 0.01,
      "org_ampDec": 0.2,
      "org_ampSus": 0.65,
      "org_ampRel": 0.55,
      "org_lfo1Rate": 1.2,
      "org_lfo1Depth": 0.05,
      "org_lfo2Rate": 0.7,
      "org_lfo2Depth": 0.08,
      "org_reverbMix": 0.28,
      "org_macroRule": 0.14,
      "org_macroSeed": 0.0,
      "org_macroCoupling": 0.0,
      "org_macroMutate": 0.0
    }
  }
}
```

---

### 7. Stochastic Organism
*Mutation=0.5 with Rule 30. The threshold where rule becomes statistical bias.*

At mutation=0.5, approximately half the cells are randomly flipped each generation after the rule step. Rule 30 — already pseudorandom without mutation — at this mutation level becomes a coloured noise synthesizer. The rule provides statistical bias: Rule 30's tendency toward asymmetric density affects the noise floor distribution. Different rules at mutation=0.5 produce different noise textures. Rule 30 produces a characteristic dark noise (lower-density bias from the rule's sparsifying tendency competing with 50% random flips).

Scope=4 at rate=4 Hz is the reference default setting. The 4-generation average smooths the noise into a continuous, slowly-varying stochastic modulation. This is ORGANISM as a parametric noise synthesizer: the MUTATE macro controls density versus structure, and the RULE macro controls the noise's color.

Low reverb (0.22) to keep the noise texture present in the direct sound. High velCutoff (0.7) so velocity meaningfully opens and closes the filter — even in stochastic mode, velocity expression is musically useful.

```json
{
  "schema_version": 1,
  "name": "Stochastic Organism",
  "mood": "Flux",
  "engines": ["Organism"],
  "author": "Guru Bin",
  "version": "1.0.0",
  "tier": "awakening",
  "description": "Mutation=0.5 — the threshold where rule becomes statistical noise color. Parametric noise synthesis.",
  "tags": ["flux", "mutation", "noise", "stochastic", "rule-30"],
  "macroLabels": ["RULE", "SEED", "COUPLING", "MUTATE"],
  "couplingIntensity": "None",
  "tempo": null,
  "dna": {
    "brightness": 0.45,
    "warmth": 0.4,
    "movement": 0.7,
    "density": 0.55,
    "space": 0.2,
    "aggression": 0.55
  },
  "parameters": {
    "Organism": {
      "org_rule": 30.0,
      "org_seed": 16384,
      "org_stepRate": 4.0,
      "org_scope": 4,
      "org_mutate": 0.5,
      "org_freeze": 0,
      "org_oscWave": 0,
      "org_subLevel": 0.3,
      "org_filterCutoff": 2800.0,
      "org_filterRes": 0.38,
      "org_velCutoff": 0.7,
      "org_ampAtk": 0.015,
      "org_ampDec": 0.35,
      "org_ampSus": 0.68,
      "org_ampRel": 0.7,
      "org_lfo1Rate": 0.6,
      "org_lfo1Depth": 0.1,
      "org_lfo2Rate": 0.35,
      "org_lfo2Depth": 0.12,
      "org_reverbMix": 0.22,
      "org_macroRule": 0.0,
      "org_macroSeed": 0.0,
      "org_macroCoupling": 0.0,
      "org_macroMutate": 0.5
    }
  }
}
```

---

### 8. Rule 54 Maturation
*Class IV long transient. A short note sounds random. A long note develops structure.*

Rule 54 is one of only a handful of confirmed Class IV rules — complex, with long transients before quasi-stable pattern emergence. In 16 cells with seed=100 (sparse, mostly zeros with a small cluster), the initial generations produce complex, apparently chaotic output. After many generations (the exact count depends on the seed), a quasi-stable pattern begins to emerge. The colony matures.

Rate=2 Hz (one generation every 500ms), scope=8. At this rate, the transient period lasts multiple seconds of audio — a sustained note witnesses the colony's progression from chaos to structure. This is narrative synthesis: ORGANISM as a time-based story where the note length determines which chapter you hear.

Slow attack (0.5s) and long release (3.0s) honor the temporal character. High reverb (0.38) so the colony's spatial evolution is audible as the reverb send (cells 12–15) changes character over the note's lifetime.

```json
{
  "schema_version": 1,
  "name": "Rule 54 Maturation",
  "mood": "Atmosphere",
  "engines": ["Organism"],
  "author": "Guru Bin",
  "version": "1.0.0",
  "tier": "awakening",
  "description": "Class IV Rule 54. Short note = chaos. Long note = maturation toward structure. A colony coming of age.",
  "tags": ["atmosphere", "rule-54", "temporal", "narrative", "maturation"],
  "macroLabels": ["RULE", "SEED", "COUPLING", "MUTATE"],
  "couplingIntensity": "None",
  "tempo": null,
  "dna": {
    "brightness": 0.4,
    "warmth": 0.55,
    "movement": 0.5,
    "density": 0.35,
    "space": 0.55,
    "aggression": 0.15
  },
  "parameters": {
    "Organism": {
      "org_rule": 54.0,
      "org_seed": 100,
      "org_stepRate": 2.0,
      "org_scope": 8,
      "org_mutate": 0.0,
      "org_freeze": 0,
      "org_oscWave": 0,
      "org_subLevel": 0.35,
      "org_filterCutoff": 2600.0,
      "org_filterRes": 0.32,
      "org_velCutoff": 0.5,
      "org_ampAtk": 0.5,
      "org_ampDec": 0.8,
      "org_ampSus": 0.75,
      "org_ampRel": 3.0,
      "org_lfo1Rate": 0.08,
      "org_lfo1Depth": 0.2,
      "org_lfo2Rate": 0.06,
      "org_lfo2Depth": 0.18,
      "org_reverbMix": 0.38,
      "org_macroRule": 0.86,
      "org_macroSeed": 0.0,
      "org_macroCoupling": 0.0,
      "org_macroMutate": 0.0
    }
  }
}
```

---

### 9. Sparse Light
*Rule 18 sparsifying + triangle oscillator + dominant sub. Rhythmic punctuation with deep bass.*

Rule 18 is the sparsifying rule — dense initial states collapse toward isolated particles. Seed=777 (moderate density initial state) gives the colony enough material to start with activity that then gradually collapses toward sparsity. The filter cells (0–3) tend toward low averages over time: most of the time, the filter is nearly closed. Brief periods of cellular density punctuate the silence with bright filter openings.

The triangle oscillator in this dark context sounds like a sine wave with occasional harmonic content — extremely clean. The sub level at 0.85 makes the sub oscillator (one octave below, square wave) the dominant character. Filter base at 400 Hz means most of the tone lives in the bass register. The brief filter openings from Rule 18's density events punch the sub's square harmonics through the filter for short bright moments.

Scope=8 ensures the sparse-dense transitions are gradual — not abrupt clicks but slow blooms of harmonic content. Rate=1 Hz gives those blooms a dreamlike, disconnected quality.

```json
{
  "schema_version": 1,
  "name": "Sparse Light",
  "mood": "Submerged",
  "engines": ["Organism"],
  "author": "Guru Bin",
  "version": "1.0.0",
  "tier": "awakening",
  "description": "Rule 18 collapses toward sparse. Triangle + dominant sub. Long silences broken by brief harmonic blooms.",
  "tags": ["submerged", "rule-18", "sparse", "sub", "dark", "deep"],
  "macroLabels": ["RULE", "SEED", "COUPLING", "MUTATE"],
  "couplingIntensity": "None",
  "tempo": null,
  "dna": {
    "brightness": 0.15,
    "warmth": 0.8,
    "movement": 0.25,
    "density": 0.2,
    "space": 0.5,
    "aggression": 0.1
  },
  "parameters": {
    "Organism": {
      "org_rule": 18.0,
      "org_seed": 777,
      "org_stepRate": 1.0,
      "org_scope": 8,
      "org_mutate": 0.0,
      "org_freeze": 0,
      "org_oscWave": 2,
      "org_subLevel": 0.85,
      "org_filterCutoff": 400.0,
      "org_filterRes": 0.25,
      "org_velCutoff": 0.6,
      "org_ampAtk": 0.3,
      "org_ampDec": 0.6,
      "org_ampSus": 0.55,
      "org_ampRel": 2.5,
      "org_lfo1Rate": 0.05,
      "org_lfo1Depth": 0.15,
      "org_lfo2Rate": 0.04,
      "org_lfo2Depth": 0.12,
      "org_reverbMix": 0.35,
      "org_macroRule": 0.71,
      "org_macroSeed": 0.0,
      "org_macroCoupling": 0.0,
      "org_macroMutate": 0.0
    }
  }
}
```

---

### 10. Aftertouch Biome
*Mutation locked to aftertouch. Press softly for order; press hard for the colony to mutate.*

This preset makes the aftertouch-to-mutation mapping its entire design premise. With macroMutate=0.0 and org_mutate=0.0, the colony runs in purely deterministic mode. Rule 110, scope=5, rate=5 Hz — the reference organism, well-behaved.

Light aftertouch (say, 0.3 value = 9% mutation probability per cell) introduces biological noise into the colony's evolution without destroying its structure. Moderate aftertouch (0.5 = 15% mutation) begins to shift the colony's trajectory noticeably — notes taken from the same starting point diverge over 10–20 generations. Heavy aftertouch (1.0 = 30% mutation) begins pushing the colony toward pseudo-random territory, with the rule providing only statistical coloring.

This is the most expressive preset in the Awakening set. The same note, played with different touch, produces different organisms. One key: infinite biomes, ordered by pressure.

Square wave at this setting: the odd-harmonic character makes mutation events more audible as harmonic jumps rather than timbral smearing.

```json
{
  "schema_version": 1,
  "name": "Aftertouch Biome",
  "mood": "Entangled",
  "engines": ["Organism"],
  "author": "Guru Bin",
  "version": "1.0.0",
  "tier": "awakening",
  "description": "Deterministic at rest. Aftertouch = mutation rate. Press softly for order; press hard for biome shift.",
  "tags": ["entangled", "aftertouch", "mutation", "expressive", "d006"],
  "macroLabels": ["RULE", "SEED", "COUPLING", "MUTATE"],
  "couplingIntensity": "None",
  "tempo": null,
  "dna": {
    "brightness": 0.55,
    "warmth": 0.45,
    "movement": 0.6,
    "density": 0.4,
    "space": 0.3,
    "aggression": 0.35
  },
  "parameters": {
    "Organism": {
      "org_rule": 110.0,
      "org_seed": 42,
      "org_stepRate": 5.0,
      "org_scope": 5,
      "org_mutate": 0.0,
      "org_freeze": 0,
      "org_oscWave": 1,
      "org_subLevel": 0.3,
      "org_filterCutoff": 3000.0,
      "org_filterRes": 0.42,
      "org_velCutoff": 0.55,
      "org_ampAtk": 0.015,
      "org_ampDec": 0.32,
      "org_ampSus": 0.7,
      "org_ampRel": 0.75,
      "org_lfo1Rate": 0.45,
      "org_lfo1Depth": 0.12,
      "org_lfo2Rate": 0.3,
      "org_lfo2Depth": 0.15,
      "org_reverbMix": 0.22,
      "org_macroRule": 0.29,
      "org_macroSeed": 0.0,
      "org_macroCoupling": 0.0,
      "org_macroMutate": 0.0
    }
  }
}
```

---

## The Five Answers Restated

**1. Which Wolfram rules produce the most musical patterns?**
Rules 110 (Class IV, Turing-complete) and 90 (Sierpinski fractal) are the most immediately musical — 110 for continuous evolving complexity, 90 for quasi-periodic rhythmic character. Rule 184 (traffic flow) is most musical for bass applications where rhythmic density waves drive sub-dominant patches. Rule 54 (Class IV transient) is most musical for temporal narrative across long notes.

**2. How does rule selection interact with oscillator type?**
Saw + complex rules (30, 110, 54) maximizes timbral range. Square + periodic rules (90, 184) produces harmonic-periodic interaction and quasi-woodwind character. Triangle + sparse rules (18, 22) produces subtle background color with near-sine character. The sub oscillator level determines whether bass content is constant (high sub + dense rule) or punctuation (high sub + sparse rule).

**3. What cell count and history depth creates interesting evolution without chaos?**
The 16-cell circular tape is fixed. The interesting zone within the scope/rate surface is scope=3–8 with rate=3–12 Hz — recognizable patterns without mechanical rigidity. Scope=1 + rate=24+ Hz creates granular chaos. Scope=16 + rate=0.5 creates geological drift. Both extremes are musically valid but require specific context.

**4. How does the state history visualization drive sound?**
The scope averaging is the key. Filter destination needs scope ≥ 6 for smooth musical sweeps; scope=1–3 for rhythmic articulation. Amplitude rate destination is most musical at scope=4–8. Pitch destination creates arpeggiation at scope=1–2 and harmonic drift at scope=6–12. Reverb destination benefits from low scope (1–4), as the reverb tail itself provides temporal smoothing.

**5. Sweet spots for the 4 macros?**
RULE: 0.29 (Rule 110 default), 0.14 (Rule 90 fractal), 0.43 (Rule 184 traffic). SEED: 0.0 (stable identity) or 0.7–1.0 (reliable re-trigger). COUPLING: 0.0 (isolated), 0.5 (standard coupling receive), 1.0 (maximum entropy addition). MUTATE: 0.0 (deterministic), 0.08–0.15 (biological noise), 0.5 (noise synthesis mode).

---

## Appendix: Curated Rule Quick Reference

| Rule | Class | Behavior | Best Waveform | Best Scope | Best Rate |
|------|-------|----------|---------------|------------|-----------|
| 30 | III | Pseudorandom, deterministic noise | Saw | 4–6 | 4–8 Hz |
| 90 | III | Sierpinski fractal, quasi-periodic | Square | 1–4 | 6–12 Hz |
| 110 | IV | Turing-complete, structured chaos | Saw | 4–6 | 4–8 Hz |
| 184 | II | Traffic flow, density waves | Saw | 6–8 | 2–4 Hz |
| 150 | III | Symmetric XOR, bilateral oscillation | Tri | 4–8 | 3–6 Hz |
| 18 | III | Sparsifying, isolated gliders | Tri | 6–10 | 1–3 Hz |
| 54 | IV | Long transient, delayed maturation | Saw | 6–10 | 1–4 Hz |
| 22 | III | Dense Class III, stable modulation | Tri or Saw | 4–8 | 3–6 Hz |

---

*Guru Bin Deep Meditation — ORGANISM — 2026-03-21*
*Coral Colony, Emergence Lime `#C6E377`. Simple rules. Emergent architecture.*
