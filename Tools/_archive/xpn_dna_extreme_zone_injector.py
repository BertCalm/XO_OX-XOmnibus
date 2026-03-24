#!/usr/bin/env python3
"""
xpn_dna_extreme_zone_injector.py
Generate 80 extreme-zone presets anchoring the 8 corners of the 6D DNA hypercube.
Targets: brightness, warmth, movement, density, space, aggression

Corners:
  C1 Hot Dense       — brightness≥0.85, warmth≥0.85, density≥0.85, aggression≥0.75  → Prism
  C2 Cold Sparse     — brightness≤0.15, warmth≤0.15, density≤0.15, aggression≤0.15  → Aether
  C3 Hot Sparse      — brightness≥0.85, warmth≥0.85, density≤0.15, space≥0.85       → Prism
  C4 Cold Dense      — brightness≤0.15, warmth≤0.15, density≥0.85, aggression≥0.75  → Aether
  C5 Kinetic Bright  — movement≥0.9, brightness≥0.8, space≤0.2, density≥0.7         → Prism
  C6 Still Dark      — movement≤0.1, brightness≤0.2, warmth≤0.2, space≥0.8          → Aether
  C7 Aggressive Sparse — aggression≥0.9, density≤0.2, brightness≥0.7, movement≥0.7 → Prism
  C8 Warm Deep Space — warmth≥0.85, space≥0.9, movement≥0.6, aggression≤0.2         → Aether
"""

import json
import os
import re
from datetime import date

TODAY = date.today().isoformat()
PRESETS_ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "Presets", "XOlokun")


def snake(name: str) -> str:
    return re.sub(r"[^a-zA-Z0-9]+", "_", name).strip("_")


def write_preset(mood: str, preset: dict):
    folder = os.path.join(PRESETS_ROOT, mood)
    os.makedirs(folder, exist_ok=True)
    filename = snake(preset["name"]) + ".xometa"
    path = os.path.join(folder, filename)
    with open(path, "w") as f:
        json.dump(preset, f, indent=2)
    return path


def make_preset(name, mood, engine, description, tags, params, dna, macro_labels=None):
    if macro_labels is None:
        macro_labels = ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"]
    return {
        "schema_version": 1,
        "name": name,
        "mood": mood,
        "engines": [engine],
        "author": "XO_OX",
        "version": "1.0.0",
        "description": description,
        "tags": tags,
        "macroLabels": macro_labels,
        "couplingIntensity": "None",
        "tempo": None,
        "created": TODAY,
        "legacy": {"sourceInstrument": None, "sourceCategory": None, "sourcePresetName": None},
        "parameters": {engine: params},
        "coupling": None,
        "sequencer": None,
        "dna": dna,
    }


# ---------------------------------------------------------------------------
# CORNER 1 — Hot Dense (Prism)
# brightness≥0.85, warmth≥0.85, density≥0.85, aggression≥0.75
# ---------------------------------------------------------------------------

c1_presets = [
    make_preset(
        "Magma Core",
        "Prism",
        "Overdub",
        "Molten density compressed to a singularity. Heat without mercy.",
        ["hot-dense", "extreme", "prism", "aggressive"],
        {"dub_oscWave": 3.0, "dub_oscOctave": 0.0, "dub_oscTune": 0.0, "dub_drive": 0.95,
         "dub_filterCutoff": 3800.0, "dub_filterReso": 0.82, "dub_filterEnvAmt": 0.88,
         "dub_attack": 2.0, "dub_decay": 180.0, "dub_sustain": 0.9, "dub_release": 120.0,
         "dub_distortion": 0.9, "dub_level": -2.0},
        {"brightness": 0.91, "warmth": 0.88, "movement": 0.65, "density": 0.92, "space": 0.08, "aggression": 0.87},
    ),
    make_preset(
        "Solar Flare Burst",
        "Prism",
        "Obese",
        "Subterranean pressure venting in a wall of saturated harmonics.",
        ["hot-dense", "extreme", "prism", "saturated"],
        {"fat_morph": 0.92, "fat_mojo": 0.88, "fat_subLevel": 0.85,
         "fat_filterCutoff": 4200.0, "fat_drive": 0.9, "fat_crush": 0.78,
         "fat_attack": 5.0, "fat_decay": 220.0, "fat_sustain": 0.88, "fat_release": 150.0},
        {"brightness": 0.88, "warmth": 0.91, "movement": 0.7, "density": 0.93, "space": 0.07, "aggression": 0.82},
    ),
    make_preset(
        "Forge Impact",
        "Prism",
        "Onset",
        "Steel on steel. Every hit carries the weight of a thousand tons.",
        ["hot-dense", "extreme", "prism", "percussion"],
        {"perc_v1_blend": 0.9, "perc_v1_algoMode": 2.0, "perc_v1_pitch": 180.0,
         "perc_v1_drive": 0.88, "perc_v1_punch": 0.95, "perc_v1_decay": 280.0,
         "perc_v2_blend": 0.85, "perc_v2_drive": 0.9, "perc_v2_pitch": 95.0,
         "perc_v2_punch": 0.88, "perc_v2_decay": 320.0},
        {"brightness": 0.86, "warmth": 0.86, "movement": 0.72, "density": 0.89, "space": 0.09, "aggression": 0.88},
    ),
    make_preset(
        "Thermal Collapse",
        "Prism",
        "Ouroboros",
        "Self-feeding harmonic loop at maximum thermal density.",
        ["hot-dense", "extreme", "prism", "feedback"],
        {"ouro_topology": 3.0, "ouro_rate": 0.85, "ouro_chaosIndex": 0.88,
         "ouro_leash": 0.15, "ouro_feedback": 0.92, "ouro_brightness": 0.9,
         "ouro_density": 0.91, "ouro_decay": 1.8},
        {"brightness": 0.9, "warmth": 0.87, "movement": 0.78, "density": 0.91, "space": 0.06, "aggression": 0.85},
    ),
    make_preset(
        "Incandescent Wall",
        "Prism",
        "Octopus",
        "Eight oscillators firing simultaneously at maximum saturation.",
        ["hot-dense", "extreme", "prism", "layered"],
        {"octo_wtPosition": 0.88, "octo_wtScanRate": 0.72, "octo_filterCutoff": 5000.0,
         "octo_filterReso": 0.75, "octo_drive": 0.91, "octo_spread": 0.82,
         "octo_attack": 3.0, "octo_decay": 250.0, "octo_sustain": 0.9, "octo_release": 200.0},
        {"brightness": 0.87, "warmth": 0.86, "movement": 0.68, "density": 0.94, "space": 0.05, "aggression": 0.79},
    ),
    make_preset(
        "Pressure Wave",
        "Prism",
        "Overbite",
        "Compressed bass pressure meets searing top-end bite.",
        ["hot-dense", "extreme", "prism", "bass"],
        {"macro_character": 0.92, "macro_movement": 0.7, "macro_coupling": 0.55, "macro_space": 0.08},
        {"brightness": 0.85, "warmth": 0.88, "movement": 0.7, "density": 0.9, "space": 0.08, "aggression": 0.82},
    ),
    make_preset(
        "Furnace Choir",
        "Prism",
        "Ottoni",
        "Triple brass at maximum blaze, stacked in a burning unison.",
        ["hot-dense", "extreme", "prism", "brass"],
        {"otto_toddlerLevel": 0.9, "otto_toddlerPressure": 0.92, "otto_toddlerInst": 2.0,
         "otto_tweenLevel": 0.88, "otto_tweenPressure": 0.89, "otto_tweenInst": 1.0,
         "otto_grownupLevel": 0.85, "otto_grownupPressure": 0.91, "otto_grownupInst": 3.0,
         "otto_crowdDensity": 0.9},
        {"brightness": 0.92, "warmth": 0.87, "movement": 0.65, "density": 0.88, "space": 0.07, "aggression": 0.8},
    ),
    make_preset(
        "Red Zone Aggressor",
        "Prism",
        "Oblique",
        "Wavefolded into oblivion. The harmonic spectrum is on fire.",
        ["hot-dense", "extreme", "prism", "distorted"],
        {"oblq_oscWave": 3.0, "oblq_oscFold": 0.94, "oblq_oscDetune": 8.0,
         "oblq_filterCutoff": 4800.0, "oblq_filterReso": 0.88, "oblq_filterDrive": 0.92,
         "oblq_envAmt": 0.85, "oblq_attack": 2.0, "oblq_decay": 200.0, "oblq_sustain": 0.9},
        {"brightness": 0.93, "warmth": 0.85, "movement": 0.62, "density": 0.9, "space": 0.06, "aggression": 0.91},
    ),
    make_preset(
        "Molten Lattice",
        "Prism",
        "Origami",
        "Every fold reflects heat. Stacked resonances glow at the seams.",
        ["hot-dense", "extreme", "prism", "textured"],
        {"origami_foldPoint": 0.9, "origami_foldAmt": 0.88, "origami_layers": 0.92,
         "origami_brightness": 0.88, "origami_tension": 0.85, "origami_resonance": 0.9,
         "origami_release": 300.0},
        {"brightness": 0.88, "warmth": 0.9, "movement": 0.6, "density": 0.91, "space": 0.06, "aggression": 0.77},
    ),
    make_preset(
        "Supercell Core",
        "Prism",
        "Overworld",
        "Chip-era chaos maximized. Every channel at peak saturation.",
        ["hot-dense", "extreme", "prism", "chip"],
        {"ow_era": 0.92, "ow_eraY": 0.88, "ow_voiceMode": 2.0, "ow_masterVol": 0.85,
         "ow_filterCutoff": 6000.0, "ow_drive": 0.9, "ow_glitchAmt": 0.75},
        {"brightness": 0.94, "warmth": 0.86, "movement": 0.75, "density": 0.87, "space": 0.06, "aggression": 0.83},
    ),
]

