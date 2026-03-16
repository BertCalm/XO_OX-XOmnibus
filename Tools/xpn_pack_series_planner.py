#!/usr/bin/env python3
"""
xpn_pack_series_planner.py — XO_OX Multi-Pack Series Planner

Plans a coherent multi-pack series by analyzing existing preset DNA coverage
and generating targeted pack briefs with DNA goals, mood distribution, and naming ideas.

6D Sonic DNA: brightness, warmth, movement, density, space, aggression (0.0–1.0 each)

Usage:
    python xpn_pack_series_planner.py --spec series.json [--presets-dir ../Presets]
    python xpn_pack_series_planner.py --name "CARBON PAPER" --engines "Overdub,Oracle,Obscura" \
        --packs 4 [--presets-dir ../Presets]

Example spec JSON:
    {
      "name": "CARBON PAPER",
      "engines": ["Overdub", "Oracle", "Obscura"],
      "target_centroid": {"brightness":0.4,"warmth":0.6,"movement":0.5,"density":0.6,"space":0.7,"aggression":0.3},
      "pack_count": 4
    }
"""

import argparse
import json
import math
import os
import statistics
import sys
from collections import defaultdict
from pathlib import Path

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

DNA_DIMS = ["brightness", "warmth", "movement", "density", "space", "aggression"]

# Zone thresholds
ZONE_LOW_MAX  = 0.33
ZONE_HIGH_MIN = 0.67

MOODS = ["Foundation", "Atmosphere", "Entangled", "Prism", "Flux", "Aether", "Family"]

# Default target centroid (balanced neutral)
DEFAULT_CENTROID = {d: 0.5 for d in DNA_DIMS}

# Default pack count
DEFAULT_PACK_COUNT = 5

# Default preset counts per pack role
PACK_PRESET_COUNTS = {
    "Hook":      20,
    "Develop":   20,
    "Challenge": 16,
    "Resolve":   20,
    "Vault":     12,  # curations
}

# Series arc roles (in order)
PACK_ROLES = ["Hook", "Develop", "Challenge", "Resolve", "Vault"]

# Human-readable DNA dimension labels
DIM_LABELS = {
    "brightness": "brightness",
    "warmth":     "warmth",
    "movement":   "movement",
    "density":    "density",
    "space":      "space",
    "aggression": "aggression",
}

# Pole descriptors for high/low DNA values
DIM_HIGH = {
    "brightness": "bright/cutting",
    "warmth":     "warm/organic",
    "movement":   "animated/evolving",
    "density":    "dense/layered",
    "space":      "expansive/reverberant",
    "aggression": "aggressive/abrasive",
}
DIM_LOW = {
    "brightness": "dark/muffled",
    "warmth":     "cold/clinical",
    "movement":   "static/stable",
    "density":    "sparse/minimal",
    "space":      "dry/intimate",
    "aggression": "gentle/smooth",
}

# ---------------------------------------------------------------------------
# Preset loading
# ---------------------------------------------------------------------------

def load_presets(presets_dir: Path, engine_filter: list[str]) -> list[dict]:
    """Load .xometa presets that contain at least one of the target engines."""
    presets = []
    engine_lower = [e.lower() for e in engine_filter]

    for path in sorted(presets_dir.rglob("*.xometa")):
        try:
            with open(path, "r", encoding="utf-8") as f:
                data = json.load(f)
        except (json.JSONDecodeError, OSError):
            continue

        dna = data.get("dna")
        if not dna or not all(dim in dna for dim in DNA_DIMS):
            continue

        engines_in_preset = [e.lower() for e in data.get("engines", [])]
        if not any(ef in e for ef in engine_lower for e in engines_in_preset):
            continue

        presets.append({
            "name":    data.get("name", path.stem),
            "path":    str(path),
            "engines": data.get("engines", []),
            "mood":    data.get("mood", "Unknown"),
            "dna":     {dim: float(dna[dim]) for dim in DNA_DIMS},
        })

    return presets

# ---------------------------------------------------------------------------
# DNA math helpers
# ---------------------------------------------------------------------------

