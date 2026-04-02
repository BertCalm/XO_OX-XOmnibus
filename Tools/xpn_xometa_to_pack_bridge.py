#!/usr/bin/env python3
"""
XPN XOMeta → Pack Bridge — XO_OX Designs
=========================================
Bridges the XOceanus .xometa preset system and the XPN pack pipeline.

Workflow:
  1. Use xpn_preset_browser.py (or hand-curate) to build pack_selection.json
  2. Run this tool to generate render_spec.json
  3. Feed render_spec.json into the render/export pipeline

pack_selection.json format:
  {
    "pack_name": "Iron Machines",
    "engine": "ONSET",
    "mood": "Foundation",
    "presets": ["Iron Hat", "Glass Snap", "Rust Cabinet"]
  }

render_spec.json output format:
  {
    "pack_name": "Iron Machines",
    "engine": "ONSET",
    "programs": [
      {
        "name": "Iron Hat",
        "preset_file": "Presets/XOceanus/Foundation/ONSET/Iron Hat.xometa",
        "engine": "ONSET",
        "render_notes": [36, 38, 42, 44, 46, 49],
        "velocities": [40, 80, 100, 120],
        "duration_s": 0.5,
        "sonic_dna": { ... },
        "macros": { "M1": 0.5, "M2": 0.5, "M3": 0.0, "M4": 0.0 },
        "suggested_program_name": "Iron Hat"
      }
    ]
  }

Usage:
  python xpn_xometa_to_pack_bridge.py \\
      --selection pack_selection.json \\
      --presets-dir ./Presets/XOceanus/ \\
      --output render_spec.json
"""

import argparse
import json
import sys
from pathlib import Path
from typing import Dict, List, Optional


# ---------------------------------------------------------------------------
# Engine render defaults
# ---------------------------------------------------------------------------

# Drum engines: GM-ish note assignments (one note per drum voice)
DRUM_ENGINES = {"ONSET"}

# Default MIDI note sets per engine category
RENDER_NOTES_DRUM = [36, 38, 42, 44, 46, 49]          # BD, SD, CHat, OHat, Clap, Crash
RENDER_NOTES_MELODIC = [48, 52, 55, 60, 64, 67, 72]   # C3–C5 spread (minor 3rd steps)

# Default velocity layers
DEFAULT_VELOCITIES = [40, 80, 100, 120]

# Default render durations (seconds) by engine
DURATION_BY_ENGINE = {
    "ONSET":     0.5,
    "ODYSSEY":   3.0,
    "BOB":       2.0,
    "FAT":       2.0,
    "OBESE":     2.0,
    "MORPH":     3.0,
    "DUB":       4.0,
    "OVERWORLD": 2.0,
    "OPAL":      8.0,   # texture renders
    "OVERLAP":   4.0,
    "OUTWIT":    3.0,
    "OHM":       3.0,
    "ORPHICA":   4.0,
    "OBBLIGATO": 3.0,
    "OTTONI":    2.5,
    "OLE":       3.0,
    "ORGANON":   3.0,
    "SNAP":      1.0,
}
DEFAULT_DURATION = 2.0


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def engine_key(name: str) -> str:
    """Normalise engine name to upper-case for lookups."""
    return name.upper()


def render_notes_for_engine(eng: str) -> list:
    if engine_key(eng) in DRUM_ENGINES:
        return list(RENDER_NOTES_DRUM)
    return list(RENDER_NOTES_MELODIC)


def duration_for_engine(eng: str) -> float:
    return DURATION_BY_ENGINE.get(engine_key(eng), DEFAULT_DURATION)


def find_xometa(presets_dir: Path, preset_name: str, mood: Optional[str]) -> Optional[Path]:
    """
    Locate a .xometa file by preset name.

    Search order:
      1. mood sub-directory (fast path when mood is known)
      2. Full recursive scan of presets_dir
    Matching is case-insensitive on the stem.
    """
    target = preset_name.lower()
    target_normalised = target.replace(" ", "_")  # "Tidal Sweep" → "tidal_sweep"

    def stem_matches(stem: str) -> bool:
        s = stem.lower()
        return s == target or s == target_normalised or s.replace("_", " ") == target

    # Fast path: check mood directory first
    if mood:
        for candidate in (presets_dir / mood).rglob("*.xometa"):
            if stem_matches(candidate.stem):
                return candidate

    # Full scan fallback
    for candidate in presets_dir.rglob("*.xometa"):
        if stem_matches(candidate.stem):
            return candidate

    return None


