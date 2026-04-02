#!/usr/bin/env python3
"""Task 7: Fix 327 single-engine Entangled presets.

Update their mood field to a more appropriate mood based on DNA.
Don't move files.
"""

import json
import glob
import os

PRESETS_ROOT = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "Presets")


def pick_mood_from_dna(dna):
    """Pick appropriate mood from DNA values."""
    if not dna or not isinstance(dna, dict):
        return "Foundation"

    space = dna.get('space', 0.5)
    aggression = dna.get('aggression', 0.5)
    movement = dna.get('movement', 0.5)

    if space > 0.6 and aggression < 0.4:
        return "Atmosphere"
    elif movement > 0.6:
        return "Flux"
    elif aggression > 0.6:
        return "Prism"
    else:
        return "Foundation"


def main():
    fixed = 0
    by_new_mood = {}
    errors = 0

    for filepath in glob.glob(os.path.join(PRESETS_ROOT, "**", "*.xometa"), recursive=True):
        # Only process files in Entangled folders
        if '/Entangled/' not in filepath.replace('\\', '/'):
            continue

        try:
            with open(filepath, 'r') as f:
                data = json.load(f)

            engines = data.get('engines', [])
            if not isinstance(engines, list) or len(engines) != 1:
                continue

            # Single-engine Entangled preset
            dna = data.get('dna', {})
            new_mood = pick_mood_from_dna(dna)
            data['mood'] = new_mood

            by_new_mood[new_mood] = by_new_mood.get(new_mood, 0) + 1

            with open(filepath, 'w') as f:
                json.dump(data, f, indent=2, ensure_ascii=False)
                f.write('\n')

            fixed += 1

        except Exception as e:
            errors += 1

    print(f"=== Task 7: Fix Single-Engine Entangled Presets ===")
    print(f"TOTAL re-mooded: {fixed}")
    for mood, count in sorted(by_new_mood.items()):
        print(f"  -> {mood}: {count}")
    print(f"Errors: {errors}")


if __name__ == '__main__':
    main()
