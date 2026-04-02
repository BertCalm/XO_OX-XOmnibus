#!/usr/bin/env python3
"""ORGANISM Preset Generator — 150 factory presets for the Coral Colony engine.

Generates .xometa JSON files across 7 moods:
  Foundation (30): teaching presets + basic building blocks
  Atmosphere (25): ambient / evolving / environmental
  Entangled (20): coupling showcases
  Prism (25): genre production (techno/ambient-techno/idm/experimental-pop)
  Flux (20): movement-forward presets
  Aether (15): deep generative / experimental
  Family (15): ORGANISM + fleet engine coupling
"""

import json
import os

PRESET_DIR = os.path.join(os.path.dirname(__file__), "..", "Presets", "XOceanus")

# ── ORGANISM defaults (24 params) ────────────────────────────────────────────
DEFAULTS = {
    "org_rule": 110.0,         # Wolfram rule (0-255), default Rule 110 (Turing-complete)
    "org_seed": 42,            # Initial 16-bit CA state
    "org_stepRate": 4.0,       # CA steps per second (0.5-32)
    "org_scope": 4,            # Scope averaging window (1-16 generations)
    "org_mutate": 0.0,         # Mutation probability per cell (0-1)
    "org_freeze": 0,           # Freeze automaton (bool)
    "org_oscWave": 0,          # 0=saw, 1=square, 2=tri
    "org_subLevel": 0.35,      # Sub oscillator level (0-1)
    "org_filterCutoff": 3000.0,# Base filter cutoff (200-8000 Hz)
    "org_filterRes": 0.3,      # Filter resonance (0-0.9)
    "org_velCutoff": 0.5,      # Velocity->cutoff depth (0-1)
    "org_ampAtk": 0.015,       # Amp attack (0.001-2.0 s)
    "org_ampDec": 0.35,        # Amp decay (0.05-4.0 s)
    "org_ampSus": 0.7,         # Amp sustain (0-1)
    "org_ampRel": 0.6,         # Amp release (0.05-5.0 s)
    "org_lfo1Rate": 0.5,       # LFO1 rate (0.01-10 Hz)
    "org_lfo1Depth": 0.2,      # LFO1 depth (0-1)
    "org_lfo2Rate": 0.3,       # LFO2 rate (0.01-10 Hz)
    "org_lfo2Depth": 0.25,     # LFO2 depth (0-1)
    "org_reverbMix": 0.2,      # Reverb wet/dry (0-1)
    "org_macroRule": 0.25,     # RULE macro (0-1, maps across 8 curated rules)
    "org_macroSeed": 0.0,      # SEED macro (0-1, re-seeds when >0.01)
    "org_macroCoupling": 0.0,  # COUPLING macro (0-1)
    "org_macroMutate": 0.0,    # MUTATE macro (0-1)
}

# Wolfram rule constants (the 8 curated rules indexed by macroRule 0-1)
# Index: 0=Rule30, 1=Rule90, 2=Rule110, 3=Rule184, 4=Rule150, 5=Rule18, 6=Rule54, 7=Rule22
# macroRule 0.0 -> Rule 30 (chaotic), 0.14 -> Rule 90 (fractal)
# macroRule 0.25 -> Rule 110 (complex/Turing-complete, default)
# macroRule 0.43 -> Rule 184 (traffic flow), 0.57 -> Rule 150 (additive)
# macroRule 0.71 -> Rule 18 (nested), 0.86 -> Rule 54 (complex nested)
# macroRule 1.0 -> Rule 22 (triangular)

# Oscillator wave constants
SAW, SQUARE, TRI = 0, 1, 2


def make_preset(name, mood, overrides, dna, desc="", tags=None, engines=None,
                coupling=None, coupling_intensity="None"):
    """Build a full .xometa dict from name + param overrides."""
    params = dict(DEFAULTS)
    params.update(overrides)

    preset = {
        "schema_version": 1,
        "name": name,
        "mood": mood,
        "engines": engines or ["Organism"],
        "author": "XO_OX Designs",
        "version": "1.0.0",
        "description": desc,
        "tags": tags or [],
        "macroLabels": ["RULE", "SEED", "COUPLING", "MUTATE"],
        "couplingIntensity": coupling_intensity,
        "tempo": None,
        "dna": {
            "brightness": dna[0], "warmth": dna[1], "movement": dna[2],
            "density": dna[3], "space": dna[4], "aggression": dna[5]
        },
        "parameters": {"Organism": params},
    }
    if coupling:
        preset["coupling"] = {"pairs": coupling}
    return preset


def save(preset):
    """Write preset to the correct mood directory."""
    mood = preset["mood"]
    safe_name = preset["name"].replace(" ", "_").replace("'", "").replace(",", "")
    filename = f"Organism_{safe_name}.xometa"
    path = os.path.join(PRESET_DIR, mood, filename)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w") as f:
        json.dump(preset, f, indent=2)
    return path


# ══════════════════════════════════════════════════════════════════════════════
# FOUNDATION (30): 15 Lesson presets + 15 basic building blocks
# ══════════════════════════════════════════════════════════════════════════════

LESSONS = [
    # 1. One Colony — default Rule 110, hear the automaton
    ("One Colony", {
        "org_rule": 110.0, "org_seed": 42, "org_stepRate": 4.0,
        "org_scope": 4, "org_mutate": 0.0,
        "org_oscWave": SAW, "org_subLevel": 0.0,
        "org_filterCutoff": 4000.0, "org_filterRes": 0.2,
        "org_lfo1Depth": 0.0, "org_lfo2Depth": 0.0,
        "org_reverbMix": 0.0,
    }, (0.5, 0.5, 0.4, 0.2, 0.0, 0.1),
     "Rule 110. The Turing-complete colony. Listen to the automaton evolve the filter and pitch.",
     ["lesson", "rule110", "basic", "beginner"]),

    # 2. Slow Cells — reduced step rate, hear each generation
    ("Slow Cells", {
        "org_rule": 110.0, "org_stepRate": 1.0, "org_scope": 1,
        "org_oscWave": SAW, "org_subLevel": 0.0,
        "org_filterCutoff": 3000.0, "org_filterRes": 0.3,
        "org_lfo1Depth": 0.0, "org_lfo2Depth": 0.0,
        "org_reverbMix": 0.0,
    }, (0.45, 0.5, 0.2, 0.2, 0.0, 0.1),
     "One step per second. Each generation is audible. Watch the filter jump.",
     ["lesson", "slow", "scope-1", "raw"]),

    # 3. Smoothed Evolution — high scope, hear the averaging
    ("Smoothed Evolution", {
        "org_rule": 110.0, "org_stepRate": 4.0, "org_scope": 12,
        "org_oscWave": SAW, "org_subLevel": 0.0,
        "org_filterCutoff": 3000.0, "org_filterRes": 0.3,
        "org_lfo1Depth": 0.0, "org_lfo2Depth": 0.0,
        "org_reverbMix": 0.0,
    }, (0.45, 0.55, 0.35, 0.2, 0.0, 0.05),
     "Same rule, high scope (12). The scope averages generations, smoothing the evolution.",
     ["lesson", "scope", "smooth", "averaged"]),

    # 4. Chaotic Rule — Rule 30, maximum entropy
    ("Chaotic Rule", {
        "org_rule": 30.0, "org_stepRate": 6.0, "org_scope": 2,
        "org_oscWave": SAW, "org_subLevel": 0.2,
        "org_filterCutoff": 4000.0, "org_filterRes": 0.35,
        "org_lfo1Depth": 0.0, "org_lfo2Depth": 0.0,
        "org_reverbMix": 0.0,
    }, (0.5, 0.45, 0.6, 0.3, 0.0, 0.2),
     "Rule 30. Wolfram's favorite chaotic rule. Fast, unpredictable, alive.",
     ["lesson", "rule30", "chaos", "fast"]),

    # 5. Traffic Flow — Rule 184, ordered patterns
    ("Traffic Flow", {
        "org_rule": 184.0, "org_stepRate": 4.0, "org_scope": 4,
        "org_oscWave": SQUARE, "org_subLevel": 0.3,
        "org_filterCutoff": 2500.0, "org_filterRes": 0.3,
        "org_lfo1Depth": 0.0, "org_lfo2Depth": 0.0,
        "org_reverbMix": 0.0,
    }, (0.4, 0.55, 0.3, 0.25, 0.0, 0.1),
     "Rule 184 simulates traffic flow. More ordered than Rule 30. Rhythmic patterns emerge.",
     ["lesson", "rule184", "traffic", "ordered"]),

    # 6. Fractal Pattern — Rule 90, Sierpinski triangle
    ("Fractal Pattern", {
        "org_rule": 90.0, "org_stepRate": 8.0, "org_scope": 3,
        "org_oscWave": TRI, "org_subLevel": 0.2,
        "org_filterCutoff": 3500.0, "org_filterRes": 0.25,
        "org_lfo1Depth": 0.0, "org_lfo2Depth": 0.0,
        "org_reverbMix": 0.1,
    }, (0.45, 0.5, 0.5, 0.25, 0.1, 0.1),
     "Rule 90 generates Sierpinski triangles. Self-similar patterns at every scale.",
     ["lesson", "rule90", "fractal", "sierpinski"]),

    # 7. Sub Depths — sub oscillator lesson
    ("Sub Depths", {
        "org_rule": 110.0, "org_stepRate": 2.0, "org_scope": 6,
        "org_oscWave": SAW, "org_subLevel": 0.8,
        "org_filterCutoff": 2000.0, "org_filterRes": 0.3,
        "org_lfo1Depth": 0.0, "org_lfo2Depth": 0.0,
        "org_reverbMix": 0.0,
    }, (0.3, 0.7, 0.25, 0.25, 0.0, 0.1),
     "Sub level at 80%. The square sub one octave below grounds the evolving saw.",
     ["lesson", "sub", "octave", "foundation"]),

    # 8. Mutation Lesson — introduce entropy
    ("Mutation Lesson", {
        "org_rule": 110.0, "org_stepRate": 4.0, "org_scope": 4,
        "org_mutate": 0.15,
        "org_oscWave": SAW, "org_subLevel": 0.3,
        "org_filterCutoff": 3500.0, "org_filterRes": 0.3,
        "org_lfo1Depth": 0.0, "org_lfo2Depth": 0.0,
        "org_reverbMix": 0.0,
    }, (0.5, 0.5, 0.5, 0.25, 0.0, 0.15),
     "15% mutation rate. Random bit flips per step. The colony never settles.",
     ["lesson", "mutation", "entropy", "random"]),

    # 9. LFO Breathes — add LFO to the automaton
    ("LFO Breathes", {
        "org_rule": 110.0, "org_stepRate": 4.0, "org_scope": 6,
        "org_oscWave": SAW, "org_subLevel": 0.3,
        "org_filterCutoff": 2500.0, "org_filterRes": 0.35,
        "org_lfo1Rate": 0.3, "org_lfo1Depth": 0.4,
        "org_lfo2Rate": 0.15, "org_lfo2Depth": 0.3,
        "org_reverbMix": 0.0,
    }, (0.4, 0.55, 0.5, 0.25, 0.0, 0.1),
     "LFOs add breathing to the automaton's filter control. Two layers of modulation.",
     ["lesson", "lfo", "modulation", "breathing"]),

    # 10. Velocity Expression — D001 velocity-to-timbre
    ("Velocity Expression", {
        "org_rule": 110.0, "org_stepRate": 4.0,
        "org_oscWave": SAW, "org_subLevel": 0.3,
        "org_filterCutoff": 1500.0, "org_filterRes": 0.4,
        "org_velCutoff": 0.8,
        "org_lfo1Depth": 0.1, "org_lfo2Depth": 0.1,
        "org_reverbMix": 0.0,
    }, (0.45, 0.55, 0.35, 0.25, 0.0, 0.2),
     "Play soft: the colony is dark. Play hard: it brightens. Velocity shapes the filter.",
     ["lesson", "velocity", "dynamics", "expression"]),

    # 11. Mod Wheel Rule — D006 mod wheel morphs the rule
    ("Mod Wheel Rule", {
        "org_rule": 110.0, "org_stepRate": 4.0,
        "org_macroRule": 0.25,
        "org_oscWave": SAW, "org_subLevel": 0.3,
        "org_filterCutoff": 3000.0, "org_filterRes": 0.3,
        "org_lfo1Depth": 0.15, "org_lfo2Depth": 0.2,
        "org_reverbMix": 0.1,
    }, (0.5, 0.5, 0.4, 0.25, 0.1, 0.1),
     "Move the mod wheel. It morphs between curated Wolfram rules. The colony's law changes.",
     ["lesson", "modwheel", "rule-morph", "expression"]),

    # 12. Aftertouch Entropy — D006 pressure adds mutation
    ("Aftertouch Entropy", {
        "org_rule": 110.0, "org_stepRate": 4.0,
        "org_mutate": 0.0,
        "org_oscWave": SAW, "org_subLevel": 0.3,
        "org_filterCutoff": 3000.0, "org_filterRes": 0.3,
        "org_lfo1Depth": 0.15, "org_lfo2Depth": 0.2,
        "org_reverbMix": 0.1,
    }, (0.5, 0.5, 0.4, 0.25, 0.1, 0.15),
     "Press into the key. Aftertouch adds mutation. Light touch = stable. Firm = chaotic.",
     ["lesson", "aftertouch", "mutation", "pressure"]),

    # 13. Macro Sweep — all four macros demonstrated
    ("Macro Sweep", {
        "org_rule": 110.0, "org_stepRate": 4.0,
        "org_oscWave": SAW, "org_subLevel": 0.35,
        "org_filterCutoff": 3000.0, "org_filterRes": 0.3,
        "org_lfo1Depth": 0.2, "org_lfo2Depth": 0.2,
        "org_reverbMix": 0.15,
        "org_macroRule": 0.25, "org_macroMutate": 0.1,
    }, (0.5, 0.5, 0.45, 0.25, 0.15, 0.1),
     "RULE sweeps the law. SEED re-rolls. COUPLING adds sensitivity. MUTATE adds entropy.",
     ["lesson", "macros", "performance", "overview"]),

    # 14. Reverb Colony — adding space
    ("Reverb Colony", {
        "org_rule": 110.0, "org_stepRate": 3.0, "org_scope": 8,
        "org_oscWave": SAW, "org_subLevel": 0.3,
        "org_filterCutoff": 2500.0, "org_filterRes": 0.25,
        "org_lfo1Depth": 0.15, "org_lfo2Depth": 0.2,
        "org_reverbMix": 0.5,
    }, (0.4, 0.55, 0.35, 0.25, 0.5, 0.05),
     "Reverb at 50%. Cells 12-15 also modulate the reverb send. Space breathes with the colony.",
     ["lesson", "reverb", "space", "modulated"]),

    # 15. Full Colony — everything combined
    ("Full Colony", {
        "org_rule": 110.0, "org_stepRate": 4.0, "org_scope": 6,
        "org_mutate": 0.08,
        "org_oscWave": SAW, "org_subLevel": 0.35,
        "org_filterCutoff": 2800.0, "org_filterRes": 0.35,
        "org_velCutoff": 0.6,
        "org_lfo1Rate": 0.3, "org_lfo1Depth": 0.25,
        "org_lfo2Rate": 0.2, "org_lfo2Depth": 0.3,
        "org_reverbMix": 0.25,
        "org_macroRule": 0.25, "org_macroMutate": 0.1,
    }, (0.5, 0.55, 0.55, 0.3, 0.25, 0.15),
     "Every element active. Mutation, LFOs, velocity, reverb. The full living colony.",
     ["lesson", "complete", "showcase", "full"]),
]

