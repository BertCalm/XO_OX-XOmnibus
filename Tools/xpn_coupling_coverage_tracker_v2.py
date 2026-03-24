#!/usr/bin/env python3
"""
xpn_coupling_coverage_tracker_v2.py
Scans all Entangled .xometa presets and reports coupling pair coverage
across all 34 registered XOlokun engines.

Usage: python3 Tools/xpn_coupling_coverage_tracker_v2.py
"""

import json
import os
import sys
from itertools import combinations
from datetime import datetime

# ─── Configuration ───────────────────────────────────────────────────────────

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ENTANGLED_DIR = os.path.join(REPO_ROOT, "Presets", "XOlokun", "Entangled")
SNAPSHOT_PATH = os.path.join(REPO_ROOT, "Docs", "snapshots", "coupling_coverage_latest.json")

# All 34 canonical engine names
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
# Maps every known variant → canonical name (uppercase)
ALIASES: dict[str, str] = {}

# Direct uppercase already canonical — build base map
for e in ENGINES:
    ALIASES[e] = e
    ALIASES[e.lower()] = e

# Legacy short names
_LEGACY = {
    "Snap": "ODDFELIX",
    "snap": "ODDFELIX",
    "OddfeliX": "ODDFELIX",
    "ODDFELIX": "ODDFELIX",
    "Morph": "ODDOSCAR",
    "morph": "ODDOSCAR",
    "OddOscar": "ODDOSCAR",
    "ODDOSCAR": "ODDOSCAR",
    "Dub": "OVERDUB",
    "dub": "OVERDUB",
    "XOverdub": "OVERDUB",
    "Drift": "ODYSSEY",
    "drift": "ODYSSEY",
    "Odyssey": "ODYSSEY",
    "Bob": "OBLONG",
    "bob": "OBLONG",
    "Oblong": "OBLONG",
    "XOblong": "OBLONG",
    "Fat": "OBESE",
    "fat": "OBESE",
    "Obese": "OBESE",
    "XObese": "OBESE",
    "Bite": "OVERBITE",
    "bite": "OVERBITE",
    "Overbite": "OVERBITE",
    "XOverbite": "OVERBITE",
    # Onset variants
    "Onset": "ONSET",
    "XOnset": "ONSET",
    # Overworld variants
    "Overworld": "OVERWORLD",
    "XOverworld": "OVERWORLD",
    # Opal
    "Opal": "OPAL",
    # Orbital
    "Orbital": "ORBITAL",
    # Organon
    "Organon": "ORGANON",
    # Ouroboros
    "Ouroboros": "OUROBOROS",
    "XOuroboros": "OUROBOROS",
    # Obsidian
    "Obsidian": "OBSIDIAN",
    # Origami
    "Origami": "ORIGAMI",
    "XOrigami": "ORIGAMI",
    # Oracle
    "Oracle": "ORACLE",
    "XOracle": "ORACLE",
    # Obscura
    "Obscura": "OBSCURA",
    # Oceanic
    "Oceanic": "OCEANIC",
    # Ocelot
    "Ocelot": "OCELOT",
    # Optic
    "Optic": "OPTIC",
    # Oblique
    "Oblique": "OBLIQUE",
    "XOblique": "OBLIQUE",
    # Osprey
    "Osprey": "OSPREY",
    # Osteria
    "Osteria": "OSTERIA",
    "XOsteria": "OSTERIA",
    # Owlfish
    "Owlfish": "OWLFISH",
    "XOwlfish": "OWLFISH",
    # Ohm
    "Ohm": "OHM",
    "XOhm": "OHM",
    # Orphica
    "Orphica": "ORPHICA",
    # Obbligato
    "Obbligato": "OBBLIGATO",
    # Ottoni
    "Ottoni": "OTTONI",
    "XOttoni": "OTTONI",
    # Ole
    "Ole": "OLE",
    # Overlap
    "Overlap": "OVERLAP",
    # Outwit
    "Outwit": "OUTWIT",
    # Ombre
    "Ombre": "OMBRE",
    # Orca
    "Orca": "ORCA",
    # Octopus
    "Octopus": "OCTOPUS",
    # Overdrive — not a registered engine; map to OVERDUB as closest
    # (legacy data artifact)
    "OVERDRIVE": "OVERDUB",
    "Overdrive": "OVERDUB",
}
ALIASES.update(_LEGACY)


def resolve(name: str):
    """Return canonical engine name, or None if unrecognised."""
    return ALIASES.get(name) or ALIASES.get(name.upper())


# ─── Scan ─────────────────────────────────────────────────────────────────────

