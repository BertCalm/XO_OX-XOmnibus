#!/usr/bin/env python3
"""
xpn_engine_coverage_mapper.py — XO_OX Engine Coverage Mapper

Scans .xometa preset files to map which XO_OX engines have coverage,
identifies gaps, analyzes coupling pairs, and recommends priority engines
for the next XPN pack.

Usage:
    python xpn_engine_coverage_mapper.py --preset-dir <dir> [--output report.txt] [--json]
"""

import argparse
import json
import sys
from collections import defaultdict
from pathlib import Path

# ─────────────────────────────────────────────────────────────────────────────
# Engine Registry
# ─────────────────────────────────────────────────────────────────────────────

REGISTERED_ENGINES = [
    "ODDFELIX", "ODDOSCAR", "OVERDUB", "ODYSSEY", "OBLONG", "OBESE",
    "ONSET", "OVERWORLD", "OPAL", "ORBITAL", "ORGANON", "OUROBOROS",
    "OBSIDIAN", "OVERBITE", "ORIGAMI", "ORACLE", "OBSCURA", "OCEANIC",
    "OCELOT", "OPTIC", "OBLIQUE", "OSPREY", "OSTERIA", "OWLFISH",
    "OHM", "ORPHICA", "OBBLIGATO", "OTTONI", "OLE",
    "OVERLAP", "OUTWIT", "OMBRE", "ORCA", "OCTOPUS",
]

CONCEPT_ENGINES = ["OSTINATO", "OPENSKY", "OCEANDEEP", "OUIE"]

ALL_ENGINES = REGISTERED_ENGINES + CONCEPT_ENGINES

# Engine novelty score (1-5): novel/complex engines score higher for recommendations.
# Newer Constellation + concept engines get highest scores.
ENGINE_NOVELTY = {
    "ODDFELIX":   2, "ODDOSCAR":  2, "OVERDUB":   2, "ODYSSEY":   2,
    "OBLONG":     2, "OBESE":     2, "ONSET":     3, "OVERWORLD": 3,
    "OPAL":       3, "ORBITAL":   2, "ORGANON":   4, "OUROBOROS": 4,
    "OBSIDIAN":   3, "OVERBITE":  3, "ORIGAMI":   3, "ORACLE":    4,
    "OBSCURA":    3, "OCEANIC":   3, "OCELOT":    3, "OPTIC":     4,
    "OBLIQUE":    4, "OSPREY":    3, "OSTERIA":   3, "OWLFISH":   4,
    "OHM":        4, "ORPHICA":   4, "OBBLIGATO": 4, "OTTONI":    4,
    "OLE":        4, "OVERLAP":   5, "OUTWIT":    5, "OMBRE":     4,
    "ORCA":       4, "OCTOPUS":   5,
    # Concept engines — highest novelty, no code yet
    "OSTINATO":   5, "OPENSKY":   5, "OCEANDEEP": 5, "OUIE":      5,
}

# Canonical name aliases used in .xometa files → normalised short name
ENGINE_ALIASES: dict[str, str] = {
    # Engine IDs used in presets (various historical forms)
    "oddfelix": "ODDFELIX", "snap": "ODDFELIX", "oddfelx": "ODDFELIX",
    "oddoscar": "ODDOSCAR", "morph": "ODDOSCAR",
    "overdub": "OVERDUB", "dub": "OVERDUB",
    "odyssey": "ODYSSEY", "drift": "ODYSSEY",
    "oblong": "OBLONG", "bob": "OBLONG",
    "obese": "OBESE", "fat": "OBESE",
    "onset": "ONSET", "perc": "ONSET",
    "overworld": "OVERWORLD",
    "opal": "OPAL",
    "orbital": "ORBITAL",
    "organon": "ORGANON",
    "ouroboros": "OUROBOROS",
    "obsidian": "OBSIDIAN",
    "overbite": "OVERBITE", "bite": "OVERBITE",
    "origami": "ORIGAMI",
    "oracle": "ORACLE",
    "obscura": "OBSCURA",
    "oceanic": "OCEANIC",
    "ocelot": "OCELOT",
    "optic": "OPTIC",
    "oblique": "OBLIQUE",
    "osprey": "OSPREY",
    "osteria": "OSTERIA",
    "owlfish": "OWLFISH", "xowlfish": "OWLFISH",
    "ohm": "OHM",
    "orphica": "ORPHICA",
    "obbligato": "OBBLIGATO",
    "ottoni": "OTTONI",
    "ole": "OLE",
    "overlap": "OVERLAP",
    "outwit": "OUTWIT",
    "ombre": "OMBRE",
    "orca": "ORCA",
    "octopus": "OCTOPUS",
    "ostinato": "OSTINATO",
    "opensky": "OPENSKY",
    "oceandeep": "OCEANDEEP",
    "ouie": "OUIE",
}


