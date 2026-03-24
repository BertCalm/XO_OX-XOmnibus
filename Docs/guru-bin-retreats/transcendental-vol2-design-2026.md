# Guru Bin Transcendental — Vol 2 Design Document
*Internal Planning Brief + Creative Direction*
*Drafted: 2026-03-21*

---

## Overview

Guru Bin Transcendental Vol 2 covers the six original standalone instruments that were complete before XOlokun existed. These are the engines that built the XO_OX vocabulary from the ground up — the instruments that proved the design philosophy was real before a fleet gave it context.

They are not the newest engines. They are the original ones.

Vol 2 ships **Week 15** on Patreon as a premium Transcendental tier release. The package includes:
- **90–120 Transcendental presets** (15–20 per engine)
- **PDF Booklet** — sound design philosophy, parameter cartography, playing strategies for each engine
- **Scripture Chapter VII: The Homecoming Verses** — new canonical scripture specific to the Vol 2 engines as a returning cohort
- **XPN Pack** — presets rendered as MPC expansion programs (pending manual WAV render session)
- **Tier:** Transcendental (premium, not free — distinct from Awakening Tier)

### Current Preset State (as of 2026-03-21)

| Engine | Existing Presets | Target Transcendental Additions |
|--------|-----------------|--------------------------------|
| OBLONG | 84 | 15–18 |
| OVERBITE | 77 | 15–18 |
| OVERDUB | 69 | 15–18 |
| ODYSSEY | 149 | 15–20 |
| ONSET | 162 | 15–20 |
| OPAL | 85 | 15–18 |

ONSET and ODYSSEY have the deepest existing libraries. Their Transcendental presets must go further than the factory library's best — these are the flagship sounds of the Vol 2 chapter, not incremental additions.

---

## Per-Engine Design Briefs

---

### Engine 1: OBLONG
*Bob the clownfish | Amber `#E9A84A` | Prefix: `bob_`*

OBLONG is the fleet's primary analog-voiced subtractive engine — the instrument every producer already knows how to use on arrival. Its identity is not novelty. It is fluency. Where newer engines require initiation, OBLONG requires only trust. Bob the clownfish navigates the anemone without burning because he has always lived there. OBLONG offers the same confidence: a warm oscillator stack, a filter that responds like good hardware, and an FM tinge that rewards restraint. It is the instrument you reach for when you need the sound to be unquestionable.

The factory library demonstrates OBLONG's range — warm saws, FM-colored leads, stacked chords, utility basses. What it underexplores is OBLONG at its most expressive: the interaction between sync drive and filter sweep, the way `bob_fmRatio` at non-integer values creates inharmonic tension that the filter can reveal incrementally, the Stack mode's capacity for evolving chord textures when LFO rates are offset across voices. Transcendental presets should demonstrate that OBLONG is not merely competent. At its best, it has character no preset library has fully captured.

**Named Preset Concepts:**

1. **Amber Tide** — Stack mode with three slightly detuned saws, filter opened slowly by mod wheel, FM tinge at 0.3. A pad that builds trust by not rushing.
2. **Clownfish Bite** — Hard sync lead with `bob_syncDrive` at 0.85, filter envelope tight and fast, a sound that has nothing to hide.
3. **Anemone Memory** — Legato performance patch with long portamento, low FM ratio creating subtle beating, filter resonance just below self-oscillation. Plays one note like it means it.
4. **Bob Dissolved** — Stack mode with high voice count, near-zero FM, filter almost closed, reverb long. The least OBLONG-sounding OBLONG preset — a test of the engine's quietest identity.
5. **Harmonic Incident** — FM ratio at a non-integer value (2.37), filter cutoff tracking the non-integer beating, velocity mapped to FM depth. Each note reveals a slightly different inharmonic event.

**Relevant Scripture:**
- Truth VI-3 (The Default Trap) — every OBLONG preset must interrogate why any parameter is at its factory default
- Truth VI-2 (The Mod Wheel Contract) — OBLONG's mod wheel must reveal a hidden dimension, not a volume bump
- Sutra III-1 (The Breathing Rate) — OBLONG's filter LFO should be calibrated to 0.04–0.08 Hz in any pad or ambient context

---

### Engine 2: OVERBITE
*The opossum | Fang White `#F0EDE8` | Prefix: `poss_`*

