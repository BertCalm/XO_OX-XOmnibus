#!/usr/bin/env python3
"""
xpn_coverage_final_sprint.py
Scans Entangled presets, finds uncovered engine pairs, generates 2 presets per pair.
"""

import json
import os
import random
import math
from itertools import combinations
from pathlib import Path

ENTANGLED_DIR = Path(__file__).parent.parent / "Presets" / "XOmnibus" / "Entangled"

ALL_ENGINES = [
    "ODDFELIX", "ODDOSCAR", "OVERDUB", "ODYSSEY", "OBLONG", "OBESE",
    "ONSET", "OVERWORLD", "OPAL", "ORBITAL", "ORGANON", "OUROBOROS",
    "OBSIDIAN", "OVERBITE", "ORIGAMI", "ORACLE", "OBSCURA", "OCEANIC",
    "OCELOT", "OPTIC", "OBLIQUE", "OSPREY", "OSTERIA", "OWLFISH",
    "OHM", "ORPHICA", "OBBLIGATO", "OTTONI", "OLE", "OMBRE",
    "ORCA", "OCTOPUS", "OVERLAP", "OUTWIT"
]

COUPLING_TYPES = [
    "TIMBRE_BLEND", "PHASE_LOCK", "HARMONIC_SYNC", "RHYTHM_LOCK",
    "SPECTRAL_BLEND", "ENVELOPE_FOLLOW", "PITCH_TRACK", "FORMANT_BLEND",
    "AMPLITUDE_LOCK", "TEXTURE_WEAVE", "RESONANCE_SHARE", "CHAOS_SYNC"
]

