#!/usr/bin/env python3
"""
CI Velocity Parity Gate — Tools/ci_check_velocity_parity.py
============================================================
Verifies that the C++ XPNDrumExporter.h velocity zone boundaries and render
velocities stay in sync with the Python canonical source of truth:

  Python canonical : Tools/xpn_velocity_standard.py   (ZONES, RENDER_MIDPOINTS)
  JSON canonical   : Tools/xpn-spec.json               (velocity.zones)
  C++ consumer     : Source/Export/XPNDrumExporter.h   (XPNVelocityCurves::Musical
                                                         via velRangeForLayer /
                                                         velocityForDrumLayer)

Exit codes:
  0  — all checks pass
  1  — parity violation found (with diff printed to stderr)

Usage:
  python Tools/ci_check_velocity_parity.py
  python Tools/ci_check_velocity_parity.py --cpp-header path/to/XPNDrumExporter.h
  python Tools/ci_check_velocity_parity.py --curves-header path/to/XPNVelocityCurves.h
  python Tools/ci_check_velocity_parity.py --spec path/to/xpn-spec.json

Author:  XO_OX Designs
Created: 2026-04-23 (QDD ship-blocker U2, issue #1187)
"""

import argparse
import importlib.util
import json
import re
import sys
from pathlib import Path

# ---------------------------------------------------------------------------
# Resolve repo root relative to this script
# ---------------------------------------------------------------------------

_TOOLS_DIR = Path(__file__).resolve().parent
_REPO_ROOT = _TOOLS_DIR.parent

_DEFAULT_VELOCITY_STANDARD = _TOOLS_DIR / "xpn_velocity_standard.py"
_DEFAULT_CURVES_HEADER = _REPO_ROOT / "Source" / "Export" / "XPNVelocityCurves.h"
_DEFAULT_DRUM_EXPORTER = _REPO_ROOT / "Source" / "Export" / "XPNDrumExporter.h"
_DEFAULT_SPEC_JSON = _TOOLS_DIR / "xpn-spec.json"

# Tolerance for float comparison (normalised velocity ratio, unitless)
_FLOAT_TOLERANCE = 1e-3

# ---------------------------------------------------------------------------
# Step 1: Load Python canonical values
# ---------------------------------------------------------------------------

