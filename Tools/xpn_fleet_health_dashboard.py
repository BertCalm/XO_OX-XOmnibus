#!/usr/bin/env python3
"""
xpn_fleet_health_dashboard.py
Combined fleet health dashboard for XOlokun.

Performs three analyses in a single pass:
  1. Coupling coverage — scans Entangled presets for engine pair coverage
  2. Mood distribution — counts presets across all 7 mood directories
  3. DNA diversity    — cosine-distance score + dimension compression %

Writes a combined JSON to Docs/snapshots/fleet_health_latest.json
Prints a clean dashboard to stdout.

Usage: python3 Tools/xpn_fleet_health_dashboard.py

Stdlib only: json, os, sys, math, random, statistics, datetime, itertools
"""

import json
import math
import os
import random
import statistics
import sys
from datetime import datetime, timezone
from itertools import combinations

# ─── Configuration ────────────────────────────────────────────────────────────

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PRESETS_ROOT = os.path.join(REPO_ROOT, "Presets", "XOlokun")
ENTANGLED_DIR = os.path.join(PRESETS_ROOT, "Entangled")
SNAPSHOT_PATH = os.path.join(REPO_ROOT, "Docs", "snapshots", "fleet_health_latest.json")

MOODS = ["Foundation", "Atmosphere", "Entangled", "Prism", "Flux", "Aether", "Family"]

DNA_DIMS = ["brightness", "warmth", "movement", "density", "space", "aggression"]

MIDRANGE_LOW  = 0.35
MIDRANGE_HIGH = 0.65

# Diversity score thresholds for HEALTH RATING
DIVERSITY_GOOD     = 0.35
DIVERSITY_FAIR     = 0.28
# Coverage thresholds (%)
COVERAGE_GOOD      = 40.0
COVERAGE_FAIR      = 20.0

SAMPLE_PAIRS = 300

# ─── Engine registry ──────────────────────────────────────────────────────────

ENGINES = [
    "ODDFELIX", "ODDOSCAR", "OVERDUB", "ODYSSEY", "OBLONG", "OBESE",
    "ONSET", "OVERWORLD", "OPAL", "ORBITAL", "ORGANON", "OUROBOROS",
    "OBSIDIAN", "OVERBITE", "ORIGAMI", "ORACLE", "OBSCURA", "OCEANIC",
    "OCELOT", "OPTIC", "OBLIQUE", "OSPREY", "OSTERIA", "OWLFISH",
    "OHM", "ORPHICA", "OBBLIGATO", "OTTONI", "OLE", "OVERLAP",
    "OUTWIT", "OMBRE", "ORCA", "OCTOPUS",
]

TOTAL_POSSIBLE = len(ENGINES) * (len(ENGINES) - 1) // 2  # C(34,2) = 561

# ─── Alias resolution ─────────────────────────────────────────────────────────

ALIASES: dict[str, str] = {}
for _e in ENGINES:
    ALIASES[_e] = _e
    ALIASES[_e.lower()] = _e

