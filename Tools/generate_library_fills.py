#!/usr/bin/env python3
"""
XOmnibus Library Gap Fill Generator
Targets: Dub/Prism, Drift missing moods, Morph missing moods,
         ONSET coupling presets, Snap thin moods.
"""

import json
import os
from pathlib import Path

REPO_ROOT = Path(__file__).parent.parent
PRESETS_DIR = REPO_ROOT / "Presets" / "XOmnibus"

SCHEMA = {
    "schema_version": 1,
    "author": "XO_OX Designs",
    "version": "1.0.0",
    "couplingIntensity": "None",
    "tempo": None,
    "macroLabels": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "coupling": {"pairs": []},
    "sequencer": None,
}


def make_preset(name, mood, engines, description, tags, dna, parameters,
                coupling_intensity="None", coupling_pairs=None, macro_labels=None):
    preset = dict(SCHEMA)
    preset.update({
        "name": name,
        "mood": mood,
        "engines": engines if isinstance(engines, list) else [engines],
        "description": description,
        "tags": tags,
        "dna": dna,
        "parameters": parameters,
        "couplingIntensity": coupling_intensity,
        "coupling": {"pairs": coupling_pairs or []},
        "macroLabels": macro_labels or ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    })
    return preset


def write_preset(preset, subdir=None):
    mood = preset["mood"]
    name_slug = preset["name"].replace(" ", "_").replace("/", "-")
    if subdir:
        out_dir = PRESETS_DIR / mood / subdir
    else:
        out_dir = PRESETS_DIR / mood
    out_dir.mkdir(parents=True, exist_ok=True)
    path = out_dir / f"{name_slug}.xometa"
    with open(path, "w") as f:
        json.dump(preset, f, indent=2)
    rel = path.relative_to(PRESETS_DIR)
    print(f"  Written: {rel}")
    return path


# =============================================================================
# DUB / PRISM — 18 presets (tonal/harmonic dub)
# =============================================================================

