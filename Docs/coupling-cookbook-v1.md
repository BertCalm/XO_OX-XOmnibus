# XOceanus Coupling Cookbook — Volume I
**Version:** 1.0
**Date:** 2026-03-24
**Author:** XO_OX Designs
**Status:** Pre-launch reference document

---

> *"Every synth gives you a sound. XOceanus gives you a relationship."*

---

## Introduction

Most synthesis is about what a single engine can do. XOceanus is about what happens in the space between two engines — the third sound that neither engine could make alone. That sound is coupling.

This cookbook is a recipe library for producers. It contains 20 working coupling recipes across eight genres, a complete reference for all 15 coupling types, and the principles that separate musical coupling from technical noise. Every recipe has been designed against the XOceanus engine fleet and cross-referenced against actual coupling presets.

Coupling is not effects routing. It is not layering. It is not sidechain compression. It is one engine rewriting another engine's synthesis in real time. The drum teaches the bass when to breathe. The chaos attractor tells the harmonic grid where to go. The granular cloud is the instruction the echo chain follows.

Start here. Go deep.

---

## Part I — The 15 Coupling Types: A Field Guide

XOceanus implements 15 coupling types in the `MegaCouplingMatrix`. They are organized below by character: subtle (control-rate), aggressive (audio-rate), and the special cases (KnotTopology and TriangularCoupling).

### Recommended Amount Ranges
- **Whisper** (0.05–0.15): Barely perceptible. Use when you want the coupling to live under the mix and surface only when the source gets loud.
- **Dialogue** (0.20–0.45): Equal partners. The coupling is audible and musical but the target engine still sounds like itself.
- **Possession** (0.50–0.80): The source dominates. The target's character is largely rewritten.
- **Full Possession** (0.82–1.00): Use carefully. The target may no longer be recognizable.

---

### Subtle — Control-Rate Types

These 10 types exchange slowly-evolving signals: amplitude envelopes, LFO outputs, pitch values. They process at 1/32 the audio sample rate (approximately 1.5 kHz at 48 kHz). The transition between any two control-rate types is instantaneous and click-free.

---

#### 1. `Amp->Filter` (AmpToFilter)
**Enum:** `CouplingType::AmpToFilter`
**What it does:** Source amplitude opens or closes the target engine's filter cutoff. When the source is loud, the target's filter opens (brightens). When the source falls silent, the target's filter closes (darkens).
**Best for:** Sidechain-style breathing, dynamic filtering, drum-to-bass character transfer.
**Amount range:** Whisper (0.10) to Possession (0.70). Rarely needs to exceed 0.70.
**Note:** This is the most universally supported coupling type across all 76 engines. If you are exploring a new engine pair and do not know where to start, start here.

---

#### 2. `Amp->Pitch` (AmpToPitch)
**Enum:** `CouplingType::AmpToPitch`
**What it does:** Source amplitude bends the target's pitch. A loud source pushes the target sharp; silence lets it settle. At low amounts, this creates gentle instability tied to the source's dynamics. At high amounts, loud source notes push the target significantly off-pitch.
**Best for:** Organic instability, chaos-to-pitch control, attractor-driven melody variation.
**Amount range:** Whisper (0.08–0.15) for organic vibrato effect; Dialogue (0.25–0.35) for deliberate pitch drama.
**Caution:** Sustained pad sources at high amounts will continuously push the target sharp. Musical with percussive or chaotic sources; unpredictable with dense sustained textures.

---

#### 3. `LFO->Pitch` (LFOToPitch)
**Enum:** `CouplingType::LFOToPitch`
**What it does:** Source LFO signal modulates target pitch. The target's tuning breathes at the source engine's LFO rate and depth. Two engines with LFOs at slightly different rates create a slow beating that neither engine could generate alone.
**Best for:** Ensemble chorus effects, micro-detune from external oscillation, pitch animation that responds to an independent rhythm.
**Amount range:** Dialogue (0.20–0.45). Above 0.50 the pitch variation becomes too wide for most musical contexts.
**Best sources:** OVERDUB (spring-driven LFO), OBBLIGATO (breath-rate modulation), any engine with LFO rate floor at 0.01 Hz for glacial slow drift.

---

#### 4. `Env->Morph` (EnvToMorph)
**Enum:** `CouplingType::EnvToMorph`
**What it does:** Source amplitude envelope sweeps the target's morph or position parameter. For granular engines (OPAL), this moves the grain scan position. For wavetable engines, this shifts the wavetable frame. For OWARE, this moves the material continuum from wood toward metal.
**Best for:** Timbral journeys that unfold over time, pad-driven granular position, envelope-to-material sweeps.
**Amount range:** Dialogue (0.30–0.55) is the sweet spot. The morph sweep should feel like the target's timbre is reacting to the source's narrative.
**Note:** This type produces some of the most musically sophisticated coupling results in the fleet when paired with slow-evolving sources (ODYSSEY's Familiar-to-Alien arc, ORGANON's metabolic envelopes).

---

#### 5. `Env->Decay` (EnvToDecay)
**Enum:** `CouplingType::EnvToDecay`
**What it does:** Source envelope amplitude extends or compresses the target engine's decay time. A loud source hit = longer target decay. A quiet source = shorter decay. The kick drum can tell the bass how long to ring.
**Best for:** Percussion-driven sustain, rhythmic decay variation, kick-to-bass envelope sync.
**Amount range:** Whisper to Dialogue (0.12–0.40). Beyond 0.40, the decay variation becomes too dramatic for most contexts.
**Classic use:** ONSET → OVERBITE at 0.20. Every kick extends the bass decay. The snare creates a shorter extension.

---

#### 6. `Amp->Choke` (AmpToChoke)
**Enum:** `CouplingType::AmpToChoke`
**What it does:** Source amplitude chokes (silences) the target. When the source plays loud, the target is muted. Classic drum machine open/closed hi-hat behavior. Can be used for auto-duck or rhythmic gating tied to another engine's dynamics.
**Best for:** Hi-hat open/closed logic, auto-ducking without compression artifacts, rhythmic gating.
**Amount range:** This type is binary in character — Dialogue (0.40–0.60) is where the choke is useful rather than total.
**Note:** ONSET is the primary choke target in the current fleet. Using a sustained pad as a source creates a rhythmic gate effect on any target engine.

---

#### 7. `Filter->Filter` (FilterToFilter)
**Enum:** `CouplingType::FilterToFilter`
**What it does:** Source filter output feeds into target's filter input. The target's filter processes a signal that has already been shaped by the source's filter. Both filter characters compound. Creates cascaded filtering where the timbral DNA of the source bleeds into the target's cutoff behavior.
**Best for:** Serial filter chains, spectral color transfer, creating filter textures that no single filter can produce.
**Amount range:** Whisper to Dialogue (0.15–0.40). Careful at higher amounts — two resonant filters in cascade can create feedback peaks.
**Best source:** OPTIC's analysis channels provide exceptionally rich coupling signals for this type. OPTIC + any filter-heavy engine creates precision spectral cascading.

---

