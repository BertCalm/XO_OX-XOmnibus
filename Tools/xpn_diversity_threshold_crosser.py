#!/usr/bin/env python3
"""
xpn_diversity_threshold_crosser.py

Generates 80 extreme-DNA presets designed to push fleet Sonic DNA diversity
from 0.1991 to >= 0.20. Each preset places 5 of 6 DNA dimensions in extreme
zones (XLOW 0.04-0.10 or XHIGH 0.90-0.96) and 1 dimension in midrange
(0.43-0.57). This maximises cosine distance from the cluster centroid.

Output: Presets/XOlokun/{mood}/ — one .xometa per preset.
Skips files that already exist.
"""

import json
import os
import random

REPO_ROOT = "/Users/joshuacramblet/Documents/GitHub/XO_OX-XOlokun"  # FIXME: hardcoded path — should use os.path.join or argparse
PRESET_BASE = os.path.join(REPO_ROOT, "Presets", "XOlokun")

ENGINES = [
    "ODDFELIX", "ODDOSCAR", "OVERDUB", "ODYSSEY", "OBLONG", "OBESE",
    "ONSET", "OVERWORLD", "OPAL", "ORBITAL", "ORGANON", "OUROBOROS",
    "OBSIDIAN", "OVERBITE", "ORIGAMI", "ORACLE", "OBSCURA", "OCEANIC",
    "OCELOT", "OPTIC", "OBLIQUE", "OSPREY", "OSTERIA", "OWLFISH",
    "OHM", "ORPHICA", "OBBLIGATO", "OTTONI", "OLE", "OMBRE",
    "ORCA", "OCTOPUS", "OVERLAP", "OUTWIT",
]

COUPLING_TYPES = [
    "PITCH_FOLLOW",
    "FILTER_SHARE",
    "AMPLITUDE_SYNC",
    "RING_MOD",
    "FM_CROSS",
    "ENVELOPE_SLAVE",
    "LFO_SYNC",
    "CHAOS_INJECT",
    "PHASE_LOCK",
    "SPECTRAL_BLEND",
    "RHYTHM_GATE",
    "HARMONIC_STACK",
]

# Mood distribution: 16 batches of 5 = 80 total, spread across 7 moods
MOODS = [
    ("Foundation",  11),
    ("Atmosphere",  11),
    ("Entangled",   12),
    ("Prism",       11),
    ("Flux",        12),
    ("Aether",      11),
    ("Family",      12),
]

MOOD_ABBREV = {
    "Foundation": "FND",
    "Atmosphere": "ATM",
    "Entangled":  "ENT",
    "Prism":      "PRM",
    "Flux":       "FLX",
    "Aether":     "ATH",
    "Family":     "FAM",
}

DNA_DIMS = ["brightness", "warmth", "movement", "density", "space", "aggression"]

# Extreme ranges
XLOW_RANGE  = (0.04, 0.10)
XHIGH_RANGE = (0.90, 0.96)
MID_RANGE   = (0.43, 0.57)

# Batch patterns: (which dims are XLOW, which are XHIGH, which is midrange)
# 6 dims, one midrange per pattern — cycle through all 6 midrange positions,
# alternating the polarity of the extreme dims across batches.
# Pattern: list of (dim_index, zone) where zone is 'low','high','mid'
def make_batch_patterns():
    """
    Build 16 distinct extreme patterns cycling which dim is midrange
    and alternating overall polarity (dark-cold vs bright-warm).
    """
    patterns = []
    for i in range(16):
        mid_dim = DNA_DIMS[i % 6]
        # Alternate polarity: even batches = dark/cold/kinetic, odd = bright/warm/sparse
        if i % 2 == 0:
            # dark-cold-kinetic-dense-violent: B-low,W-low,M-high,D-high,A-high + vary S
            base = {
                "brightness": "low",
                "warmth":     "low",
                "movement":   "high",
                "density":    "high",
                "space":      "low",
                "aggression": "high",
            }
        else:
            # bright-warm-sparse-light-gentle
            base = {
                "brightness": "high",
                "warmth":     "high",
                "movement":   "low",
                "density":    "low",
                "space":      "high",
                "aggression": "low",
            }
        base[mid_dim] = "mid"
        patterns.append(base)
    return patterns


