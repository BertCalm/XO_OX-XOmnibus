#!/usr/bin/env python3
"""
xpn_family_portrait_generator.py — XOmnibus Family Portrait Preset Generator

Generates "Family Portrait" presets — curated groupings of 3–5 engines that tell
a coherent sonic story. Unlike coupling presets, portraits have no modulation routes;
each engine contributes an independent voice in an ensemble.

Each portrait concept produces 2 variants:
  • Warm DNA  — blend of engine baselines, warmth +0.1 from neutral
  • Cool DNA  — blend of engine baselines, warmth -0.1 from neutral

All presets land in mood: Family.

Usage:
    python Tools/xpn_family_portrait_generator.py
    python Tools/xpn_family_portrait_generator.py --dry-run
    python Tools/xpn_family_portrait_generator.py --output-dir /tmp/portraits
    python Tools/xpn_family_portrait_generator.py --concepts "The Water Column,The Dreamers"
    python Tools/xpn_family_portrait_generator.py --seed 42
"""

import argparse
import json
import os
import pathlib
import random

# ---------------------------------------------------------------------------
# 6D Sonic DNA baselines for all 34 registered XOmnibus engines
# ---------------------------------------------------------------------------
ENGINE_DNA: dict[str, dict[str, float]] = {
    # Foundation / character voices
    "ODDFELIX":  {"brightness": 0.75, "warmth": 0.55, "movement": 0.60, "density": 0.50, "space": 0.50, "aggression": 0.35},
    "ODDOSCAR":  {"brightness": 0.55, "warmth": 0.65, "movement": 0.65, "density": 0.55, "space": 0.55, "aggression": 0.30},
    # Tape / dub
    "OVERDUB":   {"brightness": 0.45, "warmth": 0.70, "movement": 0.55, "density": 0.55, "space": 0.75, "aggression": 0.25},
    # Wavetable drift
    "ODYSSEY":   {"brightness": 0.55, "warmth": 0.50, "movement": 0.70, "density": 0.50, "space": 0.70, "aggression": 0.30},
    # Analog curiosity
    "OBLONG":    {"brightness": 0.60, "warmth": 0.65, "movement": 0.50, "density": 0.65, "space": 0.55, "aggression": 0.50},
    # Fat saturation
    "OBESE":     {"brightness": 0.50, "warmth": 0.70, "movement": 0.45, "density": 0.80, "space": 0.40, "aggression": 0.75},
    # Percussion
    "ONSET":     {"brightness": 0.55, "warmth": 0.50, "movement": 0.80, "density": 0.75, "space": 0.50, "aggression": 0.70},
    # Chip / era
    "OVERWORLD": {"brightness": 0.75, "warmth": 0.40, "movement": 0.50, "density": 0.65, "space": 0.55, "aggression": 0.60},
    # Granular
    "OPAL":      {"brightness": 0.70, "warmth": 0.50, "movement": 0.75, "density": 0.45, "space": 0.80, "aggression": 0.20},
    # Group envelope
    "ORBITAL":   {"brightness": 0.60, "warmth": 0.65, "movement": 0.55, "density": 0.60, "space": 0.60, "aggression": 0.50},
    # Metabolic / biomorphic
    "ORGANON":   {"brightness": 0.60, "warmth": 0.60, "movement": 0.65, "density": 0.75, "space": 0.65, "aggression": 0.40},
    # Chaotic / attractor
    "OUROBOROS": {"brightness": 0.50, "warmth": 0.40, "movement": 0.85, "density": 0.75, "space": 0.50, "aggression": 0.80},
    # Crystal / glass
    "OBSIDIAN":  {"brightness": 0.90, "warmth": 0.20, "movement": 0.30, "density": 0.50, "space": 0.60, "aggression": 0.10},
    # Bass / bite
    "OVERBITE":  {"brightness": 0.45, "warmth": 0.60, "movement": 0.50, "density": 0.75, "space": 0.40, "aggression": 0.65},
    # Fold / paper
    "ORIGAMI":   {"brightness": 0.70, "warmth": 0.45, "movement": 0.65, "density": 0.55, "space": 0.55, "aggression": 0.55},
    # Stochastic / prophetic
    "ORACLE":    {"brightness": 0.50, "warmth": 0.40, "movement": 0.60, "density": 0.70, "space": 0.70, "aggression": 0.40},
    # Daguerreotype / memory
    "OBSCURA":   {"brightness": 0.40, "warmth": 0.60, "movement": 0.35, "density": 0.60, "space": 0.65, "aggression": 0.20},
    # Aquatic / teal
    "OCEANIC":   {"brightness": 0.65, "warmth": 0.55, "movement": 0.50, "density": 0.55, "space": 0.75, "aggression": 0.30},
    # Biome / tawny
    "OCELOT":    {"brightness": 0.65, "warmth": 0.60, "movement": 0.60, "density": 0.55, "space": 0.60, "aggression": 0.50},
    # Visual / phosphor
    "OPTIC":     {"brightness": 0.85, "warmth": 0.30, "movement": 0.90, "density": 0.40, "space": 0.50, "aggression": 0.45},
    # Prismatic / electronic
    "OBLIQUE":   {"brightness": 0.75, "warmth": 0.40, "movement": 0.70, "density": 0.50, "space": 0.60, "aggression": 0.55},
    # Shore / coastal
    "OSPREY":    {"brightness": 0.55, "warmth": 0.55, "movement": 0.50, "density": 0.60, "space": 0.70, "aggression": 0.35},
    # Harbour / wine
    "OSTERIA":   {"brightness": 0.40, "warmth": 0.75, "movement": 0.45, "density": 0.70, "space": 0.60, "aggression": 0.45},
    # Abyssal / deep
    "OWLFISH":   {"brightness": 0.40, "warmth": 0.60, "movement": 0.40, "density": 0.65, "space": 0.80, "aggression": 0.20},
    # Hippy jam / commune
    "OHM":       {"brightness": 0.50, "warmth": 0.70, "movement": 0.55, "density": 0.55, "space": 0.65, "aggression": 0.20},
    # Microsound harp
    "ORPHICA":   {"brightness": 0.65, "warmth": 0.50, "movement": 0.60, "density": 0.40, "space": 0.70, "aggression": 0.15},
    # Dual wind
    "OBBLIGATO": {"brightness": 0.55, "warmth": 0.60, "movement": 0.55, "density": 0.60, "space": 0.65, "aggression": 0.30},
    # Triple brass
    "OTTONI":    {"brightness": 0.70, "warmth": 0.55, "movement": 0.60, "density": 0.65, "space": 0.55, "aggression": 0.50},
    # Afro-Latin trio
    "OLE":       {"brightness": 0.65, "warmth": 0.65, "movement": 0.75, "density": 0.60, "space": 0.50, "aggression": 0.45},
    # Shadow / mauve
    "OMBRE":     {"brightness": 0.35, "warmth": 0.55, "movement": 0.45, "density": 0.55, "space": 0.70, "aggression": 0.25},
    # Apex predator
    "ORCA":      {"brightness": 0.60, "warmth": 0.35, "movement": 0.70, "density": 0.70, "space": 0.60, "aggression": 0.80},
    # Octopus / CA arms
    "OCTOPUS":   {"brightness": 0.65, "warmth": 0.40, "movement": 0.85, "density": 0.75, "space": 0.55, "aggression": 0.60},
    # FDN reverb / knot topology
    "OVERLAP":   {"brightness": 0.55, "warmth": 0.55, "movement": 0.50, "density": 0.65, "space": 0.85, "aggression": 0.25},
    # Wolfram CA / outwit
    "OUTWIT":    {"brightness": 0.60, "warmth": 0.40, "movement": 0.80, "density": 0.70, "space": 0.50, "aggression": 0.65},
}

