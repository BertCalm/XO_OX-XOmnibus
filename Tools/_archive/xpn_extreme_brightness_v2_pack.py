#!/usr/bin/env python3
"""
xpn_extreme_brightness_v2_pack.py

Generates 60 extreme-brightness preset stubs for XOlokun:
  - 30 Ultra-Bright (brightness 0.85–1.0) → Presets/XOlokun/Prism/
  - 30 Ultra-Dark  (brightness 0.0–0.15)  → Presets/XOlokun/Aether/

Goal: populate currently-empty extreme brightness zones to fix fleet diversity score.
Fleet analysis shows 85.4% of presets cluster in the 0.55–0.75 midrange.

Run from any directory — resolves paths relative to this script's location.
"""

import json
import os
import pathlib

# ---------------------------------------------------------------------------
# Output directories (relative to repo root)
# ---------------------------------------------------------------------------
SCRIPT_DIR = pathlib.Path(__file__).resolve().parent
REPO_ROOT   = SCRIPT_DIR.parent
PRISM_DIR   = REPO_ROOT / "Presets" / "XOlokun" / "Prism"
AETHER_DIR  = REPO_ROOT / "Presets" / "XOlokun" / "Aether"

# ---------------------------------------------------------------------------
# Engine macro labels — pulled from doctrine docs where known
# ---------------------------------------------------------------------------
MACRO_LABELS = {
    "ODDFELIX":   ["SHIMMER", "SCATTER", "COUPLING", "AIR"],
    "OVERWORLD":  ["ERA", "GLITCH", "COUPLING", "SPACE"],
    "OPTIC":      ["FOCUS", "PRISM", "COUPLING", "DIFFUSE"],
    "OBLONG":     ["BOB", "SNAP", "COUPLING", "ROOM"],
    "ONSET":      ["MACHINE", "PUNCH", "COUPLING", "SPACE"],
    "ORBITAL":    ["ORBIT", "DECAY", "COUPLING", "DRIFT"],
    "ORIGAMI":    ["FOLD", "CREASE", "COUPLING", "UNFOLD"],
    "OSPREY":     ["DIVE", "THERMAL", "COUPLING", "SOAR"],
    "OSTERIA":    ["WARMTH", "SALT", "COUPLING", "SMOKE"],
    "OTTONI":     ["GROW", "BELL", "COUPLING", "HALL"],
    "OHM":        ["COMMUNE", "MEDDLING", "COUPLING", "DRIFT"],
    "ORPHICA":    ["STRINGS", "SCATTER", "COUPLING", "AIR"],
    "ODDOSCAR":   ["WEIGHT", "GRIND", "COUPLING", "CAVE"],
    "ORCA":       ["HUNT", "DIVE", "COUPLING", "DEPTH"],
    "ORACLE":     ["PROPHECY", "VOID", "COUPLING", "ECHO"],
    "OBSIDIAN":   ["EDGE", "DENSITY", "COUPLING", "PRESSURE"],
    "OBSCURA":    ["SHADOW", "BLUR", "COUPLING", "VEIL"],
    "OMBRE":      ["FADE", "DEPTH", "COUPLING", "SHADE"],
    "OCELOT":     ["STALK", "STRIKE", "COUPLING", "JUNGLE"],
    "OUROBOROS":  ["CYCLE", "CONSUME", "COUPLING", "VOID"],
    "OVERDUB":    ["DRIVE", "TAPE", "COUPLING", "SPACE"],
    "ORGANON":    ["LOGIC", "ARGUMENT", "COUPLING", "FORM"],
    "OWLFISH":    ["DEPTH", "FEEDING", "COUPLING", "PRESSURE"],
    "OCEANIC":    ["CURRENT", "PRESSURE", "COUPLING", "ABYSS"],
}

