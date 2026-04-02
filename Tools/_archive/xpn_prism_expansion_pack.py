#!/usr/bin/env python3
"""
XOceanus Prism Mood Expansion Pack
Generates 60 Prism presets across three sub-categories:
  - Spectral (20): pristine prismatic, high brightness, low aggression
  - Refraction (20): light-in-motion, medium brightness, high movement
  - Crystal (20): sparse bright crystalline, very high brightness, low density

Writes to Presets/XOceanus/Prism/ — skips existing files.
"""

import json
import os
import sys
from pathlib import Path

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------
SCRIPT_DIR = Path(__file__).resolve().parent
REPO_ROOT = SCRIPT_DIR.parent
OUTPUT_DIR = REPO_ROOT / "Presets" / "XOceanus" / "Prism"
OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

# ---------------------------------------------------------------------------
# Engine parameter templates (minimal, engine-idiomatic)
# ---------------------------------------------------------------------------

def oddfelix_params(brightness: float, movement: float, space: float, aggression: float) -> dict:
    return {
        "oddfelix_harmonicBlend": round(brightness * 0.9, 3),
        "oddfelix_spectralTilt": round(brightness * 0.7, 3),
        "oddfelix_pitchMod": round(movement * 0.4, 3),
        "oddfelix_lfoRate": round(0.5 + movement * 4.5, 3),
        "oddfelix_lfoDepth": round(movement * 0.3, 3),
        "oddfelix_reverbMix": round(space * 0.7, 3),
        "oddfelix_filterCutoff": round(4000 + brightness * 14000, 1),
        "oddfelix_filterReso": round(0.1 + aggression * 0.3, 3),
        "oddfelix_ampAttack": round(0.003 + (1 - brightness) * 0.05, 4),
        "oddfelix_ampDecay": round(0.4 + space * 0.8, 3),
        "oddfelix_ampSustain": round(0.5 + brightness * 0.3, 3),
        "oddfelix_ampRelease": round(0.8 + space * 1.5, 3),
        "oddfelix_stereoWidth": round(0.5 + space * 0.4, 3),
        "oddfelix_level": 0.8,
        "oddfelix_macroCharacter": round(brightness * 0.8, 3),
        "oddfelix_macroMovement": round(movement, 3),
        "oddfelix_macroCoupling": 0.0,
        "oddfelix_macroSpace": round(space, 3),
    }


def overworld_params(brightness: float, movement: float, space: float, aggression: float) -> dict:
    return {
        "ow_eraBlend": round(brightness * 0.8, 3),
        "ow_oscillatorMix": round(0.5 + aggression * 0.3, 3),
        "ow_glitchIntensity": round(aggression * 0.25, 3),
        "ow_filterCutoff": round(3000 + brightness * 15000, 1),
        "ow_filterReso": round(0.08 + aggression * 0.25, 3),
        "ow_lfoRate": round(0.3 + movement * 5.0, 3),
        "ow_lfoDepth": round(movement * 0.35, 3),
        "ow_reverbMix": round(space * 0.65, 3),
        "ow_ampAttack": round(0.004 + (1 - brightness) * 0.04, 4),
        "ow_ampDecay": round(0.35 + space * 0.7, 3),
        "ow_ampSustain": round(0.45 + brightness * 0.35, 3),
        "ow_ampRelease": round(0.7 + space * 1.4, 3),
        "ow_stereoWidth": round(0.45 + space * 0.45, 3),
        "ow_level": 0.8,
        "ow_macroCharacter": round(brightness * 0.75, 3),
        "ow_macroMovement": round(movement, 3),
        "ow_macroCoupling": 0.0,
        "ow_macroSpace": round(space, 3),
    }


def optic_params(brightness: float, movement: float, space: float, aggression: float) -> dict:
    return {
        "optic_dispersion": round(brightness * 0.85, 3),
        "optic_chromaticShift": round(brightness * 0.6, 3),
        "optic_modulationRate": round(0.4 + movement * 4.8, 3),
        "optic_modulationDepth": round(movement * 0.4, 3),
        "optic_filterCutoff": round(5000 + brightness * 13000, 1),
        "optic_filterReso": round(0.1 + aggression * 0.2, 3),
        "optic_reverbMix": round(space * 0.75, 3),
        "optic_ampAttack": round(0.002 + (1 - brightness) * 0.04, 4),
        "optic_ampDecay": round(0.3 + space * 0.9, 3),
        "optic_ampSustain": round(0.5 + brightness * 0.3, 3),
        "optic_ampRelease": round(0.9 + space * 1.6, 3),
        "optic_stereoWidth": round(0.55 + space * 0.4, 3),
        "optic_level": 0.78,
        "optic_macroCharacter": round(brightness * 0.8, 3),
        "optic_macroMovement": round(movement, 3),
        "optic_macroCoupling": 0.0,
        "optic_macroSpace": round(space, 3),
    }


