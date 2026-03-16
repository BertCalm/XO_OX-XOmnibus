#!/usr/bin/env python3
"""
xpn_dna_gap_finder.py — XO_OX Sonic DNA Gap Finder

Finds underrepresented regions in 6D Sonic DNA space across a preset collection.
6D Sonic DNA: brightness, warmth, movement, density, space, aggression (0.0–1.0 each)

Usage:
    python xpn_dna_gap_finder.py --preset-dir <dir> [--top 20] [--engine FILTER] [--output gaps.txt]
"""

import argparse
import json
import math
import sys
from pathlib import Path
from collections import defaultdict

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

DNA_DIMS = ["brightness", "warmth", "movement", "density", "space", "aggression"]

# Zone boundary: (low: 0–0.33, mid: 0.34–0.66, high: 0.67–1.0)
ZONE_BOUNDS = [(0.0, 0.33), (0.34, 0.66), (0.67, 1.0)]
ZONE_LABELS = ["low", "mid", "high"]
NUM_ZONES = 3
TOTAL_CELLS = NUM_ZONES ** len(DNA_DIMS)  # 729

OVERCROWD_THRESHOLD = 20
MIN_COVERAGE = 2  # minimum presets per cell for "covered"

# DNA dimension → zone label → plain language description
DNA_LANGUAGE = {
    "brightness": {
        "high": "bright/cutting",
        "mid": "balanced",
        "low": "dark/muffled",
    },
    "warmth": {
        "high": "warm/organic",
        "mid": "neutral",
        "low": "cold/clinical",
    },
    "movement": {
        "high": "animated/evolving",
        "mid": "moderate",
        "low": "static/stable",
    },
    "density": {
        "high": "dense/layered",
        "mid": "medium",
        "low": "sparse/minimal",
    },
    "space": {
        "high": "expansive/reverberant",
        "mid": "medium-space",
        "low": "dry/intimate",
    },
    "aggression": {
        "high": "aggressive/harsh",
        "mid": "moderate",
        "low": "gentle/soft",
    },
}

# feliX–Oscar axis heuristic
def felix_oscar_label(brightness, warmth):
    """Return feliX, Oscar, or Balanced based on brightness/warmth combo."""
    if brightness >= 0.67 and warmth <= 0.33:
        return "feliX pole"
    elif brightness <= 0.33 and warmth >= 0.67:
        return "Oscar pole"
    elif brightness >= 0.55 and warmth <= 0.45:
        return "feliX-leaning"
    elif brightness <= 0.45 and warmth >= 0.55:
        return "Oscar-leaning"
    else:
        return "Balanced"


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def value_to_zone(value: float) -> int:
    """Map a 0.0–1.0 value to zone index 0 (low), 1 (mid), 2 (high)."""
    value = max(0.0, min(1.0, float(value)))
    if value <= 0.33:
        return 0
    elif value <= 0.66:
        return 1
    else:
        return 2


def cell_key(dna_values: dict) -> tuple:
    """Return a 6-tuple of zone indices for the given DNA dict."""
    return tuple(value_to_zone(dna_values.get(dim, 0.5)) for dim in DNA_DIMS)


def cell_description(key: tuple) -> str:
    """Translate a cell key (zone indices) into plain-language descriptor string."""
    parts = []
    for dim, zone_idx in zip(DNA_DIMS, key):
        label = ZONE_LABELS[zone_idx]
        parts.append(DNA_LANGUAGE[dim][label])
    return ", ".join(parts)


def cell_zone_labels(key: tuple) -> dict:
    """Return dict of dim → zone label string for a cell key."""
    return {dim: ZONE_LABELS[zone_idx] for dim, zone_idx in zip(DNA_DIMS, key)}


def cell_midpoint_values(key: tuple) -> dict:
    """Return approximate midpoint float values for each dimension zone."""
    midpoints = [0.165, 0.5, 0.835]
    return {dim: midpoints[zone_idx] for dim, zone_idx in zip(DNA_DIMS, key)}