DUB_PRISM = [
    ("Chromatic Dub", "Prism", "Dub",
     "Chromatic sub-bass walk in a filtered dub space — each note a prismatic shift.",
     ["dub", "chromatic", "prism", "sub", "tonal"],
     {"brightness": 0.35, "warmth": 0.7, "movement": 0.4, "density": 0.5, "space": 0.65, "aggression": 0.2},
     {"Dub": {"dub_oscWave": 0, "dub_oscOctave": -1, "dub_oscTune": 0.0, "dub_oscPwm": 0.5,
              "dub_subLevel": 0.65, "dub_noiseLevel": 0.0, "dub_drift": 0.03,
              "dub_filterCutoff": 900.0, "dub_filterReso": 0.45, "dub_filterMode": 0,
              "dub_filterEnvAmt": 0.4, "dub_attack": 0.01, "dub_decay": 0.5,
              "dub_sustain": 0.6, "dub_release": 0.4,
              "dub_pitchEnvDepth": 0.12, "dub_pitchEnvDecay": 0.04,
              "dub_lfoRate": 0.5, "dub_lfoDepth": 0.04, "dub_lfoDest": 1,
              "dub_driveAmount": 1.1, "dub_dryLevel": 0.85,
              "dub_delayTime": 0.375, "dub_delayFeedback": 0.35, "dub_delayMix": 0.2,
              "dub_reverbSize": 0.45, "dub_reverbDamp": 0.5, "dub_reverbMix": 0.2,
              "dub_level": 0.75, "dub_voiceMode": 0, "dub_polyphony": 4}}),

    ("Modal Pressure", "Prism", "Dub",
     "Modal bass drone — one note, many harmonic overtones, deep dub pressure.",
     ["dub", "modal", "drone", "harmonic", "pressure"],
     {"brightness": 0.3, "warmth": 0.8, "movement": 0.25, "density": 0.7, "space": 0.55, "aggression": 0.3},
     {"Dub": {"dub_oscWave": 2, "dub_oscOctave": -1, "dub_oscTune": 0.0, "dub_oscPwm": 0.3,
              "dub_subLevel": 0.7, "dub_noiseLevel": 0.0, "dub_drift": 0.05,
              "dub_filterCutoff": 600.0, "dub_filterReso": 0.6, "dub_filterMode": 0,
              "dub_filterEnvAmt": 0.2, "dub_attack": 0.02, "dub_decay": 1.2,
              "dub_sustain": 0.75, "dub_release": 0.8,
              "dub_pitchEnvDepth": 0.06, "dub_pitchEnvDecay": 0.02,
              "dub_lfoRate": 0.12, "dub_lfoDepth": 0.025, "dub_lfoDest": 0,
              "dub_driveAmount": 1.3, "dub_dryLevel": 0.8,
              "dub_delayTime": 0.5, "dub_delayFeedback": 0.55, "dub_delayMix": 0.18,
              "dub_reverbSize": 0.6, "dub_reverbDamp": 0.4, "dub_reverbMix": 0.25,
              "dub_level": 0.72, "dub_voiceMode": 1, "dub_polyphony": 0}}),

    ("Filtered Chord", "Prism", "Dub",
     "Dub chord stab through a resonant filter — the harmonic dub tradition.",
     ["dub", "chord", "filter", "stab", "prism"],
     {"brightness": 0.5, "warmth": 0.6, "movement": 0.5, "density": 0.6, "space": 0.6, "aggression": 0.25},
     {"Dub": {"dub_oscWave": 1, "dub_oscOctave": 0, "dub_oscTune": 0.0, "dub_oscPwm": 0.6,
              "dub_subLevel": 0.3, "dub_noiseLevel": 0.0, "dub_drift": 0.06,
              "dub_filterCutoff": 2000.0, "dub_filterReso": 0.55, "dub_filterMode": 0,
              "dub_filterEnvAmt": 0.65, "dub_attack": 0.005, "dub_decay": 0.3,
              "dub_sustain": 0.2, "dub_release": 0.3,
              "dub_pitchEnvDepth": 0.0, "dub_pitchEnvDecay": 0.01,
              "dub_lfoRate": 0.8, "dub_lfoDepth": 0.0, "dub_lfoDest": 0,
              "dub_driveAmount": 1.15, "dub_dryLevel": 0.88,
              "dub_delayTime": 0.375, "dub_delayFeedback": 0.4, "dub_delayMix": 0.25,
              "dub_reverbSize": 0.5, "dub_reverbDamp": 0.45, "dub_reverbMix": 0.22,
              "dub_level": 0.7, "dub_voiceMode": 0, "dub_polyphony": 4}}),

    ("Tape Harmonics", "Prism", "Dub",
     "Warm tape saturation breathing life into upper harmonic content.",
     ["dub", "tape", "harmonics", "warm", "prism"],
     {"brightness": 0.45, "warmth": 0.85, "movement": 0.35, "density": 0.55, "space": 0.6, "aggression": 0.15},
     {"Dub": {"dub_oscWave": 0, "dub_oscOctave": 0, "dub_oscTune": 0.0, "dub_oscPwm": 0.5,
              "dub_subLevel": 0.45, "dub_noiseLevel": 0.0, "dub_drift": 0.08,
              "dub_filterCutoff": 3500.0, "dub_filterReso": 0.25, "dub_filterMode": 0,
              "dub_filterEnvAmt": 0.15, "dub_attack": 0.05, "dub_decay": 0.8,
              "dub_sustain": 0.65, "dub_release": 0.6,
              "dub_pitchEnvDepth": 0.04, "dub_pitchEnvDecay": 0.03,
              "dub_lfoRate": 0.3, "dub_lfoDepth": 0.03, "dub_lfoDest": 0,
              "dub_driveAmount": 1.5, "dub_dryLevel": 0.9,
              "dub_delayTime": 0.5, "dub_delayFeedback": 0.4, "dub_delayMix": 0.15,
              "dub_reverbSize": 0.55, "dub_reverbDamp": 0.6, "dub_reverbMix": 0.2,
              "dub_level": 0.72, "dub_voiceMode": 0, "dub_polyphony": 4}}),

    ("Resonant Column", "Prism", "Dub",
     "High-resonance self-oscillating filter — a column of pure frequency standing in space.",
     ["dub", "resonance", "self-osc", "filter", "tonal"],
     {"brightness": 0.6, "warmth": 0.5, "movement": 0.3, "density": 0.45, "space": 0.7, "aggression": 0.4},
     {"Dub": {"dub_oscWave": 0, "dub_oscOctave": 0, "dub_oscTune": 0.0, "dub_oscPwm": 0.5,
              "dub_subLevel": 0.2, "dub_noiseLevel": 0.05, "dub_drift": 0.04,
              "dub_filterCutoff": 1200.0, "dub_filterReso": 0.85, "dub_filterMode": 0,
              "dub_filterEnvAmt": 0.3, "dub_attack": 0.005, "dub_decay": 0.6,
              "dub_sustain": 0.4, "dub_release": 0.5,
              "dub_pitchEnvDepth": 0.15, "dub_pitchEnvDecay": 0.06,
              "dub_lfoRate": 0.4, "dub_lfoDepth": 0.06, "dub_lfoDest": 1,
              "dub_driveAmount": 1.2, "dub_dryLevel": 0.85,
              "dub_delayTime": 0.375, "dub_delayFeedback": 0.5, "dub_delayMix": 0.2,
              "dub_reverbSize": 0.65, "dub_reverbDamp": 0.3, "dub_reverbMix": 0.28,
              "dub_level": 0.68, "dub_voiceMode": 0, "dub_polyphony": 4}}),

    ("Dub Piano", "Prism", "Dub",
     "Plucked decay with tape echo — a roots dub piano tone.",
     ["dub", "piano", "pluck", "roots", "tonal"],
     {"brightness": 0.55, "warmth": 0.65, "movement": 0.45, "density": 0.5, "space": 0.65, "aggression": 0.1},
     {"Dub": {"dub_oscWave": 3, "dub_oscOctave": 1, "dub_oscTune": 0.0, "dub_oscPwm": 0.4,
              "dub_subLevel": 0.1, "dub_noiseLevel": 0.0, "dub_drift": 0.02,
              "dub_filterCutoff": 5000.0, "dub_filterReso": 0.15, "dub_filterMode": 0,
              "dub_filterEnvAmt": 0.55, "dub_attack": 0.003, "dub_decay": 0.45,
              "dub_sustain": 0.0, "dub_release": 0.2,
              "dub_pitchEnvDepth": 0.08, "dub_pitchEnvDecay": 0.025,
              "dub_lfoRate": 0.0, "dub_lfoDepth": 0.0, "dub_lfoDest": 0,
              "dub_driveAmount": 1.05, "dub_dryLevel": 0.92,
              "dub_delayTime": 0.375, "dub_delayFeedback": 0.45, "dub_delayMix": 0.3,
              "dub_reverbSize": 0.4, "dub_reverbDamp": 0.55, "dub_reverbMix": 0.18,
              "dub_level": 0.73, "dub_voiceMode": 0, "dub_polyphony": 4}}),

    ("Spring Melody", "Prism", "Dub",
     "Melodic line through a spring reverb — notes bloom into tonal clouds.",
     ["dub", "spring", "melody", "reverb", "prism"],
     {"brightness": 0.5, "warmth": 0.6, "movement": 0.55, "density": 0.45, "space": 0.8, "aggression": 0.1},
     {"Dub": {"dub_oscWave": 2, "dub_oscOctave": 0, "dub_oscTune": 0.0, "dub_oscPwm": 0.55,
              "dub_subLevel": 0.25, "dub_noiseLevel": 0.0, "dub_drift": 0.04,
              "dub_filterCutoff": 4000.0, "dub_filterReso": 0.3, "dub_filterMode": 0,
              "dub_filterEnvAmt": 0.35, "dub_attack": 0.01, "dub_decay": 0.5,
              "dub_sustain": 0.3, "dub_release": 0.6,
              "dub_pitchEnvDepth": 0.05, "dub_pitchEnvDecay": 0.02,
              "dub_lfoRate": 0.25, "dub_lfoDepth": 0.02, "dub_lfoDest": 0,
              "dub_driveAmount": 1.08, "dub_dryLevel": 0.78,
              "dub_delayTime": 0.5, "dub_delayFeedback": 0.5, "dub_delayMix": 0.28,
              "dub_reverbSize": 0.75, "dub_reverbDamp": 0.35, "dub_reverbMix": 0.38,
              "dub_level": 0.7, "dub_voiceMode": 0, "dub_polyphony": 4}}),

    ("Overtone Riddim", "Prism", "Dub",
     "Riddim bassline with rich overtone content — roots rock meet synthesis.",
     ["dub", "riddim", "overtone", "bassline", "roots"],
     {"brightness": 0.4, "warmth": 0.75, "movement": 0.6, "density": 0.65, "space": 0.5, "aggression": 0.3},
     {"Dub": {"dub_oscWave": 2, "dub_oscOctave": -1, "dub_oscTune": 0.0, "dub_oscPwm": 0.35,
              "dub_subLevel": 0.55, "dub_noiseLevel": 0.0, "dub_drift": 0.07,
              "dub_filterCutoff": 1500.0, "dub_filterReso": 0.5, "dub_filterMode": 0,
              "dub_filterEnvAmt": 0.45, "dub_attack": 0.005, "dub_decay": 0.35,
              "dub_sustain": 0.5, "dub_release": 0.3,
              "dub_pitchEnvDepth": 0.1, "dub_pitchEnvDecay": 0.03,
              "dub_lfoRate": 1.0, "dub_lfoDepth": 0.05, "dub_lfoDest": 1,
              "dub_driveAmount": 1.25, "dub_dryLevel": 0.87,
              "dub_delayTime": 0.375, "dub_delayFeedback": 0.4, "dub_delayMix": 0.22,
              "dub_reverbSize": 0.45, "dub_reverbDamp": 0.5, "dub_reverbMix": 0.18,
              "dub_level": 0.75, "dub_voiceMode": 1, "dub_polyphony": 0}}),

    ("Crystal Frequency", "Prism", "Dub",
     "Pure sine-like tone through massive reverb — a frequency becomes a place.",
     ["dub", "crystal", "sine", "pure", "tonal"],
     {"brightness": 0.7, "warmth": 0.4, "movement": 0.2, "density": 0.35, "space": 0.9, "aggression": 0.05},
     {"Dub": {"dub_oscWave": 0, "dub_oscOctave": 0, "dub_oscTune": 0.0, "dub_oscPwm": 0.5,
              "dub_subLevel": 0.05, "dub_noiseLevel": 0.0, "dub_drift": 0.01,
              "dub_filterCutoff": 8000.0, "dub_filterReso": 0.1, "dub_filterMode": 0,
              "dub_filterEnvAmt": 0.05, "dub_attack": 0.1, "dub_decay": 1.5,
              "dub_sustain": 0.5, "dub_release": 2.0,
              "dub_pitchEnvDepth": 0.02, "dub_pitchEnvDecay": 0.05,
              "dub_lfoRate": 0.08, "dub_lfoDepth": 0.015, "dub_lfoDest": 0,
              "dub_driveAmount": 1.0, "dub_dryLevel": 0.7,
              "dub_delayTime": 0.5, "dub_delayFeedback": 0.55, "dub_delayMix": 0.25,
              "dub_reverbSize": 0.92, "dub_reverbDamp": 0.2, "dub_reverbMix": 0.5,
              "dub_level": 0.65, "dub_voiceMode": 0, "dub_polyphony": 4}}),

    ("Minor Third Bass", "Prism", "Dub",
     "Interval-aware dub bass — every note implies its minor third partner.",
     ["dub", "interval", "minor", "bass", "harmonic"],
     {"brightness": 0.35, "warmth": 0.72, "movement": 0.38, "density": 0.6, "space": 0.55, "aggression": 0.22},
     {"Dub": {"dub_oscWave": 1, "dub_oscOctave": -1, "dub_oscTune": 0.0, "dub_oscPwm": 0.45,
              "dub_subLevel": 0.6, "dub_noiseLevel": 0.0, "dub_drift": 0.04,
              "dub_filterCutoff": 1100.0, "dub_filterReso": 0.4, "dub_filterMode": 0,
              "dub_filterEnvAmt": 0.35, "dub_attack": 0.008, "dub_decay": 0.55,
              "dub_sustain": 0.55, "dub_release": 0.35,
              "dub_pitchEnvDepth": 0.09, "dub_pitchEnvDecay": 0.035,
              "dub_lfoRate": 0.4, "dub_lfoDepth": 0.03, "dub_lfoDest": 0,
              "dub_driveAmount": 1.18, "dub_dryLevel": 0.88,
              "dub_delayTime": 0.375, "dub_delayFeedback": 0.38, "dub_delayMix": 0.2,
              "dub_reverbSize": 0.5, "dub_reverbDamp": 0.45, "dub_reverbMix": 0.22,
              "dub_level": 0.74, "dub_voiceMode": 1, "dub_polyphony": 0}}),

    ("High Frequency Dub", "Prism", "Dub",
     "Bright open-filter dub tone — the upper register explored.",
     ["dub", "bright", "high", "open", "prism"],
     {"brightness": 0.78, "warmth": 0.45, "movement": 0.42, "density": 0.45, "space": 0.6, "aggression": 0.28},
     {"Dub": {"dub_oscWave": 2, "dub_oscOctave": 1, "dub_oscTune": 0.0, "dub_oscPwm": 0.5,
              "dub_subLevel": 0.15, "dub_noiseLevel": 0.0, "dub_drift": 0.03,
              "dub_filterCutoff": 6000.0, "dub_filterReso": 0.35, "dub_filterMode": 0,
              "dub_filterEnvAmt": 0.4, "dub_attack": 0.003, "dub_decay": 0.3,
              "dub_sustain": 0.35, "dub_release": 0.25,
              "dub_pitchEnvDepth": 0.06, "dub_pitchEnvDecay": 0.02,
              "dub_lfoRate": 0.6, "dub_lfoDepth": 0.04, "dub_lfoDest": 1,
              "dub_driveAmount": 1.1, "dub_dryLevel": 0.9,
              "dub_delayTime": 0.375, "dub_delayFeedback": 0.45, "dub_delayMix": 0.24,
              "dub_reverbSize": 0.55, "dub_reverbDamp": 0.4, "dub_reverbMix": 0.22,
              "dub_level": 0.7, "dub_voiceMode": 0, "dub_polyphony": 4}}),

    ("Dub Organ", "Prism", "Dub",
     "Hammond-flavored dub organ — rotary cabinet through tape delay.",
     ["dub", "organ", "hammond", "rotary", "tonal"],
     {"brightness": 0.6, "warmth": 0.7, "movement": 0.55, "density": 0.65, "space": 0.6, "aggression": 0.2},
     {"Dub": {"dub_oscWave": 3, "dub_oscOctave": 0, "dub_oscTune": 0.0, "dub_oscPwm": 0.5,
              "dub_subLevel": 0.4, "dub_noiseLevel": 0.0, "dub_drift": 0.06,
              "dub_filterCutoff": 4500.0, "dub_filterReso": 0.2, "dub_filterMode": 0,
              "dub_filterEnvAmt": 0.1, "dub_attack": 0.02, "dub_decay": 0.3,
              "dub_sustain": 0.8, "dub_release": 0.2,
              "dub_pitchEnvDepth": 0.0, "dub_pitchEnvDecay": 0.01,
              "dub_lfoRate": 6.0, "dub_lfoDepth": 0.06, "dub_lfoDest": 0,
              "dub_driveAmount": 1.2, "dub_dryLevel": 0.85,
              "dub_delayTime": 0.375, "dub_delayFeedback": 0.5, "dub_delayMix": 0.28,
              "dub_reverbSize": 0.45, "dub_reverbDamp": 0.5, "dub_reverbMix": 0.18,
              "dub_level": 0.72, "dub_voiceMode": 0, "dub_polyphony": 4}}),

    ("Spectral Bass", "Prism", "Dub",
     "Extended range bass using overtone shaping — from sub to presence.",
     ["dub", "spectral", "bass", "extended", "harmonic"],
     {"brightness": 0.5, "warmth": 0.65, "movement": 0.3, "density": 0.7, "space": 0.55, "aggression": 0.35},
     {"Dub": {"dub_oscWave": 1, "dub_oscOctave": -1, "dub_oscTune": 0.0, "dub_oscPwm": 0.7,
              "dub_subLevel": 0.7, "dub_noiseLevel": 0.0, "dub_drift": 0.05,
              "dub_filterCutoff": 2500.0, "dub_filterReso": 0.45, "dub_filterMode": 0,
              "dub_filterEnvAmt": 0.3, "dub_attack": 0.01, "dub_decay": 0.7,
              "dub_sustain": 0.6, "dub_release": 0.4,
              "dub_pitchEnvDepth": 0.07, "dub_pitchEnvDecay": 0.03,
              "dub_lfoRate": 0.2, "dub_lfoDepth": 0.02, "dub_lfoDest": 0,
              "dub_driveAmount": 1.35, "dub_dryLevel": 0.88,
              "dub_delayTime": 0.5, "dub_delayFeedback": 0.35, "dub_delayMix": 0.18,
              "dub_reverbSize": 0.5, "dub_reverbDamp": 0.5, "dub_reverbMix": 0.2,
              "dub_level": 0.74, "dub_voiceMode": 1, "dub_polyphony": 0}}),

    ("Tuned Percussion Dub", "Prism", "Dub",
     "Short tuned hits with enormous reverb tails — marimba meets infinite dub.",
     ["dub", "tuned", "percussion", "marimba", "reverb"],
     {"brightness": 0.6, "warmth": 0.55, "movement": 0.5, "density": 0.4, "space": 0.85, "aggression": 0.12},
     {"Dub": {"dub_oscWave": 0, "dub_oscOctave": 1, "dub_oscTune": 0.0, "dub_oscPwm": 0.5,
              "dub_subLevel": 0.0, "dub_noiseLevel": 0.05, "dub_drift": 0.02,
              "dub_filterCutoff": 6500.0, "dub_filterReso": 0.2, "dub_filterMode": 0,
              "dub_filterEnvAmt": 0.6, "dub_attack": 0.002, "dub_decay": 0.25,
              "dub_sustain": 0.0, "dub_release": 0.15,
              "dub_pitchEnvDepth": 0.18, "dub_pitchEnvDecay": 0.04,
              "dub_lfoRate": 0.0, "dub_lfoDepth": 0.0, "dub_lfoDest": 0,
              "dub_driveAmount": 1.05, "dub_dryLevel": 0.7,
              "dub_delayTime": 0.375, "dub_delayFeedback": 0.6, "dub_delayMix": 0.35,
              "dub_reverbSize": 0.88, "dub_reverbDamp": 0.25, "dub_reverbMix": 0.45,
              "dub_level": 0.68, "dub_voiceMode": 0, "dub_polyphony": 4}}),

    ("Detuned Choir", "Prism", "Dub",
     "Detuned oscillator choir filtered through rising envelope — vocal dub harmonics.",
     ["dub", "detuned", "choir", "vocal", "harmonic"],
     {"brightness": 0.55, "warmth": 0.7, "movement": 0.45, "density": 0.6, "space": 0.7, "aggression": 0.1},
     {"Dub": {"dub_oscWave": 0, "dub_oscOctave": 0, "dub_oscTune": 7.0, "dub_oscPwm": 0.5,
              "dub_subLevel": 0.3, "dub_noiseLevel": 0.0, "dub_drift": 0.08,
              "dub_filterCutoff": 2200.0, "dub_filterReso": 0.4, "dub_filterMode": 0,
              "dub_filterEnvAmt": 0.5, "dub_attack": 0.08, "dub_decay": 0.6,
              "dub_sustain": 0.5, "dub_release": 0.7,
              "dub_pitchEnvDepth": 0.03, "dub_pitchEnvDecay": 0.05,
              "dub_lfoRate": 0.15, "dub_lfoDepth": 0.025, "dub_lfoDest": 0,
              "dub_driveAmount": 1.1, "dub_dryLevel": 0.8,
              "dub_delayTime": 0.5, "dub_delayFeedback": 0.5, "dub_delayMix": 0.25,
              "dub_reverbSize": 0.65, "dub_reverbDamp": 0.4, "dub_reverbMix": 0.32,
              "dub_level": 0.7, "dub_voiceMode": 0, "dub_polyphony": 4}}),

    ("Sine Sub Tone", "Prism", "Dub",
     "Pure sub fundamental with precision — one frequency, maximum weight.",
     ["dub", "sub", "sine", "pure", "weight"],
     {"brightness": 0.15, "warmth": 0.8, "movement": 0.15, "density": 0.75, "space": 0.4, "aggression": 0.4},
     {"Dub": {"dub_oscWave": 0, "dub_oscOctave": -2, "dub_oscTune": 0.0, "dub_oscPwm": 0.5,
              "dub_subLevel": 0.85, "dub_noiseLevel": 0.0, "dub_drift": 0.01,
              "dub_filterCutoff": 400.0, "dub_filterReso": 0.2, "dub_filterMode": 0,
              "dub_filterEnvAmt": 0.1, "dub_attack": 0.015, "dub_decay": 0.8,
              "dub_sustain": 0.7, "dub_release": 0.5,
              "dub_pitchEnvDepth": 0.04, "dub_pitchEnvDecay": 0.03,
              "dub_lfoRate": 0.0, "dub_lfoDepth": 0.0, "dub_lfoDest": 0,
              "dub_driveAmount": 1.4, "dub_dryLevel": 0.95,
              "dub_delayTime": 0.5, "dub_delayFeedback": 0.25, "dub_delayMix": 0.08,
              "dub_reverbSize": 0.35, "dub_reverbDamp": 0.7, "dub_reverbMix": 0.1,
              "dub_level": 0.78, "dub_voiceMode": 1, "dub_polyphony": 0}}),

    ("Falling Note", "Prism", "Dub",
     "Pitch envelope plummet into bass territory — the classic falling dub note.",
     ["dub", "pitch-fall", "classic", "transient", "envelope"],
     {"brightness": 0.4, "warmth": 0.65, "movement": 0.65, "density": 0.5, "space": 0.55, "aggression": 0.35},
     {"Dub": {"dub_oscWave": 2, "dub_oscOctave": 0, "dub_oscTune": 0.0, "dub_oscPwm": 0.4,
              "dub_subLevel": 0.5, "dub_noiseLevel": 0.0, "dub_drift": 0.04,
              "dub_filterCutoff": 3000.0, "dub_filterReso": 0.35, "dub_filterMode": 0,
              "dub_filterEnvAmt": 0.4, "dub_attack": 0.003, "dub_decay": 0.5,
              "dub_sustain": 0.3, "dub_release": 0.3,
              "dub_pitchEnvDepth": -0.5, "dub_pitchEnvDecay": 0.12,
              "dub_lfoRate": 0.0, "dub_lfoDepth": 0.0, "dub_lfoDest": 0,
              "dub_driveAmount": 1.2, "dub_dryLevel": 0.88,
              "dub_delayTime": 0.375, "dub_delayFeedback": 0.42, "dub_delayMix": 0.22,
              "dub_reverbSize": 0.5, "dub_reverbDamp": 0.5, "dub_reverbMix": 0.2,
              "dub_level": 0.74, "dub_voiceMode": 0, "dub_polyphony": 4}}),

    ("Prism Stab", "Prism", "Dub",
     "Hard dub stab — no sustain, maximum attack, reverb throws it into space.",
     ["dub", "stab", "attack", "sharp", "prism"],
     {"brightness": 0.65, "warmth": 0.55, "movement": 0.7, "density": 0.5, "space": 0.65, "aggression": 0.5},
     {"Dub": {"dub_oscWave": 1, "dub_oscOctave": 0, "dub_oscTune": 0.0, "dub_oscPwm": 0.6,
              "dub_subLevel": 0.35, "dub_noiseLevel": 0.02, "dub_drift": 0.03,
              "dub_filterCutoff": 2500.0, "dub_filterReso": 0.6, "dub_filterMode": 0,
              "dub_filterEnvAmt": 0.7, "dub_attack": 0.001, "dub_decay": 0.2,
              "dub_sustain": 0.0, "dub_release": 0.15,
              "dub_pitchEnvDepth": 0.2, "dub_pitchEnvDecay": 0.05,
              "dub_lfoRate": 0.0, "dub_lfoDepth": 0.0, "dub_lfoDest": 0,
              "dub_driveAmount": 1.3, "dub_dryLevel": 0.85,
              "dub_delayTime": 0.375, "dub_delayFeedback": 0.55, "dub_delayMix": 0.3,
              "dub_reverbSize": 0.6, "dub_reverbDamp": 0.35, "dub_reverbMix": 0.35,
              "dub_level": 0.72, "dub_voiceMode": 0, "dub_polyphony": 4}}),
]


