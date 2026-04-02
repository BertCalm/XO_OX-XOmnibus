# XPN Preset & Program Naming Conventions — R&D Spec
**XO_OX | March 2026**

> *"A name is a promise. Before the producer hears a single sample, the name tells them what kind of world they are entering."*

---

## Overview

This document defines the canonical naming system for XO_OX XPN pack programs and presets. It covers MPC screen constraints, vocabulary by engine family, collection-level coherence, version suffixes, forbidden patterns, alphabetical arc strategy, the `.xometa`-to-XPM truncation contract, and the naming QA process.

Every rule here derives from one principle: **names are the first layer of sound design.** A producer scanning an MPC screen is already forming expectations. A good name shapes those expectations correctly. A bad name breaks trust before the sample plays.

---

## Section 1: Program Name Rules

### 1.1 The 20-Character Hard Limit

MPC hardware truncates program names at 20 characters on-screen. This is not a guideline — it is a physical constraint. Any character beyond position 20 is invisible to the producer on-device.

**Rules:**
- Maximum 20 characters including spaces and punctuation
- Count before naming: "Iron Chest 808" = 14 chars. "Glass Snap Kit" = 14 chars. "Bioluminescent Cave" = 19 chars. All valid.
- Do not rely on the MPC Software (desktop) view to check — always count raw characters

**Counting tool (mental):** Five words of four letters each with spaces = 24 chars. Too long. Keep it to two strong words or three short ones.

### 1.2 No Abbreviations

Abbreviations save characters at the cost of evocation. A name that needs decoding has already failed.

- "Drm Mch A" — meaningless
- "Iron Chest" — immediate image

Exceptions: genre abbreviations that are themselves proper nouns (`808`, `909`, `TR-X`). These carry cultural weight and are permitted when accurate.

### 1.3 No Generic Words

Generic words produce generic expectations. The forbidden vocabulary list is in Section 5. The diagnostic test: could this name appear on any other company's pack? If yes, rename it.

- "Hip Hop Kit" → anyone could name it that. Fail.
- "Tar Throat" → only XO_OX names things like this. Pass.

### 1.4 Names Must Evoke the Sound Character

The name should activate a sensory prediction — texture, weight, material, environment. The producer should hear something in their imagination before the sample plays. When the sample confirms or interestingly violates that prediction, trust is built.

**Reference examples from existing work:**

| Name | Why It Works |
|------|-------------|
| Iron Chest 808 | Material (iron) + anatomy (chest) + machine ID. Implies weight and resonance. |
| Glass Snap | Brittle, bright, instantaneous. The transient is in the name. |
| Tar Vein | Viscous, dark, subsonic. Bass character without saying "bass." |
| Dusk Pier | Location + light condition. Nostalgic + spatial. |
| Fold Memory | Action + abstraction. Origami logic applied to recall. |
| The Wet Archive | Article + adjective + noun. Bureaucratic structure meeting moisture. Slightly wrong in a compelling way. |

---

## Section 2: Naming Vocabulary by Engine Type

### 2.1 Drum Kits — ONSET

ONSET kits name the sonic character of the drum machine being channeled or invented — not the genre, not the decade, not a number.

**Vocabulary fields:**
- **Material + body part:** Iron Chest, Brass Lung, Carbon Jaw, Glass Spine
- **Machine + quality:** Chrome Pulse, Steel Ghost, Tin Revenant
- **Environment + event:** Gravel Storm, Ash Snap, Concrete Bloom
- **Genre-adjacent machine IDs** (when accurate and evocative): Rust 808, Cracked 909

**Examples — correct:**
```
Iron Chest 808
Glass Snap
Ash Throne
Carbon Shuffle
Gravel Choir
```

**Examples — incorrect:**
```
Hip Hop Kit 1     ← genre + generic + number
Drum Program A    ← category + letter
Hard Trap Kit     ← genre descriptor only
Boom Bap Classic  ← genre nostalgia, no sonic specificity
```

