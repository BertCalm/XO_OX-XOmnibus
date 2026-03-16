#!/usr/bin/env python3
"""
xpn_xometa_batch_validator.py
Validates all .xometa preset files in a directory against the XOmnibus preset schema.

Usage:
    python xpn_xometa_batch_validator.py <dir> [--strict] [--fix] [--format text|json] [--output report.txt]

Exit codes:
    0 = all pass
    1 = warnings only (no failures)
    2 = any failures
"""

import argparse
import json
import re
import sys
from pathlib import Path

# ── Schema constants ──────────────────────────────────────────────────────────

VALID_MOODS = {"foundation", "atmosphere", "entangled", "prism", "flux", "aether", "family"}

SONIC_DNA_DIMENSIONS = {"brightness", "warmth", "movement", "density", "space", "aggression"}

REQUIRED_FIELDS = {"name", "mood", "sonic_dna", "parameters", "macros"}

ALLOWED_TOP_LEVEL_KEYS = {
    "name", "engine", "engines", "mood", "sonic_dna",
    "parameters", "macros", "tags", "version", "description", "coupling",
}

VALID_MACRO_KEYS = {"m1", "m2", "m3", "m4", "CHARACTER", "MOVEMENT", "COUPLING", "SPACE"}

NAME_MAX_LEN = 30

# ── Result containers ─────────────────────────────────────────────────────────

class PresetResult:
    def __init__(self, path: Path):
        self.path = path
        self.failures: list[str] = []
        self.warnings: list[str] = []
        self.fixed: list[str] = []
        self.data: dict | None = None

    @property
    def status(self) -> str:
        if self.failures:
            return "FAIL"
        if self.warnings:
            return "WARN"
        return "PASS"


# ── Validation logic ──────────────────────────────────────────────────────────

