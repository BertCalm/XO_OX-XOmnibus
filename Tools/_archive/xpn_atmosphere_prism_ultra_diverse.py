#!/usr/bin/env python3
"""
Generate 80 presets: 40 Atmosphere + 40 Prism
Each preset has 4 extreme DNA dimensions + 2 midrange dimensions.
8 corner combos × 5 presets per mood.

Output: Presets/XOceanus/Atmosphere/ and Presets/XOceanus/Prism/
"""

import json
import os
import random
from datetime import date

random.seed(2026)

REPO_ROOT = os.path.join(os.path.dirname(__file__), '..')
TODAY = str(date.today())

ENGINES = [
    "OddfeliX", "OddOscar", "Overdub", "Odyssey", "Oblong", "Obese",
    "Onset", "Overworld", "Opal", "Orbital", "Organon", "Ouroboros",
    "Obsidian", "Overbite", "Origami", "Oracle", "Obscura", "Oceanic",
    "Ocelot", "Optic", "Oblique", "Osprey", "Osteria", "Owlfish",
    "Ohm", "Orphica", "Obbligato", "Ottoni", "Ole", "Ombre",
    "Orca", "Octopus", "Overlap", "Outwit"
]

COUPLING_TYPES = [
    "None", "Harmonic", "FM", "RingMod", "Envelope", "Filter",
    "Pitch", "Waveshape", "Granular", "Spectral", "Rhythmic", "Chaos"
]

COUPLING_INTENSITIES = ["None", "Low", "Medium", "High", "Extreme"]


def xlow():
    return round(random.uniform(0.04, 0.11), 3)

def xhigh():
    return round(random.uniform(0.89, 0.96), 3)

def mid():
    return round(random.uniform(0.40, 0.60), 3)


# DNA dimension mapping:
# B = brightness, W = warmth, A = aggression, D = density, M = movement, S = space

def build_dna(combo):
    """combo: dict mapping dim_letter -> 'XLOW'|'XHIGH'|'MID'"""
    mapping = {'B': 'brightness', 'W': 'warmth', 'A': 'aggression',
               'D': 'density', 'M': 'movement', 'S': 'space'}
    dims = {v: mid() for v in mapping.values()}
    for letter, zone in combo.items():
        key = mapping[letter]
        if zone == 'XLOW':
            dims[key] = xlow()
        elif zone == 'XHIGH':
            dims[key] = xhigh()
    return dims


# 8 corner combos for Atmosphere
ATM_CORNERS = [
    # (label, combo_dict)
    ("DARK COLD DENSE VIOLENT",   {'B': 'XLOW',  'W': 'XLOW',  'A': 'XHIGH', 'D': 'XHIGH'}),
    ("BRIGHT HOT KINETIC DENSE",  {'B': 'XHIGH', 'W': 'XHIGH', 'M': 'XHIGH', 'S': 'XHIGH'}),
    ("DARK WIDE KINETIC VIOLENT", {'B': 'XLOW',  'S': 'XHIGH', 'M': 'XHIGH', 'A': 'XHIGH'}),
    ("BRIGHT DRY QUIET OPEN",     {'B': 'XHIGH', 'D': 'XLOW',  'A': 'XLOW',  'S': 'XHIGH'}),
    ("HOT DENSE STILL VIOLENT",   {'W': 'XHIGH', 'D': 'XHIGH', 'M': 'XLOW',  'A': 'XHIGH'}),
    ("COLD TIGHT KINETIC DARK",   {'W': 'XLOW',  'S': 'XLOW',  'M': 'XHIGH', 'B': 'XLOW'}),
    ("BRIGHT COLD DENSE VIOLENT", {'B': 'XHIGH', 'W': 'XLOW',  'D': 'XHIGH', 'A': 'XHIGH'}),
    ("DARK HOT WIDE KINETIC",     {'B': 'XLOW',  'W': 'XHIGH', 'S': 'XHIGH', 'M': 'XHIGH'}),
]

# 8 corner combos for Prism
PRI_CORNERS = [
    ("DARK COLD DENSE OPEN",      {'B': 'XLOW',  'W': 'XLOW',  'D': 'XHIGH', 'A': 'XHIGH'}),
    ("BRIGHT HOT KINETIC DENSE",  {'B': 'XHIGH', 'W': 'XHIGH', 'M': 'XHIGH', 'D': 'XHIGH'}),
    ("DARK KINETIC VIOLENT OPEN", {'B': 'XLOW',  'M': 'XHIGH', 'A': 'XHIGH', 'S': 'XHIGH'}),
    ("BRIGHT STILL WIDE TIGHT",   {'B': 'XHIGH', 'M': 'XLOW',  'S': 'XHIGH', 'D': 'XLOW'}),
    ("COLD DRY WIDE VIOLENT",     {'W': 'XLOW',  'D': 'XLOW',  'S': 'XHIGH', 'B': 'XHIGH'}),
    ("HOT OPEN KINETIC BRIGHT",   {'W': 'XHIGH', 'A': 'XHIGH', 'M': 'XHIGH', 'B': 'XHIGH'}),
    ("DARK COLD DRY STILL",       {'B': 'XLOW',  'D': 'XLOW',  'A': 'XLOW',  'M': 'XLOW'}),
    ("HOT TIGHT KINETIC DENSE",   {'W': 'XHIGH', 'S': 'XLOW',  'M': 'XHIGH', 'D': 'XHIGH'}),
]