**Format guidance:** Two to three words. The machine ID suffix (`808`, `909`, `LM-1`) is permitted when the sound actually references that machine. Invented machine names (not real hardware) are not permitted — they mislead.

### 2.2 Melodic Engines — OBLONG, OPAL, OVERWORLD, OBLIQUE, and family

Melodic programs describe a scene, not a timbre. The producer should be able to place themselves physically in the name.

**Vocabulary fields:**
- **Time + place:** Dusk Pier, Morning Inlet, Late Market
- **Material + light:** Copper Thread, Amber Column, Glass Meridian
- **Animal + action:** Moth Choir, Crane Circuit, Kelp Drift
- **Phenomenon:** Thermal Veil, Pressure Seam, Crown Flash

**Format guidance:** Two words preferred. Three words permitted when the third word adds essential specificity (not decoration). No articles ("A," "The") in melodic program names — reserve those for atmospheric programs.

**The two-word rule:** The two words should come from different semantic fields. "Dusk Pier" = time + physical structure. "Copper Thread" = material + dimension. Two words from the same field collapse ("Dark Shadow," "Bright Light") — avoid.

**Examples — correct:**
```
Dusk Pier
Copper Thread
Moth Choir
Thermal Veil
Crane Circuit
```

**Examples — incorrect:**
```
Warm Pad        ← timbre descriptor, no scene
Lush Strings    ← adjective + instrument category
Soft Bells      ← adjective + instrument category
Electric Dream  ← overused compound (appears in 1,000 packs)
```

### 2.3 Bass Engines — OBESE, OVERBITE, OCEANIC (bass roles)

Bass programs use tension and physicality language. The body is the reference frame. Weight, compression, impact, anatomy.

**Vocabulary fields:**
- **Anatomy + sensation:** Gut Punch, Jaw Drop, Spine Drag, Chest Crush
- **Material + weight:** Tar Vein, Lead Thread, Gravel Lung, Carbon Drop
- **Event + consequence:** Wall Fall, Floor Split, Chest Collapse
- **Pressure language:** Pressure Seam, Sub Floor, Deep Push

**Format guidance:** Short and blunt. Two words only. Verbs as nouns are powerful ("Gut Punch," "Jaw Drop"). Avoid adjectives — use nouns and verbs exclusively where possible.

**Examples — correct:**
```
Gut Punch
Tar Vein
Jaw Drop
Spine Drag
Lead Floor
```

**Examples — incorrect:**
```
Dark Bass       ← timbre + category
Big Sub         ← size + instrument type
Heavy Drop      ← "heavy" is overused; no physical specificity
Dirty 808       ← "dirty" is a genre-context word, not a physical description
```

### 2.4 Atmospheric Engines — ORACLE, ORIGAMI, OBSCURA, ORGANON, OUROBOROS, OMBRE

Atmospheric programs are poetic, slightly ambiguous, and allowed to be unsettling. The producer should not be entirely sure what to expect. The name promises texture and space, not melody or rhythm.

**Vocabulary fields:**
- **Abstract + concrete:** Fold Memory, Pressure Seam, The Wet Archive
- **Process + material:** Slow Silt, Oxidized Signal, Buried Frequency
- **Institutional + decay:** Archive Leak, Catalog Error, Station Drift
- **Articles permitted:** "The" as a distancing device. "The Wet Archive," "The Second Corridor."

**Format guidance:** Two to four words. Articles ("The," "A") are permitted — they create bureaucratic or documentary distance that suits these engines. Slightly wrong adjective-noun pairings are encouraged ("Wet Archive" — archives are not wet, which is exactly the point).

**Examples — correct:**
```
Fold Memory
The Wet Archive
Pressure Seam
Archive Leak
Slow Silt
Oxidized Signal
The Second Corridor
```

**Examples — incorrect:**
```
Ambient Pad     ← category descriptor
Dark Texture    ← overused
Space Drone     ← generic ambient vocabulary
Mystery Sound   ← "mystery" is never specific enough
```

