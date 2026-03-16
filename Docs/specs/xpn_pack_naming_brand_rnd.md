# XPN Pack Naming & Brand Guidelines — R&D

**Date**: 2026-03-16
**Status**: Draft / Working Spec

---

## 1. Pack Naming Formula

**Formula**: `{ENGINE} {Mood Modifier} {Material Noun}`

- `ENGINE` — all-caps engine code (OBESE, ONSET, OPAL, OVERLAP, etc.)
- `Mood Modifier` — title case adjective or verb-form word (Rising, Ritual, Ghost, Deep, Slow, Burnt, Hollow)
- `Material Noun` — title case noun describing sonic material (Textures, Drums, Pads, Brass, Pulses, Cells)

**Examples**:
- `OBESE Rising Textures`
- `ONSET Ritual Drums`
- `OPAL Ghost Pads`
- `OVERLAP Slow Knots`
- `OUTWIT Hollow Cells`
- `OVERDUB Burnt Tape`
- `BITE Fang Bass`

**Constraints**:
- Max 28 characters total (fits MPC browser pack display without truncation)
- No special characters except hyphen in mood words (e.g., `Low-End` is allowed)
- No version numbers in the display name (see Section 3)
- ENGINE code must match the canonical gallery code exactly — do not abbreviate or variant it

**Edge case — collections**: When a pack belongs to a named collection, the collection identity lives in the mood/material words, not as a prefix tag (see Section 4).

---

## 2. Program / Preset Naming

MPC browser renders approximately 20–25 visible characters before truncating. Program names must front-load the most identifying information.

**Rules**:
- Max 24 characters (hard limit; aim for 20)
- Title Case throughout — not ALL-CAPS, not all-lowercase
- No engine prefix inside program names — the pack context already establishes the engine
- Leading number with zero-pad for ordered sets: `01 Kick Main`, `02 Kick Tight`, `12 Clap Wide`
- For melodic/pad programs, lead with character: `Ghost Choir`, `Deep Shimmer`, `Silt Drone`
- For drum kits, lead with role: `Kick`, `Snare`, `Hat`, `Clap`, `Tom`, `Perc`, `FX`
- Avoid filler words: not `A Nice Pad` — say `Hollow Shimmer` or `Silt Wash`

**Numbering placement**: Number leads for sets where sequence matters (drums, layers); number trails or is omitted for character-named pads where the name is the primary identifier.

---

## 3. Version Naming

Version numbers do not belong in display names. `OPAL Ghost Pads v2` looks like a patch; it signals incompleteness rather than evolution.

**Convention**:
- Version lives in the XPN metadata `version` field only (e.g., `"version": "2.0.0"`)
- If a v2 is a substantially different artistic direction, it earns a new mood/material word: `OPAL Ghost Pads` (v1) becomes `OPAL Deep Pads` (v2)
- Hotfix re-releases use the same display name with only the metadata version bumped

---

## 4. Collection Naming

Collection membership is encoded through the vocabulary of the mood/material words — never through an explicit collection tag in the name.

**Kitchen Collection** — instrument metaphors signal membership:
- `OBESE Mortar Bass` (mortar & pestle = grinding, weight)
- `ONSET Cast Iron Drums` (cast iron = heat, impact)
- `OPAL Simmer Pads` (simmer = slow heat, patience)
- `OVERDUB Smoke Delay` (smoke = warmth, diffusion)

**Travel / Water Collection** — vessel and navigation vocabulary:
- `OBBLIGATO Sail Winds` (sail = wind propulsion, open sea)
- `OTTONI Industrial Brass` (industrial = port, machinery)
- `OUTWIT Tide Cells` (tide = rhythm, inevitability)

**Artwork / Color Collection** — pigment, material, and art-form vocabulary:
- `ORPHICA Ochre Strings` (ochre = earth pigment, ancient)
- `OHM Oxblood Drones` (oxblood = deep red, weight)

The collection name is never visible in the MPC browser. A collector who owns the full Kitchen set recognizes the culinary vocabulary; a single buyer sees only a compelling pack name. Both readings work.

---

## 5. Brand Voice for Pack Descriptions

XO_OX brand voice: experimental but accessible, aquatic and depth-aware, character over feature lists. Descriptions earn trust by naming what something *does to you*, not what it technically contains.

**ONSET Ritual Drums** (drum kit):

> Every hit arrives from below the surface. Ritual Drums pulls eight elemental voices — kick, snare, hats, clap, toms, perc, FX — through ONSET's physical modeling engine and brings them back changed. The kick holds pressure like a sealed depth charge. The snare disperses like silt caught in an upwelling current. These are not clean drums. They are drums that have been somewhere.

**OPAL Ghost Pads** (melodic pad pack):

> Granular synthesis at the thermocline — where warm and cold water refuse to mix but can't stop touching. Ghost Pads is 50 OPAL programs built around suspension: chords that won't resolve, tones that drift without leaving. Run a single note into any of these and the engine finds the undertow. Designed for tension, reverence, and the kind of silence that has weight.

**OBESE Rising Textures** (MPCe quad pack):

> Four OBESE voices loaded into a single quad — bass, sub-layer, texture, and the thing underneath the texture. Rising is about pressure accumulating over time: a low note held until the room remembers it. Each quad is built for the MPC workflow, velocity-mapped and performance-ready, with Q-Links assigned to the physical controls that matter most. Start a track. Add weight. Adjust later.

---

## 6. Cover Art Naming

**Convention**: `{ENGINE}_{PackSlug}_{version}.png`

- `ENGINE` — all-caps canonical code
- `PackSlug` — title case mood + material joined with no space, CamelCase: `RisingTextures`, `RitualDrums`, `GhostPads`
- `version` — semantic version string: `1.0.0`, `1.1.0`, `2.0.0`
- Extension always `.png` (lossless, alpha support for compositing)

**Examples**:
- `OBESE_RisingTextures_1.0.0.png`
- `ONSET_RitualDrums_1.0.0.png`
- `OPAL_GhostPads_2.0.0.png`
- `OVERLAP_SlowKnots_1.0.0.png`

**Storage location**: `Tools/cover_art/{ENGINE}/` — one folder per engine, all versions retained. Do not overwrite — bump the version slug and keep the prior file for rollback reference.

---

## Summary Reference Card

| Field | Rule | Max Length |
|---|---|---|
| Pack display name | `ENGINE Modifier Noun` | 28 chars |
| Program name | Title Case, no engine prefix | 24 chars |
| Version | Metadata field only — not in display name | — |
| Collection signal | Vocabulary encoding (implicit) | — |
| Cover art filename | `ENGINE_PackSlug_version.png` | — |