def scan_entangled() -> tuple[set[tuple[str, str]], dict, list[str]]:
    """
    Returns:
        covered_pairs  — set of frozenset-style sorted tuples
        per_engine     — dict engine → set of partner engines
        unresolved     — list of raw engine names that could not be resolved
    """
    covered_pairs = set()
    per_engine = {e: set() for e in ENGINES}
    unresolved = []
    preset_count = 0

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

        # Deduplicate within a preset
        canonical = list(dict.fromkeys(canonical))

        # Record all unique pairs
        for a, b in combinations(canonical, 2):
            pair = (min(a, b), max(a, b))
            covered_pairs.add(pair)
            per_engine[a].add(b)
            per_engine[b].add(a)

    return covered_pairs, per_engine, unresolved, preset_count  # type: ignore


# ─── Report ───────────────────────────────────────────────────────────────────

def main() -> None:
    if not os.path.isdir(ENTANGLED_DIR):
        print(f"ERROR: Entangled directory not found: {ENTANGLED_DIR}")
        sys.exit(1)

    result = scan_entangled()
    covered_pairs, per_engine, unresolved, preset_count = result

    covered_count = len(covered_pairs)
    coverage_pct = covered_count / TOTAL_POSSIBLE * 100

    # Per-engine partner counts
    partner_counts = {e: len(partners) for e, partners in per_engine.items()}
    sorted_engines = sorted(partner_counts.items(), key=lambda x: x[1], reverse=True)

    zero_coverage = [e for e, c in partner_counts.items() if c == 0]

    # ── stdout summary ──────────────────────────────────────────────────────
    print("=" * 65)
    print("  XOlokun Entangled Coupling Coverage Report")
    print(f"  Generated: {datetime.utcnow().strftime('%Y-%m-%d %H:%M UTC')}")
    print("=" * 65)
    print(f"\n  Presets scanned    : {preset_count}")
    print(f"  Pairs covered      : {covered_count} / {TOTAL_POSSIBLE}")
    print(f"  Coverage           : {coverage_pct:.1f}%")
    print(f"  Engines registered : {len(ENGINES)}")

    if unresolved:
        print(f"\n  [WARN] Unresolved engine names ({len(unresolved)}):")
        for u in sorted(unresolved):
            print(f"    - {u}")

    # Top 10
    print("\n  TOP 10 most-coupled engines")
    print("  " + "-" * 42)
    for rank, (engine, count) in enumerate(sorted_engines[:10], 1):
        bar = "█" * count
        print(f"  {rank:>2}. {engine:<14}  {count:>2} partners  {bar}")

    # Bottom 10 (excluding zeros — shown separately)
    non_zero = [(e, c) for e, c in sorted_engines if c > 0]
    bottom = non_zero[-10:] if len(non_zero) >= 10 else non_zero
    print("\n  BOTTOM 10 least-coupled (non-zero)")
    print("  " + "-" * 42)
    for engine, count in reversed(bottom):
        bar = "·" * count
        print(f"      {engine:<14}  {count:>2} partners  {bar}")

    # Zero coverage
    if zero_coverage:
        print(f"\n  ZERO coverage engines ({len(zero_coverage)}):")
        for e in sorted(zero_coverage):
            print(f"    - {e}")
    else:
        print("\n  All 34 engines appear in at least one coupling pair.")

    # Full per-engine table
    print("\n  PER-ENGINE COVERAGE (all 34, sorted by partners desc)")
    print("  " + "-" * 42)
    print(f"  {'Engine':<16}  {'Partners':>8}  {'% of possible':>14}")
    for engine, count in sorted_engines:
        possible = len(ENGINES) - 1  # max partners = 33
        pct = count / possible * 100
        print(f"  {engine:<16}  {count:>8}  {pct:>13.1f}%")

    print("\n" + "=" * 65)

    # ── JSON snapshot ───────────────────────────────────────────────────────
    os.makedirs(os.path.dirname(SNAPSHOT_PATH), exist_ok=True)
    snapshot = {
        "timestamp": datetime.utcnow().isoformat() + "Z",
        "total_pairs_covered": covered_count,
        "total_possible": TOTAL_POSSIBLE,
        "coverage_pct": round(coverage_pct, 2),
        "preset_count": preset_count,
        "per_engine_coverage": {e: c for e, c in sorted(partner_counts.items())},
        "zero_coverage_engines": sorted(zero_coverage),
        "unresolved_engine_names": sorted(unresolved),
    }
    with open(SNAPSHOT_PATH, "w", encoding="utf-8") as fh:
        json.dump(snapshot, fh, indent=2)
    print(f"\n  JSON snapshot written → {SNAPSHOT_PATH}")


if __name__ == "__main__":
    main()
