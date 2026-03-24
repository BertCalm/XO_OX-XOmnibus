#!/usr/bin/env python3
"""
xpn_flux_dna_expansion_pack.py
Generate 60 Flux mood presets with extreme DNA values to push diversity score.

Sub-themes (15 each):
  high-flux:   movement>=0.88, aggression>=0.75, density>=0.70
  cold-flux:   movement>=0.75, brightness<=0.18, warmth<=0.20
  dense-flux:  density>=0.88, aggression>=0.70, space<=0.15
  sparse-flux: density<=0.12, space>=0.82, movement>=0.65

Every preset must have at least 2 dimensions <=0.15 or >=0.85.
"""

import json
import os
import sys

OUTPUT_DIR = os.path.join(
    os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
    "Presets", "XOlokun", "Flux"
)

# Engine name -> macro parameter key prefix mapping
ENGINE_MACRO_KEYS = {
    "ONSET":      {"macro_character": 0.0, "macro_movement": 0.0, "macro_coupling": 0.0, "macro_space": 0.0},
    "OBLONG":     {"macro_character": 0.0, "macro_movement": 0.0, "macro_coupling": 0.0, "macro_space": 0.0},
    "OUROBOROS":  {"macro_character": 0.0, "macro_movement": 0.0, "macro_coupling": 0.0, "macro_space": 0.0},
    "OPTIC":      {"macro_character": 0.0, "macro_movement": 0.0, "macro_coupling": 0.0, "macro_space": 0.0},
    "OVERWORLD":  {"macro_character": 0.0, "macro_movement": 0.0, "macro_coupling": 0.0, "macro_space": 0.0},
    "OCTOPUS":    {"macro_character": 0.0, "macro_movement": 0.0, "macro_coupling": 0.0, "macro_space": 0.0},
    "OUTWIT":     {"macro_character": 0.0, "macro_movement": 0.0, "macro_coupling": 0.0, "macro_space": 0.0},
    "OBSIDIAN":   {"macro_character": 0.0, "macro_movement": 0.0, "macro_coupling": 0.0, "macro_space": 0.0},
    "OBSCURA":    {"macro_character": 0.0, "macro_movement": 0.0, "macro_coupling": 0.0, "macro_space": 0.0},
    "OMBRE":      {"macro_character": 0.0, "macro_movement": 0.0, "macro_coupling": 0.0, "macro_space": 0.0},
    "ORACLE":     {"macro_character": 0.0, "macro_movement": 0.0, "macro_coupling": 0.0, "macro_space": 0.0},
    "ODDOSCAR":   {"macro_character": 0.0, "macro_movement": 0.0, "macro_coupling": 0.0, "macro_space": 0.0},
    "OBESE":      {"macro_character": 0.0, "macro_movement": 0.0, "macro_coupling": 0.0, "macro_space": 0.0},
    "OVERBITE":   {"macro_character": 0.0, "macro_movement": 0.0, "macro_coupling": 0.0, "macro_space": 0.0},
    "ORIGAMI":    {"macro_character": 0.0, "macro_movement": 0.0, "macro_coupling": 0.0, "macro_space": 0.0},
    "ORBITAL":    {"macro_character": 0.0, "macro_movement": 0.0, "macro_coupling": 0.0, "macro_space": 0.0},
    "ORGANON":    {"macro_character": 0.0, "macro_movement": 0.0, "macro_coupling": 0.0, "macro_space": 0.0},
    "OPAL":       {"macro_character": 0.0, "macro_movement": 0.0, "macro_coupling": 0.0, "macro_space": 0.0},
    "ODYSSEY":    {"macro_character": 0.0, "macro_movement": 0.0, "macro_coupling": 0.0, "macro_space": 0.0},
    "ORPHICA":    {"macro_character": 0.0, "macro_movement": 0.0, "macro_coupling": 0.0, "macro_space": 0.0},
    "OVERDUB":    {"macro_character": 0.0, "macro_movement": 0.0, "macro_coupling": 0.0, "macro_space": 0.0},
    "OHM":        {"macro_character": 0.0, "macro_movement": 0.0, "macro_coupling": 0.0, "macro_space": 0.0},
    "OBBLIGATO":  {"macro_character": 0.0, "macro_movement": 0.0, "macro_coupling": 0.0, "macro_space": 0.0},
    "OTTONI":     {"macro_character": 0.0, "macro_movement": 0.0, "macro_coupling": 0.0, "macro_space": 0.0},
    "OLE":        {"macro_character": 0.0, "macro_movement": 0.0, "macro_coupling": 0.0, "macro_space": 0.0},
    "OVERLAP":    {"macro_character": 0.0, "macro_movement": 0.0, "macro_coupling": 0.0, "macro_space": 0.0},
}

