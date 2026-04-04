# Chapter 9: The Future of XO_OX — What's Coming

*Written by Kai, Sound Design Lead, XO_OX Designs*

---

I want to talk to you about where this is going. Not as a hype document — not "here are exciting features coming soon" — but as an honest map of what we have already built, what is in progress, and what the next two years of XO_OX look like if things go the way we think they will.

There is a lot coming. Some of it is almost here. Some of it is three years out and still changing shape. All of it is connected by the same thing that drove the original 34 engines: character instruments over feature count, and a belief that the best synthesis tools have a point of view.

---

## 9.1 V1 Complete: What Ships

Let me start with what already exists, because the scope of it is easy to miss when you are inside it.

XOceanus V1 ships with **34 synthesis engines**. That is not 34 presets. It is not 34 variations on a similar synthesis approach. It is 34 distinct characters — each built as a standalone instrument first, each with its own DSP architecture, its own aquatic mythology, its own accent color in the Gallery, its own factory preset library. The smallest preset library in the fleet is 80 presets; the largest is 200. The total is ~17,300 factory presets, all in fifteen mood categories, all with 6D Sonic DNA tags, all tested in production context.

Alongside the 34 engines, V1 ships the **Oxport Tool Suite** — eight tools for MPC producers that handle the full export pipeline from XOceanus patch to installed MPC program:

- **Drum Export** — Converts ONSET programs to MPC XPN format with full voice routing.
- **Keygroup Export** — Maps any engine's chromatically rendered output to MPC keygroups.
- **Kit Expander** — Builds multi-layer velocity/cycle kits from single-sample sources.
- **Bundle Builder** — Assembles complete XPN expansion bundles for distribution.
- **Cover Art Generator** — Produces Gallery-style artwork for MPC program tiles.
- **Packager** — Finalizes and zips bundles for delivery.
- **Sample Categorizer** — Tags samples by sonic DNA for intelligent organization.
- **Render Spec** — Specifies headless render parameters for the automation pipeline.

And underpinning all of it: the **MPC ecosystem integration** that this book is about. XPN export, Q-Link macro mapping, per-program velocity curves, CycleType round-robin, the PadNoteMap and PadGroupMap conventions — all documented, tested, and built to the exact conventions the MPC firmware expects.

V1 is a complete ecosystem for MPC producers who want synthesis with character. Not just sounds. Systems.

---

## 9.2 The 4 Concept Engines Coming in V1

Four engines are approved, designed, and on the build schedule. Their DSP is not yet written — but their identities are complete, their parameter architectures are documented, and their place in the water column is confirmed. They are V1 scope, not V2. We are building for a complete ecosystem at launch.

**OSTINATO** — The communal drum circle engine. Eight seats around a fire, any of twelve world percussion instruments in any seat, no tradition restrictions. Djembe next to taiko next to tabla — the circle doesn't care where you're from. Firelight Orange `#E8701A`. The synthesis approach is hybrid physical modeling: Exciter → Modal Membrane → Waveguide Body → Radiation Filter. What makes OSTINATO singular is its 4x4 seat interaction matrix: every seat can drive every other seat's accent, dynamics, and timing. A djembe hit in seat 1 can trigger an accent response in the tabla in seat 5. The fire in the center connects everyone. Build pending.

**OPENSKY** — The euphoric shimmer engine. Pure feliX polarity — the only engine that lives above the water's surface, in the open air, in the light. Supersaw oscillator stack, shimmer stage, bright filter, chorus/reverb. Sunburst `#FF8C00`. OPENSKY is the anthemic end of the fleet — the sound of a drop, a release, a sunrise. Its four macros are RISE, WIDTH, GLOW, and AIR. Its signature coupling is the Full Column: OPENSKY at one end, OCEANDEEP at the other, feliX and Oscar poles in one patch, the entire mythology compressed into a single preset. Build pending.

**OCEANDEEP** — The abyssal bass engine. Pure Oscar polarity — the absolute ocean floor, the counterpart to OPENSKY. 808-style pressure synthesis, bioluminescent creature modulation, shipwreck waveguide bodies (the resonance of sunken hulls, cave systems, trench walls). Trench Violet `#2D0A4E`. OCEANDEEP is the lowest point in the fleet — the only engine that lives at the true bottom of the water column. Its four macros are PRESSURE, CREATURE, WRECK, and ABYSS. The Full Column coupling with OPENSKY covers the entire vertical range of the XO_OX mythology in a single patch. Build pending.

