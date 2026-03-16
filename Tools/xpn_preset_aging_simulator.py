"""
xpn_preset_aging_simulator.py — XO_OX Sonic DNA Aging Simulator

Simulates how a preset collection's sonic DNA "ages" as presets are added
chronologically (sorted by filename). Tracks the 6D Sonic DNA centroid over
time, detects drift events, and classifies them on the feliX-Oscar axis.

feliX axis: bright / clinical / high-movement
Oscar axis: warm / organic / dense

Usage:
    python xpn_preset_aging_simulator.py --preset-dir <dir> [--threshold 0.15] [--output report.txt]

Requirements: Python stdlib only (json, os, argparse, math, pathlib)
"""

import argparse
import json
import math
import os
import sys
from pathlib import Path
from typing import Dict, List, Optional, Tuple

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

DIMS = ["brightness", "warmth", "movement", "density", "space", "aggression"]

# feliX score: high brightness, low warmth, high movement → clinical/bright
# Oscar score: high warmth, low brightness, high density → warm/organic
FELIX_DIMS_POS = ["brightness", "movement"]
FELIX_DIMS_NEG = ["warmth"]
OSCAR_DIMS_POS = ["warmth", "density"]
OSCAR_DIMS_NEG = ["brightness"]

DEFAULT_THRESHOLD = 0.15

CHART_WIDTH = 60   # characters for the ASCII plot x-axis
CHART_HEIGHT = 12  # rows for the ASCII plot y-axis


# ---------------------------------------------------------------------------
# DNA loading
# ---------------------------------------------------------------------------

def extract_dna(data: dict) -> Optional[Dict[str, float]]:
    """Return normalised 6D DNA dict from a parsed .xometa dict, or None."""
    raw = data.get("sonic_dna") or data.get("dna") or data.get("sonicDNA") or {}
    if not raw:
        return None
    result = {}
    for dim in DIMS:
        val = raw.get(dim)
        if val is None:
            return None  # incomplete DNA — skip preset
        try:
            result[dim] = float(val)
        except (TypeError, ValueError):
            return None
    return result


def load_presets(preset_dir: Path) -> Tuple[List[Tuple[str, dict]], int]:
    """
    Load all .xometa files in preset_dir (sorted by filename).
    Returns list of (filename, dna_dict) for presets with valid 6D DNA.
    """
    files = sorted(preset_dir.glob("*.xometa"))
    if not files:
        # Also try .json files that may contain xometa data
        files = sorted(preset_dir.glob("*.json"))

    loaded = []
    skipped = 0
    for f in files:
        try:
            with open(f, "r", encoding="utf-8") as fh:
                data = json.load(fh)
        except (json.JSONDecodeError, OSError):
            skipped += 1
            continue
        dna = extract_dna(data)
        if dna is None:
            skipped += 1
            continue
        loaded.append((f.name, dna))

    return loaded, skipped


# ---------------------------------------------------------------------------
# Math helpers
# ---------------------------------------------------------------------------

def centroid(dnas: list[dict]) -> dict:
    """Compute per-dimension mean across all DNA dicts."""
    n = len(dnas)
    return {dim: sum(d[dim] for d in dnas) / n for dim in DIMS}


def euclidean_distance(a: dict, b: dict) -> float:
    return math.sqrt(sum((a[dim] - b[dim]) ** 2 for dim in DIMS))


def felix_score(dna: dict) -> float:
    pos = sum(dna[d] for d in FELIX_DIMS_POS)
    neg = sum(1.0 - dna[d] for d in FELIX_DIMS_NEG)
    return (pos + neg) / (len(FELIX_DIMS_POS) + len(FELIX_DIMS_NEG))


def oscar_score(dna: dict) -> float:
    pos = sum(dna[d] for d in OSCAR_DIMS_POS)
    neg = sum(1.0 - dna[d] for d in OSCAR_DIMS_NEG)
    return (pos + neg) / (len(OSCAR_DIMS_POS) + len(OSCAR_DIMS_NEG))


def axis_label(old_centroid: dict, new_centroid: dict) -> str:
    """Classify a drift as feliX or Oscar based on which axis shifted more."""
    df = felix_score(new_centroid) - felix_score(old_centroid)
    do = oscar_score(new_centroid) - oscar_score(old_centroid)
    if df >= do:
        return "feliX-drift"
    return "Oscar-drift"


