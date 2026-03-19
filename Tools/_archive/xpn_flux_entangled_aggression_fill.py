#!/usr/bin/env python3
"""
xpn_flux_entangled_aggression_fill.py
Generates 100 aggression-XHIGH presets:
  40 Flux    (8 corners × 5 variants)
  60 Entangled (12 corners × 5 variants)
All presets have aggression ≥ 0.87.
"""

import json
import os
import random

random.seed(42)

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def r_xhigh():
    """0.87–0.99"""
    return round(random.uniform(0.87, 0.99), 3)

def r_xlow():
    """0.02–0.13"""
    return round(random.uniform(0.02, 0.13), 3)

def r_mid():
    """0.30–0.70"""
    return round(random.uniform(0.30, 0.70), 3)

def r_param():
    """Generic macro param — mid range."""
    return round(random.uniform(0.35, 0.75), 3)

def write_preset(path, data):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w") as f:
        json.dump(data, f, indent=2)

# ---------------------------------------------------------------------------
# Engine pools
# ---------------------------------------------------------------------------

FLUX_ENGINES = [
    "OUROBOROS", "ORACLE", "ORGANON", "ORIGAMI", "OPTIC", "OPAL",
    "OBLONG", "ODYSSEY", "OVERDUB", "OBLIQUE", "OCEANIC", "OVERWORLD",
]

ENTANGLED_ENGINES = [
    "ODDFELIX", "ODDOSCAR", "OBLONG", "OBESE", "OPAL", "ORBITAL", "ORGANON",
    "OUROBOROS", "OBSIDIAN", "ORIGAMI", "ORACLE", "OBSCURA", "OCEANIC",
    "OCELOT", "OPTIC", "OBLIQUE", "OSPREY", "OSTERIA", "OWLFISH", "OHM",
    "ORPHICA", "OBBLIGATO", "OTTONI", "OLE", "OVERDUB", "ODYSSEY", "OVERWORLD",
    "OVERBITE", "OMBRE", "ORCA", "OCTOPUS", "OVERLAP", "OUTWIT", "ONSET",
]

ENTANGLED_COUPLING_TYPES = [
    "CHAOS_INJECT", "RESONANCE_SHARE", "HARMONIC_FOLD",
    "SPECTRAL_MORPH", "VELOCITY_COUPLE",
]

FLUX_COUPLING_TYPES = [
    "TIMBRE_BLEND", "CHAOS_INJECT", "RESONANCE_SHARE",
    "HARMONIC_FOLD", "SPECTRAL_MORPH",
]

# ---------------------------------------------------------------------------
# Flux corners — 8 corners
# Each corner defines which DNA dimensions are fixed (XHIGH/XLOW) and which float.
# Schema: list of (dim, value_fn) — remaining dims use r_mid()
# ---------------------------------------------------------------------------

