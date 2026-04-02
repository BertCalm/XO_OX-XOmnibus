#!/usr/bin/env python3
"""Generate XOverworld .xometa preset files for XOceanus.

Overworld is a multi-era chip synthesis engine (NES + FM/Genesis + SNES).
Generates 40 presets across all 6 moods, covering:
  - NES-focused (10): Pulse leads, triangle bass, noise percussion
  - FM-focused (10): Genesis pads, FM bass, operator leads
  - SNES-focused (10): Sample pads, BRR textures, echo
  - Era Blends (5): Cross-era morph presets
  - Glitch (5): Hardware failure as instrument
"""

import json
import os
import random

random.seed(84)

PRESET_DIR = os.path.join(os.path.dirname(os.path.dirname(__file__)), "Presets", "XOceanus")

DEFAULTS = {
    "ow_ERA": 0.0, "ow_ERA_Y": 0.0, "ow_VOICE_MODE": 0,
    "ow_MASTER_VOL": 0.75, "ow_MASTER_TUNE": 0.0,
    "ow_PULSE_DUTY": 2, "ow_PULSE_SWEEP": 0.0, "ow_TRI_ENABLE": 0,
    "ow_NOISE_MODE": 0, "ow_NOISE_PERIOD": 4, "ow_DPCM_ENABLE": 0,
    "ow_DPCM_RATE": 8, "ow_NES_MIX": 0.8,
    "ow_FM_ALGORITHM": 0, "ow_FM_FEEDBACK": 3,
    "ow_FM_OP1_LEVEL": 100, "ow_FM_OP2_LEVEL": 80,
    "ow_FM_OP3_LEVEL": 90, "ow_FM_OP4_LEVEL": 70,
    "ow_FM_OP1_MULT": 1, "ow_FM_OP2_MULT": 2,
    "ow_FM_OP3_MULT": 1, "ow_FM_OP4_MULT": 3,
    "ow_FM_OP1_DETUNE": 64, "ow_FM_OP2_DETUNE": 64,
    "ow_FM_OP3_DETUNE": 64, "ow_FM_OP4_DETUNE": 64,
    "ow_FM_ATTACK": 10, "ow_FM_DECAY": 12, "ow_FM_SUSTAIN": 20, "ow_FM_RELEASE": 8,
    "ow_FM_LFO_RATE": 3, "ow_FM_LFO_DEPTH": 0,
    "ow_BRR_SAMPLE": 0, "ow_BRR_INTERP": 1,
    "ow_SNES_ATTACK": 10, "ow_SNES_DECAY": 5, "ow_SNES_SUSTAIN": 7, "ow_SNES_RELEASE": 12,
    "ow_PITCH_MOD": 0, "ow_NOISE_REPLACE": 0,
    "ow_ECHO_DELAY": 5, "ow_ECHO_FEEDBACK": 0.4, "ow_ECHO_MIX": 0.0,
    "ow_ECHO_FIR_0": 127, "ow_ECHO_FIR_1": 0, "ow_ECHO_FIR_2": 0, "ow_ECHO_FIR_3": 0,
    "ow_ECHO_FIR_4": 0, "ow_ECHO_FIR_5": 0, "ow_ECHO_FIR_6": 0, "ow_ECHO_FIR_7": 0,
    "ow_GLITCH_AMOUNT": 0.0, "ow_GLITCH_TYPE": 0, "ow_GLITCH_RATE": 2.0,
    "ow_GLITCH_DEPTH": 0.3, "ow_GLITCH_MIX": 0.0,
    "ow_AMP_ATTACK": 0.005, "ow_AMP_DECAY": 0.3, "ow_AMP_SUSTAIN": 0.8, "ow_AMP_RELEASE": 0.3,
    "ow_FILTER_CUTOFF": 12000.0, "ow_FILTER_RESO": 0.0, "ow_FILTER_TYPE": 0,
    "ow_CRUSH_BITS": 16, "ow_CRUSH_RATE": 0, "ow_CRUSH_MIX": 0.0,
    "ow_ERA_DRIFT_RATE": 0.0, "ow_ERA_DRIFT_DEPTH": 0.0, "ow_ERA_DRIFT_SHAPE": 0,
    "ow_ERA_PORTA_TIME": 0.0, "ow_ERA_MEM_TIME": 0.0, "ow_ERA_MEM_MIX": 0.0,
    "ow_VERTEX_A": 0, "ow_VERTEX_B": 1, "ow_VERTEX_C": 2,
    "ow_colorTheme": 0,
}


def r(lo, hi):
    return round(random.uniform(lo, hi), 2)


def ri(lo, hi):
    return random.randint(lo, hi)


def make_params(**overrides):
    p = dict(DEFAULTS)
    p.update(overrides)
    return p


