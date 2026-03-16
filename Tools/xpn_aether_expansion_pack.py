#!/usr/bin/env python3
"""
xpn_aether_expansion_pack.py
Generate 60 Aether mood .xometa preset stubs and save them to Presets/XOmnibus/Aether/
"""

import json
import os
import re

OUTPUT_DIR = os.path.join(os.path.dirname(__file__), "..", "Presets", "XOmnibus", "Aether")

PRESETS = [
    # ── TRANSCENDENT (15) ─────────────────────────────────────────────────────
    {
        "name": "Celestial Threshold",
        "mood": "Aether",
        "engines": ["OPAL"],
        "parameters": {
            "OPAL": {"macro_character": 0.3, "macro_movement": 0.65, "macro_coupling": 0.2, "macro_space": 0.92}
        },
        "dna": {"brightness": 0.6, "warmth": 0.4, "movement": 0.65, "density": 0.15, "space": 0.92, "aggression": 0.05},
        "macros": {"CHARACTER": 0.3, "MOVEMENT": 0.65, "COUPLING": 0.2, "SPACE": 0.92},
        "tags": ["aether", "transcendent", "ambient"],
    },
    {
        "name": "Ascending Light",
        "mood": "Aether",
        "engines": ["ODYSSEY"],
        "parameters": {
            "ODYSSEY": {"macro_character": 0.35, "macro_movement": 0.7, "macro_coupling": 0.25, "macro_space": 0.95}
        },
        "dna": {"brightness": 0.7, "warmth": 0.35, "movement": 0.7, "density": 0.1, "space": 0.95, "aggression": 0.05},
        "macros": {"CHARACTER": 0.35, "MOVEMENT": 0.7, "COUPLING": 0.25, "SPACE": 0.95},
        "tags": ["aether", "transcendent", "evolving"],
    },
    {
        "name": "Beyond the Veil",
        "mood": "Aether",
        "engines": ["ORACLE"],
        "parameters": {
            "ORACLE": {"macro_character": 0.4, "macro_movement": 0.6, "macro_coupling": 0.3, "macro_space": 0.9}
        },
        "dna": {"brightness": 0.55, "warmth": 0.3, "movement": 0.6, "density": 0.2, "space": 0.9, "aggression": 0.08},
        "macros": {"CHARACTER": 0.4, "MOVEMENT": 0.6, "COUPLING": 0.3, "SPACE": 0.9},
        "tags": ["aether", "transcendent", "otherworldly"],
    },
    {
        "name": "Infinite Horizon",
        "mood": "Aether",
        "engines": ["ORGANON"],
        "parameters": {
            "ORGANON": {"macro_character": 0.25, "macro_movement": 0.75, "macro_coupling": 0.15, "macro_space": 0.98}
        },
        "dna": {"brightness": 0.65, "warmth": 0.25, "movement": 0.75, "density": 0.08, "space": 0.98, "aggression": 0.04},
        "macros": {"CHARACTER": 0.25, "MOVEMENT": 0.75, "COUPLING": 0.15, "SPACE": 0.98},
        "tags": ["aether", "transcendent", "ambient"],
    },
    {
        "name": "Pure Elevation",
        "mood": "Aether",
        "engines": ["OUROBOROS"],
        "parameters": {
            "OUROBOROS": {"macro_character": 0.3, "macro_movement": 0.8, "macro_coupling": 0.2, "macro_space": 0.88}
        },
        "dna": {"brightness": 0.6, "warmth": 0.4, "movement": 0.8, "density": 0.15, "space": 0.88, "aggression": 0.1},
        "macros": {"CHARACTER": 0.3, "MOVEMENT": 0.8, "COUPLING": 0.2, "SPACE": 0.88},
        "tags": ["aether", "transcendent", "evolving"],
    },
    {
        "name": "Starfield Breath",
        "mood": "Aether",
        "engines": ["ORBITAL"],
        "parameters": {
            "ORBITAL": {"macro_character": 0.2, "macro_movement": 0.65, "macro_coupling": 0.1, "macro_space": 0.96}
        },
        "dna": {"brightness": 0.75, "warmth": 0.2, "movement": 0.65, "density": 0.1, "space": 0.96, "aggression": 0.03},
        "macros": {"CHARACTER": 0.2, "MOVEMENT": 0.65, "COUPLING": 0.1, "SPACE": 0.96},
        "tags": ["aether", "transcendent", "ambient"],
    },
    {
        "name": "Void of Knowing",
        "mood": "Aether",
        "engines": ["OBSCURA"],
        "parameters": {
            "OBSCURA": {"macro_character": 0.45, "macro_movement": 0.62, "macro_coupling": 0.3, "macro_space": 0.87}
        },
        "dna": {"brightness": 0.45, "warmth": 0.35, "movement": 0.62, "density": 0.2, "space": 0.87, "aggression": 0.12},
        "macros": {"CHARACTER": 0.45, "MOVEMENT": 0.62, "COUPLING": 0.3, "SPACE": 0.87},
        "tags": ["aether", "transcendent", "otherworldly"],
    },
    {
        "name": "Luminous Passage",
        "mood": "Aether",
        "engines": ["ORIGAMI"],
        "parameters": {
            "ORIGAMI": {"macro_character": 0.35, "macro_movement": 0.68, "macro_coupling": 0.2, "macro_space": 0.91}
        },
        "dna": {"brightness": 0.68, "warmth": 0.38, "movement": 0.68, "density": 0.14, "space": 0.91, "aggression": 0.06},
        "macros": {"CHARACTER": 0.35, "MOVEMENT": 0.68, "COUPLING": 0.2, "SPACE": 0.91},
        "tags": ["aether", "transcendent", "ambient"],
    },
    {
        "name": "Ether Bloom",
        "mood": "Aether",
        "engines": ["OHM"],
        "parameters": {
            "OHM": {"macro_character": 0.28, "macro_movement": 0.72, "macro_coupling": 0.18, "macro_space": 0.94}
        },
        "dna": {"brightness": 0.62, "warmth": 0.42, "movement": 0.72, "density": 0.12, "space": 0.94, "aggression": 0.05},
        "macros": {"CHARACTER": 0.28, "MOVEMENT": 0.72, "COUPLING": 0.18, "SPACE": 0.94},
        "tags": ["aether", "transcendent", "evolving"],
    },
    {
        "name": "Heaven's Undertow",
        "mood": "Aether",
        "engines": ["OCEANIC"],
        "parameters": {
            "OCEANIC": {"macro_character": 0.38, "macro_movement": 0.78, "macro_coupling": 0.22, "macro_space": 0.89}
        },
        "dna": {"brightness": 0.5, "warmth": 0.5, "movement": 0.78, "density": 0.18, "space": 0.89, "aggression": 0.09},
        "macros": {"CHARACTER": 0.38, "MOVEMENT": 0.78, "COUPLING": 0.22, "SPACE": 0.89},
        "tags": ["aether", "transcendent", "ambient"],
    },
    {
        "name": "Temple of Air",
        "mood": "Aether",
        "engines": ["ORPHICA"],
        "parameters": {
            "ORPHICA": {"macro_character": 0.22, "macro_movement": 0.63, "macro_coupling": 0.12, "macro_space": 0.97}
        },
        "dna": {"brightness": 0.72, "warmth": 0.28, "movement": 0.63, "density": 0.09, "space": 0.97, "aggression": 0.04},
        "macros": {"CHARACTER": 0.22, "MOVEMENT": 0.63, "COUPLING": 0.12, "SPACE": 0.97},
        "tags": ["aether", "transcendent", "ambient"],
    },
    {
        "name": "Prismatic Soul",
        "mood": "Aether",
        "engines": ["OPTIC"],
        "parameters": {
            "OPTIC": {"macro_character": 0.4, "macro_movement": 0.7, "macro_coupling": 0.25, "macro_space": 0.9}
        },
        "dna": {"brightness": 0.78, "warmth": 0.3, "movement": 0.7, "density": 0.12, "space": 0.9, "aggression": 0.07},
        "macros": {"CHARACTER": 0.4, "MOVEMENT": 0.7, "COUPLING": 0.25, "SPACE": 0.9},
        "tags": ["aether", "transcendent", "otherworldly"],
    },
    {
        "name": "Altar of Silence",
        "mood": "Aether",
        "engines": ["OBBLIGATO"],
        "parameters": {
            "OBBLIGATO": {"macro_character": 0.3, "macro_movement": 0.6, "macro_coupling": 0.2, "macro_space": 0.93}
        },
        "dna": {"brightness": 0.55, "warmth": 0.45, "movement": 0.6, "density": 0.14, "space": 0.93, "aggression": 0.06},
        "macros": {"CHARACTER": 0.3, "MOVEMENT": 0.6, "COUPLING": 0.2, "SPACE": 0.93},
        "tags": ["aether", "transcendent", "ambient"],
    },
    {
        "name": "Soft Transcendence",
        "mood": "Aether",
        "engines": ["OVERDUB"],
        "parameters": {
            "OVERDUB": {"macro_character": 0.25, "macro_movement": 0.82, "macro_coupling": 0.15, "macro_space": 0.86}
        },
        "dna": {"brightness": 0.48, "warmth": 0.55, "movement": 0.82, "density": 0.2, "space": 0.86, "aggression": 0.1},
        "macros": {"CHARACTER": 0.25, "MOVEMENT": 0.82, "COUPLING": 0.15, "SPACE": 0.86},
        "tags": ["aether", "transcendent", "evolving"],
    },
    {
        "name": "Open Frequency",
        "mood": "Aether",
        "engines": ["OMBRE"],
        "parameters": {
            "OMBRE": {"macro_character": 0.35, "macro_movement": 0.66, "macro_coupling": 0.2, "macro_space": 0.92}
        },
        "dna": {"brightness": 0.6, "warmth": 0.38, "movement": 0.66, "density": 0.13, "space": 0.92, "aggression": 0.07},
        "macros": {"CHARACTER": 0.35, "MOVEMENT": 0.66, "COUPLING": 0.2, "SPACE": 0.92},
        "tags": ["aether", "transcendent", "ambient"],
    },

    # ── DARK AETHER (15) ──────────────────────────────────────────────────────
    {
        "name": "Shadow Ascent",
        "mood": "Aether",
        "engines": ["OBSIDIAN"],
        "parameters": {
            "OBSIDIAN": {"macro_character": 0.6, "macro_movement": 0.55, "macro_coupling": 0.35, "macro_space": 0.82}
        },
        "dna": {"brightness": 0.2, "warmth": 0.45, "movement": 0.55, "density": 0.25, "space": 0.82, "aggression": 0.18},
        "macros": {"CHARACTER": 0.6, "MOVEMENT": 0.55, "COUPLING": 0.35, "SPACE": 0.82},
        "tags": ["aether", "dark-aether", "ambient"],
    },
    {
        "name": "Midnight Expanse",
        "mood": "Aether",
        "engines": ["OBSCURA"],
        "parameters": {
            "OBSCURA": {"macro_character": 0.65, "macro_movement": 0.5, "macro_coupling": 0.4, "macro_space": 0.88}
        },
        "dna": {"brightness": 0.15, "warmth": 0.5, "movement": 0.5, "density": 0.22, "space": 0.88, "aggression": 0.2},
        "macros": {"CHARACTER": 0.65, "MOVEMENT": 0.5, "COUPLING": 0.4, "SPACE": 0.88},
        "tags": ["aether", "dark-aether", "otherworldly"],
    },
    {
        "name": "Void Resonance",
        "mood": "Aether",
        "engines": ["OUROBOROS"],
        "parameters": {
            "OUROBOROS": {"macro_character": 0.7, "macro_movement": 0.6, "macro_coupling": 0.45, "macro_space": 0.85}
        },
        "dna": {"brightness": 0.18, "warmth": 0.55, "movement": 0.6, "density": 0.28, "space": 0.85, "aggression": 0.22},
        "macros": {"CHARACTER": 0.7, "MOVEMENT": 0.6, "COUPLING": 0.45, "SPACE": 0.85},
        "tags": ["aether", "dark-aether", "evolving"],
    },
    {
        "name": "Eclipse Chamber",
        "mood": "Aether",
        "engines": ["OBLONG"],
        "parameters": {
            "OBLONG": {"macro_character": 0.55, "macro_movement": 0.52, "macro_coupling": 0.3, "macro_space": 0.9}
        },
        "dna": {"brightness": 0.22, "warmth": 0.42, "movement": 0.52, "density": 0.2, "space": 0.9, "aggression": 0.15},
        "macros": {"CHARACTER": 0.55, "MOVEMENT": 0.52, "COUPLING": 0.3, "SPACE": 0.9},
        "tags": ["aether", "dark-aether", "ambient"],
    },
    {
        "name": "Abyssal Reverie",
        "mood": "Aether",
        "engines": ["OCEANIC"],
        "parameters": {
            "OCEANIC": {"macro_character": 0.6, "macro_movement": 0.58, "macro_coupling": 0.38, "macro_space": 0.78}
        },
        "dna": {"brightness": 0.12, "warmth": 0.58, "movement": 0.58, "density": 0.3, "space": 0.78, "aggression": 0.25},
        "macros": {"CHARACTER": 0.6, "MOVEMENT": 0.58, "COUPLING": 0.38, "SPACE": 0.78},
        "tags": ["aether", "dark-aether", "ambient"],
    },
    {
        "name": "Phantom Drift",
        "mood": "Aether",
        "engines": ["ORACLE"],
        "parameters": {
            "ORACLE": {"macro_character": 0.5, "macro_movement": 0.65, "macro_coupling": 0.35, "macro_space": 0.83}
        },
        "dna": {"brightness": 0.25, "warmth": 0.48, "movement": 0.65, "density": 0.24, "space": 0.83, "aggression": 0.17},
        "macros": {"CHARACTER": 0.5, "MOVEMENT": 0.65, "COUPLING": 0.35, "SPACE": 0.83},
        "tags": ["aether", "dark-aether", "otherworldly"],
    },
    {
        "name": "Iron Fog",
        "mood": "Aether",
        "engines": ["OBLIQUE"],
        "parameters": {
            "OBLIQUE": {"macro_character": 0.62, "macro_movement": 0.54, "macro_coupling": 0.4, "macro_space": 0.8}
        },
        "dna": {"brightness": 0.2, "warmth": 0.52, "movement": 0.54, "density": 0.32, "space": 0.8, "aggression": 0.2},
        "macros": {"CHARACTER": 0.62, "MOVEMENT": 0.54, "COUPLING": 0.4, "SPACE": 0.8},
        "tags": ["aether", "dark-aether", "ambient"],
    },
    {
        "name": "Dark Bloom",
        "mood": "Aether",
        "engines": ["OSPREY"],
        "parameters": {
            "OSPREY": {"macro_character": 0.58, "macro_movement": 0.62, "macro_coupling": 0.36, "macro_space": 0.85}
        },
        "dna": {"brightness": 0.28, "warmth": 0.44, "movement": 0.62, "density": 0.22, "space": 0.85, "aggression": 0.16},
        "macros": {"CHARACTER": 0.58, "MOVEMENT": 0.62, "COUPLING": 0.36, "SPACE": 0.85},
        "tags": ["aether", "dark-aether", "evolving"],
    },
    {
        "name": "Nocturne Space",
        "mood": "Aether",
        "engines": ["ORCA"],
        "parameters": {
            "ORCA": {"macro_character": 0.65, "macro_movement": 0.56, "macro_coupling": 0.42, "macro_space": 0.88}
        },
        "dna": {"brightness": 0.16, "warmth": 0.54, "movement": 0.56, "density": 0.26, "space": 0.88, "aggression": 0.22},
        "macros": {"CHARACTER": 0.65, "MOVEMENT": 0.56, "COUPLING": 0.42, "SPACE": 0.88},
        "tags": ["aether", "dark-aether", "ambient"],
    },
    {
        "name": "Sullen Ether",
        "mood": "Aether",
        "engines": ["ODDOSCAR"],
        "parameters": {
            "ODDOSCAR": {"macro_character": 0.7, "macro_movement": 0.5, "macro_coupling": 0.45, "macro_space": 0.82}
        },
        "dna": {"brightness": 0.14, "warmth": 0.56, "movement": 0.5, "density": 0.28, "space": 0.82, "aggression": 0.24},
        "macros": {"CHARACTER": 0.7, "MOVEMENT": 0.5, "COUPLING": 0.45, "SPACE": 0.82},
        "tags": ["aether", "dark-aether", "otherworldly"],
    },
    {
        "name": "Obsidian Tide",
        "mood": "Aether",
        "engines": ["OBSIDIAN", "OCEANIC"],
        "parameters": {
            "OBSIDIAN": {"macro_character": 0.65, "macro_movement": 0.55, "macro_coupling": 0.5, "macro_space": 0.85},
            "OCEANIC": {"macro_character": 0.5, "macro_movement": 0.6, "macro_coupling": 0.5, "macro_space": 0.8},
        },
        "dna": {"brightness": 0.18, "warmth": 0.5, "movement": 0.57, "density": 0.28, "space": 0.82, "aggression": 0.2},
        "macros": {"CHARACTER": 0.58, "MOVEMENT": 0.57, "COUPLING": 0.5, "SPACE": 0.82},
        "tags": ["aether", "dark-aether", "entangled"],
    },
    {
        "name": "Deep Current",
        "mood": "Aether",
        "engines": ["OVERLAP"],
        "parameters": {
            "OVERLAP": {"macro_character": 0.55, "macro_movement": 0.6, "macro_coupling": 0.38, "macro_space": 0.84}
        },
        "dna": {"brightness": 0.24, "warmth": 0.46, "movement": 0.6, "density": 0.24, "space": 0.84, "aggression": 0.18},
        "macros": {"CHARACTER": 0.55, "MOVEMENT": 0.6, "COUPLING": 0.38, "SPACE": 0.84},
        "tags": ["aether", "dark-aether", "evolving"],
    },
    {
        "name": "Buried Light",
        "mood": "Aether",
        "engines": ["OBESE"],
        "parameters": {
            "OBESE": {"macro_character": 0.6, "macro_movement": 0.53, "macro_coupling": 0.4, "macro_space": 0.76}
        },
        "dna": {"brightness": 0.22, "warmth": 0.6, "movement": 0.53, "density": 0.3, "space": 0.76, "aggression": 0.22},
        "macros": {"CHARACTER": 0.6, "MOVEMENT": 0.53, "COUPLING": 0.4, "SPACE": 0.76},
        "tags": ["aether", "dark-aether", "ambient"],
    },
    {
        "name": "Slow Decay",
        "mood": "Aether",
        "engines": ["OVERDUB"],
        "parameters": {
            "OVERDUB": {"macro_character": 0.5, "macro_movement": 0.58, "macro_coupling": 0.35, "macro_space": 0.87}
        },
        "dna": {"brightness": 0.19, "warmth": 0.55, "movement": 0.58, "density": 0.26, "space": 0.87, "aggression": 0.18},
        "macros": {"CHARACTER": 0.5, "MOVEMENT": 0.58, "COUPLING": 0.35, "SPACE": 0.87},
        "tags": ["aether", "dark-aether", "ambient"],
    },
    {
        "name": "Ashen Horizon",
        "mood": "Aether",
        "engines": ["OUTWIT"],
        "parameters": {
            "OUTWIT": {"macro_character": 0.58, "macro_movement": 0.62, "macro_coupling": 0.42, "macro_space": 0.88}
        },
        "dna": {"brightness": 0.25, "warmth": 0.5, "movement": 0.62, "density": 0.25, "space": 0.88, "aggression": 0.2},
        "macros": {"CHARACTER": 0.58, "MOVEMENT": 0.62, "COUPLING": 0.42, "SPACE": 0.88},
        "tags": ["aether", "dark-aether", "otherworldly"],
    },

    # ── SHIFTING AETHER (15) ──────────────────────────────────────────────────
    {
        "name": "Liquid Heaven",
        "mood": "Aether",
        "engines": ["OPAL"],
        "parameters": {
            "OPAL": {"macro_character": 0.4, "macro_movement": 0.88, "macro_coupling": 0.3, "macro_space": 0.82}
        },
        "dna": {"brightness": 0.5, "warmth": 0.4, "movement": 0.88, "density": 0.18, "space": 0.82, "aggression": 0.1},
        "macros": {"CHARACTER": 0.4, "MOVEMENT": 0.88, "COUPLING": 0.3, "SPACE": 0.82},
        "tags": ["aether", "shifting-aether", "evolving"],
    },
    {
        "name": "Morphic Pulse",
        "mood": "Aether",
        "engines": ["OVERWORLD"],
        "parameters": {
            "OVERWORLD": {"macro_character": 0.45, "macro_movement": 0.92, "macro_coupling": 0.28, "macro_space": 0.78}
        },
        "dna": {"brightness": 0.48, "warmth": 0.38, "movement": 0.92, "density": 0.22, "space": 0.78, "aggression": 0.12},
        "macros": {"CHARACTER": 0.45, "MOVEMENT": 0.92, "COUPLING": 0.28, "SPACE": 0.78},
        "tags": ["aether", "shifting-aether", "evolving"],
    },
    {
        "name": "Tide of Becoming",
        "mood": "Aether",
        "engines": ["OCEANIC"],
        "parameters": {
            "OCEANIC": {"macro_character": 0.35, "macro_movement": 0.85, "macro_coupling": 0.22, "macro_space": 0.88}
        },
        "dna": {"brightness": 0.42, "warmth": 0.48, "movement": 0.85, "density": 0.2, "space": 0.88, "aggression": 0.1},
        "macros": {"CHARACTER": 0.35, "MOVEMENT": 0.85, "COUPLING": 0.22, "SPACE": 0.88},
        "tags": ["aether", "shifting-aether", "ambient"],
    },
    {
        "name": "Phase Migration",
        "mood": "Aether",
        "engines": ["ORBITAL"],
        "parameters": {
            "ORBITAL": {"macro_character": 0.3, "macro_movement": 0.9, "macro_coupling": 0.18, "macro_space": 0.85}
        },
        "dna": {"brightness": 0.55, "warmth": 0.3, "movement": 0.9, "density": 0.14, "space": 0.85, "aggression": 0.07},
        "macros": {"CHARACTER": 0.3, "MOVEMENT": 0.9, "COUPLING": 0.18, "SPACE": 0.85},
        "tags": ["aether", "shifting-aether", "evolving"],
    },
    {
        "name": "Slow Unraveling",
        "mood": "Aether",
        "engines": ["ORIGAMI"],
        "parameters": {
            "ORIGAMI": {"macro_character": 0.4, "macro_movement": 0.78, "macro_coupling": 0.25, "macro_space": 0.84}
        },
        "dna": {"brightness": 0.52, "warmth": 0.42, "movement": 0.78, "density": 0.2, "space": 0.84, "aggression": 0.12},
        "macros": {"CHARACTER": 0.4, "MOVEMENT": 0.78, "COUPLING": 0.25, "SPACE": 0.84},
        "tags": ["aether", "shifting-aether", "evolving"],
    },
    {
        "name": "Continuous Opening",
        "mood": "Aether",
        "engines": ["ORGANON"],
        "parameters": {
            "ORGANON": {"macro_character": 0.28, "macro_movement": 0.82, "macro_coupling": 0.15, "macro_space": 0.9}
        },
        "dna": {"brightness": 0.58, "warmth": 0.28, "movement": 0.82, "density": 0.12, "space": 0.9, "aggression": 0.06},
        "macros": {"CHARACTER": 0.28, "MOVEMENT": 0.82, "COUPLING": 0.15, "SPACE": 0.9},
        "tags": ["aether", "shifting-aether", "ambient"],
    },
    {
        "name": "Breathing Cosmos",
        "mood": "Aether",
        "engines": ["OUROBOROS"],
        "parameters": {
            "OUROBOROS": {"macro_character": 0.35, "macro_movement": 0.95, "macro_coupling": 0.2, "macro_space": 0.75}
        },
        "dna": {"brightness": 0.45, "warmth": 0.45, "movement": 0.95, "density": 0.25, "space": 0.75, "aggression": 0.14},
        "macros": {"CHARACTER": 0.35, "MOVEMENT": 0.95, "COUPLING": 0.2, "SPACE": 0.75},
        "tags": ["aether", "shifting-aether", "evolving"],
    },
    {
        "name": "Wandering Signal",
        "mood": "Aether",
        "engines": ["OPTIC"],
        "parameters": {
            "OPTIC": {"macro_character": 0.42, "macro_movement": 0.8, "macro_coupling": 0.28, "macro_space": 0.82}
        },
        "dna": {"brightness": 0.62, "warmth": 0.32, "movement": 0.8, "density": 0.16, "space": 0.82, "aggression": 0.09},
        "macros": {"CHARACTER": 0.42, "MOVEMENT": 0.8, "COUPLING": 0.28, "SPACE": 0.82},
        "tags": ["aether", "shifting-aether", "otherworldly"],
    },
    {
        "name": "Form and Formless",
        "mood": "Aether",
        "engines": ["OBBLIGATO", "ORPHICA"],
        "parameters": {
            "OBBLIGATO": {"macro_character": 0.38, "macro_movement": 0.85, "macro_coupling": 0.5, "macro_space": 0.82},
            "ORPHICA": {"macro_character": 0.25, "macro_movement": 0.78, "macro_coupling": 0.5, "macro_space": 0.88},
        },
        "dna": {"brightness": 0.55, "warmth": 0.38, "movement": 0.82, "density": 0.14, "space": 0.85, "aggression": 0.07},
        "macros": {"CHARACTER": 0.32, "MOVEMENT": 0.82, "COUPLING": 0.5, "SPACE": 0.85},
        "tags": ["aether", "shifting-aether", "entangled"],
    },
    {
        "name": "Turbulent Grace",
        "mood": "Aether",
        "engines": ["OTTONI"],
        "parameters": {
            "OTTONI": {"macro_character": 0.45, "macro_movement": 0.88, "macro_coupling": 0.3, "macro_space": 0.78}
        },
        "dna": {"brightness": 0.5, "warmth": 0.42, "movement": 0.88, "density": 0.22, "space": 0.78, "aggression": 0.15},
        "macros": {"CHARACTER": 0.45, "MOVEMENT": 0.88, "COUPLING": 0.3, "SPACE": 0.78},
        "tags": ["aether", "shifting-aether", "evolving"],
    },
    {
        "name": "Dissolving Shore",
        "mood": "Aether",
        "engines": ["OHM"],
        "parameters": {
            "OHM": {"macro_character": 0.3, "macro_movement": 0.76, "macro_coupling": 0.2, "macro_space": 0.86}
        },
        "dna": {"brightness": 0.48, "warmth": 0.44, "movement": 0.76, "density": 0.18, "space": 0.86, "aggression": 0.1},
        "macros": {"CHARACTER": 0.3, "MOVEMENT": 0.76, "COUPLING": 0.2, "SPACE": 0.86},
        "tags": ["aether", "shifting-aether", "ambient"],
    },
    {
        "name": "Flux Sublime",
        "mood": "Aether",
        "engines": ["ODDFELIX"],
        "parameters": {
            "ODDFELIX": {"macro_character": 0.38, "macro_movement": 0.84, "macro_coupling": 0.25, "macro_space": 0.84}
        },
        "dna": {"brightness": 0.56, "warmth": 0.36, "movement": 0.84, "density": 0.16, "space": 0.84, "aggression": 0.09},
        "macros": {"CHARACTER": 0.38, "MOVEMENT": 0.84, "COUPLING": 0.25, "SPACE": 0.84},
        "tags": ["aether", "shifting-aether", "evolving"],
    },
    {
        "name": "River of Glass",
        "mood": "Aether",
        "engines": ["OWLFISH"],
        "parameters": {
            "OWLFISH": {"macro_character": 0.32, "macro_movement": 0.79, "macro_coupling": 0.2, "macro_space": 0.87}
        },
        "dna": {"brightness": 0.6, "warmth": 0.35, "movement": 0.79, "density": 0.15, "space": 0.87, "aggression": 0.08},
        "macros": {"CHARACTER": 0.32, "MOVEMENT": 0.79, "COUPLING": 0.2, "SPACE": 0.87},
        "tags": ["aether", "shifting-aether", "ambient"],
    },
    {
        "name": "Unstill Waters",
        "mood": "Aether",
        "engines": ["OLE"],
        "parameters": {
            "OLE": {"macro_character": 0.44, "macro_movement": 0.93, "macro_coupling": 0.3, "macro_space": 0.72}
        },
        "dna": {"brightness": 0.52, "warmth": 0.46, "movement": 0.93, "density": 0.24, "space": 0.72, "aggression": 0.14},
        "macros": {"CHARACTER": 0.44, "MOVEMENT": 0.93, "COUPLING": 0.3, "SPACE": 0.72},
        "tags": ["aether", "shifting-aether", "evolving"],
    },
    {
        "name": "Perpetual Opening",
        "mood": "Aether",
        "engines": ["OVERLAP"],
        "parameters": {
            "OVERLAP": {"macro_character": 0.35, "macro_movement": 0.87, "macro_coupling": 0.22, "macro_space": 0.83}
        },
        "dna": {"brightness": 0.54, "warmth": 0.38, "movement": 0.87, "density": 0.18, "space": 0.83, "aggression": 0.1},
        "macros": {"CHARACTER": 0.35, "MOVEMENT": 0.87, "COUPLING": 0.22, "SPACE": 0.83},
        "tags": ["aether", "shifting-aether", "evolving"],
    },

    # ── CRYSTALLINE (15) ──────────────────────────────────────────────────────
    {
        "name": "Frozen Cathedral",
        "mood": "Aether",
        "engines": ["OPTIC"],
        "parameters": {
            "OPTIC": {"macro_character": 0.2, "macro_movement": 0.5, "macro_coupling": 0.1, "macro_space": 0.95}
        },
        "dna": {"brightness": 0.85, "warmth": 0.15, "movement": 0.5, "density": 0.1, "space": 0.95, "aggression": 0.04},
        "macros": {"CHARACTER": 0.2, "MOVEMENT": 0.5, "COUPLING": 0.1, "SPACE": 0.95},
        "tags": ["aether", "crystalline", "ambient"],
    },
    {
        "name": "Glacial Harp",
        "mood": "Aether",
        "engines": ["ORPHICA"],
        "parameters": {
            "ORPHICA": {"macro_character": 0.18, "macro_movement": 0.48, "macro_coupling": 0.1, "macro_space": 0.98}
        },
        "dna": {"brightness": 0.88, "warmth": 0.12, "movement": 0.48, "density": 0.08, "space": 0.98, "aggression": 0.03},
        "macros": {"CHARACTER": 0.18, "MOVEMENT": 0.48, "COUPLING": 0.1, "SPACE": 0.98},
        "tags": ["aether", "crystalline", "ambient"],
    },
    {
        "name": "Ice Light",
        "mood": "Aether",
        "engines": ["ORIGAMI"],
        "parameters": {
            "ORIGAMI": {"macro_character": 0.22, "macro_movement": 0.55, "macro_coupling": 0.12, "macro_space": 0.92}
        },
        "dna": {"brightness": 0.82, "warmth": 0.18, "movement": 0.55, "density": 0.12, "space": 0.92, "aggression": 0.05},
        "macros": {"CHARACTER": 0.22, "MOVEMENT": 0.55, "COUPLING": 0.12, "SPACE": 0.92},
        "tags": ["aether", "crystalline", "otherworldly"],
    },
    {
        "name": "Quartz Reverie",
        "mood": "Aether",
        "engines": ["ORBITAL"],
        "parameters": {
            "ORBITAL": {"macro_character": 0.15, "macro_movement": 0.52, "macro_coupling": 0.08, "macro_space": 0.96}
        },
        "dna": {"brightness": 0.9, "warmth": 0.1, "movement": 0.52, "density": 0.09, "space": 0.96, "aggression": 0.03},
        "macros": {"CHARACTER": 0.15, "MOVEMENT": 0.52, "COUPLING": 0.08, "SPACE": 0.96},
        "tags": ["aether", "crystalline", "ambient"],
    },
    {
        "name": "Faceted Silence",
        "mood": "Aether",
        "engines": ["OCELOT"],
        "parameters": {
            "OCELOT": {"macro_character": 0.2, "macro_movement": 0.45, "macro_coupling": 0.1, "macro_space": 0.94}
        },
        "dna": {"brightness": 0.78, "warmth": 0.2, "movement": 0.45, "density": 0.1, "space": 0.94, "aggression": 0.04},
        "macros": {"CHARACTER": 0.2, "MOVEMENT": 0.45, "COUPLING": 0.1, "SPACE": 0.94},
        "tags": ["aether", "crystalline", "ambient"],
    },
    {
        "name": "Diamond Rain",
        "mood": "Aether",
        "engines": ["ODYSSEY"],
        "parameters": {
            "ODYSSEY": {"macro_character": 0.25, "macro_movement": 0.6, "macro_coupling": 0.15, "macro_space": 0.9}
        },
        "dna": {"brightness": 0.84, "warmth": 0.16, "movement": 0.6, "density": 0.11, "space": 0.9, "aggression": 0.05},
        "macros": {"CHARACTER": 0.25, "MOVEMENT": 0.6, "COUPLING": 0.15, "SPACE": 0.9},
        "tags": ["aether", "crystalline", "evolving"],
    },
    {
        "name": "Glass Nebula",
        "mood": "Aether",
        "engines": ["ORACLE"],
        "parameters": {
            "ORACLE": {"macro_character": 0.18, "macro_movement": 0.5, "macro_coupling": 0.1, "macro_space": 0.97}
        },
        "dna": {"brightness": 0.86, "warmth": 0.14, "movement": 0.5, "density": 0.09, "space": 0.97, "aggression": 0.04},
        "macros": {"CHARACTER": 0.18, "MOVEMENT": 0.5, "COUPLING": 0.1, "SPACE": 0.97},
        "tags": ["aether", "crystalline", "otherworldly"],
    },
    {
        "name": "Prism Rain",
        "mood": "Aether",
        "engines": ["OSTERIA"],
        "parameters": {
            "OSTERIA": {"macro_character": 0.22, "macro_movement": 0.55, "macro_coupling": 0.12, "macro_space": 0.93}
        },
        "dna": {"brightness": 0.8, "warmth": 0.22, "movement": 0.55, "density": 0.13, "space": 0.93, "aggression": 0.06},
        "macros": {"CHARACTER": 0.22, "MOVEMENT": 0.55, "COUPLING": 0.12, "SPACE": 0.93},
        "tags": ["aether", "crystalline", "ambient"],
    },
    {
        "name": "Aurora Lattice",
        "mood": "Aether",
        "engines": ["OPTIC", "ORBITAL"],
        "parameters": {
            "OPTIC": {"macro_character": 0.2, "macro_movement": 0.55, "macro_coupling": 0.5, "macro_space": 0.95},
            "ORBITAL": {"macro_character": 0.15, "macro_movement": 0.5, "macro_coupling": 0.5, "macro_space": 0.98},
        },
        "dna": {"brightness": 0.87, "warmth": 0.13, "movement": 0.52, "density": 0.09, "space": 0.96, "aggression": 0.03},
        "macros": {"CHARACTER": 0.18, "MOVEMENT": 0.52, "COUPLING": 0.5, "SPACE": 0.96},
        "tags": ["aether", "crystalline", "entangled"],
    },
    {
        "name": "Shard of Dawn",
        "mood": "Aether",
        "engines": ["OPAL"],
        "parameters": {
            "OPAL": {"macro_character": 0.24, "macro_movement": 0.58, "macro_coupling": 0.14, "macro_space": 0.91}
        },
        "dna": {"brightness": 0.82, "warmth": 0.2, "movement": 0.58, "density": 0.11, "space": 0.91, "aggression": 0.05},
        "macros": {"CHARACTER": 0.24, "MOVEMENT": 0.58, "COUPLING": 0.14, "SPACE": 0.91},
        "tags": ["aether", "crystalline", "ambient"],
    },
    {
        "name": "Clear Frequency",
        "mood": "Aether",
        "engines": ["ORGANON"],
        "parameters": {
            "ORGANON": {"macro_character": 0.18, "macro_movement": 0.52, "macro_coupling": 0.1, "macro_space": 0.95}
        },
        "dna": {"brightness": 0.76, "warmth": 0.24, "movement": 0.52, "density": 0.1, "space": 0.95, "aggression": 0.04},
        "macros": {"CHARACTER": 0.18, "MOVEMENT": 0.52, "COUPLING": 0.1, "SPACE": 0.95},
        "tags": ["aether", "crystalline", "ambient"],
    },
    {
        "name": "Refracted Dawn",
        "mood": "Aether",
        "engines": ["OVERWORLD"],
        "parameters": {
            "OVERWORLD": {"macro_character": 0.26, "macro_movement": 0.56, "macro_coupling": 0.15, "macro_space": 0.88}
        },
        "dna": {"brightness": 0.78, "warmth": 0.22, "movement": 0.56, "density": 0.12, "space": 0.88, "aggression": 0.06},
        "macros": {"CHARACTER": 0.26, "MOVEMENT": 0.56, "COUPLING": 0.15, "SPACE": 0.88},
        "tags": ["aether", "crystalline", "evolving"],
    },
    {
        "name": "Pure Resonance",
        "mood": "Aether",
        "engines": ["OBBLIGATO"],
        "parameters": {
            "OBBLIGATO": {"macro_character": 0.2, "macro_movement": 0.5, "macro_coupling": 0.1, "macro_space": 0.93}
        },
        "dna": {"brightness": 0.74, "warmth": 0.26, "movement": 0.5, "density": 0.1, "space": 0.93, "aggression": 0.04},
        "macros": {"CHARACTER": 0.2, "MOVEMENT": 0.5, "COUPLING": 0.1, "SPACE": 0.93},
        "tags": ["aether", "crystalline", "ambient"],
    },
    {
        "name": "Veil of Quartz",
        "mood": "Aether",
        "engines": ["OCTOPUS"],
        "parameters": {
            "OCTOPUS": {"macro_character": 0.22, "macro_movement": 0.54, "macro_coupling": 0.12, "macro_space": 0.92}
        },
        "dna": {"brightness": 0.8, "warmth": 0.2, "movement": 0.54, "density": 0.11, "space": 0.92, "aggression": 0.05},
        "macros": {"CHARACTER": 0.22, "MOVEMENT": 0.54, "COUPLING": 0.12, "SPACE": 0.92},
        "tags": ["aether", "crystalline", "otherworldly"],
    },
    {
        "name": "Thin Air",
        "mood": "Aether",
        "engines": ["ODDFELIX"],
        "parameters": {
            "ODDFELIX": {"macro_character": 0.16, "macro_movement": 0.48, "macro_coupling": 0.08, "macro_space": 0.96}
        },
        "dna": {"brightness": 0.88, "warmth": 0.12, "movement": 0.48, "density": 0.08, "space": 0.96, "aggression": 0.03},
        "macros": {"CHARACTER": 0.16, "MOVEMENT": 0.48, "COUPLING": 0.08, "SPACE": 0.96},
        "tags": ["aether", "crystalline", "ambient"],
    },
]


def name_to_snake(name: str) -> str:
    s = name.lower()
    s = re.sub(r"[^a-z0-9]+", "_", s)
    s = s.strip("_")
    return s


def main():
    os.makedirs(OUTPUT_DIR, exist_ok=True)

    saved = 0
    seen_names = set()

    for preset in PRESETS:
        name = preset["name"]
        assert name not in seen_names, f"Duplicate preset name: {name}"
        assert len(name) <= 30, f"Name too long ({len(name)}): {name}"
        seen_names.add(name)

        doc = {
            "name": name,
            "version": "1.0",
            "mood": preset["mood"],
            "engines": preset["engines"],
            "parameters": preset["parameters"],
            "dna": preset["dna"],
            "macros": preset["macros"],
            "tags": preset["tags"],
        }

        filename = name_to_snake(name) + ".xometa"
        filepath = os.path.join(OUTPUT_DIR, filename)

        with open(filepath, "w", encoding="utf-8") as f:
            json.dump(doc, f, indent=2)
            f.write("\n")

        saved += 1

    print(f"Saved {saved} Aether presets to {OUTPUT_DIR}")


if __name__ == "__main__":
    main()