def dim_delta(old_c: dict, new_c: dict) -> dict:
    return {dim: new_c[dim] - old_c[dim] for dim in DIMS}


# ---------------------------------------------------------------------------
# Recommendations
# ---------------------------------------------------------------------------

def recommend_additions(final_centroid: dict, felix_bal: float, oscar_bal: float) -> list[str]:
    """
    Suggest DNA adjustments to rebalance toward 0.5 on both feliX and Oscar axes.
    Returns a list of human-readable suggestion strings.
    """
    suggestions = []
    target = 0.50

    if felix_bal > 0.60:
        suggestions.append(
            "Collection leans feliX (bright/clinical). "
            "Add presets with high warmth, low brightness, high density to rebalance toward Oscar."
        )
    elif oscar_bal > 0.60:
        suggestions.append(
            "Collection leans Oscar (warm/organic). "
            "Add presets with high brightness, high movement, low warmth to rebalance toward feliX."
        )
    else:
        suggestions.append("feliX-Oscar balance is healthy (both scores within 0.60).")

    # Dimension-specific gaps
    for dim in DIMS:
        val = final_centroid[dim]
        if val < 0.30:
            suggestions.append(
                f"Low {dim} ({val:.2f}) — consider adding presets with higher {dim}."
            )
        elif val > 0.75:
            suggestions.append(
                f"High {dim} ({val:.2f}) — collection may benefit from presets with lower {dim}."
            )

    if not suggestions:
        suggestions.append("Collection DNA is well-balanced across all dimensions.")

    return suggestions


# ---------------------------------------------------------------------------
# ASCII chart
# ---------------------------------------------------------------------------

def _scale_val(val: float, lo: float, hi: float, width: int) -> int:
    if hi == lo:
        return width // 2
    return max(0, min(width - 1, int((val - lo) / (hi - lo) * (width - 1))))


def ascii_chart(
    series_felix: list[float],
    series_oscar: list[float],
    title: str = "feliX / Oscar Centroid Trajectory",
) -> str:
    """
    Render a simple dual-line ASCII chart.
    X-axis = preset index (time), Y-axis = score (0.0–1.0).
    'F' = feliX score, 'O' = Oscar score, '*' = overlap.
    """
    n = len(series_felix)
    if n == 0:
        return "(no data)"

    lo, hi = 0.0, 1.0
    height = CHART_HEIGHT
    width = min(CHART_WIDTH, n)

    # Downsample to chart width if necessary
    def sample(series: list[float], w: int) -> list[float]:
        if len(series) <= w:
            return series
        step = len(series) / w
        return [series[int(i * step)] for i in range(w)]

    sf = sample(series_felix, width)
    so = sample(series_oscar, width)

    # Build grid: rows = height, cols = width
    grid = [[" "] * width for _ in range(height)]

    for col, (fv, ov) in enumerate(zip(sf, so)):
        fr = height - 1 - int(fv * (height - 1))
        orw = height - 1 - int(ov * (height - 1))
        fr = max(0, min(height - 1, fr))
        orw = max(0, min(height - 1, orw))
        if fr == orw:
            grid[fr][col] = "*"
        else:
            grid[fr][col] = "F"
            grid[orw][col] = "O"

    lines = [f"  {title}"]
    lines.append(f"  {'─' * (width + 4)}")
    for r, row in enumerate(grid):
        y_val = 1.0 - r / (height - 1)
        lines.append(f"  {y_val:.1f} │{''.join(row)}│")
    lines.append(f"       └{'─' * width}┘")
    lines.append(f"        preset 1{' ' * (width - 16)}preset {n}")
    lines.append("")
    lines.append("  F = feliX score   O = Oscar score   * = overlap")
    return "\n".join(lines)


def ascii_drift_bar(drift_events: list[dict], total: int) -> str:
    """Render a compact timeline showing where drift events occurred."""
    if not drift_events or total == 0:
        return "  (no drift events)"
    width = min(CHART_WIDTH, total)
    bar = ["."] * width
    for ev in drift_events:
        idx = int(ev["after_index"] / total * (width - 1))
        marker = "F" if ev["type"] == "feliX-drift" else "O"
        bar[idx] = marker
    return "  [" + "".join(bar) + "]"


