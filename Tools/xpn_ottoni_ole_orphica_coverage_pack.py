#!/usr/bin/env python3
"""Generate ~72 Entangled mood coupling presets for OTTONI, OLE, and ORPHICA.

Covers 4 new partner engines for each: ORGANON, OUROBOROS, ORACLE, OBSCURA
6 presets per pair × 12 pairs = 72 total

Engine characters:
  OTTONI   — Patina green #5B8A72, triple brass, GROW macro
             DNA bias: warmth 0.5–0.8, moderate brightness
  OLE      — Hibiscus #C9377A, Afro-Latin trio, DRAMA macro
             DNA bias: aggression 0.4–0.7, warmth 0.5–0.75
  ORPHICA  — Siren Seafoam #7FDBCA, microsound harp, siphonophore
             DNA bias: brightness 0.6–0.9, space 0.5–0.8

Coupling types (12 total, rotated across presets):
  FREQUENCY_SHIFT, AMPLITUDE_MOD, FILTER_MOD, PITCH_SYNC,
  TIMBRE_BLEND, ENVELOPE_LINK, HARMONIC_FOLD, CHAOS_INJECT,
  RESONANCE_SHARE, SPATIAL_COUPLE, SPECTRAL_MORPH, VELOCITY_COUPLE
"""

import json
import os

PRESET_DIR = os.path.join(
    os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
    "Presets", "XOmnibus", "Entangled"
)

COUPLING_TYPES = [
    "FREQUENCY_SHIFT", "AMPLITUDE_MOD", "FILTER_MOD", "PITCH_SYNC",
    "TIMBRE_BLEND", "ENVELOPE_LINK", "HARMONIC_FOLD", "CHAOS_INJECT",
    "RESONANCE_SHARE", "SPATIAL_COUPLE", "SPECTRAL_MORPH", "VELOCITY_COUPLE",
]


def make_preset(name, engines, coupling_type, coupling_amount, params_a, params_b, dna, tags, desc):
    engine_a, engine_b = engines
    intensity = "Deep" if coupling_amount >= 0.7 else "Moderate"
    return {
        "schema_version": 1,
        "name": name,
        "mood": "Entangled",
        "engines": list(engines),
        "author": "XO_OX Designs",
        "version": "1.0",
        "description": desc,
        "tags": ["entangled", "coupling", "constellation"] + tags,
        "macroLabels": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
        "couplingIntensity": intensity,
        "tempo": None,
        "dna": dna,
        "parameters": {
            engine_a: params_a,
            engine_b: params_b,
        },
        "coupling": {
            "pairs": [{
                "engineA": engine_a,
                "engineB": engine_b,
                "type": coupling_type,
                "amount": round(coupling_amount, 3),
            }]
        },
        "macros": {
            "CHARACTER": round((params_a.get("macro_character", 0.5) + params_b.get("macro_character", 0.5)) / 2, 3),
            "MOVEMENT": round((params_a.get("macro_movement", 0.5) + params_b.get("macro_movement", 0.5)) / 2, 3),
            "COUPLING": round(coupling_amount, 3),
            "SPACE": round((params_a.get("macro_space", 0.5) + params_b.get("macro_space", 0.5)) / 2, 3),
        },
        "sequencer": None,
    }


def write_preset(preset):
    os.makedirs(PRESET_DIR, exist_ok=True)
    safe_name = preset["name"].replace("/", "-").replace(":", "-")
    filename = safe_name + ".xometa"
    filepath = os.path.join(PRESET_DIR, filename)
    with open(filepath, "w") as f:
        json.dump(preset, f, indent=2)
        f.write("\n")
    return filepath


# ---------------------------------------------------------------------------
# Preset definitions — 72 total
# Each block: 6 presets for one engine pair
# Coupling types rotate through all 12 across each block of 6
# ---------------------------------------------------------------------------

PRESETS = []

# ============================================================
# OTTONI × ORGANON  (6 presets)
# OTTONI: brass warmth feeds Organon's metabolic modulation
# ============================================================
OTTONI_ORGANON = [
    make_preset(
        "Brass Metabolism",
        ("OTTONI", "ORGANON"),
        "HARMONIC_FOLD", 0.72,
        {"macro_character": 0.65, "macro_movement": 0.5,  "macro_coupling": 0.72, "macro_space": 0.4},
        {"macro_character": 0.55, "macro_movement": 0.62, "macro_coupling": 0.72, "macro_space": 0.5},
        {"brightness": 0.45, "warmth": 0.78, "movement": 0.55, "density": 0.6, "space": 0.4, "aggression": 0.15},
        ["brass", "organic", "warm"],
        "Triple brass harmonics fold into Organon's metabolic rate — living breath.",
    ),
    make_preset(
        "Growing Cell",
        ("OTTONI", "ORGANON"),
        "ENVELOPE_LINK", 0.65,
        {"macro_character": 0.7,  "macro_movement": 0.45, "macro_coupling": 0.65, "macro_space": 0.35},
        {"macro_character": 0.5,  "macro_movement": 0.7,  "macro_coupling": 0.65, "macro_space": 0.55},
        {"brightness": 0.5,  "warmth": 0.72, "movement": 0.7,  "density": 0.55, "space": 0.35, "aggression": 0.1},
        ["growth", "envelope", "evolving"],
        "GROW macro links envelope shapes — brass swell triggers cellular expansion.",
    ),
    make_preset(
        "Patina Culture",
        ("OTTONI", "ORGANON"),
        "SPECTRAL_MORPH", 0.58,
        {"macro_character": 0.6,  "macro_movement": 0.5,  "macro_coupling": 0.58, "macro_space": 0.45},
        {"macro_character": 0.6,  "macro_movement": 0.55, "macro_coupling": 0.58, "macro_space": 0.6},
        {"brightness": 0.55, "warmth": 0.65, "movement": 0.5,  "density": 0.7,  "space": 0.5,  "aggression": 0.12},
        ["spectral", "texture", "layered"],
        "Oxidized brass spectral content morphs Organon's timbral palette.",
    ),
    make_preset(
        "Resonant Growth",
        ("ORGANON", "OTTONI"),
        "RESONANCE_SHARE", 0.75,
        {"macro_character": 0.55, "macro_movement": 0.6,  "macro_coupling": 0.75, "macro_space": 0.5},
        {"macro_character": 0.7,  "macro_movement": 0.4,  "macro_coupling": 0.75, "macro_space": 0.38},
        {"brightness": 0.4,  "warmth": 0.8,  "movement": 0.45, "density": 0.65, "space": 0.42, "aggression": 0.08},
        ["resonance", "deep", "brass"],
        "Organon resonance peaks shared into brass body — rich sympathetic vibration.",
    ),
    make_preset(
        "Metabolic Brass",
        ("OTTONI", "ORGANON"),
        "AMPLITUDE_MOD", 0.6,
        {"macro_character": 0.62, "macro_movement": 0.55, "macro_coupling": 0.6, "macro_space": 0.4},
        {"macro_character": 0.5,  "macro_movement": 0.65, "macro_coupling": 0.6, "macro_space": 0.5},
        {"brightness": 0.48, "warmth": 0.7,  "movement": 0.6,  "density": 0.58, "space": 0.38, "aggression": 0.1},
        ["amplitude", "breathing", "organic"],
        "Organon metabolic rhythm amplitude-modulates OTTONI brass — breathing sculpture.",
    ),
    make_preset(
        "Verde Vitality",
        ("OTTONI", "ORGANON"),
        "VELOCITY_COUPLE", 0.68,
        {"macro_character": 0.75, "macro_movement": 0.5,  "macro_coupling": 0.68, "macro_space": 0.3},
        {"macro_character": 0.52, "macro_movement": 0.72, "macro_coupling": 0.68, "macro_space": 0.52},
        {"brightness": 0.5,  "warmth": 0.74, "movement": 0.65, "density": 0.6,  "space": 0.3,  "aggression": 0.06},
        ["velocity", "expressive", "brass"],
        "Velocity of OTTONI attack drives Organon's vitality parameter — touch-sensitive life.",
    ),
]
PRESETS.extend(OTTONI_ORGANON)