# DNA profile per engine (characteristic tendencies)
ENGINE_DNA = {
    "ODDFELIX":   {"brightness": 0.8, "warmth": 0.4, "movement": 0.7, "density": 0.5, "space": 0.6, "aggression": 0.3},
    "ODDOSCAR":   {"brightness": 0.3, "warmth": 0.8, "movement": 0.4, "density": 0.7, "space": 0.5, "aggression": 0.5},
    "OVERDUB":    {"brightness": 0.4, "warmth": 0.7, "movement": 0.6, "density": 0.6, "space": 0.8, "aggression": 0.2},
    "ODYSSEY":    {"brightness": 0.6, "warmth": 0.5, "movement": 0.8, "density": 0.4, "space": 0.7, "aggression": 0.4},
    "OBLONG":     {"brightness": 0.5, "warmth": 0.6, "movement": 0.5, "density": 0.8, "space": 0.4, "aggression": 0.6},
    "OBESE":      {"brightness": 0.3, "warmth": 0.9, "movement": 0.3, "density": 0.9, "space": 0.3, "aggression": 0.7},
    "ONSET":      {"brightness": 0.6, "warmth": 0.4, "movement": 0.9, "density": 0.7, "space": 0.3, "aggression": 0.8},
    "OVERWORLD":  {"brightness": 0.7, "warmth": 0.3, "movement": 0.6, "density": 0.5, "space": 0.5, "aggression": 0.5},
    "OPAL":       {"brightness": 0.5, "warmth": 0.5, "movement": 0.7, "density": 0.6, "space": 0.8, "aggression": 0.2},
    "ORBITAL":    {"brightness": 0.6, "warmth": 0.4, "movement": 0.8, "density": 0.4, "space": 0.9, "aggression": 0.3},
    "ORGANON":    {"brightness": 0.4, "warmth": 0.6, "movement": 0.5, "density": 0.7, "space": 0.6, "aggression": 0.4},
    "OUROBOROS":  {"brightness": 0.5, "warmth": 0.5, "movement": 0.9, "density": 0.5, "space": 0.5, "aggression": 0.6},
    "OBSIDIAN":   {"brightness": 0.1, "warmth": 0.3, "movement": 0.4, "density": 0.8, "space": 0.5, "aggression": 0.8},
    "OVERBITE":   {"brightness": 0.4, "warmth": 0.7, "movement": 0.6, "density": 0.8, "space": 0.4, "aggression": 0.7},
    "ORIGAMI":    {"brightness": 0.7, "warmth": 0.4, "movement": 0.7, "density": 0.3, "space": 0.6, "aggression": 0.3},
    "ORACLE":     {"brightness": 0.5, "warmth": 0.6, "movement": 0.6, "density": 0.6, "space": 0.7, "aggression": 0.4},
    "OBSCURA":    {"brightness": 0.2, "warmth": 0.4, "movement": 0.5, "density": 0.7, "space": 0.8, "aggression": 0.5},
    "OCEANIC":    {"brightness": 0.4, "warmth": 0.6, "movement": 0.7, "density": 0.5, "space": 0.9, "aggression": 0.2},
    "OCELOT":     {"brightness": 0.7, "warmth": 0.5, "movement": 0.8, "density": 0.4, "space": 0.5, "aggression": 0.6},
    "OPTIC":      {"brightness": 0.9, "warmth": 0.2, "movement": 0.7, "density": 0.3, "space": 0.6, "aggression": 0.4},
    "OBLIQUE":    {"brightness": 0.5, "warmth": 0.5, "movement": 0.6, "density": 0.5, "space": 0.7, "aggression": 0.5},
    "OSPREY":     {"brightness": 0.7, "warmth": 0.4, "movement": 0.9, "density": 0.3, "space": 0.7, "aggression": 0.5},
    "OSTERIA":    {"brightness": 0.6, "warmth": 0.8, "movement": 0.4, "density": 0.6, "space": 0.5, "aggression": 0.3},
    "OWLFISH":    {"brightness": 0.4, "warmth": 0.5, "movement": 0.5, "density": 0.6, "space": 0.7, "aggression": 0.3},
    "OHM":        {"brightness": 0.3, "warmth": 0.8, "movement": 0.3, "density": 0.7, "space": 0.6, "aggression": 0.2},
    "ORPHICA":    {"brightness": 0.7, "warmth": 0.5, "movement": 0.8, "density": 0.3, "space": 0.8, "aggression": 0.2},
    "OBBLIGATO":  {"brightness": 0.6, "warmth": 0.5, "movement": 0.6, "density": 0.5, "space": 0.6, "aggression": 0.4},
    "OTTONI":     {"brightness": 0.5, "warmth": 0.6, "movement": 0.5, "density": 0.7, "space": 0.5, "aggression": 0.6},
    "OLE":        {"brightness": 0.7, "warmth": 0.7, "movement": 0.8, "density": 0.5, "space": 0.5, "aggression": 0.5},
    "OMBRE":      {"brightness": 0.4, "warmth": 0.6, "movement": 0.5, "density": 0.6, "space": 0.7, "aggression": 0.3},
    "ORCA":       {"brightness": 0.3, "warmth": 0.5, "movement": 0.7, "density": 0.8, "space": 0.6, "aggression": 0.7},
    "OCTOPUS":    {"brightness": 0.5, "warmth": 0.4, "movement": 0.9, "density": 0.7, "space": 0.5, "aggression": 0.6},
    "OVERLAP":    {"brightness": 0.5, "warmth": 0.5, "movement": 0.4, "density": 0.6, "space": 0.9, "aggression": 0.2},
    "OUTWIT":     {"brightness": 0.6, "warmth": 0.4, "movement": 0.8, "density": 0.5, "space": 0.5, "aggression": 0.6},
}

# Evocative name parts for generating preset names
NAME_PARTS_A = [
    "Abyssal", "Arctic", "Bloom", "Bronze", "Burning", "Cascade", "Chromatic",
    "Cobalt", "Coral", "Dark", "Deep", "Drift", "Dusk", "Ember", "Fade",
    "Feral", "Fractal", "Frozen", "Ghost", "Glass", "Gold", "Hollow",
    "Indigo", "Iron", "Jade", "Knotted", "Lattice", "Liminal", "Lost",
    "Midnight", "Mirror", "Murky", "Neon", "Obsidian", "Ocean", "Open",
    "Prism", "Pulse", "Radiant", "Raw", "Rift", "Ritual", "Rust",
    "Sacred", "Scarlet", "Shallow", "Shattered", "Signal", "Silver",
    "Smoke", "Solar", "Stone", "Storm", "Strange", "Sunken", "Tidal",
    "Timeless", "Twilight", "Vast", "Violet", "Void", "Warm", "Wild"
]

