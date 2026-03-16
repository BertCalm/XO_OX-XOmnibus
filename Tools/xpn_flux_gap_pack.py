#!/usr/bin/env python3
"""
xpn_flux_gap_pack.py
Generates 80 Flux presets filling two per-mood DNA gaps:
  - Part 1: 40 presets with warmth 0.02-0.12
  - Part 2: 40 presets with density 0.02-0.12
"""

import json
import os
import random

OUTPUT_DIR = "Presets/Flux"

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

COLD_NAMES = [
    "COLD FLUX", "CRYO FLUX", "ICE FLUX", "ARCTIC FLOW", "FROZEN FLUX"
]

SPARSE_NAMES = [
    "SPARSE FLUX", "THIN FLOW", "BARE FLUX", "HOLLOW CURRENT", "EMPTY FLUX"
]

COLD_TAGS = ["flux", "cold", "shifting"]
SPARSE_TAGS = ["flux", "sparse", "minimal"]


def rand(lo=0.0, hi=1.0):
    return round(random.uniform(lo, hi), 4)


def extreme_value():
    """Return a value that is extreme (<=0.15 or >=0.85)."""
    if random.random() < 0.5:
        return round(random.uniform(0.02, 0.15), 4)
    else:
        return round(random.uniform(0.85, 0.99), 4)


def engine_params():
    return {
        "macro_character": rand(),
        "macro_movement": rand(),
        "macro_coupling": rand(),
        "macro_space": rand(),
    }


def make_preset(name, mood, engines_pair, coupling_type, dna, tags):
    e1, e2 = engines_pair
    return {
        "name": name,
        "version": "1.0",
        "mood": mood,
        "engines": [e1, e2],
        "parameters": {
            e1: engine_params(),
            e2: engine_params(),
        },
        "coupling": {
            "type": coupling_type,
            "source": e1,
            "target": e2,
            "amount": rand(0.3, 0.9),
        },
        "dna": dna,
        "macros": {
            "CHARACTER": rand(),
            "MOVEMENT": rand(),
            "COUPLING": rand(),
            "SPACE": rand(),
        },
        "tags": tags,
    }


def safe_filename(name):
    return name.replace(" ", "_") + ".xometa"


def write_presets(presets, output_dir):
    os.makedirs(output_dir, exist_ok=True)
    written = 0
    skipped = 0
    for preset in presets:
        fname = safe_filename(preset["name"])
        fpath = os.path.join(output_dir, fname)
        if os.path.exists(fpath):
            skipped += 1
            continue
        with open(fpath, "w") as f:
            json.dump(preset, f, indent=2)
        written += 1
    return written, skipped


def pick_engine_pair(index, engines):
    """Deterministically vary engine pairs across 40 presets."""
    i = index * 7 % len(engines)
    j = (index * 13 + 5) % len(engines)
    if i == j:
        j = (j + 1) % len(engines)
    return engines[i], engines[j]


def build_cold_presets():
    """40 presets: warmth 0.02-0.12, at least 1 other extreme dimension."""
    presets = []
    name_cycle = COLD_NAMES
    for i in range(40):
        base_name = name_cycle[i % len(name_cycle)]
        # Suffix to make unique: use number only if needed
        suffix = f" {i // len(name_cycle) + 1}" if i >= len(name_cycle) else ""
        name = base_name + suffix

        coupling_type = COUPLING_TYPES[i % len(COUPLING_TYPES)]
        engines_pair = pick_engine_pair(i, ENGINES)

        warmth = round(random.uniform(0.02, 0.12), 4)

        # Pick one other dimension to be extreme
        other_dims = ["brightness", "movement", "density", "space", "aggression"]
        extreme_dim = other_dims[i % len(other_dims)]

        dna = {
            "brightness": rand(),
            "warmth": warmth,
            "movement": rand(),
            "density": rand(),
            "space": rand(),
            "aggression": rand(),
        }
        dna[extreme_dim] = extreme_value()

        presets.append(make_preset(name, "Flux", engines_pair, coupling_type, dna, COLD_TAGS[:]))
    return presets


def build_sparse_presets():
    """40 presets: density 0.02-0.12, at least 1 other extreme dimension."""
    presets = []
    name_cycle = SPARSE_NAMES
    for i in range(40):
        base_name = name_cycle[i % len(name_cycle)]
        suffix = f" {i // len(name_cycle) + 1}" if i >= len(name_cycle) else ""
        name = base_name + suffix

        coupling_type = COUPLING_TYPES[i % len(COUPLING_TYPES)]
        engines_pair = pick_engine_pair(i + 20, ENGINES)  # offset for variety

        density = round(random.uniform(0.02, 0.12), 4)

        other_dims = ["brightness", "warmth", "movement", "space", "aggression"]
        extreme_dim = other_dims[i % len(other_dims)]

        dna = {
            "brightness": rand(),
            "warmth": rand(),
            "movement": rand(),
            "density": density,
            "space": rand(),
            "aggression": rand(),
        }
        dna[extreme_dim] = extreme_value()

        presets.append(make_preset(name, "Flux", engines_pair, coupling_type, dna, SPARSE_TAGS[:]))
    return presets


def main():
    random.seed(42)  # reproducible output

    cold_presets = build_cold_presets()
    sparse_presets = build_sparse_presets()

    cold_written, cold_skipped = write_presets(cold_presets, OUTPUT_DIR)
    sparse_written, sparse_skipped = write_presets(sparse_presets, OUTPUT_DIR)

    total_written = cold_written + sparse_written
    total_skipped = cold_skipped + sparse_skipped

    print(f"Part 1 (warmth-XLOW): {cold_written} written, {cold_skipped} skipped")
    print(f"Part 2 (density-XLOW): {sparse_written} written, {sparse_skipped} skipped")
    print(f"Total: {total_written} written, {total_skipped} skipped")


if __name__ == "__main__":
    main()
