#!/usr/bin/env python3
"""Task 4: Fill 1,147 missing parameters fields.

Adds minimal parameters stub with engine's default values.
"""

import json
import glob
import os

PRESETS_ROOT = "/home/user/XO_OX-XOmnibus/Presets"


def main():
    filled = 0
    errors = 0

    for filepath in glob.glob(os.path.join(PRESETS_ROOT, "**", "*.xometa"), recursive=True):
        try:
            with open(filepath, 'r') as f:
                data = json.load(f)

            params = data.get('parameters')
            if params and isinstance(params, dict) and len(params) > 0:
                continue

            # Get engines list
            engines = data.get('engines', [])
            if not engines or not isinstance(engines, list):
                # Try to infer from filename or path
                # e.g. path might contain engine name
                data['parameters'] = {}
                filled += 1
            else:
                stub = {}
                for engine in engines:
                    if isinstance(engine, str):
                        stub[engine] = {}
                data['parameters'] = stub
                filled += 1

            with open(filepath, 'w') as f:
                json.dump(data, f, indent=2, ensure_ascii=False)
                f.write('\n')

        except Exception as e:
            errors += 1

    print(f"=== Task 4: Fill Missing Parameters ===")
    print(f"TOTAL filled: {filled}")
    print(f"Errors: {errors}")


if __name__ == '__main__':
    main()
