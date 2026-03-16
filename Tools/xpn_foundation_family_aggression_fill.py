#!/usr/bin/env python3
"""
xpn_foundation_family_aggression_fill.py

Generates 80 aggression-XHIGH presets to fill under-represented corners:
  - 40 Foundation presets (8 corners × 5 variants)
  - 40 Family presets (8 corners × 5 variants)

All presets have aggression ≥ 0.87 (XHIGH band: 0.87–0.99).
"""

import json
import os
import random
from pathlib import Path

random.seed(42)

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

XLOW  = lambda: round(random.uniform(0.02, 0.13), 3)
XHIGH = lambda: round(random.uniform(0.87, 0.99), 3)
MID   = lambda: round(random.uniform(0.30, 0.70), 3)
AGG   = lambda: round(random.uniform(0.87, 0.99), 3)

COUPLING_TYPES = [
    "FREQUENCY_SHIFT", "AMPLITUDE_MOD", "FILTER_MOD", "PITCH_SYNC",
    "TIMBRE_BLEND", "ENVELOPE_LINK", "HARMONIC_FOLD", "CHAOS_INJECT",
    "RESONANCE_SHARE", "SPATIAL_COUPLE", "SPECTRAL_MORPH", "VELOCITY_COUPLE",
]

def rand_coupling():
    return random.choice(COUPLING_TYPES)

def rand_amount():
    return round(random.uniform(0.70, 0.99), 3)

def make_macro():
    return round(random.uniform(0.35, 0.95), 3)

def build_preset(name, mood, engines, dna, tags):
    e1, e2 = engines[0], engines[1]
    macro_vals = {k: make_macro() for k in ("character", "movement", "coupling", "space")}
    return {
        "name": name,
        "version": "1.0",
        "mood": mood,
        "engines": [e1, e2],
        "parameters": {
            e1: {
                "macro_character": macro_vals["character"],
                "macro_movement":  macro_vals["movement"],
                "macro_coupling":  macro_vals["coupling"],
                "macro_space":     macro_vals["space"],
            },
            e2: {
                "macro_character": round(macro_vals["character"] + random.uniform(-0.05, 0.05), 3),
                "macro_movement":  round(macro_vals["movement"]  + random.uniform(-0.05, 0.05), 3),
                "macro_coupling":  round(macro_vals["coupling"]  + random.uniform(-0.05, 0.05), 3),
                "macro_space":     round(macro_vals["space"]     + random.uniform(-0.05, 0.05), 3),
            },
        },
        "coupling": {
            "type":   rand_coupling(),
            "source": e1,
            "target": e2,
            "amount": rand_amount(),
        },
        "dna": dna,
        "macros": {
            "CHARACTER": macro_vals["character"],
            "MOVEMENT":  macro_vals["movement"],
            "COUPLING":  macro_vals["coupling"],
            "SPACE":     macro_vals["space"],
        },
        "tags": tags,
    }

# ---------------------------------------------------------------------------
# Foundation corners — 8 definitions
# Each tuple: (name_slug, dna_builder_fn, tag_list, engine_pool)
# ---------------------------------------------------------------------------

FOUNDATION_ENGINES = [
    "ODDFELIX", "ODDOSCAR", "OBLONG", "OBESE", "OVERBITE",
    "OVERDUB", "ONSET", "OUROBOROS", "OBSIDIAN", "ORIGAMI", "ORACLE",
]

def fnd_dna_1():
    return {"brightness": XLOW(),  "warmth": XLOW(),  "movement": MID(),   "density": XHIGH(), "space": MID(),   "aggression": AGG()}
def fnd_dna_2():
    return {"brightness": XHIGH(), "warmth": MID(),   "movement": XHIGH(), "density": XHIGH(), "space": MID(),   "aggression": AGG()}
def fnd_dna_3():
    return {"brightness": XLOW(),  "warmth": MID(),   "movement": XHIGH(), "density": MID(),   "space": XLOW(),  "aggression": AGG()}
