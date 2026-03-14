# OBSCURA Synthesis Guide

**Engine:** OBSCURA | **Accent:** Daguerreotype Silver `#8A9BA8`
**Parameter prefix:** `obscura_` | **Max voices:** 8
**XOmnibus seance verdict:** High / unanimous — "The physics IS the synthesis."
**D003 status:** Fully compliant

---

## Introduction: The Giant Squid at the Bottom of the World

OBSCURA is the only engine in the XOmnibus fleet where the synthesis algorithm and the physical object being simulated are the same thing. When you adjust OBSCURA's stiffness, you are changing a real spring constant in a real (simulated) physical system. The sound you hear is not a metaphor for stiffness — it is what stiffness sounds like, exactly as physics dictates.

Every other synthesis method in common use — subtractive, additive, FM, wavetable — works by computing waveforms: shapes described mathematically and then transformed. OBSCURA does not do this. It simulates a chain of 128 masses connected by springs, tracks where each mass is right now and where it was one step ago, applies the laws of Newtonian mechanics to each mass at every step, and then reads the resulting chain displacement pattern as a waveform. The physics comes first. The sound emerges from the physics. There is no waveform being approximated — there is only the actual state of the physical system at this moment, read out by a scanner moving along the chain at audio rate.

This is Doctrine D003 in its purest form: the math is not overhead, it is not an approximation of something real — the math IS the instrument.

The creature identity: the giant squid. For centuries a myth, then a reality more alien than the myth. Massive, stiff, resonant — a creature whose collagen mantle is literally a pressurized resonant cavity. The 128 masses are segments of the mantle. The springs are collagen tension between segments. The stiffness parameter is the creature's muscular rigidity. OBSCURA lives in Pure Oscar territory: the abyss, the hadal zone, where sound travels differently than anywhere else.

---

## Part 1: Why Physical Modeling Changes Everything

### 1.1 The Fundamental Difference

Standard synthesis works like this: a mathematical function produces samples. An oscillator computes `sin(2π × f × t)`. An FM operator computes `sin(2π × f × t + β × sin(2π × fm × t))`. The waveform is calculated from a formula. Physical modeling works like this: differential equations describe a physical system. An integrator advances the system's state forward in time. The audio output is read from the system's state.

The distinction matters because physical systems have properties that formulas cannot emulate without enormous complexity:

**Self-consistent boundary behavior.** A fixed-boundary chain (like a guitar string anchored at both ends) automatically produces different modes than a free-boundary chain (like a rod free to vibrate at its tips) or a periodic chain (like a ring). These are not different waveshapes — they are different physical topologies with different resonance patterns. OBSCURA's boundary mode parameter (`obscura_boundary`) switches between all three. The harmonic content of Fixed, Free, and Periodic boundaries follows from the physics, not from a wavetable lookup.

**Amplitude-dependent timbre.** Real physical objects have nonlinear restoring forces at large displacements. A soft piano string hit harder does not just get louder — it gets brighter, because large displacements stretch the string and increase tension, stiffening the restoring force. OBSCURA's `obscura_nonlinear` parameter adds a cubic term to the spring force (F += k3 × dx³) that creates exactly this behavior. Higher velocities produce brighter timbres, not because a filter opened up, but because the physics of nonlinear springs produce more high-frequency content at large amplitudes.

**Emergent inharmonicity.** Stiff bars, bells, and metal plates have partials that are not integer multiples of the fundamental. This inharmonicity arises from the physical geometry of the object. In OBSCURA, inharmonicity emerges from the same spring constant that controls pitch — higher stiffness at any displacement means that higher frequency modes are stiffened proportionally less than lower modes, creating the characteristic spread of a bell or bar. You cannot achieve this with a subtractive synth without carefully pre-computing and recording every note.

**Energy physics.** The Verlet integrator used in OBSCURA is symplectic — it conserves energy over time (unlike explicit Euler integration, which gains energy and eventually blows up). This means OBSCURA's chain naturally decays: damping removes energy from the system, and without new excitation, the masses settle to rest. The physics envelope does not fade out a computed signal — it controls how much new energy is injected into the physical system each step. When the bowing force drops to zero, the chain's existing energy dissipates through damping, exactly as a real vibrating body would.

### 1.2 What Cannot Be Done Any Other Way

**True modal synthesis.** Modal synthesis means synthesizing sound by simulating the modes of a resonant object (how it "wants" to vibrate). OBSCURA does not explicitly compute modes — the modes emerge from the chain's boundary conditions and spring topology. Change the boundary mode, the stiffness, the chain length: the modes change, not because you specified them, but because the physics of the system with those parameters has those modes. This self-consistency is fundamentally different from any additive method.

**Physical coupling.** When another engine's audio is sent to OBSCURA via `AudioToFM` coupling, that audio is applied as a distributed force across the 128-mass chain — weighted by a sine function to create standing-wave patterns. The chain responds to this force as a physical object responds to being pushed. The coupling is not a mixer — it is a force acting on a resonant body. The chain's existing vibration state determines how it responds. A stiff chain responds differently to the same force than a slack chain. This is unique in the XOmnibus fleet.

**Pluck/bow duality.** The transition between a plucked response (Gaussian impulse, no sustain force) and a bowed response (continuous Gaussian force) is a parameter, not a preset. Both behaviors emerge from the same physical model — the difference is only whether energy is injected once or continuously. A real instrument cannot be changed from plucked to bowed by turning a knob; OBSCURA can, because the underlying physics supports both excitation modes.

---

## Part 2: The Physical Model — Technical Documentation

