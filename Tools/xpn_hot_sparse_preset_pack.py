#!/usr/bin/env python3
"""
xpn_hot_sparse_preset_pack.py

Generates Hot Sparse quadrant preset stubs (.xometa) for XOmnibus engines.

The warmth/density quadrant strategy needs "Hot Sparse" presets
(warmth 0.75–1.0, density 0.0–0.25) — intimate acoustic, solo cello character,
warm but skeletal. One voice, full of feeling, plenty of space around it.

Target engines: OVERDUB, OBBLIGATO, OHM, OSTERIA, OWLFISH, OCELOT,
                ORBITAL, ORGANON, OSPREY, ODDOSCAR

DNA targets:
  warmth      0.75 – 1.00   (intimate, analog, harmonically rich)
  density     0.00 – 0.25   (single voices, skeletal textures)
  brightness  0.35 – 0.60   (mid-warm, not bright, not dull)
  movement    0.35 – 0.55   (slow breath, gentle sway)
  space       0.55 – 0.75   (room-sized, not cavernous)
  aggression  0.10 – 0.35   (expressive but not violent)

Mood: Atmosphere (intimate, evocative)

Usage:
    python3 Tools/xpn_hot_sparse_preset_pack.py
    python3 Tools/xpn_hot_sparse_preset_pack.py --dry-run
    python3 Tools/xpn_hot_sparse_preset_pack.py --engines OVERDUB,OHM --count 5
    python3 Tools/xpn_hot_sparse_preset_pack.py --output-dir /tmp/hot_test --seed 42
"""

import argparse
import json
import os
import random
import sys
from pathlib import Path

# ---------------------------------------------------------------------------
# Name vocabulary — hot / sparse / intimate / acoustic aesthetic
# ---------------------------------------------------------------------------

HOT_NAMES = [
    "Solo",
    "Intimate",
    "Alone",
    "Ember",
    "Warm Breath",
    "Single Reed",
    "Soliloquy",
    "One Note",
    "Tender",
    "Slow Burn",
    "Hearth Tone",
    "Bare Warmth",
    "Still Flame",
    "Lone Voice",
    "Soft Body",
    "Warm Hollow",
    "Intimate Distance",
    "Single Bead",
]

HOT_TAGS_BASE = [
    "warm", "sparse", "intimate", "solo", "acoustic",
    "skeletal", "tender", "breath", "minimal", "expressive",
]

# ---------------------------------------------------------------------------
# Engine definitions
# ---------------------------------------------------------------------------

