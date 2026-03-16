#!/usr/bin/env python3
"""
xpn_all_mood_extreme_pole_fill.py

Generates 120 presets (≈17-18 per mood × 7 moods) targeting statistical
center-huggers by pushing ALL-XLOW and ALL-XHIGH poles simultaneously.

Per mood:
  - 6 ALL-HIGH presets  (5 dims XHIGH, 1 dim mid)
  - 6 ALL-LOW  presets  (5 dims XLOW,  1 dim mid)
  - 5 CROSS    presets  (3 XHIGH + 2 XLOW extremes)

Total: 17 × 7 = 119 → pad Family to 18 for 120 total.
"""

import json
import os
import random
import math

random.seed(42)

BASE = "/Users/joshuacramblet/Documents/GitHub/XO_OX-XOmnibus/Presets/XOmnibus"

DNA_DIMS = ["brightness", "warmth", "movement", "density", "space", "aggression"]

COUPLING_TYPES = [
    "FREQUENCY_SHIFT", "AMPLITUDE_MOD", "FILTER_MOD", "PITCH_SYNC",
    "TIMBRE_BLEND", "ENVELOPE_LINK", "HARMONIC_FOLD", "CHAOS_INJECT",
    "RESONANCE_SHARE", "SPATIAL_COUPLE", "SPECTRAL_MORPH", "VELOCITY_COUPLE"
]

MOOD_ENGINES = {
    "Foundation": ["ODDFELIX", "ODDOSCAR", "OBLONG", "OBESE", "OVERBITE", "OVERDUB", "ONSET", "OUROBOROS", "OBSIDIAN"],
    "Atmosphere": ["OPAL", "ORACLE", "OBSCURA", "OCEANIC", "OPTIC", "OBLIQUE", "OSPREY", "ORBITAL", "ORGANON"],
    "Entangled":  None,  # any 2 different engines from full list
    "Prism":      ["OBLIQUE", "ORIGAMI", "ORACLE", "OPTIC", "OCELOT", "OSTERIA", "OWLFISH", "OHM", "OLE", "ORPHICA", "OTTONI"],
    "Flux":       ["OUROBOROS", "ORACLE", "ORGANON", "ORIGAMI", "OPTIC", "OPAL", "OBLONG", "ODYSSEY", "OVERDUB", "OBLIQUE"],
    "Aether":     ["OPAL", "ORACLE", "OBSCURA", "OCEANIC", "ORBITAL", "ORGANON", "OBLIQUE", "ORIGAMI", "ODDFELIX", "OMBRE"],
    "Family":     ["ODDFELIX", "ODDOSCAR", "OBLONG", "OVERDUB", "ODYSSEY", "OPAL", "OHM", "ORPHICA", "OBBLIGATO", "OTTONI", "OLE"],
}

ALL_ENGINES = [
    "ODDFELIX", "ODDOSCAR", "OBLONG", "OBESE", "OVERBITE", "OVERDUB", "ONSET",
    "OUROBOROS", "OBSIDIAN", "OPAL", "ORACLE", "OBSCURA", "OCEANIC", "OPTIC",
    "OBLIQUE", "OSPREY", "ORBITAL", "ORGANON", "ORIGAMI", "OCELOT", "OSTERIA",
    "OWLFISH", "OHM", "OLE", "ORPHICA", "OTTONI", "OBBLIGATO", "ODYSSEY",
    "OMBRE", "OVERLAP", "OUTWIT", "OVERWORLD", "FAT"
]

MOOD_ABBR = {
    "Foundation": "FND",
    "Atmosphere": "ATM",
    "Entangled":  "ENT",
    "Prism":      "PRS",
    "Flux":       "FLX",
    "Aether":     "AET",
    "Family":     "FAM",
}

MOOD_DISPLAY = {
    "Foundation": "Foundation",
    "Atmosphere": "Atmosphere",
    "Entangled":  "Entangled",
    "Prism":      "Prism",
    "Flux":       "Flux",
    "Aether":     "Aether",
    "Family":     "Family",
}


def xhigh():
    return round(random.uniform(0.89, 0.99), 3)

def xlow():
    return round(random.uniform(0.02, 0.11), 3)

def mid():
    return round(random.uniform(0.35, 0.65), 3)

def pick_engines(mood):
    pool = MOOD_ENGINES[mood] if MOOD_ENGINES[mood] else ALL_ENGINES
    engines = random.sample(pool, min(2, len(pool)))
    if len(engines) < 2:
        # supplement from ALL_ENGINES
        extra = [e for e in ALL_ENGINES if e not in engines]
        engines.append(random.choice(extra))
    return engines

def build_dna_allhigh(free_dim):
    """5 dims XHIGH, 1 dim (free_dim) at mid."""
    dna = {}
    for d in DNA_DIMS:
        dna[d] = mid() if d == free_dim else xhigh()
    return dna

def build_dna_alllow(free_dim):
    """5 dims XLOW, 1 dim (free_dim) at mid."""
    dna = {}
    for d in DNA_DIMS:
        dna[d] = mid() if d == free_dim else xlow()
    return dna

