#!/usr/bin/env python3
"""
XPM Preset Variation Generator — XO_OX Designs
Generates N preset variations from a single XPM base preset by sweeping or
randomizing parameter axes.  Pure stdlib — no numpy/scipy required.

Modes
-----
  sweep   Vary one axis from min to max in N evenly-spaced steps.
  grid    Vary 2 params in a sqrt(N) × sqrt(N) grid.
  random  Randomize all axis params within safe bounds; seed from pack name.

Axes (sets of XPM parameters to vary)
--------------------------------------
  felix-oscar   filter cutoff + resonance + env attack + velocity-to-filter
  adsr          attack, decay, sustain, release swept across musical ranges
  filter        cutoff + resonance 2D grid (good with --mode grid)
  space         pan + reverb send (if present)

Output
------
  N XPM files in --output dir.
  index.json with variation metadata.

Usage
-----
  python xpn_preset_variation_generator.py --xpm base.xpm \\
      --variations 20 --mode sweep --axis felix-oscar --output ./variations/

  python xpn_preset_variation_generator.py --xpm base.xpm \\
      --variations 16 --mode grid --axis filter --output ./grid/

  python xpn_preset_variation_generator.py --xpm base.xpm \\
      --variations 30 --mode random --axis adsr --output ./random/
"""

import argparse
import copy
import json
import math
import os
import random
import re
import sys
import xml.etree.ElementTree as ET
from pathlib import Path
from typing import Any, Dict, List, Optional, Tuple

# ---------------------------------------------------------------------------
# Guardrails — absolute parameter limits enforced on every write
# ---------------------------------------------------------------------------

GUARDRAILS: Dict[str, Tuple[float, float]] = {
    # FilterCutoff: 0.0–1.0 normalized
    "FilterCutoff":     (0.05, 1.0),
    # FilterResonance: 0.0–1.0 normalized
    "FilterResonance":  (0.0,  0.85),
    # VolumeAttack / VolumeDecay / VolumeRelease: ms (MPC stores as float ms)
    "VolumeAttack":     (0.0,    8000.0),
    "VolumeDecay":      (0.0,   10000.0),
    "VolumeSustain":    (0.0,       1.0),
    "VolumeRelease":    (0.0,   10000.0),
    # VelocityToFilter: integer 0–127
    "VelocityToFilter": (0.0,     127.0),
    # Pan: 0.0 (full left) – 1.0 (full right); center = 0.5
    "Pan":              (0.0,       1.0),
    # ReverbSend / SendLevel: 0.0–1.0
    "ReverbSend":       (0.0,       1.0),
    "SendLevel":        (0.0,       1.0),
}


def clamp(value: float, param: str) -> float:
    lo, hi = GUARDRAILS.get(param, (0.0, 1.0))
    return max(lo, min(hi, value))


# ---------------------------------------------------------------------------
# Axis definitions
# ---------------------------------------------------------------------------

# Each axis entry: list of (param_name, range_min, range_max)
# These are the *sweep ranges* — the actual values used per variation.

AXES: Dict[str, List[Tuple[str, float, float]]] = {
    "felix-oscar": [
        ("FilterCutoff",     0.20, 0.90),
        ("FilterResonance",  0.10, 0.50),
        ("VolumeAttack",     1.0,  50.0),    # ms
        ("VelocityToFilter", 10.0, 80.0),
    ],
    "adsr": [
        ("VolumeAttack",     1.0,    2000.0),   # ms
        ("VolumeDecay",      10.0,   2000.0),   # ms
        ("VolumeSustain",    0.0,       1.0),
        ("VolumeRelease",    10.0,   5000.0),   # ms
    ],
    "filter": [
        ("FilterCutoff",     0.10, 0.95),
        ("FilterResonance",  0.0,  0.80),
    ],
    "space": [
        ("Pan",          0.25, 0.75),
        ("ReverbSend",   0.0,  0.80),
    ],
}

# ---------------------------------------------------------------------------
# Naming helpers
# ---------------------------------------------------------------------------

SWEEP_SUFFIXES_FELIX_OSCAR = ["Oscar", "feliX"]   # 2-endpoint labels for felix-oscar axis
GRID_COLS = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"