FOUNDATION_BASICS = [
    ("Clean Saw Colony", {
        "org_rule": 110.0, "org_stepRate": 4.0, "org_scope": 4,
        "org_oscWave": SAW, "org_subLevel": 0.0,
        "org_filterCutoff": 5000.0, "org_filterRes": 0.2,
        "org_lfo1Depth": 0.1, "org_lfo2Depth": 0.15,
    }, (0.6, 0.45, 0.35, 0.2, 0.0, 0.1),
     "Bright saw through open filter. The colony provides subtle movement.",
     ["saw", "clean", "basic"]),

    ("Square Pulse", {
        "org_rule": 90.0, "org_stepRate": 6.0, "org_scope": 3,
        "org_oscWave": SQUARE, "org_subLevel": 0.2,
        "org_filterCutoff": 3000.0, "org_filterRes": 0.3,
        "org_lfo1Depth": 0.15, "org_lfo2Depth": 0.1,
    }, (0.5, 0.5, 0.4, 0.25, 0.0, 0.15),
     "Square wave through fractal Rule 90. Rhythmic and angular.",
     ["square", "pulse", "fractal"]),

    ("Triangle Soft", {
        "org_rule": 184.0, "org_stepRate": 2.0, "org_scope": 8,
        "org_oscWave": TRI, "org_subLevel": 0.3,
        "org_filterCutoff": 4000.0, "org_filterRes": 0.2,
        "org_lfo1Depth": 0.1, "org_lfo2Depth": 0.2,
        "org_reverbMix": 0.15,
    }, (0.4, 0.6, 0.3, 0.2, 0.15, 0.0),
     "Gentle triangle with ordered Rule 184. Smooth and warm.",
     ["triangle", "soft", "warm"]),

    ("Deep Sub Colony", {
        "org_rule": 110.0, "org_stepRate": 2.0, "org_scope": 8,
        "org_oscWave": SAW, "org_subLevel": 0.9,
        "org_filterCutoff": 1200.0, "org_filterRes": 0.2,
        "org_lfo1Depth": 0.0, "org_lfo2Depth": 0.1,
    }, (0.2, 0.75, 0.2, 0.2, 0.0, 0.1),
     "Saw + heavy sub. Dark and grounded. Colony barely moves the filter.",
     ["sub", "deep", "dark"]),

    ("Bright Evolve", {
        "org_rule": 30.0, "org_stepRate": 8.0, "org_scope": 2,
        "org_oscWave": SAW, "org_subLevel": 0.2,
        "org_filterCutoff": 5000.0, "org_filterRes": 0.35,
        "org_velCutoff": 0.7,
        "org_lfo1Depth": 0.2, "org_lfo2Depth": 0.15,
    }, (0.6, 0.4, 0.6, 0.3, 0.0, 0.2),
     "Chaotic Rule 30 at high speed. Bright, fast, unpredictable.",
     ["bright", "chaotic", "fast"]),

    ("Resonant Colony", {
        "org_rule": 150.0, "org_stepRate": 4.0, "org_scope": 4,
        "org_oscWave": SAW, "org_subLevel": 0.3,
        "org_filterCutoff": 2000.0, "org_filterRes": 0.7,
        "org_lfo1Depth": 0.2, "org_lfo2Depth": 0.25,
    }, (0.4, 0.5, 0.45, 0.3, 0.0, 0.25),
     "High resonance filter. The colony's cutoff changes are amplified by resonance peaks.",
     ["resonant", "filter", "sharp"]),

    ("Slow Bloom", {
        "org_rule": 54.0, "org_stepRate": 1.0, "org_scope": 12,
        "org_oscWave": TRI, "org_subLevel": 0.4,
        "org_filterCutoff": 2500.0, "org_filterRes": 0.25,
        "org_ampAtk": 0.8, "org_ampRel": 2.0,
        "org_lfo1Rate": 0.05, "org_lfo1Depth": 0.2,
        "org_lfo2Rate": 0.03, "org_lfo2Depth": 0.25,
        "org_reverbMix": 0.3,
    }, (0.35, 0.6, 0.35, 0.2, 0.3, 0.0),
     "Slow attack, slow step rate, high scope. The colony unfolds over seconds.",
     ["slow", "bloom", "pad"]),

    ("Percussive Cell", {
        "org_rule": 30.0, "org_stepRate": 16.0, "org_scope": 1,
        "org_oscWave": SAW, "org_subLevel": 0.2,
        "org_filterCutoff": 4000.0, "org_filterRes": 0.4,
        "org_ampAtk": 0.001, "org_ampDec": 0.2, "org_ampSus": 0.0,
        "org_ampRel": 0.3,
        "org_lfo1Depth": 0.0, "org_lfo2Depth": 0.0,
    }, (0.55, 0.4, 0.15, 0.25, 0.0, 0.25),
     "Zero attack, zero sustain. Each note is a snapshot of one CA generation.",
     ["percussive", "short", "snapshot"]),

    ("Frozen State", {
        "org_rule": 110.0, "org_seed": 12345,
        "org_freeze": 1, "org_stepRate": 4.0,
        "org_oscWave": SAW, "org_subLevel": 0.3,
        "org_filterCutoff": 3000.0, "org_filterRes": 0.3,
        "org_lfo1Depth": 0.15, "org_lfo2Depth": 0.2,
        "org_reverbMix": 0.1,
    }, (0.45, 0.55, 0.15, 0.2, 0.1, 0.05),
     "Freeze ON. The automaton holds one state. A static timbre captured from the colony.",
     ["frozen", "static", "captured"]),

    ("Fast Cells", {
        "org_rule": 110.0, "org_stepRate": 32.0, "org_scope": 2,
        "org_oscWave": SAW, "org_subLevel": 0.2,
        "org_filterCutoff": 4000.0, "org_filterRes": 0.3,
        "org_lfo1Depth": 0.1, "org_lfo2Depth": 0.1,
    }, (0.55, 0.45, 0.7, 0.3, 0.0, 0.2),
     "32 steps per second. Maximum speed. The colony becomes a texture.",
     ["fast", "texture", "32hz"]),

    ("Wide Scope", {
        "org_rule": 110.0, "org_stepRate": 4.0, "org_scope": 16,
        "org_oscWave": SAW, "org_subLevel": 0.35,
        "org_filterCutoff": 2800.0, "org_filterRes": 0.3,
        "org_lfo1Depth": 0.2, "org_lfo2Depth": 0.2,
        "org_reverbMix": 0.2,
    }, (0.4, 0.55, 0.3, 0.2, 0.2, 0.05),
     "Maximum scope (16). The colony's output is smoothed across 16 generations.",
     ["scope", "maximum", "smooth"]),

    ("Mutant Colony", {
        "org_rule": 110.0, "org_stepRate": 6.0, "org_scope": 4,
        "org_mutate": 0.35,
        "org_oscWave": SAW, "org_subLevel": 0.3,
        "org_filterCutoff": 3500.0, "org_filterRes": 0.35,
        "org_lfo1Depth": 0.15, "org_lfo2Depth": 0.2,
        "org_reverbMix": 0.15,
    }, (0.5, 0.5, 0.6, 0.3, 0.15, 0.2),
     "35% mutation rate. The colony never finds equilibrium. Constant evolution.",
     ["mutant", "high-entropy", "evolving"]),

    ("Rule 22 Nested", {
        "org_rule": 22.0, "org_stepRate": 5.0, "org_scope": 5,
        "org_oscWave": TRI, "org_subLevel": 0.3,
        "org_filterCutoff": 3000.0, "org_filterRes": 0.3,
        "org_lfo1Depth": 0.2, "org_lfo2Depth": 0.15,
        "org_reverbMix": 0.15,
    }, (0.4, 0.55, 0.4, 0.25, 0.15, 0.05),
     "Rule 22 produces nested triangular patterns. Geometric and ordered.",
     ["rule22", "nested", "geometric"]),

    ("Rule 18 Sparse", {
        "org_rule": 18.0, "org_stepRate": 3.0, "org_scope": 6,
        "org_oscWave": SQUARE, "org_subLevel": 0.25,
        "org_filterCutoff": 2500.0, "org_filterRes": 0.3,
        "org_lfo1Depth": 0.15, "org_lfo2Depth": 0.2,
        "org_reverbMix": 0.2,
    }, (0.4, 0.55, 0.3, 0.2, 0.2, 0.1),
     "Rule 18 is sparse — few cells alive at any time. Quiet, contemplative evolution.",
     ["rule18", "sparse", "contemplative"]),

    ("Init Patch", {
    }, (0.5, 0.5, 0.35, 0.2, 0.2, 0.1),
     "Factory defaults. Rule 110, seed 42, saw + sub. The starting point.",
     ["init", "default", "starting-point"]),
]

# ══════════════════════════════════════════════════════════════════════════════
# ATMOSPHERE (25): ambient / evolving / environmental
# ══════════════════════════════════════════════════════════════════════════════

