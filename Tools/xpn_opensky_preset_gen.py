#!/usr/bin/env python3
"""OPENSKY Factory Preset Generator — 150 presets for the euphoric shimmer engine.

Generates .xometa JSON files across 7 moods:
  Foundation (30): init + ascending difficulty showcase of OpenSky features
  Atmosphere (25): ambient pads, slow shimmer, vast spaces
  Entangled (20): coupling showcases (internal + external routes)
  Prism (25): genre production (trance, EDM, pop, synthwave, progressive)
  Flux (20): movement, arps, morphing textures
  Aether (15): experimental, extreme, otherworldly
  Family (15): OPENSKY + 15 different fleet engines

Identity: XOpenSky — The Flying Fish | Sunburst #FF8C00 | Gallery: OPENSKY
  Euphoric shimmer synth: supersaw stack → bright filter → shimmer reverb →
  stereo chorus → unison engine → amp envelope → output
  4 Macros: RISE, WIDTH, GLOW, AIR
"""

import json
import os

PRESET_DIR = os.path.join(os.path.dirname(__file__), "..", "Presets", "XOceanus")

# ── OPENSKY defaults (50 params from OpenSkyEngine.h) ────────────────────────
DEFAULTS = {
    # Oscillator
    "sky_sawSpread": 0.3,
    "sky_sawMix": 0.8,
    "sky_subLevel": 0.2,
    "sky_subWave": 0,          # 0=Sine, 1=Square, 2=Triangle
    "sky_coarseTune": 0.0,
    "sky_fineTune": 0.0,
    # Pitch Envelope
    "sky_pitchEnvAmount": 0.0,
    "sky_pitchEnvDecay": 0.3,
    # Filter
    "sky_filterCutoff": 6000.0,
    "sky_filterReso": 0.2,
    "sky_filterHP": 80.0,
    "sky_filterEnvAmount": 0.4,
    "sky_filterType": 0,       # 0=LowPass, 1=BandPass
    # Shimmer Reverb
    "sky_shimmerMix": 0.3,
    "sky_shimmerSize": 0.6,
    "sky_shimmerDamping": 0.3,
    "sky_shimmerFeedback": 0.4,
    "sky_shimmerOctave": 0.6,
    # Chorus
    "sky_chorusRate": 0.5,
    "sky_chorusDepth": 0.4,
    "sky_chorusMix": 0.3,
    # Unison
    "sky_unisonCount": 1.0,
    "sky_unisonDetune": 0.2,
    "sky_unisonSpread": 0.5,
    # Amp Envelope
    "sky_attack": 0.01,
    "sky_decay": 0.3,
    "sky_sustain": 0.7,
    "sky_release": 0.5,
    "sky_level": 0.8,
    "sky_pan": 0.5,
    # LFOs
    "sky_lfo1Rate": 0.08,
    "sky_lfo1Depth": 0.15,
    "sky_lfo1Shape": 0,        # 0=Sine, 1=Triangle, 2=Saw, 3=Square, 4=S&H
    "sky_lfo2Rate": 2.0,
    "sky_lfo2Depth": 0.0,
    "sky_lfo2Shape": 0,
    # Stereo
    "sky_stereoWidth": 0.5,
    # Macros (center = 0.5, bipolar from center)
    "sky_macroRise": 0.5,
    "sky_macroWidth": 0.5,
    "sky_macroGlow": 0.5,
    "sky_macroAir": 0.5,
    # Mod Matrix
    "sky_modSlot1Src": 0.0,
    "sky_modSlot1Dst": 0.0,
    "sky_modSlot1Amt": 0.0,
    "sky_modSlot2Src": 0.0,
    "sky_modSlot2Dst": 0.0,
    "sky_modSlot2Amt": 0.0,
}

# Sub wave constants
SUB_SINE, SUB_SQUARE, SUB_TRI = 0, 1, 2

# Filter type constants
FILT_LP, FILT_BP = 0, 1

# LFO shape constants
LFO_SINE, LFO_TRI, LFO_SAW, LFO_SQUARE, LFO_SNH = 0, 1, 2, 3, 4


def make_preset(name, mood, overrides, dna, desc="", tags=None, engines=None,
                coupling=None, coupling_intensity="None"):
    """Build a full .xometa dict from name + param overrides."""
    params = dict(DEFAULTS)
    params.update(overrides)

    preset = {
        "schema_version": 1,
        "name": name,
        "mood": mood,
        "engines": engines or ["OpenSky"],
        "author": "XO_OX Designs",
        "version": "1.0.0",
        "description": desc,
        "tags": tags or [],
        "macroLabels": ["RISE", "WIDTH", "GLOW", "AIR"],
        "couplingIntensity": coupling_intensity,
        "tempo": None,
        "dna": {
            "brightness": dna[0], "warmth": dna[1], "movement": dna[2],
            "density": dna[3], "space": dna[4], "aggression": dna[5]
        },
        "parameters": {"OpenSky": params},
    }
    if coupling:
        preset["coupling"] = {"pairs": coupling}
    return preset


def save(preset):
    """Write preset to the correct mood directory."""
    mood = preset["mood"]
    safe_name = preset["name"].replace(" ", "_").replace("'", "").replace(",", "")
    filename = f"OpenSky_{safe_name}.xometa"
    path = os.path.join(PRESET_DIR, mood, filename)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w") as f:
        json.dump(preset, f, indent=2)
    return path


# ══════════════════════════════════════════════════════════════════════════════
# FOUNDATION (30) — Init presets + progressive feature showcase
# ══════════════════════════════════════════════════════════════════════════════

FOUNDATION = [
    # 1. Init — bare supersaw, no FX
    ("Sky Init", {
        "sky_shimmerMix": 0.0, "sky_chorusMix": 0.0,
        "sky_filterEnvAmount": 0.0, "sky_lfo1Depth": 0.0,
    }, (0.5, 0.4, 0.0, 0.2, 0.0, 0.0),
     "Raw supersaw stack. No shimmer, no chorus. The starting point.",
     ["init", "basic", "supersaw"]),

    # 2. Bright Saw — open filter
    ("Bright Saw", {
        "sky_filterCutoff": 10000.0, "sky_shimmerMix": 0.0,
        "sky_chorusMix": 0.0, "sky_lfo1Depth": 0.0,
    }, (0.7, 0.35, 0.0, 0.2, 0.0, 0.1),
     "Open filter reveals the full brightness of the saw stack.",
     ["bright", "saw", "basic"]),

    # 3. Warm Filter — LP shapes tone
    ("Warm Filter", {
        "sky_filterCutoff": 2000.0, "sky_filterReso": 0.3,
        "sky_shimmerMix": 0.0, "sky_chorusMix": 0.0,
    }, (0.3, 0.7, 0.0, 0.2, 0.0, 0.0),
     "Low-pass filter closes down. The supersaw becomes warm and muted.",
     ["warm", "filter", "muted"]),

    # 4. Filter Sweep — envelope opens filter
    ("Filter Sweep", {
        "sky_filterCutoff": 1500.0, "sky_filterReso": 0.35,
        "sky_filterEnvAmount": 0.7,
        "sky_shimmerMix": 0.0, "sky_chorusMix": 0.0,
    }, (0.5, 0.55, 0.3, 0.25, 0.0, 0.1),
     "Velocity opens the filter. Play hard for brightness, soft for warmth.",
     ["sweep", "filter", "velocity"]),

    # 5. Spread Wide — supersaw detune
    ("Spread Wide", {
        "sky_sawSpread": 0.7,
        "sky_shimmerMix": 0.0, "sky_chorusMix": 0.0,
    }, (0.55, 0.45, 0.1, 0.35, 0.1, 0.1),
     "Supersaw spread increased. Seven voices pulling apart.",
     ["spread", "detune", "wide"]),

    # 6. Sub Foundation — sub oscillator
    ("Sub Foundation", {
        "sky_subLevel": 0.6, "sky_subWave": SUB_SINE,
        "sky_filterCutoff": 3000.0,
        "sky_shimmerMix": 0.0, "sky_chorusMix": 0.0,
    }, (0.4, 0.65, 0.0, 0.3, 0.0, 0.1),
     "Sub sine adds weight beneath the supersaw. Full-range foundation.",
     ["sub", "bass", "foundation"]),

    # 7. First Shimmer — shimmer reverb
    ("First Shimmer", {
        "sky_shimmerMix": 0.35, "sky_shimmerSize": 0.5,
        "sky_shimmerFeedback": 0.3, "sky_chorusMix": 0.0,
    }, (0.6, 0.45, 0.15, 0.25, 0.4, 0.0),
     "Shimmer reverb engaged. Octave and fifth harmonics trail each note.",
     ["shimmer", "reverb", "ethereal"]),

    # 8. Chorus Width — stereo chorus
    ("Chorus Width", {
        "sky_chorusDepth": 0.5, "sky_chorusMix": 0.4,
        "sky_shimmerMix": 0.0,
    }, (0.5, 0.5, 0.4, 0.3, 0.3, 0.0),
     "Three-voice chorus spreads the sound across stereo. Warm movement.",
     ["chorus", "stereo", "width"]),

    # 9. Shimmer Chorus — both FX active
    ("Shimmer Chorus", {
        "sky_shimmerMix": 0.3, "sky_shimmerSize": 0.55,
        "sky_chorusDepth": 0.4, "sky_chorusMix": 0.35,
    }, (0.6, 0.5, 0.35, 0.3, 0.45, 0.0),
     "Shimmer and chorus together. The signature OpenSky combination.",
     ["shimmer", "chorus", "signature"]),

    # 10. Unison Stack — multiple unison voices
    ("Unison Stack", {
        "sky_unisonCount": 3.0, "sky_unisonDetune": 0.3,
        "sky_unisonSpread": 0.6,
        "sky_shimmerMix": 0.15, "sky_chorusMix": 0.2,
    }, (0.6, 0.45, 0.2, 0.5, 0.3, 0.15),
     "Three unison voices layered. The supersaw wall thickens.",
     ["unison", "stack", "thick"]),

    # 11. Rise Macro — pitch envelope ascension
    ("Rise Tutorial", {
        "sky_macroRise": 0.7, "sky_pitchEnvAmount": 6.0,
        "sky_pitchEnvDecay": 0.5,
        "sky_shimmerMix": 0.25, "sky_chorusMix": 0.2,
    }, (0.6, 0.45, 0.35, 0.3, 0.3, 0.1),
     "RISE macro: pitch sweeps up, filter opens, shimmer increases. Ascension.",
     ["rise", "macro", "pitch-envelope"]),

    # 12. Width Macro — stereo expansion
    ("Width Tutorial", {
        "sky_macroWidth": 0.75,
        "sky_unisonCount": 3.0, "sky_unisonDetune": 0.25,
        "sky_chorusDepth": 0.5, "sky_chorusMix": 0.4,
    }, (0.55, 0.5, 0.4, 0.4, 0.4, 0.0),
     "WIDTH macro: chorus deepens, unison spreads. The wall of sound.",
     ["width", "macro", "stereo"]),

    # 13. Glow Macro — shimmer tail
    ("Glow Tutorial", {
        "sky_macroGlow": 0.75,
        "sky_shimmerMix": 0.4, "sky_shimmerSize": 0.7,
        "sky_shimmerFeedback": 0.5,
    }, (0.55, 0.5, 0.2, 0.3, 0.6, 0.0),
     "GLOW macro: shimmer tail grows longer and feeds back. Trailing light.",
     ["glow", "macro", "shimmer-tail"]),

    # 14. Air Macro — high-frequency space
    ("Air Tutorial", {
        "sky_macroAir": 0.75,
        "sky_shimmerMix": 0.3, "sky_shimmerSize": 0.65,
    }, (0.65, 0.4, 0.15, 0.25, 0.55, 0.0),
     "AIR macro: reverb expands, high frequencies open. Oxygen.",
     ["air", "macro", "bright"]),

    # 15. Pad Init — basic pad shape
    ("Pad Init", {
        "sky_attack": 0.5, "sky_release": 1.5,
        "sky_filterCutoff": 4000.0,
        "sky_shimmerMix": 0.25, "sky_chorusMix": 0.2,
    }, (0.45, 0.6, 0.15, 0.25, 0.3, 0.0),
     "Slow attack, long release. The basic pad envelope.",
     ["pad", "init", "slow"]),

    # 16. Lead Init — focused lead
    ("Lead Init", {
        "sky_attack": 0.005, "sky_decay": 0.4, "sky_sustain": 0.6,
        "sky_filterCutoff": 8000.0, "sky_filterEnvAmount": 0.5,
        "sky_shimmerMix": 0.15, "sky_chorusMix": 0.15,
    }, (0.65, 0.4, 0.1, 0.2, 0.2, 0.15),
     "Fast attack, moderate sustain. A melodic lead starting point.",
     ["lead", "init", "melodic"]),

    # 17. Pluck Init — short percussive
    ("Pluck Init", {
        "sky_attack": 0.001, "sky_decay": 0.4, "sky_sustain": 0.0,
        "sky_release": 0.3,
        "sky_filterCutoff": 8000.0, "sky_filterEnvAmount": 0.6,
        "sky_shimmerMix": 0.2,
    }, (0.6, 0.4, 0.1, 0.2, 0.2, 0.1),
     "Zero attack, fast decay, no sustain. Pluck territory.",
     ["pluck", "short", "percussive"]),

    # 18. LFO Breathe — breathing filter
    ("LFO Breathe", {
        "sky_lfo1Rate": 0.05, "sky_lfo1Depth": 0.3,
        "sky_filterCutoff": 3000.0,
        "sky_shimmerMix": 0.2, "sky_chorusMix": 0.2,
        "sky_attack": 0.3, "sky_release": 1.0,
    }, (0.45, 0.55, 0.5, 0.25, 0.3, 0.0),
     "Slow breathing LFO modulates the filter. The sound inhales and exhales.",
     ["lfo", "breathing", "slow"]),

    # 19. HP Thinning — high-pass filter
    ("HP Thinning", {
        "sky_filterHP": 500.0,
        "sky_filterCutoff": 8000.0,
        "sky_shimmerMix": 0.2, "sky_chorusMix": 0.2,
    }, (0.6, 0.3, 0.1, 0.2, 0.2, 0.1),
     "High-pass filter removes low end. Thin and bright for layering.",
     ["highpass", "thin", "layer"]),

    # 20. BP Character — bandpass filter
    ("BP Character", {
        "sky_filterType": FILT_BP,
        "sky_filterCutoff": 3000.0, "sky_filterReso": 0.4,
        "sky_shimmerMix": 0.25,
    }, (0.45, 0.4, 0.1, 0.2, 0.2, 0.15),
     "Bandpass filter isolates a frequency band. Nasal and vocal character.",
     ["bandpass", "vocal", "character"]),

    # 21. Octave Sub — square sub
    ("Octave Sub", {
        "sky_subLevel": 0.5, "sky_subWave": SUB_SQUARE,
        "sky_coarseTune": -12.0,
        "sky_filterCutoff": 4000.0,
        "sky_shimmerMix": 0.1,
    }, (0.4, 0.6, 0.05, 0.35, 0.1, 0.15),
     "Octave-down tuning with square sub. Deep and present.",
     ["sub", "octave", "deep"]),

    # 22. Pitch Rise — pitch envelope up
    ("Pitch Rise", {
        "sky_pitchEnvAmount": 12.0, "sky_pitchEnvDecay": 0.4,
        "sky_filterCutoff": 5000.0,
        "sky_shimmerMix": 0.2,
    }, (0.55, 0.45, 0.3, 0.25, 0.2, 0.15),
     "Each note sweeps up an octave. Euphoric pitch riser.",
     ["pitch", "rise", "sweep"]),

    # 23. Shimmer Deep — full shimmer
    ("Shimmer Deep", {
        "sky_shimmerMix": 0.6, "sky_shimmerSize": 0.8,
        "sky_shimmerFeedback": 0.6, "sky_shimmerDamping": 0.2,
        "sky_attack": 0.3, "sky_release": 2.0,
    }, (0.6, 0.5, 0.2, 0.3, 0.7, 0.0),
     "Maximum shimmer depth. Long tails of octave and fifth harmonics.",
     ["shimmer", "deep", "long-tail"]),

    # 24. Full Unison — 7 voices wide
    ("Full Unison", {
        "sky_unisonCount": 7.0, "sky_unisonDetune": 0.4,
        "sky_unisonSpread": 0.8,
        "sky_shimmerMix": 0.2, "sky_chorusMix": 0.15,
        "sky_level": 0.6,
    }, (0.6, 0.45, 0.2, 0.8, 0.3, 0.2),
     "All seven unison voices. Dense, wide, massive.",
     ["unison", "full", "massive"]),

    # 25. Mod Wheel Filter — D006 expression
    ("Mod Wheel Open", {
        "sky_filterCutoff": 1500.0, "sky_filterReso": 0.35,
        "sky_filterEnvAmount": 0.3,
        "sky_shimmerMix": 0.2, "sky_chorusMix": 0.2,
    }, (0.4, 0.55, 0.2, 0.25, 0.25, 0.1),
     "Start dark. Mod wheel opens the filter — expression reveals brightness.",
     ["mod-wheel", "expression", "filter"]),

    # 26. Aftertouch Glow — D006 pressure
    ("Aftertouch Glow", {
        "sky_shimmerMix": 0.15, "sky_shimmerSize": 0.5,
        "sky_chorusMix": 0.2,
        "sky_attack": 0.3, "sky_release": 1.0,
    }, (0.5, 0.5, 0.2, 0.25, 0.35, 0.0),
     "Press into the keys. Aftertouch adds shimmer — more pressure, more glow.",
     ["aftertouch", "shimmer", "expression"]),

    # 27. Stereo Image — stereo width control
    ("Stereo Image", {
        "sky_stereoWidth": 0.9,
        "sky_chorusDepth": 0.5, "sky_chorusMix": 0.4,
        "sky_unisonCount": 3.0, "sky_unisonSpread": 0.7,
        "sky_shimmerMix": 0.2,
    }, (0.55, 0.5, 0.35, 0.4, 0.4, 0.0),
     "Maximum stereo width. Chorus and unison conspire to fill the panorama.",
     ["stereo", "wide", "panorama"]),

    # 28. Shimmer Octave — octave balance
    ("Shimmer Octave", {
        "sky_shimmerMix": 0.4, "sky_shimmerSize": 0.6,
        "sky_shimmerOctave": 1.0,
        "sky_shimmerFeedback": 0.45,
    }, (0.65, 0.4, 0.15, 0.3, 0.5, 0.0),
     "Shimmer balanced to full octave-up. Crystalline harmonic ceiling.",
     ["shimmer", "octave", "crystal"]),

    # 29. Shimmer Fifth — fifth balance
    ("Shimmer Fifth", {
        "sky_shimmerMix": 0.4, "sky_shimmerSize": 0.6,
        "sky_shimmerOctave": 0.0,
        "sky_shimmerFeedback": 0.45,
    }, (0.55, 0.5, 0.15, 0.3, 0.5, 0.0),
     "Shimmer balanced to full fifth-up. Harmonic richness with modal quality.",
     ["shimmer", "fifth", "harmonic"]),

    # 30. The Full Sky — everything combined
    ("The Full Sky", {
        "sky_sawSpread": 0.5, "sky_sawMix": 0.85,
        "sky_subLevel": 0.3, "sky_subWave": SUB_SINE,
        "sky_filterCutoff": 5000.0, "sky_filterReso": 0.25,
        "sky_filterEnvAmount": 0.5,
        "sky_shimmerMix": 0.4, "sky_shimmerSize": 0.65,
        "sky_shimmerFeedback": 0.45, "sky_shimmerDamping": 0.25,
        "sky_chorusDepth": 0.45, "sky_chorusMix": 0.35, "sky_chorusRate": 0.6,
        "sky_unisonCount": 5.0, "sky_unisonDetune": 0.3, "sky_unisonSpread": 0.6,
        "sky_attack": 0.3, "sky_release": 1.5,
        "sky_lfo1Rate": 0.06, "sky_lfo1Depth": 0.2,
        "sky_stereoWidth": 0.7,
        "sky_macroRise": 0.55, "sky_macroWidth": 0.6,
        "sky_macroGlow": 0.55, "sky_macroAir": 0.55,
    }, (0.65, 0.55, 0.45, 0.65, 0.55, 0.1),
     "Every feature active. The complete OpenSky experience. Fly.",
     ["showcase", "complete", "full"]),
]


