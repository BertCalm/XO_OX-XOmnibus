# ⚠️ SUPERSEDED — XO-Oceanic Swarm Particle Concept (Abandoned)

> **This document describes an abandoned concept.** XOceanic was originally envisioned as a
> swarm particle synthesis engine using boid flocking dynamics. The actual built engine is a
> **paraphonic string ensemble with chromatophore pedalboard**. See the authoritative spec:
> `concepts/xoceanic_master_spec.md` and `concepts/xoceanic_integration_spec.md`.

**Engine:** XO-Oceanic (Swarm Particle Synthesis — NOT BUILT)
**Short Name:** OCEANIC
**Engine ID:** `"Oceanic"`
**Parameter Prefix:** `ocean_`
**Accent Color:** Phosphorescent Teal `#00B4A0`
**Max Voices:** 4 (each voice is an independent swarm of 128 particles)
**CPU Budget:** <15% single-engine, <22% dual-engine
**Date:** 2026-03-11

---

## 1. Product Identity

**Thesis:** "XO-Oceanic generates audio from the collective behavior of 128 autonomous oscillator-particles following boid flocking rules — the sound emerges from the swarm, not from any individual voice."

**Sound family:** Pad / Texture / Experimental / Drone

**Unique capability:** Every other synthesis engine is "top-down" — the system determines what each oscillator does. OCEANIC is "bottom-up" — 128 autonomous particles each follow three simple rules (separation, alignment, cohesion) operating in frequency-amplitude-pan space. The collective behavior of the swarm *is* the timbre. No two moments are identical because the swarm is a dynamic system — but the behavior is bounded, musical, and controllable through macro-level parameters.

**Personality in 3 words:** Emergent, collective, oceanic.

**Gallery gap filled:** OBESE stacks oscillators at fixed detuning intervals — deterministic architecture, predictable result. OPAL scatters grains on a schedule — top-down decomposition of existing sound. OCEANIC's particles are autonomous agents whose relationships evolve continuously. The sound is an emergent property of collective behavior — it cannot be predicted from any individual particle's state, only from the statistical dynamics of the whole flock.

---

## 2. Signal Flow Architecture

