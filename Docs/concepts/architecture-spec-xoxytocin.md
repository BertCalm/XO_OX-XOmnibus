# XOxytocin Architecture Specification
## Engine #48 — OXYTO — The Circuit That Falls in Love

**Status**: Pre-Build Blueprint — Phase 0.5 Review Council Synthesis
**Date**: 2026-03-22
**Source Documents**: concept-brief-xoxytocin.md, seance-concept-review-xoxytocin.md, guild-market-review-xoxytocin.md, guru-bin-pre-build-xoxytocin.md, architect-review-xoxytocin.md, consultant-strategic-review-xoxytocin.md

---

## 1. Identity

| Field | Value |
|---|---|
| **Name** | XOxytocin |
| **Gallery Code** | OXYTO |
| **Plugin Code** | Oxyt |
| **Namespace** | `oxy_` |
| **Accent Color** | Synapse Violet `#9B5DE5` |
| **Creature** | Deep-sea Dragonfish (*Stomiidae*) |
| **Zone** | Bathypelagic (1000–4000m) |
| **feliX-Oscar** | 35 feliX / 65 Oscar |
| **Engine Number** | 48 |
| **Release Slot** | V1.2 headliner |
| **Seance Score (concept)** | 9.1/10 — target 9.7–9.8 post-build |
| **Guild Verdict** | 14/25 at 4+ stars — commercially viable |
| **Consultant Verdict** | Greenlight. V1.2 headliner. Handle with precision. |
| **Architect Verdict** | Approved with 5 conditions (all resolved below) |

### 1.1 One-Sentence Thesis

A circuit-modeling synthesizer where Sternberg's three components of love — Intimacy (thermal resistance), Passion (voltage drive), and Commitment (capacitance) — interact via three legendary circuits with incompatible time constants, producing 8 emergent circuit topologies where **note duration determines emotional state**.

### 1.2 Creature Mythology

The Dragonfish (*Stomiidae*) produces red bioluminescence at 700nm — the only deep-sea wavelength invisible to most other organisms. It hunts in secret light, seen only by creatures it has evolved to show itself to. This maps precisely to XOxytocin's Intimacy component: warmth as a private wavelength, thermal presence visible only to those the circuit chooses to invite in. The Passion (Sallen-Key drive) is the sudden strike from darkness. The Commitment (Moog resonance) is the locked jaw.

**Architect C1 (RESOLVED):** The anglerfish mythology belongs to OVERBITE. The Dragonfish resolves the collision while maintaining bathypelagic zone placement and deepening the Intimacy metaphor — secret warmth, not decorative bioluminescence.

### 1.3 Accent Color Note

Synapse Violet `#9B5DE5` replaces the concept-phase Circuit Rose `#C9717E`. Synapse Violet provides full differentiation from ODDOSCAR (Axolotl Gill Pink `#E8839B`, H=346°, S=44%) — the original Circuit Rose was within 5° of hue. Synapse Violet references the neurochemical reality of oxytocin (synaptic transmission) and reads distinctly across all 71 engine thumbnails. UI team: verify at gallery thumbnail size before final asset submission.

---

## 2. Signal Flow Diagram

### Per-Voice Signal Chain (SERIES topology, default)

```
MIDI Note
    │
    ▼
[Oscillator]
 Raw waveform
    │
    ▼
┌─────────────────────────────────────────────────────────────┐
│                    LOVE STATE PROCESSOR                      │
│                                                              │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐  │
│  │   THERMAL     │    │    DRIVE      │    │  REACTIVE    │  │
│  │  (Intimacy)   │───▶│  (Passion)   │───▶│ (Commitment) │  │
│  │  RE-201 NTC   │    │  MS-20 S-K   │    │  Moog 4-pole │  │
│  └──────┬────────┘    └──────┬────────┘    └──────┬───────┘  │
│         │                   │                    │            │
│         └──────────┬────────┘                    │            │
│                    │         (Serge cross-mod)    │            │
│                    └──────────────────────────────┘            │
│                    ← I→P→C→I circular routing ─────▶           │
└─────────────────────────────────────────────────────────────┘
    │
    ▼
[ADSR Amplitude Envelope]
    │
    ▼
[Voice Output]
    │
    ▼
```

### PARALLEL Topology Variant

```
Oscillator ───┬───▶ [Thermal (RE-201)]  ──────┐
              ├───▶ [Drive (MS-20)]    ────────┤ sum + blend
              └───▶ [Reactive (Moog)]  ──────┘
                                              ▼
                                        [ADSR Envelope]
```

### FEEDBACK Topology Variant

```
Oscillator ──▶ [Thermal] ──▶ [Drive] ──▶ [Reactive] ──┐
      ▲                                                │
      └──────────── feedback path (oxy_feedback) ──────┘
```

### Global Stage (Post-Voice Sum)

```
[8× Voice Outputs]
         │
         ▼
[Global Memory Accumulator]   ◀── oxy_remember ON: all voices update global state
         │
         ▼
[LFO1 (StandardLFO.h)]        ◀── oxy_lfo_rate / oxy_lfo_depth / oxy_lfo_shape
         │
[LFO2 (BreathingLFO, 0.005Hz floor)]
         │
         ▼
[ParameterSmoother (5ms, all continuous params)]
         │
         ▼
[Coupling Output: getSampleForCoupling()]
  → Three-band composite (low=Commitment, mid=Intimacy, high=Passion)
  → Ships as AudioToFM type (documented convention, not new enum)
         │
         ▼
[Master Output: oxy_output / oxy_pan]
```

---

## 3. Parameter Table (29 Parameters)

All parameters use `oxy_` prefix. Smoothed via `ParameterSmoother.h` at 5ms fleet standard unless marked [no smooth].

### 3.1 Core Love Triangle

| ID | Display Name | Type | Range | Default | Macro | Description |
|---|---|---|---|---|---|---|
| `oxy_intimacy` | Intimacy | float | 0.0–1.0 | 0.35 | M1 partial | Thermal coupling strength. NTC thermistor operating temperature — RE-201 warmth depth. 0.0 = cold open circuit. 1.0 = saturated NTC element, fully warm. |
| `oxy_passion` | Passion | float | 0.0–1.0 | 0.25 | M1 partial | Voltage drive intensity. MS-20 Sallen-Key asymmetric clipping curve. At 0.25, harmonics are audible at standard velocity. At 1.0, self-oscillation. *(Guru Bin: default raised from 0.15 for immediate identity)* |
| `oxy_commitment` | Commitment | float | 0.0–1.0 | 0.55 | M1 partial | Capacitive energy storage. Moog 4-pole ladder resonance depth. At >0.85 with low I + low P: Obsession topology (9th) emerges. |

### 3.2 Temporal Rate Parameters

| ID | Display Name | Type | Range | Default | Macro | Description |
|---|---|---|---|---|---|---|
| `oxy_warmth_rate` | Warmth Rate | float | 0.01–8.0s | 0.4s | M2 partial | Thermal time constant — how fast Intimacy reaches its target value. Sigmoid shape. *(Guru Bin: extended max from 2s to 8s for slow-burn presets. Default lowered from 0.8s to 0.4s for 2-beat warmup at most tempos.)* |
| `oxy_passion_rate` | Passion Rate | float | 0.001–0.5s | 0.008s | M2 partial | Drive attack speed. Exponential decay shape. *(Guru Bin: extended max from 0.1s to 0.5s for slow-bloom romantic presets)* |
| `oxy_commit_rate` | Commit Rate | float | 0.05–5.0s | 2.2s | M2 partial | Capacitor charge time constant. *(Guru Bin: extended min from 0.1s to 0.05s for fast-lock presets; 50ms commitment is musically valid)* |