ATMOSPHERE = [
    ("Coral Garden", {
        "org_rule": 110.0, "org_stepRate": 2.0, "org_scope": 10,
        "org_oscWave": SAW, "org_subLevel": 0.35,
        "org_filterCutoff": 2200.0, "org_filterRes": 0.25,
        "org_ampAtk": 0.8, "org_ampRel": 2.0,
        "org_lfo1Rate": 0.08, "org_lfo1Depth": 0.25,
        "org_lfo2Rate": 0.05, "org_lfo2Depth": 0.3,
        "org_reverbMix": 0.45,
    }, (0.35, 0.65, 0.4, 0.25, 0.5, 0.0),
     "Slow, smooth, warm. The colony grows quietly in filtered reverb.",
     ["ambient", "coral", "warm", "reverb"]),

    ("Tide Pool Dawn", {
        "org_rule": 184.0, "org_stepRate": 1.5, "org_scope": 12,
        "org_oscWave": TRI, "org_subLevel": 0.3,
        "org_filterCutoff": 2800.0, "org_filterRes": 0.2,
        "org_ampAtk": 1.0, "org_ampRel": 2.5,
        "org_lfo1Rate": 0.04, "org_lfo1Depth": 0.2,
        "org_lfo2Rate": 0.06, "org_lfo2Depth": 0.25,
        "org_reverbMix": 0.4,
    }, (0.35, 0.6, 0.3, 0.2, 0.45, 0.0),
     "Traffic-flow rule, gentle triangle. First light on still water.",
     ["ambient", "dawn", "gentle", "tide"]),

    ("Reef Mist", {
        "org_rule": 54.0, "org_stepRate": 2.0, "org_scope": 14,
        "org_oscWave": SAW, "org_subLevel": 0.4,
        "org_filterCutoff": 1800.0, "org_filterRes": 0.2,
        "org_ampAtk": 1.5, "org_ampRel": 3.0,
        "org_lfo1Rate": 0.03, "org_lfo1Depth": 0.3,
        "org_lfo2Rate": 0.05, "org_lfo2Depth": 0.2,
        "org_reverbMix": 0.55,
    }, (0.25, 0.65, 0.35, 0.25, 0.6, 0.0),
     "Ultra-slow attack, heavy reverb. The colony in fog.",
     ["ambient", "mist", "slow", "reverb"]),

    ("Depth Signal", {
        "org_rule": 150.0, "org_stepRate": 3.0, "org_scope": 6,
        "org_oscWave": SQUARE, "org_subLevel": 0.5,
        "org_filterCutoff": 1500.0, "org_filterRes": 0.3,
        "org_ampAtk": 0.5, "org_ampRel": 1.5,
        "org_lfo1Rate": 0.1, "org_lfo1Depth": 0.2,
        "org_lfo2Rate": 0.08, "org_lfo2Depth": 0.3,
        "org_reverbMix": 0.35,
    }, (0.3, 0.6, 0.35, 0.3, 0.4, 0.1),
     "Square wave + heavy sub. Deep sonar signals from the colony.",
     ["ambient", "depth", "sonar", "dark"]),

    ("Bioluminescence", {
        "org_rule": 90.0, "org_stepRate": 6.0, "org_scope": 4,
        "org_oscWave": TRI, "org_subLevel": 0.15,
        "org_filterCutoff": 5000.0, "org_filterRes": 0.2,
        "org_ampAtk": 0.3, "org_ampRel": 1.5,
        "org_lfo1Rate": 0.2, "org_lfo1Depth": 0.3,
        "org_lfo2Rate": 0.15, "org_lfo2Depth": 0.2,
        "org_reverbMix": 0.35,
    }, (0.55, 0.45, 0.5, 0.25, 0.4, 0.05),
     "Bright, twinkling. Fractal Rule 90 makes the light flicker.",
     ["ambient", "bioluminescent", "bright", "twinkling"]),

    ("Night Current", {
        "org_rule": 30.0, "org_stepRate": 1.0, "org_scope": 16,
        "org_oscWave": SAW, "org_subLevel": 0.5,
        "org_filterCutoff": 1200.0, "org_filterRes": 0.2,
        "org_ampAtk": 1.0, "org_ampSus": 0.85, "org_ampRel": 3.0,
        "org_lfo1Rate": 0.02, "org_lfo1Depth": 0.3,
        "org_lfo2Rate": 0.04, "org_lfo2Depth": 0.25,
        "org_reverbMix": 0.4,
    }, (0.2, 0.7, 0.3, 0.25, 0.45, 0.05),
     "Slow chaotic rule, maximum smoothing. Dark water flowing.",
     ["ambient", "night", "dark", "current"]),

    ("Polyp Choir", {
        "org_rule": 110.0, "org_stepRate": 3.0, "org_scope": 8,
        "org_mutate": 0.05,
        "org_oscWave": SAW, "org_subLevel": 0.3,
        "org_filterCutoff": 2500.0, "org_filterRes": 0.35,
        "org_ampAtk": 0.6, "org_ampRel": 2.0,
        "org_lfo1Rate": 0.1, "org_lfo1Depth": 0.25,
        "org_lfo2Rate": 0.07, "org_lfo2Depth": 0.3,
        "org_reverbMix": 0.4,
    }, (0.4, 0.6, 0.45, 0.25, 0.45, 0.05),
     "Gentle mutation makes each generation slightly different. A choir of polyps.",
     ["ambient", "choir", "mutation", "organic"]),

    ("Limestone Cave", {
        "org_rule": 22.0, "org_stepRate": 0.5, "org_scope": 16,
        "org_oscWave": TRI, "org_subLevel": 0.4,
        "org_filterCutoff": 2000.0, "org_filterRes": 0.2,
        "org_ampAtk": 2.0, "org_ampSus": 0.8, "org_ampRel": 4.0,
        "org_lfo1Rate": 0.02, "org_lfo1Depth": 0.2,
        "org_lfo2Rate": 0.01, "org_lfo2Depth": 0.3,
        "org_reverbMix": 0.6,
    }, (0.3, 0.55, 0.2, 0.2, 0.7, 0.0),
     "Slowest step rate. Maximum scope. Heavy reverb. Geological time.",
     ["ambient", "cave", "geological", "slow"]),

    ("Warm Shallows", {
        "org_rule": 184.0, "org_stepRate": 4.0, "org_scope": 6,
        "org_oscWave": SAW, "org_subLevel": 0.35,
        "org_filterCutoff": 3500.0, "org_filterRes": 0.25,
        "org_ampAtk": 0.4, "org_ampRel": 1.5,
        "org_lfo1Rate": 0.15, "org_lfo1Depth": 0.25,
        "org_lfo2Rate": 0.1, "org_lfo2Depth": 0.2,
        "org_reverbMix": 0.3,
    }, (0.45, 0.6, 0.4, 0.25, 0.3, 0.05),
     "Ordered rule, medium scope. Warm and rhythmic like shallow water.",
     ["ambient", "warm", "shallows", "rhythmic"]),

    ("Colony Drift", {
        "org_rule": 110.0, "org_stepRate": 2.0, "org_scope": 10,
        "org_mutate": 0.1,
        "org_oscWave": SAW, "org_subLevel": 0.35,
        "org_filterCutoff": 2000.0, "org_filterRes": 0.25,
        "org_ampAtk": 1.0, "org_ampSus": 0.8, "org_ampRel": 2.5,
        "org_lfo1Rate": 0.04, "org_lfo1Depth": 0.3,
        "org_lfo2Rate": 0.06, "org_lfo2Depth": 0.25,
        "org_reverbMix": 0.4,
    }, (0.35, 0.6, 0.4, 0.25, 0.45, 0.05),
     "Slow mutation drift. The colony changes over minutes.",
     ["ambient", "drift", "mutation", "slow"]),

    ("Glass Reef", {
        "org_rule": 90.0, "org_stepRate": 4.0, "org_scope": 6,
        "org_oscWave": TRI, "org_subLevel": 0.1,
        "org_filterCutoff": 5500.0, "org_filterRes": 0.15,
        "org_ampAtk": 0.3, "org_ampDec": 0.8, "org_ampSus": 0.4,
        "org_ampRel": 2.0,
        "org_lfo1Rate": 0.15, "org_lfo1Depth": 0.2,
        "org_lfo2Rate": 0.1, "org_lfo2Depth": 0.15,
        "org_reverbMix": 0.4,
    }, (0.55, 0.45, 0.4, 0.2, 0.45, 0.0),
     "Bright triangle, fractal rule. Crystalline and transparent.",
     ["ambient", "glass", "crystal", "bright"]),

    ("Abyssal Hum", {
        "org_rule": 18.0, "org_stepRate": 0.5, "org_scope": 16,
        "org_oscWave": SAW, "org_subLevel": 0.7,
        "org_filterCutoff": 800.0, "org_filterRes": 0.2,
        "org_ampAtk": 1.5, "org_ampSus": 0.9, "org_ampRel": 3.0,
        "org_lfo1Rate": 0.01, "org_lfo1Depth": 0.25,
        "org_lfo2Rate": 0.02, "org_lfo2Depth": 0.2,
        "org_reverbMix": 0.35,
    }, (0.15, 0.7, 0.2, 0.25, 0.4, 0.05),
     "Sparse rule, maximum smoothing, heavy sub. The deep ocean hum.",
     ["ambient", "abyssal", "dark", "sub"]),

    ("Sunrise Cells", {
        "org_rule": 110.0, "org_stepRate": 6.0, "org_scope": 4,
        "org_oscWave": TRI, "org_subLevel": 0.2,
        "org_filterCutoff": 4500.0, "org_filterRes": 0.2,
        "org_ampAtk": 0.5, "org_ampRel": 1.5,
        "org_lfo1Rate": 0.12, "org_lfo1Depth": 0.25,
        "org_lfo2Rate": 0.08, "org_lfo2Depth": 0.2,
        "org_reverbMix": 0.3,
    }, (0.5, 0.5, 0.45, 0.25, 0.35, 0.0),
     "Bright, active colony. Light returning to the reef.",
     ["ambient", "sunrise", "bright", "hopeful"]),

    ("Kelp Forest", {
        "org_rule": 54.0, "org_stepRate": 3.0, "org_scope": 8,
        "org_mutate": 0.08,
        "org_oscWave": SAW, "org_subLevel": 0.4,
        "org_filterCutoff": 2000.0, "org_filterRes": 0.3,
        "org_ampAtk": 0.6, "org_ampRel": 2.0,
        "org_lfo1Rate": 0.06, "org_lfo1Depth": 0.3,
        "org_lfo2Rate": 0.04, "org_lfo2Depth": 0.25,
        "org_reverbMix": 0.4,
    }, (0.35, 0.6, 0.4, 0.3, 0.45, 0.05),
     "Complex nested rule with gentle mutation. Tall, swaying, alive.",
     ["ambient", "kelp", "forest", "organic"]),

    ("Storm Approach", {
        "org_rule": 30.0, "org_stepRate": 8.0, "org_scope": 3,
        "org_mutate": 0.15,
        "org_oscWave": SAW, "org_subLevel": 0.5,
        "org_filterCutoff": 2000.0, "org_filterRes": 0.4,
        "org_ampAtk": 0.3, "org_ampRel": 1.0,
        "org_lfo1Rate": 0.2, "org_lfo1Depth": 0.35,
        "org_lfo2Rate": 0.15, "org_lfo2Depth": 0.3,
        "org_reverbMix": 0.3,
    }, (0.4, 0.5, 0.6, 0.35, 0.35, 0.2),
     "Chaotic rule, high mutation, fast steps. The water churns.",
     ["ambient", "storm", "chaotic", "tense"]),

    ("Ancient Colony", {
        "org_rule": 110.0, "org_seed": 1, "org_stepRate": 0.5, "org_scope": 16,
        "org_oscWave": SAW, "org_subLevel": 0.4,
        "org_filterCutoff": 1500.0, "org_filterRes": 0.2,
        "org_ampAtk": 2.0, "org_ampSus": 0.85, "org_ampRel": 4.0,
        "org_lfo1Rate": 0.01, "org_lfo1Depth": 0.2,
        "org_lfo2Rate": 0.02, "org_lfo2Depth": 0.25,
        "org_reverbMix": 0.5,
    }, (0.25, 0.65, 0.25, 0.2, 0.55, 0.0),
     "Slowest rate, maximum scope, seed 1. A colony that has been evolving for millennia.",
     ["ambient", "ancient", "slow", "meditative"]),

    ("Reef Whisper", {
        "org_rule": 150.0, "org_stepRate": 2.0, "org_scope": 10,
        "org_oscWave": TRI, "org_subLevel": 0.2,
        "org_filterCutoff": 3500.0, "org_filterRes": 0.15,
        "org_ampAtk": 0.4, "org_ampDec": 1.0, "org_ampSus": 0.3,
        "org_ampRel": 2.0,
        "org_lfo1Rate": 0.08, "org_lfo1Depth": 0.2,
        "org_lfo2Rate": 0.05, "org_lfo2Depth": 0.15,
        "org_reverbMix": 0.35,
    }, (0.45, 0.55, 0.3, 0.2, 0.4, 0.0),
     "Quiet triangle, additive rule. A whisper from the reef.",
     ["ambient", "whisper", "quiet", "gentle"]),

    ("Calcified Memory", {
        "org_rule": 110.0, "org_freeze": 1, "org_seed": 8080,
        "org_oscWave": SAW, "org_subLevel": 0.3,
        "org_filterCutoff": 2200.0, "org_filterRes": 0.25,
        "org_ampAtk": 1.0, "org_ampRel": 2.5,
        "org_lfo1Rate": 0.06, "org_lfo1Depth": 0.25,
        "org_lfo2Rate": 0.04, "org_lfo2Depth": 0.3,
        "org_reverbMix": 0.45,
    }, (0.35, 0.55, 0.3, 0.2, 0.5, 0.0),
     "Frozen state from seed 8080. The colony is calcified — a fossil.",
     ["ambient", "frozen", "fossil", "static"]),

    ("Surface Shimmer", {
        "org_rule": 90.0, "org_stepRate": 10.0, "org_scope": 3,
        "org_oscWave": TRI, "org_subLevel": 0.1,
        "org_filterCutoff": 6000.0, "org_filterRes": 0.2,
        "org_ampAtk": 0.2, "org_ampDec": 0.6, "org_ampSus": 0.35,
        "org_ampRel": 1.5,
        "org_lfo1Rate": 0.25, "org_lfo1Depth": 0.2,
        "org_lfo2Rate": 0.2, "org_lfo2Depth": 0.15,
        "org_reverbMix": 0.35,
    }, (0.6, 0.4, 0.5, 0.2, 0.4, 0.05),
     "Fast fractal rule, bright filter, light triangle. Sunlight on water.",
     ["ambient", "shimmer", "surface", "bright"]),

    ("Substrate Pulse", {
        "org_rule": 184.0, "org_stepRate": 4.0, "org_scope": 6,
        "org_oscWave": SQUARE, "org_subLevel": 0.4,
        "org_filterCutoff": 1800.0, "org_filterRes": 0.35,
        "org_ampAtk": 0.3, "org_ampRel": 1.0,
        "org_lfo1Rate": 0.15, "org_lfo1Depth": 0.2,
        "org_lfo2Rate": 0.1, "org_lfo2Depth": 0.25,
        "org_reverbMix": 0.25,
    }, (0.35, 0.55, 0.35, 0.3, 0.25, 0.15),
     "Ordered square-wave pulse. The substrate that the colony grows on.",
     ["ambient", "pulse", "substrate", "ordered"]),

    ("Midnight Bloom", {
        "org_rule": 110.0, "org_stepRate": 1.5, "org_scope": 14,
        "org_mutate": 0.03,
        "org_oscWave": SAW, "org_subLevel": 0.45,
        "org_filterCutoff": 1800.0, "org_filterRes": 0.2,
        "org_ampAtk": 1.5, "org_ampSus": 0.85, "org_ampRel": 3.5,
        "org_lfo1Rate": 0.03, "org_lfo1Depth": 0.25,
        "org_lfo2Rate": 0.02, "org_lfo2Depth": 0.3,
        "org_reverbMix": 0.5,
    }, (0.25, 0.65, 0.3, 0.25, 0.55, 0.0),
     "Dark, slow, reverent. The colony blooms after sunset.",
     ["ambient", "midnight", "bloom", "dark"]),

    ("Thermocline", {
        "org_rule": 184.0, "org_stepRate": 2.0, "org_scope": 10,
        "org_oscWave": SAW, "org_subLevel": 0.4,
        "org_filterCutoff": 2200.0, "org_filterRes": 0.25,
        "org_ampAtk": 0.6, "org_ampSus": 0.75, "org_ampRel": 2.0,
        "org_lfo1Rate": 0.08, "org_lfo1Depth": 0.3,
        "org_lfo2Rate": 0.05, "org_lfo2Depth": 0.25,
        "org_reverbMix": 0.35,
    }, (0.35, 0.6, 0.4, 0.25, 0.4, 0.05),
     "Where warm and cold water meet. Ordered rule creates layered movement.",
     ["ambient", "thermocline", "layered", "water"]),

    ("Spore Cloud", {
        "org_rule": 90.0, "org_stepRate": 5.0, "org_scope": 5,
        "org_mutate": 0.12,
        "org_oscWave": TRI, "org_subLevel": 0.2,
        "org_filterCutoff": 3500.0, "org_filterRes": 0.2,
        "org_ampAtk": 0.4, "org_ampDec": 0.8, "org_ampSus": 0.3,
        "org_ampRel": 1.5,
        "org_lfo1Rate": 0.15, "org_lfo1Depth": 0.25,
        "org_lfo2Rate": 0.1, "org_lfo2Depth": 0.2,
        "org_reverbMix": 0.4,
    }, (0.45, 0.5, 0.5, 0.3, 0.45, 0.05),
     "Fractal rule with mutation. Each note releases a cloud of timbral spores.",
     ["ambient", "spore", "cloud", "fractal"]),

    ("Lagoon Calm", {
        "org_rule": 22.0, "org_stepRate": 1.0, "org_scope": 12,
        "org_oscWave": TRI, "org_subLevel": 0.35,
        "org_filterCutoff": 2800.0, "org_filterRes": 0.15,
        "org_ampAtk": 1.0, "org_ampSus": 0.8, "org_ampRel": 2.5,
        "org_lfo1Rate": 0.04, "org_lfo1Depth": 0.2,
        "org_lfo2Rate": 0.03, "org_lfo2Depth": 0.2,
        "org_reverbMix": 0.4,
    }, (0.4, 0.6, 0.25, 0.2, 0.45, 0.0),
     "Nested rule, gentle triangle. A protected lagoon, barely stirring.",
     ["ambient", "lagoon", "calm", "gentle"]),

    ("Plankton Drift", {
        "org_rule": 150.0, "org_stepRate": 3.0, "org_scope": 8,
        "org_mutate": 0.06,
        "org_oscWave": SAW, "org_subLevel": 0.3,
        "org_filterCutoff": 2500.0, "org_filterRes": 0.25,
        "org_ampAtk": 0.5, "org_ampRel": 1.5,
        "org_lfo1Rate": 0.06, "org_lfo1Depth": 0.25,
        "org_lfo2Rate": 0.04, "org_lfo2Depth": 0.2,
        "org_reverbMix": 0.35,
    }, (0.4, 0.55, 0.4, 0.25, 0.4, 0.05),
     "Additive rule with gentle mutation. Millions of tiny lives drifting.",
     ["ambient", "plankton", "drift", "organic"]),
]

