#!/usr/bin/env python3
"""
xpn_collection_sequencer.py — XO_OX Collection Release Sequencer

Generates recommended release sequences for a collection of XPN packs,
determining optimal release order based on DNA diversity, mood coverage,
and engine representation.

Pack spec JSON fields expected:
  name              str        — pack display name
  engines           list[str]  — engine IDs used (e.g. ["OBLONG", "OHM"])
  mood_distribution dict       — {mood: float} summing to ~1.0
                                 moods: Foundation/Atmosphere/Entangled/Prism/Flux/Aether/Family
  dna_centroid      dict       — {brightness, warmth, movement, density, space, aggression}
                                 each 0.0–1.0
  engine_accent_color str      — hex accent color (informational, used in output)
  theme             str        — optional thematic grouping within a collection
  felixOscarRatio   float      — optional 0.0 (pure Oscar) to 1.0 (pure feliX); derived
                                 from dna if absent

Strategies:
  max-contrast  — each release maximally different from the previous
  gradual       — start accessible (high feliX/low aggression), build toward experimental
  story         — group by theme, then order within groups for narrative arc

Usage:
  python xpn_collection_sequencer.py --packs-dir <dir> \\
    [--strategy max-contrast|gradual|story|all] \\
    [--output sequence.json]

Exit codes: 0 success, 1 input error
"""

import argparse
import json
import math
from pathlib import Path

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

DNA_DIMS = ["brightness", "warmth", "movement", "density", "space", "aggression"]
MOODS = ["Foundation", "Atmosphere", "Entangled", "Prism", "Flux", "Aether", "Family"]

# feliX axis: bright + warm + movement maps to feliX; dense + aggressive maps to Oscar
FELIX_WEIGHTS = {"brightness": 0.35, "warmth": 0.25, "movement": 0.20,
                 "density": -0.10, "space": 0.10, "aggression": -0.20}


# ---------------------------------------------------------------------------
# Data loading
# ---------------------------------------------------------------------------

def load_packs(packs_dir: Path) -> list[dict]:
    """Load all *.json spec files from packs_dir."""
    specs = []
    for p in sorted(packs_dir.glob("*.json")):
        try:
            data = json.loads(p.read_text())
        except json.JSONDecodeError as e:
            print(f"  [WARN] Skipping {p.name}: JSON error — {e}")
            continue

        # Minimal validation
        name = data.get("name", p.stem)
        engines = data.get("engines", [])
        dna_raw = data.get("dna_centroid", {})
        dna = {d: float(dna_raw.get(d, 0.5)) for d in DNA_DIMS}
        mood_raw = data.get("mood_distribution", {})
        moods = {m: float(mood_raw.get(m, 0.0)) for m in MOODS}

        felix = data.get("felixOscarRatio")
        if felix is None:
            felix = sum(dna[dim] * FELIX_WEIGHTS[dim] for dim in DNA_DIMS)
            # normalise to 0–1
            felix = max(0.0, min(1.0, felix + 0.5))

        specs.append({
            "name": name,
            "file": p.name,
            "engines": [str(e).upper() for e in engines],
            "mood_distribution": moods,
            "dna_centroid": dna,
            "engine_accent_color": data.get("engine_accent_color", "#888888"),
            "theme": data.get("theme", ""),
            "felix": float(felix),
        })

    return specs


# ---------------------------------------------------------------------------
# Scoring helpers
# ---------------------------------------------------------------------------

def dna_distance(a: dict, b: dict) -> float:
    """Euclidean distance between two dna_centroid dicts (0–√6 ≈ 2.449)."""
    return math.sqrt(sum((a[d] - b[d]) ** 2 for d in DNA_DIMS))


def new_engines_score(prev_engines: set, cur_engines: list) -> float:
    """Fraction of engines in cur that are genuinely new (not in prev)."""
    if not cur_engines:
        return 0.0
    new = len(set(cur_engines) - prev_engines)
    return new / len(cur_engines)


def mood_shift(a: dict, b: dict) -> float:
    """L1 distance between two mood distributions (0–2 range)."""
    return sum(abs(a[m] - b[m]) for m in MOODS)


def contrast_score(prev: dict, cur: dict, seen_engines: set) -> float:
    """
    Composite contrast score: how different is cur from prev.
    Returns a value roughly 0–1 (not capped, can exceed 1 for dramatic shifts).
    """
    d = dna_distance(prev["dna_centroid"], cur["dna_centroid"]) / math.sqrt(6)
    e = new_engines_score(seen_engines, cur["engines"])
    m = mood_shift(prev["mood_distribution"], cur["mood_distribution"]) / 2.0
    return 0.50 * d + 0.30 * e + 0.20 * m