def make_preset(name, engine, dna, macros, tags, description, coupling_intensity="None", coupling_pairs=None):
    """Build a preset dict matching exact schema from existing Flux presets."""
    engine_title = engine.title()
    # Special casing for engines with non-standard title
    TITLE_MAP = {
        "ODDOSCAR": "OddOscar",
        "OUROBOROS": "Ouroboros",
        "OVERWORLD": "Overworld",
        "OVERDUB": "Overdub",
        "OVERBITE": "Overbite",
        "ORIGAMI": "Origami",
        "ORBITAL": "Orbital",
        "ORGANON": "Organon",
        "OBSIDIAN": "Obsidian",
        "OBSCURA": "Obscura",
        "OCTOPUS": "Octopus",
        "ODYSSEY": "Odyssey",
        "ORPHICA": "Orphica",
        "OUTWIT": "Outwit",
        "OVERLAP": "Overlap",
        "ORACLE": "Oracle",
        "OBLONG": "Oblong",
        "OBESE": "Obese",
        "OPTIC": "Optic",
        "OMBRE": "Ombre",
        "ONSET": "Onset",
        "OPAL": "Opal",
        "OHM": "Ohm",
        "OLE": "Ole",
        "OBBLIGATO": "Obbligato",
        "OTTONI": "Ottoni",
    }
    engine_display = TITLE_MAP.get(engine, engine.title())

    return {
        "schema_version": 1,
        "author": "XO_OX Designs",
        "version": "1.0",
        "name": name,
        "mood": "Flux",
        "engines": [engine_display],
        "couplingIntensity": coupling_intensity,
        "tempo": None,
        "macroLabels": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
        "macros": macros,
        "coupling": {"pairs": coupling_pairs or []},
        "sequencer": None,
        "dna": dna,
        "parameters": {},
        "tags": tags,
        "description": description,
    }