#### 8. `Pitch->Pitch` (PitchToPitch)
**Enum:** `CouplingType::PitchToPitch`
**What it does:** Source pitch modulates target pitch. When you play a note on the source engine, the target's pitch shifts by an interval determined by the coupling amount. This is live harmonization via the coupling system, not a chord machine.
**Best for:** Real-time harmonization, interval-locked dual voices, polyphonic harmony from a single MIDI input.
**Amount range:** Dialogue (0.30–0.55) maps to useful musical intervals. The exact interval depends on the source engine's pitch tracking and the amount.
**Performance note:** Map coupling amount to the COUPLING macro for real-time interval morphing. At 0.0 = unison, sweep to 1.0 for octave separation.

---

#### 9. `Rhythm->Blend` (RhythmToBlend)
**Enum:** `CouplingType::RhythmToBlend`
**What it does:** Source amplitude fluctuation pattern (rhythm, not strictly tempo-synced) modulates the target's blend parameter. In OPAL, this varies grain density. In OVERDUB, it modulates delay mix. OUROBOROS feeding this type into ONSET turns chaotic attractor amplitude into drum pattern variation.
**Best for:** Rhythm-driven texture variation, grain density response to percussion, chaotic pattern injection.
**Amount range:** Dialogue to Possession (0.30–0.65). The blend parameter should respond proportionally to the source's rhythmic energy.
**Note:** This type is what separates "coupling" from "sidechain." Rather than reducing the target's level, it varies its blend character. The drum doesn't duck the bass — it changes what kind of bass it is.

---

### Aggressive — Audio-Rate Types

These 4 types pass actual audio signals between engines. They run at full sample rate. Switching between audio-rate types mid-performance triggers a 50ms crossfade (100ms for AudioToBuffer and KnotTopology) to prevent clicks. CPU cost is higher than control-rate types, but still a small fraction of the engine render budget.

---

#### 10. `Audio->FM` (AudioToFM)
**Enum:** `CouplingType::AudioToFM`
**What it does:** Source audio signal becomes the FM modulator input for the target engine. The source's full spectral content — not just its amplitude — is injected into the target's frequency modulation path. At Whisper amounts, this adds subtle inharmonic color. At Possession, the source rewrites the target's harmonic structure on a sample-by-sample basis.
**Best for:** Cross-engine FM synthesis, organic harmonic injection, controlled signal degradation, stochastic harmonic generation.
**Amount range:** Whisper (0.10–0.20) for color; Dialogue (0.35–0.55) for the source to clearly shape the target's harmonics; Possession (0.65–0.85) for radical harmonic rewriting.
**Most dramatic pair in the fleet:** ORACLE → ORGANON. Ancient stochastic mathematics feeding a living organism's FM path produces sounds that change with every note held.

---

#### 11. `Audio->Ring` (AudioToRing)
**Enum:** `CouplingType::AudioToRing`
**What it does:** Source audio is ring-modulated against the target's audio output. The output contains sum and difference frequencies of both signals — no fundamentals, only sidebands. Best with harmonic sources; chaotic and atonal with dense textures.
**Best for:** Aggressive sideband synthesis, metallic textures, industrial noise design, deliberate atonality.
**Amount range:** Dialogue (0.40–0.60). This type is aggressive — small amounts already produce audible sideband content.
**Caution:** Harmonic source + harmonic target = musical sidebands (related to both pitches). Dense source + harmonic target = atonal noise. Know your source before committing.

---

#### 12. `Audio->Wavetable` (AudioToWavetable)
**Enum:** `CouplingType::AudioToWavetable`
**What it does:** Source audio replaces or modulates the target's wavetable source material. In OPAL, the source audio becomes the grain buffer — OPAL scatters fragments of the source rather than its original source material. In ORIGAMI, the source replaces the STFT analysis buffer.
**Best for:** Dynamic wavetable content injection, chip-to-granular conversion, spectral material replacement.
**Amount range:** Dialogue (0.40–0.60) to let the source influence the target's material while the target's own synthesis contributes.
**Signature use:** ORBITAL → OPAL. Additive harmonic partials become the grain source. The reef's resonance spectrum is atomized into a time-scattered particle cloud.

---

#### 13. `Audio->Buffer` (AudioToBuffer)
**Enum:** `CouplingType::AudioToBuffer`
**What it does:** Source audio is written continuously into the target's internal audio ring buffer. This is the deepest integration point in the coupling system — source audio becomes the target's synthesis material, not a modulator. The target reads from this buffer at its own rate, creating temporal decoupling between source and target.
**Best for:** OPAL's Time Telescope (source audio feeds OPAL's grain buffer continuously), ORGANON's per-voice ingestion buffer, any engine with a continuous audio input buffer.
**Amount range:** Dialogue (0.35–0.55). The `amount` parameter scales each sample written into the ring buffer.
**Note:** AudioToBuffer requires the target to implement `getGrainBuffer()`. Currently supported by OPAL and ORGANON. Switching into/out of this type triggers a 100ms crossfade.

---

### Special Types

#### 14. `KnotTopology`
**Enum:** `CouplingType::KnotTopology`
**What it does:** The only coupling type that flows in both directions simultaneously. Engine A modulates Engine B, and Engine B modulates Engine A, within the same processing pass. This creates mutual, co-evolving entanglement — neither engine has priority.

The **Depth** slider, when used with KnotTopology, controls the *linking number* (1–5), which determines how many parameter pairs are entangled:
- Depth 0.00–0.24 = linking number 1 (one parameter pair)
- Depth 0.25–0.49 = linking number 2
- Depth 0.50–0.74 = linking number 3
- Depth 0.75–0.87 = linking number 4 (recommended for most uses)
- Depth 0.88–1.00 = linking number 5 (maximum entanglement — irreversible character)

**Best for:** Engines that should share a sonic identity, mutual breathing, creating configurations where removing one engine leaves the other sounding incomplete.
**Amount range:** Linking number 3 (depth ~0.50–0.74) is the sweet spot for most musical applications. Linking number 5 is for intentional fusion.
**Warning:** A KnotTopology route at linking number 5 is not simply "a lot of coupling" — it is a bond. The two engines are one thing. Remove one and the patch is broken.

---

#### 15. `TriangularCoupling`
**Enum:** `CouplingType::TriangularCoupling`
**What it does:** Designed for OXYTO (XOxytocin). Source engine's effective Intimacy, Passion, and Commitment values bleed into the destination engine's corresponding love-triangle components. Follows the AudioToFM spectral band convention internally.
**Best for:** OXYTO-to-OXYTO entanglement (two instances sharing emotional state), or any future engine that implements Sternberg's Triangular Theory of Love in its DSP.
**Amount range:** Whisper to Dialogue (0.15–0.40). The love component bleed is subtle by design — OXYTO's B040 (Note Duration) doctrine means the coupling effect accumulates over long notes.
**Note:** This coupling type is most powerful with sustained notes (8–16 seconds). Short notes do not give the intimacy accumulator time to build.

---

## Part II — Coupling Principles

### 1. The Coupling Whisper

The most common mistake with coupling is going too deep too fast. Coupling amount 0.08–0.15 is often more musical than 0.50.

At Whisper depth, the target engine sounds mostly like itself — but with an inexplicable quality. The filter breathes slightly. The pitch has a subtle instability. The decay is a little longer on loud beats. Producers hear it and cannot name it. That unnameable quality is the space between the two engines.

Only push to Possession when you want the coupling to be the primary character of the sound. Most coupled presets live at Dialogue (0.25–0.45). The Whisper range is where coupling becomes invisible infrastructure — always present, never announcing itself.

