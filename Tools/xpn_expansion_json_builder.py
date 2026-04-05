#!/usr/bin/env python3
"""
xpn_expansion_json_builder.py — Build and validate expansion.json for XPN packs.

expansion.json is the primary MPC discovery manifest — the first file MPC reads
when loading an expansion. This tool builds, validates, and merges that file.

Usage:
  python xpn_expansion_json_builder.py --name "XObese" --version "1.0.0" --output expansion.json
  python xpn_expansion_json_builder.py --interactive --output expansion.json
  python xpn_expansion_json_builder.py --from-presets ./Presets --output expansion.json
  python xpn_expansion_json_builder.py --validate expansion.json
  python xpn_expansion_json_builder.py --merge existing.json --name "New Name" --output expansion.json
"""

import argparse
import json
import sys
from pathlib import Path
from typing import Dict, List, Optional

# ── Schema constants ────────────────────────────────────────────────────────

REQUIRED_FIELDS = ["name", "version", "description", "author", "genre", "type", "tier", "compatibilityVersion"]
OPTIONAL_FIELDS = ["tags", "artworkFile"]
ALL_FIELDS = REQUIRED_FIELDS + OPTIONAL_FIELDS

VALID_TYPES = ["Expansion", "Sample", "Instrument"]
VALID_TIERS = ["free", "premium", "exclusive"]
VALID_COMPAT_VERSIONS = ["2.0", "3.0", "3.x"]

DEFAULTS = {
    "author": "XO_OX",
    "genre": "Electronic",
    "type": "Expansion",
    "tier": "premium",
    "compatibilityVersion": "3.0",
    "tags": [],
    "artworkFile": "cover.png",
}

FIELD_DESCRIPTIONS = {
    "name": "Pack name (e.g. XObese)",
    "version": "Version string (e.g. 1.0.0)",
    "description": "Short description of the pack",
    "author": f"Author name [default: {DEFAULTS['author']}]",
    "genre": f"Primary genre [default: {DEFAULTS['genre']}]",
    "tags": "Comma-separated tags (e.g. bass,808,dark)",
    "type": f"Pack type — {VALID_TYPES} [default: {DEFAULTS['type']}]",
    "tier": f"Distribution tier — {VALID_TIERS} [default: {DEFAULTS['tier']}]",
    "compatibilityVersion": f"MPC compat version — {VALID_COMPAT_VERSIONS} [default: {DEFAULTS['compatibilityVersion']}]",
    "artworkFile": f"Artwork filename [default: {DEFAULTS['artworkFile']}]",
}


# ── Validation ───────────────────────────────────────────────────────────────

def validate_expansion_json(data: Dict) -> List[str]:
    """Return a list of error strings; empty list means valid."""
    errors = []

    for field in REQUIRED_FIELDS:
        if field not in data or data[field] in (None, ""):
            errors.append(f"Missing required field: '{field}'")

    if "type" in data and data["type"] not in VALID_TYPES:
        errors.append(f"Invalid type '{data['type']}' — must be one of {VALID_TYPES}")

    if "tier" in data and data["tier"] not in VALID_TIERS:
        errors.append(f"Invalid tier '{data['tier']}' — must be one of {VALID_TIERS}")

    if "compatibilityVersion" in data and data["compatibilityVersion"] not in VALID_COMPAT_VERSIONS:
        errors.append(
            f"Invalid compatibilityVersion '{data['compatibilityVersion']}' — "
            f"must be one of {VALID_COMPAT_VERSIONS}"
        )

    if "tags" in data and not isinstance(data["tags"], list):
        errors.append("Field 'tags' must be a JSON array")

    if "version" in data:
        parts = str(data["version"]).split(".")
        if not all(p.isdigit() for p in parts) or len(parts) < 2:
            errors.append(f"Version '{data['version']}' should be semver (e.g. 1.0.0)")

    unknown = [k for k in data if k not in ALL_FIELDS]
    if unknown:
        errors.append(f"Unknown fields (will be preserved): {unknown}")

    return errors


# ── Preset directory scan ─────────────────────────────────────────────────

def _extract_from_xometa(path: Path) -> dict:
    """Parse a .xometa JSON file and extract useful expansion fields."""
    try:
        with path.open() as f:
            meta = json.load(f)
    except Exception as exc:
        print(f"[WARN] {exc}", file=sys.stderr)
        return {}
    result = {}
    if "engine" in meta:
        result["engine"] = meta["engine"]
    if "genre" in meta:
        result["genre"] = meta["genre"]
    if "tags" in meta and isinstance(meta["tags"], list):
        result["tags"] = meta["tags"]
    if "mood" in meta:
        result["mood"] = meta["mood"]
    return result