FLUX_CORNERS = [
    # 1. dark cold kinetic violent
    {
        "label": "DARK_COLD_KINETIC_VIOLENT",
        "dna_fixed": {"brightness": r_xlow, "warmth": r_xlow, "movement": r_xhigh, "aggression": r_xhigh},
        "tags": ["flux", "violent", "kinetic", "dark", "cold"],
    },
    # 2. bright kinetic dense violent
    {
        "label": "BRIGHT_KINETIC_DENSE_VIOLENT",
        "dna_fixed": {"brightness": r_xhigh, "movement": r_xhigh, "density": r_xhigh, "aggression": r_xhigh},
        "tags": ["flux", "violent", "kinetic", "bright", "dense"],
    },
    # 3. dark kinetic dense violent
    {
        "label": "DARK_KINETIC_DENSE_VIOLENT",
        "dna_fixed": {"brightness": r_xlow, "movement": r_xhigh, "density": r_xhigh, "aggression": r_xhigh},
        "tags": ["flux", "violent", "kinetic", "dark", "dense"],
    },
    # 4. bright cold intimate violent
    {
        "label": "BRIGHT_COLD_INTIMATE_VIOLENT",
        "dna_fixed": {"brightness": r_xhigh, "warmth": r_xlow, "space": r_xlow, "aggression": r_xhigh},
        "tags": ["flux", "violent", "bright", "cold", "intimate"],
    },
    # 5. hot kinetic dense violent
    {
        "label": "HOT_KINETIC_DENSE_VIOLENT",
        "dna_fixed": {"warmth": r_xhigh, "movement": r_xhigh, "density": r_xhigh, "aggression": r_xhigh},
        "tags": ["flux", "violent", "kinetic", "hot", "dense"],
    },
    # 6. dark dense vast violent
    {
        "label": "DARK_DENSE_VAST_VIOLENT",
        "dna_fixed": {"brightness": r_xlow, "density": r_xhigh, "space": r_xhigh, "aggression": r_xhigh},
        "tags": ["flux", "violent", "dark", "dense", "vast"],
    },
    # 7. bright warm kinetic violent
    {
        "label": "BRIGHT_WARM_KINETIC_VIOLENT",
        "dna_fixed": {"brightness": r_xhigh, "warmth": r_xhigh, "movement": r_xhigh, "aggression": r_xhigh},
        "tags": ["flux", "violent", "bright", "warm", "kinetic"],
    },
    # 8. cold kinetic intimate violent
    {
        "label": "COLD_KINETIC_INTIMATE_VIOLENT",
        "dna_fixed": {"warmth": r_xlow, "movement": r_xhigh, "space": r_xlow, "aggression": r_xhigh},
        "tags": ["flux", "violent", "cold", "kinetic", "intimate"],
    },
]

ENTANGLED_CORNERS = [
    # 1. bright warm dense violent
    {
        "label": "BRIGHT_WARM_DENSE_VIOLENT",
        "dna_fixed": {"brightness": r_xhigh, "warmth": r_xhigh, "density": r_xhigh, "aggression": r_xhigh},
        "tags": ["entangled", "violent", "bright", "warm", "dense"],
    },
    # 2. dark cold dense violent
    {
        "label": "DARK_COLD_DENSE_VIOLENT",
        "dna_fixed": {"brightness": r_xlow, "warmth": r_xlow, "density": r_xhigh, "aggression": r_xhigh},
        "tags": ["entangled", "violent", "dark", "cold", "dense"],
    },
    # 3. bright kinetic vast violent
    {
        "label": "BRIGHT_KINETIC_VAST_VIOLENT",
        "dna_fixed": {"brightness": r_xhigh, "movement": r_xhigh, "space": r_xhigh, "aggression": r_xhigh},
        "tags": ["entangled", "violent", "bright", "kinetic", "vast"],
    },
    # 4. dark kinetic intimate violent
    {
        "label": "DARK_KINETIC_INTIMATE_VIOLENT",
        "dna_fixed": {"brightness": r_xlow, "movement": r_xhigh, "space": r_xlow, "aggression": r_xhigh},
        "tags": ["entangled", "violent", "dark", "kinetic", "intimate"],
    },
    # 5. hot dense vast violent
    {
        "label": "HOT_DENSE_VAST_VIOLENT",
        "dna_fixed": {"warmth": r_xhigh, "density": r_xhigh, "space": r_xhigh, "aggression": r_xhigh},
        "tags": ["entangled", "violent", "hot", "dense", "vast"],
    },
    # 6. cold kinetic dense violent
    {
        "label": "COLD_KINETIC_DENSE_VIOLENT",
        "dna_fixed": {"warmth": r_xlow, "movement": r_xhigh, "density": r_xhigh, "aggression": r_xhigh},
        "tags": ["entangled", "violent", "cold", "kinetic", "dense"],
    },
    # 7. bright cold intimate violent
    {
        "label": "BRIGHT_COLD_INTIMATE_VIOLENT",
        "dna_fixed": {"brightness": r_xhigh, "warmth": r_xlow, "space": r_xlow, "aggression": r_xhigh},
        "tags": ["entangled", "violent", "bright", "cold", "intimate"],
    },
    # 8. dark hot kinetic violent
    {
        "label": "DARK_HOT_KINETIC_VIOLENT",
        "dna_fixed": {"brightness": r_xlow, "warmth": r_xhigh, "movement": r_xhigh, "aggression": r_xhigh},
        "tags": ["entangled", "violent", "dark", "hot", "kinetic"],
    },
    # 9. bright dense intimate violent
    {
        "label": "BRIGHT_DENSE_INTIMATE_VIOLENT",
        "dna_fixed": {"brightness": r_xhigh, "density": r_xhigh, "space": r_xlow, "aggression": r_xhigh},
        "tags": ["entangled", "violent", "bright", "dense", "intimate"],
    },
    # 10. dark kinetic dense violent
    {
        "label": "DARK_KINETIC_DENSE_VIOLENT",
        "dna_fixed": {"brightness": r_xlow, "movement": r_xhigh, "density": r_xhigh, "aggression": r_xhigh},
        "tags": ["entangled", "violent", "dark", "kinetic", "dense"],
    },
    # 11. hot kinetic vast violent
    {
        "label": "HOT_KINETIC_VAST_VIOLENT",
        "dna_fixed": {"warmth": r_xhigh, "movement": r_xhigh, "space": r_xhigh, "aggression": r_xhigh},
        "tags": ["entangled", "violent", "hot", "kinetic", "vast"],
    },
    # 12. bright warm kinetic violent
    {
        "label": "BRIGHT_WARM_KINETIC_VIOLENT",
        "dna_fixed": {"brightness": r_xhigh, "warmth": r_xhigh, "movement": r_xhigh, "aggression": r_xhigh},
        "tags": ["entangled", "violent", "bright", "warm", "kinetic"],
    },
]

