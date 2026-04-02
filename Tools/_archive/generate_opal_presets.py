#!/usr/bin/env python3
"""Generate 100 XOpal factory presets for XOceanus.

Categories (from Phase 1 spec):
  - Smooth Clouds (20): Large grains, low scatter, atmospheric pads
  - Shimmer Textures (20): Small grains, high density, bright airy
  - Frozen Moments (15): High freeze, slow movement
  - Scattered Glass (15): High pitch scatter, rhythmic density
  - Coupling Showcases (20): Cross-engine specific pairs
  - Deep Drift (10): Max scatter, generative/ambient
"""

import json
import os
import random

random.seed(42)  # Reproducible

PRESET_DIR = os.path.join(os.path.dirname(os.path.dirname(__file__)), "Presets", "XOceanus")

# Default parameter values (all 86 opal_ params)
DEFAULTS = {
    "opal_source": 1, "opal_oscShape": 0.5, "opal_osc2Shape": 0.5,
    "opal_osc2Mix": 0.5, "opal_osc2Detune": 0.1, "opal_couplingLevel": 0.0,
    "opal_grainSize": 120.0, "opal_density": 20.0, "opal_position": 0.0,
    "opal_posScatter": 0.1, "opal_pitchShift": 0.0, "opal_pitchScatter": 0.0,
    "opal_panScatter": 0.3, "opal_window": 0, "opal_freeze": 0.0,
    "opal_freezeSize": 0.25, "opal_filterCutoff": 8000.0, "opal_filterReso": 0.15,
    "opal_filterMode": 0, "opal_filterKeyTrack": 0.3, "opal_filterDrive": 0.0,
    "opal_shimmer": 0.0, "opal_frost": 0.0,
    "opal_ampAttack": 0.3, "opal_ampDecay": 0.5, "opal_ampSustain": 0.8,
    "opal_ampRelease": 1.5, "opal_ampVelSens": 0.4,
    "opal_filterEnvAmt": 0.3, "opal_filterAttack": 0.5, "opal_filterDecay": 0.8,
    "opal_filterSustain": 0.3, "opal_filterRelease": 1.0,
    "opal_lfo1Shape": 0, "opal_lfo1Rate": 0.5, "opal_lfo1Depth": 0.0,
    "opal_lfo1Sync": 0, "opal_lfo1Retrigger": 0, "opal_lfo1Phase": 0.0,
    "opal_lfo2Shape": 0, "opal_lfo2Rate": 2.0, "opal_lfo2Depth": 0.0,
    "opal_lfo2Sync": 0, "opal_lfo2Retrigger": 0, "opal_lfo2Phase": 0.0,
    "opal_modSlot1Src": 0, "opal_modSlot1Dst": 0, "opal_modSlot1Amt": 0.0,
    "opal_modSlot2Src": 0, "opal_modSlot2Dst": 0, "opal_modSlot2Amt": 0.0,
    "opal_modSlot3Src": 0, "opal_modSlot3Dst": 0, "opal_modSlot3Amt": 0.0,
    "opal_modSlot4Src": 0, "opal_modSlot4Dst": 0, "opal_modSlot4Amt": 0.0,
    "opal_modSlot5Src": 0, "opal_modSlot5Dst": 0, "opal_modSlot5Amt": 0.0,
    "opal_modSlot6Src": 0, "opal_modSlot6Dst": 0, "opal_modSlot6Amt": 0.0,
    "opal_macroScatter": 0.0, "opal_macroDrift": 0.0,
    "opal_macroCoupling": 0.0, "opal_macroSpace": 0.0,
    "opal_fxSmearAmount": 0.0, "opal_fxSmearMix": 0.0,
    "opal_fxReverbSize": 0.4, "opal_fxReverbDecay": 2.0,
    "opal_fxReverbDamping": 0.5, "opal_fxReverbMix": 0.0,
    "opal_fxDelayTime": 0.35, "opal_fxDelayFeedback": 0.3,
    "opal_fxDelayMix": 0.0, "opal_fxDelaySync": 0, "opal_fxDelaySpread": 0.3,
    "opal_fxFinishGlue": 0.1, "opal_fxFinishWidth": 1.0, "opal_fxFinishLevel": 0.75,
    "opal_voiceMode": 0, "opal_glideTime": 0.0, "opal_glideMode": 0,
    "opal_pan": 0.0, "opal_level": 0.75,
}


def make_params(**overrides):
    """Create full parameter dict with overrides applied to defaults."""
    p = dict(DEFAULTS)
    p.update(overrides)
    return p


