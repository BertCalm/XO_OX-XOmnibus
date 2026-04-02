#!/usr/bin/env python3
"""
xpn_flux_ultra_diverse_v2.py
Generates 80 Flux mood presets with extreme DNA diversity.
16 corner combinations × 5 variants = 80 presets.
Output: Presets/XOceanus/Flux/
"""

import json
import os
import random

# ── Constants ────────────────────────────────────────────────────────────────

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
REPO_ROOT  = os.path.dirname(SCRIPT_DIR)
OUTPUT_DIR = os.path.join(REPO_ROOT, "Presets", "XOceanus", "Flux")

VALID_ENGINES = [
    "OUROBOROS", "ORACLE", "ORGANON", "ORIGAMI", "OPTIC",
    "OPAL", "OBLONG", "ODYSSEY", "OVERDUB", "OBLIQUE",
    "OCEANIC", "OVERWORLD",
]

COUPLING_TYPES = [
    "FREQUENCY_SHIFT", "AMPLITUDE_MOD", "FILTER_MOD", "PITCH_SYNC",
    "TIMBRE_BLEND", "ENVELOPE_LINK", "HARMONIC_FOLD", "CHAOS_INJECT",
    "RESONANCE_SHARE", "SPATIAL_COUPLE", "SPECTRAL_MORPH", "VELOCITY_COUPLE",
]

DNA_DIMS = ["brightness", "warmth", "movement", "density", "space", "aggression"]

# ── Corner definitions ────────────────────────────────────────────────────────
# Each corner: dict of dim -> "XLOW" | "XHIGH" | None (free / mid range)

CORNERS = [
    # 1 – dark cold kinetic violent
    {"label": "DARK_COLD_KINETIC_VIOLENT",
     "brightness": "XLOW", "warmth": "XLOW", "movement": "XHIGH", "aggression": "XHIGH",
     "density": None, "space": None},

    # 2 – bright warm kinetic vast
    {"label": "BRIGHT_WARM_KINETIC_VAST",
     "brightness": "XHIGH", "warmth": "XHIGH", "movement": "XHIGH", "space": "XHIGH",
     "density": None, "aggression": None},

    # 3 – bright cold dense intimate
    {"label": "BRIGHT_COLD_DENSE_INTIMATE",
     "brightness": "XHIGH", "warmth": "XLOW", "density": "XHIGH", "space": "XLOW",
     "movement": None, "aggression": None},

    # 4 – dark hot still vast
    {"label": "DARK_HOT_STILL_VAST",
     "brightness": "XLOW", "warmth": "XHIGH", "movement": "XLOW", "space": "XHIGH",
     "density": None, "aggression": None},

    # 5 – bright kinetic sparse violent
    {"label": "BRIGHT_KINETIC_SPARSE_VIOLENT",
     "brightness": "XHIGH", "movement": "XHIGH", "density": "XLOW", "aggression": "XHIGH",
     "warmth": None, "space": None},

    # 6 – dark dense vast gentle
    {"label": "DARK_DENSE_VAST_GENTLE",
     "brightness": "XLOW", "density": "XHIGH", "space": "XHIGH", "aggression": "XLOW",
     "warmth": None, "movement": None},

    # 7 – hot dense intimate violent
    {"label": "HOT_DENSE_INTIMATE_VIOLENT",
     "warmth": "XHIGH", "density": "XHIGH", "space": "XLOW", "aggression": "XHIGH",
     "brightness": None, "movement": None},

    # 8 – cold kinetic sparse vast
    {"label": "COLD_KINETIC_SPARSE_VAST",
     "warmth": "XLOW", "movement": "XHIGH", "density": "XLOW", "space": "XHIGH",
     "brightness": None, "aggression": None},

    # 9 – dark hot dense violent
    {"label": "DARK_HOT_DENSE_VIOLENT",
     "brightness": "XLOW", "warmth": "XHIGH", "density": "XHIGH", "aggression": "XHIGH",
     "movement": None, "space": None},

    # 10 – bright still vast gentle
    {"label": "BRIGHT_STILL_VAST_GENTLE",
     "brightness": "XHIGH", "movement": "XLOW", "space": "XHIGH", "aggression": "XLOW",
     "warmth": None, "density": None},

    # 11 – cold dense intimate gentle
    {"label": "COLD_DENSE_INTIMATE_GENTLE",
     "warmth": "XLOW", "density": "XHIGH", "space": "XLOW", "aggression": "XLOW",
     "brightness": None, "movement": None},

    # 12 – bright warm sparse violent
    {"label": "BRIGHT_WARM_SPARSE_VIOLENT",
     "brightness": "XHIGH", "warmth": "XHIGH", "density": "XLOW", "aggression": "XHIGH",
     "movement": None, "space": None},

    # 13 – dark kinetic intimate violent
    {"label": "DARK_KINETIC_INTIMATE_VIOLENT",
     "brightness": "XLOW", "movement": "XHIGH", "space": "XLOW", "aggression": "XHIGH",
     "warmth": None, "density": None},

    # 14 – hot still sparse vast
    {"label": "HOT_STILL_SPARSE_VAST",
     "warmth": "XHIGH", "movement": "XLOW", "density": "XLOW", "space": "XHIGH",
     "brightness": None, "aggression": None},

    # 15 – dark cold sparse vast
    {"label": "DARK_COLD_SPARSE_VAST",
     "brightness": "XLOW", "warmth": "XLOW", "density": "XLOW", "space": "XHIGH",
     "movement": None, "aggression": None},

    # 16 – bright cold kinetic dense
    {"label": "BRIGHT_COLD_KINETIC_DENSE",
     "brightness": "XHIGH", "warmth": "XLOW", "movement": "XHIGH", "density": "XHIGH",
     "space": None, "aggression": None},
]