# ---------------------------------------------------------------------------
# CORNER 2 — Cold Sparse (Aether)
# brightness≤0.15, warmth≤0.15, density≤0.15, aggression≤0.15
# ---------------------------------------------------------------------------

c2_presets = [
    make_preset(
        "Absolute Zero",
        "Aether",
        "Organon",
        "Nothing. Below noise floor. The absence that precedes all sound.",
        ["cold-sparse", "extreme", "aether", "minimal"],
        {"organon_metabolicRate": 0.04, "organon_enzymeSelect": 220.0, "organon_catalystDrive": 0.06,
         "organon_dampingCoeff": 0.97, "organon_signalFlux": 0.04, "organon_phasonShift": 0.02,
         "organon_isotopeBalance": 0.5, "organon_lockIn": 0.0, "organon_membrane": 0.05, "organon_noiseColor": 0.0},
        {"brightness": 0.05, "warmth": 0.06, "movement": 0.08, "density": 0.06, "space": 0.92, "aggression": 0.04},
    ),
    make_preset(
        "Void Trace",
        "Aether",
        "Opal",
        "A single grain per measure. Cold light in infinite darkness.",
        ["cold-sparse", "extreme", "aether", "granular"],
        {"opal_grainSize": 0.95, "opal_density": 0.04, "opal_posScatter": 0.85,
         "opal_pitchScatter": 0.0, "opal_filterCutoff": 280.0, "opal_filterReso": 0.08,
         "opal_attack": 800.0, "opal_release": 1200.0, "opal_dryWet": 0.92},
        {"brightness": 0.07, "warmth": 0.05, "movement": 0.05, "density": 0.04, "space": 0.95, "aggression": 0.03},
    ),
    make_preset(
        "Dead Signal",
        "Aether",
        "Obscura",
        "A plucked string with all resonance removed. Cold mathematics only.",
        ["cold-sparse", "extreme", "aether", "minimal"],
        {"obscura_stiffness": 0.06, "obscura_damping": 0.96, "obscura_nonlinear": 0.02,
         "obscura_brightness": 0.05, "obscura_decay": 0.04, "obscura_exciteAmt": 0.08},
        {"brightness": 0.06, "warmth": 0.07, "movement": 0.06, "density": 0.07, "space": 0.91, "aggression": 0.05},
    ),
    make_preset(
        "Null Field",
        "Aether",
        "Ombre",
        "Memory collapsed to a point. Interference at near-zero amplitude.",
        ["cold-sparse", "extreme", "aether", "ambient"],
        {"ombre_blend": 0.04, "ombre_interference": 0.03, "ombre_memoryDecay": 0.97,
         "ombre_brightness": 0.06, "ombre_movement": 0.05, "ombre_space": 0.95,
         "ombre_level": -24.0},
        {"brightness": 0.05, "warmth": 0.04, "movement": 0.05, "density": 0.05, "space": 0.96, "aggression": 0.03},
    ),
    make_preset(
        "Interstellar Medium",
        "Aether",
        "Osprey",
        "Shore receded to a theoretical point. Only cold swell remains.",
        ["cold-sparse", "extreme", "aether", "ambient"],
        {"osprey_shore": 0.04, "osprey_seaState": 0.03, "osprey_swellPeriod": 0.96,
         "osprey_brightness": 0.06, "osprey_foam": 0.03, "osprey_depth": 0.97,
         "osprey_wetMix": 0.9},
        {"brightness": 0.06, "warmth": 0.05, "movement": 0.07, "density": 0.06, "space": 0.94, "aggression": 0.04},
    ),
    make_preset(
        "Permafrost",
        "Aether",
        "Orbital",
        "Orbital resonance frozen. Brightness extinguished. Movement ceased.",
        ["cold-sparse", "extreme", "aether", "static"],
        {"orb_brightness": 0.05, "orb_groupEnvRate": 0.04, "orb_groupEnvDepth": 0.03,
         "orb_spread": 0.04, "orb_density": 0.06, "orb_attack": 1200.0, "orb_release": 2000.0},
        {"brightness": 0.07, "warmth": 0.06, "movement": 0.04, "density": 0.07, "space": 0.93, "aggression": 0.05},
    ),
    make_preset(
        "Dark Matter",
        "Aether",
        "Obsidian",
        "Invisible mass. No emission, no reflection. Gravitational only.",
        ["cold-sparse", "extreme", "aether", "dark"],
        {"obsidian_densityX": 0.06, "obsidian_tiltY": 0.04, "obsidian_depth": 0.97,
         "obsidian_brightness": 0.05, "obsidian_movement": 0.04, "obsidian_aggression": 0.04},
        {"brightness": 0.04, "warmth": 0.05, "movement": 0.05, "density": 0.08, "space": 0.95, "aggression": 0.05},
    ),
    make_preset(
        "Entropic Trace",
        "Aether",
        "Optic",
        "Light below detection threshold. Optic at its quietest extreme.",
        ["cold-sparse", "extreme", "aether", "minimal"],
        {"optic_reactivity": 0.04, "optic_inputGain": 0.05, "optic_autoPulse": 0.03,
         "optic_brightness": 0.06, "optic_movement": 0.04, "optic_wetDry": 0.06},
        {"brightness": 0.06, "warmth": 0.04, "movement": 0.05, "density": 0.05, "space": 0.94, "aggression": 0.04},
    ),
    make_preset(
        "Frozen Spectrum",
        "Aether",
        "Ocelot",
        "The ecosystem reduced to a single cold strand of signal.",
        ["cold-sparse", "extreme", "aether", "minimal"],
        {"ocelot_biome": 0.05, "ocelot_strataBalance": 0.04, "ocelot_ecosystemDepth": 0.04,
         "ocelot_brightness": 0.06, "ocelot_movement": 0.03, "ocelot_density": 0.05,
         "ocelot_aggression": 0.04},
        {"brightness": 0.07, "warmth": 0.06, "movement": 0.04, "density": 0.06, "space": 0.93, "aggression": 0.06},
    ),
    make_preset(
        "Zero Kelvin Drone",
        "Aether",
        "Oceanic",
        "Boid swarm frozen. Separation at maximum. No coherence, no warmth.",
        ["cold-sparse", "extreme", "aether", "drone"],
        {"ocean_separation": 0.97, "ocean_alignment": 0.03, "ocean_cohesion": 0.03,
         "ocean_brightness": 0.06, "ocean_depth": 0.96, "ocean_turbulence": 0.03},
        {"brightness": 0.05, "warmth": 0.05, "movement": 0.06, "density": 0.05, "space": 0.95, "aggression": 0.04},
    ),
]

# ---------------------------------------------------------------------------
# CORNER 3 — Hot Sparse (Prism)
# brightness≥0.85, warmth≥0.85, density≤0.15, space≥0.85
# ---------------------------------------------------------------------------

