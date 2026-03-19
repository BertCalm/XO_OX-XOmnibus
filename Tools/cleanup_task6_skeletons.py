#!/usr/bin/env python3
"""Task 6: Fix 33 skeleton presets (null mood/tags/engines).

Set mood from folder location. Set tags to []. Infer engines from filename if possible.
"""

import json
import glob
import os
import re

PRESETS_ROOT = "/home/user/XO_OX-XOmnibus/Presets"

# Known engine names for inference from filenames
KNOWN_ENGINES = [
    "OddfeliX", "OddOscar", "Overdub", "Odyssey", "Oblong", "Obese", "Onset",
    "Overworld", "Opal", "Orbital", "Organon", "Ouroboros", "Obsidian", "Overbite",
    "Origami", "Oracle", "Obscura", "Oceanic", "Ocelot", "Optic", "Oblique",
    "Osprey", "Osteria", "Owlfish", "Ohm", "Orphica", "Obbligato", "Ottoni",
    "Ole", "Overlap", "Outwit", "Ombre", "Orca", "Octopus"
]

MOOD_FOLDERS = ['Foundation', 'Atmosphere', 'Entangled', 'Prism', 'Flux', 'Aether', 'Family']


def get_mood_from_path(filepath):
    """Infer mood from file path."""
    parts = filepath.replace('\\', '/').split('/')
    for part in parts:
        for mood in MOOD_FOLDERS:
            if part.lower() == mood.lower():
                return mood
    return "Foundation"  # default


def infer_engines_from_path(filepath):
    """Try to infer engines from directory name (e.g. Osprey-Osteria)."""
    parts = filepath.replace('\\', '/').split('/')
    engines = []
    for part in parts:
        if '-' in part:
            for segment in part.split('-'):
                for eng in KNOWN_ENGINES:
                    if segment.lower() == eng.lower():
                        engines.append(eng)
    # Also check filename
    basename = os.path.splitext(os.path.basename(filepath))[0]
    for eng in KNOWN_ENGINES:
        if eng.lower() in basename.lower() and eng not in engines:
            engines.append(eng)
    return engines if engines else None


def main():
    fixed = 0
    errors = 0

    for filepath in glob.glob(os.path.join(PRESETS_ROOT, "**", "*.xometa"), recursive=True):
        try:
            with open(filepath, 'r') as f:
                data = json.load(f)

            if data.get('mood') is not None or data.get('tags') is not None:
                continue

            # This is a skeleton preset
            mood = get_mood_from_path(filepath)
            data['mood'] = mood
            data['tags'] = []

            # Fix engines if null
            if not data.get('engines'):
                engines = infer_engines_from_path(filepath)
                if engines:
                    data['engines'] = engines

            with open(filepath, 'w') as f:
                json.dump(data, f, indent=2, ensure_ascii=False)
                f.write('\n')

            fixed += 1

        except Exception as e:
            errors += 1

    print(f"=== Task 6: Fix Skeleton Presets ===")
    print(f"TOTAL fixed: {fixed}")
    print(f"Errors: {errors}")


if __name__ == '__main__':
    main()
