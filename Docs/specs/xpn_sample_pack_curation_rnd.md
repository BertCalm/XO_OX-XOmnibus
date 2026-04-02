# XPN Sample Pack Curation — R&D Spec
**Authors**: Vibe + Kai (Sound Design + Format Androids)
**Date**: 2026-03-16
**Status**: R&D Reference — Active Standard

---

## Overview

This document defines the curation philosophy for all XO_OX XPN sample packs. It answers one question with precision: what makes an XO_OX sample different from any other sample pack on the market? The answer is not production quality alone — it is intent, personality, and internal coherence. Every decision in this spec flows from that premise.

---

## Section 1: The XO_OX Sample Philosophy

### 1.1 Every Sample Is a Full Musical Statement

A sample pack is not a catalog of materials. It is a collection of moments. Each sample must be a complete thought — not a noise, a tone, or a texture, but an *event* with a beginning, a character, and an implied consequence.

**What this rules out:**
- Generic sine-based sub hits with no defining character
- Dry claps that exist only to mark the backbeat
- Pads that sustain without movement, change, or internal tension
- Loops that repeat without implying a direction

**What this requires:**
- A kick drum that lands with a specific personality — not just "punchy" but *what kind* of punchy, and why
- A melodic loop that makes a harmonic statement: tension, resolution, suspended ambiguity — one of the three, committed
- An atmospheric texture that creates a specific *place*, not a neutral backdrop
- Even a single-hit sample should have an implied character arc across its envelope: the attack tells you what just happened, the sustain tells you what it is, the release tells you where it's going

### 1.2 Samples Must Have Personality Before Processing

The MPC's filters, effects, and modulation are not corrective tools — they are expressive tools. An XO_OX sample must arrive with enough personality that the MPC can amplify or subvert it, not construct it from scratch.

**Test:** Load the raw WAV on the MPC with zero processing. Does it have an identity? If the answer is "it depends on what you do with it," the sample is not ready.

Personality markers in raw samples:
- A defined timbral signature (bright/dark, smooth/gritty, harmonic/inharmonic)
- A readable dynamic shape (does the envelope say anything?)
- A spatial quality baked in at the source (not applied later) — dry is fine if the room is audible in the source

### 1.3 The feliX-Oscar Axis Must Be Audible Within Every Pack

XO_OX products are defined by the tension between feliX (bright, forward, aggressive, electric) and Oscar (warm, receded, slow, organic). This polarity must be expressed within every pack — not just across the catalog.

**Minimum requirement per pack:**
- At least 20% of samples must read as clearly feliX-dominant (bright transients, edge, presence)
- At least 20% of samples must read as clearly Oscar-dominant (warmth, depth, rounding)
- The remaining 60% can inhabit the middle — but the extremes must exist so the middle has meaning

**Why this matters:** A pack that lives entirely in the middle is a pack without tension. Tension is what makes producers reach for the next sound. Without feliX and Oscar in the same space, the pack collapses into commodity.

### 1.4 Avoid Sample Pack Clichés

XO_OX does not make reference content. Reference content is what everyone else already has.

**Explicit prohibitions:**
- No 808 subs tuned to G# as a default (this is a specific cultural habit that signals mass production)
- No "classic hip hop snare" — defined as: 90s SP-1200/MPC3000 pitched snare with predictable crack and short tail, unmodified
- No "trap hi-hat rolls" as a designed feature — variations on a template are not a pack concept
- No reverb-washed acoustic piano loops in the key of C or A minor
- No "lo-fi" as an aesthetic applied over otherwise generic content — lo-fi must be baked into the source, not added via a filter chain

**Replacement standard:** For every cliché avoided, articulate what XO_OX does instead. "We don't make classic hip hop snares — we make ONSET snares that start from a physical model of a snare with a paper-thin shell, processed through tape saturation during synthesis, rendered at 48kHz with intentional room bleed." That is a specific position.

---

## Section 2: Recording and Sourcing Philosophy

### 2.1 Synthesis-First: XOceanus as Primary Source

