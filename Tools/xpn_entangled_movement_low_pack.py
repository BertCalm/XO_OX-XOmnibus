#!/usr/bin/env python3
"""
xpn_entangled_movement_low_pack.py
Generates 80 .xometa presets targeting Entangled mood with movement <= 0.12 (XLOW zone).
"""

import json
import os
import random

OUTPUT_DIR = os.path.join(
    os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
    "Presets", "Entangled"
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

STILLNESS_WORDS = [
    "FROZEN", "STATIC", "SUSPENDED", "GLACIAL", "BECALMED", "INERT",
    "MOTIONLESS", "CRYSTALLIZED", "PETRIFIED", "DORMANT", "ARRESTED",
    "STILLED", "HUSHED", "BREATHLESS", "HELD", "FIXED", "IMMOBILE",
    "STAGNANT", "TRANSFIXED", "PARALYZED", "LOCKED", "ANCHORED",
    "PRESERVED", "ENTOMBED", "EMBEDDED", "SEALED", "ENTANGLED",
    "BOUND", "TETHERED", "ENSNARED", "CAUGHT", "TRAPPED", "CLASPED",
    "DRAPED", "VEILED", "SHROUDED", "CLOAKED", "MUFFLED", "DAMPENED",
    "QUIETED", "SILENCED", "MUTED", "HUSHED", "SUBDUED", "CALMED",
    "SETTLED", "LULLED", "SOOTHED", "PLACID", "SERENE", "STILL",
    "DEEP", "VAST", "HOLLOW", "HOLLOW", "DISTANT", "REMOTE",
    "COLD", "PALE", "GREY", "DIM", "FAINT", "SLOW"
]

MODIFIERS = [
    "DRIFT", "FIELD", "SPACE", "VOID", "POOL", "LAKE", "BASIN",
    "CHAMBER", "HOLLOW", "DEPTH", "CURRENT", "LAYER", "VEIL",
    "SHELL", "SKIN", "MEMBRANE", "THREAD", "WIRE", "LATTICE",
    "GRID", "WEB", "MESH", "NET", "KNOT", "NODE", "POINT",
    "PLANE", "ZONE", "RING", "HALO", "CLOUD", "MIST", "FOG",
    "FROST", "ICE", "GLASS", "MIRROR", "SURFACE", "EDGE", "MARGIN"
]

def rand(lo=0.0, hi=1.0):
    return round(random.uniform(lo, hi), 3)

def low_movement():
    return round(random.uniform(0.04, 0.12), 3)

def make_name(used_names):
    for _ in range(200):
        word = random.choice(STILLNESS_WORDS)
        mod = random.choice(MODIFIERS)
        name = f"{word} {mod}"
        if name not in used_names:
            return name
    # fallback with index
    return f"STATIC NODE {len(used_names)+1}"

def make_preset(name, engine1, engine2, coupling_type):
    mv1 = low_movement()
    mv2 = low_movement()
    dna_movement = round((mv1 + mv2) / 2, 3)

    preset = {
        "name": name,
        "version": "1.0",
        "mood": "Entangled",
        "engines": [engine1, engine2],
        "parameters": {
            engine1: {
                "macro_character": rand(),
                "macro_movement": mv1,
                "macro_coupling": rand(),
                "macro_space": rand()
            },
            engine2: {
                "macro_character": rand(),
                "macro_movement": mv2,
                "macro_coupling": rand(),
                "macro_space": rand()
            }
        },
        "coupling": {
            "type": coupling_type,
            "source": engine1,
            "target": engine2,
            "amount": rand(0.2, 0.95)
        },
        "dna": {
            "brightness": rand(),
            "warmth": rand(),
            "movement": dna_movement,
            "density": rand(),
            "space": rand(),
            "aggression": rand()
        },
        "macros": {
            "CHARACTER": rand(),
            "MOVEMENT": dna_movement,
            "COUPLING": rand(),
            "SPACE": rand()
        },
        "tags": ["entangled", "stillness", name.split()[0].lower()]
    }
    return preset

def main():
    os.makedirs(OUTPUT_DIR, exist_ok=True)

    # Build 80 engine pairs — cycle through coupling types evenly
    # Use varied pairs: sequential, skip-2, skip-5, cross-family
    pairs = []
    n = len(ENGINES)
    steps = [1, 2, 3, 5, 7, 11, 13, 17]
    i = 0
    step_idx = 0
    seen_pairs = set()
    attempts = 0
    while len(pairs) < 80 and attempts < 5000:
        attempts += 1
        step = steps[step_idx % len(steps)]
        e1 = ENGINES[i % n]
        e2 = ENGINES[(i + step) % n]
        if e1 != e2:
            key = tuple(sorted([e1, e2]))
            # Allow duplicate pairs (80 presets, 34 engines — some repeats fine)
            pairs.append((e1, e2))
        i += 1
        step_idx += 1
    pairs = pairs[:80]

    used_names = set()
    written = 0
    skipped = 0

    for idx, (e1, e2) in enumerate(pairs):
        coupling_type = COUPLING_TYPES[idx % len(COUPLING_TYPES)]
        name = make_name(used_names)
        used_names.add(name)

        safe_name = name.replace(" ", "_").upper()
        filename = f"{safe_name}.xometa"
        filepath = os.path.join(OUTPUT_DIR, filename)

        if os.path.exists(filepath):
            skipped += 1
            continue

        preset = make_preset(name, e1, e2, coupling_type)

        with open(filepath, "w") as f:
            json.dump(preset, f, indent=2)
        written += 1

    print(f"Done. Written: {written} | Skipped (already existed): {skipped}")
    print(f"Output directory: {OUTPUT_DIR}")

if __name__ == "__main__":
    main()
