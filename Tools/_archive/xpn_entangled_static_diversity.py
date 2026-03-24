#!/usr/bin/env python3
"""
xpn_entangled_static_diversity.py

Generate 100 Entangled presets with movement-XLOW (≤ 0.13) and extreme
DNA vectors to push cosine diversity higher in the fleet.

20 corners × 5 variants = 100 presets, all saved to
Presets/XOlokun/Entangled/ with ENT3 suffix naming.
"""

import json
import os
import random
from pathlib import Path

random.seed(42)

# ---------------------------------------------------------------------------
# Config
# ---------------------------------------------------------------------------

SCRIPT_DIR = Path(__file__).resolve().parent
REPO_ROOT = SCRIPT_DIR.parent
OUTPUT_DIR = REPO_ROOT / "Presets" / "XOlokun" / "Entangled"
OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

TODAY = "2026-03-16"

ALL_ENGINES = [
    "ODDFELIX", "ODDOSCAR", "OBLONG", "OBESE", "OPAL", "ORBITAL",
    "ORGANON", "OUROBOROS", "OBSIDIAN", "ORIGAMI", "ORACLE", "OBSCURA",
    "OCEANIC", "OCELOT", "OPTIC", "OBLIQUE", "OSPREY", "OSTERIA",
    "OWLFISH", "OHM", "ORPHICA", "OBBLIGATO", "OTTONI", "OLE",
    "OVERDUB", "ODYSSEY", "OVERWORLD", "OVERBITE", "OMBRE", "ORCA",
    "OCTOPUS", "OVERLAP", "OUTWIT", "ONSET",
]

# Coupling types weighted toward frozen-resonance aesthetic
COUPLING_POOL = [
    "RESONANCE_SHARE", "RESONANCE_SHARE", "RESONANCE_SHARE",
    "SPECTRAL_MORPH",  "SPECTRAL_MORPH",  "SPECTRAL_MORPH",
    "HARMONIC_FOLD",   "HARMONIC_FOLD",
    "VELOCITY_COUPLE", "VELOCITY_COUPLE",
    "PITCH_SYNC",      "PITCH_SYNC",
]

# Value range helpers
def xlow_movement():
    """0.01–0.11 — strictly ≤ 0.13"""
    return round(random.uniform(0.01, 0.11), 4)

def xlow():
    return round(random.uniform(0.02, 0.13), 4)

def xhigh():
    return round(random.uniform(0.87, 0.99), 4)

def mid():
    return round(random.uniform(0.30, 0.70), 4)

# ---------------------------------------------------------------------------
# 20 corner DNA blueprints
# Each entry: dict of dna dimension -> callable for that value
# Dimensions: brightness, warmth, movement, density, space, aggression
# ---------------------------------------------------------------------------

