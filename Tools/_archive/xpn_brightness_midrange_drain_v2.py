#!/usr/bin/env python3
"""
xpn_brightness_midrange_drain_v2.py

Generates 100 .xometa presets with brightness in extreme zones (≤0.12 or ≥0.88)
to drain the fleet-wide brightness midrange cluster (65.2% in 0.3-0.7 band).

Distribution: 20 presets per mood × 5 moods = 100 presets
Each mood: 10 brightness-XLOW (0.03-0.12) + 10 brightness-XHIGH (0.88-0.98)
"""

import json
import os
import random

random.seed(42)

BASE_DIR = "/Users/joshuacramblet/Documents/GitHub/XO_OX-XOceanus"

ENGINES = [
    "ODDFELIX", "ODDOSCAR", "OVERDUB", "ODYSSEY", "OBLONG", "OBESE",
    "ONSET", "OVERWORLD", "OPAL", "ORBITAL", "ORGANON", "OUROBOROS",
    "OBSIDIAN", "OVERBITE", "ORIGAMI", "ORACLE", "OBSCURA", "OCEANIC",
    "OCELOT", "OPTIC", "OBLIQUE", "OSPREY", "OSTERIA", "OWLFISH",
    "OHM", "ORPHICA", "OBBLIGATO", "OTTONI", "OLE", "OMBRE",
    "ORCA", "OCTOPUS", "OVERLAP", "OUTWIT"
]

COUPLING_TYPES = [
    "SPECTRAL_MORPH", "HARMONIC_LOCK", "RHYTHM_SYNC", "TIMBRAL_BLEND",
    "PHASE_COUPLE", "FREQUENCY_LOCK", "AMPLITUDE_MOD", "ENVELOPE_FOLLOW",
    "PITCH_TRACK", "TEXTURE_WEAVE", "RESONANCE_LINK", "CHAOS_SYNC"
]

MOODS = ["Flux", "Foundation", "Atmosphere", "Aether", "Family"]

# Mood-specific name components
MOOD_NAMES = {
    "Flux": {
        "xlow": [
            "OBSIDIAN CURRENT", "DARK FLOW", "SHADOW DRIFT", "VOID PULSE",
            "MIDNIGHT SURGE", "ECLIPSE WAVE", "UMBRA MOTION", "BLACK TIDE",
            "DARK FLUX", "NIGHT CURRENT"
        ],
        "xhigh": [
            "RADIANT SURGE", "BLINDING FLUX", "SOLAR STORM", "LIGHT BURST",
            "NOVA WAVE", "BRIGHT CHAOS", "LUMINOUS SHIFT", "STELLAR FLUX",
            "WHITE CURRENT", "GLEAM FLOW"
        ]
    },
    "Foundation": {
        "xlow": [
            "OBSIDIAN PILLAR", "DARK BEDROCK", "SHADOW STONE", "VOID ANCHOR",
            "MIDNIGHT CORE", "ECLIPSE BASE", "UMBRA FOUNDATION", "BLACK GRANITE",
            "DEEP ROOT", "NIGHT STONE"
        ],
        "xhigh": [
            "RADIANT PILLAR", "BRIGHT BEDROCK", "LIGHT STONE", "CLEAR ANCHOR",
            "DAWN CORE", "SOLAR BASE", "LUMINOUS FOUNDATION", "WHITE GRANITE",
            "BRIGHT ROOT", "LIGHT STONE RISE"
        ]
    },
    "Atmosphere": {
        "xlow": [
            "OBSIDIAN SKY", "DARK MIST", "SHADOW CLOUD", "VOID HAZE",
            "MIDNIGHT VEIL", "ECLIPSE FOG", "UMBRA DRIFT", "BLACK VAPOR",
            "DARK AIR", "NIGHT MIST"
        ],
        "xhigh": [
            "RADIANT SKY", "BRIGHT MIST", "SOLAR CLOUD", "CLEAR HAZE",
            "DAWN VEIL", "NOON FOG", "LUMINOUS DRIFT", "WHITE VAPOR",
            "BRIGHT AIR", "SUNLIT MIST"
        ]
    },
    "Aether": {
        "xlow": [
            "OBSIDIAN ETHER", "DARK ABYSS", "SHADOW VOID", "VOID ETHER",
            "MIDNIGHT REALM", "ECLIPSE SPACE", "UMBRA AETHER", "BLACK COSMOS",
            "DARK INFINITY", "NIGHT ETHER"
        ],
        "xhigh": [
            "RADIANT ETHER", "BRIGHT ABYSS", "SOLAR VOID", "CLEAR ETHER",
            "DAWN REALM", "NOON SPACE", "LUMINOUS AETHER", "WHITE COSMOS",
            "BRIGHT INFINITY", "LIGHT ETHER"
        ]
    },
    "Family": {
        "xlow": [
            "OBSIDIAN KIN", "DARK GATHERING", "SHADOW BOND", "VOID CIRCLE",
            "MIDNIGHT CLAN", "ECLIPSE TRIBE", "UMBRA FAMILY", "BLACK HEARTH",
            "DEEP KIN", "NIGHT GATHERING"
        ],
        "xhigh": [
            "RADIANT GATHERING", "LUMINOUS KIN", "SOLAR BOND", "BRIGHT CIRCLE",
            "DAWN CLAN", "NOON TRIBE", "LUMINOUS FAMILY", "WHITE HEARTH",
            "BRIGHT KIN", "SUNLIT GATHERING"
        ]
    }
}