def oblique_params(brightness: float, movement: float, space: float, aggression: float) -> dict:
    return {
        "oblique_angle": round(brightness * 0.7, 3),
        "oblique_harmonicDrift": round(movement * 0.5, 3),
        "oblique_lfoRate": round(0.6 + movement * 3.8, 3),
        "oblique_lfoDepth": round(movement * 0.3, 3),
        "oblique_filterCutoff": round(4500 + brightness * 12000, 1),
        "oblique_filterReso": round(0.12 + aggression * 0.22, 3),
        "oblique_reverbMix": round(space * 0.7, 3),
        "oblique_ampAttack": round(0.003 + (1 - brightness) * 0.045, 4),
        "oblique_ampDecay": round(0.4 + space * 0.75, 3),
        "oblique_ampSustain": round(0.5 + brightness * 0.28, 3),
        "oblique_ampRelease": round(0.85 + space * 1.4, 3),
        "oblique_stereoWidth": round(0.5 + space * 0.42, 3),
        "oblique_level": 0.79,
        "oblique_macroCharacter": round(brightness * 0.78, 3),
        "oblique_macroMovement": round(movement, 3),
        "oblique_macroCoupling": 0.0,
        "oblique_macroSpace": round(space, 3),
    }


def origami_params(brightness: float, movement: float, space: float, aggression: float) -> dict:
    return {
        "origami_foldDepth": round(brightness * 0.75, 3),
        "origami_creaseScan": round(movement * 0.6, 3),
        "origami_unfoldRate": round(0.3 + movement * 4.2, 3),
        "origami_filterCutoff": round(4000 + brightness * 13500, 1),
        "origami_filterReso": round(0.08 + aggression * 0.2, 3),
        "origami_reverbMix": round(space * 0.72, 3),
        "origami_ampAttack": round(0.005 + (1 - brightness) * 0.04, 4),
        "origami_ampDecay": round(0.45 + space * 0.8, 3),
        "origami_ampSustain": round(0.48 + brightness * 0.3, 3),
        "origami_ampRelease": round(0.9 + space * 1.5, 3),
        "origami_stereoWidth": round(0.5 + space * 0.43, 3),
        "origami_level": 0.78,
        "origami_macroCharacter": round(brightness * 0.82, 3),
        "origami_macroMovement": round(movement, 3),
        "origami_macroCoupling": 0.0,
        "origami_macroSpace": round(space, 3),
    }


def osprey_params(brightness: float, movement: float, space: float, aggression: float) -> dict:
    return {
        "osprey_soarHeight": round(brightness * 0.85, 3),
        "osprey_wingFlutter": round(movement * 0.5, 3),
        "osprey_thermalDrift": round(0.4 + movement * 3.5, 3),
        "osprey_filterCutoff": round(5000 + brightness * 13000, 1),
        "osprey_filterReso": round(0.09 + aggression * 0.18, 3),
        "osprey_reverbMix": round(space * 0.8, 3),
        "osprey_ampAttack": round(0.004 + (1 - brightness) * 0.03, 4),
        "osprey_ampDecay": round(0.5 + space * 0.9, 3),
        "osprey_ampSustain": round(0.52 + brightness * 0.3, 3),
        "osprey_ampRelease": round(1.0 + space * 1.6, 3),
        "osprey_stereoWidth": round(0.55 + space * 0.4, 3),
        "osprey_level": 0.77,
        "osprey_macroCharacter": round(brightness * 0.8, 3),
        "osprey_macroMovement": round(movement, 3),
        "osprey_macroCoupling": 0.0,
        "osprey_macroSpace": round(space, 3),
    }


def oracle_params(brightness: float, movement: float, space: float, aggression: float) -> dict:
    return {
        "oracle_visionDepth": round(brightness * 0.8, 3),
        "oracle_propheticShimmer": round(movement * 0.55, 3),
        "oracle_temporalRate": round(0.5 + movement * 4.0, 3),
        "oracle_filterCutoff": round(4500 + brightness * 13000, 1),
        "oracle_filterReso": round(0.1 + aggression * 0.22, 3),
        "oracle_reverbMix": round(space * 0.78, 3),
        "oracle_ampAttack": round(0.004 + (1 - brightness) * 0.05, 4),
        "oracle_ampDecay": round(0.5 + space * 0.85, 3),
        "oracle_ampSustain": round(0.5 + brightness * 0.32, 3),
        "oracle_ampRelease": round(1.0 + space * 1.5, 3),
        "oracle_stereoWidth": round(0.52 + space * 0.42, 3),
        "oracle_level": 0.78,
        "oracle_macroCharacter": round(brightness * 0.78, 3),
        "oracle_macroMovement": round(movement, 3),
        "oracle_macroCoupling": 0.0,
        "oracle_macroSpace": round(space, 3),
    }


