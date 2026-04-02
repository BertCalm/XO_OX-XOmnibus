#!/usr/bin/env python3
"""
xpn_aether_space_midrange_fill.py

Generates 60 Aether presets with space DNA in the 0.35–0.65 midrange,
filling the bimodal distribution gap (14.2% XLOW + 45.7% XHIGH, thin mid).

12 corners × 5 variants = 60 presets.
Each preset: space in [0.35, 0.65], other dims at extremes or non-extreme per corner spec.
"""

import json
import os
import random

random.seed(42)

PRESETS_DIR = os.path.join(
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

# DNA dimension ranges
def xlow():
    return round(random.uniform(0.02, 0.13), 3)

def xhigh():
    return round(random.uniform(0.87, 0.99), 3)

def mid():
    return round(random.uniform(0.3, 0.7), 3)

def space_mid():
    return round(random.uniform(0.35, 0.65), 3)


# 12 corners: (label, brightness, warmth, movement, density, space, aggression)
# None = mid range (0.3–0.7), "XLOW", "XHIGH", "MID" (space only)
CORNERS = [
    # 1: bright warm kinetic balanced gentle
    ("BRIGHT_WARM_KINETIC_BALANCED",    "XHIGH", "XHIGH", "XHIGH", None,   "MID", "XLOW"),
    # 2: dark cold dense balanced violent
    ("DARK_COLD_DENSE_BALANCED",        "XLOW",  "XLOW",  None,    "XHIGH","MID", "XHIGH"),
    # 3: bright warm dense balanced gentle
    ("BRIGHT_WARM_DENSE_BALANCED",      "XHIGH", "XHIGH", None,    "XHIGH","MID", "XLOW"),
    # 4: dark hot still balanced gentle
    ("DARK_HOT_STILL_BALANCED",         "XLOW",  "XHIGH", "XLOW",  None,   "MID", "XLOW"),
    # 5: bright cold kinetic balanced violent
    ("BRIGHT_COLD_KINETIC_BALANCED",    "XHIGH", "XLOW",  "XHIGH", None,   "MID", "XHIGH"),
    # 6: dark cold sparse balanced gentle
    ("DARK_COLD_SPARSE_BALANCED",       "XLOW",  "XLOW",  None,    "XLOW", "MID", "XLOW"),
    # 7: hot kinetic dense balanced violent
    ("HOT_KINETIC_DENSE_BALANCED",      None,    "XHIGH", "XHIGH", "XHIGH","MID", "XHIGH"),
    # 8: cold still sparse balanced gentle
    ("COLD_STILL_SPARSE_BALANCED",      None,    "XLOW",  "XLOW",  "XLOW", "MID", "XLOW"),
    # 9: bright warm sparse balanced violent
    ("BRIGHT_WARM_SPARSE_BALANCED",     "XHIGH", "XHIGH", None,    "XLOW", "MID", "XHIGH"),
    # 10: dark kinetic dense balanced gentle
    ("DARK_KINETIC_DENSE_BALANCED",     "XLOW",  None,    "XHIGH", "XHIGH","MID", "XLOW"),
    # 11: bright cold dense balanced gentle
    ("BRIGHT_COLD_DENSE_BALANCED",      "XHIGH", "XLOW",  None,    "XHIGH","MID", "XLOW"),
    # 12: dark hot kinetic balanced violent
    ("DARK_HOT_KINETIC_BALANCED",       "XLOW",  "XHIGH", "XHIGH", None,   "MID", "XHIGH"),
]

def resolve_dim(spec):
    if spec == "XLOW":
        return xlow()
    elif spec == "XHIGH":
        return xhigh()
    elif spec == "MID":
        return space_mid()
    else:  # None → non-extreme mid
        return mid()


def pick_two_engines():
    engines = random.sample(AETHER_ENGINES, 2)
    return engines[0], engines[1]


def make_preset(corner_label, brightness_spec, warmth_spec, movement_spec,
                density_spec, space_spec, aggression_spec, variant_idx):
    brightness = resolve_dim(brightness_spec)
    warmth = resolve_dim(warmth_spec)
    movement = resolve_dim(movement_spec)
    density = resolve_dim(density_spec)
    space = resolve_dim(space_spec)   # always "MID" → 0.35–0.65
    aggression = resolve_dim(aggression_spec)

    engine1, engine2 = pick_two_engines()
    coupling_type = random.choice(COUPLING_TYPES)
    coupling_amount = round(random.uniform(0.4, 0.95), 3)

    # macro_character maps to brightness/warmth blend
    macro_char = round((brightness + warmth) / 2, 3)
    macro_move = round(movement, 3)
    macro_coup = round(random.uniform(0.5, 0.95), 3)
    macro_space = round(space, 3)

    # slight per-engine variation (±0.03)
    def jitter(v):
        return round(min(0.99, max(0.01, v + random.uniform(-0.03, 0.03))), 3)

    name_base = f"{corner_label}_AET_SPC_{variant_idx}"
    # Filename safe: replace spaces with underscores (already no spaces)
    filename = f"{name_base}.xometa"

    # Determine tags
    tags = ["aether", "balanced", "room", "space-mid"]
    if aggression_spec == "XHIGH":
        tags.append("aggressive")
    if aggression_spec == "XLOW":
        tags.append("gentle")
    if brightness_spec == "XHIGH":
        tags.append("bright")
    if warmth_spec == "XHIGH":
        tags.append("warm")
    if movement_spec == "XHIGH":
        tags.append("kinetic")
    if density_spec == "XHIGH":
        tags.append("dense")

    preset = {
        "name": name_base.replace("_", " "),
        "version": "1.0",
        "mood": "Aether",
        "engines": [engine1, engine2],
        "parameters": {
            engine1: {
                "macro_character": macro_char,
                "macro_movement": macro_move,
                "macro_coupling": macro_coup,
                "macro_space": macro_space
            },
            engine2: {
                "macro_character": jitter(macro_char),
                "macro_movement": jitter(macro_move),
                "macro_coupling": jitter(macro_coup),
                "macro_space": jitter(macro_space)
            }
        },
        "coupling": {
            "type": coupling_type,
            "source": engine1,
            "target": engine2,
            "amount": coupling_amount
        },
        "dna": {
            "brightness": brightness,
            "warmth": warmth,
            "movement": movement,
            "density": density,
            "space": space,
            "aggression": aggression
        },
        "macros": {
            "CHARACTER": macro_char,
            "MOVEMENT": macro_move,
            "COUPLING": macro_coup,
            "SPACE": macro_space
        },
        "tags": tags
    }
    return filename, preset


def main():
    os.makedirs(PRESETS_DIR, exist_ok=True)

    generated = []
    space_values = []

    for (label, b, w, mv, d, sp, ag) in CORNERS:
        for v in range(1, 6):  # 5 variants
            filename, preset = make_preset(label, b, w, mv, d, sp, ag, v)
            filepath = os.path.join(PRESETS_DIR, filename)
            with open(filepath, "w") as f:
                json.dump(preset, f, indent=2)
            generated.append(filename)
            space_values.append(preset["dna"]["space"])

    # Verification
    print(f"Generated {len(generated)} presets to {PRESETS_DIR}\n")
    out_of_range = [
        (fn, sv) for fn, sv in zip(generated, space_values)
        if not (0.35 <= sv <= 0.65)
    ]
    if out_of_range:
        print("ERROR: The following presets have space OUTSIDE 0.35–0.65:")
        for fn, sv in out_of_range:
            print(f"  {fn}: space={sv}")
    else:
        print("PASS: All 60 presets have space in [0.35, 0.65]")

    print(f"\nSpace range: min={min(space_values):.3f}, max={max(space_values):.3f}, "
          f"mean={sum(space_values)/len(space_values):.3f}")

    aet_spc_files = [f for f in os.listdir(PRESETS_DIR) if "AET_SPC" in f]
    print(f"\nAET_SPC files in Presets/XOceanus/Aether/: {len(aet_spc_files)}")
    for f in sorted(aet_spc_files)[:10]:
        print(f"  {f}")
    if len(aet_spc_files) > 10:
        print(f"  ... and {len(aet_spc_files) - 10} more")


if __name__ == "__main__":
    main()
