#!/usr/bin/env python3
"""
xpn_layer_balance_checker.py — XO_OX Designs
Check volume balance across velocity layers within a .xpn program.

Unbalanced layers cause jarring volume jumps at velocity crossover points,
which is a sound design problem.  This tool reads every .xpm inside a .xpn
ZIP, inspects Layer Volume attributes and velocity boundary continuity, and
reports any mismatches.

Checks performed per Keygroup:
  1. Volume delta between adjacent velocity layers (sorted by VelStart)
       WARNING  delta > --warn  (default 20)
       ERROR    delta > --error (default 40)
  2. Volume=0 layers that have a sample file attached (likely a mistake)
  3. Velocity boundary continuity: VelEnd[N] should equal VelStart[N+1] - 1

Usage:
    python xpn_layer_balance_checker.py pack.xpn
    python xpn_layer_balance_checker.py pack.xpn --program "Warm Pad 01"
    python xpn_layer_balance_checker.py pack.xpn --format json
    python xpn_layer_balance_checker.py pack.xpn --strict
    python xpn_layer_balance_checker.py pack.xpn --warn 15 --error 35
"""

import argparse
import json
import sys
import zipfile
from xml.etree import ElementTree as ET
from typing import List, Optional


# ---------------------------------------------------------------------------
# Defaults
# ---------------------------------------------------------------------------

DEFAULT_WARN = 20
DEFAULT_ERROR = 40


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _int_attr(elem: ET.Element, name: str, default: int = 0) -> int:
    raw = elem.get(name)
    if raw is None:
        return default
    try:
        return int(raw)
    except ValueError:
        return default


def _str_attr(elem: ET.Element, name: str, default: str = "") -> str:
    return elem.get(name, default)


# ---------------------------------------------------------------------------
# Parsing
# ---------------------------------------------------------------------------

