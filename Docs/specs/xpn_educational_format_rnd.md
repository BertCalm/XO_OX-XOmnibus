# XPN as Educational Format — R&D Session
**Authors**: Atlas (XOmnibus bridge android) + Vibe (sound design android)
**Date**: 2026-03-16
**Status**: Pedagogical Foundation + Actionable Spec
**Scope**: How XO_OX repositions XPN packs as teaching instruments, not finished products

---

## The Core Insight

Every XPN pack ever sold is an answer. Here is the kick. Here is the snare. Here is the hat. Here is the final sound.

Nobody sells the question.

XO_OX can sell the question. The question is: *how did this sound come to exist?* The Deconstruction Pack, the A/B Test Pack, the History Lesson Keygroup, and the Tutorial Mode XPN structure are all different ways of making that question audible. The producer does not just receive a sound — they receive the argument for why that sound is the right answer.

This is not a novelty. It is a pedagogical philosophy embedded directly in a product format.

The distinction matters because producers who understand *why* sounds work do not just use XO_OX presets — they understand synthesis well enough to modify them, extend them, and eventually design from scratch. The educational pack creates fluency, not dependency. That is a harder sell and a more valuable one.

This document specifies the formats, the production pipelines, and the tools needed to build an XO_OX educational catalog.

---

## Section 1: The Deconstruction Pack Format

### The Problem It Solves

A finished preset is opaque. You hear it, you like it, you use it. You do not know why it sounds the way it does. The kick has character — is that the envelope shape? The filter? The saturation? The coupling? You cannot tell by listening to the result.

The Deconstruction Pack makes the signal chain audible by rendering each stage as a separate pad. Pads 1–8 are not 8 different sounds. They are 8 moments in the construction of *one* sound.

Playing pads 1 through 8 in sequence is watching a synthesis lesson unfold in real time.

### Format Spec: ONSET Drum Deconstruction

Eight pads for a single drum voice (kick, used as the canonical example):

| Pad | Label | State | What the Producer Hears |
|-----|-------|-------|------------------------|
| 1 | `+Base` | Raw oscillator, no pitch, no envelope, no filter | A pure sine burst — the atoms |
| 2 | `+Pitch` | Pitch tuned, still no envelope curve | The same burst with frequency identity — "this is a kick" beginning to emerge |
| 3 | `+Envelope` | Amplitude envelope added (attack/decay shape) | The transient character appears — punch vs. thud is decided here |
| 4 | `+Filter` | Filter sweep added (cutoff + envelope amount) | High-frequency content sculpted — frequency architecture of the sound |
| 5 | `+Saturation` | Drive/saturation added | The feliX-Oscar axis appears — weight, grit, warmth vs. clean |
| 6 | `+Space` | Reverb tail added | The sound acquires a room — physical size becomes audible |
| 7 | `+Coupling` | XOmnibus coupling route added | The sound responds to engine context — it becomes relational |
| 8 | `+Final` | Complete preset (identical to the shipped product) | The finished sound |

The producer hears exactly one variable change per step. Every parameter held constant between pad N and pad N+1 is controlled. The pedagogical unit is isolated.

### Why 8 Pads

Eight is the natural count of the major synthesis stages: oscillator → pitch → amplitude envelope → filter → saturation → space → coupling → final. It maps cleanly to one row of MPC pads and to one full screen on the MPC display.

For simpler voices (a hi-hat, a clap), earlier stages may be combined, but the eight-pad limit is a hard constraint. The format must fit in one bank. If a voice requires more than eight conceptual stages, the designer collapses two into one step — which itself is a pedagogical choice about which stages are "major."

### Naming Convention

Pack name: `{ENGINE}_Deconstruction_Vol{N}`
Example: `ONSET_Deconstruction_Vol1`

Pad names within the XPM file:
```
{VoiceSlug}_{PadNum}_{Label}
kick_1_Base
kick_2_+Pitch
kick_3_+Envelope
kick_4_+Filter
kick_5_+Saturation
kick_6_+Space
kick_7_+Coupling
kick_8_Final
```