# ══════════════════════════════════════════════════════════════════════════════
# ATMOSPHERE (25) — Ambient pads, slow shimmer, vast ethereal spaces
# ══════════════════════════════════════════════════════════════════════════════

ATMOSPHERE = [
    ("Sunlit Clouds", {
        "sky_attack": 2.0, "sky_release": 3.0, "sky_sustain": 0.8,
        "sky_filterCutoff": 3500.0, "sky_filterReso": 0.15,
        "sky_shimmerMix": 0.5, "sky_shimmerSize": 0.75,
        "sky_shimmerFeedback": 0.5, "sky_shimmerDamping": 0.2,
        "sky_chorusMix": 0.3, "sky_chorusDepth": 0.4,
        "sky_unisonCount": 3.0, "sky_unisonSpread": 0.6,
        "sky_lfo1Rate": 0.03, "sky_lfo1Depth": 0.2,
    }, (0.5, 0.65, 0.35, 0.4, 0.65, 0.0),
     "Warm clouds lit from behind. Slow breathing, long shimmer tails.",
     ["ambient", "clouds", "warm", "pad"]),

    ("Morning Horizon", {
        "sky_attack": 3.0, "sky_release": 4.0, "sky_sustain": 0.9,
        "sky_filterCutoff": 2500.0, "sky_filterReso": 0.1,
        "sky_shimmerMix": 0.45, "sky_shimmerSize": 0.8,
        "sky_shimmerFeedback": 0.55,
        "sky_lfo1Rate": 0.02, "sky_lfo1Depth": 0.15,
        "sky_stereoWidth": 0.8,
    }, (0.4, 0.7, 0.25, 0.3, 0.7, 0.0),
     "Dawn arriving. Impossibly slow attack. The horizon brightens.",
     ["ambient", "dawn", "slow", "horizon"]),

    ("Cathedral Light", {
        "sky_attack": 1.5, "sky_release": 5.0, "sky_sustain": 0.85,
        "sky_filterCutoff": 4000.0,
        "sky_shimmerMix": 0.55, "sky_shimmerSize": 0.85,
        "sky_shimmerFeedback": 0.55, "sky_shimmerOctave": 0.8,
        "sky_chorusMix": 0.2,
        "sky_macroAir": 0.65,
    }, (0.55, 0.55, 0.15, 0.3, 0.85, 0.0),
     "Sunbeams through stained glass. Octave shimmer builds sacred space.",
     ["ambient", "cathedral", "sacred", "reverb"]),

    ("Thermal Updraft", {
        "sky_attack": 0.8, "sky_release": 2.0,
        "sky_sawSpread": 0.5,
        "sky_filterCutoff": 5000.0, "sky_filterEnvAmount": 0.5,
        "sky_shimmerMix": 0.35, "sky_shimmerSize": 0.6,
        "sky_chorusMix": 0.3, "sky_chorusDepth": 0.5,
        "sky_unisonCount": 3.0, "sky_unisonDetune": 0.25,
        "sky_pitchEnvAmount": 3.0, "sky_pitchEnvDecay": 1.0,
        "sky_macroRise": 0.6,
    }, (0.6, 0.5, 0.4, 0.4, 0.45, 0.05),
     "Rising pitch envelope on every note. Thermals lifting upward.",
     ["ambient", "rising", "thermal", "updraft"]),

    ("Arctic Aurora", {
        "sky_attack": 2.5, "sky_release": 4.0,
        "sky_filterCutoff": 7000.0, "sky_filterHP": 200.0,
        "sky_shimmerMix": 0.5, "sky_shimmerSize": 0.75,
        "sky_shimmerFeedback": 0.5, "sky_shimmerOctave": 0.9,
        "sky_chorusMix": 0.35, "sky_chorusRate": 0.3,
        "sky_lfo1Rate": 0.015, "sky_lfo1Depth": 0.25,
        "sky_stereoWidth": 0.85,
    }, (0.7, 0.35, 0.35, 0.35, 0.7, 0.0),
     "Cold and crystalline. LFO pulses shimmer like northern lights.",
     ["ambient", "arctic", "aurora", "cold"]),

    ("Velvet Dusk", {
        "sky_attack": 1.5, "sky_release": 3.0, "sky_sustain": 0.75,
        "sky_filterCutoff": 2000.0, "sky_filterReso": 0.2,
        "sky_shimmerMix": 0.4, "sky_shimmerSize": 0.7,
        "sky_shimmerDamping": 0.5,
        "sky_chorusMix": 0.3, "sky_chorusDepth": 0.4,
        "sky_subLevel": 0.3,
        "sky_lfo1Rate": 0.04, "sky_lfo1Depth": 0.15,
    }, (0.3, 0.75, 0.3, 0.35, 0.55, 0.0),
     "Dark and warm. Damped shimmer retains only the lowest harmonics.",
     ["ambient", "dusk", "dark", "warm"]),

    ("Glass Ocean", {
        "sky_attack": 1.0, "sky_release": 3.0,
        "sky_filterCutoff": 6000.0, "sky_filterHP": 150.0,
        "sky_shimmerMix": 0.5, "sky_shimmerSize": 0.7,
        "sky_shimmerFeedback": 0.45,
        "sky_chorusMix": 0.4, "sky_chorusRate": 0.4, "sky_chorusDepth": 0.5,
        "sky_unisonCount": 3.0, "sky_unisonSpread": 0.7,
        "sky_stereoWidth": 0.8,
    }, (0.6, 0.5, 0.4, 0.4, 0.6, 0.0),
     "Calm water reflecting sunlight. Wide chorus ripples across the surface.",
     ["ambient", "ocean", "glass", "wide"]),

    ("Ember Trail", {
        "sky_attack": 0.5, "sky_release": 4.0, "sky_sustain": 0.5,
        "sky_decay": 1.5,
        "sky_filterCutoff": 3000.0, "sky_filterEnvAmount": 0.6,
        "sky_shimmerMix": 0.45, "sky_shimmerSize": 0.75,
        "sky_shimmerFeedback": 0.55,
        "sky_macroGlow": 0.7,
    }, (0.5, 0.6, 0.2, 0.3, 0.65, 0.0),
     "Notes fade like embers. Long release, glowing shimmer trail.",
     ["ambient", "ember", "trail", "fade"]),

    ("Floating Garden", {
        "sky_attack": 1.2, "sky_release": 2.5,
        "sky_sawSpread": 0.4,
        "sky_filterCutoff": 4500.0, "sky_filterReso": 0.15,
        "sky_shimmerMix": 0.35, "sky_shimmerSize": 0.6,
        "sky_chorusMix": 0.35, "sky_chorusDepth": 0.45,
        "sky_unisonCount": 3.0, "sky_unisonDetune": 0.2,
        "sky_lfo1Rate": 0.04, "sky_lfo1Depth": 0.2,
        "sky_lfo1Shape": LFO_TRI,
    }, (0.5, 0.6, 0.45, 0.4, 0.5, 0.0),
     "Triangle LFO gently rocks the filter. A garden suspended in air.",
     ["ambient", "garden", "gentle", "floating"]),

    ("Stratosphere", {
        "sky_attack": 2.0, "sky_release": 5.0, "sky_sustain": 0.85,
        "sky_filterCutoff": 8000.0, "sky_filterHP": 400.0,
        "sky_shimmerMix": 0.55, "sky_shimmerSize": 0.85,
        "sky_shimmerFeedback": 0.6, "sky_shimmerOctave": 0.8,
        "sky_chorusMix": 0.25,
        "sky_macroAir": 0.8,
        "sky_stereoWidth": 0.9,
    }, (0.8, 0.3, 0.2, 0.3, 0.85, 0.0),
     "Maximum altitude. Thin air, crystalline shimmer, infinite space.",
     ["ambient", "stratosphere", "thin", "bright"]),

    ("Coral Sunrise", {
        "sky_attack": 1.0, "sky_release": 2.0,
        "sky_sawSpread": 0.45,
        "sky_filterCutoff": 5000.0, "sky_filterEnvAmount": 0.4,
        "sky_shimmerMix": 0.35, "sky_shimmerSize": 0.55,
        "sky_shimmerOctave": 0.4,
        "sky_chorusMix": 0.3, "sky_chorusDepth": 0.4,
        "sky_subLevel": 0.25,
    }, (0.55, 0.6, 0.3, 0.35, 0.45, 0.0),
     "Fifth-heavy shimmer warms the dawn. Coral tones rising with the sun.",
     ["ambient", "coral", "sunrise", "warm"]),

    ("Lunar Drift", {
        "sky_attack": 2.0, "sky_release": 4.0,
        "sky_filterCutoff": 3000.0, "sky_filterReso": 0.15,
        "sky_shimmerMix": 0.45, "sky_shimmerSize": 0.7,
        "sky_shimmerDamping": 0.4,
        "sky_lfo1Rate": 0.01, "sky_lfo1Depth": 0.25,
        "sky_lfo2Rate": 0.07, "sky_lfo2Depth": 0.1,
        "sky_stereoWidth": 0.7,
    }, (0.4, 0.55, 0.4, 0.25, 0.6, 0.0),
     "Two LFOs at different rates create complex slow drift. Moonlit.",
     ["ambient", "lunar", "drift", "two-lfo"]),

    ("Vapor Trail", {
        "sky_attack": 0.3, "sky_release": 5.0, "sky_sustain": 0.3,
        "sky_decay": 2.0,
        "sky_filterCutoff": 6000.0,
        "sky_shimmerMix": 0.6, "sky_shimmerSize": 0.8,
        "sky_shimmerFeedback": 0.6, "sky_shimmerDamping": 0.15,
        "sky_macroGlow": 0.75,
    }, (0.6, 0.4, 0.15, 0.25, 0.8, 0.0),
     "Short notes leave massive vapor trails. Shimmer dominates the tail.",
     ["ambient", "vapor", "trail", "shimmer"]),

    ("Deep Breath", {
        "sky_attack": 3.0, "sky_release": 3.0, "sky_sustain": 0.9,
        "sky_filterCutoff": 2500.0, "sky_filterReso": 0.2,
        "sky_shimmerMix": 0.35, "sky_shimmerSize": 0.6,
        "sky_chorusMix": 0.3,
        "sky_lfo1Rate": 0.02, "sky_lfo1Depth": 0.35,
        "sky_subLevel": 0.35,
    }, (0.35, 0.7, 0.45, 0.35, 0.5, 0.0),
     "Very slow LFO breathes the filter. Sub adds chest resonance.",
     ["ambient", "breath", "deep", "lfo"]),

    ("Cloud Nine", {
        "sky_attack": 1.5, "sky_release": 3.5, "sky_sustain": 0.8,
        "sky_sawSpread": 0.5,
        "sky_filterCutoff": 4500.0,
        "sky_shimmerMix": 0.45, "sky_shimmerSize": 0.7,
        "sky_shimmerFeedback": 0.5, "sky_shimmerOctave": 0.7,
        "sky_chorusMix": 0.35, "sky_chorusDepth": 0.45,
        "sky_unisonCount": 5.0, "sky_unisonDetune": 0.25,
        "sky_unisonSpread": 0.7,
        "sky_macroWidth": 0.65,
    }, (0.6, 0.55, 0.35, 0.55, 0.6, 0.0),
     "The quintessential euphoric pad. Five unison voices floating on shimmer.",
     ["ambient", "euphoric", "pad", "cloud"]),

    ("Prism Rain", {
        "sky_attack": 0.8, "sky_release": 2.5,
        "sky_filterCutoff": 5500.0, "sky_filterEnvAmount": 0.5,
        "sky_shimmerMix": 0.4, "sky_shimmerSize": 0.65,
        "sky_shimmerOctave": 0.5,
        "sky_chorusMix": 0.3, "sky_chorusRate": 0.7,
        "sky_lfo1Rate": 0.06, "sky_lfo1Depth": 0.2,
    }, (0.6, 0.5, 0.4, 0.3, 0.5, 0.0),
     "Light refracting through rain. Filter envelope adds prismatic color.",
     ["ambient", "prism", "rain", "color"]),

    ("Solstice Glow", {
        "sky_attack": 2.0, "sky_release": 4.0,
        "sky_filterCutoff": 3500.0,
        "sky_shimmerMix": 0.5, "sky_shimmerSize": 0.75,
        "sky_shimmerFeedback": 0.5, "sky_shimmerDamping": 0.3,
        "sky_macroGlow": 0.7, "sky_macroAir": 0.6,
    }, (0.5, 0.6, 0.2, 0.3, 0.7, 0.0),
     "The longest day. GLOW and AIR macros create endless warm light.",
     ["ambient", "solstice", "glow", "warm"]),

    ("Feather Fall", {
        "sky_attack": 0.5, "sky_release": 3.0, "sky_sustain": 0.4,
        "sky_decay": 1.0,
        "sky_filterCutoff": 7000.0, "sky_filterHP": 300.0,
        "sky_shimmerMix": 0.35, "sky_shimmerSize": 0.6,
        "sky_chorusMix": 0.25,
        "sky_pitchEnvAmount": -3.0, "sky_pitchEnvDecay": 1.5,
    }, (0.6, 0.4, 0.25, 0.25, 0.5, 0.0),
     "Downward pitch envelope. Each note drifts gently earthward.",
     ["ambient", "feather", "falling", "gentle"]),

    ("Sapphire Haze", {
        "sky_attack": 1.5, "sky_release": 3.0,
        "sky_filterCutoff": 3000.0, "sky_filterReso": 0.25,
        "sky_filterType": FILT_BP,
        "sky_shimmerMix": 0.45, "sky_shimmerSize": 0.7,
        "sky_shimmerFeedback": 0.45,
        "sky_chorusMix": 0.35, "sky_chorusDepth": 0.5,
        "sky_lfo1Rate": 0.03, "sky_lfo1Depth": 0.2,
    }, (0.45, 0.5, 0.35, 0.3, 0.6, 0.0),
     "Bandpass focus through shimmer haze. Vowel-like and mysterious.",
     ["ambient", "haze", "bandpass", "mysterious"]),

    ("Altitude", {
        "sky_attack": 1.0, "sky_release": 2.5, "sky_sustain": 0.8,
        "sky_filterCutoff": 6000.0, "sky_filterHP": 250.0,
        "sky_shimmerMix": 0.4, "sky_shimmerSize": 0.65,
        "sky_shimmerOctave": 0.75,
        "sky_chorusMix": 0.3,
        "sky_unisonCount": 3.0, "sky_unisonSpread": 0.6,
        "sky_macroRise": 0.6, "sky_macroAir": 0.6,
    }, (0.65, 0.4, 0.3, 0.4, 0.6, 0.0),
     "Bright and elevated. RISE and AIR macros push toward the ceiling.",
     ["ambient", "altitude", "elevated", "bright"]),

    ("Soft Nimbus", {
        "sky_attack": 2.5, "sky_release": 4.0, "sky_sustain": 0.85,
        "sky_filterCutoff": 2000.0, "sky_filterReso": 0.1,
        "sky_shimmerMix": 0.4, "sky_shimmerSize": 0.7,
        "sky_shimmerDamping": 0.5, "sky_shimmerFeedback": 0.45,
        "sky_chorusMix": 0.3, "sky_chorusDepth": 0.35,
        "sky_subLevel": 0.3,
        "sky_level": 0.7,
    }, (0.3, 0.75, 0.25, 0.3, 0.6, 0.0),
     "Deeply damped shimmer under a warm filter. Soft, round, cloud-like.",
     ["ambient", "soft", "nimbus", "round"]),

    ("Zenith Pulse", {
        "sky_attack": 0.5, "sky_release": 2.0,
        "sky_sawSpread": 0.55,
        "sky_filterCutoff": 5000.0,
        "sky_shimmerMix": 0.35, "sky_shimmerSize": 0.6,
        "sky_chorusMix": 0.3,
        "sky_unisonCount": 5.0, "sky_unisonDetune": 0.3,
        "sky_lfo1Rate": 0.08, "sky_lfo1Depth": 0.25,
        "sky_lfo1Shape": LFO_TRI,
    }, (0.55, 0.5, 0.45, 0.5, 0.45, 0.05),
     "Triangle LFO pulses the filter at the peak. Five voices shimmer.",
     ["ambient", "zenith", "pulse", "triangle-lfo"]),

    ("Ozone Layer", {
        "sky_attack": 1.5, "sky_release": 3.5,
        "sky_filterCutoff": 8000.0, "sky_filterHP": 500.0,
        "sky_shimmerMix": 0.5, "sky_shimmerSize": 0.8,
        "sky_shimmerFeedback": 0.55, "sky_shimmerOctave": 0.85,
        "sky_chorusMix": 0.2,
        "sky_macroAir": 0.8,
        "sky_stereoWidth": 0.85,
    }, (0.8, 0.25, 0.2, 0.25, 0.8, 0.0),
     "Extreme high-pass and high-frequency shimmer. The edge of the atmosphere.",
     ["ambient", "ozone", "thin", "extreme-air"]),

    ("Golden Hour", {
        "sky_attack": 1.0, "sky_release": 2.5, "sky_sustain": 0.75,
        "sky_sawSpread": 0.45,
        "sky_filterCutoff": 3500.0, "sky_filterReso": 0.2,
        "sky_filterEnvAmount": 0.3,
        "sky_shimmerMix": 0.4, "sky_shimmerSize": 0.65,
        "sky_shimmerDamping": 0.35, "sky_shimmerOctave": 0.45,
        "sky_chorusMix": 0.35, "sky_chorusDepth": 0.4,
        "sky_subLevel": 0.25,
        "sky_lfo1Rate": 0.04, "sky_lfo1Depth": 0.15,
    }, (0.5, 0.65, 0.35, 0.35, 0.55, 0.0),
     "Warm filter, fifth-heavy shimmer. The amber light before sunset.",
     ["ambient", "golden", "sunset", "warm"]),

    ("Cirrus Veil", {
        "sky_attack": 2.0, "sky_release": 4.0, "sky_sustain": 0.85,
        "sky_filterCutoff": 5000.0, "sky_filterHP": 200.0,
        "sky_shimmerMix": 0.45, "sky_shimmerSize": 0.7,
        "sky_shimmerFeedback": 0.5, "sky_shimmerOctave": 0.65,
        "sky_chorusMix": 0.3, "sky_chorusDepth": 0.4,
        "sky_unisonCount": 3.0, "sky_unisonSpread": 0.65,
        "sky_stereoWidth": 0.75,
    }, (0.6, 0.5, 0.3, 0.4, 0.6, 0.0),
     "High wispy clouds. Wide and transparent. Barely there, everywhere.",
     ["ambient", "cirrus", "wispy", "transparent"]),
]