c3_presets = [
    make_preset(
        "Desert Star",
        "Prism",
        "Odyssey",
        "Single oscillator burning in an empty sky. Warm tone, infinite space.",
        ["hot-sparse", "extreme", "prism", "lead"],
        {"drift_oscA_mode": 1.0, "drift_oscA_shape": 0.85, "drift_oscA_level": 0.88,
         "drift_filterCutoff": 5500.0, "drift_filterReso": 0.15, "drift_filterSlope": 0.0,
         "drift_hazeAmount": 0.72, "drift_attack": 80.0, "drift_decay": 600.0,
         "drift_sustain": 0.88, "drift_release": 800.0, "drift_filterEnvAmt": 0.3, "drift_level": -4.0},
        {"brightness": 0.88, "warmth": 0.87, "movement": 0.22, "density": 0.08, "space": 0.92, "aggression": 0.18},
    ),
    make_preset(
        "Lone Plasma Thread",
        "Prism",
        "OddfeliX",
        "One spectral tone in vast harmonic emptiness. Radiant and solitary.",
        ["hot-sparse", "extreme", "prism", "spectral"],
        {"snap_oscMode": 2.0, "snap_filterCutoff": 6000.0, "snap_filterReso": 0.1,
         "snap_filterEnvDepth": 0.2, "snap_drive": 0.85, "snap_attack": 120.0,
         "snap_decay": 800.0, "snap_sustain": 0.9, "snap_release": 1000.0},
        {"brightness": 0.91, "warmth": 0.86, "movement": 0.2, "density": 0.07, "space": 0.91, "aggression": 0.15},
    ),
    make_preset(
        "Sunbeam Solo",
        "Prism",
        "Ohm",
        "Dad's single note philosophy: one warm tone, all the room in the world.",
        ["hot-sparse", "extreme", "prism", "organic"],
        {"ohm_dadInstrument": 1.0, "ohm_dadLevel": 0.9, "ohm_pluckBrightness": 0.88,
         "ohm_pluckDecay": 1200.0, "ohm_resonance": 0.12, "ohm_space": 0.92,
         "ohm_warmth": 0.89},
        {"brightness": 0.89, "warmth": 0.91, "movement": 0.18, "density": 0.09, "space": 0.9, "aggression": 0.12},
    ),
    make_preset(
        "Brass Horizon",
        "Prism",
        "Obbligato",
        "Single brass voice singing across an empty plain at golden hour.",
        ["hot-sparse", "extreme", "prism", "brass"],
        {"obbl_breathA": 0.88, "obbl_embouchureA": 0.9, "obbl_airFlutterA": 0.15,
         "obbl_instrumentA": 1.0, "obbl_brightnessA": 0.88, "obbl_spaceA": 0.92,
         "obbl_attackA": 40.0, "obbl_releaseA": 900.0},
        {"brightness": 0.9, "warmth": 0.88, "movement": 0.2, "density": 0.07, "space": 0.93, "aggression": 0.1},
    ),
    make_preset(
        "Golden Thread",
        "Prism",
        "Orphica",
        "One string plucked in a cathedral. Warmth radiated into infinite reverb.",
        ["hot-sparse", "extreme", "prism", "harp"],
        {"orph_stringMaterial": 0.88, "orph_pluckBrightness": 0.91, "orph_pluckPosition": 0.85,
         "orph_stringCount": 1.0, "orph_resonance": 0.12, "orph_space": 0.93,
         "orph_decay": 1500.0},
        {"brightness": 0.92, "warmth": 0.89, "movement": 0.15, "density": 0.06, "space": 0.93, "aggression": 0.08},
    ),
    make_preset(
        "Infrared Whisper",
        "Prism",
        "Orca",
        "Single wavetable position, maximum warmth, room for sound to breathe.",
        ["hot-sparse", "extreme", "prism", "wavetable"],
        {"orca_wtPosition": 0.88, "orca_wtScanRate": 0.05, "orca_formantIntensity": 0.85,
         "orca_filterCutoff": 5200.0, "orca_filterReso": 0.12, "orca_attack": 60.0,
         "orca_decay": 700.0, "orca_sustain": 0.88, "orca_release": 1100.0},
        {"brightness": 0.89, "warmth": 0.87, "movement": 0.12, "density": 0.08, "space": 0.91, "aggression": 0.1},
    ),
    make_preset(
        "Solar Wind Solo",
        "Prism",
        "Overworld",
        "Single chip channel at peak brilliance. Wide empty sky around it.",
        ["hot-sparse", "extreme", "prism", "chip"],
        {"ow_era": 0.88, "ow_eraY": 0.3, "ow_voiceMode": 0.0, "ow_masterVol": 0.82,
         "ow_filterCutoff": 7000.0, "ow_drive": 0.75, "ow_glitchAmt": 0.1},
        {"brightness": 0.93, "warmth": 0.86, "movement": 0.18, "density": 0.09, "space": 0.9, "aggression": 0.14},
    ),
    make_preset(
        "Ember Flute",
        "Prism",
        "Origami",
        "One fold, maximum brightness. The crease glows in empty air.",
        ["hot-sparse", "extreme", "prism", "textured"],
        {"origami_foldPoint": 0.12, "origami_foldAmt": 0.88, "origami_layers": 0.08,
         "origami_brightness": 0.92, "origami_tension": 0.15, "origami_resonance": 0.12,
         "origami_release": 900.0},
        {"brightness": 0.91, "warmth": 0.88, "movement": 0.14, "density": 0.07, "space": 0.92, "aggression": 0.09},
    ),
    make_preset(
        "Warm Zenith",
        "Prism",
        "Optic",
        "One reactive burst in vast space. Light source without surroundings.",
        ["hot-sparse", "extreme", "prism", "reactive"],
        {"optic_reactivity": 0.88, "optic_inputGain": 0.85, "optic_autoPulse": 0.15,
         "optic_brightness": 0.9, "optic_movement": 0.2, "optic_wetDry": 0.92},
        {"brightness": 0.9, "warmth": 0.87, "movement": 0.22, "density": 0.08, "space": 0.91, "aggression": 0.12},
    ),
    make_preset(
        "Solstice Tone",
        "Prism",
        "Oracle",
        "Single breakpoint. Warm arc in unbounded time-space.",
        ["hot-sparse", "extreme", "prism", "generative"],
        {"oracle_breakpoints": 1.0, "oracle_timeStep": 0.92, "oracle_ampStep": 0.88,
         "oracle_brightness": 0.88, "oracle_space": 0.93, "oracle_density": 0.06},
        {"brightness": 0.88, "warmth": 0.9, "movement": 0.2, "density": 0.07, "space": 0.94, "aggression": 0.09},
    ),
]

# ---------------------------------------------------------------------------
# CORNER 4 — Cold Dense (Aether)
# brightness≤0.15, warmth≤0.15, density≥0.85, aggression≥0.75
# ---------------------------------------------------------------------------

