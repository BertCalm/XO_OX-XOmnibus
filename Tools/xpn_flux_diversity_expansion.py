#!/usr/bin/env python3
"""
xpn_flux_diversity_expansion.py
Generate 60 .xometa Flux presets targeting DNA diversity score >= 0.20
(current: 0.1045 — lowest in fleet)

Strategy: Every preset has AT LEAST 2 DNA dimensions in extreme zones (<=0.15 or >=0.85)
"""

import json
import os
import random

OUTPUT_DIR = os.path.join(
    os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
    "Presets", "Flux"
)

ENGINES = [
    "ODDFELIX", "ODDOSCAR", "OVERDUB", "ODYSSEY", "OBLONG", "OBESE",
    "ONSET", "OVERWORLD", "OPAL", "ORBITAL", "ORGANON", "OUROBOROS",
    "OBSIDIAN", "OVERBITE", "ORIGAMI", "ORACLE", "OBSCURA", "OCEANIC",
    "OCELOT", "OPTIC", "OBLIQUE", "OSPREY", "OSTERIA", "OWLFISH",
    "OHM", "ORPHICA", "OBBLIGATO", "OTTONI", "OLE", "OMBRE",
    "ORCA", "OCTOPUS", "OVERLAP", "OUTWIT"
]

COUPLING_TYPES = [
    "FREQUENCY_SHIFT", "AMPLITUDE_MOD", "FILTER_MOD", "PITCH_SYNC",
    "TIMBRE_BLEND", "ENVELOPE_LINK", "HARMONIC_FOLD", "CHAOS_INJECT",
    "RESONANCE_SHARE", "SPATIAL_COUPLE", "SPECTRAL_MORPH", "VELOCITY_COUPLE"
]

# Preset name pools per group
GROUP_NAMES = {
    "dark_violent": [
        "DARK TORRENT", "VOID SURGE", "SHADOW FRACTURE", "BRUTAL FLUX",
        "BLACK CASCADE", "SAVAGE DRIFT", "OBSIDIAN STORM", "WRAITH CURRENT",
        "HOLLOW RAGE", "ABYSS STRIKE"
    ],
    "bright_dense": [
        "BRIGHT SURGE", "SOLAR WEAVE", "LUMINOUS SWARM", "RADIANT MASS",
        "CRYSTALLINE FLOOD", "PHOTON CLUSTER", "GLEAM TORRENT", "BLAZE LATTICE",
        "INCANDESCENT FLOW", "STELLAR DENSE"
    ],
    "hot_kinetic": [
        "THERMAL FLUX", "PLASMA SPIN", "FEVER STREAM", "IGNITION WAVE",
        "MOLTEN CURRENT", "HEAT SPIRAL", "COMBUSTION DRIFT", "INFERNAL PULSE",
        "PYRETIC FLOW", "FORGE KINETIC"
    ],
    "vast_sparse": [
        "VOID STREAM", "OPEN ETHER", "VACANT EXPANSE", "LONE DRIFT",
        "INFINITE SPARSE", "EMPTY ORBIT", "SILENT COSMOS", "HOLLOW EXPANSE",
        "BARE HORIZON", "REMOTE FLUX"
    ],
    "dark_cold_vast": [
        "CRYOGENIC VOID", "FROZEN EXPANSE", "ARCTIC DARK", "GLACIAL ABYSS",
        "COLD INFINITE", "TUNDRA STREAM", "ICE HORIZON", "SUBZERO DRIFT",
        "PERMAFROST FLUX", "DARK POLAR"
    ],
    "bright_hot_violent": [
        "SOLAR FURY", "SCORCHED SURGE", "INCENDIARY WAVE", "BLAZING ASSAULT",
        "INFERNO FLUX", "FLARE TORRENT", "NOVA STRIKE", "WHITE HEAT STORM",
        "IGNEOUS FURY", "CHROMATIC ASSAULT"
    ]
}

# Tags per group
GROUP_TAGS = {
    "dark_violent":      ["flux", "dark", "violent", "extreme"],
    "bright_dense":      ["flux", "bright", "dense", "saturated"],
    "hot_kinetic":       ["flux", "warm", "kinetic", "movement"],
    "vast_sparse":       ["flux", "spatial", "sparse", "open"],
    "dark_cold_vast":    ["flux", "dark", "cold", "vast", "extreme"],
    "bright_hot_violent":["flux", "bright", "hot", "violent", "extreme"]
}

# DNA value helpers
XLOW  = lambda: round(random.uniform(0.03, 0.15), 3)
XHIGH = lambda: round(random.uniform(0.85, 0.97), 3)
MID   = lambda: round(random.uniform(0.30, 0.70), 3)