XOceanus-rendered samples are the primary source for all XO_OX packs. The character is architectural — built into the synthesis engine, not applied afterward. This is the competitive advantage: no other sample pack company has access to this sound source.

**What synthesis-first means in practice:**
- Drum hits: rendered from ONSET using physical modeling parameters at the preset level
- Melodic one-shots: rendered from character engines (OBLONG, OBESE, OPAL, ORACLE, etc.) at specific preset points
- Atmospheres: rendered from ORIGAMI, ORACLE, OCEANIC, ORBITAL at defined parameter states
- Loops: recorded from live XOceanus sessions, not constructed from layered one-shots

**Source preservation rule:** When a sample comes from XOceanus, log the engine, preset name, and key parameter states at time of render. This enables versioning, variation packs, and auditability. Minimum log format:
```
ENGINE: OBLONG
PRESET: Amber Chord Wide
KEY PARAMS: bob_fltCutoff=0.62, bob_env_attack=0.15, bob_chorusDepth=0.3
RENDER: 48kHz/24bit, offline, velocity=95/127
```

### 2.2 Physical Recording: Unusual Mics, Unusual Rooms

When acoustic character is added — field recordings, organic texture layers, live instrument passes — the sourcing philosophy is anti-conventional.

**Mic philosophy:** The "correct" mic is often the wrong choice for XO_OX. A condenser in a treated room produces audio that sounds like everyone else's treated-room condenser recording. Preferred approaches:
- Contact microphones on resonant objects (metal, wood, glass)
- Dynamic mics placed in unconventional positions (inside a guitar body, behind a drum, floor-facing)
- Cheap consumer mics with audible character — not as a lo-fi aesthetic but as a specific timbral choice
- Binaural sources when spatial character is the goal

**Room philosophy:** Rooms are instruments. Use rooms that have a character, not rooms that have been acoustically neutralized. A stairwell, a tiled bathroom, a concrete loading dock — these are sources. A recording studio's live room is a last resort, not a first choice.

**The commit principle:** Commit to the room at recording. Do not record clean and add reverb later "to match." If the room is part of the sample's identity, it must be baked in at source. This is not negligence — it is craft.

### 2.3 Processing Philosophy: Commit at Recording

Every XO_OX sample that passes through processing before delivery must have that processing committed at render time. No sample should be delivered with the implicit message "this is a blank canvas — process it yourself."

**Commit-at-recording chain examples:**
- Saturation: applied to taste, baked into WAV
- EQ: corrective EQ (remove resonances) committed; character EQ (frequency emphasis) committed
- Compression: dynamic shaping committed — the envelope you hear is the envelope you get
- Pitch: if a sample is tuned to a specific note, it is tuned at source, not via metadata alone

**What is NOT committed at recording:**
- Reverb/delay that would limit musical use (except atmospheric textures by design)
- Stereo width that would cause mono compatibility issues
- Any processing that makes the sample incompatible with 30Hz–18kHz playback at 0dBFS input

### 2.4 Sample Length Targets

| Category | Target Range | Notes |
|----------|-------------|-------|
| Drum transients (kick, snare, clap) | 100–500ms | Include full tail; do not hard-trim at arbitrary point |
| Percussion (hi-hats, shakers, cymbals) | 50–2000ms | Open hi-hats may extend to 2s; closed hats 50–150ms |
| One-shot instruments (melodic, bass) | 0.5–3s | Loop point optional but preferred for sustaining sounds |
| Melodic loops | 4–16 bars at stated BPM | Always include BPM in filename or metadata |
| Atmospheric textures | 8–32s | Long enough to contain a full arc of movement |
| FX/transition sounds | 0.5–8s | Must have a clear start and end — not an endless drone |

---

## Section 3: Curation Standards Per Engine Type

### 3.1 ONSET Drums: The Groove Test

Every ONSET drum sample must pass the groove test before inclusion: play it in a 4-on-the-floor pattern at 90 BPM and evaluate whether it *feels right*. This is a subjective but non-negotiable filter.