# ============================================================
# OTTONI × OUROBOROS  (6 presets)
# ============================================================
OTTONI_OUROBOROS = [
    make_preset(
        "Brass Serpent",
        ("OTTONI", "OUROBOROS"),
        "CHAOS_INJECT", 0.7,
        {"macro_character": 0.65, "macro_movement": 0.5,  "macro_coupling": 0.7,  "macro_space": 0.38},
        {"macro_character": 0.6,  "macro_movement": 0.7,  "macro_coupling": 0.7,  "macro_space": 0.45},
        {"brightness": 0.45, "warmth": 0.72, "movement": 0.7,  "density": 0.6,  "space": 0.38, "aggression": 0.55},
        ["chaos", "serpent", "tension"],
        "Chaos from the ouroboros bites into OTTONI brass — unpredictable fanfare.",
    ),
    make_preset(
        "Cyclic Patina",
        ("OUROBOROS", "OTTONI"),
        "PITCH_SYNC", 0.62,
        {"macro_character": 0.55, "macro_movement": 0.65, "macro_coupling": 0.62, "macro_space": 0.42},
        {"macro_character": 0.7,  "macro_movement": 0.45, "macro_coupling": 0.62, "macro_space": 0.35},
        {"brightness": 0.5,  "warmth": 0.68, "movement": 0.6,  "density": 0.55, "space": 0.4,  "aggression": 0.45},
        ["pitch", "cycle", "drone"],
        "Ouroboros pitch topology locks OTTONI intervals — serpentine brass loops.",
    ),
    make_preset(
        "Strange Forge",
        ("OTTONI", "OUROBOROS"),
        "FILTER_MOD", 0.78,
        {"macro_character": 0.6,  "macro_movement": 0.55, "macro_coupling": 0.78, "macro_space": 0.35},
        {"macro_character": 0.65, "macro_movement": 0.75, "macro_coupling": 0.78, "macro_space": 0.4},
        {"brightness": 0.42, "warmth": 0.75, "movement": 0.75, "density": 0.65, "space": 0.32, "aggression": 0.62},
        ["filter", "strange", "intense"],
        "Strange attractor filter events reshape OTTONI brass formants — chaotic timbre.",
    ),
    make_preset(
        "Eternal Return",
        ("OUROBOROS", "OTTONI"),
        "FREQUENCY_SHIFT", 0.55,
        {"macro_character": 0.5,  "macro_movement": 0.7,  "macro_coupling": 0.55, "macro_space": 0.45},
        {"macro_character": 0.68, "macro_movement": 0.48, "macro_coupling": 0.55, "macro_space": 0.38},
        {"brightness": 0.46, "warmth": 0.65, "movement": 0.65, "density": 0.5,  "space": 0.42, "aggression": 0.5},
        ["eternal", "brass", "return"],
        "OUROBOROS frequency topology shifts OTTONI into self-referential brass cycles.",
    ),
    make_preset(
        "Grow And Devour",
        ("OTTONI", "OUROBOROS"),
        "TIMBRE_BLEND", 0.66,
        {"macro_character": 0.72, "macro_movement": 0.5,  "macro_coupling": 0.66, "macro_space": 0.35},
        {"macro_character": 0.58, "macro_movement": 0.68, "macro_coupling": 0.66, "macro_space": 0.44},
        {"brightness": 0.44, "warmth": 0.7,  "movement": 0.6,  "density": 0.62, "space": 0.35, "aggression": 0.58},
        ["growth", "devour", "drama"],
        "GROW macro blends timbre into Ouroboros's voracious topology — eat the brass.",
    ),
    make_preset(
        "Leashed Fanfare",
        ("OUROBOROS", "OTTONI"),
        "ENVELOPE_LINK", 0.73,
        {"macro_character": 0.6,  "macro_movement": 0.72, "macro_coupling": 0.73, "macro_space": 0.42},
        {"macro_character": 0.65, "macro_movement": 0.44, "macro_coupling": 0.73, "macro_space": 0.32},
        {"brightness": 0.48, "warmth": 0.73, "movement": 0.68, "density": 0.6,  "space": 0.35, "aggression": 0.6},
        ["leash", "envelope", "fanfare"],
        "Ouroboros's leash mechanism controls OTTONI attack — controlled chaotic brass.",
    ),
]
PRESETS.extend(OTTONI_OUROBOROS)