def dna_for_group(group: str) -> dict:
    """Return DNA dict with correct extreme dimensions per group."""
    if group == "dark_violent":
        return {
            "brightness": XLOW(),
            "warmth":     MID(),
            "movement":   MID(),
            "density":    MID(),
            "space":      MID(),
            "aggression": XHIGH()
        }
    elif group == "bright_dense":
        return {
            "brightness": XHIGH(),
            "warmth":     MID(),
            "movement":   MID(),
            "density":    XHIGH(),
            "space":      MID(),
            "aggression": MID()
        }
    elif group == "hot_kinetic":
        return {
            "brightness": MID(),
            "warmth":     XHIGH(),
            "movement":   XHIGH(),
            "density":    MID(),
            "space":      MID(),
            "aggression": MID()
        }
    elif group == "vast_sparse":
        return {
            "brightness": MID(),
            "warmth":     MID(),
            "movement":   MID(),
            "density":    XLOW(),
            "space":      XHIGH(),
            "aggression": MID()
        }
    elif group == "dark_cold_vast":
        return {
            "brightness": XLOW(),
            "warmth":     XLOW(),
            "movement":   MID(),
            "density":    MID(),
            "space":      XHIGH(),
            "aggression": MID()
        }
    elif group == "bright_hot_violent":
        return {
            "brightness": XHIGH(),
            "warmth":     XHIGH(),
            "movement":   MID(),
            "density":    MID(),
            "space":      MID(),
            "aggression": XHIGH()
        }
    raise ValueError(f"Unknown group: {group}")


def engine_params(dna: dict) -> dict:
    """Derive macro values loosely from DNA."""
    return {
        "macro_character": round(dna["aggression"] * 0.6 + dna["brightness"] * 0.4, 3),
        "macro_movement":  round(dna["movement"] * 0.7 + dna["density"] * 0.3, 3),
        "macro_coupling":  round(random.uniform(0.4, 1.0), 3),
        "macro_space":     round(dna["space"] * 0.7 + (1 - dna["density"]) * 0.3, 3)
    }


def build_preset(name: str, engine1: str, engine2: str, coupling_type: str,
                 dna: dict, tags: list) -> dict:
    ep1 = engine_params(dna)
    ep2 = {k: round(min(1.0, max(0.0, v + random.uniform(-0.15, 0.15))), 3)
           for k, v in ep1.items()}
    coupling_amount = round(random.uniform(0.5, 1.0), 3)
    return {
        "name": name,
        "version": "1.0",
        "mood": "Flux",
        "engines": [engine1, engine2],
        "parameters": {
            engine1: ep1,
            engine2: ep2
        },
        "coupling": {
            "type": coupling_type,
            "source": engine1,
            "target": engine2,
            "amount": coupling_amount
        },
        "dna": dna,
        "macros": {
            "CHARACTER": ep1["macro_character"],
            "MOVEMENT":  ep1["macro_movement"],
            "COUPLING":  ep1["macro_coupling"],
            "SPACE":     ep1["macro_space"]
        },
        "tags": tags
    }


def main():
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    random.seed(42)

    # Build engine pair pool — cycle through all 34 engines evenly
    # 60 presets × 2 engines = 120 slots; shuffle and pair
    engine_pool = ENGINES * 4          # 136 entries
    random.shuffle(engine_pool)

    # Ensure no preset uses the same engine twice
    pairs = []
    used = []
    i = 0
    while len(pairs) < 60:
        e1 = engine_pool[i % len(engine_pool)]
        i += 1
        e2 = engine_pool[i % len(engine_pool)]
        i += 1
        if e1 != e2:
            pairs.append((e1, e2))

    # Distribute coupling types: 60 presets / 12 types = 5 each
    couplings = (COUPLING_TYPES * 5)
    random.shuffle(couplings)

    groups = [
        ("dark_violent",       10),
        ("bright_dense",       10),
        ("hot_kinetic",        10),
        ("vast_sparse",        10),
        ("dark_cold_vast",     10),
        ("bright_hot_violent", 10),
    ]

    written = 0
    skipped = 0
    preset_idx = 0

    for group, count in groups:
        names    = GROUP_NAMES[group]
        tags     = GROUP_TAGS[group]
        for j in range(count):
            name         = names[j]
            engine1, engine2 = pairs[preset_idx]
            coupling_type    = couplings[preset_idx]
            dna              = dna_for_group(group)
            preset           = build_preset(name, engine1, engine2, coupling_type, dna, tags)

            safe_name = name.replace(" ", "_").replace("/", "-")
            filepath  = os.path.join(OUTPUT_DIR, f"{safe_name}.xometa")

            if os.path.exists(filepath):
                print(f"  SKIP (exists): {safe_name}.xometa")
                skipped += 1
            else:
                with open(filepath, "w") as f:
                    json.dump(preset, f, indent=2)
                print(f"  WRITE: {safe_name}.xometa  [{group}]  engines={engine1}+{engine2}  coupling={coupling_type}")
                written += 1

            preset_idx += 1

    print(f"\n{'='*60}")
    print(f"Flux DNA Expansion complete.")
    print(f"  Written : {written}")
    print(f"  Skipped : {skipped}")
    print(f"  Output  : {OUTPUT_DIR}")
    print(f"\nExtreme-dimension coverage:")
    for group, _ in groups:
        print(f"  {group:<22} — 10 presets")


if __name__ == "__main__":
    main()
