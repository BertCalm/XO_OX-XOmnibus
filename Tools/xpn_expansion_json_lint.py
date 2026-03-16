#!/usr/bin/env python3
"""
xpn_expansion_json_lint.py — XO_OX Designs
Lints expansion.json files: validates required fields, format compliance,
and suggests improvements.

Accepts a standalone expansion.json file or a .xpn ZIP archive.

Usage:
    python xpn_expansion_json_lint.py expansion.json [--format text|json] [--strict]
    python xpn_expansion_json_lint.py MyPack.xpn    [--format text|json] [--strict]

Exit codes:
    0  — clean (or warnings only without --strict)
    1  — warnings present (only with --strict)
    2  — errors present
"""

import argparse
import json
import re
import sys
import zipfile
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any, List, Optional

# ---------------------------------------------------------------------------
# CONSTANTS
# ---------------------------------------------------------------------------

CANONICAL_ENGINES = {
    "ODDFELIX", "ODDOSCAR", "OVERDUB", "ODYSSEY", "OBLONG", "OBESE",
    "ONSET", "OVERWORLD", "OPAL", "ORBITAL", "ORGANON", "OUROBOROS",
    "OBSIDIAN", "OVERBITE", "ORIGAMI", "ORACLE", "OBSCURA", "OCEANIC",
    "OCELOT", "OPTIC", "OBLIQUE", "OSPREY", "OSTERIA", "OWLFISH",
    "OHM", "ORPHICA", "OBBLIGATO", "OTTONI", "OLE", "OMBRE", "ORCA",
    "OCTOPUS", "OSTINATO", "OPENSKY", "OCEANDEEP", "OUIE",
    "OVERLAP", "OUTWIT",
}

VALID_MOODS = {"Foundation", "Atmosphere", "Entangled", "Prism", "Flux", "Aether", "Family"}

SONIC_DNA_KEYS = {"brightness", "warmth", "movement", "density", "space", "aggression"}

SEMVER_RE = re.compile(r"^\d+\.\d+\.\d+$")

VALID_KEYS = {
    "C", "C#", "Db", "D", "D#", "Eb", "E", "F", "F#", "Gb",
    "G", "G#", "Ab", "A", "A#", "Bb", "B",
}


# ---------------------------------------------------------------------------
# FINDING MODEL
# ---------------------------------------------------------------------------

@dataclass
class Finding:
    level: str       # "ERROR" or "WARNING"
    field: str
    message: str

    def __str__(self) -> str:
        return f"  [{self.level}] {self.field}: {self.message}"


# ---------------------------------------------------------------------------
# LOADER
# ---------------------------------------------------------------------------

def load_expansion_json(path: Path) -> dict:
    """Load expansion.json from a standalone file or from inside a .xpn ZIP."""
    suffix = path.suffix.lower()
    if suffix == ".xpn":
        if not zipfile.is_zipfile(path):
            raise ValueError(f"{path} is not a valid ZIP / .xpn archive")
        with zipfile.ZipFile(path, "r") as zf:
            names = zf.namelist()
            candidates = [n for n in names if n.endswith("expansion.json")]
            if not candidates:
                raise FileNotFoundError("No expansion.json found inside the .xpn archive")
            # Prefer top-level over nested
            candidates.sort(key=lambda n: n.count("/"))
            with zf.open(candidates[0]) as f:
                return json.load(f)
    else:
        with open(path, "r", encoding="utf-8") as f:
            return json.load(f)


# ---------------------------------------------------------------------------
# LINT CHECKS
# ---------------------------------------------------------------------------

