#!/usr/bin/env python3
"""
quarantine_empty_presets.py — Move empty/macro-only .xometa presets to _quarantine/

A preset is "empty" if ANY engine in the parameters dict has:
  - 0 keys, OR
  - Only keys starting with `macro_` (no actual DSP parameters)

Usage:
    python3 Tools/quarantine_empty_presets.py           # dry run (default)
    python3 Tools/quarantine_empty_presets.py --apply   # move files
"""

import argparse
import json
import os
import shutil
import sys
from collections import defaultdict
from pathlib import Path


PRESETS_ROOT = Path(__file__).resolve().parent.parent / "Presets" / "XOmnibus"
QUARANTINE_ROOT = PRESETS_ROOT / "_quarantine"


def is_empty_engine(engine_params: dict) -> bool:
    """Return True if this engine's parameter dict contains no real DSP params."""
    keys = list(engine_params.keys())
    if len(keys) == 0:
        return True
    non_macro = [k for k in keys if not k.startswith("macro_")]
    return len(non_macro) == 0


def classify_preset(path: Path) -> tuple[bool, list[str]]:
    """
    Parse a .xometa file and determine if it is empty.

    Returns:
        (is_empty: bool, offending_engines: list[str])
    """
    try:
        with open(path, "r", encoding="utf-8") as fh:
            data = json.load(fh)
    except (json.JSONDecodeError, OSError) as exc:
        print(f"  WARNING: could not parse {path.name}: {exc}", file=sys.stderr)
        return False, []

    parameters = data.get("parameters", {})
    if not isinstance(parameters, dict):
        return False, []

    offending = []
    for engine_name, engine_params in parameters.items():
        if not isinstance(engine_params, dict):
            continue
        if is_empty_engine(engine_params):
            offending.append(engine_name)

    return bool(offending), offending


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Quarantine .xometa presets that have empty or macro-only engine parameters."
    )
    parser.add_argument(
        "--apply",
        action="store_true",
        default=False,
        help="Actually move files. Default is dry-run (report only).",
    )
    args = parser.parse_args()

    dry_run = not args.apply
    mode_label = "DRY RUN" if dry_run else "APPLY"

    print(f"=== quarantine_empty_presets.py [{mode_label}] ===")
    print(f"Presets root : {PRESETS_ROOT}")
    print(f"Quarantine   : {QUARANTINE_ROOT}")
    print()

    # Collect all .xometa files, skipping _quarantine itself
    all_files = [
        p
        for p in PRESETS_ROOT.rglob("*.xometa")
        if "_quarantine" not in p.parts
    ]
    all_files.sort()

    total_scanned = len(all_files)
    quarantine_list: list[tuple[Path, list[str]]] = []

    # engine → count of presets quarantined because of that engine
    engine_tally: defaultdict[str, int] = defaultdict(int)
    # mood → count quarantined
    mood_tally: defaultdict[str, int] = defaultdict(int)

    for preset_path in all_files:
        is_empty, offending_engines = classify_preset(preset_path)
        if is_empty:
            quarantine_list.append((preset_path, offending_engines))
            for eng in offending_engines:
                engine_tally[eng] += 1
            # mood = immediate parent folder name
            mood = preset_path.parent.name
            mood_tally[mood] += 1

    total_quarantined = len(quarantine_list)

    # --- Move or report ---
    moved = 0
    errors = 0

    for preset_path, offending_engines in quarantine_list:
        mood = preset_path.parent.name
        dest_dir = QUARANTINE_ROOT / mood
        dest_path = dest_dir / preset_path.name

        tag = ", ".join(offending_engines)
        print(f"  {'MOVE' if not dry_run else 'WOULD MOVE'}: {mood}/{preset_path.name}  [{tag}]")

        if not dry_run:
            try:
                dest_dir.mkdir(parents=True, exist_ok=True)
                shutil.move(str(preset_path), str(dest_path))
                moved += 1
            except OSError as exc:
                print(f"    ERROR moving {preset_path.name}: {exc}", file=sys.stderr)
                errors += 1

    # --- Summary ---
    print()
    print("=" * 60)
    print(f"SUMMARY [{mode_label}]")
    print("=" * 60)
    print(f"  Total scanned     : {total_scanned:,}")
    print(f"  Total to quarantine: {total_quarantined:,}")
    if not dry_run:
        print(f"  Successfully moved : {moved:,}")
        if errors:
            print(f"  Errors             : {errors:,}")
    print()

    if mood_tally:
        print("Breakdown by mood folder:")
        for mood in sorted(mood_tally):
            print(f"  {mood:<20} {mood_tally[mood]:>5}")
        print()

    if engine_tally:
        print("Breakdown by engine (engine with empty/macro-only params):")
        for engine in sorted(engine_tally, key=lambda e: -engine_tally[e]):
            print(f"  {engine:<30} {engine_tally[engine]:>5}")
        print()

    if dry_run and total_quarantined > 0:
        print("Run with --apply to move these files.")


if __name__ == "__main__":
    main()