# ---------------------------------------------------------------------------
# HIGH-FLUX (15 presets): movement>=0.88, aggression>=0.75, density>=0.70
# ---------------------------------------------------------------------------
HIGH_FLUX = [
    make_preset(
        "Riot Pulse",
        "ONSET",
        {"brightness": 0.72, "warmth": 0.38, "movement": 0.95, "density": 0.88, "space": 0.08, "aggression": 0.92},
        {"CHARACTER": 0.88, "MOVEMENT": 0.95, "COUPLING": 0.22, "SPACE": 0.08},
        ["flux", "high-flux", "onset", "riot", "percussion"],
        "Onset drums in full riot — every hit overlapping, rhythm dissolving into noise."
    ),
    make_preset(
        "Chainsaw Lattice",
        "OBLONG",
        {"brightness": 0.85, "warmth": 0.28, "movement": 0.92, "density": 0.78, "space": 0.10, "aggression": 0.89},
        {"CHARACTER": 0.92, "MOVEMENT": 0.92, "COUPLING": 0.18, "SPACE": 0.10},
        ["flux", "high-flux", "oblong", "chainsaw", "harsh"],
        "Oblong chord engine razor-edged — bright lattice of stacked aggression."
    ),
    make_preset(
        "Vortex Spawn",
        "OUROBOROS",
        {"brightness": 0.65, "warmth": 0.22, "movement": 0.91, "density": 0.82, "space": 0.09, "aggression": 0.87},
        {"CHARACTER": 0.85, "MOVEMENT": 0.91, "COUPLING": 0.75, "SPACE": 0.09},
        ["flux", "high-flux", "ouroboros", "vortex", "feedback"],
        "Ouroboros feedback snake consuming itself — self-modulating vortex."
    ),
    make_preset(
        "Optic Seizure",
        "OPTIC",
        {"brightness": 0.91, "warmth": 0.15, "movement": 0.94, "density": 0.71, "space": 0.12, "aggression": 0.88},
        {"CHARACTER": 0.91, "MOVEMENT": 0.94, "COUPLING": 0.30, "SPACE": 0.12},
        ["flux", "high-flux", "optic", "strobe", "seizure"],
        "Optic visual synth — stroboscopic brightness at seizure threshold."
    ),
    make_preset(
        "8-Bit Avalanche",
        "OVERWORLD",
        {"brightness": 0.78, "warmth": 0.32, "movement": 0.90, "density": 0.86, "space": 0.07, "aggression": 0.82},
        {"CHARACTER": 0.82, "MOVEMENT": 0.90, "COUPLING": 0.25, "SPACE": 0.07},
        ["flux", "high-flux", "overworld", "chiptune", "avalanche"],
        "Overworld chip engine cascading — NES/Genesis layers pile into avalanche."
    ),
    make_preset(
        "Tentacle Storm",
        "OCTOPUS",
        {"brightness": 0.58, "warmth": 0.25, "movement": 0.93, "density": 0.85, "space": 0.08, "aggression": 0.90},
        {"CHARACTER": 0.88, "MOVEMENT": 0.93, "COUPLING": 0.88, "SPACE": 0.08},
        ["flux", "high-flux", "octopus", "storm", "tentacle"],
        "Octopus 8-arm crossfade in full storm — every arm thrashing simultaneously."
    ),
    make_preset(
        "Wolfram Burst",
        "OUTWIT",
        {"brightness": 0.62, "warmth": 0.18, "movement": 0.89, "density": 0.88, "space": 0.06, "aggression": 0.86},
        {"CHARACTER": 0.86, "MOVEMENT": 0.89, "COUPLING": 0.55, "SPACE": 0.06},
        ["flux", "high-flux", "outwit", "cellular-automata", "burst"],
        "Outwit CA rules firing simultaneously — Wolfram Rule 110 at maximum density."
    ),
    make_preset(
        "Plasma Whip",
        "ONSET",
        {"brightness": 0.88, "warmth": 0.20, "movement": 0.96, "density": 0.73, "space": 0.10, "aggression": 0.94},
        {"CHARACTER": 0.94, "MOVEMENT": 0.96, "COUPLING": 0.15, "SPACE": 0.10},
        ["flux", "high-flux", "onset", "plasma", "whip"],
        "Hi-hat velocity layers cracking like plasma whip — lightning percussion."
    ),
    make_preset(
        "Fuzz Torrent",
        "OVERBITE",
        {"brightness": 0.74, "warmth": 0.35, "movement": 0.88, "density": 0.90, "space": 0.08, "aggression": 0.95},
        {"CHARACTER": 0.95, "MOVEMENT": 0.88, "COUPLING": 0.20, "SPACE": 0.08},
        ["flux", "high-flux", "overbite", "fuzz", "torrent"],
        "Overbite bass filter wide open — fuzz distortion torrenting through the mix."
    ),
    make_preset(
        "Fractal Panic",
        "OUROBOROS",
        {"brightness": 0.80, "warmth": 0.12, "movement": 0.93, "density": 0.76, "space": 0.09, "aggression": 0.85},
        {"CHARACTER": 0.80, "MOVEMENT": 0.93, "COUPLING": 0.90, "SPACE": 0.09},
        ["flux", "high-flux", "ouroboros", "fractal", "panic"],
        "Ouroboros self-referential feedback at fractal panic state."
    ),
    make_preset(
        "Grid Collapse",
        "OVERWORLD",
        {"brightness": 0.70, "warmth": 0.28, "movement": 0.91, "density": 0.83, "space": 0.07, "aggression": 0.88},
        {"CHARACTER": 0.85, "MOVEMENT": 0.91, "COUPLING": 0.35, "SPACE": 0.07},
        ["flux", "high-flux", "overworld", "grid", "collapse"],
        "Overworld era crossfade mid-collapse — all three chip eras failing together."
    ),
    make_preset(
        "Arm Race",
        "OUTWIT",
        {"brightness": 0.55, "warmth": 0.22, "movement": 0.94, "density": 0.92, "space": 0.05, "aggression": 0.91},
        {"CHARACTER": 0.91, "MOVEMENT": 0.94, "COUPLING": 0.68, "SPACE": 0.05},
        ["flux", "high-flux", "outwit", "arms", "escalation"],
        "Outwit 8-arm CA in arms race — all rules escalating toward maximum complexity."
    ),
    make_preset(
        "Kinetic Riot",
        "OPTIC",
        {"brightness": 0.86, "warmth": 0.16, "movement": 0.90, "density": 0.74, "space": 0.11, "aggression": 0.83},
        {"CHARACTER": 0.83, "MOVEMENT": 0.90, "COUPLING": 0.40, "SPACE": 0.11},
        ["flux", "high-flux", "optic", "kinetic", "riot"],
        "Optic scanner in kinetic riot — visual data streams colliding."
    ),
    make_preset(
        "Crunch Wall",
        "OBLONG",
        {"brightness": 0.68, "warmth": 0.30, "movement": 0.89, "density": 0.87, "space": 0.06, "aggression": 0.93},
        {"CHARACTER": 0.93, "MOVEMENT": 0.89, "COUPLING": 0.28, "SPACE": 0.06},
        ["flux", "high-flux", "oblong", "crunch", "wall"],
        "Oblong chord stacks compressed into a crunch wall — harmonic saturation peak."
    ),
    make_preset(
        "Surge Protocol",
        "OCTOPUS",
        {"brightness": 0.75, "warmth": 0.20, "movement": 0.92, "density": 0.80, "space": 0.07, "aggression": 0.86},
        {"CHARACTER": 0.86, "MOVEMENT": 0.92, "COUPLING": 0.82, "SPACE": 0.07},
        ["flux", "high-flux", "octopus", "surge", "protocol"],
        "Octopus surge protocol engaged — all 8 arms firing in burst sequence."
    ),
]