def make_preset(name, mood, desc, tags, params, dna, engines=None,
                coupling_intensity="None", coupling=None):
    return {
        "schema_version": 1,
        "name": name,
        "mood": mood,
        "engines": engines or ["Opal"],
        "author": "XO_OX Designs",
        "version": "1.0.0",
        "description": desc,
        "tags": tags,
        "macroLabels": ["SCATTER", "DRIFT", "COUPLING", "SPACE"],
        "couplingIntensity": coupling_intensity,
        "tempo": None,
        "dna": dna,
        "parameters": {"Opal": params},
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


def r(lo, hi):
    """Random float in range, rounded to 2 decimals."""
    return round(random.uniform(lo, hi), 2)


# ============================================================================
# CATEGORY 1: Smooth Clouds (20)
# Large grains, low scatter, atmospheric pads
# ============================================================================
SMOOTH_CLOUDS = [
    # Already have: Glass Cloud (Foundation) — skip
    ("Velvet Haze", "Foundation", "Warm saw cloud with gentle LFO drift. Soft, enveloping.",
     ["pad", "warm", "velvet"], 1, 0.3, 250, 12, 0.05, 0.5, 0, 0.2, 5500,
     {"brightness": 0.4, "warmth": 0.7, "movement": 0.3, "density": 0.3, "space": 0.4, "aggression": 0.0}),
    ("Silk Thread", "Foundation", "Pure sine grains woven into delicate translucent texture.",
     ["pad", "delicate", "pure"], 0, 0.5, 300, 10, 0.08, 0.3, 0, 0.1, 6000,
     {"brightness": 0.3, "warmth": 0.8, "movement": 0.2, "density": 0.2, "space": 0.3, "aggression": 0.0}),
    ("Amber Glow", "Foundation", "Two-oscillator warmth. Detuned saw pair granulated into golden haze.",
     ["pad", "warm", "detuned"], 4, 0.4, 180, 18, 0.12, 0.8, 0, 0.15, 7000,
     {"brightness": 0.5, "warmth": 0.7, "movement": 0.3, "density": 0.4, "space": 0.3, "aggression": 0.1}),
    ("Morning Mist", "Atmosphere", "Barely-there sine cloud. Slow attack, wide stereo, touch of reverb.",
     ["ambient", "subtle", "morning"], 0, 0.5, 400, 6, 0.15, 0.3, 1, 0.05, 4000,
     {"brightness": 0.2, "warmth": 0.8, "movement": 0.4, "density": 0.2, "space": 0.7, "aggression": 0.0}),
    ("Cloud Nine", "Atmosphere", "Lush pad from saw grains. High sustain, moderate reverb.",
     ["pad", "lush", "atmosphere"], 1, 0.5, 280, 14, 0.1, 0.6, 0, 0.2, 6500,
     {"brightness": 0.4, "warmth": 0.6, "movement": 0.3, "density": 0.3, "space": 0.5, "aggression": 0.0}),
    ("Warm Blanket", "Foundation", "Dense pulse cloud. Variable width creates hollow warmth.",
     ["pad", "hollow", "warm"], 2, 0.6, 220, 16, 0.08, 0.4, 0, 0.1, 5000,
     {"brightness": 0.3, "warmth": 0.8, "movement": 0.2, "density": 0.4, "space": 0.3, "aggression": 0.0}),
    ("Twilight Wash", "Atmosphere", "Slow-moving saw cloud with gentle filter envelope sweep.",
     ["pad", "sweep", "twilight"], 1, 0.5, 350, 8, 0.2, 0.5, 0, 0.1, 4500,
     {"brightness": 0.3, "warmth": 0.7, "movement": 0.5, "density": 0.2, "space": 0.6, "aggression": 0.0}),
    ("Cashmere", "Foundation", "Ultra-soft triangle grains. Gaussian window for maximum smoothness.",
     ["pad", "soft", "smooth"], 1, 0.5, 320, 10, 0.06, 0.3, 1, 0.0, 5500,
     {"brightness": 0.3, "warmth": 0.9, "movement": 0.2, "density": 0.3, "space": 0.3, "aggression": 0.0}),
    ("Distant Shore", "Atmosphere", "Noise-tinged cloud with heavy reverb. Ocean-like wash.",
     ["ambient", "ocean", "wash"], 3, 0.5, 200, 20, 0.3, 1.0, 0, 0.0, 3000,
     {"brightness": 0.3, "warmth": 0.5, "movement": 0.4, "density": 0.4, "space": 0.8, "aggression": 0.1}),
    ("Sunbeam", "Foundation", "Bright sine cloud with shimmer character. Clear, present.",
     ["bright", "clear", "shimmer"], 0, 0.5, 200, 15, 0.05, 0.2, 0, 0.3, 10000,
     {"brightness": 0.7, "warmth": 0.5, "movement": 0.2, "density": 0.3, "space": 0.2, "aggression": 0.0}),
    ("Still Water", "Atmosphere", "Nearly static sine cloud. Minimal movement, maximum calm.",
     ["ambient", "calm", "still"], 0, 0.5, 500, 4, 0.02, 0.1, 1, 0.0, 4000,
     {"brightness": 0.2, "warmth": 0.8, "movement": 0.1, "density": 0.1, "space": 0.5, "aggression": 0.0}),
    ("Pastel Dream", "Prism", "Two-osc cloud with light shimmer. Dreamy, colorful.",
     ["pad", "dreamy", "colorful"], 4, 0.4, 240, 12, 0.1, 0.5, 0, 0.25, 9000,
     {"brightness": 0.6, "warmth": 0.5, "movement": 0.3, "density": 0.3, "space": 0.4, "aggression": 0.0}),
    ("Cotton Drift", "Foundation", "Slow pulse cloud. Tukey window preserves gentle transients.",
     ["pad", "gentle", "drift"], 2, 0.4, 280, 10, 0.1, 0.4, 2, 0.0, 6000,
     {"brightness": 0.4, "warmth": 0.7, "movement": 0.3, "density": 0.3, "space": 0.3, "aggression": 0.0}),
    ("Vapor Trail", "Atmosphere", "Saw cloud with smear FX. Trailing, dissolving.",
     ["ambient", "trail", "dissolving"], 1, 0.5, 300, 8, 0.2, 0.6, 0, 0.1, 5000,
     {"brightness": 0.3, "warmth": 0.6, "movement": 0.5, "density": 0.2, "space": 0.7, "aggression": 0.0}),
    ("Pearl Surface", "Prism", "Smooth sine cloud with subtle filter movement. Iridescent.",
     ["smooth", "iridescent", "pearl"], 0, 0.5, 260, 14, 0.08, 0.3, 0, 0.2, 7500,
     {"brightness": 0.5, "warmth": 0.6, "movement": 0.3, "density": 0.3, "space": 0.3, "aggression": 0.0}),
    ("Pillow Talk", "Foundation", "Very slow attack, deep sustain. Intimate, close.",
     ["pad", "intimate", "slow"], 1, 0.5, 200, 15, 0.05, 0.3, 0, 0.1, 5000,
     {"brightness": 0.3, "warmth": 0.8, "movement": 0.2, "density": 0.3, "space": 0.2, "aggression": 0.0}),
    ("Honey Flow", "Foundation", "Warm detuned cloud with LFO on position. Slow, viscous movement.",
     ["pad", "warm", "flow"], 4, 0.5, 240, 12, 0.1, 0.5, 0, 0.1, 6000,
     {"brightness": 0.4, "warmth": 0.8, "movement": 0.4, "density": 0.3, "space": 0.3, "aggression": 0.0}),
    ("Feather Light", "Prism", "Minimal saw cloud with wide pan scatter. Delicate, spacious.",
     ["delicate", "spacious", "light"], 1, 0.5, 350, 6, 0.05, 0.2, 0, 0.15, 8000,
     {"brightness": 0.5, "warmth": 0.5, "movement": 0.2, "density": 0.1, "space": 0.5, "aggression": 0.0}),
    ("Moss Garden", "Atmosphere", "Low-filtered noise cloud. Organic, earthy texture.",
     ["ambient", "organic", "earthy"], 3, 0.5, 300, 12, 0.2, 0.8, 0, 0.0, 2000,
     {"brightness": 0.1, "warmth": 0.7, "movement": 0.3, "density": 0.3, "space": 0.5, "aggression": 0.1}),
]

# ============================================================================
# CATEGORY 2: Shimmer Textures (20)
# Small grains, high density, bright airy
# ============================================================================
SHIMMER_TEXTURES = [
    # Already have: Shimmer Veil (Prism) — skip
    ("Crystal Rain", "Prism", "Tiny saw grains cascading. High density, moderate pitch scatter.",
     ["shimmer", "bright", "cascade"], 1, 25, 90, 0.03, 2.0, 0.5, 0.5, 12000,
     {"brightness": 0.8, "warmth": 0.3, "movement": 0.5, "density": 0.7, "space": 0.3, "aggression": 0.1}),
    ("Aurora Dust", "Prism", "Sine micro-grains with shimmer fold. Celestial sparkle.",
     ["shimmer", "celestial", "sparkle"], 0, 20, 100, 0.02, 1.5, 0.4, 0.7, 14000,
     {"brightness": 0.9, "warmth": 0.3, "movement": 0.4, "density": 0.7, "space": 0.3, "aggression": 0.0}),
    ("Star Field", "Atmosphere", "Very small grains, maximum density. Dense stellar shimmer.",
     ["shimmer", "stars", "dense"], 1, 15, 110, 0.04, 3.0, 0.6, 0.6, 15000,
     {"brightness": 0.9, "warmth": 0.2, "movement": 0.6, "density": 0.8, "space": 0.4, "aggression": 0.1}),
    ("Tinsel Breeze", "Prism", "Pulse micro-grains. Narrow width for metallic shimmer.",
     ["shimmer", "metallic", "breeze"], 2, 15, 85, 0.02, 1.0, 0.4, 0.5, 11000,
     {"brightness": 0.7, "warmth": 0.2, "movement": 0.4, "density": 0.6, "space": 0.3, "aggression": 0.2}),
    ("Glass Bells", "Prism", "Sine grains at extreme density. Bell-like overtones from shimmer.",
     ["bell", "glass", "overtones"], 0, 12, 120, 0.01, 0.5, 0.3, 0.8, 16000,
     {"brightness": 0.9, "warmth": 0.3, "movement": 0.3, "density": 0.8, "space": 0.2, "aggression": 0.0}),
    ("Ice Crystals", "Flux", "Frost + shimmer combo. Cold, sharp, crystalline.",
     ["cold", "sharp", "crystal"], 1, 20, 95, 0.05, 2.5, 0.6, 0.5, 13000,
     {"brightness": 0.8, "warmth": 0.1, "movement": 0.5, "density": 0.7, "space": 0.2, "aggression": 0.3}),
    ("Fairy Dust", "Atmosphere", "Ultra-light sine shimmer. Barely audible sparkle.",
     ["shimmer", "fairy", "delicate"], 0, 18, 70, 0.03, 1.0, 0.5, 0.6, 10000,
     {"brightness": 0.7, "warmth": 0.4, "movement": 0.3, "density": 0.5, "space": 0.5, "aggression": 0.0}),
    ("Neon Mist", "Prism", "Saw shimmer with band-pass filter. Focused, electric.",
     ["shimmer", "neon", "focused"], 1, 22, 80, 0.04, 1.5, 0.5, 0.4, 8000,
     {"brightness": 0.6, "warmth": 0.3, "movement": 0.4, "density": 0.6, "space": 0.3, "aggression": 0.2}),
    ("Silver Thread", "Foundation", "Clean sine micro-grains. Pure harmonic shimmer.",
     ["shimmer", "clean", "harmonic"], 0, 25, 60, 0.02, 0.3, 0.3, 0.4, 12000,
     {"brightness": 0.7, "warmth": 0.5, "movement": 0.2, "density": 0.5, "space": 0.2, "aggression": 0.0}),
    ("Pixel Rain", "Flux", "Rectangular window micro-grains. Deliberate digital sparkle.",
     ["digital", "sparkle", "pixel"], 2, 10, 100, 0.06, 4.0, 0.7, 0.3, 14000,
     {"brightness": 0.8, "warmth": 0.1, "movement": 0.7, "density": 0.8, "space": 0.2, "aggression": 0.4}),
    ("Prism Spray", "Prism", "Two-osc grains with heavy shimmer. Rainbow harmonics.",
     ["shimmer", "rainbow", "harmonics"], 4, 28, 75, 0.03, 2.0, 0.5, 0.7, 11000,
     {"brightness": 0.8, "warmth": 0.4, "movement": 0.4, "density": 0.6, "space": 0.3, "aggression": 0.1}),
    ("Dew Drops", "Foundation", "Tiny clean grains. Gentle pitch scatter like droplets.",
     ["gentle", "droplets", "clean"], 0, 15, 50, 0.02, 2.0, 0.4, 0.3, 9000,
     {"brightness": 0.6, "warmth": 0.5, "movement": 0.3, "density": 0.4, "space": 0.3, "aggression": 0.0}),
    ("Halo Ring", "Atmosphere", "Sustained sine shimmer cloud. Angelic, hovering.",
     ["angelic", "sustained", "halo"], 0, 20, 65, 0.04, 0.5, 0.3, 0.6, 10000,
     {"brightness": 0.7, "warmth": 0.5, "movement": 0.3, "density": 0.5, "space": 0.6, "aggression": 0.0}),
    ("Glitter Storm", "Flux", "Maximum density saw grains. Chaotic brightness.",
     ["chaotic", "bright", "glitter"], 1, 12, 120, 0.08, 5.0, 0.8, 0.4, 16000,
     {"brightness": 1.0, "warmth": 0.1, "movement": 0.8, "density": 0.9, "space": 0.2, "aggression": 0.5}),
    ("Opal Fire", "Prism", "The engine's signature sound. Saw + shimmer + frost interplay.",
     ["signature", "fire", "opal"], 1, 22, 85, 0.04, 1.5, 0.5, 0.5, 11000,
     {"brightness": 0.7, "warmth": 0.4, "movement": 0.5, "density": 0.6, "space": 0.3, "aggression": 0.2}),
    ("Wind Chime", "Foundation", "Sine grains with high pitch scatter. Random bell tones.",
     ["bell", "chime", "random"], 0, 30, 40, 0.02, 5.0, 0.5, 0.5, 12000,
     {"brightness": 0.7, "warmth": 0.4, "movement": 0.4, "density": 0.4, "space": 0.4, "aggression": 0.0}),
    ("Laser Grid", "Flux", "Pulse micro-grains with filter resonance. Sharp, precise.",
     ["sharp", "precise", "laser"], 2, 10, 90, 0.01, 0.5, 0.3, 0.3, 8000,
     {"brightness": 0.6, "warmth": 0.2, "movement": 0.5, "density": 0.7, "space": 0.1, "aggression": 0.4}),
    ("Whisper Tone", "Atmosphere", "Very quiet sine shimmer. Background texture only.",
     ["quiet", "background", "whisper"], 0, 25, 45, 0.05, 1.0, 0.4, 0.4, 7000,
     {"brightness": 0.4, "warmth": 0.6, "movement": 0.2, "density": 0.3, "space": 0.5, "aggression": 0.0}),
    ("Diamond Dust", "Prism", "Maximum shimmer, moderate density. Precious, brilliant.",
     ["precious", "brilliant", "diamond"], 1, 18, 70, 0.02, 1.0, 0.4, 0.9, 15000,
     {"brightness": 0.9, "warmth": 0.3, "movement": 0.3, "density": 0.5, "space": 0.3, "aggression": 0.1}),
]

# ============================================================================
# CATEGORY 3: Frozen Moments (15)
# High freeze, slow movement
# ============================================================================
FROZEN_MOMENTS = [
    # Already have: Frozen Bloom (Atmosphere) — skip
    ("Time Stop", "Atmosphere", "Complete freeze on saw material. Time stands still.",
     ["frozen", "still", "suspended"], 1, 300, 10, 0.1, 0.5, 0.3, 1.0, 5000,
     {"brightness": 0.3, "warmth": 0.6, "movement": 0.1, "density": 0.3, "space": 0.6, "aggression": 0.0}),
    ("Amber Trap", "Atmosphere", "Frozen noise texture. Insect-in-amber stillness.",
     ["frozen", "noise", "amber"], 3, 200, 15, 0.2, 1.0, 0.5, 0.9, 4000,
     {"brightness": 0.3, "warmth": 0.5, "movement": 0.2, "density": 0.4, "space": 0.5, "aggression": 0.1}),
    ("Glacier Breath", "Aether", "Frozen sine with frost limiter. Cold, vast, ancient.",
     ["frozen", "glacier", "vast"], 0, 500, 4, 0.15, 0.3, 0.6, 0.85, 3000,
     {"brightness": 0.2, "warmth": 0.4, "movement": 0.2, "density": 0.2, "space": 0.9, "aggression": 0.0}),
    ("Memory Shard", "Prism", "Frozen two-osc. Snapshot of a detuned moment.",
     ["frozen", "memory", "snapshot"], 4, 250, 12, 0.1, 0.8, 0.4, 0.8, 7000,
     {"brightness": 0.5, "warmth": 0.5, "movement": 0.3, "density": 0.3, "space": 0.4, "aggression": 0.1}),
    ("Held Note", "Foundation", "Simple saw freeze. Musical, like sustain pedal forever.",
     ["frozen", "sustained", "musical"], 1, 200, 15, 0.05, 0.3, 0.2, 0.75, 6000,
     {"brightness": 0.4, "warmth": 0.6, "movement": 0.2, "density": 0.3, "space": 0.3, "aggression": 0.0}),
    ("Suspended Dust", "Aether", "Frozen noise particles hovering in reverb space.",
     ["frozen", "dust", "hovering"], 3, 150, 20, 0.3, 2.0, 0.7, 0.9, 3500,
     {"brightness": 0.3, "warmth": 0.4, "movement": 0.3, "density": 0.4, "space": 0.8, "aggression": 0.1}),
    ("Fossil Light", "Atmosphere", "Frozen shimmer texture. Ancient brightness preserved.",
     ["frozen", "ancient", "shimmer"], 0, 100, 50, 0.05, 1.0, 0.3, 0.85, 10000,
     {"brightness": 0.7, "warmth": 0.3, "movement": 0.2, "density": 0.5, "space": 0.5, "aggression": 0.0}),
    ("Ice Palace", "Prism", "Full freeze on pulse grains with frost. Architectural cold.",
     ["frozen", "cold", "palace"], 2, 180, 12, 0.08, 0.5, 0.5, 1.0, 8000,
     {"brightness": 0.5, "warmth": 0.2, "movement": 0.1, "density": 0.3, "space": 0.5, "aggression": 0.2}),
    ("Petrified Wave", "Foundation", "Frozen saw wave. The moment a wave became stone.",
     ["frozen", "wave", "stone"], 1, 350, 8, 0.05, 0.2, 0.2, 0.9, 5500,
     {"brightness": 0.4, "warmth": 0.6, "movement": 0.1, "density": 0.2, "space": 0.4, "aggression": 0.0}),
    ("Locked Garden", "Atmosphere", "Frozen two-osc with smear. Time-locked beauty.",
     ["frozen", "garden", "beauty"], 4, 280, 10, 0.12, 0.5, 0.4, 0.8, 6500,
     {"brightness": 0.4, "warmth": 0.6, "movement": 0.3, "density": 0.3, "space": 0.6, "aggression": 0.0}),
    ("Zero Kelvin", "Aether", "Maximum freeze + frost. Absolute cold.",
     ["frozen", "absolute", "cold"], 0, 400, 3, 0.05, 0.1, 0.3, 1.0, 2500,
     {"brightness": 0.1, "warmth": 0.2, "movement": 0.1, "density": 0.1, "space": 0.8, "aggression": 0.1}),
    ("Stasis Field", "Flux", "Frozen pulse with modulation still running. Frozen but alive.",
     ["frozen", "alive", "stasis"], 2, 150, 25, 0.1, 1.5, 0.4, 0.7, 7000,
     {"brightness": 0.5, "warmth": 0.4, "movement": 0.4, "density": 0.4, "space": 0.3, "aggression": 0.2}),
    ("Eternal Tone", "Aether", "Sine freeze with maximum reverb. Infinite sustain.",
     ["frozen", "eternal", "infinite"], 0, 600, 2, 0.02, 0.1, 0.2, 0.95, 3500,
     {"brightness": 0.2, "warmth": 0.7, "movement": 0.1, "density": 0.1, "space": 1.0, "aggression": 0.0}),
    ("Captured Bloom", "Atmosphere", "Frozen at the peak of a filter sweep. Moment preserved.",
     ["frozen", "bloom", "captured"], 1, 220, 14, 0.1, 0.5, 0.3, 0.85, 9000,
     {"brightness": 0.6, "warmth": 0.5, "movement": 0.2, "density": 0.3, "space": 0.5, "aggression": 0.0}),
]

# ============================================================================
# CATEGORY 4: Scattered Glass (15)
# High pitch scatter, rhythmic density
# ============================================================================
SCATTERED_GLASS = [
    # Already have: Particle Storm (Flux), Scatter Glass (Flux) — skip
    ("Broken Mirror", "Flux", "High scatter pulse grains. Shattered reflections.",
     ["scatter", "broken", "sharp"], 2, 20, 50, 0.4, 8.0, 0.7, 0.0, 10000,
     {"brightness": 0.6, "warmth": 0.2, "movement": 0.7, "density": 0.6, "space": 0.2, "aggression": 0.5}),
    ("Hail Stones", "Flux", "Rectangular noise grains. Hard percussive impacts.",
     ["percussive", "hard", "hail"], 3, 12, 80, 0.5, 6.0, 0.9, 0.0, 12000,
     {"brightness": 0.7, "warmth": 0.1, "movement": 0.8, "density": 0.7, "space": 0.1, "aggression": 0.8}),
    ("Prism Shatter", "Prism", "Saw grains with wide pitch scatter. Chromatic explosion.",
     ["scatter", "chromatic", "prism"], 1, 25, 60, 0.3, 10.0, 0.6, 0.2, 11000,
     {"brightness": 0.7, "warmth": 0.3, "movement": 0.7, "density": 0.6, "space": 0.3, "aggression": 0.4}),
    ("Gravel Path", "Foundation", "Noise micro-grains. Footsteps on gravel texture.",
     ["texture", "gravel", "organic"], 3, 15, 70, 0.2, 3.0, 0.5, 0.0, 5000,
     {"brightness": 0.3, "warmth": 0.4, "movement": 0.5, "density": 0.5, "space": 0.2, "aggression": 0.3}),
    ("Electric Swarm", "Flux", "Dense pulse swarm with pitch chaos.",
     ["swarm", "electric", "chaos"], 2, 10, 100, 0.6, 12.0, 0.8, 0.0, 14000,
     {"brightness": 0.8, "warmth": 0.1, "movement": 0.9, "density": 0.8, "space": 0.1, "aggression": 0.7}),
    ("Crushed Ice", "Prism", "Sine grains with frost limiter + scatter. Cold sparkle.",
     ["cold", "sparkle", "crushed"], 0, 18, 55, 0.3, 6.0, 0.6, 0.0, 9000,
     {"brightness": 0.6, "warmth": 0.2, "movement": 0.6, "density": 0.5, "space": 0.3, "aggression": 0.3}),
    ("Tectonic Shift", "Flux", "Low-frequency scattered grains. Rumbling, geological.",
     ["deep", "rumble", "tectonic"], 1, 40, 30, 0.4, 8.0, 0.7, 0.0, 2000,
     {"brightness": 0.1, "warmth": 0.5, "movement": 0.6, "density": 0.4, "space": 0.4, "aggression": 0.5}),
    ("Firecracker", "Flux", "Burst of rectangular grains. Explosive, percussive.",
     ["burst", "explosive", "percussive"], 3, 10, 90, 0.5, 7.0, 0.9, 0.0, 15000,
     {"brightness": 0.8, "warmth": 0.1, "movement": 0.9, "density": 0.8, "space": 0.1, "aggression": 0.9}),
    ("Fracture Line", "Prism", "Two-osc with opposing scatter. Splitting apart.",
     ["fracture", "split", "opposing"], 4, 22, 45, 0.35, 9.0, 0.7, 0.15, 8000,
     {"brightness": 0.5, "warmth": 0.3, "movement": 0.7, "density": 0.5, "space": 0.3, "aggression": 0.4}),
    ("Sand Blast", "Foundation", "Dense noise scatter. Industrial sandpaper texture.",
     ["noise", "industrial", "sand"], 3, 10, 80, 0.4, 4.0, 0.6, 0.0, 6000,
     {"brightness": 0.4, "warmth": 0.3, "movement": 0.6, "density": 0.6, "space": 0.1, "aggression": 0.5}),
    ("Meteor Shower", "Aether", "Scattered sine grains falling through reverb space.",
     ["scatter", "falling", "meteor"], 0, 30, 35, 0.5, 12.0, 0.8, 0.2, 7000,
     {"brightness": 0.5, "warmth": 0.4, "movement": 0.7, "density": 0.4, "space": 0.7, "aggression": 0.2}),
    ("Static Burst", "Flux", "Short noise bursts. Radio static, electrified.",
     ["static", "burst", "electric"], 3, 8, 110, 0.3, 2.0, 0.5, 0.0, 16000,
     {"brightness": 0.9, "warmth": 0.0, "movement": 0.8, "density": 0.9, "space": 0.1, "aggression": 0.7}),
    ("Kaleidoscope", "Prism", "Maximum pitch scatter on saw. Every grain a different note.",
     ["scatter", "colorful", "kaleidoscope"], 1, 20, 50, 0.2, 24.0, 0.6, 0.3, 10000,
     {"brightness": 0.6, "warmth": 0.3, "movement": 0.8, "density": 0.5, "space": 0.3, "aggression": 0.3}),
]

# ============================================================================
# CATEGORY 5: Coupling Showcases (20)
# Cross-engine specific pairs
# ============================================================================
COUPLING_SHOWCASES = [
    ("Chip Scatter", "Entangled", "OVERWORLD audio granulated. Retro chips through time clouds.",
     ["coupling", "retro", "chip"], "Overworld", "AudioToWavetable",
     {"brightness": 0.6, "warmth": 0.3, "movement": 0.8, "density": 0.6, "space": 0.5, "aggression": 0.3}),
    ("Climax Particles", "Entangled", "ODYSSEY Climax bloom frozen mid-bloom into particles.",
     ["coupling", "climax", "bloom"], "Odyssey", "AudioToWavetable",
     {"brightness": 0.4, "warmth": 0.7, "movement": 0.7, "density": 0.5, "space": 0.7, "aggression": 0.2}),
    ("Granular Dub", "Entangled", "OPAL cloud through OVERDUB echo chain. Granular dub.",
     ["coupling", "dub", "echo"], "Overdub", "AudioToWavetable",
     {"brightness": 0.4, "warmth": 0.5, "movement": 0.6, "density": 0.4, "space": 0.6, "aggression": 0.2}),
    ("Bass Breath", "Entangled", "OPAL density drives OVERBITE filter. Breathing bass.",
     ["coupling", "bass", "breath"], "Overbite", "AmpToFilter",
     {"brightness": 0.3, "warmth": 0.6, "movement": 0.5, "density": 0.5, "space": 0.3, "aggression": 0.3}),
    ("Pluck Cloud", "Entangled", "ODDFELIX pluck granulated. Reverb made of its own attack.",
     ["coupling", "pluck", "cloud"], "OddfeliX", "AudioToWavetable",
     {"brightness": 0.5, "warmth": 0.5, "movement": 0.6, "density": 0.5, "space": 0.5, "aggression": 0.2}),
    ("Fat Cloud", "Entangled", "OBESE amplitude modulates OPAL filter. Rhythmic brightness.",
     ["coupling", "fat", "rhythmic"], "Obese", "AmpToFilter",
     {"brightness": 0.5, "warmth": 0.4, "movement": 0.6, "density": 0.6, "space": 0.3, "aggression": 0.4}),
    ("Morph Scatter", "Entangled", "OPAL envelope drives ODDOSCAR morph. Cloud-reactive wavetable.",
     ["coupling", "morph", "reactive"], "OddOscar", "EnvToMorph",
     {"brightness": 0.5, "warmth": 0.5, "movement": 0.7, "density": 0.5, "space": 0.4, "aggression": 0.2}),
    ("Drum Cloud", "Entangled", "ONSET rhythms drive OPAL grain density. Rhythmic clouds.",
     ["coupling", "drum", "rhythmic"], "Onset", "RhythmToBlend",
     {"brightness": 0.4, "warmth": 0.4, "movement": 0.7, "density": 0.6, "space": 0.3, "aggression": 0.3}),
    ("Warm Scatter", "Entangled", "OBLONG warmth granulated through OPAL. Organic cloud material.",
     ["coupling", "warm", "organic"], "Oblong", "AudioToWavetable",
     {"brightness": 0.3, "warmth": 0.7, "movement": 0.5, "density": 0.4, "space": 0.5, "aggression": 0.1}),
    ("NES Shimmer", "Entangled", "OVERWORLD pulse wave through shimmer grains. 8-bit sparkle.",
     ["coupling", "8bit", "sparkle"], "Overworld", "AudioToWavetable",
     {"brightness": 0.7, "warmth": 0.2, "movement": 0.6, "density": 0.6, "space": 0.3, "aggression": 0.2}),
    ("Frozen Strings", "Entangled", "ODDFELIX Karplus-Strong frozen in OPAL buffer. Frozen strum.",
     ["coupling", "strings", "frozen"], "OddfeliX", "AudioToWavetable",
     {"brightness": 0.4, "warmth": 0.6, "movement": 0.3, "density": 0.4, "space": 0.6, "aggression": 0.1}),
    ("Chaos Feed", "Entangled", "OUROBOROS chaos signal granulated. Strange attractor clouds.",
     ["coupling", "chaos", "attractor"], "Ouroboros", "AudioToWavetable",
     {"brightness": 0.5, "warmth": 0.3, "movement": 0.9, "density": 0.6, "space": 0.4, "aggression": 0.5}),
    ("Organ Freeze", "Entangled", "ORGANON tones frozen in OPAL buffer. Suspended organ.",
     ["coupling", "organ", "suspended"], "Organon", "AudioToWavetable",
     {"brightness": 0.4, "warmth": 0.6, "movement": 0.3, "density": 0.4, "space": 0.5, "aggression": 0.1}),
    ("Bite Wash", "Entangled", "OVERBITE bass granulated into wash texture.",
     ["coupling", "bass", "wash"], "Overbite", "AudioToWavetable",
     {"brightness": 0.3, "warmth": 0.6, "movement": 0.5, "density": 0.4, "space": 0.5, "aggression": 0.2}),
    ("Portal Open", "Entangled", "COUPLING macro showcase. Sweep from internal to external audio.",
     ["coupling", "portal", "sweep"], "Overworld", "AudioToWavetable",
     {"brightness": 0.5, "warmth": 0.4, "movement": 0.6, "density": 0.5, "space": 0.4, "aggression": 0.2}),
    ("Echo Particles", "Entangled", "OPAL cloud through OVERDUB spring reverb. Granular spring.",
     ["coupling", "spring", "echo"], "Overdub", "AudioToWavetable",
     {"brightness": 0.4, "warmth": 0.5, "movement": 0.5, "density": 0.4, "space": 0.6, "aggression": 0.1}),
    ("Pixel Freeze", "Entangled", "OVERWORLD chiptune frozen at maximum freeze. Retro amber.",
     ["coupling", "retro", "frozen"], "Overworld", "AudioToWavetable",
     {"brightness": 0.5, "warmth": 0.3, "movement": 0.2, "density": 0.4, "space": 0.4, "aggression": 0.1}),
    ("Drift Morph", "Entangled", "ODDOSCAR morph position driven by OPAL envelope. Cloud-shaped morph.",
     ["coupling", "morph", "drift"], "OddOscar", "EnvToMorph",
     {"brightness": 0.5, "warmth": 0.5, "movement": 0.6, "density": 0.4, "space": 0.4, "aggression": 0.1}),
    ("Rhythm Filter", "Entangled", "ONSET amplitude opens OPAL filter. Beat-synced brightness.",
     ["coupling", "rhythm", "filter"], "Onset", "AmpToFilter",
     {"brightness": 0.5, "warmth": 0.4, "movement": 0.7, "density": 0.5, "space": 0.2, "aggression": 0.3}),
    ("Cloud Cascade", "Entangled", "ODYSSEY pad audio scattered through time. Psychedelic granular.",
     ["coupling", "psychedelic", "cascade"], "Odyssey", "AudioToWavetable",
     {"brightness": 0.5, "warmth": 0.5, "movement": 0.7, "density": 0.5, "space": 0.6, "aggression": 0.2}),
]

# ============================================================================
# CATEGORY 6: Deep Drift (10)
# Max scatter, generative/ambient
# ============================================================================
DEEP_DRIFT = [
    # Already have: Deep Drift (Aether) — skip
    ("Void Whisper", "Aether", "Near-silent sine drift. LFO modulates everything slowly.",
     ["ambient", "void", "whisper"], 0, 700, 3, 0.8, 10.0, 0.9, 0.0, 2500,
     {"brightness": 0.1, "warmth": 0.6, "movement": 0.5, "density": 0.1, "space": 0.9, "aggression": 0.0}),
    ("Cosmic Wash", "Aether", "Two-osc drift with max scatter. Everything dissolving.",
     ["ambient", "cosmic", "dissolving"], 4, 500, 5, 0.7, 15.0, 0.8, 0.1, 3000,
     {"brightness": 0.2, "warmth": 0.5, "movement": 0.7, "density": 0.2, "space": 0.9, "aggression": 0.0}),
    ("Event Horizon", "Aether", "Noise drift at extreme scatter. Beyond comprehension.",
     ["ambient", "extreme", "horizon"], 3, 400, 6, 0.9, 20.0, 1.0, 0.0, 2000,
     {"brightness": 0.2, "warmth": 0.3, "movement": 0.8, "density": 0.2, "space": 0.8, "aggression": 0.2}),
    ("Slow Glass", "Aether", "Saw drift through frozen time. Light takes years to pass.",
     ["ambient", "slow", "glass"], 1, 800, 2, 0.5, 8.0, 0.7, 0.15, 4000,
     {"brightness": 0.3, "warmth": 0.6, "movement": 0.4, "density": 0.1, "space": 0.8, "aggression": 0.0}),
    ("Dark Matter", "Aether", "Sub-bass drift. Felt more than heard.",
     ["ambient", "dark", "deep"], 1, 600, 3, 0.6, 5.0, 0.8, 0.0, 1500,
     {"brightness": 0.0, "warmth": 0.5, "movement": 0.4, "density": 0.1, "space": 0.7, "aggression": 0.1}),
    ("Nebula Glow", "Aether", "Warm sine drift with maximum reverb and smear.",
     ["ambient", "nebula", "warm"], 0, 500, 4, 0.6, 8.0, 0.7, 0.1, 3500,
     {"brightness": 0.2, "warmth": 0.7, "movement": 0.5, "density": 0.1, "space": 1.0, "aggression": 0.0}),
    ("Entropy Rise", "Aether", "Gradually increasing scatter. Order dissolves to chaos.",
     ["ambient", "entropy", "dissolving"], 1, 450, 5, 0.5, 12.0, 0.6, 0.05, 3000,
     {"brightness": 0.2, "warmth": 0.5, "movement": 0.6, "density": 0.2, "space": 0.8, "aggression": 0.1}),
    ("Liminal Space", "Aether", "Pulse drift in between states. Neither here nor there.",
     ["ambient", "liminal", "between"], 2, 550, 4, 0.7, 10.0, 0.8, 0.0, 2500,
     {"brightness": 0.2, "warmth": 0.4, "movement": 0.5, "density": 0.1, "space": 0.8, "aggression": 0.1}),
    ("Tidal Pull", "Aether", "Slow-cycling saw drift. LFO on density creates breathing.",
     ["ambient", "tidal", "breathing"], 1, 600, 4, 0.5, 7.0, 0.7, 0.05, 3000,
     {"brightness": 0.2, "warmth": 0.6, "movement": 0.6, "density": 0.2, "space": 0.8, "aggression": 0.0}),
]


# ============================================================================
# Build functions
# ============================================================================

def build_smooth_cloud(name, mood, desc, tags, source, shape, grain_size, density,
                       pos_scatter, pitch_scatter, window, shimmer, cutoff, dna):
    attack = r(0.2, 1.5)
    release = r(1.0, 4.0)
    params = make_params(
        opal_source=source, opal_oscShape=shape, opal_grainSize=grain_size,
        opal_density=density, opal_posScatter=pos_scatter,
        opal_pitchScatter=pitch_scatter, opal_panScatter=r(0.2, 0.6),
        opal_window=window, opal_shimmer=shimmer, opal_filterCutoff=cutoff,
        opal_ampAttack=attack, opal_ampDecay=r(0.3, 1.0),
        opal_ampSustain=r(0.6, 0.9), opal_ampRelease=release,
        opal_filterEnvAmt=r(0.05, 0.3), opal_filterAttack=r(0.3, 1.0),
        opal_filterDecay=r(0.5, 1.5), opal_filterSustain=r(0.2, 0.5),
        opal_filterRelease=r(0.5, 2.0),
        opal_lfo1Rate=r(0.1, 0.8), opal_lfo1Depth=r(0.1, 0.3),
        opal_modSlot1Src=1, opal_modSlot1Dst=3, opal_modSlot1Amt=r(0.1, 0.4),
        opal_fxReverbSize=r(0.2, 0.6), opal_fxReverbDecay=r(1.0, 4.0),
        opal_fxReverbMix=r(0.05, 0.3),
        opal_fxSmearAmount=r(0.0, 0.3), opal_fxSmearMix=r(0.0, 0.2),
        opal_fxFinishWidth=r(1.0, 1.4),
    )
    return make_preset(name, mood, desc, tags, params, dna)


def build_shimmer_texture(name, mood, desc, tags, source, grain_size, density,
                          pos_scatter, pitch_scatter, pan_scatter, shimmer, cutoff, dna):
    params = make_params(
        opal_source=source, opal_grainSize=grain_size, opal_density=density,
        opal_posScatter=pos_scatter, opal_pitchScatter=pitch_scatter,
        opal_panScatter=pan_scatter, opal_shimmer=shimmer,
        opal_filterCutoff=cutoff, opal_filterReso=r(0.1, 0.3),
        opal_ampAttack=r(0.1, 0.4), opal_ampDecay=r(0.3, 0.8),
        opal_ampSustain=r(0.6, 0.8), opal_ampRelease=r(0.8, 2.0),
        opal_filterEnvAmt=r(0.1, 0.4), opal_filterAttack=r(0.1, 0.5),
        opal_filterDecay=r(0.3, 0.8), opal_filterSustain=r(0.2, 0.5),
        opal_lfo1Rate=r(0.3, 2.0), opal_lfo1Depth=r(0.1, 0.3),
        opal_modSlot1Src=1, opal_modSlot1Dst=1, opal_modSlot1Amt=r(0.1, 0.4),
        opal_fxReverbSize=r(0.15, 0.5), opal_fxReverbDecay=r(0.8, 3.0),
        opal_fxReverbMix=r(0.1, 0.3), opal_fxFinishWidth=r(1.0, 1.5),
    )
    return make_preset(name, mood, desc, tags, params, dna)


def build_frozen_moment(name, mood, desc, tags, source, grain_size, density,
                        pos_scatter, pitch_scatter, pan_scatter, freeze, cutoff, dna):
    frost_val = r(0.0, 0.4) if "frost" in desc.lower() or "cold" in desc.lower() else 0.0
    shimmer_val = r(0.0, 0.3) if "shimmer" in desc.lower() else 0.0
    params = make_params(
        opal_source=source, opal_grainSize=grain_size, opal_density=density,
        opal_posScatter=pos_scatter, opal_pitchScatter=pitch_scatter,
        opal_panScatter=pan_scatter, opal_freeze=freeze, opal_freezeSize=r(0.2, 0.6),
        opal_shimmer=shimmer_val, opal_frost=frost_val,
        opal_filterCutoff=cutoff, opal_filterReso=r(0.05, 0.2),
        opal_ampAttack=r(0.5, 2.0), opal_ampDecay=r(0.5, 1.5),
        opal_ampSustain=r(0.7, 0.95), opal_ampRelease=r(2.0, 6.0),
        opal_lfo1Rate=r(0.05, 0.3), opal_lfo1Depth=r(0.1, 0.4),
        opal_modSlot1Src=1, opal_modSlot1Dst=3, opal_modSlot1Amt=r(0.2, 0.5),
        opal_fxReverbSize=r(0.4, 0.8), opal_fxReverbDecay=r(2.0, 6.0),
        opal_fxReverbMix=r(0.2, 0.5),
        opal_fxSmearAmount=r(0.1, 0.4), opal_fxSmearMix=r(0.1, 0.3),
        opal_fxFinishWidth=r(1.0, 1.5),
    )
    return make_preset(name, mood, desc, tags, params, dna)


def build_scattered_glass(name, mood, desc, tags, source, grain_size, density,
                          pos_scatter, pitch_scatter, pan_scatter, frost, cutoff, dna):
    params = make_params(
        opal_source=source, opal_grainSize=grain_size, opal_density=density,
        opal_posScatter=pos_scatter, opal_pitchScatter=pitch_scatter,
        opal_panScatter=pan_scatter, opal_window=3, opal_frost=frost,
        opal_filterCutoff=cutoff, opal_filterReso=r(0.2, 0.5),
        opal_filterDrive=r(0.1, 0.3),
        opal_ampAttack=r(0.001, 0.05), opal_ampDecay=r(0.1, 0.4),
        opal_ampSustain=r(0.4, 0.7), opal_ampRelease=r(0.2, 0.8),
        opal_ampVelSens=r(0.5, 0.9),
        opal_filterEnvAmt=r(0.3, 0.7), opal_filterAttack=0.001,
        opal_filterDecay=r(0.1, 0.4), opal_filterSustain=r(0.1, 0.3),
        opal_lfo1Shape=random.choice([3, 4]), opal_lfo1Rate=r(2.0, 10.0),
        opal_lfo1Depth=r(0.2, 0.5),
        opal_modSlot1Src=1, opal_modSlot1Dst=2, opal_modSlot1Amt=r(0.3, 0.6),
        opal_fxDelayTime=r(0.08, 0.2), opal_fxDelayFeedback=r(0.2, 0.5),
        opal_fxDelayMix=r(0.05, 0.15), opal_fxDelaySpread=r(0.3, 0.7),
        opal_fxFinishGlue=r(0.15, 0.3), opal_fxFinishWidth=r(1.2, 1.6),
    )
    return make_preset(name, mood, desc, tags, params, dna)


def build_coupling_showcase(name, mood, desc, tags, partner, coupling_type, dna):
    is_audio_coupling = (coupling_type == "AudioToWavetable")
    params = make_params(
        opal_source=5 if is_audio_coupling else 1,
        opal_couplingLevel=0.8 if is_audio_coupling else 0.0,
        opal_grainSize=r(80, 300), opal_density=r(10, 40),
        opal_posScatter=r(0.1, 0.4), opal_pitchScatter=r(0.5, 5.0),
        opal_panScatter=r(0.3, 0.6), opal_window=random.choice([0, 1]),
        opal_freeze=r(0.0, 0.5) if is_audio_coupling else 0.0,
        opal_shimmer=r(0.0, 0.3), opal_filterCutoff=r(4000, 10000),
        opal_ampAttack=r(0.1, 0.5), opal_ampDecay=r(0.3, 0.8),
        opal_ampSustain=r(0.6, 0.9), opal_ampRelease=r(1.0, 3.0),
        opal_lfo1Rate=r(0.2, 1.0), opal_lfo1Depth=r(0.1, 0.3),
        opal_modSlot1Src=1, opal_modSlot1Dst=3, opal_modSlot1Amt=r(0.1, 0.4),
        opal_fxReverbSize=r(0.3, 0.6), opal_fxReverbDecay=r(1.5, 4.0),
        opal_fxReverbMix=r(0.1, 0.4),
        opal_fxFinishWidth=r(1.0, 1.4),
    )
    coupling_data = {"pairs": [{"source": partner, "target": "Opal", "type": coupling_type, "amount": 0.7}]}
    return make_preset(name, mood, desc, tags, params, dna,
                       engines=["Opal", partner], coupling_intensity="Medium",
                       coupling=coupling_data)


def build_deep_drift(name, mood, desc, tags, source, grain_size, density,
                     pos_scatter, pitch_scatter, pan_scatter, shimmer, cutoff, dna):
    params = make_params(
        opal_source=source, opal_grainSize=grain_size, opal_density=density,
        opal_position=r(0.1, 0.5), opal_posScatter=pos_scatter,
        opal_pitchScatter=pitch_scatter, opal_panScatter=pan_scatter,
        opal_window=1, opal_shimmer=shimmer, opal_filterCutoff=cutoff,
        opal_filterReso=r(0.02, 0.1),
        opal_ampAttack=r(1.5, 3.0), opal_ampDecay=r(1.5, 3.0),
        opal_ampSustain=r(0.6, 0.8), opal_ampRelease=r(4.0, 8.0),
        opal_ampVelSens=r(0.0, 0.2),
        opal_filterAttack=r(1.0, 3.0), opal_filterDecay=r(1.5, 3.0),
        opal_filterSustain=r(0.3, 0.6), opal_filterRelease=r(3.0, 6.0),
        opal_lfo1Rate=r(0.02, 0.1), opal_lfo1Depth=r(0.3, 0.6),
        opal_lfo2Shape=4, opal_lfo2Rate=r(0.05, 0.2), opal_lfo2Depth=r(0.2, 0.4),
        opal_modSlot1Src=1, opal_modSlot1Dst=3, opal_modSlot1Amt=r(0.4, 0.7),
        opal_modSlot2Src=2, opal_modSlot2Dst=5, opal_modSlot2Amt=r(0.2, 0.5),
        opal_fxSmearAmount=r(0.3, 0.7), opal_fxSmearMix=r(0.4, 0.8),
        opal_fxReverbSize=r(0.7, 1.0), opal_fxReverbDecay=r(5.0, 10.0),
        opal_fxReverbDamping=r(0.2, 0.4), opal_fxReverbMix=r(0.4, 0.7),
        opal_fxDelayTime=r(0.5, 1.0), opal_fxDelayFeedback=r(0.3, 0.6),
        opal_fxDelayMix=r(0.1, 0.3), opal_fxDelaySpread=r(0.5, 0.9),
        opal_fxFinishWidth=r(1.4, 2.0), opal_fxFinishLevel=r(0.6, 0.75),
    )
    return make_preset(name, mood, desc, tags, params, dna)


# ============================================================================
# Main
# ============================================================================

def main():
    presets = []
    count = 0

    # Smooth Clouds
    for sc in SMOOTH_CLOUDS:
        name, mood, desc, tags, source, shape, gs, dens, ps, pitchs, win, shim, cut, dna = sc
        presets.append(build_smooth_cloud(name, mood, desc, tags, source, shape,
                                          gs, dens, ps, pitchs, win, shim, cut, dna))

    # Shimmer Textures
    for st in SHIMMER_TEXTURES:
        name, mood, desc, tags, source, gs, dens, ps, pitchs, pans, shim, cut, dna = st
        presets.append(build_shimmer_texture(name, mood, desc, tags, source, gs,
                                             dens, ps, pitchs, pans, shim, cut, dna))

    # Frozen Moments
    for fm in FROZEN_MOMENTS:
        name, mood, desc, tags, source, gs, dens, ps, pitchs, pans, freeze, cut, dna = fm
        presets.append(build_frozen_moment(name, mood, desc, tags, source, gs,
                                           dens, ps, pitchs, pans, freeze, cut, dna))

    # Scattered Glass
    for sg in SCATTERED_GLASS:
        name, mood, desc, tags, source, gs, dens, ps, pitchs, pans, frost, cut, dna = sg
        presets.append(build_scattered_glass(name, mood, desc, tags, source, gs,
                                              dens, ps, pitchs, pans, frost, cut, dna))

    # Coupling Showcases
    for cs in COUPLING_SHOWCASES:
        name, mood, desc, tags, partner, ctype, dna = cs
        presets.append(build_coupling_showcase(name, mood, desc, tags, partner, ctype, dna))

    # Deep Drift
    for dd in DEEP_DRIFT:
        name, mood, desc, tags, source, gs, dens, ps, pitchs, pans, shim, cut, dna = dd
        presets.append(build_deep_drift(name, mood, desc, tags, source, gs,
                                        dens, ps, pitchs, pans, shim, cut, dna))

    # Write all presets
    for p in presets:
        path = write_preset(p)
        count += 1
        print(f"  [{count:3d}] {p['mood']:12s} | {p['name']}")

    print(f"\nGenerated {count} new presets.")


if __name__ == "__main__":
    main()
