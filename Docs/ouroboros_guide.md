# OUROBOROS — Sound Design Guide

*Chaotic Attractor Synthesis | Engine: OUROBOROS | Accent: Strange Attractor Red `#FF2D2D`*

---

## What Is OUROBOROS?

OUROBOROS generates sound by continuously solving chaotic ordinary differential equations (ODEs).
There are no oscillators, no wavetables, no samples. The audio is the trajectory of a mathematical
system through three-dimensional phase space, projected onto the stereo field via a rotation matrix.

It never repeats. Not in the sense that random noise never repeats — in the sense that a deterministic
system, fully described by its equations, produces orbits so complex they never retrace their path
exactly. Every playback is a unique passage through the same strange geometry.

**Aquatic identity:** The Hydrothermal Vent. The Abyss. Pure Oscar polarity — patience, depth,
recursion, primordial energy. The vent feeds on itself. OUROBOROS feeds on its own output.

**Seance rating:** Production-ready / Most scientifically rigorous engine in the gallery.
Praised by Don Buchla and Bob Moog (B003: Leash), and Dave Smith (B007: Velocity Coupling).

---

## The Chaos Theory Context

### What Is a Strange Attractor?

An attractor is a set of states toward which a dynamical system tends to evolve. A strange attractor
is one with fractal structure — it has non-integer dimension and its trajectories are sensitive to
initial conditions (the "butterfly effect"). These are not random. They are deterministic systems
whose long-term behavior is unpredictable in practice because tiny differences in initial state
amplify exponentially.

The key property for synthesis: the system visits every neighborhood of its attractor infinitely
often, but never repeats a path exactly. This produces audio that has stable statistical properties
(consistent timbre, consistent spectral envelope) but never becomes periodic. It is the mathematical
opposite of a wavetable.

### The Four Topologies

OUROBOROS implements four canonical chaotic attractor systems, each with its own geometry, spectral
character, and bifurcation behavior.

**Lorenz (1963)** — Edward Lorenz's atmospheric convection model. The original butterfly-effect
system. Produces the iconic double-scroll attractor: two interlocking spiral lobes that the trajectory
alternates between unpredictably. Parameter `rho` (Rayleigh number) sweeps from 20 (stable limit
cycle) to 32 (fully chaotic) as Chaos Index moves 0→1. Broad harmonic spectrum. Dense, complex,
energetic timbre. The default topology and the most immediately musical.

**Rossler (1976)** — Otto Rossler's minimal chaotic system — only one nonlinear term (`sz * sx`).
Single-scroll spiral attractor. Parameter `c` sweeps 3→18 as Chaos Index moves 0→1. Softer, more
tonal character than Lorenz. The transition from limit cycle to chaos is gradual and audibly smooth.
Good starting topology for pitched, melodic material.

**Chua (1983)** — Leon Chua's electronic circuit — the first physical circuit proven to exhibit
chaos. Uses a piecewise-linear "diode" function that creates hard nonlinearity at x = ±1. Parameter
`alpha` sweeps 9→16. Double-scroll chaos with a distinctive buzzy, metallic, electronic timbre.
Sounds like the inside of a malfunctioning oscilloscope. The most industrial of the four topologies.

**Aizawa (1982)** — Yoji Aizawa's toroidal chaotic system. Compact geometry — the attractor fits
in a much smaller phase-space volume than the others. Parameter `epsilon` sweeps 0.1→0.95 (torus to
chaotic). Spiraling, complex modulation character. Sounds like a slowly rotating mobile. The most
spatial and organic topology; its compact geometry makes it responsive to projection angle changes.

### Integration: 4th-Order Runge-Kutta at 4x Oversample

The ODEs are solved using classical 4th-order Runge-Kutta (RK4) — the standard workhorse of
numerical integration. RK4 evaluates the derivative at four points per step (start, two midpoints,
end) and combines them with 1/6:1/3:1/3:1/6 weighting, achieving O(h^5) local error. This provides
stability and accuracy that cruder methods (Euler, midpoint) cannot.

Integration runs at 4x the audio sample rate. This serves two purposes: (1) RK4 requires small step
sizes to remain stable, especially at high chaos index where trajectories are most sensitive; (2) the
chaotic signal has broadband spectral content that would alias badly if computed at 1x rate. After
integration, two cascaded 12-tap half-band FIR filters decimate 4:1 back to audio rate, suppressing
aliases above the Nyquist frequency.

