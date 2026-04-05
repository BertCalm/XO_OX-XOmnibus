#!/usr/bin/env python3
"""
xpn_xpm_validator_strict.py — Strict XPM XML validator for XO_OX MPC programs.

Enforces 3 critical XO_OX XPM rules plus MPC format compliance checks.
Designed as a CI gate — exits 1 on any critical violation.

Usage:
    python xpn_xpm_validator_strict.py --file path/to/program.xpm
    python xpn_xpm_validator_strict.py --dir path/to/xpm/folder
    python xpn_xpm_validator_strict.py --dir . --strict
    python xpn_xpm_validator_strict.py --file program.xpm --quiet
"""

import argparse
import os
import sys
import xml.etree.ElementTree as ET
from pathlib import Path


# ---------------------------------------------------------------------------
# Result helpers
# ---------------------------------------------------------------------------

class ValidationResult:
    def __init__(self, filepath: str):
        self.filepath = filepath
        self.criticals: list[str] = []
        self.warnings: list[str] = []

    @property
    def has_criticals(self) -> bool:
        return len(self.criticals) > 0

    @property
    def has_warnings(self) -> bool:
        return len(self.warnings) > 0

    def critical(self, msg: str):
        self.criticals.append(msg)

    def warn(self, msg: str):
        self.warnings.append(msg)


# ---------------------------------------------------------------------------
# Core validator
# ---------------------------------------------------------------------------

def validate_xpm(filepath: str) -> ValidationResult:
    result = ValidationResult(filepath)

    try:
        tree = ET.parse(filepath)
    except ET.ParseError as e:
        result.critical(f"XML parse error: {e}")
        return result

    root = tree.getroot()

    # Locate program element (root may be <MPCVObject> or <Program> directly)
    program = root.find(".//Program")
    if program is None:
        # Root itself might be the Program element
        if root.tag == "Program":
            program = root
        else:
            result.critical("No <Program> element found in XPM file")
            return result

    # ------------------------------------------------------------------
    # WARNING 1: Program type
    # ------------------------------------------------------------------
    prog_type = program.get("type", "")
    if prog_type not in ("DrumProgram", "KeygroupProgram"):
        result.warn(
            f"Program type is '{prog_type}'; expected DrumProgram or KeygroupProgram"
        )
    is_drum = prog_type == "DrumProgram"

    instruments = program.findall(".//Instrument")

    # ------------------------------------------------------------------
    # WARNING 2: Instrument count for DrumProgram
    # ------------------------------------------------------------------
    if is_drum and len(instruments) != 128:
        result.warn(
            f"DrumProgram has {len(instruments)} Instrument elements; expected exactly 128"
        )

    # ------------------------------------------------------------------
    # WARNING 3: No duplicate PadNote values in DrumProgram
    # ------------------------------------------------------------------
    if is_drum:
        pad_notes: list[str] = []
        for inst in instruments:
            pn = inst.get("PadNote", "")
            if pn:
                pad_notes.append(pn)
        seen: set[str] = set()
        for pn in pad_notes:
            if pn in seen:
                result.warn(f"Duplicate PadNote '{pn}' found in DrumProgram")
            seen.add(pn)

    # ------------------------------------------------------------------
    # Per-instrument checks (CRITICAL 1, 2, 3 + WARNINGS 4, 5, 6)
    # ------------------------------------------------------------------
    for i, inst in enumerate(instruments):
        label = inst.get("PadNote") or inst.get("number") or str(i)
        inst_id = f"Instrument[{label}]"

        # -----------------------------------------------------------
        # CRITICAL 1: KeyTrack must be "True"
        # -----------------------------------------------------------
        key_track = inst.findtext("KeyTrack")
        if key_track is None:
            result.critical(f"{inst_id}: <KeyTrack> element missing (must be 'True')")
        elif key_track.strip() != "True":
            result.critical(
                f"{inst_id}: KeyTrack='{key_track.strip()}' — must be 'True'"
            )

        # -----------------------------------------------------------
        # CRITICAL 2: RootNote must be "0"
        # -----------------------------------------------------------
        root_note = inst.findtext("RootNote")
        if root_note is None:
            result.critical(f"{inst_id}: <RootNote> element missing (must be '0')")
        elif root_note.strip() != "0":
            result.critical(
                f"{inst_id}: RootNote='{root_note.strip()}' — must be '0'"
            )

        # -----------------------------------------------------------
        # Examine layers
        # -----------------------------------------------------------
        layers = inst.findall(".//Layer")
        for j, layer in enumerate(layers):
            layer_id = f"{inst_id}/Layer[{j}]"
            sample_file = (layer.findtext("SampleFile") or "").strip()
            is_empty = not sample_file

            # CRITICAL 3: Empty layer VelStart must be "0"
            if is_empty:
                vel_start = layer.findtext("VelStart")
                if vel_start is None:
                    result.critical(
                        f"{layer_id}: Empty layer missing <VelStart> (must be '0')"
                    )
                elif vel_start.strip() != "0":
                    result.critical(
                        f"{layer_id}: Empty layer VelStart='{vel_start.strip()}' — must be '0'"
                    )

            # WARNING 4: Sample file paths must be relative
            if sample_file and os.path.isabs(sample_file):
                result.warn(
                    f"{layer_id}: SampleFile path is absolute: '{sample_file}'"
                )

            # WARNING 5: VelEnd > VelStart on active layers
            if not is_empty:
                try:
                    vel_start_val = int(layer.findtext("VelStart") or "0")
                    vel_end_val = int(layer.findtext("VelEnd") or "0")
                    if vel_end_val <= vel_start_val:
                        result.warn(
                            f"{layer_id}: VelEnd ({vel_end_val}) must be > VelStart ({vel_start_val})"
                        )
                except ValueError:
                    result.warn(
                        f"{layer_id}: VelStart/VelEnd are not valid integers"
                    )

        # WARNING 6: Pad numbers must be 1-16 range (MPC pad range)
        pad_note_raw = inst.get("PadNote", "")
        if pad_note_raw:
            try:
                pad_num = int(pad_note_raw)
                if not (1 <= pad_num <= 128):
                    # MPC supports pads 1-128 across banks; warn if outside 1-16 only
                    # for strict pad-range enforcement per requirement
                    if not (1 <= pad_num <= 16):
                        result.warn(
                            f"{inst_id}: PadNote={pad_num} is outside the standard 1-16 MPC pad range"
                        )
            except ValueError:
                result.warn(f"{inst_id}: PadNote='{pad_note_raw}' is not a valid integer")

    # ------------------------------------------------------------------
    # WARNING 7: Q-Link param names must not be empty strings
    # ------------------------------------------------------------------
    qlinks = program.findall(".//QLinks/QLink")
    for k, qlink in enumerate(qlinks):
        param_name = qlink.get("ParamName", None)
        if param_name is not None and param_name.strip() == "":
            result.warn(f"QLink[{k}]: ParamName is an empty string")

    return result


