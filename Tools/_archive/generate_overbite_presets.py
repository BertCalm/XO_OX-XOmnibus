#!/usr/bin/env python3
"""Generate XOverbite .xometa preset files for XOlokun.

Overbite (formerly XOpossum) is a dual-oscillator subtractive synth with
character stages: Fur (pre-filter saturation), Chew (contour), Drive,
Gnash (asymmetric bite), and Trash (lo-fi destruction).

Generates 30 presets across all 6 moods:
  - Clean Bass (6): Sub-heavy, minimal character processing
  - Bite Leads (6): Aggressive, character-heavy mono leads
  - Gnash Pads (6): Warm pads with distortion character
  - Trash Textures (6): Lo-fi destruction as instrument
  - Macro Showcases (6): Designed to demo the 5 macros
"""

import json
import os
import random

random.seed(99)

PRESET_DIR = os.path.join(os.path.dirname(os.path.dirname(__file__)), "Presets", "XOlokun")


def r(lo, hi):
    return round(random.uniform(lo, hi), 3)


DEFAULTS = {
    # Oscillators
    "poss_oscAWaveform": 0, "poss_oscAShape": 0.5, "poss_oscADrift": 0.05,
    "poss_oscMix": 0.3, "poss_oscBWaveform": 0, "poss_oscBShape": 0.5,
    "poss_oscBInstability": 0.0, "poss_oscInteractMode": 0, "poss_oscInteractAmount": 0.0,
    # Sub & Weight
    "poss_subLevel": 0.3, "poss_subOctave": 0, "poss_weightShape": 0,
    "poss_weightOctave": 1, "poss_weightLevel": 0.0, "poss_weightTune": 0.0,
    # Noise
    "poss_noiseType": 0, "poss_noiseRouting": 0, "poss_noiseLevel": 0.0, "poss_noiseDecay": 0.1,
    # Filter
    "poss_filterCutoff": 2000.0, "poss_filterReso": 0.3, "poss_filterMode": 0,
    "poss_filterKeyTrack": 0.0, "poss_filterDrive": 0.0,
    # Character
    "poss_furAmount": 0.0, "poss_chewAmount": 0.0, "poss_chewFreq": 1000.0,
    "poss_chewMix": 0.5, "poss_driveAmount": 0.0, "poss_driveType": 0,
    "poss_gnashAmount": 0.0, "poss_trashMode": 0, "poss_trashAmount": 0.0,
    # Amp Envelope
    "poss_ampAttack": 0.005, "poss_ampDecay": 0.3, "poss_ampSustain": 0.8,
    "poss_ampRelease": 0.3, "poss_ampVelSens": 0.7,
    # Filter Envelope
    "poss_filterEnvAmount": 0.3, "poss_filterAttack": 0.005, "poss_filterDecay": 0.3,
    "poss_filterSustain": 0.0, "poss_filterRelease": 0.3,
    # Mod Envelope
    "poss_modEnvAmount": 0.0, "poss_modAttack": 0.01, "poss_modDecay": 0.5,
    "poss_modSustain": 0.0, "poss_modRelease": 0.5, "poss_modEnvDest": 2,
    # LFOs
    "poss_lfo1Shape": 0, "poss_lfo1Rate": 1.0, "poss_lfo1Depth": 0.0,
    "poss_lfo1Sync": 0, "poss_lfo1Retrigger": 0, "poss_lfo1Phase": 0.0,
    "poss_lfo2Shape": 0, "poss_lfo2Rate": 2.0, "poss_lfo2Depth": 0.0,
    "poss_lfo2Sync": 0, "poss_lfo2Retrigger": 0, "poss_lfo2Phase": 0.0,
    "poss_lfo3Shape": 0, "poss_lfo3Rate": 0.5, "poss_lfo3Depth": 0.0,
    "poss_lfo3Sync": 0, "poss_lfo3Retrigger": 0, "poss_lfo3Phase": 0.0,
    # Mod Matrix (8 slots)
    "poss_modSlot1Src": 0, "poss_modSlot1Dst": 0, "poss_modSlot1Amt": 0.0,
    "poss_modSlot2Src": 0, "poss_modSlot2Dst": 0, "poss_modSlot2Amt": 0.0,
    "poss_modSlot3Src": 0, "poss_modSlot3Dst": 0, "poss_modSlot3Amt": 0.0,
    "poss_modSlot4Src": 0, "poss_modSlot4Dst": 0, "poss_modSlot4Amt": 0.0,
    "poss_modSlot5Src": 0, "poss_modSlot5Dst": 0, "poss_modSlot5Amt": 0.0,
    "poss_modSlot6Src": 0, "poss_modSlot6Dst": 0, "poss_modSlot6Amt": 0.0,
    "poss_modSlot7Src": 0, "poss_modSlot7Dst": 0, "poss_modSlot7Amt": 0.0,
    "poss_modSlot8Src": 0, "poss_modSlot8Dst": 0, "poss_modSlot8Amt": 0.0,
    # Macros
    "poss_macroBelly": 0.0, "poss_macroBite": 0.0, "poss_macroScurry": 0.0,
    "poss_macroTrash": 0.0, "poss_macroPlayDead": 0.0,
    # FX
    "poss_fxMotionType": 0, "poss_fxMotionRate": 0.5, "poss_fxMotionDepth": 0.0,
    "poss_fxMotionMix": 0.0,
    "poss_fxEchoType": 0, "poss_fxEchoTime": 0.3, "poss_fxEchoFeedback": 0.3,
    "poss_fxEchoMix": 0.0, "poss_fxEchoSync": 0,
    "poss_fxSpaceType": 0, "poss_fxSpaceSize": 0.3, "poss_fxSpaceDecay": 1.5,
    "poss_fxSpaceDamping": 0.5, "poss_fxSpaceMix": 0.0,
    "poss_fxFinishGlue": 0.0, "poss_fxFinishClip": 0.0,
    "poss_fxFinishWidth": 1.0, "poss_fxFinishLowMono": 0.0,
    # Voice
    "poss_polyphony": 3, "poss_glideTime": 0.0, "poss_glideMode": 0,
    "poss_unisonVoices": 0, "poss_unisonDetune": 0.2, "poss_unisonSpread": 0.5,
    # Output
    "poss_level": 0.7, "poss_pan": 0.0,
}


