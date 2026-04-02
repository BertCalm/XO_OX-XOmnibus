# params_sidecar.json — Canonical Schema
*Status: V1 Canonical | Date: 2026-03-16*

---

## 1. Schema Decision

The **pack-level sidecar** (Schema B from `air_sonnet_tasks.md`) is the canonical format for V1. One `params_sidecar.json` file lives at the pack root alongside `expansion.json`, and contains a `mappings[]` array linking each XPM program filename to its matching `.xometa` preset filename and engine name. This format was chosen over the per-program variant (Schema A, from `xpn_air_plugin_architecture_rnd.md §4`) because the existing implementation in `Tools/xpn_params_sidecar_spec.py` already produces and validates it, it is simpler to generate and inspect (one file per pack rather than N files), and XOceanus can read the whole pack's mapping table in a single file read during pack import. The per-program variant (one `params_sidecar.json` per `Programs/<Name>/` directory, with inline `params` and `qlinks` dictionaries) is preserved as the V1.1 design target: it provides per-program portability for packs that are partially extracted, and carries explicit parameter values rather than a preset reference. V1.1 may ship both files; for V1 the pack-level file is the only required artifact.

---

## 2. Full JSON Schema

```json
{
  "version": "<string>",
  "pack_name": "<string>",
  "xoceanus_version_min": "<string>",
  "mappings": [
    {
      "program_file": "<string>",
      "preset_file": "<string>",
      "engine": "<string>",
      "confidence": "<number>",
      "match_method": "<string>"
    }
  ]
}
```

### Field Definitions

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `version` | string | Yes | Sidecar schema version. Must be `"1.0"` for V1. |
| `pack_name` | string | Yes | Human-readable pack name, e.g. `"ONSET Drum Essentials"`. Used for display in XOceanus import UI. |
| `xoceanus_version_min` | string | Yes | Minimum XOceanus version required to load all engines referenced in `mappings`. Semver format: `"major.minor.patch"`. |
| `mappings` | array | Yes | Ordered list of program-to-preset mappings. May be empty (`[]`) if no matches were found; the file presence alone signals XOceanus to look. |
| `mappings[].program_file` | string | Yes | Bare filename of the XPM program, e.g. `"Kick Hard.xpm"`. No path component — MPC program files are always in `Programs/<Name>/`. |
| `mappings[].preset_file` | string | Yes | Bare filename of the `.xometa` preset, e.g. `"Kick Hard.xometa"`. XOceanus resolves this against its preset library root. |
| `mappings[].engine` | string | Yes | Short engine name as registered in XOceanus, uppercase. E.g. `"ONSET"`, `"OPAL"`, `"OVERWORLD"`. Must match a registered engine ID. |
| `mappings[].confidence` | number | Yes | Match confidence in range `[0.0, 1.0]`. `1.0` = exact name match; values from `0.4` to `0.99` are fuzzy matches; `manual` overrides always carry `1.0`. |
| `mappings[].match_method` | string | Yes | One of `"exact"`, `"fuzzy"`, or `"manual"`. `"exact"` = normalised word sets are identical; `"fuzzy"` = Jaccard similarity above threshold; `"manual"` = human-authored mapping that overrides automatic scoring. |

---

## 3. Example — ONSET Drum Pack

```json
{
  "version": "1.0",
  "pack_name": "ONSET Drum Essentials",
  "xoceanus_version_min": "1.0.0",
  "mappings": [
    {
      "program_file": "Techno Engine 909.xpm",
      "preset_file": "Techno Engine 909.xometa",
      "engine": "ONSET",
      "confidence": 1.0,
      "match_method": "exact"
    },
    {
      "program_file": "Broken Concrete Kit.xpm",
      "preset_file": "Broken Concrete.xometa",
      "engine": "ONSET",
      "confidence": 0.67,
      "match_method": "fuzzy"
    },
    {
      "program_file": "Midnight Club Breaks.xpm",
      "preset_file": "Midnight Breaks.xometa",
      "engine": "ONSET",
      "confidence": 0.58,
      "match_method": "fuzzy"
    },
    {
      "program_file": "808 Pressure Kit.xpm",
      "preset_file": "808 Pressure.xometa",
      "engine": "ONSET",
      "confidence": 1.0,
      "match_method": "manual"
    }
  ]
}
```

