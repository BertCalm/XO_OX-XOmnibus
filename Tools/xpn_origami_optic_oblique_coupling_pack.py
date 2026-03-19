#!/usr/bin/env python3
"""Generate coupling presets for ORIGAMI, OPTIC, and OBLIQUE — the visually-coded engine trio.

Covers:
  - ORIGAMI + OPTIC (fold meets visual pulse): 4 presets
  - ORIGAMI + OBLIQUE (fold meets prism): 4 presets
  - OPTIC + OBLIQUE (digital visual family): 4 presets
  - ORIGAMI + OPTIC + OBLIQUE (3-way): 3 presets
  - ORIGAMI pairs with ORACLE, ODYSSEY, OPAL, OVERWORLD: 12 presets
  - OPTIC pairs with OUROBOROS, ORACLE, OVERWORLD, ORBITAL: 12 presets
  - OBLIQUE pairs with OPAL, ORACLE, OBLONG, OBBLIGATO: 12 presets
  Total: 51 presets — all output to Presets/XOmnibus/Entangled/
"""

import argparse
import json
import random
from pathlib import Path

# ---------------------------------------------------------------------------
# DNA baselines
# ---------------------------------------------------------------------------

DNA = {
    "Origami":   {"brightness": 0.70, "warmth": 0.45, "movement": 0.65, "density": 0.55, "space": 0.55, "aggression": 0.55},
    "Optic":     {"brightness": 0.85, "warmth": 0.30, "movement": 0.90, "density": 0.40, "space": 0.50, "aggression": 0.45},
    "Oblique":   {"brightness": 0.75, "warmth": 0.40, "movement": 0.70, "density": 0.50, "space": 0.60, "aggression": 0.55},
    "Oracle":    {"brightness": 0.50, "warmth": 0.40, "movement": 0.60, "density": 0.70, "space": 0.70, "aggression": 0.40},
    "Odyssey":   {"brightness": 0.55, "warmth": 0.50, "movement": 0.70, "density": 0.50, "space": 0.70, "aggression": 0.30},
    "Opal":      {"brightness": 0.70, "warmth": 0.50, "movement": 0.75, "density": 0.45, "space": 0.80, "aggression": 0.20},
    "Overworld": {"brightness": 0.75, "warmth": 0.40, "movement": 0.50, "density": 0.65, "space": 0.55, "aggression": 0.60},
    "Ouroboros": {"brightness": 0.50, "warmth": 0.40, "movement": 0.85, "density": 0.75, "space": 0.50, "aggression": 0.80},
    "Orbital":   {"brightness": 0.60, "warmth": 0.65, "movement": 0.55, "density": 0.60, "space": 0.60, "aggression": 0.50},
    "Oblong":    {"brightness": 0.60, "warmth": 0.65, "movement": 0.50, "density": 0.65, "space": 0.55, "aggression": 0.50},
    "Obbligato": {"brightness": 0.55, "warmth": 0.65, "movement": 0.60, "density": 0.65, "space": 0.60, "aggression": 0.45},
}


def blend_dna(*engine_names, weights=None):
    """Return a blended DNA dict from 2–3 engine names."""
    keys = ["brightness", "warmth", "movement", "density", "space", "aggression"]
    n = len(engine_names)
    if weights is None:
        weights = [1.0 / n] * n
    result = {}
    for k in keys:
        result[k] = round(sum(DNA[e][k] * weights[i] for i, e in enumerate(engine_names)), 3)
    return result


# ---------------------------------------------------------------------------
# Parameter stubs per engine
# ---------------------------------------------------------------------------

def origami_params(rng):
    return {
        "origami_foldPoint":    round(rng.uniform(0.4, 0.75), 2),
        "origami_foldAmt":      round(rng.uniform(0.4, 0.75), 2),
        "origami_layers":       rng.choice([2, 3, 4]),
        "origami_symmetry":     round(rng.uniform(0.1, 0.5), 2),
        "origami_filterCutoff": round(rng.uniform(0.45, 0.80), 2),
        "origami_filterReso":   round(rng.uniform(0.2, 0.5), 2),
        "origami_oscPitch":     0.0,
        "origami_ampAttack":    round(rng.uniform(0.01, 0.3), 3),
        "origami_ampDecay":     round(rng.uniform(0.3, 0.8), 2),
        "origami_ampSustain":   round(rng.uniform(0.55, 0.85), 2),
        "origami_ampRelease":   round(rng.uniform(0.8, 2.5), 1),
    }


def optic_params(rng):
    return {
        "optic_reactivity":   round(rng.uniform(0.5, 0.85), 2),
        "optic_inputGain":    round(rng.uniform(0.7, 1.0), 2),
        "optic_autoPulse":    rng.choice([0, 1]),
        "optic_pulseRate":    round(rng.uniform(0.15, 0.55), 2),
        "optic_pulseShape":   round(rng.uniform(0.2, 0.7), 2),
        "optic_pulseSwing":   round(rng.uniform(0.1, 0.45), 2),
        "optic_pulseEvolve":  round(rng.uniform(0.3, 0.75), 2),
        "optic_pulseSubdiv":  round(rng.uniform(0.2, 0.6), 2),
        "optic_pulseAccent":  round(rng.uniform(0.2, 0.55), 2),
        "optic_modDepth":     round(rng.uniform(0.4, 0.8), 2),
        "optic_modMixPulse":  round(rng.uniform(0.3, 0.7), 2),
        "optic_modMixSpec":   round(rng.uniform(0.3, 0.7), 2),
        "optic_vizMode":      rng.choice([0, 1, 2]),
        "optic_vizFeedback":  round(rng.uniform(0.3, 0.75), 2),
        "optic_vizSpeed":     round(rng.uniform(0.3, 0.7), 2),
        "optic_vizIntensity": round(rng.uniform(0.4, 0.8), 2),
    }