ENGINE_DEFS = {

    "OVERDUB": {
        "display": "Overdub",
        "engines_key": "Overdub",
        "macro_labels": ["SEND", "DRIVE", "TAPE", "SPRING"],
        "description_flavor": "A single bowed cello note drawn through warm spring reverb — SEND barely open, DRIVE at the edge of warmth, one breath, one room.",
        "tags_extra": ["dub", "spring-reverb", "cello", "warm-delay"],
        "param_template": lambda rng: {
            "dub_oscWave":          rng.randint(0, 1),
            "dub_oscLevel":         round(rng.uniform(0.65, 0.9), 3),
            "dub_oscDetune":        round(rng.uniform(-0.01, 0.01), 4),
            "dub_subLevel":         round(rng.uniform(0.05, 0.25), 3),
            "dub_noiseLevel":       round(rng.uniform(0.0, 0.08), 3),
            "dub_filterCutoff":     round(rng.uniform(0.35, 0.65), 3),
            "dub_filterReso":       round(rng.uniform(0.05, 0.3), 3),
            "dub_filterEnvAmt":     round(rng.uniform(0.05, 0.3), 3),
            "dub_ampAttack":        round(rng.uniform(0.05, 0.3), 3),
            "dub_ampDecay":         round(rng.uniform(0.1, 0.5), 3),
            "dub_ampSustain":       round(rng.uniform(0.65, 0.95), 3),
            "dub_ampRelease":       round(rng.uniform(0.4, 1.5), 3),
            "dub_filtEnvAttack":    round(rng.uniform(0.05, 0.25), 3),
            "dub_filtEnvDecay":     round(rng.uniform(0.1, 0.5), 3),
            "dub_filtEnvSustain":   round(rng.uniform(0.4, 0.8), 3),
            "dub_filtEnvRelease":   round(rng.uniform(0.3, 1.0), 3),
            "dub_lfo1Rate":         round(rng.uniform(0.01, 0.12), 4),
            "dub_lfo1Depth":        round(rng.uniform(0.05, 0.2), 3),
            "dub_lfo1Shape":        rng.randint(0, 2),
            "dub_sendAmount":       round(rng.uniform(0.05, 0.3), 3),
            "dub_driveAmount":      round(rng.uniform(0.1, 0.45), 3),
            "dub_tapeDelay":        round(rng.uniform(0.25, 0.6), 3),
            "dub_tapeFeedback":     round(rng.uniform(0.1, 0.4), 3),
            "dub_tapeMix":          round(rng.uniform(0.1, 0.35), 3),
            "dub_springMix":        round(rng.uniform(0.35, 0.65), 3),
            "dub_springDecay":      round(rng.uniform(0.4, 0.8), 3),
            "dub_masterVol":        round(rng.uniform(0.7, 0.9), 3),
        },
    },

    "OBBLIGATO": {
        "display": "Obbligato",
        "engines_key": "Obbligato",
        "macro_labels": ["BREATHE", "BOND", "TONE", "SPACE"],
        "description_flavor": "One wind voice in a corridor — single reed at rest, BREATHE barely moving, BOND at zero, warm tone filling the silence.",
        "tags_extra": ["wind", "reed", "breath", "dual-wind"],
        "param_template": lambda rng: {
            "obbl_breathA":         round(rng.uniform(0.3, 0.7), 3),
            "obbl_breathB":         round(rng.uniform(0.0, 0.15), 3),
            "obbl_breathNoise":     round(rng.uniform(0.1, 0.4), 3),
            "obbl_toneA":           round(rng.uniform(0.35, 0.65), 3),
            "obbl_toneB":           round(rng.uniform(0.0, 0.2), 3),
            "obbl_bondAmt":         round(rng.uniform(0.0, 0.1), 3),
            "obbl_voiceBalance":    round(rng.uniform(0.8, 1.0), 3),
            "obbl_filterCutoff":    round(rng.uniform(0.3, 0.6), 3),
            "obbl_filterReso":      round(rng.uniform(0.05, 0.25), 3),
            "obbl_ampAttack":       round(rng.uniform(0.06, 0.3), 3),
            "obbl_ampDecay":        round(rng.uniform(0.1, 0.5), 3),
            "obbl_ampSustain":      round(rng.uniform(0.65, 0.95), 3),
            "obbl_ampRelease":      round(rng.uniform(0.5, 1.8), 3),
            "obbl_lfo1Rate":        round(rng.uniform(0.01, 0.1), 4),
            "obbl_lfo1Depth":       round(rng.uniform(0.05, 0.2), 3),
            "obbl_vibRate":         round(rng.uniform(0.02, 0.1), 3),
            "obbl_vibDepth":        round(rng.uniform(0.02, 0.12), 3),
            "obbl_reverbMix":       round(rng.uniform(0.35, 0.65), 3),
            "obbl_reverbSize":      round(rng.uniform(0.4, 0.75), 3),
            "obbl_masterVol":       round(rng.uniform(0.7, 0.9), 3),
        },
    },

    "OHM": {
        "display": "Ohm",
        "engines_key": "Ohm",
        "macro_labels": ["MEDDLING", "COMMUNE", "DRIFT", "WARMTH"],
        "description_flavor": "One chord held in a warm room — MEDDLING at minimum, COMMUNE asleep, a single drone breathing at the edge of hearing.",
        "tags_extra": ["drone", "warm-pad", "commune", "hippy-dad"],
        "param_template": lambda rng: {
            "ohm_macroMeddling":     round(rng.uniform(0.0, 0.15), 3),
            "ohm_macroCommune":      round(rng.uniform(0.0, 0.1), 3),
            "ohm_osc1Wave":          rng.randint(0, 2),
            "ohm_osc1Level":         round(rng.uniform(0.6, 0.9), 3),
            "ohm_osc1Detune":        round(rng.uniform(-0.01, 0.01), 4),
            "ohm_osc2Wave":          rng.randint(0, 1),
            "ohm_osc2Level":         round(rng.uniform(0.0, 0.2), 3),
            "ohm_osc2Detune":        round(rng.uniform(-0.05, 0.05), 4),
            "ohm_subLevel":          round(rng.uniform(0.05, 0.25), 3),
            "ohm_filterCutoff":      round(rng.uniform(0.3, 0.6), 3),
            "ohm_filterReso":        round(rng.uniform(0.05, 0.3), 3),
            "ohm_filterEnvAmt":      round(rng.uniform(0.05, 0.25), 3),
            "ohm_ampAttack":         round(rng.uniform(0.08, 0.4), 3),
            "ohm_ampDecay":          round(rng.uniform(0.1, 0.5), 3),
            "ohm_ampSustain":        round(rng.uniform(0.7, 1.0), 3),
            "ohm_ampRelease":        round(rng.uniform(0.5, 2.0), 3),
            "ohm_lfo1Rate":          round(rng.uniform(0.008, 0.12), 4),
            "ohm_lfo1Depth":         round(rng.uniform(0.05, 0.2), 3),
            "ohm_lfo1Shape":         rng.randint(0, 2),
            "ohm_chorusMix":         round(rng.uniform(0.05, 0.25), 3),
            "ohm_reverbMix":         round(rng.uniform(0.3, 0.6), 3),
            "ohm_reverbSize":        round(rng.uniform(0.4, 0.7), 3),
            "ohm_masterVol":         round(rng.uniform(0.7, 0.9), 3),
        },
    },

    "OSTERIA": {
        "display": "Osteria",
        "engines_key": "Osteria",
        "macro_labels": ["SHORE", "WINE", "WARMTH", "DISTANCE"],
        "description_flavor": "A single porto wine string in a quiet trattoria — ShoreSystem at stillwater, bass coast alone, warm and full of longing.",
        "tags_extra": ["shore-system", "porto-wine", "string", "coastal"],
        "param_template": lambda rng: {
            "osteria_qBassShore":      rng.randint(0, 4),
            "osteria_shoreBlend":      round(rng.uniform(0.0, 0.2), 3),
            "osteria_bassLevel":       round(rng.uniform(0.6, 0.9), 3),
            "osteria_bassDetune":      round(rng.uniform(-0.01, 0.01), 4),
            "osteria_midLevel":        round(rng.uniform(0.0, 0.2), 3),
            "osteria_midDetune":       round(rng.uniform(-0.03, 0.03), 4),
            "osteria_filterCutoff":    round(rng.uniform(0.3, 0.6), 3),
            "osteria_filterReso":      round(rng.uniform(0.05, 0.3), 3),
            "osteria_filterEnvAmt":    round(rng.uniform(0.05, 0.25), 3),
            "osteria_ampAttack":       round(rng.uniform(0.04, 0.25), 3),
            "osteria_ampDecay":        round(rng.uniform(0.1, 0.5), 3),
            "osteria_ampSustain":      round(rng.uniform(0.65, 0.95), 3),
            "osteria_ampRelease":      round(rng.uniform(0.4, 1.5), 3),
            "osteria_lfo1Rate":        round(rng.uniform(0.01, 0.1), 4),
            "osteria_lfo1Depth":       round(rng.uniform(0.05, 0.18), 3),
            "osteria_lfo1Shape":       rng.randint(0, 2),
            "osteria_chorusMix":       round(rng.uniform(0.0, 0.2), 3),
            "osteria_reverbMix":       round(rng.uniform(0.3, 0.6), 3),
            "osteria_reverbSize":      round(rng.uniform(0.4, 0.7), 3),
            "osteria_masterVol":       round(rng.uniform(0.7, 0.9), 3),
        },
    },

    "OWLFISH": {
        "display": "Owlfish",
        "engines_key": "Owlfish",
        "macro_labels": ["STRIA", "SUBHARMONIC", "RESONANCE", "DEPTH"],
        "description_flavor": "Mixtur-Trautonium reduced to one stria — subharmonic series 1:2, resonance gentle, the abyssal warmth of a single formant.",
        "tags_extra": ["mixtur-trautonium", "subharmonic", "stria", "formant"],
        "param_template": lambda rng: {
            "owl_filterCutoff":      round(rng.uniform(0.3, 0.6), 3),
            "owl_filterReso":        round(rng.uniform(0.15, 0.5), 3),
            "owl_filterType":        rng.randint(0, 1),
            "owl_striaCount":        1,
            "owl_striaSpacing":      round(rng.uniform(0.4, 0.7), 3),
            "owl_striaBlend":        round(rng.uniform(0.5, 0.9), 3),
            "owl_subSeries":         rng.randint(0, 2),
            "owl_subLevel":          round(rng.uniform(0.2, 0.5), 3),
            "owl_subDetune":         round(rng.uniform(-0.01, 0.01), 4),
            "owl_oscLevel":          round(rng.uniform(0.6, 0.9), 3),
            "owl_ampAttack":         round(rng.uniform(0.05, 0.3), 3),
            "owl_ampDecay":          round(rng.uniform(0.1, 0.5), 3),
            "owl_ampSustain":        round(rng.uniform(0.65, 0.95), 3),
            "owl_ampRelease":        round(rng.uniform(0.4, 1.5), 3),
            "owl_lfo1Rate":          round(rng.uniform(0.01, 0.1), 4),
            "owl_lfo1Depth":         round(rng.uniform(0.04, 0.18), 3),
            "owl_lfo1Shape":         rng.randint(0, 2),
            "owl_formantFreq":       round(rng.uniform(0.3, 0.6), 3),
            "owl_formantQ":          round(rng.uniform(0.2, 0.5), 3),
            "owl_formantMix":        round(rng.uniform(0.1, 0.4), 3),
            "owl_reverbMix":         round(rng.uniform(0.35, 0.65), 3),
            "owl_reverbSize":        round(rng.uniform(0.4, 0.7), 3),
            "owl_masterVol":         round(rng.uniform(0.7, 0.9), 3),
        },
    },

    "OCELOT": {
        "display": "Ocelot",
        "engines_key": "Ocelot",
        "macro_labels": ["BIOME", "PROWL", "WARMTH", "TERRITORY"],
        "description_flavor": "Ocelot at rest in warm undergrowth — BIOME at savanna dusk, single voice, PROWL frozen, tawny warmth with sparse motion.",
        "tags_extra": ["biome", "ocelot", "warm-analog", "tropical"],
        "param_template": lambda rng: {
            "ocelot_biome":            rng.randint(0, 3),
            "ocelot_biomeBlend":       round(rng.uniform(0.0, 0.2), 3),
            "ocelot_prowlRate":        round(rng.uniform(0.01, 0.1), 4),
            "ocelot_prowlDepth":       round(rng.uniform(0.0, 0.15), 3),
            "ocelot_oscWave":          rng.randint(0, 2),
            "ocelot_oscLevel":         round(rng.uniform(0.6, 0.9), 3),
            "ocelot_oscDetune":        round(rng.uniform(-0.01, 0.01), 4),
            "ocelot_subLevel":         round(rng.uniform(0.05, 0.3), 3),
            "ocelot_filterCutoff":     round(rng.uniform(0.3, 0.6), 3),
            "ocelot_filterReso":       round(rng.uniform(0.05, 0.3), 3),
            "ocelot_filterEnvAmt":     round(rng.uniform(0.05, 0.25), 3),
            "ocelot_ampAttack":        round(rng.uniform(0.04, 0.25), 3),
            "ocelot_ampDecay":         round(rng.uniform(0.1, 0.5), 3),
            "ocelot_ampSustain":       round(rng.uniform(0.65, 0.95), 3),
            "ocelot_ampRelease":       round(rng.uniform(0.4, 1.5), 3),
            "ocelot_lfo1Rate":         round(rng.uniform(0.01, 0.1), 4),
            "ocelot_lfo1Depth":        round(rng.uniform(0.04, 0.18), 3),
            "ocelot_lfo1Shape":        rng.randint(0, 2),
            "ocelot_chorusMix":        round(rng.uniform(0.0, 0.2), 3),
            "ocelot_reverbMix":        round(rng.uniform(0.3, 0.6), 3),
            "ocelot_reverbSize":       round(rng.uniform(0.4, 0.7), 3),
            "ocelot_masterVol":        round(rng.uniform(0.7, 0.9), 3),
        },
    },

    "ORBITAL": {
        "display": "Orbital",
        "engines_key": "Orbital",
        "macro_labels": ["BRIGHTNESS", "GRAVITY", "DECAY", "SPACE"],
        "description_flavor": "One warm orbital resonance — group envelope at long attack, BRIGHTNESS dim and honeyed, a single perihelion, gently fading.",
        "tags_extra": ["orbital", "group-envelope", "warm-resonance", "slow-attack"],
        "param_template": lambda rng: {
            "orb_brightness":        round(rng.uniform(0.35, 0.6), 3),
            "orb_gravity":           round(rng.uniform(0.1, 0.4), 3),
            "orb_oscWave":           rng.randint(0, 2),
            "orb_oscLevel":          round(rng.uniform(0.6, 0.9), 3),
            "orb_oscDetune":         round(rng.uniform(-0.01, 0.01), 4),
            "orb_subLevel":          round(rng.uniform(0.05, 0.25), 3),
            "orb_filterCutoff":      round(rng.uniform(0.3, 0.6), 3),
            "orb_filterReso":        round(rng.uniform(0.05, 0.3), 3),
            "orb_filterEnvAmt":      round(rng.uniform(0.05, 0.25), 3),
            "orb_groupAttack":       round(rng.uniform(0.08, 0.5), 3),
            "orb_groupDecay":        round(rng.uniform(0.1, 0.6), 3),
            "orb_groupSustain":      round(rng.uniform(0.65, 0.95), 3),
            "orb_groupRelease":      round(rng.uniform(0.5, 2.0), 3),
            "orb_lfo1Rate":          round(rng.uniform(0.01, 0.1), 4),
            "orb_lfo1Depth":         round(rng.uniform(0.04, 0.2), 3),
            "orb_lfo1Shape":         rng.randint(0, 2),
            "orb_chorusMix":         round(rng.uniform(0.0, 0.2), 3),
            "orb_reverbMix":         round(rng.uniform(0.3, 0.6), 3),
            "orb_reverbSize":        round(rng.uniform(0.4, 0.7), 3),
            "orb_masterVol":         round(rng.uniform(0.7, 0.9), 3),
        },
    },

    "ORGANON": {
        "display": "Organon",
        "engines_key": "Organon",
        "macro_labels": ["METABOLIC", "ENTROPY", "MEMBRANE", "WARMTH"],
        "description_flavor": "One living cell at slow metabolism — METABOLIC rate near minimum, membrane resonant and warm, the variational free energy of a single breath.",
        "tags_extra": ["variational-free-energy", "organic", "metabolic", "membrane"],
        "param_template": lambda rng: {
            "organon_metabolicRate":   round(rng.uniform(0.05, 0.25), 3),
            "organon_entropy":         round(rng.uniform(0.0, 0.2), 3),
            "organon_membrane":        round(rng.uniform(0.4, 0.75), 3),
            "organon_oscillatorMode":  rng.randint(0, 1),
            "organon_oscLevel":        round(rng.uniform(0.6, 0.9), 3),
            "organon_oscDetune":       round(rng.uniform(-0.01, 0.01), 4),
            "organon_filterCutoff":    round(rng.uniform(0.3, 0.6), 3),
            "organon_filterReso":      round(rng.uniform(0.05, 0.3), 3),
            "organon_filterEnvAmt":    round(rng.uniform(0.05, 0.25), 3),
            "organon_ampAttack":       round(rng.uniform(0.06, 0.35), 3),
            "organon_ampDecay":        round(rng.uniform(0.1, 0.5), 3),
            "organon_ampSustain":      round(rng.uniform(0.65, 0.95), 3),
            "organon_ampRelease":      round(rng.uniform(0.5, 2.0), 3),
            "organon_lfo1Rate":        round(rng.uniform(0.008, 0.1), 4),
            "organon_lfo1Depth":       round(rng.uniform(0.05, 0.2), 3),
            "organon_lfo1Shape":       rng.randint(0, 2),
            "organon_warmthMod":       round(rng.uniform(0.1, 0.4), 3),
            "organon_reverbMix":       round(rng.uniform(0.3, 0.6), 3),
            "organon_reverbSize":      round(rng.uniform(0.4, 0.7), 3),
            "organon_masterVol":       round(rng.uniform(0.7, 0.9), 3),
        },
    },

    "OSPREY": {
        "display": "Osprey",
        "engines_key": "Osprey",
        "macro_labels": ["SHORE", "DIVE", "CURRENT", "HORIZON"],
        "description_flavor": "Osprey hovering alone over estuary — ShoreSystem at azure coast, single held note before the dive, warm afternoon haze.",
        "tags_extra": ["shore-system", "azulejo", "coastal", "warm-air"],
        "param_template": lambda rng: {
            "osprey_shoreBlend":      round(rng.uniform(0.0, 0.25), 3),
            "osprey_shoreIndex":      rng.randint(0, 4),
            "osprey_diveDepth":       round(rng.uniform(0.0, 0.2), 3),
            "osprey_currentRate":     round(rng.uniform(0.01, 0.1), 4),
            "osprey_currentDepth":    round(rng.uniform(0.04, 0.2), 3),
            "osprey_oscWave":         rng.randint(0, 2),
            "osprey_oscLevel":        round(rng.uniform(0.6, 0.9), 3),
            "osprey_oscDetune":       round(rng.uniform(-0.01, 0.01), 4),
            "osprey_subLevel":        round(rng.uniform(0.05, 0.25), 3),
            "osprey_filterCutoff":    round(rng.uniform(0.3, 0.6), 3),
            "osprey_filterReso":      round(rng.uniform(0.05, 0.3), 3),
            "osprey_filterEnvAmt":    round(rng.uniform(0.05, 0.25), 3),
            "osprey_ampAttack":       round(rng.uniform(0.05, 0.3), 3),
            "osprey_ampDecay":        round(rng.uniform(0.1, 0.5), 3),
            "osprey_ampSustain":      round(rng.uniform(0.65, 0.95), 3),
            "osprey_ampRelease":      round(rng.uniform(0.4, 1.5), 3),
            "osprey_lfo1Rate":        round(rng.uniform(0.01, 0.1), 4),
            "osprey_lfo1Depth":       round(rng.uniform(0.04, 0.18), 3),
            "osprey_chorusMix":       round(rng.uniform(0.0, 0.2), 3),
            "osprey_reverbMix":       round(rng.uniform(0.3, 0.6), 3),
            "osprey_reverbSize":      round(rng.uniform(0.4, 0.7), 3),
            "osprey_masterVol":       round(rng.uniform(0.7, 0.9), 3),
        },
    },

    "ODDOSCAR": {
        "display": "OddOscar",
        "engines_key": "OddOscar",
        "macro_labels": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
        "description_flavor": "Axolotl at rest in warm shallow water — morph at resting state, single voice, gill-pink warmth in still suspension.",
        "tags_extra": ["axolotl", "morph", "warm-pad", "gill-pink"],
        "param_template": lambda rng: {
            "morph_morph":            round(rng.uniform(0.0, 0.3), 3),
            "morph_filterCutoff":     round(rng.uniform(0.3, 0.6), 3),
            "morph_filterReso":       round(rng.uniform(0.05, 0.3), 3),
            "morph_filterEnvAmt":     round(rng.uniform(0.05, 0.25), 3),
            "morph_filterType":       0,
            "morph_oscAWave":         rng.randint(0, 2),
            "morph_oscALevel":        round(rng.uniform(0.6, 0.9), 3),
            "morph_oscAShape":        round(rng.uniform(0.1, 0.5), 3),
            "morph_oscBWave":         rng.randint(0, 1),
            "morph_oscBLevel":        round(rng.uniform(0.0, 0.2), 3),
            "morph_oscBDetune":       round(rng.uniform(-0.03, 0.03), 4),
            "morph_subLevel":         round(rng.uniform(0.05, 0.3), 3),
            "morph_noiseLevel":       round(rng.uniform(0.0, 0.08), 3),
            "morph_ampAttack":        round(rng.uniform(0.05, 0.3), 3),
            "morph_ampDecay":         round(rng.uniform(0.1, 0.5), 3),
            "morph_ampSustain":       round(rng.uniform(0.65, 0.95), 3),
            "morph_ampRelease":       round(rng.uniform(0.4, 1.5), 3),
            "morph_filtEnvAttack":    round(rng.uniform(0.05, 0.25), 3),
            "morph_filtEnvDecay":     round(rng.uniform(0.1, 0.5), 3),
            "morph_filtEnvSustain":   round(rng.uniform(0.4, 0.8), 3),
            "morph_filtEnvRelease":   round(rng.uniform(0.3, 1.0), 3),
            "morph_lfo1Rate":         round(rng.uniform(0.01, 0.1), 4),
            "morph_lfo1Depth":        round(rng.uniform(0.04, 0.2), 3),
            "morph_lfo1Shape":        rng.randint(0, 2),
            "morph_lfo2Rate":         round(rng.uniform(0.008, 0.08), 4),
            "morph_lfo2Depth":        round(rng.uniform(0.02, 0.12), 3),
            "morph_glide":            round(rng.uniform(0.0, 0.15), 3),
            "morph_voiceMode":        0,
            "morph_chorus":           round(rng.uniform(0.0, 0.2), 3),
            "morph_reverbMix":        round(rng.uniform(0.3, 0.6), 3),
            "morph_reverbSize":       round(rng.uniform(0.4, 0.7), 3),
            "morph_delayMix":         round(rng.uniform(0.0, 0.15), 3),
            "morph_masterVol":        round(rng.uniform(0.7, 0.9), 3),
        },
    },
}

