# XPN Pack Roadmap — R&D Spec
**Date**: 2026-03-16 | **Status**: Draft | **Category**: Release Strategy

---

## Overview

XOceanus ships with 34 registered engines + 4 V1 concept engines (OSTINATO, OPENSKY, OCEANDEEP, OUIE).
No XPN packs exist yet. This document answers: which engines get packs first, in what order, and why.

A pack is a curated `.xpn` bundle — programs, kits, and/or keygroup instruments — built from an
engine's factory presets, rendered to samples and mapped to MPC pads or zones. Each pack must work
standalone on MPC without the plugin installed.

---

## 1. Prioritization Framework

### Axis 1 — MPC Playability Score

Engines are not equally suited to the MPC context. Three factors determine fit:

| Factor | Weight | Notes |
|--------|--------|-------|
| Pad-native expression | 40% | Does pad velocity and pressure map naturally to the engine's timbre axis? |
| Self-contained identity | 35% | Does one program tell a complete story on its own, without coupling? |
| Export complexity | 25% | How many render passes, velocity layers, and round-robin cycles are needed? |

### Axis 2 — Audience Breadth

MPC users skew hip-hop, trap, and lo-fi first; electronic/experimental second; world music third.
Pack 1–6 must land firmly in the first tier. Packs 7–12 can broaden.

### Axis 3 — Narrative Momentum

A pack is also a press release. Early packs establish XO_OX's identity on MPC. The sequence must
communicate: **rhythmic power → melodic character → experimental depth**. Never lead with a texture
engine — you lose the producer before they trust the brand.

### Tier Classification

**Tier 1 — Drum & Percussion** (highest priority): Pad-native, velocity-shaped, immediate wow.
- ONSET — 8-voice synthesis engine, the most natural MPC instrument in the fleet.

**Tier 2 — Character Voice** (second priority): Melodic engines with strong personality, wide appeal.
- OBLONG, OBESE, OVERDUB, OVERWORLD, OVERBITE

**Tier 3 — Expressive Lead/Bass** (third priority): Instruments requiring some programming knowledge.
- ODYSSEY, OPAL, ORGANON, ORACLE, ORBITAL

**Tier 4 — Texture & Atmosphere** (fourth priority): Specialized, slower burn, more discerning audience.
- OCELOT, OBSCURA, OBSIDIAN, ORIGAMI, OSPREY, OSTERIA, OCEANIC, OWLFISH, OHM, ORPHICA

**Tier 5 — Coupling / Context-Dependent** (fifth priority): Best experienced inside XOceanus, harder
to represent standalone.
- OUROBOROS, OBLIQUE, OPTIC, OMBRE, ODDFELIX, ODDOSCAR, OCTOPUS, ORCA, OUTWIT, OVERLAP

**Concept Engines** — build pending, packs follow DSP completion:
- OSTINATO, OPENSKY, OCEANDEEP, OUIE

---

## 2. First 5 Pack Releases

### Pack 1 — ONSET: "Iron Machines"

**Engine**: ONSET | **Accent**: Electric Blue `#0066FF` | **Tier**: Drum & Percussion

The clearest XPN story in the fleet. ONSET is an 8-voice drum synthesis engine with individual
voices mapped to MPC pads by default. The XVC Cross-Voice Coupling (Blessing B002) and Dual-Layer
Blend (Blessing B006) mean every kit can yield velocity-responsive, organically imperfect hits
that rival sample-based kits.

**Pack structure**:
- 10 kits × velocity layers (4 layers: ghost, soft, medium, hard)
- Cycle/random-norepeat round-robin for snares and hi-hats
- Q-Link assignments: MACHINE (character), PUNCH (transient), SPACE (room), MUTATE (variation)
- Kit archetypes: 808 body, TR-909, acoustic lo-fi, industrial, drum and bass, abstract

**Why first**: Hip-hop and trap producers evaluate an MPC expansion in 30 seconds. A drum pack
is the only pack that earns instant credibility. ONSET wins this slot by wide margin.

---

### Pack 2 — OBLONG: "Workshop"

**Engine**: OBLONG (XOblong / Bob) | **Accent**: Amber `#E9A84A` | **Tier**: Character Voice