**What "feels right" means:**
- The transient lands at the right time perceptually — not early (pre-ring) or late (slow attack)
- The dynamic shape implies weight that matches the sample's frequency content (a heavy-sounding kick must feel physically grounded, not just loud)
- Playing the sample repeatedly at tempo does not create fatigue within 8 bars

**Secondary groove test:** Play the sample off the beat (syncopated) and confirm that it grooves in that context too. Samples that only work on the downbeat are limited instruments.

**ONSET-specific requirements:**
- Kick drums: sub content must be audible on speaker systems with 50Hz+ response; sample must also read on laptop speakers (transient must survive without sub)
- Snares: crack-to-body ratio must be set at source — not relying on user EQ to find the snare's identity
- Hi-hats: velocity layering must be audibly distinct across at least 3 layers (not just volume — timbre)
- Claps and snaps: stereo spread baked in is acceptable but mono fold must not cancel

### 3.2 Melodic Engines (OBLONG, OPAL): Tonal Center Clarity

Every melodic sample must have a clear tonal center that works in multiple musical contexts.

**Tonal center requirements:**
- All pitched samples are tuned to concert pitch (A=440Hz) unless explicitly marked "detune character"
- Root note is documented in filename and/or metadata
- The tonal center must be audible within the first 500ms of the sample — no ambiguous attacks that leave the root unclear
- Chord samples must have an identifiable root, third, and quality (major/minor/suspended) — complex harmony is permitted but the sample must not require explanation

**Multi-key test:** A melodic sample is approved when it can be pitched to 3 non-adjacent musical keys and remain musically usable in at least 2 of them. Samples that only work in their original key are context-specific tools, not general-purpose instruments. Label context-specific samples accordingly.

**OPAL granular specifics:** Granular texture samples from OPAL must specify grain size and scan mode in source notes. The resulting sample's movement should imply the synthesis source to an informed listener — the granular character is not obscured.

### 3.3 Bass Engines (OBESE): Full-Range Usability

Every OBESE bass sample must work at two extremes simultaneously:
1. **At very low frequencies (30Hz+):** The sub content must be present, cleanly defined, and not distort at 0dBFS playback
2. **In a mix without sub:** The sample must retain presence and identity when high-pass filtered at 80–120Hz — there must be mid-bass and upper harmonic content that carries the character

**The radio test:** Can you identify this as a compelling bass sound on a phone speaker? If the answer is no, the sub is carrying the entire personality, and the sample will disappear in the mix on half of playback systems.

**Velocity layers for bass:** OBESE bass samples must have audible timbral change across velocity — not just amplitude. Soft hits should feel different (rounder, less harmonically complex) than hard hits (brighter, more saturated, more aggressive). OBESE's Mojo Control (B015 blessing) should be reflected in exported samples.

### 3.4 Atmospheric Engines (ORACLE, ORIGAMI): Distinct Space

Atmospheric samples from ORACLE and ORIGAMI must create a *specific* space — not a generalized reverberant environment.

**What "distinct space" means:**
- Temporal signature: the space has a characteristic decay time and diffusion pattern that is audibly non-generic
- Tonal signature: the space colors the sound in a recognizable way (dark cave, bright dome, narrow corridor)
- Movement signature: the space is not static — it evolves over the sample's duration in a way that implies a living environment

**What "distinct space" rules out:**
- Long reverb tail on a pad without any spatial character beyond "big"
- White-noise-textured ambiences that exist purely as background filler
- Atmospheric samples that "go with everything" — specificity is the feature, not the bug

**ORACLE-specific:** ORACLE samples (GENDY stochastic, MAQAM modal) must carry the harmonic system they came from. A MAQAM-sourced pad should imply its modal origin to a listener familiar with maqam music. This is not an exotic flavor — it is a specific musical vocabulary that must be honored, not flattened.

---

## Section 4: Negative Space Principle

### 4.1 Background vs. Foreground Samples

Every pack must contain both types — explicitly curated, not accidentally achieved.

