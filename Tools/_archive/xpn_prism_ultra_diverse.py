#!/usr/bin/env python3
"""
xpn_prism_ultra_diverse.py
Generate 80 Prism mood presets with extreme DNA diversity.
16 corner combinations × 5 variants = 80 presets.
"""

import json
import os
import random

random.seed(42)

OUTPUT_DIR = os.path.join(
    os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
    "Presets", "XOceanus", "Prism"
)
os.makedirs(OUTPUT_DIR, exist_ok=True)

ENGINES = [
    "OBLIQUE", "ORIGAMI", "ORACLE", "OPTIC", "OCELOT",
    "OSTERIA", "OWLFISH", "OVERWORLD", "OHM", "OLE",
    "ORPHICA", "OTTONI"
]

COUPLING_TYPES = [
    "FREQUENCY_SHIFT", "AMPLITUDE_MOD", "FILTER_MOD", "PITCH_SYNC",
    "TIMBRE_BLEND", "ENVELOPE_LINK", "HARMONIC_FOLD", "CHAOS_INJECT",
    "RESONANCE_SHARE", "SPATIAL_COUPLE", "SPECTRAL_MORPH", "VELOCITY_COUPLE"
]

DNA_DIMS = ["brightness", "warmth", "movement", "density", "space", "aggression"]

# 16 corner combo definitions: list of (dim, zone) where zone is 'XLOW' or 'XHIGH'
CORNERS = [
    {
        "label": "DARK_HOT_KINETIC_DENSE",
        "tags": ["dark", "hot", "kinetic", "dense"],
        "extremes": {"brightness": "XLOW", "warmth": "XHIGH", "movement": "XHIGH", "density": "XHIGH"},
    },
    {
        "label": "BRIGHT_COLD_STILL_VIOLENT",
        "tags": ["bright", "cold", "still", "violent"],
        "extremes": {"brightness": "XHIGH", "warmth": "XLOW", "movement": "XLOW", "aggression": "XHIGH"},
    },
    {
        "label": "BRIGHT_WARM_VAST_GENTLE",
        "tags": ["bright", "warm", "vast", "gentle"],
        "extremes": {"brightness": "XHIGH", "warmth": "XHIGH", "space": "XHIGH", "aggression": "XLOW"},
    },
    {
        "label": "DARK_COLD_DENSE_VIOLENT",
        "tags": ["dark", "cold", "dense", "violent"],
        "extremes": {"brightness": "XLOW", "warmth": "XLOW", "density": "XHIGH", "aggression": "XHIGH"},
    },
    {
        "label": "BRIGHT_KINETIC_DENSE_INTIMATE",
        "tags": ["bright", "kinetic", "dense", "intimate"],
        "extremes": {"brightness": "XHIGH", "movement": "XHIGH", "density": "XHIGH", "space": "XLOW"},
    },
    {
        "label": "DARK_STILL_SPARSE_VAST",
        "tags": ["dark", "still", "sparse", "vast"],
        "extremes": {"brightness": "XLOW", "movement": "XLOW", "density": "XLOW", "space": "XHIGH"},
    },
    {
        "label": "HOT_STILL_DENSE_VIOLENT",
        "tags": ["hot", "still", "dense", "violent"],
        "extremes": {"warmth": "XHIGH", "movement": "XLOW", "density": "XHIGH", "aggression": "XHIGH"},
    },
    {
        "label": "COLD_KINETIC_VAST_GENTLE",
        "tags": ["cold", "kinetic", "vast", "gentle"],
        "extremes": {"warmth": "XLOW", "movement": "XHIGH", "space": "XHIGH", "aggression": "XLOW"},
    },
    {
        "label": "BRIGHT_COLD_SPARSE_VAST",
        "tags": ["bright", "cold", "sparse", "vast"],
        "extremes": {"brightness": "XHIGH", "warmth": "XLOW", "density": "XLOW", "space": "XHIGH"},
    },
    {
        "label": "DARK_HOT_STILL_GENTLE",
        "tags": ["dark", "hot", "still", "gentle"],
        "extremes": {"brightness": "XLOW", "warmth": "XHIGH", "movement": "XLOW", "aggression": "XLOW"},
    },
    {
        "label": "BRIGHT_DENSE_INTIMATE_VIOLENT",
        "tags": ["bright", "dense", "intimate", "violent"],
        "extremes": {"brightness": "XHIGH", "density": "XHIGH", "space": "XLOW", "aggression": "XHIGH"},
    },
    {
        "label": "HOT_KINETIC_VAST_SPARSE",
        "tags": ["hot", "kinetic", "vast", "sparse"],
        "extremes": {"warmth": "XHIGH", "movement": "XHIGH", "space": "XHIGH", "density": "XLOW"},
    },
    {
        "label": "DARK_SPARSE_INTIMATE_VIOLENT",
        "tags": ["dark", "sparse", "intimate", "violent"],
        "extremes": {"brightness": "XLOW", "density": "XLOW", "space": "XLOW", "aggression": "XHIGH"},
    },
    {
        "label": "BRIGHT_WARM_KINETIC_VIOLENT",
        "tags": ["bright", "warm", "kinetic", "violent"],
        "extremes": {"brightness": "XHIGH", "warmth": "XHIGH", "movement": "XHIGH", "aggression": "XHIGH"},
    },
    {
        "label": "DARK_COLD_KINETIC_VAST",
        "tags": ["dark", "cold", "kinetic", "vast"],
        "extremes": {"brightness": "XLOW", "warmth": "XLOW", "movement": "XHIGH", "space": "XHIGH"},
    },
    {
        "label": "BRIGHT_WARM_DENSE_GENTLE",
        "tags": ["bright", "warm", "dense", "gentle"],
        "extremes": {"brightness": "XHIGH", "warmth": "XHIGH", "density": "XHIGH", "aggression": "XLOW"},
    },
]