OBLONG is XOblongBob — 72 source files, 167 presets, Apple Liquid Glass UI, PlaySurface chord/scale
modes. On MPC it maps cleanly to keygroup programs: melodic leads, chords, and bass. The Amber
identity reads immediately as warm and tool-like — a producer's workhorse instrument.

**Pack structure**:
- 4 keygroup programs: Lead, Bass, Chord Stab, Bell
- 25 programs per archetype
- Velocity curve: Vibe's musical velocity curve (compressed dynamics, expressive top)
- Q-Link assignments: CHARACTER, MOVEMENT, COUPLING, SPACE macros

**Why second**: After drums, producers look for a melodic anchor. OBLONG has the broadest melodic
appeal in the fleet — neither too weird nor too generic. "Workshop" signals professional utility
over novelty, building producer trust before we get experimental.

---

### Pack 3 — OBESE: "Gut Physics"

**Engine**: OBESE (XObese / Fat) | **Accent**: Hot Pink `#FF1493` | **Tier**: Character Voice

OBESE is a bass-forward character synth with MOJO Control (Blessing B015) — the orthogonal
analog/digital axis that makes its bass movement genuinely unpredictable. Hot Pink reads as bold
and deliberate. This is the pack for 808 alternatives, sub bass design, and gritty melodic bass.

**Pack structure**:
- 3 keygroup programs: Deep Sub, Grit Bass, Melodic Bass
- Velocity → saturation drive (fat_satDrive) — louder hits get grittier
- Q-Link: BELLY (sub depth), BITE (distortion), analog/digital MOJO axis
- 20 single-shot samples for pad-mode 808 drops

**Why third**: Hip-hop producers will already trust the brand after Iron Machines. Gut Physics
converts them into return buyers. OBESE's Hot Pink accent also photographs beautifully in
marketing contexts — strong social media asset.

---

### Pack 4 — ORACLE: "Signal Noise"

**Engine**: ORACLE (XOracle) | **Accent**: Prophecy Indigo `#4B0082` | **Tier**: Expressive Lead

ORACLE earned a 8.6/10 seance score and Blessing B010 (GENDY Stochastic Synthesis + Maqam).
By pack 4, we've established rhythmic and melodic credibility. Now we introduce mystique.
Signal Noise positions Oracle as the experimental lead engine — pitched stochastic textures
that still play well on a keyboard.

**Pack structure**:
- 4 keygroup programs: Stochastic Lead, Maqam Pad, Noise Melody, Breakpoint Bass
- Velocity → stochastic deviation amount (wilder hits on harder plays)
- Extended tuning: includes quarter-tone Maqam scales as MPC scale presets where applicable
- 2 drum kits: "Oracle Noise Kit" — percussive hits derived from oracle_breakpoints modulation bursts

**Why fourth**: Experimental producers are an important secondary audience and brand signal.
Oracle's seance credentials (8 ghosts unanimous, Buchla gave B010) justify a bold intro.
The Indigo accent differentiates it visually from the warm first 3 packs.

---

### Pack 5 — OVERWORLD: "Chip Arcana"

**Engine**: OVERWORLD (XOverworld) | **Accent**: Neon Green `#39FF14` | **Tier**: Character Voice

OVERWORLD is a chip synth — NES 2A03 + Genesis YM2612 + SNES SPC700 — with the ERA Triangle
(Blessing B009), a 2D timbral crossfade across chip eras. Gaming community crossover potential
is significant; retro aesthetics are perennially strong in hip-hop sampling culture.

**Pack structure**:
- 3 keygroup programs: NES Lead, Genesis Bass, SNES Pad
- 1 drum kit: "Chip Drums" — quantized 8-bit percussion
- ERA crossfade exported as 3 velocity zones per program (one per era)
- Q-Link: ERA X-axis, ERA Y-axis (crossfade into hybrid era textures), CRT glitch amount

**Why fifth**: Completes the first-quarter launch with a strong genre-crossover story. Chip music
sits at the intersection of hip-hop, lo-fi, and gaming — three of the largest MPC user communities.
Neon Green reads unmistakably distinct from the previous 4 packs in any store grid layout.