# ============================================================
# OTTONI × ORACLE  (6 presets)
# ============================================================
OTTONI_ORACLE = [
    make_preset(
        "Prophecy Brass",
        ("OTTONI", "ORACLE"),
        "SPECTRAL_MORPH", 0.67,
        {"macro_character": 0.65, "macro_movement": 0.5,  "macro_coupling": 0.67, "macro_space": 0.42},
        {"macro_character": 0.58, "macro_movement": 0.62, "macro_coupling": 0.67, "macro_space": 0.55},
        {"brightness": 0.5,  "warmth": 0.7,  "movement": 0.55, "density": 0.58, "space": 0.48, "aggression": 0.18},
        ["prophecy", "spectral", "brass"],
        "OTTONI harmonics feed Oracle's breakpoint system — brass foretells timbre.",
    ),
    make_preset(
        "Stochastic Forge",
        ("ORACLE", "OTTONI"),
        "AMPLITUDE_MOD", 0.6,
        {"macro_character": 0.52, "macro_movement": 0.68, "macro_coupling": 0.6,  "macro_space": 0.55},
        {"macro_character": 0.68, "macro_movement": 0.45, "macro_coupling": 0.6,  "macro_space": 0.35},
        {"brightness": 0.48, "warmth": 0.68, "movement": 0.6,  "density": 0.55, "space": 0.45, "aggression": 0.22},
        ["stochastic", "maqam", "brass"],
        "Oracle's GENDY stochastic events amplitude-shape OTTONI attack — chance forging.",
    ),
    make_preset(
        "Indigo Patina",
        ("OTTONI", "ORACLE"),
        "HARMONIC_FOLD", 0.71,
        {"macro_character": 0.7,  "macro_movement": 0.48, "macro_coupling": 0.71, "macro_space": 0.38},
        {"macro_character": 0.55, "macro_movement": 0.65, "macro_coupling": 0.71, "macro_space": 0.58},
        {"brightness": 0.52, "warmth": 0.72, "movement": 0.5,  "density": 0.65, "space": 0.5,  "aggression": 0.12},
        ["indigo", "harmonic", "deep"],
        "OTTONI triple-brass folds into Oracle's indigo harmonic space — ancient metals.",
    ),
    make_preset(
        "Breakpoint Grow",
        ("ORACLE", "OTTONI"),
        "VELOCITY_COUPLE", 0.65,
        {"macro_character": 0.5,  "macro_movement": 0.7,  "macro_coupling": 0.65, "macro_space": 0.55},
        {"macro_character": 0.72, "macro_movement": 0.42, "macro_coupling": 0.65, "macro_space": 0.32},
        {"brightness": 0.5,  "warmth": 0.68, "movement": 0.62, "density": 0.58, "space": 0.42, "aggression": 0.15},
        ["breakpoint", "velocity", "growth"],
        "Oracle breakpoints velocity-coupled to GROW — dynamic oracle reveals its brass.",
    ),
    make_preset(
        "Maqam Brass",
        ("OTTONI", "ORACLE"),
        "PITCH_SYNC", 0.58,
        {"macro_character": 0.62, "macro_movement": 0.52, "macro_coupling": 0.58, "macro_space": 0.4},
        {"macro_character": 0.58, "macro_movement": 0.6,  "macro_coupling": 0.58, "macro_space": 0.58},
        {"brightness": 0.55, "warmth": 0.65, "movement": 0.52, "density": 0.55, "space": 0.52, "aggression": 0.2},
        ["maqam", "tuning", "brass"],
        "Oracle's Maqam scale system pitch-syncs OTTONI intervals — microtonal brass.",
    ),
    make_preset(
        "Verdant Oracle",
        ("ORACLE", "OTTONI"),
        "RESONANCE_SHARE", 0.76,
        {"macro_character": 0.55, "macro_movement": 0.65, "macro_coupling": 0.76, "macro_space": 0.58},
        {"macro_character": 0.68, "macro_movement": 0.44, "macro_coupling": 0.76, "macro_space": 0.35},
        {"brightness": 0.48, "warmth": 0.76, "movement": 0.55, "density": 0.7,  "space": 0.45, "aggression": 0.1},
        ["resonance", "deep", "verdant"],
        "Oracle resonance peaks shared into OTTONI body — prophecy in patina green.",
    ),
]
PRESETS.extend(OTTONI_ORACLE)


# ============================================================
# OTTONI × OBSCURA  (6 presets)
# ============================================================
OTTONI_OBSCURA = [
    make_preset(
        "Silver Patina",
        ("OTTONI", "OBSCURA"),
        "TIMBRE_BLEND", 0.62,
        {"macro_character": 0.65, "macro_movement": 0.5,  "macro_coupling": 0.62, "macro_space": 0.42},
        {"macro_character": 0.55, "macro_movement": 0.58, "macro_coupling": 0.62, "macro_space": 0.55},
        {"brightness": 0.5,  "warmth": 0.68, "movement": 0.5,  "density": 0.6,  "space": 0.5,  "aggression": 0.14},
        ["silver", "patina", "blend"],
        "Daguerreotype silver and patina green blend — oxidized beauty.",
    ),
    make_preset(
        "Corroded Plate",
        ("OBSCURA", "OTTONI"),
        "FILTER_MOD", 0.68,
        {"macro_character": 0.55, "macro_movement": 0.62, "macro_coupling": 0.68, "macro_space": 0.55},
        {"macro_character": 0.65, "macro_movement": 0.46, "macro_coupling": 0.68, "macro_space": 0.35},
        {"brightness": 0.42, "warmth": 0.72, "movement": 0.55, "density": 0.65, "space": 0.48, "aggression": 0.18},
        ["corroded", "filter", "texture"],
        "Obscura stiffness filter-modulates OTTONI harmonic brilliance — tarnished brass.",
    ),
    make_preset(
        "Old Photograph",
        ("OTTONI", "OBSCURA"),
        "SPATIAL_COUPLE", 0.7,
        {"macro_character": 0.62, "macro_movement": 0.48, "macro_coupling": 0.7,  "macro_space": 0.45},
        {"macro_character": 0.52, "macro_movement": 0.6,  "macro_coupling": 0.7,  "macro_space": 0.72},
        {"brightness": 0.38, "warmth": 0.65, "movement": 0.48, "density": 0.55, "space": 0.72, "aggression": 0.1},
        ["spatial", "photo", "memory"],
        "OTTONI brass fades into Obscura's spatial depth — antique brass photograph.",
    ),
    make_preset(
        "String Stiffness",
        ("OBSCURA", "OTTONI"),
        "FREQUENCY_SHIFT", 0.55,
        {"macro_character": 0.5,  "macro_movement": 0.65, "macro_coupling": 0.55, "macro_space": 0.58},
        {"macro_character": 0.68, "macro_movement": 0.45, "macro_coupling": 0.55, "macro_space": 0.38},
        {"brightness": 0.45, "warmth": 0.66, "movement": 0.58, "density": 0.55, "space": 0.5,  "aggression": 0.15},
        ["frequency", "stiffness", "shift"],
        "Obscura string stiffness frequency-shifts OTTONI partials — warped brass.",
    ),
    make_preset(
        "Grow Through Film",
        ("OTTONI", "OBSCURA"),
        "CHAOS_INJECT", 0.63,
        {"macro_character": 0.7,  "macro_movement": 0.52, "macro_coupling": 0.63, "macro_space": 0.38},
        {"macro_character": 0.52, "macro_movement": 0.62, "macro_coupling": 0.63, "macro_space": 0.6},
        {"brightness": 0.88, "warmth": 0.6,  "movement": 0.58, "density": 0.5,  "space": 0.55, "aggression": 0.22},
        ["chaos", "bright", "emerge"],
        "GROW macro injects chaos into Obscura's silver film — brilliant emergence.",
    ),
    make_preset(
        "Daguerreotype Brass",
        ("OBSCURA", "OTTONI"),
        "ENVELOPE_LINK", 0.72,
        {"macro_character": 0.55, "macro_movement": 0.6,  "macro_coupling": 0.72, "macro_space": 0.62},
        {"macro_character": 0.66, "macro_movement": 0.44, "macro_coupling": 0.72, "macro_space": 0.35},
        {"brightness": 0.44, "warmth": 0.75, "movement": 0.52, "density": 0.62, "space": 0.58, "aggression": 0.1},
        ["envelope", "daguerreotype", "warm"],
        "Obscura envelope contour linked to OTTONI attack — silver-coated fanfare.",
    ),
]
PRESETS.extend(OTTONI_OBSCURA)