def normalise_engine(raw: str):
    """Convert a raw engine string from a preset to its canonical short name."""
    key = raw.strip().lower().lstrip("x")  # strip leading 'X' prefix (e.g. XOwlfish)
    # Try with and without X prefix
    result = ENGINE_ALIASES.get(key) or ENGINE_ALIASES.get(raw.strip().lower())
    return result


# ─────────────────────────────────────────────────────────────────────────────
# Scanning
# ─────────────────────────────────────────────────────────────────────────────

def scan_presets(preset_dir: Path) -> tuple[
    dict[str, int],           # engine → total preset count
    dict[str, int],           # engine → entangled preset count
    list[tuple[str, str]],    # coupling pairs (engine_a, engine_b) from Entangled
    list[str],                # unrecognised engine strings encountered
]:
    engine_counts: dict[str, int] = defaultdict(int)
    entangled_counts: dict[str, int] = defaultdict(int)
    coupling_pairs: list[tuple[str, str]] = []
    unknown: list[str] = []

    xometa_files = list(preset_dir.rglob("*.xometa"))
    if not xometa_files:
        print(f"[warn] No .xometa files found under {preset_dir}", file=sys.stderr)
        return dict(engine_counts), dict(entangled_counts), coupling_pairs, unknown

    for fpath in xometa_files:
        try:
            data = json.loads(fpath.read_text(encoding="utf-8"))
        except (json.JSONDecodeError, OSError) as exc:
            print(f"[warn] Could not parse {fpath.name}: {exc}", file=sys.stderr)
            continue

        raw_engines: list[str] = data.get("engines", [])
        mood: str = data.get("mood", "")
        is_entangled = mood.lower() == "entangled"

        normed: list[str] = []
        for raw in raw_engines:
            canon = normalise_engine(raw)
            if canon:
                engine_counts[canon] += 1
                normed.append(canon)
                if is_entangled:
                    entangled_counts[canon] += 1
            else:
                if raw not in unknown:
                    unknown.append(raw)

        # Extract coupling pairs from preset coupling field
        coupling = data.get("coupling")
        if coupling and isinstance(coupling, dict):
            pairs = coupling.get("pairs", []) or []
            for pair in pairs:
                engine_a_raw = pair.get("engineA") or pair.get("sourceEngine", "")
                engine_b_raw = pair.get("engineB") or pair.get("targetEngine", "")
                ea = normalise_engine(engine_a_raw) if engine_a_raw else None
                eb = normalise_engine(engine_b_raw) if engine_b_raw else None
                if ea and eb and ea != eb:
                    coupling_pairs.append((min(ea, eb), max(ea, eb)))

    return dict(engine_counts), dict(entangled_counts), coupling_pairs, unknown


# ─────────────────────────────────────────────────────────────────────────────
# Metrics
# ─────────────────────────────────────────────────────────────────────────────

def compute_coverage_tiers(engine_counts: dict[str, int]) -> dict[str, list[str]]:
    tiers: dict[str, list[str]] = {
        "zero":     [],  # 0 presets
        "thin":     [],  # 1–9
        "moderate": [],  # 10–49
        "strong":   [],  # 50+
    }
    for engine in ALL_ENGINES:
        count = engine_counts.get(engine, 0)
        if count == 0:
            tiers["zero"].append(engine)
        elif count < 10:
            tiers["thin"].append(engine)
        elif count < 50:
            tiers["moderate"].append(engine)
        else:
            tiers["strong"].append(engine)
    return tiers


def compute_coupling_coverage(
    coupling_pairs: list[tuple[str, str]],
) -> tuple[dict[tuple[str, str], int], list[tuple[str, str]]]:
    """Count coupling pair occurrences and find engine pairs with zero coupling presets."""
    pair_counts: dict[tuple[str, str], int] = defaultdict(int)
    for pair in coupling_pairs:
        pair_counts[pair] += 1

    # Identify zero-coupling pairs among registered (non-concept) engines
    # Only flag pairs where both engines are in REGISTERED_ENGINES
    reg = set(REGISTERED_ENGINES)
    zero_pairs: list[tuple[str, str]] = []
    covered = set(pair_counts.keys())
    for i, ea in enumerate(REGISTERED_ENGINES):
        for eb in REGISTERED_ENGINES[i + 1:]:
            pair = (min(ea, eb), max(ea, eb))
            if pair not in covered:
                zero_pairs.append(pair)

    return dict(pair_counts), zero_pairs


