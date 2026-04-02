#!/usr/bin/env python3
"""OVERTONE Preset Generator -- 150 factory presets for the Nautilus spectral engine.

Generates .xometa JSON files across 7 moods:
  Foundation (30): intro/teaching + basic timbral starting points
  Atmosphere (25): ambient/spatial/evolving spectral landscapes
  Entangled (20): coupling-ready presets with high macroCoupling
  Prism (25): production-oriented (keys, leads, bass, pads for genres)
  Flux (20): movement-heavy, LFO-driven, rhythmic spectral animation
  Aether (15): experimental/deep/extreme convergent exploration
  Family (15): OVERTONE + 15 fleet engine coupling presets

Continued fraction constants:
  0 = Pi   -- tight cluster, metallic unison beating
  1 = E    -- wide-stepped, organ-like intervals
  2 = Phi  -- Fibonacci ratios, most musical, default
  3 = Sqrt2 -- Pell numbers, tritone-adjacent tensions
"""

import json
import os

PRESET_DIR = os.path.join(os.path.dirname(__file__), "..", "Presets", "XOceanus")

# -- OVERTONE defaults (26 params) ------------------------------------------------
DEFAULTS = {
    "over_constant":      2,        # Phi (0=Pi, 1=E, 2=Phi, 3=Sqrt2)
    "over_depth":         2.0,      # 0-7 convergent index
    "over_partial0":      1.0,      # partial amplitudes 0-1
    "over_partial1":      0.5,
    "over_partial2":      0.333333,
    "over_partial3":      0.25,
    "over_partial4":      0.2,
    "over_partial5":      0.166667,
    "over_partial6":      0.142857,
    "over_partial7":      0.125,
    "over_velBright":     0.4,      # velocity -> upper partial brightness 0-1
    "over_filterCutoff":  12000.0,  # 1000-20000 Hz
    "over_filterRes":     0.3,      # 0-0.8
    "over_ampAtk":        0.02,     # 0.001-4.0 s
    "over_ampDec":        0.3,      # 0.05-5.0 s
    "over_ampSus":        0.7,      # 0-1
    "over_ampRel":        1.0,      # 0.05-8.0 s
    "over_lfo1Rate":      0.25,     # 0.01-10 Hz (depth sweep)
    "over_lfo1Depth":     0.2,      # 0-1
    "over_lfo2Rate":      0.1,      # 0.01-10 Hz (phase rotation)
    "over_lfo2Depth":     0.15,     # 0-1
    "over_resoMix":       0.15,     # 0-1 (allpass resonator)
    "over_macroDepth":    0.35,     # 0-1 (sweeps convergent index)
    "over_macroColor":    0.5,      # 0-1 (upper partial brightness)
    "over_macroCoupling": 0.0,      # 0-1 (coupling recv + standalone shimmer)
    "over_macroSpace":    0.3,      # 0-1 (reverb depth + wet mix)
}

# Constant aliases for readability
PI, E, PHI, SQRT2 = 0, 1, 2, 3


def make_preset(name, mood, overrides, dna, desc="", tags=None, engines=None,
                coupling=None, coupling_intensity="None"):
    """Build a full .xometa dict from name + param overrides."""
    params = dict(DEFAULTS)
    params.update(overrides)

    preset = {
        "schema_version": 1,
        "name": name,
        "mood": mood,
        "engines": engines or ["Overtone"],
        "author": "XO_OX Designs",
        "version": "1.0.0",
        "description": desc,
        "tags": tags or [],
        "macroLabels": ["DEPTH", "COLOR", "COUPLING", "SPACE"],
        "couplingIntensity": coupling_intensity,
        "tempo": None,
        "dna": {
            "brightness": dna[0], "warmth": dna[1], "movement": dna[2],
            "density": dna[3], "space": dna[4], "aggression": dna[5]
        },
        "parameters": {"Overtone": params},
    }
    if coupling:
        preset["coupling"] = {"pairs": coupling}
    return preset


def save(preset):
    """Write preset to the correct mood directory."""
    mood = preset["mood"]
    safe_name = preset["name"].replace(" ", "_").replace("'", "").replace(",", "")
    filename = f"Overtone_{safe_name}.xometa"
    path = os.path.join(PRESET_DIR, mood, filename)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w") as f:
        json.dump(preset, f, indent=2)
    return path


# ==============================================================================
# FOUNDATION (30) -- introductory + basic timbral starting points
# ==============================================================================