def fill_prescription(key: tuple) -> str:
    """Generate a sound design prescription for filling an empty cell."""
    zones = cell_zone_labels(key)
    mid = cell_midpoint_values(key)
    brightness_val = mid["brightness"]
    warmth_val = mid["warmth"]
    axis = felix_oscar_label(brightness_val, warmth_val)

    lines = [f"  Axis: {axis}"]
    prescriptions = []

    # Movement → articulation advice
    if zones["movement"] == "high":
        prescriptions.append("use LFOs, step sequencers, or heavy modulation to keep it alive")
    elif zones["movement"] == "low":
        prescriptions.append("avoid mod sources — let it sit still and breathe slowly")

    # Density → layering advice
    if zones["density"] == "high":
        prescriptions.append("stack voices, add unison/spread, layer multiple textures")
    elif zones["density"] == "low":
        prescriptions.append("single voice or minimal layering — negative space is the point")

    # Space → reverb/delay advice
    if zones["space"] == "high":
        prescriptions.append("long reverb tail, hall or plate, wide stereo image")
    elif zones["space"] == "low":
        prescriptions.append("no reverb or very short room — dry and upfront")

    # Aggression → distortion/filter advice
    if zones["aggression"] == "high":
        prescriptions.append("push drive/saturation, raise filter resonance, add edge")
    elif zones["aggression"] == "low":
        prescriptions.append("keep it clean — gentle filter, no distortion")

    # Brightness + warmth → tone shaping
    if zones["brightness"] == "high" and zones["warmth"] == "low":
        prescriptions.append("high-pass the low end, bright timbre, digital or metallic character")
    elif zones["brightness"] == "low" and zones["warmth"] == "high":
        prescriptions.append("roll off highs, boost low-mids, earthy/wooden/felt character")
    elif zones["brightness"] == "low" and zones["warmth"] == "low":
        prescriptions.append("dark and clinical — sub-heavy or cold pad with no shimmer")

    lines.append("  Design: " + "; ".join(prescriptions) if prescriptions else "  Design: (no specific constraint)")
    return "\n".join(lines)


# ---------------------------------------------------------------------------
# Preset loading
# ---------------------------------------------------------------------------

def load_presets(preset_dir: Path, engine_filter: str = None) -> list[dict]:
    """
    Recursively find .xometa files in preset_dir, parse JSON, extract DNA.
    Returns list of dicts with keys: name, engine, dna (dict of 6 floats), path.
    """
    presets = []
    meta_files = list(preset_dir.rglob("*.xometa"))

    if not meta_files:
        print(f"[warn] No .xometa files found in {preset_dir}", file=sys.stderr)
        return presets

    skipped = 0
    for path in meta_files:
        try:
            data = json.loads(path.read_text(encoding="utf-8"))
        except Exception as e:
            skipped += 1
            print(f"[warn] Could not parse {path.name}: {e}", file=sys.stderr)
            continue

        engine = data.get("engine", data.get("engineId", "unknown"))

        # Apply engine filter
        if engine_filter and engine_filter.lower() not in engine.lower():
            continue

        # Extract DNA — support both flat and nested formats
        raw_dna = data.get("sonicDna", data.get("sonic_dna", data.get("dna", {})))

        dna = {}
        for dim in DNA_DIMS:
            val = raw_dna.get(dim, raw_dna.get(dim.capitalize(), None))
            if val is None:
                val = 0.5  # default to midpoint if missing
            dna[dim] = float(val)

        presets.append({
            "name": data.get("name", data.get("presetName", path.stem)),
            "engine": engine,
            "dna": dna,
            "path": str(path),
        })

    if skipped:
        print(f"[warn] Skipped {skipped} unparseable files.", file=sys.stderr)

    return presets


# ---------------------------------------------------------------------------
# Analysis
# ---------------------------------------------------------------------------