def variation_name(base_name: str, mode: str, axis: str, idx: int, n: int,
                   grid_r: int = 0, grid_c: int = 0) -> str:
    """
    Build human-readable variation name.

    sweep / random:  "{base} 01", "{base} 02", …  (zero-padded)
    felix-oscar:     first = "{base} Oscar", last = "{base} feliX", others numeric
    grid:            "{base} A1", "{base} B2", …
    """
    if mode == "grid":
        return f"{base_name} {GRID_COLS[grid_r]}{grid_c + 1}"

    if mode == "sweep" and axis == "felix-oscar":
        if n == 1:
            return f"{base_name} feliX"
        if idx == 0:
            return f"{base_name} Oscar"
        if idx == n - 1:
            return f"{base_name} feliX"
        # Middle steps: numeric
        width = len(str(n))
        return f"{base_name} {str(idx + 1).zfill(width)}"

    width = max(2, len(str(n)))
    return f"{base_name} {str(idx + 1).zfill(width)}"


def safe_filename(name: str) -> str:
    """Convert a variation name to a safe filename (no spaces → underscores)."""
    safe = re.sub(r'[^\w\s\-.]', '', name)
    safe = re.sub(r'\s+', '_', safe.strip())
    return safe + ".xpm"


# ---------------------------------------------------------------------------
# XPM XML manipulation
# ---------------------------------------------------------------------------

def load_xpm(path: Path) -> Tuple[ET.ElementTree, str]:
    """
    Parse XPM file.  Returns (tree, original_xml_string).
    Preserves the original text for fallback.
    """
    original = path.read_text(encoding="utf-8", errors="replace")
    tree = ET.parse(path)
    return tree, original


def _get_or_create(parent: ET.Element, tag: str) -> ET.Element:
    """Return existing child element or create a new one."""
    el = parent.find(tag)
    if el is None:
        el = ET.SubElement(parent, tag)
    return el


def set_param(root: ET.Element, param: str, value: float) -> None:
    """
    Set a parameter across ALL Instrument elements in the XPM tree.

    XPM structure:
      <MPCVObject>
        <Program>
          <Instruments>
            <Instrument number="0">
              <FilterCutoff>…</FilterCutoff>
              …
            </Instrument>
            …
          </Instruments>
        </Program>
      </MPCVObject>

    Some params (Pan, FilterCutoff, FilterResonance, VolumeAttack, etc.) live
    on <Instrument>.  ReverbSend / SendLevel may live on <Instrument> or inside
    <Send> blocks — we handle both.
    """
    value = clamp(value, param)
    fmt = f"{value:.6f}"

    for instrument in root.iter("Instrument"):
        if param in ("ReverbSend", "SendLevel"):
            # Try direct child first
            el = instrument.find(param)
            if el is not None:
                el.text = fmt
            # Also try <Sends><Send><Level>
            for send in instrument.iter("Send"):
                lvl = send.find("Level")
                if lvl is not None:
                    lvl.text = fmt
        else:
            el = _get_or_create(instrument, param)
            el.text = fmt

    # VelocityToFilter — store as integer string (MPC convention)
    if param == "VelocityToFilter":
        for instrument in root.iter("Instrument"):
            el = _get_or_create(instrument, "VelocityToFilter")
            el.text = str(int(round(value)))


def get_base_name(root: ET.Element, fallback: str) -> str:
    """Extract program name from XPM tree."""
    prog = root.find(".//Program")
    if prog is not None:
        name_el = prog.find("Name")
        if name_el is not None and name_el.text:
            return name_el.text.strip()
    return fallback


def set_program_name(root: ET.Element, name: str) -> None:
    """Set the program name in the XPM tree."""
    prog = root.find(".//Program")
    if prog is not None:
        el = _get_or_create(prog, "Name")
        el.text = name[:32]  # MPC truncates at 32 chars


def write_xpm(tree: ET.ElementTree, path: Path) -> None:
    """Write XPM tree to file with XML declaration."""
    path.parent.mkdir(parents=True, exist_ok=True)
    tree.write(str(path), encoding="unicode", xml_declaration=True)


# ---------------------------------------------------------------------------
# Interpolation utilities
# ---------------------------------------------------------------------------

def lerp(a: float, b: float, t: float) -> float:
    """Linear interpolation."""
    return a + (b - a) * t


def sweep_values(lo: float, hi: float, n: int) -> List[float]:
    """
    Return N evenly-spaced values from lo to hi inclusive.
    If n == 1, returns [hi] (feliX end).
    """
    if n == 1:
        return [hi]
    return [lerp(lo, hi, i / (n - 1)) for i in range(n)]