FOUNDATION = [
    # -- Teaching presets: one concept at a time --
    ("Pure Nautilus", {
        "over_constant": PHI, "over_depth": 0.0,
        "over_macroDepth": 0.0, "over_macroColor": 0.0, "over_macroSpace": 0.0,
        "over_lfo1Depth": 0.0, "over_lfo2Depth": 0.0, "over_resoMix": 0.0,
    }, (0.3, 0.6, 0.0, 0.15, 0.0, 0.0),
     "All 8 partials at harmonic series falloff, Phi constant, depth zero. The Nautilus at rest.",
     ["foundation", "init", "pure", "fibonacci"]),

    ("First Spiral", {
        "over_constant": PHI, "over_depth": 3.0,
        "over_macroDepth": 0.0, "over_macroColor": 0.0, "over_macroSpace": 0.0,
        "over_lfo1Depth": 0.0, "over_lfo2Depth": 0.0,
    }, (0.4, 0.55, 0.0, 0.2, 0.0, 0.05),
     "Depth at 3: partials now tuned to Fibonacci convergents 5/3, 8/5, 13/8. Metallic shimmer appears.",
     ["foundation", "depth", "fibonacci", "metallic"]),

    ("Deep Irrational", {
        "over_constant": PHI, "over_depth": 6.5,
        "over_macroDepth": 0.0, "over_macroColor": 0.0, "over_macroSpace": 0.0,
        "over_lfo1Depth": 0.0, "over_lfo2Depth": 0.0,
    }, (0.5, 0.4, 0.0, 0.3, 0.0, 0.15),
     "Near-maximum depth: partials approach the golden ratio 1.618. Full irrational shimmer.",
     ["foundation", "deep", "golden-ratio", "irrational"]),

    ("Pi Cluster", {
        "over_constant": PI, "over_depth": 2.0,
        "over_macroDepth": 0.0, "over_macroColor": 0.0, "over_macroSpace": 0.0,
        "over_lfo1Depth": 0.0, "over_lfo2Depth": 0.0,
    }, (0.45, 0.45, 0.0, 0.35, 0.0, 0.1),
     "Pi constant: convergents cluster tightly around 1.047. Dense beating, metallic ring.",
     ["foundation", "pi", "cluster", "beating"]),

    ("E Organ", {
        "over_constant": E, "over_depth": 3.0,
        "over_macroDepth": 0.0, "over_macroColor": 0.0, "over_macroSpace": 0.0,
        "over_lfo1Depth": 0.0, "over_lfo2Depth": 0.0,
    }, (0.5, 0.55, 0.0, 0.25, 0.0, 0.0),
     "E constant: wider-spaced convergents (1.5, 1.33, 1.375). Organ-like intervals.",
     ["foundation", "euler", "organ", "intervals"]),

    ("Sqrt2 Tension", {
        "over_constant": SQRT2, "over_depth": 3.0,
        "over_macroDepth": 0.0, "over_macroColor": 0.0, "over_macroSpace": 0.0,
        "over_lfo1Depth": 0.0, "over_lfo2Depth": 0.0,
    }, (0.45, 0.4, 0.0, 0.3, 0.0, 0.2),
     "Sqrt2 constant: Pell number convergents near 1.414. Tritone-adjacent tension.",
     ["foundation", "sqrt2", "pell", "tritone", "tension"]),

    ("Color Sweep", {
        "over_constant": PHI, "over_depth": 2.0,
        "over_macroDepth": 0.0, "over_macroColor": 0.8, "over_macroSpace": 0.0,
        "over_lfo1Depth": 0.0, "over_lfo2Depth": 0.0,
    }, (0.65, 0.45, 0.0, 0.3, 0.0, 0.1),
     "COLOR macro high: upper partials 4-7 boosted. Bright, shimmering, crystalline.",
     ["foundation", "color", "bright", "upper-partials"]),

    ("Depth Macro", {
        "over_constant": PHI, "over_depth": 0.0,
        "over_macroDepth": 0.7, "over_macroColor": 0.3, "over_macroSpace": 0.0,
        "over_lfo1Depth": 0.0, "over_lfo2Depth": 0.0,
    }, (0.5, 0.5, 0.0, 0.25, 0.0, 0.1),
     "DEPTH macro sweeps the convergent index. Turn it up: clean to metallic to irrational.",
     ["foundation", "macro", "depth", "sweep"]),

    ("Resonator Ring", {
        "over_constant": PHI, "over_depth": 2.0,
        "over_resoMix": 0.6,
        "over_macroDepth": 0.0, "over_macroColor": 0.3, "over_macroSpace": 0.0,
        "over_lfo1Depth": 0.0, "over_lfo2Depth": 0.0,
    }, (0.5, 0.5, 0.0, 0.3, 0.1, 0.1),
     "Allpass resonator tuned to fundamental at 60% mix. Adds comb reinforcement.",
     ["foundation", "resonator", "allpass", "comb"]),

    ("Space Bloom", {
        "over_constant": PHI, "over_depth": 2.0,
        "over_macroSpace": 0.8, "over_macroDepth": 0.0, "over_macroColor": 0.3,
        "over_lfo1Depth": 0.0, "over_lfo2Depth": 0.0,
    }, (0.4, 0.55, 0.0, 0.2, 0.7, 0.0),
     "SPACE macro opens the Schroeder reverb. Spectral tails bloom into crystalline room.",
     ["foundation", "space", "reverb", "bloom"]),

    ("LFO Breathe", {
        "over_constant": PHI, "over_depth": 2.0,
        "over_lfo1Rate": 0.15, "over_lfo1Depth": 0.5,
        "over_lfo2Depth": 0.0, "over_macroDepth": 0.0, "over_macroColor": 0.3,
    }, (0.45, 0.5, 0.5, 0.2, 0.0, 0.05),
     "LFO1 slowly sweeps convergent depth. The spectrum breathes between clean and metallic.",
     ["foundation", "lfo", "breathing", "sweep"]),

    ("Phase Shimmer", {
        "over_constant": PHI, "over_depth": 3.0,
        "over_lfo2Rate": 0.3, "over_lfo2Depth": 0.5,
        "over_lfo1Depth": 0.0, "over_macroDepth": 0.0, "over_macroColor": 0.4,
    }, (0.5, 0.45, 0.45, 0.25, 0.0, 0.05),
     "LFO2 rotates partial phases. Crystalline chorus effect -- each partial drifts independently.",
     ["foundation", "lfo2", "phase", "shimmer", "chorus"]),

    ("Velocity Bright", {
        "over_constant": PHI, "over_depth": 2.0,
        "over_velBright": 0.8,
        "over_filterCutoff": 6000.0,
        "over_macroDepth": 0.0, "over_macroColor": 0.2,
        "over_lfo1Depth": 0.0, "over_lfo2Depth": 0.0,
    }, (0.45, 0.55, 0.0, 0.2, 0.0, 0.15),
     "High velBright: play soft for dark, play hard for brilliant upper partials. D001 showcase.",
     ["foundation", "velocity", "dynamics", "expression"]),

    ("Filter Dark", {
        "over_constant": PHI, "over_depth": 3.0,
        "over_filterCutoff": 2000.0, "over_filterRes": 0.5,
        "over_macroDepth": 0.0, "over_macroColor": 0.0,
        "over_lfo1Depth": 0.0, "over_lfo2Depth": 0.0,
    }, (0.2, 0.7, 0.0, 0.25, 0.0, 0.0),
     "Low-pass filter closed to 2kHz with resonance. Dark, warm spectral character.",
     ["foundation", "filter", "dark", "warm"]),

    ("Full Spectrum", {
        "over_constant": PHI, "over_depth": 2.0,
        "over_partial0": 1.0, "over_partial1": 1.0, "over_partial2": 1.0,
        "over_partial3": 1.0, "over_partial4": 1.0, "over_partial5": 1.0,
        "over_partial6": 1.0, "over_partial7": 1.0,
        "over_filterCutoff": 18000.0, "over_filterRes": 0.1,
        "over_macroDepth": 0.0, "over_macroColor": 0.0,
        "over_lfo1Depth": 0.0, "over_lfo2Depth": 0.0,
    }, (0.8, 0.3, 0.0, 0.6, 0.0, 0.2),
     "All 8 partials at full amplitude, wide-open filter. Maximum spectral density.",
     ["foundation", "full", "dense", "bright"]),

    # -- Basic timbral starting points --
    ("Warm Fibonacci", {
        "over_constant": PHI, "over_depth": 1.5,
        "over_filterCutoff": 4000.0, "over_filterRes": 0.2,
        "over_ampAtk": 0.3, "over_ampRel": 1.5,
        "over_macroColor": 0.2, "over_macroSpace": 0.2,
    }, (0.3, 0.7, 0.15, 0.2, 0.2, 0.0),
     "Warm pad with gentle Fibonacci partials. Filter tames upper spectrum.",
     ["foundation", "warm", "pad", "fibonacci"]),

    ("Crystal Keys", {
        "over_constant": PHI, "over_depth": 3.0,
        "over_ampAtk": 0.005, "over_ampDec": 0.8, "over_ampSus": 0.3,
        "over_filterCutoff": 10000.0,
        "over_macroColor": 0.5, "over_macroSpace": 0.15,
        "over_lfo2Rate": 0.2, "over_lfo2Depth": 0.1,
    }, (0.6, 0.4, 0.15, 0.25, 0.15, 0.05),
     "Quick attack, medium decay. Crystal-clear key sound with Fibonacci shimmer.",
     ["foundation", "keys", "crystal", "pluck"]),

    ("Sub Nautilus", {
        "over_constant": PHI, "over_depth": 0.5,
        "over_partial0": 1.0, "over_partial1": 0.3, "over_partial2": 0.1,
        "over_partial3": 0.0, "over_partial4": 0.0, "over_partial5": 0.0,
        "over_partial6": 0.0, "over_partial7": 0.0,
        "over_filterCutoff": 2000.0,
        "over_macroDepth": 0.0, "over_macroColor": 0.0,
    }, (0.15, 0.8, 0.05, 0.1, 0.0, 0.0),
     "Fundamental-heavy: only partials 0-2 active. Deep, warm sub tone.",
     ["foundation", "sub", "bass", "minimal"]),

    ("Bell Convergent", {
        "over_constant": PI, "over_depth": 4.0,
        "over_ampAtk": 0.001, "over_ampDec": 2.0, "over_ampSus": 0.0,
        "over_ampRel": 2.5,
        "over_filterCutoff": 16000.0,
        "over_macroColor": 0.4, "over_macroSpace": 0.3,
        "over_resoMix": 0.25,
    }, (0.65, 0.35, 0.1, 0.3, 0.3, 0.1),
     "Pi convergents at depth 4, percussive envelope. Metallic bell tone.",
     ["foundation", "bell", "pi", "metallic", "percussive"]),

    ("Glass Organ", {
        "over_constant": E, "over_depth": 2.5,
        "over_ampAtk": 0.05, "over_ampSus": 0.8,
        "over_filterCutoff": 8000.0,
        "over_macroColor": 0.4, "over_macroSpace": 0.15,
        "over_lfo2Rate": 0.08, "over_lfo2Depth": 0.1,
    }, (0.55, 0.5, 0.1, 0.25, 0.15, 0.0),
     "E constant organ-like intervals with gentle phase shimmer. Glassy sustained tone.",
     ["foundation", "organ", "euler", "glass", "sustain"]),

    ("Slow Attack Pad", {
        "over_constant": PHI, "over_depth": 2.5,
        "over_ampAtk": 2.0, "over_ampRel": 3.0,
        "over_filterCutoff": 6000.0,
        "over_macroColor": 0.4, "over_macroSpace": 0.4,
        "over_lfo1Rate": 0.05, "over_lfo1Depth": 0.2,
        "over_lfo2Rate": 0.08, "over_lfo2Depth": 0.15,
    }, (0.4, 0.6, 0.3, 0.2, 0.4, 0.0),
     "Two-second attack, long release. Spectral pad that blooms slowly.",
     ["foundation", "pad", "slow", "bloom"]),

    ("Bright Lead", {
        "over_constant": PHI, "over_depth": 1.5,
        "over_ampAtk": 0.005, "over_ampDec": 0.4, "over_ampSus": 0.6,
        "over_filterCutoff": 15000.0, "over_filterRes": 0.15,
        "over_velBright": 0.6,
        "over_macroColor": 0.6, "over_macroDepth": 0.2,
    }, (0.7, 0.35, 0.1, 0.25, 0.0, 0.15),
     "Bright Fibonacci lead. Fast attack, velocity-sensitive upper partials.",
     ["foundation", "lead", "bright", "velocity"]),

    ("Dark Pell", {
        "over_constant": SQRT2, "over_depth": 2.0,
        "over_filterCutoff": 3000.0, "over_filterRes": 0.4,
        "over_macroColor": 0.1, "over_macroSpace": 0.2,
        "over_lfo1Depth": 0.0, "over_lfo2Depth": 0.0,
    }, (0.2, 0.6, 0.0, 0.3, 0.2, 0.1),
     "Sqrt2 convergents through dark filter. Tense, tritone-adjacent warmth.",
     ["foundation", "dark", "sqrt2", "pell", "tense"]),

    ("Harmonic Series", {
        "over_constant": PHI, "over_depth": 0.0,
        "over_partial0": 1.0, "over_partial1": 0.5, "over_partial2": 0.333,
        "over_partial3": 0.25, "over_partial4": 0.2, "over_partial5": 0.167,
        "over_partial6": 0.143, "over_partial7": 0.125,
        "over_filterCutoff": 16000.0,
        "over_macroDepth": 0.0, "over_macroColor": 0.0, "over_macroSpace": 0.0,
        "over_lfo1Depth": 0.0, "over_lfo2Depth": 0.0,
    }, (0.5, 0.5, 0.0, 0.25, 0.0, 0.0),
     "Depth zero: pure 1/n harmonic series. The reference tone before irrationality.",
     ["foundation", "harmonic", "reference", "classic"]),

    ("Resonant Phi", {
        "over_constant": PHI, "over_depth": 3.5,
        "over_resoMix": 0.5,
        "over_filterCutoff": 8000.0, "over_filterRes": 0.45,
        "over_macroColor": 0.3,
    }, (0.5, 0.5, 0.05, 0.3, 0.1, 0.1),
     "Phi convergents with strong allpass resonator and filter resonance. Peaky, alive.",
     ["foundation", "resonant", "phi", "peaky"]),

    ("Pluck Nautilus", {
        "over_constant": PHI, "over_depth": 2.0,
        "over_ampAtk": 0.001, "over_ampDec": 0.5, "over_ampSus": 0.0,
        "over_ampRel": 0.8,
        "over_filterCutoff": 10000.0,
        "over_velBright": 0.7,
        "over_macroColor": 0.3, "over_macroSpace": 0.15,
    }, (0.55, 0.45, 0.1, 0.2, 0.15, 0.1),
     "Fast pluck envelope. Velocity controls spectral brightness. Quick and musical.",
     ["foundation", "pluck", "short", "velocity"]),

    ("E Wide Pad", {
        "over_constant": E, "over_depth": 4.0,
        "over_ampAtk": 1.5, "over_ampRel": 2.5,
        "over_filterCutoff": 6000.0,
        "over_macroColor": 0.3, "over_macroSpace": 0.35,
        "over_lfo1Rate": 0.08, "over_lfo1Depth": 0.15,
    }, (0.4, 0.55, 0.2, 0.25, 0.35, 0.0),
     "E constant wide-stepped intervals. Slow attack pad with gentle depth breathing.",
     ["foundation", "pad", "euler", "wide", "intervals"]),

    ("Macro Explorer", {
        "over_constant": PHI, "over_depth": 1.0,
        "over_macroDepth": 0.5, "over_macroColor": 0.5,
        "over_macroCoupling": 0.3, "over_macroSpace": 0.3,
        "over_lfo1Rate": 0.2, "over_lfo1Depth": 0.15,
        "over_lfo2Rate": 0.12, "over_lfo2Depth": 0.1,
        "over_resoMix": 0.2,
    }, (0.5, 0.5, 0.3, 0.3, 0.3, 0.05),
     "All four macros active at moderate values. Move any one to hear its effect.",
     ["foundation", "macros", "explorer", "balanced"]),

    ("Aftertouch Play", {
        "over_constant": PHI, "over_depth": 2.0,
        "over_macroColor": 0.7,
        "over_velBright": 0.6,
        "over_ampAtk": 0.01, "over_ampSus": 0.75,
        "over_filterCutoff": 8000.0,
    }, (0.5, 0.5, 0.1, 0.25, 0.0, 0.1),
     "High COLOR + velBright: aftertouch adds shimmer, velocity adds brightness. D006 showcase.",
     ["foundation", "aftertouch", "expression", "performance"]),

    ("Init Overtone", {
        "over_constant": PHI, "over_depth": 2.0,
    }, (0.45, 0.5, 0.15, 0.2, 0.15, 0.05),
     "Default init patch. All parameters at factory defaults. The starting point for sound design.",
     ["foundation", "init", "default", "starting-point"]),
]

# ==============================================================================
# ATMOSPHERE (25) -- ambient/spatial spectral landscapes
# ==============================================================================

