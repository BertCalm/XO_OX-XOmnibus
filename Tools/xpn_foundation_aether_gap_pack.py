#!/usr/bin/env python3
"""
xpn_foundation_aether_gap_pack.py
Generates 80 .xometa presets filling critical per-mood DNA gaps:
  - 40 Foundation presets with space >= 0.90 (space-XHIGH)
  - 40 Aether presets with aggression >= 0.88 (aggression-XHIGH)
"""

import json
import os
import random

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

FOUNDATION_NAMES = [
    "OPEN FOUNDATION", "VAST GROUND", "INFINITE BASE", "ENDLESS FLOOR",
    "OPEN EXPANSE", "VAST HORIZON", "INFINITE PLAIN", "ENDLESS VOID",
    "OPEN SKY GROUND", "VAST OPEN BASE", "INFINITE TERRAIN", "ENDLESS SPACE FLOOR",
    "OPEN COSMOS BASE", "VAST FOUNDATION DRIFT", "INFINITE OPEN GROUND", "ENDLESS OPEN PLAIN",
    "OPEN FIELD VAST", "VAST STILL GROUND", "INFINITE BREATHING BASE", "ENDLESS DEEP FLOOR",
    "OPEN DEEP FOUNDATION", "VAST RESONANT GROUND", "INFINITE SILENT BASE", "ENDLESS WIDE FLOOR",
    "OPEN CATHEDRAL GROUND", "VAST CATHEDRAL BASE", "INFINITE HALL FLOOR", "ENDLESS HALL GROUND",
    "OPEN OCEAN FLOOR", "VAST OCEAN BASE", "INFINITE OCEAN GROUND", "ENDLESS OCEAN PLAIN",
    "OPEN TUNDRA BASE", "VAST TUNDRA FLOOR", "INFINITE TUNDRA GROUND", "ENDLESS TUNDRA PLAIN",
    "OPEN MESA FOUNDATION", "VAST MESA GROUND", "INFINITE MESA PLAIN", "ENDLESS MESA BASE",
]

AETHER_NAMES = [
    "FURIOUS ASCENT", "WRATHFUL ETHER", "AGGRESSIVE VOID", "SAVAGE TRANSCENDENCE",
    "FURIOUS TRANSCENDENCE", "WRATHFUL ASCENT", "AGGRESSIVE ETHER", "SAVAGE VOID",
    "FURIOUS VOID RISE", "WRATHFUL COSMIC SURGE", "AGGRESSIVE AETHER STORM", "SAVAGE ASCENT FURY",
    "FURIOUS LIGHT SHATTER", "WRATHFUL NEBULA BURST", "AGGRESSIVE STAR COLLAPSE", "SAVAGE PLASMA SURGE",
    "FURIOUS ETHER STORM", "WRATHFUL LIGHT BURST", "AGGRESSIVE COSMIC TEAR", "SAVAGE ETHER COLLAPSE",
    "FURIOUS TRANSCENDENT SURGE", "WRATHFUL AETHER BURST", "AGGRESSIVE VOID STORM", "SAVAGE LIGHT TEAR",
    "FURIOUS PLASMA ASCENT", "WRATHFUL STAR FURY", "AGGRESSIVE NEBULA TEAR", "SAVAGE COSMIC BURST",
    "FURIOUS SKY SHATTER", "WRATHFUL SKY BURST", "AGGRESSIVE SKY STORM", "SAVAGE SKY COLLAPSE",
    "FURIOUS ETHER RISE", "WRATHFUL ETHER SURGE", "AGGRESSIVE ETHER RISE", "SAVAGE ETHER SURGE",
    "FURIOUS VOID BURST", "WRATHFUL VOID TEAR", "AGGRESSIVE VOID RISE", "SAVAGE VOID FURY",
]


def rand_extreme_low():
    return round(random.uniform(0.0, 0.15), 3)

def rand_extreme_high():
    return round(random.uniform(0.85, 0.99), 3)

def rand_free():
    return round(random.uniform(0.0, 1.0), 3)

