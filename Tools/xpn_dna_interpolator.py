#!/usr/bin/env python3
"""
XPN DNA Interpolator — XO_OX Designs
=====================================
Generates a smooth DNA-interpolated sequence of presets between two .xometa
files — useful for building evolution packs or finding "bridge" presets
between sonic poles.

Given two .xometa preset files (start and end), this tool:
  1. Reads the 6D sonic_dna from each (brightness, warmth, movement, density,
     space, aggression)
  2. Interpolates N steps between them (linear, ease-in, ease-out, or
     ease-in-out)
  3. For each interpolated DNA point, searches a presets directory to find the
     closest real preset by Euclidean DNA distance
  4. Outputs a sequenced list: start → closest_match_1 → ... → end

Deduplication: no preset appears twice. If the closest match is already used,
the next closest is selected.

CLI:
    python xpn_dna_interpolator.py start.xometa end.xometa \\
        --presets-dir ./Presets/XOlokun/ \\
        --steps 8 \\
        --easing ease-in-out \\
        --engine OBLONG \\
        --format text

    python xpn_dna_interpolator.py start.xometa end.xometa \\
        --presets-dir ./Presets/XOlokun/ \\
        --steps 6 \\
        --format json

Easing curves:
    linear       — uniform t each step
    ease-in      — t² (slow start, fast finish)
    ease-out     — 1-(1-t)² (fast start, slow finish)
    ease-in-out  — smoothstep: 3t²-2t³ (slow both ends)
"""

import argparse
import glob
import json
import math
import os
import sys

# ---------------------------------------------------------------------------
# DNA helpers
# ---------------------------------------------------------------------------

DNA_KEYS = ["brightness", "warmth", "movement", "density", "space", "aggression"]


def extract_dna(meta: dict) -> dict | None:
    """Return a normalised 6D DNA dict from a .xometa record, or None."""
    for key in ("sonic_dna", "sonicDNA", "dna"):
        if key in meta:
            raw = meta[key]
            if isinstance(raw, dict):
                dna = {}
                for k in DNA_KEYS:
                    if k in raw:
                        dna[k] = float(raw[k])
                if len(dna) == 6:
                    return dna
    return None


def dna_to_vec(dna: dict) -> list[float]:
    return [dna[k] for k in DNA_KEYS]


def vec_distance(a: list[float], b: list[float]) -> float:
    return math.sqrt(sum((x - y) ** 2 for x, y in zip(a, b)))


# ---------------------------------------------------------------------------
# Easing functions
# ---------------------------------------------------------------------------

def apply_easing(t: float, easing: str) -> float:
    """Map linear t in [0,1] to eased t."""
    if easing == "ease-in":
        return t * t
    if easing == "ease-out":
        return 1.0 - (1.0 - t) ** 2
    if easing == "ease-in-out":
        return t * t * (3.0 - 2.0 * t)
    # linear
    return t


def interpolate_dna(start: list[float], end: list[float], t: float) -> list[float]:
    return [s + (e - s) * t for s, e in zip(start, end)]


# ---------------------------------------------------------------------------
# .xometa loading
# ---------------------------------------------------------------------------

def load_xometa(path: str) -> dict:
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)


def scan_presets(presets_dir: str, engine_filter: str | None) -> list[dict]:
    """Recursively scan presets_dir for .xometa files and return list of records."""
    pattern = os.path.join(presets_dir, "**", "*.xometa")
    records = []
    for path in glob.glob(pattern, recursive=True):
        try:
            meta = load_xometa(path)
        except (json.JSONDecodeError, OSError):
            continue

        dna = extract_dna(meta)
        if dna is None:
            continue

        engine = meta.get("engine") or meta.get("engines", [None])[0] or ""
        if isinstance(engine, list):
            engine = engine[0] if engine else ""
        engine = str(engine).upper()

        if engine_filter and engine != engine_filter.upper():
            continue

        records.append({
            "name": meta.get("name") or meta.get("preset_name") or os.path.splitext(os.path.basename(path))[0],
            "engine": engine,
            "mood": meta.get("mood") or meta.get("category") or "",
            "dna": dna,
            "dna_vec": dna_to_vec(dna),
            "path": path,
        })

    return records


