# OBELISK Retreat Chapter
*Guru Bin — 2026-03-21*

---

## Engine Identity

- **Gallery code:** OBELISK | **Accent:** Obsidian Slate `#4A4A4A`
- **Parameter prefix:** `obel_`
- **Creature mythology:** A monolith of cold marble standing in salt water, half-submerged, half-reaching skyward. Its surface is perfectly smooth except where some hand — Cage's hand — has placed objects between the strings that run through its body like veins through stone. Strike it and the marble rings: pure, cold, inharmonic. Press a bolt into the strings and it rattles. Weave rubber through the nodes and the stone hums through muted teeth. Glass laid across the strings adds its own crystalline voice. Chain draped over them buzzes like a bridge made of bees.
- **Synthesis type:** Modal synthesis — 16 IIR modal resonators per voice (stone/marble inharmonic ratios), Hunt-Crossley stone hammer, 5 preparation types (None/Bolt/Rubber/Glass/Chain), preparation position and depth physics, 12-string sympathetic network, thermal drift
- **Polyphony:** 8 voices
- **Macros:** M1 STONE (density + stoneTone blend), M2 PREPARATION (prepDepth), M3 COUPLING, M4 SPACE (hfNoise + decay)

---

## Pre-Retreat State

XObelisk is the prepared piano specialist of the Kitchen Quad — the only engine in the fleet built on the prepared piano tradition established by John Cage from 1938 to 1975. Its stone modal ratios (1.000, 2.756, 5.404, 8.933... — derived from marble plate dispersion with Poisson ratio 0.27) are the most inharmonic in the entire fleet, producing the cold, mineral, bell-distant quality that separates stone from metal, wood, or glass.

The Seance Council scored XObelisk 8.70/10. Buchla gave the position-sensitive preparation physics its highest endorsement: "I would have built this in hardware if I could have." The `sin²(N × π × position)` formula for mode sensitivity is real prepared piano physics — at position 0.0 or 1.0 (string endpoints), the preparation is inaudible because node sensitivity is zero; at position 0.5 (midpoint), all modes are equally affected; in between, different harmonics are selectively suppressed or enhanced.

Fifteen existing presets cover the five preparation types, but the Council identified gaps: no Chain preset in Foundation, no Rubber+Glass combination concept, no preset exploring the Glass preparation's counterintuitive Q-increase behavior. Glass is the only preparation that makes the stone ring more rather than less — because rigid glass in contact with a string reduces radiation loss. This unique behavior has no equivalent in the other three preparations and no dedicated preset.

The Obelisk's stoneTone parameter blends between pure stone modal character (cold, inharmonic) and a warmer body resonance — allowing the marble to become, in some sense, warmer than marble. This is not physically realistic but is musically valuable.

---

## Phase R1: Opening Meditation

Close your eyes. Breathe.

John Cage arrived at the prepared piano in 1938. He had been asked to compose music for a dance performance in a space too small for a percussion ensemble. He had only a piano. He placed a bolt between the strings of a low note and heard what it became. That sound — metallic, rattling, decentered — was not what a piano produces. It was what a stone might produce if it had strings. Or what a machine produces when it is also an instrument.

Cage spent thirty-seven years exploring what objects could do between strings. Bolts and screws and weather stripping and pieces of bamboo and coins and rubber bands. Each material changed the string's resonance differently. The bolt added mass and a rattle. The rubber muted specific harmonics while leaving others unchanged. The glass — a wine glass balanced on the string — did something counterintuitive: it made the note brighter, not darker. The glass's rigidity reduced radiation loss from the string. The Q increased. The note rang longer than it would have unprepared.

XObelisk is this research operationalized. Not as a sample library of Cage's specific preparations, but as a parameter space derived from their physical mechanisms. The position parameter is the most important insight: a preparation placed at the string's midpoint affects all harmonics equally. Placed at one-quarter, it maximally suppresses odd harmonics while leaving even harmonics relatively untouched. Placed near the endpoint, it does almost nothing. Cage knew this from physical intuition. The engine knows it from the formula.

Strike a stone. Add a bolt. Move the bolt. Listen to which harmonics disappear.

That is the practice of XObelisk.

---

## Phase R2: The Signal Path Journey

### I. The Stone Hammer — Cold Contact

