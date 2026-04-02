#!/usr/bin/env python3
"""
XPN Engine Distinctness Analyzer — XO_OX Designs
Compares preset DNA profiles across engines to detect identity overlap and drift.

With 44+ engines, sonic identity drift is a real risk — two engines can gradually
start occupying the same sonic territory. This tool quantifies how distinct each
engine's preset library is from every other engine's.

Outputs:
  - Per-engine DNA centroid (average of 6D Sonic DNA)
  - Pairwise cosine similarity matrix
  - Identity overlap warnings (similarity > threshold)
  - Engines with the weakest/strongest distinctness scores

Usage:
    # Full fleet distinctness report
    python3 xpn_engine_distinctness_analyzer.py

    # Compare two specific engines
    python3 xpn_engine_distinctness_analyzer.py --engines Overdub Oceanic

    # Set overlap warning threshold (default 0.85)
    python3 xpn_engine_distinctness_analyzer.py --threshold 0.80

    # JSON output for CI
    python3 xpn_engine_distinctness_analyzer.py --json
"""

import argparse
import json
import math
import sys
from collections import defaultdict
from pathlib import Path

REPO_ROOT = Path(__file__).parent.parent.resolve()
PRESETS_DIR = REPO_ROOT / "Presets" / "XOceanus"

DNA_DIMS = ["brightness", "warmth", "movement", "density", "space", "aggression"]

def load_engine_dna(engine_name: str) -> list[dict]:
    """Load all preset DNA vectors for an engine."""
    vectors = []
    for mood_dir in PRESETS_DIR.iterdir():
        if not mood_dir.is_dir():
            continue
        for eng_dir in mood_dir.iterdir():
            if not eng_dir.is_dir():
                continue
            if eng_dir.name.lower() != engine_name.lower():
                continue
            for p in eng_dir.glob("*.xometa"):
                if p.stem.endswith("_Dry"):
                    continue
                try:
                    data = json.loads(p.read_text(encoding="utf-8"))
                    dna = data.get("sonic_dna") or data.get("dna") or {}
                    vec = [float(dna.get(dim, 0.5)) for dim in DNA_DIMS]
                    vectors.append(vec)
                except Exception as e:
                    print(f'[WARN] Preset parse failed: {e}', file=sys.stderr)
    return vectors

def centroid(vectors: list[list[float]]) -> list[float]:
    if not vectors:
        return [0.5] * len(DNA_DIMS)
    n = len(vectors)
    return [sum(v[i] for v in vectors) / n for i in range(len(DNA_DIMS))]

def cosine_similarity(a: list[float], b: list[float]) -> float:
    dot = sum(x * y for x, y in zip(a, b))
    mag_a = math.sqrt(sum(x * x for x in a))
    mag_b = math.sqrt(sum(y * y for y in b))
    if mag_a == 0 or mag_b == 0:
        return 0.0
    return dot / (mag_a * mag_b)

def discover_engines() -> list[str]:
    engines = set()
    for mood_dir in PRESETS_DIR.iterdir():
        if mood_dir.is_dir():
            for eng_dir in mood_dir.iterdir():
                if eng_dir.is_dir() and list(eng_dir.glob("*.xometa")):
                    engines.add(eng_dir.name)
    return sorted(engines)

def main() -> int:
    parser = argparse.ArgumentParser(
        description="XPN Engine Distinctness Analyzer",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument("--engines", nargs="+", metavar="NAME",
                        help="Specific engines to compare (default: all)")
    parser.add_argument("--threshold", type=float, default=0.85,
                        help="Similarity threshold for overlap warning (default: 0.85)")
    parser.add_argument("--json", action="store_true", dest="json_out",
                        help="Output JSON instead of human-readable report")
    args = parser.parse_args()

    engines = args.engines or discover_engines()
    if not engines:
        print("No engine preset directories found.")
        return 1

    print(f"  Analyzing {len(engines)} engines…")

    # Load centroids
    centroids = {}
    preset_counts = {}
    for eng in engines:
        vecs = load_engine_dna(eng)
        preset_counts[eng] = len(vecs)
        if vecs:
            centroids[eng] = centroid(vecs)

    if len(centroids) < 2:
        print("  Need at least 2 engines with presets to compare.")
        return 1

    # Compute pairwise similarities
    eng_list = list(centroids.keys())
    similarities = {}
    warnings = []

    for i, e1 in enumerate(eng_list):
        for j, e2 in enumerate(eng_list):
            if j <= i:
                continue
            sim = cosine_similarity(centroids[e1], centroids[e2])
            similarities[(e1, e2)] = sim
            if sim > args.threshold:
                warnings.append((e1, e2, sim))

    # Per-engine distinctness score (avg dissimilarity from all others)
    distinctness = {}
    for eng in eng_list:
        dissims = []
        for other in eng_list:
            if other == eng:
                continue
            pair = (eng, other) if (eng, other) in similarities else (other, eng)
            sim = similarities.get(pair, 0.0)
            dissims.append(1.0 - sim)
        distinctness[eng] = sum(dissims) / len(dissims) if dissims else 1.0

    if args.json_out:
        output = {
            "engines": eng_list,
            "preset_counts": preset_counts,
            "centroids": {e: dict(zip(DNA_DIMS, centroids[e])) for e in eng_list},
            "distinctness_scores": distinctness,
            "overlap_warnings": [{"engine_a": a, "engine_b": b, "similarity": round(s, 3)}
                                  for a, b, s in warnings],
        }
        print(json.dumps(output, indent=2))
        return 0

    # Human-readable report
    print()
    print(f"  {'='*60}")
    print(f"  ENGINE DISTINCTNESS REPORT")
    print(f"  Overlap threshold: {args.threshold:.0%}")
    print(f"  {'='*60}")
    print()
    print(f"  {'Engine':<25} {'Presets':>8} {'Distinctness':>14}")
    print(f"  {'-'*50}")

    sorted_engines = sorted(eng_list, key=lambda e: distinctness.get(e, 0.0), reverse=True)
    for eng in sorted_engines:
        score = distinctness.get(eng, 0.0)
        count = preset_counts.get(eng, 0)
        bar = "█" * int(score * 20)
        print(f"  {eng:<25} {count:>8}  {score:.3f}  {bar}")

    if warnings:
        print()
        print(f"  ⚠  IDENTITY OVERLAP WARNINGS (similarity > {args.threshold:.0%})")
        print(f"  {'-'*50}")
        for a, b, sim in sorted(warnings, key=lambda x: -x[2]):
            print(f"  {a:<22}  ↔  {b:<22}  {sim:.3f}")
    else:
        print()
        print(f"  ✓ All engines are distinct (no pairs above {args.threshold:.0%} similarity)")

    print()
    most_distinct = sorted_engines[0]
    least_distinct = sorted_engines[-1]
    print(f"  Most distinct:  {most_distinct} ({distinctness[most_distinct]:.3f})")
    print(f"  Least distinct: {least_distinct} ({distinctness[least_distinct]:.3f})")
    print()
    return 1 if warnings else 0

if __name__ == "__main__":
    sys.exit(main())