# ---------------------------------------------------------------------------
# Sequence builder
# ---------------------------------------------------------------------------

def find_closest(target_vec: list[float], candidates: list[dict],
                 used_paths: set[str]) -> dict | None:
    """Return the closest unused candidate to target_vec."""
    best = None
    best_dist = float("inf")
    for c in candidates:
        if c["path"] in used_paths:
            continue
        d = vec_distance(target_vec, c["dna_vec"])
        if d < best_dist:
            best_dist = d
            best = c
    return best


def build_sequence(start_meta: dict, end_meta: dict, all_presets: list[dict],
                   steps: int, easing: str) -> list[dict]:
    """
    Build the interpolated sequence.

    Returns a list of dicts with keys:
      index, name, engine, mood, dna, dna_vec, distance_from_target, path
    The first and last entries are always the start/end presets themselves.
    """
    start_dna = extract_dna(start_meta)
    end_dna = extract_dna(end_meta)

    if start_dna is None or end_dna is None:
        raise ValueError("One or both endpoint presets are missing valid sonic_dna.")

    start_vec = dna_to_vec(start_dna)
    end_vec = dna_to_vec(end_dna)

    start_engine = start_meta.get("engine") or start_meta.get("engines", [None])[0] or ""
    end_engine = end_meta.get("engine") or end_meta.get("engines", [None])[0] or ""
    if isinstance(start_engine, list):
        start_engine = start_engine[0] if start_engine else ""
    if isinstance(end_engine, list):
        end_engine = end_engine[0] if end_engine else ""

    def endpoint_record(meta: dict, dna: dict, vec: list[float], engine: str) -> dict:
        return {
            "name": meta.get("name") or meta.get("preset_name") or "Unknown",
            "engine": str(engine).upper(),
            "mood": meta.get("mood") or meta.get("category") or "",
            "dna": dna,
            "dna_vec": vec,
            "distance_from_target": 0.0,
            "path": None,
        }

    sequence = []
    used_paths: set[str] = set()

    # Anchor start
    start_rec = endpoint_record(start_meta, start_dna, start_vec, start_engine)
    start_rec["index"] = 0
    sequence.append(start_rec)

    # Inner interpolation points (steps-1 gaps between start and end means
    # steps-1 intermediate points when start and end are both included)
    # The user requests `steps` total points including start+end.
    # So inner points = steps - 2.
    inner_count = max(0, steps - 2)
    for i in range(1, inner_count + 1):
        t_linear = i / (steps - 1)
        t_eased = apply_easing(t_linear, easing)
        target_vec = interpolate_dna(start_vec, end_vec, t_eased)

        match = find_closest(target_vec, all_presets, used_paths)
        if match is None:
            continue

        used_paths.add(match["path"])
        dist = vec_distance(target_vec, match["dna_vec"])
        sequence.append({
            "index": i,
            "name": match["name"],
            "engine": match["engine"],
            "mood": match["mood"],
            "dna": match["dna"],
            "dna_vec": match["dna_vec"],
            "distance_from_target": round(dist, 4),
            "path": match["path"],
        })

    # Anchor end
    end_rec = endpoint_record(end_meta, end_dna, end_vec, end_engine)
    end_rec["index"] = len(sequence)
    sequence.append(end_rec)

    return sequence


# ---------------------------------------------------------------------------
# Formatters
# ---------------------------------------------------------------------------

def format_text(sequence: list[dict], start_name: str, end_name: str) -> str:
    lines = []
    total = len(sequence) - 1  # last index
    lines.append(f'DNA Interpolation: "{start_name}" → "{end_name}" ({total} steps)')
    lines.append("")

    for entry in sequence:
        idx = entry["index"]
        name = entry["name"]
        engine = entry["engine"] or "?"
        dna = entry["dna"]
        dist_str = f"  Δ:{entry['distance_from_target']:.2f}" if entry["distance_from_target"] > 0.0 else ""

        b = dna.get("brightness", 0)
        w = dna.get("warmth", 0)
        m = dna.get("movement", 0)
        d = dna.get("density", 0)
        s = dna.get("space", 0)
        a = dna.get("aggression", 0)

        dna_str = f"B:{b:.2f} W:{w:.2f} M:{m:.2f} D:{d:.2f} S:{s:.2f} A:{a:.2f}"
        engine_col = f"[{engine}]"
        lines.append(f"{idx:2d}. {name:<22} {engine_col:<12} {dna_str}{dist_str}")

    return "\n".join(lines)