---

## Blessing B003: The Leash Mechanism

### The Engineering Problem

A chaotic oscillator is, by definition, not under direct pitch control. Its trajectory is governed by
its equations, not by a MIDI note number. Naive approaches to pitch control destroy what makes the
attractor interesting:

- **Hard clipping** the output to a range breaks the attractor's topology, producing discontinuities
  that sound like digital distortion
- **Phase-locking at the integration level** (forcing the ODE to follow a reference signal) destroys
  the chaotic dynamics entirely — you get a locked oscillator with no interesting timbre
- **Pitch-shifting the output** works for audio but loses the physical relationship between pitch
  and trajectory geometry

The Leash is OUROBOROS's solution: **Phase-Locked Chaos**. It preserves both pitch accuracy and
chaotic timbral complexity simultaneously.

### How the Leash Works

The Leash mechanism runs two parallel signal paths inside each voice:

**Free-Running Path:** The attractor evolves with no external constraints. Pure mathematical chaos.
The step size `h` controls how fast the trajectory traverses the attractor (larger steps = higher
pitch), but the trajectory is unconstrained and drifts wherever the equations take it. This path
is active when `ouro_leash` < 0.99.

**Hard-Synced Path (Poincare Reset):** A master phasor runs at exactly the target MIDI frequency.
When this phasor completes one full cycle (period = 1/freq), the synced attractor's X state is
reset to a fixed point at 10% into the topology's bounding box. The Y and Z states are preserved.
This periodic reset forces the fundamental period while allowing the topology-specific structure
to dominate the harmonics. This path is active when `ouro_leash` > 0.01.

The `ouro_leash` parameter (0.0–1.0) is a linear blend between these two outputs:

```
blendedX = (1 - leash) * freeRunX + leash * syncedX
blendedY = (1 - leash) * freeRunY + leash * syncedY
blendedZ = (1 - leash) * freeRunZ + leash * syncedZ
```

Why only reset X? The attractor is most sensitive to perturbation along the X-axis — it controls
re-entry into the attractor basin. Y and Z carry the evolved timbral memory across cycles.
Resetting only X preserves the "character" of the chaos (what it has been doing harmonically)
while enforcing the fundamental period.

### Bounding Box and the Saturator

The Leash depends on per-topology **bounding boxes** — empirically derived ranges that describe
where each attractor lives in phase space:

| Topology | X range | Y range | Z range |
|----------|---------|---------|---------|
| Lorenz | −25 to +25 | −30 to +30 | 0 to +55 |
| Rossler | −12 to +12 | −12 to +12 | 0 to +25 |
| Chua | −3 to +3 | −0.5 to +0.5 | −4 to +4 |
| Aizawa | −1.5 to +1.5 | −1.5 to +1.5 | −0.5 to +2.5 |

These were measured empirically by running each system at maximum chaos index for extended periods.
At the Poincare reset, the X coordinate is placed at 10% of the X-range minimum — specifically
chosen to re-enter the attractor basin without landing at a fixed point or triggering a long
transient spiral-in.

A **Hermite cubic saturator** (soft-clip) is applied to each state variable after every RK4 step.
This prevents runaway trajectories at high chaos index or large injection forces from producing
NaN/Inf values, while preserving smooth waveform continuity (no hard clipping artifacts). The
Hermite function `f(x) = x * (1.5 - 0.5 * x^2)` has unity gain at the origin and smoothly limits
to ±1 — it is C1-continuous, meaning no derivative discontinuities that would show up as high-
frequency artifacts.

### What the Leash Sounds Like

**Leash = 0 (Free-Running):**
The attractor evolves without any pitch lock. The fundamental "frequency" drifts based on how fast
the step size drives the trajectory, but there is no guaranteed period. Sounds like pure industrial
drone — deep, unpredictable, non-repeating. Great as a texture source or chaos injection signal.
Pitch-tracking is approximate: notes still influence tempo of orbit traversal, but won't lock to
concert pitch. Useful when pitch accuracy is less important than maximum chaos density.

**Leash = 0.3–0.5 (Controlled Chaos — the sweet spot):**
The fundamental is faintly present but the chaotic overtones are wild and alive. The pitch "implies"
a note rather than stating it — like a bowed string pressed against a thermal vent pipe. The sound
has identity and placement in a mix without sacrificing the alien, never-repeating timbral complexity.
This is the zone where OUROBOROS is most musically expressive: identifiable pitch center, but the
harmonic envelope is in constant organic motion.

