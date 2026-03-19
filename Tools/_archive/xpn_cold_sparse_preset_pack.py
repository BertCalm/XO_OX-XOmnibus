#!/usr/bin/env python3
"""
xpn_cold_sparse_preset_pack.py

Generates Cold Sparse quadrant preset stubs (.xometa) for XOmnibus engines.

The warmth/density balance doc prescribed a "Cold Sparse" quadrant
(warmth 0.0–0.25, density 0.0–0.25) — single crystal sine, clinical plucks,
skeletal digital voices. This tool fills that underpopulated territory.

Target engines: ODYSSEY, OVERWORLD, OPTIC, OBLIQUE, ORIGAMI, ORACLE,
                OBSCURA, ODDFELIX, OUTWIT, OVERLAP

DNA targets:
  warmth      0.00 – 0.25   (clinical, cold, zero harmonic warmth)
  density     0.00 – 0.25   (single voices, skeletal textures)
  brightness  0.70 – 1.00   (icy, spectral, glassy)
  movement    0.30 – 0.60   (slow drift, minimal motion)
  space       0.60 – 0.90   (reverberant emptiness)
  aggression  0.10 – 0.35   (precise, controlled, not violent)

Mood: Prism (bright, analytical, precise)

Usage:
    python3 Tools/xpn_cold_sparse_preset_pack.py
    python3 Tools/xpn_cold_sparse_preset_pack.py --dry-run
    python3 Tools/xpn_cold_sparse_preset_pack.py --engines ODYSSEY,OPTIC --count 5
    python3 Tools/xpn_cold_sparse_preset_pack.py --output-dir /tmp/cold_test --seed 42
"""

import argparse
import json
import os
import random
import sys
from pathlib import Path

# ---------------------------------------------------------------------------
# Name vocabulary — cold / sparse / clinical aesthetic
# ---------------------------------------------------------------------------

COLD_NAMES = [
    "Filament",
    "Solitary",
    "Bare Nerve",
    "Arctic",
    "Clinical",
    "Steel Thread",
    "Single Point",
    "Void Trace",
    "Sterile",
    "Crystal Shard",
    "Lone Signal",
    "Null Space",
    "Pure Sine",
    "Cold Wire",
    "Glass Needle",
    "Sparse Field",
    "Isolate",
    "Skeletal",
    "Empty Chamber",
    "One Voice",
]

COLD_TAGS_BASE = [
    "cold", "sparse", "clinical", "crystalline", "minimal",
    "analytical", "precise", "skeletal", "digital", "glassy",
]

# ---------------------------------------------------------------------------
# Engine definitions
# ---------------------------------------------------------------------------