---

## 3. Collection Engine Assignments

Collections are curated multi-engine bundles built around a theme. Each collection ships as a
single `.xpn` ZIP containing programs from 4–6 engines. Engine-to-collection assignments below
are provisional; final curation depends on coupling chemistry and sound design pass results.

### Kitchen Essentials — "Everyday Synthesis"
*Architecture: Voice × FX Recipe × Wildcard (boutique synth character)*

| Engine | Role in Collection |
|--------|-------------------|
| OBBLIGATO | Dual wind — harmonic filling, pad layer |
| OBLONG | Melodic anchor — leads and bass |
| OHM | Sage warmth — chords, sustained tones |
| OTTONI | Triple brass — stab and swell |
| ORGANON | Metabolic texture — the wildcard ingredient |
| OCEANIC | Phosphorescent tonal wash — the reverb substitute |

Rationale: Kitchen Essentials should sound immediately useful in a mix context. These engines
cover the most common synthesis roles (pad, lead, bass, brass stab, texture) without demanding
deep knowledge of any single engine.

### Travel / Vessels — "Geography of Sound"
*Architecture: Voice × FX Chain (genre signature) × Era Wildcard*

| Engine | Role in Collection |
|--------|-------------------|
| OSPREY | Shore System — coastal cultural data, world melodics |
| OSTERIA | Porto Wine warmth — European harmonic character |
| OCEANIC | Open water — transition and movement |
| OUROBOROS | The cyclical return — long-form voyage arc |
| OLE | Afro-Latin energy — the arrival |

Rationale: Travel/Vessels is built around geographic and cultural identity. OSPREY and OSTERIA
share the ShoreSystem (Blessing B012), creating native harmonic coherence across the collection.

### Artwork / Color — "Visual Synthesis"
*Architecture: Voice (art style) × FX (color science) × Wildcard (complementary color)*

| Engine | Role in Collection |
|--------|-------------------|
| ORACLE | Stochastic expressionism |
| ORIGAMI | Fold geometry — parametric structure |
| OBSCURA | Daguerreotype silver — analog photographic grain |
| ORBITAL | Color field — group envelope expressionism |
| OPTIC | Phosphor visual modulation — zero-audio identity engine |
| OBLIQUE | Prismatic bounce — RTJ × Funk × Tame Impala |

Rationale: Artwork/Color is the most conceptually demanding collection and should launch after
audiences are already invested in XO_OX. OPTIC (Blessing B005 — Zero-Audio Identity) acts as
the modulation spine of the collection rather than a standalone melodic source.

### Standalone Packs (Not Collection-Bound)
These engines have universal appeal that would be diluted by packaging them into a themed bundle.
They ship as individual packs first; collection inclusion possible in year 2.

| Engine | Rationale |
|--------|-----------|
| ONSET | Drum engine — collections don't need drums; kits are standalone |
| OBESE | Bass engine — universal utility, no theme required |
| OVERWORLD | Gaming crossover — needs its own spotlight |
| OVERDUB | Dub synth + FX — performance-centric, works best alone |
| OVERBITE | Character synth — Five-Macro System (B008) demands dedicated showcase |

---

## 4. Engine-to-Audience Mapping

### Hip-Hop / Trap Producers
Primary MPC audience. Value: groove, bass weight, character, immediacy.

| Engine | What They Get |
|--------|---------------|
| ONSET | Complete drum synthesis — 808 kick, snare, hat construction |
| OBESE | Sub bass, grit bass, MOJO Control unpredictability |
| OBLONG | Melodic workhorse — leads, stabs, PlaySurface chord modes |
| OVERDUB | Dub delay + spring reverb (B004) — looping and performance FX |
| OVERBITE | Fang White character — "villain synth" energy, B008 Five-Macro |

### Electronic / Experimental Producers
Secondary audience. Value: depth, unpredictability, system-level thinking.