# ══════════════════════════════════════════════════════════════════════════════
# ENTANGLED (20) — Coupling-focused presets, mod matrix, extreme macro use
# ══════════════════════════════════════════════════════════════════════════════

ENTANGLED = [
    ("Rise Lock", {
        "sky_macroRise": 0.85,
        "sky_pitchEnvAmount": 12.0, "sky_pitchEnvDecay": 0.6,
        "sky_filterCutoff": 2000.0, "sky_filterEnvAmount": 0.7,
        "sky_shimmerMix": 0.5, "sky_shimmerSize": 0.7,
        "sky_shimmerFeedback": 0.55,
        "sky_unisonCount": 3.0,
    }, (0.65, 0.45, 0.45, 0.4, 0.55, 0.15),
     "RISE macro locked high. Pitch sweeps, filter opens, shimmer builds.",
     ["entangled", "rise", "macro", "locked"]),

    ("Width Lock", {
        "sky_macroWidth": 0.9,
        "sky_chorusDepth": 0.6, "sky_chorusMix": 0.5,
        "sky_unisonCount": 7.0, "sky_unisonDetune": 0.4, "sky_unisonSpread": 0.9,
        "sky_stereoWidth": 0.9,
        "sky_shimmerMix": 0.2,
        "sky_level": 0.55,
    }, (0.55, 0.5, 0.5, 0.8, 0.5, 0.1),
     "WIDTH macro locked extreme. Maximum stereo spread across all systems.",
     ["entangled", "width", "extreme", "stereo"]),

    ("Glow Lock", {
        "sky_macroGlow": 0.9,
        "sky_shimmerMix": 0.6, "sky_shimmerSize": 0.85,
        "sky_shimmerFeedback": 0.7, "sky_shimmerDamping": 0.15,
        "sky_attack": 0.5, "sky_release": 4.0,
    }, (0.55, 0.5, 0.15, 0.3, 0.85, 0.0),
     "GLOW macro at maximum. Shimmer tails that never seem to end.",
     ["entangled", "glow", "infinite", "tail"]),

    ("Air Lock", {
        "sky_macroAir": 0.95,
        "sky_filterCutoff": 8000.0, "sky_filterHP": 400.0,
        "sky_shimmerMix": 0.5, "sky_shimmerSize": 0.85,
        "sky_shimmerOctave": 0.85,
        "sky_stereoWidth": 0.9,
    }, (0.85, 0.25, 0.15, 0.25, 0.85, 0.0),
     "AIR macro pushed to the limit. Pure oxygen. Brightness without weight.",
     ["entangled", "air", "extreme", "bright"]),

    ("Four Winds", {
        "sky_macroRise": 0.65, "sky_macroWidth": 0.65,
        "sky_macroGlow": 0.65, "sky_macroAir": 0.65,
        "sky_sawSpread": 0.5,
        "sky_shimmerMix": 0.4, "sky_shimmerSize": 0.7,
        "sky_chorusMix": 0.35,
        "sky_unisonCount": 5.0, "sky_unisonDetune": 0.3,
        "sky_attack": 0.5, "sky_release": 2.0,
    }, (0.65, 0.5, 0.4, 0.55, 0.6, 0.05),
     "All four macros equally elevated. Every dimension pushed outward.",
     ["entangled", "all-macros", "balanced", "elevated"]),

    ("Mod Matrix Shimmer", {
        "sky_modSlot1Src": 2.0, "sky_modSlot1Dst": 3.0, "sky_modSlot1Amt": 0.5,
        "sky_shimmerMix": 0.4, "sky_shimmerSize": 0.65,
        "sky_chorusMix": 0.3,
        "sky_attack": 0.5, "sky_release": 1.5,
    }, (0.55, 0.5, 0.35, 0.3, 0.5, 0.05),
     "Mod matrix routes LFO to shimmer. Programmatic glow modulation.",
     ["entangled", "mod-matrix", "shimmer", "routed"]),

    ("Mod Matrix Filter", {
        "sky_modSlot1Src": 1.0, "sky_modSlot1Dst": 1.0, "sky_modSlot1Amt": 0.6,
        "sky_modSlot2Src": 3.0, "sky_modSlot2Dst": 2.0, "sky_modSlot2Amt": 0.4,
        "sky_filterCutoff": 2000.0, "sky_filterReso": 0.35,
        "sky_shimmerMix": 0.25,
    }, (0.45, 0.55, 0.4, 0.25, 0.3, 0.1),
     "Two mod matrix slots: envelope to filter, velocity to resonance.",
     ["entangled", "mod-matrix", "filter", "dual-route"]),

    ("Cross Mod Pulse", {
        "sky_lfo2Rate": 6.0, "sky_lfo2Depth": 0.3, "sky_lfo2Shape": LFO_SQUARE,
        "sky_modSlot1Src": 3.0, "sky_modSlot1Dst": 4.0, "sky_modSlot1Amt": 0.5,
        "sky_shimmerMix": 0.2, "sky_chorusMix": 0.2,
        "sky_filterCutoff": 4000.0,
    }, (0.5, 0.45, 0.55, 0.3, 0.25, 0.2),
     "Square LFO2 pulsing through mod matrix. Rhythmic coupling.",
     ["entangled", "pulse", "square-lfo", "rhythmic"]),

    ("Resonant Entangle", {
        "sky_filterCutoff": 1500.0, "sky_filterReso": 0.65,
        "sky_filterEnvAmount": 0.8,
        "sky_shimmerMix": 0.35, "sky_shimmerFeedback": 0.55,
        "sky_macroRise": 0.7,
        "sky_lfo1Rate": 0.05, "sky_lfo1Depth": 0.3,
    }, (0.45, 0.45, 0.4, 0.35, 0.4, 0.25),
     "High resonance filter modulated by everything. Each control intensifies.",
     ["entangled", "resonant", "filter", "intense"]),

    ("Shimmer Feedback", {
        "sky_shimmerMix": 0.55, "sky_shimmerSize": 0.8,
        "sky_shimmerFeedback": 0.75, "sky_shimmerDamping": 0.1,
        "sky_shimmerOctave": 0.7,
        "sky_macroGlow": 0.8,
        "sky_attack": 0.3, "sky_release": 3.0,
    }, (0.65, 0.4, 0.2, 0.3, 0.8, 0.05),
     "Shimmer feeding back near maximum. Harmonics cascade upward endlessly.",
     ["entangled", "shimmer", "feedback", "cascade"]),

    ("Unison Entangle", {
        "sky_unisonCount": 7.0, "sky_unisonDetune": 0.5, "sky_unisonSpread": 0.85,
        "sky_macroWidth": 0.8,
        "sky_chorusMix": 0.4, "sky_chorusDepth": 0.5,
        "sky_shimmerMix": 0.3,
        "sky_level": 0.5,
    }, (0.55, 0.45, 0.4, 0.85, 0.45, 0.15),
     "Seven unison voices at extreme detune and spread. Dense entanglement.",
     ["entangled", "unison", "dense", "extreme"]),

    ("Dual LFO Weave", {
        "sky_lfo1Rate": 0.03, "sky_lfo1Depth": 0.3, "sky_lfo1Shape": LFO_SINE,
        "sky_lfo2Rate": 0.17, "sky_lfo2Depth": 0.2, "sky_lfo2Shape": LFO_TRI,
        "sky_filterCutoff": 3000.0,
        "sky_shimmerMix": 0.35, "sky_chorusMix": 0.3,
        "sky_attack": 0.8, "sky_release": 2.0,
    }, (0.45, 0.55, 0.55, 0.3, 0.45, 0.0),
     "Two LFOs at irrational rate ratio. Complex filter weaving.",
     ["entangled", "dual-lfo", "irrational", "weave"]),

    ("Rise and Width", {
        "sky_macroRise": 0.75, "sky_macroWidth": 0.75,
        "sky_pitchEnvAmount": 7.0, "sky_pitchEnvDecay": 0.5,
        "sky_unisonCount": 5.0, "sky_unisonDetune": 0.35,
        "sky_chorusMix": 0.4, "sky_chorusDepth": 0.5,
        "sky_shimmerMix": 0.3,
    }, (0.6, 0.45, 0.45, 0.6, 0.4, 0.1),
     "RISE and WIDTH macros coupled. Ascension expands the stereo field.",
     ["entangled", "rise-width", "dual-macro", "expanding"]),

    ("Glow and Air", {
        "sky_macroGlow": 0.8, "sky_macroAir": 0.75,
        "sky_shimmerMix": 0.5, "sky_shimmerSize": 0.8,
        "sky_shimmerFeedback": 0.6,
        "sky_filterCutoff": 7000.0, "sky_filterHP": 300.0,
        "sky_attack": 1.0, "sky_release": 3.5,
    }, (0.7, 0.4, 0.15, 0.3, 0.8, 0.0),
     "GLOW and AIR macros coupled. Radiant tails in open sky.",
     ["entangled", "glow-air", "dual-macro", "radiant"]),

    ("S and H Scatter", {
        "sky_lfo2Rate": 4.0, "sky_lfo2Depth": 0.25, "sky_lfo2Shape": LFO_SNH,
        "sky_modSlot1Src": 4.0, "sky_modSlot1Dst": 1.0, "sky_modSlot1Amt": 0.4,
        "sky_shimmerMix": 0.3, "sky_chorusMix": 0.25,
        "sky_filterCutoff": 4000.0,
    }, (0.5, 0.45, 0.5, 0.3, 0.35, 0.15),
     "Sample-and-hold LFO scatters the filter. Random stepping modulation.",
     ["entangled", "sample-hold", "random", "scatter"]),

    ("Pitch Entangle", {
        "sky_pitchEnvAmount": 18.0, "sky_pitchEnvDecay": 0.8,
        "sky_macroRise": 0.85,
        "sky_filterCutoff": 3000.0, "sky_filterEnvAmount": 0.6,
        "sky_shimmerMix": 0.35,
        "sky_unisonCount": 3.0,
    }, (0.55, 0.45, 0.4, 0.4, 0.35, 0.2),
     "Extreme pitch envelope: 18 semitones up. RISE macro pushes further.",
     ["entangled", "pitch", "extreme", "rise"]),

    ("Sub Entangle", {
        "sky_subLevel": 0.7, "sky_subWave": SUB_SQUARE,
        "sky_filterCutoff": 2000.0, "sky_filterReso": 0.35,
        "sky_shimmerMix": 0.3, "sky_shimmerSize": 0.6,
        "sky_macroRise": 0.3,
        "sky_lfo1Rate": 0.04, "sky_lfo1Depth": 0.25,
    }, (0.4, 0.65, 0.35, 0.45, 0.4, 0.1),
     "Heavy sub oscillator grounding the shimmer. LFO breathes the filter.",
     ["entangled", "sub", "heavy", "grounded"]),

    ("Saw Lfo Vibrato", {
        "sky_lfo2Rate": 5.5, "sky_lfo2Depth": 0.15, "sky_lfo2Shape": LFO_SINE,
        "sky_modSlot1Src": 3.0, "sky_modSlot1Dst": 0.0, "sky_modSlot1Amt": 0.3,
        "sky_shimmerMix": 0.25, "sky_chorusMix": 0.2,
        "sky_attack": 0.3,
    }, (0.5, 0.5, 0.4, 0.25, 0.3, 0.05),
     "LFO2 as vibrato via mod matrix. Musical pitch wobble.",
     ["entangled", "vibrato", "lfo", "musical"]),

    ("Full Entangle", {
        "sky_macroRise": 0.7, "sky_macroWidth": 0.7,
        "sky_macroGlow": 0.7, "sky_macroAir": 0.7,
        "sky_modSlot1Src": 2.0, "sky_modSlot1Dst": 3.0, "sky_modSlot1Amt": 0.4,
        "sky_modSlot2Src": 1.0, "sky_modSlot2Dst": 1.0, "sky_modSlot2Amt": 0.5,
        "sky_unisonCount": 5.0, "sky_unisonDetune": 0.35,
        "sky_shimmerMix": 0.45, "sky_shimmerSize": 0.7,
        "sky_shimmerFeedback": 0.5,
        "sky_chorusMix": 0.35,
        "sky_lfo1Rate": 0.04, "sky_lfo1Depth": 0.25,
        "sky_attack": 0.5, "sky_release": 2.5,
    }, (0.65, 0.5, 0.5, 0.6, 0.65, 0.1),
     "Everything entangled. All macros, both mod slots, dual LFO, full unison.",
     ["entangled", "full", "maximum", "showcase"]),

    ("Bipolar Macro", {
        "sky_macroRise": 0.1, "sky_macroWidth": 0.9,
        "sky_macroGlow": 0.1, "sky_macroAir": 0.9,
        "sky_shimmerMix": 0.35, "sky_shimmerSize": 0.6,
        "sky_chorusMix": 0.3,
        "sky_unisonCount": 5.0, "sky_unisonDetune": 0.3,
        "sky_filterCutoff": 4000.0,
        "sky_attack": 0.5, "sky_release": 2.0,
    }, (0.6, 0.45, 0.35, 0.55, 0.55, 0.05),
     "Macros oppose each other. RISE low + AIR high. WIDTH high + GLOW low.",
     ["entangled", "bipolar", "opposing", "macros"]),
]