# =============================================================================
# DRIFT / MISSING MOODS — Aether (18), Flux (12), Entangled (12)
# =============================================================================

DRIFT_AETHER = [
    ("Event Horizon", "Aether", "Drift",
     "Deep space pad — past the point of return, only shimmer remains.",
     ["drift", "space", "deep", "shimmer", "aether"],
     {"brightness": 0.35, "warmth": 0.4, "movement": 0.3, "density": 0.5, "space": 0.95, "aggression": 0.05},
     {"Drift": {"drift_oscA_mode": 0, "drift_oscA_level": 0.5, "drift_oscA_detune": 0.45,
                "drift_oscB_mode": 2, "drift_oscB_level": 0.4, "drift_oscB_detune": 0.4, "drift_oscB_tune": 12.0,
                "drift_subLevel": 0.1, "drift_noiseLevel": 0.05, "drift_hazeAmount": 0.45,
                "drift_filterCutoff": 4500.0, "drift_filterReso": 0.1, "drift_filterSlope": 0,
                "drift_shimmerAmount": 0.7, "drift_shimmerTone": 0.7,
                "drift_attack": 4.0, "drift_decay": 8.0, "drift_sustain": 0.5, "drift_release": 6.0,
                "drift_driftDepth": 0.55, "drift_driftRate": 0.02,
                "drift_lfoRate": 0.04, "drift_lfoDepth": 0.04, "drift_lfoDest": 0,
                "drift_level": 0.65, "drift_voiceMode": 0, "drift_polyphony": 4}}),

    ("Dark Nebula", "Aether", "Drift",
     "Cold harmonic void — formant filter opens to reveal alien resonances.",
     ["drift", "dark", "formant", "cold", "alien"],
     {"brightness": 0.2, "warmth": 0.25, "movement": 0.35, "density": 0.6, "space": 0.88, "aggression": 0.15},
     {"Drift": {"drift_oscA_mode": 1, "drift_oscA_level": 0.6, "drift_oscA_detune": 0.5,
                "drift_oscB_mode": 1, "drift_oscB_level": 0.3, "drift_oscB_detune": 0.45, "drift_oscB_tune": -12.0,
                "drift_subLevel": 0.2, "drift_noiseLevel": 0.1, "drift_hazeAmount": 0.3,
                "drift_filterCutoff": 2500.0, "drift_filterReso": 0.25, "drift_filterSlope": 1,
                "drift_formantMorph": 0.6, "drift_formantMix": 0.35,
                "drift_shimmerAmount": 0.2, "drift_shimmerTone": 0.4,
                "drift_attack": 3.0, "drift_decay": 7.0, "drift_sustain": 0.45, "drift_release": 5.0,
                "drift_driftDepth": 0.4, "drift_driftRate": 0.03,
                "drift_lfoRate": 0.05, "drift_lfoDepth": 0.05, "drift_lfoDest": 1,
                "drift_level": 0.6, "drift_voiceMode": 0, "drift_polyphony": 4}}),

    ("Signal Decay", "Aether", "Drift",
     "Radio transmission dissolving into noise — the signal wants to survive.",
     ["drift", "radio", "noise", "dissolve", "signal"],
     {"brightness": 0.5, "warmth": 0.3, "movement": 0.55, "density": 0.4, "space": 0.82, "aggression": 0.2},
     {"Drift": {"drift_oscA_mode": 0, "drift_oscA_level": 0.4, "drift_oscA_detune": 0.3,
                "drift_oscB_mode": 0, "drift_oscB_level": 0.3, "drift_oscB_detune": 0.35,
                "drift_subLevel": 0.0, "drift_noiseLevel": 0.35, "drift_hazeAmount": 0.5,
                "drift_filterCutoff": 6000.0, "drift_filterReso": 0.15, "drift_filterSlope": 0,
                "drift_shimmerAmount": 0.3, "drift_shimmerTone": 0.6,
                "drift_attack": 2.0, "drift_decay": 6.0, "drift_sustain": 0.4, "drift_release": 4.0,
                "drift_driftDepth": 0.65, "drift_driftRate": 0.06,
                "drift_lfoRate": 0.08, "drift_lfoDepth": 0.06, "drift_lfoDest": 1,
                "drift_level": 0.58, "drift_voiceMode": 0, "drift_polyphony": 4}}),

    ("Void Shimmer", "Aether", "Drift",
     "Pure shimmer in the void — no grounding, only overtones adrift.",
     ["drift", "shimmer", "void", "overtone", "aether"],
     {"brightness": 0.75, "warmth": 0.2, "movement": 0.4, "density": 0.3, "space": 0.95, "aggression": 0.05},
     {"Drift": {"drift_oscA_mode": 2, "drift_oscA_level": 0.3, "drift_oscA_detune": 0.5,
                "drift_oscB_mode": 2, "drift_oscB_level": 0.25, "drift_oscB_tune": 7.0, "drift_oscB_detune": 0.45,
                "drift_subLevel": 0.0, "drift_noiseLevel": 0.02, "drift_hazeAmount": 0.2,
                "drift_filterCutoff": 8000.0, "drift_filterReso": 0.08, "drift_filterSlope": 0,
                "drift_shimmerAmount": 0.85, "drift_shimmerTone": 0.8,
                "drift_attack": 5.0, "drift_decay": 10.0, "drift_sustain": 0.5, "drift_release": 8.0,
                "drift_driftDepth": 0.35, "drift_driftRate": 0.02,
                "drift_lfoRate": 0.03, "drift_lfoDepth": 0.03, "drift_lfoDest": 0,
                "drift_level": 0.55, "drift_voiceMode": 0, "drift_polyphony": 4}}),

    ("Thermal Gradient", "Aether", "Drift",
     "Slow temperature shift of a pad — warm to cold over minutes.",
     ["drift", "slow", "evolving", "temperature", "long"],
     {"brightness": 0.45, "warmth": 0.6, "movement": 0.2, "density": 0.5, "space": 0.85, "aggression": 0.08},
     {"Drift": {"drift_oscA_mode": 0, "drift_oscA_level": 0.55, "drift_oscA_detune": 0.35,
                "drift_oscB_mode": 1, "drift_oscB_level": 0.35, "drift_oscB_detune": 0.4,
                "drift_subLevel": 0.1, "drift_noiseLevel": 0.0, "drift_hazeAmount": 0.4,
                "drift_filterCutoff": 5500.0, "drift_filterReso": 0.12, "drift_filterSlope": 0,
                "drift_shimmerAmount": 0.45, "drift_shimmerTone": 0.6,
                "drift_attack": 6.0, "drift_decay": 10.0, "drift_sustain": 0.6, "drift_release": 8.0,
                "drift_driftDepth": 0.5, "drift_driftRate": 0.01,
                "drift_lfoRate": 0.02, "drift_lfoDepth": 0.025, "drift_lfoDest": 0,
                "drift_level": 0.6, "drift_voiceMode": 0, "drift_polyphony": 4}}),

    ("Alien Choir", "Aether", "Drift",
     "Formant choir resonating in vacuum — human voices from another dimension.",
     ["drift", "formant", "choir", "alien", "vocal"],
     {"brightness": 0.5, "warmth": 0.35, "movement": 0.45, "density": 0.55, "space": 0.9, "aggression": 0.1},
     {"Drift": {"drift_oscA_mode": 1, "drift_oscA_level": 0.5, "drift_oscA_detune": 0.4,
                "drift_oscB_mode": 0, "drift_oscB_level": 0.3, "drift_oscB_detune": 0.4,
                "drift_subLevel": 0.0, "drift_noiseLevel": 0.08, "drift_hazeAmount": 0.25,
                "drift_filterCutoff": 3500.0, "drift_filterReso": 0.2, "drift_filterSlope": 0,
                "drift_formantMorph": 0.8, "drift_formantMix": 0.55,
                "drift_shimmerAmount": 0.5, "drift_shimmerTone": 0.65,
                "drift_attack": 3.5, "drift_decay": 8.0, "drift_sustain": 0.5, "drift_release": 6.0,
                "drift_driftDepth": 0.45, "drift_driftRate": 0.04,
                "drift_lfoRate": 0.06, "drift_lfoDepth": 0.04, "drift_lfoDest": 2,
                "drift_level": 0.6, "drift_voiceMode": 0, "drift_polyphony": 4}}),
]

