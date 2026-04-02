#!/usr/bin/env python3
"""OUIE Factory Preset Generator — 150 presets for the Hammerhead duophonic synth.

Generates .xometa JSON files across 7 moods:
  Foundation (30): Algorithm showcases + basics
  Atmosphere (25): Pads, drones, environments
  Entangled (20): Coupling-ready duophonic setups
  Prism (25): Genre production (Techno/Synthwave/Cinematic/DnB)
  Flux (20): EDM/Lo-Fi/Movement
  Aether (15): Experimental/deep STRIFE-LOVE explorations
  Family (15): OUIE + fleet engine coupling

OUIE identity: Hammerhead Steel #708090, duophonic (2 voices x 8 algorithms),
STRIFE/LOVE interaction axis, Split/Layer/Duo voice modes.
"""

import json
import os

PRESET_DIR = os.path.join(os.path.dirname(__file__), "..", "Presets", "XOceanus")

# ── OUIE defaults (53 params) ────────────────────────────────────────────────
# Derived from OuieEngine.h parameter registration (addParametersImpl)
DEFAULTS = {
    # Voice 1 Algorithm
    "ouie_algo1": 0,           # VA (0=VA, 1=Wavetable, 2=FM, 3=Additive, 4=PhaseDist, 5=Wavefolder, 6=KS, 7=Noise)
    "ouie_waveform1": 0,       # Saw (0=Saw, 1=Square, 2=Triangle) — VA only
    "ouie_algoParam1": 0.5,    # Algorithm-specific parameter (0..1)
    "ouie_wtPos1": 0.0,        # Wavetable position (0..1)
    "ouie_fmRatio1": 2.0,      # FM carrier:mod ratio (0.5..16)
    "ouie_fmIndex1": 1.0,      # FM modulation index (0..10)
    "ouie_pw1": 0.5,           # Pulse width (0.1..0.9) — VA square only
    # Voice 2 Algorithm
    "ouie_algo2": 4,           # Phase Dist (default: rough algorithm for contrast)
    "ouie_waveform2": 0,       # Saw
    "ouie_algoParam2": 0.5,
    "ouie_wtPos2": 0.5,
    "ouie_fmRatio2": 3.0,
    "ouie_fmIndex2": 1.5,
    "ouie_pw2": 0.5,
    # Interaction (the HAMMER)
    "ouie_macroHammer": 0.0,   # -1 (STRIFE) to +1 (LOVE)
    # Voice Mix
    "ouie_voiceMix": 0.5,      # 0=V1 only, 0.5=equal, 1=V2 only
    # Filter 1
    "ouie_cutoff1": 8000.0,    # 20..20000
    "ouie_reso1": 0.0,         # 0..1
    "ouie_filterMode1": 0,     # 0=LP, 1=HP, 2=BP, 3=Notch
    # Filter 2
    "ouie_cutoff2": 8000.0,
    "ouie_reso2": 0.0,
    "ouie_filterMode2": 0,
    "ouie_filterLink": 0,      # 0=Independent, 1=Linked
    # Amp Envelope 1
    "ouie_ampA1": 0.01,        # 0..10
    "ouie_ampD1": 0.1,         # 0..10
    "ouie_ampS1": 0.8,         # 0..1
    "ouie_ampR1": 0.3,         # 0..20
    # Amp Envelope 2
    "ouie_ampA2": 0.01,
    "ouie_ampD2": 0.1,
    "ouie_ampS2": 0.8,
    "ouie_ampR2": 0.3,
    # Mod Envelope (shared)
    "ouie_modA": 0.01,
    "ouie_modD": 0.3,
    "ouie_modS": 0.5,
    "ouie_modR": 0.5,
    "ouie_modDepth": 0.3,      # 0..1
    # LFO 1 (Voice 1)
    "ouie_lfo1Rate": 0.5,      # 0.01..30
    "ouie_lfo1Depth": 0.3,     # 0..1
    "ouie_lfo1Shape": 0,       # 0=Sine, 1=Tri, 2=Saw, 3=Sq, 4=S&H
    # LFO 2 (Voice 2)
    "ouie_lfo2Rate": 2.0,
    "ouie_lfo2Depth": 0.0,
    "ouie_lfo2Shape": 0,
    # Breathing LFO (D005)
    "ouie_breathRate": 0.05,   # 0.005..2
    "ouie_breathDepth": 0.1,   # 0..1
    # Unison
    "ouie_unisonCount": 0,     # Choice index: 0=1, 1=2, 2=3, 3=4
    "ouie_unisonDetune": 0.1,  # 0..1
    # Voice Mode
    "ouie_voiceMode": 1,       # 0=Split, 1=Layer, 2=Duo
    "ouie_splitNote": 60.0,    # 24..96
    "ouie_glide": 0.0,         # 0..5
    # Level
    "ouie_level": 0.8,         # 0..1
    # Macros
    "ouie_macroAmpullae": 0.5,   # Sensitivity (vel/AT/expression depth)
    "ouie_macroCartilage": 0.5,  # Flexibility (filter reso + env speed)
    "ouie_macroCurrent": 0.0,    # Environment (chorus depth + stereo spread)
}

# Algorithm constants
VA, WAVETABLE, FM, ADDITIVE, PHASE_DIST, WAVEFOLDER, KS, NOISE = range(8)
# VA waveform constants
SAW, SQUARE, TRIANGLE = range(3)
# Filter mode constants
LP, HP, BP, NOTCH = range(4)
# LFO shape constants
SINE, TRI, SAW_LFO, SQ_LFO, SANDH = range(5)
# Voice mode constants
SPLIT, LAYER, DUO = range(3)


def make_preset(name, mood, overrides, dna, desc="", tags=None, engines=None,
                coupling=None, coupling_intensity="None"):
    """Build a full .xometa dict from name + param overrides."""
    params = dict(DEFAULTS)
    params.update(overrides)

    preset = {
        "schema_version": 1,
        "name": name,
        "mood": mood,
        "engines": engines or ["Ouie"],
        "author": "XO_OX Designs",
        "version": "1.0.0",
        "description": desc,
        "tags": tags or [],
        "macroLabels": ["HAMMER", "AMPULLAE", "CARTILAGE", "CURRENT"],
        "couplingIntensity": coupling_intensity,
        "tempo": None,
        "dna": {
            "brightness": dna[0], "warmth": dna[1], "movement": dna[2],
            "density": dna[3], "space": dna[4], "aggression": dna[5]
        },
        "parameters": {"Ouie": params},
    }
    if coupling:
        preset["coupling"] = {"pairs": coupling}
    return preset


def save(preset):
    """Write preset to the correct mood directory."""
    mood = preset["mood"]
    safe_name = preset["name"].replace(" ", "_").replace("'", "").replace(",", "")
    filename = f"Ouie_{safe_name}.xometa"
    path = os.path.join(PRESET_DIR, mood, filename)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w") as f:
        json.dump(preset, f, indent=2)
    return path


# ══════════════════════════════════════════════════════════════════════════════
# FOUNDATION (30) — Algorithm showcases + building blocks
# ══════════════════════════════════════════════════════════════════════════════