ENGINE_DEFS = {

    "ODYSSEY": {
        "display": "Odyssey",
        "engines_key": "Odyssey",
        "macro_labels": ["DRIFT", "WANDER", "DEPTH", "RETURN"],
        "description_flavor": "Lone wavetable voice adrift in open space — DRIFT introduces microscopic pitch wander, DEPTH opens a cathedral of silence around a single sine.",
        "tags_extra": ["wavetable", "drift", "minimal", "lone-voice"],
        "param_template": lambda rng: {
            "drift_oscA_mode":       rng.randint(0, 2),
            "drift_oscA_level":      round(rng.uniform(0.5, 0.85), 3),
            "drift_oscA_detune":     round(rng.uniform(-0.02, 0.02), 4),
            "drift_oscA_shape":      round(rng.uniform(0.0, 0.15), 3),
            "drift_oscB_mode":       0,
            "drift_oscB_level":      round(rng.uniform(0.0, 0.2), 3),
            "drift_oscB_detune":     round(rng.uniform(-0.05, 0.05), 4),
            "drift_subLevel":        round(rng.uniform(0.0, 0.1), 3),
            "drift_noiseLevel":      round(rng.uniform(0.0, 0.05), 3),
            "drift_filterCutoff":    round(rng.uniform(0.6, 1.0), 3),
            "drift_filterReso":      round(rng.uniform(0.0, 0.25), 3),
            "drift_filterEnvAmt":    round(rng.uniform(-0.1, 0.1), 3),
            "drift_ampAttack":       round(rng.uniform(0.02, 0.2), 3),
            "drift_ampDecay":        round(rng.uniform(0.05, 0.4), 3),
            "drift_ampSustain":      round(rng.uniform(0.6, 1.0), 3),
            "drift_ampRelease":      round(rng.uniform(0.3, 1.2), 3),
            "drift_filtEnvAttack":   round(rng.uniform(0.01, 0.1), 3),
            "drift_filtEnvDecay":    round(rng.uniform(0.05, 0.3), 3),
            "drift_filtEnvSustain":  round(rng.uniform(0.5, 1.0), 3),
            "drift_filtEnvRelease":  round(rng.uniform(0.2, 0.8), 3),
            "drift_lfo1Rate":        round(rng.uniform(0.01, 0.15), 4),
            "drift_lfo1Depth":       round(rng.uniform(0.0, 0.12), 3),
            "drift_lfo1Shape":       rng.randint(0, 2),
            "drift_lfo2Rate":        round(rng.uniform(0.005, 0.08), 4),
            "drift_lfo2Depth":       round(rng.uniform(0.0, 0.08), 3),
            "drift_glide":           round(rng.uniform(0.0, 0.15), 3),
            "drift_voiceMode":       0,
            "drift_chorus":          round(rng.uniform(0.0, 0.1), 3),
            "drift_reverbMix":       round(rng.uniform(0.5, 0.85), 3),
            "drift_reverbSize":      round(rng.uniform(0.6, 1.0), 3),
            "drift_delayMix":        round(rng.uniform(0.0, 0.2), 3),
            "drift_masterVol":       round(rng.uniform(0.7, 0.9), 3),
        },
    },

    "OVERWORLD": {
        "display": "Overworld",
        "engines_key": "Overworld",
        "macro_labels": ["ERA", "GLITCH", "CRUSH", "CHAOS"],
        "description_flavor": "Single NES pulse thread frozen in amber — ERA locked to the earliest silicon, pulse duty at minimum, glitch silenced, one clean digital voice.",
        "tags_extra": ["chiptune", "digital", "pulse", "minimal-chip"],
        "param_template": lambda rng: {
            "ow_era":            round(rng.uniform(0.0, 0.2), 3),
            "ow_eraY":           round(rng.uniform(0.0, 0.15), 3),
            "ow_voiceMode":      0,
            "ow_masterVol":      round(rng.uniform(0.65, 0.85), 3),
            "ow_masterTune":     0.0,
            "ow_pulseDuty":      round(rng.uniform(0.05, 0.15), 3),
            "ow_pulseSweep":     round(rng.uniform(0.0, 0.1), 3),
            "ow_triEnable":      0,
            "ow_noiseMode":      0,
            "ow_noisePeriod":    0.0,
            "ow_dpcmEnable":     0,
            "ow_dpcmRate":       0.0,
            "ow_nesMix":         round(rng.uniform(0.7, 1.0), 3),
            "ow_fmAlgorithm":    0,
            "ow_fmFeedback":     round(rng.uniform(0.0, 0.05), 3),
            "ow_fmOp1Level":     round(rng.uniform(0.7, 1.0), 3),
            "ow_fmOp2Level":     round(rng.uniform(0.0, 0.1), 3),
            "ow_fmOp3Level":     0.0,
            "ow_fmOp4Level":     0.0,
            "ow_fmOp1Mult":      1,
            "ow_fmOp2Mult":      1,
            "ow_fmOp3Mult":      1,
            "ow_fmOp4Mult":      1,
            "ow_fmOp1Detune":    0.0,
            "ow_fmOp2Detune":    0.0,
            "ow_fmOp3Detune":    0.0,
            "ow_fmOp4Detune":    0.0,
            "ow_fmAttack":       round(rng.uniform(0.01, 0.1), 3),
            "ow_fmDecay":        round(rng.uniform(0.1, 0.5), 3),
            "ow_fmSustain":      round(rng.uniform(0.7, 1.0), 3),
            "ow_fmRelease":      round(rng.uniform(0.2, 0.8), 3),
            "ow_fmLfoRate":      round(rng.uniform(0.005, 0.05), 4),
            "ow_fmLfoDepth":     round(rng.uniform(0.0, 0.08), 3),
            "ow_brrSample":      0,
            "ow_brrInterp":      0,
            "ow_snesAttack":     round(rng.uniform(0.01, 0.1), 3),
            "ow_snesDecay":      round(rng.uniform(0.1, 0.5), 3),
            "ow_snesSustain":    round(rng.uniform(0.7, 1.0), 3),
            "ow_snesRelease":    round(rng.uniform(0.2, 0.8), 3),
            "ow_pitchMod":       round(rng.uniform(0.0, 0.05), 3),
            "ow_noiseReplace":   0.0,
            "ow_echoDelay":      round(rng.uniform(0.0, 0.2), 3),
            "ow_echoFeedback":   round(rng.uniform(0.0, 0.2), 3),
            "ow_echoMix":        round(rng.uniform(0.0, 0.15), 3),
            "ow_echoFir0":       round(rng.uniform(0.5, 0.9), 3),
            "ow_echoFir1":       0.0,
            "ow_echoFir2":       0.0,
            "ow_echoFir3":       0.0,
            "ow_echoFir4":       0.0,
            "ow_echoFir5":       0.0,
            "ow_echoFir6":       0.0,
            "ow_echoFir7":       0.0,
            "ow_glitchAmount":   round(rng.uniform(0.0, 0.1), 3),
            "ow_glitchType":     0,
            "ow_glitchRate":     0.0,
            "ow_glitchDepth":    0.0,
            "ow_glitchMix":      0.0,
            "ow_ampAttack":      round(rng.uniform(0.01, 0.1), 3),
            "ow_ampDecay":       round(rng.uniform(0.05, 0.3), 3),
            "ow_ampSustain":     round(rng.uniform(0.7, 1.0), 3),
            "ow_ampRelease":     round(rng.uniform(0.2, 0.8), 3),
            "ow_filterCutoff":   round(rng.uniform(0.7, 1.0), 3),
            "ow_filterReso":     round(rng.uniform(0.0, 0.15), 3),
            "ow_filterType":     0,
            "ow_crushBits":      round(rng.uniform(0.85, 1.0), 3),
            "ow_crushRate":      round(rng.uniform(0.85, 1.0), 3),
            "ow_crushMix":       round(rng.uniform(0.0, 0.1), 3),
            "ow_eraDriftRate":   round(rng.uniform(0.005, 0.05), 4),
            "ow_eraDriftDepth":  round(rng.uniform(0.0, 0.08), 3),
            "ow_eraDriftShape":  0,
            "ow_eraPortaTime":   round(rng.uniform(0.0, 0.05), 3),
            "ow_eraMemTime":     0.0,
            "ow_eraMemMix":      0.0,
            "ow_vertexA":        round(rng.uniform(0.0, 0.2), 3),
            "ow_vertexB":        round(rng.uniform(0.0, 0.15), 3),
            "ow_vertexC":        round(rng.uniform(0.0, 0.1), 3),
            "ow_colorTheme":     rng.randint(0, 14),
        },
    },

    "OPTIC": {
        "display": "Optic",
        "engines_key": "Optic",
        "macro_labels": ["PULSE", "SCAN", "PHASE", "GATE"],
        "description_flavor": "Phosphor trace — a single AutoPulse thread at minimum rate, the visual modulator frozen in a single cycle, zero density signal.",
        "tags_extra": ["visual", "pulse", "phosphor", "zero-audio-identity"],
        "param_template": lambda rng: {
            "optic_pulseRate":        round(rng.uniform(0.005, 0.08), 4),
            "optic_pulseShape":       rng.randint(0, 2),
            "optic_pulseDepth":       round(rng.uniform(0.1, 0.4), 3),
            "optic_scanRate":         round(rng.uniform(0.005, 0.06), 4),
            "optic_scanDepth":        round(rng.uniform(0.05, 0.25), 3),
            "optic_phaseOffset":      round(rng.uniform(0.0, 0.3), 3),
            "optic_gateThreshold":    round(rng.uniform(0.5, 0.9), 3),
            "optic_gateSmooth":       round(rng.uniform(0.4, 0.8), 3),
            "optic_outputLevel":      round(rng.uniform(0.3, 0.7), 3),
            "optic_rateSync":         0,
            "optic_lfo1Rate":         round(rng.uniform(0.005, 0.05), 4),
            "optic_lfo1Depth":        round(rng.uniform(0.0, 0.15), 3),
            "optic_lfo1Shape":        rng.randint(0, 2),
            "optic_visualMode":       rng.randint(0, 3),
            "optic_colorIndex":       rng.randint(0, 7),
            "optic_trailLength":      round(rng.uniform(0.3, 0.7), 3),
            "optic_trailDecay":       round(rng.uniform(0.4, 0.8), 3),
            "optic_masterBrightness": round(rng.uniform(0.6, 0.95), 3),
        },
    },

    "OBLIQUE": {
        "display": "Oblique",
        "engines_key": "Oblique",
        "macro_labels": ["FOLD", "BOUNCE", "PRISM", "PHASE"],
        "description_flavor": "Prismatic pluck stripped to a single fold — FOLD at minimum, BOUNCE disabled, one crystalline voice launched into reverb space.",
        "tags_extra": ["wavefold", "pluck", "prism", "clinical"],
        "param_template": lambda rng: {
            "oblq_oscWave":        rng.randint(0, 2),
            "oblq_oscFold":        round(rng.uniform(0.0, 0.15), 3),
            "oblq_oscDetune":      round(rng.uniform(-0.01, 0.01), 4),
            "oblq_level":          round(rng.uniform(0.6, 0.85), 3),
            "oblq_glide":          round(rng.uniform(0.0, 0.05), 3),
            "oblq_percClick":      round(rng.uniform(0.0, 0.2), 3),
            "oblq_percDecay":      round(rng.uniform(0.005, 0.05), 4),
            "oblq_bounceRate":     round(rng.uniform(0.0, 0.1), 3),
            "oblq_bounceGravity":  round(rng.uniform(0.0, 0.1), 3),
            "oblq_bounceDamp":     round(rng.uniform(0.7, 1.0), 3),
            "oblq_bounceCnt":      1,
            "oblq_bounceSwing":    0.0,
            "oblq_clickTone":      round(rng.uniform(0.6, 1.0), 3),
            "oblq_filterCut":      round(rng.uniform(0.7, 1.0), 3),
            "oblq_filterRes":      round(rng.uniform(0.0, 0.2), 3),
            "oblq_attack":         round(rng.uniform(0.005, 0.05), 4),
            "oblq_decay":          round(rng.uniform(0.05, 0.4), 3),
            "oblq_sustain":        round(rng.uniform(0.5, 0.9), 3),
            "oblq_release":        round(rng.uniform(0.3, 1.0), 3),
            "oblq_prismDelay":     round(rng.uniform(0.2, 0.5), 3),
            "oblq_prismSpread":    round(rng.uniform(0.1, 0.35), 3),
            "oblq_prismColor":     round(rng.uniform(0.6, 1.0), 3),
            "oblq_prismWidth":     round(rng.uniform(0.3, 0.6), 3),
            "oblq_prismFeedback":  round(rng.uniform(0.0, 0.2), 3),
            "oblq_prismMix":       round(rng.uniform(0.3, 0.6), 3),
            "oblq_prismDamp":      round(rng.uniform(0.5, 0.9), 3),
            "oblq_phaserRate":     round(rng.uniform(0.005, 0.06), 4),
            "oblq_phaserDepth":    round(rng.uniform(0.1, 0.4), 3),
            "oblq_phaserFeedback": round(rng.uniform(0.0, 0.2), 3),
            "oblq_phaserMix":      round(rng.uniform(0.1, 0.35), 3),
        },
    },

    "ORIGAMI": {
        "display": "Origami",
        "engines_key": "Origami",
        "macro_labels": ["FOLD", "CREASE", "LAYER", "UNFOLD"],
        "description_flavor": "A single paper fold — FOLD at its first crease, no layers, no stacking. Vermillion light on white paper.",
        "tags_extra": ["fold", "paper", "structural", "single-fold"],
        "param_template": lambda rng: {
            "origami_foldPoint":      round(rng.uniform(0.1, 0.4), 3),
            "origami_foldDepth":      round(rng.uniform(0.05, 0.25), 3),
            "origami_foldSymmetry":   round(rng.uniform(0.8, 1.0), 3),
            "origami_creaseAngle":    round(rng.uniform(0.0, 0.2), 3),
            "origami_layerCount":     1,
            "origami_layerSpacing":   0.0,
            "origami_layerBlend":     0.0,
            "origami_unfoldRate":     round(rng.uniform(0.01, 0.1), 3),
            "origami_unfoldDepth":    round(rng.uniform(0.0, 0.15), 3),
            "origami_paperDamp":      round(rng.uniform(0.6, 1.0), 3),
            "origami_paperStiffness": round(rng.uniform(0.5, 0.9), 3),
            "origami_filterCutoff":   round(rng.uniform(0.7, 1.0), 3),
            "origami_filterReso":     round(rng.uniform(0.0, 0.2), 3),
            "origami_ampAttack":      round(rng.uniform(0.005, 0.05), 4),
            "origami_ampDecay":       round(rng.uniform(0.05, 0.4), 3),
            "origami_ampSustain":     round(rng.uniform(0.5, 0.9), 3),
            "origami_ampRelease":     round(rng.uniform(0.3, 1.0), 3),
            "origami_reverbMix":      round(rng.uniform(0.5, 0.85), 3),
            "origami_reverbSize":     round(rng.uniform(0.6, 1.0), 3),
            "origami_masterVol":      round(rng.uniform(0.65, 0.85), 3),
        },
    },

    "ORACLE": {
        "display": "Oracle",
        "engines_key": "Oracle",
        "macro_labels": ["VOICE", "MUTATION", "GRAVITY", "SILENCE"],
        "description_flavor": "A single GENDY stochastic point — breakpoints set to 1, mutation frozen, the oracle speaks once and stops.",
        "tags_extra": ["gendy", "stochastic", "single-point", "indeterminate"],
        "param_template": lambda rng: {
            "oracle_breakpoints":     1,
            "oracle_amplitude":       round(rng.uniform(0.5, 0.85), 3),
            "oracle_durationDist":    0,
            "oracle_amplitudeDist":   0,
            "oracle_durationScale":   round(rng.uniform(0.0, 0.15), 3),
            "oracle_amplitudeScale":  round(rng.uniform(0.0, 0.12), 3),
            "oracle_minFreq":         round(rng.uniform(0.3, 0.6), 3),
            "oracle_maxFreq":         round(rng.uniform(0.6, 0.95), 3),
            "oracle_stepSize":        round(rng.uniform(0.01, 0.15), 3),
            "oracle_mutation":        round(rng.uniform(0.0, 0.1), 3),
            "oracle_gravity":         round(rng.uniform(0.0, 0.15), 3),
            "oracle_maqamRoot":       rng.randint(0, 11),
            "oracle_maqamMode":       0,
            "oracle_maqamStrength":   round(rng.uniform(0.0, 0.3), 3),
            "oracle_filterCutoff":    round(rng.uniform(0.7, 1.0), 3),
            "oracle_filterReso":      round(rng.uniform(0.0, 0.2), 3),
            "oracle_ampAttack":       round(rng.uniform(0.02, 0.15), 3),
            "oracle_ampDecay":        round(rng.uniform(0.1, 0.5), 3),
            "oracle_ampSustain":      round(rng.uniform(0.5, 0.9), 3),
            "oracle_ampRelease":      round(rng.uniform(0.4, 1.2), 3),
            "oracle_reverbMix":       round(rng.uniform(0.5, 0.85), 3),
            "oracle_reverbSize":      round(rng.uniform(0.65, 1.0), 3),
            "oracle_masterVol":       round(rng.uniform(0.65, 0.85), 3),
        },
    },

    "OBSCURA": {
        "display": "Obscura",
        "engines_key": "Obscura",
        "macro_labels": ["STIFFNESS", "TAPER", "SCATTER", "EXPOSURE"],
        "description_flavor": "Daguerreotype silver — a single resonant mode, stiffness tuned to surgical sharpness, scatter near zero, one clinical exposure.",
        "tags_extra": ["modal", "resonant", "single-mode", "daguerreotype"],
        "param_template": lambda rng: {
            "obscura_stiffness":      round(rng.uniform(0.6, 0.95), 3),
            "obscura_taper":          round(rng.uniform(0.5, 0.9), 3),
            "obscura_scatter":        round(rng.uniform(0.0, 0.1), 3),
            "obscura_modeCount":      1,
            "obscura_modeSpacing":    round(rng.uniform(0.8, 1.0), 3),
            "obscura_modeDecay":      round(rng.uniform(0.05, 0.4), 3),
            "obscura_inharmonicity":  round(rng.uniform(0.0, 0.1), 3),
            "obscura_bowPressure":    round(rng.uniform(0.05, 0.25), 3),
            "obscura_bowPosition":    round(rng.uniform(0.3, 0.7), 3),
            "obscura_excitationType": 0,
            "obscura_filterCutoff":   round(rng.uniform(0.7, 1.0), 3),
            "obscura_filterReso":     round(rng.uniform(0.0, 0.2), 3),
            "obscura_ampAttack":      round(rng.uniform(0.01, 0.1), 3),
            "obscura_ampDecay":       round(rng.uniform(0.05, 0.4), 3),
            "obscura_ampSustain":     round(rng.uniform(0.5, 0.9), 3),
            "obscura_ampRelease":     round(rng.uniform(0.3, 1.0), 3),
            "obscura_reverbMix":      round(rng.uniform(0.5, 0.85), 3),
            "obscura_reverbSize":     round(rng.uniform(0.6, 1.0), 3),
            "obscura_exposure":       round(rng.uniform(0.4, 0.7), 3),
            "obscura_masterVol":      round(rng.uniform(0.65, 0.85), 3),
        },
    },

    "ODDFELIX": {
        "display": "OddfeliX",
        "engines_key": "OddfeliX",
        "macro_labels": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
        "description_flavor": "Neon tetra flash — single-voice CHARACTER mode, filter open, movement frozen, the fish holds perfectly still against black water.",
        "tags_extra": ["neon-tetra", "single-voice", "clinical", "minimal"],
        "param_template": lambda rng: {
            "snap_filterCutoff":      round(rng.uniform(0.75, 1.0), 3),
            "snap_filterReso":        round(rng.uniform(0.0, 0.2), 3),
            "snap_filterEnvAmt":      round(rng.uniform(-0.1, 0.1), 3),
            "snap_filterType":        0,
            "snap_oscAWave":          rng.randint(0, 2),
            "snap_oscALevel":         round(rng.uniform(0.6, 0.9), 3),
            "snap_oscAShape":         round(rng.uniform(0.0, 0.15), 3),
            "snap_oscBWave":          0,
            "snap_oscBLevel":         round(rng.uniform(0.0, 0.15), 3),
            "snap_oscBDetune":        round(rng.uniform(-0.02, 0.02), 4),
            "snap_subLevel":          round(rng.uniform(0.0, 0.1), 3),
            "snap_noiseLevel":        round(rng.uniform(0.0, 0.05), 3),
            "snap_ampAttack":         round(rng.uniform(0.005, 0.08), 4),
            "snap_ampDecay":          round(rng.uniform(0.05, 0.35), 3),
            "snap_ampSustain":        round(rng.uniform(0.6, 1.0), 3),
            "snap_ampRelease":        round(rng.uniform(0.2, 0.8), 3),
            "snap_filtEnvAttack":     round(rng.uniform(0.005, 0.06), 4),
            "snap_filtEnvDecay":      round(rng.uniform(0.05, 0.3), 3),
            "snap_filtEnvSustain":    round(rng.uniform(0.5, 1.0), 3),
            "snap_filtEnvRelease":    round(rng.uniform(0.1, 0.5), 3),
            "snap_lfo1Rate":          round(rng.uniform(0.005, 0.06), 4),
            "snap_lfo1Depth":         round(rng.uniform(0.0, 0.1), 3),
            "snap_lfo1Shape":         rng.randint(0, 2),
            "snap_lfo2Rate":          round(rng.uniform(0.005, 0.04), 4),
            "snap_lfo2Depth":         round(rng.uniform(0.0, 0.08), 3),
            "snap_glide":             round(rng.uniform(0.0, 0.1), 3),
            "snap_voiceMode":         0,
            "snap_chorus":            round(rng.uniform(0.0, 0.1), 3),
            "snap_reverbMix":         round(rng.uniform(0.5, 0.85), 3),
            "snap_reverbSize":        round(rng.uniform(0.6, 1.0), 3),
            "snap_delayMix":          round(rng.uniform(0.0, 0.15), 3),
            "snap_masterVol":         round(rng.uniform(0.65, 0.85), 3),
        },
    },

    "OUTWIT": {
        "display": "Outwit",
        "engines_key": "Outwit",
        "macro_labels": ["ARM", "RULE", "INK", "SPREAD"],
        "description_flavor": "One arm, one rule — giant Pacific octopus reduced to a single Wolfram CA thread, rule 90, minimum ink, maximum void.",
        "tags_extra": ["cellular-automata", "wolfram", "single-arm", "void"],
        "param_template": lambda rng: {
            "owit_armCount":          1,
            "owit_armSpread":         round(rng.uniform(0.0, 0.15), 3),
            "owit_ruleIndex":         rng.choice([0, 2, 4]),
            "owit_ruleBlend":         round(rng.uniform(0.0, 0.1), 3),
            "owit_caRate":            round(rng.uniform(0.01, 0.1), 3),
            "owit_caDensity":         round(rng.uniform(0.05, 0.2), 3),
            "owit_caDepth":           round(rng.uniform(0.1, 0.35), 3),
            "owit_voiceCount":        1,
            "owit_voiceSpread":       0.0,
            "owit_oscillatorMode":    0,
            "owit_filterCutoff":      round(rng.uniform(0.7, 1.0), 3),
            "owit_filterReso":        round(rng.uniform(0.0, 0.2), 3),
            "owit_ampAttack":         round(rng.uniform(0.01, 0.1), 3),
            "owit_ampDecay":          round(rng.uniform(0.05, 0.4), 3),
            "owit_ampSustain":        round(rng.uniform(0.5, 0.9), 3),
            "owit_ampRelease":        round(rng.uniform(0.3, 1.0), 3),
            "owit_inkThreshold":      round(rng.uniform(0.7, 1.0), 3),
            "owit_inkDensity":        round(rng.uniform(0.0, 0.1), 3),
            "owit_inkDecay":          round(rng.uniform(0.3, 0.8), 3),
            "owit_inkMix":            round(rng.uniform(0.0, 0.1), 3),
            "owit_reverbMix":         round(rng.uniform(0.5, 0.85), 3),
            "owit_reverbSize":        round(rng.uniform(0.65, 1.0), 3),
            "owit_masterVol":         round(rng.uniform(0.65, 0.85), 3),
        },
    },

    "OVERLAP": {
        "display": "Overlap",
        "engines_key": "Overlap",
        "macro_labels": ["KNOT", "DECAY", "DIFFUSE", "TANGLE"],
        "description_flavor": "Lion's mane knot topology at rest — a single FDN node, maximum decay, diffusion minimal, one thread through the void.",
        "tags_extra": ["fdn", "reverb", "single-node", "knot-topology"],
        "param_template": lambda rng: {
            "olap_knotTopology":      0,
            "olap_nodeCount":         1,
            "olap_decayTime":         round(rng.uniform(2.0, 8.0), 2),
            "olap_diffusion":         round(rng.uniform(0.0, 0.2), 3),
            "olap_damping":           round(rng.uniform(0.7, 1.0), 3),
            "olap_modRate":           round(rng.uniform(0.005, 0.04), 4),
            "olap_modDepth":          round(rng.uniform(0.0, 0.1), 3),
            "olap_tangleAmount":      round(rng.uniform(0.0, 0.1), 3),
            "olap_predelay":          round(rng.uniform(0.0, 0.05), 3),
            "olap_filterCutoff":      round(rng.uniform(0.7, 1.0), 3),
            "olap_filterReso":        round(rng.uniform(0.0, 0.15), 3),
            "olap_inputGain":         round(rng.uniform(0.5, 0.8), 3),
            "olap_wetDry":            round(rng.uniform(0.5, 0.85), 3),
            "olap_masterVol":         round(rng.uniform(0.65, 0.85), 3),
        },
    },
}

