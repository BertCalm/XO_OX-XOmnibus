#!/usr/bin/env python3
"""
xpn_macro_coverage_checker.py — XO_OX Designs
Audit macro coverage in .xometa preset files.

Checks each preset for:
  1. Macro presence   — M1–M4 all defined
  2. Macro naming     — labels are non-empty and not bare "M1"/"M2"/etc.
  3. Macro range      — each macro's min/max span ≥ 0.2 (via macroTargets)
  4. Macro routing    — each macro routes to ≥ 1 parameter (via macroTargets)
  5. D006 expression  — at least one aftertouch or mod-wheel entry in modMatrix

Supported .xometa key variants
  • macroLabels: ["FILTER", "GLIDE", "VIBE", "SPACE"]   (array, index 0→M1)
  • macros: {"M1": "FILTER", "M2": "GLIDE", ...}        (object)
  • macroTargets: [{macro: 1, parameterId: ..., min, max}, ...]
  • modMatrix:    [{source: "aftertouch"|"modWheel", destination: ..., amount}, ...]

Scoring (per preset, 100 pts max)
  All 4 macros present  : +25
  All 4 macros labeled  : +15
  All macros range ≥0.2 : +20   (skip if macroTargets absent — not penalised)
  All macros routed     : +20   (skip if macroTargets absent — not penalised)
  D006 expression       : +20

Grades
  PASS : 92–100
  WARN : 70–91
  FAIL : <70

CLI
  python xpn_macro_coverage_checker.py <presets_dir> [options]

  --engine FILTER   only audit presets whose engines/engine field contains FILTER
  --format text|json
  --strict          exit 1 if fleet avg < 80, exit 2 if any FAIL
"""

import argparse
import glob
import json
import os
import sys
from typing import Any, Dict, List, Optional, Tuple


# ---------------------------------------------------------------------------
# Parsing helpers
# ---------------------------------------------------------------------------

BARE_LABELS = {"M1", "M2", "M3", "M4", "Macro 1", "Macro 2", "Macro 3", "Macro 4", ""}
MIN_RANGE = 0.2


def _extract_macro_labels(data: dict) -> Dict[str, Optional[str]]:
    """Return {M1: label, M2: label, M3: label, M4: label} — None if absent."""
    labels: Dict[str, Optional[str]] = {f"M{i}": None for i in range(1, 5)}

    # Format A: macros object {"M1": "LABEL", ...}
    if isinstance(data.get("macros"), dict):
        for i in range(1, 5):
            key = f"M{i}"
            val = data["macros"].get(key)
            if val is not None:
                labels[key] = str(val).strip()
        return labels

    # Format B: macroLabels array ["LABEL1", "LABEL2", ...]
    if isinstance(data.get("macroLabels"), list):
        for idx, val in enumerate(data["macroLabels"][:4]):
            key = f"M{idx + 1}"
            labels[key] = str(val).strip() if val is not None else None
        return labels

    return labels


def _extract_macro_targets(data: dict) -> Dict[str, List[dict]]:
    """Return {M1: [{min, max, param}, ...], ...} from macroTargets list."""
    result: Dict[str, List[dict]] = {f"M{i}": [] for i in range(1, 5)}
    targets = data.get("macroTargets")
    if not isinstance(targets, list):
        return result
    for entry in targets:
        if not isinstance(entry, dict):
            continue
        macro_num = entry.get("macro")
        if macro_num is None:
            continue
        key = f"M{macro_num}"
        if key not in result:
            continue
        result[key].append(entry)
    return result


def _macro_range_span(targets: List[dict]) -> Optional[float]:
    """Return the maximum (max-min) span across all targets for a macro, or None."""
    if not targets:
        return None
    spans = []
    for t in targets:
        mn = t.get("min")
        mx = t.get("max")
        if mn is not None and mx is not None:
            try:
                span = abs(float(mx) - float(mn))
                spans.append(span)
            except (TypeError, ValueError):
                pass
    if not spans:
        return None
    return max(spans)


def _has_expression(data: dict) -> bool:
    """True if modMatrix contains any aftertouch or modWheel source entry."""
    matrix = data.get("modMatrix")
    if not isinstance(matrix, list):
        return False
    expression_sources = {"aftertouch", "modWheel", "expression", "mod_wheel", "cc1", "cc74"}
    for entry in matrix:
        if not isinstance(entry, dict):
            continue
        src = str(entry.get("source", "")).lower()
        if src in expression_sources:
            return True
    return False


def _engine_names(data: dict) -> List[str]:
    """Return list of engine name strings from the preset."""
    engines = data.get("engines")
    if isinstance(engines, list):
        return [str(e) for e in engines]
    engine = data.get("engine")
    if engine:
        return [str(engine)]
    return []


# ---------------------------------------------------------------------------
# Audit one preset
# ---------------------------------------------------------------------------

