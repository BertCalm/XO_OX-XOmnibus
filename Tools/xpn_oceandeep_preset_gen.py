#!/usr/bin/env python3
"""OCEANDEEP Factory Preset Generator — 150 presets for the abyssal bass engine.

Generates .xometa JSON files across 7 moods:
  Foundation (30): core bass sounds, init patches, teaching the engine
  Atmosphere (25): dark ambient, underwater environments, slow evolving
  Entangled (20): coupling showcases, designed for cross-engine interaction
  Prism (25): genre production (Techno/Dub/DnB/Trap/Ambient)
  Flux (20): movement, LFO-heavy, filter sweeps, creature motion
  Aether (15): experimental, extreme parameter ranges, alien territory
  Family (15): OCEANDEEP + fleet engine coupling

OceanDeep — Abyssal Bass Synthesizer
  Creature: Anglerfish / Gulper Eel
  Habitat: Hadal Zone (6,000-11,000m)
  Accent: Trench Violet #2D0A4E
  Engine ID: OceanDeep | Param prefix: deep_
  Monophonic. Pure Oscar polarity.
"""

import json
import os

PRESET_DIR = os.path.join(os.path.dirname(__file__), "..", "Presets", "XOceanus")

# ── OCEANDEEP defaults (25 params) ──────────────────────────────────────────
DEFAULTS = {
    # Sub oscillator stack
    "deep_subLevel":       0.7,     # 0-1
    "deep_subOctMix":      0.4,     # 0-1
    # Darkness filter (50-800 Hz LP)
    "deep_filterCutoff":   400.0,   # 50-800
    "deep_filterRes":      0.5,     # 0-0.95
    "deep_velCutoffAmt":   0.4,     # 0-1 (D001)
    # Waveguide body (0=open water, 1=cave, 2=wreck)
    "deep_bodyChar":       0,       # 0/1/2
    "deep_bodyFeedback":   0.45,    # 0-0.9
    "deep_bodyMix":        0.3,     # 0-1
    # Bioluminescent exciter
    "deep_bioRate":        0.08,    # 0.01-0.5
    "deep_bioMix":         0.15,    # 0-1
    "deep_bioBrightness":  800.0,   # 200-4000
    # Hydrostatic compressor
    "deep_pressureAmt":    0.6,     # 0-1
    # Amp envelope
    "deep_ampAtk":         0.01,    # 0.001-0.5
    "deep_ampDec":         0.5,     # 0.1-5.0
    "deep_ampSus":         0.8,     # 0-1
    "deep_ampRel":         1.5,     # 0.2-8.0
    # LFO 1: creature modulation
    "deep_lfo1Rate":       0.15,    # 0.01-2.0
    "deep_lfo1Depth":      0.3,     # 0-1
    # LFO 2: pressure wobble
    "deep_lfo2Rate":       0.05,    # 0.01-0.5
    "deep_lfo2Depth":      0.2,     # 0-1
    # Reverb
    "deep_reverbMix":      0.35,    # 0-1
    # 4 Macros
    "deep_macroPressure":  0.5,     # 0-1
    "deep_macroCreature":  0.3,     # 0-1
    "deep_macroWreck":     0.4,     # 0-1
    "deep_macroAbyss":     0.5,     # 0-1
}

# Body character constants
OPEN_WATER = 0
CAVE       = 1
WRECK      = 2


def make_preset(name, mood, overrides, dna, desc="", tags=None, engines=None,
                coupling=None, coupling_intensity="None"):
    """Build a full .xometa dict from name + param overrides."""
    params = dict(DEFAULTS)
    params.update(overrides)

    preset = {
        "schema_version": 1,
        "name": name,
        "mood": mood,
        "engines": engines or ["OceanDeep"],
        "author": "XO_OX Designs",
        "version": "1.0.0",
        "description": desc,
        "tags": tags or [],
        "macroLabels": ["PRESSURE", "CREATURE", "WRECK", "ABYSS"],
        "couplingIntensity": coupling_intensity,
        "tempo": None,
        "dna": {
            "brightness": dna[0], "warmth": dna[1], "movement": dna[2],
            "density": dna[3], "space": dna[4], "aggression": dna[5]
        },
        "parameters": {"OceanDeep": params},
    }
    if coupling:
        preset["coupling"] = {"pairs": coupling}
    return preset


def save(preset):
    """Write preset to the correct mood directory."""
    mood = preset["mood"]
    safe_name = preset["name"].replace(" ", "_").replace("'", "").replace(",", "")
    filename = f"OceanDeep_{safe_name}.xometa"
    path = os.path.join(PRESET_DIR, mood, filename)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w") as f:
        json.dump(preset, f, indent=2)
    return path


# ══════════════════════════════════════════════════════════════════════════════
# FOUNDATION (30) — core abyssal bass sounds
# ══════════════════════════════════════════════════════════════════════════════

