#!/usr/bin/env python3
"""
Generate 80 Entangled movement-XLOW presets (ENT4 suffix).
16 corners × 5 variants = 80 presets, all with movement ≤ 0.11.
Save to Presets/XOceanus/Entangled/
"""

import json
import os
import random

random.seed(42)

ENGINES = [
    "ODDFELIX", "ODDOSCAR", "OBLONG", "OBESE", "OPAL", "ORBITAL", "ORGANON",
    "OUROBOROS", "OBSIDIAN", "ORIGAMI", "ORACLE", "OBSCURA", "OCEANIC", "OCELOT",
    "OPTIC", "OBLIQUE", "OSPREY", "OSTERIA", "OWLFISH", "OHM", "ORPHICA",
    "OBBLIGATO", "OTTONI", "OLE", "OVERDUB", "ODYSSEY", "OVERWORLD", "OVERBITE",
    "OMBRE", "ORCA", "OCTOPUS", "OVERLAP", "OUTWIT", "ONSET"
]

COUPLING_TYPES = [
    "RESONANCE_SHARE", "SPECTRAL_MORPH", "PITCH_SYNC", "HARMONIC_FOLD", "TIMBRE_BLEND"
]
# Weighted toward static/frozen
COUPLING_WEIGHTS = [3, 2, 2, 2, 1]

def xlow_movement():
    return round(random.uniform(0.01, 0.10), 3)

def xlow():
    return round(random.uniform(0.02, 0.13), 3)

def xhigh():
    return round(random.uniform(0.87, 0.99), 3)

def mid():
    return round(random.uniform(0.3, 0.7), 3)

def jitter(val, amount=0.03):
    return round(max(0.01, min(0.99, val + random.uniform(-amount, amount))), 3)

