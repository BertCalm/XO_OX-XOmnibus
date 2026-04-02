#!/usr/bin/env python3
"""
xpn_clone_cluster_repair.py
----------------------------
Identifies and repairs DNA clone clusters in the XOceanus preset fleet.

A "clone cluster" is a group of 2+ presets where all 6 DNA dimensions
(brightness, warmth, movement, density, space, aggression) are within
±0.03 of each other — i.e., effectively identical sonic fingerprints.

Repair strategy (clusters with 3+ members):
  - Keep the first preset unchanged (anchor).
  - For every subsequent member: randomly perturb 2–3 DNA dimensions
    by ±0.15–0.35, pushing toward extremes, ensuring at least one
    dimension ends up XLOW (≤0.15) or XHIGH (≥0.85).
  - Update the file in-place (JSON pretty-printed, original structure
    preserved).

Report saved to: Docs/snapshots/clone_cluster_repair_report.json
"""

import json
import os
import random
import sys
from copy import deepcopy
from pathlib import Path
from typing import Dict, List, Optional

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------
REPO_ROOT = Path(__file__).resolve().parent.parent
PRESETS_DIR = REPO_ROOT / "Presets"
REPORT_PATH = REPO_ROOT / "Docs" / "snapshots" / "clone_cluster_repair_report.json"

CLUSTER_THRESHOLD = 0.03   # max per-dimension delta to be considered a clone
REPAIR_MIN_PERTURB = 0.15  # minimum absolute perturbation magnitude
REPAIR_MAX_PERTURB = 0.35  # maximum absolute perturbation magnitude
NUM_DIMS_TO_PERTURB = (2, 3)  # randomly pick 2 or 3 dimensions to mutate

DNA_KEYS = ["brightness", "warmth", "movement", "density", "space", "aggression"]

XLOW_THRESHOLD = 0.15
XHIGH_THRESHOLD = 0.85

random.seed(42)  # reproducible repair


# ---------------------------------------------------------------------------
# Step 1 — Scan
# ---------------------------------------------------------------------------

def load_preset(path: Path) -> Optional[dict]:
    """Load a .xometa JSON file; return None on parse failure."""
    try:
        with open(path, "r", encoding="utf-8") as fh:
            return json.load(fh)
    except Exception as exc:
        print(f"  [WARN] Could not parse {path.name}: {exc}")
        return None


def extract_dna(data: dict) -> Optional[List[float]]:
    """Return ordered 6-float DNA vector or None if incomplete."""
    dna = data.get("dna")
    if not isinstance(dna, dict):
        return None
    try:
        return [float(dna[k]) for k in DNA_KEYS]
    except (KeyError, TypeError, ValueError):
        return None


def scan_presets(presets_dir: Path) -> List[dict]:
    """
    Walk presets_dir recursively and collect dicts with:
      {path, name, dna_vec, raw_data}
    for every .xometa that has a valid DNA block.
    """
    records = []
    for xometa_path in sorted(presets_dir.rglob("*.xometa")):
        data = load_preset(xometa_path)
        if data is None:
            continue
        dna_vec = extract_dna(data)
        if dna_vec is None:
            continue
        records.append({
            "path": xometa_path,
            "name": data.get("name", xometa_path.stem),
            "dna_vec": dna_vec,
            "raw_data": data,
        })
    return records


# ---------------------------------------------------------------------------
# Step 2 — Identify clusters
# ---------------------------------------------------------------------------

def dna_within_threshold(vec_a: list[float], vec_b: list[float], threshold: float) -> bool:
    """Return True if ALL 6 dimensions are within ±threshold of each other."""
    return all(abs(a - b) <= threshold for a, b in zip(vec_a, vec_b))


def find_clusters(records: List[dict], threshold: float) -> List[List[int]]:
    """
    Single-linkage clustering: two presets are in the same cluster if their
    DNA vectors are within threshold on all 6 dimensions.

    Returns list of clusters (each cluster = list of record indices).
    Only clusters with 2+ members are returned.
    """
    n = len(records)
    cluster_id = list(range(n))  # union-find parent array (flat)

    def find(x):
        while cluster_id[x] != x:
            cluster_id[x] = cluster_id[cluster_id[x]]
            x = cluster_id[x]
        return x

    def union(x, y):
        rx, ry = find(x), find(y)
        if rx != ry:
            cluster_id[rx] = ry

    for i in range(n):
        for j in range(i + 1, n):
            if dna_within_threshold(records[i]["dna_vec"], records[j]["dna_vec"], threshold):
                union(i, j)

    # Group by root
    groups: Dict[int, List[int]] = {}
    for i in range(n):
        root = find(i)
        groups.setdefault(root, []).append(i)

    # Return only groups with 2+ members
    return [members for members in groups.values() if len(members) >= 2]


