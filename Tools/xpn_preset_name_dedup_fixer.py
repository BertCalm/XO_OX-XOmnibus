#!/usr/bin/env python3
"""
xpn_preset_name_dedup_fixer.py — Fix preset name collisions across the XOceanus fleet.

Detects duplicate preset names (case-insensitive) and proposes rename strategies:
  1. Rename each file by appending the engine name: "Tidal Lock" → "Tidal Lock (Orbital)"
  2. If engine cannot be determined, append the mood: "Tidal Lock (Aether)"
  3. If same engine + same mood (true duplicate), flag as MERGE candidate — do not auto-rename

Dry run by default. Use --apply to write changes.

Usage:
  python xpn_preset_name_dedup_fixer.py --presets-dir Presets/XOceanus
  python xpn_preset_name_dedup_fixer.py --presets-dir Presets/XOceanus --apply
  python xpn_preset_name_dedup_fixer.py --presets-dir Presets/XOceanus --apply --backup

Exit code: 0 always (advisory tool; CI can gate on 0 duplicates separately).
"""

import argparse
import json
import os
import shutil
import sys
from collections import defaultdict
from pathlib import Path
from typing import Optional


# ---------------------------------------------------------------------------
# Engine alias normalisation
# Engine IDs seen in the "engine" / "engines" fields may be legacy names.
# Map everything to the canonical short name used in rename suffixes.
# ---------------------------------------------------------------------------

ENGINE_ALIASES: dict[str, str] = {
    # Canonical IDs
    "OddfeliX": "OddfeliX",
    "OddOscar": "OddOscar",
    "Overdub": "Overdub",
    "Odyssey": "Odyssey",
    "Oblong": "Oblong",
    "Obese": "Obese",
    "Onset": "Onset",
    "Overworld": "Overworld",
    "Opal": "Opal",
    "Orbital": "Orbital",
    "Organon": "Organon",
    "Ouroboros": "Ouroboros",
    "Obsidian": "Obsidian",
    "Origami": "Origami",
    "Oracle": "Oracle",
    "Obscura": "Obscura",
    "Oceanic": "Oceanic",
    "Ocelot": "Ocelot",
    "Overbite": "Overbite",
    "Optic": "Optic",
    "Oblique": "Oblique",
    "Osprey": "Osprey",
    "Osteria": "Osteria",
    "Owlfish": "Owlfish",
    "Ohm": "Ohm",
    "Orphica": "Orphica",
    "Obbligato": "Obbligato",
    "Ottoni": "Ottoni",
    "Ole": "Ole",
    "Ombre": "Ombre",
    "Orca": "Orca",
    "Octopus": "Octopus",
    "Overlap": "Overlap",
    "Outwit": "Outwit",
    # Legacy names
    "Snap": "OddfeliX",
    "Morph": "OddOscar",
    "Dub": "Overdub",
    "Drift": "Odyssey",
    "Bob": "Oblong",
    "Fat": "Obese",
    "Bite": "Overbite",
    # Lower-case variants (in case fields are lower-cased)
    "oddfelix": "OddfeliX",
    "oddoscar": "OddOscar",
    "overdub": "Overdub",
    "odyssey": "Odyssey",
    "oblong": "Oblong",
    "obese": "Obese",
    "onset": "Onset",
    "overworld": "Overworld",
    "opal": "Opal",
    "orbital": "Orbital",
    "organon": "Organon",
    "ouroboros": "Ouroboros",
    "obsidian": "Obsidian",
    "origami": "Origami",
    "oracle": "Oracle",
    "obscura": "Obscura",
    "oceanic": "Oceanic",
    "ocelot": "Ocelot",
    "overbite": "Overbite",
    "optic": "Optic",
    "oblique": "Oblique",
    "osprey": "Osprey",
    "osteria": "Osteria",
    "owlfish": "Owlfish",
    "ohm": "Ohm",
    "orphica": "Orphica",
    "obbligato": "Obbligato",
    "ottoni": "Ottoni",
    "ole": "Ole",
    "ombre": "Ombre",
    "orca": "Orca",
    "octopus": "Octopus",
    "overlap": "Overlap",
    "outwit": "Outwit",
    "snap": "OddfeliX",
    "morph": "OddOscar",
    "dub": "Overdub",
    "drift": "Odyssey",
    "bob": "Oblong",
    "fat": "Obese",
    "bite": "Overbite",
}


def resolve_engine(data: dict) -> Optional[str]:
    """
    Return the canonical engine short name from the preset data dict, or None.
    Checks "engine" (string), then "engines" (list or string).
    """
    raw: str | list | None = data.get("engine") or data.get("engines")
    if raw is None:
        return None

    candidates: list[str] = []
    if isinstance(raw, str):
        candidates = [raw.strip()]
    elif isinstance(raw, list):
        candidates = [str(e).strip() for e in raw if e]

    for candidate in candidates:
        if not candidate:
            continue
        # Try direct lookup first
        if candidate in ENGINE_ALIASES:
            return ENGINE_ALIASES[candidate]
        # Try lower-case lookup
        if candidate.lower() in ENGINE_ALIASES:
            return ENGINE_ALIASES[candidate.lower()]
    return None