def oblique_params(rng):
    return {
        "oblq_oscWave":        rng.choice([0, 1, 2, 3]),
        "oblq_oscFold":        round(rng.uniform(0.3, 0.75), 2),
        "oblq_oscDetune":      round(rng.uniform(0.0, 18.0), 1),
        "oblq_level":          round(rng.uniform(0.6, 0.85), 2),
        "oblq_glide":          round(rng.uniform(0.0, 0.12), 3),
        "oblq_percClick":      round(rng.uniform(0.1, 0.7), 2),
        "oblq_percDecay":      round(rng.uniform(0.001, 0.008), 4),
        "oblq_bounceRate":     round(rng.uniform(20.0, 65.0), 1),
        "oblq_bounceGravity":  round(rng.uniform(0.3, 0.75), 2),
        "oblq_bounceDamp":     round(rng.uniform(0.2, 0.65), 2),
        "oblq_bounceCnt":      float(rng.randint(4, 14)),
        "oblq_bounceSwing":    round(rng.uniform(0.0, 0.25), 2),
        "oblq_clickTone":      round(rng.uniform(2000.0, 9000.0), 0),
        "oblq_filterCut":      round(rng.uniform(1500.0, 10000.0), 0),
        "oblq_filterRes":      round(rng.uniform(0.1, 0.65), 2),
        "oblq_attack":         round(rng.uniform(0.001, 0.05), 4),
        "oblq_decay":          round(rng.uniform(0.1, 0.5), 2),
        "oblq_sustain":        round(rng.uniform(0.3, 0.7), 2),
        "oblq_release":        round(rng.uniform(0.1, 0.6), 2),
        "oblq_prismDelay":     round(rng.uniform(20.0, 80.0), 1),
        "oblq_prismSpread":    round(rng.uniform(0.4, 0.9), 2),
        "oblq_prismColor":     round(rng.uniform(0.3, 0.85), 2),
        "oblq_prismWidth":     round(rng.uniform(0.5, 0.95), 2),
        "oblq_prismFeedback":  round(rng.uniform(0.2, 0.75), 2),
        "oblq_prismMix":       round(rng.uniform(0.25, 0.65), 2),
        "oblq_prismDamp":      round(rng.uniform(0.1, 0.5), 2),
        "oblq_phaserRate":     round(rng.uniform(0.2, 2.5), 2),
        "oblq_phaserDepth":    round(rng.uniform(0.2, 0.8), 2),
        "oblq_phaserFeedback": round(rng.uniform(0.1, 0.65), 2),
        "oblq_phaserMix":      round(rng.uniform(0.2, 0.6), 2),
    }


def oracle_params(rng):
    return {
        "oracle_breakpoints":    round(rng.uniform(0.4, 0.75), 2),
        "oracle_macroCharacter": round(rng.uniform(0.4, 0.7), 2),
        "oracle_macroMovement":  round(rng.uniform(0.45, 0.75), 2),
        "oracle_macroCoupling":  round(rng.uniform(0.4, 0.7), 2),
    }


def odyssey_params(rng):
    return {
        "ody_waveform":   rng.choice([0, 1, 2]),
        "ody_filterCut":  round(rng.uniform(0.45, 0.8), 2),
        "ody_filterRes":  round(rng.uniform(0.15, 0.5), 2),
        "ody_attack":     round(rng.uniform(0.01, 0.3), 3),
        "ody_release":    round(rng.uniform(0.5, 2.5), 1),
        "ody_lfoRate":    round(rng.uniform(0.2, 1.2), 2),
        "ody_lfoDepth":   round(rng.uniform(0.1, 0.5), 2),
        "ody_tideDepth":  round(rng.uniform(0.3, 0.7), 2),
        "ody_tideRate":   round(rng.uniform(0.05, 0.4), 3),
    }


def opal_params(rng):
    return {
        "opal_grainSize":    round(rng.uniform(0.05, 0.4), 3),
        "opal_grainDensity": round(rng.uniform(0.4, 0.9), 2),
        "opal_grainPitch":   round(rng.uniform(-0.2, 0.2), 3),
        "opal_spray":        round(rng.uniform(0.1, 0.6), 2),
        "opal_filterCutoff": round(rng.uniform(0.5, 0.85), 2),
        "opal_filterReso":   round(rng.uniform(0.1, 0.45), 2),
        "opal_attack":       round(rng.uniform(0.05, 0.5), 3),
        "opal_release":      round(rng.uniform(0.8, 3.0), 1),
        "opal_reverbMix":    round(rng.uniform(0.3, 0.75), 2),
    }


def overworld_params(rng):
    return {
        "ow_era":          round(rng.uniform(0.0, 1.0), 2),
        "ow_filterCutoff": round(rng.uniform(0.4, 0.85), 2),
        "ow_filterReso":   round(rng.uniform(0.1, 0.5), 2),
        "ow_glitch":       round(rng.uniform(0.0, 0.45), 2),
        "ow_attack":       round(rng.uniform(0.005, 0.15), 4),
        "ow_release":      round(rng.uniform(0.3, 1.8), 1),
        "ow_level":        round(rng.uniform(0.65, 0.85), 2),
    }


def ouroboros_params(rng):
    return {
        "ouro_feedbackAmt": round(rng.uniform(0.55, 0.85), 2),
        "ouro_filterCut":   round(rng.uniform(0.3, 0.75), 2),
        "ouro_filterRes":   round(rng.uniform(0.2, 0.65), 2),
        "ouro_loopSpeed":   round(rng.uniform(0.4, 0.9), 2),
        "ouro_distDrive":   round(rng.uniform(0.2, 0.7), 2),
        "ouro_attack":      round(rng.uniform(0.001, 0.05), 4),
        "ouro_release":     round(rng.uniform(0.3, 1.5), 1),
    }


