#!/usr/bin/env python3
"""
Generate 80 Foundation presets filling critical DNA gaps:
- Part 1: 40 presets with movement >= 0.88
- Part 2: 40 presets with aggression >= 0.88
"""

import json
import os
import random

OUTPUT_DIR = "Presets/Foundation"

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

MOVEMENT_NAMES = [
    "KINETIC BASE", "MOVING GROUND", "ACTIVE FLOOR", "PULSE FOUNDATION",
    "RHYTHMIC BASE", "KINETIC FLOOR", "MOVING BASE", "ACTIVE GROUND",
    "PULSE FLOOR", "RHYTHMIC GROUND", "KINETIC FOUNDATION", "MOVING FLOOR",
    "ACTIVE BASE", "PULSE GROUND", "RHYTHMIC FOUNDATION", "KINETIC GROUND",
    "MOVING FOUNDATION", "ACTIVE FOUNDATION", "PULSE BASE", "RHYTHMIC FLOOR",
    "SURGE BASE", "FLOW GROUND", "DRIVE FLOOR", "MOTION FOUNDATION",
    "CURRENT BASE", "WAVE GROUND", "STREAM FLOOR", "TORRENT FOUNDATION",
    "TIDE BASE", "FLUX GROUND", "DRIFT FLOOR", "RIPPLE FOUNDATION",
    "SWELL BASE", "RUSH GROUND", "THRUST FLOOR", "PROPEL FOUNDATION",
    "PROPEL BASE", "THRUST GROUND", "RUSH FLOOR", "SWELL FOUNDATION"
]

AGGRESSION_NAMES = [
    "BRUTAL BASE", "AGGRESSIVE GROUND", "SAVAGE FLOOR", "HARSH FOUNDATION",
    "VIOLENT BASE", "BRUTAL FLOOR", "AGGRESSIVE BASE", "SAVAGE GROUND",
    "HARSH BASE", "VIOLENT GROUND", "BRUTAL FOUNDATION", "AGGRESSIVE FLOOR",
    "SAVAGE FOUNDATION", "HARSH GROUND", "VIOLENT FLOOR", "BRUTAL GROUND",
    "AGGRESSIVE FOUNDATION", "SAVAGE BASE", "HARSH FLOOR", "VIOLENT FOUNDATION",
    "RAGE BASE", "FURY GROUND", "WRATH FLOOR", "HAVOC FOUNDATION",
    "CHAOS BASE", "MAYHEM GROUND", "CARNAGE FLOOR", "TERROR FOUNDATION",
    "VENOM BASE", "TOXIN GROUND", "ACID FLOOR", "CORROSIVE FOUNDATION",
    "SHRED BASE", "GRIND GROUND", "CRUSH FLOOR", "OBLITERATE FOUNDATION",
    "OBLITERATE BASE", "CRUSH GROUND", "GRIND FLOOR", "SHRED FOUNDATION"
]


def rand_val():
    return round(random.uniform(0.2, 0.8), 3)


def extreme_low():
    return round(random.uniform(0.02, 0.15), 3)


def extreme_high():
    return round(random.uniform(0.85, 0.98), 3)


def movement_val():
    return round(random.uniform(0.88, 0.98), 3)


def aggression_val():
    return round(random.uniform(0.88, 0.98), 3)


def make_preset_movement(name, engines_pair, coupling_type, idx):
    e1, e2 = engines_pair
    mv1 = movement_val()
    mv2 = movement_val()
    dna_movement = round((mv1 + mv2) / 2 + random.uniform(-0.02, 0.02), 3)
    dna_movement = min(0.98, max(0.88, dna_movement))

    # At least 1 other extreme DNA dimension
    extra_extreme_choice = random.choice(["brightness", "warmth", "density", "space", "aggression"])
    extra_extreme_direction = random.choice(["low", "high"])

    brightness = rand_val()
    warmth = rand_val()
    density = rand_val()
    space = rand_val()
    aggression = rand_val()

    if extra_extreme_choice == "brightness":
        brightness = extreme_low() if extra_extreme_direction == "low" else extreme_high()
    elif extra_extreme_choice == "warmth":
        warmth = extreme_low() if extra_extreme_direction == "low" else extreme_high()
    elif extra_extreme_choice == "density":
        density = extreme_low() if extra_extreme_direction == "low" else extreme_high()
    elif extra_extreme_choice == "space":
        space = extreme_low() if extra_extreme_direction == "low" else extreme_high()
    elif extra_extreme_choice == "aggression":
        aggression = extreme_low() if extra_extreme_direction == "low" else extreme_high()

    coupling_amount = round(random.uniform(0.4, 0.9), 3)

    return {
        "name": name,
        "version": "1.0",
        "mood": "Foundation",
        "engines": [e1, e2],
        "parameters": {
            e1: {
                "macro_character": round(random.uniform(0.3, 0.8), 3),
                "macro_movement": mv1,
                "macro_coupling": round(random.uniform(0.3, 0.8), 3),
                "macro_space": round(random.uniform(0.2, 0.7), 3)
            },
            e2: {
                "macro_character": round(random.uniform(0.3, 0.8), 3),
                "macro_movement": mv2,
                "macro_coupling": round(random.uniform(0.3, 0.8), 3),
                "macro_space": round(random.uniform(0.2, 0.7), 3)
            }
        },
        "coupling": {
            "type": coupling_type,
            "source": e1,
            "target": e2,
            "amount": coupling_amount
        },
        "dna": {
            "brightness": brightness,
            "warmth": warmth,
            "movement": dna_movement,
            "density": density,
            "space": space,
            "aggression": aggression
        },
        "macros": {
            "CHARACTER": round(random.uniform(0.3, 0.8), 3),
            "MOVEMENT": dna_movement,
            "COUPLING": coupling_amount,
            "SPACE": space
        },
        "tags": ["foundation", "kinetic", "moving"]
    }