# ============================================================
# OLE × ORGANON  (6 presets)
# ============================================================
OLE_ORGANON = [
    make_preset(
        "Drama Metabolism",
        ("OLE", "ORGANON"),
        "AMPLITUDE_MOD", 0.7,
        {"macro_character": 0.68, "macro_movement": 0.62, "macro_coupling": 0.7,  "macro_space": 0.4},
        {"macro_character": 0.55, "macro_movement": 0.7,  "macro_coupling": 0.7,  "macro_space": 0.52},
        {"brightness": 0.58, "warmth": 0.72, "movement": 0.7,  "density": 0.62, "space": 0.4,  "aggression": 0.65},
        ["drama", "organic", "latin"],
        "OLE DRAMA macro amplitude-modulates Organon's metabolic rate — life force crescendo.",
    ),
    make_preset(
        "Cumbia Cell",
        ("ORGANON", "OLE"),
        "VELOCITY_COUPLE", 0.65,
        {"macro_character": 0.52, "macro_movement": 0.72, "macro_coupling": 0.65, "macro_space": 0.5},
        {"macro_character": 0.7,  "macro_movement": 0.6,  "macro_coupling": 0.65, "macro_space": 0.38},
        {"brightness": 0.55, "warmth": 0.68, "movement": 0.75, "density": 0.6,  "space": 0.38, "aggression": 0.58},
        ["cumbia", "velocity", "percussive"],
        "Organon velocity output fires OLE Afro-Latin rhythm — biological cumbia.",
    ),
    make_preset(
        "Hibiscus Growth",
        ("OLE", "ORGANON"),
        "HARMONIC_FOLD", 0.68,
        {"macro_character": 0.72, "macro_movement": 0.6,  "macro_coupling": 0.68, "macro_space": 0.38},
        {"macro_character": 0.55, "macro_movement": 0.68, "macro_coupling": 0.68, "macro_space": 0.55},
        {"brightness": 0.6,  "warmth": 0.72, "movement": 0.65, "density": 0.6,  "space": 0.42, "aggression": 0.6},
        ["hibiscus", "harmonic", "bloom"],
        "OLE harmonic bloom folds into Organon's metabolic expansion — flowers multiply.",
    ),
    make_preset(
        "Variational Drama",
        ("ORGANON", "OLE"),
        "SPECTRAL_MORPH", 0.72,
        {"macro_character": 0.58, "macro_movement": 0.68, "macro_coupling": 0.72, "macro_space": 0.52},
        {"macro_character": 0.68, "macro_movement": 0.62, "macro_coupling": 0.72, "macro_space": 0.36},
        {"brightness": 0.55, "warmth": 0.7,  "movement": 0.72, "density": 0.62, "space": 0.4,  "aggression": 0.68},
        ["variational", "spectral", "vivid"],
        "Organon's free-energy inference spectrally morphs OLE's timbral drama — lived theatre.",
    ),
    make_preset(
        "Salsa Resonance",
        ("OLE", "ORGANON"),
        "RESONANCE_SHARE", 0.6,
        {"macro_character": 0.65, "macro_movement": 0.65, "macro_coupling": 0.6,  "macro_space": 0.4},
        {"macro_character": 0.52, "macro_movement": 0.62, "macro_coupling": 0.6,  "macro_space": 0.52},
        {"brightness": 0.6,  "warmth": 0.7,  "movement": 0.68, "density": 0.58, "space": 0.4,  "aggression": 0.6},
        ["salsa", "resonance", "shared"],
        "OLE-ORGANON resonance peaks dance together — living salsa rhythm.",
    ),
    make_preset(
        "Latin Enzyme",
        ("OLE", "ORGANON"),
        "FILTER_MOD", 0.75,
        {"macro_character": 0.7,  "macro_movement": 0.68, "macro_coupling": 0.75, "macro_space": 0.35},
        {"macro_character": 0.55, "macro_movement": 0.72, "macro_coupling": 0.75, "macro_space": 0.5},
        {"brightness": 0.58, "warmth": 0.68, "movement": 0.75, "density": 0.65, "space": 0.35, "aggression": 0.7},
        ["filter", "enzyme", "drive"],
        "Organon enzyme kinetics filter-modulate OLE's brightness — bio-rhythmic drama.",
    ),
]
PRESETS.extend(OLE_ORGANON)


# ============================================================
# OLE × OUROBOROS  (6 presets)
# ============================================================
OLE_OUROBOROS = [
    make_preset(
        "Serpent Dance",
        ("OLE", "OUROBOROS"),
        "CHAOS_INJECT", 0.72,
        {"macro_character": 0.7,  "macro_movement": 0.68, "macro_coupling": 0.72, "macro_space": 0.38},
        {"macro_character": 0.62, "macro_movement": 0.75, "macro_coupling": 0.72, "macro_space": 0.42},
        {"brightness": 0.58, "warmth": 0.68, "movement": 0.82, "density": 0.6,  "space": 0.38, "aggression": 0.75},
        ["chaos", "dance", "serpent"],
        "OUROBOROS chaos injected into OLE rhythm — unpredictable Afro-Latin serpent.",
    ),
    make_preset(
        "Cyclical Drama",
        ("OUROBOROS", "OLE"),
        "PITCH_SYNC", 0.65,
        {"macro_character": 0.6,  "macro_movement": 0.72, "macro_coupling": 0.65, "macro_space": 0.44},
        {"macro_character": 0.7,  "macro_movement": 0.62, "macro_coupling": 0.65, "macro_space": 0.36},
        {"brightness": 0.55, "warmth": 0.7,  "movement": 0.72, "density": 0.58, "space": 0.4,  "aggression": 0.68},
        ["cycle", "pitch", "drama"],
        "Ouroboros cyclic pitch syncs OLE intervals — drama returns to the beginning.",
    ),
    make_preset(
        "Red Hibiscus",
        ("OLE", "OUROBOROS"),
        "FREQUENCY_SHIFT", 0.58,
        {"macro_character": 0.68, "macro_movement": 0.65, "macro_coupling": 0.58, "macro_space": 0.38},
        {"macro_character": 0.6,  "macro_movement": 0.72, "macro_coupling": 0.58, "macro_space": 0.44},
        {"brightness": 0.6,  "warmth": 0.72, "movement": 0.68, "density": 0.55, "space": 0.4,  "aggression": 0.65},
        ["frequency", "red", "shift"],
        "OLE warmth frequency-shifts into strange attractor territory — red hibiscus chaos.",
    ),
    make_preset(
        "Leash The Drama",
        ("OUROBOROS", "OLE"),
        "ENVELOPE_LINK", 0.78,
        {"macro_character": 0.62, "macro_movement": 0.75, "macro_coupling": 0.78, "macro_space": 0.42},
        {"macro_character": 0.72, "macro_movement": 0.6,  "macro_coupling": 0.78, "macro_space": 0.35},
        {"brightness": 0.55, "warmth": 0.7,  "movement": 0.75, "density": 0.65, "space": 0.38, "aggression": 0.72},
        ["leash", "envelope", "control"],
        "Ouroboros leash envelope linked to OLE — chaos harnessed into dramatic performance.",
    ),
    make_preset(
        "Strange Clave",
        ("OLE", "OUROBOROS"),
        "TIMBRE_BLEND", 0.65,
        {"macro_character": 0.72, "macro_movement": 0.7,  "macro_coupling": 0.65, "macro_space": 0.35},
        {"macro_character": 0.6,  "macro_movement": 0.72, "macro_coupling": 0.65, "macro_space": 0.45},
        {"brightness": 0.56, "warmth": 0.65, "movement": 0.78, "density": 0.62, "space": 0.38, "aggression": 0.7},
        ["timbre", "clave", "strange"],
        "OLE clave pattern timbres blend with strange attractor state — topological rhythm.",
    ),
    make_preset(
        "Afro Attractor",
        ("OUROBOROS", "OLE"),
        "AMPLITUDE_MOD", 0.7,
        {"macro_character": 0.58, "macro_movement": 0.75, "macro_coupling": 0.7,  "macro_space": 0.44},
        {"macro_character": 0.72, "macro_movement": 0.62, "macro_coupling": 0.7,  "macro_space": 0.36},
        {"brightness": 0.52, "warmth": 0.72, "movement": 0.75, "density": 0.65, "space": 0.38, "aggression": 0.72},
        ["afro", "attractor", "amplitude"],
        "Strange attractor amplitude pattern drives OLE Afro-Latin density — orbital drama.",
    ),
]
PRESETS.extend(OLE_OUROBOROS)


