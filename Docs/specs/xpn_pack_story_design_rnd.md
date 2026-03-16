# Story-First Pack Design — R&D

**Date:** 2026-03-16
**Scope:** A methodology for designing XPN packs around a narrative concept rather than starting from samples. The story is written first; all subsequent decisions — DNA targets, Q-Link assignments, velocity layers, naming vocabulary — are derived from it.

---

## 1. What Is a Pack Story?

A pack story is a single sentence that places the producer inside a world. It is not a genre label, not a mood adjective, not a marketing tagline. It is a scene.

The test: could a filmmaker use this sentence as a one-line location description? If yes, it is a pack story. If it reads like sleeve copy, it is not.

**Working examples:**

| Pack | Engine | Story Sentence |
|------|--------|---------------|
| Iron Machines | ONSET | A drum kit factory floor at 3am, when the machines are running unattended. |
| Copper Thread | OBLONG | A West African village at dusk where someone is playing a melodic instrument and the sound carries over water. |
| Gut Physics | OBESE | The physical sensation of low-frequency sound pressure in a concrete room. |
| Glass Lung | OBBLIGATO | A conservatory practice room in winter, two wind players running the same passage until it falls apart. |
| Shore Debt | OSPREY | A sea cliff in fog where the tide is coming in and the light is going. |
| Amber Circuit | OVERWORLD | An arcade at closing time — the machines still running, no one playing them. |
| Drift Logic | ORACLE | A forecast office, 4am, watching a weather system that will not resolve. |

**What a story sentence does:**

- Establishes physical space → reverb character, dynamic range
- Establishes time and light → brightness balance in DNA
- Establishes human presence (or absence) → feliX / Oscar polarity
- Establishes cultural context → instrument vocabulary, tuning reference
- Establishes emotional weather → the macro axis that matters most

A single sentence cannot carry all of this explicitly. It does not need to. The detail that is missing becomes an interpretive decision the designer makes consciously, documented in the brief (Section 4).

---

## 2. Story Elements That Shape Pack Design

Four story elements translate directly into design decisions. Every element is resolved before a single sample is rendered.

### 2.1 Time of Day / Atmosphere

Time and atmosphere govern the brightness–warmth balance in Sonic DNA.

| Condition | DNA Implication |
|-----------|----------------|
| Pre-dawn / blue hour | brightness 0.2–0.4, warmth 0.2–0.3, space 0.7–0.9 |
| Morning light / open air | brightness 0.6–0.8, warmth 0.4–0.5, space 0.5–0.7 |
| Midday / overhead sun | brightness 0.8–1.0, warmth 0.3–0.5, density 0.6–0.8 |
| Dusk / golden hour | brightness 0.5–0.7, warmth 0.7–0.9, movement 0.4–0.6 |
| Night / artificial light | brightness 0.2–0.5, warmth 0.6–0.8, aggression 0.4–0.7 |
| Night / no light | brightness 0.1–0.2, warmth 0.4–0.6, space 0.8–1.0 |
| Interior / diffused | warmth dominant, space 0.3–0.5, density 0.5–0.7 |

The Sonic DNA values are targets for the rendered audio, not descriptions of the synthesis parameters. A warm, low-brightness sound can be produced through any of a dozen DSP routes. The DNA is the destination; the engine settings are the map.

### 2.2 Geographic and Cultural Context

Cultural context determines instrument vocabulary and tuning system. It also governs rhythmic subdivision assumptions and velocity curve character.

| Cultural context | Instrument reference | Tuning reference | Velocity character |
|-----------------|---------------------|------------------|--------------------|
| West African | Kora, balafon, djembe, talking drum | Equal temperament, but with strong preference for pentatonic and hexatonic | Sharp transients, long release tails, ghost notes |
| South Asian | Sitar, tabla, bansuri, harmonium | Just intonation variants, significant microtonal ornament | Dense velocity range, 64–127 heavily differentiated |
| East Asian | Guqin, erhu, sheng, taiko | Pentatonic core, equal temperament | Attack-dominant, wide range, few mid-velocity layers |
| Mediterranean | Oud, frame drum, nay, darbouka | Maqam-adjacent, quarter-tone inflections | Smooth curves, strong mid-velocity presence |
| Northern European | Piano, bowed strings, choir | Equal temperament | Gradual, linear, soft-note priority |
| Electronic / industrial | Machines, physical feedback, noise | No tuning reference; pitch is incidental | Binary (soft = noise floor, hard = clipping) |

