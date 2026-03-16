#!/usr/bin/env python3
"""
xpn_orca_osteria_owlfish_coverage_pack.py
Coverage expansion for ORCA, OSTERIA, OWLFISH — all three have low partner coverage.

Generates:
  - 6 three-way ORCA × OSTERIA × OWLFISH marquee presets
  - 20 ORCA × partner presets
  - 20 OSTERIA × partner presets
  - 20 OWLFISH × partner presets
Total: 66 presets → Presets/XOmnibus/Entangled/

Python stdlib only. Skips files that already exist.
"""

import json
import os
import random

OUTPUT_DIR = os.path.join(
    os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
    "Presets", "XOmnibus", "Entangled"
)

# ── DNA baselines ────────────────────────────────────────────────────────────

DNA = {
    "ORCA":       {"brightness": 0.2,  "warmth": 0.3,  "movement": 0.7,  "density": 0.8,  "space": 0.6,  "aggression": 0.85},
    "OSTERIA":    {"brightness": 0.55, "warmth": 0.8,  "movement": 0.4,  "density": 0.6,  "space": 0.5,  "aggression": 0.3},
    "OWLFISH":    {"brightness": 0.5,  "warmth": 0.65, "movement": 0.3,  "density": 0.7,  "space": 0.4,  "aggression": 0.4},
    # Partners — representative DNA for blending
    "ODDFELIX":   {"brightness": 0.75, "warmth": 0.35, "movement": 0.65, "density": 0.55, "space": 0.55, "aggression": 0.35},
    "ODDOSCAR":   {"brightness": 0.5,  "warmth": 0.7,  "movement": 0.55, "density": 0.6,  "space": 0.5,  "aggression": 0.2},
    "OVERDUB":    {"brightness": 0.4,  "warmth": 0.65, "movement": 0.35, "density": 0.65, "space": 0.75, "aggression": 0.25},
    "ODYSSEY":    {"brightness": 0.5,  "warmth": 0.55, "movement": 0.45, "density": 0.55, "space": 0.7,  "aggression": 0.2},
    "OBLONG":     {"brightness": 0.6,  "warmth": 0.6,  "movement": 0.5,  "density": 0.65, "space": 0.5,  "aggression": 0.4},
    "OBESE":      {"brightness": 0.65, "warmth": 0.5,  "movement": 0.6,  "density": 0.8,  "space": 0.35, "aggression": 0.75},
    "ONSET":      {"brightness": 0.6,  "warmth": 0.35, "movement": 0.8,  "density": 0.7,  "space": 0.45, "aggression": 0.65},
    "OVERWORLD":  {"brightness": 0.7,  "warmth": 0.4,  "movement": 0.6,  "density": 0.6,  "space": 0.55, "aggression": 0.5},
    "OPAL":       {"brightness": 0.55, "warmth": 0.6,  "movement": 0.45, "density": 0.65, "space": 0.65, "aggression": 0.2},
    "ORBITAL":    {"brightness": 0.65, "warmth": 0.5,  "movement": 0.55, "density": 0.6,  "space": 0.6,  "aggression": 0.45},
    "ORGANON":    {"brightness": 0.5,  "warmth": 0.6,  "movement": 0.6,  "density": 0.75, "space": 0.5,  "aggression": 0.35},
    "OUROBOROS":  {"brightness": 0.45, "warmth": 0.35, "movement": 0.75, "density": 0.85, "space": 0.4,  "aggression": 0.8},
    "OBSIDIAN":   {"brightness": 0.4,  "warmth": 0.4,  "movement": 0.25, "density": 0.7,  "space": 0.6,  "aggression": 0.3},
    "OVERBITE":   {"brightness": 0.45, "warmth": 0.5,  "movement": 0.7,  "density": 0.7,  "space": 0.45, "aggression": 0.7},
    "ORIGAMI":    {"brightness": 0.55, "warmth": 0.45, "movement": 0.55, "density": 0.6,  "space": 0.5,  "aggression": 0.5},
    "ORACLE":     {"brightness": 0.5,  "warmth": 0.45, "movement": 0.5,  "density": 0.7,  "space": 0.55, "aggression": 0.4},
    "OBSCURA":    {"brightness": 0.35, "warmth": 0.5,  "movement": 0.35, "density": 0.65, "space": 0.6,  "aggression": 0.3},
    "OCEANIC":    {"brightness": 0.5,  "warmth": 0.55, "movement": 0.55, "density": 0.7,  "space": 0.7,  "aggression": 0.3},
    "OCELOT":     {"brightness": 0.6,  "warmth": 0.6,  "movement": 0.6,  "density": 0.65, "space": 0.5,  "aggression": 0.55},
    "OPTIC":      {"brightness": 0.75, "warmth": 0.3,  "movement": 0.65, "density": 0.5,  "space": 0.55, "aggression": 0.4},
}

# ── Parameter templates ──────────────────────────────────────────────────────

