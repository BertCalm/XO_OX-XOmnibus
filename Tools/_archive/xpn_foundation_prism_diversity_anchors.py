#!/usr/bin/env python3
"""
xpn_foundation_prism_diversity_anchors.py

Generates extreme-zone anchor presets for Foundation and Prism moods to boost
DNA diversity scores from the current LOW range (0.12-0.15).

Foundation (30 presets):
  - foundation-dark (10):          brightness 0.05-0.15, density 0.60-0.85
  - foundation-aggressive (10):    aggression 0.75-0.92, density 0.70-0.90
  - foundation-bright-sparse (10): brightness 0.80-0.95, density 0.05-0.20

Prism (30 presets):
  - prism-dark-dense (10):         brightness 0.05-0.15, density 0.80-0.95, aggression 0.65-0.85
  - prism-ultra-bright (10):       brightness 0.88-0.98, warmth 0.10-0.25, space 0.70-0.90
  - prism-brutal-movement (10):    aggression 0.82-0.95, movement 0.80-0.95, density 0.55-0.78
"""

import json
import os
import random

random.seed(42)  # Reproducible output

FOUNDATION_DIR = "/Users/joshuacramblet/Documents/GitHub/XO_OX-XOmnibus/Presets/XOmnibus/Foundation"
PRISM_DIR      = "/Users/joshuacramblet/Documents/GitHub/XO_OX-XOmnibus/Presets/XOmnibus/Prism"

FOUNDATION_ENGINES = ["ODDFELIX", "ODDOSCAR", "OBLONG", "ONSET", "OVERDUB", "ODYSSEY", "OVERWORLD", "OPAL"]
PRISM_ENGINES      = ["OPTIC", "OBLIQUE", "ORIGAMI", "ORACLE", "OVERWORLD", "OBLONG", "ONSET", "ORBITAL"]

