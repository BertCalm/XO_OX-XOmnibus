# XPN Field Guide Articles — R&D Spec

**Date:** 2026-03-16
**Status:** Draft — Proposal
**Author:** XO_OX Designs
**Category:** Content Strategy / Field Guide / XPN Pack Craft

---

## Overview

The XO_OX Field Guide at `/guide/` on XO-OX.org has 14 published posts (~47,700 words). The current
corpus covers brand philosophy, engine deep dives, the XPN format introduction, and preset architecture.
What it does not yet cover — with authority — is the craft of MPC expansion pack design itself.

This spec proposes 10 new articles that collectively establish XO_OX as the primary editorial voice
on MPC expansion pack craft. Each article is written for a producer who uses a hardware MPC and cares
deeply about how their tools are built. The articles operate at two levels simultaneously: they provide
genuine technical value (producers learn something) and they position XO_OX packs as the inevitable
conclusion of that learning (producers understand why this matters in a purchase context).

**Target audience:** MPC producers (MPC X, MPC One, MPC Live II, MPCe) ranging from intermediate
beatmakers to professional sound designers. Familiarity with the MPC workflow assumed; no JUCE or
DSP knowledge required.

**SEO strategy:** Each article targets a medium-competition long-tail keyword where XO_OX can rank
without a massive backlink profile. Avoid high-competition broad terms like "MPC expansion packs."
Target specific, intent-rich phrases producers search when they have a workflow problem.

**Publication cadence:** One article per month, April–January 2027. Staggered with engine deep dives
and brand posts from the existing pipeline so the guide does not become monotone.

---

## Article 1 — "Why Most MPC Expansion Packs Are Wrong About Velocity"

**Hook:** Most expansion packs treat velocity as volume control. XO_OX designed theirs differently — and once you hear the difference, you cannot unhear it.

**Estimated word count:** 2,000

**Target SEO keyword:** `MPC expansion pack velocity layers`

**Target publish date:** April 2026

### Key Sections

**1. What Velocity Actually Controls (And What It Should)**
The physics of acoustic instruments: a harder strike does not simply make a piano louder — it changes
the harmonic distribution, the attack transient character, the sustain profile. Velocity is a proxy
for force, which is a proxy for timbre. Most commodity packs assign velocity purely to amplitude scaling
because it is easy to implement. This is a category error.

**2. XO_OX Doctrine D001 — Velocity Must Shape Timbre**
Introduce the Six Doctrines (briefly). D001 is the most producer-facing: every XO_OX engine has
velocity routed to filter brightness / harmonic content as a primary destination, not just amplitude.
How this was validated across all 34 engines in the Prism Sweep. What "timbre change at soft velocity"
sounds like in practice — describe the effect without requiring audio (link to clips).

**3. The feliX-Oscar Velocity Map**
feliX (precision, clarity) and Oscar (warmth, chaos) define the two poles of XO_OX sound design.
Velocity mapping in XO_OX packs follows this polarity: soft velocities lean Oscar (warmer, rounder,
more complex), hard velocities lean feliX (brighter, more present, cleaner). This is the opposite of
what most packs do (soft = quiet + dull; hard = loud + brighter). Explain why the XO_OX direction
produces more musical results.

**4. How XPN Pack Velocity Layers Work**
Technical but accessible: what a velocity layer is in XPN format, how XO_OX uses 4–6 layers per pad,
how the Sonic DNA velocity mapping system assigns brightness/warmth values per layer. The XPN
`VelStart`/`VelEnd` parameter and the empty-layer ghost trigger bug (and how XO_OX avoids it).

**5. What to Listen For When Evaluating a Pack**
Practical producer guide: play a single pad from soft to hard. Does only the volume change? Does the
tone change? Does the attack character change? A checklist for evaluating any expansion pack's velocity
design quality. Positions XO_OX against commodity packs without naming competitors.

---

## Article 2 — "The ONSET Drum Machine: Designing Kits for Pad Performance"

**Hook:** ONSET is a drum machine synthesizer with 8 synthesis voices, 115 presets, and more parameters
than most analog drum machines combined. Rendering it to a static XPN expansion pack meant making
irreversible decisions about what its soul is. Here is how those decisions were made.

**Estimated word count:** 2,200

**Target SEO keyword:** `MPC drum kit expansion pack design`

**Target publish date:** May 2026

