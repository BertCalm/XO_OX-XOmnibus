#!/usr/bin/env python3
"""
xpn_entangled_family_mega_diverse.py
Generates 100 presets (50 Entangled + 50 Family) with maximum DNA diversity.
10 corners × 5 variants each per mood.
"""

import json
import os
import random

random.seed(42)

# ── DNA range helpers ────────────────────────────────────────────────────────
def xlow():
    return round(random.uniform(0.02, 0.12), 3)

def xhigh():
    return round(random.uniform(0.88, 0.99), 3)

def mid():
    return round(random.uniform(0.30, 0.70), 3)

def jitter(v, spread=0.04):
    """Small perturbation for variants — keeps XLOW/XHIGH zone."""
    lo, hi = 0.01, 0.99
    return round(max(lo, min(hi, v + random.uniform(-spread, spread))), 3)

# ── Engine pools ──────────────────────────────────────────────────────────────
ENTANGLED_ENGINES = [
    "ODDFELIX", "ODDOSCAR", "OBLONG", "OBESE", "OPAL", "ORBITAL", "ORGANON",
    "OUROBOROS", "OBSIDIAN", "ORIGAMI", "ORACLE", "OBSCURA", "OCEANIC", "OCELOT",
    "OPTIC", "OBLIQUE", "OSPREY", "OSTERIA", "OWLFISH", "OHM", "ORPHICA",
    "OBBLIGATO", "OTTONI", "OLE", "OVERDUB", "ODYSSEY", "OVERWORLD", "OVERBITE",
    "OMBRE", "ORCA", "OCTOPUS", "OVERLAP", "OUTWIT", "ONSET",
]

FAMILY_ENGINES = [
    "ODDFELIX", "ODDOSCAR", "OBLONG", "OVERDUB", "ODYSSEY", "OPAL", "ORGANON",
    "ORIGAMI", "OHM", "ORPHICA", "OBBLIGATO", "OTTONI", "OLE", "OSPREY", "OSTERIA",
]

ENTANGLED_COUPLINGS = [
    "RESONANCE_SHARE", "SPECTRAL_MORPH", "HARMONIC_FOLD",
    "CHAOS_INJECT", "VELOCITY_COUPLE",
]
FAMILY_COUPLINGS = [
    "TIMBRE_BLEND", "ENVELOPE_LINK", "PITCH_SYNC",
    "AMPLITUDE_MOD", "FILTER_MOD",
]

# ── Corner definitions ────────────────────────────────────────────────────────
# Each corner is a callable that returns a fresh dna dict.
# Dims: brightness(B), warmth(W), movement(M), density(D), space(S), aggression(A)

def make_entangled_corners():
    return [
        # 1  B+ W+ D+ S+ A+  (all-high)
        lambda: {"brightness": xhigh(), "warmth": xhigh(), "movement": mid(),
                 "density": xhigh(), "space": xhigh(), "aggression": xhigh()},
        # 2  B- W- D- S- A-  (all-low)
        lambda: {"brightness": xlow(), "warmth": xlow(), "movement": mid(),
                 "density": xlow(), "space": xlow(), "aggression": xlow()},
        # 3  B+ W- M+ D+ A+
        lambda: {"brightness": xhigh(), "warmth": xlow(), "movement": xhigh(),
                 "density": xhigh(), "space": mid(), "aggression": xhigh()},
        # 4  B- W+ M- S+ A-
        lambda: {"brightness": xlow(), "warmth": xhigh(), "movement": xlow(),
                 "density": mid(), "space": xhigh(), "aggression": xlow()},
        # 5  B+ M+ S+ A+ D-
        lambda: {"brightness": xhigh(), "warmth": mid(), "movement": xhigh(),
                 "density": xlow(), "space": xhigh(), "aggression": xhigh()},
        # 6  B- W- M+ S+ D+
        lambda: {"brightness": xlow(), "warmth": xlow(), "movement": xhigh(),
                 "density": xhigh(), "space": xhigh(), "aggression": mid()},
        # 7  W+ D+ S+ A+ M+
        lambda: {"brightness": mid(), "warmth": xhigh(), "movement": xhigh(),
                 "density": xhigh(), "space": xhigh(), "aggression": xhigh()},
        # 8  W- M- D- S- A-
        lambda: {"brightness": mid(), "warmth": xlow(), "movement": xlow(),
                 "density": xlow(), "space": xlow(), "aggression": xlow()},
        # 9  B+ W+ M+ A+ D-
        lambda: {"brightness": xhigh(), "warmth": xhigh(), "movement": xhigh(),
                 "density": xlow(), "space": mid(), "aggression": xhigh()},
        # 10 B- D+ S- A+ W+
        lambda: {"brightness": xlow(), "warmth": xhigh(), "movement": mid(),
                 "density": xhigh(), "space": xlow(), "aggression": xhigh()},
    ]