def fnd_dna_4():
    return {"brightness": XHIGH(), "warmth": XHIGH(), "movement": MID(),   "density": XHIGH(), "space": MID(),   "aggression": AGG()}
def fnd_dna_5():
    return {"brightness": MID(),   "warmth": XLOW(),  "movement": MID(),   "density": XHIGH(), "space": XLOW(),  "aggression": AGG()}
def fnd_dna_6():
    return {"brightness": XLOW(),  "warmth": MID(),   "movement": MID(),   "density": XHIGH(), "space": XHIGH(), "aggression": AGG()}
def fnd_dna_7():
    return {"brightness": XHIGH(), "warmth": XLOW(),  "movement": XHIGH(), "density": MID(),   "space": MID(),   "aggression": AGG()}
def fnd_dna_8():
    return {"brightness": XLOW(),  "warmth": XHIGH(), "movement": XHIGH(), "density": MID(),   "space": MID(),   "aggression": AGG()}

FND_CORNERS = [
    ("DARK_COLD_DENSE_VIOLENT_FND_AGG",    fnd_dna_1, ["foundation", "violent", "dark", "cold", "dense"]),
    ("BRIGHT_KINETIC_DENSE_VIOLENT_FND_AGG", fnd_dna_2, ["foundation", "violent", "bright", "kinetic", "dense"]),
    ("DARK_KINETIC_INTIMATE_VIOLENT_FND_AGG", fnd_dna_3, ["foundation", "violent", "dark", "kinetic", "intimate"]),
    ("BRIGHT_WARM_DENSE_VIOLENT_FND_AGG",  fnd_dna_4, ["foundation", "violent", "bright", "warm", "dense"]),
    ("COLD_DENSE_INTIMATE_VIOLENT_FND_AGG", fnd_dna_5, ["foundation", "violent", "cold", "dense", "intimate"]),
    ("DARK_DENSE_VAST_VIOLENT_FND_AGG",    fnd_dna_6, ["foundation", "violent", "dark", "dense", "vast"]),
    ("BRIGHT_COLD_KINETIC_VIOLENT_FND_AGG", fnd_dna_7, ["foundation", "violent", "bright", "cold", "kinetic"]),
    ("DARK_HOT_KINETIC_VIOLENT_FND_AGG",   fnd_dna_8, ["foundation", "violent", "dark", "hot", "kinetic"]),
]

# ---------------------------------------------------------------------------
# Family corners — 8 definitions
# ---------------------------------------------------------------------------

FAMILY_ENGINES = [
    "ODDFELIX", "ODDOSCAR", "OBLONG", "OVERDUB", "ODYSSEY",
    "OPAL", "OHM", "ORPHICA", "OBBLIGATO", "OTTONI", "OLE", "OSPREY",
]

def fam_dna_1():
    return {"brightness": XHIGH(), "warmth": XHIGH(), "movement": MID(),   "density": XHIGH(), "space": MID(),   "aggression": AGG()}
def fam_dna_2():
    return {"brightness": XLOW(),  "warmth": XHIGH(), "movement": XHIGH(), "density": MID(),   "space": MID(),   "aggression": AGG()}
def fam_dna_3():
    return {"brightness": XHIGH(), "warmth": XHIGH(), "movement": MID(),   "density": MID(),   "space": XLOW(),  "aggression": AGG()}
def fam_dna_4():
    return {"brightness": XLOW(),  "warmth": XHIGH(), "movement": MID(),   "density": XHIGH(), "space": MID(),   "aggression": AGG()}
def fam_dna_5():
    return {"brightness": MID(),   "warmth": XHIGH(), "movement": XHIGH(), "density": XHIGH(), "space": MID(),   "aggression": AGG()}
def fam_dna_6():
    return {"brightness": XHIGH(), "warmth": XHIGH(), "movement": XHIGH(), "density": MID(),   "space": MID(),   "aggression": AGG()}
def fam_dna_7():
    return {"brightness": XLOW(),  "warmth": MID(),   "movement": MID(),   "density": XHIGH(), "space": XLOW(),  "aggression": AGG()}
