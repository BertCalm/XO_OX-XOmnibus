#!/usr/bin/env python3
"""
xpn_aether_warmth_movement_fill.py
Generates 80 Aether presets to fill secondary DNA gaps:
  - 40 warmth-XHIGH presets (AET3 suffix)
  - 40 movement-XHIGH presets (AET4 suffix)
"""

import json
import os
import random

random.seed(42)

OUTPUT_DIR = os.path.join(
    os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
    "Presets", "XOceanus", "Aether"
)

AETHER_ENGINES = [
    "OPAL", "ORACLE", "OBSCURA", "OCEANIC", "ORBITAL", "ORGANON",
    "OBLIQUE", "ORIGAMI", "ODDFELIX", "ODDOSCAR", "OSPREY", "OMBRE", "OVERWORLD"
]

COUPLING_TYPES = [
    "FREQUENCY_SHIFT", "AMPLITUDE_MOD", "FILTER_MOD", "PITCH_SYNC",
    "TIMBRE_BLEND", "ENVELOPE_LINK", "HARMONIC_FOLD", "CHAOS_INJECT",
    "RESONANCE_SHARE", "SPATIAL_COUPLE", "SPECTRAL_MORPH", "VELOCITY_COUPLE"
]

DNA_DIMS = ["brightness", "warmth", "movement", "density", "space", "aggression"]


def xlow():
    return round(random.uniform(0.02, 0.13), 4)


def xhigh():
    return round(random.uniform(0.87, 0.99), 4)


def mid():
    return round(random.uniform(0.3, 0.7), 4)


def macro_val():
    return round(random.uniform(0.2, 0.95), 4)


def pick_engines():
    engines = random.sample(AETHER_ENGINES, 2)
    return engines[0], engines[1]


def engine_params():
    return {
        "macro_character": macro_val(),
        "macro_movement": macro_val(),
        "macro_coupling": macro_val(),
        "macro_space": macro_val(),
    }


def make_preset(name, dna, tags):
    eng1, eng2 = pick_engines()
    coupling_type = random.choice(COUPLING_TYPES)
    coupling_amount = round(random.uniform(0.5, 0.95), 4)

    # Global macros derived loosely from dna
    return {
        "name": name,
        "version": "1.0",
        "mood": "Aether",
        "engines": [eng1, eng2],
        "parameters": {
            eng1: engine_params(),
            eng2: engine_params(),
        },
        "coupling": {
            "type": coupling_type,
            "source": eng1,
            "target": eng2,
            "amount": coupling_amount,
        },
        "dna": dna,
        "macros": {
            "CHARACTER": round((dna["brightness"] + dna["warmth"]) / 2, 4),
            "MOVEMENT": dna["movement"],
            "COUPLING": coupling_amount,
            "SPACE": dna["space"],
        },
        "tags": tags,
    }


def build_dna(fixed: dict) -> dict:
    """Build a full DNA dict; fixed values override; remaining dims get mid()."""
    dna = {dim: mid() for dim in DNA_DIMS}
    dna.update(fixed)
    return dna


# ---------------------------------------------------------------------------
# Warmth-XHIGH corners (AET3)
# name pattern: CORNER_LABEL_AET3_N
# ---------------------------------------------------------------------------

WARMTH_CORNERS = [
    # (label, fixed_dims, tags)
    (
        "BRIGHT_HOT_VAST_GENTLE",
        {"brightness": None, "warmth": None, "space": None, "aggression": "xlow"},
        ["aether", "warm", "bright", "vast", "gentle", "transcendent"],
    ),
    (
        "DARK_HOT_VAST_SPARSE",
        {"brightness": "xlow", "warmth": None, "space": None, "density": "xlow"},
        ["aether", "warm", "dark", "vast", "sparse"],
    ),
    (
        "BRIGHT_HOT_DENSE_VAST",
        {"brightness": None, "warmth": None, "density": None, "space": None},
        ["aether", "warm", "bright", "dense", "vast"],
    ),
    (
        "DARK_HOT_STILL_GENTLE",
        {"brightness": "xlow", "warmth": None, "movement": "xlow", "aggression": "xlow"},
        ["aether", "warm", "dark", "still", "gentle"],
    ),
    (
        "HOT_DENSE_INTIMATE_GENTLE",
        {"warmth": None, "density": None, "space": "xlow", "aggression": "xlow"},
        ["aether", "warm", "dense", "intimate", "gentle"],
    ),
    (
        "BRIGHT_HOT_KINETIC_VAST",
        {"brightness": None, "warmth": None, "movement": None, "space": None},
        ["aether", "warm", "bright", "kinetic", "vast"],
    ),
    (
        "HOT_VAST_VIOLENT_DENSE",
        {"warmth": None, "space": None, "aggression": None, "density": None},
        ["aether", "warm", "vast", "violent", "dense"],
    ),
    (
        "DARK_HOT_DENSE_VIOLENT",
        {"brightness": "xlow", "warmth": None, "density": None, "aggression": None},
        ["aether", "warm", "dark", "dense", "violent"],
    ),
]