def rand_space_xhigh():
    return round(random.uniform(0.90, 0.99), 3)

def rand_movement_xhigh():
    return round(random.uniform(0.88, 0.99), 3)

def rand_aggression_xhigh():
    return round(random.uniform(0.88, 0.99), 3)

def at_least_one_extreme(dna: dict, exclude_keys=None) -> dict:
    """Ensure at least one free DNA dimension has an extreme value."""
    exclude_keys = exclude_keys or []
    free_keys = [k for k in dna if k not in exclude_keys]
    # Check if any already extreme
    already_extreme = any(
        dna[k] <= 0.15 or dna[k] >= 0.85
        for k in free_keys
    )
    if not already_extreme and free_keys:
        chosen = random.choice(free_keys)
        dna[chosen] = rand_extreme_high() if random.random() > 0.5 else rand_extreme_low()
    return dna


def make_preset(name, mood, engine1, engine2, coupling_type, dna: dict, tags: list) -> dict:
    macro_space = dna.get("space", rand_free())
    macro_movement = dna.get("movement", rand_free())

    return {
        "name": name,
        "version": "1.0",
        "mood": mood,
        "engines": [engine1, engine2],
        "parameters": {
            engine1: {
                "macro_character": rand_free(),
                "macro_movement": round(macro_movement + random.uniform(-0.05, 0.05), 3),
                "macro_coupling": rand_free(),
                "macro_space": round(macro_space + random.uniform(-0.03, 0.03), 3),
            },
            engine2: {
                "macro_character": rand_free(),
                "macro_movement": round(macro_movement + random.uniform(-0.05, 0.05), 3),
                "macro_coupling": rand_free(),
                "macro_space": round(macro_space + random.uniform(-0.03, 0.03), 3),
            },
        },
        "coupling": {
            "type": coupling_type,
            "source": engine1,
            "target": engine2,
            "amount": round(random.uniform(0.3, 1.0), 3),
        },
        "dna": dna,
        "macros": {
            "CHARACTER": rand_free(),
            "MOVEMENT": round(macro_movement, 3),
            "COUPLING": rand_free(),
            "SPACE": round(macro_space, 3),
        },
        "tags": tags,
    }


def clamp(v, lo=0.0, hi=1.0):
    return round(max(lo, min(hi, v)), 3)


