# OVEN Retreat Chapter
*Guru Bin — 2026-03-21*

---

## Engine Identity

- **Gallery code:** OVEN | **Accent:** Cast Iron Black `#2C2C2C`
- **Parameter prefix:** `oven_`
- **Creature mythology:** A 9-foot concert grand piano cast entirely in iron — a Steinway D forged not from spruce and maple but from the same metal as Dutch ovens and engine blocks. It sits in an empty hall at midnight, massive and dark, absorbing hammer energy into its enormous thermal mass and releasing it slowly as warm, sustaining resonance. Nothing escapes quickly. Nothing is wasted. The cast iron traps everything.
- **Synthesis type:** Modal synthesis — 16 IIR modal resonators per voice, Hunt-Crossley hammer model, micro-rebound, Bloom envelope, HF noise fill, 12-string sympathetic network (shared post-mix)
- **Polyphony:** 8 voices
- **Macros:** M1 CHARACTER (density + brightness), M2 MOVEMENT (LFO depth), M3 COUPLING (competition/resonance), M4 SPACE (sympathetic + bloom)

---

## Pre-Retreat State

XOven arrived in XOlokun as one of the Kitchen Quad — four modal piano engines built around physical material properties. It holds the warmest, darkest, most massive corner of the quad: cast iron at acoustic density Z=36.72 MRayl, thermal coefficient 0.0003/°C, inharmonicity coefficient B≈0.0004. The Seance Council scored it 8.65/10, with Moog praising the Bloom envelope as "a stroke of genius" and noting that "the cast iron metaphor is the most correct physical analogy I have seen in a software synthesizer."

Ten presets exist. The Seance Council identified gaps: nothing exploring mid-register darkness, nothing percussive-drone, nothing truly slow and geological. The preset library was scored 6/10 against an engine the Council rated 9/10 for sound quality. This retreat closes that gap.