This is not a requirement to use ethnically sourced samples. It is a constraint on what the sounds should *feel like* — their dynamic behavior, their decay shape, their pitch relationship to each other within a program.

### 2.3 Physical Space

The physical space sets the reverb character, stereo width policy, and dynamic range ceiling.

| Space | Reverb character | Stereo width | Dynamic range |
|-------|-----------------|--------------|---------------|
| Concrete room (live) | Short pre-delay, long tail, high reflectivity, filtered highs | Wide on release, narrow on attack | Compressed — room absorbs peaks |
| Forest / exterior | No early reflections, sparse late tail, decaying into silence | Narrow source, wide ambient decay | Uncompressed — large natural headroom |
| Water surface / near water | Doppler flutter on decays, low-frequency build on sustained notes | Stereo image drifts with wind simulation | High natural dynamic range |
| Small room / interior | Short tail, strong comb filtering, high density | Mono dominant | Dry, limited |
| Cathedral / large hall | Long pre-delay (60–120ms), very long tail (4–8s), smooth | Wide, enveloping | Soft |
| Open urban (street) | Reflective but diffuse, medium tail, irregular | Variable — sources in space | High |
| Studio (treated) | No tail / very short, controlled | Center dominant | Reference flat |

### 2.4 Emotional State: feliX / Oscar Polarity

Every XO_OX pack has a feliX–Oscar polarity. This is not about tempo or genre. It is about the emotional intelligence of the sounds.

**feliX (the neon tetra):** precise, clinical, alert, fast, hyperaware. Sounds that cut. Sounds that are exactly where they say they are. Transients that land without apology.

**Oscar (the axolotl):** warm, organic, tired, human, slow-to-decide. Sounds that breathe. Sounds with a slight slur at the beginning. Sounds that lean into the beat rather than landing on it.

Most packs live between the poles. The story sentence usually reveals where.

| Story element | feliX pressure | Oscar pressure |
|---------------|---------------|----------------|
| Machines, factory, circuit | High | Low |
| Night, unattended, running without humans | Moderate | Moderate (eerie feliX — precise but purposeless) |
| Village, dusk, human presence | Low | High |
| Physical sensation (bass pressure, cold) | Low | High |
| Forecasting, data, watching | High | Low |
| Practice room, winter, falling apart | Moderate feliX start → Oscar end | Decay into Oscar |

The polarity informs:
- Attack shape (feliX: fast, steep / Oscar: slow, rounded)
- Tuning stability (feliX: stable / Oscar: slight drift, humanized)
- Velocity response (feliX: exponential jump / Oscar: smooth ramp)
- Q-Link priority (feliX: CUTOFF, TUNE, GATE / Oscar: DECAY, WARMTH, WIDTH)

---

## 3. Story → Design Decisions Matrix

The matrix converts a story sentence into seven concrete design decisions. Complete one row per pack before any audio work begins.

