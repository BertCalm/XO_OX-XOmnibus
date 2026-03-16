"""
xpn_manifest_validator.py — XO_OX Expansion Pack Manifest Validator

Validates expansion.json and bundle_manifest.json inside a .xpn ZIP archive
or as standalone JSON files. Checks required fields, value constraints, and
cross-file consistency.

Usage:
    python xpn_manifest_validator.py --xpn pack.xpn [--strict] [--format text|json]
    python xpn_manifest_validator.py --expansion-json expansion.json --bundle-manifest bundle_manifest.json
"""

import argparse
import json
import re
import sys
import zipfile
from typing import Any, List, Optional, Tuple

# ---------------------------------------------------------------------------
# Canonical engine list — 38 XO_OX engines
# ---------------------------------------------------------------------------
CANONICAL_ENGINES = {
    "OBLONG", "OVERBITE", "OVERDUB", "ODYSSEY", "ONSET", "OPAL",
    "OVERWORLD", "OHM", "ORPHICA", "OBBLIGATO", "OTTONI", "OLE",
    "OVERLAP", "OUTWIT", "OSTINATO", "OPENSKY", "OCEANDEEP", "OUIE",
    "OVERTONE", "KNOT", "ORGANISM", "ORIGIN", "ORBIT", "ORACLE",
    "OBSIDIAN", "ORCA", "OUTRUN", "OXIDE", "ORPHAN", "OFFSET",
    "OHMIC", "ORNATE", "OMNI", "OPTICAL", "OUTPOST", "ONYX",
    "ONWARD", "OUTLAW",
}

VALID_MOODS = {"Foundation", "Atmosphere", "Entangled", "Prism", "Flux", "Aether", "Family"}

VALID_KEYS = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"}

SONIC_DNA_KEYS = {"brightness", "warmth", "movement", "density", "space", "aggression"}

SEMVER_RE = re.compile(r"^\d+\.\d+(\.\d+)?$")

# ---------------------------------------------------------------------------
# Issue dataclass (plain dict for stdlib-only)
# ---------------------------------------------------------------------------

def issue(field: str, severity: str, message: str) -> dict:
    return {"field": field, "severity": severity, "message": message}


# ---------------------------------------------------------------------------
# Validators
# ---------------------------------------------------------------------------

def validate_expansion(data: Any) -> List[dict]:
    issues = []
    if not isinstance(data, dict):
        issues.append(issue("expansion.json", "ERROR", "Root must be a JSON object"))
        return issues

    # Required fields
    for field in ("name", "version", "manufacturer", "description"):
        if field not in data:
            issues.append(issue(field, "ERROR", f"Required field '{field}' is missing"))

    # name constraints
    name = data.get("name")
    if isinstance(name, str):
        if len(name) > 50:
            issues.append(issue("name", "ERROR",
                f"'name' exceeds 50 characters (got {len(name)})"))
    elif name is not None:
        issues.append(issue("name", "ERROR", "'name' must be a string"))

    # manufacturer must be exact string
    manufacturer = data.get("manufacturer")
    if manufacturer is not None and manufacturer != "XO_OX Designs":
        issues.append(issue("manufacturer", "ERROR",
            f"'manufacturer' must be 'XO_OX Designs', got '{manufacturer}'"))

    # version semver
    version = data.get("version")
    if isinstance(version, str):
        if not SEMVER_RE.match(version):
            issues.append(issue("version", "ERROR",
                f"'version' must match semver pattern (e.g. 1.0 or 1.0.0), got '{version}'"))
    elif version is not None:
        issues.append(issue("version", "ERROR", "'version' must be a string"))

    # description non-empty
    description = data.get("description")
    if isinstance(description, str) and not description.strip():
        issues.append(issue("description", "WARNING", "'description' is present but empty"))

    # Optional — warn if absent
    for field in ("icon", "category"):
        if field not in data:
            issues.append(issue(field, "WARNING", f"Optional field '{field}' is absent"))

    return issues


