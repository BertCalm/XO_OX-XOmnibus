#!/usr/bin/env python3
"""
xpn_oxport_pack_yml_template.py — Generate pack.yml config for oxport v2 pipeline.

Usage:
  python xpn_oxport_pack_yml_template.py [--from-dir <preset_dir>] [--from-spec <spec.json>]
         [--name "Pack Name"] [--version 1.0.0] [--description "..."]
         [--engines OPAL,ONSET] [--mood Foundation] [--output pack.yml]
"""

import argparse
import json
import sys
from pathlib import Path
from typing import Optional

# ---------------------------------------------------------------------------
# Known XOlokun engines (canonical short names)
# ---------------------------------------------------------------------------
KNOWN_ENGINES = {
    "ODDFELIX", "ODDOSCAR", "OVERDUB", "ODYSSEY", "OBLONG", "OBESE",
    "ONSET", "OVERWORLD", "OPAL", "ORBITAL", "ORGANON", "OUROBOROS",
    "OBSIDIAN", "ORIGAMI", "ORACLE", "OBSCURA", "OCEANIC", "OCELOT",
    "OVERBITE", "OPTIC", "OBLIQUE", "OSPREY", "OSTERIA", "OWLFISH",
    "OHM", "ORPHICA", "OBBLIGATO", "OTTONI", "OLE", "OMBRE", "ORCA",
    "OCTOPUS", "OVERLAP", "OUTWIT",
    # V1 concept engines (no source yet)
    "OSTINATO", "OPENSKY", "OCEANDEEP", "OUIE",
}

VALID_MOODS = {
    "Foundation", "Atmosphere", "Entangled", "Prism", "Flux", "Aether", "Family"
}

DNA_DEFAULTS = {
    "brightness": 0.5,
    "warmth": 0.5,
    "movement": 0.5,
    "density": 0.5,
    "space": 0.5,
    "aggression": 0.3,
}

DNA_KEYS = list(DNA_DEFAULTS.keys())

# ---------------------------------------------------------------------------
# DNA centroid computation
# ---------------------------------------------------------------------------

def _parse_dna_from_preset(data: dict) -> Optional[dict]:
    """Extract a DNA dict from a parsed .xometa preset dict, or return None."""
    dna = data.get("sonicDNA") or data.get("sonic_dna") or data.get("dna")
    if not isinstance(dna, dict):
        return None
    result = {}
    for key in DNA_KEYS:
        val = dna.get(key)
        if isinstance(val, (int, float)):
            result[key] = float(val)
    if len(result) == len(DNA_KEYS):
        return result
    return None


def compute_dna_centroid(preset_files: list[Path]) -> dict:
    """Average DNA across all loadable presets. Falls back to defaults."""
    accum = {k: 0.0 for k in DNA_KEYS}
    count = 0
    for path in preset_files:
        try:
            data = json.loads(path.read_text(encoding="utf-8"))
            dna = _parse_dna_from_preset(data)
            if dna:
                for k in DNA_KEYS:
                    accum[k] += dna[k]
                count += 1
        except Exception:
            pass
    if count == 0:
        return dict(DNA_DEFAULTS)
    return {k: round(accum[k] / count, 3) for k in DNA_KEYS}


# ---------------------------------------------------------------------------
# Engine detection from preset directory
# ---------------------------------------------------------------------------

def detect_engines_from_dir(preset_dir: Path) -> list[str]:
    """Scan .xometa files and collect all referenced engine names."""
    found: set[str] = set()
    for path in preset_dir.rglob("*.xometa"):
        try:
            data = json.loads(path.read_text(encoding="utf-8"))
            engines_field = data.get("engines") or []
            if isinstance(engines_field, list):
                for e in engines_field:
                    name = str(e).upper()
                    if name in KNOWN_ENGINES:
                        found.add(name)
        except Exception:
            pass
    return sorted(found)


# ---------------------------------------------------------------------------
# YAML template generation (plain string — no yaml module)
# ---------------------------------------------------------------------------

def _yaml_list(items: list[str]) -> str:
    if not items:
        return "[]"
    return "[" + ", ".join(items) + "]"


def _yaml_str(value: str) -> str:
    """Quote a string value for YAML."""
    escaped = value.replace('"', '\\"')
    return f'"{escaped}"'


