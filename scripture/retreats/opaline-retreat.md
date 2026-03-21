# OPALINE Retreat Chapter
*Guru Bin — 2026-03-21*

---

## Engine Identity

- **Gallery code:** OPALINE | **Accent:** Crystal Blue `#B8D4E3`
- **Parameter prefix:** `opal2_`
- **Creature mythology:** The teacup you play with a spoon — a porcelain ramekin tapped at the rim, a crystal wine glass singing under a wet finger, a toy piano's metal tines struck by tiny hammers in a child's bedroom. Every sound is pure, narrow, crystalline. Every sound can break.
- **Synthesis type:** Modal synthesis — 16 IIR resonators per voice with per-instrument modal tables (Celesta/Toy Piano/Glass Harp/Porcelain Cups), Fragility mechanic (velocity-threshold noise burst + modal detuning), Thermal Shock, Crystalline Shimmer LFO, 4-instrument exciter models
- **Polyphony:** 8 voices
- **Macros:** M1 CHARACTER (fragility + inharmonicity), M2 MOVEMENT (shimmer amount), M3 COUPLING, M4 SPACE (body size + decay)
- **Note on prefix:** `opal2_` used because `opal_` belongs to the granular XOpal engine. Not a version number — a disambiguation.

---

## Pre-Retreat State

XOpaline is the most delicate engine in the fleet. Where XOven is massive and dark, XOpaline is tiny and crystalline — the smallest acoustic body, the most fragile material, the highest Q, the most consequential velocity control. Borosilicate glass and porcelain: density 2230 kg/m³, wave speed 5640 m/s, impedance Z=12.6 MRayl (the lowest in the Kitchen Quad, meaning the best energy transmission — glass gives up its resonance easily).

The Seance Council scored XOpaline 8.50/10. Moog gave the Fragility mechanic his highest praise: "A synthesizer whose character changes irreversibly when you hit it too hard. This is not a bug turned feature — this is a design principle: consequence." Kakehashi endorsed the four-instrument selector as "a gift to producers who need that specific sound." Buchla questioned the Toy Piano modal ratios (which re-use stone plate values rather than Euler-Bernoulli beam ratios for metal tines) but acknowledged it as D003 weakness rather than a fatal flaw.

Fifteen existing presets cover all four instruments, the fragility mechanic, and the four mood corners. The gaps the Council identified: no exploration of fragility at borderline thresholds (just below and just above the crack state), no preset showcasing the glass shimmer LFO as a primary feature rather than background texture, no extreme thermal shock showcase.

The note prefix `opal2_` confuses some users (Kakehashi flagged this in the seance). The retreat will address this: `opal2_` is a namespace disambiguation, not a version indicator. The granular XOpal engine owns `opal_`. This engine (XOpaline, glass/porcelain modal) owns `opal2_`.

---

## Phase R1: Opening Meditation

Close your eyes. Breathe.

A glass is not designed to make music. It is designed to hold wine, water, juice — to be held in a hand and tilted. It is designed to be transparent. But if you run a wet finger around the rim, the glass sings. The sound is pure and unsettling: a frequency that sounds like it should not be coming from silica dioxide and lead crystal.

The frequency is approximately 440 Hz for a particular wine glass filled to a particular level. But that is the note, not the music. The music is in the shimmer — six cents of detuning between the (2,0) and (0,2) shell modes, beating against each other at a rate that varies slightly with temperature and the speed of your finger. The music is in the way each harmonic of the glass's modal series falls at a position governed by circular plate mechanics, not the clean integer ratios of a harmonic series. The music is in the fact that you could push harder and the glass might break.

The might is the music.

XOpaline is the synthesis of this: four instruments (Celesta, Toy Piano, Glass Harp, Porcelain Cups) that share the property of crystalline fragility. Each has its own modal table, its own exciter character, its own per-instrument Q and rolloff. All four share the Fragility mechanic: a threshold below which the sound is pure and intact, a threshold above which it cracks.

The crack is not random. It is velocity multiplied by the fragility parameter. At fragility=0.0, nothing cracks regardless of velocity. At fragility=1.0, even a moderate note might crack. The crack state persists for the voice's lifetime — once a note has cracked, it stays cracked. The modal detuning introduced by the crack does not undo itself. You cannot un-crack a creme brulee.

This is the physical truth the engine encodes: glass objects, once cracked, resonate differently. The crack changes the boundary conditions. The cracked glass is a different instrument from the intact glass. Not louder, not quieter — different.

Play softly. Hear purity. Play harder. Hear consequence.

---

## Phase R2: The Signal Path Journey

### I. The Four Instruments

