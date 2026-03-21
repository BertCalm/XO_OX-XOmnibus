# XOmnibus V1 — Launch Day Content

**Date drafted:** 2026-03-21
**Product state:** 46 engines | ~22,000 presets | macOS AU + Standalone | MIT license | Free

---

## 1. Twitter/X Launch Thread (7 tweets)

---

**(1/7)**

XOmnibus V1 is out.

46 synthesis engines. ~22,000 presets. Cross-engine coupling in real time.

Free. Open-source. MIT license. Always.

[DOWNLOAD LINK]

---

**(2/7)**

The engines are not presets of each other. Each is a distinct DSP architecture with its own physics:

OPERA uses Kuramoto synchronization — 32 partials that couple like fireflies finding phase lock, driven by an autonomous Conductor arc.

OWARE models mallet contact time from Chaigne 1997 — the same equations that describe how a marimba hammer compresses against wood.

ORGANISM runs cellular automata. The rule set is the synthesis.

---

**(3/7)**

The coupling system is the instrument.

14 coupling types. Any engine can drive any other — filter cutoff, pitch, amplitude, LFO rate, FM depth. Load ONSET (8-voice percussion) in slot 1, wire its transient envelope to OPERA's phase coherence parameter in slot 2.

That's a sound that doesn't exist in any single synth.

KnotTopology routes signal bidirectionally — two engines listening to each other simultaneously.

---

**(4/7)**

OBRIX is the flagship.

It's modular by synthesis, not by configuration. You stack bricks — oscillator sources, shapers, FM paths — and they interact. Route a noise source through a low-pass brick into an FM brick that's cross-modulating the first brick, and the instability that emerges is the sound.

81 parameters. 337 factory presets. The Reef Residency system lets coupling signal enter the brick ecology as a third organism — Competitor, Symbiote, or Parasite mode.

---

**(5/7)**

The Seance process:

Every engine was reviewed by a council of ghost engineers — Moog, Buchla, Schulze, Vangelis, Kakehashi, Pearlman, Smith, Tomita — as a structured quality gate.

37 Blessings were awarded for genuinely novel design decisions. B028 (Continued Fraction Convergent Synthesis in OVERTONE) and B035 (OperaConductor autonomous dramatic arc) are two I'm still thinking about.

Fleet average seance score: 8.8/10. Five engines at 9.0+.

---

**(6/7)**

Why free?

The kid who grew up making music in Fruity Loops demos — who couldn't save, who had to finish songs in one sitting — is building the free instrument so the next generation can use a top-of-the-line plugin.

No barriers. No paywalls. Better than what money can buy.

Premium content (Guru Bin packs, Patreon) funds development. The engine is MIT-licensed forever.

---

**(7/7)**

One person. One year. 46 engines.

The aquarium is open.

[DOWNLOAD LINK]
xo-ox.org

---

## 2. Single Tweet Version (280 char max)

XOmnibus V1: 46 synthesis engines (Kuramoto vocal, mallet physics, cellular automata, continued fractions, Berlyne curiosity). Cross-engine coupling. ~22k presets. Free. MIT. macOS AU + Standalone. [DOWNLOAD LINK]

*(243 characters)*

---

## 3. Instagram Caption

46 synthesis engines in one plugin. Free. Open-source. Here.

OPERA runs Kuramoto phase synchronization — 32 partials that lock together or fragment apart, shaped by an autonomous Conductor arc that writes its own dramatic arc through the sound. OWARE models a balafon mallet compressing against wood, mathematically, from a 1997 acoustics paper. OBRIX stacks synthesis bricks into ecologies where each source modulates the others. ORGANISM runs Conway's Game of Life on your signal. These aren't variations on a theme. Each engine is a separate physics.

The coupling system connects them in real time. Load four engines. Wire the transient envelope of a percussion synth to the phase coherence of a vocal synth. Wire a granular cloud's density to the feedback depth of an entangled reverb. The space between the engines is where the music lives. ~22,000 factory presets. MIT license. Always free. Download at link in bio.

#synthesizer #sounddesign #vst #plugin #openSource #freePlugin #audioPlugin #synthDIY #experimentalMusic #JUCE #macOS #electronicmusic #modularThinking #DSP #musicproduction #XOmnibus #XOOX

---

## 4. Signal Feed Inaugural Post

**The Aquarium Is Open**

*March 23, 2026*

---

I shipped XOmnibus V1 today.

