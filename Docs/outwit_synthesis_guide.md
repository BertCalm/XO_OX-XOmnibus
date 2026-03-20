# OUTWIT Synthesis Guide

**Engine:** OUTWIT | **Accent:** Chromatophore Amber `#CC6600`
**Parameter prefix:** `owit_` | **Arms:** 8 | **Gallery code:** OUTWIT

---

## What It Is

OUTWIT is the Giant Pacific Octopus — an 8-arm Wolfram cellular automaton synthesizer where each arm is an independent intelligence running its own Wolfram rule (0–255), producing its own pattern, controlled by its own filter and pitch, and positioned in its own place in the stereo field. The synthesis engine is the CA step — each clock tick advances all 8 automata simultaneously, generating new oscillator output based on their cellular state.

The SOLVE macro sets OUTWIT apart from every other synthesizer in the fleet: it runs a genetic algorithm that hunts for arm configurations that produce sounds matching a target Sonic DNA profile. You describe what you want (brightness, warmth, movement, density, space, aggression) and SOLVE evolves the automata toward it. This is the octopus's intelligence — not conscious, but adaptive.

The Synapse system (SYNAPSE macro) allows arm outputs to modulate other arms — the same way an octopus's distributed nervous system routes signals between its semi-autonomous arms. Chromatophore (CHROM macro) spectral-colors the output in real time, changing the timbre without changing the underlying CA rule. Den (DEN macro) is the reverb in the octopus's hiding place — the enclosed cave space where it waits and listens.

---

## The DSP Architecture

### Core: 8 ArmChannels

Each `ArmChannel` is an independent voice with:
- **Wolfram rule** (0–255): a 1D cellular automaton rule governing what the next state looks like given the current state and neighbors. Rule 110 is universal computation. Rule 30 is chaotic. Rule 90 is fractal. Rule 184 is a traffic flow model. The 8 arms default to different rules (110, 30, 90, 184, 60, 45, 150, 105).
- **Pattern length** (`owit_arm{N}Length`): 16 cells default — the working memory of the arm.
- **Oscillator wave** (`owit_arm{N}Wave`): Sine/Saw/Square/Triangle — the waveform the CA pattern modulates.
- **Pitch** (`owit_arm{N}Pitch`): Semitone offset from root. Each arm can be pitched independently to voice chord clusters or unison stacks.
- **Filter cutoff** (`owit_arm{N}Filter`): Per-arm lowpass filter for spectral shaping.
- **Level** (`owit_arm{N}Level`): Per-arm output volume.
- **Pan** (`owit_arm{N}Pan`): Default spread across the stereo field (-0.875 through +0.875) — the 8 arms naturally span the full stereo image.

### Step Rate and Timing

`owit_stepRate` sets how often the CA advances (in Hz or as a tempo-synced division when `owit_stepSync` is enabled). The `owit_stepDiv` parameter selects the tempo division (from a division table). Each step advances all 8 automata simultaneously and reads their new state as the oscillator's next cycle.

### Synapse System

`owit_synapse` controls how strongly each arm's output modulates its neighbors. At zero, 8 independent automata. As synapse increases, arm outputs route into neighbors as additional CA input — the arm's current state is perturbed by what the arms around it are doing. This is the distributed nervous system: the octopus is making decisions at the arm level, but the arms are listening to each other.

### Genetic Algorithm (SOLVE)

When `owit_solve` is above 0 and `owit_huntRate` is set, OUTWIT runs a background genetic algorithm:
- **Target:** The 6D Sonic DNA target profile set by `owit_targetBrightness`, `owit_targetWarmth`, `owit_targetMovement`, `owit_targetDensity`, `owit_targetSpace`, `owit_targetAggression`.
- **Genome:** Rule assignments for each arm (8 integers, 0–255 each).
- **Fitness:** How closely the arm's output DNA matches the target.
- **Evolution:** SOLVE mutates rule assignments and keeps improvements, gradually steering the automata toward the target sound.

The `owit_huntRate` parameter sets how aggressively the GA hunts — higher values make large jumps, lower values make fine adjustments.

### Ink Cloud

`owit_inkCloud` deploys a spectral smear that temporally blurs the output — the cephalopod defense mechanism. At zero, clean CA output. As inkCloud rises, the output is diffused over time with decaying spectral content (`owit_inkDecay` controls the decay). Useful for adding organic blur to otherwise mechanical CA patterns.

### Den Reverb

A small-room reverb modeling the enclosed cave space the octopus inhabits. `owit_denSize` sets the room size, `owit_denDecay` sets the RT60, `owit_denMix` blends wet/dry. The Den is always a small, defined space — not the open water, but the rock enclosure.

---

## The Macro System

### SOLVE (M1) — `owit_macroSolve`
Scales the genetic algorithm hunting intensity. At zero: static arm rules, no evolution. As SOLVE increases, the GA runs faster and more aggressively, hunting for rules that match the DNA target. High SOLVE during performance creates audible sonic evolution — the octopus adapting in real time. This is the most unusual macro in the fleet: it controls not a parameter but a learning process.

### SYNAPSE (M2) — `owit_macroSynapse`
Scales the inter-arm modulation routing. Low SYNAPSE: 8 independent automata. High SYNAPSE: distributed intelligence — each arm's output shapes its neighbors, creating coupled CA dynamics that no single rule could produce alone. The SYNAPSE macro is the octopus becoming a coordinated animal rather than eight separate limbs.