# ---------------------------------------------------------------------------
# COLD-FLUX (15 presets): movement>=0.75, brightness<=0.18, warmth<=0.20
# ---------------------------------------------------------------------------
COLD_FLUX = [
    make_preset(
        "Frozen Current",
        "OBSIDIAN",
        {"brightness": 0.12, "warmth": 0.10, "movement": 0.88, "density": 0.55, "space": 0.62, "aggression": 0.45},
        {"CHARACTER": 0.45, "MOVEMENT": 0.88, "COUPLING": 0.20, "SPACE": 0.62},
        ["flux", "cold-flux", "obsidian", "frozen", "current"],
        "Obsidian dark matter in motion — cold current through absolute zero."
    ),
    make_preset(
        "Arctic Drift",
        "OBSCURA",
        {"brightness": 0.08, "warmth": 0.14, "movement": 0.82, "density": 0.45, "space": 0.72, "aggression": 0.35},
        {"CHARACTER": 0.35, "MOVEMENT": 0.82, "COUPLING": 0.15, "SPACE": 0.72},
        ["flux", "cold-flux", "obscura", "arctic", "drift"],
        "Obscura granular textures drifting across arctic silence."
    ),
    make_preset(
        "Liquid Nitrogen",
        "OMBRE",
        {"brightness": 0.15, "warmth": 0.08, "movement": 0.86, "density": 0.60, "space": 0.55, "aggression": 0.52},
        {"CHARACTER": 0.52, "MOVEMENT": 0.86, "COUPLING": 0.30, "SPACE": 0.55},
        ["flux", "cold-flux", "ombre", "nitrogen", "liquid"],
        "Ombre gradient locked at coldest temperature — liquid nitrogen flow."
    ),
    make_preset(
        "Zero Kelvin Motion",
        "ORACLE",
        {"brightness": 0.10, "warmth": 0.12, "movement": 0.91, "density": 0.35, "space": 0.82, "aggression": 0.28},
        {"CHARACTER": 0.28, "MOVEMENT": 0.91, "COUPLING": 0.08, "SPACE": 0.82},
        ["flux", "cold-flux", "oracle", "zero", "kelvin"],
        "Oracle prediction engine at absolute zero — cold motion without heat."
    ),
    make_preset(
        "Cryogenic Pulse",
        "ODDOSCAR",
        {"brightness": 0.14, "warmth": 0.16, "movement": 0.85, "density": 0.48, "space": 0.68, "aggression": 0.40},
        {"CHARACTER": 0.40, "MOVEMENT": 0.85, "COUPLING": 0.25, "SPACE": 0.68},
        ["flux", "cold-flux", "oddoscar", "cryogenic", "pulse"],
        "OddOscar cold oscillator pair — cryogenic pulse through frozen tubes."
    ),
    make_preset(
        "Polar Vortex",
        "OBSIDIAN",
        {"brightness": 0.06, "warmth": 0.09, "movement": 0.93, "density": 0.62, "space": 0.58, "aggression": 0.55},
        {"CHARACTER": 0.55, "MOVEMENT": 0.93, "COUPLING": 0.35, "SPACE": 0.58},
        ["flux", "cold-flux", "obsidian", "polar", "vortex"],
        "Obsidian in polar vortex — dark kinetic mass spinning without warmth."
    ),
    make_preset(
        "Ice Erosion",
        "OMBRE",
        {"brightness": 0.09, "warmth": 0.14, "movement": 0.86, "density": 0.38, "space": 0.85, "aggression": 0.28},
        {"CHARACTER": 0.28, "MOVEMENT": 0.86, "COUPLING": 0.12, "SPACE": 0.85},
        ["flux", "cold-flux", "ombre", "ice", "erosion"],
        "Ombre gradient carved by cold — slow ice erosion over deep open time."
    ),
    make_preset(
        "Thermal Null",
        "ORACLE",
        {"brightness": 0.16, "warmth": 0.07, "movement": 0.87, "density": 0.42, "space": 0.75, "aggression": 0.38},
        {"CHARACTER": 0.38, "MOVEMENT": 0.87, "COUPLING": 0.18, "SPACE": 0.75},
        ["flux", "cold-flux", "oracle", "thermal", "null"],
        "Oracle at thermal null — movement persisting where all heat has fled."
    ),
    make_preset(
        "Subzero Engine",
        "OBSCURA",
        {"brightness": 0.09, "warmth": 0.13, "movement": 0.84, "density": 0.52, "space": 0.65, "aggression": 0.48},
        {"CHARACTER": 0.48, "MOVEMENT": 0.84, "COUPLING": 0.22, "SPACE": 0.65},
        ["flux", "cold-flux", "obscura", "subzero", "engine"],
        "Obscura granular engine running subzero — cold spectral motion."
    ),
    make_preset(
        "Black Ice Run",
        "ODDOSCAR",
        {"brightness": 0.13, "warmth": 0.11, "movement": 0.90, "density": 0.44, "space": 0.70, "aggression": 0.42},
        {"CHARACTER": 0.42, "MOVEMENT": 0.90, "COUPLING": 0.28, "SPACE": 0.70},
        ["flux", "cold-flux", "oddoscar", "black-ice", "run"],
        "OddOscar oscillator pair sliding on black ice — dangerous cold velocity."
    ),
    make_preset(
        "Permafrost Shift",
        "OBSIDIAN",
        {"brightness": 0.07, "warmth": 0.15, "movement": 0.77, "density": 0.58, "space": 0.62, "aggression": 0.38},
        {"CHARACTER": 0.38, "MOVEMENT": 0.77, "COUPLING": 0.15, "SPACE": 0.62},
        ["flux", "cold-flux", "obsidian", "permafrost", "shift"],
        "Obsidian permafrost beginning to shift — cold slow movement under pressure."
    ),
    make_preset(
        "Tundra Signal",
        "OMBRE",
        {"brightness": 0.13, "warmth": 0.10, "movement": 0.88, "density": 0.40, "space": 0.87, "aggression": 0.14},
        {"CHARACTER": 0.14, "MOVEMENT": 0.88, "COUPLING": 0.10, "SPACE": 0.87},
        ["flux", "cold-flux", "ombre", "tundra", "signal"],
        "Ombre tundra gradient — cold sparse signal moving across open plains."
    ),
    make_preset(
        "Frost Protocol",
        "ORACLE",
        {"brightness": 0.05, "warmth": 0.18, "movement": 0.89, "density": 0.50, "space": 0.68, "aggression": 0.44},
        {"CHARACTER": 0.44, "MOVEMENT": 0.89, "COUPLING": 0.20, "SPACE": 0.68},
        ["flux", "cold-flux", "oracle", "frost", "protocol"],
        "Oracle frost protocol — prediction lattice crystallizing in cold motion."
    ),
    make_preset(
        "Cryo Cascade",
        "OBSCURA",
        {"brightness": 0.12, "warmth": 0.14, "movement": 0.92, "density": 0.56, "space": 0.60, "aggression": 0.50},
        {"CHARACTER": 0.50, "MOVEMENT": 0.92, "COUPLING": 0.32, "SPACE": 0.60},
        ["flux", "cold-flux", "obscura", "cryo", "cascade"],
        "Obscura granular cryo cascade — frozen particles streaming in cold flux."
    ),
    make_preset(
        "Helium Surge",
        "ODDOSCAR",
        {"brightness": 0.08, "warmth": 0.20, "movement": 0.86, "density": 0.46, "space": 0.73, "aggression": 0.36},
        {"CHARACTER": 0.36, "MOVEMENT": 0.86, "COUPLING": 0.16, "SPACE": 0.73},
        ["flux", "cold-flux", "oddoscar", "helium", "surge"],
        "OddOscar at helium temperature — superfluid motion without resistance."
    ),
]

