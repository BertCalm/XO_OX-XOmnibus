══════════════════════════════════════════════════════
  RETREAT BRIEFING: ORGANON
  Prepared for Guru Bin — 2026-03-16
══════════════════════════════════════════════════════

## Engine Identity

- **Gallery Code:** ORGANON
- **Source Instrument:** XOrganon
- **Parameter Prefix:** `organon_`
- **Accent:** Bioluminescent Cyan `#00CED1`
- **Creature:** The Deep-Sea Chemotroph — a Fourth Generation species born from
  feliX and Oscar's coupling. Lives below the thermocline where sunlight never
  reaches. Consumes audio signals and converts their information content (entropy)
  into living harmonic structures. When starved, it dims. When fed rich signal,
  it blooms. No two performances are identical because the organism accumulates
  metabolic state from its history of coupling inputs.
- **Aquatic Depth:** Oscar-leaning — The Deep (slow, chemical, alien metabolism)
- **Polyphony:** 4 voices (independent organisms), each with its own full
  metabolic cycle
- **Synthesis Type:** Informational Dissipative Synthesis —
  Ingestion → Catabolism (Shannon entropy) → Metabolic Economy (VFE/Active
  Inference) → Anabolism (32-mode Port-Hamiltonian modal array)

---

## Architecture Summary

### Synthesis Type

Organon is not a conventional synthesizer. It is a metabolic system. Sound is
a byproduct of biological process, not a parameter adjustment.

**Four-stage signal chain:**

1. **INGESTION** — Audio enters through an enzyme-selective bandpass filter
   (CytomicSVF in BandPass mode). In self-feeding mode the organism generates
   internal xorshift32 noise filtered through this bandpass. In coupling mode
   it consumes audio from partner engines via a 2048-sample ring buffer per
   voice.

2. **CATABOLISM** — Shannon entropy analysis (`EntropyAnalyzer`): the ingested
   signal is quantized into a 32-bin amplitude histogram over a 256-sample
   (or 64-sample at high enzymeSelect) window. Shannon entropy H = -Σ(p·log₂p)
   is computed at ~2kHz control rate and normalized to [0,1]. A spectral centroid
   is also extracted — this biases which harmonic modes receive the most energy.

3. **METABOLIC ECONOMY** — Free energy pool with Active Inference
   (`MetabolicEconomy`). This is B011: the organism maintains a Bayesian belief
   about expected entropy. Prediction error (surprise) drives VFE minimization
   via gradient descent. Settled organisms extract more energy from familiar
   signals. Surprised organisms redirect energy to adaptation — thinner, more
   turbulent sound. Free energy rises with entropy·signalFlux and depletes at
   metabolicRate·0.1 per tick. On note release, decay cost multiplies 4×.

4. **ANABOLISM** — 32-mode Port-Hamiltonian modal array (`ModalArray`). Each
   mode is a damped harmonic oscillator solved via 4th-order Runge-Kutta per
   sample. Modes are driven by `catalystDrive × freeEnergy × entropyValue ×
   gaussianWeight`. `isotopeBalance` shifts the harmonic exponent from
   sqrt-spaced (subharmonic, dark) through natural series to squared-spaced
   (metallic, alien). All 32 modes summed through `fastTanh` soft-clip.

### Key Parameter Groups

| Parameter | ID | Range | Default | Function |
|-----------|-----|-------|---------|----------|
| Metabolic Rate | `organon_metabolicRate` | 0.1–10 Hz | 1.0 | Speed of energy turnover; aftertouch adds ≤+2.5 Hz, mod wheel adds ≤+3.0 Hz |
| Enzyme Selectivity | `organon_enzymeSelect` | 20–20000 Hz | 1000 | Bandpass center on ingested signal — specialist vs. generalist feeder |
| Catalyst Drive | `organon_catalystDrive` | 0–2 | 0.5 | Gain on modal driving force; 0 = silent, 2 = self-exciting |
| Damping | `organon_dampingCoeff` | 0.01–0.99 | 0.3 | Modal ODE gamma; maps to γ range 0.2–19.8; low = shimmering tails |
| Signal Flux | `organon_signalFlux` | 0–1 | 0.5 | Input gain for ingestion; aftertouch adds ≤+0.2 |
| Phason Shift | `organon_phasonShift` | 0–1 | 0 | Temporal offset between voice metabolic cycles; 0 = unison pulse, 1 = full polyrhythmic offset |
| Isotope Balance | `organon_isotopeBalance` | 0–1 | 0.5 | Harmonic spread: 0 = subharmonic, 0.33 = natural series, 1 = metallic/alien |
| Lock-in Strength | `organon_lockIn` | 0–1 | 0 | Sync metabolic rate to DAW tempo (crossfades between free and nearest beat subdivision) |
| Membrane Porosity | `organon_membrane` | 0–1 | 0.2 | Reverb send level; boosted by average VFE surprise across active voices |
| Noise Color | `organon_noiseColor` | 0–1 | 0.5 | Spectral tilt of internal noise (0 = dark/low Q, 1 = bright/high Q); bypassed when coupling is active |