def parse_xpm(xml_text: str, program_name: str, warn_delta: int, error_delta: int) -> dict:
    """
    Parse a single XPM program and return findings dict:
      {
        "program": str,
        "keygroup_count": int,
        "layer_count": int,
        "findings": [
          {
            "keygroup_name": str,
            "keygroup_index": int,
            "severity": "WARNING"|"ERROR"|"INFO",
            "kind": "volume_jump"|"silent_layer"|"velocity_gap"|"velocity_overlap",
            "message": str,
            "layer_a": int,   # 1-based layer index (where applicable)
            "layer_b": int,
          }
        ],
        "parse_error": str | None,
      }
    """
    result: dict = {
        "program": program_name,
        "keygroup_count": 0,
        "layer_count": 0,
        "findings": [],
        "parse_error": None,
    }

    try:
        root = ET.fromstring(xml_text)
    except ET.ParseError as exc:
        result["parse_error"] = str(exc)
        return result

    # XPM structure: <MPCVObject> → <Program> → <Instruments> → <Instrument> → <Layers> → <Layer>
    # Keygroup = one <Instrument> element
    instruments = root.findall(".//Instrument")
    result["keygroup_count"] = len(instruments)

    for kg_index, instrument in enumerate(instruments):
        kg_name = _str_attr(instrument, "PadName") or _str_attr(instrument, "Name") or f"KG-{kg_index+1:02d}"

        layers_elem = instrument.find("Layers")
        if layers_elem is None:
            continue

        raw_layers = list(layers_elem.findall("Layer"))
        result["layer_count"] += len(raw_layers)

        if len(raw_layers) < 2:
            # Single-layer keygroups have nothing to compare; still check silent-with-sample
            for li, layer in enumerate(raw_layers):
                volume = _int_attr(layer, "Volume", 100)
                sample_file = _str_attr(layer, "SampleFile")
                if volume == 0 and sample_file:
                    result["findings"].append({
                        "keygroup_name": kg_name,
                        "keygroup_index": kg_index + 1,
                        "severity": "WARNING",
                        "kind": "silent_layer",
                        "message": (
                            f"Layer {li+1} Volume=0 but has sample '{sample_file}' — "
                            "intentionally silent or misconfigured?"
                        ),
                        "layer_a": li + 1,
                        "layer_b": li + 1,
                    })
            continue

        # Sort layers by VelStart ascending
        def vel_start(layer: ET.Element) -> int:
            return _int_attr(layer, "VelStart", 0)

        sorted_layers = sorted(raw_layers, key=vel_start)

        for li, layer in enumerate(sorted_layers):
            volume = _int_attr(layer, "Volume", 100)
            sample_file = _str_attr(layer, "SampleFile")

            # Check 1: silent layer with sample attached
            if volume == 0 and sample_file:
                result["findings"].append({
                    "keygroup_name": kg_name,
                    "keygroup_index": kg_index + 1,
                    "severity": "WARNING",
                    "kind": "silent_layer",
                    "message": (
                        f"Layer {li+1} Volume=0 but has sample '{sample_file}' — "
                        "intentionally silent or misconfigured?"
                    ),
                    "layer_a": li + 1,
                    "layer_b": li + 1,
                })

            if li == 0:
                continue

            prev_layer = sorted_layers[li - 1]
            prev_volume = _int_attr(prev_layer, "Volume", 100)
            delta = abs(volume - prev_volume)

            # Check 2: volume jump between adjacent layers
            if delta > error_delta:
                severity = "ERROR"
            elif delta > warn_delta:
                severity = "WARNING"
            else:
                severity = None

            if severity:
                sign = "+" if (volume - prev_volume) > 0 else "-"
                result["findings"].append({
                    "keygroup_name": kg_name,
                    "keygroup_index": kg_index + 1,
                    "severity": severity,
                    "kind": "volume_jump",
                    "message": (
                        f"Layer {li}→{li+1} volume jump {sign}{delta} "
                        f"(layer {li} Vol={prev_volume}, layer {li+1} Vol={volume})"
                    ),
                    "layer_a": li,
                    "layer_b": li + 1,
                })

            # Check 3: velocity boundary continuity
            prev_vel_end = _int_attr(prev_layer, "VelEnd", -1)
            cur_vel_start = _int_attr(layer, "VelStart", -1)

            if prev_vel_end < 0 or cur_vel_start < 0:
                continue  # missing attributes — skip

            expected_start = prev_vel_end + 1
            if cur_vel_start > expected_start:
                result["findings"].append({
                    "keygroup_name": kg_name,
                    "keygroup_index": kg_index + 1,
                    "severity": "WARNING",
                    "kind": "velocity_gap",
                    "message": (
                        f"Velocity gap between layer {li} end={prev_vel_end} "
                        f"and layer {li+1} start={cur_vel_start} "
                        f"(missing velocities {expected_start}–{cur_vel_start - 1})"
                    ),
                    "layer_a": li,
                    "layer_b": li + 1,
                })
            elif cur_vel_start < expected_start:
                result["findings"].append({
                    "keygroup_name": kg_name,
                    "keygroup_index": kg_index + 1,
                    "severity": "WARNING",
                    "kind": "velocity_overlap",
                    "message": (
                        f"Velocity overlap: layer {li} end={prev_vel_end} "
                        f"and layer {li+1} start={cur_vel_start} "
                        f"(overlap by {expected_start - cur_vel_start} velocity values)"
                    ),
                    "layer_a": li,
                    "layer_b": li + 1,
                })

    return result


# ---------------------------------------------------------------------------
# Reporting
# ---------------------------------------------------------------------------

def count_severity(findings: List[dict]) -> tuple:
    errors = sum(1 for f in findings if f["severity"] == "ERROR")
    warnings = sum(1 for f in findings if f["severity"] == "WARNING")
    return errors, warnings