The `+` prefix on labels 2–7 signals "added to everything before this" — cumulative construction, not replacement.

### Which Voices Work Best

- **ONSET kick or snare**: deepest pedagogical value because the signal chain is the most differentiated. Oscillator → pitch → envelope → filter → saturation each contribute audibly.
- **OPAL granular pad**: demonstrates grain size, position jitter, envelope shape, pitch spread — the granular signal chain is invisible in finished presets.
- **OVERWORLD ERA pad**: uses pad 7 to demonstrate the ERA crossfade as a coupling stage — makes OVERWORLD's most distinctive feature tangible.
- **OVERDUB voice**: demonstrates the send VCA → drive → tape delay → spring reverb signal chain — the `dub_sendAmount` parameter finally becomes audible in isolation.

Avoid voices where the early stages are inaudible or identical. A high-hat deconstruction where steps 1 and 2 sound the same defeats the pedagogy.

### The `xpn_deconstruction_builder.py` Tool

**Purpose**: Given a base ONSET preset and a list of parameter increment steps, render each incremental state and assemble the 8-pad deconstruction kit.

**Interface**:
```
python3 xpn_deconstruction_builder.py \
    --preset "Kick_Foundation" \
    --voice kick \
    --steps steps/kick_chain.json \
    --wavs-dir /path/to/rendered/wavs \
    --output-dir ./build/deconstruction/
```

**`steps/kick_chain.json` format**:
```json
{
  "base_params": {
    "perc_kickOscFreq": 55.0,
    "perc_kickEnvDecay": 0.0,
    "perc_kickFilterCutoff": 22000.0,
    "perc_kickDrive": 0.0
  },
  "steps": [
    {
      "label": "Base",
      "add_params": {}
    },
    {
      "label": "+Pitch",
      "add_params": {
        "perc_kickOscFreq": 60.0,
        "perc_kickPitchEnvAmount": 0.8,
        "perc_kickPitchEnvDecay": 0.05
      }
    },
    {
      "label": "+Envelope",
      "add_params": {
        "perc_kickEnvDecay": 0.35,
        "perc_kickEnvCurve": 0.7
      }
    }
  ]
}
```

Each step's `add_params` is merged cumulatively onto all previous steps. The tool renders one WAV per step using the XPN render pipeline (delegating to `xpn_render_spec.py`), then writes 8 XPM files and packages the deconstruction XPN.

**Integration with existing Oxport pipeline**:
- `xpn_deconstruction_builder.py` calls `xpn_render_spec.py` for WAV rendering
- Output directory structure is compatible with `xpn_packager.py`
- Liner notes are auto-generated via `xpn_liner_notes.py` with section type `deconstruction_chain`

**New liner notes section type required**: `deconstruction_chain` — lists each step, the parameter changes made, and a one-sentence listening instruction ("Listen for the punch appearing in the attack transient").

### Production Time Estimate

| Task | Time |
|------|------|
| Design step list JSON for one voice | 1–2 hours |
| Render 8 WAVs per voice | 20 min (automated) |
| Write liner notes narration for 8 steps | 1 hour |
| Full pack build + QA | 30 min |
| **Total per voice** | **~4 hours** |
| Full ONSET kit (8 voices × 8 steps = 64 pads across 8 programs) | ~2 days |

### Distribution Strategy

**Free, unpaywalled.** Deconstruction packs are educational marketing, not revenue products. They exist to:
1. Teach producers the XO_OX synthesis vocabulary
2. Demonstrate that XO_OX sounds have visible craft behind them
3. Create a reference library that tutorial content can cite

Host on XO-OX.org as free downloads. Reference them in Field Guide posts. Link to them from the eBook.

---

## Section 2: The A/B Test Pack

### The Problem It Solves

