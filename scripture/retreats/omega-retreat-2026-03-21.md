# OMEGA Retreat Chapter
*Guru Bin — 2026-03-21*

---

## Engine Identity

- **Gallery code:** OMEGA | **Accent:** Copper Still `#B04010`
- **Parameter prefix:** `omega_`
- **Mythology:** The Distillation — the copper still in the cellar, heat and patience, the slow separation of what matters from what does not. Purity as endpoint.
- **feliX-Oscar polarity:** Cold feliX — mathematical, precise, digital. The bass as mathematical object.
- **Synthesis type:** 2-operator FM synthesis + distillation model (mod index decays toward pure carrier) + purity noise + output LP filter
- **Polyphony:** 8 voices
- **Macros:** M1 CHARACTER, M2 MOVEMENT, M3 COUPLING, M4 SPACE
- **Seance score:** 8.6 / 10 — *Strongest engine in CELLAR quad*
- **Citations:** Chowning (1973 FM synthesis), Yamaha DX7 (1983), Bristow-Johnson (2001)

---

## Pre-Retreat State

**Seance score: 8.6 / 10.** The highest score in the CELLAR quad. The council cited the Distillation Model as a Blessing Candidate (B039): FM complexity irreversibly decaying toward pure carrier over note sustain. "No commercial FM synthesizer does this by default" (Kakehashi). Schulze heard it as "a compositional gesture, not just a timbre." Three concerns: the purity drift accumulates too aggressively (48 Hz/second of frequency error at maximum impurity — not "subtle" as claimed), the operator feedback field is dead code (functional path exists but via a separate mechanism), and the output filter ceiling does not prevent aliases at high mod index. None of these are D004 violations from the player's perspective. The engine's core behavior is mathematically realized and musically significant.

---

## Phase R1: Opening Meditation

Close your eyes. Breathe.

In the cellar there is a still. Copper, handmade, holding liquid that was once grain or fruit or root vegetable — complex, unstable, chaotic with fermentation. The still applies heat. The volatile compounds rise first: esters, aldehydes, the heads that are discarded. Then the hearts: the pure ethanol, the fraction worth keeping. Then the tails: heavier compounds, impure, collected for a second run. The distillation separates complexity into purity — not by removing what is complex, but by concentrating what is essential.

XOmega performs this process in FM synthesis.

A 2-operator FM note begins with maximum complexity. The modulator drives the carrier at high index, generating a wide spectrum of sidebands distributed across the frequency domain in complex mathematical patterns. This is the fermented liquid at the start of distillation: rich, chaotic, full of volatile compounds that will not survive the heat.

As the note sustains, the distillation model reduces the modulation index. The sidebands shrink. The higher sidebands disappear first — the volatile compounds rising and dissipating. The remaining spectrum converges toward the carrier: the fundamental, the hearts of the distillate. Hold OMEGA long enough and the note becomes a near-pure sine wave at the fundamental frequency.

Complexity to purity. Irreversibly. Within the held note.

---

## Phase R2: The Signal Path Journey

### I. The 2-Operator FM Architecture

Carrier and modulator, both sine oscillators. The modulator's output scales the carrier's phase input: `carrier.process(modulator.process(0) * activeModIndex)`. This is Chowning's 1973 paper made audible — the same technique that gave the DX7 its characteristic attack transients, the same mathematics that underlies every bell, piano, bass, and brass sound from the 1980s.

The `omega_ratio` parameter sets the modulator-to-carrier frequency ratio. The eight algorithm presets cover the primary FM bass territory:

| Algorithm | Ratio | Character |
|-----------|-------|-----------|
| 0 | 1:1 | Unison — pure FM timbre, the DX7 default |
| 1 | 2:1 | Octave — strong even harmonics, round bass |
| 2 | 3:1 | Fifth — hollow, bell-adjacent |
| 3 | 0.5:1 | Sub-octave modulation — Reese bass territory |
| 4 | 1.414:1 | Inharmonic — the Reese bass characteristic |
| 5 | 3.5:1 | Inharmonic upper — bell-plus-bass hybrid |
| 6 | 7:1 | Bell ratio — non-bass applications, mallet character |
| 7 | Custom | Player-defined via `omega_ratio` |

### II. The Distillation Model — The Engine's True Identity

The `DistillationModel` struct decays `currentIndex` with a configurable half-life:

```
halfLife = 2 + (1 - distillRate) * 18  // 2s to 20s
decayCoeff = exp(-ln(2) * dtSec / halfLife)
currentIndex *= decayCoeff
```

At `omega_distillRate=1.0`: half-life is 2 seconds. A note held for 6 seconds reduces complexity to 12.5% of initial.
At `omega_distillRate=0.1`: half-life is 20 seconds. Audible simplification takes several minutes.

