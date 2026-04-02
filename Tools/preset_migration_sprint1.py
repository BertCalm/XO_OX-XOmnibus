#!/usr/bin/env python3
"""
preset_migration_sprint1.py — XOceanus .xometa Preset Migration (Sprint 1)

Applies 7 P0/P1 fixes across all .xometa files in Presets/XOceanus/:

  Fix 1: Replace `coupling: null` with `{"pairs": []}`
  Fix 2: Fix wrong parameter prefixes per engine (engine-scoped, not global)
  Fix 3: Strip legacy DNA fields (sonic_dna, sonicDNA) when canonical `dna` exists
  Fix 4: Standardize version string "1.0" -> "1.0.0"
  Fix 5: Add missing `coupling` key -> `{"pairs": []}`
  Fix 6: Add missing `tempo` field -> null
  Fix 7: Flag empty parameter blocks (report only, no modification)

Usage:
    python3 Tools/preset_migration_sprint1.py              # dry run (default)
    python3 Tools/preset_migration_sprint1.py --apply      # apply fixes
    python3 Tools/preset_migration_sprint1.py --report-only  # report only, no writes
"""

import argparse
import glob
import json
import os
import sys
from collections import defaultdict
from pathlib import Path


# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------

PRESETS_ROOT = Path(__file__).parent.parent / "Presets" / "XOceanus"

# Maps engine name -> {wrong_prefix: correct_prefix}
# Fixes are applied ONLY inside the named engine's parameter block.
# Note: org_ is the CORRECT prefix for Organism; Organon uses organon_.
PREFIX_FIXES = {
    "Oracle":    {"orc_":   "oracle_"},
    "Optic":     {"opt_":   "optic_"},
    "OddfeliX":  {"felix_": "snap_",   "morph_": "snap_"},
    "OddOscar":  {"oscar_": "morph_",  "oddo_":  "morph_"},
    "Obese":     {"ob_":    "fat_"},
    "Onset":     {"ons_":   "perc_"},
    "Organon":   {"org_":   "organon_"},
    "Ouroboros": {"uro_":   "ouro_"},
    "Obsidian":  {"obs_":   "obsidian_"},
    "Ombre":     {"ombr_":  "ombre_"},
}

# Fix 3: legacy DNA field names to remove when canonical "dna" is present
LEGACY_DNA_KEYS = {"sonic_dna", "sonicDNA"}


# ---------------------------------------------------------------------------
# Per-fix counters
# ---------------------------------------------------------------------------

class Stats:
    def __init__(self):
        self.total_scanned = 0
        self.total_modified = 0
        self.modified_files = []

        # Fix counts
        self.fix1_coupling_null = 0        # coupling: null -> {"pairs": []}
        self.fix2_prefix_renames = 0       # individual param key renames
        self.fix2_files_affected = 0       # files touched by prefix fix
        self.fix3_dna_stripped = 0         # legacy DNA keys removed
        self.fix4_version_bumped = 0       # "1.0" -> "1.0.0"
        self.fix5_coupling_added = 0       # missing coupling key added
        self.fix6_tempo_added = 0          # missing tempo key added
        self.fix7_empty_param_flags = []   # (file_path, engine_name) tuples


def apply_fix1_coupling_null(data: dict, stats: Stats, path: str) -> bool:
    """Fix 1: Replace `coupling: null` with `{"pairs": []}`."""
    if data.get("coupling") is None and "coupling" in data:
        data["coupling"] = {"pairs": []}
        stats.fix1_coupling_null += 1
        return True
    return False


def apply_fix2_prefix_renames(data: dict, stats: Stats, path: str) -> bool:
    """Fix 2: Rename stale parameter prefixes, scoped per engine block."""
    changed = False
    parameters = data.get("parameters", {})

    for engine_name, prefix_map in PREFIX_FIXES.items():
        if engine_name not in parameters:
            continue

        engine_params = parameters[engine_name]
        if not isinstance(engine_params, dict):
            continue

        new_params = {}
        engine_changed = False

        for key, value in engine_params.items():
            renamed = False
            for wrong_prefix, correct_prefix in prefix_map.items():
                if key.startswith(wrong_prefix):
                    new_key = correct_prefix + key[len(wrong_prefix):]
                    new_params[new_key] = value
                    stats.fix2_prefix_renames += 1
                    renamed = True
                    engine_changed = True
                    break
            if not renamed:
                new_params[key] = value

        if engine_changed:
            parameters[engine_name] = new_params
            changed = True

    if changed:
        stats.fix2_files_affected += 1

    return changed


