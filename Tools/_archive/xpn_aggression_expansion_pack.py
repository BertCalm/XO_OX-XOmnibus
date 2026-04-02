#!/usr/bin/env python3
"""
xpn_aggression_expansion_pack.py

Generates high-aggression preset stubs (.xometa) for XOceanus engines that are
naturally aggressive. Addresses the fleet DNA diversity problem: 27% of presets
cluster at aggression 0.0–0.1. This tool fills the 0.6–1.0 aggressive territory.

Outputs to Presets/XOceanus/Flux/ (aggressive presets belong in the Flux mood).

Usage:
    python3 Tools/xpn_aggression_expansion_pack.py
    python3 Tools/xpn_aggression_expansion_pack.py --dry-run
    python3 Tools/xpn_aggression_expansion_pack.py --engines Onset,Overworld --count-per-band 5
    python3 Tools/xpn_aggression_expansion_pack.py --output-dir /tmp/test_presets --seed 42
"""

import argparse
import json
import os
import random
import sys
from pathlib import Path

# ---------------------------------------------------------------------------
# Name vocabulary
# ---------------------------------------------------------------------------

AGGRESSION_WORDS = [
    "Gnash", "Grind", "Slash", "Bite", "Shred", "Rip", "Tear", "Crush",
    "Smash", "Detonate", "Razor", "Spike", "Serrate", "Fang", "Claw",
    "Puncture", "Impact", "Shatter", "Rupture", "Feral", "Savage", "Raw",
    "Brutal", "Stark",
]

AGGRESSION_SUFFIXES = [
    "Protocol", "Mode", "State", "Drive", "Force", "Wave", "Strike",
    "Engine", "Form", "Burst", "Pulse", "Surge", "Wall", "Edge",
    "Current", "Mass", "Front", "Charge", "Run", "Chain",
]

AGGRESSION_TAGS_BY_BAND = {
    "medium-high": ["grit", "driven", "aggressive", "musical", "approachable"],
    "high":        ["aggressive", "intense", "raw", "powerful", "beast"],
    "extreme":     ["extreme", "brutal", "savage", "full-beast", "unhinged"],
}

# ---------------------------------------------------------------------------
# Aggression bands
# ---------------------------------------------------------------------------

BANDS = {
    "medium-high": (0.60, 0.75),
    "high":        (0.75, 0.90),
    "extreme":     (0.90, 1.00),
}

# ---------------------------------------------------------------------------
# Engine definitions
# DNA profile columns: brightness_range, macro_labels, description_flavor
# Parameters are realistic stubs derived from actual engine schemas.
# ---------------------------------------------------------------------------