def make_preset(name, mood, desc, tags, params, dna, engines=None,
                coupling_intensity="None", coupling=None):
    return {
        "schema_version": 1,
        "name": name,
        "mood": mood,
        "engines": engines or ["Overworld"],
        "author": "XO_OX Designs",
        "version": "1.0.0",
        "description": desc,
        "tags": tags,
        "macroLabels": ["ERA", "CIRCUIT", "GLITCH", "SPACE"],
        "couplingIntensity": coupling_intensity,
        "tempo": None,
        "dna": dna,
        "parameters": {"Overworld": params},
        "coupling": coupling,
        "sequencer": None,
    }


def write_preset(preset):
    mood = preset["mood"]
    mood_dir = os.path.join(PRESET_DIR, mood)
    os.makedirs(mood_dir, exist_ok=True)
    filename = preset["name"] + ".xometa"
    filepath = os.path.join(mood_dir, filename)
    with open(filepath, "w") as f:
        json.dump(preset, f, indent=2)
        f.write("\n")
    return filepath


# ========================================================================
# NES-focused presets (10)
# ========================================================================
NES_PRESETS = [
    ("Pulse Hero", "Foundation", "Classic NES pulse lead. 50% duty, tight envelope.",
     ["nes", "lead", "pulse", "retro"],
     {"ow_ERA": 0.0, "ow_PULSE_DUTY": 2, "ow_AMP_ATTACK": 0.005, "ow_AMP_DECAY": 0.2,
      "ow_AMP_SUSTAIN": 0.7, "ow_AMP_RELEASE": 0.15, "ow_FILTER_CUTOFF": 10000.0},
     {"brightness": 0.6, "warmth": 0.3, "movement": 0.2, "density": 0.3, "space": 0.05, "aggression": 0.3}),

    ("Triangle Bass", "Foundation", "Deep NES triangle wave bass. Pure sub-octave warmth.",
     ["nes", "bass", "triangle", "sub"],
     {"ow_ERA": 0.0, "ow_TRI_ENABLE": 1, "ow_PULSE_DUTY": 0, "ow_NES_MIX": 0.6,
      "ow_AMP_ATTACK": 0.003, "ow_AMP_DECAY": 0.5, "ow_AMP_SUSTAIN": 0.9, "ow_AMP_RELEASE": 0.2,
      "ow_FILTER_CUTOFF": 3000.0},
     {"brightness": 0.15, "warmth": 0.7, "movement": 0.1, "density": 0.3, "space": 0.0, "aggression": 0.05}),

    ("Noise Hat", "Foundation", "Short metallic NES noise. Crisp hi-hat character.",
     ["nes", "percussion", "hat", "noise"],
     {"ow_ERA": 0.0, "ow_VOICE_MODE": 1, "ow_NOISE_MODE": 0, "ow_NOISE_PERIOD": 2,
      "ow_AMP_ATTACK": 0.001, "ow_AMP_DECAY": 0.08, "ow_AMP_SUSTAIN": 0.0, "ow_AMP_RELEASE": 0.05},
     {"brightness": 0.8, "warmth": 0.05, "movement": 0.1, "density": 0.4, "space": 0.0, "aggression": 0.5}),

    ("Dungeon Arp", "Prism", "Quick 25% duty pulse with pitch sweep. Arpeggio character.",
     ["nes", "arp", "melody", "dungeon"],
     {"ow_ERA": 0.0, "ow_PULSE_DUTY": 1, "ow_PULSE_SWEEP": 0.3,
      "ow_AMP_ATTACK": 0.003, "ow_AMP_DECAY": 0.15, "ow_AMP_SUSTAIN": 0.5, "ow_AMP_RELEASE": 0.1},
     {"brightness": 0.5, "warmth": 0.3, "movement": 0.5, "density": 0.3, "space": 0.05, "aggression": 0.2}),

    ("Castle Wall", "Flux", "Aggressive 12.5% duty pulse. Narrow, biting lead.",
     ["nes", "lead", "aggressive", "narrow"],
     {"ow_ERA": 0.0, "ow_PULSE_DUTY": 0, "ow_FILTER_CUTOFF": 8000.0, "ow_FILTER_RESO": 0.3,
      "ow_AMP_ATTACK": 0.002, "ow_AMP_DECAY": 0.1, "ow_AMP_SUSTAIN": 0.8, "ow_AMP_RELEASE": 0.08},
     {"brightness": 0.7, "warmth": 0.1, "movement": 0.3, "density": 0.5, "space": 0.0, "aggression": 0.6}),

    ("Coin Chime", "Prism", "Bright short pulse pluck. Satisfying collect sound.",
     ["nes", "pluck", "bright", "coin"],
     {"ow_ERA": 0.0, "ow_PULSE_DUTY": 2, "ow_AMP_ATTACK": 0.001, "ow_AMP_DECAY": 0.12,
      "ow_AMP_SUSTAIN": 0.0, "ow_AMP_RELEASE": 0.08, "ow_FILTER_CUTOFF": 15000.0},
     {"brightness": 0.8, "warmth": 0.2, "movement": 0.2, "density": 0.2, "space": 0.1, "aggression": 0.15}),

    ("Power Up", "Prism", "Rising pulse sweep. Classic power-up ascending tone.",
     ["nes", "effect", "rising", "power"],
     {"ow_ERA": 0.0, "ow_PULSE_DUTY": 2, "ow_PULSE_SWEEP": 0.7,
      "ow_AMP_ATTACK": 0.01, "ow_AMP_DECAY": 0.5, "ow_AMP_SUSTAIN": 0.3, "ow_AMP_RELEASE": 0.2},
     {"brightness": 0.6, "warmth": 0.3, "movement": 0.7, "density": 0.3, "space": 0.1, "aggression": 0.2}),

    ("Warp Zone", "Flux", "NES noise + pulse combination. Chaotic transition texture.",
     ["nes", "effect", "warp", "chaos"],
     {"ow_ERA": 0.0, "ow_NOISE_MODE": 1, "ow_NOISE_PERIOD": 8, "ow_PULSE_DUTY": 3,
      "ow_PULSE_SWEEP": 0.5, "ow_FILTER_CUTOFF": 6000.0, "ow_FILTER_RESO": 0.4,
      "ow_AMP_ATTACK": 0.01, "ow_AMP_DECAY": 0.4, "ow_AMP_SUSTAIN": 0.5, "ow_AMP_RELEASE": 0.3},
     {"brightness": 0.5, "warmth": 0.2, "movement": 0.6, "density": 0.5, "space": 0.1, "aggression": 0.4}),

    ("Star Road", "Atmosphere", "Soft pulse pad with triangle underneath. Nostalgic warmth.",
     ["nes", "pad", "nostalgic", "warm"],
     {"ow_ERA": 0.0, "ow_PULSE_DUTY": 2, "ow_TRI_ENABLE": 1, "ow_NES_MIX": 0.7,
      "ow_AMP_ATTACK": 0.3, "ow_AMP_DECAY": 0.5, "ow_AMP_SUSTAIN": 0.8, "ow_AMP_RELEASE": 1.0,
      "ow_FILTER_CUTOFF": 5000.0, "ow_colorTheme": 10},
     {"brightness": 0.3, "warmth": 0.6, "movement": 0.2, "density": 0.4, "space": 0.3, "aggression": 0.0}),

    ("Runner Kick", "Foundation", "Punchy NES kick using short noise burst + triangle.",
     ["nes", "percussion", "kick", "punchy"],
     {"ow_ERA": 0.0, "ow_VOICE_MODE": 1, "ow_TRI_ENABLE": 1, "ow_NOISE_MODE": 1,
      "ow_NOISE_PERIOD": 1, "ow_AMP_ATTACK": 0.001, "ow_AMP_DECAY": 0.12,
      "ow_AMP_SUSTAIN": 0.0, "ow_AMP_RELEASE": 0.05, "ow_FILTER_CUTOFF": 2000.0},
     {"brightness": 0.2, "warmth": 0.5, "movement": 0.1, "density": 0.5, "space": 0.0, "aggression": 0.6}),
]

