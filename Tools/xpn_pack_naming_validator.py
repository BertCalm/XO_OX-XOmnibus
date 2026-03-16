#!/usr/bin/env python3
"""
xpn_pack_naming_validator.py — XO_OX brand naming validator for XPN packs.

Validates pack names, program names, sample names, and preset names against
XO_OX brand conventions. Produces a health score and optional fix suggestions.

Usage:
    python xpn_pack_naming_validator.py <pack_dir> [--fix-suggestions] [--strict] [--format text|json]
"""

import argparse
import json
import re
import sys
import xml.etree.ElementTree as ET
from dataclasses import dataclass, field
from pathlib import Path
from typing import Optional


# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

JARGON_BLACKLIST = {
    "LPF", "HPF", "BPF", "VCA", "VCO", "VCF", "OSC", "ENV", "LFO",
    "ADSR", "EQ", "Hz", "dB", "ms",
}

GENERIC_NAMES = {
    "untitled", "default", "new program", "sample 1", "sample1",
    "program 1", "program1", "preset 1", "preset1", "patch 1", "patch1",
    "new preset", "new patch", "unnamed", "noname", "test", "temp",
    "placeholder", "demo",
}

# Engine names that are acceptable in preset names
ENGINE_NAMES = {
    "OPAL", "ONSET", "OVERLAP", "OUTWIT", "OVERWORLD", "OVERDUB",
    "ODYSSEY", "OBLONG", "OBBLIGATO", "OTTONI", "OLE", "ORPHICA",
    "OHM", "OVERBITE", "OSTINATO", "OPENSKY", "OCEANDEEP", "OUIE",
    "OUROBOROS", "ORGANON", "FAT", "DRIFT", "DUB", "BOB", "XOBESE",
}

SPECIAL_CHAR_PATTERN = re.compile(r"[^A-Za-z0-9 \-]")
SNAKE_OR_TITLE_UNDERSCORE = re.compile(r"^[A-Za-z0-9]+([_][A-Za-z0-9]+)*$")
SPACES_IN_FILENAME = re.compile(r" ")
VERSION_IN_NAME = re.compile(r"\b[Vv]\d+(\.\d+)*\b")

SEVERITY_CRITICAL = "CRITICAL"
SEVERITY_WARNING  = "WARNING"
SEVERITY_INFO     = "INFO"


# ---------------------------------------------------------------------------
# Data structures
# ---------------------------------------------------------------------------

@dataclass
class Violation:
    severity: str          # CRITICAL / WARNING / INFO
    category: str          # pack / program / sample / preset
    name: str              # the offending name
    rule: str              # short rule key
    message: str           # human-readable explanation
    suggestion: Optional[str] = None  # proposed fix (if --fix-suggestions)


@dataclass
class ValidationResult:
    violations: list[Violation] = field(default_factory=list)
    pack_name: Optional[str] = None
    program_names: list[str] = field(default_factory=list)
    sample_names: list[str] = field(default_factory=list)
    preset_names: list[str] = field(default_factory=list)

    def score(self) -> int:
        """Compute 0-100 health score. Criticals cost 10 pts, warnings 3, info 1."""
        deductions = sum(
            10 if v.severity == SEVERITY_CRITICAL else
             3 if v.severity == SEVERITY_WARNING else 1
            for v in self.violations
        )
        total_names = max(
            1,
            (1 if self.pack_name else 0) +
            len(self.program_names) +
            len(self.sample_names) +
            len(self.preset_names),
        )
        # Scale so a single critical in a large pack hurts less
        penalty = min(100, int(deductions * 100 / (total_names * 2 + 20)))
        return max(0, 100 - penalty)


# ---------------------------------------------------------------------------
# Suggestion helpers
# ---------------------------------------------------------------------------

def _to_title_case(name: str) -> str:
    return " ".join(w.capitalize() for w in name.strip().split())