def make_family_corners():
    return [
        # 1  B+ W+ D+ S- A-
        lambda: {"brightness": xhigh(), "warmth": xhigh(), "movement": mid(),
                 "density": xhigh(), "space": xlow(), "aggression": xlow()},
        # 2  B+ W+ M- S- D+
        lambda: {"brightness": xhigh(), "warmth": xhigh(), "movement": xlow(),
                 "density": xhigh(), "space": xlow(), "aggression": mid()},
        # 3  B- W+ D+ S- A-
        lambda: {"brightness": xlow(), "warmth": xhigh(), "movement": mid(),
                 "density": xhigh(), "space": xlow(), "aggression": xlow()},
        # 4  B+ W+ S+ A- M-
        lambda: {"brightness": xhigh(), "warmth": xhigh(), "movement": xlow(),
                 "density": mid(), "space": xhigh(), "aggression": xlow()},
        # 5  W+ M- D+ S- A-
        lambda: {"brightness": mid(), "warmth": xhigh(), "movement": xlow(),
                 "density": xhigh(), "space": xlow(), "aggression": xlow()},
        # 6  B+ W+ D- S+ A-
        lambda: {"brightness": xhigh(), "warmth": xhigh(), "movement": mid(),
                 "density": xlow(), "space": xhigh(), "aggression": xlow()},
        # 7  B- W- M- D+ A-
        lambda: {"brightness": xlow(), "warmth": xlow(), "movement": xlow(),
                 "density": xhigh(), "space": mid(), "aggression": xlow()},
        # 8  B+ W+ M+ D+ A-
        lambda: {"brightness": xhigh(), "warmth": xhigh(), "movement": xhigh(),
                 "density": xhigh(), "space": mid(), "aggression": xlow()},
        # 9  W+ D+ S- A+ B-
        lambda: {"brightness": xlow(), "warmth": xhigh(), "movement": mid(),
                 "density": xhigh(), "space": xlow(), "aggression": xhigh()},
        # 10 B+ W- M- S- A-
        lambda: {"brightness": xhigh(), "warmth": xlow(), "movement": xlow(),
                 "density": mid(), "space": xlow(), "aggression": xlow()},
    ]

# ── Label helpers ─────────────────────────────────────────────────────────────
CORNER_LABELS_ENT = [
    "ALL_HIGH", "ALL_LOW", "BRIGHT_COLD_KINETIC_DENSE_VIOLENT",
    "DARK_HOT_STILL_VAST_GENTLE", "BRIGHT_KINETIC_VAST_SPARSE_VIOLENT",
    "DARK_COLD_KINETIC_VAST_DENSE", "HOT_KINETIC_DENSE_VAST_VIOLENT",
    "COLD_STILL_SPARSE_INTIMATE_GENTLE", "BRIGHT_WARM_KINETIC_SPARSE_VIOLENT",
    "DARK_HOT_DENSE_INTIMATE_VIOLENT",
]

CORNER_LABELS_FAM = [
    "BRIGHT_WARM_DENSE_INTIMATE_GENTLE", "BRIGHT_WARM_STILL_INTIMATE_DENSE",
    "DARK_HOT_DENSE_INTIMATE_GENTLE", "BRIGHT_WARM_VAST_STILL_GENTLE",
    "HOT_STILL_DENSE_INTIMATE_GENTLE", "BRIGHT_WARM_SPARSE_VAST_GENTLE",
    "DARK_COLD_STILL_DENSE_GENTLE", "BRIGHT_WARM_KINETIC_DENSE_GENTLE",
    "DARK_HOT_DENSE_INTIMATE_VIOLENT", "BRIGHT_COLD_STILL_INTIMATE_GENTLE",
]

# ── DNA → macro/param mapping ─────────────────────────────────────────────────
def dna_to_engine_params(dna):
    """Map DNA dimensions to macro_* values for a single engine."""
    return {
        "macro_character": round((dna["brightness"] + dna["warmth"]) / 2, 3),
        "macro_movement": round(dna["movement"], 3),
        "macro_coupling": round((dna["density"] + dna["aggression"]) / 2, 3),
        "macro_space": round(dna["space"], 3),
    }

def dna_to_macros(dna):
    return {
        "CHARACTER": round((dna["brightness"] + dna["warmth"]) / 2, 3),
        "MOVEMENT": round(dna["movement"], 3),
        "COUPLING": round((dna["density"] + dna["aggression"]) / 2, 3),
        "SPACE": round(dna["space"], 3),
    }