# ---------------------------------------------------------------------------
# Step 3 — Report
# ---------------------------------------------------------------------------

def report_clusters(clusters: list[list[int]], records: list[dict]) -> None:
    print(f"\n{'='*60}")
    print(f"  CLONE CLUSTER SCAN RESULTS")
    print(f"{'='*60}")
    print(f"  Total presets scanned : {len(records)}")
    print(f"  Clone clusters found  : {len(clusters)}")
    total_in_clusters = sum(len(c) for c in clusters)
    print(f"  Presets in clusters   : {total_in_clusters}")

    sizes = sorted(set(len(c) for c in clusters))
    for sz in sizes:
        count = sum(1 for c in clusters if len(c) == sz)
        print(f"    size {sz:2d} clusters: {count}")

    print()
    print("  Top problematic DNA vectors (anchor of each cluster):")
    print(f"  {'Name':<35}  {'bright':>6} {'warmth':>6} {'move':>6} {'dens':>6} {'space':>6} {'aggr':>6}  members")
    print(f"  {'-'*35}  {'------':>6} {'------':>6} {'------':>6} {'------':>6} {'------':>6} {'------':>6}  -------")
    for cluster in sorted(clusters, key=len, reverse=True)[:20]:
        anchor = records[cluster[0]]
        v = anchor["dna_vec"]
        name = anchor["name"][:35]
        print(f"  {name:<35}  {v[0]:6.3f} {v[1]:6.3f} {v[2]:6.3f} {v[3]:6.3f} {v[4]:6.3f} {v[5]:6.3f}  {len(cluster)}")
    print()


# ---------------------------------------------------------------------------
# Step 4 — Repair
# ---------------------------------------------------------------------------

def clamp(value: float, lo: float = 0.0, hi: float = 1.0) -> float:
    return max(lo, min(hi, value))


def perturb_toward_extreme(current: float) -> float:
    """
    Apply a random ±REPAIR perturbation, biased toward the nearest extreme
    (0.0 or 1.0) so that the result is more likely to hit XLOW/XHIGH.
    """
    magnitude = random.uniform(REPAIR_MIN_PERTURB, REPAIR_MAX_PERTURB)
    # Bias direction: if below midpoint push down, above push up
    if current < 0.5:
        direction = -1 if random.random() < 0.65 else 1
    else:
        direction = 1 if random.random() < 0.65 else -1
    return clamp(current + direction * magnitude)


def ensure_xlow_or_xhigh(dna_vec: List[float], dim_indices: List[int]) -> List[float]:
    """
    If none of the mutated dimensions ended up XLOW or XHIGH, force the
    dimension that is closest to an extreme to cross the threshold.
    """
    new_vec = dna_vec[:]
    any_extreme = any(
        new_vec[i] <= XLOW_THRESHOLD or new_vec[i] >= XHIGH_THRESHOLD
        for i in range(6)
    )
    if any_extreme:
        return new_vec

    # Find the dimension in dim_indices that is closest to either extreme
    def dist_to_extreme(v):
        return min(v, 1.0 - v)

    target_dim = min(dim_indices, key=lambda i: dist_to_extreme(new_vec[i]))
    val = new_vec[target_dim]
    if val < 0.5:
        new_vec[target_dim] = clamp(val - random.uniform(REPAIR_MIN_PERTURB, REPAIR_MAX_PERTURB))
        if new_vec[target_dim] > XLOW_THRESHOLD:
            new_vec[target_dim] = random.uniform(0.0, XLOW_THRESHOLD)
    else:
        new_vec[target_dim] = clamp(val + random.uniform(REPAIR_MIN_PERTURB, REPAIR_MAX_PERTURB))
        if new_vec[target_dim] < XHIGH_THRESHOLD:
            new_vec[target_dim] = random.uniform(XHIGH_THRESHOLD, 1.0)

    return new_vec


def mutate_dna(dna_vec: List[float]) -> List[float]:
    """
    Perturb 2–3 randomly chosen dimensions; guarantee at least one extreme.
    """
    num_to_perturb = random.randint(*NUM_DIMS_TO_PERTURB)
    dims = random.sample(range(6), num_to_perturb)

    new_vec = dna_vec[:]
    for d in dims:
        new_vec[d] = perturb_toward_extreme(new_vec[d])

    new_vec = ensure_xlow_or_xhigh(new_vec, dims)
    return [round(v, 3) for v in new_vec]