def euclidean(a: dict, b: dict) -> float:
    return math.sqrt(sum((a[d] - b[d]) ** 2 for d in DNA_DIMS))

def centroid(presets: list[dict]) -> dict:
    if not presets:
        return {d: 0.5 for d in DNA_DIMS}
    return {d: statistics.mean(p["dna"][d] for p in presets) for d in DNA_DIMS}

def dna_zone(val: float) -> str:
    if val <= ZONE_LOW_MAX:
        return "low"
    if val >= ZONE_HIGH_MIN:
        return "high"
    return "mid"

def dna_coverage(presets: list[dict]) -> dict:
    """Per-dimension: mean, stdev, zone distribution."""
    result = {}
    for d in DNA_DIMS:
        vals = [p["dna"][d] for p in presets]
        zones = defaultdict(int)
        for v in vals:
            zones[dna_zone(v)] += 1
        result[d] = {
            "mean":  round(statistics.mean(vals), 3) if vals else 0.5,
            "stdev": round(statistics.stdev(vals), 3) if len(vals) > 1 else 0.0,
            "zones": dict(zones),
            "count": len(vals),
        }
    return result

def mood_distribution(presets: list[dict]) -> dict:
    dist = defaultdict(int)
    for p in presets:
        dist[p["mood"]] += 1
    return dict(dist)

def dim_descriptor(dim: str, val: float) -> str:
    z = dna_zone(val)
    if z == "high":
        return DIM_HIGH[dim]
    if z == "low":
        return DIM_LOW[dim]
    return f"balanced {dim}"

# ---------------------------------------------------------------------------
# Gap analysis
# ---------------------------------------------------------------------------

def gap_analysis(coverage: dict, target_centroid: dict) -> list[dict]:
    """Return dimensions sorted by how far current mean is from the target centroid."""
    gaps = []
    for d in DNA_DIMS:
        current_mean = coverage[d]["mean"]
        target_val   = target_centroid[d]
        delta        = target_val - current_mean  # positive = need to push up
        gaps.append({
            "dim":          d,
            "current_mean": current_mean,
            "target":       target_val,
            "delta":        round(delta, 3),
            "gap_magnitude": round(abs(delta), 3),
            "direction":    "increase" if delta > 0.05 else ("decrease" if delta < -0.05 else "on-target"),
        })
    gaps.sort(key=lambda x: x["gap_magnitude"], reverse=True)
    return gaps

# ---------------------------------------------------------------------------
# Series arc design
# ---------------------------------------------------------------------------

def design_pack_dna(role: str, target_centroid: dict, gaps: list[dict],
                    existing_centroid: dict) -> dict:
    """
    Design a DNA target for a given pack role.
    Hook:      pull toward target centroid; add slight discoverability (mid values)
    Develop:   push hardest gap dimension to its target extreme; rest near centroid
    Challenge: push 2 largest-gap dims to extremes, compress others to mid
    Resolve:   near-mirror of Challenge back toward centroid; high space/warmth
    Vault:     span the full series range — composite of Hook + Challenge
    """
    tc = dict(target_centroid)

    if role == "Hook":
        # Approachable: blend toward center but drift toward target centroid
        dna = {}
        for d in DNA_DIMS:
            # 70% target centroid, 30% neutral 0.5 → approachable
            dna[d] = round(0.7 * tc[d] + 0.3 * 0.5, 3)
        return dna

    if role == "Develop":
        # Push primary gap dimension hard, pull others toward centroid
        dna = dict(tc)
        if gaps:
            primary = gaps[0]["dim"]
            # push dimension all the way toward its target
            t = tc[primary]
            dna[primary] = round(min(1.0, max(0.0, t + (t - 0.5) * 0.5)), 3)
        return dna

    if role == "Challenge":
        # Push top 2 gap dimensions to extremes; squeeze others to mid (0.45–0.55)
        dna = {}
        pushed = {g["dim"] for g in gaps[:2]}
        for d in DNA_DIMS:
            if d in pushed:
                t = tc[d]
                # amplify away from 0.5
                extreme = round(min(1.0, max(0.0, 0.5 + (t - 0.5) * 1.8)), 3)
                dna[d] = extreme
            else:
                # compress toward mid
                dna[d] = round(0.5 + (tc[d] - 0.5) * 0.3, 3)
        return dna

    if role == "Resolve":
        # Pull back; warm + spacious, complement of Challenge
        dna = {}
        for d in DNA_DIMS:
            # drift toward centroid but with warmth/space bias
            dna[d] = round(0.6 * tc[d] + 0.4 * 0.5, 3)
        # boost warmth and space for satisfying resolution
        dna["warmth"] = round(min(1.0, dna["warmth"] + 0.12), 3)
        dna["space"]  = round(min(1.0, dna["space"]  + 0.10), 3)
        return dna

    if role == "Vault":
        # Fan-favorite curation: DNA centroid of the whole series (just use target)
        return dict(tc)

    return dict(tc)


