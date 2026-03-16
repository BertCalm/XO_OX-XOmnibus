# XPN Metadata Standards — R&D

**Topic:** Metadata and tagging standards for XO_OX XPN packs — what information lives where and how it flows.
**Date:** 2026-03-16
**Status:** Draft

---

## 1. expansion.json / Expansion.xml Fields

MPC OS reads expansion metadata from two files inside the `.xpn` ZIP:

### `Expansions/manifest` (plain-text key=value — widest firmware compatibility)

```
Name=XObese Character Kits
Version=1.0
Author=XO_OX Designs
Description=Hot Pink chaos — 18 keygroup programs from XObese
```

### `Expansions/Expansion.xml` (XML — MPC firmware 2.10+)

```xml
<?xml version="1.0" encoding="UTF-8"?>
<expansion version="2.0.0.0" buildVersion="2.10.0.0">
  <name>XObese Character Kits</name>
  <manufacturer>XO_OX Designs</manufacturer>
  <version>1.0.0</version>
  <type>Instrument</type>
  <description>Hot Pink chaos — 18 keygroup programs from XObese</description>
</expansion>
```

**Known field behavior:**

| Field | MPC Browser display | Character limit | Notes |
|-------|---------------------|-----------------|-------|
| `name` | Pack title in Expansions browser | ~40 chars visible | Truncated in grid view |
| `manufacturer` | Sub-label below name | ~30 chars | Maps to `Author=` in plain manifest |
| `version` | Hidden in browser, shown in pack details | none | XML form appends `.0` (e.g. `1.0.0`) |
| `type` | Hidden | — | `Instrument` or `Drum` accepted |
| `description` | Shown in pack detail view only | ~120 chars visible | Not indexed for search |

**What MPC does NOT read from expansion metadata:** tags, engine type, BPM, key, mood, collection. These fields are XO_OX internal only — they live in the XPM `<Name>` string and in the XO_OX `bundle_manifest.json` sidecar.

---

## 2. XPM Metadata Fields

An XPM file (`<Program type="Drum">` or `<Program type="Keygroup">`) carries the following metadata-relevant fields:

| XPM field | Location | MPC browser display | Notes |
|-----------|----------|---------------------|-------|
| `<Name>` | Top-level `<Program>` child | Primary file/program name | This is the searchable string in MPC browser |
| Macro `<Name>` x4 | Inside `<MacroControls>` | Shown as knob labels in program | TONE, PITCH, BITE, SPACE (engine-specific) |
| `<KeygroupNumKeygroups>` | Keygroup type only | Hidden | Structural only |
| `<ProgramPads>` | Drum type only | Hidden | JSON blob, no searchable text |

**MPC 3.x search behavior:** The browser's search field indexes the XPM `<Name>` field only. It does not index macro names, pad names, or embedded JSON. This means all searchable metadata must be packed into the program name string or inferred from the pack name.

**Confirmed XPM metadata gaps:** There is no native `<Tags>`, `<Genre>`, `<Tempo>`, `<Key>`, or `<Author>` field in the XPM schema. MPC does not support these. All such data must live in the pack manifest or XO_OX sidecar files.

---

## 3. XO_OX Metadata Schema Proposal

### Pack-level: `bundle_manifest.json` (inside `.xpn` archive)

This file is read by XO_OX tooling only — MPC ignores it. It is the canonical metadata store.

```json
{
  "schema_version": 2,
  "pack_name": "XObese Character Kits",
  "version": "1.2.0",
  "build_date": "2026-03-16",
  "oxport_version": "0.9.4",
  "engines": ["OBESE"],
  "mood": "Flux",
  "collection": "Kitchen Essentials",
  "sonic_dna": {
    "brightness": 0.7,
    "warmth": 0.4,
    "movement": 0.8,
    "density": 0.6,
    "space": 0.5,
    "aggression": 0.9
  },
  "felix_oscar_bias": 0.85,
  "bpm_suggestion": null,
  "key_suggestion": null,
  "tags": ["character", "hot-pink", "drive", "mojo", "bass", "lo-fi"],
  "program_count": 18,
  "author": "XO_OX Designs"
}
```

**Field definitions:**

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `schema_version` | int | yes | Increment when schema changes. Current: `2` |
| `pack_name` | string (≤40) | yes | Matches `expansion.xml <name>` exactly |
| `version` | semver string | yes | `MAJOR.MINOR.PATCH` — see versioning rules below |
| `build_date` | ISO 8601 date | yes | Auto-stamped by Oxport |
| `oxport_version` | string | yes | Auto-stamped by Oxport |
| `engines` | string[] | yes | Engine short names from CLAUDE.md table (e.g. `["OBESE", "ONSET"]`) |
| `mood` | enum | yes | Foundation / Atmosphere / Entangled / Prism / Flux / Aether / Family |
| `collection` | string or null | no | `"Kitchen Essentials"`, `"Travel"`, `"Artwork"`, null for standalone |
| `sonic_dna` | object | yes | 6D floats 0–1. Must match the aggregate DNA of featured presets |
| `felix_oscar_bias` | float 0–1 | yes | 0 = pure Oscar (dark/bass), 1 = pure feliX (bright/melodic) |
| `bpm_suggestion` | int or null | no | Tempo if pack is rhythmically locked; null otherwise |
| `key_suggestion` | string or null | no | Root key if tonally centered (e.g. `"Am"`); null if neutral |
| `tags` | string[] | yes | Min 4 tags from canonical vocabulary (see Section 4) |
| `program_count` | int | yes | Total XPM programs in the pack |
| `author` | string | yes | `"XO_OX Designs"` for factory |

