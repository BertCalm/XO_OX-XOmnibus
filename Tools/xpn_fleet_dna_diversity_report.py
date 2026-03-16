#!/usr/bin/env python3
"""
xpn_fleet_dna_diversity_report.py
Deep DNA diversity analysis of the full XOmnibus preset fleet.

Diagnoses WHY fleet diversity is low and prescribes targeted fills.
Stdlib only: json, os, math, argparse, pathlib, statistics
"""

import json
import os
import math
import argparse
import pathlib
import statistics
from collections import defaultdict

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

DNA_DIMS = ["brightness", "warmth", "movement", "density", "space", "aggression"]
MIDRANGE_LOW = 0.3
MIDRANGE_HIGH = 0.7
COMPRESSION_THRESHOLD = 0.60   # 60%+ in midrange = compression zone
CORRELATION_THRESHOLD = 0.70   # |r| > 0.7 = unhealthy co-movement
N_BINS = 10
BAR_WIDTH = 30  # max chars for ASCII bar

# ---------------------------------------------------------------------------
# Loading
# ---------------------------------------------------------------------------

def load_presets(presets_dir: pathlib.Path, engine_filter=None) -> list[dict]:
    """Walk presets_dir recursively, load all .xometa files."""
    presets = []
    for path in sorted(presets_dir.rglob("*.xometa")):
        try:
            with open(path, "r", encoding="utf-8") as f:
                data = json.load(f)
        except (json.JSONDecodeError, OSError):
            continue

        dna = data.get("dna")
        if not dna:
            continue

        # Validate all 6 dimensions present
        if not all(dim in dna for dim in DNA_DIMS):
            continue

        engines = data.get("engines", [])
        if engine_filter:
            if not any(engine_filter.lower() in e.lower() for e in engines):
                continue

        presets.append({
            "name": data.get("name", path.stem),
            "path": str(path),
            "engines": engines,
            "mood": data.get("mood", "Unknown"),
            "dna": {dim: float(dna[dim]) for dim in DNA_DIMS},
        })

    return presets

# ---------------------------------------------------------------------------
# Math helpers
# ---------------------------------------------------------------------------

def pearson_r(xs: list[float], ys: list[float]) -> float:
    n = len(xs)
    if n < 2:
        return 0.0
    mx = statistics.mean(xs)
    my = statistics.mean(ys)
    num = sum((x - mx) * (y - my) for x, y in zip(xs, ys))
    denom_x = math.sqrt(sum((x - mx) ** 2 for x in xs))
    denom_y = math.sqrt(sum((y - my) ** 2 for y in ys))
    if denom_x == 0 or denom_y == 0:
        return 0.0
    return num / (denom_x * denom_y)


def fleet_diversity_score(presets: list[dict]) -> float:
    """
    Mean pairwise L2 distance in 6D DNA space, normalised by sqrt(6).
    Returns value in [0, 1].
    """
    n = len(presets)
    if n < 2:
        return 0.0
    max_dist = math.sqrt(6.0)
    total = 0.0
    count = 0
    vecs = [[p["dna"][d] for d in DNA_DIMS] for p in presets]
    for i in range(n):
        for j in range(i + 1, n):
            dist = math.sqrt(sum((vecs[i][k] - vecs[j][k]) ** 2 for k in range(6)))
            total += dist / max_dist
            count += 1
    return total / count if count else 0.0


def simulate_diversity_with_fills(
    presets: list[dict],
    fills: list[dict],
) -> float:
    """
    Estimate the diversity score if fills were added to the fleet.
    Uses a random sample of the combined set to keep computation fast.
    """
    combined = presets + fills
    # For large fleets, sample to keep computation tractable
    import random
    sample_size = min(500, len(combined))
    if len(combined) > sample_size:
        sampled = random.sample(combined, sample_size)
    else:
        sampled = combined
    return fleet_diversity_score(sampled)