```
┌──────────────────────────────────────────────────────────────────────────┐
│                         XO-Oceanic Voice (128 Particles)                  │
│                                                                          │
│  ┌─────────────────────────────────────────────────────────────────────┐ │
│  │  MIDI + ATTRACTOR SYSTEM                                            │ │
│  │                                                                     │ │
│  │  MIDI Note ──► Target Frequency (attractor center)                  │ │
│  │  Tether param: 0 = free swarm, 1 = hard-locked to MIDI             │ │
│  │                                                                     │ │
│  │  Sub-flock attractors (1–4 independent targets):                    │ │
│  │    Flock 1: MIDI freq × 1.0 (fundamental)                          │ │
│  │    Flock 2: MIDI freq × 2.0 (octave) or custom ratio               │ │
│  │    Flock 3: MIDI freq × 1.5 (fifth) or custom ratio                │ │
│  │    Flock 4: MIDI freq × custom ratio                                │ │
│  └──────────────────────────────┬──────────────────────────────────────┘ │
│                                  │                                       │
│  ┌──────────────────────────────▼──────────────────────────────────────┐ │
│  │  BOID ENGINE (Control rate: 2–4 kHz)                                │ │
│  │                                                                     │ │
│  │  For each particle i (0 to 127):                                    │ │
│  │                                                                     │ │
│  │    SEPARATION: Repel from neighbors within radius R_sep             │ │
│  │      F_sep = Σ (pos[i] - pos[j]) / |pos[i] - pos[j]|²            │ │
│  │      (for all j where dist(i,j) < R_sep)                           │ │
│  │                                                                     │ │
│  │    ALIGNMENT: Match velocity of neighbors within radius R_align     │ │
│  │      F_align = (avg_velocity_of_neighbors - vel[i])                 │ │
│  │                                                                     │ │
│  │    COHESION: Pull toward centroid of neighbors within R_coh         │ │
│  │      F_coh = (centroid_of_neighbors - pos[i])                       │ │
│  │                                                                     │ │
│  │    ATTRACTOR: Pull toward MIDI target frequency                     │ │
│  │      F_att = tether * (target_freq - freq[i])                       │ │
│  │                                                                     │ │
│  │    DAMPING: Velocity decay                                          │ │
│  │      vel[i] *= (1.0 - damping)                                      │ │
│  │                                                                     │ │
│  │    Update:                                                          │ │
│  │      vel[i] += sep_weight*F_sep + align_weight*F_align              │ │
│  │               + coh_weight*F_coh + F_att                            │ │
│  │      pos[i] += vel[i] * dt                                          │ │
│  │                                                                     │ │
│  │  Position space: 3D (frequency_Hz, amplitude_0to1, pan_-1to1)      │ │
│  └──────────────────────────────┬──────────────────────────────────────┘ │
│                                  │                                       │
│  ┌──────────────────────────────▼──────────────────────────────────────┐ │
│  │  PARTICLE OSCILLATOR BANK (Audio rate)                              │ │
│  │                                                                     │ │
│  │  128 PolyBLEP oscillators, one per particle                         │ │
│  │  Each oscillator:                                                   │ │
│  │    frequency = particle.freq (from boid position)                   │ │
│  │    amplitude = particle.amp (from boid position)                    │ │
│  │    pan = particle.pan (from boid position)                          │ │
│  │    waveform = selectable (sine/saw/pulse/noise)                     │ │
│  │                                                                     │ │
│  │  Output = Σ all 128 oscillator outputs (L/R weighted by pan)        │ │
│  │                                                                     │ │
│  │  Amplitude normalization: output /= sqrt(active_particle_count)     │ │
│  └──────────────────────────────┬──────────────────────────────────────┘ │
│                                  │                                       │
│  ┌──────────────────────────────▼──────────────────────────────────────┐ │
│  │  PERTURBATION SYSTEM                                                │ │
│  │                                                                     │ │
│  │  Note-On Scatter: velocity impulse pushes all particles outward     │ │
│  │    scatter_force = note_velocity * scatter_param                     │ │
│  │    Each particle receives random directional impulse                 │ │
│  │    Swarm scatters then regroups (attack transient from behavior)    │ │
│  │                                                                     │ │
│  │  Murmuration Trigger: cascading reorganization wave                 │ │
│  │    One particle's large movement triggers neighbors                 │ │
│  │    Chain reaction propagates through swarm                          │ │
│  │    Produces dramatic, organic timbral shift                         │ │
│  │                                                                     │ │
│  │  Coupling Perturbation: external audio → velocity injection         │ │
│  │    External signal amplitude added to particle velocities           │ │
│  │    External audio "startles" the flock                              │ │
│  └──────────────────────────────┬──────────────────────────────────────┘ │
│                                  │                                       │
│  ┌──────────────────────────────▼──────────────────────────────────────┐ │
│  │  OUTPUT STAGE                                                       │ │
│  │                                                                     │ │
│  │  DC Blocker ──► Soft Limiter ──► Output                             │ │
│  └──────────────────────────────┬──────────────────────────────────────┘ │
│                                  │                                       │
│                                  ▼                                       │
│                        Output Cache [L, R]                               │
│                                                                          │
└──────────────────────────────────────────────────────────────────────────┘
```

### 2.1 Boid Engine — The Core

Craig Reynolds introduced the boid model in 1986 at SIGGRAPH, demonstrating that three simple rules — separation, alignment, cohesion — applied to autonomous agents produce flocking behavior indistinguishable from real birds. OCEANIC applies these rules to oscillator-particles in a 3D space where the dimensions are frequency, amplitude, and stereo pan position.

**Particle state:**

```cpp
struct Particle
{
    float freq;      // Hz (20–20000, log-mapped)
    float amp;       // 0.0–1.0
    float pan;       // -1.0 (L) to +1.0 (R)

    float vFreq;     // Frequency velocity (Hz/sec)
    float vAmp;      // Amplitude velocity (/sec)
    float vPan;      // Pan velocity (/sec)

    int subFlock;    // Sub-flock assignment (0–3)
    float phase;     // Oscillator phase accumulator
};
```