def load_python_standard(path: Path):
    """Import xpn_velocity_standard and return (ZONES, RENDER_MIDPOINTS)."""
    spec = importlib.util.spec_from_file_location("xpn_velocity_standard", path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return list(mod.ZONES), list(mod.RENDER_MIDPOINTS)


# ---------------------------------------------------------------------------
# Step 2: Parse C++ XPNVelocityCurves.h Musical table
# ---------------------------------------------------------------------------

def parse_musical_curve_from_cpp(curves_header: Path):
    """
    Extract the Musical curve VelocitySplit entries from XPNVelocityCurves.h.

    Looks for the line:
        full = {{1, 20, 0.30f, 0.08f}, {21, 55, 0.55f, 0.30f}, ...};
    inside the `case XPNVelocityCurve::Musical:` block.

    Returns list of (start, end, volume, normVel) tuples.
    """
    text = curves_header.read_text(encoding="utf-8")

    # Locate the Musical case block
    musical_match = re.search(
        r"case\s+XPNVelocityCurve::Musical\s*:\s*\n.*?full\s*=\s*(\{[^;]+\})\s*;",
        text,
        re.DOTALL,
    )
    if not musical_match:
        raise ValueError(
            f"Could not locate 'case XPNVelocityCurve::Musical' full= assignment "
            f"in {curves_header}"
        )

    raw = musical_match.group(1)

    # Extract each {start, end, volume, normVel} entry
    entry_pat = re.compile(
        r"\{(\d+),\s*(\d+),\s*([\d.]+)f?,\s*([\d.]+)f?\}"
    )
    entries = []
    for m in entry_pat.finditer(raw):
        start = int(m.group(1))
        end = int(m.group(2))
        volume = float(m.group(3))
        norm_vel = float(m.group(4))
        entries.append((start, end, volume, norm_vel))

    if not entries:
        raise ValueError(
            f"No VelocitySplit entries found in Musical curve block of {curves_header}"
        )

    return entries


# ---------------------------------------------------------------------------
# Step 3: Parse XPNDrumExporter.h — verify it delegates (not duplicates)
# ---------------------------------------------------------------------------

def check_exporter_delegates(exporter_header: Path) -> list:
    """
    Verify that XPNDrumExporter.h delegates velocity logic to XPNVelocityCurves.h
    rather than duplicating hardcoded tables.

    Returns a list of warning strings (empty = OK, non-fatal advisory).
    Raises ValueError on hard violations (duplicate hardcoded tables still present).
    """
    warnings = []
    text = exporter_header.read_text(encoding="utf-8")

    # Hard check: old hardcoded even-split values must NOT be present
    old_even_split_pattern = re.compile(
        r"VelRange\s+ranges\s*\[.*?\]\s*=\s*\{.*?\{1,\s*31\}.*?\{32,\s*63\}",
        re.DOTALL,
    )
    if old_even_split_pattern.search(text):
        raise ValueError(
            "XPNDrumExporter.h still contains old hardcoded even-split velocity ranges "
            "{1,31},{32,63},{64,95},{96,127}. These must be removed — the exporter "
            "must delegate to XPNVelocityCurves::Musical via getVelocitySplits()."
        )

    old_float_pattern = re.compile(
        r"float\s+vels\s*\[\s*\]\s*=\s*\{.*?0\.2f.*?0\.5f.*?0\.75f.*?1\.0f",
        re.DOTALL,
    )
    if old_float_pattern.search(text):
        raise ValueError(
            "XPNDrumExporter.h still contains old hardcoded velocity floats "
            "{0.2f, 0.5f, 0.75f, 1.0f}. These must be removed — the exporter "
            "must delegate to XPNVelocityCurves::Musical via renderVelocityForLayer()."
        )

    # Advisory: confirm delegation calls are present
    if "getVelocitySplits" not in text:
        warnings.append(
            "velRangeForLayer does not call getVelocitySplits() — delegation may be missing"
        )
    if "renderVelocityForLayer" not in text:
        warnings.append(
            "velocityForDrumLayer does not call renderVelocityForLayer() — delegation may be missing"
        )
    if "XPNVelocityCurves.h" not in text and "XPNVelocityCurve" not in text:
        warnings.append(
            "XPNDrumExporter.h does not include or reference XPNVelocityCurves — "
            "delegation may be incomplete"
        )

    return warnings


# ---------------------------------------------------------------------------
# Step 4: Compare Python ZONES vs C++ Musical curve
# ---------------------------------------------------------------------------

def compare_zones(py_zones, cpp_entries) -> list:
    """Return list of mismatch error strings (empty = OK)."""
    errors = []

    if len(py_zones) != len(cpp_entries):
        errors.append(
            f"Layer count mismatch: Python has {len(py_zones)} zones, "
            f"C++ Musical curve has {len(cpp_entries)} entries"
        )
        return errors

    for i, ((py_start, py_end), (cpp_start, cpp_end, _vol, _norm)) in enumerate(
        zip(py_zones, cpp_entries)
    ):
        if py_start != cpp_start:
            errors.append(
                f"Layer {i} VelStart: Python={py_start}, C++ Musical={cpp_start}"
            )
        if py_end != cpp_end:
            errors.append(
                f"Layer {i} VelEnd: Python={py_end}, C++ Musical={cpp_end}"
            )

    return errors


# ---------------------------------------------------------------------------
# Step 5: Compare Python RENDER_MIDPOINTS vs C++ normVel
# ---------------------------------------------------------------------------

def compare_render_velocities(py_midpoints, cpp_entries) -> list:
    """Return list of mismatch error strings (empty = OK)."""
    errors = []

    if len(py_midpoints) != len(cpp_entries):
        return [
            f"Cannot compare render velocities: layer count mismatch "
            f"({len(py_midpoints)} Python vs {len(cpp_entries)} C++)"
        ]

    for i, (py_mid, (_s, _e, _vol, cpp_norm)) in enumerate(
        zip(py_midpoints, cpp_entries)
    ):
        py_norm = py_mid / 127.0
        if abs(py_norm - cpp_norm) > _FLOAT_TOLERANCE:
            errors.append(
                f"Layer {i} normVel: Python midpoint={py_mid}/127={py_norm:.6f}, "
                f"C++ normVel={cpp_norm:.6f}  "
                f"(diff={abs(py_norm - cpp_norm):.6f} > tolerance={_FLOAT_TOLERANCE})"
            )

    return errors


# ---------------------------------------------------------------------------
# Step 6: Validate xpn-spec.json (optional — only if file exists)
# ---------------------------------------------------------------------------

def check_spec_json(spec_path: Path, py_zones, py_midpoints) -> list:
    """
    Compare xpn-spec.json velocity zones against Python canonical.

    Per LVL 3 DA: extend to read xpn-spec.json and validate ALL curves it
    defines are reflected somewhere C++ side.

    Returns list of error strings (empty = OK).
    """
    errors = []

    if not spec_path.exists():
        return []  # Spec JSON is optional — no error if absent

    try:
        spec = json.loads(spec_path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as exc:
        errors.append(f"xpn-spec.json is not valid JSON: {exc}")
        return errors

    vel_block = spec.get("velocity", {})
    json_zones = vel_block.get("zones", [])

    if not json_zones:
        errors.append(
            f"xpn-spec.json has no velocity.zones array — "
            f"expected {len(py_zones)} zone entries"
        )
        return errors

    if len(json_zones) != len(py_zones):
        errors.append(
            f"xpn-spec.json velocity.zones count={len(json_zones)}, "
            f"Python ZONES count={len(py_zones)}"
        )

    for i, (zone, (py_start, py_end)) in enumerate(
        zip(json_zones, py_zones)
    ):
        json_start = zone.get("velStart")
        json_end = zone.get("velEnd")
        json_mid = zone.get("renderMidpoint")

        if json_start != py_start:
            errors.append(
                f"xpn-spec.json zone[{i}] velStart={json_start}, "
                f"Python ZONES[{i}][0]={py_start}"
            )
        if json_end != py_end:
            errors.append(
                f"xpn-spec.json zone[{i}] velEnd={json_end}, "
                f"Python ZONES[{i}][1]={py_end}"
            )
        if i < len(py_midpoints) and json_mid != py_midpoints[i]:
            errors.append(
                f"xpn-spec.json zone[{i}] renderMidpoint={json_mid}, "
                f"Python RENDER_MIDPOINTS[{i}]={py_midpoints[i]}"
            )

    # LVL 3 DA: check that all named curves in spec's tiers block are defined in C++
    # (advisory only — tiers don't directly map to XPNVelocityCurve enum names)
    # For now: verify DEEP/TRENCH/SURFACE tier velLayers match Python NUM_LAYERS
    tiers = spec.get("tiers", {})
    for tier_name, tier_data in tiers.items():
        json_layers = tier_data.get("velLayers", None)
        if json_layers is not None and json_layers > 1 and json_layers != len(py_zones):
            errors.append(
                f"xpn-spec.json tiers.{tier_name}.velLayers={json_layers} "
                f"does not match Python NUM_LAYERS={len(py_zones)}"
            )

    return errors


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def parse_args(argv=None):
    p = argparse.ArgumentParser(
        description="CI gate: verify Python↔C++ velocity zone parity.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    p.add_argument(
        "--velocity-standard",
        default=str(_DEFAULT_VELOCITY_STANDARD),
        metavar="PY",
        help="Path to xpn_velocity_standard.py (default: auto-detected)",
    )
    p.add_argument(
        "--curves-header",
        default=str(_DEFAULT_CURVES_HEADER),
        metavar="H",
        help="Path to XPNVelocityCurves.h (default: auto-detected)",
    )
    p.add_argument(
        "--cpp-header",
        default=str(_DEFAULT_DRUM_EXPORTER),
        metavar="H",
        help="Path to XPNDrumExporter.h (default: auto-detected)",
    )
    p.add_argument(
        "--spec",
        default=str(_DEFAULT_SPEC_JSON),
        metavar="JSON",
        help="Path to xpn-spec.json (default: auto-detected; optional)",
    )
    p.add_argument(
        "--tolerance",
        type=float,
        default=_FLOAT_TOLERANCE,
        metavar="T",
        help=f"Float comparison tolerance for normVel (default: {_FLOAT_TOLERANCE})",
    )
    return p.parse_args(argv)


def main(argv=None) -> int:
    args = parse_args(argv)

    py_path = Path(args.velocity_standard)
    curves_path = Path(args.curves_header)
    exporter_path = Path(args.cpp_header)
    spec_path = Path(args.spec)

    all_errors: list = []
    all_warnings: list = []

    # ---- Load Python canonical ----
    print(f"[1/5] Loading Python standard: {py_path}")
    if not py_path.exists():
        print(f"ERROR: {py_path} not found", file=sys.stderr)
        return 1
    try:
        py_zones, py_midpoints = load_python_standard(py_path)
    except Exception as exc:
        print(f"ERROR loading Python standard: {exc}", file=sys.stderr)
        return 1
    print(f"      ZONES          = {py_zones}")
    print(f"      RENDER_MIDPOINTS = {py_midpoints}")

    # ---- Parse C++ Musical curve ----
    print(f"[2/5] Parsing C++ Musical curve: {curves_path}")
    if not curves_path.exists():
        print(f"ERROR: {curves_path} not found", file=sys.stderr)
        return 1
    try:
        cpp_entries = parse_musical_curve_from_cpp(curves_path)
    except Exception as exc:
        print(f"ERROR parsing C++ Musical curve: {exc}", file=sys.stderr)
        return 1
    print(f"      Entries: {[(s, e, nv) for s, e, _v, nv in cpp_entries]}")

    # ---- Check exporter delegates ----
    print(f"[3/5] Checking XPNDrumExporter.h delegates to XPNVelocityCurves.h: {exporter_path}")
    if not exporter_path.exists():
        print(f"ERROR: {exporter_path} not found", file=sys.stderr)
        return 1
    try:
        exporter_warnings = check_exporter_delegates(exporter_path)
        all_warnings.extend(exporter_warnings)
    except ValueError as exc:
        all_errors.append(f"XPNDrumExporter.h hard violation: {exc}")

    # ---- Compare zones ----
    print("[4/5] Comparing velocity zone boundaries...")
    zone_errors = compare_zones(py_zones, cpp_entries)
    all_errors.extend(zone_errors)

    vel_errors = compare_render_velocities(py_midpoints, cpp_entries)
    all_errors.extend(vel_errors)

    # ---- Check spec JSON ----
    print(f"[5/5] Validating xpn-spec.json: {spec_path}")
    spec_errors = check_spec_json(spec_path, py_zones, py_midpoints)
    all_errors.extend(spec_errors)

    # ---- Report ----
    print()
    if all_warnings:
        print("WARNINGS:", file=sys.stderr)
        for w in all_warnings:
            print(f"  [WARN] {w}", file=sys.stderr)
        print()

    if all_errors:
        print(
            f"PARITY VIOLATION: {len(all_errors)} error(s) found.\n"
            f"Python canonical (xpn_velocity_standard.py) and C++ "
            f"(XPNVelocityCurves.h / XPNDrumExporter.h) are out of sync.",
            file=sys.stderr,
        )
        print(file=sys.stderr)
        for err in all_errors:
            print(f"  [FAIL] {err}", file=sys.stderr)
        print(file=sys.stderr)
        print(
            "Fix: update XPNVelocityCurves.h Musical curve to match "
            "xpn_velocity_standard.py ZONES and RENDER_MIDPOINTS, then re-run.",
            file=sys.stderr,
        )
        return 1

    print("OK: Python↔C++ velocity parity confirmed.")
    if spec_path.exists():
        print("OK: xpn-spec.json matches Python canonical.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
