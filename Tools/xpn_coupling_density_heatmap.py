#!/usr/bin/env python3
"""
xpn_coupling_density_heatmap.py

Visualize coupling pair coverage across all 34 XO_OX engines as an ASCII heatmap.
Shows which engine×engine combinations have Entangled presets and which are empty.

Usage:
    python xpn_coupling_density_heatmap.py [--presets-dir PATH] [--engine-filter NAME]
"""

import json
import os
import argparse
import pathlib
from collections import defaultdict

# Canonical 34 engine short names and their full names (as they appear in .xometa files)
ENGINES = [
    ("ODDFELIX",    "OddfeliX"),
    ("ODDOSCAR",    "OddOscar"),
    ("OVERDUB",     "Overdub"),
    ("ODYSSEY",     "Odyssey"),
    ("OBLONG",      "Oblong"),
    ("OBESE",       "Obese"),
    ("ONSET",       "Onset"),
    ("OVERWORLD",   "Overworld"),
    ("OPAL",        "Opal"),
    ("ORBITAL",     "Orbital"),
    ("ORGANON",     "Organon"),
    ("OUROBOROS",   "Ouroboros"),
    ("OBSIDIAN",    "Obsidian"),
    ("OVERBITE",    "Overbite"),
    ("ORIGAMI",     "Origami"),
    ("ORACLE",      "Oracle"),
    ("OBSCURA",     "Obscura"),
    ("OCEANIC",     "Oceanic"),
    ("OCELOT",      "Ocelot"),
    ("OPTIC",       "Optic"),
    ("OBLIQUE",     "Oblique"),
    ("OSPREY",      "Osprey"),
    ("OSTERIA",     "Osteria"),
    ("OWLFISH",     "XOwlfish"),
    ("OHM",         "Ohm"),
    ("ORPHICA",     "Orphica"),
    ("OBBLIGATO",   "Obbligato"),
    ("OTTONI",      "Ottoni"),
    ("OLE",         "Ole"),
    ("OVERLAP",     "Overlap"),
    ("OUTWIT",      "Outwit"),
    ("OMBRE",       "Ombre"),
    ("ORCA",        "Orca"),
    ("OCTOPUS",     "Octopus"),
]

# Canonical name -> short name lookup (case-insensitive)
def build_lookup(engines):
    lookup = {}
    for short, full in engines:
        lookup[full.lower()] = short
        lookup[short.lower()] = short
    return lookup

# Engine family groupings for "recommended pairs" suggestions
ENGINE_FAMILIES = {
    "VOICE":       ["ODDFELIX", "ODDOSCAR", "ODYSSEY", "OVERDUB", "OVERBITE", "OBLIQUE"],
    "RHYTHM":      ["ONSET", "OBLONG", "OBESE", "OSTERIA", "OSPREY"],
    "TEXTURE":     ["OPAL", "OCELOT", "OMBRE", "ORIGAMI", "OCEANIC", "OBSCURA"],
    "HARMONIC":    ["ORBITAL", "ORGANON", "OUROBOROS", "OBSIDIAN", "OHM", "ORPHICA"],
    "WORLD":       ["OBBLIGATO", "OTTONI", "OLE", "OVERWORLD", "ORACLE"],
    "EVOLVING":    ["OVERLAP", "OUTWIT", "OCTOPUS", "ORCA", "OPTIC", "OWLFISH"],
}


def norm_engine_name(name: str, lookup: dict):
    """Return canonical short name for an engine name, or None if unknown."""
    return lookup.get(name.lower())


def scan_presets(presets_dir: pathlib.Path, lookup: dict) -> dict:
    """
    Scan all .xometa files in presets_dir.
    Return dict: frozenset({shortA, shortB}) -> count of presets containing that pair.
    Only counts files with 2+ engines.
    """
    pair_counts = defaultdict(int)
    skipped = 0

    for meta_file in presets_dir.glob("*.xometa"):
        try:
            with meta_file.open("r", encoding="utf-8") as fh:
                data = json.load(fh)
        except (json.JSONDecodeError, OSError):
            skipped += 1
            continue

        raw_engines = data.get("engines", [])
        if len(raw_engines) < 2:
            continue

        # Resolve to canonical short names
        resolved = []
        for raw in raw_engines:
            short = norm_engine_name(str(raw), lookup)
            if short and short not in resolved:
                resolved.append(short)

        if len(resolved) < 2:
            continue

        # Count every unique pair in this preset
        for i in range(len(resolved)):
            for j in range(i + 1, len(resolved)):
                pair = frozenset({resolved[i], resolved[j]})
                pair_counts[pair] += 1

    return dict(pair_counts), skipped