# Canonical engine order for iteration
ALL_ENGINES = [
    "ODYSSEY", "OVERWORLD", "OPTIC", "OBLIQUE", "ORIGAMI",
    "ORACLE", "OBSCURA", "ODDFELIX", "OUTWIT", "OVERLAP",
]

# ---------------------------------------------------------------------------
# DNA generation
# ---------------------------------------------------------------------------

def _generate_dna(rng: random.Random) -> dict:
    """Generate Cold Sparse 6D Sonic DNA."""
    return {
        "warmth":     round(rng.uniform(0.0, 0.25), 3),
        "density":    round(rng.uniform(0.0, 0.25), 3),
        "brightness": round(rng.uniform(0.7, 1.0), 3),
        "movement":   round(rng.uniform(0.3, 0.6), 3),
        "space":      round(rng.uniform(0.6, 0.9), 3),
        "aggression": round(rng.uniform(0.1, 0.35), 3),
    }


# ---------------------------------------------------------------------------
# Name generation
# ---------------------------------------------------------------------------

def _generate_name(rng: random.Random) -> str:
    """Pick a cold sparse name from the vocabulary."""
    return rng.choice(COLD_NAMES)


def _make_unique_name(base_name: str, seen_names: set, rng: random.Random) -> str:
    """Append a numeric suffix if the name collides."""
    if base_name not in seen_names:
        return base_name
    for i in range(2, 30):
        candidate = f"{base_name} {i}"
        if candidate not in seen_names:
            return candidate
    return f"{base_name} {rng.randint(100, 999)}"