### How these map to MPC-visible fields

```
bundle_manifest.json.pack_name  →  Expansion.xml <name>  →  MPC Expansions browser title
bundle_manifest.json.version    →  Expansion.xml <version>  →  MPC pack detail view
bundle_manifest.json.author     →  Expansion.xml <manufacturer>  →  MPC pack detail sub-label
bundle_manifest.json.tags[0:2]  →  Prepended to XPM <Name> as suffix hint (see Section 5)
```

---

## 4. Tag Taxonomy — Canonical XO_OX Vocabulary

62 tags organized into 6 families. Every pack must use at least 4; every tag used must be from this list.

### Instrument Family (12)
`keys` `bass` `lead` `pad` `arp` `bells` `strings` `brass` `woodwind` `drums` `percussion` `fx`

### Texture (12)
`clean` `warm` `dirty` `lo-fi` `granular` `noisy` `glassy` `metallic` `organic` `digital` `saturated` `detuned`

### Mood / Affect (12)
`dark` `bright` `haunting` `joyful` `tense` `meditative` `aggressive` `melancholy` `euphoric` `chaotic` `grounded` `dreamy`

### Use Case (10)
`production-ready` `sound-design` `experimental` `live-performance` `cinematic` `hip-hop` `electronic` `ambient` `dance` `world`

### Engine-Specific (10)
`character` `mojo` `coupled` `dub-pump` `cellular` `granular-cloud` `wavetable` `physical-model` `era-blend` `percussive`

### XO_OX Brand (6)
`hot-pink` `neon-green` `lavender` `electric-blue` `olive` `feliX` `oscar` `flux`

> Note: `hot-pink` through `flux` are accent-color shorthand for rapid engine identification in search. Use the engine's accent color tag plus the sonic tags.

---

## 5. Search Optimization

**MPC browser search** only indexes the XPM `<Name>` field. The pack expansion name is not searched across programs — only across the pack list.

**Recommended XPM naming pattern:**

```
{PresetName} [{engine_tag}]
```

Example: `Mojo Pressure [obese]` or `808 Reborn [onset]`

- Keep the bracket suffix 6–8 chars max — MPC grid view truncates at ~28 chars total
- Use lowercase engine short-name in brackets — it becomes a natural search token
- Avoid special characters (`/`, `&`, `#`) — MPC search can choke on them

**Known MPC 3.x search quirks:**
- Search is case-insensitive but substring-only — prefix matching works, suffix matching is unreliable on some firmware versions
- The `.xpn` expansion must be "installed" (not just present in the Expansions folder) for its programs to appear in global search
- Pack re-installs do not re-index automatically on older MPC 3.x firmware — users may need to rebuild the library index manually after version updates

**Recommended strategy:** Put the most user-recognizable keyword first in the XPM name (e.g. `808`, `Bass`, `Pad`) — these are the terms users type first. Put the engine tag in brackets at the end.

---

## 6. Version Control in Metadata

Use semantic versioning (`MAJOR.MINOR.PATCH`) with these XO_OX conventions:

| Increment | Trigger | Example |
|-----------|---------|---------|
| `PATCH` | Sample replacement, mix fix, single XPM correction | `1.0.0` → `1.0.1` |
| `MINOR` | New presets/programs added, tag updates, DNA refinement | `1.0.1` → `1.1.0` |
| `MAJOR` | Engine change, structural redesign, incompatible preset schema | `1.1.0` → `2.0.0` |

**Oxport auto-stamping:** Oxport should read `--version` from the CLI argument and write it to all three locations in one pass:

1. `bundle_manifest.json` → `"version"`
2. `Expansions/manifest` → `Version=`
3. `Expansions/Expansion.xml` → `<version>` (appending `.0` for the 4-part format MPC expects)

**Build metadata auto-fields** — Oxport stamps these without user input:

```python
"build_date": datetime.utcnow().strftime("%Y-%m-%d"),
"oxport_version": OXPORT_VERSION,   # constant in oxport.py
```

**Version source of truth:** `bundle_manifest.json` is the canonical version record. The plain manifest and XML are derived from it. When parsing a `.xpn` for version history, read `bundle_manifest.json` first; fall back to `Expansion.xml` if the sidecar is absent (pre-schema-v2 packs).

**Changelog convention:** For `MINOR` and `MAJOR` bumps, commit the pack's `bundle_manifest.json` to git with a message matching the pattern:

```
pack(XObese): v1.1.0 — added 4 programs, refined DNA warmth scores
```

This creates a version trail in git history without requiring a separate CHANGELOG file.

---

## Summary: Data Flow

```
.xometa presets
    └── Oxport pipeline
            ├── WAV renders → XPM files (<Name> = preset name + [engine] tag)
            ├── bundle_manifest.json (full XO_OX schema, not read by MPC)
            ├── Expansions/manifest (Name, Version, Author, Description)
            └── Expansions/Expansion.xml (same + <type>)
                        └── .xpn ZIP → MPC installs → browser displays pack name + version
                                                      → program search indexes XPM <Name> only
```
