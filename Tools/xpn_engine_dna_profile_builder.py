#!/usr/bin/env python3
"""
xpn_engine_dna_profile_builder.py — XO_OX XOmnibus Engine DNA Profile Builder

Loads all .xometa presets, groups by engine, and computes an authoritative
6D Sonic DNA fingerprint per engine: mean, min/max range, std dev, feliX/Oscar
score, dominant character description, and mood distribution.

Output:
  - JSON database: Docs/engine_dna_profiles.json  (canonical reference)
  - Human-readable report table: engine_profiles.txt (or stdout)

Usage:
  python xpn_engine_dna_profile_builder.py --preset-dir Presets/XOmnibus
  python xpn_engine_dna_profile_builder.py \\
      --preset-dir Presets/XOmnibus \\
      --output-json Docs/engine_dna_profiles.json \\
      --output-report engine_profiles.txt
"""

import argparse
import json
import math
import sys
from collections import defaultdict
from pathlib import Path

# ---------------------------------------------------------------------------
# DNA dimension names (canonical order)
# ---------------------------------------------------------------------------
DNA_DIMS = ["brightness", "warmth", "movement", "density", "space", "aggression"]

# ---------------------------------------------------------------------------
# Character vocabulary — thresholds for the 2-3 word dominant description
# ---------------------------------------------------------------------------

def _describe_brightness(v: float) -> str:
    if v >= 0.75:
        return "bright"
    if v >= 0.5:
        return "airy"
    if v >= 0.25:
        return "muted"
    return "dark"

def _describe_warmth(v: float) -> str:
    if v >= 0.75:
        return "warm"
    if v >= 0.5:
        return "neutral"
    if v >= 0.25:
        return "cool"
    return "cold"

def _describe_movement(v: float) -> str:
    if v >= 0.75:
        return "restless"
    if v >= 0.5:
        return "animated"
    if v >= 0.25:
        return "drifting"
    return "static"

def _describe_density(v: float) -> str:
    if v >= 0.75:
        return "dense"
    if v >= 0.5:
        return "layered"
    if v >= 0.25:
        return "sparse"
    return "bare"

def _describe_space(v: float) -> str:
    if v >= 0.75:
        return "cavernous"
    if v >= 0.5:
        return "spacious"
    if v >= 0.25:
        return "intimate"
    return "dry"

def _describe_aggression(v: float) -> str:
    if v >= 0.75:
        return "aggressive"
    if v >= 0.5:
        return "assertive"
    if v >= 0.25:
        return "gentle"
    return "tender"

_DIM_DESCRIBERS = {
    "brightness": _describe_brightness,
    "warmth": _describe_warmth,
    "movement": _describe_movement,
    "density": _describe_density,
    "space": _describe_space,
    "aggression": _describe_aggression,
}

def dominant_character(mean_dna: dict) -> str:
    """Return a 2-3 word character description from the mean DNA centroid."""
    # Pick the 2 most extreme (furthest from 0.5) dimensions
    extremes = sorted(
        DNA_DIMS,
        key=lambda d: abs(mean_dna.get(d, 0.5) - 0.5),
        reverse=True,
    )
    words = []
    for dim in extremes[:2]:
        val = mean_dna.get(dim, 0.5)
        word = _DIM_DESCRIBERS[dim](val)
        if word not in words:
            words.append(word)
    # Add a third if aggression is notable and not already included
    if len(words) < 3:
        agg = mean_dna.get("aggression", 0.5)
        agg_word = _describe_aggression(agg)
        if agg_word not in words:
            words.append(agg_word)
    return " ".join(words[:3])

# ---------------------------------------------------------------------------
# Math helpers
# ---------------------------------------------------------------------------

def mean(values: list) -> float:
    return sum(values) / len(values) if values else 0.0

def std_dev(values: list) -> float:
    if len(values) < 2:
        return 0.0
    m = mean(values)
    variance = sum((v - m) ** 2 for v in values) / len(values)
    return math.sqrt(variance)

