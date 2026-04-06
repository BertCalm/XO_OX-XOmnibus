#!/usr/bin/env python3
"""
xpn_preset_crossfade_designer.py — Crossfade Chain Designer for XOceanus / MPC

Designs "crossfade chains" — sequences of presets where consecutive pairs can be
smoothly morphed on MPC using Q-Link knobs. Useful for live performance and packs
that tell a sonic story.

Usage:
    python xpn_preset_crossfade_designer.py --preset-dir <dir> [--engine FILTER]
        [--start "Preset Name"] [--length 16] [--output chain.txt]

DNA axes: brightness, warmth, movement, density, space, aggression
"""

import argparse
import json
import math
import sys
from pathlib import Path
from typing import Dict, List, Optional

# ─── Mood Complementarity ──────────────────────────────────────────────────────

MOOD_ORDER = ["Foundation", "Atmosphere", "Entangled", "Prism", "Flux", "Aether", "Family",
              "Submerged", "Coupling", "Crystalline", "Deep", "Ethereal", "Kinetic", "Luminous",
              "Organic", "Shadow"]

COMPLEMENTARY_MOODS = {
    "Foundation": "Aether",
    "Aether": "Foundation",
    "Atmosphere": "Flux",
    "Flux": "Atmosphere",
    "Luminous": "Shadow",
    "Shadow": "Luminous",
}

DNA_KEYS = ["brightness", "warmth", "movement", "density", "space", "aggression"]

SPARKLINE_CHARS = " ▁▂▃▄▅▆▇█"


# ─── Data Loading ──────────────────────────────────────────────────────────────

def load_presets(preset_dir: Path, engine_filter: Optional[str]) -> List[dict]:
    """Load all .xometa files from a directory (recursive)."""
    presets = []
    for path in sorted(preset_dir.rglob("*.xometa")):
        try:
            data = json.loads(path.read_text(encoding="utf-8"))
        except (json.JSONDecodeError, OSError) as e:
            print(f"  [warn] skipping {path.name}: {e}", file=sys.stderr)
            continue

        # Normalise engine list
        engines = data.get("engines", [])
        if isinstance(engines, str):
            engines = [engines]

        # Engine filter
        if engine_filter:
            ef = engine_filter.lower()
            if not any(ef in e.lower() for e in engines):
                continue

        # Normalise DNA
        raw_dna = data.get("dna") or {}
        dna = {k: float(raw_dna.get(k, 0.5)) for k in DNA_KEYS}

        presets.append({
            "name": data.get("name", path.stem),
            "mood": data.get("mood", "Foundation"),
            "engines": engines,
            "dna": dna,
            "path": str(path),
            "tags": data.get("tags", []),
        })

    return presets


# ─── Morphability Scoring ──────────────────────────────────────────────────────

def dna_distance(a: dict, b: dict) -> float:
    """Euclidean distance in 6D DNA space (max possible = sqrt(6) ≈ 2.449)."""
    return math.sqrt(sum((a["dna"][k] - b["dna"][k]) ** 2 for k in DNA_KEYS))


def morphability_score(a: dict, b: dict) -> float:
    """
    Returns a score in [0, 1] representing how smoothly A→B can be morphed.

    Factors:
      - DNA proximity: closer = higher score (weight 0.55)
      - Same engine:   +0.25 bonus
      - Same mood:     +0.10 bonus
      - Complementary moods: +0.05 bonus
    """
    MAX_DIST = math.sqrt(6)  # theoretical maximum distance

    dist = dna_distance(a, b)
    dna_score = 1.0 - (dist / MAX_DIST)  # [0, 1]

    engine_bonus = 0.25 if set(a["engines"]) & set(b["engines"]) else 0.0

    mood_a, mood_b = a["mood"], b["mood"]
    if mood_a == mood_b:
        mood_bonus = 0.10
    elif COMPLEMENTARY_MOODS.get(mood_a) == mood_b:
        mood_bonus = 0.05
    else:
        mood_bonus = 0.0

    # Weighted combination (components already [0,1])
    raw = 0.55 * dna_score + engine_bonus + mood_bonus
    # Clamp to [0, 1]
    return min(1.0, max(0.0, raw))


# ─── Chain Finding ─────────────────────────────────────────────────────────────

def fleet_centroid(presets: List[dict]) -> Dict[str, float]:
    n = len(presets)
    if n == 0:
        return {k: 0.5 for k in DNA_KEYS}
    return {k: sum(p["dna"][k] for p in presets) / n for k in DNA_KEYS}


def find_most_central(presets: List[dict]) -> dict:
    """Return the preset closest to the fleet DNA centroid."""
    centroid = fleet_centroid(presets)
    centroid_preset = {"dna": centroid}
    return min(presets, key=lambda p: dna_distance(p, centroid_preset))


def build_chain(presets: List[dict], start_preset: dict, length: int) -> List[dict]:
    """
    Greedy path: at each step pick the unvisited preset with highest morphability
    to the current tail. Returns a chain of `length` presets (or fewer if the
    pool is exhausted).
    """
    pool = [p for p in presets if p is not start_preset]
    chain = [start_preset]

    while pool and len(chain) < length:
        current = chain[-1]
        best = max(pool, key=lambda p: morphability_score(current, p))
        chain.append(best)
        pool.remove(best)

    return chain


# ─── Visualisation ─────────────────────────────────────────────────────────────

def sparkline(values: list[float], width: int = 1) -> str:
    """Map a list of [0,1] floats to sparkline characters."""
    chars = []
    for v in values:
        idx = min(int(v * (len(SPARKLINE_CHARS) - 1)), len(SPARKLINE_CHARS) - 1)
        chars.append(SPARKLINE_CHARS[idx] * width)
    return "".join(chars)