_LEGACY: dict[str, str] = {
    # Short / camelCase / XO-prefixed legacy forms
    "Snap": "ODDFELIX",      "snap": "ODDFELIX",
    "OddfeliX": "ODDFELIX",
    "Morph": "ODDOSCAR",     "morph": "ODDOSCAR",
    "OddOscar": "ODDOSCAR",
    "Dub": "OVERDUB",        "dub": "OVERDUB",
    "XOverdub": "OVERDUB",
    "Drift": "ODYSSEY",      "drift": "ODYSSEY",
    "Odyssey": "ODYSSEY",
    "Bob": "OBLONG",         "bob": "OBLONG",
    "Oblong": "OBLONG",      "XOblong": "OBLONG",
    "Fat": "OBESE",          "fat": "OBESE",
    "Obese": "OBESE",        "XObese": "OBESE",
    "Bite": "OVERBITE",      "bite": "OVERBITE",
    "Overbite": "OVERBITE",  "XOverbite": "OVERBITE",
    "Onset": "ONSET",        "XOnset": "ONSET",
    "Overworld": "OVERWORLD","XOverworld": "OVERWORLD",
    "Opal": "OPAL",
    "Orbital": "ORBITAL",
    "Organon": "ORGANON",
    "Ouroboros": "OUROBOROS","XOuroboros": "OUROBOROS",
    "Obsidian": "OBSIDIAN",
    "Origami": "ORIGAMI",    "XOrigami": "ORIGAMI",
    "Oracle": "ORACLE",      "XOracle": "ORACLE",
    "Obscura": "OBSCURA",
    "Oceanic": "OCEANIC",
    "Ocelot": "OCELOT",
    "Optic": "OPTIC",
    "Oblique": "OBLIQUE",    "XOblique": "OBLIQUE",
    "Osprey": "OSPREY",
    "Osteria": "OSTERIA",    "XOsteria": "OSTERIA",
    "Owlfish": "OWLFISH",    "XOwlfish": "OWLFISH",
    "Ohm": "OHM",            "XOhm": "OHM",
    "Orphica": "ORPHICA",
    "Obbligato": "OBBLIGATO",
    "Ottoni": "OTTONI",      "XOttoni": "OTTONI",
    "Ole": "OLE",
    "Overlap": "OVERLAP",
    "Outwit": "OUTWIT",
    "Ombre": "OMBRE",
    "Orca": "ORCA",
    "Octopus": "OCTOPUS",
    # Data artifact
    "OVERDRIVE": "OVERDUB",  "Overdrive": "OVERDUB",
}
ALIASES.update(_LEGACY)


def resolve(name: str):
    """Return canonical engine name, or None if unrecognised."""
    return ALIASES.get(name) or ALIASES.get(name.upper())


# ─── 1. Coupling coverage ─────────────────────────────────────────────────────

def scan_coupling() -> dict:
    """
    Scan Entangled presets and return coupling coverage stats.
    Returns a dict with keys:
        covered_pairs, per_engine_counts, preset_count, unresolved
    """
    covered_pairs: set[tuple[str, str]] = set()
    per_engine: dict[str, set[str]] = {e: set() for e in ENGINES}
    unresolved: list[str] = []
    preset_count = 0

    if not os.path.isdir(ENTANGLED_DIR):
        return {
            "covered_pairs": covered_pairs,
            "per_engine_counts": {e: 0 for e in ENGINES},
            "preset_count": 0,
            "unresolved": [],
            "error": f"Entangled directory not found: {ENTANGLED_DIR}",
        }

    for fname in sorted(os.listdir(ENTANGLED_DIR)):
        if not fname.endswith(".xometa"):
            continue
        fpath = os.path.join(ENTANGLED_DIR, fname)
        try:
            with open(fpath, encoding="utf-8") as fh:
                data = json.load(fh)
        except (json.JSONDecodeError, OSError) as exc:
            print(f"  [WARN] Could not read {fname}: {exc}", file=sys.stderr)
            continue

        preset_count += 1
        raw_engines: list[str] = data.get("engines", [])
        canonical: list[str] = []
        for raw in raw_engines:
            c = resolve(raw)
            if c is None:
                if raw not in unresolved:
                    unresolved.append(raw)
            else:
                canonical.append(c)

        canonical = list(dict.fromkeys(canonical))  # deduplicate, preserve order
        for a, b in combinations(canonical, 2):
            pair = (min(a, b), max(a, b))
            covered_pairs.add(pair)
            per_engine[a].add(b)
            per_engine[b].add(a)

    return {
        "covered_pairs": covered_pairs,
        "per_engine_counts": {e: len(partners) for e, partners in per_engine.items()},
        "preset_count": preset_count,
        "unresolved": unresolved,
    }


# ─── 2. Mood distribution ─────────────────────────────────────────────────────

def scan_mood_counts() -> dict[str, int]:
    """Count .xometa files per mood directory."""
    counts: dict[str, int] = {}
    for mood in MOODS:
        mood_dir = os.path.join(PRESETS_ROOT, mood)
        if not os.path.isdir(mood_dir):
            counts[mood] = 0
            continue
        counts[mood] = sum(
            1 for f in os.listdir(mood_dir) if f.endswith(".xometa")
        )
    return counts


# ─── 3. DNA diversity ─────────────────────────────────────────────────────────

