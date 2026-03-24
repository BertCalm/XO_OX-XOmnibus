#!/usr/bin/env python3
"""
xpn_foundation_prism_mega_diverse.py
Generates 100 max-diversity presets: 50 Foundation + 50 Prism
20 corner DNA combinations × 5 variants each
"""

import json
import os
import random
from datetime import date

random.seed(42)

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FOUNDATION_DIR = os.path.join(REPO_ROOT, "Presets", "XOlokun", "Foundation")
PRISM_DIR = os.path.join(REPO_ROOT, "Presets", "XOlokun", "Prism")

# DNA dimension value ranges
XLOW_RANGE = (0.02, 0.13)
XHIGH_RANGE = (0.87, 0.99)
MID_RANGE = (0.3, 0.7)

DIMS = ["brightness", "warmth", "movement", "density", "space", "aggression"]

VALID_COUPLING_TYPES = [
    "FREQUENCY_SHIFT", "AMPLITUDE_MOD", "FILTER_MOD", "PITCH_SYNC",
    "TIMBRE_BLEND", "ENVELOPE_LINK", "HARMONIC_FOLD", "CHAOS_INJECT",
    "RESONANCE_SHARE", "SPATIAL_COUPLE", "SPECTRAL_MORPH", "VELOCITY_COUPLE"
]

FOUNDATION_ENGINES = [
    "ODDFELIX", "ODDOSCAR", "OBLONG", "OBESE", "OVERBITE",
    "OVERDUB", "ONSET", "OUROBOROS", "OBSIDIAN", "ORIGAMI"
]

PRISM_ENGINES = [
    "OBLIQUE", "ORIGAMI", "ORACLE", "OPTIC", "OCELOT",
    "OSTERIA", "OWLFISH", "OHM", "OLE", "ORPHICA", "OTTONI", "OVERWORLD"
]

def rval(rng):
    return round(random.uniform(*rng), 3)

def dna_from_spec(spec):
    """spec: dict mapping dim -> 'XLOW'|'XHIGH'|'MID' """
    dna = {}
    for dim in DIMS:
        zone = spec.get(dim, "MID")
        if zone == "XLOW":
            dna[dim] = rval(XLOW_RANGE)
        elif zone == "XHIGH":
            dna[dim] = rval(XHIGH_RANGE)
        else:
            dna[dim] = rval(MID_RANGE)
    return dna

# Corner definitions: each is a dict of dim -> zone (only extremes specified, rest MID)
# Foundation corners 1-10
FOUNDATION_CORNERS = [
    # 1: dark cold kinetic dense violent
    {
        "label": "DARK_COLD_KINETIC_DENSE_VIOLENT",
        "tags": ["dark", "cold", "kinetic", "dense", "violent"],
        "spec": {"brightness": "XLOW", "warmth": "XLOW", "movement": "XHIGH", "density": "XHIGH", "aggression": "XHIGH"},
    },
    # 2: bright warm kinetic vast gentle
    {
        "label": "BRIGHT_WARM_KINETIC_VAST_GENTLE",
        "tags": ["bright", "warm", "kinetic", "vast", "gentle"],
        "spec": {"brightness": "XHIGH", "warmth": "XHIGH", "movement": "XHIGH", "space": "XHIGH", "aggression": "XLOW"},
    },
    # 3: dark hot dense vast gentle
    {
        "label": "DARK_HOT_DENSE_VAST_GENTLE",
        "tags": ["dark", "hot", "dense", "vast", "gentle"],
        "spec": {"brightness": "XLOW", "warmth": "XHIGH", "density": "XHIGH", "space": "XHIGH", "aggression": "XLOW"},
    },
    # 4: bright cold still sparse intimate
    {
        "label": "BRIGHT_COLD_STILL_SPARSE_INTIMATE",
        "tags": ["bright", "cold", "still", "sparse", "intimate"],
        "spec": {"brightness": "XHIGH", "warmth": "XLOW", "movement": "XLOW", "density": "XLOW", "space": "XLOW"},
    },
    # 5: dark cold sparse vast gentle
    {
        "label": "DARK_COLD_SPARSE_VAST_GENTLE",
        "tags": ["dark", "cold", "sparse", "vast", "gentle"],
        "spec": {"brightness": "XLOW", "warmth": "XLOW", "density": "XLOW", "space": "XHIGH", "aggression": "XLOW"},
    },
    # 6: bright kinetic dense intimate violent
    {
        "label": "BRIGHT_KINETIC_DENSE_INTIMATE_VIOLENT",
        "tags": ["bright", "kinetic", "dense", "intimate", "violent"],
        "spec": {"brightness": "XHIGH", "movement": "XHIGH", "density": "XHIGH", "space": "XLOW", "aggression": "XHIGH"},
    },
    # 7: hot still dense vast violent
    {
        "label": "HOT_STILL_DENSE_VAST_VIOLENT",
        "tags": ["hot", "still", "dense", "vast", "violent"],
        "spec": {"warmth": "XHIGH", "movement": "XLOW", "density": "XHIGH", "space": "XHIGH", "aggression": "XHIGH"},
    },
    # 8: cold kinetic sparse vast violent
    {
        "label": "COLD_KINETIC_SPARSE_VAST_VIOLENT",
        "tags": ["cold", "kinetic", "sparse", "vast", "violent"],
        "spec": {"warmth": "XLOW", "movement": "XHIGH", "density": "XLOW", "space": "XHIGH", "aggression": "XHIGH"},
    },
    # 9: bright warm sparse vast gentle
    {
        "label": "BRIGHT_WARM_SPARSE_VAST_GENTLE",
        "tags": ["bright", "warm", "sparse", "vast", "gentle"],
        "spec": {"brightness": "XHIGH", "warmth": "XHIGH", "density": "XLOW", "space": "XHIGH", "aggression": "XLOW"},
    },
    # 10: dark cold kinetic intimate violent
    {
        "label": "DARK_COLD_KINETIC_INTIMATE_VIOLENT",
        "tags": ["dark", "cold", "kinetic", "intimate", "violent"],
        "spec": {"brightness": "XLOW", "warmth": "XLOW", "movement": "XHIGH", "space": "XLOW", "aggression": "XHIGH"},
    },
]

