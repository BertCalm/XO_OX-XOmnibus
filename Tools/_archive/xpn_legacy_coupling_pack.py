#!/usr/bin/env python3
"""
xpn_legacy_coupling_pack.py — Generate Entangled coupling preset stubs for
high-affinity legacy engine pairs that currently have zero Entangled coverage.

Context: coupling density heatmap found 456/561 engine pairs have zero Entangled
presets. The most-coupled engine is OBLONG (79 presets). This tool targets the
20 highest-affinity uncovered pairs based on DNA complementarity and thematic
affinity.

Usage:
    python Tools/xpn_legacy_coupling_pack.py
    python Tools/xpn_legacy_coupling_pack.py --dry-run
    python Tools/xpn_legacy_coupling_pack.py --count 5 --seed 42
    python Tools/xpn_legacy_coupling_pack.py --pairs "OPAL+OBSIDIAN,ONSET+OBLONG"
    python Tools/xpn_legacy_coupling_pack.py --output-dir /tmp/stubs
"""

import argparse
import json
import math
import os
import pathlib
import random
import sys

# ---------------------------------------------------------------------------
# DNA personality baselines (brightness, warmth, movement, density, space, aggression)
# ---------------------------------------------------------------------------
ENGINE_DNA = {
    "OPAL":      {"brightness": 0.70, "warmth": 0.50, "movement": 0.75, "density": 0.45, "space": 0.80, "aggression": 0.20},
    "OBSIDIAN":  {"brightness": 0.90, "warmth": 0.20, "movement": 0.30, "density": 0.50, "space": 0.60, "aggression": 0.10},
    "ORACLE":    {"brightness": 0.50, "warmth": 0.40, "movement": 0.60, "density": 0.70, "space": 0.70, "aggression": 0.40},
    "OBSCURA":   {"brightness": 0.40, "warmth": 0.60, "movement": 0.35, "density": 0.60, "space": 0.65, "aggression": 0.20},
    "OCEANIC":   {"brightness": 0.65, "warmth": 0.55, "movement": 0.50, "density": 0.55, "space": 0.75, "aggression": 0.30},
    "ORBITAL":   {"brightness": 0.60, "warmth": 0.65, "movement": 0.55, "density": 0.60, "space": 0.60, "aggression": 0.50},
    "OUROBOROS": {"brightness": 0.50, "warmth": 0.40, "movement": 0.85, "density": 0.75, "space": 0.50, "aggression": 0.80},
    "ORIGAMI":   {"brightness": 0.70, "warmth": 0.45, "movement": 0.65, "density": 0.55, "space": 0.55, "aggression": 0.55},
    "OVERWORLD": {"brightness": 0.75, "warmth": 0.40, "movement": 0.50, "density": 0.65, "space": 0.55, "aggression": 0.60},
    "OBESE":     {"brightness": 0.50, "warmth": 0.70, "movement": 0.45, "density": 0.80, "space": 0.40, "aggression": 0.75},
    "ODYSSEY":   {"brightness": 0.55, "warmth": 0.50, "movement": 0.70, "density": 0.50, "space": 0.70, "aggression": 0.30},
    "OVERDUB":   {"brightness": 0.45, "warmth": 0.70, "movement": 0.55, "density": 0.55, "space": 0.75, "aggression": 0.25},
    "OWLFISH":   {"brightness": 0.40, "warmth": 0.60, "movement": 0.40, "density": 0.65, "space": 0.80, "aggression": 0.20},
    "OSPREY":    {"brightness": 0.55, "warmth": 0.55, "movement": 0.50, "density": 0.60, "space": 0.70, "aggression": 0.35},
    "OSTERIA":   {"brightness": 0.40, "warmth": 0.75, "movement": 0.45, "density": 0.70, "space": 0.60, "aggression": 0.45},
    "ORGANON":   {"brightness": 0.60, "warmth": 0.60, "movement": 0.65, "density": 0.75, "space": 0.65, "aggression": 0.40},
    "OCELOT":    {"brightness": 0.65, "warmth": 0.60, "movement": 0.60, "density": 0.55, "space": 0.60, "aggression": 0.50},
    "OBLIQUE":   {"brightness": 0.75, "warmth": 0.40, "movement": 0.70, "density": 0.50, "space": 0.60, "aggression": 0.55},
    "ONSET":     {"brightness": 0.55, "warmth": 0.50, "movement": 0.80, "density": 0.75, "space": 0.50, "aggression": 0.70},
    "OBLONG":    {"brightness": 0.60, "warmth": 0.65, "movement": 0.50, "density": 0.65, "space": 0.55, "aggression": 0.50},
    "OPTIC":     {"brightness": 0.85, "warmth": 0.30, "movement": 0.90, "density": 0.40, "space": 0.50, "aggression": 0.45},
}