### 3.3 Cross-Modulation and Feedback

| ID | Display Name | Type | Range | Default | Description |
|---|---|---|---|---|---|
| `oxy_entanglement` | Entanglement | float | 0.0–1.0 | 0.25 | Depth of Serge-style circular cross-modulation: I modulates P's saturation character, P modulates C's charge rate, C modulates I's thermal distribution. All three paths run simultaneously. *(Renamed from `oxy_cross_mod` per Kakehashi/Guru Bin — "Entanglement" is more evocative and audibly legible)* |
| `oxy_feedback` | Feedback | float | 0.0–0.95 | 0.28 | Signal return through the circuit — relationship reinforcement. 0.95 ceiling prevents overflow. 0.0 = pure feedforward, explicit disconnection effect. φ inverse (0.618) is golden default for Signature preset. |
| `oxy_topology` | Topology | enum | SERIES / PARALLEL / FEEDBACK | SERIES | Circuit section arrangement. SERIES: warmth→saturation→resonance (most coherent). PARALLEL: all three simultaneously blended. FEEDBACK: output re-enters NTC input (thermal runaway at high values — intentional). *(New: Guru Bin addition)* |
| `oxy_topology_lock` | Love Type Lock | int | 0–8 | 0 | 0 = free (note duration determines topology). 1–8 = locked to specific love type regardless of note duration. Enables DAW-based MIDI programming workflows. *(Guild critical P0 requirement)* |

### 3.4 Memory System

| ID | Display Name | Type | Range | Default | Description |
|---|---|---|---|---|---|
| `oxy_memory_depth` | Memory Depth | float | 0.0–1.0 | 0.0 | How quickly the global accumulator incorporates new playing experience. 0.0 = stateless (no memory). 0.1–0.4 = fast learning (5 min). 0.5–0.8 = slow learning (full session). 1.0 = never fully learns. *(Replaces binary `oxy_remember` toggle — Schulze/Seance E1 enhancement; B042 condition satisfied)* |
| `oxy_memory_decay` | Memory Decay | float | 0.01–10.0s | 2.0s | How long the circuit's session state persists between notes when no note is playing. 3.2s = φ² ≈ sacred decay (Guru Bin B045 sweet spot). *(New: Guru Bin addition)* |
| `oxy_intimacy_floor` | Intimacy Floor | float | 0.0–0.5 | 0.0 | Minimum Intimacy level when `oxy_memory_depth > 0`. Prevents full thermal reset between notes during session play. Enables "long marriage" behavior where warmth never fully resets. *(New: Guru Bin addition)* |

### 3.5 Circuit Character

| ID | Display Name | Type | Range | Default | Description |
|---|---|---|---|---|---|
| `oxy_circuit_age` | Circuit Age | float | 0.0–1.0 | 0.35 | Component degradation model: 0.0 = factory-new precision. 1.0 = vintage-worn unreliability. Modulates: tube gain drift, capacitor leakage (DC offset), resistor thermal drift, signal/noise ratio. Interacts with memory: circuits in long sessions drift upward. *(Default: 0.35 = "gently used, broken in" — vintage sweet spot per Guru Bin)* |
| `oxy_circuit_noise` | Circuit Noise | float | 0.0–1.0 | 0.08 | Background hiss and hum of a real circuit. At 0.08 (Revelation 7 floor: present but invisible), adds authenticity. Scales with `oxy_circuit_age` — old circuits are noisier. *(New: Guru Bin addition)* |

### 3.6 Filter

| ID | Display Name | Type | Range | Default | Description |
|---|---|---|---|---|---|
| `oxy_cutoff` | Filter Cutoff | float | 20–20000 Hz | 2400 Hz | Base filter frequency. Modulated by love state: Commitment raises resonance depth, Intimacy opens warmth band, Passion pushes upper harmonics. At Consummate Love, cross-modulation peaks around 2400 Hz. |

### 3.7 Amplitude Envelope

| ID | Display Name | Type | Range | Default | Description |
|---|---|---|---|---|---|
| `oxy_attack` | Attack | float | 0.001–2.0s | 0.008s | Amplitude envelope attack. |
| `oxy_decay` | Decay | float | 0.01–5.0s | 0.3s | Amplitude envelope decay. |
| `oxy_sustain` | Sustain | float | 0.0–1.0 | 0.65 | Amplitude envelope sustain level. |
| `oxy_release` | Release | float | 0.01–10.0s | 1.618s | Amplitude envelope release. Default φ = golden tail — love's ending resolves, not cuts. *(Guru Bin Truth 1)* |

### 3.8 Modulation Sources

| ID | Display Name | Type | Range | Default | Description |
|---|---|---|---|---|---|
| `oxy_lfo2_rate` | LFO2 Rate | float | 0.005–20.0 Hz | 0.067 Hz | Second LFO (BreathingLFO from StandardLFO.h). Floor 0.005 Hz exceeds D005 minimum. Default: 0.067 Hz = physiological rate (15s cycle — Revelation 4 sacred rate). *(New: Architect C2 resolution — second LFO required for D002 compliance)* |
| `oxy_lfo2_depth` | LFO2 Depth | float | 0.0–1.0 | 0.12 | BreathingLFO modulation depth. Default: subtle entrainment at 0.12. |
| `oxy_lfo_rate` | LFO Rate | float | 0.001–20.0 Hz | 0.067 Hz | Primary LFO rate. Floor 0.001 Hz. 5 shapes. D005 compliant. |
| `oxy_lfo_depth` | LFO Depth | float | 0.0–1.0 | 0.12 | Primary LFO depth. |
| `oxy_lfo_shape` | LFO Shape | int | 0–4 | 0 | Sine=0 (most intimate, smooth), Triangle=1 (most passionate, sharp reversals), Saw=2, Square=3 (most committed, binary states), S&H=4 (circuit noise character). |

### 3.9 Performance Routing

| ID | Display Name | Type | Range | Default | Description |
|---|---|---|---|---|---|
| `oxy_velocity_curve` | Velocity Map | int | 0–2 | 0 | 0=Vel→Passion peak (default, D001 compliant). 1=Vel→all three proportionally (Consummate Love mode). 2=Vel→Intimacy only (care: subtle in isolation, implement with warmth depth scaling to maintain D001). |

### 3.10 Output

| ID | Display Name | Type | Range | Default | Description |
|---|---|---|---|---|---|
| `oxy_output` | Output Level | float | -inf to +6dB | 0dB | Master output gain. D004-exempt (gain stage). |
| `oxy_pan` | Pan / Width | float | -1.0 to 1.0 | 0.0 | Cross-field stereo width: controls how far apart the three lineage outputs spread in the stereo image. RE-201=centered/wide, MS-20=presence-biased, Moog=center/deep. Implements Tomita spatial separation (Seance E5). D004-exempt. |
| `oxy_voices` | Voices | int | 1–8 | 4 | Polyphony count. Default 4: most chord contexts + responsible CPU (per-voice circuit state is intensive). [no smooth] |