# Prism corners 11-20
PRISM_CORNERS = [
    # 11: bright cold dense intimate violent
    {
        "label": "BRIGHT_COLD_DENSE_INTIMATE_VIOLENT",
        "tags": ["bright", "cold", "dense", "intimate", "violent"],
        "spec": {"brightness": "XHIGH", "warmth": "XLOW", "density": "XHIGH", "space": "XLOW", "aggression": "XHIGH"},
    },
    # 12: dark hot kinetic sparse violent
    {
        "label": "DARK_HOT_KINETIC_SPARSE_VIOLENT",
        "tags": ["dark", "hot", "kinetic", "sparse", "violent"],
        "spec": {"brightness": "XLOW", "warmth": "XHIGH", "movement": "XHIGH", "density": "XLOW", "aggression": "XHIGH"},
    },
    # 13: bright warm still intimate violent
    {
        "label": "BRIGHT_WARM_STILL_INTIMATE_VIOLENT",
        "tags": ["bright", "warm", "still", "intimate", "violent"],
        "spec": {"brightness": "XHIGH", "warmth": "XHIGH", "movement": "XLOW", "space": "XLOW", "aggression": "XHIGH"},
    },
    # 14: dark still dense vast gentle
    {
        "label": "DARK_STILL_DENSE_VAST_GENTLE",
        "tags": ["dark", "still", "dense", "vast", "gentle"],
        "spec": {"brightness": "XLOW", "movement": "XLOW", "density": "XHIGH", "space": "XHIGH", "aggression": "XLOW"},
    },
    # 15: bright cold kinetic sparse vast
    {
        "label": "BRIGHT_COLD_KINETIC_SPARSE_VAST",
        "tags": ["bright", "cold", "kinetic", "sparse", "vast"],
        "spec": {"brightness": "XHIGH", "warmth": "XLOW", "movement": "XHIGH", "density": "XLOW", "space": "XHIGH"},
    },
    # 16: hot kinetic dense intimate gentle
    {
        "label": "HOT_KINETIC_DENSE_INTIMATE_GENTLE",
        "tags": ["hot", "kinetic", "dense", "intimate", "gentle"],
        "spec": {"warmth": "XHIGH", "movement": "XHIGH", "density": "XHIGH", "space": "XLOW", "aggression": "XLOW"},
    },
    # 17: dark cold still dense vast
    {
        "label": "DARK_COLD_STILL_DENSE_VAST",
        "tags": ["dark", "cold", "still", "dense", "vast"],
        "spec": {"brightness": "XLOW", "warmth": "XLOW", "movement": "XLOW", "density": "XHIGH", "space": "XHIGH"},
    },
    # 18: bright cold dense vast violent
    {
        "label": "BRIGHT_COLD_DENSE_VAST_VIOLENT",
        "tags": ["bright", "cold", "dense", "vast", "violent"],
        "spec": {"brightness": "XHIGH", "density": "XHIGH", "space": "XHIGH", "aggression": "XHIGH", "warmth": "XLOW"},
    },
    # 19: bright hot still intimate gentle
    {
        "label": "BRIGHT_HOT_STILL_INTIMATE_GENTLE",
        "tags": ["bright", "hot", "still", "intimate", "gentle"],
        "spec": {"warmth": "XHIGH", "movement": "XLOW", "space": "XLOW", "aggression": "XLOW", "brightness": "XHIGH"},
    },
    # 20: dark hot kinetic vast violent
    {
        "label": "DARK_HOT_KINETIC_VAST_VIOLENT",
        "tags": ["dark", "hot", "kinetic", "vast", "violent"],
        "spec": {"brightness": "XLOW", "warmth": "XHIGH", "movement": "XHIGH", "space": "XHIGH", "aggression": "XHIGH"},
    },
]