FOUNDATION = [
    # 1. Pure sine sub — the simplest deep sound
    ("Hadal Sine", {
        "deep_subOctMix": 0.0, "deep_bodyMix": 0.0, "deep_bioMix": 0.0,
        "deep_filterCutoff": 800.0, "deep_filterRes": 0.3,
        "deep_pressureAmt": 0.0, "deep_reverbMix": 0.0,
        "deep_lfo1Depth": 0.0, "deep_lfo2Depth": 0.0,
        "deep_macroPressure": 0.0, "deep_macroCreature": 0.0,
        "deep_macroWreck": 0.0, "deep_macroAbyss": 0.0,
    }, (0.1, 0.8, 0.0, 0.1, 0.0, 0.0),
     "A single sine wave at the bottom of the ocean. Pure fundamental. The starting point.",
     ["foundation", "sine", "simple", "sub"]),

    # 2. Sub stack — fundamental + sub octaves
    ("Trench Stack", {
        "deep_subOctMix": 0.6, "deep_bodyMix": 0.0, "deep_bioMix": 0.0,
        "deep_filterCutoff": 600.0, "deep_filterRes": 0.3,
        "deep_pressureAmt": 0.2, "deep_reverbMix": 0.0,
        "deep_lfo1Depth": 0.0, "deep_lfo2Depth": 0.0,
        "deep_macroPressure": 0.0, "deep_macroCreature": 0.0,
        "deep_macroWreck": 0.0, "deep_macroAbyss": 0.0,
    }, (0.1, 0.85, 0.0, 0.2, 0.0, 0.05),
     "Fundamental plus two sub-octaves. The full oscillator stack revealed.",
     ["foundation", "sub", "stack", "octave"]),

    # 3. Darkness filter — demonstrate the LP sweep
    ("Dark Curtain", {
        "deep_filterCutoff": 150.0, "deep_filterRes": 0.6,
        "deep_bodyMix": 0.0, "deep_bioMix": 0.0,
        "deep_pressureAmt": 0.3, "deep_reverbMix": 0.0,
        "deep_lfo1Depth": 0.0, "deep_lfo2Depth": 0.0,
        "deep_macroCreature": 0.0, "deep_macroWreck": 0.0,
    }, (0.05, 0.9, 0.0, 0.15, 0.0, 0.1),
     "Darkness filter at 150 Hz with resonance. Only the deepest frequencies pass.",
     ["foundation", "filter", "dark", "resonant"]),

    # 4. Velocity shapes — D001 demonstration
    ("Velocity Depth", {
        "deep_velCutoffAmt": 0.8, "deep_filterCutoff": 200.0, "deep_filterRes": 0.5,
        "deep_bodyMix": 0.0, "deep_bioMix": 0.0,
        "deep_lfo1Depth": 0.0, "deep_lfo2Depth": 0.0,
        "deep_macroCreature": 0.0, "deep_macroWreck": 0.0,
    }, (0.15, 0.8, 0.0, 0.15, 0.0, 0.1),
     "Play soft: pure darkness. Play hard: filter opens. Velocity shapes the trench.",
     ["foundation", "velocity", "dynamics", "D001"]),

    # 5. Pressure demo — hydrostatic compressor
    ("Crushing Weight", {
        "deep_pressureAmt": 0.9, "deep_subOctMix": 0.5,
        "deep_bodyMix": 0.0, "deep_bioMix": 0.0,
        "deep_filterCutoff": 500.0,
        "deep_lfo1Depth": 0.0, "deep_lfo2Depth": 0.0,
        "deep_macroCreature": 0.0, "deep_macroWreck": 0.0,
    }, (0.1, 0.75, 0.0, 0.2, 0.0, 0.15),
     "Maximum hydrostatic pressure. The signal compresses under the weight of the water.",
     ["foundation", "pressure", "compressor", "heavy"]),

    # 6. Open water body
    ("Open Water", {
        "deep_bodyChar": OPEN_WATER, "deep_bodyMix": 0.5, "deep_bodyFeedback": 0.5,
        "deep_bioMix": 0.0, "deep_filterCutoff": 400.0,
        "deep_lfo1Depth": 0.0, "deep_lfo2Depth": 0.0,
        "deep_macroCreature": 0.0,
    }, (0.15, 0.7, 0.05, 0.25, 0.1, 0.05),
     "Waveguide body in open water mode. Light resonance colors the sub.",
     ["foundation", "body", "waveguide", "open"]),

    # 7. Cave resonance
    ("Cave Tone", {
        "deep_bodyChar": CAVE, "deep_bodyMix": 0.6, "deep_bodyFeedback": 0.6,
        "deep_bioMix": 0.0, "deep_filterCutoff": 350.0, "deep_filterRes": 0.4,
        "deep_lfo1Depth": 0.0, "deep_lfo2Depth": 0.0,
        "deep_macroCreature": 0.0,
    }, (0.15, 0.7, 0.05, 0.3, 0.15, 0.1),
     "Waveguide body in cave mode. Slightly detuned comb resonance like cave reflections.",
     ["foundation", "cave", "resonance", "body"]),

    # 8. Wreck hull
    ("Wreck Hull", {
        "deep_bodyChar": WRECK, "deep_bodyMix": 0.6, "deep_bodyFeedback": 0.7,
        "deep_bioMix": 0.0, "deep_filterCutoff": 350.0, "deep_filterRes": 0.45,
        "deep_lfo1Depth": 0.0, "deep_lfo2Depth": 0.0,
        "deep_macroCreature": 0.0,
    }, (0.2, 0.6, 0.05, 0.35, 0.15, 0.15),
     "Wreck mode: high feedback, allpass diffusion. The resonance of a sunken hull.",
     ["foundation", "wreck", "hull", "metallic"]),

    # 9. Bioluminescent spark
    ("First Light", {
        "deep_bioMix": 0.4, "deep_bioRate": 0.1, "deep_bioBrightness": 1200.0,
        "deep_bodyMix": 0.0, "deep_filterCutoff": 500.0,
        "deep_lfo1Depth": 0.1, "deep_lfo2Depth": 0.0,
    }, (0.25, 0.65, 0.2, 0.2, 0.05, 0.05),
     "The bioluminescent exciter: bandpass noise bursts triggered by creature LFO.",
     ["foundation", "bio", "creature", "light"]),

    # 10. LFO creature modulation
    ("Creature Breath", {
        "deep_lfo1Rate": 0.2, "deep_lfo1Depth": 0.5,
        "deep_bodyMix": 0.2, "deep_bioMix": 0.2,
        "deep_filterCutoff": 350.0,
        "deep_lfo2Depth": 0.0,
    }, (0.15, 0.7, 0.35, 0.2, 0.05, 0.05),
     "LFO1 modulates bio rate and filter cutoff. The creature breathes.",
     ["foundation", "lfo", "creature", "breathing"]),

    # 11. Pressure wobble
    ("Tidal Wobble", {
        "deep_lfo2Rate": 0.08, "deep_lfo2Depth": 0.5,
        "deep_lfo1Depth": 0.0,
        "deep_bodyMix": 0.0, "deep_bioMix": 0.0,
        "deep_filterCutoff": 450.0,
    }, (0.1, 0.75, 0.3, 0.15, 0.0, 0.05),
     "LFO2 creates slow pitch wobble and pressure variation. The tide pulls at you.",
     ["foundation", "lfo", "wobble", "tidal"]),

    # 12. Abyssal reverb
    ("Abyssal Echo", {
        "deep_reverbMix": 0.7, "deep_filterCutoff": 300.0,
        "deep_bodyMix": 0.0, "deep_bioMix": 0.0,
        "deep_lfo1Depth": 0.0, "deep_lfo2Depth": 0.0,
        "deep_macroCreature": 0.0,
    }, (0.1, 0.65, 0.05, 0.2, 0.6, 0.0),
     "Dense dark reverb. Schroeder algorithm optimized for underwater cave reflections.",
     ["foundation", "reverb", "cave", "space"]),

    # 13. Macro PRESSURE
    ("Pressure Sweep", {
        "deep_macroPressure": 0.8, "deep_macroCreature": 0.0,
        "deep_macroWreck": 0.0, "deep_macroAbyss": 0.0,
        "deep_filterCutoff": 400.0,
        "deep_lfo1Depth": 0.1, "deep_lfo2Depth": 0.1,
    }, (0.1, 0.8, 0.1, 0.2, 0.0, 0.1),
     "PRESSURE macro at 80%. Compression depth and sub level increase together.",
     ["foundation", "macro", "pressure", "compression"]),

    # 14. Macro CREATURE
    ("Creature Macro", {
        "deep_macroCreature": 0.8, "deep_macroPressure": 0.0,
        "deep_macroWreck": 0.0, "deep_macroAbyss": 0.0,
        "deep_filterCutoff": 400.0,
    }, (0.2, 0.65, 0.35, 0.2, 0.05, 0.05),
     "CREATURE macro at 80%. Bio exciter mix and rate amplified. Life in the dark.",
     ["foundation", "macro", "creature", "bio"]),

    # 15. Macro WRECK
    ("Wreck Macro", {
        "deep_macroWreck": 0.8, "deep_macroPressure": 0.0,
        "deep_macroCreature": 0.0, "deep_macroAbyss": 0.0,
        "deep_filterCutoff": 400.0,
    }, (0.15, 0.65, 0.1, 0.3, 0.15, 0.15),
     "WRECK macro at 80%. Body mix increases, character shifts toward wreck mode.",
     ["foundation", "macro", "wreck", "body"]),

    # 16. Macro ABYSS
    ("Abyss Macro", {
        "deep_macroAbyss": 0.8, "deep_macroPressure": 0.0,
        "deep_macroCreature": 0.0, "deep_macroWreck": 0.0,
    }, (0.05, 0.7, 0.05, 0.15, 0.5, 0.0),
     "ABYSS macro at 80%. Filter closes, reverb increases. Descending into darkness.",
     ["foundation", "macro", "abyss", "dark"]),

    # 17. All macros engaged
    ("Full Descent", {
        "deep_macroPressure": 0.5, "deep_macroCreature": 0.4,
        "deep_macroWreck": 0.5, "deep_macroAbyss": 0.5,
        "deep_filterCutoff": 400.0,
    }, (0.1, 0.7, 0.25, 0.3, 0.35, 0.1),
     "All four macros active. The full engine architecture working together.",
     ["foundation", "macro", "all", "complete"]),

    # 18. Long attack sub
    ("Rising Leviathan", {
        "deep_ampAtk": 0.4, "deep_ampSus": 0.9, "deep_ampRel": 3.0,
        "deep_filterCutoff": 350.0, "deep_bodyMix": 0.2,
        "deep_pressureAmt": 0.5,
        "deep_lfo1Depth": 0.1, "deep_lfo2Depth": 0.1,
    }, (0.1, 0.75, 0.1, 0.15, 0.1, 0.05),
     "Slow attack bass. The leviathan rises from the deep. Patient and immense.",
     ["foundation", "slow", "attack", "pad"]),

    # 19. Short percussive sub
    ("Depth Charge", {
        "deep_ampAtk": 0.001, "deep_ampDec": 0.3, "deep_ampSus": 0.0, "deep_ampRel": 0.5,
        "deep_subOctMix": 0.7, "deep_filterCutoff": 600.0,
        "deep_pressureAmt": 0.4, "deep_bodyMix": 0.0, "deep_bioMix": 0.0,
        "deep_lfo1Depth": 0.0, "deep_lfo2Depth": 0.0,
        "deep_reverbMix": 0.2,
    }, (0.15, 0.7, 0.05, 0.2, 0.15, 0.25),
     "Fast attack, short decay. A percussive depth charge detonation.",
     ["foundation", "percussive", "short", "impact"]),

    # 20. Sustained drone
    ("Hadal Drone", {
        "deep_ampAtk": 0.1, "deep_ampSus": 0.95, "deep_ampRel": 4.0,
        "deep_subOctMix": 0.5, "deep_filterCutoff": 300.0,
        "deep_bodyMix": 0.3, "deep_bodyChar": CAVE,
        "deep_pressureAmt": 0.6, "deep_reverbMix": 0.4,
        "deep_lfo1Depth": 0.15, "deep_lfo2Depth": 0.15,
    }, (0.08, 0.75, 0.15, 0.25, 0.35, 0.05),
     "Long sustained bass drone with cave body and dark reverb. Hold a note and descend.",
     ["foundation", "drone", "sustain", "ambient"]),

    # 21. Mod wheel expression (D006)
    ("Pressure Control", {
        "deep_filterCutoff": 500.0, "deep_filterRes": 0.5,
        "deep_velCutoffAmt": 0.6,
        "deep_pressureAmt": 0.5, "deep_bodyMix": 0.2,
    }, (0.12, 0.75, 0.1, 0.2, 0.05, 0.1),
     "Mod wheel closes the darkness filter. Aftertouch adds compression. Full expression.",
     ["foundation", "expression", "modwheel", "D006"]),

    # 22. 808-style sub
    ("808 Trench", {
        "deep_ampAtk": 0.001, "deep_ampDec": 1.5, "deep_ampSus": 0.0, "deep_ampRel": 0.8,
        "deep_subOctMix": 0.3, "deep_filterCutoff": 600.0, "deep_filterRes": 0.3,
        "deep_pressureAmt": 0.3, "deep_bodyMix": 0.0, "deep_bioMix": 0.0,
        "deep_lfo1Depth": 0.0, "deep_lfo2Depth": 0.0,
        "deep_reverbMix": 0.1,
    }, (0.15, 0.75, 0.0, 0.15, 0.05, 0.15),
     "808-style sub bass with long decay. The trench floor version of the classic.",
     ["foundation", "808", "bass", "hip-hop"]),

    # 23. Bright sub (filter open)
    ("Bright Trench", {
        "deep_filterCutoff": 800.0, "deep_filterRes": 0.2,
        "deep_subOctMix": 0.3, "deep_pressureAmt": 0.3,
        "deep_bodyMix": 0.0, "deep_bioMix": 0.0,
        "deep_lfo1Depth": 0.0, "deep_lfo2Depth": 0.0,
    }, (0.2, 0.7, 0.0, 0.15, 0.0, 0.05),
     "Darkness filter fully open at 800 Hz. As bright as the trench gets.",
     ["foundation", "bright", "open", "sub"]),

    # 24. Heavy resonance
    ("Resonant Abyss", {
        "deep_filterCutoff": 200.0, "deep_filterRes": 0.85,
        "deep_pressureAmt": 0.5, "deep_subOctMix": 0.4,
        "deep_bodyMix": 0.0, "deep_bioMix": 0.0,
        "deep_lfo1Depth": 0.0, "deep_lfo2Depth": 0.0,
    }, (0.1, 0.6, 0.0, 0.2, 0.0, 0.2),
     "Heavy filter resonance at 200 Hz. The darkness filter sings.",
     ["foundation", "resonant", "filter", "bass"]),

    # 25. Bio + Body combo
    ("Living Wreck", {
        "deep_bodyChar": WRECK, "deep_bodyMix": 0.5, "deep_bodyFeedback": 0.6,
        "deep_bioMix": 0.3, "deep_bioRate": 0.1, "deep_bioBrightness": 1000.0,
        "deep_filterCutoff": 350.0, "deep_lfo1Depth": 0.2,
    }, (0.2, 0.65, 0.2, 0.3, 0.1, 0.1),
     "Wreck hull resonance colonized by bioluminescent creatures. Metal and life.",
     ["foundation", "wreck", "bio", "combined"]),

    # 26. Maximum sub level
    ("Bedrock", {
        "deep_subLevel": 1.0, "deep_subOctMix": 0.8,
        "deep_filterCutoff": 250.0, "deep_filterRes": 0.3,
        "deep_pressureAmt": 0.7, "deep_bodyMix": 0.0, "deep_bioMix": 0.0,
        "deep_lfo1Depth": 0.0, "deep_lfo2Depth": 0.0,
    }, (0.05, 0.9, 0.0, 0.25, 0.0, 0.1),
     "Maximum sub oscillator level with heavy sub-octave mix. Pure geological bass.",
     ["foundation", "sub", "maximum", "heavy"]),

    # 27. Aftertouch expression
    ("Pressure Touch", {
        "deep_filterCutoff": 350.0, "deep_filterRes": 0.5,
        "deep_velCutoffAmt": 0.5, "deep_pressureAmt": 0.4,
        "deep_bodyMix": 0.3, "deep_bodyChar": CAVE,
        "deep_bioMix": 0.1,
    }, (0.1, 0.7, 0.1, 0.2, 0.1, 0.1),
     "Aftertouch adds compression pressure. Push into the keys to feel the weight.",
     ["foundation", "aftertouch", "expression", "pressure"]),

    # 28. Clean deep bass
    ("Abyssal Clean", {
        "deep_subOctMix": 0.2, "deep_filterCutoff": 500.0, "deep_filterRes": 0.2,
        "deep_pressureAmt": 0.2, "deep_bodyMix": 0.0, "deep_bioMix": 0.0,
        "deep_reverbMix": 0.0,
        "deep_lfo1Depth": 0.0, "deep_lfo2Depth": 0.0,
        "deep_macroPressure": 0.0, "deep_macroCreature": 0.0,
        "deep_macroWreck": 0.0, "deep_macroAbyss": 0.0,
    }, (0.15, 0.8, 0.0, 0.1, 0.0, 0.0),
     "Clean sine-based bass. No frills, no effects. The honest deep tone.",
     ["foundation", "clean", "simple", "bass"]),

    # 29. Full cave setup
    ("Stone Cathedral", {
        "deep_bodyChar": CAVE, "deep_bodyMix": 0.7, "deep_bodyFeedback": 0.7,
        "deep_filterCutoff": 300.0, "deep_filterRes": 0.4,
        "deep_subOctMix": 0.5, "deep_pressureAmt": 0.5,
        "deep_reverbMix": 0.5,
        "deep_bioMix": 0.1, "deep_bioBrightness": 600.0,
        "deep_lfo1Depth": 0.15, "deep_lfo2Depth": 0.1,
    }, (0.1, 0.7, 0.15, 0.3, 0.5, 0.05),
     "Underwater cathedral: cave body, reverb, and faint bioluminescence. Sacred and deep.",
     ["foundation", "cathedral", "cave", "reverb"]),

    # 30. Init patch (balanced starting point)
    ("Init Trench", {
        "deep_filterCutoff": 400.0, "deep_filterRes": 0.5,
        "deep_subOctMix": 0.4, "deep_bodyMix": 0.3,
        "deep_bodyChar": OPEN_WATER, "deep_bodyFeedback": 0.45,
        "deep_bioMix": 0.15, "deep_bioRate": 0.08,
        "deep_pressureAmt": 0.6, "deep_reverbMix": 0.35,
    }, (0.12, 0.72, 0.15, 0.2, 0.2, 0.05),
     "The default OceanDeep sound. All systems at default. Start exploring from here.",
     ["foundation", "init", "default", "starting-point"]),
]


# ══════════════════════════════════════════════════════════════════════════════
# ATMOSPHERE (25) — dark ambient, underwater environments
# ══════════════════════════════════════════════════════════════════════════════