---

## Section 3: Collection Naming Conventions

Within a collection, program names share a vocabulary domain without being repetitive. The collection provides the conceptual frame; individual names inhabit it distinctly.

### 3.1 Kitchen Collection

**Frame:** Culinary materials, actions, and states. Not dish names — processes and substances.

**Vocabulary:** Reduction, Cure, Brine, Char, Steep, Render, Press, Clarify, Bloom, Temper, Smoke, Rest

**Format:** Culinary verb as standalone noun, or verb + material. "Char Point." "Salt Bloom." "Press Cure." "Smoke Column."

**What to avoid:** Dish names ("Bolognese," "Risotto"), brand names, utensil names as primary identifiers.

**Coherence check:** Read all names in the collection aloud. They should feel like they belong to one kitchen — not five different restaurants. Shared vocabulary: temperature, transformation, time.

### 3.2 Travel / Water / Vessels Collection

**Frame:** Geographic and maritime reference. Coordinates, coastlines, weather events, vessel behavior.

**Vocabulary:** Inlet, Channel, Shoal, Fetch, Surge, Tide Mark, Shore Run, Leeward, Swell, Draft, Ballast, Beacon

**Format:** Geographic noun or maritime term. "North Fetch." "Shoal Line." "Tide Mark." "Leeward Signal."

**Era wildcard names** should feel like they belong to the same voyage — same ship, different conditions. Chronological arc permitted.

### 3.3 Artwork / Color Collection

**Vocabulary:** Art movement + color field. Reference painterly process and optical phenomena, not color names directly.

**Vocabulary:** Ground, Varnish, Underpainting, Wash, Glaze, Impasto, Study, Recto, Verso, Cadmium, Umber, Sienna, Viridian, Carmine

**Format:** Pigment name or painterly process as program name. "Raw Umber Study." "Cadmium Ground." "Viridian Wash."

**What to avoid:** Color adjectives as primary word ("Red Pad," "Blue Bass"). The color word should be a substance name, not a description.

### 3.4 Cross-Collection Rule

Programs from different collections must never share a name, even if the collections are sold separately. The XO_OX fleet is one catalog. Duplicates fragment the mental map producers build over time.

Maintain a running cross-fleet name ledger. Before finalizing any collection program list, search the ledger.

---

## Section 4: Version Suffix Convention

### 4.1 `v1`, `v2` Suffixes

A version suffix signals a significant variation of the same program — different macro range, different tuning, different character macro setting — not a minor tweak.

**Use when:**
- The program shares the same sample source but the envelope, filter, or pitch character is substantially different
- You would describe the variation to a producer as "a different approach to the same sound"

**Do not use when:**
- Adjusting volume or pan
- Minor EQ change
- Anything a producer would not notice without A/B comparison

**Format:** append ` v2` (space + v2) to the base name. Character count applies to the full string. "Iron Chest 808 v2" = 18 chars. Valid.

### 4.2 `(DUB)` and `(DRY)` Suffixes

Use these to distinguish wet and dry variants of the same program when both are included in a pack.

**`(DRY)`** — the program with all reverb, delay, and modulation effects at zero or bypassed. The unprocessed signal.

**`(DUB)`** — the program with reverb and delay pushed to performance-ready levels. High-mix, performance-optimized.

**Format:** append ` (DUB)` or ` (DRY)` to the base name. Count the characters. "Tar Vein (DRY)" = 14 chars. Valid. "Bioluminescent Cave (DUB)" = 25 chars. Over limit — shorten the base name before adding the suffix.

**Rule:** If both DUB and DRY variants are included, both must exist in the pack. Do not include only one variant using the suffix — the suffix implies a pair.

---

## Section 5: Forbidden Patterns

The following patterns are banned across all XO_OX XPN programs and presets. These are not preferences — they are disqualifying.

### 5.1 Engine Name as Primary Identifier