def histogram(values: list[float], n_bins: int = N_BINS) -> list[tuple[float, float, int]]:
    """Return list of (bin_lo, bin_hi, count)."""
    bins = [(i / n_bins, (i + 1) / n_bins, 0) for i in range(n_bins)]
    result = [list(b) for b in bins]
    for v in values:
        idx = min(int(v * n_bins), n_bins - 1)
        result[idx][2] += 1
    return [(b[0], b[1], b[2]) for b in result]


def ascii_bar(count: int, max_count: int, width: int = BAR_WIDTH) -> str:
    if max_count == 0:
        return ""
    filled = int(round(count / max_count * width))
    return "█" * filled + "░" * (width - filled)

# ---------------------------------------------------------------------------
# Per-dimension analysis
# ---------------------------------------------------------------------------

def analyse_dimension(dim: str, values: list[float]) -> dict:
    n = len(values)
    mean_val = statistics.mean(values)
    stdev_val = statistics.stdev(values) if n > 1 else 0.0
    min_val = min(values)
    max_val = max(values)
    midrange_count = sum(1 for v in values if MIDRANGE_LOW <= v <= MIDRANGE_HIGH)
    midrange_pct = midrange_count / n if n else 0.0
    compressed = midrange_pct >= COMPRESSION_THRESHOLD
    hist = histogram(values)
    low_count = sum(1 for v in values if v < MIDRANGE_LOW)
    high_count = sum(1 for v in values if v > MIDRANGE_HIGH)
    return {
        "dim": dim,
        "n": n,
        "mean": mean_val,
        "stdev": stdev_val,
        "min": min_val,
        "max": max_val,
        "midrange_pct": midrange_pct,
        "compressed": compressed,
        "hist": hist,
        "low_count": low_count,
        "high_count": high_count,
    }

# ---------------------------------------------------------------------------
# Per-engine centrist/extreme scoring
# ---------------------------------------------------------------------------

def engine_centrist_score(dna: dict) -> float:
    """
    Mean absolute deviation from 0.5 across all 6 dims.
    0.0 = perfectly centrist, 0.5 = maximally extreme.
    """
    return statistics.mean(abs(dna[d] - 0.5) for d in DNA_DIMS)


def analyse_engines(presets: list[dict]) -> list[dict]:
    by_engine: dict[str, list[dict]] = defaultdict(list)
    for p in presets:
        engines = p["engines"] if p["engines"] else ["Unknown"]
        for e in engines:
            by_engine[e].append(p)

    results = []
    for engine, eps in sorted(by_engine.items()):
        # Mean DNA across all presets for this engine
        mean_dna = {}
        for dim in DNA_DIMS:
            mean_dna[dim] = statistics.mean(p["dna"][dim] for p in eps)
        score = engine_centrist_score(mean_dna)
        results.append({
            "engine": engine,
            "count": len(eps),
            "mean_dna": mean_dna,
            "centrist_score": score,  # low = centrist, high = extreme
        })

    results.sort(key=lambda x: x["centrist_score"])
    return results

# ---------------------------------------------------------------------------
# Correlation analysis
# ---------------------------------------------------------------------------

def analyse_correlations(presets: list[dict]) -> list[dict]:
    flagged = []
    for i, d1 in enumerate(DNA_DIMS):
        for j, d2 in enumerate(DNA_DIMS):
            if j <= i:
                continue
            xs = [p["dna"][d1] for p in presets]
            ys = [p["dna"][d2] for p in presets]
            r = pearson_r(xs, ys)
            if abs(r) >= CORRELATION_THRESHOLD:
                flagged.append({"dim1": d1, "dim2": d2, "r": r})
    flagged.sort(key=lambda x: abs(x["r"]), reverse=True)
    return flagged

# ---------------------------------------------------------------------------
# Prescriptions
# ---------------------------------------------------------------------------