### Key Sections

**1. ONSET Engine Overview**
Brief orientation for producers who have not encountered the Field Guide ONSET deep dive (Post 9).
8 synthesis voices: Kick, Snare, CHat, OHat, Clap, Tom, Perc, FX. 111 parameters. 4 macros: MACHINE,
PUNCH, SPACE, MUTATE. Electric Blue `#0066FF` identity. The axolotl lineage.

**2. The Problem of Rendering a Synthesizer**
A synthesizer is infinite — every parameter position is a different instrument. A sample pack is a
decision. When you render ONSET to XPN, you are saying: this is what ONSET is. The philosophical
weight of that decision. How XO_OX approached it: render the canonical version of each voice, then
design additional layers for expressivity (velocity, round-robin), not just coverage.

**3. What the MPC Gains — and What It Loses**
Gains: immediate pad access, no CPU load, works without a computer, integrates with MPC workflow
(sequences, Q-Links, choke groups). Losses: the ONSET macro system, real-time synthesis mutation,
MUTATE parameter chaos. The XPN pack is ONSET at rest; the plugin is ONSET alive. This is not a
deficiency — it is a different instrument.

**4. Choke Groups on MPC — The HiHat Problem**
The canonical drum machine choke group: open hi-hat silences closed hi-hat. How XPN choke groups
work. How XO_OX sets up ONSET choke group defaults: OHAT chokes CHAT on the same pad bank.
Why getting this wrong makes a drum kit feel mechanical even when the samples are excellent.

**5. Per-Voice Physical Defaults**
Each of ONSET's 8 synthesis voices has a characteristic attack, decay, and transient that informed
the XPN default ADSR settings. The kick has a long decay. The CHat has a razor attack and sub-5ms
decay. These defaults are not arbitrary — they are derived from ONSET's synthesis behavior and
encoded into the XPN file so the kit sounds correct on first load.

**6. The Live Performance Use Case**
ONSET kits are designed for pad performance, not just sequencing. Velocity sensitivity on every
voice. The MUTATE macro maps to Q-Link for live morphing (even in the static pack, Q-Link assignments
route to layered sample variants). What a live ONSET kit performance looks like.

---

## Article 3 — "MPCe Native Design: The Untapped Frontier"

**Hook:** Most expansion pack studios are still designing for the MPC X from 2018. The MPCe ships with
3D pad pressure (NW/NE/SW/SE per pad) that almost no expansion pack uses. XO_OX is designing for that
hardware.

**Estimated word count:** 1,800

**Target SEO keyword:** `MPC expansion packs MPCe 3D pad`

**Target publish date:** June 2026

### Key Sections

**1. The MPC Hardware Timeline**
Brief contextual orientation: MPC One (2020), MPC X (2018), MPC Live II, MPCe (2024). What changed
in the MPCe hardware: the 3D pad matrix. Each pad now has four pressure zones (NW, NE, SW, SE).
This is not widely documented. It is not widely used. It is the most significant expressive advance
in MPC pad hardware in a decade.

**2. What 3D Pad Pressure Unlocks**
The four corners of an MPCe pad can be mapped to four independent modulation sources. A single pad
can be a 4-voice chord. Or the four corners can map to four timbral variants of the same sound.
Or NW/SW can be dry/wet, NE/SE can be a timbral axis. The design space is enormous and almost
entirely unoccupied.

**3. XO_OX Quad-Corner Strategy**
How XO_OX designs packs with MPCe quad-corner intent: feliX corner (NE — bright, precise), Oscar
corner (SW — warm, textured), COUPLING corner (NW — layered with second engine), SPACE corner
(SE — wet, processed). The corners express the feliX-Oscar polarity and the XO_OX coupling philosophy
in physical pad space.

**4. How to Use MPCE_SETUP.md**
The XO_OX pack documentation system includes `MPCE_SETUP.md` files that map the quad-corner
assignments for each pack. Walk through a real example. How to load an MPCe-native pack vs. a
standard XPN pack. The fallback behavior for non-MPCe hardware (the NE corner is the primary sample,
so standard MPC users get the feliX voice by default).

**5. The Broader Argument: Forward-Designed Packs**
The industry habit of shipping packs for current-minus-two hardware. The cost of forward design
(more planning, more samples, more documentation). Why XO_OX makes this investment: packs do not
expire. A pack designed for MPCe in 2026 is still the best pack for MPCe in 2029. Evergreen pack
design as competitive moat.