def obsidian_params(brightness: float, movement: float, space: float, aggression: float) -> dict:
    return {
        "obsidian_densityX": round(0.3 + brightness * 0.5, 3),
        "obsidian_tiltY": round(brightness * 0.75, 3),
        "obsidian_depth": round(0.5 + brightness * 0.4, 3),
        "obsidian_stiffness": round(0.2 + aggression * 0.5, 3),
        "obsidian_cascadeBlend": round(movement * 0.35, 3),
        "obsidian_crossModDepth": round(aggression * 0.3, 3),
        "obsidian_formantIntensity": round(brightness * 0.45, 3),
        "obsidian_stereoWidth": round(0.5 + space * 0.42, 3),
        "obsidian_filterCutoff": round(5000 + brightness * 13000, 1),
        "obsidian_filterReso": round(0.08 + aggression * 0.2, 3),
        "obsidian_level": 0.78,
        "obsidian_ampAttack": round(0.003 + (1 - brightness) * 0.04, 4),
        "obsidian_ampDecay": round(0.5 + space * 0.8, 3),
        "obsidian_ampSustain": round(0.48 + brightness * 0.3, 3),
        "obsidian_ampRelease": round(0.9 + space * 1.4, 3),
        "obsidian_lfo1Rate": round(0.5 + movement * 5.0, 3),
        "obsidian_lfo1Depth": round(movement * 0.25, 3),
        "obsidian_lfo1Shape": 2,
        "obsidian_lfo2Rate": round(0.3 + movement * 3.0, 3),
        "obsidian_lfo2Depth": round(movement * 0.15, 3),
        "obsidian_lfo2Shape": 0,
        "obsidian_polyphony": 4,
        "obsidian_glide": 0.0,
        "obsidian_macroCharacter": round(brightness * 0.8, 3),
        "obsidian_macroMovement": round(movement, 3),
        "obsidian_macroCoupling": 0.0,
        "obsidian_macroSpace": round(space, 3),
    }


def ocelot_params(brightness: float, movement: float, space: float, aggression: float) -> dict:
    return {
        "ocelot_huntDepth": round(brightness * 0.7, 3),
        "ocelot_stalkerRate": round(0.4 + movement * 4.2, 3),
        "ocelot_filterCutoff": round(3500 + brightness * 14000, 1),
        "ocelot_filterReso": round(0.1 + aggression * 0.3, 3),
        "ocelot_reverbMix": round(space * 0.65, 3),
        "ocelot_ampAttack": round(0.005 + (1 - brightness) * 0.04, 4),
        "ocelot_ampDecay": round(0.4 + space * 0.75, 3),
        "ocelot_ampSustain": round(0.5 + brightness * 0.28, 3),
        "ocelot_ampRelease": round(0.8 + space * 1.35, 3),
        "ocelot_stereoWidth": round(0.48 + space * 0.42, 3),
        "ocelot_level": 0.79,
        "ocelot_macroCharacter": round(brightness * 0.75, 3),
        "ocelot_macroMovement": round(movement, 3),
        "ocelot_macroCoupling": 0.0,
        "ocelot_macroSpace": round(space, 3),
    }


def onset_params(brightness: float, movement: float, space: float, aggression: float) -> dict:
    return {
        "onset_kickTone": round(brightness * 0.5, 3),
        "onset_snareSnap": round(aggression * 0.6, 3),
        "onset_hihatBrightness": round(brightness * 0.85, 3),
        "onset_machine": round(0.4 + brightness * 0.4, 3),
        "onset_punch": round(aggression * 0.7, 3),
        "onset_space": round(space * 0.8, 3),
        "onset_mutate": round(movement * 0.4, 3),
        "onset_level": 0.78,
        "onset_macroCharacter": round(brightness * 0.7, 3),
        "onset_macroMovement": round(movement, 3),
        "onset_macroCoupling": 0.0,
        "onset_macroSpace": round(space, 3),
    }


