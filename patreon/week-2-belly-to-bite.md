# FOUNDER'S FREQUENCY -- Week 2
## Belly to Bite
**Engine(s)**: OVERBITE | **Mood**: Foundation | **Coupling**: None

---

### The Story

Keiko is a bassist and producer in the Guild who works across everything -- R&B, neo-soul, film scoring, lo-fi. She told me she keeps a folder called "the one bass" on every project. It's whatever bass patch she reaches for first, every time, before she even thinks about sound design. She wanted "the one bass" to be an XO_OX preset.

That's a terrifying commission. A preset that works in every genre. A bass that's warm enough for neo-soul, aggressive enough for trap, weird enough for experimental, and pure enough for film. Most synths solve this by being generic -- by being nothing in particular. OVERBITE solves it by being everything at once, with five macros that let you travel between extremes in real time.

Belly to Bite is the result. It's the most versatile preset in the entire Founder's Signature collection, and it's the only single-engine preset in the pack. I wanted to show what one engine can do when every parameter is dialed in with purpose.

This preset is also historically significant: during the Seance sessions, all eight ghosts blessed OVERBITE's five-macro system (Blessing B008). That's never happened for any other architecture in the fleet. Moog, Buchla, Smith, Kakehashi, Vangelis, Schulze, Pearlman, Tomita -- all eight said yes. The five-macro axis (BELLY/BITE/SCURRY/TRASH/PLAY DEAD) represents something genuinely new in how a bass synth can be controlled.

### The Science

OVERBITE's signal chain is built like a predator's anatomy. Let me walk you through it piece by piece.

**Oscillator A -- The Belly**

BiteOscA is the Belly oscillator. It offers four waveforms, each designed for low-end weight:

- **Sine** (wave 0): A 2048-sample table lookup with linear interpolation. Pure sub. No harmonics. This is the foundational weight -- the same synthesis method used in TR-808 sub oscillators, but with higher-precision interpolation. At C1 (32.7 Hz), this produces a sine wave that sits below the hearing threshold for harmonic content. You feel it more than hear it.

- **Triangle** (wave 1): Soft harmonic body. The odd harmonics (3rd, 5th, 7th) roll off at 1/n^2 -- much gentler than a square wave's 1/n rolloff. This gives warmth without brightness.

- **Saw** (wave 2): Rich but warm, with a gentle high-frequency rolloff applied before the filter. PolyBLEP anti-aliasing prevents the digital artifacts that plague naive sawtooth implementations.

- **Cushion Pulse** (wave 3): Variable-width pulse wave. The "Shape" parameter controls the pulse width -- at 50% it's a square wave, at 10% or 90% it thins into a nasal buzz. PolyBLEP again for clean anti-aliasing.

All four waveforms include a drift LFO -- a low-pass filtered noise source that adds sub-cent pitch instability. This is what makes digital oscillators feel analog. The drift rate is slow enough (around 0.5 Hz) that you perceive warmth, not detuning.

**The Character Chain -- Five Stages of Transformation**

After the oscillators and filter, the signal passes through OVERBITE's character chain. This is where the anglerfish bites.

1. **Fur** (BiteFur): Pre-filter soft saturation using a tanh waveshaper. `tanh(x * drive)` is one of the fundamental saturation curves in analog modeling -- it compresses peaks smoothly, adding even harmonics (2nd, 4th, 6th) that warm the sound. At low amounts, Fur is invisible but felt. The bass just sounds... rounder. That's the even harmonics filling the space between the fundamental and the first odd harmonic. Real tube amplifiers exhibit this same transfer function. Fur models that warmth without the noise floor.

2. **Chew** (BiteChew): Post-filter contour shaping. A gentle dynamics processor that controls the relationship between loud and soft parts of the waveform. Think of it as a micro-compressor operating at waveform level rather than amplitude level.

3. **Gnash** (BiteGnash): Asymmetric waveshaping. Unlike Fur's symmetric tanh, Gnash applies different curves to the positive and negative halves of the waveform. This creates odd AND even harmonics simultaneously, producing a growl that cuts through a mix without adding high-frequency harshness. Wavefolding territory -- the signal is literally folded back on itself when it exceeds a threshold.

4. **Trash** (BiteTrash): Three dirt modes for when you want to destroy the signal intentionally. Mode 0 is Rust (bitcrushing), Mode 1 is Clip (hard limiting), Mode 2 is Splatter (wavefolding). Splatter is the most musical -- it wraps the signal around a fold point, creating dense harmonic clusters that still track pitch.

5. **Drive**: Final-stage saturation at the output. The last chance to add heat before the signal leaves the engine.

**The Five Macros -- The Real Innovation**

Most synths give you four macros that control arbitrary parameters. OVERBITE's five macros are character axes -- each one tells a story.

- **BELLY** (macro_belly): Moves the entire engine toward warmth. Increases Fur saturation, opens the filter gently, boosts the sub oscillator level, reduces Gnash and Trash. The anglerfish's lure: soft, glowing, inviting.

