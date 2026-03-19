#!/usr/bin/env python3
"""
xpn_entangled_dark_brightness_pack.py
Generate 80 Entangled presets with extreme-low brightness (<=0.14).
Addresses fleet DNA gap: only 2.7% of presets have brightness <=0.15.

Sub-groups:
  - dark-coupled-bass (20): brightness 0.04-0.12
  - dark-coupled-space (20): brightness 0.05-0.14
  - dark-coupled-movement (20): brightness 0.04-0.13
  - dark-coupled-warmth (20): brightness 0.05-0.12
"""

import json
import os
import random

OUTPUT_DIR = "/Users/joshuacramblet/Documents/GitHub/XO_OX-XOmnibus/Presets/XOmnibus/Entangled"

COUPLING_TYPES = [
    "FILTER_MOD", "PITCH_MOD", "AMP_MOD", "ENVELOPE_SHARE",
    "LFO_SYNC", "RING_MOD", "FEEDBACK", "PHASE_MOD"
]

def r(lo, hi, decimals=2):
    return round(random.uniform(lo, hi), decimals)

def make_preset(name, engine_a, engine_b, dna, coupling_type=None):
    if coupling_type is None:
        coupling_type = random.choice(COUPLING_TYPES)
    source, target = (engine_a, engine_b) if random.random() > 0.5 else (engine_b, engine_a)
    return {
        "name": name,
        "version": "1.0",
        "mood": "Entangled",
        "engines": [engine_a, engine_b],
        "parameters": {
            engine_a: {
                "macro_character": r(0.1, 0.9),
                "macro_movement": r(0.1, 0.9),
                "macro_coupling": r(0.5, 0.9),
                "macro_space": r(0.1, 0.9)
            },
            engine_b: {
                "macro_character": r(0.1, 0.9),
                "macro_movement": r(0.1, 0.9),
                "macro_coupling": r(0.5, 0.9),
                "macro_space": r(0.1, 0.9)
            }
        },
        "coupling": {
            "type": coupling_type,
            "source": source,
            "target": target,
            "amount": r(0.5, 0.95)
        },
        "dna": dna,
        "macros": {
            "CHARACTER": r(0.1, 0.9),
            "MOVEMENT": dna["movement"],
            "COUPLING": r(0.6, 0.95),
            "SPACE": dna["space"]
        },
        "tags": ["entangled", "dark", "low-brightness"]
    }

# ---------------------------------------------------------------------------
# Group 1: dark-coupled-bass (20)
# brightness 0.04-0.12, density 0.75-0.95, aggression 0.55-0.85
# Pairs: OBSIDIAN/OBLONG, OBSIDIAN/OBESE, OMBRE/OUROBOROS, OBSCURA/ONSET, OBSIDIAN/OVERWORLD
# ---------------------------------------------------------------------------
bass_pairs = [
    ("OBSIDIAN", "OBLONG"),
    ("OBSIDIAN", "OBESE"),
    ("OMBRE", "OUROBOROS"),
    ("OBSCURA", "ONSET"),
    ("OBSIDIAN", "OVERWORLD"),
    ("OBLONG", "OBESE"),
    ("OUROBOROS", "OBSIDIAN"),
    ("OBESE", "OVERWORLD"),
    ("ONSET", "OBLONG"),
    ("OMBRE", "OBSIDIAN"),
    ("OBSCURA", "OBLONG"),
    ("OUROBOROS", "OBESE"),
    ("OBSIDIAN", "ONSET"),
    ("OVERWORLD", "OBLONG"),
    ("OBESE", "OBSCURA"),
    ("OMBRE", "ONSET"),
    ("OBSIDIAN", "OUROBOROS"),
    ("OBLONG", "OVERWORLD"),
    ("ONSET", "OBESE"),
    ("OBSCURA", "OVERWORLD"),
]

bass_names = [
    "Shadow Bass Grid", "Dark Coupling Engine", "Obsidian Weight",
    "Hollow Gravity Pull", "Black Soil Resonance", "Cave Floor Pulse",
    "Abyssal Bass Node", "Tar Pit Oscillator", "Tungsten Core Throb",
    "Lead Curtain Low", "Midnight Mud Wave", "Iron Veil Kick",
    "Pitch-Black Sub", "Crude Oil Current", "Subterranean Lock",
    "Dark Matter Bass", "Void Compression", "Charcoal Undertow",
    "Obsidian Marrow", "Black Tide Anchor"
]

group_bass = []
for i, (ea, eb) in enumerate(bass_pairs):
    dna = {
        "brightness": r(0.04, 0.12),
        "warmth": r(0.35, 0.70),
        "movement": r(0.15, 0.55),
        "density": r(0.75, 0.95),
        "space": r(0.10, 0.35),
        "aggression": r(0.55, 0.85)
    }
    group_bass.append(make_preset(bass_names[i], ea, eb, dna))

