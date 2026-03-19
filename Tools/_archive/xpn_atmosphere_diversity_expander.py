#!/usr/bin/env python3
"""
xpn_atmosphere_diversity_expander.py

Generates 60 Atmosphere presets targeting underrepresented extreme zones:
  - dark-atmosphere (15):       brightness 0.05-0.15, warmth 0.35-0.65, space 0.60-0.85
  - kinetic-atmosphere (15):    movement 0.80-0.95, density 0.45-0.70, aggression 0.40-0.65
  - dense-atmosphere (15):      density 0.80-0.95, warmth 0.55-0.80, brightness 0.20-0.45
  - aggressive-atmosphere (15): aggression 0.72-0.90, movement 0.65-0.85, density 0.50-0.75

Saves to Presets/XOmnibus/Atmosphere/. Skips existing files.
"""

import json
import os
import random
from pathlib import Path

random.seed(42)

OUTPUT_DIR = Path(__file__).parent.parent / "Presets" / "XOmnibus" / "Atmosphere"
OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

def r(lo, hi, decimals=3):
    return round(random.uniform(lo, hi), decimals)

def clamp(v, lo=0.0, hi=1.0):
    return max(lo, min(hi, v))

# ---------------------------------------------------------------------------
# Engine parameter definitions — macro keys per engine
# ---------------------------------------------------------------------------

ENGINE_MACROS = {
    "OBSIDIAN":   ["macro_darkness", "macro_shimmer", "macro_coupling", "macro_space"],
    "OBSCURA":    ["macro_veil", "macro_depth", "macro_coupling", "macro_space"],
    "OMBRE":      ["macro_gradient", "macro_warmth", "macro_coupling", "macro_space"],
    "ORACLE":     ["macro_vision", "macro_flow", "macro_coupling", "macro_space"],
    "OUROBOROS":  ["macro_cycle", "macro_tension", "macro_coupling", "macro_space"],
    "OCEANIC":    ["macro_tide", "macro_depth", "macro_coupling", "macro_space"],
    "ODDOSCAR":   ["macro_oddity", "macro_weight", "macro_coupling", "macro_space"],
    "OPAL":       ["macro_scatter", "macro_drift", "macro_coupling", "macro_space"],
    "OUTWIT":     ["macro_cunning", "macro_reflex", "macro_coupling", "macro_space"],
    "OCTOPUS":    ["macro_spread", "macro_pulse", "macro_coupling", "macro_space"],
    "ONSET":      ["macro_machine", "macro_punch", "macro_space", "macro_mutate"],
    "OVERWORLD":  ["macro_era", "macro_glitch", "macro_coupling", "macro_space"],
    "ORIGAMI":    ["macro_fold", "macro_tension", "macro_coupling", "macro_space"],
    "OPTIC":      ["macro_focus", "macro_aberration", "macro_coupling", "macro_space"],
    "ORBITAL":    ["macro_orbit", "macro_gravity", "macro_coupling", "macro_space"],
    "OBLONG":     ["macro_character", "macro_movement", "macro_coupling", "macro_space"],
    "OBESE":      ["macro_mass", "macro_density", "macro_coupling", "macro_space"],
    "OVERBITE":   ["macro_bite", "macro_resonance", "macro_coupling", "macro_space"],
    "ORGANON":    ["macro_system", "macro_logic", "macro_coupling", "macro_space"],
    "ORCA":       ["macro_character", "macro_movement", "macro_coupling", "macro_space"],
    "OVERDUB":    ["macro_tape", "macro_drive", "macro_coupling", "macro_space"],
}

def make_parameters(engine, dna):
    """Generate engine-specific parameter block driven by DNA values."""
    macros = ENGINE_MACROS.get(engine, ["macro_1", "macro_2", "macro_coupling", "macro_space"])
    prefix = engine.lower()

    # Map DNA to macro values with some variation
    brightness = dna["brightness"]
    warmth     = dna["warmth"]
    movement   = dna["movement"]
    density    = dna["density"]
    space      = dna["space"]
    aggression = dna["aggression"]

    # Primary macro values derived from DNA
    m1 = clamp(r(max(0.0, aggression - 0.1), min(1.0, aggression + 0.15)))
    m2 = clamp(r(max(0.0, movement   - 0.1), min(1.0, movement   + 0.15)))
    m3 = clamp(r(0.1, 0.5))  # coupling — moderate
    m4 = clamp(r(max(0.0, space - 0.1), min(1.0, space + 0.1)))

    params = {
        f"{prefix}_{macros[0].replace('macro_', '')}": m1,
        f"{prefix}_{macros[1].replace('macro_', '')}": m2,
        f"{prefix}_{macros[2].replace('macro_', '')}": m3,
        f"{prefix}_{macros[3].replace('macro_', '')}": m4,

        f"{prefix}_brightness":  clamp(r(max(0.0, brightness - 0.08), min(1.0, brightness + 0.08))),
        f"{prefix}_warmth":      clamp(r(max(0.0, warmth     - 0.08), min(1.0, warmth     + 0.08))),
        f"{prefix}_density":     clamp(r(max(0.0, density    - 0.08), min(1.0, density    + 0.08))),
        f"{prefix}_space":       clamp(r(max(0.0, space      - 0.08), min(1.0, space      + 0.08))),
        f"{prefix}_reverb_mix":  clamp(r(space * 0.5, min(1.0, space * 0.9))),
        f"{prefix}_amp_attack":  round(r(200, 1200), 1),
        f"{prefix}_amp_release": round(r(800, 3000), 1),
        f"{prefix}_amp_sustain": clamp(r(0.6, 0.95)),
    }
    return params, m1, m2, m3, m4

