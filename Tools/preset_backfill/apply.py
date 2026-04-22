#!/usr/bin/env python3
"""
apply.py — reads the approved CSV from propose.py and applies
category/timbre/schema_version=2 updates to each .xometa file.

Usage:
    python3 Tools/preset_backfill/apply.py [--dry-run] [csv_path]

Default csv_path = Docs/fleet-audit/instrument-taxonomy-proposal.csv

--dry-run prints what WOULD change without writing.
"""

import argparse
import csv
import json
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

def detect_indent(raw: str):
    """
    Match the file's existing indentation so diffs stay minimal.
    Returns: int (spaces) or str ('\\t') for tabs.
    """
    for line in raw.splitlines():
        stripped = line.lstrip()
        if not stripped or not (line.startswith(" ") or line.startswith("\t")):
            continue
        if line.startswith("\t"):
            return "\t"
        # Count leading spaces on this first indented line
        count = len(line) - len(stripped)
        if count >= 4:
            return 4
        if count >= 2:
            return 2
        return count or 4
    return 4

def apply_row(row: dict, dry_run: bool):
    path = Path(row["path"])
    cat = (row.get("approved_category") or "").strip()
    tim = (row.get("approved_timbre") or "").strip()

    if cat and cat not in VALID_CATEGORIES:
        return ("invalid_category", str(path), cat)
    if tim and tim not in VALID_TIMBRES:
        return ("invalid_timbre", str(path), tim)

    if not path.exists():
        return ("missing_file", str(path), "")

    try:
        raw = path.read_text(encoding="utf-8")
        data = json.loads(raw)
    except (json.JSONDecodeError, UnicodeDecodeError) as e:
        return ("parse_error", str(path), str(e))

    changed = False

    # Bump schema version
    if data.get("schema_version") != 2:
        data["schema_version"] = 2
        changed = True

    # Apply category (required after migration)
    if cat:
        if data.get("category") != cat:
            data["category"] = cat
            changed = True
    # else: leave whatever existing value (reviewer declined to assign)

    # Apply timbre (optional, can clear)
    if tim:
        if data.get("timbre") != tim:
            data["timbre"] = tim
            changed = True
    else:
        # Reviewer left approved_timbre blank → remove any existing timbre
        if "timbre" in data:
            del data["timbre"]
            changed = True

    if not changed:
        return ("unchanged", str(path), "")

    if dry_run:
        return ("would_change", str(path), f"cat={cat} tim={tim or 'null'}")

    # Match the file's existing indent to keep diffs minimal
    indent = detect_indent(raw)

    with path.open("w", encoding="utf-8") as f:
        json.dump(data, f, indent=indent)
        f.write("\n")

    return ("changed", str(path), f"cat={cat} tim={tim or 'null'}")

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--dry-run", action="store_true")
    parser.add_argument("csv_path", nargs="?")
    args = parser.parse_args()

    repo_root = Path(__file__).resolve().parent.parent.parent
    csv_path = Path(args.csv_path) if args.csv_path else (
        repo_root / "Docs" / "fleet-audit" / "instrument-taxonomy-proposal.csv"
    )
    if not csv_path.exists():
        print(f"ERROR: CSV not found: {csv_path}")
        sys.exit(2)

    stats = {"changed": 0, "unchanged": 0, "would_change": 0,
             "invalid_category": 0, "invalid_timbre": 0,
             "missing_file": 0, "parse_error": 0}
    errors = []

    with csv_path.open("r", encoding="utf-8") as f:
        reader = csv.DictReader(f)
        for row in reader:
            result, path, detail = apply_row(row, args.dry_run)
            stats[result] = stats.get(result, 0) + 1
            if result in ("invalid_category", "invalid_timbre",
                          "missing_file", "parse_error"):
                errors.append((result, path, detail))

    mode = "DRY RUN" if args.dry_run else "APPLIED"
    print(f"\n{mode} — backfill from {csv_path}")
    for k, v in stats.items():
        if v:
            print(f"  {k:20s} {v}")

    if errors:
        print(f"\nErrors ({len(errors)}):")
        for result, path, detail in errors[:20]:
            print(f"  [{result}] {path} — {detail}")
        sys.exit(1)

    sys.exit(0)

if __name__ == "__main__":
    main()