# ══════════════════════════════════════════════════════════════════════════════
# PRISM (25): genre production
# ══════════════════════════════════════════════════════════════════════════════

PRISM = [
    # --- Ambient Techno (8) ---
    ("Colony Pulse", {
        "org_rule": 30.0, "org_stepRate": 8.0, "org_scope": 2,
        "org_oscWave": SAW, "org_subLevel": 0.4,
        "org_filterCutoff": 2000.0, "org_filterRes": 0.5,
        "org_ampAtk": 0.001, "org_ampDec": 0.3, "org_ampSus": 0.4,
        "org_lfo1Depth": 0.15, "org_lfo2Depth": 0.2,
    }, (0.4, 0.5, 0.5, 0.3, 0.0, 0.3),
     "Fast chaotic colony over resonant filter. Techno sequence material.",
     ["techno", "sequence", "chaotic", "resonant"]),

    ("Automaton Bass", {
        "org_rule": 110.0, "org_stepRate": 4.0, "org_scope": 4,
        "org_oscWave": SAW, "org_subLevel": 0.7,
        "org_filterCutoff": 1000.0, "org_filterRes": 0.4,
        "org_ampAtk": 0.001, "org_ampDec": 0.4, "org_ampSus": 0.5,
        "org_velCutoff": 0.7,
        "org_lfo1Depth": 0.0, "org_lfo2Depth": 0.1,
    }, (0.3, 0.6, 0.3, 0.3, 0.0, 0.3),
     "Deep evolving bass. The colony shapes the filter while you play.",
     ["techno", "bass", "evolving", "deep"]),

    ("Stab Morph", {
        "org_rule": 90.0, "org_stepRate": 12.0, "org_scope": 2,
        "org_oscWave": SAW, "org_subLevel": 0.2,
        "org_filterCutoff": 4000.0, "org_filterRes": 0.4,
        "org_ampAtk": 0.001, "org_ampDec": 0.15, "org_ampSus": 0.0,
        "org_ampRel": 0.2,
        "org_velCutoff": 0.8,
        "org_lfo1Depth": 0.0, "org_lfo2Depth": 0.0,
    }, (0.55, 0.4, 0.3, 0.3, 0.0, 0.35),
     "Short bright stabs. Each stab is a different CA generation.",
     ["techno", "stab", "short", "morphing"]),

    ("Dark Colony", {
        "org_rule": 110.0, "org_stepRate": 2.0, "org_scope": 8,
        "org_mutate": 0.1,
        "org_oscWave": SAW, "org_subLevel": 0.5,
        "org_filterCutoff": 1500.0, "org_filterRes": 0.3,
        "org_ampAtk": 0.5, "org_ampSus": 0.8, "org_ampRel": 1.5,
        "org_lfo1Rate": 0.08, "org_lfo1Depth": 0.25,
        "org_lfo2Rate": 0.06, "org_lfo2Depth": 0.2,
        "org_reverbMix": 0.25,
    }, (0.3, 0.6, 0.4, 0.3, 0.25, 0.15),
     "Slow, dark, mutating. Background texture for ambient techno.",
     ["techno", "ambient-techno", "dark", "pad"]),

    ("Generative Kick", {
        "org_rule": 30.0, "org_stepRate": 16.0, "org_scope": 1,
        "org_oscWave": SAW, "org_subLevel": 0.6,
        "org_filterCutoff": 3000.0, "org_filterRes": 0.5,
        "org_ampAtk": 0.001, "org_ampDec": 0.25, "org_ampSus": 0.0,
        "org_ampRel": 0.15,
        "org_velCutoff": 0.9,
        "org_lfo1Depth": 0.0, "org_lfo2Depth": 0.0,
    }, (0.4, 0.5, 0.2, 0.3, 0.0, 0.45),
     "Each hit is a different state. Layer under your kick.",
     ["techno", "kick-layer", "percussive", "generative"]),

    ("Hypnotic Evolve", {
        "org_rule": 150.0, "org_stepRate": 6.0, "org_scope": 4,
        "org_oscWave": SQUARE, "org_subLevel": 0.3,
        "org_filterCutoff": 2500.0, "org_filterRes": 0.45,
        "org_ampAtk": 0.001, "org_ampDec": 0.25, "org_ampSus": 0.3,
        "org_lfo1Depth": 0.15, "org_lfo2Depth": 0.2,
        "org_reverbMix": 0.15,
    }, (0.4, 0.5, 0.45, 0.3, 0.15, 0.2),
     "Square wave under additive rule. Hypnotic evolving sequence.",
     ["techno", "hypnotic", "sequence", "evolving"]),

    ("Acid Colony", {
        "org_rule": 30.0, "org_stepRate": 8.0, "org_scope": 2,
        "org_oscWave": SAW, "org_subLevel": 0.3,
        "org_filterCutoff": 800.0, "org_filterRes": 0.75,
        "org_velCutoff": 0.9,
        "org_ampAtk": 0.001, "org_ampDec": 0.3, "org_ampSus": 0.3,
        "org_lfo1Depth": 0.0, "org_lfo2Depth": 0.0,
    }, (0.45, 0.45, 0.5, 0.3, 0.0, 0.45),
     "High resonance, chaotic rule. The colony does the squelch for you.",
     ["techno", "acid", "resonant", "303"]),

    ("Warehouse Haze", {
        "org_rule": 110.0, "org_stepRate": 3.0, "org_scope": 10,
        "org_mutate": 0.05,
        "org_oscWave": SAW, "org_subLevel": 0.4,
        "org_filterCutoff": 1800.0, "org_filterRes": 0.25,
        "org_ampAtk": 0.8, "org_ampSus": 0.7, "org_ampRel": 2.0,
        "org_lfo1Rate": 0.06, "org_lfo1Depth": 0.25,
        "org_lfo2Rate": 0.04, "org_lfo2Depth": 0.2,
        "org_reverbMix": 0.35,
    }, (0.3, 0.6, 0.35, 0.3, 0.35, 0.1),
     "Warm pad with slow evolution. The warehouse fills with haze.",
     ["techno", "pad", "warehouse", "ambient-techno"]),

    # --- IDM / Glitch (8) ---
    ("Glitch Colony", {
        "org_rule": 30.0, "org_stepRate": 32.0, "org_scope": 1,
        "org_mutate": 0.25,
        "org_oscWave": SAW, "org_subLevel": 0.2,
        "org_filterCutoff": 5000.0, "org_filterRes": 0.4,
        "org_ampAtk": 0.001, "org_ampDec": 0.1, "org_ampSus": 0.0,
        "org_ampRel": 0.1,
        "org_velCutoff": 0.8,
        "org_lfo1Depth": 0.0, "org_lfo2Depth": 0.0,
    }, (0.6, 0.35, 0.7, 0.35, 0.0, 0.4),
     "Maximum speed, minimum scope, high mutation. Pure generative glitch.",
     ["idm", "glitch", "fast", "chaotic"]),

    ("Pattern Stutter", {
        "org_rule": 90.0, "org_stepRate": 16.0, "org_scope": 2,
        "org_oscWave": SQUARE, "org_subLevel": 0.2,
        "org_filterCutoff": 3500.0, "org_filterRes": 0.5,
        "org_ampAtk": 0.001, "org_ampDec": 0.08, "org_ampSus": 0.0,
        "org_ampRel": 0.08,
        "org_lfo1Depth": 0.0, "org_lfo2Depth": 0.0,
    }, (0.5, 0.4, 0.5, 0.3, 0.0, 0.3),
     "Fast fractal rule, ultra-short envelope. Stuttering pattern generator.",
     ["idm", "stutter", "percussive", "fractal"]),

    ("Bit Mangler", {
        "org_rule": 150.0, "org_stepRate": 24.0, "org_scope": 1,
        "org_mutate": 0.4,
        "org_oscWave": SQUARE, "org_subLevel": 0.3,
        "org_filterCutoff": 4000.0, "org_filterRes": 0.5,
        "org_ampAtk": 0.001, "org_ampDec": 0.15, "org_ampSus": 0.2,
        "org_lfo1Depth": 0.1, "org_lfo2Depth": 0.15,
    }, (0.5, 0.4, 0.65, 0.35, 0.0, 0.4),
     "Additive rule at near-audio rate. High mutation. Bit-mangled texture.",
     ["idm", "bitmangler", "texture", "harsh"]),

    ("Emergent Melody", {
        "org_rule": 110.0, "org_stepRate": 6.0, "org_scope": 4,
        "org_oscWave": TRI, "org_subLevel": 0.2,
        "org_filterCutoff": 4000.0, "org_filterRes": 0.25,
        "org_ampAtk": 0.01, "org_ampDec": 0.5, "org_ampSus": 0.3,
        "org_ampRel": 0.5,
        "org_lfo1Depth": 0.1, "org_lfo2Depth": 0.15,
        "org_reverbMix": 0.2,
    }, (0.5, 0.5, 0.45, 0.25, 0.2, 0.1),
     "The pitch offset from cells 8-11 creates emergent melodic fragments.",
     ["idm", "melodic", "emergent", "generative"]),

    ("Noise Colony", {
        "org_rule": 30.0, "org_stepRate": 20.0, "org_scope": 1,
        "org_mutate": 0.5,
        "org_oscWave": SAW, "org_subLevel": 0.1,
        "org_filterCutoff": 6000.0, "org_filterRes": 0.3,
        "org_ampAtk": 0.001, "org_ampDec": 0.05, "org_ampSus": 0.0,
        "org_ampRel": 0.05,
        "org_lfo1Depth": 0.0, "org_lfo2Depth": 0.0,
    }, (0.6, 0.3, 0.8, 0.4, 0.0, 0.5),
     "50% mutation at 20 Hz. Nearly random. Controlled noise generator.",
     ["idm", "noise", "extreme", "random"]),

    ("Cell Division", {
        "org_rule": 110.0, "org_stepRate": 8.0, "org_scope": 3,
        "org_mutate": 0.1,
        "org_oscWave": SAW, "org_subLevel": 0.25,
        "org_filterCutoff": 3000.0, "org_filterRes": 0.4,
        "org_ampAtk": 0.001, "org_ampDec": 0.2, "org_ampSus": 0.15,
        "org_ampRel": 0.3,
        "org_velCutoff": 0.7,
        "org_lfo1Depth": 0.1, "org_lfo2Depth": 0.15,
        "org_reverbMix": 0.15,
    }, (0.5, 0.45, 0.55, 0.3, 0.15, 0.25),
     "Percussive notes that divide and mutate. Each hit is a new generation.",
     ["idm", "percussive", "division", "evolving"]),

    ("Algorithmic Pad", {
        "org_rule": 54.0, "org_stepRate": 2.0, "org_scope": 12,
        "org_mutate": 0.03,
        "org_oscWave": TRI, "org_subLevel": 0.35,
        "org_filterCutoff": 2500.0, "org_filterRes": 0.2,
        "org_ampAtk": 1.0, "org_ampSus": 0.8, "org_ampRel": 2.0,
        "org_lfo1Rate": 0.05, "org_lfo1Depth": 0.25,
        "org_lfo2Rate": 0.03, "org_lfo2Depth": 0.2,
        "org_reverbMix": 0.35,
    }, (0.4, 0.55, 0.35, 0.25, 0.35, 0.05),
     "Gentle nested rule. A pad that follows its own algorithmic logic.",
     ["idm", "pad", "algorithmic", "gentle"]),

    ("Micro Colony", {
        "org_rule": 90.0, "org_stepRate": 24.0, "org_scope": 2,
        "org_oscWave": SAW, "org_subLevel": 0.15,
        "org_filterCutoff": 5000.0, "org_filterRes": 0.35,
        "org_ampAtk": 0.001, "org_ampDec": 0.06, "org_ampSus": 0.0,
        "org_ampRel": 0.06,
        "org_lfo1Depth": 0.0, "org_lfo2Depth": 0.0,
        "org_reverbMix": 0.1,
    }, (0.55, 0.4, 0.6, 0.3, 0.1, 0.3),
     "Ultra-short envelope, near-audio-rate CA. Micro-scale generative hits.",
     ["idm", "micro", "granular", "fast"]),

    # --- Experimental Pop / Electronic (9) ---
    ("Living Bass", {
        "org_rule": 110.0, "org_stepRate": 4.0, "org_scope": 6,
        "org_oscWave": SAW, "org_subLevel": 0.6,
        "org_filterCutoff": 1500.0, "org_filterRes": 0.35,
        "org_ampAtk": 0.01, "org_ampSus": 0.7,
        "org_velCutoff": 0.6,
        "org_lfo1Depth": 0.1, "org_lfo2Depth": 0.15,
    }, (0.3, 0.6, 0.35, 0.3, 0.0, 0.2),
     "Bass that evolves while you hold it. The colony changes the timbre.",
     ["electronic", "bass", "living", "evolving"]),

    ("Pop Pluck", {
        "org_rule": 184.0, "org_stepRate": 6.0, "org_scope": 4,
        "org_oscWave": TRI, "org_subLevel": 0.2,
        "org_filterCutoff": 4500.0, "org_filterRes": 0.25,
        "org_ampAtk": 0.001, "org_ampDec": 0.4, "org_ampSus": 0.0,
        "org_ampRel": 0.5,
        "org_velCutoff": 0.7,
        "org_lfo1Depth": 0.1, "org_lfo2Depth": 0.1,
        "org_reverbMix": 0.2,
    }, (0.5, 0.5, 0.3, 0.2, 0.2, 0.1),
     "Clean pluck with ordered CA. Each note has subtle movement.",
     ["electronic", "pluck", "clean", "pop"]),

    ("Breathing Lead", {
        "org_rule": 110.0, "org_stepRate": 4.0, "org_scope": 6,
        "org_mutate": 0.05,
        "org_oscWave": SAW, "org_subLevel": 0.2,
        "org_filterCutoff": 3500.0, "org_filterRes": 0.3,
        "org_ampAtk": 0.01, "org_ampSus": 0.7,
        "org_velCutoff": 0.7,
        "org_lfo1Rate": 0.2, "org_lfo1Depth": 0.2,
        "org_lfo2Rate": 0.15, "org_lfo2Depth": 0.15,
        "org_reverbMix": 0.15,
    }, (0.5, 0.5, 0.45, 0.25, 0.15, 0.15),
     "Melodic lead that breathes. LFOs + colony = double movement.",
     ["electronic", "lead", "breathing", "melodic"]),

    ("Chord Evolve", {
        "org_rule": 110.0, "org_stepRate": 2.0, "org_scope": 10,
        "org_oscWave": SAW, "org_subLevel": 0.3,
        "org_filterCutoff": 2500.0, "org_filterRes": 0.25,
        "org_ampAtk": 0.3, "org_ampRel": 1.5,
        "org_lfo1Rate": 0.08, "org_lfo1Depth": 0.2,
        "org_lfo2Rate": 0.06, "org_lfo2Depth": 0.25,
        "org_reverbMix": 0.25,
    }, (0.4, 0.55, 0.35, 0.25, 0.25, 0.05),
     "Hold a chord and listen to it evolve. Slow colony shapes the sound.",
     ["electronic", "chord", "pad", "evolving"]),

    ("Texture Bed", {
        "org_rule": 54.0, "org_stepRate": 3.0, "org_scope": 12,
        "org_mutate": 0.08,
        "org_oscWave": SAW, "org_subLevel": 0.4,
        "org_filterCutoff": 2000.0, "org_filterRes": 0.2,
        "org_ampAtk": 1.5, "org_ampSus": 0.8, "org_ampRel": 2.5,
        "org_lfo1Rate": 0.04, "org_lfo1Depth": 0.3,
        "org_lfo2Rate": 0.03, "org_lfo2Depth": 0.2,
        "org_reverbMix": 0.4,
    }, (0.3, 0.6, 0.35, 0.25, 0.4, 0.0),
     "Background texture for production. The colony fills the space.",
     ["electronic", "texture", "background", "ambient"]),

    ("Synth Pop Keys", {
        "org_rule": 184.0, "org_stepRate": 4.0, "org_scope": 6,
        "org_oscWave": SQUARE, "org_subLevel": 0.2,
        "org_filterCutoff": 3500.0, "org_filterRes": 0.3,
        "org_ampAtk": 0.005, "org_ampDec": 0.6, "org_ampSus": 0.35,
        "org_ampRel": 0.8,
        "org_velCutoff": 0.6,
        "org_lfo1Depth": 0.1, "org_lfo2Depth": 0.15,
        "org_reverbMix": 0.15,
    }, (0.5, 0.5, 0.3, 0.25, 0.15, 0.1),
     "Square-wave keys with ordered colony. Each note has gentle life.",
     ["electronic", "keys", "pop", "square"]),

    ("Digital Warm", {
        "org_rule": 110.0, "org_stepRate": 3.0, "org_scope": 8,
        "org_oscWave": SAW, "org_subLevel": 0.35,
        "org_filterCutoff": 2500.0, "org_filterRes": 0.25,
        "org_ampAtk": 0.3, "org_ampRel": 1.0,
        "org_lfo1Rate": 0.12, "org_lfo1Depth": 0.2,
        "org_lfo2Rate": 0.08, "org_lfo2Depth": 0.2,
        "org_reverbMix": 0.25,
    }, (0.4, 0.6, 0.4, 0.25, 0.25, 0.05),
     "Warm filtered saw with smooth colony evolution. Usable in any mix.",
     ["electronic", "warm", "versatile", "mix-ready"]),

    ("Arpeggio Cells", {
        "org_rule": 90.0, "org_stepRate": 8.0, "org_scope": 3,
        "org_oscWave": SAW, "org_subLevel": 0.2,
        "org_filterCutoff": 4000.0, "org_filterRes": 0.35,
        "org_ampAtk": 0.001, "org_ampDec": 0.2, "org_ampSus": 0.0,
        "org_ampRel": 0.3,
        "org_velCutoff": 0.6,
        "org_lfo1Depth": 0.0, "org_lfo2Depth": 0.0,
        "org_reverbMix": 0.15,
    }, (0.55, 0.45, 0.35, 0.25, 0.15, 0.15),
     "Pluck designed for arpeggios. Fractal rule adds movement to repeats.",
     ["electronic", "arp", "pluck", "fractal"]),

    ("Colony Chord Stab", {
        "org_rule": 110.0, "org_stepRate": 4.0, "org_scope": 4,
        "org_oscWave": SAW, "org_subLevel": 0.3,
        "org_filterCutoff": 3500.0, "org_filterRes": 0.3,
        "org_ampAtk": 0.001, "org_ampDec": 0.2, "org_ampSus": 0.0,
        "org_ampRel": 0.3,
        "org_velCutoff": 0.8,
        "org_lfo1Depth": 0.0, "org_lfo2Depth": 0.0,
        "org_reverbMix": 0.1,
    }, (0.5, 0.45, 0.25, 0.3, 0.1, 0.25),
     "Chord stab where each hit has the colony's current state baked in.",
     ["electronic", "stab", "chord", "dynamic"]),
]