ATMOSPHERE = [
    ("Frozen Chamber", {
        "over_constant": PHI, "over_depth": 4.0,
        "over_ampAtk": 1.5, "over_ampRel": 4.0, "over_ampSus": 0.8,
        "over_filterCutoff": 5000.0,
        "over_macroSpace": 0.7, "over_macroColor": 0.3,
        "over_lfo1Rate": 0.03, "over_lfo1Depth": 0.2,
        "over_lfo2Rate": 0.05, "over_lfo2Depth": 0.2,
        "over_resoMix": 0.2,
    }, (0.35, 0.55, 0.3, 0.25, 0.7, 0.0),
     "Frozen spectral room. Slow depth breathing, heavy reverb, crystalline shimmer.",
     ["atmosphere", "frozen", "ambient", "crystal"]),

    ("Golden Drift", {
        "over_constant": PHI, "over_depth": 3.0,
        "over_ampAtk": 2.0, "over_ampRel": 5.0, "over_ampSus": 0.85,
        "over_filterCutoff": 7000.0,
        "over_macroSpace": 0.5, "over_macroColor": 0.4, "over_macroDepth": 0.3,
        "over_lfo1Rate": 0.02, "over_lfo1Depth": 0.35,
        "over_lfo2Rate": 0.04, "over_lfo2Depth": 0.25,
    }, (0.4, 0.6, 0.45, 0.2, 0.5, 0.0),
     "Fibonacci partials drifting through convergent space. 50-second LFO1 cycle.",
     ["atmosphere", "golden", "drift", "fibonacci", "slow"]),

    ("Pi Fog", {
        "over_constant": PI, "over_depth": 3.0,
        "over_ampAtk": 2.5, "over_ampRel": 4.0, "over_ampSus": 0.75,
        "over_filterCutoff": 3500.0, "over_filterRes": 0.2,
        "over_macroSpace": 0.6, "over_macroColor": 0.2,
        "over_lfo1Rate": 0.04, "over_lfo1Depth": 0.25,
    }, (0.25, 0.65, 0.3, 0.3, 0.6, 0.0),
     "Pi's tight convergent cluster through dark filter. Dense metallic fog.",
     ["atmosphere", "pi", "fog", "dark", "dense"]),

    ("Euler Cathedral", {
        "over_constant": E, "over_depth": 3.5,
        "over_ampAtk": 1.0, "over_ampRel": 6.0, "over_ampSus": 0.85,
        "over_filterCutoff": 8000.0,
        "over_macroSpace": 0.8, "over_macroColor": 0.35,
        "over_resoMix": 0.3,
        "over_lfo2Rate": 0.06, "over_lfo2Depth": 0.2,
    }, (0.45, 0.55, 0.15, 0.25, 0.8, 0.0),
     "E constant organ intervals in vast reverb space. Sacred mathematics.",
     ["atmosphere", "euler", "cathedral", "reverb", "sacred"]),

    ("Pell Tension Field", {
        "over_constant": SQRT2, "over_depth": 4.0,
        "over_ampAtk": 1.8, "over_ampRel": 3.5, "over_ampSus": 0.7,
        "over_filterCutoff": 4500.0, "over_filterRes": 0.3,
        "over_macroSpace": 0.5, "over_macroColor": 0.3,
        "over_lfo1Rate": 0.05, "over_lfo1Depth": 0.3,
    }, (0.35, 0.4, 0.35, 0.3, 0.5, 0.15),
     "Sqrt2 tritone tensions in slow sweep. Uneasy, beautiful, suspended.",
     ["atmosphere", "sqrt2", "tension", "uneasy", "suspended"]),

    ("Spectral Dawn", {
        "over_constant": PHI, "over_depth": 1.5,
        "over_ampAtk": 3.0, "over_ampRel": 4.0, "over_ampSus": 0.9,
        "over_filterCutoff": 6000.0,
        "over_macroSpace": 0.5, "over_macroColor": 0.5, "over_macroDepth": 0.2,
        "over_lfo1Rate": 0.02, "over_lfo1Depth": 0.15,
        "over_lfo2Rate": 0.03, "over_lfo2Depth": 0.2,
    }, (0.45, 0.6, 0.25, 0.2, 0.5, 0.0),
     "Three-second attack bloom. Gentle Fibonacci spectrum, slowly warming.",
     ["atmosphere", "dawn", "bloom", "gentle", "slow"]),

    ("Underwater Glass", {
        "over_constant": PHI, "over_depth": 5.0,
        "over_filterCutoff": 3000.0, "over_filterRes": 0.35,
        "over_ampAtk": 1.0, "over_ampRel": 3.0,
        "over_macroSpace": 0.6, "over_macroColor": 0.15,
        "over_resoMix": 0.35,
        "over_lfo1Rate": 0.06, "over_lfo1Depth": 0.2,
    }, (0.25, 0.6, 0.25, 0.25, 0.6, 0.0),
     "Deep Fibonacci ratios through resonant dark filter. Muffled, aquatic, distant.",
     ["atmosphere", "underwater", "glass", "dark", "aquatic"]),

    ("Stellar Harmonics", {
        "over_constant": E, "over_depth": 5.0,
        "over_partial0": 0.8, "over_partial1": 0.9, "over_partial2": 1.0,
        "over_partial3": 0.9, "over_partial4": 0.8, "over_partial5": 0.7,
        "over_partial6": 0.6, "over_partial7": 0.5,
        "over_ampAtk": 1.5, "over_ampRel": 4.0,
        "over_filterCutoff": 12000.0,
        "over_macroSpace": 0.5, "over_macroColor": 0.4,
        "over_lfo2Rate": 0.07, "over_lfo2Depth": 0.3,
    }, (0.6, 0.45, 0.25, 0.4, 0.5, 0.0),
     "Boosted mid-partials in E constant. Bright, wide, like looking at distant stars.",
     ["atmosphere", "stellar", "bright", "euler", "wide"]),

    ("Nautilus Descent", {
        "over_constant": PHI, "over_depth": 2.0,
        "over_macroDepth": 0.8,
        "over_ampAtk": 0.5, "over_ampRel": 5.0, "over_ampSus": 0.9,
        "over_filterCutoff": 5000.0,
        "over_macroSpace": 0.6, "over_macroColor": 0.3,
        "over_lfo1Rate": 0.01, "over_lfo1Depth": 0.4,
    }, (0.35, 0.55, 0.4, 0.2, 0.6, 0.0),
     "DEPTH macro + LFO1 at minimum rate: 100-second spectral mutation cycle. The deep dive.",
     ["atmosphere", "descent", "slow", "mutation", "deep"]),

    ("Mesopelagic Zone", {
        "over_constant": PHI, "over_depth": 4.5,
        "over_filterCutoff": 4000.0, "over_filterRes": 0.25,
        "over_ampAtk": 2.0, "over_ampRel": 4.0,
        "over_macroSpace": 0.55, "over_macroColor": 0.2,
        "over_resoMix": 0.3,
        "over_lfo1Rate": 0.03, "over_lfo1Depth": 0.2,
        "over_lfo2Rate": 0.05, "over_lfo2Depth": 0.15,
    }, (0.3, 0.6, 0.3, 0.25, 0.55, 0.0),
     "The Nautilus habitat: 200-1000m twilight. Warm, deep, gently breathing.",
     ["atmosphere", "mesopelagic", "nautilus", "twilight", "habitat"]),

    ("Ice Crystal", {
        "over_constant": PI, "over_depth": 5.0,
        "over_partial4": 0.6, "over_partial5": 0.55, "over_partial6": 0.5, "over_partial7": 0.45,
        "over_ampAtk": 0.8, "over_ampRel": 3.0,
        "over_filterCutoff": 14000.0, "over_filterRes": 0.15,
        "over_macroSpace": 0.5, "over_macroColor": 0.6,
    }, (0.7, 0.3, 0.1, 0.35, 0.5, 0.05),
     "Pi convergents with boosted upper partials. Bright, cold, crystalline.",
     ["atmosphere", "ice", "crystal", "pi", "bright"]),

    ("Shell Memory", {
        "over_constant": PHI, "over_depth": 3.0,
        "over_ampAtk": 3.5, "over_ampRel": 6.0, "over_ampSus": 0.85,
        "over_filterCutoff": 5000.0,
        "over_macroSpace": 0.7, "over_macroDepth": 0.2,
        "over_lfo1Rate": 0.015, "over_lfo1Depth": 0.3,
        "over_lfo2Rate": 0.025, "over_lfo2Depth": 0.2,
    }, (0.35, 0.6, 0.35, 0.2, 0.7, 0.0),
     "Extremely slow attack and release. The Nautilus remembering its shape.",
     ["atmosphere", "memory", "slow", "nautilus", "deep"]),

    ("Night Harmonics", {
        "over_constant": SQRT2, "over_depth": 3.0,
        "over_filterCutoff": 3000.0, "over_filterRes": 0.3,
        "over_ampAtk": 1.0, "over_ampRel": 3.5,
        "over_macroSpace": 0.5, "over_macroColor": 0.15,
        "over_lfo1Rate": 0.04, "over_lfo1Depth": 0.2,
    }, (0.2, 0.55, 0.25, 0.25, 0.5, 0.1),
     "Sqrt2 Pell harmonics in dark filter. Night sky tension, unresolved beauty.",
     ["atmosphere", "night", "sqrt2", "dark", "tension"]),

    ("Convergent Mist", {
        "over_constant": PHI, "over_depth": 3.5,
        "over_partial0": 0.6, "over_partial1": 0.7, "over_partial2": 0.8,
        "over_partial3": 0.7, "over_partial4": 0.6, "over_partial5": 0.5,
        "over_partial6": 0.4, "over_partial7": 0.3,
        "over_ampAtk": 2.0, "over_ampRel": 4.5,
        "over_filterCutoff": 5500.0,
        "over_macroSpace": 0.65, "over_macroColor": 0.3,
        "over_lfo2Rate": 0.04, "over_lfo2Depth": 0.25,
    }, (0.4, 0.55, 0.2, 0.3, 0.65, 0.0),
     "Humped partial profile (mid-partials loudest). Misty, diffuse, spectral fog.",
     ["atmosphere", "mist", "diffuse", "humped", "fog"]),

    ("Logarithmic Spiral", {
        "over_constant": PHI, "over_depth": 2.0,
        "over_macroDepth": 0.6,
        "over_ampAtk": 1.0, "over_ampRel": 3.0,
        "over_filterCutoff": 8000.0,
        "over_macroSpace": 0.4, "over_macroColor": 0.5,
        "over_lfo1Rate": 0.02, "over_lfo1Depth": 0.4,
        "over_lfo2Rate": 0.03, "over_lfo2Depth": 0.3,
        "over_resoMix": 0.25,
    }, (0.5, 0.5, 0.45, 0.25, 0.4, 0.0),
     "The Nautilus shell in sound: slow LFOs spiral through convergent space.",
     ["atmosphere", "spiral", "nautilus", "logarithmic", "evolving"]),

    ("Abyssal Glow", {
        "over_constant": E, "over_depth": 6.0,
        "over_filterCutoff": 2500.0, "over_filterRes": 0.25,
        "over_ampAtk": 2.0, "over_ampRel": 5.0,
        "over_macroSpace": 0.7, "over_macroColor": 0.1,
        "over_lfo1Rate": 0.02, "over_lfo1Depth": 0.15,
    }, (0.2, 0.65, 0.2, 0.2, 0.7, 0.0),
     "Deep E convergents through dark filter, heavy reverb. Bioluminescent glow in the deep.",
     ["atmosphere", "abyssal", "deep", "glow", "dark"]),

    ("Morning Spectrum", {
        "over_constant": PHI, "over_depth": 1.0,
        "over_ampAtk": 1.5, "over_ampRel": 2.5,
        "over_filterCutoff": 10000.0,
        "over_macroSpace": 0.35, "over_macroColor": 0.6,
        "over_lfo2Rate": 0.08, "over_lfo2Depth": 0.15,
    }, (0.6, 0.5, 0.15, 0.2, 0.35, 0.0),
     "Clean Fibonacci spectrum, bright filter, gentle phase shimmer. Fresh morning light.",
     ["atmosphere", "morning", "bright", "fresh", "gentle"]),

    ("Twilight Constant", {
        "over_constant": SQRT2, "over_depth": 5.0,
        "over_ampAtk": 2.5, "over_ampRel": 5.0, "over_ampSus": 0.8,
        "over_filterCutoff": 4000.0,
        "over_macroSpace": 0.6, "over_macroColor": 0.2,
        "over_lfo1Rate": 0.03, "over_lfo1Depth": 0.25,
        "over_lfo2Rate": 0.05, "over_lfo2Depth": 0.2,
    }, (0.3, 0.45, 0.35, 0.25, 0.6, 0.1),
     "Sqrt2 at deep convergence. Pell number tensions in twilight space.",
     ["atmosphere", "twilight", "sqrt2", "pell", "tension"]),

    ("Warm Resonance", {
        "over_constant": PHI, "over_depth": 2.5,
        "over_filterCutoff": 4000.0, "over_filterRes": 0.5,
        "over_resoMix": 0.45,
        "over_ampAtk": 1.0, "over_ampRel": 3.0,
        "over_macroSpace": 0.4, "over_macroColor": 0.2,
    }, (0.35, 0.65, 0.1, 0.3, 0.4, 0.05),
     "Strong allpass resonator + filter resonance. Warm, peaky, singing.",
     ["atmosphere", "warm", "resonant", "singing", "peaky"]),

    ("Distant Harmonics", {
        "over_constant": PHI, "over_depth": 3.0,
        "over_ampAtk": 2.0, "over_ampRel": 4.0,
        "over_filterCutoff": 3500.0,
        "over_macroSpace": 0.85,
        "over_lfo1Rate": 0.04, "over_lfo1Depth": 0.15,
    }, (0.25, 0.55, 0.2, 0.2, 0.85, 0.0),
     "Maximum space macro. Fibonacci harmonics heard from far away.",
     ["atmosphere", "distant", "reverb", "space", "vast"]),

    ("Iridescent Film", {
        "over_constant": PHI, "over_depth": 4.0,
        "over_macroCoupling": 0.5,
        "over_macroColor": 0.6,
        "over_ampAtk": 1.5, "over_ampRel": 3.5,
        "over_filterCutoff": 9000.0,
        "over_macroSpace": 0.35,
        "over_lfo2Rate": 0.1, "over_lfo2Depth": 0.25,
    }, (0.55, 0.45, 0.3, 0.3, 0.35, 0.0),
     "COUPLING shimmer + high COLOR + phase rotation. Thin, iridescent, alive.",
     ["atmosphere", "iridescent", "shimmer", "thin", "alive"]),

    ("Ocean Breath", {
        "over_constant": PHI, "over_depth": 2.0,
        "over_macroDepth": 0.4,
        "over_ampAtk": 1.5, "over_ampRel": 3.0, "over_ampSus": 0.85,
        "over_filterCutoff": 5000.0,
        "over_macroSpace": 0.5,
        "over_lfo1Rate": 0.01, "over_lfo1Depth": 0.5,
    }, (0.35, 0.6, 0.5, 0.2, 0.5, 0.0),
     "LFO1 at absolute minimum rate (0.01 Hz = 100 seconds). The ocean breathing.",
     ["atmosphere", "ocean", "breath", "slowest", "geological"]),

    ("Quantum Intervals", {
        "over_constant": E, "over_depth": 2.0,
        "over_partial0": 1.0, "over_partial1": 0.8, "over_partial2": 0.6,
        "over_partial3": 0.8, "over_partial4": 0.4, "over_partial5": 0.6,
        "over_partial6": 0.3, "over_partial7": 0.5,
        "over_ampAtk": 1.0, "over_ampRel": 3.0,
        "over_filterCutoff": 10000.0,
        "over_macroSpace": 0.45, "over_macroColor": 0.4,
    }, (0.55, 0.5, 0.1, 0.35, 0.45, 0.0),
     "Irregular partial amplitude pattern in E constant. Unexpected interval relationships.",
     ["atmosphere", "quantum", "euler", "irregular", "intervals"]),

    ("Chamber Echo", {
        "over_constant": PHI, "over_depth": 2.5,
        "over_resoMix": 0.4,
        "over_ampAtk": 0.5, "over_ampRel": 3.5,
        "over_filterCutoff": 6000.0,
        "over_macroSpace": 0.55, "over_macroColor": 0.25,
        "over_lfo2Rate": 0.04, "over_lfo2Depth": 0.12,
    }, (0.4, 0.55, 0.15, 0.25, 0.55, 0.0),
     "Allpass resonator adds chamber echo to Fibonacci spectrum. Intimate, resonant.",
     ["atmosphere", "chamber", "echo", "resonant", "intimate"]),

    ("Spiral Nebula", {
        "over_constant": PHI, "over_depth": 5.0,
        "over_ampAtk": 2.5, "over_ampRel": 5.0, "over_ampSus": 0.85,
        "over_filterCutoff": 7000.0,
        "over_macroSpace": 0.6, "over_macroColor": 0.45, "over_macroDepth": 0.4,
        "over_lfo1Rate": 0.02, "over_lfo1Depth": 0.35,
        "over_lfo2Rate": 0.035, "over_lfo2Depth": 0.25,
    }, (0.45, 0.5, 0.4, 0.25, 0.6, 0.0),
     "Deep Fibonacci convergents in slow dual-LFO spiral. Vast, cosmic, expanding.",
     ["atmosphere", "nebula", "spiral", "cosmic", "vast"]),
]

