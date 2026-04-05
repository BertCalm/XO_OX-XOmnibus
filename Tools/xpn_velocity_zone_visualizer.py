#!/usr/bin/env python3
"""
xpn_velocity_zone_visualizer.py — XO_OX Designs
=================================================
Parse an Akai MPC XPM program file and render each keygroup's velocity zone
layout as an ASCII map, making gaps, overlaps, and non-standard splits easy
to spot at a glance.

ASCII velocity map (128 columns, one column per velocity unit, 1-indexed display):
  1 2 3 4  — layer index covering that velocity unit
  X        — two or more layers overlap at that velocity unit
  .        — no layer covers that velocity unit (gap)

Ghost Council Modified velocity splits (4-layer curve, adopted 2026-04-04):
  Layer 1: vel  1 –  20  (Ghost)
  Layer 2: vel 21 –  55  (Light)
  Layer 3: vel 56 –  90  (Medium)
  Layer 4: vel 91 – 127  (Hard)

Deviation from any boundary by more than SPLIT_TOLERANCE (default 2) is
reported as a non-standard split.

Usage:
    python xpn_velocity_zone_visualizer.py <program.xpm>
    python xpn_velocity_zone_visualizer.py <program.xpm> --keygroup 3
    python xpn_velocity_zone_visualizer.py <program.xpm> --strict

Options:
    --keygroup <n>   Show only keygroup index n (1-based instrument number).
    --strict         Exit with code 1 if any gap, overlap, or non-standard split
                     is detected.
    --tolerance <n>  Split boundary tolerance in velocity units (default 2).
"""

import argparse
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Optional
from xml.etree import ElementTree as ET


# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

CANONICAL_SPLITS_4L = [
    (1,  20),    # Ghost  — ghost notes, barely touching
    (21, 55),    # Light  — finger drumming sweet spot
    (56, 90),    # Medium — deliberate hits
    (91, 127),   # Hard   — power hits, peak force
]

DEFAULT_SPLIT_TOLERANCE = 2

# Width of the ruler/map: 128 velocity units, printed directly (1 col = 1 vel unit).
# We use a compact display of 64 chars where each char covers 2 vel units to keep
# output under 80 columns, but the map below uses one char per unit at ruler width.
# Compromise: use 64-char display (2 units/char) for the bar, but report exact
# velocity boundaries in gap/overlap annotations.

MAP_WIDTH = 64          # display columns (each covers 2 velocity units)
VEL_PER_COL = 2        # velocity units per display column


# ---------------------------------------------------------------------------
# Data model
# ---------------------------------------------------------------------------

@dataclass
class Layer:
    index: int            # 1-based layer index within keygroup
    vel_start: int
    vel_end: int
    sample_file: str


@dataclass
class Keygroup:
    number: int           # instrument number from XPM (1-based)
    root_note: int
    low_note: int
    high_note: int
    layers: list[Layer] = field(default_factory=list)


# ---------------------------------------------------------------------------
# XPM parser
# ---------------------------------------------------------------------------

def _int(text: Optional[str], default: int = 0) -> int:
    if text is None:
        return default
    try:
        return int(text.strip())
    except ValueError:
        return default


def parse_xpm(path: Path) -> list[Keygroup]:
    """Parse XPM and return list of Keygroup objects (may be empty)."""
    tree = ET.parse(path)
    root = tree.getroot()

    keygroups: list[Keygroup] = []
    for inst_el in root.findall(".//Instruments/Instrument"):
        inst_num  = _int(inst_el.get("number"), default=0)
        root_note = _int(inst_el.findtext("RootNote"), default=60)
        low_note  = _int(inst_el.findtext("LowNote"),  default=0)
        high_note = _int(inst_el.findtext("HighNote"), default=127)

        layers: list[Layer] = []
        for layer_idx, layer_el in enumerate(inst_el.findall(".//Layer"), start=1):
            vs = _int(layer_el.findtext("VelStart"), default=0)
            ve = _int(layer_el.findtext("VelEnd"),   default=0)
            sf = (layer_el.findtext("SampleFile") or "").strip()
            # Skip empty placeholder layers
            if vs == 0 and ve == 0:
                continue
            layers.append(Layer(
                index=layer_idx,
                vel_start=vs,
                vel_end=ve,
                sample_file=sf,
            ))

        if not layers:
            continue

        # Re-index layers by their position after filtering empties
        for new_idx, lyr in enumerate(layers, start=1):
            lyr.index = new_idx

        keygroups.append(Keygroup(
            number=inst_num,
            root_note=root_note,
            low_note=low_note,
            high_note=high_note,
            layers=layers,
        ))

    return keygroups


