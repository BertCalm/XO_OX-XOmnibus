#!/usr/bin/env python3
"""
xpn_entangled_ultra_diverse.py
Generate 100 Entangled presets with FOUR extreme DNA dimensions each,
targeting maximum cosine distance from fleet centroid.

Output: Presets/XOmnibus/Entangled/
"""

import json
import os
import random

# ─── Configuration ───────────────────────────────────────────────────────────

OUTPUT_DIR = os.path.join(
    os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
    "Presets", "XOmnibus", "Entangled"
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
    "DRIFT", "ORBIT", "ECHO", "PULSE", "WEAVE", "MIRROR",
    "SURGE", "TANGLE", "PHASE", "FLOW", "SPARK", "BIND"
]

# DNA dimensions: B=brightness, W=warmth, A=aggression, M=movement, D=density, S=space
# XLOW: 0.04–0.11, XHIGH: 0.89–0.96, midrange: 0.40–0.60

XLOW = (0.04, 0.11)
XHIGH = (0.89, 0.96)
MID = (0.40, 0.60)

# 20 batches of 5 presets, each batch = one corner combination
# Each entry: (name_description, {dim: zone, ...})
# Dims: B=brightness, W=warmth, A=aggression, M=movement, D=density, S=space
# Extreme dims get XLOW or XHIGH; remaining 2 get MID

BATCHES = [
    # 1. dark cold violent dense
    ("DARK COLD VIOLENT DENSE", {"B": XLOW, "W": XLOW, "A": XHIGH, "D": XHIGH, "M": MID, "S": MID}),
    # 2. bright hot kinetic dense
    ("BRIGHT HOT KINETIC DENSE", {"B": XHIGH, "W": XHIGH, "M": XHIGH, "D": XHIGH, "A": MID, "S": MID}),
    # 3. dark vast kinetic violent
    ("DARK VAST KINETIC VIOLENT", {"B": XLOW, "S": XHIGH, "M": XHIGH, "A": XHIGH, "W": MID, "D": MID}),
    # 4. bright sparse gentle vast
    ("BRIGHT SPARSE GENTLE VAST", {"B": XHIGH, "D": XLOW, "A": XLOW, "S": XHIGH, "W": MID, "M": MID}),
    # 5. hot dense still violent
    ("HOT DENSE STILL VIOLENT", {"W": XHIGH, "D": XHIGH, "M": XLOW, "A": XHIGH, "B": MID, "S": MID}),
    # 6. cold intimate kinetic dark
    ("COLD INTIMATE KINETIC DARK", {"W": XLOW, "S": XLOW, "M": XHIGH, "B": XLOW, "A": MID, "D": MID}),
    # 7. bright cold dense violent
    ("BRIGHT COLD DENSE VIOLENT", {"B": XHIGH, "W": XLOW, "D": XHIGH, "A": XHIGH, "M": MID, "S": MID}),
    # 8. dark hot vast kinetic
    ("DARK HOT VAST KINETIC", {"B": XLOW, "W": XHIGH, "S": XHIGH, "M": XHIGH, "A": MID, "D": MID}),
    # 9. kinetic violent sparse vast
    ("KINETIC VIOLENT SPARSE VAST", {"M": XHIGH, "A": XHIGH, "D": XLOW, "S": XHIGH, "B": MID, "W": MID}),
    # 10. still gentle sparse vast
    ("STILL GENTLE SPARSE VAST", {"M": XLOW, "A": XLOW, "D": XLOW, "S": XHIGH, "B": MID, "W": MID}),
    # 11. dark cold sparse vast
    ("DARK COLD SPARSE VAST", {"B": XLOW, "W": XLOW, "D": XLOW, "S": XHIGH, "A": MID, "M": MID}),
    # 12. bright hot violent kinetic
    ("BRIGHT HOT VIOLENT KINETIC", {"B": XHIGH, "W": XHIGH, "A": XHIGH, "M": XHIGH, "D": MID, "S": MID}),
    # 13. cold dense kinetic intimate
    ("COLD DENSE KINETIC INTIMATE", {"W": XLOW, "D": XHIGH, "M": XHIGH, "S": XLOW, "B": MID, "A": MID}),
    # 14. violent dense hot intimate
    ("VIOLENT DENSE HOT INTIMATE", {"A": XHIGH, "D": XHIGH, "W": XHIGH, "S": XLOW, "B": MID, "M": MID}),
    # 15. dark kinetic violent vast
    ("DARK KINETIC VIOLENT VAST", {"B": XLOW, "M": XHIGH, "A": XHIGH, "S": XHIGH, "W": MID, "D": MID}),
    # 16. bright still vast sparse
    ("BRIGHT STILL VAST SPARSE", {"B": XHIGH, "M": XLOW, "S": XHIGH, "D": XLOW, "W": MID, "A": MID}),
    # 17. cold sparse vast bright
    ("COLD SPARSE VAST BRIGHT", {"W": XLOW, "D": XLOW, "S": XHIGH, "B": XHIGH, "A": MID, "M": MID}),
    # 18. hot violent kinetic bright
    ("HOT VIOLENT KINETIC BRIGHT", {"W": XHIGH, "A": XHIGH, "M": XHIGH, "B": XHIGH, "D": MID, "S": MID}),
    # 19. dark sparse gentle still
    ("DARK SPARSE GENTLE STILL", {"B": XLOW, "D": XLOW, "A": XLOW, "M": XLOW, "W": MID, "S": MID}),
    # 20. hot intimate kinetic dense
    ("HOT INTIMATE KINETIC DENSE", {"W": XHIGH, "S": XLOW, "M": XHIGH, "D": XHIGH, "B": MID, "A": MID}),
]