# ============================================================
# OLE × ORACLE  (6 presets)
# ============================================================
OLE_ORACLE = [
    make_preset(
        "Oracle Drama",
        ("OLE", "ORACLE"),
        "SPECTRAL_MORPH", 0.68,
        {"macro_character": 0.72, "macro_movement": 0.62, "macro_coupling": 0.68, "macro_space": 0.38},
        {"macro_character": 0.56, "macro_movement": 0.65, "macro_coupling": 0.68, "macro_space": 0.58},
        {"brightness": 0.58, "warmth": 0.7,  "movement": 0.65, "density": 0.6,  "space": 0.5,  "aggression": 0.62},
        ["spectral", "prophecy", "drama"],
        "OLE drama spectrally morphs Oracle's vision — the dancer speaks prophecy.",
    ),
    make_preset(
        "Stochastic Fiesta",
        ("ORACLE", "OLE"),
        "CHAOS_INJECT", 0.65,
        {"macro_character": 0.55, "macro_movement": 0.72, "macro_coupling": 0.65, "macro_space": 0.55},
        {"macro_character": 0.7,  "macro_movement": 0.65, "macro_coupling": 0.65, "macro_space": 0.36},
        {"brightness": 0.58, "warmth": 0.68, "movement": 0.72, "density": 0.6,  "space": 0.42, "aggression": 0.68},
        ["stochastic", "fiesta", "chaos"],
        "GENDY stochastic chaos injected into OLE — unpredictable celebration.",
    ),
    make_preset(
        "Maqam Carnival",
        ("OLE", "ORACLE"),
        "PITCH_SYNC", 0.62,
        {"macro_character": 0.68, "macro_movement": 0.65, "macro_coupling": 0.62, "macro_space": 0.38},
        {"macro_character": 0.55, "macro_movement": 0.62, "macro_coupling": 0.62, "macro_space": 0.58},
        {"brightness": 0.62, "warmth": 0.7,  "movement": 0.65, "density": 0.55, "space": 0.5,  "aggression": 0.6},
        ["maqam", "carnival", "tuning"],
        "Oracle Maqam tuning pitch-syncs OLE scales — microtonal carnival.",
    ),
    make_preset(
        "Breakpoint Clave",
        ("ORACLE", "OLE"),
        "VELOCITY_COUPLE", 0.72,
        {"macro_character": 0.55, "macro_movement": 0.7,  "macro_coupling": 0.72, "macro_space": 0.55},
        {"macro_character": 0.72, "macro_movement": 0.62, "macro_coupling": 0.72, "macro_space": 0.35},
        {"brightness": 0.55, "warmth": 0.68, "movement": 0.72, "density": 0.62, "space": 0.4,  "aggression": 0.7},
        ["breakpoint", "clave", "velocity"],
        "Oracle breakpoints velocity-drive OLE clave accents — prophetic rhythm.",
    ),
    make_preset(
        "Indigo Hibiscus",
        ("OLE", "ORACLE"),
        "HARMONIC_FOLD", 0.7,
        {"macro_character": 0.72, "macro_movement": 0.6,  "macro_coupling": 0.7,  "macro_space": 0.36},
        {"macro_character": 0.55, "macro_movement": 0.68, "macro_coupling": 0.7,  "macro_space": 0.62},
        {"brightness": 0.6,  "warmth": 0.68, "movement": 0.65, "density": 0.65, "space": 0.55, "aggression": 0.62},
        ["indigo", "hibiscus", "deep"],
        "OLE hibiscus harmonics fold into Oracle's indigo depth — mystic tropics.",
    ),
    make_preset(
        "Dramatic Foresight",
        ("ORACLE", "OLE"),
        "RESONANCE_SHARE", 0.65,
        {"macro_character": 0.55, "macro_movement": 0.65, "macro_coupling": 0.65, "macro_space": 0.58},
        {"macro_character": 0.7,  "macro_movement": 0.62, "macro_coupling": 0.65, "macro_space": 0.36},
        {"brightness": 0.56, "warmth": 0.7,  "movement": 0.65, "density": 0.6,  "space": 0.48, "aggression": 0.65},
        ["resonance", "foresight", "drama"],
        "Oracle resonance peaks shared with OLE — prophecy resonates in hibiscus red.",
    ),
]
PRESETS.extend(OLE_ORACLE)