# ==============================================================================
# ENTANGLED (20) -- coupling-ready presets
# ==============================================================================

ENTANGLED = [
    ("Coupling Donor", {
        "over_constant": PHI, "over_depth": 3.0,
        "over_macroCoupling": 0.6,
        "over_macroColor": 0.4,
        "over_filterCutoff": 10000.0,
        "over_ampSus": 0.8,
    }, (0.55, 0.45, 0.1, 0.3, 0.0, 0.1),
     "Designed to feed coupling output. Bright, sustained, rich in harmonics.",
     ["entangled", "donor", "coupling"]),

    ("Coupling Receiver", {
        "over_constant": PHI, "over_depth": 1.0,
        "over_macroCoupling": 0.9,
        "over_filterCutoff": 4000.0,
        "over_macroColor": 0.2,
    }, (0.3, 0.55, 0.15, 0.2, 0.0, 0.05),
     "Simple spectrum, maximum coupling sensitivity. Let other engines shape it.",
     ["entangled", "receiver", "coupling", "sensitive"]),

    ("Depth Entangle", {
        "over_constant": PHI, "over_depth": 1.0,
        "over_macroCoupling": 0.7,
        "over_macroDepth": 0.5,
        "over_lfo1Rate": 0.1, "over_lfo1Depth": 0.2,
        "over_macroSpace": 0.2,
    }, (0.45, 0.5, 0.35, 0.25, 0.2, 0.05),
     "EnvToMorph coupling target: partner envelope sweeps convergent depth.",
     ["entangled", "depth", "envtomorph", "sweep"]),

    ("Filter Entangle", {
        "over_constant": PHI, "over_depth": 3.0,
        "over_macroCoupling": 0.7,
        "over_filterCutoff": 5000.0, "over_filterRes": 0.4,
        "over_macroColor": 0.3,
    }, (0.4, 0.5, 0.2, 0.25, 0.0, 0.1),
     "AmpToFilter coupling target: partner amplitude modulates filter cutoff.",
     ["entangled", "filter", "amptofilter", "spectral"]),

    ("Pitch Entangle", {
        "over_constant": PHI, "over_depth": 2.0,
        "over_macroCoupling": 0.8,
        "over_ampSus": 0.8,
        "over_macroSpace": 0.15,
    }, (0.45, 0.5, 0.1, 0.2, 0.15, 0.05),
     "PitchToPitch coupling target: partner pitch offsets all OVERTONE partials.",
     ["entangled", "pitch", "pitchtopitch", "tuning"]),

    ("Shimmer Web", {
        "over_constant": PHI, "over_depth": 4.0,
        "over_macroCoupling": 0.6,
        "over_macroColor": 0.6,
        "over_lfo2Rate": 0.15, "over_lfo2Depth": 0.3,
        "over_filterCutoff": 12000.0,
        "over_macroSpace": 0.3,
    }, (0.6, 0.4, 0.35, 0.3, 0.3, 0.0),
     "Coupling shimmer + COLOR + phase rotation. A web of spectral connections.",
     ["entangled", "shimmer", "web", "bright"]),

    ("Resonant Link", {
        "over_constant": E, "over_depth": 3.0,
        "over_macroCoupling": 0.6,
        "over_resoMix": 0.5,
        "over_filterCutoff": 6000.0, "over_filterRes": 0.4,
    }, (0.45, 0.5, 0.1, 0.3, 0.1, 0.1),
     "Strong resonator + coupling. Partner engine reinforces spectral comb.",
     ["entangled", "resonant", "comb", "euler"]),

    ("Tension Link", {
        "over_constant": SQRT2, "over_depth": 3.5,
        "over_macroCoupling": 0.7,
        "over_filterCutoff": 5000.0,
        "over_macroColor": 0.3,
        "over_macroSpace": 0.2,
    }, (0.4, 0.4, 0.15, 0.3, 0.2, 0.2),
     "Sqrt2 tritone tensions made available for coupling modulation.",
     ["entangled", "tension", "sqrt2", "tritone"]),

    ("Slow Entangle", {
        "over_constant": PHI, "over_depth": 2.0,
        "over_macroCoupling": 0.6,
        "over_ampAtk": 2.0, "over_ampRel": 4.0,
        "over_macroSpace": 0.5,
        "over_lfo1Rate": 0.02, "over_lfo1Depth": 0.3,
    }, (0.35, 0.55, 0.4, 0.2, 0.5, 0.0),
     "Slow bloom with coupling. Partner engine modulates across long time scales.",
     ["entangled", "slow", "bloom", "coupled"]),

    ("All Paths Open", {
        "over_constant": PHI, "over_depth": 2.5,
        "over_macroCoupling": 0.8,
        "over_macroDepth": 0.3, "over_macroColor": 0.4,
        "over_macroSpace": 0.3,
        "over_filterCutoff": 8000.0, "over_filterRes": 0.3,
        "over_resoMix": 0.25,
        "over_lfo1Rate": 0.1, "over_lfo1Depth": 0.15,
    }, (0.5, 0.5, 0.25, 0.3, 0.3, 0.05),
     "All coupling types responsive, all macros active. Maximum entanglement readiness.",
     ["entangled", "all-paths", "open", "versatile"]),

    ("Pi Metal Couple", {
        "over_constant": PI, "over_depth": 4.0,
        "over_macroCoupling": 0.7,
        "over_filterCutoff": 8000.0,
        "over_macroColor": 0.5,
    }, (0.55, 0.4, 0.1, 0.35, 0.0, 0.15),
     "Pi metallic cluster ready for coupling. Dense beating interacts with partner.",
     ["entangled", "pi", "metal", "dense"]),

    ("Fibonacci Bridge", {
        "over_constant": PHI, "over_depth": 5.0,
        "over_macroCoupling": 0.6,
        "over_macroDepth": 0.4,
        "over_ampAtk": 0.5, "over_ampSus": 0.85,
        "over_filterCutoff": 7000.0,
        "over_macroSpace": 0.25,
    }, (0.45, 0.5, 0.2, 0.25, 0.25, 0.05),
     "Deep Fibonacci convergents bridge between engines. DEPTH macro sweeps coupling response.",
     ["entangled", "fibonacci", "bridge", "deep"]),

    ("Spectral Knot", {
        "over_constant": PHI, "over_depth": 3.5,
        "over_macroCoupling": 0.8,
        "over_resoMix": 0.4,
        "over_macroColor": 0.5,
        "over_macroSpace": 0.3,
        "over_lfo2Rate": 0.1, "over_lfo2Depth": 0.2,
    }, (0.5, 0.45, 0.25, 0.3, 0.3, 0.05),
     "Resonator + shimmer + coupling. Spectral lines knotted together.",
     ["entangled", "knot", "resonator", "shimmer"]),

    ("Dark Coupled", {
        "over_constant": SQRT2, "over_depth": 2.0,
        "over_macroCoupling": 0.7,
        "over_filterCutoff": 2500.0, "over_filterRes": 0.4,
        "over_macroColor": 0.1,
    }, (0.2, 0.6, 0.1, 0.25, 0.0, 0.1),
     "Dark Sqrt2 spectrum optimized to receive coupling modulation from brighter engines.",
     ["entangled", "dark", "sqrt2", "receiver"]),

    ("Bright Coupled", {
        "over_constant": PHI, "over_depth": 2.0,
        "over_macroCoupling": 0.6,
        "over_filterCutoff": 16000.0,
        "over_macroColor": 0.7,
        "over_partial4": 0.5, "over_partial5": 0.45, "over_partial6": 0.4, "over_partial7": 0.35,
    }, (0.75, 0.3, 0.1, 0.35, 0.0, 0.1),
     "Maximum brightness, boosted upper partials. Feeds rich coupling output to darker engines.",
     ["entangled", "bright", "donor", "upper-partials"]),

    ("Velocity Couple", {
        "over_constant": PHI, "over_depth": 2.5,
        "over_macroCoupling": 0.7,
        "over_velBright": 0.8,
        "over_filterCutoff": 6000.0,
        "over_macroColor": 0.4,
    }, (0.5, 0.5, 0.1, 0.25, 0.0, 0.15),
     "High velBright + coupling: velocity controls both internal brightness and coupling output.",
     ["entangled", "velocity", "expression", "dynamics"]),

    ("Long Entangle", {
        "over_constant": PHI, "over_depth": 3.0,
        "over_macroCoupling": 0.6,
        "over_ampAtk": 3.0, "over_ampRel": 7.0, "over_ampSus": 0.9,
        "over_macroSpace": 0.6,
        "over_lfo1Rate": 0.015, "over_lfo1Depth": 0.35,
    }, (0.35, 0.55, 0.35, 0.2, 0.6, 0.0),
     "Ultra-long envelope + coupling. For generative patches where engines breathe together.",
     ["entangled", "long", "generative", "slow"]),

    ("Sub Couple", {
        "over_constant": PHI, "over_depth": 0.5,
        "over_partial0": 1.0, "over_partial1": 0.2, "over_partial2": 0.05,
        "over_partial3": 0.0, "over_partial4": 0.0, "over_partial5": 0.0,
        "over_partial6": 0.0, "over_partial7": 0.0,
        "over_filterCutoff": 2000.0,
        "over_macroCoupling": 0.8,
    }, (0.1, 0.7, 0.05, 0.1, 0.0, 0.0),
     "Sub-bass with maximum coupling sensitivity. Let fleet engines shape the fundamental.",
     ["entangled", "sub", "bass", "receiver"]),

    ("Phase Entangle", {
        "over_constant": PHI, "over_depth": 3.5,
        "over_macroCoupling": 0.6,
        "over_lfo2Rate": 0.25, "over_lfo2Depth": 0.4,
        "over_macroColor": 0.5,
        "over_filterCutoff": 10000.0,
    }, (0.55, 0.4, 0.4, 0.3, 0.0, 0.05),
     "Strong phase rotation + coupling. The spectral shimmer feeds into the coupling bus.",
     ["entangled", "phase", "rotation", "shimmer"]),

    ("Macro Entangle", {
        "over_constant": PHI, "over_depth": 2.0,
        "over_macroCoupling": 0.7,
        "over_macroDepth": 0.4, "over_macroColor": 0.5,
        "over_macroSpace": 0.3,
        "over_lfo1Rate": 0.15, "over_lfo1Depth": 0.2,
        "over_resoMix": 0.2,
        "over_filterCutoff": 8000.0,
    }, (0.5, 0.5, 0.3, 0.3, 0.3, 0.05),
     "All four macros active + coupling. Maximum entanglement surface area.",
     ["entangled", "macros", "all-four", "versatile"]),
]