DRIFT_FLUX = [
    ("Climax Wave", "Flux", "Drift",
     "Building tension — the Climax threshold is tuned for maximum dramatic impact.",
     ["drift", "climax", "build", "tension", "dramatic"],
     {"brightness": 0.6, "warmth": 0.5, "movement": 0.75, "density": 0.6, "space": 0.7, "aggression": 0.45},
     {"Drift": {"drift_oscA_mode": 0, "drift_oscA_level": 0.65, "drift_oscA_detune": 0.4,
                "drift_oscB_mode": 2, "drift_oscB_level": 0.45, "drift_oscB_tune": 5.0, "drift_oscB_detune": 0.5,
                "drift_subLevel": 0.15, "drift_noiseLevel": 0.0, "drift_hazeAmount": 0.2,
                "drift_filterCutoff": 5000.0, "drift_filterReso": 0.2, "drift_filterSlope": 0,
                "drift_shimmerAmount": 0.55, "drift_shimmerTone": 0.7,
                "drift_attack": 1.5, "drift_decay": 5.0, "drift_sustain": 0.55, "drift_release": 3.0,
                "drift_driftDepth": 0.4, "drift_driftRate": 0.05,
                "drift_lfoRate": 0.1, "drift_lfoDepth": 0.05, "drift_lfoDest": 0,
                "drift_level": 0.68, "drift_voiceMode": 0, "drift_polyphony": 4}}),

    ("Fracture Point", "Flux", "Drift",
     "Harmonic stability breaking apart — the moment before chaos.",
     ["drift", "fracture", "unstable", "breaking", "transition"],
     {"brightness": 0.55, "warmth": 0.35, "movement": 0.8, "density": 0.55, "space": 0.65, "aggression": 0.55},
     {"Drift": {"drift_oscA_mode": 1, "drift_oscA_level": 0.55, "drift_oscA_detune": 0.55,
                "drift_oscB_mode": 3, "drift_oscB_level": 0.4, "drift_oscB_detune": 0.6,
                "drift_subLevel": 0.0, "drift_noiseLevel": 0.12, "drift_hazeAmount": 0.35,
                "drift_filterCutoff": 4000.0, "drift_filterReso": 0.35, "drift_filterSlope": 1,
                "drift_shimmerAmount": 0.3, "drift_shimmerTone": 0.5,
                "drift_attack": 1.0, "drift_decay": 4.0, "drift_sustain": 0.4, "drift_release": 2.5,
                "drift_driftDepth": 0.7, "drift_driftRate": 0.08,
                "drift_lfoRate": 0.15, "drift_lfoDepth": 0.08, "drift_lfoDest": 1,
                "drift_level": 0.62, "drift_voiceMode": 0, "drift_polyphony": 4}}),

    ("Journey Mid", "Flux", "Drift",
     "Familiar to alien — this is the midpoint, both at once.",
     ["drift", "journey", "transition", "morphing", "hybrid"],
     {"brightness": 0.5, "warmth": 0.5, "movement": 0.65, "density": 0.5, "space": 0.7, "aggression": 0.25},
     {"Drift": {"drift_oscA_mode": 0, "drift_oscA_level": 0.5, "drift_oscA_detune": 0.35,
                "drift_oscB_mode": 1, "drift_oscB_level": 0.45, "drift_oscB_tune": 7.0, "drift_oscB_detune": 0.45,
                "drift_subLevel": 0.1, "drift_noiseLevel": 0.05, "drift_hazeAmount": 0.3,
                "drift_filterCutoff": 5500.0, "drift_filterReso": 0.18, "drift_filterSlope": 0,
                "drift_shimmerAmount": 0.4, "drift_shimmerTone": 0.6,
                "drift_attack": 2.0, "drift_decay": 5.0, "drift_sustain": 0.5, "drift_release": 3.5,
                "drift_driftDepth": 0.5, "drift_driftRate": 0.04,
                "drift_lfoRate": 0.08, "drift_lfoDepth": 0.04, "drift_lfoDest": 0,
                "drift_level": 0.65, "drift_voiceMode": 0, "drift_polyphony": 4}}),

    ("Tidal Sweep", "Flux", "Drift",
     "Tidal pulse driving filter sweep in waves — ocean of sound.",
     ["drift", "tidal", "sweep", "wave", "pulse"],
     {"brightness": 0.45, "warmth": 0.55, "movement": 0.7, "density": 0.55, "space": 0.75, "aggression": 0.2},
     {"Drift": {"drift_oscA_mode": 0, "drift_oscA_level": 0.6, "drift_oscA_detune": 0.3,
                "drift_oscB_mode": 0, "drift_oscB_level": 0.4, "drift_oscB_detune": 0.35,
                "drift_subLevel": 0.2, "drift_noiseLevel": 0.0, "drift_hazeAmount": 0.15,
                "drift_filterCutoff": 4000.0, "drift_filterReso": 0.25, "drift_filterSlope": 0,
                "drift_shimmerAmount": 0.35, "drift_shimmerTone": 0.55,
                "drift_attack": 1.5, "drift_decay": 4.0, "drift_sustain": 0.55, "drift_release": 3.0,
                "drift_driftDepth": 0.35, "drift_driftRate": 0.05,
                "drift_lfoRate": 0.18, "drift_lfoDepth": 0.12, "drift_lfoDest": 1,
                "drift_level": 0.67, "drift_voiceMode": 0, "drift_polyphony": 4}}),
]

