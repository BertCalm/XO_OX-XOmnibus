#!/usr/bin/env python3
"""
xpn_dna_diversity_analyzer.py
Analyzes the full XOmnibus preset fleet for DNA diversity problems.
Outputs a human-readable report to stdout and saves a JSON snapshot.
Uses stdlib only: json, os, math, random, statistics.
"""

import json
import os
import math
import random
import statistics
from collections import defaultdict

# ── Config ──────────────────────────────────────────────────────────────────
PRESETS_ROOT = os.path.join(
    os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
    "Presets", "XOmnibus"
)
SNAPSHOT_PATH = os.path.join(
    os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
    "Docs", "snapshots", "dna_diversity_analysis.json"
)

DNA_DIMS = ["brightness", "warmth", "movement", "density", "space", "aggression"]
MOODS = ["Foundation", "Atmosphere", "Entangled", "Prism", "Flux", "Aether", "Family"]

MIDRANGE_LO = 0.3
MIDRANGE_HI = 0.7
EXTREME_LO  = 0.15
EXTREME_HI  = 0.85
COSINE_SAMPLE = 500
CLUSTER_THRESH = 0.05   # Euclidean distance threshold for "duplicate" DNA


# ── Helpers ──────────────────────────────────────────────────────────────────
def cosine_distance(a, b):
    dot  = sum(x * y for x, y in zip(a, b))
    ma   = math.sqrt(sum(x * x for x in a))
    mb   = math.sqrt(sum(y * y for y in b))
    if ma == 0 or mb == 0:
        return 1.0
    return 1.0 - dot / (ma * mb)


def euclidean_distance(a, b):
    return math.sqrt(sum((x - y) ** 2 for x, y in zip(a, b)))


def dim_stats(values):
    n = len(values)
    if n == 0:
        return {}
    mn   = statistics.mean(values)
    sd   = statistics.stdev(values) if n > 1 else 0.0
    mid  = sum(1 for v in values if MIDRANGE_LO <= v <= MIDRANGE_HI) / n * 100
    xlo  = sum(1 for v in values if v <= EXTREME_LO) / n * 100
    xhi  = sum(1 for v in values if v >= EXTREME_HI) / n * 100
    return {"mean": round(mn, 3), "std": round(sd, 3),
            "pct_midrange": round(mid, 1),
            "pct_extreme_low": round(xlo, 1),
            "pct_extreme_high": round(xhi, 1)}


def load_presets():
    """Return list of dicts: {name, mood, dna_vec, dna_dict, file}."""
    presets = []
    for mood in MOODS:
        mood_dir = os.path.join(PRESETS_ROOT, mood)
        if not os.path.isdir(mood_dir):
            continue
        for fname in os.listdir(mood_dir):
            if not fname.endswith(".xometa"):
                continue
            fpath = os.path.join(mood_dir, fname)
            try:
                with open(fpath, "r", encoding="utf-8") as f:
                    data = json.load(f)
            except Exception:
                continue
            dna = data.get("dna")
            if not isinstance(dna, dict):
                continue
            vec = [float(dna.get(d, 0.5)) for d in DNA_DIMS]
            presets.append({
                "name": data.get("name", fname),
                "mood": mood,
                "dna_vec": vec,
                "dna_dict": {d: float(dna.get(d, 0.5)) for d in DNA_DIMS},
                "file": fpath,
            })
    return presets


def compute_fleet_stats(presets):
    all_stats = {}
    dim_values = {d: [p["dna_dict"][d] for p in presets] for d in DNA_DIMS}
    for d in DNA_DIMS:
        all_stats[d] = dim_stats(dim_values[d])
    return all_stats


def compute_mood_stats(presets):
    by_mood = defaultdict(list)
    for p in presets:
        by_mood[p["mood"]].append(p)
    result = {}
    for mood, ps in by_mood.items():
        result[mood] = {
            "count": len(ps),
            "dims": {}
        }
        for d in DNA_DIMS:
            vals = [p["dna_dict"][d] for p in ps]
            result[mood]["dims"][d] = dim_stats(vals)
    return result


def compute_cosine_diversity(presets, n_pairs=COSINE_SAMPLE):
    """Sample n_pairs random pairs and return mean cosine distance (0-1)."""
    if len(presets) < 2:
        return 0.0
    pairs_done = 0
    total_dist = 0.0
    idx_range = range(len(presets))
    random.seed(42)
    for _ in range(n_pairs):
        i, j = random.sample(idx_range, 2)
        total_dist += cosine_distance(presets[i]["dna_vec"], presets[j]["dna_vec"])
        pairs_done += 1
    return round(total_dist / pairs_done, 4)