| Story Element | DNA Target | feliX/Oscar | Q-Link Priority | Velocity Layer Character | Naming Vocabulary |
|---------------|-----------|-------------|-----------------|--------------------------|-------------------|
| "3am factory floor, machines unattended" | brightness 0.3, aggression 0.7, warmth 0.2, space 0.6 | feliX 0.8 | DRIVE, CUTOFF, TUNE, MUTE | L1: soft machine tick (controlled, distant) → L4: hard metallic impact (violent, close) | Press Cycle, Stamp Gate, Pneumatic Hit, Release Valve, Idle Tick |
| "West African village at dusk, sound over water" | brightness 0.5, warmth 0.8, movement 0.6, space 0.7 | Oscar 0.75 | DECAY, WIDTH, RESONANCE, CHARACTER | L1: breath before attack → L4: full strike with resonance tail | Kora Dawn, Evening Phrase, Water Return, Crossing Hit |
| "Low-frequency pressure in concrete room" | brightness 0.15, warmth 0.6, aggression 0.8, density 0.9 | Oscar 0.7 | DRIVE, DECAY, WIDTH, GATE | L1: felt displacement (sub) → L4: room saturation point | Gut Drop, Floor Swell, Concrete Bloom, Pressure Wall |
| "Conservatory winter, two players, falling apart" | brightness 0.6, movement 0.7, warmth 0.5, space 0.4 | feliX→Oscar arc | TUNE, DECAY, BLEND, CHARACTER | L1: clean attack, in tune → L4: drift, late, over-blown | First Passage, Second Passage, Slipping, Gone |

**Reading the matrix:**

The DNA target is a finishing constraint. Every pad rendered into this pack must score within ±0.1 of the target values when evaluated. If a pad falls outside range, it either belongs to a different pack or requires re-render.

The Q-Link priority column lists the four Q-Links in left-to-right order. Q-Link 1 is the one the producer touches first, the one that does the most for the money. It should be immediately musical.

The velocity layer character column describes the arc from soft to hard — what happens to the sound, not just to the volume.

The naming vocabulary column is a word bank, not a final list. At least 70% of program names in the finished pack should draw from this vocabulary or its immediate semantic neighbors.

---

## 4. Writing the Pack Story Brief

One brief per pack. Maximum one page. Written before any rendering begins. The brief is the designer's contract with the story.

**Template:**

```
PACK STORY BRIEF
Pack name (working): [name]
Engine: [engine]
Date: [date]

STORY SENTENCE
[One sentence. Scene-level. See Section 1.]

REFERENCE IMAGES
1. [Describe a specific photograph, painting, or film still — what you see, not what it means]
2. [Second image — different medium or era from the first]
3. [Third image — one that does not obviously fit, but feels right]

REFERENCE SOUNDS
1. [Specific track or recording — artist, title, the moment within it that matters]
2. [Second reference — ideally from a different decade or genre]
3. [Third reference — the one that surprises, the oblique reference]

FORBIDDEN ZONES
This pack is NOT:
- [What genre or production style it must never sound like]
- [What emotional register is explicitly excluded]
- [What technical approach (reverb type, tuning system, velocity curve) is banned]

THE WRONG-BUT-RIGHT ELEMENT
[One unexpected detail that will make this pack memorable. Something that breaks the internal logic of the story in a way that produces a better sound design outcome. Name it explicitly and describe why it belongs.]

DNA TARGETS
brightness: [0.0–1.0]    warmth: [0.0–1.0]    movement: [0.0–1.0]
density: [0.0–1.0]       space: [0.0–1.0]      aggression: [0.0–1.0]

feliX / Oscar polarity: [0.0 = pure Oscar, 1.0 = pure feliX]

Q-LINK ASSIGNMENT (left to right)
Q1: [parameter name + rationale]
Q2: [parameter name + rationale]
Q3: [parameter name + rationale]
Q4: [parameter name + rationale]

NAMING VOCABULARY
[10–15 words or short phrases. These are seeds, not final names.]
```

**The wrong-but-right element** deserves emphasis. It is the most important field in the brief and the one most often left blank. Every memorable pack contains one sound or design choice that violates the internal logic of the story and is better for it. "Iron Machines" might include one pad that sounds like a breath — biological in a mechanical world. "Copper Thread" might include one pad that is digitally quantized — precise in a human world. The wrong-but-right element is not an accident. It is identified before the work starts and protected throughout.

---

## 5. Story Consistency Check

When a pack is complete, audit it against the brief before export. This is a 10-point check. Each point is pass or fail. A pack with fewer than 8 passes requires revision.

