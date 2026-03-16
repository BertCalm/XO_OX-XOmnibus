#!/usr/bin/env python3
"""
Generate 80 .xometa presets for Entangled mood with extreme aggression values.
- First 40: aggression 0.02-0.10 (XLOW, gentle/tender/yielding)
- Last 40: aggression 0.90-0.99 (XHIGH, savage/brutal/obliterating)
"""

import json
import os
import random

OUTPUT_DIR = "Presets/Entangled"

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

# XLOW gentle names
XLOW_NAMES = [
    "TENDER KNOT", "SOFT TETHER", "YIELDING MESH", "GENTLE BOND", "WHISPER LINK",
    "QUIET WEAVE", "FRAGILE HOLD", "DELICATE BRAID", "FEATHER TIE", "BREATH WRAP",
    "GOSSAMER KNOT", "SILKEN THREAD", "TENDER LACE", "SOFT ENTANGLE", "CALM CLASP",
    "VELVET BIND", "MORNING DEW WEB", "CLOUD TETHER", "PETAL WEAVE", "MURMUR LINK",
    "HUSH KNOT", "TENDER CLASP", "SILK MESH", "GENTLE COIL", "SOFT SPIRAL",
    "LULLABY KNOT", "DOVE TETHER", "TENDER PULSE", "FEATHER BOND", "QUIET MESH",
    "WISP WEAVE", "TENDER FOLD", "SOFT CLASP", "DREAM LINK", "GENTLE SPIRAL",
    "TENDER THREAD", "SOFT COIL", "WHISPER WEAVE", "CALM TETHER", "GENTLE WRAP"
]

# XHIGH savage names
XHIGH_NAMES = [
    "OBLITERATOR", "SAVAGE KNOT", "BRUTAL TETHER", "ANNIHILATOR", "DESTROY LINK",
    "WRATH FIELD", "MASSACRE MESH", "VIOLENT BOND", "FURY WEAVE", "DEVASTATOR",
    "CHAOS BIND", "RUIN CLASP", "SAVAGE COIL", "BRUTAL FOLD", "ANNIHILATE MESH",
    "WRATH SPIRAL", "DESTROY WEAVE", "SAVAGE THREAD", "BRUTAL CLASP", "OBLITERATE BOND",
    "RAGE TETHER", "MASSACRE KNOT", "VIOLENT MESH", "FURY LINK", "SAVAGE WRAP",
    "BRUTAL PULSE", "ANNIHILATE KNOT", "CHAOS TETHER", "WRATH WEAVE", "DESTROY COIL",
    "SAVAGE SPIRAL", "BRUTAL BIND", "MASSACRE CLASP", "VIOLENT FOLD", "FURY BOND",
    "WRATH KNOT", "OBLITERATE MESH", "SAVAGE BIND", "BRUTAL SPIRAL", "ANNIHILATE LINK"
]

# XLOW tags
XLOW_TAGS = ["entangled", "gentle", "tender", "yielding", "soft", "delicate", "quiet", "calm"]
# XHIGH tags
XHIGH_TAGS = ["entangled", "savage", "brutal", "aggressive", "violent", "obliterating", "fury", "chaos"]


def extreme_dna_value(exclude_aggression=True):
    """Return a DNA value in an extreme zone (<=0.15 or >=0.85)."""
    if random.random() < 0.5:
        return round(random.uniform(0.02, 0.15), 3)
    else:
        return round(random.uniform(0.85, 0.99), 3)


def random_dna_value():
    return round(random.uniform(0.1, 0.9), 3)