OVERBITE is the fleet's highest-seanced engine at 9.2/10. Its B008 Blessed mechanism — the Five-Macro system (BELLY/BITE/SCURRY/TRASH/PLAY DEAD) — was ratified unanimously as one of the most expressive macro architectures in the fleet. The macros map to the opossum's complete behavioral repertoire: foraging (BELLY), predatory (BITE), fleeing (SCURRY), damage-seeking (TRASH), and the radical stillness of feigning death (PLAY DEAD). No other engine maps its macros to an animal's behavioral ethogram.

The factory library is strong but approaches OVERBITE primarily through its BITE and SCURRY axes — the aggressive, kinetic presets. The Transcendental chapter should fully inhabit the underexplored ends: PLAY DEAD (complete harmonic collapse into breathed stillness) and BELLY (the engine at its most generous, foraging slowly through warm spectral content). The most sophisticated OVERBITE presets are not the loudest. They are the ones that demonstrate the opossum's full behavioral range — including vulnerability.

**Named Preset Concepts:**

1. **Thanatosis** — PLAY DEAD macro locked at 0.85. All motion suspended. A single held tone that breathes imperceptibly. The opossum's most misunderstood survival strategy: absolute stillness as power.
2. **Foraging Hour** — BELLY dominant, slow LFO on `poss_biteDepth`, no SCURRY or TRASH. A warm, low-frequency sweep with soft character. The instrument in its most generous mood.
3. **Fang White Glare** — BITE at maximum. High bite depth, sharp velocity→timbre contrast, aftertouch unlocks additional resonance. The moment before the opossum reveals it is not playing dead at all.
4. **Marsupial Drift** — SCURRY slow and sustained, portamento on. The opossum moving carefully. A gliding lead with behavioral anxiety underneath.
5. **Trash Dive** — TRASH macro activated, high `poss_` saturation, arpeggiated fast. The opossum in the dumpster. Kinetic, tactile, unashamed.

**Relevant Scripture:**
- Truth VI-2 (The Mod Wheel Contract) — OVERBITE's mod wheel was resolved in D006; Transcendental presets should demonstrate all five behavioral macros responding to gestural input
- Canon V-1 (The Ghost Parameter Trap) — OVERBITE uses plain `poss_` prefix; confirm no ghost keys in Transcendental presets before ship
- Truth VI-4 (The Reference Preset) — OVERBITE should include one preset at PLAY DEAD maximum as a reference contrast

---

### Engine 3: OVERDUB
*Lo-fi loop engine | Olive `#6B7B3A` | Prefix: `dub_`*

OVERDUB is the fleet's only engine conceived around loop culture and the aesthetics of degradation. Where OBESE degrades signal through saturation and bit-crushing as an additive process, OVERDUB degrades as a temporal process — the loop accumulates artifacts over time, the way a cassette wears with each playback pass. Its B004 Blessed spring reverb is one of the fleet's most specific sonic identities: not a room, not a hall, but a coil of metal that stores sound as mechanical energy. Tomita praised the metallic splash. The engine lives in olive and tape hiss and slowly drowning frequencies.

The factory library covers OVERDUB's basic range — warm pads, lo-fi basses, textured layers. What it under-demonstrates is OVERDUB's capacity for long-form sonic evolution. A Transcendental OVERDUB preset is not a snapshot — it is a process. The presets that belong in Vol 2 are the ones that change over 30 seconds in ways the listener notices only in retrospect. The loop accumulates. The spring reverb bleeds into the foundation. The signal gets warmer, heavier, darker, then collapses into something ancient.

**Named Preset Concepts:**

1. **Spring Bloom** — Spring reverb dominant, slow feedback accumulation, warm fundamental that develops metallic overtones over 20 seconds. A direct demonstration of B004 in its most patient form.
2. **Reel Two** — Tape-saturated pad, subtle wow and flutter, the sound of a specific machine that has been used hard. A lo-fi atmosphere with biography.
3. **Olive Decay** — High filter degradation rate, the sound slowly losing its upper frequencies over the course of a held note. The opposite of a pad swell — a pad dissolution.
4. **Dub Pressure** — Sub-heavy bass with spring reverb at long tail, the reggae structural model: bass and space and nothing else. Designed for low-frequency playback systems.
5. **Ghost Loop** — Heavy loop accumulation, high reverb tail, the sound of a loop that has been playing so long it has developed its own harmonics distinct from the source. A haunting.