def render_pack_yml(
    name: str,
    version: str,
    description: str,
    tags: list[str],
    author: str,
    primary_engines: list[str],
    secondary_engines: list[str],
    preset_dir: str,
    engine_filter: list[str],
    output_dir: str,
    dna: dict,
) -> str:
    lines = [
        "pack:",
        f'  name: {_yaml_str(name)}',
        f'  version: {_yaml_str(version)}',
        f'  description: {_yaml_str(description)}',
        f'  tags: {_yaml_list(tags)}',
        f'  author: {_yaml_str(author)}',
        "",
        "engines:",
        f'  primary: {_yaml_list(primary_engines)}',
        f'  secondary: {_yaml_list(secondary_engines)}',
        "",
        "stages:",
        "  render_spec:",
        "    enabled: true",
        f'    preset_dir: {_yaml_str(preset_dir)}',
        f'    engine_filter: {_yaml_list(engine_filter)}',
        "  categorize:",
        "    enabled: true",
        "  expand:",
        "    enabled: true",
        "    target_count: 150",
        "  qa:",
        "    enabled: true",
        "    strict: false",
        "  export:",
        "    enabled: true",
        f'    output_dir: {_yaml_str(output_dir)}',
        "  cover_art:",
        "    enabled: true",
        '    style: "gallery_model"',
        "  package:",
        "    enabled: true",
        '    format: "xpn"',
        "",
        "dna_target:",
    ]
    for key in DNA_KEYS:
        lines.append(f"  {key}: {dna[key]}")
    lines.append("")  # trailing newline
    return "\n".join(lines)


# ---------------------------------------------------------------------------
# Validation
# ---------------------------------------------------------------------------

def validate_pack_yml(content: str) -> list[str]:
    """Lightweight structural check — returns list of error strings."""
    errors = []
    required_sections = ["pack:", "engines:", "stages:", "dna_target:"]
    required_pack_keys = ["name:", "version:", "description:"]
    required_stages = [
        "render_spec:", "categorize:", "expand:", "qa:", "export:",
        "cover_art:", "package:",
    ]
    for section in required_sections:
        if section not in content:
            errors.append(f"Missing required section: {section}")
    for key in required_pack_keys:
        if key not in content:
            errors.append(f"Missing required pack key: {key}")
    for stage in required_stages:
        if stage not in content:
            errors.append(f"Missing required stage: {stage}")
    for key in DNA_KEYS:
        if f"  {key}:" not in content:
            errors.append(f"Missing dna_target key: {key}")
    return errors


# ---------------------------------------------------------------------------
# --from-dir handler
# ---------------------------------------------------------------------------

def config_from_dir(preset_dir: Path, overrides: dict) -> dict:
    if not preset_dir.exists():
        print(f"ERROR: preset_dir does not exist: {preset_dir}", file=sys.stderr)
        sys.exit(1)

    preset_files = list(preset_dir.rglob("*.xometa"))
    engines = detect_engines_from_dir(preset_dir)
    dna = compute_dna_centroid(preset_files)

    # Derive a sensible pack name from the directory name if not overridden
    default_name = preset_dir.name.replace("_", " ").replace("-", " ").title()
    # Detect mood from path components
    mood_tag = None
    for part in preset_dir.parts:
        if part in VALID_MOODS:
            mood_tag = part
            break

    tags = []
    if mood_tag:
        tags.append(mood_tag.lower())
    for e in engines:
        tags.append(e.lower())

    return {
        "name": overrides.get("name") or default_name,
        "version": overrides.get("version") or "1.0.0",
        "description": overrides.get("description") or f"XO_OX pack generated from {preset_dir.name}",
        "tags": tags,
        "author": "XO_OX",
        "primary_engines": overrides.get("engines") or engines,
        "secondary_engines": [],
        "preset_dir": str(preset_dir),
        "engine_filter": overrides.get("engines") or engines,
        "output_dir": "build/packs",
        "dna": dna,
    }


# ---------------------------------------------------------------------------
# --from-spec handler
# ---------------------------------------------------------------------------

