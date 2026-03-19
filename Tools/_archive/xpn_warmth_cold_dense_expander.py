#!/usr/bin/env python3
"""
xpn_warmth_cold_dense_expander.py

Populates the four under-represented warmth/density quadrants in the XOmnibus
preset fleet.  The fleet currently clusters 80.7% of presets in warmth 0.45-0.75
and 77.7% in density 0.45-0.75.  This tool fills the extremes.

Quadrants generated
───────────────────
  Hot Dense   warmth 0.80-1.0  density 0.80-1.0  mood: Foundation  (20 presets)
  Cold Sparse warmth 0.00-0.20 density 0.00-0.20  mood: Prism       (20 presets)
  Hot Sparse  warmth 0.80-1.0  density 0.00-0.20  mood: Atmosphere  (20 presets)
  Cold Dense  warmth 0.00-0.20 density 0.80-1.0   mood: Aether      (20 presets)

Total: 80 presets.  Existing files are never overwritten.

Usage
─────
  python3 xpn_warmth_cold_dense_expander.py
  python3 xpn_warmth_cold_dense_expander.py --dry-run
  python3 xpn_warmth_cold_dense_expander.py --seed 42
  python3 xpn_warmth_cold_dense_expander.py --quadrant hot_dense
"""

import json
import os
import argparse
import random
from pathlib import Path

# ─────────────────────────────────────────────────────────────────────────────
# Engine roster per quadrant
# ─────────────────────────────────────────────────────────────────────────────

QUADRANT_ENGINES = {
    "hot_dense": [
        "ODDOSCAR", "OVERDUB", "OBESE", "OVERBITE", "ORGANON",
        "OSTERIA", "OHM", "OTTONI", "ORCA",
    ],
    "cold_sparse": [
        "ODDFELIX", "OVERWORLD", "OPTIC", "OBLIQUE", "ORIGAMI",
        "OBSCURA", "ONSET",
    ],
    "hot_sparse": [
        "ORACLE", "OPAL", "OLE", "OSPREY", "ORPHICA", "OCEANIC",
    ],
    "cold_dense": [
        "OBSIDIAN", "OUROBOROS", "OBBLIGATO", "ORBITAL", "OWLFISH", "OCTOPUS",
    ],
}

# Canonical engine name casing used in .xometa files
ENGINE_DISPLAY = {
    "ODDOSCAR":   "OddOscar",
    "OVERDUB":    "OVERDUB",
    "OBESE":      "OBESE",
    "OVERBITE":   "OVERBITE",
    "ORGANON":    "ORGANON",
    "OSTERIA":    "OSTERIA",
    "OHM":        "OHM",
    "OTTONI":     "OTTONI",
    "ORCA":       "ORCA",
    "ODDFELIX":   "OddfeliX",
    "OVERWORLD":  "OVERWORLD",
    "OPTIC":      "OPTIC",
    "OBLIQUE":    "OBLIQUE",
    "ORIGAMI":    "ORIGAMI",
    "OBSCURA":    "OBSCURA",
    "ONSET":      "ONSET",
    "ORACLE":     "ORACLE",
    "OPAL":       "Opal",
    "OLE":        "OLE",
    "OSPREY":     "OSPREY",
    "ORPHICA":    "ORPHICA",
    "OCEANIC":    "OCEANIC",
    "OBSIDIAN":   "OBSIDIAN",
    "OUROBOROS":  "OUROBOROS",
    "OBBLIGATO":  "OBBLIGATO",
    "ORBITAL":    "ORBITAL",
    "OWLFISH":    "XOwlfish",
    "OCTOPUS":    "OCTOPUS",
}

# ─────────────────────────────────────────────────────────────────────────────
# Quadrant configuration
# ─────────────────────────────────────────────────────────────────────────────

