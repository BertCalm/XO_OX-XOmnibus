#!/usr/bin/env python3
"""
xpn_fleet_mood_balancer.py — Analyze mood distribution across the XOlokun preset fleet
and recommend which moods need more presets to reach balance.

Usage:
    python Tools/xpn_fleet_mood_balancer.py
    python Tools/xpn_fleet_mood_balancer.py --target-total 3000
    python Tools/xpn_fleet_mood_balancer.py --engine Ocelot
    python Tools/xpn_fleet_mood_balancer.py --target-dist '{"Foundation":20,"Atmosphere":20,"Entangled":15,"Prism":12,"Flux":12,"Aether":14,"Family":7}'
"""

import argparse
import json
import os
import sys
from pathlib import Path
from statistics import mean, stdev
from typing import Dict, List, Optional

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

VALID_MOODS = [
    "Foundation",
    "Atmosphere",
    "Entangled",
    "Prism",
    "Flux",
    "Aether",
    "Family",
]

DEFAULT_TARGETS = {
    "Foundation": 18.0,
    "Atmosphere": 18.0,
    "Entangled": 15.0,
    "Prism": 14.0,
    "Flux": 14.0,
    "Aether": 14.0,
    "Family": 7.0,
}

BAR_WIDTH = 30  # max chars for bar chart bars


# ---------------------------------------------------------------------------
# Scanning
# ---------------------------------------------------------------------------

def scan_xometa_files(presets_dir: Path, engine_filter: Optional[str]) -> List[dict]:
    """Walk presets_dir recursively, parse all .xometa files, return list of records."""
    records = []
    for path in sorted(presets_dir.rglob("*.xometa")):
        try:
            with path.open("r", encoding="utf-8") as f:
                data = json.load(f)
        except (json.JSONDecodeError, OSError) as exc:
            print(f"  WARNING: could not parse {path.relative_to(presets_dir)}: {exc}", file=sys.stderr)
            continue

        mood = data.get("mood", "")
        engines = data.get("engines", [])

        # Engine filter — match any engine name in the list (case-insensitive)
        if engine_filter:
            if not any(e.lower() == engine_filter.lower() for e in engines):
                continue

        records.append({
            "path": path,
            "name": data.get("name", path.stem),
            "mood": mood,
            "engines": engines,
        })
    return records


# ---------------------------------------------------------------------------
# Analysis
# ---------------------------------------------------------------------------

def compute_mood_counts(records: List[dict]) -> Dict[str, int]:
    counts = {m: 0 for m in VALID_MOODS}
    for r in records:
        m = r["mood"]
        if m in counts:
            counts[m] += 1
        else:
            counts[m] = counts.get(m, 0) + 1
    return counts


def compute_per_engine_breakdown(records: List[dict]) -> Dict[str, Dict[str, int]]:
    """Return {engine: {mood: count}}."""
    breakdown: Dict[str, Dict[str, int]] = {}
    for r in records:
        mood = r["mood"]
        for engine in r["engines"]:
            if engine not in breakdown:
                breakdown[engine] = {m: 0 for m in VALID_MOODS}
            if mood in breakdown[engine]:
                breakdown[engine][mood] += 1
            # Ignore unknown moods in per-engine counts (already warned above)
    return breakdown


# ---------------------------------------------------------------------------
# Recommendations
# ---------------------------------------------------------------------------

def compute_recommendations(
    counts: Dict[str, int],
    target_pct: Dict[str, float],
    target_total: int,
) -> List[dict]:
    """
    For each mood compute:
      - current count and pct
      - target pct and target count (at target_total)
      - gap_pct  (target_pct - actual_pct)
      - needed   (max(0, target_count - current_count))
    """
    actual_total = sum(counts.values())
    rows = []
    for mood in VALID_MOODS:
        current = counts.get(mood, 0)
        actual_pct = (current / actual_total * 100) if actual_total else 0.0
        tgt_pct = target_pct.get(mood, 0.0)
        tgt_count = round(tgt_pct / 100 * target_total)
        gap_pct = tgt_pct - actual_pct
        needed = max(0, tgt_count - current)
        rows.append({
            "mood": mood,
            "current": current,
            "actual_pct": actual_pct,
            "target_pct": tgt_pct,
            "target_count": tgt_count,
            "gap_pct": gap_pct,
            "needed": needed,
        })
    return rows


# ---------------------------------------------------------------------------
# ASCII bar chart
# ---------------------------------------------------------------------------

def _bar(value: float, max_value: float, width: int, char: str = "#") -> str:
    if max_value <= 0:
        return ""
    filled = round(value / max_value * width)
    return char * filled + "." * (width - filled)