# ---------------------------------------------------------------------------
# Loading
# ---------------------------------------------------------------------------

def load_presets(presets_dir: Path) -> list[dict]:
    """
    Walk presets_dir recursively, load every .xometa file.
    Returns list of enriched record dicts with _path, _mood, _engine.
    """
    records: list[dict] = []
    for root, _dirs, files in os.walk(presets_dir):
        for fname in sorted(files):
            if not fname.endswith(".xometa"):
                continue
            fpath = Path(root) / fname
            try:
                with open(fpath, "r", encoding="utf-8") as fh:
                    data = json.load(fh)
            except (json.JSONDecodeError, OSError) as exc:
                print(f"  [WARN] Could not parse {fpath}: {exc}", file=sys.stderr)
                continue

            # Infer mood from directory structure (…/Presets/XOceanus/<mood>/…)
            parts = fpath.parts
            mood = "Unknown"
            try:
                xoceanus_idx = next(
                    i for i, p in enumerate(parts) if p == "XOceanus"
                )
                if xoceanus_idx + 1 < len(parts):
                    mood = parts[xoceanus_idx + 1]
            except StopIteration:
                pass

            engine = resolve_engine(data)

            record = dict(data)
            record["_path"] = fpath
            record["_mood"] = mood
            record["_engine"] = engine  # None if not determinable
            records.append(record)

    return records


# ---------------------------------------------------------------------------
# Duplicate detection
# ---------------------------------------------------------------------------

def find_duplicate_groups(records: list[dict]) -> dict[str, list[dict]]:
    """
    Group records by lower-cased preset name.
    Returns only groups with 2+ members.
    """
    by_name: dict[str, list[dict]] = defaultdict(list)
    for rec in records:
        name = rec.get("name", "").strip()
        if name:
            by_name[name.lower()].append(rec)
    return {k: v for k, v in by_name.items() if len(v) >= 2}


# ---------------------------------------------------------------------------
# Rename strategy
# ---------------------------------------------------------------------------

def make_suffix(rec: dict) -> str:
    """Return the suffix to append: engine name if known, else mood."""
    if rec["_engine"]:
        return rec["_engine"]
    return rec["_mood"]


def propose_renames(groups: dict[str, list[dict]]) -> tuple[list[dict], list[list[dict]]]:
    """
    For each duplicate group decide either:
      - RENAME: append engine/mood suffix to each member
      - MERGE: same engine + same mood for all members — flag for manual review

    Returns:
      renames  — list of {"record": ..., "old_name": ..., "new_name": ...}
      merges   — list of groups (each group is a list of records)
    """
    renames: list[dict] = []
    merges: list[list[dict]] = []

    for _name_key, group in groups.items():
        # Check if this is a true duplicate (all same engine + all same mood)
        engines = {rec["_engine"] for rec in group}
        moods = {rec["_mood"] for rec in group}

        is_true_duplicate = (
            len(engines) == 1
            and None not in engines
            and len(moods) == 1
        )

        if is_true_duplicate:
            merges.append(group)
            continue

        # Propose a rename for every member
        original_name = group[0].get("name", "").strip()

        # Build proposed names; track collisions within this group
        seen_suffixes: dict[str, int] = {}
        for rec in group:
            suffix = make_suffix(rec)
            if suffix in seen_suffixes:
                # Two files in the same group have the same suffix — number them
                seen_suffixes[suffix] += 1
                proposed = f"{original_name} ({suffix} {seen_suffixes[suffix]})"
            else:
                seen_suffixes[suffix] = 1
                proposed = f"{original_name} ({suffix})"

            renames.append({
                "record": rec,
                "old_name": original_name,
                "new_name": proposed,
            })

    return renames, merges


# ---------------------------------------------------------------------------
# Reporting
# ---------------------------------------------------------------------------

COL_SEP = "  "


def _col(text: str, width: int) -> str:
    return text[:width].ljust(width)


def print_rename_table(renames: list[dict]) -> None:
    if not renames:
        return
    print("\nPROPOSED RENAMES")
    print("=" * 90)
    header = (
        _col("OLD NAME", 32)
        + COL_SEP
        + _col("NEW NAME", 38)
        + COL_SEP
        + "FILE"
    )
    print(header)
    print("-" * 90)
    for item in renames:
        row = (
            _col(item["old_name"], 32)
            + COL_SEP
            + _col(item["new_name"], 38)
            + COL_SEP
            + str(item["record"]["_path"])
        )
        print(row)
    print()


def print_merge_table(merges: list[list[dict]]) -> None:
    if not merges:
        return
    print("\nMERGE CANDIDATES (same engine + same mood — manual review required)")
    print("=" * 80)
    for group in merges:
        name = group[0].get("name", "")
        engine = group[0]["_engine"]
        mood = group[0]["_mood"]
        print(f"  Name: {name!r}  Engine: {engine}  Mood: {mood}")
        for rec in group:
            print(f"    {rec['_path']}")
    print()