ENGINE_DEFS = {

    "Onset": {
        "display": "Onset",
        "engines_key": "Onset",
        "brightness_range": (0.55, 0.85),
        "macro_labels": ["MACHINE", "PUNCH", "SPACE", "MUTATE"],
        "description_flavor": "Percussive assault — MACHINE drives the synthesis algorithm, PUNCH shapes the transient impact.",
        "tags_extra": ["percussion", "drums", "impact"],
        "param_template": lambda rng: {
            "perc_v1_blend":     round(rng.uniform(0.5, 1.0), 3),
            "perc_v1_algoMode":  rng.randint(2, 5),
            "perc_v1_pitch":     round(rng.uniform(0.3, 0.8), 3),
            "perc_v1_decay":     round(rng.uniform(0.05, 0.35), 3),
            "perc_v1_tone":      round(rng.uniform(0.6, 1.0), 3),
            "perc_v1_snap":      round(rng.uniform(0.6, 1.0), 3),
            "perc_v1_body":      round(rng.uniform(0.4, 0.85), 3),
            "perc_v1_character": round(rng.uniform(0.5, 1.0), 3),
            "perc_v1_level":     round(rng.uniform(0.75, 1.0), 3),
            "perc_v1_pan":       round(rng.uniform(-0.1, 0.1), 3),
            "perc_v2_blend":     round(rng.uniform(0.4, 1.0), 3),
            "perc_v2_algoMode":  rng.randint(0, 5),
            "perc_v2_pitch":     round(rng.uniform(0.4, 0.9), 3),
            "perc_v2_decay":     round(rng.uniform(0.05, 0.25), 3),
            "perc_v2_tone":      round(rng.uniform(0.5, 1.0), 3),
            "perc_v2_snap":      round(rng.uniform(0.5, 1.0), 3),
            "perc_v2_body":      round(rng.uniform(0.3, 0.8), 3),
            "perc_v2_character": round(rng.uniform(0.4, 1.0), 3),
            "perc_v2_level":     round(rng.uniform(0.7, 1.0), 3),
            "perc_v2_pan":       round(rng.uniform(-0.15, 0.15), 3),
            "perc_v3_blend":     round(rng.uniform(0.3, 0.9), 3),
            "perc_v3_algoMode":  rng.randint(0, 5),
            "perc_v3_pitch":     round(rng.uniform(0.5, 1.0), 3),
            "perc_v3_decay":     round(rng.uniform(0.04, 0.2), 3),
            "perc_v3_tone":      round(rng.uniform(0.6, 1.0), 3),
            "perc_v3_snap":      round(rng.uniform(0.7, 1.0), 3),
            "perc_v3_body":      round(rng.uniform(0.4, 0.9), 3),
            "perc_v3_character": round(rng.uniform(0.5, 1.0), 3),
            "perc_v3_level":     round(rng.uniform(0.6, 0.95), 3),
            "perc_v3_pan":       round(rng.uniform(-0.2, 0.2), 3),
            "perc_v4_blend":     round(rng.uniform(0.2, 0.8), 3),
            "perc_v4_algoMode":  rng.randint(0, 5),
            "perc_v4_pitch":     round(rng.uniform(0.5, 0.95), 3),
            "perc_v4_decay":     round(rng.uniform(0.04, 0.18), 3),
            "perc_v4_tone":      round(rng.uniform(0.5, 1.0), 3),
            "perc_v4_snap":      round(rng.uniform(0.5, 1.0), 3),
            "perc_v4_body":      round(rng.uniform(0.3, 0.8), 3),
            "perc_v4_character": round(rng.uniform(0.4, 0.9), 3),
            "perc_v4_level":     round(rng.uniform(0.6, 0.9), 3),
            "perc_v4_pan":       round(rng.uniform(-0.25, 0.25), 3),
            "perc_v5_blend":     round(rng.uniform(0.1, 0.7), 3),
            "perc_v5_algoMode":  rng.randint(0, 5),
            "perc_v5_pitch":     round(rng.uniform(0.4, 0.9), 3),
            "perc_v5_decay":     round(rng.uniform(0.03, 0.15), 3),
            "perc_v5_tone":      round(rng.uniform(0.4, 0.9), 3),
            "perc_v5_snap":      round(rng.uniform(0.4, 0.9), 3),
            "perc_v5_body":      round(rng.uniform(0.2, 0.7), 3),
            "perc_v5_character": round(rng.uniform(0.3, 0.85), 3),
            "perc_v5_level":     round(rng.uniform(0.5, 0.85), 3),
            "perc_v5_pan":       round(rng.uniform(-0.3, 0.3), 3),
            "perc_v6_blend":     round(rng.uniform(0.1, 0.6), 3),
            "perc_v6_algoMode":  rng.randint(0, 5),
            "perc_v6_pitch":     round(rng.uniform(0.3, 0.8), 3),
            "perc_v6_decay":     round(rng.uniform(0.03, 0.12), 3),
            "perc_v6_tone":      round(rng.uniform(0.4, 0.9), 3),
            "perc_v6_snap":      round(rng.uniform(0.4, 0.9), 3),
            "perc_v6_body":      round(rng.uniform(0.2, 0.7), 3),
            "perc_v6_character": round(rng.uniform(0.3, 0.8), 3),
            "perc_v6_level":     round(rng.uniform(0.4, 0.8), 3),
            "perc_v6_pan":       round(rng.uniform(-0.35, 0.35), 3),
            "perc_v7_blend":     round(rng.uniform(0.1, 0.6), 3),
            "perc_v7_algoMode":  rng.randint(0, 5),
            "perc_v7_pitch":     round(rng.uniform(0.3, 0.75), 3),
            "perc_v7_decay":     round(rng.uniform(0.02, 0.1), 3),
            "perc_v7_tone":      round(rng.uniform(0.3, 0.85), 3),
            "perc_v7_snap":      round(rng.uniform(0.3, 0.85), 3),
            "perc_v7_body":      round(rng.uniform(0.2, 0.65), 3),
            "perc_v7_character": round(rng.uniform(0.3, 0.75), 3),
            "perc_v7_level":     round(rng.uniform(0.35, 0.75), 3),
            "perc_v7_pan":       round(rng.uniform(-0.4, 0.4), 3),
            "perc_v8_blend":     round(rng.uniform(0.1, 0.5), 3),
            "perc_v8_algoMode":  rng.randint(0, 5),
            "perc_v8_pitch":     round(rng.uniform(0.2, 0.7), 3),
            "perc_v8_decay":     round(rng.uniform(0.02, 0.1), 3),
            "perc_v8_tone":      round(rng.uniform(0.3, 0.8), 3),
            "perc_v8_snap":      round(rng.uniform(0.3, 0.8), 3),
            "perc_v8_body":      round(rng.uniform(0.15, 0.6), 3),
            "perc_v8_character": round(rng.uniform(0.2, 0.7), 3),
            "perc_v8_level":     round(rng.uniform(0.3, 0.7), 3),
            "perc_v8_pan":       round(rng.uniform(-0.45, 0.45), 3),
            "perc_level":        round(rng.uniform(0.75, 1.0), 3),
            "perc_drive":        round(rng.uniform(0.5, 1.0), 3),
            "perc_masterTone":   round(rng.uniform(0.5, 0.9), 3),
        },
    },

    "Obese": {
        "display": "Obese",
        "engines_key": "Obese",
        "brightness_range": (0.35, 0.65),
        "macro_labels": ["MORPH", "MOJO", "DRIVE", "CRUSH"],
        "description_flavor": "Sub-bass assault — MORPH sweeps voice character, MOJO controls the analog-fat saturation stack.",
        "tags_extra": ["bass", "sub", "fat", "saturation"],
        "param_template": lambda rng: {
            "fat_morph":       round(rng.uniform(0.5, 1.0), 3),
            "fat_mojo":        round(rng.uniform(0.6, 1.0), 3),
            "fat_subLevel":    round(rng.uniform(0.6, 1.0), 3),
            "fat_subOct":      rng.choice([-2, -1]),
            "fat_groupMix":    round(rng.uniform(0.4, 0.9), 3),
            "fat_detune":      round(rng.uniform(0.1, 0.5), 3),
            "fat_stereoWidth": round(rng.uniform(0.3, 0.8), 3),
            "fat_ampAttack":   round(rng.uniform(0.001, 0.01), 4),
            "fat_ampDecay":    round(rng.uniform(0.1, 0.4), 3),
            "fat_ampSustain":  round(rng.uniform(0.5, 0.9), 3),
            "fat_ampRelease":  round(rng.uniform(0.1, 0.4), 3),
            "fat_fltCutoff":   round(rng.uniform(0.4, 0.85), 3),
            "fat_fltReso":     round(rng.uniform(0.3, 0.7), 3),
            "fat_fltDrive":    round(rng.uniform(0.5, 1.0), 3),
            "fat_fltKeyTrack": round(rng.uniform(0.3, 0.8), 3),
            "fat_fltEnvAmt":   round(rng.uniform(0.4, 0.9), 3),
            "fat_fltEnvAttack": round(rng.uniform(0.001, 0.015), 4),
            "fat_fltEnvDecay": round(rng.uniform(0.08, 0.3), 3),
            "fat_satDrive":    round(rng.uniform(0.5, 1.0), 3),
            "fat_crushDepth":  round(rng.uniform(0.3, 0.8), 3),
            "fat_crushRate":   round(rng.uniform(0.3, 0.8), 3),
            "fat_arpOn":       0,
            "fat_arpPattern":  0,
            "fat_arpRate":     round(rng.uniform(0.3, 0.7), 3),
            "fat_arpOctaves":  rng.randint(1, 3),
            "fat_arpGate":     round(rng.uniform(0.5, 0.9), 3),
            "fat_arpTempo":    round(rng.uniform(0.4, 0.8), 3),
            "fat_level":       round(rng.uniform(0.75, 1.0), 3),
            "fat_voiceMode":   rng.randint(0, 2),
            "fat_glide":       round(rng.uniform(0.0, 0.15), 3),
        },
    },

    "Ouroboros": {
        "display": "Ouroboros",
        "engines_key": "Ouroboros",
        "brightness_range": (0.45, 0.75),
        "macro_labels": ["CHARACTER", "MOVEMENT", "CHAOS", "LEASH"],
        "description_flavor": "Lorenz attractor at high chaos — CHAOS index maxed, LEASH barely constraining the strange attractor.",
        "tags_extra": ["chaos", "lorenz", "unstable", "modular"],
        "param_template": lambda rng: {
            "ouro_topology":   rng.randint(0, 3),
            "ouro_rate":       round(rng.uniform(80.0, 440.0), 1),
            "ouro_chaosIndex": round(rng.uniform(0.65, 1.0), 3),
            "ouro_leash":      round(rng.uniform(0.0, 0.35), 3),
            "ouro_theta":      round(rng.uniform(0.3, 0.9), 3),
            "ouro_phi":        round(rng.uniform(0.2, 0.8), 3),
            "ouro_damping":    round(rng.uniform(0.05, 0.3), 3),
            "ouro_injection":  round(rng.uniform(0.3, 0.9), 3),
        },
    },

    "Overworld": {
        "display": "Overworld",
        "engines_key": "Overworld",
        "brightness_range": (0.55, 0.9),
        "macro_labels": ["ERA", "GLITCH", "CRUSH", "CHAOS"],
        "description_flavor": "Chip-era brutalism — bit-crushed NES pulse waves colliding with YM2612 FM distortion and SNES sample corruption.",
        "tags_extra": ["chiptune", "bitcrush", "glitch", "fm"],
        "param_template": lambda rng: {
            "ow_era":           round(rng.uniform(0.0, 1.0), 3),
            "ow_eraY":          round(rng.uniform(0.0, 1.0), 3),
            "ow_voiceMode":     rng.randint(0, 3),
            "ow_masterVol":     round(rng.uniform(0.75, 1.0), 3),
            "ow_masterTune":    0.0,
            "ow_pulseDuty":     round(rng.uniform(0.1, 0.5), 3),
            "ow_pulseSweep":    round(rng.uniform(0.3, 0.9), 3),
            "ow_triEnable":     rng.randint(0, 1),
            "ow_noiseMode":     rng.randint(0, 2),
            "ow_noisePeriod":   round(rng.uniform(0.3, 0.9), 3),
            "ow_dpcmEnable":    rng.randint(0, 1),
            "ow_dpcmRate":      round(rng.uniform(0.3, 0.9), 3),
            "ow_nesMix":        round(rng.uniform(0.4, 1.0), 3),
            "ow_fmAlgorithm":   rng.randint(0, 7),
            "ow_fmFeedback":    round(rng.uniform(0.4, 1.0), 3),
            "ow_fmOp1Level":    round(rng.uniform(0.5, 1.0), 3),
            "ow_fmOp2Level":    round(rng.uniform(0.4, 1.0), 3),
            "ow_fmOp3Level":    round(rng.uniform(0.3, 0.9), 3),
            "ow_fmOp4Level":    round(rng.uniform(0.2, 0.8), 3),
            "ow_fmOp1Mult":     rng.randint(1, 8),
            "ow_fmOp2Mult":     rng.randint(1, 8),
            "ow_fmOp3Mult":     rng.randint(1, 8),
            "ow_fmOp4Mult":     rng.randint(1, 8),
            "ow_fmOp1Detune":   round(rng.uniform(-0.1, 0.1), 3),
            "ow_fmOp2Detune":   round(rng.uniform(-0.15, 0.15), 3),
            "ow_fmOp3Detune":   round(rng.uniform(-0.2, 0.2), 3),
            "ow_fmOp4Detune":   round(rng.uniform(-0.2, 0.2), 3),
            "ow_fmAttack":      round(rng.uniform(0.0, 0.05), 3),
            "ow_fmDecay":       round(rng.uniform(0.05, 0.3), 3),
            "ow_fmSustain":     round(rng.uniform(0.4, 0.9), 3),
            "ow_fmRelease":     round(rng.uniform(0.05, 0.35), 3),
            "ow_fmLfoRate":     round(rng.uniform(0.3, 0.9), 3),
            "ow_fmLfoDepth":    round(rng.uniform(0.2, 0.7), 3),
            "ow_brrSample":     rng.randint(0, 7),
            "ow_brrInterp":     rng.randint(0, 3),
            "ow_snesAttack":    round(rng.uniform(0.0, 0.05), 3),
            "ow_snesDecay":     round(rng.uniform(0.05, 0.35), 3),
            "ow_snesSustain":   round(rng.uniform(0.5, 0.95), 3),
            "ow_snesRelease":   round(rng.uniform(0.05, 0.3), 3),
            "ow_pitchMod":      round(rng.uniform(0.3, 0.8), 3),
            "ow_noiseReplace":  round(rng.uniform(0.0, 0.5), 3),
            "ow_echoDelay":     round(rng.uniform(0.0, 0.3), 3),
            "ow_echoFeedback":  round(rng.uniform(0.0, 0.4), 3),
            "ow_echoMix":       round(rng.uniform(0.0, 0.25), 3),
            "ow_echoFir0":      round(rng.uniform(0.3, 0.9), 3),
            "ow_echoFir1":      round(rng.uniform(-0.3, 0.3), 3),
            "ow_echoFir2":      round(rng.uniform(-0.2, 0.2), 3),
            "ow_echoFir3":      round(rng.uniform(-0.15, 0.15), 3),
            "ow_echoFir4":      round(rng.uniform(-0.1, 0.1), 3),
            "ow_echoFir5":      round(rng.uniform(-0.1, 0.1), 3),
            "ow_echoFir6":      round(rng.uniform(-0.1, 0.1), 3),
            "ow_echoFir7":      round(rng.uniform(-0.05, 0.05), 3),
            "ow_glitchAmount":  round(rng.uniform(0.5, 1.0), 3),
            "ow_glitchType":    rng.randint(0, 12),
            "ow_glitchRate":    round(rng.uniform(0.4, 0.9), 3),
            "ow_glitchDepth":   round(rng.uniform(0.4, 0.9), 3),
            "ow_glitchMix":     round(rng.uniform(0.5, 1.0), 3),
            "ow_ampAttack":     round(rng.uniform(0.0, 0.02), 3),
            "ow_ampDecay":      round(rng.uniform(0.05, 0.3), 3),
            "ow_ampSustain":    round(rng.uniform(0.5, 0.95), 3),
            "ow_ampRelease":    round(rng.uniform(0.05, 0.3), 3),
            "ow_filterCutoff":  round(rng.uniform(0.4, 0.95), 3),
            "ow_filterReso":    round(rng.uniform(0.2, 0.7), 3),
            "ow_filterType":    rng.randint(0, 3),
            "ow_crushBits":     round(rng.uniform(0.3, 1.0), 3),
            "ow_crushRate":     round(rng.uniform(0.3, 1.0), 3),
            "ow_crushMix":      round(rng.uniform(0.3, 0.9), 3),
            "ow_eraDriftRate":  round(rng.uniform(0.3, 0.8), 3),
            "ow_eraDriftDepth": round(rng.uniform(0.3, 0.7), 3),
            "ow_eraDriftShape": rng.randint(0, 3),
            "ow_eraPortaTime":  round(rng.uniform(0.0, 0.15), 3),
            "ow_eraMemTime":    round(rng.uniform(0.0, 0.5), 3),
            "ow_eraMemMix":     round(rng.uniform(0.0, 0.4), 3),
            "ow_vertexA":       round(rng.uniform(0.0, 1.0), 3),
            "ow_vertexB":       round(rng.uniform(0.0, 1.0), 3),
            "ow_vertexC":       round(rng.uniform(0.0, 1.0), 3),
            "ow_colorTheme":    rng.randint(0, 14),
        },
    },

    "Oblique": {
        "display": "Oblique",
        "engines_key": "Oblique",
        "brightness_range": (0.55, 0.85),
        "macro_labels": ["FOLD", "BOUNCE", "PRISM", "PHASE"],
        "description_flavor": "Wavefold brutalism — FOLD drives waveform into complex harmonic distortion, BOUNCE fires percussive attacks.",
        "tags_extra": ["wavefold", "percussive", "harmonic", "prism"],
        "param_template": lambda rng: {
            "oblq_oscWave":       rng.randint(0, 5),
            "oblq_oscFold":       round(rng.uniform(0.5, 1.0), 3),
            "oblq_oscDetune":     round(rng.uniform(0.0, 0.3), 3),
            "oblq_level":         round(rng.uniform(0.7, 1.0), 3),
            "oblq_glide":         round(rng.uniform(0.0, 0.1), 3),
            "oblq_percClick":     round(rng.uniform(0.5, 1.0), 3),
            "oblq_percDecay":     round(rng.uniform(0.02, 0.15), 3),
            "oblq_bounceRate":    round(rng.uniform(0.4, 0.9), 3),
            "oblq_bounceGravity": round(rng.uniform(0.4, 0.9), 3),
            "oblq_bounceDamp":    round(rng.uniform(0.3, 0.7), 3),
            "oblq_bounceCnt":     rng.randint(2, 8),
            "oblq_bounceSwing":   round(rng.uniform(0.0, 0.5), 3),
            "oblq_clickTone":     round(rng.uniform(0.5, 1.0), 3),
            "oblq_filterCut":     round(rng.uniform(0.4, 0.9), 3),
            "oblq_filterRes":     round(rng.uniform(0.2, 0.7), 3),
            "oblq_attack":        round(rng.uniform(0.0, 0.02), 3),
            "oblq_decay":         round(rng.uniform(0.05, 0.4), 3),
            "oblq_sustain":       round(rng.uniform(0.5, 0.95), 3),
            "oblq_release":       round(rng.uniform(0.05, 0.3), 3),
            "oblq_prismDelay":    round(rng.uniform(0.1, 0.5), 3),
            "oblq_prismSpread":   round(rng.uniform(0.3, 0.8), 3),
            "oblq_prismColor":    round(rng.uniform(0.4, 0.9), 3),
            "oblq_prismWidth":    round(rng.uniform(0.3, 0.8), 3),
            "oblq_prismFeedback": round(rng.uniform(0.2, 0.7), 3),
            "oblq_prismMix":      round(rng.uniform(0.3, 0.8), 3),
            "oblq_prismDamp":     round(rng.uniform(0.2, 0.6), 3),
            "oblq_phaserRate":    round(rng.uniform(0.3, 0.9), 3),
            "oblq_phaserDepth":   round(rng.uniform(0.4, 0.9), 3),
            "oblq_phaserFeedback":round(rng.uniform(0.3, 0.8), 3),
            "oblq_phaserMix":     round(rng.uniform(0.3, 0.8), 3),
        },
    },

    "Ottoni": {
        "display": "Ottoni",
        "engines_key": "Ottoni",
        "brightness_range": (0.5, 0.8),
        "macro_labels": ["EMBOUCHURE", "GROW", "FOREIGN", "LAKE"],
        "description_flavor": "Triple brass at war — GROW pushes all three voices into overblown aggression, FOREIGN injects cold alien harmonics.",
        "tags_extra": ["brass", "wind", "physical-model", "overblown"],
        "param_template": lambda rng: {
            "otto_toddlerLevel":      round(rng.uniform(0.6, 1.0), 3),
            "otto_toddlerPressure":   round(rng.uniform(0.6, 1.0), 3),
            "otto_toddlerInst":       rng.randint(0, 4),
            "otto_tweenLevel":        round(rng.uniform(0.5, 1.0), 3),
            "otto_tweenEmbouchure":   round(rng.uniform(0.5, 1.0), 3),
            "otto_tweenValve":        round(rng.uniform(0.4, 0.9), 3),
            "otto_tweenInst":         rng.randint(0, 4),
            "otto_teenLevel":         round(rng.uniform(0.5, 0.95), 3),
            "otto_teenEmbouchure":    round(rng.uniform(0.5, 1.0), 3),
            "otto_teenBore":          round(rng.uniform(0.4, 0.9), 3),
            "otto_teenInst":          rng.randint(0, 4),
            "otto_teenVibratoRate":   round(rng.uniform(0.3, 0.8), 3),
            "otto_teenVibratoDepth":  round(rng.uniform(0.2, 0.6), 3),
            "otto_damping":           round(rng.uniform(0.2, 0.5), 3),
            "otto_sympatheticAmt":    round(rng.uniform(0.3, 0.7), 3),
            "otto_driftRate":         round(rng.uniform(0.3, 0.8), 3),
            "otto_driftDepth":        round(rng.uniform(0.2, 0.6), 3),
            "otto_foreignStretch":    round(rng.uniform(0.4, 0.9), 3),
            "otto_foreignDrift":      round(rng.uniform(0.3, 0.8), 3),
            "otto_foreignCold":       round(rng.uniform(0.4, 0.9), 3),
            "otto_reverbSize":        round(rng.uniform(0.1, 0.4), 3),
            "otto_chorusRate":        round(rng.uniform(0.3, 0.7), 3),
            "otto_driveAmount":       round(rng.uniform(0.5, 1.0), 3),
            "otto_delayMix":          round(rng.uniform(0.0, 0.25), 3),
            "otto_macroEmbouchure":   round(rng.uniform(0.5, 1.0), 3),
            "otto_macroGrow":         round(rng.uniform(0.6, 1.0), 3),
            "otto_macroForeign":      round(rng.uniform(0.4, 0.9), 3),
            "otto_macroLake":         round(rng.uniform(0.0, 0.4), 3),
        },
    },

    "Ole": {
        "display": "Ole",
        "engines_key": "Ole",
        "brightness_range": (0.5, 0.8),
        "macro_labels": ["FUEGO", "DRAMA", "SIDES", "ISLA"],
        "description_flavor": "Afro-Latin trio in full frenzy — FUEGO drives the strum aggression, DRAMA pushes the ensemble into theatrical chaos.",
        "tags_extra": ["latin", "afro", "ensemble", "strum", "chaos"],
        "param_template": lambda rng: {
            "ole_aunt1Level":      round(rng.uniform(0.6, 1.0), 3),
            "ole_aunt1StrumRate":  round(rng.uniform(0.6, 1.0), 3),
            "ole_aunt1Brightness": round(rng.uniform(0.5, 0.9), 3),
            "ole_aunt2Level":      round(rng.uniform(0.5, 0.95), 3),
            "ole_aunt2CoinPress":  round(rng.uniform(0.5, 1.0), 3),
            "ole_aunt2GourdSize":  round(rng.uniform(0.3, 0.8), 3),
            "ole_aunt3Level":      round(rng.uniform(0.5, 0.9), 3),
            "ole_aunt3Tremolo":    round(rng.uniform(0.5, 1.0), 3),
            "ole_aunt3Brightness": round(rng.uniform(0.5, 0.9), 3),
            "ole_damping":         round(rng.uniform(0.15, 0.4), 3),
            "ole_sympatheticAmt":  round(rng.uniform(0.3, 0.7), 3),
            "ole_driftRate":       round(rng.uniform(0.4, 0.9), 3),
            "ole_driftDepth":      round(rng.uniform(0.3, 0.7), 3),
            "ole_allianceConfig":  rng.randint(0, 3),
            "ole_allianceBlend":   round(rng.uniform(0.4, 0.9), 3),
            "ole_husbandOudLevel": round(rng.uniform(0.3, 0.7), 3),
            "ole_husbandBouzLevel":round(rng.uniform(0.3, 0.7), 3),
            "ole_husbandPinLevel": round(rng.uniform(0.2, 0.6), 3),
            "ole_macroFuego":      round(rng.uniform(0.6, 1.0), 3),
            "ole_macroDrama":      round(rng.uniform(0.5, 1.0), 3),
            "ole_macroSides":      round(rng.uniform(0.3, 0.8), 3),
            "ole_macroIsla":       round(rng.uniform(0.0, 0.4), 3),
        },
    },

    "Orca": {
        "display": "Orca",
        "engines_key": "Orca",
        "brightness_range": (0.5, 0.85),
        "macro_labels": ["HUNT", "BREACH", "CRUSH", "SPACE"],
        "description_flavor": "Apex predator wavetable — HUNT macro unleashes the full chase sequence, BREACH fires the sub-bass transient cannon.",
        "tags_extra": ["wavetable", "formant", "apex", "sub"],
        "param_template": lambda rng: {
            "orca_wtPosition":       round(rng.uniform(0.3, 1.0), 3),
            "orca_wtScanRate":       round(rng.uniform(0.4, 1.0), 3),
            "orca_formantIntensity": round(rng.uniform(0.5, 1.0), 3),
            "orca_formantShift":     round(rng.uniform(0.3, 0.8), 3),
            "orca_glide":            round(rng.uniform(0.0, 0.1), 3),
            "orca_echoRate":         round(rng.uniform(0.4, 0.9), 3),
            "orca_echoReso":         round(rng.uniform(0.4, 0.85), 3),
            "orca_echoDamp":         round(rng.uniform(0.3, 0.7), 3),
            "orca_echoMix":          round(rng.uniform(0.0, 0.3), 3),
            "orca_huntMacro":        round(rng.uniform(0.6, 1.0), 3),
            "orca_breachSub":        round(rng.uniform(0.5, 1.0), 3),
            "orca_breachShape":      round(rng.uniform(0.4, 0.9), 3),
            "orca_breachThreshold":  round(rng.uniform(0.3, 0.7), 3),
            "orca_breachRatio":      round(rng.uniform(0.5, 1.0), 3),
            "orca_crushBits":        round(rng.uniform(0.3, 0.8), 3),
            "orca_crushDownsample":  round(rng.uniform(0.3, 0.8), 3),
            "orca_crushMix":         round(rng.uniform(0.3, 0.8), 3),
            "orca_crushSplitFreq":   round(rng.uniform(0.3, 0.7), 3),
            "orca_filterCutoff":     round(rng.uniform(0.4, 0.9), 3),
            "orca_filterReso":       round(rng.uniform(0.3, 0.7), 3),
            "orca_level":            round(rng.uniform(0.75, 1.0), 3),
            "orca_ampAttack":        round(rng.uniform(0.0, 0.02), 3),
            "orca_ampDecay":         round(rng.uniform(0.05, 0.3), 3),
            "orca_ampSustain":       round(rng.uniform(0.5, 0.95), 3),
            "orca_ampRelease":       round(rng.uniform(0.05, 0.3), 3),
            "orca_modAttack":        round(rng.uniform(0.0, 0.05), 3),
            "orca_modDecay":         round(rng.uniform(0.05, 0.3), 3),
            "orca_modSustain":       round(rng.uniform(0.3, 0.8), 3),
            "orca_modRelease":       round(rng.uniform(0.05, 0.3), 3),
            "orca_lfo1Rate":         round(rng.uniform(0.3, 0.9), 3),
            "orca_lfo1Depth":        round(rng.uniform(0.3, 0.8), 3),
            "orca_lfo1Shape":        rng.randint(0, 4),
            "orca_lfo2Rate":         round(rng.uniform(0.2, 0.8), 3),
            "orca_lfo2Depth":        round(rng.uniform(0.2, 0.7), 3),
            "orca_lfo2Shape":        rng.randint(0, 4),
            "orca_polyphony":        rng.randint(1, 6),
            "orca_macroCharacter":   round(rng.uniform(0.5, 1.0), 3),
            "orca_macroMovement":    round(rng.uniform(0.6, 1.0), 3),
            "orca_macroCoupling":    round(rng.uniform(0.0, 0.4), 3),
            "orca_macroSpace":       round(rng.uniform(0.0, 0.35), 3),
        },
    },

    "Octopus": {
        "display": "Octopus",
        "engines_key": "Octopus",
        "brightness_range": (0.4, 0.75),
        "macro_labels": ["CHARACTER", "MOVEMENT", "INK", "SUCKER"],
        "description_flavor": "8-arm Wolfram CA wavetable — each arm a synthesis voice in chaotic lockstep, INK fires the escape cloud.",
        "tags_extra": ["wavetable", "cellular-automata", "chaotic", "poly"],
        "param_template": lambda rng: {
            "octo_wtPosition":    round(rng.uniform(0.3, 1.0), 3),
            "octo_wtScanRate":    round(rng.uniform(0.4, 1.0), 3),
            "octo_filterCutoff":  round(rng.uniform(0.4, 0.9), 3),
            "octo_filterReso":    round(rng.uniform(0.3, 0.7), 3),
            "octo_level":         round(rng.uniform(0.75, 1.0), 3),
            "octo_ampAttack":     round(rng.uniform(0.0, 0.02), 3),
            "octo_ampDecay":      round(rng.uniform(0.05, 0.35), 3),
            "octo_ampSustain":    round(rng.uniform(0.5, 0.95), 3),
            "octo_ampRelease":    round(rng.uniform(0.05, 0.3), 3),
            "octo_modAttack":     round(rng.uniform(0.0, 0.05), 3),
            "octo_modDecay":      round(rng.uniform(0.05, 0.3), 3),
            "octo_modSustain":    round(rng.uniform(0.3, 0.8), 3),
            "octo_modRelease":    round(rng.uniform(0.05, 0.3), 3),
            "octo_lfo1Rate":      round(rng.uniform(0.3, 0.9), 3),
            "octo_lfo1Depth":     round(rng.uniform(0.3, 0.8), 3),
            "octo_lfo1Shape":     rng.randint(0, 4),
            "octo_lfo2Rate":      round(rng.uniform(0.2, 0.8), 3),
            "octo_lfo2Depth":     round(rng.uniform(0.2, 0.7), 3),
            "octo_lfo2Shape":     rng.randint(0, 4),
            "octo_polyphony":     rng.randint(1, 4),
            "octo_armCount":      rng.randint(4, 8),
            "octo_armSpread":     round(rng.uniform(0.4, 0.9), 3),
            "octo_armBaseRate":   round(rng.uniform(0.4, 0.9), 3),
            "octo_armDepth":      round(rng.uniform(0.5, 1.0), 3),
            "octo_chromaSens":    round(rng.uniform(0.4, 0.9), 3),
            "octo_chromaSpeed":   round(rng.uniform(0.4, 0.9), 3),
            "octo_chromaMorph":   round(rng.uniform(0.3, 0.8), 3),
            "octo_chromaDepth":   round(rng.uniform(0.3, 0.8), 3),
            "octo_chromaFreq":    round(rng.uniform(0.3, 0.8), 3),
            "octo_inkThreshold":  round(rng.uniform(0.4, 0.9), 3),
            "octo_inkDensity":    round(rng.uniform(0.5, 1.0), 3),
            "octo_inkDecay":      round(rng.uniform(0.05, 0.35), 3),
            "octo_inkMix":        round(rng.uniform(0.4, 0.9), 3),
            "octo_shiftMicro":    round(rng.uniform(0.2, 0.7), 3),
            "octo_shiftGlide":    round(rng.uniform(0.0, 0.15), 3),
            "octo_shiftDrift":    round(rng.uniform(0.3, 0.8), 3),
            "octo_suckerReso":    round(rng.uniform(0.4, 0.9), 3),
            "octo_suckerFreq":    round(rng.uniform(0.3, 0.8), 3),
            "octo_suckerDecay":   round(rng.uniform(0.05, 0.3), 3),
            "octo_suckerMix":     round(rng.uniform(0.3, 0.8), 3),
            "octo_macroCharacter":round(rng.uniform(0.5, 1.0), 3),
            "octo_macroMovement": round(rng.uniform(0.6, 1.0), 3),
            "octo_macroCoupling": round(rng.uniform(0.0, 0.4), 3),
            "octo_macroSpace":    round(rng.uniform(0.0, 0.35), 3),
        },
    },

    "Overbite": {
        "display": "Overbite",
        "engines_key": "Overbite",
        "brightness_range": (0.45, 0.75),
        "macro_labels": ["BELLY", "BITE", "SCURRY", "PLAY DEAD"],
        "description_flavor": "Opossum beast mode — BITE and GNASH at maximum, TRASH engaged, the creature has dropped its survival act entirely.",
        "tags_extra": ["snarl", "feral", "teeth", "creature", "beast"],
        "param_template": lambda rng: {
            "poss_oscAWave":         rng.randint(0, 5),
            "poss_oscALevel":        round(rng.uniform(0.5, 0.9), 3),
            "poss_oscAShape":        round(rng.uniform(0.4, 0.9), 3),
            "poss_oscBWave":         rng.randint(0, 5),
            "poss_oscBLevel":        round(rng.uniform(0.5, 0.9), 3),
            "poss_oscBDetune":       round(rng.uniform(5.0, 25.0), 1),
            "poss_oscBInstability":  round(rng.uniform(0.3, 0.8), 3),
            "poss_oscBAsymmetry":    round(rng.uniform(0.3, 0.9), 3),
            "poss_oscInteractionMode": rng.randint(0, 5),
            "poss_oscInteractionAmt":round(rng.uniform(0.2, 0.8), 3),
            "poss_subLevel":         round(rng.uniform(0.4, 0.85), 3),
            "poss_subShape":         rng.randint(0, 3),
            "poss_subOct":           -1,
            "poss_noiseLevel":       round(rng.uniform(0.05, 0.25), 3),
            "poss_noiseType":        rng.randint(0, 5),
            "poss_noiseRouting":     rng.randint(0, 3),
            "poss_noiseTransient":   round(rng.uniform(0.5, 1.0), 3),
            "poss_filterMode":       rng.randint(0, 3),
            "poss_filterCutoff":     round(rng.uniform(800.0, 4000.0), 1),
            "poss_filterReso":       round(rng.uniform(0.3, 0.8), 3),
            "poss_filterEnvAmt":     round(rng.uniform(0.4, 0.9), 3),
            "poss_filtEnvVelScale":  round(rng.uniform(0.4, 0.9), 3),
            "poss_fur":              round(rng.uniform(0.2, 0.7), 3),
            "poss_chew":             round(rng.uniform(0.4, 0.9), 3),
            "poss_gnash":            round(rng.uniform(0.6, 1.0), 3),
            "poss_trashMode":        rng.randint(0, 3),
            "poss_trashAmount":      round(rng.uniform(0.3, 0.9), 3),
            "poss_ampAttack":        round(rng.uniform(0.001, 0.01), 4),
            "poss_ampDecay":         round(rng.uniform(0.1, 0.4), 3),
            "poss_ampSustain":       round(rng.uniform(0.5, 0.85), 3),
            "poss_ampRelease":       round(rng.uniform(0.1, 0.4), 3),
            "poss_filtEnvAttack":    round(rng.uniform(0.001, 0.01), 4),
            "poss_filtEnvDecay":     round(rng.uniform(0.08, 0.3), 3),
            "poss_filtEnvSustain":   round(rng.uniform(0.05, 0.25), 3),
            "poss_filtEnvRelease":   round(rng.uniform(0.08, 0.25), 3),
            "poss_macroScurry":      round(rng.uniform(0.4, 0.9), 3),
            "poss_weightAmt":        round(rng.uniform(0.2, 0.7), 3),
            "poss_glide":            round(rng.uniform(0.0, 0.1), 3),
            "poss_masterVolume":     round(rng.uniform(0.7, 0.85), 3),
        },
    },
}