### 2.1 The Mass-Spring Chain

OBSCURA simulates a chain of 128 discrete masses (`kChainSize = 128`) connected by springs. Each mass has a displacement from its rest position. The chain state at any moment is fully described by the vector of 128 displacements.

The chain is stored as two arrays: `chain[128]` (current positions) and `chainPrevious[128]` (previous positions). Velocity is not stored explicitly — it is implicit in the difference: `velocity[i] ≈ chain[i] - chainPrevious[i]`. This is the Verlet formulation.

The number 128 is not arbitrary. It matches the default table size of Csound's `scanu` opcode (the canonical scanned synthesis implementation), and it provides a good balance between timbral resolution (more masses = richer overtones) and CPU cost (8 voices × 128 masses × physics step per block).

### 2.2 Verlet Integration

The core physics update (computed at approximately 4 kHz, or one step per ~11 audio samples) follows the Störmer–Verlet scheme:

```
x_new[i] = 2 × x[i] - x_old[i] + F[i] × dt²
```

Where F[i] is the total force on mass i and dt² is absorbed into the spring constant (normalized to 1.0). This update requires no velocity arrays — only current and previous position. The Verlet scheme is second-order accurate and, critically, symplectic: it conserves a discrete analog of the Hamiltonian energy, which means the chain does not gain energy spuriously over long simulations (unlike explicit Euler integration, which would eventually blow up).

The physics step rate of 4 kHz (`kPhysicsControlRate = 4000.0f`) was identified by Bill Verplank's original scanned synthesis research as the sweet spot for CPU efficiency and timbral fidelity. Between physics steps, the chain state is interpolated at audio rate using two snapshots (A = previous step, B = current step), with cubic Hermite (Catmull-Rom) interpolation for smooth, alias-free output.

**Stability constraint.** The Verlet scheme is only stable when the spring constant `k × dt² < 1.0`. Since dt² is normalized to 1.0, the spring constant must be less than 1.0. OBSCURA uses `kMaxSpringConstant = 0.95f` (a 5% safety margin). The `obscura_stiffness` parameter maps to `k = stiffness² × 0.95` — the quadratic mapping gives finer control at low values where the timbral change per step is most audible.

**Source:** Verlet, L. (1967). "Computer 'Experiments' on Classical Fluids." *Physical Review*, 159(1), 98–103.

### 2.3 Force Computation

For each interior mass (boundary masses are handled separately), three forces are summed:

**Linear spring force:**
```
F_spring = k × (x[i+1] - 2×x[i] + x[i-1])
```
This is the discrete Laplacian — mathematically equivalent to the finite-difference approximation of the wave equation ∂²x/∂t² = c² ∂²x/∂ξ², where ξ is position along the chain and c is wave speed. Low stiffness (k near 0): slow wave propagation, warm, fundamental-heavy tones. High stiffness (k near 0.95): fast wave propagation, brilliant metallic tones.

**Cubic nonlinear spring force:**
```
dx_right = x[i+1] - x[i]
dx_left  = x[i] - x[i-1]
F_nonlinear = k3 × (dx_right³ - dx_left³)
```
The cubic term adds amplitude-dependent stiffness. At small displacements, dx³ ≈ 0 and the chain behaves linearly. At large displacements (louder notes or stronger excitation), the cubic term dominates and stiffens the restoring force, raising the frequency of high-amplitude modes. The result is an acoustic "brightness with loudness" characteristic identical to struck bells, bars, and stiff piano strings.

**Damping force:**
```
v[i] = x[i] - x_old[i]   (velocity estimate)
F_damping = -d × v[i]
```
Viscous damping proportional to velocity. Energy is removed from the system at a rate proportional to how fast the masses are moving. `obscura_damping` maps linearly to `d ∈ [0, 0.15]`, where 0.15 is the maximum before the chain becomes overdamped and stops oscillating entirely.

### 2.4 Boundary Conditions

Boundary conditions define what happens at the endpoints of the chain (`i = 0` and `i = 127`). They fundamentally change the resonant modes of the system:

**Fixed (mode 0):** Endpoints are pinned to zero displacement.
```
x[0] = 0,  x[127] = 0
```
Like a guitar string anchored at nut and bridge. The resonant modes are integer multiples of the fundamental: f, 2f, 3f, 4f... (harmonic series). This produces the most "guitar-like" or "string-like" timbres. Default.

**Free (mode 1):** Endpoints mirror their nearest neighbor.
```
x[0] = x[1],  x[127] = x[126]
```
Like a free rod or open pipe end. The resonant modes are odd multiples of the fundamental: f, 3f, 5f, 7f... (hollow, pipe-organ character). Also models the tentacle-tips of the giant squid.

**Periodic (mode 2):** Endpoints wrap around, connecting head to tail.
```
x[0] = x[126],  x[127] = x[1]
```
Like a ring of masses. No endpoints — the chain is circular. This produces continuous modes with no distinction between "ends." Timbres are denser and more complex, with both even and odd modes equally present. The circular cross-section of the squid's mantle.

### 2.5 Excitation

OBSCURA's excitation system models two fundamentally different ways of setting a physical object in motion:

**Impulse (pluck/strike):** A Gaussian bell-curve of displacement is added to the chain at note-on. The Gaussian is centered at `obscura_excitePos` (normalized position 0–1 along the chain) with width controlled by `obscura_exciteWidth`. Narrow width: a sharp, point-like excitation (pick strike, hammer blow) that excites many modes. Wide width: a broad, gentle excitation (thumb push) that emphasizes lower modes.