MOOD_TAGS = {
    "Flux": ["flux", "dynamic", "shifting"],
    "Foundation": ["foundation", "stable", "grounded"],
    "Atmosphere": ["atmosphere", "ambient", "textural"],
    "Aether": ["aether", "ethereal", "cosmic"],
    "Family": ["family", "warm", "communal"]
}


def rand_extreme_other():
    """Return a random value that's extreme (≤0.15 or ≥0.85)."""
    if random.random() < 0.5:
        return round(random.uniform(0.01, 0.15), 3)
    else:
        return round(random.uniform(0.85, 0.99), 3)


def rand_mid():
    """Return a moderate value for non-extreme dimensions."""
    return round(random.uniform(0.2, 0.8), 3)


def make_preset(mood, brightness_zone, name, engine1, engine2, coupling_type, preset_idx):
    """Generate a single .xometa preset dict."""
    if brightness_zone == "xlow":
        brightness = round(random.uniform(0.03, 0.12), 3)
        tags = [mood.lower(), "dark", "extreme"]
    else:
        brightness = round(random.uniform(0.88, 0.98), 3)
        tags = [mood.lower(), "bright", "extreme"]

    # DNA: brightness set, one other extreme, rest moderate
    dna_dims = ["warmth", "movement", "density", "space", "aggression"]
    extreme_dim = random.choice(dna_dims)
    dna = {"brightness": brightness}
    for dim in dna_dims:
        if dim == extreme_dim:
            dna[dim] = rand_extreme_other()
        else:
            dna[dim] = rand_mid()

    # Add extreme tag for the other dimension
    if dna[extreme_dim] <= 0.15:
        tags.append(f"low-{extreme_dim}")
    else:
        tags.append(f"high-{extreme_dim}")

    coupling_amount = round(random.uniform(0.3, 0.9), 3)

    # Engine parameters — 4 macros each
    def engine_params():
        return {
            "macro_character": round(random.uniform(0.0, 1.0), 3),
            "macro_movement": round(random.uniform(0.0, 1.0), 3),
            "macro_coupling": round(random.uniform(0.0, 1.0), 3),
            "macro_space": round(random.uniform(0.0, 1.0), 3)
        }

    params = {
        engine1: engine_params(),
        engine2: engine_params()
    }

    # Global macros derived from averaged engine params
    macros = {
        "CHARACTER": round((params[engine1]["macro_character"] + params[engine2]["macro_character"]) / 2, 3),
        "MOVEMENT": round((params[engine1]["macro_movement"] + params[engine2]["macro_movement"]) / 2, 3),
        "COUPLING": round((params[engine1]["macro_coupling"] + params[engine2]["macro_coupling"]) / 2, 3),
        "SPACE": round((params[engine1]["macro_space"] + params[engine2]["macro_space"]) / 2, 3)
    }

    return {
        "name": name,
        "version": "1.0",
        "mood": mood,
        "engines": [engine1, engine2],
        "parameters": params,
        "coupling": {
            "type": coupling_type,
            "source": engine1,
            "target": engine2,
            "amount": coupling_amount
        },
        "dna": dna,
        "macros": macros,
        "tags": tags
    }


def generate_all():
    counts = {mood: 0 for mood in MOODS}

    # Build engine pairs — cycle through all 34 engines
    # 100 presets × 2 engines each = 200 engine slots
    # Shuffle engines and tile to get 200 slots with good distribution
    engine_pool = ENGINES * 6  # 204 slots
    random.shuffle(engine_pool)

    # Build coupling cycle — 12 types × ~8-9 each = ~100
    coupling_pool = (COUPLING_TYPES * 9)[:100]
    random.shuffle(coupling_pool)

    slot = 0
    coupling_idx = 0

    for mood in MOODS:
        output_dir = os.path.join(BASE_DIR, "Presets", mood)
        os.makedirs(output_dir, exist_ok=True)

        for zone_idx, zone in enumerate(["xlow"] * 10 + ["xhigh"] * 10):
            name = MOOD_NAMES[mood][zone][zone_idx % 10]

            # Pick two distinct engines from pool
            e1 = engine_pool[slot % len(engine_pool)]
            e2 = engine_pool[(slot + 1) % len(engine_pool)]
            while e2 == e1:
                slot += 1
                e2 = engine_pool[(slot + 1) % len(engine_pool)]

            coupling = coupling_pool[coupling_idx % len(coupling_pool)]
            coupling_idx += 1

            preset = make_preset(mood, zone, name, e1, e2, coupling, slot)

            safe_name = name.replace(" ", "_").replace("/", "-")
            filename = f"{safe_name}.xometa"
            filepath = os.path.join(output_dir, filename)

            if os.path.exists(filepath):
                print(f"  SKIP (exists): {filepath}")
                slot += 2
                continue

            with open(filepath, "w") as f:
                json.dump(preset, f, indent=2)

            counts[mood] += 1
            slot += 2

    return counts


if __name__ == "__main__":
    print("XPN Brightness Midrange Drain v2")
    print("=" * 50)
    counts = generate_all()
    total = sum(counts.values())
    print()
    for mood, count in counts.items():
        print(f"  {mood:12s}: {count:3d} presets written")
    print(f"  {'TOTAL':12s}: {total:3d} presets written")
