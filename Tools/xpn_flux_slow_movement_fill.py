#!/usr/bin/env python3
"""
xpn_flux_slow_movement_fill.py
Generates 60 Flux presets with movement-XLOW (≤ 0.13) to counterweight
the kinetic skew in Flux (movement-XHIGH was 40.9% of the mood).

12 corners × 5 variants = 60 presets, all saved to Presets/XOmnibus/Flux/
"""

import json
import random
from pathlib import Path

SCRIPT_DIR = Path(__file__).parent
REPO_ROOT = SCRIPT_DIR.parent
OUTPUT_DIR = REPO_ROOT / "Presets" / "XOmnibus" / "Flux"

FLUX_ENGINES = [
    "OUROBOROS", "ORACLE", "ORGANON", "ORIGAMI", "OPTIC",
    "OPAL", "OBLONG", "ODYSSEY", "OVERDUB", "OBLIQUE",
    "OCEANIC", "OVERWORLD"
]

COUPLING_TYPES = [
    "FREQUENCY_SHIFT", "AMPLITUDE_MOD", "FILTER_MOD", "PITCH_SYNC",
    "TIMBRE_BLEND", "ENVELOPE_LINK", "HARMONIC_FOLD", "CHAOS_INJECT",
    "RESONANCE_SHARE", "SPATIAL_COUPLE", "SPECTRAL_MORPH", "VELOCITY_COUPLE"
]

# Range helpers
def xlow_movement():
    return round(random.uniform(0.01, 0.10), 3)

def xlow():
    return round(random.uniform(0.02, 0.13), 3)

def xhigh():
    return round(random.uniform(0.87, 0.99), 3)

def mid():
    return round(random.uniform(0.3, 0.7), 3)


# 12 corner definitions: (name_prefix, dna_fn, tags)
def corner_dna(corner_idx, seed):
    random.seed(seed)
    mv = xlow_movement()

    if corner_idx == 1:
        # bright warm still dense
        return {
            "brightness": xhigh(), "warmth": xhigh(), "movement": mv,
            "density": xhigh(), "space": mid(), "aggression": mid()
        }, ["flux", "still", "bright", "warm", "dense", "suspended-chaos"]

    elif corner_idx == 2:
        # dark cold still violent
        return {
            "brightness": xlow(), "warmth": xlow(), "movement": mv,
            "density": mid(), "space": mid(), "aggression": xhigh()
        }, ["flux", "still", "dark", "cold", "violent", "frozen-instability"]

    elif corner_idx == 3:
        # bright cold still vast
        return {
            "brightness": xhigh(), "warmth": xlow(), "movement": mv,
            "density": mid(), "space": xhigh(), "aggression": mid()
        }, ["flux", "still", "bright", "cold", "vast", "imperceptible-morph"]

    elif corner_idx == 4:
        # dark hot still dense
        return {
            "brightness": xlow(), "warmth": xhigh(), "movement": mv,
            "density": xhigh(), "space": mid(), "aggression": mid()
        }, ["flux", "still", "dark", "warm", "dense", "suspended-chaos"]

    elif corner_idx == 5:
        # bright still dense violent
        return {
            "brightness": xhigh(), "warmth": mid(), "movement": mv,
            "density": xhigh(), "space": mid(), "aggression": xhigh()
        }, ["flux", "still", "bright", "dense", "violent", "frozen-instability"]

    elif corner_idx == 6:
        # dark still vast gentle
        return {
            "brightness": xlow(), "warmth": mid(), "movement": mv,
            "density": mid(), "space": xhigh(), "aggression": xlow()
        }, ["flux", "still", "dark", "vast", "gentle", "suspended-chaos"]

    elif corner_idx == 7:
        # hot still dense violent
        return {
            "brightness": mid(), "warmth": xhigh(), "movement": mv,
            "density": xhigh(), "space": mid(), "aggression": xhigh()
        }, ["flux", "still", "warm", "dense", "violent", "imperceptible-morph"]

    elif corner_idx == 8:
        # cold still vast violent
        return {
            "brightness": mid(), "warmth": xlow(), "movement": mv,
            "density": mid(), "space": xhigh(), "aggression": xhigh()
        }, ["flux", "still", "cold", "vast", "violent", "frozen-instability"]

    elif corner_idx == 9:
        # bright warm still vast
        return {
            "brightness": xhigh(), "warmth": xhigh(), "movement": mv,
            "density": mid(), "space": xhigh(), "aggression": mid()
        }, ["flux", "still", "bright", "warm", "vast", "suspended-chaos"]

    elif corner_idx == 10:
        # dark cold still sparse
        return {
            "brightness": xlow(), "warmth": xlow(), "movement": mv,
            "density": xlow(), "space": mid(), "aggression": mid()
        }, ["flux", "still", "dark", "cold", "sparse", "imperceptible-morph"]

    elif corner_idx == 11:
        # bright still intimate violent
        return {
            "brightness": xhigh(), "warmth": mid(), "movement": mv,
            "density": mid(), "space": xlow(), "aggression": xhigh()
        }, ["flux", "still", "bright", "intimate", "violent", "frozen-instability"]

    elif corner_idx == 12:
        # hot still vast gentle
        return {
            "brightness": mid(), "warmth": xhigh(), "movement": mv,
            "density": mid(), "space": xhigh(), "aggression": xlow()
        }, ["flux", "still", "warm", "vast", "gentle", "suspended-chaos"]