def audit_preset(path: str) -> dict:
    """
    Audit a single .xometa file.

    Returns a dict:
      {
        "path": str,
        "name": str,
        "engines": [str],
        "score": int,
        "grade": "PASS"|"WARN"|"FAIL",
        "issues": [str],
        "details": {
          "macros_present": bool,
          "macros_labeled": bool,
          "macros_ranged": bool|None,    # None = not checkable (no macroTargets)
          "macros_routed": bool|None,    # None = not checkable
          "expression_present": bool,
        },
        "parse_error": str|None,
      }
    """
    result: dict = {
        "path": path,
        "name": os.path.basename(path),
        "engines": [],
        "score": 0,
        "grade": "FAIL",
        "issues": [],
        "details": {
            "macros_present": False,
            "macros_labeled": False,
            "macros_ranged": None,
            "macros_routed": None,
            "expression_present": False,
        },
        "parse_error": None,
    }

    try:
        with open(path, "r", encoding="utf-8") as fh:
            data = json.load(fh)
    except Exception as exc:
        result["parse_error"] = str(exc)
        result["issues"].append(f"Parse error: {exc}")
        return result

    result["name"] = data.get("name", os.path.basename(path))
    result["engines"] = _engine_names(data)

    labels = _extract_macro_labels(data)
    targets = _extract_macro_targets(data)
    has_any_targets = any(targets[f"M{i}"] for i in range(1, 5))

    score = 0
    issues: List[str] = []

    # --- Check 1: All 4 macros present (+25) ---
    present = [labels[f"M{i}"] is not None for i in range(1, 5)]
    all_present = all(present)
    result["details"]["macros_present"] = all_present
    if all_present:
        score += 25
    else:
        missing = [f"M{i}" for i in range(1, 5) if not present[i - 1]]
        issues.append(f"Missing macros: {', '.join(missing)}")

    # --- Check 2: All macros labeled (+15) ---
    labeled = [
        labels[f"M{i}"] is not None and labels[f"M{i}"] not in BARE_LABELS
        for i in range(1, 5)
    ]
    all_labeled = all(labeled)
    result["details"]["macros_labeled"] = all_labeled
    if all_labeled:
        score += 15
    else:
        unlabeled = [
            f"M{i} ({labels[f'M{i}']!r})"
            for i in range(1, 5)
            if not labeled[i - 1]
        ]
        issues.append(f"Bare/missing labels: {', '.join(unlabeled)}")

    # --- Check 3: Macro range ≥ 0.2 (+20) ---
    if has_any_targets:
        range_ok: List[bool] = []
        for i in range(1, 5):
            key = f"M{i}"
            span = _macro_range_span(targets[key])
            if span is None:
                # macro has no targets — range not applicable for this macro
                range_ok.append(True)
            else:
                range_ok.append(span >= MIN_RANGE)
        all_ranged = all(range_ok)
        result["details"]["macros_ranged"] = all_ranged
        if all_ranged:
            score += 20
        else:
            dead = []
            for i in range(1, 5):
                key = f"M{i}"
                span = _macro_range_span(targets[key])
                if span is not None and span < MIN_RANGE:
                    dead.append(f"{key} range {span:.2f} (dead macro)")
            issues.extend(dead)
    # else: macroTargets absent — skip range check entirely (no penalty)

    # --- Check 4: All macros routed (+20) ---
    if has_any_targets:
        routed = [bool(targets[f"M{i}"]) for i in range(1, 5)]
        all_routed = all(routed)
        result["details"]["macros_routed"] = all_routed
        if all_routed:
            score += 20
        else:
            unrouted = [f"M{i}" for i in range(1, 5) if not routed[i - 1]]
            for m in unrouted:
                issues.append(f"{m} has no routing")
    # else: macroTargets absent — skip routing check (no penalty)

    # --- Check 5: D006 expression (+20) ---
    has_expr = _has_expression(data)
    result["details"]["expression_present"] = has_expr
    if has_expr:
        score += 20
    else:
        issues.append("No expression assignment (D006: aftertouch / mod wheel missing)")

    # --- Adjust score when routing/range checks were skipped ---
    # If macroTargets absent, max possible = 25 + 15 + 20 = 60 (without routing/range)
    # Rescale to 100 so these presets aren't unfairly penalised
    if not has_any_targets:
        max_possible = 60
        if max_possible > 0:
            score = round((score / max_possible) * 100)

    result["score"] = min(score, 100)
    result["grade"] = "PASS" if score >= 92 else ("WARN" if score >= 70 else "FAIL")
    result["issues"] = issues
    return result


# ---------------------------------------------------------------------------
# Fleet audit
# ---------------------------------------------------------------------------