**Force computation (per particle, per control tick):**

The three boid forces operate in the 3D space of (freq, amp, pan). Distance between particles is computed in this normalized space:

```cpp
float distance(const Particle& a, const Particle& b)
{
    float df = (logf(a.freq) - logf(b.freq)) / logf(20000.0f / 20.0f);  // Normalized log-freq
    float da = a.amp - b.amp;
    float dp = a.pan - b.pan;
    return std::sqrt(df * df + da * da + dp * dp);
}
```

Frequency is compared in log-space (so an octave is the same "distance" at any frequency), ensuring perceptually uniform spacing.

**Spatial hashing for O(N) neighbor lookup:**

Naive boid computation is O(N²) per tick (every particle checks every other). With 128 particles at 2–4 kHz, this is prohibitive. OCEANIC uses spatial hashing:

```cpp
// 3D grid: 8×4×4 = 128 cells
// Each cell contains a linked list of particles
// Neighbor search checks only the 27 surrounding cells (3×3×3)
// Average cost: O(N) instead of O(N²)
```

This reduces per-tick cost from ~16,384 distance calculations to ~3,000–5,000.

### 2.2 Sub-Flock System

Particles are assigned to 1–4 sub-flocks. Each sub-flock has:
- An independent frequency attractor (configurable ratio to MIDI note)
- Independent boid rule weights (one sub-flock can be tight while another is dispersed)
- Particles only flock with members of their own sub-flock (separation, alignment, cohesion computed within sub-flock)
- The MIDI attractor still applies globally (all sub-flocks orbit the note, but at different ratios)

**Musical effect:**
- 1 sub-flock: unified swarm → thick unison pad, massive shimmer
- 2 sub-flocks at 1:2 ratio: octave separation → bass + treble clouds orbiting the same note
- 3 sub-flocks at 1:1.5:2: root + fifth + octave → harmonic cloud
- 4 sub-flocks at custom ratios: chord tones → the swarm plays harmony

### 2.3 Perturbation System

**Note-On Scatter:** When a note is triggered, every particle receives a random velocity impulse proportional to MIDI velocity × scatter parameter. This explodes the swarm outward from its current formation. The boid rules then pull the particles back together — the *regrouping process* creates the attack transient. Higher scatter = more dramatic attack. Lower scatter = gentle onset.

This is fundamentally different from envelope-based attacks: the attack shape is *emergent* from the boid dynamics, not prescribed by an ADSR. Two identical notes will have slightly different attacks because the initial scatter directions are random.

**Murmuration Trigger:** Inspired by starling murmurations — when one bird makes a sudden turn, its neighbors react, their neighbors react to them, and a wave of motion cascades through the entire flock. In OCEANIC, a murmuration trigger fires a large velocity impulse at a single particle. That particle's sudden movement pushes its neighbors (via separation force), who push their neighbors, creating a cascading reorganization wave that propagates through the swarm over ~50–200ms.

Musical effect: a dramatic, organic timbral shift that unfolds over time — not instantaneous like a parameter change, but propagating like a physical wave through the swarm. Maps beautifully to the MOVEMENT macro.

**Coupling Perturbation:** External audio from the MegaCouplingMatrix is analyzed for amplitude and applied as velocity perturbation to particle positions. Loud external audio "startles" the flock — ONSET drum hits cause the swarm to scatter and regroup. OBSIDIAN crystal tones create periodic perturbation that locks the swarm into resonant patterns.

### 2.4 Particle Oscillator Bank

Each of the 128 particles drives a PolyBLEP oscillator (band-limited alias-free synthesis):