def orbital_params(rng):
    return {
        "orb_period":      round(rng.uniform(0.3, 0.8), 2),
        "orb_ellipse":     round(rng.uniform(0.1, 0.7), 2),
        "orb_tilt":        round(rng.uniform(0.0, 0.5), 2),
        "orb_filterCut":   round(rng.uniform(0.4, 0.8), 2),
        "orb_filterRes":   round(rng.uniform(0.1, 0.45), 2),
        "orb_attack":      round(rng.uniform(0.01, 0.25), 3),
        "orb_release":     round(rng.uniform(0.5, 2.0), 1),
        "orb_reverbMix":   round(rng.uniform(0.2, 0.6), 2),
    }


def oblong_params(rng):
    return {
        "bob_curiosity":   round(rng.uniform(0.3, 0.75), 2),
        "bob_filterCutoff": round(rng.uniform(0.4, 0.8), 2),
        "bob_filterReso":  round(rng.uniform(0.1, 0.45), 2),
        "bob_attack":      round(rng.uniform(0.01, 0.2), 3),
        "bob_release":     round(rng.uniform(0.4, 1.8), 1),
        "bob_wobble":      round(rng.uniform(0.1, 0.55), 2),
        "bob_level":       round(rng.uniform(0.6, 0.82), 2),
    }


def obbligato_params(rng):
    return {
        "obbl_voice1Level": round(rng.uniform(0.5, 0.85), 2),
        "obbl_voice2Level": round(rng.uniform(0.4, 0.75), 2),
        "obbl_bond":        round(rng.uniform(0.3, 0.8), 2),
        "obbl_filterCut":   round(rng.uniform(0.4, 0.8), 2),
        "obbl_filterRes":   round(rng.uniform(0.1, 0.45), 2),
        "obbl_attack":      round(rng.uniform(0.02, 0.3), 3),
        "obbl_release":     round(rng.uniform(0.5, 2.2), 1),
        "obbl_reverbMix":   round(rng.uniform(0.2, 0.65), 2),
    }


ENGINE_PARAMS = {
    "Origami":   origami_params,
    "Optic":     optic_params,
    "Oblique":   oblique_params,
    "Oracle":    oracle_params,
    "Odyssey":   odyssey_params,
    "Opal":      opal_params,
    "Overworld": overworld_params,
    "Ouroboros": ouroboros_params,
    "Orbital":   orbital_params,
    "Oblong":    oblong_params,
    "Obbligato": obbligato_params,
}


# ---------------------------------------------------------------------------
# Preset builder
# ---------------------------------------------------------------------------

def make_preset(name, desc, tags, engines, coupling_pairs, dna, macro_labels=None, rng=None):
    """Build a complete .xometa dict."""
    if rng is None:
        rng = random.Random()
    parameters = {}
    for eng in engines:
        fn = ENGINE_PARAMS.get(eng)
        if fn:
            parameters[eng] = fn(rng)

    intensity_val = max(abs(p.get("amount", 0.5)) for p in coupling_pairs)
    if intensity_val >= 0.75:
        intensity = "Deep"
    elif intensity_val >= 0.5:
        intensity = "Medium"
    else:
        intensity = "Light"

    return {
        "schema_version": 1,
        "name": name,
        "mood": "Entangled",
        "engines": engines,
        "author": "XO_OX Designs — Origami/Optic/Oblique Pack 2026",
        "version": "1.0.0",
        "description": desc,
        "tags": ["entangled", "coupling"] + tags,
        "macroLabels": macro_labels or ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
        "couplingIntensity": intensity,
        "tempo": None,
        "dna": dna,
        "parameters": parameters,
        "coupling": {"pairs": coupling_pairs},
        "sequencer": None,
    }


# ---------------------------------------------------------------------------
# Preset definitions
# ---------------------------------------------------------------------------