def mood_targets_for_pack(role: str, pack_count: int) -> dict:
    """Suggest mood distribution (counts) for a pack."""
    # Base distribution weights per role
    if role == "Hook":
        weights = {"Foundation": 6, "Atmosphere": 5, "Prism": 4, "Flux": 3, "Entangled": 2}
    elif role == "Develop":
        weights = {"Flux": 6, "Prism": 5, "Atmosphere": 4, "Foundation": 3, "Entangled": 2}
    elif role == "Challenge":
        weights = {"Aether": 5, "Entangled": 4, "Flux": 4, "Prism": 3}
    elif role == "Resolve":
        weights = {"Foundation": 6, "Atmosphere": 5, "Aether": 4, "Prism": 3, "Entangled": 2}
    else:  # Vault
        weights = {"Foundation": 3, "Flux": 3, "Prism": 3, "Entangled": 2, "Aether": 1}

    total_w = sum(weights.values())
    preset_count = PACK_PRESET_COUNTS.get(role, 20)
    dist = {}
    running = 0
    items = list(weights.items())
    for i, (mood, w) in enumerate(items):
        if i == len(items) - 1:
            dist[mood] = preset_count - running
        else:
            count = max(1, round(w / total_w * preset_count))
            dist[mood] = count
            running += count
    return dist


def generate_name_suggestions(series_name: str, role: str, pack_index: int,
                               pack_dna: dict) -> list[str]:
    """Generate 3 evocative subtitle suggestions for a pack."""
    # Adjectives derived from dominant DNA dimensions
    high_dims = [d for d in DNA_DIMS if pack_dna[d] >= ZONE_HIGH_MIN]
    low_dims  = [d for d in DNA_DIMS if pack_dna[d] <= ZONE_LOW_MAX]

    dim_words_high = {
        "brightness": ["Radiant", "Searing", "Lucid"],
        "warmth":     ["Amber", "Ember", "Velvet"],
        "movement":   ["Kinetic", "Restless", "Shifting"],
        "density":    ["Layered", "Saturated", "Thick"],
        "space":      ["Vast", "Open", "Cathedral"],
        "aggression": ["Raw", "Harsh", "Cutting"],
    }
    dim_words_low = {
        "brightness": ["Shadowed", "Muted", "Dim"],
        "warmth":     ["Cold", "Clinical", "Frost"],
        "movement":   ["Still", "Frozen", "Fixed"],
        "density":    ["Sparse", "Skeletal", "Lean"],
        "space":      ["Dry", "Tight", "Pressed"],
        "aggression": ["Smooth", "Gentle", "Soft"],
    }

    role_anchors = {
        "Hook":      ["Volume I", "First Contact", "Entry Point", "Open Door"],
        "Develop":   ["Volume II", "Deep Cut", "Descent", "Turning Point"],
        "Challenge": ["Volume III", "Outer Limit", "Edge Case", "The Extreme"],
        "Resolve":   ["Volume IV", "Homecoming", "Resolution", "Return"],
        "Vault":     ["Archive", "The Vault", "Best Of", "Collected"],
    }

    suggestions = []

    # Suggestion 1: role-based anchor
    suggestions.append(f"{series_name}: {role_anchors[role][0]}")

    # Suggestion 2: lead high-dim word + role flavor
    if high_dims:
        word = dim_words_high[high_dims[0]][pack_index % 3]
        suggestions.append(f"{series_name}: {word} {role_anchors[role][1]}")
    elif low_dims:
        word = dim_words_low[low_dims[0]][pack_index % 3]
        suggestions.append(f"{series_name}: {word} {role_anchors[role][1]}")
    else:
        suggestions.append(f"{series_name}: {role_anchors[role][2]}")

    # Suggestion 3: numeric subtitle
    suggestions.append(f"{series_name} {pack_index + 1:02d} — {role}")

    return suggestions