# ---------------------------------------------------------------------------
# Preset group definitions
# ---------------------------------------------------------------------------

DARK_NAMES = [
    "Void Between Stars", "Obsidian Seabed", "Penumbra Drift",
    "Sunken Archive", "Black Kelp Forest", "Forgotten Cave System",
    "Charcoal Canopy", "Nocturnal Descent", "Shadow Column",
    "Depths Unlit", "Tarpit Horizon", "Crepuscular Shroud",
    "Mariana Whisper", "Umber Passage", "Eclipsed Basin",
]

KINETIC_NAMES = [
    "Cyclone Interior", "Pressure Front", "Thermal Chaos",
    "Electron Storm", "Convection Surge", "Magnetic Squall",
    "Jet Stream Pulse", "Turbulent Ascent", "Wave Front Rush",
    "Piston Weather", "Hurricane Logic", "Updraft Engine",
    "Torrent Algorithm", "Atmospheric Churn", "Kinetic Envelope",
]

DENSE_NAMES = [
    "Lead Atmosphere", "Compressed Column", "Iron Fog",
    "Sediment Cloud", "Viscous Sky", "Dense Nebula Core",
    "Pressure Bloom", "Saturate Horizon", "Mass Accumulation",
    "Weight of Air", "Thick Current", "Loaded Atmosphere",
    "Gravity Pool", "Mineral Haze", "Compacted Vapor",
]

AGGRESSIVE_NAMES = [
    "Storm Pressure", "Rupture Front", "Violent Updraft",
    "Assault Weather", "Crack Thunder Wall", "Shear Force",
    "Impact Atmosphere", "Aggression Index", "Fierce Squall",
    "Rage Current", "Vortex Strike", "Combat Pressure",
    "Predator Sky", "Fractured Ceiling", "Violent Column",
]

DARK_ENGINES = [
    "OBSIDIAN", "OBSCURA", "OMBRE", "ORACLE", "OUROBOROS",
    "OCEANIC", "ODDOSCAR", "OPAL", "OBSIDIAN", "OBSCURA",
    "OMBRE", "ORACLE", "OUROBOROS", "OCEANIC", "OPAL",
]

KINETIC_ENGINES = [
    "OUTWIT", "OCTOPUS", "ONSET", "OVERWORLD", "ORIGAMI",
    "OPTIC", "OUROBOROS", "ORBITAL", "OUTWIT", "OCTOPUS",
    "ONSET", "OVERWORLD", "ORIGAMI", "OPTIC", "ORBITAL",
]

DENSE_ENGINES = [
    "OBLONG", "OBESE", "OVERBITE", "ORGANON", "ORCA",
    "ORIGAMI", "ORACLE", "OVERDUB", "OBLONG", "OBESE",
    "OVERBITE", "ORGANON", "ORCA", "ORIGAMI", "OVERDUB",
]

AGGRESSIVE_ENGINES = [
    "ORCA", "OUROBOROS", "ONSET", "OVERWORLD", "OBLONG",
    "OCTOPUS", "OPTIC", "OUTWIT", "ORCA", "OUROBOROS",
    "ONSET", "OVERWORLD", "OBLONG", "OCTOPUS", "OUTWIT",
]

# ---------------------------------------------------------------------------
# Preset builders
# ---------------------------------------------------------------------------