def validate_bundle_manifest(data: Any) -> List[dict]:
    issues = []
    if not isinstance(data, dict):
        issues.append(issue("bundle_manifest.json", "ERROR", "Root must be a JSON object"))
        return issues

    # Required fields
    for field in ("pack_name", "engine", "mood", "version", "author"):
        if field not in data:
            issues.append(issue(field, "ERROR", f"Required field '{field}' is missing"))

    # engine must be in canonical list
    engine = data.get("engine")
    if isinstance(engine, str):
        if engine.upper() not in CANONICAL_ENGINES:
            issues.append(issue("engine", "ERROR",
                f"'engine' '{engine}' is not in the canonical XO_OX engine list"))
    elif engine is not None:
        issues.append(issue("engine", "ERROR", "'engine' must be a string"))

    # mood
    mood = data.get("mood")
    if isinstance(mood, str):
        if mood not in VALID_MOODS:
            issues.append(issue("mood", "ERROR",
                f"'mood' '{mood}' must be one of: {', '.join(sorted(VALID_MOODS))}"))
    elif mood is not None:
        issues.append(issue("mood", "ERROR", "'mood' must be a string"))

    # version semver
    version = data.get("version")
    if isinstance(version, str):
        if not SEMVER_RE.match(version):
            issues.append(issue("version", "ERROR",
                f"'version' must match semver pattern, got '{version}'"))
    elif version is not None:
        issues.append(issue("version", "ERROR", "'version' must be a string"))

    # sonic_dna — optional, validate if present
    sonic_dna = data.get("sonic_dna")
    if sonic_dna is not None:
        if not isinstance(sonic_dna, dict):
            issues.append(issue("sonic_dna", "ERROR", "'sonic_dna' must be a JSON object"))
        else:
            missing_keys = SONIC_DNA_KEYS - set(sonic_dna.keys())
            for k in sorted(missing_keys):
                issues.append(issue(f"sonic_dna.{k}", "ERROR",
                    f"'sonic_dna' is missing required key '{k}'"))
            for k, v in sonic_dna.items():
                if k in SONIC_DNA_KEYS:
                    if not isinstance(v, (int, float)):
                        issues.append(issue(f"sonic_dna.{k}", "ERROR",
                            f"'sonic_dna.{k}' must be a number, got {type(v).__name__}"))
                    elif not (0.0 <= float(v) <= 1.0):
                        issues.append(issue(f"sonic_dna.{k}", "ERROR",
                            f"'sonic_dna.{k}' must be 0.0-1.0, got {v}"))

    # bpm — optional
    bpm = data.get("bpm")
    if bpm is not None:
        if not isinstance(bpm, (int, float)):
            issues.append(issue("bpm", "ERROR", "'bpm' must be a number"))
        elif not (60 <= float(bpm) <= 300):
            issues.append(issue("bpm", "ERROR",
                f"'bpm' must be 60-300, got {bpm}"))

    # key — optional
    key = data.get("key")
    if key is not None:
        if not isinstance(key, str) or key not in VALID_KEYS:
            issues.append(issue("key", "ERROR",
                f"'key' must be one of {', '.join(sorted(VALID_KEYS))}, got '{key}'"))

    # preset_count — warn if absent
    if "preset_count" not in data:
        issues.append(issue("preset_count", "WARNING",
            "'preset_count' is absent (useful for analytics)"))

    return issues


def cross_validate(exp_data: Any, bm_data: Any) -> List[dict]:
    issues = []
    if not isinstance(exp_data, dict) or not isinstance(bm_data, dict):
        return issues
    exp_name = str(exp_data.get("name", "")).upper()
    bm_engine = str(bm_data.get("engine", "")).upper()
    if exp_name and bm_engine and bm_engine not in exp_name and exp_name not in bm_engine:
        issues.append(issue("engine/name", "WARNING",
            f"bundle_manifest 'engine' ('{bm_engine}') is not contained in "
            f"expansion.json 'name' ('{exp_name}') — verify they belong to the same pack"))
    return issues


# ---------------------------------------------------------------------------
# Load helpers
# ---------------------------------------------------------------------------