def arc_score(sequence: list[dict]) -> float:
    """
    Overall arc score — average pairwise contrast for consecutive pairs.
    Higher is more varied; 0 is repetitive.
    """
    if len(sequence) < 2:
        return 0.0
    scores = []
    seen = set()
    for i in range(1, len(sequence)):
        s = contrast_score(sequence[i - 1], sequence[i], seen)
        scores.append(s)
        seen.update(sequence[i - 1]["engines"])
    return sum(scores) / len(scores)


def recommend_cadence(n_packs: int, arc: float) -> str:
    """Heuristic release cadence recommendation."""
    if n_packs <= 4:
        return "weekly"
    if arc >= 0.55:
        return "weekly"        # high contrast — audience can absorb rapid variety
    if arc >= 0.35:
        return "biweekly"
    return "monthly"           # low variety — give each pack room to breathe


# ---------------------------------------------------------------------------
# Anchor pack selection
# ---------------------------------------------------------------------------

def pick_anchor(packs: list[dict], strategy: str) -> tuple[dict, str]:
    """Return (anchor_pack, reason_string) for a given strategy."""
    if strategy == "max-contrast":
        # Anchor = most centrist DNA (easiest entry, most room to diverge)
        center = {d: 0.5 for d in DNA_DIMS}
        anchor = min(packs, key=lambda p: dna_distance(p["dna_centroid"], center))
        reason = (
            f"'{anchor['name']}' has the most centrist DNA profile — "
            "closest to the midpoint of all 6 dimensions, giving maximum room "
            "to diverge in subsequent releases."
        )
    elif strategy == "gradual":
        # Anchor = most feliX-facing (accessible, bright, warm)
        anchor = max(packs, key=lambda p: p["felix"])
        reason = (
            f"'{anchor['name']}' scores highest on the feliX axis "
            f"(felix={anchor['felix']:.2f}) — bright, warm, and approachable, "
            "making it the ideal on-ramp for new listeners."
        )
    else:  # story
        # Anchor = pack with most Foundation/Atmosphere mood weight
        anchor = max(
            packs,
            key=lambda p: p["mood_distribution"]["Foundation"]
            + p["mood_distribution"]["Atmosphere"],
        )
        reason = (
            f"'{anchor['name']}' carries the strongest Foundation + Atmosphere mood weight — "
            "the natural opening chapter that establishes the world before the narrative deepens."
        )
    return anchor, reason


# ---------------------------------------------------------------------------
# Strategy implementations
# ---------------------------------------------------------------------------

def sequence_max_contrast(packs: list[dict]) -> list[dict]:
    """
    Greedy nearest-dissimilar: at each step pick the unplaced pack with the
    highest contrast_score vs the previous release.
    """
    if not packs:
        return []
    anchor, _ = pick_anchor(packs, "max-contrast")
    remaining = [p for p in packs if p is not anchor]
    ordered = [anchor]
    seen_engines: set = set(anchor["engines"])

    while remaining:
        prev = ordered[-1]
        best = max(remaining, key=lambda p: contrast_score(prev, p, seen_engines))
        ordered.append(best)
        seen_engines.update(best["engines"])
        remaining.remove(best)

    return ordered


def sequence_gradual(packs: list[dict]) -> list[dict]:
    """
    Sort by feliX score descending (accessible → experimental).
    Ties broken by lower aggression first.
    """
    return sorted(
        packs,
        key=lambda p: (-p["felix"], p["dna_centroid"]["aggression"]),
    )


def sequence_story(packs: list[dict]) -> list[dict]:
    """
    Group by 'theme', order groups by ascending density (light → dense),
    within each group order by ascending aggression (safe → edgy).
    Groups without a theme share the "" bucket, placed first.
    """
    groups: dict[str, list[dict]] = {}
    for p in packs:
        t = p["theme"] or ""
        groups.setdefault(t, []).append(p)

    # Sort themes: unnamed first, then by average density ascending
    def group_density(theme):
        g = groups[theme]
        return sum(p["dna_centroid"]["density"] for p in g) / len(g)

    theme_order = sorted(groups.keys(), key=lambda t: (t != "", group_density(t)))

    ordered = []
    for theme in theme_order:
        group = sorted(groups[theme], key=lambda p: p["dna_centroid"]["aggression"])
        ordered.extend(group)
    return ordered


# ---------------------------------------------------------------------------
# Result building
# ---------------------------------------------------------------------------