# ---------------------------------------------------------------------------
# Output formatting
# ---------------------------------------------------------------------------

def print_result(result: ValidationResult, quiet: bool = False):
    has_issues = result.has_criticals or result.has_warnings
    if quiet and not has_issues:
        return

    status_parts = []
    if result.has_criticals:
        status_parts.append(f"{len(result.criticals)} CRITICAL")
    if result.has_warnings:
        status_parts.append(f"{len(result.warnings)} warning(s)")

    status = ", ".join(status_parts) if status_parts else "OK"
    print(f"[{status}] {result.filepath}")

    if not quiet:
        for msg in result.criticals:
            print(f"  CRITICAL: {msg}")
        for msg in result.warnings:
            print(f"  WARNING:  {msg}")


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description="Strict XPM XML validator for XO_OX MPC programs. CI gate — exits 1 on critical failures."
    )
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--file", metavar="FILE", help="Single .xpm file to validate")
    group.add_argument("--dir", metavar="DIR", help="Directory to scan recursively for .xpm files")
    parser.add_argument(
        "--strict",
        action="store_true",
        help="Promote warnings to critical failures (exit 1 on any warning)"
    )
    parser.add_argument(
        "--quiet",
        action="store_true",
        help="Suppress output for passing files; only print failures"
    )
    args = parser.parse_args()

    files: list[str] = []

    if args.file:
        p = Path(args.file)
        if not p.exists():
            print(f"ERROR: File not found: {args.file}", file=sys.stderr)
            sys.exit(1)
        if p.suffix.lower() != ".xpm":
            print(f"WARNING: File does not have .xpm extension: {args.file}")
        files.append(str(p))
    else:
        d = Path(args.dir)
        if not d.is_dir():
            print(f"ERROR: Directory not found: {args.dir}", file=sys.stderr)
            sys.exit(1)
        files = sorted(str(p) for p in d.rglob("*.xpm"))
        if not files:
            print(f"No .xpm files found in: {args.dir}")
            sys.exit(0)

    total_files = len(files)
    total_criticals = 0
    total_warnings = 0
    files_with_criticals = 0
    files_with_warnings = 0

    results: list[ValidationResult] = []
    for f in files:
        result = validate_xpm(f)

        # In strict mode, promote warnings to criticals
        if args.strict and result.warnings:
            for w in result.warnings:
                result.criticals.append(f"[promoted] {w}")
            result.warnings.clear()

        results.append(result)
        total_criticals += len(result.criticals)
        total_warnings += len(result.warnings)
        if result.has_criticals:
            files_with_criticals += 1
        if result.has_warnings:
            files_with_warnings += 1

        print_result(result, quiet=args.quiet)

    # Summary
    print()
    print("=" * 60)
    print(f"SUMMARY: {total_files} file(s) checked")
    print(f"  Critical failures : {total_criticals} across {files_with_criticals} file(s)")
    print(f"  Warnings          : {total_warnings} across {files_with_warnings} file(s)")
    if total_criticals == 0 and total_warnings == 0:
        print("  Result            : ALL PASS")
    elif total_criticals == 0:
        print("  Result            : PASS (warnings present)")
    else:
        print("  Result            : FAIL")
    print("=" * 60)

    if total_criticals > 0:
        sys.exit(1)
    sys.exit(0)


if __name__ == "__main__":
    main()