| Engine | What They Get |
|--------|---------------|
| ORACLE | GENDY stochastic + Maqam (B010) — genuinely novel algorithm |
| OUROBOROS | Leash Mechanism (B003) + Velocity Coupling Outputs (B007) |
| ORIGAMI | Fold geometry — deterministic but nonlinear |
| OBSCURA | Physical string model with daguerreotype character |
| OCTOPUS | Decentralized 8-arm Wolfram CA (XOutwit-inspired) |

### World Music / Cultural Producers
Tertiary audience. Value: tuning systems, cultural resonance, acoustic instrument emulation.

| Engine | What They Get |
|--------|---------------|
| OSPREY | Shore System — 5 coastline cultural tunings (B012) |
| OSTERIA | Porto Wine warmth — European string/guitar character |
| OHM | Sage hippie-dad energy — MEDDLING/COMMUNE axis |
| OLE | Afro-Latin trio — DRAMA macro |
| ORPHICA | Microsound harp — siphonophore delicacy |

### Sound Design / Cinematic Composers
Quaternary audience. Value: complexity, environmental sound, systems that evolve over time.

| Engine | What They Get |
|--------|---------------|
| ORGANON | Variational Free Energy metabolism (B011) — publishable-level complexity |
| ORBITAL | Group Envelope System (B001) — layered slow-bloom design |
| OPAL | Granular synthesis — 150 presets, atmospheric range |
| OPTIC | Visual modulation (B005) — synthesis without sound |
| OBSIDIAN | Crystal White — crystalline harmonic stasis |
| ORCA | Apex predator wavetable + echolocation + breach events |

---

## 5. V1 Concept Engine Packs

These four engines had their DSP completed on 2026-03-18 and are now installed in XOceanus.
Their pack strategies below were planned before DSP and remain valid — preset libraries still need building.

### Polarity Pair Strategy — OPENSKY + OCEANDEEP

OPENSKY (Sunburst `#FF8C00`) and OCEANDEEP (Trench Violet `#2D0A4E`) are the purest expression
of the feliX-Oscar polarity: euphoric shimmer vs. abyssal bass. They must launch together as a
dual pack: **"The Full Column"**.

**"The Full Column" — Joint Pack Concept**

| Pack Half | Engine | Programs |
|-----------|--------|---------|
| Top of Water | OPENSKY | 5 keygroup programs — shimmer leads, bright pads, sky textures |
| Bottom of Trench | OCEANDEEP | 5 keygroup programs — sub bass, pressure drones, abyssal hits |
| Crossover zone | Both | 2 programs using samples from both engines — "The Thermocline" |

This is a rare twin-engine pack. The visual contrast (Sunburst vs. Trench Violet) makes it
self-describing in any store grid. Aquarium water column mythology provides ready-made
editorial context.

### OSTINATO — Communal Drum Circle
Launch as a complement to ONSET: where Iron Machines is a single producer's kit, OSTINATO is
a living ensemble. Pack concept: **"Circle"** — 6 programs representing different communal
percussion traditions. Q-Links should map to ENERGY, SYNC, DRIFT, DENSITY.

### OUIE — Duophonic Hammerhead
OUIE's STRIFE↔LOVE axis needs a dedicated showcase. Pack concept: **"Tension Arc"** —
programs that begin at one pole and reward macro exploration toward the other. Duophonic
voice architecture makes it unusual in MPC keygroup context — lean into that in the editorial.

---

## 6. 12-Month Release Cadence

**Principle**: 1 pack per month for 12 months. Lead with drums, build with melody, earn the
right to get weird.

| Month | Pack | Engine | Tier | Audience |
|-------|------|--------|------|----------|
| 1 | Iron Machines | ONSET | Drum | Hip-hop / Universal |
| 2 | Workshop | OBLONG | Character | Hip-hop / Melodic |
| 3 | Gut Physics | OBESE | Character | Hip-hop / Bass |
| 4 | Signal Noise | ORACLE | Expressive | Electronic / Experimental |
| 5 | Chip Arcana | OVERWORLD | Character | Gaming / Lo-fi |
| 6 | Send & Return | OVERDUB | Character | Dub / Performance |
| 7 | Fang White | OVERBITE | Character | Hip-hop / Experimental |
| 8 | Shore Songs | OSPREY | Texture | World / Cultural |
| 9 | The Full Column | OPENSKY + OCEANDEEP | Concept | Cinematic / Universal |
| 10 | Fold Logic | ORIGAMI | Texture | Sound Design |
| 11 | Kitchen Essentials | Collection | Multi-engine | Broad |
| 12 | Circle | OSTINATO | Concept | Hip-hop / World |

