#!/usr/bin/env python3
"""
xpn_preset_rebalancer.py — XO_OX Preset Collection Rebalancer

Analyzes a directory of .xometa presets, computes the current 6D Sonic DNA
centroid, and recommends additions/removals to shift the collection toward a
target centroid (feliX = bright/clinical, Oscar = warm/organic, or custom).

Usage:
    python xpn_preset_rebalancer.py --preset-dir <dir> [options]

Options:
    --target-felix          Shift toward feliX pole (bright, clinical)
    --target-oscar          Shift toward Oscar pole (warm, organic)
    --target-balanced       Target all dimensions at 0.5
    --target-dna brightness:0.6,warmth:0.4,...  Custom target centroid
    --simulate-add N        Simulate adding N synthetic presets at recommended DNA
    --output report.txt     Write report to file (default: stdout)
    --top-removals N        Number of removal candidates to show (default: 5)
"""

import argparse
import json
import math
import sys
from pathlib import Path

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

DNA_DIMS = ["brightness", "warmth", "movement", "density", "space", "aggression"]

# Canonical poles
FELIX_CENTROID = {
    "brightness": 0.8,
    "warmth": 0.2,
    "movement": 0.65,
    "density": 0.45,
    "space": 0.7,
    "aggression": 0.35,
}

OSCAR_CENTROID = {
    "brightness": 0.25,
    "warmth": 0.8,
    "movement": 0.4,
    "density": 0.6,
    "space": 0.45,
    "aggression": 0.2,
}

BALANCED_CENTROID = {d: 0.5 for d in DNA_DIMS}


# ---------------------------------------------------------------------------
# Loading
# ---------------------------------------------------------------------------

def load_presets(preset_dir: Path) -> list[dict]:
    """Recursively load all .xometa files from preset_dir."""
    presets = []
    for path in sorted(preset_dir.rglob("*.xometa")):
        try:
            data = json.loads(path.read_text(encoding="utf-8"))
            dna = data.get("dna")
            if not dna:
                continue
            # Validate all 6 dims present
            if not all(d in dna for d in DNA_DIMS):
                continue
            presets.append({
                "path": path,
                "name": data.get("name", path.stem),
                "engine": (data.get("engines") or [None])[0],
                "mood": data.get("mood", ""),
                "dna": {d: float(dna[d]) for d in DNA_DIMS},
            })
        except (json.JSONDecodeError, KeyError, TypeError, ValueError):
            pass
    return presets


# ---------------------------------------------------------------------------
# Math helpers
# ---------------------------------------------------------------------------

def centroid(presets: list[dict]) -> dict:
    if not presets:
        return {d: 0.5 for d in DNA_DIMS}
    return {
        d: sum(p["dna"][d] for p in presets) / len(presets)
        for d in DNA_DIMS
    }


def euclidean(a: dict, b: dict) -> float:
    return math.sqrt(sum((a[d] - b[d]) ** 2 for d in DNA_DIMS))


def dot_gap(preset_dna: dict, gap: dict) -> float:
    """
    Signed score: how much does this preset's deviation from 0.5 oppose the gap?
    Negative = opposes target direction (removal candidate).
    """
    score = 0.0
    for d in DNA_DIMS:
        deviation = preset_dna[d] - 0.5   # preset's own lean
        score += deviation * gap[d]        # positive if aligned, negative if opposed
    return score


def describe_dna(dna: dict) -> str:
    parts = []
    for d in DNA_DIMS:
        v = dna[d]
        if v >= 0.7:
            parts.append(f"{d}={v:.2f} (HIGH)")
        elif v <= 0.3:
            parts.append(f"{d}={v:.2f} (LOW)")
        else:
            parts.append(f"{d}={v:.2f}")
    return ", ".join(parts)


def polarity_label(dna: dict) -> str:
    """Return feliX/Oscar/Balanced based on brightness vs warmth."""
    b, w = dna["brightness"], dna["warmth"]
    diff = b - w
    if diff > 0.15:
        return "feliX-leaning"
    if diff < -0.15:
        return "Oscar-leaning"
    return "Balanced"


# ---------------------------------------------------------------------------
# Addition profile recommendation
# ---------------------------------------------------------------------------

def recommend_additions(current: dict, target: dict, n: int = 5) -> dict:
    """
    Return a recommended DNA profile for presets to add.
    We overshoot slightly to compensate for dilution.
    """
    overshoot = 1.4  # overshoot factor to compensate for collection size
    rec = {}
    for d in DNA_DIMS:
        gap = target[d] - current[d]
        # Aim past the target to actually move the needle
        desired = target[d] + gap * overshoot
        rec[d] = max(0.0, min(1.0, desired))
    return rec


def addition_constraints(rec_dna: dict, gap: dict) -> list[str]:
    """Human-readable constraints for presets to seek/create."""
    lines = []
    for d in DNA_DIMS:
        v = rec_dna[d]
        g = gap[d]
        if abs(g) < 0.03:
            continue  # dimension already on target
        direction = ">" if g > 0 else "<"
        threshold = min(max(v, 0.0), 1.0)
        lines.append(f"  {d} {direction} {threshold:.2f}  (gap: {g:+.3f})")
    return lines