DNA_DIMS = ["brightness", "warmth", "movement", "density", "space", "aggression"]

def build_dna(fixed_spec):
    """Build a DNA dict. fixed_spec maps dim -> value_fn. Unfixed dims get r_mid()."""
    dna = {}
    for dim in DNA_DIMS:
        if dim in fixed_spec:
            dna[dim] = fixed_spec[dim]()
        else:
            dna[dim] = r_mid()
    return dna

def pick_two_engines(pool):
    a, b = random.sample(pool, 2)
    return a, b

def build_flux_preset(corner, variant_idx):
    label = corner["label"]
    suffix = f"FLX_AGG_{variant_idx}"
    name = f"{label}_{suffix}"

    eng_a, eng_b = pick_two_engines(FLUX_ENGINES)
    dna = build_dna(corner["dna_fixed"])

    # Macro params derived loosely from DNA
    mv = dna["movement"]
    sp = dna["space"]
    agg = dna["aggression"]
    char_a = r_param()
    char_b = r_param()
    coup_val = round(random.uniform(0.75, 0.99), 3)
    coupling_type = random.choice(FLUX_COUPLING_TYPES)

    preset = {
        "name": name,
        "version": "1.0",
        "mood": "Flux",
        "engines": [eng_a, eng_b],
        "parameters": {
            eng_a: {
                "macro_character": char_a,
                "macro_movement": round(mv + random.uniform(-0.05, 0.05), 3),
                "macro_coupling": coup_val,
                "macro_space": round(sp + random.uniform(-0.05, 0.05), 3),
            },
            eng_b: {
                "macro_character": char_b,
                "macro_movement": round(mv + random.uniform(-0.05, 0.05), 3),
                "macro_coupling": round(coup_val + random.uniform(-0.04, 0.04), 3),
                "macro_space": round(sp + random.uniform(-0.05, 0.05), 3),
            },
        },
        "coupling": {
            "type": coupling_type,
            "source": eng_a,
            "target": eng_b,
            "amount": round(random.uniform(0.80, 0.99), 3),
        },
        "dna": dna,
        "macros": {
            "CHARACTER": round((char_a + char_b) / 2, 3),
            "MOVEMENT": round(mv, 3),
            "COUPLING": coup_val,
            "SPACE": round(sp, 3),
        },
        "tags": corner["tags"],
    }
    # Clamp macro params to [0, 1]
    for eng in [eng_a, eng_b]:
        for k, v in preset["parameters"][eng].items():
            preset["parameters"][eng][k] = max(0.0, min(1.0, v))
    return name, preset