def load_dna_vectors() -> tuple[list[list[float]], int]:
    """
    Walk all mood directories, extract sonic_dna / dna vectors.
    Returns (vectors, skipped_count).
    """
    vectors: list[list[float]] = []
    skipped = 0

    for mood in MOODS:
        mood_dir = os.path.join(PRESETS_ROOT, mood)
        if not os.path.isdir(mood_dir):
            continue
        for fname in sorted(os.listdir(mood_dir)):
            if not fname.endswith(".xometa"):
                continue
            fpath = os.path.join(mood_dir, fname)
            try:
                with open(fpath, encoding="utf-8") as fh:
                    data = json.load(fh)
            except (json.JSONDecodeError, OSError):
                skipped += 1
                continue

            raw_dna = data.get("sonic_dna") or data.get("dna")
            if not raw_dna:
                skipped += 1
                continue
            if not all(dim in raw_dna for dim in DNA_DIMS):
                skipped += 1
                continue
            try:
                vec = [float(raw_dna[dim]) for dim in DNA_DIMS]
            except (TypeError, ValueError):
                skipped += 1
                continue

            vectors.append(vec)

    return vectors, skipped


def cosine_distance(a: list[float], b: list[float]) -> float:
    dot = sum(x * y for x, y in zip(a, b))
    mag_a = math.sqrt(sum(x * x for x in a))
    mag_b = math.sqrt(sum(x * x for x in b))
    if mag_a == 0.0 or mag_b == 0.0:
        return 1.0
    return 1.0 - (dot / (mag_a * mag_b))


def diversity_score(vectors: list[list[float]], n_pairs: int = SAMPLE_PAIRS) -> float:
    n = len(vectors)
    if n < 2:
        return 0.0
    max_possible = n * (n - 1) // 2
    if max_possible <= n_pairs:
        pairs = [(i, j) for i in range(n) for j in range(i + 1, n)]
    else:
        seen: set[tuple[int, int]] = set()
        pairs = []
        attempts = 0
        while len(pairs) < n_pairs and attempts < n_pairs * 10:
            i = random.randrange(n)
            j = random.randrange(n)
            if i != j:
                key = (min(i, j), max(i, j))
                if key not in seen:
                    seen.add(key)
                    pairs.append(key)
            attempts += 1
    if not pairs:
        return 0.0
    total = sum(cosine_distance(vectors[i], vectors[j]) for i, j in pairs)
    return total / len(pairs)


def compression_by_dim(vectors: list[list[float]]) -> dict[str, float]:
    """Return midrange % (0.35–0.65) for each DNA dimension."""
    n = len(vectors)
    if n == 0:
        return {dim: 0.0 for dim in DNA_DIMS}
    result = {}
    for idx, dim in enumerate(DNA_DIMS):
        vals = [v[idx] for v in vectors]
        mid_count = sum(1 for x in vals if MIDRANGE_LOW <= x <= MIDRANGE_HIGH)
        result[dim] = mid_count / n
    return result


# ─── Health rating ─────────────────────────────────────────────────────────────

def health_rating(div_score: float, coverage_pct: float) -> str:
    """
    GOOD  : diversity >= 0.35 AND coverage >= 40%
    FAIR  : diversity >= 0.28 AND coverage >= 20%
    CRITICAL: anything below FAIR thresholds
    """
    if div_score >= DIVERSITY_GOOD and coverage_pct >= COVERAGE_GOOD:
        return "GOOD"
    if div_score >= DIVERSITY_FAIR and coverage_pct >= COVERAGE_FAIR:
        return "FAIR"
    return "CRITICAL"


# ─── Dashboard printing ────────────────────────────────────────────────────────

BORDER = "═" * 39

