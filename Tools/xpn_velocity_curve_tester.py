#!/usr/bin/env python3
"""
xpn_velocity_curve_tester.py — XO_OX Designs
==============================================
Parse an Akai MPC XPM program file, validate each keygroup's velocity layer
splits, and generate a per-keygroup audit report with an overall program score.

Vibe's canonical 4-layer curve:
  Layer 1: vel   1 –  40  (pp  — soft)
  Layer 2: vel  41 –  90  (mp/mf — medium)
  Layer 3: vel  91 – 110  (f   — hard)
  Layer 4: vel 111 – 127  (ff  — max)

Four validation rules applied per keygroup:
  R1  Full-range coverage  — layers cover vel 1-127 with no gaps, no overlaps
  R2  Canonical compliance — 4-layer keygroups match Vibe's curve within ±TOLERANCE
  R3  Balanced splits      — no layer spans < MIN_LAYER (5) or > MAX_LAYER (80) units
  R4  VelStart=0 rule      — at least one placeholder layer with VelStart=0 present
                             (empty layers are preserved in the raw parse for this check)

ASCII velocity bar: 64 columns, each column = 2 velocity units (1-127 range).

Usage:
    python xpn_velocity_curve_tester.py <program.xpm>
    python xpn_velocity_curve_tester.py <program.xpm> --canonical
    python xpn_velocity_curve_tester.py <program.xpm> --fix-suggestions
    python xpn_velocity_curve_tester.py <program.xpm> --output report.txt

Options:
    --canonical         Only test keygroups against Vibe's canonical curve; skip
                        other rules for non-4-layer keygroups.
    --fix-suggestions   Append corrected VelStart/VelEnd proposals for any
                        non-compliant layer boundary.
    --output <file>     Write the full report to a text file in addition to stdout.
    --tolerance <n>     Boundary tolerance in velocity units for R2 (default 5).
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

CANONICAL_SPLITS = [
    (1, 40),
    (41, 90),
    (91, 110),
    (111, 127),
]

DEFAULT_TOLERANCE = 5
MIN_LAYER_SPAN    = 5
MAX_LAYER_SPAN    = 80
MAP_WIDTH         = 64   # display columns
VEL_MIN           = 1
VEL_MAX           = 127
VEL_RANGE         = VEL_MAX - VEL_MIN + 1   # 127 units

# Test velocities from the spec
TEST_VELOCITIES = [10, 40, 41, 90, 91, 110, 111, 127]

# ---------------------------------------------------------------------------
# Data model
# ---------------------------------------------------------------------------

@dataclass
class RawLayer:
    """A layer exactly as it appears in the XPM file, including empty placeholders."""
    index: int
    vel_start: int
    vel_end: int
    sample_file: str

    @property
    def is_placeholder(self) -> bool:
        return self.vel_start == 0

    @property
    def span(self) -> int:
        if self.is_placeholder:
            return 0
        return self.vel_end - self.vel_start + 1


@dataclass
class Keygroup:
    number: int
    root_note: int
    low_note: int
    high_note: int
    raw_layers: list[RawLayer] = field(default_factory=list)

    @property
    def active_layers(self) -> list[RawLayer]:
        """Non-placeholder layers only, sorted by VelStart."""
        return sorted(
            [l for l in self.raw_layers if not l.is_placeholder],
            key=lambda l: l.vel_start,
        )

    @property
    def has_placeholder(self) -> bool:
        return any(l.is_placeholder for l in self.raw_layers)


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
    tree = ET.parse(path)
    root = tree.getroot()

    keygroups: list[Keygroup] = []
    for inst_el in root.findall(".//Instruments/Instrument"):
        inst_num  = _int(inst_el.get("number"), default=0)
        root_note = _int(inst_el.findtext("RootNote"), default=60)
        low_note  = _int(inst_el.findtext("LowNote"),  default=0)
        high_note = _int(inst_el.findtext("HighNote"), default=127)

        raw_layers: list[RawLayer] = []
        for layer_idx, layer_el in enumerate(inst_el.findall(".//Layer"), start=1):
            vs = _int(layer_el.findtext("VelStart"), default=0)
            ve = _int(layer_el.findtext("VelEnd"),   default=0)
            sf = (layer_el.findtext("SampleFile") or "").strip()
            raw_layers.append(RawLayer(index=layer_idx, vel_start=vs, vel_end=ve, sample_file=sf))

        if not any(not l.is_placeholder for l in raw_layers):
            continue

        keygroups.append(Keygroup(
            number=inst_num,
            root_note=root_note,
            low_note=low_note,
            high_note=high_note,
            raw_layers=raw_layers,
        ))
    return keygroups


# ---------------------------------------------------------------------------
# Validation helpers
# ---------------------------------------------------------------------------

@dataclass
class RuleResult:
    passed: bool
    label: str
    detail: str


def check_full_coverage(kg: Keygroup) -> RuleResult:
    """R1: no gaps, no overlaps across vel 1-127."""
    layers = kg.active_layers
    if not layers:
        return RuleResult(False, "R1 Full Coverage", "No active layers found")

    issues: list[str] = []
    coverage = [0] * (VEL_MAX + 1)   # index 0 unused; 1-127 used

    for lyr in layers:
        for v in range(lyr.vel_start, lyr.vel_end + 1):
            if 1 <= v <= VEL_MAX:
                coverage[v] += 1

    gaps: list[tuple[int, int]] = []
    overlaps: list[tuple[int, int]] = []
    in_gap = in_overlap = False
    gap_start = ovl_start = 0

    for v in range(VEL_MIN, VEL_MAX + 1):
        if coverage[v] == 0:
            if not in_gap:
                in_gap = True
                gap_start = v
        else:
            if in_gap:
                gaps.append((gap_start, v - 1))
                in_gap = False

        if coverage[v] > 1:
            if not in_overlap:
                in_overlap = True
                ovl_start = v
        else:
            if in_overlap:
                overlaps.append((ovl_start, v - 1))
                in_overlap = False

    if in_gap:
        gaps.append((gap_start, VEL_MAX))
    if in_overlap:
        overlaps.append((ovl_start, VEL_MAX))

    if gaps:
        issues.append("Gaps at: " + ", ".join(f"{a}-{b}" for a, b in gaps))
    if overlaps:
        issues.append("Overlaps at: " + ", ".join(f"{a}-{b}" for a, b in overlaps))

    if issues:
        return RuleResult(False, "R1 Full Coverage", "; ".join(issues))
    return RuleResult(True, "R1 Full Coverage", "Clean 1-127 coverage")


def check_canonical(kg: Keygroup, tolerance: int) -> RuleResult:
    """R2: if 4 layers, compare against Vibe's canonical curve."""
    layers = kg.active_layers
    if len(layers) != 4:
        return RuleResult(
            True,
            "R2 Canonical Curve",
            f"Skip — {len(layers)} active layers (canonical check applies to 4-layer keygroups)",
        )

    mismatches: list[str] = []
    for i, (lyr, (cstart, cend)) in enumerate(zip(layers, CANONICAL_SPLITS), start=1):
        if abs(lyr.vel_start - cstart) > tolerance:
            mismatches.append(
                f"L{i} VelStart {lyr.vel_start} (expected {cstart} ±{tolerance})"
            )
        if abs(lyr.vel_end - cend) > tolerance:
            mismatches.append(
                f"L{i} VelEnd {lyr.vel_end} (expected {cend} ±{tolerance})"
            )

    if mismatches:
        return RuleResult(False, "R2 Canonical Curve", "; ".join(mismatches))
    return RuleResult(True, "R2 Canonical Curve", f"Matches Vibe's curve within ±{tolerance}")


