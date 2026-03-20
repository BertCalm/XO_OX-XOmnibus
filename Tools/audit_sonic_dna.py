#!/usr/bin/env python3
"""
XOmnibus Sonic DNA Coverage Auditor

Scans all .xometa preset files and audits the 6D sonic DNA coverage per engine:
  [brightness, warmth, movement, density, space, aggression]

Reports:
  - Which files are missing a DNA block
  - Per-engine min/max range for each dimension
  - Dimensions that never go above 0.7 (underrepresented high end)
  - Dimensions that never go below 0.3 (underrepresented low end)
  - Overall gap score per engine (higher = worse coverage)
  - The 3 engines with worst coverage

Usage:
    python3 audit_sonic_dna.py [--verbose]
"""

import json
import glob
import sys
from collections import defaultdict
from pathlib import Path

REPO_ROOT = Path(__file__).parent.parent
PRESET_DIR = REPO_ROOT / "Presets"

DIMS = ["brightness", "warmth", "movement", "density", "space", "aggression"]

# All 34 registered engines in XOmnibus
ALL_ENGINES = [
    "OddfeliX", "OddOscar", "XOverdub", "XOdyssey", "XOblong", "XObese",
    "XOnset", "XOverworld", "XOpal", "XOrbital", "XOrganon", "XOuroboros",
    "XObsidian", "XOverbite", "XOrigami", "XOracle", "XObscura", "XOceanic",
    "XOcelot", "XOptic", "XOblique", "XOsprey", "XOsteria", "XOwlfish",
    "XOhm", "XOrphica", "XObbligato", "XOttoni", "XOle", "XOverlap",
    "XOutwit", "XOmbre", "XOrca", "XOctopus",
]

HIGH_THRESHOLD = 0.7   # dimension max must reach this to be "covered" at high end
LOW_THRESHOLD  = 0.3   # dimension min must reach this (or lower) to be "covered" at low end


def load_presets() -> tuple[list[dict], list[str]]:
    """Load all .xometa files. Returns (list_of_dicts, list_of_missing_dna_paths)."""
    files = glob.glob(str(PRESET_DIR / "**" / "*.xometa"), recursive=True)
    presets = []
    missing_dna = []

    for f in sorted(files):
        try:
            with open(f) as fh:
                data = json.load(fh)
        except (json.JSONDecodeError, IOError) as e:
            print(f"  WARN: could not parse {f}: {e}", file=sys.stderr)
            continue

        dna = data.get("dna") or data.get("sonic_dna")
        if dna is None:
            missing_dna.append(f)
            continue

        # Validate all 6 dims present
        has_all = all(d in dna for d in DIMS)
        if not has_all:
            missing_dna.append(f)
            continue

        presets.append({
            "path": f,
            "name": data.get("name", Path(f).stem),
            "mood": data.get("mood", "Unknown"),
            "engines": data.get("engines", []),
            "dna": dna,
        })

    return presets, missing_dna


def build_engine_dna_map(presets: list[dict]) -> dict[str, dict[str, list[float]]]:
    """Group DNA values by engine. Each engine includes all presets it appears in."""
    engine_dna = defaultdict(lambda: {d: [] for d in DIMS})

    for preset in presets:
        for engine in preset["engines"]:
            for dim in DIMS:
                val = preset["dna"].get(dim)
                if val is not None:
                    engine_dna[engine][dim].append(float(val))

    return engine_dna


def compute_coverage(dim_values: list[float]) -> dict[str, float]:
    """
    Given a list of values for one dimension, return a coverage dict:
      {min, max, mean, n, gap_high (bool), gap_low (bool), range_width}
    """
    if not dim_values:
        return {
            "min": None, "max": None, "mean": None, "n": 0,
            "gap_high": True, "gap_low": True, "range_width": 0.0,
        }
    mn = min(dim_values)
    mx = max(dim_values)
    mean = sum(dim_values) / len(dim_values)
    return {
        "min": mn,
        "max": mx,
        "mean": mean,
        "n": len(dim_values),
        "gap_high": mx < HIGH_THRESHOLD,
        "gap_low": mn > LOW_THRESHOLD,
        "range_width": mx - mn,
    }


def score_engine(engine_coverage: dict[str, dict[str, float]]) -> float:
    """
    Compute a gap score for an engine (higher = worse coverage).
    Score = number of gap flags (max 12) + (1 - avg_range_width) as tiebreaker.
    """
    total_gaps = 0
    range_widths = []
    for dim, cov in engine_coverage.items():
        if cov["gap_high"]:
            total_gaps += 1
        if cov["gap_low"]:
            total_gaps += 1
        range_widths.append(cov.get("range_width", 0.0))
    avg_range = sum(range_widths) / len(range_widths) if range_widths else 0
    return total_gaps, avg_range