def compute_recommendations(
    engine_counts: dict[str, int],
    top_n: int = 5,
) -> list[tuple[str, float, int, int]]:
    """
    Score each engine: score = novelty / (preset_count + 1)
    Higher score = more urgent for next pack.
    Returns list of (engine, score, preset_count, novelty) sorted descending.
    """
    scored: list[tuple[str, float, int, int]] = []
    for engine in ALL_ENGINES:
        count = engine_counts.get(engine, 0)
        novelty = ENGINE_NOVELTY.get(engine, 3)
        score = novelty / (count + 1)
        scored.append((engine, round(score, 4), count, novelty))
    scored.sort(key=lambda x: (-x[1], x[0]))
    return scored[:top_n]


# ─────────────────────────────────────────────────────────────────────────────
# Rendering
# ─────────────────────────────────────────────────────────────────────────────

BAR_WIDTH = 30  # characters for ASCII bar


def make_bar(count: int, max_count: int) -> str:
    if max_count == 0:
        return ""
    filled = round(BAR_WIDTH * count / max_count)
    return "[" + "#" * filled + "-" * (BAR_WIDTH - filled) + "]"


def render_report(
    engine_counts: dict[str, int],
    entangled_counts: dict[str, int],
    tiers: dict[str, list[str]],
    pair_counts: dict[tuple[str, str], int],
    zero_pairs: list[tuple[str, str]],
    recommendations: list[tuple[str, float, int, int]],
    unknown: list[str],
    preset_dir: Path,
) -> str:
    lines: list[str] = []
    sep = "=" * 72

    # Header
    lines += [
        sep,
        "  XO_OX ENGINE COVERAGE MAPPER",
        f"  Preset directory : {preset_dir}",
        f"  Engines tracked  : {len(ALL_ENGINES)} ({len(REGISTERED_ENGINES)} registered + {len(CONCEPT_ENGINES)} concept)",
        f"  Total presets    : {sum(engine_counts.values())}",
        sep,
        "",
    ]

    # Coverage table (ascending preset count — most urgent first)
    lines += [
        "COVERAGE TABLE  (sorted: fewest presets first)",
        "-" * 72,
        f"{'ENGINE':<14} {'COUNT':>6}  {'TIER':<10}  {'ENTANGLED':>9}  BAR",
        "-" * 72,
    ]
    all_counts = [(e, engine_counts.get(e, 0)) for e in ALL_ENGINES]
    all_counts.sort(key=lambda x: (x[1], x[0]))
    max_count = max((c for _, c in all_counts), default=1)

    tier_map = {}
    for tier_name, engines in tiers.items():
        for e in engines:
            tier_map[e] = tier_name

    for engine, count in all_counts:
        tier = tier_map.get(engine, "?")
        entangled = entangled_counts.get(engine, 0)
        bar = make_bar(count, max_count)
        concept_marker = " *" if engine in CONCEPT_ENGINES else ""
        lines.append(
            f"{engine:<14}{concept_marker:<2} {count:>5}  {tier:<10}  {entangled:>9}  {bar}"
        )

    lines += [
        "-" * 72,
        "  * = concept engine (no source code yet)",
        "",
    ]

    # Tier summary
    lines += [
        "TIER SUMMARY",
        "-" * 72,
        f"  Zero coverage    ({len(tiers['zero']):>3} engines): {', '.join(tiers['zero']) or 'none'}",
        f"  Thin  (1-9)      ({len(tiers['thin']):>3} engines): {', '.join(tiers['thin']) or 'none'}",
        f"  Moderate (10-49) ({len(tiers['moderate']):>3} engines): {', '.join(tiers['moderate']) or 'none'}",
        f"  Strong (50+)     ({len(tiers['strong']):>3} engines): {', '.join(tiers['strong']) or 'none'}",
        "",
    ]

    # ASCII bar chart
    lines += [
        "PRESET COUNT BAR CHART  (all engines, ascending)",
        "-" * 72,
    ]
    for engine, count in all_counts:
        bar = make_bar(count, max_count)
        lines.append(f"  {engine:<14} {bar}  {count}")
    lines.append("")

    # Coupling coverage
    top_pairs = sorted(pair_counts.items(), key=lambda x: -x[1])[:15]
    lines += [
        "COUPLING COVERAGE  (Entangled mood — top pairs by frequency)",
        "-" * 72,
    ]
    if top_pairs:
        for (ea, eb), cnt in top_pairs:
            lines.append(f"  {ea} + {eb:<14}  {cnt:>3} preset(s)")
    else:
        lines.append("  No coupling pair data found.")
    lines.append("")

    zero_sample = zero_pairs[:20]
    lines += [
        f"ZERO-COUPLING PAIRS  (first 20 of {len(zero_pairs)} uncovered registered pairs)",
        "-" * 72,
    ]
    if zero_sample:
        for ea, eb in zero_sample:
            lines.append(f"  {ea} + {eb}")
        if len(zero_pairs) > 20:
            lines.append(f"  ... and {len(zero_pairs) - 20} more")
    else:
        lines.append("  All registered engine pairs have at least one coupling preset.")
    lines.append("")

    # Recommendations
    lines += [
        "TOP 5 ENGINE RECOMMENDATIONS FOR NEXT PACK",
        "-" * 72,
        f"  Score = novelty / (preset_count + 1)   [novelty 1-5: 5 = most novel]",
        "-" * 72,
        f"  {'RANK':<5} {'ENGINE':<14} {'SCORE':>6}  {'PRESETS':>7}  {'NOVELTY':>7}  REASON",
        "-" * 72,
    ]
    for rank, (engine, score, count, novelty) in enumerate(recommendations, 1):
        tier = tier_map.get(engine, "?")
        concept_note = " [concept — no DSP yet]" if engine in CONCEPT_ENGINES else ""
        reason = f"{tier} coverage, novelty={novelty}{concept_note}"
        lines.append(f"  {rank:<5} {engine:<14} {score:>6.3f}  {count:>7}  {novelty:>7}  {reason}")
    lines.append("")

    # Unknowns
    if unknown:
        lines += [
            "UNRECOGNISED ENGINE STRINGS  (not in alias table — check manually)",
            "-" * 72,
        ]
        for u in unknown:
            lines.append(f"  '{u}'")
        lines.append("")

    lines.append(sep)
    return "\n".join(lines)


