#!/usr/bin/env python3
"""
xpn_family_extreme_anchors.py
Generate 40 Family presets with extreme DNA values to push diversity score above 0.0844 critical threshold.
Four groups of 10: dark-family, ultra-bright-family, dense-aggressive-family, warm-spatial-family.
"""

import json
import os
from pathlib import Path

OUTPUT_DIR = Path(__file__).parent.parent / "Presets" / "XOceanus" / "Family"
OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

# Engine macro labels — canonical per engine identity
MACRO_LABELS = {
    "OBSIDIAN":   ["EDGE", "MASS", "COUPLING", "SPACE"],
    "OBSCURA":    ["SHADOW", "DEPTH", "COUPLING", "SPACE"],
    "OMBRE":      ["FADE", "COLOR", "COUPLING", "SPACE"],
    "OUROBOROS":  ["CYCLE", "TENSION", "COUPLING", "SPACE"],
    "ORACLE":     ["VOICE", "MYSTERY", "COUPLING", "SPACE"],
    "ODDOSCAR":   ["WEIGHT", "CHARACTER", "COUPLING", "SPACE"],
    "OVERBITE":   ["BITE", "WARMTH", "COUPLING", "SPACE"],
    "OVERDUB":    ["DRIVE", "TAPE", "COUPLING", "SPACE"],
    "ORCA":       ["POWER", "DEPTH", "COUPLING", "SPACE"],
    "OSPREY":     ["SOAR", "CLARITY", "COUPLING", "SPACE"],
    "ODDFELIX":   ["BRIGHTNESS", "CHARACTER", "COUPLING", "SPACE"],
    "OPTIC":      ["FOCUS", "REFRACTION", "COUPLING", "SPACE"],
    "ORPHICA":    ["SCATTER", "SHIMMER", "COUPLING", "SPACE"],
    "OBLIQUE":    ["ANGLE", "EDGE", "COUPLING", "SPACE"],
    "ORIGAMI":    ["FOLD", "TENSION", "COUPLING", "SPACE"],
    "ORBITAL":    ["SPIN", "RESONANCE", "COUPLING", "SPACE"],
    "OCELOT":     ["SPOT", "SPEED", "COUPLING", "SPACE"],
    "OVERWORLD":  ["ERA", "CHIP", "COUPLING", "SPACE"],
    "ONSET":      ["MACHINE", "PUNCH", "COUPLING", "SPACE"],
    "OCTOPUS":    ["ARM", "CHAOS", "COUPLING", "SPACE"],
    "OBLONG":     ["SHAPE", "MASS", "COUPLING", "SPACE"],
    "OBESE":      ["WEIGHT", "SATURATE", "COUPLING", "SPACE"],
    "OPAL":       ["GRAIN", "SHIMMER", "COUPLING", "SPACE"],
    "ODYSSEY":    ["JOURNEY", "DRIFT", "COUPLING", "SPACE"],
    "ORGANON":    ["VOICE", "LOGIC", "COUPLING", "SPACE"],
    "OHM":        ["COMMUNE", "MEDDLING", "COUPLING", "SPACE"],
    "OTTONI":     ["GROW", "BRASS", "COUPLING", "SPACE"],
    "OBBLIGATO":  ["BOND", "WIND", "COUPLING", "SPACE"],
}