def build_result(strategy: str, packs: list[dict], ordered: list[dict]) -> dict:
    anchor, anchor_reason = pick_anchor(packs, strategy)

    steps = []
    seen_engines: set = set()
    for i, pack in enumerate(ordered):
        if i == 0:
            c = None
        else:
            c = round(contrast_score(ordered[i - 1], pack, seen_engines), 4)
        new_e = sorted(set(pack["engines"]) - seen_engines)
        steps.append({
            "position": i + 1,
            "name": pack["name"],
            "file": pack["file"],
            "engines": pack["engines"],
            "new_engines": new_e,
            "engine_accent_color": pack["engine_accent_color"],
            "felix_score": round(pack["felix"], 3),
            "dna_centroid": {k: round(v, 3) for k, v in pack["dna_centroid"].items()},
            "dominant_mood": max(pack["mood_distribution"], key=pack["mood_distribution"].get),
            "contrast_from_previous": c,
        })
        seen_engines.update(pack["engines"])

    overall_arc = round(arc_score(ordered), 4)
    cadence = recommend_cadence(len(ordered), overall_arc)

    return {
        "strategy": strategy,
        "anchor_pack": anchor["name"],
        "anchor_reason": anchor_reason,
        "overall_arc_score": overall_arc,
        "recommended_cadence": cadence,
        "pack_count": len(ordered),
        "sequence": steps,
    }


# ---------------------------------------------------------------------------
# Report formatting
# ---------------------------------------------------------------------------

def print_result(result: dict) -> None:
    strat = result["strategy"].upper().replace("-", " ")
    print(f"\n{'=' * 60}")
    print(f"  STRATEGY: {strat}")
    print(f"{'=' * 60}")
    print(f"  Anchor pack   : {result['anchor_pack']}")
    print(f"  Anchor reason : {result['anchor_reason']}")
    print(f"  Arc score     : {result['overall_arc_score']:.4f}  (higher = more varied)")
    print(f"  Release cadence: {result['recommended_cadence']}")
    print(f"  Packs         : {result['pack_count']}")
    print()

    for step in result["sequence"]:
        pos = step["position"]
        name = step["name"]
        dominant = step["dominant_mood"]
        felix = step["felix_score"]
        contrast = step["contrast_from_previous"]
        new_e = step["new_engines"]

        contrast_str = f"  contrast={contrast:.3f}" if contrast is not None else "  [ANCHOR]     "
        new_str = f"  +new: {', '.join(new_e)}" if new_e else ""
        print(f"  {pos:2d}. {name:<32} [{dominant:<12}] feliX={felix:.2f}{contrast_str}{new_str}")

    print()


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def parse_args():
    p = argparse.ArgumentParser(
        description="Generate recommended XPN pack release sequences for a collection.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    p.add_argument(
        "--packs-dir",
        required=True,
        type=Path,
        help="Directory containing pack spec JSON files",
    )
    p.add_argument(
        "--strategy",
        choices=["max-contrast", "gradual", "story", "all"],
        default="all",
        help="Sequencing strategy (default: all — run all three)",
    )
    p.add_argument(
        "--output",
        type=Path,
        default=None,
        help="Optional JSON output file (default: stdout only)",
    )
    return p.parse_args()


def main():
    args = parse_args()

    if not args.packs_dir.is_dir():
        print(f"[ERROR] packs-dir not found: {args.packs_dir}")
        raise SystemExit(1)

    packs = load_packs(args.packs_dir)
    if not packs:
        print(f"[ERROR] No valid JSON pack specs found in {args.packs_dir}")
        raise SystemExit(1)

    print(f"\nLoaded {len(packs)} pack(s) from {args.packs_dir}")

    strategies = (
        ["max-contrast", "gradual", "story"]
        if args.strategy == "all"
        else [args.strategy]
    )

    strategy_fn = {
        "max-contrast": sequence_max_contrast,
        "gradual": sequence_gradual,
        "story": sequence_story,
    }

    all_results = []
    for strat in strategies:
        ordered = strategy_fn[strat](packs)
        result = build_result(strat, packs, ordered)
        all_results.append(result)
        print_result(result)

    # Summary comparison when all three run
    if len(all_results) == 3:
        print("=" * 60)
        print("  STRATEGY COMPARISON")
        print("=" * 60)
        best = max(all_results, key=lambda r: r["overall_arc_score"])
        print(f"  Highest arc score : {best['strategy']}  ({best['overall_arc_score']:.4f})")
        for r in all_results:
            print(f"    {r['strategy']:<16} arc={r['overall_arc_score']:.4f}  cadence={r['recommended_cadence']}")
        print()

    if args.output:
        payload = {
            "generator": "xpn_collection_sequencer",
            "packs_dir": str(args.packs_dir),
            "pack_count": len(packs),
            "results": all_results,
        }
        args.output.write_text(json.dumps(payload, indent=2))
        print(f"  Wrote results to {args.output}")


if __name__ == "__main__":
    main()
