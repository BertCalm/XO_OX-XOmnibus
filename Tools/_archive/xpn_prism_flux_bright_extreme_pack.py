#!/usr/bin/env python3
"""
xpn_prism_flux_bright_extreme_pack.py

Generate 60 ultra-bright .xometa presets:
  - 30 Prism  (brightness 0.87–0.98, movement 0.55–0.90, aggression 0.40–0.80)
  - 30 Flux   (brightness 0.88–0.99, movement 0.70–0.95, aggression 0.50–0.85)

Prism engines : OPTIC, OBLIQUE, ODDFELIX, ORIGAMI, OVERWORLD, ORBITAL,
                ONSET, OCTOPUS, ORPHICA, OCELOT
Flux engines  : OPTIC, OBLIQUE, OVERWORLD, ONSET, OCTOPUS, OUTWIT,
                ORBITAL, ORIGAMI, OPTIC, ODDFELIX  (3× OPTIC weighted intentionally)

Usage:
    python3 Tools/xpn_prism_flux_bright_extreme_pack.py

Output: Presets/XOlokun/Prism/*.xometa  (30 files)
        Presets/XOlokun/Flux/*.xometa   (30 files)
Skip if file already exists.
"""

import json
import math
import os
import random

random.seed(0xBEEF)  # reproducible

REPO_ROOT = os.path.join(os.path.dirname(__file__), "..")
PRISM_DIR = os.path.join(REPO_ROOT, "Presets", "XOlokun", "Prism")
FLUX_DIR  = os.path.join(REPO_ROOT, "Presets", "XOlokun", "Flux")

# ─── Helpers ──────────────────────────────────────────────────────────────────

def rnd(lo, hi, dp=3):
    return round(random.uniform(lo, hi), dp)

def rnd_int(lo, hi):
    return random.randint(lo, hi)

def clamp(v, lo, hi):
    return max(lo, min(hi, v))

# ─── Per-engine parameter builders ────────────────────────────────────────────

def make_optic_params(brightness, movement, aggression):
    return {
        "Optic": {
            "optic_reactivity":    rnd(0.70, 0.98),
            "optic_inputGain":     rnd(0.75, 1.0),
            "optic_autoPulse":     rnd(0.55, 0.95),
            "optic_pulseRate":     rnd(movement * 4.0, movement * 12.0, 2),
            "optic_pulseShape":    rnd(0.4, 1.0),
            "optic_pulseSwing":    rnd(0.0, 0.4),
            "optic_pulseEvolve":   rnd(0.5, 1.0),
            "optic_pulseSubdiv":   rnd_int(1, 4),
            "optic_filterCutoff":  rnd(brightness * 0.85, 1.0),
            "optic_filterReso":    rnd(0.3, 0.75),
            "optic_drive":         rnd(aggression * 0.6, aggression),
            "optic_outputLevel":   rnd(0.78, 0.92),
            "optic_couplingLevel": rnd(0.0, 0.35),
            "optic_couplingBus":   0,
        }
    }