---

## Article 4 — "feliX and Oscar: A Theory of Timbral Polarity"

**Hook:** Every sound a synthesizer produces lives somewhere on a spectrum between clinical precision
and warm chaos. XO_OX named the two ends of that spectrum, mapped 34 engines across it, and built
every expansion pack with that map in hand.

**Estimated word count:** 2,500

**Target SEO keyword:** `synthesis sound design polarity warm bright`

**Target publish date:** July 2026

### Key Sections

**1. The Problem with "Warm vs. Bright"**
Most sound design vocabulary is vague. "Warm" and "bright" describe opposite ends of a frequency
response — but they do not account for density, movement, harmonic complexity, or chaos. The
feliX-Oscar axis is a more complete polarity: it describes not just frequency character but
personality, behavior under modulation, and musical role.

**2. feliX — The Neon Tetra (Neon Tetra Blue `#00A6D6`)**
feliX is precision, clarity, definition. The feliX pole sounds like: tight attacks, clean transients,
harmonic series with clear fundamental, predictable behavior. feliX sounds are leadable — they sit
in a mix with identity. OPTIC, ODDFELIX, ORIGAMI, and OBLONG lean feliX. Define with examples a
producer can immediately imagine.

**3. Oscar — The Axolotl (Axolotl Gill Pink `#E8839B`)**
Oscar is warmth, ambiguity, texture. The Oscar pole sounds like: soft attacks, complex overtones,
modulation that surprises, behavior that rewards exploration. Oscar sounds are supporting characters —
they fill space, create intimacy, resist definition. OVERDUB, OHM, OPAL, and OCEANIC lean Oscar.
Oscar sounds are often described as "organic" or "alive."

**4. The Axis in Practice — 34 Engines Mapped**
A visual or textual spectrum mapping all 34 XOmnibus engines on the feliX-Oscar axis. (This is
publishable as an infographic companion to the post.) Where ONSET sits (feliX-dominant in Kick/Snare,
Oscar-dominant in FX/Perc). Where OUROBOROS sits (deeply Oscar — the chaos engine). Where OVERWORLD
sits (era-dependent: NES/SNES lean feliX, Genesis/SPC lean Oscar).

**5. Using the Axis as a Producer**
How to use the feliX-Oscar map when choosing sounds. The layering principle: feliX sounds sit above
Oscar sounds in frequency space — they do not cancel, they complement. The XO_OX Mood system uses
the axis: Foundation presets are feliX-dominant, Atmosphere presets are Oscar-dominant, Entangled
presets are axis-crossing. The axis as a production decision tool, not just a brand metaphor.

**6. Velocity and the Axis (Callback to Article 1)**
Cross-reference: the feliX-Oscar velocity map from Article 1 is the axis in action. Soft velocity
moves toward Oscar; hard velocity moves toward feliX. This is the most producer-accessible expression
of the polarity. Link + callback.

---

## Article 5 — "The Pack Is Not the Preset: On the Difference Between Samples and Synthesis"

**Hook:** A sample pack captures a moment. A synthesis engine creates infinite moments. What happens
when you attempt to bridge the two — and what do you necessarily leave behind?

**Estimated word count:** 2,000

**Target SEO keyword:** `sample pack vs synthesis MPC workflow`

**Target publish date:** August 2026

### Key Sections

**1. The Capture Problem**
When you record an acoustic instrument or synthesizer, you record one state of that instrument.
Even with multisampling (velocity layers, round-robin variations), you are covering a finite set
of states. A synthesizer in real-time is not a set of states — it is a continuous function. The
philosophical gap between sample libraries and synthesis engines.

**2. What XO_OX Packs Preserve**
The XPN export pipeline is designed to preserve the parts of synthesis that matter most in a
production context: timbre character (the recognizable voice of the engine), expressivity (velocity
response, round-robin variations), and musical role (what the instrument does in a mix). What it
cannot preserve: real-time macro mutation, coupling interactions, parameter exploration. This is
honest — XO_OX documents the distinction.

**3. The Synthesis-Informed Sample Design Difference**
Standard sample libraries are recorded from instruments. XO_OX packs are designed from synthesis
engines — the sample selection is driven by parameter knowledge. An XO_OX kick drum is the specific
kick that the ONSET Kick voice produces when MACHINE is 40%, PUNCH is 70%, and SPACE is 20%.
That specificity produces a more distinctive sample than "kick drum with some saturation."

