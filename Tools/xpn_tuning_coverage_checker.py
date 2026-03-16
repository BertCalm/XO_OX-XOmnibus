#!/usr/bin/env python3
"""
xpn_tuning_coverage_checker.py — XO_OX Designs
Detect accidental tuning inconsistencies across programs in a .xpn pack.

When multiple XPM programs live in the same pack they should be in tune with
each other (or deliberately detuned for a specific effect).  This tool reads
every .xpm inside a .xpn ZIP, computes a per-program median tuning offset in
cents, and flags programs that deviate from the pack-wide median.

Tuning offset (cents) = TuneCoarse * 100 + TuneFine
  TuneCoarse: semitone offset attribute on <Layer> elements  (integer)
  TuneFine:   cent offset attribute on <Layer> elements       (integer, −50…+50)

Thresholds
  WARNING  |program_median − pack_median| > --threshold-warn  (default 10 cents)
  ERROR    |program_median − pack_median| > --threshold-error (default 50 cents)

Within-program check
  If any single layer's offset differs from the program median by > 600 cents
  (half an octave), flag it as a potential note-off-root layering error.

Usage:
    python xpn_tuning_coverage_checker.py --xpn pack.xpn
    python xpn_tuning_coverage_checker.py --xpn pack.xpn --threshold-warn 5 --threshold-error 25
    python xpn_tuning_coverage_checker.py --xpn pack.xpn --format json
    python xpn_tuning_coverage_checker.py --xpn pack.xpn --reference-hz 432
"""

import argparse
import json
import sys
import zipfile
from statistics import median
from typing import Dict, List, Optional, Tuple
from xml.etree import ElementTree as ET


# ---------------------------------------------------------------------------
# Data structures
# ---------------------------------------------------------------------------

WITHIN_PROGRAM_OUTLIER_CENTS = 600  # half-octave — almost certainly a mistake


def _int_attr(elem: ET.Element, name: str, default: int = 0) -> int:
    """Return integer attribute value, falling back to *default* if missing."""
    raw = elem.get(name)
    if raw is None:
        return default
    try:
        return int(raw)
    except ValueError:
        return default


# ---------------------------------------------------------------------------
# XPM parsing
# ---------------------------------------------------------------------------

def parse_xpm_tuning(xpm_xml: str, program_name: str) -> dict:
    """
    Parse a single XPM program's XML and return a summary dict:
      {
        "program": str,
        "layers": [{"pad": str, "layer_index": int, "cents": int}, ...],
        "median_cents": float | None,
        "within_program_outliers": [...],
        "layer_count": int,
      }
    Returns a summary with empty layers if the XML is unparseable.
    """
    result: dict = {
        "program": program_name,
        "layers": [],
        "median_cents": None,
        "within_program_outliers": [],
        "layer_count": 0,
        "parse_error": None,
    }
    try:
        root = ET.fromstring(xpm_xml)
    except ET.ParseError as exc:
        result["parse_error"] = str(exc)
        return result

    # XPM structure: <MPCVObject><Program><PadList><Pad PadNumber="N"><Layer .../>
    layer_entries: List[dict] = []
    for pad in root.iter("Pad"):
        pad_num = pad.get("PadNumber", "?")
        for idx, layer in enumerate(pad.findall("Layer")):
            coarse = _int_attr(layer, "TuneCoarse", 0)
            fine = _int_attr(layer, "TuneFine", 0)
            cents = coarse * 100 + fine
            layer_entries.append({
                "pad": pad_num,
                "layer_index": idx,
                "coarse": coarse,
                "fine": fine,
                "cents": cents,
            })

    result["layers"] = layer_entries
    result["layer_count"] = len(layer_entries)

    if not layer_entries:
        return result

    all_cents = [e["cents"] for e in layer_entries]
    prog_median = median(all_cents)
    result["median_cents"] = prog_median

    # Within-program outlier check
    outliers = []
    for e in layer_entries:
        delta = abs(e["cents"] - prog_median)
        if delta > WITHIN_PROGRAM_OUTLIER_CENTS:
            outliers.append({
                "pad": e["pad"],
                "layer_index": e["layer_index"],
                "cents": e["cents"],
                "delta_from_program_median": round(delta, 2),
            })
    result["within_program_outliers"] = outliers
    return result