def make_oblique_params(brightness, movement, aggression):
    return {
        "Oblique": {
            "oblq_oscWave":       rnd_int(0, 2),
            "oblq_oscFold":       rnd(aggression * 0.6, aggression),
            "oblq_oscDetune":     rnd(0.5, 3.5),
            "oblq_level":         rnd(0.78, 0.95),
            "oblq_glide":         rnd(0.0, 0.08),
            "oblq_percClick":     rnd(0.6, 0.98),
            "oblq_percDecay":     rnd(0.001, 0.04),
            "oblq_bounceRate":    rnd(movement * 8.0, movement * 22.0, 2),
            "oblq_bounceGravity": rnd(0.70, 0.98),
            "oblq_bounceDamp":    rnd(0.1, 0.35),
            "oblq_bounceCnt":     rnd(8.0, 20.0, 1),
            "oblq_bounceSwing":   rnd(0.0, 0.3),
            "oblq_clickTone":     rnd(brightness * 8000, 16000, 1),
            "oblq_filterCut":     rnd(brightness * 8000, 16000, 1),
            "oblq_filterRes":     rnd(0.3, 0.8),
            "oblq_attack":        rnd(0.001, 0.005),
            "oblq_decay":         rnd(0.04, 0.15),
            "oblq_sustain":       rnd(0.3, 0.65),
            "oblq_release":       rnd(0.04, 0.18),
            "oblq_prismDelay":    rnd(8.0, 25.0, 1),
            "oblq_prismSpread":   rnd(0.6, 1.0),
            "oblq_prismColor":    rnd(brightness * 0.8, 1.0),
            "oblq_prismWidth":    rnd(0.65, 1.0),
            "oblq_prismFeedback": rnd(0.45, 0.85),
            "oblq_prismMix":      rnd(0.35, 0.70),
            "oblq_prismDamp":     rnd(0.0, 0.2),
            "oblq_phaserRate":    rnd(movement * 2.0, movement * 6.0, 2),
            "oblq_phaserDepth":   rnd(0.4, 0.85),
            "oblq_phaserFeedback":rnd(0.3, 0.6),
            "oblq_phaserMix":     rnd(0.2, 0.5),
        }
    }

def make_oddfelix_params(brightness, movement, aggression):
    return {
        "OddfeliX": {
            "snap_snap":          rnd(aggression * 0.6, aggression),
            "snap_decay":         rnd(0.05, 0.35),
            "snap_detune":        rnd(0.0, 2.5),
            "snap_filterCutoff":  rnd(brightness * 0.75, 1.0),
            "snap_filterReso":    rnd(0.2, 0.7),
            "snap_filterEnvDepth":rnd(0.4, 0.9),
            "snap_pitchEnvDepth": rnd(0.2, 0.8),
            "snap_pitchEnvDecay": rnd(0.01, 0.12),
            "snap_noiseLevel":    rnd(aggression * 0.3, aggression * 0.7),
            "snap_morphRate":     rnd(movement * 0.5, movement * 2.0, 3),
            "snap_outputLevel":   rnd(0.78, 0.92),
            "snap_couplingLevel": rnd(0.0, 0.3),
            "snap_couplingBus":   0,
        }
    }

def make_origami_params(brightness, movement, aggression):
    return {
        "ORIGAMI": {
            "ori_macro_fold":     rnd(aggression * 0.55, aggression),
            "ori_macro_tension":  rnd(brightness * 0.6, brightness),
            "ori_macro_couple":   rnd(0.0, 0.45),
            "ori_macro_space":    rnd(0.3, 0.75),
            "ori_outputLevel":    rnd(0.78, 0.92),
            "ori_couplingLevel":  rnd(0.0, 0.4),
            "ori_couplingBus":    0,
        }
    }

def make_overworld_params(brightness, movement, aggression):
    return {
        "OVERWORLD": {
            "ow_macro_era":       rnd(0.55, 1.0),
            "ow_macro_chaos":     rnd(aggression * 0.5, aggression),
            "ow_macro_couple":    rnd(0.0, 0.4),
            "ow_macro_space":     rnd(0.3, 0.7),
            "ow_outputLevel":     rnd(0.78, 0.92),
            "ow_couplingLevel":   rnd(0.0, 0.35),
            "ow_couplingBus":     0,
        }
    }