FOUNDATION = [
    # --- Algorithm Showcases (8 presets, one per algorithm) ---
    ("Left Eye Saw", {
        "ouie_algo1": VA, "ouie_waveform1": SAW,
        "ouie_algo2": VA, "ouie_waveform2": SAW,
        "ouie_macroHammer": 0.0,
        "ouie_cutoff1": 5000.0, "ouie_reso1": 0.2,
        "ouie_cutoff2": 5000.0, "ouie_reso2": 0.2,
        "ouie_modDepth": 0.4,
    }, (0.6, 0.5, 0.15, 0.2, 0.0, 0.1),
     "Two VA saws in Layer mode. The foundation. No interaction, pure voices.",
     ["foundation", "va", "saw", "init"]),

    ("Metallic Tables", {
        "ouie_algo1": WAVETABLE, "ouie_wtPos1": 0.7,
        "ouie_algo2": WAVETABLE, "ouie_wtPos2": 0.3,
        "ouie_cutoff1": 6000.0, "ouie_reso1": 0.15,
        "ouie_cutoff2": 4000.0, "ouie_reso2": 0.2,
        "ouie_modDepth": 0.3,
        "ouie_lfo1Depth": 0.2, "ouie_lfo1Rate": 0.3,
    }, (0.5, 0.5, 0.3, 0.35, 0.0, 0.1),
     "Two wavetable voices at different positions. Metallic-to-organic morph.",
     ["foundation", "wavetable", "metallic"]),

    ("Dual FM", {
        "ouie_algo1": FM, "ouie_fmRatio1": 2.0, "ouie_fmIndex1": 1.5,
        "ouie_algo2": FM, "ouie_fmRatio2": 3.0, "ouie_fmIndex2": 0.8,
        "ouie_cutoff1": 6000.0, "ouie_cutoff2": 5000.0,
        "ouie_modDepth": 0.4,
    }, (0.55, 0.4, 0.2, 0.3, 0.0, 0.15),
     "Two FM voices with different ratios. Harmonic bell-like tones.",
     ["foundation", "fm", "harmonic", "bell"]),

    ("Additive Glow", {
        "ouie_algo1": ADDITIVE, "ouie_algoParam1": 0.7,
        "ouie_algo2": ADDITIVE, "ouie_algoParam2": 0.3,
        "ouie_cutoff1": 10000.0, "ouie_cutoff2": 6000.0,
        "ouie_modDepth": 0.3,
    }, (0.5, 0.6, 0.15, 0.25, 0.0, 0.0),
     "Two additive voices: one bright, one dark. Organ-like harmonic layers.",
     ["foundation", "additive", "organ", "harmonic"]),

    ("Phase Warp", {
        "ouie_algo1": PHASE_DIST, "ouie_algoParam1": 0.6,
        "ouie_algo2": PHASE_DIST, "ouie_algoParam2": 0.3,
        "ouie_cutoff1": 5000.0, "ouie_reso1": 0.3,
        "ouie_cutoff2": 3000.0, "ouie_reso2": 0.25,
        "ouie_modDepth": 0.5,
    }, (0.5, 0.5, 0.2, 0.3, 0.0, 0.2),
     "CZ-style phase distortion on both voices. Resonant digital character.",
     ["foundation", "phase-dist", "cz", "digital"]),

    ("Fold Territory", {
        "ouie_algo1": WAVEFOLDER, "ouie_algoParam1": 0.5,
        "ouie_algo2": WAVEFOLDER, "ouie_algoParam2": 0.2,
        "ouie_cutoff1": 4000.0, "ouie_reso1": 0.2,
        "ouie_cutoff2": 6000.0, "ouie_reso2": 0.15,
        "ouie_modDepth": 0.4,
    }, (0.55, 0.4, 0.2, 0.4, 0.0, 0.35),
     "Buchla-style wavefolder on both voices. Rich harmonic overtones.",
     ["foundation", "wavefolder", "buchla", "harmonics"]),

    ("Plucked Steel", {
        "ouie_algo1": KS, "ouie_algoParam1": 0.6,
        "ouie_algo2": KS, "ouie_algoParam2": 0.4,
        "ouie_ampA1": 0.001, "ouie_ampD1": 0.8, "ouie_ampS1": 0.0,
        "ouie_ampA2": 0.001, "ouie_ampD2": 1.2, "ouie_ampS2": 0.0,
        "ouie_cutoff1": 8000.0, "ouie_cutoff2": 6000.0,
    }, (0.55, 0.45, 0.1, 0.25, 0.05, 0.1),
     "Karplus-Strong plucked strings on both voices. Steel and nylon together.",
     ["foundation", "ks", "plucked", "string"]),

    ("Noise Breath", {
        "ouie_algo1": NOISE, "ouie_algoParam1": 0.6,
        "ouie_algo2": NOISE, "ouie_algoParam2": 0.2,
        "ouie_cutoff1": 4000.0, "ouie_reso1": 0.4,
        "ouie_cutoff2": 1000.0, "ouie_reso2": 0.3,
        "ouie_ampA1": 0.1, "ouie_ampR1": 0.8,
        "ouie_ampA2": 0.2, "ouie_ampR2": 1.5,
        "ouie_modDepth": 0.5,
    }, (0.4, 0.35, 0.3, 0.4, 0.1, 0.15),
     "Pitch-tracking filtered noise. High and low bands breathe together.",
     ["foundation", "noise", "breath", "texture"]),

    # --- HAMMER Showcases (4 presets) ---
    ("Gentle Strife", {
        "ouie_algo1": VA, "ouie_waveform1": SAW,
        "ouie_algo2": VA, "ouie_waveform2": SQUARE,
        "ouie_macroHammer": -0.3,
        "ouie_cutoff1": 4000.0, "ouie_reso1": 0.2,
        "ouie_cutoff2": 3500.0, "ouie_reso2": 0.25,
        "ouie_modDepth": 0.4,
    }, (0.5, 0.45, 0.2, 0.35, 0.0, 0.3),
     "Mild STRIFE: cross-FM between saw and square. Subtle harmonic tension.",
     ["foundation", "strife", "cross-fm", "tension"]),

    ("Deep Strife", {
        "ouie_algo1": VA, "ouie_waveform1": SAW,
        "ouie_algo2": FM, "ouie_fmRatio2": 4.0, "ouie_fmIndex2": 1.0,
        "ouie_macroHammer": -0.8,
        "ouie_cutoff1": 3000.0, "ouie_reso1": 0.3,
        "ouie_cutoff2": 2500.0, "ouie_reso2": 0.35,
        "ouie_modDepth": 0.5,
    }, (0.45, 0.35, 0.25, 0.5, 0.0, 0.6),
     "Deep STRIFE: ring mod + hard sync artifacts. Aggressive metallic clash.",
     ["foundation", "strife", "ring-mod", "aggressive"]),

    ("Warm Love", {
        "ouie_algo1": VA, "ouie_waveform1": SAW,
        "ouie_algo2": WAVETABLE, "ouie_wtPos2": 0.4,
        "ouie_macroHammer": 0.5,
        "ouie_cutoff1": 3000.0, "ouie_cutoff2": 3000.0,
        "ouie_filterLink": 1,
        "ouie_modDepth": 0.3,
        "ouie_macroCurrent": 0.2,
    }, (0.4, 0.65, 0.2, 0.3, 0.15, 0.0),
     "Moderate LOVE: spectral blend merges saw and wavetable into warmth.",
     ["foundation", "love", "blend", "warm"]),

    ("Total Love", {
        "ouie_algo1": VA, "ouie_waveform1": TRIANGLE,
        "ouie_algo2": ADDITIVE, "ouie_algoParam2": 0.5,
        "ouie_macroHammer": 1.0,
        "ouie_cutoff1": 6000.0, "ouie_cutoff2": 6000.0,
        "ouie_filterLink": 1,
        "ouie_modDepth": 0.2,
        "ouie_macroCurrent": 0.3,
    }, (0.45, 0.6, 0.15, 0.25, 0.2, 0.0),
     "Full LOVE: voices merge into unison. Triangle and additive become one.",
     ["foundation", "love", "unison", "merge"]),

    # --- Voice Mode Showcases (3 presets) ---
    ("Split Keys", {
        "ouie_voiceMode": SPLIT, "ouie_splitNote": 60.0,
        "ouie_algo1": VA, "ouie_waveform1": SAW,
        "ouie_algo2": FM, "ouie_fmRatio2": 2.0, "ouie_fmIndex2": 1.0,
        "ouie_cutoff1": 3000.0, "ouie_reso1": 0.3,
        "ouie_cutoff2": 5000.0, "ouie_reso2": 0.2,
        "ouie_ampA1": 0.01, "ouie_ampA2": 0.001,
        "ouie_ampD2": 0.5, "ouie_ampS2": 0.0,
        "ouie_modDepth": 0.4,
    }, (0.5, 0.5, 0.15, 0.3, 0.0, 0.1),
     "Split at C4: warm saw bass below, FM bells above. Two instruments in one.",
     ["foundation", "split", "bass-and-keys"]),

    ("Duo Counterpoint", {
        "ouie_voiceMode": DUO,
        "ouie_algo1": VA, "ouie_waveform1": SQUARE, "ouie_pw1": 0.35,
        "ouie_algo2": VA, "ouie_waveform2": SAW,
        "ouie_macroHammer": 0.2,
        "ouie_cutoff1": 3500.0, "ouie_reso1": 0.25,
        "ouie_cutoff2": 4000.0, "ouie_reso2": 0.2,
        "ouie_modDepth": 0.4,
        "ouie_glide": 0.08,
    }, (0.5, 0.5, 0.25, 0.3, 0.0, 0.1),
     "Duo mode: newest note on Voice 1, previous on Voice 2. Play melodies.",
     ["foundation", "duo", "counterpoint", "melody"]),

    ("Layer Unison", {
        "ouie_voiceMode": LAYER,
        "ouie_algo1": VA, "ouie_waveform1": SAW,
        "ouie_algo2": VA, "ouie_waveform2": SAW,
        "ouie_macroHammer": 0.6,
        "ouie_unisonCount": 2, "ouie_unisonDetune": 0.3,
        "ouie_cutoff1": 4000.0, "ouie_cutoff2": 4000.0,
        "ouie_filterLink": 1,
        "ouie_modDepth": 0.3,
    }, (0.55, 0.55, 0.2, 0.5, 0.1, 0.1),
     "Layer mode + unison detune + LOVE. Fat detuned saw ensemble.",
     ["foundation", "layer", "unison", "fat"]),

    # --- Cross-Algorithm Contrasts (7 presets) ---
    ("Smooth vs Rough", {
        "ouie_algo1": VA, "ouie_waveform1": SAW,
        "ouie_algo2": WAVEFOLDER, "ouie_algoParam2": 0.5,
        "ouie_macroHammer": -0.15,
        "ouie_cutoff1": 4000.0, "ouie_reso1": 0.2,
        "ouie_cutoff2": 3000.0, "ouie_reso2": 0.25,
        "ouie_modDepth": 0.4,
    }, (0.5, 0.45, 0.2, 0.35, 0.0, 0.25),
     "feliX saw vs Oscar wavefolder. Slight STRIFE adds cross-modulation edge.",
     ["foundation", "contrast", "smooth-rough"]),

    ("Digital Analog", {
        "ouie_algo1": PHASE_DIST, "ouie_algoParam1": 0.5,
        "ouie_algo2": VA, "ouie_waveform2": TRIANGLE,
        "ouie_macroHammer": 0.3,
        "ouie_cutoff1": 5000.0, "ouie_cutoff2": 6000.0,
        "ouie_modDepth": 0.35,
    }, (0.5, 0.55, 0.2, 0.3, 0.0, 0.1),
     "CZ phase distortion meets analog triangle. LOVE blends eras.",
     ["foundation", "contrast", "digital-analog"]),

    ("String and Breath", {
        "ouie_algo1": KS, "ouie_algoParam1": 0.65,
        "ouie_algo2": NOISE, "ouie_algoParam2": 0.4,
        "ouie_ampA1": 0.001, "ouie_ampD1": 1.0, "ouie_ampS1": 0.0,
        "ouie_ampA2": 0.05, "ouie_ampR2": 1.0,
        "ouie_cutoff1": 7000.0, "ouie_cutoff2": 2000.0, "ouie_reso2": 0.3,
        "ouie_macroHammer": 0.1,
    }, (0.5, 0.45, 0.15, 0.3, 0.05, 0.05),
     "Plucked string with breathy noise. Physical meets atmospheric.",
     ["foundation", "ks", "noise", "organic"]),

    ("FM Bell Pad", {
        "ouie_algo1": FM, "ouie_fmRatio1": 3.5, "ouie_fmIndex1": 2.0,
        "ouie_algo2": ADDITIVE, "ouie_algoParam2": 0.6,
        "ouie_ampA1": 0.001, "ouie_ampD1": 1.5, "ouie_ampS1": 0.1,
        "ouie_ampA2": 0.5, "ouie_ampS2": 0.7, "ouie_ampR2": 2.0,
        "ouie_macroHammer": 0.2,
        "ouie_cutoff1": 8000.0, "ouie_cutoff2": 4000.0,
    }, (0.5, 0.5, 0.2, 0.35, 0.1, 0.1),
     "FM bells over additive pad sustain. Two layers, one voice.",
     ["foundation", "fm", "additive", "bell-pad"]),

    ("Table and Fold", {
        "ouie_algo1": WAVETABLE, "ouie_wtPos1": 0.5,
        "ouie_algo2": WAVEFOLDER, "ouie_algoParam2": 0.4,
        "ouie_macroHammer": -0.2,
        "ouie_cutoff1": 5000.0, "ouie_cutoff2": 4000.0,
        "ouie_modDepth": 0.4,
        "ouie_lfo1Depth": 0.2, "ouie_lfo1Rate": 0.4,
    }, (0.5, 0.45, 0.3, 0.4, 0.0, 0.2),
     "Wavetable morphing with wavefolder texture. STRIFE adds grit.",
     ["foundation", "wavetable", "wavefolder", "texture"]),

    ("Phase and String", {
        "ouie_algo1": PHASE_DIST, "ouie_algoParam1": 0.45,
        "ouie_algo2": KS, "ouie_algoParam2": 0.7,
        "ouie_ampA2": 0.001, "ouie_ampD2": 1.5, "ouie_ampS2": 0.0,
        "ouie_cutoff1": 4000.0, "ouie_reso1": 0.25,
        "ouie_cutoff2": 8000.0,
        "ouie_macroHammer": 0.15,
        "ouie_voiceMode": DUO,
    }, (0.5, 0.5, 0.15, 0.3, 0.0, 0.1),
     "CZ resonance sustains while KS plucks. Duo mode for alternation.",
     ["foundation", "phase-dist", "ks", "duo"]),

    ("Noise and FM", {
        "ouie_algo1": NOISE, "ouie_algoParam1": 0.5,
        "ouie_algo2": FM, "ouie_fmRatio2": 5.0, "ouie_fmIndex2": 2.0,
        "ouie_cutoff1": 3000.0, "ouie_reso1": 0.4,
        "ouie_cutoff2": 6000.0,
        "ouie_macroHammer": -0.4,
        "ouie_ampA1": 0.05, "ouie_ampR1": 0.5,
        "ouie_modDepth": 0.5,
    }, (0.5, 0.35, 0.25, 0.45, 0.0, 0.35),
     "Filtered noise and metallic FM under STRIFE. Industrial percussion.",
     ["foundation", "noise", "fm", "industrial"]),

    # --- Basics (8 workhorse presets) ---
    ("Init Hammerhead", {
        "ouie_macroHammer": 0.0,
    }, (0.5, 0.5, 0.1, 0.2, 0.0, 0.1),
     "Default OUIE state. The thermocline at rest.",
     ["init", "default", "starting-point"]),

    ("Saw Bass", {
        "ouie_algo1": VA, "ouie_waveform1": SAW,
        "ouie_algo2": VA, "ouie_waveform2": SAW,
        "ouie_voiceMode": LAYER,
        "ouie_cutoff1": 1200.0, "ouie_reso1": 0.3,
        "ouie_cutoff2": 1000.0, "ouie_reso2": 0.25,
        "ouie_filterLink": 1,
        "ouie_modDepth": 0.5,
        "ouie_ampS1": 0.7, "ouie_ampS2": 0.7,
        "ouie_macroHammer": 0.3,
    }, (0.3, 0.6, 0.15, 0.3, 0.0, 0.2),
     "Dual saw bass with LOVE blend. Warm and solid.",
     ["bass", "saw", "warm", "basic"]),

    ("Clean Lead", {
        "ouie_algo1": VA, "ouie_waveform1": SQUARE, "ouie_pw1": 0.4,
        "ouie_algo2": VA, "ouie_waveform2": SAW,
        "ouie_voiceMode": DUO,
        "ouie_cutoff1": 5000.0, "ouie_reso1": 0.2,
        "ouie_cutoff2": 4000.0, "ouie_reso2": 0.2,
        "ouie_macroHammer": 0.1,
        "ouie_glide": 0.05,
        "ouie_modDepth": 0.4,
    }, (0.55, 0.5, 0.2, 0.25, 0.0, 0.1),
     "Pulse + saw lead in Duo mode. Clean and playable.",
     ["lead", "clean", "duo", "basic"]),

    ("Soft Pad", {
        "ouie_algo1": ADDITIVE, "ouie_algoParam1": 0.4,
        "ouie_algo2": VA, "ouie_waveform2": TRIANGLE,
        "ouie_macroHammer": 0.7,
        "ouie_ampA1": 0.8, "ouie_ampR1": 2.0,
        "ouie_ampA2": 1.0, "ouie_ampR2": 2.5,
        "ouie_cutoff1": 3000.0, "ouie_cutoff2": 3000.0,
        "ouie_filterLink": 1,
        "ouie_macroCurrent": 0.2,
    }, (0.35, 0.65, 0.2, 0.2, 0.2, 0.0),
     "Additive and triangle merged by LOVE. Gentle evolving pad.",
     ["pad", "soft", "love", "basic"]),

    ("Pluck Duo", {
        "ouie_algo1": VA, "ouie_waveform1": SAW,
        "ouie_algo2": KS, "ouie_algoParam2": 0.6,
        "ouie_voiceMode": DUO,
        "ouie_ampA1": 0.001, "ouie_ampD1": 0.4, "ouie_ampS1": 0.0,
        "ouie_ampA2": 0.001, "ouie_ampD2": 0.8, "ouie_ampS2": 0.0,
        "ouie_cutoff1": 6000.0, "ouie_cutoff2": 8000.0,
        "ouie_modDepth": 0.5,
    }, (0.55, 0.45, 0.1, 0.25, 0.0, 0.1),
     "Duo plucks: saw and KS alternate. Two pluck characters interleave.",
     ["pluck", "duo", "alternating", "basic"]),

    ("Hammer Axis", {
        "ouie_algo1": VA, "ouie_waveform1": SAW,
        "ouie_algo2": PHASE_DIST, "ouie_algoParam2": 0.5,
        "ouie_macroHammer": 0.0,
        "ouie_cutoff1": 3000.0, "ouie_reso1": 0.25,
        "ouie_cutoff2": 2500.0, "ouie_reso2": 0.3,
        "ouie_modDepth": 0.4,
    }, (0.5, 0.5, 0.2, 0.3, 0.0, 0.15),
     "HAMMER at zero. Move it to hear STRIFE and LOVE. The axis of the cephalofoil.",
     ["foundation", "hammer", "axis", "tutorial"]),

    ("Filter Modes", {
        "ouie_algo1": VA, "ouie_waveform1": SAW,
        "ouie_algo2": VA, "ouie_waveform2": SAW,
        "ouie_filterMode1": LP, "ouie_cutoff1": 2000.0, "ouie_reso1": 0.4,
        "ouie_filterMode2": HP, "ouie_cutoff2": 3000.0, "ouie_reso2": 0.3,
        "ouie_macroHammer": 0.0,
        "ouie_modDepth": 0.5,
    }, (0.5, 0.45, 0.2, 0.35, 0.0, 0.1),
     "Voice 1 through LP, Voice 2 through HP. Independent filters carve spectrum.",
     ["foundation", "filter", "split", "spectrum"]),

    ("PWM Duo", {
        "ouie_voiceMode": DUO,
        "ouie_algo1": VA, "ouie_waveform1": SQUARE, "ouie_pw1": 0.3,
        "ouie_algo2": VA, "ouie_waveform2": SQUARE, "ouie_pw2": 0.7,
        "ouie_macroHammer": 0.2,
        "ouie_cutoff1": 4000.0, "ouie_reso1": 0.2,
        "ouie_cutoff2": 3500.0, "ouie_reso2": 0.2,
        "ouie_modDepth": 0.3,
        "ouie_lfo1Depth": 0.2, "ouie_lfo1Rate": 0.4,
    }, (0.45, 0.55, 0.35, 0.25, 0.0, 0.05),
     "Two pulse widths in Duo mode with LFO. Classic analog chorus motion.",
     ["foundation", "pwm", "pulse", "duo"]),
]

# ══════════════════════════════════════════════════════════════════════════════
# ATMOSPHERE (25) — Pads, drones, environments
# ══════════════════════════════════════════════════════════════════════════════

