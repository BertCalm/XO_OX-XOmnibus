#!/usr/bin/env python3
"""
xpn_collection_arc_validator.py — XO_OX Collection Arc Validator

Validates that a set of .xpn packs forms a coherent collection arc — ensuring
tonal variety, DNA coverage, and no redundant overlap between packs in the
same collection.

Checks performed:
  1. DNA Coverage     — each of the 6 dimensions should span ≥ 0.5 range
                        WARNING if < 0.5, ERROR if < 0.3
  2. Tonal Redundancy — pair-wise DNA similarity; ERROR if all dims within 0.1,
                        WARNING if all dims within 0.2
  3. Engine Diversity — for 4+ packs, no single engine in >50% of packs (WARNING)
  4. Mood Distribution — for 4+ packs, all 7 moods should appear (INFO if missing)
  5. Pack Count       — INFO if <3, WARNING if >12

Usage:
    python xpn_collection_arc_validator.py <collection_dir> [--format text|json] [--strict]

    --strict   Exit 1 on WARNING, exit 2 on ERROR
    --format   Output format: text (default) or json
"""

import argparse
import json
import sys
import zipfile
from collections import Counter
from pathlib import Path
from typing import Any, Dict, List, Optional, Tuple


# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

DNA_DIMENSIONS = ("brightness", "warmth", "movement", "density", "space", "aggression")

VALID_MOODS = {"Foundation", "Atmosphere", "Entangled", "Prism", "Flux", "Aether", "Family"}

ERROR = "ERROR"
WARN  = "WARN"
INFO  = "INFO"

DNA_COVERAGE_ERROR_THRESHOLD   = 0.3
DNA_COVERAGE_WARNING_THRESHOLD = 0.5

REDUNDANCY_ERROR_THRESHOLD   = 0.1
REDUNDANCY_WARNING_THRESHOLD = 0.2

ENGINE_DOMINANCE_THRESHOLD = 0.50   # >50 %
PACK_COUNT_MIN = 3
PACK_COUNT_MAX = 12
ENGINE_DIVERSITY_MIN_PACKS = 4
MOOD_DIVERSITY_MIN_PACKS   = 4


# ---------------------------------------------------------------------------
# Data helpers
# ---------------------------------------------------------------------------

def _issue(severity: str, code: str, message: str) -> Dict[str, str]:
    return {"severity": severity, "code": code, "message": message}


def _read_json_from_zip(zf: zipfile.ZipFile, filename: str) -> Optional[Any]:
    """Return parsed JSON for *filename* inside *zf*, or None if missing/invalid."""
    try:
        with zf.open(filename) as fh:
            return json.load(fh)
    except (KeyError, json.JSONDecodeError):
        return None


# ---------------------------------------------------------------------------
# Per-pack extraction
# ---------------------------------------------------------------------------

def load_pack(xpn_path: Path) -> Tuple[Optional[Dict], Optional[Dict], List[str]]:
    """
    Open a .xpn ZIP and return (expansion_data, bundle_manifest_data, warnings).
    Either dict may be None if the file is absent or unparseable.
    """
    load_warnings: List[str] = []
    if not zipfile.is_zipfile(xpn_path):
        load_warnings.append(f"{xpn_path.name}: not a valid ZIP/XPN file — skipped")
        return None, None, load_warnings

    with zipfile.ZipFile(xpn_path, "r") as zf:
        expansion = _read_json_from_zip(zf, "expansion.json")
        manifest  = _read_json_from_zip(zf, "bundle_manifest.json")

    if expansion is None:
        load_warnings.append(f"{xpn_path.name}: expansion.json missing or invalid")
    if manifest is None:
        load_warnings.append(f"{xpn_path.name}: bundle_manifest.json missing or invalid")

    return expansion, manifest, load_warnings