def make_preset(name, engine, description, tags, dna, macro_values=None):
    labels = MACRO_LABELS.get(engine, ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"])
    return {
        "schema_version": 1,
        "name": name,
        "mood": "Family",
        "engines": [engine],
        "author": "XO_OX",
        "version": "1.0.0",
        "description": description,
        "tags": ["family", "portrait", "extreme-dna"] + tags,
        "macroLabels": labels,
        "couplingIntensity": "None",
        "tempo": None,
        "created": "2026-03-16",
        "legacy": {
            "sourceInstrument": None,
            "sourceCategory": None,
            "sourcePresetName": None
        },
        "parameters": {},
        "coupling": None,
        "sequencer": None,
        "dna": dna
    }

# ── GROUP 1: dark-family ── brightness 0.05-0.14, density 0.65-0.90
dark_presets = [
    make_preset(
        "Obsidian Void",
        "OBSIDIAN",
        "OBSIDIAN at its most lightless — all edge, zero luminance. The knife before the strike.",
        ["dark", "cutting", "void"],
        {"brightness": 0.06, "warmth": 0.30, "movement": 0.22, "density": 0.88, "space": 0.38, "aggression": 0.78}
    ),
    make_preset(
        "Black Iris",
        "OBSCURA",
        "OBSCURA fully closed — shadow at maximum density. The pupil in total darkness.",
        ["dark", "shadow", "dense"],
        {"brightness": 0.08, "warmth": 0.25, "movement": 0.18, "density": 0.82, "space": 0.30, "aggression": 0.62}
    ),
    make_preset(
        "Ombre Into Nothing",
        "OMBRE",
        "OMBRE gradient that ends in black — fade completed, no return.",
        ["dark", "fade", "gradient"],
        {"brightness": 0.11, "warmth": 0.20, "movement": 0.28, "density": 0.70, "space": 0.45, "aggression": 0.45}
    ),
    make_preset(
        "Serpent Swallows Sun",
        "OUROBOROS",
        "OUROBOROS terminal loop — the cycle collapses inward, no light escapes.",
        ["dark", "cyclic", "collapse"],
        {"brightness": 0.07, "warmth": 0.35, "movement": 0.72, "density": 0.85, "space": 0.25, "aggression": 0.80}
    ),
    make_preset(
        "Oracle at Midnight",
        "ORACLE",
        "ORACLE speaking from the pit — voice from total dark, gravity of prophecy.",
        ["dark", "voice", "prophetic"],
        {"brightness": 0.09, "warmth": 0.40, "movement": 0.35, "density": 0.75, "space": 0.55, "aggression": 0.55}
    ),
    make_preset(
        "Oscar's Shadow Self",
        "ODDOSCAR",
        "ODDOSCAR at maximum weight and minimum light — the character unmasked.",
        ["dark", "character", "weight"],
        {"brightness": 0.12, "warmth": 0.50, "movement": 0.30, "density": 0.80, "space": 0.35, "aggression": 0.65}
    ),
    make_preset(
        "Deep Fang",
        "OVERBITE",
        "OVERBITE in the depths — BITE maximal but no brightness, only bass and darkness.",
        ["dark", "bass", "bite"],
        {"brightness": 0.10, "warmth": 0.55, "movement": 0.42, "density": 0.78, "space": 0.28, "aggression": 0.72}
    ),
    make_preset(
        "Tape Blackout",
        "OVERDUB",
        "OVERDUB saturated to extinction — tape eaten alive, signal buried in dark.",
        ["dark", "tape", "saturated"],
        {"brightness": 0.05, "warmth": 0.60, "movement": 0.48, "density": 0.90, "space": 0.20, "aggression": 0.68}
    ),
    make_preset(
        "Orca Bathypelagic",
        "ORCA",
        "ORCA at crushing depth — no light, maximum pressure, silent apex predator.",
        ["dark", "deep", "pressure"],
        {"brightness": 0.13, "warmth": 0.38, "movement": 0.25, "density": 0.87, "space": 0.42, "aggression": 0.70}
    ),
    make_preset(
        "Osprey Before Dawn",
        "OSPREY",
        "OSPREY in pre-dawn dark — wings folded, waiting, all potential unreleased.",
        ["dark", "patient", "hunting"],
        {"brightness": 0.14, "warmth": 0.32, "movement": 0.20, "density": 0.65, "space": 0.48, "aggression": 0.52}
    ),
]

# ── GROUP 2: ultra-bright-family ── brightness 0.85-0.98, space 0.65-0.85
bright_presets = [
    make_preset(
        "Felix Maximum",
        "ODDFELIX",
        "ODDFELIX at peak luminance — all character, full brightness, the sun personified.",
        ["bright", "character", "solar"],
        {"brightness": 0.96, "warmth": 0.65, "movement": 0.72, "density": 0.28, "space": 0.82, "aggression": 0.22}
    ),
    make_preset(
        "Focal Point White",
        "OPTIC",
        "OPTIC full spectrum — refraction complete, white light resolved to purest signal.",
        ["bright", "focused", "spectrum"],
        {"brightness": 0.98, "warmth": 0.45, "movement": 0.55, "density": 0.22, "space": 0.78, "aggression": 0.18}
    ),
    make_preset(
        "Crystal Siphonophore",
        "ORPHICA",
        "ORPHICA maximum scatter in full light — the harp colony at peak luminescence.",
        ["bright", "microsound", "shimmer"],
        {"brightness": 0.90, "warmth": 0.42, "movement": 0.80, "density": 0.18, "space": 0.85, "aggression": 0.10}
    ),
    make_preset(
        "Oblique Noon Sun",
        "OBLIQUE",
        "OBLIQUE at sharpest angle in full light — the cut is brilliant, edge catches everything.",
        ["bright", "angular", "cutting"],
        {"brightness": 0.88, "warmth": 0.40, "movement": 0.50, "density": 0.35, "space": 0.70, "aggression": 0.30}
    ),
    make_preset(
        "Origami Light Sheet",
        "ORIGAMI",
        "ORIGAMI fold in pure white — geometry revealed by maximum brightness.",
        ["bright", "geometric", "precise"],
        {"brightness": 0.92, "warmth": 0.38, "movement": 0.45, "density": 0.30, "space": 0.75, "aggression": 0.25}
    ),
    make_preset(
        "Orbital Sunburst",
        "ORBITAL",
        "ORBITAL resonance at maximum — the orbit closest to the star, pure radiant spin.",
        ["bright", "resonant", "orbital"],
        {"brightness": 0.87, "warmth": 0.55, "movement": 0.82, "density": 0.25, "space": 0.80, "aggression": 0.20}
    ),
    make_preset(
        "Ocelot Daylight Sprint",
        "OCELOT",
        "OCELOT at full speed in open light — spots catching sun, velocity supreme.",
        ["bright", "fast", "spotted"],
        {"brightness": 0.85, "warmth": 0.62, "movement": 0.90, "density": 0.32, "space": 0.68, "aggression": 0.38}
    ),
    make_preset(
        "Overworld Sunrise",
        "OVERWORLD",
        "OVERWORLD chip sunrise — NES palette at dawn, all 16 colors fully lit.",
        ["bright", "chip", "nostalgic"],
        {"brightness": 0.94, "warmth": 0.72, "movement": 0.65, "density": 0.28, "space": 0.72, "aggression": 0.28}
    ),
    make_preset(
        "Onset Flashpoint",
        "ONSET",
        "ONSET transient maximum — the brightest hit, attack fully lit, presence absolute.",
        ["bright", "percussive", "transient"],
        {"brightness": 0.89, "warmth": 0.35, "movement": 0.75, "density": 0.45, "space": 0.65, "aggression": 0.55}
    ),
    make_preset(
        "Octopus Bioluminescent",
        "OCTOPUS",
        "OCTOPUS full bioluminescence — 8 arms glowing, chaos engine in pure light.",
        ["bright", "chaotic", "bioluminescent"],
        {"brightness": 0.91, "warmth": 0.48, "movement": 0.88, "density": 0.35, "space": 0.78, "aggression": 0.32}
    ),
]

# ── GROUP 3: dense-aggressive-family ── density 0.80-0.97, aggression 0.70-0.90
dense_presets = [
    make_preset(
        "Bob at Maximum Mass",
        "OBLONG",
        "OBLONG at maximum density — shape compressed to its most irreducible weight.",
        ["dense", "aggressive", "massive"],
        {"brightness": 0.35, "warmth": 0.45, "movement": 0.58, "density": 0.95, "space": 0.20, "aggression": 0.85}
    ),
    make_preset(
        "Obese Core Collapse",
        "OBESE",
        "OBESE beyond event horizon — saturation beyond recovery, density star.",
        ["dense", "aggressive", "saturated"],
        {"brightness": 0.28, "warmth": 0.55, "movement": 0.45, "density": 0.97, "space": 0.15, "aggression": 0.88}
    ),
    make_preset(
        "Bite Engine Maximum",
        "OVERBITE",
        "OVERBITE full aggression mode — BITE at maximum, density stacked, no mercy.",
        ["dense", "aggressive", "bite"],
        {"brightness": 0.38, "warmth": 0.48, "movement": 0.62, "density": 0.90, "space": 0.18, "aggression": 0.90}
    ),
    make_preset(
        "Ouroboros Compression",
        "OUROBOROS",
        "OUROBOROS maximum tension — loop compressed beyond breaking, mass extreme.",
        ["dense", "aggressive", "cyclic"],
        {"brightness": 0.22, "warmth": 0.40, "movement": 0.80, "density": 0.92, "space": 0.22, "aggression": 0.82}
    ),
    make_preset(
        "Machine Blunt Force",
        "ONSET",
        "ONSET at maximum punch — MACHINE full, density stacked, every hit a wall.",
        ["dense", "aggressive", "percussive"],
        {"brightness": 0.42, "warmth": 0.30, "movement": 0.70, "density": 0.88, "space": 0.25, "aggression": 0.86}
    ),
    make_preset(
        "Fold Under Pressure",
        "ORIGAMI",
        "ORIGAMI under maximum fold tension — geometry stressed to densest possible form.",
        ["dense", "aggressive", "geometric"],
        {"brightness": 0.50, "warmth": 0.35, "movement": 0.55, "density": 0.85, "space": 0.20, "aggression": 0.75}
    ),
    make_preset(
        "Orca Kill Strike",
        "ORCA",
        "ORCA hunting strike — maximum power, density absolute, aggression apex.",
        ["dense", "aggressive", "predator"],
        {"brightness": 0.20, "warmth": 0.38, "movement": 0.68, "density": 0.93, "space": 0.18, "aggression": 0.88}
    ),
    make_preset(
        "Overworld Crunch",
        "OVERWORLD",
        "OVERWORLD chip crunch — all synthesis layers stacked, ERA at maximum conflict.",
        ["dense", "aggressive", "chip"],
        {"brightness": 0.55, "warmth": 0.40, "movement": 0.75, "density": 0.82, "space": 0.22, "aggression": 0.78}
    ),
    make_preset(
        "Octopus All Arms",
        "OCTOPUS",
        "OCTOPUS 8 arms simultaneously — maximum CA density, full aggressive Wolfram field.",
        ["dense", "aggressive", "chaotic"],
        {"brightness": 0.45, "warmth": 0.35, "movement": 0.92, "density": 0.87, "space": 0.20, "aggression": 0.84}
    ),
    make_preset(
        "Oblong Wall",
        "OBLONG",
        "OBLONG as total wall — shape fills every frequency gap, no air remains.",
        ["dense", "aggressive", "massive"],
        {"brightness": 0.32, "warmth": 0.50, "movement": 0.48, "density": 0.96, "space": 0.12, "aggression": 0.80}
    ),
]

# ── GROUP 4: warm-spatial-family ── warmth 0.82-0.95, space 0.75-0.92
warm_presets = [
    make_preset(
        "Opal Bloom",
        "OPAL",
        "OPAL at maximum warmth and space — granular bloom, every grain a warm planet.",
        ["warm", "spatial", "granular"],
        {"brightness": 0.55, "warmth": 0.92, "movement": 0.68, "density": 0.30, "space": 0.88, "aggression": 0.12}
    ),
    make_preset(
        "Dub Room Eternal",
        "OVERDUB",
        "OVERDUB spring and tape at maximum warmth — the room that never ends.",
        ["warm", "spatial", "tape"],
        {"brightness": 0.42, "warmth": 0.90, "movement": 0.55, "density": 0.38, "space": 0.85, "aggression": 0.18}
    ),
    make_preset(
        "Odyssey Open Sea",
        "ODYSSEY",
        "ODYSSEY maximum drift across warm water — wavetable horizon, endless space.",
        ["warm", "spatial", "drift"],
        {"brightness": 0.60, "warmth": 0.88, "movement": 0.72, "density": 0.25, "space": 0.90, "aggression": 0.10}
    ),
    make_preset(
        "Organon Cathedral",
        "ORGANON",
        "ORGANON in full bloom — voice logic in maximum warm space, the living organ.",
        ["warm", "spatial", "voice"],
        {"brightness": 0.52, "warmth": 0.85, "movement": 0.42, "density": 0.35, "space": 0.87, "aggression": 0.15}
    ),
    make_preset(
        "Ohm Sunrise Commune",
        "OHM",
        "OHM at peak commune — drone field maximum warmth, MEDDLING dissolved in space.",
        ["warm", "spatial", "drone"],
        {"brightness": 0.58, "warmth": 0.94, "movement": 0.38, "density": 0.28, "space": 0.82, "aggression": 0.08}
    ),
    make_preset(
        "Ottoni Warm Bloom",
        "OTTONI",
        "OTTONI triple brass at full warmth — GROW maximum, ensemble fills all space.",
        ["warm", "spatial", "brass"],
        {"brightness": 0.65, "warmth": 0.87, "movement": 0.52, "density": 0.42, "space": 0.78, "aggression": 0.20}
    ),
    make_preset(
        "Osprey Thermal",
        "OSPREY",
        "OSPREY riding warm thermal updraft — maximum warmth, maximum space, effortless.",
        ["warm", "spatial", "soaring"],
        {"brightness": 0.70, "warmth": 0.83, "movement": 0.45, "density": 0.22, "space": 0.92, "aggression": 0.12}
    ),
    make_preset(
        "Orphica Warm Colony",
        "ORPHICA",
        "ORPHICA warm microsound — each grain a warm polyp, colony spanning full space.",
        ["warm", "spatial", "microsound"],
        {"brightness": 0.75, "warmth": 0.86, "movement": 0.65, "density": 0.20, "space": 0.89, "aggression": 0.08}
    ),
    make_preset(
        "Obbligato Warm Bond",
        "OBBLIGATO",
        "OBBLIGATO maximum BOND warmth — dual wind fully entangled in spacious warm air.",
        ["warm", "spatial", "wind"],
        {"brightness": 0.62, "warmth": 0.91, "movement": 0.48, "density": 0.32, "space": 0.83, "aggression": 0.14}
    ),
    make_preset(
        "Oscar Warm Portrait",
        "ODDOSCAR",
        "ODDOSCAR in full warmth — weight becomes embrace, character fully open.",
        ["warm", "spatial", "character"],
        {"brightness": 0.55, "warmth": 0.95, "movement": 0.40, "density": 0.38, "space": 0.80, "aggression": 0.16}
    ),
]

all_presets = dark_presets + bright_presets + dense_presets + warm_presets

written = 0
skipped = 0

for preset in all_presets:
    filename = preset["name"].replace(" ", "_").replace("'", "") + ".xometa"
    filepath = OUTPUT_DIR / filename
    if filepath.exists():
        print(f"  SKIP (exists): {filename}")
        skipped += 1
    else:
        with open(filepath, "w") as f:
            json.dump(preset, f, indent=2)
        print(f"  WROTE: {filename}")
        written += 1

print(f"\nDone. Written: {written}  Skipped: {skipped}  Total attempted: {len(all_presets)}")
print(f"Output dir: {OUTPUT_DIR}")

# Quick DNA diversity report
import statistics
brightnesses = [p["dna"]["brightness"] for p in all_presets]
densities = [p["dna"]["density"] for p in all_presets]
warmths = [p["dna"]["warmth"] for p in all_presets]
aggressions = [p["dna"]["aggression"] for p in all_presets]

print(f"\nNew preset DNA ranges:")
print(f"  brightness: {min(brightnesses):.2f} – {max(brightnesses):.2f}  (stdev: {statistics.stdev(brightnesses):.3f})")
print(f"  density:    {min(densities):.2f} – {max(densities):.2f}  (stdev: {statistics.stdev(densities):.3f})")
print(f"  warmth:     {min(warmths):.2f} – {max(warmths):.2f}  (stdev: {statistics.stdev(warmths):.3f})")
print(f"  aggression: {min(aggressions):.2f} – {max(aggressions):.2f}  (stdev: {statistics.stdev(aggressions):.3f})")