# ========================================================================
# FM-focused presets (10)
# ========================================================================
FM_PRESETS = [
    ("Genesis Pad", "Atmosphere", "Warm FM pad. Algorithm 5, detuned operators.",
     ["fm", "pad", "warm", "genesis"],
     {"ow_ERA": 0.5, "ow_ERA_Y": 0.8, "ow_FM_ALGORITHM": 5, "ow_FM_FEEDBACK": 2,
      "ow_FM_OP1_MULT": 1, "ow_FM_OP2_MULT": 1, "ow_FM_OP3_MULT": 2, "ow_FM_OP4_MULT": 1,
      "ow_FM_OP2_DETUNE": 68, "ow_FM_OP3_DETUNE": 60,
      "ow_AMP_ATTACK": 0.4, "ow_AMP_SUSTAIN": 0.85, "ow_AMP_RELEASE": 1.5},
     {"brightness": 0.4, "warmth": 0.6, "movement": 0.3, "density": 0.5, "space": 0.3, "aggression": 0.1}),

    ("Slap Bass", "Foundation", "Punchy FM bass. Algorithm 4, tight envelope.",
     ["fm", "bass", "slap", "punchy"],
     {"ow_ERA": 0.5, "ow_ERA_Y": 0.8, "ow_FM_ALGORITHM": 4, "ow_FM_FEEDBACK": 5,
      "ow_FM_OP1_MULT": 1, "ow_FM_OP2_MULT": 1, "ow_FM_OP3_MULT": 2, "ow_FM_OP4_MULT": 4,
      "ow_FM_ATTACK": 0, "ow_FM_DECAY": 8, "ow_FM_SUSTAIN": 15,
      "ow_AMP_ATTACK": 0.003, "ow_AMP_DECAY": 0.2, "ow_AMP_SUSTAIN": 0.6, "ow_AMP_RELEASE": 0.1},
     {"brightness": 0.5, "warmth": 0.4, "movement": 0.3, "density": 0.4, "space": 0.0, "aggression": 0.5}),

    ("Streets Stab", "Flux", "Aggressive FM stab. Algorithm 2, high feedback.",
     ["fm", "stab", "aggressive", "streets"],
     {"ow_ERA": 0.5, "ow_ERA_Y": 0.9, "ow_FM_ALGORITHM": 2, "ow_FM_FEEDBACK": 6,
      "ow_FM_OP1_MULT": 1, "ow_FM_OP2_MULT": 3, "ow_FM_OP3_MULT": 7, "ow_FM_OP4_MULT": 1,
      "ow_FM_OP3_LEVEL": 110, "ow_FM_ATTACK": 0, "ow_FM_DECAY": 5,
      "ow_AMP_ATTACK": 0.001, "ow_AMP_DECAY": 0.15, "ow_AMP_SUSTAIN": 0.4, "ow_AMP_RELEASE": 0.08},
     {"brightness": 0.7, "warmth": 0.15, "movement": 0.4, "density": 0.6, "space": 0.0, "aggression": 0.7}),

    ("Crystal Bell", "Prism", "FM bell tone. Algorithm 7, high ratio operators.",
     ["fm", "bell", "crystal", "bright"],
     {"ow_ERA": 0.5, "ow_ERA_Y": 0.8, "ow_FM_ALGORITHM": 7, "ow_FM_FEEDBACK": 1,
      "ow_FM_OP1_MULT": 1, "ow_FM_OP2_MULT": 4, "ow_FM_OP3_MULT": 7, "ow_FM_OP4_MULT": 11,
      "ow_FM_OP2_LEVEL": 50, "ow_FM_OP3_LEVEL": 35, "ow_FM_OP4_LEVEL": 20,
      "ow_AMP_ATTACK": 0.001, "ow_AMP_DECAY": 0.8, "ow_AMP_SUSTAIN": 0.1, "ow_AMP_RELEASE": 0.5},
     {"brightness": 0.85, "warmth": 0.2, "movement": 0.2, "density": 0.3, "space": 0.2, "aggression": 0.1}),

    ("Brass Section", "Foundation", "FM brass. Algorithm 0, moderate feedback.",
     ["fm", "brass", "warm", "section"],
     {"ow_ERA": 0.5, "ow_ERA_Y": 0.9, "ow_FM_ALGORITHM": 0, "ow_FM_FEEDBACK": 4,
      "ow_FM_OP1_MULT": 1, "ow_FM_OP2_MULT": 1, "ow_FM_OP3_MULT": 1, "ow_FM_OP4_MULT": 1,
      "ow_FM_OP2_DETUNE": 67, "ow_FM_OP3_DETUNE": 61,
      "ow_AMP_ATTACK": 0.05, "ow_AMP_SUSTAIN": 0.85, "ow_AMP_RELEASE": 0.2},
     {"brightness": 0.5, "warmth": 0.5, "movement": 0.2, "density": 0.6, "space": 0.1, "aggression": 0.35}),

    ("Neon Arp", "Prism", "Quick FM arp tone. Bright, percussive, fast decay.",
     ["fm", "arp", "neon", "bright"],
     {"ow_ERA": 0.5, "ow_ERA_Y": 0.85, "ow_FM_ALGORITHM": 3, "ow_FM_FEEDBACK": 3,
      "ow_FM_OP1_MULT": 1, "ow_FM_OP2_MULT": 5, "ow_FM_OP3_MULT": 1, "ow_FM_OP4_MULT": 2,
      "ow_AMP_ATTACK": 0.001, "ow_AMP_DECAY": 0.2, "ow_AMP_SUSTAIN": 0.3, "ow_AMP_RELEASE": 0.1},
     {"brightness": 0.7, "warmth": 0.2, "movement": 0.5, "density": 0.4, "space": 0.05, "aggression": 0.3}),

    ("Sunset Drive", "Atmosphere", "Smooth FM pad with LFO vibrato. Warm evening feel.",
     ["fm", "pad", "smooth", "evening"],
     {"ow_ERA": 0.5, "ow_ERA_Y": 0.75, "ow_FM_ALGORITHM": 5, "ow_FM_FEEDBACK": 2,
      "ow_FM_LFO_RATE": 4, "ow_FM_LFO_DEPTH": 30,
      "ow_AMP_ATTACK": 0.5, "ow_AMP_SUSTAIN": 0.9, "ow_AMP_RELEASE": 2.0, "ow_colorTheme": 12},
     {"brightness": 0.35, "warmth": 0.7, "movement": 0.4, "density": 0.4, "space": 0.4, "aggression": 0.0}),

    ("Thunderforce", "Flux", "Maximum FM aggression. Algorithm 0, max feedback.",
     ["fm", "lead", "aggressive", "intense"],
     {"ow_ERA": 0.5, "ow_ERA_Y": 0.95, "ow_FM_ALGORITHM": 0, "ow_FM_FEEDBACK": 7,
      "ow_FM_OP1_MULT": 1, "ow_FM_OP2_MULT": 3, "ow_FM_OP3_MULT": 5, "ow_FM_OP4_MULT": 7,
      "ow_FM_OP1_LEVEL": 127, "ow_FM_OP2_LEVEL": 100, "ow_FM_OP3_LEVEL": 110, "ow_FM_OP4_LEVEL": 90,
      "ow_AMP_ATTACK": 0.002, "ow_AMP_SUSTAIN": 0.85, "ow_AMP_RELEASE": 0.1},
     {"brightness": 0.8, "warmth": 0.1, "movement": 0.5, "density": 0.7, "space": 0.0, "aggression": 0.9}),

    ("Phantom Organ", "Aether", "Eerie FM organ with slow attack. Gothic atmosphere.",
     ["fm", "organ", "eerie", "gothic"],
     {"ow_ERA": 0.5, "ow_ERA_Y": 0.8, "ow_FM_ALGORITHM": 6, "ow_FM_FEEDBACK": 3,
      "ow_FM_OP1_MULT": 1, "ow_FM_OP2_MULT": 2, "ow_FM_OP3_MULT": 4, "ow_FM_OP4_MULT": 8,
      "ow_AMP_ATTACK": 0.8, "ow_AMP_SUSTAIN": 0.9, "ow_AMP_RELEASE": 2.0,
      "ow_FILTER_CUTOFF": 6000.0, "ow_colorTheme": 8},
     {"brightness": 0.4, "warmth": 0.5, "movement": 0.3, "density": 0.5, "space": 0.5, "aggression": 0.15}),

    ("Ring Lead", "Prism", "FM ring-mod-like lead. Inharmonic partials, metallic.",
     ["fm", "lead", "metallic", "ring"],
     {"ow_ERA": 0.5, "ow_ERA_Y": 0.85, "ow_FM_ALGORITHM": 1, "ow_FM_FEEDBACK": 4,
      "ow_FM_OP1_MULT": 1, "ow_FM_OP2_MULT": 3, "ow_FM_OP3_MULT": 5, "ow_FM_OP4_MULT": 9,
      "ow_AMP_ATTACK": 0.003, "ow_AMP_DECAY": 0.4, "ow_AMP_SUSTAIN": 0.5, "ow_AMP_RELEASE": 0.2},
     {"brightness": 0.65, "warmth": 0.15, "movement": 0.3, "density": 0.4, "space": 0.1, "aggression": 0.4}),
]