# ---------------------------------------------------------------------------
# Priority pair list — (engineA, engineB, thematic_tag)
# ---------------------------------------------------------------------------
PRIORITY_PAIRS = [
    ("OPAL",      "OBSIDIAN",  "granular-crystal"),
    ("ORACLE",    "OBSCURA",   "prophetic-daguerreotype"),
    ("OCEANIC",   "ORBITAL",   "phosphorescent-warm"),
    ("OUROBOROS", "ORIGAMI",   "chaos-fold"),
    ("OVERWORLD", "OBESE",     "chip-fat"),
    ("ODYSSEY",   "OVERDUB",   "drifting-tape"),
    ("OWLFISH",   "ORACLE",    "abyssal-prophetic"),
    ("OSPREY",    "OSTERIA",   "coastal-harbour"),
    ("ORGANON",   "OUROBOROS", "metabolism-chaos"),
    ("OCELOT",    "OBLIQUE",   "biome-prism"),
    ("ONSET",     "OBLONG",    "drum-foundational"),
    ("ORIGAMI",   "OPTIC",     "fold-visual"),
    ("ORBITAL",   "ORACLE",    "group-prophecy"),
    ("OBLIQUE",   "OPAL",      "prism-granular"),
    ("OBSCURA",   "ORGANON",   "daguerreotype-metabolism"),
    ("OCEANIC",   "OPAL",      "teal-lavender"),
    ("OVERWORLD", "ORACLE",    "chip-prophecy"),
    ("OBESE",     "OVERDUB",   "fat-tape"),
    ("OWLFISH",   "OCEANIC",   "abyssal-teal"),
    ("OSPREY",    "OCEANIC",   "coastal-phosphorescent"),
]

COUPLING_TYPES = ["SYNC_LFO", "FILTER_MOD", "AMP_MOD", "PITCH_MOD", "FORMANT_CROSS"]

# ---------------------------------------------------------------------------
# Per-pair evocative name banks (3 names each, deterministic with seed)
# ---------------------------------------------------------------------------
PAIR_NAMES = {
    ("OPAL", "OBSIDIAN"): [
        "Crystal Nebula", "Frozen Spectrum", "Vitreous Bloom"
    ],
    ("ORACLE", "OBSCURA"): [
        "Phantom Oracle", "Silver Prophecy", "Daguerreotype Vision"
    ],
    ("OCEANIC", "ORBITAL"): [
        "Warm Phosphor", "Tidal Warmth", "Orbital Current"
    ],
    ("OUROBOROS", "ORIGAMI"): [
        "Folded Serpent", "Chaos Crease", "Recursive Fold"
    ],
    ("OVERWORLD", "OBESE"): [
        "Fat Pixel", "Saturated Era", "Chip Compression"
    ],
    ("ODYSSEY", "OVERDUB"): [
        "Drifting Tape", "Delayed Voyage", "Capstan Dream"
    ],
    ("OWLFISH", "ORACLE"): [
        "Deep Prophecy", "Abyssal Seer", "Pressure Vision"
    ],
    ("OSPREY", "OSTERIA"): [
        "Harbour Wing", "Shore Tavern", "Coastal Cellar"
    ],
    ("ORGANON", "OUROBOROS"): [
        "Metabolic Spiral", "Cellular Chaos", "Living Loop"
    ],
    ("OCELOT", "OBLIQUE"): [
        "Biome Prism", "Tawny Refraction", "Savanna Scatter"
    ],
    ("ONSET", "OBLONG"): [
        "Drum Foundation", "Strike Amber", "Percussive Warmth"
    ],
    ("ORIGAMI", "OPTIC"): [
        "Visual Fold", "Phosphor Crease", "Light Geometry"
    ],
    ("ORBITAL", "ORACLE"): [
        "Group Prophecy", "Harmonic Vision", "Spectral Council"
    ],
    ("OBLIQUE", "OPAL"): [
        "Prism Grain", "Violet Scatter", "Refracted Cloud"
    ],
    ("OBSCURA", "ORGANON"): [
        "Silver Metabolism", "Daguerreotype Cell", "Chemical Memory"
    ],
    ("OCEANIC", "OPAL"): [
        "Teal Lavender", "Bioluminescent Grain", "Phosphor Bloom"
    ],
    ("OVERWORLD", "ORACLE"): [
        "Pixel Prophecy", "Chiptune Oracle", "8-Bit Vision"
    ],
    ("OBESE", "OVERDUB"): [
        "Fat Tape", "Saturated Delay", "Drive and Flutter"
    ],
    ("OWLFISH", "OCEANIC"): [
        "Gold Teal", "Abyssal Shimmer", "Deep Phosphor"
    ],
    ("OSPREY", "OCEANIC"): [
        "Coastal Phosphor", "Shore Luminescence", "Tidal Wing"
    ],
}