NAME_PARTS_B = [
    "Algorithm", "Altar", "Archive", "Arc", "Basin", "Beacon", "Bell",
    "Bloom", "Bond", "Bridge", "Burn", "Cage", "Canvas", "Chain",
    "Chamber", "Chord", "Circuit", "Compass", "Core", "Cradle", "Crest",
    "Current", "Curve", "Dawn", "Delta", "Depth", "Drift", "Drive",
    "Echo", "Edge", "Engine", "Fiber", "Field", "Filter", "Flame",
    "Flood", "Flow", "Fold", "Form", "Frame", "Gate", "Grid", "Ground",
    "Harbor", "Heart", "Hook", "Horizon", "Hum", "Hunt", "Key",
    "Knot", "Layer", "Loop", "Lure", "Map", "Mass", "Membrane",
    "Mesh", "Mirror", "Mode", "Node", "Notch", "Orbit", "Passage",
    "Path", "Peak", "Phase", "Pillar", "Plane", "Plate", "Portal",
    "Pulse", "Ridge", "Ring", "Root", "Route", "Rune", "Scaffold",
    "Scope", "Seam", "Seed", "Sequence", "Shore", "Shrine", "Signal",
    "Sink", "Source", "Space", "Span", "Spiral", "Spoke", "Stage",
    "State", "Stream", "String", "Surface", "Surge", "Sweep", "System",
    "Temple", "Tide", "Thread", "Threshold", "Tower", "Track", "Trace",
    "Tunnel", "Vessel", "Void", "Vortex", "Wave", "Web", "Wire", "Zone"
]


def blend_dna(dna_a, dna_b, weight_a=0.5):
    """Blend two DNA profiles. Ensure at least one dimension is extreme (<=0.15 or >=0.85)."""
    weight_b = 1.0 - weight_a
    blended = {}
    for key in dna_a:
        blended[key] = round(dna_a[key] * weight_a + dna_b[key] * weight_b, 3)

    # Guarantee at least one extreme dimension
    extremes = {k: v for k, v in blended.items() if v <= 0.15 or v >= 0.85}
    if not extremes:
        # Pick the dimension furthest from 0.5 and push it extreme
        key_to_push = max(blended, key=lambda k: abs(blended[k] - 0.5))
        val = blended[key_to_push]
        if val >= 0.5:
            blended[key_to_push] = min(1.0, round(val + 0.3, 3))
        else:
            blended[key_to_push] = max(0.0, round(val - 0.3, 3))

    return blended


def make_preset_name(eng_a, eng_b, variant, used_names):
    """Generate a unique evocative preset name."""
    random.seed(hash((eng_a, eng_b, variant)) & 0xFFFFFFFF)
    for _ in range(50):
        part_a = random.choice(NAME_PARTS_A)
        part_b = random.choice(NAME_PARTS_B)
        name = f"{part_a} {part_b}"
        if name not in used_names:
            return name
    # Fallback to engine-based name
    return f"{eng_a[:3].title()}{eng_b[:3].title()} {variant}"


def make_filename(name):
    """Convert name to safe filename."""
    safe = name.replace(" ", "_").replace("/", "-").replace("\\", "-")
    return f"{safe}.xometa"


def make_macro_params(dna):
    """Derive macro values from DNA."""
    return {
        "CHARACTER": round(dna["warmth"] * 0.6 + dna["brightness"] * 0.4, 3),
        "MOVEMENT": round(dna["movement"] * 0.7 + dna["aggression"] * 0.3, 3),
        "COUPLING": round(0.5 + (dna["density"] - 0.5) * 0.4 + random.uniform(-0.1, 0.1), 3),
        "SPACE": round(dna["space"] * 0.8 + (1 - dna["density"]) * 0.2, 3),
    }


def make_preset(eng_a, eng_b, variant, dna, coupling_type, used_names):
    """Build a full .xometa preset dict."""
    name = make_preset_name(eng_a, eng_b, variant, used_names)
    used_names.add(name)

    random.seed(hash((eng_a, eng_b, variant, "params")) & 0xFFFFFFFF)

    source = eng_a if variant == "A" else eng_b
    target = eng_b if variant == "A" else eng_a

    coupling_amount = round(random.uniform(0.45, 0.85), 3)
    macro_vals = make_macro_params(dna)

    # Clamp macros
    for k in macro_vals:
        macro_vals[k] = max(0.0, min(1.0, macro_vals[k]))

    preset = {
        "schema_version": 1,
        "name": name,
        "mood": "Entangled",
        "engines": [eng_a, eng_b],
        "author": "XO_OX",
        "version": "1.0.0",
        "tags": ["entangled", "coupling", "coverage",
                 eng_a.lower(), eng_b.lower()],
        "macroLabels": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
        "macros": macro_vals,
        "couplingIntensity": "Moderate" if coupling_amount < 0.65 else "Strong",
        "dna": dna,
        "sonic_dna": dna,
        "parameters": {
            eng_a: {
                "macro_character": round(random.uniform(0.3, 0.8), 3),
                "macro_movement": round(random.uniform(0.3, 0.8), 3),
                "macro_coupling": round(random.uniform(0.5, 0.9), 3),
                "macro_space": round(random.uniform(0.2, 0.7), 3),
            },
            eng_b: {
                "macro_character": round(random.uniform(0.3, 0.8), 3),
                "macro_movement": round(random.uniform(0.3, 0.8), 3),
                "macro_coupling": round(random.uniform(0.4, 0.8), 3),
                "macro_space": round(random.uniform(0.2, 0.7), 3),
            },
        },
        "coupling": {
            "pairs": [
                {
                    "engineA": source,
                    "engineB": target,
                    "type": coupling_type,
                    "amount": coupling_amount,
                }
            ]
        },
    }
    return name, preset