def build_prescriptions(
    dim_analyses: dict[str, dict],
    n_fleet: int,
    target_diversity: float,
    current_diversity: float,
    presets: list[dict],
) -> list[dict]:
    """
    For each compressed dimension, prescribe N high + N low presets
    needed to push the midrange% below COMPRESSION_THRESHOLD.
    Also simulate diversity improvement.
    """
    prescriptions = []

    for dim, da in dim_analyses.items():
        if not da["compressed"]:
            continue

        # How many presets needed outside midrange to hit <60% midrange?
        # Let M = midrange_count, T = total. We want (M / (T + X)) < threshold
        # => M < threshold * (T + X)
        # => X > M/threshold - T
        M = int(da["midrange_pct"] * n_fleet)
        needed_outside = math.ceil(M / COMPRESSION_THRESHOLD - n_fleet)
        needed_outside = max(needed_outside, 10)  # at least 10

        low_share = 0.5
        high_share = 0.5
        # Bias toward whichever side is more empty
        if da["low_count"] < da["high_count"]:
            low_share = 0.65
            high_share = 0.35
        elif da["high_count"] < da["low_count"]:
            low_share = 0.35
            high_share = 0.65

        n_low = math.ceil(needed_outside * low_share)
        n_high = math.ceil(needed_outside * high_share)

        # Simulate diversity if we added synthetic extreme presets.
        # Spread other dimensions uniformly so synthetic fills don't cluster.
        import random as _rnd
        synthetic = []
        for _ in range(n_low):
            dna = {d: _rnd.random() for d in DNA_DIMS}
            dna[dim] = 0.15
            synthetic.append({"dna": dna})
        for _ in range(n_high):
            dna = {d: _rnd.random() for d in DNA_DIMS}
            dna[dim] = 0.85
            synthetic.append({"dna": dna})

        projected = simulate_diversity_with_fills(presets, synthetic)

        prescriptions.append({
            "dim": dim,
            "midrange_pct": da["midrange_pct"],
            "needed_outside": needed_outside,
            "n_low": n_low,
            "n_high": n_high,
            "low_target": "<0.2",
            "high_target": ">0.8",
            "projected_diversity": projected,
            "diversity_gain": projected - current_diversity,
        })

    return prescriptions


def prescriptions_combined_diversity(
    presets: list[dict],
    prescriptions: list[dict],
    target_diversity: float,
) -> float:
    """Build all prescription fills together and compute combined diversity."""
    import random as _rnd
    all_fills = []
    for rx in prescriptions:
        for _ in range(rx["n_low"]):
            dna = {d: _rnd.random() for d in DNA_DIMS}
            dna[rx["dim"]] = 0.15
            all_fills.append({"dna": dna})
        for _ in range(rx["n_high"]):
            dna = {d: _rnd.random() for d in DNA_DIMS}
            dna[rx["dim"]] = 0.85
            all_fills.append({"dna": dna})
    return simulate_diversity_with_fills(presets, all_fills), len(all_fills)

# ---------------------------------------------------------------------------
# Printing
# ---------------------------------------------------------------------------

def print_section(title: str) -> None:
    width = 72
    print()
    print("=" * width)
    print(f"  {title}")
    print("=" * width)