# ---------------------------------------------------------------------------
# Preset assembly
# ---------------------------------------------------------------------------

def build_preset(engine_key: str, preset_name: str, rng: random.Random) -> dict:
    """Build a complete .xometa cold sparse preset dict."""
    engine_def = ENGINE_DEFS[engine_key]
    dna = _generate_dna(rng)

    tags = list(COLD_TAGS_BASE) + list(engine_def["tags_extra"])
    rng.shuffle(tags)
    # Deduplicate while preserving rough order
    seen_tags: set = set()
    unique_tags = []
    for t in tags:
        if t not in seen_tags:
            unique_tags.append(t)
            seen_tags.add(t)

    params = engine_def["param_template"](rng)

    description = (
        f"{engine_def['description_flavor']} "
        f"Warmth {dna['warmth']:.2f} / Density {dna['density']:.2f} / "
        f"Brightness {dna['brightness']:.2f} / Space {dna['space']:.2f}."
    )

    return {
        "schema_version": 1,
        "name": preset_name,
        "mood": "Prism",
        "sonic_dna": dna,
        "engines": [engine_def["engines_key"]],
        "author": "XO_OX Cold Sparse Pack",
        "version": "1.0.0",
        "description": description,
        "tags": unique_tags[:8],
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
    default_output = repo_root / "Presets" / "XOmnibus" / "Prism"

    parser = argparse.ArgumentParser(
        description="Generate Cold Sparse quadrant preset stubs (warmth≤0.25, density≤0.25) for XOmnibus.",
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
        help="Print what would be written without creating any files.",
    )
    parser.add_argument(
        "--count",
        type=int,
        default=3,
        metavar="N",
        help="Number of presets per engine (default: 3).",
    )
    parser.add_argument(
        "--engines",
        type=str,
        default=None,
        help=(
            "Comma-separated engine short names to generate "
            f"(e.g. ODYSSEY,OPTIC). Available: {', '.join(ALL_ENGINES)}"
        ),
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
    requested = [e.strip().upper() for e in engines_arg.split(",") if e.strip()]
    bad = [e for e in requested if e not in ENGINE_DEFS]
    if bad:
        print(f"ERROR: Unknown engine(s): {', '.join(bad)}", file=sys.stderr)
        print(f"       Available: {', '.join(ALL_ENGINES)}", file=sys.stderr)
        sys.exit(1)
    return requested


def main() -> None:
    args = parse_args()
    rng = random.Random(args.seed)

    engines = resolve_engines(args.engines)
    count = max(1, args.count)
    output_dir = args.output_dir

    if not args.dry_run:
        output_dir.mkdir(parents=True, exist_ok=True)

    total_written = 0
    total_skipped = 0
    seen_names: set = set()

    target = len(engines) * count

    print("XOmnibus Cold Sparse Preset Pack")
    print(f"  Quadrant    : warmth 0.0–0.25, density 0.0–0.25")
    print(f"  Mood        : Prism")
    print(f"  Engines     : {', '.join(engines)}")
    print(f"  Per engine  : {count}")
    print(f"  Total target: {target}")
    print(f"  Output dir  : {output_dir}")
    print(f"  Dry run     : {args.dry_run}")
    if args.seed is not None:
        print(f"  Seed        : {args.seed}")
    print()

    for engine_key in engines:
        print(f"  [{engine_key}]")
        for i in range(count):
            raw_name = _generate_name(rng)
            preset_name = _make_unique_name(raw_name, seen_names, rng)
            seen_names.add(preset_name)

            preset = build_preset(engine_key, preset_name, rng)

            safe_name = preset_name.replace("/", "-").replace("\\", "-")
            filename = f"{safe_name}.xometa"
            filepath = output_dir / filename

            dna = preset["dna"]
            dna_summary = (
                f"warmth={dna['warmth']:.2f} density={dna['density']:.2f} "
                f"brightness={dna['brightness']:.2f}"
            )

            if args.dry_run:
                print(f"    [DRY RUN] {filename}  ({dna_summary})")
                total_written += 1
                continue

            if filepath.exists():
                print(f"    SKIP (exists): {filename}")
                total_skipped += 1
                continue

            with open(filepath, "w", encoding="utf-8") as fh:
                json.dump(preset, fh, indent=2, ensure_ascii=False)
                fh.write("\n")

            print(f"    WRITE: {filename}  ({dna_summary})")
            total_written += 1

    print()
    if args.dry_run:
        print(f"Dry run complete. Would write {total_written} preset(s).")
    else:
        print(f"Done. Wrote {total_written} preset(s), skipped {total_skipped} existing.")

    sys.exit(0)


if __name__ == "__main__":
    main()