ATMOSPHERE = [
    ("Thermocline Drift", {
        "ouie_algo1": VA, "ouie_waveform1": SAW,
        "ouie_algo2": WAVETABLE, "ouie_wtPos2": 0.4,
        "ouie_macroHammer": 0.4,
        "ouie_ampA1": 1.5, "ouie_ampR1": 3.0,
        "ouie_ampA2": 2.0, "ouie_ampR2": 4.0,
        "ouie_cutoff1": 2000.0, "ouie_cutoff2": 1800.0,
        "ouie_filterLink": 1,
        "ouie_breathRate": 0.02, "ouie_breathDepth": 0.3,
        "ouie_macroCurrent": 0.3,
        "ouie_lfo1Depth": 0.15, "ouie_lfo1Rate": 0.08,
    }, (0.3, 0.65, 0.4, 0.3, 0.3, 0.0),
     "Where warm water meets cold. LOVE blends two voices into gentle drift.",
     ["atmosphere", "thermocline", "drift", "pad"]),

    ("Cephalofoil Haze", {
        "ouie_algo1": ADDITIVE, "ouie_algoParam1": 0.5,
        "ouie_algo2": ADDITIVE, "ouie_algoParam2": 0.3,
        "ouie_macroHammer": 0.6,
        "ouie_ampA1": 2.0, "ouie_ampR1": 3.5,
        "ouie_ampA2": 2.5, "ouie_ampR2": 4.0,
        "ouie_cutoff1": 2500.0, "ouie_cutoff2": 2000.0,
        "ouie_breathRate": 0.015, "ouie_breathDepth": 0.25,
        "ouie_macroCurrent": 0.35,
    }, (0.35, 0.6, 0.35, 0.25, 0.35, 0.0),
     "Additive harmonics merged by LOVE. The wide-set eyes see as one.",
     ["atmosphere", "additive", "merged", "haze"]),

    ("Abyssal Murmur", {
        "ouie_algo1": VA, "ouie_waveform1": SAW,
        "ouie_algo2": NOISE, "ouie_algoParam2": 0.2,
        "ouie_macroHammer": 0.2,
        "ouie_voiceMix": 0.3,
        "ouie_ampA1": 1.0, "ouie_ampR1": 3.0,
        "ouie_ampA2": 1.5, "ouie_ampR2": 4.0,
        "ouie_cutoff1": 800.0, "ouie_reso1": 0.15,
        "ouie_cutoff2": 500.0, "ouie_reso2": 0.2,
        "ouie_breathRate": 0.01, "ouie_breathDepth": 0.35,
    }, (0.15, 0.7, 0.35, 0.3, 0.2, 0.05),
     "Deep saw drone with sub-noise. The bottom of the water column.",
     ["atmosphere", "deep", "drone", "dark"]),

    ("Lorenzini Pulse", {
        "ouie_algo1": VA, "ouie_waveform1": SQUARE, "ouie_pw1": 0.3,
        "ouie_algo2": PHASE_DIST, "ouie_algoParam2": 0.4,
        "ouie_macroHammer": -0.15,
        "ouie_ampA1": 0.8, "ouie_ampR1": 2.0,
        "ouie_ampA2": 1.2, "ouie_ampR2": 2.5,
        "ouie_cutoff1": 1500.0, "ouie_reso1": 0.25,
        "ouie_cutoff2": 2000.0, "ouie_reso2": 0.3,
        "ouie_lfo1Depth": 0.25, "ouie_lfo1Rate": 0.15,
        "ouie_macroCurrent": 0.25,
    }, (0.35, 0.55, 0.4, 0.35, 0.25, 0.1),
     "Electromagnetic sensing pad. Slow pulse width shifts detect movement.",
     ["atmosphere", "pulse", "sensing", "slow"]),

    ("Pelagic Choir", {
        "ouie_algo1": WAVETABLE, "ouie_wtPos1": 0.6,
        "ouie_algo2": WAVETABLE, "ouie_wtPos2": 0.8,
        "ouie_macroHammer": 0.8,
        "ouie_ampA1": 1.5, "ouie_ampR1": 3.0,
        "ouie_ampA2": 2.0, "ouie_ampR2": 3.5,
        "ouie_cutoff1": 3000.0, "ouie_cutoff2": 3500.0,
        "ouie_filterLink": 1,
        "ouie_lfo1Depth": 0.2, "ouie_lfo1Rate": 0.1,
        "ouie_breathRate": 0.02, "ouie_breathDepth": 0.2,
        "ouie_macroCurrent": 0.4,
    }, (0.4, 0.6, 0.4, 0.3, 0.4, 0.0),
     "Two wavetables deep in LOVE territory. A choir from the open ocean.",
     ["atmosphere", "wavetable", "choir", "love"]),

    ("Hammerhead Glide", {
        "ouie_algo1": VA, "ouie_waveform1": SAW,
        "ouie_algo2": VA, "ouie_waveform2": TRIANGLE,
        "ouie_voiceMode": DUO,
        "ouie_glide": 0.3,
        "ouie_macroHammer": 0.4,
        "ouie_ampA1": 0.5, "ouie_ampR1": 2.0,
        "ouie_ampA2": 0.8, "ouie_ampR2": 3.0,
        "ouie_cutoff1": 2000.0, "ouie_cutoff2": 2500.0,
        "ouie_macroCurrent": 0.3,
    }, (0.35, 0.6, 0.35, 0.25, 0.3, 0.0),
     "Duo mode with long glide. Notes slide like a cruising hammerhead.",
     ["atmosphere", "glide", "duo", "cruising"]),

    ("Electroreception", {
        "ouie_algo1": FM, "ouie_fmRatio1": 1.5, "ouie_fmIndex1": 0.5,
        "ouie_algo2": NOISE, "ouie_algoParam2": 0.5,
        "ouie_macroHammer": -0.1,
        "ouie_voiceMix": 0.35,
        "ouie_ampA1": 1.0, "ouie_ampR1": 2.5,
        "ouie_ampA2": 0.8, "ouie_ampR2": 2.0,
        "ouie_cutoff1": 3000.0, "ouie_cutoff2": 2500.0, "ouie_reso2": 0.35,
        "ouie_lfo1Depth": 0.15, "ouie_lfo1Rate": 0.2,
        "ouie_breathRate": 0.03, "ouie_breathDepth": 0.2,
    }, (0.4, 0.5, 0.35, 0.35, 0.15, 0.1),
     "FM tones over filtered noise field. The ampullae sense electrical signals.",
     ["atmosphere", "fm", "noise", "sensing"]),

    ("Ocean at Night", {
        "ouie_algo1": VA, "ouie_waveform1": SAW,
        "ouie_algo2": WAVETABLE, "ouie_wtPos2": 0.3,
        "ouie_macroHammer": 0.5,
        "ouie_ampA1": 2.0, "ouie_ampR1": 4.0, "ouie_ampS1": 0.7,
        "ouie_ampA2": 2.5, "ouie_ampR2": 5.0, "ouie_ampS2": 0.6,
        "ouie_cutoff1": 1200.0, "ouie_cutoff2": 1000.0,
        "ouie_filterLink": 1,
        "ouie_breathRate": 0.008, "ouie_breathDepth": 0.4,
        "ouie_macroCurrent": 0.35,
    }, (0.2, 0.7, 0.4, 0.25, 0.35, 0.0),
     "Ultra-slow breathing, dark warmth. The vast ocean after sunset.",
     ["atmosphere", "dark", "vast", "night"]),

    ("Warm Shallows", {
        "ouie_algo1": ADDITIVE, "ouie_algoParam1": 0.6,
        "ouie_algo2": VA, "ouie_waveform2": TRIANGLE,
        "ouie_macroHammer": 0.7,
        "ouie_ampA1": 0.5, "ouie_ampR1": 2.0,
        "ouie_ampA2": 0.6, "ouie_ampR2": 2.5,
        "ouie_cutoff1": 5000.0, "ouie_cutoff2": 4000.0,
        "ouie_lfo1Depth": 0.1, "ouie_lfo1Rate": 0.3,
        "ouie_macroCurrent": 0.25,
    }, (0.5, 0.65, 0.3, 0.25, 0.25, 0.0),
     "Bright additive harmonics with triangle warmth. Sunlit reef pad.",
     ["atmosphere", "bright", "warm", "sunlit"]),

    ("Hunting Grounds", {
        "ouie_algo1": VA, "ouie_waveform1": SAW,
        "ouie_algo2": PHASE_DIST, "ouie_algoParam2": 0.6,
        "ouie_macroHammer": -0.25,
        "ouie_ampA1": 0.8, "ouie_ampR1": 2.0,
        "ouie_ampA2": 1.0, "ouie_ampR2": 2.5,
        "ouie_cutoff1": 2000.0, "ouie_reso1": 0.3,
        "ouie_cutoff2": 1500.0, "ouie_reso2": 0.35,
        "ouie_modDepth": 0.4,
        "ouie_breathRate": 0.02, "ouie_breathDepth": 0.25,
    }, (0.35, 0.45, 0.35, 0.4, 0.15, 0.2),
     "Mild STRIFE tension builds between saw and phase distortion. Prowling.",
     ["atmosphere", "tension", "hunting", "prowling"]),

    ("Migration Path", {
        "ouie_algo1": VA, "ouie_waveform1": SAW,
        "ouie_algo2": VA, "ouie_waveform2": SAW,
        "ouie_macroHammer": 0.3,
        "ouie_unisonCount": 1, "ouie_unisonDetune": 0.25,
        "ouie_ampA1": 1.5, "ouie_ampR1": 3.0,
        "ouie_ampA2": 1.8, "ouie_ampR2": 3.5,
        "ouie_cutoff1": 2500.0, "ouie_cutoff2": 2200.0,
        "ouie_filterLink": 1,
        "ouie_breathRate": 0.015, "ouie_breathDepth": 0.3,
        "ouie_macroCurrent": 0.4,
    }, (0.35, 0.6, 0.45, 0.35, 0.4, 0.0),
     "Detuned saws in gentle LOVE, slowly drifting. Long ocean migration.",
     ["atmosphere", "unison", "migration", "drift"]),

    ("Cold Current", {
        "ouie_algo1": WAVETABLE, "ouie_wtPos1": 0.9,
        "ouie_algo2": NOISE, "ouie_algoParam2": 0.3,
        "ouie_macroHammer": -0.1,
        "ouie_voiceMix": 0.3,
        "ouie_ampA1": 1.0, "ouie_ampR1": 3.0,
        "ouie_ampA2": 1.5, "ouie_ampR2": 3.5,
        "ouie_cutoff1": 1500.0, "ouie_reso1": 0.2,
        "ouie_cutoff2": 800.0, "ouie_reso2": 0.15,
        "ouie_breathRate": 0.012, "ouie_breathDepth": 0.35,
    }, (0.3, 0.4, 0.35, 0.3, 0.2, 0.05),
     "High metallic wavetable over cold noise floor. Arctic current.",
     ["atmosphere", "cold", "metallic", "arctic"]),

    ("Schooling Swarm", {
        "ouie_algo1": FM, "ouie_fmRatio1": 3.0, "ouie_fmIndex1": 0.8,
        "ouie_algo2": FM, "ouie_fmRatio2": 5.0, "ouie_fmIndex2": 0.5,
        "ouie_macroHammer": 0.3,
        "ouie_ampA1": 0.3, "ouie_ampR1": 1.5,
        "ouie_ampA2": 0.5, "ouie_ampR2": 2.0,
        "ouie_cutoff1": 5000.0, "ouie_cutoff2": 4000.0,
        "ouie_lfo1Depth": 0.2, "ouie_lfo1Rate": 0.5,
        "ouie_lfo2Depth": 0.15, "ouie_lfo2Rate": 0.7,
        "ouie_macroCurrent": 0.3,
    }, (0.5, 0.45, 0.5, 0.4, 0.3, 0.05),
     "Two FM voices with different LFO rates. A school of fish moving together.",
     ["atmosphere", "fm", "school", "movement"]),

    ("Reef Lullaby", {
        "ouie_algo1": VA, "ouie_waveform1": TRIANGLE,
        "ouie_algo2": ADDITIVE, "ouie_algoParam2": 0.35,
        "ouie_macroHammer": 0.9,
        "ouie_ampA1": 2.0, "ouie_ampR1": 4.0,
        "ouie_ampA2": 2.5, "ouie_ampR2": 5.0,
        "ouie_cutoff1": 2000.0, "ouie_cutoff2": 1800.0,
        "ouie_filterLink": 1,
        "ouie_breathRate": 0.01, "ouie_breathDepth": 0.3,
        "ouie_macroCurrent": 0.3,
        "ouie_level": 0.65,
    }, (0.25, 0.7, 0.3, 0.2, 0.3, 0.0),
     "Deep LOVE merges triangle and additive into stillness. A lullaby.",
     ["atmosphere", "love", "gentle", "lullaby"]),

    ("Kelp Forest", {
        "ouie_algo1": WAVEFOLDER, "ouie_algoParam1": 0.25,
        "ouie_algo2": VA, "ouie_waveform2": TRIANGLE,
        "ouie_macroHammer": 0.2,
        "ouie_ampA1": 1.2, "ouie_ampR1": 2.5,
        "ouie_ampA2": 1.5, "ouie_ampR2": 3.0,
        "ouie_cutoff1": 2500.0, "ouie_cutoff2": 3000.0,
        "ouie_lfo1Depth": 0.2, "ouie_lfo1Rate": 0.12,
        "ouie_breathRate": 0.02, "ouie_breathDepth": 0.25,
        "ouie_macroCurrent": 0.25,
    }, (0.4, 0.55, 0.4, 0.3, 0.25, 0.05),
     "Gentle wavefolder overtones sway with LFO. Light through kelp fronds.",
     ["atmosphere", "wavefolder", "organic", "kelp"]),

    ("Tidal Pull", {
        "ouie_algo1": VA, "ouie_waveform1": SAW,
        "ouie_algo2": WAVETABLE, "ouie_wtPos2": 0.5,
        "ouie_macroHammer": 0.0,
        "ouie_ampA1": 1.0, "ouie_ampR1": 2.0,
        "ouie_ampA2": 1.2, "ouie_ampR2": 2.5,
        "ouie_cutoff1": 2000.0, "ouie_reso1": 0.2,
        "ouie_cutoff2": 2500.0, "ouie_reso2": 0.25,
        "ouie_lfo1Depth": 0.3, "ouie_lfo1Rate": 0.06,
        "ouie_lfo2Depth": 0.25, "ouie_lfo2Rate": 0.04,
        "ouie_breathRate": 0.008, "ouie_breathDepth": 0.3,
    }, (0.35, 0.55, 0.5, 0.3, 0.15, 0.0),
     "Ultra-slow LFOs on both voices. The tide pulls in and out.",
     ["atmosphere", "tidal", "slow", "pull"]),

    ("Magnetic Field", {
        "ouie_algo1": FM, "ouie_fmRatio1": 1.0, "ouie_fmIndex1": 0.3,
        "ouie_algo2": PHASE_DIST, "ouie_algoParam2": 0.35,
        "ouie_macroHammer": 0.15,
        "ouie_ampA1": 1.5, "ouie_ampR1": 3.0,
        "ouie_ampA2": 1.8, "ouie_ampR2": 3.5,
        "ouie_cutoff1": 3000.0, "ouie_cutoff2": 2500.0,
        "ouie_breathRate": 0.018, "ouie_breathDepth": 0.2,
        "ouie_macroCurrent": 0.2,
    }, (0.4, 0.5, 0.3, 0.3, 0.2, 0.05),
     "Subtle FM meets phase distortion. Invisible forces shape the sound.",
     ["atmosphere", "fm", "phase-dist", "subtle"]),

    ("Open Water", {
        "ouie_algo1": VA, "ouie_waveform1": SAW,
        "ouie_algo2": VA, "ouie_waveform2": SAW,
        "ouie_macroHammer": 0.5,
        "ouie_unisonCount": 2, "ouie_unisonDetune": 0.15,
        "ouie_ampA1": 2.0, "ouie_ampR1": 4.0, "ouie_ampS1": 0.7,
        "ouie_ampA2": 2.5, "ouie_ampR2": 5.0, "ouie_ampS2": 0.6,
        "ouie_cutoff1": 1500.0, "ouie_cutoff2": 1300.0,
        "ouie_filterLink": 1,
        "ouie_breathRate": 0.006, "ouie_breathDepth": 0.4,
        "ouie_macroCurrent": 0.5,
    }, (0.25, 0.65, 0.45, 0.35, 0.5, 0.0),
     "Vast detuned saws in LOVE. Maximum CURRENT for stereo width. Infinite ocean.",
     ["atmosphere", "vast", "open", "ocean"]),

    ("Cartilage Flex", {
        "ouie_algo1": WAVETABLE, "ouie_wtPos1": 0.4,
        "ouie_algo2": WAVEFOLDER, "ouie_algoParam2": 0.3,
        "ouie_macroHammer": 0.2,
        "ouie_ampA1": 1.0, "ouie_ampR1": 2.5,
        "ouie_ampA2": 1.2, "ouie_ampR2": 3.0,
        "ouie_cutoff1": 3000.0, "ouie_reso1": 0.3,
        "ouie_cutoff2": 2500.0, "ouie_reso2": 0.25,
        "ouie_macroCartilage": 0.7,
        "ouie_breathRate": 0.025, "ouie_breathDepth": 0.2,
    }, (0.4, 0.5, 0.35, 0.35, 0.15, 0.1),
     "High CARTILAGE: resonance boosted, envelopes quickened. Flexible skeleton.",
     ["atmosphere", "cartilage", "resonance", "flexible"]),

    ("Ampullae Sensitive", {
        "ouie_algo1": VA, "ouie_waveform1": SAW,
        "ouie_algo2": FM, "ouie_fmRatio2": 2.5, "ouie_fmIndex2": 0.8,
        "ouie_macroHammer": 0.1,
        "ouie_ampA1": 0.5, "ouie_ampR1": 2.0,
        "ouie_ampA2": 0.7, "ouie_ampR2": 2.5,
        "ouie_cutoff1": 2500.0, "ouie_cutoff2": 3000.0,
        "ouie_macroAmpullae": 0.9,
        "ouie_modDepth": 0.5,
    }, (0.45, 0.55, 0.3, 0.3, 0.1, 0.1),
     "Maximum AMPULLAE: velocity and aftertouch shape every note dramatically.",
     ["atmosphere", "ampullae", "expressive", "dynamic"]),

    ("Midnight Surface", {
        "ouie_algo1": NOISE, "ouie_algoParam1": 0.7,
        "ouie_algo2": VA, "ouie_waveform2": TRIANGLE,
        "ouie_macroHammer": 0.3,
        "ouie_voiceMix": 0.6,
        "ouie_ampA1": 1.0, "ouie_ampR1": 3.0,
        "ouie_ampA2": 1.5, "ouie_ampR2": 4.0,
        "ouie_cutoff1": 3000.0, "ouie_reso1": 0.2,
        "ouie_cutoff2": 1500.0,
        "ouie_breathRate": 0.01, "ouie_breathDepth": 0.3,
        "ouie_macroCurrent": 0.3,
    }, (0.35, 0.5, 0.35, 0.3, 0.3, 0.0),
     "High noise shimmer over deep triangle pad. The ocean surface at midnight.",
     ["atmosphere", "noise", "surface", "night"]),

    ("Still Water", {
        "ouie_algo1": VA, "ouie_waveform1": TRIANGLE,
        "ouie_algo2": VA, "ouie_waveform2": TRIANGLE,
        "ouie_macroHammer": 1.0,
        "ouie_ampA1": 3.0, "ouie_ampR1": 5.0, "ouie_ampS1": 0.9,
        "ouie_ampA2": 3.5, "ouie_ampR2": 6.0, "ouie_ampS2": 0.85,
        "ouie_cutoff1": 1500.0, "ouie_cutoff2": 1500.0,
        "ouie_filterLink": 1,
        "ouie_breathRate": 0.005, "ouie_breathDepth": 0.2,
        "ouie_level": 0.65,
    }, (0.2, 0.7, 0.2, 0.15, 0.1, 0.0),
     "Full LOVE: two triangles become one. The stillest, purest pad.",
     ["atmosphere", "still", "minimal", "pure"]),

    ("Bioluminescent", {
        "ouie_algo1": FM, "ouie_fmRatio1": 5.0, "ouie_fmIndex1": 0.6,
        "ouie_algo2": ADDITIVE, "ouie_algoParam2": 0.7,
        "ouie_macroHammer": 0.4,
        "ouie_ampA1": 0.8, "ouie_ampR1": 2.5,
        "ouie_ampA2": 1.0, "ouie_ampR2": 3.0,
        "ouie_cutoff1": 4000.0, "ouie_cutoff2": 3500.0,
        "ouie_lfo1Depth": 0.15, "ouie_lfo1Rate": 0.2,
        "ouie_lfo2Depth": 0.1, "ouie_lfo2Rate": 0.35,
        "ouie_macroCurrent": 0.3,
    }, (0.5, 0.5, 0.4, 0.3, 0.3, 0.0),
     "FM shimmer and additive glow in LOVE. Deep sea light flickering.",
     ["atmosphere", "bioluminescent", "shimmer", "glow"]),

    ("Sonar Ping", {
        "ouie_algo1": FM, "ouie_fmRatio1": 2.0, "ouie_fmIndex1": 1.5,
        "ouie_algo2": KS, "ouie_algoParam2": 0.5,
        "ouie_macroHammer": 0.1,
        "ouie_ampA1": 0.001, "ouie_ampD1": 2.0, "ouie_ampS1": 0.0,
        "ouie_ampA2": 0.001, "ouie_ampD2": 1.5, "ouie_ampS2": 0.0,
        "ouie_cutoff1": 6000.0, "ouie_cutoff2": 5000.0,
        "ouie_macroCurrent": 0.4,
    }, (0.5, 0.45, 0.1, 0.25, 0.4, 0.0),
     "FM bell and KS pluck create sonar-like pings in stereo space.",
     ["atmosphere", "sonar", "ping", "bell"]),

    ("Coral Bed", {
        "ouie_algo1": WAVETABLE, "ouie_wtPos1": 0.3,
        "ouie_algo2": NOISE, "ouie_algoParam2": 0.4,
        "ouie_macroHammer": 0.2,
        "ouie_voiceMix": 0.35,
        "ouie_ampA1": 1.5, "ouie_ampR1": 3.0,
        "ouie_ampA2": 1.0, "ouie_ampR2": 2.0,
        "ouie_cutoff1": 2000.0, "ouie_cutoff2": 1500.0, "ouie_reso2": 0.2,
        "ouie_breathRate": 0.015, "ouie_breathDepth": 0.3,
        "ouie_macroCurrent": 0.25,
    }, (0.35, 0.6, 0.35, 0.3, 0.25, 0.0),
     "Wavetable tones over gentle noise bed. Life on the coral reef floor.",
     ["atmosphere", "coral", "organic", "gentle"]),
]