def analyze(presets: list[dict]) -> dict:
    """
    Bin presets into 729 cells, return analysis dict.
    """
    cell_counts = defaultdict(int)
    cell_preset_names = defaultdict(list)

    for p in presets:
        key = cell_key(p["dna"])
        cell_counts[key] += 1
        cell_preset_names[key].append(f"{p['name']} ({p['engine']})")

    # Build full 729-cell picture
    all_keys = set()
    for b in range(NUM_ZONES):
        for w in range(NUM_ZONES):
            for m in range(NUM_ZONES):
                for d in range(NUM_ZONES):
                    for s in range(NUM_ZONES):
                        for a in range(NUM_ZONES):
                            all_keys.add((b, w, m, d, s, a))

    occupied = sum(1 for k in all_keys if cell_counts[k] > 0)
    coverage_pct = (occupied / TOTAL_CELLS) * 100

    # Sort empty / sparse cells by count ascending
    sparse_cells = sorted(
        [(k, cell_counts[k]) for k in all_keys if cell_counts[k] < MIN_COVERAGE],
        key=lambda x: x[1]
    )

    # Sort overcrowded cells descending
    crowded_cells = sorted(
        [(k, cell_counts[k]) for k in all_keys if cell_counts[k] > OVERCROWD_THRESHOLD],
        key=lambda x: -x[1]
    )

    # Compute standard deviation as a distribution metric
    counts = [cell_counts[k] for k in all_keys]
    mean = sum(counts) / len(counts)
    variance = sum((c - mean) ** 2 for c in counts) / len(counts)
    std_dev = math.sqrt(variance)

    return {
        "total_presets": len(presets),
        "occupied_cells": occupied,
        "coverage_pct": coverage_pct,
        "mean_per_cell": mean,
        "std_dev": std_dev,
        "sparse_cells": sparse_cells,
        "crowded_cells": crowded_cells,
        "cell_counts": cell_counts,
        "cell_preset_names": cell_preset_names,
    }


# ---------------------------------------------------------------------------
# Reporting
# ---------------------------------------------------------------------------