# Engine ID to parameter prefix mapping (frozen — never rename after release)
ENGINE_PREFIX = {
    "OPAL":      "opal_",
    "OBSIDIAN":  "obsidian_",
    "ORACLE":    "oracle_",
    "OBSCURA":   "obscura_",
    "OCEANIC":   "ocean_",
    "ORBITAL":   "orb_",
    "OUROBOROS": "ouro_",
    "ORIGAMI":   "origami_",
    "OVERWORLD": "ow_",
    "OBESE":     "fat_",
    "ODYSSEY":   "drift_",
    "OVERDUB":   "dub_",
    "OWLFISH":   "owl_",
    "OSPREY":    "osprey_",
    "OSTERIA":   "osteria_",
    "ORGANON":   "organon_",
    "OCELOT":    "ocelot_",
    "OBLIQUE":   "oblq_",
    "ONSET":     "perc_",
    "OBLONG":    "bob_",
    "OPTIC":     "optic_",
}

# Representative stub parameters per engine (minimal, plausible defaults)
ENGINE_STUB_PARAMS = {
    "OPAL":      ["grainSize", "grainDensity", "filterCutoff", "reverbMix", "couplingLevel"],
    "OBSIDIAN":  ["depth", "shimmer", "filterCutoff", "spaceMix", "couplingLevel"],
    "ORACLE":    ["breakpoints", "stochasticRate", "filterCutoff", "reverbMix", "couplingLevel"],
    "OBSCURA":   ["stiffness", "exposure", "filterCutoff", "spaceMix", "couplingLevel"],
    "OCEANIC":   ["separation", "chromatophore", "filterCutoff", "reverbMix", "couplingLevel"],
    "ORBITAL":   ["brightness", "groupEnv", "filterCutoff", "warmth", "couplingLevel"],
    "OUROBOROS": ["topology", "leash", "filterCutoff", "velCoupling", "couplingLevel"],
    "ORIGAMI":   ["foldPoint", "foldDepth", "filterCutoff", "spaceMix", "couplingLevel"],
    "OVERWORLD": ["era", "noiseLevel", "filterCutoff", "reverbMix", "couplingLevel"],
    "OBESE":     ["satDrive", "mojo", "filterCutoff", "density", "couplingLevel"],
    "ODYSSEY":   ["oscA_mode", "driftRate", "filterCutoff", "spaceMix", "couplingLevel"],
    "OVERDUB":   ["oscWave", "sendAmount", "filterCutoff", "reverbMix", "couplingLevel"],
    "OWLFISH":   ["filterCutoff", "subMix", "grainMix", "reverbMix", "couplingLevel"],
    "OSPREY":    ["shoreBlend", "coastalDrift", "filterCutoff", "spaceMix", "couplingLevel"],
    "OSTERIA":   ["qBassShore", "wineDepth", "filterCutoff", "warmth", "couplingLevel"],
    "ORGANON":   ["metabolicRate", "cellDensity", "filterCutoff", "spaceMix", "couplingLevel"],
    "OCELOT":    ["biome", "spotRate", "filterCutoff", "warmth", "couplingLevel"],
    "OBLIQUE":   ["prismColor", "bounceRate", "filterCutoff", "spaceMix", "couplingLevel"],
    "ONSET":     ["noiseLevel", "attackSharp", "filterCutoff", "velSens", "couplingLevel"],
    "OBLONG":    ["fltCutoff", "envDepth", "oscLevel", "reverbMix", "couplingLevel"],
    "OPTIC":     ["pulseRate", "autoSync", "visualDepth", "flickerRate", "couplingLevel"],
}

