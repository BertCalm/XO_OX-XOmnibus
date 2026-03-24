#!/usr/bin/env python3
"""
xpn_space_dimension_expander.py

Targets the SPACE DNA dimension, which is compressed in the fleet (most presets
cluster 0.4–0.7). Generates presets at the two extreme poles:

  Intimate (Foundation)  — space 0.0–0.10  — dry, close-miked, in-your-face
  Vast     (Aether)      — space 0.90–1.0  — cosmic, cavernous, infinite

  25 Intimate + 25 Vast = 50 presets total.

Intimate engines (naturally dry):
    ONSET, OBLONG, OVERBITE, OBESE, OVERDUB, OVERWORLD, OPTIC, ORBITAL, OTTONI

Vast engines (naturally spacious):
    OPAL, ORACLE, OBSIDIAN, ORGANON, OCEANIC, OSPREY, OMBRE, OHM, ORPHICA

Usage:
    python3 Tools/xpn_space_dimension_expander.py
    python3 Tools/xpn_space_dimension_expander.py --dry-run
    python3 Tools/xpn_space_dimension_expander.py --band intimate
    python3 Tools/xpn_space_dimension_expander.py --band vast
    python3 Tools/xpn_space_dimension_expander.py --seed 42
"""

import argparse
import json
import os
import random
import sys
from pathlib import Path

# ---------------------------------------------------------------------------
# Vocabulary
# ---------------------------------------------------------------------------

INTIMATE_NAMES = [
    # Dry / close / present evocations
    "Point Blank",
    "Booth Signal",
    "Dead Room Hit",
    "Contact Mic",
    "Anechoic Strike",
    "Pocket Pulse",
    "Live Wire Feed",
    "Face Level",
    "Close Take",
    "Dry Wick",
    "No Reverb Needed",
    "Skin Contact",
    "Bare Room",
    "Gravel Presence",
    "Direct Source",
    "Bone Conduction",
    "Studio One",
    "Signal Before Air",
    "Chalk Line",
    "Zero Distance",
    "Sealed Chamber",
    "Raw Input",
    "Forehead Touch",
    "Short Chain",
    "Sub Skin",
]

VAST_NAMES = [
    # Cosmic / cavernous / infinite evocations
    "Cathedral Dissolve",
    "Infinite Ceiling",
    "Abyssal Hall",
    "Void Bloom",
    "Event Horizon Choir",
    "Cosmic Tidal Pull",
    "Deep Field Wash",
    "Atmosphere Exit",
    "Nebula Sustain",
    "Pressure From Above",
    "Glacial Cave",
    "Orbital Decay",
    "Frozen Expanse",
    "Signal To Sky",
    "Last Known Room",
    "Tectonic Echo",
    "Sea Cave Resonance",
    "Distant Shore Haze",
    "Horizon Fold",
    "Stellar Mist",
    "Dark Matter Pool",
    "Thermocline Drift",
    "Bathypelagic Shimmer",
    "Hadal Reverb",
    "Beyond The Shelf",
]

# ---------------------------------------------------------------------------
# Engine definitions
# ---------------------------------------------------------------------------

INTIMATE_ENGINES = ["ONSET", "OBLONG", "OVERBITE", "OBESE", "OVERDUB",
                    "OVERWORLD", "OPTIC", "ORBITAL", "OTTONI"]

VAST_ENGINES = ["OPAL", "ORACLE", "OBSIDIAN", "ORGANON", "OCEANIC",
                "OSPREY", "OMBRE", "OHM", "ORPHICA"]

# Canonical engine key as used in the "engines" array and "parameters" dict
ENGINE_KEY = {
    "ONSET":     "Onset",
    "OBLONG":    "Oblong",
    "OVERBITE":  "Overbite",
    "OBESE":     "Obese",
    "OVERDUB":   "Overdub",
    "OVERWORLD": "Overworld",
    "OPTIC":     "Optic",
    "ORBITAL":   "Orbital",
    "OTTONI":    "Ottoni",
    "OPAL":      "Opal",
    "ORACLE":    "Oracle",
    "OBSIDIAN":  "Obsidian",
    "ORGANON":   "Organon",
    "OCEANIC":   "Oceanic",
    "OSPREY":    "Osprey",
    "OMBRE":     "Ombre",
    "OHM":       "Ohm",
    "ORPHICA":   "Orphica",
}

