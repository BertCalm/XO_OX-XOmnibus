#!/usr/bin/env python3
"""
xpn_odyssey_oblong_obese_coverage_pack.py
Generate ~90 Entangled mood .xometa preset stubs covering new coupling pairs
for ODYSSEY, OBLONG, and OBESE engines.

ODYSSEY × ORGANON/OUROBOROS/OBSIDIAN/ORIGAMI/ORACLE: 5 pairs × 6 = 30 presets
OBLONG  × ORGANON/OUROBOROS/ORIGAMI/ORACLE/OBSCURA:  5 pairs × 6 = 30 presets
OBESE   × ORGANON/OUROBOROS/ORIGAMI/ORACLE/OBSCURA:  5 pairs × 6 = 30 presets

Total: 90 presets written to Presets/XOceanus/Entangled/
Skips files that already exist.
"""

import json
import os
import re
import random
from datetime import date

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
REPO_ROOT  = os.path.dirname(SCRIPT_DIR)
OUTPUT_DIR = os.path.join(REPO_ROOT, "Presets", "XOceanus", "Entangled")

TODAY   = str(date.today())
VERSION = "1.0"
AUTHOR  = "XO_OX Coverage Pack 2026-03-16"

COUPLING_TYPES = [
    "FREQUENCY_SHIFT", "AMPLITUDE_MOD", "FILTER_MOD", "PITCH_SYNC",
    "TIMBRE_BLEND", "ENVELOPE_LINK", "HARMONIC_FOLD", "CHAOS_INJECT",
    "RESONANCE_SHARE", "SPATIAL_COUPLE", "SPECTRAL_MORPH", "VELOCITY_COUPLE",
]

# ──────────────────────────────────────────────────────────────
# 90 preset definitions
# Each row: (engine1, engine2, name, dna_dict, e1_params, e2_params, coupling_type)
# DNA keys: brightness, warmth, movement, density, space, aggression
# ──────────────────────────────────────────────────────────────

