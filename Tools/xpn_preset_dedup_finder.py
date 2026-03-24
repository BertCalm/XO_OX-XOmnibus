#!/usr/bin/env python3
"""
xpn_preset_dedup_finder.py — Find duplicate and near-duplicate presets across the XOlokun fleet.

Goes beyond cosine similarity: targets exact structural duplicates using three independent signals.

Detection modes:
  1. name   — Same preset name (case-insensitive) anywhere in the fleet
  2. dna    — Identical 6D sonic_dna dict values
  3. params — MD5 of sorted parameters dict (same preset, different names)
  all       — Run all three modes (default)

Usage:
  python xpn_preset_dedup_finder.py --presets-dir Presets/XOlokun
  python xpn_preset_dedup_finder.py --presets-dir Presets/XOlokun --mode dna
  python xpn_preset_dedup_finder.py --presets-dir Presets/XOlokun --fix-names

Exit codes:
  0 — No duplicates found (clean)
  1 — Duplicates detected
"""

import argparse
import collections
import hashlib
import json
import os
import sys
from pathlib import Path


# ---------------------------------------------------------------------------
# Loading
# ---------------------------------------------------------------------------

def load_presets(presets_dir: Path) -> list[dict]:
    """
    Walk presets_dir recursively, load every .xometa file, return list of
    record dicts enriched with _path, _mood, _engine metadata.
    """
    records = []
    for root, _dirs, files in os.walk(presets_dir):
        for fname in files:
            if not fname.endswith(".xometa"):
                continue
            fpath = Path(root) / fname
            try:
                with open(fpath, "r", encoding="utf-8") as fh:
                    data = json.load(fh)
            except (json.JSONDecodeError, OSError) as exc:
                print(f"  [WARN] Could not parse {fpath}: {exc}", file=sys.stderr)
                continue

            # Infer mood from directory structure (…/Presets/XOlokun/<mood>/…)
            parts = fpath.parts
            mood = "Unknown"
            try:
                xolokun_idx = next(
                    i for i, p in enumerate(parts) if p == "XOlokun"
                )
                if xolokun_idx + 1 < len(parts):
                    mood = parts[xolokun_idx + 1]
            except StopIteration:
                pass

            # Derive engine from engines array or primary_engine field
            engine = "Unknown"
            if "primary_engine" in data:
                engine = data["primary_engine"]
            elif "engines" in data and isinstance(data["engines"], list) and data["engines"]:
                engine = data["engines"][0]

            records.append(
                {
                    "_path": str(fpath),
                    "_mood": mood,
                    "_engine": engine,
                    "_data": data,
                }
            )

    return records


# ---------------------------------------------------------------------------
# Hashing helpers
# ---------------------------------------------------------------------------

def _dna_key(data: dict):
    """Return a canonical string for the sonic_dna dict, or None if absent."""
    dna = data.get("sonic_dna")
    if not isinstance(dna, dict) or not dna:
        return None
    # Sort keys for stability
    return json.dumps(dna, sort_keys=True)


def _params_hash(data: dict):
    """Return MD5 hex of the sorted parameters dict, or None if absent."""
    params = data.get("parameters")
    if not isinstance(params, dict) or not params:
        return None
    canonical = json.dumps(params, sort_keys=True).encode("utf-8")
    return hashlib.md5(canonical).hexdigest()


def _preset_name(data: dict):
    name = data.get("name") or data.get("preset_name")
    if name:
        return str(name).strip().lower()
    return None


# ---------------------------------------------------------------------------
# Detection
# ---------------------------------------------------------------------------

def find_name_dupes(records: list[dict]) -> list[dict]:
    """Group records sharing the same case-insensitive name."""
    groups: dict[str, list[dict]] = collections.defaultdict(list)
    for rec in records:
        key = _preset_name(rec["_data"])
        if key:
            groups[key].append(rec)
    return [
        {
            "key": key,
            "type": "name",
            "members": members,
        }
        for key, members in groups.items()
        if len(members) > 1
    ]


def find_dna_dupes(records: list[dict]) -> list[dict]:
    """Group records with identical 6D sonic_dna values."""
    groups: dict[str, list[dict]] = collections.defaultdict(list)
    for rec in records:
        key = _dna_key(rec["_data"])
        if key:
            groups[key].append(rec)
    return [
        {
            "key": key,
            "type": "dna",
            "members": members,
        }
        for key, members in groups.items()
        if len(members) > 1
    ]