def find_top_clusters(presets, top_n=10, sample_cap=600):
    """Find presets whose DNA vectors are most similar (potential duplicates)."""
    # Work on a sample if fleet is large to keep runtime reasonable
    pool = presets
    if len(presets) > sample_cap:
        random.seed(42)
        pool = random.sample(presets, sample_cap)

    # For each preset, count how many others fall within CLUSTER_THRESH
    neighbor_counts = []
    for i, p in enumerate(pool):
        neighbors = []
        for j, q in enumerate(pool):
            if i == j:
                continue
            if euclidean_distance(p["dna_vec"], q["dna_vec"]) <= CLUSTER_THRESH:
                neighbors.append(q["name"])
        neighbor_counts.append({
            "name": p["name"],
            "mood": p["mood"],
            "dna": p["dna_dict"],
            "close_neighbors": len(neighbors),
            "neighbor_names": neighbors[:5],
        })

    neighbor_counts.sort(key=lambda x: x["close_neighbors"], reverse=True)
    return neighbor_counts[:top_n]


def generate_repair_recommendations(fleet_stats, mood_stats, diversity_score):
    recs = []

    # Fleet-level dimension checks
    for d in DNA_DIMS:
        s = fleet_stats[d]
        issues = []
        if s["pct_midrange"] > 65:
            issues.append(f"midrange-heavy ({s['pct_midrange']}% in 0.3–0.7)")
        if s["pct_extreme_low"] < 5:
            issues.append(f"very few extreme-low presets ({s['pct_extreme_low']}% ≤ 0.15)")
        if s["pct_extreme_high"] < 5:
            issues.append(f"very few extreme-high presets ({s['pct_extreme_high']}% ≥ 0.85)")
        if s["std"] < 0.15:
            issues.append(f"low variance (std={s['std']})")
        if issues:
            recs.append(f"[FLEET] {d.upper()}: {'; '.join(issues)}")

    # Mood-level clustering
    for mood, ms in mood_stats.items():
        mood_issues = []
        for d in DNA_DIMS:
            s = ms["dims"].get(d, {})
            if not s:
                continue
            if s.get("pct_midrange", 0) > 75:
                mood_issues.append(f"{d} midrange={s['pct_midrange']}%")
            if s.get("std", 1) < 0.10:
                mood_issues.append(f"{d} std={s['std']} (very low variance)")
        if mood_issues:
            recs.append(f"[MOOD:{mood}] clustered dimensions: {', '.join(mood_issues)}")

    # Overall diversity score interpretation
    if diversity_score < 0.10:
        recs.append(f"[CRITICAL] Fleet cosine diversity score {diversity_score} is dangerously low — fleet sounds monochromatic")
    elif diversity_score < 0.20:
        recs.append(f"[WARNING] Fleet cosine diversity score {diversity_score} is below healthy threshold (≥0.20)")
    elif diversity_score >= 0.30:
        recs.append(f"[GOOD] Fleet cosine diversity score {diversity_score} is healthy (≥0.30)")
    else:
        recs.append(f"[OK] Fleet cosine diversity score {diversity_score} is acceptable (0.20–0.30)")

    if not recs:
        recs.append("[GOOD] No critical diversity issues found.")
    return recs


def print_dim_table(stats_dict, label="Fleet"):
    print(f"\n{'─'*70}")
    print(f"  {label}")
    print(f"{'─'*70}")
    header = f"  {'DIM':<12}  {'MEAN':>6}  {'STD':>6}  {'MIDRANGE%':>10}  {'XLOW%':>7}  {'XHIGH%':>8}"
    print(header)
    print(f"  {'─'*10}  {'─'*6}  {'─'*6}  {'─'*10}  {'─'*7}  {'─'*8}")
    for d in DNA_DIMS:
        s = stats_dict[d]
        flag = ""
        if s["pct_midrange"] > 65:
            flag += " ◈MIDCROWD"
        if s["pct_extreme_low"] < 5:
            flag += " ▼XLOW"
        if s["pct_extreme_high"] < 5:
            flag += " ▲XHIGH"
        if s["std"] < 0.15:
            flag += " ~FLATVAR"
        print(f"  {d:<12}  {s['mean']:>6.3f}  {s['std']:>6.3f}  {s['pct_midrange']:>9.1f}%  {s['pct_extreme_low']:>6.1f}%  {s['pct_extreme_high']:>7.1f}%{flag}")