def validate_preset(path: Path, fix: bool = False) -> PresetResult:
    result = PresetResult(path)

    # 1. JSON parse
    try:
        raw = path.read_text(encoding="utf-8")
        data = json.loads(raw)
    except json.JSONDecodeError as e:
        result.failures.append(f"JSON parse error: {e}")
        return result
    except OSError as e:
        result.failures.append(f"File read error: {e}")
        return result

    if not isinstance(data, dict):
        result.failures.append("Root value is not a JSON object")
        return result

    result.data = data

    # 2. Unknown top-level keys (warn)
    unknown_keys = set(data.keys()) - ALLOWED_TOP_LEVEL_KEYS
    if unknown_keys:
        result.warnings.append(f"Unknown top-level keys: {sorted(unknown_keys)}")

    # 3. Required fields
    for field in REQUIRED_FIELDS:
        if field not in data:
            result.failures.append(f"Missing required field: '{field}'")

    # Engine presence (engine OR engines required)
    if "engine" not in data and "engines" not in data:
        result.failures.append("Missing required field: 'engine' or 'engines'")

    # 4. name validation
    if "name" in data:
        name = data["name"]
        if not isinstance(name, str):
            result.failures.append("'name' must be a string")
        else:
            if name != name.strip():
                if fix:
                    data["name"] = name.strip()
                    result.fixed.append(f"Trimmed whitespace from name: {repr(name)} -> {repr(data['name'])}")
                else:
                    result.warnings.append(f"'name' has leading/trailing whitespace: {repr(name)}")
                name = name.strip()
            if len(name) == 0:
                result.failures.append("'name' must not be empty")
            elif len(name) > NAME_MAX_LEN:
                result.failures.append(f"'name' exceeds {NAME_MAX_LEN} chars (got {len(name)}): {repr(name)}")

    # 5. mood validation
    if "mood" in data:
        mood = data["mood"]
        if not isinstance(mood, str):
            result.failures.append("'mood' must be a string")
        else:
            mood_lower = mood.lower()
            if mood_lower not in VALID_MOODS:
                result.failures.append(
                    f"'mood' invalid value: {repr(mood)}. "
                    f"Must be one of: {sorted(VALID_MOODS)}"
                )
            else:
                # Normalize casing: capitalize first letter
                canonical = mood_lower.capitalize()
                if mood != canonical:
                    if fix:
                        data["mood"] = canonical
                        result.fixed.append(f"Normalized mood casing: {repr(mood)} -> {repr(canonical)}")
                    else:
                        result.warnings.append(
                            f"'mood' casing differs from canonical: {repr(mood)} -> {repr(canonical)}"
                        )

    # 6. sonic_dna validation
    if "sonic_dna" in data:
        dna = data["sonic_dna"]
        if not isinstance(dna, dict):
            result.failures.append("'sonic_dna' must be an object")
        else:
            missing_dims = SONIC_DNA_DIMENSIONS - set(dna.keys())
            if missing_dims:
                result.failures.append(f"'sonic_dna' missing dimensions: {sorted(missing_dims)}")
            for dim in SONIC_DNA_DIMENSIONS:
                if dim in dna:
                    val = dna[dim]
                    if not isinstance(val, (int, float)):
                        result.failures.append(f"'sonic_dna.{dim}' must be a number (got {type(val).__name__})")
                    elif not (0.0 <= float(val) <= 1.0):
                        result.failures.append(
                            f"'sonic_dna.{dim}' out of range [0.0, 1.0]: {val}"
                        )

    # 7. engines + Entangled coupling check
    mood_val = data.get("mood", "")
    mood_is_entangled = isinstance(mood_val, str) and mood_val.lower() == "entangled"

    engines_field = data.get("engines", data.get("engine"))
    if engines_field is not None:
        if isinstance(engines_field, list):
            engine_count = len(engines_field)
            if not all(isinstance(e, str) for e in engines_field):
                result.failures.append("'engines' array must contain only strings")
        elif isinstance(engines_field, str):
            engine_count = 1
        else:
            engine_count = 0
            result.failures.append("'engine'/'engines' must be a string or array of strings")

        if mood_is_entangled and engine_count < 2:
            result.failures.append(
                f"Entangled mood requires 2+ engines (got {engine_count})"
            )

    # 8. parameters validation
    if "parameters" in data:
        params = data["parameters"]
        if not isinstance(params, dict):
            result.failures.append("'parameters' must be an object")
        elif len(params) == 0:
            result.failures.append("'parameters' must not be empty")

    # 9. macros validation
    if "macros" in data:
        macros = data["macros"]
        if not isinstance(macros, dict):
            result.failures.append("'macros' must be an object")
        elif len(macros) == 0:
            result.failures.append("'macros' must have at least 1 macro defined")
        else:
            unknown_macro_keys = set(macros.keys()) - VALID_MACRO_KEYS
            if unknown_macro_keys:
                result.warnings.append(
                    f"'macros' has unrecognized keys: {sorted(unknown_macro_keys)}. "
                    f"Expected keys from: {sorted(VALID_MACRO_KEYS)}"
                )

    # 10. Optional field type checks
    if "tags" in data and not isinstance(data["tags"], list):
        result.failures.append("'tags' must be an array")

    if "version" in data and not isinstance(data["version"], str):
        result.warnings.append("'version' should be a string")

    if "description" in data and not isinstance(data["description"], str):
        result.warnings.append("'description' should be a string")

    if "coupling" in data and not isinstance(data["coupling"], dict):
        result.warnings.append("'coupling' should be an object")

    # Write back fixes if requested
    if fix and result.fixed:
        try:
            path.write_text(json.dumps(data, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
        except OSError as e:
            result.warnings.append(f"Could not write fixes: {e}")

    return result


# ── Directory scan ────────────────────────────────────────────────────────────

def scan_directory(directory: Path, fix: bool = False) -> list[PresetResult]:
    xometa_files = sorted(directory.rglob("*.xometa"))
    results = []
    for file_path in xometa_files:
        results.append(validate_preset(file_path, fix=fix))
    return results


# ── Reporting ─────────────────────────────────────────────────────────────────

def format_text_report(results: list[PresetResult], strict: bool = False) -> str:
    lines = []
    pass_count = sum(1 for r in results if r.status == "PASS")
    warn_count = sum(1 for r in results if r.status == "WARN")
    fail_count = sum(1 for r in results if r.status == "FAIL")
    total = len(results)

    lines.append("=" * 70)
    lines.append("XOmnibus .xometa Batch Validator")
    lines.append("=" * 70)
    lines.append(f"Scanned : {total} files")
    lines.append(f"PASS    : {pass_count}")
    lines.append(f"WARN    : {warn_count}")
    lines.append(f"FAIL    : {fail_count}")
    if strict:
        lines.append("Mode    : STRICT (warnings treated as failures)")
    lines.append("")

    for result in results:
        effective_status = result.status
        if strict and effective_status == "WARN":
            effective_status = "FAIL"

        if effective_status == "PASS" and not result.fixed:
            continue  # Skip clean passes in text mode for brevity

        lines.append(f"[{effective_status}] {result.path}")

        for fix_msg in result.fixed:
            lines.append(f"  FIXED   : {fix_msg}")
        for issue in result.failures:
            lines.append(f"  FAILURE : {issue}")
        for issue in result.warnings:
            marker = "FAILURE" if strict else "WARNING"
            lines.append(f"  {marker} : {issue}")
        lines.append("")

    lines.append("=" * 70)
    overall = "ALL PASS" if (fail_count == 0 and (not strict or warn_count == 0)) else "FAILED"
    lines.append(f"Result  : {overall}")
    lines.append("=" * 70)

    return "\n".join(lines)


def format_json_report(results: list[PresetResult], strict: bool = False) -> str:
    pass_count = sum(1 for r in results if r.status == "PASS")
    warn_count = sum(1 for r in results if r.status == "WARN")
    fail_count = sum(1 for r in results if r.status == "FAIL")

    report = {
        "summary": {
            "total": len(results),
            "pass": pass_count,
            "warn": warn_count,
            "fail": fail_count,
            "strict": strict,
            "overall": "pass" if (fail_count == 0 and (not strict or warn_count == 0)) else "fail",
        },
        "files": [],
    }

    for result in results:
        effective_status = result.status
        if strict and effective_status == "WARN":
            effective_status = "FAIL"
        report["files"].append({
            "path": str(result.path),
            "status": effective_status,
            "failures": result.failures,
            "warnings": result.warnings,
            "fixed": result.fixed,
        })

    return json.dumps(report, indent=2)


# ── CLI entry point ───────────────────────────────────────────────────────────

def main() -> int:
    parser = argparse.ArgumentParser(
        description="Validate XOmnibus .xometa preset files against the preset schema.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Exit codes:
  0  All files pass (no failures, no warnings)
  1  Warnings present but no failures (or 0 if --strict is not set and all pass)
  2  One or more failures detected
""",
    )
    parser.add_argument("directory", type=Path, help="Directory to scan recursively for .xometa files")
    parser.add_argument("--strict", action="store_true", help="Treat warnings as failures")
    parser.add_argument("--fix", action="store_true", help="Auto-fix: trim name whitespace, normalize mood casing")
    parser.add_argument(
        "--format", choices=["text", "json"], default="text", help="Output format (default: text)"
    )
    parser.add_argument("--output", type=Path, default=None, help="Write report to file instead of stdout")

    args = parser.parse_args()

    if not args.directory.exists():
        print(f"Error: directory not found: {args.directory}", file=sys.stderr)
        return 2

    if not args.directory.is_dir():
        print(f"Error: not a directory: {args.directory}", file=sys.stderr)
        return 2

    results = scan_directory(args.directory, fix=args.fix)

    if not results:
        msg = f"No .xometa files found in: {args.directory}"
        if args.output:
            args.output.write_text(msg + "\n", encoding="utf-8")
        else:
            print(msg)
        return 0

    if args.format == "json":
        report = format_json_report(results, strict=args.strict)
    else:
        report = format_text_report(results, strict=args.strict)

    if args.output:
        args.output.write_text(report + "\n", encoding="utf-8")
        print(f"Report written to: {args.output}")
    else:
        print(report)

    # Determine exit code
    fail_count = sum(1 for r in results if r.status == "FAIL")
    warn_count = sum(1 for r in results if r.status == "WARN")

    if fail_count > 0:
        return 2
    if args.strict and warn_count > 0:
        return 2
    if warn_count > 0:
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