I've been sitting with that sentence for a few hours trying to figure out what the right thing to say is. The honest answer is: I don't know exactly what this is yet. I know what it contains — 46 synthesis engines, ~22,000 presets, a cross-engine coupling system, 14 coupling types, a shared DSP library — but the thing about building something like this for a year is that you lose the ability to see it from the outside.

So let me just tell you what happened, and what comes next.

The original idea was simpler. I wanted to build a synthesizer that didn't cost anything and didn't apologize for being free. Most free synths are either stripped-down demos of paid versions, or passion projects that plateau at "good enough." I wanted something that could compete with Omnisphere on sound design depth and beat it on one axis: you can see every line of code.

Coupling became the organizing principle by accident. I had three engines built — the granular one, the FM one, the physical model — and I kept wanting to hear what happened when they talked to each other. The answer was: something that neither of them could produce alone. So I built the routing system, then I built more engines to justify it, and somewhere in month nine I looked up and there were 40 engines and 15,000 presets and I had to stop and ask if I was building a plugin or a world.

The aquatic mythology helped. Every engine has an identity — ORACLE is a prophet fish that generates stochastic melody from ancient maqam scales, OUROBOROS is a feedback creature that eats its own tail, OPERA is a choral organism whose 32 partials synchronize like fireflies. When the engines have characters, the coupling between them feels like drama instead of routing. That's not a metaphor I planned. It just happened.

What comes next:

**Week 1**: ORBWEAVE preset expansion, 5+ audio demos live on the site, Discord server open.
**Month 1**: Engine quality sprint — six engines need DSP passes to reach 8.0+ seance scores.
**Month 2**: Guru Bin Vol 1 — Transcendental preset packs, PDF ritual guides, audio walkthroughs. This is how development gets funded.
**Month 3+**: OFFERING (psychology-driven drum synthesis, Berlyne curiosity as the oscillator), coupling sequencer, AUv3.

Everything in the core is free. That doesn't change.

I grew up making music in Fruity Loops demos. The version that couldn't save. You had to finish the song in one sitting or lose it. I'm not building this for the person who already has Omnisphere. I'm building it for the kid in the demo version, and I want it to be better than what money can buy.

The aquarium is open. Come in.

— Joshua
xo-ox.org

---

## 5. Reddit Post (r/synthesizers)

**Title:** I spent a year building a free, open-source multi-engine synth — 46 engines, ~22k presets, cross-engine coupling. Here's the thing I kept getting wrong.

---

Hi r/synthesizers. I shipped XOmnibus V1 today. Free, MIT-licensed, macOS AU + Standalone. Download is at xo-ox.org. But that's not the interesting part.

The thing I kept getting wrong was assuming more engines meant more variety. It doesn't, by itself. You can have 46 engines that each sound like a slightly different subtractive synth with different filter characteristics. That's not what this is, but it's what it could have been.

The design discipline I had to learn was: every engine needs to be unreproducible from the others. Not "different flavor." Genuinely unreachable from any other synthesis paradigm in the plugin.

A few that I think actually get there:

**OPERA**: Kuramoto synchronicity synthesis. 32 partials that are treated as coupled oscillators — they pull toward phase synchronization or fragment into chaos depending on a coupling strength parameter. There's an autonomous Conductor arc that shapes the drama of each note over time. The stereo field is a readout of the order parameter R: when the partials are locked, they spread wide; when they fragment, they collapse to center.

**OVERTONE**: Timbre derived from continued fraction convergents of irrational numbers. The harmonic relationships in a note are the rational approximations to π, e, φ, or √2 — the same mathematics a nautilus shell uses, but applied spectrally.

**ORGANISM**: Cellular automata running on your signal. Rule set is the synthesis parameter.

**OWARE**: Mallet contact-time physics from the Chaigne 1997 model. The material continuum (wood↔metal↔bell↔bowl) crossfades through genuinely different physical equations, not just EQ curves. Per-mode sympathetic resonance networks with 5 strings per mode, 40 unique resonance profiles total.

The **coupling system** is what makes the engine count matter. Any engine can modulate any other — filter, pitch, amplitude, FM depth, LFO rate. Fourteen coupling types including KnotTopology, which routes bidirectionally: two engines listening to each other simultaneously.

Load ONSET (8-voice percussion with cross-voice coupling) in slot 1 and wire its transient envelope to OPERA's phase coherence in slot 2. That specific combination does not exist anywhere else.

It's one person, MIT license, always free. Happy to answer questions about specific DSP decisions, the coupling architecture, or why the engines have aquatic creature identities (it helps more than I expected).

[DOWNLOAD LINK] | GitHub: [REPO LINK]

---