# ==============================================================================
# PRISM (25) -- production-oriented genre presets
# ==============================================================================

PRISM = [
    ("Ambient Keys", {
        "over_constant": PHI, "over_depth": 2.0,
        "over_ampAtk": 0.01, "over_ampDec": 1.0, "over_ampSus": 0.4,
        "over_filterCutoff": 8000.0,
        "over_macroColor": 0.4, "over_macroSpace": 0.25,
        "over_lfo2Rate": 0.08, "over_lfo2Depth": 0.1,
    }, (0.55, 0.5, 0.1, 0.2, 0.25, 0.0),
     "Clean Fibonacci keys for ambient production. Quick attack, medium decay.",
     ["prism", "keys", "ambient", "clean"]),

    ("Spectral Bass", {
        "over_constant": PHI, "over_depth": 1.0,
        "over_partial0": 1.0, "over_partial1": 0.6, "over_partial2": 0.3,
        "over_partial3": 0.15, "over_partial4": 0.0, "over_partial5": 0.0,
        "over_partial6": 0.0, "over_partial7": 0.0,
        "over_filterCutoff": 3000.0, "over_filterRes": 0.3,
        "over_ampAtk": 0.005, "over_ampDec": 0.4, "over_ampSus": 0.5,
        "over_velBright": 0.5,
    }, (0.3, 0.65, 0.05, 0.15, 0.0, 0.15),
     "Fundamental + first 3 partials only. Warm spectral bass for any genre.",
     ["prism", "bass", "warm", "spectral"]),

    ("Golden Lead", {
        "over_constant": PHI, "over_depth": 3.0,
        "over_ampAtk": 0.005, "over_ampDec": 0.3, "over_ampSus": 0.65,
        "over_filterCutoff": 12000.0,
        "over_velBright": 0.6,
        "over_macroColor": 0.5, "over_macroDepth": 0.2,
    }, (0.65, 0.4, 0.1, 0.25, 0.0, 0.15),
     "Bright Fibonacci lead with velocity control. Cuts through any mix.",
     ["prism", "lead", "bright", "golden", "velocity"]),

    ("Techno Resonance", {
        "over_constant": PI, "over_depth": 3.0,
        "over_filterCutoff": 3000.0, "over_filterRes": 0.6,
        "over_ampAtk": 0.005, "over_ampDec": 0.2, "over_ampSus": 0.3,
        "over_resoMix": 0.4,
        "over_macroDepth": 0.4,
        "over_velBright": 0.6,
    }, (0.45, 0.4, 0.1, 0.35, 0.0, 0.3),
     "Pi cluster through resonant filter. Metallic techno stab with resonator bite.",
     ["prism", "techno", "resonant", "pi", "stab"]),

    ("Synthwave Pad", {
        "over_constant": PHI, "over_depth": 2.0,
        "over_ampAtk": 0.5, "over_ampRel": 2.0,
        "over_filterCutoff": 5000.0, "over_filterRes": 0.2,
        "over_macroColor": 0.4, "over_macroSpace": 0.4,
        "over_lfo1Rate": 0.15, "over_lfo1Depth": 0.15,
        "over_lfo2Rate": 0.1, "over_lfo2Depth": 0.1,
    }, (0.4, 0.6, 0.25, 0.25, 0.4, 0.0),
     "Warm Fibonacci pad with gentle motion. Retro-modern synthwave character.",
     ["prism", "synthwave", "pad", "warm", "retro"]),

    ("Lo-Fi Spectrum", {
        "over_constant": PHI, "over_depth": 1.5,
        "over_filterCutoff": 3000.0, "over_filterRes": 0.15,
        "over_ampAtk": 0.02, "over_ampDec": 0.5, "over_ampSus": 0.4,
        "over_macroColor": 0.15,
        "over_macroSpace": 0.2,
    }, (0.25, 0.7, 0.1, 0.2, 0.2, 0.0),
     "Dark, warm, reduced bandwidth. Lo-fi spectral character.",
     ["prism", "lofi", "dark", "warm", "muted"]),

    ("EDM Pluck", {
        "over_constant": PHI, "over_depth": 2.5,
        "over_ampAtk": 0.001, "over_ampDec": 0.25, "over_ampSus": 0.0,
        "over_ampRel": 0.3,
        "over_filterCutoff": 14000.0,
        "over_macroColor": 0.6,
        "over_velBright": 0.7,
    }, (0.65, 0.35, 0.05, 0.2, 0.0, 0.15),
     "Short, bright Fibonacci pluck for EDM drops and arpeggios.",
     ["prism", "edm", "pluck", "short", "bright"]),

    ("Film Score Swell", {
        "over_constant": E, "over_depth": 3.0,
        "over_ampAtk": 3.0, "over_ampRel": 2.0, "over_ampSus": 0.9,
        "over_filterCutoff": 8000.0,
        "over_macroColor": 0.4, "over_macroSpace": 0.5,
        "over_macroDepth": 0.3,
        "over_lfo1Rate": 0.03, "over_lfo1Depth": 0.15,
    }, (0.45, 0.55, 0.2, 0.25, 0.5, 0.0),
     "Long swell with E constant intervals. Cinematic, building, emotional.",
     ["prism", "film", "swell", "cinematic", "euler"]),

    ("Industrial Bell", {
        "over_constant": PI, "over_depth": 5.0,
        "over_ampAtk": 0.001, "over_ampDec": 1.5, "over_ampSus": 0.0,
        "over_ampRel": 2.0,
        "over_filterCutoff": 18000.0,
        "over_macroColor": 0.5,
        "over_resoMix": 0.3,
    }, (0.7, 0.3, 0.05, 0.3, 0.1, 0.25),
     "Pi deep convergents, percussive envelope. Industrial metallic bell.",
     ["prism", "industrial", "bell", "pi", "metallic"]),

    ("Chill Organ", {
        "over_constant": E, "over_depth": 2.0,
        "over_ampAtk": 0.03, "over_ampSus": 0.75,
        "over_filterCutoff": 6000.0,
        "over_macroColor": 0.3, "over_macroSpace": 0.3,
        "over_lfo2Rate": 0.06, "over_lfo2Depth": 0.1,
    }, (0.45, 0.55, 0.1, 0.2, 0.3, 0.0),
     "E constant organ for chill/downtempo. Wide intervals, gentle shimmer.",
     ["prism", "chill", "organ", "euler", "downtempo"]),

    ("Trap Crystal", {
        "over_constant": PHI, "over_depth": 4.0,
        "over_ampAtk": 0.001, "over_ampDec": 0.4, "over_ampSus": 0.0,
        "over_filterCutoff": 16000.0,
        "over_macroColor": 0.7,
        "over_velBright": 0.8,
        "over_macroSpace": 0.15,
    }, (0.75, 0.25, 0.05, 0.25, 0.15, 0.2),
     "Crystal-bright Fibonacci pluck for trap melodies. Maximum upper partial shimmer.",
     ["prism", "trap", "crystal", "bright", "pluck"]),

    ("House Stab", {
        "over_constant": SQRT2, "over_depth": 2.0,
        "over_ampAtk": 0.002, "over_ampDec": 0.15, "over_ampSus": 0.2,
        "over_filterCutoff": 8000.0, "over_filterRes": 0.35,
        "over_velBright": 0.6,
        "over_resoMix": 0.2,
    }, (0.55, 0.4, 0.05, 0.3, 0.0, 0.25),
     "Sqrt2 tension stab for house music. Tritone edge, filter resonance.",
     ["prism", "house", "stab", "sqrt2", "resonant"]),

    ("Drone Bed", {
        "over_constant": PHI, "over_depth": 3.0,
        "over_ampAtk": 2.0, "over_ampSus": 1.0, "over_ampRel": 4.0,
        "over_filterCutoff": 4000.0,
        "over_macroDepth": 0.3, "over_macroColor": 0.2,
        "over_macroSpace": 0.5,
        "over_lfo1Rate": 0.02, "over_lfo1Depth": 0.25,
        "over_lfo2Rate": 0.03, "over_lfo2Depth": 0.15,
    }, (0.3, 0.6, 0.35, 0.2, 0.5, 0.0),
     "Sustained Fibonacci drone. Slow depth sweep and phase rotation. Bed for any mix.",
     ["prism", "drone", "bed", "sustained", "fibonacci"]),

    ("Pop Bell", {
        "over_constant": PHI, "over_depth": 2.0,
        "over_ampAtk": 0.001, "over_ampDec": 0.8, "over_ampSus": 0.1,
        "over_filterCutoff": 12000.0,
        "over_macroColor": 0.4, "over_macroSpace": 0.2,
        "over_velBright": 0.5,
    }, (0.6, 0.45, 0.05, 0.2, 0.2, 0.05),
     "Soft bell tone for pop production. Fibonacci warmth, quick decay.",
     ["prism", "pop", "bell", "soft", "musical"]),

    ("DnB Metallic", {
        "over_constant": PI, "over_depth": 4.5,
        "over_ampAtk": 0.001, "over_ampDec": 0.3, "over_ampSus": 0.15,
        "over_filterCutoff": 10000.0, "over_filterRes": 0.4,
        "over_resoMix": 0.35,
        "over_velBright": 0.7,
        "over_macroColor": 0.5,
    }, (0.6, 0.35, 0.05, 0.35, 0.0, 0.35),
     "Pi metallic attack for DnB. Resonator adds edge, velocity drives brightness.",
     ["prism", "dnb", "metallic", "pi", "aggressive"]),

    ("R&B Glass", {
        "over_constant": PHI, "over_depth": 1.5,
        "over_ampAtk": 0.01, "over_ampDec": 0.6, "over_ampSus": 0.45,
        "over_filterCutoff": 7000.0,
        "over_macroColor": 0.35, "over_macroSpace": 0.3,
        "over_lfo2Rate": 0.1, "over_lfo2Depth": 0.08,
    }, (0.5, 0.55, 0.1, 0.2, 0.3, 0.0),
     "Smooth Fibonacci glass for R&B. Warm but clear, gentle phase movement.",
     ["prism", "rnb", "glass", "smooth", "warm"]),

    ("Experimental Stab", {
        "over_constant": SQRT2, "over_depth": 5.0,
        "over_ampAtk": 0.001, "over_ampDec": 0.2, "over_ampSus": 0.0,
        "over_filterCutoff": 14000.0,
        "over_macroColor": 0.6,
        "over_resoMix": 0.3,
        "over_velBright": 0.8,
    }, (0.6, 0.3, 0.05, 0.3, 0.0, 0.35),
     "Deep Sqrt2 convergents, ultra-short decay. Experimental percussive hit.",
     ["prism", "experimental", "stab", "sqrt2", "percussive"]),

    ("Chillout Wash", {
        "over_constant": PHI, "over_depth": 2.5,
        "over_ampAtk": 1.5, "over_ampRel": 3.0, "over_ampSus": 0.85,
        "over_filterCutoff": 5000.0,
        "over_macroColor": 0.3, "over_macroSpace": 0.5,
        "over_lfo1Rate": 0.04, "over_lfo1Depth": 0.2,
        "over_lfo2Rate": 0.06, "over_lfo2Depth": 0.15,
    }, (0.35, 0.6, 0.3, 0.2, 0.5, 0.0),
     "Slow wash of Fibonacci harmonics. Background texture for chillout tracks.",
     ["prism", "chillout", "wash", "texture", "background"]),

    ("Funk Pluck", {
        "over_constant": E, "over_depth": 2.0,
        "over_ampAtk": 0.001, "over_ampDec": 0.15, "over_ampSus": 0.1,
        "over_filterCutoff": 8000.0, "over_filterRes": 0.35,
        "over_velBright": 0.7,
        "over_resoMix": 0.2,
    }, (0.55, 0.45, 0.05, 0.2, 0.0, 0.2),
     "E constant intervals, quick pluck, resonant filter. Funky spectral character.",
     ["prism", "funk", "pluck", "euler", "resonant"]),

    ("Orchestral Shimmer", {
        "over_constant": PHI, "over_depth": 2.0,
        "over_ampAtk": 0.8, "over_ampRel": 2.0,
        "over_filterCutoff": 9000.0,
        "over_macroColor": 0.5, "over_macroSpace": 0.4,
        "over_lfo2Rate": 0.1, "over_lfo2Depth": 0.15,
    }, (0.5, 0.55, 0.15, 0.25, 0.4, 0.0),
     "Fibonacci shimmer for orchestral hybrid scoring. Phase rotation adds ensemble width.",
     ["prism", "orchestral", "shimmer", "scoring", "ensemble"]),

    ("Acid Spectral", {
        "over_constant": PI, "over_depth": 2.5,
        "over_filterCutoff": 2000.0, "over_filterRes": 0.7,
        "over_ampAtk": 0.005, "over_ampDec": 0.3, "over_ampSus": 0.3,
        "over_macroDepth": 0.5,
        "over_velBright": 0.6,
        "over_resoMix": 0.25,
    }, (0.4, 0.4, 0.1, 0.3, 0.0, 0.35),
     "Pi convergents through high-resonance filter. Acid-inspired spectral squelch.",
     ["prism", "acid", "resonant", "pi", "squelch"]),

    ("Cinematic Tension", {
        "over_constant": SQRT2, "over_depth": 4.0,
        "over_ampAtk": 2.0, "over_ampRel": 3.0, "over_ampSus": 0.8,
        "over_filterCutoff": 5000.0,
        "over_macroSpace": 0.5, "over_macroColor": 0.3,
        "over_lfo1Rate": 0.03, "over_lfo1Depth": 0.2,
    }, (0.35, 0.45, 0.25, 0.25, 0.5, 0.15),
     "Sqrt2 Pell tensions for film scoring. Unresolved, building, suspenseful.",
     ["prism", "cinematic", "tension", "sqrt2", "suspense"]),

    ("Minimal Sine", {
        "over_constant": PHI, "over_depth": 0.0,
        "over_partial0": 1.0, "over_partial1": 0.0, "over_partial2": 0.0,
        "over_partial3": 0.0, "over_partial4": 0.0, "over_partial5": 0.0,
        "over_partial6": 0.0, "over_partial7": 0.0,
        "over_filterCutoff": 20000.0,
        "over_ampAtk": 0.01, "over_ampSus": 0.7,
    }, (0.2, 0.6, 0.0, 0.05, 0.0, 0.0),
     "Single sine partial. The absolute minimum. For production layering.",
     ["prism", "minimal", "sine", "pure", "layer"]),

    ("Neo-Soul Glass", {
        "over_constant": E, "over_depth": 1.5,
        "over_ampAtk": 0.01, "over_ampDec": 0.7, "over_ampSus": 0.5,
        "over_filterCutoff": 7000.0,
        "over_macroColor": 0.4, "over_macroSpace": 0.25,
        "over_lfo2Rate": 0.07, "over_lfo2Depth": 0.08,
        "over_velBright": 0.5,
    }, (0.5, 0.55, 0.1, 0.2, 0.25, 0.0),
     "E constant warmth for neo-soul keys. Gentle, clear, expressive.",
     ["prism", "neo-soul", "keys", "euler", "warm"]),

    ("Garage Stab", {
        "over_constant": PHI, "over_depth": 2.5,
        "over_ampAtk": 0.002, "over_ampDec": 0.2, "over_ampSus": 0.15,
        "over_filterCutoff": 7000.0, "over_filterRes": 0.3,
        "over_velBright": 0.6,
        "over_macroColor": 0.4,
    }, (0.5, 0.45, 0.05, 0.25, 0.0, 0.2),
     "Fibonacci stab for UK garage. Resonant, punchy, spectral.",
     ["prism", "garage", "stab", "fibonacci", "punchy"]),
]

