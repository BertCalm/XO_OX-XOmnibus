#!/usr/bin/env python3
"""
Generate 80 .xometa presets: 40 Aether + 40 Family.
Each preset has 4 DNA dimensions in extreme zones (XLOW: 0.04–0.11 or XHIGH: 0.89–0.96)
and 2 dimensions in midrange (0.40–0.60).

Output:
  Presets/XOlokun/Aether/  — 40 presets
  Presets/XOlokun/Family/  — 40 presets

Usage:
  python3 Tools/xpn_aether_family_ultra_diverse.py
"""

import json, os, random
from datetime import date

random.seed(2026)

REPO_ROOT = os.path.join(os.path.dirname(__file__), '..')
AETHER_DIR = os.path.join(REPO_ROOT, 'Presets', 'XOlokun', 'Aether')
FAMILY_DIR = os.path.join(REPO_ROOT, 'Presets', 'XOlokun', 'Family')
TODAY = str(date.today())

# ─── All 34 engines ──────────────────────────────────────────────────────────
ALL_ENGINES = [
    'ODDFELIX', 'ODDOSCAR', 'OVERDUB', 'ODYSSEY', 'OBLONG', 'OBESE',
    'ONSET', 'OVERWORLD', 'OPAL', 'ORBITAL', 'ORGANON', 'OUROBOROS',
    'OBSIDIAN', 'OVERBITE', 'ORIGAMI', 'ORACLE', 'OBSCURA', 'OCEANIC',
    'OCELOT', 'OPTIC', 'OBLIQUE', 'OSPREY', 'OSTERIA', 'OWLFISH',
    'OHM', 'ORPHICA', 'OBBLIGATO', 'OTTONI', 'OLE', 'OMBRE',
    'ORCA', 'OCTOPUS', 'OVERLAP', 'OUTWIT',
]

ALL_COUPLING_TYPES = [
    'FREQUENCY_SHIFT', 'AMPLITUDE_MOD', 'FILTER_MOD', 'PITCH_SYNC',
    'TIMBRE_BLEND', 'ENVELOPE_LINK', 'HARMONIC_FOLD', 'CHAOS_INJECT',
    'RESONANCE_SHARE', 'SPATIAL_COUPLE', 'SPECTRAL_MORPH', 'VELOCITY_COUPLE',
]

# DNA dimension shorthand keys
# B=brightness, W=warmth, M=movement, D=density, S=space, A=aggression

def xlow():
    return round(random.uniform(0.04, 0.11), 3)

def xhigh():
    return round(random.uniform(0.89, 0.96), 3)

def mid():
    return round(random.uniform(0.40, 0.60), 3)

def build_dna(combo):
    """
    combo: dict of {dim: 'XLOW'|'XHIGH'|'MID'} for all 6 dims.
    Exactly 4 extreme + 2 mid.
    """
    MAP = {'B': 'brightness', 'W': 'warmth', 'M': 'movement',
           'D': 'density', 'S': 'space', 'A': 'aggression'}
    dna = {}
    for short, val_type in combo.items():
        key = MAP[short]
        if val_type == 'XLOW':
            dna[key] = xlow()
        elif val_type == 'XHIGH':
            dna[key] = xhigh()
        else:
            dna[key] = mid()
    return dna

def sanitize_filename(name):
    return name.replace(' ', '_').replace("'", '').replace('/', '_').replace(':', '') + '.xometa'

def pick_engines(seed_idx, count=2):
    """Deterministically pick engines cycling through all 34."""
    out = []
    for i in range(count):
        out.append(ALL_ENGINES[(seed_idx + i * 7) % len(ALL_ENGINES)])
    # Deduplicate preserving order
    seen = set()
    result = []
    for e in out:
        if e not in seen:
            seen.add(e)
            result.append(e)
    if len(result) < count:
        for e in ALL_ENGINES:
            if e not in seen:
                seen.add(e)
                result.append(e)
            if len(result) == count:
                break
    return result

def pick_coupling_type(idx):
    return ALL_COUPLING_TYPES[idx % len(ALL_COUPLING_TYPES)]