def print_dim_analysis(da: dict, n_fleet: int) -> None:
    dim = da["dim"].upper()
    compressed_tag = " [COMPRESSED]" if da["compressed"] else ""
    print(f"\n  {dim}{compressed_tag}")
    print(f"    mean={da['mean']:.3f}  std={da['stdev']:.3f}  "
          f"min={da['min']:.3f}  max={da['max']:.3f}")
    print(f"    midrange ({MIDRANGE_LOW}–{MIDRANGE_HIGH}): "
          f"{da['midrange_pct']*100:.1f}% of fleet  "
          f"low(<{MIDRANGE_LOW}): {da['low_count']}  high(>{MIDRANGE_HIGH}): {da['high_count']}")
    print()
    max_count = max(b[2] for b in da["hist"])
    for (lo, hi, count) in da["hist"]:
        bar = ascii_bar(count, max_count)
        pct = count / n_fleet * 100 if n_fleet else 0
        print(f"    [{lo:.1f}–{hi:.1f}]  {bar}  {count:4d} ({pct:4.1f}%)")

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main() -> None:
    parser = argparse.ArgumentParser(
        description="Deep DNA diversity analysis of the XOmnibus preset fleet."
    )
    parser.add_argument(
        "--presets-dir",
        type=pathlib.Path,
        default=pathlib.Path(__file__).parent.parent / "Presets" / "XOmnibus",
        help="Root directory containing .xometa preset files.",
    )
    parser.add_argument(
        "--engine",
        type=str,
        default=None,
        help="Filter analysis to a single engine (case-insensitive substring match).",
    )
    parser.add_argument(
        "--prescribe",
        action="store_true",
        help="Show fill prescriptions for compressed dimensions.",
    )
    parser.add_argument(
        "--target-diversity",
        type=float,
        default=0.35,
        help="Target fleet diversity score (default: 0.35).",
    )
    args = parser.parse_args()

    print_section("XOmnibus Fleet DNA Diversity Report")
    print(f"  Presets dir : {args.presets_dir}")
    if args.engine:
        print(f"  Engine filter: {args.engine}")
    print(f"  Target diversity: {args.target_diversity:.3f}")

    # Load
    presets = load_presets(args.presets_dir, args.engine)
    n = len(presets)
    if n == 0:
        print("\n  ERROR: No .xometa presets with valid dna found. Check --presets-dir.")
        raise SystemExit(0)

    print(f"\n  Loaded {n} presets with 6D sonic_dna")

    # Fleet diversity
    print("\n  Computing fleet diversity score (this may take a moment for large fleets)...")
    current_diversity = fleet_diversity_score(presets)
    print(f"\n  Current fleet diversity score : {current_diversity:.4f}")
    print(f"  Target fleet diversity score  : {args.target_diversity:.4f}")
    gap = args.target_diversity - current_diversity
    if gap > 0:
        print(f"  Gap to target                 : +{gap:.4f}  (fleet needs more spread)")
    else:
        print(f"  Gap to target                 : {gap:.4f}  (fleet already meets target)")

    # Per-dimension analysis
    print_section("Per-Dimension Analysis")
    dim_analyses = {}
    for dim in DNA_DIMS:
        values = [p["dna"][dim] for p in presets]
        da = analyse_dimension(dim, values)
        dim_analyses[dim] = da
        print_dim_analysis(da, n)

    # Compression zones summary
    compressed_dims = [d for d, da in dim_analyses.items() if da["compressed"]]
    print_section("Compression Zones Summary")
    if compressed_dims:
        print(f"  ALERT: {len(compressed_dims)} dimension(s) have ≥{COMPRESSION_THRESHOLD*100:.0f}% of presets in midrange band.")
        for dim in compressed_dims:
            da = dim_analyses[dim]
            print(f"    • {dim.upper():12s}  {da['midrange_pct']*100:.1f}% in [{MIDRANGE_LOW}–{MIDRANGE_HIGH}]  "
                  f"(low={da['low_count']}, mid={int(da['midrange_pct']*n)}, high={da['high_count']})")
    else:
        print(f"  No compression zones detected (no dimension has ≥{COMPRESSION_THRESHOLD*100:.0f}% midrange crowding).")

    # Per-engine analysis
    print_section("Per-Engine DNA Profile (Centrist vs Extreme)")
    engine_analyses = analyse_engines(presets)
    print(f"  {'Engine':<22} {'Presets':>7}  {'Centrist Score':>14}  {'Profile':<40}")
    print(f"  {'-'*22} {'-'*7}  {'-'*14}  {'-'*40}")
    for ea in engine_analyses:
        bar_val = ea["centrist_score"] / 0.5  # normalise 0–1
        bar = ascii_bar(int(bar_val * 20), 20, 20)
        profile_parts = []
        for dim in DNA_DIMS:
            v = ea["mean_dna"][dim]
            if v < 0.25:
                profile_parts.append(f"{dim[:3]}↓")
            elif v > 0.75:
                profile_parts.append(f"{dim[:3]}↑")
        profile = " ".join(profile_parts) if profile_parts else "centrist"
        label = "CENTRIST" if ea["centrist_score"] < 0.12 else ("EXTREME" if ea["centrist_score"] > 0.22 else "moderate")
        print(f"  {ea['engine']:<22} {ea['count']:>7}  {ea['centrist_score']:>14.4f}  {label:<10} {profile}")

    most_centrist = engine_analyses[:3]
    most_extreme = engine_analyses[-3:]
    print(f"\n  Most centrist engines : {', '.join(e['engine'] for e in most_centrist)}")
    print(f"  Most extreme engines  : {', '.join(e['engine'] for e in reversed(most_extreme))}")

    # Cross-dimension correlations
    print_section("Cross-Dimension Correlation Analysis")
    correlations = analyse_correlations(presets)
    if correlations:
        print(f"  Flagged pairs with |r| > {CORRELATION_THRESHOLD} (unhealthy co-movement):")
        for c in correlations:
            direction = "positive" if c["r"] > 0 else "negative"
            severity = "CRITICAL" if abs(c["r"]) > 0.85 else "WARNING"
            print(f"    [{severity}] {c['dim1'].upper()} ↔ {c['dim2'].upper()}  "
                  f"r={c['r']:+.4f}  ({direction} correlation)")
        print()
        print("  High correlation means these dimensions effectively reduce to fewer than 6")
        print("  independent axes, suppressing true diversity despite different dim values.")
    else:
        print("  No unhealthy correlations found (all |r| < 0.70). Good dimensional independence.")

    all_pairs = []
    for i, d1 in enumerate(DNA_DIMS):
        for j, d2 in enumerate(DNA_DIMS):
            if j <= i:
                continue
            xs = [p["dna"][d1] for p in presets]
            ys = [p["dna"][d2] for p in presets]
            r = pearson_r(xs, ys)
            all_pairs.append((d1, d2, r))
    print(f"\n  Full correlation matrix:")
    header = f"  {'':12}" + "".join(f"{d[:6]:>8}" for d in DNA_DIMS)
    print(header)
    for d1 in DNA_DIMS:
        row = f"  {d1:<12}"
        for d2 in DNA_DIMS:
            if d1 == d2:
                row += f"{'1.000':>8}"
            else:
                pair = next((p for p in all_pairs if (p[0] == d1 and p[1] == d2) or (p[0] == d2 and p[1] == d1)), None)
                r_val = pair[2] if pair else 0.0
                flag = "*" if abs(r_val) >= CORRELATION_THRESHOLD else " "
                row += f"{r_val:>+7.3f}{flag}"
        print(row)
    print(f"  (* = |r| ≥ {CORRELATION_THRESHOLD})")

    # Prescriptions
    if args.prescribe:
        print_section("Fill Prescriptions")
        prescriptions = build_prescriptions(
            dim_analyses, n, args.target_diversity, current_diversity, presets
        )
        if not prescriptions:
            print("  No compressed dimensions detected — no prescriptions needed.")
        else:
            total_fills = 0
            for rx in prescriptions:
                total_fills += rx["n_low"] + rx["n_high"]
                print(f"\n  DIMENSION: {rx['dim'].upper()}")
                print(f"    Current midrange coverage : {rx['midrange_pct']*100:.1f}%  (target: <{COMPRESSION_THRESHOLD*100:.0f}%)")
                print(f"    Prescription:")
                print(f"      Add {rx['n_low']:3d} presets with {rx['dim']} {rx['low_target']}  (low extreme)")
                print(f"      Add {rx['n_high']:3d} presets with {rx['dim']} {rx['high_target']}  (high extreme)")
                print(f"      Total new presets for this dim : {rx['n_low'] + rx['n_high']}")
                print(f"    Projected diversity (this dim only) : {rx['projected_diversity']:.4f}  "
                      f"(+{rx['diversity_gain']:.4f})")
                print(f"    Example prompts:")
                if rx['n_low'] > 0:
                    print(f"      \"Need {rx['n_low']} low-{rx['dim']} ({rx['low_target']}) presets to normalise distribution\"")
                if rx['n_high'] > 0:
                    print(f"      \"Need {rx['n_high']} high-{rx['dim']} ({rx['high_target']}) presets to normalise distribution\"")

            # Combined projection
            combined_diversity, total_fill_count = prescriptions_combined_diversity(
                presets, prescriptions, args.target_diversity
            )
            print_section("Diversity Improvement Projection")
            print(f"  Current fleet diversity  : {current_diversity:.4f}")
            print(f"  Target diversity         : {args.target_diversity:.4f}")
            print(f"  Total prescription fills : {total_fill_count} presets across {len(prescriptions)} dimension(s)")
            print(f"  Projected diversity      : {combined_diversity:.4f}  "
                  f"(+{combined_diversity - current_diversity:.4f})")

            if combined_diversity >= args.target_diversity:
                print(f"\n  RESULT: Applying all prescriptions would MEET the {args.target_diversity:.3f} target.")
            else:
                remaining = args.target_diversity - combined_diversity
                scale = remaining / max(combined_diversity - current_diversity, 0.001)
                extra = math.ceil(total_fill_count * scale)
                print(f"\n  RESULT: Applying all prescriptions still falls short of target.")
                print(f"          Need approximately {extra} additional extreme-positioned presets")
                print(f"          spread across all 6 dimensions to close the gap.")

            print(f"\n  SUMMARY TABLE:")
            print(f"  {'Dimension':<14} {'Fills Needed':>12}  {'Projected Score':>15}  {'Gain':>6}")
            print(f"  {'-'*14} {'-'*12}  {'-'*15}  {'-'*6}")
            for rx in sorted(prescriptions, key=lambda x: -x["diversity_gain"]):
                fills = rx["n_low"] + rx["n_high"]
                print(f"  {rx['dim']:<14} {fills:>12}  {rx['projected_diversity']:>15.4f}  {rx['diversity_gain']:>+6.4f}")
            print(f"  {'COMBINED':<14} {total_fill_count:>12}  {combined_diversity:>15.4f}  "
                  f"{combined_diversity - current_diversity:>+6.4f}")

    # Closing summary
    print_section("Diagnosis Summary")
    print(f"  Fleet size             : {n} presets")
    print(f"  Fleet diversity score  : {current_diversity:.4f}  (higher = more diverse)")
    print(f"  Compressed dimensions  : {len(compressed_dims)} / 6  → {', '.join(d.upper() for d in compressed_dims) if compressed_dims else 'none'}")
    print(f"  Correlated pairs       : {len(correlations)} flagged")

    if current_diversity < 0.20:
        diagnosis = "CRITICAL — fleet is highly clustered. Major preset diversification required."
    elif current_diversity < 0.28:
        diagnosis = "POOR — significant midrange clustering suppresses expressiveness."
    elif current_diversity < 0.35:
        diagnosis = "FAIR — some spread but extremes are under-represented."
    elif current_diversity < 0.45:
        diagnosis = "GOOD — reasonable diversity with room for polar expansion."
    else:
        diagnosis = "EXCELLENT — fleet covers the DNA space well."

    print(f"\n  VERDICT: {diagnosis}")

    if compressed_dims:
        print(f"\n  PRIMARY CAUSE: {len(compressed_dims)} dimension(s) have excessive midrange clustering.")
        print(f"  RECOMMENDATION: Run with --prescribe to get specific fill targets.")

    if correlations:
        print(f"\n  SECONDARY CAUSE: {len(correlations)} dimension pair(s) move together,")
        print(f"  reducing effective dimensionality. Decouple these when designing new presets.")

    print()


if __name__ == "__main__":
    main()
