#!/usr/bin/env python3
"""Fix Drift engine ADSR values authored in milliseconds — convert to seconds."""
import json
from pathlib import Path

PRESET_ROOT = Path(__file__).parent.parent / "Presets" / "XOceanus"
DRIFT_PARAMS = ["drift_attack", "drift_decay", "drift_release"]
RANGES = {"drift_attack": 2.0, "drift_decay": 5.0, "drift_release": 5.0}

fixed = 0
for path in PRESET_ROOT.rglob("*.xometa"):
    if "_quarantine" in str(path):
        continue
    with open(path) as f:
        data = json.load(f)

    params = data.get("parameters", {})
    changed = False

    for eng_name, eng_params in params.items():
        if not isinstance(eng_params, dict):
            continue
        for param in DRIFT_PARAMS:
            if param in eng_params:
                val = eng_params[param]
                max_val = RANGES[param]
                if isinstance(val, (int, float)) and val > max_val * 2:
                    # Value is likely in ms — convert to seconds
                    eng_params[param] = round(val / 1000.0, 4)
                    changed = True

    if changed:
        with open(path, 'w') as f:
            json.dump(data, f, indent=2)
        fixed += 1

print(f"Fixed {fixed} presets")
