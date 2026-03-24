#!/usr/bin/env python3
"""
xpn_pack_format_validator.py — Deep XPN/XPM format validator for XO_OX XOlokun packs.

Validates XPM XML schema and MPC compatibility beyond surface-level QA checks.

Usage:
    python xpn_pack_format_validator.py <dir_or_xpm> [--strict] [--output report.txt] [--format text|json]
"""

import argparse
import json
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

VALID_PROGRAM_TYPES = {
    "KeygroupProgram",
    "DrumProgram",
    "MidiProgram",
    "CVProgram",
    "PluginProgram",
    "ElectronicDrumProgram",
}

ENVELOPE_ELEMENTS = [
    "VolumeAttack",
    "VolumeHold",
    "VolumeDecay",
    "VolumeSustain",
    "VolumeRelease",
]

MIDI_MIN = 0
MIDI_MAX = 127


# ---------------------------------------------------------------------------
# Issue dataclass (plain dict for stdlib compat)
# ---------------------------------------------------------------------------

def make_issue(level: str, code: str, message: str, element: str = "") -> dict:
    return {"level": level, "code": code, "message": message, "element": element}


# ---------------------------------------------------------------------------
# Core validator
# ---------------------------------------------------------------------------

def validate_xpm(path: Path, strict: bool = False) -> dict:
    """
    Validate a single XPM file.
    Returns a result dict with keys: path, status, issues.
    """
    issues = []
    result = {"path": str(path), "status": "PASS", "issues": issues}

    # --- Encoding check ---
    try:
        raw = path.read_bytes()
    except OSError as e:
        issues.append(make_issue("FAIL", "IO_ERROR", f"Cannot read file: {e}"))
        result["status"] = "FAIL"
        return result

    try:
        text = raw.decode("utf-8")
    except UnicodeDecodeError as e:
        issues.append(make_issue("FAIL", "ENCODING", f"File is not valid UTF-8: {e}"))
        result["status"] = "FAIL"
        return result

    # Check for non-ASCII characters in the decoded text
    for lineno, line in enumerate(text.splitlines(), 1):
        for col, ch in enumerate(line, 1):
            if ord(ch) > 127:
                issues.append(make_issue(
                    "WARN", "NON_ASCII",
                    f"Non-ASCII character U+{ord(ch):04X} '{ch}' at line {lineno}, col {col} "
                    f"(may cause issues on some MPC firmware versions)"
                ))
                break  # one warning per line is enough

    # --- XML parse ---
    try:
        root = ET.fromstring(text)
    except ET.ParseError as e:
        issues.append(make_issue("FAIL", "XML_PARSE", f"XML parse error: {e}"))
        result["status"] = "FAIL"
        return result

    # --- Rule 1: Root element ---
    if root.tag not in ("MPCVObject", "Program"):
        issues.append(make_issue(
            "FAIL", "ROOT_ELEMENT",
            f"Root element must be <MPCVObject> or <Program>, got <{root.tag}>"
        ))
        result["status"] = "FAIL"
        return result

    # Navigate to Program element
    if root.tag == "MPCVObject":
        program = root.find("Program")
        if program is None:
            issues.append(make_issue("FAIL", "NO_PROGRAM", "<MPCVObject> contains no <Program> child"))
            result["status"] = "FAIL"
            return result
    else:
        program = root

    # --- Rule 2: Program Type attribute ---
    prog_type = program.get("type") or program.get("Type")
    if not prog_type:
        issues.append(make_issue("FAIL", "MISSING_TYPE", "<Program> has no 'type' or 'Type' attribute"))
        result["status"] = "FAIL"
    else:
        if prog_type not in VALID_PROGRAM_TYPES:
            issues.append(make_issue(
                "WARN", "UNKNOWN_TYPE",
                f"Program type '{prog_type}' is not a known MPC program type"
            ))

    # --- Rule 3: KeygroupProgram must have Keygroup elements ---
    if prog_type == "KeygroupProgram":
        keygroups = program.findall(".//Keygroup")
        if not keygroups:
            issues.append(make_issue("FAIL", "NO_KEYGROUPS", "KeygroupProgram has no <Keygroup> elements"))
            result["status"] = "FAIL"
        else:
            _validate_keygroups(program, keygroups, issues, strict)

    # --- Rule 4: DrumProgram must have Pad elements ---
    elif prog_type == "DrumProgram":
        pads = program.findall(".//Pad")
        if not pads:
            issues.append(make_issue("FAIL", "NO_PADS", "DrumProgram has no <Pad> elements"))
            result["status"] = "FAIL"
        else:
            _validate_drum_pads(pads, issues, strict)

    # --- Rules 5-8: Instrument (layer) validation for all program types ---
    _validate_instruments(program, issues, strict)

    # --- Derive final status ---
    if result["status"] != "FAIL":
        has_fail = any(i["level"] == "FAIL" for i in issues)
        has_warn = any(i["level"] == "WARN" for i in issues)
        if has_fail:
            result["status"] = "FAIL"
        elif has_warn:
            result["status"] = "WARN"
        else:
            result["status"] = "PASS"

    return result