# ---------------------------------------------------------------------------
# DENSE-FLUX (15 presets): density>=0.88, aggression>=0.70, space<=0.15
# ---------------------------------------------------------------------------
DENSE_FLUX = [
    make_preset(
        "Mass Collapse",
        "OBESE",
        {"brightness": 0.55, "warmth": 0.60, "movement": 0.65, "density": 0.96, "space": 0.05, "aggression": 0.88},
        {"CHARACTER": 0.88, "MOVEMENT": 0.65, "COUPLING": 0.45, "SPACE": 0.05},
        ["flux", "dense-flux", "obese", "mass", "collapse"],
        "Obese at maximum density — gravitational collapse of all harmonic matter."
    ),
    make_preset(
        "Root Pressure",
        "OVERBITE",
        {"brightness": 0.42, "warmth": 0.68, "movement": 0.72, "density": 0.92, "space": 0.06, "aggression": 0.90},
        {"CHARACTER": 0.90, "MOVEMENT": 0.72, "COUPLING": 0.35, "SPACE": 0.06},
        ["flux", "dense-flux", "overbite", "root", "pressure"],
        "Overbite root frequencies under maximum pressure — tectonic bass density."
    ),
    make_preset(
        "Paper Fold Crush",
        "ORIGAMI",
        {"brightness": 0.48, "warmth": 0.52, "movement": 0.68, "density": 0.90, "space": 0.08, "aggression": 0.82},
        {"CHARACTER": 0.82, "MOVEMENT": 0.68, "COUPLING": 0.55, "SPACE": 0.08},
        ["flux", "dense-flux", "origami", "paper", "crush"],
        "Origami fold geometry at maximum density — paper crushed into singularity."
    ),
    make_preset(
        "Orbital Debris",
        "ORBITAL",
        {"brightness": 0.60, "warmth": 0.40, "movement": 0.75, "density": 0.94, "space": 0.07, "aggression": 0.85},
        {"CHARACTER": 0.85, "MOVEMENT": 0.75, "COUPLING": 0.62, "SPACE": 0.07},
        ["flux", "dense-flux", "orbital", "debris", "field"],
        "Orbital ring collapsed — dense debris field with no clear space between particles."
    ),
    make_preset(
        "Harmonic Compression",
        "ORGANON",
        {"brightness": 0.65, "warmth": 0.55, "movement": 0.70, "density": 0.91, "space": 0.08, "aggression": 0.78},
        {"CHARACTER": 0.78, "MOVEMENT": 0.70, "COUPLING": 0.50, "SPACE": 0.08},
        ["flux", "dense-flux", "organon", "harmonic", "compression"],
        "Organon logic engine under harmonic compression — syllogism stack overflow."
    ),
    make_preset(
        "Sub Density",
        "OBESE",
        {"brightness": 0.38, "warmth": 0.72, "movement": 0.62, "density": 0.97, "space": 0.04, "aggression": 0.86},
        {"CHARACTER": 0.86, "MOVEMENT": 0.62, "COUPLING": 0.40, "SPACE": 0.04},
        ["flux", "dense-flux", "obese", "sub", "density"],
        "Obese sub-bass at extreme density — compressed low-end at crushing pressure."
    ),
    make_preset(
        "Bite Compression",
        "OVERBITE",
        {"brightness": 0.50, "warmth": 0.62, "movement": 0.78, "density": 0.93, "space": 0.06, "aggression": 0.92},
        {"CHARACTER": 0.92, "MOVEMENT": 0.78, "COUPLING": 0.28, "SPACE": 0.06},
        ["flux", "dense-flux", "overbite", "bite", "compression"],
        "Overbite maximum bite — all frequency bands compressed into dense mass."
    ),
    make_preset(
        "Fold Stack",
        "ORIGAMI",
        {"brightness": 0.52, "warmth": 0.48, "movement": 0.74, "density": 0.89, "space": 0.09, "aggression": 0.75},
        {"CHARACTER": 0.75, "MOVEMENT": 0.74, "COUPLING": 0.60, "SPACE": 0.09},
        ["flux", "dense-flux", "origami", "fold", "stack"],
        "Origami fold-stack maximum — 64 simultaneous fold geometries in dense lattice."
    ),
    make_preset(
        "Ring Crush",
        "ORBITAL",
        {"brightness": 0.58, "warmth": 0.45, "movement": 0.80, "density": 0.95, "space": 0.05, "aggression": 0.87},
        {"CHARACTER": 0.87, "MOVEMENT": 0.80, "COUPLING": 0.70, "SPACE": 0.05},
        ["flux", "dense-flux", "orbital", "ring", "crush"],
        "Orbital ring crush — circumference shrinking, density increasing to critical."
    ),
    make_preset(
        "Logic Saturation",
        "ORGANON",
        {"brightness": 0.62, "warmth": 0.50, "movement": 0.66, "density": 0.92, "space": 0.07, "aggression": 0.80},
        {"CHARACTER": 0.80, "MOVEMENT": 0.66, "COUPLING": 0.55, "SPACE": 0.07},
        ["flux", "dense-flux", "organon", "logic", "saturation"],
        "Organon logic saturation — all propositions true simultaneously."
    ),
    make_preset(
        "Gravity Well",
        "OBESE",
        {"brightness": 0.45, "warmth": 0.65, "movement": 0.68, "density": 0.98, "space": 0.03, "aggression": 0.84},
        {"CHARACTER": 0.84, "MOVEMENT": 0.68, "COUPLING": 0.38, "SPACE": 0.03},
        ["flux", "dense-flux", "obese", "gravity", "well"],
        "Obese gravity well — all surrounding sound drawn into dense singularity."
    ),
    make_preset(
        "Compression Event",
        "OVERBITE",
        {"brightness": 0.46, "warmth": 0.58, "movement": 0.76, "density": 0.91, "space": 0.08, "aggression": 0.89},
        {"CHARACTER": 0.89, "MOVEMENT": 0.76, "COUPLING": 0.32, "SPACE": 0.08},
        ["flux", "dense-flux", "overbite", "compression", "event"],
        "Overbite compression event — all bass frequencies collapsing inward."
    ),
    make_preset(
        "Dense Matter",
        "ORIGAMI",
        {"brightness": 0.54, "warmth": 0.44, "movement": 0.71, "density": 0.88, "space": 0.10, "aggression": 0.76},
        {"CHARACTER": 0.76, "MOVEMENT": 0.71, "COUPLING": 0.58, "SPACE": 0.10},
        ["flux", "dense-flux", "origami", "dense", "matter"],
        "Origami folded to dense matter — paper geometry approaching neutron star density."
    ),
    make_preset(
        "Orbital Impact",
        "ORBITAL",
        {"brightness": 0.62, "warmth": 0.42, "movement": 0.83, "density": 0.93, "space": 0.06, "aggression": 0.82},
        {"CHARACTER": 0.82, "MOVEMENT": 0.83, "COUPLING": 0.65, "SPACE": 0.06},
        ["flux", "dense-flux", "orbital", "impact", "collision"],
        "Orbital impact event — ring system collision, debris compacted instantly."
    ),
    make_preset(
        "Argument Crush",
        "ORGANON",
        {"brightness": 0.58, "warmth": 0.48, "movement": 0.69, "density": 0.90, "space": 0.09, "aggression": 0.73},
        {"CHARACTER": 0.73, "MOVEMENT": 0.69, "COUPLING": 0.52, "SPACE": 0.09},
        ["flux", "dense-flux", "organon", "argument", "crush"],
        "Organon argument crush — logical structure compressed beyond recognition."
    ),
]