# ---------------------------------------------------------------------------
# Simulation
# ---------------------------------------------------------------------------

def simulate_add(current: dict, n_add: int, add_dna: dict, collection_size: int) -> dict:
    """Compute new centroid after adding n_add synthetic presets at add_dna."""
    total = collection_size + n_add
    new_centroid = {}
    for d in DNA_DIMS:
        new_centroid[d] = (current[d] * collection_size + add_dna[d] * n_add) / total
    return new_centroid


# ---------------------------------------------------------------------------
# Per-engine breakdown
# ---------------------------------------------------------------------------

def engine_breakdown(presets: list[dict]) -> list[dict]:
    engines: dict[str, list] = {}
    for p in presets:
        eng = p["engine"] or "Unknown"
        engines.setdefault(eng, []).append(p)

    rows = []
    for eng, eng_presets in sorted(engines.items()):
        c = centroid(eng_presets)
        rows.append({
            "engine": eng,
            "count": len(eng_presets),
            "centroid": c,
            "polarity": polarity_label(c),
            "dist_felix": euclidean(c, FELIX_CENTROID),
            "dist_oscar": euclidean(c, OSCAR_CENTROID),
        })
    # Sort: most feliX first (smallest dist to felix pole)
    rows.sort(key=lambda r: r["dist_felix"])
    return rows


# ---------------------------------------------------------------------------
# Report generation
# ---------------------------------------------------------------------------

def fmt_centroid(c: dict) -> str:
    return "  " + "  ".join(f"{d}={c[d]:.3f}" for d in DNA_DIMS)