def coupling_intensity(amount):
    if amount >= 0.7:
        return "High"
    elif amount >= 0.4:
        return "Medium"
    return "Low"


def make_preset(mood, engines_pool, corner, variant_idx):
    """Generate one .xometa preset dict."""
    label = corner["label"]
    suffix = "FND" if mood == "Foundation" else "PRS"
    name = f"{label}_{suffix}_{variant_idx}"

    dna = dna_from_spec(corner["spec"])

    # Pick 2 engines (no repeat within a preset; vary by variant)
    eng_pool = list(engines_pool)
    random.shuffle(eng_pool)
    engine1, engine2 = eng_pool[variant_idx % len(eng_pool)], eng_pool[(variant_idx + 1) % len(eng_pool)]
    if engine1 == engine2:
        engine2 = eng_pool[(variant_idx + 2) % len(eng_pool)]

    # Engine macros derived loosely from DNA
    def eng_params(seed_offset):
        return {
            "macro_character": round(max(0.0, min(1.0, dna["brightness"] + random.uniform(-0.1, 0.1) + seed_offset)), 3),
            "macro_movement": round(max(0.0, min(1.0, dna["movement"] + random.uniform(-0.1, 0.1))), 3),
            "macro_coupling": round(max(0.0, min(1.0, dna["density"] + random.uniform(-0.15, 0.15))), 3),
            "macro_space": round(max(0.0, min(1.0, dna["space"] + random.uniform(-0.1, 0.1))), 3),
        }

    params = {
        engine1: eng_params(0.0),
        engine2: eng_params(0.05),
    }

    coupling_type = random.choice(VALID_COUPLING_TYPES)
    coupling_amount = round(random.uniform(0.45, 0.97), 3)

    macro_char = round((params[engine1]["macro_character"] + params[engine2]["macro_character"]) / 2, 3)
    macro_mov = round((params[engine1]["macro_movement"] + params[engine2]["macro_movement"]) / 2, 3)
    macro_coup = round((params[engine1]["macro_coupling"] + params[engine2]["macro_coupling"]) / 2, 3)
    macro_space = round((params[engine1]["macro_space"] + params[engine2]["macro_space"]) / 2, 3)

    mood_tag = mood.lower()
    tags = list(set([mood_tag, "extreme", "diversity"] + corner["tags"]))

    preset = {
        "schema_version": 1,
        "name": name,
        "mood": mood,
        "engines": [engine1, engine2],
        "author": "XO_OX",
        "version": "1.0.0",
        "description": (
            f"Extreme-DNA diversity anchor. 5 dimensions in maximum zones; 1 dimension midrange. "
            f"Corner: {label}. Variant {variant_idx}."
        ),
        "tags": tags,
        "macroLabels": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
        "couplingIntensity": coupling_intensity(coupling_amount),
        "tempo": None,
        "created": str(date.today()),
        "legacy": {
            "sourceInstrument": None,
            "sourceCategory": None,
            "sourcePresetName": None,
        },
        "parameters": params,
        "coupling": {
            "type": coupling_type,
            "source": engine1,
            "target": engine2,
            "amount": coupling_amount,
        },
        "sequencer": None,
        "dna": dna,
        "macros": {
            "CHARACTER": macro_char,
            "MOVEMENT": macro_mov,
            "COUPLING": macro_coup,
            "SPACE": macro_space,
        },
    }
    return preset


def generate_batch(mood, engines_pool, corners, out_dir):
    os.makedirs(out_dir, exist_ok=True)
    generated = []
    for corner in corners:
        for variant in range(1, 6):
            preset = make_preset(mood, engines_pool, corner, variant)
            filename = f"{preset['name']}.xometa"
            filepath = os.path.join(out_dir, filename)
            with open(filepath, "w") as f:
                json.dump(preset, f, indent=2)
            generated.append(filepath)
    return generated


def main():
    print("Generating Foundation presets (corners 1-10, 5 variants each = 50)...")
    fnd_files = generate_batch("Foundation", FOUNDATION_ENGINES, FOUNDATION_CORNERS, FOUNDATION_DIR)
    print(f"  Written {len(fnd_files)} Foundation presets to {FOUNDATION_DIR}")

    print("Generating Prism presets (corners 11-20, 5 variants each = 50)...")
    prism_files = generate_batch("Prism", PRISM_ENGINES, PRISM_CORNERS, PRISM_DIR)
    print(f"  Written {len(prism_files)} Prism presets to {PRISM_DIR}")

    total = len(fnd_files) + len(prism_files)
    print(f"\nDone. {total} total presets generated.")

    # Quick DNA diversity report
    print("\nSample DNA corners (first preset per corner):")
    for f in fnd_files[::5]:
        with open(f) as fh:
            p = json.load(fh)
        dna = p["dna"]
        print(f"  {p['name']}: B={dna['brightness']:.3f} W={dna['warmth']:.3f} "
              f"M={dna['movement']:.3f} D={dna['density']:.3f} "
              f"S={dna['space']:.3f} A={dna['aggression']:.3f}")


if __name__ == "__main__":
    main()