# ---------------------------------------------------------------------------
# Group 2: dark-coupled-space (20)
# brightness 0.05-0.14, space 0.70-0.90, movement 0.30-0.60
# Pairs: ORACLE/ODYSSEY, OMBRE/OPAL, OBSCURA/OVERDUB, OBSIDIAN/ORIGAMI, OCEANIC/ORACLE
# ---------------------------------------------------------------------------
space_pairs = [
    ("ORACLE", "ODYSSEY"),
    ("OMBRE", "OPAL"),
    ("OBSCURA", "OVERDUB"),
    ("OBSIDIAN", "ORIGAMI"),
    ("OCEANIC", "ORACLE"),
    ("ODYSSEY", "OPAL"),
    ("ORACLE", "OVERDUB"),
    ("OBSIDIAN", "OCEANIC"),
    ("ORIGAMI", "OPAL"),
    ("OVERDUB", "ODYSSEY"),
    ("OMBRE", "ORACLE"),
    ("OPAL", "OCEANIC"),
    ("OBSCURA", "ORIGAMI"),
    ("ORACLE", "OBSIDIAN"),
    ("OVERDUB", "ORIGAMI"),
    ("OCEANIC", "ODYSSEY"),
    ("OMBRE", "OVERDUB"),
    ("ORIGAMI", "ORACLE"),
    ("OPAL", "ODYSSEY"),
    ("OBSCURA", "OCEANIC"),
]

space_names = [
    "Dark Orbit Drift", "Shadow Nebula", "Void Between Stars",
    "Eclipse Resonance", "Obsidian Cosmos", "Black Horizon Field",
    "Dark Matter Halo", "Night Tide Expanse", "Umbra Cascade",
    "Hollow Universe", "Midnight Atmosphere", "Occulted Orbit",
    "Deep Space Fold", "Dark Aether Bloom", "Abyssal Drift Gate",
    "Shadow Void Expanse", "Celestial Blackout", "Nocturnal Orbital",
    "Dark Stratosphere", "Phantom Cosmos"
]

group_space = []
for i, (ea, eb) in enumerate(space_pairs):
    dna = {
        "brightness": r(0.05, 0.14),
        "warmth": r(0.20, 0.55),
        "movement": r(0.30, 0.60),
        "density": r(0.25, 0.60),
        "space": r(0.70, 0.90),
        "aggression": r(0.10, 0.45)
    }
    group_space.append(make_preset(space_names[i], ea, eb, dna))

# ---------------------------------------------------------------------------
# Group 3: dark-coupled-movement (20)
# brightness 0.04-0.13, movement 0.70-0.92, density 0.45-0.70
# Pairs: OUTWIT/ORIGAMI, OCTOPUS/OUROBOROS, OBLONG/OPTIC, OVERWORLD/ONSET, ORIGAMI/OCTOPUS
# ---------------------------------------------------------------------------
movement_pairs = [
    ("OUTWIT", "ORIGAMI"),
    ("OCTOPUS", "OUROBOROS"),
    ("OBLONG", "OPTIC"),
    ("OVERWORLD", "ONSET"),
    ("ORIGAMI", "OCTOPUS"),
    ("OUROBOROS", "OPTIC"),
    ("OUTWIT", "OCTOPUS"),
    ("OVERWORLD", "ORIGAMI"),
    ("ONSET", "OUROBOROS"),
    ("OPTIC", "OUTWIT"),
    ("OCTOPUS", "OVERWORLD"),
    ("ORIGAMI", "OUROBOROS"),
    ("OBLONG", "OUTWIT"),
    ("OPTIC", "ONSET"),
    ("OUROBOROS", "OVERWORLD"),
    ("OUTWIT", "OVERWORLD"),
    ("ORIGAMI", "OPTIC"),
    ("OCTOPUS", "ONSET"),
    ("OBLONG", "OUROBOROS"),
    ("OUTWIT", "ONSET"),
]

movement_names = [
    "Dark Kinetic Web", "Shadow Organism Pulse", "Black Flux Engine",
    "Void Automata March", "Obsidian Motion Field", "Dark Current Cascade",
    "Midnight Kinetics", "Shadow Velocity Swarm", "Abyssal Wave Form",
    "Dark Rhythm Cell", "Nocturnal Flux Grid", "Black Tide Motion",
    "Phantom Automata", "Umbra Surge Wave", "Shadow Locomotion",
    "Dark Swarm Protocol", "Void Momentum Arc", "Obsidian Drift Current",
    "Hollow Motion Engine", "Black Water Churn"
]

