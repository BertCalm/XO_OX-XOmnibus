#!/usr/bin/env python3
"""
xpn_preset_similarity_matrix.py
Compute pairwise cosine similarity across all .xometa presets using 6D Sonic DNA vectors.
Outputs ranked pairs, mood cluster density, and a 7×7 ASCII heatmap.

Usage:
    python Tools/xpn_preset_similarity_matrix.py
    python Tools/xpn_preset_similarity_matrix.py --top 20
    python Tools/xpn_preset_similarity_matrix.py --mood-filter Flux
    python Tools/xpn_preset_similarity_matrix.py --presets-dir /path/to/Presets/XOceanus
"""

import argparse
import json
import math
import os
import sys
from pathlib import Path
from typing import Dict, List, Optional, Tuple

# ─── Constants ────────────────────────────────────────────────────────────────

DNA_KEYS = ["brightness", "warmth", "movement", "density", "space", "aggression"]
MOODS = ["Foundation", "Atmosphere", "Entangled", "Prism", "Flux", "Aether", "Family"]
BLOCK_CHARS = " ░▒▓█"  # 5 levels: 0–0.2, 0.2–0.4, 0.4–0.6, 0.6–0.8, 0.8–1.0

# ─── DNA helpers ──────────────────────────────────────────────────────────────

def extract_dna(data: dict) -> Optional[List[float]]:
    """Return 6D vector from preset JSON, or None if missing/incomplete."""
    raw = data.get("dna") or data.get("sonic_dna")
    if not raw or not isinstance(raw, dict):
        return None
    vec = [float(raw.get(k, 0.0)) for k in DNA_KEYS]
    return vec if any(v != 0.0 for v in vec) else None


def cosine_similarity(a: list[float], b: list[float]) -> float:
    dot = sum(x * y for x, y in zip(a, b))
    mag_a = math.sqrt(sum(x * x for x in a))
    mag_b = math.sqrt(sum(y * y for y in b))
    if mag_a == 0.0 or mag_b == 0.0:
        return 0.0
    return dot / (mag_a * mag_b)


# ─── Scanning ─────────────────────────────────────────────────────────────────

def scan_presets(presets_dir: Path, mood_filter: Optional[str]) -> List[dict]:
    """Walk presets_dir for .xometa files, return list of preset records."""
    records = []
    skipped = 0
    for root, _dirs, files in os.walk(presets_dir):
        for fname in files:
            if not fname.endswith(".xometa"):
                continue
            fpath = Path(root) / fname
            try:
                with open(fpath, "r", encoding="utf-8") as fh:
                    data = json.load(fh)
            except (json.JSONDecodeError, OSError):
                skipped += 1
                continue

            dna = extract_dna(data)
            if dna is None:
                skipped += 1
                continue

            mood = data.get("mood", "Unknown")
            if mood_filter and mood.lower() != mood_filter.lower():
                continue

            records.append(
                {
                    "name": data.get("name", fname.replace(".xometa", "")),
                    "mood": mood,
                    "engines": data.get("engines", []),
                    "dna": dna,
                    "path": str(fpath),
                }
            )

    if skipped:
        print(f"  [scan] Skipped {skipped} files (missing DNA or parse error)")
    return records


# ─── Similarity matrix ────────────────────────────────────────────────────────

def compute_pairs(records: List[dict]) -> List[Tuple[float, int, int]]:
    """Return sorted list of (similarity, i, j) for all unique pairs."""
    n = len(records)
    pairs = []
    for i in range(n):
        for j in range(i + 1, n):
            sim = cosine_similarity(records[i]["dna"], records[j]["dna"])
            pairs.append((sim, i, j))
    pairs.sort(key=lambda x: x[0], reverse=True)
    return pairs


# ─── Mood density ─────────────────────────────────────────────────────────────

def mood_density(records: List[dict], pairs: List[Tuple[float, int, int]]) -> Dict[Tuple[str, str], List[float]]:
    """Average similarity for each mood-to-mood combination."""
    density: dict[tuple[str, str], list[float]] = {}
    for sim, i, j in pairs:
        mi = records[i]["mood"]
        mj = records[j]["mood"]
        key = (min(mi, mj), max(mi, mj))
        density.setdefault(key, []).append(sim)
    return density


# ─── ASCII heatmap ────────────────────────────────────────────────────────────

