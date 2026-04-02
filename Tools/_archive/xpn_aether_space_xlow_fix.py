#!/usr/bin/env python3
"""
xpn_aether_space_xlow_fix.py
Generates 80 Aether presets all with space-XLOW (space <= 0.13).
Intimate/claustrophobic Aether — transcendence in a tiny space.
16 corner combos × 5 variants = 80 presets.
"""

import json
import os
import random

random.seed(42)

OUTPUT_DIR = os.path.join(os.path.dirname(__file__), "..", "Presets", "XOceanus", "Aether")
os.makedirs(OUTPUT_DIR, exist_ok=True)

AETHER_ENGINES = [
    "OPAL", "ORACLE", "OBSCURA", "OCEANIC", "ORBITAL", "ORGANON",
    "OBLIQUE", "ORIGAMI", "ODDFELIX", "ODDOSCAR", "OSPREY", "OMBRE"
]

COUPLING_TYPES = [
    "FREQUENCY_SHIFT", "AMPLITUDE_MOD", "FILTER_MOD", "PITCH_SYNC",
    "TIMBRE_BLEND", "ENVELOPE_LINK", "HARMONIC_FOLD", "CHAOS_INJECT",
    "RESONANCE_SHARE", "SPATIAL_COUPLE", "SPECTRAL_MORPH", "VELOCITY_COUPLE"
]

def xlow_space():
    return round(random.uniform(0.02, 0.10), 4)

def xlow():
    return round(random.uniform(0.02, 0.13), 4)

def xhigh():
    return round(random.uniform(0.87, 0.99), 4)

def mid():
    return round(random.uniform(0.3, 0.7), 4)

# 16 corner definitions: each is a dict of dna dimension -> value generator
CORNERS = [
    {
        "label": "BRIGHT_WARM_INTIMATE_GENTLE",
        "tags": ["bright", "warm", "intimate", "gentle", "celestial"],
        "dna": lambda: {"brightness": xhigh(), "warmth": xhigh(), "space": xlow_space(), "aggression": xlow(), "movement": mid(), "density": mid()},
    },
    {
        "label": "DARK_COLD_INTIMATE_VIOLENT",
        "tags": ["dark", "cold", "intimate", "violent", "compressed"],
        "dna": lambda: {"brightness": xlow(), "warmth": xlow(), "space": xlow_space(), "aggression": xhigh(), "movement": mid(), "density": mid()},
    },
    {
        "label": "BRIGHT_KINETIC_INTIMATE_DENSE",
        "tags": ["bright", "kinetic", "intimate", "dense", "ethereal"],
        "dna": lambda: {"brightness": xhigh(), "movement": xhigh(), "space": xlow_space(), "density": xhigh(), "warmth": mid(), "aggression": mid()},
    },
    {
        "label": "DARK_STILL_INTIMATE_SPARSE",
        "tags": ["dark", "still", "intimate", "sparse", "void"],
        "dna": lambda: {"brightness": xlow(), "movement": xlow(), "space": xlow_space(), "density": xlow(), "warmth": mid(), "aggression": mid()},
    },
    {
        "label": "HOT_DENSE_INTIMATE_VIOLENT",
        "tags": ["hot", "dense", "intimate", "violent", "compressed"],
        "dna": lambda: {"warmth": xhigh(), "density": xhigh(), "space": xlow_space(), "aggression": xhigh(), "brightness": mid(), "movement": mid()},
    },
    {
        "label": "BRIGHT_COLD_INTIMATE_VIOLENT",
        "tags": ["bright", "cold", "intimate", "violent", "crystalline"],
        "dna": lambda: {"brightness": xhigh(), "warmth": xlow(), "space": xlow_space(), "aggression": xhigh(), "movement": mid(), "density": mid()},
    },
    {
        "label": "DARK_HOT_KINETIC_INTIMATE",
        "tags": ["dark", "hot", "kinetic", "intimate", "smoldering"],
        "dna": lambda: {"brightness": xlow(), "warmth": xhigh(), "space": xlow_space(), "movement": xhigh(), "density": mid(), "aggression": mid()},
    },
    {
        "label": "BRIGHT_SPARSE_INTIMATE_GENTLE",
        "tags": ["bright", "sparse", "intimate", "gentle", "airy"],
        "dna": lambda: {"brightness": xhigh(), "density": xlow(), "space": xlow_space(), "aggression": xlow(), "warmth": mid(), "movement": mid()},
    },
    {
        "label": "COLD_KINETIC_INTIMATE_DENSE",
        "tags": ["cold", "kinetic", "intimate", "dense", "mechanical"],
        "dna": lambda: {"warmth": xlow(), "movement": xhigh(), "space": xlow_space(), "density": xhigh(), "brightness": mid(), "aggression": mid()},
    },
    {
        "label": "BRIGHT_WARM_KINETIC_INTIMATE",
        "tags": ["bright", "warm", "kinetic", "intimate", "celestial"],
        "dna": lambda: {"brightness": xhigh(), "warmth": xhigh(), "space": xlow_space(), "movement": xhigh(), "density": mid(), "aggression": mid()},
    },
    {
        "label": "DARK_DENSE_INTIMATE_VIOLENT",
        "tags": ["dark", "dense", "intimate", "violent", "abyssal"],
        "dna": lambda: {"brightness": xlow(), "density": xhigh(), "space": xlow_space(), "aggression": xhigh(), "warmth": mid(), "movement": mid()},
    },
    {
        "label": "BRIGHT_COLD_STILL_INTIMATE",
        "tags": ["bright", "cold", "still", "intimate", "frozen"],
        "dna": lambda: {"brightness": xhigh(), "movement": xlow(), "space": xlow_space(), "warmth": xlow(), "density": mid(), "aggression": mid()},
    },
    {
        "label": "HOT_KINETIC_INTIMATE_GENTLE",
        "tags": ["hot", "kinetic", "intimate", "gentle", "tender"],
        "dna": lambda: {"warmth": xhigh(), "movement": xhigh(), "space": xlow_space(), "aggression": xlow(), "brightness": mid(), "density": mid()},
    },
    {
        "label": "DARK_COLD_DENSE_INTIMATE",
        "tags": ["dark", "cold", "dense", "intimate", "compressed"],
        "dna": lambda: {"brightness": xlow(), "warmth": xlow(), "space": xlow_space(), "density": xhigh(), "movement": mid(), "aggression": mid()},
    },
    {
        "label": "BRIGHT_WARM_DENSE_INTIMATE",
        "tags": ["bright", "warm", "dense", "intimate", "lush"],
        "dna": lambda: {"brightness": xhigh(), "warmth": xhigh(), "space": xlow_space(), "density": xhigh(), "movement": mid(), "aggression": mid()},
    },
    {
        "label": "DARK_KINETIC_INTIMATE_GENTLE",
        "tags": ["dark", "kinetic", "intimate", "gentle", "stirring"],
        "dna": lambda: {"brightness": xlow(), "movement": xhigh(), "space": xlow_space(), "aggression": xlow(), "warmth": mid(), "density": mid()},
    },
]