def make_params(**overrides):
    p = dict(DEFAULTS)
    p.update(overrides)
    return p


def make_preset(name, mood, desc, tags, params, dna):
    return {
        "schema_version": 1,
        "name": name,
        "mood": mood,
        "engines": ["Overbite"],
        "author": "XO_OX Designs",
        "version": "1.0.0",
        "description": desc,
        "tags": tags,
        "macroLabels": ["BELLY", "BITE", "SCURRY", "TRASH"],
        "couplingIntensity": "None",
        "tempo": None,
        "dna": dna,
        "parameters": {"Overbite": params},
        "coupling": None,
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


PRESETS = [
    # ---- Clean Bass (6) ----
    ("Burrow Bass", "Foundation", "Deep sub-heavy bass. Sine OscA + sub octave. Clean and present.",
     ["bass", "sub", "clean", "deep"],
     {"poss_oscAWaveform": 0, "poss_subLevel": 0.7, "poss_subOctave": 1,
      "poss_filterCutoff": 1500.0, "poss_filterReso": 0.15, "poss_filterKeyTrack": 0.3,
      "poss_filterEnvAmount": 0.4, "poss_filterDecay": 0.2,
      "poss_ampAttack": 0.003, "poss_ampDecay": 0.2, "poss_ampSustain": 0.85, "poss_ampRelease": 0.15,
      "poss_polyphony": 0, "poss_glideTime": 0.03, "poss_glideMode": 1},
     {"brightness": 0.15, "warmth": 0.8, "movement": 0.1, "density": 0.5, "space": 0.0, "aggression": 0.05}),

    ("Saw Growl", "Foundation", "Classic saw bass with subtle fur warmth. Solid foundation.",
     ["bass", "saw", "warm", "solid"],
     {"poss_oscAWaveform": 2, "poss_subLevel": 0.4, "poss_filterCutoff": 2500.0,
      "poss_filterReso": 0.25, "poss_filterKeyTrack": 0.4,
      "poss_filterEnvAmount": 0.5, "poss_filterDecay": 0.25, "poss_furAmount": 0.15,
      "poss_ampAttack": 0.005, "poss_ampSustain": 0.8, "poss_ampRelease": 0.12,
      "poss_polyphony": 0, "poss_glideTime": 0.04, "poss_glideMode": 1},
     {"brightness": 0.3, "warmth": 0.6, "movement": 0.15, "density": 0.5, "space": 0.0, "aggression": 0.2}),

    ("Pluck Tooth", "Prism", "Short percussive bass pluck. Triangle + noise transient.",
     ["bass", "pluck", "percussive", "short"],
     {"poss_oscAWaveform": 1, "poss_noiseLevel": 0.15, "poss_noiseDecay": 0.03,
      "poss_filterCutoff": 3000.0, "poss_filterReso": 0.3,
      "poss_filterEnvAmount": 0.7, "poss_filterDecay": 0.15, "poss_filterSustain": 0.0,
      "poss_ampAttack": 0.001, "poss_ampDecay": 0.25, "poss_ampSustain": 0.3, "poss_ampRelease": 0.1},
     {"brightness": 0.4, "warmth": 0.4, "movement": 0.3, "density": 0.4, "space": 0.05, "aggression": 0.25}),

    ("Weight Drop", "Foundation", "Ultra-deep weight oscillator bass. Subsonic presence.",
     ["bass", "weight", "deep", "subsonic"],
     {"poss_oscAWaveform": 0, "poss_weightLevel": 0.6, "poss_weightOctave": 2,
      "poss_weightShape": 0, "poss_subLevel": 0.5, "poss_subOctave": 1,
      "poss_filterCutoff": 800.0, "poss_filterReso": 0.1,
      "poss_ampAttack": 0.005, "poss_ampSustain": 0.9, "poss_ampRelease": 0.2,
      "poss_polyphony": 0, "poss_fxFinishLowMono": 0.8},
     {"brightness": 0.05, "warmth": 0.9, "movement": 0.05, "density": 0.6, "space": 0.0, "aggression": 0.05}),

    ("Cushion Pulse", "Foundation", "Soft cushion pulse bass. Round, pillowy low end.",
     ["bass", "pulse", "soft", "round"],
     {"poss_oscAWaveform": 3, "poss_oscAShape": 0.6, "poss_subLevel": 0.35,
      "poss_filterCutoff": 1800.0, "poss_filterReso": 0.2,
      "poss_filterEnvAmount": 0.3, "poss_filterDecay": 0.3,
      "poss_ampAttack": 0.01, "poss_ampSustain": 0.85, "poss_ampRelease": 0.15,
      "poss_polyphony": 0, "poss_glideTime": 0.05, "poss_glideMode": 1},
     {"brightness": 0.2, "warmth": 0.75, "movement": 0.1, "density": 0.4, "space": 0.0, "aggression": 0.05}),

    ("Fang Stab", "Flux", "Short aggressive saw stab. Filter snap + gnash bite.",
     ["bass", "stab", "aggressive", "short"],
     {"poss_oscAWaveform": 2, "poss_filterCutoff": 4000.0, "poss_filterReso": 0.4,
      "poss_filterEnvAmount": 0.8, "poss_filterDecay": 0.1, "poss_filterSustain": 0.0,
      "poss_gnashAmount": 0.3, "poss_filterDrive": 0.2,
      "poss_ampAttack": 0.001, "poss_ampDecay": 0.15, "poss_ampSustain": 0.3, "poss_ampRelease": 0.08,
      "poss_ampVelSens": 0.9},
     {"brightness": 0.5, "warmth": 0.2, "movement": 0.4, "density": 0.5, "space": 0.0, "aggression": 0.7}),

    # ---- Bite Leads (6) ----
    ("Snarl Lead", "Flux", "Aggressive mono lead. Saw + hard sync, heavy gnash.",
     ["lead", "aggressive", "snarl", "mono"],
     {"poss_oscAWaveform": 2, "poss_oscBWaveform": 0, "poss_oscMix": 0.5,
      "poss_oscInteractMode": 1, "poss_oscInteractAmount": 0.6,
      "poss_filterCutoff": 5000.0, "poss_filterReso": 0.35, "poss_filterDrive": 0.3,
      "poss_gnashAmount": 0.5, "poss_furAmount": 0.3,
      "poss_filterEnvAmount": 0.5, "poss_filterDecay": 0.2,
      "poss_ampAttack": 0.003, "poss_ampSustain": 0.85, "poss_ampRelease": 0.1,
      "poss_polyphony": 0, "poss_glideTime": 0.03, "poss_glideMode": 2},
     {"brightness": 0.6, "warmth": 0.2, "movement": 0.3, "density": 0.6, "space": 0.0, "aggression": 0.75}),

    ("Wire Scream", "Flux", "High-pass filtered lead with grit multiply. Thin, cutting.",
     ["lead", "thin", "cutting", "wire"],
     {"poss_oscAWaveform": 2, "poss_oscBWaveform": 4, "poss_oscMix": 0.4,
      "poss_oscInteractMode": 4, "poss_oscInteractAmount": 0.5,
      "poss_filterMode": 2, "poss_filterCutoff": 3000.0, "poss_filterReso": 0.5,
      "poss_driveAmount": 0.4, "poss_driveType": 2,
      "poss_ampAttack": 0.002, "poss_ampSustain": 0.8, "poss_ampRelease": 0.08,
      "poss_polyphony": 0},
     {"brightness": 0.75, "warmth": 0.05, "movement": 0.3, "density": 0.5, "space": 0.0, "aggression": 0.85}),

    ("Fur Coat", "Prism", "Warm furry lead. Saw with heavy fur saturation, low filter.",
     ["lead", "warm", "furry", "saturated"],
     {"poss_oscAWaveform": 2, "poss_oscADrift": 0.1, "poss_furAmount": 0.7,
      "poss_filterCutoff": 3500.0, "poss_filterReso": 0.2, "poss_filterKeyTrack": 0.5,
      "poss_filterEnvAmount": 0.3, "poss_filterDecay": 0.4,
      "poss_ampAttack": 0.01, "poss_ampSustain": 0.85, "poss_ampRelease": 0.15,
      "poss_polyphony": 0, "poss_glideTime": 0.05, "poss_glideMode": 1},
     {"brightness": 0.35, "warmth": 0.7, "movement": 0.2, "density": 0.5, "space": 0.0, "aggression": 0.3}),

    ("Phase Bite", "Prism", "Phase push interaction creates moving harmonics.",
     ["lead", "phase", "moving", "harmonics"],
     {"poss_oscAWaveform": 2, "poss_oscBWaveform": 0, "poss_oscMix": 0.5,
      "poss_oscInteractMode": 3, "poss_oscInteractAmount": 0.4,
      "poss_filterCutoff": 6000.0, "poss_filterReso": 0.25,
      "poss_lfo1Rate": 0.3, "poss_lfo1Depth": 0.4,
      "poss_modSlot1Src": 1, "poss_modSlot1Dst": 7, "poss_modSlot1Amt": 0.5,
      "poss_ampAttack": 0.01, "poss_ampSustain": 0.8, "poss_ampRelease": 0.2,
      "poss_polyphony": 0},
     {"brightness": 0.55, "warmth": 0.4, "movement": 0.5, "density": 0.5, "space": 0.05, "aggression": 0.3}),

    ("Ring Fang", "Flux", "Ring mod OscB interaction. Metallic, alien character.",
     ["lead", "ring", "metallic", "alien"],
     {"poss_oscAWaveform": 2, "poss_oscBWaveform": 2, "poss_oscMix": 0.45,
      "poss_oscBShape": 0.3, "poss_oscBInstability": 0.15,
      "poss_filterCutoff": 8000.0, "poss_filterReso": 0.3,
      "poss_gnashAmount": 0.35, "poss_furAmount": 0.2,
      "poss_ampAttack": 0.005, "poss_ampSustain": 0.75, "poss_ampRelease": 0.12,
      "poss_polyphony": 0},
     {"brightness": 0.65, "warmth": 0.15, "movement": 0.35, "density": 0.5, "space": 0.0, "aggression": 0.6}),

    ("Chew Lead", "Prism", "Lead with heavy chew contour. Compressed, present.",
     ["lead", "chew", "compressed", "present"],
     {"poss_oscAWaveform": 2, "poss_chewAmount": 0.6, "poss_chewFreq": 2000.0,
      "poss_chewMix": 0.7, "poss_furAmount": 0.2,
      "poss_filterCutoff": 4500.0, "poss_filterReso": 0.2, "poss_filterKeyTrack": 0.4,
      "poss_filterEnvAmount": 0.4, "poss_filterDecay": 0.25,
      "poss_ampAttack": 0.005, "poss_ampSustain": 0.85, "poss_ampRelease": 0.1,
      "poss_polyphony": 0, "poss_glideTime": 0.03, "poss_glideMode": 1},
     {"brightness": 0.45, "warmth": 0.45, "movement": 0.2, "density": 0.55, "space": 0.0, "aggression": 0.4}),

    # ---- Gnash Pads (6) ----
    ("Warm Gnash", "Atmosphere", "Warm pad with gentle gnash character. Cozy distortion.",
     ["pad", "warm", "gnash", "cozy"],
     {"poss_oscAWaveform": 2, "poss_oscADrift": 0.08, "poss_oscMix": 0.3,
      "poss_oscBWaveform": 1, "poss_gnashAmount": 0.25, "poss_furAmount": 0.15,
      "poss_filterCutoff": 3000.0, "poss_filterReso": 0.15,
      "poss_ampAttack": 0.3, "poss_ampSustain": 0.85, "poss_ampRelease": 1.0,
      "poss_lfo1Rate": 0.2, "poss_lfo1Depth": 0.15,
      "poss_modSlot1Src": 1, "poss_modSlot1Dst": 10, "poss_modSlot1Amt": 0.2,
      "poss_unisonVoices": 1, "poss_unisonDetune": 0.15},
     {"brightness": 0.3, "warmth": 0.7, "movement": 0.3, "density": 0.5, "space": 0.2, "aggression": 0.2}),

    ("Hollow Pad", "Atmosphere", "Notch-filtered pad. Hollow, haunting character.",
     ["pad", "hollow", "haunting", "notch"],
     {"poss_oscAWaveform": 2, "poss_filterMode": 3, "poss_filterCutoff": 2500.0,
      "poss_filterReso": 0.4, "poss_oscADrift": 0.06,
      "poss_ampAttack": 0.5, "poss_ampSustain": 0.8, "poss_ampRelease": 1.5,
      "poss_lfo1Rate": 0.15, "poss_lfo1Depth": 0.2,
      "poss_modSlot1Src": 1, "poss_modSlot1Dst": 10, "poss_modSlot1Amt": 0.3},
     {"brightness": 0.35, "warmth": 0.5, "movement": 0.35, "density": 0.4, "space": 0.3, "aggression": 0.1}),

    ("Crushed Velvet", "Atmosphere", "Soft pad through tube drive. Warm compression.",
     ["pad", "velvet", "tube", "warm"],
     {"poss_oscAWaveform": 1, "poss_oscADrift": 0.1, "poss_subLevel": 0.2,
      "poss_driveAmount": 0.35, "poss_driveType": 3, "poss_furAmount": 0.1,
      "poss_filterCutoff": 3500.0, "poss_filterReso": 0.1,
      "poss_ampAttack": 0.4, "poss_ampSustain": 0.85, "poss_ampRelease": 1.2,
      "poss_unisonVoices": 2, "poss_unisonDetune": 0.12},
     {"brightness": 0.25, "warmth": 0.75, "movement": 0.2, "density": 0.5, "space": 0.15, "aggression": 0.15}),

    ("Toxic Cloud", "Flux", "Aggressive pad. Heavy gnash + grit, bandpass focused.",
     ["pad", "aggressive", "toxic", "bandpass"],
     {"poss_oscAWaveform": 2, "poss_oscBWaveform": 4, "poss_oscMix": 0.4,
      "poss_oscInteractMode": 4, "poss_oscInteractAmount": 0.3,
      "poss_filterMode": 1, "poss_filterCutoff": 2000.0, "poss_filterReso": 0.45,
      "poss_gnashAmount": 0.5, "poss_driveAmount": 0.3, "poss_driveType": 1,
      "poss_ampAttack": 0.2, "poss_ampSustain": 0.8, "poss_ampRelease": 0.8},
     {"brightness": 0.4, "warmth": 0.3, "movement": 0.4, "density": 0.6, "space": 0.1, "aggression": 0.6}),

    ("Noise Bed", "Aether", "Sustained noise pad with crackle character. Atmospheric.",
     ["pad", "noise", "crackle", "atmospheric"],
     {"poss_noiseType": 3, "poss_noiseLevel": 0.4, "poss_noiseRouting": 2,
      "poss_oscAWaveform": 0, "poss_filterCutoff": 2500.0, "poss_filterReso": 0.2,
      "poss_ampAttack": 1.0, "poss_ampSustain": 0.7, "poss_ampRelease": 2.0,
      "poss_fxSpaceSize": 0.5, "poss_fxSpaceDecay": 3.0, "poss_fxSpaceMix": 0.0},
     {"brightness": 0.3, "warmth": 0.4, "movement": 0.3, "density": 0.4, "space": 0.3, "aggression": 0.15}),

    ("FM Pad", "Prism", "Low FM interaction creates complex harmonics. Rich, evolving.",
     ["pad", "fm", "complex", "evolving"],
     {"poss_oscAWaveform": 0, "poss_oscBWaveform": 1, "poss_oscMix": 0.5,
      "poss_oscInteractMode": 2, "poss_oscInteractAmount": 0.35,
      "poss_filterCutoff": 5000.0, "poss_filterReso": 0.15,
      "poss_lfo1Rate": 0.3, "poss_lfo1Depth": 0.3,
      "poss_modSlot1Src": 1, "poss_modSlot1Dst": 7, "poss_modSlot1Amt": 0.3,
      "poss_ampAttack": 0.3, "poss_ampSustain": 0.85, "poss_ampRelease": 1.0,
      "poss_unisonVoices": 1, "poss_unisonDetune": 0.1},
     {"brightness": 0.5, "warmth": 0.5, "movement": 0.45, "density": 0.5, "space": 0.1, "aggression": 0.15}),

    # ---- Trash Textures (6) ----
    ("Rust Bucket", "Flux", "Heavy rust trash mode. Degraded, corroded texture.",
     ["trash", "rust", "degraded", "texture"],
     {"poss_oscAWaveform": 2, "poss_trashMode": 1, "poss_trashAmount": 0.7,
      "poss_gnashAmount": 0.3, "poss_filterCutoff": 4000.0, "poss_filterReso": 0.3,
      "poss_ampAttack": 0.01, "poss_ampSustain": 0.7, "poss_ampRelease": 0.3},
     {"brightness": 0.4, "warmth": 0.25, "movement": 0.4, "density": 0.6, "space": 0.05, "aggression": 0.7}),

    ("Splatter Pad", "Flux", "Splatter trash mode on pad. Chaotic, unpredictable.",
     ["trash", "splatter", "chaotic", "pad"],
     {"poss_oscAWaveform": 2, "poss_oscADrift": 0.15, "poss_trashMode": 2,
      "poss_trashAmount": 0.6, "poss_furAmount": 0.3,
      "poss_filterCutoff": 3000.0, "poss_filterReso": 0.25,
      "poss_ampAttack": 0.2, "poss_ampSustain": 0.75, "poss_ampRelease": 0.8},
     {"brightness": 0.45, "warmth": 0.3, "movement": 0.55, "density": 0.55, "space": 0.1, "aggression": 0.6}),

    ("Crushed Signal", "Flux", "Maximum crushed trash. Digital destruction.",
     ["trash", "crushed", "digital", "destruction"],
     {"poss_oscAWaveform": 2, "poss_trashMode": 3, "poss_trashAmount": 0.9,
      "poss_driveAmount": 0.5, "poss_driveType": 2, "poss_gnashAmount": 0.4,
      "poss_filterCutoff": 6000.0, "poss_filterReso": 0.4,
      "poss_ampAttack": 0.003, "poss_ampSustain": 0.8, "poss_ampRelease": 0.15},
     {"brightness": 0.55, "warmth": 0.1, "movement": 0.5, "density": 0.7, "space": 0.0, "aggression": 0.9}),

    ("Gentle Decay", "Aether", "Subtle trash + noise. Aging signal, decaying gracefully.",
     ["trash", "subtle", "aging", "decay"],
     {"poss_oscAWaveform": 1, "poss_trashMode": 1, "poss_trashAmount": 0.2,
      "poss_noiseType": 2, "poss_noiseLevel": 0.1, "poss_noiseRouting": 2,
      "poss_filterCutoff": 2000.0, "poss_furAmount": 0.1,
      "poss_ampAttack": 0.8, "poss_ampSustain": 0.7, "poss_ampRelease": 2.0},
     {"brightness": 0.2, "warmth": 0.5, "movement": 0.25, "density": 0.3, "space": 0.3, "aggression": 0.15}),

    ("Hiss Drone", "Aether", "Sustained hiss noise through trash. Atmospheric texture.",
     ["trash", "hiss", "drone", "atmospheric"],
     {"poss_noiseType": 4, "poss_noiseLevel": 0.5, "poss_noiseRouting": 0,
      "poss_trashMode": 1, "poss_trashAmount": 0.3, "poss_filterCutoff": 3500.0,
      "poss_ampAttack": 1.5, "poss_ampSustain": 0.6, "poss_ampRelease": 3.0},
     {"brightness": 0.35, "warmth": 0.35, "movement": 0.2, "density": 0.3, "space": 0.4, "aggression": 0.2}),

    ("Full Chain", "Flux", "Every character stage engaged. Maximum processing.",
     ["trash", "chain", "maximum", "all-stages"],
     {"poss_oscAWaveform": 2, "poss_furAmount": 0.4, "poss_chewAmount": 0.5,
      "poss_chewFreq": 1500.0, "poss_driveAmount": 0.4, "poss_driveType": 1,
      "poss_gnashAmount": 0.4, "poss_trashMode": 2, "poss_trashAmount": 0.5,
      "poss_filterCutoff": 5000.0, "poss_filterReso": 0.3, "poss_filterDrive": 0.3,
      "poss_ampAttack": 0.005, "poss_ampSustain": 0.8, "poss_ampRelease": 0.2,
      "poss_fxFinishClip": 0.4},
     {"brightness": 0.5, "warmth": 0.2, "movement": 0.4, "density": 0.7, "space": 0.0, "aggression": 0.8}),

    # ---- Macro Showcases (6) ----
    ("Belly Rub", "Foundation", "Sweep BELLY macro from lean to full. Sub rises, filter drops.",
     ["macro", "belly", "sub", "sweep"],
     {"poss_oscAWaveform": 2, "poss_oscMix": 0.4, "poss_subLevel": 0.2,
      "poss_filterCutoff": 3000.0, "poss_filterReso": 0.2, "poss_furAmount": 0.1,
      "poss_ampAttack": 0.01, "poss_ampSustain": 0.85, "poss_ampRelease": 0.2},
     {"brightness": 0.35, "warmth": 0.6, "movement": 0.3, "density": 0.5, "space": 0.0, "aggression": 0.15}),

    ("Bite Force", "Flux", "Sweep BITE macro for increasing aggression. OscB + gnash rise.",
     ["macro", "bite", "aggressive", "sweep"],
     {"poss_oscAWaveform": 2, "poss_oscBWaveform": 0, "poss_oscMix": 0.3,
      "poss_filterCutoff": 4000.0, "poss_filterReso": 0.3,
      "poss_ampAttack": 0.005, "poss_ampSustain": 0.8, "poss_ampRelease": 0.15,
      "poss_polyphony": 0},
     {"brightness": 0.5, "warmth": 0.3, "movement": 0.4, "density": 0.5, "space": 0.0, "aggression": 0.5}),

    ("Scurry Mode", "Entangled", "Sweep SCURRY for LFO rate multiply + envelope compression.",
     ["macro", "scurry", "fast", "animated"],
     {"poss_oscAWaveform": 2, "poss_lfo1Rate": 0.5, "poss_lfo1Depth": 0.3,
      "poss_modSlot1Src": 1, "poss_modSlot1Dst": 10, "poss_modSlot1Amt": 0.4,
      "poss_filterCutoff": 3500.0, "poss_filterReso": 0.25,
      "poss_filterEnvAmount": 0.4, "poss_filterDecay": 0.3,
      "poss_ampAttack": 0.005, "poss_ampSustain": 0.7, "poss_ampRelease": 0.2},
     {"brightness": 0.4, "warmth": 0.4, "movement": 0.6, "density": 0.4, "space": 0.05, "aggression": 0.3}),

    ("Trash Sweep", "Flux", "Sweep TRASH macro. Clean to destroyed.",
     ["macro", "trash", "sweep", "destruction"],
     {"poss_oscAWaveform": 2, "poss_filterCutoff": 4000.0, "poss_filterReso": 0.3,
      "poss_ampAttack": 0.005, "poss_ampSustain": 0.8, "poss_ampRelease": 0.15,
      "poss_polyphony": 0},
     {"brightness": 0.45, "warmth": 0.3, "movement": 0.4, "density": 0.5, "space": 0.0, "aggression": 0.5}),

    ("Play Dead", "Aether", "Sweep PLAYDEAD. Volume ducks, release extends, filter closes.",
     ["macro", "playdead", "fade", "release"],
     {"poss_oscAWaveform": 1, "poss_oscADrift": 0.08, "poss_subLevel": 0.2,
      "poss_filterCutoff": 3000.0, "poss_filterReso": 0.15,
      "poss_ampAttack": 0.3, "poss_ampSustain": 0.8, "poss_ampRelease": 1.0},
     {"brightness": 0.3, "warmth": 0.6, "movement": 0.2, "density": 0.3, "space": 0.3, "aggression": 0.05}),

    ("All Macros", "Entangled", "All 5 macros active. Maximum character transformation.",
     ["macro", "all", "transformation", "showcase"],
     {"poss_oscAWaveform": 2, "poss_oscBWaveform": 0, "poss_oscMix": 0.4,
      "poss_subLevel": 0.3, "poss_filterCutoff": 3500.0, "poss_filterReso": 0.25,
      "poss_lfo1Rate": 0.8, "poss_lfo1Depth": 0.2,
      "poss_modSlot1Src": 1, "poss_modSlot1Dst": 10, "poss_modSlot1Amt": 0.3,
      "poss_ampAttack": 0.01, "poss_ampSustain": 0.8, "poss_ampRelease": 0.3},
     {"brightness": 0.4, "warmth": 0.4, "movement": 0.5, "density": 0.5, "space": 0.1, "aggression": 0.35}),
]


def main():
    count = 0
    for name, mood, desc, tags, overrides, dna in PRESETS:
        params = make_params(**overrides)
        preset = make_preset(name, mood, desc, tags, params, dna)
        path = write_preset(preset)
        count += 1
        print(f"  [{count:3d}] {mood:12s} | {name}")

    print(f"\nGenerated {count} Overbite presets.")


if __name__ == "__main__":
    main()
