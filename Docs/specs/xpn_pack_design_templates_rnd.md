# XPN Pack Design Templates — R&D Spec
**Date**: 2026-03-16
**Status**: Active Reference

---

## 1. Template Philosophy

These templates are starting points, not constraints. They define the minimum viable architecture so sound design energy goes into the sounds themselves — not into deciding how many programs to make, what to name the velocity layers, or how to assign choke groups.

Each template encodes decisions that have already been made well once. Use them as scaffolding. Deviate intentionally when the engine or concept demands it, and document why.

A pack that fills a template competently ships faster and sounds more coherent than a pack that reinvents its own structure mid-session. The goal is to eliminate architecture decisions from the creative session entirely, so the only work left is sound.

---

## 2. Template A: Drum Kit Pack

*For ONSET and future percussion engines.*

**Program count**: 8 total — 4 core kits + 4 variation kits.

Core kits should represent a complete sonic range on their own. Variation kits explore altered takes: pitched down, heavily processed, stripped back, or genre-specific.

**Voice map per core kit (16 pads)**:

| Pad | Voice Type | Notes |
|-----|-----------|-------|
| A01 | Kick 1 | Primary, full-weight |
| A02 | Kick 2 | Alt character (shorter, pitched, or distorted) |
| A03 | Snare 1 | Primary, center-weighted |
| A04 | Snare 2 | Alt (ghost, rimshot, or layered) |
| A05 | Closed Hat 1 | Tight, staccato |
| A06 | Closed Hat 2 | Looser or slightly open |
| A07 | Open Hat | Sustaining, choke target |
| A08 | Clap | Layered or processed |
| B01 | Percussion 1 | Conga, bongo, clave, or tom |
| B02 | Percussion 2 | Contrast with B01 |
| B03 | FX 1 | Riser, noise, or atmosphere |
| B04 | FX 2 | Downward sweep, hit, or tail |
| B05–B08 | Open | Fills, accents, or engine-specific voices |

9 defined voice types across 12 assigned pads. Pads B05–B08 are deliberately open — fill with what the kit needs, not with what the template expects.

**Velocity layers** — use Vibe's curve on all voices:

| Layer | Range | Character |
|-------|-------|-----------|
| 1 | 1–40 | Ghost, whisper — subdued transient |
| 2 | 41–90 | Groove — working velocity, where most playing lands |
| 3 | 91–110 | Accent — noticeable timbre shift, not just louder |
| 4 | 111–127 | Full hit — maximum transient, brightest filter state |

Timbre must shift between layers, not just amplitude. Layer 1 and Layer 4 should be distinguishable in a mix with the fader up.

**Choke groups**:
- Hat group: Closed Hat 1, Closed Hat 2, Open Hat share one choke group. Open Hat is the choke target — it cuts when any hat is triggered.
- Snare choke (optional): Snare 1 and Snare 2 choke each other only if both are sustaining variants. For transient snares, choke is unnecessary.

**DNA target ranges**:
- Movement: 0.7–0.9 (drums are inherently rhythmic — they should animate the DNA read)
- Aggression: 0.4–0.8 (vary across kits — one leans low, one leans high; do not cluster)
- Space: 0.3–0.6 (drums are present, not ambient — reverb is a spice, not a bath)

**Naming convention** — the 4 core kits follow a time-of-day arc:

| Kit | Name | Tonal Character |
|-----|------|----------------|
| Core 1 | Dawn Kit | Open, clean, light transients |
| Core 2 | Noon Kit | Forward, dry, punchy |
| Core 3 | Dusk Kit | Warmer, slightly saturated |
| Core 4 | Midnight Kit | Dark, heavy, processed |

This is not a literal instruction (Dawn does not mean acoustic brushes). It is a tonal arc from open to saturated. Variation kits depart freely from this arc.

---

## 3. Template B: Melodic Instrument Pack

*For OBLONG, OPAL melodic programs, ORACLE, and any chromatic-range engine.*

**Program count**: 16 total in 4 tiers:

| Tier | Count | Description |
|------|-------|-------------|
| Foundation | 4 | Core engine voice — clean, playable, unprocessed or lightly touched |
| Variations | 4 | Envelope, filter, or character twist on a foundation preset |
| Performance | 4 | Macro-ready, expressive, designed for real-time play |
| Experimental | 4 | Pushed processing, unusual tuning, or generative behavior |