def make_orbital_params(brightness, movement, aggression):
    return {
        "Orbital": {
            "orb_brightness":     rnd(brightness * 0.85, 1.0),
            "orb_groupEnvRate":   rnd(movement * 0.5, movement * 2.5, 3),
            "orb_groupEnvDepth":  rnd(0.5, 0.95),
            "orb_oscCount":       rnd_int(4, 8),
            "orb_spread":         rnd(0.4, 0.9),
            "orb_detune":         rnd(0.0, 0.5),
            "orb_filterCutoff":   rnd(brightness * 0.8, 1.0),
            "orb_filterReso":     rnd(0.2, 0.65),
            "orb_drive":          rnd(aggression * 0.4, aggression * 0.85),
            "orb_modRate":        rnd(movement * 1.0, movement * 5.0, 3),
            "orb_modDepth":       rnd(0.3, 0.8),
            "orb_reverbSize":     rnd(0.35, 0.75),
            "orb_reverbMix":      rnd(0.1, 0.45),
            "orb_outputLevel":    rnd(0.78, 0.92),
            "orb_couplingLevel":  rnd(0.0, 0.35),
            "orb_couplingBus":    0,
        }
    }

def make_onset_params(brightness, movement, aggression):
    return {
        "Onset": {
            "perc_v1_blend":      rnd(0.5, 1.0),
            "perc_v1_algoMode":   rnd_int(0, 3),
            "perc_v1_pitch":      rnd(brightness * 0.5, brightness),
            "perc_v1_decay":      rnd(0.05, 0.45),
            "perc_v1_tone":       rnd(brightness * 0.7, 1.0),
            "perc_v1_snap":       rnd(aggression * 0.55, aggression),
            "perc_v1_body":       rnd(0.3, 0.75),
            "perc_v1_character":  rnd(0.4, 0.9),
            "perc_v2_blend":      rnd(0.2, 0.7),
            "perc_v2_pitch":      rnd(0.5, 0.9),
            "perc_v2_decay":      rnd(0.03, 0.3),
            "perc_v2_tone":       rnd(brightness * 0.6, 0.95),
            "perc_v2_snap":       rnd(aggression * 0.4, aggression * 0.85),
            "perc_v2_body":       rnd(0.2, 0.6),
            "perc_machine":       rnd(0.55, 1.0),
            "perc_punch":         rnd(aggression * 0.5, aggression),
            "perc_space":         rnd(0.1, 0.55),
            "perc_mutate":        rnd(0.1, 0.6),
            "perc_outputLevel":   rnd(0.78, 0.92),
            "perc_couplingLevel": rnd(0.0, 0.35),
            "perc_couplingBus":   0,
        }
    }

def make_octopus_params(brightness, movement, aggression):
    return {
        "Octopus": {
            "octo_armCount":      rnd_int(4, 8),
            "octo_armSpread":     rnd(0.5, 1.0),
            "octo_armBaseRate":   rnd(movement * 0.8, movement * 3.5, 3),
            "octo_armDepth":      rnd(0.5, 0.95),
            "octo_chromaSens":    rnd(brightness * 0.6, 1.0),
            "octo_chromaSpeed":   rnd(movement * 0.5, movement * 2.0, 3),
            "octo_chromaMorph":   rnd(0.4, 0.9),
            "octo_chromaDepth":   rnd(0.5, 0.95),
            "octo_inkDrive":      rnd(aggression * 0.5, aggression),
            "octo_inkFilter":     rnd(brightness * 0.7, 1.0),
            "octo_reverbMix":     rnd(0.1, 0.5),
            "octo_outputLevel":   rnd(0.78, 0.92),
            "octo_couplingLevel": rnd(0.0, 0.35),
            "octo_couplingBus":   0,
        }
    }

