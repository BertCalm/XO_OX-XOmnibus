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

from engine_registry import get_all_engines

# Canonical engine name mapping — ALL-CAPS and common misspellings → canonical form.
# The canonical names themselves come from engine_registry.get_all_engines() so this
# file never needs updating when a new engine is added.  Only add entries here for
# new misspelling / casing variants that appear in the wild.
CANONICAL: dict[str, str] = {}

# Auto-populate ALL-CAPS → canonical for every registered engine
for _name in get_all_engines():
    CANONICAL[_name.upper()] = _name
    CANONICAL[_name.lower()] = _name

# Hand-authored variants that cannot be derived from upper/lower alone
CANONICAL.update({
    # OddfeliX / OddOscar casing quirks
    "OddFelix":   "OddfeliX",
    "oddfelix":   "OddfeliX",
    "Oddoscar":   "OddOscar",
    "oddoscar":   "OddOscar",
    "XOddFelix":  "OddfeliX",
    "XOddOscar":  "OddOscar",
    # Multi-word names with collapsed casing
    "Opensky":    "OpenSky",
    "Oceandeep":  "OceanDeep",
    # X-prefixed variants not in PresetManager alias table
    "XOhm":       "Ohm",
    "XOrphica":   "Orphica",
    "XObbligato": "Obbligato",
    "XOttoni":    "Ottoni",
    "XOle":       "Ole",
    "XOmbre":     "Ombre",
    "XOsprey":    "Osprey",
    "XOsteria":   "Osteria",
    "XOwlfish":   "Owlfish",
    "XOcelot":    "Ocelot",
    "XOutwit":    "Outwit",
    "XOverlap":   "Overlap",
    # Newer engine variants spotted in agent-written presets
    "XOxytocin":  "Oxytocin",
    "XOutlook":   "Outlook",
    "XObiont":    "Obiont",
    "XOkeanos":   "Okeanos",
    "XOutflow":   "Outflow",
})


def normalize_preset(path: Path) -> bool:
    """Returns True if file was modified."""
    try:
        text = path.read_text(encoding="utf-8")
        data = json.loads(text)
    except Exception:
        return False

    changed = False

    # Migrate singular "engine" key to plural "engines" array
    if "engine" in data and "engines" not in data:
        singular = data.pop("engine")
        data["engines"] = [singular]
        changed = True

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
    preset_dir = repo_root / "Presets" / "XOceanus"

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
