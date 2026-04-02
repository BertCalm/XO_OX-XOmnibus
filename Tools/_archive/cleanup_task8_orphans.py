#!/usr/bin/env python3
"""Task 8: Triage 1,189 orphan presets.

Orphan directories: Presets/Foundation/, Presets/Entangled/, etc. (not under Presets/XOceanus/)
- If duplicate of canonical preset, delete orphan
- If unique, move to Presets/XOceanus/{mood}/
- Fix "OCELOT" -> "Ocelot" in engine references
"""

import json
import glob
import os
import shutil

PRESETS_ROOT = "/home/user/XO_OX-XOceanus/Presets"
CANONICAL_ROOT = os.path.join(PRESETS_ROOT, "XOceanus")

# Orphan directories (top-level mood folders that aren't XOceanus)
ORPHAN_DIRS = ["Foundation", "Atmosphere", "Entangled", "Prism", "Flux", "Aether", "Family", "Drift"]


def fix_ocelot(data):
    """Fix OCELOT -> Ocelot in engine references."""
    fixed = False
    engines = data.get('engines', [])
    if isinstance(engines, list):
        for i, e in enumerate(engines):
            if e == 'OCELOT':
                engines[i] = 'Ocelot'
                fixed = True
        data['engines'] = engines

    # Also fix in coupling pairs
    coupling = data.get('coupling', {})
    if isinstance(coupling, dict):
        pairs = coupling.get('pairs', [])
        if isinstance(pairs, list):
            for pair in pairs:
                if isinstance(pair, dict):
                    if pair.get('engineA') == 'OCELOT':
                        pair['engineA'] = 'Ocelot'
                        fixed = True
                    if pair.get('engineB') == 'OCELOT':
                        pair['engineB'] = 'Ocelot'
                        fixed = True

    # Fix in parameters keys
    params = data.get('parameters', {})
    if isinstance(params, dict) and 'OCELOT' in params:
        params['Ocelot'] = params.pop('OCELOT')
        fixed = True

    return fixed


def get_canonical_names():
    """Build set of preset names in canonical location."""
    names = {}
    for filepath in glob.glob(os.path.join(CANONICAL_ROOT, "**", "*.xometa"), recursive=True):
        try:
            with open(filepath, 'r') as f:
                data = json.load(f)
            name = data.get('name', '')
            if name:
                names[name.lower()] = filepath
        except:
            pass
    return names


def main():
    canonical_names = get_canonical_names()

    orphan_count = {}
    deleted_dupes = 0
    moved_unique = 0
    ocelot_fixed = 0
    errors = 0

    for orphan_dir in ORPHAN_DIRS:
        orphan_path = os.path.join(PRESETS_ROOT, orphan_dir)
        if not os.path.isdir(orphan_path):
            continue

        files = glob.glob(os.path.join(orphan_path, "**", "*.xometa"), recursive=True)
        orphan_count[orphan_dir] = len(files)

        for filepath in files:
            try:
                with open(filepath, 'r') as f:
                    data = json.load(f)

                name = data.get('name', '')

                # Fix OCELOT everywhere
                if fix_ocelot(data):
                    ocelot_fixed += 1

                # Check if duplicate
                if name and name.lower() in canonical_names:
                    os.remove(filepath)
                    deleted_dupes += 1
                else:
                    # Move to canonical location
                    mood = data.get('mood') or data.get('category') or orphan_dir
                    # Normalize mood
                    if mood == 'Drift':
                        mood = 'Flux'  # Drift maps to Flux
                    if mood not in ['Foundation', 'Atmosphere', 'Entangled', 'Prism', 'Flux', 'Aether', 'Family']:
                        mood = 'Foundation'

                    # Ensure mood field is set
                    data['mood'] = mood

                    # Ensure tags is not None
                    if data.get('tags') is None:
                        data['tags'] = []

                    dest_dir = os.path.join(CANONICAL_ROOT, mood)
                    os.makedirs(dest_dir, exist_ok=True)

                    dest_file = os.path.join(dest_dir, os.path.basename(filepath))
                    # Handle name collision in destination
                    if os.path.exists(dest_file):
                        base, ext = os.path.splitext(os.path.basename(filepath))
                        counter = 2
                        while os.path.exists(dest_file):
                            dest_file = os.path.join(dest_dir, f"{base}_{counter}{ext}")
                            counter += 1

                    with open(dest_file, 'w') as f:
                        json.dump(data, f, indent=2, ensure_ascii=False)
                        f.write('\n')

                    os.remove(filepath)
                    moved_unique += 1

            except Exception as e:
                errors += 1

    # Also fix OCELOT in canonical presets
    ocelot_canonical = 0
    for filepath in glob.glob(os.path.join(CANONICAL_ROOT, "**", "*.xometa"), recursive=True):
        try:
            with open(filepath, 'r') as f:
                data = json.load(f)
            if fix_ocelot(data):
                ocelot_canonical += 1
                with open(filepath, 'w') as f:
                    json.dump(data, f, indent=2, ensure_ascii=False)
                    f.write('\n')
        except:
            pass

    # Clean up empty orphan dirs
    for orphan_dir in ORPHAN_DIRS:
        orphan_path = os.path.join(PRESETS_ROOT, orphan_dir)
        if os.path.isdir(orphan_path):
            for root, dirs, files in os.walk(orphan_path, topdown=False):
                for d in dirs:
                    dp = os.path.join(root, d)
                    try:
                        if os.path.isdir(dp) and not os.listdir(dp):
                            os.rmdir(dp)
                    except OSError:
                        pass
                try:
                    if os.path.isdir(root) and not os.listdir(root):
                        os.rmdir(root)
                except OSError:
                    pass

    print(f"=== Task 8: Triage Orphan Presets ===")
    print(f"Orphan counts by directory:")
    for d, c in sorted(orphan_count.items()):
        print(f"  {d}: {c}")
    total_orphans = sum(orphan_count.values())
    print(f"  TOTAL orphans: {total_orphans}")
    print(f"Deleted duplicates: {deleted_dupes}")
    print(f"Moved unique to XOceanus/: {moved_unique}")
    print(f"OCELOT->Ocelot fixed (orphans): {ocelot_fixed}")
    print(f"OCELOT->Ocelot fixed (canonical): {ocelot_canonical}")
    print(f"Errors: {errors}")


if __name__ == '__main__':
    main()
