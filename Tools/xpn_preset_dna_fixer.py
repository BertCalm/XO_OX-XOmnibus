#!/usr/bin/env python3
"""
xpn_preset_dna_fixer.py — XOmnibus Sonic DNA Auto-Repair Tool

Scans .xometa preset files recursively and repairs incomplete or inconsistent
6D Sonic DNA blocks:
  [brightness, warmth, movement, density, space, aggression] — each 0.0–1.0

Repairs performed:
  - Missing sonic_dna / dna block entirely  → estimate from params or use engine defaults
  - Missing individual dimensions            → fill with engine-based defaults
  - Values outside [0.0, 1.0]               → clamp with warning
  - Wrong key names (camelCase, abbreviations, nested) → normalize to snake_case
  - Nested structure                         → flatten to top-level dna dict

Usage:
    python xpn_preset_dna_fixer.py <dir> [--dry-run] [--write] [--engine FILTER]
                                         [--output report.txt]

Options:
    <dir>              Root directory to scan (default: repo Presets/)
    --dry-run          Report changes without writing (default if neither flag given)
    --write            Apply fixes in-place
    --engine FILTER    Only process presets whose engine matches FILTER (case-insensitive)
    --output FILE      Write report to FILE instead of stdout
"""

import argparse
import json
import re
import sys
from pathlib import Path


# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

DIMS = ["brightness", "warmth", "movement", "density", "space", "aggression"]

# Canonical engine DNA defaults — used when a dimension is missing entirely
ENGINE_DNA_DEFAULTS: dict[str, dict[str, float]] = {
    # ONSET (drums / percussion)
    "ONSET": {
        "brightness": 0.6,
        "warmth":     0.3,
        "movement":   0.7,
        "density":    0.7,
        "space":      0.4,
        "aggression": 0.6,
    },
    # OPAL (granular)
    "OPAL": {
        "brightness": 0.4,
        "warmth":     0.6,
        "movement":   0.5,
        "density":    0.3,
        "space":      0.8,
        "aggression": 0.2,
    },
    # Neutral fallback for any other engine
    "DEFAULT": {
        "brightness": 0.5,
        "warmth":     0.5,
        "movement":   0.5,
        "density":    0.5,
        "space":      0.5,
        "aggression": 0.5,
    },
}

# Alias map: any variant key -> canonical dimension name
# Order matters for ambiguous abbreviations — more specific first
KEY_ALIASES: dict[str, str] = {
    # brightness
    "brightness":  "brightness",
    "Brightness":  "brightness",
    "BRIGHTNESS":  "brightness",
    "bright":      "brightness",
    "Bright":      "brightness",
    "brite":       "brightness",
    # warmth
    "warmth":      "warmth",
    "Warmth":      "warmth",
    "WARMTH":      "warmth",
    "warm":        "warmth",
    "Warm":        "warmth",
    # movement
    "movement":    "movement",
    "Movement":    "movement",
    "MOVEMENT":    "movement",
    "motion":      "movement",
    "Motion":      "movement",
    "move":        "movement",
    # density
    "density":     "density",
    "Density":     "density",
    "DENSITY":     "density",
    "dense":       "density",
    "Dense":       "density",
    # space
    "space":       "space",
    "Space":       "space",
    "SPACE":       "space",
    "spacious":    "space",
    "reverb":      "space",
    # aggression
    "aggression":  "aggression",
    "Aggression":  "aggression",
    "AGGRESSION":  "aggression",
    "aggressive":  "aggression",
    "Aggressive":  "aggression",
    "aggressiveness": "aggression",
    "attack":      "aggression",
}


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _camel_to_snake(name: str) -> str:
    """Convert camelCase / PascalCase to snake_case."""
    s1 = re.sub(r"(.)([A-Z][a-z]+)", r"\1_\2", name)
    return re.sub(r"([a-z0-9])([A-Z])", r"\1_\2", s1).lower()