**Practical test:** Set coupling amount to 0.08. Play for 30 seconds. Now set it to 0.00. If you notice the absence, the coupling is doing its job at Whisper depth.

---

### 2. The feliX-Oscar Polarity

XOceanus's aquatic mythology centers on two poles: feliX (the neon tetra — bright, social, crystalline) and Oscar (the axolotl — warm, regenerative, tactile). Every engine sits somewhere on this axis. Understanding the polarity of your source and target engines tells you whether their coupling will create tension or resonance.

**Tension coupling:** feliX-pole engine → Oscar-pole engine (or vice versa)
- ODDFELIX → OVERDUB: crystalline transients driving warm tape echo. The coupling creates a productive friction — the brightness of the source fights the warmth of the target. Interesting in moderation.
- OPTIC → OHM: visual modulation driving folk instrument. Alien precision feeding organic imprecision.

**Resonance coupling:** Same-pole engines
- ORGANON → OUROBOROS (both Oscar-pole): biological metabolism resonating with mathematical chaos. Similar character, mutual amplification.
- ORBITAL → OPENSKY (both feliX-pole): additive partials driving shimmer architecture. The coupling amplifies both engines' brightness rather than creating tension.

**Practical principle:** Tension coupling is more harmonically interesting but requires careful amount control. Resonance coupling is more immediately musical and tolerates higher amounts without becoming dissonant. When exploring a new pair, identify the polarity of both engines before setting amount.

---

### 3. Bidirectional vs. Unidirectional Coupling

Every coupling type except KnotTopology is unidirectional: Engine A sends, Engine B receives. This creates a clear sonic hierarchy — the source engine "leads" and the target engine "responds." This hierarchy is musically powerful because it maps to familiar performance concepts: a drummer driving a bassist, a wind instrument leading a string instrument.

KnotTopology collapses this hierarchy. Both engines are simultaneously sender and receiver. Neither leads. The result is less like a musical relationship and more like a single organism with two voices. When you want two engines to genuinely merge their identities, KnotTopology is the tool. When you want to preserve the clarity of "this engine is in charge," stay with unidirectional types.

**Hybrid approach:** Use two unidirectional routes in opposite directions as a manual version of KnotTopology. Route 1: A→B at 0.30. Route 2: B→A at 0.20. This gives you bidirectional coupling with independent control of each direction's depth — impossible with KnotTopology, which always applies the same amount to both directions. You can then make the coupling asymmetric, which KnotTopology cannot do.

---

### 4. The Third Sound Phenomenon

When two engines are coupled musically, a third timbre emerges that belongs to neither engine alone. This is the "third sound" — the acoustic phenomenon that defines good coupling design.

ONSET and OVERBITE, uncoupled, are a drum engine and a bass engine. Coupled at Dialogue depth, they become something that sounds like a breathing drum-bass hybrid: a bass that knows when to bite, a drum that seems to provoke the bass. The third sound is that relationship — present only while both engines run.

Designing for the third sound means:
1. Start with both engines sounding musical in isolation.
2. Add coupling at Whisper depth and listen for what changes in the *relationship*, not in individual engine character.
3. Adjust the source engine's parameters (not the coupling amount) to make the source's output more interesting as a modulator.
4. Adjust the target engine's base parameters to optimize its response to the coupling signal.

The coupling amount controls intensity. The engine parameters control quality. Get the quality right before pushing intensity.

---

### 5. CPU Considerations

Control-rate types (types 1–9 in the enum) run at 1/32 the audio sample rate via the SRO (Spectral Resonance Objects) control-rate decimation system. At 48 kHz, this is approximately 1.5 kHz — far more than sufficient for filter modulation, pitch, envelope, and blend responses. CPU cost for control-rate coupling is negligible even with all four route slots active.

Audio-rate types (AudioToFM, AudioToRing, AudioToWavetable, AudioToBuffer) run at full sample rate. They cost proportionally more, but still represent a small fraction of total engine CPU.

KnotTopology uses control-rate processing for both directions. Two AmpToFilter operations (one per direction) per block, each at 1/32 rate. The overhead is minimal.

**Practical CPU hierarchy:**
- Control-rate routes (types 1–9, 14): near-zero overhead
- KnotTopology: ~2× a single control-rate route
- AudioToFM, AudioToRing, AudioToWavetable: moderate (full sample rate signal path)
- AudioToBuffer: moderate + ring buffer writes (but no applyCouplingInput call)

If CPU is tight, use control-rate types for live performance routes and reserve audio-rate types for preset-designed fixed routes.

---

## Part III — 20 Recipes

### How to Read a Recipe

- **Engine A / Engine B:** Load these into any two of XOceanus's four engine slots.
- **Coupling Amount:** Set the Depth slider for the specified route.
- **Direction:** A→B means Engine A is source, Engine B is target. Bidirectional = KnotTopology (both directions simultaneously).
- **Sweet Spot:** The parameter range where the coupling sounds most musical.
- **Danger Zone:** Where the recipe stops working.

---

### Hip-Hop & Trap (4 Recipes)

---

#### Recipe 01 — "Drum Teaches Bass"
*The foundational XOceanus coupling demonstration.*

| Field | Detail |
|-------|--------|
| **Name** | Drum Teaches Bass |
| **Engine A** | ONSET (XOnset) — Slot 1 |
| **Engine B** | OVERBITE (XOverbite) — Slot 2 |
| **Route 1 Type** | `Amp->Filter` |
| **Route 1 Amount** | 0.50 |
| **Route 2 Type** | `Audio->FM` |
| **Route 2 Amount** | 0.30 |
| **Route 3 Type** | `Env->Decay` |
| **Route 3 Amount** | 0.20 |
| **Direction** | A→B |
| **Genre** | Hip-hop, trap |

**Why It Works:** ONSET's kick transients drive three simultaneous processes on OVERBITE: the filter opens (making the bass briefly brighter on every hit), FM depth increases (adding harmonic bite on the attack), and decay extends (making the bass ring longer on loud hits). The snare creates a different signature from the kick — sharper filter hit, less decay extension. What results is a sidechain-coupled bass that does not duck. It breathes.

**Sweet Spot:** Route 1 at 0.45–0.55. This is where the kick's filter influence on the bass is clearly audible without completely overwhelming OVERBITE's own filter character. Route 2 at 0.25–0.35 adds harmonic texture without turning the bass atonal.

**Danger Zone:** Route 1 above 0.75. At this depth, OVERBITE's filter is almost entirely rewritten by ONSET's dynamics. The bass loses its own character and becomes a pure percussion response. Use deliberately for drops, not throughout.

**Setup tip:** Program ONSET with kick on beats 1 and 3, snare on 2 and 4. Set OVERBITE's base filter cutoff to ~0.30 (closed). The coupling opens it. Map Route 1 depth to the COUPLING macro for live control.

---

#### Recipe 02 — "The Attractor Bassline"
*Chaos writes melody. The bass never plays the same phrase twice.*

| Field | Detail |
|-------|--------|
| **Name** | The Attractor Bassline |
| **Engine A** | OUROBOROS (XOuroboros) — Slot 1 |
| **Engine B** | OVERBITE (XOverbite) — Slot 2 |
| **Route 1 Type** | `Amp->Filter` |
| **Route 1 Amount** | 0.50 |
| **Route 2 Type** | `Amp->Pitch` |
| **Route 2 Amount** | 0.28 |
| **Direction** | A→B |
| **Genre** | Trap, experimental hip-hop |