def extract_preset_data(xometa_path: Path) -> dict:
    """Parse a .xometa file and return relevant fields."""
    with xometa_path.open("r", encoding="utf-8") as fh:
        data = json.load(fh)

    # Engine: use first entry in 'engines' list if present
    engines_list = data.get("engines", [])
    engine = engines_list[0] if engines_list else data.get("engine", "Unknown")

    # Sonic DNA
    sonic_dna = data.get("dna", {})

    # Macro values: look inside parameters for the engine block,
    # then fall back to top-level macroValues / macros fields.
    macro_labels = data.get("macroLabels", [])
    parameters = data.get("parameters", {})

    # Try to pull macro values from the engine's parameter block
    engine_params = parameters.get(engine, parameters.get(engine.lower(), {}))
    raw_macros: dict = {}
    for key in list(engine_params.keys()):
        if key.lower().startswith("macro") or key.upper().startswith("M") and len(key) <= 3:
            raw_macros[key] = engine_params[key]

    # Prefer explicit macroValues dict if provided at top level
    if "macroValues" in data:
        raw_macros = data["macroValues"]
    elif "macros" in data:
        raw_macros = data["macros"]

    # Build normalised M1-M4 dict
    macros: dict = {}
    for i, label in enumerate(macro_labels, start=1):
        key = f"M{i}"
        # Try to find a matching value in raw_macros by index key or label
        val = raw_macros.get(key, raw_macros.get(str(i), raw_macros.get(label, 0.0)))
        macros[key] = float(val)
    # If no macro labels, still try to populate M1-M4
    if not macro_labels:
        for i in range(1, 5):
            key = f"M{i}"
            macros[key] = float(raw_macros.get(key, 0.0))

    return {
        "engine": engine,
        "sonic_dna": sonic_dna,
        "macros": macros,
        "macro_labels": macro_labels,
        "mood": data.get("mood"),
        "tags": data.get("tags", []),
        "description": data.get("description", ""),
        "parameters": parameters,
    }


def build_program(
    preset_name: str,
    xometa_path: Path,
    preset_data: dict,
    presets_dir: Path,
    pack_engine: Optional[str],
) -> dict:
    """Build a single program entry for render_spec.json."""
    engine = preset_data["engine"] or pack_engine or "Unknown"
    eng_key = engine_key(engine)

    # Make preset_file relative to the presets_dir parent (repo root)
    try:
        rel_path = xometa_path.relative_to(presets_dir.parent)
    except ValueError:
        rel_path = xometa_path  # fallback: absolute

    return {
        "name": preset_name,
        "preset_file": str(rel_path),
        "engine": engine,
        "render_notes": render_notes_for_engine(eng_key),
        "velocities": list(DEFAULT_VELOCITIES),
        "duration_s": duration_for_engine(eng_key),
        "sonic_dna": preset_data["sonic_dna"],
        "macros": preset_data["macros"],
        "macro_labels": preset_data["macro_labels"],
        "tags": preset_data["tags"],
        "description": preset_data["description"],
        "suggested_program_name": preset_name,
    }


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main() -> int:
    parser = argparse.ArgumentParser(
        description="Bridge .xometa presets → XPN render_spec.json"
    )
    parser.add_argument(
        "--selection", "-s",
        required=True,
        help="Path to pack_selection.json",
    )
    parser.add_argument(
        "--presets-dir", "-p",
        default=str(Path(__file__).parent.parent / "Presets" / "XOceanus"),
        help="Root directory of XOceanus presets (default: ../../Presets/XOceanus)",
    )
    parser.add_argument(
        "--output", "-o",
        default="render_spec.json",
        help="Output path for render_spec.json (default: render_spec.json)",
    )
    parser.add_argument(
        "--verbose", "-v",
        action="store_true",
        help="Print progress to stderr",
    )
    args = parser.parse_args()

    selection_path = Path(args.selection)
    presets_dir = Path(args.presets_dir)
    output_path = Path(args.output)

    # --- Load selection ---
    if not selection_path.exists():
        print(f"ERROR: selection file not found: {selection_path}", file=sys.stderr)
        return 1

    with selection_path.open("r", encoding="utf-8") as fh:
        selection = json.load(fh)

    pack_name = selection.get("pack_name", "Unnamed Pack")
    pack_engine = selection.get("engine")
    mood = selection.get("mood")
    preset_names: list = selection.get("presets", [])

    if not preset_names:
        print("ERROR: 'presets' list is empty in selection file.", file=sys.stderr)
        return 1

    if not presets_dir.is_dir():
        print(f"ERROR: presets-dir not found: {presets_dir}", file=sys.stderr)
        return 1

    # --- Process each preset ---
    programs = []
    missing = []

    for name in preset_names:
        xometa_path = find_xometa(presets_dir, name, mood)
        if xometa_path is None:
            print(f"  WARN: preset not found — '{name}'", file=sys.stderr)
            missing.append(name)
            continue

        if args.verbose:
            print(f"  OK  : {name} → {xometa_path}", file=sys.stderr)

        try:
            preset_data = extract_preset_data(xometa_path)
        except (json.JSONDecodeError, KeyError) as exc:
            print(f"  WARN: failed to parse '{xometa_path}' — {exc}", file=sys.stderr)
            missing.append(name)
            continue

        programs.append(
            build_program(name, xometa_path, preset_data, presets_dir, pack_engine)
        )

    # --- Build render spec ---
    render_spec = {
        "pack_name": pack_name,
        "engine": pack_engine or (programs[0]["engine"] if programs else "Unknown"),
        "mood": mood,
        "programs": programs,
    }

    if missing:
        render_spec["missing_presets"] = missing

    with output_path.open("w", encoding="utf-8") as fh:
        json.dump(render_spec, fh, indent=2)

    print(f"render_spec.json written → {output_path}")
    print(f"  {len(programs)} programs, {len(missing)} missing")
    if missing:
        print(f"  Missing: {', '.join(missing)}")

    return 0 if not missing else 2  # 2 = partial success


if __name__ == "__main__":
    sys.exit(main())