# ==============================================================================
# FLUX (20) -- movement-heavy, LFO-driven, spectral animation
# ==============================================================================

FLUX = [
    ("Depth Pulse", {
        "over_constant": PHI, "over_depth": 1.0,
        "over_macroDepth": 0.6,
        "over_lfo1Rate": 2.0, "over_lfo1Depth": 0.6,
        "over_filterCutoff": 8000.0,
        "over_macroColor": 0.4,
    }, (0.5, 0.45, 0.7, 0.3, 0.0, 0.15),
     "Fast LFO1 pulsing through convergent depth. Rapid spectral mutation.",
     ["flux", "pulse", "fast", "lfo", "mutation"]),

    ("Phase Cascade", {
        "over_constant": PHI, "over_depth": 3.0,
        "over_lfo2Rate": 1.5, "over_lfo2Depth": 0.6,
        "over_macroColor": 0.5,
        "over_filterCutoff": 12000.0,
        "over_macroSpace": 0.2,
    }, (0.6, 0.4, 0.65, 0.3, 0.2, 0.05),
     "Fast phase rotation across all 8 partials. Cascading spectral movement.",
     ["flux", "phase", "cascade", "fast", "shimmer"]),

    ("Dual LFO Weave", {
        "over_constant": PHI, "over_depth": 2.0,
        "over_lfo1Rate": 0.3, "over_lfo1Depth": 0.4,
        "over_lfo2Rate": 0.5, "over_lfo2Depth": 0.35,
        "over_macroColor": 0.4, "over_macroDepth": 0.3,
        "over_macroSpace": 0.2,
    }, (0.5, 0.5, 0.55, 0.25, 0.2, 0.05),
     "Both LFOs at different rates weaving through depth and phase space.",
     ["flux", "dual-lfo", "weave", "polyrhythmic"]),

    ("Color Throb", {
        "over_constant": PHI, "over_depth": 3.0,
        "over_macroColor": 0.8,
        "over_lfo2Rate": 0.8, "over_lfo2Depth": 0.5,
        "over_filterCutoff": 10000.0,
        "over_ampSus": 0.8,
    }, (0.6, 0.4, 0.6, 0.3, 0.0, 0.1),
     "High COLOR + fast phase rotation. Upper partials throb with iridescent shimmer.",
     ["flux", "color", "throb", "shimmer", "iridescent"]),

    ("Metallic Sweep", {
        "over_constant": PI, "over_depth": 2.0,
        "over_macroDepth": 0.7,
        "over_lfo1Rate": 0.5, "over_lfo1Depth": 0.5,
        "over_filterCutoff": 6000.0, "over_filterRes": 0.35,
        "over_resoMix": 0.3,
    }, (0.45, 0.4, 0.55, 0.35, 0.0, 0.2),
     "Pi convergents swept by depth LFO. Metallic texture in constant motion.",
     ["flux", "metallic", "sweep", "pi", "lfo"]),

    ("Fibonacci Arp Sim", {
        "over_constant": PHI, "over_depth": 2.0,
        "over_lfo1Rate": 4.0, "over_lfo1Depth": 0.8,
        "over_ampAtk": 0.001, "over_ampDec": 0.15, "over_ampSus": 0.3,
        "over_filterCutoff": 10000.0,
        "over_macroColor": 0.5,
    }, (0.6, 0.4, 0.8, 0.25, 0.0, 0.15),
     "Fast depth LFO simulates arpeggiation by rapidly cycling through convergent ratios.",
     ["flux", "arp", "fast", "fibonacci", "rhythmic"]),

    ("Slow Morph", {
        "over_constant": PHI, "over_depth": 0.0,
        "over_macroDepth": 1.0,
        "over_lfo1Rate": 0.02, "over_lfo1Depth": 0.8,
        "over_ampAtk": 0.5, "over_ampSus": 0.85,
        "over_filterCutoff": 7000.0,
        "over_macroSpace": 0.3,
    }, (0.4, 0.55, 0.6, 0.2, 0.3, 0.0),
     "Maximum depth range sweep at 0.02 Hz. Full convergent journey every 50 seconds.",
     ["flux", "morph", "slow", "journey", "full-range"]),

    ("Resonant Flutter", {
        "over_constant": E, "over_depth": 3.0,
        "over_resoMix": 0.5,
        "over_lfo1Rate": 3.0, "over_lfo1Depth": 0.3,
        "over_lfo2Rate": 2.0, "over_lfo2Depth": 0.2,
        "over_filterCutoff": 6000.0, "over_filterRes": 0.4,
    }, (0.5, 0.45, 0.65, 0.3, 0.1, 0.15),
     "E intervals with fast LFOs and strong resonator. Fluttering, alive, restless.",
     ["flux", "resonant", "flutter", "euler", "fast"]),

    ("Tension Wobble", {
        "over_constant": SQRT2, "over_depth": 3.0,
        "over_lfo1Rate": 1.0, "over_lfo1Depth": 0.5,
        "over_filterCutoff": 5000.0, "over_filterRes": 0.4,
        "over_macroColor": 0.3,
    }, (0.4, 0.4, 0.6, 0.3, 0.0, 0.25),
     "Sqrt2 tritone tensions with depth wobble. Uneasy, rhythmic, dark.",
     ["flux", "wobble", "sqrt2", "tension", "dark"]),

    ("Crystal Rain", {
        "over_constant": PHI, "over_depth": 4.0,
        "over_ampAtk": 0.001, "over_ampDec": 0.2, "over_ampSus": 0.0,
        "over_lfo1Rate": 5.0, "over_lfo1Depth": 0.4,
        "over_filterCutoff": 16000.0,
        "over_macroColor": 0.7,
        "over_macroSpace": 0.3,
    }, (0.7, 0.3, 0.7, 0.25, 0.3, 0.1),
     "Percussive envelope + fast depth LFO. Each note is a burst of crystal droplets.",
     ["flux", "crystal", "rain", "percussive", "fast"]),

    ("Breathing Room", {
        "over_constant": PHI, "over_depth": 2.0,
        "over_lfo1Rate": 0.1, "over_lfo1Depth": 0.3,
        "over_macroSpace": 0.6,
        "over_macroDepth": 0.3,
        "over_ampAtk": 0.5, "over_ampRel": 2.0,
    }, (0.4, 0.55, 0.35, 0.2, 0.6, 0.0),
     "Gentle depth breathing in a spacious reverb room. Inhale, exhale, repeat.",
     ["flux", "breathing", "room", "gentle", "space"]),

    ("Scatter Partials", {
        "over_constant": PHI, "over_depth": 5.0,
        "over_partial0": 0.3, "over_partial1": 0.8, "over_partial2": 0.2,
        "over_partial3": 0.9, "over_partial4": 0.1, "over_partial5": 0.7,
        "over_partial6": 0.4, "over_partial7": 0.6,
        "over_lfo2Rate": 0.8, "over_lfo2Depth": 0.4,
        "over_filterCutoff": 14000.0,
    }, (0.55, 0.4, 0.5, 0.4, 0.0, 0.1),
     "Irregular partial amplitudes + phase rotation. Scattered, unpredictable shimmer.",
     ["flux", "scatter", "irregular", "unpredictable"]),

    ("Subharmonic Drift", {
        "over_constant": PHI, "over_depth": 0.5,
        "over_partial0": 1.0, "over_partial1": 0.7, "over_partial2": 0.4,
        "over_partial3": 0.2, "over_partial4": 0.0, "over_partial5": 0.0,
        "over_partial6": 0.0, "over_partial7": 0.0,
        "over_lfo1Rate": 0.08, "over_lfo1Depth": 0.3,
        "over_filterCutoff": 3000.0,
    }, (0.2, 0.65, 0.35, 0.15, 0.0, 0.0),
     "Low partials only, gentle depth drift. Warm subharmonic movement.",
     ["flux", "sub", "drift", "warm", "low"]),

    ("Spectral Strobe", {
        "over_constant": PI, "over_depth": 3.0,
        "over_lfo1Rate": 8.0, "over_lfo1Depth": 0.7,
        "over_filterCutoff": 10000.0,
        "over_macroColor": 0.5,
        "over_ampAtk": 0.001, "over_ampSus": 0.6,
    }, (0.55, 0.35, 0.85, 0.35, 0.0, 0.25),
     "Maximum LFO rate strobing through Pi convergent table. Aggressive spectral flicker.",
     ["flux", "strobe", "fast", "pi", "aggressive"]),

    ("Evolving Glass", {
        "over_constant": E, "over_depth": 2.0,
        "over_lfo1Rate": 0.05, "over_lfo1Depth": 0.35,
        "over_lfo2Rate": 0.08, "over_lfo2Depth": 0.25,
        "over_ampAtk": 1.0, "over_ampRel": 3.0,
        "over_filterCutoff": 8000.0,
        "over_macroColor": 0.4, "over_macroSpace": 0.3,
    }, (0.5, 0.5, 0.4, 0.25, 0.3, 0.0),
     "E constant glass tones evolving through both LFOs. Slowly changing, never static.",
     ["flux", "evolving", "glass", "euler", "dual-lfo"]),

    ("Harmonic Tide", {
        "over_constant": PHI, "over_depth": 2.0,
        "over_macroDepth": 0.5,
        "over_lfo1Rate": 0.15, "over_lfo1Depth": 0.5,
        "over_macroColor": 0.5,
        "over_macroSpace": 0.3,
        "over_ampSus": 0.85,
    }, (0.5, 0.5, 0.5, 0.25, 0.3, 0.0),
     "DEPTH macro + LFO1 create a tidal sweep of harmonics. In and out, in and out.",
     ["flux", "tide", "sweep", "harmonic", "fibonacci"]),

    ("Velocity Storm", {
        "over_constant": PI, "over_depth": 4.0,
        "over_velBright": 1.0,
        "over_filterCutoff": 4000.0, "over_filterRes": 0.5,
        "over_lfo1Rate": 1.5, "over_lfo1Depth": 0.4,
        "over_macroColor": 0.3,
    }, (0.4, 0.4, 0.5, 0.3, 0.0, 0.35),
     "Maximum velocity sensitivity + Pi depth LFO. Every keystroke is different.",
     ["flux", "velocity", "storm", "pi", "dynamic"]),

    ("Phase Tornado", {
        "over_constant": PHI, "over_depth": 4.0,
        "over_lfo2Rate": 6.0, "over_lfo2Depth": 0.7,
        "over_lfo1Rate": 0.5, "over_lfo1Depth": 0.3,
        "over_filterCutoff": 14000.0,
        "over_macroColor": 0.6,
    }, (0.6, 0.35, 0.8, 0.3, 0.0, 0.15),
     "Near-maximum phase rotation rate. Partials spin against each other in spectral vortex.",
     ["flux", "tornado", "phase", "fast", "vortex"]),

    ("Macro Dance", {
        "over_constant": PHI, "over_depth": 1.0,
        "over_macroDepth": 0.6, "over_macroColor": 0.6,
        "over_macroSpace": 0.4,
        "over_lfo1Rate": 0.3, "over_lfo1Depth": 0.4,
        "over_lfo2Rate": 0.5, "over_lfo2Depth": 0.3,
        "over_resoMix": 0.2,
    }, (0.5, 0.5, 0.55, 0.3, 0.4, 0.05),
     "All macros engaged, both LFOs active. The full OVERTONE dance.",
     ["flux", "macros", "dance", "all-active", "movement"]),

    ("Resonant Bounce", {
        "over_constant": E, "over_depth": 2.5,
        "over_resoMix": 0.5,
        "over_lfo1Rate": 1.0, "over_lfo1Depth": 0.4,
        "over_filterCutoff": 5000.0, "over_filterRes": 0.45,
        "over_macroColor": 0.3,
    }, (0.45, 0.45, 0.55, 0.3, 0.1, 0.15),
     "E intervals bouncing through resonator + filter. Rhythmic spectral motion.",
     ["flux", "resonant", "bounce", "euler", "rhythmic"]),
]