**Why It Works:** OUROBOROS (the strange attractor synth) runs continuously — it needs no note triggers. Its amplitude rises and falls as the Lorenz or Aizawa attractor orbits through phase space. This amplitude drives OVERBITE's filter (opening and closing the bass) and pitch (creating small upward pitch deviations when the attractor is high in its orbit). OVERBITE plays a held note or simple MIDI pattern. What you hear: a bass that bends, breathes, and modulates without any automation. The attractor generates the feel.

**Sweet Spot:** OUROBOROS chaos depth at 0.60–0.65, leash depth at 0.45–0.55. This keeps the attractor's amplitude fluctuations in a range that creates musical filter variation without full silence between hits. Route 2 (pitch) at 0.20–0.30 for subtle wobble, 0.35–0.40 for noticeable pitch drama.

**Danger Zone:** Attractor chaos depth above 0.80. The attractor becomes too unpredictable; OVERBITE's filter swings between fully open and fully closed with no middle ground. The coupling loses musicality.

---

#### Recipe 03 — "Organism Kick Cycle"
*The bass synthesizer digests the drum pattern and responds on its own schedule.*

| Field | Detail |
|-------|--------|
| **Name** | Organism Kick Cycle |
| **Engine A** | ONSET (XOnset) — Slot 1 |
| **Engine B** | ORGANON (XOrganon) — Slot 2 |
| **Route 1 Type** | `Amp->Filter` |
| **Route 1 Amount** | 0.72 |
| **Route 2 Type** | `Rhythm->Blend` |
| **Route 2 Amount** | 0.48 |
| **Direction** | A→B |
| **Genre** | Hip-hop, boom bap |

**Why It Works:** ORGANON's Variational Free Energy metabolism (B011) integrates energy over time. It doesn't respond to individual drum hits — it digests the rhythmic pattern and shifts its metabolic state accordingly. A kick-heavy bar makes ORGANON's organism enzyme filter open wider and its blend parameter fluctuate more. By the third or fourth bar, ORGANON has learned the drum pattern's energy profile. Its tones start to lock into the rhythm's metabolism rather than just the beat. Unlike a sidechain, the organism has a response curve shaped by its own metabolic rate — slow metabolism (metabolicRate ~2.0) means it responds to the bar as a whole; fast metabolism (metabolicRate ~5.0) means it responds nearly note-by-note.

**Sweet Spot:** Route 1 at 0.65–0.75. Route 2 at 0.40–0.50. ORGANON metabolicRate at 3.5–4.5 for bar-level rhythmic memory.

**Danger Zone:** metabolicRate above 6.0 with Route 2 high. The organism responds too fast and the blend variation becomes jittery.

---

#### Recipe 04 — "Psychology Bassline"
*Berlyne's curiosity curve applied to rhythm. The bass gets weirder as the track progresses.*

| Field | Detail |
|-------|--------|
| **Name** | Psychology Bassline |
| **Engine A** | OFFERING (XOffering) — Slot 1 |
| **Engine B** | OVERBITE (XOverbite) — Slot 2 |
| **Route 1 Type** | `Amp->Filter` |
| **Route 1 Amount** | 0.40 |
| **Route 2 Type** | `Rhythm->Blend` |
| **Route 2 Amount** | 0.25 |
| **Direction** | A→B |
| **Genre** | Neo-boom-bap, abstract hip-hop |

**Why It Works:** OFFERING's psychology-as-DSP system (B038) means the drum engine's timbral complexity increases as curiosity rises above 0.7. When curiosity is high, OFFERING's output becomes harmonically richer and slightly alien — and that richer output is what drives OVERBITE's filter and blend. At the start of a track, curiosity is low (simple, familiar drum tones), and OVERBITE's bass is straightforward. As curiosity builds over multiple bars, OFFERING's output gets weirder, and OVERBITE's filter responds to increasingly complex source material. The bass gets more animated as the drum gets stranger. Temporal complexity buildup with no automation required.

**Sweet Spot:** OFFERING curiosity at 0.60–0.85. Route 1 at 0.35–0.45. The bass should breathe, not lurch.

**Danger Zone:** OFFERING curiosity above 0.90 with Route 1 above 0.55. The alien shift in OFFERING's output (documented in B038) becomes too chaotic for OVERBITE to produce coherent bass response. Keep one or the other moderate.

---

### Ambient & Cinematic (3 Recipes)

---

#### Recipe 05 — "Journey Atomized"
*A temporal arc becomes a spatial scatter. Time-travel as sound design.*

| Field | Detail |
|-------|--------|
| **Name** | Journey Atomized |
| **Engine A** | ODYSSEY (XOdyssey) — Slot 1 |
| **Engine B** | OPAL (XOpal) — Slot 2 |
| **Route 1 Type** | `Env->Morph` |
| **Route 1 Amount** | 0.52 |
| **Route 2 Type** | `Amp->Filter` |
| **Route 2 Amount** | 0.32 |
| **Route 3 Type** | `LFO->Pitch` |
| **Route 3 Amount** | 0.20 |
| **Direction** | A→B |
| **Genre** | Ambient, cinematic, psychedelic |

**Why It Works:** ODYSSEY's Familiar-to-Alien journey envelope sweeps OPAL's grain scan position as the note evolves. A six-second sustain starts with OPAL drawing grains from one position in its source buffer — familiar, warm. By the midpoint of the note, ODYSSEY's envelope has moved OPAL's scan position halfway across the buffer — alien, scattered, from a different time in the source material. Add LFOToPitch from ODYSSEY's LFO and the grain pitches drift slowly, breathing with the pad. Three dimensions of granular variation from a single held note.

**Sweet Spot:** ODYSSEY ampAttack 0.12, ampRelease 2.0+ so the envelope moves slowly enough to hear the granular position change. OPAL grainSize 0.40–0.50 (medium grains respond more clearly to position changes than very small or very large grains).

