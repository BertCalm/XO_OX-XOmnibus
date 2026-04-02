#!/usr/bin/env python3
"""
xpn_obscura_ocelot_oblique_coverage_pack.py

Generates ~66 Entangled mood .xometa presets covering new coupling pairs for
the visual/photographic trio: OBSCURA, OCELOT, OBLIQUE.

OBSCURA: daguerreotype silver, photographic, stiff strings — #8A9BA8
OCELOT:  ocelot tawny, biome-shifting                      — #C5832B
OBLIQUE: prism violet, RTJ×Funk×Tame Impala, bouncing prism — #BF40FF

11 pairs × 6 presets = 66 total
"""

import json
import os
import re

OUTPUT_DIR = os.path.join(
    os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
    "Presets", "XOceanus", "Entangled"
)

ENGINE_DEFAULTS = {
    "OBSCURA": dict(macro_character=0.35, macro_movement=0.25, macro_coupling=0.55, macro_space=0.60),
    "OCELOT":  dict(macro_character=0.65, macro_movement=0.70, macro_coupling=0.55, macro_space=0.45),
    "OBLIQUE": dict(macro_character=0.80, macro_movement=0.75, macro_coupling=0.65, macro_space=0.55),
    "OPTIC":   dict(macro_character=0.50, macro_movement=0.45, macro_coupling=0.60, macro_space=0.50),
    "ORBITAL": dict(macro_character=0.40, macro_movement=0.55, macro_coupling=0.50, macro_space=0.70),
    "OCEANIC": dict(macro_character=0.30, macro_movement=0.35, macro_coupling=0.45, macro_space=0.80),
}