_DIMS = ["brightness", "warmth", "movement", "density", "space", "aggression"]
_DEFAULT_DNA = {d: 0.5 for d in _DIMS}


def _engine_dna(engine_name: str) -> dict[str, float]:
    """Return baseline DNA for an engine, falling back to neutral 0.5."""
    return ENGINE_DNA.get(engine_name.upper(), _DEFAULT_DNA.copy())


def _blend_dna(engines: list[str]) -> dict[str, float]:
    """Average the DNA baselines of all listed engines."""
    n = len(engines)
    if n == 0:
        return _DEFAULT_DNA.copy()
    blended = {d: 0.0 for d in _DIMS}
    for eng in engines:
        dna = _engine_dna(eng)
        for d in _DIMS:
            blended[d] += dna.get(d, 0.5)
    return {d: round(blended[d] / n, 3) for d in _DIMS}


def _warm_variant(base: dict[str, float]) -> dict[str, float]:
    """Warm portrait: warmth +0.1, clamped [0, 1]."""
    v = dict(base)
    v["warmth"] = round(min(1.0, v["warmth"] + 0.1), 3)
    return v


def _cool_variant(base: dict[str, float]) -> dict[str, float]:
    """Cool portrait: warmth -0.1, clamped [0, 1]."""
    v = dict(base)
    v["warmth"] = round(max(0.0, v["warmth"] - 0.1), 3)
    return v


