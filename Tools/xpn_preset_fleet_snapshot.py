#!/usr/bin/env python3
"""
xpn_preset_fleet_snapshot.py
Generate a point-in-time fleet snapshot JSON and human-readable report.

Usage:
  python3 Tools/xpn_preset_fleet_snapshot.py [--presets-dir DIR] [--save [FILE]] [--report] [--compare SNAPSHOT]
"""

import argparse
import json
import os
import statistics
import sys
from collections import Counter, defaultdict
from datetime import date
from pathlib import Path


# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

MOODS = ["Foundation", "Atmosphere", "Entangled", "Prism", "Flux", "Aether", "Family"]
DNA_KEYS = ["brightness", "warmth", "movement", "density", "space", "aggression"]
ENTANGLED_PAIRS_TOTAL = 561

REPO_ROOT = Path(__file__).resolve().parent.parent
DEFAULT_PRESETS_DIR = REPO_ROOT / "Presets" / "XOmnibus"
DEFAULT_SNAPSHOT_DIR = REPO_ROOT / "Docs" / "snapshots"
TOOLS_DIR = REPO_ROOT / "Tools"
DOCS_DIR = REPO_ROOT / "Docs"


# ---------------------------------------------------------------------------
# Scanning helpers
# ---------------------------------------------------------------------------

def load_presets(presets_dir: Path) -> list[dict]:
    """Load all .xometa files under presets_dir and return list of dicts."""
    presets = []
    for path in presets_dir.rglob("*.xometa"):
        try:
            with open(path, "r", encoding="utf-8") as fh:
                data = json.load(fh)
            data["_path"] = str(path)
            presets.append(data)
        except Exception:
            pass
    return presets


def count_files(directory: Path, extension: str) -> int:
    if not directory.exists():
        return 0
    return sum(1 for _ in directory.rglob(f"*{extension}"))


# ---------------------------------------------------------------------------
# Snapshot computation
# ---------------------------------------------------------------------------

def build_snapshot(presets: list[dict], tools_count: int, docs_count: int) -> dict:
    today = date.today().isoformat()
    total = len(presets)

    # --- by_mood ---
    by_mood: dict[str, int] = Counter()
    for p in presets:
        mood = p.get("mood") or "Unknown"
        by_mood[mood] += 1

    # --- by_engine ---
    by_engine: dict[str, int] = Counter()
    for p in presets:
        engines = p.get("engines") or []
        if isinstance(engines, list):
            for eng in engines:
                by_engine[eng] += 1
        elif isinstance(engines, str):
            by_engine[engines] += 1

    # --- DNA fleet stats ---
    dna_vectors: dict[str, list[float]] = defaultdict(list)
    for p in presets:
        dna = p.get("dna") or {}
        for key in DNA_KEYS:
            val = dna.get(key)
            if val is not None:
                try:
                    dna_vectors[key].append(float(val))
                except (TypeError, ValueError):
                    pass

    dna_fleet_mean: dict[str, float] = {}
    dna_fleet_std: dict[str, float] = {}
    for key in DNA_KEYS:
        vals = dna_vectors[key]
        if vals:
            dna_fleet_mean[key] = round(statistics.mean(vals), 4)
            dna_fleet_std[key] = round(statistics.pstdev(vals), 4)
        else:
            dna_fleet_mean[key] = None
            dna_fleet_std[key] = None

    # --- entangled pairs covered ---
    seen_pairs: set[frozenset] = set()
    for p in presets:
        coupling = p.get("coupling") or {}
        if isinstance(coupling, list):
            pairs = coupling
        else:
            pairs = coupling.get("pairs") or []
        for pair in pairs:
            eng_a = pair.get("engineA")
            eng_b = pair.get("engineB")
            if eng_a and eng_b:
                seen_pairs.add(frozenset([eng_a, eng_b]))
    entangled_pairs_covered = len(seen_pairs)

    # --- zero_tag_count ---
    zero_tag_count = sum(1 for p in presets if not p.get("tags"))

    # --- duplicate_name_count ---
    name_counter: Counter = Counter()
    for p in presets:
        name = (p.get("name") or "").strip()
        if name:
            name_counter[name] += 1
    duplicate_name_count = sum(1 for cnt in name_counter.values() if cnt > 1)

    return {
        "snapshot_date": today,
        "total_presets": total,
        "by_mood": dict(sorted(by_mood.items())),
        "by_engine": dict(sorted(by_engine.items(), key=lambda x: -x[1])),
        "dna_fleet_mean": dna_fleet_mean,
        "dna_fleet_std": dna_fleet_std,
        "entangled_pairs_covered": entangled_pairs_covered,
        "entangled_pairs_total": ENTANGLED_PAIRS_TOTAL,
        "zero_tag_count": zero_tag_count,
        "duplicate_name_count": duplicate_name_count,
        "tools_count": tools_count,
        "docs_count": docs_count,
    }