- **BITE** (macro_bite): Moves toward aggression. Increases Gnash depth, pushes filter resonance, adds drive. The jaw: sudden, cutting, feral. BELLY and BITE aren't opposites -- you can have both high simultaneously for a sound that's warm AND aggressive. That's the anglerfish: plush body, razor teeth.

- **SCURRY** (macro_scurry): Speed and nervousness. Increases LFO rate, adds vibrato, tightens envelope times. The prey running. At maximum, the bass twitches and stutters like something alive and panicking.

- **TRASH** (macro_trash): Destruction axis. Pushes the Trash stage harder, adds filter resonance, reduces filter cutoff (creating a trapped, pressurized sound). At maximum, the bass is barely recognizable -- a beautiful wreck.

- **PLAY DEAD** (macro_play_dead): The most unusual macro. It reduces velocity sensitivity, lengthens attack, flattens dynamics. The bass becomes still, sustained, nearly lifeless -- but still present. A drone. The anglerfish floating motionless in the deep, waiting. This is the macro that made the Seance ghosts pay attention.

### How to Use It

**1. Neo-Soul Foundation (BELLY at 70%, everything else at 0)**
Play a simple two-note pattern (root and fifth) below middle C. The Belly macro gives you a warm, round sub with just enough Fur saturation to cut through speakers that can't reproduce true sub frequencies. The drift oscillator adds natural movement. This is "the one bass" for warm genres.

**2. Trap / Hip-Hop Aggression (BITE at 55%, BELLY at 30%)**
The combination of moderate BITE with gentle BELLY gives you a bass that has weight AND teeth. The Gnash wavefolder adds upper harmonics that translate on phone speakers (critical for streaming), while the Belly sub retains physical impact on monitors and systems.

**3. Experimental Bass (TRASH at 70%, SCURRY at 40%)**
This is where OVERBITE becomes something no other bass synth can be. The wavefolding creates dense harmonic clusters that shift as the Scurry macro adds LFO movement. Play sustained notes and slowly increase SCURRY -- the bass starts breathing, twitching, evolving. Modular synthesis territory from a preset.

**4. Film Score Drone (PLAY DEAD at 80%, BELLY at 50%)**
Hold a low note and let it sustain. The Play Dead macro flattens all dynamics into a single, unbroken tone. The Belly warmth fills the register without drawing attention. This is the kind of bass that sits under dialogue in a film scene -- you don't consciously hear it, but you'd notice if it disappeared. Remove it and the scene feels empty. That's PLAY DEAD.

**5. Performance -- Ride the Macros**
Map BELLY and BITE to your mod wheel and expression pedal. Play a bass line and sweep between warm and aggressive in real time. This is how Keiko uses the preset in live sets -- the same bass line transforms from neo-soul to trap and back without changing a single note.

### Macro Guide

| Macro | What It Does | Sweet Spot |
|-------|-------------|-----------|
| M1 (CHARACTER) | Maps to BELLY. Warmth axis -- Fur saturation, sub level, filter opening. | 0.50 -- balanced warmth. Above 0.7, the sub dominates. |
| M2 (MOVEMENT) | Maps to SCURRY. Speed and nervousness -- LFO rate, vibrato depth, envelope speed. | 0.20 -- gentle life. Above 0.5, things get twitchy. |
| M3 (COUPLING) | Maps to BITE. Aggression axis -- Gnash depth, resonance, drive. (No coupling in this preset, so M3 repurposed.) | 0.35 -- adds harmonic presence without harshness. |
| M4 (SPACE) | Maps to PLAY DEAD. Stillness axis -- dynamics flatten, attack lengthens, the bass goes dormant. | 0.0 (default) -- only engage when you need the drone effect. |

### Pair It With

- **"Coupled Bounce"** (Week 1 preset) -- Use Belly to Bite for the melodic bass line, Coupled Bounce for the rhythmic foundation. The two OVERBITE instances use different macro positions, giving you two characters from the same engine -- warm melodic bass above, coupled trap bounce below.

- **ONSET drum kits** -- Any ONSET Foundation preset pairs naturally with Belly to Bite. The ONSET kick occupies the transient space, Belly to Bite sustains the sub. No frequency fighting because ONSET's BridgedT decays before OVERBITE's sustain phase fills in.

- **OBLONG atmospheric textures** -- OBLONG's DustTape warmth and CuriosityEngine movement create a bed that Belly to Bite sits in perfectly. The two engines share a design philosophy (warmth + character) but occupy different frequency ranges.

### The Download

This preset is included in the **Founder's Signature Vol. 1** XPN pack -- 5 presets, free, for XOmnibus and MPC.
[Download link placeholder]

---
*Next week: Coral Investigations -- an OBLONG preset where the LFO has a mind of its own.*
*Founder's Frequency is a free weekly series from XO_OX Designs.*