def ohm_params(brightness: float, movement: float, space: float, aggression: float) -> dict:
    return {
        "ohm_resonantPeak": round(brightness * 0.75, 3),
        "ohm_harmonicContent": round(brightness * 0.65, 3),
        "ohm_communeDepth": round(movement * 0.55, 3),
        "ohm_meddlingRate": round(0.3 + movement * 4.5, 3),
        "ohm_filterCutoff": round(3000 + brightness * 15000, 1),
        "ohm_filterReso": round(0.08 + aggression * 0.25, 3),
        "ohm_reverbMix": round(space * 0.72, 3),
        "ohm_ampAttack": round(0.005 + (1 - brightness) * 0.05, 4),
        "ohm_ampDecay": round(0.5 + space * 0.8, 3),
        "ohm_ampSustain": round(0.5 + brightness * 0.3, 3),
        "ohm_ampRelease": round(1.0 + space * 1.5, 3),
        "ohm_stereoWidth": round(0.5 + space * 0.4, 3),
        "ohm_level": 0.78,
        "ohm_macroCharacter": round(brightness * 0.78, 3),
        "ohm_macroMovement": round(movement, 3),
        "ohm_macroCoupling": 0.0,
        "ohm_macroSpace": round(space, 3),
    }


def orphica_params(brightness: float, movement: float, space: float, aggression: float) -> dict:
    return {
        "orph_stringTension": round(brightness * 0.8, 3),
        "orph_microsoundDensity": round(0.2 + aggression * 0.3, 3),
        "orph_grainRate": round(0.4 + movement * 4.0, 3),
        "orph_grainScatter": round(movement * 0.5, 3),
        "orph_filterCutoff": round(5000 + brightness * 13000, 1),
        "orph_filterReso": round(0.07 + aggression * 0.18, 3),
        "orph_reverbMix": round(space * 0.82, 3),
        "orph_ampAttack": round(0.003 + (1 - brightness) * 0.04, 4),
        "orph_ampDecay": round(0.55 + space * 0.9, 3),
        "orph_ampSustain": round(0.52 + brightness * 0.3, 3),
        "orph_ampRelease": round(1.1 + space * 1.6, 3),
        "orph_stereoWidth": round(0.55 + space * 0.42, 3),
        "orph_level": 0.77,
        "orph_macroCharacter": round(brightness * 0.82, 3),
        "orph_macroMovement": round(movement, 3),
        "orph_macroCoupling": 0.0,
        "orph_macroSpace": round(space, 3),
    }


ENGINE_PARAM_BUILDERS = {
    "ODDFELIX":  ("ODDFELIX",  oddfelix_params),
    "OVERWORLD": ("OVERWORLD", overworld_params),
    "OPTIC":     ("OPTIC",     optic_params),
    "OBLIQUE":   ("OBLIQUE",   oblique_params),
    "ORIGAMI":   ("ORIGAMI",   origami_params),
    "OSPREY":    ("OSPREY",    osprey_params),
    "ORACLE":    ("ORACLE",    oracle_params),
    "OBSIDIAN":  ("OBSIDIAN",  obsidian_params),
    "OCELOT":    ("OCELOT",    ocelot_params),
    "ONSET":     ("ONSET",     onset_params),
    "OHM":       ("OHM",       ohm_params),
    "ORPHICA":   ("ORPHICA",   orphica_params),
}

ENGINE_DISPLAY_NAMES = {
    "ODDFELIX":  "OddFelix",
    "OVERWORLD": "Overworld",
    "OPTIC":     "Optic",
    "OBLIQUE":   "Oblique",
    "ORIGAMI":   "Origami",
    "OSPREY":    "Osprey",
    "ORACLE":    "Oracle",
    "OBSIDIAN":  "Obsidian",
    "OCELOT":    "Ocelot",
    "ONSET":     "Onset",
    "OHM":       "Ohm",
    "ORPHICA":   "Orphica",
}

# ---------------------------------------------------------------------------
# Preset definitions
# ---------------------------------------------------------------------------

