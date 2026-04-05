#!/usr/bin/env python3
"""Generate Submerged mood presets for engines that have zero.

Usage:
    python3 generate_submerged_presets.py           # dry-run (default)
    python3 generate_submerged_presets.py --dry-run # explicit dry-run
    python3 generate_submerged_presets.py --apply   # write files

Step 1: Scan Presets/XOceanus/Submerged/ for engines that already have presets.
Step 2: For each engine with 0 Submerged presets, generate 3 evocative presets
        with appropriate Submerged DNA and macro parameters.
"""

import argparse
import collections
import json
import os
import sys

# ── Engine & Prefix Tables ────────────────────────────────────────────────────

ALL_ENGINES = [
    "OddfeliX", "OddOscar", "Overdub", "Odyssey", "Oblong", "Obese", "Onset",
    "Overworld", "Opal", "Orbital", "Organon", "Ouroboros", "Obsidian", "Overbite",
    "Origami", "Oracle", "Obscura", "Oceanic", "Ocelot", "Optic", "Oblique",
    "Osprey", "Osteria", "Owlfish", "Ohm", "Orphica", "Obbligato", "Ottoni", "Ole",
    "Ombre", "Orca", "Octopus", "Overlap", "Outwit", "Ostinato", "OpenSky",
    "OceanDeep", "Ouie", "Obrix", "Orbweave", "Overtone", "Organism", "Oxbow", "Oware",
]

ENGINE_PREFIXES = {
    "OddfeliX":  "snap_",
    "OddOscar":  "morph_",
    "Overdub":   "dub_",
    "Odyssey":   "drift_",
    "Oblong":    "bob_",
    "Obese":     "fat_",
    "Overbite":  "poss_",
    "Onset":     "perc_",
    "Overworld": "ow_",
    "Opal":      "opal_",
    "Orbital":   "orb_",
    "Organon":   "organon_",
    "Ouroboros": "ouro_",
    "Obsidian":  "obsidian_",
    "Origami":   "origami_",
    "Oracle":    "oracle_",
    "Obscura":   "obscura_",
    "Oceanic":   "ocean_",
    "Ocelot":    "ocelot_",
    "Optic":     "optic_",
    "Oblique":   "oblq_",
    "Osprey":    "osprey_",
    "Osteria":   "osteria_",
    "Owlfish":   "owl_",
    "Ohm":       "ohm_",
    "Orphica":   "orph_",
    "Obbligato": "obbl_",
    "Ottoni":    "otto_",
    "Ole":       "ole_",
    "Ombre":     "ombre_",
    "Orca":      "orca_",
    "Octopus":   "octo_",
    "Overlap":   "olap_",
    "Outwit":    "owit_",
    "Ostinato":  "osti_",
    "OpenSky":   "sky_",
    "OceanDeep": "deep_",
    "Ouie":      "ouie_",
    "Obrix":     "obrix_",
    "Orbweave":  "weave_",
    "Overtone":  "over_",
    "Organism":  "org_",
    "Oxbow":     "oxb_",
    "Oware":     "owr_",
}

# ── Name Pool ─────────────────────────────────────────────────────────────────

SUBMERGED_NAMES = [
    ("Hadal",          "Whisper"),
    ("Benthic",        "Glow"),
    ("Pressure",       "Hymn"),
    ("Abyssal",        "Calm"),
    ("Trench",         "Lullaby"),
    ("Deep",           "Current"),
    ("Sediment",       "Dream"),
    ("Bathyal",        "Descent"),
    ("Midnight",       "Zone"),
    ("Pelagic",        "Silence"),
    ("Fathom",         "Weight"),
    ("Aphotic",        "Song"),
    ("Mariana",        "Echo"),
    ("Twilight",       "Sinking"),
    ("Anoxic",         "Rest"),
    ("Seafloor",       "Memory"),
    ("Hydrothermal",   "Murmur"),
    ("Tidal",          "Requiem"),
    ("Brine",          "Pool"),
    ("Continental",    "Shelf"),
    ("Oceanic",        "Crust"),
    ("Volcanic",       "Vent"),
    ("Coral",          "Tomb"),
    ("Kelp",           "Forest"),
    ("Salt",           "Cavern"),
    ("Submarine",      "Canyon"),
    ("Abyssopelagic",  "Drift"),
    ("Mesopelagic",    "Fade"),
    ("Neritic",        "Haze"),
    ("Littoral",       "Shadow"),
    ("Turbidity",      "Flow"),
    ("Upwelling",      "Sigh"),
    ("Thermocline",    "Border"),
    ("Halocline",      "Veil"),
    ("Pycnocline",     "Threshold"),
    ("Deep",           "Scatter"),
]