**4. The Bridge: XPN as Synthesis Snapshot**
The XPN format is positioned as a synthesis snapshot — a moment in the continuous function of a
synthesis engine, designed to be the most useful moment. The Oxport philosophy (see Article 10):
the export tool should be transparent about what it is capturing and honest about what it cannot.

**5. What Producers Should Expect from a Synthesis-Derived Pack**
Practical section: synthesis-derived packs have a character signature that is consistent across the
pack. OPAL packs sound like OPAL. ONSET packs sound like ONSET. This consistency is a feature
(packs are cohesive) and a constraint (they are not general-purpose rompler libraries). How to
use packs that have a strong character identity: treat them as instrument packs, not sample banks.

---

## Article 6 — "The Aquarium as Studio: XO_OX's Mythological Approach to Sound Design"

**Hook:** Most synthesis companies name their products after features. XO_OX names its engines after
aquatic organisms that live at specific ocean depths, and it is not marketing — it is a working
methodology for sound design.

**Estimated word count:** 2,200

**Target SEO keyword:** `sound design mythology creative process synthesis`

**Target publish date:** September 2026

### Key Sections

**1. The Water Column Atlas**
Introduce the Water Column Atlas: 29 engines mapped across 9 ocean depth zones, from sunlit surface
to abyssal trenches. The zones are not arbitrary aesthetic groupings — they encode timbral logic.
Surface engines (OPENSKY, OBBLIGATO) are bright and airy. Mid-water engines (ONSET, OPAL) are
balanced. Deep-water engines (OUROBOROS, OCEANDEEP) are dense and dark. The atlas is a production
decision tool disguised as mythology.

**2. Why Mythology Works as Design Constraint**
The problem with purely technical naming: "Multi-oscillator Subtractive Synthesizer with Resonant
Filter" describes architecture, not character. Character is what producers remember and reach for.
Giving an engine an organism identity forces the design team to answer: what does this organism
*do*, and how does its behavior map to synthesis behavior? The axolotl regenerates (ODDOSCAR's
morphing architecture). The jellyfish (OVERLAP) has topological bell chambers. The metaphor
creates real design constraints.

**3. The Organism Identity Cards in Pack Design**
Each XO_OX expansion pack references the organism identity of its source engine. The OPAL pack
documentation references the sea slug (chromodoris). The OVERDUB pack references the cuttlefish.
How this organism context shapes pack design: the OPAL pack emphasizes textures that granular
synthesis specifically produces (the chromodoris's impossible pattern complexity). The identity
card is a creative brief that produces pack coherence.

**4. Sound Design by Character, Not by Category**
Commodity sound design: "pads," "leads," "bass," "fx" — genre categories that produce predictable
results. XO_OX pack categories are mood-based (Foundation, Atmosphere, Entangled, Prism, Flux,
Aether, Family) and character-based (organism behavior metaphors in preset names). The ONSET Kick
preset called "MACHINE GUN REEF" is more memorable and more distinctive than "Techno Kick 3."
Why distinctive preset names produce more creative use.

**5. The Producer's Use of the Mythology**
Practical: you do not need to know the mythology to use the packs. But if you know that OPAL is
the sea slug and its design philosophy is "impossible pattern complexity," you will reach for OPAL
differently than if you think of it as "a granular synth." The mythology gives producers a
decision-making shorthand. How to use the Aquarium atlas in XO_OX pack selection.

---

## Article 7 — "How to Build a 12-Month Pack Catalog" (Meta / Business)

**Hook:** Most independent sound designers release sporadic packs with no arc. Here is how XO_OX
planned a coherent catalog around engine character, audience development, and production cadence —
and what we learned from it.

**Estimated word count:** 1,800

**Target SEO keyword:** `how to make MPC expansion packs sell`

**Target publish date:** October 2026

### Key Sections

**1. The Problem with Sporadic Releases**
A catalog is not a collection of packs — it is a narrative. Sporadic releases (pack whenever ready,
whatever character, whatever instrument) do not build an audience. They build a catalog that looks
arbitrary. Producers who buy Pack 1 do not know why to buy Pack 2. This is the primary failure
mode of independent sound designers with talent but no publishing strategy.

**2. The Anchor Engine Strategy**
XO_OX's approach: every 3 months, release a pack for the anchor engine that most represents the
brand's character (currently ONSET for drums, OPAL for textures, OBLONG/OVERBITE for character
synths). Anchor packs create a home base for new audience members. Mid-cycle releases (specialty
packs, coupling packs, MPCe-native packs) expand the catalog for existing subscribers without
requiring new audiences to enter the catalog mid-arc.