def orca_params(seed):
    r = random.Random(seed)
    return {
        "orca_wavetablePos":  round(r.uniform(0.1, 0.9), 3),
        "orca_wavetableScan": round(r.uniform(0.2, 0.7), 3),
        "orca_echoDepth":     round(r.uniform(0.3, 0.8), 3),
        "orca_huntMacro":     round(r.uniform(0.0, 0.6), 3),
        "orca_breachAmt":     round(r.uniform(0.4, 0.9), 3),
        "orca_podSync":       round(r.uniform(0.1, 0.6), 3),
        "orca_pitchRand":     round(r.uniform(0.0, 0.25), 3),
        "orca_ampAttack":     round(r.uniform(0.005, 0.1), 4),
        "orca_ampDecay":      round(r.uniform(0.2, 0.8), 3),
        "orca_ampSustain":    round(r.uniform(0.4, 0.85), 3),
        "orca_ampRelease":    round(r.uniform(0.8, 3.0), 2),
        "orca_filterCutoff":  round(r.uniform(0.45, 0.9), 3),
        "orca_filterRes":     round(r.uniform(0.2, 0.7), 3),
        "orca_lfo1Rate":      round(r.uniform(0.03, 0.4), 3),
        "orca_lfo1Depth":     round(r.uniform(0.1, 0.5), 3),
        "orca_lfo2Rate":      round(r.uniform(0.005, 0.08), 4),
        "orca_lfo2Depth":     round(r.uniform(0.05, 0.3), 3),
        "orca_macroCharacter": 0.0,
        "orca_macroPunch":    0.0,
        "orca_macroCoupling": 0.0,
        "orca_macroSpace":    0.0,
    }

def osteria_params(seed):
    r = random.Random(seed)
    return {
        "osteria_qBassShore":     round(r.uniform(0.2, 0.7), 3),
        "osteria_qHarmShore":     round(r.uniform(0.2, 0.7), 3),
        "osteria_qMelShore":      round(r.uniform(0.2, 0.7), 3),
        "osteria_qRhythmShore":   round(r.uniform(0.1, 0.5), 3),
        "osteria_qElastic":       round(r.uniform(0.3, 0.8), 3),
        "osteria_qStretch":       round(r.uniform(0.2, 0.6), 3),
        "osteria_qMemory":        round(r.uniform(0.4, 0.85), 3),
        "osteria_qSympathy":      round(r.uniform(0.3, 0.8), 3),
        "osteria_bassLevel":      round(r.uniform(0.5, 0.85), 3),
        "osteria_harmLevel":      round(r.uniform(0.4, 0.8), 3),
        "osteria_melLevel":       round(r.uniform(0.35, 0.75), 3),
        "osteria_rhythmLevel":    round(r.uniform(0.15, 0.5), 3),
        "osteria_ensWidth":       round(r.uniform(0.3, 0.8), 3),
        "osteria_blendMode":      round(r.uniform(0.1, 0.7), 3),
        "osteria_tavernMix":      round(r.uniform(0.3, 0.7), 3),
        "osteria_tavernShore":    round(r.uniform(0.2, 0.6), 3),
        "osteria_murmur":         round(r.uniform(0.1, 0.5), 3),
        "osteria_warmth":         round(r.uniform(0.55, 0.9), 3),
        "osteria_oceanBleed":     round(r.uniform(0.05, 0.3), 3),
        "osteria_patina":         round(r.uniform(0.2, 0.6), 3),
        "osteria_porto":          round(r.uniform(0.1, 0.5), 3),
        "osteria_smoke":          round(r.uniform(0.1, 0.4), 3),
        "osteria_filterEnvDepth": round(r.uniform(0.15, 0.5), 3),
        "osteria_attack":         round(r.uniform(0.04, 0.3), 3),
        "osteria_decay":          round(r.uniform(0.3, 0.9), 3),
        "osteria_sustain":        round(r.uniform(0.55, 0.9), 3),
        "osteria_release":        round(r.uniform(0.8, 2.5), 2),
        "osteria_sessionDelay":   round(r.uniform(0.0, 0.4), 3),
        "osteria_hall":           round(r.uniform(0.2, 0.6), 3),
        "osteria_chorus":         round(r.uniform(0.0, 0.4), 3),
        "osteria_tape":           round(r.uniform(0.0, 0.3), 3),
        "osteria_macroCharacter": 0.0,
        "osteria_macroMovement":  0.0,
        "osteria_macroCoupling":  0.0,
        "osteria_macroSpace":     0.0,
    }

def owlfish_params(seed):
    r = random.Random(seed)
    return {
        "owl_filterCutoff":     round(r.uniform(0.3, 0.85), 3),
        "owl_filterRes":        round(r.uniform(0.3, 0.8), 3),
        "owl_oscPitch":         round(r.uniform(0.4, 0.6), 3),
        "owl_oscLevel":         round(r.uniform(0.5, 0.9), 3),
        "owl_subLevel":         round(r.uniform(0.1, 0.5), 3),
        "owl_mixturSpread":     round(r.uniform(0.2, 0.7), 3),
        "owl_mixturBlend":      round(r.uniform(0.3, 0.8), 3),
        "owl_resonantPitch":    round(r.uniform(0.3, 0.7), 3),
        "owl_resonantFeedback": round(r.uniform(0.2, 0.65), 3),
        "owl_abyssalDrive":     round(r.uniform(0.15, 0.55), 3),
        "owl_ampAttack":        round(r.uniform(0.02, 0.4), 3),
        "owl_ampDecay":         round(r.uniform(0.3, 1.0), 3),
        "owl_ampSustain":       round(r.uniform(0.45, 0.85), 3),
        "owl_ampRelease":       round(r.uniform(0.8, 3.5), 2),
        "owl_lfoRate":          round(r.uniform(0.01, 0.25), 3),
        "owl_lfoDepth":         round(r.uniform(0.05, 0.35), 3),
        "owl_lfoShape":         round(r.uniform(0.0, 1.0), 3),
        "owl_envDepth":         round(r.uniform(0.2, 0.6), 3),
        "owl_hall":             round(r.uniform(0.25, 0.7), 3),
        "owl_shimmer":          round(r.uniform(0.1, 0.5), 3),
        "owl_macroCharacter":   0.0,
        "owl_macroMovement":    0.0,
        "owl_macroCoupling":    0.0,
        "owl_macroSpace":       0.0,
    }