# ══════════════════════════════════════════════════════════════════════════════
# FLUX (20): movement-forward presets
# ══════════════════════════════════════════════════════════════════════════════

FLUX = [
    ("Mutation Drift", {
        "org_rule": 110.0, "org_stepRate": 4.0, "org_scope": 6,
        "org_mutate": 0.2,
        "org_oscWave": SAW, "org_subLevel": 0.35,
        "org_filterCutoff": 2500.0, "org_filterRes": 0.35,
        "org_ampAtk": 0.3, "org_ampSus": 0.7, "org_ampRel": 1.5,
        "org_lfo1Rate": 0.1, "org_lfo1Depth": 0.3,
        "org_lfo2Rate": 0.08, "org_lfo2Depth": 0.25,
        "org_reverbMix": 0.25,
    }, (0.4, 0.55, 0.55, 0.3, 0.25, 0.15),
     "High mutation + LFOs = constant timbral movement.",
     ["flux", "mutation", "drift", "movement"]),

    ("Chaotic Sweep", {
        "org_rule": 30.0, "org_stepRate": 6.0, "org_scope": 3,
        "org_mutate": 0.15,
        "org_oscWave": SAW, "org_subLevel": 0.25,
        "org_filterCutoff": 3000.0, "org_filterRes": 0.5,
        "org_lfo1Rate": 0.3, "org_lfo1Depth": 0.4,
        "org_lfo2Rate": 0.2, "org_lfo2Depth": 0.3,
    }, (0.45, 0.45, 0.65, 0.3, 0.0, 0.25),
     "Chaotic rule + fast LFOs + mutation = wild filter sweeping.",
     ["flux", "sweep", "chaotic", "wild"]),

    ("Scope Walker", {
        "org_rule": 110.0, "org_stepRate": 4.0, "org_scope": 1,
        "org_oscWave": SAW, "org_subLevel": 0.3,
        "org_filterCutoff": 3000.0, "org_filterRes": 0.35,
        "org_ampAtk": 0.01, "org_ampSus": 0.6,
        "org_lfo1Rate": 0.15, "org_lfo1Depth": 0.25,
        "org_lfo2Rate": 0.1, "org_lfo2Depth": 0.3,
        "org_macroMutate": 0.3,
    }, (0.5, 0.5, 0.55, 0.25, 0.0, 0.15),
     "Scope at 1: raw CA output. Every generation is a timbral step.",
     ["flux", "raw", "scope-1", "stepping"]),

    ("Double LFO Morph", {
        "org_rule": 150.0, "org_stepRate": 3.0, "org_scope": 6,
        "org_oscWave": SAW, "org_subLevel": 0.35,
        "org_filterCutoff": 2500.0, "org_filterRes": 0.3,
        "org_lfo1Rate": 0.5, "org_lfo1Depth": 0.4,
        "org_lfo2Rate": 0.35, "org_lfo2Depth": 0.35,
        "org_reverbMix": 0.15,
    }, (0.4, 0.55, 0.6, 0.25, 0.15, 0.1),
     "Both LFOs at high depth. The colony adds a third modulation layer.",
     ["flux", "lfo", "double", "morphing"]),

    ("Entropy Rise", {
        "org_rule": 30.0, "org_stepRate": 4.0, "org_scope": 4,
        "org_mutate": 0.0, "org_macroMutate": 0.0,
        "org_oscWave": SAW, "org_subLevel": 0.3,
        "org_filterCutoff": 2000.0, "org_filterRes": 0.4,
        "org_ampAtk": 2.0, "org_ampSus": 0.8,
        "org_lfo1Rate": 0.06, "org_lfo1Depth": 0.3,
        "org_lfo2Rate": 0.04, "org_lfo2Depth": 0.25,
        "org_reverbMix": 0.2,
    }, (0.35, 0.55, 0.5, 0.25, 0.2, 0.15),
     "Start with zero mutation. Slowly raise MUTATE macro for entropy riser.",
     ["flux", "riser", "entropy", "build"]),

    ("Pulse Evolve", {
        "org_rule": 90.0, "org_stepRate": 6.0, "org_scope": 4,
        "org_mutate": 0.08,
        "org_oscWave": SQUARE, "org_subLevel": 0.3,
        "org_filterCutoff": 2500.0, "org_filterRes": 0.4,
        "org_ampAtk": 0.01, "org_ampSus": 0.6,
        "org_lfo1Rate": 0.2, "org_lfo1Depth": 0.25,
        "org_lfo2Rate": 0.15, "org_lfo2Depth": 0.2,
    }, (0.4, 0.5, 0.5, 0.3, 0.0, 0.15),
     "Square pulse under fractal rule. The timbre never sits still.",
     ["flux", "pulse", "fractal", "evolving"]),

    ("Velocity Dance", {
        "org_rule": 110.0, "org_stepRate": 8.0, "org_scope": 3,
        "org_oscWave": SAW, "org_subLevel": 0.2,
        "org_filterCutoff": 1500.0, "org_filterRes": 0.4,
        "org_velCutoff": 0.9,
        "org_ampAtk": 0.001, "org_ampDec": 0.3, "org_ampSus": 0.2,
        "org_lfo1Depth": 0.1, "org_lfo2Depth": 0.15,
    }, (0.45, 0.5, 0.45, 0.25, 0.0, 0.2),
     "Maximum velocity sensitivity. Play dynamics to dance with the colony.",
     ["flux", "velocity", "dynamic", "expressive"]),

    ("Resonant Walk", {
        "org_rule": 110.0, "org_stepRate": 4.0, "org_scope": 4,
        "org_oscWave": SAW, "org_subLevel": 0.3,
        "org_filterCutoff": 2000.0, "org_filterRes": 0.7,
        "org_lfo1Rate": 0.15, "org_lfo1Depth": 0.3,
        "org_lfo2Rate": 0.1, "org_lfo2Depth": 0.25,
        "org_reverbMix": 0.1,
    }, (0.4, 0.5, 0.55, 0.3, 0.1, 0.25),
     "High resonance + colony cutoff modulation = walking resonant peaks.",
     ["flux", "resonant", "walking", "filter"]),

    ("Speed Shift", {
        "org_rule": 110.0, "org_stepRate": 16.0, "org_scope": 2,
        "org_oscWave": SAW, "org_subLevel": 0.25,
        "org_filterCutoff": 3500.0, "org_filterRes": 0.35,
        "org_lfo1Rate": 0.4, "org_lfo1Depth": 0.3,
        "org_lfo2Rate": 0.3, "org_lfo2Depth": 0.25,
        "org_macroRule": 0.5,
    }, (0.5, 0.45, 0.6, 0.3, 0.0, 0.2),
     "Fast steps + fast LFOs. Three layers of movement at different rates.",
     ["flux", "fast", "layered", "speed"]),

    ("Organic Wobble", {
        "org_rule": 30.0, "org_stepRate": 4.0, "org_scope": 6,
        "org_mutate": 0.12,
        "org_oscWave": SAW, "org_subLevel": 0.4,
        "org_filterCutoff": 1500.0, "org_filterRes": 0.5,
        "org_lfo1Rate": 0.5, "org_lfo1Depth": 0.35,
        "org_lfo2Rate": 0.35, "org_lfo2Depth": 0.3,
    }, (0.35, 0.55, 0.6, 0.3, 0.0, 0.25),
     "LFO wobble combined with chaotic colony. Organic bass movement.",
     ["flux", "wobble", "bass", "organic"]),

    ("Aftertouch Chaos", {
        "org_rule": 110.0, "org_stepRate": 6.0, "org_scope": 4,
        "org_mutate": 0.0,
        "org_oscWave": SAW, "org_subLevel": 0.3,
        "org_filterCutoff": 3000.0, "org_filterRes": 0.35,
        "org_lfo1Rate": 0.2, "org_lfo1Depth": 0.2,
        "org_lfo2Rate": 0.15, "org_lfo2Depth": 0.2,
        "org_reverbMix": 0.15,
    }, (0.5, 0.5, 0.5, 0.25, 0.15, 0.15),
     "Zero base mutation. Press harder for chaos. Release for order.",
     ["flux", "aftertouch", "chaos-control", "expressive"]),

    ("Rule Morph", {
        "org_rule": 110.0, "org_stepRate": 4.0, "org_scope": 4,
        "org_macroRule": 0.0,
        "org_oscWave": SAW, "org_subLevel": 0.3,
        "org_filterCutoff": 2800.0, "org_filterRes": 0.3,
        "org_lfo1Depth": 0.15, "org_lfo2Depth": 0.2,
        "org_reverbMix": 0.15,
    }, (0.45, 0.5, 0.45, 0.25, 0.15, 0.1),
     "RULE macro at zero. Sweep it to morph through all 8 curated rules.",
     ["flux", "macro", "rule-morph", "sweep"]),

    ("Seed Roller", {
        "org_rule": 110.0, "org_stepRate": 4.0, "org_scope": 4,
        "org_macroSeed": 0.0,
        "org_oscWave": SAW, "org_subLevel": 0.3,
        "org_filterCutoff": 3000.0, "org_filterRes": 0.3,
        "org_lfo1Depth": 0.2, "org_lfo2Depth": 0.2,
        "org_reverbMix": 0.15,
    }, (0.45, 0.5, 0.5, 0.25, 0.15, 0.1),
     "Tap SEED macro to re-roll the colony. Each tap starts a new evolution.",
     ["flux", "seed", "reroll", "generative"]),

    ("Triple Modulation", {
        "org_rule": 110.0, "org_stepRate": 6.0, "org_scope": 4,
        "org_mutate": 0.1,
        "org_oscWave": SAW, "org_subLevel": 0.3,
        "org_filterCutoff": 2500.0, "org_filterRes": 0.4,
        "org_lfo1Rate": 0.8, "org_lfo1Depth": 0.3,
        "org_lfo2Rate": 0.5, "org_lfo2Depth": 0.3,
        "org_reverbMix": 0.15,
    }, (0.45, 0.5, 0.7, 0.3, 0.15, 0.2),
     "Colony + LFO1 + LFO2 all modulating the filter. Triple-layered movement.",
     ["flux", "triple", "modulation", "complex"]),

    ("Breathing Colony", {
        "org_rule": 184.0, "org_stepRate": 2.0, "org_scope": 8,
        "org_oscWave": SAW, "org_subLevel": 0.35,
        "org_filterCutoff": 2500.0, "org_filterRes": 0.25,
        "org_ampAtk": 0.5, "org_ampSus": 0.7, "org_ampRel": 1.5,
        "org_lfo1Rate": 0.05, "org_lfo1Depth": 0.35,
        "org_lfo2Rate": 0.03, "org_lfo2Depth": 0.3,
        "org_reverbMix": 0.25,
    }, (0.4, 0.55, 0.45, 0.25, 0.25, 0.05),
     "Slow LFOs create deep breathing. The colony adds subtle life beneath.",
     ["flux", "breathing", "slow", "deep"]),

    ("Step Shimmer", {
        "org_rule": 90.0, "org_stepRate": 12.0, "org_scope": 3,
        "org_oscWave": TRI, "org_subLevel": 0.15,
        "org_filterCutoff": 5000.0, "org_filterRes": 0.25,
        "org_ampAtk": 0.01, "org_ampDec": 0.5, "org_ampSus": 0.3,
        "org_lfo1Rate": 0.2, "org_lfo1Depth": 0.2,
        "org_lfo2Rate": 0.15, "org_lfo2Depth": 0.15,
        "org_reverbMix": 0.3,
    }, (0.55, 0.45, 0.5, 0.2, 0.3, 0.05),
     "Bright triangle with fast fractal steps. Shimmering movement.",
     ["flux", "shimmer", "bright", "fractal"]),

    ("Macro Dance", {
        "org_rule": 110.0, "org_stepRate": 4.0, "org_scope": 4,
        "org_macroRule": 0.25, "org_macroMutate": 0.2,
        "org_oscWave": SAW, "org_subLevel": 0.3,
        "org_filterCutoff": 3000.0, "org_filterRes": 0.35,
        "org_lfo1Rate": 0.25, "org_lfo1Depth": 0.25,
        "org_lfo2Rate": 0.2, "org_lfo2Depth": 0.2,
        "org_reverbMix": 0.15,
    }, (0.5, 0.5, 0.55, 0.25, 0.15, 0.15),
     "All macros set for live manipulation. Move them while playing.",
     ["flux", "macros", "performance", "live"]),

    ("Dark Motion", {
        "org_rule": 30.0, "org_stepRate": 4.0, "org_scope": 6,
        "org_mutate": 0.08,
        "org_oscWave": SAW, "org_subLevel": 0.5,
        "org_filterCutoff": 1200.0, "org_filterRes": 0.3,
        "org_lfo1Rate": 0.1, "org_lfo1Depth": 0.3,
        "org_lfo2Rate": 0.08, "org_lfo2Depth": 0.25,
        "org_reverbMix": 0.2,
    }, (0.25, 0.6, 0.45, 0.3, 0.2, 0.15),
     "Dark filtered colony with constant subtle movement.",
     ["flux", "dark", "motion", "subtle"]),

    ("Tidal Shift", {
        "org_rule": 184.0, "org_stepRate": 3.0, "org_scope": 8,
        "org_mutate": 0.05,
        "org_oscWave": SAW, "org_subLevel": 0.4,
        "org_filterCutoff": 2200.0, "org_filterRes": 0.3,
        "org_lfo1Rate": 0.06, "org_lfo1Depth": 0.35,
        "org_lfo2Rate": 0.04, "org_lfo2Depth": 0.3,
        "org_reverbMix": 0.3,
    }, (0.35, 0.6, 0.5, 0.25, 0.3, 0.05),
     "Traffic-flow rule creates tidal rhythm. LFOs add swell.",
     ["flux", "tidal", "swell", "rhythmic"]),

    ("Wobble Square", {
        "org_rule": 150.0, "org_stepRate": 8.0, "org_scope": 3,
        "org_oscWave": SQUARE, "org_subLevel": 0.35,
        "org_filterCutoff": 2000.0, "org_filterRes": 0.5,
        "org_lfo1Rate": 0.6, "org_lfo1Depth": 0.4,
        "org_lfo2Rate": 0.4, "org_lfo2Depth": 0.3,
    }, (0.4, 0.5, 0.65, 0.3, 0.0, 0.25),
     "Square wave wobble with additive rule adding harmonic complexity.",
     ["flux", "wobble", "square", "bass"]),
]