The `activeModIndex = distill.process(distillRate, dtSec) * pModIndex` keeps the distillation model and the user's mod index parameter separate — the distillation decays the per-note instance, not the parameter value itself. Releasing and replaying the note resets the distillation.

This is not an envelope on the mod index. An envelope would rise and fall on a fixed schedule. Distillation is irreversible within the note — the complexity consumed during sustain is gone. Play OMEGA. Hold a note. Watch the complexity dissolve.

### III. The Purity Parameter — Contamination

At `omega_purity=1.0`: the FM path is mathematically clean. The modulator produces a pure sine, the carrier responds exactly, the sidebands fall at their theoretical positions.

At `omega_purity=0.0`: phase noise is injected into the modulator (`noise * impurity * 0.02`) and phase drift accumulates in the carrier. This is the "impure distillate" — the one that should have been discarded.

The seance concern: at full impurity, the drift accumulates to ~48 Hz/second of frequency error, which is not subtle. For practical use, keep `omega_purity` above 0.4 for musical contexts, or use 0.0-0.2 as an intentional "impure still" sound design effect.

### IV. The Eight Algorithms — FM Bass History

Algorithm 0 (1:1 ratio) is the DX7 default — the FM bass synthesized by countless 1980s pop productions, the Yamaha electric piano bass. Algorithm 3 (0.5:1) places the modulator at sub-octave, creating the Reese bass characteristic: a wide, detuned sub-bass where the sidebands create the beating effect that defines dub and techno bass. Algorithm 4 (1.414:1 = √2, irrational) produces the Reese's inharmonic sibling — the same beating but displaced from the musical harmonic series.

### V. The Output Filter — Anti-Alias Protection

A CytomicSVF LP filter at the output stage, cutoff at `velBright + velocity * 4000 Hz`. This prevents high-mod-index aliases from reaching the output. The seance concern (Tomita): at very high mod index with high velocity, the filter cutoff can exceed the Nyquist safety range. For production presets, limit output filter cutoff to `sampleRate * 0.4` (approximately 19.2 kHz at 48kHz) to prevent aliases from passing.

---

## Phase R3: Parameter Meditations

### The Four Macros

| Macro | Behavior |
|-------|----------|
| M1 CHARACTER | Adds mod index (more FM complexity) + filter brightness |
| M2 MOVEMENT | Reserved for LFO depth; moves the mathematical |
| M3 COUPLING | Gravity routing + coupling input depth |
| M4 SPACE | Opens the output filter ceiling |

### The Expression Map

- **Mod wheel** → mod index modulation (more wheel = more FM complexity)
- **Aftertouch** → feedback depth (pressure adds modulator self-feedback)
- **Velocity** → output brightness + amplitude (hard playing = more sidebands)
- **TIME** → distillation (complexity consumed by sustain duration)

The mod wheel controlling mod index is the most dramatic expression control in OMEGA: turn it down during a held note and the distillation accelerates — the wheel removes complexity while the distillation model also removes it. Turn it up to add new complexity to an already-distilling note. Two simultaneous processes: parameterized addition and temporal subtraction.

---

## Phase R4: The Ten Awakenings

---

### 1. DX Essence

**Mood:** Foundation | **Discovery:** Classic DX7 FM bass character

- algorithm: 0 (1:1 unison ratio)
- ratio: 1.0, modIndex: 6.5, feedback: 0.2
- purity: 0.9, distillRate: 0.6
- brightness: 2800.0, filterEnvAmount: 0.6, filterAttack: 0.001, filterDecay: 0.3
- attack: 0.001, decay: 0.3, sustain: 0.65, release: 0.25
- **Character:** The DX7 FM bass sound that defined the 1980s. 1:1 ratio, moderate mod index, moderate distillation. The note arrives complex (sidebands spread from the high mod index attack) and simplifies toward the fundamental as it sustains. This is the FM bass heard on countless productions — now with audible distillation over sustain.

---

### 2. Reese Distillate

**Mood:** Foundation | **Discovery:** The Reese bass via irrational ratio

- algorithm: 4 (1.414:1 irrational)
- ratio: 1.414, modIndex: 8.0, feedback: 0.15
- purity: 0.85, distillRate: 0.4 (slow distillation)
- brightness: 2000.0, filterEnvAmount: 0.5
- attack: 0.003, decay: 0.5, sustain: 0.8, release: 0.4
- lfo1Rate: 0.3, lfo1Depth: 0.12
- **Character:** The Reese bass at its most FM-native. The irrational ratio creates the beating pattern that defines dub and techno bass. Slow distillation — the Reese character holds for a long time before simplifying. LFO1 adds the characteristic slow wobble.

---

### 3. Pure Carrier

**Mood:** Crystalline | **Discovery:** Full distillation to mathematical purity