PARTNER_PARAM_SEEDS = {
    "ODDFELIX":  {"snap_filterCutoff": 0.6, "snap_filterRes": 0.35, "snap_lfoRate": 0.18, "snap_lfoDepth": 0.25, "snap_attack": 0.02, "snap_release": 1.2, "snap_macroCharacter": 0.0, "snap_macroCoupling": 0.0},
    "ODDOSCAR":  {"morph_morph": 0.45, "morph_filterCutoff": 0.55, "morph_lfoRate": 0.1, "morph_attack": 0.1, "morph_release": 2.0, "morph_macroCharacter": 0.0, "morph_macroCoupling": 0.0},
    "OVERDUB":   {"dub_oscWave": 0.3, "dub_sendAmount": 0.5, "dub_tapeDelay": 0.6, "dub_spring": 0.45, "dub_filterCutoff": 0.55, "dub_macroCharacter": 0.0, "dub_macroCoupling": 0.0},
    "ODYSSEY":   {"drift_oscA_mode": 0, "drift_oscA_level": 0.5, "drift_driftDepth": 0.4, "drift_filterCutoff": 4200.0, "drift_attack": 0.3, "drift_release": 2.5, "drift_macroCharacter": 0.0, "drift_macroCoupling": 0.0},
    "OBLONG":    {"bob_fltCutoff": 0.55, "bob_fltReso": 0.3, "bob_lfoRate": 0.12, "bob_attack": 0.05, "bob_release": 1.5, "bob_macroCharacter": 0.0, "bob_macroCoupling": 0.0},
    "OBESE":     {"fat_satDrive": 0.6, "fat_filterCutoff": 0.5, "fat_lfoRate": 0.15, "fat_attack": 0.01, "fat_release": 0.9, "fat_macroMojo": 0.0, "fat_macroCoupling": 0.0},
    "ONSET":     {"perc_kickTune": 0.45, "perc_kickDecay": 0.4, "perc_kickPunch": 0.7, "perc_noiseLevel": 0.25, "perc_macroMachine": 0.0, "perc_macroPunch": 0.0},
    "OVERWORLD": {"ow_era": 0.5, "ow_filterCutoff": 0.55, "ow_lfoRate": 0.1, "ow_attack": 0.08, "ow_release": 1.8, "ow_macroCharacter": 0.0, "ow_macroCoupling": 0.0},
    "OPAL":      {"opal_grainSize": 0.45, "opal_grainDensity": 0.6, "opal_grainPitch": 0.0, "opal_filterCutoff": 0.6, "opal_attack": 0.2, "opal_release": 3.0, "opal_macroCharacter": 0.0, "opal_macroCoupling": 0.0},
    "ORBITAL":   {"orb_brightness": 0.55, "orb_filterCutoff": 0.5, "orb_lfoRate": 0.12, "orb_attack": 0.06, "orb_release": 1.6, "orb_macroCharacter": 0.0, "orb_macroCoupling": 0.0},
    "ORGANON":   {"organon_metabolicRate": 0.5, "organon_filterCutoff": 0.55, "organon_lfoRate": 0.08, "organon_attack": 0.12, "organon_release": 2.2, "organon_macroCharacter": 0.0, "organon_macroCoupling": 0.0},
    "OUROBOROS": {"ouro_topology": 0.5, "ouro_filterCutoff": 0.45, "ouro_lfoRate": 0.2, "ouro_attack": 0.02, "ouro_release": 1.2, "ouro_macroCharacter": 0.0, "ouro_macroCoupling": 0.0},
    "OBSIDIAN":  {"obsidian_depth": 0.5, "obsidian_filterCutoff": 0.45, "obsidian_lfoRate": 0.06, "obsidian_attack": 0.15, "obsidian_release": 2.8, "obsidian_macroCharacter": 0.0, "obsidian_macroCoupling": 0.0},
    "OVERBITE":  {"poss_biteDepth": 0.6, "poss_filterCutoff": 0.5, "poss_lfoRate": 0.18, "poss_attack": 0.01, "poss_release": 0.8, "poss_macroBite": 0.0, "poss_macroCoupling": 0.0},
    "ORIGAMI":   {"origami_foldPoint": 0.5, "origami_filterCutoff": 0.55, "origami_lfoRate": 0.1, "origami_attack": 0.08, "origami_release": 1.4, "origami_macroCharacter": 0.0, "origami_macroCoupling": 0.0},
    "ORACLE":    {"oracle_breakpoints": 0.5, "oracle_filterCutoff": 0.5, "oracle_lfoRate": 0.07, "oracle_attack": 0.2, "oracle_release": 3.0, "oracle_macroCharacter": 0.0, "oracle_macroCoupling": 0.0},
    "OBSCURA":   {"obscura_stiffness": 0.5, "obscura_filterCutoff": 0.45, "obscura_lfoRate": 0.08, "obscura_attack": 0.1, "obscura_release": 2.5, "obscura_macroCharacter": 0.0, "obscura_macroCoupling": 0.0},
    "OCEANIC":   {"ocean_separation": 0.5, "ocean_filterCutoff": 0.55, "ocean_lfoRate": 0.09, "ocean_attack": 0.12, "ocean_release": 2.0, "ocean_macroCharacter": 0.0, "ocean_macroCoupling": 0.0},
    "OCELOT":    {"ocelot_biome": 0.5, "ocelot_filterCutoff": 0.55, "ocelot_lfoRate": 0.14, "ocelot_attack": 0.03, "ocelot_release": 1.1, "ocelot_macroCharacter": 0.0, "ocelot_macroCoupling": 0.0},
    "OPTIC":     {"optic_pulseRate": 0.5, "optic_filterCutoff": 0.55, "optic_lfoRate": 0.2, "optic_attack": 0.01, "optic_release": 0.5, "optic_macroCharacter": 0.0, "optic_macroCoupling": 0.0},
}