def build_series_plan(spec: dict, presets: list[dict]) -> dict:
    """Core planning logic. Returns the full series plan dict."""
    series_name   = spec["name"]
    engines       = spec["engines"]
    target_ct     = {d: float(spec.get("target_centroid", DEFAULT_CENTROID).get(d, 0.5))
                     for d in DNA_DIMS}
    pack_count    = int(spec.get("pack_count", DEFAULT_PACK_COUNT))
    roles         = PACK_ROLES[:pack_count]

    # --- Existing preset analysis ---
    existing_centroid = centroid(presets)
    coverage          = dna_coverage(presets)
    mood_dist         = mood_distribution(presets)
    gaps              = gap_analysis(coverage, target_ct)

    existing_summary = {
        "preset_count":       len(presets),
        "centroid":           {d: round(existing_centroid[d], 3) for d in DNA_DIMS},
        "distance_to_target": round(euclidean(existing_centroid, target_ct), 3),
        "coverage_by_dim":    coverage,
        "mood_distribution":  mood_dist,
        "gap_analysis":       gaps,
    }

    # --- Pack plans ---
    packs = []
    for i, role in enumerate(roles):
        pack_dna    = design_pack_dna(role, target_ct, gaps, existing_centroid)
        mood_dist_p = mood_targets_for_pack(role, pack_count)
        names       = generate_name_suggestions(series_name, role, i, pack_dna)
        preset_n    = PACK_PRESET_COUNTS.get(role, 20)

        # Describe the DNA target in human terms
        dna_narrative = ", ".join(
            f"{DIM_LABELS[d]}: {dim_descriptor(d, pack_dna[d])} ({pack_dna[d]:.2f})"
            for d in DNA_DIMS
        )

        # What gaps this pack addresses
        addressed_gaps = []
        for g in gaps[:3]:
            d = g["dim"]
            delta = pack_dna[d] - existing_centroid[d]
            if abs(delta) >= 0.05:
                direction = "higher" if delta > 0 else "lower"
                addressed_gaps.append(
                    f"{d} pushed {direction} ({existing_centroid[d]:.2f} → {pack_dna[d]:.2f})"
                )

        packs.append({
            "pack_number":      i + 1,
            "role":             role,
            "name_suggestions": names,
            "preset_count":     preset_n,
            "dna_target":       pack_dna,
            "dna_narrative":    dna_narrative,
            "mood_distribution": mood_dist_p,
            "addressed_gaps":   addressed_gaps,
        })

    return {
        "series_name":    series_name,
        "engines":        engines,
        "target_centroid": {d: round(target_ct[d], 3) for d in DNA_DIMS},
        "pack_count":     pack_count,
        "existing_analysis": existing_summary,
        "packs":          packs,
    }

# ---------------------------------------------------------------------------
# Human-readable summary
# ---------------------------------------------------------------------------