def xlow():
    return round(random.uniform(0.03, 0.14), 3)


def xhigh():
    return round(random.uniform(0.86, 0.98), 3)


def mid():
    return round(random.uniform(0.3, 0.7), 3)


def engine_params(brightness, movement, space, aggression):
    """Derive macro params loosely from DNA dims."""
    character = round((brightness + aggression) / 2, 3)
    mov = round(movement, 3)
    coupling = round(random.uniform(0.3, 0.9), 3)
    spc = round(space, 3)
    return {
        "macro_character": character,
        "macro_movement": mov,
        "macro_coupling": coupling,
        "macro_space": spc,
    }


def make_preset(corner, variant_idx):
    extremes = corner["extremes"]
    label = corner["label"]

    # Build DNA
    dna = {}
    for dim in DNA_DIMS:
        if dim in extremes:
            dna[dim] = xhigh() if extremes[dim] == "XHIGH" else xlow()
        else:
            dna[dim] = mid()

    # Pick 2 distinct engines (vary by variant)
    engine_pool = ENGINES[:]
    random.shuffle(engine_pool)
    eng1 = engine_pool[variant_idx % len(engine_pool)]
    eng2 = engine_pool[(variant_idx + 3) % len(engine_pool)]
    if eng1 == eng2:
        eng2 = engine_pool[(variant_idx + 5) % len(engine_pool)]

    # Build parameters per engine
    params = {
        eng1: engine_params(dna["brightness"], dna["movement"], dna["space"], dna["aggression"]),
        eng2: engine_params(
            1.0 - dna["brightness"],
            dna["movement"],
            dna["space"],
            dna.get("aggression", 0.5),
        ),
    }

    coupling_type = COUPLING_TYPES[variant_idx % len(COUPLING_TYPES)]
    coupling_amount = round(random.uniform(0.4, 0.95), 3)

    tags = ["prism"] + corner["tags"]

    name = f"{label}_PRS_{variant_idx + 1}"

    macros = {
        "CHARACTER": params[eng1]["macro_character"],
        "MOVEMENT": params[eng1]["macro_movement"],
        "COUPLING": params[eng1]["macro_coupling"],
        "SPACE": params[eng1]["macro_space"],
    }

    preset = {
        "name": name,
        "version": "1.0",
        "mood": "Prism",
        "engines": [eng1, eng2],
        "parameters": params,
        "coupling": {
            "type": coupling_type,
            "source": eng1,
            "target": eng2,
            "amount": coupling_amount,
        },
        "dna": dna,
        "macros": macros,
        "tags": tags,
    }
    return name, preset


def main():
    count = 0
    for corner in CORNERS:
        for v in range(5):
            name, preset = make_preset(corner, v)
            filename = name + ".xometa"
            filepath = os.path.join(OUTPUT_DIR, filename)
            with open(filepath, "w") as f:
                json.dump(preset, f, indent=2)
            count += 1

    print(f"Generated {count} presets in {OUTPUT_DIR}")
    files = [f for f in os.listdir(OUTPUT_DIR) if f.endswith(".xometa") and "_PRS_" in f]
    print(f"PRS files in directory: {len(files)}")


if __name__ == "__main__":
    main()
