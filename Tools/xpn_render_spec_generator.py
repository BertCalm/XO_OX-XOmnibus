#!/usr/bin/env python3
"""
XPN Render Spec Generator — XO_OX Designs
Generates render_spec.json files for the oxport pipeline.

Reads .xometa preset files and produces a complete render specification:
which presets to render, at which MIDI notes, velocities, and durations.
This bridges the gap between XOceanus presets and the oxport pipeline.

Usage:
    # Generate spec for all ONSET presets in Foundation mood
    python3 xpn_render_spec_generator.py \\
        --presets-dir ./Presets/XOceanus/ \\
        --engine ONSET \\
        --pack-name "Machine Gun Reef" \\
        --mood Foundation

    # Specific preset list, JSON output
    python3 xpn_render_spec_generator.py \\
        --presets-dir ./Presets/XOceanus/ \\
        --engine ONSET \\
        --pack-name "Tide Pack" \\
        --mood Foundation \\
        --preset-list "Tide Kit 1,Tide Kit 2" \\
        --output tide_pack_render_spec.json \\
        --format json
"""

import argparse
import glob
import json
import os
import sys
import warnings

warnings.warn(
    "xpn_render_spec_generator.py is deprecated. Use xpn_render_spec.py instead.",
    DeprecationWarning, stacklevel=2
)

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

VERSION = "1.0.0"

# Render velocities — DEPRECATED constant kept for backward compatibility.
# Old values were [40, 80, 100, 120] (replaced 2026-04-04).
# Import from the single source of truth: xpn_velocity_standard.
from xpn_velocity_standard import RENDER_MIDPOINTS as VIBE_VELOCITIES  # [10, 38, 73, 109]

# GM drum notes used by XOnset
DRUM_NOTES = [36, 38, 42, 44, 46, 49, 51]

# C4 root + major scale (MIDI note numbers)
MELODIC_NOTES = [48, 52, 55, 60, 64, 67, 72]

# Duration in seconds per program type
DURATION_MAP = {
    "drum":    2.0,
    "bass":    3.0,
    "pad":     5.0,
    "texture": 8.0,
    # fallback for melodic engines not in above categories
    "melodic": 3.0,
}

# Tail (release ring-out) per program type
TAIL_MAP = {
    "drum":    0.5,
    "bass":    1.0,
    "pad":     2.0,
    "texture": 3.0,
    "melodic": 1.0,
}

# Engines that are drum programs (trigger-per-voice, not pitched)
DRUM_ENGINES = {"ONSET", "OSTINATO"}

# Engines whose primary register is bass (low-frequency priority)
BASS_ENGINES = {"OBESE", "OCEANDEEP", "OWLFISH", "OVERBITE"}

# Engines that produce ambient/texture sounds (long, evolving)
TEXTURE_ENGINES = {"OPAL", "OPTIC", "OPENSKY", "OCEANIC", "OCELOT", "OMBRE"}

# Engines that map to "pad" (long melodic, slow attack / slow release)
PAD_ENGINES = {
    "OVERDUB", "OHM", "ORACLE", "ORGANON", "OUROBOROS",
    "OBSCURA", "ORIGAMI", "OVERLAP", "OCTOPUS",
}

# Canonical engine name aliases (legacy short names → ALLCAPS canonical)
ENGINE_ALIASES = {
    "Onset":    "ONSET",
    "Drift":    "ODYSSEY",
    "Odyssey":  "ODYSSEY",
    "Dub":      "OVERDUB",
    "Overdub":  "OVERDUB",
    "Bob":      "OBLONG",
    "Oblong":   "OBLONG",
    "Fat":      "OBESE",
    "Obese":    "OBESE",
    "Bite":     "OVERBITE",
    "Overbite": "OVERBITE",
    "Snap":     "ODDFELIX",
    "OddfeliX": "ODDFELIX",
    "Morph":    "ODDOSCAR",
    "OddOscar": "ODDOSCAR",
    "Olé":      "OLE",
    "Ole":      "OLE",
    "Ouïe":     "OUIE",
    "Ouie":     "OUIE",
}


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def canonicalize_engine(raw: str) -> str:
    """Return the canonical ALLCAPS engine ID, resolving any alias."""
    if raw in ENGINE_ALIASES:
        return ENGINE_ALIASES[raw]
    # Already canonical (e.g. "ONSET"), or title-cased engine we don't know
    return raw.upper()


def program_type_for_engine(engine_id: str) -> str:
    """Derive program type from engine ID."""
    e = engine_id.upper()
    if e in DRUM_ENGINES:
        return "drum"
    if e in BASS_ENGINES:
        return "bass"
    if e in TEXTURE_ENGINES:
        return "texture"
    if e in PAD_ENGINES:
        return "pad"
    return "melodic"