# ---------------------------------------------------------------------------
# Mode: sweep
# ---------------------------------------------------------------------------

def generate_sweep(
    base_tree: ET.ElementTree,
    base_name: str,
    axis: str,
    n: int,
    output_dir: Path,
) -> List[Dict[str, Any]]:
    """
    Vary the first param of the axis (or all params proportionally) from min
    to max in N steps.  For multi-param axes, all params move together at the
    same normalized t.
    """
    params = AXES[axis]
    records = []

    ts = [i / (n - 1) if n > 1 else 1.0 for i in range(n)]

    for idx, t in enumerate(ts):
        tree_copy = copy.deepcopy(base_tree)
        root = tree_copy.getroot()

        param_values: Dict[str, float] = {}
        for param, lo, hi in params:
            v = lerp(lo, hi, t)
            set_param(root, param, v)
            param_values[param] = round(clamp(v, param), 6)

        vname = variation_name(base_name, "sweep", axis, idx, n)
        set_program_name(root, vname)

        fname = safe_filename(vname)
        write_xpm(tree_copy, output_dir / fname)

        records.append({
            "index":      idx,
            "name":       vname,
            "file":       fname,
            "t":          round(t, 4),
            "params":     param_values,
        })
        print(f"  [{idx + 1:>3}/{n}]  {vname}  →  {fname}")

    return records


# ---------------------------------------------------------------------------
# Mode: grid
# ---------------------------------------------------------------------------

def generate_grid(
    base_tree: ET.ElementTree,
    base_name: str,
    axis: str,
    n: int,
    output_dir: Path,
) -> List[Dict[str, Any]]:
    """
    Vary 2 params (first two in axis) in a sqrt(N) × sqrt(N) grid.
    If axis has only 1 param, grid is 1D (same as sweep).
    N is rounded up to nearest perfect square.
    """
    params = AXES[axis]
    side = max(1, math.ceil(math.sqrt(n)))
    actual_n = side * side

    if actual_n != n:
        print(
            f"  NOTE: grid mode rounds N={n} up to {actual_n} ({side}×{side}).",
            file=sys.stderr,
        )

    p1_name, p1_lo, p1_hi = params[0]
    p2_name, p2_lo, p2_hi = params[1] if len(params) > 1 else params[0]

    row_vals = sweep_values(p1_lo, p1_hi, side)
    col_vals = sweep_values(p2_lo, p2_hi, side)

    records = []

    for r, rv in enumerate(row_vals):
        for c, cv in enumerate(col_vals):
            tree_copy = copy.deepcopy(base_tree)
            root = tree_copy.getroot()

            set_param(root, p1_name, rv)
            if p2_name != p1_name:
                set_param(root, p2_name, cv)

            # Remaining params in axis stay at midpoint
            for pname, plo, phi in params[2:]:
                set_param(root, pname, lerp(plo, phi, 0.5))

            vname = variation_name(base_name, "grid", axis, 0, actual_n,
                                   grid_r=r, grid_c=c)
            set_program_name(root, vname)

            fname = safe_filename(vname)
            write_xpm(tree_copy, output_dir / fname)

            param_values = {
                p1_name: round(clamp(rv, p1_name), 6),
                p2_name: round(clamp(cv, p2_name), 6),
            }

            records.append({
                "index":  r * side + c,
                "name":   vname,
                "file":   fname,
                "row":    r,
                "col":    c,
                "params": param_values,
            })
            print(f"  [{r},{c}]  {vname}  →  {fname}")

    return records


# ---------------------------------------------------------------------------
# Mode: random
# ---------------------------------------------------------------------------

def generate_random(
    base_tree: ET.ElementTree,
    base_name: str,
    axis: str,
    n: int,
    output_dir: Path,
    seed: str,
) -> List[Dict[str, Any]]:
    """
    Randomize all axis params within their safe bounds.
    Seed is derived from pack name for reproducibility.
    """
    rng = random.Random(seed)
    params = AXES[axis]
    records = []

    for idx in range(n):
        tree_copy = copy.deepcopy(base_tree)
        root = tree_copy.getroot()

        param_values: Dict[str, float] = {}
        for param, lo, hi in params:
            v = rng.uniform(lo, hi)
            set_param(root, param, v)
            param_values[param] = round(clamp(v, param), 6)

        vname = variation_name(base_name, "random", axis, idx, n)
        set_program_name(root, vname)

        fname = safe_filename(vname)
        write_xpm(tree_copy, output_dir / fname)

        records.append({
            "index":  idx,
            "name":   vname,
            "file":   fname,
            "seed":   seed,
            "params": param_values,
        })
        print(f"  [{idx + 1:>3}/{n}]  {vname}  →  {fname}")

    return records


