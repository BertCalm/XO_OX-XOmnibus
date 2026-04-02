# OUTWIT Seance Prep Document

**Engine**: XOutwit (OUTWIT)
**Prepared**: 2026-03-16
**Status**: Phase 4 COMPLETE (installed 2026-03-15) | auval PASS | Seance PENDING
**Source files**: `Source/Engines/Outwit/XOutwitAdapter.h`, `Source/Engines/Outwit/DSP/`
**Architecture brief**: `~/.claude/projects/-Users-joshuacramblet/memory/xoutwit-architecture.md`

---

## 1. Engine Identity

| Field | Value |
|-------|-------|
| Gallery Code | OUTWIT |
| Creature | Giant Pacific Octopus (*Enteroctopus dofleini*), Inside Passage kelp forest, Alaska |
| Accent Color | Chromatophore Amber `#CC6600` |
| Parameter Prefix | `owit_` |
| Plugin Code | OWIT |
| Voice Model | Monophonic (1 MIDI note = 8 arms simultaneously); getMaxVoices() = 1 |
| Total Parameters | 95 (56 per-arm × 8 + 39 global) |
| Presets | 150 (Entangled 51 / Atmosphere 35 / Prism 30 / Flux 15 / Aether 15 / Foundation 4) |

---

## 2. DSP Architecture Summary

OUTWIT is an **8-arm Wolfram cellular automaton synthesizer** where each voice-channel runs its own independent Wolfram elementary CA rule (0-255). Eight parallel computational processes evolve and interact — the Giant Pacific Octopus as a distributed intelligence model. It is the only instrument in the fleet where a genetic algorithm is embedded in the synthesis loop to hunt for sounds.

### Signal Flow

```
MIDI Input
  → noteOn: pitch assigned to 8 arm oscillators
  → velocity: CA step density bias + filter cutoff range + ink cloud threshold
  → aftertouch: chromatophore depth
  → mod wheel: SYNAPSE amount

8 ARM CHANNELS (parallel):
  Per Arm:
    CA Engine (WolframCA struct)
      Rule 0-255 → 8-bit LUT lookup
      1D binary grid (4-64 cells, armLength)
      Independent step clock per arm
      3-cell neighborhood → next row
      density = count(ON cells) / armLength
      Periodic boundary conditions (ring topology)
    → Cell-to-Synthesis Mapping:
        density → amplitude multiplier (quadratic)
        density → filter cutoff (exponential)
        density → chromatophore spectral shape
        rising edge of cell[0] → note retrigger
        row hash → pitch micro-detune (±25 cents)
    → Oscillator (Saw/Pulse/Sine per arm, PolyBLEP anti-aliased)
    → CytomicSVF Filter (per arm, cutoff driven by CA density)
    → Chromatophore Layer (density-driven one-pole LPF, sigmoid expansion curve)
    → Arm VCA (level × density × amp envelope)
    → DC Blocker
    → Pan position (8 arms spread across stereo field)

  SYNAPSE BUS: arm[N].density → seed perturbation → arm[N+1]
  (circular: arm[7] → arm[0])

  SOLVE Engine (GA, runs at start of renderBlock):
    Fitness = 1/(1 + euclideanDistance(targetDNA, observedDNA))
    4 mutation operators: bit flip / neighbor step / arm swap / random reset
    ~6400 ops/generation (negligible CPU)
    Atomic write of new rules for audio thread consumption

Arm Mixer (owit_arm{N}Level × 8)
→ Ink Cloud (noise burst on velocity > 0.8)
→ Master SVF (global tone shaping)
→ Den Reverb (4-line FDN, Hadamard matrix, rocky cave model)
→ Soft Limiter (tanh saturation)
→ Stereo Output Cache (for coupling reads)
```

### Key DSP Concepts

- **Wolfram Elementary CA**: Each arm runs an 8-bit rule number as its own LUT. Rule behavior spans Class I (silence/drone) through Class IV (Rule 110, Turing-complete, genuinely unpredictable). No other synthesizer runs independent computational rules per voice channel.
- **CA step rate vs. sample rate**: CA evolves at 0.01–40 Hz, NOT sample rate. The CA computation cost is near-zero — 8 arms × ~100 integer ops per step, stepping at most 40 times per second vs. 48,000 audio samples.
- **SYNAPSE**: Arm N's cell density probabilistically flips cells in Arm N+1's grid before its next step. Creates emergent collective behavior from simple nearest-neighbor coupling.
- **SOLVE GA**: Inline genetic algorithm evaluates rule mutations against a 6D Sonic DNA fitness target. At SOLVE=1.0, the octopus is always hunting — timbral/rhythmic identity evolves toward a DNA target while perpetually exploring. 5% annealing acceptance prevents local optima.
- **Chromatophore Layer**: Sigmoid-shaped one-pole LPF whose cutoff is driven by CA cell density. `chromCutoff = 200 * pow(80, expansion * chromAmount)` — density maps spectral range from 200 Hz to 16 kHz.
- **Den Reverb**: 4-line FDN with Hadamard mixing matrix. Mutually prime delay lengths (three size presets: crevice / overhang / cave). Not a concert hall — an enclosed rocky space.
- **Ink Cloud**: Velocity-triggered noise burst (threshold 0.8). Defensive mechanism — hard hits produce an ink-smoke transient.