def build_dna_cross(high_dims, low_dims):
    """3 XHIGH + 2 XLOW + 1 mid (remaining dim)."""
    dna = {}
    for d in DNA_DIMS:
        if d in high_dims:
            dna[d] = xhigh()
        elif d in low_dims:
            dna[d] = xlow()
        else:
            dna[d] = mid()
    return dna

def engine_params(dna):
    """Map DNA to macro params for one engine."""
    return {
        "macro_character": round((dna["brightness"] + dna["warmth"]) / 2, 3),
        "macro_movement":  round((dna["movement"] + dna["density"]) / 2, 3),
        "macro_coupling":  round((dna["space"] + dna["aggression"]) / 2, 3),
        "macro_space":     round(dna["space"], 3),
    }

def build_preset(name, mood, engines, dna, preset_type):
    avg_hi = (dna["brightness"] + dna["warmth"] + dna["movement"] + dna["aggression"]) / 4
    tags = ["extreme", "pole-fill"]
    if preset_type == "allhigh":
        tags += ["high-energy", "dense"]
    elif preset_type == "alllow":
        tags += ["minimal", "sparse"]
    else:
        tags += ["cross-pole", "contrast"]

    return {
        "name": name,
        "version": "1.0",
        "mood": MOOD_DISPLAY[mood],
        "engines": engines,
        "parameters": {
            engines[0]: engine_params(dna),
            engines[1]: engine_params(dna),
        },
        "coupling": {
            "type": random.choice(COUPLING_TYPES),
            "source": engines[0],
            "target": engines[1],
            "amount": round(random.uniform(0.7, 0.99), 3) if preset_type == "allhigh" else round(random.uniform(0.01, 0.3), 3) if preset_type == "alllow" else round(random.uniform(0.35, 0.75), 3),
        },
        "dna": dna,
        "macros": {
            "CHARACTER": round((dna["brightness"] + dna["warmth"]) / 2, 3),
            "MOVEMENT":  round((dna["movement"] + dna["density"]) / 2, 3),
            "COUPLING":  round((dna["space"] + dna["aggression"]) / 2, 3),
            "SPACE":     round(dna["space"], 3),
        },
        "tags": tags,
    }


def generate_mood_presets(mood, count_extra=0):
    abbr = MOOD_ABBR[mood]
    presets = []

    # 6 ALL-HIGH presets — cycle through dims as the free (mid) dim
    for i, free_dim in enumerate(DNA_DIMS):
        engines = pick_engines(mood)
        dna = build_dna_allhigh(free_dim)
        name = f"ALLHI {abbr} {i+1:02d}"
        filename = f"_ALLHI_{abbr}_{i+1:02d}.xometa"
        presets.append((filename, build_preset(name, mood, engines, dna, "allhigh")))

    # 6 ALL-LOW presets — cycle through dims as the free (mid) dim
    for i, free_dim in enumerate(DNA_DIMS):
        engines = pick_engines(mood)
        dna = build_dna_alllow(free_dim)
        name = f"ALLLO {abbr} {i+1:02d}"
        filename = f"_ALLLO_{abbr}_{i+1:02d}.xometa"
        presets.append((filename, build_preset(name, mood, engines, dna, "alllow")))

    # CROSS presets — 5 base, +extra if needed
    cross_count = 5 + count_extra
    # Cycle through cross patterns: pick 3 high dims + 2 low dims from DNA_DIMS
    cross_patterns = [
        (["brightness","warmth","movement"], ["density","aggression"]),
        (["brightness","density","space"],   ["warmth","aggression"]),
        (["movement","density","aggression"],["brightness","space"]),
        (["warmth","space","aggression"],    ["movement","density"]),
        (["brightness","movement","aggression"],["warmth","space"]),
        (["density","space","warmth"],       ["brightness","movement"]),
    ]
    for i in range(cross_count):
        pattern = cross_patterns[i % len(cross_patterns)]
        engines = pick_engines(mood)
        dna = build_dna_cross(pattern[0], pattern[1])
        name = f"CROSS {abbr} {i+1:02d}"
        filename = f"_CROSS_{abbr}_{i+1:02d}.xometa"
        presets.append((filename, build_preset(name, mood, engines, dna, "cross")))

    return presets


def main():
    moods = ["Foundation", "Atmosphere", "Entangled", "Prism", "Flux", "Aether", "Family"]

    # 6+6+5=17 per mood × 7 = 119; give Family 1 extra cross = 120 total
    extras = {m: 0 for m in moods}
    extras["Family"] = 1

    total = 0
    for mood in moods:
        out_dir = os.path.join(BASE, mood)
        os.makedirs(out_dir, exist_ok=True)

        preset_list = generate_mood_presets(mood, count_extra=extras[mood])
        for filename, data in preset_list:
            path = os.path.join(out_dir, filename)
            with open(path, "w") as f:
                json.dump(data, f, indent=2)
            total += 1

        print(f"  {mood:12s} ({MOOD_ABBR[mood]}): {len(preset_list):3d} presets written → {out_dir}")

    print(f"\nTotal presets written: {total}")


if __name__ == "__main__":
    main()