# ============================================================
# OLE × OBSCURA  (6 presets)
# ============================================================
OLE_OBSCURA = [
    make_preset(
        "Hidden Drama",
        ("OLE", "OBSCURA"),
        "SPATIAL_COUPLE", 0.68,
        {"macro_character": 0.7,  "macro_movement": 0.62, "macro_coupling": 0.68, "macro_space": 0.4},
        {"macro_character": 0.52, "macro_movement": 0.58, "macro_coupling": 0.68, "macro_space": 0.72},
        {"brightness": 0.55, "warmth": 0.68, "movement": 0.62, "density": 0.55, "space": 0.7,  "aggression": 0.58},
        ["spatial", "hidden", "drama"],
        "OLE drama spatially coupled into Obscura's depth — the dancer disappears.",
    ),
    make_preset(
        "Silver Clave",
        ("OBSCURA", "OLE"),
        "FILTER_MOD", 0.7,
        {"macro_character": 0.52, "macro_movement": 0.62, "macro_coupling": 0.7,  "macro_space": 0.62},
        {"macro_character": 0.7,  "macro_movement": 0.65, "macro_coupling": 0.7,  "macro_space": 0.36},
        {"brightness": 0.5,  "warmth": 0.68, "movement": 0.65, "density": 0.6,  "space": 0.55, "aggression": 0.62},
        ["silver", "clave", "filter"],
        "Obscura stiffness filter-modulates OLE's clave brightness — tarnished rhythm.",
    ),
    make_preset(
        "Daguerreotype Dance",
        ("OLE", "OBSCURA"),
        "TIMBRE_BLEND", 0.6,
        {"macro_character": 0.68, "macro_movement": 0.65, "macro_coupling": 0.6,  "macro_space": 0.38},
        {"macro_character": 0.52, "macro_movement": 0.58, "macro_coupling": 0.6,  "macro_space": 0.68},
        {"brightness": 0.45, "warmth": 0.65, "movement": 0.62, "density": 0.55, "space": 0.62, "aggression": 0.55},
        ["daguerreotype", "dance", "vintage"],
        "OLE vibrant timbre blended with Obscura's silver grain — vintage dance floor.",
    ),
    make_preset(
        "Corroded Fiesta",
        ("OBSCURA", "OLE"),
        "FREQUENCY_SHIFT", 0.62,
        {"macro_character": 0.5,  "macro_movement": 0.62, "macro_coupling": 0.62, "macro_space": 0.65},
        {"macro_character": 0.7,  "macro_movement": 0.65, "macro_coupling": 0.62, "macro_space": 0.35},
        {"brightness": 0.48, "warmth": 0.68, "movement": 0.65, "density": 0.58, "space": 0.58, "aggression": 0.6},
        ["corroded", "frequency", "fiesta"],
        "Obscura string corrosion frequency-shifts OLE tones — weathered celebration.",
    ),
    make_preset(
        "Drama In Silver",
        ("OLE", "OBSCURA"),
        "ENVELOPE_LINK", 0.72,
        {"macro_character": 0.72, "macro_movement": 0.65, "macro_coupling": 0.72, "macro_space": 0.35},
        {"macro_character": 0.52, "macro_movement": 0.58, "macro_coupling": 0.72, "macro_space": 0.68},
        {"brightness": 0.52, "warmth": 0.7,  "movement": 0.65, "density": 0.6,  "space": 0.62, "aggression": 0.65},
        ["envelope", "drama", "silver"],
        "OLE envelope drama linked into Obscura's decay — passionate slow fade.",
    ),
    make_preset(
        "Chromogenic Drama",
        ("OBSCURA", "OLE"),
        "CHAOS_INJECT", 0.65,
        {"macro_character": 0.52, "macro_movement": 0.62, "macro_coupling": 0.65, "macro_space": 0.65},
        {"macro_character": 0.72, "macro_movement": 0.68, "macro_coupling": 0.65, "macro_space": 0.35},
        {"brightness": 0.9,  "warmth": 0.65, "movement": 0.68, "density": 0.58, "space": 0.55, "aggression": 0.65},
        ["chromogenic", "chaos", "bright"],
        "Obscura photographic chaos injected into OLE — exposure reveals drama.",
    ),
]
PRESETS.extend(OLE_OBSCURA)


# ============================================================
# ORPHICA × ORGANON  (6 presets)
# ============================================================
ORPHICA_ORGANON = [
    make_preset(
        "Harp Metabolism",
        ("ORPHICA", "ORGANON"),
        "SPECTRAL_MORPH", 0.68,
        {"macro_character": 0.6,  "macro_movement": 0.55, "macro_coupling": 0.68, "macro_space": 0.68},
        {"macro_character": 0.55, "macro_movement": 0.7,  "macro_coupling": 0.68, "macro_space": 0.52},
        {"brightness": 0.78, "warmth": 0.48, "movement": 0.6,  "density": 0.45, "space": 0.72, "aggression": 0.08},
        ["harp", "spectral", "organic"],
        "ORPHICA microsound harp spectrally morphs Organon's metabolic palette — bioluminescent life.",
    ),
    make_preset(
        "Siphonophore Cell",
        ("ORGANON", "ORPHICA"),
        "ENVELOPE_LINK", 0.65,
        {"macro_character": 0.52, "macro_movement": 0.72, "macro_coupling": 0.65, "macro_space": 0.55},
        {"macro_character": 0.62, "macro_movement": 0.55, "macro_coupling": 0.65, "macro_space": 0.72},
        {"brightness": 0.8,  "warmth": 0.45, "movement": 0.65, "density": 0.42, "space": 0.75, "aggression": 0.06},
        ["siphonophore", "envelope", "marine"],
        "Organon cellular envelope linked to ORPHICA — colony modulates harp breath.",
    ),
    make_preset(
        "Bioluminescent Harp",
        ("ORPHICA", "ORGANON"),
        "RESONANCE_SHARE", 0.72,
        {"macro_character": 0.65, "macro_movement": 0.52, "macro_coupling": 0.72, "macro_space": 0.75},
        {"macro_character": 0.55, "macro_movement": 0.68, "macro_coupling": 0.72, "macro_space": 0.5},
        {"brightness": 0.85, "warmth": 0.42, "movement": 0.55, "density": 0.4,  "space": 0.78, "aggression": 0.05},
        ["bioluminescent", "resonance", "harp"],
        "ORPHICA resonance peaks shared with Organon's metabolic nodes — underwater glow.",
    ),
    make_preset(
        "Variational Pluck",
        ("ORGANON", "ORPHICA"),
        "VELOCITY_COUPLE", 0.62,
        {"macro_character": 0.5,  "macro_movement": 0.7,  "macro_coupling": 0.62, "macro_space": 0.55},
        {"macro_character": 0.65, "macro_movement": 0.5,  "macro_coupling": 0.62, "macro_space": 0.73},
        {"brightness": 0.75, "warmth": 0.45, "movement": 0.62, "density": 0.42, "space": 0.72, "aggression": 0.07},
        ["variational", "pluck", "velocity"],
        "Organon variational energy velocity-couples ORPHICA pluck intensity — adaptive harp.",
    ),
    make_preset(
        "Metabolic Mist",
        ("ORPHICA", "ORGANON"),
        "SPATIAL_COUPLE", 0.7,
        {"macro_character": 0.62, "macro_movement": 0.5,  "macro_coupling": 0.7,  "macro_space": 0.8},
        {"macro_character": 0.52, "macro_movement": 0.65, "macro_coupling": 0.7,  "macro_space": 0.52},
        {"brightness": 0.82, "warmth": 0.44, "movement": 0.55, "density": 0.38, "space": 0.82, "aggression": 0.05},
        ["spatial", "mist", "ethereal"],
        "ORPHICA microsound spatially coupled with Organon's diffusion — sea mist harp.",
    ),
    make_preset(
        "Colony Harp",
        ("ORGANON", "ORPHICA"),
        "HARMONIC_FOLD", 0.65,
        {"macro_character": 0.55, "macro_movement": 0.68, "macro_coupling": 0.65, "macro_space": 0.55},
        {"macro_character": 0.65, "macro_movement": 0.52, "macro_coupling": 0.65, "macro_space": 0.75},
        {"brightness": 0.8,  "warmth": 0.45, "movement": 0.58, "density": 0.45, "space": 0.75, "aggression": 0.06},
        ["colony", "harmonic", "fold"],
        "Organon colony harmonics fold into ORPHICA's harp strings — communal instrument.",
    ),
]
PRESETS.extend(ORPHICA_ORGANON)