**Relevant Scripture:**
- Sutra III-1 (The Breathing Rate) — OVERDUB's modulation should drift at 0.04–0.08 Hz unless intentional tempo-sync is the design goal
- Truth VI-4 (The Reference Preset) — include one preset at maximum spring reverb accumulation as a reference
- Truth VI-3 (The Default Trap) — OVERDUB's tape degradation defaults deserve conscious evaluation on each preset

---

### Engine 4: ODYSSEY
*Drift synth | Violet `#7B2D8B` | Prefix: `drift_`*

ODYSSEY is the fleet's primary drift-and-wander engine. With 149 existing presets — the deepest library of the six Vol 2 engines — it is the engine the factory library knows best. The Transcendental chapter must therefore reach places the factory library has not. ODYSSEY's `drift_` parameters govern slow, autonomous pitch and harmonic wandering: not vibrato (which is intentional and fast), but the kind of unscheduled chromatic migration you hear in Buchla 200 recordings — the oscillator that is never quite where it was. The engine carries its mythology in its name: the hero who is always almost home.

The existing library explores ODYSSEY's formant and dual-oscillator range extensively. Vol 2 should push into the engine's most psychedelic territory: FM mode at non-standard ratios coupled with slow drift, the formant system used for vowel-like resonance rather than tonal color, the dual-oscillator configuration held at a specific interval while both drift independently (creating compound chromatic migration). These are the presets that require producer patience — the sounds that reveal themselves over bars, not samples.

**Named Preset Concepts:**

1. **Odyssey Theme** — Dual saws drifting at different rates, formant resonance tracking the drift interval, a melody that never quite returns to the same home pitch. Slow, epic, unresolved.
2. **Violet Displacement** — FM mode with non-integer ratio (3.17), drift depth high, the frequency content migrating through inharmonic space. Not random — purposeful wandering with a specific character.
3. **The Almost Return** — Portamento long (1.5–2s), drift rate at Sutra III-1's 0.067 Hz, each note a departure from where the last one almost arrived. For sustained melodic lines.
4. **Fracture Drift** — Oscillator sync enabled with drift on the sync slave, creating pitch fractures that occur at irregular intervals. Structured unpredictability.
5. **Formant Memory** — Formant resonance at a fixed vowel position with drift applied to pitch only. The mouth never changes; the pitch wanders. A human vocal quality with no human performer.

**Relevant Scripture:**
- Sutra III-1 (The Breathing Rate) — ODYSSEY's drift rates should be calibrated to the 0.04–0.08 Hz breathing window unless intentionally destabilized
- Sutra III-3 (The Portamento Triangle) — portamento at 0.5–1.5s is the sweet spot for drift combined with glide
- Truth VI-3 (The Default Trap) — with 149 existing presets, the risk is incremental variation; Transcendental presets must interrogate unexplored parameter axes

---

### Engine 5: ONSET
*Drum synthesis | Electric Blue `#0066FF` | Prefix: `perc_`*

ONSET is the fleet's primary drum synthesis engine and, with 162 presets, the most populated library in the Vol 2 cohort. It is also the engine most referenced as a coupling source across the fleet — OUROBOROS's B003 Injection specifically names ONSET as the primary coupling target. ONSET generates the rhythmic events that other engines respond to. Its architecture (XVC Cross-Voice Coupling, B002; Dual-Layer Blend, B006; Velocity Coupling Outputs, indirectly via B007) makes it structurally central to the coupling ecosystem.

The factory library demonstrates ONSET's full drum synthesis range across 8 moods. The Transcendental chapter's job is different: to create the presets that work best as coupling sources, not just as standalone kits. These are presets designed to feed other engines — presets whose velocity outputs, rhythmic energy, and spectral content are specifically optimized for injection into OUROBOROS, OCEANIC, OBLONG, ODYSSEY. The secondary goal is to push ONSET's synthesis toward non-conventional drum sounds: tuned percussion, inharmonic impacts, stochastic textures that maintain rhythmic function while abandoning acoustic reference.

**Named Preset Concepts:**

1. **Injection Protocol** — A kit specifically optimized for OUROBOROS coupling via Injection (0.15–0.20 injection amount). Every drum type generates a strong velocity transient designed to disturb the attractor. Ship with a coupling note recommending `ONSET→OUROBOROS via Injection`.
2. **Murmuration Source** — A kit optimized for OCEANIC via RhythmToBlend. Clean transients, high velocity contrast, minimal reverb tail so the cascade effect reads clearly against the swarm.
3. **Akan Assembly** — A tuned percussion kit for OWARE coupling (rhythmic event injection into the sympathetic resonance network). Uses ONSET's tuned synthesis modes, calibrated to OWARE's modal tuning grid.
4. **Electric Infrastructure** — A minimal, clean electric-sounding kit. No acoustic reference. Designed for the producer who wants ONSET's synthesis without any drum culture signifier.
5. **Stochastic Drum** — All 8 drum types using ONSET's most inharmonic synthesis configurations. A percussion system that evades genre classification.