# ══════════════════════════════════════════════════════════════════════════════
# PRISM (25) — Genre production: trance, EDM, pop, synthwave, progressive
# ══════════════════════════════════════════════════════════════════════════════

PRISM = [
    # --- TRANCE (8) ---
    ("Anthem Pad", {
        "sky_sawSpread": 0.5, "sky_sawMix": 0.85,
        "sky_unisonCount": 5.0, "sky_unisonDetune": 0.3, "sky_unisonSpread": 0.7,
        "sky_attack": 1.5, "sky_release": 2.5, "sky_sustain": 0.8,
        "sky_filterCutoff": 4000.0, "sky_filterEnvAmount": 0.4,
        "sky_shimmerMix": 0.4, "sky_shimmerSize": 0.65,
        "sky_shimmerFeedback": 0.45,
        "sky_chorusMix": 0.3, "sky_chorusDepth": 0.4,
    }, (0.6, 0.55, 0.35, 0.6, 0.55, 0.05),
     "The classic euphoric trance pad. Five unison voices, warm shimmer.",
     ["trance", "anthem", "pad", "euphoric"]),

    ("Trance Lead", {
        "sky_sawSpread": 0.35,
        "sky_attack": 0.005, "sky_sustain": 0.65, "sky_release": 0.4,
        "sky_filterCutoff": 7000.0, "sky_filterEnvAmount": 0.5,
        "sky_shimmerMix": 0.2, "sky_shimmerSize": 0.5,
        "sky_unisonCount": 3.0, "sky_unisonDetune": 0.2,
        "sky_chorusMix": 0.2,
    }, (0.65, 0.45, 0.15, 0.35, 0.3, 0.15),
     "Bright, focused trance lead. Three voices, filter envelope attack.",
     ["trance", "lead", "bright", "focused"]),

    ("Supersaw Stack", {
        "sky_sawSpread": 0.6, "sky_sawMix": 0.9,
        "sky_unisonCount": 7.0, "sky_unisonDetune": 0.4, "sky_unisonSpread": 0.8,
        "sky_filterCutoff": 6000.0, "sky_filterReso": 0.15,
        "sky_shimmerMix": 0.15, "sky_chorusMix": 0.2,
        "sky_attack": 0.01, "sky_release": 0.5,
        "sky_level": 0.55,
    }, (0.65, 0.45, 0.15, 0.85, 0.2, 0.2),
     "Maximum supersaw. Seven unison, seven internal saws. 49 voices total.",
     ["trance", "supersaw", "massive", "stack"]),

    ("Breakdown Wash", {
        "sky_attack": 3.0, "sky_release": 5.0, "sky_sustain": 0.85,
        "sky_filterCutoff": 3000.0,
        "sky_shimmerMix": 0.55, "sky_shimmerSize": 0.8,
        "sky_shimmerFeedback": 0.55,
        "sky_chorusMix": 0.35,
        "sky_unisonCount": 5.0, "sky_unisonSpread": 0.7,
        "sky_macroGlow": 0.7, "sky_macroAir": 0.6,
    }, (0.5, 0.6, 0.25, 0.5, 0.75, 0.0),
     "Trance breakdown washer. Ultra-slow attack swells into shimmering infinity.",
     ["trance", "breakdown", "wash", "swell"]),

    ("Uplifting Riser", {
        "sky_pitchEnvAmount": 12.0, "sky_pitchEnvDecay": 2.0,
        "sky_macroRise": 0.85,
        "sky_filterCutoff": 2000.0, "sky_filterEnvAmount": 0.7,
        "sky_shimmerMix": 0.4, "sky_shimmerSize": 0.6,
        "sky_unisonCount": 5.0, "sky_unisonDetune": 0.35,
        "sky_attack": 2.0, "sky_release": 1.0,
    }, (0.6, 0.45, 0.45, 0.55, 0.45, 0.15),
     "Slow pitch rise over two seconds. The moment before the drop.",
     ["trance", "riser", "uplifting", "build"]),

    ("Gated Trance", {
        "sky_attack": 0.001, "sky_decay": 0.15, "sky_sustain": 0.0,
        "sky_release": 0.1,
        "sky_sawSpread": 0.45,
        "sky_filterCutoff": 6000.0, "sky_filterEnvAmount": 0.6,
        "sky_unisonCount": 3.0, "sky_unisonDetune": 0.25,
        "sky_shimmerMix": 0.2,
    }, (0.6, 0.4, 0.15, 0.4, 0.15, 0.3),
     "Tight gated supersaw. Zero sustain for rhythmic chord stabs.",
     ["trance", "gated", "stab", "rhythmic"]),

    ("Festival Chords", {
        "sky_sawSpread": 0.5,
        "sky_unisonCount": 5.0, "sky_unisonDetune": 0.3, "sky_unisonSpread": 0.65,
        "sky_attack": 0.01, "sky_decay": 0.5, "sky_sustain": 0.5, "sky_release": 0.8,
        "sky_filterCutoff": 5000.0, "sky_filterEnvAmount": 0.45,
        "sky_shimmerMix": 0.25, "sky_chorusMix": 0.25,
    }, (0.6, 0.5, 0.2, 0.55, 0.3, 0.15),
     "Bright stabbing chords with sustain tail. The festival moment.",
     ["trance", "festival", "chords", "bright"]),

    ("Trancegate Pad", {
        "sky_lfo2Rate": 8.0, "sky_lfo2Depth": 0.4, "sky_lfo2Shape": LFO_SQUARE,
        "sky_modSlot1Src": 3.0, "sky_modSlot1Dst": 4.0, "sky_modSlot1Amt": 0.7,
        "sky_attack": 0.3, "sky_sustain": 0.8, "sky_release": 1.0,
        "sky_shimmerMix": 0.3, "sky_chorusMix": 0.3,
        "sky_unisonCount": 3.0,
    }, (0.55, 0.5, 0.6, 0.4, 0.4, 0.1),
     "Square LFO gates the pad rhythmically. Classic trancegate effect.",
     ["trance", "gate", "rhythmic", "lfo"]),

    # --- EDM / POP (8) ---
    ("Pop Shimmer", {
        "sky_attack": 0.01, "sky_decay": 0.6, "sky_sustain": 0.5, "sky_release": 0.6,
        "sky_filterCutoff": 5500.0, "sky_filterEnvAmount": 0.35,
        "sky_shimmerMix": 0.25, "sky_shimmerSize": 0.5,
        "sky_chorusMix": 0.3,
        "sky_unisonCount": 3.0, "sky_unisonDetune": 0.2,
    }, (0.6, 0.5, 0.2, 0.35, 0.35, 0.05),
     "Bright polished shimmer for pop productions. Clean and radio-ready.",
     ["pop", "shimmer", "clean", "radio"]),

    ("EDM Pluck", {
        "sky_attack": 0.001, "sky_decay": 0.25, "sky_sustain": 0.0, "sky_release": 0.2,
        "sky_filterCutoff": 8000.0, "sky_filterEnvAmount": 0.7,
        "sky_sawSpread": 0.4,
        "sky_shimmerMix": 0.15, "sky_unisonCount": 3.0,
    }, (0.7, 0.35, 0.1, 0.35, 0.15, 0.2),
     "Bright, snappy pluck for melodic EDM. Fast decay, high filter envelope.",
     ["edm", "pluck", "bright", "snappy"]),

    ("Future Bass Wash", {
        "sky_attack": 0.5, "sky_release": 2.0, "sky_sustain": 0.7,
        "sky_sawSpread": 0.5,
        "sky_filterCutoff": 3500.0, "sky_filterReso": 0.2,
        "sky_shimmerMix": 0.35, "sky_shimmerSize": 0.6,
        "sky_chorusMix": 0.4, "sky_chorusDepth": 0.5,
        "sky_unisonCount": 5.0, "sky_unisonDetune": 0.3,
        "sky_unisonSpread": 0.7,
    }, (0.5, 0.55, 0.35, 0.55, 0.5, 0.05),
     "Wide, warm, chorused. The future bass pad staple.",
     ["edm", "future-bass", "wash", "wide"]),

    ("Sidechain Pad", {
        "sky_attack": 0.05, "sky_decay": 0.3, "sky_sustain": 0.6, "sky_release": 0.4,
        "sky_filterCutoff": 4000.0,
        "sky_shimmerMix": 0.25, "sky_chorusMix": 0.3,
        "sky_unisonCount": 3.0, "sky_unisonDetune": 0.2,
        "sky_lfo2Rate": 4.0, "sky_lfo2Depth": 0.3, "sky_lfo2Shape": LFO_SAW,
    }, (0.5, 0.5, 0.5, 0.35, 0.3, 0.1),
     "Saw LFO pumps the pad like sidechain compression. Rhythmic duck.",
     ["edm", "sidechain", "pump", "rhythmic"]),

    ("Bright Stab", {
        "sky_attack": 0.001, "sky_decay": 0.12, "sky_sustain": 0.0,
        "sky_filterCutoff": 10000.0, "sky_filterEnvAmount": 0.5,
        "sky_sawSpread": 0.5,
        "sky_unisonCount": 5.0, "sky_unisonDetune": 0.3,
        "sky_shimmerMix": 0.1,
    }, (0.75, 0.35, 0.1, 0.55, 0.1, 0.3),
     "Ultra-short bright stab. Five unison voices for maximum impact.",
     ["edm", "stab", "bright", "impact"]),

    ("Vocal Chord", {
        "sky_filterType": FILT_BP,
        "sky_filterCutoff": 2500.0, "sky_filterReso": 0.4,
        "sky_filterEnvAmount": 0.3,
        "sky_attack": 0.3, "sky_release": 1.0,
        "sky_shimmerMix": 0.3, "sky_shimmerSize": 0.5,
        "sky_chorusMix": 0.3,
        "sky_unisonCount": 3.0,
    }, (0.45, 0.55, 0.2, 0.35, 0.4, 0.05),
     "Bandpass filter creates vowel-like quality. Vocal synth territory.",
     ["pop", "vocal", "bandpass", "chord"]),

    ("Crystal Keys", {
        "sky_attack": 0.001, "sky_decay": 0.8, "sky_sustain": 0.3, "sky_release": 0.6,
        "sky_filterCutoff": 8000.0,
        "sky_shimmerMix": 0.3, "sky_shimmerSize": 0.5,
        "sky_shimmerOctave": 0.8,
        "sky_chorusMix": 0.2,
        "sky_unisonCount": 3.0, "sky_unisonDetune": 0.15,
    }, (0.7, 0.4, 0.1, 0.3, 0.35, 0.05),
     "Bright percussive keys with octave shimmer. Crystalline and musical.",
     ["pop", "keys", "crystal", "bright"]),

    ("Stadium Pad", {
        "sky_sawSpread": 0.55, "sky_sawMix": 0.85,
        "sky_unisonCount": 7.0, "sky_unisonDetune": 0.35, "sky_unisonSpread": 0.75,
        "sky_attack": 0.8, "sky_release": 2.0, "sky_sustain": 0.8,
        "sky_filterCutoff": 4500.0,
        "sky_shimmerMix": 0.35, "sky_shimmerSize": 0.65,
        "sky_chorusMix": 0.3,
        "sky_macroWidth": 0.65,
        "sky_level": 0.55,
    }, (0.6, 0.5, 0.3, 0.8, 0.5, 0.1),
     "Seven unison voices for arena-scale chords. Massive and uplifting.",
     ["edm", "stadium", "massive", "chords"]),

    # --- SYNTHWAVE (5) ---
    ("Retro Saw Lead", {
        "sky_sawSpread": 0.25,
        "sky_attack": 0.005, "sky_sustain": 0.6, "sky_release": 0.3,
        "sky_filterCutoff": 5000.0, "sky_filterReso": 0.25,
        "sky_filterEnvAmount": 0.5,
        "sky_chorusMix": 0.35, "sky_chorusRate": 0.6,
        "sky_shimmerMix": 0.1,
    }, (0.6, 0.5, 0.3, 0.2, 0.25, 0.15),
     "Classic synthwave saw lead. Chorus adds analog warmth.",
     ["synthwave", "lead", "retro", "saw"]),

    ("Neon Pad", {
        "sky_sawSpread": 0.4,
        "sky_attack": 0.5, "sky_release": 1.5, "sky_sustain": 0.75,
        "sky_filterCutoff": 3500.0, "sky_filterReso": 0.2,
        "sky_chorusMix": 0.4, "sky_chorusDepth": 0.5,
        "sky_shimmerMix": 0.2, "sky_shimmerSize": 0.5,
        "sky_unisonCount": 3.0, "sky_unisonDetune": 0.2,
    }, (0.5, 0.6, 0.35, 0.35, 0.35, 0.05),
     "Warm detuned pad with heavy chorus. Late-night neon aesthetic.",
     ["synthwave", "pad", "neon", "warm"]),

    ("Drive Lead", {
        "sky_sawSpread": 0.3,
        "sky_attack": 0.005, "sky_sustain": 0.7,
        "sky_filterCutoff": 6000.0, "sky_filterEnvAmount": 0.4,
        "sky_chorusMix": 0.25,
        "sky_unisonCount": 3.0, "sky_unisonDetune": 0.2,
        "sky_shimmerMix": 0.15,
        "sky_pitchEnvAmount": 2.0, "sky_pitchEnvDecay": 0.15,
    }, (0.6, 0.5, 0.2, 0.3, 0.2, 0.2),
     "Bright lead with slight pitch attack. Outrun energy.",
     ["synthwave", "lead", "drive", "outrun"]),

    ("Sunset Brass", {
        "sky_sawSpread": 0.35,
        "sky_attack": 0.08, "sky_sustain": 0.7, "sky_release": 0.5,
        "sky_filterCutoff": 3000.0, "sky_filterReso": 0.3,
        "sky_filterEnvAmount": 0.6,
        "sky_chorusMix": 0.3, "sky_chorusDepth": 0.35,
        "sky_shimmerMix": 0.15,
        "sky_subLevel": 0.2,
    }, (0.5, 0.6, 0.25, 0.3, 0.25, 0.15),
     "Detuned saw brass with filter envelope. Warm sunset energy.",
     ["synthwave", "brass", "sunset", "warm"]),

    ("Starfield Arp", {
        "sky_attack": 0.001, "sky_decay": 0.3, "sky_sustain": 0.1, "sky_release": 0.4,
        "sky_filterCutoff": 6000.0, "sky_filterEnvAmount": 0.5,
        "sky_shimmerMix": 0.25, "sky_shimmerSize": 0.5,
        "sky_chorusMix": 0.2,
    }, (0.6, 0.45, 0.2, 0.2, 0.3, 0.1),
     "Short plucky arp tone with shimmer tail. Stars blinking in sequence.",
     ["synthwave", "arp", "pluck", "stars"]),

    # --- PROGRESSIVE (4) ---
    ("Progressive Pad", {
        "sky_attack": 1.0, "sky_release": 2.5, "sky_sustain": 0.75,
        "sky_sawSpread": 0.45,
        "sky_filterCutoff": 3500.0, "sky_filterReso": 0.2,
        "sky_shimmerMix": 0.35, "sky_shimmerSize": 0.6,
        "sky_chorusMix": 0.3,
        "sky_unisonCount": 3.0, "sky_unisonDetune": 0.2,
        "sky_lfo1Rate": 0.05, "sky_lfo1Depth": 0.2,
    }, (0.5, 0.55, 0.4, 0.4, 0.5, 0.05),
     "Breathing prog pad. LFO slowly opens and closes the filter.",
     ["progressive", "pad", "breathing", "evolving"]),

    ("Melodic House", {
        "sky_attack": 0.01, "sky_decay": 0.5, "sky_sustain": 0.4, "sky_release": 0.6,
        "sky_filterCutoff": 5000.0, "sky_filterEnvAmount": 0.45,
        "sky_shimmerMix": 0.2, "sky_shimmerSize": 0.45,
        "sky_chorusMix": 0.25,
        "sky_unisonCount": 3.0, "sky_unisonDetune": 0.2,
    }, (0.55, 0.5, 0.2, 0.35, 0.3, 0.1),
     "Clean melodic house chord tone. Restrained shimmer keeps it tight.",
     ["progressive", "house", "melodic", "clean"]),

    ("Euphoria Drop", {
        "sky_sawSpread": 0.5,
        "sky_unisonCount": 7.0, "sky_unisonDetune": 0.35, "sky_unisonSpread": 0.75,
        "sky_attack": 0.001, "sky_decay": 0.3, "sky_sustain": 0.6, "sky_release": 0.5,
        "sky_filterCutoff": 8000.0, "sky_filterEnvAmount": 0.6,
        "sky_shimmerMix": 0.2, "sky_chorusMix": 0.2,
        "sky_macroWidth": 0.65,
        "sky_level": 0.5,
    }, (0.7, 0.4, 0.15, 0.8, 0.2, 0.25),
     "The moment the drop hits. Maximum unison, instant attack, bright filter.",
     ["edm", "drop", "euphoria", "impact"]),

    ("Ambient House", {
        "sky_attack": 1.0, "sky_release": 2.0, "sky_sustain": 0.7,
        "sky_filterCutoff": 3000.0, "sky_filterReso": 0.15,
        "sky_shimmerMix": 0.35, "sky_shimmerSize": 0.6,
        "sky_shimmerDamping": 0.35,
        "sky_chorusMix": 0.3, "sky_chorusDepth": 0.4,
        "sky_unisonCount": 3.0, "sky_unisonDetune": 0.2,
        "sky_lfo1Rate": 0.04, "sky_lfo1Depth": 0.15,
    }, (0.45, 0.6, 0.35, 0.35, 0.5, 0.0),
     "Soft pad for ambient house. Damped shimmer keeps it grounded.",
     ["progressive", "ambient-house", "pad", "soft"]),
]