# Macro labels per engine (position 3 is SPACE / space-equivalent)
MACRO_LABELS = {
    "ONSET":     ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "OBLONG":    ["ACID SWEEP", "RESONANCE", "DRIVE", "MOTION"],
    "OVERBITE":  ["BELLY", "BITE", "SCURRY", "TRASH"],
    "OBESE":     ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "OVERDUB":   ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "OVERWORLD": ["ERA", "CIRCUIT", "GLITCH", "SPACE"],
    "OPTIC":     ["REACTIVITY", "TEMPO", "EVOLVE", "SPACE"],
    "ORBITAL":   ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "OTTONI":    ["EMBOUCHURE", "GROW", "FOREIGN", "LAKE"],
    "OPAL":      ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "ORACLE":    ["PROPHECY", "EVOLUTION", "GRAVITY", "DRIFT"],
    "OBSIDIAN":  ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "ORGANON":   ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "OCEANIC":   ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "OSPREY":    ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "OMBRE":     ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "OHM":       ["MEDDLING", "COMMUNE", "COUPLING", "SPACE"],
    "ORPHICA":   ["PLUCK", "FRACTURE", "SURFACE", "DIVINE"],
}

# ---------------------------------------------------------------------------
# Parameter generators per engine
# Parameters are intentionally sparse — only the most load-bearing controls.
# The space macro param (where it exists) is set to match the space DNA value.
# ---------------------------------------------------------------------------

def _rng(lo, hi):
    """Uniform random float in [lo, hi], rounded to 3dp."""
    return round(random.uniform(lo, hi), 3)

def _ri(lo, hi):
    """Uniform random int in [lo, hi]."""
    return random.randint(lo, hi)

def _choice(seq):
    return random.choice(seq)