# ==============================================================================
# AETHER (15) -- experimental/deep/extreme convergent exploration
# ==============================================================================

AETHER = [
    ("Maximum Depth", {
        "over_constant": PHI, "over_depth": 7.0,
        "over_macroDepth": 0.0,
        "over_filterCutoff": 18000.0,
        "over_macroColor": 0.6,
        "over_ampRel": 4.0,
    }, (0.65, 0.35, 0.0, 0.35, 0.0, 0.2),
     "Depth 7: the deepest convergent index. Partials at golden ratio limit. Pure irrationality.",
     ["aether", "maximum", "deep", "irrational", "golden-ratio"]),

    ("All Constants", {
        "over_constant": PI, "over_depth": 3.0,
        "over_macroDepth": 0.5,
        "over_lfo1Rate": 0.01, "over_lfo1Depth": 0.8,
        "over_filterCutoff": 8000.0,
        "over_macroColor": 0.4,
    }, (0.5, 0.4, 0.6, 0.3, 0.0, 0.1),
     "Start with Pi, sweep DEPTH macro through full range. Listen to the mathematics change.",
     ["aether", "constants", "sweep", "mathematics", "exploration"]),

    ("Inverse Spectrum", {
        "over_constant": PHI, "over_depth": 4.0,
        "over_partial0": 0.1, "over_partial1": 0.2, "over_partial2": 0.3,
        "over_partial3": 0.4, "over_partial4": 0.5, "over_partial5": 0.6,
        "over_partial6": 0.8, "over_partial7": 1.0,
        "over_filterCutoff": 16000.0,
        "over_macroColor": 0.3,
    }, (0.7, 0.25, 0.0, 0.4, 0.0, 0.2),
     "Inverted partial amplitudes: highest partials loudest. Alien, bright, unstable.",
     ["aether", "inverse", "alien", "experimental", "upper"]),

    ("Geological Time", {
        "over_constant": PHI, "over_depth": 2.0,
        "over_macroDepth": 0.5,
        "over_lfo1Rate": 0.01, "over_lfo1Depth": 0.6,
        "over_lfo2Rate": 0.015, "over_lfo2Depth": 0.5,
        "over_ampAtk": 4.0, "over_ampRel": 8.0, "over_ampSus": 0.9,
        "over_macroSpace": 0.6,
    }, (0.35, 0.55, 0.5, 0.2, 0.6, 0.0),
     "Both LFOs at minimum rates, maximum envelope times. Sound evolves over minutes.",
     ["aether", "geological", "slowest", "minimal-rate", "deep-time"]),

    ("Shimmer Cloud", {
        "over_constant": PHI, "over_depth": 5.0,
        "over_macroCoupling": 0.8,
        "over_macroColor": 0.8,
        "over_lfo2Rate": 2.0, "over_lfo2Depth": 0.5,
        "over_filterCutoff": 18000.0,
        "over_macroSpace": 0.5,
    }, (0.7, 0.3, 0.5, 0.35, 0.5, 0.0),
     "Maximum shimmer: COUPLING flutter + COLOR boost + fast phase rotation. Spectral aurora.",
     ["aether", "shimmer", "cloud", "aurora", "extreme"]),

    ("Null Partials", {
        "over_constant": PHI, "over_depth": 3.0,
        "over_partial0": 1.0, "over_partial1": 0.0, "over_partial2": 1.0,
        "over_partial3": 0.0, "over_partial4": 1.0, "over_partial5": 0.0,
        "over_partial6": 1.0, "over_partial7": 0.0,
        "over_filterCutoff": 14000.0,
    }, (0.55, 0.35, 0.0, 0.3, 0.0, 0.15),
     "Alternating partial pattern: odd partials on, even off. Hollow, square-wave-like spectrum.",
     ["aether", "null", "alternating", "hollow", "experimental"]),

    ("Pi Unison", {
        "over_constant": PI, "over_depth": 5.0,
        "over_partial0": 1.0, "over_partial1": 1.0, "over_partial2": 1.0,
        "over_partial3": 1.0, "over_partial4": 1.0, "over_partial5": 1.0,
        "over_partial6": 1.0, "over_partial7": 1.0,
        "over_filterCutoff": 18000.0,
    }, (0.65, 0.3, 0.0, 0.7, 0.0, 0.25),
     "All 8 Pi partials at full amplitude, deep convergence. Dense beating cluster.",
     ["aether", "pi", "unison", "dense", "beating"]),

    ("Fibonacci Ghost", {
        "over_constant": PHI, "over_depth": 6.0,
        "over_partial0": 0.05, "over_partial1": 0.1, "over_partial2": 0.15,
        "over_partial3": 0.1, "over_partial4": 0.05, "over_partial5": 0.1,
        "over_partial6": 0.05, "over_partial7": 0.08,
        "over_filterCutoff": 10000.0,
        "over_macroSpace": 0.7,
        "over_ampAtk": 2.0, "over_ampRel": 6.0,
    }, (0.3, 0.45, 0.15, 0.15, 0.7, 0.0),
     "All partials nearly silent. A ghost of the golden ratio, barely audible.",
     ["aether", "ghost", "quiet", "fibonacci", "fragile"]),

    ("Resonator Solo", {
        "over_constant": PHI, "over_depth": 3.0,
        "over_resoMix": 0.9,
        "over_filterCutoff": 6000.0, "over_filterRes": 0.5,
        "over_macroSpace": 0.3,
    }, (0.45, 0.5, 0.05, 0.3, 0.3, 0.15),
     "Allpass resonator at 90% mix. The comb dominates, fundamental reinforced by physics.",
     ["aether", "resonator", "comb", "extreme", "physics"]),

    ("Four Constants Walk", {
        "over_constant": PI, "over_depth": 4.0,
        "over_macroDepth": 0.8,
        "over_lfo1Rate": 0.03, "over_lfo1Depth": 0.7,
        "over_lfo2Rate": 0.05, "over_lfo2Depth": 0.3,
        "over_macroColor": 0.4,
        "over_macroSpace": 0.4,
        "over_ampAtk": 1.0, "over_ampRel": 4.0,
    }, (0.45, 0.45, 0.55, 0.3, 0.4, 0.1),
     "Pi at high depth + maximum DEPTH macro. Switch constants to hear how mathematics shapes timbre.",
     ["aether", "constants", "walk", "exploration", "educational"]),

    ("Entropy Field", {
        "over_constant": SQRT2, "over_depth": 7.0,
        "over_lfo1Rate": 3.0, "over_lfo1Depth": 0.7,
        "over_lfo2Rate": 4.0, "over_lfo2Depth": 0.6,
        "over_filterCutoff": 12000.0,
        "over_macroColor": 0.6,
    }, (0.55, 0.3, 0.8, 0.35, 0.0, 0.3),
     "Sqrt2 at maximum depth, both LFOs at high rate. Maximum spectral entropy.",
     ["aether", "entropy", "chaos", "sqrt2", "maximum"]),

    ("Chamber Seed", {
        "over_constant": PHI, "over_depth": 0.0,
        "over_partial0": 1.0, "over_partial1": 0.0, "over_partial2": 0.0,
        "over_partial3": 0.0, "over_partial4": 0.0, "over_partial5": 0.0,
        "over_partial6": 0.0, "over_partial7": 0.0,
        "over_macroDepth": 1.0,
        "over_ampAtk": 3.0, "over_ampSus": 0.9, "over_ampRel": 5.0,
        "over_macroSpace": 0.6,
    }, (0.2, 0.55, 0.15, 0.05, 0.6, 0.0),
     "A single sine. DEPTH macro adds convergent partials. The Nautilus building its first chamber.",
     ["aether", "seed", "growth", "nautilus", "minimal"]),

    ("E Deep", {
        "over_constant": E, "over_depth": 7.0,
        "over_filterCutoff": 14000.0,
        "over_macroColor": 0.5,
        "over_macroSpace": 0.4,
        "over_lfo1Rate": 0.04, "over_lfo1Depth": 0.3,
        "over_ampAtk": 1.0, "over_ampRel": 3.0,
    }, (0.55, 0.45, 0.25, 0.3, 0.4, 0.05),
     "E constant at maximum depth. Wide organ-like intervals at their most extreme.",
     ["aether", "euler", "deep", "extreme", "organ"]),

    ("Spectral Void", {
        "over_constant": PHI, "over_depth": 5.0,
        "over_partial0": 0.0, "over_partial1": 0.0, "over_partial2": 0.0,
        "over_partial3": 0.0, "over_partial4": 0.5, "over_partial5": 0.7,
        "over_partial6": 0.9, "over_partial7": 1.0,
        "over_filterCutoff": 18000.0,
        "over_macroSpace": 0.5,
        "over_ampRel": 3.0,
    }, (0.65, 0.2, 0.0, 0.3, 0.5, 0.15),
     "Only upper partials active, fundamental absent. A spectrum with a void at its center.",
     ["aether", "void", "upper-only", "experimental", "hollow"]),

    ("Maximum Everything", {
        "over_constant": PHI, "over_depth": 7.0,
        "over_partial0": 1.0, "over_partial1": 1.0, "over_partial2": 1.0,
        "over_partial3": 1.0, "over_partial4": 1.0, "over_partial5": 1.0,
        "over_partial6": 1.0, "over_partial7": 1.0,
        "over_macroDepth": 1.0, "over_macroColor": 1.0,
        "over_macroCoupling": 1.0, "over_macroSpace": 1.0,
        "over_lfo1Rate": 5.0, "over_lfo1Depth": 1.0,
        "over_lfo2Rate": 7.0, "over_lfo2Depth": 1.0,
        "over_resoMix": 0.8,
        "over_filterCutoff": 20000.0, "over_filterRes": 0.8,
        "over_velBright": 1.0,
    }, (0.8, 0.3, 0.9, 0.8, 0.8, 0.5),
     "Every parameter at or near maximum. The Nautilus at absolute peak. Use with caution.",
     ["aether", "maximum", "everything", "extreme", "full"]),
]