def make_preset(name, aggression, tags, engine_pair, coupling_type, preset_index):
    eng1, eng2 = engine_pair

    # Ensure at least 1 other DNA dimension is extreme
    dna_keys = ["brightness", "warmth", "movement", "density", "space"]
    # Pick 1-3 extreme dims
    num_extreme = random.randint(1, 3)
    extreme_keys = random.sample(dna_keys, num_extreme)

    dna = {}
    for key in dna_keys:
        if key in extreme_keys:
            dna[key] = extreme_dna_value()
        else:
            dna[key] = random_dna_value()
    dna["aggression"] = round(aggression, 3)

    def rand_macro():
        return round(random.uniform(0.1, 0.9), 3)

    parameters = {
        eng1: {
            "macro_character": rand_macro(),
            "macro_movement": rand_macro(),
            "macro_coupling": rand_macro(),
            "macro_space": rand_macro()
        },
        eng2: {
            "macro_character": rand_macro(),
            "macro_movement": rand_macro(),
            "macro_coupling": rand_macro(),
            "macro_space": rand_macro()
        }
    }

    coupling_amount = round(random.uniform(0.2, 0.95), 3)

    macros = {
        "CHARACTER": round(random.uniform(0.1, 0.9), 3),
        "MOVEMENT": round(random.uniform(0.1, 0.9), 3),
        "COUPLING": round(random.uniform(0.1, 0.9), 3),
        "SPACE": round(random.uniform(0.1, 0.9), 3)
    }

    return {
        "name": name,
        "version": "1.0",
        "mood": "Entangled",
        "engines": [eng1, eng2],
        "parameters": parameters,
        "coupling": {
            "type": coupling_type,
            "source": eng1,
            "target": eng2,
            "amount": coupling_amount
        },
        "dna": dna,
        "macros": macros,
        "tags": tags
    }


def main():
    random.seed(42)
    os.makedirs(OUTPUT_DIR, exist_ok=True)

    # Build engine pairs for 80 presets — cycle through pairs widely
    # Generate 80 unique pairs by cycling
    engine_pairs = []
    engines_copy = ENGINES[:]
    random.shuffle(engines_copy)
    # Build 80 pairs with wide variety
    used = set()
    all_pairs = []
    for i in range(len(engines_copy)):
        for j in range(i + 1, len(engines_copy)):
            all_pairs.append((engines_copy[i], engines_copy[j]))
    random.shuffle(all_pairs)
    # Take 80 from shuffled pairs
    selected_pairs = all_pairs[:80]

    # Distribute coupling types evenly across 80 presets
    coupling_assignments = []
    for i in range(80):
        coupling_assignments.append(COUPLING_TYPES[i % len(COUPLING_TYPES)])
    random.shuffle(coupling_assignments)

    written = 0
    skipped = 0

    # First 40: XLOW aggression
    for i in range(40):
        name = XLOW_NAMES[i]
        aggression = round(random.uniform(0.02, 0.10), 3)
        tags = random.sample(XLOW_TAGS, min(4, len(XLOW_TAGS)))
        engine_pair = selected_pairs[i]
        coupling_type = coupling_assignments[i]
        preset = make_preset(name, aggression, tags, engine_pair, coupling_type, i)

        filename = name.replace(" ", "_").upper() + ".xometa"
        filepath = os.path.join(OUTPUT_DIR, filename)

        if os.path.exists(filepath):
            print(f"  SKIP (exists): {filename}")
            skipped += 1
            continue

        with open(filepath, "w") as f:
            json.dump(preset, f, indent=2)
        print(f"  WROTE (XLOW): {filename}  aggression={aggression}")
        written += 1

    # Last 40: XHIGH aggression
    for i in range(40):
        name = XHIGH_NAMES[i]
        aggression = round(random.uniform(0.90, 0.99), 3)
        tags = random.sample(XHIGH_TAGS, min(4, len(XHIGH_TAGS)))
        engine_pair = selected_pairs[40 + i]
        coupling_type = coupling_assignments[40 + i]
        preset = make_preset(name, aggression, tags, engine_pair, coupling_type, 40 + i)

        filename = name.replace(" ", "_").upper() + ".xometa"
        filepath = os.path.join(OUTPUT_DIR, filename)

        if os.path.exists(filepath):
            print(f"  SKIP (exists): {filename}")
            skipped += 1
            continue

        with open(filepath, "w") as f:
            json.dump(preset, f, indent=2)
        print(f"  WROTE (XHIGH): {filename}  aggression={aggression}")
        written += 1

    print(f"\nDone. Written: {written}  Skipped: {skipped}  Total: {written + skipped}")


if __name__ == "__main__":
    main()
