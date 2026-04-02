#!/usr/bin/env python3
"""fix_preset_macros.py — Remove dead macro_* keys from .xometa preset parameter blocks.

Context: Presets stored macro_character, macro_movement, macro_coupling, and macro_space
inside per-engine parameter blocks (under the "parameters" key). These are not valid
APVTS parameter IDs — applyPreset() silently ignores them. Keeping them is misleading
and bloats preset files unnecessarily.

This script removes those 4 keys from every engine block in every .xometa file under
Presets/XOceanus/, writing the cleaned JSON back in-place (preserving indent=2 style).

Usage:
    python3 Tools/fix_preset_macros.py [--dry-run]

Options:
    --dry-run   Print affected files and key counts without writing any changes.
"""

import json
import os
import sys
from pathlib import Path

DEAD_KEYS = {"macro_character", "macro_movement", "macro_coupling", "macro_space"}

def process_file(path: Path, dry_run: bool) -> int:
    """Remove dead macro keys from a single .xometa file.

    Returns the number of keys removed (0 if the file was untouched).
    """
    try:
        with open(path, "r", encoding="utf-8") as f:
            data = json.load(f)
    except (json.JSONDecodeError, IOError) as exc:
        print(f"  SKIP (parse error): {path.name} — {exc}")
        return 0

    params = data.get("parameters")
    if not isinstance(params, dict):
        return 0

    removed = 0
    for engine_name, engine_block in params.items():
        if not isinstance(engine_block, dict):
            continue
        for key in DEAD_KEYS:
            if key in engine_block:
                del engine_block[key]
                removed += 1

    if removed == 0:
        return 0

    if not dry_run:
        with open(path, "w", encoding="utf-8") as f:
            json.dump(data, f, indent=2, ensure_ascii=False)
            f.write("\n")  # trailing newline for clean git diffs

    return removed


def main():
    dry_run = "--dry-run" in sys.argv

    # Locate the Presets/XOceanus directory relative to this script.
    script_dir = Path(__file__).resolve().parent
    repo_root = script_dir.parent
    presets_root = repo_root / "Presets" / "XOceanus"

    if not presets_root.is_dir():
        print(f"ERROR: Presets directory not found: {presets_root}")
        sys.exit(1)

    if dry_run:
        print("DRY RUN — no files will be written.\n")

    xometa_files = list(presets_root.rglob("*.xometa"))
    print(f"Scanning {len(xometa_files)} .xometa files in {presets_root} ...\n")

    total_files_changed = 0
    total_keys_removed = 0

    for path in sorted(xometa_files):
        removed = process_file(path, dry_run=dry_run)
        if removed > 0:
            total_files_changed += 1
            total_keys_removed += removed
            rel = path.relative_to(repo_root)
            print(f"  {'[DRY] ' if dry_run else ''}cleaned {removed:3d} key(s): {rel}")

    print(f"\nDone. {total_files_changed} file(s) changed, {total_keys_removed} dead key(s) removed.")


if __name__ == "__main__":
    main()