def audit_directory(
    presets_dir: str,
    engine_filter: Optional[str] = None,
) -> List[dict]:
    """Collect and audit all .xometa files under presets_dir."""
    pattern = os.path.join(presets_dir, "**", "*.xometa")
    paths = sorted(glob.glob(pattern, recursive=True))

    if not paths:
        print(f"WARNING: No .xometa files found under {presets_dir}", file=sys.stderr)
        return []

    results = []
    for path in paths:
        audit = audit_preset(path)
        if engine_filter:
            if not any(engine_filter.lower() in e.lower() for e in audit["engines"]):
                continue
        results.append(audit)

    return results


# ---------------------------------------------------------------------------
# Output formatters
# ---------------------------------------------------------------------------

def _rel_path(path: str, base: str) -> str:
    try:
        return os.path.relpath(path, base)
    except ValueError:
        return path


def format_text(results: List[dict], presets_dir: str) -> str:
    if not results:
        return "No presets audited."

    total = len(results)
    passes = [r for r in results if r["grade"] == "PASS"]
    warns = [r for r in results if r["grade"] == "WARN"]
    fails = [r for r in results if r["grade"] == "FAIL"]
    avg_score = sum(r["score"] for r in results) / total
    d006_count = sum(1 for r in results if r["details"]["expression_present"])

    # Determine label for header
    dir_label = os.path.basename(presets_dir.rstrip("/\\")) or presets_dir
    lines: List[str] = [
        f"MACRO COVERAGE AUDIT — {dir_label} ({total} files)",
        "",
        f"PASS (92+ pts): {len(passes):3d} presets",
        f"WARN (70-91):   {len(warns):3d} presets",
        f"FAIL (<70):     {len(fails):3d} presets",
    ]

    if fails:
        lines.append("")
        lines.append("FAILS:")
        for r in sorted(fails, key=lambda x: x["score"]):
            rel = _rel_path(r["path"], presets_dir)
            lines.append(f"  {rel} — {r['score']} pts")
            for issue in r["issues"]:
                lines.append(f"    \u2717 {issue}")

    if warns:
        lines.append("")
        lines.append("WARNINGS:")
        for r in sorted(warns, key=lambda x: x["score"]):
            rel = _rel_path(r["path"], presets_dir)
            lines.append(f"  {rel} — {r['score']} pts")
            for issue in r["issues"]:
                lines.append(f"    \u2022 {issue}")

    lines.append("")
    lines.append(f"Fleet score: {avg_score:.0f}/100 avg")
    lines.append(f"D006 coverage: {d006_count}/{total} ({100 * d006_count // total}%)")

    return "\n".join(lines)


def format_json(results: List[dict], presets_dir: str) -> str:
    total = len(results)
    avg_score = sum(r["score"] for r in results) / total if total else 0
    d006_count = sum(1 for r in results if r["details"]["expression_present"])
    output = {
        "summary": {
            "total": total,
            "pass": sum(1 for r in results if r["grade"] == "PASS"),
            "warn": sum(1 for r in results if r["grade"] == "WARN"),
            "fail": sum(1 for r in results if r["grade"] == "FAIL"),
            "fleet_avg": round(avg_score, 1),
            "d006_count": d006_count,
            "d006_pct": round(100 * d006_count / total, 1) if total else 0,
        },
        "presets": [
            {
                "path": _rel_path(r["path"], presets_dir),
                "name": r["name"],
                "engines": r["engines"],
                "score": r["score"],
                "grade": r["grade"],
                "issues": r["issues"],
                "details": r["details"],
                "parse_error": r["parse_error"],
            }
            for r in results
        ],
    }
    return json.dumps(output, indent=2)


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        prog="xpn_macro_coverage_checker",
        description="Audit macro coverage in .xometa preset files.",
    )
    p.add_argument("presets_dir", help="Directory containing .xometa files (searched recursively)")
    p.add_argument(
        "--engine",
        metavar="FILTER",
        default=None,
        help="Only audit presets whose engine field contains this string (case-insensitive)",
    )
    p.add_argument(
        "--format",
        choices=["text", "json"],
        default="text",
        help="Output format (default: text)",
    )
    p.add_argument(
        "--strict",
        action="store_true",
        help="Exit 1 if fleet avg < 80; exit 2 if any preset is FAIL",
    )
    return p


def main() -> None:
    parser = build_parser()
    args = parser.parse_args()

    if not os.path.isdir(args.presets_dir):
        print(f"ERROR: Not a directory: {args.presets_dir}", file=sys.stderr)
        sys.exit(3)

    results = audit_directory(args.presets_dir, engine_filter=args.engine)

    if not results:
        print("No matching presets found.")
        sys.exit(0)

    if args.format == "json":
        print(format_json(results, args.presets_dir))
    else:
        print(format_text(results, args.presets_dir))

    if args.strict:
        total = len(results)
        avg_score = sum(r["score"] for r in results) / total
        has_fails = any(r["grade"] == "FAIL" for r in results)
        if has_fails:
            sys.exit(2)
        if avg_score < 80:
            sys.exit(1)


if __name__ == "__main__":
    main()