DRIFT_ENTANGLED = [
    ("Onset Climax Trigger", "Entangled", ["Drift", "Onset"],
     "Drum hits from ONSET trigger Drift's Climax bloom — cinema percussion.",
     ["drift", "onset", "coupling", "climax", "cinematic"],
     {"brightness": 0.6, "warmth": 0.45, "movement": 0.75, "density": 0.6, "space": 0.75, "aggression": 0.45},
     {"Drift": {"drift_oscA_mode": 0, "drift_oscA_level": 0.6, "drift_oscA_detune": 0.4,
                "drift_oscB_mode": 2, "drift_oscB_level": 0.35, "drift_oscB_tune": 7.0,
                "drift_filterCutoff": 5000.0, "drift_filterReso": 0.15, "drift_shimmerAmount": 0.5,
                "drift_attack": 0.5, "drift_decay": 4.0, "drift_sustain": 0.5, "drift_release": 3.0,
                "drift_driftDepth": 0.4, "drift_driftRate": 0.04, "drift_level": 0.65, "drift_polyphony": 4},
      "Onset": {"perc_v1_blend": 0.2, "perc_v1_decay": 0.6, "perc_v1_tone": 0.3,
                "perc_v1_level": 0.65, "perc_macro_punch": 0.5, "perc_macro_mutate": 0.2}},
     "Moderate",
     [{"engineA": "Onset", "engineB": "Drift", "type": "Amp->Filter", "amount": 0.6}]),

    ("Rhythm Shimmer", "Entangled", ["Drift", "Onset"],
     "Onset rhythm drives Drift shimmer amount — beats create harmonic sparkle.",
     ["drift", "onset", "rhythm", "shimmer", "coupling"],
     {"brightness": 0.65, "warmth": 0.4, "movement": 0.7, "density": 0.55, "space": 0.8, "aggression": 0.3},
     {"Drift": {"drift_oscA_mode": 2, "drift_oscA_level": 0.45, "drift_oscA_detune": 0.4,
                "drift_oscB_mode": 0, "drift_oscB_level": 0.4, "drift_oscB_detune": 0.4,
                "drift_filterCutoff": 6000.0, "drift_filterReso": 0.1, "drift_shimmerAmount": 0.65,
                "drift_attack": 1.5, "drift_decay": 5.0, "drift_sustain": 0.55, "drift_release": 4.0,
                "drift_driftDepth": 0.35, "drift_driftRate": 0.04, "drift_level": 0.62, "drift_polyphony": 4},
      "Onset": {"perc_v1_blend": 0.15, "perc_v1_decay": 0.35, "perc_v1_level": 0.6,
                "perc_v2_blend": 0.3, "perc_v2_decay": 0.15, "perc_v2_level": 0.55,
                "perc_macro_punch": 0.4, "perc_macro_space": 0.55}},
     "Moderate",
     [{"engineA": "Onset", "engineB": "Drift", "type": "Rhythm->Blend", "amount": 0.55}]),

    ("Drift Bob Orbit", "Entangled", ["Drift", "Bob"],
     "BOB's warmth orbits DRIFT's alien core — familiar and strange in balance.",
     ["drift", "bob", "warm", "alien", "layer"],
     {"brightness": 0.5, "warmth": 0.65, "movement": 0.45, "density": 0.6, "space": 0.78, "aggression": 0.12},
     {"Drift": {"drift_oscA_mode": 1, "drift_oscA_level": 0.5, "drift_oscA_detune": 0.4,
                "drift_filterCutoff": 5500.0, "drift_filterReso": 0.1, "drift_shimmerAmount": 0.5,
                "drift_attack": 2.5, "drift_sustain": 0.5, "drift_release": 3.5,
                "drift_driftDepth": 0.45, "drift_level": 0.6, "drift_polyphony": 4},
      "Bob": {"bob_oscA_wave": 0, "bob_oscA_shape": 0.5, "bob_oscA_drift": 0.12,
              "bob_fltMode": 0, "bob_fltCutoff": 4000.0, "bob_fltChar": 0.55,
              "bob_ampAttack": 1.5, "bob_ampSustain": 0.65, "bob_ampRelease": 2.5,
              "bob_bobMode": 0.3, "bob_level": 0.65, "bob_polyphony": 4}},
     "Subtle",
     [{"engineA": "Bob", "engineB": "Drift", "type": "Amp->Filter", "amount": 0.4}]),

    ("FM Drift", "Entangled", ["Drift", "Morph"],
     "MORPH's morphing engine modulates DRIFT's FM depth — texture multiplied.",
     ["drift", "morph", "FM", "texture", "coupling"],
     {"brightness": 0.55, "warmth": 0.4, "movement": 0.65, "density": 0.6, "space": 0.72, "aggression": 0.3},
     {"Drift": {"drift_oscA_mode": 3, "drift_oscA_level": 0.55, "drift_oscA_detune": 0.35,
                "drift_oscA_fmDepth": 0.4,
                "drift_filterCutoff": 4500.0, "drift_filterReso": 0.2, "drift_shimmerAmount": 0.3,
                "drift_attack": 1.5, "drift_sustain": 0.5, "drift_release": 2.5,
                "drift_driftDepth": 0.45, "drift_level": 0.62, "drift_polyphony": 4},
      "Morph": {"morph_morph": 0.5, "morph_bloom": 0.4, "morph_decay": 0.8,
                "morph_filterCutoff": 5000.0, "morph_filterReso": 0.15,
                "morph_driftDepth": 0.3, "morph_level": 0.6, "morph_polyphony": 4}},
     "Moderate",
     [{"engineA": "Morph", "engineB": "Drift", "type": "Env->Morph", "amount": 0.55}]),
]