def _to_snake_case(name: str) -> str:
    name = name.strip()
    name = re.sub(r"[^A-Za-z0-9_]", "_", name)
    name = re.sub(r"_+", "_", name).strip("_")
    return name


def _strip_jargon(name: str) -> str:
    words = name.split()
    cleaned = [w for w in words if w.upper() not in JARGON_BLACKLIST]
    return " ".join(cleaned) if cleaned else name


def _strip_version(name: str) -> str:
    return VERSION_IN_NAME.sub("", name).strip()


# ---------------------------------------------------------------------------
# Per-field validators
# ---------------------------------------------------------------------------

def validate_pack_name(name: str, fix: bool) -> list[Violation]:
    violations: list[Violation] = []

    if not name or name.lower() in GENERIC_NAMES:
        violations.append(Violation(
            SEVERITY_CRITICAL, "pack", name, "generic_name",
            f"Pack name '{name}' is generic or empty.",
            suggestion=None,
        ))
        return violations

    words = name.split()
    if len(words) < 2 or len(words) > 3:
        msg = f"Pack name '{name}' has {len(words)} word(s); expected 2-3."
        sugg = None
        if fix:
            sugg = name  # can't auto-fix evocativeness
        violations.append(Violation(SEVERITY_WARNING, "pack", name, "word_count", msg, sugg))

    if name != _to_title_case(name):
        violations.append(Violation(
            SEVERITY_WARNING, "pack", name, "title_case",
            f"Pack name '{name}' should be Title Case.",
            suggestion=_to_title_case(name) if fix else None,
        ))

    if VERSION_IN_NAME.search(name):
        violations.append(Violation(
            SEVERITY_WARNING, "pack", name, "version_in_name",
            f"Pack name '{name}' contains a version number; remove from base name.",
            suggestion=_strip_version(name) if fix else None,
        ))

    return violations


def validate_preset_or_program_name(name: str, category: str, fix: bool, strict: bool) -> list[Violation]:
    violations: list[Violation] = []

    if not name or name.lower().strip() in GENERIC_NAMES:
        violations.append(Violation(
            SEVERITY_CRITICAL, category, name, "generic_name",
            f"{category.capitalize()} name '{name}' is generic or empty.",
        ))
        return violations

    if len(name) > 30:
        violations.append(Violation(
            SEVERITY_WARNING, category, name, "too_long",
            f"'{name}' is {len(name)} chars; max is 30.",
            suggestion=name[:30].strip() if fix else None,
        ))

    words = name.split()
    if len(words) < 2 or len(words) > 3:
        violations.append(Violation(
            SEVERITY_INFO, category, name, "word_count",
            f"'{name}' has {len(words)} word(s); 2-3 words preferred.",
        ))

    # Jargon check — skip words that are engine names
    jargon_hits = [
        w for w in words
        if w.upper() in JARGON_BLACKLIST and w.upper() not in ENGINE_NAMES
    ]
    if jargon_hits:
        sev = SEVERITY_WARNING if strict else SEVERITY_WARNING
        cleaned = _strip_jargon(name)
        violations.append(Violation(
            sev, category, name, "jargon",
            f"'{name}' contains jargon: {', '.join(jargon_hits)}.",
            suggestion=cleaned if fix and cleaned != name else None,
        ))

    bad_chars = SPECIAL_CHAR_PATTERN.findall(name)
    if bad_chars:
        violations.append(Violation(
            SEVERITY_WARNING, category, name, "special_chars",
            f"'{name}' contains disallowed chars: {''.join(set(bad_chars))}. Use only letters, digits, hyphen, space.",
            suggestion=re.sub(SPECIAL_CHAR_PATTERN, "", name).strip() if fix else None,
        ))

    return violations


