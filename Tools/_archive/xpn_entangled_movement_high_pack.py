#!/usr/bin/env python3
"""
Generate 80 .xometa presets targeting Entangled mood with movement >= 0.88 (XHIGH zone).
Output dir: Presets/Entangled/
"""

import json
import os
import random

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

KINETIC_WORDS = [
    "TURBULENCE", "FRENZY", "AVALANCHE", "TORRENT", "WILDFIRE",
    "KINETIC STORM", "VORTEX", "MAELSTROM", "TEMPEST", "CYCLONE",
    "UPHEAVAL", "SURGE", "ERUPTION", "DETONATION", "IGNITION",
    "FRACTURE", "CASCADE", "DELUGE", "INFERNO", "SHOCKWAVE",
    "BLIZZARD", "TREMOR", "SEISMIC", "RAMPAGE", "WHIPLASH",
    "RIPTIDE", "FLASHPOINT", "THUNDERHEAD", "FIRESTORM", "LANDSLIDE",
    "OVERDRIVE", "BREAKNECK", "SPIRAL", "FRANTIC PULSE", "CHAOS BLOOM",
    "ELECTRIC FRENZY", "KINETIC CORE", "VELOCITY BURST", "ATOMIC DRIFT",
    "ENTROPY SPIKE", "FEEDBACK STORM", "STATIC SURGE", "MELTDOWN",
    "CRITICAL MASS", "NUCLEAR DRIFT", "HYPERDRIVE", "OVERDRIVE CORE",
    "WARP SURGE", "SONIC SHOCKWAVE", "PULSE STORM", "RESONANT FURY",
    "HARMONIC ERUPTION", "SPECTRAL BLAST", "FREQUENCY RIOT",
    "AMPLITUDE RAMPAGE", "FILTER FURY", "PITCH SPIRAL", "TIMBRE CHAOS",
    "ENVELOPE FRENZY", "HARMONIC FRACTURE", "RESONANCE QUAKE",
    "SPATIAL STORM", "SPECTRAL VORTEX", "VELOCITY SURGE",
    "DARK TURBULENCE", "DEEP FRENZY", "VOID AVALANCHE", "PRIMAL TORRENT",
    "RAW WILDFIRE", "PURE STORM", "SIGNAL FRENZY", "NOISE SURGE",
    "GLITCH CASCADE", "DIGITAL TEMPEST", "ANALOG ERUPTION",
    "MODULAR CHAOS", "PATCH STORM", "FEEDBACK LOOP FURY",
    "OSCILLATOR RIOT", "FILTER MELTDOWN"
]

TAGS_POOL = [
    ["entangled", "kinetic", "turbulent"],
    ["entangled", "frantic", "extreme"],
    ["entangled", "chaotic", "high-energy"],
    ["entangled", "violent", "surge"],
    ["entangled", "explosive", "unstable"],
    ["entangled", "wild", "movement"],
    ["entangled", "storm", "vortex"],
    ["entangled", "rupture", "intensity"],
]


def rnd(lo=0.0, hi=1.0):
    return round(random.uniform(lo, hi), 3)


def rnd_movement():
    return round(random.uniform(0.88, 0.98), 3)


def make_preset(name, engine1, engine2, coupling_type):
    mv1 = rnd_movement()
    mv2 = rnd_movement()
    dna_movement = round((mv1 + mv2) / 2, 3)

    return {
        "name": name,
        "version": "1.0",
        "mood": "Entangled",
        "engines": [engine1, engine2],
        "parameters": {
            engine1: {
                "macro_character": rnd(),
                "macro_movement": mv1,
                "macro_coupling": rnd(),
                "macro_space": rnd()
            },
            engine2: {
                "macro_character": rnd(),
                "macro_movement": mv2,
                "macro_coupling": rnd(),
                "macro_space": rnd()
            }
        },
        "coupling": {
            "type": coupling_type,
            "source": engine1,
            "target": engine2,
            "amount": rnd(0.5, 1.0)
        },
        "dna": {
            "brightness": rnd(),
            "warmth": rnd(),
            "movement": dna_movement,
            "density": rnd(),
            "space": rnd(),
            "aggression": rnd(0.5, 1.0)
        },
        "macros": {
            "CHARACTER": rnd(),
            "MOVEMENT": dna_movement,
            "COUPLING": rnd(),
            "SPACE": rnd()
        },
        "tags": random.choice(TAGS_POOL)
    }


def main():
    repo_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    output_dir = os.path.join(repo_root, "Presets", "Entangled")
    os.makedirs(output_dir, exist_ok=True)

    random.seed(42)

    # Ensure all 34 engines appear and all 12 coupling types are used
    # Build 80 pairs: cycle through coupling types, spread engine pairs widely
    engine_pairs = []
    engines_copy = ENGINES[:]
    random.shuffle(engines_copy)

    # Generate diverse pairs covering all engines at least twice
    used_pairs = set()
    attempts = 0
    while len(engine_pairs) < 80 and attempts < 10000:
        attempts += 1
        e1, e2 = random.sample(ENGINES, 2)
        pair_key = tuple(sorted([e1, e2]))
        if pair_key not in used_pairs:
            used_pairs.add(pair_key)
            engine_pairs.append((e1, e2))

    # If we still need more (unlikely with 34 engines = 561 possible pairs), allow repeats
    while len(engine_pairs) < 80:
        e1, e2 = random.sample(ENGINES, 2)
        engine_pairs.append((e1, e2))

    engine_pairs = engine_pairs[:80]

    # Cycle coupling types evenly across 80 presets
    coupling_assignments = [COUPLING_TYPES[i % len(COUPLING_TYPES)] for i in range(80)]
    random.shuffle(coupling_assignments)

    # Use kinetic names; deduplicate with suffix if needed
    names_used = set()
    name_list = []
    name_pool = KINETIC_WORDS[:] + [f"{w} {i+2}" for i, w in enumerate(KINETIC_WORDS)]
    random.shuffle(name_pool)
    for n in name_pool:
        if n not in names_used:
            names_used.add(n)
            name_list.append(n)
        if len(name_list) >= 80:
            break

    written = 0
    skipped = 0

    for i in range(80):
        name = name_list[i]
        e1, e2 = engine_pairs[i]
        coupling = coupling_assignments[i]
        preset = make_preset(name, e1, e2, coupling)

        safe_name = name.replace(" ", "_").replace("/", "-")
        filename = f"{safe_name}.xometa"
        filepath = os.path.join(output_dir, filename)

        if os.path.exists(filepath):
            print(f"  SKIP (exists): {filename}")
            skipped += 1
            continue

        with open(filepath, "w") as f:
            json.dump(preset, f, indent=2)
        written += 1
        print(f"  WROTE: {filename}  [{e1} + {e2}] {coupling}")

    print(f"\nDone. Written: {written}  Skipped: {skipped}  Total attempted: 80")


if __name__ == "__main__":
    main()