Most producers cannot describe *what they are hearing* when two sounds differ. They know "I prefer this one" but not "I prefer this one because the attack is faster and the filter envelope has a positive amount instead of zero." The A/B Test Pack trains that vocabulary.

Eight pairs of pads. Each pair holds all parameters constant except one. The producer plays pad A, plays pad B, hears the difference. If the description in the liner notes matches what they hear, they have learned that parameter's function.

### Pair Spec: The "XOmnibus Fundamentals" A/B Pack

| Pads | Variable | A State | B State | What to Listen For |
|------|----------|---------|---------|-------------------|
| 1–2 | feliX vs. Oscar filter character | feliX bright open filter (`snap_filterCutoff` high, resonance low) | Oscar resonant sub filter (`morph_filterCutoff` mid, resonance high) | High-end air vs. mid-body weight |
| 3–4 | Attack transient | Short attack (1ms) | Long attack (80ms) | Punch vs. swell — the moment of entry |
| 5–6 | LFO rate | Fast LFO (4Hz) | Slow LFO (0.05Hz) | Tremolo vs. drift — time scale of motion |
| 7–8 | Coupling on/off | Engine A in isolation | Engine A + Engine B coupling at 60% | Added harmonics, frequency interaction, beating |
| 9–10 | Reverb time | Short reverb (RT60 = 0.4s) | Long reverb (RT60 = 3.2s) | Intimate room vs. cathedral — spatial scale |
| 11–12 | Velocity sensitivity | High velocity response (velocity → filter + amp) | Flat velocity (all layers identical) | Expressive range vs. consistency |
| 13–14 | DNA-matched vs. random preset | Preset with DNA matched to `brightness=0.8, aggression=0.3` | Random preset from same engine | The coherence that targeted DNA produces |
| 15–16 | Contextually correct vs. wrong | The "right" sound for a 90 BPM hip-hop context | Same engine, wrong context (cinematic orchestral character) | Why context shapes sound choice beyond aesthetics |

### Pair 15–16: The "Right vs. Wrong" Pair

This pair deserves explanation because it is the most pedagogically unusual.

The "wrong" sound is not a bad sound. It may be beautiful. But it is beautiful in the wrong way for the stated context. A producer who can hear *why* a sound is wrong — not that it sounds bad, but that it carries incompatible energy — has developed contextual ear training.

Liner notes for this pair should specify: "In a 90 BPM hip-hop groove, the second pad sounds unsettled — not because it is poorly made, but because its harmonic language belongs to a different genre grammar. Notice that your first instinct is probably 'this doesn't fit' before you can articulate why. That instinct is your context ear working. The A/B pair teaches you to name it."

### Educational Value Beyond the Producer

The A/B pack is a **workshop tool**. A sound design instructor can:
- Play each pair and ask students to describe the difference before reading the liner notes
- Use pairs 1–2 as the opening lesson in any session covering filter character
- Use pairs 7–8 as the clearest demonstration of XOmnibus coupling that exists

It is also a **blog post**. Each pair is a section. "What makes a filter warm? Load pads 1 and 2 in the FUNDAMENTALS A/B pack and hear the same notes through feliX and Oscar. The difference is not subtle." The physical product and the written content cross-reference each other.

### Naming Convention

Pack name: `{ENGINE_OR_TOPIC}_AB_Vol{N}`
Example: `XOmnibus_Fundamentals_AB_Vol1`, `OVERWORLD_ERA_AB_Vol1`

Pad names: `{PairNum}{Side}_{VariableName}`
Example: `1A_Felix`, `1B_Oscar`, `7A_NoCoupling`, `7B_Coupled`

### Production Time Estimate

| Task | Time |
|------|------|
| Design 8 pairs with controlled parameter sets | 2 hours |
| Render 16 WAVs | 20 min |
| Write liner notes for 8 pairs (listening instructions) | 1.5 hours |
| Full pack build + QA | 30 min |
| **Total per A/B pack** | **~4.5 hours** |

### Distribution Strategy

