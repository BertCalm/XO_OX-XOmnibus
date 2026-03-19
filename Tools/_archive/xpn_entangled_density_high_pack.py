#!/usr/bin/env python3
"""
xpn_entangled_density_high_pack.py
Generates 80 .xometa presets targeting Entangled mood with density >= 0.88 (XHIGH zone).
Output: Presets/Entangled/
"""

import json
import os
import random

ENGINES = [
    "ODDFELIX", "ODDOSCAR", "OVERDUB", "ODYSSEY", "OBLONG", "OBESE",
    "ONSET", "OVERWORLD", "OPAL", "ORBITAL", "ORGANON", "OUROBOROS",
    "OBSIDIAN", "OVERBITE", "ORIGAMI", "ORACLE", "OBSCURA", "OCEANIC",
    "OCELOT", "OPTIC", "OBLIQUE", "OSPREY", "OSTERIA", "OWLFISH",
    "OHM", "ORPHICA", "OBBLIGATO", "OTTONI", "OLE", "OMBRE",
    "ORCA", "OCTOPUS", "OVERLAP", "OUTWIT"
]

COUPLING_TYPES = [
    "FREQUENCY_SHIFT", "AMPLITUDE_MOD", "FILTER_MOD", "PITCH_SYNC",
    "TIMBRE_BLEND", "ENVELOPE_LINK", "HARMONIC_FOLD", "CHAOS_INJECT",
    "RESONANCE_SHARE", "SPATIAL_COUPLE", "SPECTRAL_MORPH", "VELOCITY_COUPLE"
]

NAME_PARTS_A = [
    "MASS", "GRANITE", "DENSITY", "PRESSURE", "PACKED", "TECTONIC",
    "IRON", "STONE", "CONCRETE", "BEDROCK", "MONOLITH", "SLAB",
    "BULK", "WEIGHT", "CRUSH", "VOID", "CARBON", "TUNGSTEN",
    "LEADEN", "FORGE", "ANVIL", "BASALT", "OBSIDIAN", "OSMIUM",
    "DARK", "DEEP", "THICK", "HEAVY", "DENSE", "SOLID",
    "LOCKED", "FUSED", "WELDED", "BOUND", "SEALED", "ENTANGLED",
    "KNOTTED", "COILED", "PACKED", "STACKED", "LAYERED", "COMPRESSED"
]

NAME_PARTS_B = [
    "WALL", "BLOCK", "FIELD", "ZONE", "LAYERS", "MASS",
    "CORE", "SHELL", "PLATE", "GRID", "WAVE", "PULSE",
    "SURGE", "FLOOD", "TIDE", "CLOUD", "STORM", "FRONT",
    "MATTER", "FORM", "BODY", "SHAPE", "STRUCTURE", "LATTICE",
    "NEXUS", "NODE", "KNOT", "LINK", "CHAIN", "BOND",
    "MESH", "NET", "WEB", "FABRIC", "WEAVE", "THREAD",
    "CRUSH", "PRESS", "FORCE", "DRIVE", "PUSH", "LOCK"
]

TAGS_POOL = [
    "entangled", "dense", "massive", "thick", "heavy", "crushing",
    "pressure", "tectonic", "monolithic", "layered", "packed",
    "granite", "iron", "extreme-density", "xhigh", "solid",
    "industrial", "dark", "deep", "fused", "welded", "coiled",
    "stacked", "compressed", "bound"
]


def r(lo=0.0, hi=1.0):
    return round(random.uniform(lo, hi), 4)


def generate_name(used_names):
    for _ in range(1000):
        a = random.choice(NAME_PARTS_A)
        b = random.choice(NAME_PARTS_B)
        name = f"{a} {b}"
        if name not in used_names:
            return name
    # fallback with index
    return f"DENSITY FIELD {len(used_names)+1}"


def generate_preset(name, engine1, engine2, coupling_type):
    density = round(random.uniform(0.88, 0.98), 4)
    tags = random.sample(TAGS_POOL, k=random.randint(3, 6))
    if "entangled" not in tags:
        tags.insert(0, "entangled")
    if "dense" not in tags:
        tags.insert(1, "dense")

    return {
        "name": name,
        "version": "1.0",
        "mood": "Entangled",
        "engines": [engine1, engine2],
        "parameters": {
            engine1: {
                "macro_character": r(),
                "macro_movement": r(),
                "macro_coupling": r(0.6, 1.0),
                "macro_space": r()
            },
            engine2: {
                "macro_character": r(),
                "macro_movement": r(),
                "macro_coupling": r(0.6, 1.0),
                "macro_space": r()
            }
        },
        "coupling": {
            "type": coupling_type,
            "source": engine1,
            "target": engine2,
            "amount": r(0.7, 1.0)
        },
        "dna": {
            "brightness": r(),
            "warmth": r(),
            "movement": r(),
            "density": density,
            "space": r(),
            "aggression": r()
        },
        "macros": {
            "CHARACTER": r(),
            "MOVEMENT": r(),
            "COUPLING": r(0.7, 1.0),
            "SPACE": r()
        },
        "tags": tags
    }


def main():
    random.seed(42)

    repo_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    out_dir = os.path.join(repo_root, "Presets", "Entangled")
    os.makedirs(out_dir, exist_ok=True)

    target = 80
    written = 0
    skipped = 0
    used_names = set()

    # Build a broad set of engine pairs covering all 34 engines
    # We cycle through coupling types evenly
    engine_pairs = []
    engines_shuffled = ENGINES[:]
    random.shuffle(engines_shuffled)

    # Generate pairs ensuring good engine variety
    all_engines = ENGINES[:]
    used_engines = set()
    pairs_pool = []

    # First pass: pair each engine with a different engine
    random.shuffle(all_engines)
    for i in range(0, len(all_engines) - 1, 2):
        pairs_pool.append((all_engines[i], all_engines[i + 1]))

    # Fill remaining with random pairs
    while len(pairs_pool) < target:
        e1, e2 = random.sample(ENGINES, 2)
        pairs_pool.append((e1, e2))

    random.shuffle(pairs_pool)

    # Distribute coupling types evenly
    coupling_cycle = []
    while len(coupling_cycle) < target:
        shuffled = COUPLING_TYPES[:]
        random.shuffle(shuffled)
        coupling_cycle.extend(shuffled)

    for i in range(target):
        engine1, engine2 = pairs_pool[i % len(pairs_pool)]
        coupling_type = coupling_cycle[i]

        name = generate_name(used_names)
        used_names.add(name)

        filename = name.replace(" ", "_") + ".xometa"
        filepath = os.path.join(out_dir, filename)

        if os.path.exists(filepath):
            skipped += 1
            continue

        preset = generate_preset(name, engine1, engine2, coupling_type)

        with open(filepath, "w") as f:
            json.dump(preset, f, indent=2)

        written += 1

    print(f"Done. Written: {written} | Skipped (already exist): {skipped}")
    print(f"Output directory: {out_dir}")


if __name__ == "__main__":
    main()
