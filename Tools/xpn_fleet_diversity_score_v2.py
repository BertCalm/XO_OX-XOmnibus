#!/usr/bin/env python3
"""
xpn_fleet_diversity_score_v2.py
Fleet diversity analysis of the full XOmnibus preset fleet.

Scans all .xometa files in Presets/XOmnibus/{mood}/ directories.
Reads sonic_dna block (with dna as fallback) for each preset.
Computes:
  - Fleet diversity score (mean cosine distance, sampled)
  - Midrange compression % per dimension (0.35–0.65 zone)
  - Extreme zone % per dimension (0.0–0.2 or 0.8–1.0)
  - Distribution histogram per dimension (10 buckets)
  - Per-mood diversity scores
Writes JSON report to Docs/snapshots/diversity_score_latest.json

Stdlib only: json, os, math, pathlib, random, statistics, datetime, argparse
"""

import argparse
import json
import math
import pathlib
import random
import statistics
from collections import defaultdict
from datetime import datetime, timezone

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

DNA_DIMS = ["brightness", "warmth", "movement", "density", "space", "aggression"]
MOODS = ["Foundation", "Atmosphere", "Entangled", "Prism", "Flux", "Aether", "Family"]

MIDRANGE_LOW = 0.35
MIDRANGE_HIGH = 0.65
EXTREME_LOW_HI = 0.20   # <= this = low extreme
EXTREME_HIGH_LO = 0.80  # >= this = high extreme

N_BINS = 10
BAR_WIDTH = 28
SAMPLE_PAIRS = 500

# Reference score from session start for delta display
REFERENCE_DIVERSITY = 0.173

# ---------------------------------------------------------------------------
# Loading
# ---------------------------------------------------------------------------

def load_presets(presets_dir: pathlib.Path) -> list[dict]:
    """
    Walk each mood subdirectory, load all .xometa files.
    Reads sonic_dna first, falls back to dna.
    Skips files missing both blocks or missing any of the 6 dimensions.
    """
    presets = []
    skipped = 0

    for mood in MOODS:
        mood_dir = presets_dir / mood
        if not mood_dir.is_dir():
            continue
        for path in sorted(mood_dir.glob("*.xometa")):
            try:
                with open(path, "r", encoding="utf-8") as f:
                    data = json.load(f)
            except (json.JSONDecodeError, OSError):
                skipped += 1
                continue

            # sonic_dna takes priority over dna
            raw_dna = data.get("sonic_dna") or data.get("dna")
            if not raw_dna:
                skipped += 1
                continue

            # Validate all 6 dimensions are present
            if not all(dim in raw_dna for dim in DNA_DIMS):
                skipped += 1
                continue

            try:
                dna = {dim: float(raw_dna[dim]) for dim in DNA_DIMS}
            except (TypeError, ValueError):
                skipped += 1
                continue

            presets.append({
                "name": data.get("name", path.stem),
                "path": str(path),
                "mood": mood,
                "engines": data.get("engines", []),
                "dna": dna,
                "dna_key": "sonic_dna" if "sonic_dna" in data else "dna",
            })

    return presets, skipped


# ---------------------------------------------------------------------------
# Math helpers
# ---------------------------------------------------------------------------

def cosine_distance(a: list[float], b: list[float]) -> float:
    """
    Cosine distance in [0, 1]: 1 - cosine_similarity.
    Returns 1.0 if either vector is zero.
    """
    dot = sum(x * y for x, y in zip(a, b))
    mag_a = math.sqrt(sum(x * x for x in a))
    mag_b = math.sqrt(sum(x * x for x in b))
    if mag_a == 0.0 or mag_b == 0.0:
        return 1.0
    return 1.0 - (dot / (mag_a * mag_b))


def fleet_diversity_score(presets: list[dict], n_pairs: int = SAMPLE_PAIRS) -> float:
    """
    Mean cosine distance between randomly sampled pairs of preset DNA vectors.
    Uses n_pairs random pairs for efficiency on large fleets.
    Returns value in [0, 1].
    """
    n = len(presets)
    if n < 2:
        return 0.0

    vecs = [[p["dna"][d] for d in DNA_DIMS] for p in presets]

    # If fleet is small enough, use all pairs; otherwise sample
    max_possible_pairs = n * (n - 1) // 2
    if max_possible_pairs <= n_pairs:
        pairs = [(i, j) for i in range(n) for j in range(i + 1, n)]
    else:
        seen: set[tuple[int, int]] = set()
        pairs = []
        attempts = 0
        while len(pairs) < n_pairs and attempts < n_pairs * 10:
            i = random.randrange(n)
            j = random.randrange(n)
            if i != j:
                key = (min(i, j), max(i, j))
                if key not in seen:
                    seen.add(key)
                    pairs.append(key)
            attempts += 1

    if not pairs:
        return 0.0

    total = sum(cosine_distance(vecs[i], vecs[j]) for i, j in pairs)
    return total / len(pairs)