## 6. Forum Post (Gearspace)

**Title:** XOmnibus V1 — cross-engine coupling architecture and physics citations, if you want them

**Subforum:** Instruments & Plug-Ins / Soft Synths, Samplers, ROMplers & Plugin Instruments

---

I shipped XOmnibus V1 today. Free, open-source (MIT), macOS AU + Standalone. I'll put the download link at the bottom, but I want to lead with the technical specifics because Gearspace readers are the ones who'll actually care about them.

**The coupling system**

The MegaCouplingMatrix supports 14 coupling types. The routing is any-to-any across the four engine slots, and the coupling input becomes a first-class synthesis parameter in the receiving engine — not just a modulation wheel substitute.

Coupling types include:

- **Standard** (amplitude → target parameter, configurable amount + polarity)
- **Frequency** (pitch output of source drives pitch offset in target — for real-time harmonic stacking)
- **Envelope Follow** (RMS envelope of source drives target parameter)
- **KnotTopology** (bidirectional — source and target exchange signal simultaneously; implemented in ORBWEAVE)
- **XVC Cross-Voice Coupling** (in ONSET: voice 1's transient envelope drives voice 3's LFO rate — within-engine voice topology; Blessing B002 "3-5 years ahead" per the seance council)
- **Phase Sync** (OPERA-specific: Kuramoto coupling coefficient exposed as a coupling input, so an external engine can drive phase coherence directly)

**Physics-based engines with citations**

I take "physical modeling" seriously as a claim, so here are the actual models:

*OWARE (tuned percussion)*: Mallet contact-time synthesis per Chaigne & Askenfelt, "Numerical simulations of struck strings," JASA 1994. Material continuum (wood → metal → bell → crystal bowl) is not a filter-slope crossfade — each mode of the material continuum uses genuinely different stiffness and damping coefficients. Sympathetic resonance network: 5 strings per mode × 8 modes = 40 resonance profiles, each with unique ratios per mode.

*OSTINATO (12-instrument percussion)*: Modal membrane synthesis with Bessel function zeros sourced from published physics tables. Membrane mode shapes (01, 11, 21, 02) are correct. The citation is Kinsler et al., "Fundamentals of Acoustics."

*OPERA (vocal synthesis)*: Kuramoto model (Y. Kuramoto, 1975) applied to additive synthesis partials. Peterson & Barney (1952) formant data for 10 vowels from their original study. The Conductor arc is autonomous — it generates a 4-shape dramatic arc (rise, peak, fall, decay) with configurable arc time and ±5% jitter, and yields to live player input via `max(conductorK, manualK)`. This is the first engine in the fleet where the machine has narrative intent.

*ORGANON (metabolic synthesis)*: Variational Free Energy from Karl Friston's active inference framework, implemented as a synthesis metabolism. The seance council flagged this as "publishable as a paper" — I'll take their word for it.

*ORACLE (stochastic + maqam)*: GENDY stochastic synthesis (Xenakis, 1992) with maqam scale quantization. Not "microtonal mode selection" — the stochastic segment generator runs, then the output is pulled toward the nearest scale degree in the selected maqam.

*ORGANISM (cellular automata)*: Wolfram Rule 30/Rule 110/Conways Game of Life applied to a binary state grid that gates and modulates the signal. The rule set evolves in real time; the synthesis changes with the automaton state.

**The seance quality system**

Every engine was evaluated against 6 Doctrines (velocity must shape timbre, modulation is the lifeblood, the physics IS the synthesis, dead parameters are broken promises, an engine that cannot breathe is a photograph, expression input is not optional) and scored by a "ghost council" of historical synthesist perspectives. 37 Blessings were awarded for genuinely novel design decisions. Fleet average post-fixes: 8.8/10. Five engines at 9.0+.

I'm not claiming this is perfect. The seance process found real bugs — OCELOT's biome crossfade was dead (setBiomeTarget() was never called), ORPHICA's buffer was too short, several LFO rate floors were above the 0.01 Hz doctrine requirement. All of them are fixed in V1.

**What it is**

46 engines. ~22,000 presets. 6D Sonic DNA fingerprinting. Four macros per preset (CHARACTER, MOVEMENT, COUPLING, SPACE). Gallery Model UI — warm white shell, engine-specific accent colors. MIT license.

Free. That part is not changing.

Download: [DOWNLOAD LINK]
GitHub: [REPO LINK]
Site / Field Guide: xo-ox.org

Happy to answer specific technical questions about any engine's DSP, the coupling routing implementation, or the preset system.

---
