#!/usr/bin/env python3
"""
xpn_aether_dark_cold_expansion.py
Generate 50 dark/cold extreme Aether presets to diversify the mood
away from mid-range transcendent clusters.

Dark Aether (25):  brightness 0.04-0.14, space 0.75-0.97
Frozen Aether (25): warmth 0.03-0.11, brightness 0.70-0.90, space 0.80-0.97
"""

import json
import os
import random

OUTPUT_DIR = os.path.join(
    os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
    "Presets", "XOmnibus", "Aether"
)

random.seed(42)

# ---------------------------------------------------------------------------
# Dark Aether presets — 25 presets
# brightness 0.04-0.14, space 0.75-0.97, movement 0.20-0.60
# Engines: OBSIDIAN, OBSCURA, OMBRE, ORACLE, OUROBOROS, OCEANIC, ODDOSCAR,
#          OPAL, OVERDUB, ORGANON, OSPREY
# ---------------------------------------------------------------------------

DARK_ENGINES = [
    "OBSIDIAN", "OBSCURA", "OMBRE", "ORACLE", "OUROBOROS",
    "OCEANIC", "ODDOSCAR", "OPAL", "OVERDUB", "ORGANON", "OSPREY",
]

DARK_PRESETS = [
    # (name, engine, brightness, warmth, movement, density, aggression, extra_tags)
    ("Void Communion",      "OBSIDIAN",   0.06, 0.25, 0.22, 0.12, 0.04, ["void", "communion"]),
    ("Event Horizon",       "OBSCURA",    0.05, 0.18, 0.35, 0.15, 0.06, ["event-horizon", "cosmic"]),
    ("Shadow Cathedral",    "OMBRE",      0.09, 0.30, 0.28, 0.18, 0.03, ["shadow", "cathedral"]),
    ("Oracle Of Darkness",  "ORACLE",     0.07, 0.20, 0.40, 0.10, 0.05, ["oracle", "prophecy"]),
    ("Eternal Coil",        "OUROBOROS",  0.08, 0.22, 0.45, 0.20, 0.07, ["eternal", "ouroboros"]),
    ("Abyssal Current",     "OCEANIC",    0.12, 0.35, 0.30, 0.25, 0.04, ["abyssal", "current"]),
    ("Oscar's Tomb",        "ODDOSCAR",   0.06, 0.28, 0.25, 0.14, 0.03, ["oscar", "tomb"]),
    ("Black Granules",      "OPAL",       0.10, 0.15, 0.38, 0.22, 0.05, ["granular", "black"]),
    ("Dub Oblivion",        "OVERDUB",    0.08, 0.32, 0.42, 0.18, 0.06, ["dub", "oblivion"]),
    ("Organ Of Night",      "ORGANON",    0.11, 0.26, 0.27, 0.16, 0.04, ["organ", "night"]),
    ("Osprey Descends",     "OSPREY",     0.09, 0.20, 0.35, 0.12, 0.05, ["osprey", "descent"]),
    ("Null Space",          "OBSIDIAN",   0.05, 0.12, 0.20, 0.08, 0.03, ["null", "void"]),
    ("Penumbra Drift",      "OBSCURA",    0.13, 0.28, 0.50, 0.20, 0.04, ["penumbra", "drift"]),
    ("Ombre Void",          "OMBRE",      0.07, 0.18, 0.32, 0.15, 0.05, ["ombre", "void"]),
    ("Dark Prophecy",       "ORACLE",     0.10, 0.22, 0.55, 0.17, 0.06, ["prophecy", "dark"]),
    ("Serpent Abyss",       "OUROBOROS",  0.06, 0.20, 0.48, 0.22, 0.08, ["serpent", "abyss"]),
    ("Hadal Whisper",       "OCEANIC",    0.14, 0.38, 0.25, 0.28, 0.03, ["hadal", "whisper"]),
    ("Odd Dark Matter",     "ODDOSCAR",   0.08, 0.24, 0.30, 0.16, 0.04, ["dark-matter", "odd"]),
    ("Obsidian Rain",       "OPAL",       0.11, 0.18, 0.42, 0.24, 0.05, ["obsidian", "rain"]),
    ("Tape Silence",        "OVERDUB",    0.05, 0.30, 0.22, 0.12, 0.03, ["tape", "silence"]),
    ("Logos Of Void",       "ORGANON",    0.13, 0.24, 0.38, 0.18, 0.04, ["logos", "void"]),
    ("Nightfall Raptor",    "OSPREY",     0.07, 0.16, 0.45, 0.14, 0.06, ["nightfall", "raptor"]),
    ("Lightless Cavern",    "OBSIDIAN",   0.04, 0.10, 0.28, 0.10, 0.03, ["lightless", "cavern"]),
    ("Blur Singularity",    "OBSCURA",    0.09, 0.22, 0.58, 0.20, 0.05, ["blur", "singularity"]),
    ("Inky Transcendence",  "OMBRE",      0.12, 0.30, 0.35, 0.18, 0.04, ["inky", "transcendence"]),
]