def check_balanced(kg: Keygroup) -> RuleResult:
    """R3: no layer < MIN_LAYER_SPAN or > MAX_LAYER_SPAN velocity units."""
    layers = kg.active_layers
    issues: list[str] = []
    for lyr in layers:
        if lyr.span < MIN_LAYER_SPAN:
            issues.append(f"L{lyr.index} too narrow ({lyr.span} units, min {MIN_LAYER_SPAN})")
        if lyr.span > MAX_LAYER_SPAN:
            issues.append(f"L{lyr.index} too wide ({lyr.span} units, max {MAX_LAYER_SPAN})")
    if issues:
        return RuleResult(False, "R3 Balanced Splits", "; ".join(issues))
    return RuleResult(True, "R3 Balanced Splits", "All layer spans within acceptable range")


def check_placeholder(kg: Keygroup) -> RuleResult:
    """R4: at least one layer with VelStart=0 (empty placeholder)."""
    if kg.has_placeholder:
        return RuleResult(True, "R4 VelStart=0 Rule", "Placeholder layer present")
    return RuleResult(
        False,
        "R4 VelStart=0 Rule",
        "No placeholder layer found (VelStart=0 required to prevent ghost triggering)",
    )


# ---------------------------------------------------------------------------
# Velocity simulation
# ---------------------------------------------------------------------------