**OUIE** — The duophonic hammerhead shark engine. Dead-center on the feliX-Oscar axis — the thermocline boundary hunter, the only predator whose anatomy evolved two independent sensory platforms fused into one animal. Two algorithms, two synthesis voices, two perspectives on the same note. The HAMMER macro sweeps between STRIFE (cross-FM, ring mod, phase cancellation — the brothers disagreeing) and LOVE (spectral blend, harmonic lock, unison — the brothers fusing). Hammerhead Steel `#708090`. OUIE is unique in the fleet for its dead-center polarity: it is equally feliX and Oscar, which makes it the natural bridge between any two engines at extreme polarity. Build pending.

These four engines will complete the water column. From OPENSKY above the surface to OCEANDEEP at the ocean floor, the full vertical mythology will be covered. All four are V1.

---

## 9.3 Utility Engines — A New Category

Here is something we have not talked about publicly yet: there is a second category of engine coming to XOceanus, and it is architecturally different from everything in Chapter 8.

The 34 synthesis engines in the current fleet are all creatures. They all MAKE sound. They inhabit the water column; they have biological identities; they couple and collide and produce audio. That is what synthesis engines do.

Utility engines are different. Utility engines are **musicians**. They SHAPE sound — they process, gate, spatialize, harmonize, and sequence the output of synthesis engines. A synthesis engine is a creature. A utility engine is the artist who works with what the creatures produce.

The distinction is important because it changes what "character" means for a utility engine. A synthesis engine has the character of its creature — the owlfish's predatory patience, the siphonophore's divine precision. A utility engine has the character of a performer — their approach to manipulation, their signature way of reshaping raw material.

**V1 Rapper Bundle** — The first utility engine bundle will be themed around rappers: artists who chop, sample, remix, layer, and transform. This maps exactly to what utility engines do. A rapper-themed gate doesn't just gate — it chops with swagger. A rapper-themed harmonizer doesn't just harmonize — it stacks like a verse and a hook. The V1 Rapper Bundle will contain 10 utility engines, each named after and inspired by a rapper's approach to sound construction.

We are not announcing names yet. What we can tell you is the 10 engine archetypes: gate/chopper, sample slicer, harmonic stacker, spatial spreader, compressor/sidechain, pitch manipulator, filter sequencer, rhythmic arpeggiator, reverb designer, and signal router. Every one of these tools already exists in the world as a generic plugin. None of them have a character. The Rapper Bundle changes that.

The Musician metaphor has longevity: after rappers come DJ culture, jazz improvisers, classical conductors, rock innovators, world music masters, experimental/avant-garde artists. Each bundle packages the same utility archetypes through a completely different performance philosophy. The utility engine library will eventually be as large as the synthesis engine library — and every bit as opinionated.

---

## 9.4 MPCe and 3D Pads — The Platform Shift

If you are reading this book in 2026, you may have already heard about the Akai MPCe — Akai's next-generation MPC hardware with quad-corner pads instead of the traditional 4×4 grid. Each pad has four independent pressure-sensitive zones, one at each corner, each assignable to a different parameter. A single pad becomes a 4-way performance surface.

This changes everything about how XO_OX is designed.

The XOceanus PlaySurface — our 4-zone unified playing interface across Pad, Fretless, and Drum modes — was designed before the MPCe was announced. But when we saw the MPCe specs, we realized: we had already built the software architecture for exactly this hardware. Four corner zones. Independent parameter assignment per zone. The PlaySurface was prescient.

Right now, every macro in every XOceanus preset is designed with four macros (CHARACTER, MOVEMENT, COUPLING, SPACE) that map directly to the four Q-Link knobs on current MPC hardware. On the MPCe, those four macros become the four corners of every pad. Not four knobs you reach for — four pressure points under your fingers while you play.

The implication: on MPCe, you are not playing notes with your right hand and tweaking knobs with your left. You are playing the synthesis and the modulation simultaneously with both hands, using every finger as a performance parameter. The COUPLING macro — which Chapter 4 describes as the most expressive Q-Link in a live performance context — becomes a pressure sensitivity dimension of the pad itself. You push into the corner and the engines entangle. You release and they separate.

We are building for this. Not retrofitting — building from first principles. Every preset we design now is tested for how it performs in a 4-zone pressure context, not just in a Q-Link sweep context. The fleet is ready.

---

## 9.5 The Collections — V2 Paid Expansion

V1 is the foundation. V2 is where the world gets very wide.