# Macro label banks per pair thematic tag
MACRO_BANKS = {
    "granular-crystal":        ["SCATTER", "CLARITY", "DRIFT", "SPACE"],
    "prophetic-daguerreotype": ["VISION", "EXPOSURE", "FADE", "DEPTH"],
    "phosphorescent-warm":     ["GLOW", "WARMTH", "ORBIT", "SPACE"],
    "chaos-fold":              ["ENTROPY", "CREASE", "LEASH", "SPACE"],
    "chip-fat":                ["ERA", "DRIVE", "COMPRESS", "SPACE"],
    "drifting-tape":           ["DRIFT", "FLUTTER", "DELAY", "SPACE"],
    "abyssal-prophetic":       ["DEPTH", "VISION", "PRESSURE", "SPACE"],
    "coastal-harbour":         ["SHORE", "WARMTH", "TIDE", "SPACE"],
    "metabolism-chaos":        ["ENERGY", "ENTROPY", "LEASH", "SPACE"],
    "biome-prism":             ["TERRAIN", "SCATTER", "HUNT", "SPACE"],
    "drum-foundational":       ["PUNCH", "WARMTH", "SPACE", "DRIVE"],
    "fold-visual":             ["CREASE", "PULSE", "LIGHT", "SPACE"],
    "group-prophecy":          ["HARMONY", "VISION", "GROUP", "SPACE"],
    "prism-granular":          ["SCATTER", "REFRACT", "GRAIN", "SPACE"],
    "daguerreotype-metabolism": ["EXPOSURE", "ENERGY", "FADE", "SPACE"],
    "teal-lavender":           ["LUMINANCE", "GRAIN", "DRIFT", "SPACE"],
    "chip-prophecy":           ["ERA", "VISION", "PIXEL", "SPACE"],
    "fat-tape":                ["DRIVE", "FLUTTER", "WEIGHT", "SPACE"],
    "abyssal-teal":            ["DEPTH", "GLOW", "PRESSURE", "SPACE"],
    "coastal-phosphorescent":  ["SHORE", "GLOW", "TIDE", "SPACE"],
}

COUPLING_INTENSITY_CHOICES = ["Light", "Medium", "Heavy"]


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def blend_dna(dna_a: dict, dna_b: dict, rng: random.Random, jitter: float = 0.08) -> dict:
    """Blend two DNA dicts with equal weight, then apply ±jitter."""
    keys = ["brightness", "warmth", "movement", "density", "space", "aggression"]
    result = {}
    for k in keys:
        mid = (dna_a[k] + dna_b[k]) / 2.0
        j = rng.uniform(-jitter, jitter)
        result[k] = round(max(0.0, min(1.0, mid + j)), 3)
    return result