ATMOSPHERE = [
    ("Midnight Column", {
        "deep_ampAtk": 0.3, "deep_ampSus": 0.9, "deep_ampRel": 4.0,
        "deep_filterCutoff": 200.0, "deep_filterRes": 0.4,
        "deep_subOctMix": 0.6, "deep_bodyMix": 0.3, "deep_bodyChar": CAVE,
        "deep_reverbMix": 0.6, "deep_pressureAmt": 0.5,
        "deep_lfo1Depth": 0.15, "deep_lfo2Depth": 0.2,
    }, (0.05, 0.7, 0.2, 0.25, 0.6, 0.0),
     "The water column at midnight. Slow-moving sub bass with cave reflections.",
     ["atmosphere", "midnight", "column", "ambient"]),

    ("Benthic Mist", {
        "deep_ampAtk": 0.4, "deep_ampSus": 0.85, "deep_ampRel": 5.0,
        "deep_filterCutoff": 150.0, "deep_filterRes": 0.5,
        "deep_subOctMix": 0.5, "deep_bodyMix": 0.2,
        "deep_bioMix": 0.25, "deep_bioRate": 0.05, "deep_bioBrightness": 600.0,
        "deep_reverbMix": 0.65, "deep_pressureAmt": 0.6,
        "deep_lfo1Depth": 0.2, "deep_lfo2Depth": 0.15,
    }, (0.08, 0.7, 0.25, 0.2, 0.65, 0.0),
     "Bottom-dwelling fog. Faint creature lights pulse through extreme darkness.",
     ["atmosphere", "benthic", "mist", "deep"]),

    ("Ink Cloud", {
        "deep_ampAtk": 0.2, "deep_ampSus": 0.7, "deep_ampRel": 3.0,
        "deep_filterCutoff": 120.0, "deep_filterRes": 0.6,
        "deep_pressureAmt": 0.8, "deep_subOctMix": 0.7,
        "deep_bodyMix": 0.0, "deep_bioMix": 0.0,
        "deep_reverbMix": 0.5,
        "deep_lfo1Depth": 0.0, "deep_lfo2Depth": 0.25,
    }, (0.03, 0.8, 0.15, 0.2, 0.45, 0.05),
     "Maximum darkness: filter at 120 Hz, heavy pressure. An octopus ink cloud obscures everything.",
     ["atmosphere", "ink", "dark", "extreme"]),

    ("Pressure Haze", {
        "deep_ampAtk": 0.3, "deep_ampSus": 0.9, "deep_ampRel": 4.5,
        "deep_filterCutoff": 250.0, "deep_filterRes": 0.35,
        "deep_pressureAmt": 0.8, "deep_subOctMix": 0.5,
        "deep_bodyMix": 0.15, "deep_bodyChar": OPEN_WATER,
        "deep_reverbMix": 0.5,
        "deep_lfo2Depth": 0.3, "deep_lfo2Rate": 0.03,
    }, (0.06, 0.75, 0.2, 0.2, 0.45, 0.05),
     "Heavy compression under slow pressure wobble. Like breathing at extreme depth.",
     ["atmosphere", "pressure", "haze", "slow"]),

    ("Wreck Fog", {
        "deep_ampAtk": 0.35, "deep_ampSus": 0.8, "deep_ampRel": 3.5,
        "deep_filterCutoff": 250.0, "deep_filterRes": 0.4,
        "deep_bodyChar": WRECK, "deep_bodyMix": 0.6, "deep_bodyFeedback": 0.65,
        "deep_bioMix": 0.15, "deep_bioBrightness": 500.0,
        "deep_reverbMix": 0.55,
        "deep_lfo1Depth": 0.2,
    }, (0.1, 0.65, 0.2, 0.3, 0.55, 0.1),
     "A sunken ship dissolving in the dark. Wreck body with bioluminescent visitors.",
     ["atmosphere", "wreck", "fog", "haunted"]),

    ("Trench Drone", {
        "deep_ampAtk": 0.15, "deep_ampSus": 0.95, "deep_ampRel": 6.0,
        "deep_filterCutoff": 180.0, "deep_filterRes": 0.3,
        "deep_subOctMix": 0.7, "deep_pressureAmt": 0.7,
        "deep_bodyMix": 0.2, "deep_bodyChar": CAVE,
        "deep_reverbMix": 0.45,
        "deep_lfo1Depth": 0.1, "deep_lfo2Depth": 0.15,
    }, (0.05, 0.8, 0.1, 0.25, 0.4, 0.05),
     "Sustained sub drone at the trench floor. Nearly subsonic. Felt more than heard.",
     ["atmosphere", "drone", "trench", "subsonic"]),

    ("Bioluminescent Fog", {
        "deep_ampAtk": 0.3, "deep_ampSus": 0.85, "deep_ampRel": 4.0,
        "deep_bioMix": 0.5, "deep_bioRate": 0.04, "deep_bioBrightness": 1500.0,
        "deep_filterCutoff": 300.0, "deep_filterRes": 0.3,
        "deep_bodyMix": 0.15,
        "deep_reverbMix": 0.6,
        "deep_lfo1Depth": 0.3, "deep_lfo1Rate": 0.08,
    }, (0.15, 0.6, 0.3, 0.25, 0.6, 0.0),
     "Dense bioluminescent activity over dark bass. Alien fog at the ocean floor.",
     ["atmosphere", "bio", "fog", "alien"]),

    ("Cold Seep", {
        "deep_ampAtk": 0.2, "deep_ampSus": 0.8, "deep_ampRel": 3.0,
        "deep_filterCutoff": 200.0, "deep_filterRes": 0.55,
        "deep_bodyChar": OPEN_WATER, "deep_bodyMix": 0.4, "deep_bodyFeedback": 0.5,
        "deep_pressureAmt": 0.5,
        "deep_reverbMix": 0.4,
        "deep_lfo2Depth": 0.2, "deep_lfo2Rate": 0.04,
    }, (0.08, 0.7, 0.15, 0.25, 0.4, 0.05),
     "Methane seepage at the ocean floor. Resonant filter and slow pressure variation.",
     ["atmosphere", "seep", "cold", "resonant"]),

    ("Abyssal Stillness", {
        "deep_ampAtk": 0.5, "deep_ampSus": 0.9, "deep_ampRel": 8.0,
        "deep_filterCutoff": 100.0, "deep_filterRes": 0.3,
        "deep_subOctMix": 0.6, "deep_pressureAmt": 0.6,
        "deep_bodyMix": 0.0, "deep_bioMix": 0.0,
        "deep_reverbMix": 0.5,
        "deep_lfo1Depth": 0.05, "deep_lfo2Depth": 0.05,
    }, (0.02, 0.85, 0.05, 0.15, 0.5, 0.0),
     "Near-total stillness. Filter at 100 Hz. Maximum release. The abyss holds still.",
     ["atmosphere", "still", "extreme", "dark"]),

    ("Thermal Vent", {
        "deep_ampAtk": 0.1, "deep_ampSus": 0.85, "deep_ampRel": 3.0,
        "deep_filterCutoff": 350.0, "deep_filterRes": 0.5,
        "deep_bodyChar": OPEN_WATER, "deep_bodyMix": 0.35,
        "deep_bioMix": 0.3, "deep_bioRate": 0.15, "deep_bioBrightness": 2000.0,
        "deep_pressureAmt": 0.5,
        "deep_reverbMix": 0.4,
        "deep_lfo1Depth": 0.25, "deep_lfo1Rate": 0.2,
    }, (0.2, 0.65, 0.3, 0.3, 0.35, 0.1),
     "Hydrothermal vent: warmth rises from the earth. Bio exciter at higher brightness.",
     ["atmosphere", "thermal", "vent", "warm"]),

    ("Manganese Field", {
        "deep_ampAtk": 0.4, "deep_ampSus": 0.9, "deep_ampRel": 5.0,
        "deep_filterCutoff": 180.0, "deep_filterRes": 0.4,
        "deep_subOctMix": 0.5, "deep_pressureAmt": 0.65,
        "deep_bodyChar": CAVE, "deep_bodyMix": 0.25, "deep_bodyFeedback": 0.55,
        "deep_reverbMix": 0.55,
        "deep_lfo2Depth": 0.2, "deep_lfo2Rate": 0.03,
    }, (0.05, 0.75, 0.15, 0.2, 0.55, 0.0),
     "Nodule fields on the abyssal plain. Slow, dark, with cave-like resonance.",
     ["atmosphere", "manganese", "plain", "mineral"]),

    ("Cephalopod Dream", {
        "deep_ampAtk": 0.35, "deep_ampSus": 0.8, "deep_ampRel": 4.0,
        "deep_bioMix": 0.45, "deep_bioRate": 0.06, "deep_bioBrightness": 1800.0,
        "deep_filterCutoff": 280.0, "deep_filterRes": 0.35,
        "deep_bodyMix": 0.2, "deep_bodyChar": CAVE,
        "deep_reverbMix": 0.55,
        "deep_lfo1Depth": 0.35, "deep_lfo1Rate": 0.1,
        "deep_macroCreature": 0.3,
    }, (0.15, 0.6, 0.35, 0.25, 0.55, 0.0),
     "What does the giant squid dream? Slow bioluminescent pulses in cave darkness.",
     ["atmosphere", "cephalopod", "dream", "bio"]),

    ("Salinity Layer", {
        "deep_ampAtk": 0.25, "deep_ampSus": 0.85, "deep_ampRel": 3.5,
        "deep_filterCutoff": 220.0, "deep_filterRes": 0.55,
        "deep_pressureAmt": 0.55, "deep_subOctMix": 0.4,
        "deep_bodyMix": 0.3, "deep_bodyChar": OPEN_WATER,
        "deep_reverbMix": 0.45,
        "deep_lfo1Depth": 0.1, "deep_lfo2Depth": 0.2,
    }, (0.08, 0.7, 0.18, 0.2, 0.45, 0.05),
     "Where salt concentration changes. Resonant filter boundary between water masses.",
     ["atmosphere", "salinity", "layer", "gradient"]),

    ("Night Drift", {
        "deep_ampAtk": 0.3, "deep_ampSus": 0.9, "deep_ampRel": 5.0,
        "deep_filterCutoff": 200.0, "deep_filterRes": 0.3,
        "deep_subOctMix": 0.5,
        "deep_bodyMix": 0.2, "deep_bodyChar": OPEN_WATER,
        "deep_bioMix": 0.2, "deep_bioRate": 0.03,
        "deep_reverbMix": 0.5,
        "deep_lfo2Depth": 0.3, "deep_lfo2Rate": 0.02,
        "deep_lfo1Depth": 0.15,
    }, (0.06, 0.75, 0.22, 0.2, 0.5, 0.0),
     "Drifting in total darkness. The slowest LFO2 creates glacial pitch movement.",
     ["atmosphere", "night", "drift", "glacial"]),

    ("Turbidity Current", {
        "deep_ampAtk": 0.15, "deep_ampSus": 0.8, "deep_ampRel": 2.5,
        "deep_filterCutoff": 300.0, "deep_filterRes": 0.45,
        "deep_pressureAmt": 0.7, "deep_subOctMix": 0.6,
        "deep_bodyMix": 0.4, "deep_bodyChar": OPEN_WATER, "deep_bodyFeedback": 0.55,
        "deep_reverbMix": 0.35,
        "deep_lfo1Depth": 0.2, "deep_lfo2Depth": 0.2,
    }, (0.1, 0.7, 0.25, 0.3, 0.3, 0.1),
     "Underwater avalanche of sediment. Pressure and body resonance create density.",
     ["atmosphere", "turbidity", "current", "dense"]),

    ("Bone Current", {
        "deep_ampAtk": 0.2, "deep_ampSus": 0.85, "deep_ampRel": 3.0,
        "deep_filterCutoff": 250.0, "deep_filterRes": 0.6,
        "deep_bodyChar": WRECK, "deep_bodyMix": 0.45, "deep_bodyFeedback": 0.6,
        "deep_pressureAmt": 0.5,
        "deep_bioMix": 0.1, "deep_bioBrightness": 400.0,
        "deep_reverbMix": 0.4,
    }, (0.1, 0.65, 0.15, 0.3, 0.4, 0.1),
     "Whale fall site. Wreck body resonates with the bones of a leviathan.",
     ["atmosphere", "bone", "whale-fall", "resonant"]),

    ("Pelagic Void", {
        "deep_ampAtk": 0.5, "deep_ampSus": 0.9, "deep_ampRel": 7.0,
        "deep_filterCutoff": 80.0, "deep_filterRes": 0.25,
        "deep_subOctMix": 0.7, "deep_pressureAmt": 0.5,
        "deep_bodyMix": 0.0, "deep_bioMix": 0.0,
        "deep_reverbMix": 0.6,
        "deep_lfo1Depth": 0.0, "deep_lfo2Depth": 0.1,
    }, (0.01, 0.85, 0.05, 0.15, 0.6, 0.0),
     "The open water void between surface and floor. Filter at near-minimum. Pure pressure.",
     ["atmosphere", "pelagic", "void", "minimal"]),

    ("Sediment Rain", {
        "deep_ampAtk": 0.3, "deep_ampSus": 0.8, "deep_ampRel": 3.5,
        "deep_filterCutoff": 280.0, "deep_filterRes": 0.4,
        "deep_bioMix": 0.35, "deep_bioRate": 0.12, "deep_bioBrightness": 900.0,
        "deep_bodyMix": 0.15,
        "deep_reverbMix": 0.5, "deep_pressureAmt": 0.5,
        "deep_lfo1Depth": 0.2,
    }, (0.12, 0.65, 0.25, 0.25, 0.5, 0.0),
     "Marine snow falling through the dark. Bio exciter creates particle-like textures.",
     ["atmosphere", "sediment", "marine-snow", "particle"]),

    ("Dark Thermocline", {
        "deep_ampAtk": 0.25, "deep_ampSus": 0.85, "deep_ampRel": 4.0,
        "deep_filterCutoff": 200.0, "deep_filterRes": 0.5,
        "deep_subOctMix": 0.45,
        "deep_bodyChar": OPEN_WATER, "deep_bodyMix": 0.3, "deep_bodyFeedback": 0.5,
        "deep_pressureAmt": 0.6,
        "deep_reverbMix": 0.45,
        "deep_lfo2Depth": 0.25, "deep_lfo2Rate": 0.04,
    }, (0.07, 0.72, 0.18, 0.22, 0.45, 0.03),
     "Temperature boundary between warm and cold water. Slow pressure oscillation.",
     ["atmosphere", "thermocline", "boundary", "temperature"]),

    ("Silent Depths", {
        "deep_ampAtk": 0.4, "deep_ampSus": 0.9, "deep_ampRel": 6.0,
        "deep_filterCutoff": 100.0, "deep_filterRes": 0.2,
        "deep_subOctMix": 0.5, "deep_pressureAmt": 0.4,
        "deep_bodyMix": 0.0, "deep_bioMix": 0.05, "deep_bioRate": 0.02,
        "deep_reverbMix": 0.55,
        "deep_lfo1Depth": 0.05, "deep_lfo2Depth": 0.05,
        "deep_macroAbyss": 0.3,
    }, (0.02, 0.82, 0.05, 0.1, 0.55, 0.0),
     "Near silence. Filter at minimum with ABYSS engaged. The quietest sound that still exists.",
     ["atmosphere", "silent", "minimal", "extreme"]),

    ("Hadopelagic Air", {
        "deep_ampAtk": 0.35, "deep_ampSus": 0.85, "deep_ampRel": 5.0,
        "deep_filterCutoff": 160.0, "deep_filterRes": 0.35,
        "deep_subOctMix": 0.6, "deep_pressureAmt": 0.65,
        "deep_bodyMix": 0.25, "deep_bodyChar": CAVE,
        "deep_bioMix": 0.15, "deep_bioBrightness": 500.0,
        "deep_reverbMix": 0.6,
        "deep_lfo1Depth": 0.15, "deep_lfo2Depth": 0.15,
        "deep_macroAbyss": 0.2,
    }, (0.04, 0.72, 0.2, 0.2, 0.6, 0.0),
     "The hadopelagic zone: deepest ocean layer. Everything compressed and dark.",
     ["atmosphere", "hadopelagic", "deepest", "zone"]),

    ("Fathom Cloud", {
        "deep_ampAtk": 0.3, "deep_ampSus": 0.88, "deep_ampRel": 4.5,
        "deep_filterCutoff": 220.0, "deep_filterRes": 0.4,
        "deep_bodyMix": 0.35, "deep_bodyChar": CAVE, "deep_bodyFeedback": 0.55,
        "deep_bioMix": 0.2, "deep_bioRate": 0.06, "deep_bioBrightness": 800.0,
        "deep_reverbMix": 0.6,
        "deep_lfo1Depth": 0.2, "deep_lfo2Depth": 0.15,
    }, (0.08, 0.68, 0.22, 0.25, 0.6, 0.0),
     "A thousand fathoms of water overhead. Cave resonance and bio-luminescent cloud.",
     ["atmosphere", "fathom", "cloud", "deep"]),

    ("Rumble Haze", {
        "deep_ampAtk": 0.15, "deep_ampSus": 0.9, "deep_ampRel": 3.0,
        "deep_filterCutoff": 200.0, "deep_filterRes": 0.3,
        "deep_subOctMix": 0.8, "deep_pressureAmt": 0.75,
        "deep_bodyMix": 0.2,
        "deep_reverbMix": 0.4,
        "deep_lfo2Depth": 0.3, "deep_lfo2Rate": 0.04,
    }, (0.04, 0.82, 0.15, 0.25, 0.35, 0.1),
     "Heavy sub-octave mix with maximum pressure. Geological rumble at the earth's crust.",
     ["atmosphere", "rumble", "geological", "heavy"]),

    ("Micro Current", {
        "deep_ampAtk": 0.25, "deep_ampSus": 0.85, "deep_ampRel": 3.5,
        "deep_filterCutoff": 300.0, "deep_filterRes": 0.45,
        "deep_bodyMix": 0.3, "deep_bodyChar": OPEN_WATER,
        "deep_bioMix": 0.25, "deep_bioRate": 0.08, "deep_bioBrightness": 1200.0,
        "deep_pressureAmt": 0.45,
        "deep_reverbMix": 0.45,
        "deep_lfo1Depth": 0.25, "deep_lfo1Rate": 0.12,
    }, (0.12, 0.65, 0.28, 0.25, 0.45, 0.03),
     "Small-scale water movement with bioluminescent organisms caught in the flow.",
     ["atmosphere", "micro", "current", "flow"]),

    ("Abyssal Swell", {
        "deep_ampAtk": 0.4, "deep_ampSus": 0.88, "deep_ampRel": 5.0,
        "deep_filterCutoff": 220.0, "deep_filterRes": 0.35,
        "deep_subOctMix": 0.55, "deep_pressureAmt": 0.55,
        "deep_bodyMix": 0.2, "deep_bodyChar": OPEN_WATER,
        "deep_bioMix": 0.1,
        "deep_reverbMix": 0.5,
        "deep_lfo2Depth": 0.25, "deep_lfo2Rate": 0.03,
        "deep_lfo1Depth": 0.1,
    }, (0.06, 0.74, 0.18, 0.2, 0.5, 0.0),
     "Slow deep swell from the ocean floor. Pressure and pitch rise and fall together.",
     ["atmosphere", "swell", "slow", "floor"]),
]