def validate_sample_name(stem: str, fix: bool) -> list[Violation]:
    """Validate the filename stem (no extension, no path)."""
    violations: list[Violation] = []

    if not stem or stem.lower() in GENERIC_NAMES:
        violations.append(Violation(
            SEVERITY_CRITICAL, "sample", stem, "generic_name",
            f"Sample '{stem}' is generic or empty.",
        ))
        return violations

    if SPACES_IN_FILENAME.search(stem):
        violations.append(Violation(
            SEVERITY_CRITICAL, "sample", stem, "spaces_in_filename",
            f"Sample filename '{stem}' contains spaces; use underscores.",
            suggestion=stem.replace(" ", "_") if fix else None,
        ))

    if not SNAKE_OR_TITLE_UNDERSCORE.match(stem):
        violations.append(Violation(
            SEVERITY_WARNING, "sample", stem, "filename_format",
            f"Sample '{stem}' should be snake_case or Title_Case_With_Underscores.",
            suggestion=_to_snake_case(stem) if fix else None,
        ))

    return violations


def check_duplicates(names: list[str], category: str) -> list[Violation]:
    seen: dict[str, int] = {}
    violations: list[Violation] = []
    for n in names:
        key = n.strip().lower()
        seen[key] = seen.get(key, 0) + 1
    for key, count in seen.items():
        if count > 1:
            violations.append(Violation(
                SEVERITY_CRITICAL, category, key, "duplicate",
                f"'{key}' appears {count} times in {category} names.",
            ))
    return violations


# ---------------------------------------------------------------------------
# Pack scanning
# ---------------------------------------------------------------------------

def scan_pack(pack_dir: Path, fix: bool, strict: bool) -> ValidationResult:
    result = ValidationResult()

    # --- expansion.json → pack name ---
    exp_json = pack_dir / "expansion.json"
    if exp_json.exists():
        try:
            data = json.loads(exp_json.read_text(encoding="utf-8"))
            result.pack_name = data.get("name") or data.get("Name") or ""
        except (json.JSONDecodeError, OSError) as exc:
            result.violations.append(Violation(
                SEVERITY_CRITICAL, "pack", str(exp_json), "parse_error",
                f"Could not parse expansion.json: {exc}",
            ))

    if result.pack_name is not None:
        result.violations.extend(validate_pack_name(result.pack_name, fix))
    else:
        result.violations.append(Violation(
            SEVERITY_WARNING, "pack", str(pack_dir), "no_expansion_json",
            "No expansion.json found; pack name cannot be validated.",
        ))

    # --- .xpm files → program names ---
    for xpm_path in sorted(pack_dir.rglob("*.xpm")):
        try:
            tree = ET.parse(xpm_path)
            root = tree.getroot()
            prog_name = (
                root.findtext("Name") or
                root.get("name") or
                root.findtext("ProgramName") or
                xpm_path.stem
            )
        except ET.ParseError:
            prog_name = xpm_path.stem
        result.program_names.append(prog_name)
        result.violations.extend(
            validate_preset_or_program_name(prog_name, "program", fix, strict)
        )

    result.violations.extend(check_duplicates(result.program_names, "program"))

    # --- .wav files → sample names ---
    for wav_path in sorted(pack_dir.rglob("*.wav")):
        stem = wav_path.stem
        result.sample_names.append(stem)
        result.violations.extend(validate_sample_name(stem, fix))

    result.violations.extend(check_duplicates(result.sample_names, "sample"))

    # --- .xometa files → preset names ---
    for meta_path in sorted(pack_dir.rglob("*.xometa")):
        try:
            data = json.loads(meta_path.read_text(encoding="utf-8"))
            preset_name = (
                data.get("name") or data.get("Name") or
                data.get("presetName") or meta_path.stem
            )
        except (json.JSONDecodeError, OSError):
            preset_name = meta_path.stem
        result.preset_names.append(preset_name)
        result.violations.extend(
            validate_preset_or_program_name(preset_name, "preset", fix, strict)
        )

    result.violations.extend(check_duplicates(result.preset_names, "preset"))

    return result


# ---------------------------------------------------------------------------
# Output formatters
# ---------------------------------------------------------------------------