def build_matrix(engines, pair_counts):
    """Build NxN count matrix from pair_counts dict."""
    short_names = [s for s, _ in engines]
    idx = {s: i for i, s in enumerate(short_names)}
    n = len(short_names)
    matrix = [[0] * n for _ in range(n)]

    for pair, count in pair_counts.items():
        pair_list = list(pair)
        if len(pair_list) == 2:
            a, b = pair_list
            if a in idx and b in idx:
                i, j = idx[a], idx[b]
                matrix[i][j] = count
                matrix[j][i] = count

    return matrix, short_names


DENSITY_CHARS = {
    0:          "·",   # empty
    (1, 2):     "░",   # sparse
    (3, 5):     "▒",   # moderate
    (6, 10):    "▓",   # dense
}
DENSE_CHAR = "█"       # 6+


def count_to_char(count: int) -> str:
    if count == 0:
        return "·"
    elif count <= 2:
        return "░"
    elif count <= 5:
        return "▒"
    elif count <= 10:
        return "▓"
    else:
        return "█"


def abbreviate(name: str, width: int = 6) -> str:
    return name[:width].ljust(width)


def print_heatmap(matrix, short_names, engine_filter=None):
    n = len(short_names)

    # If engine_filter, determine which indices to show
    if engine_filter:
        ef = engine_filter.upper()
        show_indices = [i for i, s in enumerate(short_names) if ef in s]
        if not show_indices:
            print(f"  [!] No engines matched filter '{engine_filter}'. Showing all.")
            show_indices = list(range(n))
    else:
        show_indices = list(range(n))

    visible_names = [short_names[i] for i in show_indices]
    col_w = 7

    # Column header (abbreviated, rotated-style using truncation)
    header_top = " " * 9
    for name in visible_names:
        header_top += abbreviate(name, col_w - 1) + " "
    print(header_top)
    print(" " * 9 + ("─" * col_w) * len(visible_names))

    for ri in show_indices:
        row_label = abbreviate(short_names[ri], 8)
        row_str = f"{row_label} │"
        for ci in show_indices:
            count = matrix[ri][ci]
            if ri == ci:
                cell = "  ██  "  # diagonal (self)
            else:
                ch = count_to_char(count)
                cell = f"  {ch}{ch}  "
            row_str += cell[:col_w]
        print(row_str)

    print()
    print("  Legend:  ·· = 0 presets   ░░ = 1-2   ▒▒ = 3-5   ▓▓ = 6-10   ██ = 11+")
    print("           ██ (diagonal) = same-engine (N/A)")


def compute_summary(matrix, short_names, pair_counts):
    n = len(short_names)
    total_possible = n * (n - 1) // 2
    covered = sum(1 for v in pair_counts.values() if v > 0)
    uncovered = total_possible - covered

    # Per-engine coupling score
    engine_totals = defaultdict(int)
    engine_pairs  = defaultdict(int)
    for pair, count in pair_counts.items():
        pair_list = list(pair)
        for e in pair_list:
            engine_totals[e] += count
            engine_pairs[e]  += 1

    if engine_totals:
        most_coupled   = max(engine_totals, key=engine_totals.get)
        least_name = min(
            short_names,
            key=lambda s: (engine_totals.get(s, 0), s)
        )
    else:
        most_coupled = "N/A"
        least_name   = "N/A"

    return {
        "total_possible": total_possible,
        "covered": covered,
        "uncovered": uncovered,
        "most_coupled": most_coupled,
        "most_coupled_count": engine_totals.get(most_coupled, 0),
        "least_coupled": least_name,
        "least_coupled_count": engine_totals.get(least_name, 0),
        "engine_totals": engine_totals,
        "engine_pairs": engine_pairs,
    }


def top_richest_pairs(pair_counts, n=10):
    sorted_pairs = sorted(pair_counts.items(), key=lambda x: x[1], reverse=True)
    return sorted_pairs[:n]