# ══════════════════════════════════════════════════════════════════════════════
# AETHER (15): deep generative / experimental
# ══════════════════════════════════════════════════════════════════════════════

AETHER = [
    ("Infinite Colony", {
        "org_rule": 110.0, "org_stepRate": 1.0, "org_scope": 16,
        "org_mutate": 0.03,
        "org_oscWave": SAW, "org_subLevel": 0.4,
        "org_filterCutoff": 2000.0, "org_filterRes": 0.2,
        "org_ampAtk": 2.0, "org_ampSus": 0.9, "org_ampRel": 5.0,
        "org_lfo1Rate": 0.01, "org_lfo1Depth": 0.3,
        "org_lfo2Rate": 0.02, "org_lfo2Depth": 0.25,
        "org_reverbMix": 0.5,
    }, (0.3, 0.6, 0.35, 0.25, 0.55, 0.0),
     "Slowest possible evolution. Hold one note and wait. It never repeats.",
     ["aether", "infinite", "generative", "meditative"]),

    ("Frozen Fossil", {
        "org_rule": 110.0, "org_seed": 31415, "org_freeze": 1,
        "org_oscWave": SAW, "org_subLevel": 0.35,
        "org_filterCutoff": 2500.0, "org_filterRes": 0.25,
        "org_ampAtk": 1.5, "org_ampRel": 3.0,
        "org_lfo1Rate": 0.03, "org_lfo1Depth": 0.25,
        "org_lfo2Rate": 0.02, "org_lfo2Depth": 0.3,
        "org_reverbMix": 0.45,
    }, (0.35, 0.55, 0.25, 0.2, 0.5, 0.0),
     "Frozen state from seed 31415. A fossil of emergence. LFOs provide the only movement.",
     ["aether", "frozen", "fossil", "static"]),

    ("Maximum Entropy", {
        "org_rule": 30.0, "org_stepRate": 32.0, "org_scope": 1,
        "org_mutate": 0.6,
        "org_oscWave": SAW, "org_subLevel": 0.2,
        "org_filterCutoff": 4000.0, "org_filterRes": 0.4,
        "org_ampAtk": 0.01, "org_ampSus": 0.5,
        "org_lfo1Depth": 0.1, "org_lfo2Depth": 0.1,
    }, (0.5, 0.35, 0.8, 0.4, 0.0, 0.4),
     "60% mutation at max speed. The colony is nearly random. Controlled chaos.",
     ["aether", "entropy", "extreme", "chaos"]),

    ("Geological Pulse", {
        "org_rule": 22.0, "org_stepRate": 0.5, "org_scope": 16,
        "org_oscWave": TRI, "org_subLevel": 0.5,
        "org_filterCutoff": 1500.0, "org_filterRes": 0.2,
        "org_ampAtk": 2.0, "org_ampSus": 0.85, "org_ampRel": 4.0,
        "org_lfo1Rate": 0.01, "org_lfo1Depth": 0.2,
        "org_lfo2Rate": 0.01, "org_lfo2Depth": 0.25,
        "org_reverbMix": 0.5,
    }, (0.25, 0.6, 0.2, 0.2, 0.55, 0.0),
     "One step every 2 seconds. Maximum scope. Geological timescale.",
     ["aether", "geological", "slow", "immense"]),

    ("Rule Space", {
        "org_rule": 54.0, "org_stepRate": 2.0, "org_scope": 12,
        "org_mutate": 0.05,
        "org_oscWave": SAW, "org_subLevel": 0.35,
        "org_filterCutoff": 2200.0, "org_filterRes": 0.25,
        "org_ampAtk": 1.0, "org_ampSus": 0.8, "org_ampRel": 3.0,
        "org_lfo1Rate": 0.04, "org_lfo1Depth": 0.3,
        "org_lfo2Rate": 0.03, "org_lfo2Depth": 0.25,
        "org_reverbMix": 0.55,
        "org_macroRule": 0.5,
    }, (0.35, 0.55, 0.35, 0.25, 0.6, 0.05),
     "RULE macro at center. Sweep to explore the rule space while listening.",
     ["aether", "rule-space", "exploration", "sweep"]),

    ("Singularity", {
        "org_rule": 110.0, "org_stepRate": 4.0, "org_scope": 1,
        "org_mutate": 0.0,
        "org_oscWave": SAW, "org_subLevel": 0.3,
        "org_filterCutoff": 3000.0, "org_filterRes": 0.6,
        "org_ampAtk": 0.01, "org_ampSus": 0.7,
        "org_lfo1Depth": 0.0, "org_lfo2Depth": 0.0,
        "org_reverbMix": 0.0,
    }, (0.45, 0.45, 0.4, 0.25, 0.0, 0.2),
     "Pure Rule 110, scope 1, zero mutation, zero LFO. The automaton alone.",
     ["aether", "singularity", "pure", "raw"]),

    ("Dream Colony", {
        "org_rule": 90.0, "org_stepRate": 1.0, "org_scope": 14,
        "org_mutate": 0.02,
        "org_oscWave": TRI, "org_subLevel": 0.25,
        "org_filterCutoff": 3000.0, "org_filterRes": 0.2,
        "org_ampAtk": 2.0, "org_ampSus": 0.85, "org_ampRel": 4.0,
        "org_lfo1Rate": 0.02, "org_lfo1Depth": 0.25,
        "org_lfo2Rate": 0.015, "org_lfo2Depth": 0.3,
        "org_reverbMix": 0.55,
    }, (0.4, 0.5, 0.3, 0.2, 0.6, 0.0),
     "Fractal rule, deep smoothing, gentle triangle. A dream of emergence.",
     ["aether", "dream", "fractal", "ethereal"]),

    ("Primordial Soup", {
        "org_rule": 30.0, "org_stepRate": 4.0, "org_scope": 8,
        "org_mutate": 0.3,
        "org_oscWave": SAW, "org_subLevel": 0.5,
        "org_filterCutoff": 1200.0, "org_filterRes": 0.3,
        "org_ampAtk": 0.5, "org_ampSus": 0.8, "org_ampRel": 2.0,
        "org_lfo1Rate": 0.06, "org_lfo1Depth": 0.3,
        "org_lfo2Rate": 0.04, "org_lfo2Depth": 0.25,
        "org_reverbMix": 0.35,
    }, (0.25, 0.6, 0.45, 0.35, 0.4, 0.1),
     "High mutation, dark filter. Life forming from disorder.",
     ["aether", "primordial", "dark", "mutation"]),

    ("Colony Ghost", {
        "org_rule": 150.0, "org_stepRate": 2.0, "org_scope": 12,
        "org_oscWave": TRI, "org_subLevel": 0.15,
        "org_filterCutoff": 4000.0, "org_filterRes": 0.15,
        "org_ampAtk": 1.5, "org_ampDec": 2.0, "org_ampSus": 0.2,
        "org_ampRel": 4.0,
        "org_lfo1Rate": 0.03, "org_lfo1Depth": 0.2,
        "org_lfo2Rate": 0.02, "org_lfo2Depth": 0.15,
        "org_reverbMix": 0.6,
    }, (0.4, 0.45, 0.25, 0.15, 0.7, 0.0),
     "Quiet triangle fading into reverb. The ghost of a colony.",
     ["aether", "ghost", "fading", "ethereal"]),

    ("Symmetry Break", {
        "org_rule": 110.0, "org_seed": 32768, "org_stepRate": 4.0, "org_scope": 4,
        "org_mutate": 0.01,
        "org_oscWave": SAW, "org_subLevel": 0.3,
        "org_filterCutoff": 2500.0, "org_filterRes": 0.3,
        "org_ampAtk": 0.5, "org_ampSus": 0.7, "org_ampRel": 2.0,
        "org_lfo1Rate": 0.08, "org_lfo1Depth": 0.2,
        "org_lfo2Rate": 0.06, "org_lfo2Depth": 0.25,
        "org_reverbMix": 0.35,
    }, (0.4, 0.55, 0.4, 0.25, 0.4, 0.05),
     "Seed 32768 (single bit set). Watch how the colony breaks symmetry over time.",
     ["aether", "symmetry", "seed", "emergent"]),

    ("Last Generation", {
        "org_rule": 18.0, "org_stepRate": 0.5, "org_scope": 16,
        "org_mutate": 0.0,
        "org_oscWave": SAW, "org_subLevel": 0.4,
        "org_filterCutoff": 1800.0, "org_filterRes": 0.2,
        "org_ampAtk": 2.0, "org_ampSus": 0.85, "org_ampRel": 5.0,
        "org_lfo1Rate": 0.01, "org_lfo1Depth": 0.2,
        "org_lfo2Rate": 0.01, "org_lfo2Depth": 0.2,
        "org_reverbMix": 0.55,
    }, (0.3, 0.6, 0.2, 0.2, 0.6, 0.0),
     "Sparse Rule 18. The colony is dying out. Each generation has fewer cells.",
     ["aether", "sparse", "dying", "minimal"]),

    ("Emergence Pattern", {
        "org_rule": 110.0, "org_stepRate": 3.0, "org_scope": 6,
        "org_mutate": 0.05,
        "org_oscWave": SAW, "org_subLevel": 0.35,
        "org_filterCutoff": 2500.0, "org_filterRes": 0.3,
        "org_ampAtk": 0.8, "org_ampSus": 0.75, "org_ampRel": 2.5,
        "org_lfo1Rate": 0.05, "org_lfo1Depth": 0.25,
        "org_lfo2Rate": 0.04, "org_lfo2Depth": 0.2,
        "org_reverbMix": 0.4,
        "org_macroRule": 0.25, "org_macroMutate": 0.1,
    }, (0.4, 0.55, 0.4, 0.25, 0.45, 0.05),
     "The flagship aether preset. Emergence itself, audible.",
     ["aether", "emergence", "flagship", "generative"]),

    ("Deep Time", {
        "org_rule": 110.0, "org_stepRate": 0.5, "org_scope": 16,
        "org_mutate": 0.01,
        "org_oscWave": SAW, "org_subLevel": 0.5,
        "org_filterCutoff": 1000.0, "org_filterRes": 0.15,
        "org_ampAtk": 2.0, "org_ampSus": 0.9, "org_ampRel": 5.0,
        "org_lfo1Rate": 0.01, "org_lfo1Depth": 0.15,
        "org_lfo2Rate": 0.01, "org_lfo2Depth": 0.2,
        "org_reverbMix": 0.45,
    }, (0.15, 0.7, 0.2, 0.2, 0.5, 0.0),
     "Sub-bass territory. The colony evolves on timescales beyond human attention.",
     ["aether", "deep-time", "sub", "glacial"]),

    ("Colony Extinction", {
        "org_rule": 22.0, "org_seed": 1, "org_stepRate": 1.0, "org_scope": 16,
        "org_mutate": 0.0,
        "org_oscWave": TRI, "org_subLevel": 0.3,
        "org_filterCutoff": 2000.0, "org_filterRes": 0.2,
        "org_ampAtk": 2.0, "org_ampSus": 0.8, "org_ampRel": 5.0,
        "org_lfo1Rate": 0.02, "org_lfo1Depth": 0.2,
        "org_lfo2Rate": 0.015, "org_lfo2Depth": 0.15,
        "org_reverbMix": 0.5,
    }, (0.3, 0.55, 0.2, 0.15, 0.55, 0.0),
     "Seed 1 under Rule 22. The colony will eventually die. Listen to the end.",
     ["aether", "extinction", "finite", "contemplative"]),

    ("All Rules Journey", {
        "org_rule": 110.0, "org_stepRate": 2.0, "org_scope": 8,
        "org_mutate": 0.04,
        "org_macroRule": 0.0,
        "org_oscWave": SAW, "org_subLevel": 0.35,
        "org_filterCutoff": 2500.0, "org_filterRes": 0.25,
        "org_ampAtk": 1.5, "org_ampSus": 0.85, "org_ampRel": 3.0,
        "org_lfo1Rate": 0.03, "org_lfo1Depth": 0.25,
        "org_lfo2Rate": 0.02, "org_lfo2Depth": 0.2,
        "org_reverbMix": 0.45,
    }, (0.4, 0.55, 0.35, 0.25, 0.5, 0.05),
     "RULE macro at zero. Slowly sweep it from 0 to 1 to journey through all 8 curated rules.",
     ["aether", "journey", "rule-sweep", "meditative"]),
]