**Keyboard range**: C0–C8 full chromatic, 61 keygroups standard. Do not leave gaps. Extreme registers can use pitch-shifted root samples when full multisampling is impractical — this is acceptable. Audible pitch artifacts at the extremes are not.

**Velocity layers**: 4-layer Vibe curve on all programs (same breakpoints as Template A). Foundation presets may use 2 layers if the sound source does not benefit from 4 — document the exception.

**DNA spread rule**: Across the 16 programs, at least one program must sit at the feliX extreme (bright, harmonically rich, forward) and at least one at the Oscar extreme (dark, subdued, recessive). The remaining 14 span between. Do not cluster all presets in the middle range — a DNA read that shows 16 programs in a tight center band is a design failure.

**Naming rules**:
- 3 words maximum
- No articles (no "The", "A", "An")
- Prefer noun + noun or adjective + noun: "Copper Thread", "Bone Wire", "Iron Shelf"
- Avoid generic descriptors — not "Warm Pad", not "Big Lead", not "Soft Keys"
- Names should be distinguishable at a glance in an MPC program list

---

## 4. Template C: Atmospheric / Texture Pack

*For OPAL atmospheric programs, OCEANDEEP, and future ambient-forward engines.*

**Program count**: 8–12 programs. No strict structural split — let the concept breathe.

**Sample coverage**: Programs may use a single root note with a long sustained sample rather than full chromatic coverage. Looping and granular stretch are expected. If chromatic coverage is included, it is a bonus, not a requirement.

**DNA minimum targets**:
- Movement: ≥ 0.6 (texture packs must breathe and shift — a frozen atmospheric is a field recording, not a patch)
- Space: ≥ 0.7 (wide, roomy, environmental — this is the defining characteristic of the template)
- Aggression: 0.0–0.4 (atmospheric packs stay low; push above 0.4 only for intentional tension programs, and note it explicitly)

**Envelope rule**: Release time must be ≥ 2× attack time on all programs. Atmospheric sounds decay gracefully. If attack is 200ms, release must be at least 400ms. This is a hard minimum. Violation means the envelope is mismatched to the identity of the template.

**Naming** — evocative of physical environments. Prefer two-word combinations drawing from sensory or geographic imagery: "Harbor Fog", "Stone Hall", "Salt Shelf", "Cave Breath", "Canopy Drift". Avoid abstract adjective stacks — "Deep Wide Warm" says nothing a producer can orient to.

---

## 5. Program Naming Vocabulary

Word banks by role. Combine words across banks for contrast. Avoid combining two words from the same bank — "Pulse Crack" doubles percussion terms and reads redundant.

**Percussion / rhythmic** — use in Template A:
tide, pulse, crack, rim, ghost, snap, crush, knock, rattle, strike, skin, grain

**Melodic / tonal** — use in Template B:
thread, wire, bone, breath, copper, iron, glass, reed, coil, root, vein, stem

**Atmospheric** — use in Template C:
fog, drift, shelf, cave, canopy, salt, ash, haze, basin, swell, glow, bloom

**Intensity markers** — use as modifiers in any template (not as standalone names):
deep, shallow, wide, tight, raw, warm, cold, hollow, dense, sparse, still, heavy

Usage rule: pick one word from one content bank, one word from intensity markers, and combine. "Deep Crack", "Still Thread", "Raw Fog" — two-word names that locate the sound in a physical register without over-specifying.

---

## 6. Cover Art Brief Template

Fill in all six fields before briefing a designer or generating artwork. An incomplete brief produces art that needs revision. A complete brief produces art that ships.

```
Engine accent color:       [hex, e.g. #A78BFA]
Secondary / background:    [hex or plain descriptor, e.g. deep navy #0A0A1A]
Pack mood (one sentence):  [e.g. "Industrial percussion — heavy and mechanical, no warmth"]
Key image word:            [one noun — the visual anchor, e.g. "chain", "coral", "ember"]
Typography style:          [e.g. "wide-tracked mono caps", "serif italic", "hand-stencil"]
Avoid:                     [one or two things that would be wrong — e.g. "no organic textures", "no warm tones"]
```

**Key image word** is the most important field. Give the designer a noun, not a direction. "Ember" works. "Something glowing and warm" does not. The noun gives them a concrete visual starting point; the interpretation is theirs.

**Avoid** is the second most important field. Telling a designer what to exclude is often more useful than describing what to include. One sentence. Be specific.

---

*Templates live in R&D. Update them when a pack ships and reveals a structural gap. The value of a template system is accumulation of correct defaults — not premature lock-in.*