def top_empty_recommended(pair_counts, engines, n=10):
    """
    Suggest engine pairs that are empty but belong to complementary families.
    Complementary = different families (cross-family coupling is usually more interesting).
    """
    short_names = [s for s, _ in engines]
    # Build engine -> family map
    engine_family = {}
    for family, members in ENGINE_FAMILIES.items():
        for m in members:
            engine_family[m] = family

    candidates = []
    for i in range(len(short_names)):
        for j in range(i + 1, len(short_names)):
            a, b = short_names[i], short_names[j]
            pair = frozenset({a, b})
            if pair_counts.get(pair, 0) == 0:
                fa = engine_family.get(a, "OTHER")
                fb = engine_family.get(b, "OTHER")
                # Prefer cross-family
                cross = fa != fb
                candidates.append((a, b, cross, fa, fb))

    # Sort: cross-family first, then alphabetical
    candidates.sort(key=lambda x: (not x[2], x[0], x[1]))
    return candidates[:n]


def main():
    parser = argparse.ArgumentParser(
        description="ASCII heatmap of XO_OX Entangled coupling pair coverage across 34 engines."
    )
    parser.add_argument(
        "--presets-dir",
        default=None,
        help="Path to Presets/XOceanus/Entangled/ directory. Auto-detected if omitted.",
    )
    parser.add_argument(
        "--engine-filter",
        default=None,
        help="Show only rows/cols containing this engine name (e.g. OPAL or opal).",
    )
    args = parser.parse_args()

    # Resolve presets dir
    if args.presets_dir:
        presets_dir = pathlib.Path(args.presets_dir)
    else:
        script_dir = pathlib.Path(__file__).resolve().parent
        repo_root  = script_dir.parent
        presets_dir = repo_root / "Presets" / "XOceanus" / "Entangled"

    if not presets_dir.exists():
        print(f"[ERROR] Presets directory not found: {presets_dir}")
        raise SystemExit(1)

    lookup = build_lookup(ENGINES)

    print(f"\n  XO_OX Coupling Density Heatmap")
    print(f"  Scanning: {presets_dir}")
    print()

    pair_counts, skipped = scan_presets(presets_dir, lookup)
    matrix, short_names  = build_matrix(ENGINES, pair_counts)

    # Print heatmap
    print_heatmap(matrix, short_names, engine_filter=args.engine_filter)

    # Summary stats
    summary = compute_summary(matrix, short_names, pair_counts)

    print("  ─" * 40)
    print(f"\n  SUMMARY")
    print(f"  Total possible pairs   : {summary['total_possible']}")
    print(f"  Covered pairs          : {summary['covered']}  ({summary['covered']/summary['total_possible']*100:.1f}%)")
    print(f"  Uncovered pairs        : {summary['uncovered']}")
    print(f"  Most-coupled engine    : {summary['most_coupled']} ({summary['most_coupled_count']} total presets across its pairs)")
    print(f"  Least-coupled engine   : {summary['least_coupled']} ({summary['least_coupled_count']} total presets)")
    if skipped:
        print(f"  Skipped (parse errors) : {skipped}")

    # Top 10 richest pairs
    print(f"\n  TOP 10 RICHEST PAIRS (most presets)")
    print(f"  {'Pair':<28} Presets")
    print(f"  {'─'*28} {'─'*7}")
    richest = top_richest_pairs(pair_counts, n=10)
    if richest:
        for pair, count in richest:
            pair_list = sorted(pair)
            label = f"{pair_list[0]} × {pair_list[1]}"
            bar = count_to_char(count) * min(count, 20)
            print(f"  {label:<28} {count:>3}  {bar}")
    else:
        print("  (none found)")

    # Top 10 empty recommended pairs
    print(f"\n  TOP 10 EMPTY BUT RECOMMENDED PAIRS (cross-family)")
    print(f"  {'Pair':<28} Families")
    print(f"  {'─'*28} {'─'*20}")
    recommended = top_empty_recommended(pair_counts, ENGINES, n=10)
    if recommended:
        for a, b, cross, fa, fb in recommended:
            label = f"{a} × {b}"
            families = f"{fa} ↔ {fb}"
            print(f"  {label:<28} {families}")
    else:
        print("  (all pairs covered!)")

    print()


if __name__ == "__main__":
    main()
