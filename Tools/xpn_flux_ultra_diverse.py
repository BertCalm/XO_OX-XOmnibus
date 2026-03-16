#!/usr/bin/env python3
"""
xpn_flux_ultra_diverse.py
Generates 80 Flux presets with FOUR extreme DNA dimensions each.
16 batches of 5 presets covering all extreme DNA combos.
Output: Presets/XOmnibus/Flux/
"""

import json
import os
import random
import math

random.seed(42)

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
REPO_ROOT = os.path.dirname(SCRIPT_DIR)
OUTPUT_DIR = os.path.join(REPO_ROOT, "Presets", "XOmnibus", "Flux")

os.makedirs(OUTPUT_DIR, exist_ok=True)

# All 34 engines
ENGINES = [
    "ODDFELIX", "ODDOSCAR", "OVERDUB", "ODYSSEY", "OBLONG", "OBESE",
    "ONSET", "OVERWORLD", "OPAL", "ORBITAL", "ORGANON", "OUROBOROS",
    "OBSIDIAN", "OVERBITE", "ORIGAMI", "ORACLE", "OBSCURA", "OCEANIC",
    "OCELOT", "OPTIC", "OBLIQUE", "OSPREY", "OSTERIA", "OWLFISH",
    "OHM", "ORPHICA", "OBBLIGATO", "OTTONI", "OLE", "OMBRE",
    "ORCA", "OCTOPUS", "OVERLAP", "OUTWIT"
]

COUPLING_TYPES = [
    "None", "Sync", "Harmonic", "Spectral", "Rhythmic", "Timbral",
    "Spatial", "Dynamic", "Temporal", "Modulation", "Resonant", "Chaotic"
]

# DNA dimension ranges
XLOW_RANGE = (0.04, 0.11)
XHIGH_RANGE = (0.89, 0.96)
MID_RANGE = (0.40, 0.60)

def xlow():
    return round(random.uniform(*XLOW_RANGE), 3)

def xhigh():
    return round(random.uniform(*XHIGH_RANGE), 3)

def mid():
    return round(random.uniform(*MID_RANGE), 3)

# 16 batches: (name_prefix, B, W, A, D, M, S)
# Each batch defines which dims are XLOW/XHIGH/MID
# Dimensions: brightness(B), warmth(W), aggression(A), density(D), movement(M), space(S)
# Format: dict mapping dim -> 'xl', 'xh', or 'mid'