def stub_parameters(engine: str, dna: dict, rng: random.Random) -> dict:
    """Generate minimal plausible parameter values for one engine."""
    prefix = ENGINE_PREFIX.get(engine, engine.lower()[:4] + "_")
    param_names = ENGINE_STUB_PARAMS.get(engine, ["filterCutoff", "level", "couplingLevel"])
    params = {}
    for name in param_names:
        key = prefix + name
        if name == "couplingLevel":
            params[key] = round(rng.uniform(0.55, 0.85), 2)
        elif name in ("filterCutoff", "fltCutoff"):
            # Map brightness → filter openness
            params[key] = round(dna["brightness"] * 0.6 + rng.uniform(-0.1, 0.1), 2)
        elif name in ("reverbMix", "spaceMix"):
            params[key] = round(dna["space"] * 0.6 + rng.uniform(-0.05, 0.05), 2)
        elif name in ("grainSize", "grainDensity"):
            params[key] = round(dna["density"] * 0.7 + rng.uniform(-0.1, 0.1), 2)
        else:
            params[key] = round(rng.uniform(0.3, 0.75), 2)
    return params


def make_coupling_route(engine_a: str, engine_b: str, coupling_type: str, rng: random.Random) -> dict:
    """Build a coupling route object."""
    return {
        "source": engine_a,
        "target": engine_b,
        "type": coupling_type,
        "amount": round(rng.uniform(0.35, 0.75), 2),
        "bidirectional": rng.random() > 0.5,
    }


def pair_key(a: str, b: str):
    return (min(a, b), max(a, b))