# ==============================================================================
# FAMILY (15) -- OVERTONE + 15 fleet engines
# ==============================================================================

FAMILY_PRESETS = []
partners = [
    ("Overdub", "Spectral harmonics through dub delay and spring reverb.",
     {"over_constant": PHI, "over_depth": 2.0, "over_macroSpace": 0.3, "over_macroColor": 0.3},
     (0.4, 0.55, 0.3, 0.25, 0.4, 0.1), ["family", "overdub", "dub", "spectral"]),

    ("Odyssey", "Fibonacci drift meets analog drift. Two wanderers.",
     {"over_constant": PHI, "over_depth": 3.0, "over_lfo1Rate": 0.05, "over_lfo1Depth": 0.25, "over_macroSpace": 0.25},
     (0.4, 0.55, 0.4, 0.25, 0.25, 0.05), ["family", "odyssey", "drift", "analog"]),

    ("Onset", "Fibonacci partials triggered by drum envelopes.",
     {"over_constant": PHI, "over_depth": 4.0, "over_ampAtk": 0.001, "over_ampDec": 0.3, "over_ampSus": 0.0, "over_macroColor": 0.5},
     (0.55, 0.35, 0.1, 0.3, 0.0, 0.25), ["family", "onset", "drums", "percussive"]),

    ("Overworld", "Spectral ice meets chip synthesis. Mathematical chiptune.",
     {"over_constant": E, "over_depth": 2.0, "over_filterCutoff": 8000.0, "over_ampDec": 0.4, "over_ampSus": 0.3},
     (0.55, 0.4, 0.1, 0.25, 0.1, 0.15), ["family", "overworld", "chip", "retro"]),

    ("Opal", "Convergent partials granulated. Fibonacci cloud.",
     {"over_constant": PHI, "over_depth": 3.0, "over_ampAtk": 1.0, "over_ampRel": 3.0, "over_macroSpace": 0.4, "over_macroColor": 0.3},
     (0.4, 0.55, 0.4, 0.35, 0.45, 0.0), ["family", "opal", "granular", "cloud"]),

    ("Organon", "Mathematical harmonics meet metabolic synthesis. Living spectrum.",
     {"over_constant": PHI, "over_depth": 3.0, "over_lfo1Rate": 0.03, "over_lfo1Depth": 0.3, "over_macroDepth": 0.3},
     (0.4, 0.55, 0.4, 0.3, 0.2, 0.05), ["family", "organon", "metabolic", "living"]),

    ("Ouroboros", "Spectral convergents in a feedback loop. Infinite spectrum.",
     {"over_constant": PI, "over_depth": 4.0, "over_resoMix": 0.4, "over_filterRes": 0.4, "over_macroColor": 0.3},
     (0.45, 0.45, 0.35, 0.35, 0.1, 0.25), ["family", "ouroboros", "feedback", "infinite"]),

    ("Oblong", "Warm amber + spectral ice. Temperature contrast.",
     {"over_constant": PHI, "over_depth": 2.0, "over_filterCutoff": 5000.0, "over_macroSpace": 0.3, "over_macroColor": 0.2},
     (0.35, 0.65, 0.2, 0.25, 0.3, 0.0), ["family", "oblong", "warm", "amber"]),

    ("Obese", "Mathematical partials through saturation. Fat Fibonacci.",
     {"over_constant": PHI, "over_depth": 2.5, "over_filterCutoff": 4000.0, "over_filterRes": 0.4, "over_macroColor": 0.4},
     (0.4, 0.55, 0.15, 0.35, 0.0, 0.35), ["family", "obese", "fat", "saturation"]),

    ("Oracle", "Convergent fractions meet stochastic synthesis. Mathematical prophecy.",
     {"over_constant": E, "over_depth": 4.0, "over_lfo1Rate": 0.04, "over_lfo1Depth": 0.3, "over_macroDepth": 0.4},
     (0.45, 0.45, 0.35, 0.3, 0.15, 0.1), ["family", "oracle", "stochastic", "prophecy"]),

    ("Oceanic", "Spectral Nautilus in the deep ocean. Phosphorescent harmonics.",
     {"over_constant": PHI, "over_depth": 3.0, "over_filterCutoff": 4000.0, "over_macroSpace": 0.5, "over_lfo1Rate": 0.02, "over_lfo1Depth": 0.2},
     (0.3, 0.6, 0.3, 0.25, 0.55, 0.0), ["family", "oceanic", "deep", "phosphorescent"]),

    ("Overlap", "Convergent spectra entangled in knot topology.",
     {"over_constant": PHI, "over_depth": 3.5, "over_macroCoupling": 0.6, "over_macroColor": 0.4, "over_macroSpace": 0.2},
     (0.5, 0.45, 0.25, 0.3, 0.2, 0.1), ["family", "overlap", "knot", "entangled"]),

    ("Origami", "Spectral partials folded like paper. Fibonacci folds.",
     {"over_constant": PHI, "over_depth": 3.0, "over_filterCutoff": 6000.0, "over_macroColor": 0.5, "over_resoMix": 0.25},
     (0.5, 0.45, 0.15, 0.3, 0.1, 0.2), ["family", "origami", "fold", "fibonacci"]),

    ("Obrix", "Spectral partials meet modular bricks. Reef harmonics.",
     {"over_constant": PHI, "over_depth": 2.5, "over_macroColor": 0.4, "over_macroSpace": 0.2, "over_resoMix": 0.2},
     (0.45, 0.5, 0.2, 0.3, 0.2, 0.15), ["family", "obrix", "reef", "bricks"]),

    ("Osprey", "Fibonacci harmonics blown across the shore.",
     {"over_constant": PHI, "over_depth": 2.0, "over_macroSpace": 0.4, "over_macroColor": 0.35, "over_lfo2Rate": 0.08, "over_lfo2Depth": 0.15},
     (0.45, 0.55, 0.2, 0.2, 0.4, 0.0), ["family", "osprey", "shore", "wind"]),
]

for partner_name, desc, overrides, dna, tags in partners:
    name = f"Nautilus x {partner_name}"
    overrides_with_coupling = dict(overrides)
    overrides_with_coupling["over_macroCoupling"] = 0.5
    FAMILY_PRESETS.append(make_preset(name, "Family", overrides_with_coupling, dna, desc, tags,
        engines=["Overtone", partner_name],
        coupling=[{"engineA": "Overtone", "engineB": partner_name,
                   "type": "AmpToFilter", "amount": 0.5}],
        coupling_intensity="Medium"))


# ==============================================================================
# GENERATE ALL PRESETS
# ==============================================================================

def main():
    count = 0
    # Foundation
    for name, overrides, dna, desc, tags in FOUNDATION:
        save(make_preset(name, "Foundation", overrides, dna, desc, tags))
        count += 1
    # Atmosphere
    for name, overrides, dna, desc, tags in ATMOSPHERE:
        save(make_preset(name, "Atmosphere", overrides, dna, desc, tags))
        count += 1
    # Entangled
    for name, overrides, dna, desc, tags in ENTANGLED:
        save(make_preset(name, "Entangled", overrides, dna, desc, tags))
        count += 1
    # Prism
    for name, overrides, dna, desc, tags in PRISM:
        save(make_preset(name, "Prism", overrides, dna, desc, tags))
        count += 1
    # Flux
    for name, overrides, dna, desc, tags in FLUX:
        save(make_preset(name, "Flux", overrides, dna, desc, tags))
        count += 1
    # Aether
    for name, overrides, dna, desc, tags in AETHER:
        save(make_preset(name, "Aether", overrides, dna, desc, tags))
        count += 1
    # Family (pre-built)
    for preset in FAMILY_PRESETS:
        save(preset)
        count += 1

    print(f"Generated {count} OVERTONE presets across 7 moods")
    # Distribution check
    from collections import Counter
    moods = Counter()
    moods["Foundation"] = len(FOUNDATION)
    moods["Atmosphere"] = len(ATMOSPHERE)
    moods["Entangled"] = len(ENTANGLED)
    moods["Prism"] = len(PRISM)
    moods["Flux"] = len(FLUX)
    moods["Aether"] = len(AETHER)
    moods["Family"] = len(FAMILY_PRESETS)
    for mood, n in sorted(moods.items()):
        print(f"  {mood}: {n}")
    print(f"  TOTAL: {sum(moods.values())}")


if __name__ == "__main__":
    main()