def fam_dna_8():
    return {"brightness": XHIGH(), "warmth": XLOW(),  "movement": MID(),   "density": XHIGH(), "space": MID(),   "aggression": AGG()}

FAM_CORNERS = [
    ("BRIGHT_WARM_DENSE_VIOLENT_FAM_AGG",   fam_dna_1, ["family", "violent", "bright", "warm", "dense"]),
    ("DARK_HOT_KINETIC_VIOLENT_FAM_AGG",    fam_dna_2, ["family", "violent", "dark", "hot", "kinetic"]),
    ("BRIGHT_WARM_INTIMATE_VIOLENT_FAM_AGG", fam_dna_3, ["family", "violent", "bright", "warm", "intimate"]),
    ("DARK_HOT_DENSE_VIOLENT_FAM_AGG",      fam_dna_4, ["family", "violent", "dark", "hot", "dense"]),
    ("HOT_KINETIC_DENSE_VIOLENT_FAM_AGG",   fam_dna_5, ["family", "violent", "hot", "kinetic", "dense"]),
    ("BRIGHT_WARM_KINETIC_VIOLENT_FAM_AGG", fam_dna_6, ["family", "violent", "bright", "warm", "kinetic"]),
    ("DARK_DENSE_INTIMATE_VIOLENT_FAM_AGG", fam_dna_7, ["family", "violent", "dark", "dense", "intimate"]),
    ("BRIGHT_COLD_DENSE_VIOLENT_FAM_AGG",   fam_dna_8, ["family", "violent", "bright", "cold", "dense"]),
]

# ---------------------------------------------------------------------------
# Generator
# ---------------------------------------------------------------------------

def pick_engines(pool):
    """Pick 2 distinct engines from pool."""
    return random.sample(pool, 2)

def generate_batch(corners, engine_pool, mood, out_dir, variants=5):
    Path(out_dir).mkdir(parents=True, exist_ok=True)
    presets = []
    for slug, dna_fn, tags in corners:
        for v in range(1, variants + 1):
            name = f"{slug}_{v}"
            filename = f"{name}.xometa"
            engines = pick_engines(engine_pool)
            dna = dna_fn()
            preset = build_preset(name, mood, engines, dna, tags)
            path = os.path.join(out_dir, filename)
            with open(path, "w") as f:
                json.dump(preset, f, indent=2)
            presets.append((name, dna["aggression"], path))
    return presets

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

REPO_ROOT = Path(__file__).resolve().parent.parent
FND_OUT = REPO_ROOT / "Presets" / "XOmnibus" / "Foundation"
FAM_OUT = REPO_ROOT / "Presets" / "XOmnibus" / "Family"

def main():
    print("Generating Foundation aggression-XHIGH presets...")
    fnd_results = generate_batch(FND_CORNERS, FOUNDATION_ENGINES, "Foundation", FND_OUT, variants=5)

    print("Generating Family aggression-XHIGH presets...")
    fam_results = generate_batch(FAM_CORNERS, FAMILY_ENGINES, "Family", FAM_OUT, variants=5)

    # Verification
    print("\n--- Verification ---")
    all_results = [("Foundation", fnd_results), ("Family", fam_results)]
    grand_total = 0
    for mood, results in all_results:
        failures = [(n, a) for n, a, _ in results if a < 0.87]
        print(f"{mood}: {len(results)} presets generated | aggression failures (< 0.87): {len(failures)}")
        if failures:
            for n, a in failures:
                print(f"  FAIL: {n} aggression={a}")
        grand_total += len(results)

    print(f"\nTotal presets written: {grand_total}")
    assert grand_total == 80, f"Expected 80 presets, got {grand_total}"
    total_failures = sum(1 for _, results in all_results for n, a, _ in results if a < 0.87)
    assert total_failures == 0, f"{total_failures} preset(s) failed aggression ≥ 0.87 check"
    print("All assertions PASS. 80 presets written with aggression ≥ 0.87.")

if __name__ == "__main__":
    main()