```
gaussian[i] = amplitude × exp(-(i - center)² / (2σ²))
```

The Gaussian is physically motivated: no real excitation applies force at a single point. Even a piano hammer has finite width. The parameter `obscura_excitePos` controls where on the "string" the pluck occurs — a pluck near the midpoint emphasizes fundamentals; a pluck near the endpoints produces brightness (more high modes) as string players know intuitively.

**Continuous force (bow):** When `obscura_sustain` is non-zero, the physics envelope modulates a continuous Gaussian force applied to the chain at every physics step. This is the bowing force — energy is injected into the chain throughout the note, maintaining oscillation rather than letting it decay. The force amplitude is controlled by the physics envelope: the physics envelope's attack, decay, sustain, and release shape how the bowing force evolves over time independently of the amplitude envelope.

This separation of amplitude envelope from physics envelope is one of OBSCURA's most powerful features: the amplitude envelope controls how loud the note sounds to the listener; the physics envelope controls how much physical energy is injected into the resonant body.

### 2.6 The Scanner

The scanner is the mechanism that converts chain state into audio. It sweeps across the chain at the MIDI note frequency, reading each mass's displacement via cubic Hermite interpolation. The scanner's position is a phase accumulator:

```
scannerPhase += frequency / sampleRate
if scannerPhase >= 1.0: scannerPhase -= 1.0
```

At each audio sample, the scanner reads the chain at `scannerPhase × (chainSize - 1)`. The displacement at that fractional position is the audio output for that sample. The scanner sweeps at exactly the MIDI note frequency, which means it takes exactly one note period to sweep the entire chain — the chain displacement pattern becomes the waveform period-by-period.

**Scanner width** (`obscura_scanWidth`) controls how much of the chain is averaged at each read. Narrow width (< 2 masses): a single cubic Hermite interpolation at a point — maximum brightness, all detail preserved, analogous to a magnetic pickup very close to a guitar string. Wide width (up to chainSize/4 masses): a Hann-windowed average over many chain positions — dark, filtered timbre, analogous to a ribbon microphone averaging across a broad area of the source.

The Hann window (w = 0.5 - 0.5×cos(2πt)) weights the averaging so that masses near the center of the window contribute more than masses at the edges, preventing spectral leakage artifacts.

**Stereo imaging:** The left channel scans forward (0 → 1) and the right channel scans backward (1 → 0). Because the chain is never perfectly symmetric (boundary conditions, impulse position, nonlinear forces all break symmetry), the two directions see different displacement patterns and produce naturally different (but correlated) waveforms. This creates genuine phase-based stereo width without any added stereo processing.

---

## Part 3: Parameter Reference

All parameters are prefixed `obscura_`. Default values shown in brackets.

### Physics Parameters

**`obscura_stiffness`** [0.5] — Range: 0–1
Spring constant for the mass-spring chain. Maps quadratically to `k = stiffness² × 0.95`. At 0.0: the masses are nearly uncoupled, chain is floppy, sound is warm and slow-decaying with strong fundamental. At 1.0: near-maximum spring constant, chain is rigid, metallic/bell-like sound with fast wave propagation and extended high-frequency content. The quadratic mapping provides fine control in the musically interesting low-to-medium range.

**`obscura_damping`** [0.3] — Range: 0–1
Viscous damping coefficient. Maps linearly to `d ∈ [0, 0.15]`. At 0.0: no energy loss — the chain would ring indefinitely after excitation (pair with short physics envelope for realistic percussive decay). At 1.0 (d=0.15): heavy damping, chain approaches overdamped state — sound is muted, dead, like striking a heavily padded surface. Medium values (0.2–0.4) produce naturally decaying tones.

**`obscura_nonlinear`** [0.0] — Range: 0–1
Cubic spring coefficient. Maps quadratically to `k3 = nonlinear² × 0.5`. At 0.0: purely linear springs, harmonic overtone series, clean tones. Above 0.0: amplitude-dependent pitch brightening — harder notes, louder displacements, and longer bowing produce progressively brighter spectra. Creates the "stiffness" character of real metal and glass objects. This parameter affects timbre in a way that is physically impossible to replicate with a standard filter.

**`obscura_excitePos`** [0.5] — Range: 0–1
Normalized position of the Gaussian excitation impulse along the chain. 0.0 = chain head, 0.5 = midpoint, 1.0 = chain tail. Midpoint (0.5) excites all modes roughly equally. Positions near 0.0 or 1.0 (near the endpoints) emphasize higher modes (brighter attack) because the fundamental mode has a node there — this is the physical principle behind why a guitar plucked near the bridge is brighter. Also controls the position of the continuous bowing force.

**`obscura_exciteWidth`** [0.3] — Range: 0–1
Width of the Gaussian excitation. At 0.0: narrow impulse (σ = 0.5 masses) — point-like excitation, excites many modes, bright transient. At 1.0: broad impulse (σ = chainSize/4 = 32 masses) — wide excitation, suppresses high modes, smooth attack. Models the physical size of the excitation mechanism.

**`obscura_scanWidth`** [0.5] — Range: 0–1
Scanner aperture. Maps to number of chain masses averaged: `width_in_masses = 1 + scanWidth × (chainSize/4 - 1)`. At 0.0: single-point scanner (1 mass), maximum brightness — every detail in the chain displacement is audible. At 1.0: wide scanner (32 masses), Hann-windowed averaging — high-frequency content suppressed, warm, filtered timbre. Analogous to microphone placement: close-mic (narrow) vs. distant-mic (wide). Can be modulated by LFO1 for tremolo-like timbral motion.