# ============================================================
# ORPHICA × OUROBOROS  (6 presets)
# ============================================================
ORPHICA_OUROBOROS = [
    make_preset(
        "Serpent Harp",
        ("ORPHICA", "OUROBOROS"),
        "CHAOS_INJECT", 0.62,
        {"macro_character": 0.6,  "macro_movement": 0.55, "macro_coupling": 0.62, "macro_space": 0.72},
        {"macro_character": 0.62, "macro_movement": 0.72, "macro_coupling": 0.62, "macro_space": 0.42},
        {"brightness": 0.75, "warmth": 0.42, "movement": 0.68, "density": 0.45, "space": 0.7,  "aggression": 0.45},
        ["serpent", "chaos", "harp"],
        "OUROBOROS chaos injected into ORPHICA microsound — serpentine harp dissolution.",
    ),
    make_preset(
        "Cyclic Seafoam",
        ("OUROBOROS", "ORPHICA"),
        "FREQUENCY_SHIFT", 0.58,
        {"macro_character": 0.58, "macro_movement": 0.7,  "macro_coupling": 0.58, "macro_space": 0.44},
        {"macro_character": 0.65, "macro_movement": 0.5,  "macro_coupling": 0.58, "macro_space": 0.75},
        {"brightness": 0.78, "warmth": 0.4,  "movement": 0.65, "density": 0.42, "space": 0.73, "aggression": 0.4},
        ["cycle", "seafoam", "frequency"],
        "Ouroboros cyclic topology frequency-shifts ORPHICA strings — eternal pluck.",
    ),
    make_preset(
        "Strange Siphon",
        ("ORPHICA", "OUROBOROS"),
        "PITCH_SYNC", 0.68,
        {"macro_character": 0.65, "macro_movement": 0.52, "macro_coupling": 0.68, "macro_space": 0.75},
        {"macro_character": 0.6,  "macro_movement": 0.72, "macro_coupling": 0.68, "macro_space": 0.42},
        {"brightness": 0.72, "warmth": 0.42, "movement": 0.7,  "density": 0.48, "space": 0.72, "aggression": 0.48},
        ["strange", "siphon", "pitch"],
        "ORPHICA siphonophore pitch-synced to strange attractor — colony orbits chaos.",
    ),
    make_preset(
        "Leashed Pluck",
        ("OUROBOROS", "ORPHICA"),
        "ENVELOPE_LINK", 0.75,
        {"macro_character": 0.6,  "macro_movement": 0.75, "macro_coupling": 0.75, "macro_space": 0.42},
        {"macro_character": 0.65, "macro_movement": 0.48, "macro_coupling": 0.75, "macro_space": 0.78},
        {"brightness": 0.76, "warmth": 0.4,  "movement": 0.68, "density": 0.44, "space": 0.75, "aggression": 0.42},
        ["leash", "pluck", "envelope"],
        "Ouroboros leash envelope shape linked to ORPHICA pluck decay — controlled chaos harp.",
    ),
    make_preset(
        "Attractor Mist",
        ("ORPHICA", "OUROBOROS"),
        "TIMBRE_BLEND", 0.6,
        {"macro_character": 0.62, "macro_movement": 0.52, "macro_coupling": 0.6,  "macro_space": 0.78},
        {"macro_character": 0.6,  "macro_movement": 0.7,  "macro_coupling": 0.6,  "macro_space": 0.42},
        {"brightness": 0.7,  "warmth": 0.42, "movement": 0.65, "density": 0.45, "space": 0.76, "aggression": 0.42},
        ["attractor", "mist", "blend"],
        "ORPHICA microsound timbre blends into strange attractor state — ethereal topology.",
    ),
    make_preset(
        "Velocity Siren",
        ("OUROBOROS", "ORPHICA"),
        "VELOCITY_COUPLE", 0.7,
        {"macro_character": 0.58, "macro_movement": 0.72, "macro_coupling": 0.7,  "macro_space": 0.44},
        {"macro_character": 0.68, "macro_movement": 0.5,  "macro_coupling": 0.7,  "macro_space": 0.76},
        {"brightness": 0.8,  "warmth": 0.4,  "movement": 0.68, "density": 0.42, "space": 0.74, "aggression": 0.44},
        ["velocity", "siren", "harp"],
        "Ouroboros velocity output drives ORPHICA harp intensity — siren song emerges.",
    ),
]
PRESETS.extend(ORPHICA_OUROBOROS)


# ============================================================
# ORPHICA × ORACLE  (6 presets)
# ============================================================
ORPHICA_ORACLE = [
    make_preset(
        "Prophecy Harp",
        ("ORPHICA", "ORACLE"),
        "HARMONIC_FOLD", 0.68,
        {"macro_character": 0.65, "macro_movement": 0.52, "macro_coupling": 0.68, "macro_space": 0.75},
        {"macro_character": 0.56, "macro_movement": 0.65, "macro_coupling": 0.68, "macro_space": 0.62},
        {"brightness": 0.85, "warmth": 0.4,  "movement": 0.55, "density": 0.42, "space": 0.75, "aggression": 0.06},
        ["prophecy", "harp", "harmonic"],
        "ORPHICA harp harmonics fold into Oracle's vision — micro-prophecy.",
    ),
    make_preset(
        "Stochastic Pluck",
        ("ORACLE", "ORPHICA"),
        "CHAOS_INJECT", 0.6,
        {"macro_character": 0.54, "macro_movement": 0.68, "macro_coupling": 0.6,  "macro_space": 0.62},
        {"macro_character": 0.65, "macro_movement": 0.5,  "macro_coupling": 0.6,  "macro_space": 0.78},
        {"brightness": 0.82, "warmth": 0.4,  "movement": 0.62, "density": 0.4,  "space": 0.78, "aggression": 0.1},
        ["stochastic", "pluck", "chaos"],
        "GENDY chaos injected into ORPHICA pluck — stochastic microsound.",
    ),
    make_preset(
        "Indigo Seafoam",
        ("ORPHICA", "ORACLE"),
        "SPECTRAL_MORPH", 0.72,
        {"macro_character": 0.68, "macro_movement": 0.5,  "macro_coupling": 0.72, "macro_space": 0.78},
        {"macro_character": 0.55, "macro_movement": 0.65, "macro_coupling": 0.72, "macro_space": 0.62},
        {"brightness": 0.88, "warmth": 0.38, "movement": 0.55, "density": 0.4,  "space": 0.8,  "aggression": 0.05},
        ["indigo", "seafoam", "spectral"],
        "Seafoam microsound spectrally morphs Oracle's indigo space — depth of vision.",
    ),
    make_preset(
        "Maqam Siren",
        ("ORACLE", "ORPHICA"),
        "PITCH_SYNC", 0.65,
        {"macro_character": 0.54, "macro_movement": 0.65, "macro_coupling": 0.65, "macro_space": 0.62},
        {"macro_character": 0.68, "macro_movement": 0.5,  "macro_coupling": 0.65, "macro_space": 0.76},
        {"brightness": 0.84, "warmth": 0.4,  "movement": 0.58, "density": 0.42, "space": 0.75, "aggression": 0.07},
        ["maqam", "siren", "pitch"],
        "Oracle Maqam scales pitch-sync ORPHICA — siren song in ancient tuning.",
    ),
    make_preset(
        "Breakpoint Harp",
        ("ORACLE", "ORPHICA"),
        "AMPLITUDE_MOD", 0.62,
        {"macro_character": 0.55, "macro_movement": 0.68, "macro_coupling": 0.62, "macro_space": 0.62},
        {"macro_character": 0.65, "macro_movement": 0.5,  "macro_coupling": 0.62, "macro_space": 0.78},
        {"brightness": 0.8,  "warmth": 0.42, "movement": 0.6,  "density": 0.4,  "space": 0.77, "aggression": 0.06},
        ["breakpoint", "harp", "amplitude"],
        "Oracle breakpoints amplitude-shape ORPHICA harp dynamics — written in light.",
    ),
    make_preset(
        "Visionary Microsound",
        ("ORPHICA", "ORACLE"),
        "RESONANCE_SHARE", 0.7,
        {"macro_character": 0.65, "macro_movement": 0.52, "macro_coupling": 0.7,  "macro_space": 0.78},
        {"macro_character": 0.55, "macro_movement": 0.68, "macro_coupling": 0.7,  "macro_space": 0.62},
        {"brightness": 0.86, "warmth": 0.38, "movement": 0.55, "density": 0.42, "space": 0.78, "aggression": 0.05},
        ["visionary", "resonance", "microsound"],
        "ORPHICA resonance nodes shared with Oracle's prophetic harmonics — sea seer.",
    ),
]
PRESETS.extend(ORPHICA_ORACLE)