BATCHES = [
    # 1. B-XLOW + W-XLOW + A-XHIGH + D-XHIGH
    {
        "name": "DARK COLD VIOLENT DENSE",
        "dims": {"brightness": "xl", "warmth": "xl", "aggression": "xh", "density": "xh",
                 "movement": "mid", "space": "mid"}
    },
    # 2. B-XHIGH + W-XHIGH + M-XHIGH + D-XHIGH
    {
        "name": "BRIGHT HOT KINETIC DENSE",
        "dims": {"brightness": "xh", "warmth": "xh", "movement": "xh", "density": "xh",
                 "aggression": "mid", "space": "mid"}
    },
    # 3. B-XLOW + S-XHIGH + M-XHIGH + A-XHIGH
    {
        "name": "DARK VAST KINETIC VIOLENT",
        "dims": {"brightness": "xl", "space": "xh", "movement": "xh", "aggression": "xh",
                 "warmth": "mid", "density": "mid"}
    },
    # 4. B-XHIGH + D-XLOW + A-XLOW + S-XHIGH
    {
        "name": "BRIGHT SPARSE GENTLE VAST",
        "dims": {"brightness": "xh", "density": "xl", "aggression": "xl", "space": "xh",
                 "warmth": "mid", "movement": "mid"}
    },
    # 5. W-XHIGH + D-XHIGH + M-XLOW + A-XHIGH
    {
        "name": "HOT DENSE STATIC VIOLENT",
        "dims": {"warmth": "xh", "density": "xh", "movement": "xl", "aggression": "xh",
                 "brightness": "mid", "space": "mid"}
    },
    # 6. W-XLOW + S-XLOW + M-XHIGH + B-XLOW
    {
        "name": "COLD TIGHT KINETIC DARK",
        "dims": {"warmth": "xl", "space": "xl", "movement": "xh", "brightness": "xl",
                 "aggression": "mid", "density": "mid"}
    },
    # 7. B-XHIGH + W-XLOW + D-XHIGH + A-XHIGH
    {
        "name": "BRIGHT COLD VIOLENT DENSE",
        "dims": {"brightness": "xh", "warmth": "xl", "density": "xh", "aggression": "xh",
                 "movement": "mid", "space": "mid"}
    },
    # 8. B-XLOW + W-XHIGH + S-XHIGH + M-XHIGH
    {
        "name": "DARK HOT VAST KINETIC",
        "dims": {"brightness": "xl", "warmth": "xh", "space": "xh", "movement": "xh",
                 "aggression": "mid", "density": "mid"}
    },
    # 9. M-XHIGH + A-XHIGH + D-XLOW + S-XHIGH
    {
        "name": "KINETIC VIOLENT SPARSE VAST",
        "dims": {"movement": "xh", "aggression": "xh", "density": "xl", "space": "xh",
                 "brightness": "mid", "warmth": "mid"}
    },
    # 10. M-XLOW + A-XLOW + D-XLOW + S-XHIGH
    {
        "name": "STATIC GENTLE SPARSE VAST",
        "dims": {"movement": "xl", "aggression": "xl", "density": "xl", "space": "xh",
                 "brightness": "mid", "warmth": "mid"}
    },
    # 11. B-XLOW + W-XLOW + D-XLOW + S-XHIGH
    {
        "name": "DARK COLD SPARSE VAST",
        "dims": {"brightness": "xl", "warmth": "xl", "density": "xl", "space": "xh",
                 "aggression": "mid", "movement": "mid"}
    },
    # 12. B-XHIGH + W-XHIGH + A-XHIGH + M-XHIGH
    {
        "name": "BRIGHT HOT VIOLENT KINETIC",
        "dims": {"brightness": "xh", "warmth": "xh", "aggression": "xh", "movement": "xh",
                 "density": "mid", "space": "mid"}
    },
    # 13. W-XLOW + D-XHIGH + M-XHIGH + S-XLOW
    {
        "name": "COLD DENSE KINETIC TIGHT",
        "dims": {"warmth": "xl", "density": "xh", "movement": "xh", "space": "xl",
                 "brightness": "mid", "aggression": "mid"}
    },
    # 14. A-XHIGH + D-XHIGH + W-XHIGH + S-XLOW
    {
        "name": "VIOLENT DENSE HOT TIGHT",
        "dims": {"aggression": "xh", "density": "xh", "warmth": "xh", "space": "xl",
                 "brightness": "mid", "movement": "mid"}
    },
    # 15. B-XLOW + M-XHIGH + A-XHIGH + S-XHIGH
    {
        "name": "DARK KINETIC VIOLENT VAST",
        "dims": {"brightness": "xl", "movement": "xh", "aggression": "xh", "space": "xh",
                 "warmth": "mid", "density": "mid"}
    },
    # 16. B-XHIGH + M-XLOW + S-XHIGH + D-XLOW
    {
        "name": "BRIGHT STATIC VAST SPARSE",
        "dims": {"brightness": "xh", "movement": "xl", "space": "xh", "density": "xl",
                 "warmth": "mid", "aggression": "mid"}
    },
]

def resolve_dim(spec):
    if spec == "xl":
        return xlow()
    elif spec == "xh":
        return xhigh()
    else:
        return mid()

def make_dna(dims_spec):
    return {
        "brightness": resolve_dim(dims_spec["brightness"]),
        "warmth": resolve_dim(dims_spec["warmth"]),
        "aggression": resolve_dim(dims_spec["aggression"]),
        "movement": resolve_dim(dims_spec["movement"]),
        "density": resolve_dim(dims_spec["density"]),
        "space": resolve_dim(dims_spec["space"]),
    }