SPECTRAL_PRESETS = [
    # (name, engine, brightness, warmth, movement, density, space, aggression, tags, description)
    ("Photon Cascade",    "ODDFELIX",  0.92, 0.18, 0.35, 0.40, 0.72, 0.12,
     ["spectral", "harmonic", "bright", "prismatic"],
     "Harmonic overtones cascade like photons through a prism — pure spectral radiance."),
    ("Prismatic Dawn",    "OVERWORLD", 0.88, 0.22, 0.28, 0.38, 0.68, 0.10,
     ["spectral", "dawn", "bright", "airy"],
     "First light splitting through atmospheric crystal — chromatic dawn chorus."),
    ("Spectral Lattice",  "OPTIC",     0.90, 0.15, 0.30, 0.42, 0.65, 0.15,
     ["spectral", "lattice", "crystalline", "bright"],
     "Tonal grid of pure overtones — light caught in a crystalline matrix."),
    ("Iridescent Field",  "OBLIQUE",   0.85, 0.25, 0.32, 0.35, 0.75, 0.18,
     ["spectral", "iridescent", "shimmer", "prismatic"],
     "Oblique light strikes thin film — iridescent interference patterns bloom."),
    ("Radiant Apex",      "ORIGAMI",   0.93, 0.12, 0.25, 0.38, 0.70, 0.11,
     ["spectral", "radiant", "apex", "bright"],
     "Folded harmonics unfurl at maximum luminance — a spectral peak unbroken."),
    ("Chromatic Aria",    "OSPREY",    0.87, 0.20, 0.30, 0.40, 0.78, 0.13,
     ["spectral", "chromatic", "melodic", "soaring"],
     "Soaring chromatic melody caught on thermals of pure spectral air."),
    ("Oracle Spectrum",   "ORACLE",    0.91, 0.17, 0.28, 0.45, 0.65, 0.14,
     ["spectral", "oracle", "prophecy", "prismatic"],
     "Prophetic overtones shimmer in crystalline suspension — the oracle speaks light."),
    ("Obsidian Glare",    "OBSIDIAN",  0.89, 0.14, 0.22, 0.50, 0.60, 0.20,
     ["spectral", "obsidian", "glare", "metallic"],
     "Volcanic glass refracts cold spectral fire — cutting prismatic edges."),
    ("Diffraction Bloom", "ODDFELIX",  0.86, 0.22, 0.35, 0.38, 0.74, 0.16,
     ["spectral", "diffraction", "bloom", "harmonic"],
     "Wave interference blooms outward — tonal diffraction painting light on air."),
    ("Luminance Arc",     "OVERWORLD", 0.94, 0.12, 0.26, 0.35, 0.68, 0.09,
     ["spectral", "luminance", "arc", "bright"],
     "Maximal luminance sweeps a gentle arc — spectral purity in slow motion."),
    ("Prism Veil",        "OPTIC",     0.83, 0.28, 0.32, 0.42, 0.76, 0.17,
     ["spectral", "veil", "translucent", "shimmer"],
     "Light through translucent membrane — soft prismatic veil drifts in space."),
    ("Interference Tone", "OBLIQUE",   0.88, 0.18, 0.27, 0.40, 0.70, 0.15,
     ["spectral", "interference", "tonal", "precise"],
     "Two pure tones in constructive interference — standing wave of perfect clarity."),
    ("Solar Harmonic",    "ORIGAMI",   0.90, 0.20, 0.28, 0.36, 0.66, 0.12,
     ["spectral", "solar", "harmonic", "warm-bright"],
     "Solar spectrum folded into harmonic structure — ordered radiance, folded tight."),
    ("Frequency Bloom",   "OSPREY",    0.85, 0.24, 0.33, 0.38, 0.73, 0.14,
     ["spectral", "frequency", "bloom", "soaring"],
     "Frequency bands bloom open like wings — osprey riding the spectral thermal."),
    ("Tonal Prism",       "ORACLE",    0.87, 0.19, 0.29, 0.44, 0.67, 0.16,
     ["spectral", "tonal", "prism", "oracular"],
     "Pure tones refracted through oracular prism — six colors of harmonic truth."),
    ("Glass Harmonic",    "OBSIDIAN",  0.92, 0.13, 0.20, 0.48, 0.62, 0.18,
     ["spectral", "glass", "harmonic", "crystalline"],
     "Obsidian ground to glass thinness — harmonic overtones ring like crystal bowls."),
    ("Spectrum Peak",     "ODDFELIX",  0.95, 0.10, 0.22, 0.35, 0.64, 0.10,
     ["spectral", "peak", "pure", "maximal"],
     "Upper limit of tonal brightness — the spectral peak at full luminance."),
    ("Phosphene Wash",    "OVERWORLD", 0.84, 0.26, 0.31, 0.40, 0.77, 0.13,
     ["spectral", "phosphene", "wash", "neural"],
     "Light-behind-eyes phenomenon mapped to tone — phosphene wash of pure frequency."),
    ("Wavelength Pool",   "OPTIC",     0.86, 0.21, 0.30, 0.37, 0.71, 0.15,
     ["spectral", "wavelength", "pool", "still"],
     "Still pool of standing wavelengths — spectral calm before prismatic storm."),
    ("Aureole Drift",     "OBLIQUE",   0.83, 0.27, 0.34, 0.36, 0.78, 0.11,
     ["spectral", "aureole", "drift", "halo"],
     "Luminous halo drifts in slow spectral tide — prismatic aureole in open space."),
]