# ---------------------------------------------------------------------------
# index.json writer
# ---------------------------------------------------------------------------

def write_index(
    output_dir: Path,
    base_xpm: str,
    base_name: str,
    mode: str,
    axis: str,
    n: int,
    records: List[Dict[str, Any]],
) -> Path:
    """Write index.json summarizing all generated variations."""
    payload = {
        "generator":   "xpn_preset_variation_generator.py",
        "base_xpm":    base_xpm,
        "base_name":   base_name,
        "mode":        mode,
        "axis":        axis,
        "requested_n": n,
        "actual_n":    len(records),
        "axis_params": [
            {"param": p, "range_min": lo, "range_max": hi}
            for p, lo, hi in AXES[axis]
        ],
        "guardrails": {
            p: {"min": lo, "max": hi}
            for p, (lo, hi) in GUARDRAILS.items()
        },
        "variations": records,
    }
    idx_path = output_dir / "index.json"
    idx_path.write_text(json.dumps(payload, indent=2), encoding="utf-8")
    return idx_path


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main(argv: Optional[List[str]] = None) -> int:
    parser = argparse.ArgumentParser(
        description="XPM Preset Variation Generator — XO_OX Designs",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument(
        "--xpm", required=True, metavar="base.xpm",
        help="Base XPM preset file to vary.",
    )
    parser.add_argument(
        "--variations", type=int, default=10, metavar="N",
        help="Number of variations to generate (default: 10).",
    )
    parser.add_argument(
        "--mode", choices=["sweep", "grid", "random"], default="sweep",
        help="Variation mode: sweep / grid / random (default: sweep).",
    )
    parser.add_argument(
        "--axis", choices=list(AXES.keys()), default="felix-oscar",
        help=(
            "Parameter axis to vary. "
            "felix-oscar: cutoff+res+attack+vel2filter | "
            "adsr: full envelope | "
            "filter: cutoff+resonance | "
            "space: pan+reverb (default: felix-oscar)."
        ),
    )
    parser.add_argument(
        "--output", default="./variations/", metavar="DIR",
        help="Output directory (default: ./variations/).",
    )
    parser.add_argument(
        "--seed", default=None, metavar="SEED",
        help=(
            "Random seed for --mode random. "
            "Defaults to base XPM stem (reproducible per pack)."
        ),
    )

    args = parser.parse_args(argv)

    xpm_path = Path(args.xpm)
    if not xpm_path.exists():
        print(f"ERROR: {xpm_path} not found.", file=sys.stderr)
        return 1

    output_dir = Path(args.output)
    output_dir.mkdir(parents=True, exist_ok=True)

    try:
        base_tree, _ = load_xpm(xpm_path)
    except ET.ParseError as e:
        print(f"ERROR: Failed to parse XPM XML: {e}", file=sys.stderr)
        return 1

    root = base_tree.getroot()
    base_name = get_base_name(root, xpm_path.stem)
    seed = args.seed or xpm_path.stem

    n = args.variations
    mode = args.mode
    axis = args.axis

    print(f"\nXPM Preset Variation Generator")
    print(f"  Base preset : {xpm_path.name}  ({base_name!r})")
    print(f"  Mode        : {mode}")
    print(f"  Axis        : {axis}")
    print(f"  Variations  : {n}")
    print(f"  Output dir  : {output_dir}\n")

    try:
        if mode == "sweep":
            records = generate_sweep(base_tree, base_name, axis, n, output_dir)
        elif mode == "grid":
            records = generate_grid(base_tree, base_name, axis, n, output_dir)
        elif mode == "random":
            records = generate_random(base_tree, base_name, axis, n, output_dir, seed)
        else:
            print(f"ERROR: Unknown mode: {mode}", file=sys.stderr)
            return 1
    except Exception as e:
        print(f"ERROR: {e}", file=sys.stderr)
        return 1

    idx_path = write_index(output_dir, str(xpm_path), base_name,
                           mode, axis, n, records)
    print(f"\n  index.json  →  {idx_path}")
    print(f"\nDone. {len(records)} variation(s) written to {output_dir}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