def make_orphica_params(brightness, movement, aggression):
    # Orphica uses Ohm/Orphica-style macro params based on constellation engines
    return {
        "Orphica": {
            "orph_harpTension":    rnd(brightness * 0.7, 1.0),
            "orph_grainSize":      rnd(0.05, 0.4),
            "orph_grainDensity":   rnd(0.4, 0.9),
            "orph_grainPitch":     rnd(-0.2, 0.2),
            "orph_grainSpread":    rnd(0.5, 1.0),
            "orph_shimmerRate":    rnd(movement * 1.0, movement * 4.0, 3),
            "orph_shimmerDepth":   rnd(brightness * 0.5, brightness),
            "orph_filterCutoff":   rnd(brightness * 0.8, 1.0),
            "orph_filterReso":     rnd(0.2, 0.65),
            "orph_attack":         rnd(0.001, 0.02),
            "orph_decay":          rnd(0.3, 1.5),
            "orph_sustain":        rnd(0.5, 0.85),
            "orph_release":        rnd(0.3, 1.2),
            "orph_reverbSize":     rnd(0.45, 0.85),
            "orph_reverbMix":      rnd(0.2, 0.55),
            "orph_outputLevel":    rnd(0.78, 0.92),
            "orph_couplingLevel":  rnd(0.0, 0.35),
            "orph_couplingBus":    0,
        }
    }

def make_ocelot_params(brightness, movement, aggression):
    return {
        "Ocelot": {
            "ocelot_biome":           rnd_int(0, 3),
            "ocelot_strataBalance":   rnd(0.4, 0.9),
            "ocelot_ecosystemDepth":  rnd(0.4, 0.85),
            "ocelot_humidity":        rnd(0.0, 0.45),
            "ocelot_swing":           rnd(0.0, 0.35),
            "ocelot_density":         rnd(0.5, 0.9),
            "ocelot_filterCutoff":    rnd(brightness * 0.75, 1.0),
            "ocelot_brightness":      rnd(brightness * 0.85, 1.0),
            "ocelot_agility":         rnd(movement * 0.6, movement),
            "ocelot_predation":       rnd(aggression * 0.5, aggression),
            "ocelot_reverbMix":       rnd(0.1, 0.45),
            "ocelot_outputLevel":     rnd(0.78, 0.92),
            "ocelot_couplingLevel":   rnd(0.0, 0.35),
            "ocelot_couplingBus":     0,
        }
    }

def make_outwit_params(brightness, movement, aggression):
    return {
        "Outwit": {
            "owit_rule":          rnd_int(0, 255),
            "owit_armBalance":    rnd(0.3, 0.9),
            "owit_mutationRate":  rnd(aggression * 0.3, aggression * 0.75),
            "owit_armSpread":     rnd(0.5, 1.0),
            "owit_sync":          rnd(0.0, 0.6),
            "owit_feedbackAmt":   rnd(0.3, 0.8),
            "owit_filterCutoff":  rnd(brightness * 0.75, 1.0),
            "owit_filterReso":    rnd(0.2, 0.65),
            "owit_drive":         rnd(aggression * 0.45, aggression),
            "owit_modRate":       rnd(movement * 1.0, movement * 4.0, 3),
            "owit_reverbMix":     rnd(0.1, 0.4),
            "owit_outputLevel":   rnd(0.78, 0.92),
            "owit_couplingLevel": rnd(0.0, 0.35),
            "owit_couplingBus":   0,
        }
    }

ENGINE_PARAM_BUILDERS = {
    "OPTIC":      make_optic_params,
    "OBLIQUE":    make_oblique_params,
    "ODDFELIX":   make_oddfelix_params,
    "ORIGAMI":    make_origami_params,
    "OVERWORLD":  make_overworld_params,
    "ORBITAL":    make_orbital_params,
    "ONSET":      make_onset_params,
    "OCTOPUS":    make_octopus_params,
    "ORPHICA":    make_orphica_params,
    "OCELOT":     make_ocelot_params,
    "OUTWIT":     make_outwit_params,
}

# Engine display names (as used in existing presets)
ENGINE_DISPLAY = {
    "OPTIC":     "Optic",
    "OBLIQUE":   "Oblique",
    "ODDFELIX":  "OddfeliX",
    "ORIGAMI":   "ORIGAMI",
    "OVERWORLD": "OVERWORLD",
    "ORBITAL":   "Orbital",
    "ONSET":     "Onset",
    "OCTOPUS":   "Octopus",
    "ORPHICA":   "Orphica",
    "OCELOT":    "Ocelot",
    "OUTWIT":    "Outwit",
}