PRESETS = [
    # OBSCURA x OCELOT
    {"pair":("OBSCURA","OCELOT"),"name":"Silver Prowl","coupling_type":"TIMBRE_BLEND","coupling_amount":0.65,
     "dna":{"brightness":0.40,"warmth":0.70,"movement":0.60,"density":0.50,"space":0.55,"aggression":0.88},
     "tags":["entangled","coupling","visual","photographic","feral"]},
    {"pair":("OBSCURA","OCELOT"),"name":"Daguerreotype Spotted","coupling_type":"SPECTRAL_SHIFT","coupling_amount":0.50,
     "dna":{"brightness":0.12,"warmth":0.65,"movement":0.45,"density":0.60,"space":0.70,"aggression":0.40},
     "tags":["entangled","coupling","visual","photographic","tawny"]},
    {"pair":("OBSCURA","OCELOT"),"name":"Exposure Biome","coupling_type":"HARMONIC_LOCK","coupling_amount":0.72,
     "dna":{"brightness":0.55,"warmth":0.85,"movement":0.65,"density":0.40,"space":0.50,"aggression":0.30},
     "tags":["entangled","coupling","visual","warm"]},
    {"pair":("OBSCURA","OCELOT"),"name":"Tarnished Savanna","coupling_type":"ENVELOPE_MIRROR","coupling_amount":0.58,
     "dna":{"brightness":0.35,"warmth":0.75,"movement":0.88,"density":0.55,"space":0.45,"aggression":0.50},
     "tags":["entangled","coupling","visual","movement"]},
    {"pair":("OBSCURA","OCELOT"),"name":"Collodion Stalk","coupling_type":"MODULATION_SHARE","coupling_amount":0.45,
     "dna":{"brightness":0.25,"warmth":0.60,"movement":0.55,"density":0.87,"space":0.40,"aggression":0.60},
     "tags":["entangled","coupling","visual","dense"]},
    {"pair":("OBSCURA","OCELOT"),"name":"Fixed Lens Hunter","coupling_type":"TEXTURE_MORPH","coupling_amount":0.80,
     "dna":{"brightness":0.60,"warmth":0.55,"movement":0.70,"density":0.45,"space":0.14,"aggression":0.75},
     "tags":["entangled","coupling","visual","intimate"]},

    # OBSCURA x OBLIQUE
    {"pair":("OBSCURA","OBLIQUE"),"name":"Prism Plate","coupling_type":"SPECTRAL_SHIFT","coupling_amount":0.70,
     "dna":{"brightness":0.86,"warmth":0.35,"movement":0.65,"density":0.45,"space":0.60,"aggression":0.50},
     "tags":["entangled","coupling","visual","prismatic"]},
    {"pair":("OBSCURA","OBLIQUE"),"name":"Violet Exposure","coupling_type":"HARMONIC_LOCK","coupling_amount":0.55,
     "dna":{"brightness":0.75,"warmth":0.25,"movement":0.80,"density":0.40,"space":0.65,"aggression":0.13},
     "tags":["entangled","coupling","visual","delicate"]},
    {"pair":("OBSCURA","OBLIQUE"),"name":"Refracted Silver","coupling_type":"PITCH_WARP","coupling_amount":0.62,
     "dna":{"brightness":0.90,"warmth":0.30,"movement":0.55,"density":0.50,"space":0.70,"aggression":0.35},
     "tags":["entangled","coupling","visual","bright"]},
    {"pair":("OBSCURA","OBLIQUE"),"name":"Darkroom Bounce","coupling_type":"RHYTHM_SYNC","coupling_amount":0.75,
     "dna":{"brightness":0.55,"warmth":0.20,"movement":0.88,"density":0.60,"space":0.50,"aggression":0.70},
     "tags":["entangled","coupling","visual","rhythmic"]},
    {"pair":("OBSCURA","OBLIQUE"),"name":"Stiff String Funk","coupling_type":"MODULATION_SHARE","coupling_amount":0.68,
     "dna":{"brightness":0.70,"warmth":0.40,"movement":0.85,"density":0.55,"space":0.35,"aggression":0.80},
     "tags":["entangled","coupling","visual","funk"]},
    {"pair":("OBSCURA","OBLIQUE"),"name":"Collodion Prism","coupling_type":"SPACE_BLEND","coupling_amount":0.48,
     "dna":{"brightness":0.80,"warmth":0.28,"movement":0.45,"density":0.35,"space":0.87,"aggression":0.25},
     "tags":["entangled","coupling","visual","spacious"]},

    # OCELOT x OBLIQUE
    {"pair":("OCELOT","OBLIQUE"),"name":"Tawny Prism","coupling_type":"TIMBRE_BLEND","coupling_amount":0.72,
     "dna":{"brightness":0.85,"warmth":0.75,"movement":0.80,"density":0.45,"space":0.50,"aggression":0.60},
     "tags":["entangled","coupling","visual","vivid"]},
    {"pair":("OCELOT","OBLIQUE"),"name":"Biome Bounce","coupling_type":"RHYTHM_SYNC","coupling_amount":0.80,
     "dna":{"brightness":0.75,"warmth":0.65,"movement":0.90,"density":0.55,"space":0.40,"aggression":0.70},
     "tags":["entangled","coupling","visual","rhythmic","funk"]},
    {"pair":("OCELOT","OBLIQUE"),"name":"Spotted Spectrum","coupling_type":"SPECTRAL_SHIFT","coupling_amount":0.65,
     "dna":{"brightness":0.88,"warmth":0.55,"movement":0.65,"density":0.40,"space":0.60,"aggression":0.45},
     "tags":["entangled","coupling","visual","prismatic"]},
    {"pair":("OCELOT","OBLIQUE"),"name":"Feral Impala","coupling_type":"ENVELOPE_MIRROR","coupling_amount":0.58,
     "dna":{"brightness":0.70,"warmth":0.60,"movement":0.85,"density":0.30,"space":0.55,"aggression":0.87},
     "tags":["entangled","coupling","visual","aggressive","tame-impala"]},
    {"pair":("OCELOT","OBLIQUE"),"name":"Violet Savanna","coupling_type":"TEXTURE_MORPH","coupling_amount":0.52,
     "dna":{"brightness":0.65,"warmth":0.70,"movement":0.50,"density":0.60,"space":0.75,"aggression":0.13},
     "tags":["entangled","coupling","visual","lush"]},
    {"pair":("OCELOT","OBLIQUE"),"name":"RTJ Leopard","coupling_type":"PITCH_WARP","coupling_amount":0.77,
     "dna":{"brightness":0.78,"warmth":0.50,"movement":0.88,"density":0.65,"space":0.35,"aggression":0.85},
     "tags":["entangled","coupling","visual","rtj","aggressive"]},

    # OBSCURA x OPTIC
    {"pair":("OBSCURA","OPTIC"),"name":"Aperture Study","coupling_type":"TIMBRE_BLEND","coupling_amount":0.60,
     "dna":{"brightness":0.55,"warmth":0.35,"movement":0.40,"density":0.45,"space":0.70,"aggression":0.12},
     "tags":["entangled","coupling","visual","optical"]},
    {"pair":("OBSCURA","OPTIC"),"name":"Silver Lens","coupling_type":"SPECTRAL_SHIFT","coupling_amount":0.68,
     "dna":{"brightness":0.87,"warmth":0.30,"movement":0.35,"density":0.50,"space":0.65,"aggression":0.25},
     "tags":["entangled","coupling","visual","bright"]},
    {"pair":("OBSCURA","OPTIC"),"name":"Long Exposure","coupling_type":"SPACE_BLEND","coupling_amount":0.75,
     "dna":{"brightness":0.40,"warmth":0.25,"movement":0.14,"density":0.40,"space":0.88,"aggression":0.20},
     "tags":["entangled","coupling","visual","spacious","slow"]},
    {"pair":("OBSCURA","OPTIC"),"name":"Grain Refraction","coupling_type":"TEXTURE_MORPH","coupling_amount":0.55,
     "dna":{"brightness":0.45,"warmth":0.40,"movement":0.50,"density":0.85,"space":0.55,"aggression":0.30},
     "tags":["entangled","coupling","visual","textured"]},
    {"pair":("OBSCURA","OPTIC"),"name":"Wet Plate Optic","coupling_type":"HARMONIC_LOCK","coupling_amount":0.50,
     "dna":{"brightness":0.30,"warmth":0.45,"movement":0.55,"density":0.60,"space":0.65,"aggression":0.10},
     "tags":["entangled","coupling","visual","subtle"]},
    {"pair":("OBSCURA","OPTIC"),"name":"Focus Pull","coupling_type":"MODULATION_SHARE","coupling_amount":0.82,
     "dna":{"brightness":0.65,"warmth":0.35,"movement":0.70,"density":0.45,"space":0.55,"aggression":0.88},
     "tags":["entangled","coupling","visual","dynamic"]},

    # OBSCURA x ORBITAL
    {"pair":("OBSCURA","ORBITAL"),"name":"Celestial Plate","coupling_type":"SPACE_BLEND","coupling_amount":0.78,
     "dna":{"brightness":0.40,"warmth":0.25,"movement":0.35,"density":0.30,"space":0.90,"aggression":0.15},
     "tags":["entangled","coupling","visual","cosmic","spacious"]},
    {"pair":("OBSCURA","ORBITAL"),"name":"Orbital Daguerreotype","coupling_type":"TIMBRE_BLEND","coupling_amount":0.55,
     "dna":{"brightness":0.35,"warmth":0.30,"movement":0.45,"density":0.50,"space":0.85,"aggression":0.12},
     "tags":["entangled","coupling","visual","cosmic"]},
    {"pair":("OBSCURA","ORBITAL"),"name":"Stiff String Orbit","coupling_type":"HARMONIC_LOCK","coupling_amount":0.65,
     "dna":{"brightness":0.50,"warmth":0.35,"movement":0.55,"density":0.60,"space":0.85,"aggression":0.20},
     "tags":["entangled","coupling","visual","harmonic"]},
    {"pair":("OBSCURA","ORBITAL"),"name":"Silver Perihelion","coupling_type":"PITCH_WARP","coupling_amount":0.70,
     "dna":{"brightness":0.60,"warmth":0.20,"movement":0.65,"density":0.40,"space":0.85,"aggression":0.30},
     "tags":["entangled","coupling","visual","orbital"]},
    {"pair":("OBSCURA","ORBITAL"),"name":"Dark Matter Photo","coupling_type":"SPECTRAL_SHIFT","coupling_amount":0.48,
     "dna":{"brightness":0.13,"warmth":0.28,"movement":0.40,"density":0.65,"space":0.88,"aggression":0.25},
     "tags":["entangled","coupling","visual","dark","deep"]},
    {"pair":("OBSCURA","ORBITAL"),"name":"Exposure Arc","coupling_type":"ENVELOPE_MIRROR","coupling_amount":0.62,
     "dna":{"brightness":0.45,"warmth":0.30,"movement":0.75,"density":0.35,"space":0.87,"aggression":0.18},
     "tags":["entangled","coupling","visual","movement"]},

    # OCELOT x OPTIC
    {"pair":("OCELOT","OPTIC"),"name":"Amber Optic","coupling_type":"TIMBRE_BLEND","coupling_amount":0.67,
     "dna":{"brightness":0.75,"warmth":0.85,"movement":0.55,"density":0.45,"space":0.50,"aggression":0.40},
     "tags":["entangled","coupling","visual","warm","optical"]},
    {"pair":("OCELOT","OPTIC"),"name":"Tawny Lens","coupling_type":"SPECTRAL_SHIFT","coupling_amount":0.55,
     "dna":{"brightness":0.80,"warmth":0.70,"movement":0.65,"density":0.50,"space":0.40,"aggression":0.13},
     "tags":["entangled","coupling","visual","bright","warm"]},
    {"pair":("OCELOT","OPTIC"),"name":"Biome Refraction","coupling_type":"TEXTURE_MORPH","coupling_amount":0.72,
     "dna":{"brightness":0.65,"warmth":0.75,"movement":0.70,"density":0.88,"space":0.40,"aggression":0.50},
     "tags":["entangled","coupling","visual","dense","textured"]},
    {"pair":("OCELOT","OPTIC"),"name":"Spotted Focus","coupling_type":"HARMONIC_LOCK","coupling_amount":0.60,
     "dna":{"brightness":0.86,"warmth":0.60,"movement":0.50,"density":0.55,"space":0.60,"aggression":0.35},
     "tags":["entangled","coupling","visual"]},
    {"pair":("OCELOT","OPTIC"),"name":"Feline Aperture","coupling_type":"MODULATION_SHARE","coupling_amount":0.78,
     "dna":{"brightness":0.88,"warmth":0.65,"movement":0.75,"density":0.40,"space":0.45,"aggression":0.55},
     "tags":["entangled","coupling","visual","vivid"]},
    {"pair":("OCELOT","OPTIC"),"name":"Chromatic Predator","coupling_type":"PITCH_WARP","coupling_amount":0.68,
     "dna":{"brightness":0.78,"warmth":0.55,"movement":0.85,"density":0.50,"space":0.35,"aggression":0.80},
     "tags":["entangled","coupling","visual","aggressive"]},

    # OCELOT x ORBITAL
    {"pair":("OCELOT","ORBITAL"),"name":"Tawny Orbit","coupling_type":"SPACE_BLEND","coupling_amount":0.65,
     "dna":{"brightness":0.60,"warmth":0.80,"movement":0.55,"density":0.35,"space":0.87,"aggression":0.30},
     "tags":["entangled","coupling","visual","cosmic","warm"]},
    {"pair":("OCELOT","ORBITAL"),"name":"Amber Perihelion","coupling_type":"TIMBRE_BLEND","coupling_amount":0.72,
     "dna":{"brightness":0.70,"warmth":0.85,"movement":0.65,"density":0.40,"space":0.70,"aggression":0.25},
     "tags":["entangled","coupling","visual","warm"]},
    {"pair":("OCELOT","ORBITAL"),"name":"Biome Ellipse","coupling_type":"HARMONIC_LOCK","coupling_amount":0.58,
     "dna":{"brightness":0.55,"warmth":0.75,"movement":0.88,"density":0.50,"space":0.65,"aggression":0.40},
     "tags":["entangled","coupling","visual","movement"]},
    {"pair":("OCELOT","ORBITAL"),"name":"Spotted Apogee","coupling_type":"RHYTHM_SYNC","coupling_amount":0.80,
     "dna":{"brightness":0.65,"warmth":0.70,"movement":0.90,"density":0.45,"space":0.55,"aggression":0.60},
     "tags":["entangled","coupling","visual","rhythmic"]},
    {"pair":("OCELOT","ORBITAL"),"name":"Equatorial Stalk","coupling_type":"ENVELOPE_MIRROR","coupling_amount":0.62,
     "dna":{"brightness":0.75,"warmth":0.65,"movement":0.60,"density":0.55,"space":0.60,"aggression":0.88},
     "tags":["entangled","coupling","visual","feral"]},
    {"pair":("OCELOT","ORBITAL"),"name":"Resonant Savanna","coupling_type":"RESONANCE_CHAIN","coupling_amount":0.55,
     "dna":{"brightness":0.50,"warmth":0.78,"movement":0.50,"density":0.60,"space":0.80,"aggression":0.10},
     "tags":["entangled","coupling","visual","resonant"]},

    # OBLIQUE x OPTIC
    {"pair":("OBLIQUE","OPTIC"),"name":"Prism Optic","coupling_type":"SPECTRAL_SHIFT","coupling_amount":0.82,
     "dna":{"brightness":0.90,"warmth":0.30,"movement":0.70,"density":0.40,"space":0.60,"aggression":0.50},
     "tags":["entangled","coupling","visual","prismatic","bright"]},
    {"pair":("OBLIQUE","OPTIC"),"name":"Violet Aperture","coupling_type":"TIMBRE_BLEND","coupling_amount":0.68,
     "dna":{"brightness":0.82,"warmth":0.25,"movement":0.65,"density":0.45,"space":0.65,"aggression":0.12},
     "tags":["entangled","coupling","visual","delicate"]},
    {"pair":("OBLIQUE","OPTIC"),"name":"Funk Focus","coupling_type":"RHYTHM_SYNC","coupling_amount":0.75,
     "dna":{"brightness":0.70,"warmth":0.45,"movement":0.88,"density":0.60,"space":0.40,"aggression":0.78},
     "tags":["entangled","coupling","visual","funk","rhythmic"]},
    {"pair":("OBLIQUE","OPTIC"),"name":"Bouncing Lens","coupling_type":"PITCH_WARP","coupling_amount":0.70,
     "dna":{"brightness":0.85,"warmth":0.35,"movement":0.80,"density":0.35,"space":0.55,"aggression":0.65},
     "tags":["entangled","coupling","visual","dynamic"]},
    {"pair":("OBLIQUE","OPTIC"),"name":"Spectral Skew","coupling_type":"TEXTURE_MORPH","coupling_amount":0.58,
     "dna":{"brightness":0.75,"warmth":0.14,"movement":0.55,"density":0.70,"space":0.50,"aggression":0.40},
     "tags":["entangled","coupling","visual","textured"]},
    {"pair":("OBLIQUE","OPTIC"),"name":"Impala Optic","coupling_type":"MODULATION_SHARE","coupling_amount":0.85,
     "dna":{"brightness":0.88,"warmth":0.40,"movement":0.85,"density":0.50,"space":0.45,"aggression":0.55},
     "tags":["entangled","coupling","visual","tame-impala"]},

    # OBLIQUE x ORBITAL
    {"pair":("OBLIQUE","ORBITAL"),"name":"Violet Orbit","coupling_type":"SPACE_BLEND","coupling_amount":0.78,
     "dna":{"brightness":0.80,"warmth":0.20,"movement":0.60,"density":0.35,"space":0.90,"aggression":0.25},
     "tags":["entangled","coupling","visual","cosmic","violet"]},
    {"pair":("OBLIQUE","ORBITAL"),"name":"Prism Perihelion","coupling_type":"SPECTRAL_SHIFT","coupling_amount":0.72,
     "dna":{"brightness":0.88,"warmth":0.25,"movement":0.70,"density":0.40,"space":0.80,"aggression":0.30},
     "tags":["entangled","coupling","visual","prismatic","orbital"]},
    {"pair":("OBLIQUE","ORBITAL"),"name":"Bouncing Ellipse","coupling_type":"RHYTHM_SYNC","coupling_amount":0.80,
     "dna":{"brightness":0.75,"warmth":0.30,"movement":0.90,"density":0.45,"space":0.65,"aggression":0.70},
     "tags":["entangled","coupling","visual","rhythmic"]},
    {"pair":("OBLIQUE","ORBITAL"),"name":"RTJ Apogee","coupling_type":"PITCH_WARP","coupling_amount":0.75,
     "dna":{"brightness":0.70,"warmth":0.35,"movement":0.85,"density":0.55,"space":0.55,"aggression":0.88},
     "tags":["entangled","coupling","visual","rtj","aggressive"]},
    {"pair":("OBLIQUE","ORBITAL"),"name":"Funk Arc","coupling_type":"HARMONIC_LOCK","coupling_amount":0.62,
     "dna":{"brightness":0.65,"warmth":0.45,"movement":0.80,"density":0.60,"space":0.50,"aggression":0.86},
     "tags":["entangled","coupling","visual","funk"]},
    {"pair":("OBLIQUE","ORBITAL"),"name":"Oblique Resonance","coupling_type":"RESONANCE_CHAIN","coupling_amount":0.55,
     "dna":{"brightness":0.72,"warmth":0.30,"movement":0.50,"density":0.40,"space":0.87,"aggression":0.14},
     "tags":["entangled","coupling","visual","resonant","spacious"]},

    # OBSCURA x OCEANIC
    {"pair":("OBSCURA","OCEANIC"),"name":"Submerged Plate","coupling_type":"SPACE_BLEND","coupling_amount":0.70,
     "dna":{"brightness":0.20,"warmth":0.35,"movement":0.30,"density":0.55,"space":0.90,"aggression":0.10},
     "tags":["entangled","coupling","visual","deep","aquatic"]},
    {"pair":("OBSCURA","OCEANIC"),"name":"Silver Current","coupling_type":"TEXTURE_MORPH","coupling_amount":0.62,
     "dna":{"brightness":0.30,"warmth":0.30,"movement":0.65,"density":0.45,"space":0.85,"aggression":0.12},
     "tags":["entangled","coupling","visual","flowing"]},
    {"pair":("OBSCURA","OCEANIC"),"name":"Daguerreotype Depth","coupling_type":"HARMONIC_LOCK","coupling_amount":0.55,
     "dna":{"brightness":0.13,"warmth":0.40,"movement":0.40,"density":0.60,"space":0.88,"aggression":0.15},
     "tags":["entangled","coupling","visual","dark","deep"]},
    {"pair":("OBSCURA","OCEANIC"),"name":"Tidal Exposure","coupling_type":"ENVELOPE_MIRROR","coupling_amount":0.75,
     "dna":{"brightness":0.35,"warmth":0.28,"movement":0.75,"density":0.50,"space":0.85,"aggression":0.20},
     "tags":["entangled","coupling","visual","tidal"]},
    {"pair":("OBSCURA","OCEANIC"),"name":"Stiff String Kelp","coupling_type":"MODULATION_SHARE","coupling_amount":0.58,
     "dna":{"brightness":0.25,"warmth":0.45,"movement":0.55,"density":0.70,"space":0.75,"aggression":0.10},
     "tags":["entangled","coupling","visual","aquatic","textured"]},
    {"pair":("OBSCURA","OCEANIC"),"name":"Fixed Lens Abyss","coupling_type":"RESONANCE_CHAIN","coupling_amount":0.85,
     "dna":{"brightness":0.10,"warmth":0.30,"movement":0.35,"density":0.65,"space":0.88,"aggression":0.08},
     "tags":["entangled","coupling","visual","abyssal","dark"]},

    # OCELOT x OCEANIC
    {"pair":("OCELOT","OCEANIC"),"name":"Tawny Tide","coupling_type":"TIMBRE_BLEND","coupling_amount":0.65,
     "dna":{"brightness":0.50,"warmth":0.80,"movement":0.65,"density":0.40,"space":0.85,"aggression":0.25},
     "tags":["entangled","coupling","visual","aquatic","warm"]},
    {"pair":("OCELOT","OCEANIC"),"name":"Spotted Current","coupling_type":"TEXTURE_MORPH","coupling_amount":0.72,
     "dna":{"brightness":0.60,"warmth":0.70,"movement":0.80,"density":0.55,"space":0.75,"aggression":0.12},
     "tags":["entangled","coupling","visual","flowing"]},
    {"pair":("OCELOT","OCEANIC"),"name":"Amber Depth","coupling_type":"SPACE_BLEND","coupling_amount":0.78,
     "dna":{"brightness":0.45,"warmth":0.85,"movement":0.45,"density":0.50,"space":0.88,"aggression":0.15},
     "tags":["entangled","coupling","visual","deep","warm"]},
    {"pair":("OCELOT","OCEANIC"),"name":"Biome Brine","coupling_type":"HARMONIC_LOCK","coupling_amount":0.60,
     "dna":{"brightness":0.55,"warmth":0.75,"movement":0.55,"density":0.65,"space":0.86,"aggression":0.30},
     "tags":["entangled","coupling","visual","aquatic"]},
    {"pair":("OCELOT","OCEANIC"),"name":"Feline Pelagic","coupling_type":"ENVELOPE_MIRROR","coupling_amount":0.55,
     "dna":{"brightness":0.65,"warmth":0.65,"movement":0.88,"density":0.45,"space":0.65,"aggression":0.50},
     "tags":["entangled","coupling","visual","pelagic"]},
    {"pair":("OCELOT","OCEANIC"),"name":"Tawny Thermocline","coupling_type":"RESONANCE_CHAIN","coupling_amount":0.68,
     "dna":{"brightness":0.40,"warmth":0.88,"movement":0.50,"density":0.60,"space":0.80,"aggression":0.10},
     "tags":["entangled","coupling","visual","aquatic","resonant"]},
]