def extract_dna(manifest: Dict) -> Optional[Dict[str, float]]:
    """
    Pull sonic_dna from bundle_manifest.json.
    Looks for a top-level 'sonic_dna' dict, falling back to checking nested
    'programs' entries for the first available sonic_dna block.
    """
    if not isinstance(manifest, dict):
        return None

    # Top-level sonic_dna
    dna = manifest.get("sonic_dna")
    if isinstance(dna, dict) and dna:
        return {k: float(v) for k, v in dna.items() if k in DNA_DIMENSIONS}

    # Nested under programs list
    programs = manifest.get("programs", [])
    if isinstance(programs, list):
        for prog in programs:
            if isinstance(prog, dict):
                dna = prog.get("sonic_dna")
                if isinstance(dna, dict) and dna:
                    return {k: float(v) for k, v in dna.items() if k in DNA_DIMENSIONS}

    return None


def extract_engines(expansion: Dict) -> List[str]:
    """Return list of engine names referenced in expansion.json."""
    if not isinstance(expansion, dict):
        return []
    engines = expansion.get("engines", [])
    if isinstance(engines, list):
        return [str(e).upper() for e in engines if e]
    # Some packs use a single 'engine' field
    single = expansion.get("engine")
    if single:
        return [str(single).upper()]
    return []


def extract_mood(expansion: Dict) -> Optional[str]:
    if not isinstance(expansion, dict):
        return None
    mood = expansion.get("mood")
    return str(mood) if mood else None


# ---------------------------------------------------------------------------
# Validation checks
# ---------------------------------------------------------------------------

def check_pack_count(n: int) -> List[Dict[str, str]]:
    issues = []
    if n < PACK_COUNT_MIN:
        issues.append(_issue(INFO, "PACK_COUNT_LOW",
            f"Collection has only {n} pack(s); recommend ≥{PACK_COUNT_MIN} for a coherent arc"))
    elif n > PACK_COUNT_MAX:
        issues.append(_issue(WARN, "PACK_COUNT_HIGH",
            f"Collection has {n} packs; recommend ≤{PACK_COUNT_MAX} to avoid redundancy"))
    return issues


def check_dna_coverage(dna_map: Dict[str, List[float]]) -> List[Dict[str, str]]:
    """
    dna_map: dimension → list of values (one per pack that has DNA data).
    Returns issues and a per-dimension summary dict for display.
    """
    issues = []
    summary = {}  # dimension → (range_value, status_char)

    for dim in DNA_DIMENSIONS:
        values = dna_map.get(dim, [])
        if not values:
            summary[dim] = (None, "?")
            issues.append(_issue(WARN, "DNA_MISSING_DIM",
                f"Dimension '{dim}' has no data across packs"))
            continue

        span = max(values) - min(values)
        if span < DNA_COVERAGE_ERROR_THRESHOLD:
            status = "x"
            issues.append(_issue(ERROR, "DNA_COVERAGE_LOW",
                f"Dimension '{dim}' range {span:.2f} < {DNA_COVERAGE_ERROR_THRESHOLD} — ERROR"))
        elif span < DNA_COVERAGE_WARNING_THRESHOLD:
            status = "~"
            issues.append(_issue(WARN, "DNA_COVERAGE_NARROW",
                f"Dimension '{dim}' range {span:.2f} < {DNA_COVERAGE_WARNING_THRESHOLD} — WARNING"))
        else:
            status = "v"

        summary[dim] = (span, status)

    return issues, summary


def check_redundancy(
    pack_names: List[str],
    dna_list: List[Optional[Dict[str, float]]],
) -> List[Dict[str, str]]:
    issues = []
    n = len(pack_names)
    for i in range(n):
        for j in range(i + 1, n):
            dna_a = dna_list[i]
            dna_b = dna_list[j]
            if dna_a is None or dna_b is None:
                continue
            dims = [d for d in DNA_DIMENSIONS if d in dna_a and d in dna_b]
            if not dims:
                continue
            diffs = [abs(dna_a[d] - dna_b[d]) for d in dims]
            max_diff = max(diffs)
            # Similarity = 1 - normalised max_diff
            similarity = 1.0 - max_diff

            if max_diff <= REDUNDANCY_ERROR_THRESHOLD:
                issues.append(_issue(ERROR, "PACK_NEAR_DUPLICATE",
                    f"'{pack_names[i]}' / '{pack_names[j]}' max dimension diff {max_diff:.3f} "
                    f"(≤{REDUNDANCY_ERROR_THRESHOLD}) — near-duplicate packs"))
            elif max_diff <= REDUNDANCY_WARNING_THRESHOLD:
                issues.append(_issue(WARN, "PACK_HIGH_SIMILARITY",
                    f"'{pack_names[i]}' / '{pack_names[j]}' max dimension diff {max_diff:.3f} "
                    f"(≤{REDUNDANCY_WARNING_THRESHOLD}) — high overlap"))

    return issues


