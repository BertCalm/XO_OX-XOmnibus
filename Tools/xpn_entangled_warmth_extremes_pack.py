#!/usr/bin/env python3
"""
xpn_entangled_warmth_extremes_pack.py
Generates 80 .xometa presets for the Entangled mood with extreme warmth values.
  - First 40: warmth 0.02-0.12 (XLOW, cold/icy/clinical)
  - Last  40: warmth 0.88-0.98 (XHIGH, hot/molten/furnace)
Output: Presets/Entangled/
"""

import json
import os
import random

random.seed(42)

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

# --- Cold preset names (40) ---
COLD_NAMES = [
    "ICE LATTICE", "CRYO BOND", "FROZEN SIGNAL", "CLINICAL MESH", "STEEL THREAD",
    "ARCTIC LINK", "POLAR WEAVE", "GLACIAL SYNC", "FROST CIRCUIT", "VOID CURRENT",
    "ZERO PULSE", "CRYSTAL NODE", "COLD ENTANGLE", "CRYOGENIC TIE", "BRITTLE WIRE",
    "PERMAFROST", "TUNDRA KNOT", "WINTER CORE", "STERILE FOLD", "NITROGEN DRIFT",
    "SHARD COUPLE", "BLIZZARD NET", "DEEP FREEZE", "HARD VACUUM", "CERAMIC BOND",
    "ALUMINIUM TRACE", "SURGICAL MESH", "PALE SIGNAL", "WHITE NOISE TIE", "ICE VEIN",
    "PLATINUM LINK", "GLASS THREAD", "BARE CIRCUIT", "COLD LATTICE", "STERILE SYNC",
    "LIQUID HELIUM", "ZERO KELVIN", "CLINICAL KNOT", "FROST ENVELOPE", "EMPTY WIRE",
]

# --- Hot preset names (40) ---
HOT_NAMES = [
    "FURNACE KNOT", "MOLTEN CORE", "EMBER BOND", "FORGE HEAT", "THERMAL SURGE",
    "LAVA THREAD", "INFERNO MESH", "SOLAR FLARE TIE", "MAGMA LINK", "COMBUSTION FOLD",
    "INCANDESCENT SYNC", "CINDER COUPLE", "BLAZE CIRCUIT", "RADIANT KNOT", "SCORCHED WEAVE",
    "PLASMA BOND", "KILN SIGNAL", "CRUCIBLE THREAD", "SMELT CURRENT", "VOLCANIC PULSE",
    "FIRE LATTICE", "ASH ENTANGLE", "HEAT ENVELOPE", "COAL CORE", "IGNITION MESH",
    "FUSED SIGNAL", "BOILING SYNC", "SEARING LINK", "PYROCLASTIC", "FORGE WELD",
    "THERMAL KNOT", "OBSIDIAN HEAT", "CHAR BOND", "FUSION WIRE", "MELT COUPLE",
    "CORE TEMPERATURE", "CANDESCENT FOLD", "SLAG THREAD", "HOT LATTICE", "THERMAL BIND",
]


def rnd(lo, hi, decimals=3):
    return round(random.uniform(lo, hi), decimals)


def extreme_other_dna(exclude_key):
    """Return a dict of DNA values where at least one non-warmth dimension is extreme."""
    keys = ["brightness", "movement", "density", "space", "aggression"]
    values = {k: rnd(0.1, 0.9) for k in keys}
    # Force at least one extreme (besides warmth)
    other_keys = [k for k in keys if k != exclude_key]
    extreme_key = random.choice(other_keys)
    if random.random() < 0.5:
        values[extreme_key] = rnd(0.02, 0.15)
    else:
        values[extreme_key] = rnd(0.85, 0.98)
    return values


def make_preset(name, warmth, coupling_type, engine_a, engine_b, tags):
    other_dna = extreme_other_dna("warmth")
    dna = {
        "brightness": other_dna["brightness"],
        "warmth": round(warmth, 3),
        "movement": other_dna["movement"],
        "density": other_dna["density"],
        "space": other_dna["space"],
        "aggression": other_dna["aggression"],
    }

    def engine_params():
        return {
            "macro_character": rnd(0.0, 1.0),
            "macro_movement": rnd(0.0, 1.0),
            "macro_coupling": rnd(0.0, 1.0),
            "macro_space": rnd(0.0, 1.0),
        }

    preset = {
        "name": name,
        "version": "1.0",
        "mood": "Entangled",
        "engines": [engine_a, engine_b],
        "parameters": {
            engine_a: engine_params(),
            engine_b: engine_params(),
        },
        "coupling": {
            "type": coupling_type,
            "source": engine_a,
            "target": engine_b,
            "amount": rnd(0.3, 0.9),
        },
        "dna": dna,
        "macros": {
            "CHARACTER": rnd(0.0, 1.0),
            "MOVEMENT": rnd(0.0, 1.0),
            "COUPLING": rnd(0.0, 1.0),
            "SPACE": rnd(0.0, 1.0),
        },
        "tags": tags,
    }
    return preset


def engine_pairs(n):
    """Generate n varied engine pairs, cycling through all 34 engines."""
    pairs = []
    engines = ENGINES[:]
    random.shuffle(engines)
    used = list(zip(engines, engines[1:] + engines[:1]))  # shift-by-1 pairing
    # Extend if needed
    while len(used) < n:
        random.shuffle(engines)
        used += list(zip(engines, engines[1:] + engines[:1]))
    return used[:n]


def main():
    os.makedirs(OUTPUT_DIR, exist_ok=True)

    cold_pairs = engine_pairs(40)
    hot_pairs = engine_pairs(40)

    # Distribute coupling types evenly across all 80 presets
    coupling_pool = (COUPLING_TYPES * 7)[:80]
    random.shuffle(coupling_pool)

    written = 0
    skipped = 0

    # --- COLD presets (0-39) ---
    for i, (name, (eng_a, eng_b), coupling) in enumerate(
        zip(COLD_NAMES, cold_pairs, coupling_pool[:40])
    ):
        warmth = rnd(0.02, 0.12)
        tags = ["entangled", "cold", "clinical", "icy"]
        preset = make_preset(name, warmth, coupling, eng_a, eng_b, tags)
        filename = name.replace(" ", "_") + ".xometa"
        filepath = os.path.join(OUTPUT_DIR, filename)
        if os.path.exists(filepath):
            print(f"  SKIP (exists): {filename}")
            skipped += 1
            continue
        with open(filepath, "w") as f:
            json.dump(preset, f, indent=2)
        print(f"  WRITE: {filename}  warmth={warmth:.3f}  engines={eng_a}+{eng_b}  coupling={coupling}")
        written += 1

    # --- HOT presets (40-79) ---
    for i, (name, (eng_a, eng_b), coupling) in enumerate(
        zip(HOT_NAMES, hot_pairs, coupling_pool[40:])
    ):
        warmth = rnd(0.88, 0.98)
        tags = ["entangled", "hot", "molten", "furnace"]
        preset = make_preset(name, warmth, coupling, eng_a, eng_b, tags)
        filename = name.replace(" ", "_") + ".xometa"
        filepath = os.path.join(OUTPUT_DIR, filename)
        if os.path.exists(filepath):
            print(f"  SKIP (exists): {filename}")
            skipped += 1
            continue
        with open(filepath, "w") as f:
            json.dump(preset, f, indent=2)
        print(f"  WRITE: {filename}  warmth={warmth:.3f}  engines={eng_a}+{eng_b}  coupling={coupling}")
        written += 1

    print(f"\nDone. Written: {written}  Skipped: {skipped}  Total attempted: {written + skipped}")


if __name__ == "__main__":
    main()