def build_entangled_preset(corner, variant_idx):
    label = corner["label"]
    suffix = f"ENT_AGG_{variant_idx}"
    name = f"{label}_{suffix}"

    eng_a, eng_b = pick_two_engines(ENTANGLED_ENGINES)
    dna = build_dna(corner["dna_fixed"])

    mv = dna["movement"]
    sp = dna["space"]
    agg = dna["aggression"]
    char_a = r_param()
    char_b = r_param()
    coup_val = round(random.uniform(0.75, 0.99), 3)
    # Weight coupling toward preferred types
    coupling_type = random.choices(
        ENTANGLED_COUPLING_TYPES,
        weights=[3, 2, 2, 2, 2],
    )[0]

    preset = {
        "name": name,
        "version": "1.0",
        "mood": "Entangled",
        "engines": [eng_a, eng_b],
        "parameters": {
            eng_a: {
                "macro_character": char_a,
                "macro_movement": round(mv + random.uniform(-0.05, 0.05), 3),
                "macro_coupling": coup_val,
                "macro_space": round(sp + random.uniform(-0.05, 0.05), 3),
            },
            eng_b: {
                "macro_character": char_b,
                "macro_movement": round(mv + random.uniform(-0.05, 0.05), 3),
                "macro_coupling": round(coup_val + random.uniform(-0.04, 0.04), 3),
                "macro_space": round(sp + random.uniform(-0.05, 0.05), 3),
            },
        },
        "coupling": {
            "type": coupling_type,
            "source": eng_a,
            "target": eng_b,
            "amount": round(random.uniform(0.80, 0.99), 3),
        },
        "dna": dna,
        "macros": {
            "CHARACTER": round((char_a + char_b) / 2, 3),
            "MOVEMENT": round(mv, 3),
            "COUPLING": coup_val,
            "SPACE": round(sp, 3),
        },
        "tags": corner["tags"],
    }
    for eng in [eng_a, eng_b]:
        for k, v in preset["parameters"][eng].items():
            preset["parameters"][eng][k] = max(0.0, min(1.0, v))
    return name, preset


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FLUX_DIR = os.path.join(REPO_ROOT, "Presets", "XOmnibus", "Flux")
ENT_DIR  = os.path.join(REPO_ROOT, "Presets", "XOmnibus", "Entangled")


def main():
    flux_written = 0
    ent_written  = 0
    agg_violations = []

    # --- Flux: 8 corners × 5 variants ---
    for corner in FLUX_CORNERS:
        for v in range(1, 6):
            name, preset = build_flux_preset(corner, v)
            path = os.path.join(FLUX_DIR, f"{name}.xometa")
            write_preset(path, preset)
            flux_written += 1
            agg = preset["dna"]["aggression"]
            if agg < 0.87:
                agg_violations.append((name, agg))

    # --- Entangled: 12 corners × 5 variants ---
    for corner in ENTANGLED_CORNERS:
        for v in range(1, 6):
            name, preset = build_entangled_preset(corner, v)
            path = os.path.join(ENT_DIR, f"{name}.xometa")
            write_preset(path, preset)
            ent_written += 1
            agg = preset["dna"]["aggression"]
            if agg < 0.87:
                agg_violations.append((name, agg))

    total = flux_written + ent_written
    print(f"Flux presets written   : {flux_written}  (target 40)")
    print(f"Entangled presets written: {ent_written}  (target 60)")
    print(f"Total written          : {total}  (target 100)")
    if agg_violations:
        print(f"\nWARNING — aggression < 0.87 in {len(agg_violations)} presets:")
        for name, val in agg_violations:
            print(f"  {name}: {val}")
    else:
        print("\nAll presets: aggression >= 0.87  ✓")


if __name__ == "__main__":
    main()