# --------------------------------------------------------------------------- #
# Engine → parameter prefix + macro label map
# --------------------------------------------------------------------------- #
ENGINE_META = {
    "ODDFELIX":  {"prefix": "felix_",   "macros": ["VOICE", "MOTION", "COUPLE", "SPACE"]},
    "ODDOSCAR":  {"prefix": "oscar_",   "macros": ["WEIGHT", "BREATH", "COUPLE", "SPACE"]},
    "OBLONG":    {"prefix": "bob_",     "macros": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"]},
    "ONSET":     {"prefix": "onset_",   "macros": ["MACHINE", "PUNCH", "SPACE", "MUTATE"]},
    "OVERDUB":   {"prefix": "dub_",     "macros": ["VOICE", "SEND", "ECHO", "PANIC"]},
    "ODYSSEY":   {"prefix": "od_",      "macros": ["ODYSSEY", "MOTION", "COUPLING", "SPACE"]},
    "OVERWORLD": {"prefix": "ow_",      "macros": ["ERA", "CHAOS", "COUPLE", "SPACE"]},
    "OPAL":      {"prefix": "opal_",    "macros": ["GRAIN", "SCATTER", "DENSITY", "SPACE"]},
    "OPTIC":     {"prefix": "optic_",   "macros": ["REFRACT", "SPECTRUM", "SCATTER", "SPACE"]},
    "OBLIQUE":   {"prefix": "obl_",     "macros": ["ANGLE", "MOTION", "COUPLE", "SPACE"]},
    "ORIGAMI":   {"prefix": "ori_",     "macros": ["FOLD", "TENSION", "COUPLE", "SPACE"]},
    "ORACLE":    {"prefix": "orc_",     "macros": ["SIGNAL", "DRIFT", "COUPLE", "SPACE"]},
    "ORBITAL":   {"prefix": "orb_",     "macros": ["ORBIT", "PHASE", "COUPLE", "SPACE"]},
}

# Generic macro param keys used in parameters block (prefix + macro_N)
def engine_params(engine: str, macro_vals: list[float]) -> dict:
    meta   = ENGINE_META.get(engine, {"prefix": engine.lower() + "_", "macros": ["M1", "M2", "M3", "M4"]})
    prefix = meta["prefix"]
    labels = meta["macros"]
    params = {}
    for i, (label, val) in enumerate(zip(labels, macro_vals)):
        key = f"{prefix}macro_{label.lower()}"
        params[key] = round(val, 4)
    # Add standard coupling + output
    params[f"{prefix}outputLevel"]    = 0.85
    params[f"{prefix}couplingLevel"]  = 0.5
    params[f"{prefix}couplingBus"]    = 0
    return params


def make_macros(engine: str, vals: list[float]) -> dict:
    meta   = ENGINE_META.get(engine, {"prefix": "", "macros": ["M1", "M2", "M3", "M4"]})
    labels = meta["macros"]
    return {label: round(v, 4) for label, v in zip(labels, vals)}


def rand_range(lo: float, hi: float) -> float:
    return round(random.uniform(lo, hi), 4)


def preset(name: str, mood: str, engine: str, dna: dict, tags: list[str], description: str) -> dict:
    macro_vals = [rand_range(0.2, 0.9) for _ in range(4)]
    # Steer macro CHARACTER/WEIGHT/VOICE toward dna cues
    if dna["brightness"] < 0.2:
        macro_vals[0] = rand_range(0.1, 0.35)   # low character = dark timbre
    elif dna["brightness"] > 0.8:
        macro_vals[0] = rand_range(0.65, 0.95)  # high character = bright timbre
    if dna.get("aggression", 0.3) > 0.75:
        macro_vals[1] = rand_range(0.70, 0.95)  # high motion = aggressive
    if dna.get("density", 0.5) > 0.75:
        macro_vals[3] = rand_range(0.60, 0.85)  # high space fill

    meta_labels = ENGINE_META.get(engine, {"macros": ["M1", "M2", "M3", "M4"]})["macros"]

    return {
        "schema_version": 1,
        "name": name,
        "mood": mood,
        "sonic_dna": dna,
        "dna": dna,
        "engines": [engine],
        "author": "XO_OX",
        "version": "1.0.0",
        "description": description,
        "tags": tags,
        "macroLabels": meta_labels,
        "macros": make_macros(engine, macro_vals),
        "couplingIntensity": "None",
        "parameters": {engine: engine_params(engine, macro_vals)},
        "coupling": {"pairs": []},
    }


def save(data: dict, directory: str):
    filename = data["name"].replace(" ", "_").replace("/", "-") + ".xometa"
    path = os.path.join(directory, filename)
    if os.path.exists(path):
        print(f"  SKIP (exists): {filename}")
        return
    with open(path, "w") as f:
        json.dump(data, f, indent=2)
    print(f"  WROTE: {filename}")


# --------------------------------------------------------------------------- #
# Foundation — Dark (brightness 0.05-0.15, density 0.60-0.85)
# --------------------------------------------------------------------------- #
FOUNDATION_DARK = [
    ("Abyssal Root",       "OBLONG",    "Low-harmonic rumble anchoring the deepest register"),
    ("Tar Pool Bass",      "ODDOSCAR",  "Viscous subharmonic pulse, zero shimmer"),
    ("Cave Resonance",     "OVERDUB",   "Cavernous dub tone, heavy tape saturation"),
    ("Obsidian Pad",       "OPAL",      "Dense granular darkness, no brightness artifacts"),
    ("Silt Floor",         "ONSET",     "Sub-kick foundation with felt-tip attack"),
    ("Void Sustain",       "ODYSSEY",   "Limitless sustain in pure darkness"),
    ("Iron Curtain",       "OVERWORLD", "NES pulse wave pushed into shadow territory"),
    ("Midnight Root",      "ODDFELIX",  "Felix voice locked in lowest harmonic field"),
    ("Anthracite Low",     "OBLONG",    "Coal-black fundamental, no overtone relief"),
    ("Depth Charge Sine",  "OVERDUB",   "Single-cycle sine at extreme low, dub-processed"),
]

FOUNDATION_AGGRESSIVE = [
    ("Diesel Drive",       "OBLONG",    "High-aggression bob drive, dense harmonic grit"),
    ("Compressor Crush",   "ONSET",     "Machine-tight kick/snare with extreme punch"),
    ("Tape Slam",          "OVERDUB",   "Saturated tape overdrive at maximum send"),
    ("Growl Machine",      "ODDOSCAR",  "Oscar vocal growl pushed into full aggression"),
    ("Hammer Pad",         "OPAL",      "Granular impact cloud — every grain hits hard"),
    ("Crunch Core",        "OVERWORLD", "YM2612 FM crunch, maximum distortion register"),
    ("Felix Fury",         "ODDFELIX",  "Felix voice at peak harmonic aggression"),
    ("Anvil Pulse",        "OBLONG",    "Rhythmic anvil-weight pulse, unrelenting density"),
    ("Riot Low",           "ODYSSEY",   "Driven oscillator sweeping aggressive low territory"),
    ("Steam Hammer",       "ONSET",     "Industrial machine beat, maximum punch and density"),
]

FOUNDATION_BRIGHT_SPARSE = [
    ("Crystal Root",       "ODDFELIX",  "Felix in pure crystalline overtone, sparse texture"),
    ("Sine Prism",         "OPAL",      "Single bright grain cluster, maximum air"),
    ("Ice Fundamental",    "OVERWORLD", "NES square wave at high brightness, wide open"),
    ("Glass Tone",         "ODYSSEY",   "Glassy oscillator with minimal density, pure shimmer"),
    ("Daylight Bass",      "OVERDUB",   "Bright dub tone — clean tape, no saturation"),
    ("Quartz Pad",         "OBLONG",    "Sparse bob texture, crystalline upper partials"),
    ("Photon Root",        "ODDOSCAR",  "Oscar voice at bright register, breath wide open"),
    ("Clear Foundation",   "OPAL",      "Granular foundation stripped to luminous core"),
    ("Limpid Sine",        "ONSET",     "Bright pitched percussion foundation, zero mud"),
    ("Sunlit Bottom",      "ODDFELIX",  "Felix fundamental with extreme brightness, low density"),
]

# --------------------------------------------------------------------------- #
# Prism — Dark Dense (brightness 0.05-0.15, density 0.80-0.95, aggression 0.65-0.85)
# --------------------------------------------------------------------------- #
PRISM_DARK_DENSE = [
    ("Black Spectrum",     "OPTIC",    "Refracted darkness — all prism bands collapsed low"),
    ("Dense Prism Black",  "OBLIQUE",  "Oblique angle into lowest spectral mass"),
    ("Origami Shadow",     "ORIGAMI",  "Folded dark layers, maximum density stack"),
    ("Oracle Void",        "ORACLE",   "Signal drift into spectral darkness, dense output"),
    ("NES Crusher",        "OVERWORLD","YM2612 dense crunch, brightness cut to zero"),
    ("Bob Dark Prism",     "OBLONG",   "Oblong in extreme low-brightness dense mode"),
    ("Refract Black",      "OPTIC",    "Full spectral decomposition into shadow register"),
    ("Orbital Dark",       "ORBITAL",  "Orbit phase locked in dense dark spectral field"),
    ("Fold Abyss",         "ORIGAMI",  "Origami fold collapsed into maximum darkness"),
    ("Dense Signal",       "ORACLE",   "Oracle output at peak density, minimal brightness"),
]

PRISM_ULTRA_BRIGHT = [
    ("Diamond Scatter",    "OPTIC",    "Prism scattering into pure high brightness"),
    ("Aurora Spectrum",    "OBLIQUE",  "Oblique angle producing maximum luminance"),
    ("Photon Prism",       "OVERWORLD","SPC700 era at full brightness, wide spectral air"),
    ("Crystal Fold",       "ORIGAMI",  "Paper-light origami surface, brilliant reflections"),
    ("Oracle Flash",       "ORACLE",   "Signal burst at peak brightness, sparse warmth"),
    ("Orbital Noon",       "ORBITAL",  "Orbit at maximum altitude brightness"),
    ("Refract White",      "OPTIC",    "Full-spectrum white-light refraction output"),
    ("Prism Zenith",       "OBLIQUE",  "Extreme high-end prism angle, near-UV brightness"),
    ("Onset Shimmer",      "ONSET",    "Transient shimmer prism — percussion gone spectral"),
    ("Bob Light Prism",    "OBLONG",   "Oblong in ultra-bright sparse prism territory"),
]

PRISM_BRUTAL_MOVEMENT = [
    ("Chaos Refract",      "OPTIC",    "Rapidly cycling prism scatter, maximum movement"),
    ("Origami Storm",      "ORIGAMI",  "Fast-folding chaos, brutal rhythmic density"),
    ("Orbital Fury",       "ORBITAL",  "Orbit at maximum angular velocity, brutal phase"),
    ("Signal Riot",        "ORACLE",   "Oracle signal in maximum drift-aggression mode"),
    ("Overworld Blitz",    "OVERWORLD","Chip-era glitch storm, movement fully unleashed"),
    ("Oblique Assault",    "OBLIQUE",  "High-angle oblique sweep, relentless motion"),
    ("Refract Violence",   "OPTIC",    "Spectral decomposition under maximum aggression"),
    ("Bob Prism Riot",     "OBLONG",   "Oblong in brutal movement prism configuration"),
    ("Onset Chaos",        "ONSET",    "Percussion prism exploding with brutal movement"),
    ("Oracle Surge",       "ORACLE",   "Drift surge at peak aggression and movement"),
]


def build_foundation_presets():
    presets = []

    for name, engine, desc in FOUNDATION_DARK:
        dna = {
            "brightness": rand_range(0.05, 0.15),
            "warmth":     rand_range(0.55, 0.85),
            "movement":   rand_range(0.15, 0.45),
            "density":    rand_range(0.60, 0.85),
            "space":      rand_range(0.30, 0.60),
            "aggression": rand_range(0.10, 0.35),
        }
        presets.append(preset(name, "Foundation", engine, dna,
                               ["foundation", "dark", "extreme", "anchor"], desc))

    for name, engine, desc in FOUNDATION_AGGRESSIVE:
        dna = {
            "brightness": rand_range(0.20, 0.55),
            "warmth":     rand_range(0.40, 0.70),
            "movement":   rand_range(0.40, 0.70),
            "density":    rand_range(0.70, 0.90),
            "space":      rand_range(0.20, 0.50),
            "aggression": rand_range(0.75, 0.92),
        }
        presets.append(preset(name, "Foundation", engine, dna,
                               ["foundation", "aggressive", "extreme", "anchor", "drive"], desc))

    for name, engine, desc in FOUNDATION_BRIGHT_SPARSE:
        dna = {
            "brightness": rand_range(0.80, 0.95),
            "warmth":     rand_range(0.10, 0.35),
            "movement":   rand_range(0.10, 0.40),
            "density":    rand_range(0.05, 0.20),
            "space":      rand_range(0.55, 0.85),
            "aggression": rand_range(0.05, 0.25),
        }
        presets.append(preset(name, "Foundation", engine, dna,
                               ["foundation", "bright", "sparse", "crystalline", "anchor"], desc))

    return presets


def build_prism_presets():
    presets = []

    for name, engine, desc in PRISM_DARK_DENSE:
        dna = {
            "brightness": rand_range(0.05, 0.15),
            "warmth":     rand_range(0.40, 0.65),
            "movement":   rand_range(0.35, 0.65),
            "density":    rand_range(0.80, 0.95),
            "space":      rand_range(0.20, 0.45),
            "aggression": rand_range(0.65, 0.85),
        }
        presets.append(preset(name, "Prism", engine, dna,
                               ["prism", "dark", "dense", "extreme", "anchor"], desc))

    for name, engine, desc in PRISM_ULTRA_BRIGHT:
        dna = {
            "brightness": rand_range(0.88, 0.98),
            "warmth":     rand_range(0.10, 0.25),
            "movement":   rand_range(0.25, 0.60),
            "density":    rand_range(0.20, 0.55),
            "space":      rand_range(0.70, 0.90),
            "aggression": rand_range(0.10, 0.40),
        }
        presets.append(preset(name, "Prism", engine, dna,
                               ["prism", "bright", "ultra", "spectral", "anchor"], desc))

    for name, engine, desc in PRISM_BRUTAL_MOVEMENT:
        dna = {
            "brightness": rand_range(0.30, 0.70),
            "warmth":     rand_range(0.25, 0.55),
            "movement":   rand_range(0.80, 0.95),
            "density":    rand_range(0.55, 0.78),
            "space":      rand_range(0.30, 0.60),
            "aggression": rand_range(0.82, 0.95),
        }
        presets.append(preset(name, "Prism", engine, dna,
                               ["prism", "brutal", "movement", "aggressive", "anchor"], desc))

    return presets


# --------------------------------------------------------------------------- #
# Main
# --------------------------------------------------------------------------- #
def main():
    os.makedirs(FOUNDATION_DIR, exist_ok=True)
    os.makedirs(PRISM_DIR, exist_ok=True)

    print("=" * 60)
    print("FOUNDATION — Diversity Anchor Presets")
    print("=" * 60)
    foundation = build_foundation_presets()
    written_f = skipped_f = 0
    for p in foundation:
        filename = p["name"].replace(" ", "_").replace("/", "-") + ".xometa"
        path = os.path.join(FOUNDATION_DIR, filename)
        if os.path.exists(path):
            print(f"  SKIP (exists): {filename}")
            skipped_f += 1
        else:
            with open(path, "w") as f:
                json.dump(p, f, indent=2)
            print(f"  WROTE: {filename}")
            written_f += 1

    print()
    print("=" * 60)
    print("PRISM — Diversity Anchor Presets")
    print("=" * 60)
    prism = build_prism_presets()
    written_p = skipped_p = 0
    for p in prism:
        filename = p["name"].replace(" ", "_").replace("/", "-") + ".xometa"
        path = os.path.join(PRISM_DIR, filename)
        if os.path.exists(path):
            print(f"  SKIP (exists): {filename}")
            skipped_p += 1
        else:
            with open(path, "w") as f:
                json.dump(p, f, indent=2)
            print(f"  WROTE: {filename}")
            written_p += 1

    print()
    print("=" * 60)
    print("SUMMARY")
    print("=" * 60)
    print(f"Foundation: {written_f} written, {skipped_f} skipped  (30 target)")
    print(f"Prism:      {written_p} written, {skipped_p} skipped  (30 target)")
    print()
    print("DNA zones covered:")
    print("  Foundation-dark:           brightness 0.05-0.15, density 0.60-0.85")
    print("  Foundation-aggressive:     aggression 0.75-0.92, density 0.70-0.90")
    print("  Foundation-bright-sparse:  brightness 0.80-0.95, density 0.05-0.20")
    print("  Prism-dark-dense:          brightness 0.05-0.15, density 0.80-0.95, aggression 0.65-0.85")
    print("  Prism-ultra-bright:        brightness 0.88-0.98, warmth 0.10-0.25,  space 0.70-0.90")
    print("  Prism-brutal-movement:     aggression 0.82-0.95, movement 0.80-0.95, density 0.55-0.78")


if __name__ == "__main__":
    main()