def format_report(analysis: dict, top_n: int = 20, preset_dir: str = "") -> str:
    lines = []

    def h(text):
        lines.append("")
        lines.append("=" * 70)
        lines.append(f"  {text}")
        lines.append("=" * 70)

    def section(text):
        lines.append("")
        lines.append(f"--- {text} ---")

    lines.append("XO_OX Sonic DNA Gap Finder")
    lines.append(f"Preset directory : {preset_dir}")
    lines.append(f"Total presets    : {analysis['total_presets']}")
    lines.append(f"Total cells      : {TOTAL_CELLS} (3^6 grid)")
    lines.append(f"Occupied cells   : {analysis['occupied_cells']}")
    lines.append(f"Coverage density : {analysis['coverage_pct']:.1f}%")
    lines.append(f"Mean per cell    : {analysis['mean_per_cell']:.2f}")
    lines.append(f"Std deviation    : {analysis['std_dev']:.2f}")

    # --- Sparse / Empty cells ---
    h(f"TOP {top_n} MOST EMPTY CELLS (priority fill zones)")
    sparse = analysis["sparse_cells"][:top_n]
    if not sparse:
        lines.append("  None found — all cells have at least 2 presets. Well covered!")
    for rank, (key, count) in enumerate(sparse, 1):
        mid = cell_midpoint_values(key)
        axis = felix_oscar_label(mid["brightness"], mid["warmth"])
        desc = cell_description(key)
        gap = max(0, MIN_COVERAGE - count)
        lines.append("")
        lines.append(f"  #{rank:02d}  [{count} preset{'s' if count != 1 else ''}]  Cell {key}")
        lines.append(f"       Sound: {desc}")
        lines.append(f"       Axis : {axis}")
        lines.append(f"       Need : +{gap} preset{'s' if gap != 1 else ''} to reach minimum coverage")
        lines.append("       Fill prescription:")
        lines.append(fill_prescription(key))

    # --- Overcrowded cells ---
    h("TOP 10 OVERCROWDED CELLS (saturation warning)")
    crowded = analysis["crowded_cells"][:10]
    if not crowded:
        lines.append(f"  None — no cells exceed {OVERCROWD_THRESHOLD} presets.")
    for rank, (key, count) in enumerate(crowded, 1):
        desc = cell_description(key)
        mid = cell_midpoint_values(key)
        axis = felix_oscar_label(mid["brightness"], mid["warmth"])
        # Show a sample of preset names
        names = analysis["cell_preset_names"][key]
        sample = names[:3]
        sample_str = ", ".join(sample)
        if len(names) > 3:
            sample_str += f" ... (+{len(names) - 3} more)"
        lines.append("")
        lines.append(f"  #{rank:02d}  [{count} presets]  Cell {key}")
        lines.append(f"       Sound: {desc}")
        lines.append(f"       Axis : {axis}")
        lines.append(f"       Examples: {sample_str}")
        lines.append(f"       [!] Saturated — consider diversifying into adjacent cells")

    # --- Coverage summary ---
    h("COVERAGE SUMMARY")
    total = TOTAL_CELLS
    occupied = analysis["occupied_cells"]
    zero_count = sum(1 for _, c in analysis["sparse_cells"] if c == 0)
    one_count = sum(1 for _, c in analysis["sparse_cells"] if c == 1)
    well_covered = total - len(analysis["sparse_cells"])

    lines.append(f"  Empty (0 presets)      : {zero_count:4d} cells  ({zero_count/total*100:.1f}%)")
    lines.append(f"  Thin  (1 preset)       : {one_count:4d} cells  ({one_count/total*100:.1f}%)")
    lines.append(f"  Well-covered (≥{MIN_COVERAGE})      : {well_covered:4d} cells  ({well_covered/total*100:.1f}%)")
    lines.append(f"  Overcrowded (>{OVERCROWD_THRESHOLD})    : {len(analysis['crowded_cells']):4d} cells  ({len(analysis['crowded_cells'])/total*100:.1f}%)")

    # feliX–Oscar balance
    section("feliX–Oscar Axis Distribution")
    axis_counts = defaultdict(int)
    for p_count in range(TOTAL_CELLS):
        pass  # calculated below

    axis_map = defaultdict(int)
    for key, count in analysis["cell_counts"].items():
        if count == 0:
            continue
        mid = cell_midpoint_values(key)
        ax = felix_oscar_label(mid["brightness"], mid["warmth"])
        axis_map[ax] += count

    total_placed = sum(axis_map.values())
    for ax in ["feliX pole", "feliX-leaning", "Balanced", "Oscar-leaning", "Oscar pole"]:
        c = axis_map.get(ax, 0)
        bar = "#" * min(40, int(c / max(total_placed, 1) * 40))
        lines.append(f"  {ax:<20} : {c:5d}  {bar}")

    lines.append("")
    lines.append("=" * 70)
    lines.append("  END OF REPORT")
    lines.append("=" * 70)
    lines.append("")

    return "\n".join(lines)


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description="XO_OX Sonic DNA Gap Finder — find underrepresented regions in preset space"
    )
    parser.add_argument(
        "--preset-dir", "-d",
        required=True,
        type=Path,
        help="Directory to scan recursively for .xometa preset files",
    )
    parser.add_argument(
        "--top", "-t",
        type=int,
        default=20,
        help="Number of empty/sparse cells to report (default: 20)",
    )
    parser.add_argument(
        "--engine", "-e",
        default=None,
        help="Filter presets by engine name substring (case-insensitive)",
    )
    parser.add_argument(
        "--output", "-o",
        default=None,
        help="Write report to this file in addition to stdout",
    )
    args = parser.parse_args()

    if not args.preset_dir.exists():
        print(f"[error] Preset directory not found: {args.preset_dir}", file=sys.stderr)
        sys.exit(1)

    print(f"Scanning {args.preset_dir} ...", file=sys.stderr)
    presets = load_presets(args.preset_dir, engine_filter=args.engine)

    if not presets:
        print("[error] No presets loaded. Check directory and engine filter.", file=sys.stderr)
        sys.exit(1)

    print(f"Loaded {len(presets)} presets. Analyzing...", file=sys.stderr)
    analysis = analyze(presets)

    report = format_report(analysis, top_n=args.top, preset_dir=str(args.preset_dir))

    print(report)

    if args.output:
        out_path = Path(args.output)
        out_path.write_text(report, encoding="utf-8")
        print(f"[info] Report written to {out_path}", file=sys.stderr)


if __name__ == "__main__":
    main()
