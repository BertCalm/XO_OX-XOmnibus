"""
xpn_atmosphere_ultra_diverse.py
Generates 80 Atmosphere mood presets with extreme DNA diversity.
16 corner combinations × 5 variants = 80 presets.
"""

import os
import json
import random

random.seed(None)

PRESETS_DIR = os.path.join(
    os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
    "Presets", "XOceanus", "Atmosphere"
)
os.makedirs(PRESETS_DIR, exist_ok=True)

ATMOSPHERE_ENGINES = [
    "OPAL", "ORACLE", "OBSCURA", "OCEANIC", "OPTIC",
    "OBLIQUE", "OSPREY", "ORBITAL", "ORGANON",
    "ORIGAMI", "ODDFELIX", "ODDOSCAR"
]

COUPLING_TYPES = [
    "FREQUENCY_SHIFT", "AMPLITUDE_MOD", "FILTER_MOD", "PITCH_SYNC",
    "TIMBRE_BLEND", "ENVELOPE_LINK", "HARMONIC_FOLD", "CHAOS_INJECT",
    "RESONANCE_SHARE", "SPATIAL_COUPLE", "SPECTRAL_MORPH", "VELOCITY_COUPLE"
]

DNA_DIMS = ["brightness", "warmth", "movement", "density", "space", "aggression"]

def xlow():
    return round(random.uniform(0.03, 0.14), 3)

def xhigh():
    return round(random.uniform(0.86, 0.98), 3)

def mid():
    return round(random.uniform(0.3, 0.7), 3)

# 16 corner combinations: list of dicts mapping dim -> "XLOW" | "XHIGH" | None (mid)
CORNERS = [
    # 1. bright cold kinetic vast
    {"brightness": "XHIGH", "warmth": "XLOW", "movement": "XHIGH", "space": "XHIGH"},
    # 2. dark hot dense vast
    {"brightness": "XLOW", "warmth": "XHIGH", "density": "XHIGH", "space": "XHIGH"},
    # 3. bright warm kinetic gentle
    {"brightness": "XHIGH", "warmth": "XHIGH", "movement": "XHIGH", "aggression": "XLOW"},
    # 4. dark cold still intimate
    {"brightness": "XLOW", "warmth": "XLOW", "movement": "XLOW", "space": "XLOW"},
    # 5. bright sparse vast gentle
    {"brightness": "XHIGH", "density": "XLOW", "space": "XHIGH", "aggression": "XLOW"},
    # 6. dark kinetic sparse gentle
    {"brightness": "XLOW", "movement": "XHIGH", "density": "XLOW", "aggression": "XLOW"},
    # 7. hot kinetic vast gentle
    {"warmth": "XHIGH", "movement": "XHIGH", "space": "XHIGH", "aggression": "XLOW"},
    # 8. cold dense intimate violent
    {"warmth": "XLOW", "density": "XHIGH", "space": "XLOW", "aggression": "XHIGH"},
    # 9. bright warm sparse vast
    {"brightness": "XHIGH", "warmth": "XHIGH", "density": "XLOW", "space": "XHIGH"},
    # 10. dark still vast gentle
    {"brightness": "XLOW", "movement": "XLOW", "space": "XHIGH", "aggression": "XLOW"},
    # 11. cold kinetic dense violent
    {"warmth": "XLOW", "movement": "XHIGH", "density": "XHIGH", "aggression": "XHIGH"},
    # 12. bright still sparse vast
    {"brightness": "XHIGH", "movement": "XLOW", "density": "XLOW", "space": "XHIGH"},
    # 13. dark hot kinetic violent
    {"brightness": "XLOW", "warmth": "XHIGH", "movement": "XHIGH", "aggression": "XHIGH"},
    # 14. hot sparse vast gentle
    {"warmth": "XHIGH", "density": "XLOW", "space": "XHIGH", "aggression": "XLOW"},
    # 15. dark dense vast gentle
    {"brightness": "XLOW", "density": "XHIGH", "space": "XHIGH", "aggression": "XLOW"},
    # 16. bright cold dense violent
    {"brightness": "XHIGH", "warmth": "XLOW", "density": "XHIGH", "aggression": "XHIGH"},
]