def generate_stub(
    engine_a: str,
    engine_b: str,
    thematic_tag: str,
    stub_index: int,
    name: str,
    rng: random.Random,
) -> dict:
    """Generate one .xometa stub dict for an engine pair."""
    dna_a = ENGINE_DNA.get(engine_a, {k: 0.5 for k in ["brightness","warmth","movement","density","space","aggression"]})
    dna_b = ENGINE_DNA.get(engine_b, {k: 0.5 for k in ["brightness","warmth","movement","density","space","aggression"]})
    blended = blend_dna(dna_a, dna_b, rng)

    coupling_type = rng.choice(COUPLING_TYPES)
    intensity = rng.choice(COUPLING_INTENSITY_CHOICES)
    macros = MACRO_BANKS.get(thematic_tag, ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"])

    tags = [
        "coupling", "entangled",
        engine_a.lower(), engine_b.lower(),
        thematic_tag.replace("-", " ").split()[0],
    ]

    params_a = stub_parameters(engine_a, blended, rng)
    params_b = stub_parameters(engine_b, blended, rng)

    description = (
        f"{engine_a} × {engine_b} coupling — {thematic_tag} "
        f"via {coupling_type.replace('_', ' ').lower()}. "
        f"Blended DNA stub generated by xpn_legacy_coupling_pack."
    )

    return {
        "schema_version": 1,
        "name": name,
        "mood": "Entangled",
        "sonic_dna": blended,
        "engines": [engine_a, engine_b],
        "author": "XO_OX",
        "version": "1.0.0",
        "description": description,
        "tags": tags,
        "macroLabels": macros,
        "couplingIntensity": intensity,
        "dna": blended,
        "parameters": {
            engine_a: params_a,
            engine_b: params_b,
        },
        "coupling": {
            "pairs": [
                make_coupling_route(engine_a, engine_b, coupling_type, rng)
            ],
            "type": coupling_type,
        },
    }


def sanitize_filename(name: str) -> str:
    """Convert preset name to safe filename (no spaces → underscores)."""
    safe = name.replace(" ", "_").replace("/", "-").replace("\\", "-")
    return safe + ".xometa"


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def parse_args():
    parser = argparse.ArgumentParser(
        description="Generate Entangled coupling preset stubs for legacy engine pairs."
    )
    parser.add_argument(
        "--output-dir",
        default=None,
        help="Output directory for .xometa files. Default: Presets/XOceanus/Entangled/ relative to repo root.",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print what would be written without creating files.",
    )
    parser.add_argument(
        "--count",
        type=int,
        default=3,
        help="Number of stubs to generate per pair (default: 3).",
    )
    parser.add_argument(
        "--seed",
        type=int,
        default=None,
        help="Random seed for reproducible output.",
    )
    parser.add_argument(
        "--pairs",
        type=str,
        default=None,
        help='Comma-separated subset of pairs to generate, e.g. "OPAL+OBSIDIAN,ONSET+OBLONG".',
    )
    return parser.parse_args()


def resolve_output_dir(output_dir_arg: str) -> pathlib.Path:
    if output_dir_arg:
        return pathlib.Path(output_dir_arg)
    # Try to locate repo root relative to this script
    script_dir = pathlib.Path(__file__).resolve().parent
    repo_root = script_dir.parent
    candidate = repo_root / "Presets" / "XOceanus" / "Entangled"
    return candidate


def parse_pairs_filter(pairs_str: str):
    """Parse --pairs argument into a set of normalised pair keys."""
    result = set()
    for token in pairs_str.split(","):
        token = token.strip()
        if "+" in token:
            parts = [p.strip().upper() for p in token.split("+", 1)]
            if len(parts) == 2:
                result.add(pair_key(parts[0], parts[1]))
    return result


def main():
    args = parse_args()

    rng = random.Random(args.seed)

    # Filter pairs if --pairs supplied
    pairs_filter = None
    if args.pairs:
        pairs_filter = parse_pairs_filter(args.pairs)
        if not pairs_filter:
            print("ERROR: --pairs argument produced no valid pairs. Use format 'ENGINEA+ENGINEB'.", file=sys.stderr)
            sys.exit(1)

    active_pairs = []
    for engine_a, engine_b, tag in PRIORITY_PAIRS:
        pk = pair_key(engine_a, engine_b)
        if pairs_filter is not None and pk not in pairs_filter:
            continue
        active_pairs.append((engine_a, engine_b, tag))

    if not active_pairs:
        print("No pairs matched the --pairs filter.", file=sys.stderr)
        sys.exit(1)

    output_dir = resolve_output_dir(args.output_dir)

    if not args.dry_run:
        output_dir.mkdir(parents=True, exist_ok=True)

    total_written = 0
    total_planned = 0
    skipped_name_collision = 0

    for engine_a, engine_b, tag in active_pairs:
        # Fetch name bank, fallback to generic names
        pk = pair_key(engine_a, engine_b)
        name_bank = PAIR_NAMES.get(pk, PAIR_NAMES.get((engine_a, engine_b), None))
        if name_bank is None:
            name_bank = [
                f"{engine_a} {engine_b} I",
                f"{engine_a} {engine_b} II",
                f"{engine_a} {engine_b} III",
            ]

        count = min(args.count, len(name_bank))
        # If more stubs requested than names in bank, generate extras with numeric suffix
        names = list(name_bank[:count])
        if args.count > len(name_bank):
            for i in range(len(name_bank), args.count):
                names.append(f"{engine_a} {engine_b} {i + 1}")

        for idx, name in enumerate(names):
            total_planned += 1
            stub = generate_stub(engine_a, engine_b, tag, idx, name, rng)
            filename = sanitize_filename(name)
            filepath = output_dir / filename

            if args.dry_run:
                print(f"[DRY-RUN] Would write: {filepath}")
                print(f"          engines={engine_a}+{engine_b}  tag={tag}  coupling={stub['coupling']['type']}  intensity={stub['couplingIntensity']}")
            else:
                if filepath.exists():
                    # Avoid overwriting existing presets — append pair suffix
                    stem = f"{engine_a}_{engine_b}_{idx+1}"
                    filename = sanitize_filename(stem + " " + name)
                    filepath = output_dir / filename
                    if filepath.exists():
                        skipped_name_collision += 1
                        print(f"SKIP (collision): {filepath}")
                        continue

                with open(filepath, "w", encoding="utf-8") as f:
                    json.dump(stub, f, indent=2)
                    f.write("\n")
                print(f"WRITTEN: {filepath.name}")
                total_written += 1

    # Summary
    pair_count = len(active_pairs)
    print()
    if args.dry_run:
        print(f"[DRY-RUN] Would generate {total_planned} stubs across {pair_count} pairs → {output_dir}")
    else:
        print(f"Generated {total_written}/{total_planned} stubs across {pair_count} pairs → {output_dir}")
        if skipped_name_collision:
            print(f"Skipped {skipped_name_collision} stubs due to filename collisions (already exist).")

    sys.exit(0)


if __name__ == "__main__":
    main()