def make_preset(name, mood, engines_list, dna, global_idx):
    """Build a .xometa preset dict."""
    # Build minimal parameter block for each engine
    params = {}
    for eng in engines_list:
        params[eng] = {
            'level': round(random.uniform(0.65, 0.85), 3),
            'pan': round(random.uniform(-0.15, 0.15), 3),
        }

    # Coupling between engines if 2+
    coupling = None
    if len(engines_list) >= 2:
        coupling = [{
            'source': engines_list[0],
            'destination': engines_list[1],
            'type': pick_coupling_type(global_idx),
            'amount': round(random.uniform(0.3, 0.75), 3),
        }]

    # Tags derived from DNA
    tags = [mood.lower()]
    if dna['brightness'] < 0.15:
        tags.append('dark')
    elif dna['brightness'] > 0.85:
        tags.append('bright')
    if dna['warmth'] < 0.15:
        tags.append('cold')
    elif dna['warmth'] > 0.85:
        tags.append('warm')
    if dna['movement'] < 0.15:
        tags.append('still')
    elif dna['movement'] > 0.85:
        tags.append('kinetic')
    if dna['density'] < 0.15:
        tags.append('sparse')
    elif dna['density'] > 0.85:
        tags.append('dense')
    if dna['space'] < 0.15:
        tags.append('intimate')
    elif dna['space'] > 0.85:
        tags.append('vast')
    if dna['aggression'] < 0.15:
        tags.append('gentle')
    elif dna['aggression'] > 0.85:
        tags.append('violent')
    tags.append('ultra-diverse')

    return {
        'schema_version': 1,
        'name': name,
        'mood': mood,
        'engines': engines_list,
        'author': 'XO_OX Designs',
        'version': '1.0.0',
        'description': f'Extreme DNA corners — {", ".join(tags[:4])}.',
        'tags': tags[:6],
        'macroLabels': ['CHARACTER', 'MOVEMENT', 'COUPLING', 'SPACE'],
        'couplingIntensity': 'High' if coupling else 'None',
        'tempo': None,
        'created': TODAY,
        'dna': dna,
        'parameters': params,
        'coupling': coupling,
        'sequencer': None,
    }

# ─── Aether corner combos (8 × 5 = 40) ──────────────────────────────────────
# Format: {dim: 'XLOW'|'XHIGH'|'MID'}, 4 extreme + 2 mid
AETHER_COMBOS = [
    # 1. B-XLOW + W-XLOW + A-XHIGH + D-XHIGH
    {'B':'XLOW','W':'XLOW','M':'MID','D':'XHIGH','S':'MID','A':'XHIGH'},
    # 2. B-XHIGH + W-XHIGH + M-XHIGH + S-XHIGH
    {'B':'XHIGH','W':'XHIGH','M':'XHIGH','D':'MID','S':'XHIGH','A':'MID'},
    # 3. B-XLOW + S-XHIGH + M-XHIGH + A-XHIGH
    {'B':'XLOW','W':'MID','M':'XHIGH','D':'MID','S':'XHIGH','A':'XHIGH'},
    # 4. B-XHIGH + D-XLOW + A-XLOW + W-XLOW
    {'B':'XHIGH','W':'XLOW','M':'MID','D':'XLOW','S':'MID','A':'XLOW'},
    # 5. W-XHIGH + D-XHIGH + M-XLOW + S-XLOW
    {'B':'MID','W':'XHIGH','M':'XLOW','D':'XHIGH','S':'XLOW','A':'MID'},
    # 6. W-XLOW + S-XHIGH + B-XLOW + M-XHIGH
    {'B':'XLOW','W':'XLOW','M':'XHIGH','D':'MID','S':'XHIGH','A':'MID'},
    # 7. B-XHIGH + W-XLOW + D-XHIGH + M-XHIGH
    {'B':'XHIGH','W':'XLOW','M':'XHIGH','D':'XHIGH','S':'MID','A':'MID'},
    # 8. B-XLOW + W-XHIGH + S-XHIGH + A-XHIGH
    {'B':'XLOW','W':'XHIGH','M':'MID','D':'MID','S':'XHIGH','A':'XHIGH'},
]

AETHER_COMBO_NAMES = [
    'DARK COLD DENSE VIOLENT',
    'BRIGHT WARM KINETIC VAST',
    'DARK KINETIC VAST VIOLENT',
    'BRIGHT COLD SPARSE GENTLE',
    'WARM DENSE STILL INTIMATE',
    'DARK COLD KINETIC VAST',
    'BRIGHT COLD KINETIC DENSE',
    'DARK WARM VAST VIOLENT',
]