def config_from_spec(spec_path: Path, overrides: dict) -> dict:
    if not spec_path.exists():
        print(f"ERROR: spec file not found: {spec_path}", file=sys.stderr)
        sys.exit(1)

    try:
        spec = json.loads(spec_path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as exc:
        print(f"ERROR: invalid JSON in spec file: {exc}", file=sys.stderr)
        sys.exit(1)

    # Accept flat or nested spec structures
    pack = spec.get("pack") or spec
    raw_engines = pack.get("engines") or spec.get("engines") or []
    if isinstance(raw_engines, dict):
        primary = raw_engines.get("primary") or []
        secondary = raw_engines.get("secondary") or []
    elif isinstance(raw_engines, list):
        primary = raw_engines
        secondary = []
    else:
        primary = []
        secondary = []

    primary = [e.upper() for e in primary]
    secondary = [e.upper() for e in secondary]

    dna_raw = spec.get("dna_target") or spec.get("dna") or {}
    dna = {}
    for key in DNA_KEYS:
        val = dna_raw.get(key)
        dna[key] = round(float(val), 3) if isinstance(val, (int, float)) else DNA_DEFAULTS[key]

    preset_dir = pack.get("preset_dir") or spec.get("preset_dir") or "Presets/XOlokun/Foundation"
    output_dir = pack.get("output_dir") or spec.get("output_dir") or "build/packs"
    tags_raw = pack.get("tags") or spec.get("tags") or []

    return {
        "name": overrides.get("name") or pack.get("name") or spec.get("name") or "Unnamed Pack",
        "version": overrides.get("version") or pack.get("version") or spec.get("version") or "1.0.0",
        "description": overrides.get("description") or pack.get("description") or spec.get("description") or "",
        "tags": tags_raw,
        "author": pack.get("author") or spec.get("author") or "XO_OX",
        "primary_engines": overrides.get("engines") or primary,
        "secondary_engines": secondary,
        "preset_dir": preset_dir,
        "engine_filter": overrides.get("engines") or primary,
        "output_dir": output_dir,
        "dna": dna,
    }


# ---------------------------------------------------------------------------
# Default config (no --from-dir or --from-spec)
# ---------------------------------------------------------------------------

def config_defaults(overrides: dict) -> dict:
    engines = overrides.get("engines") or []
    return {
        "name": overrides.get("name") or "My XO_OX Pack",
        "version": overrides.get("version") or "1.0.0",
        "description": overrides.get("description") or "An XO_OX pack for the MPC.",
        "tags": [e.lower() for e in engines] if engines else ["xo_ox"],
        "author": "XO_OX",
        "primary_engines": engines,
        "secondary_engines": [],
        "preset_dir": "Presets/XOlokun/Foundation",
        "engine_filter": engines,
        "output_dir": "build/packs",
        "dna": dict(DNA_DEFAULTS),
    }


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def parse_engines(raw: str) -> list[str]:
    return [e.strip().upper() for e in raw.split(",") if e.strip()]


def main():
    parser = argparse.ArgumentParser(
        description="Generate a pack.yml config for the oxport v2 pipeline."
    )
    parser.add_argument("--from-dir", metavar="DIR",
                        help="Scan a preset directory to auto-detect engines and DNA centroid.")
    parser.add_argument("--from-spec", metavar="SPEC_JSON",
                        help="Read a pack spec JSON and generate pack.yml.")
    parser.add_argument("--name", metavar="NAME", help="Override pack name.")
    parser.add_argument("--version", metavar="VERSION", default=None,
                        help="Override version string (default: 1.0.0).")
    parser.add_argument("--description", metavar="DESC", help="Override description.")
    parser.add_argument("--engines", metavar="E1,E2",
                        help="Comma-separated engine list override, e.g. OPAL,ONSET.")
    parser.add_argument("--mood", metavar="MOOD",
                        help="Mood hint (Foundation/Atmosphere/etc.) — used as a tag.")
    parser.add_argument("--output", metavar="FILE", default="pack.yml",
                        help="Output file path (default: pack.yml).")
    parser.add_argument("--dry-run", action="store_true",
                        help="Print generated YAML to stdout, do not write file.")

    args = parser.parse_args()

    overrides: dict = {}
    if args.name:
        overrides["name"] = args.name
    if args.version:
        overrides["version"] = args.version
    if args.description:
        overrides["description"] = args.description
    if args.engines:
        overrides["engines"] = parse_engines(args.engines)

    # Build config dict
    if args.from_dir:
        cfg = config_from_dir(Path(args.from_dir), overrides)
    elif args.from_spec:
        cfg = config_from_spec(Path(args.from_spec), overrides)
    else:
        cfg = config_defaults(overrides)

    # Inject mood tag if supplied
    if args.mood:
        mood = args.mood.strip().title()
        if mood not in VALID_MOODS:
            print(f"WARNING: '{mood}' is not a recognised mood. Valid: {', '.join(sorted(VALID_MOODS))}",
                  file=sys.stderr)
        if mood.lower() not in cfg["tags"]:
            cfg["tags"].insert(0, mood.lower())

    # Render YAML
    content = render_pack_yml(
        name=cfg["name"],
        version=cfg["version"],
        description=cfg["description"],
        tags=cfg["tags"],
        author=cfg["author"],
        primary_engines=cfg["primary_engines"],
        secondary_engines=cfg["secondary_engines"],
        preset_dir=cfg["preset_dir"],
        engine_filter=cfg["engine_filter"],
        output_dir=cfg["output_dir"],
        dna=cfg["dna"],
    )

    # Validate
    errors = validate_pack_yml(content)
    if errors:
        print("VALIDATION ERRORS:", file=sys.stderr)
        for err in errors:
            print(f"  - {err}", file=sys.stderr)
        sys.exit(1)

    # Output
    if args.dry_run:
        print(content)
    else:
        out_path = Path(args.output)
        out_path.write_text(content, encoding="utf-8")
        print(f"pack.yml written to: {out_path.resolve()}")
        print(f"  Pack:    {cfg['name']} v{cfg['version']}")
        print(f"  Engines: {', '.join(cfg['primary_engines']) or '(none)'}")
        print(f"  Stages:  render_spec → categorize → expand → qa → export → cover_art → package")


if __name__ == "__main__":
    main()
