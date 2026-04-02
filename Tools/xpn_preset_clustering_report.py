#!/usr/bin/env python3
"""
xpn_preset_clustering_report.py

Clusters presets by their 6D Sonic DNA using k-means (stdlib only).
Generates a sonic topology report identifying sparse/dense zones.

6D Sonic DNA dimensions: brightness, warmth, movement, density, space, aggression

Usage:
    python xpn_preset_clustering_report.py --preset-dir <dir> [--clusters 8] \
        [--seed 42] [--output report.txt] [--json clusters.json]
"""

import argparse
import json
import math
import random
import sys
from pathlib import Path

# ---------------------------------------------------------------------------
# Data loading
# ---------------------------------------------------------------------------

DNA_KEYS = ["brightness", "warmth", "movement", "density", "space", "aggression"]


def load_presets(preset_dir: Path) -> list[dict]:
    """Load all .xometa files from a directory, extract Sonic DNA + metadata."""
    presets = []
    for path in sorted(preset_dir.rglob("*.xometa")):
        try:
            with path.open("r", encoding="utf-8") as f:
                data = json.load(f)
        except (json.JSONDecodeError, OSError) as e:
            print(f"  Warning: skipping {path.name} — {e}", file=sys.stderr)
            continue

        dna_raw = data.get("sonicDna") or data.get("sonic_dna") or {}
        dna = []
        valid = True
        for key in DNA_KEYS:
            val = dna_raw.get(key)
            if val is None:
                valid = False
                break
            try:
                dna.append(float(val))
            except (TypeError, ValueError):
                valid = False
                break

        if not valid:
            print(f"  Warning: skipping {path.name} — missing/invalid Sonic DNA", file=sys.stderr)
            continue

        presets.append({
            "name": data.get("name") or path.stem,
            "engine": data.get("engine") or data.get("engineId") or "Unknown",
            "mood": data.get("mood") or data.get("moodTag") or "Unknown",
            "dna": dna,
            "path": str(path),
        })

    return presets


# ---------------------------------------------------------------------------
# K-means (stdlib only)
# ---------------------------------------------------------------------------

def euclidean(a: list[float], b: list[float]) -> float:
    return math.sqrt(sum((x - y) ** 2 for x, y in zip(a, b)))


def centroid(points: list[list[float]]) -> list[float]:
    n = len(points)
    dims = len(points[0])
    return [sum(p[d] for p in points) / n for d in range(dims)]


def kmeans(
    vectors: list[list[float]],
    k: int,
    seed: int = 42,
    max_iter: int = 100,
) -> tuple[list[int], list[list[float]]]:
    """
    K-means clustering from scratch.

    Returns:
        assignments: list of cluster indices (one per vector)
        centroids: list of k centroid vectors
    """
    rng = random.Random(seed)
    n = len(vectors)

    if n < k:
        raise ValueError(f"Not enough presets ({n}) for k={k} clusters. Lower --clusters.")

    # Random initialisation — pick k distinct vectors as starting centroids
    indices = rng.sample(range(n), k)
    centroids_cur = [vectors[i][:] for i in indices]

    assignments = [0] * n

    for iteration in range(max_iter):
        # Assignment step
        new_assignments = []
        for vec in vectors:
            dists = [euclidean(vec, c) for c in centroids_cur]
            new_assignments.append(dists.index(min(dists)))

        # Check convergence
        if new_assignments == assignments and iteration > 0:
            break
        assignments = new_assignments

        # Update step
        new_centroids = []
        for cluster_id in range(k):
            members = [vectors[i] for i, a in enumerate(assignments) if a == cluster_id]
            if members:
                new_centroids.append(centroid(members))
            else:
                # Empty cluster — reinitialise to a random vector
                new_centroids.append(vectors[rng.randrange(n)][:])
        centroids_cur = new_centroids

    return assignments, centroids_cur


# ---------------------------------------------------------------------------
# Analysis helpers
# ---------------------------------------------------------------------------

def dominant(values: list[str]) -> str:
    """Return the most common string in a list."""
    counts: dict[str, int] = {}
    for v in values:
        counts[v] = counts.get(v, 0) + 1
    return max(counts, key=lambda k: counts[k])


def representatives(
    presets: list[dict],
    centroid_vec: list[float],
    n: int = 3,
) -> list[str]:
    """Return names of the n presets closest to the centroid."""
    ranked = sorted(presets, key=lambda p: euclidean(p["dna"], centroid_vec))
    return [p["name"] for p in ranked[:n]]