**`obscura_boundary`** [Fixed] — Options: Fixed / Free / Periodic
Chain boundary conditions. **Fixed:** endpoints pinned at zero, integer harmonic series (string/keyboard character). **Free:** endpoints reflective, odd harmonics dominant (hollow, wind-instrument character). **Periodic:** chain forms a ring, complex dense modes, no fundamental mode emphasis (metallic, bell-like). This is a qualitative timbral character choice — Fixed for strings and marimba, Free for pipes and reeds, Periodic for bells and bowls.

**`obscura_sustain`** [0.0] — Range: 0–1
Bowing (continuous excitation) force amplitude. At 0.0: pure impulse response, sound decays after note-on. At 1.0: full bowing force applied continuously (shaped by physics envelope), chain is driven like a bowed string. Intermediate values mix plucked and bowed character. This is not the amplitude envelope's sustain — it is the physical energy source that keeps the chain vibrating.

### Envelope Parameters

OBSCURA has two independent ADSR envelopes. This is unusual and important.

**Amplitude Envelope** (`obscura_ampAttack/Decay/Sustain/Release`): Controls the volume of the output signal as heard by the listener. Standard ADSR behavior. Attack range: 0–10s. Decay: 0–10s. Sustain: 0–1. Release: 0–20s.

**Physics Envelope** (`obscura_physEnvAttack/Decay/Sustain/Release`): Controls how much bowing force is applied to the chain over time. At any moment, `excitation_force = obscura_sustain × physics_envelope_level`. This means:
- **Sustained notes:** Both envelopes at sustain phase — output level constant, bowing force constant.
- **Evolving bow pressure:** Attack the physics envelope slowly (physAttack > 0): chain starts nearly quiet and gradually builds resonance as bowing force increases, even though the amplitude envelope may already be at full level.
- **Physical decay independent of note-off:** Set physSustain low, physDecay short — the chain's physical energy dies away during the held note. Combine with high amplitude sustain: the sound gets softer not because the amplitude envelope said to, but because the physical chain has run out of energy.

The physics envelope also gates the coupling force modulation (AudioToFM coupling) — external forces are weighted by the physics envelope level, so coupling input only affects the chain when the physics envelope is active.

### LFO Parameters

Two independent LFOs per voice. Identical parameter sets:

**`obscura_lfo1Rate`** [1.0 Hz] — Range: 0.01–30 Hz
**`obscura_lfo1Depth`** [0.0] — Range: 0–1
**`obscura_lfo1Shape`** [Sine] — Options: Sine / Triangle / Saw / Square / S&H

LFO1 destination: **scan width modulation** (in mass units: depth × 8 masses). Slow LFO1 (0.01–0.5 Hz) at moderate depth creates a gentle timbral tremolo as the scanner aperture opens and closes. Fast LFO1 (5–30 Hz) adds amplitude-modulation artifacts at audio rate.

**`obscura_lfo2Rate`** [1.0 Hz] — Range: 0.01–30 Hz
**`obscura_lfo2Depth`** [0.0] — Range: 0–1
**`obscura_lfo2Shape`** [Sine] — Options: Sine / Triangle / Saw / Square / S&H

LFO2 destination: **excitation position modulation** (±0.2 along normalized chain). Modulates where bowing force is applied along the chain, shifting which modes are driven. This produces a characteristic "wobble" as the excitation sweeps different modal nodes and antinodes — unlike any LFO-to-filter path, because the spectral content doesn't just get filtered, it changes based on physical mode interaction.

**Note on S&H shape:** The Sample-and-Hold LFO uses a linear congruential generator (LCG) with Numerical Recipes constants (multiplier 1664525, increment 1013904223, mod 2³²). The RNG is seeded at voice initialization, so each note gets a deterministic pseudo-random LFO sequence based on the note number and voice counter. Identical notes sound identical; different notes sound different.

### Voice Parameters

**`obscura_voiceMode`** [Poly4] — Options: Mono / Legato / Poly4 / Poly8
Mono: single voice, new notes interrupt previous. Legato: single voice, new notes glide without envelope retrigger — a gentle secondary impulse (amplitude 0.05) re-excites the chain at the new pitch. Poly4/Poly8: up to 4 or 8 simultaneous voices, LRU voice stealing with 5ms crossfade.

**`obscura_glide`** [0.0s] — Range: 0–2s
Portamento time. Exponential frequency approach: `frequency += (target - current) × glideCoeff` each sample. Only active in Mono/Legato modes (poly mode always uses instant glide). Implemented as `glideCoeff = 1 - exp(-1 / (glideTime × sampleRate))` — this gives a portamento speed independent of the interval size.

**`obscura_initShape`** [Sine] — Options: Sine / Saw / Random / Flat
Initial chain displacement pattern before the first excitation impulse. **Sine:** half-period sine across the chain, amplitude 0.1 — fundamental-heavy initial state. **Saw:** linear ramp from -0.1 to +0.1 — spectrally richer starting point. **Random:** white noise displacement, amplitude 0.05, seeded by note number — each note starts with a unique chaotic chain shape, maximum timbral variety. **Flat:** zero displacement — pure impulse response, character determined entirely by the Gaussian impulse on the blank canvas.

### Macro Parameters

XOmnibus standard 4-macro layout, each mapped to core physical quantities:

**`obscura_macroCharacter`** [0.0] — CHARACTER
Increases stiffness (+0.3) and nonlinearity (+0.2). Turning CHARACTER up moves OBSCURA from warm to metallic, from marimba to bell, from wood to glass. This is the fastest way to shift the fundamental acoustic character of the instrument.