REFRACTION_PRESETS = [
    # (name, engine, brightness, warmth, movement, density, space, aggression, tags, description)
    ("Chromatic Drift",   "ORIGAMI",   0.78, 0.32, 0.85, 0.45, 0.70, 0.22,
     ["refraction", "chromatic", "drift", "motion"],
     "Light bends through density shift — chromatic drift tracing the refraction line."),
    ("Angle of Incidence","OPTIC",     0.72, 0.30, 0.80, 0.42, 0.68, 0.20,
     ["refraction", "angle", "incident", "bending"],
     "Precise angle of incidence — tones bend at the interface between two worlds."),
    ("Snell's Cascade",   "OBLIQUE",   0.75, 0.35, 0.88, 0.40, 0.72, 0.25,
     ["refraction", "cascade", "Snell", "physics"],
     "Snell's law in sound — frequency cascades through successive density boundaries."),
    ("Dispersive Wave",   "ODDFELIX",  0.80, 0.28, 0.78, 0.44, 0.65, 0.18,
     ["refraction", "dispersive", "wave", "spreading"],
     "Wave dispersion in motion — high frequencies race ahead in prismatic scatter."),
    ("Mirage Layer",      "OVERWORLD", 0.76, 0.38, 0.82, 0.46, 0.75, 0.20,
     ["refraction", "mirage", "thermal", "wavering"],
     "Thermal mirage bends the horizon — shimmering tonal layer in constant motion."),
    ("Lenticular Pulse",  "OCELOT",    0.70, 0.40, 0.88, 0.50, 0.68, 0.28,
     ["refraction", "lenticular", "pulse", "rhythmic"],
     "Lens-shaped pressure wave pulsing through medium — rhythmic lenticular refraction."),
    ("Onset Scatter",     "ONSET",     0.65, 0.35, 0.90, 0.55, 0.60, 0.30,
     ["refraction", "onset", "scatter", "percussive"],
     "Percussive scatter through prismatic space — onset energy refracting into many angles."),
    ("Commune Refraction","OHM",       0.72, 0.42, 0.76, 0.45, 0.78, 0.18,
     ["refraction", "commune", "resonant", "warm"],
     "Communal resonance bending through shared medium — ohmic refraction, warm and wide."),
    ("Prismatic Current",  "ORIGAMI",  0.74, 0.36, 0.84, 0.43, 0.70, 0.22,
     ["refraction", "prismatic", "current", "flowing"],
     "Folded light currents in motion — prismatic flow that bends and returns."),
    ("Temporal Bend",     "OPTIC",     0.77, 0.31, 0.79, 0.41, 0.73, 0.19,
     ["refraction", "temporal", "bend", "warping"],
     "Time itself bent at the optical interface — temporal refraction of slow harmonics."),
    ("Velocity Prism",    "OBLIQUE",   0.68, 0.38, 0.86, 0.48, 0.65, 0.26,
     ["refraction", "velocity", "prism", "speed"],
     "Different velocities create different bending angles — velocity prism in full motion."),
    ("Wavefront Fold",    "ODDFELIX",  0.73, 0.33, 0.82, 0.42, 0.68, 0.21,
     ["refraction", "wavefront", "fold", "interference"],
     "Wavefront folds back on itself at density boundary — interference cascade unfolds."),
    ("Shimmer Transit",   "OVERWORLD", 0.78, 0.30, 0.75, 0.40, 0.76, 0.17,
     ["refraction", "shimmer", "transit", "passing"],
     "Transit through shimmering atmosphere — light in constant refractive passage."),
    ("Ocelot Flicker",    "OCELOT",    0.71, 0.36, 0.91, 0.52, 0.62, 0.29,
     ["refraction", "flicker", "hunting", "movement"],
     "Fast flicker through dappled light — hunting movement in prismatic forest."),
    ("Machine Scatter",   "ONSET",     0.66, 0.32, 0.87, 0.58, 0.58, 0.32,
     ["refraction", "machine", "scatter", "mechanical"],
     "Machine strikes scatter light — mechanical refraction in bright percussive space."),
    ("OhmLight Bend",     "OHM",       0.75, 0.45, 0.80, 0.43, 0.80, 0.16,
     ["refraction", "ohm", "bend", "resonant"],
     "Resonant frequency bends at ohmic threshold — warm light in refractive communion."),
    ("Index Shift",       "ORIGAMI",   0.70, 0.40, 0.88, 0.45, 0.67, 0.24,
     ["refraction", "index", "shift", "transitional"],
     "Refractive index shifts mid-phrase — tonal character transforms as light bends."),
    ("Color Dispersion",  "OPTIC",     0.79, 0.27, 0.77, 0.40, 0.74, 0.18,
     ["refraction", "color", "dispersion", "spectrum"],
     "White light dispersed into full spectrum — each frequency finds its own path."),
    ("Oblique Entry",     "OBLIQUE",   0.67, 0.42, 0.90, 0.50, 0.63, 0.27,
     ["refraction", "oblique", "entry", "steep"],
     "Steep oblique entry through the medium — maximum bend at near-horizontal angle."),
    ("OddLight Run",      "ODDFELIX",  0.76, 0.34, 0.83, 0.44, 0.69, 0.20,
     ["refraction", "oddlight", "run", "fluid"],
     "OddFelix harmonics in fluid refractive motion — spectral run through shifting medium."),
]

