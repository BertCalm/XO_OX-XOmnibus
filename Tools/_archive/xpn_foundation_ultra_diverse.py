"""
xpn_foundation_ultra_diverse.py
Generates 80 Foundation mood presets with extreme DNA diversity.
16 corner combinations × 5 variants = 80 presets.
Output: Presets/XOmnibus/Foundation/
"""

import os
import json
import random

random.seed(None)

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
REPO_ROOT = os.path.dirname(SCRIPT_DIR)
OUTPUT_DIR = os.path.join(REPO_ROOT, "Presets", "XOmnibus", "Foundation")

ENGINES = [
    "ODDFELIX", "ODDOSCAR", "OBLONG", "OBESE", "OVERBITE",
    "OVERDUB", "ONSET", "OUROBOROS", "OBSIDIAN", "ORIGAMI",
    "ORACLE", "OVERWORLD"
]

COUPLING_TYPES = [
    "FREQUENCY_SHIFT", "AMPLITUDE_MOD", "FILTER_MOD", "PITCH_SYNC",
    "TIMBRE_BLEND", "ENVELOPE_LINK", "HARMONIC_FOLD", "CHAOS_INJECT",
    "RESONANCE_SHARE", "SPATIAL_COUPLE", "SPECTRAL_MORPH", "VELOCITY_COUPLE"
]

# 16 corner combos: each entry is (label, fixed_dims_dict)
# Fixed dims specify which DNA dimensions are extreme (True=XHIGH, False=XLOW)
# Dimensions: brightness, warmth, movement, density, space, aggression
CORNERS = [
    ("DARK_COLD_KINETIC_VIOLENT",    {"brightness": False, "warmth": False, "movement": True,  "aggression": True}),
    ("BRIGHT_WARM_DENSE_VAST",       {"brightness": True,  "warmth": True,  "density": True,   "space": True}),
    ("DARK_HOT_SPARSE_VAST",         {"brightness": False, "warmth": True,  "density": False,  "space": True}),
    ("BRIGHT_COLD_STILL_GENTLE",     {"brightness": True,  "warmth": False, "movement": False, "aggression": False}),
    ("DARK_DENSE_INTIMATE_VIOLENT",  {"brightness": False, "density": True, "space": False,    "aggression": True}),
    ("BRIGHT_COLD_SPARSE_VAST",      {"brightness": True,  "warmth": False, "density": False,  "space": True}),
    ("HOT_KINETIC_DENSE_VIOLENT",    {"warmth": True,      "movement": True,"density": True,   "aggression": True}),
    ("COLD_STILL_SPARSE_INTIMATE",   {"warmth": False,     "movement": False,"density": False, "space": False}),
    ("DARK_COLD_VAST_GENTLE",        {"brightness": False, "warmth": False, "space": True,     "aggression": False}),
    ("BRIGHT_KINETIC_INTIMATE_VIOLENT",{"brightness": True,"movement": True,"space": False,    "aggression": True}),
    ("HOT_DENSE_VAST_GENTLE",        {"warmth": True,      "density": True, "space": True,     "aggression": False}),
    ("DARK_KINETIC_SPARSE_GENTLE",   {"brightness": False, "movement": True,"density": False,  "aggression": False}),
    ("BRIGHT_WARM_STILL_SPARSE",     {"brightness": True,  "warmth": True,  "movement": False, "density": False}),
    ("DARK_HOT_KINETIC_DENSE",       {"brightness": False, "warmth": True,  "movement": True,  "density": True}),
    ("BRIGHT_DENSE_INTIMATE_GENTLE", {"brightness": True,  "density": True, "space": False,    "aggression": False}),
    ("COLD_KINETIC_VAST_VIOLENT",    {"warmth": False,     "movement": True,"space": True,     "aggression": True}),
]

DNA_DIMS = ["brightness", "warmth", "movement", "density", "space", "aggression"]


def xlow():
    return round(random.uniform(0.03, 0.14), 3)


def xhigh():
    return round(random.uniform(0.86, 0.98), 3)


def mid():
    return round(random.uniform(0.3, 0.7), 3)


def build_dna(fixed_dims):
    dna = {}
    for dim in DNA_DIMS:
        if dim in fixed_dims:
            dna[dim] = xhigh() if fixed_dims[dim] else xlow()
        else:
            dna[dim] = mid()
    return dna


def derive_tags(label, dna):
    tags = ["foundation", "extreme"]
    parts = label.lower().split("_")
    tags += [p for p in parts if p not in ("fnd",)]
    if dna["aggression"] >= 0.85:
        tags.append("aggressive")
    if dna["movement"] >= 0.85:
        tags.append("kinetic")
    if dna["space"] >= 0.85:
        tags.append("spatial")
    if dna["brightness"] <= 0.15:
        tags.append("dark")
    if dna["warmth"] <= 0.15:
        tags.append("cold")
    return list(dict.fromkeys(tags))  # deduplicate, preserve order


def pick_two_engines():
    engines = random.sample(ENGINES, 2)
    return engines[0], engines[1]


def engine_params():
    return {
        "macro_character": round(random.uniform(0.1, 0.9), 3),
        "macro_movement":  round(random.uniform(0.1, 0.9), 3),
        "macro_coupling":  round(random.uniform(0.1, 0.9), 3),
        "macro_space":     round(random.uniform(0.1, 0.9), 3),
    }


def build_preset(corner_label, fixed_dims, variant_num):
    eng1, eng2 = pick_two_engines()
    coupling_type = random.choice(COUPLING_TYPES)
    dna = build_dna(fixed_dims)
    tags = derive_tags(corner_label, dna)

    name = f"{corner_label}_FND_{variant_num}"

    preset = {
        "name": name,
        "version": "1.0",
        "mood": "Foundation",
        "engines": [eng1, eng2],
        "parameters": {
            eng1: engine_params(),
            eng2: engine_params(),
        },
        "coupling": {
            "type": coupling_type,
            "source": eng1,
            "target": eng2,
            "amount": round(random.uniform(0.2, 0.95), 3),
        },
        "dna": dna,
        "macros": {
            "CHARACTER": round(random.uniform(0.1, 0.9), 3),
            "MOVEMENT":  round(random.uniform(0.1, 0.9), 3),
            "COUPLING":  round(random.uniform(0.1, 0.9), 3),
            "SPACE":     round(random.uniform(0.1, 0.9), 3),
        },
        "tags": tags,
    }
    return preset


def main():
    os.makedirs(OUTPUT_DIR, exist_ok=True)

    count = 0
    for corner_label, fixed_dims in CORNERS:
        for variant in range(1, 6):
            preset = build_preset(corner_label, fixed_dims, variant)
            filename = f"{preset['name']}.xometa"
            filepath = os.path.join(OUTPUT_DIR, filename)
            with open(filepath, "w") as f:
                json.dump(preset, f, indent=2)
            count += 1
            print(f"  [{count:02d}/80] {filename}")

    print(f"\nDone. {count} presets written to {OUTPUT_DIR}")

    # Verify
    written = [f for f in os.listdir(OUTPUT_DIR) if f.endswith(".xometa") and "_FND_" in f]
    print(f"Verification: {len(written)} _FND_ .xometa files found in output dir.")


if __name__ == "__main__":
    main()