QUADRANT_CONFIG = {
    "hot_dense": {
        "mood":         "Foundation",
        "subdir":       "Foundation",
        "warmth_range": (0.80, 1.00),
        "density_range": (0.80, 1.00),
        "brightness_range": (0.25, 0.65),
        "space_range":  (0.10, 0.45),
        "aggression_range": (0.35, 0.75),
        "movement_range": (0.30, 0.65),
        "base_tags":    ["foundation", "warm", "dense", "analog", "thick"],
        "macro_pools": [
            ["CHARACTER", "DRIVE", "COUPLING", "SPACE"],
            ["WARMTH", "WEIGHT", "COUPLING", "DEPTH"],
            ["BODY", "SATURATE", "COUPLING", "ROOM"],
            ["MELT", "COMPRESS", "COUPLING", "TAIL"],
        ],
        "description_pool": [
            "Saturated low-end mass — every harmonic stacked and warm.",
            "Dense analog body with thick sustain and minimal air.",
            "Molten core tone. Filter barely open, heat radiating outward.",
            "Sub-heavy foundation with slow bloom and settled weight.",
            "Packed formant stack — rich overtones, no breath to spare.",
            "Compressor glue holding a dense harmonic mass in place.",
            "Wool-thick pad with heavy body resonance and amber warmth.",
            "Tar-slow attack into a saturated, immovable bed of sound.",
            "Low-frequency anchor: all density, no shimmer, pure earth.",
            "Cauldron harmonic — boiling slowly, dense as cast iron.",
        ],
    },
    "cold_sparse": {
        "mood":         "Prism",
        "subdir":       "Prism",
        "warmth_range": (0.00, 0.20),
        "density_range": (0.00, 0.20),
        "brightness_range": (0.55, 0.95),
        "space_range":  (0.60, 1.00),
        "aggression_range": (0.00, 0.25),
        "movement_range": (0.10, 0.40),
        "base_tags":    ["prism", "cold", "sparse", "crystalline", "airy"],
        "macro_pools": [
            ["SCATTER", "DRIFT", "COUPLING", "SPACE"],
            ["FREEZE", "GLINT", "COUPLING", "VOID"],
            ["PRISM", "DISTANCE", "COUPLING", "TAIL"],
            ["CRYSTAL", "BREATH", "COUPLING", "OPEN"],
        ],
        "description_pool": [
            "Single photon trace — sparse, cold, absolutely precise.",
            "Ice-thin harmonic. One partial, infinite room.",
            "Frigid air column with no warmth, no weight, pure geometry.",
            "Crystal lattice tone — intervals measured, density absent.",
            "Sub-zero shimmer. The silence between notes is the sound.",
            "Refracted spectrum: one cold sine per octave, wide open.",
            "Arctic breath tone. Maximum air, minimum body.",
            "Frozen millisecond — cold attack, instant decay, vast tail.",
            "Sparse prism scatter — each partial isolated in cold space.",
            "Glass filament vibration, barely there, barely audible.",
        ],
    },
    "hot_sparse": {
        "mood":         "Atmosphere",
        "subdir":       "Atmosphere",
        "warmth_range": (0.80, 1.00),
        "density_range": (0.00, 0.20),
        "brightness_range": (0.45, 0.80),
        "space_range":  (0.55, 0.95),
        "aggression_range": (0.05, 0.35),
        "movement_range": (0.40, 0.75),
        "base_tags":    ["atmosphere", "warm", "sparse", "airy", "breath"],
        "macro_pools": [
            ["SCATTER", "DRIFT", "COUPLING", "SPACE"],
            ["WARMTH", "BREATH", "COUPLING", "OPEN"],
            ["GLOW", "WANDER", "COUPLING", "AIR"],
            ["BLOOM", "EXHALE", "COUPLING", "HORIZON"],
        ],
        "description_pool": [
            "Warm breath on cold glass — heat without weight.",
            "Golden haze, no mass. Warmth floats free of density.",
            "Single warm partial in vast open space — glowing, alone.",
            "Sun through fog: radiant warmth, minimal presence.",
            "Thermal drift — a sparse harmonic rising on warm air.",
            "Amber filament in open sky — warm, sparse, weightless.",
            "One warm voice exhaling slowly into cathedral space.",
            "Desert heat shimmer: warm glow, no groundmass at all.",
            "Rare-earth warmth — light touch, maximum space around it.",
            "Sparse solar wind: each warm partial wide apart, drifting.",
        ],
    },
    "cold_dense": {
        "mood":         "Aether",
        "subdir":       "Aether",
        "warmth_range": (0.00, 0.20),
        "density_range": (0.80, 1.00),
        "brightness_range": (0.20, 0.55),
        "space_range":  (0.20, 0.55),
        "aggression_range": (0.30, 0.70),
        "movement_range": (0.20, 0.55),
        "base_tags":    ["aether", "cold", "dense", "dark", "pressure"],
        "macro_pools": [
            ["CYCLE", "CONSUME", "COUPLING", "VOID"],
            ["PRESSURE", "DEPTH", "COUPLING", "MASS"],
            ["COLLAPSE", "FEEDBACK", "COUPLING", "DARK"],
            ["GRAVITY", "CRUSH", "COUPLING", "DESCENT"],
        ],
        "description_pool": [
            "Cold iron mass — dense, toneless, infinite pressure.",
            "Abyssal density without warmth: dark matter in audio form.",
            "Frozen harmonic pile-up. Cold, packed, unrelenting.",
            "Sub-zero pressure wave — thick and cold, no amber anywhere.",
            "Dark plasma density: cold layers stacked to event horizon.",
            "Permafrost texture — dense and cold as geological strata.",
            "Void accumulation: cold resonance packed into black mass.",
            "Negative thermal density. Mass without heat, depth without air.",
            "Cryogenic thickness — cold partials locked together tightly.",
            "Deep ocean pressure, cold and dense, nothing escapes.",
        ],
    },
}

