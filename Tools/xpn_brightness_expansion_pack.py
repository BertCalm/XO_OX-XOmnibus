#!/usr/bin/env python3
"""
xpn_brightness_expansion_pack.py — XO_OX Brightness Expansion Pack Generator

Fleet DNA diversity report finding: brightness is the most compressed dimension
(77.6% of presets in 0.3-0.7 midrange). This tool generates stub presets at both
extreme ends to fill the void.

Two bands:
  ultra-bright  brightness 0.85-1.0  → Prism mood  (airy, crystalline, shimmer)
  ultra-dark    brightness 0.0-0.15  → Aether mood (murky, subterranean, void)

Usage:
    python xpn_brightness_expansion_pack.py --output-dir ./brightness_stubs
    python xpn_brightness_expansion_pack.py --band dark --engines ORCA,OVERDUB --count 6
    python xpn_brightness_expansion_pack.py --dry-run --band both --seed 42
"""

import argparse
import json
import os
import random
import sys
from pathlib import Path

# ---------------------------------------------------------------------------
# Engine Rosters
# ---------------------------------------------------------------------------

BRIGHT_ENGINES = [
    "OPTIC",
    "OVERWORLD",
    "ORIGAMI",
    "OBLIQUE",
    "ORACLE",
    "OBSCURA",
    "ORPHICA",
    "ODDFELIX",
]

DARK_ENGINES = [
    "ORCA",
    "OVERDUB",
    "OBSIDIAN",
    "OWLFISH",
    "OCEANIC",
    "OPAL",
    "ORACLE",
    "ODDOSCAR",
]

# ---------------------------------------------------------------------------
# Name Vocabularies
# ---------------------------------------------------------------------------

BRIGHT_WORDS = [
    "Crystalline",
    "Radiant",
    "Scintilla",
    "Luminous",
    "Solar",
    "Quartz",
    "Vitreous",
    "Glacial",
    "Coronal",
    "Spectral",
    "Phosphor",
    "Incandescent",
]

DARK_WORDS = [
    "Void",
    "Abyss",
    "Murk",
    "Obsidian",
    "Pitch",
    "Tenebrous",
    "Umbral",
    "Stygian",
    "Crepuscular",
    "Midnight",
    "Chthonic",
    "Tartarean",
]

# Evocative second words that pair well with the primaries
BRIGHT_SECOND = [
    "Veil",
    "Peak",
    "Surge",
    "Arc",
    "Drift",
    "Field",
    "Haze",
    "Crown",
    "Bloom",
    "Fringe",
    "Wave",
    "Core",
]

DARK_SECOND = [
    "Depth",
    "Fall",
    "Pulse",
    "Floor",
    "Tide",
    "Hollow",
    "Well",
    "Root",
    "Layer",
    "Vein",
    "Current",
    "Mass",
]

# ---------------------------------------------------------------------------
# Mood + DNA Profiles
# ---------------------------------------------------------------------------

BRIGHT_MOOD = "Prism"
DARK_MOOD = "Aether"

# DNA ranges per band: (min, max) for each dimension
BRIGHT_DNA_RANGES = {
    "brightness": (0.85, 1.0),
    "warmth": (0.1, 0.3),
    "movement": (0.4, 0.85),
    "density": (0.1, 0.5),
    "space": (0.6, 1.0),
    "aggression": (0.05, 0.4),
}

DARK_DNA_RANGES = {
    "brightness": (0.0, 0.15),
    "warmth": (0.6, 0.95),
    "movement": (0.15, 0.6),
    "density": (0.6, 0.95),
    "space": (0.3, 0.75),
    "aggression": (0.2, 0.7),
}

# Descriptor tags per band
BRIGHT_TAGS = [
    "airy",
    "crystalline",
    "shimmer",
    "bright",
    "expansive",
    "luminous",
    "prism",
    "radiant",
]

DARK_TAGS = [
    "murky",
    "subterranean",
    "void",
    "dark",
    "dense",
    "tenebrous",
    "aether",
    "warm",
]

# ---------------------------------------------------------------------------
# Engine → parameter prefix map (from CLAUDE.md)
# ---------------------------------------------------------------------------