# ---------------------------------------------------------------------------
# Core simulation
# ---------------------------------------------------------------------------

def simulate(presets: list[tuple[str, dict]], threshold: float) -> dict:
    """
    Step through presets chronologically, computing centroid at each step.
    Returns full simulation result dict.
    """
    centroids = []
    felix_scores = []
    oscar_scores = []
    drift_events = []
    total_drift = 0.0

    accumulated = []
    prev_centroid = None

    for i, (fname, dna) in enumerate(presets):
        accumulated.append(dna)
        c = centroid(accumulated)
        centroids.append(c)
        felix_scores.append(felix_score(c))
        oscar_scores.append(oscar_score(c))

        if prev_centroid is not None:
            dist = euclidean_distance(prev_centroid, c)
            total_drift += dist
            if dist >= threshold:
                drift_type = axis_label(prev_centroid, c)
                drift_events.append({
                    "after_index": i,
                    "filename": fname,
                    "distance": dist,
                    "type": drift_type,
                    "delta": dim_delta(prev_centroid, c),
                    "centroid_after": dict(c),
                })

        prev_centroid = c

    final_c = centroids[-1] if centroids else {d: 0.5 for d in DIMS}
    final_felix = felix_scores[-1] if felix_scores else 0.5
    final_oscar = oscar_scores[-1] if oscar_scores else 0.5

    felix_drift_count = sum(1 for e in drift_events if e["type"] == "feliX-drift")
    oscar_drift_count = sum(1 for e in drift_events if e["type"] == "Oscar-drift")

    return {
        "centroids": centroids,
        "felix_scores": felix_scores,
        "oscar_scores": oscar_scores,
        "drift_events": drift_events,
        "total_drift": total_drift,
        "final_centroid": final_c,
        "final_felix": final_felix,
        "final_oscar": final_oscar,
        "felix_drift_count": felix_drift_count,
        "oscar_drift_count": oscar_drift_count,
    }


# ---------------------------------------------------------------------------
# Report builder
# ---------------------------------------------------------------------------

