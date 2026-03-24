#!/usr/bin/env python3
"""
xpn_fleet_coupling_coverage_updater.py

After each coupling preset generation wave, update and report the current
coupling coverage stats. Shows progress toward the 561-pair coverage goal.

Usage:
    python Tools/xpn_fleet_coupling_coverage_updater.py
    python Tools/xpn_fleet_coupling_coverage_updater.py --save coverage_baseline.json
    python Tools/xpn_fleet_coupling_coverage_updater.py --baseline coverage_baseline.json --save coverage_new.json
"""

import argparse
import collections
import itertools
import json
import os
import sys
from pathlib import Path

# ---------------------------------------------------------------------------
# Canonical engine list (34 engines)
# ---------------------------------------------------------------------------
ENGINES = [
    "OddfeliX",
    "OddOscar",
    "Overdub",
    "Odyssey",
    "Oblong",
    "Obese",
    "Onset",
    "Overworld",
    "Opal",
    "Orbital",
    "Organon",
    "Ouroboros",
    "Obsidian",
    "Overbite",
    "Origami",
    "Oracle",
    "Obscura",
    "Oceanic",
    "Ocelot",
    "Optic",
    "Oblique",
    "Osprey",
    "Osteria",
    "Owlfish",
    "Ohm",
    "Orphica",
    "Obbligato",
    "Ottoni",
    "Ole",
    "Overlap",
    "Outwit",
    "Ombre",
    "Orca",
    "Octopus",
]

TOTAL_POSSIBLE_PAIRS = len(ENGINES) * (len(ENGINES) - 1) // 2  # C(34,2) = 561
MILESTONE_STEP = 10  # percent

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def all_possible_pairs():
    """Return sorted list of all frozenset pairs from ENGINES."""
    return [frozenset(pair) for pair in itertools.combinations(ENGINES, 2)]


def scan_entangled_presets(presets_dir: Path):
    """
    Walk presets_dir recursively for .xometa files that contain an
    "engines" array with 2+ entries.

    Returns:
        pair_to_presets: dict[frozenset -> list[str]]  (pair -> list of preset filenames)
        engine_preset_count: Counter[str]              (engine -> total coupling preset appearances)
    """
    pair_to_presets = collections.defaultdict(list)
    engine_preset_count = collections.Counter()

    if not presets_dir.exists():
        return pair_to_presets, engine_preset_count

    for root, _dirs, files in os.walk(presets_dir):
        for fname in files:
            if not fname.endswith(".xometa"):
                continue
            fpath = Path(root) / fname
            try:
                with open(fpath, "r", encoding="utf-8") as f:
                    data = json.load(f)
            except (json.JSONDecodeError, OSError):
                continue

            engines_field = data.get("engines")
            if not isinstance(engines_field, list) or len(engines_field) < 2:
                continue

            # Normalize to canonical names (case-insensitive match)
            engine_lower = {e.lower(): e for e in ENGINES}
            matched = []
            for raw in engines_field:
                canonical = engine_lower.get(str(raw).lower())
                if canonical:
                    matched.append(canonical)

            if len(matched) < 2:
                continue

            # Record every unique pair within this preset
            seen_pairs = set()
            for a, b in itertools.combinations(matched, 2):
                pair = frozenset([a, b])
                if pair not in seen_pairs:
                    seen_pairs.add(pair)
                    pair_to_presets[pair].append(fname)

            # Count per-engine appearances
            for eng in set(matched):
                engine_preset_count[eng] += 1

    return pair_to_presets, engine_preset_count


def coverage_state_to_dict(pair_to_presets: dict, engine_preset_count: dict) -> dict:
    """Serialize coverage state to a JSON-safe dict."""
    return {
        "pairs": {
            "__".join(sorted(pair)): list(filenames)
            for pair, filenames in pair_to_presets.items()
        },
        "engine_counts": dict(engine_preset_count),
    }


def dict_to_coverage_state(data: dict):
    """Deserialize coverage state from a JSON dict."""
    pair_to_presets = {}
    for key, filenames in data.get("pairs", {}).items():
        engines = key.split("__")
        if len(engines) == 2:
            pair_to_presets[frozenset(engines)] = filenames
    engine_preset_count = collections.Counter(data.get("engine_counts", {}))
    return pair_to_presets, engine_preset_count


def nearest_milestone(covered: int, total: int) -> tuple:
    """Return (next_milestone_pct, pairs_needed) for the next 10% threshold."""
    current_pct = covered / total * 100
    for threshold in range(MILESTONE_STEP, 101, MILESTONE_STEP):
        if threshold > current_pct:
            pairs_needed = int(total * threshold / 100) - covered
            return threshold, pairs_needed
    return 100, 0  # Already at 100%


# ---------------------------------------------------------------------------
# Reporting
# ---------------------------------------------------------------------------