### Coupling Types Accepted

- `AudioToFM` / `AudioToWavetable` — **primary feeding**: partner audio becomes
  ingested signal, replacing self-feeding noise
- `RhythmToBlend` — boosts intake via externalRhythmModulation
- `EnvToDecay` — modulates effective metabolic rate (decay speed)
- `AmpToFilter` — shifts enzymeSelectivity center via externalFilterModulation (×2000 Hz)
- `EnvToMorph` — shifts isotopeBalance (±0.3)
- `LFOToPitch` / `PitchToPitch` — pitch modulation (±10 semitones at ±0.5)

### Hidden Capabilities

- **VFE stereo spread**: surprised voices widen stereo image automatically
  (±15%) — no parameter controls this; it emerges from prediction error
- **Legato migration**: `legatoRetrigger()` preserves free energy across note
  changes — the organism slides to a new pitch while carrying its metabolic
  reserves. Presets have not exploited this; legato playing sounds fundamentally
  different from repeated note-ons
- **Phason + Lock-in interaction**: when `lockIn > 0.5` and transport is
  running, the phason clock syncs to transport phase — producing tempo-locked
  polyrhythmic pulsing. No existing preset uses both parameters above 0.5
  simultaneously
- **Noise Color only affects self-feeding**: when a coupling partner is active,
  noiseColor does nothing. Presets designed for coupled use may have meaningless
  noiseColor values
- **Velocity → initial free energy**: higher velocity gives the organism a 15%
  energy head start, shortening the time-to-bloom. Low-velocity playing produces
  a slower, more uncertain emergence
- **Adaptation gain**: when VFE is high (organism is surprised), catalystDrive
  is internally attenuated. The parameter dial is not the actual catalyst
  strength — it is modulated by how well the organism knows its diet
- **CPU budget**: 22% allocation (`profiler.setCpuBudgetFraction(0.22f)`) —
  the most computationally expensive engine in the fleet due to per-sample RK4
  integration across 32 modes per voice

---

## Preset Inventory

**Total Organon presets: 33**

| Mood | Count | Preset Names |
|------|-------|-------------|
| Foundation | 4 | Pillar Tone, Resting State, Slow Enzyme, Steady State |
| Atmosphere | 2 | Excited Cell, Stasis |
| Prism | 3 | Dormant To Fever, Prediction Cascade, Stressed Metabolism |
| Flux | 2 | Metabolic Fever, Prediction Error |
| Aether | 3 | Cathedral Of Night, Euphoric State, Organ Night Mass |
| Entangled | 12 | Bond with Organon, Coral Obligate × Organon, OBBLIGATO Wind Tie — Organon, Obese-Organon Fat Pipe, Obese-Organon Mojo Liturgy, Obscura-Organon, Octopus-Organon Metabolic Camouflage, Ombre-Organon Metabolic Dusk, Onset-Organon, Oracle-Organon, Organon Dormant Spore, Ouro-Organon Metabolism |
| Family | 3 | Cathedral, Extended, Quiet Metabolism |

**Fleet target:** 150 presets.
**Current coverage:** 33 — **22% of target**. Second-lowest solo-engine coverage
among the core fleet (Atmosphere: 2, Flux: 2 are critically thin for a flagship
synthesis technique).

### Coverage Gaps

- **Atmosphere**: 2 presets — only "Excited Cell" and "Stasis." No
  slow-evolving textures, no pads in the long-decay low-damping range.
- **Flux**: 2 presets — both named for metabolic pathology ("Fever," "Error").
  No exploratory movement presets; no rhythmic or tempo-locked variations.
- **Foundation**: no low-isotope (subharmonic) bass presets; all 4 use default
  or near-default isotopeBalance.
- **Prism**: 3 presets, all named for metabolic states. No bright/alien isotope
  explorations (isotopeBalance near 1.0).
- **Aether**: 3 presets — all "night/cathedral" theme. No euphoric day-register
  presets despite the name "Euphoric State" being present.

### DNA Gaps