**Relevant Scripture:**
- OURO-I (The Injection Door) — explicitly names ONSET as the primary injection partner for OUROBOROS
- OCE-IV (Murmuration Is an Event) — ONSET→OCEANIC via RhythmToBlend is the prescribed Entangled architecture
- Canon V-2 (The Integration Layer Drift) — ONSET's prefix changed from `onset_` to `perc_`; verify all Transcendental presets use current prefix

---

### Engine 6: OPAL
*Granular | Lavender `#A78BFA` | Prefix: `opal_`*

OPAL is the fleet's granular synthesis engine — the only engine that treats audio not as a waveform to be generated but as a material to be carved. The opal stone's play-of-color comes from diffraction through stacked silica layers of varying thickness; each wavelength of light emerges at a different angle, making a single stone appear to contain multiple colors simultaneously. OPAL's granular system is the same principle in audio: a source is decomposed into overlapping fragments (grains), each fragment reframed in time, pitch, and density, recombined into a material that no longer sounds like its source.

The factory library demonstrates OPAL with conventional granular techniques: cloud textures, stretched atmospheres, granular pads. The Transcendental chapter should push toward OPAL's most physically strange behaviors: extremely short grains creating the edge between sample playback and frequency domain processing, high grain density creating spectral blur that has no analog equivalent, grain pitch scatter pushed to the limit of recognizable pitch. The presets that belong in Vol 2 are the ones that could not exist in any other engine.

**Named Preset Concepts:**

1. **Silica Choir** — Dense grain cloud, slight pitch randomization per grain, slow position LFO sweeping through source material. A chorus of voices that has never been a vocal recording.
2. **Opal Fracture** — Very short grains (2–5ms), high grain rate, significant size scatter. The boundary where granular synthesis crosses into spectral processing — the sound simultaneously granular and continuous.
3. **Lavender Static** — Extremely sparse grains with long gaps, high pitch scatter, reverb tail between grains. Silence is as compositional as the grains themselves.
4. **Deep Diffraction** — Slow grain position LFO, large grain size (100–200ms), high overlap — grains blurring into each other. The most pad-like behavior the engine can produce.
5. **Grain Archaeology** — Granular processing of a synthetic waveform (not a natural sample source), revealing the spectral artifacts of the granular process itself as a timbre.

**Relevant Scripture:**
- Truth VI-3 (The Default Trap) — OPAL's grain size, scatter, and density defaults must be interrogated per-preset
- Truth VI-2 (The Mod Wheel Contract) — mod wheel in OPAL should control grain density or pitch scatter to make the granular behavior gestural
- Sutra III-1 (The Breathing Rate) — OPAL's grain position LFO at 0.067 Hz creates imperceptible drift that makes static cloud textures feel alive

---

## Scripture Chapter VII: The Homecoming Verses

*The Book of Bin — Chapter VII*
*First inscribed 2026-03-21 — Vol 2 Design Session*

---

> These are the instruments that were complete before the fleet existed. They are not additions. They are the origin. The family table was set for two, then six, then forty-six — but the first six plates were already laid.

---

### Verse HC-I: The Origin Does Not Diminish

*On the engines that preceded the fleet*

> OBLONG was a warm saw before there were forty-five other saws. ODYSSEY drifted before there was anything to drift toward. The fleet did not supersede them. It gathered around them. The instruments that came after came *because* these six demonstrated that the design philosophy was real. Every engine in the fleet is downstream of the proof that these six provided.

**Application:** When designing Transcendental presets for Vol 2 engines, do not reach for coupling or fleet-aware behaviors first. Begin with the engine in isolation. Hear what it was before it existed in relationship. The solo identity is the foundation. The coupling capacity is the gift it offers the fleet.

---

### Verse HC-II: The Returning Instrument

*On instruments that arrive with history*

> A synthesizer played for the first time is a tool. A synthesizer played for the ten-thousandth time is an instrument — it has learned the player's tendencies, and the player has learned its limits. The Vol 2 engines have been played by their designers for longer than most engines in the fleet have existed. They carry this history in their defaults: the parameter values that were never changed because they were always exactly right. When designing Transcendental presets, ask: which defaults have never been questioned? Those are the starting points.