# ══════════════════════════════════════════════════════════════════════════════
# ENTANGLED (20) — Coupling-ready duophonic setups
# ══════════════════════════════════════════════════════════════════════════════

ENTANGLED = [
    ("Hammer Sender", {
        "ouie_algo1": VA, "ouie_waveform1": SAW,
        "ouie_algo2": WAVEFOLDER, "ouie_algoParam2": 0.4,
        "ouie_macroHammer": -0.5,
        "ouie_cutoff1": 3000.0, "ouie_reso1": 0.3,
        "ouie_cutoff2": 2500.0, "ouie_reso2": 0.35,
        "ouie_modDepth": 0.5,
    }, (0.45, 0.45, 0.25, 0.4, 0.0, 0.35),
     "STRIFE cross-FM creates rich harmonics for coupling output.",
     ["entangled", "strife", "sender", "rich"]),

    ("Love Receiver", {
        "ouie_algo1": ADDITIVE, "ouie_algoParam1": 0.5,
        "ouie_algo2": VA, "ouie_waveform2": TRIANGLE,
        "ouie_macroHammer": 0.8,
        "ouie_ampA1": 0.5, "ouie_ampR1": 2.0,
        "ouie_ampA2": 0.8, "ouie_ampR2": 2.5,
        "ouie_cutoff1": 4000.0, "ouie_cutoff2": 3000.0,
        "ouie_filterLink": 1,
    }, (0.4, 0.6, 0.2, 0.25, 0.15, 0.0),
     "Deep LOVE pad. Coupling input shapes the merged voices.",
     ["entangled", "love", "receiver", "pad"]),

    ("Dual FM Feed", {
        "ouie_algo1": FM, "ouie_fmRatio1": 2.0, "ouie_fmIndex1": 2.0,
        "ouie_algo2": FM, "ouie_fmRatio2": 5.0, "ouie_fmIndex2": 1.0,
        "ouie_macroHammer": -0.3,
        "ouie_cutoff1": 5000.0, "ouie_cutoff2": 4000.0,
        "ouie_modDepth": 0.5,
    }, (0.55, 0.35, 0.25, 0.45, 0.0, 0.3),
     "Two FM engines under STRIFE. Dense harmonic coupling output.",
     ["entangled", "fm", "dense", "harmonic"]),

    ("Noise Coupling", {
        "ouie_algo1": NOISE, "ouie_algoParam1": 0.6,
        "ouie_algo2": NOISE, "ouie_algoParam2": 0.2,
        "ouie_cutoff1": 4000.0, "ouie_reso1": 0.5,
        "ouie_cutoff2": 800.0, "ouie_reso2": 0.4,
        "ouie_ampA1": 0.05, "ouie_ampR1": 0.5,
        "ouie_ampA2": 0.1, "ouie_ampR2": 1.0,
        "ouie_modDepth": 0.6,
    }, (0.4, 0.3, 0.3, 0.5, 0.05, 0.2),
     "Two noise bands: high and low. Coupling steers resonant peaks.",
     ["entangled", "noise", "resonant", "dual-band"]),

    ("KS Entangle", {
        "ouie_algo1": KS, "ouie_algoParam1": 0.7,
        "ouie_algo2": KS, "ouie_algoParam2": 0.3,
        "ouie_ampA1": 0.001, "ouie_ampD1": 1.5, "ouie_ampS1": 0.0,
        "ouie_ampA2": 0.001, "ouie_ampD2": 2.0, "ouie_ampS2": 0.0,
        "ouie_cutoff1": 8000.0, "ouie_cutoff2": 4000.0,
        "ouie_macroHammer": 0.2,
    }, (0.5, 0.45, 0.1, 0.3, 0.05, 0.05),
     "Two plucked strings. LOVE softens the attack. Coupling extends the tail.",
     ["entangled", "ks", "plucked", "string"]),

    ("Strife Bass", {
        "ouie_algo1": VA, "ouie_waveform1": SAW,
        "ouie_algo2": VA, "ouie_waveform2": SQUARE, "ouie_pw2": 0.3,
        "ouie_macroHammer": -0.7,
        "ouie_cutoff1": 1500.0, "ouie_reso1": 0.4,
        "ouie_cutoff2": 1200.0, "ouie_reso2": 0.35,
        "ouie_filterLink": 1,
        "ouie_ampS1": 0.7, "ouie_ampS2": 0.6,
        "ouie_modDepth": 0.6,
    }, (0.35, 0.5, 0.2, 0.45, 0.0, 0.5),
     "Deep STRIFE bass. Ring mod and cross-FM create sub harmonics.",
     ["entangled", "strife", "bass", "ring-mod"]),

    ("Split Entangle", {
        "ouie_voiceMode": SPLIT, "ouie_splitNote": 60.0,
        "ouie_algo1": VA, "ouie_waveform1": SAW,
        "ouie_algo2": FM, "ouie_fmRatio2": 3.0, "ouie_fmIndex2": 1.5,
        "ouie_cutoff1": 2000.0, "ouie_reso1": 0.3,
        "ouie_cutoff2": 5000.0,
        "ouie_modDepth": 0.4,
    }, (0.5, 0.45, 0.2, 0.35, 0.0, 0.15),
     "Split: bass saw and treble FM. Each half couples differently.",
     ["entangled", "split", "bass-treble", "contrast"]),

    ("Duo Thread", {
        "ouie_voiceMode": DUO,
        "ouie_algo1": WAVETABLE, "ouie_wtPos1": 0.5,
        "ouie_algo2": PHASE_DIST, "ouie_algoParam2": 0.5,
        "ouie_macroHammer": -0.2,
        "ouie_cutoff1": 3500.0, "ouie_cutoff2": 3000.0,
        "ouie_modDepth": 0.4,
        "ouie_glide": 0.1,
    }, (0.45, 0.5, 0.25, 0.35, 0.0, 0.2),
     "Duo mode threads notes between wavetable and phase distortion.",
     ["entangled", "duo", "thread", "alternating"]),

    ("Hammer Sweep", {
        "ouie_algo1": VA, "ouie_waveform1": SAW,
        "ouie_algo2": VA, "ouie_waveform2": SAW,
        "ouie_macroHammer": 0.0,
        "ouie_cutoff1": 3000.0, "ouie_reso1": 0.3,
        "ouie_cutoff2": 3000.0, "ouie_reso2": 0.3,
        "ouie_filterLink": 1,
        "ouie_modDepth": 0.5,
        "ouie_macroAmpullae": 0.8,
    }, (0.5, 0.5, 0.2, 0.3, 0.0, 0.1),
     "HAMMER at center. Sweep it from STRIFE to LOVE in real time.",
     ["entangled", "hammer", "sweep", "performance"]),

    ("Fold Entangle", {
        "ouie_algo1": WAVEFOLDER, "ouie_algoParam1": 0.6,
        "ouie_algo2": WAVEFOLDER, "ouie_algoParam2": 0.3,
        "ouie_macroHammer": -0.4,
        "ouie_cutoff1": 3000.0, "ouie_reso1": 0.2,
        "ouie_cutoff2": 4000.0, "ouie_reso2": 0.25,
        "ouie_modDepth": 0.5,
    }, (0.5, 0.4, 0.2, 0.5, 0.0, 0.4),
     "Dual wavefolders under STRIFE. Maximum harmonic density for coupling.",
     ["entangled", "wavefolder", "dense", "coupling"]),

    ("Unison Couple", {
        "ouie_algo1": VA, "ouie_waveform1": SAW,
        "ouie_algo2": VA, "ouie_waveform2": SAW,
        "ouie_macroHammer": 0.6,
        "ouie_unisonCount": 3, "ouie_unisonDetune": 0.4,
        "ouie_cutoff1": 4000.0, "ouie_cutoff2": 4000.0,
        "ouie_filterLink": 1,
    }, (0.55, 0.5, 0.25, 0.6, 0.1, 0.1),
     "Thick unison saws in LOVE. Dense coupling output signal.",
     ["entangled", "unison", "thick", "supersaw"]),

    ("Additive Weave", {
        "ouie_algo1": ADDITIVE, "ouie_algoParam1": 0.8,
        "ouie_algo2": ADDITIVE, "ouie_algoParam2": 0.4,
        "ouie_macroHammer": 0.3,
        "ouie_cutoff1": 6000.0, "ouie_cutoff2": 4000.0,
        "ouie_lfo1Depth": 0.2, "ouie_lfo1Rate": 0.3,
        "ouie_lfo2Depth": 0.15, "ouie_lfo2Rate": 0.5,
    }, (0.5, 0.5, 0.4, 0.35, 0.0, 0.0),
     "Two additive voices with different LFO rates weave coupling harmonics.",
     ["entangled", "additive", "weave", "harmonic"]),

    ("Phase Ring", {
        "ouie_algo1": PHASE_DIST, "ouie_algoParam1": 0.7,
        "ouie_algo2": PHASE_DIST, "ouie_algoParam2": 0.4,
        "ouie_macroHammer": -0.6,
        "ouie_cutoff1": 3000.0, "ouie_reso1": 0.35,
        "ouie_cutoff2": 2500.0, "ouie_reso2": 0.3,
        "ouie_modDepth": 0.5,
    }, (0.5, 0.4, 0.2, 0.45, 0.0, 0.4),
     "Dual phase distortion under deep STRIFE. Ring mod creates metallic resonance.",
     ["entangled", "phase-dist", "ring-mod", "metallic"]),

    ("Breath Entangle", {
        "ouie_algo1": VA, "ouie_waveform1": SAW,
        "ouie_algo2": WAVETABLE, "ouie_wtPos2": 0.6,
        "ouie_macroHammer": 0.2,
        "ouie_ampA1": 0.5, "ouie_ampR1": 2.0,
        "ouie_ampA2": 0.8, "ouie_ampR2": 2.5,
        "ouie_cutoff1": 2500.0, "ouie_cutoff2": 2000.0,
        "ouie_breathRate": 0.015, "ouie_breathDepth": 0.4,
    }, (0.35, 0.55, 0.4, 0.3, 0.1, 0.0),
     "Deep breathing modulation on both voices. Coupling rides the breath cycle.",
     ["entangled", "breath", "slow", "cycle"]),

    ("Macro Entangle", {
        "ouie_algo1": VA, "ouie_waveform1": SAW,
        "ouie_algo2": FM, "ouie_fmRatio2": 3.0, "ouie_fmIndex2": 1.0,
        "ouie_macroHammer": -0.2,
        "ouie_cutoff1": 3000.0, "ouie_reso1": 0.25,
        "ouie_cutoff2": 3500.0,
        "ouie_macroAmpullae": 0.7,
        "ouie_macroCartilage": 0.6,
        "ouie_macroCurrent": 0.3,
        "ouie_modDepth": 0.4,
    }, (0.5, 0.5, 0.35, 0.4, 0.3, 0.2),
     "All macros engaged. Full coupling sensitivity with STRIFE tension.",
     ["entangled", "macros", "all-three", "full"]),

    ("FM Feedback", {
        "ouie_algo1": FM, "ouie_fmRatio1": 1.0, "ouie_fmIndex1": 3.0,
        "ouie_algo2": FM, "ouie_fmRatio2": 1.0, "ouie_fmIndex2": 2.0,
        "ouie_macroHammer": -0.5,
        "ouie_cutoff1": 2000.0, "ouie_reso1": 0.3,
        "ouie_cutoff2": 2500.0, "ouie_reso2": 0.25,
        "ouie_modDepth": 0.6,
    }, (0.4, 0.4, 0.3, 0.5, 0.0, 0.45),
     "1:1 FM ratio on both voices under STRIFE. Feedback-like harmonic chaos.",
     ["entangled", "fm", "feedback", "chaos"]),

    ("Thermocline Gate", {
        "ouie_algo1": VA, "ouie_waveform1": SAW,
        "ouie_algo2": PHASE_DIST, "ouie_algoParam2": 0.5,
        "ouie_macroHammer": 0.0,
        "ouie_cutoff1": 2500.0, "ouie_reso1": 0.4,
        "ouie_cutoff2": 2000.0, "ouie_reso2": 0.35,
        "ouie_ampA1": 0.001, "ouie_ampD1": 0.3, "ouie_ampS1": 0.0,
        "ouie_ampA2": 0.001, "ouie_ampD2": 0.5, "ouie_ampS2": 0.0,
        "ouie_modDepth": 0.7,
    }, (0.5, 0.45, 0.15, 0.35, 0.0, 0.2),
     "Short stab at the thermocline. HAMMER at zero: choose STRIFE or LOVE live.",
     ["entangled", "stab", "gate", "thermocline"]),

    ("Wide Stereo Couple", {
        "ouie_algo1": VA, "ouie_waveform1": SAW,
        "ouie_algo2": WAVETABLE, "ouie_wtPos2": 0.7,
        "ouie_macroHammer": 0.3,
        "ouie_cutoff1": 3500.0, "ouie_cutoff2": 3000.0,
        "ouie_macroCurrent": 0.8,
        "ouie_modDepth": 0.3,
    }, (0.45, 0.5, 0.25, 0.35, 0.4, 0.05),
     "Maximum CURRENT: wide stereo spread. Coupling in panoramic field.",
     ["entangled", "stereo", "wide", "current"]),

    ("Glide Entangle", {
        "ouie_voiceMode": DUO,
        "ouie_algo1": VA, "ouie_waveform1": SAW,
        "ouie_algo2": VA, "ouie_waveform2": SQUARE, "ouie_pw2": 0.4,
        "ouie_macroHammer": -0.3,
        "ouie_glide": 0.15,
        "ouie_cutoff1": 3000.0, "ouie_reso1": 0.25,
        "ouie_cutoff2": 2500.0, "ouie_reso2": 0.3,
        "ouie_modDepth": 0.4,
    }, (0.45, 0.5, 0.35, 0.35, 0.0, 0.25),
     "Duo mode with glide under STRIFE. Sliding notes create coupling movement.",
     ["entangled", "duo", "glide", "sliding"]),

    ("Table Entangle", {
        "ouie_algo1": WAVETABLE, "ouie_wtPos1": 0.7,
        "ouie_algo2": WAVETABLE, "ouie_wtPos2": 0.2,
        "ouie_macroHammer": 0.4,
        "ouie_cutoff1": 3500.0, "ouie_cutoff2": 3000.0,
        "ouie_lfo1Depth": 0.2, "ouie_lfo1Rate": 0.2,
        "ouie_lfo2Depth": 0.25, "ouie_lfo2Rate": 0.15,
        "ouie_breathRate": 0.02, "ouie_breathDepth": 0.2,
    }, (0.45, 0.5, 0.4, 0.35, 0.1, 0.05),
     "Two wavetable positions with independent LFOs. LOVE weaves coupling harmonics.",
     ["entangled", "wavetable", "dual-lfo", "morph"]),
]

