#!/usr/bin/env python3
"""
xpn_entangled_hot_warmth_pack.py
Generate 80 scorching-warm Entangled presets (warmth >= 0.86) across 4 sub-groups.
Saves to Presets/XOmnibus/Entangled/. Skips if file exists.
"""

import json
import os
import random

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUTPUT_DIR = os.path.join(REPO_ROOT, "Presets", "XOmnibus", "Entangled")

random.seed(42)

# ---------------------------------------------------------------------------
# Coupling types used for Entangled presets
# ---------------------------------------------------------------------------
COUPLING_TYPES = [
    "TIMBRE_BLEND",
    "RHYTHM_LOCK",
    "PITCH_FOLLOW",
    "SPECTRAL_BRAID",
    "ENVELOPE_MIRROR",
    "HARMONIC_LOCK",
    "DYNAMICS_SHARE",
    "PHASE_SYNC",
]


def rng(lo, hi, decimals=2):
    return round(random.uniform(lo, hi), decimals)


def make_engine_params(character, movement, coupling, space):
    return {
        "macro_character": round(character, 2),
        "macro_movement": round(movement, 2),
        "macro_coupling": round(coupling, 2),
        "macro_space": round(space, 2),
    }


def make_preset(
    name,
    engines,
    dna,
    coupling_type=None,
    tags=None,
):
    """Build a complete .xometa preset dict."""
    eng_a, eng_b = engines
    coupling_type = coupling_type or random.choice(COUPLING_TYPES)
    coupling_amount = rng(0.55, 0.90)

    # Derive per-engine macros loosely from DNA
    a_char = rng(max(0.4, dna["warmth"] - 0.25), min(1.0, dna["warmth"] + 0.05))
    a_mov = rng(max(0.2, dna["movement"] - 0.15), min(1.0, dna["movement"] + 0.15))
    a_coup = rng(max(0.4, coupling_amount - 0.15), min(1.0, coupling_amount + 0.1))
    a_space = rng(max(0.2, dna["space"] - 0.15), min(1.0, dna["space"] + 0.15))

    b_char = rng(max(0.4, dna["warmth"] - 0.30), min(1.0, dna["warmth"] - 0.05))
    b_mov = rng(max(0.2, dna["movement"] - 0.20), min(1.0, dna["movement"] + 0.10))
    b_coup = rng(max(0.35, coupling_amount - 0.20), min(1.0, coupling_amount + 0.05))
    b_space = rng(max(0.2, dna["space"] - 0.20), min(1.0, dna["space"] + 0.10))

    macro_character = round((a_char + b_char) / 2, 2)
    macro_movement = round((a_mov + b_mov) / 2, 2)
    macro_coupling = round((a_coup + b_coup) / 2, 2)
    macro_space = round((a_space + b_space) / 2, 2)

    preset = {
        "name": name,
        "version": "1.0",
        "mood": "Entangled",
        "engines": [eng_a, eng_b],
        "parameters": {
            eng_a: make_engine_params(a_char, a_mov, a_coup, a_space),
            eng_b: make_engine_params(b_char, b_mov, b_coup, b_space),
        },
        "coupling": {
            "type": coupling_type,
            "source": eng_a,
            "target": eng_b,
            "amount": coupling_amount,
        },
        "dna": dna,
        "macros": {
            "CHARACTER": macro_character,
            "MOVEMENT": macro_movement,
            "COUPLING": macro_coupling,
            "SPACE": macro_space,
        },
        "tags": tags or ["entangled", "warm", "scorching"],
    }
    return preset


# ---------------------------------------------------------------------------
# Sub-group definitions
# ---------------------------------------------------------------------------

# --- scorching-dense (20) ---
# warmth 0.88-0.98, density 0.75-0.95, brightness 0.35-0.65
DENSE_PAIRS = [
    ("OVERDUB", "OBLONG"),
    ("ODDOSCAR", "ORGANON"),
    ("OBESE", "ORGANON"),
    ("OVERDUB", "OSPREY"),
    ("ODDOSCAR", "OPAL"),
    ("OBLONG", "ORGANON"),
    ("OBESE", "OBLONG"),
    ("OVERDUB", "ORGANON"),
    ("OBLONG", "OSPREY"),
    ("ODDOSCAR", "OBESE"),
]

