# XOceanus V1 — Release Notes

**XO_OX Designs | March 2026**

---

Welcome to XOceanus V1 — a synthesizer built around a single idea: what happens when 71 distinct sonic personalities share the same room, listen to each other, and argue?

This is our biggest release. Here is everything that is in it.

---

## 71 Engines

XOceanus ships with 71 registered synthesis engines. Every one of them has a name, a character, a color, and a sound that cannot come from anywhere else. They range from familiar territory to things we are still not entirely sure how to describe.

If you started counting engines at 22 and wondered where the rest came from — this is where.

---

## New Engines

### OBRIX — Reef Jade
*The Flagship. The Constructive Collision.*

OBRIX is what happens when synthesis becomes modular by nature rather than by configuration. You stack bricks — sound sources, shapers, modulators — and they influence each other. Route two oscillators into a shared filter, detune them against each other, push FM feedback into the seam between them, and something emerges that neither brick was capable of alone.

Built across three iterative passes: the core collision architecture, then FM and real wavetable import, then the Drift Bus, Journey macro, and Spatial field. 65 parameters. Seance score 8.9/10.

### OSTINATO — Firelight Orange
*The Communal Drum Circle.*

Twelve percussion instruments under one roof, physically modeled. OSTINATO thinks about rhythm the way a fire circle does — every instrument shapes the space around it. Not just a drum machine. Seance score 8.7/10.

### OPENSKY — Sunburst
*Euphoric Shimmer.*

Supersaw stacks, shimmer reverb, wide chorus, deep unison. OPENSKY is the engine for sounds that want to take up the entire room. Seance score 8.1/10.

### OCEANDEEP — Trench Violet
*The Pressure Engine.*

Synthesis from the bottom of the water column — where pressure is everything and light does not reach. Dense, slow-moving, fundamental. Seance score 7.8/10.

### OUIE — Hammerhead Steel
*The Duophonic Thinker.*

Two voices, eight algorithms apiece, and a STRIFE/LOVE interaction axis that governs how much the voices agree with each other. When they disagree, the seam between them becomes the sound. Seance score 8.5/10.

### ORBWEAVE — Kelp Knot Purple
*The Topological Knot.*

ORBWEAVE treats audio signal as something that can be threaded, knotted, and woven through itself. It is the first engine in XOceanus built specifically around the KnotTopology coupling type — connections that flow both directions simultaneously. Seance score 8.4/10.

### OVERTONE — Spectral Ice
*The Nautilus.*

Continued fraction mathematics applied to spectral synthesis. OVERTONE derives its harmonic relationships from irrational numbers the way a nautilus shell is derived from phi — not by design, but by the logic of growth.

### ORGANISM — Emergence Lime
*The Coral Colony.*

Cellular automata as a synthesis engine. ORGANISM runs rule sets — simple local rules that produce unexpected global behavior. The sound changes not because you moved a parameter but because the system evolved.

### OVERLAP — Bioluminescent Cyan-Green
*Interference Patterns.*

OVERLAP works with what happens when two signals occupy the same space. KnotMatrix routing, feedback delay networks, stereo bioluminescence — the engine is built for the artifact rather than the original signal. Seance score 8.4/10.

### OUTWIT — Chromatophore Amber
*Adaptive Timbre.*

OUTWIT changes color — literally and sonically. Like a cephalopod matching its surroundings, OUTWIT responds to what the signal is doing and shifts its character to complement or contrast it. Seance score 8.4/10.

### OMBRE, ORCA, OCTOPUS

Three more engines, each with their own logic. OMBRE holds two narratives in tension — memory and forgetting. ORCA hunts through wavetables with echolocation-style scanning. OCTOPUS distributes processing across decentralized arms, each capable of independent chromatophore modulation.

---

## Three New FX Chains

All three run as master effects at the output stage. They are not utility processors. Each one has a character.

### The Aquarium — Aquatic FX Suite
Six water-themed effects. The Aquarium is what the output sounds like after it has passed through the water column. Reef saturation. Fathom pressure filtering. Drift shimmer. Tide movement.

### Mathematical FX
Four processors built on mathematical principles that normally stay in research papers. Entropy measures disorder and uses it. Voronoi partitions audio space geometrically. Quantum introduces superposition-style uncertainty. Attractor pulls audio toward a strange attractor trajectory.

### Boutique FX
Four environment generators for sounds that need to exist in a specific kind of space. Anomaly distorts the rules of the room. Archive adds the texture of age. Cathedral makes space out of reflection. Submersion puts the signal underwater and holds it there.

---

## The Preset Library

### ~21,918 Factory Presets

XOceanus V1 ships with more than 21,000 presets in `.xometa` format across eight moods:

**Foundation** — the architecture. Dry, fundamental, structural.
**Atmosphere** — space and movement. The environment around the note.
**Entangled** — coupling presets. Two or more engines in conversation.
**Prism** — spectral and timbral exploration. Color over motion.
**Flux** — movement and modulation-forward.
**Aether** — the most ambient, most diffuse category.
**Family** — the Constellation group character presets.
**Submerged** — deepwater sounds from the bottom of the column.

Every preset carries a 6D Sonic DNA fingerprint: brightness, warmth, movement, density, space, aggression. These are not tags — they are measurements. The DNA browser lets you find a sound by what it feels like rather than what it is named.

### Transcendental Presets — The First Tier

Beginning with V1, XO_OX is introducing the **Guru Bin Pilgrimage** — a curated tier of presets that go deeper than the factory library.

**Awakening presets** (included in XOceanus, free): 10 per engine. Gold visual treatment. These are the hero sounds — the ones that show each engine at its most essential.

**Transcendental presets** (premium, released in volumes): 15–20 per engine. Deeper exploration of each engine's character. Released alongside lore booklets documenting how and why the sounds were designed.

Volume 1 covers OBRIX, OSTINATO, OPENSKY, OCEANDEEP, and OUIE. The OBRIX Transcendental library includes "Coral Architect" — a preset designed specifically to demonstrate what the Drift Bus does when it disagrees with itself.

---

## Coupling — 13 Types

Cross-engine modulation in XOceanus works through the MegaCouplingMatrix. V1 ships with 13 coupling types, including the new **KnotTopology** — the first bidirectional coupling type, introduced alongside ORBWEAVE.

The other 12 types cover amplitude-to-filter, amplitude-to-pitch, LFO-to-pitch, envelope-to-morph, audio-to-FM, audio-to-ring, filter-to-filter, amplitude-to-choke, rhythm-to-blend, envelope-to-decay, pitch-to-pitch, and audio-to-wavetable.

Coupling presets live in the Entangled mood. 18 factory coupling presets ship at launch, covering six engine pairs at three interaction intensities (subtle, present, dominant).

---

## The SDK — For Developers

XOceanus V1 ships with a public SDK at `SDK/include/xoceanus/`.

The SDK is JUCE-free. If you want to build an engine that runs inside XOceanus, you do not need to link against JUCE — only the XOceanus SDK headers. The SDK includes the `SynthEngine` interface, all 13 `CouplingType` definitions, the `EngineModule` base contract, and a minimal engine template.

Third-party engines follow the same rules as first-party engines: parameter IDs are namespaced, presets use `.xometa` format, and DSP lives in inline headers. The full process is documented in `Docs/xoceanus_new_engine_process.md`.

---

## Expression and Playability

Every engine in XOceanus has velocity-to-timbre routing. Velocity does not just change volume — it opens filters, shifts harmonic content, and triggers envelope curves differently depending on how hard you play.

All MIDI-capable engines have aftertouch (23/23) and mod wheel (22/22) wired to meaningful destinations. These are not mappings to generic parameters. Each engine's expression inputs are routed to the parameter that makes the most sense for that character.

The PlaySurface provides four playable zones: Pad, Fretless, and Drum modes, each surfacing a different way to interact with the same underlying engine.

---

## Quality Pass — What Got Fixed

A 12-round quality sweep ran across the original engine fleet before the new engines were added. The significant results:

- All six doctrinal compliance rules resolved fleet-wide (velocity-to-timbre, modulation, physical model rigor, zero dead parameters, breathing LFOs, expression routing)
- Zero duplicate preset names in the factory library
- 100% 6D Sonic DNA coverage across all presets
- All presets validated against the `.xometa` schema
- auval PASS for all 42 engines as an AU plugin on macOS

---

## What Is Coming Next

**V1.1** — additional preset volumes, XPN pack export for MPC compatibility, audio sample recordings of hero presets for the website.

**Shaper Bus** — a new class of utility engine is in architecture. XObserve (parametric EQ) and XOxide (2D character shaper) are the first two. They shape sound rather than make it. Coming post-V1.

**Collections** — three V2 paid expansion packs are designed and in production. Details announced closer to release.

---

## A Note on the Number 42

*[Historical note: This section describes the state at V1 draft (2026-03-21), when 42 engines were registered. The fleet has since grown to 76 engines. The narrative below is preserved as written.]*

XOceanus started as a question: what if every character instrument in the XO_OX family could speak at the same time, through each other?

42 engines is not where we planned to land. It is where the question led. There are still engines we have not built yet. There are coupling types we have not written. There are sounds that only become possible when two specific engines are routed into each other in a configuration nobody has tried.

That is what XOceanus V1 is. The starting condition.

---

*XO_OX Designs — XOceanus V1 — March 2026*
*xo-ox.org*
