#!/usr/bin/env python3
"""
xpn_family_dna_extremes_pack.py
Generates 60 Family mood .xometa presets filling XLOW gaps:
- 30 presets: warmth DNA 0.02-0.12 (warmth-XLOW), density normal
- 30 presets: density DNA 0.02-0.12 (density-XLOW), warmth normal
"""

import json
import os
import random

OUTPUT_DIR = "Presets/Family"

ENGINES = [
    "ODDFELIX", "ODDOSCAR", "OVERDUB", "ODYSSEY", "OBLONG", "OBESE",
    "ONSET", "OVERWORLD", "OPAL", "ORBITAL", "ORGANON", "OUROBOROS",
    "OBSIDIAN", "OVERBITE", "ORIGAMI", "ORACLE", "OBSCURA", "OCEANIC",
    "OCELOT", "OPTIC", "OBLIQUE", "OSPREY", "OSTERIA", "OWLFISH",
    "OHM", "ORPHICA", "OBBLIGATO", "OTTONI", "OLE", "OMBRE",
    "ORCA", "OCTOPUS", "OVERLAP", "OUTWIT"
]

COUPLING_TYPES = [
    "TIMBRE_BLEND", "PITCH_LOCK", "RHYTHM_SYNC", "HARMONIC_FOLD",
    "SPECTRAL_GATE", "ENVELOPE_SHARE", "FILTER_COUPLE", "AMP_COUPLE",
    "RING_MOD", "FM_COUPLE", "GRANULAR_SYNC", "CHAOS_LOCK"
]

WARMTH_XLOW_NAMES = [
    "COLD REUNION", "WINTER FAMILY", "FROST GATHERING", "ICE BOND",
    "FROZEN KINSHIP", "ARCTIC CLAN", "COLD HEARTH", "CHILL CIRCLE",
    "WINTER NEST", "FROST LINEAGE", "COLD ROOTS", "FROZEN EMBRACE",
    "ARCTIC ORIGIN", "COLD KINDRED", "WINTER TRIBE", "FROST COVENANT",
    "ICE LINEAGE", "COLD ANCESTRY", "FROZEN CIRCLE", "ARCTIC BOND",
    "WINTER KINSHIP", "FROST CLAN", "ICE GATHERING", "COLD ORIGIN",
    "FROZEN ROOTS", "ARCTIC HEARTH", "COLD COVENANT", "WINTER ANCESTRY",
    "FROST EMBRACE", "ICE TRIBE"
]

DENSITY_XLOW_NAMES = [
    "SPARSE CLAN", "OPEN CIRCLE", "BARE GATHERING", "EMPTY NEST",
    "LONE KINDRED", "SPARSE ROOTS", "OPEN LINEAGE", "BARE ORIGIN",
    "EMPTY BOND", "LONE ANCESTRY", "SPARSE TRIBE", "OPEN COVENANT",
    "BARE KINSHIP", "EMPTY HEARTH", "LONE CIRCLE", "SPARSE EMBRACE",
    "OPEN CLAN", "BARE LINEAGE", "EMPTY ANCESTRY", "LONE GATHERING",
    "SPARSE ORIGIN", "OPEN ROOTS", "BARE BOND", "EMPTY KINDRED",
    "LONE TRIBE", "SPARSE COVENANT", "OPEN HEARTH", "BARE CIRCLE",
    "EMPTY LINEAGE", "LONE ORIGIN"
]

random.seed(42)

def rand_extreme_or_normal(force_extreme=False):
    """Return a value that is either extreme (<=0.15 or >=0.85) or normal (0.0-1.0)."""
    if force_extreme:
        if random.random() < 0.5:
            return round(random.uniform(0.0, 0.15), 3)
        else:
            return round(random.uniform(0.85, 1.0), 3)
    return round(random.uniform(0.0, 1.0), 3)

def rand_normal():
    return round(random.uniform(0.0, 1.0), 3)

def rand_xlow():
    return round(random.uniform(0.02, 0.12), 3)

