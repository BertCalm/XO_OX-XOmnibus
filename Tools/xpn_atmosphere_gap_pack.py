#!/usr/bin/env python3
"""
xpn_atmosphere_gap_pack.py
Generates 80 .xometa Atmosphere presets filling two critical per-mood gaps:
  Part 1: 40 presets with space ≤0.10 (space-XLOW)
  Part 2: 40 presets with aggression ≥0.88 (aggression-XHIGH)
"""

import json
import os
import random

OUTPUT_DIR = os.path.join(
    os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
    "Presets", "Atmosphere"
)

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

# Part 1 name pool — close/intimate/claustrophobic atmosphere
XLOW_SPACE_NAMES = [
    "CLOSE HAZE", "CLAUSTRAL MIST", "INTIMATE FOG", "PRESSURE VEIL",
    "SEALED ATMOSPHERE", "COMPRESSED CLOUD", "NEAR BREATH", "TIGHT VAPOR",
    "POCKET AIR", "DENSE SHROUD", "HOLLOW CLOSE", "BOXED MIST",
    "CONFINED HAZE", "PRESSED VAPOR", "INTERIOR FOG", "CRUSH CLOUD",
    "CABIN AIR", "NARROW VEIL", "SUFFUSED MIST", "CLAMPED HAZE",
    "SHALLOW BREATH", "TRAPPED VAPOR", "AIRLESS FOG", "SEALED CLOUD",
    "COMPACT SHROUD", "CROWDED MIST", "ENCLOSED HAZE", "CLOISTERED AIR",
    "PINCHED FOG", "DENSE BREATH", "CELL VAPOR", "CRAMMED CLOUD",
    "AIRLOCK MIST", "BUNKER HAZE", "VAULT FOG", "CRYPT VAPOR",
    "CISTERN AIR", "CAVITY MIST", "CHASM HAZE", "HOLLOW PRESSURE"
]

# Part 2 name pool — harsh/abrasive/hostile atmosphere
XHIGH_AGGRESSION_NAMES = [
    "ABRASIVE HAZE", "CORROSIVE FOG", "HOSTILE MIST", "VIOLENT ATMOSPHERE",
    "TOXIC VEIL", "CAUSTIC CLOUD", "ACID SHROUD", "BRUTAL VAPOR",
    "SCORCH MIST", "SEARING HAZE", "VENOM FOG", "SCORCHED AIR",
    "BITING CLOUD", "SAVAGE VAPOR", "HARSH BREATH", "ERODING MIST",
    "ACRID HAZE", "BURNING FOG", "SCALDING CLOUD", "FERAL MIST",
    "RAW PRESSURE", "BLISTER VAPOR", "ABRADE CLOUD", "CHAR HAZE",
    "ETCH MIST", "CORRODE FOG", "RAVAGE VAPOR", "SCAR CLOUD",
    "SLAG BREATH", "CINDER HAZE", "EMBER FOG", "BLAZE MIST",
    "TORCH VAPOR", "SMELT CLOUD", "FORGE HAZE", "CRUCIBLE FOG",
    "BRAND MIST", "SCALD VAPOR", "FLARE HAZE", "INFERNO BREATH"
]


def rnd(lo, hi):
    return round(random.uniform(lo, hi), 3)


def extreme_other(exclude_key):
    """Return a dict of DNA dimensions with at least one extra extreme."""
    dims = ["brightness", "warmth", "movement", "density"]
    dims = [d for d in dims if d != exclude_key]
    chosen = random.choice(dims)
    result = {}
    for d in dims:
        if d == chosen:
            # extreme: either ≤0.15 or ≥0.85
            result[d] = rnd(0.0, 0.15) if random.random() < 0.5 else rnd(0.85, 1.0)
        else:
            result[d] = rnd(0.1, 0.9)
    return result


def engine_params(space_val):
    return {
        "macro_character": rnd(0.1, 0.9),
        "macro_movement": rnd(0.1, 0.9),
        "macro_coupling": rnd(0.1, 0.9),
        "macro_space": round(space_val, 3),
    }