c4_presets = [
    make_preset(
        "Tectonic Pressure",
        "Aether",
        "Overlap",
        "Cold mass compressed to maximum density. Topology at terminal load.",
        ["cold-dense", "extreme", "aether", "dense"],
        {"olap_knotType": 3.0, "olap_delayFeedback": 0.92, "olap_diffusion": 0.96,
         "olap_spread": 0.88, "olap_size": 0.04, "olap_decay": 0.96,
         "olap_damping": 0.05, "olap_brightness": 0.06},
        {"brightness": 0.06, "warmth": 0.08, "movement": 0.65, "density": 0.93, "space": 0.05, "aggression": 0.82},
    ),
    make_preset(
        "Frozen Horde",
        "Aether",
        "Outwit",
        "Eight arms maximally active in total darkness. Cold cellular riot.",
        ["cold-dense", "extreme", "aether", "cellular"],
        {"owit_rule": 30.0, "owit_armBalance": 0.88, "owit_mutationRate": 0.85,
         "owit_armSpread": 0.05, "owit_brightness": 0.06, "owit_density": 0.93,
         "owit_aggression": 0.87, "owit_warmth": 0.06},
        {"brightness": 0.07, "warmth": 0.07, "movement": 0.78, "density": 0.92, "space": 0.05, "aggression": 0.88},
    ),
    make_preset(
        "Dark Compression",
        "Aether",
        "Oblong",
        "All oscillators locked tight in cold phase. No light escapes.",
        ["cold-dense", "extreme", "aether", "compressed"],
        {"bob_oscA_wave": 2.0, "bob_oscA_shape": 0.88, "bob_oscA_tune": 0.0,
         "bob_oscB_wave": 2.0, "bob_oscB_shape": 0.86, "bob_oscB_tune": -0.01,
         "bob_filterCutoff": 120.0, "bob_filterReso": 0.9, "bob_drive": 0.88,
         "bob_attack": 2.0, "bob_decay": 150.0, "bob_sustain": 0.95, "bob_release": 80.0},
        {"brightness": 0.06, "warmth": 0.08, "movement": 0.55, "density": 0.94, "space": 0.04, "aggression": 0.83},
    ),
    make_preset(
        "Void Swarm",
        "Aether",
        "Oceanic",
        "Maximum cohesion with zero brightness. Dark boids in lockstep.",
        ["cold-dense", "extreme", "aether", "swarm"],
        {"ocean_separation": 0.05, "ocean_alignment": 0.95, "ocean_cohesion": 0.95,
         "ocean_brightness": 0.05, "ocean_depth": 0.97, "ocean_turbulence": 0.88},
        {"brightness": 0.05, "warmth": 0.06, "movement": 0.82, "density": 0.93, "space": 0.04, "aggression": 0.85},
    ),
    make_preset(
        "Basalt Grid",
        "Aether",
        "Ocelot",
        "Every stratum occupied, no light admitted. Maximum geological density.",
        ["cold-dense", "extreme", "aether", "layered"],
        {"ocelot_biome": 0.96, "ocelot_strataBalance": 0.92, "ocelot_ecosystemDepth": 0.95,
         "ocelot_brightness": 0.05, "ocelot_movement": 0.75, "ocelot_density": 0.93,
         "ocelot_aggression": 0.82},
        {"brightness": 0.06, "warmth": 0.07, "movement": 0.72, "density": 0.94, "space": 0.04, "aggression": 0.82},
    ),
    make_preset(
        "Abyssal Stampede",
        "Aether",
        "Onset",
        "Maximum percussion density in absolute darkness. The floor shakes.",
        ["cold-dense", "extreme", "aether", "percussion"],
        {"perc_v1_blend": 0.92, "perc_v1_algoMode": 3.0, "perc_v1_pitch": 55.0,
         "perc_v1_drive": 0.88, "perc_v1_punch": 0.92, "perc_v1_decay": 180.0,
         "perc_v2_blend": 0.88, "perc_v2_drive": 0.85, "perc_v2_pitch": 40.0,
         "perc_v2_punch": 0.9, "perc_v2_decay": 200.0},
        {"brightness": 0.07, "warmth": 0.08, "movement": 0.85, "density": 0.95, "space": 0.03, "aggression": 0.9},
    ),
    make_preset(
        "Cold Iron Mass",
        "Aether",
        "Overdub",
        "Dub engine at maximum thickness, all brightness stripped out.",
        ["cold-dense", "extreme", "aether", "dub"],
        {"dub_oscWave": 2.0, "dub_oscOctave": -2.0, "dub_oscTune": 0.0, "dub_drive": 0.88,
         "dub_filterCutoff": 85.0, "dub_filterReso": 0.85, "dub_filterEnvAmt": 0.8,
         "dub_attack": 5.0, "dub_decay": 300.0, "dub_sustain": 0.95, "dub_release": 200.0,
         "dub_distortion": 0.82, "dub_level": -1.0},
        {"brightness": 0.07, "warmth": 0.09, "movement": 0.55, "density": 0.91, "space": 0.04, "aggression": 0.81},
    ),
    make_preset(
        "Subzero Pulse Grid",
        "Aether",
        "Oblique",
        "Wavefolded into darkness. Dense harmonic web without any light.",
        ["cold-dense", "extreme", "aether", "distorted"],
        {"oblq_oscWave": 2.0, "oblq_oscFold": 0.92, "oblq_oscDetune": 4.0,
         "oblq_filterCutoff": 90.0, "oblq_filterReso": 0.88, "oblq_filterDrive": 0.9,
         "oblq_envAmt": 0.82, "oblq_attack": 2.0, "oblq_decay": 180.0, "oblq_sustain": 0.92},
        {"brightness": 0.06, "warmth": 0.08, "movement": 0.6, "density": 0.92, "space": 0.04, "aggression": 0.84},
    ),
    make_preset(
        "Dark Web Resonance",
        "Aether",
        "Ouroboros",
        "Feedback loop locked in the sub-bass. Cold, self-amplifying density.",
        ["cold-dense", "extreme", "aether", "feedback"],
        {"ouro_topology": 2.0, "ouro_rate": 0.88, "ouro_chaosIndex": 0.82,
         "ouro_leash": 0.06, "ouro_feedback": 0.95, "ouro_brightness": 0.05,
         "ouro_density": 0.93, "ouro_decay": 0.5},
        {"brightness": 0.05, "warmth": 0.07, "movement": 0.72, "density": 0.94, "space": 0.04, "aggression": 0.87},
    ),
    make_preset(
        "Cryogenic Stampede",
        "Aether",
        "Obese",
        "Maximum sub density with all warmth frozen out. Clinical aggression.",
        ["cold-dense", "extreme", "aether", "bass"],
        {"fat_morph": 0.06, "fat_mojo": 0.05, "fat_subLevel": 0.95,
         "fat_filterCutoff": 75.0, "fat_drive": 0.92, "fat_crush": 0.88,
         "fat_attack": 2.0, "fat_decay": 160.0, "fat_sustain": 0.94, "fat_release": 100.0},
        {"brightness": 0.07, "warmth": 0.06, "movement": 0.58, "density": 0.95, "space": 0.04, "aggression": 0.85},
    ),
]

# ---------------------------------------------------------------------------
# CORNER 5 — Kinetic Bright (Prism)
# movement≥0.9, brightness≥0.8, space≤0.2, density≥0.7
# ---------------------------------------------------------------------------