# ══════════════════════════════════════════════════════════════════════════════
# FLUX (20) — Movement, arps, evolving textures, morphing pads
# ══════════════════════════════════════════════════════════════════════════════

FLUX = [
    ("Shimmer Pulse", {
        "sky_lfo1Rate": 0.1, "sky_lfo1Depth": 0.35,
        "sky_shimmerMix": 0.4, "sky_shimmerSize": 0.6,
        "sky_shimmerFeedback": 0.45,
        "sky_chorusMix": 0.25,
        "sky_attack": 0.3, "sky_release": 1.5,
    }, (0.55, 0.5, 0.5, 0.3, 0.5, 0.0),
     "LFO pulses the shimmer. Rhythmic breathing of light.",
     ["flux", "pulse", "shimmer", "lfo"]),

    ("Chorus Swirl", {
        "sky_chorusRate": 1.2, "sky_chorusDepth": 0.7, "sky_chorusMix": 0.5,
        "sky_shimmerMix": 0.2,
        "sky_unisonCount": 3.0, "sky_unisonDetune": 0.2,
        "sky_attack": 0.3, "sky_release": 1.0,
    }, (0.5, 0.55, 0.65, 0.35, 0.35, 0.0),
     "Fast deep chorus creates swirling stereo motion. Hypnotic.",
     ["flux", "chorus", "swirl", "motion"]),

    ("Filter Dance", {
        "sky_lfo1Rate": 0.15, "sky_lfo1Depth": 0.4,
        "sky_lfo2Rate": 0.37, "sky_lfo2Depth": 0.2,
        "sky_filterCutoff": 3000.0, "sky_filterReso": 0.3,
        "sky_shimmerMix": 0.25, "sky_chorusMix": 0.2,
        "sky_attack": 0.3,
    }, (0.45, 0.5, 0.6, 0.25, 0.3, 0.05),
     "Two LFOs at different rates dance the filter open and closed.",
     ["flux", "filter", "dance", "dual-lfo"]),

    ("Tremolo Sky", {
        "sky_lfo2Rate": 6.0, "sky_lfo2Depth": 0.35, "sky_lfo2Shape": LFO_SINE,
        "sky_modSlot1Src": 3.0, "sky_modSlot1Dst": 4.0, "sky_modSlot1Amt": 0.6,
        "sky_shimmerMix": 0.3, "sky_chorusMix": 0.2,
        "sky_attack": 0.3, "sky_release": 1.0,
    }, (0.5, 0.5, 0.6, 0.3, 0.3, 0.05),
     "Fast LFO2 creates tremolo through mod matrix. Flickering brightness.",
     ["flux", "tremolo", "fast-lfo", "flicker"]),

    ("Rise and Fall", {
        "sky_pitchEnvAmount": 8.0, "sky_pitchEnvDecay": 1.5,
        "sky_filterCutoff": 2000.0, "sky_filterEnvAmount": 0.6,
        "sky_shimmerMix": 0.35, "sky_shimmerSize": 0.6,
        "sky_attack": 1.0, "sky_release": 2.0,
        "sky_macroRise": 0.7,
    }, (0.55, 0.5, 0.45, 0.3, 0.45, 0.05),
     "Pitch sweeps up while filter opens. A single arc of ascension.",
     ["flux", "rise", "fall", "arc"]),

    ("Stutter Saw", {
        "sky_lfo2Rate": 12.0, "sky_lfo2Depth": 0.4, "sky_lfo2Shape": LFO_SQUARE,
        "sky_modSlot1Src": 3.0, "sky_modSlot1Dst": 4.0, "sky_modSlot1Amt": 0.8,
        "sky_shimmerMix": 0.15, "sky_chorusMix": 0.2,
        "sky_sawSpread": 0.4,
    }, (0.55, 0.4, 0.7, 0.3, 0.15, 0.25),
     "Fast square LFO chops the signal. Stuttered supersaw.",
     ["flux", "stutter", "rhythmic", "square-lfo"]),

    ("Slow Morph", {
        "sky_lfo1Rate": 0.02, "sky_lfo1Depth": 0.4,
        "sky_filterCutoff": 2500.0, "sky_filterReso": 0.25,
        "sky_shimmerMix": 0.4, "sky_shimmerSize": 0.65,
        "sky_shimmerFeedback": 0.45,
        "sky_chorusMix": 0.3,
        "sky_attack": 1.0, "sky_sustain": 0.8, "sky_release": 2.5,
    }, (0.45, 0.6, 0.5, 0.3, 0.5, 0.0),
     "Ultra-slow LFO morphs between bright and dark over 50 seconds.",
     ["flux", "morph", "slow", "evolving"]),

    ("Pulsing Width", {
        "sky_macroWidth": 0.3,
        "sky_lfo1Rate": 0.08, "sky_lfo1Depth": 0.3,
        "sky_chorusMix": 0.4, "sky_chorusDepth": 0.5,
        "sky_unisonCount": 5.0, "sky_unisonDetune": 0.3,
        "sky_shimmerMix": 0.2,
        "sky_attack": 0.5,
    }, (0.5, 0.5, 0.55, 0.5, 0.35, 0.0),
     "LFO breathes the stereo width. Sound narrows and expands.",
     ["flux", "width", "pulsing", "stereo"]),

    ("Ascending Fifths", {
        "sky_shimmerMix": 0.5, "sky_shimmerSize": 0.7,
        "sky_shimmerFeedback": 0.6, "sky_shimmerOctave": 0.2,
        "sky_macroGlow": 0.75,
        "sky_attack": 0.5, "sky_release": 3.0,
    }, (0.5, 0.5, 0.25, 0.3, 0.65, 0.0),
     "Fifth-heavy shimmer with high feedback. Harmonics build in perfect fifths.",
     ["flux", "fifths", "ascending", "harmonic"]),

    ("Detune Drift", {
        "sky_sawSpread": 0.7,
        "sky_unisonCount": 5.0, "sky_unisonDetune": 0.5, "sky_unisonSpread": 0.7,
        "sky_lfo1Rate": 0.03, "sky_lfo1Depth": 0.2,
        "sky_shimmerMix": 0.25, "sky_chorusMix": 0.3,
        "sky_attack": 0.5, "sky_release": 1.5,
    }, (0.5, 0.5, 0.5, 0.65, 0.35, 0.1),
     "High detune on both saw spread and unison. LFO adds drift. Thick soup.",
     ["flux", "detune", "drift", "thick"]),

    ("Sparkle Decay", {
        "sky_attack": 0.001, "sky_decay": 1.5, "sky_sustain": 0.0, "sky_release": 1.5,
        "sky_filterCutoff": 10000.0, "sky_filterEnvAmount": 0.8,
        "sky_shimmerMix": 0.4, "sky_shimmerSize": 0.6,
        "sky_shimmerOctave": 0.85,
        "sky_chorusMix": 0.2,
    }, (0.75, 0.35, 0.15, 0.25, 0.45, 0.05),
     "Bright attack decays into shimmering sparkle. Octave shimmer catches light.",
     ["flux", "sparkle", "decay", "bright"]),

    ("Wobble Filter", {
        "sky_lfo2Rate": 3.0, "sky_lfo2Depth": 0.3, "sky_lfo2Shape": LFO_TRI,
        "sky_filterCutoff": 2500.0, "sky_filterReso": 0.4,
        "sky_shimmerMix": 0.2, "sky_chorusMix": 0.2,
    }, (0.45, 0.5, 0.55, 0.25, 0.25, 0.15),
     "Triangle LFO wobbles the filter. Dubstep-adjacent modulation.",
     ["flux", "wobble", "filter", "triangle-lfo"]),

    ("Phase Shift", {
        "sky_chorusRate": 2.0, "sky_chorusDepth": 0.6, "sky_chorusMix": 0.45,
        "sky_shimmerMix": 0.15,
        "sky_lfo1Rate": 0.06, "sky_lfo1Depth": 0.15,
        "sky_filterCutoff": 5000.0,
        "sky_attack": 0.2, "sky_release": 1.0,
    }, (0.55, 0.5, 0.55, 0.3, 0.3, 0.0),
     "Fast chorus creates phase-shift-like motion. Rotating speakers.",
     ["flux", "phase", "chorus", "rotation"]),

    ("Arp Shimmer", {
        "sky_attack": 0.001, "sky_decay": 0.35, "sky_sustain": 0.1, "sky_release": 0.5,
        "sky_filterCutoff": 7000.0, "sky_filterEnvAmount": 0.5,
        "sky_shimmerMix": 0.35, "sky_shimmerSize": 0.55,
        "sky_shimmerFeedback": 0.4,
        "sky_chorusMix": 0.2,
    }, (0.6, 0.45, 0.25, 0.25, 0.4, 0.1),
     "Short pluck notes with shimmering tails. Each arp note leaves light.",
     ["flux", "arp", "shimmer", "pluck"]),

    ("Breath Gate", {
        "sky_lfo1Rate": 0.04, "sky_lfo1Depth": 0.3,
        "sky_lfo2Rate": 8.0, "sky_lfo2Depth": 0.25, "sky_lfo2Shape": LFO_SQUARE,
        "sky_modSlot1Src": 3.0, "sky_modSlot1Dst": 4.0, "sky_modSlot1Amt": 0.5,
        "sky_shimmerMix": 0.3, "sky_chorusMix": 0.25,
        "sky_attack": 0.5, "sky_release": 1.5,
    }, (0.5, 0.5, 0.6, 0.3, 0.35, 0.1),
     "Slow breathing LFO under fast square gate. Rhythmic breathing.",
     ["flux", "gate", "breath", "layered-lfo"]),

    ("Cascade Rise", {
        "sky_pitchEnvAmount": 5.0, "sky_pitchEnvDecay": 0.8,
        "sky_shimmerMix": 0.45, "sky_shimmerSize": 0.7,
        "sky_shimmerFeedback": 0.55, "sky_shimmerOctave": 0.7,
        "sky_macroRise": 0.65, "sky_macroGlow": 0.65,
        "sky_attack": 0.3, "sky_release": 2.0,
    }, (0.6, 0.45, 0.4, 0.3, 0.6, 0.05),
     "Pitch rises while shimmer cascades upward. Double ascension.",
     ["flux", "cascade", "rise", "double"]),

    ("Random Walk", {
        "sky_lfo2Rate": 3.0, "sky_lfo2Depth": 0.2, "sky_lfo2Shape": LFO_SNH,
        "sky_modSlot1Src": 4.0, "sky_modSlot1Dst": 1.0, "sky_modSlot1Amt": 0.35,
        "sky_filterCutoff": 4000.0, "sky_filterReso": 0.25,
        "sky_shimmerMix": 0.3, "sky_chorusMix": 0.25,
        "sky_attack": 0.3, "sky_release": 1.0,
    }, (0.5, 0.45, 0.5, 0.3, 0.35, 0.1),
     "Sample-and-hold randomly steps the filter. Unpredictable movement.",
     ["flux", "random", "sample-hold", "walk"]),

    ("Tidal Motion", {
        "sky_lfo1Rate": 0.025, "sky_lfo1Depth": 0.35,
        "sky_filterCutoff": 3000.0, "sky_filterReso": 0.2,
        "sky_shimmerMix": 0.35, "sky_shimmerSize": 0.6,
        "sky_chorusMix": 0.3, "sky_chorusDepth": 0.45,
        "sky_subLevel": 0.3,
        "sky_attack": 1.0, "sky_release": 2.0,
    }, (0.45, 0.6, 0.5, 0.35, 0.45, 0.0),
     "Very slow LFO creates tidal motion. The filter ebbs and flows.",
     ["flux", "tidal", "slow-lfo", "oceanic"]),

    ("Glitch Scatter", {
        "sky_lfo2Rate": 15.0, "sky_lfo2Depth": 0.3, "sky_lfo2Shape": LFO_SNH,
        "sky_modSlot1Src": 4.0, "sky_modSlot1Dst": 1.0, "sky_modSlot1Amt": 0.5,
        "sky_modSlot2Src": 4.0, "sky_modSlot2Dst": 3.0, "sky_modSlot2Amt": 0.3,
        "sky_shimmerMix": 0.2, "sky_chorusMix": 0.15,
        "sky_filterCutoff": 5000.0,
    }, (0.55, 0.4, 0.65, 0.3, 0.25, 0.25),
     "Fast S&H modulates filter and shimmer. Digital glitch energy.",
     ["flux", "glitch", "scatter", "digital"]),

    ("Evolving Stack", {
        "sky_lfo1Rate": 0.03, "sky_lfo1Depth": 0.3,
        "sky_lfo2Rate": 0.11, "sky_lfo2Depth": 0.15, "sky_lfo2Shape": LFO_TRI,
        "sky_sawSpread": 0.5,
        "sky_unisonCount": 5.0, "sky_unisonDetune": 0.3,
        "sky_filterCutoff": 3500.0, "sky_filterReso": 0.2,
        "sky_shimmerMix": 0.35, "sky_shimmerSize": 0.6,
        "sky_chorusMix": 0.3,
        "sky_attack": 0.8, "sky_sustain": 0.8, "sky_release": 2.0,
    }, (0.5, 0.55, 0.55, 0.5, 0.45, 0.0),
     "Two LFOs at prime-ratio rates evolve the stack. Never repeats exactly.",
     ["flux", "evolving", "dual-lfo", "stack"]),
]