def build_dark(name, engine):
    brightness = r(0.05, 0.15)
    warmth     = r(0.35, 0.65)
    movement   = r(0.10, 0.40)
    density    = r(0.30, 0.60)
    space      = r(0.60, 0.85)
    aggression = r(0.05, 0.25)

    dna = dict(brightness=brightness, warmth=warmth, movement=movement,
               density=density, space=space, aggression=aggression)
    params, m1, m2, m3, m4 = make_parameters(engine, dna)

    macros = ENGINE_MACROS.get(engine, ["macro_1", "macro_2", "macro_coupling", "macro_space"])

    return {
        "schema_version": 1,
        "name": name,
        "mood": "Atmosphere",
        "engines": [engine],
        "author": "XO_OX",
        "version": "1.0",
        "description": f"Shadowy environmental texture. Low brightness, expansive space. {engine} in void register.",
        "tags": ["atmosphere", "dark", "extreme", "shadowy", "environmental", "deep"],
        "macroLabels": [m.replace("macro_", "").upper() for m in macros],
        "dna": {k: round(v, 3) for k, v in dna.items()},
        "parameters": {
            engine: {
                macros[0].replace("macro_", ""): round(m1, 3),
                macros[1].replace("macro_", ""): round(m2, 3),
                macros[2].replace("macro_", ""): round(m3, 3),
                macros[3].replace("macro_", ""): round(m4, 3),
            }
        },
        "macros": {
            macros[0].replace("macro_", "").upper(): round(m1, 3),
            macros[1].replace("macro_", "").upper(): round(m2, 3),
            macros[2].replace("macro_", "").upper(): round(m3, 3),
            macros[3].replace("macro_", "").upper(): round(m4, 3),
        },
        "couplingIntensity": "None",
        "coupling": {"pairs": []},
    }


def build_kinetic(name, engine):
    brightness = r(0.30, 0.65)
    warmth     = r(0.25, 0.55)
    movement   = r(0.80, 0.95)
    density    = r(0.45, 0.70)
    space      = r(0.40, 0.70)
    aggression = r(0.40, 0.65)

    dna = dict(brightness=brightness, warmth=warmth, movement=movement,
               density=density, space=space, aggression=aggression)
    params, m1, m2, m3, m4 = make_parameters(engine, dna)

    macros = ENGINE_MACROS.get(engine, ["macro_1", "macro_2", "macro_coupling", "macro_space"])

    return {
        "schema_version": 1,
        "name": name,
        "mood": "Atmosphere",
        "engines": [engine],
        "author": "XO_OX",
        "version": "1.0",
        "description": f"Weather system in motion. High movement, active aggression. {engine} kinetic register.",
        "tags": ["atmosphere", "kinetic", "extreme", "movement", "weather", "dynamic"],
        "macroLabels": [m.replace("macro_", "").upper() for m in macros],
        "dna": {k: round(v, 3) for k, v in dna.items()},
        "parameters": {
            engine: {
                macros[0].replace("macro_", ""): round(m1, 3),
                macros[1].replace("macro_", ""): round(m2, 3),
                macros[2].replace("macro_", ""): round(m3, 3),
                macros[3].replace("macro_", ""): round(m4, 3),
            }
        },
        "macros": {
            macros[0].replace("macro_", "").upper(): round(m1, 3),
            macros[1].replace("macro_", "").upper(): round(m2, 3),
            macros[2].replace("macro_", "").upper(): round(m3, 3),
            macros[3].replace("macro_", "").upper(): round(m4, 3),
        },
        "couplingIntensity": "None",
        "coupling": {"pairs": []},
    }


def build_dense(name, engine):
    brightness = r(0.20, 0.45)
    warmth     = r(0.55, 0.80)
    movement   = r(0.15, 0.45)
    density    = r(0.80, 0.95)
    space      = r(0.25, 0.55)
    aggression = r(0.15, 0.40)

    dna = dict(brightness=brightness, warmth=warmth, movement=movement,
               density=density, space=space, aggression=aggression)
    params, m1, m2, m3, m4 = make_parameters(engine, dna)

    macros = ENGINE_MACROS.get(engine, ["macro_1", "macro_2", "macro_coupling", "macro_space"])

    return {
        "schema_version": 1,
        "name": name,
        "mood": "Atmosphere",
        "engines": [engine],
        "author": "XO_OX",
        "version": "1.0",
        "description": f"Thick, heavy environment. Maximum density, warm compression. {engine} dense register.",
        "tags": ["atmosphere", "dense", "extreme", "heavy", "warm", "thick"],
        "macroLabels": [m.replace("macro_", "").upper() for m in macros],
        "dna": {k: round(v, 3) for k, v in dna.items()},
        "parameters": {
            engine: {
                macros[0].replace("macro_", ""): round(m1, 3),
                macros[1].replace("macro_", ""): round(m2, 3),
                macros[2].replace("macro_", ""): round(m3, 3),
                macros[3].replace("macro_", ""): round(m4, 3),
            }
        },
        "macros": {
            macros[0].replace("macro_", "").upper(): round(m1, 3),
            macros[1].replace("macro_", "").upper(): round(m2, 3),
            macros[2].replace("macro_", "").upper(): round(m3, 3),
            macros[3].replace("macro_", "").upper(): round(m4, 3),
        },
        "couplingIntensity": "None",
        "coupling": {"pairs": []},
    }


