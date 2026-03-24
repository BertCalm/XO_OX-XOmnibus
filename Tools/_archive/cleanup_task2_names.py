#!/usr/bin/env python3
"""Task 2: Fix 1,498 preset names exceeding 30 chars.

DNA-descriptor names (e.g. "5X DARK_COLD_KINETIC_DENSE_VIOLENT_ENT_1") get
evocative 2-3 word names based on DNA values. Others get truncated to 27+"..."
"""

import json
import glob
import os
import re
import random

random.seed(42)  # reproducible

PRESETS_ROOT = "/home/user/XO_OX-XOlokun/Presets"

# Name generation pools based on DNA dimensions
BRIGHT_COLD = ["Crystal", "Glass", "Ice", "Frost", "Silver", "Prism", "Quartz", "Mirror", "Arctic", "Chrome"]
DARK_WARM = ["Ember", "Velvet", "Shadow", "Dusk", "Smoke", "Amber", "Copper", "Mahogany", "Cocoa", "Rust"]
HIGH_MOVEMENT = ["Drift", "Pulse", "Surge", "Current", "Stream", "Flutter", "Cascade", "Swirl", "Ripple", "Shimmer"]
HIGH_DENSITY = ["Dense", "Heavy", "Thick", "Massive", "Deep", "Solid", "Molten", "Saturated", "Packed", "Titan"]
HIGH_AGGRESSION = ["Storm", "Strike", "Blade", "Edge", "Force", "Fang", "Wrath", "Grit", "Rage", "Crush"]
HIGH_SPACE = ["Vast", "Horizon", "Expanse", "Cathedral", "Abyss", "Cavern", "Orbit", "Void", "Canyon", "Dome"]
WARM_BRIGHT = ["Glow", "Honey", "Sunrise", "Golden", "Halo", "Candlelight", "Radiant", "Solar", "Bloom", "Peach"]
NEUTRAL = ["Signal", "Phase", "Tone", "Wave", "Grain", "Node", "Flux", "Thread", "Shard", "Fragment"]

# DNA descriptor pattern: words like DARK, COLD, KINETIC, etc separated by underscores,
# possibly with a prefix like "5X " and suffix like "_ENT_1" or "_FND_4"
DNA_PATTERN = re.compile(
    r'^(?:\d+X\s+)?'  # optional "5X " prefix
    r'[A-Z_]+$'       # ALL CAPS with underscores
)

# Also match patterns like "BRIGHT_COLD_STILL_SPARSE_INTIMATE_FND_4"
DNA_KEYWORDS = {
    'BRIGHT', 'DARK', 'HOT', 'COLD', 'WARM', 'COOL',
    'KINETIC', 'STILL', 'MOVING', 'FLOWING',
    'DENSE', 'SPARSE', 'THICK', 'THIN',
    'VIOLENT', 'GENTLE', 'AGGRESSIVE', 'PEACEFUL', 'CALM',
    'VAST', 'INTIMATE', 'WIDE', 'NARROW',
    'DRY', 'WET', 'AIRY', 'TIGHT',
    'ENT', 'FND', 'ATM', 'PRS', 'FLX', 'AET', 'FAM',
    'AGG', 'MOV', 'SPC', 'DNS', 'BRT', 'WRM'
}


def is_dna_descriptor_name(name):
    """Check if name matches DNA descriptor format."""
    # Remove number suffix like _1, _2, _3
    cleaned = re.sub(r'_\d+$', '', name)
    # Remove prefix like "5X "
    cleaned = re.sub(r'^\d+X\s+', '', cleaned)
    # Check if remaining is all caps with underscores
    parts = cleaned.split('_')
    if len(parts) >= 3:
        # At least 3 uppercase word segments
        caps_parts = sum(1 for p in parts if p.isupper() and len(p) >= 2)
        if caps_parts >= 3:
            return True
    return False


def generate_evocative_name(dna, existing_names):
    """Generate a 2-3 word evocative name from DNA values."""
    if not dna or not isinstance(dna, dict):
        return None

    brightness = dna.get('brightness', 0.5)
    warmth = dna.get('warmth', 0.5)
    movement = dna.get('movement', 0.5)
    density = dna.get('density', 0.5)
    space = dna.get('space', 0.5)
    aggression = dna.get('aggression', 0.5)

    # Pick primary descriptor pool based on strongest dimension
    candidates = []

    # Brightness + warmth interaction
    if brightness > 0.6 and warmth < 0.4:
        candidates.append(random.choice(BRIGHT_COLD))
    elif brightness < 0.4 and warmth > 0.6:
        candidates.append(random.choice(DARK_WARM))
    elif brightness > 0.6 and warmth > 0.6:
        candidates.append(random.choice(WARM_BRIGHT))
    else:
        candidates.append(random.choice(NEUTRAL))

    # Secondary: pick from strongest remaining dimension
    dims = [
        (movement, HIGH_MOVEMENT),
        (density, HIGH_DENSITY),
        (aggression, HIGH_AGGRESSION),
        (space, HIGH_SPACE),
    ]
    dims.sort(key=lambda x: x[0], reverse=True)
    top_dim_val, top_dim_pool = dims[0]
    if top_dim_val > 0.5:
        candidates.append(random.choice(top_dim_pool))
    else:
        candidates.append(random.choice(NEUTRAL))

    name = " ".join(candidates[:2])

    # Ensure uniqueness
    base_name = name
    suffix = 1
    lower_existing = {n.lower() for n in existing_names}
    while name.lower() in lower_existing:
        suffix += 1
        name = f"{base_name} {suffix}"

    if len(name) > 30:
        name = name[:27] + "..."

    return name


def main():
    # First pass: collect all existing names
    all_names = set()
    files_data = []

    for filepath in glob.glob(os.path.join(PRESETS_ROOT, "**", "*.xometa"), recursive=True):
        try:
            with open(filepath, 'r') as f:
                data = json.load(f)
            name = data.get('name', '')
            if name:
                all_names.add(name)
            files_data.append((filepath, data))
        except:
            pass

    renamed_dna = 0
    renamed_truncated = 0

    for filepath, data in files_data:
        name = data.get('name', '')
        if not name or len(name) <= 30:
            continue

        modified = False

        if is_dna_descriptor_name(name):
            dna = data.get('dna', {})
            new_name = generate_evocative_name(dna, all_names)
            if new_name:
                data['name'] = new_name
                all_names.add(new_name)
                renamed_dna += 1
                modified = True
            else:
                # Fallback to truncation
                data['name'] = name[:27] + "..."
                renamed_truncated += 1
                modified = True
        else:
            data['name'] = name[:27] + "..."
            renamed_truncated += 1
            modified = True

        if modified:
            with open(filepath, 'w') as f:
                json.dump(data, f, indent=2, ensure_ascii=False)
                f.write('\n')

    total = renamed_dna + renamed_truncated
    print(f"=== Task 2: Name Cleanup ===")
    print(f"DNA-descriptor names renamed: {renamed_dna}")
    print(f"Truncated names: {renamed_truncated}")
    print(f"TOTAL fixed: {total}")


if __name__ == '__main__':
    main()