def simulate_velocity(kg: Keygroup, velocities: list[int]) -> list[tuple[int, Optional[int], str]]:
    """
    For each test velocity return (velocity, layer_index_or_None, sample_name).
    Layer index is 1-based position in the sorted active layers list.
    """
    results: list[tuple[int, Optional[int], str]] = []
    layers = kg.active_layers
    for v in velocities:
        fired: Optional[tuple[int, str]] = None
        for idx, lyr in enumerate(layers, start=1):
            if lyr.vel_start <= v <= lyr.vel_end:
                sample = Path(lyr.sample_file).name if lyr.sample_file else "(no sample)"
                fired = (idx, sample)
                break
        if fired:
            results.append((v, fired[0], fired[1]))
        else:
            results.append((v, None, "(no layer fires — gap)"))
    return results


# ---------------------------------------------------------------------------
# ASCII velocity bar
# ---------------------------------------------------------------------------

def render_velocity_bar(kg: Keygroup) -> str:
    """
    64-column bar, 2 velocity units per column (covering vel 1-127).
    Each column shows: layer number, X for overlap, . for gap.
    """
    layers = kg.active_layers
    cols: list[str] = []

    for col in range(MAP_WIDTH):
        v_start = VEL_MIN + col * 2        # first velocity unit in this column
        v_end   = v_start + 1              # second velocity unit (may exceed 127)

        hits: set[int] = set()
        for idx, lyr in enumerate(layers, start=1):
            for v in range(v_start, min(v_end, VEL_MAX) + 1):
                if lyr.vel_start <= v <= lyr.vel_end:
                    hits.add(idx)

        if len(hits) == 0:
            cols.append(".")
        elif len(hits) > 1:
            cols.append("X")
        else:
            n = next(iter(hits))
            cols.append(str(n) if n <= 9 else "+")

    return "".join(cols)


def render_ruler() -> str:
    """Ruler showing approximate velocity positions beneath the bar."""
    # Mark every 16 velocity units — positions 1,17,33,49,...,113
    # In 64-col display: col = (v - 1) // 2
    marks = {}
    for v in range(1, VEL_MAX + 1, 16):
        col = (v - 1) // 2
        marks[col] = str(v)

    ruler = []
    col = 0
    while col < MAP_WIDTH:
        if col in marks:
            label = marks[col]
            ruler.append(label)
            col += len(label)
        else:
            ruler.append(" ")
            col += 1
    return "".join(ruler)


# ---------------------------------------------------------------------------
# Fix suggestions
# ---------------------------------------------------------------------------

def fix_suggestions(kg: Keygroup) -> list[str]:
    """
    Propose corrected VelStart/VelEnd values.
    Strategy: redistribute 1-127 evenly across the active layer count, then
    annotate deviations from the canonical curve if 4 layers.
    """
    layers = kg.active_layers
    n = len(layers)
    if n == 0:
        return ["No active layers — cannot suggest fixes."]

    lines: list[str] = []

    # Propose even redistribution
    span = VEL_RANGE  # 127 units
    base = span // n
    remainder = span % n

    proposed: list[tuple[int, int]] = []
    cursor = VEL_MIN
    for i in range(n):
        extra = 1 if i < remainder else 0
        end = cursor + base + extra - 1
        proposed.append((cursor, end))
        cursor = end + 1

    lines.append(f"  Even redistribution ({n} layers):")
    for i, (s, e) in enumerate(proposed, start=1):
        lines.append(f"    Layer {i}: VelStart={s}  VelEnd={e}  (span={e - s + 1})")

    if n == 4:
        lines.append(f"  Vibe canonical curve (recommended for 4-layer keygroups):")
        for i, (s, e) in enumerate(CANONICAL_SPLITS, start=1):
            lines.append(f"    Layer {i}: VelStart={s}  VelEnd={e}  (span={e - s + 1})")

    return lines


# ---------------------------------------------------------------------------
# Report builder
# ---------------------------------------------------------------------------