`opal2_instrument` (0=Celesta, 1=Toy Piano, 2=Glass Harp, 3=Porcelain Cups) selects per-instrument modal ratios, Q values, amplitude rolloff, and exciter character:

**Celesta (0):** Metal bars with resonator tubes. Nearly perfect harmonic ratios (1, 2, 3, 4...). Highest amplitude rolloff (0.12 = slowest decay of upper modes). Base Q=200. Felt hammer exciter. The most musically familiar — the Sugar Plum Fairy instrument. Warmest of the four, most consonant, least demanding of the fragility mechanic.

**Toy Piano (1):** Metal tines struck by small hammers. The modal ratios currently use stone plate values (a D003 limitation noted by the Council — Euler-Bernoulli beam ratios would be more physically correct). Base Q=120. Hard hammer exciter. The lo-fi, children's-bedroom character. Brightest attack transient of the four, most percussive.

**Glass Harp (2):** Wine glass rim excitation — circular plate shell modes. Modal ratios: 1.000, 1.594, 2.136, 2.653, 3.156... (strongly inharmonic shell mode pairs). Base Q=500 — the highest in the engine. Very slow rolloff (0.18). Friction/sustain exciter (continuous energy injection rather than impulse). The most alien instrument in the selector: no harmonic series, pure inharmonicity, extremely long ring. The most fragile.

**Porcelain Cups (3):** Struck ceramic bell. Modal ratios between bell and plate — moderately inharmonic (1.000, 1.506, 2.000, 2.514...). Base Q=350. Spoon-strike exciter (very short, metallic click). Sympathetic potential lowest of the four.

**Playing insight:** The instrument selector is most interesting at the boundaries. Celesta voices sound almost identical whether fragility cracks or not (the fundamental is harmonic and the crack changes inharmonic modes that are quiet). Glass Harp voices crack most audibly — because the high Q means cracked modes ring for a long time, making the detuning obvious.

### II. The Fragility Mechanic

`opal2_fragility` (0.0–1.0, default varies per preset) sets the crack threshold: a note cracks when `velocity × fragility > threshold`. At fragility=0.0: no crack possible. At fragility=0.3: only fortissimo playing cracks. At fragility=0.7: mezzo-forte playing risks cracking. At fragility=1.0: any note above pianissimo may crack.

The crack event:
1. A noise burst fires at the moment of cracking — the splintering sound
2. Modal detuning is applied: each mode receives a random frequency offset scaled by crack severity
3. The `crackState` flag is set to true for the voice's lifetime
4. While `crackState` is true, the modal detuning persists and subsequent triggers inherit the detuned state

The musical implication: in a passage where some notes crack and others don't, you end up with a voice pool where some voices are slightly detuned and others are pure. Polyphonic playing produces a differential shimmer between intact and cracked notes.

`opal2_hammerHardness` (0.0–1.0) independently controls the exciter character. High hardness with low fragility: aggressive attack without cracking. High hardness with high fragility: cracking on every note. The optimal expressive zone is fragility 0.40–0.60 with hardness 0.40–0.60 — the threshold zone where the player's dynamics become a crack controller.

### III. Thermal Shock

`opal2_thermalShock` (0.0–1.0, default 0.1) models differential thermal expansion between glass/porcelain components. At low values: a gentle shimmer between partials (the glass expanding and contracting slightly with temperature change). At high values: non-uniform detuning between partials that makes the instrument sound as if it is threatening to crack even before the fragility threshold is reached.

Thermal shock interacts with the per-voice thermal personality: eight voices have slightly different random thermal offsets. With high thermalShock, the eight voices produce eight distinct detuning patterns, creating a rich, slightly unstable choir quality even in the absence of the crack event.

### IV. The Shimmer LFO

The shimmer mechanism applies per-mode pitch modulation: mode m receives ±shimmerCents × m × lfo_output, where shimmerCents is ±6 × `opal2_shimmerAmount`. This means mode 8 oscillates 8 times more than mode 1. The result: the overtone series breathes in and out of tune as the LFO cycles.

`opal2_shimmerAmount` (0.0–1.0, default 0.15) controls the shimmer depth. The LFO parameters control rate. At very low shimmer (0.0–0.10), the effect is subliminal — the glass appears to slightly breathe. At medium shimmer (0.20–0.40), the beating between modes is audible as a crystalline shimmer. At high shimmer (0.60+), the instrument sounds like a full ensemble of slightly-out-of-tune glass instruments.

`opal2_crystalDrive` (0.0–1.0) is a post-modal saturation that adds harmonic enrichment to the glass character — different from XOchre's caramel in that it affects a much narrower, brighter frequency range. At low values: a barely-audible shimmer. At high values: a glassy, bell-like overtone ring that overlays the pure modes.