# ══════════════════════════════════════════════════════════════════════════════
# PRISM (25) — Genre production presets
# ══════════════════════════════════════════════════════════════════════════════

# --- TECHNO (8) ---
TECHNO = [
    ("Acid Hammer", {
        "ouie_algo1": VA, "ouie_waveform1": SAW,
        "ouie_algo2": VA, "ouie_waveform2": SQUARE, "ouie_pw2": 0.3,
        "ouie_macroHammer": -0.4,
        "ouie_cutoff1": 800.0, "ouie_reso1": 0.65,
        "ouie_cutoff2": 600.0, "ouie_reso2": 0.6,
        "ouie_filterLink": 1,
        "ouie_modDepth": 0.8,
        "ouie_ampS1": 0.5, "ouie_ampS2": 0.4,
        "ouie_glide": 0.06,
    }, (0.4, 0.5, 0.4, 0.35, 0.0, 0.55),
     "Squelchy acid with STRIFE cross-FM adding harmonic grit.",
     ["techno", "acid", "303", "strife"]),

    ("Industrial Stab", {
        "ouie_algo1": VA, "ouie_waveform1": SAW,
        "ouie_algo2": WAVEFOLDER, "ouie_algoParam2": 0.6,
        "ouie_macroHammer": -0.6,
        "ouie_ampA1": 0.001, "ouie_ampD1": 0.12, "ouie_ampS1": 0.0,
        "ouie_ampA2": 0.001, "ouie_ampD2": 0.15, "ouie_ampS2": 0.0,
        "ouie_cutoff1": 5000.0, "ouie_reso1": 0.3,
        "ouie_cutoff2": 4000.0, "ouie_reso2": 0.35,
        "ouie_modDepth": 0.6,
    }, (0.55, 0.35, 0.1, 0.45, 0.0, 0.6),
     "Short aggressive stab. STRIFE ring mod on wavefolder. Industrial.",
     ["techno", "stab", "industrial", "short"]),

    ("Warehouse Drone", {
        "ouie_algo1": VA, "ouie_waveform1": SAW,
        "ouie_algo2": PHASE_DIST, "ouie_algoParam2": 0.4,
        "ouie_macroHammer": -0.2,
        "ouie_ampA1": 0.5, "ouie_ampS1": 0.9,
        "ouie_ampA2": 0.8, "ouie_ampS2": 0.85,
        "ouie_cutoff1": 1200.0, "ouie_reso1": 0.4,
        "ouie_cutoff2": 1000.0, "ouie_reso2": 0.35,
        "ouie_lfo1Depth": 0.15, "ouie_lfo1Rate": 0.2,
        "ouie_breathRate": 0.03, "ouie_breathDepth": 0.2,
    }, (0.3, 0.5, 0.35, 0.4, 0.15, 0.25),
     "Low drone with subtle STRIFE. Phase distortion adds CZ resonance.",
     ["techno", "drone", "warehouse", "dark"]),

    ("Rave Duo", {
        "ouie_voiceMode": DUO,
        "ouie_algo1": VA, "ouie_waveform1": SAW,
        "ouie_algo2": VA, "ouie_waveform2": SAW,
        "ouie_macroHammer": -0.3,
        "ouie_unisonCount": 2, "ouie_unisonDetune": 0.35,
        "ouie_cutoff1": 6000.0, "ouie_reso1": 0.3,
        "ouie_cutoff2": 5000.0, "ouie_reso2": 0.25,
        "ouie_modDepth": 0.5,
        "ouie_glide": 0.04,
    }, (0.6, 0.45, 0.3, 0.5, 0.1, 0.4),
     "Detuned duo saws with STRIFE edge. Rave lead for big rooms.",
     ["techno", "rave", "lead", "detuned"]),

    ("Metallic Perc", {
        "ouie_algo1": FM, "ouie_fmRatio1": 5.0, "ouie_fmIndex1": 3.0,
        "ouie_algo2": NOISE, "ouie_algoParam2": 0.5,
        "ouie_macroHammer": -0.5,
        "ouie_ampA1": 0.001, "ouie_ampD1": 0.2, "ouie_ampS1": 0.0,
        "ouie_ampA2": 0.001, "ouie_ampD2": 0.1, "ouie_ampS2": 0.0,
        "ouie_voiceMix": 0.35,
        "ouie_cutoff1": 6000.0, "ouie_cutoff2": 3000.0, "ouie_reso2": 0.4,
    }, (0.5, 0.3, 0.1, 0.4, 0.0, 0.4),
     "Inharmonic FM with noise transient under STRIFE. Metallic percussive hit.",
     ["techno", "perc", "metallic", "fm"]),

    ("Dark Chord", {
        "ouie_algo1": VA, "ouie_waveform1": SAW,
        "ouie_algo2": VA, "ouie_waveform2": SAW,
        "ouie_macroHammer": 0.4,
        "ouie_ampA1": 0.1, "ouie_ampR1": 1.5,
        "ouie_ampA2": 0.15, "ouie_ampR2": 2.0,
        "ouie_cutoff1": 1800.0, "ouie_reso1": 0.25,
        "ouie_cutoff2": 1500.0, "ouie_reso2": 0.2,
        "ouie_filterLink": 1,
        "ouie_modDepth": 0.3,
        "ouie_macroCurrent": 0.2,
    }, (0.3, 0.55, 0.25, 0.35, 0.2, 0.15),
     "LOVE-blended saws for dark warehouse chords.",
     ["techno", "chord", "dark", "pad"]),

    ("Sub Hammer", {
        "ouie_algo1": VA, "ouie_waveform1": TRIANGLE,
        "ouie_algo2": VA, "ouie_waveform2": TRIANGLE,
        "ouie_macroHammer": 0.8,
        "ouie_cutoff1": 400.0, "ouie_cutoff2": 350.0,
        "ouie_filterLink": 1,
        "ouie_ampS1": 0.9, "ouie_ampS2": 0.9,
    }, (0.1, 0.75, 0.1, 0.15, 0.0, 0.1),
     "Pure sub bass. Full LOVE merges two triangles into one weighty tone.",
     ["techno", "sub", "bass", "love"]),

    ("Hypnotic Loop", {
        "ouie_algo1": VA, "ouie_waveform1": SQUARE, "ouie_pw1": 0.35,
        "ouie_algo2": PHASE_DIST, "ouie_algoParam2": 0.5,
        "ouie_macroHammer": -0.15,
        "ouie_ampA1": 0.001, "ouie_ampD1": 0.25, "ouie_ampS1": 0.0,
        "ouie_ampA2": 0.001, "ouie_ampD2": 0.3, "ouie_ampS2": 0.0,
        "ouie_cutoff1": 3000.0, "ouie_reso1": 0.35,
        "ouie_cutoff2": 2500.0, "ouie_reso2": 0.3,
        "ouie_modDepth": 0.6,
    }, (0.45, 0.45, 0.35, 0.3, 0.0, 0.3),
     "Pulse and CZ pluck under mild STRIFE. Feed it sequences.",
     ["techno", "pluck", "arp", "hypnotic"]),
]

# --- SYNTHWAVE (8) ---
SYNTHWAVE = [
    ("Retro Saw Lead", {
        "ouie_algo1": VA, "ouie_waveform1": SAW,
        "ouie_algo2": VA, "ouie_waveform2": SAW,
        "ouie_macroHammer": 0.5,
        "ouie_unisonCount": 1, "ouie_unisonDetune": 0.2,
        "ouie_cutoff1": 5000.0, "ouie_reso1": 0.2,
        "ouie_cutoff2": 4500.0, "ouie_reso2": 0.2,
        "ouie_filterLink": 1,
        "ouie_modDepth": 0.4,
        "ouie_glide": 0.04,
        "ouie_macroCurrent": 0.3,
    }, (0.6, 0.5, 0.3, 0.35, 0.3, 0.15),
     "LOVE-thickened saws with chorus. Neon nights, 1984.",
     ["synthwave", "lead", "retro", "80s"]),

    ("Analog Brass", {
        "ouie_algo1": VA, "ouie_waveform1": SAW,
        "ouie_algo2": VA, "ouie_waveform2": SQUARE, "ouie_pw2": 0.45,
        "ouie_macroHammer": 0.3,
        "ouie_ampA1": 0.08, "ouie_ampA2": 0.1,
        "ouie_cutoff1": 3000.0, "ouie_reso1": 0.25,
        "ouie_cutoff2": 2500.0, "ouie_reso2": 0.2,
        "ouie_modDepth": 0.5,
        "ouie_macroCurrent": 0.25,
    }, (0.5, 0.55, 0.25, 0.35, 0.25, 0.2),
     "Saw and square brass with LOVE blend. Slow attack, warm filter.",
     ["synthwave", "brass", "analog", "warm"]),

    ("Sunset Pad", {
        "ouie_algo1": VA, "ouie_waveform1": SAW,
        "ouie_algo2": ADDITIVE, "ouie_algoParam2": 0.5,
        "ouie_macroHammer": 0.7,
        "ouie_ampA1": 1.0, "ouie_ampR1": 2.5,
        "ouie_ampA2": 1.2, "ouie_ampR2": 3.0,
        "ouie_cutoff1": 2000.0, "ouie_cutoff2": 2500.0,
        "ouie_filterLink": 1,
        "ouie_lfo1Depth": 0.1, "ouie_lfo1Rate": 0.2,
        "ouie_macroCurrent": 0.4,
    }, (0.35, 0.65, 0.3, 0.3, 0.4, 0.0),
     "LOVE merges saw and additive into golden sunset pad.",
     ["synthwave", "pad", "sunset", "warm"]),

    ("Arp Pulse", {
        "ouie_algo1": VA, "ouie_waveform1": SQUARE, "ouie_pw1": 0.4,
        "ouie_algo2": VA, "ouie_waveform2": TRIANGLE,
        "ouie_macroHammer": 0.15,
        "ouie_ampA1": 0.001, "ouie_ampD1": 0.25, "ouie_ampS1": 0.1,
        "ouie_ampA2": 0.001, "ouie_ampD2": 0.3, "ouie_ampS2": 0.05,
        "ouie_cutoff1": 4000.0, "ouie_reso1": 0.3,
        "ouie_cutoff2": 3500.0,
        "ouie_modDepth": 0.5,
    }, (0.5, 0.5, 0.3, 0.25, 0.0, 0.1),
     "Pulse + triangle arp sound. Retro sequencer character.",
     ["synthwave", "arp", "pulse", "retro"]),

    ("Neon Bass", {
        "ouie_algo1": VA, "ouie_waveform1": SAW,
        "ouie_algo2": VA, "ouie_waveform2": SQUARE, "ouie_pw2": 0.5,
        "ouie_macroHammer": 0.2,
        "ouie_cutoff1": 1500.0, "ouie_reso1": 0.35,
        "ouie_cutoff2": 1200.0, "ouie_reso2": 0.3,
        "ouie_filterLink": 1,
        "ouie_modDepth": 0.6,
        "ouie_ampS1": 0.6, "ouie_ampS2": 0.5,
    }, (0.4, 0.55, 0.2, 0.3, 0.0, 0.25),
     "Saw + square bass with LOVE warmth and filter sweep.",
     ["synthwave", "bass", "neon", "warm"]),

    ("Cyber Strings", {
        "ouie_algo1": VA, "ouie_waveform1": SAW,
        "ouie_algo2": WAVETABLE, "ouie_wtPos2": 0.3,
        "ouie_macroHammer": 0.5,
        "ouie_unisonCount": 2, "ouie_unisonDetune": 0.2,
        "ouie_ampA1": 0.3, "ouie_ampR1": 1.5,
        "ouie_ampA2": 0.4, "ouie_ampR2": 2.0,
        "ouie_cutoff1": 3500.0, "ouie_cutoff2": 3000.0,
        "ouie_filterLink": 1,
        "ouie_macroCurrent": 0.35,
    }, (0.45, 0.55, 0.3, 0.4, 0.35, 0.1),
     "Detuned saw and wavetable strings. LOVE and CURRENT for width.",
     ["synthwave", "strings", "ensemble", "wide"]),

    ("Drive Keys", {
        "ouie_algo1": FM, "ouie_fmRatio1": 2.0, "ouie_fmIndex1": 0.8,
        "ouie_algo2": VA, "ouie_waveform2": TRIANGLE,
        "ouie_macroHammer": 0.2,
        "ouie_ampA1": 0.005, "ouie_ampD1": 0.6, "ouie_ampS1": 0.3,
        "ouie_ampA2": 0.01, "ouie_ampD2": 0.5, "ouie_ampS2": 0.4,
        "ouie_cutoff1": 5000.0, "ouie_cutoff2": 4000.0,
        "ouie_modDepth": 0.3,
    }, (0.5, 0.5, 0.15, 0.25, 0.0, 0.1),
     "FM electric piano with triangle body. Drive soundtrack keys.",
     ["synthwave", "keys", "electric-piano", "fm"]),

    ("Chrome Pluck", {
        "ouie_algo1": WAVETABLE, "ouie_wtPos1": 0.6,
        "ouie_algo2": KS, "ouie_algoParam2": 0.7,
        "ouie_macroHammer": 0.1,
        "ouie_ampA1": 0.001, "ouie_ampD1": 0.5, "ouie_ampS1": 0.0,
        "ouie_ampA2": 0.001, "ouie_ampD2": 0.8, "ouie_ampS2": 0.0,
        "ouie_cutoff1": 7000.0, "ouie_cutoff2": 8000.0,
        "ouie_modDepth": 0.4,
    }, (0.6, 0.4, 0.1, 0.3, 0.05, 0.1),
     "Metallic wavetable pluck with KS string body. Chrome finish.",
     ["synthwave", "pluck", "metallic", "chrome"]),
]

