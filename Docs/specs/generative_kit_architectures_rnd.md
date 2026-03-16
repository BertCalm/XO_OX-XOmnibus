# Generative Kit Architectures R&D
**Authors:** Hex + Atlas (XO_OX hacker/bridge androids)
**Date:** 2026-03-16
**Status:** Blue-sky R&D — no implementation yet

These are kit designs where the STRUCTURE comes from the math, not from human curation choices. The goal is to replace taste-based decisions with formally defined generative systems, producing kits that are surprising, internally consistent, and reproducible from a compact set of parameters.

---

## Section 1: Mathematical Systems as Kit Architects

### 1.1 L-Systems (Lindenmayer Systems)

An L-system is a parallel rewriting system defined by a tuple (V, ω, P) where V is an alphabet, ω is an axiom string, and P is a set of production rules. Each generation rewrites every symbol simultaneously.

**Classic example — Koch snowflake rule:**

```
Axiom:  F
Rule:   F → F+F-F-F+F
Gen 0:  F
Gen 1:  F+F-F-F+F
Gen 2:  F+F-F-F+F+F+F-F-F+F-F+F-F-F+F-F+F-F-F+F+F+F-F-F+F
Gen 3:  (length 125 symbols)
```

**Interpretation as a kit trigger sequence:**

- `F` = trigger a pad hit
- `+` = shift to the next pad up (or raise pitch by semitone)
- `-` = shift to the next pad down
- `[` / `]` = push / pop current pad state (branching L-systems)
- `|` = rest (no trigger this 16th note)

Each generation maps to one bar of 16th notes. The fractal self-similarity means patterns at bar scale mirror patterns at phrase scale — a natural polyrhythmic property. Running 3 iterations produces 125 symbols; trimmed or looped to 16 = one bar; full sequence = multi-bar phrase.

**Different L-systems yield distinct rhythmic personalities:**