CORNER_LABELS = [
    "BRIGHT_COLD_KINETIC_VAST",
    "DARK_HOT_DENSE_VAST",
    "BRIGHT_WARM_KINETIC_GENTLE",
    "DARK_COLD_STILL_INTIMATE",
    "BRIGHT_SPARSE_VAST_GENTLE",
    "DARK_KINETIC_SPARSE_GENTLE",
    "HOT_KINETIC_VAST_GENTLE",
    "COLD_DENSE_INTIMATE_VIOLENT",
    "BRIGHT_WARM_SPARSE_VAST",
    "DARK_STILL_VAST_GENTLE",
    "COLD_KINETIC_DENSE_VIOLENT",
    "BRIGHT_STILL_SPARSE_VAST",
    "DARK_HOT_KINETIC_VIOLENT",
    "HOT_SPARSE_VAST_GENTLE",
    "DARK_DENSE_VAST_GENTLE",
    "BRIGHT_COLD_DENSE_VIOLENT",
]

def build_dna(corner_spec):
    dna = {}
    for dim in DNA_DIMS:
        zone = corner_spec.get(dim)
        if zone == "XHIGH":
            dna[dim] = xhigh()
        elif zone == "XLOW":
            dna[dim] = xlow()
        else:
            dna[dim] = mid()
    return dna

def dna_to_tags(dna):
    tags = ["atmosphere"]
    if dna["brightness"] >= 0.85:
        tags.append("bright")
    elif dna["brightness"] <= 0.15:
        tags.append("dark")
    if dna["warmth"] >= 0.85:
        tags.append("warm")
    elif dna["warmth"] <= 0.15:
        tags.append("cold")
    if dna["movement"] >= 0.85:
        tags.append("kinetic")
    elif dna["movement"] <= 0.15:
        tags.append("still")
    if dna["density"] >= 0.85:
        tags.append("dense")
    elif dna["density"] <= 0.15:
        tags.append("sparse")
    if dna["space"] >= 0.85:
        tags.append("vast")
    elif dna["space"] <= 0.15:
        tags.append("intimate")
    if dna["aggression"] >= 0.85:
        tags.append("violent")
    elif dna["aggression"] <= 0.15:
        tags.append("gentle")
    return tags

def engine_macros(engine, dna):
    """Return macro parameters appropriate for the engine based on DNA."""
    return {
        "macro_character": round(random.uniform(0.3, 0.8), 3),
        "macro_movement": round(dna["movement"] * 0.8 + random.uniform(0.0, 0.2), 3),
        "macro_coupling": round(random.uniform(0.4, 0.9), 3),
        "macro_space": round(dna["space"] * 0.8 + random.uniform(0.0, 0.2), 3),
    }

def build_preset(label, variant_num, corner_spec):
    dna = build_dna(corner_spec)

    # Pick 2 distinct engines
    engines = random.sample(ATMOSPHERE_ENGINES, 2)
    coupling_type = random.choice(COUPLING_TYPES)
    coupling_amount = round(random.uniform(0.4, 0.95), 3)

    name = f"{label}_ATM_{variant_num}"

    parameters = {
        engines[0]: engine_macros(engines[0], dna),
        engines[1]: engine_macros(engines[1], dna),
    }

    tags = dna_to_tags(dna)

    preset = {
        "name": name,
        "version": "1.0",
        "mood": "Atmosphere",
        "engines": engines,
        "parameters": parameters,
        "coupling": {
            "type": coupling_type,
            "source": engines[0],
            "target": engines[1],
            "amount": coupling_amount,
        },
        "dna": dna,
        "macros": {
            "CHARACTER": parameters[engines[0]]["macro_character"],
            "MOVEMENT": parameters[engines[0]]["macro_movement"],
            "COUPLING": parameters[engines[0]]["macro_coupling"],
            "SPACE": parameters[engines[0]]["macro_space"],
        },
        "tags": tags,
    }
    return preset

def main():
    count = 0
    for corner_idx, (corner_spec, label) in enumerate(zip(CORNERS, CORNER_LABELS)):
        for variant in range(1, 6):
            preset = build_preset(label, variant, corner_spec)
            filename = f"{preset['name']}.xometa"
            filepath = os.path.join(PRESETS_DIR, filename)
            with open(filepath, "w") as f:
                json.dump(preset, f, indent=2)
            count += 1
            print(f"  [{count:02d}] {filename}")

    print(f"\nDone. {count} presets written to {PRESETS_DIR}")

if __name__ == "__main__":
    main()