# ─── Preset name banks ────────────────────────────────────────────────────────

PRISM_NAMES = {
    "OPTIC": [
        "Photon Cascade",       "Laser Glass Burst",    "UV Pulse Storm",
        "Optic Solar Crown",    "Phosphor Ignition",    "Spectra Maximum",
        "Blinding Array",       "Coherent Light Veil",  "White Peak Flare",
        "Optical Overexpose",
    ],
    "OBLIQUE": [
        "Oblique Neon Surge",   "Prism Shatter White",  "Acute Refraction",
        "Oblique Apex Flash",   "Glass Lattice Burst",  "Bounce Ultraviolet",
        "Acute Angular Blaze",  "Oblique Solar Max",    "Spectral Bounce Riot",
        "Hyper-Oblique Prism",
    ],
    "ODDFELIX": [
        "Snapper Ultra",        "Snap Overexposed",     "OddfeliX Phosphor",
        "Crystal Snap Peak",    "Bright Morph Impact",  "White Velocity Hit",
        "Felix Photon Crack",   "Sharp Light Strike",   "Oversnap Glitter",
        "OddfeliX Apex",
    ],
    "ORIGAMI": [
        "Diamond Fold",         "Bright Crease Burst",  "White Paper Ignite",
        "Fold Line Prism",      "Taut Origami Flash",   "Tension Peak Crystal",
        "Refracted Fold",       "Radiant Paper Wing",   "Ultra-Fold Prism",
        "Origami Solar Flare",
    ],
    "OVERWORLD": [
        "Chip Overexposed",     "NES White Crown",      "Overworld Apex",
        "SNES Bright Storm",    "Retro Photon Core",    "Genesis Flash Wave",
        "Era Maximum Light",    "Chip Solar Pulse",     "8-Bit Blinding Arc",
        "Overworld Ultraviolet",
    ],
    "ORBITAL": [
        "Orbital Zenith Burst", "Solar Orbit Maximum",  "Aphelion Blaze",
        "Perihelion Flash",     "Orbital Light Crown",  "Bright Resonance Halo",
        "Orbit White Pulse",    "Stellar Overexpose",   "Orbital UV Ring",
        "Zenith Array",
    ],
    "ONSET": [
        "Snap Machine Max",     "Peak Transient Storm", "Drum Overexpose",
        "Crystal Snare Apex",   "Onset Solar Hit",      "Bright Drum Burst",
        "White Transient Wall", "Onset Photon Clap",    "Maximum Snap Impact",
        "Onset Radiance",
    ],
    "OCTOPUS": [
        "Octopus Bioluminescent", "Chroma Solar Burst", "Arm Light Storm",
        "Radiant Chromatophore",  "Ink Flash Maximum",  "Bright Eight-Arm Wave",
        "Octopus Neon Corona",    "Phosphor Arm Array", "Chroma Overexposed",
        "Octopus Light Apex",
    ],
    "ORPHICA": [
        "Orphica Solar String", "Harp Blinding Light",  "Siren Ultraviolet",
        "Bright Siphon Chord",  "Orphica Apex Shimmer", "Crystal Harp Storm",
        "Radiant Colony Pluck", "White Orphica Burst",  "Siren Overexposed",
        "Orphica Light Crown",
    ],
    "OCELOT": [
        "Ocelot Solar Pounce",  "Bright Predator Flash","Apex Ocelot Sprint",
        "Ocelot Photon Strike", "Crystal Hunter Burst", "Neon Canopy Apex",
        "Ocelot UV Maximum",    "Radiant Ecosystem",    "Bright Biome Storm",
        "Ocelot Light Peak",
    ],
}

