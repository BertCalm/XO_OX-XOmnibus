#!/usr/bin/env python3
"""
XOlokun — Find Presets Missing Sonic DNA Blocks

Scans all .xometa files and identifies presets that are missing a top-level
`sonic_dna` or `dna` block (or have an incomplete block — any of the 6 required
dimensions absent).

Usage:
    python3 Tools/find_missing_dna.py
    python3 Tools/find_missing_dna.py --verbose   # show full file paths
"""

import json
import glob
import sys
from collections import defaultdict
from pathlib import Path

REPO_ROOT = Path(__file__).parent.parent
PRESET_DIR = REPO_ROOT / "Presets"

REQUIRED_DIMS = ["brightness", "warmth", "movement", "density", "space", "aggression"]


def engine_short_name(engine_str: str) -> str:
    """Return a display-friendly short name for the engine."""
    return engine_str.replace("X", "", 1) if engine_str.startswith("X") else engine_str


def scan_presets() -> list[dict]:
    """
    Scan all .xometa files.

    Returns:
        missing: list of dicts with keys: path, name, mood, engines, reason
        total:   total number of files scanned
    """
    files = sorted(glob.glob(str(PRESET_DIR / "**" / "*.xometa"), recursive=True))
    missing = []
    total = len(files)

    for fpath in files:
        try:
            with open(fpath, encoding="utf-8") as fh:
                data = json.load(fh)
        except (json.JSONDecodeError, IOError) as e:
            print(f"  WARN: could not parse {fpath}: {e}", file=sys.stderr)
            total -= 1
            continue

        dna = data.get("sonic_dna") or data.get("dna")

        if dna is None:
            reason = "no dna/sonic_dna key"
        else:
            missing_dims = [d for d in REQUIRED_DIMS if d not in dna]
            if missing_dims:
                reason = f"missing dims: {', '.join(missing_dims)}"
            else:
                continue  # complete — skip

        engines = data.get("engines", [])
        missing.append({
            "path": fpath,
            "name": data.get("name", Path(fpath).stem),
            "mood": data.get("mood", "Unknown"),
            "engines": engines,
            "reason": reason,
        })

    return missing, total


def group_by_engine(missing: list[dict]) -> dict[str, list[dict]]:
    """Group missing-DNA entries by engine name."""
    by_engine = defaultdict(list)
    no_engine = []
    for entry in missing:
        if entry["engines"]:
            for eng in entry["engines"]:
                by_engine[eng].append(entry)
        else:
            no_engine.append(entry)
    if no_engine:
        by_engine["(no engine)"] = no_engine
    return by_engine


def main() -> int:
    verbose = "--verbose" in sys.argv or "-v" in sys.argv

    missing, total = scan_presets()
    complete = total - len(missing)

    sep = "=" * 70

    print(sep)
    print("XOlokun — Missing Sonic DNA Report")
    print(sep)
    print(f"Total .xometa files scanned : {total}")
    print(f"Presets with complete DNA   : {complete}")
    print(f"Presets MISSING DNA         : {len(missing)}")
    print()

    if not missing:
        print("All presets have complete DNA blocks. Nothing to do.")
        return 0

    by_engine = group_by_engine(missing)

    print(sep)
    print("MISSING DNA — GROUPED BY ENGINE")
    print(sep)
    print(f"{'Engine':<20} {'Count':>6}   Moods")
    print("-" * 70)

    for engine in sorted(by_engine.keys()):
        entries = by_engine[engine]
        moods = sorted(set(e["mood"] for e in entries))
        print(f"{engine:<20} {len(entries):>6}   {', '.join(moods)}")

    if verbose:
        print()
        print(sep)
        print("FULL FILE LIST")
        print(sep)
        for engine in sorted(by_engine.keys()):
            print(f"\n  [{engine}]")
            for entry in sorted(by_engine[engine], key=lambda x: x["path"]):
                rel = Path(entry["path"]).relative_to(REPO_ROOT)
                print(f"    {rel}  ({entry['mood']})  — {entry['reason']}")

    print()
    print(sep)
    print(f"Run `python3 Tools/add_missing_dna.py --dry-run` to preview backfill.")
    print(f"Run `python3 Tools/add_missing_dna.py`          to apply backfill.")
    print(sep)

    return 0


if __name__ == "__main__":
    sys.exit(main())