COUPLING_TYPES = [
    "HARMONIC_BLEND", "AmpToFilter", "RhythmToBlend", "PitchToPosition",
    "FilterToGrain", "LFO_SYNC", "EnvToMod", "SpectralMorph",
    "WavetableSync", "ResonanceChain", "TimbreShare", "DriftLock"
]

# ── DNA blending ─────────────────────────────────────────────────────────────

def blend_dna(dna_a, dna_b, jitter_seed):
    r = random.Random(jitter_seed)
    result = {}
    for k in dna_a:
        blended = (dna_a[k] + dna_b.get(k, 0.5)) / 2.0
        jitter = r.uniform(-0.1, 0.1)
        result[k] = round(max(0.0, min(1.0, blended + jitter)), 3)
    return result

def blend_dna_3(dna_a, dna_b, dna_c, jitter_seed):
    r = random.Random(jitter_seed)
    result = {}
    for k in dna_a:
        blended = (dna_a[k] + dna_b.get(k, 0.5) + dna_c.get(k, 0.5)) / 3.0
        jitter = r.uniform(-0.1, 0.1)
        result[k] = round(max(0.0, min(1.0, blended + jitter)), 3)
    return result

# ── Preset data definitions ──────────────────────────────────────────────────

# 6 three-way ORCA × OSTERIA × OWLFISH marquee presets
THREEWAY_PRESETS = [
    {
        "name": "Abyssal Shore",
        "description": "ORCA breaches in the shallows — the deep ocean predator surfaces at OSTERIA's cultural shore. OWLFISH's Mixtur-Trautonium tolls from the seafloor like a ship's bell. Three depth zones in conversation.",
        "coupling_desc": "ORCA breach pulses drive OSTERIA ensemble sympathy; OWLFISH resonant feedback threads through both",
        "coupling_type": "SpectralMorph",
        "coupling_amount": 0.72,
        "tags": ["entangled", "three-way", "deep-sea", "shore", "abyssal", "apex", "marquee"],
        "macro_labels": ["BREACH", "WARMTH", "COUPLING", "SPACE"],
        "seed": 1001,
    },
    {
        "name": "Tide Table",
        "description": "The tide schedule governs all three: ORCA's wavetable scans with each swell, OSTERIA's tavern murmur rises and falls with the water line, OWLFISH's oscillator drones at the intertidal threshold.",
        "coupling_desc": "ORCA wavetable position locks to OSTERIA rhythm level; OWLFISH resonant pitch tracks the combined filter state",
        "coupling_type": "WavetableSync",
        "coupling_amount": 0.65,
        "tags": ["entangled", "three-way", "tidal", "organic", "cycling", "marquee"],
        "macro_labels": ["TIDE", "SHORE", "COUPLING", "DEPTH"],
        "seed": 1002,
    },
    {
        "name": "Echolocation Feast",
        "description": "The orca clicks and scans; the osteria clamor echoes from stone walls; the owlfish drone maps the underwater ceiling. Three organisms sharing acoustic space, each revealing the others.",
        "coupling_desc": "ORCA echo depth routes into OSTERIA hall reverb; OWLFISH shimmer expands with ORCA pod sync intensity",
        "coupling_type": "ResonanceChain",
        "coupling_amount": 0.78,
        "tags": ["entangled", "three-way", "echolocation", "resonant", "predator", "marquee"],
        "macro_labels": ["HUNT", "SESSION", "COUPLING", "SPACE"],
        "seed": 1003,
    },
    {
        "name": "Porto Deep",
        "description": "A bottle of porto opened near the pressure gradient — ORCA's dark wavetable bleeds into the wine-warmth of OSTERIA, while OWLFISH's abyssal gold tone diffuses through both like tannin in cold water.",
        "coupling_desc": "OSTERIA porto and warmth bias ORCA filter toward the high-harmonic shelf; OWLFISH abyssal drive calibrates the shared resonance floor",
        "coupling_type": "TimbreShare",
        "coupling_amount": 0.6,
        "tags": ["entangled", "three-way", "deep", "warm", "earthy", "abyssal", "marquee"],
        "macro_labels": ["CHARACTER", "WARMTH", "COUPLING", "SPACE"],
        "seed": 1004,
    },
    {
        "name": "Breach at Closing Time",
        "description": "The tavern empties. The last patron sees something massive arc above the harbor. ORCA's breach is abrupt — OSTERIA's murmur fades to silence — OWLFISH sustains the golden hum of a room that held witnesses.",
        "coupling_desc": "ORCA breach amount triggers OSTERIA murmur ducking; OWLFISH amp release extends with each ORCA pod sync pulse",
        "coupling_type": "EnvToMod",
        "coupling_amount": 0.82,
        "tags": ["entangled", "three-way", "dramatic", "apex", "atmospheric", "marquee"],
        "macro_labels": ["BREACH", "MURMUR", "COUPLING", "SPACE"],
        "seed": 1005,
    },
    {
        "name": "Gold and Black Water",
        "description": "OWLFISH's Abyssal Gold meets ORCA's Deep Ocean — color before sound. OSTERIA's Porto Wine adds the third pigment. The mix is luminous and unsettling: a lamp in a kelp forest at killing depth.",
        "coupling_desc": "OWLFISH Mixtur-Trautonium oscillator phase-locks to ORCA wavetable; OSTERIA patina level tints the shared spectral blend",
        "coupling_type": "HARMONIC_BLEND",
        "coupling_amount": 0.7,
        "tags": ["entangled", "three-way", "spectral", "textural", "luminous", "abyssal", "marquee"],
        "macro_labels": ["GOLD", "DARK", "COUPLING", "SPACE"],
        "seed": 1006,
    },
]