# ══════════════════════════════════════════════════════════════════════════════
# AETHER (15) — Experimental, extreme, otherworldly, boundary-pushing
# ══════════════════════════════════════════════════════════════════════════════

AETHER = [
    ("Infinite Ascent", {
        "sky_macroRise": 0.85, "sky_macroGlow": 0.8, "sky_macroAir": 0.8,
        "sky_pitchEnvAmount": 4.0, "sky_pitchEnvDecay": 3.0,
        "sky_attack": 4.0, "sky_release": 5.0, "sky_sustain": 0.9,
        "sky_shimmerMix": 0.6, "sky_shimmerSize": 0.85,
        "sky_shimmerFeedback": 0.65, "sky_shimmerOctave": 0.75,
        "sky_lfo1Rate": 0.01, "sky_lfo1Depth": 0.2,
        "sky_filterCutoff": 5000.0,
        "sky_unisonCount": 5.0, "sky_unisonSpread": 0.7,
        "sky_stereoWidth": 0.85,
    }, (0.7, 0.45, 0.35, 0.5, 0.85, 0.0),
     "Hold a note. Wait. It never stops rising. Shimmer cascades forever.",
     ["aether", "infinite", "ascent", "drone"]),

    ("Ghost Frequency", {
        "sky_filterCutoff": 1000.0, "sky_filterReso": 0.6,
        "sky_filterHP": 800.0,
        "sky_filterType": FILT_BP,
        "sky_shimmerMix": 0.5, "sky_shimmerSize": 0.75,
        "sky_shimmerFeedback": 0.6, "sky_shimmerDamping": 0.5,
        "sky_attack": 1.5, "sky_release": 4.0,
        "sky_lfo1Rate": 0.015, "sky_lfo1Depth": 0.3,
    }, (0.35, 0.4, 0.35, 0.2, 0.65, 0.1),
     "Narrow bandpass with high resonance and damped shimmer. Spectral.",
     ["aether", "ghost", "spectral", "bandpass"]),

    ("Suborbital", {
        "sky_coarseTune": 12.0, "sky_filterHP": 1000.0,
        "sky_filterCutoff": 15000.0,
        "sky_shimmerMix": 0.55, "sky_shimmerSize": 0.8,
        "sky_shimmerFeedback": 0.55, "sky_shimmerOctave": 0.9,
        "sky_attack": 2.0, "sky_release": 5.0,
        "sky_macroAir": 0.9,
        "sky_stereoWidth": 0.9,
    }, (0.9, 0.15, 0.15, 0.2, 0.85, 0.0),
     "Octave up, extreme high-pass. Only harmonics survive. Near-space.",
     ["aether", "suborbital", "extreme-high", "sparse"]),

    ("Dark Matter", {
        "sky_coarseTune": -12.0,
        "sky_filterCutoff": 800.0, "sky_filterReso": 0.4,
        "sky_shimmerMix": 0.3, "sky_shimmerSize": 0.7,
        "sky_shimmerDamping": 0.7, "sky_shimmerOctave": 0.3,
        "sky_subLevel": 0.6, "sky_subWave": SUB_SINE,
        "sky_attack": 2.0, "sky_release": 3.0,
        "sky_lfo1Rate": 0.02, "sky_lfo1Depth": 0.2,
    }, (0.2, 0.7, 0.3, 0.4, 0.5, 0.05),
     "Octave down. Dark and heavy. Shimmer barely visible through density.",
     ["aether", "dark", "sub", "deep"]),

    ("Frozen Light", {
        "sky_attack": 5.0, "sky_release": 5.0, "sky_sustain": 1.0,
        "sky_filterCutoff": 4000.0,
        "sky_shimmerMix": 0.5, "sky_shimmerSize": 0.9,
        "sky_shimmerFeedback": 0.7, "sky_shimmerDamping": 0.1,
        "sky_chorusMix": 0.2, "sky_chorusRate": 0.2,
        "sky_macroGlow": 0.9,
    }, (0.55, 0.5, 0.1, 0.25, 0.9, 0.0),
     "Five-second attack. Maximum shimmer feedback. Sound freezes in place.",
     ["aether", "frozen", "static", "maximum-reverb"]),

    ("Solar Wind", {
        "sky_lfo1Rate": 0.008, "sky_lfo1Depth": 0.4,
        "sky_lfo2Rate": 0.05, "sky_lfo2Depth": 0.15,
        "sky_filterCutoff": 5000.0, "sky_filterHP": 200.0,
        "sky_shimmerMix": 0.45, "sky_shimmerSize": 0.7,
        "sky_shimmerFeedback": 0.5,
        "sky_chorusMix": 0.3, "sky_chorusDepth": 0.5,
        "sky_attack": 1.5, "sky_release": 3.0,
        "sky_stereoWidth": 0.8,
    }, (0.6, 0.4, 0.55, 0.3, 0.6, 0.0),
     "Sub-audible LFO1 at 0.008 Hz. Movement felt but not heard. Solar plasma.",
     ["aether", "solar", "sub-audible", "drift"]),

    ("Resonance Cave", {
        "sky_filterCutoff": 1200.0, "sky_filterReso": 0.7,
        "sky_filterEnvAmount": 0.6,
        "sky_shimmerMix": 0.4, "sky_shimmerSize": 0.75,
        "sky_shimmerFeedback": 0.5,
        "sky_shimmerDamping": 0.4,
        "sky_attack": 0.5, "sky_release": 3.0,
    }, (0.35, 0.45, 0.2, 0.3, 0.6, 0.2),
     "High resonance near self-oscillation. Shimmer diffuses in the cave.",
     ["aether", "resonance", "cave", "self-oscillation"]),

    ("Time Stretch", {
        "sky_attack": 0.001, "sky_decay": 0.1, "sky_sustain": 0.0,
        "sky_shimmerMix": 0.7, "sky_shimmerSize": 0.9,
        "sky_shimmerFeedback": 0.7, "sky_shimmerDamping": 0.15,
        "sky_shimmerOctave": 0.6,
        "sky_release": 5.0,
    }, (0.6, 0.4, 0.1, 0.2, 0.9, 0.05),
     "Instant attack, zero sustain. The shimmer catches and stretches the note.",
     ["aether", "time-stretch", "shimmer-catch", "transient"]),

    ("Particle Cloud", {
        "sky_lfo2Rate": 20.0, "sky_lfo2Depth": 0.15, "sky_lfo2Shape": LFO_SNH,
        "sky_modSlot1Src": 4.0, "sky_modSlot1Dst": 0.0, "sky_modSlot1Amt": 0.2,
        "sky_modSlot2Src": 4.0, "sky_modSlot2Dst": 1.0, "sky_modSlot2Amt": 0.3,
        "sky_shimmerMix": 0.35, "sky_shimmerSize": 0.6,
        "sky_chorusMix": 0.25,
        "sky_unisonCount": 5.0, "sky_unisonDetune": 0.4,
        "sky_attack": 0.5, "sky_release": 2.0,
    }, (0.5, 0.4, 0.6, 0.5, 0.4, 0.15),
     "Fast S&H modulates pitch and filter. Notes dissolve into particle spray.",
     ["aether", "particle", "random", "granular"]),

    ("Event Horizon", {
        "sky_shimmerMix": 0.6, "sky_shimmerSize": 0.95,
        "sky_shimmerFeedback": 0.8, "sky_shimmerDamping": 0.05,
        "sky_shimmerOctave": 0.5,
        "sky_macroGlow": 0.95,
        "sky_filterCutoff": 3000.0,
        "sky_attack": 1.0, "sky_release": 5.0, "sky_sustain": 0.8,
        "sky_level": 0.6,
    }, (0.5, 0.5, 0.15, 0.3, 0.95, 0.0),
     "Near-maximum shimmer feedback. Sound crosses the event horizon. No return.",
     ["aether", "event-horizon", "extreme-feedback", "no-return"]),

    ("Void Hum", {
        "sky_coarseTune": -24.0,
        "sky_filterCutoff": 500.0, "sky_filterReso": 0.35,
        "sky_subLevel": 0.7, "sky_subWave": SUB_TRI,
        "sky_shimmerMix": 0.2, "sky_shimmerSize": 0.6,
        "sky_shimmerDamping": 0.7,
        "sky_attack": 3.0, "sky_sustain": 0.9, "sky_release": 4.0,
        "sky_lfo1Rate": 0.01, "sky_lfo1Depth": 0.15,
    }, (0.15, 0.7, 0.2, 0.35, 0.45, 0.05),
     "Two octaves down. Triangle sub. The hum of empty space.",
     ["aether", "void", "sub", "drone"]),

    ("Prism Shatter", {
        "sky_pitchEnvAmount": 24.0, "sky_pitchEnvDecay": 0.1,
        "sky_attack": 0.001, "sky_decay": 0.3, "sky_sustain": 0.0,
        "sky_filterCutoff": 12000.0, "sky_filterEnvAmount": 0.9,
        "sky_shimmerMix": 0.3, "sky_shimmerSize": 0.5,
        "sky_unisonCount": 7.0, "sky_unisonDetune": 0.5,
        "sky_level": 0.5,
    }, (0.8, 0.25, 0.2, 0.7, 0.25, 0.5),
     "Maximum pitch envelope: 24 semitones, ultra-fast decay. Glass shattering.",
     ["aether", "shatter", "extreme-pitch", "impact"]),

    ("Nebula Breath", {
        "sky_lfo1Rate": 0.008, "sky_lfo1Depth": 0.45,
        "sky_filterCutoff": 2500.0, "sky_filterReso": 0.2,
        "sky_shimmerMix": 0.5, "sky_shimmerSize": 0.75,
        "sky_shimmerFeedback": 0.55,
        "sky_chorusMix": 0.35, "sky_chorusDepth": 0.5,
        "sky_unisonCount": 5.0, "sky_unisonSpread": 0.7,
        "sky_attack": 2.0, "sky_sustain": 0.85, "sky_release": 4.0,
        "sky_stereoWidth": 0.8,
    }, (0.45, 0.6, 0.5, 0.5, 0.65, 0.0),
     "Two-minute LFO cycle. The nebula breathes once, slowly, completely.",
     ["aether", "nebula", "ultra-slow", "cosmic"]),

    ("White Dwarf", {
        "sky_filterCutoff": 15000.0, "sky_filterHP": 1500.0,
        "sky_shimmerMix": 0.5, "sky_shimmerSize": 0.7,
        "sky_shimmerFeedback": 0.5, "sky_shimmerOctave": 1.0,
        "sky_shimmerDamping": 0.0,
        "sky_macroAir": 0.9, "sky_macroGlow": 0.6,
        "sky_attack": 1.0, "sky_release": 3.0,
        "sky_level": 0.5,
    }, (0.9, 0.1, 0.15, 0.2, 0.7, 0.1),
     "Extreme high-pass. Only the highest harmonics remain. Dense and white-hot.",
     ["aether", "white-dwarf", "extreme-hp", "bright"]),

    ("Quantum Foam", {
        "sky_lfo2Rate": 25.0, "sky_lfo2Depth": 0.2, "sky_lfo2Shape": LFO_SNH,
        "sky_modSlot1Src": 4.0, "sky_modSlot1Dst": 0.0, "sky_modSlot1Amt": 0.15,
        "sky_modSlot2Src": 4.0, "sky_modSlot2Dst": 3.0, "sky_modSlot2Amt": 0.2,
        "sky_shimmerMix": 0.4, "sky_shimmerSize": 0.6,
        "sky_shimmerFeedback": 0.45,
        "sky_lfo1Rate": 0.015, "sky_lfo1Depth": 0.25,
        "sky_attack": 0.3, "sky_release": 2.0,
    }, (0.5, 0.4, 0.6, 0.35, 0.45, 0.15),
     "Fast random modulation under slow drift. The texture of spacetime foam.",
     ["aether", "quantum", "foam", "random"]),
]