# ---------------------------------------------------------------------------
# Pack analysis
# ---------------------------------------------------------------------------

def analyse_pack(
    xpn_path: str,
    threshold_warn: float,
    threshold_error: float,
) -> dict:
    """
    Open a .xpn ZIP, parse every .xpm inside, compute fleet median, and
    classify programs as OK / WARNING / ERROR.

    Returns a comprehensive report dict.
    """
    programs: List[dict] = []
    parse_errors: List[str] = []

    with zipfile.ZipFile(xpn_path, "r") as zf:
        xpm_names = [n for n in zf.namelist() if n.lower().endswith(".xpm")]
        if not xpm_names:
            return {
                "xpn": xpn_path,
                "error": "No .xpm files found inside the archive.",
                "programs": [],
                "fleet_median_cents": None,
                "flagged": [],
            }

        for name in sorted(xpm_names):
            raw = zf.read(name).decode("utf-8", errors="replace")
            prog_name = name.rsplit("/", 1)[-1]  # basename only
            summary = parse_xpm_tuning(raw, prog_name)
            if summary.get("parse_error"):
                parse_errors.append(f"{prog_name}: {summary['parse_error']}")
            programs.append(summary)

    # Fleet median — built only from programs that have at least one layer
    valid_medians = [
        p["median_cents"]
        for p in programs
        if p["median_cents"] is not None
    ]
    fleet_median: Optional[float] = median(valid_medians) if valid_medians else None

    # Classify each program
    flagged: List[dict] = []
    for p in programs:
        prog_median = p["median_cents"]
        if prog_median is None or fleet_median is None:
            continue
        delta = abs(prog_median - fleet_median)
        if delta > threshold_error:
            severity = "ERROR"
        elif delta > threshold_warn:
            severity = "WARNING"
        else:
            severity = None

        if severity:
            flagged.append({
                "program": p["program"],
                "severity": severity,
                "program_median_cents": round(prog_median, 2),
                "fleet_median_cents": round(fleet_median, 2),
                "delta_cents": round(delta, 2),
            })

        # Also surface within-program outliers regardless of fleet delta
        if p["within_program_outliers"]:
            flagged.append({
                "program": p["program"],
                "severity": "WARNING",
                "note": "Within-program layer outlier(s) detected",
                "outliers": p["within_program_outliers"],
            })

    return {
        "xpn": xpn_path,
        "program_count": len(programs),
        "parse_errors": parse_errors,
        "fleet_median_cents": round(fleet_median, 2) if fleet_median is not None else None,
        "programs": [
            {
                "program": p["program"],
                "layer_count": p["layer_count"],
                "median_cents": round(p["median_cents"], 2) if p["median_cents"] is not None else None,
                "within_program_outlier_count": len(p["within_program_outliers"]),
            }
            for p in programs
        ],
        "flagged": flagged,
    }


# ---------------------------------------------------------------------------
# Output formatters
# ---------------------------------------------------------------------------

def _severity_symbol(severity: str) -> str:
    return {"ERROR": "[ERROR]  ", "WARNING": "[WARN]   "}.get(severity, "[OK]     ")