group_movement = []
for i, (ea, eb) in enumerate(movement_pairs):
    dna = {
        "brightness": r(0.04, 0.13),
        "warmth": r(0.25, 0.60),
        "movement": r(0.70, 0.92),
        "density": r(0.45, 0.70),
        "space": r(0.20, 0.55),
        "aggression": r(0.30, 0.65)
    }
    group_movement.append(make_preset(movement_names[i], ea, eb, dna))

# ---------------------------------------------------------------------------
# Group 4: dark-coupled-warmth (20)
# brightness 0.05-0.12, warmth 0.72-0.90, density 0.55-0.80
# Pairs: ODDOSCAR/ORGANON, OVERDUB/OPAL, ORACLE/OBBLIGATO, ORGANON/OBSCURA, ORACLE/OHM
# ---------------------------------------------------------------------------
warmth_pairs = [
    ("ODDOSCAR", "ORGANON"),
    ("OVERDUB", "OPAL"),
    ("ORACLE", "OBBLIGATO"),
    ("ORGANON", "OBSCURA"),
    ("ORACLE", "OHM"),
    ("OPAL", "ORGANON"),
    ("OVERDUB", "OHM"),
    ("ODDOSCAR", "ORACLE"),
    ("OBBLIGATO", "ORGANON"),
    ("OBSCURA", "OHM"),
    ("ORGANON", "OVERDUB"),
    ("OHM", "OPAL"),
    ("ORACLE", "OVERDUB"),
    ("OBBLIGATO", "ODDOSCAR"),
    ("OBSCURA", "OPAL"),
    ("OHM", "ORGANON"),
    ("OVERDUB", "OBBLIGATO"),
    ("ODDOSCAR", "OHM"),
    ("ORGANON", "ORACLE"),
    ("OPAL", "OBBLIGATO"),
]

warmth_names = [
    "Dark Ember Coupling", "Shadow Warmth Core", "Obsidian Hearth",
    "Black Velvet Resonance", "Night Fire Synthesis", "Deep Coal Glow",
    "Smoldering Dark Mass", "Umbra Thermal Bloom", "Hollow Warmth Wave",
    "Phantom Heat Cell", "Dark Amber Pulse", "Nocturnal Warmth Field",
    "Obsidian Glow Engine", "Shadow Thermal Web", "Black Candle Hum",
    "Dark Wool Resonance", "Midnight Ember Core", "Void Warmth Cascade",
    "Charcoal Bloom Field", "Deep Maroon Synthesis"
]

group_warmth = []
for i, (ea, eb) in enumerate(warmth_pairs):
    dna = {
        "brightness": r(0.05, 0.12),
        "warmth": r(0.72, 0.90),
        "movement": r(0.15, 0.55),
        "density": r(0.55, 0.80),
        "space": r(0.15, 0.45),
        "aggression": r(0.15, 0.50)
    }
    group_warmth.append(make_preset(warmth_names[i], ea, eb, dna))

# ---------------------------------------------------------------------------
# Write all presets
# ---------------------------------------------------------------------------
all_presets = group_bass + group_space + group_movement + group_warmth

os.makedirs(OUTPUT_DIR, exist_ok=True)

skipped = 0
written = 0
for preset in all_presets:
    filename = preset["name"].replace("/", "-").replace(" ", "_") + ".xometa"
    filepath = os.path.join(OUTPUT_DIR, filename)
    if os.path.exists(filepath):
        print(f"  SKIP (exists): {filename}")
        skipped += 1
        continue
    with open(filepath, "w") as f:
        json.dump(preset, f, indent=2)
    print(f"  WROTE: {filename}  brightness={preset['dna']['brightness']:.2f}")
    written += 1

print(f"\nDone. Written: {written}  Skipped: {skipped}  Total attempted: {len(all_presets)}")

# Verify brightness constraint
violations = [p for p in all_presets if p["dna"]["brightness"] > 0.14]
if violations:
    print(f"\nWARNING: {len(violations)} preset(s) exceed brightness 0.14:")
    for v in violations:
        print(f"  {v['name']}  brightness={v['dna']['brightness']}")
else:
    print("All presets pass brightness <=0.14 constraint.")

# Summary stats
brightnesses = [p["dna"]["brightness"] for p in all_presets]
print(f"\nBrightness stats: min={min(brightnesses):.3f}  max={max(brightnesses):.3f}  "
      f"mean={sum(brightnesses)/len(brightnesses):.3f}")
groups = {"bass": group_bass, "space": group_space, "movement": group_movement, "warmth": group_warmth}
for g, presets in groups.items():
    b = [p["dna"]["brightness"] for p in presets]
    print(f"  {g:10s}: {len(presets)} presets  brightness {min(b):.3f}–{max(b):.3f}")