def params_for_engine(engine_id: str, space: float, brightness: float,
                      warmth: float, movement: float, density: float,
                      aggression: float) -> dict:
    """
    Return a parameters dict for the given engine tuned to the provided DNA.
    space is the primary driver (0.0–0.10 for intimate, 0.90–1.0 for vast).
    """
    intimate = space < 0.5

    # Shared helpers derived from DNA
    cutoff_hz_dry   = int(200 + brightness * 8000)
    cutoff_hz_bright = int(1000 + brightness * 14000)
    cutoff_norm     = round(0.1 + brightness * 0.8, 3)
    reso            = round(0.05 + aggression * 0.45, 3)
    atk_fast        = round(0.001 + (1 - aggression) * 0.04, 4)
    atk_slow        = round(0.05 + (1 - aggression) * 0.8, 3)
    decay_short     = round(0.05 + density * 0.4, 3)
    decay_long      = round(0.5 + (1 - density) * 3.0, 2)
    sustain         = round(0.3 + warmth * 0.5, 3)
    release_tight   = round(0.05 + (1 - aggression) * 0.3, 3)
    release_wide    = round(0.8 + (1 - aggression) * 3.0, 2)
    lfo_rate_slow   = round(0.05 + movement * 0.3, 3)
    lfo_rate_fast   = round(0.2 + movement * 2.0, 3)
    lfo_depth       = round(movement * 0.4, 3)
    reverb_mix_dry  = round(space * 0.12, 3)   # max 0.012 at space=0.10
    reverb_mix_vast = round(0.55 + space * 0.4, 3)  # 0.95–1.0 range

    e = engine_id

    if e == "ONSET":
        # 8 percussion voices; dry = tight snappy
        p = {}
        voice_decays   = [0.08, 0.12, 0.06, 0.18, 0.10, 0.14, 0.07, 0.09]
        voice_pitches  = [0.0, 0.3, -2.0, 0.5, 1.0, -1.0, 0.2, -0.5]
        voice_levels   = [0.9, 0.8, 0.75, 0.85, 0.7, 0.8, 0.6, 0.65]
        voice_snaps    = [0.7+aggression*0.3, 0.6, 0.8, 0.5, 0.7, 0.6, 0.75, 0.55]
        voice_tones    = [brightness*0.9, brightness*0.7, brightness*0.5,
                          brightness*0.8, brightness*0.6, brightness*0.7,
                          brightness*0.4, brightness*0.5]
        voice_modes    = [1, 0, 2, 1, 0, 3, 0, 1]
        for v in range(1, 9):
            idx = v - 1
            d_val = round(voice_decays[idx] * (0.5 + aggression * 0.5), 3) if intimate else round(voice_decays[idx] * 2.5, 3)
            p[f"perc_v{v}_blend"]     = round(0.6 + density * 0.35, 3)
            p[f"perc_v{v}_algoMode"]  = voice_modes[idx]
            p[f"perc_v{v}_pitch"]     = round(voice_pitches[idx], 2)
            p[f"perc_v{v}_decay"]     = round(d_val, 3)
            p[f"perc_v{v}_tone"]      = round(max(0.05, min(0.95, voice_tones[idx])), 3)
            p[f"perc_v{v}_snap"]      = round(max(0.1, min(0.95, voice_snaps[idx])), 3)
            p[f"perc_v{v}_body"]      = round(0.4 + warmth * 0.5, 3)
            p[f"perc_v{v}_character"] = round(aggression * 0.8, 3)
            p[f"perc_v{v}_level"]     = round(voice_levels[idx], 2)
            p[f"perc_v{v}_pan"]       = round(random.uniform(-0.3, 0.3) if not intimate else 0.0, 2)
        p["perc_level"]      = 0.9 if intimate else 0.75
        p["perc_drive"]      = round(0.1 + aggression * 0.5, 3) if intimate else round(aggression * 0.2, 3)
        p["perc_masterTone"] = round(brightness * 0.85, 3)
        return p

    if e == "OBLONG":
        osc_waves = [0, 1, 2, 3]
        return {
            "bob_oscA_wave":    _choice(osc_waves),
            "bob_oscA_shape":   round(brightness * 0.8, 3),
            "bob_oscA_tune":    _choice([-24, -12, 0, 0, 0, 7, 12]),
            "bob_oscA_drift":   round(0.02 + movement * 0.12, 3),
            "bob_oscB_wave":    _choice(osc_waves),
            "bob_oscB_detune":  round(2.0 + density * 14.0, 1),
            "bob_oscB_blend":   round(0.2 + density * 0.6, 3),
            "bob_oscB_sync":    0,
            "bob_oscB_fm":      round(aggression * 0.3, 3),
            "bob_texMode":      _ri(0, 2),
            "bob_texLevel":     round(density * 0.5, 3) if not intimate else 0.0,
            "bob_texTone":      round(brightness * 0.6, 3),
            "bob_fltMode":      _choice([0, 1, 3]),
            "bob_fltCutoff":    float(cutoff_hz_dry) if intimate else float(cutoff_hz_bright),
            "bob_fltReso":      reso,
            "bob_ampAttack":    atk_fast if intimate else atk_slow,
            "bob_ampDecay":     decay_short if intimate else decay_long,
            "bob_ampSustain":   sustain,
            "bob_ampRelease":   release_tight if intimate else release_wide,
            "bob_spaceMix":     reverb_mix_dry if intimate else reverb_mix_vast,
            "bob_spaceSize":    round(space * 0.9, 3),
            "bob_bobMode":      round(warmth * 0.4, 3),
        }

    if e == "OVERBITE":
        return {
            "poss_oscAWaveform":      _choice([0, 1, 2, 3]),
            "poss_oscAShape":         round(brightness * 0.7, 3),
            "poss_oscADrift":         round(0.02 + movement * 0.08, 3),
            "poss_oscMix":            round(0.3 + density * 0.5, 3),
            "poss_oscBWaveform":      _choice([0, 2, 4]),
            "poss_oscBShape":         round(0.2 + brightness * 0.6, 3),
            "poss_oscBInstability":   round(aggression * 0.25, 3),
            "poss_oscInteractMode":   _choice([0, 1, 4]),
            "poss_oscInteractAmount": round(aggression * 0.5, 3),
            "poss_subLevel":          round(0.2 + warmth * 0.5, 3),
            "poss_subOctave":         _choice([0, -1]),
            "poss_weightShape":       _choice([0, 1]),
            "poss_filterCutoff":      float(cutoff_hz_dry) if intimate else float(cutoff_hz_bright),
            "poss_filterReso":        reso,
            "poss_ampAttack":         atk_fast if intimate else atk_slow,
            "poss_ampDecay":          decay_short if intimate else decay_long,
            "poss_ampSustain":        sustain,
            "poss_ampRelease":        release_tight if intimate else release_wide,
            "poss_fxSpaceType":       0 if intimate else _choice([1, 2, 3]),
            "poss_fxSpaceMix":        reverb_mix_dry if intimate else reverb_mix_vast,
            "poss_fxSpaceSize":       round(space * 0.9, 3),
            "poss_fxSpaceDecay":      round(0.05 + space * 0.8, 3),
            "poss_fxSpaceDamping":    round(0.7 - space * 0.5, 3),
        }

    if e == "OBESE":
        return {
            "fat_morph":       round(0.1 + aggression * 0.5, 3),
            "fat_mojo":        round(warmth * 0.7, 3),
            "fat_subLevel":    round(0.4 + warmth * 0.4, 3),
            "fat_subOct":      _choice([0, -1]),
            "fat_groupMix":    round(0.4 + density * 0.4, 3),
            "fat_detune":      round(density * 12.0, 1),
            "fat_stereoWidth": round(space * 0.5, 3),   # near 0 when intimate
            "fat_ampAttack":   atk_fast if intimate else atk_slow,
            "fat_ampDecay":    decay_short if intimate else decay_long,
            "fat_ampSustain":  sustain,
            "fat_ampRelease":  release_tight if intimate else release_wide,
            "fat_fltCutoff":   float(cutoff_hz_dry) if intimate else float(cutoff_hz_bright),
            "fat_fltReso":     reso,
            "fat_fltDrive":    round(0.1 + aggression * 0.6, 3),
            "fat_fltKeyTrack": round(0.3 + brightness * 0.5, 3),
            "fat_fltEnvAmt":   round(0.1 + aggression * 0.5, 3),
            "fat_satDrive":    round(aggression * 0.4, 3),
            "fat_crushDepth":  8 if aggression > 0.6 else 0,
            "fat_level":       0.85,
            "fat_voiceMode":   0,
            "fat_glide":       round(movement * 0.06, 3),
            "fat_polyphony":   _ri(1, 4),
        }

    if e == "OVERDUB":
        return {
            "dub_oscWave":        _choice([0, 1, 2]),
            "dub_filterCutoff":   float(cutoff_hz_dry) if intimate else float(cutoff_hz_bright),
            "dub_filterReso":     reso,
            "dub_delayTime":      round(0.05 + movement * 0.15, 3) if intimate else round(0.25 + movement * 0.55, 3),
            "dub_delayFeedback":  round(0.1 + density * 0.25, 3) if intimate else round(0.4 + density * 0.45, 3),
            "dub_delayWow":       round(movement * 0.08, 3),
            "dub_delayWear":      round(warmth * 0.4, 3),
            "dub_sendLevel":      reverb_mix_dry if intimate else reverb_mix_vast,
            "dub_returnLevel":    reverb_mix_dry if intimate else reverb_mix_vast,
            "dub_driveAmount":    round(aggression * 0.5, 3),
        }

    if e == "OVERWORLD":
        return {
            "ow_era":          round(brightness * 0.7, 3),
            "ow_eraY":         round(0.3 + warmth * 0.6, 3),
            "ow_voiceMode":    _choice([0, 1, 2]),
            "ow_masterVol":    0.8,
            "ow_pulseDuty":    _choice([0, 1, 2, 3]),
            "ow_pulseSweep":   round(movement * 0.3, 3),
            "ow_triEnable":    _choice([0, 1]),
            "ow_noiseMode":    0 if intimate else _choice([0, 1, 2]),
            "ow_noisePeriod":  _ri(2, 8),
            "ow_dpcmEnable":   0,
            "ow_macroEra":     round(brightness, 3),
            "ow_macroCrush":   round(aggression * 0.5, 3),
            "ow_macroGlitch":  round(aggression * 0.3, 3) if not intimate else 0.0,
            "ow_macroSpace":   round(space, 3),
        }

    if e == "OPTIC":
        return {
            "optic_reactivity":   round(0.5 + aggression * 0.4, 3),
            "optic_inputGain":    round(0.7 + brightness * 0.25, 3),
            "optic_autoPulse":    1,
            "optic_pulseRate":    round(0.2 + movement * 0.7, 3),
            "optic_pulseShape":   round(brightness * 0.5, 3),
            "optic_pulseSwing":   round(movement * 0.4, 3),
            "optic_pulseEvolve":  round(0.3 + movement * 0.6, 3),
            "optic_pulseSubdiv":  round(0.3 + density * 0.6, 3),
            "optic_pulseAccent":  round(0.4 + aggression * 0.5, 3),
            "optic_modDepth":     round(0.3 + density * 0.5, 3),
            "optic_modMixPulse":  round(0.4 + brightness * 0.4, 3),
            "optic_modMixSpec":   round(density * 0.5, 3),
            "optic_space":        round(space, 3),
        }

    if e == "ORBITAL":
        return {
            "orb_brightness":    round(brightness, 3),
            "orb_groupEnvRate":  round(0.2 + movement * 0.7, 3),
            "orb_groupEnvDepth": round(0.2 + density * 0.6, 3),
            "orb_oscCount":      _ri(2, 8) if intimate else _ri(6, 16),
            "orb_spread":        round(space * 0.5, 3),
            "orb_detune":        round(0.05 + density * 0.4, 3),
            "orb_filterCutoff":  cutoff_norm,
            "orb_filterReso":    reso,
            "orb_ampAttack":     atk_fast if intimate else atk_slow,
            "orb_ampDecay":      decay_short if intimate else decay_long,
            "orb_ampSustain":    sustain,
            "orb_ampRelease":    release_tight if intimate else release_wide,
            "orb_macroCharacter": round(aggression, 3),
            "orb_macroMovement":  round(movement, 3),
            "orb_macroCoupling":  0.0,
            "orb_macroSpace":     round(space, 3),
        }

    if e == "OTTONI":
        return {
            "otto_toddlerLevel":      round(0.3 + warmth * 0.4, 3),
            "otto_toddlerPressure":   round(0.2 + aggression * 0.4, 3),
            "otto_toddlerInst":       _ri(0, 3),
            "otto_tweenLevel":        round(0.25 + density * 0.45, 3),
            "otto_tweenEmbouchure":   round(0.3 + brightness * 0.4, 3),
            "otto_tweenValve":        round(0.3 + movement * 0.5, 3),
            "otto_tweenInst":         _ri(0, 7),
            "otto_teenLevel":         round(0.4 + aggression * 0.3, 3),
            "otto_teenEmbouchure":    round(0.5 + brightness * 0.35, 3),
            "otto_teenBore":          round(0.3 + warmth * 0.4, 3),
            "otto_teenInst":          _ri(0, 9),
            "otto_teenVibratoRate":   round(0.5 + movement * 0.8, 2),
            "otto_macroEmbouchure":   round(brightness, 3),
            "otto_macroGrow":         round(density, 3),
            "otto_macroForeign":      round(aggression * 0.5, 3),
            "otto_macroLake":         round(space, 3),
        }

    if e == "OPAL":
        return {
            "opal_grainSize":   round(0.01 + (1 - density) * 0.12, 4) if not intimate else round(0.05 + density * 0.1, 3),
            "opal_density":     round(0.4 + density * 0.55, 3),
            "opal_posScatter":  round(0.3 + space * 0.65, 3),
            "opal_pitchScatter":round(0.1 + movement * 0.6, 3),
            "opal_filterCutoff":round(0.3 + brightness * 0.55, 3),
            "opal_filterReso":  reso,
            "opal_ampAttack":   round(0.05 + space * 0.4, 3),
            "opal_ampDecay":    round(0.2 + space * 0.8, 3),
            "opal_ampSustain":  sustain,
            "opal_ampRelease":  round(0.5 + space * 3.5, 2),
            "opal_speed":       round(movement * 0.3, 3),
            "opal_reverse":     _choice([0.0, 1.0]),
            "opal_macroCharacter": round(warmth, 3),
            "opal_macroMovement":  round(movement, 3),
            "opal_macroCoupling":  0.0,
            "opal_macroSpace":     round(space, 3),
        }

    if e == "ORACLE":
        return {
            "oracle_breakpoints":       _ri(8, 32),
            "oracle_timeStep":          round(0.3 + movement * 0.5, 3),
            "oracle_ampStep":           round(0.2 + density * 0.5, 3),
            "oracle_distribution":      round(0.05 + aggression * 0.3, 3),
            "oracle_barrierElasticity": round(0.1 + space * 0.7, 3),
            "oracle_maqam":             _ri(0, 6),
            "oracle_gravity":           round(0.1 + space * 0.65, 3),
            "oracle_drift":             round(0.2 + space * 0.65, 3),
            "oracle_level":             0.75,
            "oracle_ampAttack":         round(0.05 + space * 0.8, 3),
            "oracle_ampDecay":          round(0.4 + space * 1.5, 2),
            "oracle_ampSustain":        sustain,
            "oracle_ampRelease":        round(0.8 + space * 4.0, 2),
            "oracle_macroProphecy":     round(brightness, 3),
            "oracle_macroEvolution":    round(movement, 3),
            "oracle_macroGravity":      round(aggression, 3),
            "oracle_macroDrift":        round(space, 3),
        }

    if e == "OBSIDIAN":
        return {
            "obsidian_densityX":        round(density, 3),
            "obsidian_tiltY":           round(0.3 + brightness * 0.5, 3),
            "obsidian_depth":           round(0.3 + space * 0.6, 3),
            "obsidian_stiffness":       round(0.05 + (1 - space) * 0.4, 3),
            "obsidian_cascadeBlend":    round(0.1 + space * 0.7, 3),
            "obsidian_crossModDepth":   round(aggression * 0.5, 3),
            "obsidian_formantIntensity":round(0.2 + warmth * 0.6, 3),
            "obsidian_stereoWidth":     round(0.3 + space * 0.65, 3),
            "obsidian_filterCutoff":    float(cutoff_hz_bright),
            "obsidian_filterReso":      reso,
            "obsidian_level":           0.78,
            "obsidian_ampAttack":       round(0.05 + space * 0.5, 3),
            "obsidian_ampRelease":      round(0.5 + space * 3.5, 2),
            "obsidian_macroCharacter":  round(warmth, 3),
            "obsidian_macroMovement":   round(movement, 3),
            "obsidian_macroCoupling":   0.0,
            "obsidian_macroSpace":      round(space, 3),
        }

    if e == "ORGANON":
        return {
            "organon_metabolicRate":  round(0.3 + movement * 0.6, 3),
            "organon_enzymeSelect":   float(int(200 + brightness * 1600)),
            "organon_catalystDrive":  round(0.2 + aggression * 0.6, 3),
            "organon_dampingCoeff":   round(0.05 + (1 - space) * 0.5, 3),
            "organon_signalFlux":     round(0.3 + density * 0.5, 3),
            "organon_phasonShift":    round(0.2 + movement * 0.6, 3),
            "organon_isotopeBalance": round(0.2 + warmth * 0.5, 3),
            "organon_lockIn":         round(0.3 + density * 0.5, 3),
            "organon_membrane":       round(0.1 + space * 0.7, 3),
            "organon_noiseColor":     round(0.1 + aggression * 0.6, 3),
            "organon_macroCharacter": round(warmth, 3),
            "organon_macroMovement":  round(movement, 3),
            "organon_macroCoupling":  0.0,
            "organon_macroSpace":     round(space, 3),
        }

    if e == "OCEANIC":
        return {
            "ocean_separation": round(0.1 + (1 - density) * 0.5, 3),
            "ocean_alignment":  round(0.3 + density * 0.4, 3),
            "ocean_cohesion":   round(0.3 + warmth * 0.4, 3),
            "ocean_tether":     round(0.2 + (1 - space) * 0.6, 3),
            "ocean_scatter":    round(0.05 + space * 0.6, 3),
            "ocean_subflocks":  _ri(2, 6),
            "ocean_damping":    round(0.1 + (1 - space) * 0.5, 3),
            "ocean_waveform":   _choice([0, 1, 2]),
            "ocean_ampAttack":  round(0.02 + space * 0.5, 3),
            "ocean_ampDecay":   round(0.2 + space * 1.0, 3),
            "ocean_ampSustain": sustain,
            "ocean_ampRelease": round(0.3 + space * 4.0, 2),
            "ocean_macroCharacter": round(brightness, 3),
            "ocean_macroMovement":  round(movement, 3),
            "ocean_macroCoupling":  0.0,
            "ocean_macroSpace":     round(space, 3),
        }

    if e == "OSPREY":
        return {
            "osprey_shore":           round(1.0 + space * 3.0, 1),
            "osprey_seaState":        round(0.2 + space * 0.6, 3),
            "osprey_swellPeriod":     round(3.0 + space * 8.0, 1),
            "osprey_windDir":         round(0.3 + movement * 0.5, 3),
            "osprey_depth":           round(0.2 + space * 0.7, 3),
            "osprey_resonatorBright": round(brightness * 0.8, 3),
            "osprey_resonatorDecay":  round(0.3 + space * 0.65, 3),
            "osprey_sympathyAmount":  round(0.1 + space * 0.6, 3),
            "osprey_creatureRate":    round(0.2 + movement * 0.6, 3),
            "osprey_creatureDepth":   round(0.1 + density * 0.5, 3),
            "osprey_coherence":       round(0.3 + (1 - space) * 0.4, 3),
            "osprey_foam":            round(aggression * 0.4, 3),
            "osprey_macroCharacter":  round(warmth, 3),
            "osprey_macroMovement":   round(movement, 3),
            "osprey_macroCoupling":   0.0,
            "osprey_macroSpace":      round(space, 3),
        }

    if e == "OMBRE":
        return {
            "ombre_blend":        round(0.3 + space * 0.5, 3),
            "ombre_interference": round(0.3 + space * 0.6, 3),
            "ombre_memoryDecay":  round(0.3 + space * 0.6, 3),
            "ombre_memoryGrain":  round(5.0 + space * 25.0, 1),
            "ombre_memoryDrift":  round(0.2 + space * 0.75, 3),
            "ombre_oscShape":     round(brightness * 0.8, 3),
            "ombre_reactivity":   round(0.4 + movement * 0.5, 3),
            "ombre_subLevel":     round(0.1 + warmth * 0.4, 3),
            "ombre_filterCutoff": float(cutoff_hz_bright),
            "ombre_filterReso":   reso,
            "ombre_attack":       round(0.001 + space * 0.5, 4),
            "ombre_decay":        round(0.05 + space * 0.8, 3),
            "ombre_macroCharacter": round(brightness, 3),
            "ombre_macroMovement":  round(movement, 3),
            "ombre_macroCoupling":  0.0,
            "ombre_macroSpace":     round(space, 3),
        }

    if e == "OHM":
        return {
            "ohm_dadInstrument":   _ri(0, 5),
            "ohm_dadLevel":        round(0.3 + warmth * 0.5, 3),
            "ohm_pluckBrightness": round(brightness * 0.8, 3),
            "ohm_bowPressure":     0.0 if intimate else round(space * 0.6, 3),
            "ohm_bowSpeed":        round(0.3 + movement * 0.4, 3),
            "ohm_bodyMaterial":    _ri(0, 3),
            "ohm_sympatheticAmt":  round(0.1 + space * 0.7, 3),
            "ohm_driftRate":       round(0.02 + movement * 0.05, 3),
            "ohm_driftDepth":      round(0.3 + movement * 0.6, 3),
            "ohm_damping":         round(0.6 + (1 - space) * 0.35, 3),
            "ohm_inlawLevel":      round(0.1 + density * 0.6, 3),
            "ohm_spectralFreeze":  round(space * 0.6, 3),
            "ohm_grainSize":       round(0.2 + space * 0.5, 3),
            "ohm_grainDensity":    round(0.4 + density * 0.5, 3),
            "ohm_grainScatter":    round(0.1 + space * 0.5, 3),
            "ohm_reverbMix":       reverb_mix_dry if intimate else reverb_mix_vast,
            "ohm_delayTime":       round(0.05 + space * 0.35, 3),
            "ohm_delayFeedback":   round(0.1 + space * 0.5, 3),
            "ohm_macroMeddling":   round(aggression, 3),
            "ohm_macroCommune":    round(density, 3),
            "ohm_macroCoupling":   0.0,
            "ohm_macroSpace":      round(space, 3),
        }

    if e == "ORPHICA":
        return {
            "orph_stringMaterial":  _ri(0, 3),
            "orph_pluckBrightness": round(brightness * 0.9, 3),
            "orph_pluckPosition":   round(0.2 + brightness * 0.6, 3),
            "orph_stringCount":     _ri(1, 4) if intimate else _ri(4, 8),
            "orph_bodySize":        round(0.3 + space * 0.6, 3),
            "orph_sympatheticAmt":  round(0.05 + space * 0.7, 3),
            "orph_damping":         round(0.7 + (1 - space) * 0.28, 3),
            "orph_driftRate":       round(0.02 + movement * 0.1, 3),
            "orph_driftDepth":      round(0.3 + space * 0.6, 3),
            "orph_microMode":       0 if intimate else _ri(0, 2),
            "orph_microRate":       round(0.5 + movement * 0.8, 2),
            "orph_microSize":       round(0.3 + space * 0.65, 3),
            "orph_macroPluck":      round(brightness, 3),
            "orph_macroFracture":   round(aggression, 3),
            "orph_macroSurface":    round(warmth, 3),
            "orph_macroDivine":     round(space, 3),
        }

    # Fallback: empty params (engine uses all defaults)
    return {}


