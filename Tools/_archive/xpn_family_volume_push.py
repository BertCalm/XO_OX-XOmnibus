#!/usr/bin/env python3
"""
xpn_family_volume_push.py
Generates 150 Family mood presets (FAM_VOL series) for XOceanus.
30 corner DNA combinations × 5 variants each = 150 presets.
Output: Presets/XOceanus/Family/
"""

import json
import os
import random
from pathlib import Path

# ── Seed for reproducibility ──────────────────────────────────────────────────
random.seed(2026_03_16)

# ── Paths ─────────────────────────────────────────────────────────────────────
REPO_ROOT = Path(__file__).parent.parent
OUTPUT_DIR = REPO_ROOT / "Presets" / "XOceanus" / "Family"
OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

# ── DNA range helpers ─────────────────────────────────────────────────────────
def xhigh():
    return round(random.uniform(0.87, 0.99), 3)

def xlow():
    return round(random.uniform(0.02, 0.13), 3)

def mid():
    return round(random.uniform(0.30, 0.70), 3)

# ── Family engines ────────────────────────────────────────────────────────────
ENGINES = [
    "ODDFELIX", "ODDOSCAR", "OBLONG", "OVERDUB", "ODYSSEY",
    "OPAL", "OHM", "ORPHICA", "OBBLIGATO", "OTTONI",
    "OLE", "OSPREY", "OSTERIA", "ORGANON", "ORIGAMI",
]

# ── Coupling types ────────────────────────────────────────────────────────────
COUPLING_TYPES = [
    "TIMBRE_BLEND", "PITCH_SYNC", "RHYTHM_LOCK", "HARMONIC_MESH",
    "ENVELOPE_MIRROR", "SPECTRAL_WEAVE", "RESONANCE_SHARE",
    "PHASE_COUPLE", "DYNAMIC_CROSS", "FILTER_LINK",
    "MODULATION_FLOW", "SPATIAL_BIND",
]

# ── Tags per corner archetype ─────────────────────────────────────────────────
CORNER_TAGS = [
    ["family", "warm", "intimate", "bright", "still"],           # 1
    ["family", "warm", "kinetic", "dense", "bright"],            # 2
    ["family", "hot", "vast", "gentle", "dark"],                 # 3
    ["family", "warm", "sparse", "vast", "bright"],              # 4
    ["family", "warm", "still", "dense", "bright"],              # 5
    ["family", "hot", "kinetic", "gentle", "dark"],              # 6
    ["family", "warm", "intimate", "gentle", "bright"],          # 7
    ["family", "bright", "dense", "intimate", "cool"],           # 8
    ["family", "dark", "cold", "still", "dense"],                # 9
    ["family", "warm", "kinetic", "vast", "bright"],             # 10
    ["family", "warm", "dense", "intimate", "bright"],           # 11
    ["family", "hot", "dense", "intimate", "dark"],              # 12
    ["family", "warm", "still", "vast", "bright"],               # 13
    ["family", "bright", "cold", "kinetic", "vast"],             # 14
    ["family", "hot", "still", "sparse", "dark"],                # 15
    ["family", "warm", "kinetic", "aggressive", "bright"],       # 16
    ["family", "warm", "dense", "gentle", "bright"],             # 17
    ["family", "hot", "intimate", "gentle", "dark"],             # 18
    ["family", "warm", "kinetic", "sparse", "bright"],           # 19
    ["family", "bright", "cold", "sparse", "vast"],              # 20
    ["family", "dark", "cold", "dense", "violent"],              # 21
    ["family", "warm", "vast", "aggressive", "bright"],          # 22
    ["family", "warm", "kinetic", "intimate", "bright"],         # 23
    ["family", "hot", "dense", "aggressive", "dark"],            # 24
    ["family", "warm", "sparse", "aggressive", "bright"],        # 25
    ["family", "dark", "cold", "vast", "gentle"],                # 26
    ["family", "warm", "still", "aggressive", "bright"],         # 27
    ["family", "hot", "kinetic", "vast", "dark"],                # 28
    ["family", "bright", "cold", "intimate", "violent"],         # 29
    ["family", "hot", "kinetic", "dense", "dark"],               # 30
]