Starting a program name with the engine name is redundant and wastes the name's evocative budget. The producer already knows which engine they loaded.

- "OBESE Dark Bass" → fail. Use "Tar Vein."
- "ONSET Trap Kit" → fail. Use "Carbon Shuffle."
- "OPAL Granular Pad" → fail. Use "Moth Choir."

### 5.2 Numbers as Primary Identifier

Numbers communicate nothing about sound character. They communicate production laziness.

- "Pad 04" → fail
- "Kit 12" → fail
- "Bass 003" → fail

Machine ID suffixes (`808`, `909`) are not "numbers as primary identifier" — they are proper nouns with cultural meaning. They are permitted only when accurate.

### 5.3 Generic Timbre Descriptors

The following words are prohibited as primary or sole descriptors:

```
Warm, Dark, Bright, Soft, Hard, Deep, Lush, Rich, Fat, Thick,
Clean, Dirty, Heavy, Light, Smooth, Raw, Classic, Modern, Fresh,
Crisp, Punchy, Tight, Loose, Big, Small, Huge, Tiny
```

These words describe virtually every sound in every pack ever released. They communicate nothing. When a designer reaches for one of these words, it means the program does not yet have a name — it has a placeholder.

**Diagnostic question:** Would this description appear in a stock Kontakt library? If yes, rename.

### 5.4 Generic Category Names

```
Kit, Pad, Bass, Lead, Chord, Stab, Arp, Pluck, Bell, Drone, Texture
```

These words may appear as a secondary word in a specific compound, but never as the primary identifier alone. "Moth Choir" — "choir" is specific and evocative. "Warm Pad" — "pad" is a category, "warm" is a generic descriptor. Both fail.

---

## Section 6: Sorting Strategy

MPC hardware displays programs alphabetically within a pack. This is not a problem — it is a design opportunity.

**Principle:** Alphabetical order should create a logical arc from foundation to experimental.

### 6.1 Arc Design by Program Type

**Drum kits:** Heaviest, most foundational kits first (A-G), mid-weight character kits in the middle (H-P), experimental and textural kits last (Q-Z).

**Melodic packs:** Anchor sounds first (recognizable, melodic, in-key-friendly), then harmonic character sounds, then detuned/processed/ambient variants last.

**Bass packs:** Sub-dominant, clean sounds first. Character and distorted variants last.

### 6.2 Tactical First-Letter Use

Name the program you want producers to reach first with a letter that sorts to the front. If "Iron Chest 808" is the flagship kit, it sorts to position I. If you want it first, the kit must be renamed to start with A or B, or all other kits must start with J-Z.

Design the full name list before committing any individual name. Lay them out in alphabetical order and read the sequence as an arc. Ask: does this collection tell a story from start to end?

### 6.3 Avoid Accidental Clusters

Do not let naming choices accidentally cluster three programs under the same first letter. Three "C" names in a 10-program pack pushes them all to the same region and breaks the arc.

Audit the initial-letter distribution before finalizing. Aim for no more than two programs per starting letter in packs under 20 programs.

---

## Section 7: Preset vs. Program Naming

### 7.1 The Two Contexts

`.xometa` presets are the source format — viewed in XOceanus plugin, on desktop, in catalog listings. Maximum 30 characters. Full evocative name in this context.

XPM programs are the MPC export format — viewed on MPC hardware screen. Maximum 20 characters.

The same sound has two names: one for the plugin context (30-char), one for the MPC program (20-char).

### 7.2 Truncation Contract

When a `.xometa` name exceeds 20 characters, the XPM program name is not an automatic truncation — it is a deliberate re-name that preserves meaning.

**Automatic truncation fails:**
```
.xometa: "Bioluminescent Drift"    (20 chars — fine, equal)
.xometa: "The Second Corridor"     (19 chars — fine)
.xometa: "Oxidized Signal Path"    (20 chars — fine)
.xometa: "Pressure Anomaly Valve"  (22 chars — too long for XPM)
XPM truncation: "Pressure Anomaly V"  ← meaningless fragment
XPM re-name:    "Pressure Valve"      ← 14 chars, preserves meaning
```