def felix_score(brightness: float, warmth: float) -> float:
    """feliX/Oscar polarity score: 0 = pure Oscar, 1 = pure feliX."""
    return (brightness - warmth + 1.0) / 2.0

# ---------------------------------------------------------------------------
# Preset loading
# ---------------------------------------------------------------------------

def load_presets(preset_dir: Path) -> list[dict]:
    """Recursively load all .xometa files under preset_dir."""
    presets = []
    missing_dna = 0
    for path in sorted(preset_dir.rglob("*.xometa")):
        try:
            with path.open("r", encoding="utf-8") as fh:
                data = json.load(fh)
        except (json.JSONDecodeError, OSError) as exc:
            print(f"  WARNING: could not parse {path}: {exc}", file=sys.stderr)
            continue

        dna = data.get("dna")
        if not isinstance(dna, dict):
            missing_dna += 1
            continue

        engines = data.get("engines", [])
        mood = data.get("mood", "Unknown")
        presets.append({"path": str(path), "engines": engines, "mood": mood, "dna": dna})

    if missing_dna:
        print(f"  NOTE: {missing_dna} preset(s) skipped — no 'dna' field.", file=sys.stderr)
    return presets

# ---------------------------------------------------------------------------
# Engine grouping
# ---------------------------------------------------------------------------

def group_by_engine(presets: list[dict]) -> dict[str, list[dict]]:
    """Map engine name → list of preset dicts that include that engine."""
    groups: dict[str, list[dict]] = defaultdict(list)
    for preset in presets:
        for engine in preset["engines"]:
            groups[engine].append(preset)
    return dict(groups)

# ---------------------------------------------------------------------------
# Profile computation
# ---------------------------------------------------------------------------

def compute_engine_profile(engine: str, presets: list[dict]) -> dict:
    """Compute the full DNA profile for a single engine."""
    # Collect per-dimension value lists
    dim_values: dict[str, list[float]] = {d: [] for d in DNA_DIMS}
    mood_counts: dict[str, int] = defaultdict(int)

    for preset in presets:
        dna = preset["dna"]
        for dim in DNA_DIMS:
            val = dna.get(dim)
            if isinstance(val, (int, float)):
                dim_values[dim].append(float(val))
        mood_counts[preset["mood"]] += 1

    mean_dna = {d: round(mean(dim_values[d]), 4) for d in DNA_DIMS}
    min_dna  = {d: round(min(dim_values[d]), 4) if dim_values[d] else 0.0 for d in DNA_DIMS}
    max_dna  = {d: round(max(dim_values[d]), 4) if dim_values[d] else 0.0 for d in DNA_DIMS}
    std_dna  = {d: round(std_dev(dim_values[d]), 4) for d in DNA_DIMS}

    fs = felix_score(mean_dna["brightness"], mean_dna["warmth"])

    # Overall versatility = mean std dev across all 6 dims
    versatility = round(mean([std_dna[d] for d in DNA_DIMS]), 4)

    # Mood distribution sorted by count desc
    mood_dist = dict(sorted(mood_counts.items(), key=lambda x: x[1], reverse=True))

    return {
        "engine": engine,
        "preset_count": len(presets),
        "mean_dna": mean_dna,
        "min_dna": min_dna,
        "max_dna": max_dna,
        "std_dna": std_dna,
        "felix_score": round(fs, 4),
        "versatility": versatility,
        "dominant_character": dominant_character(mean_dna),
        "mood_distribution": mood_dist,
    }

def build_profiles(groups: dict[str, list[dict]]) -> list[dict]:
    """Build sorted profile list (most feliX → most Oscar)."""
    profiles = [compute_engine_profile(eng, presets) for eng, presets in groups.items()]
    profiles.sort(key=lambda p: p["felix_score"], reverse=True)
    return profiles