# ========================================================================
# SNES-focused presets (10)
# ========================================================================
SNES_PRESETS = [
    ("Cinematic Pad", "Aether", "Lush SNES sample pad with Gaussian interpolation and echo.",
     ["snes", "pad", "cinematic", "lush"],
     {"ow_ERA": 1.0, "ow_BRR_INTERP": 1, "ow_SNES_ATTACK": 12, "ow_SNES_SUSTAIN": 10,
      "ow_SNES_RELEASE": 14, "ow_ECHO_DELAY": 8, "ow_ECHO_FEEDBACK": 0.5, "ow_ECHO_MIX": 0.3,
      "ow_AMP_ATTACK": 0.5, "ow_AMP_SUSTAIN": 0.9, "ow_AMP_RELEASE": 2.0},
     {"brightness": 0.3, "warmth": 0.7, "movement": 0.3, "density": 0.4, "space": 0.7, "aggression": 0.0}),

    ("Jungle Bass", "Foundation", "Punchy SNES bass with short echo. DKC inspired.",
     ["snes", "bass", "jungle", "punchy"],
     {"ow_ERA": 1.0, "ow_BRR_SAMPLE": 2, "ow_SNES_ATTACK": 0, "ow_SNES_DECAY": 8,
      "ow_SNES_SUSTAIN": 6, "ow_ECHO_DELAY": 2, "ow_ECHO_FEEDBACK": 0.2, "ow_ECHO_MIX": 0.1,
      "ow_AMP_ATTACK": 0.003, "ow_AMP_DECAY": 0.3, "ow_AMP_SUSTAIN": 0.7, "ow_AMP_RELEASE": 0.15,
      "ow_FILTER_CUTOFF": 4000.0, "ow_colorTheme": 6},
     {"brightness": 0.3, "warmth": 0.6, "movement": 0.2, "density": 0.5, "space": 0.15, "aggression": 0.3}),

    ("Crystal Cave", "Prism", "Bright SNES tones with long FIR echo. Sparkling.",
     ["snes", "bright", "crystal", "echo"],
     {"ow_ERA": 1.0, "ow_BRR_INTERP": 1, "ow_ECHO_DELAY": 10, "ow_ECHO_FEEDBACK": 0.6,
      "ow_ECHO_MIX": 0.4, "ow_ECHO_FIR_0": 127, "ow_ECHO_FIR_1": 64, "ow_ECHO_FIR_2": 32,
      "ow_AMP_ATTACK": 0.01, "ow_AMP_DECAY": 0.5, "ow_AMP_SUSTAIN": 0.4, "ow_AMP_RELEASE": 0.8},
     {"brightness": 0.7, "warmth": 0.4, "movement": 0.4, "density": 0.3, "space": 0.6, "aggression": 0.05}),

    ("Memory Theme", "Atmosphere", "Nostalgic SNES pad. Slow attack, warm FIR filtering.",
     ["snes", "pad", "nostalgic", "warm"],
     {"ow_ERA": 1.0, "ow_BRR_INTERP": 1, "ow_SNES_ATTACK": 14, "ow_SNES_SUSTAIN": 12,
      "ow_ECHO_DELAY": 6, "ow_ECHO_FEEDBACK": 0.45, "ow_ECHO_MIX": 0.25,
      "ow_AMP_ATTACK": 0.6, "ow_AMP_SUSTAIN": 0.85, "ow_AMP_RELEASE": 1.5,
      "ow_FILTER_CUTOFF": 5000.0},
     {"brightness": 0.3, "warmth": 0.75, "movement": 0.25, "density": 0.35, "space": 0.5, "aggression": 0.0}),

    ("Pitch Warp", "Flux", "SNES pitch modulation creates warping effect.",
     ["snes", "effect", "warp", "modulation"],
     {"ow_ERA": 1.0, "ow_PITCH_MOD": 1, "ow_BRR_INTERP": 0,
      "ow_AMP_ATTACK": 0.01, "ow_AMP_DECAY": 0.3, "ow_AMP_SUSTAIN": 0.6, "ow_AMP_RELEASE": 0.2},
     {"brightness": 0.5, "warmth": 0.3, "movement": 0.8, "density": 0.4, "space": 0.2, "aggression": 0.35}),

    ("Frozen Echo", "Aether", "SNES pad with maximum echo feedback. Self-evolving.",
     ["snes", "ambient", "echo", "frozen"],
     {"ow_ERA": 1.0, "ow_BRR_INTERP": 1, "ow_ECHO_DELAY": 12, "ow_ECHO_FEEDBACK": 0.85,
      "ow_ECHO_MIX": 0.5, "ow_AMP_ATTACK": 1.0, "ow_AMP_SUSTAIN": 0.8, "ow_AMP_RELEASE": 3.0,
      "ow_FILTER_CUTOFF": 4000.0},
     {"brightness": 0.2, "warmth": 0.6, "movement": 0.5, "density": 0.3, "space": 0.9, "aggression": 0.0}),

    ("Steel Drum", "Prism", "Short SNES sample with metallic echo character.",
     ["snes", "percussion", "steel", "metallic"],
     {"ow_ERA": 1.0, "ow_BRR_SAMPLE": 5, "ow_SNES_ATTACK": 0, "ow_SNES_DECAY": 4,
      "ow_SNES_SUSTAIN": 0, "ow_ECHO_DELAY": 3, "ow_ECHO_FEEDBACK": 0.3, "ow_ECHO_MIX": 0.2,
      "ow_AMP_ATTACK": 0.001, "ow_AMP_DECAY": 0.3, "ow_AMP_SUSTAIN": 0.0, "ow_AMP_RELEASE": 0.15},
     {"brightness": 0.6, "warmth": 0.3, "movement": 0.3, "density": 0.3, "space": 0.25, "aggression": 0.2}),

    ("Noise Wind", "Atmosphere", "SNES noise replace mode creates wind-like ambience.",
     ["snes", "ambient", "noise", "wind"],
     {"ow_ERA": 1.0, "ow_NOISE_REPLACE": 1, "ow_ECHO_DELAY": 10, "ow_ECHO_FEEDBACK": 0.6,
      "ow_ECHO_MIX": 0.4, "ow_FILTER_CUTOFF": 3000.0, "ow_FILTER_RESO": 0.2,
      "ow_AMP_ATTACK": 0.8, "ow_AMP_SUSTAIN": 0.7, "ow_AMP_RELEASE": 2.0},
     {"brightness": 0.25, "warmth": 0.5, "movement": 0.4, "density": 0.3, "space": 0.6, "aggression": 0.1}),

    ("BRR Crunch", "Flux", "Lo-fi SNES sample with no interpolation. Raw digital.",
     ["snes", "lofi", "crunch", "digital"],
     {"ow_ERA": 1.0, "ow_BRR_INTERP": 0, "ow_CRUSH_BITS": 8, "ow_CRUSH_MIX": 0.5,
      "ow_AMP_ATTACK": 0.005, "ow_AMP_DECAY": 0.2, "ow_AMP_SUSTAIN": 0.7, "ow_AMP_RELEASE": 0.15},
     {"brightness": 0.5, "warmth": 0.2, "movement": 0.3, "density": 0.5, "space": 0.05, "aggression": 0.5}),

    ("Ancient Temple", "Aether", "Deep SNES reverb pad. Sacred, vast space.",
     ["snes", "pad", "sacred", "temple"],
     {"ow_ERA": 1.0, "ow_BRR_INTERP": 1, "ow_ECHO_DELAY": 14, "ow_ECHO_FEEDBACK": 0.7,
      "ow_ECHO_MIX": 0.45, "ow_ECHO_FIR_0": 100, "ow_ECHO_FIR_1": 80, "ow_ECHO_FIR_2": 50,
      "ow_ECHO_FIR_3": 30, "ow_AMP_ATTACK": 0.8, "ow_AMP_SUSTAIN": 0.9, "ow_AMP_RELEASE": 3.0,
      "ow_FILTER_CUTOFF": 4500.0},
     {"brightness": 0.25, "warmth": 0.65, "movement": 0.3, "density": 0.3, "space": 0.85, "aggression": 0.0}),
]

