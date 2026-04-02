#!/usr/bin/env python3
"""
dedup_preset_names.py — Find and fix duplicate preset names across the XOceanus fleet.

Scans ALL mood subdirectories under Presets/XOceanus/ so cross-mood duplicates are
caught (fixes #236 — 824 cross-mood duplicates reported when 388 unique names appeared
in 2+ moods). Cross-mood duplicates were fully resolved by prior dedup passes; this
script is the ongoing guard to prevent regressions.

Usage:
  python dedup_preset_names.py             # dry-run (default)
  python dedup_preset_names.py --dry-run   # explicit dry-run
  python dedup_preset_names.py --apply     # write changes to disk

Strategy for disambiguating duplicates:
  - The first file (alphabetical path order) keeps its original name.
  - Each subsequent duplicate is renamed: "Base Name (EngineName)"
  - If that is also taken, roman numerals are appended: "Base Name (EngineName II)"
"""

import argparse
import json
import sys
from collections import defaultdict
from pathlib import Path
from typing import Dict, List, Optional, Set, Tuple

ROMAN = ["I", "II", "III", "IV", "V", "VI", "VII", "VIII", "IX", "X",
         "XI", "XII", "XIII", "XIV", "XV", "XVI", "XVII", "XVIII", "XIX", "XX"]

PRESETS_ROOT = Path(__file__).parent.parent / "Presets" / "XOceanus"


def to_roman(n: int) -> str:
    """Return roman numeral string for 1-based integer n (1=I, 2=II, …)."""
    if 1 <= n <= len(ROMAN):
        return ROMAN[n - 1]
    return str(n)


def engine_name_from_meta(data: dict) -> str:
    """Extract engine name from the engines array; fall back to 'Unknown'."""
    engines = data.get("engines", [])
    if engines and isinstance(engines, list) and engines[0]:
        return engines[0]
    return "Unknown"


def build_new_name(base_name: str, engine: str, suffix_index: int, taken: set) -> str:
    """
    Build a unique name for a duplicate.

    suffix_index=1 → "Base (Engine)"
    suffix_index=2 → "Base (Engine II)"
    suffix_index=3 → "Base (Engine III)"
    … and so on, skipping any that are already taken.
    """
    candidate = f"{base_name} ({engine})"
    if suffix_index == 1 and candidate not in taken:
        return candidate

    # suffix_index >= 2, or the plain engine name is taken — add roman numerals
    for roman_n in range(1, 100):
        candidate = f"{base_name} ({engine} {to_roman(roman_n)})"
        if candidate not in taken:
            return candidate

    raise RuntimeError(f"Could not find unique name for '{base_name}' ({engine})")


def collect_files() -> list[Path]:
    """Return sorted list of all .xometa files under PRESETS_ROOT."""
    return sorted(PRESETS_ROOT.rglob("*.xometa"))


def load_preset(path: Path) -> Optional[dict]:
    """Load JSON from a .xometa file; return None on parse error."""
    try:
        with path.open("r", encoding="utf-8") as f:
            return json.load(f)
    except (json.JSONDecodeError, OSError) as exc:
        print(f"  WARNING: could not read {path}: {exc}", file=sys.stderr)
        return None


def save_preset(path: Path, data: dict) -> None:
    """Write preset JSON back to disk with consistent formatting."""
    text = json.dumps(data, indent=2, ensure_ascii=False) + "\n"
    with path.open("w", encoding="utf-8") as f:
        f.write(text)


def find_and_fix_duplicates(
    files: List[Path], apply: bool
) -> Tuple[List[Tuple[Path, str, str]], int]:
    """
    Core dedup logic.

    Returns:
        renames  — list of (path, old_name, new_name)
        total_duplicates — number of files that shared a name with at least one other
    """
    # Phase 1: index all presets by name
    # name → list of (path, data)
    by_name: Dict[str, List[Tuple[Path, dict]]] = defaultdict(list)
    data_cache: Dict[Path, dict] = {}

    for path in files:
        data = load_preset(path)
        if data is None:
            continue
        name = data.get("name", "").strip()
        if not name:
            continue
        data_cache[path] = data
        by_name[name].append((path, data))

    # Phase 2: for each group of duplicates, decide new names
    renames: List[Tuple[Path, str, str]] = []

    # Collect all current names so we can check uniqueness globally
    all_current_names: Set[str] = {
        data.get("name", "") for data in data_cache.values()
    }

    total_duplicates = 0

    for original_name, group in sorted(by_name.items()):
        if len(group) < 2:
            continue

        total_duplicates += len(group)

        # First file (sorted order == alphabetical file path) is kept as-is
        keeper_path, _keeper_data = group[0]

        # Names that are "taken" — starts with every name currently in the fleet
        # We'll extend this set as we assign new names within this group
        taken: Set[str] = set(all_current_names)

        for suffix_idx, (path, data) in enumerate(group[1:], start=1):
            engine = engine_name_from_meta(data)
            new_name = build_new_name(original_name, engine, suffix_idx, taken)
            taken.add(new_name)
            all_current_names.add(new_name)
            renames.append((path, original_name, new_name))

    # Phase 3: apply if requested
    if apply:
        for path, old_name, new_name in renames:
            data = data_cache[path]
            data["name"] = new_name
            save_preset(path, data)

    return renames, total_duplicates


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Find and fix duplicate preset names in XOceanus fleet."
    )
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument(
        "--dry-run",
        action="store_true",
        default=False,
        help="Print changes without writing to disk (default behaviour).",
    )
    mode.add_argument(
        "--apply",
        action="store_true",
        default=False,
        help="Write updated name fields to .xometa files.",
    )
    args = parser.parse_args()

    # Default to dry-run when neither flag is given
    applying = args.apply

    if not PRESETS_ROOT.exists():
        print(f"ERROR: Presets directory not found: {PRESETS_ROOT}", file=sys.stderr)
        sys.exit(1)

    print(f"Scanning: {PRESETS_ROOT}")
    files = collect_files()
    print(f"Found {len(files)} .xometa files\n")

    renames, total_duplicates = find_and_fix_duplicates(files, apply=applying)

    mode_label = "APPLY" if applying else "DRY-RUN"
    print(f"=== {mode_label} ===\n")

    if not renames:
        print("No duplicate preset names found. Fleet is clean.")
        return

    # Print all renames
    for path, old_name, new_name in renames:
        rel = path.relative_to(PRESETS_ROOT)
        print(f"  [{rel}]")
        print(f"    '{old_name}'  →  '{new_name}'")

    print()
    print("=" * 60)
    print(f"Total presets sharing a name with another:  {total_duplicates}")
    print(f"Total presets renamed:                       {len(renames)}")

    if not applying:
        print("\nRun with --apply to write changes to disk.")
    else:
        print("\nChanges applied.")


if __name__ == "__main__":
    main()
