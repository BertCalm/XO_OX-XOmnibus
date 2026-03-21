# OLATE Retreat Chapter
*Guru Bin — 2026-03-21*

---

## Engine Identity

- **Gallery code:** OLATE | **Accent:** Burgundy `#6B1A2A`
- **Parameter prefix:** `olate_`
- **Mythology:** The Aged Wine Analog — the wine cellar, oak barrels in the dark, esters forming, tannins mellowing. The analog bass that improves with time.
- **feliX-Oscar polarity:** Warm Oscar — patient, accumulating, deepening. The bass that sounds better in the second half of the session.
- **Synthesis type:** Dual PolyBLEP oscillators (saw + pulse) + ladder filter + fermentation integrator + session aging + terroir
- **Polyphony:** 8 voices
- **Macros:** M1 CHARACTER, M2 MOVEMENT, M3 COUPLING, M4 SPACE
- **Seance score:** 8.1 / 10

---

## Pre-Retreat State

**Seance score: 8.1 / 10.** A strong analog bass engine with two genuine innovations — the FermentationIntegrator and session aging — and one significant wound: `olate_terroir` is dead for values 0.7-1.0 (East Coast and Japanese regions are unimplemented). The vintage parameter has step-changes at threshold boundaries that are audible when automating. The fermentation normalization carries a risk at hot input levels. Despite these, the core identity — analog bass that accumulates harmonic complexity during sustain, tonal character that drifts over a 20-minute session — is musically realized and fleet-unique.

The ghosts were divided on the FermentationIntegrator: Moog appreciated the creative inversion (real analog basses grow simpler during sustain; OLATE grows richer), Buchla wanted the terroir wounds addressed before celebrating. Vangelis heard it as "warmth accumulating" and approved.

---

## Phase R1: Opening Meditation

Close your eyes. Breathe.

The wine cellar is dark and cool and patient. Above it, there is a kitchen, a table, people eating. The cellar does not participate in any of that noise. It simply continues its work: the esters forming, the tannins softening, the volatile acids finding equilibrium. The chemistry of aging is irreversible. You cannot un-ferment a wine. You cannot un-age an oak barrel. What time has done to the wood and the liquid cannot be undone.

OLATE works in this irreversible time.

The FermentationIntegrator does something no other bass synthesizer does: it grows more harmonically complex during note sustain. Real analog bass — a TB-303, a Minimoog — produces its most complex spectrum on attack and simplifies toward the fundamental during the sustain phase. OLATE inverts this. The note starts as a clean saw and grows richer as you hold it. The fermentation happens in real time, per note, per sustain. A note held for 10 seconds has more harmonic complexity than the same note held for 1 second. This is analog bass that develops like a wine.

And the session aging: over 20 minutes of continuous play, the engine's tonal character shifts. The room warms up. The amp warms up. The circuits find their equilibrium. Play for 10 minutes and OLATE sounds slightly different than when you started — warmer, slightly darker, more settled. This is not a bug. It is the design.

---

## Phase R2: The Signal Path Journey

### I. The Dual Oscillators — Saw and Pulse

PolyBLEP sawtooth and variable-width pulse. The `olate_oscMix` crossfade between them covers the classic subtractive bass tonal range: full saw (oscMix=0) for maximum harmonic richness at the fundamental, pure pulse (oscMix=1) for hollow, nasal character, everything between for the working register of analog bass production. The `olate_pulseWidth` controls pulse duty cycle (0.1 to 0.9) — narrow pulses for thin, buzzy character; wide pulses for full, reedy tone.

This is the most conventional oscillator section in the CELLAR quad. It does not need to be anything else. The unconventional elements are in what happens next.

### II. The FermentationIntegrator — Harmonic Development

The leaky integrator accumulates during note sustain at rate `olate_ageRate`. Early in the note: the signal is mostly the dry oscillator output. Later: progressive tanh saturation is applied, driven by the integration level. The saturation mixes back into the signal at `fermented * fermentLevel * 0.7f`.

