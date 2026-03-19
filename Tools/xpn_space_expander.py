#!/usr/bin/env python3
"""
xpn_space_expander.py

Generates presets at both extremes of the space dimension to improve XOmnibus fleet
diversity. The fleet DNA report found "space" is moderate in distribution; this tool
fills the intimate (0.0–0.15) and vast (0.85–1.0) extremes.

Two bands:
  intimate  — close-mic, dry, zero reverb, in-your-face (space 0.0–0.15)
  vast      — infinite reverb, spatial depth, room/hall character (space 0.85–1.0)

Intimate engines: Onset, Oblong, Obese, Overbite, Overworld, Origami, Oblique
Vast engines    : Opal, Oracle, Overdub, Oceanic, Osprey, Owlfish, Organon

Usage:
    python3 Tools/xpn_space_expander.py
    python3 Tools/xpn_space_expander.py --dry-run
    python3 Tools/xpn_space_expander.py --band intimate
    python3 Tools/xpn_space_expander.py --band vast --count 6
    python3 Tools/xpn_space_expander.py --engines Onset,Opal --seed 42
    python3 Tools/xpn_space_expander.py --output-dir /tmp/test_presets --dry-run
"""

import argparse
import json
import random
import sys
from pathlib import Path
from typing import Optional

# ---------------------------------------------------------------------------
# Name vocabulary
# ---------------------------------------------------------------------------

INTIMATE_WORDS = [
    "Close", "Present", "Dry", "Direct", "Face", "Booth", "Pocket",
    "Contact", "Immediate", "Point Blank", "Anechoic", "Dead Room", "Live Wire",
]

INTIMATE_SUFFIXES = [
    "Session", "Take", "Signal", "Feed", "Source", "Cut", "Hit",
    "Strike", "Chain", "Loop", "Pulse", "Burst", "Edge", "Core",
]

VAST_WORDS = [
    "Cathedral", "Infinite", "Distant", "Beyond", "Cosmos", "Expanse",
    "Void Hall", "Horizon", "Cosmic", "Vast Chamber", "Abyss Hall", "Deep Space",
]

VAST_SUFFIXES = [
    "Drift", "Bloom", "Echo", "Field", "Wash", "Reverb", "Shimmer",
    "Expanse", "Fold", "Cloud", "Mist", "Pool", "Haze", "Veil",
]

INTIMATE_TAGS = ["dry", "close", "intimate", "direct", "present", "booth"]
VAST_TAGS     = ["vast", "reverb", "spatial", "hall", "infinite", "atmospheric"]

# ---------------------------------------------------------------------------
# Band definitions
# ---------------------------------------------------------------------------

BANDS = {
    "intimate": (0.0,  0.15),
    "vast":     (0.85, 1.0),
}

# mood per band
BAND_MOOD = {
    "intimate": "Flux",
    "vast":     "Aether",
}

# DNA constraints per band
BAND_DNA_CONSTRAINTS = {
    "intimate": {
        "density_range":    (0.6, 0.9),
        "aggression_range": (0.5, 0.85),
        "movement_range":   (0.5, 0.9),
    },
    "vast": {
        "density_range":    (0.3, 0.5),
        "aggression_range": (0.2, 0.4),
        "movement_range":   (0.2, 0.6),
    },
}

# ---------------------------------------------------------------------------
# Engine definitions
# ---------------------------------------------------------------------------
# intimate engines: forward, present, transient-focused
# vast engines: reverberant, spatial, ambient