# Canonical engine order for iteration
ALL_ENGINES = [
    "OVERDUB", "OBBLIGATO", "OHM", "OSTERIA", "OWLFISH",
    "OCELOT", "ORBITAL", "ORGANON", "OSPREY", "ODDOSCAR",
]

# ---------------------------------------------------------------------------
# DNA generation
# ---------------------------------------------------------------------------

def _generate_dna(rng: random.Random) -> dict:
    """Generate Hot Sparse 6D Sonic DNA."""
    return {
        "warmth":     round(rng.uniform(0.75, 1.0), 3),
        "density":    round(rng.uniform(0.0, 0.25), 3),
        "brightness": round(rng.uniform(0.35, 0.6), 3),
        "movement":   round(rng.uniform(0.35, 0.55), 3),
        "space":      round(rng.uniform(0.55, 0.75), 3),
        "aggression": round(rng.uniform(0.1, 0.35), 3),
    }


# ---------------------------------------------------------------------------
# Name generation
# ---------------------------------------------------------------------------

def _generate_name(rng: random.Random) -> str:
    """Pick a hot sparse name from the vocabulary."""
    return rng.choice(HOT_NAMES)


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
    """Build a complete .xometa hot sparse preset dict."""
    engine_def = ENGINE_DEFS[engine_key]
    dna = _generate_dna(rng)

    tags = list(HOT_TAGS_BASE) + list(engine_def["tags_extra"])
    rng.shuffle(tags)
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
        f"Movement {dna['movement']:.2f} / Space {dna['space']:.2f}."
    )

    return {
        "schema_version": 1,
        "name": preset_name,
        "mood": "Atmosphere",
        "sonic_dna": dna,
        "engines": [engine_def["engines_key"]],
        "author": "XO_OX Hot Sparse Pack",
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
    default_output = repo_root / "Presets" / "XOmnibus" / "Atmosphere"

    parser = argparse.ArgumentParser(
        description="Generate Hot Sparse quadrant preset stubs (warmth≥0.75, density≤0.25) for XOmnibus.",
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
            f"(e.g. OVERDUB,OHM). Available: {', '.join(ALL_ENGINES)}"
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

    print("XOmnibus Hot Sparse Preset Pack")
    print(f"  Quadrant    : warmth 0.75–1.0, density 0.0–0.25")
    print(f"  Mood        : Atmosphere")
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
                f"movement={dna['movement']:.2f}"
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