DENSE_NAMES = [
    "Scorched Marrow",
    "Furnace Knot",
    "Molten Lattice",
    "Ember Tangle",
    "Cinder Mesh",
    "Magma Weave",
    "Hot Iron Braid",
    "Smoldering Wick",
    "Forge Entangle",
    "Combustion Core",
    "Thermal Dense",
    "Volcanic Knit",
    "Charcoal Braid",
    "Burning Mantle",
    "Incandescent Mass",
    "Pyroclastic Flow",
    "Annealing Chamber",
    "Smelter's Knot",
    "Briquette Mesh",
    "Ignis Weave",
]


def gen_dense():
    presets = []
    for i in range(20):
        pair = DENSE_PAIRS[i % len(DENSE_PAIRS)]
        dna = {
            "brightness": rng(0.35, 0.65),
            "warmth": rng(0.88, 0.98),
            "movement": rng(0.25, 0.55),
            "density": rng(0.75, 0.95),
            "space": rng(0.30, 0.60),
            "aggression": rng(0.20, 0.55),
        }
        presets.append(
            make_preset(
                name=DENSE_NAMES[i],
                engines=pair,
                dna=dna,
                tags=["entangled", "warm", "scorching", "dense"],
            )
        )
    return presets


# --- scorching-bright (20) ---
# warmth 0.86-0.95, brightness 0.60-0.85, movement 0.40-0.70
BRIGHT_PAIRS = [
    ("ORBITAL", "ORACLE"),
    ("ORIGAMI", "OHM"),
    ("OTTONI", "ORACLE"),
    ("ORBITAL", "OBBLIGATO"),
    ("OHM", "ORACLE"),
    ("ORIGAMI", "ORACLE"),
    ("ORBITAL", "OHM"),
    ("OTTONI", "OHM"),
    ("ORIGAMI", "OTTONI"),
    ("OHM", "OBBLIGATO"),
]

BRIGHT_NAMES = [
    "Solar Filament",
    "Sunspot Tangle",
    "Radiant Braid",
    "Luminous Knot",
    "Corona Weave",
    "Gleaming Lattice",
    "Incandescent Arc",
    "Photon Mesh",
    "White-Hot Filament",
    "Aureate Entangle",
    "Torch Braid",
    "Blazing Orbit",
    "Stellar Fuse",
    "Helioscorch",
    "Amber Cascade",
    "Candescent Drift",
    "Gilded Weave",
    "Scorch Prism",
    "Ignite Bright",
    "Aurelius Knot",
]


def gen_bright():
    presets = []
    for i in range(20):
        pair = BRIGHT_PAIRS[i % len(BRIGHT_PAIRS)]
        dna = {
            "brightness": rng(0.60, 0.85),
            "warmth": rng(0.86, 0.95),
            "movement": rng(0.40, 0.70),
            "density": rng(0.40, 0.70),
            "space": rng(0.35, 0.65),
            "aggression": rng(0.15, 0.50),
        }
        presets.append(
            make_preset(
                name=BRIGHT_NAMES[i],
                engines=pair,
                dna=dna,
                tags=["entangled", "warm", "scorching", "bright"],
            )
        )
    return presets


# --- scorching-aggressive (20) ---
# warmth 0.87-0.97, aggression 0.65-0.88, density 0.65-0.88
AGGRESSIVE_PAIRS = [
    ("OUROBOROS", "OBESE"),
    ("OVERBITE", "OUROBOROS"),
    ("OBLONG", "OUROBOROS"),
    ("ORCA", "OBLONG"),
    ("ORCA", "OBESE"),
    ("OUROBOROS", "ORCA"),
    ("OVERBITE", "OBESE"),
    ("OVERBITE", "OBLONG"),
    ("ORCA", "OUROBOROS"),
    ("OBLONG", "OBESE"),
]

AGGRESSIVE_NAMES = [
    "Burning Jaw",
    "Scorch Fang",
    "Thermal Strike",
    "Molten Bite",
    "Forge Predator",
    "Hot Venom",
    "Searing Coil",
    "Pyroclastic Bite",
    "Branded Aggressor",
    "Ignite Ouroboros",
    "Char Predator",
    "Thermal Spine",
    "Iron Hunger",
    "Combustion Orca",
    "Blaze Circuit",
    "Furnace Serpent",
    "Raging Knot",
    "Infernal Loop",
    "Scorched Fangs",
    "Obsidian Strike",
]