def dna_to_tags(dna, mood):
    tags = [mood.lower()]
    if dna["brightness"] >= 0.75:
        tags.append("bright")
    elif dna["brightness"] <= 0.25:
        tags.append("dark")
    if dna["warmth"] >= 0.75:
        tags.append("warm")
    elif dna["warmth"] <= 0.25:
        tags.append("cold")
    if dna["movement"] >= 0.75:
        tags.append("kinetic")
    elif dna["movement"] <= 0.25:
        tags.append("still")
    if dna["density"] >= 0.75:
        tags.append("dense")
    elif dna["density"] <= 0.25:
        tags.append("sparse")
    if dna["space"] >= 0.75:
        tags.append("vast")
    elif dna["space"] <= 0.25:
        tags.append("intimate")
    if dna["aggression"] >= 0.75:
        tags.append("violent")
    elif dna["aggression"] <= 0.25:
        tags.append("gentle")
    return tags

def variant_dna(base_dna):
    """Create a jittered variant keeping extreme values in their zones."""
    v = {}
    for k, val in base_dna.items():
        if val <= 0.12:
            # stay xlow zone
            v[k] = round(max(0.02, min(0.18, val + random.uniform(-0.04, 0.04))), 3)
        elif val >= 0.88:
            # stay xhigh zone
            v[k] = round(max(0.82, min(0.99, val + random.uniform(-0.04, 0.04))), 3)
        else:
            v[k] = round(max(0.20, min(0.80, val + random.uniform(-0.08, 0.08))), 3)
    return v

# ── Engine pair selection — ensure diversity across variants ──────────────────
def pick_engine_pair(pool, used_pairs):
    """Pick a fresh pair not in used_pairs; fallback to any pair."""
    attempts = 0
    while attempts < 200:
        engines = random.sample(pool, 2)
        pair = tuple(sorted(engines))
        if pair not in used_pairs:
            used_pairs.add(pair)
            return engines
        attempts += 1
    engines = random.sample(pool, 2)
    return engines

# ── Preset builder ────────────────────────────────────────────────────────────
def build_preset(name, mood, engines, dna, coupling_type):
    e1, e2 = engines
    # Slightly different params per engine via jitter
    dna2 = variant_dna(dna)
    params = {
        e1: dna_to_engine_params(dna),
        e2: dna_to_engine_params(dna2),
    }
    coupling_amount = round((dna["density"] + dna["aggression"]) / 2, 3)
    return {
        "name": name.replace("_", " "),
        "version": "1.0",
        "mood": mood,
        "engines": engines,
        "parameters": params,
        "coupling": {
            "type": coupling_type,
            "source": e1,
            "target": e2,
            "amount": coupling_amount,
        },
        "dna": dna,
        "macros": dna_to_macros(dna),
        "tags": dna_to_tags(dna, mood),
    }

# ── Main generation ───────────────────────────────────────────────────────────
def generate(out_dir_ent, out_dir_fam):
    os.makedirs(out_dir_ent, exist_ok=True)
    os.makedirs(out_dir_fam, exist_ok=True)

    ent_corners = make_entangled_corners()
    fam_corners = make_family_corners()

    ent_used_pairs = set()
    fam_used_pairs = set()
    ent_count = 0
    fam_count = 0

    # ── Entangled: 10 corners × 5 variants ───────────────────────────────────
    for ci, corner_fn in enumerate(ent_corners):
        label = CORNER_LABELS_ENT[ci]
        # Generate base DNA once per corner then make 5 variants
        base_dna = corner_fn()
        for vi in range(1, 6):
            dna = base_dna if vi == 1 else variant_dna(base_dna)
            name = f"{label}_ENT2_{vi}"
            engines = pick_engine_pair(ENTANGLED_ENGINES, ent_used_pairs)
            coupling = random.choice(ENTANGLED_COUPLINGS)
            preset = build_preset(name, "Entangled", engines, dna, coupling)
            fname = f"{name}.xometa"
            path = os.path.join(out_dir_ent, fname)
            with open(path, "w") as f:
                json.dump(preset, f, indent=2)
            ent_count += 1

    # ── Family: 10 corners × 5 variants ──────────────────────────────────────
    for ci, corner_fn in enumerate(fam_corners):
        label = CORNER_LABELS_FAM[ci]
        base_dna = corner_fn()
        for vi in range(1, 6):
            dna = base_dna if vi == 1 else variant_dna(base_dna)
            name = f"{label}_FAM2_{vi}"
            engines = pick_engine_pair(FAMILY_ENGINES, fam_used_pairs)
            coupling = random.choice(FAMILY_COUPLINGS)
            preset = build_preset(name, "Family", engines, dna, coupling)
            fname = f"{name}.xometa"
            path = os.path.join(out_dir_fam, fname)
            with open(path, "w") as f:
                json.dump(preset, f, indent=2)
            fam_count += 1

    print(f"Generated {ent_count} Entangled presets → {out_dir_ent}")
    print(f"Generated {fam_count} Family presets    → {out_dir_fam}")
    print(f"Total: {ent_count + fam_count}")


if __name__ == "__main__":
    repo = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    out_ent = os.path.join(repo, "Presets", "XOmnibus", "Entangled")
    out_fam = os.path.join(repo, "Presets", "XOmnibus", "Family")
    generate(out_ent, out_fam)