# ─── Family corner combos (8 × 5 = 40) ──────────────────────────────────────
FAMILY_COMBOS = [
    # 1. B-XLOW + W-XLOW + A-XHIGH + S-XHIGH
    {'B':'XLOW','W':'XLOW','M':'MID','D':'MID','S':'XHIGH','A':'XHIGH'},
    # 2. B-XHIGH + W-XHIGH + D-XHIGH + M-XHIGH
    {'B':'XHIGH','W':'XHIGH','M':'XHIGH','D':'XHIGH','S':'MID','A':'MID'},
    # 3. B-XLOW + D-XLOW + A-XLOW + M-XLOW (ultra-gentle sparse dark still)
    {'B':'XLOW','W':'MID','M':'XLOW','D':'XLOW','S':'MID','A':'XLOW'},
    # 4. B-XHIGH + W-XHIGH + A-XLOW + S-XHIGH (bright warm gentle vast)
    {'B':'XHIGH','W':'XHIGH','M':'MID','D':'MID','S':'XHIGH','A':'XLOW'},
    # 5. W-XHIGH + M-XHIGH + A-XHIGH + D-XHIGH (hot kinetic violent dense)
    {'B':'MID','W':'XHIGH','M':'XHIGH','D':'XHIGH','S':'MID','A':'XHIGH'},
    # 6. W-XLOW + D-XLOW + S-XHIGH + B-XHIGH (cold sparse vast bright)
    {'B':'XHIGH','W':'XLOW','M':'MID','D':'XLOW','S':'XHIGH','A':'MID'},
    # 7. B-XLOW + W-XHIGH + D-XHIGH + A-XHIGH (dark hot dense violent)
    {'B':'XLOW','W':'XHIGH','M':'MID','D':'XHIGH','S':'MID','A':'XHIGH'},
    # 8. B-XHIGH + M-XLOW + S-XLOW + D-XHIGH (bright still intimate dense)
    {'B':'XHIGH','W':'MID','M':'XLOW','D':'XHIGH','S':'XLOW','A':'MID'},
]

FAMILY_COMBO_NAMES = [
    'DARK COLD VAST VIOLENT',
    'BRIGHT WARM KINETIC DENSE',
    'DARK SPARSE STILL GENTLE',
    'BRIGHT WARM VAST GENTLE',
    'HOT KINETIC DENSE VIOLENT',
    'COLD SPARSE VAST BRIGHT',
    'DARK HOT DENSE VIOLENT',
    'BRIGHT STILL INTIMATE DENSE',
]

def write_preset(preset, out_dir):
    """Write preset to disk; skip if file exists."""
    fname = sanitize_filename(preset['name'])
    path = os.path.join(out_dir, fname)
    if os.path.exists(path):
        return False
    with open(path, 'w') as f:
        json.dump(preset, f, indent=2)
    return True

def main():
    os.makedirs(AETHER_DIR, exist_ok=True)
    os.makedirs(FAMILY_DIR, exist_ok=True)

    aether_written = 0
    aether_skipped = 0
    family_written = 0
    family_skipped = 0

    global_idx = 0

    # ── Aether: 8 combos × 5 presets ─────────────────────────────────────────
    for combo_idx, (combo, combo_name) in enumerate(zip(AETHER_COMBOS, AETHER_COMBO_NAMES)):
        for variant in range(1, 6):
            name = f"{combo_name} AET {variant}"
            dna = build_dna(combo)
            engines = pick_engines(global_idx, count=2)
            preset = make_preset(name, 'Aether', engines, dna, global_idx)
            written = write_preset(preset, AETHER_DIR)
            if written:
                aether_written += 1
            else:
                aether_skipped += 1
            global_idx += 1

    # ── Family: 8 combos × 5 presets ─────────────────────────────────────────
    for combo_idx, (combo, combo_name) in enumerate(zip(FAMILY_COMBOS, FAMILY_COMBO_NAMES)):
        for variant in range(1, 6):
            name = f"{combo_name} FAM {variant}"
            dna = build_dna(combo)
            engines = pick_engines(global_idx, count=2)
            preset = make_preset(name, 'Family', engines, dna, global_idx)
            written = write_preset(preset, FAMILY_DIR)
            if written:
                family_written += 1
            else:
                family_skipped += 1
            global_idx += 1

    print(f"Aether: {aether_written} written, {aether_skipped} skipped (already existed)")
    print(f"Family: {family_written} written, {family_skipped} skipped (already existed)")
    print(f"Total new presets: {aether_written + family_written}")
    print(f"Output dirs:")
    print(f"  {os.path.realpath(AETHER_DIR)}")
    print(f"  {os.path.realpath(FAMILY_DIR)}")

if __name__ == '__main__':
    main()