def dna_to_tags(dna):
    tags = []
    if dna["brightness"] <= 0.12:
        tags.append("dark")
    elif dna["brightness"] >= 0.88:
        tags.append("bright")
    if dna["warmth"] <= 0.12:
        tags.append("cold")
    elif dna["warmth"] >= 0.88:
        tags.append("warm")
    if dna["aggression"] <= 0.12:
        tags.append("gentle")
    elif dna["aggression"] >= 0.88:
        tags.append("aggressive")
    if dna["movement"] <= 0.12:
        tags.append("static")
    elif dna["movement"] >= 0.88:
        tags.append("kinetic")
    if dna["density"] <= 0.12:
        tags.append("sparse")
    elif dna["density"] >= 0.88:
        tags.append("dense")
    if dna["space"] <= 0.12:
        tags.append("tight")
    elif dna["space"] >= 0.88:
        tags.append("vast")
    tags.append("flux")
    tags.append("extreme")
    return tags

def make_description(name, dna):
    extremes = []
    dim_names = {
        "brightness": ("dark", "bright"),
        "warmth": ("cold", "hot"),
        "aggression": ("gentle", "violent"),
        "movement": ("static", "kinetic"),
        "density": ("sparse", "dense"),
        "space": ("tight", "vast"),
    }
    for dim, (low_word, high_word) in dim_names.items():
        val = dna[dim]
        if val <= 0.12:
            extremes.append(f"extreme {low_word} {dim}")
        elif val >= 0.88:
            extremes.append(f"extreme {high_word} {dim}")
    return f"Ultra-diverse Flux preset — {', '.join(extremes)}. Maximum contrast character design."

def make_preset(batch_name, batch_dims, index, engine, coupling_type):
    dna = make_dna(batch_dims)
    name = f"{batch_name} F{index}"
    tags = dna_to_tags(dna)
    description = make_description(name, dna)

    # Minimal parameters block — engine-agnostic placeholder
    engine_key = f"X{engine.capitalize()}" if not engine.startswith("ODD") else engine
    # Use consistent key format matching existing presets
    engine_display = engine

    preset = {
        "schema_version": 1,
        "name": name,
        "mood": "Flux",
        "sonic_dna": {
            "brightness": dna["brightness"],
            "warmth": dna["warmth"],
            "aggression": dna["aggression"],
            "movement": dna["movement"],
            "density": dna["density"],
            "space": dna["space"],
        },
        "engines": [engine_display],
        "author": "XO_OX",
        "version": "1.0.0",
        "description": description,
        "tags": tags,
        "macroLabels": ["MACRO1", "MACRO2", "MACRO3", "MACRO4"],
        "couplingIntensity": coupling_type,
        "dna": {
            "brightness": dna["brightness"],
            "warmth": dna["warmth"],
            "movement": dna["movement"],
            "density": dna["density"],
            "space": dna["space"],
            "aggression": dna["aggression"],
        },
        "parameters": {
            engine_display: {}
        },
        "coupling": {
            "pairs": []
        }
    }
    return preset

def filename_for(name):
    safe = name.replace(" ", "_").replace("/", "-")
    return f"{safe}.xometa"

def main():
    # Shuffle engines and couplings for distribution across 80 presets
    engines_cycle = ENGINES * 3  # 102 slots, we use 80
    random.shuffle(engines_cycle)
    couplings_cycle = (COUPLING_TYPES * 7)[:80]  # 84 -> take 80
    random.shuffle(couplings_cycle)

    written = 0
    skipped = 0
    preset_idx = 0

    for batch_i, batch in enumerate(BATCHES):
        batch_name = batch["name"]
        batch_dims = batch["dims"]
        for j in range(1, 6):  # 5 presets per batch
            engine = engines_cycle[preset_idx % len(engines_cycle)]
            coupling = couplings_cycle[preset_idx % len(couplings_cycle)]
            preset_idx += 1

            preset = make_preset(batch_name, batch_dims, j, engine, coupling)
            fname = filename_for(preset["name"])
            fpath = os.path.join(OUTPUT_DIR, fname)

            if os.path.exists(fpath):
                print(f"  SKIP (exists): {fname}")
                skipped += 1
                continue

            with open(fpath, "w") as f:
                json.dump(preset, f, indent=2)
            print(f"  WRITE: {fname}")
            written += 1

    print(f"\nDone. Written: {written}, Skipped: {skipped}")
    print(f"Output dir: {OUTPUT_DIR}")
    total = len(os.listdir(OUTPUT_DIR))
    print(f"Total Flux presets now: {total}")

if __name__ == "__main__":
    main()