```cpp
float processParticleBank(Particle* particles, int N, float sampleRate, int waveform)
{
    float sumL = 0.0f, sumR = 0.0f;

    for (int i = 0; i < N; ++i)
    {
        // Update phase
        float phaseInc = particles[i].freq / sampleRate;
        particles[i].phase += phaseInc;
        if (particles[i].phase >= 1.0f) particles[i].phase -= 1.0f;

        // Generate sample (PolyBLEP)
        float sample = 0.0f;
        switch (waveform)
        {
            case 0: sample = sinf(2.0f * M_PI * particles[i].phase); break;
            case 1: sample = polyBLEPSaw(particles[i].phase, phaseInc); break;
            case 2: sample = polyBLEPPulse(particles[i].phase, phaseInc, 0.5f); break;
            case 3: sample = whiteNoise(); break;
        }

        // Apply particle amplitude and pan
        float amp = particles[i].amp;
        float panL = 0.5f * (1.0f - particles[i].pan);
        float panR = 0.5f * (1.0f + particles[i].pan);

        sumL += sample * amp * panL;
        sumR += sample * amp * panR;
    }

    // Normalize
    float norm = 1.0f / std::sqrt(static_cast<float>(N));
    return sumL * norm;  // (similarly for R)
}
```

The normalization by `1/sqrt(N)` ensures the output level remains consistent regardless of how many particles are active, while preserving the density perception of more particles.

---

## 3. Parameter Taxonomy

### 3.1 Core Parameters (8)

| ID | Parameter | Range | Curve | Rate | Description |
|----|-----------|-------|-------|------|-------------|
| `ocean_separation` | Separation | 0.0–1.0 | Linear | Control | Frequency-space repulsion strength. Low = particles cluster (unison). High = particles spread (wide spectrum). CHARACTER macro target. |
| `ocean_alignment` | Alignment | 0.0–1.0 | Linear | Control | Velocity-matching strength. High = particles stream together (directional spectral motion). Low = random individual movement. |
| `ocean_cohesion` | Cohesion | 0.0–1.0 | Linear | Control | Pull toward swarm centroid. High = tight formation. Low = dispersed cloud. |
| `ocean_tether` | Tether | 0.0–1.0 | Linear | Control | Anchor strength to MIDI pitch. 0 = free swarm (atonal). 1 = hard-locked to note. Sweet spot: 0.3–0.6. COUPLING macro target. |
| `ocean_scatter` | Scatter | 0.0–1.0 | Exponential | Control | Note-on perturbation intensity. 0 = gentle onset. 1 = explosive scatter. |
| `ocean_subflocks` | Sub-Flocks | 1–4 | Stepped | Control | Number of independent sub-swarms. 1 = unified. 4 = harmonic cloud. |
| `ocean_damping` | Damping | 0.0–1.0 | Exponential | Control | Velocity decay rate. Low = hyperactive swarm. High = sluggish, heavy swarm. MOVEMENT macro target. |
| `ocean_waveform` | Waveform | 0–3 | Stepped | Control | Particle oscillator shape. Sine (0), Saw (1), Pulse (2), Noise (3). SPACE macro target (morphs from pure sine to noisy). |

### 3.2 Macro Mapping

| Macro | Primary Target | Secondary Target | Musical Effect |
|-------|---------------|-----------------|----------------|
| CHARACTER (M1) | `ocean_separation` | `ocean_cohesion` (inverse) | Tight unison cluster → wide dispersed cloud |
| MOVEMENT (M2) | `ocean_damping` (inverse) | Murmuration trigger threshold | Sluggish heavy swarm → hyperactive, self-reorganizing flock |
| COUPLING (M3) | `ocean_tether` | Coupling input gain | Free atonal swarm → locked to MIDI + external engine influence |
| SPACE (M4) | `ocean_waveform` | `ocean_subflocks` + reverb send | Pure sine unison → noisy multi-flock cloud with space |

### 3.3 Envelope & Modulation Parameters