In this pack every mapping references the ONSET engine. Confidence `1.0` + `"exact"` means the XPM and `.xometa` filenames normalise to identical word sets. Confidence `1.0` + `"manual"` means a sound designer authored the pairing by hand (overrides any automatic score).

---

## 4. Example — OPAL Pad Pack

```json
{
  "version": "1.0",
  "pack_name": "OPAL Granular Atmospheres",
  "xoceanus_version_min": "1.0.0",
  "mappings": [
    {
      "program_file": "Spectral Tide.xpm",
      "preset_file": "Spectral Tide.xometa",
      "engine": "OPAL",
      "confidence": 1.0,
      "match_method": "exact"
    },
    {
      "program_file": "Frozen Memory Pad.xpm",
      "preset_file": "Frozen Memory.xometa",
      "engine": "OPAL",
      "confidence": 0.71,
      "match_method": "fuzzy"
    },
    {
      "program_file": "Deep Glass Shimmer.xpm",
      "preset_file": "Glass Shimmer.xometa",
      "engine": "OPAL",
      "confidence": 0.63,
      "match_method": "fuzzy"
    },
    {
      "program_file": "Aether Bloom.xpm",
      "preset_file": "Aether Bloom.xometa",
      "engine": "OPAL",
      "confidence": 1.0,
      "match_method": "exact"
    }
  ]
}
```

OPAL presets reference the granular engine. A pack may mix multiple engines in one sidecar — for example a pack pairing OPAL atmospheric pads with OVERWORLD era-chip leads would list both `"engine": "OPAL"` and `"engine": "OVERWORLD"` entries in the same `mappings` array.

---

## 5. Validation Rules

### Top-level

| Rule | Constraint |
|------|-----------|
| `version` | Must equal `"1.0"` (string, not number). |
| `pack_name` | Non-empty string. Max recommended length: 60 characters. |
| `xoceanus_version_min` | Semver string matching `\d+\.\d+\.\d+`. Must be `"1.0.0"` or higher. |
| `mappings` | Must be a JSON array. Empty array is valid. |

### Per mapping entry

| Rule | Constraint |
|------|-----------|
| `program_file` | Non-empty string. Must end in `.xpm`. No path separators (`/` or `\`). |
| `preset_file` | Non-empty string. Must end in `.xometa`. No path separators. |
| `engine` | Uppercase string. Must match a registered XOceanus engine short name (see `CLAUDE.md` engine table). Unknown engine names are logged as warnings, not errors, to allow forward compatibility. |
| `confidence` | Float in `[0.0, 1.0]` inclusive. Values below the generation threshold (`default: 0.4`) should not appear in generated files, but are not invalid on import. |
| `match_method` | One of `"exact"`, `"fuzzy"`, `"manual"`. Any other value is a validation error. |
| Uniqueness | Each `program_file` value must appear at most once. Each `preset_file` value must appear at most once. Duplicate entries are a validation error. |

### File existence (optional, requires pack context)

When validating against a live pack and preset directory:
- `program_file` should resolve to an existing file under `Programs/`.
- `preset_file` should resolve to an existing file under the XOceanus preset library root.
- Missing files are reported as **warnings**, not errors, because packs may be partially extracted and preset libraries may differ between systems.

### Generation threshold

`xpn_params_sidecar_spec.py` defaults to `--threshold 0.4`. Mappings with Jaccard similarity below `0.4` are silently excluded from the generated sidecar. The threshold may be lowered for packs whose program names diverge significantly from preset names (e.g., stylized names like `"XBeat_Alpha_v3.xpm"` matching `"Alpha Pressure.xometa"`).

---

## 6. V1.1 Forward-Compatibility Note

The V1.1 per-program schema (`xpn_air_plugin_architecture_rnd.md §4`) embeds a `params` dictionary and a `qlinks` dictionary directly inside each program's directory. It uses `"xo_ox_version"` and `"target_plugin"` as top-level keys rather than `"version"` and `"pack_name"`. These two schemas do not conflict: the pack-level file lives at the archive root; the per-program file lives inside `Programs/<Name>/`. XOceanus importers should check for both locations and prefer the per-program file if present (it carries more specific data), falling back to the pack-level file.