def to_snake(name):
    s = name.lower()
    s = re.sub(r"[^a-z0-9]+", "_", s)
    return s.strip("_")

def clamp(v):
    return max(0.0, min(1.0, round(float(v), 3)))

def build_preset(spec):
    engine_a, engine_b = spec["pair"]
    defs_a = ENGINE_DEFAULTS.get(engine_a, {})
    defs_b = ENGINE_DEFAULTS.get(engine_b, {})
    variation = (list(spec["dna"].values())[0] - 0.5) * 0.2
    params_a = {k: clamp(v + variation * 0.5) for k, v in defs_a.items()}
    params_b = {k: clamp(v - variation * 0.5) for k, v in defs_b.items()}
    amount = clamp(spec["coupling_amount"] + variation * 0.1)
    global_char = clamp((params_a["macro_character"] + params_b["macro_character"]) / 2)
    global_move = clamp((params_a["macro_movement"]  + params_b["macro_movement"])  / 2)
    global_coup = clamp(amount)
    global_spac = clamp((params_a["macro_space"]     + params_b["macro_space"])     / 2)
    return {
        "name":    spec["name"],
        "version": "1.0",
        "mood":    "Entangled",
        "engines": [engine_a, engine_b],
        "parameters": {engine_a: params_a, engine_b: params_b},
        "coupling": {"type": spec["coupling_type"], "source": engine_a, "target": engine_b, "amount": amount},
        "dna": {k: clamp(v) for k, v in spec["dna"].items()},
        "macros": {"CHARACTER": global_char, "MOVEMENT": global_move, "COUPLING": global_coup, "SPACE": global_spac},
        "tags": spec.get("tags", ["entangled","coupling","visual"]),
    }