def resolve_dims(corner_fixed: dict) -> dict:
    """Resolve None → xhigh(), 'xlow' → xlow(), 'xhigh' → xhigh() for fixed dims."""
    result = {}
    for dim, val in corner_fixed.items():
        if val is None:
            result[dim] = xhigh()
        elif val == "xlow":
            result[dim] = xlow()
        elif val == "xhigh":
            result[dim] = xhigh()
        else:
            result[dim] = val
    return result


warmth_presets = []
for label, corner_fixed, tags in WARMTH_CORNERS:
    for variant in range(1, 6):
        resolved = resolve_dims(corner_fixed)
        dna = build_dna(resolved)
        name = f"{label}_AET3_{variant}"
        warmth_presets.append(make_preset(name, dna, tags))

# ---------------------------------------------------------------------------
# Movement-XHIGH corners (AET4)
# ---------------------------------------------------------------------------

MOVEMENT_CORNERS = [
    (
        "BRIGHT_KINETIC_VAST_GENTLE",
        {"brightness": None, "movement": None, "space": None, "aggression": "xlow"},
        ["aether", "kinetic", "bright", "vast", "gentle"],
    ),
    (
        "DARK_KINETIC_VAST_DENSE",
        {"brightness": "xlow", "movement": None, "space": None, "density": None},
        ["aether", "kinetic", "dark", "vast", "dense"],
    ),
    (
        "BRIGHT_HOT_KINETIC_SPARSE",
        {"brightness": None, "warmth": None, "movement": None, "density": "xlow"},
        ["aether", "kinetic", "bright", "warm", "sparse"],
    ),
    (
        "DARK_COLD_KINETIC_VAST",
        {"brightness": "xlow", "warmth": "xlow", "movement": None, "space": None},
        ["aether", "kinetic", "dark", "cold", "vast"],
    ),
    (
        "KINETIC_DENSE_VAST_VIOLENT",
        {"movement": None, "density": None, "space": None, "aggression": None},
        ["aether", "kinetic", "dense", "vast", "violent"],
    ),
    (
        "BRIGHT_KINETIC_INTIMATE_VIOLENT",
        {"brightness": None, "movement": None, "space": "xlow", "aggression": None},
        ["aether", "kinetic", "bright", "intimate", "violent"],
    ),
    (
        "HOT_KINETIC_SPARSE_GENTLE",
        {"warmth": None, "movement": None, "density": "xlow", "aggression": "xlow"},
        ["aether", "kinetic", "warm", "sparse", "gentle"],
    ),
    (
        "DARK_KINETIC_DENSE_VIOLENT",
        {"brightness": "xlow", "movement": None, "density": None, "aggression": None},
        ["aether", "kinetic", "dark", "dense", "violent"],
    ),
]

movement_presets = []
for label, corner_fixed, tags in MOVEMENT_CORNERS:
    for variant in range(1, 6):
        resolved = resolve_dims(corner_fixed)
        dna = build_dna(resolved)
        name = f"{label}_AET4_{variant}"
        movement_presets.append(make_preset(name, dna, tags))

# ---------------------------------------------------------------------------
# Write files
# ---------------------------------------------------------------------------

os.makedirs(OUTPUT_DIR, exist_ok=True)

all_presets = warmth_presets + movement_presets
written = 0
for preset in all_presets:
    filename = preset["name"].replace(" ", "_") + ".xometa"
    filepath = os.path.join(OUTPUT_DIR, filename)
    with open(filepath, "w") as f:
        json.dump(preset, f, indent=2)
    written += 1

print(f"Written {written} presets to {OUTPUT_DIR}")
print(f"  warmth-XHIGH (AET3): {len(warmth_presets)}")
print(f"  movement-XHIGH (AET4): {len(movement_presets)}")

# Spot-check DNA constraints
errors = []
for p in warmth_presets:
    if p["dna"]["warmth"] < 0.87:
        errors.append(f"{p['name']}: warmth={p['dna']['warmth']} below XHIGH threshold")
for p in movement_presets:
    if p["dna"]["movement"] < 0.87:
        errors.append(f"{p['name']}: movement={p['dna']['movement']} below XHIGH threshold")

if errors:
    print("\nDNA CONSTRAINT ERRORS:")
    for e in errors:
        print(f"  {e}")
else:
    print("DNA constraints: all pass")
