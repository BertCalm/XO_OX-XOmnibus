#!/usr/bin/env python3
"""
xpn_aggression_xhigh_correction.py

Generates 120 aggression-XHIGH correction presets to address systemic bias:
- 40 Atmosphere presets (aggression >= 0.87)
- 40 Aether presets (aggression >= 0.87)
- 40 Prism presets (aggression >= 0.87)

8 corners x 5 variants = 40 presets per mood.
"""

import json
import os
import random
from pathlib import Path

random.seed(42)

REPO_ROOT = Path(__file__).parent.parent
PRESETS_BASE = REPO_ROOT / "Presets" / "XOceanus"

COUPLING_TYPES = [
    "FREQUENCY_SHIFT", "AMPLITUDE_MOD", "FILTER_MOD", "PITCH_SYNC",
    "TIMBRE_BLEND", "ENVELOPE_LINK", "HARMONIC_FOLD", "CHAOS_INJECT",
    "RESONANCE_SHARE", "SPATIAL_COUPLE", "SPECTRAL_MORPH", "VELOCITY_COUPLE"
]

MOOD_ENGINES = {
    "Atmosphere": [
        "OPAL", "ORACLE", "OBSCURA", "OCEANIC", "OPTIC", "OBLIQUE",
        "OSPREY", "ORBITAL", "ORGANON", "ORIGAMI", "ODDFELIX", "ODDOSCAR"
    ],
    "Aether": [
        "OPAL", "ORACLE", "OBSCURA", "OCEANIC", "ORBITAL", "ORGANON",
        "OBLIQUE", "ORIGAMI", "ODDFELIX", "ODDOSCAR", "OSPREY", "OMBRE", "OVERWORLD"
    ],
    "Prism": [
        "OBLIQUE", "ORIGAMI", "ORACLE", "OPTIC", "OCELOT", "OSTERIA",
        "OWLFISH", "OHM", "OLE", "ORPHICA", "OTTONI", "OVERWORLD"
    ],
}

MOOD_SUFFIX = {
    "Atmosphere": "ATM_AGG",
    "Aether": "AET_AGG",
    "Prism": "PRS_AGG",
}

# Corner definitions per mood: list of (label_parts, dna_overrides)
# Each corner specifies which DNA axes are XHIGH or XLOW; aggression is always XHIGH.
# Unlisted axes are non-extreme (0.3–0.7).

def xhigh():
    return round(random.uniform(0.87, 0.99), 3)

def xlow():
    return round(random.uniform(0.02, 0.13), 3)

def mid():
    return round(random.uniform(0.3, 0.7), 3)

# Corner specs: dict of axis -> 'H'|'L'|'M'
# aggression is always 'H'

ATMOSPHERE_CORNERS = [
    # 1. brightness-H + movement-H + aggression-H + space-H
    {"label": "BRIGHT_KINETIC_VAST_VIOLENT", "axes": {"brightness": "H", "warmth": "M", "movement": "H", "density": "M", "space": "H", "aggression": "H"}},
    # 2. brightness-L + warmth-L + movement-H + aggression-H
    {"label": "DARK_COLD_KINETIC_VIOLENT", "axes": {"brightness": "L", "warmth": "L", "movement": "H", "density": "M", "space": "M", "aggression": "H"}},
    # 3. brightness-H + warmth-H + density-H + aggression-H
    {"label": "BRIGHT_WARM_DENSE_VIOLENT", "axes": {"brightness": "H", "warmth": "H", "movement": "M", "density": "H", "space": "M", "aggression": "H"}},
    # 4. brightness-L + movement-H + density-H + aggression-H
    {"label": "DARK_KINETIC_DENSE_VIOLENT", "axes": {"brightness": "L", "warmth": "M", "movement": "H", "density": "H", "space": "M", "aggression": "H"}},
    # 5. warmth-H + movement-H + space-H + aggression-H
    {"label": "WARM_KINETIC_VAST_VIOLENT", "axes": {"brightness": "M", "warmth": "H", "movement": "H", "density": "M", "space": "H", "aggression": "H"}},
    # 6. brightness-H + warmth-L + density-H + aggression-H
    {"label": "BRIGHT_COLD_DENSE_VIOLENT", "axes": {"brightness": "H", "warmth": "L", "movement": "M", "density": "H", "space": "M", "aggression": "H"}},
    # 7. brightness-L + warmth-H + space-L + aggression-H
    {"label": "DARK_WARM_TIGHT_VIOLENT", "axes": {"brightness": "L", "warmth": "H", "movement": "M", "density": "M", "space": "L", "aggression": "H"}},
    # 8. brightness-H + movement-H + space-L + aggression-H
    {"label": "BRIGHT_KINETIC_TIGHT_VIOLENT", "axes": {"brightness": "H", "warmth": "M", "movement": "H", "density": "M", "space": "L", "aggression": "H"}},
]