# ══════════════════════════════════════════════════════════════════════════════
# FAMILY (15) — OPENSKY + 15 different fleet engines
# ══════════════════════════════════════════════════════════════════════════════

FAMILY_DATA = [
    # (partner_engine, preset_name, overrides, dna, desc, tags, coupling_type, amount)
    ("OceanDeep", "Full Column", {
        "sky_shimmerMix": 0.4, "sky_shimmerSize": 0.65,
        "sky_shimmerFeedback": 0.5, "sky_filterCutoff": 5000.0,
        "sky_unisonCount": 5.0, "sky_unisonDetune": 0.3,
        "sky_macroRise": 0.6, "sky_attack": 0.8, "sky_release": 2.0,
    }, (0.6, 0.5, 0.35, 0.5, 0.5, 0.05),
     "The Full Column. Sky above, abyss below. The complete XO_OX water column.",
     ["family", "oceandeep", "full-column", "mythology"], "PitchToPitch", 0.7),

    ("Opal", "Shimmer Grains", {
        "sky_shimmerMix": 0.4, "sky_shimmerSize": 0.6,
        "sky_attack": 0.5, "sky_release": 2.0,
        "sky_chorusMix": 0.3, "sky_unisonCount": 3.0,
    }, (0.55, 0.5, 0.35, 0.4, 0.5, 0.0),
     "Shimmer pads frozen and scattered by Opal's granular engine.",
     ["family", "opal", "granular", "frozen"], "AudioToFM", 0.5),

    ("Orbital", "Ascension Arc", {
        "sky_macroRise": 0.7, "sky_pitchEnvAmount": 6.0,
        "sky_pitchEnvDecay": 0.8,
        "sky_shimmerMix": 0.35, "sky_shimmerSize": 0.6,
        "sky_unisonCount": 3.0, "sky_attack": 0.5,
    }, (0.6, 0.45, 0.4, 0.4, 0.4, 0.1),
     "Orbital's group envelope drives RISE. Synchronized ascension arcs.",
     ["family", "orbital", "ascension", "group-envelope"], "AmpToFilter", 0.6),

    ("Odyssey", "Analog Shimmer", {
        "sky_shimmerMix": 0.3, "sky_shimmerSize": 0.55,
        "sky_chorusMix": 0.35, "sky_chorusDepth": 0.45,
        "sky_filterCutoff": 4000.0,
        "sky_attack": 0.3, "sky_release": 1.5,
    }, (0.55, 0.55, 0.35, 0.3, 0.4, 0.05),
     "Drift engine grounds the shimmer. Analog warmth beneath digital light.",
     ["family", "odyssey", "analog", "warm"], "LFOToPitch", 0.4),

    ("Overdub", "Dub Shimmer", {
        "sky_shimmerMix": 0.35, "sky_shimmerSize": 0.6,
        "sky_shimmerFeedback": 0.45,
        "sky_chorusMix": 0.25,
        "sky_filterCutoff": 3500.0,
        "sky_attack": 0.3, "sky_release": 1.5,
    }, (0.5, 0.55, 0.35, 0.3, 0.45, 0.1),
     "Dub delay and spring reverb beneath the shimmer. Reggae meets sky.",
     ["family", "overdub", "dub", "delay"], "AudioToFM", 0.45),

    ("Organon", "Living Shimmer", {
        "sky_shimmerMix": 0.4, "sky_shimmerSize": 0.65,
        "sky_lfo1Rate": 0.03, "sky_lfo1Depth": 0.25,
        "sky_chorusMix": 0.3,
        "sky_attack": 1.0, "sky_release": 2.5,
    }, (0.5, 0.55, 0.45, 0.35, 0.5, 0.0),
     "Metabolic breathing beneath the shimmer. Organon gives it life.",
     ["family", "organon", "metabolic", "living"], "AmpToFilter", 0.5),

    ("Ouroboros", "Infinite Loop", {
        "sky_shimmerMix": 0.4, "sky_shimmerSize": 0.7,
        "sky_shimmerFeedback": 0.55,
        "sky_filterCutoff": 3000.0, "sky_filterReso": 0.3,
        "sky_lfo1Rate": 0.04, "sky_lfo1Depth": 0.2,
        "sky_attack": 0.5, "sky_release": 2.0,
    }, (0.5, 0.5, 0.4, 0.35, 0.5, 0.15),
     "Strange attractors feed shimmer. Chaos loops through the sky.",
     ["family", "ouroboros", "feedback", "chaos"], "AudioToFM", 0.5),

    ("Overworld", "Chip Shimmer", {
        "sky_shimmerMix": 0.25, "sky_shimmerSize": 0.5,
        "sky_filterCutoff": 6000.0,
        "sky_chorusMix": 0.2,
        "sky_attack": 0.005, "sky_decay": 0.4, "sky_sustain": 0.3,
    }, (0.6, 0.4, 0.2, 0.3, 0.3, 0.1),
     "8-bit meets euphoria. Chip arpeggios through shimmer clouds.",
     ["family", "overworld", "chip", "retro"], "AudioToFM", 0.4),

    ("Onset", "Percussion Sky", {
        "sky_shimmerMix": 0.35, "sky_shimmerSize": 0.55,
        "sky_attack": 0.001, "sky_decay": 0.3, "sky_sustain": 0.0,
        "sky_filterCutoff": 8000.0, "sky_filterEnvAmount": 0.6,
    }, (0.6, 0.35, 0.15, 0.3, 0.35, 0.2),
     "Percussion triggers through shimmer. Each hit trails upward.",
     ["family", "onset", "percussion", "trail"], "AudioToFM", 0.5),

    ("Oblong", "Amber Sky", {
        "sky_shimmerMix": 0.3, "sky_shimmerSize": 0.55,
        "sky_chorusMix": 0.35, "sky_chorusDepth": 0.4,
        "sky_filterCutoff": 4000.0,
        "sky_attack": 0.3, "sky_release": 1.5,
        "sky_unisonCount": 3.0,
    }, (0.5, 0.6, 0.3, 0.35, 0.4, 0.05),
     "Oblong's warm amber tone beneath the shimmer. Comfort and light.",
     ["family", "oblong", "warm", "amber"], "AmpToFilter", 0.45),

    ("Origami", "Folded Light", {
        "sky_shimmerMix": 0.35, "sky_shimmerSize": 0.6,
        "sky_shimmerOctave": 0.7,
        "sky_filterCutoff": 5000.0, "sky_filterEnvAmount": 0.4,
        "sky_attack": 0.01, "sky_release": 1.0,
    }, (0.6, 0.4, 0.2, 0.35, 0.4, 0.2),
     "Origami folds interact with shimmer harmonics. Paper crane in sunlight.",
     ["family", "origami", "fold", "light"], "AudioToFM", 0.5),

    ("Osprey", "Shore Shimmer", {
        "sky_shimmerMix": 0.35, "sky_shimmerSize": 0.6,
        "sky_shimmerDamping": 0.35,
        "sky_chorusMix": 0.3,
        "sky_filterCutoff": 4500.0,
        "sky_attack": 0.5, "sky_release": 1.5,
        "sky_macroAir": 0.6,
    }, (0.55, 0.55, 0.3, 0.3, 0.5, 0.0),
     "Osprey's coastal tones shimmer above the shore. Salt air and sunlight.",
     ["family", "osprey", "shore", "coastal"], "AmpToFilter", 0.45),

    ("Obese", "Fat Shimmer", {
        "sky_shimmerMix": 0.3, "sky_shimmerSize": 0.55,
        "sky_filterCutoff": 3000.0, "sky_filterReso": 0.25,
        "sky_subLevel": 0.4,
        "sky_unisonCount": 5.0, "sky_unisonDetune": 0.35,
        "sky_level": 0.6,
    }, (0.45, 0.6, 0.2, 0.6, 0.35, 0.25),
     "Obese's saturation weights the shimmer. Fat and bright together.",
     ["family", "obese", "fat", "saturated"], "AudioToFM", 0.55),

    ("Ouie", "Duophonic Sky", {
        "sky_shimmerMix": 0.35, "sky_shimmerSize": 0.6,
        "sky_shimmerFeedback": 0.4,
        "sky_chorusMix": 0.25,
        "sky_filterCutoff": 5000.0,
        "sky_attack": 0.3, "sky_release": 1.5,
        "sky_unisonCount": 3.0,
    }, (0.55, 0.5, 0.3, 0.4, 0.45, 0.1),
     "Ouie's two voices interact beneath the shimmer. Duophonic euphoria.",
     ["family", "ouie", "duophonic", "interaction"], "PitchToPitch", 0.5),

    ("Obrix", "Reef Shimmer", {
        "sky_shimmerMix": 0.35, "sky_shimmerSize": 0.6,
        "sky_shimmerFeedback": 0.45,
        "sky_chorusMix": 0.3,
        "sky_filterCutoff": 4000.0, "sky_filterEnvAmount": 0.4,
        "sky_attack": 0.3, "sky_release": 1.5,
        "sky_unisonCount": 3.0, "sky_unisonDetune": 0.2,
    }, (0.55, 0.5, 0.3, 0.4, 0.45, 0.1),
     "Modular bricks glowing with shimmer. Coral reef at golden hour.",
     ["family", "obrix", "reef", "modular"], "AudioToFM", 0.5),
]

