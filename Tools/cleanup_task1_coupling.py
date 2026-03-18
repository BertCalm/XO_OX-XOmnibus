#!/usr/bin/env python3
"""Task 1: Migrate flat-dict coupling presets to pairs[] format.

Handles three cases:
1. Flat dict with source/target/type/amount (no pairs key) -> convert to pairs[]
2. Routes format: coupling.routes[] with source/dest -> convert to pairs[]
3. Hybrid: flat dict WITH pairs already present -> strip flat keys, keep pairs
"""

import json
import glob
import os

PRESETS_ROOT = "/home/user/XO_OX-XOmnibus/Presets"
converted_flat = 0
converted_routes = 0
converted_hybrid = 0
errors = 0

for filepath in glob.glob(os.path.join(PRESETS_ROOT, "**", "*.xometa"), recursive=True):
    try:
        with open(filepath, 'r') as f:
            data = json.load(f)

        coupling = data.get('coupling')
        if not isinstance(coupling, dict):
            continue

        modified = False
        has_pairs = 'pairs' in coupling and isinstance(coupling.get('pairs'), list)
        has_source_target = 'source' in coupling and 'target' in coupling
        has_routes = 'routes' in coupling and isinstance(coupling.get('routes'), list)

        if has_source_target and not has_pairs:
            # Case 1: flat dict -> pairs[]
            pair = {
                "engineA": coupling["source"],
                "engineB": coupling["target"],
                "type": coupling.get("type", "HARMONIC_BLEND"),
                "amount": coupling.get("amount", 0.5)
            }
            new_coupling = {"pairs": [pair]}
            if 'couplingIntensity' in data:
                pass  # kept at top level
            data['coupling'] = new_coupling
            converted_flat += 1
            modified = True

        elif has_routes and not has_pairs:
            # Case 2: routes[] -> pairs[]
            pairs = []
            for route in coupling['routes']:
                pair = {
                    "engineA": route.get("source", ""),
                    "engineB": route.get("dest", route.get("target", "")),
                    "type": route.get("type", "HARMONIC_BLEND"),
                    "amount": route.get("amount", 0.5)
                }
                pairs.append(pair)
            new_coupling = {"pairs": pairs}
            data['coupling'] = new_coupling
            converted_routes += 1
            modified = True

        elif has_source_target and has_pairs:
            # Case 3: hybrid - strip flat keys, keep pairs
            keys_to_remove = [k for k in coupling if k not in ('pairs',)]
            for k in keys_to_remove:
                del coupling[k]
            converted_hybrid += 1
            modified = True

        if modified:
            with open(filepath, 'w') as f:
                json.dump(data, f, indent=2, ensure_ascii=False)
                f.write('\n')

    except Exception as e:
        errors += 1

total = converted_flat + converted_routes + converted_hybrid
print(f"=== Task 1: Coupling Migration ===")
print(f"Flat dict (source/target) -> pairs[]: {converted_flat}")
print(f"Routes[] -> pairs[]: {converted_routes}")
print(f"Hybrid (stripped flat keys): {converted_hybrid}")
print(f"TOTAL converted: {total}")
print(f"Errors: {errors}")
