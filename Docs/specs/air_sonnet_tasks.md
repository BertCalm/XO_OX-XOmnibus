# AIR Integration: Sonnet-Ready Tasks

*Date: 2026-03-16 | Scope: Approach B (sidecar) only — zero external dependencies*

---

## Current State

Before doing any work, note what already exists:

- **Schema specified** — `Docs/specs/xpn_air_plugin_architecture_rnd.md` §4 defines the
  per-program sidecar schema (engine, preset_name, params dict, qlinks dict).
- **Standalone tool built** — `Tools/xpn_params_sidecar_spec.py` implements `generate` and
  `validate` CLI commands, Jaccard name-matching, and a pack-level sidecar schema
  (`version / pack_name / xoceanus_version_min / mappings[]`).
- **NOT integrated** — `xpn_bundle_builder.py` has zero sidecar references. The sidecar tool
  exists but is never called during a pack build.
- **NOT shipped in any pack** — no existing XPN ZIP output contains `params_sidecar.json`.

The tasks below close those gaps and add the XOceanus-side importer.

---

## Task 1: Reconcile the Two Sidecar Schemas

**Status: Sonnet-ready | Complexity: Small**

Two schemas currently coexist and must be unified before anything else:

### Schema A — Architecture R&D spec (`xpn_air_plugin_architecture_rnd.md` §4)

Per-program sidecar placed inside each `Programs/<ProgramName>/` directory:

```json
{
  "xo_ox_version": "1.0",
  "target_plugin": "XO_OX.XOceanus",
  "engine": "OVERBITE",
  "preset_name": "Concrete Jaw",
  "params": {
    "filter_cutoff": 0.62,
    "env_attack": 0.08
  },
  "qlinks": {
    "Q1": "filter_cutoff",
    "Q2": "drive"
  }
}
```

### Schema B — Standalone tool (`xpn_params_sidecar_spec.py`)

Pack-level sidecar placed at the pack root, with a `mappings[]` array:

```json
{
  "version": "1.0",
  "pack_name": "ONSET Drum Essentials",
  "xoceanus_version_min": "1.0.0",
  "mappings": [
    {
      "program_file": "kick_hard.xpm",
      "preset_file": "Kick Hard.xometa",
      "engine": "ONSET",
      "confidence": 0.95,
      "match_method": "exact|fuzzy|manual"
    }
  ]
}
```

### Decision required (choose one approach)

**Option 1 — Keep pack-level sidecar (Schema B, keep existing tool as-is)**
- One file at pack root: `params_sidecar.json`
- Mappings array links XPM filename → .xometa filename → engine
- XOceanus importer reads this one file to load all engine presets for the pack
- Pro: one file per pack, simpler ZIP layout, existing tool already implements it
- Con: requires XOceanus to cross-reference which program the user just loaded

**Option 2 — Per-program sidecar (Schema A, rewrite tool)**
- One `params_sidecar.json` per `Programs/<ProgramName>/` directory
- XOceanus can read it when the user opens a specific program
- Pro: decoupled — each program carries its own engine preset inline
- Con: many small files, params duplication if multiple programs share a preset

**Recommendation**: Use Option 1 (pack-level) for the initial implementation. It matches the
existing tool, is simpler to validate, and XOceanus can read it on pack import. Option 2 can
be added in V1.1 as a per-program companion alongside the pack-level file.

### Files to modify

- `Tools/xpn_params_sidecar_spec.py` — no changes needed if Option 1 chosen
- `Docs/specs/xpn_air_plugin_architecture_rnd.md` — update §4 to note the pack-level schema
  is canonical for V1; per-program variant is V1.1

### Sonnet-ready?

Yes. Schema decision is documented above; no novel architecture needed.

---

## Task 2: Integrate Sidecar Emission into xpn_bundle_builder.py

**Status: Sonnet-ready | Complexity: Medium**

`xpn_bundle_builder.py` builds the XPN ZIP but never calls `xpn_params_sidecar_spec.py`.
Add sidecar generation as a post-build step inside the `build` and `category` modes.

### What to add

**File: `Tools/xpn_bundle_builder.py`**

1. Add import at top (guarded like the other optional imports):
   ```python
   try:
       from xpn_params_sidecar_spec import generate_sidecar
       SIDECAR_AVAILABLE = True
   except ImportError:
       SIDECAR_AVAILABLE = False
   ```

2. Add `--sidecar` flag to the `build` and `category` subparsers:
   ```
   --sidecar          Emit params_sidecar.json into the pack output dir (default: on if SIDECAR_AVAILABLE)
   --no-sidecar       Suppress sidecar emission
   ```