**`obscura_macroMovement`** [0.0] — MOVEMENT
Increases scan width (+0.3). Makes the timbral reading of the chain "blurrier" — wider scanner aperture averages out detail. Turning MOVEMENT up darkens the sound and blends harmonic content. Combine with LFO1 for animated timbral motion.

**`obscura_macroCoupling`** [0.0] — COUPLING
Increases sustain/bowing force (+0.3). Turns a dry plucked response into a bowed response. Turning COUPLING up opens the physical energy source, maintaining the chain's vibration indefinitely (subject to the physics envelope). The giant squid flexing its mantle, maintaining pressure against the deep.

**`obscura_macroSpace`** [0.0] — SPACE
Increases damping (+0.2). Damps the chain more aggressively — more energy removed per step, faster decay, "drier" resonance. Counter-intuitively, high SPACE does not add reverb; it physically removes energy from the resonant body.

**`obscura_level`** [0.8] — Output Level

---

## Part 4: Historical Context — Physical Modeling Lineage

### 4.1 Karplus-Strong (1983)

Alex Karplus and Kevin Strong published "Digital Synthesis of Plucked-String and Drum Timbres" in *Computer Music Journal* (1983). Their insight was simple: initialize a delay line with noise, then feed the output back through a two-point averaging filter. The feedback loop creates self-reinforcing modes; the delay length determines the pitch; the averaging filter damps high frequencies, simulating energy loss in a real string.

Karplus-Strong is not a physics simulation — it is a clever feedback structure that approximates the behavior of a plucked string without simulating any actual masses or springs. It is computationally cheap and sounds recognizably string-like. But it is limited: the "string" has fixed damping behavior, no nonlinearity, no boundary condition choices, and no way to add continuous excitation (bowing). OBSCURA does all of this, at higher CPU cost, because it actually simulates the physics.

### 4.2 Scanned Synthesis — Verplank and Mathews (1998)

Bill Verplank and Max Mathews at CCRMA (Center for Computer Research in Music and Acoustics, Stanford) published scanned synthesis in the late 1990s. Their key paper: Verplank, B., Mathews, M., and Shaw, R. (2000). "Scanned Synthesis." *Proceedings of the 2000 International Computer Music Conference*.

The insight was different from Karplus-Strong. Rather than approximating a string's output with a filter, Verplank and Mathews proposed actually simulating a physical object — a mass-spring chain — and reading its state with a scanner at audio rate. The "scanned" part refers to the scanner: a pointer that sweeps across the chain at the desired pitch frequency, reading displacements and producing the audio output.

The key observation: the chain's displacement pattern, read by the scanner, is not just an audio output — it evolves under the laws of physics, producing timbral changes over time that reflect the physical system's internal dynamics. The timbre is not static; it breathes, shifts, and evolves because the chain is alive.

Verplank and Mathews implemented scanned synthesis in Csound via the `scanu` (update) and `scans` (scan) opcodes. OBSCURA's architecture mirrors these directly: the 128-mass chain, the 4 kHz control rate, and the audio-rate scanner are all drawn from the original Csound implementation.

### 4.3 KTH Speech Transmission Lab — Physical Voice Models

The KTH (Kungliga Tekniska Högskolan) group in Stockholm, led by Johan Sundberg, developed physical models of the human voice starting in the 1970s. Their models treat the vocal tract as a sequence of cylindrical tubes with area functions derived from X-ray measurements of real singers. The physics of wave propagation through the tubes — transmission line theory — determines the formant structure.

KTH voice models are the reason that physical modeling can produce vowels that sound distinctly human: the formants emerge from the tube geometry, not from formant filters manually tuned by a sound designer. The resonances are what they are because the physics of that tube geometry produces those resonances.

### 4.4 IRCAM — Modalys (1987–present)

IRCAM (Institut de Recherche et Coordination Acoustique/Musique) in Paris developed Modalys, a physical modeling synthesis environment that simulates the modes of real physical objects (strings, plates, membranes, beams). The name refers to modal synthesis — the object's shape and material properties determine its resonant modes, and the synthesis works by driving and reading those modes.

Unlike OBSCURA's full wave-equation simulation, Modalys precomputes the modes from the object's geometry and simulates only the relevant modes (typically 50–200). This is computationally more efficient for complex 3D objects but does not naturally capture nonlinear behavior (which requires full-wave simulation, as OBSCURA does).

### 4.5 Yamaha VL1 (1994)

The Yamaha VL1 (Virtual Acoustic, 1994) was the first commercial physical modeling synthesizer. It implemented waveguide synthesis — Julius O. Smith's (also CCRMA) technique of modeling acoustic waveguides (tubes, strings) using bidirectional digital delay lines with scattering junctions. The VL1 modeled wind instruments (flute, oboe, brass) and plucked strings with remarkable realism for its time.

The VL1 had approximately 100 parameters, most with physical interpretations (bore shape, reed stiffness, mouth pressure, embouchure). It was notoriously difficult to program — every parameter affected the physical system in ways that could easily produce no sound, unusable noise, or unpredictable behavior. This was the first time the "physics IS the synthesis" problem was faced commercially: how do you make a physically-modeled instrument accessible to non-physicists?

OBSCURA faces the same challenge. The solution in XOmnibus is the macro layer: four macro parameters (CHARACTER, MOVEMENT, COUPLING, SPACE) each map to physically meaningful combinations of lower-level physics parameters. CHARACTER = stiffness + nonlinearity is an "acoustic hardness" control. SPACE = damping is an "acoustic liveness" control. These abstractions let players work at the level of "harder/softer" and "livelier/deader" without knowing spring constants.

