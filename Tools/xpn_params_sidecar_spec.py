"""
xpn_params_sidecar_spec.py — XOmnibus params_sidecar.json generator and validator.

Bridges MPC XPN programs with XOmnibus .xometa presets via a companion sidecar file.
When MPC loads an XPN program, XOmnibus reads the sidecar to auto-load a matching
preset voice, linking sample pack character to synthesis engine character.

Usage:
  python xpn_params_sidecar_spec.py generate \\
      --pack <pack_dir> --presets <preset_dir> \\
      [--output sidecar.json] [--threshold 0.4] [--pack-name "My Pack"]

  python xpn_params_sidecar_spec.py validate \\
      --sidecar sidecar.json --pack <pack_dir> --presets <preset_dir>

Schema (params_sidecar.json):
  {
    "version": "1.0",
    "pack_name": "...",
    "xomnibus_version_min": "1.0.0",
    "mappings": [
      {
        "program_file": "kick_hard.xpm",
        "preset_file": "Kick Hard.xometa",
        "engine": "ONSET",
        "confidence": 0.95,
        "match_method": "exact|fuzzy|manual"
      }
    ]
  }
"""

from __future__ import annotations

import argparse
import json
import re
import sys
import xml.etree.ElementTree as ET
from pathlib import Path


# ---------------------------------------------------------------------------
# Name normalisation + Jaccard similarity
# ---------------------------------------------------------------------------

def normalize_name(name: str) -> set[str]:
    """Lowercase, strip extension, remove punctuation, split into word set."""
    stem = Path(name).stem
    lower = stem.lower()
    # Replace punctuation and underscores with spaces
    cleaned = re.sub(r"[_\-\.]+", " ", lower)
    cleaned = re.sub(r"[^a-z0-9 ]", "", cleaned)
    words = [w for w in cleaned.split() if w]
    return set(words)


def jaccard(a: set[str], b: set[str]) -> float:
    """Jaccard similarity of two word sets. Returns 0.0 if both are empty."""
    if not a and not b:
        return 0.0
    union = a | b
    if not union:
        return 0.0
    return len(a & b) / len(union)


# ---------------------------------------------------------------------------
# XPM parsing helpers
# ---------------------------------------------------------------------------

def read_program_engine(xpm_path: Path) -> str | None:
    """
    Try to extract an engine hint from an XPM file.
    XOmnibus-exported XPMs may embed an <Engine> element or a comment.
    Falls back to None if not found (caller should use directory/naming heuristics).
    """
    try:
        tree = ET.parse(xpm_path)
        root = tree.getroot()
        # Look for <Engine> anywhere in the tree
        for elem in root.iter("Engine"):
            text = (elem.text or "").strip().upper()
            if text:
                return text
        # Look for an attribute named 'engine' on any element
        for elem in root.iter():
            eng = elem.get("engine") or elem.get("Engine")
            if eng:
                return eng.strip().upper()
    except ET.ParseError:
        pass
    return None


def read_xometa_engine(xometa_path: Path) -> str | None:
    """Extract the engine name from a .xometa JSON preset file."""
    try:
        data = json.loads(xometa_path.read_text(encoding="utf-8"))
        # Common keys: "engine", "Engine", nested under "header"
        for key in ("engine", "Engine"):
            if key in data:
                return str(data[key]).upper()
        header = data.get("header") or data.get("Header") or {}
        for key in ("engine", "Engine"):
            if key in header:
                return str(header[key]).upper()
    except (json.JSONDecodeError, OSError):
        pass
    return None


# ---------------------------------------------------------------------------
# Discovery helpers
# ---------------------------------------------------------------------------

def collect_xpm_files(pack_dir: Path) -> list[Path]:
    """Recursively collect all .xpm files under pack_dir."""
    return sorted(pack_dir.rglob("*.xpm"))


def collect_xometa_files(preset_dir: Path) -> list[Path]:
    """Recursively collect all .xometa files under preset_dir."""
    return sorted(preset_dir.rglob("*.xometa"))


# ---------------------------------------------------------------------------
# Matching logic
# ---------------------------------------------------------------------------

def build_mappings(
    xpm_files: list[Path],
    xometa_files: list[Path],
    threshold: float = 0.4,
) -> list[dict]:
    """
    Match each XPM program to the best-scoring .xometa preset.

    Returns a list of mapping dicts sorted by confidence descending.
    A program may appear at most once; a preset may appear at most once
    (greedy best-first assignment).
    """
    # Pre-compute normalised word sets
    xpm_words = {p: normalize_name(p.name) for p in xpm_files}
    meta_words = {p: normalize_name(p.name) for p in xometa_files}

    # Score all pairs above threshold
    candidates: list[tuple[float, Path, Path]] = []
    for xpm, xw in xpm_words.items():
        for meta, mw in meta_words.items():
            score = jaccard(xw, mw)
            if score >= threshold:
                candidates.append((score, xpm, meta))

    # Sort by confidence descending; greedy assign
    candidates.sort(key=lambda t: t[0], reverse=True)
    used_xpm: set[Path] = set()
    used_meta: set[Path] = set()
    mappings: list[dict] = []

    for score, xpm, meta in candidates:
        if xpm in used_xpm or meta in used_meta:
            continue
        used_xpm.add(xpm)
        used_meta.add(meta)

        # Determine match_method
        xw = xpm_words[xpm]
        mw = meta_words[meta]
        if xw == mw:
            method = "exact"
        else:
            method = "fuzzy"

        # Try to determine engine
        engine = read_program_engine(xpm) or read_xometa_engine(meta) or "UNKNOWN"

        mappings.append(
            {
                "program_file": xpm.name,
                "preset_file": meta.name,
                "engine": engine,
                "confidence": round(score, 4),
                "match_method": method,
            }
        )

    # Sort final list by confidence descending for readability
    mappings.sort(key=lambda m: m["confidence"], reverse=True)
    return mappings