3. After the pack output directory is written and before the ZIP is sealed, call:
   ```python
   if SIDECAR_AVAILABLE and args.sidecar:
       sidecar_path = output_dir / "params_sidecar.json"
       generate_sidecar(
           pack_dir=output_dir / "Programs",
           preset_dir=PRESETS_DIR,
           output_path=sidecar_path,
           pack_name=pack_name,
       )
   ```

4. When the ZIP is assembled, include `params_sidecar.json` at the archive root (same level as
   `expansion.json`):
   ```python
   zip_file.write(sidecar_path, "params_sidecar.json")
   ```

### Where in the file

Search for the function that seals the ZIP (likely `build_pack()` or similar). The sidecar
call goes immediately before `zipfile.ZipFile` is opened for writing, so the file exists on
disk before being added to the archive.

### Edge cases to handle

- If `Programs/` directory is empty or contains no `.xpm` files, skip sidecar and log a
  warning — do not fail the build.
- If zero mappings are found (no `.xometa` files match the pack's programs), write the sidecar
  anyway with an empty `mappings: []` — the file presence is what signals XOceanus to look.
- Log the sidecar mapping count at the end of the build summary alongside existing stats.

### Files to modify

- `Tools/xpn_bundle_builder.py`

### Sonnet-ready?

Yes. Pattern follows the existing guarded-import pattern already used for `xpn_cover_art`,
`xpn_drum_export`, and `xpn_packager`. No new architecture.

---

## Task 3: XPN Pack Quality — AIR-Compatible Preset Pairs

**Status: Sonnet-ready (preset authoring) | Complexity: Large**

Design XPN pack programs that are explicitly intended to sit alongside AIR plugins. The
sidecar carries the XOceanus engine character; the pack liner notes document the pairing.

### Pairing targets

These are the three highest-value complementary pairs based on sonic role:

#### Pair A — ONSET Drum Essentials + AIR Bassline

AIR Bassline is an acid bass synthesizer (Roland TB-303 style). It provides melodic bass lines.
ONSET provides drums. They are rhythmically complementary — drums under acid bassline is a
foundational electronic music texture.

Concrete deliverable: For each of the 20 kits in the ONSET Drum Essentials pack
(`Docs/specs/pack_design_onset_drums.md`), add a `"air_complement"` annotation to the pack
liner notes doc naming the AIR Bassline preset character that pairs well. Example:

```
Kit: Techno Engine 909
AIR complement: AIR Bassline — acid pattern, Cm, 130 BPM
Sidecar engine: ONSET
Sidecar preset: Techno Engine 909.xometa
```

No code changes — this is copywriting for the pack README / liner notes file.

#### Pair B — OPAL Pad Presets + AIR Solina

AIR Solina is a string machine (ARP Solina style). OPAL is granular — it produces evolving
textural pads. Both occupy the same frequency region (lush, sustained, mid-high). They pair
as layers rather than complements: Solina for the string attack edge, OPAL for the granular
shimmer underneath.

Concrete deliverable: Identify 8–12 existing OPAL presets from `Presets/XOceanus/` that have
DNA brightness ≥ 0.5 and movement ≥ 0.4 (the Solina-adjacent zone). Document them in a
`Docs/specs/air_opal_solina_pairs.md` file with: preset name, DNA values, recommended AIR
Solina preset character, suggested mix role.

This requires reading `.xometa` files — Sonnet can do this with a short Python scan script or
manual inspection.

#### Pair C — OVERWORLD Era Presets + AIR WaveTable

AIR WaveTable is a wavetable synthesizer. OVERWORLD is also a synthesis engine with an ERA
triangle (Buchla/Schulze/Vangelis/Pearlman). Both can cover lead and evolving pad territory.

Concrete deliverable: Document 6 OVERWORLD presets where the ERA setting pushes it into
complementary (not competing) territory with a wavetable lead. Specifically: OVERWORLD presets
with high Buchla axis values (organic/acoustic-leaning) complement a WaveTable lead (digital/
spectral). Document in liner notes format.

### Files to create / modify

- `Docs/specs/air_opal_solina_pairs.md` — new doc (Pairs B findings)
- `Docs/specs/pack_design_onset_drums.md` — add AIR Bassline complement column to kit table
- `Docs/specs/pack_design_overworld_air.md` — new doc (Pair C findings, if worth a full doc)

### Sonnet-ready?

Pairs A and C: Yes — copywriting and preset curation, no code.
Pair B: Yes — requires scanning `.xometa` DNA fields, which is straightforward Python or
manual review.

---

## Task 4: XOceanus Sidecar Importer (C++ Feature)

**Status: Sonnet-ready | Complexity: Medium | Not pure documentation — touches JUCE code**

This is the XOceanus-side feature that makes the sidecar actionable for end users. Without it,
the sidecar ships in packs but nothing loads it.

### What to build

A `SidecarImporter` utility class in `Source/Export/SidecarImporter.h` (inline header,
following XOceanus conventions — DSP in `.h`, `.cpp` is a one-line stub).

**Responsibilities:**

1. `scanDirectory(File dir)` — find `params_sidecar.json` in a user-selected directory
2. `parseSidecar(File sidecarFile)` — read JSON, return list of `SidecarMapping` structs
3. `loadMapping(SidecarMapping m, PresetManager& pm)` — call `pm.loadPreset()` for the
   matching engine + preset name

**SidecarMapping struct:**

```cpp
struct SidecarMapping {
    juce::String programFile;   // XPM filename (for display only)
    juce::String presetFile;    // .xometa filename
    juce::String engine;        // Engine short name e.g. "ONSET"
    float confidence;           // Match confidence 0.0–1.0
    juce::String matchMethod;   // "exact" | "fuzzy" | "manual"
};
```

**UI integration point:** Add a "Load XO_OX Pack Presets" button to the XOceanus preset
browser or pack import panel. On click: open a directory picker, call `SidecarImporter::scan`,
show a list of found mappings with confidence scores, let user confirm, then call
`loadMapping()` for each confirmed entry.

**JSON parsing:** Use `juce::JSON::parse()` — already available in JUCE, no new dependency.

### Files to create / modify

- `Source/Export/SidecarImporter.h` — new file (implement full class inline)
- `Source/Export/SidecarImporter.cpp` — new file (one-line `#include` stub)
- `Source/UI/` — add button and directory picker to whichever panel owns preset loading
  (read `Source/UI/` to find the right component before writing)
- `CMakeLists.txt` — add `SidecarImporter.cpp` to the source list

### Sonnet-ready?

Yes. Pattern follows existing `Source/Export/` files. JSON parsing via `juce::JSON` is
standard JUCE pattern. No novel DSP. The UI addition is a button + file chooser — a
well-documented JUCE pattern.

**Note:** Confirm the correct UI component file by reading `Source/UI/` before writing — do
not guess the file name.

---

## Task 5: Documentation Updates

**Status: Sonnet-ready | Complexity: Small**

### 5a. Update xpn_tool_suite_complete_reference.md

File: `Docs/specs/xpn_tool_suite_complete_reference.md`

Add an entry for `xpn_params_sidecar_spec.py` to the tool table. It currently exists in
`Tools/` but may not be listed in the master reference doc. Check the file first.

Entry format:
```
| xpn_params_sidecar_spec.py | Generate + validate params_sidecar.json for any pack |
| Usage: python xpn_params_sidecar_spec.py generate --pack <dir> --presets <dir> --output sidecar.json |
```

### 5b. Add sidecar section to xpn-tools.md skill

File: `~/.claude/skills/xpn-export-specialist/` (or wherever xpn-tools.md lives — check
`~/.claude/skills/`)

Add a "Sidecar Emission" step to the XPN pack build workflow, between "Generate XPM files"
and "Package ZIP". One paragraph describing what the sidecar is, when to skip it (hardware-
only packs), and the CLI invocation.

### 5c. Update air_plugin_research.md recommended path

File: `Docs/specs/air_plugin_research.md`

The "Recommended Path" section says "Now: Ship params_sidecar.json in all new XPN packs" but
the mechanism to do this (bundle_builder integration) did not yet exist when the file was
written. Update the "Now" item to reference that Task 2 above closes this gap, and note that
`xpn_params_sidecar_spec.py` is the standalone tool for manual sidecar generation.

### Files to modify

- `Docs/specs/xpn_tool_suite_complete_reference.md`
- `Docs/specs/air_plugin_research.md`
- `~/.claude/skills/xpn-export-specialist/` skill file (locate exact path before editing)

---

## Execution Order

| Order | Task | Blocker |
|-------|------|---------|
| 1 | Task 1 — Schema reconciliation | None. Do this first so Tasks 2 and 4 build on the right schema. |
| 2 | Task 2 — bundle_builder integration | Task 1 must be decided. |
| 3 | Task 3 — Preset pair documentation | Independent. Can run in parallel with Task 2. |
| 4 | Task 4 — XOceanus SidecarImporter | Independent of Tasks 2–3. Needs JUCE context. |
| 5 | Task 5 — Docs | Run last, after Tasks 1–4 are done and confirmed. |

---

## Opus-Level Work (out of scope here)

These items are referenced for completeness but require Opus sessions or external cooperation:

- Full `plugin_bundle/` ZIP architecture — requires MPC Software firmware support from Akai.
- Hardware MPC native plugin port — requires Akai OEM arrangement.
- Per-program sidecar (Schema A / Option 2) — straightforward but involves rewriting the
  existing tool; defer to V1.1 after the pack-level sidecar ships.
- Apple notarization — process work, not code. Requires Apple Developer account activation.