# ── Per-engine macro label sets  ──────────────────────────────────────────────
# These are the canonical macro labels for each engine, drawn from existing
# presets where available, else generic Submerged labels.

ENGINE_MACRO_LABELS = {
    "OddfeliX":  ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "OddOscar":  ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "Overdub":   ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "Odyssey":   ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "Oblong":    ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "Obese":     ["MOJO", "GRIT", "SIZE", "CRUSH"],
    "Onset":     ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "Overworld": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "Opal":      ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "Orbital":   ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "Organon":   ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "Ouroboros": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "Obsidian":  ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "Overbite":  ["BELLY", "BITE", "SCURRY", "TRASH"],
    "Origami":   ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "Oracle":    ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "Obscura":   ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "Oceanic":   ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "Ocelot":    ["PROWL", "FOLIAGE", "ECOSYSTEM", "CANOPY"],
    "Optic":     ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "Oblique":   ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "Osprey":    ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "Osteria":   ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "Owlfish":   ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "Ohm":       ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "Orphica":   ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "Obbligato": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "Ottoni":    ["GROW", "EMBOUCHURE", "LAKE", "FOREIGN"],
    "Ole":       ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "Ombre":     ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "Orca":      ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "Octopus":   ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "Overlap":   ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "Outwit":    ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "Ostinato":  ["GATHER", "FIRE", "CIRCLE", "SPACE"],
    "OpenSky":   ["RISE", "SHIMMER", "COUPLING", "SPACE"],
    "OceanDeep": ["PRESSURE", "BIOLUM", "COUPLING", "SPACE"],
    "Ouie":      ["HAMMER", "INTERVAL", "COUPLING", "SPACE"],
    "Obrix":     ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "Orbweave":  ["KNOT", "TOPOLOGY", "COUPLING", "SPACE"],
    "Overtone":  ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "Organism":  ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "Oxbow":     ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "Oware":     ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
}

# ── Per-engine extra tag ──────────────────────────────────────────────────────

ENGINE_EXTRA_TAGS = {
    "OddfeliX":  "neon",
    "OddOscar":  "morphing",
    "Overdub":   "dub",
    "Odyssey":   "drift",
    "Oblong":    "resonant",
    "Obese":     "fat",
    "Onset":     "percussive",
    "Overworld": "chip",
    "Opal":      "granular",
    "Orbital":   "spectral",
    "Organon":   "metabolic",
    "Ouroboros": "chaotic",
    "Obsidian":  "crystalline",
    "Overbite":  "gritty",
    "Origami":   "folded",
    "Oracle":    "stochastic",
    "Obscura":   "dark",
    "Oceanic":   "flocking",
    "Ocelot":    "layered",
    "Optic":     "visual",
    "Oblique":   "prismatic",
    "Osprey":    "coastal",
    "Osteria":   "warm",
    "Owlfish":   "resonant",
    "Ohm":       "drone",
    "Orphica":   "plucked",
    "Obbligato": "breath",
    "Ottoni":    "brass",
    "Ole":       "dramatic",
    "Ombre":     "shadowed",
    "Orca":      "predator",
    "Octopus":   "alien",
    "Overlap":   "knotted",
    "Outwit":    "sequenced",
    "Ostinato":  "rhythmic",
    "OpenSky":   "shimmer",
    "OceanDeep": "trench",
    "Ouie":      "duophonic",
    "Obrix":     "modular",
    "Orbweave":  "topology",
    "Overtone":  "harmonic",
    "Organism":  "cellular",
    "Oxbow":     "entangled",
    "Oware":     "percussive",
}

# ── Preset variations ─────────────────────────────────────────────────────────
# Three archetypes per engine: Darker, Spacious, Movement
# Each tuple: (brightness, space, aggression, warmth, movement, density, desc_theme, name_idx)

