#!/usr/bin/env python3
"""
xpn_atmosphere_flux_mega_diverse.py
Generates 100 presets (50 Atmosphere + 50 Flux) with maximum DNA diversity.
10 extreme corners × 5 variants per mood.
"""

import json
import os
import random

random.seed(42)

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ATM_DIR = os.path.join(REPO_ROOT, "Presets", "XOmnibus", "Atmosphere")
FLUX_DIR = os.path.join(REPO_ROOT, "Presets", "XOmnibus", "Flux")

# DNA ranges
XLOW_RANGE  = (0.02, 0.13)
XHIGH_RANGE = (0.87, 0.99)
MID_RANGE   = (0.30, 0.70)

DIMS = ["brightness", "warmth", "movement", "density", "space", "aggression"]

COUPLING_TYPES = [
    "FREQUENCY_SHIFT", "AMPLITUDE_MOD", "FILTER_MOD", "PITCH_SYNC",
    "TIMBRE_BLEND", "ENVELOPE_LINK", "HARMONIC_FOLD", "CHAOS_INJECT",
    "RESONANCE_SHARE", "SPATIAL_COUPLE", "SPECTRAL_MORPH", "VELOCITY_COUPLE",
]

ATMOSPHERE_ENGINES = [
    "OPAL", "ORACLE", "OBSCURA", "OCEANIC", "OPTIC", "OBLIQUE",
    "OSPREY", "ORBITAL", "ORGANON", "ORIGAMI", "ODDFELIX", "ODDOSCAR",
]

FLUX_ENGINES = [
    "OUROBOROS", "ORACLE", "ORGANON", "ORIGAMI", "OPTIC", "OPAL",
    "OBLONG", "ODYSSEY", "OVERDUB", "OBLIQUE", "OCEANIC", "OVERWORLD",
]

def sample_range(r):
    return round(random.uniform(*r), 4)

def jitter(val, delta=0.03):
    """Small jitter around a value while staying in [0.02, 0.99]."""
    return round(min(0.99, max(0.02, val + random.uniform(-delta, delta))), 4)

# ---------------------------------------------------------------------------
# Corner definitions
# Each corner is a dict of dim -> zone ('XLOW'|'XHIGH'|'MID')
# Unspecified dims default to MID.
# ---------------------------------------------------------------------------

def build_corner(spec):
    """spec: dict of dim -> zone string. Missing dims = MID."""
    result = {}
    for dim in DIMS:
        zone = spec.get(dim, "MID")
        if zone == "XLOW":
            result[dim] = sample_range(XLOW_RANGE)
        elif zone == "XHIGH":
            result[dim] = sample_range(XHIGH_RANGE)
        else:
            result[dim] = sample_range(MID_RANGE)
    return result

ATM_CORNERS = [
    # 1. bright cold kinetic vast gentle
    {"brightness":"XHIGH","warmth":"XLOW","movement":"XHIGH","space":"XHIGH","aggression":"XLOW"},
    # 2. bright warm sparse vast gentle
    {"brightness":"XHIGH","warmth":"XHIGH","density":"XLOW","space":"XHIGH","aggression":"XLOW"},
    # 3. dark hot kinetic vast gentle
    {"brightness":"XLOW","warmth":"XHIGH","movement":"XHIGH","space":"XHIGH","aggression":"XLOW"},
    # 4. bright kinetic sparse vast violent
    {"brightness":"XHIGH","movement":"XHIGH","density":"XLOW","space":"XHIGH","aggression":"XHIGH"},
    # 5. dark cold kinetic sparse vast
    {"brightness":"XLOW","warmth":"XLOW","movement":"XHIGH","density":"XLOW","space":"XHIGH"},
    # 6. bright cold sparse vast gentle
    {"brightness":"XHIGH","warmth":"XLOW","density":"XLOW","space":"XHIGH","aggression":"XLOW"},
    # 7. hot kinetic sparse vast gentle
    {"warmth":"XHIGH","movement":"XHIGH","density":"XLOW","space":"XHIGH","aggression":"XLOW"},
    # 8. bright warm kinetic sparse gentle
    {"brightness":"XHIGH","warmth":"XHIGH","movement":"XHIGH","density":"XLOW","aggression":"XLOW"},
    # 9. dark still sparse vast gentle
    {"brightness":"XLOW","movement":"XLOW","density":"XLOW","space":"XHIGH","aggression":"XLOW"},
    # 10. bright warm kinetic vast violent
    {"brightness":"XHIGH","warmth":"XHIGH","space":"XHIGH","aggression":"XHIGH","movement":"XHIGH"},
]