def load_json_from_zip(zip_path: str, filename: str) -> Tuple[Optional[Any], Optional[str]]:
    """Return (parsed_data, error_string). error_string is None on success."""
    try:
        with zipfile.ZipFile(zip_path, "r") as zf:
            names = zf.namelist()
            matches = [n for n in names if n == filename or n.endswith(f"/{filename}")]
            if not matches:
                return None, f"'{filename}' not found in archive"
            with zf.open(matches[0]) as f:
                return json.load(f), None
    except zipfile.BadZipFile:
        return None, f"'{zip_path}' is not a valid ZIP/XPN file"
    except json.JSONDecodeError as e:
        return None, f"JSON parse error in '{filename}': {e}"


def load_json_from_file(path: str) -> Tuple[Optional[Any], Optional[str]]:
    try:
        with open(path, "r", encoding="utf-8") as f:
            return json.load(f), None
    except FileNotFoundError:
        return None, f"File not found: '{path}'"
    except json.JSONDecodeError as e:
        return None, f"JSON parse error in '{path}': {e}"


# ---------------------------------------------------------------------------
# Output formatters
# ---------------------------------------------------------------------------

def format_text(all_issues: List[dict], passed: bool) -> str:
    lines = []
    for iss in all_issues:
        lines.append(f"  [{iss['severity']}] {iss['field']}: {iss['message']}")
    if not lines:
        lines.append("  (no issues found)")
    lines.append("")
    lines.append("RESULT: PASS" if passed else "RESULT: FAIL")
    return "\n".join(lines)


def format_json(all_issues: List[dict], passed: bool) -> str:
    return json.dumps({"passed": passed, "issues": all_issues}, indent=2)


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main() -> int:
    parser = argparse.ArgumentParser(
        description="Validate XO_OX expansion.json and bundle_manifest.json")
    src = parser.add_mutually_exclusive_group(required=True)
    src.add_argument("--xpn", metavar="PACK.XPN",
        help="Path to a .xpn ZIP archive")
    src.add_argument("--expansion-json", metavar="FILE",
        help="Path to a standalone expansion.json file")
    parser.add_argument("--bundle-manifest", metavar="FILE",
        help="Path to a standalone bundle_manifest.json (use with --expansion-json)")
    parser.add_argument("--strict", action="store_true",
        help="Treat WARNINGs as failures")
    parser.add_argument("--format", choices=["text", "json"], default="text",
        help="Output format (default: text)")
    args = parser.parse_args()

    all_issues: List[dict] = []
    exp_data = None
    bm_data = None

    if args.xpn:
        exp_data, err = load_json_from_zip(args.xpn, "expansion.json")
        if err:
            all_issues.append(issue("expansion.json", "ERROR", err))
        else:
            all_issues.extend(validate_expansion(exp_data))

        bm_data, err = load_json_from_zip(args.xpn, "bundle_manifest.json")
        if err:
            all_issues.append(issue("bundle_manifest.json", "ERROR", err))
        else:
            all_issues.extend(validate_bundle_manifest(bm_data))
    else:
        exp_data, err = load_json_from_file(args.expansion_json)
        if err:
            all_issues.append(issue("expansion.json", "ERROR", err))
        else:
            all_issues.extend(validate_expansion(exp_data))

        if args.bundle_manifest:
            bm_data, err = load_json_from_file(args.bundle_manifest)
            if err:
                all_issues.append(issue("bundle_manifest.json", "ERROR", err))
            else:
                all_issues.extend(validate_bundle_manifest(bm_data))

    if exp_data is not None and bm_data is not None:
        all_issues.extend(cross_validate(exp_data, bm_data))

    has_errors = any(i["severity"] == "ERROR" for i in all_issues)
    has_warnings = any(i["severity"] == "WARNING" for i in all_issues)
    passed = not has_errors and not (args.strict and has_warnings)

    output = format_json(all_issues, passed) if args.format == "json" else format_text(all_issues, passed)
    print(output)
    return 0 if passed else 1


if __name__ == "__main__":
    sys.exit(main())