# ---------------------------------------------------------------------------
# Sub-validators
# ---------------------------------------------------------------------------

def _validate_keygroups(program: ET.Element, keygroups: list, issues: list, strict: bool):
    """Rules 9 + 10: MIDI range and duplicate name checks."""
    seen_names = {}

    for kg in keygroups:
        # Name uniqueness (Rule 10)
        name_el = kg.find("Name") or kg.find("ProgramName")
        name = name_el.text.strip() if (name_el is not None and name_el.text) else None
        if name:
            if name in seen_names:
                issues.append(make_issue(
                    "FAIL", "DUPLICATE_KG_NAME",
                    f"Duplicate keygroup name '{name}'",
                    element="Keygroup"
                ))
            else:
                seen_names[name] = True

        # MIDI range (Rule 9)
        for tag in ("KeyLow", "KeyHigh"):
            el = kg.find(tag)
            if el is not None and el.text is not None:
                raw = el.text.strip()
                try:
                    val = int(raw)
                    if not (MIDI_MIN <= val <= MIDI_MAX):
                        issues.append(make_issue(
                            "FAIL", "MIDI_RANGE",
                            f"<{tag}> value {val} is outside valid MIDI range 0-127",
                            element=tag
                        ))
                except ValueError:
                    issues.append(make_issue(
                        "FAIL", "MIDI_RANGE",
                        f"<{tag}> has non-integer value '{raw}'",
                        element=tag
                    ))
            elif strict and el is None:
                issues.append(make_issue(
                    "WARN", "MISSING_MIDI_RANGE",
                    f"Keygroup missing <{tag}> element",
                    element="Keygroup"
                ))


def _validate_drum_pads(pads: list, issues: list, strict: bool):
    """Basic drum pad validation — instruments inside pads handled by _validate_instruments."""
    if strict and len(pads) < 1:
        issues.append(make_issue("WARN", "FEW_PADS", f"DrumProgram has only {len(pads)} pad(s)"))


def _validate_instruments(program: ET.Element, issues: list, strict: bool):
    """Rules 5, 6, 7, 8: Envelope elements, path checks on all Instrument/Layer elements."""
    instruments = program.findall(".//Instrument")
    # Also catch Layer elements used in some MPC firmware variants
    instruments += program.findall(".//Layer")

    for inst in instruments:
        # Rule 5: Envelope elements
        for env_tag in ENVELOPE_ELEMENTS:
            if inst.find(env_tag) is None:
                level = "FAIL" if strict else "WARN"
                issues.append(make_issue(
                    level, "MISSING_ENVELOPE",
                    f"<Instrument> missing envelope element <{env_tag}>",
                    element=env_tag
                ))

        # Rules 6, 7, 8: SampleFile path validation
        for sf_el in inst.findall(".//SampleFile") + inst.findall("SampleFile"):
            _validate_sample_file(sf_el, issues)

    # Also catch SampleFile elements that live outside Instrument (some drum programs)
    for sf_el in program.findall(".//SampleFile"):
        # Avoid double-counting those already inside Instrument
        # (findall searches descendants so this catches all; we de-dup by only checking
        # ones not inside Instrument by checking parent chain — too complex for stdlib;
        # instead we just call the same fn which is idempotent for duplicate issues)
        _validate_sample_file(sf_el, issues)

    # De-duplicate issues (same level+code+message) that may arise from double-traversal
    seen = set()
    deduped = []
    for iss in issues:
        key = (iss["level"], iss["code"], iss["message"], iss["element"])
        if key not in seen:
            seen.add(key)
            deduped.append(iss)
    issues[:] = deduped