# Canonical engine order for iteration
ALL_ENGINES = [
    "Onset", "Obese", "Ouroboros", "Overworld", "Oblique",
    "Ottoni", "Ole", "Orca", "Octopus", "Overbite",
]

# ---------------------------------------------------------------------------
# Name generation
# ---------------------------------------------------------------------------

def _generate_name(engine_name: str, band_name: str, index: int, rng: random.Random) -> str:
    """Generate a unique aggressive preset name."""
    word = rng.choice(AGGRESSION_WORDS)
    suffix = rng.choice(AGGRESSION_SUFFIXES)
    # Avoid word == suffix collision (unlikely but guard)
    return f"{word} {suffix}"


def _make_unique_name(base_name: str, seen_names: set, rng: random.Random) -> str:
    """Append a roman numeral / ordinal if the name collides."""
    if base_name not in seen_names:
        return base_name
    for i in range(2, 20):
        candidate = f"{base_name} {i}"
        if candidate not in seen_names:
            return candidate
    # Fallback: append random hex
    return f"{base_name} {rng.randint(100, 999)}"


# ---------------------------------------------------------------------------
# DNA generation
# ---------------------------------------------------------------------------

def _generate_dna(band_name: str, brightness_range: tuple, rng: random.Random) -> dict:
    """
    Generate sonic DNA for an aggressive preset.
    Rules:
      - aggression: sampled from the band's range
      - movement: always high (0.7–1.0) — aggression implies motion
      - space: low–medium (0.3–0.5) — tight, compressed
      - brightness: engine-specific range
      - warmth: inverse to aggression band (beast mode is cold/bright)
      - density: high (0.65–0.95)
    """
    lo, hi = BANDS[band_name]
    aggression = round(rng.uniform(lo, hi), 3)
    movement = round(rng.uniform(0.7, 1.0), 3)
    space = round(rng.uniform(0.3, 0.5), 3)
    brightness = round(rng.uniform(*brightness_range), 3)

    # Warmth inversely correlated with aggression band
    if band_name == "medium-high":
        warmth = round(rng.uniform(0.3, 0.55), 3)
    elif band_name == "high":
        warmth = round(rng.uniform(0.2, 0.45), 3)
    else:  # extreme
        warmth = round(rng.uniform(0.1, 0.35), 3)

    density = round(rng.uniform(0.65, 0.95), 3)

    return {
        "brightness": brightness,
        "warmth":     warmth,
        "aggression": aggression,
        "movement":   movement,
        "density":    density,
        "space":      space,
    }