def print_text_report(report: dict, reference_hz: float) -> None:
    xpn = report["xpn"]
    print(f"\n{'='*64}")
    print(f"  XPN Tuning Coverage Checker — XO_OX Designs")
    print(f"  Pack : {xpn}")
    print(f"{'='*64}")

    if "error" in report:
        print(f"\n  [ERROR] {report['error']}")
        return

    fleet = report["fleet_median_cents"]
    print(f"\n  Programs     : {report['program_count']}")
    print(f"  Fleet median : {fleet} cents" if fleet is not None else "  Fleet median : N/A")
    print(f"\n  Note: --reference-hz ({reference_hz} Hz) sets concert pitch for")
    print(f"  context only — relative tuning checks are pitch-reference independent.\n")

    # Per-program table
    print(f"  {'Program':<40} {'Layers':>6}  {'Median (¢)':>10}  {'Outliers':>8}")
    print(f"  {'-'*40} {'------':>6}  {'----------':>10}  {'--------':>8}")
    for p in report["programs"]:
        med_str = f"{p['median_cents']:+.1f}" if p["median_cents"] is not None else "  N/A "
        print(
            f"  {p['program']:<40} {p['layer_count']:>6}  {med_str:>10}  {p['within_program_outlier_count']:>8}"
        )

    # Flagged items
    if not report["flagged"]:
        print(f"\n  All programs are within tuning tolerance. No issues found.")
    else:
        print(f"\n  Flagged ({len(report['flagged'])} issue(s)):")
        print(f"  {'-'*60}")
        for item in report["flagged"]:
            sym = _severity_symbol(item["severity"])
            if "note" in item:
                print(f"  {sym}{item['program']}")
                print(f"           {item['note']}")
                for out in item.get("outliers", []):
                    print(
                        f"           pad {out['pad']} layer {out['layer_index']}: "
                        f"{out['cents']:+d} ¢  (Δ {out['delta_from_program_median']:+.0f} ¢ from program median)"
                    )
            else:
                print(
                    f"  {sym}{item['program']}"
                    f"  |  prog {item['program_median_cents']:+.1f} ¢"
                    f"  |  fleet {item['fleet_median_cents']:+.1f} ¢"
                    f"  |  Δ {item['delta_cents']:.1f} ¢"
                )

    if report.get("parse_errors"):
        print(f"\n  Parse errors:")
        for e in report["parse_errors"]:
            print(f"    {e}")

    print(f"\n{'='*64}\n")


def print_json_report(report: dict, reference_hz: float) -> None:
    report["reference_hz"] = reference_hz
    report["note_reference_hz"] = (
        "Concert pitch reference is provided for context. "
        "Relative tuning checks are pitch-reference independent."
    )
    print(json.dumps(report, indent=2))


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        description="Detect tuning inconsistencies across XPM programs in a .xpn pack.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    p.add_argument("--xpn", required=True, metavar="FILE", help="Path to the .xpn archive.")
    p.add_argument(
        "--threshold-warn",
        type=float,
        default=10.0,
        metavar="CENTS",
        help="Deviation from fleet median that triggers a WARNING (default: 10 cents).",
    )
    p.add_argument(
        "--threshold-error",
        type=float,
        default=50.0,
        metavar="CENTS",
        help="Deviation from fleet median that triggers an ERROR (default: 50 cents).",
    )
    p.add_argument(
        "--format",
        choices=["text", "json"],
        default="text",
        help="Output format (default: text).",
    )
    p.add_argument(
        "--reference-hz",
        type=float,
        default=440.0,
        metavar="HZ",
        help="Concert pitch reference in Hz, e.g. 432 (default: 440). "
             "Printed for context; does not affect relative tuning calculations.",
    )
    return p


def main() -> int:
    args = build_parser().parse_args()

    if args.threshold_warn >= args.threshold_error:
        print(
            f"[ERROR] --threshold-warn ({args.threshold_warn}) must be less than "
            f"--threshold-error ({args.threshold_error}).",
            file=sys.stderr,
        )
        return 2

    try:
        report = analyse_pack(args.xpn, args.threshold_warn, args.threshold_error)
    except FileNotFoundError:
        print(f"[ERROR] File not found: {args.xpn}", file=sys.stderr)
        return 1
    except zipfile.BadZipFile:
        print(f"[ERROR] Not a valid ZIP/XPN archive: {args.xpn}", file=sys.stderr)
        return 1

    if args.format == "json":
        print_json_report(report, args.reference_hz)
    else:
        print_text_report(report, args.reference_hz)

    # Exit code: 1 if any ERROR, 0 otherwise (WARNs are non-fatal)
    has_error = any(f["severity"] == "ERROR" for f in report.get("flagged", []))
    return 1 if has_error else 0


if __name__ == "__main__":
    sys.exit(main())