DIM_MAP = {
    "B": "brightness",
    "W": "warmth",
    "A": "aggression",
    "M": "movement",
    "D": "density",
    "S": "space",
}

COUPLING_INTENSITIES = ["Low", "Medium", "High", "Extreme"]

MACRO_POOLS = {
    "brightness": ["GLEAM", "SHIMMER", "RADIANCE", "FLARE", "LUSTER"],
    "warmth":     ["HEAT", "GLOW", "BURN", "EMBER", "FORGE"],
    "aggression": ["IMPACT", "FORCE", "STRIKE", "CRUSH", "SHRED"],
    "movement":   ["FLOW", "SURGE", "PULSE", "DRIFT", "SWEEP"],
    "density":    ["MASS", "WEIGHT", "PACK", "LAYER", "PRESS"],
    "space":      ["VOID", "OPEN", "FIELD", "DEPTH", "EXPANSE"],
}


def rand_in(zone):
    return round(random.uniform(zone[0], zone[1]), 4)


def make_dna(dim_zones):
    return {DIM_MAP[k]: rand_in(v) for k, v in dim_zones.items()}


def pick_macros(dna):
    # Pick 4 macros from the dominant dimensions (highest absolute deviation from 0.5)
    dims_sorted = sorted(dna.items(), key=lambda kv: abs(kv[1] - 0.5), reverse=True)
    labels = []
    for dim_name, _ in dims_sorted[:4]:
        pool = MACRO_POOLS.get(dim_name, ["MACRO"])
        labels.append(random.choice(pool))
    return labels