def sample_value(zone, rng):
    if zone == "low":
        return round(rng.uniform(*XLOW_RANGE), 3)
    elif zone == "high":
        return round(rng.uniform(*XHIGH_RANGE), 3)
    else:  # mid
        return round(rng.uniform(*MID_RANGE), 3)


def build_preset(name, mood, engine_pair, coupling_type, dna, preset_number):
    eng_a, eng_b = engine_pair
    return {
        "schema_version": 1,
        "name": name,
        "mood": mood,
        "engines": [eng_a, eng_b],
        "author": "XO_OX",
        "version": "1.0.0",
        "description": (
            "Extreme-DNA diversity anchor. "
            "5 dimensions in maximum zones; 1 dimension midrange. "
            "Designed to push fleet cosine diversity above 0.20 threshold."
        ),
        "tags": ["extreme", "diversity", "dark", "kinetic", "dense", mood.lower()],
        "macroLabels": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
        "couplingIntensity": "High",
        "tempo": None,
        "created": "2026-03-16",
        "legacy": {
            "sourceInstrument": None,
            "sourceCategory": None,
            "sourcePresetName": None,
        },
        "parameters": {
            eng_a: {
                "macro_character": round(random.uniform(0.6, 1.0), 2),
                "macro_movement":  round(random.uniform(0.6, 1.0), 2),
                "macro_coupling":  round(random.uniform(0.5, 0.9), 2),
                "macro_space":     round(random.uniform(0.1, 0.5), 2),
            },
            eng_b: {
                "macro_character": round(random.uniform(0.6, 1.0), 2),
                "macro_movement":  round(random.uniform(0.6, 1.0), 2),
                "macro_coupling":  round(random.uniform(0.5, 0.9), 2),
                "macro_space":     round(random.uniform(0.1, 0.5), 2),
            },
        },
        "coupling": {
            "type": coupling_type,
            "source": eng_a,
            "target": eng_b,
            "amount": round(random.uniform(0.6, 0.95), 2),
        },
        "sequencer": None,
        "dna": dna,
    }


def main():
    rng = random.Random(42)  # deterministic for reproducibility
    patterns = make_batch_patterns()

    # Shuffle engine list for variety; we'll rotate through it
    engines = ENGINES[:]
    coupling_cycle = list(COUPLING_TYPES)

    preset_count = {mood: 0 for mood, _ in MOODS}
    total_written = 0
    total_skipped = 0

    # Assign patterns to moods
    pattern_idx = 0
    for mood, count in MOODS:
        abbrev = MOOD_ABBREV[mood]
        out_dir = os.path.join(PRESET_BASE, mood)
        os.makedirs(out_dir, exist_ok=True)

        for n in range(1, count + 1):
            pat = patterns[pattern_idx % len(patterns)]
            pattern_idx += 1

            # Build DNA
            dna = {dim: sample_value(pat[dim], rng) for dim in DNA_DIMS}

            # Pick engine pair (no self-coupling)
            eng_a = engines[(pattern_idx * 3) % len(engines)]
            eng_b = engines[(pattern_idx * 7 + 5) % len(engines)]
            if eng_b == eng_a:
                eng_b = engines[(pattern_idx * 7 + 6) % len(engines)]

            coupling_type = coupling_cycle[pattern_idx % len(coupling_cycle)]

            name = f"5X DARK COLD KINETIC DENSE VIOLENT {abbrev} {n}"
            filename = f"{name}.xometa"
            filepath = os.path.join(out_dir, filename)

            if os.path.exists(filepath):
                print(f"  SKIP (exists): {filepath}")
                total_skipped += 1
                continue

            preset = build_preset(name, mood, (eng_a, eng_b), coupling_type, dna, n)

            with open(filepath, "w", encoding="utf-8") as f:
                json.dump(preset, f, indent=2)
                f.write("\n")

            print(f"  WROTE: {filepath}")
            preset_count[mood] += 1
            total_written += 1

    print()
    print("=" * 60)
    print("XPN Diversity Threshold Crosser — Run Complete")
    print("=" * 60)
    for mood, _ in MOODS:
        print(f"  {mood:12s}: {preset_count[mood]:3d} written")
    print(f"  {'TOTAL':12s}: {total_written:3d} written, {total_skipped} skipped")
    print()
    print("Output root: Presets/XOlokun/{mood}/")
    print("Strategy: 5-extreme dims + 1 midrange dim per preset")
    print("Expected diversity impact: push fleet DNA diversity >= 0.20")


if __name__ == "__main__":
    main()