# ══════════════════════════════════════════════════════════════════════════════
# ENTANGLED (20) — coupling showcases
# ══════════════════════════════════════════════════════════════════════════════

ENTANGLED = [
    ("Pressure Donor", {
        "deep_subOctMix": 0.6, "deep_pressureAmt": 0.7,
        "deep_filterCutoff": 400.0, "deep_bodyMix": 0.2,
        "deep_macroPressure": 0.6,
    }, (0.1, 0.75, 0.1, 0.2, 0.05, 0.1),
     "Heavy sub designed to feed coupling output. Pressure shapes what it touches.",
     ["entangled", "donor", "pressure", "coupling"]),

    ("Darkness Receiver", {
        "deep_filterCutoff": 250.0, "deep_filterRes": 0.5,
        "deep_pressureAmt": 0.4, "deep_subOctMix": 0.3,
        "deep_bodyMix": 0.2,
    }, (0.08, 0.72, 0.15, 0.15, 0.1, 0.05),
     "Dark sub bass waiting for coupling input. Other engines shape its filter.",
     ["entangled", "receiver", "dark", "coupling"]),

    ("Wreck Resonator", {
        "deep_bodyChar": WRECK, "deep_bodyMix": 0.7, "deep_bodyFeedback": 0.7,
        "deep_filterCutoff": 350.0, "deep_filterRes": 0.4,
        "deep_pressureAmt": 0.5,
    }, (0.15, 0.6, 0.1, 0.35, 0.15, 0.15),
     "High wreck body mix as coupling resonator. Incoming signals excite hull modes.",
     ["entangled", "wreck", "resonator", "hull"]),

    ("Creature Sync", {
        "deep_bioMix": 0.5, "deep_bioRate": 0.1, "deep_bioBrightness": 1500.0,
        "deep_filterCutoff": 350.0, "deep_bodyMix": 0.15,
        "deep_lfo1Depth": 0.3, "deep_macroCreature": 0.5,
    }, (0.2, 0.6, 0.35, 0.25, 0.1, 0.05),
     "Bioluminescent exciter synchronized to coupling tempo. Creature responds to fleet.",
     ["entangled", "creature", "sync", "bio"]),

    ("Coupled Darkness", {
        "deep_filterCutoff": 150.0, "deep_filterRes": 0.6,
        "deep_pressureAmt": 0.7, "deep_subOctMix": 0.5,
        "deep_reverbMix": 0.3,
        "deep_macroAbyss": 0.4,
    }, (0.04, 0.78, 0.1, 0.2, 0.3, 0.08),
     "Maximum darkness with coupling modulating the filter. Light comes from outside.",
     ["entangled", "dark", "coupled", "filter"]),

    ("Pressure Gate", {
        "deep_ampAtk": 0.001, "deep_ampDec": 0.2, "deep_ampSus": 0.0, "deep_ampRel": 0.3,
        "deep_filterCutoff": 500.0, "deep_pressureAmt": 0.8,
        "deep_subOctMix": 0.5,
    }, (0.15, 0.7, 0.05, 0.2, 0.0, 0.2),
     "Percussive sub burst. Coupling controls the gate timing from another engine.",
     ["entangled", "gate", "percussive", "coupling"]),

    ("Sub Granulate", {
        "deep_filterCutoff": 300.0, "deep_filterRes": 0.4,
        "deep_bodyMix": 0.3, "deep_bioMix": 0.3,
        "deep_pressureAmt": 0.5,
        "deep_lfo1Depth": 0.2,
    }, (0.1, 0.65, 0.25, 0.25, 0.15, 0.05),
     "Sub bass with bio texture. Designed for Opal granular coupling overlay.",
     ["entangled", "granular", "texture", "opal"]),

    ("FM Creature", {
        "deep_bioMix": 0.4, "deep_bioRate": 0.15, "deep_bioBrightness": 2500.0,
        "deep_filterCutoff": 400.0, "deep_bodyMix": 0.2,
        "deep_lfo1Depth": 0.3,
    }, (0.2, 0.6, 0.3, 0.25, 0.1, 0.1),
     "Bio exciter at high brightness. Coupling FM input creates alien creature sounds.",
     ["entangled", "fm", "creature", "alien"]),

    ("Overdub Floor", {
        "deep_subOctMix": 0.5, "deep_filterCutoff": 350.0,
        "deep_bodyMix": 0.25, "deep_bodyChar": CAVE,
        "deep_pressureAmt": 0.5,
        "deep_reverbMix": 0.4,
    }, (0.1, 0.7, 0.1, 0.2, 0.35, 0.05),
     "Sub floor for dub coupling. OceanDeep provides depth; Overdub provides echo.",
     ["entangled", "overdub", "dub", "floor"]),

    ("Onset Pair", {
        "deep_ampAtk": 0.001, "deep_ampDec": 0.3, "deep_ampSus": 0.1, "deep_ampRel": 0.5,
        "deep_filterCutoff": 500.0, "deep_subOctMix": 0.6,
        "deep_pressureAmt": 0.4,
    }, (0.15, 0.7, 0.05, 0.2, 0.0, 0.2),
     "Tight sub designed to pair with Onset drum hits. Coupling adds kick weight.",
     ["entangled", "onset", "drums", "kick"]),

    ("Envelope Weight", {
        "deep_ampAtk": 0.05, "deep_ampDec": 1.0, "deep_ampSus": 0.3, "deep_ampRel": 2.0,
        "deep_filterCutoff": 350.0, "deep_filterRes": 0.4,
        "deep_velCutoffAmt": 0.7, "deep_pressureAmt": 0.5,
    }, (0.1, 0.7, 0.1, 0.2, 0.1, 0.1),
     "Envelope-shaped sub. Coupling modulates the weight of each note.",
     ["entangled", "envelope", "shaped", "dynamic"]),

    ("Abyss Modulator", {
        "deep_filterCutoff": 200.0, "deep_filterRes": 0.5,
        "deep_pressureAmt": 0.6, "deep_subOctMix": 0.5,
        "deep_lfo1Depth": 0.3, "deep_lfo2Depth": 0.2,
        "deep_macroAbyss": 0.5,
    }, (0.06, 0.72, 0.25, 0.2, 0.25, 0.08),
     "ABYSS macro with LFO modulation. Coupling adds filter movement from partner.",
     ["entangled", "abyss", "modulator", "lfo"]),

    ("Reverb Send", {
        "deep_reverbMix": 0.7, "deep_filterCutoff": 300.0,
        "deep_bodyMix": 0.2, "deep_bodyChar": CAVE,
        "deep_pressureAmt": 0.4,
        "deep_lfo1Depth": 0.1,
    }, (0.08, 0.65, 0.1, 0.2, 0.65, 0.0),
     "Heavy reverb send for coupling. Partner engines get swallowed by the cave.",
     ["entangled", "reverb", "send", "cave"]),

    ("Sidechain Sub", {
        "deep_ampAtk": 0.001, "deep_ampDec": 0.15, "deep_ampSus": 0.7, "deep_ampRel": 0.2,
        "deep_filterCutoff": 450.0, "deep_subOctMix": 0.5,
        "deep_pressureAmt": 0.5,
    }, (0.12, 0.72, 0.05, 0.15, 0.0, 0.1),
     "Sub bass with sidechain-style envelope. Ducks when coupled engine plays.",
     ["entangled", "sidechain", "sub", "duck"]),

    ("Opal Feed", {
        "deep_filterCutoff": 350.0, "deep_bodyMix": 0.3,
        "deep_bioMix": 0.2, "deep_bioBrightness": 1000.0,
        "deep_pressureAmt": 0.4,
        "deep_reverbMix": 0.3,
    }, (0.12, 0.68, 0.2, 0.22, 0.25, 0.05),
     "Textured sub designed for Opal granular feed. Bass grains from the deep.",
     ["entangled", "opal", "feed", "granular"]),

    ("Dub Predator", {
        "deep_bodyChar": CAVE, "deep_bodyMix": 0.5, "deep_bodyFeedback": 0.6,
        "deep_filterCutoff": 300.0, "deep_filterRes": 0.45,
        "deep_pressureAmt": 0.6,
        "deep_reverbMix": 0.45,
        "deep_lfo2Depth": 0.2,
    }, (0.1, 0.68, 0.15, 0.3, 0.4, 0.12),
     "Cave bass stalking through dub delay. Coupling from Overdub creates echo depth.",
     ["entangled", "dub", "predator", "cave"]),

    ("Amp Gate", {
        "deep_ampAtk": 0.001, "deep_ampDec": 0.1, "deep_ampSus": 0.0, "deep_ampRel": 0.2,
        "deep_filterCutoff": 600.0, "deep_subOctMix": 0.4,
        "deep_pressureAmt": 0.3,
    }, (0.15, 0.7, 0.0, 0.15, 0.0, 0.15),
     "Extremely short gate. Coupling from another engine retrigs the amp envelope.",
     ["entangled", "gate", "retrig", "short"]),

    ("Full Column", {
        "deep_subOctMix": 0.6, "deep_filterCutoff": 350.0, "deep_filterRes": 0.4,
        "deep_bodyMix": 0.4, "deep_bodyChar": OPEN_WATER,
        "deep_bioMix": 0.2, "deep_pressureAmt": 0.5,
        "deep_reverbMix": 0.4,
        "deep_lfo1Depth": 0.2, "deep_lfo2Depth": 0.15,
        "deep_macroPressure": 0.3, "deep_macroCreature": 0.2,
    }, (0.1, 0.7, 0.2, 0.3, 0.35, 0.05),
     "The full water column from surface to floor. All systems engaged for coupling.",
     ["entangled", "full", "column", "all"]),

    ("Abyssal Link", {
        "deep_filterCutoff": 200.0, "deep_filterRes": 0.5,
        "deep_pressureAmt": 0.65, "deep_subOctMix": 0.5,
        "deep_bodyMix": 0.3, "deep_bodyChar": WRECK,
        "deep_reverbMix": 0.35,
        "deep_macroAbyss": 0.3, "deep_macroPressure": 0.3,
    }, (0.07, 0.72, 0.12, 0.25, 0.3, 0.1),
     "Dark linked sub. Two macros engaged to respond to coupling modulation.",
     ["entangled", "link", "abyssal", "macros"]),

    ("Delay Trap", {
        "deep_bodyChar": WRECK, "deep_bodyMix": 0.5, "deep_bodyFeedback": 0.7,
        "deep_filterCutoff": 300.0, "deep_filterRes": 0.5,
        "deep_reverbMix": 0.5, "deep_pressureAmt": 0.5,
        "deep_lfo1Depth": 0.15,
    }, (0.1, 0.62, 0.15, 0.3, 0.5, 0.12),
     "Wreck body as delay trap. Coupling signals get caught in the hull feedback.",
     ["entangled", "delay", "trap", "feedback"]),
]


