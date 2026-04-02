#!/usr/bin/env python3
"""fix_duplicate_names.py — Disambiguate same-mood duplicate preset names.

Scans all .xometa files under Presets/XOceanus/{mood}/ and finds presets
that share the same name within the same mood folder. For each duplicate,
appends the first engine name in parens to make it unique:

    "Aurora Veil"  →  "Aurora Veil (Oxytocin)"

Only the top-level "name" field in the JSON is modified. All other fields
are left untouched.

Usage:
    python3 Tools/fix_duplicate_names.py [--dry-run]

Closes #183.
"""

import argparse
import json
import os
import sys
from collections import defaultdict
from pathlib import Path


def load_preset(path: Path) -> dict | None:
    """Load and return parsed JSON from a .xometa file, or None on error."""
    try:
        with open(path, "r", encoding="utf-8") as f:
            return json.load(f)
    except Exception as exc:
        print(f"  WARN: could not parse {path.name}: {exc}", file=sys.stderr)
        return None


def save_preset(path: Path, data: dict) -> None:
    """Write preset data back to disk preserving 2-space indent + trailing newline."""
    with open(path, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2, ensure_ascii=False)
        f.write("\n")


def engine_label(data: dict) -> str:
    """Return the first engine name from the preset's 'engines' list, or '' if absent."""
    engines = data.get("engines", [])
    if isinstance(engines, list) and engines:
        return str(engines[0])
    # Fallback: try to derive from parameters keys
    params = data.get("parameters", {})
    if isinstance(params, dict) and params:
        return next(iter(params.keys()), "")
    return ""


def find_and_fix_duplicates(presets_root: Path, dry_run: bool) -> int:
    """
    Walk each mood subdirectory, find duplicate names, and rename them.

    Pass 1: Disambiguate by appending "(EngineName)".
    Pass 2: If duplicates still exist after pass 1 (same name + same engine),
            append a numeric suffix "(EngineName 2)", "(EngineName 3)", etc.

    Returns the total number of files modified (or that would be modified).
    """
    total_fixed = 0

    # Mood folders are direct children of presets_root (skip _quarantine)
    mood_dirs = sorted(
        d for d in presets_root.iterdir()
        if d.is_dir() and not d.name.startswith("_")
    )

    for mood_dir in mood_dirs:
        xometa_files = sorted(mood_dir.rglob("*.xometa"))
        if not xometa_files:
            continue

        # Load all presets and track desired name changes in two passes.
        # path_to_data holds the current (possibly already-patched) data.
        path_to_data: dict[Path, dict] = {}
        for path in xometa_files:
            data = load_preset(path)
            if data is not None:
                path_to_data[path] = data

        def group_by_name(
            path_data: dict[Path, dict]
        ) -> dict[str, list[Path]]:
            groups: dict[str, list[Path]] = defaultdict(list)
            for p, d in path_data.items():
                raw = d.get("name", "")
                if isinstance(raw, str) and raw.strip():
                    groups[raw].append(p)
            return groups

        # ── Pass 1: append engine name to disambiguate ────────────────────
        groups = group_by_name(path_to_data)
        for base_name, paths in groups.items():
            if len(paths) < 2:
                continue

            print(f"\n[{mood_dir.name}] Pass-1 duplicate: '{base_name}' ({len(paths)} files)")

            for path in paths:
                data = path_to_data[path]
                eng = engine_label(data)
                new_name = f"{base_name} ({eng})" if eng else f"{base_name} ({path.stem})"

                print(f"  {'[dry-run] ' if dry_run else ''}  {path.name}")
                print(f"    '{base_name}'  →  '{new_name}'")

                # Update the in-memory copy so pass 2 sees the updated names
                data["name"] = new_name
                total_fixed += 1

        # ── Pass 2: handle remaining duplicates with numeric suffix ───────
        groups2 = group_by_name(path_to_data)
        for base_name, paths in groups2.items():
            if len(paths) < 2:
                continue

            print(f"\n[{mood_dir.name}] Pass-2 duplicate: '{base_name}' ({len(paths)} files)")

            # Keep the first occurrence as-is; suffix the rest 2, 3, …
            for idx, path in enumerate(paths[1:], start=2):
                data = path_to_data[path]
                new_name = f"{base_name} {idx}"

                print(f"  {'[dry-run] ' if dry_run else ''}  {path.name}")
                print(f"    '{base_name}'  →  '{new_name}'")

                data["name"] = new_name
                total_fixed += 1

        # ── Write all modified presets ────────────────────────────────────
        if not dry_run:
            for path, data in path_to_data.items():
                save_preset(path, data)

    return total_fixed


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--dry-run", action="store_true",
                        help="Print what would change without writing any files")
    args = parser.parse_args()

    # Resolve the Presets/XOceanus directory relative to this script's repo root.
    script_dir = Path(__file__).resolve().parent
    repo_root = script_dir.parent
    presets_root = repo_root / "Presets" / "XOceanus"

    if not presets_root.is_dir():
        print(f"ERROR: preset root not found: {presets_root}", file=sys.stderr)
        sys.exit(1)

    print(f"Scanning: {presets_root}")
    if args.dry_run:
        print("(DRY RUN — no files will be written)\n")

    count = find_and_fix_duplicates(presets_root, dry_run=args.dry_run)

    print(f"\n{'Would fix' if args.dry_run else 'Fixed'} {count} preset name(s).")


if __name__ == "__main__":
    main()