def find_params_dupes(records: list[dict]) -> list[dict]:
    """Group records with identical parameters dicts (by MD5)."""
    groups: dict[str, list[dict]] = collections.defaultdict(list)
    for rec in records:
        key = _params_hash(rec["_data"])
        if key:
            groups[key].append(rec)
    return [
        {
            "key": key,
            "type": "params",
            "members": members,
        }
        for key, members in groups.items()
        if len(members) > 1
    ]


# ---------------------------------------------------------------------------
# Reporting
# ---------------------------------------------------------------------------

def _short_path(path: str, presets_dir: Path) -> str:
    try:
        return str(Path(path).relative_to(presets_dir))
    except ValueError:
        return path


def _recommendation(group: dict) -> str:
    """
    Return a terse keep/merge/rename recommendation for a duplicate group.
    """
    gtype = group["type"]
    n = len(group["members"])
    if gtype == "name":
        return f"RENAME — {n} presets share the same name; append engine suffix to each (e.g. 'Drift Pad [OPAL]')"
    if gtype == "dna":
        return f"REVIEW — {n} presets have identical Sonic DNA; confirm intentional or differentiate one dimension"
    if gtype == "params":
        return f"MERGE — {n} presets are parameter-identical; keep the best-named copy and delete the rest"
    return "UNKNOWN"


def print_report(all_groups: list[dict], presets_dir: Path) -> None:
    if not all_groups:
        print("✓ No duplicates found. Fleet is clean.")
        return

    # Tally by type
    by_type: dict[str, list] = collections.defaultdict(list)
    for g in all_groups:
        by_type[g["type"]].append(g)

    total = sum(len(g["members"]) for g in all_groups)
    total_groups = len(all_groups)

    print("=" * 72)
    print(f"DUPLICATE REPORT — {total_groups} duplicate group(s) affecting {total} preset files")
    print("=" * 72)

    # Summary table
    for dtype, groups in sorted(by_type.items()):
        affected = sum(len(g["members"]) for g in groups)
        print(f"  [{dtype.upper():6s}]  {len(groups):3d} group(s)  /  {affected:4d} affected files")
    print()

    # Worst offenders
    sorted_groups = sorted(all_groups, key=lambda g: len(g["members"]), reverse=True)
    print("WORST OFFENDERS (largest groups first):")
    print("-" * 72)
    for rank, group in enumerate(sorted_groups, 1):
        members = group["members"]
        gtype = group["type"].upper()
        key_display = group["key"]
        if gtype == "DNA":
            try:
                dna = json.loads(group["key"])
                key_display = ", ".join(f"{k}={v}" for k, v in sorted(dna.items()))
            except Exception:
                pass
        elif gtype == "PARAMS":
            key_display = f"MD5:{group['key'][:12]}…"
        else:
            key_display = f'"{group["key"]}"'

        print(f"\n#{rank}  [{gtype}]  {len(members)} copies  —  {key_display}")
        print(f"     Recommendation: {_recommendation(group)}")
        for m in members:
            name = _preset_name(m["_data"]) or "(no name)"
            engine = m["_engine"]
            mood = m["_mood"]
            short = _short_path(m["_path"], presets_dir)
            print(f"       • [{engine:12s}] [{mood:12s}]  {short}")
            if gtype == "DNA":
                dna = m["_data"].get("sonic_dna", {})
                vals = " | ".join(f"{k}={v}" for k, v in sorted(dna.items()))
                print(f"         DNA: {vals}")

    print()
    print("=" * 72)
    print(f"ACTION REQUIRED: {total_groups} duplicate group(s) found. Run with --fix-names to auto-rename name dupes.")
    print("=" * 72)


# ---------------------------------------------------------------------------
# Fix names
# ---------------------------------------------------------------------------

