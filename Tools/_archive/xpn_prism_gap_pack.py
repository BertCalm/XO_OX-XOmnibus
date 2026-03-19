#!/usr/bin/env python3
"""
xpn_prism_gap_pack.py
Generates 80 Prism .xometa presets filling two critical per-mood gaps:
  Part 1: 40 presets with brightness <= 0.12 (brightness-XLOW)
  Part 2: 40 presets with warmth >= 0.88 (warmth-XHIGH) + boosted aggression
"""

import json
import os
import random

OUTPUT_DIR = "Presets/Prism"

ENGINES = [
    "ODDFELIX", "ODDOSCAR", "OVERDUB", "ODYSSEY", "OBLONG", "OBESE", "ONSET",
    "OVERWORLD", "OPAL", "ORBITAL", "ORGANON", "OUROBOROS", "OBSIDIAN", "OVERBITE",
    "ORIGAMI", "ORACLE", "OBSCURA", "OCEANIC", "OCELOT", "OPTIC", "OBLIQUE",
    "OSPREY", "OSTERIA", "OWLFISH", "OHM", "ORPHICA", "OBBLIGATO", "OTTONI",
    "OLE", "OMBRE", "ORCA", "OCTOPUS", "OVERLAP", "OUTWIT"
]

COUPLING_TYPES = [
    "FREQUENCY_SHIFT", "AMPLITUDE_MOD", "FILTER_MOD", "PITCH_SYNC",
    "TIMBRE_BLEND", "ENVELOPE_LINK", "HARMONIC_FOLD", "CHAOS_INJECT",
    "RESONANCE_SHARE", "SPATIAL_COUPLE", "SPECTRAL_MORPH", "VELOCITY_COUPLE"
]

DARK_NAMES = [
    "DARK PRISM", "SHADOW REFRACTION", "BLACK SPECTRUM", "OBSIDIAN PRISM",
    "VOID PRISM", "ECLIPSE SPECTRUM", "MIDNIGHT REFRACTION", "SHADOW PRISM",
    "DARK SPECTRUM", "OBSIDIAN REFRACTION", "BLACK PRISM", "VOID REFRACTION",
    "ECLIPSE PRISM", "MIDNIGHT SPECTRUM", "SHADOW SPECTRUM", "DARK REFRACTION",
    "ONYX PRISM", "RAVEN SPECTRUM", "PITCH BLACK PRISM", "ABYSSAL REFRACTION",
    "COAL SPECTRUM", "DUSK PRISM", "UMBRA REFRACTION", "NOCTURNE SPECTRUM",
    "TENEBRIS PRISM", "CARBON REFRACTION", "OBSIDIAN SPECTRUM", "SHADOW LATTICE",
    "DARK LATTICE", "BLACK LATTICE", "VOID LATTICE", "ECLIPSE LATTICE",
    "SHADOW CRYSTAL", "DARK CRYSTAL", "BLACK CRYSTAL", "OBSIDIAN CRYSTAL",
    "VOID CRYSTAL", "ECLIPSE CRYSTAL", "MIDNIGHT LATTICE", "CHARCOAL PRISM"
]

WARM_NAMES = [
    "THERMAL PRISM", "WARM SPECTRUM", "FIRE REFRACTION", "EMBER PRISM",
    "SOLAR SPECTRUM", "BLAZE PRISM", "INFERNO REFRACTION", "SCORCHED SPECTRUM",
    "MAGMA PRISM", "FLAME REFRACTION", "HOT SPECTRUM", "FURNACE PRISM",
    "SEARING REFRACTION", "AMBER SPECTRUM", "TORCH PRISM", "COMBUSTION SPECTRUM",
    "SOLAR PRISM", "WILDFIRE REFRACTION", "CINDERBLOCK SPECTRUM", "PYROCLAST PRISM",
    "LAVA SPECTRUM", "CINDER REFRACTION", "IGNITE PRISM", "RADIANT SPECTRUM",
    "THERMAL REFRACTION", "WARM LATTICE", "FIRE LATTICE", "EMBER LATTICE",
    "SOLAR LATTICE", "BLAZE LATTICE", "THERMAL CRYSTAL", "WARM CRYSTAL",
    "FIRE CRYSTAL", "EMBER CRYSTAL", "SOLAR CRYSTAL", "BLAZE CRYSTAL",
    "INFERNO LATTICE", "MAGMA LATTICE", "SCORCHED LATTICE", "FURNACE SPECTRUM"
]


def random_engine_pair(rng):
    pair = rng.sample(ENGINES, 2)
    return pair[0], pair[1]


def random_params(rng):
    return {
        "macro_character": round(rng.uniform(0.0, 1.0), 3),
        "macro_movement": round(rng.uniform(0.0, 1.0), 3),
        "macro_coupling": round(rng.uniform(0.0, 1.0), 3),
        "macro_space": round(rng.uniform(0.0, 1.0), 3),
    }