# ---------------------------------------------------------------------------
# Analysis
# ---------------------------------------------------------------------------

def build_coverage(layers: list[Layer]) -> list[int]:
    """
    Return a 128-int array where each entry is the count of layers covering
    that velocity unit (0-indexed: index 0 = velocity 1, index 127 = velocity 128).
    We use 1-based velocity (1–127 as MPC convention) mapped to 0-based array.
    """
    coverage = [0] * 128
    for lyr in layers:
        lo = max(0, lyr.vel_start - 1)
        hi = min(127, lyr.vel_end - 1)
        for v in range(lo, hi + 1):
            coverage[v] += 1
    return coverage


def build_layer_map(layers: list[Layer]) -> list[Optional[int]]:
    """
    Return a 128-entry list mapping each velocity unit (0-indexed) to the
    1-based layer index covering it, or None if uncovered.
    Only the first matching layer is stored (overlap is detected via coverage).
    """
    layer_at: list[Optional[int]] = [None] * 128
    for lyr in layers:
        lo = max(0, lyr.vel_start - 1)
        hi = min(127, lyr.vel_end - 1)
        for v in range(lo, hi + 1):
            if layer_at[v] is None:
                layer_at[v] = lyr.index
    return layer_at


def find_gaps(coverage: list[int]) -> list[tuple[int, int]]:
    """Return list of (vel_start, vel_end) 1-based gap spans."""
    gaps: list[tuple[int, int]] = []
    in_gap = False
    gap_start = 0
    for i, c in enumerate(coverage):
        vel = i + 1  # 1-based
        if c == 0 and not in_gap:
            in_gap = True
            gap_start = vel
        elif c > 0 and in_gap:
            gaps.append((gap_start, vel - 1))
            in_gap = False
    if in_gap:
        gaps.append((gap_start, 128))
    return gaps


def find_overlaps(coverage: list[int]) -> list[tuple[int, int]]:
    """Return list of (vel_start, vel_end) 1-based overlap spans."""
    overlaps: list[tuple[int, int]] = []
    in_ovl = False
    ovl_start = 0
    for i, c in enumerate(coverage):
        vel = i + 1
        if c >= 2 and not in_ovl:
            in_ovl = True
            ovl_start = vel
        elif c < 2 and in_ovl:
            overlaps.append((ovl_start, vel - 1))
            in_ovl = False
    if in_ovl:
        overlaps.append((ovl_start, 128))
    return overlaps


def check_canonical_split(layers: list[Layer], tolerance: int) -> list[str]:
    """
    Compare the layer velocity zones against Vibe's canonical 4-layer split.
    Returns list of deviation descriptions (empty = clean canonical split).
    Only meaningful when len(layers) == 4.
    """
    if len(layers) != 4:
        return []  # canonical split is defined for exactly 4 layers

    deviations: list[str] = []
    for lyr, (canon_start, canon_end) in zip(sorted(layers, key=lambda l: l.vel_start),
                                              CANONICAL_SPLITS_4L):
        ds = abs(lyr.vel_start - canon_start)
        de = abs(lyr.vel_end   - canon_end)
        if ds > tolerance:
            deviations.append(
                f"  Layer {lyr.index} start {lyr.vel_start} "
                f"(canonical {canon_start}, delta {ds})"
            )
        if de > tolerance:
            deviations.append(
                f"  Layer {lyr.index} end   {lyr.vel_end} "
                f"(canonical {canon_end}, delta {de})"
            )
    return deviations


# ---------------------------------------------------------------------------
# ASCII rendering
# ---------------------------------------------------------------------------