def _severity_symbol(sev: str) -> str:
    return {"CRITICAL": "[!!]", "WARNING": "[ W]", "INFO": "[ i]"}.get(sev, "[  ]")


def format_text(result: ValidationResult, fix: bool) -> str:
    lines: list[str] = []
    score = result.score()

    lines.append("=" * 60)
    lines.append("  XO_OX Pack Naming Validator")
    lines.append("=" * 60)
    if result.pack_name is not None:
        lines.append(f"  Pack : {result.pack_name or '(empty)'}")
    lines.append(f"  Programs : {len(result.program_names)}")
    lines.append(f"  Samples  : {len(result.sample_names)}")
    lines.append(f"  Presets  : {len(result.preset_names)}")
    lines.append(f"  Score    : {score}/100")
    lines.append("-" * 60)

    criticals = [v for v in result.violations if v.severity == SEVERITY_CRITICAL]
    warnings  = [v for v in result.violations if v.severity == SEVERITY_WARNING]
    infos     = [v for v in result.violations if v.severity == SEVERITY_INFO]

    for group_label, group in [
        ("CRITICAL", criticals),
        ("WARNINGS", warnings),
        ("INFO",     infos),
    ]:
        if not group:
            continue
        lines.append(f"\n  {group_label} ({len(group)})")
        lines.append("  " + "-" * 40)
        for v in group:
            sym = _severity_symbol(v.severity)
            lines.append(f"  {sym} [{v.category}] {v.rule}: {v.message}")
            if fix and v.suggestion:
                lines.append(f"       --> Suggested: {v.suggestion}")

    if not result.violations:
        lines.append("\n  No violations found. Pack naming looks healthy!")

    lines.append("\n" + "=" * 60)
    grade = "A" if score >= 90 else "B" if score >= 75 else "C" if score >= 60 else "D" if score >= 40 else "F"
    lines.append(f"  Naming Health: {score}/100  (Grade: {grade})")
    lines.append("=" * 60)
    return "\n".join(lines)


def format_json(result: ValidationResult, fix: bool) -> str:
    payload = {
        "pack_name": result.pack_name,
        "counts": {
            "programs": len(result.program_names),
            "samples": len(result.sample_names),
            "presets": len(result.preset_names),
        },
        "score": result.score(),
        "violations": [
            {
                "severity": v.severity,
                "category": v.category,
                "name": v.name,
                "rule": v.rule,
                "message": v.message,
                **({"suggestion": v.suggestion} if fix and v.suggestion else {}),
            }
            for v in result.violations
        ],
    }
    return json.dumps(payload, indent=2)


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        description="Validate XO_OX brand naming conventions in an XPN pack directory.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    p.add_argument("pack_dir", type=Path, help="Path to the XPN pack directory.")
    p.add_argument(
        "--fix-suggestions",
        action="store_true",
        help="Propose corrected names for each violation.",
    )
    p.add_argument(
        "--strict",
        action="store_true",
        help="Treat INFO-level issues as warnings and warnings as criticals.",
    )
    p.add_argument(
        "--format",
        choices=["text", "json"],
        default="text",
        help="Output format (default: text).",
    )
    return p


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()

    pack_dir: Path = args.pack_dir.resolve()
    if not pack_dir.is_dir():
        print(f"ERROR: '{pack_dir}' is not a directory.", file=sys.stderr)
        return 1

    result = scan_pack(pack_dir, fix=args.fix_suggestions, strict=args.strict)

    if args.strict:
        # Escalate severities
        for v in result.violations:
            if v.severity == SEVERITY_INFO:
                v.severity = SEVERITY_WARNING
            elif v.severity == SEVERITY_WARNING:
                v.severity = SEVERITY_CRITICAL

    if args.format == "json":
        print(format_json(result, args.fix_suggestions))
    else:
        print(format_text(result, args.fix_suggestions))

    # Exit code: 0 = healthy (score >= 75), 1 = needs attention
    return 0 if result.score() >= 75 else 1


if __name__ == "__main__":
    sys.exit(main())