def engine_class_name(engine_id):
    """Convert engine ID to XO-prefixed class name used in presets."""
    # Map special cases
    SPECIAL = {
        "ODDFELIX":   "XOddFelix",
        "ODDOSCAR":   "XOddOscar",
        "OVERDUB":    "XOverdub",
        "ODYSSEY":    "XOdyssey",
        "OBLONG":     "XOblongBob",
        "OBESE":      "XObese",
        "ONSET":      "XOnset",
        "OVERWORLD":  "XOverworld",
        "OPAL":       "XOpal",
        "ORBITAL":    "XOrbital",
        "ORGANON":    "XOrganon",
        "OUROBOROS":  "XOuroboros",
        "OBSIDIAN":   "XObsidian",
        "OVERBITE":   "XOpossum",
        "ORIGAMI":    "XOrigami",
        "ORACLE":     "XOracle",
        "OBSCURA":    "XObscura",
        "OCEANIC":    "XOceanic",
        "OCELOT":     "XOcelot",
        "OPTIC":      "XOptic",
        "OBLIQUE":    "XOblique",
        "OSPREY":     "XOsprey",
        "OSTERIA":    "XOsteria",
        "OWLFISH":    "XOwlfish",
        "OHM":        "XOhm",
        "ORPHICA":    "XOrphica",
        "OBBLIGATO":  "XObbligato",
        "OTTONI":     "XOttoni",
        "OLE":        "XOle",
        "OMBRE":      "XOmbre",
        "ORCA":       "XOrca",
        "OCTOPUS":    "XOctopus",
        "OVERLAP":    "XOverlap",
        "OUTWIT":     "XOutwit",
    }
    return SPECIAL.get(engine_id, f"X{engine_id.capitalize()}")


def make_preset(name, dna, engine_id, coupling_type, coupling_intensity, batch_idx, preset_idx):
    class_name = engine_class_name(engine_id)
    macros = pick_macros(dna)
    tags = ["entangled", "ultra-diverse", coupling_type.lower(), engine_id.lower()]

    description = (
        f"Ultra-diverse Entangled preset — {name.lower()}. "
        f"Designed for {coupling_type} coupling with {class_name}."
    )

    # Minimal parameters block — just coupling level/bus as placeholder
    params = {
        class_name: {
            "couplingLevel": round(random.uniform(0.5, 1.0), 3),
            "couplingBus": batch_idx % 4,
        }
    }

    preset = {
        "schema_version": 1,
        "name": f"{name} {preset_idx}",
        "mood": "Entangled",
        "sonic_dna": dna,
        "dna": dna,
        "engines": [class_name],
        "author": "XO_OX",
        "version": "1.0.0",
        "description": description,
        "tags": tags,
        "macroLabels": macros,
        "couplingType": coupling_type,
        "couplingIntensity": coupling_intensity,
        "parameters": params,
        "coupling": {"pairs": []},
    }
    return preset


def filename_for(name, preset_idx):
    slug = name.replace(" ", "_").lower()
    return f"{slug}_{preset_idx}.xometa"


def main():
    random.seed(42)
    os.makedirs(OUTPUT_DIR, exist_ok=True)

    written = 0
    skipped = 0

    # Cycle through engines and coupling types across all 100 presets
    engine_cycle = ENGINES[:]
    coupling_cycle = COUPLING_TYPES[:]

    preset_global_idx = 0

    for batch_idx, (batch_name, dim_zones) in enumerate(BATCHES):
        for i in range(1, 6):  # 5 presets per batch
            dna = make_dna(dim_zones)
            engine_id = engine_cycle[preset_global_idx % len(engine_cycle)]
            coupling_type = coupling_cycle[preset_global_idx % len(coupling_cycle)]
            coupling_intensity = random.choice(COUPLING_INTENSITIES)

            preset = make_preset(
                name=batch_name,
                dna=dna,
                engine_id=engine_id,
                coupling_type=coupling_type,
                coupling_intensity=coupling_intensity,
                batch_idx=batch_idx,
                preset_idx=i,
            )

            fname = filename_for(batch_name, i)
            fpath = os.path.join(OUTPUT_DIR, fname)

            if os.path.exists(fpath):
                print(f"  SKIP (exists): {fname}")
                skipped += 1
            else:
                with open(fpath, "w") as f:
                    json.dump(preset, f, indent=2)
                print(f"  WROTE: {fname}")
                written += 1

            preset_global_idx += 1

    print(f"\nDone. Written: {written} | Skipped: {skipped} | Output: {OUTPUT_DIR}")


if __name__ == "__main__":
    main()