# ========================================================================
# Era Blend presets (5)
# ========================================================================
ERA_BLENDS = [
    ("Console Morph", "Entangled", "Slowly morphing between all three console eras.",
     ["blend", "morph", "evolving", "eras"],
     {"ow_ERA": 0.33, "ow_ERA_Y": 0.33, "ow_ERA_DRIFT_RATE": 0.1, "ow_ERA_DRIFT_DEPTH": 0.8,
      "ow_ERA_DRIFT_SHAPE": 0, "ow_AMP_ATTACK": 0.3, "ow_AMP_SUSTAIN": 0.85, "ow_AMP_RELEASE": 1.0},
     {"brightness": 0.5, "warmth": 0.5, "movement": 0.7, "density": 0.5, "space": 0.3, "aggression": 0.2}),

    ("Bit Fusion", "Entangled", "NES pulse meets FM operator. Mid-era hybrid character.",
     ["blend", "fusion", "hybrid", "nes-fm"],
     {"ow_ERA": 0.3, "ow_ERA_Y": 0.5, "ow_PULSE_DUTY": 2, "ow_FM_ALGORITHM": 3,
      "ow_AMP_ATTACK": 0.01, "ow_AMP_DECAY": 0.3, "ow_AMP_SUSTAIN": 0.6, "ow_AMP_RELEASE": 0.2},
     {"brightness": 0.55, "warmth": 0.35, "movement": 0.4, "density": 0.5, "space": 0.1, "aggression": 0.3}),

    ("Sample Meets FM", "Entangled", "SNES warmth blended with FM harmonics.",
     ["blend", "snes-fm", "warm", "harmonic"],
     {"ow_ERA": 0.7, "ow_ERA_Y": 0.5, "ow_FM_ALGORITHM": 5, "ow_BRR_INTERP": 1,
      "ow_ECHO_DELAY": 5, "ow_ECHO_MIX": 0.15,
      "ow_AMP_ATTACK": 0.2, "ow_AMP_SUSTAIN": 0.8, "ow_AMP_RELEASE": 0.8},
     {"brightness": 0.4, "warmth": 0.6, "movement": 0.35, "density": 0.5, "space": 0.3, "aggression": 0.1}),

    ("Timeline Drift", "Entangled", "ERA position drifts randomly. Unpredictable era shifts.",
     ["blend", "drift", "random", "unpredictable"],
     {"ow_ERA": 0.5, "ow_ERA_Y": 0.5, "ow_ERA_DRIFT_RATE": 0.3, "ow_ERA_DRIFT_DEPTH": 0.9,
      "ow_ERA_DRIFT_SHAPE": 2, "ow_AMP_ATTACK": 0.1, "ow_AMP_SUSTAIN": 0.8, "ow_AMP_RELEASE": 0.5},
     {"brightness": 0.5, "warmth": 0.4, "movement": 0.8, "density": 0.5, "space": 0.2, "aggression": 0.25}),

    ("Era Memory", "Atmosphere", "ERA blend with memory ghost layer. Echoes of past eras.",
     ["blend", "memory", "ghost", "echoes"],
     {"ow_ERA": 0.4, "ow_ERA_Y": 0.3, "ow_ERA_MEM_TIME": 2.0, "ow_ERA_MEM_MIX": 0.4,
      "ow_ERA_DRIFT_RATE": 0.05, "ow_ERA_DRIFT_DEPTH": 0.5,
      "ow_AMP_ATTACK": 0.3, "ow_AMP_SUSTAIN": 0.85, "ow_AMP_RELEASE": 1.5},
     {"brightness": 0.35, "warmth": 0.55, "movement": 0.5, "density": 0.4, "space": 0.5, "aggression": 0.05}),
]