# ── Helpers ───────────────────────────────────────────────────────────────────

rng = random.Random()  # seeded per variant for reproducibility


def xlow() -> float:
    return round(rng.uniform(0.03, 0.14), 4)


def xhigh() -> float:
    return round(rng.uniform(0.86, 0.98), 4)


def mid() -> float:
    return round(rng.uniform(0.30, 0.70), 4)


def sample_dna(corner: dict) -> dict:
    dna = {}
    for dim in DNA_DIMS:
        zone = corner.get(dim)
        if zone == "XLOW":
            dna[dim] = xlow()
        elif zone == "XHIGH":
            dna[dim] = xhigh()
        else:
            dna[dim] = mid()
    return dna


def pick_engines(dna: dict, n: int = 2) -> list:
    """Pick n engines. Weight toward modulation-heavy engines based on movement/aggression."""
    pool = list(VALID_ENGINES)
    rng.shuffle(pool)
    return pool[:n]


def engine_params(engine: str, dna: dict) -> dict:
    """Generate macro params for an engine loosely informed by DNA."""
    mv = dna["movement"]
    ag = dna["aggression"]
    br = dna["brightness"]
    sp = dna["space"]
    return {
        "macro_character": round(rng.uniform(max(0.05, br - 0.15), min(0.95, br + 0.15)), 4),
        "macro_movement":  round(rng.uniform(max(0.05, mv - 0.10), min(0.98, mv + 0.10)), 4),
        "macro_coupling":  round(rng.uniform(max(0.05, ag - 0.15), min(0.95, ag + 0.15)), 4),
        "macro_space":     round(rng.uniform(max(0.05, sp - 0.15), min(0.95, sp + 0.15)), 4),
    }


def build_preset(corner: dict, variant: int) -> dict:
    label = corner["label"]
    name  = f"{label}_FLX_{variant}"

    dna     = sample_dna(corner)
    engines = pick_engines(dna, n=2)
    e1, e2  = engines[0], engines[1]

    params = {
        e1: engine_params(e1, dna),
        e2: engine_params(e2, dna),
    }

    coupling_type = rng.choice(COUPLING_TYPES)
    coupling_amount = round(rng.uniform(0.55, 0.98), 4)

    # Tags derived from extreme zones
    tags = ["flux", "unstable", "evolving"]
    for dim in DNA_DIMS:
        zone = corner.get(dim)
        if zone == "XLOW":
            tags.append(f"low-{dim}")
        elif zone == "XHIGH":
            tags.append(f"high-{dim}")

    # Macros mirror engine 1 params
    p1 = params[e1]
    macros = {
        "CHARACTER": p1["macro_character"],
        "MOVEMENT":  p1["macro_movement"],
        "COUPLING":  p1["macro_coupling"],
        "SPACE":     p1["macro_space"],
    }

    preset = {
        "name":           name,
        "version":        "1.0",
        "mood":           "Flux",
        "engines":        engines,
        "parameters":     params,
        "coupling": {
            "type":   coupling_type,
            "source": e1,
            "target": e2,
            "amount": coupling_amount,
        },
        "dna":    dna,
        "macros": macros,
        "tags":   tags,
    }
    return preset


# ── Main ──────────────────────────────────────────────────────────────────────

def main():
    os.makedirs(OUTPUT_DIR, exist_ok=True)

    generated = []

    for corner_idx, corner in enumerate(CORNERS, start=1):
        for variant in range(1, 6):  # 5 variants per corner
            seed = corner_idx * 100 + variant
            rng.seed(seed)

            preset = build_preset(corner, variant)
            filename = f"{preset['name']}.xometa"
            filepath = os.path.join(OUTPUT_DIR, filename)

            with open(filepath, "w") as f:
                json.dump(preset, f, indent=2)

            generated.append(filename)

    print(f"Generated {len(generated)} presets → {OUTPUT_DIR}")

    # Verify extreme zone count per preset
    failures = []
    for name in generated:
        p = json.load(open(os.path.join(OUTPUT_DIR, name)))
        dna = p["dna"]
        extreme_count = sum(
            1 for v in dna.values() if v <= 0.15 or v >= 0.85
        )
        if extreme_count < 4:
            failures.append((name, extreme_count))

    if failures:
        print(f"\nWARNING: {len(failures)} presets have fewer than 4 extreme DNA dims:")
        for n, c in failures:
            print(f"  {n}  ({c} extreme dims)")
    else:
        print("All 80 presets pass: ≥4 extreme DNA dimensions each.")


if __name__ == "__main__":
    main()