---

## 3. The 6 Doctrine Checks (Preliminary Assessment)

### D001 — Velocity Must Shape Timbre
**Status: PASS (DESIGNED)**

Velocity shapes timbre through two independent paths:
1. **Filter cutoff range**: `cutoffMod = velocity * density * cutoffRange` — high velocity opens filter further
2. **CA step density bias**: Higher velocity initializes the CA with denser starting rows — denser CA = brighter chromatophore response immediately on note-on

Additionally, velocity > 0.8 triggers the Ink Cloud (noise burst), which is a discrete timbral event exclusive to hard hits.

This is a D001 implementation with three velocity→timbre mechanisms. Unusually thorough.

**Examiner note**: The CA density bias on velocity is an indirect but real mechanism — verify in adapter that `noteOn` actually seeds the CA grids with different initial densities based on velocity, not just amplitude.

---

### D002 — Modulation is the Lifeblood
**Status: PASS (DESIGNED)**

- **LFO 1**: `owit_lfo1Rate` 0.01–20 Hz, 5 shapes, 4 destinations (StepRate / FilterCutoff / ChromAmount / ArmLevels)
- **LFO 2**: Same range/shapes, 4 destinations (defaults to FilterCutoff)
- **Mod Wheel**: Routes to SYNAPSE amount (`owit_synapse`) — increasing arm-to-arm coupling via performance gesture
- **Aftertouch**: Routes to Chromatophore depth — pressure expands spectral coloring
- **4 Macros**: SOLVE (character), SYNAPSE (movement), CHROMATOPHORE (coupling), DEN (space)
- **6 expression routes**: velocity→filter, velocity→density, velocity→ink, aftertouch→chrom, MW→synapse, CC74→master filter

The LFO targeting StepRate is particularly potent: modulating how fast the CA evolves creates rhythmic acceleration and deceleration — a modulation target impossible in any other synthesis paradigm.

**Examiner note**: With 95 parameters and 8 independent arm channels, there is a non-trivial risk of some per-arm parameters being wired correctly in isolation but not consumed from the adapter's APVTS cache. Verify `attachParameters()` caches all `owit_arm{N}Rule` through `owit_arm{N}Pan` pointers for all N=0..7. That is 56 pointers from per-arm params alone.

---

### D003 — The Physics IS the Synthesis
**Status: PASS (APPLICABLE AND RIGOROUS)**

OUTWIT claims Wolfram cellular automaton computation as its synthesis paradigm. This is academically well-founded:

- **Wolfram ECA reference**: Wolfram (1983) is the canonical source. Rule 110 Turing-completeness is from Cook (2004) — both explicitly referenced in the architecture spec.
- **CA implementation**: The `WolframCA` struct implements correct 3-cell neighborhood lookup with periodic boundary conditions (ring topology). The rule number → LUT encoding is mathematically exact, not approximate.
- **Wolfram class taxonomy**: The architecture spec correctly maps rules to Class I/II/III/IV behaviors and the sonic character of each.
- **SOLVE GA fitness function**: Euclidean distance in 6D DNA space is a valid fitness metric. The simulated annealing acceptance (5%) is correct to prevent premature convergence.

**Examiner note**: The CA itself is mathematically honest. The SOLVE genetic algorithm is a practical approximation (hill-climbing with annealing, not a full evolutionary algorithm), but this is an engineering implementation choice, not a physics violation. D003 is well-handled here.

---

### D004 — Dead Parameters Are Broken Promises
**Status: PASS (DESIGNED) — NEEDS VERIFICATION**

With 95 parameters, the D004 risk is higher than average for this engine. Key categories to examine:

| Parameter Group | Count | Risk |
|----------------|-------|------|
| Per-arm parameters (all 8 arms) | 56 total | If any arm index (e.g., arms 4-7) is not fully wired, those 28 params are dead |
| SOLVE target DNA params (`owit_targetBrightness` through `owit_targetAggression`) | 6 | Are these actually read in the GA fitness function, or is target DNA hardcoded? |
| `owit_stepSync` / `owit_stepDiv` | 2 | Tempo sync — requires host BPM. If not connected to host transport, these are dead |
| `owit_voiceMode` / `owit_glide` | 2 | Glide only meaningful in Mono mode — contextually inactive in Poly, which is acceptable |
| `owit_inkDecay` | 1 | Is Ink Cloud decay rate wired or fixed? |

**Examiner note**: The 6 SOLVE target DNA parameters are the highest D004 risk. If the GA target is hardcoded to `[0.5, 0.5, 0.5, 0.5, 0.5, 0.5]` rather than reading from the adapter, all 6 target params are broken promises. Verify these are read from APVTS.

---

### D005 — An Engine That Cannot Breathe Is a Photograph
**Status: PASS (DESIGNED)**

- Both LFOs: 0.01–20 Hz floor (explicit D005 compliance)
- CA step rate (`owit_stepRate`): 0.01–40 Hz — at minimum, arms evolve once per 100 seconds (geological time)
- The CA itself is an autonomous breathing mechanism: density rises and falls as cellular evolution proceeds, creating organic timbral contours that would exist even without any LFO

OUTWIT has at minimum three independent D005-satisfying mechanisms. The CA evolution functioning as a slow modulation source is particularly elegant — the entire engine is, at its core, a modulation source that also generates audio. D005 is a category that OUTWIT's design philosophy inherently satisfies.

**Examiner note**: Verify `owit_stepRate = 0.01 Hz` is genuinely audible — at that rate, one CA step per 100 seconds means the timbre barely changes. Is this useful, or is the 0.01 Hz floor purely nominal for D005 compliance? The seance may probe whether the floor is musically meaningful or just technically present.

---

### D006 — Expression Input Is Not Optional
**Status: PASS (DESIGNED)**

| Source | Target | Note |
|--------|--------|------|
| Velocity | Filter cutoff range | D001 overlap — strong |
| Velocity | CA step density bias | Novel D001 implementation |
| Velocity > 0.8 | Ink cloud trigger | Exclusive timbral event |
| Aftertouch | Chromatophore depth | Spectral bloom under pressure |
| Mod Wheel | SYNAPSE amount | Collective behavior via gesture |
| CC74 (brightness) | Master filter cutoff | Standard MIDI convention |

Six expression routes covering D006 requirements with architectural elegance. Mod wheel controlling SYNAPSE (arm coupling) is a particularly inspired mapping — the performer's hand gesture directly controls whether the 8 arms act as independent minds or a collective consciousness.

**Examiner note**: Verify `modWheelValue` and `aftertouchValue` are actually read from adapter state per block and applied. The `XOutwitAdapter.h` shows `modWheelValue` and `aftertouchValue` initialized to 0 in `prepare()` — confirm they are updated via `handleMidi()` or equivalent.

---

## 4. Potential Blessing Candidates

### Primary Candidate: **New Blessing — Wolfram CA as Synthesis Architecture**

No precedent exists in the fleet or in commercial synthesis. Eight parallel Wolfram automata each running independent rules, coupled through SYNAPSE, hunting via GA. The CA IS the synthesis, not a modulation source bolted onto a conventional oscillator.

**Candidate Blessing**: *Distributed CA Intelligence* — the only synthesizer where independent computational rules per voice channel produce emergent polymetric rhythm and organic timbral evolution. The SOLVE macro transforms the instrument into a hunting organism that seeks a sonic target.

This would be B016 or later in the sequence, following OVERLAP's potential new blessing.

### Secondary Candidate: **SOLVE Genetic Algorithm**

The GA fitness-based sound evolution is a self-contained innovation. While other instruments have "mutation" or "randomize" features, OUTWIT's SOLVE is directional — it moves toward a target DNA, not randomly. This is closer to artificial life than synthesis.

The council may grant this as a sub-component of the CA blessing, or as a distinct blessing for "directional self-evolution."

### Comparison to Existing Blessings

- **B003 (Leash Mechanism / OUROBOROS)**: OUROBOROS has a chaotic system with a stabilizing leash. OUTWIT has a CA system with a genetic hunt. Both involve containing/directing emergent computational behavior. The council will likely draw this parallel — OUTWIT is "leash via fitness function" vs. OUROBOROS's "leash via damping."
- **B002 (XVC / ONSET)**: Both ONSET and OUTWIT have 8 parallel synthesis channels. ONSET's cross-voice coupling (XVC) was blessed for being 3-5 years ahead. OUTWIT's SYNAPSE is structurally analogous but computationally orthogonal. The council will debate whether OUTWIT advances or parallels ONSET's achievement.