VARIATIONS = [
    # Archetype 0: Darker — low brightness, moderate space, very low aggression
    {
        "dna": {
            "brightness": 0.15,
            "warmth":     0.50,
            "movement":   0.22,
            "density":    0.55,
            "space":      0.65,
            "aggression": 0.08,
        },
        "character": 0.3,   # macroCharacter
        "movement":  0.2,   # macroMovement
        "coupling":  0.0,   # macroCoupling
        "space":     0.65,  # macroSpace
    },
    # Archetype 1: Spacious — very high space, a touch more brightness
    {
        "dna": {
            "brightness": 0.30,
            "warmth":     0.40,
            "movement":   0.30,
            "density":    0.35,
            "space":      0.78,
            "aggression": 0.12,
        },
        "character": 0.5,
        "movement":  0.3,
        "coupling":  0.0,
        "space":     0.78,
    },
    # Archetype 2: Movement — more movement and warmth, moderate space
    {
        "dna": {
            "brightness": 0.22,
            "warmth":     0.55,
            "movement":   0.45,
            "density":    0.60,
            "space":      0.60,
            "aggression": 0.20,
        },
        "character": 0.55,
        "movement":  0.45,
        "coupling":  0.0,
        "space":     0.60,
    },
]

VARIATION_DESCRIPTIONS = [
    # darker
    "{engine} at maximum depth — pressure dims every harmonic, only the lowest "
    "frequencies survive the descent into the hadal zone.",
    # spacious
    "{engine} adrift in the mesopelagic: wide reverb tails dissolve into "
    "surrounding silence, sound dispersed by ten thousand metres of water.",
    # movement
    "{engine} carried by a slow deep current — subtle modulation breathes through "
    "the mix like bioluminescent pulses in otherwise absolute darkness.",
]

# Name pool rotation per engine — deterministic, seeded by engine index
def _name_for(engine_index: int, variation_index: int) -> tuple[str, str]:
    """Return a (word1, word2) pair from the name pool."""
    pool_index = (engine_index * 3 + variation_index) % len(SUBMERGED_NAMES)
    return SUBMERGED_NAMES[pool_index]


# ── Core scanning logic ───────────────────────────────────────────────────────

def scan_existing_engines(submerged_dir: str) -> collections.Counter:
    """Return a Counter of engine → preset count from .xometa files."""
    counts: collections.Counter = collections.Counter()
    for filename in os.listdir(submerged_dir):
        if not filename.endswith(".xometa"):
            continue
        filepath = os.path.join(submerged_dir, filename)
        try:
            with open(filepath, "r", encoding="utf-8") as fh:
                data = json.load(fh)
            for engine in data.get("engines", []):
                counts[engine] += 1
        except (json.JSONDecodeError, OSError) as exc:
            print(f"  [warn] Could not parse {filename}: {exc}", file=sys.stderr)
    return counts


def find_gap_engines(counts: collections.Counter) -> list[str]:
    """Return engines from ALL_ENGINES that have 0 Submerged presets."""
    return [e for e in ALL_ENGINES if counts.get(e, 0) == 0]


# ── Preset construction ───────────────────────────────────────────────────────

def build_preset(engine: str, engine_index: int, variation_index: int) -> dict:
    """Build a single .xometa dict for the given engine + variation."""
    prefix = ENGINE_PREFIXES[engine]
    var = VARIATIONS[variation_index]
    labels = ENGINE_MACRO_LABELS[engine]
    extra_tag = ENGINE_EXTRA_TAGS.get(engine, "dark")

    word1, word2 = _name_for(engine_index, variation_index)
    name = f"{word1} {word2}"

    description = VARIATION_DESCRIPTIONS[variation_index].format(engine=engine)

    # Macro param keys — map label names to prefix_macro<Label> or use generic names
    # We write the 4 standard macro params regardless of label naming.
    # For CHARACTER / MOVEMENT / COUPLING / SPACE labels, the param names are:
    #   {prefix}macroCharacter, {prefix}macroMovement, {prefix}macroCoupling, {prefix}macroSpace
    # For engines with custom label names we still write generic macro keys as
    # those are what the engine registers.  We use the label list index to decide.
    label_to_generic = {
        0: "macroCharacter",
        1: "macroMovement",
        2: "macroCoupling",
        3: "macroSpace",
    }
    macro_values = [var["character"], var["movement"], var["coupling"], var["space"]]

    parameters = {
        f"{prefix}{label_to_generic[i]}": macro_values[i]
        for i in range(4)
    }

    preset = {
        "schema_version": 1,
        "name": name,
        "mood": "Submerged",
        "engines": [engine],
        "author": "XO_OX Designs",
        "version": "1.0.0",
        "description": description,
        "tags": ["submerged", "deep", "pressure", extra_tag],
        "macroLabels": labels,
        "couplingIntensity": "None",
        "tempo": None,
        "dna": var["dna"],
        "parameters": {
            engine: parameters,
        },
        "coupling": {"pairs": []},
    }
    return preset


def preset_filename(engine: str, word1: str, word2: str) -> str:
    """Return the canonical filename for a preset."""
    return f"{engine}_{word1}_{word2}.xometa"