def apply_fix3_strip_legacy_dna(data: dict, stats: Stats, path: str) -> bool:
    """Fix 3: Remove sonic_dna / sonicDNA when canonical `dna` exists."""
    if "dna" not in data:
        return False

    changed = False
    for legacy_key in LEGACY_DNA_KEYS:
        if legacy_key in data:
            del data[legacy_key]
            stats.fix3_dna_stripped += 1
            changed = True

    return changed


def apply_fix4_version_string(data: dict, stats: Stats, path: str) -> bool:
    """Fix 4: Standardize version "1.0" -> "1.0.0"."""
    if data.get("version") == "1.0":
        data["version"] = "1.0.0"
        stats.fix4_version_bumped += 1
        return True
    return False


def apply_fix5_add_coupling(data: dict, stats: Stats, path: str) -> bool:
    """Fix 5: Add missing `coupling` key."""
    if "coupling" not in data:
        data["coupling"] = {"pairs": []}
        stats.fix5_coupling_added += 1
        return True
    return False


def apply_fix6_add_tempo(data: dict, stats: Stats, path: str) -> bool:
    """Fix 6: Add missing `tempo` field."""
    if "tempo" not in data:
        data["tempo"] = None
        stats.fix6_tempo_added += 1
        return True
    return False


def check_fix7_empty_params(data: dict, stats: Stats, path: str) -> None:
    """Fix 7: Flag engines whose parameter block is empty or macro-only (report only)."""
    parameters = data.get("parameters", {})
    if not isinstance(parameters, dict):
        return

    for engine_name, engine_params in parameters.items():
        if not isinstance(engine_params, dict):
            continue

        # Count non-macro params
        non_macro = [k for k in engine_params if not k.startswith("macro_")]
        # Also consider keys like "opal_macroCoupling" — these have the engine prefix
        # A block is "macro-only" when ALL keys contain "macro" anywhere after the prefix
        def is_macro_param(key: str) -> bool:
            # Strip any engine prefix (up to first _) then check remaining
            parts = key.split("_", 1)
            if len(parts) == 2:
                return "macro" in parts[1].lower()
            return "macro" in key.lower()

        substantive = [k for k in engine_params if not is_macro_param(k)]

        if len(engine_params) == 0 or len(substantive) == 0:
            stats.fix7_empty_param_flags.append((path, engine_name))


# ---------------------------------------------------------------------------
# Core file processor
# ---------------------------------------------------------------------------

def process_file(file_path: str, stats: Stats, dry_run: bool, report_only: bool) -> None:
    """Load, fix, and (optionally) write a single .xometa file."""
    stats.total_scanned += 1

    try:
        with open(file_path, "r", encoding="utf-8") as f:
            raw = f.read()
        data = json.loads(raw)
    except (json.JSONDecodeError, OSError) as exc:
        print(f"  ERROR reading {file_path}: {exc}", file=sys.stderr)
        return

    file_changed = False

    # Apply fixes in order — each returns True if the file was mutated
    file_changed |= apply_fix1_coupling_null(data, stats, file_path)
    file_changed |= apply_fix2_prefix_renames(data, stats, file_path)
    file_changed |= apply_fix3_strip_legacy_dna(data, stats, file_path)
    file_changed |= apply_fix4_version_string(data, stats, file_path)
    file_changed |= apply_fix5_add_coupling(data, stats, file_path)
    file_changed |= apply_fix6_add_tempo(data, stats, file_path)

    # Fix 7 is report-only — always run
    check_fix7_empty_params(data, stats, file_path)

    if not file_changed:
        return

    stats.total_modified += 1
    stats.modified_files.append(file_path)

    if dry_run or report_only:
        # Print what would happen, but don't write
        rel = os.path.relpath(file_path, PRESETS_ROOT.parent.parent)
        print(f"  [DRY RUN] Would modify: {rel}")
        return

    # Write back with consistent formatting + trailing newline
    new_content = json.dumps(data, indent=2, ensure_ascii=False) + "\n"
    with open(file_path, "w", encoding="utf-8") as f:
        f.write(new_content)