# ══════════════════════════════════════════════════════════════════════════════
# PRISM (25) — genre production presets
# ══════════════════════════════════════════════════════════════════════════════

# --- Techno (5) ---
TECHNO = [
    ("Warehouse Sub", {
        "deep_ampAtk": 0.001, "deep_ampDec": 0.4, "deep_ampSus": 0.6, "deep_ampRel": 0.3,
        "deep_filterCutoff": 500.0, "deep_filterRes": 0.3,
        "deep_subOctMix": 0.5, "deep_pressureAmt": 0.5,
        "deep_velCutoffAmt": 0.5,
    }, (0.12, 0.75, 0.05, 0.2, 0.0, 0.2),
     "Tight techno sub for warehouse floors. Velocity-responsive darkness.",
     ["techno", "sub", "warehouse", "tight"]),

    ("Industrial Crawl", {
        "deep_filterCutoff": 300.0, "deep_filterRes": 0.6,
        "deep_bodyChar": WRECK, "deep_bodyMix": 0.5, "deep_bodyFeedback": 0.65,
        "deep_pressureAmt": 0.7, "deep_subOctMix": 0.6,
        "deep_lfo2Depth": 0.15,
    }, (0.1, 0.6, 0.12, 0.35, 0.1, 0.35),
     "Metallic wreck resonance for industrial techno. Heavy and aggressive.",
     ["techno", "industrial", "metallic", "heavy"]),

    ("Acid Abyss", {
        "deep_filterCutoff": 200.0, "deep_filterRes": 0.85,
        "deep_velCutoffAmt": 0.8,
        "deep_pressureAmt": 0.5, "deep_subOctMix": 0.4,
        "deep_lfo1Depth": 0.15, "deep_lfo1Rate": 0.3,
    }, (0.12, 0.65, 0.2, 0.2, 0.0, 0.3),
     "High resonance darkness filter with velocity sweep. Acid from the trench floor.",
     ["techno", "acid", "resonant", "bass"]),

    ("Perc Sub", {
        "deep_ampAtk": 0.001, "deep_ampDec": 0.15, "deep_ampSus": 0.0, "deep_ampRel": 0.2,
        "deep_filterCutoff": 600.0, "deep_subOctMix": 0.3,
        "deep_pressureAmt": 0.3,
        "deep_bodyMix": 0.0, "deep_bioMix": 0.0,
    }, (0.15, 0.7, 0.0, 0.15, 0.0, 0.2),
     "Ultra-short percussive sub. Layer under kicks for underground techno weight.",
     ["techno", "percussive", "kick", "short"]),

    ("Berlin Rumble", {
        "deep_filterCutoff": 250.0, "deep_filterRes": 0.4,
        "deep_subOctMix": 0.6, "deep_pressureAmt": 0.65,
        "deep_bodyChar": CAVE, "deep_bodyMix": 0.3,
        "deep_reverbMix": 0.2,
        "deep_lfo2Depth": 0.2, "deep_lfo2Rate": 0.06,
    }, (0.08, 0.72, 0.15, 0.25, 0.15, 0.2),
     "Deep Berlin-style sub bass with cave resonance. Steady rumble for four-on-the-floor.",
     ["techno", "berlin", "rumble", "steady"]),
]

# --- Dub / Dub Techno (5) ---
DUB = [
    ("Dub Pressure", {
        "deep_filterCutoff": 300.0, "deep_filterRes": 0.4,
        "deep_bodyChar": CAVE, "deep_bodyMix": 0.5, "deep_bodyFeedback": 0.55,
        "deep_pressureAmt": 0.6,
        "deep_reverbMix": 0.5,
        "deep_lfo2Depth": 0.2, "deep_lfo2Rate": 0.04,
    }, (0.08, 0.7, 0.15, 0.25, 0.45, 0.1),
     "Classic dub sub with cave body and deep reverb. The weight beneath the delay.",
     ["dub", "bass", "pressure", "cave"]),

    ("Steppers Weight", {
        "deep_ampAtk": 0.001, "deep_ampDec": 0.5, "deep_ampSus": 0.5, "deep_ampRel": 0.5,
        "deep_filterCutoff": 400.0, "deep_subOctMix": 0.5,
        "deep_pressureAmt": 0.5, "deep_bodyMix": 0.2,
        "deep_velCutoffAmt": 0.5,
    }, (0.12, 0.72, 0.05, 0.2, 0.1, 0.15),
     "Steppers-style sub bass. Velocity opens the filter for dynamic bass lines.",
     ["dub", "steppers", "bass", "dynamic"]),

    ("Roots Thunder", {
        "deep_subOctMix": 0.7, "deep_filterCutoff": 250.0,
        "deep_pressureAmt": 0.75, "deep_bodyMix": 0.15,
        "deep_reverbMix": 0.35,
        "deep_ampSus": 0.85, "deep_ampRel": 2.0,
    }, (0.06, 0.8, 0.05, 0.2, 0.3, 0.1),
     "Maximum sub octaves under heavy pressure. Roots reggae earthquake bass.",
     ["dub", "roots", "thunder", "earthquake"]),

    ("Echo Chamber", {
        "deep_bodyChar": CAVE, "deep_bodyMix": 0.6, "deep_bodyFeedback": 0.65,
        "deep_filterCutoff": 350.0, "deep_filterRes": 0.45,
        "deep_reverbMix": 0.55,
        "deep_bioMix": 0.1, "deep_bioBrightness": 600.0,
        "deep_lfo1Depth": 0.15,
    }, (0.1, 0.65, 0.15, 0.3, 0.55, 0.05),
     "Cave body as echo chamber. The King Tubby of the deep ocean.",
     ["dub", "echo", "chamber", "king-tubby"]),

    ("Dub Siren", {
        "deep_filterCutoff": 400.0, "deep_filterRes": 0.6,
        "deep_lfo1Rate": 0.5, "deep_lfo1Depth": 0.5,
        "deep_lfo2Rate": 0.08, "deep_lfo2Depth": 0.3,
        "deep_bodyMix": 0.2, "deep_reverbMix": 0.4,
    }, (0.15, 0.6, 0.45, 0.2, 0.35, 0.15),
     "LFO-driven filter modulation for dub siren effects. Creature as alarm.",
     ["dub", "siren", "lfo", "alarm"]),
]

# --- DnB / Jungle (5) ---
DNB = [
    ("Reese Abyss", {
        "deep_filterCutoff": 350.0, "deep_filterRes": 0.5,
        "deep_subOctMix": 0.4, "deep_pressureAmt": 0.5,
        "deep_lfo1Rate": 0.8, "deep_lfo1Depth": 0.3,
        "deep_bodyMix": 0.2,
    }, (0.12, 0.65, 0.3, 0.25, 0.1, 0.25),
     "Reese bass from the deep. LFO1 at faster rate creates the classic phasing movement.",
     ["dnb", "reese", "bass", "phasing"]),

    ("Jungle Sub", {
        "deep_ampAtk": 0.001, "deep_ampDec": 0.3, "deep_ampSus": 0.6, "deep_ampRel": 0.4,
        "deep_filterCutoff": 500.0, "deep_subOctMix": 0.5,
        "deep_pressureAmt": 0.4,
        "deep_velCutoffAmt": 0.6,
    }, (0.13, 0.72, 0.05, 0.18, 0.0, 0.18),
     "Tight jungle sub bass. Velocity controls darkness for expressive bass lines.",
     ["dnb", "jungle", "sub", "tight"]),

    ("Amen Weight", {
        "deep_ampAtk": 0.001, "deep_ampDec": 0.2, "deep_ampSus": 0.0, "deep_ampRel": 0.3,
        "deep_filterCutoff": 600.0, "deep_subOctMix": 0.6,
        "deep_pressureAmt": 0.5,
        "deep_reverbMix": 0.15,
    }, (0.15, 0.7, 0.0, 0.2, 0.1, 0.2),
     "Percussive sub designed to layer under chopped breaks. The weight beneath the Amen.",
     ["dnb", "amen", "percussive", "breaks"]),

    ("Liquid Depth", {
        "deep_ampAtk": 0.05, "deep_ampSus": 0.8, "deep_ampRel": 1.5,
        "deep_filterCutoff": 400.0, "deep_filterRes": 0.3,
        "deep_bodyMix": 0.25, "deep_bodyChar": OPEN_WATER,
        "deep_reverbMix": 0.3,
        "deep_lfo2Depth": 0.15,
    }, (0.1, 0.72, 0.12, 0.2, 0.25, 0.08),
     "Smooth liquid DnB sub with gentle body resonance and space.",
     ["dnb", "liquid", "smooth", "bass"]),

    ("Half-time Crawl", {
        "deep_ampAtk": 0.01, "deep_ampDec": 0.8, "deep_ampSus": 0.5, "deep_ampRel": 1.0,
        "deep_filterCutoff": 300.0, "deep_filterRes": 0.5,
        "deep_pressureAmt": 0.6,
        "deep_bodyChar": WRECK, "deep_bodyMix": 0.3,
        "deep_lfo1Rate": 0.4, "deep_lfo1Depth": 0.2,
    }, (0.1, 0.65, 0.2, 0.25, 0.1, 0.25),
     "Heavy half-time bass with wreck character. Slow and menacing.",
     ["dnb", "halftime", "heavy", "crawl"]),
]