# ---------------------------------------------------------------------------
# 15 Portrait Concepts
# ---------------------------------------------------------------------------
PORTRAITS: list[dict] = [
    {
        "name": "The Water Column",
        "engines": ["OCEANIC", "OPAL", "ORGANON", "OWLFISH", "ONSET"],
        "description": (
            "Five engines spanning the full aquatic depth range: tidal surface tones, "
            "granular mid-water shimmer, metabolic bioluminescence, abyssal resonance, "
            "and rhythmic percussion from below. A complete vertical portrait of the water column."
        ),
    },
    {
        "name": "The Shore Meeting",
        "engines": ["OSPREY", "OSTERIA", "OCEANIC"],
        "description": (
            "The ShoreSystem family convenes: coastal wing voices, harbour warmth, "
            "and phosphorescent teal. Three engines sharing the same shoreline blueprint "
            "painted in complementary cultural hues."
        ),
    },
    {
        "name": "The Rhythm Section",
        "engines": ["ONSET", "OBLONG", "OBESE", "OVERBITE"],
        "description": (
            "The percussion family united: drum machine precision, analog amber curiosity, "
            "saturated low-end warmth, and bass bite character. Four engines that "
            "together nail the rhythmic backbone of any production."
        ),
    },
    {
        "name": "The Dreamers",
        "engines": ["ODYSSEY", "OPAL", "ORACLE", "OBSCURA"],
        "description": (
            "Four evocative engines adrift in memory and imagination: tidal wavetable "
            "drift, granular texture clouds, stochastic prophecy, and daguerreotype "
            "silver resonance. An introspective quartet for deep listening."
        ),
    },
    {
        "name": "The Architects",
        "engines": ["OVERWORLD", "ORBITAL", "ORIGAMI", "ORACLE"],
        "description": (
            "Structure-first engines: ERA triangle chiptune, grouped envelope systems, "
            "geometric folded resonance, and deterministic prophecy. Four voices for "
            "builders who design sound with intention."
        ),
    },
    {
        "name": "The Chaos Theory",
        "engines": ["OUROBOROS", "OUTWIT", "ORACLE", "OPTIC"],
        "description": (
            "Four stochastic engines in volatile conversation: strange attractor loops, "
            "Wolfram cellular automata arms, GENDY stochastic synthesis, and autonomous "
            "visual modulation pulses. Unpredictability as a compositional tool."
        ),
    },
    {
        "name": "The Brass Band",
        "engines": ["OTTONI", "OBBLIGATO", "OHM", "OLE"],
        "description": (
            "The Constellation winds assembled: triple brass ensemble, dual woodwind "
            "obligation, hippy commune resonance, and Afro-Latin drama. Four engines "
            "from the same Constellation fast-track lineage playing together."
        ),
    },
    {
        "name": "The Microsound Collective",
        "engines": ["ORPHICA", "OPAL", "OCEANIC", "ORIGAMI"],
        "description": (
            "Texture-first engines working at the grain level: siphonophore microsound "
            "harp, granular clouds, aquatic chromatic modulation, and geometric "
            "paper-fold resonance. A study in density without mass."
        ),
    },
    {
        "name": "The Deep Six",
        "engines": ["ORCA", "OCEANIC", "OWLFISH", "OVERDUB", "OPAL", "ORACLE"],
        "description": (
            "Six engines of deep water: apex predator wavetable, phosphorescent teal, "
            "abyssal Mixtur-Trautonium, tape delay reverb tail, granular shimmer, "
            "and prophetic stochastic voice. A full immersion portrait."
        ),
    },
    {
        "name": "The Analog Chain",
        "engines": ["OBESE", "OVERDUB", "OVERBITE", "OBLONG"],
        "description": (
            "Four engines of analog warmth chained end to end: fat saturation, tape "
            "echo character, bass bite, and amber analog curiosity. The classic "
            "outboard chain reimagined as a synthesis portrait."
        ),
    },
    {
        "name": "The Digital Vanguard",
        "engines": ["OPTIC", "OBLIQUE", "OVERWORLD", "OUTWIT"],
        "description": (
            "Four electronic engines at the frontier: phosphor visual modulation, "
            "prismatic bounce synthesis, chiptune ERA morphing, and Wolfram cellular "
            "automata. Uncompromisingly digital, uncompromisingly expressive."
        ),
    },
    {
        "name": "The Alien Collective",
        "engines": ["OCTOPUS", "OUTWIT", "OPTIC", "ORACLE"],
        "description": (
            "Four non-human intelligence engines: decentralized octopus arm synthesis, "
            "Wolfram CA pattern generation, zero-audio visual intelligence, and "
            "GENDY stochastic prophecy. Sound from minds that do not think like us."
        ),
    },
    {
        "name": "The Predators",
        "engines": ["ORCA", "OCELOT", "OUROBOROS", "ONSET"],
        "description": (
            "The hunt family: apex echolocation wavetable, biome tawny pursuit, "
            "strange attractor chaos loops, and precision percussion strike. "
            "Four engines united by aggression, timing, and the forward edge."
        ),
    },
    {
        "name": "The Memory Palace",
        "engines": ["OMBRE", "OVERDUB", "ORACLE", "OBSCURA"],
        "description": (
            "Four engines of memory, time, and image: shadow mauve dual-narrative, "
            "tape delay ghost trails, stochastic prophecy recall, and daguerreotype "
            "silver resonance. A portrait of what is almost remembered."
        ),
    },
    {
        "name": "The Complete Ensemble",
        "engines": ["ODDFELIX", "ODDOSCAR", "OVERDUB", "OBLONG", "ONSET"],
        "description": (
            "The founding XO_OX family gathered: feliX the neon tetra, Oscar the "
            "axolotl, tape echo warmth, amber curiosity, and drum machine pulse. "
            "The original voices of the XO_OX universe in one portrait."
        ),
    },
]