# ============================================================
# ORPHICA × OBSCURA  (6 presets)
# ============================================================
ORPHICA_OBSCURA = [
    make_preset(
        "Silver Seafoam",
        ("ORPHICA", "OBSCURA"),
        "SPATIAL_COUPLE", 0.72,
        {"macro_character": 0.65, "macro_movement": 0.5,  "macro_coupling": 0.72, "macro_space": 0.78},
        {"macro_character": 0.52, "macro_movement": 0.58, "macro_coupling": 0.72, "macro_space": 0.7},
        {"brightness": 0.82, "warmth": 0.4,  "movement": 0.5,  "density": 0.42, "space": 0.82, "aggression": 0.05},
        ["silver", "seafoam", "spatial"],
        "ORPHICA microsound spatially coupled with Obscura depth — silver seafoam.",
    ),
    make_preset(
        "Corroded Pluck",
        ("OBSCURA", "ORPHICA"),
        "FILTER_MOD", 0.65,
        {"macro_character": 0.52, "macro_movement": 0.62, "macro_coupling": 0.65, "macro_space": 0.68},
        {"macro_character": 0.65, "macro_movement": 0.5,  "macro_coupling": 0.65, "macro_space": 0.78},
        {"brightness": 0.78, "warmth": 0.4,  "movement": 0.55, "density": 0.42, "space": 0.78, "aggression": 0.08},
        ["corroded", "filter", "pluck"],
        "Obscura string corrosion filter-modulates ORPHICA brightness — tarnished siren.",
    ),
    make_preset(
        "Daguerreotype Siren",
        ("ORPHICA", "OBSCURA"),
        "TIMBRE_BLEND", 0.6,
        {"macro_character": 0.62, "macro_movement": 0.52, "macro_coupling": 0.6,  "macro_space": 0.75},
        {"macro_character": 0.52, "macro_movement": 0.58, "macro_coupling": 0.6,  "macro_space": 0.7},
        {"brightness": 0.76, "warmth": 0.42, "movement": 0.52, "density": 0.45, "space": 0.76, "aggression": 0.06},
        ["daguerreotype", "siren", "blend"],
        "ORPHICA siren timbre blended with Obscura silver grain — vintage marine photograph.",
    ),
    make_preset(
        "Stiff Harp",
        ("OBSCURA", "ORPHICA"),
        "FREQUENCY_SHIFT", 0.55,
        {"macro_character": 0.5,  "macro_movement": 0.62, "macro_coupling": 0.55, "macro_space": 0.68},
        {"macro_character": 0.65, "macro_movement": 0.5,  "macro_coupling": 0.55, "macro_space": 0.76},
        {"brightness": 0.8,  "warmth": 0.4,  "movement": 0.55, "density": 0.42, "space": 0.75, "aggression": 0.07},
        ["stiff", "frequency", "harp"],
        "Obscura stiffness frequency-shifts ORPHICA strings — rigid glass harp.",
    ),
    make_preset(
        "Siphon In Silver",
        ("ORPHICA", "OBSCURA"),
        "ENVELOPE_LINK", 0.68,
        {"macro_character": 0.65, "macro_movement": 0.52, "macro_coupling": 0.68, "macro_space": 0.78},
        {"macro_character": 0.52, "macro_movement": 0.58, "macro_coupling": 0.68, "macro_space": 0.7},
        {"brightness": 0.84, "warmth": 0.38, "movement": 0.52, "density": 0.4,  "space": 0.8,  "aggression": 0.05},
        ["siphon", "envelope", "silver"],
        "ORPHICA siphonophore envelope linked into Obscura's decay — colony fades to silver.",
    ),
    make_preset(
        "Exposed Microsound",
        ("OBSCURA", "ORPHICA"),
        "VELOCITY_COUPLE", 0.65,
        {"macro_character": 0.52, "macro_movement": 0.62, "macro_coupling": 0.65, "macro_space": 0.68},
        {"macro_character": 0.68, "macro_movement": 0.5,  "macro_coupling": 0.65, "macro_space": 0.77},
        {"brightness": 0.9,  "warmth": 0.38, "movement": 0.55, "density": 0.4,  "space": 0.78, "aggression": 0.06},
        ["exposed", "velocity", "microsound"],
        "Obscura photographic exposure velocity-couples ORPHICA dynamics — light reveals harp.",
    ),
]
PRESETS.extend(ORPHICA_OBSCURA)


# ---------------------------------------------------------------------------
# Write all presets
# ---------------------------------------------------------------------------

def main():
    written = []
    skipped = []
    for preset in PRESETS:
        path = write_preset(preset)
        written.append(path)

    print(f"Written {len(written)} presets to {PRESET_DIR}")
    print("\nPairs covered:")
    pairs_seen = set()
    for p in PRESETS:
        key = tuple(sorted(p["engines"]))
        pairs_seen.add(key)
    for pair in sorted(pairs_seen):
        count = sum(1 for p in PRESETS if set(p["engines"]) == set(pair))
        print(f"  {pair[0]} × {pair[1]}: {count} presets")
    print(f"\nTotal: {len(PRESETS)} presets")


if __name__ == "__main__":
    main()