assert len(CORNERS) == 16, f"Expected 16 corners, got {len(CORNERS)}"

generated = []

for corner in CORNERS:
    label = corner["label"]
    tags = corner["tags"]
    for variant in range(1, 6):
        name = f"{label}_AET2_{variant}"
        dna = corner["dna"]()

        # Pick 2 engines (no repeat)
        engines = random.sample(AETHER_ENGINES, 2)
        engine1, engine2 = engines

        # Coupling
        coupling_type = random.choice(COUPLING_TYPES)
        coupling_amount = round(random.uniform(0.5, 0.95), 4)

        # Per-engine macros derived from dna
        def engine_params(dna, jitter=0.03):
            def j(v):
                return round(min(0.99, max(0.01, v + random.uniform(-jitter, jitter))), 4)
            return {
                "macro_character": j(dna.get("brightness", 0.5)),
                "macro_movement": j(dna.get("movement", 0.5)),
                "macro_coupling": j(dna.get("density", 0.5)),
                "macro_space": j(dna["space"]),
            }

        params = {
            engine1: engine_params(dna),
            engine2: engine_params(dna, jitter=0.05),
        }

        # Macro summary
        macros = {
            "CHARACTER": round((params[engine1]["macro_character"] + params[engine2]["macro_character"]) / 2, 4),
            "MOVEMENT": round((params[engine1]["macro_movement"] + params[engine2]["macro_movement"]) / 2, 4),
            "COUPLING": coupling_amount,
            "SPACE": dna["space"],
        }

        preset = {
            "name": name,
            "version": "1.0",
            "mood": "Aether",
            "engines": [engine1, engine2],
            "parameters": params,
            "coupling": {
                "type": coupling_type,
                "source": engine1,
                "target": engine2,
                "amount": coupling_amount,
            },
            "dna": dna,
            "macros": macros,
            "tags": ["aether", "intimate", "space-xlow"] + tags,
        }

        filename = f"{name}.xometa"
        filepath = os.path.join(OUTPUT_DIR, filename)
        with open(filepath, "w") as f:
            json.dump(preset, f, indent=2)

        generated.append((name, dna["space"]))

# Verification
print(f"\nGenerated {len(generated)} presets")
violations = [(n, s) for n, s in generated if s > 0.13]
if violations:
    print(f"VIOLATIONS (space > 0.13): {violations}")
else:
    print("All presets pass space <= 0.13 check.")

max_space = max(s for _, s in generated)
min_space = min(s for _, s in generated)
print(f"Space range: {min_space} – {max_space}")

# Count total Aether presets
total = len([f for f in os.listdir(OUTPUT_DIR) if f.endswith(".xometa")])
print(f"Total .xometa files in Aether/: {total}")