# ---------------------------------------------------------------------------
# Report formatting
# ---------------------------------------------------------------------------

REPORT_HEADER = (
    "XO_OX XOmnibus — Engine DNA Profile Report\n"
    "==========================================\n"
    "Sorted: most feliX (1.0) → most Oscar (0.0)\n"
    "Versatility: mean std-dev across 6 DNA dims (higher = more varied)\n"
    "High versatility (≥0.18) = broad sonic range | Low (≤0.08) = consistent character\n"
)

COL_W = {
    "engine":     12,
    "presets":     7,
    "felix":       6,
    "character":  22,
    "bright":      7,
    "warm":        7,
    "move":        7,
    "dense":       7,
    "space":       7,
    "aggr":        7,
    "vers":        6,
    "top_mood":   14,
}

def _pad(s: str, w: int) -> str:
    return str(s)[:w].ljust(w)

def format_report(profiles: list[dict]) -> str:
    lines = [REPORT_HEADER, ""]

    header = (
        _pad("Engine", COL_W["engine"]) +
        _pad("Psts", COL_W["presets"]) +
        _pad("feliX", COL_W["felix"]) +
        _pad("Character", COL_W["character"]) +
        _pad("Bright", COL_W["bright"]) +
        _pad("Warm", COL_W["warm"]) +
        _pad("Move", COL_W["move"]) +
        _pad("Dense", COL_W["dense"]) +
        _pad("Space", COL_W["space"]) +
        _pad("Aggr", COL_W["aggr"]) +
        _pad("Vers", COL_W["vers"]) +
        _pad("Top Mood", COL_W["top_mood"])
    )
    sep = "-" * len(header)
    lines += [header, sep]

    VERSATILE_THRESHOLD = 0.18
    CONSISTENT_THRESHOLD = 0.08

    for p in profiles:
        md = p["mean_dna"]
        vers = p["versatility"]
        vers_flag = " [V]" if vers >= VERSATILE_THRESHOLD else (" [C]" if vers <= CONSISTENT_THRESHOLD else "    ")
        top_mood = next(iter(p["mood_distribution"]), "—")

        row = (
            _pad(p["engine"], COL_W["engine"]) +
            _pad(p["preset_count"], COL_W["presets"]) +
            _pad(f"{p['felix_score']:.3f}", COL_W["felix"]) +
            _pad(p["dominant_character"], COL_W["character"]) +
            _pad(f"{md['brightness']:.2f}", COL_W["bright"]) +
            _pad(f"{md['warmth']:.2f}", COL_W["warm"]) +
            _pad(f"{md['movement']:.2f}", COL_W["move"]) +
            _pad(f"{md['density']:.2f}", COL_W["dense"]) +
            _pad(f"{md['space']:.2f}", COL_W["space"]) +
            _pad(f"{md['aggression']:.2f}", COL_W["aggr"]) +
            _pad(f"{vers:.3f}{vers_flag}", COL_W["vers"] + 5) +
            _pad(top_mood, COL_W["top_mood"])
        )
        lines.append(row)

    lines += [
        sep,
        "",
        "Legend:",
        "  feliX score: (brightness - warmth + 1) / 2  →  0.0 = pure Oscar, 1.0 = pure feliX",
        "  [V] Versatile engine (mean std-dev ≥ 0.18)  — broad palette, suits many contexts",
        "  [C] Consistent engine (mean std-dev ≤ 0.08) — strong identity, predictable character",
        f"  Engines in report: {len(profiles)}",
        f"  Total presets analysed: {sum(p['preset_count'] for p in profiles)}",
    ]

    return "\n".join(lines) + "\n"

# ---------------------------------------------------------------------------
# Summary stats for JSON metadata block
# ---------------------------------------------------------------------------