| ID | Parameter | Type | Description |
|----|-----------|------|-------------|
| `ocean_ampAttack` | Amp Attack | Time | 0ms–10s (note: attack shape also influenced by scatter) |
| `ocean_ampDecay` | Amp Decay | Time | 0ms–10s |
| `ocean_ampSustain` | Amp Sustain | Level | 0–1 |
| `ocean_ampRelease` | Amp Release | Time | 0ms–20s |
| `ocean_swarmEnvAttack` | Swarm Env Attack | Time | Controls boid rule intensity over note duration |
| `ocean_swarmEnvDecay` | Swarm Env Decay | Time | |
| `ocean_swarmEnvSustain` | Swarm Env Sustain | Level | |
| `ocean_swarmEnvRelease` | Swarm Env Release | Time | |
| `ocean_lfo1Rate` | LFO 1 Rate | Hz | 0.01–30 Hz |
| `ocean_lfo1Depth` | LFO 1 Depth | Level | |
| `ocean_lfo1Shape` | LFO 1 Shape | Enum | Sine / Triangle / Saw / Square / S&H |
| `ocean_lfo2Rate` | LFO 2 Rate | Hz | 0.01–30 Hz |
| `ocean_lfo2Depth` | LFO 2 Depth | Level | |
| `ocean_lfo2Shape` | LFO 2 Shape | Enum | |

### 3.4 Voice Parameters

| ID | Parameter | Description |
|----|-----------|-------------|
| `ocean_voiceMode` | Voice Mode | Mono / Poly2 / Poly4 |
| `ocean_glide` | Glide Time | Portamento — attractor position slides (0–2s) |
| `ocean_flockRatios` | Sub-Flock Ratios | Frequency ratios for sub-flocks 2–4 (e.g., 2.0, 1.5, 3.0) |

---

## 4. The Ghosts in OCEANIC

### Ghost 1: The Swarmatron (Dewanatron, 2004) — The Analog Flock

**The instrument:** Leon and Brian Dewan — brothers, inventors, and artists operating as Dewanatron in the Hudson Valley, New York — created the Swarmatron: eight analog oscillators with swarming behavior. Each oscillator's pitch is influenced by its neighbors through analog coupling circuits. The player controls the spread between oscillators with one knob and the coupling strength with another. The oscillators don't behave independently — they influence each other, creating emergent pitch relationships that shift and evolve.

The Swarmatron was adopted by Trent Reznor and Alessandro Cortini (Nine Inch Nails), who used it extensively on the *The Social Network* and *Gone Girl* soundtracks. Its sound — sometimes a tight, beating unison, sometimes a dispersing cloud of tones — proved that flocking oscillators produce timbres no other method can create.

But the Swarmatron has fundamental limitations: 8 oscillators (not enough for true emergent behavior), analog coupling (no boid rules, just nearest-neighbor influence), and no independent sub-flocks. It demonstrated the *concept* of swarm synthesis without having the computational resources to realize it fully.

**How it lives in OCEANIC:** OCEANIC is the Swarmatron realized at full scale. 128 particles (vs. 8), true 3D boid rules (vs. simple analog coupling), sub-flock system (vs. one unified group), MIDI pitch tethering (vs. manual tuning), and perturbation system (vs. static behavior). The Swarmatron proved that swarm synthesis works musically. OCEANIC extends it from demonstration to instrument.

### Ghost 2: Verbos Harmonic Oscillator (Mark Verbos, 2014) — The Harmonic Cloud

**The instrument:** Mark Verbos's eurorack Harmonic Oscillator generates a fundamental plus harmonics 2–8, each with independent amplitude control (eight sliders). By manipulating the sliders in real time, the player sculpts the harmonic spectrum directly — adding or removing individual harmonics to morph the timbre continuously.

The Verbos HO demonstrated that giving a performer direct control over many simultaneous oscillators produces extraordinary timbral flexibility. But its harmonics are fixed in frequency relationship — they don't move independently. The harmonic series is static; only the amplitudes change.

**How it lives in OCEANIC:** OCEANIC removes the "fixed frequency" constraint. Instead of harmonics locked to integer multiples of the fundamental, OCEANIC's particles are free agents — they *tend* toward harmonic relationships (via the tether parameter) but can deviate, wander, and regroup. The Verbos HO is an orchestra reading a score; OCEANIC is the same orchestra improvising within shared rules.

### Ghost 3: Cristián Vogel's Buchla Flock Patches (2000s) — The Unrepeatable Performance