CRYSTAL_PRESETS = [
    # (name, engine, brightness, warmth, movement, density, space, aggression, tags, description)
    ("Quartz Lattice",    "ODDFELIX",  0.95, 0.08, 0.22, 0.25, 0.80, 0.08,
     ["crystal", "quartz", "lattice", "pure"],
     "Atomic quartz structure resonating at pure frequency — perfect crystalline lattice."),
    ("Obsidian Point",    "OBSIDIAN",  0.92, 0.10, 0.18, 0.30, 0.75, 0.12,
     ["crystal", "obsidian", "point", "sharp"],
     "Volcanic glass worked to a razor point — crystalline precision at maximum brightness."),
    ("Frost Needle",      "OVERWORLD", 0.90, 0.12, 0.20, 0.22, 0.82, 0.07,
     ["crystal", "frost", "needle", "sparse"],
     "Single ice needle hanging in clear air — sparse crystalline tone of winter light."),
    ("Facet Array",       "OPTIC",     0.96, 0.06, 0.24, 0.28, 0.78, 0.09,
     ["crystal", "facet", "array", "geometric"],
     "Array of precisely cut facets — each one casting its own bright sliver of light."),
    ("Diamond Harmonic",  "OBLIQUE",   0.93, 0.09, 0.19, 0.24, 0.83, 0.10,
     ["crystal", "diamond", "harmonic", "pure"],
     "Diamond lattice mapped to harmonic series — hardest crystal, purest tone."),
    ("Hoarfrost Bell",    "OSPREY",    0.88, 0.14, 0.26, 0.20, 0.85, 0.06,
     ["crystal", "hoarfrost", "bell", "delicate"],
     "Hoarfrost on bell metal at dawn — delicate crystal ringing in cold still air."),
    ("Ice Cathedral",     "ORIGAMI",   0.87, 0.15, 0.23, 0.28, 0.88, 0.08,
     ["crystal", "ice", "cathedral", "spacious"],
     "Ice architecture on massive scale — vast cathedral of frozen crystal silence."),
    ("Orphica Shard",     "ORPHICA",   0.91, 0.11, 0.30, 0.22, 0.84, 0.07,
     ["crystal", "orphica", "shard", "microsound"],
     "Microsound harp struck at crystalline frequencies — grain-sized shards of pure light."),
    ("Selenite Column",   "ODDFELIX",  0.89, 0.13, 0.21, 0.26, 0.79, 0.09,
     ["crystal", "selenite", "column", "translucent"],
     "Selenite column transmitting lunar light — translucent crystal pillar, softly glowing."),
    ("Beryl Resonance",   "OBSIDIAN",  0.93, 0.09, 0.17, 0.32, 0.76, 0.11,
     ["crystal", "beryl", "resonance", "gem"],
     "Beryl struck at resonant frequency — gemstone singing its one pure crystalline note."),
    ("Clear Quartz Run",  "OVERWORLD", 0.94, 0.08, 0.28, 0.20, 0.80, 0.08,
     ["crystal", "clear", "quartz", "run"],
     "Running passage in clear quartz register — fast and pure as light through glass."),
    ("Tourmaline Pulse",  "OPTIC",     0.85, 0.18, 0.35, 0.25, 0.77, 0.11,
     ["crystal", "tourmaline", "pulse", "color"],
     "Tourmaline pulses its rainbow of stored light — multi-spectral crystal heartbeat."),
    ("Glacier Shard",     "OBLIQUE",   0.88, 0.12, 0.22, 0.23, 0.87, 0.07,
     ["crystal", "glacier", "shard", "cold"],
     "Broken glacier shard hanging in blue silence — cold crystal tone, ancient and pure."),
    ("Osprey Ice",        "OSPREY",    0.90, 0.10, 0.24, 0.21, 0.86, 0.06,
     ["crystal", "osprey", "ice", "soaring"],
     "Osprey cry in frozen air — crystalline call that rings across ice-silence."),
    ("Folded Crystal",    "ORIGAMI",   0.86, 0.16, 0.27, 0.30, 0.81, 0.10,
     ["crystal", "folded", "origami", "geometric"],
     "Crystal folded along perfect cleavage planes — geometric light trap, folded tight."),
    ("Orphica Frost",     "ORPHICA",   0.94, 0.08, 0.32, 0.18, 0.88, 0.06,
     ["crystal", "orphica", "frost", "grain"],
     "Frost grain microsounds in crystalline suspension — orphic harp at maximum brightness."),
    ("Diamond Dust",      "ODDFELIX",  0.97, 0.05, 0.20, 0.15, 0.85, 0.06,
     ["crystal", "diamond", "dust", "extreme"],
     "Diamond dust suspended in near-zero air — extreme upper brightness limit, pure scatter."),
    ("Calcite Window",    "OBSIDIAN",  0.83, 0.20, 0.25, 0.35, 0.78, 0.13,
     ["crystal", "calcite", "window", "birefringent"],
     "Calcite double-refraction through thin window — birefringent crystal splitting each tone."),
    ("Snow Quartz",       "OVERWORLD", 0.80, 0.22, 0.30, 0.38, 0.82, 0.09,
     ["crystal", "snow", "quartz", "cloudy"],
     "Snow quartz — slightly milky, slightly warm, still unmistakably crystalline."),
    ("Crystal Zero",      "OPTIC",     0.99, 0.03, 0.15, 0.12, 0.90, 0.04,
     ["crystal", "zero", "pure", "maximal"],
     "Crystal at absolute zero — theoretical maximum brightness, perfect silence between notes."),
]