PRESETS = [
    # ─── ODYSSEY × ORGANON (6) ───
    ("ODYSSEY", "ORGANON", "Spectral Covenant",
     dict(brightness=0.8, warmth=0.3, movement=0.7, density=0.4, space=0.7, aggression=0.2),
     dict(macro_character=0.7, macro_movement=0.8, macro_coupling=0.6, macro_space=0.7),
     dict(macro_character=0.5, macro_movement=0.6, macro_coupling=0.7, macro_space=0.6),
     "FREQUENCY_SHIFT"),
    ("ODYSSEY", "ORGANON", "Deep Logic Thread",
     dict(brightness=0.3, warmth=0.6, movement=0.4, density=0.8, space=0.3, aggression=0.4),
     dict(macro_character=0.4, macro_movement=0.4, macro_coupling=0.8, macro_space=0.3),
     dict(macro_character=0.7, macro_movement=0.5, macro_coupling=0.9, macro_space=0.3),
     "HARMONIC_FOLD"),
    ("ODYSSEY", "ORGANON", "Amber Logic Shift",
     dict(brightness=0.6, warmth=0.7, movement=0.5, density=0.5, space=0.5, aggression=0.3),
     dict(macro_character=0.6, macro_movement=0.5, macro_coupling=0.7, macro_space=0.5),
     dict(macro_character=0.5, macro_movement=0.6, macro_coupling=0.6, macro_space=0.5),
     "FILTER_MOD"),
    ("ODYSSEY", "ORGANON", "Cold Meridian",
     dict(brightness=0.2, warmth=0.2, movement=0.8, density=0.3, space=0.9, aggression=0.1),
     dict(macro_character=0.3, macro_movement=0.9, macro_coupling=0.5, macro_space=0.9),
     dict(macro_character=0.2, macro_movement=0.7, macro_coupling=0.6, macro_space=0.8),
     "SPATIAL_COUPLE"),
    ("ODYSSEY", "ORGANON", "Radiant Calculus",
     dict(brightness=0.9, warmth=0.4, movement=0.6, density=0.6, space=0.5, aggression=0.5),
     dict(macro_character=0.8, macro_movement=0.6, macro_coupling=0.7, macro_space=0.5),
     dict(macro_character=0.6, macro_movement=0.7, macro_coupling=0.8, macro_space=0.4),
     "SPECTRAL_MORPH"),
    ("ODYSSEY", "ORGANON", "Bone Logic Drift",
     dict(brightness=0.4, warmth=0.8, movement=0.3, density=0.9, space=0.2, aggression=0.7),
     dict(macro_character=0.5, macro_movement=0.3, macro_coupling=0.9, macro_space=0.2),
     dict(macro_character=0.8, macro_movement=0.4, macro_coupling=0.8, macro_space=0.2),
     "CHAOS_INJECT"),

    # ─── ODYSSEY × OUROBOROS (6) ───
    ("ODYSSEY", "OUROBOROS", "Eternal Wavetide",
     dict(brightness=0.5, warmth=0.5, movement=0.9, density=0.5, space=0.6, aggression=0.3),
     dict(macro_character=0.5, macro_movement=0.9, macro_coupling=0.8, macro_space=0.6),
     dict(macro_character=0.6, macro_movement=0.9, macro_coupling=0.7, macro_space=0.6),
     "ENVELOPE_LINK"),
    ("ODYSSEY", "OUROBOROS", "Serpent Passage",
     dict(brightness=0.3, warmth=0.4, movement=0.7, density=0.6, space=0.4, aggression=0.6),
     dict(macro_character=0.4, macro_movement=0.7, macro_coupling=0.8, macro_space=0.4),
     dict(macro_character=0.5, macro_movement=0.8, macro_coupling=0.9, macro_space=0.3),
     "PITCH_SYNC"),
    ("ODYSSEY", "OUROBOROS", "Coil Horizon",
     dict(brightness=0.7, warmth=0.3, movement=0.8, density=0.4, space=0.7, aggression=0.2),
     dict(macro_character=0.6, macro_movement=0.8, macro_coupling=0.6, macro_space=0.7),
     dict(macro_character=0.4, macro_movement=0.9, macro_coupling=0.7, macro_space=0.7),
     "RESONANCE_SHARE"),
    ("ODYSSEY", "OUROBOROS", "Tail Devourer",
     dict(brightness=0.2, warmth=0.6, movement=0.6, density=0.9, space=0.2, aggression=0.8),
     dict(macro_character=0.3, macro_movement=0.6, macro_coupling=0.9, macro_space=0.2),
     dict(macro_character=0.7, macro_movement=0.6, macro_coupling=0.9, macro_space=0.2),
     "CHAOS_INJECT"),
    ("ODYSSEY", "OUROBOROS", "Bright Loop Fracture",
     dict(brightness=0.9, warmth=0.2, movement=0.7, density=0.3, space=0.8, aggression=0.4),
     dict(macro_character=0.8, macro_movement=0.7, macro_coupling=0.6, macro_space=0.8),
     dict(macro_character=0.3, macro_movement=0.8, macro_coupling=0.7, macro_space=0.7),
     "FREQUENCY_SHIFT"),
    ("ODYSSEY", "OUROBOROS", "Dense Cycle Gate",
     dict(brightness=0.4, warmth=0.7, movement=0.5, density=0.8, space=0.3, aggression=0.5),
     dict(macro_character=0.5, macro_movement=0.5, macro_coupling=0.8, macro_space=0.3),
     dict(macro_character=0.7, macro_movement=0.6, macro_coupling=0.8, macro_space=0.3),
     "AMPLITUDE_MOD"),

    # ─── ODYSSEY × OBSIDIAN (6) ───
    ("ODYSSEY", "OBSIDIAN", "Volcanic Transit",
     dict(brightness=0.2, warmth=0.7, movement=0.6, density=0.8, space=0.3, aggression=0.7),
     dict(macro_character=0.3, macro_movement=0.6, macro_coupling=0.8, macro_space=0.3),
     dict(macro_character=0.8, macro_movement=0.5, macro_coupling=0.9, macro_space=0.2),
     "FILTER_MOD"),
    ("ODYSSEY", "OBSIDIAN", "Glassy Wanderer",
     dict(brightness=0.7, warmth=0.3, movement=0.5, density=0.4, space=0.7, aggression=0.2),
     dict(macro_character=0.7, macro_movement=0.5, macro_coupling=0.6, macro_space=0.7),
     dict(macro_character=0.4, macro_movement=0.4, macro_coupling=0.5, macro_space=0.8),
     "SPECTRAL_MORPH"),
    ("ODYSSEY", "OBSIDIAN", "Black Mirror Path",
     dict(brightness=0.1, warmth=0.5, movement=0.8, density=0.7, space=0.5, aggression=0.6),
     dict(macro_character=0.2, macro_movement=0.8, macro_coupling=0.9, macro_space=0.5),
     dict(macro_character=0.6, macro_movement=0.7, macro_coupling=0.8, macro_space=0.4),
     "HARMONIC_FOLD"),
    ("ODYSSEY", "OBSIDIAN", "Shard Odyssey",
     dict(brightness=0.6, warmth=0.4, movement=0.7, density=0.5, space=0.6, aggression=0.5),
     dict(macro_character=0.6, macro_movement=0.7, macro_coupling=0.7, macro_space=0.6),
     dict(macro_character=0.5, macro_movement=0.6, macro_coupling=0.8, macro_space=0.5),
     "PITCH_SYNC"),
    ("ODYSSEY", "OBSIDIAN", "Obsidian Voyage",
     dict(brightness=0.3, warmth=0.8, movement=0.4, density=0.9, space=0.2, aggression=0.8),
     dict(macro_character=0.4, macro_movement=0.4, macro_coupling=0.9, macro_space=0.2),
     dict(macro_character=0.9, macro_movement=0.3, macro_coupling=0.9, macro_space=0.2),
     "CHAOS_INJECT"),
    ("ODYSSEY", "OBSIDIAN", "Pale Fracture Map",
     dict(brightness=0.8, warmth=0.2, movement=0.6, density=0.3, space=0.9, aggression=0.1),
     dict(macro_character=0.8, macro_movement=0.6, macro_coupling=0.5, macro_space=0.9),
     dict(macro_character=0.2, macro_movement=0.5, macro_coupling=0.6, macro_space=0.9),
     "SPATIAL_COUPLE"),

    # ─── ODYSSEY × ORIGAMI (6) ───
    ("ODYSSEY", "ORIGAMI", "Fold Horizon Line",
     dict(brightness=0.6, warmth=0.5, movement=0.7, density=0.5, space=0.6, aggression=0.2),
     dict(macro_character=0.6, macro_movement=0.7, macro_coupling=0.7, macro_space=0.6),
     dict(macro_character=0.5, macro_movement=0.6, macro_coupling=0.6, macro_space=0.7),
     "ENVELOPE_LINK"),
    ("ODYSSEY", "ORIGAMI", "Crease Journey",
     dict(brightness=0.4, warmth=0.6, movement=0.5, density=0.6, space=0.5, aggression=0.3),
     dict(macro_character=0.4, macro_movement=0.5, macro_coupling=0.7, macro_space=0.5),
     dict(macro_character=0.6, macro_movement=0.5, macro_coupling=0.7, macro_space=0.5),
     "TIMBRE_BLEND"),
    ("ODYSSEY", "ORIGAMI", "Paper Wave Gate",
     dict(brightness=0.8, warmth=0.3, movement=0.8, density=0.3, space=0.8, aggression=0.2),
     dict(macro_character=0.8, macro_movement=0.8, macro_coupling=0.6, macro_space=0.8),
     dict(macro_character=0.3, macro_movement=0.8, macro_coupling=0.7, macro_space=0.8),
     "FREQUENCY_SHIFT"),
    ("ODYSSEY", "ORIGAMI", "Dense Fold Flux",
     dict(brightness=0.2, warmth=0.8, movement=0.4, density=0.9, space=0.2, aggression=0.6),
     dict(macro_character=0.3, macro_movement=0.4, macro_coupling=0.9, macro_space=0.2),
     dict(macro_character=0.8, macro_movement=0.4, macro_coupling=0.8, macro_space=0.2),
     "RESONANCE_SHARE"),
    ("ODYSSEY", "ORIGAMI", "Acute Transit",
     dict(brightness=0.5, warmth=0.4, movement=0.9, density=0.4, space=0.7, aggression=0.4),
     dict(macro_character=0.5, macro_movement=0.9, macro_coupling=0.7, macro_space=0.7),
     dict(macro_character=0.4, macro_movement=0.9, macro_coupling=0.8, macro_space=0.6),
     "AMPLITUDE_MOD"),
    ("ODYSSEY", "ORIGAMI", "Concertina Drift",
     dict(brightness=0.7, warmth=0.6, movement=0.3, density=0.7, space=0.4, aggression=0.4),
     dict(macro_character=0.7, macro_movement=0.3, macro_coupling=0.8, macro_space=0.4),
     dict(macro_character=0.6, macro_movement=0.3, macro_coupling=0.7, macro_space=0.4),
     "SPECTRAL_MORPH"),

    # ─── ODYSSEY × ORACLE (6) ───
    ("ODYSSEY", "ORACLE", "Prophetic Course",
     dict(brightness=0.6, warmth=0.5, movement=0.6, density=0.5, space=0.6, aggression=0.3),
     dict(macro_character=0.6, macro_movement=0.6, macro_coupling=0.7, macro_space=0.6),
     dict(macro_character=0.5, macro_movement=0.6, macro_coupling=0.7, macro_space=0.6),
     "PITCH_SYNC"),
    ("ODYSSEY", "ORACLE", "Foresight Transit",
     dict(brightness=0.8, warmth=0.3, movement=0.7, density=0.3, space=0.8, aggression=0.2),
     dict(macro_character=0.8, macro_movement=0.7, macro_coupling=0.6, macro_space=0.8),
     dict(macro_character=0.3, macro_movement=0.6, macro_coupling=0.7, macro_space=0.8),
     "SPATIAL_COUPLE"),
    ("ODYSSEY", "ORACLE", "Oracle Wanderer",
     dict(brightness=0.4, warmth=0.7, movement=0.5, density=0.7, space=0.4, aggression=0.5),
     dict(macro_character=0.4, macro_movement=0.5, macro_coupling=0.8, macro_space=0.4),
     dict(macro_character=0.7, macro_movement=0.5, macro_coupling=0.8, macro_space=0.4),
     "FILTER_MOD"),
    ("ODYSSEY", "ORACLE", "Omen Passage",
     dict(brightness=0.2, warmth=0.5, movement=0.8, density=0.6, space=0.5, aggression=0.7),
     dict(macro_character=0.2, macro_movement=0.8, macro_coupling=0.9, macro_space=0.5),
     dict(macro_character=0.6, macro_movement=0.8, macro_coupling=0.8, macro_space=0.4),
     "CHAOS_INJECT"),
    ("ODYSSEY", "ORACLE", "Augur Horizon",
     dict(brightness=0.7, warmth=0.6, movement=0.4, density=0.6, space=0.5, aggression=0.3),
     dict(macro_character=0.7, macro_movement=0.4, macro_coupling=0.7, macro_space=0.5),
     dict(macro_character=0.6, macro_movement=0.4, macro_coupling=0.7, macro_space=0.5),
     "HARMONIC_FOLD"),
    ("ODYSSEY", "ORACLE", "Pale Augury",
     dict(brightness=0.9, warmth=0.2, movement=0.6, density=0.2, space=0.9, aggression=0.1),
     dict(macro_character=0.9, macro_movement=0.6, macro_coupling=0.5, macro_space=0.9),
     dict(macro_character=0.2, macro_movement=0.5, macro_coupling=0.6, macro_space=0.9),
     "TIMBRE_BLEND"),

    # ─── OBLONG × ORGANON (6) ───
    ("OBLONG", "ORGANON", "Lateral Theorem",
     dict(brightness=0.5, warmth=0.6, movement=0.5, density=0.7, space=0.4, aggression=0.4),
     dict(macro_character=0.5, macro_movement=0.5, macro_coupling=0.8, macro_space=0.4),
     dict(macro_character=0.7, macro_movement=0.5, macro_coupling=0.8, macro_space=0.4),
     "FILTER_MOD"),
    ("OBLONG", "ORGANON", "Stretched Logic",
     dict(brightness=0.3, warmth=0.4, movement=0.7, density=0.5, space=0.6, aggression=0.3),
     dict(macro_character=0.3, macro_movement=0.7, macro_coupling=0.7, macro_space=0.6),
     dict(macro_character=0.5, macro_movement=0.6, macro_coupling=0.7, macro_space=0.6),
     "AMPLITUDE_MOD"),
    ("OBLONG", "ORGANON", "Elongate Formula",
     dict(brightness=0.7, warmth=0.5, movement=0.4, density=0.6, space=0.5, aggression=0.3),
     dict(macro_character=0.7, macro_movement=0.4, macro_coupling=0.7, macro_space=0.5),
     dict(macro_character=0.6, macro_movement=0.4, macro_coupling=0.7, macro_space=0.5),
     "SPECTRAL_MORPH"),
    ("OBLONG", "ORGANON", "Calculus Slab",
     dict(brightness=0.2, warmth=0.8, movement=0.3, density=0.9, space=0.2, aggression=0.7),
     dict(macro_character=0.3, macro_movement=0.3, macro_coupling=0.9, macro_space=0.2),
     dict(macro_character=0.8, macro_movement=0.3, macro_coupling=0.9, macro_space=0.2),
     "CHAOS_INJECT"),
    ("OBLONG", "ORGANON", "Bright Axiom",
     dict(brightness=0.9, warmth=0.2, movement=0.7, density=0.3, space=0.8, aggression=0.2),
     dict(macro_character=0.9, macro_movement=0.7, macro_coupling=0.6, macro_space=0.8),
     dict(macro_character=0.2, macro_movement=0.6, macro_coupling=0.7, macro_space=0.8),
     "FREQUENCY_SHIFT"),
    ("OBLONG", "ORGANON", "Parallel Proof",
     dict(brightness=0.5, warmth=0.7, movement=0.6, density=0.6, space=0.5, aggression=0.5),
     dict(macro_character=0.5, macro_movement=0.6, macro_coupling=0.8, macro_space=0.5),
     dict(macro_character=0.7, macro_movement=0.6, macro_coupling=0.8, macro_space=0.5),
     "ENVELOPE_LINK"),

    # ─── OBLONG × OUROBOROS (6) ───
    ("OBLONG", "OUROBOROS", "Slab Cycle Return",
     dict(brightness=0.4, warmth=0.6, movement=0.8, density=0.6, space=0.4, aggression=0.5),
     dict(macro_character=0.4, macro_movement=0.8, macro_coupling=0.8, macro_space=0.4),
     dict(macro_character=0.6, macro_movement=0.8, macro_coupling=0.9, macro_space=0.4),
     "PITCH_SYNC"),
    ("OBLONG", "OUROBOROS", "Stretched Serpent",
     dict(brightness=0.6, warmth=0.4, movement=0.7, density=0.5, space=0.6, aggression=0.4),
     dict(macro_character=0.6, macro_movement=0.7, macro_coupling=0.7, macro_space=0.6),
     dict(macro_character=0.4, macro_movement=0.8, macro_coupling=0.8, macro_space=0.5),
     "RESONANCE_SHARE"),
    ("OBLONG", "OUROBOROS", "Plinth Loop Gate",
     dict(brightness=0.2, warmth=0.5, movement=0.9, density=0.7, space=0.3, aggression=0.7),
     dict(macro_character=0.2, macro_movement=0.9, macro_coupling=0.9, macro_space=0.3),
     dict(macro_character=0.7, macro_movement=0.9, macro_coupling=0.9, macro_space=0.3),
     "CHAOS_INJECT"),
    ("OBLONG", "OUROBOROS", "Dense Coil Slab",
     dict(brightness=0.3, warmth=0.8, movement=0.5, density=0.9, space=0.2, aggression=0.6),
     dict(macro_character=0.3, macro_movement=0.5, macro_coupling=0.9, macro_space=0.2),
     dict(macro_character=0.8, macro_movement=0.5, macro_coupling=0.9, macro_space=0.2),
     "HARMONIC_FOLD"),
    ("OBLONG", "OUROBOROS", "Bright Recursion",
     dict(brightness=0.9, warmth=0.3, movement=0.7, density=0.3, space=0.8, aggression=0.2),
     dict(macro_character=0.9, macro_movement=0.7, macro_coupling=0.6, macro_space=0.8),
     dict(macro_character=0.3, macro_movement=0.8, macro_coupling=0.7, macro_space=0.7),
     "TIMBRE_BLEND"),
    ("OBLONG", "OUROBOROS", "Uroboric Monolith",
     dict(brightness=0.5, warmth=0.5, movement=0.6, density=0.7, space=0.4, aggression=0.4),
     dict(macro_character=0.5, macro_movement=0.6, macro_coupling=0.8, macro_space=0.4),
     dict(macro_character=0.6, macro_movement=0.7, macro_coupling=0.8, macro_space=0.4),
     "AMPLITUDE_MOD"),

    # ─── OBLONG × ORIGAMI (6) ───
    ("OBLONG", "ORIGAMI", "Slab Fold Protocol",
     dict(brightness=0.6, warmth=0.5, movement=0.6, density=0.6, space=0.5, aggression=0.3),
     dict(macro_character=0.6, macro_movement=0.6, macro_coupling=0.7, macro_space=0.5),
     dict(macro_character=0.5, macro_movement=0.6, macro_coupling=0.7, macro_space=0.6),
     "FILTER_MOD"),
    ("OBLONG", "ORIGAMI", "Rigid Crease Line",
     dict(brightness=0.4, warmth=0.4, movement=0.7, density=0.5, space=0.7, aggression=0.3),
     dict(macro_character=0.4, macro_movement=0.7, macro_coupling=0.7, macro_space=0.7),
     dict(macro_character=0.4, macro_movement=0.7, macro_coupling=0.6, macro_space=0.7),
     "SPATIAL_COUPLE"),
    ("OBLONG", "ORIGAMI", "Monument Crease",
     dict(brightness=0.2, warmth=0.7, movement=0.4, density=0.9, space=0.2, aggression=0.7),
     dict(macro_character=0.2, macro_movement=0.4, macro_coupling=0.9, macro_space=0.2),
     dict(macro_character=0.8, macro_movement=0.4, macro_coupling=0.8, macro_space=0.2),
     "VELOCITY_COUPLE"),
    ("OBLONG", "ORIGAMI", "Thin Air Slab",
     dict(brightness=0.8, warmth=0.2, movement=0.8, density=0.2, space=0.9, aggression=0.1),
     dict(macro_character=0.8, macro_movement=0.8, macro_coupling=0.5, macro_space=0.9),
     dict(macro_character=0.2, macro_movement=0.8, macro_coupling=0.6, macro_space=0.9),
     "FREQUENCY_SHIFT"),
    ("OBLONG", "ORIGAMI", "Warm Pleat Mass",
     dict(brightness=0.5, warmth=0.8, movement=0.4, density=0.8, space=0.3, aggression=0.5),
     dict(macro_character=0.5, macro_movement=0.4, macro_coupling=0.8, macro_space=0.3),
     dict(macro_character=0.8, macro_movement=0.4, macro_coupling=0.8, macro_space=0.3),
     "RESONANCE_SHARE"),
    ("OBLONG", "ORIGAMI", "Edge Buckle Form",
     dict(brightness=0.7, warmth=0.5, movement=0.6, density=0.5, space=0.5, aggression=0.4),
     dict(macro_character=0.7, macro_movement=0.6, macro_coupling=0.7, macro_space=0.5),
     dict(macro_character=0.5, macro_movement=0.6, macro_coupling=0.7, macro_space=0.5),
     "SPECTRAL_MORPH"),

    # ─── OBLONG × ORACLE (6) ───
    ("OBLONG", "ORACLE", "Slab Oracle Rite",
     dict(brightness=0.5, warmth=0.6, movement=0.5, density=0.6, space=0.5, aggression=0.4),
     dict(macro_character=0.5, macro_movement=0.5, macro_coupling=0.8, macro_space=0.5),
     dict(macro_character=0.6, macro_movement=0.5, macro_coupling=0.8, macro_space=0.5),
     "ENVELOPE_LINK"),
    ("OBLONG", "ORACLE", "Prophetic Monolith",
     dict(brightness=0.3, warmth=0.7, movement=0.4, density=0.8, space=0.3, aggression=0.6),
     dict(macro_character=0.3, macro_movement=0.4, macro_coupling=0.9, macro_space=0.3),
     dict(macro_character=0.7, macro_movement=0.4, macro_coupling=0.8, macro_space=0.3),
     "HARMONIC_FOLD"),
    ("OBLONG", "ORACLE", "Vision Slab Wide",
     dict(brightness=0.8, warmth=0.3, movement=0.7, density=0.3, space=0.8, aggression=0.2),
     dict(macro_character=0.8, macro_movement=0.7, macro_coupling=0.6, macro_space=0.8),
     dict(macro_character=0.3, macro_movement=0.6, macro_coupling=0.7, macro_space=0.8),
     "PITCH_SYNC"),
    ("OBLONG", "ORACLE", "Omen Block Weight",
     dict(brightness=0.2, warmth=0.5, movement=0.8, density=0.7, space=0.4, aggression=0.8),
     dict(macro_character=0.2, macro_movement=0.8, macro_coupling=0.9, macro_space=0.4),
     dict(macro_character=0.7, macro_movement=0.8, macro_coupling=0.9, macro_space=0.3),
     "CHAOS_INJECT"),
    ("OBLONG", "ORACLE", "Augury Plinth",
     dict(brightness=0.6, warmth=0.6, movement=0.5, density=0.6, space=0.5, aggression=0.3),
     dict(macro_character=0.6, macro_movement=0.5, macro_coupling=0.7, macro_space=0.5),
     dict(macro_character=0.6, macro_movement=0.5, macro_coupling=0.7, macro_space=0.5),
     "FILTER_MOD"),
    ("OBLONG", "ORACLE", "Clear Weight Read",
     dict(brightness=0.9, warmth=0.2, movement=0.6, density=0.2, space=0.9, aggression=0.1),
     dict(macro_character=0.9, macro_movement=0.6, macro_coupling=0.5, macro_space=0.9),
     dict(macro_character=0.2, macro_movement=0.5, macro_coupling=0.6, macro_space=0.9),
     "SPATIAL_COUPLE"),

    # ─── OBLONG × OBSCURA (6) ───
    ("OBLONG", "OBSCURA", "Dark Slab Veil",
     dict(brightness=0.1, warmth=0.6, movement=0.5, density=0.8, space=0.4, aggression=0.6),
     dict(macro_character=0.2, macro_movement=0.5, macro_coupling=0.8, macro_space=0.4),
     dict(macro_character=0.7, macro_movement=0.5, macro_coupling=0.8, macro_space=0.4),
     "AMPLITUDE_MOD"),
    ("OBLONG", "OBSCURA", "Obscured Plinth",
     dict(brightness=0.3, warmth=0.7, movement=0.4, density=0.8, space=0.3, aggression=0.5),
     dict(macro_character=0.3, macro_movement=0.4, macro_coupling=0.9, macro_space=0.3),
     dict(macro_character=0.7, macro_movement=0.4, macro_coupling=0.8, macro_space=0.3),
     "FILTER_MOD"),
    ("OBLONG", "OBSCURA", "Haze Monument",
     dict(brightness=0.5, warmth=0.5, movement=0.6, density=0.6, space=0.5, aggression=0.4),
     dict(macro_character=0.5, macro_movement=0.6, macro_coupling=0.7, macro_space=0.5),
     dict(macro_character=0.5, macro_movement=0.6, macro_coupling=0.7, macro_space=0.5),
     "SPECTRAL_MORPH"),
    ("OBLONG", "OBSCURA", "Fog Slab Drift",
     dict(brightness=0.2, warmth=0.4, movement=0.7, density=0.5, space=0.7, aggression=0.3),
     dict(macro_character=0.2, macro_movement=0.7, macro_coupling=0.7, macro_space=0.7),
     dict(macro_character=0.5, macro_movement=0.7, macro_coupling=0.7, macro_space=0.7),
     "RESONANCE_SHARE"),
    ("OBLONG", "OBSCURA", "Penumbra Slab",
     dict(brightness=0.4, warmth=0.6, movement=0.5, density=0.7, space=0.4, aggression=0.5),
     dict(macro_character=0.4, macro_movement=0.5, macro_coupling=0.8, macro_space=0.4),
     dict(macro_character=0.6, macro_movement=0.5, macro_coupling=0.8, macro_space=0.4),
     "TIMBRE_BLEND"),
    ("OBLONG", "OBSCURA", "Murk Block Surge",
     dict(brightness=0.1, warmth=0.8, movement=0.7, density=0.9, space=0.2, aggression=0.8),
     dict(macro_character=0.2, macro_movement=0.7, macro_coupling=0.9, macro_space=0.2),
     dict(macro_character=0.8, macro_movement=0.7, macro_coupling=0.9, macro_space=0.2),
     "CHAOS_INJECT"),

    # ─── OBESE × ORGANON (6) ───
    ("OBESE", "ORGANON", "Mass Theorem Drive",
     dict(brightness=0.3, warmth=0.8, movement=0.4, density=0.9, space=0.2, aggression=0.7),
     dict(macro_character=0.3, macro_movement=0.4, macro_coupling=0.9, macro_space=0.2),
     dict(macro_character=0.8, macro_movement=0.4, macro_coupling=0.9, macro_space=0.2),
     "HARMONIC_FOLD"),
    ("OBESE", "ORGANON", "Fat Axiom",
     dict(brightness=0.5, warmth=0.7, movement=0.5, density=0.8, space=0.3, aggression=0.5),
     dict(macro_character=0.5, macro_movement=0.5, macro_coupling=0.8, macro_space=0.3),
     dict(macro_character=0.7, macro_movement=0.5, macro_coupling=0.8, macro_space=0.3),
     "FILTER_MOD"),
    ("OBESE", "ORGANON", "Logic Expansion",
     dict(brightness=0.4, warmth=0.6, movement=0.6, density=0.7, space=0.4, aggression=0.4),
     dict(macro_character=0.4, macro_movement=0.6, macro_coupling=0.8, macro_space=0.4),
     dict(macro_character=0.6, macro_movement=0.6, macro_coupling=0.8, macro_space=0.4),
     "RESONANCE_SHARE"),
    ("OBESE", "ORGANON", "Dense Proof Wall",
     dict(brightness=0.2, warmth=0.9, movement=0.3, density=0.9, space=0.1, aggression=0.8),
     dict(macro_character=0.2, macro_movement=0.3, macro_coupling=0.9, macro_space=0.1),
     dict(macro_character=0.9, macro_movement=0.3, macro_coupling=0.9, macro_space=0.1),
     "CHAOS_INJECT"),
    ("OBESE", "ORGANON", "Bright Fat Formula",
     dict(brightness=0.8, warmth=0.4, movement=0.6, density=0.5, space=0.6, aggression=0.3),
     dict(macro_character=0.8, macro_movement=0.6, macro_coupling=0.7, macro_space=0.6),
     dict(macro_character=0.4, macro_movement=0.6, macro_coupling=0.7, macro_space=0.6),
     "SPECTRAL_MORPH"),
    ("OBESE", "ORGANON", "Saturated Calculus",
     dict(brightness=0.6, warmth=0.7, movement=0.5, density=0.7, space=0.3, aggression=0.6),
     dict(macro_character=0.6, macro_movement=0.5, macro_coupling=0.8, macro_space=0.3),
     dict(macro_character=0.7, macro_movement=0.5, macro_coupling=0.8, macro_space=0.3),
     "AMPLITUDE_MOD"),

    # ─── OBESE × OUROBOROS (6) ───
    ("OBESE", "OUROBOROS", "Full Cycle Mass",
     dict(brightness=0.4, warmth=0.6, movement=0.8, density=0.8, space=0.3, aggression=0.5),
     dict(macro_character=0.4, macro_movement=0.8, macro_coupling=0.8, macro_space=0.3),
     dict(macro_character=0.7, macro_movement=0.9, macro_coupling=0.8, macro_space=0.3),
     "PITCH_SYNC"),
    ("OBESE", "OUROBOROS", "Fat Serpent Loop",
     dict(brightness=0.3, warmth=0.7, movement=0.7, density=0.8, space=0.2, aggression=0.6),
     dict(macro_character=0.3, macro_movement=0.7, macro_coupling=0.9, macro_space=0.2),
     dict(macro_character=0.7, macro_movement=0.8, macro_coupling=0.9, macro_space=0.2),
     "ENVELOPE_LINK"),
    ("OBESE", "OUROBOROS", "Bloat Recursion",
     dict(brightness=0.5, warmth=0.5, movement=0.9, density=0.6, space=0.4, aggression=0.5),
     dict(macro_character=0.5, macro_movement=0.9, macro_coupling=0.8, macro_space=0.4),
     dict(macro_character=0.5, macro_movement=0.9, macro_coupling=0.8, macro_space=0.4),
     "FREQUENCY_SHIFT"),
    ("OBESE", "OUROBOROS", "Dense Coil Bulk",
     dict(brightness=0.2, warmth=0.8, movement=0.5, density=0.9, space=0.2, aggression=0.7),
     dict(macro_character=0.2, macro_movement=0.5, macro_coupling=0.9, macro_space=0.2),
     dict(macro_character=0.8, macro_movement=0.6, macro_coupling=0.9, macro_space=0.2),
     "HARMONIC_FOLD"),
    ("OBESE", "OUROBOROS", "Bright Girth Loop",
     dict(brightness=0.9, warmth=0.3, movement=0.7, density=0.4, space=0.7, aggression=0.3),
     dict(macro_character=0.9, macro_movement=0.7, macro_coupling=0.7, macro_space=0.7),
     dict(macro_character=0.3, macro_movement=0.8, macro_coupling=0.7, macro_space=0.6),
     "TIMBRE_BLEND"),
    ("OBESE", "OUROBOROS", "Swollen Cycle",
     dict(brightness=0.6, warmth=0.6, movement=0.6, density=0.7, space=0.3, aggression=0.5),
     dict(macro_character=0.6, macro_movement=0.6, macro_coupling=0.8, macro_space=0.3),
     dict(macro_character=0.6, macro_movement=0.7, macro_coupling=0.8, macro_space=0.3),
     "RESONANCE_SHARE"),

    # ─── OBESE × ORIGAMI (6) ───
    ("OBESE", "ORIGAMI", "Mass Fold Bloom",
     dict(brightness=0.6, warmth=0.6, movement=0.5, density=0.7, space=0.4, aggression=0.4),
     dict(macro_character=0.6, macro_movement=0.5, macro_coupling=0.8, macro_space=0.4),
     dict(macro_character=0.6, macro_movement=0.5, macro_coupling=0.7, macro_space=0.5),
     "FILTER_MOD"),
    ("OBESE", "ORIGAMI", "Fat Paper Sheet",
     dict(brightness=0.4, warmth=0.5, movement=0.7, density=0.6, space=0.5, aggression=0.3),
     dict(macro_character=0.4, macro_movement=0.7, macro_coupling=0.7, macro_space=0.5),
     dict(macro_character=0.5, macro_movement=0.7, macro_coupling=0.7, macro_space=0.6),
     "AMPLITUDE_MOD"),
    ("OBESE", "ORIGAMI", "Dense Pleat Form",
     dict(brightness=0.2, warmth=0.8, movement=0.4, density=0.9, space=0.2, aggression=0.6),
     dict(macro_character=0.2, macro_movement=0.4, macro_coupling=0.9, macro_space=0.2),
     dict(macro_character=0.8, macro_movement=0.4, macro_coupling=0.8, macro_space=0.2),
     "VELOCITY_COUPLE"),
    ("OBESE", "ORIGAMI", "Bloat Crease",
     dict(brightness=0.7, warmth=0.3, movement=0.8, density=0.3, space=0.8, aggression=0.2),
     dict(macro_character=0.7, macro_movement=0.8, macro_coupling=0.6, macro_space=0.8),
     dict(macro_character=0.3, macro_movement=0.8, macro_coupling=0.7, macro_space=0.8),
     "SPATIAL_COUPLE"),
    ("OBESE", "ORIGAMI", "Thick Fold Rush",
     dict(brightness=0.5, warmth=0.7, movement=0.7, density=0.7, space=0.3, aggression=0.6),
     dict(macro_character=0.5, macro_movement=0.7, macro_coupling=0.8, macro_space=0.3),
     dict(macro_character=0.7, macro_movement=0.7, macro_coupling=0.8, macro_space=0.3),
     "CHAOS_INJECT"),
    ("OBESE", "ORIGAMI", "Girth Buckle Arc",
     dict(brightness=0.8, warmth=0.5, movement=0.5, density=0.5, space=0.6, aggression=0.3),
     dict(macro_character=0.8, macro_movement=0.5, macro_coupling=0.7, macro_space=0.6),
     dict(macro_character=0.5, macro_movement=0.5, macro_coupling=0.7, macro_space=0.6),
     "SPECTRAL_MORPH"),

    # ─── OBESE × ORACLE (6) ───
    ("OBESE", "ORACLE", "Fat Oracle Vision",
     dict(brightness=0.5, warmth=0.6, movement=0.5, density=0.7, space=0.4, aggression=0.4),
     dict(macro_character=0.5, macro_movement=0.5, macro_coupling=0.8, macro_space=0.4),
     dict(macro_character=0.6, macro_movement=0.5, macro_coupling=0.8, macro_space=0.5),
     "PITCH_SYNC"),
    ("OBESE", "ORACLE", "Swollen Prophecy",
     dict(brightness=0.3, warmth=0.7, movement=0.5, density=0.8, space=0.3, aggression=0.6),
     dict(macro_character=0.3, macro_movement=0.5, macro_coupling=0.9, macro_space=0.3),
     dict(macro_character=0.7, macro_movement=0.5, macro_coupling=0.8, macro_space=0.3),
     "ENVELOPE_LINK"),
    ("OBESE", "ORACLE", "Dense Augury Wall",
     dict(brightness=0.2, warmth=0.8, movement=0.4, density=0.9, space=0.2, aggression=0.7),
     dict(macro_character=0.2, macro_movement=0.4, macro_coupling=0.9, macro_space=0.2),
     dict(macro_character=0.8, macro_movement=0.4, macro_coupling=0.9, macro_space=0.2),
     "HARMONIC_FOLD"),
    ("OBESE", "ORACLE", "Bright Mass Omen",
     dict(brightness=0.9, warmth=0.3, movement=0.6, density=0.3, space=0.8, aggression=0.2),
     dict(macro_character=0.9, macro_movement=0.6, macro_coupling=0.6, macro_space=0.8),
     dict(macro_character=0.3, macro_movement=0.5, macro_coupling=0.7, macro_space=0.8),
     "FREQUENCY_SHIFT"),
    ("OBESE", "ORACLE", "Corpus Reading",
     dict(brightness=0.6, warmth=0.5, movement=0.7, density=0.5, space=0.5, aggression=0.4),
     dict(macro_character=0.6, macro_movement=0.7, macro_coupling=0.7, macro_space=0.5),
     dict(macro_character=0.5, macro_movement=0.6, macro_coupling=0.7, macro_space=0.5),
     "FILTER_MOD"),
    ("OBESE", "ORACLE", "Bulk Seer Gate",
     dict(brightness=0.4, warmth=0.6, movement=0.6, density=0.7, space=0.4, aggression=0.5),
     dict(macro_character=0.4, macro_movement=0.6, macro_coupling=0.8, macro_space=0.4),
     dict(macro_character=0.6, macro_movement=0.6, macro_coupling=0.8, macro_space=0.4),
     "RESONANCE_SHARE"),

    # ─── OBESE × OBSCURA (6) ───
    ("OBESE", "OBSCURA", "Dark Mass Veil",
     dict(brightness=0.1, warmth=0.7, movement=0.4, density=0.9, space=0.2, aggression=0.7),
     dict(macro_character=0.1, macro_movement=0.4, macro_coupling=0.9, macro_space=0.2),
     dict(macro_character=0.8, macro_movement=0.4, macro_coupling=0.9, macro_space=0.2),
     "CHAOS_INJECT"),
    ("OBESE", "OBSCURA", "Fog Bulk Field",
     dict(brightness=0.3, warmth=0.6, movement=0.5, density=0.8, space=0.3, aggression=0.5),
     dict(macro_character=0.3, macro_movement=0.5, macro_coupling=0.8, macro_space=0.3),
     dict(macro_character=0.6, macro_movement=0.5, macro_coupling=0.8, macro_space=0.3),
     "AMPLITUDE_MOD"),
    ("OBESE", "OBSCURA", "Opaque Girth",
     dict(brightness=0.2, warmth=0.5, movement=0.6, density=0.7, space=0.4, aggression=0.6),
     dict(macro_character=0.2, macro_movement=0.6, macro_coupling=0.9, macro_space=0.4),
     dict(macro_character=0.6, macro_movement=0.6, macro_coupling=0.8, macro_space=0.4),
     "FILTER_MOD"),
    ("OBESE", "OBSCURA", "Penumbra Mass",
     dict(brightness=0.4, warmth=0.7, movement=0.5, density=0.8, space=0.3, aggression=0.5),
     dict(macro_character=0.4, macro_movement=0.5, macro_coupling=0.8, macro_space=0.3),
     dict(macro_character=0.7, macro_movement=0.5, macro_coupling=0.8, macro_space=0.3),
     "SPECTRAL_MORPH"),
    ("OBESE", "OBSCURA", "Dense Shadow Bulk",
     dict(brightness=0.1, warmth=0.8, movement=0.4, density=0.9, space=0.1, aggression=0.8),
     dict(macro_character=0.1, macro_movement=0.4, macro_coupling=0.9, macro_space=0.1),
     dict(macro_character=0.9, macro_movement=0.4, macro_coupling=0.9, macro_space=0.1),
     "HARMONIC_FOLD"),
    ("OBESE", "OBSCURA", "Murk Saturation",
     dict(brightness=0.3, warmth=0.7, movement=0.7, density=0.8, space=0.2, aggression=0.6),
     dict(macro_character=0.3, macro_movement=0.7, macro_coupling=0.8, macro_space=0.2),
     dict(macro_character=0.7, macro_movement=0.7, macro_coupling=0.8, macro_space=0.2),
     "VELOCITY_COUPLE"),
]