c5_presets = [
    make_preset(
        "Photon Storm",
        "Prism",
        "Ouroboros",
        "Feedback loop at maximum velocity. Bright cascades with no room to breathe.",
        ["kinetic-bright", "extreme", "prism", "fast"],
        {"ouro_topology": 4.0, "ouro_rate": 0.96, "ouro_chaosIndex": 0.88,
         "ouro_leash": 0.08, "ouro_feedback": 0.88, "ouro_brightness": 0.92,
         "ouro_density": 0.82, "ouro_decay": 0.12},
        {"brightness": 0.92, "warmth": 0.55, "movement": 0.95, "density": 0.85, "space": 0.08, "aggression": 0.78},
    ),
    make_preset(
        "Orbital Frenzy",
        "Prism",
        "Orbital",
        "Every grouped resonance spinning at maximum rate. No stillness survives.",
        ["kinetic-bright", "extreme", "prism", "fast"],
        {"orb_brightness": 0.91, "orb_groupEnvRate": 0.95, "orb_groupEnvDepth": 0.88,
         "orb_spread": 0.85, "orb_density": 0.82, "orb_attack": 2.0, "orb_release": 50.0},
        {"brightness": 0.9, "warmth": 0.52, "movement": 0.94, "density": 0.83, "space": 0.1, "aggression": 0.72},
    ),
    make_preset(
        "Cascade Eruption",
        "Prism",
        "Octopus",
        "Eight voices firing at light speed. Saturated, dense, relentless.",
        ["kinetic-bright", "extreme", "prism", "dense"],
        {"octo_wtPosition": 0.85, "octo_wtScanRate": 0.95, "octo_filterCutoff": 7000.0,
         "octo_filterReso": 0.55, "octo_drive": 0.82, "octo_spread": 0.75,
         "octo_attack": 1.0, "octo_decay": 80.0, "octo_sustain": 0.85, "octo_release": 60.0},
        {"brightness": 0.88, "warmth": 0.5, "movement": 0.93, "density": 0.85, "space": 0.09, "aggression": 0.75},
    ),
    make_preset(
        "Kinetic Seizure",
        "Prism",
        "Optic",
        "Reactive light at maximum pulse rate. No gap between flashes.",
        ["kinetic-bright", "extreme", "prism", "reactive"],
        {"optic_reactivity": 0.96, "optic_inputGain": 0.88, "optic_autoPulse": 0.95,
         "optic_brightness": 0.9, "optic_movement": 0.95, "optic_wetDry": 0.85},
        {"brightness": 0.91, "warmth": 0.48, "movement": 0.96, "density": 0.8, "space": 0.08, "aggression": 0.72},
    ),
    make_preset(
        "Firefly Swarm",
        "Prism",
        "Outwit",
        "Cellular automata at max update rate. Every arm firing bright.",
        ["kinetic-bright", "extreme", "prism", "cellular"],
        {"owit_rule": 110.0, "owit_armBalance": 0.88, "owit_mutationRate": 0.92,
         "owit_armSpread": 0.18, "owit_brightness": 0.88, "owit_density": 0.82,
         "owit_aggression": 0.72, "owit_warmth": 0.5},
        {"brightness": 0.88, "warmth": 0.52, "movement": 0.93, "density": 0.82, "space": 0.1, "aggression": 0.72},
    ),
    make_preset(
        "Hyper Pluck Cascade",
        "Prism",
        "Orphica",
        "All strings at maximum pluck density, bright harmonics stacking fast.",
        ["kinetic-bright", "extreme", "prism", "harp"],
        {"orph_stringMaterial": 0.85, "orph_pluckBrightness": 0.92, "orph_pluckPosition": 0.15,
         "orph_stringCount": 0.95, "orph_resonance": 0.82, "orph_space": 0.12,
         "orph_decay": 120.0},
        {"brightness": 0.9, "warmth": 0.55, "movement": 0.92, "density": 0.82, "space": 0.11, "aggression": 0.68},
    ),
    make_preset(
        "Percussive Supernova",
        "Prism",
        "Onset",
        "All eight voices at maximum rate. Bright transient mass with no gaps.",
        ["kinetic-bright", "extreme", "prism", "percussion"],
        {"perc_v1_blend": 0.88, "perc_v1_algoMode": 1.0, "perc_v1_pitch": 280.0,
         "perc_v1_drive": 0.82, "perc_v1_punch": 0.92, "perc_v1_decay": 80.0,
         "perc_v2_blend": 0.85, "perc_v2_drive": 0.8, "perc_v2_pitch": 320.0,
         "perc_v2_punch": 0.88, "perc_v2_decay": 65.0},
        {"brightness": 0.88, "warmth": 0.5, "movement": 0.94, "density": 0.85, "space": 0.09, "aggression": 0.78},
    ),
    make_preset(
        "Spectral Blur",
        "Prism",
        "OddfeliX",
        "Harmonic scan at maximum rate. Bright spectrum smeared into motion.",
        ["kinetic-bright", "extreme", "prism", "spectral"],
        {"snap_oscMode": 3.0, "snap_filterCutoff": 7500.0, "snap_filterReso": 0.45,
         "snap_filterEnvDepth": 0.88, "snap_drive": 0.82, "snap_attack": 1.0,
         "snap_decay": 60.0, "snap_sustain": 0.82, "snap_release": 45.0},
        {"brightness": 0.91, "warmth": 0.48, "movement": 0.95, "density": 0.8, "space": 0.08, "aggression": 0.7},
    ),
    make_preset(
        "Ohm Overdrive",
        "Prism",
        "Ohm",
        "Dad's commune instrument going too fast. Maximum pluck density.",
        ["kinetic-bright", "extreme", "prism", "organic"],
        {"ohm_dadInstrument": 3.0, "ohm_dadLevel": 0.88, "ohm_pluckBrightness": 0.9,
         "ohm_pluckDecay": 85.0, "ohm_resonance": 0.82, "ohm_space": 0.12,
         "ohm_warmth": 0.55},
        {"brightness": 0.89, "warmth": 0.55, "movement": 0.92, "density": 0.8, "space": 0.12, "aggression": 0.65},
    ),
    make_preset(
        "Neon Turbulence",
        "Prism",
        "Oceanic",
        "Maximum boid turbulence under bright lights. Dense, fast, no space.",
        ["kinetic-bright", "extreme", "prism", "swarm"],
        {"ocean_separation": 0.18, "ocean_alignment": 0.88, "ocean_cohesion": 0.85,
         "ocean_brightness": 0.9, "ocean_depth": 0.15, "ocean_turbulence": 0.95},
        {"brightness": 0.9, "warmth": 0.52, "movement": 0.95, "density": 0.82, "space": 0.1, "aggression": 0.72},
    ),
]

# ---------------------------------------------------------------------------
# CORNER 6 — Still Dark (Aether)
# movement≤0.1, brightness≤0.2, warmth≤0.2, space≥0.8
# ---------------------------------------------------------------------------

c6_presets = [
    make_preset(
        "Bathyal Stasis",
        "Aether",
        "Opal",
        "One grain per epoch. No motion. Deep dark space with no warmth.",
        ["still-dark", "extreme", "aether", "granular"],
        {"opal_grainSize": 0.98, "opal_density": 0.05, "opal_posScatter": 0.02,
         "opal_pitchScatter": 0.0, "opal_filterCutoff": 180.0, "opal_filterReso": 0.05,
         "opal_attack": 2000.0, "opal_release": 3000.0, "opal_dryWet": 0.88},
        {"brightness": 0.06, "warmth": 0.07, "movement": 0.04, "density": 0.12, "space": 0.95, "aggression": 0.05},
    ),
    make_preset(
        "Midnight Monolith",
        "Aether",
        "Obsidian",
        "Immovable, lightless, warm-free. A slab of sonic dark matter.",
        ["still-dark", "extreme", "aether", "dark"],
        {"obsidian_densityX": 0.15, "obsidian_tiltY": 0.06, "obsidian_depth": 0.95,
         "obsidian_brightness": 0.05, "obsidian_movement": 0.04, "obsidian_aggression": 0.08},
        {"brightness": 0.05, "warmth": 0.06, "movement": 0.04, "density": 0.15, "space": 0.93, "aggression": 0.06},
    ),
    make_preset(
        "Hadal Silence",
        "Aether",
        "Osprey",
        "Shore ten thousand meters below. No wave motion. Pure acoustic void.",
        ["still-dark", "extreme", "aether", "ambient"],
        {"osprey_shore": 0.02, "osprey_seaState": 0.02, "osprey_swellPeriod": 0.98,
         "osprey_brightness": 0.05, "osprey_foam": 0.02, "osprey_depth": 0.98,
         "osprey_wetMix": 0.85},
        {"brightness": 0.05, "warmth": 0.06, "movement": 0.03, "density": 0.08, "space": 0.96, "aggression": 0.04},
    ),
    make_preset(
        "Crepuscular Hold",
        "Aether",
        "Ombre",
        "Twilight memory with no interference. Still, dark, spacious.",
        ["still-dark", "extreme", "aether", "ambient"],
        {"ombre_blend": 0.08, "ombre_interference": 0.02, "ombre_memoryDecay": 0.98,
         "ombre_brightness": 0.06, "ombre_movement": 0.04, "ombre_space": 0.94,
         "ombre_level": -18.0},
        {"brightness": 0.07, "warmth": 0.08, "movement": 0.04, "density": 0.1, "space": 0.94, "aggression": 0.05},
    ),
    make_preset(
        "Pelagic Rest",
        "Aether",
        "Oceanic",
        "Boids suspended without motion in lightless deep water.",
        ["still-dark", "extreme", "aether", "drone"],
        {"ocean_separation": 0.5, "ocean_alignment": 0.5, "ocean_cohesion": 0.5,
         "ocean_brightness": 0.05, "ocean_depth": 0.97, "ocean_turbulence": 0.02},
        {"brightness": 0.06, "warmth": 0.07, "movement": 0.03, "density": 0.12, "space": 0.95, "aggression": 0.04},
    ),
    make_preset(
        "Dead Zone Ambient",
        "Aether",
        "Organon",
        "Metabolic rate at zero. Enzymatic stillness. No light catalyzed.",
        ["still-dark", "extreme", "aether", "biological"],
        {"organon_metabolicRate": 0.02, "organon_enzymeSelect": 160.0, "organon_catalystDrive": 0.04,
         "organon_dampingCoeff": 0.99, "organon_signalFlux": 0.03, "organon_phasonShift": 0.01,
         "organon_isotopeBalance": 0.5, "organon_lockIn": 0.0, "organon_membrane": 0.04, "organon_noiseColor": 0.0},
        {"brightness": 0.04, "warmth": 0.05, "movement": 0.03, "density": 0.08, "space": 0.96, "aggression": 0.04},
    ),
    make_preset(
        "Coma Drone",
        "Aether",
        "Overlap",
        "FDN decay so long it's essentially static. Dark, motionless, vast.",
        ["still-dark", "extreme", "aether", "drone"],
        {"olap_knotType": 1.0, "olap_delayFeedback": 0.97, "olap_diffusion": 0.88,
         "olap_spread": 0.9, "olap_size": 0.98, "olap_decay": 0.98,
         "olap_damping": 0.96, "olap_brightness": 0.04},
        {"brightness": 0.05, "warmth": 0.06, "movement": 0.04, "density": 0.12, "space": 0.95, "aggression": 0.04},
    ),
    make_preset(
        "Crypt Resonance",
        "Aether",
        "Obscura",
        "Physical model at standstill. Zero excitation. Maximum hollow space.",
        ["still-dark", "extreme", "aether", "physical"],
        {"obscura_stiffness": 0.04, "obscura_damping": 0.98, "obscura_nonlinear": 0.01,
         "obscura_brightness": 0.04, "obscura_decay": 0.02, "obscura_exciteAmt": 0.04},
        {"brightness": 0.05, "warmth": 0.05, "movement": 0.04, "density": 0.08, "space": 0.95, "aggression": 0.04},
    ),
    make_preset(
        "Void Sustain",
        "Aether",
        "Orbital",
        "Orbital group rates frozen. Silence stretched across infinite space.",
        ["still-dark", "extreme", "aether", "minimal"],
        {"orb_brightness": 0.05, "orb_groupEnvRate": 0.02, "orb_groupEnvDepth": 0.03,
         "orb_spread": 0.92, "orb_density": 0.1, "orb_attack": 3000.0, "orb_release": 4000.0},
        {"brightness": 0.06, "warmth": 0.07, "movement": 0.03, "density": 0.1, "space": 0.95, "aggression": 0.04},
    ),
    make_preset(
        "Night Topology",
        "Aether",
        "Ouroboros",
        "Self-reference loop so slow it's effectively frozen in dark eternity.",
        ["still-dark", "extreme", "aether", "feedback"],
        {"ouro_topology": 1.0, "ouro_rate": 0.04, "ouro_chaosIndex": 0.06,
         "ouro_leash": 0.96, "ouro_feedback": 0.3, "ouro_brightness": 0.05,
         "ouro_density": 0.12, "ouro_decay": 6.0},
        {"brightness": 0.05, "warmth": 0.06, "movement": 0.04, "density": 0.12, "space": 0.94, "aggression": 0.05},
    ),
]