**Danger Zone:** ODYSSEY journeyDepth above 0.85. At this setting ODYSSEY's own harmonic character becomes so alien that OPAL's filter (driven by ODYSSEY's amp via Route 2) swings between fully dark and fully bright too dramatically.

---

#### Recipe 06 — "Shore Flood"
*The ocean enters the building. Physical fluid dynamics driving a resonant room.*

| Field | Detail |
|-------|--------|
| **Name** | Shore Flood |
| **Engine A** | OSPREY (XOsprey) — Slot 1 |
| **Engine B** | OSTERIA (XOsteria) — Slot 2 |
| **Route 1 Type** | `Audio->FM` |
| **Route 1 Amount** | 0.48 |
| **Route 2 Type** | `Amp->Filter` |
| **Route 2 Amount** | 0.30 |
| **Direction** | A→B |
| **Genre** | Cinematic, sound design, ambient |

**Why It Works:** OSPREY models coastline physics — wave cycles, swell period, sea state. Its audio output is the sound of water at a specific shore. OSTERIA models a resonant room (the Porto wine tavern). AudioToFM injects OSPREY's coastal audio directly into OSTERIA's FM path — the ocean frequency-modulates the room's resonance. AmpToFilter opens and closes OSTERIA's filter with the wave amplitude. The result: a room whose walls literally vibrate with the sound of the ocean outside. Slow swell periods (OSPREY swellPeriod ~0.45) create long room breathing cycles. Fast swell creates flutter.

**Sweet Spot:** OSPREY seaState at 0.50–0.65 for moderate wave energy. OSTERIA roomSize at 0.60–0.70 for the room to have enough resonance to be clearly frequency-modulated. Route 1 at 0.40–0.55.

**Danger Zone:** OSPREY seaState above 0.80 with Route 1 above 0.60. The ocean overwhelms the room — you hear FM distortion rather than resonance.

---

#### Recipe 07 — "The Granular Tide"
*Two time scales — grain scatter and swell — locked in slow conversation.*

| Field | Detail |
|-------|--------|
| **Name** | The Granular Tide |
| **Engine A** | OPAL (XOpal) — Slot 1 |
| **Engine B** | OSPREY (XOsprey) — Slot 2 |
| **Route 1 Type** | `Amp->Filter` |
| **Route 1 Amount** | 0.22 |
| **Route 2 Type** | `Env->Morph` |
| **Route 2 Amount** | 0.18 |
| **Direction** | A→B |
| **Genre** | Ambient, meditative, cinematic |

**Why It Works:** Reversing the expected direction. OPAL's grain cloud amplitude and envelope drive OSPREY's sea state and swell period — the granular density becomes the tidal force. Louder grain clouds mean higher sea state (more turbulent water). As the grain envelope rises and falls over long notes, the swell period of OSPREY's coastal model changes. The water responds to the cloud above it. At Whisper amounts, this creates a subtle synchronized breathing between two entirely different physical models.

**Sweet Spot:** OPAL grainDensity at 0.35–0.50, lfoRate at 0.02–0.05 (very slow grain fluctuation). Route 1 at 0.18–0.25 — the tidal control is gentle. OSPREY swellPeriod at 0.55 so the wave cycles are slow enough to hear the coupling's effect.

**Danger Zone:** OPAL grainDensity fluctuation too fast (lfoRate above 0.15). OSPREY's physical model cannot respond quickly enough to rapid grain amplitude changes — the coupling becomes incoherent.

---

### Electronic & Techno (3 Recipes)

---

#### Recipe 08 — "Deterministic Chaos Groove"
*The drum pattern never repeats exactly. The attractor is the groove.*

| Field | Detail |
|-------|--------|
| **Name** | Deterministic Chaos Groove |
| **Engine A** | OUROBOROS (XOuroboros) — Slot 1 |
| **Engine B** | ONSET (XOnset) — Slot 2 |
| **Route 1 Type** | `Amp->Filter` |
| **Route 1 Amount** | 0.72 |
| **Route 2 Type** | `Env->Decay` |
| **Route 2 Amount** | 0.48 |
| **Route 3 Type** | `Rhythm->Blend` |
| **Route 3 Amount** | 0.38 |
| **Direction** | A→B |
| **Genre** | Techno, industrial, modular-adjacent |

**Why It Works:** OUROBOROS (strange attractor synth) runs continuously. Its amplitude orbits through the Lorenz or Aizawa attractor's phase space — mathematically deterministic but non-repeating on human timescales. This amplitude drives ONSET's filter character (kick tone changes subtly every bar), decay time (some kicks ring longer depending on where the attractor is), and rhythm blend (drum pattern density varies with attractor amplitude). The pattern sounds almost random but is reproducible exactly with the same initial attractor conditions. Deterministic chaos as groove. No two bars sound identical; no two performances sound identical.

**Sweet Spot:** OUROBOROS attractorType 0 (Lorenz) or 2 (Aizawa). chaosDepth 0.65–0.72. leashDepth 0.45–0.55 to keep the amplitude in a useful range. Route 1 at 0.65–0.75 for clear filter variation. Map Route 1 depth to CC3 for live chaos-to-filter control.

**Danger Zone:** OUROBOROS chaosDepth above 0.85. The attractor becomes too erratic; the drum filter swings between extremes without the rhythmic logic that makes this recipe musical.

---

#### Recipe 09 — "Chaos Partials"
*Mathematical inevitability rewrites the harmonic grid. Additive synthesis under attractor control.*

| Field | Detail |
|-------|--------|
| **Name** | Chaos Partials |
| **Engine A** | OUROBOROS (XOuroboros) — Slot 1 |
| **Engine B** | ORBITAL (XOrbital) — Slot 2 |
| **Route 1 Type** | `LFO->Pitch` |
| **Route 1 Amount** | 0.48 |
| **Route 2 Type** | `Audio->FM` |
| **Route 2 Amount** | 0.30 |
| **Direction** | A→B |
| **Genre** | Techno, IDM, experimental electronic |

**Why It Works:** OUROBOROS's attractor velocity derivatives (dx/dt, dy/dt from channels 2 and 3) drive ORBITAL's additive harmonic grid via LFOToPitch. The partial pitch grid is continuously pushed by the attractor's trajectory rather than sitting at static harmonic ratios. At low amounts, the partials have a slow non-repeating vibrato. At Dialogue amounts, each partial drifts to a different position in the harmonic spectrum over time. AudioToFM from OUROBOROS's main audio injects additional inharmonic content at the core. The result: additive synthesis that sounds alive, chaotic, and unpredictable — but not random, because the attractor is mathematically determined.

**Sweet Spot:** ORBITAL partials at 8–12. More partials = more pitch drift to hear. OUROBOROS orbitFreq at 1.5–2.0 for meaningful attractor velocity. Route 1 at 0.40–0.55.

**Danger Zone:** Route 1 above 0.65. ORBITAL's partials drift so far from harmonic relationships that the tone becomes untuned noise.

---

#### Recipe 10 — "Ring Neon"
*Crystalline transients shatter into sideband light. AudioToRing at its most precise.*

| Field | Detail |
|-------|--------|
| **Name** | Ring Neon |
| **Engine A** | ODDFELIX (OddfeliX) — Slot 1 |
| **Engine B** | ORBITAL (XOrbital) — Slot 2 |
| **Route 1 Type** | `Audio->Ring` |
| **Route 1 Amount** | 0.52 |
| **Route 2 Type** | `LFO->Pitch` |
| **Route 2 Amount** | 0.22 |
| **Direction** | A→B |
| **Genre** | Techno, IDM, electronic |

**Why It Works:** ODDFELIX (neon tetra — bright, crystalline, snappy) produces brief harmonic transients. AudioToRing ring-modulates these transients against ORBITAL's additive harmonic content. Each snap from ODDFELIX produces sum and difference sidebands across all of ORBITAL's partials simultaneously — an entire harmonic grid multiplied by a crystalline transient. The sidebands are musical (not random noise) because both sources are harmonic. LFOToPitch from ODDFELIX's LFO adds gentle pitch drift to ORBITAL between the ring-modulated hits.

**Sweet Spot:** ODDFELIX ampDecay at 0.20–0.30 (short enough to be a transient, long enough to produce meaningful sidebands). ORBITAL partials 6–8. Route 1 at 0.45–0.58 for clearly audible but not overwhelming sideband content.

**Danger Zone:** ODDFELIX filterCutoff above 0.85. At high cutoff the source becomes too bright and narrow — sidebands push into high-frequency noise territory.

---

### Lo-Fi (2 Recipes)

---

#### Recipe 11 — "Tape Erosion"
*Grain density determines how warm the echo is. The cloud controls the room.*

| Field | Detail |
|-------|--------|
| **Name** | Tape Erosion |
| **Engine A** | OPAL (XOpal) — Slot 1 |
| **Engine B** | OVERDUB (XOverdub) — Slot 2 |
| **Route 1 Type** | `Amp->Filter` |
| **Route 1 Amount** | 0.68 |
| **Route 2 Type** | `Rhythm->Blend` |
| **Route 2 Amount** | 0.45 |
| **Direction** | A→B |
| **Genre** | Lo-fi, chillhop, tape aesthetic |

**Why It Works:** OPAL grain density drives OVERDUB's tape delay filter cutoff (a denser cloud = brighter delay) and its delay mix (more grain activity = wetter delay). The delay doesn't run at a fixed brightness — it responds to the granular texture above it. When grains thin out, the tape becomes darker and drier. When grains accumulate, the tape brightens and the echo deepens. This creates a lo-fi texture that feels organic rather than produced — the delay seems to be reacting to the music rather than just repeating it.

**Sweet Spot:** OPAL lfoRate at 0.015–0.025 (very slow grain density fluctuation). OVERDUB delayFeedback at 0.42–0.55 (moderate feedback so the brightness variation is audible in the repeats). Route 1 at 0.60–0.72.

**Danger Zone:** Route 1 above 0.80. The delay filter swings between fully dark (grains quiet) and fully bright (grains loud) too dramatically — the tape character becomes uneven rather than worn.

---

#### Recipe 12 — "Felt and Fog"
*The worn surface of a muted piano in a room full of morning light.*

| Field | Detail |
|-------|--------|
| **Name** | Felt and Fog |
| **Engine A** | OVERWORN (XOverworn) — Slot 1 |
| **Engine B** | OVERCAST (XOvercast) — Slot 2 |
| **Route 1 Type** | `Amp->Filter` |
| **Route 1 Amount** | 0.25 |
| **Route 2 Type** | `Env->Morph` |
| **Route 2 Amount** | 0.20 |
| **Direction** | A→B |
| **Genre** | Lo-fi, bedroom pop, jazz-adjacent |

**Why It Works:** OVERWORN models worn felt and aged material degradation — its character is warm, muffled, slightly inconsistent. OVERCAST models atmospheric cloud density and light diffusion. At Whisper coupling amounts, OVERWORN's amplitude gently opens OVERCAST's filter (as if morning light brightening through clouds), and OVERWORN's envelope sweeps OVERCAST's morph parameter (atmospheric density shifting). The result is a textural coupling that sounds like weather responding to temperature — slow, organic, never mechanical.

**Sweet Spot:** Both engines at long attack (0.15+) and long release (2.0+). Route 1 at 0.20–0.28. The coupling should be barely perceptible — present as a quality, not an effect.

**Danger Zone:** OVERWORN feltAge above 0.85 with Route 2 above 0.35. The morph sweep becomes too wide and OVERCAST loses its atmospheric coherence.

---

### Experimental (2 Recipes)

---

#### Recipe 13 — "Prophecy Fed"
*Ancient probability mathematics feeding a living organism. No two notes sound alike.*

| Field | Detail |
|-------|--------|
| **Name** | Prophecy Fed |
| **Engine A** | ORACLE (XOracle) — Slot 1 |
| **Engine B** | ORGANON (XOrganon) — Slot 2 |
| **Route 1 Type** | `Audio->FM` |
| **Route 1 Amount** | 0.50 |
| **Route 2 Type** | `Amp->Filter` |
| **Route 2 Amount** | 0.30 |
| **Route 3 Type** | `Env->Morph` |
| **Route 3 Amount** | 0.22 |
| **Direction** | A→B |
| **Genre** | Experimental, drone, algorithmic |

**Why It Works:** ORACLE's GENDY stochastic synthesis (B010) generates waveforms through ancient probability mathematics — Xenakis's stochastic synthesis formulas. This mathematically unpredictable audio enters ORGANON's per-voice ingestion buffer via AudioToFM. ORGANON (Variational Free Energy organism — B011) metabolizes what it receives, adapting its modal array over time. Hold a note for 10 seconds. In the first second, ORGANON sounds like itself. By the fourth second, it has been partially shaped by ORACLE's probabilistic output. By the eighth second, you are hearing an organism that has eaten the prophet's voice. Release the note and hold a new one — ORGANON's character has been permanently altered by the ingestion.

**Sweet Spot:** ORACLE breakpoints at 8–12, probability at 0.55–0.70 (moderate unpredictability). ORGANON metabolicRate at 2.0–3.0 (slow enough to hear the adaptation, fast enough to respond within a single note).

**Danger Zone:** ORACLE probability above 0.85. At this setting the stochastic output is so random that ORGANON's FM input is essentially white noise — the organism adapts to noise rather than to a probabilistic signal.

---

#### Recipe 14 — "The Knot — Ocean Creature"
*Two engines that breathe as one organism. Removing one breaks both.*

| Field | Detail |
|-------|--------|
| **Name** | The Knot — Ocean Creature |
| **Engine A** | OCEANIC (XOceanic) — Slot 1 |
| **Engine B** | OCTOPUS (XOctopus) — Slot 2 |
| **Route 1 Type** | `KnotTopology` |
| **Route 1 Amount** | 0.75 (linking number 4) |
| **Direction** | Bidirectional |
| **Genre** | Experimental, sound art, ambient |

**Why It Works:** OCEANIC (bioluminescent pad — phosphorescent teal, slow swell) and OCTOPUS (decentralized alien intelligence — eight arms, chromatophore spread) share a KnotTopology bond at linking number 4. Both engines modulate each other's filter and character in the same processing pass. Neither leads. The chromatophore spread of OCTOPUS affects OCEANIC's separation. OCEANIC's phosphorescent swell affects OCTOPUS's arm activity. After 4–8 bars of held notes, the two engines find a mutual state — a resonance that neither could achieve alone. This is the only coupling type that creates a genuinely irreducible sonic identity.

**Sweet Spot:** Hold a sustained chord for 8+ bars without changing note. Let the system find its mutual resonance. After bar 4, the entanglement reaches linking number 4's full character.

**Danger Zone:** Depth at 1.00 (linking number 5). At maximum entanglement, the two engines are inseparable — removing one from the slot leaves the other sounding tuned to a relationship that no longer exists. Use deliberate choice.

---

### Neo-Soul & R&B (2 Recipes)

---

#### Recipe 15 — "Love Circuit"
*Sternberg's triangle embedded in synthesis. Intimacy accumulates.*

| Field | Detail |
|-------|--------|
| **Name** | Love Circuit |
| **Engine A** | OXYTOCIN (XOxytocin) — Slot 1 |
| **Engine B** | OWARE (XOware) — Slot 2 |
| **Route 1 Type** | `TriangularCoupling` |
| **Route 1 Amount** | 0.35 |
| **Route 2 Type** | `Amp->Filter` |
| **Route 2 Amount** | 0.22 |
| **Direction** | A→B |
| **Genre** | Neo-soul, R&B, ambient soul |

**Why It Works:** OXYTOCIN (circuit-modeling × Sternberg's love theory — B040) accumulates intimacy over held notes. Long notes unlock timbral states unavailable to short ones. TriangularCoupling bleeds OXYTOCIN's effective Intimacy, Passion, and Commitment values into OWARE's material physics. As OXYTOCIN's intimacy accumulates on a long sustained chord, OWARE's mallet material shifts — wood becoming warmer, the sympathetic resonance network activating more fully. The R&B chord held for eight beats produces a physically richer percussion sound than the same chord cut short. Emotional duration as synthesis parameter.

**Sweet Spot:** OXYTOCIN oxy_intimacy at 0.5–0.7 base level. Hold notes for 6–10 seconds. OWARE owr_material at 0.3 (wood side of the continuum) so there is room for warmth to increase.

**Danger Zone:** Route 1 above 0.55 with short note lengths. TriangularCoupling requires sustained notes to build effective Intimacy values. Short staccato playing produces no meaningful coupling output.

---

#### Recipe 16 — "Pitch Harmony Live"
*One keyboard. Two voices. The coupling is the harmony.*

| Field | Detail |
|-------|--------|
| **Name** | Pitch Harmony Live |
| **Engine A** | ODYSSEY (XOdyssey) — Slot 1 |
| **Engine B** | OBLONG (XOblong) — Slot 2 |
| **Route 1 Type** | `Pitch->Pitch` |
| **Route 1 Amount** | 0.50 |
| **Route 2 Type** | `Amp->Filter` |
| **Route 2 Amount** | 0.25 |
| **Direction** | A→B |
| **Genre** | R&B, neo-soul, gospel |

**Why It Works:** ODYSSEY's pitch drives OBLONG's pitch via PitchToPitch. Play a single key on ODYSSEY and OBLONG harmonizes in real time — not through chord memory, but through live pitch coupling. The COUPLING macro maps to Route 1 depth: at 0.0, both engines play the same note (unison); at 0.50, they harmonize at a fifth or fourth; at 1.0, they separate by an octave. AmpToFilter adds subtle filter breathing to OBLONG, so when ODYSSEY plays loud, OBLONG's harmonic brightens. The coupling system as a harmony generator: no MIDI splitting, no chord machine, just two engines sharing pitch information.

**Sweet Spot:** Map COUPLING macro to Route 1 depth. OBLONG bob_transpose at 12 (up one octave) as baseline, then use coupling amount to fine-tune the interval. OBLONG glide at 0.05 for smooth pitch coupling response.

**Danger Zone:** ODYSSEY pitchBend while Route 1 is active at high amounts. Pitch bending the source engine bends both engines' pitch — which can be a feature but requires awareness.

---

### Drum & Bass (2 Recipes)

---

#### Recipe 17 — "Reese Feedback"
*Two oscillator engines sharing pitch identity. The Reese bass as a coupled organism.*

| Field | Detail |
|-------|--------|
| **Name** | Reese Feedback |
| **Engine A** | OBLONG (XOblong) — Slot 1 |
| **Engine B** | OMEGA (XOmega) — Slot 2 |
| **Route 1 Type** | `LFO->Pitch` |
| **Route 1 Amount** | 0.22 |
| **Route 2 Type** | `Filter->Filter` |
| **Route 2 Amount** | 0.30 |
| **Direction** | A→B |
| **Genre** | Drum & bass, liquid DnB |

**Why It Works:** The classic Reese bass sound is defined by a slowly detuned second oscillator creating beating interference. Here, OBLONG's LFO signal modulates OMEGA's pitch via LFOToPitch — a second voice that is not quite tracking the fundamental, producing the characteristic DnB bass movement. FilterToFilter cascades OBLONG's filter output into OMEGA's filter input, so the resonance of the first voice colors the second voice's frequency response. The two engines, playing the same MIDI note, produce the spatial, moving Reese character through coupling rather than internal detuning.

**Sweet Spot:** OBLONG lfoRate at 0.05–0.10 (slow LFO for gentle beating). Route 1 at 0.18–0.28. Route 2 at 0.25–0.35. OMEGA filterReso at 0.35–0.50 so the cascaded filter character is audible.

**Danger Zone:** Route 2 (FilterToFilter) above 0.50 with high resonance on both engines. Two resonant filters in cascade can create self-oscillation peaks. Keep total resonance moderate.

---

#### Recipe 18 — "Spectral Chain"
*Physical resonance → spectral folding. Two physically-inspired engines in series.*

| Field | Detail |
|-------|--------|
| **Name** | Spectral Chain |
| **Engine A** | OBSCURA (XObscura) — Slot 1 |
| **Engine B** | ORIGAMI (XOrigami) — Slot 2 |
| **Route 1 Type** | `Audio->FM` |
| **Route 1 Amount** | 0.50 |
| **Route 2 Type** | `Amp->Filter` |
| **Route 2 Amount** | 0.28 |
| **Route 3 Type** | `Env->Morph` |
| **Route 3 Amount** | 0.22 |
| **Direction** | A→B |
| **Genre** | Drum & bass, neurofunk, cinematic DnB |

**Why It Works:** OBSCURA models physical spring chain behavior (Daguerreotype Silver — stiffness, damping, nonlinearity). Its output is a physically complex resonance spectrum. This enters ORIGAMI's STFT fold via AudioToFM — a spectral refolding engine processing a physically-modeled input. The physical resonance of OBSCURA becomes the frequency-modulation source for ORIGAMI's geometric fold. AmpToFilter opens ORIGAMI's filter with OBSCURA's amplitude envelope, and EnvToMorph moves ORIGAMI's fold point as the physical model's envelope evolves. The result: DnB textures that sound like metal and geometry simultaneously.

**Sweet Spot:** OBSCURA stiffness at 0.60–0.72. ORIGAMI foldDepth at 0.50–0.65 for a dramatic fold response to the FM input. Route 1 at 0.45–0.55.

**Danger Zone:** ORIGAMI foldDepth above 0.80 with Route 1 above 0.65. Spectral fold at maximum depth + strong FM input produces extreme harmonic distortion that loses pitch reference entirely.

---

### Wild Cards (2 Recipes)

---

#### Recipe 19 — "The Photon Harvest"
*OPTIC has no sound. But it can rewrite what everything else sounds like.*

| Field | Detail |
|-------|--------|
| **Name** | The Photon Harvest |
| **Engine A** | OPTIC (XOptic) — Slot 1 (zero mix — no audio contribution) |
| **Engine B** | ORGANON (XOrganon) — Slot 2 |
| **Route 1 Type** | `Amp->Filter` |
| **Route 1 Amount** | 0.55 |
| **Route 2 Type** | `LFO->Pitch` |
| **Route 2 Amount** | 0.35 |
| **Route 3 Type** | `Filter->Filter` |
| **Route 3 Amount** | 0.28 |
| **Direction** | A→B |
| **Genre** | All genres (OPTIC is a universal modulation source) |

**Why It Works:** OPTIC (B005 — Zero-Audio Identity, Phosphor Green) is a zero-audio engine. It produces no sound, only modulation signals derived from its analysis input (which can be any other engine's audio via the internal routing, an external input, or its own self-generated reference signal). Set OPTIC's mix contribution to 0.0 — it is invisible in the mix. Its three coupling routes drive ORGANON's filter, pitch, and cascaded filter character using OPTIC's eight independent analysis channels: composite envelope, pulse, bass, mid, high, spectral centroid, flux, energy, transient. ORGANON's biological behavior is now responding to spectral analysis data rather than raw audio. The organism is fed photons.

**Sweet Spot:** OPTIC optic_pulseRate at 1.0–4.0 Hz for rhythmic modulation. Map OPTIC's analysis mode to a macro to switch between tonal and transient analysis inputs mid-performance.

**Danger Zone:** OPTIC as source for AudioToFM. OPTIC's modulation signals are control-rate by design — using them as audio-rate FM creates digital hash rather than musical frequency modulation. Use AudioToFM only with audio-producing engines.

---

#### Recipe 20 — "Cellular Bass Drum"
*A living rule system generates a rhythm that no human programmed. Cellular automata as percussion.*

| Field | Detail |
|-------|--------|
| **Name** | Cellular Bass Drum |
| **Engine A** | ORGANISM (XOrganism) — Slot 1 |
| **Engine B** | OFFERING (XOffering) — Slot 2 |
| **Route 1 Type** | `Amp->Filter` |
| **Route 1 Amount** | 0.48 |
| **Route 2 Type** | `Rhythm->Blend` |
| **Route 2 Amount** | 0.42 |
| **Route 3 Type** | `Env->Decay` |
| **Route 3 Amount** | 0.18 |
| **Direction** | A→B |
| **Genre** | All genres — experimental percussion programming |

**Why It Works:** ORGANISM (Emergence Lime — cellular automata generative engine) runs a living rule system that generates emergent amplitude patterns. These patterns, unlike an LFO, have irregular spacings and sudden amplitude clusters that emerge from cellular complexity. OFFERING (psychology-driven boom bap engine) receives these patterns: AmpToFilter opens the drum's filter in response to cellular amplitude clusters (sudden drum tone brightness), RhythmToBlend varies the drum synthesis blend (cellular clusters create denser drum hits), and EnvToDecay extends decay on high-amplitude cellular bursts (long tail on cluster peaks). The drum pattern is not programmed — it emerges from ORGANISM's rule system. Add a simple MIDI loop triggering OFFERING and let ORGANISM reshape what each hit sounds like.

**Sweet Spot:** ORGANISM ruleSet at B1 or B3 (generative but not too chaotic). Route 1 at 0.40–0.55. OFFERING curiosity at 0.45–0.60 so the drum itself has some intrinsic variation that ORGANISM's coupling can amplify.

**Danger Zone:** ORGANISM ruleSet above B5 with Route 1 at 0.65+. The cellular amplitude pattern becomes too erratic for OFFERING to produce coherent drum character. The coupling turns the percussion into noise rather than evolving rhythm.

---

## Part IV — Quick Reference

### Coupling Type Summary Table

| # | Type | Rate | Best Source | Best Target | Amount Sweet Spot |
|---|------|------|-------------|-------------|-------------------|
| 1 | `Amp->Filter` | Control | Any | Any (universal) | 0.25–0.55 |
| 2 | `Amp->Pitch` | Control | Percussive, chaotic | Melodic, pad | 0.10–0.30 |
| 3 | `LFO->Pitch` | Control | Pad, melodic | Melodic, pad | 0.20–0.45 |
| 4 | `Env->Morph` | Control | Pad, slow envelope | OPAL, wavetable | 0.30–0.55 |
| 5 | `Env->Decay` | Control | Percussive | Bass, pad | 0.12–0.40 |
| 6 | `Amp->Choke` | Control | Percussive | Percussive | 0.40–0.65 |
| 7 | `Filter->Filter` | Control | OPTIC, any | Any with resonance | 0.15–0.40 |
| 8 | `Pitch->Pitch` | Control | Melodic | Melodic | 0.35–0.65 |
| 9 | `Rhythm->Blend` | Control | OUROBOROS, percussive | OPAL, OVERDUB | 0.30–0.55 |
| 10 | `Audio->FM` | Audio | ORACLE, OBSCURA | ORGANON, OVERBITE | 0.30–0.55 |
| 11 | `Audio->Ring` | Audio | OddfeliX, harmonic | ORBITAL | 0.40–0.60 |
| 12 | `Audio->Wavetable` | Audio | ORBITAL, OVERWORLD | OPAL, ORIGAMI | 0.40–0.60 |
| 13 | `Audio->Buffer` | Audio | Any | OPAL, ORGANON | 0.35–0.55 |
| 14 | `KnotTopology` | Control | Any | Any (bidirectional) | 0.50–0.78 |
| 15 | `TriangularCoupling` | Control | OXYTOCIN | OWARE, OXYTOCIN | 0.20–0.40 |

---

### Engine Coupling Character — Quick Reference

**Best source engines (strong coupling output):**
- OUROBOROS — continuous chaotic amplitude, 4 output channels (B007)
- ONSET — strong transient output, XVC cross-voice coupling (B002)
- OPTIC — zero-audio precision modulation engine (B005)
- ORACLE — stochastic audio, authentic randomness (B010)
- ORGANISM — cellular automata amplitude patterns
- OBBLIGATO — breath-rate LFO, natural fluctuation

**Best target engines (rich coupling response):**
- OPAL — responds to all 15 coupling types with distinct granular behavior
- ORGANON — biological integration of FM input over time (B011)
- OVERBITE — strong character chain responds to filter, FM, decay (B008)
- ORBITAL — additive grid responds richly to pitch and audio-rate coupling
- OWARE — material continuum responds to Env->Morph with physical character changes (B032-034)
- ODYSSEY — journey arc responds to envelope coupling across long notes

**Best bidirectional pairs (KnotTopology):**
- OCEANIC + OCTOPUS (bioluminescent + chromatophore — complementary alien biology)
- OUROBOROS + ORBITAL (chaotic + harmonic — productive tension)
- OVERDUB + ORGANON (echo + organism — mutual sustain resonance)
- OXBOW + OVERFLOW (entangled reverb + deep current — complementary water physics)

---

### The Three Golden Rules

1. **Start at 0.10, not 0.50.** Coupling at Whisper depth is often the most musical result. Only push to Possession when you want the coupling to define the sound, not support it.

2. **Fix engine parameters, not coupling amount.** If the coupling sounds wrong, adjust the source engine's output character first. Coupling amount controls intensity; engine parameters control quality.

3. **One new coupling type per session.** Don't load four audio-rate routes simultaneously and wonder why the result is chaos. Learn one type at a time. `Amp->Filter` first. Then `Env->Morph`. Then `Audio->FM`. The fleet is 76 engines deep — there is no rush.

---

## Closing Note

The recipes in this cookbook are starting points. Each one demonstrates a specific coupling relationship with specific engines at specific amounts. What they cannot demonstrate is what happens when you take any two engines from a fleet of 76 and connect them with any of 15 coupling types at any amount in either direction.

That space — 76 × 75 × 15 × the full 0.0–1.0 depth range — is the actual size of the coupling system. This cookbook covers 20 points in that space. The rest is yours.

Start with `Amp->Filter` at 0.30 and two engines you already know. Find the space between them. Then raise the depth.

---

*XOceanus — for all*
*XO_OX Designs*
*2026-03-24*