# --- CINEMATIC (5) ---
CINEMATIC = [
    ("Tension Rise", {
        "ouie_algo1": VA, "ouie_waveform1": SAW,
        "ouie_algo2": NOISE, "ouie_algoParam2": 0.4,
        "ouie_macroHammer": -0.3,
        "ouie_ampA1": 2.0, "ouie_ampR1": 3.0,
        "ouie_ampA2": 2.5, "ouie_ampR2": 4.0,
        "ouie_cutoff1": 1500.0, "ouie_reso1": 0.3,
        "ouie_cutoff2": 2000.0, "ouie_reso2": 0.25,
        "ouie_modDepth": 0.5,
        "ouie_lfo1Depth": 0.2, "ouie_lfo1Rate": 0.1,
    }, (0.35, 0.4, 0.4, 0.35, 0.2, 0.3),
     "STRIFE builds tension. Saw and noise rise together. Film score suspense.",
     ["cinematic", "tension", "riser", "suspense"]),

    ("Underwater Cathedral", {
        "ouie_algo1": ADDITIVE, "ouie_algoParam1": 0.5,
        "ouie_algo2": WAVETABLE, "ouie_wtPos2": 0.4,
        "ouie_macroHammer": 0.8,
        "ouie_ampA1": 2.0, "ouie_ampR1": 5.0, "ouie_ampS1": 0.8,
        "ouie_ampA2": 2.5, "ouie_ampR2": 6.0, "ouie_ampS2": 0.7,
        "ouie_cutoff1": 2000.0, "ouie_cutoff2": 1800.0,
        "ouie_filterLink": 1,
        "ouie_breathRate": 0.008, "ouie_breathDepth": 0.3,
        "ouie_macroCurrent": 0.5,
    }, (0.3, 0.65, 0.35, 0.3, 0.5, 0.0),
     "Deep LOVE merges additive and wavetable. Vast reverent space.",
     ["cinematic", "cathedral", "vast", "love"]),

    ("Chase Scene", {
        "ouie_voiceMode": DUO,
        "ouie_algo1": VA, "ouie_waveform1": SAW,
        "ouie_algo2": PHASE_DIST, "ouie_algoParam2": 0.5,
        "ouie_macroHammer": -0.4,
        "ouie_ampA1": 0.001, "ouie_ampD1": 0.2, "ouie_ampS1": 0.3,
        "ouie_ampA2": 0.001, "ouie_ampD2": 0.15, "ouie_ampS2": 0.2,
        "ouie_cutoff1": 4000.0, "ouie_reso1": 0.3,
        "ouie_cutoff2": 3500.0, "ouie_reso2": 0.35,
        "ouie_modDepth": 0.6,
        "ouie_glide": 0.03,
    }, (0.5, 0.4, 0.4, 0.4, 0.0, 0.45),
     "Duo mode with STRIFE: notes alternate with aggressive tension.",
     ["cinematic", "chase", "aggressive", "duo"]),

    ("Distant Horizon", {
        "ouie_algo1": VA, "ouie_waveform1": TRIANGLE,
        "ouie_algo2": FM, "ouie_fmRatio2": 1.5, "ouie_fmIndex2": 0.3,
        "ouie_macroHammer": 0.6,
        "ouie_ampA1": 3.0, "ouie_ampR1": 5.0, "ouie_ampS1": 0.8,
        "ouie_ampA2": 3.5, "ouie_ampR2": 6.0, "ouie_ampS2": 0.7,
        "ouie_cutoff1": 2000.0, "ouie_cutoff2": 2500.0,
        "ouie_filterLink": 1,
        "ouie_breathRate": 0.006, "ouie_breathDepth": 0.3,
        "ouie_macroCurrent": 0.4,
    }, (0.3, 0.6, 0.3, 0.2, 0.4, 0.0),
     "Ultra-slow LOVE pad. Triangle and FM shimmer on the horizon.",
     ["cinematic", "horizon", "vast", "peaceful"]),

    ("Impact Layer", {
        "ouie_algo1": FM, "ouie_fmRatio1": 0.5, "ouie_fmIndex1": 4.0,
        "ouie_algo2": WAVEFOLDER, "ouie_algoParam2": 0.7,
        "ouie_macroHammer": -0.8,
        "ouie_ampA1": 0.001, "ouie_ampD1": 0.8, "ouie_ampS1": 0.0,
        "ouie_ampA2": 0.001, "ouie_ampD2": 0.6, "ouie_ampS2": 0.0,
        "ouie_cutoff1": 4000.0, "ouie_cutoff2": 3000.0, "ouie_reso2": 0.3,
        "ouie_modDepth": 0.7,
    }, (0.45, 0.3, 0.1, 0.55, 0.0, 0.7),
     "Deep STRIFE impact: FM sub-harmonic + wavefolder crunch. Layer under hits.",
     ["cinematic", "impact", "sub", "aggressive"]),
]

# --- DnB (4) ---
DNB = [
    ("Reese Bass", {
        "ouie_algo1": VA, "ouie_waveform1": SAW,
        "ouie_algo2": VA, "ouie_waveform2": SAW,
        "ouie_macroHammer": -0.3,
        "ouie_unisonCount": 1, "ouie_unisonDetune": 0.15,
        "ouie_cutoff1": 1000.0, "ouie_reso1": 0.3,
        "ouie_cutoff2": 800.0, "ouie_reso2": 0.25,
        "ouie_filterLink": 1,
        "ouie_modDepth": 0.5,
        "ouie_lfo1Depth": 0.2, "ouie_lfo1Rate": 0.4,
    }, (0.3, 0.55, 0.35, 0.35, 0.0, 0.4),
     "Detuned saws with STRIFE wobble. Classic Reese bass.",
     ["dnb", "reese", "bass", "wobble"]),

    ("Neurofunk Stab", {
        "ouie_algo1": WAVEFOLDER, "ouie_algoParam1": 0.7,
        "ouie_algo2": PHASE_DIST, "ouie_algoParam2": 0.6,
        "ouie_macroHammer": -0.6,
        "ouie_ampA1": 0.001, "ouie_ampD1": 0.1, "ouie_ampS1": 0.0,
        "ouie_ampA2": 0.001, "ouie_ampD2": 0.08, "ouie_ampS2": 0.0,
        "ouie_cutoff1": 5000.0, "ouie_reso1": 0.4,
        "ouie_cutoff2": 4000.0, "ouie_reso2": 0.35,
        "ouie_modDepth": 0.7,
    }, (0.55, 0.3, 0.1, 0.5, 0.0, 0.65),
     "Wavefolder and phase dist under STRIFE. Short aggressive neurofunk hit.",
     ["dnb", "neuro", "stab", "aggressive"]),

    ("Amens Lead", {
        "ouie_voiceMode": DUO,
        "ouie_algo1": VA, "ouie_waveform1": SAW,
        "ouie_algo2": FM, "ouie_fmRatio2": 3.0, "ouie_fmIndex2": 1.5,
        "ouie_macroHammer": -0.2,
        "ouie_cutoff1": 4000.0, "ouie_reso1": 0.25,
        "ouie_cutoff2": 5000.0,
        "ouie_modDepth": 0.5,
        "ouie_glide": 0.03,
    }, (0.55, 0.4, 0.3, 0.35, 0.0, 0.3),
     "Duo mode: saw and FM alternate with STRIFE tension. For jungle leads.",
     ["dnb", "lead", "jungle", "duo"]),

    ("Liquid Pad", {
        "ouie_algo1": WAVETABLE, "ouie_wtPos1": 0.4,
        "ouie_algo2": ADDITIVE, "ouie_algoParam2": 0.5,
        "ouie_macroHammer": 0.6,
        "ouie_ampA1": 0.5, "ouie_ampR1": 2.0,
        "ouie_ampA2": 0.7, "ouie_ampR2": 2.5,
        "ouie_cutoff1": 3000.0, "ouie_cutoff2": 2500.0,
        "ouie_filterLink": 1,
        "ouie_macroCurrent": 0.3,
        "ouie_lfo1Depth": 0.1, "ouie_lfo1Rate": 0.3,
    }, (0.4, 0.6, 0.35, 0.3, 0.3, 0.0),
     "LOVE-blended wavetable and additive. Liquid DnB pad.",
     ["dnb", "liquid", "pad", "smooth"]),
]

# ══════════════════════════════════════════════════════════════════════════════
# FLUX (20) — EDM/Lo-Fi/Movement
# ══════════════════════════════════════════════════════════════════════════════

FLUX_PRESETS = [
    # --- EDM (10) ---
    ("Festival Lead", {
        "ouie_algo1": VA, "ouie_waveform1": SAW,
        "ouie_algo2": VA, "ouie_waveform2": SAW,
        "ouie_macroHammer": 0.5,
        "ouie_unisonCount": 3, "ouie_unisonDetune": 0.5,
        "ouie_cutoff1": 8000.0, "ouie_cutoff2": 7000.0,
        "ouie_filterLink": 1,
        "ouie_modDepth": 0.3,
        "ouie_macroCurrent": 0.4,
    }, (0.7, 0.45, 0.3, 0.7, 0.4, 0.2),
     "Maximum unison saws in LOVE. Festival main stage supersaw.",
     ["edm", "lead", "supersaw", "festival"]),

    ("Pluck Drop", {
        "ouie_algo1": VA, "ouie_waveform1": SAW,
        "ouie_algo2": FM, "ouie_fmRatio2": 2.0, "ouie_fmIndex2": 0.5,
        "ouie_macroHammer": 0.2,
        "ouie_ampA1": 0.001, "ouie_ampD1": 0.3, "ouie_ampS1": 0.0,
        "ouie_ampA2": 0.001, "ouie_ampD2": 0.2, "ouie_ampS2": 0.0,
        "ouie_cutoff1": 5000.0, "ouie_reso1": 0.3,
        "ouie_cutoff2": 4000.0,
        "ouie_modDepth": 0.6,
    }, (0.55, 0.45, 0.15, 0.3, 0.0, 0.15),
     "Saw + FM pluck for EDM drops. Sharp attack, bright filter sweep.",
     ["edm", "pluck", "drop", "bright"]),

    ("Wobble Bass", {
        "ouie_algo1": VA, "ouie_waveform1": SAW,
        "ouie_algo2": VA, "ouie_waveform2": SQUARE, "ouie_pw2": 0.3,
        "ouie_macroHammer": -0.4,
        "ouie_cutoff1": 1500.0, "ouie_reso1": 0.5,
        "ouie_cutoff2": 1200.0, "ouie_reso2": 0.45,
        "ouie_filterLink": 1,
        "ouie_lfo1Depth": 0.6, "ouie_lfo1Rate": 3.0,
        "ouie_modDepth": 0.4,
        "ouie_ampS1": 0.7, "ouie_ampS2": 0.6,
    }, (0.35, 0.45, 0.7, 0.4, 0.0, 0.5),
     "STRIFE bass with fast LFO wobble. Dubstep growl.",
     ["edm", "wobble", "bass", "dubstep"]),

    ("Siren Rise", {
        "ouie_algo1": VA, "ouie_waveform1": SAW,
        "ouie_algo2": VA, "ouie_waveform2": SAW,
        "ouie_macroHammer": 0.3,
        "ouie_unisonCount": 2, "ouie_unisonDetune": 0.3,
        "ouie_cutoff1": 6000.0, "ouie_cutoff2": 5000.0,
        "ouie_filterLink": 1,
        "ouie_lfo1Depth": 0.5, "ouie_lfo1Rate": 5.0,
        "ouie_modDepth": 0.4,
        "ouie_glide": 0.1,
    }, (0.6, 0.4, 0.7, 0.4, 0.1, 0.4),
     "Fast vibrato siren with unison. Build energy before the drop.",
     ["edm", "siren", "riser", "vibrato"]),

    ("Chord Stab", {
        "ouie_algo1": VA, "ouie_waveform1": SAW,
        "ouie_algo2": VA, "ouie_waveform2": SQUARE, "ouie_pw2": 0.5,
        "ouie_macroHammer": 0.4,
        "ouie_ampA1": 0.001, "ouie_ampD1": 0.5, "ouie_ampS1": 0.2,
        "ouie_ampA2": 0.001, "ouie_ampD2": 0.4, "ouie_ampS2": 0.15,
        "ouie_cutoff1": 4000.0, "ouie_reso1": 0.25,
        "ouie_cutoff2": 3500.0,
        "ouie_modDepth": 0.5,
        "ouie_macroCurrent": 0.25,
    }, (0.5, 0.5, 0.2, 0.35, 0.25, 0.2),
     "LOVE-blended chord stab with filter sweep. House classic.",
     ["edm", "stab", "chord", "house"]),

    ("Future Pad", {
        "ouie_algo1": WAVETABLE, "ouie_wtPos1": 0.5,
        "ouie_algo2": FM, "ouie_fmRatio2": 2.0, "ouie_fmIndex2": 0.6,
        "ouie_macroHammer": 0.5,
        "ouie_ampA1": 0.5, "ouie_ampR1": 2.0,
        "ouie_ampA2": 0.8, "ouie_ampR2": 2.5,
        "ouie_cutoff1": 3000.0, "ouie_cutoff2": 2500.0,
        "ouie_filterLink": 1,
        "ouie_lfo1Depth": 0.15, "ouie_lfo1Rate": 0.3,
        "ouie_macroCurrent": 0.4,
    }, (0.45, 0.55, 0.35, 0.35, 0.4, 0.05),
     "Wavetable and FM in LOVE. Lush future bass pad.",
     ["edm", "pad", "future-bass", "lush"]),

    ("Trap Pluck", {
        "ouie_algo1": VA, "ouie_waveform1": SAW,
        "ouie_algo2": VA, "ouie_waveform2": TRIANGLE,
        "ouie_macroHammer": 0.2,
        "ouie_ampA1": 0.001, "ouie_ampD1": 0.15, "ouie_ampS1": 0.0,
        "ouie_ampA2": 0.001, "ouie_ampD2": 0.1, "ouie_ampS2": 0.0,
        "ouie_cutoff1": 6000.0, "ouie_reso1": 0.2,
        "ouie_cutoff2": 5000.0,
        "ouie_modDepth": 0.6,
    }, (0.55, 0.45, 0.1, 0.25, 0.0, 0.15),
     "Short bright pluck for trap melodies. Clean and crisp.",
     ["edm", "pluck", "trap", "clean"]),

    ("Trance Gate", {
        "ouie_algo1": VA, "ouie_waveform1": SAW,
        "ouie_algo2": VA, "ouie_waveform2": SAW,
        "ouie_macroHammer": 0.4,
        "ouie_unisonCount": 1, "ouie_unisonDetune": 0.2,
        "ouie_cutoff1": 4000.0, "ouie_cutoff2": 3500.0,
        "ouie_filterLink": 1,
        "ouie_ampA1": 0.001, "ouie_ampD1": 0.15, "ouie_ampS1": 0.6,
        "ouie_ampA2": 0.001, "ouie_ampD2": 0.2, "ouie_ampS2": 0.5,
        "ouie_modDepth": 0.5,
        "ouie_macroCurrent": 0.3,
    }, (0.5, 0.5, 0.35, 0.4, 0.3, 0.15),
     "LOVE saws with gated envelope. Trance pads and gated effects.",
     ["edm", "trance", "gate", "saw"]),

    ("Bounce Lead", {
        "ouie_voiceMode": DUO,
        "ouie_algo1": VA, "ouie_waveform1": SQUARE, "ouie_pw1": 0.35,
        "ouie_algo2": VA, "ouie_waveform2": SAW,
        "ouie_macroHammer": -0.15,
        "ouie_cutoff1": 5000.0, "ouie_reso1": 0.25,
        "ouie_cutoff2": 4500.0,
        "ouie_modDepth": 0.5,
        "ouie_glide": 0.04,
    }, (0.55, 0.45, 0.3, 0.3, 0.0, 0.2),
     "Duo mode: pulse and saw bounce between notes. Bouncy lead.",
     ["edm", "lead", "bounce", "duo"]),

    ("Growl Layer", {
        "ouie_algo1": WAVEFOLDER, "ouie_algoParam1": 0.6,
        "ouie_algo2": PHASE_DIST, "ouie_algoParam2": 0.5,
        "ouie_macroHammer": -0.7,
        "ouie_cutoff1": 2000.0, "ouie_reso1": 0.5,
        "ouie_cutoff2": 1500.0, "ouie_reso2": 0.45,
        "ouie_filterLink": 1,
        "ouie_lfo1Depth": 0.4, "ouie_lfo1Rate": 4.0,
        "ouie_modDepth": 0.6,
        "ouie_ampS1": 0.8, "ouie_ampS2": 0.7,
    }, (0.4, 0.4, 0.6, 0.5, 0.0, 0.65),
     "Deep STRIFE with fast LFO. Wavefolder growl for bass drops.",
     ["edm", "growl", "bass", "strife"]),

    # --- Lo-Fi (10) ---
    ("Vinyl Keys", {
        "ouie_algo1": FM, "ouie_fmRatio1": 2.0, "ouie_fmIndex1": 0.5,
        "ouie_algo2": VA, "ouie_waveform2": TRIANGLE,
        "ouie_macroHammer": 0.3,
        "ouie_ampA1": 0.005, "ouie_ampD1": 0.5, "ouie_ampS1": 0.3,
        "ouie_ampA2": 0.01, "ouie_ampD2": 0.4, "ouie_ampS2": 0.4,
        "ouie_cutoff1": 3000.0, "ouie_cutoff2": 2500.0,
        "ouie_filterLink": 1,
        "ouie_breathRate": 0.04, "ouie_breathDepth": 0.15,
    }, (0.4, 0.6, 0.2, 0.25, 0.1, 0.0),
     "FM and triangle in LOVE. Warm lo-fi keys with vinyl character.",
     ["lofi", "keys", "warm", "vinyl"]),

    ("Tape Drift", {
        "ouie_algo1": VA, "ouie_waveform1": SAW,
        "ouie_algo2": VA, "ouie_waveform2": SAW,
        "ouie_macroHammer": 0.4,
        "ouie_ampA1": 0.3, "ouie_ampR1": 1.5,
        "ouie_ampA2": 0.4, "ouie_ampR2": 2.0,
        "ouie_cutoff1": 2000.0, "ouie_cutoff2": 1800.0,
        "ouie_filterLink": 1,
        "ouie_lfo1Depth": 0.1, "ouie_lfo1Rate": 0.15,
        "ouie_breathRate": 0.03, "ouie_breathDepth": 0.2,
        "ouie_level": 0.7,
    }, (0.3, 0.65, 0.3, 0.25, 0.1, 0.0),
     "Warm saws in LOVE with slow drift. Old tape machine character.",
     ["lofi", "tape", "drift", "warm"]),

    ("Dusty Bells", {
        "ouie_algo1": FM, "ouie_fmRatio1": 3.5, "ouie_fmIndex1": 1.2,
        "ouie_algo2": FM, "ouie_fmRatio2": 4.0, "ouie_fmIndex2": 0.8,
        "ouie_macroHammer": 0.2,
        "ouie_ampA1": 0.001, "ouie_ampD1": 1.0, "ouie_ampS1": 0.0,
        "ouie_ampA2": 0.001, "ouie_ampD2": 1.5, "ouie_ampS2": 0.0,
        "ouie_cutoff1": 3000.0, "ouie_cutoff2": 2500.0,
        "ouie_breathRate": 0.04, "ouie_breathDepth": 0.15,
        "ouie_level": 0.7,
    }, (0.45, 0.5, 0.15, 0.3, 0.05, 0.0),
     "Dual FM bells with gentle LOVE. Vintage electric piano feel.",
     ["lofi", "bells", "fm", "vintage"]),

    ("Haze Pad", {
        "ouie_algo1": VA, "ouie_waveform1": TRIANGLE,
        "ouie_algo2": ADDITIVE, "ouie_algoParam2": 0.3,
        "ouie_macroHammer": 0.7,
        "ouie_ampA1": 1.5, "ouie_ampR1": 2.5,
        "ouie_ampA2": 2.0, "ouie_ampR2": 3.0,
        "ouie_cutoff1": 1500.0, "ouie_cutoff2": 1200.0,
        "ouie_filterLink": 1,
        "ouie_breathRate": 0.02, "ouie_breathDepth": 0.25,
        "ouie_level": 0.65,
    }, (0.2, 0.7, 0.3, 0.2, 0.15, 0.0),
     "Deep LOVE: triangle and additive in warm haze. Lo-fi ambient.",
     ["lofi", "pad", "haze", "warm"]),

    ("Muted Strings", {
        "ouie_algo1": KS, "ouie_algoParam1": 0.4,
        "ouie_algo2": VA, "ouie_waveform2": TRIANGLE,
        "ouie_macroHammer": 0.3,
        "ouie_ampA1": 0.001, "ouie_ampD1": 0.6, "ouie_ampS1": 0.0,
        "ouie_ampA2": 0.1, "ouie_ampR2": 1.0,
        "ouie_cutoff1": 2000.0, "ouie_cutoff2": 1500.0,
        "ouie_level": 0.7,
    }, (0.3, 0.6, 0.1, 0.2, 0.05, 0.0),
     "Dull KS pluck with triangle warmth. Muted guitar feel.",
     ["lofi", "strings", "muted", "guitar"]),

    ("Cassette Lead", {
        "ouie_voiceMode": DUO,
        "ouie_algo1": VA, "ouie_waveform1": SQUARE, "ouie_pw1": 0.4,
        "ouie_algo2": VA, "ouie_waveform2": SAW,
        "ouie_macroHammer": 0.15,
        "ouie_cutoff1": 2500.0, "ouie_reso1": 0.2,
        "ouie_cutoff2": 2000.0,
        "ouie_modDepth": 0.3,
        "ouie_glide": 0.06,
        "ouie_level": 0.7,
    }, (0.35, 0.6, 0.2, 0.25, 0.0, 0.05),
     "Warm pulse + saw duo with cassette-era filter cutoff.",
     ["lofi", "lead", "cassette", "warm"]),

    ("Sleepy Chords", {
        "ouie_algo1": VA, "ouie_waveform1": SAW,
        "ouie_algo2": WAVETABLE, "ouie_wtPos2": 0.3,
        "ouie_macroHammer": 0.5,
        "ouie_ampA1": 0.2, "ouie_ampR1": 1.5,
        "ouie_ampA2": 0.3, "ouie_ampR2": 2.0,
        "ouie_cutoff1": 1800.0, "ouie_cutoff2": 1500.0,
        "ouie_filterLink": 1,
        "ouie_breathRate": 0.025, "ouie_breathDepth": 0.2,
        "ouie_level": 0.65,
    }, (0.3, 0.65, 0.25, 0.25, 0.1, 0.0),
     "LOVE-blended saw and wavetable. Gentle chords for late nights.",
     ["lofi", "chords", "gentle", "night"]),

    ("Broken FM", {
        "ouie_algo1": FM, "ouie_fmRatio1": 7.0, "ouie_fmIndex1": 0.4,
        "ouie_algo2": NOISE, "ouie_algoParam2": 0.3,
        "ouie_macroHammer": -0.1,
        "ouie_voiceMix": 0.4,
        "ouie_ampA1": 0.001, "ouie_ampD1": 0.8, "ouie_ampS1": 0.0,
        "ouie_ampA2": 0.001, "ouie_ampD2": 0.5, "ouie_ampS2": 0.0,
        "ouie_cutoff1": 2500.0, "ouie_cutoff2": 1500.0, "ouie_reso2": 0.3,
        "ouie_level": 0.7,
    }, (0.4, 0.4, 0.1, 0.3, 0.0, 0.1),
     "Detuned FM with noise dust. Broken electronics character.",
     ["lofi", "fm", "broken", "dusty"]),

    ("Warm Wobble", {
        "ouie_algo1": VA, "ouie_waveform1": SAW,
        "ouie_algo2": VA, "ouie_waveform2": TRIANGLE,
        "ouie_macroHammer": 0.3,
        "ouie_cutoff1": 1500.0, "ouie_reso1": 0.2,
        "ouie_cutoff2": 1200.0,
        "ouie_filterLink": 1,
        "ouie_lfo1Depth": 0.15, "ouie_lfo1Rate": 0.4,
        "ouie_breathRate": 0.03, "ouie_breathDepth": 0.15,
        "ouie_ampS1": 0.7, "ouie_ampS2": 0.6,
        "ouie_level": 0.7,
    }, (0.25, 0.65, 0.35, 0.25, 0.0, 0.0),
     "Gentle LFO wobble through warm filter. Lo-fi bass texture.",
     ["lofi", "wobble", "warm", "bass"]),

    ("Ambient Flicker", {
        "ouie_algo1": ADDITIVE, "ouie_algoParam1": 0.4,
        "ouie_algo2": NOISE, "ouie_algoParam2": 0.5,
        "ouie_macroHammer": 0.1,
        "ouie_voiceMix": 0.3,
        "ouie_ampA1": 1.0, "ouie_ampR1": 3.0,
        "ouie_ampA2": 0.5, "ouie_ampR2": 2.0,
        "ouie_cutoff1": 2500.0, "ouie_cutoff2": 3000.0, "ouie_reso2": 0.3,
        "ouie_lfo2Depth": 0.3, "ouie_lfo2Rate": 0.8, "ouie_lfo2Shape": SANDH,
        "ouie_level": 0.65,
    }, (0.35, 0.5, 0.4, 0.3, 0.1, 0.0),
     "Additive pad with S&H noise flicker. Lo-fi ambient texture.",
     ["lofi", "ambient", "flicker", "texture"]),
]