# ---------------------------------------------------------------------------
# Frozen Aether presets — 25 presets
# warmth 0.03-0.11, brightness 0.70-0.90, space 0.80-0.97
# Engines: ODDFELIX, OPTIC, ORPHICA, OCELOT, OVERWORLD, ORIGAMI, ORBITAL,
#          OBLIQUE, OCTOPUS, OHM, ONSET, OBBLIGATO
# ---------------------------------------------------------------------------

FROZEN_ENGINES = [
    "ODDFELIX", "OPTIC", "ORPHICA", "OCELOT", "OVERWORLD",
    "ORIGAMI", "ORBITAL", "OBLIQUE", "OCTOPUS", "OHM", "ONSET", "OBBLIGATO",
]

FROZEN_PRESETS = [
    # (name, engine, brightness, warmth, movement, density, aggression, extra_tags)
    ("Arctic Harp",         "ORPHICA",    0.82, 0.05, 0.20, 0.08, 0.03, ["arctic", "harp"]),
    ("Felix Ice Field",     "ODDFELIX",   0.78, 0.04, 0.15, 0.10, 0.02, ["felix", "ice"]),
    ("Optic Frost",         "OPTIC",      0.88, 0.06, 0.25, 0.12, 0.04, ["optic", "frost"]),
    ("Ocelot Glacier",      "OCELOT",     0.75, 0.08, 0.18, 0.09, 0.03, ["glacier", "ocelot"]),
    ("Chip Tundra",         "OVERWORLD",  0.85, 0.05, 0.30, 0.14, 0.05, ["chip", "tundra"]),
    ("Ice Origami",         "ORIGAMI",    0.80, 0.07, 0.22, 0.11, 0.03, ["ice", "origami"]),
    ("Orbital Winter",      "ORBITAL",    0.87, 0.04, 0.28, 0.10, 0.04, ["orbital", "winter"]),
    ("Oblique Crystal",     "OBLIQUE",    0.73, 0.09, 0.35, 0.16, 0.05, ["oblique", "crystal"]),
    ("Tentacle Ice",        "OCTOPUS",    0.76, 0.06, 0.40, 0.18, 0.06, ["tentacle", "ice"]),
    ("OHM Below Zero",      "OHM",        0.83, 0.05, 0.20, 0.08, 0.03, ["ohm", "zero"]),
    ("Frozen Onset",        "ONSET",      0.79, 0.08, 0.45, 0.20, 0.07, ["frozen", "percussion"]),
    ("Wind Bond Freeze",    "OBBLIGATO",  0.86, 0.04, 0.18, 0.09, 0.03, ["wind", "bond", "freeze"]),
    ("Polar Resonance",     "ORPHICA",    0.90, 0.03, 0.15, 0.07, 0.02, ["polar", "resonance"]),
    ("Cryogenic Felix",     "ODDFELIX",   0.84, 0.05, 0.22, 0.12, 0.03, ["cryogenic", "felix"]),
    ("Permafrost Beam",     "OPTIC",      0.77, 0.07, 0.32, 0.14, 0.04, ["permafrost", "beam"]),
    ("Snow Leopard Glide",  "OCELOT",     0.81, 0.09, 0.28, 0.11, 0.04, ["snow", "leopard"]),
    ("NES Blizzard",        "OVERWORLD",  0.88, 0.04, 0.38, 0.16, 0.06, ["nes", "blizzard"]),
    ("Paper Snow",          "ORIGAMI",    0.74, 0.10, 0.20, 0.08, 0.02, ["paper", "snow"]),
    ("Frozen Ellipse",      "ORBITAL",    0.83, 0.06, 0.25, 0.10, 0.03, ["ellipse", "frozen"]),
    ("Oblique Icicle",      "OBLIQUE",    0.70, 0.11, 0.42, 0.18, 0.05, ["icicle", "oblique"]),
    ("Ice Kraken",          "OCTOPUS",    0.85, 0.05, 0.50, 0.22, 0.07, ["kraken", "ice"]),
    ("Resonant Zero",       "OHM",        0.89, 0.04, 0.18, 0.07, 0.02, ["resonant", "zero"]),
    ("Glacial Machine",     "ONSET",      0.76, 0.09, 0.55, 0.24, 0.08, ["glacial", "machine"]),
    ("Brass Frost",         "OBBLIGATO",  0.82, 0.06, 0.24, 0.10, 0.03, ["brass", "frost"]),
    ("Crystalline Zenith",  "ORPHICA",    0.87, 0.03, 0.12, 0.06, 0.02, ["crystalline", "zenith"]),
]