def scan_covered_pairs():
    """Read all .xometa files and extract covered engine pairs."""
    covered = set()
    for path in ENTANGLED_DIR.glob("*.xometa"):
        try:
            with open(path) as f:
                data = json.load(f)
            engines = data.get("engines", [])
            if len(engines) >= 2:
                # Normalize to frozenset so order doesn't matter
                pair = frozenset(e.upper() for e in engines[:2])
                if len(pair) == 2:
                    covered.add(pair)
        except Exception:
            pass
    return covered


def main():
    all_pairs = set(frozenset(p) for p in combinations(ALL_ENGINES, 2))
    total_possible = len(all_pairs)

    print(f"Total possible pairs: C(34,2) = {total_possible}")

    covered = scan_covered_pairs()
    print(f"Currently covered pairs: {len(covered)}")

    uncovered = all_pairs - covered
    print(f"Uncovered pairs: {len(uncovered)}")

    if not uncovered:
        print("Full coverage achieved! Nothing to generate.")
        return

    # Build set of existing filenames to avoid name collisions
    existing_names = set()
    for path in ENTANGLED_DIR.glob("*.xometa"):
        try:
            with open(path) as f:
                data = json.load(f)
            existing_names.add(data.get("name", ""))
        except Exception:
            pass

    # Sort uncovered for determinism
    uncovered_sorted = sorted(uncovered, key=lambda p: sorted(p))

    generated_count = 0
    skipped_count = 0
    coupling_cycle = COUPLING_TYPES.copy()
    coupling_idx = 0

    for pair in uncovered_sorted:
        engines = sorted(pair)
        eng_a, eng_b = engines[0], engines[1]

        dna_a = ENGINE_DNA.get(eng_a, {"brightness": 0.5, "warmth": 0.5, "movement": 0.5,
                                        "density": 0.5, "space": 0.5, "aggression": 0.3})
        dna_b = ENGINE_DNA.get(eng_b, {"brightness": 0.5, "warmth": 0.5, "movement": 0.5,
                                        "density": 0.5, "space": 0.5, "aggression": 0.3})

        for variant in ("A", "B"):
            coupling_type = coupling_cycle[coupling_idx % len(coupling_cycle)]
            coupling_idx += 1

            # Blend DNA: variant A weights eng_a more, variant B weights eng_b more
            weight = 0.65 if variant == "A" else 0.35
            dna = blend_dna(dna_a, dna_b, weight_a=weight)

            preset_name, preset_data = make_preset(
                eng_a, eng_b, variant, dna, coupling_type, existing_names
            )

            filename = make_filename(preset_name)
            out_path = ENTANGLED_DIR / filename

            if out_path.exists():
                skipped_count += 1
                continue

            with open(out_path, "w") as f:
                json.dump(preset_data, f, indent=2)
            generated_count += 1

    print(f"\nResults:")
    print(f"  Uncovered pairs found:   {len(uncovered)}")
    print(f"  Presets generated:       {generated_count}")
    print(f"  Skipped (already exist): {skipped_count}")
    print(f"  Expected (2 per pair):   {len(uncovered) * 2}")

    # Final coverage check
    covered_after = scan_covered_pairs()
    print(f"\nCoverage after sprint:")
    print(f"  Covered pairs:           {len(covered_after)} / {total_possible}")
    print(f"  Coverage %:              {100 * len(covered_after) / total_possible:.1f}%")

    still_uncovered = all_pairs - covered_after
    if still_uncovered:
        print(f"\nStill uncovered ({len(still_uncovered)} pairs):")
        for p in sorted(still_uncovered, key=lambda x: sorted(x)):
            print(f"  {sorted(p)}")
    else:
        print("\n100% pair coverage achieved!")


if __name__ == "__main__":
    main()