### CHROMATOPHORE (M3) — `owit_macroChromatophore`
Scales the spectral coloring applied to the output — changing the timbre without changing the CA rules. In real cephalopods, chromatophores are color-changing cells that fire in coordinated waves across the skin. In OUTWIT, CHROM modulates the spectral balance of all 8 arms simultaneously: wave-shaped frequency emphasis that travels across the stereo field. Low CHROM: raw CA output. High CHROM: shimmering spectral color patterns across the stereo image.

### DEN (M4) — `owit_macroDen`
Scales the Den reverb size, decay, and mix simultaneously. Low DEN: the octopus is in open water, dry. High DEN: deep in the rock cave, long resonant tail. DEN is the macro for placing OUTWIT in physical space — from exposed reef to sheltered grotto.

---

## Key Parameters

| Parameter | Range | Function |
|-----------|-------|----------|
| `owit_arm{N}Rule` | 0–255 | Wolfram CA rule for arm N (0-7) |
| `owit_arm{N}Length` | 4–32 | Cell count (pattern resolution) per arm |
| `owit_arm{N}Level` | 0–1 | Per-arm output volume |
| `owit_arm{N}Pitch` | -24–+24 | Semitone offset from root note |
| `owit_arm{N}Filter` | 200–16000Hz | Per-arm lowpass filter cutoff |
| `owit_arm{N}Wave` | 0–3 | Oscillator wave: Sine/Saw/Square/Triangle |
| `owit_arm{N}Pan` | -1–+1 | Per-arm stereo pan |
| `owit_stepRate` | 0.5–32Hz | CA step clock rate |
| `owit_stepSync` | bool | Sync step rate to host tempo |
| `owit_stepDiv` | table | Tempo division (when stepSync=true) |
| `owit_synapse` | 0–1 | Inter-arm cross-modulation strength |
| `owit_chromAmount` | 0–1 | Spectral chromatophore coloring intensity |
| `owit_solve` | 0–1 | GA hunting intensity |
| `owit_huntRate` | 0.1–1.0 | GA mutation rate / step size |
| `owit_target{X}` | 0–1 | Target DNA values: Brightness/Warmth/Movement/Density/Space/Aggression |
| `owit_inkCloud` | 0–1 | Ink cloud temporal blur level |
| `owit_inkDecay` | 0.01–0.5 | Ink cloud decay rate |
| `owit_denSize` | 0–1 | Den reverb room size |
| `owit_denDecay` | 0–1 | Den reverb RT60 |
| `owit_denMix` | 0–1 | Den reverb wet/dry |
| `owit_triggerThresh` | 0–1 | CA density gate (0=always output) |

---

## Sound Design Recipes

**The Octopus Pad** — Arm rules: default (110/30/90/184/60/45/150/105). StepRate 2Hz. Synapse 0.3. CHROM 0.4. DEN at 0.4 (small room). The 8 different rules produce 8 different rhythmic patterns that when summed and synapsed together form an organic, slowly evolving texture. Layer with OPAL for granular depth.

**Rule 110 Universal** — Set all 8 arms to Rule 110 with different pattern lengths (8, 10, 12, 14, 16, 18, 20, 22). StepRate 8Hz (fast). Synapse 0. No cross-modulation — just 8 instances of Rule 110 producing complex computational patterns at different speeds. Rule 110 is Turing-complete: it can compute anything.

**The GA Hunt** — Set target DNA: Brightness 0.8, Warmth 0.2, Movement 0.9, Density 0.3, Space 0.6, Aggression 0.7. SOLVE at maximum. HuntRate 0.6. Wait 8-16 bars and listen to the octopus evolve toward the target. Record the final arm rules when you hear the sound you want. This is OUTWIT's most unique workflow — evolution as sound design.

**The Ink Cloud** — StepRate 0.5Hz (very slow). InkCloud 0.8, InkDecay 0.15. DEN 0.6. The slow CA advances produce clear discrete state changes; the ink cloud blurs them into long, morphing drones. Pair with Obbligato or Oracle for melodic content over the blur.

**The Coordinated Flash** — StepRate 16Hz (fast, rhythmic). Synapse 0.9 (high coupling). CHROM 1.0. All arms now responding to each other — the synapse coupling at high step rates creates rhythmic spectral patterns that flash across the stereo field like coordinated chromatophore displays.

---

## Coupling Notes

OUTWIT couples outward as a distributed rhythmic/textural source — 8 independent voices each generating pattern output. Send to ONSET (arm outputs as parallel drum triggers — the octopus's pattern becomes a drum machine), OVERDUB (the dub FX chain turns CA rhythms into spatial loops), and OPAL (arm audio as grain source for granular processing of CA textures).

Receive from ORACLE (stochastic breakpoints perturb arm rules mid-sequence) and OUROBOROS (chaotic attractors modulate the step rate, making the CA evolve at an evolving speed — chaos governing computation). OUTWIT is the synthesizer most suited to Entangled presets where the coupling IS the instrument: two engines at high synapse with bidirectional coupling can produce emergent behaviors that neither engine would generate alone.