def fix_names(name_groups: list[dict], presets_dir: Path, dry_run: bool = False) -> int:
    """
    Auto-append engine suffix to name duplicates.
    Returns count of files modified.
    """
    if not name_groups:
        print("No name duplicates to fix.")
        return 0

    modified = 0
    for group in name_groups:
        members = group["members"]
        # Skip if all members are already differentiated (different engines)
        engines = [m["_engine"] for m in members]
        if len(set(engines)) == len(members):
            # All different engines — append engine to each
            suffix_needed = True
        else:
            suffix_needed = True  # Append engine in all cases for clarity

        for m in members:
            fpath = Path(m["_path"])
            data = m["_data"]
            engine = m["_engine"]
            old_name = data.get("name") or data.get("preset_name") or ""
            # If already has engine suffix, skip
            suffix = f"[{engine}]"
            if old_name.endswith(suffix):
                continue
            new_name = f"{old_name} {suffix}"
            short = _short_path(str(fpath), presets_dir)
            if dry_run:
                print(f"  [DRY RUN] Would rename: '{old_name}' -> '{new_name}'  ({short})")
            else:
                if "name" in data:
                    data["name"] = new_name
                elif "preset_name" in data:
                    data["preset_name"] = new_name
                with open(fpath, "w", encoding="utf-8") as fh:
                    json.dump(data, fh, indent=2, ensure_ascii=False)
                    fh.write("\n")
                print(f"  RENAMED: '{old_name}' -> '{new_name}'  ({short})")
            modified += 1

    return modified


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        description="Find exact duplicate presets across the XOlokun fleet.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    p.add_argument(
        "--presets-dir",
        default="Presets/XOlokun",
        help="Root directory containing .xometa preset files (default: Presets/XOlokun)",
    )
    p.add_argument(
        "--mode",
        choices=["all", "name", "dna", "params"],
        default="all",
        help="Which duplicate signal(s) to check (default: all)",
    )
    p.add_argument(
        "--fix-names",
        action="store_true",
        help="Auto-append engine suffix to name duplicate presets in-place",
    )
    p.add_argument(
        "--dry-run",
        action="store_true",
        help="With --fix-names: show what would change without writing files",
    )
    p.add_argument(
        "--json",
        action="store_true",
        dest="json_output",
        help="Emit machine-readable JSON report to stdout",
    )
    return p


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()

    presets_dir = Path(args.presets_dir).resolve()
    if not presets_dir.is_dir():
        print(f"ERROR: presets-dir not found: {presets_dir}", file=sys.stderr)
        return 1

    print(f"Scanning: {presets_dir}", file=sys.stderr)
    records = load_presets(presets_dir)
    print(f"Loaded {len(records)} preset file(s).", file=sys.stderr)

    all_groups: list[dict] = []
    name_groups: list[dict] = []

    if args.mode in ("all", "name"):
        name_groups = find_name_dupes(records)
        all_groups.extend(name_groups)
        print(f"  name   duplicates: {len(name_groups)} group(s)", file=sys.stderr)

    if args.mode in ("all", "dna"):
        dna_groups = find_dna_dupes(records)
        all_groups.extend(dna_groups)
        print(f"  dna    duplicates: {len(dna_groups)} group(s)", file=sys.stderr)

    if args.mode in ("all", "params"):
        params_groups = find_params_dupes(records)
        all_groups.extend(params_groups)
        print(f"  params duplicates: {len(params_groups)} group(s)", file=sys.stderr)

    # --fix-names
    if args.fix_names:
        if not name_groups and args.mode in ("all", "name"):
            print("No name duplicates to fix.")
        elif args.mode not in ("all", "name"):
            print("--fix-names requires --mode all or --mode name", file=sys.stderr)
        else:
            n = fix_names(name_groups, presets_dir, dry_run=args.dry_run)
            print(f"{'[DRY RUN] ' if args.dry_run else ''}Fixed {n} file(s).")
        # Re-run name check to see if resolved
        if not args.dry_run and args.mode in ("all", "name"):
            remaining = find_name_dupes(load_presets(presets_dir))
            if not remaining:
                all_groups = [g for g in all_groups if g["type"] != "name"]

    # Output
    if args.json_output:
        out = []
        for g in all_groups:
            out.append(
                {
                    "type": g["type"],
                    "key": g["key"],
                    "count": len(g["members"]),
                    "recommendation": _recommendation(g),
                    "members": [
                        {
                            "path": m["_path"],
                            "mood": m["_mood"],
                            "engine": m["_engine"],
                            "name": _preset_name(m["_data"]),
                            "sonic_dna": m["_data"].get("sonic_dna"),
                        }
                        for m in g["members"]
                    ],
                }
            )
        print(json.dumps(out, indent=2))
    else:
        print_report(all_groups, presets_dir)

    # Exit code
    return 1 if all_groups else 0


if __name__ == "__main__":
    sys.exit(main())