def lint(data: dict) -> List[Finding]:
    findings: List[Finding] = []

    def err(fld: str, msg: str) -> None:
        findings.append(Finding("ERROR", fld, msg))

    def warn(fld: str, msg: str) -> None:
        findings.append(Finding("WARNING", fld, msg))

    # --- Required: Name ---
    name = data.get("Name")
    if name is None:
        err("Name", "Missing required field")
    elif not isinstance(name, str) or not name.strip():
        err("Name", "Must be a non-empty string")
    else:
        if len(name) > 40:
            err("Name", f"Exceeds 40-char limit (got {len(name)})")

    # --- Required: Engine ---
    engine = data.get("Engine")
    if engine is None:
        err("Engine", "Missing required field")
    elif not isinstance(engine, str):
        err("Engine", "Must be a string")
    elif engine.upper() not in CANONICAL_ENGINES:
        err("Engine", f"'{engine}' is not a canonical engine name. "
            f"Valid engines: {', '.join(sorted(CANONICAL_ENGINES))}")
    else:
        engine = engine.upper()  # normalise for downstream consistency checks

    # --- Required: Version ---
    version = data.get("Version")
    if version is None:
        err("Version", "Missing required field")
    elif not isinstance(version, str):
        err("Version", "Must be a string")
    elif not SEMVER_RE.match(version):
        err("Version", f"Must match semver X.Y.Z (got '{version}')")

    # --- Required: sonic_dna ---
    dna = data.get("sonic_dna")
    if dna is None:
        err("sonic_dna", "Missing required field")
    elif not isinstance(dna, dict):
        err("sonic_dna", "Must be an object")
    else:
        missing_keys = SONIC_DNA_KEYS - set(dna.keys())
        if missing_keys:
            err("sonic_dna", f"Missing required dimensions: {', '.join(sorted(missing_keys))}")
        extra_keys = set(dna.keys()) - SONIC_DNA_KEYS
        if extra_keys:
            warn("sonic_dna", f"Unknown dimensions (will be ignored): {', '.join(sorted(extra_keys))}")
        for k in SONIC_DNA_KEYS:
            if k in dna:
                v = dna[k]
                if not isinstance(v, (int, float)):
                    err(f"sonic_dna.{k}", "Must be a number")
                elif not (0.0 <= float(v) <= 1.0):
                    err(f"sonic_dna.{k}", f"Value {v} out of range [0.0, 1.0]")

    # --- Recommended: Mood ---
    mood = data.get("Mood")
    if mood is None:
        warn("Mood", "Recommended field missing. Valid values: " + ", ".join(sorted(VALID_MOODS)))
    elif mood not in VALID_MOODS:
        warn("Mood", f"'{mood}' is not a recognised mood. Valid: {', '.join(sorted(VALID_MOODS))}")

    # --- Recommended: Tags ---
    tags = data.get("Tags")
    if tags is None:
        warn("Tags", "Recommended field missing (array of 1-10 strings)")
    elif not isinstance(tags, list):
        warn("Tags", "Must be an array")
    else:
        if len(tags) == 0:
            warn("Tags", "Array is empty — at least 1 tag recommended")
        elif len(tags) > 10:
            warn("Tags", f"Too many tags ({len(tags)}); keep to 10 or fewer")
        seen: set = set()
        for i, tag in enumerate(tags):
            if not isinstance(tag, str):
                warn(f"Tags[{i}]", "Each tag must be a string")
                continue
            if len(tag) > 20:
                warn(f"Tags[{i}]", f"Tag '{tag}' exceeds 20-char limit ({len(tag)})")
            tag_lower = tag.lower()
            if tag_lower in seen:
                warn("Tags", f"Duplicate tag '{tag}' (case-insensitive)")
            seen.add(tag_lower)

    # --- Recommended: Description ---
    description = data.get("Description")
    if description is None:
        warn("Description", "Recommended field missing (20-200 chars)")
    elif not isinstance(description, str):
        warn("Description", "Must be a string")
    else:
        length = len(description)
        if length < 20:
            warn("Description", f"Too short ({length} chars); aim for 20-200")
        elif length > 200:
            warn("Description", f"Too long ({length} chars); keep to 200 or fewer")

    # --- Optional: BPM (validate if present) ---
    bpm = data.get("BPM")
    if bpm is not None:
        if not isinstance(bpm, int) or isinstance(bpm, bool):
            warn("BPM", "Must be an integer")
        elif not (60 <= bpm <= 300):
            warn("BPM", f"Value {bpm} outside expected range [60, 300]")

    # --- Optional: Key (validate if present) ---
    key = data.get("Key")
    if key is not None:
        if not isinstance(key, str):
            warn("Key", "Must be a string (e.g. 'C', 'F#', 'Bb')")
        elif key not in VALID_KEYS:
            warn("Key", f"'{key}' is not a recognised musical key. "
                 f"Valid: {', '.join(sorted(VALID_KEYS))}")

    # --- Consistency: Engine mentioned in Name ---
    if isinstance(name, str) and isinstance(engine, str):
        name_upper = name.upper()
        if engine in name_upper:
            # Engine IS in name — check it's the same one declared
            # (Always consistent in this branch; just informational — no warning needed)
            pass
        # Check if a DIFFERENT engine name appears in the pack name (potential mislabel)
        for eng in CANONICAL_ENGINES:
            if eng != engine and eng in name_upper:
                warn("Name/Engine",
                     f"Pack name '{name}' mentions '{eng}' but Engine is '{engine}'. "
                     "Possible mislabel?")
                break

    # --- Consistency: DNA brightness + warmth both > 0.85 ---
    if isinstance(dna, dict):
        brightness = dna.get("brightness")
        warmth = dna.get("warmth")
        if (isinstance(brightness, (int, float)) and isinstance(warmth, (int, float))
                and float(brightness) > 0.85 and float(warmth) > 0.85):
            warn("sonic_dna",
                 f"brightness ({brightness}) and warmth ({warmth}) are both > 0.85 — "
                 "physically inconsistent; verify intentional")

        movement = dna.get("movement")
        aggression = dna.get("aggression")
        if (isinstance(movement, (int, float)) and isinstance(aggression, (int, float))
                and float(movement) < 0.1 and float(aggression) > 0.8):
            warn("sonic_dna",
                 f"movement ({movement}) < 0.1 with aggression ({aggression}) > 0.8 — "
                 "probably wrong; static aggressive textures are unusual")

    return findings