FAMILY_PRESETS = []
for partner, pname, overrides, dna, desc, tags, coupling_type, amount in FAMILY_DATA:
    name = f"Sky x {pname}"
    FAMILY_PRESETS.append(make_preset(
        name, "Family", overrides, dna, desc, tags,
        engines=["OpenSky", partner],
        coupling=[{"engineA": "OpenSky", "engineB": partner,
                   "type": coupling_type, "amount": amount}],
        coupling_intensity="Medium"
    ))


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

    # Prism (25)
    for name, overrides, dna, desc, tags in PRISM:
        save(make_preset(name, "Prism", overrides, dna, desc, tags))
        count += 1

    # Flux (20)
    for name, overrides, dna, desc, tags in FLUX:
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

    print(f"Generated {count} OPENSKY presets across 7 moods")

    # Distribution check
    from collections import Counter
    moods = Counter()
    moods["Foundation"] = len(FOUNDATION)
    moods["Atmosphere"] = len(ATMOSPHERE)
    moods["Entangled"] = len(ENTANGLED)
    moods["Prism"] = len(PRISM)
    moods["Flux"] = len(FLUX)
    moods["Aether"] = len(AETHER)
    moods["Family"] = len(FAMILY_PRESETS)
    for mood, n in sorted(moods.items()):
        print(f"  {mood}: {n}")
    print(f"  TOTAL: {sum(moods.values())}")


if __name__ == "__main__":
    main()