def build_report(
    presets: list[dict],
    target: dict,
    target_label: str,
    simulate_n: int,
    top_removals: int,
) -> list[str]:
    lines = []
    sep = "=" * 70

    lines.append(sep)
    lines.append("  XO_OX PRESET COLLECTION REBALANCER")
    lines.append(sep)
    lines.append(f"  Presets loaded : {len(presets)}")
    lines.append(f"  Target preset  : {target_label}")
    lines.append("")

    if not presets:
        lines.append("  ERROR: No valid .xometa presets found.")
        return lines

    # Current centroid
    current = centroid(presets)
    lines.append("CURRENT CENTROID")
    lines.append(fmt_centroid(current))
    lines.append(f"  Polarity: {polarity_label(current)}")
    lines.append(f"  Distance to feliX pole : {euclidean(current, FELIX_CENTROID):.4f}")
    lines.append(f"  Distance to Oscar pole : {euclidean(current, OSCAR_CENTROID):.4f}")
    lines.append("")

    # Target centroid
    lines.append("TARGET CENTROID")
    lines.append(fmt_centroid(target))
    lines.append("")

    # Gap
    gap = {d: target[d] - current[d] for d in DNA_DIMS}
    lines.append("CENTROID GAP  (target - current)")
    lines.append("  " + "  ".join(
        f"{d}={gap[d]:+.3f}" for d in DNA_DIMS
    ))
    total_gap = math.sqrt(sum(v ** 2 for v in gap.values()))
    lines.append(f"  Total gap magnitude: {total_gap:.4f}")
    lines.append("")

    # Removal candidates
    lines.append(f"REMOVAL CANDIDATES  (top {top_removals} presets opposing target)")
    lines.append("  These presets pull the centroid away from the target direction.")
    lines.append("")

    scored = sorted(presets, key=lambda p: dot_gap(p["dna"], gap))
    removal_candidates = scored[:top_removals]

    for i, p in enumerate(removal_candidates, 1):
        score = dot_gap(p["dna"], gap)
        lines.append(f"  {i}. {p['name']}")
        lines.append(f"     Engine : {p['engine'] or 'Unknown'}  |  Mood: {p['mood']}")
        lines.append(f"     DNA    : {describe_dna(p['dna'])}")
        lines.append(f"     Score  : {score:.4f}  (more negative = more opposed to target)")
        lines.append(f"     File   : {p['path']}")
        lines.append("")

    # Addition recommendations
    rec_dna = recommend_additions(current, target, n=simulate_n)
    constraints = addition_constraints(rec_dna, gap)

    lines.append("ADDITION RECOMMENDATIONS")
    lines.append("  Presets you should add/create to shift the centroid toward target.")
    lines.append("")
    lines.append("  Recommended DNA profile for new presets:")
    lines.append(fmt_centroid(rec_dna))
    lines.append(f"  Polarity of ideal addition: {polarity_label(rec_dna)}")
    lines.append("")
    if constraints:
        lines.append("  Per-dimension constraints:")
        lines += constraints
    else:
        lines.append("  Collection is already near target — no strong constraints.")
    lines.append("")

    # Simulation
    if simulate_n > 0:
        sim_centroid = simulate_add(current, simulate_n, rec_dna, len(presets))
        sim_gap_after = {d: target[d] - sim_centroid[d] for d in DNA_DIMS}
        sim_total_gap = math.sqrt(sum(v ** 2 for v in sim_gap_after.values()))
        lines.append(f"SIMULATION  (adding {simulate_n} synthetic presets at recommended DNA)")
        lines.append(f"  New collection size : {len(presets) + simulate_n}")
        lines.append("  Projected centroid  :")
        lines.append(fmt_centroid(sim_centroid))
        lines.append(f"  Projected polarity  : {polarity_label(sim_centroid)}")
        lines.append(f"  Remaining gap magnitude: {sim_total_gap:.4f}  "
                     f"(was {total_gap:.4f}, "
                     f"reduction: {(1 - sim_total_gap / total_gap) * 100:.1f}%)")
        lines.append("")

    # Per-engine breakdown
    lines.append("PER-ENGINE BREAKDOWN  (sorted most feliX → most Oscar)")
    lines.append("")
    rows = engine_breakdown(presets)
    for row in rows:
        c_str = "  ".join(f"{d}={row['centroid'][d]:.2f}" for d in DNA_DIMS)
        lines.append(f"  {row['engine']:20s}  n={row['count']:4d}  {row['polarity']:18s}")
        lines.append(f"    {c_str}")
        lines.append(f"    dist_feliX={row['dist_felix']:.3f}  dist_Oscar={row['dist_oscar']:.3f}")
        lines.append("")

    lines.append(sep)
    lines.append("  Report complete.")
    lines.append(sep)
    return lines


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def parse_custom_dna(raw: str) -> dict:
    """Parse 'brightness:0.6,warmth:0.4,...' into a dict."""
    result = dict(BALANCED_CENTROID)  # start from balanced defaults
    for token in raw.split(","):
        token = token.strip()
        if ":" not in token:
            continue
        key, val = token.split(":", 1)
        key = key.strip().lower()
        if key not in DNA_DIMS:
            raise ValueError(f"Unknown DNA dimension: '{key}'. Valid: {DNA_DIMS}")
        result[key] = float(val.strip())
    return result


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Rebalance an XO_OX preset collection toward a target DNA centroid.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument("--preset-dir", required=True,
                        help="Directory containing .xometa preset files (searched recursively)")
    parser.add_argument("--target-felix", action="store_true",
                        help="Target feliX pole: bright, clinical, spacious")
    parser.add_argument("--target-oscar", action="store_true",
                        help="Target Oscar pole: warm, organic, dense")
    parser.add_argument("--target-balanced", action="store_true",
                        help="Target balanced centroid (all 0.5)")
    parser.add_argument("--target-dna", metavar="DIM:VAL,...",
                        help="Custom target centroid, e.g. brightness:0.7,warmth:0.3")
    parser.add_argument("--simulate-add", type=int, default=10, metavar="N",
                        help="Simulate adding N presets at recommended DNA (default: 10)")
    parser.add_argument("--top-removals", type=int, default=5, metavar="N",
                        help="Number of removal candidates to show (default: 5)")
    parser.add_argument("--output", metavar="FILE",
                        help="Write report to file instead of stdout")
    args = parser.parse_args()

    # Resolve target
    target_count = sum([args.target_felix, args.target_oscar,
                        args.target_balanced, bool(args.target_dna)])
    if target_count == 0:
        print("ERROR: Specify one of --target-felix, --target-oscar, "
              "--target-balanced, or --target-dna", file=sys.stderr)
        sys.exit(1)
    if target_count > 1:
        print("ERROR: Specify only one target mode.", file=sys.stderr)
        sys.exit(1)

    if args.target_felix:
        target, target_label = FELIX_CENTROID, "feliX pole (bright/clinical)"
    elif args.target_oscar:
        target, target_label = OSCAR_CENTROID, "Oscar pole (warm/organic)"
    elif args.target_balanced:
        target, target_label = BALANCED_CENTROID, "Balanced (all 0.5)"
    else:
        try:
            target = parse_custom_dna(args.target_dna)
        except ValueError as e:
            print(f"ERROR: {e}", file=sys.stderr)
            sys.exit(1)
        target_label = f"Custom ({args.target_dna})"

    preset_dir = Path(args.preset_dir)
    if not preset_dir.is_dir():
        print(f"ERROR: --preset-dir '{preset_dir}' is not a valid directory.", file=sys.stderr)
        sys.exit(1)

    presets = load_presets(preset_dir)
    if not presets:
        print(f"WARNING: No valid .xometa presets found in '{preset_dir}'", file=sys.stderr)

    report_lines = build_report(
        presets=presets,
        target=target,
        target_label=target_label,
        simulate_n=args.simulate_add,
        top_removals=args.top_removals,
    )

    report_text = "\n".join(report_lines)

    if args.output:
        out_path = Path(args.output)
        out_path.write_text(report_text + "\n", encoding="utf-8")
        print(f"Report written to: {out_path}")
    else:
        print(report_text)


if __name__ == "__main__":
    main()