# ---------------------------------------------------------------------------
# SPARSE-FLUX (15 presets): density<=0.12, space>=0.82, movement>=0.65
# ---------------------------------------------------------------------------
SPARSE_FLUX = [
    make_preset(
        "Scattered Light",
        "OPAL",
        {"brightness": 0.75, "warmth": 0.35, "movement": 0.82, "density": 0.08, "space": 0.92, "aggression": 0.22},
        {"CHARACTER": 0.22, "MOVEMENT": 0.82, "COUPLING": 0.12, "SPACE": 0.92},
        ["flux", "sparse-flux", "opal", "scattered", "light"],
        "Opal granular particles scattered through maximum space — light without mass."
    ),
    make_preset(
        "Distant Resonance",
        "ODYSSEY",
        {"brightness": 0.62, "warmth": 0.28, "movement": 0.78, "density": 0.09, "space": 0.95, "aggression": 0.18},
        {"CHARACTER": 0.18, "MOVEMENT": 0.78, "COUPLING": 0.08, "SPACE": 0.95},
        ["flux", "sparse-flux", "odyssey", "distant", "resonance"],
        "Odyssey wavetable in deep space — single resonance moving through void."
    ),
    make_preset(
        "Harp Void",
        "ORPHICA",
        {"brightness": 0.80, "warmth": 0.22, "movement": 0.75, "density": 0.07, "space": 0.93, "aggression": 0.15},
        {"CHARACTER": 0.15, "MOVEMENT": 0.75, "COUPLING": 0.06, "SPACE": 0.93},
        ["flux", "sparse-flux", "orphica", "harp", "void"],
        "Orphica microsound harp in void — single pluck resonating through absolute space."
    ),
    make_preset(
        "Orbital Solitude",
        "ORBITAL",
        {"brightness": 0.55, "warmth": 0.30, "movement": 0.72, "density": 0.10, "space": 0.90, "aggression": 0.20},
        {"CHARACTER": 0.20, "MOVEMENT": 0.72, "COUPLING": 0.10, "SPACE": 0.90},
        ["flux", "sparse-flux", "orbital", "solitude", "ring"],
        "Orbital single ring — one particle traversing vast empty circumference."
    ),
    make_preset(
        "Dub Echo Void",
        "OVERDUB",
        {"brightness": 0.45, "warmth": 0.38, "movement": 0.80, "density": 0.06, "space": 0.96, "aggression": 0.12},
        {"CHARACTER": 0.12, "MOVEMENT": 0.80, "COUPLING": 0.05, "SPACE": 0.96},
        ["flux", "sparse-flux", "overdub", "dub", "void"],
        "Overdub tape echo in maximum space — single repeat dying across infinite room."
    ),
    make_preset(
        "Grain Desert",
        "OPAL",
        {"brightness": 0.68, "warmth": 0.25, "movement": 0.85, "density": 0.05, "space": 0.94, "aggression": 0.16},
        {"CHARACTER": 0.16, "MOVEMENT": 0.85, "COUPLING": 0.08, "SPACE": 0.94},
        ["flux", "sparse-flux", "opal", "grain", "desert"],
        "Opal grains scattered across desert space — rare particles in fast motion."
    ),
    make_preset(
        "Lone Wavetable",
        "ODYSSEY",
        {"brightness": 0.58, "warmth": 0.32, "movement": 0.76, "density": 0.08, "space": 0.91, "aggression": 0.14},
        {"CHARACTER": 0.14, "MOVEMENT": 0.76, "COUPLING": 0.07, "SPACE": 0.91},
        ["flux", "sparse-flux", "odyssey", "lone", "wavetable"],
        "Odyssey lone wavetable scanning through empty frequency space."
    ),
    make_preset(
        "Thread Pull",
        "ORPHICA",
        {"brightness": 0.72, "warmth": 0.20, "movement": 0.88, "density": 0.09, "space": 0.88, "aggression": 0.18},
        {"CHARACTER": 0.18, "MOVEMENT": 0.88, "COUPLING": 0.09, "SPACE": 0.88},
        ["flux", "sparse-flux", "orphica", "thread", "pull"],
        "Orphica single thread pulled through silence — sparse harp gesture."
    ),
    make_preset(
        "Sparse Ring",
        "ORBITAL",
        {"brightness": 0.50, "warmth": 0.28, "movement": 0.70, "density": 0.11, "space": 0.87, "aggression": 0.22},
        {"CHARACTER": 0.22, "MOVEMENT": 0.70, "COUPLING": 0.12, "SPACE": 0.87},
        ["flux", "sparse-flux", "orbital", "sparse", "ring"],
        "Orbital sparse ring — wide circumference with few particles in motion."
    ),
    make_preset(
        "Echo Dispersal",
        "OVERDUB",
        {"brightness": 0.42, "warmth": 0.35, "movement": 0.83, "density": 0.07, "space": 0.93, "aggression": 0.10},
        {"CHARACTER": 0.10, "MOVEMENT": 0.83, "COUPLING": 0.06, "SPACE": 0.93},
        ["flux", "sparse-flux", "overdub", "echo", "dispersal"],
        "Overdub echo dispersal — sound dissolving into space, barely traceable."
    ),
    make_preset(
        "Crystal Scatter",
        "OPAL",
        {"brightness": 0.88, "warmth": 0.18, "movement": 0.79, "density": 0.10, "space": 0.89, "aggression": 0.12},
        {"CHARACTER": 0.12, "MOVEMENT": 0.79, "COUPLING": 0.10, "SPACE": 0.89},
        ["flux", "sparse-flux", "opal", "crystal", "scatter"],
        "Opal crystal grains scattered bright — high-frequency particles in open field."
    ),
    make_preset(
        "Odyssey Drift",
        "ODYSSEY",
        {"brightness": 0.65, "warmth": 0.24, "movement": 0.73, "density": 0.06, "space": 0.92, "aggression": 0.15},
        {"CHARACTER": 0.15, "MOVEMENT": 0.73, "COUPLING": 0.08, "SPACE": 0.92},
        ["flux", "sparse-flux", "odyssey", "drift", "sparse"],
        "Odyssey wavetable adrift in open space — minimal presence, maximum journey."
    ),
    make_preset(
        "Harp Static",
        "ORPHICA",
        {"brightness": 0.76, "warmth": 0.15, "movement": 0.77, "density": 0.08, "space": 0.90, "aggression": 0.14},
        {"CHARACTER": 0.14, "MOVEMENT": 0.77, "COUPLING": 0.07, "SPACE": 0.90},
        ["flux", "sparse-flux", "orphica", "harp", "static"],
        "Orphica harp static — microsound particles barely disturbing the silence."
    ),
    make_preset(
        "Void Pulse",
        "OVERDUB",
        {"brightness": 0.38, "warmth": 0.28, "movement": 0.91, "density": 0.05, "space": 0.97, "aggression": 0.08},
        {"CHARACTER": 0.08, "MOVEMENT": 0.91, "COUPLING": 0.04, "SPACE": 0.97},
        ["flux", "sparse-flux", "overdub", "void", "pulse"],
        "Overdub void pulse — single tape echo in absolute maximum space."
    ),
    make_preset(
        "Open Signal",
        "OPAL",
        {"brightness": 0.70, "warmth": 0.22, "movement": 0.86, "density": 0.12, "space": 0.85, "aggression": 0.20},
        {"CHARACTER": 0.20, "MOVEMENT": 0.86, "COUPLING": 0.11, "SPACE": 0.85},
        ["flux", "sparse-flux", "opal", "open", "signal"],
        "Opal open signal — granular emission spreading freely through maximum space."
    ),
]