**Leash = 0.7–0.85 (Pitched Chaos):**
A clear fundamental with complex, evolving overtones. The pitch is reliable enough for chordal or
melodic contexts. The harmonics still shift and breathe — this is not a static waveform. Sounds
like a hybrid between a filtered oscillator and a stochastic modulator. Chord stabs from OUROBOROS
at this Leash value have recognizable pitch with an aura of unpredictability above.

**Leash = 1.0 (Hard-Synced):**
The attractor is fully periodically reset. Strong fundamental, richer harmonics than a standard
oscillator (because the chaos generates spectral content between resets), but the timbre is now
deterministic within each period. Most like a conventional synth. Still sounds unusual because
the harmonic spectrum is shaped by attractor geometry, not a wavetable. Good for situations
demanding pitch precision.

### Interaction with Chaos Index

Chaos Index and Leash interact multiplicatively:

- **High Chaos + Low Leash:** Maximum entropy. Dense, swirling, tonally ambiguous. Extreme texture.
- **High Chaos + High Leash:** The attractor is fighting its own pitch lock. Rich harmonic content
  but with moments of near-tonal resolution as the Poincare reset forces periodicity. The most
  "alive" zone — the serpent straining against its collar.
- **Low Chaos + Low Leash:** The system is near a limit cycle — periodic, smooth, sine-like. Free-
  running but almost tonal. Useful for slow-evolving pad foundations.
- **Low Chaos + High Leash:** Clean, controllable, almost like a conventional oscillator with an
  interesting spectral shape. Good for melodic applications.

The step size safety margin also tightens at high Chaos Index (30% reduction at maximum) — this
prevents RK4 divergence that would produce audible clicks or pops at the most chaotic settings.

---

## Stereo Projection: Theta and Phi

The 3D attractor trajectory (X, Y, Z state variables) must be projected to a stereo pair. OUROBOROS
uses two sequential rotation matrices: Rx(theta) then Ry(phi).

```
rotatedY = cos(theta) * Y - sin(theta) * Z
rotatedZ = sin(theta) * Y + cos(theta) * Z
L = cos(phi) * X + sin(phi) * rotatedZ
R = rotatedY
```

This means:
- **Left channel** depends on X and the Y/Z rotation via phi
- **Right channel** is the Y-axis component after theta rotation

The stereo width is **intrinsic to the attractor geometry**, not a post-process effect. Different
projection angles reveal different cross-sections of the same 3D structure — like rotating a crystal
to reveal different facets. The same chaos sounds different from different viewing angles.

**Theta = 0, Phi = 0:** Default projection. Widest stereo for Lorenz and Chua; narrower for Aizawa.
**Theta = π/4 (0.785):** Tilts Y into Z, creating a different cross-section of the spiral lobes.
**Phi = π/4:** Mixes the Z-axis (which carries orbital height information in Lorenz) into the left.
**M2 (MOVEMENT) macro:** Sweeping Theta and Phi simultaneously "walks around" the sound sculpture —
the spatial impression changes continuously, as if orbiting a sonic object in 3D space.

---

## Damping

The **damping** parameter controls a first-order lowpass accumulator applied after DC blocking:

```
accumulator = accumulator * (1 - alpha) + output * alpha
```

Where `alpha = 1 - damping * 0.95`, clamped to [0.05, 1.0].

- **Damping = 0:** alpha = 1.0. The accumulator is the output directly — no smoothing. Raw chaos
  passes through at full bandwidth. Thin, bright, grainy texture.
- **Damping = 0.5:** alpha ≈ 0.525. Medium smoothing. The fast chaotic variations are attenuated;
  only the slower orbital evolution passes through. Warm, evolving, mid-register character.
- **Damping = 1.0:** alpha = 0.05. Heavy smoothing. Only the very lowest-frequency components of
  the chaotic trajectory survive. Warm, slow-moving, almost resonant — like the heat shimmer above
  a thermal vent. Good for sub-register material when combined with a low orbit rate.

The M4 (SPACE) macro sweeps Damping and Orbit Rate together — higher damping with lower rate creates
"warm slow drift"; lower damping with higher rate creates "bright fast orbit."

Denormal protection is applied in the damping feedback path. Without it, the accumulator can decay
into subnormal floating-point territory during silence, causing 10-100x CPU spikes on x86 from
microcode assists — an inaudible value causing very audible CPU behavior.