def print_report(
    pair_to_presets: dict,
    engine_preset_count: collections.Counter,
    baseline_pair_to_presets: dict = None,
    baseline_engine_preset_count: collections.Counter = None,
):
    covered_pairs = set(pair_to_presets.keys())
    n_covered = len(covered_pairs)
    pct = n_covered / TOTAL_POSSIBLE_PAIRS * 100

    print("=" * 64)
    print("  XOlokun Fleet Coupling Coverage Report")
    print("=" * 64)
    print()

    # ---- Overall coverage -----------------------------------------------
    bar_filled = int(pct / 2)  # 50-char bar
    bar = "#" * bar_filled + "-" * (50 - bar_filled)
    print(f"  Overall Coverage : {n_covered}/{TOTAL_POSSIBLE_PAIRS} pairs  ({pct:.1f}%)")
    print(f"  [{bar}]")
    print()

    # ---- Delta since baseline -------------------------------------------
    if baseline_pair_to_presets is not None:
        baseline_covered = set(baseline_pair_to_presets.keys())
        new_pairs = covered_pairs - baseline_covered
        lost_pairs = baseline_covered - covered_pairs  # shouldn't happen but show if so
        print(f"  Delta Since Baseline")
        print(f"    New pairs covered : +{len(new_pairs)}")
        if lost_pairs:
            print(f"    Pairs removed     : -{len(lost_pairs)}  (unexpected — check source)")
        if new_pairs:
            print("    New pairs:")
            for pair in sorted(new_pairs, key=lambda p: sorted(p)):
                a, b = sorted(pair)
                cnt = len(pair_to_presets[pair])
                print(f"      {a} × {b}  ({cnt} preset{'s' if cnt != 1 else ''})")
        print()

    # ---- Nearest milestone ----------------------------------------------
    milestone_pct, pairs_needed = nearest_milestone(n_covered, TOTAL_POSSIBLE_PAIRS)
    if pairs_needed > 0:
        print(f"  Nearest Milestone : {milestone_pct}%  (need {pairs_needed} more pair{'s' if pairs_needed != 1 else ''})")
    else:
        print("  Milestone         : 100% — Full coverage achieved!")
    print()

    # ---- Top 10 richest pairs -------------------------------------------
    print("  Richest Pairs (top 10 by preset count)")
    print("  " + "-" * 54)
    if pair_to_presets:
        richest = sorted(pair_to_presets.items(), key=lambda kv: len(kv[1]), reverse=True)[:10]
        for pair, filenames in richest:
            a, b = sorted(pair)
            print(f"    {a:<14} × {b:<14}  {len(filenames):>3} preset{'s' if len(filenames) != 1 else ''}")
    else:
        print("    (none)")
    print()

    # ---- Top 10 most-coupled engines ------------------------------------
    print("  Engines with Most Coupling Presets (top 10)")
    print("  " + "-" * 54)
    if engine_preset_count:
        top_engines = engine_preset_count.most_common(10)
        for eng, cnt in top_engines:
            print(f"    {eng:<16}  {cnt:>4} coupling preset{'s' if cnt != 1 else ''}")
    else:
        print("    (none)")
    print()

    # ---- Zero-coverage engines (critical gap) ---------------------------
    zero_engines = [e for e in ENGINES if engine_preset_count.get(e, 0) == 0]
    print("  Engines with ZERO Coupling Presets  [Critical Gap]")
    print("  " + "-" * 54)
    if zero_engines:
        for eng in zero_engines:
            print(f"    {eng}")
    else:
        print("    (none — all engines represented)")
    print()

    # ---- Uncovered pairs count ------------------------------------------
    all_pairs = set(all_possible_pairs())
    uncovered = all_pairs - covered_pairs
    print(f"  Uncovered Pairs   : {len(uncovered)}/{TOTAL_POSSIBLE_PAIRS}")
    print()
    print("=" * 64)


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    repo_root = Path(__file__).resolve().parent.parent
    default_presets = repo_root / "Presets" / "XOlokun" / "Entangled"

    parser = argparse.ArgumentParser(
        description="Report XOlokun fleet coupling coverage toward 561-pair goal.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument(
        "--presets-dir",
        type=Path,
        default=default_presets,
        help=f"Path to Entangled presets directory (default: {default_presets})",
    )
    parser.add_argument(
        "--save",
        metavar="FILE",
        type=Path,
        help="Write current coverage state as JSON for future --baseline comparison",
    )
    parser.add_argument(
        "--baseline",
        metavar="FILE",
        type=Path,
        help="Load previous coverage state JSON and show delta",
    )
    args = parser.parse_args()

    # Sanity-check engine count
    assert len(ENGINES) == 34, f"Expected 34 engines, got {len(ENGINES)}"
    assert TOTAL_POSSIBLE_PAIRS == 561, f"Expected 561 pairs, got {TOTAL_POSSIBLE_PAIRS}"

    # Scan current state
    pair_to_presets, engine_preset_count = scan_entangled_presets(args.presets_dir)

    # Load baseline if provided
    baseline_pair_to_presets = None
    baseline_engine_preset_count = None
    if args.baseline:
        if not args.baseline.exists():
            print(f"ERROR: Baseline file not found: {args.baseline}", file=sys.stderr)
            sys.exit(1)
        with open(args.baseline, "r", encoding="utf-8") as f:
            baseline_data = json.load(f)
        baseline_pair_to_presets, baseline_engine_preset_count = dict_to_coverage_state(baseline_data)

    # Print report
    print_report(
        pair_to_presets,
        engine_preset_count,
        baseline_pair_to_presets=baseline_pair_to_presets,
        baseline_engine_preset_count=baseline_engine_preset_count,
    )

    # Save current state if requested
    if args.save:
        state = coverage_state_to_dict(pair_to_presets, engine_preset_count)
        args.save.parent.mkdir(parents=True, exist_ok=True)
        with open(args.save, "w", encoding="utf-8") as f:
            json.dump(state, f, indent=2, sort_keys=True)
        print(f"  Coverage state saved → {args.save}")
        print()

    sys.exit(0)


if __name__ == "__main__":
    main()