def fleet_summary(profiles: list[dict]) -> dict:
    felix_scores = [p["felix_score"] for p in profiles]
    versatilities = [p["versatility"] for p in profiles]
    return {
        "engine_count": len(profiles),
        "total_presets": sum(p["preset_count"] for p in profiles),
        "most_felix_engine": profiles[0]["engine"] if profiles else None,
        "most_oscar_engine": profiles[-1]["engine"] if profiles else None,
        "most_versatile_engine": max(profiles, key=lambda p: p["versatility"])["engine"] if profiles else None,
        "most_consistent_engine": min(profiles, key=lambda p: p["versatility"])["engine"] if profiles else None,
        "fleet_mean_felix": round(mean(felix_scores), 4) if felix_scores else 0.0,
        "fleet_mean_versatility": round(mean(versatilities), 4) if versatilities else 0.0,
    }

# ---------------------------------------------------------------------------
# CLI entry point
# ---------------------------------------------------------------------------

def main() -> None:
    parser = argparse.ArgumentParser(
        description="Build 6D Sonic DNA profiles for every XOmnibus engine.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument(
        "--preset-dir",
        required=True,
        metavar="DIR",
        help="Root directory containing .xometa preset files (scanned recursively).",
    )
    parser.add_argument(
        "--output-json",
        default=None,
        metavar="FILE",
        help="Path for JSON output (default: Docs/engine_dna_profiles.json relative to preset-dir parent).",
    )
    parser.add_argument(
        "--output-report",
        default=None,
        metavar="FILE",
        help="Path for human-readable text report (default: stdout).",
    )
    args = parser.parse_args()

    preset_dir = Path(args.preset_dir).expanduser().resolve()
    if not preset_dir.is_dir():
        print(f"ERROR: preset-dir not found: {preset_dir}", file=sys.stderr)
        sys.exit(1)

    # Default JSON output path
    if args.output_json:
        json_path = Path(args.output_json).expanduser().resolve()
    else:
        json_path = preset_dir.parent / "Docs" / "engine_dna_profiles.json"

    print(f"Scanning presets in: {preset_dir}", file=sys.stderr)
    presets = load_presets(preset_dir)
    print(f"Loaded {len(presets)} presets with DNA data.", file=sys.stderr)

    if not presets:
        print("ERROR: no valid presets found. Check --preset-dir path.", file=sys.stderr)
        sys.exit(1)

    groups = group_by_engine(presets)
    print(f"Found {len(groups)} unique engines.", file=sys.stderr)

    profiles = build_profiles(groups)
    summary = fleet_summary(profiles)

    # Build JSON output
    output = {
        "_meta": {
            "tool": "xpn_engine_dna_profile_builder",
            "description": "Authoritative 6D Sonic DNA fingerprint per XOmnibus engine",
            "sort_order": "felix_score descending (most feliX → most Oscar)",
            "dna_dimensions": DNA_DIMS,
            "felix_score_formula": "(brightness - warmth + 1) / 2",
            "versatility_formula": "mean(std_dev per dimension)",
        },
        "fleet_summary": summary,
        "engines": profiles,
    }

    # Write JSON
    json_path.parent.mkdir(parents=True, exist_ok=True)
    with json_path.open("w", encoding="utf-8") as fh:
        json.dump(output, fh, indent=2, ensure_ascii=False)
    print(f"JSON written: {json_path}", file=sys.stderr)

    # Generate report
    report = format_report(profiles)

    if args.output_report:
        report_path = Path(args.output_report).expanduser().resolve()
        report_path.parent.mkdir(parents=True, exist_ok=True)
        with report_path.open("w", encoding="utf-8") as fh:
            fh.write(report)
        print(f"Report written: {report_path}", file=sys.stderr)
    else:
        print(report)

    # Print fleet summary to stderr for quick reference
    print("\n--- Fleet Summary ---", file=sys.stderr)
    for k, v in summary.items():
        print(f"  {k}: {v}", file=sys.stderr)

if __name__ == "__main__":
    main()