**Free (Tier 1) and Patreon (Tier 2)**. Basic A/B packs (feliX/Oscar, attack, LFO) are free. Advanced packs (coupling types, DNA matching, contextual correctness) are Patreon benefits — they require more background knowledge to appreciate and reward the audience that has done the reading.

---

## Section 3: The History Lesson Keygroup

### The Problem It Solves

XOmnibus spans synthesis eras — analog voltage control, FM digital, sample-based, virtual analog, hybrid modular, neural. OVERWORLD makes this history playable via its ERA triangle. But the ERA triangle is invisible to producers who did not read the documentation.

The History Lesson Keygroup makes synthesis history audible as a chromatic instrument. Playing up the keyboard is a guided tour through 50 years of synthesis character.

### Octave Mapping

| Octave | Era | Historical Source | Timbral Character | OVERWORLD Parameter State |
|--------|-----|------------------|--------------------|--------------------------|
| C1–B1 | 1970s Voltage Controlled | Minimoog, ARP Odyssey, early Buchla | Warm, slightly drifting, harmonically saturated, analog noise floor audible | ERA triangle: Buchla corner, `ow_driftAmount` at 0.3, `ow_era` = 0.15 |
| C2–B2 | 1980s FM Synthesis | Yamaha DX7, TX816, Chowning at Stanford | Bell-like, metallic upper harmonics, precise attack, digital clarity | ERA triangle: mid-point between corners, FM operators active, `ow_era` = 0.45 |
| C3–B3 | 1990s Sample-Based | Akai S1000, E-mu Emulator, Roland S-750 | Loop artifacts at sustain boundary, frequency-accurate but physically static | Sample-character simulation via looping + subtle pitch drift at sustain |
| C4–B4 | 2000s Virtual Analog | Native Instruments Pro-53, Arturia Moog V | Clean, accurate, absent of noise floor and drift — analog emulation without analog flaws | ERA triangle: Schulze corner, high accuracy, `ow_driftAmount` at 0.05 |
| C5–B5 | 2010s Modular/Hybrid | Eurorack + plugins, Make Noise, Mutable Instruments | Unstable at edges, voltage-controlled randomness, self-patching character | ERA triangle: Vangelis corner, `ow_modulationDepth` high, chaotic modulation |
| C6–B6 | 2020s Neural/Spectral | RAVE, NSynth, diffusion-based synthesis | Impossible timbres, smeared onset, ghost harmonics, non-physical decay | All ERA triangle axes at 0.5 + coupling at maximum — the synthetic unknown |

### Why OVERWORLD is the Natural Generator

OVERWORLD's ERA triangle is a 2D timbral crossfade between three synthesizer eras. The History Lesson Keygroup maps specific ERA coordinates to octaves, so each octave samples a fixed point in OVERWORLD's ERA space. The pack is not a metaphor for OVERWORLD — it *is* OVERWORLD, sampled at six historical coordinates.

This also makes the pack a demonstration of OVERWORLD's range. A producer who plays the full 6-octave keygroup from C1 to B6 hears the complete span of the ERA triangle in sequence. The pack teaches OVERWORLD's parameter space.

### Integration with the Field Guide

The History Lesson Keygroup pairs directly with a Field Guide post: "50 Years of Synthesis in 72 Notes." The post explains each era — historical context, canonical instruments, how that era shaped modern production — and directs the reader to load the corresponding octave to *hear* the era described. The physical pack and the written post are one artifact.

This is the target for all XO_OX educational content: the pack and the text are not separate products. They are two surfaces of the same object.

### Distribution Strategy

**Paid, premium pricing.** The History Lesson Keygroup is the most production-intensive format (6 octave-zones, 72 tuned samples, extensive liner notes, companion blog post). It is also the most complete product — a buyer receives both a chromatic instrument and a synthesis education.

Suggested price: $14–18. Bundle with the companion Field Guide post into a "History" package for Patreon Tier 3.

---