ALL_PRESETS = HIGH_FLUX + COLD_FLUX + DENSE_FLUX + SPARSE_FLUX


def validate_preset(preset):
    """Verify extreme DNA constraint: at least 2 dimensions <=0.15 or >=0.85."""
    dna = preset["dna"]
    dims = list(dna.values())
    extreme_count = sum(1 for v in dims if v <= 0.15 or v >= 0.85)
    if extreme_count < 2:
        raise ValueError(
            f"Preset '{preset['name']}' only has {extreme_count} extreme dimensions. Need >= 2."
        )
    return True


def save_preset(preset, output_dir):
    """Save preset JSON to output_dir. Skip if file exists."""
    filename = preset["name"].replace(" ", "_").replace("/", "-") + ".xometa"
    filepath = os.path.join(output_dir, filename)
    if os.path.exists(filepath):
        return False, filepath
    with open(filepath, "w", encoding="utf-8") as f:
        json.dump(preset, f, indent=2)
        f.write("\n")
    return True, filepath


def main():
    os.makedirs(OUTPUT_DIR, exist_ok=True)

    saved = 0
    skipped = 0
    errors = 0

    print(f"Output directory: {OUTPUT_DIR}")
    print(f"Total presets to process: {len(ALL_PRESETS)}")
    print()

    sub_counts = {"high-flux": 0, "cold-flux": 0, "dense-flux": 0, "sparse-flux": 0}

    for preset in ALL_PRESETS:
        try:
            validate_preset(preset)
        except ValueError as e:
            print(f"  VALIDATION ERROR: {e}")
            errors += 1
            continue

        ok, filepath = save_preset(preset, OUTPUT_DIR)
        fname = os.path.basename(filepath)

        # Determine sub-theme for reporting
        sub = next((t for t in preset["tags"] if t in sub_counts), "unknown")

        if ok:
            saved += 1
            sub_counts[sub] = sub_counts.get(sub, 0) + 1
            print(f"  SAVED    [{sub:12s}] {preset['name']}")
        else:
            skipped += 1
            print(f"  SKIPPED  [{sub:12s}] {preset['name']} (exists)")

    print()
    print("=" * 60)
    print(f"Results: {saved} saved, {skipped} skipped, {errors} errors")
    print()
    print("Sub-theme breakdown (saved):")
    for sub, count in sub_counts.items():
        print(f"  {sub:15s}: {count}")
    print()

    if errors > 0:
        print("FAILED: validation errors found.")
        sys.exit(1)
    else:
        print("SUCCESS: All Flux DNA expansion presets generated.")


if __name__ == "__main__":
    main()