def inter_cluster_distances(centroids: list[list[float]]) -> list[tuple[int, int, float]]:
    """Compute all pairwise centroid distances, sorted ascending."""
    pairs = []
    k = len(centroids)
    for i in range(k):
        for j in range(i + 1, k):
            pairs.append((i, j, euclidean(centroids[i], centroids[j])))
    return sorted(pairs, key=lambda t: t[2])


def dna_bar(value: float, width: int = 10) -> str:
    """ASCII bar for a 0-1 float."""
    filled = round(value * width)
    return "[" + "█" * filled + "░" * (width - filled) + f"] {value:.2f}"


# ---------------------------------------------------------------------------
# Report generation
# ---------------------------------------------------------------------------

def build_report(
    presets: list[dict],
    assignments: list[int],
    centroids: list[list[float]],
    k: int,
    preset_dir: Path,
) -> str:
    lines = []
    total = len(presets)

    lines.append("=" * 72)
    lines.append("  XO_OX XOceanus — Sonic DNA Clustering Report")
    lines.append("=" * 72)
    lines.append(f"  Preset directory : {preset_dir}")
    lines.append(f"  Total presets    : {total}")
    lines.append(f"  Clusters (k)     : {k}")
    lines.append("")

    # Build per-cluster data
    clusters: list[dict] = []
    for cid in range(k):
        members = [presets[i] for i, a in enumerate(assignments) if a == cid]
        count = len(members)
        pct = count / total * 100 if total else 0
        dom_mood = dominant([m["mood"] for m in members]) if members else "—"
        dom_engine = dominant([m["engine"] for m in members]) if members else "—"
        reps = representatives(members, centroids[cid]) if members else []
        clusters.append({
            "id": cid,
            "count": count,
            "pct": pct,
            "centroid": centroids[cid],
            "dom_mood": dom_mood,
            "dom_engine": dom_engine,
            "reps": reps,
            "members": members,
        })

    # Sort clusters by size descending for readability
    clusters_by_size = sorted(clusters, key=lambda c: c["count"], reverse=True)

    # ---- Cluster details -----------------------------------------------
    lines.append("─" * 72)
    lines.append("  CLUSTER PROFILES")
    lines.append("─" * 72)

    for cl in clusters_by_size:
        cid = cl["id"]
        count = cl["count"]
        pct = cl["pct"]
        flag = ""
        if pct < 5:
            flag = "  ⚑ SPARSE"
        elif pct > 25:
            flag = "  ⚑ DENSE"

        lines.append(f"\n  Cluster {cid:2d}  —  {count:4d} presets  ({pct:5.1f}%){flag}")
        lines.append(f"  Dominant mood  : {cl['dom_mood']}")
        lines.append(f"  Dominant engine: {cl['dom_engine']}")
        lines.append(f"  Representatives: {', '.join(cl['reps']) if cl['reps'] else '—'}")
        lines.append("")
        lines.append("  Sonic DNA centroid:")
        for dim, val in zip(DNA_KEYS, cl["centroid"]):
            lines.append(f"    {dim:<12}  {dna_bar(val)}")

    # ---- Sparse / dense summary ----------------------------------------
    sparse = [cl for cl in clusters if cl["pct"] < 5.0 and cl["count"] > 0]
    dense  = [cl for cl in clusters if cl["pct"] > 25.0]
    empty  = [cl for cl in clusters if cl["count"] == 0]

    lines.append("\n" + "─" * 72)
    lines.append("  ZONE ANALYSIS")
    lines.append("─" * 72)

    if sparse:
        lines.append(f"\n  Sparse clusters (< 5% of presets) — potential gap zones:")
        for cl in sparse:
            dna_summary = ", ".join(
                f"{k}={v:.2f}" for k, v in zip(DNA_KEYS, cl["centroid"])
            )
            lines.append(f"    Cluster {cl['id']:2d}  {cl['count']:3d} presets  [{dna_summary}]")
    else:
        lines.append("\n  No sparse clusters — coverage looks balanced.")

    if dense:
        lines.append(f"\n  Dense clusters (> 25% of presets) — potential oversaturation:")
        for cl in dense:
            lines.append(
                f"    Cluster {cl['id']:2d}  {cl['count']:3d} presets  "
                f"({cl['pct']:.1f}%)  dom_mood={cl['dom_mood']}  dom_engine={cl['dom_engine']}"
            )
    else:
        lines.append("\n  No dense clusters — distribution looks even.")

    if empty:
        lines.append(f"\n  Empty clusters: {[cl['id'] for cl in empty]}")
        lines.append("  Consider reducing --clusters or adding more presets in those zones.")

    # ---- Inter-cluster distances ----------------------------------------
    pairs = inter_cluster_distances(centroids)

    lines.append("\n" + "─" * 72)
    lines.append("  INTER-CLUSTER DISTANCES (centroid-to-centroid, Euclidean 6D)")
    lines.append("─" * 72)
    lines.append("\n  Most similar pairs (closest — risk of overlap):")
    for i, j, dist in pairs[:5]:
        lines.append(f"    Clusters {i:2d} ↔ {j:2d}   distance = {dist:.4f}")
    lines.append("\n  Most distinct pairs (furthest — sonic extremes):")
    for i, j, dist in pairs[-5:]:
        lines.append(f"    Clusters {i:2d} ↔ {j:2d}   distance = {dist:.4f}")

    # ---- DNA dimension spread ------------------------------------------
    lines.append("\n" + "─" * 72)
    lines.append("  CENTROID DNA HEATMAP  (all clusters × 6 dimensions)")
    lines.append("─" * 72)
    header = f"  {'Cluster':<10}" + "".join(f"{d:<14}" for d in DNA_KEYS)
    lines.append("\n" + header)
    lines.append("  " + "─" * (10 + 14 * len(DNA_KEYS)))
    for cl in clusters:
        row = f"  {cl['id']:<10}" + "".join(f"{v:<14.3f}" for v in cl["centroid"])
        lines.append(row)

    lines.append("\n" + "=" * 72)
    lines.append("  End of report")
    lines.append("=" * 72)

    return "\n".join(lines)