def generate_foundation_presets(count=40):
    presets = []
    # Cycle through coupling types evenly
    coupling_cycle = COUPLING_TYPES * (count // len(COUPLING_TYPES) + 1)
    random.shuffle(coupling_cycle)

    # Engine pairs — shuffle engines and take pairs
    engine_pool = ENGINES * (count // len(ENGINES) + 2)
    random.shuffle(engine_pool)

    for i in range(count):
        name = FOUNDATION_NAMES[i % len(FOUNDATION_NAMES)]
        coupling_type = coupling_cycle[i]

        # Engine pair — ensure distinct
        e1 = engine_pool[i * 2 % len(engine_pool)]
        e2 = engine_pool[(i * 2 + 1) % len(engine_pool)]
        while e2 == e1:
            e2 = random.choice(ENGINES)

        space = rand_space_xhigh()

        # ~30% of Foundation presets get movement-XHIGH
        if i < 12:
            movement = rand_movement_xhigh()
        else:
            movement = rand_free()

        dna = {
            "brightness": rand_free(),
            "warmth": rand_free(),
            "movement": movement,
            "density": rand_free(),
            "space": space,
            "aggression": rand_free(),
        }
        dna = at_least_one_extreme(dna, exclude_keys=["space"])

        tags = ["foundation", "vast", "open", "space-xhigh"]
        if movement >= 0.88:
            tags.append("movement-xhigh")

        presets.append(make_preset(name, "Foundation", e1, e2, coupling_type, dna, tags))

    return presets


def generate_aether_presets(count=40):
    presets = []
    coupling_cycle = COUPLING_TYPES * (count // len(COUPLING_TYPES) + 1)
    random.shuffle(coupling_cycle)

    engine_pool = ENGINES * (count // len(ENGINES) + 2)
    random.shuffle(engine_pool)

    for i in range(count):
        name = AETHER_NAMES[i % len(AETHER_NAMES)]
        coupling_type = coupling_cycle[i]

        e1 = engine_pool[i * 2 % len(engine_pool)]
        e2 = engine_pool[(i * 2 + 1) % len(engine_pool)]
        while e2 == e1:
            e2 = random.choice(ENGINES)

        aggression = rand_aggression_xhigh()

        # ~30% of Aether presets get movement-XHIGH
        if i < 12:
            movement = rand_movement_xhigh()
        else:
            movement = rand_free()

        dna = {
            "brightness": rand_free(),
            "warmth": rand_free(),
            "movement": movement,
            "density": rand_free(),
            "space": rand_free(),
            "aggression": aggression,
        }
        dna = at_least_one_extreme(dna, exclude_keys=["aggression"])

        tags = ["aether", "violent", "transcendence", "aggression-xhigh"]
        if movement >= 0.88:
            tags.append("movement-xhigh")

        presets.append(make_preset(name, "Aether", e1, e2, coupling_type, dna, tags))

    return presets


def slugify(name: str) -> str:
    return name.lower().replace(" ", "_").replace("-", "_")


def write_presets(presets, output_dir):
    os.makedirs(output_dir, exist_ok=True)
    written = 0
    skipped = 0
    for preset in presets:
        filename = slugify(preset["name"]) + ".xometa"
        filepath = os.path.join(output_dir, filename)
        if os.path.exists(filepath):
            skipped += 1
            continue
        with open(filepath, "w") as f:
            json.dump(preset, f, indent=2)
        written += 1
    return written, skipped


def main():
    random.seed(42)

    base_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    foundation_dir = os.path.join(base_dir, "Presets", "Foundation")
    aether_dir = os.path.join(base_dir, "Presets", "Aether")

    print("Generating Foundation presets (space-XHIGH)...")
    foundation_presets = generate_foundation_presets(40)
    f_written, f_skipped = write_presets(foundation_presets, foundation_dir)
    print(f"  Written: {f_written} | Skipped (exists): {f_skipped}")
    print(f"  Output dir: {foundation_dir}")

    print("Generating Aether presets (aggression-XHIGH)...")
    aether_presets = generate_aether_presets(40)
    a_written, a_skipped = write_presets(aether_presets, aether_dir)
    print(f"  Written: {a_written} | Skipped (exists): {a_skipped}")
    print(f"  Output dir: {aether_dir}")

    total = f_written + a_written
    print(f"\nTotal files written: {total} (Foundation: {f_written}, Aether: {a_written})")

    # Validation summary
    foundation_space_values = [p["dna"]["space"] for p in foundation_presets]
    aether_aggression_values = [p["dna"]["aggression"] for p in aether_presets]
    foundation_movement_xhigh = sum(1 for p in foundation_presets if p["dna"]["movement"] >= 0.88)
    aether_movement_xhigh = sum(1 for p in aether_presets if p["dna"]["movement"] >= 0.88)

    print(f"\nValidation:")
    print(f"  Foundation space range: {min(foundation_space_values):.3f} – {max(foundation_space_values):.3f} (all >= 0.90: {all(v >= 0.90 for v in foundation_space_values)})")
    print(f"  Foundation movement-XHIGH count: {foundation_movement_xhigh}/40")
    print(f"  Aether aggression range: {min(aether_aggression_values):.3f} – {max(aether_aggression_values):.3f} (all >= 0.88: {all(v >= 0.88 for v in aether_aggression_values)})")
    print(f"  Aether movement-XHIGH count: {aether_movement_xhigh}/40")

    # Coupling type coverage
    f_couplings = set(p["coupling"]["type"] for p in foundation_presets)
    a_couplings = set(p["coupling"]["type"] for p in aether_presets)
    print(f"  Foundation coupling types used: {len(f_couplings)}/12")
    print(f"  Aether coupling types used: {len(a_couplings)}/12")


if __name__ == "__main__":
    main()
