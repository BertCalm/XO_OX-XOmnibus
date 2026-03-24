#!/usr/bin/env python3
"""
xpn_entangled_warm_dense_pack.py

Generate 80 Entangled presets in the "scorching dense" zone:
  warmth >= 0.87  AND  density >= 0.75

16 pairs × 5 presets each = 80 total.
Saves to Presets/XOlokun/Entangled/. Skips if file already exists.

Pairs:
  OVERDUB×OBLONG, OBESE×ORGANON, ODDOSCAR×OBLONG, OVERDUB×OPAL,
  ORGANON×OBLONG, OVERDUB×ORGANON, OBLONG×OPAL, ODDOSCAR×ORGANON,
  OBESE×OBLONG, OVERDUB×ODDOSCAR, OBLONG×OSPREY, ORGANON×OSPREY,
  OVERDUB×OSPREY, ODDOSCAR×OPAL, OBLONG×OHM, ORGANON×OHM
"""

import json
import os
import random

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUTPUT_DIR = os.path.join(REPO_ROOT, "Presets", "XOlokun", "Entangled")

random.seed(77)

# ---------------------------------------------------------------------------
# Coupling types for warm-dense / thick / saturated pairings
# ---------------------------------------------------------------------------
COUPLING_TYPES = [
    "TIMBRE_BLEND",
    "RESONANCE_SHARE",
    "AMPLITUDE_MOD",
    "ENVELOPE_LINK",
    "HARMONIC_FOLD",
    "FILTER_MOD",
]

# ---------------------------------------------------------------------------
# 16 pairs with 5 evocative name seeds each
# ---------------------------------------------------------------------------
PAIR_NAMES = {
    ("OVERDUB", "OBLONG"): [
        "Tape Slab",
        "Dub Monolith",
        "Pressed Oxide",
        "Sub Concrete",
        "Woolen Delay",
    ],
    ("OBESE", "ORGANON"): [
        "Tectonic Choir",
        "Fat Oracle",
        "Organ Mass",
        "Adipose Drone",
        "Harmonic Tonnage",
    ],
    ("ODDOSCAR", "OBLONG"): [
        "Oscar's Monolith",
        "Odd Slab Bass",
        "Detuned Concrete",
        "Oscar Anchor",
        "Thick Odd Grid",
    ],
    ("OVERDUB", "OPAL"): [
        "Tape Grain",
        "Saturation Dust",
        "Warm Granule",
        "Oxide Scatter",
        "Dub Haze",
    ],
    ("ORGANON", "OBLONG"): [
        "Reed Monolith",
        "Pipe Mass",
        "Organ Slab",
        "Harmonic Block",
        "Bellows Wall",
    ],
    ("OVERDUB", "ORGANON"): [
        "Saturated Pipe",
        "Dub Reed",
        "Tape Harmonium",
        "Oxide Organ",
        "Warm Bellows",
    ],
    ("OBLONG", "OPAL"): [
        "Slab Grain",
        "Concrete Dust",
        "Bob Scatter",
        "Monolith Haze",
        "Dense Granule",
    ],
    ("ODDOSCAR", "ORGANON"): [
        "Oscar Pipe",
        "Odd Reed Wall",
        "Detuned Choir",
        "Oscar Harmonium",
        "Thick Organ Odd",
    ],
    ("OBESE", "OBLONG"): [
        "Double Slab",
        "Sub Monolith",
        "Fat Concrete",
        "Thick Bob Mass",
        "Bass Megalith",
    ],
    ("OVERDUB", "ODDOSCAR"): [
        "Tape Oscar",
        "Dub Odd Bass",
        "Saturated Odd",
        "Oxide Detuned",
        "Warm Odd Loop",
    ],
    ("OBLONG", "OSPREY"): [
        "Slab Drift",
        "Bob Glide",
        "Concrete Wing",
        "Thick Shore",
        "Monolith Tide",
    ],
    ("ORGANON", "OSPREY"): [
        "Pipe Glide",
        "Reed Shore",
        "Organ Drift",
        "Harmonic Wing",
        "Bellows Tide",
    ],
    ("OVERDUB", "OSPREY"): [
        "Tape Shore",
        "Dub Glide",
        "Oxide Drift",
        "Warm Wing",
        "Saturated Tide",
    ],
    ("ODDOSCAR", "OPAL"): [
        "Oscar Scatter",
        "Odd Grain Haze",
        "Detuned Dust",
        "Oscar Granule",
        "Thick Odd Cloud",
    ],
    ("OBLONG", "OHM"): [
        "Bob Commune",
        "Slab Resonance",
        "Concrete Hum",
        "Monolith Om",
        "Dense Ohm Wall",
    ],
    ("ORGANON", "OHM"): [
        "Pipe Commune",
        "Reed Om",
        "Organ Resonance",
        "Harmonic Hum",
        "Bellows Om Wall",
    ],
}