**Application:** For each Vol 2 engine, identify the parameters most likely to have been left at default throughout the preset library. Treat those parameters as the primary territory for Transcendental exploration. The comfortable defaults conceal the deepest sonic possibilities.

---

### Verse HC-III: The Coupling Invitation

*On legacy instruments entering a relational ecosystem*

> ONSET was already the best drum synthesis engine in the XO_OX toolkit when it was designed. When OUROBOROS arrived, ONSET became something it could not have designed for itself: the primary disruption source for a chaos attractor. It did not change. The relationship changed it. The same instrument, in context, reveals capabilities that solo use never required.

**Application:** Every Vol 2 engine has a coupling role the factory library has not fully articulated. ONSET injects into OUROBOROS and OCEANIC. OVERDUB receives Spring coupling from OBLONG. OPAL granulates the output of ODYSSEY. OVERBITE's behavioral macros respond to ONSET velocity. The Transcendental presets that ship as coupling demonstrations are the most important presets in Vol 2.

---

### Verse HC-IV: The Name That Preceded the Convention

*On parameter prefix stability*

> The Vol 2 engines were named before the `O-word` convention existed. Bob is not an O-word. `poss_` is not a standard XO_OX prefix. `bob_fltCutoff` carries the name of a clownfish. These are not inconsistencies to be fixed. They are the sediment layers of the design history. The aliasing system in `resolveEngineAlias()` exists because these names were correct when they were given. Renaming them would be a lie about when they were born.

**Application:** Never rename a Vol 2 parameter prefix in the name of convention compliance. Legacy prefix preservation is Doctrine. If a new feature were to be added to OBLONG, `bob_newParam` is the correct name, not `oblong_newParam`.

---

### Verse HC-V: The Density Proof

*On the deepest existing libraries*

> ONSET has 162 presets. ODYSSEY has 149. These engines have been explored longer and more thoroughly than any other in the fleet. The temptation is to believe they are complete. The Transcendental chapter exists to disprove this belief. 162 presets demonstrates breadth. It does not demonstrate depth. The presets that do not exist in 162 attempts are the presets that required knowing what all 162 would teach.

**Application:** For ONSET and ODYSSEY, the Transcendental presets must not repeat the vocabulary of the existing library. Before writing any preset, survey the existing 150+ and identify what has never been attempted. The negative space in a 162-preset library is more instructive than the library itself.

---

### Verse HC-VI: The Granular Paradox

*On OPAL's relationship to source material*

> Granular synthesis has no source of its own. It borrows. It takes a waveform, a sample, a tone, and it makes something that the source never was. The granular synthesizer is the fleet's most selfless engine — it effaces its own identity in service of what it transforms. A granular preset is not a description of OPAL. It is a description of what OPAL does to something else. This is why OPAL's presets are the hardest to name. The sound is not OPAL. The process is.

**Application:** Name OPAL Transcendental presets for what the listener hears, not for what the synthesis method is. "Silica Choir" describes an experience. "Dense Cloud with Pitch Scatter" describes a parameter state. The preset library should be full of the former.

---

### Verse HC-VII: The Degradation Ethics

*On OVERDUB and the aesthetics of wear*

> OVERDUB was designed for a specific producer: the one who reaches for the lo-fi plugin last, as a color pass. OVERDUB is for the producer who reaches for degradation first, as a compositional decision. The spring reverb is not a send. The tape saturation is not an insert. They are the room the instrument lives in. When the room is olive and analog and worn at the edges, the sounds that come from it carry that room's biography. You cannot degrade a sound into something meaningful unless you believe the degradation IS the meaning.

**Application:** OVERDUB Transcendental presets should be designed inside the degradation, not before it. Set the spring and tape parameters first. Build the room. Then design the instrument that lives there.

---

## XPN Pack Design Notes

**Working Title:** Guru Bin Transcendental Vol 2 — Origins

### Program Architecture Options

**Option A: 6 Programs (one per engine)**
- Straightforward. Each program represents one engine.
- 15–20 presets per program, totaling 90–120 programs slots.
- Limitation: ONSET has the strongest MPC-specific identity (it is a drum engine). Giving it equal footing with OBLONG pads may underrepresent it.