def random_extreme_dimension(rng, exclude_keys):
    """Pick a DNA key not in exclude_keys and give it an extreme value."""
    candidates = [k for k in ["movement", "density", "space", "aggression"] if k not in exclude_keys]
    key = rng.choice(candidates)
    value = round(rng.uniform(0.85, 1.0), 3) if rng.random() > 0.5 else round(rng.uniform(0.0, 0.15), 3)
    return key, value


def build_dark_preset(name, idx, rng):
    """brightness <= 0.12, at least 1 other extreme dimension."""
    e1, e2 = random_engine_pair(rng)
    coupling_type = COUPLING_TYPES[idx % len(COUPLING_TYPES)]
    brightness = round(rng.uniform(0.02, 0.12), 3)
    warmth = round(rng.uniform(0.1, 0.9), 3)
    movement = round(rng.uniform(0.0, 1.0), 3)
    density = round(rng.uniform(0.0, 1.0), 3)
    space = round(rng.uniform(0.0, 1.0), 3)
    aggression = round(rng.uniform(0.0, 1.0), 3)

    # Force at least one other extreme
    extra_key, extra_val = random_extreme_dimension(rng, [])
    dna_map = {"brightness": brightness, "warmth": warmth, "movement": movement,
               "density": density, "space": space, "aggression": aggression}
    dna_map[extra_key] = extra_val

    return {
        "name": name,
        "version": "1.0",
        "mood": "Prism",
        "engines": [e1, e2],
        "parameters": {
            e1: random_params(rng),
            e2: random_params(rng),
        },
        "coupling": {
            "type": coupling_type,
            "source": e1,
            "target": e2,
            "amount": round(rng.uniform(0.3, 0.9), 3),
        },
        "dna": dna_map,
        "macros": {
            "CHARACTER": round(rng.uniform(0.0, 1.0), 3),
            "MOVEMENT": round(rng.uniform(0.0, 1.0), 3),
            "COUPLING": round(rng.uniform(0.0, 1.0), 3),
            "SPACE": round(rng.uniform(0.0, 1.0), 3),
        },
        "tags": ["prism", "dark", "spectrum"],
    }


def build_warm_preset(name, idx, rng):
    """warmth >= 0.88, aggression boosted (>= 0.75), at least 1 other extreme."""
    e1, e2 = random_engine_pair(rng)
    coupling_type = COUPLING_TYPES[(idx + 4) % len(COUPLING_TYPES)]
    brightness = round(rng.uniform(0.1, 0.9), 3)
    warmth = round(rng.uniform(0.88, 0.98), 3)
    movement = round(rng.uniform(0.0, 1.0), 3)
    density = round(rng.uniform(0.0, 1.0), 3)
    space = round(rng.uniform(0.0, 1.0), 3)
    aggression = round(rng.uniform(0.75, 1.0), 3)  # boosted

    dna_map = {"brightness": brightness, "warmth": warmth, "movement": movement,
               "density": density, "space": space, "aggression": aggression}

    # Force at least one more extreme (beyond warmth + aggression already high)
    extra_key, extra_val = random_extreme_dimension(rng, ["warmth", "aggression"])
    dna_map[extra_key] = extra_val

    return {
        "name": name,
        "version": "1.0",
        "mood": "Prism",
        "engines": [e1, e2],
        "parameters": {
            e1: random_params(rng),
            e2: random_params(rng),
        },
        "coupling": {
            "type": coupling_type,
            "source": e1,
            "target": e2,
            "amount": round(rng.uniform(0.3, 0.9), 3),
        },
        "dna": dna_map,
        "macros": {
            "CHARACTER": round(rng.uniform(0.0, 1.0), 3),
            "MOVEMENT": round(rng.uniform(0.0, 1.0), 3),
            "COUPLING": round(rng.uniform(0.0, 1.0), 3),
            "SPACE": round(rng.uniform(0.0, 1.0), 3),
        },
        "tags": ["prism", "warm", "spectrum", "fire"],
    }


def safe_filename(name):
    return name.replace(" ", "_").replace("/", "-") + ".xometa"


def main():
    rng = random.Random(20260316)  # deterministic seed
    os.makedirs(OUTPUT_DIR, exist_ok=True)

    written = 0
    skipped = 0

    # Part 1: 40 dark presets
    for i, name in enumerate(DARK_NAMES[:40]):
        preset = build_dark_preset(name, i, rng)
        filename = safe_filename(name)
        path = os.path.join(OUTPUT_DIR, filename)
        if os.path.exists(path):
            skipped += 1
            continue
        with open(path, "w") as f:
            json.dump(preset, f, indent=2)
        written += 1

    # Part 2: 40 warm presets
    for i, name in enumerate(WARM_NAMES[:40]):
        preset = build_warm_preset(name, i, rng)
        filename = safe_filename(name)
        path = os.path.join(OUTPUT_DIR, filename)
        if os.path.exists(path):
            skipped += 1
            continue
        with open(path, "w") as f:
            json.dump(preset, f, indent=2)
        written += 1

    print(f"Done. Written: {written}  Skipped (already existed): {skipped}")
    print(f"Output directory: {os.path.abspath(OUTPUT_DIR)}")


if __name__ == "__main__":
    main()
