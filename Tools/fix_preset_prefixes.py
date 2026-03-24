#!/usr/bin/env python3
"""
fix_preset_prefixes.py — Batch-fix parameter prefix mismatches in .xometa presets.

Rename mappings (wrong_prefix → correct_prefix, scoped to the named engine):
  ori_       → origami_   (engine: Origami)
  orac_      → oracle_    (engine: Oracle)
  oblique_   → oblq_      (engine: Oblique)
  oddfelix_  → snap_      (engine: OddfeliX)
  ocel_      → ocelot_    (engine: Ocelot)
  onset_     → perc_      (engine: Onset)
  odyssey_   → drift_     (engine: Odyssey)
  ouroboros_ → ouro_      (engine: Ouroboros)
  oceanic_   → ocean_     (engine: Oceanic)
  orbital_   → orb_       (engine: Orbital)
"""

import json
import os
import sys
from pathlib import Path

# Map: engine_name_in_preset -> (wrong_prefix, correct_prefix)
# Engine name as it appears in the "parameters" dict key (case-sensitive match).
ENGINE_PREFIX_FIXES = {
    "Origami":   ("ori_",       "origami_"),
    "Oracle":    ("orac_",      "oracle_"),
    "Oblique":   ("oblique_",   "oblq_"),
    "OddfeliX":  ("oddfelix_",  "snap_"),
    "Ocelot":    ("ocel_",      "ocelot_"),
    "Onset":     ("onset_",     "perc_"),
    "Odyssey":   ("odyssey_",   "drift_"),
    "Ouroboros": ("ouroboros_", "ouro_"),
    "Oceanic":   ("oceanic_",   "ocean_"),
    "Orbital":   ("orbital_",   "orb_"),
}

# Also handle engine name aliases that may appear in the parameters dict
# (legacy names as noted in CLAUDE.md resolveEngineAlias)
ENGINE_ALIASES = {
    "Snap":  "OddfeliX",
    "Morph": "OddOscar",
    "Dub":   "Overdub",
    "Drift": "Odyssey",
    "Bob":   "Oblong",
    "Fat":   "Obese",
    "Bite":  "Overbite",
}


def fix_engine_params(engine_key: str, params: dict) -> tuple[dict, int]:
    """
    Fix parameter keys for a single engine block.
    Returns (fixed_params_dict, number_of_keys_renamed).
    """
    # Resolve alias to canonical engine name
    canonical = ENGINE_ALIASES.get(engine_key, engine_key)
    if canonical not in ENGINE_PREFIX_FIXES:
        return params, 0

    wrong_prefix, correct_prefix = ENGINE_PREFIX_FIXES[canonical]
    renamed = 0
    fixed = {}
    for k, v in params.items():
        if k.startswith(wrong_prefix):
            new_key = correct_prefix + k[len(wrong_prefix):]
            fixed[new_key] = v
            renamed += 1
        else:
            fixed[k] = v
    return fixed, renamed


def process_file(path: Path) -> tuple[bool, int]:
    """
    Process a single .xometa file.
    Returns (was_modified, total_keys_renamed).
    """
    try:
        text = path.read_text(encoding="utf-8")
        data = json.loads(text)
    except (json.JSONDecodeError, OSError) as e:
        print(f"  SKIP (parse error): {path} — {e}", file=sys.stderr)
        return False, 0

    parameters = data.get("parameters")
    if not isinstance(parameters, dict):
        return False, 0

    total_renamed = 0
    new_parameters = {}
    for engine_key, params in parameters.items():
        if isinstance(params, dict):
            fixed_params, renamed = fix_engine_params(engine_key, params)
            new_parameters[engine_key] = fixed_params
            total_renamed += renamed
        else:
            new_parameters[engine_key] = params

    if total_renamed == 0:
        return False, 0

    data["parameters"] = new_parameters
    try:
        new_text = json.dumps(data, indent=2, ensure_ascii=False) + "\n"
        path.write_text(new_text, encoding="utf-8")
    except OSError as e:
        print(f"  ERROR writing {path}: {e}", file=sys.stderr)
        return False, 0

    return True, total_renamed


def main():
    preset_root = Path("/Users/joshuacramblet/Documents/GitHub/XO_OX-XOlokun/Presets/XOlokun")
    if not preset_root.exists():
        print(f"ERROR: Preset root not found: {preset_root}", file=sys.stderr)
        sys.exit(1)

    xometa_files = sorted(preset_root.rglob("*.xometa"))
    print(f"Scanning {len(xometa_files)} .xometa files under {preset_root} ...\n")

    files_fixed = 0
    total_keys_renamed = 0
    per_engine_stats: dict[str, int] = {}

    for path in xometa_files:
        # Quick pre-filter: skip files that don't contain any wrong prefix
        try:
            raw = path.read_text(encoding="utf-8")
        except OSError:
            continue

        has_candidate = any(
            wrong in raw
            for wrong, _ in ENGINE_PREFIX_FIXES.values()
        )
        if not has_candidate:
            continue

        modified, keys_renamed = process_file(path)
        if modified:
            files_fixed += 1
            total_keys_renamed += keys_renamed
            print(f"  FIXED ({keys_renamed:3d} keys): {path.relative_to(preset_root)}")

            # Collect per-engine stats from the file we just read
            try:
                data = json.loads(path.read_text(encoding="utf-8"))
                for engine_key in data.get("parameters", {}).keys():
                    canonical = ENGINE_ALIASES.get(engine_key, engine_key)
                    if canonical in ENGINE_PREFIX_FIXES:
                        per_engine_stats[canonical] = per_engine_stats.get(canonical, 0) + 1
            except (json.JSONDecodeError, OSError):
                pass

    print()
    print("=" * 60)
    print(f"Summary")
    print("=" * 60)
    print(f"  Files scanned : {len(xometa_files)}")
    print(f"  Files fixed   : {files_fixed}")
    print(f"  Keys renamed  : {total_keys_renamed}")
    print()
    if per_engine_stats:
        print("  Fixes by engine:")
        for engine, count in sorted(per_engine_stats.items()):
            wrong, correct = ENGINE_PREFIX_FIXES[engine]
            print(f"    {engine:12s}  {wrong:12s} → {correct:12s}  ({count} files)")
    print()
    if files_fixed == 0:
        print("  No files required fixing.")
    else:
        print(f"  Done. {files_fixed} files rewritten.")


if __name__ == "__main__":
    main()