def render_notes_for_type(program_type: str) -> list:
    """Return the MIDI render notes for a program type."""
    if program_type == "drum":
        return DRUM_NOTES
    return MELODIC_NOTES


def duration_for_type(program_type: str) -> float:
    return DURATION_MAP.get(program_type, DURATION_MAP["melodic"])


def tail_for_type(program_type: str) -> float:
    return TAIL_MAP.get(program_type, TAIL_MAP["melodic"])


def load_xometa(path: str) -> dict:
    """Load and return a .xometa JSON file, tolerating minor format issues."""
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)


def preset_name_from_meta(data: dict, path: str) -> str:
    """Extract preset name from .xometa data, falling back to filename stem."""
    return data.get("name") or os.path.splitext(os.path.basename(path))[0]


def engine_from_meta(data: dict) -> str:
    """Extract canonical engine ID from .xometa data."""
    engines = data.get("engines") or data.get("engine") or []
    if isinstance(engines, str):
        engines = [engines]
    if engines:
        return canonicalize_engine(engines[0])
    return "UNKNOWN"


def mood_from_meta(data: dict, fallback: str = "") -> str:
    """Extract mood from .xometa data."""
    return data.get("mood") or fallback


def sonic_dna_from_meta(data: dict) -> dict:
    """
    Extract Sonic DNA from .xometa, handling all known key variants:
    sonic_dna, dna, sonicDNA
    """
    for key in ("sonic_dna", "dna", "sonicDNA"):
        val = data.get(key)
        if isinstance(val, dict):
            return val
    return {}


def preset_path_relative(path: str, presets_dir: str) -> str:
    """Return preset path relative to presets_dir if possible."""
    try:
        return os.path.relpath(path, presets_dir)
    except ValueError:
        return path


# ---------------------------------------------------------------------------
# Discovery
# ---------------------------------------------------------------------------

def find_xometa_files(presets_dir: str, engine_filter: str = None,
                      mood_filter: list = None) -> list:
    """
    Recursively find .xometa files under presets_dir.

    Applies optional engine and mood filters. Returns a sorted list of
    absolute file paths.
    """
    pattern = os.path.join(os.path.abspath(presets_dir), "**", "*.xometa")
    all_files = sorted(glob.glob(pattern, recursive=True))

    results = []
    for path in all_files:
        try:
            data = load_xometa(path)
        except Exception:
            continue

        # Engine filter
        if engine_filter:
            engine = engine_from_meta(data)
            if engine.upper() != engine_filter.upper():
                continue

        # Mood filter
        if mood_filter:
            mood = mood_from_meta(data)
            if mood not in mood_filter:
                continue

        results.append(path)

    return results


# ---------------------------------------------------------------------------
# Spec building
# ---------------------------------------------------------------------------

def build_program_entry(path: str, presets_dir: str) -> dict:
    """Build a single program entry for the render spec from a .xometa file."""
    data = load_xometa(path)

    preset_name = preset_name_from_meta(data, path)
    engine      = engine_from_meta(data)
    mood        = mood_from_meta(data)
    prog_type   = program_type_for_engine(engine)
    notes       = render_notes_for_type(prog_type)
    duration    = duration_for_type(prog_type)
    tail        = tail_for_type(prog_type)
    rel_path    = preset_path_relative(path, presets_dir)

    return {
        "preset_name":   preset_name,
        "preset_path":   rel_path,
        "program_type":  prog_type,
        "render_notes":  notes,
        "velocities":    VIBE_VELOCITIES,
        "duration_s":    duration,
        "tail_s":        tail,
    }


def build_render_spec(programs: list, pack_name: str, engine: str,
                      mood: str, version: str = VERSION) -> dict:
    """Assemble the top-level render_spec.json structure."""
    return {
        "pack_name": pack_name,
        "engine":    engine,
        "version":   version,
        "mood":      mood,
        "programs":  programs,
    }


# ---------------------------------------------------------------------------
# Text summary
# ---------------------------------------------------------------------------

def render_count(program: dict) -> int:
    return len(program["render_notes"]) * len(program["velocities"])