# ── Short archetype names for file names ──────────────────────────────────────
CORNER_NAMES = [
    "BRIGHT_WARM_STILL_INTIMATE",      # 1
    "BRIGHT_WARM_KINETIC_DENSE",       # 2
    "DARK_HOT_VAST_GENTLE",            # 3
    "BRIGHT_WARM_SPARSE_VAST",         # 4
    "BRIGHT_WARM_STILL_DENSE",         # 5
    "DARK_HOT_KINETIC_GENTLE",         # 6
    "BRIGHT_WARM_INTIMATE_GENTLE",     # 7
    "BRIGHT_COLD_DENSE_INTIMATE",      # 8
    "DARK_COLD_STILL_DENSE",           # 9
    "BRIGHT_WARM_KINETIC_VAST",        # 10
    "BRIGHT_WARM_DENSE_INTIMATE",      # 11
    "DARK_HOT_DENSE_INTIMATE",         # 12
    "BRIGHT_WARM_STILL_VAST",          # 13
    "BRIGHT_COLD_KINETIC_VAST",        # 14
    "DARK_HOT_STILL_SPARSE",           # 15
    "BRIGHT_WARM_KINETIC_VIOLENT",     # 16
    "BRIGHT_WARM_DENSE_GENTLE",        # 17
    "DARK_HOT_INTIMATE_GENTLE",        # 18
    "BRIGHT_WARM_KINETIC_SPARSE",      # 19
    "BRIGHT_COLD_SPARSE_VAST",         # 20
    "DARK_COLD_DENSE_VIOLENT",         # 21
    "BRIGHT_WARM_VAST_VIOLENT",        # 22
    "BRIGHT_WARM_KINETIC_INTIMATE",    # 23
    "DARK_HOT_DENSE_VIOLENT",          # 24
    "BRIGHT_WARM_SPARSE_VIOLENT",      # 25
    "DARK_COLD_VAST_GENTLE",           # 26
    "BRIGHT_WARM_STILL_VIOLENT",       # 27
    "DARK_HOT_KINETIC_VAST",           # 28
    "BRIGHT_COLD_INTIMATE_VIOLENT",    # 29
    "DARK_HOT_KINETIC_DENSE",          # 30
]