# ---------------------------------------------------------------------------
# OUTPUT FORMATTERS
# ---------------------------------------------------------------------------

def format_text(path: Path, findings: List[Finding]) -> str:
    errors = [f for f in findings if f.level == "ERROR"]
    warnings = [f for f in findings if f.level == "WARNING"]
    lines = [f"Linting: {path}"]
    if not findings:
        lines.append("  OK — no issues found")
    else:
        if errors:
            lines.append(f"  {len(errors)} error(s):")
            lines.extend(str(f) for f in errors)
        if warnings:
            lines.append(f"  {len(warnings)} warning(s):")
            lines.extend(str(f) for f in warnings)
    lines.append("")
    return "\n".join(lines)


def format_json(path: Path, findings: List[Finding]) -> str:
    return json.dumps({
        "file": str(path),
        "errors": [{"field": f.field, "message": f.message}
                   for f in findings if f.level == "ERROR"],
        "warnings": [{"field": f.field, "message": f.message}
                     for f in findings if f.level == "WARNING"],
    }, indent=2)


# ---------------------------------------------------------------------------
# MAIN
# ---------------------------------------------------------------------------

def main() -> int:
    parser = argparse.ArgumentParser(
        description="Lint expansion.json (standalone or inside .xpn ZIP)"
    )
    parser.add_argument("file", help="Path to expansion.json or .xpn archive")
    parser.add_argument(
        "--format", choices=["text", "json"], default="text",
        help="Output format (default: text)"
    )
    parser.add_argument(
        "--strict", action="store_true",
        help="Exit 1 on WARNING, exit 2 on ERROR (default: only exit 2 on ERROR)"
    )
    args = parser.parse_args()

    path = Path(args.file)
    if not path.exists():
        print(f"ERROR: File not found: {path}", file=sys.stderr)
        return 2

    try:
        data = load_expansion_json(path)
    except (json.JSONDecodeError, ValueError, FileNotFoundError) as exc:
        print(f"ERROR: Could not load expansion.json — {exc}", file=sys.stderr)
        return 2

    findings = lint(data)

    if args.format == "json":
        print(format_json(path, findings))
    else:
        print(format_text(path, findings))

    has_errors = any(f.level == "ERROR" for f in findings)
    has_warnings = any(f.level == "WARNING" for f in findings)

    if has_errors:
        return 2
    if has_warnings and args.strict:
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
