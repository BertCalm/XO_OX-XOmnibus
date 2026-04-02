# XOceanus Coupling Preset Library
**Date:** 2026-03-14
**Author:** XO_OX Designs
**Status:** v1.0 — 18 presets across 6 engine pairs

---

## Overview

Cross-engine coupling is XOceanus's most powerful differentiator. Most factory presets use a single engine or layer two engines passively. This library is the first systematic collection devoted entirely to demonstrating what coupling *sounds like* at three distinct intensities — subtle, equal partnership, and full domination.

Each pair was chosen for a combination of three criteria:

1. **Coupling quality score** — both engines have working `applyCouplingInput` implementations (score ≥ 3 per the coupling audit)
2. **Musical coherence** — the source engine's output is a *meaningful* type of input for the target engine
3. **Narrative resonance** — the pair has a story: mythology, sonic metaphor, or creative tension

---

## The 6 Pairs

### Pair 1: ONSET → OVERBITE (`Amp->Filter` + `Audio->FM` + `Env->Decay`)

**Why this pair:** ONSET has the fleet's most powerful percussion synthesis (XVC system, Blessing B002). OVERBITE has the fleet's richest character chain (Belly/Bite/Scurry/Trash macros, Blessing B008). When drum transients drive bass filter and FM depth, the result is a sidechain-coupled bass that *breathes* with the rhythm rather than merely ducking under it.

**Coupling types used:** `Amp->Filter` (OVERBITE handles it), `Audio->FM` (OVERBITE handles it), `Env->Decay` (OVERBITE handles it). All three are wired and confirmed in the coupling audit.

**Mythology:** Drum teaches the predator how to move. Percussion as animal behavior trigger — the snare snaps the bite instinct awake.

---

### Pair 2: OPAL → OVERDUB (`Amp->Filter` + `Rhythm->Blend`)

**Why this pair:** OPAL's granular engine outputs amplitude envelopes and rhythm-like grain density fluctuations. OVERDUB (XOverdub) handles `Amp->Filter` (inverted/ducking) and `AmpToPitch`. When grain density shapes the dub filter, the echo chain becomes a living organism — the tape delay responds to the particle cloud rather than a fixed gate.

**Coupling types used:** `Amp->Filter` (OVERDUB handles as inverted duck), `Rhythm->Blend` (repurposed as blend modulation). Both confirmed in coupling audit.

**Mythology:** Grains dissolving into echo. The granular cloud is not the sound — it is the *instruction* for the echo to follow.

---

### Pair 3: ODYSSEY → OPAL (`Env->Morph` + `Amp->Filter` + `LFO->Pitch`)

**Why this pair:** ODYSSEY (XOdyssey) is explicitly designed around the Familiar→Alien journey. OPAL's grain position (`Env->Morph`) is its core timbral axis — a scan position through an internal wavetable/source buffer. When DRIFT's journey envelope sweeps Opal's morph, the familiar pad unfolds in time-scattered fragments. The psychedelic surface of DRIFT becomes the atomic structure of OPAL.

**Coupling types used:** `Env->Morph` (OPAL handles it — sets grain scan position), `Amp->Filter` (OPAL handles), `LFO->Pitch` (OPAL handles). All confirmed.

**Mythology:** The Familiar→Alien journey atomized. Each grain is a moment in the journey, scattered across time. From the XO_OX aquatic mythology: DRIFT lives at the photic zone boundary where familiar light meets alien deep — OPAL scatters those photons into particles.

---

### Pair 4: OVERWORLD → OPAL (`Env->Morph` + `Amp->Filter` + `Audio->FM`)

**Why this pair:** OVERWORLD (XOverworld) outputs chip synthesis — NES 2A03, Sega YM2612, SNES SPC700. OPAL's `AudioToWavetable` coupling uses the source audio as a grain buffer. The chip's tonal palette — pulse waves, FM operators, Gaussian-interpolated BRR samples — becomes the raw material that Opal atomizes. This is explicitly listed as Priority #4 in the coupling audit's Priority Wiring Queue.

**Coupling types used:** `Env->Morph` (grain position), `Amp->Filter` (grain filter), `Audio->FM` (OPAL handles as wavetable source injection). The `Audio->FM` coupling in Opal is handled via a buffer copy mechanism confirmed in the audit.

**Mythology:** The console dissolved into time. OVERWORLD's ERA crossfade maps exactly to OPAL's scatter — chip synthesis is the source material, granular synthesis is the microscope.

---

### Pair 5: ORACLE → ORGANON (`Audio->FM` + `Amp->Filter` + `Env->Morph`)