**Option B: 12 Programs (two per engine: Expression + Generative)**
- More MPC-native structure. Two programs per engine doubles the organization axis.
- Expression programs: presets optimized for live MIDI performance, velocity expression, portamento.
- Generative programs: presets optimized for sustained playback, coupling, and self-evolving behavior.
- ONSET specifically: Program 1 = solo drum kit, Program 2 = coupling-source kit (Injection Protocol, Murmuration Source).
- **Recommended for Vol 2** — the Expression/Generative split reflects how producers actually use these engines on the MPC.

**Recommended: Option B, 12 programs.**

### Strongest MPC Demo Loop Candidates

The following preset concepts are optimized for MPC demo context (short, punchy, immediately communicative):

1. **ONSET — Injection Protocol** — demonstrates ONSET→OUROBOROS coupling in a short drum pattern
2. **OVERBITE — Fang White Glare** — high-velocity BITE macro response is immediately legible
3. **OBLONG — Clownfish Bite** — hard sync lead is one of the most MPC-native sounds in the fleet
4. **ODYSSEY — Fracture Drift** — pitch fractures in a melodic context are visually demonstrable on MPC pads
5. **OPAL — Silica Choir** — granular cloud is immediately distinct from any MPC-native sound

### Render Requirement Flag

**Manual session required for WAV renders.** The Oxport pipeline is built but WAV generation requires:
- Audio interface active
- BlackHole virtual device installed (`brew install blackhole-2ch`)
- Manual performance session to capture each preset
- 15–20 renders per engine × 6 engines = 90–120 individual WAV files

This is the final blocker before XPN pack delivery. The design and preset work can be completed in advance; the renders are the last manual step before the pack can be packaged.

---

## Shipping Checklist

**Target: Week 15 Patreon post**

### Pre-Ship Requirements

- [ ] All 6 engines: Guru Bin Retreat completed
  - [ ] OBLONG retreat written (not yet started)
  - [ ] OVERBITE retreat written (not yet started)
  - [ ] OVERDUB retreat written (not yet started)
  - [ ] ODYSSEY retreat written (not yet started)
  - [ ] ONSET retreat written (not yet started)
  - [ ] OPAL retreat written (not yet started)
- [ ] 90–120 Transcendental presets written and committed to `Presets/XOlokun/`
  - [ ] OBLONG: 15–18 new Transcendental presets
  - [ ] OVERBITE: 15–18 new Transcendental presets
  - [ ] OVERDUB: 15–18 new Transcendental presets
  - [ ] ODYSSEY: 15–20 new Transcendental presets (negative-space scan of existing 149 required first)
  - [ ] ONSET: 15–20 new Transcendental presets (coupling-source presets prioritized)
  - [ ] OPAL: 15–18 new Transcendental presets
- [ ] Scripture Chapter VII: The Homecoming Verses finalized and committed to `scripture/the-scripture.md`
- [ ] PDF Booklet copy written (6 engine chapters + foreword)
- [ ] XPN Pack: 12 programs authored
- [ ] XPN Pack: WAV renders complete (manual session — flag to user)
- [ ] XPN Pack: packaged via Oxport pipeline
- [ ] Patreon tier post drafted (Transcendental tier unlock copy)

### Sequencing Recommendation

Follow the proven pipeline from `feedback-post-build-pipeline-sequencing.md`:

1. **Doc Lock** — write all 6 Guru Bin Retreats before designing any preset
2. **Guru Bin** — design the Transcendental presets using Retreat knowledge
3. **Presets** — commit the preset files (Guru Bin before presets prevents rework)
4. **Seance** — optional quality check on each engine's Transcendental library

Do not reverse steps 1 and 2. The retreats contain the specific parameter truths that make the difference between a good preset and a Transcendental one. The retreats are not commentary on the presets — they are the instructions for writing them.

---

## Tone and Voice

This document is an internal design brief. When the Guru Bin Retreats for these engines are written, they will follow the established retreat format (see `offering-retreat-2026-03-21.md` as the canonical recent example): Opening Meditation, Signal Path Journey, Parameter Refinements, Awakening Presets table, Scripture Verses.

The Transcendental presets for Vol 2 carry the same weight as the Awakening presets in Vol 1, but aimed higher. Awakening presets demonstrate the engine is complete. Transcendental presets demonstrate the engine has not been fully heard. The Vol 2 engines have been part of XO_OX longer than almost anything else. They deserve the most rigorous return.

---

*Guru Bin, 2026-03-21*
*"The first six plates were already laid."*