def note_name(midi: int) -> str:
    names = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]
    octave = (midi // 12) - 2
    return f"{names[midi % 12]}{octave}"


def render_map(layers: list[Layer]) -> str:
    """
    Render a MAP_WIDTH-character ASCII string representing the full 128-velocity
    range.  Each character covers VEL_PER_COL velocity units.

    Character meanings:
      1–4   : layer index (only one layer covers this column)
      X     : overlap (two or more layers claim at least one unit in this column)
      .     : gap (no layer covers any unit in this column)
      +     : coverage but by layer index 5+ (rare; show '+')
    """
    coverage  = build_coverage(layers)
    layer_map = build_layer_map(layers)

    chars: list[str] = []
    for col in range(MAP_WIDTH):
        v0 = col * VEL_PER_COL          # 0-based index start
        v1 = min(v0 + VEL_PER_COL, 128) # exclusive end
        col_coverage = [coverage[v]  for v in range(v0, v1)]
        col_layers   = [layer_map[v] for v in range(v0, v1)]

        max_cov = max(col_coverage)
        if max_cov == 0:
            chars.append(".")
        elif max_cov >= 2:
            chars.append("X")
        else:
            # Exactly one layer covers something in this column
            layer_indices = {l for l in col_layers if l is not None}
            if len(layer_indices) == 1:
                idx = next(iter(layer_indices))
                if idx <= 4:
                    chars.append(str(idx))
                else:
                    chars.append("+")
            elif len(layer_indices) > 1:
                # Different single layers in the two units — treat as boundary, use first
                idx = min(layer_indices)
                chars.append(str(idx) if idx <= 4 else "+")
            else:
                chars.append(".")

    return "".join(chars)


def render_ruler() -> str:
    """
    Ruler line for 64-column display with velocity labels at key positions.
    Each column = 2 velocity units; velocity values shown are 1-based.
    Positions: 1, 17, 33, 49, 64 (every 16 cols = every 32 vel units).
    """
    labels = {0: "1", 16: "33", 32: "65", 48: "97", 63: "127"}
    ruler = [" "] * MAP_WIDTH
    for col, label in labels.items():
        for i, ch in enumerate(label):
            if col + i < MAP_WIDTH:
                ruler[col + i] = ch
    return "".join(ruler)


def render_tick() -> str:
    ticks = ["-"] * MAP_WIDTH
    for pos in [0, 16, 32, 48, 63]:
        ticks[pos] = "|"
    return "".join(ticks)


def sample_short(path: str, width: int = 16) -> str:
    stem = Path(path).stem if path else "(empty)"
    if len(stem) <= width:
        return stem
    return stem[: width - 1] + "~"


# ---------------------------------------------------------------------------
# Per-keygroup display
# ---------------------------------------------------------------------------

def display_keygroup(
    kg: Keygroup,
    tolerance: int,
    show_header: bool = True,
) -> dict:
    """
    Print the full analysis block for one keygroup.
    Returns a stats dict with keys: has_gap, has_overlap, has_nonstandard_split.
    """
    coverage    = build_coverage(kg.layers)
    gaps        = find_gaps(coverage)
    overlaps    = find_overlaps(coverage)
    split_devs  = check_canonical_split(kg.layers, tolerance)

    has_gap        = bool(gaps)
    has_overlap    = bool(overlaps)
    has_nonstandard = bool(split_devs)

    # Title line
    note_range = f"{note_name(kg.low_note)}-{note_name(kg.high_note)}"
    root_str   = note_name(kg.root_note)
    print(f"Keygroup #{kg.number:>3}  root={root_str:<4}  range={note_range:<10}  "
          f"layers={len(kg.layers)}")

    # Velocity map
    if show_header:
        print(f"  vel: {render_ruler()}")
        print(f"       {render_tick()}")

    vel_map = render_map(kg.layers)
    print(f"  map: {vel_map}")

    # Layer legend
    for lyr in sorted(kg.layers, key=lambda l: l.vel_start):
        sample_label = sample_short(lyr.sample_file, 18)
        bar_start = lyr.vel_start // VEL_PER_COL
        bar_end   = lyr.vel_end   // VEL_PER_COL
        span_str  = f"{lyr.vel_start:>3}-{lyr.vel_end:<3}"
        print(f"    [{lyr.index}] vel {span_str}  cols {bar_start:>2}-{bar_end:<2}  {sample_label}")

    # Issues
    if gaps:
        gap_str = ", ".join(f"{g[0]}-{g[1]}" for g in gaps)
        print(f"  GAPS     : vel {gap_str}")
    if overlaps:
        ovl_str = ", ".join(f"{o[0]}-{o[1]}" for o in overlaps)
        print(f"  OVERLAPS : vel {ovl_str}")
    if split_devs:
        print(f"  NON-CANONICAL SPLIT (4-layer, tolerance={tolerance}):")
        for dev in split_devs:
            print(f"   {dev}")

    if not gaps and not overlaps and not split_devs:
        print(f"  OK")

    print()

    return {
        "has_gap": has_gap,
        "has_overlap": has_overlap,
        "has_nonstandard_split": has_nonstandard,
    }


# ---------------------------------------------------------------------------
# Summary stats
# ---------------------------------------------------------------------------

def print_summary(
    total: int,
    clean: int,
    with_gaps: int,
    with_overlaps: int,
    with_nonstandard: int,
) -> None:
    pct = lambda n: f"{100 * n // total:>3}%" if total else "  0%"
    print("=" * 60)
    print("SUMMARY")
    print(f"  Total keygroups        : {total}")
    print(f"  Clean splits           : {clean} ({pct(clean)})")
    print(f"  With gaps              : {with_gaps} ({pct(with_gaps)})")
    print(f"  With overlaps          : {with_overlaps} ({pct(with_overlaps)})")
    print(f"  Non-canonical splits   : {with_nonstandard} ({pct(with_nonstandard)})")
    print()
    print("  Map key:")
    print("    1 2 3 4  = layer index (exclusive coverage)")
    print("    X        = overlap (2+ layers)")
    print("    .        = gap (no layer)")
    print("    +        = layer index 5+")
    print()
    print("  Canonical 4-layer split (Vibe):")
    for i, (vs, ve) in enumerate(CANONICAL_SPLITS_4L, start=1):
        print(f"    Layer {i}: vel {vs:>3}-{ve}")
    print("=" * 60)


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main() -> int:
    parser = argparse.ArgumentParser(
        description="Visualize XPM velocity zone layout per keygroup as ASCII art.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument("xpm_path", help="Path to .xpm program file")
    parser.add_argument(
        "--keygroup", "-k",
        type=int,
        default=None,
        metavar="N",
        help="Show only keygroup with instrument number N (1-based).",
    )
    parser.add_argument(
        "--strict",
        action="store_true",
        help="Exit with code 1 if any gap, overlap, or non-standard split found.",
    )
    parser.add_argument(
        "--tolerance",
        type=int,
        default=DEFAULT_SPLIT_TOLERANCE,
        metavar="N",
        help=f"Split boundary tolerance in velocity units (default {DEFAULT_SPLIT_TOLERANCE}).",
    )
    args = parser.parse_args()

    xpm_path = Path(args.xpm_path)
    if not xpm_path.exists():
        print(f"ERROR: file not found: {xpm_path}", file=sys.stderr)
        return 1

    try:
        keygroups = parse_xpm(xpm_path)
    except ET.ParseError as exc:
        print(f"ERROR: XML parse failed: {exc}", file=sys.stderr)
        return 1

    if not keygroups:
        print("No keygroups with velocity layers found.")
        return 0

    print()
    print(f"XPM Velocity Zone Visualizer")
    print(f"File : {xpm_path.name}")
    print(f"Keygroups with layers: {len(keygroups)}")
    print()

    # Filter to single keygroup if requested
    if args.keygroup is not None:
        target = [kg for kg in keygroups if kg.number == args.keygroup]
        if not target:
            print(
                f"ERROR: keygroup {args.keygroup} not found. "
                f"Available: {[kg.number for kg in keygroups]}",
                file=sys.stderr,
            )
            return 1
        keygroups = target

    # Print ruler once before all keygroups (unless single keygroup with its own header)
    if len(keygroups) > 1:
        print(f"  vel: {render_ruler()}")
        print(f"       {render_tick()}")
        print()

    total           = len(keygroups)
    count_gap       = 0
    count_overlap   = 0
    count_nonstandard = 0
    count_clean     = 0

    for kg in sorted(keygroups, key=lambda k: k.number):
        stats = display_keygroup(
            kg,
            tolerance=args.tolerance,
            show_header=(len(keygroups) == 1),
        )
        if stats["has_gap"]:
            count_gap += 1
        if stats["has_overlap"]:
            count_overlap += 1
        if stats["has_nonstandard_split"]:
            count_nonstandard += 1
        if not stats["has_gap"] and not stats["has_overlap"] and not stats["has_nonstandard_split"]:
            count_clean += 1

    # Summary (only when showing multiple keygroups)
    if len(keygroups) > 1:
        print_summary(
            total=total,
            clean=count_clean,
            with_gaps=count_gap,
            with_overlaps=count_overlap,
            with_nonstandard=count_nonstandard,
        )

    if args.strict and (count_gap > 0 or count_overlap > 0 or count_nonstandard > 0):
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