ENGINE_PREFIX = {
    "ODDFELIX": "snap_",
    "ODDOSCAR": "morph_",
    "OVERDUB": "dub_",
    "ODYSSEY": "drift_",
    "OBLONG": "bob_",
    "OBESE": "fat_",
    "OVERBITE": "poss_",
    "ONSET": "perc_",
    "OVERWORLD": "ow_",
    "OPAL": "opal_",
    "ORBITAL": "orb_",
    "ORGANON": "organon_",
    "OUROBOROS": "ouro_",
    "OBSIDIAN": "obsidian_",
    "ORIGAMI": "origami_",
    "ORACLE": "oracle_",
    "OBSCURA": "obscura_",
    "OCEANIC": "ocean_",
    "OCELOT": "ocelot_",
    "OPTIC": "optic_",
    "OBLIQUE": "oblq_",
    "OSPREY": "osprey_",
    "OSTERIA": "osteria_",
    "OWLFISH": "owl_",
    "OHM": "ohm_",
    "ORPHICA": "orph_",
    "OBBLIGATO": "obbl_",
    "OTTONI": "otto_",
    "OLE": "ole_",
    "OMBRE": "ombre_",
    "ORCA": "orca_",
    "OCTOPUS": "octo_",
}

# ---------------------------------------------------------------------------
# Generation helpers
# ---------------------------------------------------------------------------


def _rand_dna(ranges: dict, rng: random.Random) -> dict:
    """Return a 6D Sonic DNA dict with values sampled within band ranges."""
    return {dim: round(rng.uniform(lo, hi), 3) for dim, (lo, hi) in ranges.items()}


def _unique_name(primary_pool: list, second_pool: list, used: set, rng: random.Random) -> str:
    """Pick a two-word name not already in `used`."""
    attempts = 0
    while True:
        name = f"{rng.choice(primary_pool)} {rng.choice(second_pool)}"
        if name not in used:
            used.add(name)
            return name
        attempts += 1
        if attempts > 500:
            # Fallback: append a number to guarantee uniqueness
            base = f"{rng.choice(primary_pool)} {rng.choice(second_pool)}"
            candidate = f"{base} {attempts}"
            used.add(candidate)
            return candidate


def _make_preset(
    engine: str,
    name: str,
    mood: str,
    dna: dict,
    tags: list,
    rng: random.Random,
    band: str,
) -> dict:
    """Build a minimal .xometa preset stub."""
    prefix = ENGINE_PREFIX.get(engine, engine.lower()[:4] + "_")

    # Macro defaults — CHARACTER reflects brightness extreme
    if band == "bright":
        char_macro = round(rng.uniform(0.7, 1.0), 3)
        space_macro = round(rng.uniform(0.6, 1.0), 3)
        movement_macro = round(rng.uniform(0.4, 0.85), 3)
        coupling_macro = round(rng.uniform(0.1, 0.5), 3)
    else:
        char_macro = round(rng.uniform(0.0, 0.35), 3)
        space_macro = round(rng.uniform(0.3, 0.7), 3)
        movement_macro = round(rng.uniform(0.15, 0.55), 3)
        coupling_macro = round(rng.uniform(0.3, 0.8), 3)

    # Pick a random subset of tags (3–5)
    num_tags = rng.randint(3, min(5, len(tags)))
    chosen_tags = rng.sample(tags, num_tags)

    preset = {
        "version": "1.0",
        "name": name,
        "engines": [engine],
        "mood": mood,
        "sonic_dna": dna,
        "tags": chosen_tags,
        "macros": {
            "M1_CHARACTER": char_macro,
            "M2_MOVEMENT": movement_macro,
            "M3_COUPLING": coupling_macro,
            "M4_SPACE": space_macro,
        },
        "parameters": {
            # Stub — real parameters will be filled by sound designer or
            # parameter-generation pass. Brightness band hint baked in.
            f"{prefix}brightness_hint": round(dna["brightness"], 3),
        },
        "coupling": [],
        "notes": (
            f"Brightness expansion stub — band: {band}. "
            "Parameters are placeholders; DNA values are target ranges."
        ),
    }
    return preset


def generate_band(
    band: str,
    engines: list,
    count: int,
    rng: random.Random,
) -> list:
    """Generate `count` preset stubs per engine for the given band."""
    if band == "bright":
        dna_ranges = BRIGHT_DNA_RANGES
        mood = BRIGHT_MOOD
        primary_pool = BRIGHT_WORDS
        second_pool = BRIGHT_SECOND
        tags = BRIGHT_TAGS
    else:
        dna_ranges = DARK_DNA_RANGES
        mood = DARK_MOOD
        primary_pool = DARK_WORDS
        second_pool = DARK_SECOND
        tags = DARK_TAGS

    presets = []
    used_names: set = set()

    for engine in engines:
        for _ in range(count):
            dna = _rand_dna(dna_ranges, rng)
            name = _unique_name(primary_pool, second_pool, used_names, rng)
            preset = _make_preset(engine, name, mood, dna, tags, rng, band)
            presets.append((engine, band, preset))

    return presets