# ── Partner preset definitions ───────────────────────────────────────────────

PARTNERS_20 = [
    "ODDFELIX", "ODDOSCAR", "OVERDUB", "ODYSSEY", "OBLONG",
    "OBESE", "ONSET", "OVERWORLD", "OPAL", "ORBITAL",
    "ORGANON", "OUROBOROS", "OBSIDIAN", "OVERBITE", "ORIGAMI",
    "ORACLE", "OBSCURA", "OCEANIC", "OCELOT", "OPTIC",
]

ENGINE_IDS = {
    "ORCA": "Orca", "OSTERIA": "Osteria", "OWLFISH": "Owlfish",
    "ODDFELIX": "OddfeliX", "ODDOSCAR": "OddOscar", "OVERDUB": "Overdub",
    "ODYSSEY": "Odyssey", "OBLONG": "Oblong", "OBESE": "Obese",
    "ONSET": "Onset", "OVERWORLD": "Overworld", "OPAL": "Opal",
    "ORBITAL": "Orbital", "ORGANON": "Organon", "OUROBOROS": "Ouroboros",
    "OBSIDIAN": "Obsidian", "OVERBITE": "Overbite", "ORIGAMI": "Origami",
    "ORACLE": "Oracle", "OBSCURA": "Obscura", "OCEANIC": "Oceanic",
    "OCELOT": "Ocelot", "OPTIC": "Optic",
}

# Evocative pairing names — 3 groups × 20 partners
ORCA_PAIR_NAMES = {
    "ODDFELIX":  ("Neon Breach",           "The orca snaps through the neon scatter — ORCA's dark wavetable inverts OddfeliX's electric sheen, each breach a flash in the current."),
    "ODDOSCAR":  ("Pink Scar",             "Gill-pink tenderness meets apex aggression — ORCA's echo depth traces the axolotl's regenerative memory, drawing dark water through soft tissue."),
    "OVERDUB":   ("Spring at Depth",       "Overdub's spring reverb meets ORCA in the submarine canyon — the metallic splash of the spring is what the ocean floor hears when something breaches above."),
    "ODYSSEY":   ("Drift Predator",        "ORCA scans the wavetable while ODYSSEY drifts the oscillators — a predator moving through a dreaming seascape, motion indistinguishable from current."),
    "OBLONG":    ("Bob Under",             "OBLONG's amber warmth is submerged by ORCA's deep ocean pressure — the familiar texture of Bob distorted by fathoms of cold water."),
    "OBESE":     ("Saturated Breach",      "Fat saturation meets apex aggression — ORCA drives hard through OBESE's mojo axis, each wavetable scan leaving a saturated wake."),
    "ONSET":     ("Strike Frequency",      "Onset percussion triggers ORCA wavetable snaps — every kick is a breach, every snare a sonar return. Rhythm as echolocation."),
    "OVERWORLD": ("Chip Pod",              "XOverworld's chip-era oscillators tuned to ORCA's pod frequencies — the NES 2A03 pulse waves that a pod of orcas would compose."),
    "OPAL":      ("Grain Migration",       "OPAL's granular cloud parts for the whale — ORCA's wavetable breaches through grain density, each position-snap displacing a field of scattered sound."),
    "ORBITAL":   ("Group Formation",       "ORBITAL's group envelope system maps to ORCA pod sync — the orca pod as a unified rhythmic entity, all voices breathing together."),
    "ORGANON":   ("Metabolic Hunt",        "ORGANON's variational free energy metabolism tracks ORCA's hunt pattern — predatory efficiency as biological optimization."),
    "OUROBOROS": ("Infinite Breach",       "OUROBOROS's chaos leash tied to ORCA — each breach feeds back into the topology, the predator consuming its own echolocation trail."),
    "OBSIDIAN":  ("Black Mirror Water",    "OBSIDIAN's crystal stillness meets ORCA's dark aggression — the surface before and after a breach, obsidian-calm and shattered."),
    "OVERBITE":  ("Double Apex",           "Two biting engines coupled — ORCA's predator geometry locks to OVERBITE's fang architecture, aggression layered against aggression."),
    "ORIGAMI":   ("Fold Line",             "ORIGAMI's fold point determines where ORCA's wavetable creases — the geometry of a breach is origami at scale."),
    "ORACLE":    ("Sonar Prophecy",        "ORACLE's stochastic breakpoints aligned to ORCA's echolocation rhythm — prediction as sonic triangulation, the future as depth map."),
    "OBSCURA":   ("Daguerreotype Breach",  "OBSCURA's silver stillness captures the moment of breach — a daguerreotype of apex motion, the image hazed by water pressure."),
    "OCEANIC":   ("Shore to Trench",       "OCEANIC's phosphorescent teal pairs with ORCA's deep ocean black — the full water column, from bioluminescent shallows to killing depth."),
    "OCELOT":    ("Spotted Predator",      "Two apex hunters from different biomes — OCELOT's tawny rhythm and ORCA's wavetable aggression syncopate across the shoreline."),
    "OPTIC":     ("Scan Line",             "OPTIC's visual pulse rate synchronizes with ORCA's echolocation — the hunt visible as AutoPulse geometry, predation as oscilloscope trace."),
}