ENGINE_DEFS = {

    # ---- INTIMATE ----

    "Onset": {
        "display": "Onset",
        "engines_key": "Onset",
        "band": "intimate",
        "brightness_range": (0.55, 0.85),
        "macro_labels": ["MACHINE", "PUNCH", "SPACE", "MUTATE"],
        "description_flavor": "Close-mic percussion — PUNCH drives transient impact at zero distance, MACHINE locks the algorithm to attack-forward mode.",
        "tags_extra": ["percussion", "drums", "transient"],
        "param_template": lambda rng: {
            "perc_v1_blend":     round(rng.uniform(0.5, 1.0), 3),
            "perc_v1_algoMode":  rng.randint(2, 5),
            "perc_v1_pitch":     round(rng.uniform(0.3, 0.75), 3),
            "perc_v1_decay":     round(rng.uniform(0.03, 0.25), 3),
            "perc_v1_tone":      round(rng.uniform(0.6, 1.0), 3),
            "perc_v1_snap":      round(rng.uniform(0.7, 1.0), 3),
            "perc_v1_body":      round(rng.uniform(0.5, 0.95), 3),
            "perc_v1_character": round(rng.uniform(0.5, 1.0), 3),
            "perc_v1_level":     round(rng.uniform(0.8, 1.0), 3),
            "perc_v1_pan":       round(rng.uniform(-0.05, 0.05), 3),
            "perc_v2_blend":     round(rng.uniform(0.4, 0.9), 3),
            "perc_v2_algoMode":  rng.randint(0, 5),
            "perc_v2_pitch":     round(rng.uniform(0.4, 0.85), 3),
            "perc_v2_decay":     round(rng.uniform(0.03, 0.2), 3),
            "perc_v2_tone":      round(rng.uniform(0.5, 1.0), 3),
            "perc_v2_snap":      round(rng.uniform(0.6, 1.0), 3),
            "perc_v2_body":      round(rng.uniform(0.4, 0.9), 3),
            "perc_v2_character": round(rng.uniform(0.4, 0.95), 3),
            "perc_v2_level":     round(rng.uniform(0.75, 1.0), 3),
            "perc_v2_pan":       round(rng.uniform(-0.1, 0.1), 3),
            "perc_level":        round(rng.uniform(0.8, 1.0), 3),
            "perc_drive":        round(rng.uniform(0.55, 1.0), 3),
            "perc_masterTone":   round(rng.uniform(0.55, 0.95), 3),
            "perc_reverbSend":   round(rng.uniform(0.0, 0.05), 3),
            "perc_delayMix":     round(rng.uniform(0.0, 0.04), 3),
        },
    },

    "Oblong": {
        "display": "Oblong",
        "engines_key": "Oblong",
        "band": "intimate",
        "brightness_range": (0.45, 0.75),
        "macro_labels": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
        "description_flavor": "Close-contact bass synth — dry body with immediate attack, zero ambience, pure fundamental presence.",
        "tags_extra": ["bass", "synth", "present", "punchy"],
        "param_template": lambda rng: {
            "bob_fltCutoff":    round(rng.uniform(0.3, 0.75), 3),
            "bob_fltReso":      round(rng.uniform(0.2, 0.6), 3),
            "bob_fltEnvAmt":    round(rng.uniform(0.3, 0.8), 3),
            "bob_fltEnvAttack": round(rng.uniform(0.0, 0.008), 4),
            "bob_fltEnvDecay":  round(rng.uniform(0.05, 0.25), 3),
            "bob_fltEnvSustain":round(rng.uniform(0.3, 0.75), 3),
            "bob_fltEnvRelease":round(rng.uniform(0.05, 0.2), 3),
            "bob_ampAttack":    round(rng.uniform(0.0, 0.006), 4),
            "bob_ampDecay":     round(rng.uniform(0.05, 0.3), 3),
            "bob_ampSustain":   round(rng.uniform(0.5, 0.95), 3),
            "bob_ampRelease":   round(rng.uniform(0.05, 0.2), 3),
            "bob_oscWave":      rng.randint(0, 3),
            "bob_oscDetune":    round(rng.uniform(0.0, 0.15), 3),
            "bob_subLevel":     round(rng.uniform(0.5, 1.0), 3),
            "bob_drive":        round(rng.uniform(0.4, 0.85), 3),
            "bob_level":        round(rng.uniform(0.8, 1.0), 3),
            "bob_reverbSend":   round(rng.uniform(0.0, 0.04), 3),
            "bob_delayMix":     round(rng.uniform(0.0, 0.03), 3),
        },
    },

    "Obese": {
        "display": "Obese",
        "engines_key": "Obese",
        "band": "intimate",
        "brightness_range": (0.3, 0.6),
        "macro_labels": ["MORPH", "MOJO", "DRIVE", "CRUSH"],
        "description_flavor": "In-your-face sub assault — MOJO maxed, dry signal chain, no reverb tail, pure impact.",
        "tags_extra": ["bass", "sub", "fat", "dry"],
        "param_template": lambda rng: {
            "fat_morph":        round(rng.uniform(0.4, 0.9), 3),
            "fat_mojo":         round(rng.uniform(0.65, 1.0), 3),
            "fat_subLevel":     round(rng.uniform(0.7, 1.0), 3),
            "fat_subOct":       rng.choice([-2, -1]),
            "fat_groupMix":     round(rng.uniform(0.45, 0.9), 3),
            "fat_detune":       round(rng.uniform(0.05, 0.35), 3),
            "fat_stereoWidth":  round(rng.uniform(0.1, 0.45), 3),
            "fat_ampAttack":    round(rng.uniform(0.0, 0.008), 4),
            "fat_ampDecay":     round(rng.uniform(0.08, 0.3), 3),
            "fat_ampSustain":   round(rng.uniform(0.55, 0.95), 3),
            "fat_ampRelease":   round(rng.uniform(0.06, 0.25), 3),
            "fat_fltCutoff":    round(rng.uniform(0.35, 0.8), 3),
            "fat_fltReso":      round(rng.uniform(0.25, 0.65), 3),
            "fat_fltDrive":     round(rng.uniform(0.55, 1.0), 3),
            "fat_fltKeyTrack":  round(rng.uniform(0.3, 0.8), 3),
            "fat_fltEnvAmt":    round(rng.uniform(0.4, 0.9), 3),
            "fat_fltEnvAttack": round(rng.uniform(0.0, 0.01), 4),
            "fat_fltEnvDecay":  round(rng.uniform(0.06, 0.25), 3),
            "fat_satDrive":     round(rng.uniform(0.55, 1.0), 3),
            "fat_crushDepth":   round(rng.uniform(0.2, 0.7), 3),
            "fat_crushRate":    round(rng.uniform(0.2, 0.7), 3),
            "fat_level":        round(rng.uniform(0.8, 1.0), 3),
            "fat_reverbSend":   round(rng.uniform(0.0, 0.04), 3),
            "fat_delayMix":     round(rng.uniform(0.0, 0.03), 3),
        },
    },

    "Overbite": {
        "display": "Overbite",
        "engines_key": "Overbite",
        "band": "intimate",
        "brightness_range": (0.5, 0.8),
        "macro_labels": ["BELLY", "BITE", "SCURRY", "TRASH"],
        "description_flavor": "Booth-recorded bite synth — BITE snaps with close-mic ferocity, zero room ambience.",
        "tags_extra": ["bite", "attack", "direct", "character"],
        "param_template": lambda rng: {
            "poss_biteDepth":   round(rng.uniform(0.6, 1.0), 3),
            "poss_biteRate":    round(rng.uniform(0.4, 0.9), 3),
            "poss_belly":       round(rng.uniform(0.5, 1.0), 3),
            "poss_scurry":      round(rng.uniform(0.4, 0.9), 3),
            "poss_trash":       round(rng.uniform(0.3, 0.8), 3),
            "poss_playDead":    round(rng.uniform(0.0, 0.3), 3),
            "poss_fltCutoff":   round(rng.uniform(0.4, 0.85), 3),
            "poss_fltReso":     round(rng.uniform(0.2, 0.6), 3),
            "poss_ampAttack":   round(rng.uniform(0.0, 0.008), 4),
            "poss_ampDecay":    round(rng.uniform(0.05, 0.28), 3),
            "poss_ampSustain":  round(rng.uniform(0.45, 0.9), 3),
            "poss_ampRelease":  round(rng.uniform(0.05, 0.22), 3),
            "poss_drive":       round(rng.uniform(0.4, 0.85), 3),
            "poss_level":       round(rng.uniform(0.8, 1.0), 3),
            "poss_reverbSend":  round(rng.uniform(0.0, 0.04), 3),
            "poss_stereoWidth": round(rng.uniform(0.05, 0.35), 3),
        },
    },

    "Overworld": {
        "display": "Overworld",
        "engines_key": "Overworld",
        "band": "intimate",
        "brightness_range": (0.6, 0.95),
        "macro_labels": ["ERA", "GLITCH", "CRUSH", "CHAOS"],
        "description_flavor": "Booth-dry chip synthesis — bit-crushed NES/YM2612 with no spatial tail, raw digital texture direct to channel.",
        "tags_extra": ["chiptune", "bitcrush", "dry", "direct"],
        "param_template": lambda rng: {
            "ow_era":           round(rng.uniform(0.0, 1.0), 3),
            "ow_eraY":          round(rng.uniform(0.0, 1.0), 3),
            "ow_voiceMode":     rng.randint(0, 3),
            "ow_masterVol":     round(rng.uniform(0.8, 1.0), 3),
            "ow_masterTune":    0.0,
            "ow_pulseDuty":     round(rng.uniform(0.1, 0.5), 3),
            "ow_pulseSweep":    round(rng.uniform(0.3, 0.9), 3),
            "ow_triEnable":     rng.randint(0, 1),
            "ow_noiseMode":     rng.randint(0, 2),
            "ow_noisePeriod":   round(rng.uniform(0.3, 0.9), 3),
            "ow_nesMix":        round(rng.uniform(0.5, 1.0), 3),
            "ow_fmAlgorithm":   rng.randint(0, 7),
            "ow_fmFeedback":    round(rng.uniform(0.5, 1.0), 3),
            "ow_fmOp1Level":    round(rng.uniform(0.6, 1.0), 3),
            "ow_fmOp2Level":    round(rng.uniform(0.5, 1.0), 3),
            "ow_fmOp3Level":    round(rng.uniform(0.4, 0.9), 3),
            "ow_fmOp4Level":    round(rng.uniform(0.3, 0.85), 3),
            "ow_fmOp1Mult":     rng.randint(1, 8),
            "ow_fmOp2Mult":     rng.randint(1, 8),
            "ow_fmOp3Mult":     rng.randint(1, 8),
            "ow_fmOp4Mult":     rng.randint(1, 8),
            "ow_echoDelay":     round(rng.uniform(0.0, 0.05), 3),
            "ow_echoFeedback":  round(rng.uniform(0.0, 0.05), 3),
            "ow_echoMix":       round(rng.uniform(0.0, 0.03), 3),
        },
    },

    "Origami": {
        "display": "Origami",
        "engines_key": "Origami",
        "band": "intimate",
        "brightness_range": (0.5, 0.8),
        "macro_labels": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
        "description_flavor": "Direct fold physics — each crease articulated at close range, no room smear, paper contact mic proximity.",
        "tags_extra": ["physical", "fold", "intimate", "articulate"],
        "param_template": lambda rng: {
            "origami_foldPoint":  round(rng.uniform(0.3, 0.8), 3),
            "origami_foldAngle":  round(rng.uniform(0.2, 0.9), 3),
            "origami_tension":    round(rng.uniform(0.4, 0.9), 3),
            "origami_crease":     round(rng.uniform(0.5, 1.0), 3),
            "origami_layerCount": rng.randint(1, 5),
            "origami_layerSpread":round(rng.uniform(0.1, 0.5), 3),
            "origami_brightness": round(rng.uniform(0.5, 0.9), 3),
            "origami_ampAttack":  round(rng.uniform(0.0, 0.006), 4),
            "origami_ampDecay":   round(rng.uniform(0.04, 0.25), 3),
            "origami_ampSustain": round(rng.uniform(0.4, 0.9), 3),
            "origami_ampRelease": round(rng.uniform(0.04, 0.2), 3),
            "origami_fltCutoff":  round(rng.uniform(0.45, 0.9), 3),
            "origami_fltReso":    round(rng.uniform(0.2, 0.6), 3),
            "origami_drive":      round(rng.uniform(0.3, 0.75), 3),
            "origami_level":      round(rng.uniform(0.8, 1.0), 3),
            "origami_reverbSend": round(rng.uniform(0.0, 0.04), 3),
            "origami_stereoWidth":round(rng.uniform(0.05, 0.3), 3),
        },
    },

    "Oblique": {
        "display": "Oblique",
        "engines_key": "Oblique",
        "band": "intimate",
        "brightness_range": (0.55, 0.85),
        "macro_labels": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
        "description_flavor": "Prism-bounce in a dead room — prismatic reflections without reverb tail, maximum transient clarity.",
        "tags_extra": ["prism", "bounce", "direct", "dry"],
        "param_template": lambda rng: {
            "oblq_prismColor":  round(rng.uniform(0.3, 0.9), 3),
            "oblq_bounceRate":  round(rng.uniform(0.4, 1.0), 3),
            "oblq_bounceDecay": round(rng.uniform(0.05, 0.3), 3),
            "oblq_angle":       round(rng.uniform(0.1, 0.9), 3),
            "oblq_refraction":  round(rng.uniform(0.3, 0.85), 3),
            "oblq_dispersion":  round(rng.uniform(0.2, 0.75), 3),
            "oblq_fltCutoff":   round(rng.uniform(0.4, 0.9), 3),
            "oblq_fltReso":     round(rng.uniform(0.2, 0.6), 3),
            "oblq_ampAttack":   round(rng.uniform(0.0, 0.007), 4),
            "oblq_ampDecay":    round(rng.uniform(0.05, 0.28), 3),
            "oblq_ampSustain":  round(rng.uniform(0.45, 0.9), 3),
            "oblq_ampRelease":  round(rng.uniform(0.04, 0.22), 3),
            "oblq_drive":       round(rng.uniform(0.35, 0.8), 3),
            "oblq_level":       round(rng.uniform(0.8, 1.0), 3),
            "oblq_reverbSend":  round(rng.uniform(0.0, 0.04), 3),
            "oblq_stereoWidth": round(rng.uniform(0.05, 0.35), 3),
        },
    },

    # ---- VAST ----

    "Opal": {
        "display": "Opal",
        "engines_key": "Opal",
        "band": "vast",
        "brightness_range": (0.3, 0.65),
        "macro_labels": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
        "description_flavor": "Infinite granular cathedral — grain clouds dissolve into hall reverb, spatial depth exceeds the physical room.",
        "tags_extra": ["granular", "ambient", "reverb", "spatial"],
        "param_template": lambda rng: {
            "opal_grainSize":    round(rng.uniform(0.2, 0.9), 3),
            "opal_grainDensity": round(rng.uniform(0.4, 1.0), 3),
            "opal_grainPitch":   round(rng.uniform(-0.3, 0.3), 3),
            "opal_grainSpread":  round(rng.uniform(0.5, 1.0), 3),
            "opal_grainScan":    round(rng.uniform(0.0, 1.0), 3),
            "opal_grainShape":   rng.randint(0, 4),
            "opal_grainMix":     round(rng.uniform(0.6, 1.0), 3),
            "opal_reverbSize":   round(rng.uniform(0.75, 1.0), 3),
            "opal_reverbDamp":   round(rng.uniform(0.1, 0.5), 3),
            "opal_reverbMix":    round(rng.uniform(0.6, 0.95), 3),
            "opal_reverbDecay":  round(rng.uniform(0.7, 1.0), 3),
            "opal_delayTime":    round(rng.uniform(0.3, 0.8), 3),
            "opal_delayFeedback":round(rng.uniform(0.4, 0.85), 3),
            "opal_delayMix":     round(rng.uniform(0.2, 0.55), 3),
            "opal_fltCutoff":    round(rng.uniform(0.2, 0.65), 3),
            "opal_fltReso":      round(rng.uniform(0.1, 0.5), 3),
            "opal_ampAttack":    round(rng.uniform(0.2, 1.5), 3),
            "opal_ampDecay":     round(rng.uniform(0.3, 1.0), 3),
            "opal_ampSustain":   round(rng.uniform(0.5, 0.9), 3),
            "opal_ampRelease":   round(rng.uniform(0.8, 3.0), 3),
            "opal_level":        round(rng.uniform(0.7, 0.95), 3),
            "opal_stereoWidth":  round(rng.uniform(0.7, 1.0), 3),
        },
    },

    "Oracle": {
        "display": "Oracle",
        "engines_key": "Oracle",
        "band": "vast",
        "brightness_range": (0.25, 0.55),
        "macro_labels": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
        "description_flavor": "Stochastic prophecy in endless hall — GENDY synthesis dissolves across maqam scale space into infinite reverb.",
        "tags_extra": ["stochastic", "ambient", "maqam", "reverb"],
        "param_template": lambda rng: {
            "oracle_breakpoints": rng.randint(4, 16),
            "oracle_stochRange":  round(rng.uniform(0.3, 0.9), 3),
            "oracle_stochRate":   round(rng.uniform(0.05, 0.4), 3),
            "oracle_maqamRoot":   rng.randint(0, 11),
            "oracle_maqamMode":   rng.randint(0, 7),
            "oracle_maqamDepth":  round(rng.uniform(0.4, 0.9), 3),
            "oracle_prophetMix":  round(rng.uniform(0.5, 1.0), 3),
            "oracle_reverbSize":  round(rng.uniform(0.8, 1.0), 3),
            "oracle_reverbDamp":  round(rng.uniform(0.05, 0.35), 3),
            "oracle_reverbMix":   round(rng.uniform(0.65, 0.95), 3),
            "oracle_reverbDecay": round(rng.uniform(0.75, 1.0), 3),
            "oracle_delayTime":   round(rng.uniform(0.3, 0.9), 3),
            "oracle_delayFeedback":round(rng.uniform(0.45, 0.85), 3),
            "oracle_delayMix":    round(rng.uniform(0.25, 0.6), 3),
            "oracle_fltCutoff":   round(rng.uniform(0.15, 0.6), 3),
            "oracle_fltReso":     round(rng.uniform(0.1, 0.45), 3),
            "oracle_ampAttack":   round(rng.uniform(0.3, 2.0), 3),
            "oracle_ampRelease":  round(rng.uniform(1.0, 4.0), 3),
            "oracle_level":       round(rng.uniform(0.65, 0.9), 3),
            "oracle_stereoWidth": round(rng.uniform(0.7, 1.0), 3),
        },
    },

    "Overdub": {
        "display": "Overdub",
        "engines_key": "Overdub",
        "band": "vast",
        "brightness_range": (0.3, 0.6),
        "macro_labels": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
        "description_flavor": "Dub reverb cathedral — spring + tape delay cascade into vast hall, Vangelis spatial philosophy.",
        "tags_extra": ["dub", "reverb", "spring", "tape"],
        "param_template": lambda rng: {
            "dub_oscWave":      rng.randint(0, 4),
            "dub_oscDetune":    round(rng.uniform(-0.2, 0.2), 3),
            "dub_oscPW":        round(rng.uniform(0.2, 0.8), 3),
            "dub_fltCutoff":    round(rng.uniform(0.2, 0.65), 3),
            "dub_fltReso":      round(rng.uniform(0.1, 0.5), 3),
            "dub_fltEnvAmt":    round(rng.uniform(0.1, 0.5), 3),
            "dub_ampAttack":    round(rng.uniform(0.1, 1.0), 3),
            "dub_ampDecay":     round(rng.uniform(0.2, 0.8), 3),
            "dub_ampSustain":   round(rng.uniform(0.4, 0.85), 3),
            "dub_ampRelease":   round(rng.uniform(0.5, 3.0), 3),
            "dub_sendAmount":   round(rng.uniform(0.6, 1.0), 3),
            "dub_driveAmount":  round(rng.uniform(0.1, 0.4), 3),
            "dub_tapeDelay":    round(rng.uniform(0.3, 0.8), 3),
            "dub_tapeFeedback": round(rng.uniform(0.5, 0.9), 3),
            "dub_tapeMix":      round(rng.uniform(0.4, 0.8), 3),
            "dub_springSize":   round(rng.uniform(0.7, 1.0), 3),
            "dub_springMix":    round(rng.uniform(0.5, 0.95), 3),
            "dub_springDecay":  round(rng.uniform(0.6, 1.0), 3),
            "dub_reverbSize":   round(rng.uniform(0.8, 1.0), 3),
            "dub_reverbMix":    round(rng.uniform(0.55, 0.9), 3),
            "dub_level":        round(rng.uniform(0.65, 0.9), 3),
            "dub_stereoWidth":  round(rng.uniform(0.65, 1.0), 3),
        },
    },

    "Oceanic": {
        "display": "Oceanic",
        "engines_key": "Oceanic",
        "band": "vast",
        "brightness_range": (0.2, 0.55),
        "macro_labels": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
        "description_flavor": "Water column infinity — chromatophore modulations dissolve into oceanic hall, phosphorescent spatial depth.",
        "tags_extra": ["ambient", "water", "oceanic", "reverb"],
        "param_template": lambda rng: {
            "ocean_separation":  round(rng.uniform(0.5, 1.0), 3),
            "ocean_depth":       round(rng.uniform(0.6, 1.0), 3),
            "ocean_pressure":    round(rng.uniform(0.3, 0.8), 3),
            "ocean_current":     round(rng.uniform(0.2, 0.7), 3),
            "ocean_biolum":      round(rng.uniform(0.4, 1.0), 3),
            "ocean_chromaRate":  round(rng.uniform(0.05, 0.4), 3),
            "ocean_chromaDepth": round(rng.uniform(0.4, 0.9), 3),
            "ocean_fltCutoff":   round(rng.uniform(0.15, 0.55), 3),
            "ocean_fltReso":     round(rng.uniform(0.1, 0.45), 3),
            "ocean_ampAttack":   round(rng.uniform(0.3, 2.0), 3),
            "ocean_ampRelease":  round(rng.uniform(1.0, 5.0), 3),
            "ocean_reverbSize":  round(rng.uniform(0.8, 1.0), 3),
            "ocean_reverbDamp":  round(rng.uniform(0.05, 0.3), 3),
            "ocean_reverbMix":   round(rng.uniform(0.6, 0.95), 3),
            "ocean_reverbDecay": round(rng.uniform(0.75, 1.0), 3),
            "ocean_delayTime":   round(rng.uniform(0.4, 0.9), 3),
            "ocean_delayFeedback":round(rng.uniform(0.4, 0.85), 3),
            "ocean_delayMix":    round(rng.uniform(0.2, 0.55), 3),
            "ocean_level":       round(rng.uniform(0.65, 0.9), 3),
            "ocean_stereoWidth": round(rng.uniform(0.75, 1.0), 3),
        },
    },

    "Osprey": {
        "display": "Osprey",
        "engines_key": "Osprey",
        "band": "vast",
        "brightness_range": (0.35, 0.65),
        "macro_labels": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
        "description_flavor": "Coastal hall — ShoreSystem cultural data mapped to infinite sky reverb, distance and horizon character.",
        "tags_extra": ["coastal", "ambient", "reverb", "horizon"],
        "param_template": lambda rng: {
            "osprey_shoreBlend":   round(rng.uniform(0.4, 1.0), 3),
            "osprey_coastLine":    rng.randint(0, 4),
            "osprey_windSpeed":    round(rng.uniform(0.2, 0.8), 3),
            "osprey_tidePhase":    round(rng.uniform(0.0, 1.0), 3),
            "osprey_saltiness":    round(rng.uniform(0.3, 0.9), 3),
            "osprey_breathRate":   round(rng.uniform(0.02, 0.2), 3),
            "osprey_breathDepth":  round(rng.uniform(0.3, 0.8), 3),
            "osprey_fltCutoff":    round(rng.uniform(0.2, 0.6), 3),
            "osprey_fltReso":      round(rng.uniform(0.1, 0.45), 3),
            "osprey_ampAttack":    round(rng.uniform(0.3, 2.0), 3),
            "osprey_ampRelease":   round(rng.uniform(0.8, 4.0), 3),
            "osprey_reverbSize":   round(rng.uniform(0.8, 1.0), 3),
            "osprey_reverbDamp":   round(rng.uniform(0.05, 0.35), 3),
            "osprey_reverbMix":    round(rng.uniform(0.6, 0.95), 3),
            "osprey_reverbDecay":  round(rng.uniform(0.7, 1.0), 3),
            "osprey_delayTime":    round(rng.uniform(0.35, 0.85), 3),
            "osprey_delayFeedback":round(rng.uniform(0.4, 0.85), 3),
            "osprey_delayMix":     round(rng.uniform(0.2, 0.55), 3),
            "osprey_level":        round(rng.uniform(0.65, 0.9), 3),
            "osprey_stereoWidth":  round(rng.uniform(0.7, 1.0), 3),
        },
    },

    "Owlfish": {
        "display": "Owlfish",
        "engines_key": "Owlfish",
        "band": "vast",
        "brightness_range": (0.25, 0.6),
        "macro_labels": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
        "description_flavor": "Mixtur-Trautonium in the abyss — formant subharmonic series dissolves into deep void hall reverb.",
        "tags_extra": ["subharmonic", "formant", "ambient", "reverb"],
        "param_template": lambda rng: {
            "owl_filterCutoff":  round(rng.uniform(0.15, 0.55), 3),
            "owl_filterReso":    round(rng.uniform(0.1, 0.5), 3),
            "owl_subharmIndex":  rng.randint(1, 8),
            "owl_subharmDepth":  round(rng.uniform(0.4, 0.9), 3),
            "owl_formantShift":  round(rng.uniform(0.2, 0.8), 3),
            "owl_formantBlend":  round(rng.uniform(0.3, 0.85), 3),
            "owl_oscFundLevel":  round(rng.uniform(0.3, 0.8), 3),
            "owl_oscSubLevel":   round(rng.uniform(0.4, 0.9), 3),
            "owl_ampAttack":     round(rng.uniform(0.2, 1.5), 3),
            "owl_ampDecay":      round(rng.uniform(0.3, 1.0), 3),
            "owl_ampSustain":    round(rng.uniform(0.5, 0.9), 3),
            "owl_ampRelease":    round(rng.uniform(0.8, 4.0), 3),
            "owl_reverbSize":    round(rng.uniform(0.82, 1.0), 3),
            "owl_reverbDamp":    round(rng.uniform(0.05, 0.3), 3),
            "owl_reverbMix":     round(rng.uniform(0.6, 0.95), 3),
            "owl_reverbDecay":   round(rng.uniform(0.75, 1.0), 3),
            "owl_delayTime":     round(rng.uniform(0.3, 0.8), 3),
            "owl_delayFeedback": round(rng.uniform(0.4, 0.85), 3),
            "owl_delayMix":      round(rng.uniform(0.2, 0.55), 3),
            "owl_level":         round(rng.uniform(0.65, 0.9), 3),
            "owl_stereoWidth":   round(rng.uniform(0.7, 1.0), 3),
        },
    },

    "Organon": {
        "display": "Organon",
        "engines_key": "Organon",
        "band": "vast",
        "brightness_range": (0.2, 0.5),
        "macro_labels": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
        "description_flavor": "Variational free energy in infinite reverb — metabolic rate slow-breathing across cosmic hall depth.",
        "tags_extra": ["organic", "ambient", "reverb", "metabolic"],
        "param_template": lambda rng: {
            "organon_metabolicRate": round(rng.uniform(0.02, 0.25), 3),
            "organon_entropy":       round(rng.uniform(0.2, 0.7), 3),
            "organon_cellCount":     rng.randint(4, 32),
            "organon_coupling":      round(rng.uniform(0.3, 0.8), 3),
            "organon_breathRate":    round(rng.uniform(0.01, 0.15), 3),
            "organon_breathDepth":   round(rng.uniform(0.3, 0.85), 3),
            "organon_fltCutoff":     round(rng.uniform(0.1, 0.5), 3),
            "organon_fltReso":       round(rng.uniform(0.05, 0.4), 3),
            "organon_ampAttack":     round(rng.uniform(0.5, 3.0), 3),
            "organon_ampRelease":    round(rng.uniform(1.5, 6.0), 3),
            "organon_reverbSize":    round(rng.uniform(0.85, 1.0), 3),
            "organon_reverbDamp":    round(rng.uniform(0.03, 0.25), 3),
            "organon_reverbMix":     round(rng.uniform(0.65, 0.95), 3),
            "organon_reverbDecay":   round(rng.uniform(0.8, 1.0), 3),
            "organon_delayTime":     round(rng.uniform(0.4, 0.9), 3),
            "organon_delayFeedback": round(rng.uniform(0.45, 0.85), 3),
            "organon_delayMix":      round(rng.uniform(0.2, 0.55), 3),
            "organon_level":         round(rng.uniform(0.6, 0.88), 3),
            "organon_stereoWidth":   round(rng.uniform(0.75, 1.0), 3),
        },
    },
}