---

## Part 5: Sound Design Guide — Thinking Physically

The fundamental shift required to get the most from OBSCURA: think in physical terms, not synthesis terms.

Instead of "what waveform do I want?" ask "what physical object am I trying to model, and what are its properties?"

### 5.1 Physical Properties → Parameter Mapping

**"I want a metallic bell-like tone"**
- High `obscura_stiffness` (0.7–0.9): high spring constant → fast wave propagation → inharmonic metallic modes
- `obscura_boundary` = Periodic: circular chain produces dense, complex modes
- Low/no `obscura_sustain`: bells are struck, not bowed
- Medium `obscura_damping` (0.2–0.4): bells ring for a while but not forever
- `obscura_nonlinear` > 0 (0.3–0.6): adds characteristic bell inharmonicity

**"I want a wooden marimba/bar sound"**
- Low-medium `obscura_stiffness` (0.3–0.5): softer restoring force → warmer modes
- `obscura_boundary` = Fixed: bars have fixed endpoints in their mounting
- Low `obscura_sustain`: struck, not bowed
- Very low `obscura_nonlinear` (0.0–0.1): wood is relatively linear
- Medium `obscura_excitePos` (0.3–0.5): mallet strikes near center or slightly off-center

**"I want a bowed cello-like sustained tone"**
- Medium `obscura_stiffness` (0.4–0.6): string-like
- `obscura_boundary` = Fixed: string anchored at both ends
- High `obscura_sustain` (0.5–0.8): bowing force maintained
- Set physics envelope with long attack (0.1–0.3s): bow pressure builds gradually
- `obscura_nonlinear` = 0.1–0.2: slight string nonlinearity for warmth

**"I want an alien resonant tone"**
- Very high `obscura_stiffness` (0.85–1.0): near-maximum spring constant, inharmonic modes
- `obscura_initShape` = Random: chaotic starting condition
- High `obscura_nonlinear` (0.5–0.8): strong amplitude-dependent pitch effects
- `obscura_boundary` = Periodic: circular topology, no natural string modes
- LFO2 at moderate speed (0.5–2 Hz) to scan position: shifting which modes are driven

### 5.2 The Physics Envelope as a Second Instrument

Most players treat OBSCURA's physics envelope as "attack on the bowing force," but it is much more. The physics envelope and amplitude envelope describe two independent timescales:

**Scenario: Fade-out from physics, not volume**
Set `obscura_sustain` to 0.5. Set physics envelope sustain to 0.8 and physics decay to 1.5s. Now: at note-on, bowing force ramps up quickly, chain vibrates strongly. Over 1.5 seconds, the physics envelope decays to 0.8×sustain — bowing force reduces. The sound becomes quieter not because the amplitude envelope reduced volume, but because the physical energy source weakened and the chain's natural damping is taking over. Release the note: both envelopes release simultaneously. The result: a naturally decaying physical tone, with the authentic energy physics of a real resonant object rather than a volume automation curve.

**Scenario: Energize on note-release**
Unusual technique: set physics envelope release to near-zero (0.01s) while amplitude release is long (2s). On note-release: bowing force instantly cuts off, chain resonance begins decaying naturally through damping. The amplitude envelope keeps the sound audible for 2 seconds, but the chain is now in free decay — the timbre changes over those 2 seconds in the way a real struck object changes. The first 500ms have active bowing harmonics; the last second is pure free resonance. This is only possible with two independent envelopes.

### 5.3 Stiffness and Nonlinearity Together

`obscura_stiffness` and `obscura_nonlinear` interact in physically meaningful ways:

- **Low stiffness, high nonlinearity:** The chain is floppy but nonlinear. Small displacements behave normally; large displacements get bright fast. Result: dark character with aggressive attack artifacts when struck hard. Physically similar to a rubbery membrane.
- **High stiffness, high nonlinearity:** Metallic AND amplitude-responsive. Very loud bowing or hard striking produces spectral content that quieter playing does not. Physically similar to a stiff metal bar. This is the "living bell" territory.
- **High stiffness, zero nonlinearity:** Pure linear metallic response. Each note sounds the same regardless of velocity. Classical bell-mode synthesis.
- **Low stiffness, zero nonlinearity:** Clean, warm, harmonically pure tones — closer to an idealized string with no real-world imperfection.

### 5.4 Excitation Position and Modal Nodes

On a fixed-boundary string, the fundamental mode has maximum displacement at the midpoint and zero displacement (a node) at both endpoints. Playing near the midpoint (excitePos = 0.5) drives the fundamental strongly. Playing near the endpoints (excitePos = 0.1 or 0.9) places the excitation near a node of the fundamental — the fundamental is weakly driven, and higher harmonics (which have antinodes near the endpoints) are emphasized instead.

This is exactly why a guitarist's position between the bridge and neck changes tone. OBSCURA's `obscura_excitePos` gives direct access to this effect in real time — sweep it with LFO2 to animate the spectral balance in ways no filter can replicate (because it is changing which modes are driven, not just cutting frequencies).

---

## Part 6: Sound Design Recipes

### Recipe 1: Deep Ocean Bell (Foundation mood)

Physical concept: a massive metal bell at extreme depth, pressure-distorted modes, long sustain

