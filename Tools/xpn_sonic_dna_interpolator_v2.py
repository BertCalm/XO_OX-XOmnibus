#!/usr/bin/env python3
"""
XPN Sonic DNA Interpolator v2 — XO_OX Designs
===============================================
Interpolates between two preset DNA profiles, generating NEW .xometa files at
evenly spaced points along the "sonic path" between two sound characters. Unlike
v1 (which finds nearest real presets), v2 synthesises brand-new presets by
blending both DNA vectors AND parameter dictionaries.

Interpolation modes
-------------------
  linear    — straight-line blend (uniform t)
  ease-in   — slow start, fast finish (t²)
  ease-out  — fast start, slow finish (√t)
  scurve    — smooth-step: slow–fast–slow (3t²−2t³)
  bounce    — oscillate slightly around the midpoint

For each interpolated preset
-----------------------------
  DNA        — interpolated 6D values
  Parameters — blended values for keys present in BOTH A and B (same engine)
  Name       — "<prefix> 01", "<prefix> 02", …
  Mood       — A's mood if step position < 0.5, else B's mood
  Engine     — A's engine (A and B must share an engine when --same-engine-only)

CLI
---
  python xpn_sonic_dna_interpolator_v2.py \\
      --source A.xometa --target B.xometa --steps 8 \\
      [--mode linear|ease-in|ease-out|scurve|bounce] \\
      [--output-dir interpolated/] \\
      [--prefix "Journey"] \\
      [--same-engine-only] \\
      [--dry-run]
"""

from __future__ import annotations

import argparse
import json
import math
import sys
from pathlib import Path

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

DNA_KEYS = ["brightness", "warmth", "movement", "density", "space", "aggression"]
DNA_ABBREV = {"brightness": "B", "warmth": "W", "movement": "M",
              "density": "D", "space": "S", "aggression": "A"}

CHART_WIDTH = 40   # ASCII chart bar max width
CHART_HEIGHT = 12  # rows in the ASCII chart

# ---------------------------------------------------------------------------
# .xometa helpers
# ---------------------------------------------------------------------------

def load_xometa(path: Path) -> dict:
    with path.open("r", encoding="utf-8") as fh:
        return json.load(fh)


def extract_dna(meta: dict) -> dict | None:
    """Return normalised 6D DNA dict, or None."""
    for key in ("sonic_dna", "sonicDNA", "dna"):
        if key in meta:
            raw = meta[key]
            if isinstance(raw, dict):
                dna = {k: float(raw[k]) for k in DNA_KEYS if k in raw}
                if len(dna) == 6:
                    return dna
    return None


def get_engine(meta: dict) -> str:
    engine = meta.get("engine") or meta.get("engines", [None])[0] or ""
    if isinstance(engine, list):
        engine = engine[0] if engine else ""
    return str(engine).upper()


def get_mood(meta: dict) -> str:
    return meta.get("mood") or meta.get("category") or ""


def get_name(meta: dict, path: Path) -> str:
    return meta.get("name") or meta.get("preset_name") or path.stem


def get_parameters(meta: dict) -> dict:
    return meta.get("parameters") or {}


def get_macros(meta: dict) -> dict:
    return meta.get("macros") or {}

# ---------------------------------------------------------------------------
# Easing / curve functions
# ---------------------------------------------------------------------------

def apply_mode(t: float, mode: str) -> float:
    """Map linear t ∈ [0,1] through the selected curve."""
    if mode == "ease-in":
        return t * t
    if mode == "ease-out":
        return math.sqrt(t)
    if mode == "scurve":
        return t * t * (3.0 - 2.0 * t)
    if mode == "bounce":
        # Sinusoidal wobble: oscillates once around the midpoint
        base = t * t * (3.0 - 2.0 * t)          # underlying smoothstep
        wobble = math.sin(t * math.pi) * 0.08    # ±0.08 bump near mid
        return max(0.0, min(1.0, base + wobble))
    # linear
    return t

# ---------------------------------------------------------------------------
# Interpolation
# ---------------------------------------------------------------------------

def lerp(a: float, b: float, t: float) -> float:
    return a + (b - a) * t


def interp_dna(dna_a: dict, dna_b: dict, t: float) -> dict:
    return {k: round(lerp(dna_a[k], dna_b[k], t), 4) for k in DNA_KEYS}


def interp_parameters(params_a: dict, params_b: dict, t: float) -> dict:
    """Blend parameter values present in BOTH dicts. Skip non-matching keys."""
    result = {}
    for key in params_a:
        if key not in params_b:
            continue
        va, vb = params_a[key], params_b[key]
        if isinstance(va, (int, float)) and isinstance(vb, (int, float)):
            result[key] = round(lerp(float(va), float(vb), t), 6)
        else:
            # Non-numeric: take A's value below midpoint, B's at/above
            result[key] = va if t < 0.5 else vb
    return result