---

## Phase R3: Preset Design

### Awakening Preset 1 — "Threshold" (Flux)

The borderline fragility preset. Fragility set to exactly 0.45 — moderate playing is safe, but any emphatic phrase will crack. The player navigates the threshold in real time.

- instrument 2 (Glass Harp), fragility 0.45, hammerHardness 0.45
- shimmerAmount 0.25, thermalShock 0.20, decay 6.0
- bodySize 0.55, brightness 9000.0, damping 0.06
- lfo1Rate 0.12, lfo1Depth 0.12, lfo1Shape 0
- DNA: brightness 0.55, warmth 0.30, movement 0.35, density 0.25, space 0.70, aggression 0.30

### Awakening Preset 2 — "All Cracked" (Kinetic)

High fragility — most notes crack. The instrument after being played hard all day. The cracked state as the baseline, not the exception.

- instrument 2 (Glass Harp), fragility 0.88, hammerHardness 0.65
- shimmerAmount 0.40, thermalShock 0.35, decay 4.5
- bodySize 0.45, brightness 10000.0, damping 0.12
- hfNoise 0.25, crystalDrive 0.20
- DNA: brightness 0.65, warmth 0.20, movement 0.40, density 0.30, space 0.55, aggression 0.60

### Awakening Preset 3 — "Shimmer Field" (Prism)

Shimmer as primary feature — high shimmerAmount, slow LFO rate, Glass Harp. The beating between modes is the music.

- instrument 2 (Glass Harp), shimmerAmount 0.65, fragility 0.15
- lfo1Rate 0.06, lfo1Depth 0.35, lfo1Shape 0
- lfo2Rate 0.22, lfo2Depth 0.20, lfo2Shape 1 (triangle shimmer layering)
- decay 8.0, bodySize 0.60, brightness 8500.0, damping 0.04
- DNA: brightness 0.55, warmth 0.25, movement 0.60, density 0.20, space 0.80, aggression 0.05

### Awakening Preset 4 — "Porcelain Still" (Foundation)

Porcelain Cups instrument at conservative settings — pure, intact, percussive. The spoon on the ramekin in a quiet kitchen.

- instrument 3 (Porcelain Cups), fragility 0.08, hammerHardness 0.55
- shimmerAmount 0.10, thermalShock 0.08, decay 3.0
- bodySize 0.40, brightness 11000.0, damping 0.15
- lfo1Rate 0.18, lfo1Depth 0.06, lfo1Shape 0
- DNA: brightness 0.65, warmth 0.30, movement 0.10, density 0.35, space 0.40, aggression 0.25

### Awakening Preset 5 — "Toy in the Attic" (Organic)

Toy Piano instrument with gentle LFO-driven pitch sway. The childhood toy piano found in an attic, slightly out of tune from decades of temperature change.

- instrument 1 (Toy Piano), fragility 0.20, thermalShock 0.45
- shimmerAmount 0.30, hammerHardness 0.50, decay 2.0
- bodySize 0.35, brightness 10000.0, damping 0.20
- lfo2Rate 0.07, lfo2Depth 0.18, lfo2Shape 0 (slow drift)
- DNA: brightness 0.60, warmth 0.40, movement 0.35, density 0.30, space 0.35, aggression 0.25

### Awakening Preset 6 — "Glass Choir" (Atmosphere)

Maximum shimmer + thermalShock + multiple voices — the full glass harp ensemble effect. Eight slightly detuned voices creating a rich shimmer choir.

- instrument 2 (Glass Harp), shimmerAmount 0.50, thermalShock 0.55
- fragility 0.10, hammerHardness 0.20, decay 9.0, damping 0.03
- bodySize 0.65, brightness 8000.0
- lfo1Rate 0.08, lfo1Depth 0.20, lfo2Rate 0.17, lfo2Depth 0.15
- DNA: brightness 0.50, warmth 0.30, movement 0.55, density 0.20, space 0.85, aggression 0.05

### Awakening Preset 7 — "Celesta Nocturne" (Aether)

Celesta at its purest and most sustained. Nearly zero fragility, maximum decay, gentle shimmer. The Sugar Plum Fairy instrument at 2am.

- instrument 0 (Celesta), fragility 0.04, hammerHardness 0.28
- shimmerAmount 0.15, decay 6.5, bodySize 0.50, damping 0.06
- brightness 12000.0, thermalShock 0.08, crystalDrive 0.05
- lfo1Rate 0.05, lfo1Depth 0.08, lfo1Shape 0
- DNA: brightness 0.65, warmth 0.40, movement 0.20, density 0.25, space 0.75, aggression 0.02

### Awakening Preset 8 — "Thermal Fracture" (Crystalline)