def scan_presets_dir(presets_dir: Path) -> Dict:
    """
    Walk a preset directory tree, read .xometa files, and synthesise
    expansion.json field suggestions.
    """
    xometa_files = list(presets_dir.rglob("*.xometa"))
    if not xometa_files:
        print(f"[warn] No .xometa files found under {presets_dir}", file=sys.stderr)

    engines: set[str] = set()
    genres: set[str] = set()
    tags: set[str] = set()
    moods: set[str] = set()

    for f in xometa_files:
        meta = _extract_from_xometa(f)
        if "engine" in meta:
            engines.add(meta["engine"])
        if "genre" in meta:
            genres.add(meta["genre"])
        if "tags" in meta:
            tags.update(meta["tags"])
        if "mood" in meta:
            moods.add(meta["mood"])

    print(f"[info] Scanned {len(xometa_files)} .xometa files in {presets_dir}", file=sys.stderr)
    if engines:
        print(f"[info] Engines detected: {sorted(engines)}", file=sys.stderr)

    suggested: dict = {}
    if genres:
        suggested["genre"] = sorted(genres)[0]           # pick first alphabetically
    if tags:
        suggested["tags"] = sorted(tags)
    if engines:
        # Use engines as additional tags if not already present
        engine_tags = [e.lower() for e in engines]
        existing = suggested.get("tags", [])
        suggested["tags"] = sorted(set(existing) | set(engine_tags))

    return suggested


# ── Interactive prompt ────────────────────────────────────────────────────

def _prompt(field: str, default=None) -> str:
    desc = FIELD_DESCRIPTIONS.get(field, field)
    if default is not None:
        prompt_str = f"  {desc} [{default}]: "
    else:
        prompt_str = f"  {desc}: "
    try:
        value = input(prompt_str).strip()
    except (KeyboardInterrupt, EOFError):
        print("\n[abort] Interactive mode cancelled.", file=sys.stderr)
        sys.exit(1)
    return value if value else (str(default) if default is not None else "")


def interactive_build(base: Optional[Dict] = None) -> Dict:
    """Prompt the user for each field, pre-filling from base if provided."""
    print("\nexpansion.json interactive builder — press Enter to accept [default]\n")
    data = dict(base or {})

    for field in REQUIRED_FIELDS:
        default = data.get(field) or DEFAULTS.get(field)
        value = _prompt(field, default)
        if not value and default is not None:
            value = str(default)
        data[field] = value

    # Optional: tags
    existing_tags = data.get("tags", DEFAULTS["tags"])
    tag_default = ",".join(existing_tags) if existing_tags else ""
    raw_tags = _prompt("tags", tag_default or None)
    data["tags"] = [t.strip() for t in raw_tags.split(",") if t.strip()] if raw_tags else []

    # Optional: artworkFile
    artwork_default = data.get("artworkFile", DEFAULTS["artworkFile"])
    artwork = _prompt("artworkFile", artwork_default)
    data["artworkFile"] = artwork if artwork else artwork_default

    return data


# ── Build from CLI args ───────────────────────────────────────────────────

def build_from_args(args: argparse.Namespace, base: Optional[Dict] = None) -> Dict:
    """Construct expansion dict from CLI args, merging into base if provided."""
    data = dict(base or {})

    field_map = {
        "name": args.name,
        "version": args.version,
        "description": args.description,
        "author": args.author,
        "genre": args.genre,
        "type": args.type,
        "tier": args.tier,
        "compatibilityVersion": args.compat,
        "artworkFile": args.artwork,
    }
    for field, value in field_map.items():
        if value is not None:
            data[field] = value

    if args.tags is not None:
        data["tags"] = [t.strip() for t in args.tags.split(",") if t.strip()]

    # Fill in schema defaults for any field still missing
    for field, default in DEFAULTS.items():
        if field not in data:
            data[field] = default

    return data


# ── Output helpers ────────────────────────────────────────────────────────

def write_output(data: Dict, output_path: Optional[Path]) -> None:
    text = json.dumps(data, indent=2)
    if output_path:
        output_path.write_text(text + "\n")
        print(f"[ok] Written to {output_path}", file=sys.stderr)
    else:
        print(text)


def print_validation_report(errors: List[str], path: str = "") -> bool:
    label = f" ({path})" if path else ""
    if not errors:
        print(f"[ok] expansion.json{label} is valid.")
        return True
    print(f"[fail] expansion.json{label} has {len(errors)} issue(s):")
    for e in errors:
        print(f"  • {e}")
    return False


# ── CLI ───────────────────────────────────────────────────────────────────