def format_text(program_results: List[dict]) -> str:
    lines = []
    total_errors = 0
    total_warnings = 0

    for prog in program_results:
        if prog.get("parse_error"):
            lines.append(f"PROGRAM: {prog['program']} — PARSE ERROR: {prog['parse_error']}")
            continue

        findings = prog["findings"]
        errors, warnings = count_severity(findings)
        total_errors += errors
        total_warnings += warnings

        status = "OK" if not findings else f"{errors} errors, {warnings} warnings"
        lines.append(
            f"PROGRAM: {prog['program']} — "
            f"{prog['layer_count']} layers, {prog['keygroup_count']} keygroups — {status}"
        )

        # Group findings by keygroup for cleaner output
        by_kg: dict = {}
        for f in findings:
            key = (f["keygroup_index"], f["keygroup_name"])
            by_kg.setdefault(key, []).append(f)

        for (kg_idx, kg_name), kg_findings in sorted(by_kg.items()):
            for f in kg_findings:
                lines.append(f"  Keygroup {kg_name} [KG-{kg_idx:02d}]: {f['message']} ({f['severity']})")

    lines.append("")
    lines.append(f"SCORE: {total_errors} errors, {total_warnings} warnings")
    return "\n".join(lines)


def format_json(program_results: List[dict]) -> str:
    total_errors = 0
    total_warnings = 0
    for prog in program_results:
        e, w = count_severity(prog.get("findings", []))
        total_errors += e
        total_warnings += w

    output = {
        "summary": {
            "total_errors": total_errors,
            "total_warnings": total_warnings,
            "programs_checked": len(program_results),
        },
        "programs": program_results,
    }
    return json.dumps(output, indent=2)


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main() -> int:
    parser = argparse.ArgumentParser(
        description="Check volume balance across velocity layers in a .xpn pack.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument("xpn", help="Path to the .xpn ZIP file")
    parser.add_argument("--program", metavar="NAME", help="Only check this program name (substring match)")
    parser.add_argument(
        "--format", choices=["text", "json"], default="text",
        help="Output format (default: text)"
    )
    parser.add_argument(
        "--warn", type=int, default=DEFAULT_WARN, metavar="DELTA",
        help=f"Volume delta threshold for WARNING (default: {DEFAULT_WARN})"
    )
    parser.add_argument(
        "--error", type=int, default=DEFAULT_ERROR, metavar="DELTA",
        help=f"Volume delta threshold for ERROR (default: {DEFAULT_ERROR})"
    )
    parser.add_argument(
        "--strict", action="store_true",
        help="Exit 1 on any WARNING, exit 2 on any ERROR"
    )
    args = parser.parse_args()

    # Open .xpn ZIP
    try:
        zf = zipfile.ZipFile(args.xpn, "r")
    except FileNotFoundError:
        print(f"ERROR: File not found: {args.xpn}", file=sys.stderr)
        return 2
    except zipfile.BadZipFile:
        print(f"ERROR: Not a valid ZIP/XPN file: {args.xpn}", file=sys.stderr)
        return 2

    program_results: List[dict] = []

    with zf:
        xpm_entries = [n for n in zf.namelist() if n.lower().endswith(".xpm")]

        if not xpm_entries:
            print("ERROR: No .xpm files found inside the .xpn archive.", file=sys.stderr)
            return 2

        for entry in sorted(xpm_entries):
            # Derive program name from filename
            program_name = entry.rsplit("/", 1)[-1].replace(".xpm", "").replace(".XPM", "")

            # Optional program filter
            if args.program and args.program.lower() not in program_name.lower():
                continue

            try:
                xml_bytes = zf.read(entry)
                xml_text = xml_bytes.decode("utf-8", errors="replace")
            except Exception as exc:
                program_results.append({
                    "program": program_name,
                    "keygroup_count": 0,
                    "layer_count": 0,
                    "findings": [],
                    "parse_error": f"Could not read entry: {exc}",
                })
                continue

            result = parse_xpm(xml_text, program_name, args.warn, args.error)
            program_results.append(result)

    if not program_results:
        if args.program:
            print(f"No programs matched filter: '{args.program}'", file=sys.stderr)
        else:
            print("No programs found.", file=sys.stderr)
        return 2

    # Output
    if args.format == "json":
        print(format_json(program_results))
    else:
        print(format_text(program_results))

    # Exit codes
    total_errors = sum(count_severity(p.get("findings", []))[0] for p in program_results)
    total_warnings = sum(count_severity(p.get("findings", []))[1] for p in program_results)

    if args.strict:
        if total_errors > 0:
            return 2
        if total_warnings > 0:
            return 1
    else:
        if total_errors > 0:
            return 2

    return 0


if __name__ == "__main__":
    sys.exit(main())