# ---------------------------------------------------------------------------
# Report
# ---------------------------------------------------------------------------

def print_report(stats: Stats, dry_run: bool, report_only: bool) -> None:
    mode = "DRY RUN" if dry_run else ("REPORT ONLY" if report_only else "APPLIED")
    print()
    print("=" * 60)
    print(f"  XOceanus Preset Migration Sprint 1 — {mode}")
    print("=" * 60)
    print(f"  Files scanned:          {stats.total_scanned:>6,}")
    print(f"  Files with changes:     {stats.total_modified:>6,}")
    print()
    print("  Fix breakdown:")
    print(f"    Fix 1  coupling:null  → {{pairs:[]}}: {stats.fix1_coupling_null:>5,}  files")
    print(f"    Fix 2  prefix renames (params):       {stats.fix2_prefix_renames:>5,}  keys  "
          f"({stats.fix2_files_affected} files)")
    print(f"    Fix 3  legacy DNA keys stripped:      {stats.fix3_dna_stripped:>5,}  fields")
    print(f"    Fix 4  version '1.0' → '1.0.0':       {stats.fix4_version_bumped:>5,}  files")
    print(f"    Fix 5  coupling key added:             {stats.fix5_coupling_added:>5,}  files")
    print(f"    Fix 6  tempo key added:                {stats.fix6_tempo_added:>5,}  files")
    print(f"    Fix 7  empty param blocks flagged:     {len(stats.fix7_empty_param_flags):>5,}  blocks")
    print()

    if stats.fix7_empty_param_flags:
        print("  Fix 7 — Empty/macro-only parameter blocks:")
        for file_path, engine_name in stats.fix7_empty_param_flags:
            rel = os.path.relpath(file_path, PRESETS_ROOT.parent.parent)
            print(f"    [{engine_name}]  {rel}")
        print()

    if dry_run or report_only:
        print("  (No files were written — run with --apply to commit changes)")
    else:
        print(f"  {stats.total_modified:,} files written successfully.")

    print("=" * 60)


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main() -> None:
    parser = argparse.ArgumentParser(
        description="XOceanus .xometa preset migration — Sprint 1 P0/P1 fixes."
    )
    mode_group = parser.add_mutually_exclusive_group()
    mode_group.add_argument(
        "--apply",
        action="store_true",
        help="Actually write changes to disk (default is dry-run).",
    )
    mode_group.add_argument(
        "--report-only",
        action="store_true",
        dest="report_only",
        help="Scan and report only; never write even if --apply would be set.",
    )
    args = parser.parse_args()

    # Determine effective mode
    dry_run = not args.apply and not args.report_only
    report_only = args.report_only

    if not PRESETS_ROOT.exists():
        print(f"ERROR: Presets root not found: {PRESETS_ROOT}", file=sys.stderr)
        sys.exit(1)

    # Collect all .xometa files in deterministic (sorted) order
    pattern = str(PRESETS_ROOT / "**" / "*.xometa")
    all_files = sorted(glob.glob(pattern, recursive=True))

    if not all_files:
        print(f"No .xometa files found under {PRESETS_ROOT}", file=sys.stderr)
        sys.exit(1)

    mode_label = "DRY RUN" if dry_run else ("REPORT ONLY" if report_only else "APPLYING FIXES")
    print(f"[preset_migration_sprint1] {mode_label} — {len(all_files):,} files found")

    stats = Stats()

    for file_path in all_files:
        process_file(file_path, stats, dry_run=dry_run, report_only=report_only)

    print_report(stats, dry_run=dry_run, report_only=report_only)


if __name__ == "__main__":
    main()