# ---------------------------------------------------------------------------
# Sidecar generation
# ---------------------------------------------------------------------------

def generate_sidecar(
    pack_dir: Path,
    preset_dir: Path,
    output_path: Path,
    threshold: float = 0.4,
    pack_name: str | None = None,
    xomnibus_version_min: str = "1.0.0",
) -> dict:
    """Generate params_sidecar.json and write to output_path. Returns the dict."""
    if not pack_dir.is_dir():
        raise ValueError(f"Pack directory not found: {pack_dir}")
    if not preset_dir.is_dir():
        raise ValueError(f"Preset directory not found: {preset_dir}")

    xpm_files = collect_xpm_files(pack_dir)
    xometa_files = collect_xometa_files(preset_dir)

    if not xpm_files:
        print(f"  WARNING: No .xpm files found under {pack_dir}", file=sys.stderr)
    if not xometa_files:
        print(f"  WARNING: No .xometa files found under {preset_dir}", file=sys.stderr)

    mappings = build_mappings(xpm_files, xometa_files, threshold=threshold)

    sidecar = {
        "version": "1.0",
        "pack_name": pack_name or pack_dir.name,
        "xomnibus_version_min": xomnibus_version_min,
        "mappings": mappings,
    }

    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(json.dumps(sidecar, indent=2), encoding="utf-8")
    return sidecar


# ---------------------------------------------------------------------------
# Sidecar validation
# ---------------------------------------------------------------------------

def validate_sidecar(
    sidecar_path: Path,
    pack_dir: Path,
    preset_dir: Path,
) -> bool:
    """
    Validate an existing params_sidecar.json.

    Checks:
    - File is valid JSON and contains required top-level keys
    - Each mapping has required fields with correct types
    - program_file references exist in pack_dir
    - preset_file references exist in preset_dir
    - confidence is in [0.0, 1.0]
    - match_method is one of exact|fuzzy|manual

    Returns True if all checks pass, False otherwise.
    """
    errors: list[str] = []
    warnings: list[str] = []

    # --- Load JSON ---
    try:
        raw = sidecar_path.read_text(encoding="utf-8")
        data = json.loads(raw)
    except (OSError, json.JSONDecodeError) as exc:
        print(f"  ERROR: Cannot read sidecar: {exc}", file=sys.stderr)
        return False

    # --- Top-level keys ---
    required_top = ("version", "pack_name", "xomnibus_version_min", "mappings")
    for key in required_top:
        if key not in data:
            errors.append(f"Missing top-level key: '{key}'")

    if errors:
        for e in errors:
            print(f"  ERROR: {e}", file=sys.stderr)
        return False

    if not isinstance(data["mappings"], list):
        print("  ERROR: 'mappings' must be a list", file=sys.stderr)
        return False

    # --- Build file lookup sets (name only — sidecar stores bare filenames) ---
    xpm_names: set[str] = {p.name for p in collect_xpm_files(pack_dir)}
    meta_names: set[str] = {p.name for p in collect_xometa_files(preset_dir)}

    valid_methods = {"exact", "fuzzy", "manual"}

    for idx, mapping in enumerate(data["mappings"]):
        prefix = f"mappings[{idx}]"

        # Required fields
        for field in ("program_file", "preset_file", "engine", "confidence", "match_method"):
            if field not in mapping:
                errors.append(f"{prefix}: missing field '{field}'")

        if errors:
            continue  # skip further checks for this entry

        # Type checks
        if not isinstance(mapping["program_file"], str):
            errors.append(f"{prefix}.program_file must be a string")
        if not isinstance(mapping["preset_file"], str):
            errors.append(f"{prefix}.preset_file must be a string")
        if not isinstance(mapping["engine"], str):
            errors.append(f"{prefix}.engine must be a string")
        if not isinstance(mapping["confidence"], (int, float)):
            errors.append(f"{prefix}.confidence must be a number")
        elif not (0.0 <= mapping["confidence"] <= 1.0):
            errors.append(f"{prefix}.confidence {mapping['confidence']!r} out of range [0, 1]")
        if mapping.get("match_method") not in valid_methods:
            errors.append(
                f"{prefix}.match_method {mapping.get('match_method')!r} "
                f"not in {sorted(valid_methods)}"
            )

        # File existence
        pf = mapping.get("program_file", "")
        mf = mapping.get("preset_file", "")

        if pf and pf not in xpm_names:
            warnings.append(f"{prefix}: program_file '{pf}' not found in pack dir")
        if mf and mf not in meta_names:
            warnings.append(f"{prefix}: preset_file '{mf}' not found in preset dir")

    # --- Report ---
    ok = len(errors) == 0
    total = len(data["mappings"])

    for w in warnings:
        print(f"  WARNING: {w}", file=sys.stderr)
    for e in errors:
        print(f"  ERROR: {e}", file=sys.stderr)

    status = "PASS" if ok else "FAIL"
    warn_count = len(warnings)
    err_count = len(errors)
    print(
        f"\n  Validation {status}: {total} mapping(s), "
        f"{err_count} error(s), {warn_count} warning(s)"
    )
    return ok


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def cmd_generate(args: argparse.Namespace) -> int:
    pack_dir = Path(args.pack).expanduser().resolve()
    preset_dir = Path(args.presets).expanduser().resolve()
    output_path = Path(args.output).expanduser().resolve()
    threshold = float(args.threshold)
    pack_name = args.pack_name or None

    print(f"Generating sidecar...")
    print(f"  Pack dir   : {pack_dir}")
    print(f"  Preset dir : {preset_dir}")
    print(f"  Threshold  : {threshold}")
    print(f"  Output     : {output_path}")

    try:
        sidecar = generate_sidecar(
            pack_dir=pack_dir,
            preset_dir=preset_dir,
            output_path=output_path,
            threshold=threshold,
            pack_name=pack_name,
        )
    except ValueError as exc:
        print(f"  ERROR: {exc}", file=sys.stderr)
        return 1

    count = len(sidecar["mappings"])
    exact = sum(1 for m in sidecar["mappings"] if m["match_method"] == "exact")
    fuzzy = sum(1 for m in sidecar["mappings"] if m["match_method"] == "fuzzy")
    print(f"\n  Done. {count} mapping(s) written ({exact} exact, {fuzzy} fuzzy).")

    if count > 0:
        print("\n  Top matches:")
        for m in sidecar["mappings"][:5]:
            print(
                f"    [{m['confidence']:.2f}] {m['program_file']} "
                f"-> {m['preset_file']} ({m['engine']})"
            )
        if count > 5:
            print(f"    ... and {count - 5} more")

    return 0