FLUX_NAMES = {
    "OPTIC": [
        "Optic Flux Maximum",   "Pulse Storm Overload", "Reactive Bright Chaos",
        "Optic Melt White",     "Flash Rate Extreme",   "Optic Surge Apex",
        "Hyperflux Photon",     "Optic Pulse Apex",     "White Rate Flux",
        "Reactive Overexpose",  "Optic Solar Flux",     "Coherent Flux Burst",
    ],
    "OBLIQUE": [
        "Oblique Flux Storm",   "Bounce Chaos Maximum", "Oblique Melt Rate",
        "Prism Flux Shatter",   "Acute Flux Maximum",   "Oblique Surge White",
    ],
    "OVERWORLD": [
        "Overworld Flux Apex",  "Chip Chaos Maximum",   "ERA Flux Blaze",
    ],
    "ONSET": [
        "Onset Flux Maximum",   "Transient Chaos Storm","Machine Flux Blaze",
    ],
    "OCTOPUS": [
        "Octopus Flux Radiance","Chroma Chaos Maximum", "Arm Flux Blaze",
    ],
    "OUTWIT": [
        "Outwit Bright Chaos",  "Wolfram Flux Maximum", "Arm Overexposed",
        "CA Blaze Storm",       "Outwit Solar Surge",   "Arm Rate Maximum",
        "Outwit Photon Apex",   "Rule Flux White",      "Outwit Light Chaos",
    ],
    "ORBITAL": [
        "Orbital Flux Surge",   "Orbit Chaos Maximum",  "Bright Orbital Flux",
    ],
    "ORIGAMI": [
        "Origami Flux Blaze",   "Fold Chaos Maximum",   "Tension Flux White",
    ],
    "ODDFELIX": [
        "Felix Flux Maximum",   "Snap Chaos Storm",     "OddfeliX Flux White",
    ],
}

# ─── Preset definition lists ──────────────────────────────────────────────────

def _spread_engines(engine_list, count):
    """Distribute count presets round-robin across engine_list."""
    assignments = []
    for i in range(count):
        assignments.append(engine_list[i % len(engine_list)])
    random.shuffle(assignments)
    return assignments

PRISM_ENGINES = [
    "OPTIC", "OBLIQUE", "ODDFELIX", "ORIGAMI", "OVERWORLD",
    "ORBITAL", "ONSET", "OCTOPUS", "ORPHICA", "OCELOT",
]

FLUX_ENGINES = [
    "OPTIC", "OBLIQUE", "OVERWORLD", "ONSET", "OCTOPUS",
    "OUTWIT", "ORBITAL", "ORIGAMI", "OPTIC", "ODDFELIX",
]

# ─── Preset builder ───────────────────────────────────────────────────────────

def make_preset(name, mood, engine_key, brightness, movement, aggression):
    warmth    = rnd(0.05, 0.28)
    density   = rnd(0.30, 0.75)
    space     = rnd(0.25, 0.70)

    display   = ENGINE_DISPLAY[engine_key]
    builder   = ENGINE_PARAM_BUILDERS[engine_key]
    params    = builder(brightness, movement, aggression)

    # Coupling intensity label
    coup_level = list(params.values())[0].get(
        next((k for k in list(params.values())[0] if "coupling" in k.lower() and "bus" not in k.lower()), ""), 0
    )
    if coup_level > 0.6:
        coupling_intensity = "Heavy"
    elif coup_level > 0.3:
        coupling_intensity = "Moderate"
    else:
        coupling_intensity = "None"

    return {
        "schema_version": 1,
        "name":           name,
        "mood":           mood,
        "engines":        [display],
        "author":         "XO_OX Designs",
        "version":        "1.0.0",
        "description":    f"{display} pushed to extreme brightness — brightness {brightness:.2f}, "
                          f"movement {movement:.2f}, aggression {aggression:.2f}. "
                          f"Ultra-bright {mood} zone filler.",
        "tags":           [mood.lower(), "ultra-bright", "extreme", "brightness", engine_key.lower()],
        "macroLabels":    ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
        "couplingIntensity": coupling_intensity,
        "tempo":          None,
        "dna": {
            "brightness": brightness,
            "warmth":     warmth,
            "movement":   movement,
            "density":    density,
            "space":      space,
            "aggression": aggression,
        },
        "sonic_dna": {
            "brightness": brightness,
            "warmth":     warmth,
            "movement":   movement,
            "density":    density,
            "space":      space,
            "aggression": aggression,
        },
        "parameters": params,
        "coupling": {"pairs": []},
        "sequencer": None,
    }