def build_arg_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        prog="xpn_expansion_json_builder.py",
        description="Build, validate, and merge expansion.json for XPN/MPC packs.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python xpn_expansion_json_builder.py --name "XObese" --version "1.0.0" \\
      --description "808s and sub-bass heaven" --output expansion.json

  python xpn_expansion_json_builder.py --interactive --output expansion.json

  python xpn_expansion_json_builder.py --from-presets ./Presets \\
      --name "XObese" --version "1.0.0" --output expansion.json

  python xpn_expansion_json_builder.py --validate expansion.json

  python xpn_expansion_json_builder.py --merge expansion.json \\
      --version "1.1.0" --output expansion.json
""",
    )
    p.add_argument("--name", help="Pack name")
    p.add_argument("--version", help="Version string (e.g. 1.0.0)")
    p.add_argument("--description", help="Short description")
    p.add_argument("--author", help=f"Author [default: {DEFAULTS['author']}]")
    p.add_argument("--genre", help=f"Primary genre [default: {DEFAULTS['genre']}]")
    p.add_argument("--tags", help="Comma-separated tags")
    p.add_argument("--type", choices=VALID_TYPES, help=f"Pack type [default: {DEFAULTS['type']}]")
    p.add_argument("--tier", choices=VALID_TIERS, help=f"Distribution tier [default: {DEFAULTS['tier']}]")
    p.add_argument("--compat", metavar="VER", help=f"compatibilityVersion [default: {DEFAULTS['compatibilityVersion']}]")
    p.add_argument("--artwork", help=f"Artwork filename [default: {DEFAULTS['artworkFile']}]")

    modes = p.add_argument_group("modes (mutually exclusive)")
    mx = modes.add_mutually_exclusive_group()
    mx.add_argument("--interactive", action="store_true", help="Prompt for each field interactively")
    mx.add_argument("--from-presets", metavar="DIR", help="Auto-detect fields from .xometa files in DIR")
    mx.add_argument("--validate", metavar="FILE", help="Validate an existing expansion.json file")
    mx.add_argument("--merge", metavar="FILE", help="Load existing file and update specified fields")

    p.add_argument("--output", "-o", metavar="FILE", help="Write output to FILE (default: stdout)")
    return p


def main() -> None:
    parser = build_arg_parser()
    args = parser.parse_args()

    output_path = Path(args.output) if args.output else None

    # ── Validate mode ──────────────────────────────────────────────────────
    if args.validate:
        path = Path(args.validate)
        if not path.exists():
            print(f"[error] File not found: {path}", file=sys.stderr)
            sys.exit(1)
        with path.open() as f:
            data = json.load(f)
        errors = validate_expansion_json(data)
        ok = print_validation_report(errors, str(path))
        sys.exit(0 if ok else 1)

    # ── Merge mode ─────────────────────────────────────────────────────────
    if args.merge:
        path = Path(args.merge)
        if not path.exists():
            print(f"[error] File not found: {path}", file=sys.stderr)
            sys.exit(1)
        with path.open() as f:
            base = json.load(f)
        print(f"[info] Loaded existing expansion.json from {path}", file=sys.stderr)
        if args.interactive:
            data = interactive_build(base)
        else:
            data = build_from_args(args, base)
        errors = validate_expansion_json(data)
        print_validation_report(errors)
        write_output(data, output_path)
        sys.exit(0 if not [e for e in errors if "Unknown" not in e] else 1)

    # ── From-presets mode ──────────────────────────────────────────────────
    if args.from_presets:
        presets_dir = Path(args.from_presets)
        if not presets_dir.is_dir():
            print(f"[error] Not a directory: {presets_dir}", file=sys.stderr)
            sys.exit(1)
        scanned = scan_presets_dir(presets_dir)
        # Scanned data is the base; CLI args can override
        base = {**DEFAULTS, **scanned}
        if args.interactive:
            data = interactive_build(base)
        else:
            data = build_from_args(args, base)
        errors = validate_expansion_json(data)
        print_validation_report(errors)
        write_output(data, output_path)
        sys.exit(0 if not [e for e in errors if "Unknown" not in e] else 1)

    # ── Interactive mode ───────────────────────────────────────────────────
    if args.interactive:
        base = build_from_args(args, None) if any([
            args.name, args.version, args.description, args.author,
            args.genre, args.tags, args.type, args.tier, args.compat, args.artwork
        ]) else None
        data = interactive_build(base)
        errors = validate_expansion_json(data)
        print_validation_report(errors)
        write_output(data, output_path)
        sys.exit(0 if not [e for e in errors if "Unknown" not in e] else 1)

    # ── Plain build from args ──────────────────────────────────────────────
    data = build_from_args(args, None)
    errors = validate_expansion_json(data)
    print_validation_report(errors)
    write_output(data, output_path)
    fatal = [e for e in errors if "Unknown" not in e]
    sys.exit(0 if not fatal else 1)


if __name__ == "__main__":
    main()