def gen_aggressive():
    presets = []
    for i in range(20):
        pair = AGGRESSIVE_PAIRS[i % len(AGGRESSIVE_PAIRS)]
        dna = {
            "brightness": rng(0.35, 0.65),
            "warmth": rng(0.87, 0.97),
            "movement": rng(0.35, 0.65),
            "density": rng(0.65, 0.88),
            "space": rng(0.25, 0.55),
            "aggression": rng(0.65, 0.88),
        }
        presets.append(
            make_preset(
                name=AGGRESSIVE_NAMES[i],
                engines=pair,
                dna=dna,
                tags=["entangled", "warm", "scorching", "aggressive"],
            )
        )
    return presets


# --- scorching-spatial (20) ---
# warmth 0.88-0.96, space 0.60-0.85, movement 0.50-0.75
SPATIAL_PAIRS = [
    ("OVERDUB", "OPAL"),
    ("OPAL", "ORGANON"),
    ("ODYSSEY", "ORGANON"),
    ("OVERDUB", "ODYSSEY"),
    ("ORGANON", "OSPREY"),
    ("OPAL", "ODYSSEY"),
    ("OVERDUB", "ORGANON"),
    ("ODYSSEY", "OSPREY"),
    ("OPAL", "OSPREY"),
    ("OVERDUB", "OPAL"),
]

SPATIAL_NAMES = [
    "Warm Expanse",
    "Thermal Horizon",
    "Scorched Orbit",
    "Blazing Atmosphere",
    "Sunward Drift",
    "Radiant Passage",
    "Hot Void Braid",
    "Incandescent Room",
    "Ember Sky",
    "Pyrosphere",
    "Thermal Cathedral",
    "Warm Reverb Knot",
    "Hot Aether Weave",
    "Scorched Reverb",
    "Solar Wind Trail",
    "Ignis Expanse",
    "Cinder Reverb",
    "Warm Cosmos Braid",
    "Thermal Diffusion",
    "Scorched Passage",
]


def gen_spatial():
    presets = []
    for i in range(20):
        pair = SPATIAL_PAIRS[i % len(SPATIAL_PAIRS)]
        dna = {
            "brightness": rng(0.40, 0.70),
            "warmth": rng(0.88, 0.96),
            "movement": rng(0.50, 0.75),
            "density": rng(0.35, 0.65),
            "space": rng(0.60, 0.85),
            "aggression": rng(0.15, 0.45),
        }
        presets.append(
            make_preset(
                name=SPATIAL_NAMES[i],
                engines=pair,
                dna=dna,
                tags=["entangled", "warm", "scorching", "spatial"],
            )
        )
    return presets


# ---------------------------------------------------------------------------
# Write presets
# ---------------------------------------------------------------------------

def sanitize_filename(name):
    return name.replace("/", "-").replace("\\", "-").replace(":", "-").replace(" ", "_")


def main():
    os.makedirs(OUTPUT_DIR, exist_ok=True)

    all_presets = (
        gen_dense()
        + gen_bright()
        + gen_aggressive()
        + gen_spatial()
    )

    written = 0
    skipped = 0

    for preset in all_presets:
        filename = sanitize_filename(preset["name"]) + ".xometa"
        filepath = os.path.join(OUTPUT_DIR, filename)
        if os.path.exists(filepath):
            print(f"  SKIP (exists): {filename}")
            skipped += 1
            continue
        with open(filepath, "w") as f:
            json.dump(preset, f, indent=2)
        print(f"  WRITE: {filename}  warmth={preset['dna']['warmth']}")
        written += 1

    print(f"\nDone. Written: {written}  Skipped: {skipped}  Total attempted: {len(all_presets)}")

    # Validation summary
    print("\n--- DNA Validation ---")
    groups = {
        "scorching-dense": all_presets[0:20],
        "scorching-bright": all_presets[20:40],
        "scorching-aggressive": all_presets[40:60],
        "scorching-spatial": all_presets[60:80],
    }
    for group_name, group in groups.items():
        warmths = [p["dna"]["warmth"] for p in group]
        print(f"{group_name}: warmth min={min(warmths):.2f} max={max(warmths):.2f} avg={sum(warmths)/len(warmths):.2f}")
        violations = [p for p in group if p["dna"]["warmth"] < 0.86]
        if violations:
            print(f"  VIOLATIONS (<0.86): {[p['name'] for p in violations]}")
        else:
            print(f"  All warmth >= 0.86 ✓")


if __name__ == "__main__":
    main()