def make_preset_aggression(name, engines_pair, coupling_type, idx):
    e1, e2 = engines_pair
    ag1 = aggression_val()
    ag2 = aggression_val()
    dna_aggression = round((ag1 + ag2) / 2 + random.uniform(-0.02, 0.02), 3)
    dna_aggression = min(0.98, max(0.88, dna_aggression))

    # At least 1 other extreme DNA dimension
    extra_extreme_choice = random.choice(["brightness", "warmth", "movement", "density", "space"])
    extra_extreme_direction = random.choice(["low", "high"])

    brightness = rand_val()
    warmth = rand_val()
    movement = rand_val()
    density = rand_val()
    space = rand_val()

    if extra_extreme_choice == "brightness":
        brightness = extreme_low() if extra_extreme_direction == "low" else extreme_high()
    elif extra_extreme_choice == "warmth":
        warmth = extreme_low() if extra_extreme_direction == "low" else extreme_high()
    elif extra_extreme_choice == "movement":
        movement = extreme_low() if extra_extreme_direction == "low" else extreme_high()
    elif extra_extreme_choice == "density":
        density = extreme_low() if extra_extreme_direction == "low" else extreme_high()
    elif extra_extreme_choice == "space":
        space = extreme_low() if extra_extreme_direction == "low" else extreme_high()

    coupling_amount = round(random.uniform(0.4, 0.9), 3)

    return {
        "name": name,
        "version": "1.0",
        "mood": "Foundation",
        "engines": [e1, e2],
        "parameters": {
            e1: {
                "macro_character": round(random.uniform(0.3, 0.8), 3),
                "macro_movement": round(random.uniform(0.3, 0.7), 3),
                "macro_coupling": round(random.uniform(0.3, 0.8), 3),
                "macro_space": round(random.uniform(0.2, 0.7), 3)
            },
            e2: {
                "macro_character": round(random.uniform(0.3, 0.8), 3),
                "macro_movement": round(random.uniform(0.3, 0.7), 3),
                "macro_coupling": round(random.uniform(0.3, 0.8), 3),
                "macro_space": round(random.uniform(0.2, 0.7), 3)
            }
        },
        "coupling": {
            "type": coupling_type,
            "source": e1,
            "target": e2,
            "amount": coupling_amount
        },
        "dna": {
            "brightness": brightness,
            "warmth": warmth,
            "movement": movement,
            "density": density,
            "space": space,
            "aggression": dna_aggression
        },
        "macros": {
            "CHARACTER": round(random.uniform(0.3, 0.8), 3),
            "MOVEMENT": movement,
            "COUPLING": coupling_amount,
            "SPACE": space
        },
        "tags": ["foundation", "aggressive", "harsh"]
    }


def filename_for(name):
    safe = name.replace(" ", "_").replace("/", "-")
    return f"{safe}.xometa"


def main():
    random.seed(42)
    os.makedirs(OUTPUT_DIR, exist_ok=True)

    written = 0
    skipped = 0

    # Build engine pairs — cycle through all 34 engines widely
    def engine_pairs(count):
        pairs = []
        engines = ENGINES[:]
        for i in range(count):
            # Pick two distinct engines, rotating through the list
            e1 = engines[i % len(engines)]
            e2 = engines[(i * 7 + 13) % len(engines)]
            if e1 == e2:
                e2 = engines[(i * 7 + 14) % len(engines)]
            pairs.append((e1, e2))
        return pairs

    movement_pairs = engine_pairs(40)
    aggression_pairs = engine_pairs(40)

    # Part 1: Movement XHIGH
    for i, name in enumerate(MOVEMENT_NAMES):
        coupling_type = COUPLING_TYPES[i % len(COUPLING_TYPES)]
        engines_pair = movement_pairs[i]
        preset = make_preset_movement(name, engines_pair, coupling_type, i)
        fname = filename_for(name)
        fpath = os.path.join(OUTPUT_DIR, fname)
        if os.path.exists(fpath):
            skipped += 1
            continue
        with open(fpath, "w") as f:
            json.dump(preset, f, indent=2)
        written += 1

    # Part 2: Aggression XHIGH
    for i, name in enumerate(AGGRESSION_NAMES):
        coupling_type = COUPLING_TYPES[(i + 5) % len(COUPLING_TYPES)]
        engines_pair = aggression_pairs[i]
        preset = make_preset_aggression(name, engines_pair, coupling_type, i)
        fname = filename_for(name)
        fpath = os.path.join(OUTPUT_DIR, fname)
        if os.path.exists(fpath):
            skipped += 1
            continue
        with open(fpath, "w") as f:
            json.dump(preset, f, indent=2)
        written += 1

    print(f"Done. Written: {written} | Skipped (already exist): {skipped}")


if __name__ == "__main__":
    main()