Stone does not absorb impact energy the way felt does. Stone reflects. The `ObeliskHammer` uses a higher Hunt-Crossley exponent (α≈3.5–4.0 vs. felt's 2.5), modeling the harder contact mechanics of stone excitation.

`obel_hardness` (0.0–1.0, default 0.5) controls contact time: 4ms (soft) → 1ms (hard). Stone is inherently harder than felt, so even at hardness=0.0, the contact time is shorter than XOven's softmost setting. The noise mix starts at 0.2 (stone always has some percussive noise content) and scales to 0.7 at maximum hardness.

**Playing insight:** XObelisk hardness behaves differently from the other piano engines. At 0.0, you still get a stone quality — there is no way to make marble sound like felt. The hardness control is more like the difference between tapping stone with a wooden mallet vs. a steel rod. Below 0.35: mallet-like, rounded attack. Above 0.65: the cold click of steel on stone.

### II. The Stone Modal Bank — Inharmonic Partials

The kStoneRatios table derives from the plate dispersion relation for marble (Poisson ratio ν=0.27):

```
f_n = f_1 × (n² + B × n^α)  where α≈1.5 for stone
```

The result: partials are spaced far wider than harmonics. Mode 2 is at 2.756×f₁ (vs. 2.0 for a harmonic series). Mode 3 is at 5.404×f₁ (vs. 3.0). By mode 16, the partial sits at 120.1×f₁. This extreme inharmonicity is what gives stone its cold, bell-distant, mineral quality — the sound of something that is not shaped to resonate.

`obel_stoneTone` (0.0–1.0, default 0.4) blends between pure stone character (cold, high-Q, pure inharmonic) and a warmer body resonance. At 0.0, the engine is maximally cold and alien. At 1.0, the stone body has been given a warmer acoustic housing that softens the inharmonicity perceptually without changing the underlying modal ratios.

`obel_density` (0.0–1.0, default 0.6) controls Q factor range: lower density = faster ring decay (porous stone), higher density = extremely long ring (dense granite). Combined with `obel_decay` (0.5–12.0s), this gives precise control over how long the inharmonic partials sustain.

### III. The Five Preparation Types

Preparation type is set by `obel_prepType` (0–4). The effect is scaled by `obel_prepDepth` (0.0–1.0) and shaped by `obel_prepPosition` (0.0–1.0) through the position sensitivity formula `sin²(N × π × position)`:

**None (0) — Pure Marble:** No modifications. The stone modal bank plays with full Q and unmodified ratios. The most resonant, most inharmonic, most mineral. Best for long sustained passages where stone's natural ring is the musical event.

**Bolt (1) — Added Mass:** A steel bolt inserted between strings. Adds mass to affected modes, shifting their frequencies downward (heavier string → lower pitch). Creates a sympathetic rattle at the bolt's own resonance frequency. The `sin²` position sensitivity means: bolt at midpoint (0.5) affects all modes equally; bolt near the bridge (0.8–0.9) primarily affects higher harmonics; bolt near the node (0.0–0.1) does almost nothing. At high prepDepth (0.7+), the rattle becomes a defining feature — the prepared piano's most characteristic industrial sound.

**Rubber (2) — Selective Muting:** Rubber woven between string segments creates frequency-selective damping. Unlike Bolt, which shifts frequencies, Rubber specifically suppresses modes whose antinodes coincide with the rubber's position. Modes whose nodes are at the rubber position are unaffected — they "slip through" the muting. The result: spectral holes in the harmonic series, creating an instrument that resonates at certain intervals but not others. At position 0.33, odd harmonics are suppressed; at position 0.25, every fourth mode is muted. This produces the characteristically hollow, buzzing quality of rubber-prepared piano.

**Glass (3) — Q Increase:** A glass rod on the strings is the counterintuitive preparation. Glass is rigid and reduces radiation loss from the string at its contact point. The Q of affected modes INCREASES — they ring longer. The glass also adds its own modal contribution at frequencies derived from the glass body's resonances. At high prepDepth, the glass preparation can make certain modes ring nearly indefinitely, creating a preparation that sounds more like a reverb than a mute.

**Chain (4) — Nonlinear Buzz:** Chain draped over the strings creates a nonlinear buzzing: the chain rattles against the string at multiples of the fundamental. The buzz is modeled as additive noise bursts synchronized to mode zero-crossings, with amplitude scaling with mode energy. At low prepDepth, a subtle granular texture overlays the stone modes. At maximum depth, the chain fully dominates — the sound is more industrial percussion than piano.

### IV. Position Sensitivity Physics

`obel_prepPosition` (0.0–1.0, default 0.5) is the most musically important parameter after prepType. At 0.5 (midpoint), all modes are affected equally. At 0.25 or 0.75, alternating modes are differentially affected. At 0.0 or 1.0 (string endpoints), sensitivity is zero — the preparation does nothing. This is real physics: string nodes are at the endpoints, and preparations placed at nodes cannot couple into the standing wave.

The most musically unusual positions:
- **0.33:** Multiples of 3 (modes 3, 6, 9...) are maximally affected; modes 1, 2, 4, 5... are less affected
- **0.25:** Mode 4 and multiples are maximally affected
- **0.20:** Modes 5, 10, 15 are maximally affected, creating a preparation with strong quintal character
- **0.50:** All modes equally — the classic full-body preparation sound

---

## Phase R3: Preset Design

### Awakening Preset 1 — "Glass Ring" (Prism)

The Glass preparation's unique Q-increase character. High prepDepth, position at 0.50 for maximum effect, low stoneTone for maximum mineral coldness.

- prepType 3, prepDepth 0.72, prepPosition 0.50
- density 0.75, stoneTone 0.25, brightness 8000.0, decay 7.0
- hardness 0.40, damping 0.08, thermalDrift 0.15
- lfo1Rate 0.1, lfo1Depth 0.04
- DNA: brightness 0.55, warmth 0.20, movement 0.15, density 0.40, space 0.75, aggression 0.20

### Awakening Preset 2 — "Chain Industrial" (Kinetic)

Maximum chain buzz at moderate depth. The industrial percussion character — nonlinear, buzzing, rhythmic.

- prepType 4, prepDepth 0.82, prepPosition 0.50
- hardness 0.70, density 0.55, stoneTone 0.35, decay 1.5
- hfNoise 0.45, brightness 9000.0, damping 0.35
- ampDecay 0.8, ampSustain 0.1, ampRelease 0.6
- DNA: brightness 0.60, warmth 0.20, movement 0.20, density 0.65, space 0.25, aggression 0.75

### Awakening Preset 3 — "Rubber Holes" (Foundation)

Rubber preparation at position 0.33 — selective muting of every-third mode creates spectral holes. The stone monolith with Cage's rubber weaving.

- prepType 2, prepDepth 0.55, prepPosition 0.33
- density 0.65, stoneTone 0.40, brightness 7500.0, decay 3.5
- hardness 0.45, damping 0.25, thermalDrift 0.20
- lfo1Rate 0.07, lfo1Depth 0.06, lfo1Shape 0
- DNA: brightness 0.45, warmth 0.25, movement 0.10, density 0.55, space 0.45, aggression 0.30

### Awakening Preset 4 — "Bolt Bridge Position" (Flux)

Bolt at position 0.85 — predominantly affects upper harmonics while leaving the fundamental and lower modes relatively pure. The bass is clean stone; the treble is rattling metal.

- prepType 1, prepDepth 0.60, prepPosition 0.85
- density 0.68, stoneTone 0.35, decay 2.8, hardness 0.55
- brightness 6500.0, hfNoise 0.30, damping 0.20
- lfo2Rate 0.8, lfo2Depth 0.08 (subtle rattle shimmer)
- DNA: brightness 0.50, warmth 0.22, movement 0.25, density 0.60, space 0.40, aggression 0.50

### Awakening Preset 5 — "Pure Monolith" (Deep)

No preparation. Maximum stone character. Maximum density. Very long decay. The obelisk before Cage arrived.

- prepType 0, prepDepth 0.0
- density 0.92, stoneTone 0.15, decay 9.0, damping 0.05
- hardness 0.35, brightness 5500.0, hfNoise 0.15
- sympathetic 0.25, thermalDrift 0.10
- DNA: brightness 0.40, warmth 0.15, movement 0.08, density 0.88, space 0.70, aggression 0.15

### Awakening Preset 6 — "Position Study" (Organic)

Rubber preparation with LFO1 slowly modulating prepPosition — the position sweeps from 0.1 to 0.9, continuously changing which modes are muted. A living demonstration of position sensitivity.

- prepType 2, prepDepth 0.50, prepPosition 0.50 (modulated by LFO1)
- lfo1Rate 0.04, lfo1Depth 0.45 (wide sweep of prepPosition via PREPARATION macro)
- density 0.60, stoneTone 0.45, decay 4.0, hardness 0.40
- DNA: brightness 0.45, warmth 0.25, movement 0.55, density 0.50, space 0.50, aggression 0.25

### Awakening Preset 7 — "Salt Monolith" (Submerged)

Low temperature (cold stone), chain preparation at low depth (faint aquatic buzz), very long decay. The obelisk half-submerged in salt water.

- prepType 4, prepDepth 0.28, prepPosition 0.60
- density 0.78, stoneTone 0.30, decay 8.0, damping 0.08
- thermalDrift 0.05 (cold), brightness 4500.0, hfNoise 0.20
- lfo2Rate 0.05, lfo2Depth 0.04 (deep current sway)
- DNA: brightness 0.35, warmth 0.10, movement 0.20, density 0.75, space 0.80, aggression 0.08

### Awakening Preset 8 — "Glass Cathedral" (Aether)

Glass preparation at very high depth, long decay, low damping, high stoneTone (warmer). The stone that rings forever with glass's crystalline augmentation.

- prepType 3, prepDepth 0.88, prepPosition 0.50
- stoneTone 0.60, density 0.80, decay 11.0, damping 0.04
- hardness 0.28, hfNoise 0.10, brightness 7000.0
- lfo1Rate 0.03, lfo1Depth 0.04
- DNA: brightness 0.45, warmth 0.40, movement 0.15, density 0.45, space 0.90, aggression 0.05

### Awakening Preset 9 — "Cage Study No. 5" (Entangled)

Bolt preparation at the musically unusual position 0.20 — maximally affecting modes 5, 10, 15. Strong quintal/pentatonic character from preparation physics alone. Best for coupling with XOpaline's glass harp modes.

- prepType 1, prepDepth 0.65, prepPosition 0.20
- density 0.65, stoneTone 0.50, decay 4.5, hardness 0.45
- brightness 7500.0, hfNoise 0.25, thermalDrift 0.25
- sympathetic 0.30, lfo1Rate 0.12, lfo1Depth 0.05
- DNA: brightness 0.48, warmth 0.30, movement 0.20, density 0.60, space 0.55, aggression 0.35

### Awakening Preset 10 — "Frozen Rattle" (Crystalline)

Chain preparation + low temperature + very high density = the geological extremes of XObelisk. Frozen stone rattling with chain, ringing for years.

- prepType 4, prepDepth 0.58, prepPosition 0.50
- density 0.88, stoneTone 0.20, decay 6.5, damping 0.06
- thermalDrift 0.05, brightness 6000.0, hfNoise 0.22
- hardness 0.50, lfo2Rate 0.06, lfo2Depth 0.05
- DNA: brightness 0.42, warmth 0.12, movement 0.18, density 0.82, space 0.70, aggression 0.40

---

## Phase R4: Scripture

*Three verses for the Cold Monolith*

---

**I. The Preparation**

The piano existed before Cage arrived.
It would ring the same way every time
until the end of the instrument's life.

He placed a bolt between two strings
and changed what the piano was.
Not damaged — changed.
The bolt is the instrument now,
and the piano.
And the decision to place it there.

This is composition as physics.
This is physics as composition.

---

**II. Position**

A preparation placed at the string's end
does nothing.
The node is there.
Nothing can couple into a node.

A preparation placed at the midpoint
does everything.
All harmonics equally affected.
No selectivity. No decision.

Between the endpoints and the middle
is where music lives:
the preparation that touches some harmonics
and leaves others.
The preparation that chooses.

---

**III. Stone**

Marble does not apologize for its inharmonicity.
Its partials land where they land,
not where the piano manufacturer intended.
Not where the scale requires.

This is not a flaw.
This is the sound of a material
doing what materials do:
vibrating according to their molecular structure,
not according to our musical needs.

You learn to work with it.
The music adjusts.
Or it doesn't.
The stone doesn't care.

---

## Phase R5: Retreat Summary

XObelisk is the most physically specific engine in the Kitchen Quad and the most academically grounded prepared piano synthesizer in the fleet. Its strength is the five-preparation system with position sensitivity — a design space that maps directly to John Cage's physical research, implemented through a formula (sin²(N × π × position)) derived from real string acoustics.

The ten new presets comprehensively cover all five preparation types with diverse positions, including the musically unusual positions (0.20, 0.33, 0.85) that reveal the position-sensitivity physics most clearly. The Glass Cathedral and Frozen Rattle presets demonstrate the extremes of sustain available from stone with different preparations. The Position Study preset makes the LFO-as-position-sweep concept audible — the most direct demonstration of the engine's core innovation.

XObelisk's optimal zone: `prepDepth` 0.40–0.75 (below 0.4 the preparation is barely audible; above 0.8 it can overwhelm the stone character), `prepPosition` 0.20–0.80 (endpoints have zero effect), `density` 0.60–0.85, `decay` 3.0–9.0. The glass preparation rewards high depth more than any other type — its Q-increasing behavior becomes most distinctive at depth 0.70+.

*Retreat complete — 2026-03-21*