# ---------------------------------------------------------------------------
# Preset builder
# ---------------------------------------------------------------------------

def build_preset(name: str, engine_id: str, space: float,
                 brightness: float, warmth: float, movement: float,
                 density: float, aggression: float,
                 mood: str, tags: list) -> dict:

    engine_key = ENGINE_KEY[engine_id]
    params = params_for_engine(engine_id, space, brightness, warmth,
                               movement, density, aggression)

    macro_labels = MACRO_LABELS[engine_id]

    # Build description
    if mood == "Foundation":
        desc_fragments = [
            f"Close-miked {engine_key.lower()} presence.",
            f"Dry and immediate — space locked at {space:.2f}.",
            "No reverb needed. The room is your ears.",
        ]
    else:
        desc_fragments = [
            f"{engine_key} dissolved into infinite space.",
            f"Space dimension at {space:.2f} — cavernous and unbound.",
            "The sound arrives from everywhere and nowhere.",
        ]
    description = " ".join(desc_fragments)

    return {
        "schema_version": 1,
        "name": name,
        "mood": mood,
        "engines": [engine_key],
        "author": "XO_OX",
        "version": "1.0",
        "description": description,
        "tags": tags,
        "macroLabels": macro_labels,
        "couplingIntensity": "None",
        "tempo": None,
        "created": "2026-03-16",
        "dna": {
            "brightness": round(brightness, 3),
            "warmth":     round(warmth, 3),
            "movement":   round(movement, 3),
            "density":    round(density, 3),
            "space":      round(space, 3),
            "aggression": round(aggression, 3),
        },
        "parameters": {engine_key: params} if params else {},
        "coupling": None,
        "sequencer": None,
    }