---

## 5. Open Questions for the Full Seance

1. **OUTWIT vs. ONSET architectural overlap**: Both are monophonic 8-channel engines with coupling between channels. The council should explicitly evaluate whether OUTWIT is a genuinely different paradigm from ONSET or a cellular-automaton variant of the same concept. The core difference: ONSET channels are physically modeled voice types (kick/snare/hat/etc.), while OUTWIT channels are computationally abstract (rule numbers). This is a meaningful distinction, but the council should rule on it.

2. **SOLVE target DNA parameters**: Are `owit_targetBrightness` through `owit_targetAggression` wired to the GA fitness function in the adapter? These are the six most conceptually important parameters in the engine — if dead, it is a critical D004 failure.

3. **Tempo sync completeness**: Does `owit_stepSync` correctly read host BPM and apply `owit_stepDiv`? This requires host transport access in the XOceanus context. Several engines have had issues with this in the gallery context even when it worked correctly standalone.

4. **Per-arm parameter diversity in presets**: The engine has 56 per-arm parameters. Do the 150 presets meaningfully exploit per-arm diversity (different rules, lengths, pitches, wave shapes per arm), or do many presets set all 8 arms to the same values with only slight variations? Flat per-arm preset design would undermine the engine's core premise.

5. **Rule 110 sonic demonstration**: Rule 110 is Turing-complete (the mathematically remarkable Class IV behavior). Does at least one preset explicitly demonstrate what Turing-complete CA synthesis sounds like? The council will likely ask to hear Rule 110 across all 8 arms at various lengths.

6. **OUTWIT → ONSET coupling demonstration**: The crown jewel coupling (CA arm density → ONSET drum voice blend) — does a preset exist demonstrating it? The architecture declares it as "the crown jewel." If no preset exploits it, that is a missed showcase of the engine's most significant capability.

7. **Ink Cloud threshold**: The Ink Cloud triggers at velocity > 0.8. Is this threshold hardcoded or parameterized? At 0.8, most playing will never trigger it unless the player specifically hammers. Should this be exposed as `owit_inkThresh` for performer control?

8. **CA initialization at note-on**: When a note is retriggered, does the CA grid reinitialize from a single center cell (canonical Wolfram start) or continue from its current state? The choice fundamentally affects the "attack character" of the engine. Continuation means each note attack is different based on history; reinit means attacks are predictable.

9. **Monophonic voice model justification**: `getMaxVoices() = 1` makes OUTWIT fully monophonic in XOceanus gallery contexts. Is this artistically correct for the Giant Pacific Octopus creature identity (which hunts alone), or does it limit the engine's usability in ensemble presets?

---

## 6. Recommended Ghost Council Members

OUTWIT's paradigm spans computational theory (cellular automata, genetic algorithms), electronic music history (early algorithmic composition), and the specific tradition of treating computation as musical performance. Recommended council:

| Ghost | Rationale |
|-------|-----------|
| **Stephen Wolfram** | Not a musician but the originator of elementary CA classification. His presence as a fictional "ghost" consultant would be historically apt. He would evaluate whether Rule 110's Turing-completeness is musically meaningful or just a theoretical curiosity. |
| **Roger Linn** | Drum machine pioneer (LinnDrum, MPC). OUTWIT's crown jewel coupling is OUTWIT→ONSET (CA drives drum kit). Linn would evaluate whether CA-driven polymetric rhythm is genuinely useful for drum programming or academically clever but impractical. |
| **Morton Subotnick** | Electronic music pioneer, algorithmic composition. He worked with Buchla systems to create generative music in the 1960s. The SOLVE genetic algorithm as a compositional tool — hunting for sound — aligns with his "composer as system designer" philosophy. |
| **Ikutaro Kakehashi** | Roland founder. Pragmatic, commercial perspective. He would push back on any feature that prioritizes computational elegance over immediate playability. The 56 per-arm parameters would be his target — "too complex to program quickly." |
| **Pauline Oliveros** | Deep listening, emergent process. Like OVERLAP, OUTWIT's CA evolution over time is a listening process. At SOLVE=0, owit_stepRate=0.01Hz, the engine barely changes over a session — this is genuinely meditative. She would evaluate whether the engine rewards deep listening as well as active programming. |

**Optional addition**: A cognitive scientist or AI researcher ghost (Alan Turing himself?) to evaluate the SOLVE GA's relationship to true artificial intelligence vs. sophisticated parameter search. This could be a seance first — the fleet has never debated AI agency in synthesis before.

---

*Prep completed 2026-03-16. Ready for full ghost council seance.*