# =============================================================================
# SNAP GAP FILLS — Aether (8), Atmosphere (8)
# =============================================================================

SNAP_AETHER = [
    ("Phantom Tick", "Aether", "Snap",
     "Barely audible snap — a ghost in the machine ticking in void.",
     ["snap", "ghost", "quiet", "aether", "minimal"],
     {"brightness": 0.55, "warmth": 0.2, "movement": 0.25, "density": 0.2, "space": 0.9, "aggression": 0.1},
     {"Snap": {"snap_oscMode": 0, "snap_oscTune": 0.0, "snap_oscShape": 0.3,
               "snap_filterCutoff": 8000.0, "snap_filterReso": 0.15, "snap_filterEnvAmt": 0.5,
               "snap_attack": 0.0, "snap_decay": 0.06, "snap_sustain": 0.0, "snap_release": 0.08,
               "snap_snap": 0.3, "snap_level": 0.4, "snap_detune": 0.0}}),

    ("Silicon Echo", "Aether", "Snap",
     "Crisp digital snap thrown into infinite reverb — silicon signal lost in space.",
     ["snap", "digital", "reverb", "space", "echo"],
     {"brightness": 0.7, "warmth": 0.15, "movement": 0.3, "density": 0.25, "space": 0.95, "aggression": 0.2},
     {"Snap": {"snap_oscMode": 2, "snap_oscTune": 0.0, "snap_oscShape": 0.6,
               "snap_filterCutoff": 12000.0, "snap_filterReso": 0.1, "snap_filterEnvAmt": 0.4,
               "snap_attack": 0.0, "snap_decay": 0.04, "snap_sustain": 0.0, "snap_release": 0.05,
               "snap_snap": 0.7, "snap_level": 0.5, "snap_detune": 0.05}}),

    ("Abstract Pop", "Aether", "Snap",
     "Detuned abstract snap — no sonic reference point, pure transient concept.",
     ["snap", "abstract", "detuned", "concept", "aether"],
     {"brightness": 0.5, "warmth": 0.3, "movement": 0.4, "density": 0.3, "space": 0.85, "aggression": 0.25},
     {"Snap": {"snap_oscMode": 1, "snap_oscTune": 7.0, "snap_oscShape": 0.7,
               "snap_filterCutoff": 6000.0, "snap_filterReso": 0.35, "snap_filterEnvAmt": 0.6,
               "snap_attack": 0.0, "snap_decay": 0.08, "snap_sustain": 0.0, "snap_release": 0.1,
               "snap_snap": 0.5, "snap_level": 0.45, "snap_detune": 0.15}}),

    ("Resonant Void", "Aether", "Snap",
     "High-resonance snap with nothing beneath — resonance as sculpture.",
     ["snap", "resonance", "void", "pure", "aether"],
     {"brightness": 0.65, "warmth": 0.1, "movement": 0.2, "density": 0.15, "space": 0.92, "aggression": 0.3},
     {"Snap": {"snap_oscMode": 0, "snap_oscTune": 0.0, "snap_oscShape": 0.5,
               "snap_filterCutoff": 5000.0, "snap_filterReso": 0.8, "snap_filterEnvAmt": 0.7,
               "snap_attack": 0.0, "snap_decay": 0.07, "snap_sustain": 0.0, "snap_release": 0.08,
               "snap_snap": 0.6, "snap_level": 0.42, "snap_detune": 0.0}}),
]

SNAP_ATMOSPHERE = [
    ("Rain Tap", "Atmosphere", "Snap",
     "Single raindrop — snapping against glass in slow irregular rhythm.",
     ["snap", "rain", "organic", "gentle", "atmosphere"],
     {"brightness": 0.6, "warmth": 0.45, "movement": 0.35, "density": 0.3, "space": 0.72, "aggression": 0.05},
     {"Snap": {"snap_oscMode": 0, "snap_oscTune": 0.0, "snap_oscShape": 0.4,
               "snap_filterCutoff": 7000.0, "snap_filterReso": 0.2, "snap_filterEnvAmt": 0.45,
               "snap_attack": 0.0, "snap_decay": 0.05, "snap_sustain": 0.0, "snap_release": 0.07,
               "snap_snap": 0.35, "snap_level": 0.5, "snap_detune": 0.0}}),

    ("Brushed Surface", "Atmosphere", "Snap",
     "Soft brush against a surface — snap with rounded attack and warm tone.",
     ["snap", "brush", "soft", "warm", "texture"],
     {"brightness": 0.4, "warmth": 0.65, "movement": 0.3, "density": 0.4, "space": 0.65, "aggression": 0.08},
     {"Snap": {"snap_oscMode": 1, "snap_oscTune": -5.0, "snap_oscShape": 0.3,
               "snap_filterCutoff": 4000.0, "snap_filterReso": 0.15, "snap_filterEnvAmt": 0.3,
               "snap_attack": 0.005, "snap_decay": 0.1, "snap_sustain": 0.0, "snap_release": 0.12,
               "snap_snap": 0.2, "snap_level": 0.55, "snap_detune": 0.05}}),

    ("Muted Click", "Atmosphere", "Snap",
     "Cloth-muted snap — intimate, close-mic'd, present but gentle.",
     ["snap", "muted", "intimate", "close", "atmosphere"],
     {"brightness": 0.3, "warmth": 0.7, "movement": 0.25, "density": 0.5, "space": 0.5, "aggression": 0.12},
     {"Snap": {"snap_oscMode": 0, "snap_oscTune": -3.0, "snap_oscShape": 0.25,
               "snap_filterCutoff": 3500.0, "snap_filterReso": 0.1, "snap_filterEnvAmt": 0.25,
               "snap_attack": 0.002, "snap_decay": 0.08, "snap_sustain": 0.0, "snap_release": 0.1,
               "snap_snap": 0.15, "snap_level": 0.6, "snap_detune": 0.02}}),

    ("Wood Grain", "Atmosphere", "Snap",
     "Organic wooden snap — knuckle on hollow wood, natural resonance.",
     ["snap", "wood", "organic", "natural", "acoustic"],
     {"brightness": 0.45, "warmth": 0.72, "movement": 0.3, "density": 0.45, "space": 0.55, "aggression": 0.1},
     {"Snap": {"snap_oscMode": 1, "snap_oscTune": -7.0, "snap_oscShape": 0.35,
               "snap_filterCutoff": 5000.0, "snap_filterReso": 0.25, "snap_filterEnvAmt": 0.4,
               "snap_attack": 0.001, "snap_decay": 0.09, "snap_sustain": 0.0, "snap_release": 0.12,
               "snap_snap": 0.25, "snap_level": 0.58, "snap_detune": 0.03}}),
]


# =============================================================================
# ONSET COUPLING PRESETS — Onset paired with Bob, Morph, Dub, Snap, Fat
# =============================================================================