**The performer:** Cristián Vogel, a Chilean-British electronic musician and one of the most technically sophisticated Buchla 200e performers, developed a series of patches in the mid-2000s where multiple oscillators were cross-patched through complex feedback networks. The oscillators influenced each other's pitch, amplitude, and timbre through nonlinear feedback paths, creating emergent "flocking" behavior — the oscillators would synchronize, scatter, orbit each other, and reorganize without any external modulation.

These patches were unrepeatable. Each performance was unique because the oscillator interactions were chaotic and state-dependent — the same patch, started from the same initial conditions, would evolve differently each time. Vogel described performing with them as "conducting a flock — you can influence the direction, but you can't dictate the path."

**How it lives in OCEANIC:** Vogel's Buchla patches proved that emergent oscillator behavior produces compelling live performance material — sounds that surprise even the performer. OCEANIC makes this repeatable and controllable through parameterized boid rules while preserving the emergent quality. The `scatter` and murmuration system give the performer Vogel's "conductor" role — influence without dictation.

---

## 5. The Cultural Lens: Salegy + Kompa + Chutney Soca

### Salegy (Madagascar)

Salegy is the dominant popular music of Madagascar — a fast, polyrhythmic style built on interlocking patterns where each musician plays a simple independent figure. The accordion plays a repeating riff. The electric guitar plays a complementary riff. The bass locks to the root. Multiple percussion instruments play independent rhythmic cells. No conductor coordinates them — each player follows local rules:

1. **Separation:** Stay in your own frequency and rhythmic space (don't step on other instruments)
2. **Alignment:** Match the general tempo direction (speed up or slow down with the group)
3. **Cohesion:** Stay near the tonal center (all parts orbit the same key)

The resulting 6/8 polyrhythm is an emergent phenomenon — it exists in the collective, not in any individual part. This is boid dynamics in acoustic form. The tsapiky variant from southern Madagascar makes this even more explicit — up to 15 musicians interlocking without written arrangement, the groove emerging from collective listening.

### Kompa (Haiti)

Kompa (also konpa) layers tanbou drums, guitar skank, bass ostinato, keyboard pads, and horn stabs into the gouyad groove — a slow, rolling, hip-driven feel. Each layer follows autonomous rules:

- The tanbou establishes the rhythmic floor
- The guitar skank plays off-beats, avoiding the tanbou's accents
- The bass locks to the harmonic root, moving only at chord changes
- The horns punctuate, filling gaps left by the other instruments

Kompa musicians describe the gouyad as something that "arrives" during performance — it's not played by any individual, it emerges from the interaction of all parts. The groove is an emergent property of the flock.

### Chutney Soca (Trinidad & Tobago)

Chutney Soca is a cultural swarm — two autonomous musical traditions coexisting in the same space:

- **Indian musical DNA:** dholak rhythms, tassa drumming, film-song melodic patterns, devotional singing traditions
- **Afro-Caribbean musical DNA:** soca rhythm section, calypso melodic contour, steelpan harmonic vocabulary, carnival energy

Neither tradition dissolves into the other. Both maintain identity — their separation parameter is high. But alignment (shared groove, shared key center) and cohesion (pull toward the song structure) create a hybrid that belongs to neither parent culture. Chutney Soca is boid dynamics with two sub-flocks: distinct groups that flock among themselves while sharing a global attractor.

---

## 6. XOceanus Integration

### 6.1 MegaCouplingMatrix Compatibility

**Emits:**
- `SWARM_DENSITY` — Number of particles within a frequency band around the MIDI note (normalized 0–1). Indicates how "converged" the swarm is. High density = tight unison. Low density = dispersed cloud.
- `SWARM_CENTROID` — The frequency centroid of the swarm (in Hz) as a continuous modulation signal. Broadcasts the swarm's collective "pitch" which may differ from the MIDI note when tether is low.
- `SWARM_VELOCITY` — Average particle velocity magnitude (normalized 0–1). Indicates how "active" the swarm is. High velocity = rapid movement. Low velocity = settled formation.

**Accepts:**
- `AudioToFM` — External audio converted to velocity perturbation on particles. External sound "startles" the flock.
- `AmpToFilter` — External amplitude modulates cohesion strength. Loud = tight swarm. Quiet = dispersed.
- `RhythmToBlend` — External rhythm triggers murmuration events. Each beat causes a cascading reorganization.
- `PITCH_GRAVITY` (from XOntara) — Topological pitch gravity modulates sub-flock attractor positions.

### 6.2 PlaySurface Interaction Model

**Pad Mode:**
- X-axis: `ocean_separation` — Left = tight cluster, Right = wide spread
- Y-axis: `ocean_cohesion` — Bottom = dispersed, Top = tight
- Pressure (Z): `ocean_scatter` — Hard press triggers scatter perturbation

**Fretless Mode:**
- X-axis: Continuous pitch (attractor frequency)
- Y-axis: `ocean_tether` — Slide up = stronger MIDI lock
- Pressure (Z): Murmuration trigger (hard press starts cascading reorganization)

**Drum Mode:**
- X-axis: Pad assignment by sub-flock count × waveform (8 pads)
- Y-axis: `ocean_separation` — Vertical = spread
- Pressure (Z): Scatter intensity (velocity = scatter force)

---

## 7. Preset Archetypes

### 7.1 Murmuration
`separation=0.3, alignment=0.7, cohesion=0.5, tether=0.5, scatter=0.4, subflocks=1, damping=0.3, waveform=Sine`

Unified sine swarm with moderate tether. Particles orbit the MIDI note in a shimmering cloud, occasionally scattering and regrouping. High alignment creates directional streaming — the swarm moves *through* the frequency space rather than randomly jittering. Warm, evolving, alive.

### 7.2 Reef
`separation=0.6, alignment=0.3, cohesion=0.4, tether=0.7, scatter=0.2, subflocks=3, damping=0.5, waveform=Saw`

Three saw-wave sub-flocks at root/fifth/octave. Moderate separation keeps the sub-flocks distinct. Tether locks them near the harmonic ratios. The three saw clouds create a rich, harmonically complex chord tone — like an organ where each "rank" is a living, breathing flock rather than a fixed set of pipes. The coral reef: many organisms, one structure.

### 7.3 Tsunami
`separation=0.8, alignment=0.9, cohesion=0.2, tether=0.2, scatter=0.9, subflocks=1, damping=0.1, waveform=Saw`

Maximum scatter, high alignment, low cohesion and tether. Note-on explodes the swarm across the entire frequency range. High alignment means all particles stream in the same direction — a wave of sound sweeping across the spectrum. Very low damping means the swarm takes a long time to settle. Cinematic, overwhelming, catastrophic.

### 7.4 Gouyad
`separation=0.4, alignment=0.5, cohesion=0.6, tether=0.6, scatter=0.3, subflocks=2, damping=0.6, waveform=Pulse`

Two pulse-wave sub-flocks (root + octave) with moderate parameters. The high damping creates a sluggish, heavy swarm — the "gouyad" rolling feel. The two sub-flocks orbit independently but stay near their harmonic targets, creating a thick, groovy tone with subtle internal motion. The sound of Kompa made literal.

### 7.5 Plankton Bloom
`separation=0.2, alignment=0.2, cohesion=0.8, tether=0.4, scatter=0.1, subflocks=4, damping=0.4, waveform=Noise`

Four noise-wave sub-flocks with very high cohesion — each sub-flock is a tight cluster of noise particles. Low separation means clusters can overlap. The four tight noise clouds at different frequency ratios create a dense, evolving textural drone — like the sudden density increase of a plankton bloom. Gentle scatter means note-on barely perturbs the formation — the texture emerges slowly from the boid dynamics.

---

## 8. CPU Analysis

### 8.1 Boid Engine Cost (Control Rate)

| Component | Operations per Tick | Notes |
|-----------|--------------------|-------|
| Spatial hash rebuild (128 particles) | ~640 ops | Position quantization + insertion |
| Neighbor search (128 particles × ~20 neighbors avg) | ~7680 distance calcs | Spatial hashing reduces from 16384 |
| Force computation (128 × 3 forces) | ~1920 multiply + ~1536 add | Separation + alignment + cohesion |
| Attractor force (128 particles) | 128 × 2 ops | Simple subtraction + scale |
| Velocity update (128 particles × 3 dims) | 384 multiply + 384 add | vel += force * weight |
| Position update (128 particles × 3 dims) | 384 multiply + 384 add | pos += vel * dt |
| **Total per tick** | **~13,000 ops** | |

At 2 kHz control rate: ~26M ops/sec per voice.

### 8.2 Oscillator Bank Cost (Audio Rate)

| Component | Operations per Sample | Notes |
|-----------|----------------------|-------|
| Phase accumulate (128 oscillators) | 128 add | |
| PolyBLEP saw (128 oscillators) | 128 × 6 ops | PolyBLEP correction |
| Amplitude + pan (128 oscillators) | 128 × 4 multiply | amp × panL, amp × panR |
| Sum to L/R | 128 add × 2 | |
| Normalization | 2 multiply | 1/sqrt(N) |
| DC blocker + limiter | 6 ops | |
| **Total per sample** | **~1536 ops** | |

At 44.1 kHz: ~67.7M ops/sec per voice.

### 8.3 Voice Budget

Per-voice total: ~93.7M ops/sec (boid + oscillators)
4 voices: ~375M ops/sec
M1 single-core: ~3.2 GFLOPS
**CPU usage: ~11.7%** per engine instance

This is the heaviest V3 engine. With SIMD vectorization (processing 4 particles simultaneously), the oscillator bank cost drops by ~4×, bringing total CPU to **~7%** vectorized. Without SIMD, full modulation overhead pushes it toward **15%**.

**Eco mode:** Cap particles at 64 in 4-engine configs. Halves all costs.

### 8.4 Memory

- Per-voice particle state: 128 × 32 bytes = 4 KB
- Spatial hash grid: 128 × 8 bytes = 1 KB
- Oscillator phases: 128 × 4 bytes = 512 bytes
- 4 voices: ~22 KB total
- **Total: ~22 KB** — negligible

---

## 9. Implementation Notes

### 9.1 SIMD Vectorization Strategy

The oscillator bank is the primary optimization target. 128 particles can be processed in groups of 4 (NEON) or 8 (AVX2):

```cpp
// NEON: Process 4 oscillators simultaneously
for (int i = 0; i < 128; i += 4)
{
    float32x4_t phases = vld1q_f32(&particle_phases[i]);
    float32x4_t freqs = vld1q_f32(&particle_freqs[i]);
    float32x4_t amps = vld1q_f32(&particle_amps[i]);

    // Phase increment
    float32x4_t phaseInc = vmulq_f32(freqs, invSampleRate);
    phases = vaddq_f32(phases, phaseInc);
    // ... wrap, generate, accumulate
}
```

Expected speedup: 3–4× on Apple M-series (NEON), bringing oscillator cost from ~67M to ~17M ops/sec per voice.

### 9.2 Particle Frequency Clamping

Particle frequencies must stay within audible range (20–20000 Hz). Soft-clamping applied in the boid velocity update:

```cpp
if (particle.freq < 20.0f)
{
    particle.freq = 20.0f;
    particle.vFreq = std::abs(particle.vFreq) * 0.5f;  // Bounce off lower bound
}
if (particle.freq > 20000.0f)
{
    particle.freq = 20000.0f;
    particle.vFreq = -std::abs(particle.vFreq) * 0.5f;  // Bounce off upper bound
}
```

### 9.3 Thread Safety

- Particle arrays pre-allocated per voice in `prepare()`. No audio-thread allocation.
- Boid engine updates particle positions at control rate. Oscillator bank reads positions at audio rate.
- Double-buffered particle state: boid engine writes to back buffer, oscillator bank reads from front buffer, atomic pointer swap at control rate tick.
- ParamSnapshot pattern for all parameter reads.

### 9.4 Denormal Protection

All particle amplitudes clamped to minimum 1e-7f (not zero — zero-amplitude particles should still participate in boid dynamics). Oscillator outputs flushed if magnitude < 1e-15f. DC blocker state flushed at < 1e-15f.

---

*Architecture spec owner: XO_OX Designs | Engine: OCEANIC | Next action: Phase 1 — Boid Core + Particle Oscillator Bank*