# ── 30 corner DNA definitions ─────────────────────────────────────────────────
# Each entry: dict with keys brightness/warmth/movement/density/space/aggression
# Using callables so each call produces a fresh random draw within the range.
# The "fixed" dims are flagged; unspecified dims default to mid().
CORNERS = [
    # 1  brightness-XHIGH warmth-XHIGH movement-XLOW space-XLOW
    dict(brightness="H", warmth="H", movement="L", density="M", space="L", aggression="M"),
    # 2  brightness-XHIGH warmth-XHIGH movement-XHIGH density-XHIGH
    dict(brightness="H", warmth="H", movement="H", density="H", space="M", aggression="M"),
    # 3  brightness-XLOW warmth-XHIGH space-XHIGH aggression-XLOW
    dict(brightness="L", warmth="H", movement="M", density="M", space="H", aggression="L"),
    # 4  brightness-XHIGH warmth-XHIGH density-XLOW space-XHIGH
    dict(brightness="H", warmth="H", movement="M", density="L", space="H", aggression="M"),
    # 5  brightness-XHIGH warmth-XHIGH movement-XLOW density-XHIGH
    dict(brightness="H", warmth="H", movement="L", density="H", space="M", aggression="M"),
    # 6  brightness-XLOW warmth-XHIGH movement-XHIGH aggression-XLOW
    dict(brightness="L", warmth="H", movement="H", density="M", space="M", aggression="L"),
    # 7  brightness-XHIGH warmth-XHIGH space-XLOW aggression-XLOW
    dict(brightness="H", warmth="H", movement="M", density="M", space="L", aggression="L"),
    # 8  brightness-XHIGH warmth-XLOW density-XHIGH space-XLOW
    dict(brightness="H", warmth="L", movement="M", density="H", space="L", aggression="M"),
    # 9  brightness-XLOW warmth-XLOW movement-XLOW density-XHIGH
    dict(brightness="L", warmth="L", movement="L", density="H", space="M", aggression="M"),
    # 10 brightness-XHIGH warmth-XHIGH movement-XHIGH space-XHIGH
    dict(brightness="H", warmth="H", movement="H", density="M", space="H", aggression="M"),
    # 11 brightness-XHIGH warmth-XHIGH density-XHIGH space-XLOW
    dict(brightness="H", warmth="H", movement="M", density="H", space="L", aggression="M"),
    # 12 brightness-XLOW warmth-XHIGH density-XHIGH space-XLOW
    dict(brightness="L", warmth="H", movement="M", density="H", space="L", aggression="M"),
    # 13 brightness-XHIGH warmth-XHIGH movement-XLOW space-XHIGH
    dict(brightness="H", warmth="H", movement="L", density="M", space="H", aggression="M"),
    # 14 brightness-XHIGH warmth-XLOW movement-XHIGH space-XHIGH
    dict(brightness="H", warmth="L", movement="H", density="M", space="H", aggression="M"),
    # 15 brightness-XLOW warmth-XHIGH movement-XLOW density-XLOW
    dict(brightness="L", warmth="H", movement="L", density="L", space="M", aggression="M"),
    # 16 brightness-XHIGH warmth-XHIGH movement-XHIGH aggression-XHIGH
    dict(brightness="H", warmth="H", movement="H", density="M", space="M", aggression="H"),
    # 17 brightness-XHIGH warmth-XHIGH density-XHIGH aggression-XLOW
    dict(brightness="H", warmth="H", movement="M", density="H", space="M", aggression="L"),
    # 18 brightness-XLOW warmth-XHIGH space-XLOW aggression-XLOW
    dict(brightness="L", warmth="H", movement="M", density="M", space="L", aggression="L"),
    # 19 brightness-XHIGH warmth-XHIGH movement-XHIGH density-XLOW
    dict(brightness="H", warmth="H", movement="H", density="L", space="M", aggression="M"),
    # 20 brightness-XHIGH warmth-XLOW density-XLOW space-XHIGH
    dict(brightness="H", warmth="L", movement="M", density="L", space="H", aggression="M"),
    # 21 brightness-XLOW warmth-XLOW density-XHIGH aggression-XHIGH
    dict(brightness="L", warmth="L", movement="M", density="H", space="M", aggression="H"),
    # 22 brightness-XHIGH warmth-XHIGH space-XHIGH aggression-XHIGH
    dict(brightness="H", warmth="H", movement="M", density="M", space="H", aggression="H"),
    # 23 brightness-XHIGH warmth-XHIGH movement-XHIGH space-XLOW
    dict(brightness="H", warmth="H", movement="H", density="M", space="L", aggression="M"),
    # 24 brightness-XLOW warmth-XHIGH density-XHIGH aggression-XHIGH
    dict(brightness="L", warmth="H", movement="M", density="H", space="M", aggression="H"),
    # 25 brightness-XHIGH warmth-XHIGH density-XLOW aggression-XHIGH
    dict(brightness="H", warmth="H", movement="M", density="L", space="M", aggression="H"),
    # 26 brightness-XLOW warmth-XLOW space-XHIGH aggression-XLOW
    dict(brightness="L", warmth="L", movement="M", density="M", space="H", aggression="L"),
    # 27 brightness-XHIGH warmth-XHIGH movement-XLOW aggression-XHIGH
    dict(brightness="H", warmth="H", movement="L", density="M", space="M", aggression="H"),
    # 28 brightness-XLOW warmth-XHIGH movement-XHIGH space-XHIGH
    dict(brightness="L", warmth="H", movement="H", density="M", space="H", aggression="M"),
    # 29 brightness-XHIGH warmth-XLOW space-XLOW aggression-XHIGH
    dict(brightness="H", warmth="L", movement="M", density="M", space="L", aggression="H"),
    # 30 brightness-XLOW warmth-XHIGH movement-XHIGH density-XHIGH
    dict(brightness="L", warmth="H", movement="H", density="H", space="M", aggression="M"),
]