# ---------------------------------------------------------------------------
# Main generation logic
# ---------------------------------------------------------------------------

PRESET_BASE = Path("/Users/joshuacramblet/Documents/GitHub/XO_OX-XOlokun/Presets/XOlokun")

BAND_CONFIG = {
    "intimate": {
        "mood":         "Foundation",
        "out_dir":      PRESET_BASE / "Foundation",
        "engines":      INTIMATE_ENGINES,
        "names":        INTIMATE_NAMES,
        "space_range":  (0.0, 0.10),
        "tags":         ["foundation", "intimate", "dry", "close", "present"],
        # DNA constraints: intimate = high density/aggression, low space
        "brightness_range":  (0.25, 0.75),
        "warmth_range":      (0.30, 0.75),
        "movement_range":    (0.10, 0.65),
        "density_range":     (0.55, 0.90),
        "aggression_range":  (0.30, 0.75),
    },
    "vast": {
        "mood":         "Aether",
        "out_dir":      PRESET_BASE / "Aether",
        "engines":      VAST_ENGINES,
        "names":        VAST_NAMES,
        "space_range":  (0.90, 1.00),
        "tags":         ["aether", "vast", "reverb", "spatial", "infinite", "cosmic"],
        # DNA constraints: vast = low aggression, high space
        "brightness_range":  (0.15, 0.65),
        "warmth_range":      (0.25, 0.70),
        "movement_range":    (0.20, 0.75),
        "density_range":     (0.30, 0.75),
        "aggression_range":  (0.05, 0.40),
    },
}


