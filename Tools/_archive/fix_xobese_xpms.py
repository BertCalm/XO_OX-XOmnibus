#!/usr/bin/env python3
"""
XObese XPM Fixer — Patches broken keygroup programs to match MPC conventions.

Issues found by comparing against working DX-TX v2 expansion:
  1. KeyTrack=False → should be True (samples must transpose across zones)
  2. RootNote=MIDI+1 → should be 0 (MPC auto-detect convention)
  3. Empty layers have VelStart=1 → should be 0 (prevents ghost triggering)

Usage:
    python3 fix_xobese_xpms.py [--dry-run]
"""

import os
import re
import sys
from pathlib import Path

EXPANSION_DIR = Path.home() / "Library" / "Application Support" / "Akai" / "MPC" / "Expansions" / "com.xo-ox.xobese"


def fix_xpm(filepath: Path, dry_run: bool = False) -> dict:
    """Fix a single XPM file. Returns stats."""
    text = filepath.read_text(encoding="utf-8")
    original = text
    stats = {"keytrack": 0, "rootnote": 0, "velstart": 0}

    # Fix 1: KeyTrack False → True
    count = text.count("<KeyTrack>False</KeyTrack>")
    text = text.replace("<KeyTrack>False</KeyTrack>", "<KeyTrack>True</KeyTrack>")
    stats["keytrack"] = count

    # Fix 2: RootNote non-zero → 0
    # Only change non-zero RootNote values
    def fix_rootnote(match):
        val = int(match.group(1))
        if val != 0:
            stats["rootnote"] += 1
            return "<RootNote>0</RootNote>"
        return match.group(0)
    text = re.sub(r"<RootNote>(\d+)</RootNote>", fix_rootnote, text)

    # Fix 3: Empty layers — set VelStart to 0
    # Strategy: find Layer blocks with empty SampleName and fix their VelStart
    # We process instrument-by-instrument to handle layers correctly
    def fix_empty_layer_vel(match):
        layer_block = match.group(0)
        # Check if this layer has an empty SampleName
        if "<SampleName></SampleName>" in layer_block:
            old = "<VelStart>1</VelStart>"
            if old in layer_block:
                stats["velstart"] += 1
                layer_block = layer_block.replace(old, "<VelStart>0</VelStart>")
        return layer_block

    text = re.sub(
        r"<Layer number=\"\d+\">.*?</Layer>",
        fix_empty_layer_vel,
        text,
        flags=re.DOTALL
    )

    if text != original and not dry_run:
        filepath.write_text(text, encoding="utf-8")

    changed = text != original
    return {"changed": changed, **stats}


def main():
    dry_run = "--dry-run" in sys.argv

    if not EXPANSION_DIR.exists():
        print(f"ERROR: Expansion directory not found: {EXPANSION_DIR}")
        return 1

    xpm_files = sorted(EXPANSION_DIR.glob("*.xpm"))
    print(f"Found {len(xpm_files)} XPM files in {EXPANSION_DIR.name}")
    print(f"Mode: {'DRY RUN' if dry_run else 'LIVE — writing fixes'}\n")

    total = {"files": 0, "changed": 0, "keytrack": 0, "rootnote": 0, "velstart": 0}

    for fpath in xpm_files:
        stats = fix_xpm(fpath, dry_run)
        total["files"] += 1
        if stats["changed"]:
            total["changed"] += 1
        total["keytrack"] += stats["keytrack"]
        total["rootnote"] += stats["rootnote"]
        total["velstart"] += stats["velstart"]

        if stats["changed"] and dry_run:
            print(f"  [WOULD FIX] {fpath.name}: "
                  f"KeyTrack×{stats['keytrack']}, "
                  f"RootNote×{stats['rootnote']}, "
                  f"VelStart×{stats['velstart']}")

    print(f"\n{'=' * 50}")
    print(f"RESULTS")
    print(f"{'=' * 50}")
    print(f"Files scanned:     {total['files']}")
    print(f"Files {'would fix' if dry_run else 'fixed'}:  {total['changed']}")
    print(f"KeyTrack fixes:    {total['keytrack']}")
    print(f"RootNote fixes:    {total['rootnote']}")
    print(f"VelStart fixes:    {total['velstart']}")

    if not dry_run and total["changed"] > 0:
        print(f"\nAll fixes applied. Restart MPC to test.")

    return 0


if __name__ == "__main__":
    sys.exit(main())