def build_aggressive(name, engine):
    brightness = r(0.35, 0.70)
    warmth     = r(0.25, 0.55)
    movement   = r(0.65, 0.85)
    density    = r(0.50, 0.75)
    space      = r(0.30, 0.60)
    aggression = r(0.72, 0.90)

    dna = dict(brightness=brightness, warmth=warmth, movement=movement,
               density=density, space=space, aggression=aggression)
    params, m1, m2, m3, m4 = make_parameters(engine, dna)

    macros = ENGINE_MACROS.get(engine, ["macro_1", "macro_2", "macro_coupling", "macro_space"])

    return {
        "schema_version": 1,
        "name": name,
        "mood": "Atmosphere",
        "engines": [engine],
        "author": "XO_OX",
        "version": "1.0",
        "description": f"Storm and pressure atmosphere. Maximum aggression, kinetic density. {engine} aggressive register.",
        "tags": ["atmosphere", "aggressive", "extreme", "storm", "pressure", "fierce"],
        "macroLabels": [m.replace("macro_", "").upper() for m in macros],
        "dna": {k: round(v, 3) for k, v in dna.items()},
        "parameters": {
            engine: {
                macros[0].replace("macro_", ""): round(m1, 3),
                macros[1].replace("macro_", ""): round(m2, 3),
                macros[2].replace("macro_", ""): round(m3, 3),
                macros[3].replace("macro_", ""): round(m4, 3),
            }
        },
        "macros": {
            macros[0].replace("macro_", "").upper(): round(m1, 3),
            macros[1].replace("macro_", "").upper(): round(m2, 3),
            macros[2].replace("macro_", "").upper(): round(m3, 3),
            macros[3].replace("macro_", "").upper(): round(m4, 3),
        },
        "couplingIntensity": "None",
        "coupling": {"pairs": []},
    }


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

GROUPS = [
    ("dark",       DARK_NAMES,       DARK_ENGINES,       build_dark),
    ("kinetic",    KINETIC_NAMES,    KINETIC_ENGINES,    build_kinetic),
    ("dense",      DENSE_NAMES,      DENSE_ENGINES,      build_dense),
    ("aggressive", AGGRESSIVE_NAMES, AGGRESSIVE_ENGINES, build_aggressive),
]

written = 0
skipped = 0

for group_tag, names, engines, builder in GROUPS:
    for i, (name, engine) in enumerate(zip(names, engines)):
        filename = name.replace(" ", "_").replace("/", "-") + ".xometa"
        filepath = OUTPUT_DIR / filename

        if filepath.exists():
            print(f"  SKIP  {filename}")
            skipped += 1
            continue

        preset = builder(name, engine)
        with open(filepath, "w") as f:
            json.dump(preset, f, indent=2)

        print(f"  WRITE [{group_tag:11s}] [{engine:10s}] {name}")
        written += 1

print(f"\nDone. Written: {written}  Skipped (already existed): {skipped}")
print(f"Output directory: {OUTPUT_DIR}")

# Quick DNA stats
print("\n--- DNA Stats Verification ---")
import statistics

all_presets = list(OUTPUT_DIR.glob("*.xometa"))
agg_vals = []
mov_vals = []
den_vals = []
bri_vals = []

for p in all_presets:
    try:
        data = json.loads(p.read_text())
        dna = data.get("dna") or data.get("sonic_dna", {})
        if dna:
            agg_vals.append(dna.get("aggression", 0))
            mov_vals.append(dna.get("movement", 0))
            den_vals.append(dna.get("density", 0))
            bri_vals.append(dna.get("brightness", 0))
    except Exception:
        pass

if agg_vals:
    print(f"  aggression  mean={statistics.mean(agg_vals):.3f}  std={statistics.stdev(agg_vals):.3f}  min={min(agg_vals):.3f}  max={max(agg_vals):.3f}")
    print(f"  movement    mean={statistics.mean(mov_vals):.3f}  std={statistics.stdev(mov_vals):.3f}  min={min(mov_vals):.3f}  max={max(mov_vals):.3f}")
    print(f"  density     mean={statistics.mean(den_vals):.3f}  std={statistics.stdev(den_vals):.3f}  min={min(den_vals):.3f}  max={max(den_vals):.3f}")
    print(f"  brightness  mean={statistics.mean(bri_vals):.3f}  std={statistics.stdev(bri_vals):.3f}  min={min(bri_vals):.3f}  max={max(bri_vals):.3f}")
    print(f"\n  Total presets with DNA in Atmosphere: {len(agg_vals)}")