AETHER_CORNERS = [
    # 1. brightness-H + warmth-H + space-H + aggression-H
    {"label": "BRIGHT_WARM_VAST_VIOLENT", "axes": {"brightness": "H", "warmth": "H", "movement": "M", "density": "M", "space": "H", "aggression": "H"}},
    # 2. brightness-L + movement-H + space-H + aggression-H
    {"label": "DARK_KINETIC_VAST_VIOLENT", "axes": {"brightness": "L", "warmth": "M", "movement": "H", "density": "M", "space": "H", "aggression": "H"}},
    # 3. brightness-H + density-H + space-H + aggression-H
    {"label": "BRIGHT_DENSE_VAST_VIOLENT", "axes": {"brightness": "H", "warmth": "M", "movement": "M", "density": "H", "space": "H", "aggression": "H"}},
    # 4. warmth-L + movement-H + density-H + aggression-H
    {"label": "COLD_KINETIC_DENSE_VIOLENT", "axes": {"brightness": "M", "warmth": "L", "movement": "H", "density": "H", "space": "M", "aggression": "H"}},
    # 5. brightness-H + warmth-L + space-L + aggression-H
    {"label": "BRIGHT_COLD_TIGHT_VIOLENT", "axes": {"brightness": "H", "warmth": "L", "movement": "M", "density": "M", "space": "L", "aggression": "H"}},
    # 6. brightness-L + warmth-H + density-H + aggression-H
    {"label": "DARK_WARM_DENSE_VIOLENT", "axes": {"brightness": "L", "warmth": "H", "movement": "M", "density": "H", "space": "M", "aggression": "H"}},
    # 7. movement-H + density-L + space-H + aggression-H
    {"label": "KINETIC_SPARSE_VAST_VIOLENT", "axes": {"brightness": "M", "warmth": "M", "movement": "H", "density": "L", "space": "H", "aggression": "H"}},
    # 8. brightness-H + warmth-H + movement-H + aggression-H
    {"label": "BRIGHT_WARM_KINETIC_VIOLENT", "axes": {"brightness": "H", "warmth": "H", "movement": "H", "density": "M", "space": "M", "aggression": "H"}},
]

PRISM_CORNERS = [
    # 1. brightness-H + warmth-H + density-H + aggression-H
    {"label": "BRIGHT_WARM_DENSE_VIOLENT", "axes": {"brightness": "H", "warmth": "H", "movement": "M", "density": "H", "space": "M", "aggression": "H"}},
    # 2. brightness-L + movement-H + density-H + aggression-H
    {"label": "DARK_KINETIC_DENSE_VIOLENT", "axes": {"brightness": "L", "warmth": "M", "movement": "H", "density": "H", "space": "M", "aggression": "H"}},
    # 3. brightness-H + warmth-L + movement-H + aggression-H
    {"label": "BRIGHT_COLD_KINETIC_VIOLENT", "axes": {"brightness": "H", "warmth": "L", "movement": "H", "density": "M", "space": "M", "aggression": "H"}},
    # 4. brightness-L + warmth-H + space-L + aggression-H
    {"label": "DARK_WARM_TIGHT_VIOLENT", "axes": {"brightness": "L", "warmth": "H", "movement": "M", "density": "M", "space": "L", "aggression": "H"}},
    # 5. brightness-H + density-H + space-L + aggression-H
    {"label": "BRIGHT_DENSE_TIGHT_VIOLENT", "axes": {"brightness": "H", "warmth": "M", "movement": "M", "density": "H", "space": "L", "aggression": "H"}},
    # 6. warmth-L + density-H + space-H + aggression-H
    {"label": "COLD_DENSE_VAST_VIOLENT", "axes": {"brightness": "M", "warmth": "L", "movement": "M", "density": "H", "space": "H", "aggression": "H"}},
    # 7. brightness-L + warmth-L + movement-H + aggression-H
    {"label": "DARK_COLD_KINETIC_VIOLENT", "axes": {"brightness": "L", "warmth": "L", "movement": "H", "density": "M", "space": "M", "aggression": "H"}},
    # 8. brightness-H + warmth-H + space-H + aggression-H
    {"label": "BRIGHT_WARM_VAST_VIOLENT", "axes": {"brightness": "H", "warmth": "H", "movement": "M", "density": "M", "space": "H", "aggression": "H"}},
]

MOOD_CORNERS = {
    "Atmosphere": ATMOSPHERE_CORNERS,
    "Aether": AETHER_CORNERS,
    "Prism": PRISM_CORNERS,
}

MOOD_TAGS = {
    "Atmosphere": ["atmosphere", "aggressive", "turbulent", "violent"],
    "Aether": ["aether", "aggressive", "transcendent", "violent"],
    "Prism": ["prism", "aggressive", "chromatic", "violent"],
}

def resolve_axis(spec):
    if spec == "H":
        return xhigh()
    elif spec == "L":
        return xlow()
    else:
        return mid()

def build_dna(axes):
    return {axis: resolve_axis(spec) for axis, spec in axes.items()}