ATM_SUFFIX = "ATM"
PRI_SUFFIX = "PRI"


def make_preset(name, mood, engine, dna, coupling_type, coupling_intensity):
    tags = []
    for dim, val in dna.items():
        if val < 0.15:
            tags.append(f"x-low-{dim}")
        elif val > 0.85:
            tags.append(f"x-high-{dim}")
    tags = tags[:4]

    return {
        "schema_version": 1,
        "name": name,
        "mood": mood,
        "engines": [engine],
        "author": "XO_OX",
        "version": "1.0.0",
        "description": f"Ultra-diverse {mood} preset: {name}.",
        "tags": tags,
        "macroLabels": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
        "couplingIntensity": coupling_intensity,
        "tempo": None,
        "created": TODAY,
        "legacy": {
            "sourceInstrument": None,
            "sourceCategory": None,
            "sourcePresetName": None
        },
        "parameters": {},
        "coupling": None if coupling_type == "None" else {
            "type": coupling_type,
            "intensity": coupling_intensity,
            "sourceEngine": engine,
            "targetEngine": None
        },
        "sequencer": None,
        "dna": dna
    }


def safe_filename(name):
    return name.replace(' ', '_').replace('/', '-') + ".xometa"


def generate_section(mood, corners, suffix, engine_cycle, coupling_cycle):
    out_dir = os.path.join(REPO_ROOT, 'Presets', 'XOceanus', mood)
    os.makedirs(out_dir, exist_ok=True)

    written = 0
    skipped = 0

    for corner_idx, (label, combo) in enumerate(corners):
        for rep in range(1, 6):
            name = f"{label} {suffix} {rep}"
            filename = safe_filename(name)
            filepath = os.path.join(out_dir, filename)

            if os.path.exists(filepath):
                skipped += 1
                continue

            dna = build_dna(combo)
            engine = engine_cycle[corner_idx * 5 + (rep - 1)]
            coupling_type = coupling_cycle[corner_idx * 5 + (rep - 1)]
            coupling_intensity = random.choice(COUPLING_INTENSITIES)

            preset = make_preset(name, mood, engine, dna, coupling_type, coupling_intensity)

            with open(filepath, 'w') as f:
                json.dump(preset, f, indent=2)
            written += 1

    return written, skipped


def main():
    # Build engine assignment: cycle through all 34 engines for 40 presets each
    # Repeat the list to cover 40 slots, ensuring diversity
    atm_engines = []
    pri_engines = []
    engine_pool = ENGINES[:]  # 34 engines

    # For 40 presets: 34 + 6 repeats. Shuffle each time.
    random.shuffle(engine_pool)
    atm_engines = (engine_pool * 2)[:40]

    engine_pool2 = ENGINES[:]
    random.shuffle(engine_pool2)
    pri_engines = (engine_pool2 * 2)[:40]

    # Build coupling type assignment: cycle 12 types across 40 slots
    coupling_pool = COUPLING_TYPES * 4  # 48 items
    atm_couplings = coupling_pool[:40]
    pri_couplings = coupling_pool[:40]
    # Shuffle for variety
    random.shuffle(atm_couplings)
    random.shuffle(pri_couplings)

    atm_written, atm_skipped = generate_section(
        'Atmosphere', ATM_CORNERS, ATM_SUFFIX, atm_engines, atm_couplings
    )
    pri_written, pri_skipped = generate_section(
        'Prism', PRI_CORNERS, PRI_SUFFIX, pri_engines, pri_couplings
    )

    print(f"Atmosphere: {atm_written} written, {atm_skipped} skipped")
    print(f"Prism:      {pri_written} written, {pri_skipped} skipped")
    print(f"Total:      {atm_written + pri_written} written, {atm_skipped + pri_skipped} skipped")

    # Verify output dirs
    atm_dir = os.path.join(REPO_ROOT, 'Presets', 'XOceanus', 'Atmosphere')
    pri_dir = os.path.join(REPO_ROOT, 'Presets', 'XOceanus', 'Prism')
    atm_count = len([f for f in os.listdir(atm_dir) if f.endswith('.xometa') and not os.path.isdir(os.path.join(atm_dir, f))])
    pri_count = len([f for f in os.listdir(pri_dir) if f.endswith('.xometa') and not os.path.isdir(os.path.join(pri_dir, f))])
    print(f"\nTotal .xometa in Presets/XOceanus/Atmosphere/: {atm_count}")
    print(f"Total .xometa in Presets/XOceanus/Prism/:      {pri_count}")


if __name__ == '__main__':
    main()