# ---------------------------------------------------------------------------
# Preset assembly
# ---------------------------------------------------------------------------

def build_preset(
    engine_key: str,
    band_name: str,
    preset_name: str,
    rng: random.Random,
) -> dict:
    """Build a complete .xometa preset dict."""
    engine_def = ENGINE_DEFS[engine_key]
    dna = _generate_dna(band_name, engine_def["brightness_range"], rng)

    tags = list(AGGRESSION_TAGS_BY_BAND[band_name]) + list(engine_def["tags_extra"])
    rng.shuffle(tags)

    params = engine_def["param_template"](rng)

    description = (
        f"{engine_def['description_flavor']} "
        f"Aggression {dna['aggression']:.2f} / Movement {dna['movement']:.2f} / "
        f"Space {dna['space']:.2f}. Band: {band_name}."
    )

    return {
        "schema_version": 1,
        "name": preset_name,
        "mood": "Flux",
        "sonic_dna": dna,
        "engines": [engine_def["engines_key"]],
        "author": "XO_OX Aggression Expansion",
        "version": "1.0.0",
        "description": description,
        "tags": tags,
        "macroLabels": engine_def["macro_labels"],
        "couplingIntensity": "None",
        "dna": dna,
        "parameters": {
            engine_def["engines_key"]: params,
        },
        "coupling": {"pairs": []},
        "sequencer": None,
    }


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def parse_args():
    repo_root = Path(__file__).resolve().parent.parent
    default_output = repo_root / "Presets" / "XOceanus" / "Flux"

    parser = argparse.ArgumentParser(
        description="Generate high-aggression preset stubs for XOceanus engines.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=default_output,
        help=f"Directory to write .xometa files (default: {default_output})",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print what would be written without creating files.",
    )
    parser.add_argument(
        "--engines",
        type=str,
        default=None,
        help="Comma-separated engine names to generate (e.g. Onset,Overworld). "
             f"Available: {', '.join(ALL_ENGINES)}",
    )
    parser.add_argument(
        "--count-per-band",
        type=int,
        default=3,
        metavar="N",
        help="Number of presets per engine per band (default: 3).",
    )
    parser.add_argument(
        "--seed",
        type=int,
        default=None,
        help="Random seed for reproducible output.",
    )
    return parser.parse_args()