# ══════════════════════════════════════════════════════════════════════════════
# ENTANGLED (20): coupling showcases (single-engine but coupling-ready)
# ══════════════════════════════════════════════════════════════════════════════

ENTANGLED = [
    ("Coupling Receiver", {
        "org_rule": 110.0, "org_stepRate": 4.0, "org_scope": 6,
        "org_oscWave": SAW, "org_subLevel": 0.3,
        "org_filterCutoff": 2000.0, "org_filterRes": 0.3,
        "org_macroCoupling": 0.8,
        "org_lfo1Depth": 0.15, "org_lfo2Depth": 0.2,
        "org_reverbMix": 0.15,
    }, (0.4, 0.55, 0.4, 0.25, 0.15, 0.1),
     "High coupling sensitivity. Let other engines shape the colony's filter.",
     ["entangled", "receiver", "coupling", "sensitive"]),

    ("Coupling Donor", {
        "org_rule": 30.0, "org_stepRate": 8.0, "org_scope": 3,
        "org_oscWave": SAW, "org_subLevel": 0.2,
        "org_filterCutoff": 3500.0, "org_filterRes": 0.35,
        "org_velCutoff": 0.7,
        "org_macroCoupling": 0.5,
        "org_lfo1Depth": 0.2, "org_lfo2Depth": 0.15,
    }, (0.5, 0.45, 0.5, 0.3, 0.0, 0.2),
     "Chaotic colony output as coupling donor. Feed emergence to other engines.",
     ["entangled", "donor", "chaotic", "feeding"]),

    ("Colony Sympathy", {
        "org_rule": 110.0, "org_stepRate": 3.0, "org_scope": 8,
        "org_mutate": 0.05,
        "org_oscWave": SAW, "org_subLevel": 0.35,
        "org_filterCutoff": 2500.0, "org_filterRes": 0.25,
        "org_macroCoupling": 0.6,
        "org_lfo1Rate": 0.08, "org_lfo1Depth": 0.2,
        "org_lfo2Rate": 0.06, "org_lfo2Depth": 0.25,
        "org_reverbMix": 0.25,
    }, (0.4, 0.55, 0.4, 0.25, 0.25, 0.05),
     "Sympathetic to fleet coupling. The colony responds to partner engines.",
     ["entangled", "sympathy", "responsive", "fleet"]),

    ("Resonant Entangle", {
        "org_rule": 110.0, "org_stepRate": 4.0, "org_scope": 4,
        "org_oscWave": SAW, "org_subLevel": 0.3,
        "org_filterCutoff": 1500.0, "org_filterRes": 0.6,
        "org_macroCoupling": 0.7,
        "org_lfo1Depth": 0.15, "org_lfo2Depth": 0.2,
    }, (0.4, 0.5, 0.45, 0.3, 0.0, 0.2),
     "High resonance + high coupling. External modulation excites the resonant peak.",
     ["entangled", "resonant", "excitable", "filter"]),

    ("Deep Entangle", {
        "org_rule": 110.0, "org_stepRate": 1.0, "org_scope": 12,
        "org_oscWave": SAW, "org_subLevel": 0.5,
        "org_filterCutoff": 1200.0, "org_filterRes": 0.2,
        "org_macroCoupling": 0.7,
        "org_ampAtk": 1.0, "org_ampSus": 0.85, "org_ampRel": 2.5,
        "org_lfo1Rate": 0.04, "org_lfo1Depth": 0.2,
        "org_lfo2Rate": 0.03, "org_lfo2Depth": 0.25,
        "org_reverbMix": 0.35,
    }, (0.25, 0.65, 0.35, 0.25, 0.4, 0.05),
     "Deep, dark colony with high coupling. Partner engine shapes the low end.",
     ["entangled", "deep", "dark", "sub"]),

    ("Bright Entangle", {
        "org_rule": 90.0, "org_stepRate": 6.0, "org_scope": 4,
        "org_oscWave": TRI, "org_subLevel": 0.15,
        "org_filterCutoff": 5000.0, "org_filterRes": 0.2,
        "org_macroCoupling": 0.6,
        "org_lfo1Depth": 0.2, "org_lfo2Depth": 0.15,
        "org_reverbMix": 0.2,
    }, (0.55, 0.45, 0.45, 0.2, 0.2, 0.05),
     "Bright triangle with fractal rule. Coupling adds movement to the high end.",
     ["entangled", "bright", "triangle", "fractal"]),

    ("Mutation Link", {
        "org_rule": 110.0, "org_stepRate": 4.0, "org_scope": 4,
        "org_mutate": 0.15,
        "org_oscWave": SAW, "org_subLevel": 0.3,
        "org_filterCutoff": 2800.0, "org_filterRes": 0.35,
        "org_macroCoupling": 0.6,
        "org_lfo1Depth": 0.15, "org_lfo2Depth": 0.2,
        "org_reverbMix": 0.15,
    }, (0.45, 0.5, 0.5, 0.3, 0.15, 0.15),
     "Mutating colony with coupling link. Entropy shared between engines.",
     ["entangled", "mutation", "linked", "entropy"]),

    ("Frozen Entangle", {
        "org_rule": 110.0, "org_seed": 65535, "org_freeze": 1,
        "org_oscWave": SAW, "org_subLevel": 0.3,
        "org_filterCutoff": 2500.0, "org_filterRes": 0.3,
        "org_macroCoupling": 0.8,
        "org_lfo1Rate": 0.05, "org_lfo1Depth": 0.2,
        "org_lfo2Rate": 0.03, "org_lfo2Depth": 0.25,
        "org_reverbMix": 0.3,
    }, (0.4, 0.55, 0.3, 0.2, 0.3, 0.05),
     "Frozen colony, max coupling. The partner engine provides ALL movement.",
     ["entangled", "frozen", "dependent", "passive"]),

    ("Scope Entangle", {
        "org_rule": 110.0, "org_stepRate": 4.0, "org_scope": 1,
        "org_oscWave": SAW, "org_subLevel": 0.3,
        "org_filterCutoff": 3000.0, "org_filterRes": 0.35,
        "org_macroCoupling": 0.6,
        "org_lfo1Depth": 0.1, "org_lfo2Depth": 0.15,
    }, (0.45, 0.5, 0.5, 0.25, 0.0, 0.15),
     "Raw scope (1). Every CA step is a timbral jump. Coupling adds pitch modulation.",
     ["entangled", "raw", "scope-1", "jumping"]),

    ("Square Entangle", {
        "org_rule": 150.0, "org_stepRate": 4.0, "org_scope": 6,
        "org_oscWave": SQUARE, "org_subLevel": 0.35,
        "org_filterCutoff": 2000.0, "org_filterRes": 0.35,
        "org_macroCoupling": 0.6,
        "org_lfo1Depth": 0.2, "org_lfo2Depth": 0.2,
        "org_reverbMix": 0.15,
    }, (0.4, 0.55, 0.4, 0.3, 0.15, 0.15),
     "Square wave colony with additive rule. Coupling enriches the harmonic content.",
     ["entangled", "square", "additive", "harmonic"]),

    ("Sub Coupling", {
        "org_rule": 110.0, "org_stepRate": 2.0, "org_scope": 8,
        "org_oscWave": SAW, "org_subLevel": 0.8,
        "org_filterCutoff": 800.0, "org_filterRes": 0.2,
        "org_macroCoupling": 0.9,
        "org_lfo1Depth": 0.0, "org_lfo2Depth": 0.1,
    }, (0.15, 0.7, 0.25, 0.2, 0.0, 0.1),
     "Pure sub colony. Maximum coupling. Let the fleet shape the foundation.",
     ["entangled", "sub", "maximum", "foundation"]),

    ("Percussive Entangle", {
        "org_rule": 30.0, "org_stepRate": 12.0, "org_scope": 2,
        "org_oscWave": SAW, "org_subLevel": 0.2,
        "org_filterCutoff": 4000.0, "org_filterRes": 0.4,
        "org_ampAtk": 0.001, "org_ampDec": 0.15, "org_ampSus": 0.0,
        "org_ampRel": 0.2,
        "org_macroCoupling": 0.6,
        "org_velCutoff": 0.8,
    }, (0.55, 0.4, 0.35, 0.3, 0.0, 0.3),
     "Short percussive hits. Coupling modulates pitch for melodic percussion.",
     ["entangled", "percussive", "short", "pitched"]),

    ("Reverb Entangle", {
        "org_rule": 110.0, "org_stepRate": 3.0, "org_scope": 8,
        "org_oscWave": SAW, "org_subLevel": 0.3,
        "org_filterCutoff": 2500.0, "org_filterRes": 0.25,
        "org_macroCoupling": 0.6,
        "org_ampAtk": 0.5, "org_ampRel": 2.0,
        "org_lfo1Depth": 0.2, "org_lfo2Depth": 0.2,
        "org_reverbMix": 0.5,
    }, (0.4, 0.55, 0.35, 0.25, 0.55, 0.05),
     "Heavy reverb colony. Coupling signal reverberates through shared space.",
     ["entangled", "reverb", "spatial", "wet"]),

    ("Fast Entangle", {
        "org_rule": 30.0, "org_stepRate": 24.0, "org_scope": 2,
        "org_mutate": 0.1,
        "org_oscWave": SAW, "org_subLevel": 0.2,
        "org_filterCutoff": 4000.0, "org_filterRes": 0.35,
        "org_macroCoupling": 0.6,
        "org_lfo1Depth": 0.1, "org_lfo2Depth": 0.1,
    }, (0.5, 0.4, 0.6, 0.3, 0.0, 0.25),
     "Fast chaotic colony + coupling = high-speed timbral exchange.",
     ["entangled", "fast", "chaotic", "exchange"]),

    ("Tri Entangle", {
        "org_rule": 184.0, "org_stepRate": 3.0, "org_scope": 8,
        "org_oscWave": TRI, "org_subLevel": 0.3,
        "org_filterCutoff": 3000.0, "org_filterRes": 0.2,
        "org_macroCoupling": 0.6,
        "org_ampAtk": 0.4, "org_ampRel": 1.5,
        "org_lfo1Depth": 0.2, "org_lfo2Depth": 0.2,
        "org_reverbMix": 0.25,
    }, (0.4, 0.55, 0.35, 0.2, 0.25, 0.05),
     "Gentle triangle with ordered rule. Coupling adds warmth from the fleet.",
     ["entangled", "triangle", "gentle", "warm"]),

    ("Macro Entangle", {
        "org_rule": 110.0, "org_stepRate": 4.0, "org_scope": 4,
        "org_oscWave": SAW, "org_subLevel": 0.3,
        "org_filterCutoff": 2800.0, "org_filterRes": 0.3,
        "org_macroRule": 0.25, "org_macroMutate": 0.15,
        "org_macroCoupling": 0.6,
        "org_lfo1Depth": 0.2, "org_lfo2Depth": 0.2,
        "org_reverbMix": 0.2,
    }, (0.45, 0.5, 0.45, 0.25, 0.2, 0.1),
     "All macros active. COUPLING responds alongside RULE and MUTATE.",
     ["entangled", "macros", "all-active", "performance"]),

    ("Colony Bridge", {
        "org_rule": 110.0, "org_stepRate": 4.0, "org_scope": 6,
        "org_mutate": 0.08,
        "org_oscWave": SAW, "org_subLevel": 0.35,
        "org_filterCutoff": 2500.0, "org_filterRes": 0.3,
        "org_macroCoupling": 0.7,
        "org_lfo1Depth": 0.2, "org_lfo2Depth": 0.2,
        "org_reverbMix": 0.2,
    }, (0.4, 0.55, 0.45, 0.3, 0.2, 0.1),
     "A bridge preset: good for coupling to any engine in the fleet.",
     ["entangled", "bridge", "versatile", "universal"]),

    ("Pitch Entangle", {
        "org_rule": 110.0, "org_stepRate": 6.0, "org_scope": 3,
        "org_oscWave": SAW, "org_subLevel": 0.2,
        "org_filterCutoff": 3500.0, "org_filterRes": 0.25,
        "org_macroCoupling": 0.7,
        "org_lfo1Depth": 0.1, "org_lfo2Depth": 0.1,
        "org_reverbMix": 0.1,
    }, (0.5, 0.45, 0.45, 0.25, 0.1, 0.1),
     "High coupling focused on pitch modulation. Partner engine bends the notes.",
     ["entangled", "pitch", "bend", "coupled"]),

    ("Entangled Pad", {
        "org_rule": 110.0, "org_stepRate": 2.0, "org_scope": 10,
        "org_mutate": 0.05,
        "org_oscWave": SAW, "org_subLevel": 0.35,
        "org_filterCutoff": 2200.0, "org_filterRes": 0.25,
        "org_macroCoupling": 0.6,
        "org_ampAtk": 0.8, "org_ampRel": 2.0,
        "org_lfo1Rate": 0.06, "org_lfo1Depth": 0.25,
        "org_lfo2Rate": 0.04, "org_lfo2Depth": 0.2,
        "org_reverbMix": 0.35,
    }, (0.35, 0.6, 0.4, 0.25, 0.4, 0.05),
     "Pad designed for coupling. Slow evolution + partner input = rich texture.",
     ["entangled", "pad", "slow", "texture"]),

    ("Speed Entangle", {
        "org_rule": 30.0, "org_stepRate": 16.0, "org_scope": 2,
        "org_mutate": 0.05,
        "org_oscWave": SAW, "org_subLevel": 0.25,
        "org_filterCutoff": 3500.0, "org_filterRes": 0.4,
        "org_macroCoupling": 0.6,
        "org_ampAtk": 0.001, "org_ampDec": 0.25, "org_ampSus": 0.3,
        "org_lfo1Depth": 0.15, "org_lfo2Depth": 0.15,
    }, (0.5, 0.45, 0.55, 0.3, 0.0, 0.2),
     "Fast colony + coupling = rapid timbral exchange between engines.",
     ["entangled", "speed", "fast", "exchange"]),
]