def filename_for(name):
    safe = name.replace(" ", "_").replace("/", "-").replace("\\", "-")
    return f"{safe}.xometa"

# ─── Main ─────────────────────────────────────────────────────────────────────

def generate_prism(count=30):
    engines = _spread_engines(PRISM_ENGINES, count)
    name_counters = {e: 0 for e in PRISM_ENGINES}
    presets = []
    for i, eng in enumerate(engines):
        brightness = rnd(0.87, 0.98)
        movement   = rnd(0.55, 0.90)
        aggression = rnd(0.40, 0.80)
        pool       = PRISM_NAMES[eng]
        idx        = name_counters[eng] % len(pool)
        name       = pool[idx]
        name_counters[eng] += 1
        presets.append(make_preset(name, "Prism", eng, brightness, movement, aggression))
    return presets

def generate_flux(count=30):
    engines = _spread_engines(FLUX_ENGINES, count)
    name_counters = {e: 0 for e in set(FLUX_ENGINES)}
    presets = []
    for i, eng in enumerate(engines):
        brightness = rnd(0.88, 0.99)
        movement   = rnd(0.70, 0.95)
        aggression = rnd(0.50, 0.85)
        pool       = FLUX_NAMES.get(eng, [f"{ENGINE_DISPLAY[eng]} Flux Burst {i}"])
        idx        = name_counters[eng] % len(pool)
        name       = pool[idx]
        name_counters[eng] += 1
        presets.append(make_preset(name, "Flux", eng, brightness, movement, aggression))
    return presets

def save_presets(presets, directory):
    os.makedirs(directory, exist_ok=True)
    saved = 0
    skipped = 0
    for p in presets:
        fname = filename_for(p["name"])
        fpath = os.path.join(directory, fname)
        if os.path.exists(fpath):
            print(f"  SKIP (exists): {fname}")
            skipped += 1
            continue
        with open(fpath, "w") as f:
            json.dump(p, f, indent=2)
        print(f"  WROTE: {fname}")
        saved += 1
    return saved, skipped

def main():
    print("=== xpn_prism_flux_bright_extreme_pack ===")
    print()

    print(f"Generating 30 Prism ultra-bright presets → {PRISM_DIR}")
    prism = generate_prism(30)
    s, sk = save_presets(prism, PRISM_DIR)
    print(f"  Done: {s} saved, {sk} skipped\n")

    print(f"Generating 30 Flux ultra-bright presets → {FLUX_DIR}")
    flux  = generate_flux(30)
    s, sk = save_presets(flux, FLUX_DIR)
    print(f"  Done: {s} saved, {sk} skipped\n")

    # Quick DNA verification
    all_p = prism + flux
    min_b = min(p["dna"]["brightness"] for p in all_p)
    max_b = max(p["dna"]["brightness"] for p in all_p)
    print(f"DNA verification — brightness range: {min_b:.3f} – {max_b:.3f}")
    below_threshold = [p["name"] for p in all_p if p["dna"]["brightness"] < 0.87]
    if below_threshold:
        print(f"  WARNING: {len(below_threshold)} preset(s) below 0.87 brightness:")
        for n in below_threshold:
            print(f"    {n}")
    else:
        print("  All presets meet brightness ≥ 0.87 requirement.")

    print("\nComplete.")

if __name__ == "__main__":
    main()