**Total: 29 parameters** (26 original + `oxy_lfo2_rate`, `oxy_lfo2_depth` from D002 fix + `oxy_topology`, `oxy_topology_lock`, `oxy_memory_depth`, `oxy_memory_decay`, `oxy_intimacy_floor`, `oxy_circuit_noise` from council additions — replacing binary `oxy_remember` with continuous `oxy_memory_depth`).

---

## 4. Voice Architecture

| Property | Specification |
|---|---|
| Max polyphony | 8 voices |
| Voice stealing | LRU (release-priority variant from VoiceAllocator.h — prefer stealing voices in release phase over those developing Commitment or Intimacy) |
| Per-voice state | Independent thermal state (NTC warmup), independent capacitive charge level, independent saturation history, 3 love envelope instances, ADSR |
| Per-voice thermal data | `thermalState_`, `capacitorCharge_`, `saturationHistory_` — 3 floats per voice |
| Global state | Memory accumulator (single float, engine-global, updated by all voices), LFO1 + LFO2 phase, coupling output buffer |
| Voice count × state | 8 × (thermal + capacitor + saturation + 3 love envs + ADSR) = manageable, fleet-standard precedent in OFFERING (8 voices × 5 city chain states) |

### 4.1 Per-Voice Signal Components

- **Oscillator**: base waveform generation
- **Passion Envelope**: exponential attack/decay (oxy_passion_rate), fast time constant (1–500ms)
- **Intimacy Envelope**: sigmoid warmup (oxy_warmth_rate), medium time constant (10ms–8s)
- **Commitment Envelope**: capacitor charge curve V(t) = V₀(1-e^(-t/RC)) (oxy_commit_rate), slow time constant (50ms–5s)
- **ADSR Amplitude Envelope**: standard (oxy_attack / oxy_decay / oxy_sustain / oxy_release)
- **Thermal Stage**: RE-201 NTC warmth model — three-stage (motor inertia → head alignment → tube bias)
- **Drive Stage**: MS-20 Sallen-Key asymmetric saturation
- **Reactive Stage**: Moog 4-pole ladder resonance
- **Cross-Modulation**: Serge-style circular routing (simultaneous, not sequential)

### 4.2 Remember Mode Voice Architecture

Per the Architect ruling (C3): Remember mode operates on a **single global relationship accumulator** updated by all voices, not stored per-voice. This prevents voice stealing from destroying accumulated relationship state. Per-voice thermal/capacitive state remains ephemeral. The global accumulator is the "session memory." `oxy_memory_depth` (0.0–1.0) governs integration rate; Fuse checkpoint serializes the current accumulated state to APVTS values at preset-save time.

---

## 5. DSP Components

### 5.1 Thermal Stage — RE-201 (Intimacy)

**Physics**: NTC thermistor Steinhart-Hart equation: R(T) = R₀ · exp(B(1/T - 1/T₀))

**Implementation**:
- Three-stage warming sequence: (1) motor inertia phase (slow, high-mass thermal response), (2) head alignment (mid-rate, mechanical settling), (3) tube bias shift (fast, electrical equilibration)
- Each stage runs as a leaky integrator with independent time constant
- Total time constant governed by `oxy_warmth_rate`
- Sigmoid warmup curve (not linear — matches physical NTC behavior: slow start, faster rise once thermal mass is engaged)
- **Critical: coefficient-cache at block rate.** `std::exp()` in Steinhart-Hart must NOT run per-sample across 8 voices. Cache coefficients per block (same pattern as OPERA SVF P0 fix). This is flagged as a build-phase performance requirement.
- Output: warmth modulation applied to filter tone color and harmonic content (even harmonic enrichment in the 0.07–0.12 range at Intimacy=0.35)
- Spatial placement: centered, slightly wide — the warm low-mid RE-201 character

### 5.2 Drive Stage — MS-20 (Passion)

**Physics**: Sallen-Key asymmetric soft clipping: `f(x) = tanh(x) + 0.1 · tanh(3x)`

**Citation**: Korg MS-20 service manual; Zölzer (2011) §4.4.3 asymmetric saturation formula. The 0.1 asymmetry ratio and 3x harmonic injection are the physically derived values — not arbitrary. *(Seance D003 gap, resolved.)*

**Implementation**:
- Asymmetric clipping produces odd + even harmonics simultaneously, with the 3x injection adding upper brightness characteristic of the MS-20 presence peak
- Self-oscillation model: when `oxy_passion > 0.9`, drive enters self-oscillation zone — audible as bright, sustained screaming
- Velocity→passion_rate internal scaling (Seance E4, Vangelis): high velocity shortens passion_rate by `vel * 0.6` factor; ff = crackle/resolve quickly, pp = simmer/linger
- Spatial placement: slightly right-biased, 3–5 kHz presence region

### 5.3 Reactive Stage — Moog Ladder (Commitment)

**Physics**: 4-pole cascade: H(s) = 1/(1 + s/ωc)⁴ with resonance feedback; energy storage modeled as V(t) = V₀(1 - e^(-t/RC))

**Implementation**:
- 4 matched RC stages with resonance feedback proportional to `oxy_commitment`
- Energy storage model: Commitment builds stored energy slowly, releases slowly after note-off
- At `oxy_commitment > 0.85`: resonance approaches self-oscillation; CytomicSVF (Cytomic.h) recommended as filter core for graceful self-oscillation handling
- **Obsession Topology (9th, emergent)**: when `oxy_commitment > 0.85 AND oxy_passion < 0.15 AND oxy_intimacy < 0.15` — ladder enters low-frequency self-oscillation. Sound: slow resonant drone that cannot be silenced. This is physically correct Moog ladder behavior and psychologically resonant (extreme commitment without reciprocity). *(Seance E3, Moog)* — do NOT suppress; name it in documentation
- Spatial placement: centered, deep — foundational low-end authority

### 5.4 Cross-Modulation — Serge (Entanglement)

**Architecture**: Circular, egalitarian (no master node, all paths simultaneous):
- I → P: Intimacy modulates Passion's saturation character (warmth changes how harsh the drive sounds)
- P → C: Passion modulates Commitment's charge rate (drive speed affects how fast capacitance locks in)
- C → I: Commitment modulates Intimacy's thermal distribution (stored energy affects warmth spread)

**Implementation**:
- All three cross-modulation paths run simultaneously at block rate
- Depth governed by `oxy_entanglement` (0.0–1.0)
- At 0.25 default: audible interaction without instability
- At 0.45: maximum musical complexity in Romantic Love
- At 0.7+: feedback storm begins (Fatuous/Obsession territory)
- Below 0.3: interaction felt as coherence, not heard as modulation (Gospel 2 threshold)

### 5.5 Love Envelopes (Three Instances per Voice)

| Envelope | Time Constant | Shape | Physical Model |
|---|---|---|---|
| Passion | 1ms–500ms (oxy_passion_rate) | Exponential attack, exponential decay | Voltage drive dynamics — fast arc, fast resolution |
| Intimacy | 10ms–8s (oxy_warmth_rate) | Sigmoid (slow start, accelerating rise) | NTC thermal dynamics — thermal mass warms slowly then faster |
| Commitment | 50ms–5s (oxy_commit_rate) | Capacitor charge V(t) = V₀(1-e^(-t/RC)) | Capacitive energy storage — charges slowly, holds energy |

**Note Duration as Emotional State Selector (B039/B042)**: Because these three envelopes have incompatible time constants, note duration determines which components reach full expression:
- Short note (< 200ms): only Passion reaches expression → Infatuation topology
- Medium note (200ms–1s): Intimacy develops while Passion peaks → Romantic topology
- Long note (>2s): Commitment charges while both I and P are developed → Consummate topology