# ---------------------------------------------------------------------------
# Human-readable report
# ---------------------------------------------------------------------------

def trend_arrow(old_val, new_val) -> str:
    """Return ▲, ▼, or → based on comparison. Returns → if either is None."""
    if old_val is None or new_val is None:
        return "→"
    if new_val > old_val:
        return "▲"
    if new_val < old_val:
        return "▼"
    return "→"


def fmt_delta(old_val, new_val) -> str:
    """Return a compact delta string like '+12' or '-3' or '—'."""
    if old_val is None or new_val is None:
        return "—"
    diff = new_val - old_val
    if diff > 0:
        return f"+{diff}"
    if diff < 0:
        return str(diff)
    return "±0"


def print_report(snap: dict, prev=None) -> None:
    compare = prev is not None
    sep = "=" * 60

    def row(label: str, key: str, good_direction: str = "up") -> None:
        """Print a single metric row."""
        val = snap.get(key)
        if compare:
            old = prev.get(key)
            arrow = trend_arrow(old, val)
            # flip arrow meaning for metrics where lower is better
            if good_direction == "down":
                display_arrow = {"▲": "▼ !", "▼": "▲  ", "→": "→  "}.get(arrow, arrow)
            else:
                display_arrow = arrow
            delta = fmt_delta(old, val)
            print(f"  {label:<36} {val!s:<8} {display_arrow}  ({delta})")
        else:
            print(f"  {label:<36} {val}")

    print(sep)
    print("  XO_OX PRESET FLEET SNAPSHOT")
    print(f"  Date : {snap['snapshot_date']}")
    if compare:
        print(f"  Vs   : {prev['snapshot_date']}")
    print(sep)

    print("\n[ TOTALS ]")
    row("Total presets", "total_presets")
    row("Tools (.py) in Tools/", "tools_count")
    row("Docs (.md) in Docs/", "docs_count")

    print("\n[ BY MOOD ]")
    by_mood = snap.get("by_mood", {})
    prev_mood = (prev or {}).get("by_mood", {})
    for mood in MOODS:
        cnt = by_mood.get(mood, 0)
        if compare:
            old = prev_mood.get(mood, 0)
            arrow = trend_arrow(old, cnt)
            delta = fmt_delta(old, cnt)
            print(f"  {mood:<20} {cnt:<8} {arrow}  ({delta})")
        else:
            print(f"  {mood:<20} {cnt}")
    # Any moods not in canonical list
    for mood, cnt in sorted(by_mood.items()):
        if mood not in MOODS:
            print(f"  {mood:<20} {cnt}  [non-standard]")

    print("\n[ TOP ENGINES BY PRESET COUNT ]")
    by_engine = snap.get("by_engine", {})
    prev_engine = (prev or {}).get("by_engine", {})
    top_engines = sorted(by_engine.items(), key=lambda x: -x[1])[:20]
    for eng, cnt in top_engines:
        if compare:
            old = prev_engine.get(eng, 0)
            arrow = trend_arrow(old, cnt)
            delta = fmt_delta(old, cnt)
            print(f"  {eng:<22} {cnt:<8} {arrow}  ({delta})")
        else:
            print(f"  {eng:<22} {cnt}")

    print("\n[ DNA FLEET MEAN ]")
    dna_mean = snap.get("dna_fleet_mean", {})
    dna_std = snap.get("dna_fleet_std", {})
    prev_mean = (prev or {}).get("dna_fleet_mean", {})
    for key in DNA_KEYS:
        val = dna_mean.get(key)
        std = dna_std.get(key)
        if compare:
            old = prev_mean.get(key)
            arrow = trend_arrow(old, val)
            delta = fmt_delta(old, val) if (old is not None and val is not None) else "—"
            print(f"  {key:<14} mean={val!s:<8} std={std!s:<8} {arrow}  (Δ {delta})")
        else:
            print(f"  {key:<14} mean={val!s:<8} std={std}")

    print("\n[ HEALTH METRICS ]")
    ep_cov = snap.get("entangled_pairs_covered", 0)
    ep_tot = snap.get("entangled_pairs_total", ENTANGLED_PAIRS_TOTAL)
    ep_pct = round(100 * ep_cov / ep_tot, 1) if ep_tot else 0
    if compare:
        old_ep = (prev or {}).get("entangled_pairs_covered", 0)
        arrow = trend_arrow(old_ep, ep_cov)
        delta = fmt_delta(old_ep, ep_cov)
        print(f"  {'Entangled pairs covered':<36} {ep_cov}/{ep_tot} ({ep_pct}%)  {arrow}  ({delta})")
    else:
        print(f"  {'Entangled pairs covered':<36} {ep_cov}/{ep_tot} ({ep_pct}%)")

    row("Presets with zero tags", "zero_tag_count", good_direction="down")
    row("Duplicate preset names", "duplicate_name_count", good_direction="down")

    print()
    print(sep)


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate XO_OX preset fleet snapshot JSON and/or report."
    )
    parser.add_argument(
        "--presets-dir",
        type=Path,
        default=DEFAULT_PRESETS_DIR,
        help=f"Path to XOmnibus presets directory (default: {DEFAULT_PRESETS_DIR})",
    )
    parser.add_argument(
        "--save",
        nargs="?",
        const="auto",
        metavar="FILE",
        help=(
            "Write snapshot JSON to FILE. "
            "If FILE is omitted, writes to Docs/snapshots/fleet_YYYYMMDD.json"
        ),
    )
    parser.add_argument(
        "--report",
        action="store_true",
        help="Print human-readable report to stdout (default if --save not given)",
    )
    parser.add_argument(
        "--compare",
        type=Path,
        metavar="SNAPSHOT_FILE",
        help="Compare current snapshot against a previously saved JSON snapshot",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()

    presets_dir: Path = args.presets_dir
    if not presets_dir.exists():
        print(f"ERROR: presets directory not found: {presets_dir}", file=sys.stderr)
        return 1

    presets = load_presets(presets_dir)
    tools_count = count_files(TOOLS_DIR, ".py")
    docs_count = count_files(DOCS_DIR, ".md")

    snap = build_snapshot(presets, tools_count, docs_count)

    # Load previous snapshot for comparison
    prev_snap = None
    if args.compare:
        compare_path = Path(args.compare)
        if not compare_path.exists():
            print(f"ERROR: comparison snapshot not found: {compare_path}", file=sys.stderr)
            return 1
        with open(compare_path, "r", encoding="utf-8") as fh:
            prev_snap = json.load(fh)

    # Determine output actions
    do_save = args.save is not None
    do_report = args.report or not do_save  # default to report if not saving

    if do_save:
        if args.save == "auto":
            DEFAULT_SNAPSHOT_DIR.mkdir(parents=True, exist_ok=True)
            out_path = DEFAULT_SNAPSHOT_DIR / f"fleet_{snap['snapshot_date'].replace('-', '')}.json"
        else:
            out_path = Path(args.save)
            out_path.parent.mkdir(parents=True, exist_ok=True)
        with open(out_path, "w", encoding="utf-8") as fh:
            json.dump(snap, fh, indent=2)
        print(f"Snapshot saved to {out_path}")

    if do_report:
        print_report(snap, prev=prev_snap)

    return 0


if __name__ == "__main__":
    sys.exit(main())