**Why this pair:** ORACLE generates waveforms through genuine stochastic processes (GENDY algorithm). ORGANON has the fleet's deepest coupling reception — per-voice ingestion buffers that consume audio as metabolic input (Blessing B011, score 5). When prophecy feeds metabolism, the organism's harmonic evolution becomes genuinely random-walk-driven rather than state-determined. No two notes are alike.

**Coupling types used:** `Audio->FM` (ORGANON's per-voice ingestion buffer), `Amp->Filter` (ORGANON handles), `Env->Morph` (ORGANON handles morph). All confirmed score-5 implementations.

**Mythology:** The organism eats the prophet's words. From aquatic mythology: ORGANON is the chemotroph; ORACLE is the volcanic vent. The organism was always eating the vent's chaotic output — this preset makes it audible.

---

### Pair 6: OUROBOROS → ONSET (`Amp->Filter` + `Env->Decay` + `Rhythm->Blend`)

**Why this pair:** OUROBOROS (score 5, "chaos source pattern") exposes 4 output channels — including ch2 and ch3 as attractor velocity derivatives (dx/dt, dy/dt). ONSET's `applyCouplingInput` handles `AmpToFilter`, `EnvToDecay`, `RhythmToBlend`, and `AmpToChoke`. When a strange attractor's dynamics modulate drum filter, decay time, and rhythm blend, the percussion becomes *deterministically chaotic* — repeating over cosmic timescales, never quite the same on any human-perceivable scale.

**Coupling types used:** `Amp->Filter` (ONSET handles), `Env->Decay` (ONSET handles), `Rhythm->Blend` (ONSET handles). All confirmed; ONSET's coupling input is score 4 in the audit.

**Mythology:** The orbit collapses into the drum. OUROBOROS is the self-eating serpent; when it swallows ONSET, the percussion becomes the serpent's heartbeat.

---

## Preset Table

### Pair 1: ONSET → OVERBITE
Receiving engine: **Overbite**. Directory: `Presets/XOceanus/Entangled/Onset-Overbite/`

| Preset Name | Intensity | Coupling Amount(s) | Musical Effect |
|-------------|-----------|-------------------|----------------|
| Nudge and Growl | Whisper | `Amp->Filter` 0.20 | Each kick barely opens Bite's filter — the bass breathes but the drum is not loud in the mix |
| The Drum Teaches Bass | Dialogue | `Amp->Filter` 0.50, `Env->Decay` 0.20 | Kick and snare co-sculpt the bass character; each hit reshapes the filter and extends decay |
| Percussion Is Character | Possession | `Amp->Filter` 0.80, `Audio->FM` 0.45, `Env->Decay` 0.30 | Drum transients fully rewrite Bite's FM depth and filter on every hit — the percussion is the sound design |

### Pair 2: OPAL → OVERDUB
Receiving engine: **Overdub**. Directory: `Presets/XOceanus/Entangled/Opal-Overdub/`

| Preset Name | Intensity | Coupling Amount(s) | Musical Effect |
|-------------|-----------|-------------------|----------------|
| Dust in the Echo | Whisper | `Amp->Filter` 0.18 | Sparse grain cloud barely stirs the dub filter — shimmer at the edge of reverb tail |
| Grain Meets Tape | Dialogue | `Amp->Filter` 0.50, `Rhythm->Blend` 0.25 | Grain density and dub delay are co-equal; particle rhythm shapes echo blend |
| Scattered Signal | Possession | `Amp->Filter` 0.75, `Amp->Pitch` 0.30, `Rhythm->Blend` 0.45 | Dense grain storm seizes filter, pitch, and delay blend — the dub engine is a granular instrument |

### Pair 3: ODYSSEY → OPAL
Receiving engine: **Opal**. Directory: `Presets/XOceanus/Entangled/Odyssey-Opal/`

| Preset Name | Intensity | Coupling Amount(s) | Musical Effect |
|-------------|-----------|-------------------|----------------|
| Familiar Shimmer | Whisper | `Env->Morph` 0.22 | Drift's envelope gently shifts Opal's scan position — cloud breathes with the pad |
| Alien Fragments | Dialogue | `Env->Morph` 0.50, `Amp->Filter` 0.30 | Pad envelope and filter co-shape grain position and cutoff — journey and scatter in balance |
| Journey Atomized | Possession | `Env->Morph` 0.80, `Amp->Filter` 0.55, `LFO->Pitch` 0.35 | Drift fully decomposes into Opal — LFO scatters grain pitch, envelope seizes morph |

### Pair 4: OVERWORLD → OPAL
Receiving engine: **Opal**. Directory: `Presets/XOceanus/Entangled/Overworld-Opal/`

| Preset Name | Intensity | Coupling Amount(s) | Musical Effect |
|-------------|-----------|-------------------|----------------|
| Pixel Whisper | Whisper | `Env->Morph` 0.22 | NES pulse energy barely nudges grain scan — pixelated flicker in the cloud |
| Console Cloud | Dialogue | `Env->Morph` 0.48, `Amp->Filter` 0.30 | Chip envelope and filter equally sculpt the granular morph — retro synthesis atomized |
| 8bit Dissolution | Possession | `Audio->FM` 0.70, `Env->Morph` 0.55, `Amp->Filter` 0.40 | FM chip audio injected as grain source; chip envelope seizes morph and filter entirely |

### Pair 5: ORACLE → ORGANON
Receiving engine: **Organon**. Directory: `Presets/XOceanus/Entangled/Oracle-Organon/`

| Preset Name | Intensity | Coupling Amount(s) | Musical Effect |
|-------------|-----------|-------------------|----------------|
| Ancient Murmur | Whisper | `Audio->FM` 0.20 | GENDY audio barely disturbs metabolic input — prophecy whispered to a living organism |
| Prophecy Fed | Dialogue | `Audio->FM` 0.50, `Amp->Filter` 0.30 | Stochastic waveform and barrier envelope co-shape Organon's filter and FM — organism digests prophecy |
| Chaos Ingested | Possession | `Audio->FM` 0.80, `Amp->Filter` 0.55, `Env->Morph` 0.45 | GENDY fully replaces metabolic input; envelope seizes filter and morph — organism now speaks in prophecy |

### Pair 6: OUROBOROS → ONSET
Receiving engine: **Onset**. Directory: `Presets/XOceanus/Entangled/Ouroboros-Onset/`

| Preset Name | Intensity | Coupling Amount(s) | Musical Effect |
|-------------|-----------|-------------------|----------------|
| Attractor Tap | Whisper | `Amp->Filter` 0.18 | Chaotic amplitude barely modulates drum decay — trembling hand on snare |
| Strange Groove | Dialogue | `Amp->Filter` 0.48, `Env->Decay` 0.25 | Lorenz attractor and Onset co-shape drum filter and decay — deterministic chaos as groove |
| Orbit Collapse | Possession | `Amp->Filter` 0.78, `Env->Decay` 0.45, `Rhythm->Blend` 0.40 | Aizawa attractor fully possesses Onset — drum pattern becomes attractor orbit made audible |

---

## Intensity Scale

| Label | Coupling Amount Range | Character |
|-------|----------------------|-----------|
| **Whisper** | 0.15–0.25 | Source influences target at the edge of perception. The receiving engine sounds nearly like itself — only a trained ear detects the source. Best for blend and layering contexts. |
| **Dialogue** | 0.45–0.55 | Source and target are equal creative partners. Neither dominates. The relationship is audible and musical but not extreme. Best for most production contexts. |
| **Possession** | 0.75–0.85 | Source engine dominates target behavior. The target no longer sounds like itself in isolation. Best for performance/modulation contexts where the coupling IS the sound. |

---

## Coupling Ecosystem

This section describes the 6 pairs as a *network* of musical relationships — how engines feed, respond to, and transform each other when placed together.

### The Rhythm-Timbre Axis: ONSET ↔ OVERBITE

ONSET and OVERBITE form the most *physically immediate* coupling in the library. Percussion and bass have always been linked in music theory (the kick and bass sharing a frequency region, playing together or in opposition). XOceanus makes this structural link *literally audible* — drum amplitude rewires bass character in real time. At Whisper intensity this is a sidechain; at Possession intensity it is a new synthesis paradigm where the drum IS the bass sound design.

This pair benefits most from a sequencer with a kick on beats 1 and 3. Without rhythm, the coupling is invisible.

### The Texture-Echo Axis: OPAL ↔ OVERDUB

OPAL and OVERDUB share a common ancestor in dub music: granular processing evolved from tape manipulation; dub delay and reverb evolved from studio tape tricks. When Opal's grain cloud shapes Dub's filter, you are connecting two different interpretations of the same lineage. At Whisper intensity the result is ambient — clouds hanging over an echo room. At Possession intensity it is a new dub synthesis where the granular engine has replaced the DJ's hand on the mixer.

This pair benefits from sparse harmonic content. Dense chords overwhelm the coupling's subtlety.

### The Journey-Atomization Axis: ODYSSEY ↔ OPAL

ODYSSEY's Familiar→Alien journey produces long, evolving envelopes and LFO depths designed to traverse psychological distance over time. OPAL's grain scan position is a different kind of journey — spatial rather than emotional. When DRIFT's temporal journey drives OPAL's spatial scan, you get a sound that is simultaneously unfolding in time (the pad evolving) and scattered across time (the grains displaced). The coupling maps two kinds of journey onto each other.

This pair sounds best in pad contexts — slow attacks, long releases, generous reverb. The coupling is most audible on held notes.

### The Archaeology Axis: OVERWORLD ↔ OPAL

OVERWORLD is the only engine in the fleet that models specific historical hardware — three distinct consoles with exact frequency tables and signal-processing characteristics. OPAL's granular engine makes fragments of any source. The OVERWORLD→OPAL coupling is *historical deconstruction*: you take 1985 NES audio and scatter it into the future. At Whisper intensity, the chip sound retains its identity but shimmers. At Possession intensity, the chip is unrecognizable — only the harmonic content survives in the particle cloud.

This pair has the most literal archaeological narrative in the library: digging up the past and dispersing it.

### The Ancient-Living Axis: ORACLE ↔ ORGANON

ORACLE generates waveforms through genuine stochastic processes derived from Xenakis's GENDY algorithm — the most mathematically ancient synthesis technique in the fleet. ORGANON is based on Friston's Variational Free Energy principle — the most biologically sophisticated model in the fleet. Ancient probability mathematics feeding a living organism's metabolism is the deepest intellectual coupling in XOceanus.

The musical result is genuinely unpredictable: even with identical parameters, no two notes sound the same because ORACLE's randomness feeds ORGANON's adaptation. This pair rewards long sustain contexts — let the organism digest the prophecy for several seconds before releasing the key.

### The Deterministic-Chaos Axis: OUROBOROS ↔ ONSET

OUROBOROS is deterministic but chaotic — given the same initial conditions it produces identical output, but the attractor's sensitivity to state means that any tiny perturbation produces completely different behavior over time. ONSET is human-rhythmic — kick, snare, hat, patterns. When the deterministic chaos modulates the human pattern, you get percussion that *sounds* random but is actually mathematically determined. At Possession intensity, the drum pattern is no longer a grid — it is a chaotic orbit that never quite repeats.

This pair is the most performative in the library. The `Rhythm->Blend` coupling type routes attractor amplitude directly into ONSET's rhythm processing, creating beat patterns that change continuously without requiring automation.

---

## Coupling Topology Notes

The 6 pairs cover 4 of the 12 canonical coupling types:

| Type | Pairs Using It |
|------|----------------|
| `Amp->Filter` | All 6 pairs |
| `Env->Morph` | Odyssey→Opal, Overworld→Opal, Oracle→Organon |
| `Audio->FM` | Overworld→Opal, Oracle→Organon |
| `Env->Decay` | Onset→Overbite, Ouroboros→Onset |
| `Rhythm->Blend` | Opal→Overdub, Ouroboros→Onset |
| `Amp->Pitch` | Opal→Overdub (Possession only) |
| `LFO->Pitch` | Odyssey→Opal (Possession only) |

`Amp->Filter` is the foundational coupling type — present in all 6 pairs because amplitude is the most universally available modulation source and filter cutoff is the most universally responsive target. Future expansion of this library should deliberately target the less-covered types: `FilterToFilter`, `AudioToRing`, `PitchToPitch`, and `AmpToChoke`.

---

## Future Expansion

Priority pairs not included in v1.0 but documented in the coupling audit:

| Source | Target | Type | Priority | Status |
|--------|--------|------|----------|--------|
| ORBITAL | OPAL | `Audio->Wavetable` | P4 | Orbital's additive harmonics as Opal grain source — richer than Overworld |
| OSPREY | OSTERIA | `Audio->FM` | P5 | Shore wave energy flooding the tavern room model |
| OPTIC | OBLIQUE | `Audio->FM` | P6 | Comb jelly neural pulse shapes prism filter sweeps |
| OUROBOROS | ORBITAL | `LFO->Pitch` | P8 | Chaos velocity modulates additive harmonic grid |
| OBSCURA | ORIGAMI | `Audio->FM` | P9 | Spring chain output drives STFT source |

The two stub engines (OCELOT, OWLFISH) are not included in this library because their `applyCouplingInput` implementations are stubs — presets would document coupling that has no audible effect. Once their coupling wiring is complete (OWLFISH: `AudioToFM` from ONSET; OCELOT: `EnvToMorph` from ONSET), they are natural additions as high-priority pairs.