# ---------------------------------------------------------------------------
# CORNER 7 — Aggressive Sparse (Prism)
# aggression≥0.9, density≤0.2, brightness≥0.7, movement≥0.7
# ---------------------------------------------------------------------------

c7_presets = [
    make_preset(
        "Spike Array",
        "Prism",
        "Oblique",
        "Isolated transients from a wavefolded blade. Each hit is a scar.",
        ["aggressive-sparse", "extreme", "prism", "transient"],
        {"oblq_oscWave": 3.0, "oblq_oscFold": 0.97, "oblq_oscDetune": 0.0,
         "oblq_filterCutoff": 6500.0, "oblq_filterReso": 0.92, "oblq_filterDrive": 0.95,
         "oblq_envAmt": 0.92, "oblq_attack": 0.5, "oblq_decay": 45.0, "oblq_sustain": 0.0},
        {"brightness": 0.92, "warmth": 0.35, "movement": 0.88, "density": 0.1, "space": 0.75, "aggression": 0.96},
    ),
    make_preset(
        "Lone Predator",
        "Prism",
        "Ocelot",
        "Solitary apex signal. One strata, maximum brightness and violence.",
        ["aggressive-sparse", "extreme", "prism", "blade"],
        {"ocelot_biome": 0.08, "ocelot_strataBalance": 0.05, "ocelot_ecosystemDepth": 0.06,
         "ocelot_brightness": 0.92, "ocelot_movement": 0.88, "ocelot_density": 0.1,
         "ocelot_aggression": 0.95},
        {"brightness": 0.9, "warmth": 0.32, "movement": 0.88, "density": 0.09, "space": 0.78, "aggression": 0.95},
    ),
    make_preset(
        "Needle Burst",
        "Prism",
        "Onset",
        "Single percussive voice. Maximum attack, instant release, pure aggression.",
        ["aggressive-sparse", "extreme", "prism", "percussion"],
        {"perc_v1_blend": 0.95, "perc_v1_algoMode": 0.0, "perc_v1_pitch": 420.0,
         "perc_v1_drive": 0.95, "perc_v1_punch": 0.97, "perc_v1_decay": 28.0,
         "perc_v2_blend": 0.0},
        {"brightness": 0.88, "warmth": 0.3, "movement": 0.92, "density": 0.08, "space": 0.8, "aggression": 0.97},
    ),
    make_preset(
        "Chaos Seed",
        "Prism",
        "Ouroboros",
        "Single loop, maximum chaos. A violent point in empty space.",
        ["aggressive-sparse", "extreme", "prism", "chaos"],
        {"ouro_topology": 4.0, "ouro_rate": 0.82, "ouro_chaosIndex": 0.97,
         "ouro_leash": 0.04, "ouro_feedback": 0.72, "ouro_brightness": 0.88,
         "ouro_density": 0.1, "ouro_decay": 0.08},
        {"brightness": 0.87, "warmth": 0.28, "movement": 0.9, "density": 0.09, "space": 0.8, "aggression": 0.96},
    ),
    make_preset(
        "Electrocution",
        "Prism",
        "OddfeliX",
        "Single oscillator with maximum snap aggression. Bite and disappear.",
        ["aggressive-sparse", "extreme", "prism", "transient"],
        {"snap_oscMode": 1.0, "snap_filterCutoff": 7800.0, "snap_filterReso": 0.95,
         "snap_filterEnvDepth": 0.95, "snap_drive": 0.95, "snap_attack": 0.2,
         "snap_decay": 30.0, "snap_sustain": 0.0, "snap_release": 20.0},
        {"brightness": 0.93, "warmth": 0.25, "movement": 0.92, "density": 0.08, "space": 0.82, "aggression": 0.97},
    ),
    make_preset(
        "Razor Pluck",
        "Prism",
        "Orphica",
        "One string, maximum pluck position aggression. Gone before you register it.",
        ["aggressive-sparse", "extreme", "prism", "harp"],
        {"orph_stringMaterial": 0.95, "orph_pluckBrightness": 0.92, "orph_pluckPosition": 0.98,
         "orph_stringCount": 0.08, "orph_resonance": 0.95, "orph_space": 0.78,
         "orph_decay": 35.0},
        {"brightness": 0.9, "warmth": 0.28, "movement": 0.88, "density": 0.08, "space": 0.8, "aggression": 0.95},
    ),
    make_preset(
        "Pulse Predator",
        "Prism",
        "Optic",
        "Reactive spike at maximum aggression. Single flash, maximum violence.",
        ["aggressive-sparse", "extreme", "prism", "reactive"],
        {"optic_reactivity": 0.97, "optic_inputGain": 0.95, "optic_autoPulse": 0.88,
         "optic_brightness": 0.9, "optic_movement": 0.9, "optic_wetDry": 0.18},
        {"brightness": 0.89, "warmth": 0.3, "movement": 0.9, "density": 0.09, "space": 0.79, "aggression": 0.96},
    ),
    make_preset(
        "Sniper Tone",
        "Prism",
        "Odyssey",
        "Single filtered oscillator. Maximum filter envelope aggression. One shot.",
        ["aggressive-sparse", "extreme", "prism", "lead"],
        {"drift_oscA_mode": 2.0, "drift_oscA_shape": 0.92, "drift_oscA_level": 0.88,
         "drift_filterCutoff": 7500.0, "drift_filterReso": 0.95, "drift_filterSlope": 1.0,
         "drift_hazeAmount": 0.05, "drift_attack": 0.5, "drift_decay": 40.0,
         "drift_sustain": 0.0, "drift_release": 30.0, "drift_filterEnvAmt": 0.95, "drift_level": -3.0},
        {"brightness": 0.92, "warmth": 0.25, "movement": 0.88, "density": 0.07, "space": 0.82, "aggression": 0.96},
    ),
    make_preset(
        "Whiplash",
        "Prism",
        "Overdub",
        "Dub engine stripped to one violent transient. Tape crack in a void.",
        ["aggressive-sparse", "extreme", "prism", "transient"],
        {"dub_oscWave": 1.0, "dub_oscOctave": 1.0, "dub_oscTune": 0.0, "dub_drive": 0.97,
         "dub_filterCutoff": 7000.0, "dub_filterReso": 0.92, "dub_filterEnvAmt": 0.95,
         "dub_attack": 0.3, "dub_decay": 35.0, "dub_sustain": 0.0, "dub_release": 25.0,
         "dub_distortion": 0.95, "dub_level": -3.0},
        {"brightness": 0.88, "warmth": 0.28, "movement": 0.9, "density": 0.08, "space": 0.81, "aggression": 0.97},
    ),
    make_preset(
        "Ion Cannon",
        "Prism",
        "Overworld",
        "Single chip channel at maximum aggression. One note obliterates the mix.",
        ["aggressive-sparse", "extreme", "prism", "chip"],
        {"ow_era": 0.5, "ow_eraY": 0.9, "ow_voiceMode": 0.0, "ow_masterVol": 0.88,
         "ow_filterCutoff": 8000.0, "ow_drive": 0.97, "ow_glitchAmt": 0.88},
        {"brightness": 0.91, "warmth": 0.27, "movement": 0.88, "density": 0.09, "space": 0.8, "aggression": 0.96},
    ),
]