def block_char(value: float) -> str:
    idx = min(int(value * len(BLOCK_CHARS)), len(BLOCK_CHARS) - 1)
    return BLOCK_CHARS[idx]


def render_heatmap(records: List[dict], pairs: List[Tuple[float, int, int]], moods_present: List[str]) -> str:
    # Build mood→indices map
    mood_idx: Dict[str, List[int]] = {}
    for idx, rec in enumerate(records):
        mood_idx.setdefault(rec["mood"], []).append(idx)

    # Build fast lookup: (min_i, max_j) → sim
    sim_lookup: Dict[Tuple[int, int], float] = {(i, j): s for s, i, j in pairs}

    # Compute average sim for each mood pair (including self-pairs)
    avg: Dict[Tuple[str, str], float] = {}
    for m1 in moods_present:
        for m2 in moods_present:
            indices1 = mood_idx.get(m1, [])
            indices2 = mood_idx.get(m2, [])
            sims = []
            for a in indices1:
                for b in indices2:
                    if a == b:
                        sims.append(1.0)
                    else:
                        key = (min(a, b), max(a, b))
                        if key in sim_lookup:
                            sims.append(sim_lookup[key])
            avg[(m1, m2)] = (sum(sims) / len(sims)) if sims else 0.0

    # Render
    col_w = 12
    label_w = 11

    header = " " * label_w + "  "
    header += "  ".join(m[:col_w].ljust(col_w) for m in moods_present)
    lines = [header]

    for m1 in moods_present:
        row = m1[:label_w].ljust(label_w) + " |"
        for m2 in moods_present:
            val = avg.get((m1, m2), 0.0)
            cell = f" {block_char(val)}{val:.2f} "
            row += cell.ljust(col_w + 2)
        lines.append(row)

    # Legend
    lines.append("")
    lines.append("Legend: " + "  ".join(f"{c}={i*0.2:.1f}+" for i, c in enumerate(BLOCK_CHARS)))
    return "\n".join(lines)


# ─── CLI ──────────────────────────────────────────────────────────────────────

def parse_args() -> argparse.Namespace:
    repo_root = Path(__file__).resolve().parent.parent
    default_presets = repo_root / "Presets" / "XOceanus"

    p = argparse.ArgumentParser(
        description="Compute pairwise Sonic DNA similarity matrix across .xometa presets."
    )
    p.add_argument(
        "--presets-dir",
        type=Path,
        default=default_presets,
        help=f"Root directory to scan for .xometa files (default: {default_presets})",
    )
    p.add_argument(
        "--top",
        type=int,
        default=10,
        metavar="N",
        help="Number of most similar / most dissimilar pairs to show (default: 10)",
    )
    p.add_argument(
        "--mood-filter",
        type=str,
        default=None,
        metavar="MOOD",
        help="Only include presets whose mood matches this string (case-insensitive)",
    )
    return p.parse_args()


# ─── Main ─────────────────────────────────────────────────────────────────────