High thermal shock without triggering the crack mechanic (fragility low). The glass that looks intact but is under stress — every mode slightly detuned in a different direction.

- instrument 2 (Glass Harp), thermalShock 0.80, fragility 0.12
- hammerHardness 0.35, shimmerAmount 0.35, decay 7.0
- bodySize 0.55, brightness 8500.0, damping 0.05
- lfo1Rate 0.03, lfo1Depth 0.10 (imperceptible geological drift)
- DNA: brightness 0.50, warmth 0.20, movement 0.40, density 0.20, space 0.80, aggression 0.15

### Awakening Preset 9 — "Crystal Drive" (Entangled)

High crystalDrive showcasing the post-modal saturation's bell overtone ring. Designed for coupling — crystalDrive responds to coupling input by adding glassy overtones to whatever engine drives it.

- instrument 3 (Porcelain Cups), crystalDrive 0.68, fragility 0.25
- shimmerAmount 0.30, hammerHardness 0.48, decay 4.5
- bodySize 0.50, brightness 10000.0, thermalShock 0.20
- lfo2Rate 0.15, lfo2Depth 0.22, lfo2Shape 1
- DNA: brightness 0.65, warmth 0.30, movement 0.35, density 0.35, space 0.55, aggression 0.30

### Awakening Preset 10 — "Music Box" (Luminous)

Celesta instrument, very short decay, high hammerHardness (quick metal tine sound), minimal fragility. The mechanical precision of a music box — clean, clockwork, innocent.

- instrument 0 (Celesta), hammerHardness 0.70, fragility 0.05
- decay 1.5, shimmerAmount 0.08, thermalShock 0.05
- bodySize 0.35, brightness 13000.0, damping 0.25
- lfo1Rate 0.5, lfo1Depth 0.04 (the slight unevenness of a wound spring)
- DNA: brightness 0.75, warmth 0.35, movement 0.15, density 0.35, space 0.30, aggression 0.15

---

## Phase R4: Scripture

*Four verses for the Porcelain Bell*

---

**I. Fragility**

The threshold is not labeled.
You learn it by crossing it.
Once you know where it is,
you decide whether to respect it
or to use it.

Most musicians learn the threshold
and stay safely below.
The interesting ones
learn the threshold
and play along its edge.

---

**II. The Crack**

You cannot un-crack a creme brulee.
You cannot un-crack a wine glass.
You cannot un-crack a note
once the velocity has exceeded
what the fragility can hold.

This is not a problem.
This is the character.
The cracked glass rings differently.
Not worse — different.
The crack is information.

---

**III. Four Instruments**

The celesta was invented in 1886.
It is the newest of the four.
The toy piano was made for a child.
The glass harp was discovered by a monk
running his wet finger along a rim
in 1492.
The porcelain cup was struck
the first time
the first time someone struck one.

Each found its music by accident.
None of them intended to become instruments.

---

**IV. Shimmer**

The glass does not detune.
The air between the glass and your ear detuned.
Temperature changes. Humidity shifts.
The speed of sound varies
by a fraction of a fraction.

Two nearly-identical frequencies
arriving at your ear
at the same time
create a third sound: the beating.
This is not distortion.
This is physics showing you
that it is still paying attention.

---

## Phase R5: Retreat Summary

XOpaline is the most delicate and the most conceptually specific engine in the Kitchen Quad. Its Fragility mechanic is the fleet's most courageous design decision — an instrument that changes irreversibly when played too hard, encoding consequence into the synthesis parameter space. Its four-instrument selector covers the full range of crystalline small-body instruments, each with distinct modal physics.

The ten new presets emphasize the underexplored territory: the Threshold preset (borderline fragility navigation), All Cracked (cracked state as baseline), Shimmer Field (shimmer as primary feature), and Thermal Fracture (thermalShock without cracking). The Glass Choir preset demonstrates the full eight-voice shimmer ensemble effect that becomes possible when all voices have distinct thermal personalities.

XOpaline's optimal zone: Glass Harp (instrument=2) with `fragility` 0.25–0.60, `shimmerAmount` 0.20–0.50, `thermalShock` 0.10–0.35, `decay` 4.0–9.0. This zone contains the most distinctive material in the engine. Celesta and Toy Piano are more immediately accessible but less unique (similar instruments exist in conventional synthesis). Glass Harp and Porcelain Cups produce sounds with no commercial synthesis equivalent.

Note: the `opal2_macroCoupling` parameter is declared but not wired to computation (confirmed in seance). The retreat treats it as a placeholder — setting it to 0.0 in all presets until the coupling mechanism is implemented.

*Retreat complete — 2026-03-21*