## Section 4: The Tutorial Mode XPN Structure

### Standard vs. Tutorial XPN Directory Layout

Standard XPN packs use this structure:

```
PackName.xpn/
  Programs/
    KitName.xpm
  Samples/
    sample_01.wav
    sample_02.wav
  expansion.json
```

Tutorial Mode adds three new layers:

```
PackName_Tutorial.xpn/
  Programs/
    KitName_Final.xpm          ← finished product (loads as default)
    KitName_Step1.xpm          ← oscillator only
    KitName_Step2.xpm          ← + pitch
    KitName_Step3.xpm          ← + envelope
    KitName_Step4.xpm          ← + filter
    KitName_Step5.xpm          ← + saturation
    KitName_Step6.xpm          ← + space
    KitName_Step7.xpm          ← + coupling
    KitName_Step8.xpm          ← = Final (redundant but explicit)
  Samples/
    step1_base.wav
    step2_pitched.wav
    step3_envelope.wav
    step4_filter.wav
    step5_saturation.wav
    step6_space.wav
    step7_coupled.wav
    step8_final.wav
    [standard production samples for Final program]
  Tutorial/
    tutorial.md
    tutorial_audio_notes.txt
    step_parameters.json
  expansion.json
  liner_notes.json
```

The MPC loads `KitName_Final.xpm` by default. The step programs are available as alternate programs within the same pack — the producer can navigate to them from the Programs menu.

### The Self-Unpacking Tutorial

`tutorial.md` inside the pack is a step-by-step text guide that references the programs by name:

```markdown
# {KitName} — How This Sound Was Built

## What You Are Looking At

This kit contains the finished {KitName} preset and 7 intermediate steps
showing how it was constructed. Each step adds exactly one signal chain stage.

## Step 1: Load KitName_Step1

Start here. This is just the oscillator. No envelope, no filter, no character.
Hit pads 1-4 and listen to the raw frequency. This is the material we will work with.

**What you are hearing:** A sine oscillator at 55Hz. Nothing else.
**Parameter in focus:** `perc_kickOscFreq` = 55.0

## Step 2: Load KitName_Step2

Now the oscillator has pitch character — a fast downward sweep from 200Hz to 55Hz
over 50ms. This is what makes a kick sound like a kick rather than a sine burst.

**What you are hearing:** Pitch envelope. The "boing" is the pitch sweep.
**Parameters added:** `perc_kickPitchEnvAmount` = 0.8, `perc_kickPitchEnvDecay` = 0.05s

...
```

### `tutorial_audio_notes.txt`

A supplementary file with terse listening notes for users who prefer to skip prose:

```
STEP 1 → STEP 2: Hear the pitch sweep arrive. The "boing" is new.
STEP 2 → STEP 3: Hear the body appear. The attack gets punch.
STEP 3 → STEP 4: Hear the top end close down. Filter is shaping the character.
STEP 4 → STEP 5: Hear the weight increase. Saturation adds harmonic density.
STEP 5 → STEP 6: Hear the room appear. The sound gets larger.
STEP 6 → STEP 7: Load two engines, play coupled. Hear the frequency interaction.
STEP 7 → STEP 8: They are identical. You built the finished preset.
```

### `step_parameters.json`

A machine-readable record of every parameter change at each step, suitable for tooling consumption:

```json
{
  "pack_name": "ONSET_Kick_Foundation_Tutorial",
  "engine": "ONSET",
  "voice": "kick",
  "base_params": { ... },
  "steps": [
    { "step": 1, "label": "Base", "params_added": {} },
    { "step": 2, "label": "+Pitch", "params_added": { "perc_kickPitchEnvAmount": 0.8 } },
    ...
  ]
}
```

This file is consumed by `xpn_tutorial_builder.py` for regeneration and by future tooling (eBook export, website interactive).

### The Tutorial Mode XPN as an MPC-Native Object

The key design constraint is that the Tutorial Mode XPN must work with no companion material. A producer who receives this pack without reading a word of documentation should be able to:

1. Load it on the MPC
2. See that there are 9 programs (Step1 through Step8 + Final)
3. Navigate through them in order
4. Hear the sound build

The `tutorial.md` is supplementary depth, not a requirement for basic understanding. The pack teaches by its structure.

---

## Section 5: The Micro-Tutorial Pack Series

### The Problem It Solves

Deconstruction Packs and Tutorial Mode packs require significant production time. The Micro-Tutorial is the minimum viable educational unit: 4 pads, one concept, 15 minutes of listening.

Micro-packs are free. They are marketing, education, and community resource simultaneously.

### Canonical Micro-Tutorial Concepts

**"What is Coupling?" (4 pads)**

| Pad | State | What It Demonstrates |
|-----|-------|---------------------|
| 1 | Engine A in isolation | Character A's voice, unmodified |
| 2 | Engine B in isolation | Character B's voice, unmodified |
| 3 | A → B (A modulates B) | A's character imposed on B's synthesis |
| 4 | B → A (B modulates A) | B's character imposed on A's synthesis |

This is the best possible introduction to XOmnibus coupling. In 4 pads, the producer hears what coupling *does* without needing to understand what coupling *is*. The experience precedes the concept.

**"What is the feliX-Oscar Axis?" (4 pads)**

| Pad | State |
|-----|-------|
| 1 | Full feliX (OddfeliX engine at max, no OddOscar) |
| 2 | feliX-dominant mix (70/30) |
| 3 | Oscar-dominant mix (30/70) |
| 4 | Full Oscar (OddOscar engine at max, no OddfeliX) |

**"What is 6D Sonic DNA?" (4 pads)**

| Pad | DNA Target | Description |
|-----|------------|-------------|
| 1 | `brightness=0.9, aggression=0.8` — top-right: aggressive brightness | Cutting, harsh, modern |
| 2 | `brightness=0.9, aggression=0.1` — top-left: bright but gentle | Airy, open, light |
| 3 | `brightness=0.2, aggression=0.8` — bottom-right: dark and aggressive | Heavy, grinding, deep |
| 4 | `brightness=0.2, aggression=0.1` — bottom-left: dark and soft | Warm, intimate, submerged |

Four extremes of a 2D slice through DNA space. The producer hears the coordinate system.

**"What is Granular Synthesis?" (4 pads)**

| Pad | Grain Size | Character |
|-----|------------|-----------|
| 1 | 5ms | Spectral smear, pitched noise, grains too small to resolve |
| 2 | 20ms | Granular shimmer, pitch beginning to stabilize |
| 3 | 100ms | Recognizable pitch, smooth texture, classic granular |
| 4 | 500ms | Long grains, audible splicing, almost sample playback |

This is the single most efficient way to explain granular synthesis. The OPAL engine generates all four pads from the same source — only `opal_grainSize` changes.

**"What are Velocity Layers?" (1 pad, 4 velocity zones)**

A single pad with four dramatically different sounds mapped to pp/mp/mf/ff:
- pp (0–31): a whisper, barely present
- mp (32–63): a statement, moderate energy
- mf (64–95): an emphasis, filter opening noticeably
- ff (96–127): full saturation, maximum harmonic content

The producer plays the pad at different velocities and hears that velocity is not just volume — it is timbre.

**"What is the ERA Triangle?" (4 pads)**

| Pad | ERA Coordinate | Era |
|-----|---------------|-----|
| 1 | Buchla corner | 1970s voltage control |
| 2 | Schulze corner | 1980s Berlin school |
| 3 | Vangelis corner | 1980s cinematic synthesis |
| 4 | Center | All three eras crossfaded equally |

Four points in OVERWORLD's ERA space. Explains the ERA triangle in 4 hits.

### Series Naming

`XO_OX_Micro_{ConceptSlug}_Vol{N}`
Examples: `XO_OX_Micro_Coupling_Vol1`, `XO_OX_Micro_GrainSize_Vol1`, `XO_OX_Micro_DNA_Vol1`