# ---------------------------------------------------------------------------
# JSON output
# ---------------------------------------------------------------------------

def build_json(
    presets: list[dict],
    assignments: list[int],
    centroids: list[list[float]],
    k: int,
) -> dict:
    total = len(presets)
    cluster_data = []
    for cid in range(k):
        members = [
            {"name": presets[i]["name"], "engine": presets[i]["engine"],
             "mood": presets[i]["mood"], "path": presets[i]["path"]}
            for i, a in enumerate(assignments) if a == cid
        ]
        count = len(members)
        pct = count / total * 100 if total else 0
        cluster_data.append({
            "cluster_id": cid,
            "count": count,
            "percent": round(pct, 2),
            "centroid": {k: round(v, 4) for k, v in zip(DNA_KEYS, centroids[cid])},
            "sparse": pct < 5.0,
            "dense": pct > 25.0,
            "members": members,
        })

    assignments_out = [
        {"name": p["name"], "cluster": a}
        for p, a in zip(presets, assignments)
    ]

    return {
        "total_presets": total,
        "k": k,
        "clusters": cluster_data,
        "assignments": assignments_out,
    }


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Cluster XO_OX presets by Sonic DNA and generate a topology report."
    )
    parser.add_argument(
        "--preset-dir", "-d",
        required=True,
        type=Path,
        help="Directory to scan recursively for .xometa files.",
    )
    parser.add_argument(
        "--clusters", "-k",
        type=int,
        default=8,
        metavar="K",
        help="Number of clusters (default: 8, matching Plutchik emotions).",
    )
    parser.add_argument(
        "--seed",
        type=int,
        default=42,
        help="Random seed for k-means initialisation (default: 42).",
    )
    parser.add_argument(
        "--output", "-o",
        type=Path,
        default=None,
        help="Write text report to this file (default: print to stdout).",
    )
    parser.add_argument(
        "--json", "-j",
        type=Path,
        default=None,
        dest="json_out",
        help="Write full cluster assignments to this JSON file.",
    )
    return parser.parse_args()


def main() -> None:
    args = parse_args()

    preset_dir = args.preset_dir.resolve()
    if not preset_dir.is_dir():
        print(f"Error: --preset-dir '{preset_dir}' is not a directory.", file=sys.stderr)
        sys.exit(1)

    print(f"Loading presets from {preset_dir} …", file=sys.stderr)
    presets = load_presets(preset_dir)

    if not presets:
        print("Error: no valid .xometa presets found.", file=sys.stderr)
        sys.exit(1)

    print(f"Loaded {len(presets)} presets. Running k-means (k={args.clusters}, seed={args.seed}) …",
          file=sys.stderr)

    vectors = [p["dna"] for p in presets]

    try:
        assignments, centroids = kmeans(vectors, k=args.clusters, seed=args.seed)
    except ValueError as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)

    print("Clustering complete. Generating report …", file=sys.stderr)

    report = build_report(presets, assignments, centroids, args.clusters, preset_dir)

    if args.output:
        args.output.write_text(report, encoding="utf-8")
        print(f"Report written to {args.output}", file=sys.stderr)
    else:
        print(report)

    if args.json_out:
        data = build_json(presets, assignments, centroids, args.clusters)
        args.json_out.write_text(json.dumps(data, indent=2), encoding="utf-8")
        print(f"JSON written to {args.json_out}", file=sys.stderr)


if __name__ == "__main__":
    main()