# --- Trap / Hip-Hop (5) ---
TRAP = [
    ("808 Subterranean", {
        "deep_ampAtk": 0.001, "deep_ampDec": 2.0, "deep_ampSus": 0.0, "deep_ampRel": 1.0,
        "deep_filterCutoff": 500.0, "deep_filterRes": 0.2,
        "deep_subOctMix": 0.5, "deep_pressureAmt": 0.4,
    }, (0.12, 0.75, 0.0, 0.15, 0.05, 0.15),
     "808-style long decay sub bass for trap production. Clean and heavy.",
     ["trap", "808", "sub", "long-decay"]),

    ("Distorted Deep", {
        "deep_filterCutoff": 500.0, "deep_filterRes": 0.5,
        "deep_pressureAmt": 0.85, "deep_subOctMix": 0.6,
        "deep_bodyChar": WRECK, "deep_bodyMix": 0.3,
    }, (0.12, 0.65, 0.05, 0.25, 0.05, 0.35),
     "Maximum pressure into wreck body creates natural distortion. Dark trap bass.",
     ["trap", "distorted", "heavy", "dark"]),

    ("Glide Trench", {
        "deep_ampAtk": 0.001, "deep_ampDec": 1.5, "deep_ampSus": 0.0, "deep_ampRel": 0.8,
        "deep_filterCutoff": 400.0, "deep_subOctMix": 0.4,
        "deep_pressureAmt": 0.5,
        "deep_velCutoffAmt": 0.5,
    }, (0.1, 0.73, 0.0, 0.15, 0.0, 0.18),
     "808 sub with velocity control. Pitch-bend for trap glide patterns.",
     ["trap", "glide", "808", "pitch-bend"]),

    ("Dark Atlanta", {
        "deep_filterCutoff": 250.0, "deep_filterRes": 0.4,
        "deep_subOctMix": 0.7, "deep_pressureAmt": 0.65,
        "deep_ampDec": 1.8, "deep_ampSus": 0.0, "deep_ampRel": 0.8,
        "deep_reverbMix": 0.15,
    }, (0.06, 0.78, 0.0, 0.2, 0.1, 0.2),
     "Ultra-dark sub for modern trap. Sub-octaves add geological weight.",
     ["trap", "dark", "heavy", "atlanta"]),

    ("Bouncy Sub", {
        "deep_ampAtk": 0.001, "deep_ampDec": 0.6, "deep_ampSus": 0.3, "deep_ampRel": 0.4,
        "deep_filterCutoff": 450.0, "deep_subOctMix": 0.3,
        "deep_pressureAmt": 0.3,
        "deep_velCutoffAmt": 0.6,
    }, (0.12, 0.72, 0.05, 0.15, 0.0, 0.12),
     "Shorter decay sub with bounce. Velocity adds filter brightness for articulation.",
     ["trap", "bouncy", "sub", "articulated"]),
]

# --- Ambient / Film Score (5) ---
AMBIENT_SCORE = [
    ("Abyssal Score", {
        "deep_ampAtk": 0.4, "deep_ampSus": 0.9, "deep_ampRel": 6.0,
        "deep_filterCutoff": 200.0, "deep_filterRes": 0.3,
        "deep_subOctMix": 0.6, "deep_pressureAmt": 0.6,
        "deep_bodyMix": 0.3, "deep_bodyChar": CAVE,
        "deep_reverbMix": 0.6,
        "deep_lfo1Depth": 0.15, "deep_lfo2Depth": 0.2,
    }, (0.05, 0.75, 0.18, 0.2, 0.6, 0.0),
     "Film score bass for underwater scenes. Cave body, deep reverb, slow LFOs.",
     ["ambient", "score", "film", "cinematic"]),

    ("Submarine Tone", {
        "deep_filterCutoff": 300.0, "deep_filterRes": 0.4,
        "deep_bodyChar": WRECK, "deep_bodyMix": 0.5, "deep_bodyFeedback": 0.55,
        "deep_ampAtk": 0.1, "deep_ampSus": 0.85, "deep_ampRel": 3.0,
        "deep_reverbMix": 0.4,
        "deep_lfo2Depth": 0.15,
    }, (0.1, 0.65, 0.1, 0.3, 0.35, 0.1),
     "Sonar-like tone with wreck body. The submarine searching in the dark.",
     ["ambient", "submarine", "sonar", "metallic"]),

    ("Deep Meditation", {
        "deep_ampAtk": 0.5, "deep_ampSus": 0.95, "deep_ampRel": 8.0,
        "deep_filterCutoff": 150.0, "deep_filterRes": 0.25,
        "deep_subOctMix": 0.5, "deep_pressureAmt": 0.4,
        "deep_reverbMix": 0.55,
        "deep_lfo2Depth": 0.15, "deep_lfo2Rate": 0.02,
    }, (0.03, 0.82, 0.08, 0.12, 0.55, 0.0),
     "Extended meditation drone. Maximum release, minimum brightness. Just pressure.",
     ["ambient", "meditation", "drone", "minimal"]),

    ("Tension Rise", {
        "deep_ampAtk": 0.5, "deep_ampSus": 0.9, "deep_ampRel": 2.0,
        "deep_filterCutoff": 150.0, "deep_filterRes": 0.6,
        "deep_pressureAmt": 0.7, "deep_subOctMix": 0.6,
        "deep_bodyMix": 0.2,
        "deep_lfo1Rate": 0.3, "deep_lfo1Depth": 0.2,
        "deep_macroAbyss": 0.3,
    }, (0.05, 0.7, 0.2, 0.2, 0.15, 0.15),
     "Tension-building bass for horror/thriller scoring. Resonance and creature movement.",
     ["ambient", "tension", "horror", "thriller"]),

    ("Horizon Line", {
        "deep_ampAtk": 0.35, "deep_ampSus": 0.85, "deep_ampRel": 5.0,
        "deep_filterCutoff": 250.0, "deep_filterRes": 0.3,
        "deep_subOctMix": 0.4, "deep_bodyMix": 0.2,
        "deep_bioMix": 0.15, "deep_bioBrightness": 800.0,
        "deep_reverbMix": 0.5,
        "deep_lfo1Depth": 0.1, "deep_lfo2Depth": 0.15,
    }, (0.08, 0.72, 0.15, 0.18, 0.5, 0.0),
     "Where the ocean meets the abyss. Balanced bass for ambient production.",
     ["ambient", "horizon", "balanced", "production"]),
]


# ══════════════════════════════════════════════════════════════════════════════
# FLUX (20) — movement, LFO-heavy, filter sweeps
# ══════════════════════════════════════════════════════════════════════════════

FLUX = [
    ("Creature Chase", {
        "deep_lfo1Rate": 1.0, "deep_lfo1Depth": 0.6,
        "deep_bioMix": 0.4, "deep_bioRate": 0.2, "deep_bioBrightness": 2000.0,
        "deep_filterCutoff": 400.0, "deep_filterRes": 0.4,
        "deep_bodyMix": 0.15,
    }, (0.2, 0.55, 0.55, 0.25, 0.1, 0.15),
     "Fast creature LFO with bright bio exciter. Something is hunting in the dark.",
     ["flux", "creature", "chase", "fast"]),

    ("Darkness Sweep", {
        "deep_lfo1Rate": 0.08, "deep_lfo1Depth": 0.7,
        "deep_filterCutoff": 400.0, "deep_filterRes": 0.5,
        "deep_pressureAmt": 0.5,
        "deep_lfo2Depth": 0.0,
    }, (0.1, 0.65, 0.45, 0.2, 0.1, 0.1),
     "Slow LFO1 sweeps the darkness filter. Light and dark cycle like tidal breathing.",
     ["flux", "sweep", "filter", "slow"]),

    ("Pressure Wave", {
        "deep_lfo2Rate": 0.15, "deep_lfo2Depth": 0.6,
        "deep_pressureAmt": 0.6, "deep_subOctMix": 0.5,
        "deep_filterCutoff": 350.0,
        "deep_lfo1Depth": 0.0,
    }, (0.08, 0.72, 0.4, 0.2, 0.05, 0.1),
     "LFO2 creates strong pressure wobble. Pitch and compression oscillate together.",
     ["flux", "pressure", "wobble", "wave"]),

    ("Tidal Pulse", {
        "deep_lfo1Rate": 0.5, "deep_lfo1Depth": 0.4,
        "deep_lfo2Rate": 0.07, "deep_lfo2Depth": 0.3,
        "deep_filterCutoff": 350.0, "deep_filterRes": 0.4,
        "deep_bodyMix": 0.2, "deep_pressureAmt": 0.5,
    }, (0.1, 0.68, 0.45, 0.22, 0.1, 0.1),
     "Two LFOs at different rates. Polyrhythmic tidal movement in the bass.",
     ["flux", "tidal", "pulse", "polyrhythm"]),

    ("Bioluminescent Pulse", {
        "deep_bioMix": 0.6, "deep_bioRate": 0.15, "deep_bioBrightness": 2500.0,
        "deep_filterCutoff": 350.0,
        "deep_lfo1Depth": 0.4, "deep_lfo1Rate": 0.2,
        "deep_reverbMix": 0.3,
    }, (0.22, 0.55, 0.4, 0.25, 0.25, 0.05),
     "Maximum bio mix with fast triggering. The ocean floor pulses with alien light.",
     ["flux", "bio", "pulse", "bioluminescent"]),

    ("Surge Bass", {
        "deep_ampAtk": 0.001, "deep_ampDec": 0.5, "deep_ampSus": 0.6, "deep_ampRel": 0.5,
        "deep_lfo1Rate": 0.6, "deep_lfo1Depth": 0.5,
        "deep_filterCutoff": 450.0, "deep_filterRes": 0.4,
        "deep_pressureAmt": 0.5,
    }, (0.12, 0.65, 0.45, 0.2, 0.05, 0.18),
     "Bass with fast LFO surge. Filter opens and closes on each creature breath.",
     ["flux", "surge", "bass", "lfo"]),

    ("Downward Drift", {
        "deep_lfo2Rate": 0.03, "deep_lfo2Depth": 0.5,
        "deep_filterCutoff": 300.0, "deep_filterRes": 0.35,
        "deep_subOctMix": 0.5, "deep_pressureAmt": 0.55,
        "deep_reverbMix": 0.4,
        "deep_lfo1Depth": 0.1,
    }, (0.06, 0.72, 0.3, 0.2, 0.35, 0.05),
     "Ultra-slow LFO2 drift. The pitch descends imperceptibly over minutes.",
     ["flux", "drift", "slow", "descending"]),

    ("Flash Fauna", {
        "deep_bioMix": 0.5, "deep_bioRate": 0.3, "deep_bioBrightness": 3000.0,
        "deep_lfo1Rate": 0.4, "deep_lfo1Depth": 0.3,
        "deep_filterCutoff": 400.0,
        "deep_bodyMix": 0.15,
    }, (0.25, 0.55, 0.45, 0.25, 0.1, 0.1),
     "Rapid bioluminescent flashing with high brightness. Deep-sea light show.",
     ["flux", "flash", "fauna", "bright"]),

    ("Sediment Plume", {
        "deep_bodyChar": OPEN_WATER, "deep_bodyMix": 0.4, "deep_bodyFeedback": 0.6,
        "deep_lfo1Rate": 0.15, "deep_lfo1Depth": 0.3,
        "deep_lfo2Rate": 0.08, "deep_lfo2Depth": 0.25,
        "deep_filterCutoff": 300.0,
        "deep_bioMix": 0.2,
    }, (0.12, 0.65, 0.35, 0.25, 0.15, 0.08),
     "Disturbed sediment rising from the floor. Body resonance with dual LFO motion.",
     ["flux", "sediment", "plume", "movement"]),

    ("Thermal Shift", {
        "deep_lfo2Rate": 0.1, "deep_lfo2Depth": 0.4,
        "deep_lfo1Rate": 0.2, "deep_lfo1Depth": 0.3,
        "deep_filterCutoff": 350.0, "deep_filterRes": 0.45,
        "deep_pressureAmt": 0.5,
        "deep_bodyMix": 0.2, "deep_bodyChar": OPEN_WATER,
    }, (0.1, 0.65, 0.4, 0.2, 0.1, 0.08),
     "Temperature changes create filter and pressure flux. Warm and cold water mixing.",
     ["flux", "thermal", "shift", "temperature"]),

    ("Abyssal Wave", {
        "deep_lfo2Rate": 0.04, "deep_lfo2Depth": 0.5,
        "deep_filterCutoff": 250.0, "deep_subOctMix": 0.6,
        "deep_pressureAmt": 0.6,
        "deep_reverbMix": 0.35,
    }, (0.06, 0.75, 0.35, 0.2, 0.3, 0.05),
     "Deep internal wave. The slow oscillation of entire water masses.",
     ["flux", "wave", "internal", "slow"]),

    ("Wreck Dance", {
        "deep_bodyChar": WRECK, "deep_bodyMix": 0.6, "deep_bodyFeedback": 0.65,
        "deep_lfo1Rate": 0.4, "deep_lfo1Depth": 0.4,
        "deep_filterCutoff": 350.0, "deep_filterRes": 0.4,
        "deep_bioMix": 0.2,
    }, (0.15, 0.58, 0.4, 0.3, 0.15, 0.15),
     "Wreck body resonance animated by fast LFO. Metal singing in the current.",
     ["flux", "wreck", "dance", "resonant"]),

    ("Depth Charge Bounce", {
        "deep_ampAtk": 0.001, "deep_ampDec": 0.4, "deep_ampSus": 0.2, "deep_ampRel": 0.5,
        "deep_lfo1Rate": 1.5, "deep_lfo1Depth": 0.3,
        "deep_filterCutoff": 500.0, "deep_filterRes": 0.35,
        "deep_pressureAmt": 0.4,
    }, (0.15, 0.65, 0.4, 0.2, 0.05, 0.2),
     "Percussive sub with fast creature LFO. Bouncing impacts through the water.",
     ["flux", "bounce", "percussive", "fast"]),

    ("Filter Sweep", {
        "deep_lfo1Rate": 0.05, "deep_lfo1Depth": 0.8,
        "deep_filterCutoff": 400.0, "deep_filterRes": 0.55,
        "deep_pressureAmt": 0.4, "deep_subOctMix": 0.4,
    }, (0.1, 0.62, 0.5, 0.2, 0.05, 0.1),
     "Maximum LFO depth on the darkness filter. Full sweep between dark and less dark.",
     ["flux", "sweep", "filter", "maximum"]),

    ("Brine Cascade", {
        "deep_lfo1Rate": 0.3, "deep_lfo1Depth": 0.4,
        "deep_lfo2Rate": 0.12, "deep_lfo2Depth": 0.35,
        "deep_filterCutoff": 300.0, "deep_filterRes": 0.4,
        "deep_bodyMix": 0.3, "deep_bodyChar": CAVE,
        "deep_bioMix": 0.15,
        "deep_reverbMix": 0.35,
    }, (0.1, 0.62, 0.45, 0.25, 0.3, 0.08),
     "Dual LFO cascade through cave body. Dense brine flowing over the trench wall.",
     ["flux", "brine", "cascade", "dual-lfo"]),

    ("Darting Creature", {
        "deep_lfo1Rate": 2.0, "deep_lfo1Depth": 0.3,
        "deep_bioMix": 0.35, "deep_bioRate": 0.4, "deep_bioBrightness": 2500.0,
        "deep_filterCutoff": 450.0,
    }, (0.2, 0.55, 0.55, 0.22, 0.05, 0.12),
     "Maximum LFO1 rate. A deep-sea creature darting through the darkness.",
     ["flux", "darting", "creature", "fast"]),

    ("Tectonic Shift", {
        "deep_lfo2Rate": 0.02, "deep_lfo2Depth": 0.5,
        "deep_pressureAmt": 0.7, "deep_subOctMix": 0.7,
        "deep_filterCutoff": 200.0,
        "deep_bodyMix": 0.2, "deep_bodyChar": CAVE,
        "deep_reverbMix": 0.3,
    }, (0.04, 0.78, 0.2, 0.22, 0.25, 0.1),
     "Geological timescale movement. The slowest LFO2 shifts tectonic plates.",
     ["flux", "tectonic", "geological", "ultra-slow"]),

    ("Hadal Current", {
        "deep_lfo1Rate": 0.12, "deep_lfo1Depth": 0.4,
        "deep_lfo2Rate": 0.06, "deep_lfo2Depth": 0.3,
        "deep_filterCutoff": 280.0, "deep_filterRes": 0.4,
        "deep_bodyMix": 0.25,
        "deep_pressureAmt": 0.55,
    }, (0.08, 0.68, 0.4, 0.22, 0.1, 0.08),
     "Deep current in the hadal zone. Dual LFO creates complex movement at extreme depth.",
     ["flux", "hadal", "current", "dual-lfo"]),

    ("Rising Tide", {
        "deep_lfo1Rate": 0.1, "deep_lfo1Depth": 0.5,
        "deep_filterCutoff": 250.0, "deep_filterRes": 0.4,
        "deep_pressureAmt": 0.5,
        "deep_ampAtk": 0.2, "deep_ampSus": 0.85,
        "deep_reverbMix": 0.35,
    }, (0.08, 0.7, 0.4, 0.2, 0.3, 0.05),
     "LFO1 slowly opens the filter as if the tide is rising. Cyclical brightening.",
     ["flux", "tide", "rising", "cyclical"]),

    ("Submarine Flux", {
        "deep_lfo1Rate": 0.25, "deep_lfo1Depth": 0.45,
        "deep_lfo2Rate": 0.1, "deep_lfo2Depth": 0.35,
        "deep_filterCutoff": 350.0, "deep_filterRes": 0.45,
        "deep_bodyMix": 0.25, "deep_bodyChar": WRECK,
        "deep_pressureAmt": 0.5,
        "deep_bioMix": 0.15,
        "deep_reverbMix": 0.3,
    }, (0.1, 0.62, 0.45, 0.25, 0.25, 0.12),
     "Dual LFOs through wreck body. A submarine caught between currents and hull resonance.",
     ["flux", "submarine", "dual-lfo", "wreck"]),
]


