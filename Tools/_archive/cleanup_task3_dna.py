#!/usr/bin/env python3
"""Task 3: Fill 437 missing DNA fields.

Estimates DNA from parameters if available, otherwise uses mood-based defaults.
"""

import json
import glob
import os

PRESETS_ROOT = "/home/user/XO_OX-XOmnibus/Presets"

MOOD_DEFAULTS = {
    "Foundation":  {"brightness": 0.5, "warmth": 0.5, "movement": 0.3, "density": 0.5, "space": 0.4, "aggression": 0.3},
    "Atmosphere":  {"brightness": 0.4, "warmth": 0.6, "movement": 0.4, "density": 0.3, "space": 0.7, "aggression": 0.2},
    "Entangled":   {"brightness": 0.5, "warmth": 0.5, "movement": 0.6, "density": 0.6, "space": 0.5, "aggression": 0.4},
    "Prism":       {"brightness": 0.7, "warmth": 0.4, "movement": 0.5, "density": 0.4, "space": 0.5, "aggression": 0.3},
    "Flux":        {"brightness": 0.5, "warmth": 0.5, "movement": 0.8, "density": 0.5, "space": 0.4, "aggression": 0.5},
    "Aether":      {"brightness": 0.3, "warmth": 0.5, "movement": 0.4, "density": 0.3, "space": 0.9, "aggression": 0.1},
    "Family":      {"brightness": 0.6, "warmth": 0.7, "movement": 0.3, "density": 0.3, "space": 0.5, "aggression": 0.1},
}

# Fallback default
DEFAULT_DNA = {"brightness": 0.5, "warmth": 0.5, "movement": 0.5, "density": 0.5, "space": 0.5, "aggression": 0.3}


def estimate_dna_from_params(params):
    """Rough DNA estimation from parameter values."""
    if not params or not isinstance(params, dict):
        return None

    # Flatten all param values
    all_vals = {}
    for engine_name, engine_params in params.items():
        if isinstance(engine_params, dict):
            for k, v in engine_params.items():
                if isinstance(v, (int, float)):
                    all_vals[k.lower()] = v

    if not all_vals:
        return None

    dna = dict(DEFAULT_DNA)

    # Estimate brightness from filter cutoff params
    for k, v in all_vals.items():
        if 'cutoff' in k or 'bright' in k or 'filter' in k:
            dna['brightness'] = max(0.0, min(1.0, v))
            break

    # Estimate warmth from drive/saturation
    for k, v in all_vals.items():
        if 'drive' in k or 'sat' in k or 'warm' in k:
            dna['warmth'] = max(0.0, min(1.0, v))
            break

    # Estimate movement from LFO rate or movement params
    for k, v in all_vals.items():
        if 'lfo' in k or 'rate' in k or 'movement' in k or 'speed' in k:
            dna['movement'] = max(0.0, min(1.0, v))
            break

    # Estimate space from reverb/delay
    for k, v in all_vals.items():
        if 'reverb' in k or 'space' in k or 'delay' in k or 'room' in k:
            dna['space'] = max(0.0, min(1.0, v))
            break

    # Estimate density
    for k, v in all_vals.items():
        if 'density' in k or 'thick' in k or 'unison' in k:
            dna['density'] = max(0.0, min(1.0, v))
            break

    # Estimate aggression
    for k, v in all_vals.items():
        if 'aggr' in k or 'distort' in k or 'bite' in k or 'attack' in k:
            dna['aggression'] = max(0.0, min(1.0, v))
            break

    return dna


def get_mood_from_path(filepath):
    """Infer mood from file path."""
    parts = filepath.lower().replace('\\', '/').split('/')
    for mood in ['foundation', 'atmosphere', 'entangled', 'prism', 'flux', 'aether', 'family']:
        if mood in parts:
            return mood.capitalize()
    return None


def main():
    filled_from_params = 0
    filled_from_mood = 0
    filled_default = 0
    errors = 0

    for filepath in glob.glob(os.path.join(PRESETS_ROOT, "**", "*.xometa"), recursive=True):
        try:
            with open(filepath, 'r') as f:
                data = json.load(f)

            dna = data.get('dna')
            # Check if DNA is missing, null, or empty
            if dna and isinstance(dna, dict) and any(v for v in dna.values() if v is not None and v != 0):
                continue

            # Try to estimate from parameters
            params = data.get('parameters', {})
            estimated = estimate_dna_from_params(params)

            if estimated and estimated != DEFAULT_DNA:
                data['dna'] = estimated
                filled_from_params += 1
            else:
                # Use mood-based default
                mood = data.get('mood') or get_mood_from_path(filepath)
                if mood and mood in MOOD_DEFAULTS:
                    data['dna'] = dict(MOOD_DEFAULTS[mood])
                    filled_from_mood += 1
                else:
                    data['dna'] = dict(DEFAULT_DNA)
                    filled_default += 1

            with open(filepath, 'w') as f:
                json.dump(data, f, indent=2, ensure_ascii=False)
                f.write('\n')

        except Exception as e:
            errors += 1

    total = filled_from_params + filled_from_mood + filled_default
    print(f"=== Task 3: Fill Missing DNA ===")
    print(f"Filled from parameters: {filled_from_params}")
    print(f"Filled from mood defaults: {filled_from_mood}")
    print(f"Filled with generic defaults: {filled_default}")
    print(f"TOTAL filled: {total}")
    print(f"Errors: {errors}")


if __name__ == '__main__':
    main()
