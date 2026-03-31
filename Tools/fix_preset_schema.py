#!/usr/bin/env python3
"""Fix preset schema: rename 'params' to 'parameters' and nest flat params under engine name."""
import json, sys
from pathlib import Path

PRESET_ROOT = Path(__file__).parent.parent / "Presets" / "XOlokun"

# Engine prefix → engine name mapping (for nesting flat params)
PREFIX_TO_ENGINE = {
    "ofr_": "Offering", "wash_": "Overwash", "worn_": "Overworn",
    "flow_": "Overflow", "cast_": "Overcast", "osmo_": "Osmosis",
}

def fix_preset(path):
    with open(path, 'r') as f:
        data = json.load(f)

    changed = False

    # Fix 1: "params" → "parameters"
    if "params" in data and "parameters" not in data:
        data["parameters"] = data.pop("params")
        changed = True

    params = data.get("parameters", {})

    # Fix 2: Flat params → nested under engine name
    # Check if any top-level key in parameters looks like a param ID (has underscore prefix)
    flat_keys = [k for k in params if "_" in k and not any(k == eng for eng in ["Offering", "Overwash", "Overworn", "Overflow", "Overcast", "Osmosis"])]

    if flat_keys:
        # Determine engine from prefix
        nested = {}
        for key, val in params.items():
            engine = None
            for prefix, eng_name in PREFIX_TO_ENGINE.items():
                if key.startswith(prefix):
                    engine = eng_name
                    break
            if engine:
                nested.setdefault(engine, {})[key] = val
            else:
                nested[key] = val  # keep unknown keys at top level

        if nested != params:
            data["parameters"] = nested
            changed = True

    if changed:
        with open(path, 'w') as f:
            json.dump(data, f, indent=2)
        return True
    return False

fixed = 0
for path in PRESET_ROOT.rglob("*.xometa"):
    if "_quarantine" in str(path):
        continue
    if fix_preset(path):
        fixed += 1
        print(f"Fixed: {path.relative_to(PRESET_ROOT)}")

print(f"\nTotal fixed: {fixed}")