def audit_keygroup(
    kg: Keygroup,
    tolerance: int,
    canonical_only: bool,
    show_fixes: bool,
) -> tuple[list[str], int]:
    """
    Returns (lines, score) where score is 0-100 based on rules passed.
    """
    lines: list[str] = []
    layers = kg.active_layers
    total_layers = len(kg.raw_layers)
    placeholder_count = sum(1 for l in kg.raw_layers if l.is_placeholder)

    lines.append(
        f"Keygroup {kg.number:>3}  root={kg.root_note}  "
        f"note={kg.low_note}-{kg.high_note}  "
        f"layers={len(layers)} active / {total_layers} total "
        f"({placeholder_count} placeholder)"
    )

    # Velocity bar
    bar = render_velocity_bar(kg)
    ruler = render_ruler()
    lines.append(f"  vel |{bar}|")
    lines.append(f"      |{ruler}|")
    lines.append(f"       1{'':>61}127")

    # Layer summary
    for lyr in layers:
        lines.append(
            f"  L{lyr.index}: {lyr.vel_start:>3}-{lyr.vel_end:<3}  "
            f"span={lyr.span:<3}  {Path(lyr.sample_file).name if lyr.sample_file else '(none)'}"
        )

    # Rules
    results: list[RuleResult] = []
    if not canonical_only:
        results.append(check_full_coverage(kg))
        results.append(check_canonical(kg, tolerance))
        results.append(check_balanced(kg))
        results.append(check_placeholder(kg))
    else:
        results.append(check_canonical(kg, tolerance))

    pass_count = 0
    for r in results:
        icon = "PASS" if r.passed else "FAIL"
        lines.append(f"  [{icon}] {r.label}: {r.detail}")
        if r.passed:
            pass_count += 1

    score = int(100 * pass_count / len(results)) if results else 0
    lines.append(f"  Score: {pass_count}/{len(results)} rules passed ({score}%)")

    # Velocity simulation
    sim = simulate_velocity(kg, TEST_VELOCITIES)
    lines.append("  Velocity simulation:")
    for vel, layer_idx, sample in sim:
        layer_str = f"Layer {layer_idx}" if layer_idx is not None else "NO LAYER"
        lines.append(f"    vel {vel:>3} → {layer_str:<8}  {sample}")

    # Fix suggestions
    if show_fixes and pass_count < len(results):
        lines.append("  Fix suggestions:")
        lines.extend(fix_suggestions(kg))

    return lines, score


def build_report(
    path: Path,
    keygroups: list[Keygroup],
    tolerance: int,
    canonical_only: bool,
    show_fixes: bool,
) -> tuple[str, int]:
    """Build the full report string and return (report, overall_score)."""
    out: list[str] = []

    out.append("=" * 72)
    out.append(f"XPN Velocity Curve Tester — XO_OX Designs")
    out.append(f"File     : {path}")
    out.append(f"Keygroups: {len(keygroups)}")
    out.append(f"Tolerance: ±{tolerance} velocity units (canonical boundary check)")
    out.append("=" * 72)
    out.append("")

    if not keygroups:
        out.append("No active keygroups found in this XPM file.")
        return "\n".join(out), 0

    scores: list[int] = []
    for kg in keygroups:
        lines, score = audit_keygroup(kg, tolerance, canonical_only, show_fixes)
        out.extend(lines)
        out.append("")
        scores.append(score)

    overall = int(sum(scores) / len(scores)) if scores else 0

    out.append("-" * 72)
    out.append(f"Program Score: {overall}%  ({len(scores)} keygroup(s) evaluated)")
    if overall == 100:
        out.append("Status: ALL RULES PASS — velocity curve is production-ready.")
    elif overall >= 75:
        out.append("Status: MOSTLY COMPLIANT — minor fixes recommended.")
    else:
        out.append("Status: NEEDS ATTENTION — significant velocity curve issues found.")
    out.append("=" * 72)

    return "\n".join(out), overall


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main() -> None:
    parser = argparse.ArgumentParser(
        description="Validate velocity curve layers in an Akai MPC XPM program file."
    )
    parser.add_argument("xpm", type=Path, help="Path to the .xpm program file")
    parser.add_argument(
        "--canonical",
        action="store_true",
        help="Only check 4-layer keygroups against Vibe's canonical curve; skip other rules",
    )
    parser.add_argument(
        "--fix-suggestions",
        action="store_true",
        dest="fix_suggestions",
        help="Append corrected VelStart/VelEnd proposals for non-compliant keygroups",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=None,
        metavar="FILE",
        help="Write report to this file in addition to stdout",
    )
    parser.add_argument(
        "--tolerance",
        type=int,
        default=DEFAULT_TOLERANCE,
        metavar="N",
        help=f"Boundary tolerance for canonical curve check (default {DEFAULT_TOLERANCE})",
    )
    args = parser.parse_args()

    xpm_path: Path = args.xpm
    if not xpm_path.exists():
        print(f"ERROR: File not found: {xpm_path}", file=sys.stderr)
        sys.exit(1)
    if xpm_path.suffix.lower() != ".xpm":
        print(f"WARNING: Expected a .xpm file, got: {xpm_path.suffix}", file=sys.stderr)

    try:
        keygroups = parse_xpm(xpm_path)
    except ET.ParseError as exc:
        print(f"ERROR: Failed to parse XPM XML: {exc}", file=sys.stderr)
        sys.exit(1)

    report, overall_score = build_report(
        path=xpm_path,
        keygroups=keygroups,
        tolerance=args.tolerance,
        canonical_only=args.canonical,
        show_fixes=args.fix_suggestions,
    )

    print(report)

    if args.output:
        args.output.write_text(report, encoding="utf-8")
        print(f"\nReport saved to: {args.output}")

    sys.exit(0 if overall_score == 100 else 1)


if __name__ == "__main__":
    main()