# ── Report + apply ────────────────────────────────────────────────────────────

def run(apply: bool, repo_root: str) -> None:
    submerged_dir = os.path.join(repo_root, "Presets", "XOceanus", "Submerged")

    if not os.path.isdir(submerged_dir):
        print(f"ERROR: Submerged directory not found: {submerged_dir}", file=sys.stderr)
        sys.exit(1)

    # Step 1: Scan
    counts = scan_existing_engines(submerged_dir)
    gap_engines = find_gap_engines(counts)

    total_existing = sum(counts.values())
    total_engines_with = len([e for e in ALL_ENGINES if counts.get(e, 0) > 0])

    print("=" * 64)
    print("  XOceanus — Submerged Preset Gap Report")
    print("=" * 64)
    print(f"  Preset directory : {submerged_dir}")
    print(f"  Total presets    : {total_existing}")
    print(f"  Engines covered  : {total_engines_with} / {len(ALL_ENGINES)}")
    print(f"  Gap engines      : {len(gap_engines)}")
    print()

    if not gap_engines:
        print("  All engines have at least one Submerged preset. Nothing to do.")
        return

    print("  Engines with 0 Submerged presets:")
    for e in gap_engines:
        print(f"    - {e}")
    print()

    # Step 2: Build presets
    presets_to_write: list[tuple[str, dict]] = []  # (filepath, data)

    print("  Presets that would be created:")
    print("  " + "-" * 60)

    for engine_index, engine in enumerate(ALL_ENGINES):
        if engine not in gap_engines:
            continue

        for var_idx in range(3):
            word1, word2 = _name_for(ALL_ENGINES.index(engine), var_idx)
            preset = build_preset(engine, ALL_ENGINES.index(engine), var_idx)
            filename = preset_filename(engine, word1, word2)
            filepath = os.path.join(submerged_dir, filename)

            archetype = ["Darker", "Spacious", "Movement"][var_idx]
            print(f"  [{engine}] [{archetype}]  {filename}")
            print(f"    name : {preset['name']}")
            print(f"    dna  : brightness={preset['dna']['brightness']}, "
                  f"space={preset['dna']['space']}, "
                  f"movement={preset['dna']['movement']}, "
                  f"aggression={preset['dna']['aggression']}")
            print(f"    desc : {preset['description'][:80]}...")
            print()

            presets_to_write.append((filepath, preset))

    total_new = len(presets_to_write)
    print("  " + "-" * 60)
    print(f"  Total new presets : {total_new}")
    print(f"  Engines covered   : {len(gap_engines)}")
    print()

    if apply:
        print("  Writing files...")
        written = 0
        for filepath, preset in presets_to_write:
            if os.path.exists(filepath):
                print(f"  [skip] Already exists: {os.path.basename(filepath)}")
                continue
            with open(filepath, "w", encoding="utf-8") as fh:
                json.dump(preset, fh, indent=2, ensure_ascii=False)
                fh.write("\n")
            print(f"  [ok]   {os.path.basename(filepath)}")
            written += 1

        print()
        print(f"  Done. {written} file(s) written.")

        # Re-scan and report new totals
        counts_after = scan_existing_engines(submerged_dir)
        total_after = sum(counts_after.values())
        covered_after = len([e for e in ALL_ENGINES if counts_after.get(e, 0) > 0])
        print()
        print("  Post-apply summary:")
        print(f"    Total Submerged presets : {total_after}")
        print(f"    Engines covered         : {covered_after} / {len(ALL_ENGINES)}")
    else:
        print("  DRY-RUN — no files written.")
        print("  Run with --apply to write files.")

    print("=" * 64)


# ── Entry point ───────────────────────────────────────────────────────────────

def main() -> None:
    parser = argparse.ArgumentParser(
        description="Generate Submerged mood presets for engines with zero coverage."
    )
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument(
        "--dry-run",
        action="store_true",
        default=False,
        help="List what would be created without writing files (default behaviour).",
    )
    mode.add_argument(
        "--apply",
        action="store_true",
        default=False,
        help="Write preset files to Presets/XOceanus/Submerged/.",
    )
    args = parser.parse_args()

    # Repo root is one directory above Tools/
    script_dir = os.path.dirname(os.path.abspath(__file__))
    repo_root = os.path.dirname(script_dir)

    apply = args.apply and not args.dry_run
    run(apply=apply, repo_root=repo_root)


if __name__ == "__main__":
    main()