def check_engine_diversity(
    pack_names: List[str],
    engine_lists: List[List[str]],
) -> List[Dict[str, str]]:
    issues = []
    n = len(pack_names)
    if n < ENGINE_DIVERSITY_MIN_PACKS:
        return issues

    engine_counter: Counter = Counter()
    for engines in engine_lists:
        for e in set(engines):  # count each engine once per pack
            engine_counter[e] += 1

    for engine, count in engine_counter.items():
        ratio = count / n
        if ratio > ENGINE_DOMINANCE_THRESHOLD:
            issues.append(_issue(WARN, "ENGINE_DOMINANCE",
                f"'{engine}' appears in {count}/{n} packs ({ratio*100:.0f}%) — exceeds 50%"))

    return issues


def check_mood_distribution(moods: List[Optional[str]], n: int) -> List[Dict[str, str]]:
    issues = []
    if n < MOOD_DIVERSITY_MIN_PACKS:
        return issues

    present = {m for m in moods if m in VALID_MOODS}
    missing = VALID_MOODS - present
    if missing:
        issues.append(_issue(INFO, "MOOD_MISSING",
            f"Missing moods: {', '.join(sorted(missing))}"))

    return issues


# ---------------------------------------------------------------------------
# Output formatting
# ---------------------------------------------------------------------------

def _status_char(issues: List[Dict]) -> str:
    severities = {i["severity"] for i in issues}
    if ERROR in severities:
        return "ERROR"
    if WARN in severities:
        return "WARN"
    return "OK"


def format_text(
    collection_dir: str,
    packs: List[str],
    dna_summary: Dict,
    all_issues: List[Dict],
) -> str:
    lines = []
    n = len(packs)
    lines.append(f"COLLECTION: {collection_dir} ({n} pack{'s' if n != 1 else ''})")

    # DNA coverage line
    dna_parts = []
    for dim in DNA_DIMENSIONS:
        entry = dna_summary.get(dim)
        if entry is None:
            dna_parts.append(f"{dim}=? ?")
        else:
            span, status = entry
            if span is None:
                dna_parts.append(f"{dim}=? ?")
            else:
                tick = "v" if status == "v" else ("~" if status == "~" else "x")
                dna_parts.append(f"{dim}={span:.2f} {tick}")
    lines.append(f"  DNA COVERAGE: {', '.join(dna_parts)}")

    # Group and print issues by code
    for issue_dict in all_issues:
        sev = issue_dict["severity"]
        msg = issue_dict["message"]
        lines.append(f"  {sev}: {msg}")

    # Verdict
    errors   = sum(1 for i in all_issues if i["severity"] == ERROR)
    warnings = sum(1 for i in all_issues if i["severity"] == WARN)
    infos    = sum(1 for i in all_issues if i["severity"] == INFO)

    overall = "OK"
    if errors:
        overall = "ERROR"
    elif warnings:
        overall = "WARN"

    lines.append(
        f"VERDICT: {overall} ({errors} error{'s' if errors != 1 else ''}, "
        f"{warnings} warning{'s' if warnings != 1 else ''}, "
        f"{infos} info)"
    )
    return "\n".join(lines)