### Rationale for Month 6–12 Choices

**Month 6 — OVERDUB "Send & Return"**: The spring reverb (B004) and tape delay are MPC-friendly
performance effects. A dedicated pack positions OVERDUB as the go-to dub layer for producers
who already have Iron Machines kits.

**Month 7 — OVERBITE "Fang White"**: The Five-Macro System (B008) deserves a pack built around
demonstrating each macro independently. Month 7 is early enough to matter, late enough that
buyers already trust the brand.

**Month 8 — OSPREY "Shore Songs"**: World music producers need 8 months of runway before they
find an MPC expansion catalog. Shore Songs arrives when that audience has been warmed up by
the first 7 packs.

**Month 9 — The Full Column**: The twin polarity pack is a major creative statement and marketing
event. Month 9 is the right moment — after the fleet has proven itself rhythmically and
melodically, the brand earns the right to make a conceptual statement.

**Month 10 — ORIGAMI "Fold Logic"**: Sound designers and cinematic composers represent an
important quarterly revenue segment. Fold Logic's parametric geometry appeals directly to this
audience.

**Month 11 — Kitchen Essentials Collection**: The first multi-engine pack is the biggest
structural bet of year 1. Launching it at month 11 means buyers already know OBBLIGATO,
OBLONG, OHM, and OTTONI individually — the collection becomes a reunion rather than an introduction.

**Month 12 — OSTINATO "Circle"**: Close year 1 with the communal drum circle — a contrast to
Iron Machines that demonstrates how far the fleet has grown. If OSTINATO DSP is delayed,
substitute OPAL "Grain Theory" (granular engine with strong cinematic appeal).

---

## 7. Open Questions

1. **XPN ZIP packaging pipeline**: Does `Tools/` support batch rendering all velocity layers + round-robin
   cycles for a 10-kit ONSET pack without manual intervention? If not, fleet_render_automation_spec.md
   should be extended before Pack 1 production begins.

2. **Store platform**: Are packs sold through XO-OX.org directly, Patreon, or a third-party marketplace
   (Splice, MPC forum)? Platform choice affects pack pricing, bundle strategy, and community submission
   pipeline timing.

3. **OSTINATO DSP timeline**: Month 12 assumes OSTINATO ships in V1. If concept-engine DSP build takes
   longer than 6 months post-Constellation, OPAL "Grain Theory" is the Month 12 fallback.

4. **Coupling packs**: Several engines (OUROBOROS, OBLIQUE, OVERLAP) are most interesting when
   coupled. A future "Coupling Collection" format — where programs are rendered from coupled
   engine pairs — is worth scoping in year 2 but is outside V1 XPN scope.

5. **Community submission cadence**: The community_pack_submission_rnd.md pipeline should be open
   by Month 6, so community creators can submit packs that complement Months 7–12.

---

## Summary

| Priority | Engine | Pack Name | Why First |
|----------|--------|-----------|-----------|
| 1 | ONSET | Iron Machines | Drums land first, always |
| 2 | OBLONG | Workshop | Melodic trust-builder |
| 3 | OBESE | Gut Physics | Bass = hip-hop credibility |
| 4 | ORACLE | Signal Noise | Mystique and seance credentials |
| 5 | OVERWORLD | Chip Arcana | Gaming community crossover |
| 6 | OVERDUB | Send & Return | Dub FX layer for drum buyers |
| 7 | OVERBITE | Fang White | Character showcase, B008 |
| 8 | OSPREY | Shore Songs | World music audience arrives late |
| 9 | OPENSKY + OCEANDEEP | The Full Column | Polarity pair statement |
| 10 | ORIGAMI | Fold Logic | Sound design quarter |
| 11 | Collection | Kitchen Essentials | First multi-engine bundle |
| 12 | OSTINATO | Circle | Year-1 close, communal finale |