def _resolve_key(key: str):
    """Return canonical dimension name for key, or None if unrecognised."""
    # Direct lookup
    if key in KEY_ALIASES:
        return KEY_ALIASES[key]
    # Try snake-casing first
    snake = _camel_to_snake(key)
    if snake in KEY_ALIASES:
        return KEY_ALIASES[snake]
    # Case-insensitive fallback
    lower = key.lower()
    for alias, canon in KEY_ALIASES.items():
        if alias.lower() == lower:
            return canon
    return None


def _engine_defaults(engines: list[str]) -> dict[str, float]:
    """Return DNA defaults for the first recognised engine, else DEFAULT."""
    for eng in engines:
        upper = eng.upper()
        if upper in ENGINE_DNA_DEFAULTS:
            return ENGINE_DNA_DEFAULTS[upper]
    return ENGINE_DNA_DEFAULTS["DEFAULT"]


def _estimate_from_params(parameters: dict, engines: list) -> dict:
    """
    Rough heuristic estimate of DNA from parameter values.
    Falls back gracefully to engine defaults for any dimension we cannot estimate.
    """
    defaults = _engine_defaults(engines)
    dna = dict(defaults)  # start from engine defaults

    # Collect all param values into a flat dict (ignore engine sub-keys)
    flat: dict[str, float] = {}
    for v in parameters.values():
        if isinstance(v, dict):
            flat.update(v)
        elif isinstance(v, (int, float)):
            flat[str(v)] = float(v)

    def _get(keys: list):
        for k in keys:
            if k in flat:
                val = flat[k]
                if isinstance(val, (int, float)):
                    return float(val)
        return None

    # brightness ← filter cutoff (normalised 0–1 if stored that way)
    b = _get(["filterCutoff", "cutoff", "filter_cutoff", "fltCutoff",
               "fat_filterCutoff", "snap_filterCutoff"])
    if b is not None:
        dna["brightness"] = max(0.0, min(1.0, b))

    # warmth ← filter resonance inverted, or drive
    r = _get(["filterRes", "resonance", "fltRes", "filter_res"])
    if r is not None:
        dna["warmth"] = max(0.0, min(1.0, 1.0 - r * 0.5))

    # movement ← LFO rate or mod depth
    lfo = _get(["lfoRate", "lfo_rate", "lfoSpeed", "modDepth", "mod_depth"])
    if lfo is not None:
        dna["movement"] = max(0.0, min(1.0, lfo))

    # density ← oscillator count / unison or grain density
    den = _get(["unisonVoices", "unison", "grainDensity", "grain_density", "density"])
    if den is not None:
        # many unison params are counts (1-8) — normalise
        if den > 1.0:
            den = min(den / 8.0, 1.0)
        dna["density"] = max(0.0, min(1.0, den))

    # space ← reverb mix
    sp = _get(["reverbMix", "reverb_mix", "roomSize", "room_size", "space"])
    if sp is not None:
        dna["space"] = max(0.0, min(1.0, sp))

    # aggression ← drive or distortion
    ag = _get(["drive", "distortion", "satDrive", "sat_drive",
               "fat_satDrive", "perc_noiseLevel"])
    if ag is not None:
        dna["aggression"] = max(0.0, min(1.0, ag))

    return dna


# ---------------------------------------------------------------------------
# Core repair logic
# ---------------------------------------------------------------------------

class DnaIssue:
    MISSING_BLOCK       = "missing_block"
    MISSING_DIMENSION   = "missing_dimension"
    CLAMPED_VALUE       = "clamped_value"
    NORMALIZED_KEY      = "normalized_key"
    FLATTENED_STRUCT    = "flattened_struct"