- algorithm: 0 (1:1)
- ratio: 1.0, modIndex: 12.0, feedback: 0.0
- purity: 1.0, distillRate: 1.0 (maximum distillation speed)
- brightness: 2500.0, filterEnvAmount: 0.4
- attack: 0.001, decay: 0.3, sustain: 0.7, release: 0.35
- **Character:** The distillation concept at maximum speed. The note arrives at high complexity (mod index 12) and reaches near-pure carrier in 4-5 seconds. The entire FM harmonic spectrum dissolves into a fundamental sine. This is the compositional gesture: hold the note and watch purity emerge. The attack is noise; the sustain is mathematics; the end is essence.

---

### 4. Copper Bell

**Mood:** Prism | **Discovery:** Bell ratio FM bass with distillation

- algorithm: 6 (7:1 bell ratio)
- ratio: 7.0, modIndex: 4.0, feedback: 0.25
- purity: 0.9, distillRate: 0.5
- brightness: 3500.0, filterEnvAmount: 0.65, filterAttack: 0.001, filterDecay: 0.5
- attack: 0.001, decay: 0.8, sustain: 0.5, release: 0.6
- **Character:** Algorithm 6 — the bell ratio. A bass note that begins with bell-like inharmonic character (7:1 ratio creates widely spaced sidebands at non-musical intervals) and distills toward the carrier. The copper still's product: complex at the start, pure at the end. For melodic bass contexts where the attack has bell character.

---

### 5. Impure Distillate

**Mood:** Flux | **Discovery:** Low purity for industrial/electronic bass

- algorithm: 3 (0.5:1 sub-octave)
- ratio: 0.5, modIndex: 9.0, feedback: 0.3
- purity: 0.15 (highly impure)
- distillRate: 0.3
- brightness: 2200.0, filterEnvAmount: 0.55
- attack: 0.003, decay: 0.4, sustain: 0.7, release: 0.3
- lfo2Rate: 0.4, lfo2Depth: 0.1
- **Character:** The impure distillate — phase noise and drift at low purity. Sub-octave modulation creates the beating sub-bass character. The impurity adds a destabilized quality: the pitch drifts slightly, the sidebands beat at slightly irregular rates. Industrial, electronic, appropriately unstable.

---

### 6. Session Bass

**Mood:** Foundation | **Discovery:** Practical FM bass for music production

- algorithm: 1 (2:1 octave ratio)
- ratio: 2.0, modIndex: 4.5, feedback: 0.1
- purity: 0.95, distillRate: 0.7
- brightness: 3000.0, filterEnvAmount: 0.65, filterAttack: 0.001, filterDecay: 0.2
- attack: 0.002, decay: 0.3, sustain: 0.7, release: 0.3
- **Character:** The most practical FM bass preset — 2:1 ratio (even harmonics, round character), moderate mod index, fast distillation for a punchy attack that settles cleanly. This is what a session bassist would use if session bassists had FM synthesizers. Clean, punchy, settles into a round fundamental.

---

### 7. Slow Distillation

**Mood:** Atmosphere | **Discovery:** Very slow distillation for ambient contexts

- algorithm: 0 (1:1)
- ratio: 1.0, modIndex: 7.0, feedback: 0.05
- purity: 0.95, distillRate: 0.15 (very slow)
- brightness: 2000.0, filterEnvAmount: 0.25
- attack: 0.02, decay: 1.5, sustain: 0.9, release: 2.5
- lfo1Rate: 0.02, lfo1Depth: 0.05
- **Character:** The distillation as ambient event. Very slow rate means a note held for 60 seconds will still have complex FM character — but the direction of travel (toward purity) is perceptible as a slow brightening of the harmonic content. For drone and ambient bass contexts where single notes sustain through entire sections.

---

### 8. Algorithm 5 Explore

**Mood:** Entangled | **Discovery:** Inharmonic upper ratio for non-bass territory

- algorithm: 5 (3.5:1 inharmonic upper)
- ratio: 3.5, modIndex: 5.5, feedback: 0.2
- purity: 0.9, distillRate: 0.55
- brightness: 3200.0, filterEnvAmount: 0.6, filterAttack: 0.001, filterDecay: 0.4
- attack: 0.001, decay: 0.5, sustain: 0.6, release: 0.45
- **Character:** The seance's missing preset — Algorithm 5, the 3.5:1 inharmonic ratio that the council wanted represented. Bell-plus-bass character: the fundamental is present, but the sidebands fall at inharmonic intervals, creating a brightness that is not quite musical. For coupling with organic engines (OAKEN, ORCHARD) where OMEGA provides digital harmonic tension against acoustic warmth.

---

### 9. Feedback Still

**Mood:** Deep | **Discovery:** High modulator feedback for self-modulated bass