CORNER_PREFIXES = {
    1:  "BRIGHT_WARM_STILL_DENSE",
    2:  "DARK_COLD_STILL_VIOLENT",
    3:  "BRIGHT_COLD_STILL_VAST",
    4:  "DARK_HOT_STILL_DENSE",
    5:  "BRIGHT_STILL_DENSE_VIOLENT",
    6:  "DARK_STILL_VAST_GENTLE",
    7:  "HOT_STILL_DENSE_VIOLENT",
    8:  "COLD_STILL_VAST_VIOLENT",
    9:  "BRIGHT_WARM_STILL_VAST",
    10: "DARK_COLD_STILL_SPARSE",
    11: "BRIGHT_STILL_INTIMATE_VIOLENT",
    12: "HOT_STILL_VAST_GENTLE",
}


def pick_engines(seed):
    random.seed(seed + 1000)
    engines = random.sample(FLUX_ENGINES, 2)
    return engines[0], engines[1]


def make_preset(corner_idx, variant_idx):
    seed = corner_idx * 100 + variant_idx
    dna, tags = corner_dna(corner_idx, seed)
    engine1, engine2 = pick_engines(seed)

    random.seed(seed + 2000)
    mv = dna["movement"]

    # Per-engine macro values derived from DNA
    def engine_macros(eng_seed_offset):
        random.seed(seed + eng_seed_offset)
        char = round(random.uniform(0.82, 0.99) if dna["density"] > 0.7 else random.uniform(0.3, 0.7), 3)
        coup = round(random.uniform(0.65, 0.95), 3)
        sp = round(dna["space"] * random.uniform(0.9, 1.1), 3)
        sp = max(0.0, min(1.0, sp))
        return {
            "macro_character": char,
            "macro_movement": round(mv * random.uniform(0.8, 1.2), 3),
            "macro_coupling": coup,
            "macro_space": round(sp, 3)
        }

    params_e1 = engine_macros(3000)
    params_e2 = engine_macros(4000)

    # Clamp movement to XLOW ceiling
    params_e1["macro_movement"] = min(params_e1["macro_movement"], 0.13)
    params_e2["macro_movement"] = min(params_e2["macro_movement"], 0.13)

    random.seed(seed + 5000)
    coupling_type = random.choice(COUPLING_TYPES)
    coupling_amount = round(random.uniform(0.6, 0.95), 3)

    random.seed(seed + 6000)
    macro_movement = round(min(mv, 0.13), 3)
    macro_coupling = round(random.uniform(0.65, 0.9), 3)
    macro_space = round(dna["space"], 3)
    macro_character = round((dna["density"] + dna["aggression"]) / 2, 3)

    prefix = CORNER_PREFIXES[corner_idx]
    preset_name = f"{prefix}_FLX_SLW_{variant_idx}"

    return {
        "name": preset_name,
        "version": "1.0",
        "mood": "Flux",
        "engines": [engine1, engine2],
        "parameters": {
            engine1: params_e1,
            engine2: params_e2
        },
        "coupling": {
            "type": coupling_type,
            "source": engine1,
            "target": engine2,
            "amount": coupling_amount
        },
        "dna": dna,
        "macros": {
            "CHARACTER": macro_character,
            "MOVEMENT": macro_movement,
            "COUPLING": macro_coupling,
            "SPACE": macro_space
        },
        "tags": tags
    }


def main():
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

    generated = []
    for corner in range(1, 13):
        for variant in range(1, 6):
            preset = make_preset(corner, variant)
            filename = f"{preset['name']}.xometa"
            filepath = OUTPUT_DIR / filename
            with open(filepath, "w") as f:
                json.dump(preset, f, indent=2)
            generated.append((preset["name"], preset["dna"]["movement"], filepath))

    print(f"Generated {len(generated)} presets → {OUTPUT_DIR}\n")

    # Verify movement constraint
    violations = [(n, m) for n, m, _ in generated if m > 0.13]
    if violations:
        print(f"VIOLATION — {len(violations)} preset(s) exceed movement 0.13:")
        for n, m in violations:
            print(f"  {n}: movement={m}")
    else:
        print("All presets PASS movement ≤ 0.13 check.")

    print(f"\nSample output:")
    for name, mv, path in generated[:3]:
        print(f"  {path.name}  movement={mv}")
    print(f"  ... ({len(generated) - 3} more)")

    # Summary by corner
    print("\nCorner summary (movement range):")
    for corner in range(1, 13):
        corner_presets = [(n, m) for n, m, _ in generated if f"_FLX_SLW_" in n and CORNER_PREFIXES[corner] in n]
        mvs = [m for _, m in corner_presets]
        print(f"  Corner {corner:2d} ({CORNER_PREFIXES[corner][:30]}): "
              f"movement {min(mvs):.3f}–{max(mvs):.3f}")


if __name__ == "__main__":
    main()