# ══════════════════════════════════════════════════════════════════════════════
# AETHER (15) — experimental, extreme
# ══════════════════════════════════════════════════════════════════════════════

AETHER = [
    ("Trench Cosmos", {
        "deep_ampAtk": 0.5, "deep_ampSus": 0.95, "deep_ampRel": 8.0,
        "deep_filterCutoff": 100.0, "deep_filterRes": 0.3,
        "deep_subOctMix": 0.8, "deep_pressureAmt": 0.6,
        "deep_bodyMix": 0.2, "deep_bodyChar": CAVE,
        "deep_reverbMix": 0.7,
        "deep_lfo2Depth": 0.2, "deep_lfo2Rate": 0.02,
        "deep_macroAbyss": 0.5,
    }, (0.02, 0.82, 0.12, 0.18, 0.7, 0.0),
     "The cosmos beneath the ocean. Minimum filter, maximum reverb, maximum sub.",
     ["aether", "cosmos", "extreme", "reverb"]),

    ("Void Pressure", {
        "deep_filterCutoff": 60.0, "deep_filterRes": 0.2,
        "deep_pressureAmt": 0.95, "deep_subOctMix": 0.9,
        "deep_bodyMix": 0.0, "deep_bioMix": 0.0,
        "deep_reverbMix": 0.3,
        "deep_lfo1Depth": 0.0, "deep_lfo2Depth": 0.05,
    }, (0.01, 0.9, 0.02, 0.15, 0.25, 0.05),
     "Near-maximum pressure. Filter at near-minimum. Almost subsonic. Felt, not heard.",
     ["aether", "void", "pressure", "extreme"]),

    ("Eternal Trench", {
        "deep_ampAtk": 0.5, "deep_ampSus": 0.95, "deep_ampRel": 8.0,
        "deep_filterCutoff": 150.0, "deep_filterRes": 0.35,
        "deep_subOctMix": 0.6, "deep_pressureAmt": 0.5,
        "deep_bodyMix": 0.3, "deep_bodyChar": CAVE, "deep_bodyFeedback": 0.6,
        "deep_bioMix": 0.2, "deep_bioRate": 0.03,
        "deep_reverbMix": 0.6,
        "deep_lfo1Depth": 0.2, "deep_lfo2Depth": 0.2,
    }, (0.05, 0.72, 0.2, 0.25, 0.6, 0.0),
     "Infinite sustain, cave body, bio life. Hold one note and the trench opens forever.",
     ["aether", "eternal", "infinite", "sustain"]),

    ("Dark Infinity", {
        "deep_filterCutoff": 80.0, "deep_filterRes": 0.5,
        "deep_subOctMix": 0.7, "deep_pressureAmt": 0.7,
        "deep_ampRel": 8.0, "deep_ampSus": 0.9,
        "deep_reverbMix": 0.55,
        "deep_macroAbyss": 0.8,
    }, (0.01, 0.85, 0.05, 0.15, 0.55, 0.0),
     "ABYSS macro at maximum. Filter closes to near-nothing. Infinite darkness.",
     ["aether", "dark", "infinity", "maximum-abyss"]),

    ("Anglerfish Signal", {
        "deep_bioMix": 0.8, "deep_bioRate": 0.3, "deep_bioBrightness": 4000.0,
        "deep_filterCutoff": 400.0, "deep_filterRes": 0.3,
        "deep_bodyMix": 0.1,
        "deep_lfo1Depth": 0.5, "deep_lfo1Rate": 0.3,
        "deep_reverbMix": 0.4,
        "deep_macroCreature": 0.7,
    }, (0.3, 0.45, 0.5, 0.25, 0.35, 0.1),
     "Maximum bio exciter. The anglerfish lure at full brightness. Alien broadcast.",
     ["aether", "anglerfish", "signal", "bio-extreme"]),

    ("Crustal Resonance", {
        "deep_bodyChar": WRECK, "deep_bodyMix": 0.9, "deep_bodyFeedback": 0.85,
        "deep_filterCutoff": 200.0, "deep_filterRes": 0.6,
        "deep_pressureAmt": 0.6,
        "deep_reverbMix": 0.5,
        "deep_macroWreck": 0.8,
    }, (0.1, 0.55, 0.1, 0.4, 0.45, 0.2),
     "Maximum wreck body. Feedback near self-oscillation. The earth's crust vibrating.",
     ["aether", "crustal", "resonance", "extreme-body"]),

    ("Mariana Forever", {
        "deep_ampAtk": 0.5, "deep_ampSus": 0.98, "deep_ampRel": 8.0,
        "deep_filterCutoff": 120.0, "deep_filterRes": 0.3,
        "deep_subOctMix": 0.7, "deep_pressureAmt": 0.55,
        "deep_bodyMix": 0.25, "deep_bodyChar": CAVE,
        "deep_bioMix": 0.15, "deep_bioRate": 0.03,
        "deep_reverbMix": 0.65,
        "deep_lfo1Depth": 0.15, "deep_lfo2Depth": 0.2,
    }, (0.03, 0.78, 0.15, 0.2, 0.65, 0.0),
     "The Mariana Trench in drone form. Hold one note. The deepest place on Earth.",
     ["aether", "mariana", "drone", "deepest"]),

    ("Zero Gravity Sub", {
        "deep_pressureAmt": 0.0, "deep_subOctMix": 0.5,
        "deep_filterCutoff": 500.0, "deep_filterRes": 0.2,
        "deep_bodyMix": 0.0, "deep_bioMix": 0.0,
        "deep_reverbMix": 0.7,
        "deep_ampAtk": 0.3, "deep_ampRel": 6.0,
    }, (0.12, 0.6, 0.05, 0.12, 0.7, 0.0),
     "Zero pressure, maximum reverb. Sub bass floating weightless in underwater space.",
     ["aether", "zero-gravity", "weightless", "reverb"]),

    ("Hadal Heaven", {
        "deep_ampAtk": 0.4, "deep_ampSus": 0.9, "deep_ampRel": 7.0,
        "deep_filterCutoff": 250.0, "deep_filterRes": 0.3,
        "deep_subOctMix": 0.5,
        "deep_bodyMix": 0.3, "deep_bodyChar": CAVE,
        "deep_bioMix": 0.3, "deep_bioRate": 0.04, "deep_bioBrightness": 1200.0,
        "deep_reverbMix": 0.6,
        "deep_lfo1Depth": 0.25, "deep_lfo2Depth": 0.2,
        "deep_macroPressure": 0.3, "deep_macroCreature": 0.4,
    }, (0.1, 0.65, 0.3, 0.25, 0.6, 0.0),
     "Beauty at the deepest point. Cave, creature, and reverb create an underwater paradise.",
     ["aether", "heaven", "beautiful", "deep"]),

    ("Fathomless Dark", {
        "deep_filterCutoff": 50.0, "deep_filterRes": 0.15,
        "deep_subOctMix": 0.8, "deep_pressureAmt": 0.8,
        "deep_bodyMix": 0.0, "deep_bioMix": 0.0,
        "deep_reverbMix": 0.4,
        "deep_ampRel": 6.0,
        "deep_macroAbyss": 0.9,
    }, (0.0, 0.9, 0.0, 0.15, 0.35, 0.0),
     "Filter at absolute minimum. ABYSS at 90%. The sound of complete darkness.",
     ["aether", "fathomless", "dark", "extreme-minimum"]),

    ("Pelagic Eternity", {
        "deep_ampAtk": 0.5, "deep_ampSus": 0.95, "deep_ampRel": 8.0,
        "deep_filterCutoff": 200.0, "deep_filterRes": 0.3,
        "deep_subOctMix": 0.5,
        "deep_bodyMix": 0.2,
        "deep_reverbMix": 0.6,
        "deep_lfo2Depth": 0.3, "deep_lfo2Rate": 0.01,
    }, (0.05, 0.78, 0.1, 0.18, 0.6, 0.0),
     "LFO2 at 0.01 Hz — one cycle every 100 seconds. Geological patience.",
     ["aether", "eternity", "ultra-slow", "geological"]),

    ("Timeless Pressure", {
        "deep_pressureAmt": 0.8, "deep_subOctMix": 0.6,
        "deep_filterCutoff": 180.0, "deep_filterRes": 0.4,
        "deep_ampAtk": 0.3, "deep_ampSus": 0.92, "deep_ampRel": 7.0,
        "deep_bodyMix": 0.15, "deep_bodyChar": OPEN_WATER,
        "deep_reverbMix": 0.5,
        "deep_lfo1Depth": 0.1, "deep_lfo2Depth": 0.15,
    }, (0.04, 0.8, 0.12, 0.2, 0.5, 0.05),
     "Immense pressure sustained indefinitely. The weight of the entire ocean above.",
     ["aether", "timeless", "pressure", "immense"]),

    ("Beyond Depth", {
        "deep_filterCutoff": 100.0, "deep_filterRes": 0.4,
        "deep_subOctMix": 0.7, "deep_pressureAmt": 0.65,
        "deep_bodyChar": CAVE, "deep_bodyMix": 0.4, "deep_bodyFeedback": 0.6,
        "deep_bioMix": 0.2, "deep_bioBrightness": 500.0,
        "deep_reverbMix": 0.65,
        "deep_ampRel": 8.0, "deep_ampSus": 0.9,
        "deep_macroAbyss": 0.4, "deep_macroPressure": 0.3,
    }, (0.03, 0.75, 0.15, 0.25, 0.65, 0.0),
     "Deeper than the deepest trench. Every system engaged at extreme settings.",
     ["aether", "beyond", "extreme", "all-systems"]),

    ("Deep Space Sub", {
        "deep_subOctMix": 0.6, "deep_filterCutoff": 250.0,
        "deep_pressureAmt": 0.3, "deep_bodyMix": 0.0, "deep_bioMix": 0.0,
        "deep_reverbMix": 0.8,
        "deep_ampAtk": 0.4, "deep_ampRel": 8.0, "deep_ampSus": 0.9,
        "deep_lfo2Depth": 0.1,
    }, (0.06, 0.68, 0.05, 0.12, 0.8, 0.0),
     "Maximum reverb. Sub bass in an infinite cave. Sound in deep space.",
     ["aether", "space", "reverb", "infinite"]),

    ("The Long Dark", {
        "deep_filterCutoff": 70.0, "deep_filterRes": 0.3,
        "deep_subOctMix": 0.7, "deep_pressureAmt": 0.6,
        "deep_bodyMix": 0.15, "deep_bodyChar": CAVE,
        "deep_bioMix": 0.05, "deep_bioRate": 0.02,
        "deep_reverbMix": 0.55,
        "deep_ampAtk": 0.5, "deep_ampSus": 0.95, "deep_ampRel": 8.0,
        "deep_lfo1Depth": 0.05, "deep_lfo2Depth": 0.1,
        "deep_macroAbyss": 0.6,
    }, (0.01, 0.85, 0.08, 0.15, 0.55, 0.0),
     "The longest, darkest sound. Filter at 70 Hz. A single note that lasts forever.",
     ["aether", "long", "dark", "final"]),
]