def print_bar_chart(rows: List[dict]) -> None:
    max_pct = max(max(r["actual_pct"] for r in rows), max(r["target_pct"] for r in rows), 1)

    header_mood = "Mood".ljust(12)
    header_cur = "Actual %".rjust(9)
    header_tgt = "Target %".rjust(9)
    header_gap = "Gap %".rjust(7)
    header_needed = "Needed".rjust(8)
    print(f"\n{'='*76}")
    print("  MOOD DISTRIBUTION — CURRENT vs TARGET")
    print(f"{'='*76}")
    print(f"  {header_mood}  {header_cur}  {header_tgt}  {header_gap}  {header_needed}")
    print(f"  {'-'*12}  {'-'*9}  {'-'*9}  {'-'*7}  {'-'*8}")

    for r in rows:
        mood_label = r["mood"].ljust(12)
        cur_bar  = _bar(r["actual_pct"], max_pct, BAR_WIDTH, "#")
        tgt_bar  = _bar(r["target_pct"], max_pct, BAR_WIDTH, "-")
        gap_sign = "+" if r["gap_pct"] >= 0 else ""
        line = (
            f"  {mood_label}"
            f"  {r['actual_pct']:8.1f}%"
            f"  {r['target_pct']:8.1f}%"
            f"  {gap_sign}{r['gap_pct']:6.1f}%"
            f"  {r['needed']:8d}"
        )
        print(line)

    # Visual bars on separate lines
    print()
    print("  BAR CHART  (#=actual  -=target)")
    print(f"  {'0%':<4}{'':>{BAR_WIDTH - 8}}{int(max_pct)}%")
    print(f"  {'Mood':<12}  {'Actual / Target bars'}")
    print(f"  {'-'*12}  {'-'*BAR_WIDTH}")
    for r in rows:
        cur_bar = _bar(r["actual_pct"], max_pct, BAR_WIDTH, "#")
        tgt_bar = _bar(r["target_pct"], max_pct, BAR_WIDTH, "-")
        print(f"  {r['mood']:<12}  {cur_bar}  {r['actual_pct']:.1f}%")
        print(f"  {'':12}  {tgt_bar}  {r['target_pct']:.1f}% (target)")
    print(f"{'='*76}")


# ---------------------------------------------------------------------------
# Per-engine balance
# ---------------------------------------------------------------------------

def print_engine_breakdown(breakdown: Dict[str, Dict[str, int]]) -> None:
    if not breakdown:
        return
    print(f"\n{'='*76}")
    print("  PER-ENGINE MOOD COVERAGE")
    print(f"{'='*76}")
    header = "Engine".ljust(16) + "  " + "  ".join(m[:4].ljust(6) for m in VALID_MOODS) + "  Total"
    print(f"  {header}")
    print(f"  {'-'*16}  " + "  ".join("-"*6 for _ in VALID_MOODS) + "  -----")

    for engine in sorted(breakdown.keys()):
        counts = breakdown[engine]
        total = sum(counts.values())
        if total == 0:
            continue
        row = f"  {engine:<16}  "
        row += "  ".join(f"{counts.get(m, 0):6d}" for m in VALID_MOODS)
        row += f"  {total:5d}"
        print(row)

        # Flag engines with zero coverage in any mood
        missing = [m for m in VALID_MOODS if counts.get(m, 0) == 0]
        if missing:
            print(f"  {'':16}  WARNING: no presets for {', '.join(missing)}")

    print(f"{'='*76}")


# ---------------------------------------------------------------------------
# Recommendations summary
# ---------------------------------------------------------------------------

