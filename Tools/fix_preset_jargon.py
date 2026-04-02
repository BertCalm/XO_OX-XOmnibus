#!/usr/bin/env python3
"""
fix_preset_jargon.py — closes #240
Replace DSP jargon in preset "name" fields with evocative equivalents.

Substitutions (applied in order, case-sensitive):
  LFO  -> Pulse
  FM   -> Spectral
  OSC  -> Wave
   LP  ->  Filter
   HP  ->  Bright Filter

Only the "name" field is modified; parameters, tags, and filenames
are left unchanged to avoid breaking preset loading.
"""

import argparse
import json
import os
import sys

SUBSTITUTIONS = [
    ("LFO", "Pulse"),
    ("FM",  "Spectral"),
    ("OSC", "Wave"),
    (" LP", " Filter"),
    (" HP", " Bright Filter"),
]


def apply_substitutions(name: str) -> str:
    for old, new in SUBSTITUTIONS:
        name = name.replace(old, new)
    return name


def process_preset(fpath: str, dry_run: bool) -> bool:
    """Return True if file was (or would be) changed."""
    try:
        with open(fpath, encoding="utf-8") as f:
            data = json.load(f)
    except Exception as exc:
        print(f"  SKIP  {fpath}: {exc}", file=sys.stderr)
        return False

    original = data.get("name", "")
    updated = apply_substitutions(original)

    if updated == original:
        return False

    print(f"  {'DRY ' if dry_run else ''}RENAME  {original!r}  ->  {updated!r}")
    print(f"          {fpath}")

    if not dry_run:
        data["name"] = updated
        with open(fpath, "w", encoding="utf-8") as f:
            json.dump(data, f, indent=2, ensure_ascii=False)
            f.write("\n")

    return True


def main() -> None:
    parser = argparse.ArgumentParser(description="Replace DSP jargon in preset names.")
    parser.add_argument("root", nargs="?", default="Presets/XOceanus",
                        help="Root directory to scan (default: Presets/XOceanus)")
    parser.add_argument("--dry-run", action="store_true",
                        help="Print changes without writing files")
    args = parser.parse_args()

    changed = 0
    scanned = 0
    for dirpath, _dirs, files in os.walk(args.root):
        for fname in files:
            if not fname.endswith(".xometa"):
                continue
            scanned += 1
            fpath = os.path.join(dirpath, fname)
            if process_preset(fpath, dry_run=args.dry_run):
                changed += 1

    action = "Would rename" if args.dry_run else "Renamed"
    print(f"\n{action} {changed}/{scanned} presets.")


if __name__ == "__main__":
    main()
