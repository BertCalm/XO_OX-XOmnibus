#!/usr/bin/env python3
"""Normalize engine names in all .xometa preset files to canonical mixed-case form.

Fixes:
  - All-caps agent-written names (ORIGAMI → Origami, OHM → Ohm, etc.)
  - Wrong-case variants (OddFelix → OddfeliX, Oddoscar → OddOscar)
  - Concept/theorem engines with wrong case (OPENSKY → OpenSky, etc.)
  - Updates both "engines" array AND "parameters" object keys
"""

import json
import os
import sys
from pathlib import Path

# Canonical engine name mapping — source of truth
CANONICAL = {
    # Original 26 engines
    "ODDFELIX": "OddfeliX",
    "OddFelix": "OddfeliX",
    "oddfelix": "OddfeliX",
    "ODDOSCAR": "OddOscar",
    "Oddoscar": "OddOscar",
    "oddoscar": "OddOscar",
    "OVERDUB": "Overdub",
    "ODYSSEY": "Odyssey",
    "OBLONG": "Oblong",
    "OBESE": "Obese",
    "ONSET": "Onset",
    "OVERWORLD": "Overworld",
    "OPAL": "Opal",
    "ORBITAL": "Orbital",
    "ORGANON": "Organon",
    "OUROBOROS": "Ouroboros",
    "OBSIDIAN": "Obsidian",
    "OVERBITE": "Overbite",
    "ORIGAMI": "Origami",
    "ORACLE": "Oracle",
    "OBSCURA": "Obscura",
    "OCEANIC": "Oceanic",
    "OCELOT": "Ocelot",
    "OPTIC": "Optic",
    "OBLIQUE": "Oblique",
    "OSPREY": "Osprey",
    "OSTERIA": "Osteria",
    "OWLFISH": "Owlfish",
    # Constellation engines
    "OHM": "Ohm",
    "ORPHICA": "Orphica",
    "OBBLIGATO": "Obbligato",
    "OTTONI": "Ottoni",
    "OLE": "Ole",
    # Phase 3/4 engines
    "OMBRE": "Ombre",
    "ORCA": "Orca",
    "OCTOPUS": "Octopus",
    "OVERLAP": "Overlap",
    "OUTWIT": "Outwit",
    # V1 concept engines
    "OPENSKY": "OpenSky",
    "Opensky": "OpenSky",
    "OSTINATO": "Ostinato",
    "OCEANDEEP": "OceanDeep",
    "Oceandeep": "OceanDeep",
    "OUIE": "Ouie",
    # V2 theorem engines
    "OVERTONE": "Overtone",
    "ORGANISM": "Organism",
    # X-prefixed variants not in PresetManager alias table
    # (Constellation + Phase 3/4 engines added after the original alias set)
    "XOhm": "Ohm",
    "XOrphica": "Orphica",
    "XObbligato": "Obbligato",
    "XOttoni": "Ottoni",
    "XOle": "Ole",
    "XOmbre": "Ombre",
    "XOsprey": "Osprey",
    "XOsteria": "Osteria",
    "XOwlfish": "Owlfish",
    "XOcelot": "Ocelot",
    "XOddFelix": "OddfeliX",
    "XOddOscar": "OddOscar",
    "XOutwit": "Outwit",
    "XOverlap": "Overlap",
}


def normalize_preset(path: Path) -> bool:
    """Returns True if file was modified."""
    try:
        text = path.read_text(encoding="utf-8")
        data = json.loads(text)
    except Exception:
        return False

    changed = False

    # Fix engines array
    if "engines" in data and isinstance(data["engines"], list):
        new_engines = []
        for eng in data["engines"]:
            canonical = CANONICAL.get(eng, eng)
            if canonical != eng:
                changed = True
            new_engines.append(canonical)
        data["engines"] = new_engines

    # Fix parameters keys
    if "parameters" in data and isinstance(data["parameters"], dict):
        old_params = data["parameters"]
        new_params = {}
        for key, val in old_params.items():
            canonical = CANONICAL.get(key, key)
            if canonical != key:
                changed = True
            new_params[canonical] = val
        if changed:
            data["parameters"] = new_params

    if changed:
        path.write_text(json.dumps(data, indent=2, ensure_ascii=False) + "\n",
                        encoding="utf-8")

    return changed


def main():
    repo_root = Path(__file__).parent.parent
    preset_dir = repo_root / "Presets" / "XOmnibus"

    if not preset_dir.exists():
        print(f"ERROR: preset dir not found: {preset_dir}")
        sys.exit(1)

    total = 0
    fixed = 0

    for xometa in sorted(preset_dir.rglob("*.xometa")):
        total += 1
        if normalize_preset(xometa):
            fixed += 1

    print(f"Scanned : {total:,} preset files")
    print(f"Fixed   : {fixed:,} files normalized")
    print(f"Unchanged: {total - fixed:,}")


if __name__ == "__main__":
    main()