def engine_macro_key(engine):
    """Return the macro param prefix for a given engine."""
    prefix_map = {
        "OBSIDIAN":  "obsidian",
        "OBSCURA":   "obscura",
        "OMBRE":     "ombre",
        "ORACLE":    "oracle",
        "OUROBOROS": "ouro",
        "OCEANIC":   "oceanic",
        "ODDOSCAR":  "osco",
        "OPAL":      "opal",
        "OVERDUB":   "dub",
        "ORGANON":   "org",
        "OSPREY":    "osp",
        "ODDFELIX":  "felx",
        "OPTIC":     "optic",
        "ORPHICA":   "orph",
        "OCELOT":    "ocel",
        "OVERWORLD": "ow",
        "ORIGAMI":   "orig",
        "ORBITAL":   "orb",
        "OBLIQUE":   "oblq",
        "OCTOPUS":   "oct",
        "OHM":       "ohm",
        "ONSET":     "onset",
        "OBBLIGATO": "obbl",
    }
    return prefix_map.get(engine, engine.lower()[:4])


def build_dark_preset(name, engine, brightness, warmth, movement, density, aggression, extra_tags):
    space = round(random.uniform(0.75, 0.97), 2)
    prefix = engine_macro_key(engine)
    slug = name.replace(" ", "_").replace("'", "")
    return slug, {
        "name": name,
        "version": "1.0",
        "mood": "Aether",
        "engines": [engine],
        "parameters": {
            engine: {
                f"{prefix}_macro_character":  round(random.uniform(0.2, 0.5), 2),
                f"{prefix}_macro_movement":   round(movement + random.uniform(-0.05, 0.05), 2),
                f"{prefix}_macro_coupling":   round(random.uniform(0.15, 0.45), 2),
                f"{prefix}_macro_space":      space,
            }
        },
        "dna": {
            "brightness": brightness,
            "warmth":     warmth,
            "movement":   round(movement, 2),
            "density":    density,
            "space":      space,
            "aggression": aggression,
        },
        "macros": {
            "CHARACTER": round(random.uniform(0.2, 0.5), 2),
            "MOVEMENT":  round(movement, 2),
            "COUPLING":  round(random.uniform(0.15, 0.45), 2),
            "SPACE":     space,
        },
        "tags": ["aether", "dark", "void"] + extra_tags,
    }


def build_frozen_preset(name, engine, brightness, warmth, movement, density, aggression, extra_tags):
    space = round(random.uniform(0.80, 0.97), 2)
    prefix = engine_macro_key(engine)
    slug = name.replace(" ", "_").replace("'", "")
    return slug, {
        "name": name,
        "version": "1.0",
        "mood": "Aether",
        "engines": [engine],
        "parameters": {
            engine: {
                f"{prefix}_macro_character":  round(random.uniform(0.15, 0.45), 2),
                f"{prefix}_macro_movement":   round(movement + random.uniform(-0.05, 0.05), 2),
                f"{prefix}_macro_coupling":   round(random.uniform(0.10, 0.40), 2),
                f"{prefix}_macro_space":      space,
            }
        },
        "dna": {
            "brightness": brightness,
            "warmth":     warmth,
            "movement":   round(movement, 2),
            "density":    density,
            "space":      space,
            "aggression": aggression,
        },
        "macros": {
            "CHARACTER": round(random.uniform(0.15, 0.45), 2),
            "MOVEMENT":  round(movement, 2),
            "COUPLING":  round(random.uniform(0.10, 0.40), 2),
            "SPACE":     space,
        },
        "tags": ["aether", "frozen", "crystalline"] + extra_tags,
    }


def main():
    os.makedirs(OUTPUT_DIR, exist_ok=True)

    written = 0
    skipped = 0

    all_presets = []

    for row in DARK_PRESETS:
        name, engine, brightness, warmth, movement, density, aggression, extra_tags = row
        slug, data = build_dark_preset(name, engine, brightness, warmth, movement, density, aggression, extra_tags)
        all_presets.append((slug, data))

    for row in FROZEN_PRESETS:
        name, engine, brightness, warmth, movement, density, aggression, extra_tags = row
        slug, data = build_frozen_preset(name, engine, brightness, warmth, movement, density, aggression, extra_tags)
        all_presets.append((slug, data))

    for slug, data in all_presets:
        filename = f"{slug}.xometa"
        filepath = os.path.join(OUTPUT_DIR, filename)
        if os.path.exists(filepath):
            print(f"  SKIP  {filename}")
            skipped += 1
            continue
        with open(filepath, "w") as f:
            json.dump(data, f, indent=2)
        print(f"  WRITE {filename}")
        written += 1

    print(f"\nDone. Written: {written}  Skipped: {skipped}  Total attempted: {len(all_presets)}")
    print(f"Output dir: {OUTPUT_DIR}")


if __name__ == "__main__":
    main()