ONSET_COUPLING = [
    ("Kick Drives Warmth", "Entangled", ["Onset", "Bob"],
     "Kick amplitude from ONSET sweeps BOB's filter — the bass breathes with the beat.",
     ["onset", "bob", "kick", "filter", "coupling", "live"],
     {"brightness": 0.45, "warmth": 0.7, "movement": 0.65, "density": 0.6, "space": 0.55, "aggression": 0.35},
     {"Onset": {"perc_v1_blend": 0.1, "perc_v1_algoMode": 1, "perc_v1_decay": 0.7,
                "perc_v1_tone": 0.35, "perc_v1_snap": 0.6, "perc_v1_body": 0.8,
                "perc_v1_level": 0.75, "perc_macro_punch": 0.6, "perc_macro_machine": 0.3},
      "Bob": {"bob_oscA_wave": 0, "bob_oscA_shape": 0.5, "bob_oscA_drift": 0.08,
              "bob_fltMode": 0, "bob_fltCutoff": 800.0, "bob_fltChar": 0.65, "bob_fltEnvAmt": 0.5,
              "bob_ampAttack": 0.005, "bob_ampDecay": 0.4, "bob_ampSustain": 0.7, "bob_ampRelease": 0.5,
              "bob_motAttack": 0.01, "bob_motDecay": 0.4, "bob_motSustain": 0.5, "bob_motRelease": 0.4,
              "bob_motDepth": 0.55, "bob_bobMode": 0.35, "bob_level": 0.7, "bob_polyphony": 0}},
     "Strong",
     [{"engineA": "Onset", "engineB": "Bob", "type": "Amp->Filter", "amount": 0.7}]),

    ("Snare Morph Trigger", "Entangled", ["Onset", "Morph"],
     "Snare from ONSET drives MORPH's morph parameter — rhythm shapes synthesis.",
     ["onset", "morph", "snare", "morph", "coupling", "rhythm"],
     {"brightness": 0.55, "warmth": 0.45, "movement": 0.7, "density": 0.55, "space": 0.6, "aggression": 0.4},
     {"Onset": {"perc_v1_blend": 0.2, "perc_v1_decay": 0.6, "perc_v1_level": 0.65,
                "perc_v2_blend": 0.3, "perc_v2_algoMode": 0, "perc_v2_decay": 0.25,
                "perc_v2_tone": 0.6, "perc_v2_snap": 0.7, "perc_v2_level": 0.7,
                "perc_macro_punch": 0.5, "perc_macro_mutate": 0.25},
      "Morph": {"morph_morph": 0.4, "morph_bloom": 0.5, "morph_decay": 0.7,
                "morph_filterCutoff": 4500.0, "morph_filterReso": 0.2, "morph_drift": 0.2,
                "morph_level": 0.65, "morph_polyphony": 4}},
     "Strong",
     [{"engineA": "Onset", "engineB": "Morph", "type": "Rhythm->Blend", "amount": 0.65}]),

    ("Hi Hat Dub Send", "Entangled", ["Onset", "Dub"],
     "Hi-hats from ONSET control DUB's send amount — cymbals mix into echo.",
     ["onset", "dub", "hihat", "send", "echo", "coupling"],
     {"brightness": 0.5, "warmth": 0.6, "movement": 0.6, "density": 0.5, "space": 0.7, "aggression": 0.25},
     {"Onset": {"perc_v3_blend": 0.5, "perc_v3_algoMode": 2, "perc_v3_decay": 0.12,
                "perc_v3_tone": 0.8, "perc_v3_level": 0.6,
                "perc_v4_blend": 0.6, "perc_v4_algoMode": 2, "perc_v4_decay": 0.5,
                "perc_v4_tone": 0.75, "perc_v4_level": 0.55,
                "perc_macro_space": 0.55},
      "Dub": {"dub_oscWave": 2, "dub_oscOctave": 0, "dub_filterCutoff": 1500.0,
              "dub_filterReso": 0.4, "dub_filterEnvAmt": 0.35,
              "dub_attack": 0.01, "dub_decay": 0.4, "dub_sustain": 0.5, "dub_release": 0.4,
              "dub_sendLevel": 0.7, "dub_delayTime": 0.375, "dub_delayFeedback": 0.5,
              "dub_delayMix": 0.3, "dub_reverbSize": 0.55, "dub_reverbMix": 0.22,
              "dub_level": 0.7, "dub_voiceMode": 1}},
     "Moderate",
     [{"engineA": "Onset", "engineB": "Dub", "type": "Amp->Filter", "amount": 0.5}]),

    ("Rim Snap Sync", "Entangled", ["Onset", "Snap"],
     "ONSET rim triggers SNAP's sharp attack — stacked transients, double impact.",
     ["onset", "snap", "rim", "transient", "stack", "coupling"],
     {"brightness": 0.7, "warmth": 0.35, "movement": 0.7, "density": 0.5, "space": 0.45, "aggression": 0.6},
     {"Onset": {"perc_v5_blend": 0.4, "perc_v5_algoMode": 0, "perc_v5_decay": 0.08,
                "perc_v5_tone": 0.7, "perc_v5_snap": 0.85, "perc_v5_level": 0.65,
                "perc_macro_punch": 0.7, "perc_macro_machine": 0.45},
      "Snap": {"snap_oscMode": 2, "snap_oscTune": 0.0, "snap_oscShape": 0.65,
               "snap_filterCutoff": 9000.0, "snap_filterReso": 0.3, "snap_filterEnvAmt": 0.6,
               "snap_attack": 0.0, "snap_decay": 0.05, "snap_sustain": 0.0, "snap_release": 0.06,
               "snap_snap": 0.8, "snap_level": 0.6}},
     "Moderate",
     [{"engineA": "Onset", "engineB": "Snap", "type": "Amp->Pitch", "amount": 0.45}]),

    ("Beat Drives Fat", "Entangled", ["Onset", "Fat"],
     "Kick energy from ONSET pumps FAT's density — classic sidechain feel, synthesized.",
     ["onset", "fat", "sidechain", "pump", "kick", "coupling"],
     {"brightness": 0.5, "warmth": 0.6, "movement": 0.75, "density": 0.65, "space": 0.5, "aggression": 0.45},
     {"Onset": {"perc_v1_blend": 0.05, "perc_v1_algoMode": 1, "perc_v1_decay": 0.65,
                "perc_v1_snap": 0.5, "perc_v1_body": 0.75, "perc_v1_level": 0.8,
                "perc_macro_punch": 0.65, "perc_macro_machine": 0.2},
      "Fat": {"fat_morph": 0.5, "fat_mojo": 0.55, "fat_subLevel": 0.4, "fat_groupMix": 0.9,
              "fat_detune": 8.0, "fat_fltCutoff": 3000.0, "fat_fltReso": 0.25,
              "fat_fltDrive": 0.1, "fat_fltEnvAmt": 0.25,
              "fat_ampAttack": 0.003, "fat_ampDecay": 0.5, "fat_ampSustain": 0.7, "fat_ampRelease": 0.6,
              "fat_satDrive": 0.12, "fat_level": 0.75, "fat_voiceMode": 0, "fat_polyphony": 4}},
     "Strong",
     [{"engineA": "Onset", "engineB": "Fat", "type": "Amp->Filter", "amount": 0.65}]),

    ("Percussion Shimmer", "Entangled", ["Onset", "Drift"],
     "Each drum hit triggers Drift shimmer burst — percussion creates harmonic halos.",
     ["onset", "drift", "shimmer", "halo", "transient", "coupling"],
     {"brightness": 0.7, "warmth": 0.4, "movement": 0.65, "density": 0.45, "space": 0.82, "aggression": 0.3},
     {"Onset": {"perc_v1_blend": 0.3, "perc_v1_decay": 0.45, "perc_v1_level": 0.65,
                "perc_v2_blend": 0.4, "perc_v2_decay": 0.2, "perc_v2_level": 0.6,
                "perc_macro_punch": 0.45, "perc_macro_space": 0.5},
      "Drift": {"drift_oscA_mode": 2, "drift_oscA_level": 0.45, "drift_oscA_detune": 0.4,
                "drift_filterCutoff": 7000.0, "drift_filterReso": 0.1,
                "drift_shimmerAmount": 0.7, "drift_shimmerTone": 0.75,
                "drift_attack": 0.3, "drift_sustain": 0.35, "drift_release": 2.5,
                "drift_driftDepth": 0.3, "drift_level": 0.55, "drift_polyphony": 4}},
     "Moderate",
     [{"engineA": "Onset", "engineB": "Drift", "type": "Amp->Filter", "amount": 0.55}]),

    ("Four Engine Orchestra", "Entangled", ["Onset", "Bob", "Drift", "Dub"],
     "All four engines locked together — percussive drives melodic drives harmonic drives space.",
     ["onset", "bob", "drift", "dub", "orchestra", "four-engine", "epic"],
     {"brightness": 0.55, "warmth": 0.6, "movement": 0.7, "density": 0.8, "space": 0.75, "aggression": 0.4},
     {"Onset": {"perc_v1_blend": 0.1, "perc_v1_decay": 0.7, "perc_v1_level": 0.7,
                "perc_v2_blend": 0.25, "perc_v2_decay": 0.2, "perc_v2_level": 0.65,
                "perc_macro_punch": 0.55},
      "Bob": {"bob_oscA_wave": 0, "bob_oscA_drift": 0.1, "bob_fltCutoff": 2000.0,
              "bob_fltChar": 0.6, "bob_ampAttack": 0.5, "bob_ampSustain": 0.65,
              "bob_bobMode": 0.35, "bob_level": 0.65, "bob_polyphony": 4},
      "Drift": {"drift_oscA_mode": 0, "drift_oscA_level": 0.5, "drift_filterCutoff": 5500.0,
                "drift_shimmerAmount": 0.45, "drift_attack": 1.5, "drift_sustain": 0.5,
                "drift_driftDepth": 0.4, "drift_level": 0.55, "drift_polyphony": 4},
      "Dub": {"dub_oscWave": 2, "dub_oscOctave": -1, "dub_subLevel": 0.55,
              "dub_filterCutoff": 800.0, "dub_filterEnvAmt": 0.35,
              "dub_attack": 0.01, "dub_sustain": 0.6, "dub_sendLevel": 0.5,
              "dub_delayTime": 0.375, "dub_delayMix": 0.22, "dub_level": 0.65, "dub_voiceMode": 1}},
     "Strong",
     [{"engineA": "Onset", "engineB": "Bob", "type": "Amp->Filter", "amount": 0.55},
      {"engineA": "Bob", "engineB": "Drift", "type": "Amp->Filter", "amount": 0.4},
      {"engineA": "Onset", "engineB": "Dub", "type": "Rhythm->Blend", "amount": 0.45}]),
]