def macro_labels_for(engine):
    return MACRO_LABELS.get(engine, ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"])

# ---------------------------------------------------------------------------
# Preset definitions
# ---------------------------------------------------------------------------
# Each tuple: (name, engine, brightness, warmth, movement, density, space, aggression, description, tags_extra)
ULTRA_BRIGHT = [
    # ODDFELIX — naturally bright shimmer engine
    ("Arctic Shimmer",     "ODDFELIX",  0.97, 0.15, 0.65, 0.30, 0.70, 0.10, "Razor-edge shimmer dispersed across the upper register", ["ice", "shimmer", "airy"]),
    ("Photon Bloom",       "ODDFELIX",  0.92, 0.20, 0.80, 0.25, 0.85, 0.08, "Rapid modulation scatter paints the treble sky", ["bloom", "scatter", "radiant"]),

    # OVERWORLD — NES/chip brightness
    ("Chip Sunrise",       "OVERWORLD", 0.96, 0.10, 0.70, 0.20, 0.60, 0.15, "2A03 pulse wave at dawn — pure retro luminance", ["chip", "8bit", "morning"]),
    ("Crystal Palace",     "OVERWORLD", 0.90, 0.22, 0.50, 0.35, 0.75, 0.12, "YM2612 FM operators locked to a bright resonant peak", ["crystal", "fm", "retro"]),
    ("Neon Overpass",      "OVERWORLD", 0.88, 0.18, 0.60, 0.45, 0.65, 0.20, "Glitch-shredded SNES palette, aggressive treble presence", ["neon", "glitch", "urban"]),

    # OPTIC — light/prism engine
    ("Prism Flare",        "OPTIC",     0.99, 0.08, 0.55, 0.15, 0.90, 0.05, "Diffracted light — pure spectral white", ["prism", "diffraction", "white"]),
    ("Iris Spike",         "OPTIC",     0.93, 0.12, 0.40, 0.20, 0.80, 0.10, "Single focused beam, knife-sharp frequency", ["focus", "beam", "sharp"]),

    # OBLONG
    ("Glass Snap",         "OBLONG",    0.91, 0.25, 0.85, 0.40, 0.55, 0.30, "Bob's snap transient cracked to maximum brilliance", ["snap", "transient", "crack"]),
    ("Mirror Bounce",      "OBLONG",    0.87, 0.30, 0.75, 0.50, 0.60, 0.25, "Rapid room reflections — bright and percussive", ["room", "bounce", "percussive"]),

    # ONSET — drum engine brightness
    ("Hi-Hat Constellation", "ONSET",  0.95, 0.05, 0.90, 0.30, 0.70, 0.22, "Open hat shimmer at the edge of the frequency spectrum", ["hihat", "drum", "metallic"]),
    ("Snap Machine",       "ONSET",    0.89, 0.15, 0.80, 0.55, 0.50, 0.35, "Clap + snap layered for hyper-bright transient impact", ["clap", "snap", "bright-drum"]),

    # ORBITAL
    ("Solar Periapsis",    "ORBITAL",  0.94, 0.10, 0.55, 0.20, 0.85, 0.08, "Orbital decay frozen at closest bright approach", ["space", "orbit", "luminous"]),
    ("Stardust Scatter",   "ORBITAL",  0.86, 0.18, 0.70, 0.25, 0.90, 0.10, "Microdebris trail — shimmering upper partials", ["space", "scatter", "airy"]),

    # ORIGAMI
    ("Paper Crane",        "ORIGAMI",  0.92, 0.28, 0.45, 0.35, 0.65, 0.12, "Fold geometry opens to a bright, thin resonance", ["fold", "paper", "delicate"]),
    ("Scored Light",       "ORIGAMI",  0.88, 0.20, 0.60, 0.30, 0.75, 0.15, "Crease lines become frequency partials — luminous structure", ["crease", "structure", "bright"]),

    # OSPREY
    ("Thermal Ascent",     "OSPREY",   0.90, 0.12, 0.75, 0.20, 0.80, 0.10, "Riding the updraft — open treble soar", ["soar", "air", "bird"]),
    ("Dive Glare",         "OSPREY",   0.95, 0.08, 0.85, 0.15, 0.70, 0.18, "High-velocity descent into blinding treble light", ["dive", "speed", "radiant"]),

    # OSTERIA
    ("Salt Spray",         "OSTERIA",  0.87, 0.35, 0.50, 0.40, 0.65, 0.15, "Coastal brightness — warmth tempered by sea air", ["coastal", "salt", "warm-bright"]),

    # OTTONI
    ("Piccolo Trumpet",    "OTTONI",   0.96, 0.32, 0.60, 0.45, 0.55, 0.28, "Triple-brass GROW macro pushed to piccolo register", ["brass", "trumpet", "high"]),
    ("Bell Tower",         "OTTONI",   0.91, 0.25, 0.40, 0.55, 0.70, 0.20, "Sustained brass bell ring — bright, resonant, tall", ["bell", "resonant", "brass"]),

    # OHM
    ("Commune Frequency",  "OHM",      0.89, 0.30, 0.65, 0.35, 0.75, 0.08, "Hippy Dad's highest harmonic — open, radiant, communal", ["commune", "harmonic", "open"]),

    # ORPHICA
    ("Harp Corona",        "ORPHICA",  0.93, 0.22, 0.70, 0.25, 0.85, 0.10, "Microsound harp at maximum brightness — siphonophore glowing", ["harp", "microsound", "glow"]),
    ("String Ultraviolet", "ORPHICA",  0.97, 0.15, 0.55, 0.20, 0.90, 0.05, "Plucked upper strings beyond the visible spectrum", ["strings", "bright", "ultra"]),

    # Remaining ODDFELIX / ORBITAL fills
    ("White Frequency",    "ODDFELIX", 0.98, 0.05, 0.50, 0.10, 0.95, 0.03, "Flat spectral energy — maximum brightness, minimal character", ["white", "flat", "spectral"]),
    ("Zenith Pulse",       "ORBITAL",  0.85, 0.20, 0.80, 0.30, 0.80, 0.15, "Orbital peak at zenith — highest orbital resonance", ["zenith", "pulse", "peak"]),
    ("Fractal Apex",       "OPTIC",    0.86, 0.12, 0.65, 0.25, 0.88, 0.08, "Self-similar bright structure — recursive upper harmonics", ["fractal", "recursive", "apex"]),
    ("Morning Signal",     "OVERWORLD", 0.93, 0.14, 0.45, 0.18, 0.72, 0.06, "First light transmitted through chip oscillators", ["morning", "signal", "dawn"]),
    ("Crown Glass",        "ORIGAMI",  0.94, 0.16, 0.35, 0.22, 0.82, 0.07, "Optical crown glass fold — pure refracted luminance", ["crown", "optical", "glass"]),
    ("Altitude Tone",      "OSPREY",   0.85, 0.10, 0.60, 0.18, 0.88, 0.05, "Above cloud ceiling — breathless treble presence", ["altitude", "thin", "pure"]),
    ("Platinum Veil",      "OHM",      0.86, 0.12, 0.55, 0.22, 0.82, 0.06, "OHM commune at maximum frequency — a shared platinum radiance", ["commune", "platinum", "radiance"]),
]

ULTRA_DARK = [
    # ODDOSCAR — pure Oscar weight
    ("Tar Frequency",      "ODDOSCAR",  0.03, 0.85, 0.20, 0.80, 0.30, 0.40, "Maximum Oscar density — thick sub matter", ["sub", "dense", "heavy"]),
    ("Silt Bed",           "ODDOSCAR",  0.08, 0.90, 0.15, 0.90, 0.20, 0.25, "Settled sediment — the lowest resonant floor", ["sediment", "low", "settled"]),

    # ORCA — deep hunt
    ("Echolocation Dark",  "ORCA",      0.05, 0.75, 0.35, 0.70, 0.45, 0.55, "Orca pulse in lightless deep — sonar without reflection", ["orca", "sonar", "deep"]),
    ("Midnight Current",   "ORCA",      0.12, 0.80, 0.25, 0.65, 0.55, 0.45, "Cold thermocline current — dark and directional", ["current", "cold", "thermocline"]),
    ("Abyssal Hunt",       "ORCA",      0.02, 0.70, 0.45, 0.75, 0.35, 0.65, "Maximum depth pursuit — bass frequencies only", ["hunt", "abyssal", "predator"]),

    # ORACLE — void prophecy
    ("Void Oracle",        "ORACLE",    0.04, 0.60, 0.30, 0.60, 0.70, 0.20, "Prophecy spoken from absolute darkness", ["void", "prophecy", "oracle"]),
    ("Obsidian Echo",      "ORACLE",    0.10, 0.55, 0.20, 0.50, 0.80, 0.15, "Dark reverb tail — reflection from obsidian walls", ["echo", "obsidian", "reverb"]),

    # OBSIDIAN
    ("Volcanic Glass",     "OBSIDIAN",  0.06, 0.78, 0.10, 0.85, 0.25, 0.50, "Hardest darkness — razor density, zero treble", ["volcanic", "glass", "dense"]),
    ("Magma Slow",         "OBSIDIAN",  0.09, 0.88, 0.08, 0.95, 0.15, 0.35, "Molten rock at minimum movement — near-static dark mass", ["magma", "slow", "molten"]),

    # OBSCURA
    ("Camera Obscura",     "OBSCURA",   0.07, 0.65, 0.25, 0.55, 0.60, 0.18, "Inverted light — dark projection from a pinhole", ["obscura", "invert", "shadow"]),
    ("Shadow Architecture","OBSCURA",   0.13, 0.70, 0.18, 0.65, 0.50, 0.22, "Structural shadow — form defined only by absence", ["shadow", "structure", "negative"]),

    # OMBRE
    ("Deep Gradient",      "OMBRE",     0.05, 0.82, 0.15, 0.60, 0.55, 0.12, "Smooth descent into total darkness — no hard edge", ["gradient", "fade", "deep"]),
    ("Ink Dissolve",       "OMBRE",     0.11, 0.75, 0.20, 0.55, 0.65, 0.10, "Black ink expanding in still water", ["ink", "dissolve", "water"]),

    # OCELOT
    ("Night Stalk",        "OCELOT",    0.04, 0.72, 0.50, 0.65, 0.40, 0.60, "Jungle darkness — ocelot movement before the strike", ["jungle", "stalk", "predator"]),
    ("Underbrush",         "OCELOT",    0.09, 0.80, 0.35, 0.70, 0.30, 0.45, "Dense foliage swallowing all treble light", ["jungle", "dense", "cover"]),

    # OUROBOROS
    ("Endless Descent",    "OUROBOROS", 0.03, 0.68, 0.40, 0.75, 0.45, 0.30, "Self-consuming loop — cycle into infinite dark", ["cycle", "descent", "infinite"]),
    ("Serpent Root",       "OUROBOROS", 0.07, 0.78, 0.22, 0.80, 0.35, 0.35, "Tail-devouring tone at the lowest root frequency", ["serpent", "root", "cycle"]),

    # OVERDUB
    ("Tape Blackout",      "OVERDUB",   0.06, 0.85, 0.18, 0.70, 0.50, 0.42, "Saturated tape — all HF eaten by magnetic oxide", ["tape", "saturation", "dub"]),
    ("Dub Basement",       "OVERDUB",   0.12, 0.90, 0.30, 0.65, 0.60, 0.38, "Sub-bass dub pressure — room below the room", ["dub", "basement", "sub"]),

    # ORGANON
    ("Dark Syllogism",     "ORGANON",   0.08, 0.60, 0.12, 0.55, 0.70, 0.20, "Logical form in darkness — argument without light", ["logic", "formal", "dark"]),
    ("Premise Zero",       "ORGANON",   0.05, 0.65, 0.08, 0.60, 0.65, 0.15, "First premise: silence. Conclusion: sub-bass only.", ["argument", "formal", "minimal"]),

    # OWLFISH
    ("Hadal Pressure",     "OWLFISH",   0.02, 0.88, 0.28, 0.90, 0.25, 0.55, "Maximum depth pressure — owlfish at 8000m", ["hadal", "pressure", "depth"]),
    ("Bioluminescence Off","OWLFISH",   0.10, 0.82, 0.35, 0.80, 0.40, 0.40, "All bioluminescence extinguished — only weight remains", ["dark", "deep", "owlfish"]),

    # OCEANIC
    ("Trench Signal",      "OCEANIC",   0.04, 0.92, 0.20, 0.85, 0.30, 0.30, "Signal from the Mariana Trench — pure abyssal current", ["trench", "abyss", "signal"]),
    ("Cold Seep",          "OCEANIC",   0.09, 0.86, 0.12, 0.88, 0.20, 0.22, "Methane seep at trench floor — cold, dense, dark", ["seep", "methane", "abyssal"]),

    # Fill: additional dark ORCA / ODDOSCAR / OBSIDIAN
    ("Below Zero",         "ODDOSCAR",  0.06, 0.92, 0.10, 0.95, 0.15, 0.30, "Below freezing — matter compressed to maximum Oscar density", ["zero", "dense", "sub"]),
    ("Basalt Ground",      "OBSIDIAN",  0.12, 0.82, 0.05, 0.88, 0.22, 0.28, "Basalt ocean floor — nothing reflects, everything absorbs", ["basalt", "ground", "absorb"]),
    ("Penumbra Tone",      "OBSCURA",   0.08, 0.68, 0.22, 0.58, 0.68, 0.18, "Half-shadow region — almost dark, never light", ["penumbra", "shadow", "half"]),
    ("Deep Dub",           "OVERDUB",   0.03, 0.93, 0.25, 0.72, 0.55, 0.48, "Dub at maximum tape saturation — treble is myth", ["dub", "tape", "saturation"]),
    ("Roots System",       "OUROBOROS", 0.11, 0.76, 0.30, 0.78, 0.38, 0.28, "Underground root network — exchange in total darkness", ["roots", "underground", "system"]),
]

# ---------------------------------------------------------------------------
# Build preset dict
# ---------------------------------------------------------------------------
def build_preset(name, engine, brightness, warmth, movement, density, space, aggression,
                 mood, description, tags_extra):
    labels = macro_labels_for(engine)
    base_tags = ["prism", "bright", "extreme"] if mood == "Prism" else ["aether", "dark", "extreme"]
    return {
        "schema_version": 1,
        "name": name,
        "mood": mood,
        "sonic_dna": {
            "brightness": round(brightness, 3),
            "warmth":     round(warmth,     3),
            "aggression": round(aggression, 3),
            "movement":   round(movement,   3),
            "density":    round(density,    3),
            "space":      round(space,      3),
        },
        "engines": [engine],
        "author": "XO_OX",
        "version": "1.0.0",
        "description": description,
        "tags": base_tags + tags_extra,
        "macroLabels": labels,
        "couplingIntensity": "None",
        "dna": {
            "brightness": round(brightness, 3),
            "warmth":     round(warmth,     3),
            "movement":   round(movement,   3),
            "density":    round(density,    3),
            "space":      round(space,      3),
            "aggression": round(aggression, 3),
        },
        "parameters": {},
        "coupling": {"pairs": []},
    }

def filename_for(name):
    safe = name.replace(" ", "_").replace("/", "-").replace("'", "")
    return f"{safe}.xometa"

# ---------------------------------------------------------------------------
# Write presets
# ---------------------------------------------------------------------------
def write_presets():
    PRISM_DIR.mkdir(parents=True, exist_ok=True)
    AETHER_DIR.mkdir(parents=True, exist_ok=True)

    bright_written = 0
    for row in ULTRA_BRIGHT:
        name, engine, brightness, warmth, movement, density, space, aggression, desc, tags_extra = row
        preset = build_preset(name, engine, brightness, warmth, movement, density, space, aggression,
                              "Prism", desc, tags_extra)
        out_path = PRISM_DIR / filename_for(name)
        with open(out_path, "w", encoding="utf-8") as f:
            json.dump(preset, f, indent=2)
        bright_written += 1
        print(f"  [Prism]  {name:30s}  brightness={brightness:.2f}  engine={engine}")

    dark_written = 0
    for row in ULTRA_DARK:
        name, engine, brightness, warmth, movement, density, space, aggression, desc, tags_extra = row
        preset = build_preset(name, engine, brightness, warmth, movement, density, space, aggression,
                              "Aether", desc, tags_extra)
        out_path = AETHER_DIR / filename_for(name)
        with open(out_path, "w", encoding="utf-8") as f:
            json.dump(preset, f, indent=2)
        dark_written += 1
        print(f"  [Aether] {name:30s}  brightness={brightness:.2f}  engine={engine}")

    print(f"\n✓ {bright_written} Prism presets written to  {PRISM_DIR}")
    print(f"✓ {dark_written} Aether presets written to {AETHER_DIR}")
    print(f"✓ {bright_written + dark_written} total extreme-brightness presets generated")

    # Sanity checks
    bright_range = [(r[0], r[2]) for r in ULTRA_BRIGHT if not (0.85 <= r[2] <= 1.0)]
    dark_range   = [(r[0], r[2]) for r in ULTRA_DARK   if not (0.0  <= r[2] <= 0.15)]
    if bright_range:
        print(f"WARNING: out-of-range Prism presets: {bright_range}")
    if dark_range:
        print(f"WARNING: out-of-range Aether presets: {dark_range}")

if __name__ == "__main__":
    print("XPN Extreme Brightness v2 Pack — generating 60 extreme presets...")
    print(f"  Prism  → {PRISM_DIR}")
    print(f"  Aether → {AETHER_DIR}\n")
    write_presets()
