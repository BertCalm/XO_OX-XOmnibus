#!/usr/bin/env python3
"""Rename presets with DNA-encoded names (e.g. '5X DARK COLD...') to evocative names.

Generates names from engine pair + mood + DNA character.
"""

import json
import pathlib
import re
import sys

REPO_ROOT = pathlib.Path(__file__).parent.parent
PRESET_DIR = REPO_ROOT / "Presets" / "XOceanus"

# Short evocative identifiers per engine
ENGINE_SHORT = {
    "OddfeliX": "Spark", "OddOscar": "Morph", "Overdub": "Dub",
    "Odyssey": "Drift", "Oblong": "Bob", "Obese": "Fat",
    "Onset": "Drum", "Overworld": "Era", "Opal": "Grain",
    "Orbital": "Ring", "Organon": "Cell", "Ouroboros": "Loop",
    "Obsidian": "Glass", "Overbite": "Fang", "Origami": "Fold",
    "Oracle": "Omen", "Obscura": "Fog", "Oceanic": "Swarm",
    "Ocelot": "Track", "Optic": "Pulse", "Oblique": "Prism",
    "Osprey": "Shore", "Osteria": "Wine", "Owlfish": "Abyss",
    "Ohm": "Jam", "Orphica": "Harp", "Obbligato": "Wind",
    "Ottoni": "Brass", "Ole": "Ritual", "Ombre": "Shade",
    "Orca": "Hunt", "Octopus": "Arm", "Overlap": "Knot",
    "Outwit": "Cell", "OpenSky": "Sky", "Ostinato": "Beat",
    "OceanDeep": "Trench", "Ouie": "Strike",
    "Overtone": "Spiral", "Organism": "Colony",
}

# DNA summary → adjective
def dna_adjective(dna: dict) -> str:
    b, w = dna.get("brightness", 0.5), dna.get("warmth", 0.5)
    m, a = dna.get("movement", 0.5), dna.get("aggression", 0.5)
    if a > 0.7: return "Violent"
    if m > 0.7: return "Kinetic"
    if b > 0.7: return "Radiant"
    if w > 0.7: return "Warm"
    if b < 0.3 and w < 0.3: return "Cold"
    if m < 0.2: return "Still"
    return "Deep"

# Mood-flavored suffix
MOOD_SUFFIX = {
    "Foundation": "Base", "Atmosphere": "Drift", "Entangled": "Mesh",
    "Prism": "Split", "Flux": "Flow", "Aether": "Void",
    "Family": "Blend", "Submerged": "Sink",
}


def make_name(engines, mood, dna, existing_names: set) -> str:
    e1 = ENGINE_SHORT.get(engines[0], engines[0][:4]) if engines else "??"
    e2 = ENGINE_SHORT.get(engines[1], engines[1][:4]) if len(engines) > 1 else ""
    adj = dna_adjective(dna)
    suffix = MOOD_SUFFIX.get(mood, "Wave")

    candidates = [
        f"{adj} {e1} {suffix}",
        f"{e1} {e2} {adj}",
        f"{adj} {e1}",
        f"{e1} {e2} Wave",
    ]
    for c in candidates:
        if len(c) <= 30 and c not in existing_names:
            return c
    # Fallback with counter
    base = f"{e1} {e2}"[:25]
    i = 2
    while f"{base} {i}" in existing_names:
        i += 1
    return f"{base} {i}"


def main():
    bad = sorted(PRESET_DIR.rglob("5X *.xometa"))
    if not bad:
        print("No bad-named presets found.")
        return

    # Collect all existing preset names to avoid duplicates
    existing = set()
    for f in PRESET_DIR.rglob("*.xometa"):
        if f not in bad:
            try:
                d = json.loads(f.read_text())
                if n := d.get("name"):
                    existing.add(n)
            except Exception as e:
                print(f'[WARN] Rename failed: {e}', file=sys.stderr)

    renamed = 0
    for f in bad:
        try:
            data = json.loads(f.read_text())
        except Exception:
            continue

        engines = data.get("engines", [])
        mood = data.get("mood", "Foundation")
        dna = data.get("dna", {})

        new_name = make_name(engines, mood, dna, existing)
        existing.add(new_name)

        # Update name in data
        data["name"] = new_name

        # New filename: {engines[0]}_{engines[1]}_{name_underscored}.xometa
        e_tag = "_".join(e[:4] for e in engines[:2]) if engines else "Coup"
        slug = new_name.replace(" ", "_").replace("/", "_")
        new_filename = f"{e_tag}_{slug}.xometa"
        new_path = f.parent / new_filename

        # Write updated content to new path, delete old
        new_path.write_text(json.dumps(data, indent=2, ensure_ascii=False) + "\n",
                            encoding="utf-8")
        f.unlink()
        renamed += 1

    print(f"Renamed {renamed} presets.")


if __name__ == "__main__":
    main()