def to_snake(name):
    s = re.sub(r"[^a-zA-Z0-9 ]", "", name)
    return s.strip().replace(" ", "_").lower()


def build_preset(engine1, engine2, name, dna, e1_params, e2_params, coupling_type):
    return {
        "name": name,
        "version": VERSION,
        "mood": "Entangled",
        "engines": [engine1, engine2],
        "parameters": {
            engine1: e1_params,
            engine2: e2_params,
        },
        "coupling": {
            "type": coupling_type,
            "source": engine1,
            "target": engine2,
            "amount": round(e1_params["macro_coupling"], 2),
        },
        "dna": dna,
        "macros": {
            "CHARACTER": round((e1_params["macro_character"] + e2_params["macro_character"]) / 2, 2),
            "MOVEMENT":  round((e1_params["macro_movement"]  + e2_params["macro_movement"])  / 2, 2),
            "COUPLING":  round(e1_params["macro_coupling"], 2),
            "SPACE":     round((e1_params["macro_space"]     + e2_params["macro_space"])     / 2, 2),
        },
        "tags": ["entangled", "coupling", engine1.lower(), engine2.lower()],
        "author": AUTHOR,
        "created": TODAY,
    }


def main():
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    seen_names = set()
    count = 0
    skipped = 0

    for row in PRESETS:
        engine1, engine2, name, dna, e1_params, e2_params, coupling_type = row

        # Deduplicate names
        base = name
        suffix = 2
        while name in seen_names:
            name = f"{base} {suffix}"
            suffix += 1
        seen_names.add(name)

        preset = build_preset(engine1, engine2, name, dna, e1_params, e2_params, coupling_type)
        filename = to_snake(name) + ".xometa"
        path = os.path.join(OUTPUT_DIR, filename)

        if os.path.exists(path):
            print(f"  [skip]  {filename}")
            skipped += 1
            continue

        with open(path, "w", encoding="utf-8") as f:
            json.dump(preset, f, indent=2, ensure_ascii=False)
            f.write("\n")

        count += 1
        print(f"  [{count:02d}] {engine1} x {engine2}: {name}")

    print(f"\nDone. {count} new presets written, {skipped} skipped (already exist).")
    print(f"Output: {OUTPUT_DIR}")


if __name__ == "__main__":
    main()