def resolve_dna_val(flag):
    if flag == "H":
        return xhigh()
    elif flag == "L":
        return xlow()
    else:
        return mid()

def make_dna(corner_spec):
    return {
        "brightness": resolve_dna_val(corner_spec["brightness"]),
        "warmth":     resolve_dna_val(corner_spec["warmth"]),
        "movement":   resolve_dna_val(corner_spec["movement"]),
        "density":    resolve_dna_val(corner_spec["density"]),
        "space":      resolve_dna_val(corner_spec["space"]),
        "aggression": resolve_dna_val(corner_spec["aggression"]),
    }

def make_macro_params(dna, engine):
    """Derive per-engine macro params from DNA values with slight per-engine jitter."""
    jitter = lambda v: round(min(0.99, max(0.01, v + random.uniform(-0.03, 0.03))), 3)
    return {
        engine: {
            "macro_character": jitter((dna["brightness"] + dna["warmth"]) / 2),
            "macro_movement":  jitter(dna["movement"]),
            "macro_coupling":  jitter((dna["density"] + dna["aggression"]) / 2),
            "macro_space":     jitter(dna["space"]),
        }
    }

def generate_preset(corner_idx, variant_idx):
    corner_spec = CORNERS[corner_idx]
    corner_name = CORNER_NAMES[corner_idx]
    tags = CORNER_TAGS[corner_idx]
    vol_num = corner_idx * 5 + variant_idx + 1  # 1–150

    dna = make_dna(corner_spec)

    # Pick 2 distinct engines
    eng1, eng2 = random.sample(ENGINES, 2)

    params = {}
    params.update(make_macro_params(dna, eng1))
    params.update(make_macro_params(dna, eng2))

    coupling_type = random.choice(COUPLING_TYPES)
    coupling_amount = round(random.uniform(0.4, 0.95), 3)

    # Determine source/target — engine with higher character drives
    if params[eng1]["macro_character"] >= params[eng2]["macro_character"]:
        source, target = eng1, eng2
    else:
        source, target = eng2, eng1

    macros = {
        "CHARACTER": round((params[eng1]["macro_character"] + params[eng2]["macro_character"]) / 2, 3),
        "MOVEMENT":  round((params[eng1]["macro_movement"]  + params[eng2]["macro_movement"])  / 2, 3),
        "COUPLING":  round((params[eng1]["macro_coupling"]  + params[eng2]["macro_coupling"])  / 2, 3),
        "SPACE":     round((params[eng1]["macro_space"]     + params[eng2]["macro_space"])     / 2, 3),
    }

    preset_name = f"{corner_name}_FAM_VOL_{vol_num}"
    filename = f"{preset_name}.xometa"

    preset = {
        "name": preset_name,
        "version": "1.0",
        "mood": "Family",
        "engines": [eng1, eng2],
        "parameters": params,
        "coupling": {
            "type": coupling_type,
            "source": source,
            "target": target,
            "amount": coupling_amount,
        },
        "dna": dna,
        "macros": macros,
        "tags": tags,
    }

    return filename, preset


def main():
    generated = []
    for c_idx in range(30):
        for v_idx in range(5):
            filename, preset = generate_preset(c_idx, v_idx)
            out_path = OUTPUT_DIR / filename
            with open(out_path, "w") as f:
                json.dump(preset, f, indent=2)
            generated.append(filename)

    print(f"Generated {len(generated)} FAM_VOL presets → {OUTPUT_DIR}")
    print("Sample files:")
    for name in generated[:5]:
        print(f"  {name}")
    print(f"  ...")
    for name in generated[-3:]:
        print(f"  {name}")

    # Quick verification
    all_files = list(OUTPUT_DIR.glob("*_FAM_VOL_*.xometa"))
    print(f"\nVerification: {len(all_files)} FAM_VOL files confirmed in {OUTPUT_DIR}")
    total_family = list(OUTPUT_DIR.glob("*.xometa"))
    print(f"Total Family presets (all series): {len(total_family)}")


if __name__ == "__main__":
    main()