# ─────────────────────────────────────────────────────────────────────────────
# Evocative 2-3 word name vocabulary per quadrant
# ─────────────────────────────────────────────────────────────────────────────

NAME_VOCAB = {
    "hot_dense": {
        "A": [
            "Amber", "Molten", "Smolder", "Hearth", "Cauldron",
            "Humid", "Sinter", "Velvet", "Tar", "Boil",
            "Embers", "Pressure", "Cast", "Dense", "Forge",
        ],
        "B": [
            "Core", "Mass", "Weight", "Floor", "Body",
            "Bed", "Clay", "Resin", "Crush", "Bloom",
            "Iron", "Drift", "Root", "Pulse", "Vein",
        ],
    },
    "cold_sparse": {
        "A": [
            "Frost", "Glacial", "Crystal", "Arctic", "Prism",
            "Sleet", "Clear", "Frozen", "Void", "Glass",
            "Ice", "Sparse", "Null", "Thin", "Pale",
        ],
        "B": [
            "Filament", "Trace", "Glint", "Breath", "Scatter",
            "Point", "Edge", "Sliver", "Shard", "Mote",
            "Geometry", "Line", "Gap", "Distance", "Veil",
        ],
    },
    "hot_sparse": {
        "A": [
            "Amber", "Solar", "Thermal", "Warm", "Golden",
            "Radiant", "Desert", "Glow", "Bloom", "Rare",
            "Sun", "Haze", "Luminous", "Soft", "Dusk",
        ],
        "B": [
            "Breath", "Drift", "Exhale", "Air", "Horizon",
            "Mist", "Float", "Filament", "Wander", "Rise",
            "Shimmer", "Open", "Alone", "Sky", "Thread",
        ],
    },
    "cold_dense": {
        "A": [
            "Abyssal", "Frozen", "Dark", "Cold", "Void",
            "Permafrost", "Cryogenic", "Obsidian", "Null", "Deep",
            "Pressure", "Black", "Iron", "Gravity", "Dense",
        ],
        "B": [
            "Mass", "Collapse", "Descent", "Floor", "Crush",
            "Strata", "Weight", "Accumulation", "Density", "Horizon",
            "Core", "Layer", "Pressure", "Void", "Pull",
        ],
    },
}

