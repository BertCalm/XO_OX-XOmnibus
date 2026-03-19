#!/usr/bin/env python3
"""
xpn_prism_dark_brightness_fill.py
Generates 60 Dark Prism presets (brightness-XLOW ≤ 0.13) to balance the Prism mood DNA.
12 corners × 5 variants = 60 presets with PRS_DRK suffix.
"""

import json
import os
import random

random.seed(42)

PRISM_ENGINES = [
    "OBLIQUE", "ORIGAMI", "ORACLE", "OPTIC", "OCELOT", "OSTERIA",
    "OWLFISH", "OHM", "OLE", "ORPHICA", "OTTONI", "OVERWORLD"
]

VALID_COUPLING_TYPES = [
    "FREQUENCY_SHIFT", "AMPLITUDE_MOD", "FILTER_MOD", "PITCH_SYNC",
    "TIMBRE_BLEND", "ENVELOPE_LINK", "HARMONIC_FOLD", "CHAOS_INJECT",
    "RESONANCE_SHARE", "SPATIAL_COUPLE", "SPECTRAL_MORPH", "VELOCITY_COUPLE"
]

OUTPUT_DIR = os.path.join(
    os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
    "Presets", "XOmnibus", "Prism"
)

def xlow_brightness():
    return round(random.uniform(0.02, 0.11), 4)

def xlow():
    return round(random.uniform(0.02, 0.13), 4)

def xhigh():
    return round(random.uniform(0.87, 0.99), 4)

def mid():
    return round(random.uniform(0.3, 0.7), 4)

# 12 corner definitions: (label, dna_fn)
# Each returns a dict of dna values; brightness always XLOW
CORNERS = [
    {
        "label": "DARK_HOT_KINETIC_DENSE",
        "suffix": "PRS_DRK",
        "tags": ["prism", "dark", "hot", "kinetic", "dense"],
        "dna": lambda: {
            "brightness": xlow_brightness(),
            "warmth": xhigh(),
            "movement": xhigh(),
            "density": xhigh(),
            "space": mid(),
            "aggression": mid(),
        },
    },
    {
        "label": "DARK_COLD_VAST_VIOLENT",
        "suffix": "PRS_DRK",
        "tags": ["prism", "dark", "cold", "vast", "violent"],
        "dna": lambda: {
            "brightness": xlow_brightness(),
            "warmth": xlow(),
            "movement": mid(),
            "density": mid(),
            "space": xhigh(),
            "aggression": xhigh(),
        },
    },
    {
        "label": "DARK_HOT_VAST_GENTLE",
        "suffix": "PRS_DRK",
        "tags": ["prism", "dark", "hot", "vast", "gentle"],
        "dna": lambda: {
            "brightness": xlow_brightness(),
            "warmth": xhigh(),
            "movement": mid(),
            "density": mid(),
            "space": xhigh(),
            "aggression": xlow(),
        },
    },
    {
        "label": "DARK_COLD_DENSE_GENTLE",
        "suffix": "PRS_DRK",
        "tags": ["prism", "dark", "cold", "dense", "gentle"],
        "dna": lambda: {
            "brightness": xlow_brightness(),
            "warmth": xlow(),
            "movement": mid(),
            "density": xhigh(),
            "space": mid(),
            "aggression": xlow(),
        },
    },
    {
        "label": "DARK_KINETIC_DENSE_VIOLENT",
        "suffix": "PRS_DRK",
        "tags": ["prism", "dark", "kinetic", "dense", "violent"],
        "dna": lambda: {
            "brightness": xlow_brightness(),
            "warmth": mid(),
            "movement": xhigh(),
            "density": xhigh(),
            "space": mid(),
            "aggression": xhigh(),
        },
    },
    {
        "label": "DARK_HOT_STILL_INTIMATE",
        "suffix": "PRS_DRK",
        "tags": ["prism", "dark", "hot", "still", "intimate"],
        "dna": lambda: {
            "brightness": xlow_brightness(),
            "warmth": xhigh(),
            "movement": xlow(),
            "density": mid(),
            "space": xlow(),
            "aggression": mid(),
        },
    },
    {
        "label": "DARK_COLD_KINETIC_VAST",
        "suffix": "PRS_DRK",
        "tags": ["prism", "dark", "cold", "kinetic", "vast"],
        "dna": lambda: {
            "brightness": xlow_brightness(),
            "warmth": xlow(),
            "movement": xhigh(),
            "density": mid(),
            "space": xhigh(),
            "aggression": mid(),
        },
    },
    {
        "label": "DARK_SPARSE_VAST_VIOLENT",
        "suffix": "PRS_DRK",
        "tags": ["prism", "dark", "sparse", "vast", "violent"],
        "dna": lambda: {
            "brightness": xlow_brightness(),
            "warmth": mid(),
            "movement": mid(),
            "density": xlow(),
            "space": xhigh(),
            "aggression": xhigh(),
        },
    },
    {
        "label": "DARK_HOT_DENSE_VAST",
        "suffix": "PRS_DRK",
        "tags": ["prism", "dark", "hot", "dense", "vast"],
        "dna": lambda: {
            "brightness": xlow_brightness(),
            "warmth": xhigh(),
            "movement": mid(),
            "density": xhigh(),
            "space": xhigh(),
            "aggression": mid(),
        },
    },
    {
        "label": "DARK_COLD_STILL_DENSE",
        "suffix": "PRS_DRK",
        "tags": ["prism", "dark", "cold", "still", "dense"],
        "dna": lambda: {
            "brightness": xlow_brightness(),
            "warmth": xlow(),
            "movement": xlow(),
            "density": xhigh(),
            "space": mid(),
            "aggression": mid(),
        },
    },
    {
        "label": "DARK_KINETIC_VAST_GENTLE",
        "suffix": "PRS_DRK",
        "tags": ["prism", "dark", "kinetic", "vast", "gentle"],
        "dna": lambda: {
            "brightness": xlow_brightness(),
            "warmth": mid(),
            "movement": xhigh(),
            "density": mid(),
            "space": xhigh(),
            "aggression": xlow(),
        },
    },
    {
        "label": "DARK_HOT_KINETIC_VIOLENT",
        "suffix": "PRS_DRK",
        "tags": ["prism", "dark", "hot", "kinetic", "violent"],
        "dna": lambda: {
            "brightness": xlow_brightness(),
            "warmth": xhigh(),
            "movement": xhigh(),
            "density": mid(),
            "space": mid(),
            "aggression": xhigh(),
        },
    },
]