def print_dashboard(
    mood_counts: dict[str, int],
    total_presets: int,
    coupling: dict,
    div: float,
    compression: dict[str, float],
    rating: str,
) -> None:
    covered_count = len(coupling["covered_pairs"])
    coverage_pct  = covered_count / TOTAL_POSSIBLE * 100

    per_engine_counts = coupling["per_engine_counts"]
    sorted_by_partners = sorted(per_engine_counts.items(), key=lambda x: x[1], reverse=True)
    most_engine,  most_count  = sorted_by_partners[0]
    least_engine, least_count = sorted_by_partners[-1]

    compressed_dims = {d: v for d, v in compression.items() if v > 0.60}

    print(BORDER)
    print("   XO_OX FLEET HEALTH DASHBOARD")
    print(BORDER)
    print(f"Total presets: {total_presets} (across {len(MOODS)} moods)")
    print()
    print("MOOD DISTRIBUTION:")
    for mood in MOODS:
        count = mood_counts.get(mood, 0)
        label = f"{mood}:"
        print(f"  {label:<12} {count}")
    print()
    print(f"COUPLING COVERAGE: {covered_count}/{TOTAL_POSSIBLE} ({coverage_pct:.1f}%)")
    print(f"  Most coupled:  {most_engine} ({most_count} partners)")
    print(f"  Least coupled: {least_engine} ({least_count} partners)")
    print()
    print(f"DNA DIVERSITY SCORE: {div:.3f}")
    if compressed_dims:
        print(f"  Compressed dims (>60% midrange):")
        for dim, pct in sorted(compressed_dims.items(), key=lambda x: -x[1]):
            print(f"    {dim + ':':<14} {pct * 100:.1f}%")
    else:
        print("  No dimensions compressed above 60% midrange.")
    print()
    print(f"HEALTH RATING: [{rating}]")
    print(BORDER)


# ─── JSON snapshot ─────────────────────────────────────────────────────────────

def build_snapshot(
    mood_counts: dict[str, int],
    total_presets: int,
    coupling: dict,
    div: float,
    compression: dict[str, float],
    rating: str,
    generated_at: str,
) -> dict:
    covered_count = len(coupling["covered_pairs"])
    coverage_pct  = covered_count / TOTAL_POSSIBLE * 100
    per_counts = coupling["per_engine_counts"]
    sorted_engines = sorted(per_counts.items(), key=lambda x: x[1], reverse=True)

    return {
        "generated_at": generated_at,
        "health_rating": rating,
        "total_presets": total_presets,
        "mood_distribution": mood_counts,
        "coupling_coverage": {
            "covered_pairs": covered_count,
            "total_possible": TOTAL_POSSIBLE,
            "coverage_pct": round(coverage_pct, 2),
            "entangled_preset_count": coupling["preset_count"],
            "per_engine_partner_counts": {e: c for e, c in sorted(per_counts.items())},
            "most_coupled_engine": sorted_engines[0][0],
            "most_coupled_count": sorted_engines[0][1],
            "least_coupled_engine": sorted_engines[-1][0],
            "least_coupled_count": sorted_engines[-1][1],
            "unresolved_engine_names": sorted(coupling.get("unresolved", [])),
        },
        "dna_diversity": {
            "score": round(div, 4),
            "sample_pairs": SAMPLE_PAIRS,
            "dimension_compression": {
                dim: round(pct, 4) for dim, pct in compression.items()
            },
            "compressed_dims_above_60pct": [
                d for d, v in compression.items() if v > 0.60
            ],
        },
    }


# ─── Entry point ───────────────────────────────────────────────────────────────

def main() -> None:
    random.seed(42)
    generated_at = datetime.now(timezone.utc).isoformat()

    # 1. Mood counts
    mood_counts = scan_mood_counts()
    total_presets = sum(mood_counts.values())

    # 2. Coupling coverage
    coupling = scan_coupling()
    if "error" in coupling:
        print(f"[WARN] {coupling['error']}", file=sys.stderr)

    # 3. DNA diversity
    vectors, _skipped = load_dna_vectors()
    div = diversity_score(vectors)
    compression = compression_by_dim(vectors)

    # 4. Health rating
    covered_count = len(coupling["covered_pairs"])
    coverage_pct  = covered_count / TOTAL_POSSIBLE * 100
    rating = health_rating(div, coverage_pct)

    # 5. Print dashboard
    print_dashboard(mood_counts, total_presets, coupling, div, compression, rating)

    # 6. Write JSON snapshot
    os.makedirs(os.path.dirname(SNAPSHOT_PATH), exist_ok=True)
    snapshot = build_snapshot(
        mood_counts, total_presets, coupling, div, compression, rating, generated_at
    )
    with open(SNAPSHOT_PATH, "w", encoding="utf-8") as fh:
        json.dump(snapshot, fh, indent=2)
    print(f"\nJSON snapshot written → {SNAPSHOT_PATH}")


if __name__ == "__main__":
    main()