# Optional 3rd word for some names (used ~40% of the time)
THIRD_WORDS = [
    "I", "II", "III", "Low", "Deep", "Dark", "Core",
    "Slow", "Pure", "Raw", "Wide", "Near", "Far",
]

# ─────────────────────────────────────────────────────────────────────────────
# Helpers
# ─────────────────────────────────────────────────────────────────────────────

def clamp(v, lo=0.0, hi=1.0):
    return max(lo, min(hi, v))


def jitter(base, spread, rng):
    return clamp(base + rng.uniform(-spread, spread))


def rand_in(lo, hi, rng, spread=0.04):
    base = rng.uniform(lo + spread, hi - spread)
    return round(clamp(base + rng.uniform(-spread, spread), lo, hi), 3)


def make_name(quadrant, used, rng):
    vocab = NAME_VOCAB[quadrant]
    for _ in range(50):
        a = rng.choice(vocab["A"])
        b = rng.choice(vocab["B"])
        if rng.random() < 0.40:
            c = rng.choice(THIRD_WORDS)
            candidate = f"{a} {b} {c}"
        else:
            candidate = f"{a} {b}"
        if candidate not in used:
            used.add(candidate)
            return candidate
    # Fallback with index
    n = len(used) + 1
    candidate = f"{rng.choice(vocab['A'])} {rng.choice(vocab['B'])} {n}"
    used.add(candidate)
    return candidate


def build_dna(cfg, rng):
    warmth    = rand_in(*cfg["warmth_range"], rng)
    density   = rand_in(*cfg["density_range"], rng)
    brightness = rand_in(*cfg["brightness_range"], rng)
    space     = rand_in(*cfg["space_range"], rng)
    aggression = rand_in(*cfg["aggression_range"], rng)
    movement  = rand_in(*cfg["movement_range"], rng)
    return {
        "brightness": brightness,
        "warmth":     warmth,
        "movement":   movement,
        "density":    density,
        "space":      space,
        "aggression": aggression,
    }


def build_preset(quadrant, engine, name, cfg, rng):
    dna = build_dna(cfg, rng)
    macro_labels = rng.choice(cfg["macro_pools"])
    description = rng.choice(cfg["description_pool"])
    display_engine = ENGINE_DISPLAY.get(engine, engine)

    # Vary tags slightly
    tags = list(cfg["base_tags"])
    if dna["aggression"] > 0.5:
        tags.append("aggressive")
    if dna["movement"] > 0.55:
        tags.append("evolving")
    if dna["space"] > 0.7:
        tags.append("spacious")
    if dna["brightness"] > 0.7:
        tags.append("bright")

    preset = {
        "schema_version": 1,
        "name":           name,
        "mood":           cfg["mood"],
        "sonic_dna":      dna,
        "engines":        [display_engine],
        "author":         "XO_OX",
        "version":        "1.0.0",
        "description":    description,
        "tags":           tags,
        "macroLabels":    macro_labels,
        "couplingIntensity": "None",
        "dna":            dna,
        "parameters":     {},
        "coupling":       {"pairs": []},
    }
    return preset


# ─────────────────────────────────────────────────────────────────────────────
# File I/O
# ─────────────────────────────────────────────────────────────────────────────

def safe_filename(name):
    return name.replace(" ", "_").replace("/", "-").replace("\\", "-") + ".xometa"


def write_preset(preset, out_dir, dry_run):
    filename = safe_filename(preset["name"])
    filepath = out_dir / filename
    if filepath.exists():
        return filepath, "skipped"
    if dry_run:
        return filepath, "dry-run"
    out_dir.mkdir(parents=True, exist_ok=True)
    with open(filepath, "w", encoding="utf-8") as f:
        json.dump(preset, f, indent=2)
    return filepath, "written"