def repair_dna(data: dict, file_path: Path):
    """
    Inspect and repair the DNA block in *data*.

    Returns (repaired_data, list_of_issue_descriptions).
    Does not mutate the original dict — returns a new one.
    """
    import copy
    data = copy.deepcopy(data)
    issues: list[str] = []

    engines: list[str] = data.get("engines", [])
    parameters: dict = data.get("parameters", {})

    # ------------------------------------------------------------------
    # 1. Locate the DNA block (might be "dna" or "sonic_dna")
    # ------------------------------------------------------------------
    raw_dna = data.get("dna") or data.get("sonic_dna")

    if raw_dna is None:
        issues.append(f"{DnaIssue.MISSING_BLOCK}: no dna/sonic_dna key found")
        estimated = _estimate_from_params(parameters, engines)
        data["dna"] = estimated
        # Remove stale "sonic_dna" key if present under wrong name
        data.pop("sonic_dna", None)
        return data, issues

    # ------------------------------------------------------------------
    # 2. Handle nested structure  {"sonic_dna": {"dna": {...}}}  etc.
    # ------------------------------------------------------------------
    if isinstance(raw_dna, dict):
        # Check if all values are themselves dicts (nested)
        if all(isinstance(v, dict) for v in raw_dna.values()) and raw_dna:
            # Flatten: merge first nested level
            flat: dict = {}
            for sub in raw_dna.values():
                flat.update(sub)
            issues.append(f"{DnaIssue.FLATTENED_STRUCT}: nested dna structure flattened")
            raw_dna = flat
    else:
        # Completely wrong type
        issues.append(f"{DnaIssue.MISSING_BLOCK}: dna block has wrong type ({type(raw_dna).__name__}), replacing")
        data["dna"] = _engine_defaults(engines)
        data.pop("sonic_dna", None)
        return data, issues

    # ------------------------------------------------------------------
    # 3. Normalise key names
    # ------------------------------------------------------------------
    normalised: dict[str, float] = {}
    for key, val in raw_dna.items():
        canon = _resolve_key(key)
        if canon is None:
            # Unknown key — skip, will be detected as missing below
            issues.append(f"{DnaIssue.NORMALIZED_KEY}: unrecognised key '{key}' dropped")
            continue
        if canon != key:
            issues.append(f"{DnaIssue.NORMALIZED_KEY}: '{key}' → '{canon}'")
        if not isinstance(val, (int, float)):
            issues.append(f"{DnaIssue.CLAMPED_VALUE}: '{canon}' has non-numeric value ({val!r}), defaulting to 0.5")
            val = 0.5
        normalised[canon] = float(val)

    # ------------------------------------------------------------------
    # 4. Fill missing dimensions
    # ------------------------------------------------------------------
    defaults = _engine_defaults(engines)
    for dim in DIMS:
        if dim not in normalised:
            fill = defaults[dim]
            normalised[dim] = fill
            issues.append(f"{DnaIssue.MISSING_DIMENSION}: '{dim}' missing → filled with {fill:.2f} (engine default)")

    # ------------------------------------------------------------------
    # 5. Clamp out-of-range values
    # ------------------------------------------------------------------
    for dim in DIMS:
        v = normalised[dim]
        if v < 0.0 or v > 1.0:
            clamped = max(0.0, min(1.0, v))
            issues.append(f"{DnaIssue.CLAMPED_VALUE}: '{dim}' = {v:.4f} clamped to {clamped:.4f}")
            normalised[dim] = clamped

    # ------------------------------------------------------------------
    # 6. Write back — always use "dna" key (canonical)
    # ------------------------------------------------------------------
    data["dna"] = normalised
    data.pop("sonic_dna", None)  # remove legacy key if it existed

    return data, issues


# ---------------------------------------------------------------------------
# File scanning
# ---------------------------------------------------------------------------