# ---------------------------------------------------------------------------
# Preset writer
# ---------------------------------------------------------------------------

def make_preset(name: str, engine_key: str, brightness: float, warmth: float,
                movement: float, density: float, space: float, aggression: float,
                tags: list, description: str, sub_category: str) -> dict:
    _, builder = ENGINE_PARAM_BUILDERS[engine_key]
    display = ENGINE_DISPLAY_NAMES[engine_key]
    params = builder(brightness, movement, space, aggression)

    return {
        "schema_version": 1,
        "name": name,
        "mood": "Prism",
        "engines": [display],
        "author": "XO_OX",
        "version": "1.0.0",
        "description": description,
        "tags": tags + ["prism"],
        "macroLabels": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
        "couplingIntensity": "None",
        "tempo": None,
        "created": "2026-03-16",
        "legacy": {
            "sourceInstrument": display,
            "sourceCategory": "Prism",
            "sourcePresetName": None,
        },
        "parameters": {display: params},
        "coupling": None,
        "sequencer": None,
        "dna": {
            "brightness": round(brightness, 3),
            "warmth": round(warmth, 3),
            "movement": round(movement, 3),
            "density": round(density, 3),
            "space": round(space, 3),
            "aggression": round(aggression, 3),
        },
    }


def filename_for(name: str) -> str:
    return name.replace("/", "-").replace("\\", "-") + ".xometa"


def write_preset(preset: dict, output_dir: Path) -> tuple[bool, str]:
    fname = filename_for(preset["name"])
    fpath = output_dir / fname
    if fpath.exists():
        return False, fname
    with open(fpath, "w", encoding="utf-8") as f:
        json.dump(preset, f, indent=2, ensure_ascii=False)
        f.write("\n")
    return True, fname


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    written = 0
    skipped = 0
    errors = []

    all_batches = [
        ("Spectral", SPECTRAL_PRESETS),
        ("Refraction", REFRACTION_PRESETS),
        ("Crystal", CRYSTAL_PRESETS),
    ]

    for sub_cat, batch in all_batches:
        print(f"\n--- {sub_cat} ({len(batch)} presets) ---")
        for row in batch:
            name, engine, brightness, warmth, movement, density, space, aggression, tags, desc = row
            try:
                preset = make_preset(
                    name, engine, brightness, warmth, movement, density, space, aggression,
                    tags, desc, sub_cat
                )
                ok, fname = write_preset(preset, OUTPUT_DIR)
                if ok:
                    print(f"  WRITE  {fname}")
                    written += 1
                else:
                    print(f"  SKIP   {fname} (exists)")
                    skipped += 1
            except Exception as e:
                msg = f"  ERROR  {name}: {e}"
                print(msg)
                errors.append(msg)

    print(f"\n{'='*50}")
    print(f"Prism Expansion Pack complete.")
    print(f"  Written : {written}")
    print(f"  Skipped : {skipped}")
    print(f"  Errors  : {len(errors)}")
    if errors:
        for e in errors:
            print(e)
    print(f"  Output  : {OUTPUT_DIR}")


if __name__ == "__main__":
    main()