### Distribution Strategy

**Always free, permanently.** Micro-packs exist to drive awareness. They are the entry point for producers who have never heard of XO_OX. They make complex synthesis concepts accessible with zero prior knowledge. They cost nothing to receive and they cost relatively little to produce.

Release schedule: one micro-pack per major Field Guide post. When a post explains coupling, the coupling micro-pack drops the same day. The written content and the physical pack share a release.

---

## Section 6: Practical Implementation

### Tool Inventory

#### Existing Tools Leveraged

| Tool | Role in Educational Packs |
|------|--------------------------|
| `xpn_render_spec.py` | WAV rendering for all step-by-step variants |
| `xpn_drum_export.py` | Base XPM generation; Deconstruction packs use its velocity layer infrastructure |
| `xpn_keygroup_export.py` | History Lesson keygroup XPM generation |
| `xpn_liner_notes.py` | Liner notes for all formats; requires 2 new section types (see below) |
| `xpn_packager.py` | Final XPN assembly for all formats |
| `xpn_bundle_builder.py` | Multi-pack bundle assembly (useful for "Complete Deconstruction Vol. 1" bundles) |

#### New Tools Required

**`xpn_deconstruction_builder.py`**

Takes a base preset, a step list JSON (`steps/voice_chain.json`), rendered WAVs, and a target output directory. Produces N XPM files (one per step) and hands off to `xpn_packager.py`.

Core logic:
1. Load `base_params` from step JSON
2. For each step: merge `add_params` cumulatively onto all previous params
3. Write XPM pointing to the corresponding WAV (`step{N}_{label}.wav`)
4. Call `xpn_liner_notes.py` with section type `deconstruction_chain` to auto-generate listener instructions
5. Bundle all step XPMs + samples + liner notes via `xpn_packager.py`

Key implementation note: the WAVs are rendered *externally* (using `xpn_render_spec.py` or manual export from XOmnibus). The builder assembles the XPN structure around pre-existing WAVs. Separation of rendering from assembly is intentional — rendering requires the audio engine, assembly is pure file manipulation.

**`xpn_tutorial_builder.py`**