def scan_directory(root: Path, engine_filter):
    """Return all .xometa files under root, optionally filtered by engine name."""
    files = sorted(root.rglob("*.xometa"))
    if not engine_filter:
        return files

    filtered = []
    ef_lower = engine_filter.lower()
    for f in files:
        try:
            data = json.loads(f.read_text(encoding="utf-8"))
        except Exception:
            continue
        engines = [e.lower() for e in data.get("engines", [])]
        if any(ef_lower in e for e in engines):
            filtered.append(f)
    return filtered


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main() -> None:
    parser = argparse.ArgumentParser(
        description="Auto-repair incomplete or inconsistent Sonic DNA in .xometa preset files."
    )
    parser.add_argument(
        "dir",
        nargs="?",
        default=None,
        help="Root directory to scan. Defaults to <repo>/Presets/",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        default=False,
        help="Report what would change without writing (default mode if --write not given)",
    )
    parser.add_argument(
        "--write",
        action="store_true",
        default=False,
        help="Apply fixes in-place",
    )
    parser.add_argument(
        "--engine",
        default=None,
        metavar="FILTER",
        help="Only process presets whose engines[] contains FILTER (case-insensitive substring)",
    )
    parser.add_argument(
        "--output",
        default=None,
        metavar="FILE",
        help="Write report to FILE instead of stdout",
    )

    args = parser.parse_args()

    # Resolve root directory
    if args.dir:
        root = Path(args.dir).expanduser().resolve()
    else:
        root = Path(__file__).parent.parent / "Presets"

    if not root.exists():
        print(f"ERROR: directory not found: {root}", file=sys.stderr)
        sys.exit(1)

    # Default to dry-run if neither flag given
    write_mode = args.write
    if not write_mode and not args.dry_run:
        args.dry_run = True

    # Open output
    if args.output:
        out = open(args.output, "w", encoding="utf-8")
    else:
        out = sys.stdout

    def emit(line: str = "") -> None:
        print(line, file=out)

    emit(f"XPN Preset DNA Fixer — {'DRY RUN' if args.dry_run else 'WRITE MODE'}")
    emit(f"Scanning: {root}")
    if args.engine:
        emit(f"Engine filter: {args.engine}")
    emit()

    files = scan_directory(root, args.engine)
    emit(f"Found {len(files)} .xometa file(s)")
    emit()

    # Counters
    total_files_with_issues = 0
    total_dims_filled = 0
    total_values_clamped = 0
    total_keys_normalised = 0
    total_blocks_created = 0
    total_written = 0
    parse_errors: list[str] = []

    for f in files:
        try:
            text = f.read_text(encoding="utf-8")
            data = json.loads(text)
        except Exception as e:
            parse_errors.append(f"  PARSE ERROR {f}: {e}")
            continue

        repaired, issues = repair_dna(data, f)

        if not issues:
            continue

        total_files_with_issues += 1
        for issue in issues:
            kind = issue.split(":")[0]
            if kind == DnaIssue.MISSING_BLOCK:
                total_blocks_created += 1
            elif kind == DnaIssue.MISSING_DIMENSION:
                total_dims_filled += 1
            elif kind == DnaIssue.CLAMPED_VALUE:
                total_values_clamped += 1
            elif kind == DnaIssue.NORMALIZED_KEY:
                total_keys_normalised += 1

        rel = f.relative_to(root) if f.is_relative_to(root) else f
        emit(f"[{'FIXED' if write_mode else 'WOULD FIX'}] {rel}")
        for issue in issues:
            emit(f"    {issue}")

        if write_mode:
            try:
                f.write_text(
                    json.dumps(repaired, indent=2, ensure_ascii=False) + "\n",
                    encoding="utf-8",
                )
                total_written += 1
            except Exception as e:
                emit(f"    ERROR writing file: {e}")

    # Parse errors
    if parse_errors:
        emit()
        emit("Parse errors (files skipped):")
        for err in parse_errors:
            emit(err)

    # Summary
    emit()
    emit("=" * 60)
    emit("SUMMARY")
    emit("=" * 60)
    emit(f"Files scanned:              {len(files)}")
    emit(f"Files with issues:          {total_files_with_issues}")
    emit(f"DNA blocks created:         {total_blocks_created}")
    emit(f"Dimensions filled:          {total_dims_filled}")
    emit(f"Values clamped:             {total_values_clamped}")
    emit(f"Key names normalised:       {total_keys_normalised}")
    if write_mode:
        emit(f"Files written:              {total_written}")
    else:
        emit("(dry-run — no files written)")
    emit()

    if args.output:
        out.close()
        print(f"Report written to: {args.output}")


if __name__ == "__main__":
    main()