# ========================================================================
# Glitch presets (5)
# ========================================================================
GLITCH_PRESETS = [
    ("Cartridge Tilt", "Flux", "NES glitch — loose cartridge contact. Periodic crackles.",
     ["glitch", "nes", "crackle", "retro"],
     {"ow_ERA": 0.0, "ow_GLITCH_AMOUNT": 0.4, "ow_GLITCH_TYPE": 0, "ow_GLITCH_RATE": 3.0,
      "ow_GLITCH_DEPTH": 0.5, "ow_GLITCH_MIX": 0.6, "ow_PULSE_DUTY": 2,
      "ow_AMP_ATTACK": 0.01, "ow_AMP_SUSTAIN": 0.7, "ow_AMP_RELEASE": 0.3},
     {"brightness": 0.5, "warmth": 0.25, "movement": 0.6, "density": 0.5, "space": 0.1, "aggression": 0.5}),

    ("Register Bleed", "Flux", "FM glitch — operator corruption. Timbre morphing.",
     ["glitch", "fm", "corruption", "morph"],
     {"ow_ERA": 0.5, "ow_ERA_Y": 0.85, "ow_GLITCH_AMOUNT": 0.5, "ow_GLITCH_TYPE": 1,
      "ow_GLITCH_RATE": 5.0, "ow_GLITCH_DEPTH": 0.6, "ow_GLITCH_MIX": 0.7,
      "ow_FM_ALGORITHM": 2, "ow_FM_FEEDBACK": 5,
      "ow_AMP_ATTACK": 0.005, "ow_AMP_SUSTAIN": 0.8, "ow_AMP_RELEASE": 0.2},
     {"brightness": 0.6, "warmth": 0.2, "movement": 0.7, "density": 0.6, "space": 0.05, "aggression": 0.6}),

    ("Echo Overflow", "Aether", "SNES echo buffer overwriting sample RAM. Self-evolving.",
     ["glitch", "snes", "echo", "evolving"],
     {"ow_ERA": 1.0, "ow_GLITCH_AMOUNT": 0.6, "ow_GLITCH_TYPE": 2, "ow_GLITCH_RATE": 1.0,
      "ow_GLITCH_DEPTH": 0.7, "ow_GLITCH_MIX": 0.5, "ow_ECHO_DELAY": 10,
      "ow_ECHO_FEEDBACK": 0.75, "ow_ECHO_MIX": 0.4,
      "ow_AMP_ATTACK": 0.5, "ow_AMP_SUSTAIN": 0.8, "ow_AMP_RELEASE": 2.0},
     {"brightness": 0.3, "warmth": 0.4, "movement": 0.7, "density": 0.4, "space": 0.6, "aggression": 0.3}),

    ("Kill Screen", "Flux", "Maximum glitch across all eras. Beautiful chaos.",
     ["glitch", "chaos", "extreme", "all-eras"],
     {"ow_ERA": 0.5, "ow_ERA_Y": 0.5, "ow_GLITCH_AMOUNT": 1.0, "ow_GLITCH_TYPE": 3,
      "ow_GLITCH_RATE": 10.0, "ow_GLITCH_DEPTH": 1.0, "ow_GLITCH_MIX": 0.8,
      "ow_ERA_DRIFT_RATE": 2.0, "ow_ERA_DRIFT_DEPTH": 0.7,
      "ow_AMP_ATTACK": 0.01, "ow_AMP_SUSTAIN": 0.6, "ow_AMP_RELEASE": 0.3},
     {"brightness": 0.6, "warmth": 0.15, "movement": 0.95, "density": 0.7, "space": 0.1, "aggression": 0.85}),

    ("Ghost Sprites", "Atmosphere", "Subtle sprite flicker glitch. Ghostly, nostalgic.",
     ["glitch", "ghost", "subtle", "nostalgic"],
     {"ow_ERA": 0.0, "ow_GLITCH_AMOUNT": 0.2, "ow_GLITCH_TYPE": 0, "ow_GLITCH_RATE": 1.5,
      "ow_GLITCH_DEPTH": 0.3, "ow_GLITCH_MIX": 0.35, "ow_PULSE_DUTY": 2, "ow_TRI_ENABLE": 1,
      "ow_AMP_ATTACK": 0.3, "ow_AMP_SUSTAIN": 0.75, "ow_AMP_RELEASE": 1.0,
      "ow_FILTER_CUTOFF": 5000.0},
     {"brightness": 0.35, "warmth": 0.5, "movement": 0.4, "density": 0.3, "space": 0.3, "aggression": 0.15}),
]


def main():
    count = 0
    all_presets = []

    for category, presets in [("NES", NES_PRESETS), ("FM", FM_PRESETS),
                               ("SNES", SNES_PRESETS), ("ERA BLEND", ERA_BLENDS),
                               ("GLITCH", GLITCH_PRESETS)]:
        for name, mood, desc, tags, overrides, dna in presets:
            params = make_params(**overrides)
            preset = make_preset(name, mood, desc, tags, params, dna)
            path = write_preset(preset)
            count += 1
            print(f"  [{count:3d}] {category:10s} | {mood:12s} | {name}")
            all_presets.append(preset)

    print(f"\nGenerated {count} Overworld presets.")


if __name__ == "__main__":
    main()