```
obscura_stiffness:     0.82
obscura_damping:       0.22
obscura_nonlinear:     0.45
obscura_excitePos:     0.50
obscura_exciteWidth:   0.25
obscura_scanWidth:     0.35
obscura_boundary:      Periodic
obscura_sustain:       0.0

obscura_ampAttack:     0.01s
obscura_ampDecay:      0.30s
obscura_ampSustain:    0.0
obscura_ampRelease:    4.0s

obscura_physEnvAttack:  0.01s
obscura_physEnvDecay:   0.30s
obscura_physEnvSustain: 0.0
obscura_physEnvRelease: 4.0s

obscura_initShape:     Flat
obscura_voiceMode:     Poly4
```

Physical reasoning: Periodic boundary creates circular-chain bell modes. High stiffness (0.82) places modes in metallic territory. Moderate nonlinearity (0.45) adds bell inharmonicity — partials drift from harmonic ratios exactly as they do in a real cast-metal bell. Zero sustain force means this is a pure impulse response — every note is a separate pluck of the chain. Long amplitude release (4s) lets the chain's natural resonance ring. Because amplitude sustain is 0, holding the note adds nothing — this is a per-note ringing tone, not a sustained pad.

### Recipe 2: Bowed Glass Drone (Atmosphere mood)

Physical concept: giant glass bowls played with wet fingers, sustained resonance

```
obscura_stiffness:     0.70
obscura_damping:       0.10
obscura_nonlinear:     0.20
obscura_excitePos:     0.30
obscura_exciteWidth:   0.40
obscura_scanWidth:     0.60
obscura_boundary:      Fixed
obscura_sustain:       0.65

obscura_ampAttack:     0.50s
obscura_ampDecay:      0.0s
obscura_ampSustain:    1.0
obscura_ampRelease:    1.5s

obscura_physEnvAttack:  0.80s
obscura_physEnvDecay:   0.0s
obscura_physEnvSustain: 1.0
obscura_physEnvRelease: 0.50s

obscura_lfo1Rate:      0.08 Hz
obscura_lfo1Depth:     0.25
obscura_lfo1Shape:     Sine

obscura_initShape:     Sine
obscura_voiceMode:     Poly4
```

Physical reasoning: Fixed boundary with moderate stiffness produces a string-like mode structure. Low damping (0.10) means the chain retains energy well — once bowing begins, oscillation is self-sustaining. Slow physics envelope attack (0.80s) means the bowing force builds gradually, mimicking the time a player needs to build pressure on a glass bowl. LFO1 at 0.08 Hz slowly oscillates scan width — timbral tremolo with an 12.5-second cycle, subtle and organic. Excite position at 0.30 (near a node of even harmonics) gives a slightly hollow character.

### Recipe 3: Struck Marimba Bar (Foundation mood)

Physical concept: rosewood marimba bars, short sustain, warm attack

```
obscura_stiffness:     0.38
obscura_damping:       0.35
obscura_nonlinear:     0.05
obscura_excitePos:     0.48
obscura_exciteWidth:   0.20
obscura_scanWidth:     0.45
obscura_boundary:      Fixed
obscura_sustain:       0.0

obscura_ampAttack:     0.005s
obscura_ampDecay:      0.40s
obscura_ampSustain:    0.0
obscura_ampRelease:    0.60s

obscura_physEnvAttack:  0.005s
obscura_physEnvDecay:   0.40s
obscura_physEnvSustain: 0.0
obscura_physEnvRelease: 0.50s

obscura_initShape:     Flat
obscura_voiceMode:     Poly8
```