def build_report(
    presets: list[tuple[str, dict]],
    result: dict,
    skipped: int,
    threshold: float,
    preset_dir: Path,
) -> str:
    n = len(presets)
    lines = []

    lines.append("=" * 66)
    lines.append("  XO_OX PRESET AGING SIMULATOR — Sonic DNA Trajectory Report")
    lines.append("=" * 66)
    lines.append(f"  Directory : {preset_dir}")
    lines.append(f"  Presets   : {n} loaded, {skipped} skipped (missing/invalid DNA)")
    lines.append(f"  Threshold : {threshold}")
    lines.append("")

    if n == 0:
        lines.append("  No presets with valid 6D Sonic DNA found. Nothing to simulate.")
        return "\n".join(lines)

    # --- centroid trajectory chart ---
    lines.append("─" * 66)
    lines.append("  CENTROID TRAJECTORY")
    lines.append("─" * 66)
    lines.append(ascii_chart(result["felix_scores"], result["oscar_scores"]))
    lines.append("")

    # --- drift timeline ---
    lines.append("─" * 66)
    lines.append("  DRIFT EVENT TIMELINE  (F=feliX, O=Oscar, .=stable)")
    lines.append("─" * 66)
    lines.append(ascii_drift_bar(result["drift_events"], n))
    lines.append("")

    # --- drift events detail ---
    drift_events = result["drift_events"]
    lines.append("─" * 66)
    lines.append(f"  DRIFT EVENTS  (threshold ≥ {threshold})  — {len(drift_events)} total")
    lines.append("─" * 66)
    if not drift_events:
        lines.append("  No drift events detected.")
    else:
        for ev in drift_events:
            lines.append(
                f"  [{ev['after_index']:>3}] {ev['filename']:<40}  "
                f"{ev['type']:<14}  dist={ev['distance']:.4f}"
            )
            delta = ev["delta"]
            dim_parts = []
            for dim in DIMS:
                d = delta[dim]
                if abs(d) >= 0.005:
                    sign = "+" if d >= 0 else ""
                    dim_parts.append(f"{dim[:3]}:{sign}{d:.3f}")
            if dim_parts:
                lines.append(f"        Δ  {', '.join(dim_parts)}")
    lines.append("")

    # --- summary stats ---
    fc = result["final_centroid"]
    lines.append("─" * 66)
    lines.append("  FINAL CENTROID")
    lines.append("─" * 66)
    for dim in DIMS:
        bar_len = int(fc[dim] * 30)
        bar = "█" * bar_len + "░" * (30 - bar_len)
        lines.append(f"  {dim:<12} {fc[dim]:.3f}  [{bar}]")
    lines.append("")

    lines.append("─" * 66)
    lines.append("  SUMMARY")
    lines.append("─" * 66)
    lines.append(f"  Total drift distance  : {result['total_drift']:.4f}")
    lines.append(f"  Drift events          : {len(drift_events)}")
    lines.append(f"    feliX-drift events  : {result['felix_drift_count']}")
    lines.append(f"    Oscar-drift events  : {result['oscar_drift_count']}")
    lines.append(f"  Final feliX score     : {result['final_felix']:.3f}")
    lines.append(f"  Final Oscar score     : {result['final_oscar']:.3f}")

    # Balance score: 1.0 = perfect balance, 0.0 = fully skewed
    balance = 1.0 - abs(result["final_felix"] - result["final_oscar"])
    lines.append(f"  feliX-Oscar balance   : {balance:.3f}  (1.0 = perfect)")
    lines.append("")

    # --- recommendations ---
    lines.append("─" * 66)
    lines.append("  RECOMMENDATIONS")
    lines.append("─" * 66)
    recs = recommend_additions(fc, result["final_felix"], result["final_oscar"])
    for rec in recs:
        # Word-wrap at ~62 chars
        words = rec.split()
        current = "  • "
        for word in words:
            if len(current) + len(word) + 1 > 64:
                lines.append(current)
                current = "    " + word + " "
            else:
                current += word + " "
        lines.append(current.rstrip())
    lines.append("")
    lines.append("=" * 66)

    return "\n".join(lines)


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def parse_args(argv=None):
    parser = argparse.ArgumentParser(
        description="XO_OX Preset Aging Simulator — track Sonic DNA centroid drift over time.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python xpn_preset_aging_simulator.py --preset-dir ./Presets/OPAL
  python xpn_preset_aging_simulator.py --preset-dir ./Presets/ONSET --threshold 0.10 --output aging_report.txt
        """,
    )
    parser.add_argument(
        "--preset-dir",
        required=True,
        metavar="DIR",
        help="Directory containing .xometa preset files",
    )
    parser.add_argument(
        "--threshold",
        type=float,
        default=DEFAULT_THRESHOLD,
        metavar="FLOAT",
        help=f"Centroid shift distance to trigger a drift event (default: {DEFAULT_THRESHOLD})",
    )
    parser.add_argument(
        "--output",
        metavar="FILE",
        help="Write report to this file in addition to stdout",
    )
    return parser.parse_args(argv)


def main(argv=None):
    args = parse_args(argv)

    preset_dir = Path(args.preset_dir)
    if not preset_dir.is_dir():
        print(f"ERROR: --preset-dir '{preset_dir}' is not a directory.", file=sys.stderr)
        sys.exit(1)

    threshold = args.threshold
    if threshold <= 0:
        print("ERROR: --threshold must be > 0.", file=sys.stderr)
        sys.exit(1)

    presets, skipped = load_presets(preset_dir)

    if len(presets) == 0:
        print(f"No .xometa files with valid 6D Sonic DNA found in '{preset_dir}'.")
        print(f"Skipped: {skipped} files (missing, unreadable, or incomplete DNA).")
        sys.exit(0)

    result = simulate(presets, threshold)
    report = build_report(presets, result, skipped, threshold, preset_dir)

    print(report)

    if args.output:
        out_path = Path(args.output)
        try:
            out_path.write_text(report, encoding="utf-8")
            print(f"\nReport written to: {out_path}")
        except OSError as e:
            print(f"WARNING: Could not write output file: {e}", file=sys.stderr)


if __name__ == "__main__":
    main()