def rng(lo, hi, decimals=2):
    return round(random.uniform(lo, hi), decimals)


def make_preset(name, eng_a, eng_b):
    """Build one warm+dense Entangled .xometa preset."""
    warmth = rng(0.87, 0.97)
    density = rng(0.76, 0.97)
    brightness = rng(0.18, 0.62)      # wide range — unconstrained
    movement = rng(0.20, 0.72)        # wide range — unconstrained
    space = rng(0.18, 0.58)           # lower space = tighter / more dense feel
    aggression = rng(0.25, 0.75)      # wide range — freely varied

    coupling_type = random.choice(COUPLING_TYPES)
    coupling_amount = rng(0.55, 0.92)

    # Per-engine macros derived from DNA with slight divergence
    a_char = rng(max(0.45, warmth - 0.20), min(1.0, warmth + 0.05))
    a_mov  = rng(max(0.15, movement - 0.15), min(1.0, movement + 0.15))
    a_coup = rng(max(0.45, coupling_amount - 0.15), min(1.0, coupling_amount + 0.10))
    a_space = rng(max(0.15, space - 0.15), min(1.0, space + 0.15))

    b_char = rng(max(0.40, warmth - 0.28), min(1.0, warmth - 0.02))
    b_mov  = rng(max(0.12, movement - 0.20), min(1.0, movement + 0.10))
    b_coup = rng(max(0.38, coupling_amount - 0.22), min(1.0, coupling_amount + 0.05))
    b_space = rng(max(0.12, space - 0.20), min(1.0, space + 0.10))

    macro_character = round((a_char + b_char) / 2, 2)
    macro_movement  = round((a_mov  + b_mov)  / 2, 2)
    macro_coupling  = round((a_coup + b_coup) / 2, 2)
    macro_space     = round((a_space + b_space) / 2, 2)

    return {
        "name": name,
        "version": "1.0",
        "mood": "Entangled",
        "engines": [eng_a, eng_b],
        "parameters": {
            eng_a: {
                "macro_character": a_char,
                "macro_movement":  a_mov,
                "macro_coupling":  a_coup,
                "macro_space":     a_space,
            },
            eng_b: {
                "macro_character": b_char,
                "macro_movement":  b_mov,
                "macro_coupling":  b_coup,
                "macro_space":     b_space,
            },
        },
        "coupling": {
            "type":   coupling_type,
            "source": eng_a,
            "target": eng_b,
            "amount": coupling_amount,
        },
        "dna": {
            "brightness": brightness,
            "warmth":     warmth,
            "movement":   movement,
            "density":    density,
            "space":      space,
            "aggression": aggression,
        },
        "macros": {
            "CHARACTER": macro_character,
            "MOVEMENT":  macro_movement,
            "COUPLING":  macro_coupling,
            "SPACE":     macro_space,
        },
        "tags": ["entangled", "warm-dense", "scorching"],
    }


def safe_filename(name):
    return name.replace(" ", "_").replace("/", "-").replace("'", "") + ".xometa"


def main():
    os.makedirs(OUTPUT_DIR, exist_ok=True)

    written = 0
    skipped = 0
    presets_by_pair = {}

    for (eng_a, eng_b), names in PAIR_NAMES.items():
        presets_by_pair[(eng_a, eng_b)] = []
        for name in names:
            preset = make_preset(name, eng_a, eng_b)
            fname = safe_filename(name)
            fpath = os.path.join(OUTPUT_DIR, fname)

            if os.path.exists(fpath):
                print(f"  SKIP  {fname}")
                skipped += 1
            else:
                with open(fpath, "w") as f:
                    json.dump(preset, f, indent=2)
                print(f"  WRITE {fname}  warmth={preset['dna']['warmth']}  density={preset['dna']['density']}")
                written += 1
            presets_by_pair[(eng_a, eng_b)].append(preset)

    print(f"\n{'='*60}")
    print(f"Entangled Warm+Dense Pack — done")
    print(f"  Written : {written}")
    print(f"  Skipped : {skipped}")
    print(f"  Total   : {written + skipped}")
    print(f"  Output  : {OUTPUT_DIR}")
    print()
    print("DNA summary per pair:")
    for (eng_a, eng_b), presets in presets_by_pair.items():
        ws = [p["dna"]["warmth"] for p in presets]
        ds = [p["dna"]["density"] for p in presets]
        print(f"  {eng_a}×{eng_b}: "
              f"warmth [{min(ws):.2f}–{max(ws):.2f}]  "
              f"density [{min(ds):.2f}–{max(ds):.2f}]")


if __name__ == "__main__":
    main()