# ─────────────────────────────────────────────────────────────────────────────
# Generation
# ─────────────────────────────────────────────────────────────────────────────

TARGET_PER_QUADRANT = 20


def generate_quadrant(quadrant, repo_root, rng, dry_run, verbose):
    cfg = QUADRANT_CONFIG[quadrant]
    engines = QUADRANT_ENGINES[quadrant]
    out_dir = repo_root / "Presets" / "XOmnibus" / cfg["subdir"]

    total = TARGET_PER_QUADRANT
    used_names = set()
    stats = {"written": 0, "skipped": 0, "dry-run": 0}

    # Distribute evenly across engines, cycling if total > len(engines)
    presets_data = []
    for i in range(total):
        engine = engines[i % len(engines)]
        name = make_name(quadrant, used_names, rng)
        preset = build_preset(quadrant, engine, name, cfg, rng)
        presets_data.append((preset, engine))

    for preset, engine in presets_data:
        fp, status = write_preset(preset, out_dir, dry_run)
        stats[status] += 1
        if verbose or dry_run:
            marker = {"written": "+", "skipped": "=", "dry-run": "~"}[status]
            print(f"  [{marker}] {cfg['subdir']}/{fp.name}  ({engine})")

    return stats


# ─────────────────────────────────────────────────────────────────────────────
# CLI
# ─────────────────────────────────────────────────────────────────────────────

QUADRANT_CHOICES = ["hot_dense", "cold_sparse", "hot_sparse", "cold_dense", "all"]


def parse_args():
    parser = argparse.ArgumentParser(
        description="Fill warmth/density extreme quadrants in the XOmnibus preset fleet.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument(
        "--quadrant",
        choices=QUADRANT_CHOICES,
        default="all",
        help="Which quadrant to generate (default: all)",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print what would be written without creating any files.",
    )
    parser.add_argument(
        "--seed",
        type=int,
        default=None,
        help="Random seed for reproducible output.",
    )
    parser.add_argument(
        "--verbose",
        action="store_true",
        help="Print every file path even when not in dry-run.",
    )
    return parser.parse_args()


def main():
    args = parse_args()
    rng = random.Random(args.seed)
    repo_root = Path(__file__).resolve().parent.parent

    quadrants = (
        list(QUADRANT_CONFIG.keys())
        if args.quadrant == "all"
        else [args.quadrant]
    )

    print("XPN Warmth/Cold Dense Expander")
    print(f"  Repo root : {repo_root}")
    print(f"  Quadrants : {', '.join(quadrants)}")
    print(f"  Target    : {TARGET_PER_QUADRANT} presets per quadrant")
    print(f"  Dry run   : {args.dry_run}")
    print(f"  Seed      : {args.seed if args.seed is not None else 'random'}")
    print()

    grand_total = {"written": 0, "skipped": 0, "dry-run": 0}

    for quadrant in quadrants:
        cfg = QUADRANT_CONFIG[quadrant]
        engines = QUADRANT_ENGINES[quadrant]
        label = quadrant.replace("_", " ").title()
        print(f"── {label}  →  {cfg['mood']}  ({len(engines)} engines)")
        stats = generate_quadrant(quadrant, repo_root, rng, args.dry_run, args.verbose)
        for k, v in stats.items():
            grand_total[k] += v
        tag = "dry-run" if args.dry_run else "written"
        count = stats[tag] if args.dry_run else stats["written"]
        print(f"   {count} presets {tag}, {stats['skipped']} skipped")
        print()

    print("── Summary")
    if args.dry_run:
        print(f"   {grand_total['dry-run']} presets would be written")
    else:
        print(f"   {grand_total['written']} written, {grand_total['skipped']} skipped (already exist)")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