FLUX_CORNERS = [
    # 1. dark cold kinetic dense violent
    {"brightness":"XLOW","warmth":"XLOW","movement":"XHIGH","density":"XHIGH","aggression":"XHIGH"},
    # 2. bright warm kinetic dense violent
    {"brightness":"XHIGH","warmth":"XHIGH","movement":"XHIGH","density":"XHIGH","aggression":"XHIGH"},
    # 3. dark hot kinetic intimate violent
    {"brightness":"XLOW","warmth":"XHIGH","movement":"XHIGH","space":"XLOW","aggression":"XHIGH"},
    # 4. bright cold kinetic sparse violent
    {"brightness":"XHIGH","warmth":"XLOW","movement":"XHIGH","density":"XLOW","aggression":"XHIGH"},
    # 5. dark kinetic dense vast violent
    {"brightness":"XLOW","movement":"XHIGH","density":"XHIGH","space":"XHIGH","aggression":"XHIGH"},
    # 6. cold kinetic dense intimate violent
    {"warmth":"XLOW","movement":"XHIGH","density":"XHIGH","space":"XLOW","aggression":"XHIGH"},
    # 7. dark cold still dense violent
    {"brightness":"XLOW","warmth":"XLOW","movement":"XLOW","density":"XHIGH","aggression":"XHIGH"},
    # 8. bright kinetic dense intimate violent
    {"brightness":"XHIGH","movement":"XHIGH","density":"XHIGH","space":"XLOW","aggression":"XHIGH"},
    # 9. hot kinetic sparse vast violent
    {"warmth":"XHIGH","movement":"XHIGH","density":"XLOW","space":"XHIGH","aggression":"XHIGH"},
    # 10. dark hot dense vast violent
    {"brightness":"XLOW","warmth":"XHIGH","density":"XHIGH","space":"XHIGH","aggression":"XHIGH"},
]

# Human-readable labels for corner names
ATM_LABELS = [
    "BRIGHT_COLD_KINETIC_VAST_GENTLE",
    "BRIGHT_WARM_SPARSE_VAST_GENTLE",
    "DARK_HOT_KINETIC_VAST_GENTLE",
    "BRIGHT_KINETIC_SPARSE_VAST_VIOLENT",
    "DARK_COLD_KINETIC_SPARSE_VAST",
    "BRIGHT_COLD_SPARSE_VAST_GENTLE",
    "HOT_KINETIC_SPARSE_VAST_GENTLE",
    "BRIGHT_WARM_KINETIC_SPARSE_GENTLE",
    "DARK_STILL_SPARSE_VAST_GENTLE",
    "BRIGHT_WARM_KINETIC_VAST_VIOLENT",
]

FLUX_LABELS = [
    "DARK_COLD_KINETIC_DENSE_VIOLENT",
    "BRIGHT_WARM_KINETIC_DENSE_VIOLENT",
    "DARK_HOT_KINETIC_INTIMATE_VIOLENT",
    "BRIGHT_COLD_KINETIC_SPARSE_VIOLENT",
    "DARK_KINETIC_DENSE_VAST_VIOLENT",
    "COLD_KINETIC_DENSE_INTIMATE_VIOLENT",
    "DARK_COLD_STILL_DENSE_VIOLENT",
    "BRIGHT_KINETIC_DENSE_INTIMATE_VIOLENT",
    "HOT_KINETIC_SPARSE_VAST_VIOLENT",
    "DARK_HOT_DENSE_VAST_VIOLENT",
]


def dna_to_tags(dna, mood):
    tags = [mood.lower()]
    if dna["brightness"] >= 0.87:
        tags.append("bright")
    elif dna["brightness"] <= 0.13:
        tags.append("dark")
    if dna["warmth"] >= 0.87:
        tags.append("warm")
    elif dna["warmth"] <= 0.13:
        tags.append("cold")
    if dna["movement"] >= 0.87:
        tags.append("kinetic")
    elif dna["movement"] <= 0.13:
        tags.append("still")
    if dna["density"] >= 0.87:
        tags.append("dense")
    elif dna["density"] <= 0.13:
        tags.append("sparse")
    if dna["space"] >= 0.87:
        tags.append("vast")
    elif dna["space"] <= 0.13:
        tags.append("intimate")
    if dna["aggression"] >= 0.87:
        tags.append("violent")
    elif dna["aggression"] <= 0.13:
        tags.append("gentle")
    return tags