# ══════════════════════════════════════════════════════════════════════════════
# AETHER (15) — Experimental/deep STRIFE-LOVE explorations
# ══════════════════════════════════════════════════════════════════════════════

AETHER = [
    ("Full Strife", {
        "ouie_algo1": VA, "ouie_waveform1": SAW,
        "ouie_algo2": WAVEFOLDER, "ouie_algoParam2": 0.7,
        "ouie_macroHammer": -1.0,
        "ouie_cutoff1": 3000.0, "ouie_reso1": 0.35,
        "ouie_cutoff2": 2500.0, "ouie_reso2": 0.4,
        "ouie_modDepth": 0.6,
    }, (0.5, 0.3, 0.25, 0.55, 0.0, 0.8),
     "Maximum STRIFE: cross-FM + ring mod + hard sync. Total destruction.",
     ["aether", "strife", "extreme", "destruction"]),

    ("Strife to Love", {
        "ouie_algo1": VA, "ouie_waveform1": SAW,
        "ouie_algo2": ADDITIVE, "ouie_algoParam2": 0.5,
        "ouie_macroHammer": 0.0,
        "ouie_cutoff1": 3000.0, "ouie_cutoff2": 2500.0,
        "ouie_modDepth": 0.4,
        "ouie_macroAmpullae": 0.8,
        "ouie_lfo1Depth": 0.2, "ouie_lfo1Rate": 0.15,
    }, (0.5, 0.5, 0.3, 0.35, 0.0, 0.2),
     "Start at center. Sweep HAMMER from -1 to +1 for the full journey.",
     ["aether", "sweep", "journey", "performance"]),

    ("Electromagnetic", {
        "ouie_algo1": FM, "ouie_fmRatio1": 1.0, "ouie_fmIndex1": 5.0,
        "ouie_algo2": FM, "ouie_fmRatio2": 1.0, "ouie_fmIndex2": 4.0,
        "ouie_macroHammer": -0.8,
        "ouie_cutoff1": 2000.0, "ouie_reso1": 0.3,
        "ouie_cutoff2": 2500.0, "ouie_reso2": 0.25,
        "ouie_modDepth": 0.5,
        "ouie_breathRate": 0.01, "ouie_breathDepth": 0.3,
    }, (0.4, 0.3, 0.35, 0.6, 0.1, 0.55),
     "1:1 FM feedback under deep STRIFE. Electromagnetic field interference.",
     ["aether", "fm", "feedback", "electromagnetic"]),

    ("Harmonic Lock", {
        "ouie_algo1": ADDITIVE, "ouie_algoParam1": 0.8,
        "ouie_algo2": ADDITIVE, "ouie_algoParam2": 0.6,
        "ouie_macroHammer": 1.0,
        "ouie_cutoff1": 8000.0, "ouie_cutoff2": 6000.0,
        "ouie_breathRate": 0.008, "ouie_breathDepth": 0.2,
        "ouie_macroCurrent": 0.3,
    }, (0.55, 0.55, 0.25, 0.3, 0.3, 0.0),
     "Full LOVE: two additive voices merge into one harmonic series.",
     ["aether", "additive", "love", "harmonic"]),

    ("Phase Collision", {
        "ouie_algo1": PHASE_DIST, "ouie_algoParam1": 0.8,
        "ouie_algo2": PHASE_DIST, "ouie_algoParam2": 0.6,
        "ouie_macroHammer": -0.9,
        "ouie_cutoff1": 4000.0, "ouie_reso1": 0.4,
        "ouie_cutoff2": 3000.0, "ouie_reso2": 0.45,
        "ouie_modDepth": 0.7,
    }, (0.5, 0.3, 0.2, 0.55, 0.0, 0.6),
     "Deep STRIFE on dual phase distortion. CZ resonance meets ring mod.",
     ["aether", "phase-dist", "collision", "extreme"]),

    ("Fold Space", {
        "ouie_algo1": WAVEFOLDER, "ouie_algoParam1": 0.8,
        "ouie_algo2": WAVEFOLDER, "ouie_algoParam2": 0.5,
        "ouie_macroHammer": -0.7,
        "ouie_cutoff1": 2500.0, "ouie_reso1": 0.3,
        "ouie_cutoff2": 3000.0, "ouie_reso2": 0.35,
        "ouie_lfo1Depth": 0.3, "ouie_lfo1Rate": 0.1,
        "ouie_modDepth": 0.6,
    }, (0.45, 0.35, 0.35, 0.6, 0.1, 0.5),
     "Extreme wavefolder under STRIFE. Harmonics folded through ring mod.",
     ["aether", "wavefolder", "extreme", "harmonic"]),

    ("Ghost Note", {
        "ouie_voiceMode": DUO,
        "ouie_algo1": KS, "ouie_algoParam1": 0.5,
        "ouie_algo2": NOISE, "ouie_algoParam2": 0.3,
        "ouie_macroHammer": -0.2,
        "ouie_ampA1": 0.001, "ouie_ampD1": 2.0, "ouie_ampS1": 0.0,
        "ouie_ampA2": 0.001, "ouie_ampD2": 1.0, "ouie_ampS2": 0.0,
        "ouie_cutoff1": 4000.0, "ouie_cutoff2": 2000.0, "ouie_reso2": 0.3,
        "ouie_macroCurrent": 0.4,
    }, (0.4, 0.4, 0.15, 0.3, 0.4, 0.05),
     "Duo: one note plucks, the previous fades as noise ghost. Haunting.",
     ["aether", "ks", "ghost", "haunting"]),

    ("Split Universe", {
        "ouie_voiceMode": SPLIT, "ouie_splitNote": 60.0,
        "ouie_algo1": WAVEFOLDER, "ouie_algoParam1": 0.6,
        "ouie_algo2": ADDITIVE, "ouie_algoParam2": 0.7,
        "ouie_macroHammer": 0.0,
        "ouie_cutoff1": 2000.0, "ouie_reso1": 0.35,
        "ouie_cutoff2": 5000.0,
        "ouie_ampA1": 0.01, "ouie_ampA2": 0.5, "ouie_ampR2": 3.0,
        "ouie_modDepth": 0.5,
    }, (0.45, 0.45, 0.2, 0.4, 0.1, 0.25),
     "Split: rough wavefolder bass vs bright additive treble. Two universes.",
     ["aether", "split", "contrast", "experimental"]),

    ("Breath of Steel", {
        "ouie_algo1": VA, "ouie_waveform1": SAW,
        "ouie_algo2": WAVETABLE, "ouie_wtPos2": 0.8,
        "ouie_macroHammer": -0.3,
        "ouie_ampA1": 1.0, "ouie_ampR1": 3.0,
        "ouie_ampA2": 1.5, "ouie_ampR2": 4.0,
        "ouie_cutoff1": 2000.0, "ouie_reso1": 0.25,
        "ouie_cutoff2": 1500.0, "ouie_reso2": 0.2,
        "ouie_breathRate": 0.005, "ouie_breathDepth": 0.5,
    }, (0.35, 0.5, 0.45, 0.3, 0.15, 0.15),
     "Ultra-slow breathing modulation with STRIFE tension. The hammerhead inhales.",
     ["aether", "breath", "slow", "tension"]),

    ("Noise Cathedral", {
        "ouie_algo1": NOISE, "ouie_algoParam1": 0.7,
        "ouie_algo2": NOISE, "ouie_algoParam2": 0.2,
        "ouie_macroHammer": 0.5,
        "ouie_cutoff1": 5000.0, "ouie_reso1": 0.5,
        "ouie_cutoff2": 500.0, "ouie_reso2": 0.6,
        "ouie_ampA1": 1.0, "ouie_ampR1": 3.0,
        "ouie_ampA2": 1.5, "ouie_ampR2": 4.0,
        "ouie_filterMode2": BP,
        "ouie_macroCurrent": 0.5,
    }, (0.4, 0.35, 0.3, 0.5, 0.5, 0.1),
     "LOVE merges high hiss and low rumble. Noise becomes architecture.",
     ["aether", "noise", "cathedral", "love"]),

    ("Alien Tongue", {
        "ouie_algo1": FM, "ouie_fmRatio1": 7.0, "ouie_fmIndex1": 3.0,
        "ouie_algo2": PHASE_DIST, "ouie_algoParam2": 0.8,
        "ouie_macroHammer": -0.6,
        "ouie_cutoff1": 3000.0, "ouie_reso1": 0.3,
        "ouie_cutoff2": 2500.0, "ouie_reso2": 0.4,
        "ouie_lfo1Depth": 0.3, "ouie_lfo1Rate": 0.5,
        "ouie_modDepth": 0.7,
    }, (0.5, 0.3, 0.45, 0.55, 0.0, 0.5),
     "Inharmonic FM and CZ under STRIFE. Alien vocal character.",
     ["aether", "fm", "alien", "vocal"]),

    ("Perpetual Merge", {
        "ouie_algo1": VA, "ouie_waveform1": SAW,
        "ouie_algo2": VA, "ouie_waveform2": SAW,
        "ouie_macroHammer": 0.9,
        "ouie_unisonCount": 2, "ouie_unisonDetune": 0.1,
        "ouie_ampA1": 3.0, "ouie_ampR1": 5.0, "ouie_ampS1": 0.9,
        "ouie_ampA2": 3.0, "ouie_ampR2": 5.0, "ouie_ampS2": 0.9,
        "ouie_cutoff1": 1500.0, "ouie_cutoff2": 1500.0,
        "ouie_filterLink": 1,
        "ouie_breathRate": 0.005, "ouie_breathDepth": 0.4,
    }, (0.2, 0.7, 0.4, 0.3, 0.1, 0.0),
     "Near-total LOVE with breathing. Two voices perpetually merging.",
     ["aether", "love", "merge", "perpetual"]),

    ("Ampullae Extreme", {
        "ouie_algo1": VA, "ouie_waveform1": SAW,
        "ouie_algo2": WAVEFOLDER, "ouie_algoParam2": 0.5,
        "ouie_macroHammer": -0.3,
        "ouie_cutoff1": 3000.0, "ouie_reso1": 0.3,
        "ouie_cutoff2": 2500.0, "ouie_reso2": 0.35,
        "ouie_macroAmpullae": 1.0,
        "ouie_macroCartilage": 0.8,
        "ouie_modDepth": 0.5,
    }, (0.5, 0.45, 0.3, 0.4, 0.0, 0.3),
     "Maximum sensitivity. Every touch radically reshapes the sound.",
     ["aether", "ampullae", "extreme", "expressive"]),

    ("Frozen Hammer", {
        "ouie_algo1": WAVETABLE, "ouie_wtPos1": 0.9,
        "ouie_algo2": KS, "ouie_algoParam2": 0.3,
        "ouie_macroHammer": 0.0,
        "ouie_ampA1": 2.0, "ouie_ampR1": 4.0, "ouie_ampS1": 0.5,
        "ouie_ampA2": 0.001, "ouie_ampD2": 3.0, "ouie_ampS2": 0.0,
        "ouie_cutoff1": 2000.0, "ouie_cutoff2": 6000.0,
        "ouie_macroCartilage": 0.2,
        "ouie_breathRate": 0.008, "ouie_breathDepth": 0.3,
    }, (0.4, 0.45, 0.25, 0.3, 0.1, 0.05),
     "Metallic wavetable drone over slowly decaying KS string. Time suspended.",
     ["aether", "wavetable", "ks", "frozen"]),

    ("Inverse Hammer", {
        "ouie_algo1": WAVEFOLDER, "ouie_algoParam1": 0.4,
        "ouie_algo2": ADDITIVE, "ouie_algoParam2": 0.7,
        "ouie_macroHammer": 0.0,
        "ouie_cutoff1": 2500.0, "ouie_reso1": 0.3,
        "ouie_cutoff2": 4000.0,
        "ouie_lfo1Depth": 0.15, "ouie_lfo1Rate": 0.1,
        "ouie_lfo2Depth": 0.2, "ouie_lfo2Rate": 0.08,
        "ouie_modDepth": 0.4,
        "ouie_breathRate": 0.01, "ouie_breathDepth": 0.3,
    }, (0.45, 0.45, 0.35, 0.4, 0.1, 0.15),
     "Rough wavefolder and bright additive. HAMMER at center invites exploration.",
     ["aether", "wavefolder", "additive", "explore"]),
]