def format_text_summary(spec: dict, output_path: str) -> str:
    programs  = spec["programs"]
    total     = sum(render_count(p) for p in programs)
    drum_ct   = sum(1 for p in programs if p["program_type"] == "drum")
    melodic_ct = len(programs) - drum_ct

    # Rough estimate: average duration across programs
    avg_dur   = (sum(p["duration_s"] for p in programs) / len(programs)) if programs else 2.0
    est_min   = (total * avg_dur) / 60.0

    lines = [
        f"RENDER SPEC GENERATED — {spec['pack_name']}",
        "",
        f"Engine: {spec['engine']} | Mood: {spec['mood']} | Version: {spec['version']}",
        f"Programs: {len(programs)} | Drum: {drum_ct} | Melodic: {melodic_ct}",
        "",
    ]

    for p in programs:
        rc = render_count(p)
        ncount = len(p["render_notes"])
        vcount = len(p["velocities"])
        lines.append(
            f"  {p['preset_name']} — {p['program_type']} — "
            f"{ncount} notes x {vcount} velocities = {rc} renders "
            f"({p['duration_s']}s each)"
        )

    lines += [
        "",
        f"Total renders: {total}",
        f"Estimated render time: ~{est_min:.1f} min (at {avg_dur:.0f}s/render)",
        f"Output: {output_path}",
    ]

    return "\n".join(lines)


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def parse_args():
    parser = argparse.ArgumentParser(
        description=(
            "XPN Render Spec Generator — generate render_spec.json "
            "for the oxport pipeline from .xometa presets."
        ),
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument(
        "--presets-dir", required=True, metavar="DIR",
        help="Root directory containing .xometa preset files (searched recursively).",
    )
    parser.add_argument(
        "--engine", required=True, metavar="ENGINE",
        help="Engine to filter presets by (e.g. ONSET, ODYSSEY).",
    )
    parser.add_argument(
        "--pack-name", required=True, metavar="NAME",
        help='Pack name for the render spec (e.g. "Machine Gun Reef").',
    )
    parser.add_argument(
        "--mood", default="", metavar="MOOD[,MOOD...]",
        help=(
            "Filter presets by mood. Comma-separated for multiple moods "
            "(e.g. Foundation,Atmosphere). Omit to include all moods."
        ),
    )
    parser.add_argument(
        "--preset-list", default="", metavar="NAME[,NAME...]",
        help=(
            "Comma-separated list of specific preset names to include. "
            "If omitted, all matching presets are included."
        ),
    )
    parser.add_argument(
        "--output", default="render_spec.json", metavar="PATH",
        help="Output path for the render_spec.json (default: render_spec.json).",
    )
    parser.add_argument(
        "--format", choices=["text", "json"], default="text",
        help="Output format for stdout summary (default: text).",
    )
    parser.add_argument(
        "--version", default=VERSION, metavar="X.Y.Z",
        help=f"Version string for the spec (default: {VERSION}).",
    )
    return parser.parse_args()


def main():
    args = parse_args()

    presets_dir  = os.path.abspath(args.presets_dir)
    engine_id    = canonicalize_engine(args.engine)
    mood_list    = [m.strip() for m in args.mood.split(",") if m.strip()] if args.mood else None
    preset_allow = {n.strip() for n in args.preset_list.split(",") if n.strip()} if args.preset_list else None

    if not os.path.isdir(presets_dir):
        print(f"ERROR: --presets-dir '{presets_dir}' is not a directory.", file=sys.stderr)
        return 1

    # Discover presets
    xometa_files = find_xometa_files(presets_dir, engine_filter=engine_id, mood_filter=mood_list)

    if not xometa_files:
        print(
            f"WARNING: No .xometa files found for engine={engine_id} "
            f"mood={mood_list or 'any'} in {presets_dir}",
            file=sys.stderr,
        )

    # Build program entries
    programs = []
    skipped  = 0
    for path in xometa_files:
        try:
            entry = build_program_entry(path, presets_dir)
        except Exception as exc:
            print(f"  [WARN] Skipping {os.path.basename(path)}: {exc}", file=sys.stderr)
            skipped += 1
            continue

        # Apply preset-list filter (by name)
        if preset_allow and entry["preset_name"] not in preset_allow:
            continue

        programs.append(entry)

    # Primary mood for the spec header: first mood from mood_list, or blank
    spec_mood = (mood_list[0] if mood_list else "") or ""

    spec = build_render_spec(
        programs=programs,
        pack_name=args.pack_name,
        engine=engine_id,
        mood=spec_mood,
        version=args.version,
    )

    # Write JSON output
    output_path = os.path.abspath(args.output)
    output_dir  = os.path.dirname(output_path)
    if output_dir:
        os.makedirs(output_dir, exist_ok=True)

    with open(output_path, "w", encoding="utf-8") as f:
        json.dump(spec, f, indent=2)
        f.write("\n")

    # Print summary
    if args.format == "json":
        print(json.dumps(spec, indent=2))
    else:
        print(format_text_summary(spec, output_path))

    if skipped:
        print(f"\n[WARN] {skipped} preset(s) skipped due to parse errors.", file=sys.stderr)

    return 0


if __name__ == "__main__":
    sys.exit(main())