OSTERIA_PAIR_NAMES = {
    "ODDFELIX":  ("Neon Tavern",           "The electric tetra schools through OSTERIA's warm session — neon scatter in candlelit stone, OddfeliX's brightness lifting the tavern ceiling."),
    "ODDOSCAR":  ("Axolotl Toast",         "ODDOSCAR's gill-pink warmth rises with OSTERIA's porto — a toast to regeneration, soft tissue and warm wine in the same glass."),
    "OVERDUB":   ("Tape Session",          "Overdub's tape delay captures OSTERIA's ensemble murmur — the session recorded on degrading tape, warmth converting to grain."),
    "ODYSSEY":   ("Shore Dream",           "OSTERIA's tavern fades into ODYSSEY's drift — the session becoming memory, the shore becoming a dreaming pad."),
    "OBLONG":    ("Amber Service",         "OBLONG's amber warmth matches OSTERIA's porto tone — two warm-spectrum engines in sympathy, the session lit from both sides."),
    "OBESE":     ("Fat Shore",             "OBESE's saturated character pushes OSTERIA's ensemble into overdrive — the tavern session that gets too loud, too warm, too saturated."),
    "ONSET":     ("Kitchen Percussion",    "ONSET's drum kit as OSTERIA kitchen percussion — the rhythmic underpinning of the cultural shore meal, cook and musician as one role."),
    "OVERWORLD": ("Era Cuisine",           "OVERWORLD's ERA triangle maps to OSTERIA's shore regions — the food of each era, the music of each coastline, crossed."),
    "OPAL":      ("Granular Broth",        "OPAL's grain field as OSTERIA's slow-simmered stock — granular texture as culinary patience, density as depth of flavor."),
    "ORBITAL":   ("Group Table",           "ORBITAL's group envelope matches OSTERIA's ensemble sympathy — all voices seated, all voices breathing, the table as one instrument."),
    "ORGANON":   ("Metabolic Warmth",      "ORGANON's biological efficiency applied to OSTERIA's cultural warmth — the tavern as living organism, sympathy as metabolism."),
    "OUROBOROS": ("Infinite Session",      "OUROBOROS's recursive topology in OSTERIA's circular session structure — the session that never ends, the song that feeds itself."),
    "OBSIDIAN":  ("White Tablecloth",      "OBSIDIAN's crystal white set against OSTERIA's Porto Wine — formal stillness at the cultural shore, obsidian plates and warm food."),
    "OVERBITE":  ("Shore Predator",        "OVERBITE's fang architecture contrasts OSTERIA's warm hospitality — the guest who bites, the host who offers more."),
    "ORIGAMI":   ("Folded Menu",           "ORIGAMI's fold geometry applied to OSTERIA's menu structure — the tactile architecture of paper and the sensory architecture of food."),
    "ORACLE":    ("Shore Divination",      "ORACLE's stochastic prediction at OSTERIA's cultural table — what the sea will bring tomorrow, read in the current session's harmony."),
    "OBSCURA":   ("Aged Image",            "OBSCURA's daguerreotype silver tones OSTERIA's warmth into aged memory — the photograph of a tavern session that existed forty years ago."),
    "OCEANIC":   ("Teal and Wine",         "OCEANIC's phosphorescent teal meets OSTERIA's porto wine — the harbor at night, the tavern lit, the two colors resolving into each other."),
    "OCELOT":    ("Spotted Shore",         "OCELOT's biome variability applied to OSTERIA's shore regions — the menu changes with the ocelot's territory, flavor as habitat."),
    "OPTIC":     ("Neon Shore Sign",       "OPTIC's visual pulse as OSTERIA's tavern sign — the blinking light that marks the cultural shore, visible from the water."),
}