def build_json_output(
    engine_counts: dict[str, int],
    entangled_counts: dict[str, int],
    tiers: dict[str, list[str]],
    pair_counts: dict[tuple[str, str], int],
    zero_pairs: list[tuple[str, str]],
    recommendations: list[tuple[str, float, int, int]],
    unknown: list[str],
) -> dict:
    return {
        "engine_counts": {e: engine_counts.get(e, 0) for e in ALL_ENGINES},
        "entangled_counts": {e: entangled_counts.get(e, 0) for e in ALL_ENGINES},
        "tiers": tiers,
        "coupling_pair_counts": {f"{ea}+{eb}": cnt for (ea, eb), cnt in pair_counts.items()},
        "zero_coupling_pairs": [f"{ea}+{eb}" for ea, eb in zero_pairs],
        "recommendations": [
            {"engine": e, "score": s, "preset_count": c, "novelty": n}
            for e, s, c, n in recommendations
        ],
        "unrecognised_engines": unknown,
        "summary": {
            "total_presets_scanned": sum(engine_counts.values()),
            "zero_coverage_count": len(tiers["zero"]),
            "thin_coverage_count": len(tiers["thin"]),
            "moderate_coverage_count": len(tiers["moderate"]),
            "strong_coverage_count": len(tiers["strong"]),
            "total_coupling_pairs_covered": len(pair_counts),
            "total_coupling_pairs_missing": len(zero_pairs),
        },
    }


# ─────────────────────────────────────────────────────────────────────────────
# CLI
# ─────────────────────────────────────────────────────────────────────────────

def main() -> None:
    parser = argparse.ArgumentParser(
        description="Map XO_OX engine coverage across .xometa preset files.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument(
        "--preset-dir",
        required=True,
        metavar="DIR",
        help="Root directory to scan recursively for .xometa files",
    )
    parser.add_argument(
        "--output",
        metavar="FILE",
        help="Write text report to FILE (default: stdout)",
    )
    parser.add_argument(
        "--json",
        action="store_true",
        dest="as_json",
        help="Output JSON instead of text report",
    )
    parser.add_argument(
        "--top",
        type=int,
        default=5,
        metavar="N",
        help="Number of engines to recommend (default: 5)",
    )
    args = parser.parse_args()

    preset_dir = Path(args.preset_dir)
    if not preset_dir.is_dir():
        print(f"[error] --preset-dir '{preset_dir}' is not a directory.", file=sys.stderr)
        sys.exit(1)

    print(f"Scanning {preset_dir} ...", file=sys.stderr)
    engine_counts, entangled_counts, coupling_pairs, unknown = scan_presets(preset_dir)
    print(f"Scan complete: {sum(engine_counts.values())} preset-engine references found.", file=sys.stderr)

    tiers = compute_coverage_tiers(engine_counts)
    pair_counts, zero_pairs = compute_coupling_coverage(coupling_pairs)
    recommendations = compute_recommendations(engine_counts, top_n=args.top)

    if args.as_json:
        output = json.dumps(
            build_json_output(
                engine_counts, entangled_counts, tiers,
                pair_counts, zero_pairs, recommendations, unknown,
            ),
            indent=2,
        )
    else:
        output = render_report(
            engine_counts, entangled_counts, tiers,
            pair_counts, zero_pairs, recommendations, unknown,
            preset_dir,
        )

    if args.output:
        out_path = Path(args.output)
        out_path.write_text(output, encoding="utf-8")
        print(f"Report written to {out_path}", file=sys.stderr)
    else:
        print(output)


if __name__ == "__main__":
    main()
