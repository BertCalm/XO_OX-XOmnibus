#!/usr/bin/env python3
"""
migrate_onset_macro_labels.py
------------------------------
Fix ONSET macro label vocabulary across all presets.

Wrong:   ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"]
Correct: ["MACHINE",   "PUNCH",    "SPACE",    "MUTATE"]

Position mapping (by index):
  [0] CHARACTER -> MACHINE
  [1] MOVEMENT  -> PUNCH
  [2] COUPLING  -> SPACE
  [3] SPACE     -> MUTATE

The macros dict keys are renamed to match.

Only touches presets where:
  - "Onset" appears in the engines array
  - macroLabels contains "CHARACTER" or "MOVEMENT"

Run:
    python3 Tools/migrate_onset_macro_labels.py
    python3 Tools/migrate_onset_macro_labels.py --dry-run
"""

import argparse
import json
import os
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
PRESETS_ROOT = REPO_ROOT / "Presets"

# Position-based label mapping for wrong -> correct
LABEL_REMAP = {
    "CHARACTER": "MACHINE",
    "MOVEMENT": "PUNCH",
    "COUPLING": "SPACE",
    "SPACE": "MUTATE",
}

# The only labels that are "wrong" and should trigger a fix
WRONG_LABELS = {"CHARACTER", "MOVEMENT"}

CANONICAL_LABELS = ["MACHINE", "PUNCH", "SPACE", "MUTATE"]


def remap_label(label: str) -> str:
    """Remap a single label using position-based mapping, fallback to identity."""
    return LABEL_REMAP.get(label, label)


def fix_preset(path: Path, dry_run: bool) -> bool:
    """
    Load a preset, apply label migration if needed, write back.
    Returns True if the preset was (or would be) modified.
    """
    try:
        with open(path, "r", encoding="utf-8") as f:
            data = json.load(f)
    except (json.JSONDecodeError, OSError) as e:
        print(f"  ERROR reading {path}: {e}", file=sys.stderr)
        return False

    engines = data.get("engines", [])
    labels = data.get("macroLabels", [])

    # Only fix presets that have Onset as an engine
    if "Onset" not in engines:
        return False

    # Only fix if wrong labels are present
    if not any(lbl in WRONG_LABELS for lbl in labels):
        return False

    # Build the new macroLabels by position
    new_labels = [remap_label(lbl) for lbl in labels]

    # Build the label rename map for this specific preset (old_key -> new_key)
    label_rename = {old: new for old, new in zip(labels, new_labels) if old != new}

    # Rename macros dict keys
    macros = data.get("macros")
    new_macros = None
    if isinstance(macros, dict):
        new_macros = {}
        for k, v in macros.items():
            new_key = label_rename.get(k, k)
            new_macros[new_key] = v

    # Check if anything actually changed
    if new_labels == labels and new_macros == macros:
        return False

    if dry_run:
        print(f"  [DRY-RUN] {path.relative_to(REPO_ROOT)}")
        print(f"    labels:  {labels} -> {new_labels}")
        if label_rename:
            print(f"    macros keys renamed: {label_rename}")
        return True

    # Apply changes
    data["macroLabels"] = new_labels
    if new_macros is not None:
        data["macros"] = new_macros

    try:
        with open(path, "w", encoding="utf-8") as f:
            json.dump(data, f, indent=2, ensure_ascii=False)
            f.write("\n")
    except OSError as e:
        print(f"  ERROR writing {path}: {e}", file=sys.stderr)
        return False

    return True


def main():
    parser = argparse.ArgumentParser(
        description="Migrate ONSET macro labels from CHARACTER/MOVEMENT to MACHINE/PUNCH."
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print what would change without writing files.",
    )
    parser.add_argument(
        "--presets-root",
        type=Path,
        default=PRESETS_ROOT,
        help=f"Root directory to scan for .xometa files (default: {PRESETS_ROOT})",
    )
    args = parser.parse_args()

    presets_root = args.presets_root
    if not presets_root.exists():
        print(f"ERROR: Presets root not found: {presets_root}", file=sys.stderr)
        sys.exit(1)

    all_presets = sorted(presets_root.rglob("*.xometa"))
    print(f"Scanning {len(all_presets)} preset files under {presets_root}")
    if args.dry_run:
        print("DRY-RUN mode — no files will be written.\n")

    fixed = 0
    skipped = 0
    errors = 0

    for path in all_presets:
        try:
            result = fix_preset(path, dry_run=args.dry_run)
            if result:
                if not args.dry_run:
                    print(f"  FIXED: {path.relative_to(REPO_ROOT)}")
                fixed += 1
            else:
                skipped += 1
        except Exception as e:
            print(f"  UNEXPECTED ERROR {path}: {e}", file=sys.stderr)
            errors += 1

    print(f"\nDone.")
    print(f"  {'Would fix' if args.dry_run else 'Fixed'}: {fixed}")
    print(f"  Skipped (no change needed): {skipped}")
    if errors:
        print(f"  Errors: {errors}")

    return 0 if errors == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