def cmd_validate(args: argparse.Namespace) -> int:
    sidecar_path = Path(args.sidecar).expanduser().resolve()
    pack_dir = Path(args.pack).expanduser().resolve()
    preset_dir = Path(args.presets).expanduser().resolve()

    print(f"Validating sidecar...")
    print(f"  Sidecar    : {sidecar_path}")
    print(f"  Pack dir   : {pack_dir}")
    print(f"  Preset dir : {preset_dir}")

    if not sidecar_path.is_file():
        print(f"  ERROR: Sidecar file not found: {sidecar_path}", file=sys.stderr)
        return 1
    if not pack_dir.is_dir():
        print(f"  ERROR: Pack directory not found: {pack_dir}", file=sys.stderr)
        return 1
    if not preset_dir.is_dir():
        print(f"  ERROR: Preset directory not found: {preset_dir}", file=sys.stderr)
        return 1

    ok = validate_sidecar(sidecar_path, pack_dir, preset_dir)
    return 0 if ok else 1


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="XOmnibus params_sidecar.json generator and validator.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    subparsers = parser.add_subparsers(dest="command", required=True)

    # generate
    gen = subparsers.add_parser(
        "generate",
        help="Scan pack + preset dirs and generate params_sidecar.json",
    )
    gen.add_argument("--pack", required=True, metavar="DIR", help="Pack directory containing .xpm files")
    gen.add_argument("--presets", required=True, metavar="DIR", help="Preset directory containing .xometa files")
    gen.add_argument("--output", default="params_sidecar.json", metavar="FILE", help="Output path (default: params_sidecar.json)")
    gen.add_argument("--threshold", default=0.4, type=float, metavar="N", help="Minimum Jaccard confidence to include a match (default: 0.4)")
    gen.add_argument("--pack-name", default=None, metavar="NAME", help="Override pack_name in sidecar (default: pack dir name)")

    # validate
    val = subparsers.add_parser(
        "validate",
        help="Validate an existing params_sidecar.json against pack + preset dirs",
    )
    val.add_argument("--sidecar", required=True, metavar="FILE", help="Path to params_sidecar.json")
    val.add_argument("--pack", required=True, metavar="DIR", help="Pack directory containing .xpm files")
    val.add_argument("--presets", required=True, metavar="DIR", help="Preset directory containing .xometa files")

    return parser


def main() -> None:
    parser = build_parser()
    args = parser.parse_args()

    if args.command == "generate":
        sys.exit(cmd_generate(args))
    elif args.command == "validate":
        sys.exit(cmd_validate(args))
    else:
        parser.print_help()
        sys.exit(1)


if __name__ == "__main__":
    main()