**Foreground samples** (demand attention):
- High transient energy, forward-mixed character
- Bright or harmonically rich upper midrange
- Dynamic shape that peaks early and commands the mix position
- Designed to be audible at low volumes in a dense arrangement

**Background samples** (sit in the mix):
- Sustained, low-transient character
- Frequency content that fills space without masking foreground elements
- Can loop or repeat without drawing attention to their repetition
- Designed to create depth in an arrangement, not presence

**Minimum ratio:** Every pack with 20+ programs must include at least 25% foreground-designed samples and at least 25% background-designed samples. A pack skewed entirely toward foreground sounds is a pack of lead instruments without a rhythm section. A pack skewed entirely toward background is wallpaper.

### 4.2 The Pack's Character Is Its Ratio

The specific mix of foreground and background samples defines the pack's identity more than any individual sound.

A pack with 60% foreground / 40% background reads as aggressive and performative.
A pack with 30% foreground / 70% background reads as textural and cinematic.
Neither is wrong — but the ratio must be *chosen*, not accidental.

Document the intended foreground/background ratio in the pack's design brief before curation begins. Curate to hit it.

---

## Section 5: The Listen-First Rule

### 5.1 Hardware Audition Is Mandatory

Every sample considered for inclusion must be auditioned on physical hardware before approval. The minimum hardware standard is an Akai MPC loaded onto a pad with no processing, played by hand.

**Why hardware-only:** DAW playback through studio monitors is an ideal listening condition that most end users will never reproduce. The MPC's internal D/A converters, the built-in speakers (if applicable), and the feel of triggering a pad manually are all part of the sample's real-world context. A sample that sounds compelling in a DAW but flat on MPC hardware is not an XO_OX sample.

**The hesitation test:** If you load a sample onto a pad and hesitate before hitting it — if you pause to think "is this good?" rather than immediately wanting to play it again — it is not ready. Approval requires an immediate, positive physical response. Hesitation is a veto.

### 5.2 FX Program Classification

Samples that are "interesting in a mix but weird solo" are valid — but they must be labeled as FX programs, not instrument programs.

**FX program definition:** A sample whose musical value is context-dependent — it requires accompaniment, specific tempo, or a particular mix position to make sense. These samples are legitimate creative tools but must be classified accurately so the user knows what they are getting.

**FX program labeling standard:**
- Program type: FX (not DRUM, KEYS, BASS, PAD, etc.)
- Name prefix: "FX " or suffix " [FX]"
- Pack documentation note: "FX programs in this pack are designed as textural elements. They are most effective layered with primary instruments."

---

## Section 6: Sample Relationships Within a Pack

### 6.1 Kick and Bass Frequency Relationship

The relationship between kick drum and bass samples is not accidental — it is designed.

**Three valid kick/bass relationships:**

| Relationship | Description | When to Use |
|-------------|-------------|-------------|
| Sidechain Designed | Kick occupies 50–80Hz, bass occupies 80–200Hz — they divide the spectrum | High-energy, dense arrangements where sub clarity is critical |
| Overlapping | Kick and bass share sub territory — must be tuned to the same root to avoid cancellation | Slower, heavier styles where sub mass is the feature |
| Harmonic Complement | Bass tonal center is the dominant of the kick's fundamental — creates tension/release | Melodic bass-forward arrangements |

Every pack with both kick and bass content must specify which relationship is designed. This is documented in the pack brief and optionally in the liner notes.

### 6.2 Tuning Reference Across Melodic Programs

All pitched programs within a single pack share a tuning reference. The default is A=440Hz unless explicitly declared otherwise.

**Detuned packs (e.g., vintage character, tape warmth aesthetic):** Declare the reference pitch (e.g., A=432Hz, or "pitched down 15 cents for vintage weight") and apply it consistently across all melodic content.

**Why consistency matters:** A producer using multiple programs from one pack must be able to combine them without tuning conflicts. Pack-internal tuning consistency is a basic promise of the format.

### 6.3 Acoustic/Electronic Hybrid Rationale