| # | Check | Pass Condition |
|---|-------|----------------|
| 1 | DNA range | Every program's 6D DNA scores within ±0.15 of brief targets |
| 2 | Naming vocabulary | ≥ 70% of program names draw from brief vocabulary or direct semantic neighbors |
| 3 | Q-Link 1 immediacy | Q-Link 1 produces a musically useful result within 2 seconds of random turning |
| 4 | Velocity arc | Soft hits and hard hits feel like the same scene, not two different packs |
| 5 | Forbidden zones | No program in the pack sounds like anything in the forbidden zones list |
| 6 | feliX/Oscar coherence | All programs share a consistent polarity character (±0.2 variance across the pack) |
| 7 | Wrong-but-right element | The wrong-but-right element is present and audible — not cut in revision |
| 8 | Space consistency | Reverb character across all programs is coherent (same room, same distance, same air) |
| 9 | Cultural vocabulary | Instrument choices, tuning reference, and velocity behavior are consistent with the geographic/cultural context in the brief |
| 10 | Story sentence test | A producer who does not know the brief could construct a sentence within 30 words of the original story sentence after listening to the full pack |

Point 10 is the hardest and the most important. The story must be audible without explanation. If the brief says "3am factory floor" but a producer who listens blind describes the pack as "warm jazz room samples," the story failed to reach the audio.

---

## 6. Collection Stories

A collection is a group of packs connected by an overarching story. The collection story does not replace the individual pack stories — it frames them.

**Structure:**
- Collection story: one sentence, same scene-level standard as pack stories
- Each pack story extends, contrasts, or inhabits a specific part of the collection world

**Kitchen Collection:**

> "The kitchen as the most honest room in a house — where things are made, not performed."

Every pack in the Kitchen Collection must fit inside this frame. The frame implies:
- No decorative sounds — everything has a function
- Process as subject matter (preparation, transformation, heat, time)
- Human scale — domestic, not monumental
- Honesty as an aesthetic value — sounds that do not try to be beautiful

Individual pack stories within Kitchen might include:
- "A heavy cast iron pan heating on a gas flame, the oil just beginning to move" (ONSET — industrial-domestic percussion)
- "Four hands working the same bread dough, different rhythms, same batch" (OBBLIGATO — paired wind / rhythm)
- "The refrigerator running at 2am, the only sound in the house" (OBLONG — tonal drone, machinery as melody)
- "Spice jar lids in a drawer, ceramic against glass" (ORIGAMI — folded, textural, percussive)

The collection story creates a constraint that keeps individual pack designers from drifting apart. When a pack designer asks "does this pad belong?" the collection story is the first test, before the pack story.

**Collection story tests:**

| Test | Question |
|------|----------|
| Inclusion | If someone described this sound using the collection story's vocabulary, would they be right? |
| Exclusion | Does this sound require a vocabulary that contradicts the collection story? |
| Coherence | If all packs in the collection are played simultaneously, do they inhabit the same world? |

A collection story should fail the exclusion test for at least 50% of sounds in the world — if everything fits, the story is not constraining anything. The constraint is what creates identity.

---

## Summary

Story-first pack design operates as a cascading constraint system:

1. **Story sentence** establishes the world (one sentence, scene-level)
2. **Story elements** translate the sentence into four design axes: atmosphere, culture, space, polarity
3. **Design matrix** converts the four axes into seven concrete decisions before any audio is produced
4. **Story brief** formalizes the decisions into a one-page contract, including reference images, reference sounds, forbidden zones, and the wrong-but-right element
5. **Consistency check** audits the finished pack against the brief — the story must be audible without explanation
6. **Collection story** frames a group of packs inside a larger world, adding cross-pack coherence constraints

The wrong-but-right element is the differentiator. Sample-first pack design produces technically accurate sounds. Story-first pack design produces sounds with memory — the one detail that does not belong, that the producer keeps coming back to, that makes the pack unrepeatable.