CORNERS = [
    # 1. all-warm frozen
    {"label": "BRIGHT_WARM_FROZEN_DENSE_VAST",
     "dna": {"brightness": xhigh, "warmth": xhigh, "movement": xlow_movement,
             "density": xhigh, "space": xhigh, "aggression": mid}},
    # 2. dark cold frozen dense intimate
    {"label": "DARK_COLD_FROZEN_DENSE_INTIMATE",
     "dna": {"brightness": xlow, "warmth": xlow, "movement": xlow_movement,
             "density": xhigh, "space": xlow, "aggression": mid}},
    # 3. bright cold frozen violent vast
    {"label": "BRIGHT_COLD_FROZEN_VIOLENT_VAST",
     "dna": {"brightness": xhigh, "warmth": xlow, "movement": xlow_movement,
             "density": mid, "space": xhigh, "aggression": xhigh}},
    # 4. dark hot frozen sparse gentle
    {"label": "DARK_HOT_FROZEN_SPARSE_GENTLE",
     "dna": {"brightness": xlow, "warmth": xhigh, "movement": xlow_movement,
             "density": xlow, "space": mid, "aggression": xlow}},
    # 5. bright still sparse vast gentle
    {"label": "BRIGHT_STILL_SPARSE_VAST_GENTLE",
     "dna": {"brightness": xhigh, "warmth": mid, "movement": xlow_movement,
             "density": xlow, "space": xhigh, "aggression": xlow}},
    # 6. dark still dense vast violent
    {"label": "DARK_STILL_DENSE_VAST_VIOLENT",
     "dna": {"brightness": xlow, "warmth": mid, "movement": xlow_movement,
             "density": xhigh, "space": xhigh, "aggression": xhigh}},
    # 7. hot frozen dense intimate violent
    {"label": "HOT_FROZEN_DENSE_INTIMATE_VIOLENT",
     "dna": {"brightness": mid, "warmth": xhigh, "movement": xlow_movement,
             "density": xhigh, "space": xlow, "aggression": xhigh}},
    # 8. cold frozen sparse intimate gentle
    {"label": "COLD_FROZEN_SPARSE_INTIMATE_GENTLE",
     "dna": {"brightness": mid, "warmth": xlow, "movement": xlow_movement,
             "density": xlow, "space": xlow, "aggression": xlow}},
    # 9. bright warm frozen intimate violent
    {"label": "BRIGHT_WARM_FROZEN_INTIMATE_VIOLENT",
     "dna": {"brightness": xhigh, "warmth": xhigh, "movement": xlow_movement,
             "density": mid, "space": xlow, "aggression": xhigh}},
    # 10. dark cold frozen sparse violent
    {"label": "DARK_COLD_FROZEN_SPARSE_VIOLENT",
     "dna": {"brightness": xlow, "warmth": xlow, "movement": xlow_movement,
             "density": xlow, "space": mid, "aggression": xhigh}},
    # 11. bright hot frozen dense violent
    {"label": "BRIGHT_HOT_FROZEN_DENSE_VIOLENT",
     "dna": {"brightness": xhigh, "warmth": xhigh, "movement": xlow_movement,
             "density": xhigh, "space": mid, "aggression": xhigh}},
    # 12. dark hot frozen vast gentle
    {"label": "DARK_HOT_FROZEN_VAST_GENTLE",
     "dna": {"brightness": xlow, "warmth": xhigh, "movement": xlow_movement,
             "density": mid, "space": xhigh, "aggression": xlow}},
    # 13. bright cold frozen dense gentle
    {"label": "BRIGHT_COLD_FROZEN_DENSE_GENTLE",
     "dna": {"brightness": xhigh, "warmth": xlow, "movement": xlow_movement,
             "density": xhigh, "space": mid, "aggression": xlow}},
    # 14. hot frozen sparse vast gentle
    {"label": "HOT_FROZEN_SPARSE_VAST_GENTLE",
     "dna": {"brightness": mid, "warmth": xhigh, "movement": xlow_movement,
             "density": xlow, "space": xhigh, "aggression": xlow}},
    # 15. dark still dense intimate gentle
    {"label": "DARK_STILL_DENSE_INTIMATE_GENTLE",
     "dna": {"brightness": xlow, "warmth": mid, "movement": xlow_movement,
             "density": xhigh, "space": xlow, "aggression": xlow}},
    # 16. bright warm frozen sparse intimate
    {"label": "BRIGHT_WARM_FROZEN_SPARSE_INTIMATE",
     "dna": {"brightness": xhigh, "warmth": xhigh, "movement": xlow_movement,
             "density": xlow, "space": xlow, "aggression": mid}},
    # 17. dark hot frozen dense vast
    {"label": "DARK_HOT_FROZEN_DENSE_VAST",
     "dna": {"brightness": xlow, "warmth": xhigh, "movement": xlow_movement,
             "density": xhigh, "space": xhigh, "aggression": mid}},
    # 18. bright cold frozen vast gentle
    {"label": "BRIGHT_COLD_FROZEN_VAST_GENTLE",
     "dna": {"brightness": xhigh, "warmth": xlow, "movement": xlow_movement,
             "density": mid, "space": xhigh, "aggression": xlow}},
    # 19. cold frozen dense vast violent
    {"label": "COLD_FROZEN_DENSE_VAST_VIOLENT",
     "dna": {"brightness": mid, "warmth": xlow, "movement": xlow_movement,
             "density": xhigh, "space": xhigh, "aggression": xhigh}},
    # 20. bright warm frozen dense gentle
    {"label": "BRIGHT_WARM_FROZEN_DENSE_GENTLE",
     "dna": {"brightness": xhigh, "warmth": xhigh, "movement": xlow_movement,
             "density": xhigh, "space": mid, "aggression": xlow}},
]

# ---------------------------------------------------------------------------
# Preset generation
# ---------------------------------------------------------------------------

def pick_engines():
    """Pick 2 distinct engines."""
    return random.sample(ALL_ENGINES, 2)

def pick_coupling(engine1, engine2):
    coupling_type = random.choice(COUPLING_POOL)
    amount = round(random.uniform(0.75, 0.99), 4)
    return {
        "type": coupling_type,
        "source": engine1,
        "target": engine2,
        "amount": amount,
    }

def engine_params(movement_val):
    """
    Generate per-engine macro params.
    macro_movement reflects the DNA movement (frozen).
    macro_coupling is high for entangled aesthetic.
    """
    char = round(random.uniform(0.50, 0.99), 4)
    coupling = round(random.uniform(0.75, 0.99), 4)
    space = round(random.uniform(0.10, 0.95), 4)
    # movement: small jitter around the DNA movement value, still ≤ 0.13
    mv = round(min(0.13, max(0.01, movement_val + random.uniform(-0.02, 0.02))), 4)
    return {
        "macro_character": char,
        "macro_movement": mv,
        "macro_coupling": coupling,
        "macro_space": space,
    }

