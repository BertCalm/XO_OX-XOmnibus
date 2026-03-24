#!/usr/bin/env python3
"""
migrate_simplified_presets.py
Converts agent-written simplified preset format to the canonical XOlokun schema.

Simplified format (agent-written):
{
  "engine": "Ostinato",          ← single string
  "name": "...",
  "mood": "...",
  "macros": { "CHARACTER": 0.5, ... },
  "parameters": { "osti_param": value, ... },
  "sonicDNA": { ... },
  "tags": [...]
}

Canonical format (PresetManager.h schema_version 1):
{
  "schema_version": 1,
  "name": "...",
  "mood": "...",
  "engines": ["Ostinato"],        ← array of engine names
  "author": "XO_OX",
  "version": "1.0.0",
  "description": "",
  "tags": [...],
  "macroLabels": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
  "couplingIntensity": "None",
  "tempo": null,
  "dna": { "brightness": .., "warmth": .., "movement": .., "density": .., "space": .., "aggression": .. },
  "parameters": {
    "Ostinato": {                  ← engine-keyed sub-object
      "osti_param": value, ...
    }
  },
  "macros": { "CHARACTER": 0.5, ... },
  "coupling": { "pairs": [] }
}
"""

import json
import os
import glob
import sys
from pathlib import Path

PRESETS_ROOT = Path(__file__).parent.parent / "Presets" / "XOlokun"

# Map simplified sonicDNA keys to canonical dna keys
DNA_KEY_MAP = {
    "sonicDNA": None,  # container key
    "brightness": "brightness",
    "warmth": "warmth",
    "movement": "movement",
    "density": "density",
    "space": "space",
    "aggression": "aggression",
}

# Canonical macro label order for each engine (fallback to standard 4)
STANDARD_MACROS = ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"]


def is_simplified(data: dict) -> bool:
    """Return True if this preset uses the simplified agent-written format."""
    return "engine" in data and "schema_version" not in data


def migrate(data: dict, filepath: str) -> dict:
    """Convert simplified format to canonical format."""
    engine_name = data.get("engine", "Unknown")
    name = data.get("name", Path(filepath).stem.replace("_", " "))
    mood = data.get("mood", "Foundation")
    tags = data.get("tags", [])

    # Normalize tags to list of strings
    if isinstance(tags, list):
        tags = [str(t) for t in tags]

    # Extract macros
    macros_raw = data.get("macros", {})
    # Build macroLabels from the keys present, preserving standard order
    macro_labels = []
    for k in STANDARD_MACROS:
        if k in macros_raw:
            macro_labels.append(k)
    # Add any non-standard macro labels that were present
    for k in macros_raw:
        if k not in macro_labels:
            macro_labels.append(k)
    if not macro_labels:
        macro_labels = STANDARD_MACROS[:]

    # Extract DNA
    dna_raw = data.get("sonicDNA", data.get("dna", {}))
    dna = {
        "brightness": float(dna_raw.get("brightness", 0.5)),
        "warmth":     float(dna_raw.get("warmth", 0.5)),
        "movement":   float(dna_raw.get("movement", 0.5)),
        "density":    float(dna_raw.get("density", 0.5)),
        "space":      float(dna_raw.get("space", 0.5)),
        "aggression": float(dna_raw.get("aggression", 0.3)),
    }

    # Extract parameters — nest under engine name
    params_raw = data.get("parameters", {})
    parameters = {engine_name: params_raw} if params_raw else {}

    # Build macro values dict (keep for compatibility)
    macros = {}
    for k, v in macros_raw.items():
        try:
            macros[k] = float(v)
        except (TypeError, ValueError):
            macros[k] = 0.5  # non-numeric macro value — use default

    canonical = {
        "schema_version": 1,
        "name": name,
        "mood": mood,
        "engines": [engine_name],
        "author": "XO_OX",
        "version": "1.0.0",
        "description": data.get("description", ""),
        "tags": tags,
        "macroLabels": macro_labels,
        "couplingIntensity": "None",
        "tempo": None,
        "dna": dna,
        "parameters": parameters,
        "macros": macros,
        "coupling": {"pairs": []},
    }

    return canonical


def process_all(dry_run: bool = False) -> tuple[int, int, int]:
    """Walk preset dirs and migrate all simplified presets.
    Returns (found, migrated, skipped)."""
    found = migrated = skipped = 0

    pattern = str(PRESETS_ROOT / "**" / "*.xometa")
    files = glob.glob(pattern, recursive=True)

    for filepath in sorted(files):
        try:
            with open(filepath, "r", encoding="utf-8") as f:
                data = json.load(f)
        except (json.JSONDecodeError, OSError) as e:
            print(f"  SKIP (unreadable): {filepath} — {e}")
            skipped += 1
            continue

        if not is_simplified(data):
            skipped += 1
            continue

        found += 1
        canonical = migrate(data, filepath)

        if dry_run:
            print(f"  WOULD MIGRATE: {Path(filepath).name}  [{data.get('engine')}]")
        else:
            try:
                with open(filepath, "w", encoding="utf-8") as f:
                    json.dump(canonical, f, indent=2, ensure_ascii=False)
                    f.write("\n")
                migrated += 1
            except OSError as e:
                print(f"  ERROR writing {filepath}: {e}")
                skipped += 1

    return found, migrated, skipped


def main():
    dry_run = "--dry-run" in sys.argv
    mode = "DRY RUN" if dry_run else "MIGRATING"
    print(f"=== migrate_simplified_presets.py — {mode} ===")
    print(f"Scanning: {PRESETS_ROOT}")
    print()

    found, migrated, skipped = process_all(dry_run)

    print()
    print(f"Found simplified presets : {found}")
    if not dry_run:
        print(f"Successfully migrated    : {migrated}")
    print(f"Skipped (canonical/error): {skipped}")

    if dry_run and found > 0:
        print()
        print("Run without --dry-run to apply migrations.")


if __name__ == "__main__":
    main()