def format_json(
    collection_dir: str,
    packs: List[str],
    dna_summary: Dict,
    all_issues: List[Dict],
) -> str:
    errors   = sum(1 for i in all_issues if i["severity"] == ERROR)
    warnings = sum(1 for i in all_issues if i["severity"] == WARN)
    infos    = sum(1 for i in all_issues if i["severity"] == INFO)
    overall  = "ERROR" if errors else ("WARN" if warnings else "OK")

    dna_out = {}
    for dim in DNA_DIMENSIONS:
        entry = dna_summary.get(dim)
        if entry and entry[0] is not None:
            dna_out[dim] = {"range": round(entry[0], 4), "status": entry[1]}
        else:
            dna_out[dim] = {"range": None, "status": "?"}

    result = {
        "collection": str(collection_dir),
        "pack_count": len(packs),
        "packs": packs,
        "dna_coverage": dna_out,
        "issues": all_issues,
        "verdict": {
            "overall": overall,
            "errors": errors,
            "warnings": warnings,
            "info": infos,
        },
    }
    return json.dumps(result, indent=2)


# ---------------------------------------------------------------------------
# Main entry point
# ---------------------------------------------------------------------------

def validate_collection(collection_dir: Path) -> Tuple[List[str], Dict, List[Dict]]:
    """
    Run all checks on a directory of .xpn files.
    Returns (pack_names, dna_summary, all_issues).
    """
    xpn_files = sorted(collection_dir.glob("*.xpn"))
    all_issues: List[Dict] = []

    pack_names:    List[str]                    = []
    dna_list:      List[Optional[Dict[str, float]]] = []
    engine_lists:  List[List[str]]              = []
    moods:         List[Optional[str]]          = []

    # Load each pack
    for xpn_path in xpn_files:
        expansion, manifest, load_warnings = load_pack(xpn_path)
        for w in load_warnings:
            all_issues.append(_issue(WARN, "LOAD_WARNING", w))

        if expansion is None and manifest is None:
            continue

        pack_names.append(xpn_path.stem)
        dna_list.append(extract_dna(manifest) if manifest else None)
        engine_lists.append(extract_engines(expansion) if expansion else [])
        moods.append(extract_mood(expansion) if expansion else None)

    n = len(pack_names)

    # 1. Pack count
    all_issues.extend(check_pack_count(n))

    # 2. DNA coverage
    dna_map: Dict[str, List[float]] = {dim: [] for dim in DNA_DIMENSIONS}
    for dna in dna_list:
        if dna:
            for dim in DNA_DIMENSIONS:
                if dim in dna:
                    dna_map[dim].append(dna[dim])

    coverage_issues, dna_summary = check_dna_coverage(dna_map)
    all_issues.extend(coverage_issues)

    # 3. Tonal redundancy
    all_issues.extend(check_redundancy(pack_names, dna_list))

    # 4. Engine diversity
    all_issues.extend(check_engine_diversity(pack_names, engine_lists))

    # 5. Mood distribution
    all_issues.extend(check_mood_distribution(moods, n))

    return pack_names, dna_summary, all_issues


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Validate that a directory of .xpn packs forms a coherent collection arc."
    )
    parser.add_argument("collection_dir", help="Directory containing .xpn files")
    parser.add_argument(
        "--format",
        choices=["text", "json"],
        default="text",
        help="Output format (default: text)",
    )
    parser.add_argument(
        "--strict",
        action="store_true",
        help="Exit 1 on WARNING, exit 2 on ERROR (default: only exit 2 on ERROR)",
    )
    args = parser.parse_args()

    collection_dir = Path(args.collection_dir)
    if not collection_dir.is_dir():
        print(f"ERROR: '{collection_dir}' is not a directory", file=sys.stderr)
        return 2

    pack_names, dna_summary, all_issues = validate_collection(collection_dir)

    errors   = sum(1 for i in all_issues if i["severity"] == ERROR)
    warnings = sum(1 for i in all_issues if i["severity"] == WARN)

    if args.format == "json":
        print(format_json(str(collection_dir), pack_names, dna_summary, all_issues))
    else:
        print(format_text(str(collection_dir), pack_names, dna_summary, all_issues))

    # Exit codes
    if errors:
        return 2
    if args.strict and warnings:
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