def render_dna_trajectories(chain: List[dict]) -> List[str]:
    """Return 6 sparkline rows, one per DNA axis."""
    lines = []
    for key in DNA_KEYS:
        values = [p["dna"][key] for p in chain]
        bar = sparkline(values)
        lines.append(f"  {key:<12} {bar}")
    return lines


def render_chain_report(chain: list[dict], length: int) -> str:
    """Full ASCII report for the chain."""
    lines = []
    lines.append("=" * 70)
    lines.append(f"  XOceanus Crossfade Chain  ({len(chain)} presets)")
    lines.append("=" * 70)
    lines.append("")

    # Pad assignment
    lines.append("  MPC Bank Assignment")
    lines.append("  " + "─" * 50)
    bank = "A"
    pad_num = 1
    for i, preset in enumerate(chain):
        # Score to next
        if i < len(chain) - 1:
            score = morphability_score(preset, chain[i + 1])
            arrow = f"→ {score:.2f}"
        else:
            arrow = "   (end)"

        # Bank boundaries at 16
        if pad_num > 16:
            bank = chr(ord(bank) + 1)
            pad_num = 1
            lines.append("")

        lines.append(
            f"  {bank}{pad_num:02d}  {preset['name']:<30}  "
            f"[{', '.join(preset['engines'][:2])}]  "
            f"{preset['mood']:<12}  {arrow}"
        )
        pad_num += 1

    lines.append("")
    lines.append("  DNA Trajectory  (left → right = chain order)")
    lines.append("  " + "─" * 50)
    lines.extend(render_dna_trajectories(chain))
    lines.append("")

    # Per-step morphability summary
    if len(chain) > 1:
        scores = [
            morphability_score(chain[i], chain[i + 1])
            for i in range(len(chain) - 1)
        ]
        avg = sum(scores) / len(scores)
        min_s = min(scores)
        max_s = max(scores)
        lines.append(
            f"  Morphability  avg={avg:.3f}  min={min_s:.3f}  max={max_s:.3f}"
        )
        bar = sparkline(scores)
        lines.append(f"  Step scores   {bar}")

    lines.append("")
    lines.append("=" * 70)
    return "\n".join(lines)


def render_preset_list(chain: list[dict]) -> str:
    """Plain ordered list with DNA values for copy-paste."""
    lines = ["# Crossfade Chain — Preset List", ""]
    for i, p in enumerate(chain, 1):
        dna_str = "  ".join(f"{k[0]}={p['dna'][k]:.2f}" for k in DNA_KEYS)
        lines.append(f"{i:>2}. {p['name']}")
        lines.append(f"    engines={', '.join(p['engines'])}  mood={p['mood']}")
        lines.append(f"    dna: {dna_str}")
        lines.append(f"    path: {p['path']}")
        lines.append("")
    return "\n".join(lines)


# ─── CLI ───────────────────────────────────────────────────────────────────────

def parse_args():
    parser = argparse.ArgumentParser(
        description="Design MPC-ready crossfade chains from .xometa presets.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument(
        "--preset-dir", required=True, type=Path,
        help="Directory to scan for .xometa files (recursive)",
    )
    parser.add_argument(
        "--engine", default=None,
        help="Filter to presets that include this engine name (case-insensitive substring)",
    )
    parser.add_argument(
        "--start", default=None,
        help='Name of the preset to start the chain from (exact match)',
    )
    parser.add_argument(
        "--length", type=int, default=16, choices=[8, 16, 32],
        help="Chain length: 8 (1 bank), 16 (2 banks), 32 (4 banks). Default: 16",
    )
    parser.add_argument(
        "--output", type=Path, default=None,
        help="Write report to this file (stdout if omitted)",
    )
    parser.add_argument(
        "--list-only", action="store_true",
        help="Output plain preset list instead of full ASCII report",
    )
    return parser.parse_args()


def main():
    args = parse_args()

    if not args.preset_dir.is_dir():
        print(f"Error: --preset-dir '{args.preset_dir}' is not a directory.", file=sys.stderr)
        sys.exit(1)

    print(f"Loading presets from: {args.preset_dir}", file=sys.stderr)
    presets = load_presets(args.preset_dir, args.engine)

    if not presets:
        print("No .xometa presets found (check --engine filter).", file=sys.stderr)
        sys.exit(1)

    print(f"  Loaded {len(presets)} presets", file=sys.stderr)

    # Resolve start preset
    if args.start:
        candidates = [p for p in presets if p["name"].lower() == args.start.lower()]
        if not candidates:
            # Fuzzy fallback: substring
            candidates = [p for p in presets if args.start.lower() in p["name"].lower()]
        if not candidates:
            print(
                f"Error: no preset matching '{args.start}' found. "
                "Use --list to see available names.",
                file=sys.stderr,
            )
            sys.exit(1)
        start_preset = candidates[0]
        print(f"  Start preset: {start_preset['name']}", file=sys.stderr)
    else:
        start_preset = find_most_central(presets)
        print(f"  Auto-selected central preset: {start_preset['name']}", file=sys.stderr)

    # Build chain
    chain = build_chain(presets, start_preset, args.length)
    print(f"  Chain length: {len(chain)} (requested {args.length})", file=sys.stderr)

    # Render report
    if args.list_only:
        report = render_preset_list(chain)
    else:
        report = render_chain_report(chain, args.length) + "\n\n" + render_preset_list(chain)

    # Output
    if args.output:
        args.output.write_text(report, encoding="utf-8")
        print(f"  Report written to: {args.output}", file=sys.stderr)
    else:
        print(report)


if __name__ == "__main__":
    main()