**Rule:** Any `.xometa` name over 20 characters requires a deliberately written XPM short form — not a character slice.

### 7.3 Tracking the Two Names

The XPN manifest should record both names explicitly:

```json
{
  "xometa_name": "Pressure Anomaly Valve",
  "xpm_name": "Pressure Valve",
  "engine": "ORACLE"
}
```

When these diverge, both must be intentional. The XPM name is not a fallback — it is a second creative decision.

---

## Section 8: Naming Review Process

### 8.1 Read Aloud Test

Read every program name in the pack aloud, in the alphabetical order they will appear on the MPC screen. This test catches:

- Names that sound like other names (acoustic confusion between "Tar Vein" and "Car Lane")
- Names that are grammatically awkward when spoken
- Names that feel inconsistent with the collection's register (one formal name in a colloquial pack, or vice versa)
- Alphabetical arc problems — a sequence that peaks too early or ends flat

### 8.2 Fleet Duplicate Check

Before releasing any pack, run every program name against the cross-fleet ledger. No two programs in the XO_OX catalog may share an identical name, regardless of which pack they belong to.

**Process:**
1. Search the ledger for exact match
2. Search for near-match (one word different, same structure)
3. Flag both — near-matches are permitted if the sonic characters are genuinely different and the difference is obvious

The ledger lives at: `Tools/naming/fleet_name_ledger.txt` (one name per line, alphabetical).

### 8.3 MPC Truncation Audit

For every program name in a pack, count the characters manually. Do not trust visual inspection — count.

**Quick count method:** Write the name, mark every fifth character with a slash.

```
I r o n / C h e s t / _ 8 0 8
1 2 3 4 5 6 7 8 9 10 11 12 13 14
```

14 characters. Safe. Any count over 20 triggers the truncation contract (Section 7.2).

### 8.4 The Three Questions

Before any program name is finalized, answer these three questions:

1. **Does this name tell you what the sound feels like before you hear it?** If the answer is "somewhat" or "not really," rename.
2. **Would this name appear in a generic sample pack?** If the answer is "maybe" or "yes," rename.
3. **Does this name belong to this collection specifically?** If the answer is "it could belong to any collection," rename.

A name that passes all three is ready for the ledger.

---

## Appendix A: Approved Name Examples by Engine

| Engine | Good Examples | Banned Examples |
|--------|--------------|-----------------|
| ONSET | Iron Chest 808, Glass Snap, Ash Throne | Hip Hop Kit 1, Drum A, Classic Trap |
| OBLONG | Dusk Pier, Copper Thread, Amber Column | Warm Keys, Soft Bell, Lead 3 |
| OPAL | Moth Choir, Thermal Veil, Kelp Drift | Ambient Texture, Granular Pad, Space |
| OBESE | Gut Punch, Tar Vein, Jaw Drop | Dark Bass, Fat Sub, Heavy 808 |
| ORACLE | Fold Memory, The Wet Archive, Archive Leak | Mystery, Dark Pad, Ambient 02 |
| ORIGAMI | Pressure Seam, Slow Silt, Oxidized Signal | Fold 1, Texture A, Ambient Layer |
| OBSCURA | The Second Corridor, Station Drift, Catalog Error | Dark Atmosphere, Reverb Pad, Space 3 |

---

## Appendix B: Character Count Reference

| Length | Example | Status |
|--------|---------|--------|
| ≤ 14 chars | Iron Chest 808 | Comfortable |
| 15–18 chars | Bioluminescent Cave | Tight but valid |
| 19–20 chars | The Second Corridor | At limit — verify |
| 21+ chars | Pressure Anomaly Valve | Over limit — XPM re-name required |

---

*Document version: 1.0 — March 2026*
*Owner: XO_OX Sound Design*
*Next review: When first Kitchen Collection pack enters QA*