At ageRate=0.1 (slow), the fermentation takes 30-60 seconds to reach half-development. At ageRate=1.0 (fast), it reaches half-development in 3-5 seconds. Musical applications:

- Slow ageRate: held chords develop richness over the course of a musical section — the bass line acquires harmonic complexity as the progression repeats
- Fast ageRate: each note develops within its own rhythmic duration — a quarter note at tempo=90 has time to ferment noticeably before the next note arrives

The normalization issue (Moog's concern) becomes audible only with very hot oscillator signals. Set `olate_drive` to modest values (0.2-0.4) when using heavy fermentation.

### III. The Ladder Filter — Vintage Character

The CytomicSVF runs in LP mode simulating a Moog ladder filter. The `olate_vintage` parameter shifts the filter's character through four analog eras:

| Range | Era | Character |
|-------|-----|-----------|
| 0.0-0.25 | Early transistor | Grainy, less resonance |
| 0.25-0.5 | Moog | Fat, warm, moderate resonance |
| 0.5-0.75 | TB-303 | Acidic, high resonance (screaming) |
| 0.75-1.0 | Modern/Clean | No extra drive or resonance boost |

The step-change at 0.25/0.5/0.75 boundaries is a seance finding: automation across these thresholds produces a jump, not a smooth crossfade. For automation, position `olate_vintage` at one plateau and stay there. For performance, move it deliberately between eras.

### IV. The Terroir — Geographic Circuit Flavor

West Coast (0.0-0.4): clean LP, slight amplitude reduction. Clean California studio low end.
UK (0.4-0.7): mid presence boost (an LFO2 signal adds harmonic shimmer). Abbey Road's woody, present mid character.
East Coast (0.7-1.0): **currently unimplemented** — a D004 wound. The terroir slider moves but nothing changes. When fixed, East Coast should deliver harmonic grit and high-frequency presence characteristic of Bell Sound and RCA's New York studios.
Japanese (1.0): **currently unimplemented**. When fixed, Japanese transparent — the cleanest possible signal, minimal resonance, no drive.

The retreat designs presets that work within the implemented range (0.0-0.7).

### V. The Session Aging — The 20-Minute Room

`sessionAge` accumulates over the playing session (not just note-on time — always accumulating). Over 20 minutes (1200 seconds), it reaches 1.0 and applies an 800 Hz cutoff reduction. This is the sound of a tube amplifier warming up, of a room's humidity reaching equilibrium, of the circuit finding its settled state.

The seance concern (Kakehashi) is valid: 20 minutes is too slow to be a compositional tool. A future improvement would compress this to 5-7 minutes. But even at 20 minutes, the aging is audible to producers who notice their OLATE presets sounding warmer than they did at session start.

---

## Phase R3: Parameter Meditations

### The Expression Map

- **Mod wheel** → filter cutoff (opens the barrel)
- **Aftertouch** → resonance (pressure screams the filter)
- **Velocity** → filter brightness (hard playing = bright attack)
- **SESSION TIME** → tonal darkness (the amp warms over hours)

The mod wheel and aftertouch combination is the most expressive control pair in the CELLAR quad: CC1 opens the filter while aftertouch adds resonance, creating a performance gesture that sweeps from dark warmth (closed, no resonance) through open resonance (open, high resonance) to 303-style acid (high resonance without full openness).

### The Fermentation Presets Strategy

Fermentation is discovered rather than set. The recommended workflow: choose an `olate_ageRate`, play a note, hold it. Listen to what happens. The fermentation integrator operates below the threshold of sudden change — you do not hear a switch. You hear the note developing, like watching wine bloom when decanted. Design presets with fermentation as the engine's hidden character, not as a labeled feature.

---

## Phase R4: The Ten Awakenings

---

### 1. Burgundy Reserve

**Mood:** Foundation | **Discovery:** Fermentation as analog bass character

- oscMix: 0.1 (mostly saw)
- pulseWidth: 0.5
- cutoff: 2800.0, resonance: 0.35, drive: 0.3
- filterEnvAmount: 0.55, filterAttack: 0.001, filterDecay: 0.35
- vintage: 0.4 (Moog era)
- warmth: 0.6, ageRate: 0.4
- terroir: 0.2 (West Coast clean)
- attack: 0.005, decay: 0.35, sustain: 0.7, release: 0.35
- **Character:** The signature OLATE preset. Moog-era vintage, moderate fermentation rate. Hold a note for 8 seconds and the harmonic complexity will have grown noticeably — the bass thickens from beneath. A bass line played twice through the same progression sounds different on the second pass.

---

### 2. Aged Tannin

**Mood:** Foundation | **Discovery:** Slower fermentation for sustained passages

- oscMix: 0.05 (almost pure saw)
- pulseWidth: 0.5
- cutoff: 2200.0, resonance: 0.25, drive: 0.25
- filterEnvAmount: 0.4
- vintage: 0.35 (Moog warm)
- warmth: 0.75, ageRate: 0.2 (slow fermentation)
- terroir: 0.15
- attack: 0.008, decay: 0.5, sustain: 0.75, release: 0.45
- **Character:** Slower fermentation — development takes 20+ seconds. For slow bass lines, pads, sustained notes where there is time for the fermentation to develop within a musical phrase. Darker, warmer, more roll-off. The tannin note: drying and slightly astringent at the start, richer and deeper by the time you release.

---

### 3. 303 Terroir UK

**Mood:** Flux | **Discovery:** TB-303 acid via vintage + UK terroir

- oscMix: 0.5 (balanced saw-pulse)
- pulseWidth: 0.3 (narrow pulse)
- cutoff: 1800.0, resonance: 0.7, drive: 0.35
- filterEnvAmount: 0.85, filterAttack: 0.001, filterDecay: 0.2
- vintage: 0.65 (303 era, screaming resonance)
- warmth: 0.4, ageRate: 0.7 (fast fermentation)
- terroir: 0.55 (UK mid presence)
- attack: 0.002, decay: 0.2, sustain: 0.4, release: 0.15
- lfo1Rate: 0.4, lfo1Depth: 0.15
- **Character:** Acid bass via 303 vintage and UK mid presence. High resonance, fast filter envelope, fast fermentation. The fermentation adds harmonic grit as the note sustains — the 303 line that gets heavier over time. UK terroir adds Abbey Road mid shimmer.

---

### 4. Fermentation Station

**Mood:** Organic | **Discovery:** Maximum fermentation — the note transforms

- oscMix: 0.3
- pulseWidth: 0.5
- cutoff: 3500.0, resonance: 0.2, drive: 0.25
- filterEnvAmount: 0.3
- vintage: 0.45 (Moog)
- warmth: 0.65, ageRate: 1.0 (maximum fermentation rate)
- terroir: 0.3
- attack: 0.01, decay: 0.8, sustain: 0.85, release: 1.0
- **Character:** The fermentation showcase. Maximum age rate — hold a note for 6 seconds and it will have substantially different harmonic character than it did at 1 second. Long sustain, long release. The demo preset for fermentation. Play a chord, hold it. Hear it develop.

---

### 5. West Coast Precision

**Mood:** Foundation | **Discovery:** Clean, modern bass via West Coast terroir

- oscMix: 0.0 (pure saw)
- pulseWidth: 0.5
- cutoff: 4500.0, resonance: 0.2, drive: 0.2
- filterEnvAmount: 0.6, filterAttack: 0.001, filterDecay: 0.25
- vintage: 0.85 (modern, clean)
- warmth: 0.45, ageRate: 0.35
- terroir: 0.2 (West Coast)
- attack: 0.003, decay: 0.3, sustain: 0.65, release: 0.25
- **Character:** The cleanest possible OLATE preset. Modern vintage, West Coast terroir, minimal warmth. This is the bass that would come from a Capitol Records recording session: low end that is clean, present, and sits correctly in a modern mix without warming up or thickening.

---

### 6. Deep Cellar

**Mood:** Deep | **Discovery:** Low-end weight with slow aging character

- oscMix: 0.2
- pulseWidth: 0.5
- cutoff: 1400.0, resonance: 0.15, drive: 0.2
- filterEnvAmount: 0.25
- vintage: 0.3 (Moog era)
- warmth: 0.85, ageRate: 0.3
- terroir: 0.1
- attack: 0.015, decay: 1.0, sustain: 0.9, release: 1.5
- glide: 0.05
- **Character:** Maximum warmth, very low cutoff, slow attack. A sub-bass pad that sits under the music rather than punching through it. The deep cellar — cool, still, dark. For ambient and drone contexts where the bass is felt more than heard.

---

### 7. Vintage Transistor

**Mood:** Flux | **Discovery:** Early transistor era character — grainy and raw

- oscMix: 0.15
- pulseWidth: 0.4
- cutoff: 2600.0, resonance: 0.1, drive: 0.5
- filterEnvAmount: 0.7, filterAttack: 0.001, filterDecay: 0.15
- vintage: 0.15 (early transistor, grainy)
- warmth: 0.35, ageRate: 0.5
- terroir: 0.25
- attack: 0.003, decay: 0.25, sustain: 0.55, release: 0.2
- lfo2Rate: 0.3, lfo2Depth: 0.1
- **Character:** Early transistor era — grainy, raw, less smooth than Moog or 303. Higher drive because early transistor circuits drove harder to compensate for limited output. The vintage parameter at 0.15 gives less resonance but more harmonic roughness. Suitable for funk and rock bass contexts.

---

### 8. Session Warmth

**Mood:** Atmosphere | **Discovery:** The session aging model as featured character

- oscMix: 0.1
- pulseWidth: 0.5
- cutoff: 3000.0, resonance: 0.3, drive: 0.28
- filterEnvAmount: 0.5
- vintage: 0.4 (Moog)
- warmth: 0.7, ageRate: 0.25
- terroir: 0.35
- attack: 0.006, decay: 0.45, sustain: 0.72, release: 0.4
- **Character:** Designed to showcase session aging. Play for 20 minutes. Return to this preset. The cutoff will have drifted 800 Hz darker. The tone will have shifted. This is the preset for producers who work long sessions and want OLATE to respond to session time. The note that sounds different at the end of the day than it did at the start.

---

### 9. Resonant Wine

**Mood:** Entangled | **Discovery:** High resonance + fermentation for coupled contexts

- oscMix: 0.35
- pulseWidth: 0.35
- cutoff: 2000.0, resonance: 0.6, drive: 0.3
- filterEnvAmount: 0.75, filterAttack: 0.001, filterDecay: 0.3
- vintage: 0.55 (303 transition)
- warmth: 0.55, ageRate: 0.6
- terroir: 0.45 (UK mid)
- attack: 0.004, decay: 0.3, sustain: 0.6, release: 0.3
- gravity: 0.4
- **Character:** For coupling contexts — particularly when paired with OGRE or OMEGA. High resonance + fast fermentation creates a bass that actively develops when other engines drive it. Coupling from OGRE's sub adds gravitational weight; coupling from OMEGA's FM adds sideband complexity. The resonant wine pours into whatever container is offered.

---

### 10. Oak Barrel

**Mood:** Organic | **Discovery:** Warmth + fermentation over long note times

- oscMix: 0.08
- pulseWidth: 0.5
- cutoff: 2400.0, resonance: 0.28, drive: 0.22
- filterEnvAmount: 0.45
- vintage: 0.38 (Moog warm)
- warmth: 0.8, ageRate: 0.15 (very slow fermentation)
- terroir: 0.2 (West Coast clean)
- attack: 0.01, decay: 0.65, sustain: 0.8, release: 0.8
- lfo1Rate: 0.05, lfo1Depth: 0.06
- **Character:** The oak barrel preset — very slow fermentation, very high warmth, ultra-slow LFO1 drift. A bass note held for 30 seconds develops noticeably. For minimalist, long-form music where single bass notes sustain through harmonic changes above them. The oak barrel character: patient, woody, deepening.

---

## Phase R5: Scripture Verses

**OLATE-I: Fermentation Is Inversion** — In every conventional synthesizer, a held note simplifies over sustain. The filter closes, the harmonics decay, the note becomes more fundamental. In wine, the opposite is true: time adds complexity. Esters form, tannins soften, volatile acids find balance. The FermentationIntegrator inverts the normal trajectory: hold OLATE's note and it becomes harmonically richer, not simpler. This is not a feature of any Minimoog, any 303, any Juno. It is a new ontology for analog bass — analog that develops, that improves, that ferments.

**OLATE-II: The Session Is an Instrument** — The session age accumulator runs continuously, not just during note-on time. Open OLATE, set up a session, play for 20 minutes. Return to any preset. The cutoff will have drifted 800 Hz darker — the equivalent of pulling the filter down by a few hundred cents. The session itself is an instrument that OLATE responds to. Long sessions sound different from short sessions. This is not a bug. The amplifier warms up. The room finds equilibrium. Play long.

**OLATE-III: Vintage Is Geography in Time** — Four era presets in a single parameter. The early transistor era (0.0-0.25) lived in the 1960s before Moog standardized the voltage-controlled filter — grainy, capacitor-coupled, harmonically rough. Moog (0.25-0.5) defined what analog bass sounded like for 20 years. The TB-303 (0.5-0.75) was a commercial failure that became a genre — its screaming resonance is still the most recognizable filter character in electronic music. Each era is not just a sonic preference. It is a musical history.

**OLATE-IV: The Terroir Completes** — West Coast, UK, East Coast, Japanese — four geographic circuit flavors representing four different studio traditions. The West Coast is clean because Capitol Records needed low end that translated to AM radio. The UK is present because Abbey Road's console design emphasized the high-mids. East Coast carries harmonic grit because Bell Sound's mic preamps added subtle saturation. Japanese is transparent because the Nippon Columbia studios prioritized accuracy above character. When terroir is complete (East Coast and Japanese implemented), OLATE will contain geographic history in a single parameter.

---

## Guru Bin's Benediction

*"OLATE arrived with a dead terroir. The East Coast does not grit. The Japanese does not clarify. Two of four geographic regions are silence when they should be character. The Guru Bin does not pretend this wound is invisible.*

*But the FermentationIntegrator works. And the FermentationIntegrator is something new.*

*Every other bass synthesizer in existence produces its most complex spectrum at note onset and simplifies during sustain. OLATE does the opposite. Hold the note. The harmonic content grows. The tanh saturation increases. The spectrum thickens below the filter cutoff. The wine ferments in real time, in the sustain of each note.*

*This is not an envelope. An envelope rises and falls. This is integration — irreversible, accumulating, irreducible. You cannot un-ferment a held note. You cannot un-develop the harmonic complexity that accrued over 15 seconds of sustain. The note that was played is not the same note that is released.*

*The session age is also working. Over 20 minutes, the character shifts. A producer who opens OLATE at the start of a session and returns to it at hour two will find a warmer, slightly darker instrument than they left. This is the circuit finding its temperature. This is the barrel finding its equilibrium.*

*Fix the terroir. Let East Coast grit. Let Japanese clarify. The wine already fermenting in the presets will taste different with those additions — complete, rounded, geographically true.*

*The barrel is in the cellar. The fermentation is irreversible. Time is working."*