---

## Blessing B007: Velocity Coupling Outputs

### What This Means

Every engine in the gallery exports coupling signals to other engines. Most engines export their
audio output (or a filtered/enveloped version of it). OUROBOROS exports something unique: the
**instantaneous time derivatives** of the chaotic trajectory — dx/dt and dy/dt — normalized to
[-1, 1] and scaled by MIDI velocity.

These are not the audio signal. They are the **rate of change** of the audio signal: how fast the
attractor is moving through phase space at each moment.

OUROBOROS provides four coupling output channels:
- **Channel 0:** Left audio output
- **Channel 1:** Right audio output
- **Channel 2:** Normalized dx/dt (X-axis attractor velocity)
- **Channel 3:** Normalized dy/dt (Y-axis attractor velocity)

Channels 2 and 3 are the velocity coupling outputs. The normalization constants are per-topology
(Lorenz peaks ~200 units/sec; Aizawa peaks ~10) — so the [-1, 1] output is consistent across
topologies regardless of their wildly different physical scales.

### Why dx/dt Matters as a Modulation Source

The first derivative of a chaotic signal has fundamentally different statistical properties than
the signal itself. Where the attractor position (x, y, z) tracks the orbit's location in phase
space, dx/dt tracks its **momentum** — how urgently it is moving. Near an attractor's wing tips
(extreme excursions), the trajectory is slow and dx/dt is small. Near the center of the attractor
(the "neck" between Lorenz's two lobes), the trajectory accelerates and dx/dt spikes.

This means dx/dt is a **geometrically meaningful** modulation signal that captures the attractor's
dynamical structure in a way raw position does not. It also has useful statistical properties:
it tends toward zero mean (the trajectory accelerates and decelerates symmetrically around the
attractor), and it has a different spectral profile — richer in higher-frequency content than
the position signal itself.

Channels 2 and 3 are **decorrelated** because they represent orthogonal axes of the same 3D
trajectory — useful for driving two independent parameters of a receiving engine without both
modulations doing the same thing at the same time.

### Velocity Scaling

At `noteOn`, the velocity value is stored as `voice.velocity`. During the per-sample render loop,
after the ADSR envelope is applied:

```cpp
outputLeft  *= envelopeGain * voice.velocity;
outputRight *= envelopeGain * voice.velocity;
```

The velocity also directly scales a 50ms injection boost transient at note-on:

```cpp
injectionBoost = vel * 0.5f;
```

This means harder keystrikes kick the attractor harder at onset — creating a percussive transient
proportional to velocity — and the sustained audio output is proportionally louder. The velocity
coupling outputs (dx/dt, dy/dt) carry this same velocity scaling since they are derived from the
same voice states.

The effect: **how hard you play OUROBOROS modulates other engines**. A forte attack not only makes
OUROBOROS louder, it sends a stronger modulation signal to any engine coupled on channels 2-3.
This is "unique in the gallery" — no other engine routes velocity to coupling outputs.

### Musical Applications of Velocity Coupling

**Velocity-gated filter sweep (OUROBOROS → OBLONG/ODDFELIX):**
Route ch2 or ch3 via `EnvToMorph` to a receiving engine's filter cutoff. Hard strikes brighten
the receiving engine's filter; soft touches keep it dark. The chaos in the modulation signal means
the filter doesn't track a clean envelope curve — it breathes with the attractor's dynamics.

**Velocity-driven FM intensity (OUROBOROS → ORGANON):**
ORGANON accepts `AudioToFM` coupling. Using OUROBOROS's velocity outputs as FM carriers means
the FM intensity — and therefore harmonic brightness — scales with how hard you play. Forte notes
generate complex FM spectra; piano notes keep ORGANON's texture smooth.

**Cross-engine expression (OUROBOROS → ODDOSCAR):**
Route ch2 or ch3 to ODDOSCAR's morph position via `EnvToMorph`. Your playing dynamics on
OUROBOROS slowly rotate ODDOSCAR's wavetable scan — velocity becomes a morphological controller
for the pad layer beneath the chaos.

**Chaotic LFO (OUROBOROS → any engine):**
At low orbit rates (below ~5 Hz), OUROBOROS's velocity outputs become complex, non-repeating LFO
signals. Unlike a standard LFO, these are **aperiodic** — they never exactly repeat. Route
to pitch, filter, or FX sends on any engine for permanent non-repeating modulation. The Lorenz
topology at this rate produces especially interesting behavior: long plateaus with sudden
directional reversals (the lobes of the attractor).

---

## Parameter Reference

All parameters use the `ouro_` prefix.

| ID | Display Name | Range | Default | Description |
|----|-------------|-------|---------|-------------|
| `ouro_topology` | Topology | 0–3 (int) | 0 (Lorenz) | Selects the chaotic ODE system. 0=Lorenz, 1=Rossler, 2=Chua, 3=Aizawa. Topology changes trigger a 50ms crossfade to prevent clicks. |
| `ouro_chaosIndex` | Chaos Index | 0.0–1.0 | 0.3 | Sweeps the bifurcation parameter of the selected topology from periodic orbit (0) to fully chaotic (1). This is the engine's essential dial. |
| `ouro_leash` | Leash | 0.0–1.0 | 0.5 | Blend between free-running chaos (0) and hard-synced pitch-locked chaos (1). The Poincare reset mechanism. See the Leash section above. |
| `ouro_rate` | Orbit Rate | 0.01–20000 Hz | 130 Hz | Target orbital frequency used in drone mode (no active MIDI note). When a MIDI note is playing, the note's frequency overrides this. Skew 0.3 gives finer control in the audible range. |
| `ouro_theta` | Theta | −π to +π | 0 | 3D→2D projection rotation around the X-axis (radians). Controls how Y and Z contribute to the stereo output. |
| `ouro_phi` | Phi | −π to +π | 0 | 3D→2D projection rotation around the Y-axis (radians). Controls how X and Z mix into the left channel. |
| `ouro_damping` | Damping | 0.0–1.0 | 0.3 | LP accumulator smoothing. 0 = raw chaos (thin, bright). 1 = heavy smoothing (warm, slow). Affects bandwidth of chaotic output, not pitch. |
| `ouro_injection` | Injection | 0.0–1.0 | 0.0 | Depth of external coupling audio injection into the ODE derivatives. Allows other engines to physically push the attractor's trajectory through phase space. |

### Macro Mapping

| Macro | Label | Targets | Effect |
|-------|-------|---------|--------|
| M1 | CHARACTER | `ouro_chaosIndex` + `ouro_damping` | Serene stable loop ↔ screaming torn chaos |
| M2 | MOVEMENT | `ouro_theta` + `ouro_phi` | Rotate around the sound sculpture — spatial variation |
| M3 | COUPLING | `ouro_injection` + `ouro_leash` | External perturbation depth + pitch tightness |
| M4 | SPACE | `ouro_damping` + `ouro_rate` | Warm slow drift ↔ bright fast orbit |

### Fixed Envelope

OUROBOROS has a fixed ADSR envelope built into the voice — it is not exposed as parameters because
it defines the organism's breath rather than a musician's choice:

- **Attack:** 5ms — snap of the vent opening
- **Decay:** 100ms — initial energy burst subsides
- **Sustain:** −1.9 dB (level 0.8) — sustained thermal output
- **Release:** 200ms — thermal dissipation tail

### Polyphony

6 voices maximum. Voice stealing uses LRU (oldest note stolen first) with a 5ms crossfade to
prevent clicks. Legato retrigger is supported: the attractor state is preserved across notes,
so legato passages maintain timbral continuity while changing pitch.

---

## Sound Design Recipes

### Recipe 1: Warm Atmospheric Drone

A slow-evolving texture for ambient production. Works as a pad foundation with organic movement.

```
ouro_topology    = 1 (Rossler)
ouro_chaosIndex  = 0.35
ouro_leash       = 0.0   (free-running — pitch is approximate, which is fine for drone)
ouro_rate        = 65.4  (C2 — or tune by ear)
ouro_damping     = 0.7   (heavy smoothing, warm character)
ouro_theta       = 0.4
ouro_phi         = 0.2
ouro_injection   = 0.0
```

Play a long sustained note. The Rossler topology's single-scroll geometry produces a slowly spiraling
texture. High damping removes the fast chaotic variations, leaving the slow orbital evolution.
Free-running mode at low chaos index means the system is near a limit cycle — almost tonal but
never quite locked. Automate ouro_theta over 16–32 bars for a full rotation around the sound.

### Recipe 2: Industrial Clang (Chua Fundamental)

A metallic, buzzy, industrial hit. The Chua diode's piecewise nonlinearity creates characteristic
"electronic failure" harmonics.

```
ouro_topology    = 2 (Chua)
ouro_chaosIndex  = 0.75
ouro_leash       = 0.85  (pitched enough to recognize the note)
ouro_damping     = 0.05  (minimum damping — full raw bandwidth)
ouro_theta       = 0.8
ouro_phi         = -0.5
ouro_injection   = 0.0
```

Short notes at velocity 100+. The Chua topology with high chaos and low damping exposes the diode
function's hard nonlinearity. The leash keeps pitch recognizable. Play minor 2nds or tritones for
maximum industrial dissonance. The velocity coupling outputs at these settings produce sharp spikes
that can drive filters on a layered texture engine.

### Recipe 3: Controlled Chaos Lead (Lorenz Vocal)

The "controlled chaos" sweet spot — a pitched lead line where every note has unique timbral
character that still sits in tune.

```
ouro_topology    = 0 (Lorenz)
ouro_chaosIndex  = 0.45
ouro_leash       = 0.55  (the sweet spot — pitch implied, harmonics alive)
ouro_damping     = 0.25
ouro_theta       = 0.0
ouro_phi         = 0.0
ouro_injection   = 0.0
```

Play melodies. Each note will have the same fundamental (MIDI-controlled) but slightly different
harmonic content — because the attractor's timbral state differs at each note-on depending on
where the chaos was when the key was pressed. Louder velocities create more percussive onsets.
No two performances of the same melody will sound identical.

### Recipe 4: Chaotic LFO Source (Aizawa Sub-Audio)

OUROBOROS as a modulation engine, not a sound source. Set rate sub-audio and route velocity
coupling outputs to other engines.

```
ouro_topology    = 3 (Aizawa)
ouro_chaosIndex  = 0.6
ouro_leash       = 0.0   (free-running — no pitch lock needed at sub-audio rates)
ouro_rate        = 1.2   (1.2 Hz — slow enough to feel like LFO)
ouro_damping     = 0.15
ouro_theta       = 0.0
ouro_phi         = 0.0
ouro_injection   = 0.0
```

The Aizawa attractor's compact toroidal geometry at 1.2 Hz produces a modulation signal with rich
structure — not a sine or triangle, but a never-repeating waveform with spiral sub-oscillations.
Connect ch2 → `EnvToMorph` on ODDOSCAR for non-repeating wavetable scan modulation. Connect
ch3 → `LFOToPitch` on any melodic engine for slight pitch aura. Hold a single note and the
modulation runs forever without looping.

### Recipe 5: Orbit Smash (Coupled with ODDFELIX)

ODDFELIX percussive transients physically knock the OUROBOROS attractor into new trajectories.
Each drum hit is a physical force on the chaotic orbit.

```
OUROBOROS settings:
  ouro_topology   = 0 (Lorenz)
  ouro_chaosIndex = 0.55
  ouro_leash      = 0.4
  ouro_damping    = 0.3
  ouro_injection  = 0.65  (deep injection — hits must matter)

Coupling route:
  ODDFELIX → OUROBOROS, type=AudioToFM, amount=0.8
```

ODDFELIX's percussive hits inject into OUROBOROS's dx/dt via the RK4 perturbation force. Each
kick or snare bump physically displaces the attractor in phase space — the chaos responds with a
burst of altered harmonic content that settles back into its orbit over 50–100ms. The effect: a
chaotic pad that reacts to rhythm as if being physically struck. The attractor has "memory" of the
hit in its subsequent trajectory.

---

## Coupling Guide

### What OUROBOROS Exports

| Channel | Signal | Typical Use |
|---------|--------|-------------|
| 0 | Left audio output | Standard audio coupling |
| 1 | Right audio output | Standard audio coupling |
| 2 | dx/dt (X-axis velocity), normalized [-1,1], velocity-scaled | Non-repeating complex LFO; velocity-expressive modulation |
| 3 | dy/dt (Y-axis velocity), normalized [-1,1], velocity-scaled | Decorrelated from ch2; independent modulation source |

The velocity normalization constants are per-topology: Lorenz uses 200, Rossler 30, Chua 50,
Aizawa 10. This ensures consistent [-1, 1] output across all topologies regardless of the wildly
different physical scales of their derivatives.

### What OUROBOROS Receives

| Coupling Type | Effect on OUROBOROS | Recommended Source |
|---------------|--------------------|--------------------|
| `AudioToFM` | Audio injected into dx/dt — orbit perturbation along X | ODDFELIX (percussive hits), ONSET (drum patterns) |
| `AudioToWavetable` | Audio injected into dy/dt — orthogonal orbit perturbation | ODYSSEY (envelope shapes), OBLONG (slow pads) |
| `RhythmToBlend` | Modulates Chaos Index by ±30% | ONSET (beat-synced bifurcation) |
| `EnvToDecay` | Modulates Damping (tames or unleashes chaos) | Any engine with a slow envelope |
| `AmpToFilter` | Modulates Damping via source amplitude | Amplitude-reactive warmth changes |
| `EnvToMorph` | Modulates Theta (envelope-driven harmonic rotation) | ODYSSEY (long morphing envelopes) |
| `LFOToPitch` | Modulates Orbit Rate / step size | Standard vibrato from any LFO source |
| `PitchToPitch` | Pitch-tracks Orbit Rate to source pitch | Harmonic interval tracking |

**Unsupported:** `AudioToRing` (ring mod doesn't map to ODE perturbation — the attractor IS the
waveform), `FilterToFilter` (no traditional filter chain), `AmpToChoke` (killing attractor mid-orbit
resets state, causes click), `AmpToPitch` (creates feedback loops with the Leash).

### Best Coupling Partners

| Partner | Route | Musical Effect |
|---------|-------|----------------|
| **ODDFELIX** | ODDFELIX → Ouroboros via `AudioToFM` | Percussive hits knock the orbit into new trajectories. Rhythm as physical force. |
| **ORGANON** | Ouroboros → Organon via `AudioToFM` | Chaotic audio feeds the Variational Free Energy metabolism — the organism processes chaos as food. |
| **ODYSSEY** | ODYSSEY → Ouroboros via `EnvToMorph` | Long Odyssey envelopes slowly rotate projection angle — the journey shapes the viewing angle. |
| **ODDOSCAR** | Ouroboros ch2/3 → OddOscar via `EnvToMorph` | Velocity vectors as non-repeating morph source — Oscar breathes with the chaos. |
| **ONSET** | ONSET → Ouroboros via `AudioToFM` | Drum patterns as physical forces — the snare displaces the orbit every hit. |
| **OBLONG** | Ouroboros ch2/3 → Oblong via `EnvToMorph` | Chaotic velocity vectors as non-repeating morph source for Bob's complex oscillator. |

---

## Integration Notes

**Engine ID:** `"Ouroboros"` (used in preset `"engines"` arrays and coupling routes)
**Parameter prefix:** `ouro_` (frozen — never change after release)
**Max voices:** 6
**CPU budget:** <22% single-engine. Actual measured: <2% single-engine (well under budget).
RK4 at 4x oversample with 6 voices is computationally efficient because the equations themselves
are simple — three coupled derivatives each with one or two nonlinear terms.

**Topology switching** triggers a 50ms equal-power crossfade across all active voices. The
outgoing topology state is normalized to [0,1]^3 via the bounding box; the incoming topology is
seeded from this normalized position mapped to its own coordinate system. This preserves timbral
continuity across topology changes.

**Preset count:** 20 factory presets across all 6 moods. Cross-engine coupling presets include
Orbit Smash (ODDFELIX), Chaos Metabolism (ORGANON), Manifold Drift (ODYSSEY), Velocity Morph
(ODDOSCAR), Beat Force (ONSET), Strange Bass (OBLONG).

---

## Summary: What Makes OUROBOROS Special

OUROBOROS is the only engine in the XOlokun gallery that generates audio by solving physics
equations — not by processing a waveform, not by scanning a wavetable, not by filtering noise.
The sound is the solution to a mathematical system. It has no fundamental waveform because it has
no fundamental period — the Leash creates one from above, without altering the equations below.

Two features earned unanimous praise from the gallery's founding engineers:

**B003 (Leash):** A genuine control-theory solution to pitch control on a chaotic system — not
a workaround, not an approximation, but a boundary condition that the mathematics obeys while
preserving the underlying topology. Buchla and Moog noted this because it is the correct answer
to a hard problem.

**B007 (Velocity Coupling):** The only engine in the gallery where how hard you play becomes a
cross-engine modulation signal. Your hands on the keyboard don't just change OUROBOROS's volume —
they modulate the parameters of every engine downstream. Velocity becomes systemic expression.