# ---------------------------------------------------------------------------
# CORNER 8 — Warm Deep Space (Aether)
# warmth≥0.85, space≥0.9, movement≥0.6, aggression≤0.2
# ---------------------------------------------------------------------------

c8_presets = [
    make_preset(
        "Amber Nebula",
        "Aether",
        "Opal",
        "Warm grain clouds drifting through stellar distance. Unhurried and vast.",
        ["warm-deep-space", "extreme", "aether", "granular"],
        {"opal_grainSize": 0.55, "opal_density": 0.62, "opal_posScatter": 0.88,
         "opal_pitchScatter": 0.3, "opal_filterCutoff": 1800.0, "opal_filterReso": 0.12,
         "opal_attack": 400.0, "opal_release": 800.0, "opal_dryWet": 0.95},
        {"brightness": 0.52, "warmth": 0.88, "movement": 0.65, "density": 0.42, "space": 0.94, "aggression": 0.08},
    ),
    make_preset(
        "Thermal Cosmos",
        "Aether",
        "Ombre",
        "Warm interference patterns echoing across cosmological distances.",
        ["warm-deep-space", "extreme", "aether", "ambient"],
        {"ombre_blend": 0.72, "ombre_interference": 0.65, "ombre_memoryDecay": 0.15,
         "ombre_brightness": 0.52, "ombre_movement": 0.68, "ombre_space": 0.94,
         "ombre_level": -8.0},
        {"brightness": 0.5, "warmth": 0.9, "movement": 0.68, "density": 0.38, "space": 0.93, "aggression": 0.07},
    ),
    make_preset(
        "Golden Drift",
        "Aether",
        "Organon",
        "Warm metabolic pulse sent across immeasurable space. Unhurried biology.",
        ["warm-deep-space", "extreme", "aether", "biological"],
        {"organon_metabolicRate": 0.62, "organon_enzymeSelect": 1200.0, "organon_catalystDrive": 0.55,
         "organon_dampingCoeff": 0.12, "organon_signalFlux": 0.62, "organon_phasonShift": 0.45,
         "organon_isotopeBalance": 0.65, "organon_lockIn": 0.0, "organon_membrane": 0.68, "organon_noiseColor": 0.72},
        {"brightness": 0.55, "warmth": 0.91, "movement": 0.65, "density": 0.35, "space": 0.92, "aggression": 0.07},
    ),
    make_preset(
        "Sundowner Reverb",
        "Aether",
        "Overlap",
        "Warm knot topology spreading into infinite FDN decay. Sunset in a hall.",
        ["warm-deep-space", "extreme", "aether", "reverb"],
        {"olap_knotType": 2.0, "olap_delayFeedback": 0.88, "olap_diffusion": 0.72,
         "olap_spread": 0.92, "olap_size": 0.95, "olap_decay": 0.92,
         "olap_damping": 0.35, "olap_brightness": 0.52},
        {"brightness": 0.52, "warmth": 0.88, "movement": 0.62, "density": 0.32, "space": 0.94, "aggression": 0.08},
    ),
    make_preset(
        "Waltz of Embers",
        "Aether",
        "Ohm",
        "Dad's warmest chord floating in a cathedral. Moving without urgency.",
        ["warm-deep-space", "extreme", "aether", "organic"],
        {"ohm_dadInstrument": 0.0, "ohm_dadLevel": 0.88, "ohm_pluckBrightness": 0.55,
         "ohm_pluckDecay": 2200.0, "ohm_resonance": 0.18, "ohm_space": 0.94,
         "ohm_warmth": 0.92},
        {"brightness": 0.52, "warmth": 0.92, "movement": 0.62, "density": 0.3, "space": 0.93, "aggression": 0.07},
    ),
    make_preset(
        "Sepia Cloud",
        "Aether",
        "Osprey",
        "Warm tidal breathing at deep sea swell period. Vast and gentle.",
        ["warm-deep-space", "extreme", "aether", "ambient"],
        {"osprey_shore": 0.42, "osprey_seaState": 0.25, "osprey_swellPeriod": 0.65,
         "osprey_brightness": 0.52, "osprey_foam": 0.3, "osprey_depth": 0.62,
         "osprey_wetMix": 0.94},
        {"brightness": 0.5, "warmth": 0.88, "movement": 0.62, "density": 0.28, "space": 0.93, "aggression": 0.07},
    ),
    make_preset(
        "Amber Orbit",
        "Aether",
        "Orbital",
        "Slow warm orbital resonance groups. Vast ellipses in heated space.",
        ["warm-deep-space", "extreme", "aether", "ambient"],
        {"orb_brightness": 0.52, "orb_groupEnvRate": 0.62, "orb_groupEnvDepth": 0.72,
         "orb_spread": 0.92, "orb_density": 0.35, "orb_attack": 600.0, "orb_release": 1200.0},
        {"brightness": 0.5, "warmth": 0.89, "movement": 0.65, "density": 0.3, "space": 0.93, "aggression": 0.07},
    ),
    make_preset(
        "Warm Topology",
        "Aether",
        "Ouroboros",
        "Feedback at gentle warmth setting. Long looping arcs across open space.",
        ["warm-deep-space", "extreme", "aether", "feedback"],
        {"ouro_topology": 1.0, "ouro_rate": 0.62, "ouro_chaosIndex": 0.28,
         "ouro_leash": 0.55, "ouro_feedback": 0.62, "ouro_brightness": 0.52,
         "ouro_density": 0.3, "ouro_decay": 4.5},
        {"brightness": 0.5, "warmth": 0.88, "movement": 0.62, "density": 0.3, "space": 0.93, "aggression": 0.09},
    ),
    make_preset(
        "Ember Migration",
        "Aether",
        "Oceanic",
        "Warm boid migration with maximum spread. Unhurried, amber, infinite.",
        ["warm-deep-space", "extreme", "aether", "swarm"],
        {"ocean_separation": 0.65, "ocean_alignment": 0.62, "ocean_cohesion": 0.45,
         "ocean_brightness": 0.52, "ocean_depth": 0.35, "ocean_turbulence": 0.28},
        {"brightness": 0.48, "warmth": 0.9, "movement": 0.65, "density": 0.28, "space": 0.94, "aggression": 0.08},
    ),
    make_preset(
        "Solstice Bloom",
        "Aether",
        "Obbligato",
        "Warm dual wind breathing slowly across a vast frozen plain.",
        ["warm-deep-space", "extreme", "aether", "wind"],
        {"obbl_breathA": 0.88, "obbl_embouchureA": 0.65, "obbl_airFlutterA": 0.62,
         "obbl_instrumentA": 0.0, "obbl_brightnessA": 0.52, "obbl_spaceA": 0.94,
         "obbl_attackA": 400.0, "obbl_releaseA": 2000.0},
        {"brightness": 0.5, "warmth": 0.89, "movement": 0.65, "density": 0.28, "space": 0.94, "aggression": 0.08},
    ),
]

# ---------------------------------------------------------------------------
# FLUX bonus: scatter 10 presets across Flux mood from mixed corners
# ---------------------------------------------------------------------------