def make_preset(corner_label, corner_dna_spec, variant_idx):
    """Build one preset dict."""
    # Resolve DNA values (call each callable)
    dna = {k: fn() for k, fn in corner_dna_spec.items()}
    movement_val = dna["movement"]

    engines = pick_engines()
    engine1, engine2 = engines

    name = f"{corner_label}_ENT3_{variant_idx}"

    params = {
        engine1: engine_params(movement_val),
        engine2: engine_params(movement_val),
    }
    # Set macros to reflect aggregate DNA
    macros = {
        "CHARACTER": round((params[engine1]["macro_character"] + params[engine2]["macro_character"]) / 2, 4),
        "MOVEMENT":  round((params[engine1]["macro_movement"]  + params[engine2]["macro_movement"])  / 2, 4),
        "COUPLING":  round((params[engine1]["macro_coupling"]  + params[engine2]["macro_coupling"])  / 2, 4),
        "SPACE":     round((params[engine1]["macro_space"]     + params[engine2]["macro_space"])     / 2, 4),
    }

    # Build descriptive tags
    tags = ["entangled", "frozen", "static", "resonant"]
    if dna["brightness"] >= 0.87:
        tags.append("bright")
    elif dna["brightness"] <= 0.13:
        tags.append("dark")
    if dna["warmth"] >= 0.87:
        tags.append("warm")
    elif dna["warmth"] <= 0.13:
        tags.append("cold")
    if dna["aggression"] >= 0.87:
        tags.append("violent")
    elif dna["aggression"] <= 0.13:
        tags.append("gentle")
    if dna["density"] >= 0.87:
        tags.append("dense")
    elif dna["density"] <= 0.13:
        tags.append("sparse")
    if dna["space"] >= 0.87:
        tags.append("vast")
    elif dna["space"] <= 0.13:
        tags.append("intimate")

    return {
        "schema_version": 1,
        "name": name,
        "mood": "Entangled",
        "engines": engines,
        "author": "XO_OX",
        "version": "1.0.0",
        "description": (
            f"Static Entangled diversity anchor. Frozen resonance — movement ≤ 0.13. "
            f"Corner: {corner_label}. Designed to raise Entangled cosine diversity."
        ),
        "tags": tags,
        "macroLabels": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
        "couplingIntensity": "High",
        "tempo": None,
        "created": TODAY,
        "legacy": {
            "sourceInstrument": None,
            "sourceCategory": None,
            "sourcePresetName": None,
        },
        "parameters": params,
        "coupling": pick_coupling(engine1, engine2),
        "macros": macros,
        "sequencer": None,
        "dna": dna,
    }

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    presets = []
    for corner in CORNERS:
        for v in range(1, 6):  # variants 1–5
            preset = make_preset(corner["label"], corner["dna"], v)
            presets.append(preset)

    # Validate movement ≤ 0.13
    violations = []
    for p in presets:
        mv = p["dna"]["movement"]
        if mv > 0.13:
            violations.append((p["name"], mv))
        # Also check per-engine macro_movement
        for eng, params in p["parameters"].items():
            if params["macro_movement"] > 0.13:
                violations.append((f"{p['name']} / {eng} macro_movement", params["macro_movement"]))

    if violations:
        print("VIOLATIONS FOUND (movement > 0.13):")
        for name, val in violations:
            print(f"  {name}: {val}")
    else:
        print("All movement values ≤ 0.13 — PASS")

    # Write files
    written = 0
    for p in presets:
        filename = p["name"] + ".xometa"
        out_path = OUTPUT_DIR / filename
        with open(out_path, "w", encoding="utf-8") as f:
            json.dump(p, f, indent=2)
        written += 1

    print(f"Written {written} presets to {OUTPUT_DIR}")

    # Count ENT3 files
    ent3_files = list(OUTPUT_DIR.glob("*ENT3*.xometa"))
    print(f"ENT3 files in Entangled/: {len(ent3_files)}")

    # Quick diversity summary
    import math
    vectors = [list(p["dna"].values()) for p in presets]
    # Pairwise cosine diversity sample (first 20 pairs as proxy)
    def cosine_sim(a, b):
        dot = sum(x*y for x,y in zip(a,b))
        na = math.sqrt(sum(x*x for x in a))
        nb = math.sqrt(sum(x*x for x in b))
        return dot / (na * nb) if na and nb else 1.0

    sample_pairs = [(vectors[i], vectors[j]) for i in range(0, 20) for j in range(i+1, 20)]
    sims = [cosine_sim(a, b) for a, b in sample_pairs]
    avg_sim = sum(sims) / len(sims)
    avg_div = 1.0 - avg_sim
    print(f"Sample cosine diversity (first 20 presets, pairwise avg): {avg_div:.4f}")
    print(f"Movement range: {min(p['dna']['movement'] for p in presets):.4f} – {max(p['dna']['movement'] for p in presets):.4f}")

if __name__ == "__main__":
    main()