def build_preset(name, engines, coupling_type, dna, tags):
    eng1, eng2 = engines
    macro_keys = ["macro_character", "macro_movement", "macro_coupling", "macro_space"]

    params = {}
    for eng in engines:
        params[eng] = {k: rand_normal() for k in macro_keys}

    coupling = {
        "type": coupling_type,
        "source": eng1,
        "target": eng2,
        "amount": round(random.uniform(0.3, 0.9), 3)
    }

    macros = {
        "CHARACTER": round(random.uniform(0.0, 1.0), 3),
        "MOVEMENT": round(random.uniform(0.0, 1.0), 3),
        "COUPLING": round(random.uniform(0.0, 1.0), 3),
        "SPACE": round(random.uniform(0.0, 1.0), 3)
    }

    return {
        "name": name,
        "version": "1.0",
        "mood": "Family",
        "engines": list(engines),
        "parameters": params,
        "coupling": coupling,
        "dna": dna,
        "macros": macros,
        "tags": tags
    }

def pick_engine_pair(index, all_engines):
    """Pick 2 engines deterministically based on index to spread coverage."""
    n = len(all_engines)
    i = (index * 2) % n
    j = (index * 2 + 1) % n
    if i == j:
        j = (j + 1) % n
    return (all_engines[i], all_engines[j])

def ensure_one_other_extreme(dna, fixed_keys):
    """Guarantee at least one dimension outside fixed_keys is extreme."""
    candidates = [k for k in ["brightness", "movement", "space", "aggression"]
                  if k not in fixed_keys]
    chosen = random.choice(candidates)
    if random.random() < 0.5:
        dna[chosen] = round(random.uniform(0.0, 0.15), 3)
    else:
        dna[chosen] = round(random.uniform(0.85, 1.0), 3)
    return dna

def main():
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    written = 0
    skipped = 0

    # Shuffle engines for variety
    engines = ENGINES[:]
    random.shuffle(engines)

    # --- 30 warmth-XLOW presets ---
    for i in range(30):
        name = WARMTH_XLOW_NAMES[i]
        eng_pair = pick_engine_pair(i, engines)
        coupling_type = COUPLING_TYPES[i % len(COUPLING_TYPES)]

        dna = {
            "brightness": rand_normal(),
            "warmth": rand_xlow(),
            "movement": rand_normal(),
            "density": rand_normal(),
            "space": rand_normal(),
            "aggression": rand_normal()
        }
        dna = ensure_one_other_extreme(dna, fixed_keys={"warmth"})

        tags = ["family", "cold", "xlow-warmth"]
        preset = build_preset(name, eng_pair, coupling_type, dna, tags)

        filename = name.replace(" ", "_").replace("-", "_") + ".xometa"
        filepath = os.path.join(OUTPUT_DIR, filename)

        if os.path.exists(filepath):
            skipped += 1
            continue

        with open(filepath, "w") as f:
            json.dump(preset, f, indent=2)
        written += 1

    # --- 30 density-XLOW presets ---
    for i in range(30):
        name = DENSITY_XLOW_NAMES[i]
        eng_pair = pick_engine_pair(i + 30, engines)
        coupling_type = COUPLING_TYPES[(i + 6) % len(COUPLING_TYPES)]

        dna = {
            "brightness": rand_normal(),
            "warmth": rand_normal(),
            "movement": rand_normal(),
            "density": rand_xlow(),
            "space": rand_normal(),
            "aggression": rand_normal()
        }
        dna = ensure_one_other_extreme(dna, fixed_keys={"density"})

        tags = ["family", "sparse", "xlow-density"]
        preset = build_preset(name, eng_pair, coupling_type, dna, tags)

        filename = name.replace(" ", "_").replace("-", "_") + ".xometa"
        filepath = os.path.join(OUTPUT_DIR, filename)

        if os.path.exists(filepath):
            skipped += 1
            continue

        with open(filepath, "w") as f:
            json.dump(preset, f, indent=2)
        written += 1

    print(f"Done. Written: {written}, Skipped (already exist): {skipped}")
    print(f"Output directory: {OUTPUT_DIR}")

if __name__ == "__main__":
    main()