def histogram(values: list[float], n_bins: int = N_BINS) -> list[tuple[float, float, int]]:
    """Return list of (bin_lo, bin_hi, count) covering [0.0, 1.0]."""
    result = [[i / n_bins, (i + 1) / n_bins, 0] for i in range(n_bins)]
    for v in values:
        v_clamped = max(0.0, min(1.0, v))
        idx = min(int(v_clamped * n_bins), n_bins - 1)
        result[idx][2] += 1
    return [(b[0], b[1], b[2]) for b in result]


def ascii_bar(count: int, max_count: int, width: int = BAR_WIDTH) -> str:
    if max_count == 0:
        return " " * width
    filled = int(round(count / max_count * width))
    return "█" * filled + "░" * (width - filled)


# ---------------------------------------------------------------------------
# Per-dimension analysis
# ---------------------------------------------------------------------------

def analyse_dimension(dim: str, values: list[float]) -> dict:
    n = len(values)
    mean_val = statistics.mean(values) if n > 0 else 0.0
    stdev_val = statistics.stdev(values) if n > 1 else 0.0
    midrange_count = sum(1 for v in values if MIDRANGE_LOW <= v <= MIDRANGE_HIGH)
    midrange_pct = midrange_count / n if n else 0.0
    extreme_low_count = sum(1 for v in values if v <= EXTREME_LOW_HI)
    extreme_high_count = sum(1 for v in values if v >= EXTREME_HIGH_LO)
    extreme_pct = (extreme_low_count + extreme_high_count) / n if n else 0.0
    hist = histogram(values)
    return {
        "dim": dim,
        "n": n,
        "mean": mean_val,
        "stdev": stdev_val,
        "midrange_pct": midrange_pct,
        "midrange_count": midrange_count,
        "extreme_pct": extreme_pct,
        "extreme_low_count": extreme_low_count,
        "extreme_high_count": extreme_high_count,
        "hist": hist,
    }


# ---------------------------------------------------------------------------
# Per-mood analysis
# ---------------------------------------------------------------------------

def per_mood_diversity(presets: list[dict], n_pairs: int = SAMPLE_PAIRS) -> dict[str, float]:
    """Return diversity score for each mood."""
    by_mood: dict[str, list[dict]] = defaultdict(list)
    for p in presets:
        by_mood[p["mood"]].append(p)
    return {
        mood: fleet_diversity_score(plist, n_pairs)
        for mood, plist in sorted(by_mood.items())
        if len(plist) >= 2
    }


# ---------------------------------------------------------------------------
# Printing
# ---------------------------------------------------------------------------

def print_header(title: str) -> None:
    width = 70
    print()
    print("=" * width)
    print(f"  {title}")
    print("=" * width)


def print_dim_histogram(da: dict, n_fleet: int) -> None:
    """Print ASCII histogram for one dimension."""
    hist = da["hist"]
    max_count = max(b[2] for b in hist) if hist else 1

    for lo, hi, count in hist:
        bar = ascii_bar(count, max_count)
        pct = count / n_fleet * 100 if n_fleet else 0.0

        # Mark midrange and extreme zones
        if MIDRANGE_LOW <= lo and hi <= MIDRANGE_HIGH + 0.01:
            zone = "MID"
        elif hi <= EXTREME_LOW_HI + 0.01 or lo >= EXTREME_HIGH_LO - 0.01:
            zone = "EXT"
        else:
            zone = "   "

        print(f"    [{lo:.1f}–{hi:.1f}] {zone}  {bar}  {count:4d} ({pct:4.1f}%)")


# ---------------------------------------------------------------------------
# JSON snapshot
# ---------------------------------------------------------------------------