Physical reasoning: Low stiffness (0.38) places chain modes in wood/bar territory — relatively close harmonic spacing, warm character. Higher damping (0.35) creates the characteristic short ring of wood (vs. metal's longer ring). Near-zero nonlinearity matches wood's linear restoring force. Short attack (5ms) gives the percussive snap of a mallet strike. Sustain = 0, both envelopes set to decay-only shapes. Poly8 allows keyboard rolls.

### Recipe 4: Living Alien Resonance (Flux mood — coupling showcase)

Physical concept: an object with no earthly material parallel, responding to its environment

```
obscura_stiffness:     0.88
obscura_damping:       0.15
obscura_nonlinear:     0.70
obscura_excitePos:     0.50
obscura_exciteWidth:   0.35
obscura_scanWidth:     0.50
obscura_boundary:      Periodic
obscura_sustain:       0.40

obscura_ampAttack:     0.20s
obscura_ampDecay:      0.10s
obscura_ampSustain:    0.85
obscura_ampRelease:    2.0s

obscura_physEnvAttack:  0.30s
obscura_physEnvDecay:   0.20s
obscura_physEnvSustain: 0.70
obscura_physEnvRelease: 1.0s

obscura_lfo2Rate:      0.35 Hz
obscura_lfo2Depth:     0.40
obscura_lfo2Shape:     Sine

obscura_macroCharacter: 0.50  (additional stiffness + nonlinearity)
obscura_initShape:     Random

Coupling input (from any feliX-polarity engine):
  AudioToFM → OBSCURA  (external audio strikes the chain)
  AmpToFilter → OBSCURA  (external amplitude modulates stiffness)
```

Physical reasoning: Very high stiffness + high nonlinearity = the chain is in a regime where small changes produce large timbral shifts. Periodic boundary creates complex circular modes. LFO2 at 0.35 Hz sweeps excitation position — spectral content shifts as different modal nodes and antinodes come under the bowing force 3.5 times per second. Random init shape means every note starts with a different chain pattern, so this engine sounds different every time a note is played. The coupling input transforms OBSCURA into a resonator for other engines' output — OBSCURA's chain physically responds to the other engine's audio, literally resonating in response to external forces.

### Recipe 5: Inharmonic Bowed Bar — Studying Boundary Modes

This recipe is designed for educational exploration of boundary conditions rather than a single preset. Play the same note in each mode to hear the physical difference:

**Fixed boundary (mode 0):**
Clear pitch center, ring down naturally. Modes at f, 2f, 3f, 4f... — classic harmonic series. Sounds recognizable, "string-like."

**Free boundary (mode 1):**
Same stiffness and damping, different resonance. The fundamental is weakened; odd harmonics (f, 3f, 5f...) dominate. Hollow character, slightly hollow center, like a pipe open at both ends. Octave jump sounds more pronounced.

**Periodic boundary (mode 2):**
Denser modes, no single "fundamental" dominates. The pitch is less clear, the sound more metallic and complex. Long resonance with a diffuse quality — more like a metal plate than a string or pipe.

All three from the same chain, same stiffness, same damping — only the endpoints change. This is pure physical modeling: the topology of the object determines its resonant character.

---

## Part 7: Coupling Guide

OBSCURA has a distinctive coupling personality that follows from its physical nature:

### 7.1 As a Coupling Source

OBSCURA exports post-output stereo audio via `getSampleForCoupling(channel, sampleIndex)`:
- `channel = 0`: left output
- `channel = 1`: right output
- `channel = 2`: peak amplitude envelope follower (for amplitude coupling)

As a source, OBSCURA provides harmonically complex, physically motivated audio. Its output has real resonant body — long sustains, evolving modal structure, natural-sounding decay. This pairs well as a harmonic foundation under brighter, more percussive engines.

### 7.2 As a Coupling Target

OBSCURA accepts three coupling types, each physically meaningful:

**`AudioToFM` → Force on chain masses**
External audio is applied as a spatially-varying force across the chain, weighted by a sine function:
```
force[i] = couplingForceAmount × physicsEnvelopeLevel × kCouplingForceScale
         × sin(normalizedPosition × 2π)
```
The sine weighting means the external force creates standing-wave patterns rather than uniform displacement — it excites the chain's natural modes rather than just pushing all masses in the same direction. This is like coupling sound to a resonant body: the body responds in its own way to the stimulus, with its own modal character. Pair any engine's audio with OBSCURA via AudioToFM to create a resonator effect — OBSCURA's chain vibrates sympathetically with the other engine's sound.

**`AmpToFilter` → Stiffness modulation**
External amplitude modulates the chain's spring constant:
```
effectiveStiffness += couplingAmount × 0.3
```
This literally changes the stiffness of the resonant body in real time — as the source engine gets louder, OBSCURA's chain becomes stiffer and its modes shift. Pair a pad engine's amplitude envelope with OBSCURA's AmpToFilter input: the pad's swells change the squid's body tension, morphing the timbral character of OBSCURA's output in response.

**`RhythmToBlend` → Impulse excitation**
External amplitude triggers Gaussian impulses on the chain:
```
if couplingImpulse > 0.01:
    applyImpulse(excitePosition, exciteWidth, impulse × 0.1)
```
Every drum hit or rhythmic transient from the source engine fires a new pluck on OBSCURA's chain. Pair ONSET's kick or snare with OBSCURA's RhythmToBlend input: each drum hit sets the 128-mass chain ringing, with pitch determined by the held MIDI note. This is a physically intuitive resonator: percussion transmitted through water (or the squid's body) causing the resonant object to ring.

### 7.3 Recommended Coupling Combinations

**ONSET → OBSCURA (RhythmToBlend):** Every drum hit plucks the chain. Pad OBSCURA to a note, let ONSET's rhythm drive the resonance. The kick plucks the fundamental; the snare plucks higher harmonics (if excitePos is near an endpoint).

**Any oscillator engine → OBSCURA (AudioToFM):** Use OBSCURA as a physical resonator for any engine's output. The other engine's audio drives modal patterns in the chain; OBSCURA's output contains the physically filtered version.

**OUROBOROS → OBSCURA (AmpToFilter):** The chaotic Ouroboros engine's irregular amplitude fluctuations modulate OBSCURA's stiffness. The resonant body becomes elastic, constantly changing material properties in response to chaos.

**OBSCURA → [any melody engine] (via stereo out):** OBSCURA as bass and harmonic foundation. Its physically complex output provides body and warmth under any melody engine.

---

## Part 8: Seance Record

OBSCURA's seance produced a unanimous verdict — the only engine across all 24 seances to receive unqualified praise with no doctrine violations or P0 bugs:

**Verdict:** High / unanimous — "Physics IS synthesis."

**Key ghost quote:** "The physics IS the synthesis — this is the only engine where the algorithm is the character."

**Bob Moog's contribution (D003 doctrine establishment):** "A well-tuned differential equation sings better than a sampled recording of singing."

OBSCURA's unanimous blessing was explicitly cited as the foundation for Doctrine D003: "The Physics IS the Synthesis." The seance established that physically-motivated engines do not just simulate real instruments — they ARE real physical systems, implemented in code. The rigor is not overhead; the rigor is the instrument.

**D003 compliance checklist (OBSCURA passes all):**
- Sources cited in code (Verplank/Mathews, Csound lineage, Verlet)
- Real physical constants used (Verlet stability limit, Gaussian damping coefficients)
- Empirical values documented (4 kHz control rate from Verplank's research)
- Physics envelope documented as a genuine second control axis
- Coupling inputs are physical forces, not abstract modulation

---

*XO_OX Designs | OBSCURA — the creature sings from depths no light has ever reached*
*Documentation: 2026-03-14 | Source: `Source/Engines/Obscura/ObscuraEngine.h`*