# Each corner: dict with brightness, warmth, movement, density, space, aggression
# movement is always XLOW; extremes defined per corner
CORNERS = [
    # 1. brightness-XHIGH + warmth-XHIGH + movement-XLOW + density-XHIGH + space-XHIGH
    {"name": "BRIGHT_WARM_FROZEN_DENSE_VAST",
     "dims": {"brightness": "H", "warmth": "H", "movement": "XL", "density": "H", "space": "H", "aggression": "M"}},
    # 2. brightness-XLOW + warmth-XLOW + movement-XLOW + density-XLOW + aggression-XLOW
    {"name": "DARK_COLD_FROZEN_SPARSE_CALM",
     "dims": {"brightness": "L", "warmth": "L", "movement": "XL", "density": "L", "space": "M", "aggression": "L"}},
    # 3. brightness-XHIGH + warmth-XLOW + movement-XLOW + space-XHIGH + aggression-XHIGH
    {"name": "BRIGHT_COLD_FROZEN_VAST_HARSH",
     "dims": {"brightness": "H", "warmth": "L", "movement": "XL", "density": "M", "space": "H", "aggression": "H"}},
    # 4. brightness-XLOW + warmth-XHIGH + movement-XLOW + density-XHIGH + aggression-XLOW
    {"name": "DARK_WARM_FROZEN_DENSE_GENTLE",
     "dims": {"brightness": "L", "warmth": "H", "movement": "XL", "density": "H", "space": "M", "aggression": "L"}},
    # 5. brightness-XHIGH + movement-XLOW + density-XHIGH + space-XLOW + aggression-XHIGH
    {"name": "BRIGHT_FROZEN_DENSE_CLOSE_HARSH",
     "dims": {"brightness": "H", "warmth": "M", "movement": "XL", "density": "H", "space": "L", "aggression": "H"}},
    # 6. brightness-XLOW + movement-XLOW + space-XHIGH + aggression-XHIGH + density-XLOW
    {"name": "DARK_FROZEN_VAST_HARSH_SPARSE",
     "dims": {"brightness": "L", "warmth": "M", "movement": "XL", "density": "L", "space": "H", "aggression": "H"}},
    # 7. warmth-XHIGH + movement-XLOW + density-XHIGH + space-XHIGH + aggression-XHIGH
    {"name": "WARM_FROZEN_DENSE_VAST_HARSH",
     "dims": {"brightness": "M", "warmth": "H", "movement": "XL", "density": "H", "space": "H", "aggression": "H"}},
    # 8. warmth-XLOW + movement-XLOW + density-XLOW + space-XLOW + aggression-XLOW
    {"name": "COLD_FROZEN_SPARSE_CLOSE_CALM",
     "dims": {"brightness": "M", "warmth": "L", "movement": "XL", "density": "L", "space": "L", "aggression": "L"}},
    # 9. brightness-XHIGH + warmth-XHIGH + movement-XLOW + aggression-XHIGH + density-XLOW
    {"name": "BRIGHT_WARM_FROZEN_HARSH_SPARSE",
     "dims": {"brightness": "H", "warmth": "H", "movement": "XL", "density": "L", "space": "M", "aggression": "H"}},
    # 10. brightness-XLOW + warmth-XLOW + movement-XLOW + space-XHIGH + aggression-XHIGH
    {"name": "DARK_COLD_FROZEN_VAST_HARSH",
     "dims": {"brightness": "L", "warmth": "L", "movement": "XL", "density": "M", "space": "H", "aggression": "H"}},
    # 11. brightness-XHIGH + movement-XLOW + space-XLOW + aggression-XHIGH + warmth-XHIGH
    {"name": "BRIGHT_WARM_FROZEN_CLOSE_HARSH",
     "dims": {"brightness": "H", "warmth": "H", "movement": "XL", "density": "M", "space": "L", "aggression": "H"}},
    # 12. brightness-XLOW + warmth-XHIGH + movement-XLOW + space-XLOW + aggression-XHIGH
    {"name": "DARK_WARM_FROZEN_CLOSE_HARSH",
     "dims": {"brightness": "L", "warmth": "H", "movement": "XL", "density": "M", "space": "L", "aggression": "H"}},
    # 13. brightness-XHIGH + warmth-XLOW + movement-XLOW + density-XHIGH + space-XLOW
    {"name": "BRIGHT_COLD_FROZEN_DENSE_CLOSE",
     "dims": {"brightness": "H", "warmth": "L", "movement": "XL", "density": "H", "space": "L", "aggression": "M"}},
    # 14. warmth-XHIGH + movement-XLOW + density-XLOW + space-XHIGH + aggression-XLOW
    {"name": "WARM_FROZEN_SPARSE_VAST_GENTLE",
     "dims": {"brightness": "M", "warmth": "H", "movement": "XL", "density": "L", "space": "H", "aggression": "L"}},
    # 15. brightness-XLOW + movement-XLOW + density-XHIGH + space-XHIGH + aggression-XHIGH
    {"name": "DARK_FROZEN_DENSE_VAST_HARSH",
     "dims": {"brightness": "L", "warmth": "M", "movement": "XL", "density": "H", "space": "H", "aggression": "H"}},
    # 16. brightness-XHIGH + warmth-XHIGH + movement-XLOW + space-XHIGH + aggression-XLOW
    {"name": "BRIGHT_WARM_FROZEN_VAST_GENTLE",
     "dims": {"brightness": "H", "warmth": "H", "movement": "XL", "density": "M", "space": "H", "aggression": "L"}},
]

def resolve_dim(code):
    if code == "XL":
        return xlow_movement()
    elif code == "L":
        return xlow()
    elif code == "H":
        return xhigh()
    else:
        return mid()

