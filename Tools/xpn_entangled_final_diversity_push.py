#!/usr/bin/env python3
"""
xpn_entangled_final_diversity_push.py
Generates 120 .xometa Entangled presets with MAXIMUM DNA diversity impact.
24 batches × 5 presets each — one batch per extreme DNA corner combination.
Output: Presets/XOmnibus/Entangled/
"""

import json
import os
import random

# ── Constants ──────────────────────────────────────────────────────────────────
REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUTPUT_DIR = os.path.join(REPO_ROOT, "Presets", "XOmnibus", "Entangled")

XLOW  = (0.04, 0.12)
XHIGH = (0.88, 0.96)
MID   = (0.35, 0.65)

ENGINES = [
    "OddFelix", "OddOscar", "Overdub", "Odyssey", "Oblong", "Obese",
    "Onset", "Overworld", "Opal", "Orbital", "Organon", "Ouroboros",
    "Obsidian", "Overbite", "Origami", "Oracle", "Obscura", "Oceanic",
    "Ocelot", "Optic", "Oblique", "Osprey", "Osteria", "Owlfish",
    "Ohm", "Orphica", "Obbligato", "Ottoni", "Ole", "Ombre",
    "Orca", "Octopus", "Overlap", "Outwit",
]

COUPLING_TYPES = [
    "FREQUENCY_SHIFT", "AMPLITUDE_MOD", "FILTER_MOD", "PITCH_SYNC",
    "TIMBRE_BLEND", "ENVELOPE_LINK", "HARMONIC_FOLD", "CHAOS_INJECT",
    "RESONANCE_SHARE", "SPATIAL_COUPLE", "SPECTRAL_MORPH", "VELOCITY_COUPLE",
]

DNA_DIMS = ["brightness", "warmth", "movement", "density", "space", "aggression"]

# ── 24 corner batches ──────────────────────────────────────────────────────────
# Each entry: (name_prefix, dna_overrides_dict)
# Keys use short codes: B=brightness W=warmth M=movement D=density S=space A=aggression
BATCHES = [
    ("DARK COLD FURY",      {"brightness": XLOW,  "warmth": XLOW,  "aggression": XHIGH}),
    ("BRIGHT HOT MASS",     {"brightness": XHIGH, "warmth": XHIGH, "density": XHIGH}),
    ("DARK VAST KINETIC",   {"brightness": XLOW,  "space": XHIGH,  "movement": XHIGH}),
    ("BRIGHT SPARSE GENTLE",{"brightness": XHIGH, "density": XLOW, "aggression": XLOW}),
    ("HOT DENSE STILL",     {"warmth": XHIGH,     "density": XHIGH,"movement": XLOW}),
    ("COLD INTIMATE FURY",  {"warmth": XLOW,      "space": XLOW,   "aggression": XHIGH}),
    ("DARK HOT DENSE",      {"brightness": XLOW,  "warmth": XHIGH, "density": XHIGH}),
    ("BRIGHT VAST GENTLE",  {"brightness": XHIGH, "space": XHIGH,  "aggression": XLOW}),
    ("KINETIC FURY DENSE",  {"movement": XHIGH,   "aggression": XHIGH, "density": XHIGH}),
    ("STILL GENTLE SPARSE", {"movement": XLOW,    "aggression": XLOW,  "density": XLOW}),
    ("DARK COLD DENSE",     {"brightness": XLOW,  "warmth": XLOW,  "density": XHIGH}),
    ("BRIGHT HOT VAST",     {"brightness": XHIGH, "warmth": XHIGH, "space": XHIGH}),
    ("KINETIC VAST COLD",   {"movement": XHIGH,   "space": XHIGH,  "warmth": XLOW}),
    ("FURY DENSE HOT",      {"aggression": XHIGH, "density": XHIGH,"warmth": XHIGH}),
    ("DARK KINETIC FURY",   {"brightness": XLOW,  "movement": XHIGH,"aggression": XHIGH}),
    ("BRIGHT STILL VAST",   {"brightness": XHIGH, "movement": XLOW, "space": XHIGH}),
    ("COLD SPARSE VAST",    {"warmth": XLOW,      "density": XLOW,  "space": XHIGH}),
    ("HOT FURY KINETIC",    {"warmth": XHIGH,     "aggression": XHIGH,"movement": XHIGH}),
    ("DARK SPARSE GENTLE",  {"brightness": XLOW,  "density": XLOW,  "aggression": XLOW}),
    ("BRIGHT COLD KINETIC", {"brightness": XHIGH, "warmth": XLOW,   "movement": XHIGH}),
    ("INTIMATE FURY DENSE", {"space": XLOW,       "aggression": XHIGH,"density": XHIGH}),
    ("VAST GENTLE SPARSE",  {"space": XHIGH,      "aggression": XLOW, "density": XLOW}),
    ("HOT INTIMATE KINETIC",{"warmth": XHIGH,     "space": XLOW,    "movement": XHIGH}),
    ("DARK HOT GENTLE",     {"brightness": XLOW,  "warmth": XHIGH,  "aggression": XLOW}),
]