# ---------------------------------------------------------------------------
# Output helpers
# ---------------------------------------------------------------------------


def preset_filename(engine: str, name: str) -> str:
    """Convert preset name to safe filename."""
    safe = name.replace(" ", "_").replace("/", "-")
    return f"{engine}_{safe}.xometa"


def write_presets(presets: list, output_dir: Path, dry_run: bool) -> int:
    """Write presets to output_dir. Returns count written (or would write)."""
    if not dry_run:
        output_dir.mkdir(parents=True, exist_ok=True)

    written = 0
    for engine, band, preset in presets:
        subdir = output_dir / band / engine
        if not dry_run:
            subdir.mkdir(parents=True, exist_ok=True)

        filename = preset_filename(engine, preset["name"])
        filepath = subdir / filename

        if dry_run:
            print(f"[DRY-RUN] Would write: {filepath}")
        else:
            filepath.write_text(json.dumps(preset, indent=2) + "\n", encoding="utf-8")
            print(f"  Wrote: {filepath}")

        written += 1

    return written


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------


def build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        prog="xpn_brightness_expansion_pack",
        description=(
            "Generate extreme-brightness preset stubs to fill fleet DNA gaps. "
            "Ultra-bright (0.85-1.0) → Prism mood. "
            "Ultra-dark (0.0-0.15) → Aether mood."
        ),
    )
    p.add_argument(
        "--output-dir",
        type=Path,
        default=Path("brightness_expansion_stubs"),
        help="Root directory for output preset files (default: ./brightness_expansion_stubs)",
    )
    p.add_argument(
        "--dry-run",
        action="store_true",
        help="Print what would be written without creating any files",
    )
    p.add_argument(
        "--band",
        choices=["bright", "dark", "both"],
        default="both",
        help="Which brightness band to generate (default: both)",
    )
    p.add_argument(
        "--engines",
        type=str,
        default=None,
        help=(
            "Comma-separated list of engine IDs to include "
            "(default: canonical bright/dark engine lists). "
            "Example: --engines OPTIC,ORCA,ORACLE"
        ),
    )
    p.add_argument(
        "--count",
        type=int,
        default=4,
        help="Number of presets per engine per band (default: 4)",
    )
    p.add_argument(
        "--seed",
        type=int,
        default=None,
        help="Random seed for reproducible output",
    )
    return p


def resolve_engines(band: str, engines_arg) -> tuple:
    """Return (bright_engines, dark_engines) based on --band and --engines args."""
    if engines_arg:
        override = [e.strip().upper() for e in engines_arg.split(",") if e.strip()]
        bright = override if band in ("bright", "both") else []
        dark = override if band in ("dark", "both") else []
        return bright, dark

    bright = BRIGHT_ENGINES if band in ("bright", "both") else []
    dark = DARK_ENGINES if band in ("dark", "both") else []
    return bright, dark


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()

    rng = random.Random(args.seed)

    bright_engines, dark_engines = resolve_engines(args.band, args.engines)

    all_presets: list = []

    if bright_engines:
        print(
            f"\nGenerating ultra-bright presets ({args.count}/engine) "
            f"for engines: {', '.join(bright_engines)}"
        )
        all_presets.extend(generate_band("bright", bright_engines, args.count, rng))

    if dark_engines:
        print(
            f"\nGenerating ultra-dark presets ({args.count}/engine) "
            f"for engines: {', '.join(dark_engines)}"
        )
        all_presets.extend(generate_band("dark", dark_engines, args.count, rng))

    if not all_presets:
        print("No presets generated — check --band and --engines arguments.")
        return 0

    print(f"\nTotal presets: {len(all_presets)}")
    print(f"Output dir:    {args.output_dir.resolve()}")
    if args.dry_run:
        print("Dry-run mode — no files will be written.\n")

    written = write_presets(all_presets, args.output_dir, args.dry_run)

    # Summary
    bright_count = sum(1 for _, b, _ in all_presets if b == "bright")
    dark_count = sum(1 for _, b, _ in all_presets if b == "dark")

    print(f"\n{'[DRY-RUN] ' if args.dry_run else ''}Summary:")
    if bright_count:
        print(f"  Ultra-bright (Prism, brightness 0.85-1.0): {bright_count} presets")
    if dark_count:
        print(f"  Ultra-dark   (Aether, brightness 0.0-0.15): {dark_count} presets")
    print(f"  Total: {written} presets {'would be ' if args.dry_run else ''}written")

    return 0


if __name__ == "__main__":
    sys.exit(main())