def build_snapshot(
    presets: list[dict],
    fleet_score: float,
    dim_analyses: dict,
    mood_scores: dict,
    skipped: int,
) -> dict:
    """Build the JSON snapshot dict."""
    dims_out = {}
    for dim, da in dim_analyses.items():
        dims_out[dim] = {
            "midrange_pct": round(da["midrange_pct"], 4),
            "extreme_pct": round(da["extreme_pct"], 4),
            "extreme_low_count": da["extreme_low_count"],
            "extreme_high_count": da["extreme_high_count"],
            "mean": round(da["mean"], 4),
            "stdev": round(da["stdev"], 4),
            "histogram": [
                {"lo": round(lo, 1), "hi": round(hi, 1), "count": cnt}
                for lo, hi, cnt in da["hist"]
            ],
        }

    return {
        "generated_at": datetime.now(timezone.utc).isoformat(),
        "total_presets": len(presets),
        "skipped_presets": skipped,
        "fleet_diversity_score": round(fleet_score, 4),
        "reference_diversity_score": REFERENCE_DIVERSITY,
        "delta_from_reference": round(fleet_score - REFERENCE_DIVERSITY, 4),
        "midrange_zone": [MIDRANGE_LOW, MIDRANGE_HIGH],
        "extreme_zones": [f"0.0–{EXTREME_LOW_HI}", f"{EXTREME_HIGH_LO}–1.0"],
        "dimensions": dims_out,
        "per_mood_diversity": {k: round(v, 4) for k, v in mood_scores.items()},
        "sonic_dna_key_usage": {
            "sonic_dna": sum(1 for p in presets if p["dna_key"] == "sonic_dna"),
            "dna": sum(1 for p in presets if p["dna_key"] == "dna"),
        },
    }


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main() -> None:
    parser = argparse.ArgumentParser(
        description="XOmnibus fleet diversity score v2 — cosine-distance based analysis."
    )
    parser.add_argument(
        "--presets-dir",
        type=pathlib.Path,
        default=pathlib.Path(__file__).parent.parent / "Presets" / "XOmnibus",
        help="Root directory containing mood subdirectories with .xometa files.",
    )
    parser.add_argument(
        "--snapshot-dir",
        type=pathlib.Path,
        default=pathlib.Path(__file__).parent.parent / "Docs" / "snapshots",
        help="Directory to write diversity_score_latest.json.",
    )
    parser.add_argument(
        "--pairs",
        type=int,
        default=SAMPLE_PAIRS,
        help=f"Number of random pairs to sample for diversity score (default: {SAMPLE_PAIRS}).",
    )
    parser.add_argument(
        "--seed",
        type=int,
        default=42,
        help="Random seed for reproducible pair sampling (default: 42).",
    )
    parser.add_argument(
        "--no-snapshot",
        action="store_true",
        help="Skip writing JSON snapshot.",
    )
    args = parser.parse_args()

    random.seed(args.seed)

    # ---- Load ---------------------------------------------------------------
    print_header("FLEET DIVERSITY REPORT  v2")
    print(f"  Presets dir : {args.presets_dir}")
    print(f"  Snapshot dir: {args.snapshot_dir}")

    presets, skipped = load_presets(args.presets_dir)
    n = len(presets)

    if n == 0:
        print("\n  ERROR: No valid .xometa presets found. Check --presets-dir.")
        raise SystemExit(1)

    sonic_dna_count = sum(1 for p in presets if p["dna_key"] == "sonic_dna")
    dna_count = n - sonic_dna_count
    print(f"\n  Total presets loaded : {n}")
    print(f"  Skipped (no/bad DNA) : {skipped}")
    print(f"  sonic_dna key        : {sonic_dna_count}  |  dna key: {dna_count}")
    print(f"  Pair sample size     : {args.pairs}  (seed={args.seed})")

    # ---- Fleet diversity score ----------------------------------------------
    print(f"\n  Computing fleet diversity score ({args.pairs} sampled pairs)...")
    fleet_score = fleet_diversity_score(presets, args.pairs)
    delta = fleet_score - REFERENCE_DIVERSITY
    delta_str = f"+{delta:.3f}" if delta >= 0 else f"{delta:.3f}"

    print(f"\n  Diversity score: {fleet_score:.3f}  (was {REFERENCE_DIVERSITY:.3f} at session start, {delta_str})")

    # ---- Per-dimension analysis ---------------------------------------------
    print_header("Dimension Compression Analysis  (midrange = 0.35–0.65)")

    dim_analyses = {}
    for dim in DNA_DIMS:
        values = [p["dna"][dim] for p in presets]
        dim_analyses[dim] = analyse_dimension(dim, values)

    # Summary table first
    print()
    print(f"  {'Dimension':<12}  {'Midrange %':>10}  {'Extreme %':>9}  {'Mean':>6}  {'Std':>5}")
    print(f"  {'-'*12}  {'-'*10}  {'-'*9}  {'-'*6}  {'-'*5}")
    for dim in DNA_DIMS:
        da = dim_analyses[dim]
        compressed_flag = " [!]" if da["midrange_pct"] >= 0.60 else "     "
        print(
            f"  {dim:<12}  {da['midrange_pct']*100:>9.1f}%  "
            f"{da['extreme_pct']*100:>8.1f}%  "
            f"{da['mean']:>6.3f}  {da['stdev']:>5.3f}"
            f"{compressed_flag}"
        )

    # Identify compression ranking
    sorted_dims = sorted(DNA_DIMS, key=lambda d: dim_analyses[d]["midrange_pct"], reverse=True)
    top3_compressed = sorted_dims[:3]

    compressed_dims = [d for d in DNA_DIMS if dim_analyses[d]["midrange_pct"] >= 0.60]
    if compressed_dims:
        print(f"\n  [!] = ≥60% of fleet in midrange band — compressed dimension")
    print(f"\n  Top 3 still-compressed : {', '.join(top3_compressed)}")

    # ---- Per-dimension histograms -------------------------------------------
    print_header("Distribution Histograms  (MID=midrange zone, EXT=extreme zone)")

    for dim in DNA_DIMS:
        da = dim_analyses[dim]
        mid_pct = da["midrange_pct"] * 100
        ext_pct = da["extreme_pct"] * 100
        flag = "  [COMPRESSED]" if da["midrange_pct"] >= 0.60 else ""
        print(f"\n  {dim.upper():<12}  midrange={mid_pct:.1f}%  extreme={ext_pct:.1f}%{flag}")
        print_dim_histogram(da, n)

    # ---- Per-mood diversity -------------------------------------------------
    print_header("Per-Mood Diversity Scores")

    mood_scores = per_mood_diversity(presets, args.pairs)
    mood_counts: dict[str, int] = defaultdict(int)
    for p in presets:
        mood_counts[p["mood"]] += 1

    print()
    print(f"  {'Mood':<14}  {'Presets':>7}  {'Diversity':>9}  {'vs Fleet':>8}")
    print(f"  {'-'*14}  {'-'*7}  {'-'*9}  {'-'*8}")
    for mood in MOODS:
        count = mood_counts.get(mood, 0)
        score = mood_scores.get(mood)
        if score is None:
            print(f"  {mood:<14}  {count:>7}  {'n/a':>9}  {'':>8}")
            continue
        diff = score - fleet_score
        diff_str = f"{diff:+.3f}"
        bar_val = int(score * 20)
        bar = ascii_bar(bar_val, 20, 16)
        print(f"  {mood:<14}  {count:>7}  {score:>9.3f}  {diff_str:>8}  {bar}")

    # ---- Summary ------------------------------------------------------------
    print_header("Summary")

    print(f"\n  Total presets   : {n}")
    print(f"  Diversity score : {fleet_score:.3f}  (was {REFERENCE_DIVERSITY:.3f}, {delta_str})")
    print()

    print(f"  Dimension compression (% in {MIDRANGE_LOW}–{MIDRANGE_HIGH} midrange):")
    max_mid = max(dim_analyses[d]["midrange_pct"] for d in DNA_DIMS)
    for dim in DNA_DIMS:
        da = dim_analyses[dim]
        pct = da["midrange_pct"] * 100
        bar = ascii_bar(int(da["midrange_pct"] * BAR_WIDTH), BAR_WIDTH, BAR_WIDTH)
        flag = " [!]" if da["midrange_pct"] >= 0.60 else "    "
        print(f"    {dim:<12}: {pct:5.1f}%  {bar}{flag}")

    print()
    print(f"  Top 3 still-compressed: {', '.join(top3_compressed)}")

    if fleet_score < 0.20:
        verdict = "CRITICAL — fleet highly clustered. Major diversification needed."
    elif fleet_score < 0.28:
        verdict = "POOR — significant midrange clustering suppresses expressiveness."
    elif fleet_score < 0.35:
        verdict = "FAIR — some spread but extremes under-represented."
    elif fleet_score < 0.45:
        verdict = "GOOD — reasonable diversity with room for polar expansion."
    else:
        verdict = "EXCELLENT — fleet covers the DNA space well."

    print(f"\n  VERDICT: {verdict}")
    print()

    # ---- JSON snapshot ------------------------------------------------------
    if not args.no_snapshot:
        snapshot = build_snapshot(presets, fleet_score, dim_analyses, mood_scores, skipped)
        args.snapshot_dir.mkdir(parents=True, exist_ok=True)
        snapshot_path = args.snapshot_dir / "diversity_score_latest.json"
        with open(snapshot_path, "w", encoding="utf-8") as f:
            json.dump(snapshot, f, indent=2)
        print(f"  JSON snapshot written: {snapshot_path}")
        print()


if __name__ == "__main__":
    main()