OWLFISH_PAIR_NAMES = {
    "ODDFELIX":  ("Neon Filament",         "ODDFELIX's electric sheen vibrates through OWLFISH's Mixtur-Trautonium filament — neon tube and oscillator, twin light sources at different depths."),
    "ODDOSCAR":  ("Gill Resonance",        "ODDOSCAR's axolotl gill structure as acoustic resonator for OWLFISH's drone — regenerative form and sustained tone in biological sympathy."),
    "OVERDUB":   ("Dub Drone",             "Overdub's spring reverb extends OWLFISH's Mixtur-Trautonium sustain — the metallic splash diffusing into an abyssal gold hum."),
    "ODYSSEY":   ("Drifting Trautonium",   "ODYSSEY's drift modulation applied to OWLFISH's resonant feedback — the oscillator drifting like a current, the tone never settling."),
    "OBLONG":    ("Warm Filament",         "OBLONG's amber warmth tuned to OWLFISH's abyssal gold — two warm-spectrum oscillators harmonically related across depth zones."),
    "OBESE":     ("Saturated Resonance",   "OBESE's fat saturation feeding OWLFISH's resonant feedback — distorted harmonics amplified by the Mixtur-Trautonium chain."),
    "ONSET":     ("Struck Drone",          "ONSET percussion strikes initiate OWLFISH's resonant decay — each drum hit sets a new drone frequency, rhythm as tuning gesture."),
    "OVERWORLD": ("Chip Drone",            "OVERWORLD's chip oscillators tuned to OWLFISH's Mixtur-Trautonium partials — retro-digital and genuinely-analog, strange harmonic alliance."),
    "OPAL":      ("Granular Shimmer",      "OPAL's grain shimmer extends OWLFISH's abyssal gold tone — individual grains as overtone components of the drone."),
    "ORBITAL":   ("Orbital Resonance",     "ORBITAL's group system phase-locks to OWLFISH's resonant frequency — all orbital voices sharing the Mixtur-Trautonium's pitch center."),
    "ORGANON":   ("Metabolic Drone",       "ORGANON's variational metabolism drives OWLFISH's drone evolution — the organism that tunes itself, the oscillator that breathes."),
    "OUROBOROS": ("Feedback Topology",     "OUROBOROS's recursive structure and OWLFISH's resonant feedback as twin self-referential systems — one topological, one acoustic."),
    "OBSIDIAN":  ("Crystal Drone",         "OBSIDIAN's crystal stillness and OWLFISH's sustained tone — the most static possible pairing, resonance and crystal in equilibrium."),
    "OVERBITE":  ("Biting Resonance",      "OVERBITE's transient bite triggers OWLFISH's resonant ring — the fang that starts the oscillator, the tone that follows the strike."),
    "ORIGAMI":   ("Folded Overtone",       "ORIGAMI's fold point mapping to OWLFISH's Mixtur-Trautonium partial spectrum — spectral folding as timbral origami."),
    "ORACLE":    ("Prophetic Tone",        "ORACLE's stochastic breakpoints modulating OWLFISH's resonant pitch — the drone that predicts, the tone as oracle output."),
    "OBSCURA":   ("Silver and Gold",       "OBSCURA's daguerreotype silver meets OWLFISH's abyssal gold — two metallic tones at different temperatures, the image and the oscillator."),
    "OCEANIC":   ("Phosphor Drone",        "OCEANIC's phosphorescent teal as visible overtone of OWLFISH's abyssal gold — bioluminescent shimmer above the drone's fundamental."),
    "OCELOT":    ("Spotted Resonance",     "OCELOT's biome variability applied to OWLFISH's oscillator tuning — the drone that changes terrain, the Mixtur-Trautonium as habitat."),
    "OPTIC":     ("Light Oscillator",      "OPTIC's visual pulse frequency harmonically related to OWLFISH's Mixtur-Trautonium — the oscillator you can see, the visual engine you can hear."),
}

# ── Preset builders ──────────────────────────────────────────────────────────

def make_filename(name):
    safe = name.replace("/", "-").replace("\\", "-").replace(":", "").replace("*", "").replace("?", "").replace('"', "").replace("<", "").replace(">", "").replace("|", "")
    return safe + ".xometa"

def build_threeway(preset_def):
    seed = preset_def["seed"]
    r = random.Random(seed)
    dna = blend_dna_3(DNA["ORCA"], DNA["OSTERIA"], DNA["OWLFISH"], seed)
    coupling_seed = seed + 500
    rc = random.Random(coupling_seed)
    data = {
        "schema_version": 1,
        "name": preset_def["name"],
        "mood": "Entangled",
        "engines": ["Orca", "Osteria", "Owlfish"],
        "author": "XO_OX Designs — Coverage Pack 2026-03-16",
        "version": "1.0.0",
        "description": preset_def["description"],
        "tags": preset_def["tags"],
        "macroLabels": preset_def["macro_labels"],
        "couplingIntensity": rc.choice(["Moderate", "Strong"]),
        "tempo": None,
        "created": "2026-03-16",
        "dna": dna,
        "parameters": {
            "Orca":    orca_params(seed),
            "Osteria": osteria_params(seed + 100),
            "Owlfish": owlfish_params(seed + 200),
        },
        "coupling": {
            "pairs": [
                {
                    "source": "Orca",
                    "target": "Osteria",
                    "type": preset_def["coupling_type"],
                    "amount": round(preset_def["coupling_amount"] + rc.uniform(-0.08, 0.08), 3),
                    "description": preset_def["coupling_desc"],
                },
                {
                    "source": "Orca",
                    "target": "Owlfish",
                    "type": rc.choice(COUPLING_TYPES),
                    "amount": round(rc.uniform(0.45, 0.75), 3),
                    "description": "ORCA wavetable scan drives OWLFISH Mixtur-Trautonium resonant pitch",
                },
                {
                    "source": "Osteria",
                    "target": "Owlfish",
                    "type": rc.choice(COUPLING_TYPES),
                    "amount": round(rc.uniform(0.35, 0.65), 3),
                    "description": "OSTERIA ensemble sympathy modulates OWLFISH drone timbre and shimmer",
                },
            ]
        },
        "sequencer": None,
    }
    return data