def build_parameters(engines, dna):
    params = {}
    for engine in engines:
        # Map DNA axes to macro params with slight per-engine variation
        params[engine] = {
            "macro_character": round(dna["brightness"] * random.uniform(0.92, 1.0), 3),
            "macro_movement": round(dna["movement"] * random.uniform(0.92, 1.0), 3),
            "macro_coupling": round(dna["aggression"] * random.uniform(0.88, 1.0), 3),
            "macro_space": round(dna["space"] * random.uniform(0.92, 1.0), 3),
        }
        # Clamp all values to [0.0, 1.0]
        for k in params[engine]:
            params[engine][k] = max(0.0, min(1.0, params[engine][k]))
    return params

def generate_preset(mood, corner, variant_idx, engines):
    suffix = MOOD_SUFFIX[mood]
    label = corner["label"]
    name = f"{label}_{suffix}_{variant_idx}"
    filename = f"{label}_{suffix}_{variant_idx:02d}.xometa"

    # Pick 2 engines for this preset
    engine_pair = random.sample(engines, 2)

    dna = build_dna(corner["axes"])

    parameters = build_parameters(engine_pair, dna)

    coupling = {
        "type": random.choice(COUPLING_TYPES),
        "source": engine_pair[0],
        "target": engine_pair[1],
        "amount": round(random.uniform(0.7, 0.99), 3),
    }

    macros = {
        "CHARACTER": round((parameters[engine_pair[0]]["macro_character"] + parameters[engine_pair[1]]["macro_character"]) / 2, 3),
        "MOVEMENT": round((parameters[engine_pair[0]]["macro_movement"] + parameters[engine_pair[1]]["macro_movement"]) / 2, 3),
        "COUPLING": round((parameters[engine_pair[0]]["macro_coupling"] + parameters[engine_pair[1]]["macro_coupling"]) / 2, 3),
        "SPACE": round((parameters[engine_pair[0]]["macro_space"] + parameters[engine_pair[1]]["macro_space"]) / 2, 3),
    }

    tags = list(MOOD_TAGS[mood])
    # Add contextual tags based on corner
    if dna["brightness"] >= 0.87:
        tags.append("bright")
    elif dna["brightness"] <= 0.13:
        tags.append("dark")
    if dna["movement"] >= 0.87:
        tags.append("kinetic")
    if dna["density"] >= 0.87:
        tags.append("dense")
    if dna["space"] >= 0.87:
        tags.append("vast")
    elif dna["space"] <= 0.13:
        tags.append("tight")

    preset = {
        "name": name.replace("_", " "),
        "version": "1.0",
        "mood": mood,
        "engines": engine_pair,
        "parameters": parameters,
        "coupling": coupling,
        "dna": dna,
        "macros": macros,
        "tags": tags,
    }

    return filename, preset

def main():
    total_generated = 0
    results = {}

    for mood in ["Atmosphere", "Aether", "Prism"]:
        out_dir = PRESETS_BASE / mood
        out_dir.mkdir(parents=True, exist_ok=True)

        corners = MOOD_CORNERS[mood]
        engines = MOOD_ENGINES[mood]
        suffix = MOOD_SUFFIX[mood]

        mood_count = 0
        aggression_values = []

        for corner_idx, corner in enumerate(corners):
            for variant in range(1, 6):  # 5 variants per corner
                filename, preset = generate_preset(mood, corner, variant, engines)
                filepath = out_dir / filename

                with open(filepath, "w") as f:
                    json.dump(preset, f, indent=2)

                agg = preset["dna"]["aggression"]
                aggression_values.append(agg)
                mood_count += 1

                assert agg >= 0.87, f"FAIL: {filename} has aggression={agg} < 0.87"

        total_generated += mood_count
        min_agg = min(aggression_values)
        max_agg = max(aggression_values)
        avg_agg = sum(aggression_values) / len(aggression_values)

        results[mood] = {
            "count": mood_count,
            "min_aggression": round(min_agg, 3),
            "max_aggression": round(max_agg, 3),
            "avg_aggression": round(avg_agg, 3),
            "all_xhigh": all(v >= 0.87 for v in aggression_values),
        }

        print(f"{mood}: {mood_count} presets | aggression min={min_agg:.3f} max={max_agg:.3f} avg={avg_agg:.3f} | all_xhigh={results[mood]['all_xhigh']}")

    print(f"\nTotal generated: {total_generated} presets")
    print("\nVerification summary:")
    for mood, r in results.items():
        status = "PASS" if r["all_xhigh"] and r["count"] == 40 else "FAIL"
        print(f"  [{status}] {mood}: {r['count']} presets, aggression {r['min_aggression']}–{r['max_aggression']}")

    assert total_generated == 120, f"Expected 120 presets, got {total_generated}"
    print("\nAll assertions passed. 120 aggression-XHIGH correction presets written.")

if __name__ == "__main__":
    main()