When a pack combines acoustic-sourced samples and synthesized samples, the rationale for their coexistence must be explicit.

**Acceptable rationale types:**
- Spectral complementarity: acoustic sources fill frequency gaps that synthesis leaves open (e.g., real hi-hats in a kick-heavy ONSET pack)
- Character contrast by design: the tension between organic and synthetic is the pack's stated theme (feliX-Oscar axis made explicit)
- Processing convergence: acoustic sources processed to sound synthetic, and/or synthetic sources processed to sound organic — both approaching a shared timbral identity

**Not acceptable:** "We had both types of sounds and included them all." The combination must be motivated.

---

## Section 7: Quality Floor Standards

These are binary pass/fail standards. A sample that fails any one of them does not ship, regardless of its other qualities.

### 7.1 Noise Floor

- Ambient noise floor: **≤ -60dBFS** across the full sample duration
- Measured as: RMS of the quietest 100ms window in the sample's tail, excluding intentional decay
- Exception: samples with designed noise character (tape hiss, vinyl crackle, room ambience) must have that noise character clearly intentional and consistent — not the result of poor gain staging

### 7.2 Attack Integrity

- No audible clicks or pops at sample onset
- Attack must be clean at all velocity layers
- Zero-crossing check: all samples must begin and end within ±10 of digital zero to prevent edge-click on playback
- If a sample's natural attack includes a pop (e.g., a pressure transient from a physical instrument), that pop must be musically meaningful — not an artifact of poor mic placement or gain staging

### 7.3 Clipping and Headroom

- No digital clipping at any velocity layer at 0dBFS input
- Maximum peak level: **-0.3dBFS** (leave intersample headroom)
- All velocity layers normalized relative to each other: loudest layer at -0.3dBFS, quieter layers scaled proportionally
- Verify with true peak meter, not standard RMS or sample peak — intersample clipping is a failure

### 7.4 Velocity Layer Consistency

- Timbral change across velocity layers must be intentional and audible (D001 doctrine applied to samples, not just synthesis)
- No velocity layer should produce an audible discontinuity (a sudden jump in character that sounds like a different sample, not a louder/softer version of the same one)
- Each layer tested at its boundary velocities: a layer covering 64–95 must be tested at 64, 79, and 95

---

## Section 8: Pack-Level Quality Review Process

### 8.1 Pre-Delivery Checklist

Before any XPN pack is finalized for export, complete the following:

| Check | Method | Pass Criteria |
|-------|--------|--------------|
| Noise floor audit | RMS meter on sample tails | All samples ≤ -60dBFS ambient |
| Click/pop scan | Visual waveform review at 0-crossing + listening | Zero audible artifacts |
| Peak limiting check | True peak meter | No sample exceeds -0.3dBFS |
| feliX-Oscar balance | Manual classification of all samples | ≥20% feliX-dominant, ≥20% Oscar-dominant |
| Tonal center verification | Pitch detection on all melodic content | All roots documented, tuning reference consistent |
| Groove test | Live hardware playback on MPC | All drums pass without hesitation |
| Foreground/background ratio | Manual classification | ≥25% foreground, ≥25% background |
| Kick/bass relationship | Spectral analysis + listening | Relationship documented and audibly achieved |
| FX program labeling | Metadata review | All context-dependent samples marked FX |
| Velocity layer audit | MPC hardware playback at multiple velocities | Timbral change present, no discontinuities |

### 8.2 The Final Listen

After the pre-delivery checklist is complete, perform one uninterrupted session on MPC hardware using only samples from the pack. No production aids, no outside samples, no DAW.

**Purpose:** Confirm that the pack works as a self-contained creative environment. A producer should be able to make music with this pack alone — not every genre, not every context, but *something* compelling, and something that is recognizably XO_OX.

If the final listen session produces music you would want to finish, the pack ships. If it produces functional but uninspiring arrangements, return to curation.

---

*End of Spec — R&D Reference: XPN Sample Pack Curation Philosophy v1.0*