assert len(BATCHES) == 24, "Must have exactly 24 batches"

# ── Helpers ────────────────────────────────────────────────────────────────────
def rand_in(rng):
    return round(random.uniform(rng[0], rng[1]), 3)

def build_dna(overrides):
    dna = {dim: rand_in(MID) for dim in DNA_DIMS}
    for dim, rng in overrides.items():
        dna[dim] = rand_in(rng)
    return dna

def engine_pair(used_pairs):
    """Pick a pair of distinct engines not recently used together."""
    for _ in range(100):
        a, b = random.sample(ENGINES, 2)
        key = tuple(sorted([a, b]))
        if key not in used_pairs:
            used_pairs.add(key)
            return a, b
    # fallback: allow repeat
    a, b = random.sample(ENGINES, 2)
    return a, b

def build_preset(name, mood, engine_a, engine_b, dna, coupling_type):
    tags = [
        name.split()[0].lower(),
        engine_a.lower(),
        engine_b.lower(),
        coupling_type.lower().replace("_", "-"),
        "extreme-dna",
        "diversity-push",
    ]
    macro_pool = [
        ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
        ["BRIGHTNESS", "WARMTH", "FURY", "DENSITY"],
        ["TEXTURE", "DEPTH", "CHAOS", "MOTION"],
        ["EDGE", "BODY", "FLUX", "FIELD"],
    ]
    macros = random.choice(macro_pool)
    amount = round(random.uniform(0.55, 0.98), 3)
    return {
        "schema_version": 1,
        "name": name,
        "mood": mood,
        "engines": [engine_a, engine_b],
        "author": "XO_OX Designs",
        "version": "1.0.0",
        "description": f"{engine_a} × {engine_b} — {coupling_type.replace('_',' ').title()} coupling at extreme DNA corner.",
        "tags": tags,
        "macroLabels": macros,
        "couplingIntensity": "Extreme",
        "tempo": None,
        "dna": dna,
        "parameters": {},
        "coupling": {
            "pairs": [
                {
                    "engineA": engine_a,
                    "engineB": engine_b,
                    "type": coupling_type,
                    "amount": amount,
                }
            ]
        },
        "sequencer": None,
    }

# ── Main ───────────────────────────────────────────────────────────────────────
def main():
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    random.seed(42)

    written = 0
    skipped = 0
    used_pairs = set()
    coupling_cycle = list(COUPLING_TYPES)
    random.shuffle(coupling_cycle)
    coupling_idx = 0

    for batch_idx, (name_prefix, overrides) in enumerate(BATCHES):
        for n in range(1, 6):
            preset_name = f"{name_prefix} {n}"
            filename = f"{preset_name}.xometa"
            filepath = os.path.join(OUTPUT_DIR, filename)

            if os.path.exists(filepath):
                skipped += 1
                continue

            engine_a, engine_b = engine_pair(used_pairs)
            dna = build_dna(overrides)
            coupling_type = coupling_cycle[coupling_idx % len(coupling_cycle)]
            coupling_idx += 1

            preset = build_preset(preset_name, "Entangled", engine_a, engine_b, dna, coupling_type)

            with open(filepath, "w") as f:
                json.dump(preset, f, indent=2)
            written += 1

    print(f"Done.")
    print(f"  Written : {written}")
    print(f"  Skipped : {skipped} (already existed)")
    print(f"  Output  : {OUTPUT_DIR}")

    # Confirm a few files
    sample_files = [f for f in os.listdir(OUTPUT_DIR) if f.startswith("DARK COLD FURY")]
    print(f"  Sample batch 1 files: {sorted(sample_files)}")

if __name__ == "__main__":
    main()