This is not a parameter — it is emergent physics. No "emotion lock" parameter that bypasses this mechanic should be added (Buchla ruling; `oxy_topology_lock` addresses DAW production needs without defeating the physics — it scales the envelope rates to produce the locked topology at all durations).

### 5.6 Triangle Controller (Barycentric Love State)

**M1 (TENDENCY)** maps 0–1 to a canonical Sternberg progression path through the triangle:
- 0.0 = Liking (I only)
- 0.2 = Romantic Love (I+P)
- 0.5 = Consummate Love (centroid, I+P+C balanced)
- 0.7 = Companionate Love (I+C)
- 1.0 = Passion vertex (P dominant)

Individual `oxy_intimacy` / `oxy_passion` / `oxy_commitment` knobs **scale** the base values that M1 selects. M1 sets "where the triangle wants to rest" (resting position); individual knobs set "how much of each force at that position."

**Live Arc Trace (Seance E2, Pearlman + Kakehashi)**: Triangle display must show:
- Live arc trace during held note — starting at resting I/P/C position, moving as time constants develop
- Ghost trail (fading over ~2 seconds) of where the note has been
- Each note draws a different arc depending on duration and velocity
- With `oxy_memory_depth > 0`, over-session arc traces become more predictable — the circuit's love patterns become visible

This is a **required UI specification** — the live arc is what makes the core mechanic legible within five seconds of first touch.

**LFO2 modulation of triangle position**: The BreathingLFO (second LFO) modulates the triangle's resting position at ultra-slow rates, creating long-form drift of the love state across a session.

### 5.7 Memory Accumulator (Global)