def print_report(engine_dna_map: dict, missing_dna_paths: list[str], presets: list[dict], verbose: bool = False) -> None:
    """Print the full audit report to stdout."""
    sep  = "=" * 90
    sep2 = "-" * 90

    print(sep)
    print("XOmnibus Sonic DNA Coverage Audit")
    print(sep)
    print(f"Total presets scanned:       {len(presets) + len(missing_dna_paths)}")
    print(f"Presets with complete DNA:   {len(presets)}")
    print(f"Presets missing/incomplete:  {len(missing_dna_paths)}")
    if missing_dna_paths and verbose:
        print("\nMissing DNA files:")
        for p in missing_dna_paths:
            print(f"  {p}")

    print()
    print(sep)
    print("PER-ENGINE COVERAGE TABLE")
    print(sep)
    print(f"{'Engine':<15} {'N':>5}   " +
          "  ".join(f"{'['+d[:5]+']':>12}" for d in DIMS))
    print(f"{'':15} {'':5}   " +
          "  ".join(f"{'min-max':>12}" for _ in DIMS))
    print(sep2)

    engine_scores = {}
    engine_coverages = {}

    for engine in sorted(engine_dna_map.keys()):
        vals = engine_dna_map[engine]
        n = len(vals["brightness"])
        covs = {d: compute_coverage(vals[d]) for d in DIMS}
        engine_coverages[engine] = covs
        gap_count, avg_range = score_engine(covs)
        engine_scores[engine] = (gap_count, avg_range, n)

        row = f"{engine:<15} {n:>5}   "
        for dim in DIMS:
            cov = covs[dim]
            if cov["min"] is None:
                cell = "     N/A    "
            else:
                mn_s = f"{cov['min']:.2f}"
                mx_s = f"{cov['max']:.2f}"
                flags = ("!" if cov["gap_high"] else " ") + ("!" if cov["gap_low"] else " ")
                cell = f"{mn_s}-{mx_s} {flags}"
            row += f"  {cell:>12}"
        print(row)

    print()
    print("  Legend: ! after max-min pair = coverage gap (! after first number = max<0.7; ! after second = min>0.3)")

    # Rank by gap count (descending), then avg_range (ascending = narrower = worse)
    ranked = sorted(engine_scores.items(), key=lambda x: (-x[1][0], x[1][1]))

    print()
    print(sep)
    print("ENGINE GAP SCORE RANKING (higher gap count = worse coverage)")
    print(sep)
    print(f"{'Rank':<5} {'Engine':<15} {'Presets':>8} {'Gap Count':>10} {'Avg Range':>10}")
    print(sep2)
    for rank, (engine, (gaps, avg_range, n)) in enumerate(ranked, 1):
        print(f"{rank:<5} {engine:<15} {n:>8} {gaps:>10} {avg_range:>10.3f}")

    print()
    print(sep)
    print("TOP 3 ENGINES WITH WORST DNA COVERAGE")
    print(sep)
    worst_3 = ranked[:3]
    for rank, (engine, (gaps, avg_range, n)) in enumerate(worst_3, 1):
        print(f"\n#{rank}: {engine}  (gap_count={gaps}, avg_range={avg_range:.3f}, presets={n})")
        covs = engine_coverages[engine]
        for dim in DIMS:
            cov = covs[dim]
            if cov["min"] is None:
                print(f"      {dim:<12}: NO DATA")
                continue
            gaps_str = []
            if cov["gap_high"]:
                gaps_str.append(f"max={cov['max']:.2f} < {HIGH_THRESHOLD} (HIGH END MISSING)")
            if cov["gap_low"]:
                gaps_str.append(f"min={cov['min']:.2f} > {LOW_THRESHOLD} (LOW END MISSING)")
            gap_note = " | ".join(gaps_str) if gaps_str else "OK"
            print(f"      {dim:<12}: min={cov['min']:.2f}  max={cov['max']:.2f}  "
                  f"mean={cov['mean']:.2f}  range={cov['range_width']:.2f}  [{gap_note}]")

    print()
    print(sep)
    print("GAP-FILL RECOMMENDATIONS (4 corners per engine)")
    print(sep)
    for rank, (engine, (gaps, avg_range, n)) in enumerate(worst_3, 1):
        print(f"\n{engine} — 4 gap-fill targets:")
        print(f"  Corner 1: High brightness (0.90) + High aggression (0.90)")
        print(f"  Corner 2: Low warmth (0.05) + High complexity/density (0.90)")
        print(f"  Corner 3: High depth/space (0.95) + Low movement (0.05)")
        print(f"  Corner 4: Maximum everything (all dims 0.85+)")

    # --- Fleet coverage: ensure all 34 engines have at least some DNA data ---
    print()
    print(sep)
    print("FLEET DNA COVERAGE (all 34 registered engines)")
    print(sep)
    engines_with_data = set(engine_dna_map.keys())
    engines_missing = [e for e in ALL_ENGINES if e not in engines_with_data]
    engines_present = [e for e in ALL_ENGINES if e in engines_with_data]

    print(f"  Engines with DNA data:  {len(engines_present)}/34")
    print(f"  Engines missing data:   {len(engines_missing)}/34")
    if engines_missing:
        print(f"\n  MISSING engines (no presets with DNA found):")
        for e in engines_missing:
            print(f"    - {e}")
    else:
        print(f"\n  All 34 engines have DNA coverage.")

    # Check for engines in presets that are not in the registered list
    unknown_engines = engines_with_data - set(ALL_ENGINES)
    if unknown_engines:
        print(f"\n  UNKNOWN engines found in presets (not in registered list):")
        for e in sorted(unknown_engines):
            print(f"    - {e}")

    return worst_3, engine_coverages


def main() -> int:
    verbose = "--verbose" in sys.argv or "-v" in sys.argv

    presets, missing_dna = load_presets()
    engine_dna_map = build_engine_dna_map(presets)

    worst_3, engine_coverages = print_report(
        engine_dna_map, missing_dna, presets, verbose=verbose
    )

    print()
    print("=" * 90)
    print("Audit complete.")
    print(f"  3 worst engines: {', '.join(e for e, _ in worst_3)}")
    print("  Run generate_dna_gap_fills.py to write gap-fill presets for these engines.")
    print("=" * 90)

    return 0


if __name__ == "__main__":
    sys.exit(main())