Takes a step list JSON (same format as `xpn_deconstruction_builder.py`'s input) plus a narration file and builds the full Tutorial Mode XPN structure — programs for each step, `Tutorial/` directory with `tutorial.md` + `tutorial_audio_notes.txt` + `step_parameters.json`, and final `expansion.json`.

Interface:
```
python3 xpn_tutorial_builder.py \
    --steps steps/kick_chain.json \
    --narration narration/kick_tutorial.md \
    --wavs-dir /path/to/wavs \
    --output-dir ./build/tutorial/
```

The `narration/kick_tutorial.md` is the human-authored tutorial text. The tool injects the parameter tables from `step_parameters.json` automatically into the correct positions using `<!-- PARAMS:stepN -->` placeholder tokens.

Output `Tutorial/tutorial.md` is the merged document: human narration + auto-generated parameter tables.

#### New Liner Notes Section Types

Two new section types must be added to `xpn_liner_notes.py`:

**`deconstruction_chain`**: Documents the signal chain progression for a Deconstruction Pack. Fields: `voice`, `engine`, `steps` (array of `{ step, label, params_added, listener_note }`). The listener note is one sentence: what the producer should hear change between the previous step and this one.

**`ab_pair`**: Documents one pair from an A/B Test Pack. Fields: `pair_number`, `variable`, `pad_a` (`{ label, state_description, key_params }`), `pad_b` (`{ label, state_description, key_params }`), `listening_instruction`.

### eBook Cross-Referencing

Each eBook chapter that covers a synthesis concept should include a sidebar: "Hear This Concept." The sidebar names the specific XPN pack and pad numbers that demonstrate the concept discussed. Example:

> **Hear This Concept**
> Load `XO_OX_Micro_GrainSize_Vol1` on your MPC. Play pad 1, then pad 4. The difference between 5ms and 500ms grain size is the difference between a spectral smear and a coherent pitch. You just heard granular synthesis from its most abstract to its most recognizable.

The `liner_notes.json` structure supports this via a `cross_references` array. Each entry in `cross_references` has: `ebook_chapter`, `section`, `quote` (the suggested sidebar text). The eBook generation pipeline reads `cross_references` from `liner_notes.json` and injects sidebars automatically.

This is the architectural principle: the pack, the liner notes, the eBook, and the Field Guide post are not separate content. They are one document in four physical forms.

### Production Time Summary

| Format | Per-Unit Time | Priority |
|--------|--------------|----------|
| Micro-Tutorial (4 pads) | 1.5–2 hours | HIGH — free, maximum reach |
| A/B Test Pack (16 pads, 8 pairs) | 4–5 hours | HIGH — free, workshop utility |
| Deconstruction Pack (8-pad, 1 voice) | 4 hours | MEDIUM — free, requires tool build first |
| Deconstruction Kit (8 voices × 8 pads) | 2 days | MEDIUM — flagship educational product |
| History Lesson Keygroup (72 notes) | 1–2 days | LOWER — premium product, requires OVERWORLD depth |
| Tutorial Mode XPN | +2–3 hours over base pack | ONGOING — attach to any major pack release |

### Showcase Engine by Format

| Format | Best Showcase Engine | Why |
|--------|---------------------|-----|
| Deconstruction Pack | ONSET | Drum signal chain is clearest — each stage is audibly distinct |
| A/B Test Pack | OddfeliX + OddOscar | The feliX/Oscar pairing *is* the A/B contrast; it teaches the XO_OX duality |
| History Lesson Keygroup | OVERWORLD | ERA triangle maps directly to synthesis eras; the format is native to the engine |
| Micro-Tutorial (Coupling) | Any two engines | OPAL → OVERDUB or OVERWORLD → OBLONG are the most audible couplings |
| Micro-Tutorial (Granular) | OPAL | Single parameter (`opal_grainSize`) produces maximum timbral range |
| Tutorial Mode XPN | OPAL | Granular signal chain stages are the most invisible in finished presets — making them visible is high value |

---

## Summary: What This Document Defines

Five formats that reposition XPN packs as teaching instruments:

1. **Deconstruction Pack** — 8 pads showing one sound's construction stage by stage. Tool: `xpn_deconstruction_builder.py`. Free distribution.

2. **A/B Test Pack** — 8 paired pads, each pair holding all variables constant except one. Teaches parameter function by controlled contrast. Free (basic) / Patreon (advanced).

3. **History Lesson Keygroup** — Chromatic instrument mapping synthesis eras to octaves. Makes OVERWORLD's ERA triangle a guided tour through 50 years of synthesis history. Paid premium.

4. **Tutorial Mode XPN** — Standard pack extended with step programs, a `Tutorial/` directory containing `tutorial.md` + `tutorial_audio_notes.txt` + `step_parameters.json`. The pack teaches by its structure without requiring companion material. Attached to major pack releases.

5. **Micro-Tutorial Series** — 4-pad free packs, each teaching one concept. Maximum reach, minimum production cost. Always free, released alongside companion Field Guide posts.

Two new tools are specified: `xpn_deconstruction_builder.py` and `xpn_tutorial_builder.py`. Both integrate with the existing Oxport pipeline. Two new liner notes section types are required in `xpn_liner_notes.py`: `deconstruction_chain` and `ab_pair`.

The unifying principle: the pack, the liner notes, the eBook, and the Field Guide post are not separate content. They are one pedagogical object in four physical forms. A producer who plays the pack, reads the liner notes, follows the tutorial, and reads the Field Guide post has completed a coherent synthesis lesson — not a sequence of marketing materials.

XO_OX is not selling sounds. It is selling understanding. The sounds come with the lesson.