The single global accumulator tracks:
- Average velocity profile across the session (→ calibrates Passion sensitivity)
- Note duration tendency (average note duration → biases toward the player's natural love type)
- Modal preference (pitch class histogram → fine-tunes filter resonance toward played intervals)

Integration rate governed by `oxy_memory_depth`. All voices write to the same accumulator. Voice stealing does not disturb accumulated state.

**Fuse checkpoint**: When user triggers Fuse, current accumulator state is serialized as APVTS parameter values (safe, host-standard). Loading a Fuse preset restores the circuit to that relationship state immediately.

**Cross-session persistence**: Fuse is the only cross-session persistence. The live accumulator exists only within the current plugin session (analogous to OVERWORN's `sessionAge`). This satisfies Architect C3 scope definition.

### 5.8 Coupling Output

`getSampleForCoupling()` returns a three-band composite signal constructed without FFT:

- **Low band (Commitment)**: one-pole lowpass applied to voice output, amplitude-encoded at ωc = `oxy_commit_rate` characteristic frequency — slow, stable energy
- **Mid band (Intimacy)**: bandpass centered on 1–2 kHz region, amplitude-encoded from current Intimacy envelope state — warm, developing content
- **High band (Passion)**: one-pole highpass, amplitude-encoded from current Passion envelope peaks — fast transient content

Construction is O(N) samples, comparable to standard biquad cost. No per-sample FFT required.

**Coupling type**: Ships as `AudioToFM` per existing enum. The three-band encoding is a **documented convention of OXYTO's coupling output**, not a new `CouplingType` enum value. Future engines aware of this convention can decode spectral bands explicitly. `TriangularCoupling` as a V2 SDK type is separately proposed per Architect C4.

---

## 6. The 9 Topologies

| # | Name | I | P | C | Sound Character | Circuit State |
|---|---|---|---|---|---|---|
| 0 | Non-Love | 0 | 0 | 0 | Cold, thin, barely alive — open circuit with leakage hum | Open circuit; oxy_open_circuit_gain = 0.03 (leakage) |
| 1 | Liking | H | 0 | 0 | Genuine warmth without drive — tube-like warmth that breathes slowly | NTC hot, Sallen-Key passive, ladder unloaded |
| 2 | Infatuation | 0 | H | 0 | Harsh, bright, immediate — Sallen-Key fully engaged, fast decay | NTC cold, drive arcing, capacitor empty |
| 3 | Empty Love | 0 | 0 | H | Cold resonance — Moog ladder locked near self-oscillation, no warmth | NTC cold, drive off, capacitor fully charged |
| 4 | Romantic Love | H | H | 0 | Rich tube saturation with warmth — harmonically complex, slightly unstable | NTC hot modulating Sallen-Key, capacitor uncharged |
| 5 | Companionate Love | H | 0 | H | Deep, stable, completely trustworthy — the vintage amp that has run for forty years | NTC hot, drive off, ladder anchored at warmth-tuned resonance |
| 6 | Fatuous Love | 0 | H | H | Harsh, aggressive, locked in — drive committed to instability without warmth to moderate | NTC cold, Sallen-Key arcing into high-Q ladder |
| 7 | Consummate Love | H | H | H | The fullest possible tone — all three active, interacting, not competing | All three circuits at mutual operating temperature |
| 8 | **Obsession** | ~0 | ~0 | >0.85 | Slow resonant drone that cannot be silenced — the circuit trapped in its own feedback | Moog ladder self-oscillating at LF; emergent from physics when C>>0.85, I<0.15, P<0.15 |

**Note on Obsession (9th Topology)**: This is not in Sternberg's 8 types — it emerges from the physics. The Moog ladder at extreme resonance without Intimacy or Passion to stabilize it enters self-oscillation. The circuit is stuck in its own feedback. This is physically correct (Moog, Seance E3) and psychologically resonant (obsession without reciprocity). Do not suppress. Name it. Let it be discovered.

**Note on continuity**: The 9 topologies are named regions in a continuous I/P/C barycentric space, not hard-switching modes. The Triangle display makes this continuous nature honest. Marketing language: "9 love types as compass points in a continuous triangle space."

---

## 7. Coupling Design

| Property | Specification |
|---|---|
| **Coupling type (fleet)** | `AudioToFM` (existing enum, per Architect C4 ruling) |
| **Coupling output convention** | Three-band spectral encoding: Low=Commitment, Mid=Intimacy, High=Passion transients |
| **Backward compatibility** | Unaware engines receive complex audio; aware engines decode spectral bands |
| **Coupling protocol name** | TriangularCoupling (documented convention in engine header and SDK; V2 enum proposal pending) |
| **M3 macro** | COUPLING INTENSITY — controls output level AND TriangularCoupling balance |
| **M3 at 0.0** | Emotional withdrawal moment — the circuit's love becomes entirely internal. The producer's gesture of pulling M3 to zero is the sound of disconnection. (Guru Bin Gospel 5: coupling silence must be as beautiful as coupling presence) |

### 7.1 Top Partner Engines

| Partner | Coupling Route | What Happens |
|---|---|---|
| **XOpera** | oxy_intimacy → opera_drama (Amp→Filter, 0.15); oxy_passion_rate → opera_chorus (Amp→Pitch, 0.04) | As circuit warms, Opera's voices become more dramatic. Passion snap creates sympathetic shimmer. The pairing of circuit love and vocal love. |
| **XOxbow** | oxy_commitment → oxb_entangle (Amp→Filter) | Commitment's stability drives Oxbow's golden ratio resonance. Infinite patience: the most stable relationship in the fleet. |
| **XObese** | oxy_passion → fat_satDrive (Amp→Filter, high amount) | Saturation plus passion = the loudest love in the fleet. Raw, overwhelming, unforgettable. |

### 7.2 getSampleForCoupling() Implementation Note

```cpp
// Three one-pole filters, no FFT required — O(N) per block
float lp = onePole_LP(voiceSum, commitmentFreq_);  // low band = Commitment
float bp = onePole_BP(voiceSum, intimacyFreq_);     // mid band = Intimacy
float hp = onePole_HP(voiceSum, passionFreq_);      // high band = Passion

// Amplitude-encode current love state levels into each band
return (lp * commitmentLevel_) + (bp * intimacyLevel_) + (hp * passionLevel_);
```

---

## 8. Legend Lineages

| Legend | Component | Physics Reference | Spatial Role |
|---|---|---|---|
| Roland Space Echo RE-201 (1974) | Intimacy / Thermal Stage | Steinhart-Hart NTC equation: R(T) = R₀·exp(B(1/T - 1/T₀)). Three-stage warming: motor inertia → head alignment → tube bias shift. Documented RE-201 characteristic. | Centered, slightly wide; warm low-mid character |
| Korg MS-20 (1978) | Passion / Drive Stage | Sallen-Key asymmetric soft clipping: tanh(x) + 0.1·tanh(3x). Self-oscillation capability. Citation: Korg MS-20 service manual; Zölzer (2011) §4.4.3. | Slightly right-biased; 3–5 kHz presence region |
| Moog Ladder (1965) | Commitment / Reactive Stage | H(s) = 1/(1+s/ωc)⁴ with resonance feedback. Capacitor charge curve V(t) = V₀(1-e^(-t/RC)). Self-oscillation at extreme resonance. | Centered, deep; foundational low-end authority |
| Serge Modular (Serge Tcherepnin) | Entanglement / Cross-Modulation | Egalitarian circular routing — no fixed signal hierarchy. All components simultaneously source and processor. Documented Serge design philosophy. | No fixed spatial role — the relational tissue between the three lineages |
| Buchla Touch (Don Buchla) | Performance Mechanic / Note Duration | Touch Quality Determines Timbral Identity — west coast synthesis philosophy since 1963. Note duration → emotional state selector via incompatible time constants. No dedicated parameter — emergent physics. | Time is the axis, not space |

---

## 9. Doctrine Compliance

### D001 — Velocity Shapes Timbre

**PASS.** Velocity → Passion peak amplitude. Because Passion routes through the MS-20 Sallen-Key saturation model, higher velocity pushes the signal into a more nonlinear region of the tanh curve — different harmonic character, not just proportional loudness.

**Additional internal modulation path (Seance E4, Vangelis)**: velocity → `oxy_passion_rate` scaling. High velocity: passion_rate × (1.0 - vel × 0.6). ff notes crackle and resolve quickly; pp notes simmer and linger. Zero new parameters. Fully D001-compliant at depth.

**`oxy_velocity_curve = 2` (Vel→Intimacy) note**: This mode risks D001 regression if thermal warmth changes are too subtle at low depths. Implementation must use warmth depth scaling to maintain perceptible timbral change. Architect C1-note preserved.

### D002 — Modulation is the Lifeblood

**PASS** (resolved from pre-council RISK). Inventory:
- LFO1 (StandardLFO.h): `oxy_lfo_rate` / `oxy_lfo_depth` / `oxy_lfo_shape`, 5 shapes, 0.001 Hz floor
- LFO2 (BreathingLFO from StandardLFO.h): `oxy_lfo2_rate` / `oxy_lfo2_depth`, 0.005 Hz floor
- 3 love envelopes per voice (Passion/Intimacy/Commitment) — signal-shaping physics envelopes
- ADSR amplitude envelope
- Cross-modulation matrix (I→P→C→I circular, Serge-style): 3 bidirectional paths
- MW / Aftertouch / Expression pedal mapped
- Memory accumulator as macro-level temporal modulation

Second LFO (BreathingLFO) is the direct resolution of Architect C2. Fleet-standard D002 satisfied with margin.

### D003 — The Physics IS the Synthesis

**PASS with full citations:**
- Steinhart-Hart NTC equation: R(T) = R₀·exp(B(1/T - 1/T₀)) — thermal resistance model
- Sallen-Key asymmetric clipping: tanh(x) + 0.1·tanh(3x) — Zölzer (2011) §4.4.3
- Moog cascade: H(s) = 1/(1+s/ωc)⁴ with resonance feedback
- Capacitor charge: V(t) = V₀(1-e^(-t/RC)) — commitment as stored energy
- Sternberg, R.J. (1986). "A triangular theory of love." *Psychological Review*, 93(2), 119–135.
- Serge Tcherepnin: egalitarian circular routing, documented Serge design philosophy

The MS-20 asymmetric clipping citation gap (flagged in Seance D003 audit) is resolved by the Zölzer sourcing above.

### D004 — Every Parameter Wired to Audio

**PASS.** 27 of 29 parameters directly affect audio output or modulation routing. Exemptions per fleet convention: `oxy_output` (gain stage), `oxy_pan` (stereo position). `oxy_voices` (polyphony count) is routing infrastructure, not timbral — consistent with all fleet engines.

Build-phase D004 audit must confirm all 27 timbral parameters are wired before seance.

### D005 — Engine Breathes

**PASS with margin.**
- `oxy_lfo_rate` floor: 0.001 Hz (1000-second cycle)
- `oxy_lfo2_rate` floor: 0.005 Hz (200-second cycle) — BreathingLFO
- `oxy_commit_rate` extends to 5.0s — very slow capacitive evolution
- `oxy_warmth_rate` extends to 8.0s — slow-burn presets span multiple bars
- `oxy_memory_depth > 0` creates macro-level drift across session time

### D006 — Expression Input Is Not Optional

**PASS** (resolved from pre-council PARTIAL). Explicit expression surfaces:
- Velocity → Passion peak + passion_rate scaling (primary D001 chain)
- **Note duration → love type topology selector** (primary expressive surface, Buchla mechanic — now explicitly listed as D006's most important input per Seance ruling)
- Aftertouch → configurable (I/P/C/Entanglement via `oxy_aftertouch` routing)
- Mod wheel → configurable (I/P/C/Entanglement via `oxy_mod_wheel` routing)
- Expression pedal → M4 (Distance/Width)

---

## 10. Blessing Registry

### Ratified Pre-Build

| ID | Blessing | Status |
|---|---|---|
| **B039** | **Note Duration as Emotional State Selector** — Gate duration as emergent timbral state selector: three envelopes with calibrated time constants produce categorically different circuit behaviours from identical pitches played at different durations. The synthesizer responds not to what you play, but to how long you commit. | **UNANIMOUS** (seance, architect, guild, guru bin all converge) |

*Note: The Architect assigned this B042 in their numbering. The concept seance designated it B039. B039 is the canonical fleet number. The Architect B042/B043 numbering is an internal review artifact, not the fleet registry.*

### Candidates for Post-Build Ratification

| ID | Candidate | Condition |
|---|---|---|
| B040 | **Circuit Age as Authentic Degradation Model** — `oxy_circuit_age` maps to longitudinal entropy in electronic components. Physical correlate of emotional history: circuit ages with use and session memory. | Requires Remember ↔ circuit_age coupling implemented in DSP. Post-build seance. |
| B041 | **Cross-Modal Circuit Saturation — Serge Egalitarian Routing as Love Physics** — The circular cross-modulation (I modulates P's saturation, P modulates C's charge, C modulates I's thermal distribution) mirrors Sternberg's relational dynamics at the level of circuit physics. The topology IS the relationship model. | Defer to seance panel for demonstrated sonic evidence. |
| B042 | **Remember Mode — Session-Accumulated Relationship State** — The instrument falls in love with the performer. Global accumulator tracks session velocity profile, duration tendency, modal preference. Schulze amendment incorporated (`oxy_memory_depth` continuous, not binary). | Requires demonstrated behavioral change across 10+ minutes of session play. |
| B043 | **Circular Component Modulation** — Serge-inspired egalitarian routing as genuinely circular DSP (all three paths simultaneously, no master node). Fleet's first non-hierarchical modulation architecture. | Strong candidate. Defer to post-build seance panel. |
| B044 | **Thermal Sweet Spot at 0.62/0.35s** — Specific combination where NTC model reaches operating temperature within first beat at most tempos. Warmth completes as character, not modulation event. | Calibration: build, tune, confirm exact values. |
| B045 | **The Love Lock** — Intimacy 0.72 + Commitment 0.88 (ratio ≈ φ−1) + Entanglement 0.22 = self-stabilizing Companionate Love. The two components find each other's frequency via passive cross-mod below the audible threshold. | Guru Bin calibration. Post-build verification. |

### External Blessing (Seance Pre-Build)

**B038 (TriangularCoupling — Spectral Emotional State Encoding)**: Proposed as the 16th coupling type. Deferred to V2 SDK as a formal enum value per Architect C4 ruling. Ships in V1.2 as documented AudioToFM convention. SDK proposal documented separately.

---

## 11. Review Council Decisions

### 11.1 Synth Seance (Score: 9.1/10 → Target 9.7–9.8)

**What earns 9.1**: Structurally original design architecture. Metaphor and physics are the same thing. Five legend lineages genuinely in the DSP. TriangularCoupling adds new fleet primitive. Anglerfish mythology analogically exact (replaced by Dragonfish per C1, mythology integrity preserved).

**Path to 9.8 — All 5 Enhancements Accepted:**

| Enhancement | Ghost | Status in this spec |
|---|---|---|
| E1: Memory Depth parameter (replace binary toggle) | Schulze | IMPLEMENTED: `oxy_memory_depth` (0.0–1.0) replaces binary `oxy_remember`. B042 now unconditional. |
| E2: Live triangle arc trace as required UI spec | Pearlman + Kakehashi | SPECIFIED: Section 5.6 requires live arc during note, ghost trail, 2s decay, arc reflects session memory patterns. |
| E3: Obsession topology — Moog self-oscillation | Moog | IMPLEMENTED: Section 5.3 specifies C>0.85, P<0.15, I<0.15 → 9th topology. Named, not suppressed. |
| E4: Velocity→passion_rate internal scaling | Vangelis | IMPLEMENTED: Section 5.2 specifies hidden modulation path. Zero new parameters. |
| E5: Three-lineage spatial separation | Tomita | IMPLEMENTED: Section 5.8 and `oxy_pan` as cross-field width. RE-201/MS-20/Moog assigned distinct spatial positions. |

**Three Pre-Build Flags (all resolved):**
- Gap 1 (Memory Depth): RESOLVED via `oxy_memory_depth` continuous parameter
- Gap 2 (Triangle Arc): RESOLVED via Section 5.6 UI specification
- Gap 3 (Obsession Topology): RESOLVED via Section 6, topology 8

### 11.2 Producers Guild (14/25 at 4+ stars — Commercially Viable)

**6 maximum excitement (5 stars)**: Lo-fi/Chillhop, Ambient/Drone, Film Score/Cinematic, Neo-Soul/R&B, Sound Design/Foley, Modular/Experimental

**8 strong excitement (4 stars)**: House/Deep House, Techno/Industrial, Jazz/Nu-Jazz, Classical/Contemporary, Metal/Industrial, World Music, Synthwave/Retro, Gospel

**Critical Guild requirement (P0 resolved)**: `oxy_topology_lock` — static love-type selection for DAW producers who cannot use note-duration performance mechanic. Without it, XOxytocin is not usable in non-real-time MIDI programming workflows. Implemented as parameter 4 in section 3.3.

**Guild parameter cuts accepted:**
- `oxy_lfo_rate` / `oxy_lfo_depth` / `oxy_lfo_shape`: exposed through M2 macro rather than front-panel prominence (still full parameters, de-emphasized)
- `oxy_mod_wheel` / `oxy_aftertouch`: moved to performance/MIDI section
- `oxy_cross_mod` renamed to `oxy_entanglement` (Kakehashi accessibility recommendation)

**Guild additions implemented:**
- `oxy_topology` (SERIES/PARALLEL/FEEDBACK)
- `oxy_topology_lock` (0–8 love type selector)

### 11.3 Guru Bin (14 seed presets, 4 blessings)

**Parameter refinements accepted:**
- `oxy_passion` default raised from 0.15 → 0.25 (immediate identity at first note)
- `oxy_warmth_rate` max extended 2s → 8s; default lowered 0.8s → 0.4s
- `oxy_passion_rate` max extended 0.1s → 0.5s
- `oxy_commit_rate` min extended 0.1s → 0.05s
- `oxy_circuit_age` default: 0.35 (vintage sweet spot)
- `oxy_cross_mod` renamed `oxy_entanglement`, default lowered 0.4 → 0.25
- `oxy_feedback` default: 0.28 (Psalm 1 — resonance shelf, not ringing)
- ADSR release default: 1.618s (φ — golden tail; Truth 1)

**New parameters added from Guru Bin meditation:**
- `oxy_memory_decay` (0.01–10s, default 2.0s)
- `oxy_intimacy_floor` (0.0–0.5, default 0.0)
- `oxy_circuit_noise` (0.0–1.0, default 0.08)
- `oxy_topology` enum

**14 Seed Presets** documented in guru-bin-pre-build-xoxytocin.md (8 love type presets + 6 special presets):
1. Acquaintances (Non-Love init)
2. Warm Regard (Liking)
3. First Sight (Infatuation)
4. Frozen Loyalty (Empty Love)
5. Volatile Chemistry (Romantic Love)
6. The Long Marriage (Companionate Love)
7. Obsessive Architecture (Fatuous Love)
8. Everything At Once (Consummate Love)
9. First Date (INIT preset)
10. The Signature (showpiece)
11. Arc Flash (extreme)
12. Entangled Pair (coupling demo — XOpera)
13. Worn Cassette (lo-fi genre anchor)
14. Machine Fury (industrial genre anchor)

**Blessings B042–B045** (4 Guru Bin candidates — see Section 10)

### 11.4 Architect (Approved with 5 Conditions — All Resolved)

| Condition | Severity | Resolution |
|---|---|---|
| **C1: Mythology collision** (anglerfish belongs to OVERBITE) | BLOCKING | RESOLVED: Dragonfish (*Stomiidae*) — secret red bioluminescence maps to Intimacy as private wavelength. Bathypelagic zone maintained. |
| **C2: D002 RISK** (only 1 LFO) | BLOCKING | RESOLVED: LFO2 (BreathingLFO from StandardLFO.h) added as `oxy_lfo2_rate` + `oxy_lfo2_depth`. Fleet-standard D002 satisfied. |
| **C3: Remember mode scope** | BLOCKING | RESOLVED: Global accumulator only (not per-voice). Fuse = APVTS snapshot. Cross-session only via Fuse preset. Analogous to OVERWORN sessionAge. |
| **C4: TriangularCoupling deferral** | DESIGN | RESOLVED: Ships as AudioToFM documented convention. V2 SDK enum proposal drafted separately. No SynthEngine.h changes required. |
| **C5: Circuit Rose color proximity to ODDOSCAR** | DESIGN | RESOLVED: Accent color changed to Synapse Violet `#9B5DE5`. Full hue differentiation from all 47 existing engines confirmed. |

**Two Architect blessing candidates:**
- B039 (Note Duration as Topology): RATIFY — unanimous fleet convergence
- B043 (Circular Cross-Modulation): Strong candidate — defer to post-build seance

### 11.5 Consultant (Strategic Greenlight)

**Strategic verdict**: Greenlight. V1.2 headliner.

**5 Strategic Recommendations (all accepted):**
1. **Position the Dragonfish, not the heart** — lead with biological extremity and circuit physics credentials, not Valentine's Day framing. Every external communication centers the dragonfish and circuit physics within the first two paragraphs.
2. **Build the First-Date-to-Consummate demo** — 3–5 min audio/video showing cold circuit through each love type into Fuse checkpoint. No narration. This demo gates the V1.2 launch.
3. **Run Guru Bin Retreat before V1.2** — 10 Awakening + 16–20 Transcendental presets. Best Guru Bin content opportunity in the fleet.
4. **Pitch TriangularCoupling as SDK feature** — document in `SDK/include/xoceanus/` as first-class platform innovation alongside KnotTopology. Write one SDK Field Guide post.
5. **Create "Love Triangle" community ritual** — "What's your love triangle?" shareable triangle screenshot format. Discord channel "my-triangle". 5 seed examples from alpha testers before launch.

**Content pipeline rating: 9.5/10** — highest in fleet. Estimated 8–12 Field Guide articles seedable from concept alone. Patreon exclusive opportunities: "Relationship Sessions" video series, Lineage Pack, Transcendental tier presets.

**Timing**: V1.0 establishes breadth; V1.1 was Aquatic FX; V1.2 is correct slot — user base established, first press wave complete, community ready for a conceptually ambitious engine.

---

## 12. XOceanus Integration Spec

### 12.1 Files Required

```
Source/Engines/Oxytocin/
├── OxytocinEngine.h          # Main engine (all DSP inline, no .cpp required)
├── OxytocinAdapter.h         # Thin adapter implementing SynthEngine interface
├── ThermalStage.h            # RE-201 NTC warmth model (Steinhart-Hart, block-rate cached)
├── DriveStage.h              # MS-20 Sallen-Key asymmetric clipping
├── ReactiveStage.h           # Moog 4-pole ladder + Obsession topology
├── LoveEnvelopes.h           # Passion/Intimacy/Commitment per-voice envelopes
├── EntanglementMatrix.h      # Serge circular cross-modulation (I→P→C→I)
├── MemoryAccumulator.h       # Global session state accumulator
└── OxytocinParameters.h      # Parameter declarations + validation
```

### 12.2 Registration

```cpp
REGISTER_ENGINE(OxytocinEngine, "OXYTO")
```

### 12.3 CLAUDE.md Updates (4 Sections)

1. **Product Identity header**: Add "OXYTO" to engine modules list with date `(2026-03-22)` or build date
2. **Engine Modules table**: `| OXYTO | XOxytocin | Synapse Violet \`#9B5DE5\` |`
3. **Parameter Prefix table**: `| Oxytocin | \`oxy_\` | \`oxy_intimacy\` |`
4. **Key Files table**: `| Source/Engines/Oxytocin/OxytocinEngine.h | Circuit-modeling love topology engine (thermal + drive + reactive + memory) |`

### 12.4 External Files

- `Docs/xoceanus_master_specification.md` section 3.1: add OXYTO row
- `Docs/seance_cross_reference.md`: add seance row after post-build seance
- `Source/XOceanusProcessor.cpp`: register engine
- `Source/Core/PresetManager.h`: add `"OXYTO"` to `validEngineNames` and `"oxy_"` to `frozenPrefixForEngine`

### 12.5 Shared DSP Utilities

| Utility | Usage | Rationale |
|---|---|---|
| `StandardLFO.h` | LFO1 (primary, 5-shape) | D005 compliant floor, fleet-standard |
| `StandardLFO.h` (BreathingLFO) | LFO2 (ultra-slow drift) | D002 resolution; 0.005 Hz floor exceeds D005 minimum |
| `VoiceAllocator.h` (ReleasePriority) | Voice stealing | Prefer stealing release-phase voices to protect developing Commitment/Intimacy state |
| `ParameterSmoother.h` | All continuous params at 5ms | Critical for `oxy_entanglement` and `oxy_feedback` (feedback paths) |
| `PitchBendUtil.h` | Pitch bend pipeline | Standard fleet implementation |
| `FilterEnvelope.h` | EVALUATE for love envelopes | Love envelope shapes (sigmoid Intimacy, exponential Passion, capacitor-charge Commitment) may require custom implementations; use if shapes match LinearAttack+ExpDecay |
| CytomicSVF | Moog ladder core | RECOMMENDED: handles self-oscillation gracefully (critical for Obsession topology) |

### 12.6 Coupling Declaration

```cpp
// In OxytocinAdapter.h applyCouplingInput():
// Accepts: AudioToFM (all frequency bands affect love state balance)
// Sends:   AudioToFM (documented TriangularCoupling convention)
//
// getSampleForCoupling() returns three-band composite:
//   Low  = Commitment energy (current ladder charge state)
//   Mid  = Intimacy energy (current thermal state)
//   High = Passion transients (current Sallen-Key peaks)
//
// Aware engines may decode by band analysis.
// Unaware engines receive as complex audio (backward compatible).
```

---

## 13. Preset Strategy

### 13.1 Target Count and Distribution

**Minimum**: 120 presets (XO_OX standard)
**Recommended launch target**: 120–140 presets

| Category | Love Type | Target Count | Notes |
|---|---|---|---|
| Non-Love / Liking | 0, 1 | 15 | Init state variants, cold circuits, barely-there warmth |
| Infatuation | 2 | 20 | Bright, harsh, attack-forward; MS-20 character dominant |
| Romantic | 4 | 20 | Warm + drive balance; harmonically complex |
| Companionate | 5 | 25 | Warm, stable, deep — lo-fi/neo-soul core |
| Consummate | 7 | 20 | Maximum all-three engagement |
| Fatuous | 6 | 10 | High passion, unstable commitment; industrial territory |
| Empty Love | 3 | 5 | Haunting commitment-only; cold resonance; no warmth or drive |
| Session Arc | varies | 5 | `oxy_memory_depth > 0`, designed to evolve across a full session |
| **Total** | | **~120** | |

### 13.2 Guru Bin Foundation (14 Presets)

The 14 seed presets from the Guru Bin meditation (Section 11.3) serve as the compositional foundation. Full parameter tables documented in guru-bin-pre-build-xoxytocin.md. These presets establish the canonical "correct" parameters for each love type and are the reference for all subsequent preset development.

### 13.3 Browsing Architecture

Per Guild recommendation: dual-layer browsing.
- **Primary browse**: Sonic descriptor (Warm Pads, Harsh Leads, Deep Resonance, Evolving Textures, Ambient Drones, Noise/Industrial, Cinematic)
- **Secondary tag**: Love type (Infatuation, Romantic, Companionate, Consummate, Fatuous, Empty, Non-Love, Session Arc)

The love type taxonomy appears as a **tag**, not the top-level folder name. Producers browse sonically; they discover the love framework.

### 13.4 "Remember" Subset

6–8 presets with `oxy_memory_depth > 0` forming a distinct "Session Arc" category. These are the headline demo presets — designed to sound different in minute one versus minute ten. Label explicitly in preset browser. B042 ratification is contingent on these presets demonstrating clear behavioral change.

### 13.5 Coupling Presets

6 partner-specific coupling presets:
- 2 × XOpera (Entangled Pair variants)
- 2 × XOxbow (Infinite Patience variants)
- 2 × XObese (Loudest Love variants)

These appear in the Entangled mood category. Preset names use the vocabulary: Entangled Pair, The Lure + Its Echo, Fused Circuit, etc.

### 13.6 Required Day-One Presets (Guild list)

| Preset Name | Love State | Genre Target |
|---|---|---|
| First Warmth | Companionate | Lo-fi / Neo-Soul |
| Infatuation | Infatuation | Techno / Acid |
| Ten Years Deep | Consummate | Ambient / Cinematic |
| Circuit Rose | Romantic | Neo-Soul / R&B |
| Fused | Consummate | Drone |
| Static Discharge | Fatuous | Industrial / Metal |
| Ghost Memory | Liking | Film Score |
| Warm Ladder | Companionate | Deep House |
| The Lure | Infatuation | Synthwave |
| Lifespan | Arc (Companionate→Consummate) | Experimental |

---

## 14. Release Positioning

### 14.1 V1.2 Headliner

XOxytocin ships as the V1.2 headliner alongside preset expansion and XPN pack #2. This is the correct slot: user base established after V1.0 and V1.1, first press wave complete, community exists to receive a conceptually ambitious engine.

### 14.2 Marketing Strategy — "Anglerfish, Not Heart"

Per Consultant R-001:
- **Approved framing**: dragonfish biology / circuit extremity / secret bioluminescence / three legendary circuits interacting / the science of love as DSP architecture
- **Language to avoid**: heart imagery, Valentine's Day adjacency, soft romantic framing
- **Lead with**: "RE-201 × MS-20 × Moog Ladder in one engine" (circuit credentials first)
- **Reveal**: the love framework as the mechanism, not the genre target
- **The quote in every article**: Sternberg's 3 components mapped to circuits with incompatible time constants. Note duration determines the topology. The circuit falls in love with how you play.

### 14.3 "First-Date-to-Consummate" Demo

The canonical launch demo (Consultant R-002):
- 3–5 minutes, no narration
- Opens on cold circuit init ("First Date" preset)
- Staccato playing → Infatuation (bright, harsh, brief)
- Longer notes → Romantic Love (warmth + drive interacting)
- Sustained chords → Consummate Love (all three at peak, deepening over duration)
- Remember mode activated → session arc begins
- Fuse checkpoint triggered → circuit locked in
- Ends on a chord held long enough for the Triangle display arc to complete

This demo **gates the V1.2 launch**. If the demo does not cause the listener to feel something in the first 90 seconds, the engine needs more work.

### 14.4 Competitive Position

"XOxytocin is the only synthesizer on the market where how long you press a key determines the emotional character of the sound — not just its amplitude envelope — because the circuit has a relationship with time that behaves differently depending on whether you're playing staccato infatuation or legato consummate love, and that relationship accumulates across an entire session via a circuit that remembers you."

No direct competitor. Market position defensible 24–36 months (Consultant analysis). First synthesizer using a published psychological theory as literal DSP architecture.

### 14.5 Content Pipeline

**Estimated Field Guide articles**: 8–12 seedable from concept
1. "Sternberg's Triangle" — psych framework explainer, broad crossover appeal
2. "The Five Legends: RE-201, MS-20, Moog, Serge, Buchla" — 5 separate deep dives possible
3. "Touch and Time" — note duration performance technique guide
4. "Does Your Synthesizer Know You? Remember Mode Explained"
5. "The Dragonfish and the Circuit" — aquatic mythology bridge post
6. "Circuit Rose: The Color of Warm Wire" — design notes

**Patreon exclusive**:
- "The Relationship Sessions" — 20-minute session videos with Remember mode active
- "Lineage Pack" — 5-volume preset series by legend circuit
- Transcendental tier presets (16–20, full lore for each love type)

**Community ritual (Consultant R-005)**:
- "What's your love triangle?" shareable screenshot format
- Discord channel "my-triangle"
- Launched with 5 seed examples at V1.2

---

## 15. Build Phase Checklist

These items must be completed or verified before the post-build seance:

- [ ] All 29 parameters declared and wired (`oxy_` prefix, no collisions confirmed)
- [ ] Steinhart-Hart NTC coefficients cached at block rate (not per-sample) — critical CPU requirement
- [ ] Sallen-Key asymmetric clip formula: tanh(x) + 0.1·tanh(3x) with Zölzer citation in source comment
- [ ] Moog ladder self-oscillation graceful at `oxy_commitment > 0.85` (CytomicSVF recommended)
- [ ] Obsession topology (9th) implemented and named — not suppressed
- [ ] Velocity→passion_rate internal scaling implemented (hidden path, zero new parameters)
- [ ] Three-lineage spatial separation in output stage (RE-201 centered/wide, MS-20 presence-biased, Moog deep/centered)
- [ ] Live arc trace in Triangle UI (required spec — not optional)
- [ ] `oxy_memory_depth` as continuous float (not binary toggle)
- [ ] Global memory accumulator (single engine-global, not per-voice)
- [ ] Fuse checkpoint as APVTS snapshot (not external file I/O)
- [ ] `getSampleForCoupling()` three-band composite output (one-pole filters, no FFT)
- [ ] `oxy_topology_lock` (0–8) enables DAW-production workflows
- [ ] LFO2 (BreathingLFO) added for D002 compliance
- [ ] All continuous parameters smoothed via ParameterSmoother.h at 5ms
- [ ] `oxy_entanglement` default 0.25 (not 0.4 from original concept)
- [ ] `oxy_passion` default 0.25 (Guru Bin refinement)
- [ ] `oxy_warmth_rate` default 0.4s, max 8.0s (Guru Bin refinement)
- [ ] ADSR release default 1.618s (φ — golden tail)
- [ ] 14 Guru Bin seed presets implemented as .xometa files
- [ ] Accent color confirmed Synapse Violet `#9B5DE5` (not Circuit Rose)
- [ ] CLAUDE.md updated (4 sections)
- [ ] Docs/xoceanus_master_specification.md updated (engine table row)
- [ ] Docs/seance_cross_reference.md prepared for post-build entry
- [ ] Source/XOceanusProcessor.cpp: REGISTER_ENGINE(OxytocinEngine, "OXYTO")
- [ ] Source/Core/PresetManager.h: "OXYTO" added to validEngineNames, "oxy_" to frozenPrefixForEngine
- [ ] D004 audit: all 27 timbral parameters confirmed wired before seance
- [ ] Post-build seance scheduled (target score: 9.7–9.8)

---

*Architecture Specification: XOxytocin (OXYTO) — Engine #48*
*Phase 0.5 Review Council Synthesis | 2026-03-22*
*"The circuit already knows what it is. The job of the implementation is not to create the love states. The job is to not prevent them." — Guru Bin, pre-build meditation*