# ── Main ─────────────────────────────────────────────────────────────────────
def main():
    print("=" * 70)
    print("  XOmnibus DNA Diversity Analyzer")
    print("=" * 70)

    print("\nLoading presets...")
    presets = load_presets()
    total = len(presets)
    no_dna = 0  # already filtered in load_presets
    print(f"  Loaded {total} presets with DNA data")

    # Count presets missing DNA
    all_files = 0
    for mood in MOODS:
        mood_dir = os.path.join(PRESETS_ROOT, mood)
        if os.path.isdir(mood_dir):
            all_files += sum(1 for f in os.listdir(mood_dir) if f.endswith(".xometa"))
    no_dna = all_files - total
    print(f"  {no_dna} presets skipped (missing/invalid 'dna' field)")

    # Fleet-level stats
    fleet_stats = compute_fleet_stats(presets)
    print_dim_table(fleet_stats, label="FLEET-WIDE DNA STATISTICS")

    # Cosine diversity
    print(f"\nComputing cosine diversity ({COSINE_SAMPLE} random pairs)...")
    diversity_score = compute_cosine_diversity(presets)
    print(f"  Fleet cosine diversity score: {diversity_score}  (0=identical, 1=maximally diverse)")

    # Per-mood breakdown
    mood_stats = compute_mood_stats(presets)
    print("\n\n" + "=" * 70)
    print("  PER-MOOD BREAKDOWN")
    print("=" * 70)
    for mood in MOODS:
        if mood not in mood_stats:
            print(f"\n  {mood}: no data")
            continue
        ms = mood_stats[mood]
        print_dim_table(ms["dims"], label=f"{mood.upper()}  (n={ms['count']})")

    # Mood cosine diversity
    print("\n\n" + "=" * 70)
    print("  PER-MOOD COSINE DIVERSITY")
    print("=" * 70)
    by_mood = defaultdict(list)
    for p in presets:
        by_mood[p["mood"]].append(p)
    mood_diversity = {}
    for mood in MOODS:
        ps = by_mood.get(mood, [])
        if len(ps) < 2:
            mood_diversity[mood] = None
            print(f"  {mood:<14}: n/a (too few presets)")
            continue
        score = compute_cosine_diversity(ps, n_pairs=min(COSINE_SAMPLE, len(ps) * (len(ps) - 1) // 2))
        mood_diversity[mood] = score
        flag = ""
        if score < 0.10:
            flag = "  !! CRITICAL"
        elif score < 0.20:
            flag = "  ! LOW"
        elif score >= 0.30:
            flag = "  ✓ HEALTHY"
        print(f"  {mood:<14}: {score:.4f}{flag}")

    # Top 10 most-duplicated clusters
    print("\n\n" + "=" * 70)
    print("  TOP 10 MOST-DUPLICATED DNA VECTORS")
    print(f"  (Euclidean distance ≤ {CLUSTER_THRESH} counts as duplicate)")
    print("=" * 70)
    clusters = find_top_clusters(presets)
    for i, c in enumerate(clusters, 1):
        dna_str = "  ".join(f"{k[0].upper()}{v:.2f}" for k, v in c["dna"].items())
        print(f"\n  {i:2}. {c['name'][:45]:<45}  [{c['mood']}]")
        print(f"      DNA: {dna_str}")
        print(f"      Close neighbors: {c['close_neighbors']}", end="")
        if c["neighbor_names"]:
            print(f"  → {', '.join(c['neighbor_names'][:3])}", end="")
        print()

    # Repair recommendations
    recs = generate_repair_recommendations(fleet_stats, mood_stats, diversity_score)
    print("\n\n" + "=" * 70)
    print("  REPAIR RECOMMENDATIONS")
    print("=" * 70)
    for r in recs:
        print(f"  • {r}")

    # Build JSON snapshot
    snapshot = {
        "generated": "2026-03-16",
        "total_presets_with_dna": total,
        "total_presets_missing_dna": no_dna,
        "fleet_cosine_diversity": diversity_score,
        "fleet_dim_stats": fleet_stats,
        "per_mood": {
            mood: {
                "count": mood_stats[mood]["count"] if mood in mood_stats else 0,
                "cosine_diversity": mood_diversity.get(mood),
                "dims": mood_stats[mood]["dims"] if mood in mood_stats else {},
            }
            for mood in MOODS
        },
        "top_10_duplicate_clusters": [
            {
                "name": c["name"],
                "mood": c["mood"],
                "dna": c["dna"],
                "close_neighbors": c["close_neighbors"],
                "neighbor_names": c["neighbor_names"],
            }
            for c in clusters
        ],
        "repair_recommendations": recs,
    }

    os.makedirs(os.path.dirname(SNAPSHOT_PATH), exist_ok=True)
    with open(SNAPSHOT_PATH, "w", encoding="utf-8") as f:
        json.dump(snapshot, f, indent=2)
    print(f"\n\nSnapshot saved → {SNAPSHOT_PATH}")
    print("=" * 70)


if __name__ == "__main__":
    main()