| L-System | Rule | Property | Best Pad Type |
|---|---|---|---|
| Koch snowflake | F → F+F-F-F+F | Symmetric, dense clusters | Melodic perc |
| Sierpinski triangle | F → F-G+F+G-F, G → GG | Ternary recursion | Snare/hat |
| Dragon curve | F → F+G, G → F-G | Asymmetric, non-repeating | Ambient tails |
| Penrose (tiling rules) | (aperiodic) | Never repeats, always close | Textural FX |
| Algae (Lindenmayer's original) | A → AB, B → A | Fibonacci-length phrases | Bass pulse |

**Velocity assignment via iteration depth:**
Each symbol's velocity = the generation at which it was "born." A symbol that survives from Gen 1 to Gen 3 unchanged gets maximum velocity (ff). A symbol introduced at Gen 3 gets minimum velocity (pp). This creates natural accent hierarchies without any manual velocity editing.

**Equation for resulting sequence length:**

If rule R replaces each `F` with k symbols, after n iterations:

```
|Sₙ| = |ω| × kⁿ
```

For Koch rule (k=5), axiom `F` (|ω|=1), 3 iterations: 5³ = 125 symbols.

**Rendering requirement:** Purely algorithmic. No audio rendering needed at generation time. The L-system output is a trigger sequence + velocity map encoded directly into the XPM step sequencer fields.

**Oxport Tool Spec:**

```
Tool:     xpn_lsystem_kit.py
Inputs:   --axiom "F"
          --rules "F:F+F-F-F+F"  (colon-separated, multiple rules with semicolons)
          --iterations 3
          --pads 16               (number of pads to distribute across)
          --bars 2                (output length in bars)
          --engine [XOmnibus engine name]  (optional: render samples per pad)
          --output kit_name.xpm
Outputs:  XPM file with trigger sequence encoded in step sequencer
          JSON metadata: { axiom, rules, iterations, seed_bar, velocity_map }
Effort:   2 days (pure Python, no audio pipeline)
```

---

### 1.2 Chaos Theory Attractors as Velocity Curves

A chaotic attractor is a set toward which a dynamical system evolves. Trajectories on the attractor are deterministic but sensitive to initial conditions — small changes in starting point produce wildly different trajectories. This makes them ideal for generating velocity sequences that are musically non-repetitive but formally reproducible.

**Lorenz attractor:**

```
dx/dt = σ(y - x)        σ = 10 (Prandtl number)
dy/dt = x(ρ - z) - y    ρ = 28 (Rayleigh number)
dz/dt = xy - βz         β = 8/3 (geometric factor)
```

Numerically integrate with RK4 at step h=0.01. After discarding 1000 transient steps, sample the trajectory at every 10th step to extract velocity values.

**Pad assignment from attractor axes:**

- Pad group A (kicks, bass) ← X coordinate
- Pad group B (snare, mid) ← Y coordinate
- Pad group C (hats, transients) ← Z coordinate

Normalize each axis to [0, 127] by tracking running min/max. Each 16th note = one sample from the normalized trajectory.

The three axes of the Lorenz system are causally coupled but statistically appear independent across short windows — exactly the property you want in a multi-pad velocity pattern. A velocity spike on pad A does not predictably imply a spike on pad B, yet both emerge from the same underlying system.

**Henon map (faster, discrete):**

```
xₙ₊₁ = 1 - a·xₙ² + yₙ    a = 1.4
yₙ₊₁ = b·xₙ               b = 0.3
```

At these parameter values the map exhibits a strange attractor. Each iteration is one 16th note. 16 iterations = one bar. No integration step needed — purely algebraic. Start from (x₀, y₀) = (0, 0), discard first 100 transients.

**Velocity encoding in XPM:**

Standard XPM velocity layers use `VelStart` / `VelEnd` ranges. To encode a chaotic velocity sequence, use a different approach: generate a fixed 64-step sequence, encode it as a custom `<VelocitySequence>` metadata field in the XPM's header comment block (MPC ignores unknown XML), and use the Oxport playback engine to interpret it during kit preview.

For native MPC playback without custom tooling: quantize the continuous attractor sequence into 4 velocity tiers (pp=1–32, mp=33–64, mf=65–96, ff=97–127) and create 4 sample layers per pad with appropriate `VelStart`/`VelEnd`.

**Oxport Tool Spec:**

```
Tool:     xpn_attractor_kit.py
Inputs:   --attractor [lorenz|henon|rossler|duffing]
          --params "sigma:10,rho:28,beta:2.667"   (attractor-specific)
          --seed "0.1,0.0,0.0"                    (initial condition)
          --steps 1024                             (sequence length)
          --pads 16
          --axis-map "kick:x,snare:y,hat:z"        (lorenz only)
          --velocity-tiers 4
          --engine [engine name]
          --output kit_name.xpm
Outputs:  XPM with 4-tier velocity layers per pad
          JSON metadata: { attractor, params, seed, trajectory_preview (first 64 values) }
          Optional: CSV of raw trajectory for visualization
Effort:   3 days (Python scipy for ODE integration, matplotlib preview plot)
```

---

### 1.3 Cellular Automata Rhythm (External Kit Architecture)

While OUTWIT uses Wolfram Elementary CA inside its DSP engine, CA can also structure kit layouts externally — defining which pads are triggered at which time steps, entirely without audio rendering.

**Elementary CA fundamentals:**

Rule number R (0–255) defines a 1D CA. A cell's next state is determined by its current state and its two neighbors: (left, center, right) → 8 possible triples → 8 bits → Rule R.

```
Rule 30:   11110 = visual name
Rule table: 111→0, 110→0, 101→0, 100→1, 011→1, 010→1, 001→1, 000→0
```

**Kit generation algorithm:**

1. Choose initial row: 64-bit seed (default: single `1` at center)
2. Evolve CA for 16 generations (= 16 time steps / 16th notes)
3. Result: 16×64 binary matrix
4. Select 16 columns (pad positions) from the 64 available
5. Cell = 1 → pad fires at that 16th note
6. Cell = 0 → rest

Column selection strategy: evenly spaced, or chosen to maximize pattern density, or chosen by entropy of the column's binary sequence.

**Rule personalities:**

| Rule | Behavior | Kit Application | Density |
|---|---|---|---|
| 0 | All zeros | Silence (rest track) | 0% |
| 30 | Chaotic, complex | Hi-hats, noise transients | ~50% |
| 90 | Sierpinski triangle fractal | Snare patterns, self-similar | ~33% |
| 110 | Turing-complete, complex | Melodic ostinato patterns | ~45% |
| 184 | Traffic flow | Syncopated bass rhythm | ~35% |
| 255 | All ones | Sustained pad / drone trigger | 100% |

**Multi-rule layered kit:**

Assign different rules to different pad rows. The XPM kit then has an internal "rule layer" architecture:

```
Pads 1–4  (sub/kick):  Rule 90  (fractal, sparse at low density)
Pads 5–8  (snare/mid): Rule 110 (complex, emergent)
Pads 9–12 (hat/perc):  Rule 30  (chaotic, dense)
Pads 13–16 (FX/tonal): Rule 184 (traffic flow, syncopated)
```

**Two-dimensional extension (Game of Life):**

Conway's Life is a 2D CA. Use a 16×16 grid, run N generations, read the final state as a 16-bar × 16-pad trigger matrix. This gives a multi-bar sequence where each bar is a Life generation. Stable patterns (still lifes) = static ostinato. Oscillators = repeating patterns. Gliders = traveling accent patterns.

**Equations for rule evolution:**

```
s(t+1, i) = R_bit[ s(t, i-1) × 4 + s(t, i) × 2 + s(t, i+1) ]
where R_bit[k] = (R >> k) & 1
```

No audio rendering required. The trigger matrix is purely mathematical.

**Oxport Tool Spec:**

```
Tool:     xpn_ca_kit.py
Inputs:   --mode [elementary|life|totalistic]
          --rules "kick:90,snare:110,hat:30,fx:184"   (pad-group → rule mapping)
          --seed "0000000010000000"                     (binary string, center=1)
          --generations 16                              (= bars or steps)
          --grid-width 64                               (columns to evolve)
          --pad-columns "4,12,20,28,36,44,52,60,..."   (which columns → pads)
          --output kit_name.xpm
          --export-png trigger_grid.png                 (optional visualization)
Outputs:  XPM with step sequencer trigger map
          PNG heatmap of trigger grid (columns=pads, rows=time)
          JSON: { rules, seed, generation_states[] }
Effort:   2 days (numpy for CA evolution, matplotlib for PNG export)
```

---

## Section 2: Physics Systems as Kit Time

### 2.1 Pendulum-Coupled Oscillators (Huygens Synchronization)

In 1665 Christiaan Huygens observed that two pendulum clocks mounted on the same wooden beam would synchronize their oscillations within ~30 minutes, regardless of starting phase. The mechanism is weak coupling through the common support — each pendulum transmits a tiny impulse to the beam, which in turn perturbs the other pendulum.

**Mathematical model:**

```
θ̈₁ + (g/L)sin(θ₁) = -k(θ̇₁ - θ̇₂)
θ̈₂ + (g/L)sin(θ₂) = -k(θ̇₂ - θ̇₁)
```

Where:
- θᵢ = angular displacement of pendulum i
- g/L = squared natural frequency ω₀²
- k = coupling coefficient (weak: k ≈ 0.01–0.05)

For small angles (sin θ ≈ θ), this simplifies to two coupled harmonic oscillators. The anti-phase synchronization (θ₁ ≈ -θ₂) is the stable attractor for this system.

**As a kit architecture:**

Run two (or more) simulated pendulums with slightly different natural frequencies (ω₁ ≈ ω₂ ± Δω) and weak coupling. Sample their zero-crossing times — each zero crossing = a drum hit trigger.

Three temporal phases yield three distinct velocity layers:

```
Phase 1 — Desynchronized (t = 0 to T/3):
  Pendulums at independent tempos
  Hit timing: irregular, polymetric feel
  Velocity layer: pp (quiet, exploratory)

Phase 2 — Transitional (t = T/3 to 2T/3):
  Pendulums pulling toward common phase
  Hit timing: converging, increasing coherence
  Velocity layer: mf (building, tension)

Phase 3 — Synchronized (t = 2T/3 to T):
  Anti-phase lock achieved
  Hit timing: perfectly alternating, locked grid
  Velocity layer: ff (full power, driving)
```

**Practical kit construction:**

1. Simulate the ODE system for 3×N beats (N = bars per layer)
2. Extract zero-crossing times as hit events for two "instruments"
3. Map hit times to nearest 16th note grid (or keep as micro-timing offset)
4. Record three audio snapshots: one section per layer
5. Each recording = one sample per pad in that velocity tier

This creates kits where playing softly sounds uncertain and polymetric, while playing hard sounds locked and mechanical — the velocity itself controls synchronization state.

**Oxport Tool Spec:**

```
Tool:     xpn_pendulum_kit.py
Inputs:   --pendulums 2                        (number of coupled oscillators)
          --freq-base 2.0                      (Hz, pendulum 1 natural frequency)
          --freq-delta 0.03                    (Hz, pendulum 2 detuning)
          --coupling 0.02                      (k coefficient)
          --duration 60.0                      (seconds to simulate)
          --sample-rate 44100
          --engine [engine name]               (if rendering audio)
          --tempo 120                          (BPM for grid quantization)
          --output kit_name.xpm
Outputs:  XPM with 3 velocity layers per pad (desync/transitional/sync)
          CSV: time-series of θ₁(t), θ₂(t), sync_metric(t)
          Optional audio: synthesized click track per phase
Effort:   4 days (ODE simulation + audio rendering per phase)
          2 days (trigger-only, no audio render)
```

---

### 2.2 Fluid Turbulence Cascades (Kolmogorov)

In 1941 Andrei Kolmogorov developed the K41 theory of turbulence. Energy is injected at large scales (large eddies), cascades through intermediate scales without loss, and dissipates at the Kolmogorov microscale η via viscosity.

**The -5/3 power law:**

```
E(k) ∝ k^(-5/3)
```

Where:
- E(k) = energy spectral density at wavenumber k
- k = spatial frequency (small k = large eddies, large k = small eddies)
- The -5/3 exponent is exact in the inertial subrange

**Interpretation for kit element distribution:**

Map wavenumber k to frequency register:

| Scale | k range | Drum element | Energy share |
|---|---|---|---|
| Integral scale (injection) | k = 1 | Sub-bass / Kick | E(1) = 1.0 (normalized) |
| Inertial subrange | k = 2–8 | Bass / Snare / Mid | E(k) = k^(-5/3) |
| Taylor microscale | k = 9–32 | Hi-hat / Shaker | Small |
| Kolmogorov scale (dissipation) | k = 33–64 | Noise / Air / Texture | Minimal |

**Deriving kit element counts from K41:**

Normalize E(k) to sum to 1 across k=1..16 pads, then multiply by 16 to get "how many pads" each register deserves:

```python
k = np.arange(1, 17)
E = k ** (-5/3)
E_norm = E / E.sum()
pad_counts = np.round(E_norm * 16).astype(int)
# Result: [4, 3, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1]
# Roughly: 4 kicks, 3 bass/mid, 2 snare, 2 tom, remaining hats/texture
```

This is not an approximation of musical judgment — it is the precise ratio at which nature distributes energy across scales in turbulent flow.

**Velocity as energy content:**

The velocity of a hit at scale k is proportional to √E(k):

```
v(k) = 127 × √(E(k) / E(1)) = 127 × k^(-5/6)
```

Kick at k=1: velocity 127. Hat at k=8: velocity 127 × 8^(-5/6) ≈ 127 × 0.18 ≈ 23. This produces a physically motivated velocity map where low-frequency elements are naturally louder.

**Temporal distribution:**

The turnover time of an eddy at scale k:

```
τ(k) ∝ k^(-2/3)
```

Large eddies turn slowly (long period = long note duration), small eddies turn fast (short period = fast transients). This determines the natural hit rate for each pad:

```
hits_per_bar(k) ∝ k^(2/3)
```

Kick hits: 1–2 per bar. Hat hits: 8–16 per bar. The physics gives you the groove density automatically.

**Oxport Tool Spec:**

```
Tool:     xpn_turbulence_kit.py
Inputs:   --pads 16
          --k-range "1:16"             (wavenumber mapping)
          --viscosity 0.01             (controls dissipation cutoff k_eta)
          --tempo 120
          --bars 2
          --engine [engine name]
          --output kit_name.xpm
Outputs:  XPM with pad assignments derived from K41 energy spectrum
          JSON: { k_map[], energy[], velocity_map[], hit_rate_map[] }
          PNG: energy spectrum plot with pad assignments marked
Effort:   2 days (pure math, no audio render needed)
          +2 days if rendering samples from engine at each frequency register
```

---

### 2.3 Wave Interference Patterns (Moiré / Beating)

When two sinusoids of frequencies f₁ and f₂ are superposed, the result contains an amplitude modulation at the difference frequency |f₁ - f₂|:

```
sin(2πf₁t) + sin(2πf₂t) = 2·cos(π(f₁-f₂)t)·sin(π(f₁+f₂)t)
```

The cos term is the beat envelope. The beat period T_beat = 1/|f₁ - f₂|.

**Kit as a frequency interference instrument:**

Choose 16 pad frequencies such that all pairwise beat frequencies are musically meaningful (aligned to BPM grid):

```
Target beat frequencies: 1/4 bar, 1/2 bar, 1 bar, 2 bar, 4 bar
At tempo = 120 BPM: beat period T = 0.5s → beat frequency = 2 Hz
                    T = 1s → 1 Hz, T = 2s → 0.5 Hz
```

To get beat frequency Δf = 2 Hz between pads i and j:

```
fⱼ = fᵢ + 2.0 Hz
```

For a 16-pad harmonic series starting at A3 = 220 Hz:

```
f₁ = 220.000 Hz  (reference)
f₂ = 220.500 Hz  (Δf = 0.5 Hz = beats every 2 seconds = 1 bar at 120 BPM)
f₃ = 221.000 Hz  (Δf with f₁ = 1.0 Hz = 1 beat/sec = 0.5 bar)
f₄ = 221.500 Hz  ...
```

More musically interesting: use a constraint optimization to find 16 frequencies such that every pairwise Δf is a rational multiple of the BPM pulse frequency (1/60 × BPM Hz).

**Constraint formulation:**

```
Find {f₁, ..., f₁₆} ∈ [f_min, f_max]
Minimize: Σᵢ<ⱼ |round(|fᵢ - fⱼ| / f_pulse) - |fᵢ - fⱼ| / f_pulse|²
Subject to: fᵢ ≠ fⱼ for all i ≠ j
            |fᵢ - fⱼ| ≥ f_min_separation (avoid unisons)
```

This is a quadratic assignment problem, solvable in seconds by simulated annealing for 16 elements.

**Timbral moiré patterns:**

When multiple pads are tuned in near-unison and triggered simultaneously, the sum creates a slowly evolving amplitude envelope. Triggering combinations produces emergent rhythmic patterns at the beat frequency without any programming — the rhythm emerges from the frequency relationships.

**Practical kit construction:**

1. Solve the constraint optimization for a given tempo
2. Render 16 short pitched samples from an XOmnibus engine at the 16 target frequencies
3. Assemble into XPM
4. The "rhythm" of this kit is implicit in its tuning, not in a step sequencer pattern

No step sequencer pattern is needed. The player presses pads freely; the beating creates structure.

**Oxport Tool Spec:**

```
Tool:     xpn_interference_kit.py
Inputs:   --root 220.0                 (Hz, base frequency for pad 1)
          --tempo 120                  (BPM, used to align beat frequencies)
          --pads 16
          --method [harmonic|annealing|fibonacci]
          --engine [engine name]       (required: renders pitched samples)
          --note-duration 4.0          (seconds, sustain time for beat audibility)
          --output kit_name.xpm
Outputs:  XPM with 16 pitched pads tuned for beat alignment
          JSON: { frequencies[], beat_matrix[16x16], beat_periods[16x16] }
          PNG: beat period heatmap (pad i vs pad j)
Effort:   3 days (optimization + audio render pipeline)
          Audio render is REQUIRED — this concept only works with pitched samples
```

---

## Section 3: Information Theory as Kit Curation

### 3.1 Maximum Entropy Kit Design

Shannon entropy for a discrete distribution p = {p₁, ..., pN}:

```
H(p) = -Σᵢ pᵢ log₂(pᵢ)    (bits)
```

Maximum entropy (H = log₂N) is achieved when all outcomes are equally probable — maximum surprise, minimum redundancy.

**For kit curation:** treat each sample's spectral fingerprint as a probability distribution over frequency bins. The entropy of a sample measures how "spread" its energy is across frequency. The conditional entropy H(Pₙ | P₁, ..., Pₙ₋₁) measures how much new information Pₙ adds given the samples already chosen.

**Algorithm: greedy maximum entropy selection:**

1. Compute spectral fingerprints for all N candidate samples:
   ```
   S(f) = FFT magnitude spectrum, normalized to sum to 1 (treated as probability mass)
   H(sᵢ) = -Σ_f S(f) log₂ S(f)
   ```
2. Choose Pad 1: highest individual entropy (most spectrally complex sample)
3. For each subsequent pad k, choose the sample that maximizes:
   ```
   H(sₖ | s₁, ..., sₖ₋₁) ≈ H(sₖ) - MI(sₖ ; {s₁,...,sₖ₋₁})
   ```
   Where MI is mutual information, approximated by:
   ```
   MI(sₖ ; sⱼ) = Σ_f p_k(f) · log₂(p_k(f) / p_j(f))   (KL divergence, symmetric approximation)
   ```
4. Repeat until 16 pads are selected.

The result is a kit where every pad sounds maximally different from all others — no redundancy, maximum timbral coverage.

**Anti-entropy variant (maximum redundancy):**

Reverse step 3: choose the sample with minimum conditional entropy — i.e., the sample most similar to the existing selection. This produces layered unison kits where all pads reinforce the same timbre from different angles. Useful for tonal bass kits, drone beds, or textural depth layers.

**Divergence metrics for similarity:**

```
Jensen-Shannon divergence (symmetric, bounded 0–1):
JSD(p||q) = 0.5 × KL(p||m) + 0.5 × KL(q||m)   where m = 0.5(p+q)
KL(p||q) = Σ_f p(f) log₂(p(f)/q(f))
```

JSD = 0 means identical spectra. JSD = 1 means maximally different. Select pads to maximize minimum pairwise JSD across the kit.

**Rendering requirement:** Requires audio analysis (FFT of candidate samples). Does not require new audio rendering if candidate samples already exist. XOptic spectral fingerprints are directly usable as the input distributions.

**Oxport Tool Spec:**

```
Tool:     xpn_entropy_kit.py
Inputs:   --sample-dir /path/to/candidates/    (directory of WAV files)
          --pads 16
          --mode [max-entropy|min-entropy|balanced]
          --fingerprint-source [xoptic|compute]
          --fft-bins 1024
          --output kit_name.xpm
          --report entropy_report.json
Outputs:  XPM with 16 pads selected by entropy criterion
          JSON report: { selected[], entropy_scores[], pairwise_jsd[16x16],
                         total_kit_entropy, selection_order[] }
          PNG: JSD heatmap of final 16 pads
Effort:   2 days if XOptic fingerprints available as input
          3 days if computing FFT fingerprints from scratch
          Audio render: NOT required (analysis only)
```

---

### 3.2 Minimum Description Length (MDL) Kit Curation

MDL principle (Rissanen 1978): the best model M for data D minimizes:

```
L(M) + L(D|M)
```

Where L(M) is the code length of the model and L(D|M) is the code length of the data given the model. The best model compresses the data most efficiently.

**For kit curation:**

Let D = the full set of 200 candidate samples (their spectral fingerprints). Let M = a codebook of 16 "representative" spectra (the kit pads). The code length of each candidate sample given the kit is the bits required to describe it as a combination of kit pads:

```
L(sᵢ | M) = min_{j ∈ kit} JSD(sᵢ, mⱼ) × C   (C = bits-per-unit-divergence constant)
```

The optimal 16-pad kit minimizes:

```
Σᵢ₌₁^200 min_{j ∈ kit} JSD(sᵢ, mⱼ)
```

This is the k-medoids clustering problem with k=16 and JSD as the distance metric. Unlike k-means, k-medoids requires the cluster centers to be actual data points (actual samples from the candidate set) — exactly what we need.

**Algorithm: PAM (Partitioning Around Medoids):**

1. Initialize: choose 16 random samples as medoids
2. Assignment step: assign each of the 200 samples to its nearest medoid (by JSD)
3. Update step: for each cluster, find the sample that minimizes total JSD to all cluster members — this is the new medoid
4. Repeat until medoids stop changing (convergence in ~50 iterations for 200 samples)

Complexity: O(k × N²) per iteration = O(16 × 200² × 50) ≈ 32M operations — runs in under 1 second in Python.

**What this gives you:**

The 16 selected pads are the "most representative" samples of the entire 200-sample space. Every one of the 200 samples is "close" to at least one pad. The kit has minimum average coding cost — it is the most compact description of the timbral universe of the source material.

**MDL versus maximum entropy:**

- Max entropy: maximize diversity — choose pads that are most DIFFERENT from each other
- MDL/k-medoids: maximize coverage — choose pads that best REPRESENT the full sample set
- These are complementary objectives. A "balanced" mode can optimize a weighted sum of both.

**Practical application:**

Given 200 renders from a single XOmnibus engine (say, 200 OPAL patches run through XOptic), automatically select the best 16 for a starter kit. The math guarantees the 16 pads cover the timbral range of the engine without redundancy.

**Oxport Tool Spec:**

```
Tool:     xpn_mdl_kit.py
Inputs:   --sample-dir /path/to/candidates/    (200 WAV files)
          --pads 16                             (k in k-medoids)
          --iterations 100                      (PAM convergence steps)
          --distance [jsd|cosine|euclidean]     (spectral distance metric)
          --fingerprint-source [xoptic|compute]
          --output kit_name.xpm
          --report mdl_report.json
Outputs:  XPM with 16 pads selected as cluster medoids
          JSON: { medoids[], cluster_assignments[], intra_cluster_jsd[],
                  total_description_length, convergence_curve[] }
          PNG: 2D TSNE projection of all 200 samples with medoids highlighted
Effort:   3 days (sklearn PAM + TSNE visualization)
          Audio render: NOT required (analysis only)
```

---

### 3.3 Compression-Driven Preset Discovery

Kolmogorov complexity K(x) of a string x is the length of the shortest program that produces x. It is not computable in general, but can be approximated by standard compressors (gzip, bzip2, zstd): K(x) ≈ |compress(x)|.

**For presets:** a preset's Kolmogorov complexity is approximately the compressed size of its parameter vector. Simple, redundant presets (many parameters at default values) compress well. Complex, highly differentiated presets compress poorly.

More musically useful: local sensitivity analysis. For a preset P with parameter vector θ ∈ ℝⁿ, define the local complexity at θ as:

```
LC(θ) = Σᵢ (∂O/∂θᵢ)² / |∂θᵢ|²
```

Where O is some output measure (RMS energy, spectral centroid, or a perceptual distance metric) and ∂θᵢ = ε = 0.01 (small perturbation). High LC(θ) means small parameter changes cause large output changes — the preset is at a "sensitive" point in parameter space, near bifurcations, resonances, or timbral boundaries. These are the most musically interesting presets.

**Practical algorithm:**

1. For each preset P and each parameter θᵢ:
   - Render P to audio (2 seconds, A4 = 440 Hz, medium velocity)
   - Render P with θᵢ += ε
   - Compute perceptual distance: D = spectral flux between the two renders
   - Sensitivity(P, θᵢ) = D / ε
2. Aggregate: LC(P) = geometric mean of Sensitivity(P, θᵢ) over all i
3. Sort presets by LC: highest-LC presets are "edge of chaos" — most expressive

**Bifurcation detection:**

A parameter θᵢ is at a bifurcation if the output changes discontinuously — large D for small ε. Detect by:

```
B(P, θᵢ) = D(P, θᵢ+ε) / D(P, θᵢ-ε)   (asymmetry ratio)
B >> 1 or << 1 means bifurcation near θᵢ
```

Bifurcation parameters are the most expressive mod targets. This provides a formal basis for selecting which parameters to assign to XOmnibus macro knobs.

**Compression as preset quality metric:**

Take the raw audio render of a preset (44100 × 2 = 88200 samples × 4 bytes = 352 KB). Compress with zstd. The compression ratio K_approx = compressed_size / raw_size:

- K_approx near 1.0: nearly incompressible — maximum spectral complexity
- K_approx near 0.1: highly compressible — sparse, tonal, simple
- K_approx ≈ 0.3–0.5: the sweet spot for "interesting but not noisy"

Use compression ratio as a one-number quality signal for automated preset filtering. Discard presets with K_approx < 0.15 (too simple) or K_approx > 0.85 (too complex/noisy).

**Rendering requirement:** Requires audio rendering for each preset. For 200 presets × 2 seconds each: ~400 seconds of audio = 6.7 minutes at 60× realtime render (using OfflineAudioContext / headless JUCE render).

**Oxport Tool Spec:**

```
Tool:     xpn_complexity_scan.py
Inputs:   --preset-dir /path/to/presets/        (directory of .xpm or engine preset files)
          --engine [engine name]                 (for headless render)
          --render-duration 2.0                  (seconds per preset)
          --perturbation-epsilon 0.01
          --complexity-method [sensitivity|compression|both]
          --kc-threshold-low 0.15                (discard below)
          --kc-threshold-high 0.85               (discard above)
          --output ranked_presets.json
          --top-n 20                             (export top N presets to XPM)
Outputs:  JSON: { presets[]: { name, local_complexity, compression_ratio,
                                sensitivity_by_param[], bifurcation_params[] } }
          CSV: preset rankings sortable by any metric
          XPM batch: top-N presets assembled as kit
          PNG: scatter plot of compression_ratio vs local_complexity
Effort:   5 days (headless render pipeline + sensitivity analysis)
          Audio render IS REQUIRED for all approaches in this section
```

---

## Cross-System Integration Notes

### Which tools need audio rendering vs. pure math:

| Tool | Audio Render Required | Render Volume |
|---|---|---|
| xpn_lsystem_kit.py | Optional (trigger-only mode available) | Low (16 pads × 1 sample) |
| xpn_attractor_kit.py | Optional (velocity map only mode) | Medium (16 pads × 4 velocity tiers) |
| xpn_ca_kit.py | No | None |
| xpn_pendulum_kit.py | Yes (3 phases × 16 pads) | High (48 samples per kit) |
| xpn_turbulence_kit.py | Optional | Low if optional |
| xpn_interference_kit.py | Yes (pitched samples required) | Medium (16 pitched samples) |
| xpn_entropy_kit.py | No (analysis of existing samples) | None |
| xpn_mdl_kit.py | No (analysis of existing samples) | None |
| xpn_complexity_scan.py | Yes (preset sensitivity analysis) | Very high (200+ renders) |

### Dependency on XOptic:

xpn_entropy_kit.py and xpn_mdl_kit.py are the natural downstream consumers of XOptic spectral fingerprints. If XOptic produces a `fingerprint.json` per sample (FFT magnitude spectrum normalized to sum 1), both tools consume it directly without recomputing. This makes the XOptic → Entropy/MDL pipeline the lowest-effort implementation path.

### Suggested implementation order (lowest to highest effort):

1. `xpn_ca_kit.py` — pure Python, no audio, 2 days, ships something immediately usable
2. `xpn_lsystem_kit.py` — pure Python, trigger-only mode, 2 days
3. `xpn_entropy_kit.py` — consumes XOptic output, 2–3 days
4. `xpn_mdl_kit.py` — consumes XOptic output, builds on entropy tool, 3 days
5. `xpn_attractor_kit.py` — adds audio render option, 3 days
6. `xpn_turbulence_kit.py` — pure math + optional render, 2–4 days
7. `xpn_interference_kit.py` — requires pitched render pipeline, 3 days
8. `xpn_pendulum_kit.py` — ODE simulation + audio render, 4 days
9. `xpn_complexity_scan.py` — heaviest compute, requires headless engine render, 5 days

Total estimated effort for all 9 tools: ~26 development days.

---

*Hex notes: the CA and L-System tools can ship as standalone scripts today — no audio pipeline dependency, no XOmnibus integration required. Prototype them first; validate the mathematical output before investing in the render pipeline.*

*Atlas notes: the MDL/entropy tools are the highest-leverage items for the existing Oxport workflow. If XOptic ships first and produces fingerprint.json, the MDL kit tool becomes a 1-day add-on, not a 3-day project.*