def _validate_sample_file(sf_el: ET.Element, issues: list):
    """Validate a single <SampleFile> element (Rules 6, 7, 8)."""
    val = sf_el.text
    if val is None or val.strip() == "":
        # Rule 8: empty sample file — only warn (may be intentionally empty placeholder layer)
        issues.append(make_issue(
            "WARN", "EMPTY_SAMPLE_FILE",
            "<SampleFile> is empty — layer may be a placeholder or unfilled",
            element="SampleFile"
        ))
        return

    val = val.strip()

    # Rule 6: must not be absolute path
    if val.startswith("/"):
        issues.append(make_issue(
            "FAIL", "ABSOLUTE_PATH",
            f"<SampleFile> is an absolute path (will break on MPC): '{val}'",
            element="SampleFile"
        ))

    # Rule 7: must not contain backslashes
    if "\\" in val:
        issues.append(make_issue(
            "FAIL", "BACKSLASH_PATH",
            f"<SampleFile> contains backslash (Windows path, will fail on MPC): '{val}'",
            element="SampleFile"
        ))


# ---------------------------------------------------------------------------
# Reporting
# ---------------------------------------------------------------------------

def collect_xpm_files(target: Path) -> list:
    if target.is_file():
        return [target] if target.suffix.lower() == ".xpm" else []
    return sorted(target.rglob("*.xpm"))


def compute_fleet_score(results: list) -> dict:
    total = len(results)
    passed = sum(1 for r in results if r["status"] == "PASS")
    warned = sum(1 for r in results if r["status"] == "WARN")
    failed = sum(1 for r in results if r["status"] == "FAIL")
    score = int((passed + warned * 0.5) / total * 100) if total else 0
    return {
        "total": total,
        "pass": passed,
        "warn": warned,
        "fail": failed,
        "fleet_score": score,
    }


def render_text(results: list, summary: dict) -> str:
    lines = []
    lines.append("=" * 72)
    lines.append("XPN PACK FORMAT VALIDATOR — DEEP XPM SCHEMA CHECK")
    lines.append("=" * 72)
    lines.append("")

    for r in results:
        status = r["status"]
        marker = {"PASS": "[PASS]", "WARN": "[WARN]", "FAIL": "[FAIL]"}.get(status, "[????]")
        lines.append(f"{marker}  {r['path']}")
        if r["issues"]:
            for iss in r["issues"]:
                prefix = {"FAIL": "  !! ", "WARN": "  ?? ", "INFO": "  -- "}.get(iss["level"], "  -- ")
                elem = f" [{iss['element']}]" if iss["element"] else ""
                lines.append(f"{prefix}[{iss['code']}]{elem} {iss['message']}")
        lines.append("")

    lines.append("-" * 72)
    lines.append("FLEET SUMMARY")
    lines.append(f"  Total files : {summary['total']}")
    lines.append(f"  PASS        : {summary['pass']}")
    lines.append(f"  WARN        : {summary['warn']}")
    lines.append(f"  FAIL        : {summary['fail']}")
    lines.append(f"  Fleet score : {summary['fleet_score']}%")
    lines.append("=" * 72)
    return "\n".join(lines)


def render_json(results: list, summary: dict) -> str:
    return json.dumps({"summary": summary, "results": results}, indent=2)


# ---------------------------------------------------------------------------
# CLI entry point
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description="Deep XPN/XPM format validator for XO_OX XOlokun packs."
    )
    parser.add_argument("target", help="Path to a .xpm file or directory to scan recursively")
    parser.add_argument(
        "--strict",
        action="store_true",
        help="Treat WARN-level issues (e.g. missing envelopes) as FAIL",
    )
    parser.add_argument("--output", metavar="FILE", help="Write report to this file instead of stdout")
    parser.add_argument(
        "--format",
        choices=["text", "json"],
        default="text",
        help="Output format: text (default) or json",
    )
    args = parser.parse_args()

    target = Path(args.target)
    if not target.exists():
        print(f"ERROR: Path does not exist: {target}", file=sys.stderr)
        sys.exit(2)

    xpm_files = collect_xpm_files(target)
    if not xpm_files:
        print(f"No .xpm files found under: {target}", file=sys.stderr)
        sys.exit(1)

    results = [validate_xpm(f, strict=args.strict) for f in xpm_files]

    # In strict mode, upgrade WARN results to FAIL
    if args.strict:
        for r in results:
            if r["status"] == "WARN":
                r["status"] = "FAIL"

    summary = compute_fleet_score(results)

    if args.format == "json":
        output = render_json(results, summary)
    else:
        output = render_text(results, summary)

    if args.output:
        out_path = Path(args.output)
        out_path.write_text(output, encoding="utf-8")
        print(f"Report written to: {out_path}")
    else:
        print(output)

    # Exit code: 0 = all pass/warn, 1 = any fail
    any_fail = any(r["status"] == "FAIL" for r in results)
    sys.exit(1 if any_fail else 0)


if __name__ == "__main__":
    main()