def validate_dna(dna, name):
    extremes = [v for v in dna.values() if v <= 0.15 or v >= 0.85]
    if not extremes:
        raise ValueError(f"Preset '{name}' has no extreme DNA dimension (need <=0.15 or >=0.85)")

def main():
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    written = skipped = 0
    errors = []
    for spec in PRESETS:
        try:
            preset = build_preset(spec)
            validate_dna(preset["dna"], preset["name"])
            filename = to_snake(preset["name"]) + ".xometa"
            filepath = os.path.join(OUTPUT_DIR, filename)
            with open(filepath, "w", encoding="utf-8") as fh:
                json.dump(preset, fh, indent=2)
                fh.write("\n")
            written += 1
            print(f"  [OK] {filename}")
        except Exception as exc:
            errors.append((spec.get("name", "?"), str(exc)))
            skipped += 1
    print(f"\n{'='*60}")
    print(f"OBSCURA / OCELOT / OBLIQUE Coverage Pack")
    print(f"  Written : {written}")
    print(f"  Skipped : {skipped}")
    if errors:
        print("  ERRORS:")
        for name, msg in errors:
            print(f"    - {name}: {msg}")
    print(f"  Output  : {OUTPUT_DIR}")
    print(f"{'='*60}")

if __name__ == "__main__":
    main()