def make_engine_params(dna):
    """Map DNA dims to per-engine macro params with small per-engine jitter."""
    return {
        "macro_character": jitter((dna["brightness"] + dna["warmth"]) / 2),
        "macro_movement":  jitter(dna["movement"]),
        "macro_coupling":  jitter(dna["density"]),
        "macro_space":     jitter(dna["space"]),
    }


def make_preset(name, mood, engines_pool, dna, label):
    eng1, eng2 = random.sample(engines_pool, 2)
    coupling_type = random.choice(COUPLING_TYPES)
    coupling_amount = round(random.uniform(0.4, 0.95), 3)

    parameters = {
        eng1: make_engine_params(dna),
        eng2: make_engine_params(dna),
    }

    avg_char  = round((parameters[eng1]["macro_character"] + parameters[eng2]["macro_character"]) / 2, 4)
    avg_move  = round((parameters[eng1]["macro_movement"]  + parameters[eng2]["macro_movement"])  / 2, 4)
    avg_coupl = round((parameters[eng1]["macro_coupling"]  + parameters[eng2]["macro_coupling"])  / 2, 4)
    avg_space = round((parameters[eng1]["macro_space"]     + parameters[eng2]["macro_space"])     / 2, 4)

    return {
        "name": name,
        "version": "1.0",
        "mood": mood,
        "engines": [eng1, eng2],
        "parameters": parameters,
        "coupling": {
            "type": coupling_type,
            "source": eng1,
            "target": eng2,
            "amount": coupling_amount,
        },
        "dna": dna,
        "macros": {
            "CHARACTER": avg_char,
            "MOVEMENT":  avg_move,
            "COUPLING":  avg_coupl,
            "SPACE":     avg_space,
        },
        "tags": dna_to_tags(dna, mood),
    }


def generate_batch(corners, labels, mood, engines_pool, suffix, out_dir, variants=5):
    os.makedirs(out_dir, exist_ok=True)
    count = 0
    for ci, (corner_spec, label) in enumerate(zip(corners, labels)):
        for v in range(1, variants + 1):
            dna = build_corner(corner_spec)
            name = f"{label}_{suffix}_{v}"
            preset = make_preset(name, mood, engines_pool, dna, label)
            path = os.path.join(out_dir, f"{name}.xometa")
            with open(path, "w") as f:
                json.dump(preset, f, indent=2)
            count += 1
    return count


def main():
    atm_count = generate_batch(
        ATM_CORNERS, ATM_LABELS,
        mood="Atmosphere",
        engines_pool=ATMOSPHERE_ENGINES,
        suffix="ATM",
        out_dir=ATM_DIR,
        variants=5,
    )

    flux_count = generate_batch(
        FLUX_CORNERS, FLUX_LABELS,
        mood="Flux",
        engines_pool=FLUX_ENGINES,
        suffix="FLX2",
        out_dir=FLUX_DIR,
        variants=5,
    )

    print(f"Atmosphere presets written : {atm_count}  →  {ATM_DIR}")
    print(f"Flux presets written       : {flux_count}  →  {FLUX_DIR}")
    print(f"Total                      : {atm_count + flux_count}")

    # Verify
    atm_files  = [f for f in os.listdir(ATM_DIR)  if f.endswith(".xometa") and "_ATM_"  in f]
    flux_files  = [f for f in os.listdir(FLUX_DIR) if f.endswith(".xometa") and "_FLX2_" in f]
    print(f"\nVerification — ATM files  in dir: {len(atm_files)}")
    print(f"Verification — FLX2 files in dir: {len(flux_files)}")

    if len(atm_files) == 50 and len(flux_files) == 50:
        print("\n✓ 50 Atmosphere + 50 Flux confirmed.")
    else:
        print("\n✗ Count mismatch — check output.")


if __name__ == "__main__":
    main()