def interp_macros(macros_a: dict, macros_b: dict, t: float) -> dict:
    result = {}
    for key in macros_a:
        if key not in macros_b:
            result[key] = macros_a[key]
            continue
        va, vb = macros_a[key], macros_b[key]
        if isinstance(va, (int, float)) and isinstance(vb, (int, float)):
            result[key] = round(lerp(float(va), float(vb), t), 4)
        else:
            result[key] = va if t < 0.5 else vb
    return result

# ---------------------------------------------------------------------------
# Preset generation
# ---------------------------------------------------------------------------

def build_preset(
    meta_a: dict,
    meta_b: dict,
    dna_a: dict,
    dna_b: dict,
    t_linear: float,
    t_eased: float,
    name: str,
) -> dict:
    mood = get_mood(meta_a) if t_linear < 0.5 else get_mood(meta_b)
    engine = get_engine(meta_a)

    dna = interp_dna(dna_a, dna_b, t_eased)
    params = interp_parameters(get_parameters(meta_a), get_parameters(meta_b), t_eased)
    macros = interp_macros(get_macros(meta_a), get_macros(meta_b), t_eased)

    preset = {
        "name": name,
        "engine": engine,
        "mood": mood,
        "sonic_dna": dna,
        "parameters": params,
    }
    if macros:
        preset["macros"] = macros

    # Carry over any additional top-level metadata from A
    skip = {"name", "engine", "mood", "sonic_dna", "sonicDNA", "dna",
            "parameters", "macros", "preset_name", "category", "engines"}
    for k, v in meta_a.items():
        if k not in skip:
            preset.setdefault(k, v)

    return preset


def generate_interpolated_presets(
    meta_a: dict,
    meta_b: dict,
    steps: int,
    mode: str,
    prefix: str,
) -> list[tuple[float, float, dict]]:
    """
    Returns list of (t_linear, t_eased, preset_dict) for each step.
    steps = number of intermediate presets (NOT including endpoints).
    """
    dna_a = extract_dna(meta_a)
    dna_b = extract_dna(meta_b)

    results = []
    for i in range(steps):
        # Map i → t in (0, 1) exclusive of endpoints
        t_linear = (i + 1) / (steps + 1)
        t_eased = apply_mode(t_linear, mode)
        name = f"{prefix} {i + 1:02d}"
        preset = build_preset(meta_a, meta_b, dna_a, dna_b,
                              t_linear, t_eased, name)
        results.append((t_linear, t_eased, preset))

    return results

# ---------------------------------------------------------------------------
# ASCII visualizer
# ---------------------------------------------------------------------------

def ascii_chart(
    meta_a: dict,
    meta_b: dict,
    interpolated: list[tuple[float, float, dict]],
) -> str:
    """
    Print a grid: rows = DNA dimensions, columns = A + steps + B.
    Each cell shows a bar proportional to the value (0–1 assumed range).
    """
    dna_a = extract_dna(meta_a)
    dna_b = extract_dna(meta_b)
    name_a = get_name(meta_a, Path("A"))
    name_b = get_name(meta_b, Path("B"))

    # Collect all DNA vectors in order: A, intermediates, B
    all_dna = [dna_a] + [p["sonic_dna"] for _, _, p in interpolated] + [dna_b]
    labels = ([f"A:{name_a[:10]}"]
              + [p["name"] for _, _, p in interpolated]
              + [f"B:{name_b[:10]}"])
    col_w = 12

    lines = []
    lines.append("")
    lines.append("  DNA PATH VISUALIZATION")
    lines.append("  " + "─" * (len(all_dna) * col_w + 8))

    for dim in DNA_KEYS:
        abbrev = DNA_ABBREV[dim]
        row_vals = [d.get(dim, 0.0) for d in all_dna]
        bar_cells = []
        for v in row_vals:
            filled = int(round(v * (col_w - 2)))
            filled = max(0, min(col_w - 2, filled))
            bar = "█" * filled + "░" * (col_w - 2 - filled)
            bar_cells.append(f"[{bar}]")
        lines.append(f"  {abbrev:<12}" + "".join(bar_cells))

    lines.append("  " + "─" * (len(all_dna) * col_w + 8))

    # Column headers (truncated)
    header_parts = []
    for lbl in labels:
        truncated = lbl[:col_w].center(col_w)
        header_parts.append(truncated)
    lines.append("  " + " " * 12 + "".join(header_parts))

    # Numeric summary row
    lines.append("")
    lines.append("  Step values (t_linear → t_eased):")
    for t_lin, t_eas, p in interpolated:
        lines.append(f"    {p['name']:<14}  t={t_lin:.3f}  →  t_eased={t_eas:.3f}  "
                     f"  DNA: " + "  ".join(
                         f"{DNA_ABBREV[k]}={p['sonic_dna'][k]:.2f}" for k in DNA_KEYS))

    return "\n".join(lines)

# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate interpolated .xometa presets between two sonic DNA poles.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument("--source", required=True, metavar="A.xometa",
                        help="Source (start) preset file")
    parser.add_argument("--target", required=True, metavar="B.xometa",
                        help="Target (end) preset file")
    parser.add_argument("--steps", type=int, default=8,
                        help="Number of intermediate presets to generate (default: 8)")
    parser.add_argument("--mode",
                        choices=["linear", "ease-in", "ease-out", "scurve", "bounce"],
                        default="linear",
                        help="Interpolation curve (default: linear)")
    parser.add_argument("--output-dir", default="interpolated",
                        metavar="DIR",
                        help="Output directory for generated .xometa files (default: interpolated/)")
    parser.add_argument("--prefix", default="Journey",
                        help="Prefix for generated preset names (default: Journey)")
    parser.add_argument("--same-engine-only", action="store_true",
                        help="Abort if A and B do not share the same engine")
    parser.add_argument("--dry-run", action="store_true",
                        help="Print what would be created without writing files")
    return parser.parse_args()


def main() -> None:
    args = parse_args()

    # Validate inputs
    path_a = Path(args.source)
    path_b = Path(args.target)

    for p in (path_a, path_b):
        if not p.is_file():
            print(f"Error: file not found: {p}", file=sys.stderr)
            sys.exit(1)

    try:
        meta_a = load_xometa(path_a)
        meta_b = load_xometa(path_b)
    except (json.JSONDecodeError, OSError) as exc:
        print(f"Error reading preset: {exc}", file=sys.stderr)
        sys.exit(1)

    # Validate DNA
    if extract_dna(meta_a) is None:
        print(f"Error: source preset has no valid sonic_dna: {path_a}", file=sys.stderr)
        sys.exit(1)
    if extract_dna(meta_b) is None:
        print(f"Error: target preset has no valid sonic_dna: {path_b}", file=sys.stderr)
        sys.exit(1)

    # Engine check
    engine_a = get_engine(meta_a)
    engine_b = get_engine(meta_b)
    if args.same_engine_only and engine_a != engine_b:
        print(f"Error: --same-engine-only: source engine '{engine_a}' != "
              f"target engine '{engine_b}'", file=sys.stderr)
        sys.exit(1)
    if engine_a != engine_b:
        print(f"Warning: engines differ ({engine_a} vs {engine_b}). "
              "Parameter interpolation will only blend shared parameter keys.",
              file=sys.stderr)

    # Validate steps
    if not (1 <= args.steps <= 50):
        print(f"Error: --steps must be between 1 and 50 (got {args.steps})", file=sys.stderr)
        sys.exit(1)

    # Generate
    name_a = get_name(meta_a, path_a)
    name_b = get_name(meta_b, path_b)

    print(f'\nSonic path: "{name_a}" [{engine_a}] → "{name_b}" [{engine_b}]')
    print(f"Mode: {args.mode}  |  Steps: {args.steps}  |  Prefix: {args.prefix}")

    interpolated = generate_interpolated_presets(
        meta_a, meta_b,
        steps=args.steps,
        mode=args.mode,
        prefix=args.prefix,
    )

    # ASCII chart
    print(ascii_chart(meta_a, meta_b, interpolated))

    # Output
    output_dir = Path(args.output_dir)

    if args.dry_run:
        print(f"\n[dry-run] Would write {len(interpolated)} files to: {output_dir}/")
        for _, _, preset in interpolated:
            filename = preset["name"].replace(" ", "_") + ".xometa"
            print(f"  {output_dir / filename}")
        print("\n[dry-run] No files written.")
        return

    output_dir.mkdir(parents=True, exist_ok=True)
    written = []
    for _, _, preset in interpolated:
        filename = preset["name"].replace(" ", "_") + ".xometa"
        dest = output_dir / filename
        with dest.open("w", encoding="utf-8") as fh:
            json.dump(preset, fh, indent=2, ensure_ascii=False)
        written.append(dest)

    print(f"\nWrote {len(written)} presets to: {output_dir.resolve()}/")
    for p in written:
        print(f"  {p.name}")


if __name__ == "__main__":
    main()