def repair_cluster(cluster: List[int], records: List[dict]) -> List[dict]:
    """
    For clusters with 3+ members, keep index 0 unchanged, mutate the rest.
    For 2-member clusters, no repair is performed (minor issue, not worth
    risking over-homogenisation; just reported).

    Returns a list of repair-record dicts (one per repaired preset).
    """
    if len(cluster) < 3:
        return []

    repairs = []
    for idx in cluster[1:]:  # skip anchor (cluster[0])
        rec = records[idx]
        old_vec = rec["dna_vec"][:]
        new_vec = mutate_dna(old_vec)

        # Patch the raw_data in-place
        data = rec["raw_data"]
        for ki, key in enumerate(DNA_KEYS):
            data["dna"][key] = new_vec[ki]

        # Write file
        with open(rec["path"], "w", encoding="utf-8") as fh:
            json.dump(data, fh, indent=2)
            fh.write("\n")

        repairs.append({
            "name": rec["name"],
            "path": str(rec["path"].relative_to(REPO_ROOT)),
            "dna_before": dict(zip(DNA_KEYS, old_vec)),
            "dna_after": dict(zip(DNA_KEYS, new_vec)),
        })
        # Update in-memory record so subsequent cluster checks see new value
        rec["dna_vec"] = new_vec

    return repairs


# ---------------------------------------------------------------------------
# Step 5 — Save report
# ---------------------------------------------------------------------------

def build_report(
    total_scanned: int,
    clusters: List[List[int]],
    records: List[dict],
    all_repairs: List[dict],
) -> dict:
    cluster_summaries = []
    for cluster in clusters:
        anchor = records[cluster[0]]
        members = [
            {"name": records[i]["name"], "path": str(records[i]["path"].relative_to(REPO_ROOT))}
            for i in cluster
        ]
        cluster_summaries.append({
            "anchor_dna": dict(zip(DNA_KEYS, anchor["dna_vec"])),
            "size": len(cluster),
            "repaired": len(cluster) >= 3,
            "members": members,
        })

    return {
        "scan_date": "2026-03-16",
        "presets_scanned": total_scanned,
        "cluster_threshold": CLUSTER_THRESHOLD,
        "clusters_found": len(clusters),
        "clusters_repaired": sum(1 for c in clusters if len(c) >= 3),
        "presets_repaired": len(all_repairs),
        "clusters": cluster_summaries,
        "repairs": all_repairs,
    }


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    print(f"XPN Clone Cluster Repair")
    print(f"Scanning: {PRESETS_DIR}")
    print(f"Threshold: ±{CLUSTER_THRESHOLD} on all 6 DNA dimensions")
    print()

    # 1. Scan
    records = scan_presets(PRESETS_DIR)
    print(f"Loaded {len(records)} presets with valid DNA vectors.")

    if not records:
        print("No presets found. Exiting.")
        sys.exit(0)

    # 2. Find clusters
    print("Identifying clone clusters...")
    clusters = find_clusters(records, CLUSTER_THRESHOLD)

    # 3. Report
    report_clusters(clusters, records)

    # 4. Repair
    all_repairs: List[dict] = []
    clusters_to_repair = [c for c in clusters if len(c) >= 3]
    print(f"Repairing {len(clusters_to_repair)} clusters (size ≥ 3)...")
    for cluster in clusters_to_repair:
        repairs = repair_cluster(cluster, records)
        all_repairs.extend(repairs)

    # 5. Print repair summary
    print(f"\n{'='*60}")
    print(f"  REPAIR SUMMARY")
    print(f"{'='*60}")
    print(f"  Clusters repaired : {len(clusters_to_repair)}")
    print(f"  Presets mutated   : {len(all_repairs)}")
    if all_repairs:
        print()
        print("  Repaired presets:")
        for r in all_repairs:
            before = list(r["dna_before"].values())
            after = list(r["dna_after"].values())
            b_str = " ".join(f"{v:.3f}" for v in before)
            a_str = " ".join(f"{v:.3f}" for v in after)
            print(f"    {r['name']:<40}  before [{b_str}]")
            print(f"    {'':40}  after  [{a_str}]")
    print()

    # 6. Save report
    REPORT_PATH.parent.mkdir(parents=True, exist_ok=True)
    report = build_report(len(records), clusters, records, all_repairs)
    with open(REPORT_PATH, "w", encoding="utf-8") as fh:
        json.dump(report, fh, indent=2)
        fh.write("\n")
    print(f"Report saved to: {REPORT_PATH.relative_to(REPO_ROOT)}")
    print()
    print(f"Done. {len(clusters)} cluster(s) found, {len(all_repairs)} preset(s) repaired.")


if __name__ == "__main__":
    main()
