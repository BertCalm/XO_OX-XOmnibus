#!/usr/bin/env python3
"""
xpn_entangled_density_low_pack.py
Generates 80 .xometa presets targeting Entangled mood with density ≤0.14 (XLOW zone).
Output: Presets/Entangled/
"""

import json
import os
import random

OUTPUT_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "Presets", "Entangled")

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

# Sparse/minimal/hollow naming pools
ADJECTIVES = [
    "VOID", "SKELETAL", "SPARSE", "HOLLOW", "BARE", "THIN", "GHOSTED",
    "FAINT", "MINIMAL", "VACANT", "SILENT", "EMPTY", "NAKED", "RAW",
    "OPEN", "DESERT", "BLEACHED", "TRACE", "SPECTRAL", "LEAN",
    "STRIPPED", "NEGATIVE", "ABSENT", "ZERO", "NULL", "PALE",
    "DIFFUSE", "DISTANT", "FILAMENT", "THREAD", "DRIFT", "ETHER",
    "HUSK", "RESIDUE", "ECHO", "SHADOW", "MIST", "STATIC", "BREATH",
    "PULSE"
]

NOUNS = [
    "WHISPER", "TRACE", "FIELD", "CORE", "SIGNAL", "THREAD", "GHOST",
    "WIRE", "MEMBRANE", "FRAME", "LATTICE", "GRID", "VEIL", "CURRENT",
    "TOPOLOGY", "EDGE", "NODE", "VOID", "CHAMBER", "SPACE",
    "PARTICLE", "DRIFT", "WAVE", "RIBBON", "SEAM", "BRIDGE", "PATH",
    "LOOP", "STRAND", "FIBER", "PULSE", "TONE", "HAZE", "BLOOM",
    "SILENCE", "ECHO", "SHADOW", "LAYER", "STEM", "POINT"
]

SPARSE_TAGS = [
    ["entangled", "sparse", "minimal"],
    ["entangled", "hollow", "void"],
    ["entangled", "bare", "skeletal"],
    ["entangled", "thin", "ghosted"],
    ["entangled", "minimal", "negative-space"],
    ["entangled", "sparse", "spectral"],
    ["entangled", "drift", "filament"],
    ["entangled", "trace", "residue"],
]

rng = random.Random(42)  # deterministic seed for reproducibility


def rand(lo=0.0, hi=1.0):
    return round(rng.uniform(lo, hi), 3)


def engine_pair():
    a, b = rng.sample(ENGINES, 2)
    return a, b


def make_preset(index, used_names):
    engine1, engine2 = engine_pair()
    coupling_type = COUPLING_TYPES[index % len(COUPLING_TYPES)]

    # Generate a unique name
    attempts = 0
    while True:
        adj = rng.choice(ADJECTIVES)
        noun = rng.choice(NOUNS)
        name = f"{adj} {noun}"
        if name not in used_names or attempts > 50:
            # Add index suffix if still colliding
            if name in used_names:
                name = f"{name} {index + 1}"
            break
        attempts += 1
    used_names.add(name)

    density = rand(0.03, 0.14)

    preset = {
        "name": name,
        "version": "1.0",
        "mood": "Entangled",
        "engines": [engine1, engine2],
        "parameters": {
            engine1: {
                "macro_character": rand(),
                "macro_movement": rand(),
                "macro_coupling": rand(),
                "macro_space": rand()
            },
            engine2: {
                "macro_character": rand(),
                "macro_movement": rand(),
                "macro_coupling": rand(),
                "macro_space": rand()
            }
        },
        "coupling": {
            "type": coupling_type,
            "source": engine1,
            "target": engine2,
            "amount": rand(0.1, 0.9)
        },
        "dna": {
            "brightness": rand(),
            "warmth": rand(),
            "movement": rand(),
            "density": density,
            "space": rand(),
            "aggression": rand()
        },
        "macros": {
            "CHARACTER": rand(),
            "MOVEMENT": rand(),
            "COUPLING": rand(),
            "SPACE": rand()
        },
        "tags": list(rng.choice(SPARSE_TAGS))
    }

    return preset


def filename_for(preset):
    safe = preset["name"].replace(" ", "_").upper()
    return f"{safe}.xometa"


def main():
    os.makedirs(OUTPUT_DIR, exist_ok=True)

    used_names = set()
    written = 0
    skipped = 0

    for i in range(80):
        preset = make_preset(i, used_names)
        fname = filename_for(preset)
        fpath = os.path.join(OUTPUT_DIR, fname)

        if os.path.exists(fpath):
            skipped += 1
            continue

        with open(fpath, "w") as f:
            json.dump(preset, f, indent=2)
        written += 1

    print(f"Done. Written: {written}  Skipped (already exist): {skipped}")
    print(f"Output directory: {OUTPUT_DIR}")


if __name__ == "__main__":
    main()
