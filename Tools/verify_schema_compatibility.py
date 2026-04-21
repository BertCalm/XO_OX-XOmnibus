#!/usr/bin/env python3
"""
verify_schema_compatibility.py

Scans every .xometa in the preset library and reports:
  - schema v1 file count
  - schema v2 file count
  - files missing `category` (should equal v1 count pre-backfill)
  - files with malformed JSON (should be 0)

Run before and after any schema-changing commit to verify no regressions.

Usage:
    python3 Tools/verify_schema_compatibility.py [preset_root]

Default preset_root = "XOceanus Presets" under repo root.
Falls back to scanning the whole repo for .xometa if that dir doesn't exist.
"""

import json
import os
import sys
from pathlib import Path

VALID_CATEGORIES = {
    "keys", "pads", "leads", "bass", "drums",
    "perc", "textures", "fx", "sequence", "vocal",
}
VALID_TIMBRES = {
    "strings", "brass", "wind", "choir",
    "organ", "plucked", "metallic", "world",
}

def scan(preset_root: Path):
    v1 = 0
    v2 = 0
    missing_category = []
    malformed = []
    invalid_category = []
    invalid_timbre = []

    for path in preset_root.rglob("*.xometa"):
        try:
            with path.open("r", encoding="utf-8") as f:
                data = json.load(f)
        except (json.JSONDecodeError, UnicodeDecodeError) as e:
            malformed.append((str(path), str(e)))
            continue

        schema = data.get("schema_version", 1)
        if schema == 1:
            v1 += 1
        elif schema == 2:
            v2 += 1

        cat = data.get("category")
        if cat is None:
            missing_category.append(str(path))
        elif cat not in VALID_CATEGORIES:
            invalid_category.append((str(path), cat))

        tim = data.get("timbre")
        if tim is not None and tim not in VALID_TIMBRES:
            invalid_timbre.append((str(path), tim))

    return {
        "v1_count": v1,
        "v2_count": v2,
        "total": v1 + v2,
        "missing_category": missing_category,
        "malformed": malformed,
        "invalid_category": invalid_category,
        "invalid_timbre": invalid_timbre,
    }

def main():
    repo_root = Path(__file__).resolve().parent.parent
    preset_root_str = sys.argv[1] if len(sys.argv) > 1 else "XOceanus Presets"
    preset_root = repo_root / preset_root_str
    if not preset_root.exists():
        # Fallback: scan whole repo
        candidates = list(repo_root.rglob("*.xometa"))
        if not candidates:
            print(f"ERROR: No .xometa files found under {repo_root}")
            sys.exit(2)
        preset_root = repo_root

    results = scan(preset_root)
    print(f"Preset library scan: {preset_root}")
    print(f"  Total presets:        {results['total']}")
    print(f"  Schema v1 (legacy):   {results['v1_count']}")
    print(f"  Schema v2 (migrated): {results['v2_count']}")
    print(f"  Missing `category`:   {len(results['missing_category'])}")
    print(f"  Malformed JSON:       {len(results['malformed'])}")
    print(f"  Invalid category:     {len(results['invalid_category'])}")
    print(f"  Invalid timbre:       {len(results['invalid_timbre'])}")

    if results['malformed']:
        print("\nMalformed files (first 10):")
        for path, err in results['malformed'][:10]:
            print(f"  {path}: {err}")

    if results['invalid_category']:
        print("\nInvalid category values (first 10):")
        for path, cat in results['invalid_category'][:10]:
            print(f"  {path}: '{cat}'")

    if results['invalid_timbre']:
        print("\nInvalid timbre values (first 10):")
        for path, tim in results['invalid_timbre'][:10]:
            print(f"  {path}: '{tim}'")

    if results['malformed'] or results['invalid_category'] or results['invalid_timbre']:
        sys.exit(1)
    sys.exit(0)

if __name__ == "__main__":
    main()
