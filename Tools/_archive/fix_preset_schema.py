#!/usr/bin/env python3
"""
XOlokun Preset Schema Fixer — Fleet-Wide .xometa Repair

Fixes:
  1. Missing schema_version (adds 1)
  2. Missing/empty author (sets "XO_OX Designs")
  3. Missing macroLabels (adds ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"])
  4. Engine name normalization (legacy aliases, ALLCAPS, X-prefix variants)

Usage:
    python3 fix_preset_schema.py [--dry-run]

Options:
    --dry-run   Report what would be fixed without writing files
"""

import json
import sys
from pathlib import Path
from collections import Counter

PRESET_DIR = Path(__file__).resolve().parent.parent / "Presets" / "XOlokun"

DEFAULT_MACRO_LABELS = ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"]
DEFAULT_AUTHOR = "XO_OX Designs"
DEFAULT_SCHEMA_VERSION = 1

# ---------------------------------------------------------------------------
# Canonical engine names (34 registered engines)
# ---------------------------------------------------------------------------

CANONICAL_ENGINES = [
    "OddfeliX", "OddOscar", "Overdub", "Odyssey", "Oblong",
    "Obese", "Onset", "Overworld", "Opal", "Orbital",
    "Organon", "Ouroboros", "Obsidian", "Overbite", "Origami",
    "Oracle", "Obscura", "Oceanic", "Ocelot", "Optic",
    "Oblique", "Osprey", "Osteria", "Owlfish",
    "Ohm", "Orphica", "Obbligato", "Ottoni", "Ole",
    "Overlap", "Outwit", "Ombre", "Orca", "Octopus",
]

CANONICAL_SET = set(CANONICAL_ENGINES)

# ---------------------------------------------------------------------------
# Engine alias map: all known variants -> canonical name
# Sourced from apply_renames.py and PresetManager::resolveEngineAlias()
# ---------------------------------------------------------------------------

ENGINE_ALIAS_MAP = {
    # Legacy short names
    "Snap": "OddfeliX",
    "Morph": "OddOscar",
    "Dub": "Overdub",
    "Drift": "Odyssey",
    "Bob": "Oblong",
    "Fat": "Obese",
    "Bite": "Overbite",
    # X-prefix full instrument names
    "XOddCouple": "OddfeliX",
    "XOddFelix": "OddfeliX",
    "XOddOscar": "OddOscar",
    "XOverdub": "Overdub",
    "XOdyssey": "Odyssey",
    "XOblong": "Oblong",
    "XOblongBob": "Oblong",
    "XObese": "Obese",
    "XOnset": "Onset",
    "XOverworld": "Overworld",
    "XOpal": "Opal",
    "XOrbital": "Orbital",
    "XOrganon": "Organon",
    "XOuroboros": "Ouroboros",
    "XObsidian": "Obsidian",
    "XOverbite": "Overbite",
    "XOrigami": "Origami",
    "XOracle": "Oracle",
    "XObscura": "Obscura",
    "XOceanic": "Oceanic",
    "XOcelot": "Ocelot",
    "XOptic": "Optic",
    "XOblique": "Oblique",
    "XOsprey": "Osprey",
    "XOsteria": "Osteria",
    "XOwlfish": "Owlfish",
    "XOhm": "Ohm",
    "XOrphica": "Orphica",
    "XObbligato": "Obbligato",
    "XOttoni": "Ottoni",
    "XOle": "Ole",
    "XOverlap": "Overlap",
    "XOutwit": "Outwit",
    "XOmbre": "Ombre",
    "XOrca": "Orca",
    "XOctopus": "Octopus",
    # Common casing errors
    "OddFelix": "OddfeliX",
    "Oddoscar": "OddOscar",
}

# Build case-insensitive lookup: lowercase -> canonical
_CASE_MAP = {name.lower(): name for name in CANONICAL_ENGINES}
for alias, canonical in ENGINE_ALIAS_MAP.items():
    _CASE_MAP[alias.lower()] = canonical


def normalize_engine_name(name: str) -> str:
    """Resolve an engine name to its canonical form.

    Priority:
    1. Exact match in canonical set (already correct)
    2. Exact match in alias map
    3. Case-insensitive match
    4. Strip leading 'X' and retry case-insensitive
    5. Return original (unknown engine)
    """
    if name in CANONICAL_SET:
        return name
    if name in ENGINE_ALIAS_MAP:
        return ENGINE_ALIAS_MAP[name]
    lower = name.lower()
    if lower in _CASE_MAP:
        return _CASE_MAP[lower]
    if lower.startswith("x") and lower[1:] in _CASE_MAP:
        return _CASE_MAP[lower[1:]]
    return name