def print_recommendations(rows: List[dict], actual_total: int, target_total: int) -> None:
    overrepresented = [r for r in rows if r["gap_pct"] < -1.0]
    underrepresented = [r for r in rows if r["gap_pct"] > 1.0]

    print(f"\n{'='*76}")
    print("  RECOMMENDATIONS")
    print(f"{'='*76}")
    print(f"  Current fleet size : {actual_total:,} presets")
    print(f"  Target fleet size  : {target_total:,} presets")
    total_needed = sum(r["needed"] for r in rows)
    print(f"  Total presets to add (to reach target balance): {total_needed:,}")
    print()

    if not underrepresented and not overrepresented:
        print("  Fleet is well-balanced. No significant gaps detected.")
    else:
        if underrepresented:
            print("  UNDER-REPRESENTED moods (gap > 1%):")
            for r in sorted(underrepresented, key=lambda x: -x["gap_pct"]):
                print(
                    f"    {r['mood']:<12}  Need {r['needed']:4d} more presets "
                    f"({r['actual_pct']:.1f}% actual → {r['target_pct']:.1f}% target, "
                    f"gap +{r['gap_pct']:.1f}%)"
                )
        if overrepresented:
            print()
            print("  OVER-REPRESENTED moods (gap < -1%):")
            for r in sorted(overrepresented, key=lambda x: x["gap_pct"]):
                print(
                    f"    {r['mood']:<12}  {abs(r['gap_pct']):.1f}% over target "
                    f"({r['actual_pct']:.1f}% actual vs {r['target_pct']:.1f}% target)"
                )

    print(f"{'='*76}")


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def build_arg_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog="xpn_fleet_mood_balancer",
        description="Analyze mood distribution across the XOlokun preset fleet.",
    )
    parser.add_argument(
        "--presets-dir",
        default=None,
        help=(
            "Root directory to scan for .xometa files. "
            "Defaults to <repo-root>/Source/Engines (auto-detected)."
        ),
    )
    parser.add_argument(
        "--target-total",
        type=int,
        default=None,
        help="Target fleet size for balance calculations. Defaults to current fleet size.",
    )
    parser.add_argument(
        "--target-dist",
        default=None,
        help=(
            'Custom target distribution as JSON, e.g. \'{"Foundation":20,"Atmosphere":20,"Entangled":15,"Prism":12,"Flux":12,"Aether":14,"Family":7}\'. '
            "Values are percentages and should sum to 100."
        ),
    )
    parser.add_argument(
        "--engine",
        default=None,
        help="Filter analysis to a single engine name (case-insensitive), e.g. --engine Ocelot",
    )
    parser.add_argument(
        "--no-engine-breakdown",
        action="store_true",
        help="Skip the per-engine mood breakdown table.",
    )
    return parser


def resolve_presets_dir(arg: Optional[str]) -> Path:
    """Auto-detect the repo root and default presets dir if not supplied."""
    if arg:
        p = Path(arg)
        if not p.exists():
            print(f"ERROR: --presets-dir '{arg}' does not exist.", file=sys.stderr)
            sys.exit(1)
        return p

    # Walk upward from this file's location to find the repo root
    script_dir = Path(__file__).resolve().parent
    candidates = [
        script_dir.parent / "Source" / "Engines",
        script_dir.parent / "Presets",
        script_dir.parent,
    ]
    for c in candidates:
        if c.exists():
            return c
    return script_dir.parent


def parse_target_dist(raw: Optional[str]) -> Dict[str, float]:
    if raw is None:
        return dict(DEFAULT_TARGETS)
    try:
        parsed = json.loads(raw)
    except json.JSONDecodeError as exc:
        print(f"ERROR: --target-dist is not valid JSON: {exc}", file=sys.stderr)
        sys.exit(1)

    result: Dict[str, float] = {}
    for mood in VALID_MOODS:
        val = parsed.get(mood, DEFAULT_TARGETS.get(mood, 0.0))
        result[mood] = float(val)

    total = sum(result.values())
    if abs(total - 100.0) > 0.5:
        print(
            f"WARNING: --target-dist percentages sum to {total:.1f}% (expected 100%).",
            file=sys.stderr,
        )
    return result


def main() -> None:
    parser = build_arg_parser()
    args = parser.parse_args()

    presets_dir = resolve_presets_dir(args.presets_dir)
    target_dist = parse_target_dist(args.target_dist)

    print(f"\nScanning: {presets_dir}")
    if args.engine:
        print(f"Engine filter: {args.engine}")

    records = scan_xometa_files(presets_dir, args.engine)

    if not records:
        print("No .xometa files found. Check --presets-dir.", file=sys.stderr)
        sys.exit(0)

    actual_total = len(records)
    target_total = args.target_total if args.target_total else actual_total

    print(f"Found {actual_total:,} preset(s).")

    counts = compute_mood_counts(records)

    # Warn about unrecognised moods
    all_moods_seen = {r["mood"] for r in records}
    unknown = all_moods_seen - set(VALID_MOODS)
    if unknown:
        print(f"  WARNING: unrecognised mood(s) in files: {', '.join(sorted(unknown))}", file=sys.stderr)

    rows = compute_recommendations(counts, target_dist, target_total)

    print_bar_chart(rows)

    if not args.no_engine_breakdown:
        breakdown = compute_per_engine_breakdown(records)
        print_engine_breakdown(breakdown)

    print_recommendations(rows, actual_total, target_total)

    # Print target distribution note if customised
    if args.target_dist:
        print("\n  (Custom target distribution applied)")
    else:
        print(
            "\n  Default targets: Foundation=18% Atmosphere=18% Entangled=15% "
            "Prism=14% Flux=14% Aether=14% Family=7%"
        )

    sys.exit(0)


if __name__ == "__main__":
    main()