Three collections are designed and waiting for their engines to be built. These are not single-engine additions — they are themed ecosystems of 20–24 engines each, organized around a central concept, built to work together as a complete creative world.

**Kitchen Essentials Collection** — Six culinary quads, each quad anchored by four engines representing a Voice, an FX Recipe, a Wildcard, and a Fusion instrument. The metaphor is the kitchen: synthesis engines are ingredients, utility engines are techniques, and the collection as a whole is a recipe book for sonic cooking. Twenty-four engines. Proposed release: V2.

**Travel / Water / Vessels Collection** — Five journey sets, each representing a sailing tradition: Sail (Woodwinds × Hip Hop), Industrial (Brass × Dance), Leisure (Island Cultural × Island Music), Historical (Historical Percussion × Synth Genres), and Sable Island (Fusion × Cross-Genre). Twenty engines. The Architecture: Voice × FX Chain (genre signature) × Era Wildcard. Proposed release: V2.

**Artwork / Color Collection** — Five quads built around visual artists and color science. Color Quad A (Oxblood/Onyx/Ochre/Orchid — Erhu/Didgeridoo/Oud/Guzheng), Color Quad B (Ottanio/Oma'oma'o/Ostrum/Ōni — Handpan/Angklung/Sitar/Shamisen), plus three thematic quads (Showmen, Aesthetes, Magic/Water) and an Arcade Fusion fifth slot. Twenty-four engines. Rich historical and artistic content — the lore here extends into blog posts, e-books, and zines. Proposed release: V2.

These collections will be paid expansions. V1 is, and will remain, free and open-source. The collections fund the next several years of development while keeping the core fleet accessible.

---

## 9.6 Fleet Render Automation — When Packs Become Instant

This one is for the producers who have spent an afternoon rendering fifty keygroup samples one at a time and then manually building the XPN structure. We see you. We are working on it.

Fleet Render Automation is our term for the headless rendering pipeline that will eventually allow you to say "render this preset as a 37-note chromatic keygroup pack" and have the MPC program appear in your project five minutes later with no manual steps.

The architecture exists in rough form in the Render Spec tool in the Oxport suite. What is missing is the full headless render integration — the ability to run XOceanus in a non-GUI mode, accept a render specification, synthesize the audio, and output an XPN-ready bundle without the user touching anything.

The timeline is 2026. The dependencies are: stable V1 with all 34+4 engines installed, the headless audio render API in JUCE finalized, and the Render Spec tool validated against all engine types. We are not there yet. But the Render Spec tool was designed specifically so that the work of specifying a render — which notes, which velocity layers, which round-robin cycle — can be done in advance and saved, so that when the headless pipeline arrives, every preset you have ever built already has its render specification waiting.

If you are building XPN packs now, build your Render Specs alongside them. Future you will be grateful.

---

## 9.7 The Community Vision

Everything described so far is software. Engines, presets, tools, pipelines. But the part of XO_OX that we think about most is harder to version and harder to ship.

It is the community.

Three programs are in early design:

**Seed+Grow** — A mentorship and collaboration program where established producers partner with new producers around XOceanus. The metaphor: a seed (new producer) gets planted in the soil of a mentor (experienced producer), grows with specific guidance, and eventually produces something neither could have made alone. Seed+Grow pairs will create joint preset libraries — the mentor's craft and the seed's fresh ears in collaboration. These libraries will be distributed through XO-OX.org and credited to both producers.

**DNA Challenge** — A periodic community challenge where a "seed preset" is released: a single engine patch with a Sonic DNA signature (brightness: 0.8 / warmth: 0.3 / movement: 0.6 / density: 0.5 / space: 0.7 / aggression: 0.2 — or whatever the challenge specifies). Community members couple that seed with their own engines, modify it through the coupling matrix, and submit the result. The rule: the Sonic DNA of the submitted patch must be measurably different from the seed preset in at least three dimensions. The point: coupling transforms. The community demonstrates that transformation in real time, and we publish the best submissions as a community expansion pack.

**Kit Family Tree** — A visual lineage tracker for user-created kits. When you build a kit from an existing kit (modifying, adding, coupling differently), you mark it as a "child" of the parent kit. The Kit Family Tree maps these relationships — you can see that a kit published by a producer in Lagos has 23 children built by producers in Seoul, São Paulo, Lagos, and Chicago, and that three of those children have become parents of their own lineages. The tree makes visible what coupling makes audible: these sounds are in conversation with each other. They evolve. They have ancestry and descendants.