The Bloom envelope is the defining character feature. `oven_bloomTime` controls how slowly the resonance rises after the initial strike — at 0.0, the attack is immediate; at 1.0, the fundamental takes nearly two seconds to reach full amplitude, imitating the slow thermal mass response of a cast iron body absorbing energy. The micro-rebound (a secondary impulse at 20–35ms from the iron body's slow reflection) distinguishes XOven from every other piano synthesizer in the fleet.

---

## Phase R1: Opening Meditation

Close your eyes. Breathe.

Cast iron is not beautiful in the way bronze is beautiful. It is not decorative. It is the metal of function — of cookware, of engine blocks, of bridges. It is heavy, dark, slightly porous, prone to rust if left untended. When you forge a piano from cast iron, you are not making an elegant object. You are making a fact.

And yet: strike it. A single note in an empty concert hall. The sound that emerges is not thin or industrial. It is warm, massive, sustaining — because cast iron's enormous acoustic impedance (Z=36.72 MRayl) means that when a string vibrates against a cast iron frame, most of the energy reflects back. The string and the body argue. The body wins. And in winning, it absorbs the argument entirely, holding the energy inside itself for seconds, releasing it as warmth.

This is the Bloom. The initial strike is percussive — the hammer hits, the string excites the first mode. But the cast iron body hasn't fully responded yet. Its thermal mass is too great. The lower modes begin to ring before the body even knows they've been excited. Then, slowly — twenty milliseconds, a hundred milliseconds, half a second — the body warms to the note. The resonance blooms. The warmth arrives after the attack.

There is a kind of music that requires exactly this: music where the note is still arriving when the next one begins. Music where notes pile into each other not as chords but as layers of warming. Arvo Pärt's tintinnabuli. Morton Feldman's long, sustained pianissimo. The Bloom parameter is their instrument.

Now play one note. Hold the sustain pedal. Let it ring.

That is where XOven lives.

---

## Phase R2: The Signal Path Journey

### I. The Hammer — Hunt-Crossley Contact Model

Every note begins with a collision. `OvenHammerModel` implements Hunt and Crossley's nonlinear contact mechanics (1975): a half-sine pulse whose duration scales inversely with effective hardness (`8ms` at soft → `1ms` at hard), followed by a noise component representing the Maillard "char" — the spectral crust of a hard hammer strike.

`oven_hardness` (0.0–1.0, default 0.4) controls the hammer felt. At 0.0, the contact time is 8ms and noise mix is near zero — a warm, rounded pulse that excites only the lower modes. At 1.0, contact is 1ms and noise mix climbs to 30% of the pulse amplitude — a sharp, bright strike that excites every mode.

The micro-rebound fires at 20–35ms with 15% of the peak amplitude when velocity exceeds 0.3. This models the cast iron body's slow response: the initial impulse propagates through the dense material and reflects back with a slight delay. Hard hammers reduce the rebound amplitude because their shorter contact time creates less initial body excitation. This mechanism is unique to XOven in the fleet.

**Playing insight:** Hardness 0.2–0.35 is the expressive center. At this range, velocity control is most transparent — soft notes are genuinely softer in timbre (not just amplitude), and the micro-rebound is at its most audible. Hardness above 0.65 produces a more percussive, piano-like attack that suits staccato playing but loses the iron character.

### II. The Modal Bank — 16 Resonators Per Voice

Sixteen second-order IIR resonators derive their frequencies from cast iron's inharmonicity coefficient (B≈0.0004):

```
f_n = f_1 × n × sqrt(1 + B × n²)
```

The result: mode 2 is at 2.005×f₁ (nearly octave), mode 3 at 3.018×f₁, stretching further with each mode. The Audsley amplitude envelope weights the fundamental strongest and rolls off with slight formant boost at modes 3–5. Sixteen modes captures the audible character; modes above the 16th are represented by the HF noise fill.

`oven_density` (0.0–1.0, default 0.7) controls the Q factor mapping: 150 (low density) → 600 (high density). This is the most important parameter for character — high density makes the iron body thick and sustaining; low density opens the body to faster decay.

`oven_bodyResonance` (0.0–1.0, default 0.6) drives the body cavity resonance post-mix. It is not a filter but a resonant amplitude boost at specific body frequencies — the physical signature of the iron frame itself.

### III. The Bloom Envelope

`oven_bloomTime` (0.0–1.0) is the unique XOven feature. At 0.0, no bloom — immediate response. At 1.0, the exponential attack coefficient creates a slow rise that reaches full amplitude in approximately 1.8 seconds. The coefficient formula:

```
bloomCoeff = exp(−1.0 / (bloomTime × sampleRate × 0.35))  [approximate]
```

Bloom does not affect the initial hammer excitation — the modal bank rings immediately. Bloom modulates the output amplitude of each voice through a slow VCA that fades in after the attack. The musical effect: chord notes arrive sequentially (lowest modes ring first, body resonance arrives later), exactly mimicking the physical response of a very large resonant body.

Bloom interacts beautifully with `oven_sympathetic`: when bloom is slow and sustain is long, the sympathetic strings pick up the note before the bloom has reached full amplitude and re-excite it — creating a secondary swell.

### IV. Sympathetic Network

`oven_sympathetic` (0.0–1.0, default 0.4) controls the 12-string shared post-mix resonator. The network is not per-voice — it is a shared body that all voices drive simultaneously, exactly as a real piano's un-damped strings resonate together when the damper pedal is raised.

At 0.4–0.6, the sympathetic network creates the rich chordal shimmer that makes cast iron piano particularly suited to dense voicings. At 0.8+, the network begins to dominate the decay tail, creating an almost reverb-like sustain that outlasts the individual voices.

**Warning zone:** `sympathetic` > 0.75 with `sustainTime` > 8.0 and `density` > 0.85 creates very long decays that can mask subsequent notes in fast passages. Beautiful for ambient slow music; problematic for anything rhythmic.

### V. Thermal Drift

`oven_temperature` (0.0–1.0, default 0.5) applies Young's modulus temperature correction:

```
E(T) = E₀ × (1 − 0.0003 × T)  →  ~0.8 cents per 10°C
```

At 0.5, thermal drift is subtle — the engine sounds like a well-tuned piano that has been playing for an hour. At 0.9+, the iron has been playing in a very hot room and the fundamental has dropped several cents, creating the characteristic "night after a concert" warmth of an instrument that has thermally settled. Per-voice thermal personality (a fixed random offset set at voice reset) means each of the 8 voices drifts differently, simulating a real instrument's non-uniform thermal response across its compass.

---

## Phase R3: Preset Design

### Awakening Preset 1 — "Slow Iron" (Aether)

The geological-slow type the Council identified as missing. Maximum bloom with very slow attack. Single notes arrive like tidal events.

- bloom 1.0, sustainTime 12.0, sympathetic 0.55, density 0.80, temperature 0.7
- hardness 0.15 (very soft), bodyResonance 0.75
- lfo1Rate 0.04, lfo1Depth 0.06 (breathing)
- DNA: brightness 0.25, warmth 0.95, movement 0.35, density 0.55, space 0.75, aggression 0.05

### Awakening Preset 2 — "Iron Drone" (Deep)

Percussive-drone territory the Council found missing. Short sustain, very high density, fast attack, no bloom. The monolithic struck-metal sound.

- bloom 0.0, sustainTime 1.2, density 0.95, hardness 0.75, bodyResonance 0.85
- sympathetic 0.20, temperature 0.35
- lfo1Rate 0.005 (geological breathing), lfo1Depth 0.04
- DNA: brightness 0.45, warmth 0.55, movement 0.05, density 0.90, space 0.25, aggression 0.55

### Awakening Preset 3 — "Maillard Bloom" (Prism)

The bloom + high hardness combination: a hard initial char strike that slowly blossoms into warm iron resonance. The Maillard reaction as synthesis.

- hardness 0.72, bloomTime 0.65, sustainTime 6.0, density 0.75
- hfAmount 0.60 (char!), sympathetic 0.45, temperature 0.55
- filterEnvAmt 0.55 (bright attack that darkens as bloom rises)
- DNA: brightness 0.60, warmth 0.70, movement 0.20, density 0.65, space 0.45, aggression 0.45

### Awakening Preset 4 — "Mid Register Dark" (Foundation)

The mid-register darkness the Council identified. C3–C4 territory, tuned for the body resonance of an iron frame. Not the full concert grand — the section of the keyboard where cast iron's impedance mismatch creates the most character.

- brightness 1800.0, bodyResonance 0.80, density 0.78
- hardness 0.28, bloomTime 0.15, sustainTime 4.5
- sympathetic 0.50, temperature 0.50
- DNA: brightness 0.22, warmth 0.90, movement 0.10, density 0.72, space 0.50, aggression 0.12

### Awakening Preset 5 — "Forge Strike" (Kinetic)

Maximum hardness, short sustain, aggressive filter envelope. The sound of an actual forging strike — not a musical instrument but an industrial event that happens to be pitched.

- hardness 1.0, bloomTime 0.0, sustainTime 0.8, density 0.60
- hfAmount 0.80, filterEnvAmt 0.75, bodyResonance 0.50
- ampAttack 0.001, ampDecay 0.4, ampSustain 0.1, ampRelease 0.6
- DNA: brightness 0.70, warmth 0.40, movement 0.15, density 0.65, space 0.20, aggression 0.80

### Awakening Preset 6 — "Sympathetic Hall" (Atmosphere)

Maximum sympathetic resonance with generous sustain. The feel of playing in a concert hall where the iron body of the instrument and the room itself become indistinguishable.

- sympathetic 0.75, sustainTime 10.0, density 0.70, bloom 0.35
- bodyResonance 0.90, temperature 0.60
- lfo1Rate 0.06, lfo1Depth 0.05, lfo2Rate 1.2, lfo2Depth 0.03 (shimmer)
- DNA: brightness 0.35, warmth 0.80, movement 0.25, density 0.60, space 0.85, aggression 0.10

### Awakening Preset 7 — "Winter Iron" (Crystalline)

Low temperature setting — the iron has been cold all night. Eigenfrequencies slightly elevated, less sympathetic resonance, more brittle character. The feeling of an instrument that hasn't warmed up yet.

- temperature 0.05, density 0.65, sympathetic 0.30
- hardness 0.45, brightness 4500.0, bodyResonance 0.55
- bloomTime 0.08, sustainTime 3.5
- DNA: brightness 0.55, warmth 0.25, movement 0.10, density 0.60, space 0.40, aggression 0.25

### Awakening Preset 8 — "Thermal Swell" (Flux)

LFO2 modulating bloomTime via the MOVEMENT macro — the bloom depth fluctuates, making each phrase dynamically different. Notes from one bar bloom immediately; notes from the next bloom slowly.

- bloomTime 0.45, sustainTime 7.0, density 0.72
- lfo2Rate 0.18, lfo2Depth 0.25 (MOVEMENT macro controls this)
- sympathetic 0.55, temperature 0.55, bodyResonance 0.70
- DNA: brightness 0.30, warmth 0.80, movement 0.55, density 0.65, space 0.60, aggression 0.15

### Awakening Preset 9 — "Iron Chorale" (Entangled)

Built for coupling — the competition and couplingResonance parameters active, designed to receive coupling input from XOchre. The iron body opens to copper influence: couplingResonance acts as a sympathetic amplifier when another engine drives it.

- competition 0.35, couplingResonance 0.55
- density 0.75, sympathetic 0.60, bloomTime 0.20, sustainTime 6.0
- bodyResonance 0.80, temperature 0.50
- DNA: brightness 0.35, warmth 0.80, movement 0.30, density 0.70, space 0.55, aggression 0.20

### Awakening Preset 10 — "Mellow Rebound" (Organic)

Center of the micro-rebound phenomenon. Hardness tuned to the exact zone where the rebound is most audible (velocity ~0.45, hardness ~0.30). Notes have a subtle double-onset warmth — the primary strike, then the body answering.

- hardness 0.30, bloomTime 0.10, sustainTime 4.0, density 0.70
- bodyResonance 0.65, sympathetic 0.45, temperature 0.50
- hfAmount 0.30, filterEnvAmt 0.35
- DNA: brightness 0.35, warmth 0.85, movement 0.20, density 0.55, space 0.45, aggression 0.10

---

## Phase R4: Scripture

*Four verses for the Cast Iron Concert Grand*

---

**I. The Weight of Resonance**

The iron does not want to ring.
Its density resists, its mass objects.
But you strike it anyway,
and because you struck it,
it has no choice but to answer.

The answer is slow, and warm, and long.
Everything that resists eventually yields.
Everything that yields eventually sings.

---

**II. Maillard**

At high heat, sugar becomes caramel.
At high velocity, felt becomes char.
Both are transformations, not damage.
Both taste better than what came before.

Strike hard. Let the char be the color.
The darkness at the front edge of the note
is not a flaw in the piano —
it is where the heat entered.

---

**III. Bloom**

The pianist struck the key and left.
The note was still arriving
when the door closed behind him.
The hall was empty for twenty minutes.
The note was still arriving.

This is not metaphor.
This is physics.
Thermal mass does not hurry.

---

**IV. Every Hall at Midnight**

It is not the same instrument in the morning.
Temperature has settled the iron,
the body has forgotten the day's playing.
The eigenfrequencies have drifted three cents
from where they were at noon.

This is why you tune before each concert.
This is why the piano in the empty hall
sounds different from the piano
surrounded by an audience's body heat.

XOven knows this. It drifts, too.

---

## Phase R5: Retreat Summary

XOven is the anchor of the Kitchen Quad — the most physically grounded, the most massive, the most sustained. The Bloom envelope is its singular gift to synthesis: a parameter that has no analog in any commercial piano synthesizer and maps directly to a real acoustic phenomenon. The micro-rebound at 20–35ms is the second differentiating feature, audible only at moderate velocities and hardness settings, invisible in percussive playing.

The ten new presets address every gap the Seance Council identified: geological-slow types (Slow Iron), percussive-drone territory (Iron Drone), mid-register darkness (Mid Register Dark), and coupling-ready presets (Iron Chorale). The Maillard Bloom preset demonstrates the char+bloom interaction that defines the engine's most distinctive character combination.

XOven's optimal zone: `hardness` 0.20–0.45, `bloomTime` 0.15–0.65, `sustainTime` 4.0–10.0, `sympathetic` 0.40–0.65. Outside these ranges the engine becomes either indistinguishable from a conventional piano (low everything) or unusably sustained (all at maximum). Inside this zone, every combination yields a unique variation of the cast iron character.

*Retreat complete — 2026-03-21*