**3. The Coupling Pack as Catalog Glue**
A coupling pack — a pack designed to layer with a previously released pack — is both a production
win and a retention win. Producers who bought the OPAL texture pack have a reason to buy the
OVERDUB coupling pack (designed to complement OPAL's register). The catalog structure creates
portfolio completion incentives without artificial scarcity. Explain the OPAL→DUB coupling pathway
and how it was designed into pack release order.

**4. Pricing Architecture Across a Catalog**
Introductory pack (free or loss-leader), standard pack (core product), specialist pack (premium,
smaller audience, higher price). Bundle architecture: any three packs at 30% off, any engine's
complete set at 40% off. How to sequence releases to feed each tier. This section is based on
XO_OX's actual planned pricing architecture — transparent about the business logic.

**5. Content Ecosystem Around a Release**
A pack release without content around it is a product launch without a story. The XO_OX release
cycle: Field Guide article (the intellectual case), Patreon source XPM (the behind-the-scenes),
audio demos (the sonic case), Aquarium entry for new engines (the lore). How four content pieces
around one pack release compound over a 12-month catalog.

---

## Article 8 — "The Kit That Grooves: What Makes Drum Samples Feel Human"

**Hook:** You can have technically perfect drum samples and a kit that refuses to feel alive. Groove
is not in the samples — it is in the architecture around them.

**Estimated word count:** 2,000

**Target SEO keyword:** `drum samples feel human groove MPC kit`

**Target publish date:** November 2026

### Key Sections

**1. The "Dead Kit" Problem**
Define: a kit where every hit sounds technically correct but the groove does not breathe. The
mechanical hit: consistent timing, consistent velocity, consistent timbre. Why this is a kit
architecture problem, not a sample problem. Producers often blame the samples when the issue is
how the kit is configured.

**2. Velocity Inconsistency as Groove Engine**
Acoustic drummers do not hit consistently. The variance in velocity between hits of the same drum
is where groove lives. MPC pads have velocity sensitivity — the kit must be designed to respond
to that sensitivity in a way that is musically meaningful (Article 1 callback). How XO_OX velocity
layer design preserves micro-variation: the 4–6 layer structure means even 10-velocity-unit
differences produce timbral variation, not just amplitude variation.

**3. Round-Robin as Performance Memory**
Round-robin sample cycling prevents the "machine gun effect" (same sample repeating). But XO_OX
uses round-robin for more than repetition prevention — the cycle is designed so that consecutive
hits form a micro-narrative. Hit 1 is the feliX voice (bright, present). Hit 2 is the Oscar voice
(warm, textured). Hit 3 is the feliX voice again, slightly different. The round-robin is a breathing
pattern, not a randomizer.

**4. The Choke Group as Rhythmic Gating**
Revisit choke groups (more thoroughly than Article 2's overview). HiHat choke is the canonical example,
but choke can be used for rhythmic gating on any pad: a shaker that silences an open texture, a
clap that silences a room reverb. How XO_OX uses choke groups as musical architecture, not just
bleed prevention.

**5. The Physical Default ADSR and Its Role in Groove**
Groove is largely in the attack-to-sustain relationship. A kick with a 3ms attack and 400ms decay
grooves differently than a kick with the same samples but a 20ms attack and 800ms decay. How XO_OX
ONSET-derived ADSR defaults were set to match the synthesis behavior and produce natural groove
at default settings. The argument against neutral/flat defaults.

---

## Article 9 — "Choke Groups, Mute Groups, and the Hidden Architecture of MPC Kits"

**Hook:** Most producers use choke groups for one thing: silencing open hi-hats. This is a small
fraction of what MPC choke architecture can do — and understanding the full system changes how you
design and use kits.

**Estimated word count:** 1,600

**Target SEO keyword:** `MPC choke groups mute groups drum kit`

**Target publish date:** December 2026

### Key Sections

**1. Choke vs. Mute — The Distinction Matters**
Define both clearly. Choke: pad A silences pad B when triggered. Mute: pad A and pad B cannot play
simultaneously (bidirectional). The MPC implements these differently; confusing them produces kits
that behave unpredictably. Most pack documentation is silent on which is used and when.

**2. The XPN Architecture for Choke/Mute**
How choke groups are encoded in XPN files. The `ChokGroup` and `MuteGroup` fields. Default values
and what "unassigned" behavior looks like. The XO_OX convention: choke groups for acoustic bleed
relationships (hat choke), mute groups for timbral variants (when you want only one character of
a sound at a time).

**3. Choke Groups Beyond HiHats**
Three non-standard uses XO_OX implements in its packs:
- Open texture choke: a long-decay ambient pad on Pad 13 is choked by a transient hit on Pad 9.
  The hit "punches through" the atmosphere.
- Layered synth voice choke: two melodic variants of a synth voice share a choke group —
  pressing either silences the other, creating a monophonic lead feel on pads.
- FX send choke: a reverb tail is choked by a dry re-trigger, enabling stutter performance.

**4. Performance Implications — Designing for Live Use**
A kit designed for live pad performance has different choke architecture than a kit designed for
sequencing. Live pads benefit from aggressive choke (fewer overlapping decays = more immediate
response). Sequenced kits benefit from more generous decay (the sequence provides the timing
structure; overlap is musical). How XO_OX ships two choke configurations in flagship ONSET packs:
LIVE and SEQUENCE modes.

**5. Troubleshooting Choke Problems**
Common mistakes in expansion pack choke design: wrong group assignments (pad silences itself),
missing choke (open hat rings forever), mute groups on pads that should be independent. A producer's
checklist for auditing a new kit's choke behavior before using it in a session.

---

## Article 10 — "Open Source Pack Tools: The Oxport Philosophy"

**Hook:** The XO_OX pack export pipeline is open source, documented, and designed so that other
developers can build on it. Here is why — and what the tools actually do.

**Estimated word count:** 1,800

**Target SEO keyword:** `MPC expansion pack creation tools open source`

**Target publish date:** January 2027

### Key Sections

**1. What Oxport Is**
The XPN tool suite in `Tools/` is the complete pipeline that takes XOmnibus synthesis engine output
and produces shipping-quality MPC expansion packs. 8 tools: drum export, keygroup export, kit
expander, bundle builder, cover art, packager, sample categorizer, render spec. Open source,
Python-based, documented.

**2. The Philosophy Behind Open Sourcing Pack Tools**
Most commercial expansion pack studios treat their production pipeline as proprietary. XO_OX inverts
this: the value is in the design judgment (the ONSET render decisions, the velocity layer curves,
the feliX-Oscar mapping), not in the toolchain that executes those decisions. Open-sourcing the
tools creates community (other developers can build for MPCe), creates trust (producers can audit
how packs are made), and creates infrastructure (the XPN format becomes better-supported).

**3. The 5 Kit Modes**
The kit expander supports 5 playback modes: velocity, cycle, random, random-norepeat, smart.
Explain each briefly. Smart mode (Sonic DNA velocity mapping + musical velocity curve from Vibe
sound designer persona) is the XO_OX default for all flagship packs. Describe what "musical
velocity curve" means in practice: it is not a linear mapping — it preserves expressivity in the
mid-range (most playing velocity), uses the extremes for true pianissimo and fortissimo, and
applies the feliX-Oscar direction (soft → Oscar, hard → feliX) across the entire curve.

**4. The Invisible Intelligence Principle**
Derived from the Synth Seance demands on the XPN tool suite: the best pack tools produce behavior
that sounds natural without the producer knowing why. Round-robin sequencing that forms micro-
narratives (Article 8). Velocity curves that feel right before you tune them. Choke defaults that
prevent common mistakes automatically. "Invisible intelligence" is the design goal for every
default value in the Oxport pipeline.

**5. What Community Developers Can Build With It**
Concrete examples: an independent sound designer can use Oxport to convert a hardware synth
recording session into an MPCe-native pack with correct choke groups, Sonic DNA metadata, and
proper velocity curves. The tools are designed to bring synthesis-informed pack design within
reach of producers who do not work in JUCE or audio DSP. The XO_OX open-source tools as
infrastructure for a higher-quality MPC expansion pack ecosystem.

**6. The Future of the Pipeline**
V2 priorities: adaptive intelligence (tools that learn from producer feedback which velocity curves
work for which genres), automatic MPCe quad-corner assignment, and integration with XOmnibus's
AUv3 engine for real-time render-to-XPN (synthesis snapshot on demand). How community contributions
to Oxport accelerate the V2 roadmap.

---

## Editorial Calendar Summary

| Article | Publish Month | Primary Tie-In |
|---------|--------------|---------------|
| 1. Velocity Layers | April 2026 | Doctrine D001, Prism Sweep findings |
| 2. ONSET Drum Kits | May 2026 | ONSET engine, XPN drum export pipeline |
| 3. MPCe Native Design | June 2026 | MPCe hardware, MPCE_SETUP.md documentation |
| 4. feliX-Oscar Polarity | July 2026 | Brand philosophy, Water Column Atlas |
| 5. Pack vs. Synthesis | August 2026 | Oxport philosophy, XPN format |
| 6. Aquarium as Studio | September 2026 | Aquatic mythology, organism identity cards |
| 7. 12-Month Catalog | October 2026 | Pack strategy, Patreon integration |
| 8. Kit That Grooves | November 2026 | ONSET kits, round-robin design |
| 9. Choke Groups | December 2026 | XPN format details, ONSET LIVE/SEQUENCE modes |
| 10. Oxport Philosophy | January 2027 | Open source tools, community ecosystem |

---

## Total Estimated Word Count

| Article | Words |
|---------|-------|
| 1 — Velocity Layers | 2,000 |
| 2 — ONSET Drum Kits | 2,200 |
| 3 — MPCe Native Design | 1,800 |
| 4 — feliX-Oscar Polarity | 2,500 |
| 5 — Pack vs. Synthesis | 2,000 |
| 6 — Aquarium as Studio | 2,200 |
| 7 — 12-Month Catalog | 1,800 |
| 8 — Kit That Grooves | 2,000 |
| 9 — Choke Groups | 1,600 |
| 10 — Oxport Philosophy | 1,800 |
| **TOTAL** | **19,900** |

Adding these 10 articles brings the Field Guide to approximately **67,600 words** across 24 posts.

---

## Production Dependencies

- **Article 1**: Prism Sweep velocity findings (exists); ONSET velocity layer documentation (exists); audio demo clips preferred but not required at launch.
- **Article 2**: ONSET engine spec (exists); XPN drum export pipeline documentation (exists); per-voice physical defaults table from Seance data.
- **Article 3**: `MPCE_SETUP.md` convention needs to be formally documented if not already; MPCe NW/NE/SW/SE zone architecture research (see `mpc_evolving_capabilities_research.md`).
- **Article 4**: feliX-Oscar axis mapping for all 34 engines — infographic asset highly recommended. Mood family mapping (exists in master spec).
- **Article 5**: Oxport source XPM philosophy documentation; XPN format article (Post 13, published) for cross-reference.
- **Article 6**: Water Column Atlas (published as Post 10); organism identity cards (11 engines still need cards per MEMORY.md — priority for pre-publication).
- **Article 7**: Pack pricing architecture needs to be finalized before publication (do not publish placeholder prices). Patreon integration documentation.
- **Article 8**: ONSET round-robin design documentation; Vibe musical velocity curve specification (from Synth Seance XPN demands).
- **Article 9**: XPN choke/mute group specification (exists in XPN format tools); ONSET LIVE/SEQUENCE mode documentation.
- **Article 10**: Oxport V2 roadmap (see `oxport_v2_feature_backlog.md`); community contribution guidelines (to be written pre-publication).

---

## Cross-Reference: Existing Specs These Articles Draw From

- `velocity_science_rnd.md` — Article 1
- `pack_design_onset_drums.md` — Article 2
- `mpce_native_pack_design.md` — Article 3
- `engine_sound_design_philosophy_rnd.md` — Articles 4, 5, 6
- `xpn_narrative_format_rnd.md` — Articles 6, 8
- `pack_economics_strategy_rnd.md` — Article 7
- `xpn_round_robin_strategies_rnd.md` — Article 8
- `xpn_choke_mute_groups_rnd.md` — Article 9
- `oxport_v2_architecture_rnd.md`, `oxport_innovation_round5.md` — Article 10