# =============================================================================
# MORPH — MISSING MOODS: Flux (8), Prism (8)
# =============================================================================

MORPH_FLUX = [
    ("Morph Surge", "Flux", "Morph",
     "Morph parameter surging forward — a slow-motion wave of tonal transformation.",
     ["morph", "flux", "surge", "evolving", "transform"],
     {"brightness": 0.55, "warmth": 0.5, "movement": 0.75, "density": 0.55, "space": 0.65, "aggression": 0.3},
     {"Morph": {"morph_morph": 0.6, "morph_bloom": 0.5, "morph_decay": 0.8, "morph_detune": 0.3,
                "morph_drift": 0.3, "morph_filterCutoff": 5000.0, "morph_filterReso": 0.22,
                "morph_sustain": 0.55, "morph_release": 1.5,
                "morph_sub": 0.2, "morph_level": 0.65, "morph_polyphony": 4}}),

    ("Bloom Collapse", "Flux", "Morph",
     "Bloom opens, then collapses under its own weight — a cycle of expansion.",
     ["morph", "bloom", "collapse", "cycle", "evolving"],
     {"brightness": 0.5, "warmth": 0.55, "movement": 0.8, "density": 0.6, "space": 0.7, "aggression": 0.35},
     {"Morph": {"morph_morph": 0.35, "morph_bloom": 0.75, "morph_decay": 1.0, "morph_detune": 0.4,
                "morph_drift": 0.35, "morph_filterCutoff": 4500.0, "morph_filterReso": 0.3,
                "morph_sustain": 0.45, "morph_release": 2.0,
                "morph_sub": 0.1, "morph_level": 0.63, "morph_polyphony": 4}}),

    ("State Machine", "Flux", "Morph",
     "Discrete morph states cycling — feels like digital state machine oscillating.",
     ["morph", "state", "digital", "cycle", "flux"],
     {"brightness": 0.6, "warmth": 0.35, "movement": 0.7, "density": 0.5, "space": 0.6, "aggression": 0.4},
     {"Morph": {"morph_morph": 0.75, "morph_bloom": 0.35, "morph_decay": 0.5, "morph_detune": 0.25,
                "morph_drift": 0.15, "morph_filterCutoff": 5500.0, "morph_filterReso": 0.25,
                "morph_sustain": 0.4, "morph_release": 0.8,
                "morph_sub": 0.0, "morph_level": 0.66, "morph_polyphony": 4}}),

    ("Drift Bloom", "Flux", "Morph",
     "Drift modulation on maximum — each note drifts into its own version of the patch.",
     ["morph", "drift", "bloom", "evolve", "organic"],
     {"brightness": 0.45, "warmth": 0.6, "movement": 0.75, "density": 0.5, "space": 0.72, "aggression": 0.15},
     {"Morph": {"morph_morph": 0.5, "morph_bloom": 0.6, "morph_decay": 0.9, "morph_detune": 0.45,
                "morph_drift": 0.6, "morph_filterCutoff": 4000.0, "morph_filterReso": 0.18,
                "morph_sustain": 0.5, "morph_release": 2.0,
                "morph_sub": 0.15, "morph_level": 0.62, "morph_polyphony": 4}}),
]

MORPH_PRISM = [
    ("Spectral Morph", "Prism", "Morph",
     "Morph reveals harmonic structure — each position is a different spectral fingerprint.",
     ["morph", "spectral", "harmonic", "prism", "tonal"],
     {"brightness": 0.65, "warmth": 0.45, "movement": 0.4, "density": 0.55, "space": 0.68, "aggression": 0.2},
     {"Morph": {"morph_morph": 0.5, "morph_bloom": 0.3, "morph_decay": 0.6, "morph_detune": 0.2,
                "morph_drift": 0.1, "morph_filterCutoff": 6000.0, "morph_filterReso": 0.3,
                "morph_sustain": 0.55, "morph_release": 1.0,
                "morph_sub": 0.1, "morph_level": 0.67, "morph_polyphony": 4}}),

    ("Harmonic Series", "Prism", "Morph",
     "Morph sweeps through the harmonic series — tuned, purposeful, spectral.",
     ["morph", "harmonic", "series", "tonal", "prism"],
     {"brightness": 0.6, "warmth": 0.5, "movement": 0.35, "density": 0.5, "space": 0.65, "aggression": 0.18},
     {"Morph": {"morph_morph": 0.3, "morph_bloom": 0.25, "morph_decay": 0.7, "morph_detune": 0.15,
                "morph_drift": 0.08, "morph_filterCutoff": 5500.0, "morph_filterReso": 0.35,
                "morph_sustain": 0.6, "morph_release": 0.9,
                "morph_sub": 0.2, "morph_level": 0.68, "morph_polyphony": 4}}),

    ("Pure Resonance", "Prism", "Morph",
     "High resonance in mid-morph position — where the two states agree on one tone.",
     ["morph", "resonance", "pure", "tonal", "prism"],
     {"brightness": 0.7, "warmth": 0.4, "movement": 0.25, "density": 0.4, "space": 0.7, "aggression": 0.25},
     {"Morph": {"morph_morph": 0.5, "morph_bloom": 0.2, "morph_decay": 0.5, "morph_detune": 0.1,
                "morph_drift": 0.05, "morph_filterCutoff": 4000.0, "morph_filterReso": 0.65,
                "morph_sustain": 0.45, "morph_release": 0.7,
                "morph_sub": 0.0, "morph_level": 0.64, "morph_polyphony": 4}}),

    ("Morphed Lead", "Prism", "Morph",
     "Lead tone shaped by morph position — different notes land in different spectral zones.",
     ["morph", "lead", "melody", "prism", "expressive"],
     {"brightness": 0.65, "warmth": 0.5, "movement": 0.5, "density": 0.45, "space": 0.55, "aggression": 0.3},
     {"Morph": {"morph_morph": 0.6, "morph_bloom": 0.4, "morph_decay": 0.4, "morph_detune": 0.2,
                "morph_drift": 0.12, "morph_filterCutoff": 5000.0, "morph_filterReso": 0.3,
                "morph_sustain": 0.35, "morph_release": 0.5,
                "morph_sub": 0.0, "morph_level": 0.7, "morph_voiceMode": 1, "morph_polyphony": 0}}),
]


# =============================================================================
# MAIN
# =============================================================================

def _write_group(presets, label):
    print(f"\n=== {label} ({len(presets)}) ===")
    for entry in presets:
        # Unpack based on tuple length
        if len(entry) == 7:
            name, mood, engine, desc, tags, dna, params = entry
            coupling_intensity = "None"
            coupling_pairs = []
        elif len(entry) == 9:
            name, mood, engine, desc, tags, dna, params, coupling_intensity, coupling_pairs = entry
        else:
            print(f"  [SKIP] Bad entry: {entry[0]}")
            continue

        preset = make_preset(name, mood, engine, desc, tags, dna, params,
                             coupling_intensity, coupling_pairs)
        write_preset(preset)


if __name__ == "__main__":
    print("Generating library gap fills...")

    _write_group(DUB_PRISM, "Dub / Prism")
    _write_group(DRIFT_AETHER, "Drift / Aether")
    _write_group(DRIFT_FLUX, "Drift / Flux")
    _write_group(DRIFT_ENTANGLED, "Drift / Entangled")
    _write_group(SNAP_AETHER, "Snap / Aether")
    _write_group(SNAP_ATMOSPHERE, "Snap / Atmosphere")
    _write_group(ONSET_COUPLING, "Onset Coupling")
    _write_group(MORPH_FLUX, "Morph / Flux")
    _write_group(MORPH_PRISM, "Morph / Prism")

    totals = (len(DUB_PRISM) + len(DRIFT_AETHER) + len(DRIFT_FLUX) +
              len(DRIFT_ENTANGLED) + len(SNAP_AETHER) + len(SNAP_ATMOSPHERE) +
              len(ONSET_COUPLING) + len(MORPH_FLUX) + len(MORPH_PRISM))
    print(f"\nTotal: {totals} presets generated")