def existing_names(directory: Path) -> set:
    names = set()
    for f in directory.glob("*.xometa"):
        try:
            d = json.loads(f.read_text())
            names.add(d.get("name", "").lower().strip())
        except Exception:
            names.add(f.stem.lower().strip())
    return names


def sanitize_filename(name: str) -> str:
    """Convert preset name to a safe filename."""
    safe = name.replace("/", "-").replace("\\", "-")
    safe = "".join(c for c in safe if c.isalnum() or c in " _-.()")
    return safe.strip()


def generate_band(band: str, count: int, dry_run: bool, seed: int) -> int:
    cfg = BAND_CONFIG[band]
    out_dir: Path = cfg["out_dir"]
    out_dir.mkdir(parents=True, exist_ok=True)

    taken = existing_names(out_dir)
    engines = cfg["engines"]
    names = list(cfg["names"])
    random.shuffle(names)

    written = 0
    name_idx = 0

    for i in range(count):
        engine_id = engines[i % len(engines)]

        # Pick a name not already in the folder
        while name_idx < len(names) and names[name_idx].lower().strip() in taken:
            name_idx += 1
        if name_idx >= len(names):
            print(f"  [warn] Exhausted name list for {band} at preset {i+1}; skipping remainder.")
            break

        preset_name = names[name_idx]
        name_idx += 1

        # Sample DNA
        space      = _rng(*cfg["space_range"])
        brightness = _rng(*cfg["brightness_range"])
        warmth     = _rng(*cfg["warmth_range"])
        movement   = _rng(*cfg["movement_range"])
        density    = _rng(*cfg["density_range"])
        aggression = _rng(*cfg["aggression_range"])

        # Build extra engine-specific tags
        engine_tag = engine_id.lower()
        extra_tags = [engine_tag]
        if space < 0.05:
            extra_tags.append("anechoic")
        elif space > 0.97:
            extra_tags.append("infinite")

        tags = cfg["tags"] + extra_tags

        preset = build_preset(
            name=preset_name,
            engine_id=engine_id,
            space=space,
            brightness=brightness,
            warmth=warmth,
            movement=movement,
            density=density,
            aggression=aggression,
            mood=cfg["mood"],
            tags=tags,
        )

        filename = sanitize_filename(preset_name) + ".xometa"
        out_path = out_dir / filename

        if dry_run:
            print(f"  [dry-run] Would write: {out_path.name}  engine={engine_id}  space={space:.3f}")
        else:
            out_path.write_text(json.dumps(preset, indent=2))
            print(f"  Wrote: {out_path.name}  engine={engine_id}  space={space:.3f}")

        taken.add(preset_name.lower().strip())
        written += 1

    return written


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--band", choices=["intimate", "vast", "both"],
                        default="both",
                        help="Which space band to generate (default: both)")
    parser.add_argument("--count", type=int, default=25,
                        help="Number of presets per band (default: 25)")
    parser.add_argument("--seed", type=int, default=None,
                        help="Random seed for reproducibility")
    parser.add_argument("--dry-run", action="store_true",
                        help="Print what would be written without creating files")
    args = parser.parse_args()

    seed = args.seed if args.seed is not None else random.randint(0, 99999)
    random.seed(seed)
    print(f"Seed: {seed}")

    bands = ["intimate", "vast"] if args.band == "both" else [args.band]

    total = 0
    for band in bands:
        cfg = BAND_CONFIG[band]
        label = "DRY-RUN " if args.dry_run else ""
        print(f"\n{label}Generating {args.count} {band.upper()} presets → {cfg['out_dir'].name}/")
        n = generate_band(band=band, count=args.count,
                          dry_run=args.dry_run, seed=seed)
        total += n

    action = "Would generate" if args.dry_run else "Generated"
    print(f"\n{action} {total} presets total.")


if __name__ == "__main__":
    main()