def format_json(sequence: list[dict]) -> str:
    output = []
    for entry in sequence:
        output.append({
            "index": entry["index"],
            "name": entry["name"],
            "engine": entry["engine"],
            "mood": entry["mood"],
            "dna": entry["dna"],
            "distance_from_target": entry["distance_from_target"],
        })
    return json.dumps(output, indent=2)


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate a DNA-interpolated preset sequence between two .xometa files.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument("start", help="Path to start .xometa preset")
    parser.add_argument("end", help="Path to end .xometa preset")
    parser.add_argument(
        "--presets-dir",
        default="./Presets/XOlokun/",
        help="Root directory to scan for .xometa files (default: ./Presets/XOlokun/)",
    )
    parser.add_argument(
        "--steps",
        type=int,
        default=8,
        help="Total number of steps in the sequence including start and end (default: 8, range 3-20)",
    )
    parser.add_argument(
        "--easing",
        choices=["linear", "ease-in", "ease-out", "ease-in-out"],
        default="linear",
        help="Easing curve for interpolation (default: linear)",
    )
    parser.add_argument(
        "--engine",
        default=None,
        help="Filter presets to only this engine short name (e.g. OBLONG, ORACLE)",
    )
    parser.add_argument(
        "--format",
        choices=["text", "json"],
        default="text",
        help="Output format (default: text)",
    )
    return parser.parse_args()


def main() -> None:
    args = parse_args()

    # Validate steps range
    if not (3 <= args.steps <= 20):
        print(f"Error: --steps must be between 3 and 20 (got {args.steps})", file=sys.stderr)
        sys.exit(1)

    # Load endpoint presets
    for path in (args.start, args.end):
        if not os.path.isfile(path):
            print(f"Error: file not found: {path}", file=sys.stderr)
            sys.exit(1)

    try:
        start_meta = load_xometa(args.start)
        end_meta = load_xometa(args.end)
    except (json.JSONDecodeError, OSError) as exc:
        print(f"Error reading preset file: {exc}", file=sys.stderr)
        sys.exit(1)

    # Validate DNA present in endpoints
    if extract_dna(start_meta) is None:
        print(f"Error: start preset has no valid sonic_dna: {args.start}", file=sys.stderr)
        sys.exit(1)
    if extract_dna(end_meta) is None:
        print(f"Error: end preset has no valid sonic_dna: {args.end}", file=sys.stderr)
        sys.exit(1)

    # Scan preset library
    if not os.path.isdir(args.presets_dir):
        print(f"Error: presets directory not found: {args.presets_dir}", file=sys.stderr)
        sys.exit(1)

    all_presets = scan_presets(args.presets_dir, args.engine)
    if not all_presets:
        filter_note = f" (engine filter: {args.engine})" if args.engine else ""
        print(f"Error: no .xometa presets with DNA found in {args.presets_dir}{filter_note}", file=sys.stderr)
        sys.exit(1)

    # Exclude the endpoint files themselves from the search pool to avoid
    # them being chosen as intermediate matches (they anchor the sequence).
    start_abs = os.path.abspath(args.start)
    end_abs = os.path.abspath(args.end)
    pool = [p for p in all_presets
            if os.path.abspath(p["path"]) not in (start_abs, end_abs)]

    # Build sequence
    try:
        sequence = build_sequence(
            start_meta, end_meta, pool,
            steps=args.steps,
            easing=args.easing,
        )
    except ValueError as exc:
        print(f"Error: {exc}", file=sys.stderr)
        sys.exit(1)

    # Output
    start_name = start_meta.get("name") or start_meta.get("preset_name") or os.path.splitext(os.path.basename(args.start))[0]
    end_name = end_meta.get("name") or end_meta.get("preset_name") or os.path.splitext(os.path.basename(args.end))[0]

    if args.format == "json":
        print(format_json(sequence))
    else:
        print(format_text(sequence, start_name, end_name))


if __name__ == "__main__":
    main()