flux_scatter = [
    make_preset(
        "Chromatic Rupture",
        "Flux",
        "Oblique",
        "Between hot-dense and aggressive-sparse. The boundary where heat meets void.",
        ["flux", "extreme", "hybrid", "aggressive"],
        {"oblq_oscWave": 3.0, "oblq_oscFold": 0.88, "oblq_oscDetune": 12.0,
         "oblq_filterCutoff": 3500.0, "oblq_filterReso": 0.82, "oblq_filterDrive": 0.88,
         "oblq_envAmt": 0.78, "oblq_attack": 3.0, "oblq_decay": 120.0, "oblq_sustain": 0.72},
        {"brightness": 0.85, "warmth": 0.75, "movement": 0.82, "density": 0.45, "space": 0.35, "aggression": 0.88},
    ),
    make_preset(
        "Cryogenic Flash",
        "Flux",
        "Onset",
        "Cold transient burst. Sparse aggression with zero warmth.",
        ["flux", "extreme", "cold", "transient"],
        {"perc_v1_blend": 0.92, "perc_v1_algoMode": 0.0, "perc_v1_pitch": 380.0,
         "perc_v1_drive": 0.88, "perc_v1_punch": 0.95, "perc_v1_decay": 22.0,
         "perc_v2_blend": 0.0},
        {"brightness": 0.82, "warmth": 0.08, "movement": 0.88, "density": 0.12, "space": 0.78, "aggression": 0.93},
    ),
    make_preset(
        "Thermocline",
        "Flux",
        "Oceanic",
        "The layer where hot meets cold. Maximum turbulence at the boundary.",
        ["flux", "extreme", "swarm", "boundary"],
        {"ocean_separation": 0.5, "ocean_alignment": 0.72, "ocean_cohesion": 0.72,
         "ocean_brightness": 0.55, "ocean_depth": 0.45, "ocean_turbulence": 0.95},
        {"brightness": 0.68, "warmth": 0.55, "movement": 0.92, "density": 0.72, "space": 0.3, "aggression": 0.65},
    ),
    make_preset(
        "Polarized Grain",
        "Flux",
        "Opal",
        "Warm grains erupting into cold space. Thermal polarity at maximum.",
        ["flux", "extreme", "granular", "polar"],
        {"opal_grainSize": 0.25, "opal_density": 0.45, "opal_posScatter": 0.92,
         "opal_pitchScatter": 0.72, "opal_filterCutoff": 2800.0, "opal_filterReso": 0.28,
         "opal_attack": 5.0, "opal_release": 180.0, "opal_dryWet": 0.82},
        {"brightness": 0.72, "warmth": 0.82, "movement": 0.78, "density": 0.35, "space": 0.85, "aggression": 0.28},
    ),
    make_preset(
        "Volcanic Silence",
        "Flux",
        "Ouroboros",
        "Maximum feedback followed by complete cessation. Violence then void.",
        ["flux", "extreme", "feedback", "dynamic"],
        {"ouro_topology": 3.0, "ouro_rate": 0.65, "ouro_chaosIndex": 0.72,
         "ouro_leash": 0.25, "ouro_feedback": 0.82, "ouro_brightness": 0.78,
         "ouro_density": 0.42, "ouro_decay": 0.8},
        {"brightness": 0.78, "warmth": 0.62, "movement": 0.75, "density": 0.42, "space": 0.62, "aggression": 0.72},
    ),
    make_preset(
        "Pressure Inversion",
        "Flux",
        "Overworld",
        "Chip-era sound flipping between dense and sparse at every measure.",
        ["flux", "extreme", "chip", "dynamic"],
        {"ow_era": 0.62, "ow_eraY": 0.72, "ow_voiceMode": 1.0, "ow_masterVol": 0.82,
         "ow_filterCutoff": 4500.0, "ow_drive": 0.82, "ow_glitchAmt": 0.88},
        {"brightness": 0.82, "warmth": 0.45, "movement": 0.88, "density": 0.55, "space": 0.45, "aggression": 0.78},
    ),
    make_preset(
        "Midnight Surge",
        "Flux",
        "Overdub",
        "Cold dub engine suddenly warming. The tape heats up unexpectedly.",
        ["flux", "extreme", "dub", "dynamic"],
        {"dub_oscWave": 2.0, "dub_oscOctave": 0.0, "dub_oscTune": 0.0, "dub_drive": 0.82,
         "dub_filterCutoff": 1200.0, "dub_filterReso": 0.78, "dub_filterEnvAmt": 0.88,
         "dub_attack": 20.0, "dub_decay": 280.0, "dub_sustain": 0.75, "dub_release": 350.0,
         "dub_distortion": 0.75, "dub_level": -4.0},
        {"brightness": 0.45, "warmth": 0.72, "movement": 0.72, "density": 0.68, "space": 0.42, "aggression": 0.68},
    ),
    make_preset(
        "Arctic Signal",
        "Flux",
        "Orphica",
        "Harp in permafrost. Cold sparse brightness with slow harmonic decay.",
        ["flux", "extreme", "harp", "cold"],
        {"orph_stringMaterial": 0.15, "orph_pluckBrightness": 0.88, "orph_pluckPosition": 0.8,
         "orph_stringCount": 0.12, "orph_resonance": 0.08, "orph_space": 0.88,
         "orph_decay": 800.0},
        {"brightness": 0.88, "warmth": 0.08, "movement": 0.35, "density": 0.12, "space": 0.88, "aggression": 0.42},
    ),
    make_preset(
        "Fermenting Chaos",
        "Flux",
        "Organon",
        "Biological process at maximum chaos setting. Warm entropy cascade.",
        ["flux", "extreme", "biological", "chaos"],
        {"organon_metabolicRate": 0.92, "organon_enzymeSelect": 3500.0, "organon_catalystDrive": 0.88,
         "organon_dampingCoeff": 0.08, "organon_signalFlux": 0.92, "organon_phasonShift": 0.88,
         "organon_isotopeBalance": 0.85, "organon_lockIn": 0.0, "organon_membrane": 0.88, "organon_noiseColor": 0.88},
        {"brightness": 0.62, "warmth": 0.82, "movement": 0.92, "density": 0.72, "space": 0.32, "aggression": 0.58},
    ),
    make_preset(
        "Event Horizon",
        "Flux",
        "Overlap",
        "Warm FDN mass collapsed to near-infinite feedback density.",
        ["flux", "extreme", "reverb", "dense"],
        {"olap_knotType": 4.0, "olap_delayFeedback": 0.96, "olap_diffusion": 0.82,
         "olap_spread": 0.72, "olap_size": 0.25, "olap_decay": 0.95,
         "olap_damping": 0.18, "olap_brightness": 0.68},
        {"brightness": 0.65, "warmth": 0.75, "movement": 0.72, "density": 0.88, "space": 0.32, "aggression": 0.45},
    ),
]

# ---------------------------------------------------------------------------
# Write all presets
# ---------------------------------------------------------------------------

ALL_CORNERS = [
    ("C1 Hot Dense",        "Prism",  c1_presets),
    ("C2 Cold Sparse",      "Aether", c2_presets),
    ("C3 Hot Sparse",       "Prism",  c3_presets),
    ("C4 Cold Dense",       "Aether", c4_presets),
    ("C5 Kinetic Bright",   "Prism",  c5_presets),
    ("C6 Still Dark",       "Aether", c6_presets),
    ("C7 Aggressive Sparse","Prism",  c7_presets),
    ("C8 Warm Deep Space",  "Aether", c8_presets),
    ("Flux Scatter",        "Flux",   flux_scatter),
]

def main():
    total = 0
    for corner_name, mood, presets in ALL_CORNERS:
        written = []
        for p in presets:
            path = write_preset(mood, p)
            written.append(os.path.basename(path))
            total += 1
        print(f"[{corner_name}] wrote {len(written)} presets → Presets/XOlokun/{mood}/")
        for w in written:
            print(f"  {w}")

    print(f"\nTotal extreme-zone presets written: {total}")
    print("DNA hypercube corners anchored: 8 corners × 10 + 10 Flux scatter = 90 presets")
    print("\nExpected diversity lift:")
    print("  Previous score: 0.148 (critical)")
    print("  Target:         ≥0.28 (threshold)")
    print("  Extreme presets push tails of all 6 DNA dimensions to 0.03–0.97 range")

if __name__ == "__main__":
    main()