def fix_preset(filepath: Path, dry_run: bool = False):
    """Fix a single .xometa file. Returns dict of what was fixed."""
    fixes = {}

    try:
        text = filepath.read_text(encoding="utf-8")
        data = json.loads(text)
    except (json.JSONDecodeError, OSError) as e:
        return {"error": str(e)}

    if not isinstance(data, dict):
        return {"error": "Root is not a JSON object"}

    modified = False

    # 1. schema_version
    if "schema_version" not in data:
        data["schema_version"] = DEFAULT_SCHEMA_VERSION
        fixes["schema_version"] = "added"
        modified = True

    # 2. author
    if "author" not in data or not data.get("author") or not isinstance(data.get("author"), str):
        data["author"] = DEFAULT_AUTHOR
        fixes["author"] = "added"
        modified = True

    # 3. macroLabels
    if "macroLabels" not in data:
        data["macroLabels"] = list(DEFAULT_MACRO_LABELS)
        fixes["macroLabels"] = "added"
        modified = True

    # 4. Engine name normalization
    engine_renames = []

    # 4a. engines array
    if "engines" in data and isinstance(data["engines"], list):
        new_engines = []
        for eng in data["engines"]:
            canonical = normalize_engine_name(eng)
            if canonical != eng:
                engine_renames.append(f"{eng} -> {canonical}")
            new_engines.append(canonical)
        if new_engines != data["engines"]:
            data["engines"] = new_engines
            modified = True

    # 4b. parameters keys
    if "parameters" in data and isinstance(data["parameters"], dict):
        new_params = {}
        for key, val in data["parameters"].items():
            canonical = normalize_engine_name(key)
            if canonical != key:
                engine_renames.append(f"param:{key} -> {canonical}")
            new_params[canonical] = val
        if list(new_params.keys()) != list(data["parameters"].keys()):
            data["parameters"] = new_params
            modified = True

    # 4c. coupling references
    coupling = data.get("coupling")
    if coupling and isinstance(coupling, dict):
        _COUPLING_FIELDS = ("source", "target", "sourceEngine", "targetEngine",
                            "engine", "engineA", "engineB")
        for field in _COUPLING_FIELDS:
            if field in coupling and isinstance(coupling[field], str):
                canonical = normalize_engine_name(coupling[field])
                if canonical != coupling[field]:
                    engine_renames.append(f"coupling.{field}:{coupling[field]} -> {canonical}")
                    coupling[field] = canonical
                    modified = True
        for list_key in ("pairs", "entries", "slots"):
            items = coupling.get(list_key)
            if isinstance(items, list):
                for item in items:
                    if not isinstance(item, dict):
                        continue
                    for field in _COUPLING_FIELDS:
                        if field in item and isinstance(item[field], str):
                            canonical = normalize_engine_name(item[field])
                            if canonical != item[field]:
                                engine_renames.append(
                                    f"coupling.{list_key}.{field}:{item[field]} -> {canonical}")
                                item[field] = canonical
                                modified = True

    # 4d. legacy.sourceInstrument
    legacy = data.get("legacy")
    if legacy and isinstance(legacy, dict):
        si = legacy.get("sourceInstrument")
        if si and isinstance(si, str):
            canonical = normalize_engine_name(si)
            if canonical != si:
                engine_renames.append(f"legacy:{si} -> {canonical}")
                legacy["sourceInstrument"] = canonical
                modified = True

    # 4e. sequencer track engine refs
    seq = data.get("sequencer")
    if seq and isinstance(seq, dict):
        tracks = seq.get("tracks", [])
        if isinstance(tracks, list):
            for track in tracks:
                if isinstance(track, dict) and "engine" in track and isinstance(track["engine"], str):
                    canonical = normalize_engine_name(track["engine"])
                    if canonical != track["engine"]:
                        engine_renames.append(f"seq:{track['engine']} -> {canonical}")
                        track["engine"] = canonical
                        modified = True

    if engine_renames:
        fixes["engine_renames"] = engine_renames

    # Write back with sorted keys and indent=2
    if modified and not dry_run:
        out = json.dumps(data, indent=2, ensure_ascii=False, sort_keys=True) + "\n"
        filepath.write_text(out, encoding="utf-8")

    return fixes


def main():
    args = sys.argv[1:]
    dry_run = "--dry-run" in args

    if not PRESET_DIR.exists():
        print(f"ERROR: Preset directory not found: {PRESET_DIR}")
        return 1

    files = sorted(PRESET_DIR.rglob("*.xometa"))
    if not files:
        print("WARNING: No .xometa files found")
        return 1

    print("=" * 60)
    print("XOlokun Preset Schema Fixer")
    print("=" * 60)
    print(f"Scanning: {PRESET_DIR}")
    print(f"Found: {len(files)} preset files")
    print(f"Mode: {'DRY RUN' if dry_run else 'LIVE FIX'}")
    print()

    files_modified = 0
    files_errors = 0
    field_counts = Counter()
    engine_rename_count = 0
    engine_rename_detail = Counter()

    for filepath in files:
        rel = filepath.relative_to(PRESET_DIR)
        fixes = fix_preset(filepath, dry_run=dry_run)

        if "error" in fixes:
            files_errors += 1
            print(f"  ERROR  {rel}: {fixes['error']}")
            continue

        if fixes:
            files_modified += 1
            for key, val in fixes.items():
                if key == "engine_renames":
                    engine_rename_count += len(val)
                    for rename in val:
                        parts = rename.split(" -> ")
                        if len(parts) == 2:
                            old_name = parts[0].split(":")[-1]
                            engine_rename_detail[f"{old_name} -> {parts[1]}"] += 1
                else:
                    field_counts[key] += 1

    # Summary
    print()
    print("=" * 60)
    print("SUMMARY")
    print("=" * 60)
    print(f"  Total files scanned:  {len(files)}")
    print(f"  Files modified:       {files_modified}")
    print(f"  Files with errors:    {files_errors}")
    print()
    print("FIELDS ADDED/FIXED:")
    print(f"  schema_version added: {field_counts.get('schema_version', 0)}")
    print(f"  author added/fixed:   {field_counts.get('author', 0)}")
    print(f"  macroLabels added:    {field_counts.get('macroLabels', 0)}")
    print()
    print(f"ENGINE NAMES NORMALIZED: {engine_rename_count} total references")
    if engine_rename_detail:
        print()
        print("  Rename breakdown:")
        for rename, count in engine_rename_detail.most_common(30):
            print(f"    {rename}: {count}")
    else:
        print("  (no renames needed)")

    if dry_run:
        print()
        print("*** DRY RUN -- no files were modified. Re-run without --dry-run to apply. ***")

    print()
    print("Done.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