def render_summary(plan: dict) -> str:
    lines = []
    w = 72

    def hr(char="─"):
        return char * w

    def section(title):
        lines.append("")
        lines.append(hr("═"))
        lines.append(f"  {title}")
        lines.append(hr("═"))

    def subsection(title):
        lines.append(f"\n  ── {title} ──")

    lines.append(hr("═"))
    lines.append(f"  SERIES PLAN: {plan['series_name']}")
    lines.append(f"  Engines: {', '.join(plan['engines'])}")
    lines.append(f"  Packs: {plan['pack_count']}   Total presets: "
                 f"{sum(p['preset_count'] for p in plan['packs'])}")
    lines.append(hr("═"))

    # Existing analysis
    ea = plan["existing_analysis"]
    section("EXISTING PRESET ANALYSIS")
    lines.append(f"\n  Presets found: {ea['preset_count']}")
    lines.append(f"  Current centroid vs target (distance: {ea['distance_to_target']:.3f}):\n")
    tc = plan["target_centroid"]
    ec = ea["centroid"]
    for d in DNA_DIMS:
        bar_curr = int(ec[d] * 20)
        bar_targ = int(tc[d] * 20)
        arrow = "↑" if tc[d] > ec[d] + 0.05 else ("↓" if tc[d] < ec[d] - 0.05 else "✓")
        lines.append(f"    {d:12s}  current {ec[d]:.2f}  target {tc[d]:.2f}  {arrow}")

    subsection("DNA Gap Priority (biggest → smallest)")
    for g in ea["gap_analysis"]:
        if g["gap_magnitude"] < 0.02:
            status = "on-target"
        elif g["direction"] == "increase":
            status = f"need +{g['gap_magnitude']:.2f} (push higher)"
        else:
            status = f"need -{g['gap_magnitude']:.2f} (pull lower)"
        lines.append(f"    {g['dim']:12s}  {status}")

    subsection("Mood Coverage")
    if ea["mood_distribution"]:
        for mood, count in sorted(ea["mood_distribution"].items(),
                                   key=lambda x: -x[1]):
            lines.append(f"    {mood:14s} {count:3d} presets")
    else:
        lines.append("    (no existing presets found for these engines)")

    # Pack plans
    section("SERIES ARC — PACK PLANS")
    for pack in plan["packs"]:
        lines.append(f"\n  ┌─ Pack {pack['pack_number']}: {pack['role'].upper()} "
                     f"({pack['preset_count']} presets) " + "─" * 20)
        lines.append(f"  │")
        lines.append(f"  │  Name ideas:")
        for name in pack["name_suggestions"]:
            lines.append(f"  │    • {name}")
        lines.append(f"  │")
        lines.append(f"  │  DNA Target:")
        for d in DNA_DIMS:
            v = pack["dna_target"][d]
            bar = "█" * int(v * 20) + "░" * (20 - int(v * 20))
            lines.append(f"  │    {d:12s} [{bar}] {v:.2f}")
        lines.append(f"  │")
        lines.append(f"  │  Character: {pack['dna_narrative'][:68]}")
        if len(pack["dna_narrative"]) > 68:
            # wrap remainder
            rest = pack["dna_narrative"][68:]
            lines.append(f"  │             {rest[:68]}")
        lines.append(f"  │")
        lines.append(f"  │  Mood distribution:")
        for mood, count in sorted(pack["mood_distribution"].items(),
                                   key=lambda x: -x[1]):
            lines.append(f"  │    {mood:14s} {count:3d}")
        if pack["addressed_gaps"]:
            lines.append(f"  │")
            lines.append(f"  │  Gaps addressed:")
            for g in pack["addressed_gaps"]:
                lines.append(f"  │    • {g}")
        lines.append(f"  └" + "─" * (w - 3))

    # Series DNA arc summary
    section("DNA ARC ACROSS SERIES")
    header = f"  {'Dim':12s} | " + " | ".join(
        f"P{p['pack_number']} {p['role'][:4]:4s}" for p in plan["packs"]
    )
    lines.append(header)
    lines.append("  " + "─" * (len(header) - 2))
    for d in DNA_DIMS:
        row = f"  {d:12s} | "
        row += " | ".join(
            f"{p['dna_target'][d]:.2f}      "[:10] for p in plan["packs"]
        )
        lines.append(row)

    lines.append("")
    lines.append(hr("═"))
    lines.append("  End of series plan.")
    lines.append(hr("═"))
    lines.append("")

    return "\n".join(lines)

# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def parse_args():
    parser = argparse.ArgumentParser(
        description="Plan a multi-pack series for XO_OX preset collections."
    )
    parser.add_argument("--spec", metavar="FILE.json",
                        help="Series spec JSON file")
    parser.add_argument("--name", default=None,
                        help="Series name (overrides spec)")
    parser.add_argument("--engines", default=None,
                        help="Comma-separated engine names, e.g. Overdub,Oracle (overrides spec)")
    parser.add_argument("--packs", type=int, default=None,
                        help="Number of packs 1–5 (overrides spec)")
    parser.add_argument("--centroid", default=None, metavar="JSON",
                        help='Target centroid as inline JSON, e.g. \'{"brightness":0.4,"warmth":0.6,...}\'')
    parser.add_argument("--presets-dir", default=None, metavar="DIR",
                        help="Path to Presets directory (default: auto-detect from script location)")
    parser.add_argument("--output-json", default=None, metavar="FILE",
                        help="Write plan JSON to this file instead of stdout")
    parser.add_argument("--output-summary", default=None, metavar="FILE",
                        help="Write human-readable summary to this file instead of stdout")
    return parser.parse_args()


def resolve_presets_dir(cli_dir) -> Path:
    """Find the Presets directory relative to the tool's location."""
    if cli_dir:
        p = Path(cli_dir)
        if not p.is_dir():
            print(f"ERROR: --presets-dir '{cli_dir}' is not a directory.", file=sys.stderr)
            sys.exit(1)
        return p
    # Default: two levels up from Tools/
    script_dir = Path(__file__).resolve().parent
    candidates = [
        script_dir.parent / "Presets",
        script_dir.parent / "Presets" / "XOmnibus",
        script_dir / "Presets",
    ]
    for c in candidates:
        if c.is_dir():
            return c
    print("WARNING: Could not find Presets directory automatically. "
          "Pass --presets-dir explicitly. Analysis will proceed with 0 existing presets.",
          file=sys.stderr)
    return script_dir  # fallback — will find no .xometa files, but won't crash


def main():
    args = parse_args()

    # --- Build spec dict ---
    spec = {}

    if args.spec:
        spec_path = Path(args.spec)
        if not spec_path.is_file():
            print(f"ERROR: spec file '{args.spec}' not found.", file=sys.stderr)
            sys.exit(1)
        with open(spec_path, "r", encoding="utf-8") as f:
            spec = json.load(f)

    # CLI overrides
    if args.name:
        spec["name"] = args.name
    if args.engines:
        spec["engines"] = [e.strip() for e in args.engines.split(",") if e.strip()]
    if args.packs:
        spec["pack_count"] = min(5, max(1, args.packs))
    if args.centroid:
        try:
            spec["target_centroid"] = json.loads(args.centroid)
        except json.JSONDecodeError as e:
            print(f"ERROR: --centroid JSON parse failed: {e}", file=sys.stderr)
            sys.exit(1)

    # Validate required fields
    if not spec.get("name"):
        print("ERROR: Series name required. Use --name or set 'name' in spec JSON.", file=sys.stderr)
        sys.exit(1)
    if not spec.get("engines"):
        print("ERROR: At least one engine required. Use --engines or set 'engines' in spec JSON.",
              file=sys.stderr)
        sys.exit(1)

    # Fill defaults
    spec.setdefault("target_centroid", DEFAULT_CENTROID)
    spec.setdefault("pack_count", DEFAULT_PACK_COUNT)

    # Validate centroid keys
    tc = spec["target_centroid"]
    for d in DNA_DIMS:
        if d not in tc:
            tc[d] = 0.5
        tc[d] = min(1.0, max(0.0, float(tc[d])))

    pack_count = min(5, max(1, int(spec["pack_count"])))
    spec["pack_count"] = pack_count

    # --- Load presets ---
    presets_dir = resolve_presets_dir(args.presets_dir)
    presets = load_presets(presets_dir, spec["engines"])

    # --- Build plan ---
    plan = build_series_plan(spec, presets)

    # --- Output ---
    plan_json    = json.dumps(plan, indent=2)
    plan_summary = render_summary(plan)

    if args.output_json:
        with open(args.output_json, "w", encoding="utf-8") as f:
            f.write(plan_json)
        print(f"Plan JSON written to: {args.output_json}")
    else:
        print(plan_json)

    if args.output_summary:
        with open(args.output_summary, "w", encoding="utf-8") as f:
            f.write(plan_summary)
        print(f"Summary written to: {args.output_summary}")
    else:
        print(plan_summary)

    sys.exit(0)


if __name__ == "__main__":
    main()