def main() -> int:
    args = parse_args()

    if not args.presets_dir.is_dir():
        print(f"ERROR: Presets directory not found: {args.presets_dir}", file=sys.stderr)
        return 1

    # ── Scan
    print(f"\nScanning: {args.presets_dir}")
    if args.mood_filter:
        print(f"  Mood filter: {args.mood_filter}")
    records = scan_presets(args.presets_dir, args.mood_filter)
    print(f"  Loaded {len(records)} presets with valid DNA vectors")

    if len(records) < 2:
        print("ERROR: Need at least 2 presets to compute similarity.", file=sys.stderr)
        return 1

    # ── Compute pairs
    print(f"\nComputing {len(records) * (len(records) - 1) // 2:,} pairwise similarities …")
    pairs = compute_pairs(records)

    # ── Top N most similar
    top_n = min(args.top, len(pairs))
    print(f"\n{'═' * 70}")
    print(f"  TOP {top_n} MOST SIMILAR PAIRS")
    print(f"{'═' * 70}")
    for rank, (sim, i, j) in enumerate(pairs[:top_n], 1):
        a, b = records[i], records[j]
        engines_a = "/".join(a["engines"]) or "?"
        engines_b = "/".join(b["engines"]) or "?"
        print(
            f"  {rank:>3}. {sim:.4f}  [{a['mood']}] {a['name']} ({engines_a})"
        )
        print(
            f"          ↕        [{b['mood']}] {b['name']} ({engines_b})"
        )
        dna_a = dict(zip(DNA_KEYS, a["dna"]))
        dna_b = dict(zip(DNA_KEYS, b["dna"]))
        dna_str = "  DNA A: " + "  ".join(f"{k[0].upper()}={dna_a[k]:.2f}" for k in DNA_KEYS)
        print(dna_str)
        dna_str = "  DNA B: " + "  ".join(f"{k[0].upper()}={dna_b[k]:.2f}" for k in DNA_KEYS)
        print(dna_str)
        print()

    # ── Top N most dissimilar
    print(f"{'═' * 70}")
    print(f"  TOP {top_n} MOST DISSIMILAR PAIRS")
    print(f"{'═' * 70}")
    for rank, (sim, i, j) in enumerate(reversed(pairs[-top_n:]), 1):
        a, b = records[i], records[j]
        engines_a = "/".join(a["engines"]) or "?"
        engines_b = "/".join(b["engines"]) or "?"
        print(
            f"  {rank:>3}. {sim:.4f}  [{a['mood']}] {a['name']} ({engines_a})"
        )
        print(
            f"          ↕        [{b['mood']}] {b['name']} ({engines_b})"
        )

    # ── Per-mood density
    density = mood_density(records, pairs)
    mood_counts: dict[str, int] = {}
    for rec in records:
        mood_counts[rec["mood"]] = mood_counts.get(rec["mood"], 0) + 1

    moods_present = sorted(mood_counts.keys(), key=lambda m: MOODS.index(m) if m in MOODS else 99)

    print(f"\n{'═' * 70}")
    print("  PER-MOOD SIMILARITY DENSITY")
    print(f"{'═' * 70}")
    print(f"  {'Mood':<14} {'Count':>6}  {'Avg intra-mood sim':>20}  {'Min':>6}  {'Max':>6}")
    print(f"  {'-'*14} {'-'*6}  {'-'*20}  {'-'*6}  {'-'*6}")
    for mood in moods_present:
        key_self = (mood, mood)
        sims = density.get(key_self, [])
        count = mood_counts[mood]
        if sims:
            avg_sim = sum(sims) / len(sims)
            min_sim = min(sims)
            max_sim = max(sims)
            bar = block_char(avg_sim) * int(avg_sim * 20)
            print(
                f"  {mood:<14} {count:>6}  {avg_sim:>8.4f}  {bar:<12}  {min_sim:>6.4f}  {max_sim:>6.4f}"
            )
        else:
            print(f"  {mood:<14} {count:>6}  {'(single preset — no pairs)':>38}")

    # ── Cross-mood density
    print(f"\n  Cross-mood average similarities (top 10 cross-mood pairs):")
    cross = []
    for (m1, m2), sims in density.items():
        if m1 != m2:
            cross.append((sum(sims) / len(sims), m1, m2, len(sims)))
    cross.sort(reverse=True)
    for avg_s, m1, m2, cnt in cross[:10]:
        print(f"    {avg_s:.4f}  {m1} ↔ {m2}  ({cnt} pairs)")

    # ── ASCII Heatmap
    print(f"\n{'═' * 70}")
    print("  MOOD × MOOD SIMILARITY HEATMAP  (avg cosine similarity)")
    print(f"{'═' * 70}")
    print(render_heatmap(records, pairs, moods_present))

    # ── Summary stats
    all_sims = [s for s, _, _ in pairs]
    global_avg = sum(all_sims) / len(all_sims)
    global_min = min(all_sims)
    global_max = max(all_sims)
    # Std dev
    var = sum((s - global_avg) ** 2 for s in all_sims) / len(all_sims)
    global_std = math.sqrt(var)

    print(f"\n{'═' * 70}")
    print("  GLOBAL STATISTICS")
    print(f"{'═' * 70}")
    print(f"  Total presets analysed : {len(records):,}")
    print(f"  Total pairs computed   : {len(pairs):,}")
    print(f"  Global avg similarity  : {global_avg:.4f}")
    print(f"  Global std dev         : {global_std:.4f}")
    print(f"  Min similarity         : {global_min:.4f}")
    print(f"  Max similarity         : {global_max:.4f}")

    # Diversity score: lower avg → more diverse fleet
    diversity = 1.0 - global_avg
    bar = block_char(diversity) * int(diversity * 20)
    print(f"  Fleet diversity score  : {diversity:.4f}  {bar}")
    print()

    return 0


if __name__ == "__main__":
    sys.exit(main())