def build_all_presets(rng):
    presets = []

    # ============================================================
    # ORIGAMI + OPTIC  (fold meets visual pulse)  — 4 presets
    # ============================================================
    presets.append(make_preset(
        name="Fold Pulse",
        desc="OPTIC AutoPulse drives ORIGAMI fold point — each visual beat collapses and reopens the paper harmonic structure. The fold becomes a rhythm.",
        tags=["fold", "pulse", "rhythm", "origami", "optic"],
        engines=["Origami", "Optic"],
        coupling_pairs=[{"source": "Optic", "target": "Origami", "type": "PulseToFold", "amount": 0.68}],
        dna=blend_dna("Origami", "Optic"),
        macro_labels=["FOLD DEPTH", "PULSE RATE", "COUPLING", "SPACE"],
        rng=rng,
    ))

    presets.append(make_preset(
        name="Phosphor Crease",
        desc="ORIGAMI's fold harmonic feeds OPTIC visualization input — the sound of folded paper becomes a phosphor-green waveform display. Visual synthesis from acoustic origin.",
        tags=["phosphor", "crease", "visual", "origami", "optic"],
        engines=["Origami", "Optic"],
        coupling_pairs=[{"source": "Origami", "target": "Optic", "type": "AudioToViz", "amount": 0.62}],
        dna=blend_dna("Origami", "Optic", weights=[0.55, 0.45]),
        macro_labels=["FOLD", "PHOSPHOR", "COUPLING", "SPACE"],
        rng=rng,
    ))

    presets.append(make_preset(
        name="Retinal Score",
        desc="OPTIC modulation depth sculpts ORIGAMI score lines — visualization feedback becomes the instruction set for the fold. Light writes the geometry.",
        tags=["retinal", "score", "light", "origami", "optic"],
        engines=["Origami", "Optic"],
        coupling_pairs=[
            {"source": "Optic", "target": "Origami", "type": "ModToSymmetry", "amount": 0.55},
            {"source": "Origami", "target": "Optic", "type": "EnvToVizSpeed", "amount": 0.38},
        ],
        dna=blend_dna("Origami", "Optic", weights=[0.5, 0.5]),
        macro_labels=["SCORE", "RETINAL", "COUPLING", "SPACE"],
        rng=rng,
    ))

    presets.append(make_preset(
        name="Zero Sound Vector",
        desc="OPTIC's zero-audio spectral model feeds ORIGAMI as carrier — visualization data becomes pitch material. A sound generated from a picture of a sound.",
        tags=["zero-sound", "vector", "spectral", "origami", "optic"],
        engines=["Origami", "Optic"],
        coupling_pairs=[{"source": "Optic", "target": "Origami", "type": "SpectralToCarrier", "amount": 0.72}],
        dna=blend_dna("Origami", "Optic", weights=[0.45, 0.55]),
        macro_labels=["VECTOR", "SIGNAL", "COUPLING", "SPACE"],
        rng=rng,
    ))

    # ============================================================
    # ORIGAMI + OBLIQUE  (fold meets prism)  — 4 presets
    # ============================================================
    presets.append(make_preset(
        name="Prism Fold",
        desc="OBLIQUE prism delay refracts ORIGAMI's fold harmonics — each prism band holds a different fold layer. The crease disperses into spectrum.",
        tags=["prism", "fold", "refract", "origami", "oblique"],
        engines=["Origami", "Oblique"],
        coupling_pairs=[{"source": "Origami", "target": "Oblique", "type": "HarmonicToPrism", "amount": 0.65}],
        dna=blend_dna("Origami", "Oblique"),
        macro_labels=["FOLD", "PRISM", "COUPLING", "SPACE"],
        rng=rng,
    ))

    presets.append(make_preset(
        name="Diagonal Crease",
        desc="OBLIQUE bounce angle modulates ORIGAMI fold point — the prism trajectory defines where the paper bends. Geometry coupling geometry.",
        tags=["diagonal", "crease", "geometry", "origami", "oblique"],
        engines=["Origami", "Oblique"],
        coupling_pairs=[
            {"source": "Oblique", "target": "Origami", "type": "BounceToFold", "amount": 0.60},
            {"source": "Origami", "target": "Oblique", "type": "FoldToSpread", "amount": 0.42},
        ],
        dna=blend_dna("Origami", "Oblique", weights=[0.5, 0.5]),
        macro_labels=["CREASE", "BOUNCE", "COUPLING", "SPACE"],
        rng=rng,
    ))

    presets.append(make_preset(
        name="Spectrum Pleat",
        desc="ORIGAMI layered pleats feed OBLIQUE phaser — each pleat layer becomes a phase stage. Structural harmony through optical dispersal.",
        tags=["spectrum", "pleat", "phase", "origami", "oblique"],
        engines=["Origami", "Oblique"],
        coupling_pairs=[{"source": "Origami", "target": "Oblique", "type": "LayerToPhaser", "amount": 0.58}],
        dna=blend_dna("Origami", "Oblique", weights=[0.55, 0.45]),
        macro_labels=["PLEAT", "SPECTRUM", "COUPLING", "SPACE"],
        rng=rng,
    ))

    presets.append(make_preset(
        name="Refract Unfold",
        desc="OBLIQUE prism color drives ORIGAMI filter cutoff — prismatic warmth opens the fold, prism cool closes it. Color as envelope.",
        tags=["refract", "unfold", "color", "origami", "oblique"],
        engines=["Origami", "Oblique"],
        coupling_pairs=[{"source": "Oblique", "target": "Origami", "type": "PrismColorToFilter", "amount": 0.63}],
        dna=blend_dna("Origami", "Oblique", weights=[0.45, 0.55]),
        macro_labels=["REFRACT", "UNFOLD", "COUPLING", "SPACE"],
        rng=rng,
    ))

    # ============================================================
    # OPTIC + OBLIQUE  (digital visual family)  — 4 presets
    # ============================================================
    presets.append(make_preset(
        name="Phosphor Bounce",
        desc="The digital visual family in full contact — OPTIC phosphor pulse rate syncs to OBLIQUE bounce gravity. A display that moves like a thrown object.",
        tags=["phosphor", "bounce", "digital", "optic", "oblique", "visual-family"],
        engines=["Optic", "Oblique"],
        coupling_pairs=[{"source": "Optic", "target": "Oblique", "type": "PulseToGravity", "amount": 0.66}],
        dna=blend_dna("Optic", "Oblique"),
        macro_labels=["PULSE", "BOUNCE", "COUPLING", "SPACE"],
        rng=rng,
    ))

    presets.append(make_preset(
        name="Visual Prism",
        desc="OBLIQUE prism spread modulates OPTIC visualization mode — the dispersed spectrum becomes the display instruction. Refraction as UI.",
        tags=["visual", "prism", "display", "optic", "oblique", "visual-family"],
        engines=["Optic", "Oblique"],
        coupling_pairs=[
            {"source": "Oblique", "target": "Optic", "type": "PrismToVizMode", "amount": 0.55},
            {"source": "Optic", "target": "Oblique", "type": "VizFeedbackToPrism", "amount": 0.48},
        ],
        dna=blend_dna("Optic", "Oblique", weights=[0.45, 0.55]),
        macro_labels=["VISUAL", "PRISM", "COUPLING", "SPACE"],
        rng=rng,
    ))

    presets.append(make_preset(
        name="Signal Angle",
        desc="OPTIC signal reactivity controls OBLIQUE bounce angle — high-signal moments produce extreme trajectory changes. The data drives the physics.",
        tags=["signal", "angle", "reactive", "optic", "oblique", "visual-family"],
        engines=["Optic", "Oblique"],
        coupling_pairs=[{"source": "Optic", "target": "Oblique", "type": "ReactivityToAngle", "amount": 0.70}],
        dna=blend_dna("Optic", "Oblique", weights=[0.5, 0.5]),
        macro_labels=["SIGNAL", "ANGLE", "COUPLING", "SPACE"],
        rng=rng,
    ))

    presets.append(make_preset(
        name="AutoPulse Prismatic",
        desc="OPTIC AutoPulse subdivisions drive OBLIQUE bounce count — each rhythmic subdivision becomes a bounce event. Visual rhythm becomes kinetic rhythm.",
        tags=["autopulse", "prismatic", "rhythm", "optic", "oblique", "visual-family"],
        engines=["Optic", "Oblique"],
        coupling_pairs=[{"source": "Optic", "target": "Oblique", "type": "SubdivToBounceCount", "amount": 0.62}],
        dna=blend_dna("Optic", "Oblique", weights=[0.55, 0.45]),
        macro_labels=["AUTOPULSE", "PRISMATIC", "COUPLING", "SPACE"],
        rng=rng,
    ))

    # ============================================================
    # ORIGAMI + OPTIC + OBLIQUE  (3-way)  — 3 presets
    # ============================================================
    presets.append(make_preset(
        name="Visual Fold Prism",
        desc="All three visual-coded engines in full triangle coupling — OPTIC pulse folds ORIGAMI, ORIGAMI fold disperses through OBLIQUE prism, OBLIQUE bounce feeds OPTIC viz. A closed visual loop.",
        tags=["three-way", "visual-family", "fold", "prism", "visual-fold-prism"],
        engines=["Origami", "Optic", "Oblique"],
        coupling_pairs=[
            {"source": "Optic",    "target": "Origami", "type": "PulseToFold",     "amount": 0.60},
            {"source": "Origami",  "target": "Oblique", "type": "HarmonicToPrism", "amount": 0.55},
            {"source": "Oblique",  "target": "Optic",   "type": "BounceToViz",     "amount": 0.50},
        ],
        dna=blend_dna("Origami", "Optic", "Oblique"),
        macro_labels=["FOLD", "PHOSPHOR", "PRISM", "SPACE"],
        rng=rng,
    ))

    presets.append(make_preset(
        name="Phosphor Crease Bounce",
        desc="OPTIC phosphor texture creases ORIGAMI and both scatter through OBLIQUE bounce. The display, the fold, and the throw — simultaneous.",
        tags=["three-way", "phosphor", "crease", "bounce", "visual-family"],
        engines=["Origami", "Optic", "Oblique"],
        coupling_pairs=[
            {"source": "Optic",   "target": "Origami", "type": "ModToSymmetry",   "amount": 0.58},
            {"source": "Optic",   "target": "Oblique", "type": "PulseToGravity",  "amount": 0.52},
            {"source": "Origami", "target": "Oblique", "type": "LayerToPhaser",   "amount": 0.45},
        ],
        dna=blend_dna("Origami", "Optic", "Oblique", weights=[0.35, 0.35, 0.30]),
        macro_labels=["CREASE", "BOUNCE", "PHOSPHOR", "SPACE"],
        rng=rng,
    ))

    presets.append(make_preset(
        name="The Visual Family",
        desc="Portrait of three: ORIGAMI unfolds into spectrum, OPTIC reads it as signal, OBLIQUE throws it across the room. The complete visual-coded lineage in one gesture.",
        tags=["three-way", "visual-family", "portrait", "origami", "optic", "oblique"],
        engines=["Origami", "Optic", "Oblique"],
        coupling_pairs=[
            {"source": "Origami", "target": "Optic",   "type": "AudioToViz",     "amount": 0.65},
            {"source": "Optic",   "target": "Oblique", "type": "VizFeedbackToPrism", "amount": 0.60},
            {"source": "Oblique", "target": "Origami", "type": "PrismColorToFilter", "amount": 0.50},
        ],
        dna=blend_dna("Origami", "Optic", "Oblique"),
        macro_labels=["FOLD", "VISUAL", "PRISM", "SPACE"],
        rng=rng,
    ))

    # ============================================================
    # ORIGAMI pairs  (ORACLE, ODYSSEY, OPAL, OVERWORLD) × 3 each
    # ============================================================

    # ORIGAMI + ORACLE
    origami_oracle_specs = [
        ("Oracle Unfold",    "fold", "ORACLE breakpoints modulate ORIGAMI fold layers — prophecy as harmonic architecture.",
         ["oracle", "fold", "stochastic"],
         [{"source": "Oracle", "target": "Origami", "type": "BreakpointToLayers", "amount": 0.62}]),
        ("Fold Prediction",  "fold", "ORIGAMI fold symmetry feeds ORACLE character — the geometry of the paper shapes what the oracle sees.",
         ["oracle", "fold", "symmetry"],
         [{"source": "Origami", "target": "Oracle", "type": "SymmetryToCharacter", "amount": 0.55}]),
        ("Harmonic Oracle",  "harmonic", "Bidirectional fold-oracle exchange — each fold opens a new prediction state, each state reshapes the fold.",
         ["oracle", "fold", "bidirectional"],
         [{"source": "Oracle", "target": "Origami", "type": "StateToFoldPoint", "amount": 0.58},
          {"source": "Origami", "target": "Oracle", "type": "LayerToBreakpoint", "amount": 0.40}]),
    ]
    for name, tag, desc, tags, pairs in origami_oracle_specs:
        presets.append(make_preset(name, desc, ["origami", "oracle"] + [tag] + tags,
                                   ["Origami", "Oracle"], pairs,
                                   blend_dna("Origami", "Oracle"), rng=rng))

    # ORIGAMI + ODYSSEY
    origami_odyssey_specs = [
        ("Tide Fold",       "tidal", "ODYSSEY tidal system modulates ORIGAMI fold point in slow waves — the ocean folds the paper.",
         ["odyssey", "tidal"],
         [{"source": "Odyssey", "target": "Origami", "type": "TideToFoldPoint", "amount": 0.60}]),
        ("Pleat Current",   "drift", "ORIGAMI pleat layers feed ODYSSEY LFO depth — the paper's strata become the depth of the current.",
         ["odyssey", "drift"],
         [{"source": "Origami", "target": "Odyssey", "type": "LayerToLFODepth", "amount": 0.52}]),
        ("Originate Voyage","voyage", "ORIGAMI's origin point drifts with ODYSSEY's evolutionary current — a slow transformation across harmonic space.",
         ["odyssey", "evolve"],
         [{"source": "Odyssey", "target": "Origami", "type": "DriftToSymmetry", "amount": 0.55},
          {"source": "Origami", "target": "Odyssey", "type": "FoldToTideRate", "amount": 0.38}]),
    ]
    for name, tag, desc, tags, pairs in origami_odyssey_specs:
        presets.append(make_preset(name, desc, ["origami", "odyssey"] + [tag] + tags,
                                   ["Origami", "Odyssey"], pairs,
                                   blend_dna("Origami", "Odyssey"), rng=rng))

    # ORIGAMI + OPAL
    origami_opal_specs = [
        ("Grain Fold",     "granular", "OPAL grain density controls ORIGAMI layer count — dense clouds produce maximum fold complexity.",
         ["opal", "granular"],
         [{"source": "Opal", "target": "Origami", "type": "GrainDensityToLayers", "amount": 0.65}]),
        ("Paper Cloud",    "cloud", "ORIGAMI fold harmonics feed OPAL as grain pitch scatter source — folded structure diffuses into cloud.",
         ["opal", "cloud"],
         [{"source": "Origami", "target": "Opal", "type": "HarmonicToGrainPitch", "amount": 0.58}]),
        ("Score Spray",    "spray", "ORIGAMI score lines set OPAL spray boundaries — the geometry of paper defines the scatter field.",
         ["opal", "spray"],
         [{"source": "Origami", "target": "Opal", "type": "SymmetryToSpray", "amount": 0.55},
          {"source": "Opal", "target": "Origami", "type": "GrainSizeToFoldAmt", "amount": 0.42}]),
    ]
    for name, tag, desc, tags, pairs in origami_opal_specs:
        presets.append(make_preset(name, desc, ["origami", "opal"] + [tag] + tags,
                                   ["Origami", "Opal"], pairs,
                                   blend_dna("Origami", "Opal"), rng=rng))

    # ORIGAMI + OVERWORLD
    origami_overworld_specs = [
        ("Pixel Fold",     "pixel", "OVERWORLD era position modulates ORIGAMI fold point — different chip eras produce different fold geometries.",
         ["overworld", "pixel"],
         [{"source": "Overworld", "target": "Origami", "type": "EraToFoldPoint", "amount": 0.60}]),
        ("Chip Crease",    "chip", "ORIGAMI crease amplitude drives OVERWORLD filter — the fold's intensity opens the chip texture.",
         ["overworld", "chip"],
         [{"source": "Origami", "target": "Overworld", "type": "AmpToFilter", "amount": 0.58}]),
        ("Glitch Vector",  "glitch", "OVERWORLD glitch events trigger ORIGAMI layer changes — pixel errors become fold events.",
         ["overworld", "glitch"],
         [{"source": "Overworld", "target": "Origami", "type": "GlitchToLayers", "amount": 0.65},
          {"source": "Origami", "target": "Overworld", "type": "FoldToGlitchRate", "amount": 0.45}]),
    ]
    for name, tag, desc, tags, pairs in origami_overworld_specs:
        presets.append(make_preset(name, desc, ["origami", "overworld"] + [tag] + tags,
                                   ["Origami", "Overworld"], pairs,
                                   blend_dna("Origami", "Overworld"), rng=rng))

    # ============================================================
    # OPTIC pairs  (OUROBOROS, ORACLE, OVERWORLD, ORBITAL) × 3 each
    # ============================================================

    # OPTIC + OUROBOROS
    optic_ouro_specs = [
        ("Feedback Flash",  "feedback", "OUROBOROS feedback amplitude drives OPTIC reactivity — the more it feeds back, the more the display reacts.",
         ["ouroboros", "feedback"],
         [{"source": "Ouroboros", "target": "Optic", "type": "FeedbackToReactivity", "amount": 0.72}]),
        ("Loop Signal",     "loop", "OPTIC pulse events trigger OUROBOROS loop restart — visualization beats reset the feedback cycle.",
         ["ouroboros", "loop"],
         [{"source": "Optic", "target": "Ouroboros", "type": "PulseToLoopReset", "amount": 0.65}]),
        ("Retinal Ouroboros", "serpent", "Bidirectional visual-feedback loop — OPTIC reads OUROBOROS's self-eating signal, OUROBOROS reacts to what OPTIC displays.",
         ["ouroboros", "bidirectional"],
         [{"source": "Ouroboros", "target": "Optic", "type": "AudioToViz", "amount": 0.60},
          {"source": "Optic", "target": "Ouroboros", "type": "VizFeedbackToFeedback", "amount": 0.55}]),
    ]
    for name, tag, desc, tags, pairs in optic_ouro_specs:
        presets.append(make_preset(name, desc, ["optic", "ouroboros"] + [tag] + tags,
                                   ["Optic", "Ouroboros"], pairs,
                                   blend_dna("Optic", "Ouroboros"), rng=rng))

    # OPTIC + ORACLE
    optic_oracle_specs = [
        ("Prophecy Display", "prophecy", "ORACLE prediction state routes to OPTIC visualization mode — future states change what is shown.",
         ["oracle", "prophecy"],
         [{"source": "Oracle", "target": "Optic", "type": "StateToVizMode", "amount": 0.60}]),
        ("Visual Oracle",   "visual", "OPTIC display intensity feeds ORACLE character — bright displays predict bold outcomes.",
         ["oracle", "visual"],
         [{"source": "Optic", "target": "Oracle", "type": "IntensityToCharacter", "amount": 0.55}]),
        ("Flash Breakpoint", "flash", "OPTIC pulse events become ORACLE breakpoint markers — each flash is a prophetic moment.",
         ["oracle", "flash"],
         [{"source": "Optic", "target": "Oracle", "type": "PulseToBreakpoint", "amount": 0.62},
          {"source": "Oracle", "target": "Optic", "type": "BreakpointToVizSpeed", "amount": 0.45}]),
    ]
    for name, tag, desc, tags, pairs in optic_oracle_specs:
        presets.append(make_preset(name, desc, ["optic", "oracle"] + [tag] + tags,
                                   ["Optic", "Oracle"], pairs,
                                   blend_dna("Optic", "Oracle"), rng=rng))

    # OPTIC + OVERWORLD
    optic_overworld_specs = [
        ("Chip Display",    "chip", "OVERWORLD era controls OPTIC viz mode — NES era triggers scanline display, Genesis era triggers waveform.",
         ["overworld", "chip"],
         [{"source": "Overworld", "target": "Optic", "type": "EraToVizMode", "amount": 0.60}]),
        ("Pixel Pulse",     "pixel", "OPTIC pulse rate drives OVERWORLD glitch frequency — the display clock sets the pixel error rate.",
         ["overworld", "pixel"],
         [{"source": "Optic", "target": "Overworld", "type": "PulseRateToGlitch", "amount": 0.65}]),
        ("AutoPulse Era",   "era", "OPTIC AutoPulse subdivisions trigger OVERWORLD era jumps — each pulse accent shifts the chip era.",
         ["overworld", "era"],
         [{"source": "Optic", "target": "Overworld", "type": "AccentToEra", "amount": 0.58},
          {"source": "Overworld", "target": "Optic", "type": "GlitchToVizFeedback", "amount": 0.42}]),
    ]
    for name, tag, desc, tags, pairs in optic_overworld_specs:
        presets.append(make_preset(name, desc, ["optic", "overworld"] + [tag] + tags,
                                   ["Optic", "Overworld"], pairs,
                                   blend_dna("Optic", "Overworld"), rng=rng))

    # OPTIC + ORBITAL
    optic_orbital_specs = [
        ("Orbital Display", "orbital", "ORBITAL period drives OPTIC viz speed — slow orbits produce slow visual sweeps, fast ellipses flicker.",
         ["orbital", "period"],
         [{"source": "Orbital", "target": "Optic", "type": "PeriodToVizSpeed", "amount": 0.60}]),
        ("Signal Orbit",    "orbit", "OPTIC signal reactivity modulates ORBITAL ellipse ratio — reactive moments stretch the orbit.",
         ["orbital", "ellipse"],
         [{"source": "Optic", "target": "Orbital", "type": "ReactivityToEllipse", "amount": 0.58}]),
        ("Flash Perihelion", "perihelion", "OPTIC flash moments trigger ORBITAL tilt shifts — closest approach changes display geometry.",
         ["orbital", "perihelion"],
         [{"source": "Optic", "target": "Orbital", "type": "PulseToTilt", "amount": 0.55},
          {"source": "Orbital", "target": "Optic", "type": "TiltToVizIntensity", "amount": 0.48}]),
    ]
    for name, tag, desc, tags, pairs in optic_orbital_specs:
        presets.append(make_preset(name, desc, ["optic", "orbital"] + [tag] + tags,
                                   ["Optic", "Orbital"], pairs,
                                   blend_dna("Optic", "Orbital"), rng=rng))

    # ============================================================
    # OBLIQUE pairs  (OPAL, ORACLE, OBLONG, OBBLIGATO) × 3 each
    # ============================================================

    # OBLIQUE + OPAL
    oblique_opal_specs = [
        ("Grain Bounce",    "granular", "OPAL grain size sets OBLIQUE bounce height — large grains produce high-arc trajectories.",
         ["opal", "granular"],
         [{"source": "Opal", "target": "Oblique", "type": "GrainSizeToGravity", "amount": 0.62}]),
        ("Prism Cloud",     "cloud", "OBLIQUE prism spread diffuses OPAL grain positions — the bounce angle becomes scatter direction.",
         ["opal", "cloud"],
         [{"source": "Oblique", "target": "Opal", "type": "PrismSpreadToSpray", "amount": 0.58}]),
        ("Diagonal Spray",  "spray", "Bidirectional prism-grain exchange — OBLIQUE angle shapes OPAL spray field, grain density controls bounce count.",
         ["opal", "bidirectional"],
         [{"source": "Oblique", "target": "Opal", "type": "AngleToSpray", "amount": 0.55},
          {"source": "Opal", "target": "Oblique", "type": "GrainDensityToBounceCount", "amount": 0.48}]),
    ]
    for name, tag, desc, tags, pairs in oblique_opal_specs:
        presets.append(make_preset(name, desc, ["oblique", "opal"] + [tag] + tags,
                                   ["Oblique", "Opal"], pairs,
                                   blend_dna("Oblique", "Opal"), rng=rng))

    # OBLIQUE + ORACLE
    oblique_oracle_specs = [
        ("Prophecy Bounce", "prophecy", "ORACLE breakpoints trigger OBLIQUE bounce resets — each prediction moment launches a new trajectory.",
         ["oracle", "prophecy"],
         [{"source": "Oracle", "target": "Oblique", "type": "BreakpointToBounceReset", "amount": 0.65}]),
        ("Prism Prediction","prism", "OBLIQUE prism color feeds ORACLE character — spectral tone shapes the quality of prediction.",
         ["oracle", "prism"],
         [{"source": "Oblique", "target": "Oracle", "type": "PrismColorToCharacter", "amount": 0.55}]),
        ("Spectral Oracle",  "spectral", "Bidirectional prism-oracle coupling — ORACLE movement drives bounce gravity, prism spread opens prediction space.",
         ["oracle", "bidirectional"],
         [{"source": "Oracle", "target": "Oblique", "type": "MovementToGravity", "amount": 0.58},
          {"source": "Oblique", "target": "Oracle", "type": "PrismSpreadToMovement", "amount": 0.42}]),
    ]
    for name, tag, desc, tags, pairs in oblique_oracle_specs:
        presets.append(make_preset(name, desc, ["oblique", "oracle"] + [tag] + tags,
                                   ["Oblique", "Oracle"], pairs,
                                   blend_dna("Oblique", "Oracle"), rng=rng))

    # OBLIQUE + OBLONG
    oblique_oblong_specs = [
        ("Curious Bounce",  "curious", "OBLONG curiosity drives OBLIQUE bounce rate — curious states increase bounce frequency. Inquisitiveness as kinetics.",
         ["oblong", "curious"],
         [{"source": "Oblong", "target": "Oblique", "type": "CuriosityToBounceRate", "amount": 0.60}]),
        ("Wobble Prism",    "wobble", "OBLIQUE prism spread modulates OBLONG wobble depth — prismatic dispersion becomes character instability.",
         ["oblong", "wobble"],
         [{"source": "Oblique", "target": "Oblong", "type": "PrismSpreadToWobble", "amount": 0.58}]),
        ("Bob Ricochet",    "ricochet", "OBLONG and OBLIQUE in playful exchange — Bob's curiosity bounces off the prism, prism bounce makes Bob wobble more.",
         ["oblong", "ricochet"],
         [{"source": "Oblong", "target": "Oblique", "type": "CuriosityToBounceCount", "amount": 0.55},
          {"source": "Oblique", "target": "Oblong", "type": "BounceToWobble", "amount": 0.50}]),
    ]
    for name, tag, desc, tags, pairs in oblique_oblong_specs:
        presets.append(make_preset(name, desc, ["oblique", "oblong"] + [tag] + tags,
                                   ["Oblique", "Oblong"], pairs,
                                   blend_dna("Oblique", "Oblong"), rng=rng))

    # OBLIQUE + OBBLIGATO
    oblique_obbligato_specs = [
        ("Bond Bounce",     "bond", "OBBLIGATO BOND macro tightens OBLIQUE bounce gravity — strong bond = high arc held longer.",
         ["obbligato", "bond"],
         [{"source": "Obbligato", "target": "Oblique", "type": "BondToGravity", "amount": 0.62}]),
        ("Prism Wind",      "wind", "OBLIQUE prism delay refracts OBBLIGATO's dual-voice texture — each voice lands in a different prism band.",
         ["obbligato", "wind"],
         [{"source": "Obbligato", "target": "Oblique", "type": "VoiceBalanceToPrismSpread", "amount": 0.58}]),
        ("Obligato Ricochet","ricochet", "OBLIQUE bounce click timing syncs OBBLIGATO voice 2 attack — each ricochet pulls the second voice in.",
         ["obbligato", "ricochet"],
         [{"source": "Oblique", "target": "Obbligato", "type": "BounceClickToVoice2", "amount": 0.55},
          {"source": "Obbligato", "target": "Oblique", "type": "BondToBounceSwing", "amount": 0.45}]),
    ]
    for name, tag, desc, tags, pairs in oblique_obbligato_specs:
        presets.append(make_preset(name, desc, ["oblique", "obbligato"] + [tag] + tags,
                                   ["Oblique", "Obbligato"], pairs,
                                   blend_dna("Oblique", "Obbligato"), rng=rng))

    return presets


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def parse_args():
    parser = argparse.ArgumentParser(
        description="Generate ORIGAMI / OPTIC / OBLIQUE coupling presets for XOmnibus."
    )
    repo_root = Path(__file__).resolve().parent.parent
    default_out = str(repo_root / "Presets" / "XOmnibus" / "Entangled")
    parser.add_argument(
        "--output-dir", default=default_out,
        help=f"Directory to write .xometa files (default: {default_out})"
    )
    parser.add_argument(
        "--dry-run", action="store_true",
        help="Print what would be written without creating files."
    )
    parser.add_argument(
        "--seed", type=int, default=None,
        help="Random seed for reproducible parameter generation."
    )
    return parser.parse_args()


def main():
    args = parse_args()
    rng = random.Random(args.seed)

    presets = build_all_presets(rng)

    output_dir = Path(args.output_dir)
    written = 0
    skipped = 0

    for preset in presets:
        filename = preset["name"].replace(" ", "_").replace("/", "-") + ".xometa"
        filepath = output_dir / filename

        if args.dry_run:
            print(f"[dry-run] Would write: {filepath}")
            written += 1
            continue

        output_dir.mkdir(parents=True, exist_ok=True)
        with open(filepath, "w", encoding="utf-8") as f:
            json.dump(preset, f, indent=2)
            f.write("\n")
        print(f"Wrote: {filepath}")
        written += 1

    print(f"\n{'[dry-run] ' if args.dry_run else ''}Done — {written} presets, {skipped} skipped.")
    print(f"Total: {len(presets)} presets generated.")


if __name__ == "__main__":
    main()