# ══════════════════════════════════════════════════════════════════════════════
# FAMILY (15) — OCEANDEEP + fleet engine coupling
# ══════════════════════════════════════════════════════════════════════════════

FAMILY_PARTNERS = [
    ("Overdub", "Trench x Dub",
     "Dub echo from the deep. OceanDeep provides the weight; Overdub provides the space.",
     {"deep_filterCutoff": 300.0, "deep_bodyChar": CAVE, "deep_bodyMix": 0.4, "deep_reverbMix": 0.4, "deep_pressureAmt": 0.5},
     (0.1, 0.68, 0.15, 0.25, 0.4, 0.08), ["family", "overdub", "dub", "echo"]),

    ("Odyssey", "Trench x Drift",
     "Two wanderers in the dark. OceanDeep anchors the drift.",
     {"deep_filterCutoff": 250.0, "deep_subOctMix": 0.5, "deep_bodyMix": 0.2, "deep_reverbMix": 0.45, "deep_lfo2Depth": 0.2},
     (0.07, 0.72, 0.2, 0.2, 0.4, 0.0), ["family", "odyssey", "drift", "anchor"]),

    ("Onset", "Trench x Drums",
     "Percussive depth. OceanDeep sub layers beneath Onset hits.",
     {"deep_ampAtk": 0.001, "deep_ampDec": 0.3, "deep_ampSus": 0.1, "deep_filterCutoff": 500.0, "deep_subOctMix": 0.6, "deep_pressureAmt": 0.4},
     (0.14, 0.7, 0.05, 0.2, 0.0, 0.2), ["family", "onset", "drums", "percussive"]),

    ("Overworld", "Trench x Chip",
     "The deepest bass meets 8-bit chips. Geological NES.",
     {"deep_filterCutoff": 400.0, "deep_bodyMix": 0.2, "deep_pressureAmt": 0.4, "deep_subOctMix": 0.4},
     (0.12, 0.68, 0.1, 0.2, 0.05, 0.1), ["family", "overworld", "chip", "retro"]),

    ("Opal", "Trench x Grains",
     "Granular textures floating over abyssal bass. Marine snow.",
     {"deep_filterCutoff": 300.0, "deep_bodyMix": 0.25, "deep_bioMix": 0.2, "deep_reverbMix": 0.45, "deep_lfo1Depth": 0.15},
     (0.1, 0.65, 0.2, 0.25, 0.45, 0.0), ["family", "opal", "granular", "texture"]),

    ("Organon", "Trench x Metabolism",
     "Living bass. OceanDeep pressure meets Organon metabolic modulation.",
     {"deep_filterCutoff": 280.0, "deep_pressureAmt": 0.6, "deep_bodyMix": 0.3, "deep_lfo1Depth": 0.2, "deep_lfo2Depth": 0.2},
     (0.08, 0.7, 0.25, 0.22, 0.2, 0.05), ["family", "organon", "metabolic", "living"]),

    ("Ouroboros", "Trench x Feedback",
     "Infinite recursion in the abyss. Self-eating bass.",
     {"deep_bodyChar": WRECK, "deep_bodyMix": 0.5, "deep_bodyFeedback": 0.7, "deep_filterCutoff": 250.0, "deep_filterRes": 0.5, "deep_pressureAmt": 0.6},
     (0.1, 0.6, 0.15, 0.3, 0.15, 0.25), ["family", "ouroboros", "feedback", "infinite"]),

    ("Oblong", "Trench x Amber",
     "Warm amber tone meets trench darkness. Light above, weight below.",
     {"deep_filterCutoff": 350.0, "deep_bodyMix": 0.2, "deep_subOctMix": 0.4, "deep_reverbMix": 0.3},
     (0.1, 0.72, 0.1, 0.18, 0.25, 0.05), ["family", "oblong", "warm", "amber"]),

    ("Obese", "Trench x Fat",
     "Maximum weight. OceanDeep pressure plus Obese saturation.",
     {"deep_pressureAmt": 0.8, "deep_filterCutoff": 300.0, "deep_bodyChar": WRECK, "deep_bodyMix": 0.3, "deep_subOctMix": 0.6},
     (0.08, 0.7, 0.05, 0.3, 0.05, 0.35), ["family", "obese", "fat", "maximum-weight"]),

    ("Oceanic", "Trench x Boids",
     "Deep bass anchoring Oceanic's schooling algorithm above.",
     {"deep_filterCutoff": 300.0, "deep_subOctMix": 0.5, "deep_bodyMix": 0.2, "deep_reverbMix": 0.4, "deep_lfo1Depth": 0.15},
     (0.08, 0.7, 0.15, 0.22, 0.35, 0.0), ["family", "oceanic", "boids", "schooling"]),

    ("Oracle", "Trench x Prophecy",
     "Prophetic bass from the abyss. Stochastic tones over deep pressure.",
     {"deep_filterCutoff": 250.0, "deep_filterRes": 0.4, "deep_pressureAmt": 0.5, "deep_bodyChar": CAVE, "deep_bodyMix": 0.3, "deep_reverbMix": 0.4},
     (0.08, 0.68, 0.15, 0.22, 0.35, 0.08), ["family", "oracle", "prophecy", "stochastic"]),

    ("Overlap", "Trench x Knot",
     "Topological bass. OceanDeep pressure entangled with Overlap FDN.",
     {"deep_filterCutoff": 300.0, "deep_bodyMix": 0.35, "deep_bodyChar": WRECK, "deep_pressureAmt": 0.5, "deep_reverbMix": 0.35},
     (0.1, 0.65, 0.15, 0.28, 0.3, 0.1), ["family", "overlap", "knot", "entangled"]),

    ("Obrix", "Trench x Reef",
     "The reef grows on the trench wall. Modular coral over abyssal bass.",
     {"deep_filterCutoff": 350.0, "deep_bodyMix": 0.25, "deep_pressureAmt": 0.4, "deep_bioMix": 0.15, "deep_reverbMix": 0.3},
     (0.12, 0.68, 0.15, 0.22, 0.25, 0.08), ["family", "obrix", "reef", "coral"]),

    ("OpenSky", "Trench x Sky",
     "The full water column: OceanDeep at the floor, OpenSky at the surface.",
     {"deep_filterCutoff": 200.0, "deep_subOctMix": 0.6, "deep_pressureAmt": 0.6, "deep_reverbMix": 0.5, "deep_macroAbyss": 0.3},
     (0.05, 0.75, 0.1, 0.2, 0.45, 0.0), ["family", "opensky", "column", "surface-to-floor"]),

    ("Osprey", "Trench x Shore",
     "Shore meets abyss. The transition from light to darkness.",
     {"deep_filterCutoff": 350.0, "deep_bodyMix": 0.2, "deep_subOctMix": 0.4, "deep_reverbMix": 0.4, "deep_bioMix": 0.1},
     (0.1, 0.7, 0.1, 0.18, 0.35, 0.03), ["family", "osprey", "shore", "transition"]),
]

FAMILY_PRESETS = []
for partner_id, name, desc, overrides, dna, tags in FAMILY_PARTNERS:
    FAMILY_PRESETS.append(make_preset(name, "Family", overrides, dna, desc, tags,
        engines=["OceanDeep", partner_id],
        coupling=[{"engineA": "OceanDeep", "engineB": partner_id,
                   "type": "AmpToFilter", "amount": 0.5}],
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

    # Prism: Techno (5) + Dub (5) + DnB (5) + Trap (5) + Ambient/Score (5) = 25
    for name, overrides, dna, desc, tags in TECHNO:
        save(make_preset(name, "Prism", overrides, dna, desc, tags))
        count += 1
    for name, overrides, dna, desc, tags in DUB:
        save(make_preset(name, "Prism", overrides, dna, desc, tags))
        count += 1
    for name, overrides, dna, desc, tags in DNB:
        save(make_preset(name, "Prism", overrides, dna, desc, tags))
        count += 1
    for name, overrides, dna, desc, tags in TRAP:
        save(make_preset(name, "Prism", overrides, dna, desc, tags))
        count += 1
    for name, overrides, dna, desc, tags in AMBIENT_SCORE:
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

    print(f"Generated {count} OCEANDEEP presets across 7 moods")

    # Distribution check
    from collections import Counter
    moods = Counter()
    moods["Foundation"] = len(FOUNDATION)
    moods["Atmosphere"] = len(ATMOSPHERE)
    moods["Entangled"] = len(ENTANGLED)
    moods["Prism"] = len(TECHNO) + len(DUB) + len(DNB) + len(TRAP) + len(AMBIENT_SCORE)
    moods["Flux"] = len(FLUX)
    moods["Aether"] = len(AETHER)
    moods["Family"] = len(FAMILY_PRESETS)
    for mood, n in sorted(moods.items()):
        print(f"  {mood}: {n}")
    print(f"  TOTAL: {sum(moods.values())}")


if __name__ == "__main__":
    main()