def print_summary(renames: list[dict], merges: list[list[dict]], applied: bool) -> None:
    n_groups_resolved = len({item["old_name"].lower() for item in renames})
    n_renamed = len(renames)
    n_merge_candidates = sum(len(g) for g in merges)
    mode = "APPLIED" if applied else "DRY RUN"

    print(f"\nSUMMARY [{mode}]")
    print(f"  Groups with renames resolved : {n_groups_resolved}")
    print(f"  Files renamed                : {n_renamed}")
    print(f"  MERGE candidates (files)     : {n_merge_candidates}")
    if not applied and (n_renamed > 0):
        print("\n  Re-run with --apply to write changes.")


# ---------------------------------------------------------------------------
# Writing
# ---------------------------------------------------------------------------

def apply_renames(renames: list[dict], backup: bool) -> int:
    """Write updated 'name' field back to each .xometa file. Returns error count."""
    errors = 0
    for item in renames:
        rec = item["record"]
        fpath: Path = rec["_path"]
        new_name: str = item["new_name"]

        # Re-read the file to avoid writing stale data
        try:
            with open(fpath, "r", encoding="utf-8") as fh:
                data = json.load(fh)
        except (json.JSONDecodeError, OSError) as exc:
            print(f"  [ERROR] Re-read failed for {fpath}: {exc}", file=sys.stderr)
            errors += 1
            continue

        if backup:
            bak_path = fpath.with_suffix(".xometa.bak")
            try:
                shutil.copy2(fpath, bak_path)
            except OSError as exc:
                print(f"  [ERROR] Backup failed for {fpath}: {exc}", file=sys.stderr)
                errors += 1
                continue

        data["name"] = new_name

        try:
            with open(fpath, "w", encoding="utf-8") as fh:
                json.dump(data, fh, indent=2, ensure_ascii=False)
                fh.write("\n")
        except OSError as exc:
            print(f"  [ERROR] Write failed for {fpath}: {exc}", file=sys.stderr)
            errors += 1
            continue

        print(f"  Renamed: {item['old_name']!r} → {new_name!r}  ({fpath.name})")

    return errors


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Detect and fix duplicate preset names across the XOceanus fleet.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Dry run — show all proposed renames (default)
  python xpn_preset_name_dedup_fixer.py --presets-dir Presets/XOceanus

  # Apply renames in-place
  python xpn_preset_name_dedup_fixer.py --presets-dir Presets/XOceanus --apply

  # Apply with .xometa.bak backups
  python xpn_preset_name_dedup_fixer.py --presets-dir Presets/XOceanus --apply --backup

Exit code: always 0 (advisory tool).
        """,
    )
    parser.add_argument(
        "--presets-dir",
        type=Path,
        default=Path("Presets/XOceanus"),
        help="Root directory to scan for .xometa files (default: Presets/XOceanus)",
    )
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument(
        "--dry-run",
        action="store_true",
        default=True,
        help="Show proposed changes without writing (default)",
    )
    mode.add_argument(
        "--apply",
        action="store_true",
        default=False,
        help="Write changes in-place",
    )
    parser.add_argument(
        "--backup",
        action="store_true",
        default=False,
        help="Write .xometa.bak before modifying each file (only with --apply)",
    )
    return parser


def main(argv: Optional[list] = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)

    presets_dir = args.presets_dir.expanduser().resolve()
    if not presets_dir.is_dir():
        print(f"[ERROR] Presets directory not found: {presets_dir}", file=sys.stderr)
        return 0  # advisory; always exit 0

    apply_mode = args.apply
    dry_run = not apply_mode

    if dry_run:
        print(f"[DRY RUN] Scanning {presets_dir} …")
    else:
        print(f"[APPLY] Scanning {presets_dir} …")
        if args.backup:
            print("  Backups enabled (.xometa.bak)")

    # Load
    records = load_presets(presets_dir)
    print(f"  Loaded {len(records)} preset files.")

    if not records:
        print("  No .xometa files found.")
        return 0

    # Detect
    groups = find_duplicate_groups(records)
    total_dup_files = sum(len(v) for v in groups.values())
    print(f"  Found {len(groups)} duplicate name groups ({total_dup_files} files total).")

    if not groups:
        print("\nNo duplicate names detected. Fleet is clean.")
        return 0

    # Propose
    renames, merges = propose_renames(groups)

    # Report
    print_rename_table(renames)
    print_merge_table(merges)

    # Apply
    if apply_mode and renames:
        print("\nApplying renames …")
        errors = apply_renames(renames, backup=args.backup)
        if errors:
            print(f"\n  [WARN] {errors} file(s) could not be updated — see errors above.")

    print_summary(renames, merges, applied=apply_mode)

    return 0


if __name__ == "__main__":
    sys.exit(main())