# Ordered lists for --engines validation
INTIMATE_ENGINES = ["Onset", "Oblong", "Obese", "Overbite", "Overworld", "Origami", "Oblique"]
VAST_ENGINES     = ["Opal", "Oracle", "Overdub", "Oceanic", "Osprey", "Owlfish", "Organon"]
ALL_ENGINES      = INTIMATE_ENGINES + VAST_ENGINES

# ---------------------------------------------------------------------------
# Name generation
# ---------------------------------------------------------------------------

def _generate_name(engine_key: str, band: str, index: int, rng: random.Random) -> str:
    """Generate a preset name from the band-appropriate vocabulary."""
    if band == "intimate":
        word   = rng.choice(INTIMATE_WORDS)
        suffix = rng.choice(INTIMATE_SUFFIXES)
    else:
        word   = rng.choice(VAST_WORDS)
        suffix = rng.choice(VAST_SUFFIXES)
    return f"{word} {suffix}"


def _make_unique_name(base_name: str, seen_names: set, rng: random.Random) -> str:
    """Append disambiguation suffix if needed."""
    if base_name not in seen_names:
        return base_name
    for i in range(2, 25):
        candidate = f"{base_name} {i}"
        if candidate not in seen_names:
            return candidate
    return f"{base_name} {rng.randint(100, 999)}"