# ══════════════════════════════════════════════════════════════════════════════
# FAMILY (15): ORGANISM + fleet engine coupling
# ══════════════════════════════════════════════════════════════════════════════

FAMILY_PRESETS = []
partners = [
    ("Overdub", "Colony meets dub delay. Generative echoes.",
     {"org_rule": 110.0, "org_stepRate": 4.0, "org_scope": 6, "org_oscWave": SAW, "org_subLevel": 0.3,
      "org_filterCutoff": 2500.0, "org_filterRes": 0.3, "org_reverbMix": 0.2, "org_macroCoupling": 0.5},
     (0.4, 0.55, 0.4, 0.25, 0.3, 0.1), ["family", "overdub", "dub", "delay"]),

    ("Odyssey", "Colony drift meets analog drift.",
     {"org_rule": 110.0, "org_stepRate": 2.0, "org_scope": 10, "org_mutate": 0.05, "org_oscWave": SAW,
      "org_subLevel": 0.35, "org_filterCutoff": 2200.0, "org_reverbMix": 0.3, "org_macroCoupling": 0.5},
     (0.35, 0.6, 0.45, 0.25, 0.35, 0.05), ["family", "odyssey", "drift", "analog"]),

    ("Onset", "Colony rhythm meets drum synthesis.",
     {"org_rule": 30.0, "org_stepRate": 8.0, "org_scope": 2, "org_oscWave": SAW, "org_subLevel": 0.3,
      "org_filterCutoff": 3000.0, "org_filterRes": 0.4, "org_ampAtk": 0.001, "org_ampDec": 0.2,
      "org_ampSus": 0.0, "org_macroCoupling": 0.5},
     (0.5, 0.4, 0.3, 0.35, 0.0, 0.35), ["family", "onset", "drums", "percussive"]),

    ("Opal", "Colony meets granular. Texture layers.",
     {"org_rule": 110.0, "org_stepRate": 3.0, "org_scope": 8, "org_mutate": 0.05, "org_oscWave": SAW,
      "org_subLevel": 0.3, "org_filterCutoff": 2500.0, "org_reverbMix": 0.35, "org_macroCoupling": 0.6},
     (0.4, 0.55, 0.45, 0.35, 0.4, 0.05), ["family", "opal", "granular", "texture"]),

    ("Organon", "Colony meets metabolism. Two living systems.",
     {"org_rule": 110.0, "org_stepRate": 4.0, "org_scope": 6, "org_mutate": 0.08, "org_oscWave": SAW,
      "org_subLevel": 0.35, "org_filterCutoff": 2500.0, "org_reverbMix": 0.25, "org_macroCoupling": 0.6},
     (0.4, 0.55, 0.5, 0.3, 0.3, 0.05), ["family", "organon", "metabolic", "living"]),

    ("Ouroboros", "Colony meets strange attractor. Feedback loops.",
     {"org_rule": 30.0, "org_stepRate": 6.0, "org_scope": 4, "org_mutate": 0.12, "org_oscWave": SAW,
      "org_subLevel": 0.3, "org_filterCutoff": 2000.0, "org_filterRes": 0.4, "org_macroCoupling": 0.6},
     (0.4, 0.5, 0.5, 0.35, 0.1, 0.25), ["family", "ouroboros", "feedback", "attractor"]),

    ("Overworld", "Colony meets chip sounds. Digital ecology.",
     {"org_rule": 90.0, "org_stepRate": 8.0, "org_scope": 3, "org_oscWave": SQUARE, "org_subLevel": 0.2,
      "org_filterCutoff": 4000.0, "org_ampAtk": 0.005, "org_ampDec": 0.3, "org_macroCoupling": 0.5},
     (0.5, 0.4, 0.4, 0.3, 0.1, 0.15), ["family", "overworld", "chip", "digital"]),

    ("Oceanic", "Colony meets phosphorescent ocean.",
     {"org_rule": 110.0, "org_stepRate": 2.0, "org_scope": 10, "org_oscWave": SAW, "org_subLevel": 0.4,
      "org_filterCutoff": 1800.0, "org_reverbMix": 0.4, "org_macroCoupling": 0.6,
      "org_lfo1Rate": 0.05, "org_lfo1Depth": 0.25},
     (0.3, 0.65, 0.4, 0.25, 0.45, 0.0), ["family", "oceanic", "phosphorescent", "deep"]),

    ("Oblong", "Colony meets amber warmth.",
     {"org_rule": 184.0, "org_stepRate": 3.0, "org_scope": 8, "org_oscWave": SAW, "org_subLevel": 0.35,
      "org_filterCutoff": 2500.0, "org_reverbMix": 0.2, "org_macroCoupling": 0.5},
     (0.4, 0.6, 0.35, 0.25, 0.2, 0.05), ["family", "oblong", "warm", "amber"]),

    ("Oracle", "Colony meets stochastic prophecy.",
     {"org_rule": 110.0, "org_stepRate": 4.0, "org_scope": 6, "org_mutate": 0.1, "org_oscWave": SAW,
      "org_subLevel": 0.3, "org_filterCutoff": 2500.0, "org_filterRes": 0.3, "org_macroCoupling": 0.6},
     (0.45, 0.5, 0.45, 0.3, 0.2, 0.1), ["family", "oracle", "stochastic", "prophecy"]),

    ("Obsidian", "Colony meets crystal white.",
     {"org_rule": 110.0, "org_stepRate": 3.0, "org_scope": 8, "org_oscWave": TRI, "org_subLevel": 0.2,
      "org_filterCutoff": 3500.0, "org_reverbMix": 0.35, "org_macroCoupling": 0.5,
      "org_ampAtk": 0.5, "org_ampRel": 2.0},
     (0.45, 0.5, 0.35, 0.2, 0.4, 0.0), ["family", "obsidian", "crystal", "clean"]),

    ("Overbite", "Colony meets fang. Bass + evolution.",
     {"org_rule": 30.0, "org_stepRate": 4.0, "org_scope": 4, "org_oscWave": SAW, "org_subLevel": 0.5,
      "org_filterCutoff": 1500.0, "org_filterRes": 0.4, "org_macroCoupling": 0.5},
     (0.35, 0.55, 0.35, 0.3, 0.0, 0.3), ["family", "overbite", "bass", "fang"]),

    ("Obrix", "Colony meets modular reef.",
     {"org_rule": 110.0, "org_stepRate": 4.0, "org_scope": 6, "org_mutate": 0.05, "org_oscWave": SAW,
      "org_subLevel": 0.3, "org_filterCutoff": 2500.0, "org_filterRes": 0.3, "org_reverbMix": 0.2,
      "org_macroCoupling": 0.6},
     (0.45, 0.55, 0.4, 0.3, 0.2, 0.1), ["family", "obrix", "modular", "reef"]),

    ("Overlap", "Colony meets entangled topology.",
     {"org_rule": 110.0, "org_stepRate": 3.0, "org_scope": 8, "org_mutate": 0.08, "org_oscWave": SAW,
      "org_subLevel": 0.3, "org_filterCutoff": 2200.0, "org_reverbMix": 0.3, "org_macroCoupling": 0.7},
     (0.4, 0.55, 0.45, 0.3, 0.3, 0.05), ["family", "overlap", "topology", "entangled"]),

    ("Optic", "Colony meets visual modulation. Emergence becomes light.",
     {"org_rule": 90.0, "org_stepRate": 6.0, "org_scope": 4, "org_oscWave": TRI, "org_subLevel": 0.15,
      "org_filterCutoff": 4000.0, "org_reverbMix": 0.2, "org_macroCoupling": 0.5},
     (0.5, 0.45, 0.45, 0.2, 0.2, 0.05), ["family", "optic", "visual", "light"]),
]

for partner_name, desc, overrides, dna, tags in partners:
    name = f"Colony x {partner_name}"
    FAMILY_PRESETS.append(make_preset(name, "Family", overrides, dna, desc, tags,
        engines=["Organism", partner_name],
        coupling=[{"engineA": "Organism", "engineB": partner_name,
                   "type": "AmpToFilter", "amount": 0.5}],
        coupling_intensity="Medium"))


# ══════════════════════════════════════════════════════════════════════════════
# GENERATE ALL PRESETS
# ══════════════════════════════════════════════════════════════════════════════

def main():
    count = 0
    # Lessons (Foundation)
    for name, overrides, dna, desc, tags in LESSONS:
        save(make_preset(name, "Foundation", overrides, dna, desc, tags))
        count += 1
    # Foundation basics
    for name, overrides, dna, desc, tags in FOUNDATION_BASICS:
        save(make_preset(name, "Foundation", overrides, dna, desc, tags))
        count += 1
    # Atmosphere
    for name, overrides, dna, desc, tags in ATMOSPHERE:
        save(make_preset(name, "Atmosphere", overrides, dna, desc, tags))
        count += 1
    # Prism
    for name, overrides, dna, desc, tags in PRISM:
        save(make_preset(name, "Prism", overrides, dna, desc, tags))
        count += 1
    # Flux
    for name, overrides, dna, desc, tags in FLUX:
        save(make_preset(name, "Flux", overrides, dna, desc, tags))
        count += 1
    # Aether
    for name, overrides, dna, desc, tags in AETHER:
        save(make_preset(name, "Aether", overrides, dna, desc, tags))
        count += 1
    # Entangled
    for name, overrides, dna, desc, tags in ENTANGLED:
        save(make_preset(name, "Entangled", overrides, dna, desc, tags))
        count += 1
    # Family (pre-built)
    for preset in FAMILY_PRESETS:
        save(preset)
        count += 1

    print(f"Generated {count} ORGANISM presets across 7 moods")
    # Distribution check
    from collections import Counter
    moods = Counter()
    moods["Foundation"] = len(LESSONS) + len(FOUNDATION_BASICS)
    moods["Atmosphere"] = len(ATMOSPHERE)
    moods["Prism"] = len(PRISM)
    moods["Flux"] = len(FLUX)
    moods["Aether"] = len(AETHER)
    moods["Entangled"] = len(ENTANGLED)
    moods["Family"] = len(FAMILY_PRESETS)
    for mood, n in sorted(moods.items()):
        print(f"  {mood}: {n}")
    print(f"  TOTAL: {sum(moods.values())}")


if __name__ == "__main__":
    main()