def resolve_engines(engines_arg) -> list:
    """Validate and return list of engine keys to generate."""
    if engines_arg is None:
        return list(ALL_ENGINES)
    requested = [e.strip() for e in engines_arg.split(",") if e.strip()]
    valid = set(ALL_ENGINES)
    bad = [e for e in requested if e not in valid]
    if bad:
        print(f"ERROR: Unknown engine(s): {', '.join(bad)}", file=sys.stderr)
        print(f"       Available: {', '.join(ALL_ENGINES)}", file=sys.stderr)
        sys.exit(1)
    return requested


def main() -> None:
    args = parse_args()
    rng = random.Random(args.seed)

    engines = resolve_engines(args.engines)
    count_per_band = max(1, args.count_per_band)
    output_dir = args.output_dir

    if not args.dry_run:
        output_dir.mkdir(parents=True, exist_ok=True)

    total_written = 0
    total_skipped = 0
    seen_names: set[str] = set()

    print(f"XOceanus Aggression Expansion Pack")
    print(f"  Engines     : {', '.join(engines)}")
    print(f"  Bands       : {', '.join(BANDS.keys())}")
    print(f"  Per band    : {count_per_band}")
    print(f"  Total target: {len(engines) * len(BANDS) * count_per_band}")
    print(f"  Output dir  : {output_dir}")
    print(f"  Dry run     : {args.dry_run}")
    if args.seed is not None:
        print(f"  Seed        : {args.seed}")
    print()

    for engine_key in engines:
        engine_def = ENGINE_DEFS[engine_key]
        print(f"  [{engine_key}]")
        for band_name in BANDS:
            for i in range(count_per_band):
                raw_name = _generate_name(engine_key, band_name, i, rng)
                preset_name = _make_unique_name(raw_name, seen_names, rng)
                seen_names.add(preset_name)

                preset = build_preset(engine_key, band_name, preset_name, rng)

                # Filename: sanitize preset name
                safe_name = preset_name.replace("/", "-").replace("\\", "-")
                filename = f"{safe_name}.xometa"
                filepath = output_dir / filename

                if args.dry_run:
                    aggr = preset["dna"]["aggression"]
                    print(f"    [DRY RUN] {filename}  (aggression={aggr:.3f}, band={band_name})")
                    total_written += 1
                    continue

                if filepath.exists():
                    print(f"    SKIP (exists): {filename}")
                    total_skipped += 1
                    continue

                with open(filepath, "w", encoding="utf-8") as fh:
                    json.dump(preset, fh, indent=2, ensure_ascii=False)
                    fh.write("\n")

                aggr = preset["dna"]["aggression"]
                print(f"    WRITE: {filename}  (aggression={aggr:.3f}, band={band_name})")
                total_written += 1

    print()
    if args.dry_run:
        print(f"Dry run complete. Would write {total_written} presets.")
    else:
        print(f"Done. Wrote {total_written} preset(s), skipped {total_skipped} existing.")


if __name__ == "__main__":
    main()