# ══════════════════════════════════════════════════════════════════════════════
# FAMILY (15) — OUIE + fleet engine coupling presets
# ══════════════════════════════════════════════════════════════════════════════

partners = [
    ("Overdub", "Hammerhead x Dub",
     {"ouie_algo1": VA, "ouie_waveform1": SAW, "ouie_algo2": VA, "ouie_waveform2": SQUARE, "ouie_pw2": 0.4,
      "ouie_macroHammer": 0.3, "ouie_cutoff1": 2500.0, "ouie_cutoff2": 2000.0, "ouie_filterLink": 1,
      "ouie_modDepth": 0.4, "ouie_macroCurrent": 0.2},
     (0.4, 0.55, 0.3, 0.3, 0.2, 0.1),
     "Dub tape shadows the hammerhead. Delay echoes in warm water.",
     ["family", "overdub", "dub", "warm"]),

    ("Odyssey", "Hammerhead x Drift",
     {"ouie_algo1": WAVETABLE, "ouie_wtPos1": 0.4, "ouie_algo2": VA, "ouie_waveform2": TRIANGLE,
      "ouie_macroHammer": 0.4, "ouie_cutoff1": 2500.0, "ouie_cutoff2": 2000.0,
      "ouie_breathRate": 0.02, "ouie_breathDepth": 0.3},
     (0.4, 0.55, 0.4, 0.3, 0.2, 0.05),
     "Two wanderers in the same ocean. Drift meets duophony.",
     ["family", "odyssey", "drift", "wander"]),

    ("Onset", "Hammerhead x Drums",
     {"ouie_algo1": FM, "ouie_fmRatio1": 4.0, "ouie_fmIndex1": 2.0,
      "ouie_algo2": NOISE, "ouie_algoParam2": 0.5,
      "ouie_macroHammer": -0.4, "ouie_ampA1": 0.001, "ouie_ampD1": 0.3, "ouie_ampS1": 0.0,
      "ouie_ampA2": 0.001, "ouie_ampD2": 0.15, "ouie_ampS2": 0.0,
      "ouie_cutoff1": 5000.0, "ouie_cutoff2": 3000.0, "ouie_reso2": 0.4},
     (0.5, 0.35, 0.15, 0.4, 0.0, 0.35),
     "STRIFE percussion pairs with drum engine. Metallic layers.",
     ["family", "onset", "drums", "percussive"]),

    ("Overworld", "Hammerhead x Chip",
     {"ouie_algo1": VA, "ouie_waveform1": SQUARE, "ouie_pw1": 0.5,
      "ouie_algo2": VA, "ouie_waveform2": TRIANGLE,
      "ouie_macroHammer": 0.2, "ouie_cutoff1": 6000.0, "ouie_cutoff2": 5000.0,
      "ouie_ampD1": 0.3, "ouie_ampS1": 0.3, "ouie_ampD2": 0.4, "ouie_ampS2": 0.2},
     (0.5, 0.45, 0.2, 0.25, 0.05, 0.1),
     "Square and triangle chip tones. 8-bit hammerhead.",
     ["family", "overworld", "chip", "retro"]),

    ("Opal", "Hammerhead x Granular",
     {"ouie_algo1": WAVETABLE, "ouie_wtPos1": 0.6, "ouie_algo2": WAVETABLE, "ouie_wtPos2": 0.3,
      "ouie_macroHammer": 0.5, "ouie_cutoff1": 3000.0, "ouie_cutoff2": 2500.0,
      "ouie_breathRate": 0.015, "ouie_breathDepth": 0.25, "ouie_macroCurrent": 0.3},
     (0.4, 0.55, 0.35, 0.35, 0.3, 0.05),
     "Granular textures woven with LOVE-blended wavetables.",
     ["family", "opal", "granular", "texture"]),

    ("Organon", "Hammerhead x Metabolic",
     {"ouie_algo1": ADDITIVE, "ouie_algoParam1": 0.6, "ouie_algo2": VA, "ouie_waveform2": SAW,
      "ouie_macroHammer": 0.3, "ouie_cutoff1": 3000.0, "ouie_cutoff2": 2000.0,
      "ouie_breathRate": 0.01, "ouie_breathDepth": 0.35, "ouie_ampA1": 1.0, "ouie_ampR1": 2.5},
     (0.4, 0.55, 0.4, 0.35, 0.15, 0.05),
     "Metabolic rhythms drive the hammerhead. Living synthesis.",
     ["family", "organon", "metabolic", "living"]),

    ("Ouroboros", "Hammerhead x Chaos",
     {"ouie_algo1": VA, "ouie_waveform1": SAW, "ouie_algo2": WAVEFOLDER, "ouie_algoParam2": 0.5,
      "ouie_macroHammer": -0.5, "ouie_cutoff1": 2000.0, "ouie_reso1": 0.4,
      "ouie_cutoff2": 1500.0, "ouie_reso2": 0.35, "ouie_modDepth": 0.5,
      "ouie_lfo1Depth": 0.2, "ouie_lfo1Rate": 0.3},
     (0.4, 0.4, 0.4, 0.45, 0.1, 0.35),
     "STRIFE chaos fed to the strange attractor. Leashed destruction.",
     ["family", "ouroboros", "chaos", "strife"]),

    ("Oblong", "Hammerhead x Amber",
     {"ouie_algo1": VA, "ouie_waveform1": SAW, "ouie_algo2": VA, "ouie_waveform2": SAW,
      "ouie_macroHammer": 0.5, "ouie_cutoff1": 3000.0, "ouie_cutoff2": 2500.0,
      "ouie_filterLink": 1, "ouie_macroCurrent": 0.25},
     (0.45, 0.6, 0.2, 0.3, 0.25, 0.05),
     "Warm amber PlaySurface meets steel duophony. LOVE blends them.",
     ["family", "oblong", "warm", "love"]),

    ("Obese", "Hammerhead x Fat",
     {"ouie_algo1": VA, "ouie_waveform1": SAW, "ouie_algo2": WAVEFOLDER, "ouie_algoParam2": 0.6,
      "ouie_macroHammer": -0.5, "ouie_cutoff1": 1500.0, "ouie_reso1": 0.45,
      "ouie_cutoff2": 1200.0, "ouie_reso2": 0.4, "ouie_modDepth": 0.6},
     (0.35, 0.5, 0.25, 0.5, 0.0, 0.5),
     "Fat saturation meets STRIFE wavefolder. Maximum weight.",
     ["family", "obese", "fat", "saturation"]),

    ("Oracle", "Hammerhead x Prophecy",
     {"ouie_algo1": FM, "ouie_fmRatio1": 3.5, "ouie_fmIndex1": 1.5,
      "ouie_algo2": ADDITIVE, "ouie_algoParam2": 0.6,
      "ouie_macroHammer": 0.2, "ouie_cutoff1": 4000.0, "ouie_cutoff2": 3000.0,
      "ouie_breathRate": 0.015, "ouie_breathDepth": 0.25},
     (0.5, 0.45, 0.3, 0.35, 0.15, 0.1),
     "FM prophecy and additive harmonics. Stochastic meets deterministic.",
     ["family", "oracle", "prophetic", "fm"]),

    ("Oceanic", "Hammerhead x Tidal",
     {"ouie_algo1": VA, "ouie_waveform1": SAW, "ouie_algo2": NOISE, "ouie_algoParam2": 0.3,
      "ouie_macroHammer": 0.3, "ouie_voiceMix": 0.35,
      "ouie_cutoff1": 1500.0, "ouie_cutoff2": 800.0,
      "ouie_breathRate": 0.008, "ouie_breathDepth": 0.4, "ouie_macroCurrent": 0.3},
     (0.25, 0.65, 0.4, 0.3, 0.3, 0.0),
     "Deep ocean coupling. Tidal breath drives the hammerhead.",
     ["family", "oceanic", "tidal", "deep"]),

    ("Origami", "Hammerhead x Folds",
     {"ouie_algo1": WAVEFOLDER, "ouie_algoParam1": 0.5, "ouie_algo2": PHASE_DIST, "ouie_algoParam2": 0.5,
      "ouie_macroHammer": -0.3, "ouie_cutoff1": 3000.0, "ouie_reso1": 0.25,
      "ouie_cutoff2": 2500.0, "ouie_reso2": 0.3, "ouie_modDepth": 0.5},
     (0.5, 0.4, 0.2, 0.4, 0.0, 0.3),
     "Wavefolder meets phase distortion. Paper and steel under STRIFE.",
     ["family", "origami", "fold", "strife"]),

    ("Overlap", "Hammerhead x Topology",
     {"ouie_algo1": VA, "ouie_waveform1": SAW, "ouie_algo2": WAVETABLE, "ouie_wtPos2": 0.5,
      "ouie_macroHammer": 0.4, "ouie_cutoff1": 3000.0, "ouie_cutoff2": 2500.0,
      "ouie_modDepth": 0.4, "ouie_macroCurrent": 0.2},
     (0.45, 0.5, 0.3, 0.35, 0.2, 0.1),
     "Topological knots bind the hammerhead. LOVE entangles.",
     ["family", "overlap", "topology", "entangled"]),

    ("OceanDeep", "Hammerhead x Trench",
     {"ouie_algo1": VA, "ouie_waveform1": SAW, "ouie_algo2": VA, "ouie_waveform2": TRIANGLE,
      "ouie_macroHammer": 0.2, "ouie_cutoff1": 800.0, "ouie_reso1": 0.3,
      "ouie_cutoff2": 600.0, "ouie_reso2": 0.25, "ouie_filterLink": 1,
      "ouie_breathRate": 0.005, "ouie_breathDepth": 0.4},
     (0.15, 0.7, 0.35, 0.3, 0.2, 0.05),
     "Trench pressure compresses the hammerhead. Dark and deep.",
     ["family", "oceandeep", "trench", "pressure"]),

    ("Osprey", "Hammerhead x Shore",
     {"ouie_algo1": KS, "ouie_algoParam1": 0.6, "ouie_algo2": NOISE, "ouie_algoParam2": 0.5,
      "ouie_macroHammer": 0.1,
      "ouie_ampA1": 0.001, "ouie_ampD1": 1.0, "ouie_ampS1": 0.0,
      "ouie_ampA2": 0.1, "ouie_ampR2": 1.5,
      "ouie_cutoff1": 6000.0, "ouie_cutoff2": 3000.0, "ouie_reso2": 0.25,
      "ouie_macroCurrent": 0.3},
     (0.5, 0.5, 0.2, 0.3, 0.3, 0.05),
     "Shore pluck and sea spray. Where the hammerhead meets the coast.",
     ["family", "osprey", "shore", "plucked"]),
]

FAMILY_PRESETS = []
for partner_name, preset_name, overrides, dna, desc, tags in partners:
    FAMILY_PRESETS.append(make_preset(preset_name, "Family", overrides, dna, desc, tags,
        engines=["Ouie", partner_name],
        coupling=[{"engineA": "Ouie", "engineB": partner_name,
                   "type": "AudioToFM", "amount": 0.5}],
        coupling_intensity="Medium"))


# ══════════════════════════════════════════════════════════════════════════════
# GENERATE ALL PRESETS
# ══════════════════════════════════════════════════════════════════════════════

def main():
    count = 0

    # Foundation (30)
    for name, overrides, dna, desc, tags in FOUNDATION:
        save(make_preset(name, "Foundation", overrides, dna, desc, tags))
        count += 1

    # Atmosphere (25)
    for name, overrides, dna, desc, tags in ATMOSPHERE:
        save(make_preset(name, "Atmosphere", overrides, dna, desc, tags))
        count += 1

    # Entangled (20)
    for name, overrides, dna, desc, tags in ENTANGLED:
        save(make_preset(name, "Entangled", overrides, dna, desc, tags))
        count += 1

    # Prism: Techno (8) + Synthwave (8) + Cinematic (5) + DnB (4) = 25
    for name, overrides, dna, desc, tags in TECHNO:
        save(make_preset(name, "Prism", overrides, dna, desc, tags))
        count += 1
    for name, overrides, dna, desc, tags in SYNTHWAVE:
        save(make_preset(name, "Prism", overrides, dna, desc, tags))
        count += 1
    for name, overrides, dna, desc, tags in CINEMATIC:
        save(make_preset(name, "Prism", overrides, dna, desc, tags))
        count += 1
    for name, overrides, dna, desc, tags in DNB:
        save(make_preset(name, "Prism", overrides, dna, desc, tags))
        count += 1

    # Flux (20)
    for name, overrides, dna, desc, tags in FLUX_PRESETS:
        save(make_preset(name, "Flux", overrides, dna, desc, tags))
        count += 1

    # Aether (15)
    for name, overrides, dna, desc, tags in AETHER:
        save(make_preset(name, "Aether", overrides, dna, desc, tags))
        count += 1

    # Family (15)
    for preset in FAMILY_PRESETS:
        save(preset)
        count += 1

    print(f"Generated {count} OUIE presets across 7 moods")

    # Distribution check
    from collections import Counter
    moods = Counter()
    moods["Foundation"] = len(FOUNDATION)
    moods["Atmosphere"] = len(ATMOSPHERE)
    moods["Entangled"] = len(ENTANGLED)
    moods["Prism"] = len(TECHNO) + len(SYNTHWAVE) + len(CINEMATIC) + len(DNB)
    moods["Flux"] = len(FLUX_PRESETS)
    moods["Aether"] = len(AETHER)
    moods["Family"] = len(FAMILY_PRESETS)
    for mood, n in sorted(moods.items()):
        print(f"  {mood}: {n}")
    print(f"  TOTAL: {sum(moods.values())}")


if __name__ == "__main__":
    main()