def tags_for_dna(dna):
    tags = ["entangled", "frozen", "movement-xlow"]
    if dna["brightness"] >= 0.87:
        tags.append("bright")
    elif dna["brightness"] <= 0.13:
        tags.append("dark")
    if dna["warmth"] >= 0.87:
        tags.append("warm")
    elif dna["warmth"] <= 0.13:
        tags.append("cold")
    if dna["density"] >= 0.87:
        tags.append("dense")
    elif dna["density"] <= 0.13:
        tags.append("sparse")
    if dna["space"] >= 0.87:
        tags.append("vast")
    elif dna["space"] <= 0.13:
        tags.append("close")
    if dna["aggression"] >= 0.87:
        tags.append("harsh")
    elif dna["aggression"] <= 0.13:
        tags.append("gentle")
    return tags

def pick_engines():
    e1, e2 = random.sample(ENGINES, 2)
    return e1, e2

def pick_coupling(e1, e2):
    ctype = random.choices(COUPLING_TYPES, weights=COUPLING_WEIGHTS, k=1)[0]
    return {"type": ctype, "source": e1, "target": e2, "amount": round(random.uniform(0.7, 0.98), 3)}

def make_preset(corner, variant_idx):
    dims = corner["dims"]
    dna = {k: resolve_dim(v) for k, v in dims.items()}
    # Ensure movement is strictly <= 0.11
    dna["movement"] = min(dna["movement"], 0.10)

    e1, e2 = pick_engines()
    coupling = pick_coupling(e1, e2)

    # Engine macro_movement mirrors DNA movement; character/coupling/space from relevant DNA dims
    def engine_params(brightness, warmth, space):
        return {
            "macro_character": jitter((brightness + warmth) / 2),
            "macro_movement": jitter(dna["movement"], 0.01),
            "macro_coupling": jitter(dna["density"]),
            "macro_space": jitter(space),
        }

    params = {
        e1: engine_params(dna["brightness"], dna["warmth"], dna["space"]),
        e2: engine_params(dna["brightness"], dna["warmth"], dna["space"]),
    }
    # Clamp macro_movement
    for eng in [e1, e2]:
        params[eng]["macro_movement"] = min(params[eng]["macro_movement"], 0.11)

    macros = {
        "CHARACTER": round((params[e1]["macro_character"] + params[e2]["macro_character"]) / 2, 3),
        "MOVEMENT": min(round((params[e1]["macro_movement"] + params[e2]["macro_movement"]) / 2, 3), 0.11),
        "COUPLING": round(coupling["amount"], 3),
        "SPACE": round((params[e1]["macro_space"] + params[e2]["macro_space"]) / 2, 3),
    }

    name = f"{corner['name']}_ENT4_{variant_idx}"
    return {
        "name": name,
        "version": "1.0",
        "mood": "Entangled",
        "engines": [e1, e2],
        "parameters": params,
        "coupling": coupling,
        "dna": dna,
        "macros": macros,
        "tags": tags_for_dna(dna),
    }

def main():
    out_dir = os.path.join(
        os.path.dirname(__file__), "..", "Presets", "XOceanus", "Entangled"
    )
    out_dir = os.path.abspath(out_dir)
    os.makedirs(out_dir, exist_ok=True)

    generated = []
    for corner in CORNERS:
        for v in range(1, 6):
            preset = make_preset(corner, v)
            filename = f"{preset['name']}.xometa"
            filepath = os.path.join(out_dir, filename)
            with open(filepath, "w") as f:
                json.dump(preset, f, indent=2)
            generated.append((filename, preset["dna"]["movement"]))

    # Verify
    violations = [(fn, mv) for fn, mv in generated if mv > 0.11]
    print(f"Generated: {len(generated)} presets")
    print(f"Movement violations (> 0.11): {len(violations)}")
    if violations:
        for fn, mv in violations:
            print(f"  VIOLATION: {fn} movement={mv}")
    else:
        print("All presets have movement <= 0.11 — PASS")

    # Confirm ENT4 files in dir
    ent4_files = [f for f in os.listdir(out_dir) if "ENT4" in f and f.endswith(".xometa")]
    print(f"ENT4 files in {out_dir}: {len(ent4_files)}")

if __name__ == "__main__":
    main()