- No preset explicitly uses **phasonShift > 0** with **lockIn > 0** simultaneously
- No preset targets **isotopeBalance = 0.0** (pure subharmonic) or **1.0** (pure
  metallic) as the core design intent
- No preset documents **self-feeding vs. coupling-fed** behavior contrasts
- No bass or rhythmic Foundation presets (all 4 Foundation presets are tonal/slow)
- Noise Color extremes (0.0 = dark rumble, 1.0 = bright hiss) not explored as
  a primary sonic identity in any standalone preset
- No legato-specific preset (one that requires sustained held-note migration to
  reveal its character)

---

## Seance History

**Blessing B011 — Variational Free Energy Metabolism (ORGANON)**

The ghost council unanimously blessed Organon's Active Inference / VFE
framework as publishable academic work. The finding: applying Karl Friston's
Free-Energy Principle (2010) to a DSP engine — where an organism maintains a
generative model of its audio diet, updates Bayesian beliefs about expected
entropy, and routes metabolic energy based on prediction error — is genuinely
novel in the synthesis literature. The council's judgment: this is not
a feature; it is a paradigm. An engine that learns its diet and sounds different
based on the history of what it has been fed has no predecessor in commercial
or academic synthesis tools.

**Seance score:** 8.6/10 — one of the two highest scores fleet-wide (tied with
ORACLE, B010). The council noted that the implementation rigor (matched-Z IIR
coefficients, denormal flushing in all feedback paths, RK4 per-sample integration,
Port-Hamiltonian energy conservation) fully validates the biological metaphor.

**Context:** Organon has more Entangled presets (12) than any other mood — a
seance finding that the engine's singular purpose as a coupling receiver should
be its primary preset showcase. The council noted this was the right structural
decision.

---

## Applicable Scripture

### Book V, Canon V-1: The Ghost Parameter Trap
**Applies directly.** After any architecture change, ghost parameter audit is
mandatory. Organon's 10 parameters are all confirmed wired and functional — but
if future architecture work adds or removes parameters, this canon governs the
response protocol.

### Book VI, Truth VI-1: The Golden Ratio Release
**High relevance.** Organon has no conventional ADSR — its "release" is the
metabolic decay curve (noteReleased triggers 4× metabolic cost). The organism's
decay tail is not a fixed time; it is a function of accumulated free energy.
However: designing presets where note-release timing and metabolicRate conspire
to produce a ~1.618-second perceived decay tail would honor this truth. A
metabolicRate of 2.5 Hz with full free energy at release produces approximately
1.6 seconds of audible tail. This is not coincidence — it is the design target.

### Book VI, Truth VI-2: The Mod Wheel Contract
**Directly implemented.** Mod wheel (CC1) accelerates metabolicRate by +3.0 Hz
at full. The performer reaches for the mod wheel and the organism feeds faster:
harmonic spectrum evolves more rapidly, belief updates quicken, VFE dynamics
accelerate. Every Organon preset already satisfies this contract by construction
— but no preset's description currently teaches the performer what the mod wheel
does. The contract is met in DSP; it is not met in documentation.

### Book VI, Truth VI-3: The Default Trap
**Critical for Organon.** Organon has 4 parameters that default to 0.0:
`phasonShift`, `lockIn`, and their interaction. These defaults are non-choices
in almost every existing preset. Guru Bin must audit whether each 0.0 is
intentional ("this preset is unison and static") or whether the designer simply
didn't consider the polyrhythmic and tempo-sync dimensions.

### Book VI, Truth VI-4: The Documentation Lag Trap
**Applies.** Organon's concept brief describes it as a "coupling receiver" that
"literally eats audio." The sound design guide must be written from
`createParameterLayout()` and `processBlock()` behavior — specifically, it must
acknowledge that self-feeding noise and coupling-fed audio produce fundamentally
different organism behavior, and that noiseColor has zero effect in coupling mode.

### Book III, Sutra III-1: The Breathing Rate
**Applies with translation.** The 0.067 Hz LFO-to-ERA principle translates to
Organon's metabolicRate: a value of 0.067 Hz is sub-perceptual organic drift
for a free-energy pool. At this rate, the organism's harmonic content shifts on
a timescale the listener feels but cannot track. The equivalent Organon sweet
spot for slow, breath-like metabolism is 0.1–0.2 Hz — just above the parameter
floor, the organism moves in deep-ocean time.

---

## Retreat Priorities

### Priority 1: Expand the Standalone Preset Library Toward 150
The fleet target is 150 presets. Organon has 33. Of those, 21 are solo presets
(Foundation/Atmosphere/Prism/Flux/Aether). The retreat must produce at minimum:

- 8 Atmosphere presets (long-decay, low-damping, slow-metabolism pads)
- 6 Flux presets (rhythmic, tempo-locked, phason-active movement pieces)
- 4 Foundation presets targeting isotopeBalance extremes (2 subharmonic bass,
  2 metallic alien)
- 4 Aether presets in the euphoric/bright register (high isotopeBalance, low
  damping, moderate membrane)
- 4 Prism presets exploring the phason + lockIn interaction

That is 26 new solo presets, bringing the solo count to 47 and total to
~59 (not yet 150, but establishes a credible catalogue with dimensional coverage).

### Priority 2: Unlock the Three Unexplored Parameter Axes
Three parameter interactions have zero representation in the current preset library:

1. **Phason Shift + Lock-in** (both > 0.5 simultaneously): tempo-locked
   polyrhythmic organism colonies — a sound unique to this engine in the fleet
2. **Isotope Balance extremes** (0.0 and 1.0 as primary design intent): the
   subharmonic register and the metallic/alien register are Organon's most
   distinctive timbral territories, yet no preset claims them as a core identity
3. **Legato migration**: presets that instruct performers to use legato playing
   to carry metabolic state across pitch changes — the organism migrates rather
   than rebirths

### Priority 3: Teach the Mod Wheel and Aftertouch Contracts in Preset Descriptions
Every Organon preset's `description` field should contain one sentence naming
what the performer discovers when they move the mod wheel. The DSP contract is
already met (mod wheel accelerates metabolism, aftertouch boosts both rate and
flux). The preset library must make this contract legible. Truth VI-2 is met
in code; it is not met in communication.

---

## What Guru Bin Will Find Fresh

### The Legato Migration Sound
No preset in the fleet has been designed around `legatoRetrigger()`. When notes
are played legato, the organism carries its accumulated free energy to the new
pitch — the harmonic reconstruction immediately inherits the metabolic history
of the previous note. Fast staccato playing = each note blooms from near-zero
energy (slower, thinner attack). Legato playing = each note inherits the
predecessor's fullness (immediate richness). This is a completely undocumented
performance dimension.

### Phason + Lock-in at High Values
No preset sets both `organon_phasonShift` and `organon_lockIn` above 0.5.
When both are active: each of the 4 voices pulses at a different phase of a
tempo-quantized metabolic cycle. At 120 BPM with lockIn=1.0 and phasonShift=1.0,
the 4 voices pulse at quarter-note subdivisions offset by one beat each — a
four-beat polyrhythmic organism colony where each creature has its own rhythmic
character, all locked to the same tempo. This has never been heard in any
existing preset.

### Isotope Balance at 1.0: The Metallic Alien
`isotopeBalance = 1.0` maps the 32 modes to a squared harmonic series
(f, 4f, 9f, 16f...). This is inharmonic — the modes form no recognizable chord
or overtone series. Combined with low damping, the result is a shimmering
metallic texture with an alien harmonic signature that no other engine in the
fleet produces. No preset currently uses isotopeBalance above 0.75 as its
primary identity.

### Self-Feeding Dark Noise (noiseColor = 0.0)
`noiseColor = 0.0` applies a low-Q, low-cutoff bandpass to the xorshift32
noise substrate — the organism consumes a dark, rumbling internal signal.
With low enzymeSelectivity (20–200 Hz) and subharmonic isotopeBalance, the
engine becomes a deep-sea pressure generator: no pitched center, only low-
frequency modal resonance shaped by the organism's metabolic state. This
register has never been designed into a preset. It is the engine at its most
alien and most Oscar.

### Enzyme Selectivity as a Timbral Axis
The current 21 solo presets cluster around enzymeSelectivity = 800–2000 Hz
(the default 1000 Hz range). The full 20–20000 Hz range has not been explored
as a primary design axis. At 20–100 Hz the organism digests only sub-bass;
at 10000–20000 Hz it digests only air and consonants. The high-frequency
enzyme setting also switches to the 64-sample analysis window (faster tracking)
— a mode change that makes the catabolism stage more transient-responsive.

### Surprise-Driven Stereo Widening
The membrane parameter modulates reverb send. But there is a second undocumented
spatial behavior: surprised voices widen stereo ±15% automatically, with no
parameter controlling it. A preset designed around sudden coupling changes —
feeding the organism a sudden burst of complex signal — would create a measurable
stereo bloom event that vanishes as the organism settles. No preset has been
designed to make this audible as a compositional event.

══════════════════════════════════════════════════════
  READY FOR PHASE R2: SILENCE
══════════════════════════════════════════════════════