VARIANTS_PER_CORNER = 5


def pick_engines():
    engines = random.sample(PRISM_ENGINES, 2)
    return engines[0], engines[1]


def dna_to_macros(dna):
    """Map DNA axes to the 4 macro slots."""
    return {
        "CHARACTER": round((dna["warmth"] + dna["aggression"]) / 2, 4),
        "MOVEMENT": dna["movement"],
        "COUPLING": dna["density"],
        "SPACE": dna["space"],
    }


def build_engine_params(dna):
    """Derive per-engine macro params from DNA."""
    character = round((dna["warmth"] + dna["aggression"]) / 2, 4)
    return {
        "macro_character": character,
        "macro_movement": dna["movement"],
        "macro_coupling": dna["density"],
        "macro_space": dna["space"],
    }


def generate_preset(corner, variant_num):
    dna = corner["dna"]()
    engine1, engine2 = pick_engines()
    coupling_type = random.choice(VALID_COUPLING_TYPES)
    coupling_amount = round(random.uniform(0.55, 0.95), 4)

    label = corner["label"]
    suffix = corner["suffix"]
    name = f"{label}_{suffix}_{variant_num}"

    params1 = build_engine_params(dna)
    # Slight variation for engine 2
    params2 = {k: round(min(0.99, max(0.01, v + random.uniform(-0.04, 0.04))), 4)
               for k, v in params1.items()}

    preset = {
        "name": name,
        "version": "1.0",
        "mood": "Prism",
        "engines": [engine1, engine2],
        "parameters": {
            engine1: params1,
            engine2: params2,
        },
        "coupling": {
            "type": coupling_type,
            "source": engine1,
            "target": engine2,
            "amount": coupling_amount,
        },
        "dna": dna,
        "macros": dna_to_macros(dna),
        "tags": corner["tags"][:],
    }
    return name, preset


def main():
    os.makedirs(OUTPUT_DIR, exist_ok=True)

    generated = []
    for corner in CORNERS:
        for v in range(1, VARIANTS_PER_CORNER + 1):
            name, preset = generate_preset(corner, v)
            filename = f"{name}.xometa"
            filepath = os.path.join(OUTPUT_DIR, filename)
            with open(filepath, "w") as f:
                json.dump(preset, f, indent=2)
            generated.append((name, preset["dna"]["brightness"]))

    # Verification
    print(f"Generated {len(generated)} presets to {OUTPUT_DIR}")
    violations = [(n, b) for n, b in generated if b > 0.13]
    if violations:
        print(f"VIOLATIONS (brightness > 0.13):")
        for n, b in violations:
            print(f"  {n}: {b}")
    else:
        print("All 60 presets have brightness <= 0.13 — PASS")

    brightnesses = [b for _, b in generated]
    print(f"Brightness range: {min(brightnesses):.4f} – {max(brightnesses):.4f}")

    prs_drk = [n for n, _ in generated if "PRS_DRK" in n]
    print(f"PRS_DRK files: {len(prs_drk)}")


if __name__ == "__main__":
    main()