# ---------------------------------------------------------------------------
# Preset builder
# ---------------------------------------------------------------------------

def _make_preset(
    portrait: dict,
    variant: str,
    dna: dict[str, float],
) -> dict:
    """Build a single .xometa portrait preset dict."""
    engines = portrait["engines"]
    suffix = "Warm" if variant == "warm" else "Cool"
    name = f"{portrait['name']} ({suffix})"

    tags = (
        [e.lower() for e in engines]
        + ["portrait", "ensemble", "family"]
    )

    return {
        "schema_version": 1,
        "name": name,
        "mood": "Family",
        "engines": engines,
        "author": "XO_OX Designs",
        "version": "1.0.0",
        "description": portrait["description"],
        "tags": tags,
        "macroLabels": ["CHARACTER", "MOVEMENT", "BLEND", "SPACE"],
        "tempo": None,
        "dna": dna,
        "parameters": {},
        "coupling": None,
        "sequencer": None,
    }


# ---------------------------------------------------------------------------
# Write helper
# ---------------------------------------------------------------------------

def _safe_filename(name: str) -> str:
    """Convert portrait name to a filesystem-safe filename."""
    return name.replace(" ", "_").replace("/", "-").replace("(", "").replace(")", "") + ".xometa"


def _write_preset(preset: dict, output_dir: pathlib.Path, dry_run: bool) -> str:
    """Write preset to disk; return the destination path string."""
    dest = output_dir / _safe_filename(preset["name"])
    if not dry_run:
        output_dir.mkdir(parents=True, exist_ok=True)
        with open(dest, "w", encoding="utf-8") as fh:
            json.dump(preset, fh, indent=2)
            fh.write("\n")
    return str(dest)


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def _parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate XOmnibus Family Portrait presets (2 variants per concept).",
    )
    parser.add_argument(
        "--output-dir",
        default=None,
        help="Destination directory (default: <repo>/Presets/XOmnibus/Family/)",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print what would be written without touching the filesystem.",
    )
    parser.add_argument(
        "--concepts",
        default=None,
        help="Comma-separated portrait names to generate (default: all 15).",
    )
    parser.add_argument(
        "--seed",
        type=int,
        default=None,
        help="Random seed for reproducibility (currently unused but wired for future use).",
    )
    return parser.parse_args()


def _resolve_output_dir(arg) -> pathlib.Path:
    if arg:
        return pathlib.Path(arg)
    # Default: <repo root>/Presets/XOmnibus/Family/
    repo_root = pathlib.Path(__file__).parent.parent
    return repo_root / "Presets" / "XOmnibus" / "Family"


def main() -> None:
    args = _parse_args()

    if args.seed is not None:
        random.seed(args.seed)

    output_dir = _resolve_output_dir(args.output_dir)

    # Filter concepts
    portraits = PORTRAITS
    if args.concepts:
        requested = {c.strip().lower() for c in args.concepts.split(",")}
        portraits = [p for p in PORTRAITS if p["name"].lower() in requested]
        if not portraits:
            print(f"ERROR: No concepts matched. Available: {', '.join(p['name'] for p in PORTRAITS)}")
            raise SystemExit(1)

    generated: list[str] = []
    skipped: list[str] = []

    for portrait in portraits:
        base_dna = _blend_dna(portrait["engines"])
        warm_dna = _warm_variant(base_dna)
        cool_dna = _cool_variant(base_dna)

        for variant, dna in [("warm", warm_dna), ("cool", cool_dna)]:
            preset = _make_preset(portrait, variant, dna)
            dest = _write_preset(preset, output_dir, dry_run=args.dry_run)
            action = "[DRY RUN]" if args.dry_run else "wrote"
            print(f"  {action}  {dest}")
            generated.append(dest)

    print()
    print(f"{'[DRY RUN] Would generate' if args.dry_run else 'Generated'} "
          f"{len(generated)} presets from {len(portraits)} portrait concept(s).")
    if skipped:
        print(f"  Skipped: {len(skipped)}")


if __name__ == "__main__":
    main()
