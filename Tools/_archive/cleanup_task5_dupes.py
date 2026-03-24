#!/usr/bin/env python3
"""Task 5: Fix 16 case-insensitive name dupes.

For ALL-CAPS versions, convert to Title Case to match the other.
"""

import json
import glob
import os
from collections import defaultdict

PRESETS_ROOT = "/home/user/XO_OX-XOlokun/Presets"


def main():
    # Collect all names
    names_map = defaultdict(list)  # lower_name -> [(name, filepath)]

    for filepath in glob.glob(os.path.join(PRESETS_ROOT, "**", "*.xometa"), recursive=True):
        try:
            with open(filepath, 'r') as f:
                data = json.load(f)
            name = data.get('name', '')
            if name:
                names_map[name.lower()].append((name, filepath))
        except:
            pass

    # Find groups with different casings
    fixed = 0
    dupe_groups = 0

    for lower_name, entries in names_map.items():
        distinct_names = set(n for n, _ in entries)
        if len(distinct_names) <= 1:
            continue

        dupe_groups += 1

        # Find the "good" name (Title Case or mixed case, not ALL CAPS)
        good_name = None
        for name in distinct_names:
            if name != name.upper():
                good_name = name
                break

        if not good_name:
            # All are ALL-CAPS, just title-case the first
            good_name = list(distinct_names)[0].title()

        # Fix ALL-CAPS entries to match
        for name, filepath in entries:
            if name == name.upper() and name != good_name:
                try:
                    with open(filepath, 'r') as f:
                        data = json.load(f)
                    data['name'] = good_name
                    with open(filepath, 'w') as f:
                        json.dump(data, f, indent=2, ensure_ascii=False)
                        f.write('\n')
                    fixed += 1
                except:
                    pass

    print(f"=== Task 5: Fix Case-Insensitive Name Dupes ===")
    print(f"Duplicate groups found: {dupe_groups}")
    print(f"Files fixed (ALL-CAPS -> Title Case): {fixed}")


if __name__ == '__main__':
    main()