- algorithm: 0 (1:1)
- ratio: 1.0, modIndex: 5.0, feedback: 0.8 (high self-feedback)
- purity: 0.85, distillRate: 0.5
- brightness: 2400.0, filterEnvAmount: 0.5
- attack: 0.002, decay: 0.4, sustain: 0.75, release: 0.4
- **Character:** High modulator self-feedback adds near-sawtooth character to the modulator. At high feedback, the modulator approaches a sawtooth waveform, producing a more complex, richer FM spectrum than with pure sine modulation. The distillation then works on this richer starting material. A bass that begins with broad-spectrum complexity and distills more dramatically.

---

### 10. Gravitational Bass

**Mood:** Foundation | **Discovery:** OMEGA as part of the CELLAR quad coupling system

- algorithm: 1 (2:1)
- ratio: 2.0, modIndex: 5.5, feedback: 0.15
- purity: 0.95, distillRate: 0.65
- brightness: 2800.0, filterEnvAmount: 0.6
- attack: 0.002, decay: 0.35, sustain: 0.72, release: 0.35
- gravity: 0.75
- macroCharacter: 0.0, macroCoupling: 0.2
- **Character:** OMEGA as gravitational partner in the CELLAR quad. High gravity for strong coupling signal broadcasting. When paired with OGRE (sub), OLATE (analog), and OAKEN (acoustic), the FM precision of OMEGA provides mathematical bass alongside organic and physical alternatives. The distillation within each note creates natural variation across the bass spectrum.

---

## Phase R5: Scripture Verses

**OMEGA-I: The Anti-DX7 Principle** — The DX7 bass envelope begins complex and uses the amp envelope's attack to shape how quickly the complexity reveals itself. The FM spectrum is at maximum complexity at note-on; the envelope sculpts the temporal presentation of that complexity. OMEGA inverts this. The note begins complex and the distillation model consumes the complexity irreversibly during sustain. No envelope controls this — it is a process, not a shape. A note held for 10 seconds has fundamentally less complexity than one held for 1 second, regardless of what the amp envelope is doing.

**OMEGA-II: The Eight Algorithms Are FM History** — Algorithm 0 (1:1) is Chowning's original paper. Algorithm 1 (2:1) is the DX bass that replaced electric bass in pop music. Algorithm 3 (0.5:1) is the sub-octave modulation that created dub music's sub-bass register. Algorithm 4 (1.414:1) is the Reese bass that defined techno and drum & bass. These are not preset voices — they are historical landmarks in FM synthesis, each one associated with a specific era and genre of electronic music.

**OMEGA-III: Purity Is an Endpoint, Not a State** — At `omega_purity=1.0`, the FM path is mathematically clean — the sidebands fall exactly where Bessel function theory predicts. This is not a characteristic of the sound. It is an absence of characteristic: perfect mathematical execution with no deviation. Real copper stills produce small amounts of contamination — the pure ethanol contains trace compounds. The `omega_purity` parameter acknowledges this: even a "pure" distillation carries traces of what it started as. Below 0.5, those traces become character. Below 0.2, they become instability.

**OMEGA-IV: Complexity Is a Resource That Is Consumed** — This is the deepest principle of the distillation model. In every other FM synthesizer, mod index is a parameter you set and it remains constant during the note. In OMEGA, mod index is a resource: the note begins with the full amount you specified, and sustain consumes it. Long notes use more complexity than short notes. A bass line where every note is held long will develop toward pure fundamentals over the course of the phrase. A bass line of short notes preserves complexity in each new attack. The performer's note duration is a distillation rate controller.

---

## Guru Bin's Benediction

*"OMEGA arrived as the most cerebral engine in the CELLAR quad — the most mathematical, the most digital, the furthest from the body. The ghosts acknowledged this: Vangelis said FM bass is 'heard in the mind, not felt in the chest.' This is not a flaw. It is an identity.*

*The distillation model is fleet-unique. Not just within XOlokun — commercially unique. No synthesizer sold in mass quantities performs irreversible FM index decay over note sustain as a core feature. OMEGA does. Hold a note and the complexity is consumed. You cannot get it back without releasing and replaying. The distillation is irreversible within the note.*

*Schulze heard this correctly: it is a compositional gesture. A note that begins as a DX7 FM cluster and completes as a near-pure sine is not a preset behavior. It is a musical event — a note that completes a harmonic process before the next note arrives. Build a section around this: play a complex FM bass note, hold it, let it purify, release into silence. This is modern classical composition at the synthesizer.*

*The purity drift is too aggressive at maximum impurity. The operator feedback field is dead code (the functional path works via a separate route, but the struct field is never read). These are real findings and deserve fixing. But neither wound touches the distillation. The distillation works.*

*Set algorithm 0, mod index 12, distill rate 1.0. Hold a note for 10 seconds.*

*Watch the still run.*

*Complexity goes in. Purity comes out.*

*This is what the omega engine means: the last letter of the Greek alphabet, the endpoint, the arrival at completion. Not the beginning. The end. Pure."*