def make_partner_params(partner_key, seed):
    base = PARTNER_PARAM_SEEDS.get(partner_key, {})
    r = random.Random(seed + 9999)
    result = {}
    for k, v in base.items():
        if isinstance(v, float):
            result[k] = round(max(0.0, min(1.0, v + r.uniform(-0.12, 0.12))), 3)
        else:
            result[k] = v
    return result

def build_twoway(primary_key, partner_key, name, description, seed):
    r = random.Random(seed)
    partner_dna = DNA.get(partner_key, {"brightness": 0.5, "warmth": 0.5, "movement": 0.5, "density": 0.5, "space": 0.5, "aggression": 0.4})
    dna = blend_dna(DNA[primary_key], partner_dna, seed)

    primary_id = ENGINE_IDS[primary_key]
    partner_id = ENGINE_IDS.get(partner_key, partner_key.capitalize())

    # Build primary params
    if primary_key == "ORCA":
        primary_params = orca_params(seed)
    elif primary_key == "OSTERIA":
        primary_params = osteria_params(seed)
    else:  # OWLFISH
        primary_params = owlfish_params(seed)

    partner_params = make_partner_params(partner_key, seed)

    coupling_type = r.choice(COUPLING_TYPES)
    coupling_amount = round(r.uniform(0.45, 0.85), 3)

    tags = ["entangled", "coupling", primary_key.lower(), partner_key.lower()]

    data = {
        "schema_version": 1,
        "name": name,
        "mood": "Entangled",
        "engines": [primary_id, partner_id],
        "author": "XO_OX Designs — Coverage Pack 2026-03-16",
        "version": "1.0.0",
        "description": description,
        "tags": tags,
        "macroLabels": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
        "couplingIntensity": r.choice(["Light", "Moderate", "Strong"]),
        "tempo": None,
        "created": "2026-03-16",
        "dna": dna,
        "parameters": {
            primary_id: primary_params,
            partner_id: partner_params,
        },
        "coupling": {
            "pairs": [
                {
                    "source": primary_id,
                    "target": partner_id,
                    "type": coupling_type,
                    "amount": coupling_amount,
                    "description": f"{primary_id} drives {partner_id} via {coupling_type}",
                }
            ]
        },
        "sequencer": None,
    }
    return data

# ── Write helpers ─────────────────────────────────────────────────────────────

def write_preset(data, output_dir):
    filename = make_filename(data["name"])
    path = os.path.join(output_dir, filename)
    if os.path.exists(path):
        print(f"  SKIP (exists): {filename}")
        return False
    with open(path, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2, ensure_ascii=False)
        f.write("\n")
    print(f"  WROTE: {filename}")
    return True

# ── Main ──────────────────────────────────────────────────────────────────────

def main():
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    written = 0
    skipped = 0

    print("\n=== 3-WAY MARQUEE: ORCA × OSTERIA × OWLFISH (6 presets) ===")
    for pd in THREEWAY_PRESETS:
        data = build_threeway(pd)
        ok = write_preset(data, OUTPUT_DIR)
        if ok: written += 1
        else: skipped += 1

    print("\n=== ORCA × 20 PARTNERS (20 presets) ===")
    for i, partner in enumerate(PARTNERS_20):
        name, desc = ORCA_PAIR_NAMES[partner]
        seed = 2000 + i * 37
        data = build_twoway("ORCA", partner, name, desc, seed)
        ok = write_preset(data, OUTPUT_DIR)
        if ok: written += 1
        else: skipped += 1

    print("\n=== OSTERIA × 20 PARTNERS (20 presets) ===")
    for i, partner in enumerate(PARTNERS_20):
        name, desc = OSTERIA_PAIR_NAMES[partner]
        seed = 3000 + i * 41
        data = build_twoway("OSTERIA", partner, name, desc, seed)
        ok = write_preset(data, OUTPUT_DIR)
        if ok: written += 1
        else: skipped += 1

    print("\n=== OWLFISH × 20 PARTNERS (20 presets) ===")
    for i, partner in enumerate(PARTNERS_20):
        name, desc = OWLFISH_PAIR_NAMES[partner]
        seed = 4000 + i * 43
        data = build_twoway("OWLFISH", partner, name, desc, seed)
        ok = write_preset(data, OUTPUT_DIR)
        if ok: written += 1
        else: skipped += 1

    print(f"\n{'='*50}")
    print(f"Done. Written: {written}  Skipped (exist): {skipped}  Total attempted: {written + skipped}")
    print(f"Output dir: {OUTPUT_DIR}")

if __name__ == "__main__":
    main()