# ---------------------------------------------------------------------------
# DNA generation
# ---------------------------------------------------------------------------

def _generate_dna(engine_key: str, band: str, rng: random.Random) -> dict:
    """Generate 6D Sonic DNA appropriate for the space band."""
    space_lo, space_hi = BANDS[band]
    space = round(rng.uniform(space_lo, space_hi), 3)

    constraints = BAND_DNA_CONSTRAINTS[band]
    density    = round(rng.uniform(*constraints["density_range"]), 3)
    aggression = round(rng.uniform(*constraints["aggression_range"]), 3)
    movement   = round(rng.uniform(*constraints["movement_range"]), 3)

    br_lo, br_hi = ENGINE_DEFS[engine_key]["brightness_range"]
    brightness = round(rng.uniform(br_lo, br_hi), 3)

    # warmth: intimate = lower (punchy, cold); vast = higher (warm, diffuse)
    if band == "intimate":
        warmth = round(rng.uniform(0.2, 0.55), 3)
    else:
        warmth = round(rng.uniform(0.4, 0.75), 3)

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

def build_preset(engine_key: str, band: str, preset_name: str, rng: random.Random) -> dict:
    """Build a complete .xometa preset dict."""
    engine_def = ENGINE_DEFS[engine_key]
    dna = _generate_dna(engine_key, band, rng)
    mood = BAND_MOOD[band]

    band_tags = INTIMATE_TAGS if band == "intimate" else VAST_TAGS
    tags = list(band_tags) + list(engine_def["tags_extra"])
    rng.shuffle(tags)

    params = engine_def["param_template"](rng)

    description = (
        f"{engine_def['description_flavor']} "
        f"Space {dna['space']:.2f} / Density {dna['density']:.2f} / "
        f"Aggression {dna['aggression']:.2f}. Band: {band}."
    )

    return {
        "schema_version": 1,
        "name": preset_name,
        "mood": mood,
        "sonic_dna": dna,
        "engines": [engine_def["engines_key"]],
        "author": "XO_OX Space Expander",
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

    parser = argparse.ArgumentParser(
        description="Generate space-extreme preset stubs for XOmnibus engines.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=None,
        help="Directory to write .xometa files (default: auto-selected per band: Presets/XOmnibus/Flux or Aether)",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print what would be written without creating files.",
    )
    parser.add_argument(
        "--band",
        choices=["intimate", "vast", "both"],
        default="both",
        help="Which space band to generate (default: both).",
    )
    parser.add_argument(
        "--engines",
        type=str,
        default=None,
        help=(
            "Comma-separated engine names to generate. "
            f"Intimate: {', '.join(INTIMATE_ENGINES)}. "
            f"Vast: {', '.join(VAST_ENGINES)}."
        ),
    )
    parser.add_argument(
        "--count",
        type=int,
        default=4,
        metavar="N",
        help="Number of presets per engine per band (default: 4).",
    )
    parser.add_argument(
        "--seed",
        type=int,
        default=None,
        help="Random seed for reproducible output.",
    )
    return parser.parse_args(), repo_root


def resolve_engines(engines_arg, band_filter: str) -> list:
    """Validate and return list of engine keys to generate."""
    if band_filter == "intimate":
        eligible = INTIMATE_ENGINES
    elif band_filter == "vast":
        eligible = VAST_ENGINES
    else:
        eligible = ALL_ENGINES

    if engines_arg is None:
        return list(eligible)

    requested = [e.strip() for e in engines_arg.split(",") if e.strip()]
    bad = [e for e in requested if e not in ALL_ENGINES]
    if bad:
        print(f"ERROR: Unknown engine(s): {', '.join(bad)}", file=sys.stderr)
        print(f"       Available: {', '.join(ALL_ENGINES)}", file=sys.stderr)
        sys.exit(1)

    # If band filter is active, warn about engines outside the band but still generate
    out_of_band = [e for e in requested if e not in eligible]
    if out_of_band and band_filter != "both":
        print(
            f"WARNING: Engines {', '.join(out_of_band)} are not in the '{band_filter}' band. "
            f"They will be generated using their own band definition.",
            file=sys.stderr,
        )

    return requested


def _output_dir_for_band(band: str, repo_root: Path, override: Optional[Path]) -> Path:
    if override is not None:
        return override
    mood = BAND_MOOD[band]
    return repo_root / "Presets" / "XOmnibus" / mood


def main() -> None:
    args, repo_root = parse_args()
    rng = random.Random(args.seed)

    band_filter = args.band
    if band_filter == "both":
        bands_to_run = ["intimate", "vast"]
    else:
        bands_to_run = [band_filter]

    engines = resolve_engines(args.engines, band_filter)
    count = max(1, args.count)

    print("XOmnibus Space Expander")
    print(f"  Bands       : {', '.join(bands_to_run)}")
    print(f"  Engines     : {', '.join(engines)}")
    print(f"  Count/band  : {count}")
    print(f"  Dry run     : {args.dry_run}")
    if args.seed is not None:
        print(f"  Seed        : {args.seed}")
    print()

    total_written  = 0
    total_skipped  = 0
    seen_names: set = set()

    for band in bands_to_run:
        # engines that belong to this band (or were explicitly requested)
        band_engines = [e for e in engines if ENGINE_DEFS[e]["band"] == band]
        if not band_engines:
            # If user explicitly requested engines from the other band, still generate
            band_engines = [e for e in engines if e in ALL_ENGINES]
            band_engines = [e for e in band_engines if ENGINE_DEFS.get(e, {}).get("band") == band]

        if not band_engines:
            continue

        out_dir = _output_dir_for_band(band, repo_root, args.output_dir)

        print(f"  [{band.upper()}]  →  {out_dir}")

        if not args.dry_run:
            out_dir.mkdir(parents=True, exist_ok=True)

        for engine_key in band_engines:
            engine_def = ENGINE_DEFS[engine_key]
            print(f"    [{engine_key}]")
            for i in range(count):
                raw_name    = _generate_name(engine_key, band, i, rng)
                preset_name = _make_unique_name(raw_name, seen_names, rng)
                seen_names.add(preset_name)

                preset = build_preset(engine_key, band, preset_name, rng)

                safe_name = preset_name.replace("/", "-").replace("\\", "-")
                filename  = f"{safe_name}.xometa"
                filepath  = out_dir / filename

                if args.dry_run:
                    sp = preset["dna"]["space"]
                    de = preset["dna"]["density"]
                    ag = preset["dna"]["aggression"]
                    print(
                        f"      [DRY] {filename}  "
                        f"space={sp:.3f}  density={de:.3f}  aggression={ag:.3f}"
                    )
                    total_written += 1
                    continue

                if filepath.exists():
                    print(f"      [SKIP] {filename} (already exists)")
                    total_skipped += 1
                    continue

                with open(filepath, "w", encoding="utf-8") as fh:
                    json.dump(preset, fh, indent=2)
                    fh.write("\n")

                print(f"      [OK]   {filename}")
                total_written += 1

        print()

    label = "would write" if args.dry_run else "written"
    print(f"Done. {total_written} preset(s) {label}, {total_skipped} skipped.")
    sys.exit(0)


if __name__ == "__main__":
    main()