None of this is live yet. Seed+Grow is in design. The DNA Challenge format is prototyped. The Kit Family Tree needs the XO-OX.org infrastructure. But the vision is clear: XO_OX is not a plugin company. It is a synthesis ecosystem, and an ecosystem has inhabitants, not just users.

---

## 9.8 A Personal Note from XO_OX

I want to end this book the same way I think about every session in XOceanus — by saying something true about what we are actually doing here.

We are not trying to make the most powerful synthesizer. We are not trying to have the most engines, the most presets, the most parameters, the most coupling types. There are plugins with more of all of those things. Some of them are excellent.

What we are trying to do is harder to describe and I think more rare: we are trying to give every sound in the fleet a reason to exist beyond its function. The owlfish is not in XOceanus because we needed a monophonic bass engine. It is in XOceanus because the *owlfish* — a real creature living in crushing darkness at 2,000 meters, with eyes like searchlights evolved to find photons in absolute black — *is the synthesis*. The extreme compression that pulls inaudible harmonics into audibility is not a feature. It is the creature's biology, translated into audio. When you turn up PRESSURE and the subharmonics deepen and the reverb tail extends into the abyss, you are not adjusting a macro. You are descending.

This is what we mean by character instruments. It is not branding. It is a design constraint: every feature must support a sonic pillar, and every sonic pillar must be grounded in something real — a creature, a physical phenomenon, a cultural tradition, a relationship between two things. The reef that Oblong provides is literally the architecture Oscar lives in. The in-laws that interfere with OHM's jam are nautiluses because nautiluses are mathematical and cold and they measure everything in logarithmic spirals. The flying fish brothers of OBBLIGATO can't sit still because flying fish literally launch from the water and glide.

We believe this approach produces better instruments. Not because mythology makes audio better — it doesn't, DSP makes audio better. But because when a synthesis concept is grounded in something vivid and specific, the parameters become expressive rather than functional. MEDDLING means something. BONE means something. ECOSYSTEM DEPTH means something. The player understands what they are doing without reading the manual.

The community we are building is made of people who feel that same way: that the best instrument is the one that makes you want to play, not just the one that makes the most technically impressive sound. The MPC is that instrument for a lot of us — not because it is the most powerful sequencer, but because it sits in front of you like a flat square of intention, and you hit the pads and something happens that feels like you, not like the software.

XOceanus is trying to be that, but for synthesis. Thirty-four creatures living in a water column, waiting to be coupled, waiting to collide, waiting for you to find the pairing that neither engine was expecting.

We are just getting started.

---

> **What's Coming When**
>
> **Now (V1.0)**
> — 34 synthesis engines (ODDFELIX through OPAL) — COMPLETE
> — ~17,300 factory presets in 15 moods — COMPLETE
> — Oxport Tool Suite (8 tools) — COMPLETE
> — MPC XPN integration — COMPLETE
> — AU, Standalone (macOS) — COMPLETE
>
> **V1.1 (2026, Q2)**
> — OSTINATO (communal drum circle) — BUILD IN PROGRESS
> — OPENSKY (euphoric shimmer, pure feliX) — BUILD IN PROGRESS
> — OCEANDEEP (abyssal bass, pure Oscar) — BUILD IN PROGRESS
> — OUIE (duophonic hammerhead, dead-center) — BUILD IN PROGRESS
> — Fleet raises to 39 engines
>
> **V1.2 (2026, Q3)**
> — V1 Rapper Bundle — 10 utility engines (names TBA)
> — Utility engine category launches in Gallery
> — VST3 format
>
> **V1.3 (2026, Q4)**
> — Fleet Render Automation (headless XPN rendering)
> — MPCe PlaySurface 4-zone pressure mapping
> — Seed+Grow community program launches
>
> **V2.0 (2027, H1)**
> — Kitchen Essentials Collection (paid expansion)
> — Travel / Water / Vessels Collection (paid expansion)
> — DNA Challenge and Kit Family Tree programs
> — iOS AUv3 release
>
> **V2.x (2027–2028)**
> — Artwork / Color Collection (paid expansion)
> — Additional musician-themed utility engine bundles
> — Theorem engines (OVERTONE, KNOT, ORGANISM) — research phase

---

The water column keeps getting deeper.

---

*Thank you for reading The MPC Producer's XO_OX Field Manual.*
*More at XO-OX.org | Free download at XO-OX.org/download*