def make_preset_xlow_space(name, engine_pair, coupling_type, idx):
    e1, e2 = engine_pair
    space = rnd(0.02, 0.10)
    other = extreme_other("space")

    dna = {
        "brightness": other.get("brightness", rnd(0.1, 0.9)),
        "warmth": other.get("warmth", rnd(0.1, 0.9)),
        "movement": other.get("movement", rnd(0.1, 0.9)),
        "density": other.get("density", rnd(0.1, 0.9)),
        "space": round(space, 3),
        "aggression": rnd(0.05, 0.75),
    }

    tags = ["atmosphere", "claustrophobic", "intimate", "close", "space-xlow"]

    return {
        "name": name,
        "version": "1.0",
        "mood": "Atmosphere",
        "engines": [e1, e2],
        "parameters": {
            e1: engine_params(space),
            e2: engine_params(rnd(0.02, 0.10)),
        },
        "coupling": {
            "type": coupling_type,
            "source": e1,
            "target": e2,
            "amount": rnd(0.2, 0.9),
        },
        "dna": dna,
        "macros": {
            "CHARACTER": rnd(0.1, 0.9),
            "MOVEMENT": rnd(0.1, 0.9),
            "COUPLING": rnd(0.1, 0.9),
            "SPACE": round(space, 3),
        },
        "tags": tags,
    }


def make_preset_xhigh_aggression(name, engine_pair, coupling_type, idx):
    e1, e2 = engine_pair
    aggression = rnd(0.88, 0.98)
    other = extreme_other("aggression")

    dna = {
        "brightness": other.get("brightness", rnd(0.1, 0.9)),
        "warmth": other.get("warmth", rnd(0.1, 0.9)),
        "movement": other.get("movement", rnd(0.1, 0.9)),
        "density": other.get("density", rnd(0.1, 0.9)),
        "space": rnd(0.1, 0.9),
        "aggression": round(aggression, 3),
    }

    tags = ["atmosphere", "harsh", "abrasive", "hostile", "aggression-xhigh"]

    return {
        "name": name,
        "version": "1.0",
        "mood": "Atmosphere",
        "engines": [e1, e2],
        "parameters": {
            e1: engine_params(rnd(0.1, 0.9)),
            e2: engine_params(rnd(0.1, 0.9)),
        },
        "coupling": {
            "type": coupling_type,
            "source": e1,
            "target": e2,
            "amount": rnd(0.2, 0.9),
        },
        "dna": dna,
        "macros": {
            "CHARACTER": rnd(0.1, 0.9),
            "MOVEMENT": rnd(0.1, 0.9),
            "COUPLING": rnd(0.1, 0.9),
            "SPACE": round(dna["space"], 3),
        },
        "tags": tags,
    }


def generate_engine_pairs(n):
    """Generate n varied engine pairs, cycling through all engines."""
    pairs = []
    engines = ENGINES[:]
    random.shuffle(engines)
    for i in range(n):
        e1 = engines[i % len(engines)]
        e2 = engines[(i + 7) % len(engines)]
        if e1 == e2:
            e2 = engines[(i + 13) % len(engines)]
        pairs.append((e1, e2))
    return pairs


def safe_filename(name):
    return name.replace(" ", "_").replace("/", "-") + ".xometa"


def write_preset(preset, part_label):
    fname = safe_filename(preset["name"])
    fpath = os.path.join(OUTPUT_DIR, fname)
    if os.path.exists(fpath):
        return False
    with open(fpath, "w") as f:
        json.dump(preset, f, indent=2)
    return True


def main():
    random.seed(42)
    os.makedirs(OUTPUT_DIR, exist_ok=True)

    coupling_cycle_1 = [COUPLING_TYPES[i % len(COUPLING_TYPES)] for i in range(40)]
    coupling_cycle_2 = [COUPLING_TYPES[(i + 3) % len(COUPLING_TYPES)] for i in range(40)]

    pairs_1 = generate_engine_pairs(40)
    pairs_2 = generate_engine_pairs(40)

    # --- Part 1: space XLOW ---
    written_1 = 0
    for i in range(40):
        name = XLOW_SPACE_NAMES[i]
        preset = make_preset_xlow_space(name, pairs_1[i], coupling_cycle_1[i], i)
        if write_preset(preset, "Part1"):
            written_1 += 1

    # --- Part 2: aggression XHIGH ---
    written_2 = 0
    for i in range(40):
        name = XHIGH_AGGRESSION_NAMES[i]
        preset = make_preset_xhigh_aggression(name, pairs_2[i], coupling_cycle_2[i], i)
        if write_preset(preset, "Part2"):
            written_2 += 1

    print(f"Part 1 (space XLOW  ≤0.10): {written_1}/40 files written")
    print(f"Part 2 (aggression XHIGH ≥0.88): {written_2}/40 files written")
    print(f"Total: {written_1 + written_2}/80 files written")
    print(f"Output dir: {OUTPUT_DIR}")


if __name__ == "__main__":
    main()
