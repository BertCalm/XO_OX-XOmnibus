#!/usr/bin/env python3
"""Generate ~72 Entangled mood coupling presets for OTTONI, OLE, and ORPHICA.

Covers 4 new partner engines for each: ORGANON, OUROBOROS, ORACLE, OBSCURA
6 presets per pair × 12 pairs = 72 total

Engine characters:
  OTTONI   — Patina green #5B8A72, triple brass, GROW macro
             warmth 0.7, brightness 0.6
             DNA bias: warmth 0.65-0.85, moderate brightness
  OLE      — Hibiscus #C9377A, Afro-Latin trio, DRAMA macro
             aggression 0.65, warmth 0.7
             DNA bias: aggression 0.55-0.85, warmth 0.60-0.80
  ORPHICA  — Siren Seafoam #7FDBCA, microsound harp, siphonophore
             brightness 0.75, space 0.7
             DNA bias: brightness 0.70-0.92, space 0.60-0.85

Coupling types (12 total, rotated across presets):
  FREQUENCY_SHIFT, AMPLITUDE_MOD, FILTER_MOD, PITCH_SYNC,
  TIMBRE_BLEND, ENVELOPE_LINK, HARMONIC_FOLD, CHAOS_INJECT,
  RESONANCE_SHARE, SPATIAL_COUPLE, SPECTRAL_MORPH, VELOCITY_COUPLE

DNA rule: every preset must have at least 1 extreme dimension (<=0.15 or >=0.85).
"""

import json
import os

PRESET_DIR = os.path.join(
    os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
    "Presets", "XOlokun", "Entangled"
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
        "parameters": {
            engine_a: params_a,
            engine_b: params_b,
        },
        "coupling": {
            "type": coupling_type,
            "source": engine_a,
            "target": engine_b,
            "amount": round(coupling_amount, 2),
            "intensity": intensity,
        },
        "dna": {k: round(v, 2) for k, v in dna.items()},
        "macros": {
            "CHARACTER": round(params_a.get("macro_character", 0.5), 2),
            "MOVEMENT":  round(params_a.get("macro_movement",  0.5), 2),
            "COUPLING":  round(params_a.get("macro_coupling",  0.5), 2),
            "SPACE":     round(params_a.get("macro_space",     0.5), 2),
        },
        "tags": tags,
    }


def validate_extreme(dna):
    """Ensure at least one DNA dimension is extreme (<= 0.15 or >= 0.85)."""
    return any(v <= 0.15 or v >= 0.85 for v in dna.values())


def save_preset(preset):
    name = preset["name"]
    filename = name.replace(" ", "_").replace("/", "-") + ".xometa"
    path = os.path.join(PRESET_DIR, filename)
    if os.path.exists(path):
        return False, path
    with open(path, "w") as f:
        json.dump(preset, f, indent=2)
    return True, path


# ---------------------------------------------------------------------------
# OTTONI preset data (24 presets across 4 partners)
# ---------------------------------------------------------------------------

OTTONI_PRESETS = [
    # ---- OTTONI × ORGANON (indices 0-5, coupling types 0-5) ----
    make_preset(
        "Verde Metabolism",
        ("OTTONI", "ORGANON"),
        COUPLING_TYPES[0], 0.72,
        {"macro_character": 0.70, "macro_movement": 0.55, "macro_coupling": 0.72, "macro_space": 0.45},
        {"macro_character": 0.50, "macro_movement": 0.65, "macro_coupling": 0.60, "macro_space": 0.55},
        {"brightness": 0.58, "warmth": 0.78, "movement": 0.55, "density": 0.60, "space": 0.42, "aggression": 0.14},
        ["entangled", "ottoni", "organon", "biological", "brass"],
        "Brass frequency shifts drive ORGANON's metabolic rhythm — a living ecosystem of harmonic life.",
    ),
    make_preset(
        "Patina Pulse",
        ("OTTONI", "ORGANON"),
        COUPLING_TYPES[1], 0.65,
        {"macro_character": 0.65, "macro_movement": 0.60, "macro_coupling": 0.68, "macro_space": 0.50},
        {"macro_character": 0.55, "macro_movement": 0.70, "macro_coupling": 0.55, "macro_space": 0.48},
        {"brightness": 0.55, "warmth": 0.82, "movement": 0.62, "density": 0.55, "space": 0.45, "aggression": 0.12},
        ["entangled", "ottoni", "organon", "warmth", "pulse"],
        "OTTONI's amplitude envelope breathes life into ORGANON's cellular texture.",
    ),
    make_preset(
        "Grow Circuit",
        ("OTTONI", "ORGANON"),
        COUPLING_TYPES[2], 0.78,
        {"macro_character": 0.75, "macro_movement": 0.50, "macro_coupling": 0.78, "macro_space": 0.40},
        {"macro_character": 0.60, "macro_movement": 0.55, "macro_coupling": 0.70, "macro_space": 0.52},
        {"brightness": 0.60, "warmth": 0.86, "movement": 0.50, "density": 0.65, "space": 0.40, "aggression": 0.10},
        ["entangled", "ottoni", "organon", "filter", "growth"],
        "Brass filter sweeps cultivate ORGANON's bio-signal — warmth at 0.86 anchors the growth arc.",
    ),
    make_preset(
        "Copper Catalyst",
        ("OTTONI", "ORGANON"),
        COUPLING_TYPES[3], 0.68,
        {"macro_character": 0.68, "macro_movement": 0.58, "macro_coupling": 0.65, "macro_space": 0.48},
        {"macro_character": 0.52, "macro_movement": 0.62, "macro_coupling": 0.58, "macro_space": 0.50},
        {"brightness": 0.52, "warmth": 0.75, "movement": 0.58, "density": 0.62, "space": 0.44, "aggression": 0.08},
        ["entangled", "ottoni", "organon", "pitch", "catalyst"],
        "Pitch-locked brass harmonics synchronize ORGANON's enzymatic cascade.",
    ),
    make_preset(
        "Brass Biome",
        ("OTTONI", "ORGANON"),
        COUPLING_TYPES[4], 0.74,
        {"macro_character": 0.72, "macro_movement": 0.62, "macro_coupling": 0.74, "macro_space": 0.43},
        {"macro_character": 0.58, "macro_movement": 0.68, "macro_coupling": 0.62, "macro_space": 0.53},
        {"brightness": 0.62, "warmth": 0.80, "movement": 0.64, "density": 0.70, "space": 0.38, "aggression": 0.13},
        ["entangled", "ottoni", "organon", "timbre", "ecosystem"],
        "Timbre blend merges triple brass with metabolic granularity — a living brass biome.",
    ),
    make_preset(
        "Ferrous Memory",
        ("OTTONI", "ORGANON"),
        COUPLING_TYPES[5], 0.62,
        {"macro_character": 0.62, "macro_movement": 0.52, "macro_coupling": 0.60, "macro_space": 0.55},
        {"macro_character": 0.48, "macro_movement": 0.60, "macro_coupling": 0.52, "macro_space": 0.58},
        {"brightness": 0.50, "warmth": 0.72, "movement": 0.52, "density": 0.58, "space": 0.50, "aggression": 0.09},
        ["entangled", "ottoni", "organon", "envelope", "memory"],
        "Envelope-linked brass decay imprints on ORGANON's long-term cellular pattern.",
    ),

    # ---- OTTONI × OUROBOROS (coupling types 6-11) ----
    make_preset(
        "Serpent Forge",
        ("OTTONI", "OUROBOROS"),
        COUPLING_TYPES[6], 0.82,
        {"macro_character": 0.78, "macro_movement": 0.65, "macro_coupling": 0.82, "macro_space": 0.35},
        {"macro_character": 0.70, "macro_movement": 0.72, "macro_coupling": 0.75, "macro_space": 0.40},
        {"brightness": 0.65, "warmth": 0.85, "movement": 0.68, "density": 0.72, "space": 0.32, "aggression": 0.25},
        ["entangled", "ottoni", "ouroboros", "chaos", "serpent"],
        "Harmonic folding draws the ouroboros into brass's recursive grain — dense and relentless.",
    ),
    make_preset(
        "Patina Attractor",
        ("OTTONI", "OUROBOROS"),
        COUPLING_TYPES[7], 0.88,
        {"macro_character": 0.80, "macro_movement": 0.70, "macro_coupling": 0.88, "macro_space": 0.30},
        {"macro_character": 0.75, "macro_movement": 0.78, "macro_coupling": 0.80, "macro_space": 0.35},
        {"brightness": 0.68, "warmth": 0.86, "movement": 0.72, "density": 0.78, "space": 0.28, "aggression": 0.35},
        ["entangled", "ottoni", "ouroboros", "chaos", "attractor"],
        "Chaos injection sends OUROBOROS into strange attractor orbits seeded by warm brass.",
    ),
    make_preset(
        "Bronze Recursion",
        ("OTTONI", "OUROBOROS"),
        COUPLING_TYPES[8], 0.76,
        {"macro_character": 0.74, "macro_movement": 0.60, "macro_coupling": 0.76, "macro_space": 0.38},
        {"macro_character": 0.68, "macro_movement": 0.65, "macro_coupling": 0.70, "macro_space": 0.42},
        {"brightness": 0.62, "warmth": 0.86, "movement": 0.62, "density": 0.68, "space": 0.35, "aggression": 0.28},
        ["entangled", "ottoni", "ouroboros", "resonance", "recursion"],
        "Resonance shared between OTTONI's bell and OUROBOROS's loop — self-devouring harmonics.",
    ),
    make_preset(
        "GROW Serpent",
        ("OTTONI", "OUROBOROS"),
        COUPLING_TYPES[9], 0.70,
        {"macro_character": 0.70, "macro_movement": 0.55, "macro_coupling": 0.70, "macro_space": 0.42},
        {"macro_character": 0.62, "macro_movement": 0.60, "macro_coupling": 0.65, "macro_space": 0.45},
        {"brightness": 0.58, "warmth": 0.86, "movement": 0.58, "density": 0.64, "space": 0.4, "aggression": 0.22},
        ["entangled", "ottoni", "ouroboros", "spatial", "growth"],
        "Spatial coupling expands OUROBOROS's coil through OTTONI's growing brass horizon.",
    ),
    make_preset(
        "Verdigris Loop",
        ("OTTONI", "OUROBOROS"),
        COUPLING_TYPES[10], 0.84,
        {"macro_character": 0.82, "macro_movement": 0.68, "macro_coupling": 0.84, "macro_space": 0.33},
        {"macro_character": 0.76, "macro_movement": 0.74, "macro_coupling": 0.78, "macro_space": 0.38},
        {"brightness": 0.7, "warmth": 0.86, "movement": 0.7, "density": 0.76, "space": 0.3, "aggression": 0.32},
        ["entangled", "ottoni", "ouroboros", "spectral", "loop"],
        "Spectral morphing between brass formants and attractor-state noise — warmth 0.84 locks the loop.",
    ),
    make_preset(
        "Brass Ouroboros",
        ("OTTONI", "OUROBOROS"),
        COUPLING_TYPES[11], 0.78,
        {"macro_character": 0.76, "macro_movement": 0.62, "macro_coupling": 0.78, "macro_space": 0.36},
        {"macro_character": 0.70, "macro_movement": 0.68, "macro_coupling": 0.72, "macro_space": 0.40},
        {"brightness": 0.64, "warmth": 0.86, "movement": 0.65, "density": 0.72, "space": 0.33, "aggression": 0.3},
        ["entangled", "ottoni", "ouroboros", "velocity", "snake"],
        "Velocity coupling ties OTTONI's attack to OUROBOROS's cycle speed — serpent swallows brass.",
    ),

    # ---- OTTONI × ORACLE (coupling types 0-5) ----
    make_preset(
        "Prophecy Brass",
        ("OTTONI", "ORACLE"),
        COUPLING_TYPES[0], 0.68,
        {"macro_character": 0.68, "macro_movement": 0.52, "macro_coupling": 0.68, "macro_space": 0.50},
        {"macro_character": 0.55, "macro_movement": 0.58, "macro_coupling": 0.60, "macro_space": 0.55},
        {"brightness": 0.55, "warmth": 0.86, "movement": 0.52, "density": 0.58, "space": 0.52, "aggression": 0.1},
        ["entangled", "ottoni", "oracle", "frequency", "prophecy"],
        "Brass frequency shifts seed ORACLE's stochastic breakpoint predictions.",
    ),
    make_preset(
        "Patina Oracle",
        ("OTTONI", "ORACLE"),
        COUPLING_TYPES[1], 0.72,
        {"macro_character": 0.72, "macro_movement": 0.58, "macro_coupling": 0.72, "macro_space": 0.46},
        {"macro_character": 0.60, "macro_movement": 0.62, "macro_coupling": 0.65, "macro_space": 0.52},
        {"brightness": 0.58, "warmth": 0.78, "movement": 0.55, "density": 0.62, "space": 0.48, "aggression": 0.11},
        ["entangled", "ottoni", "oracle", "amplitude", "maqam"],
        "OTTONI's amplitude envelope modulates ORACLE's maqam scale selection in real time.",
    ),
    make_preset(
        "Indigo Forge",
        ("OTTONI", "ORACLE"),
        COUPLING_TYPES[2], 0.76,
        {"macro_character": 0.74, "macro_movement": 0.56, "macro_coupling": 0.76, "macro_space": 0.44},
        {"macro_character": 0.62, "macro_movement": 0.60, "macro_coupling": 0.68, "macro_space": 0.50},
        {"brightness": 0.60, "warmth": 0.82, "movement": 0.56, "density": 0.65, "space": 0.46, "aggression": 0.09},
        ["entangled", "ottoni", "oracle", "filter", "indigo"],
        "Filter coupling bridges OTTONI's warm resonance with ORACLE's GENDY stochastic body.",
    ),
    make_preset(
        "Tuning Oracle",
        ("OTTONI", "ORACLE"),
        COUPLING_TYPES[3], 0.65,
        {"macro_character": 0.65, "macro_movement": 0.50, "macro_coupling": 0.65, "macro_space": 0.52},
        {"macro_character": 0.52, "macro_movement": 0.55, "macro_coupling": 0.58, "macro_space": 0.56},
        {"brightness": 0.52, "warmth": 0.70, "movement": 0.50, "density": 0.55, "space": 0.54, "aggression": 0.08},
        ["entangled", "ottoni", "oracle", "pitch", "tuning"],
        "Pitch sync locks OTTONI's third brass voice to ORACLE's microtonal breakpoint map.",
    ),
    make_preset(
        "Brass Prophecy",
        ("OTTONI", "ORACLE"),
        COUPLING_TYPES[4], 0.74,
        {"macro_character": 0.74, "macro_movement": 0.60, "macro_coupling": 0.74, "macro_space": 0.44},
        {"macro_character": 0.62, "macro_movement": 0.64, "macro_coupling": 0.66, "macro_space": 0.50},
        {"brightness": 0.62, "warmth": 0.84, "movement": 0.60, "density": 0.66, "space": 0.44, "aggression": 0.10},
        ["entangled", "ottoni", "oracle", "timbre", "stochastic"],
        "Timbre blend weaves triple brass into ORACLE's GENDY waveform — warmth 0.84 grounds the prophecy.",
    ),
    make_preset(
        "Green Augury",
        ("OTTONI", "ORACLE"),
        COUPLING_TYPES[5], 0.70,
        {"macro_character": 0.70, "macro_movement": 0.55, "macro_coupling": 0.70, "macro_space": 0.48},
        {"macro_character": 0.58, "macro_movement": 0.60, "macro_coupling": 0.62, "macro_space": 0.52},
        {"brightness": 0.56, "warmth": 0.76, "movement": 0.54, "density": 0.60, "space": 0.48, "aggression": 0.09},
        ["entangled", "ottoni", "oracle", "envelope", "augury"],
        "Envelope linking draws ORACLE's sustained glissandi through OTTONI's brass sustain arc.",
    ),

    # ---- OTTONI × OBSCURA (coupling types 6-11) ----
    make_preset(
        "Daguerreotype Brass",
        ("OTTONI", "OBSCURA"),
        COUPLING_TYPES[6], 0.74,
        {"macro_character": 0.72, "macro_movement": 0.58, "macro_coupling": 0.74, "macro_space": 0.46},
        {"macro_character": 0.60, "macro_movement": 0.55, "macro_coupling": 0.65, "macro_space": 0.52},
        {"brightness": 0.58, "warmth": 0.80, "movement": 0.55, "density": 0.64, "space": 0.50, "aggression": 0.10},
        ["entangled", "ottoni", "obscura", "harmonic", "silver"],
        "Harmonic folding through OBSCURA's string network — brass preserved in silver.",
    ),
    make_preset(
        "Patina Film",
        ("OTTONI", "OBSCURA"),
        COUPLING_TYPES[7], 0.68,
        {"macro_character": 0.68, "macro_movement": 0.52, "macro_coupling": 0.68, "macro_space": 0.50},
        {"macro_character": 0.55, "macro_movement": 0.50, "macro_coupling": 0.60, "macro_space": 0.55},
        {"brightness": 0.52, "warmth": 0.76, "movement": 0.50, "density": 0.60, "space": 0.54, "aggression": 0.08},
        ["entangled", "ottoni", "obscura", "chaos", "film"],
        "Chaos injection sends OBSCURA's stiff strings into unpredictable resonant territories.",
    ),
    make_preset(
        "GROW Obscura",
        ("OTTONI", "OBSCURA"),
        COUPLING_TYPES[8], 0.78,
        {"macro_character": 0.76, "macro_movement": 0.62, "macro_coupling": 0.78, "macro_space": 0.42},
        {"macro_character": 0.65, "macro_movement": 0.58, "macro_coupling": 0.70, "macro_space": 0.48},
        {"brightness": 0.62, "warmth": 0.84, "movement": 0.60, "density": 0.68, "space": 0.44, "aggression": 0.09},
        ["entangled", "ottoni", "obscura", "resonance", "growth"],
        "Resonance shared through OBSCURA's plate — warmth 0.84 floods the system.",
    ),
    make_preset(
        "Brass Exposure",
        ("OTTONI", "OBSCURA"),
        COUPLING_TYPES[9], 0.72,
        {"macro_character": 0.70, "macro_movement": 0.55, "macro_coupling": 0.72, "macro_space": 0.46},
        {"macro_character": 0.58, "macro_movement": 0.52, "macro_coupling": 0.64, "macro_space": 0.52},
        {"brightness": 0.55, "warmth": 0.78, "movement": 0.52, "density": 0.62, "space": 0.48, "aggression": 0.07},
        ["entangled", "ottoni", "obscura", "spatial", "exposure"],
        "Spatial coupling spreads triple brass across OBSCURA's reverberant field.",
    ),
    make_preset(
        "Verdigris Daguerreotype",
        ("OTTONI", "OBSCURA"),
        COUPLING_TYPES[10], 0.80,
        {"macro_character": 0.78, "macro_movement": 0.65, "macro_coupling": 0.80, "macro_space": 0.38},
        {"macro_character": 0.68, "macro_movement": 0.60, "macro_coupling": 0.74, "macro_space": 0.44},
        {"brightness": 0.65, "warmth": 0.86, "movement": 0.62, "density": 0.70, "space": 0.40, "aggression": 0.10},
        ["entangled", "ottoni", "obscura", "spectral", "daguerreotype"],
        "Spectral morphing captures brass formants in OBSCURA's photographic resonance — warmth 0.86.",
    ),
    make_preset(
        "Velocity Patina",
        ("OTTONI", "OBSCURA"),
        COUPLING_TYPES[11], 0.66,
        {"macro_character": 0.66, "macro_movement": 0.52, "macro_coupling": 0.66, "macro_space": 0.52},
        {"macro_character": 0.54, "macro_movement": 0.48, "macro_coupling": 0.58, "macro_space": 0.56},
        {"brightness": 0.50, "warmth": 0.74, "movement": 0.50, "density": 0.58, "space": 0.52, "aggression": 0.06},
        ["entangled", "ottoni", "obscura", "velocity", "patina"],
        "Velocity coupling: harder strikes age the patina faster, bowing OBSCURA's plate inward.",
    ),
]

# ---------------------------------------------------------------------------
# OLE preset data (24 presets across 4 partners)
# ---------------------------------------------------------------------------

OLE_PRESETS = [
    # ---- OLE × ORGANON (coupling types 0-5) ----
    make_preset(
        "Drama Metabolism",
        ("OLE", "ORGANON"),
        COUPLING_TYPES[0], 0.80,
        {"macro_character": 0.78, "macro_movement": 0.72, "macro_coupling": 0.80, "macro_space": 0.38},
        {"macro_character": 0.58, "macro_movement": 0.70, "macro_coupling": 0.68, "macro_space": 0.52},
        {"brightness": 0.62, "warmth": 0.72, "movement": 0.75, "density": 0.68, "space": 0.38, "aggression": 0.86},
        ["entangled", "ole", "organon", "frequency", "drama"],
        "OLE's percussive frequency spikes drive ORGANON into metabolic overdrive — aggression 0.86.",
    ),
    make_preset(
        "Hibiscus Pulse",
        ("OLE", "ORGANON"),
        COUPLING_TYPES[1], 0.74,
        {"macro_character": 0.74, "macro_movement": 0.68, "macro_coupling": 0.74, "macro_space": 0.42},
        {"macro_character": 0.55, "macro_movement": 0.65, "macro_coupling": 0.62, "macro_space": 0.55},
        {"brightness": 0.58, "warmth": 0.74, "movement": 0.7, "density": 0.65, "space": 0.42, "aggression": 0.86},
        ["entangled", "ole", "organon", "amplitude", "pulse"],
        "Amplitude modulation stamps Afro-Latin rhythm into ORGANON's cellular texture.",
    ),
    make_preset(
        "Drama Circuit",
        ("OLE", "ORGANON"),
        COUPLING_TYPES[2], 0.82,
        {"macro_character": 0.80, "macro_movement": 0.74, "macro_coupling": 0.82, "macro_space": 0.36},
        {"macro_character": 0.62, "macro_movement": 0.72, "macro_coupling": 0.72, "macro_space": 0.50},
        {"brightness": 0.65, "warmth": 0.78, "movement": 0.78, "density": 0.72, "space": 0.35, "aggression": 0.88},
        ["entangled", "ole", "organon", "filter", "drama"],
        "Filter sweeps from OLE's DRAMA macro trigger ORGANON's enzymatic filters — aggression 0.88.",
    ),
    make_preset(
        "Latin Catalyst",
        ("OLE", "ORGANON"),
        COUPLING_TYPES[3], 0.70,
        {"macro_character": 0.70, "macro_movement": 0.64, "macro_coupling": 0.70, "macro_space": 0.46},
        {"macro_character": 0.52, "macro_movement": 0.62, "macro_coupling": 0.60, "macro_space": 0.54},
        {"brightness": 0.55, "warmth": 0.7, "movement": 0.66, "density": 0.62, "space": 0.44, "aggression": 0.86},
        ["entangled", "ole", "organon", "pitch", "latin"],
        "Pitch synchronization locks Afro-Latin polyrhythm into ORGANON's replication cycles.",
    ),
    make_preset(
        "Hibiscus Biome",
        ("OLE", "ORGANON"),
        COUPLING_TYPES[4], 0.76,
        {"macro_character": 0.76, "macro_movement": 0.70, "macro_coupling": 0.76, "macro_space": 0.40},
        {"macro_character": 0.60, "macro_movement": 0.68, "macro_coupling": 0.65, "macro_space": 0.52},
        {"brightness": 0.62, "warmth": 0.76, "movement": 0.72, "density": 0.7, "space": 0.38, "aggression": 0.86},
        ["entangled", "ole", "organon", "timbre", "tropical"],
        "Timbre blend — DRAMA's tropical palette colors ORGANON's bio-signal vermillion.",
    ),
    make_preset(
        "Afro Enzyme",
        ("OLE", "ORGANON"),
        COUPLING_TYPES[5], 0.72,
        {"macro_character": 0.72, "macro_movement": 0.66, "macro_coupling": 0.72, "macro_space": 0.44},
        {"macro_character": 0.56, "macro_movement": 0.64, "macro_coupling": 0.62, "macro_space": 0.54},
        {"brightness": 0.58, "warmth": 0.72, "movement": 0.68, "density": 0.66, "space": 0.42, "aggression": 0.86},
        ["entangled", "ole", "organon", "envelope", "afro"],
        "Envelope linking: OLE's decay triggers ORGANON's protein-synthesis envelope.",
    ),

    # ---- OLE × OUROBOROS (coupling types 6-11) ----
    make_preset(
        "Drama Attractor",
        ("OLE", "OUROBOROS"),
        COUPLING_TYPES[6], 0.90,
        {"macro_character": 0.88, "macro_movement": 0.80, "macro_coupling": 0.90, "macro_space": 0.28},
        {"macro_character": 0.80, "macro_movement": 0.84, "macro_coupling": 0.85, "macro_space": 0.32},
        {"brightness": 0.72, "warmth": 0.75, "movement": 0.85, "density": 0.82, "space": 0.25, "aggression": 0.90},
        ["entangled", "ole", "ouroboros", "chaos", "drama"],
        "DRAMA × CHAOS: OLE's harmonic folds ignite OUROBOROS's strange attractor — aggression 0.90.",
    ),
    make_preset(
        "Serpent Drama",
        ("OLE", "OUROBOROS"),
        COUPLING_TYPES[7], 0.85,
        {"macro_character": 0.84, "macro_movement": 0.76, "macro_coupling": 0.85, "macro_space": 0.32},
        {"macro_character": 0.76, "macro_movement": 0.80, "macro_coupling": 0.80, "macro_space": 0.36},
        {"brightness": 0.68, "warmth": 0.72, "movement": 0.82, "density": 0.78, "space": 0.28, "aggression": 0.85},
        ["entangled", "ole", "ouroboros", "chaos", "serpent"],
        "Chaos injection from OUROBOROS dismantles OLE's Afro-Latin phrase structure, rebuilding it recursively.",
    ),
    make_preset(
        "Ole Resonance",
        ("OLE", "OUROBOROS"),
        COUPLING_TYPES[8], 0.80,
        {"macro_character": 0.80, "macro_movement": 0.72, "macro_coupling": 0.80, "macro_space": 0.36},
        {"macro_character": 0.72, "macro_movement": 0.76, "macro_coupling": 0.75, "macro_space": 0.40},
        {"brightness": 0.65, "warmth": 0.7, "movement": 0.78, "density": 0.75, "space": 0.32, "aggression": 0.86},
        ["entangled", "ole", "ouroboros", "resonance", "drama"],
        "Resonance shared between OLE's clave hit and OUROBOROS's infinite cycle — a drum inside the snake.",
    ),
    make_preset(
        "Latin Ouroboros",
        ("OLE", "OUROBOROS"),
        COUPLING_TYPES[9], 0.78,
        {"macro_character": 0.78, "macro_movement": 0.70, "macro_coupling": 0.78, "macro_space": 0.38},
        {"macro_character": 0.70, "macro_movement": 0.74, "macro_coupling": 0.72, "macro_space": 0.42},
        {"brightness": 0.62, "warmth": 0.68, "movement": 0.76, "density": 0.72, "space": 0.35, "aggression": 0.86},
        ["entangled", "ole", "ouroboros", "spatial", "latin"],
        "Spatial coupling extends OLE's call-and-response into OUROBOROS's dimensional loop.",
    ),
    make_preset(
        "Hibiscus Chaos",
        ("OLE", "OUROBOROS"),
        COUPLING_TYPES[10], 0.88,
        {"macro_character": 0.86, "macro_movement": 0.78, "macro_coupling": 0.88, "macro_space": 0.30},
        {"macro_character": 0.78, "macro_movement": 0.82, "macro_coupling": 0.83, "macro_space": 0.34},
        {"brightness": 0.70, "warmth": 0.73, "movement": 0.84, "density": 0.80, "space": 0.26, "aggression": 0.88},
        ["entangled", "ole", "ouroboros", "spectral", "chaos"],
        "Spectral morphing between hibiscus harmonic burst and OUROBOROS's self-replicating noise.",
    ),
    make_preset(
        "Velocity Serpent",
        ("OLE", "OUROBOROS"),
        COUPLING_TYPES[11], 0.82,
        {"macro_character": 0.82, "macro_movement": 0.74, "macro_coupling": 0.82, "macro_space": 0.34},
        {"macro_character": 0.74, "macro_movement": 0.78, "macro_coupling": 0.77, "macro_space": 0.38},
        {"brightness": 0.65, "warmth": 0.7, "movement": 0.8, "density": 0.76, "space": 0.3, "aggression": 0.86},
        ["entangled", "ole", "ouroboros", "velocity", "serpent"],
        "Velocity coupling: OLE's attack energy winds OUROBOROS's recursion rate tighter.",
    ),

    # ---- OLE × ORACLE (coupling types 0-5) ----
    make_preset(
        "Drama Prophecy",
        ("OLE", "ORACLE"),
        COUPLING_TYPES[0], 0.78,
        {"macro_character": 0.78, "macro_movement": 0.70, "macro_coupling": 0.78, "macro_space": 0.40},
        {"macro_character": 0.62, "macro_movement": 0.65, "macro_coupling": 0.68, "macro_space": 0.52},
        {"brightness": 0.62, "warmth": 0.73, "movement": 0.72, "density": 0.68, "space": 0.4, "aggression": 0.86},
        ["entangled", "ole", "oracle", "frequency", "prophecy"],
        "Afro-Latin frequency bursts seed ORACLE's prophetic stochastic sequences.",
    ),
    make_preset(
        "Hibiscus Augury",
        ("OLE", "ORACLE"),
        COUPLING_TYPES[1], 0.72,
        {"macro_character": 0.72, "macro_movement": 0.65, "macro_coupling": 0.72, "macro_space": 0.44},
        {"macro_character": 0.58, "macro_movement": 0.62, "macro_coupling": 0.63, "macro_space": 0.54},
        {"brightness": 0.58, "warmth": 0.7, "movement": 0.68, "density": 0.65, "space": 0.42, "aggression": 0.86},
        ["entangled", "ole", "oracle", "amplitude", "maqam"],
        "Amplitude of OLE's drum kit sculpts ORACLE's microtonal landscape in real-time.",
    ),
    make_preset(
        "DRAMA Filter Oracle",
        ("OLE", "ORACLE"),
        COUPLING_TYPES[2], 0.82,
        {"macro_character": 0.80, "macro_movement": 0.72, "macro_coupling": 0.82, "macro_space": 0.38},
        {"macro_character": 0.66, "macro_movement": 0.68, "macro_coupling": 0.72, "macro_space": 0.50},
        {"brightness": 0.66, "warmth": 0.76, "movement": 0.75, "density": 0.72, "space": 0.36, "aggression": 0.86},
        ["entangled", "ole", "oracle", "filter", "drama"],
        "Filter coupling — DRAMA macro sweeps forward into ORACLE's GENDY resonant body — aggression 0.86.",
    ),
    make_preset(
        "Afro Prophecy",
        ("OLE", "ORACLE"),
        COUPLING_TYPES[3], 0.68,
        {"macro_character": 0.68, "macro_movement": 0.62, "macro_coupling": 0.68, "macro_space": 0.48},
        {"macro_character": 0.55, "macro_movement": 0.58, "macro_coupling": 0.60, "macro_space": 0.55},
        {"brightness": 0.55, "warmth": 0.68, "movement": 0.64, "density": 0.62, "space": 0.46, "aggression": 0.86},
        ["entangled", "ole", "oracle", "pitch", "afro"],
        "Pitch sync aligns OLE's tumbadora tuning to ORACLE's microtonal breakpoint grid.",
    ),
    make_preset(
        "Ole Oracle",
        ("OLE", "ORACLE"),
        COUPLING_TYPES[4], 0.76,
        {"macro_character": 0.76, "macro_movement": 0.68, "macro_coupling": 0.76, "macro_space": 0.42},
        {"macro_character": 0.62, "macro_movement": 0.64, "macro_coupling": 0.66, "macro_space": 0.52},
        {"brightness": 0.62, "warmth": 0.74, "movement": 0.7, "density": 0.68, "space": 0.4, "aggression": 0.86},
        ["entangled", "ole", "oracle", "timbre", "ritual"],
        "Timbre blend: ORACLE's stochastic tone emerges from OLE's ritual drumming.",
    ),
    make_preset(
        "Hibiscus Indigo",
        ("OLE", "ORACLE"),
        COUPLING_TYPES[5], 0.74,
        {"macro_character": 0.74, "macro_movement": 0.66, "macro_coupling": 0.74, "macro_space": 0.44},
        {"macro_character": 0.60, "macro_movement": 0.62, "macro_coupling": 0.64, "macro_space": 0.52},
        {"brightness": 0.6, "warmth": 0.72, "movement": 0.68, "density": 0.66, "space": 0.42, "aggression": 0.86},
        ["entangled", "ole", "oracle", "envelope", "hibiscus"],
        "Envelope linking: OLE's sustain bloom triggers ORACLE's GENDY glissando arc.",
    ),

    # ---- OLE × OBSCURA (coupling types 6-11) ----
    make_preset(
        "Drama Daguerreotype",
        ("OLE", "OBSCURA"),
        COUPLING_TYPES[6], 0.80,
        {"macro_character": 0.80, "macro_movement": 0.72, "macro_coupling": 0.80, "macro_space": 0.36},
        {"macro_character": 0.65, "macro_movement": 0.58, "macro_coupling": 0.70, "macro_space": 0.50},
        {"brightness": 0.65, "warmth": 0.74, "movement": 0.74, "density": 0.7, "space": 0.34, "aggression": 0.86},
        ["entangled", "ole", "obscura", "harmonic", "drama"],
        "Harmonic folds from DRAMA's percussive attack are preserved in OBSCURA's silver plate.",
    ),
    make_preset(
        "Ole Obscura",
        ("OLE", "OBSCURA"),
        COUPLING_TYPES[7], 0.76,
        {"macro_character": 0.76, "macro_movement": 0.68, "macro_coupling": 0.76, "macro_space": 0.40},
        {"macro_character": 0.62, "macro_movement": 0.55, "macro_coupling": 0.65, "macro_space": 0.52},
        {"brightness": 0.62, "warmth": 0.72, "movement": 0.7, "density": 0.68, "space": 0.38, "aggression": 0.86},
        ["entangled", "ole", "obscura", "chaos", "obscura"],
        "Chaos injection sends OBSCURA's strings into Afro-Latin syncopated chaos.",
    ),
    make_preset(
        "Latin Resonance",
        ("OLE", "OBSCURA"),
        COUPLING_TYPES[8], 0.74,
        {"macro_character": 0.74, "macro_movement": 0.66, "macro_coupling": 0.74, "macro_space": 0.42},
        {"macro_character": 0.60, "macro_movement": 0.52, "macro_coupling": 0.62, "macro_space": 0.54},
        {"brightness": 0.58, "warmth": 0.7, "movement": 0.68, "density": 0.65, "space": 0.4, "aggression": 0.86},
        ["entangled", "ole", "obscura", "resonance", "latin"],
        "Resonance shared between clave transient and OBSCURA's plate nodes — rhythmically focused.",
    ),
    make_preset(
        "Hibiscus Exposure",
        ("OLE", "OBSCURA"),
        COUPLING_TYPES[9], 0.70,
        {"macro_character": 0.70, "macro_movement": 0.62, "macro_coupling": 0.70, "macro_space": 0.46},
        {"macro_character": 0.56, "macro_movement": 0.50, "macro_coupling": 0.58, "macro_space": 0.55},
        {"brightness": 0.55, "warmth": 0.68, "movement": 0.65, "density": 0.62, "space": 0.44, "aggression": 0.86},
        ["entangled", "ole", "obscura", "spatial", "hibiscus"],
        "Spatial coupling places OLE's call-and-response inside OBSCURA's reverberant chamber.",
    ),
    make_preset(
        "Drama Silver",
        ("OLE", "OBSCURA"),
        COUPLING_TYPES[10], 0.84,
        {"macro_character": 0.82, "macro_movement": 0.74, "macro_coupling": 0.84, "macro_space": 0.34},
        {"macro_character": 0.68, "macro_movement": 0.60, "macro_coupling": 0.74, "macro_space": 0.48},
        {"brightness": 0.68, "warmth": 0.76, "movement": 0.76, "density": 0.74, "space": 0.32, "aggression": 0.88},
        ["entangled", "ole", "obscura", "spectral", "drama"],
        "Spectral morphing: DRAMA's harmonic spray exposed on OBSCURA's silver surface — aggression 0.88.",
    ),
    make_preset(
        "Velocity Drama",
        ("OLE", "OBSCURA"),
        COUPLING_TYPES[11], 0.78,
        {"macro_character": 0.78, "macro_movement": 0.70, "macro_coupling": 0.78, "macro_space": 0.38},
        {"macro_character": 0.64, "macro_movement": 0.56, "macro_coupling": 0.68, "macro_space": 0.50},
        {"brightness": 0.62, "warmth": 0.72, "movement": 0.72, "density": 0.68, "space": 0.36, "aggression": 0.86},
        ["entangled", "ole", "obscura", "velocity", "drama"],
        "Velocity coupling: harder OLE hits expose deeper grain in OBSCURA's surface.",
    ),
]

# ---------------------------------------------------------------------------
# ORPHICA preset data (24 presets across 4 partners)
# ---------------------------------------------------------------------------

ORPHICA_PRESETS = [
    # ---- ORPHICA × ORGANON (coupling types 0-5) ----
    make_preset(
        "Harp Metabolism",
        ("ORPHICA", "ORGANON"),
        COUPLING_TYPES[0], 0.68,
        {"macro_character": 0.68, "macro_movement": 0.55, "macro_coupling": 0.68, "macro_space": 0.72},
        {"macro_character": 0.50, "macro_movement": 0.62, "macro_coupling": 0.58, "macro_space": 0.58},
        {"brightness": 0.88, "warmth": 0.42, "movement": 0.55, "density": 0.45, "space": 0.78, "aggression": 0.08},
        ["entangled", "orphica", "organon", "frequency", "harp"],
        "Microsound harp frequency shifts trigger ORGANON's metabolic cascade — brightness 0.88.",
    ),
    make_preset(
        "Seafoam Pulse",
        ("ORPHICA", "ORGANON"),
        COUPLING_TYPES[1], 0.65,
        {"macro_character": 0.65, "macro_movement": 0.52, "macro_coupling": 0.65, "macro_space": 0.75},
        {"macro_character": 0.48, "macro_movement": 0.60, "macro_coupling": 0.55, "macro_space": 0.60},
        {"brightness": 0.86, "warmth": 0.40, "movement": 0.52, "density": 0.42, "space": 0.76, "aggression": 0.07},
        ["entangled", "orphica", "organon", "amplitude", "seafoam"],
        "ORPHICA's siphonophore amplitude pulse breathes into ORGANON's cellular rhythm.",
    ),
    make_preset(
        "Siren Circuit",
        ("ORPHICA", "ORGANON"),
        COUPLING_TYPES[2], 0.72,
        {"macro_character": 0.72, "macro_movement": 0.58, "macro_coupling": 0.72, "macro_space": 0.70},
        {"macro_character": 0.55, "macro_movement": 0.65, "macro_coupling": 0.62, "macro_space": 0.56},
        {"brightness": 0.90, "warmth": 0.38, "movement": 0.58, "density": 0.48, "space": 0.72, "aggression": 0.09},
        ["entangled", "orphica", "organon", "filter", "siren"],
        "Filter coupling — ORPHICA's glass-harmonic filter feeds ORGANON's enzymatic sweep — brightness 0.90.",
    ),
    make_preset(
        "Harp Catalyst",
        ("ORPHICA", "ORGANON"),
        COUPLING_TYPES[3], 0.62,
        {"macro_character": 0.62, "macro_movement": 0.50, "macro_coupling": 0.62, "macro_space": 0.74},
        {"macro_character": 0.46, "macro_movement": 0.58, "macro_coupling": 0.52, "macro_space": 0.60},
        {"brightness": 0.84, "warmth": 0.40, "movement": 0.50, "density": 0.40, "space": 0.74, "aggression": 0.07},
        ["entangled", "orphica", "organon", "pitch", "harp"],
        "Pitch sync locks ORPHICA's microtonal glissando into ORGANON's enzymatic resonance map.",
    ),
    make_preset(
        "Seafoam Biome",
        ("ORPHICA", "ORGANON"),
        COUPLING_TYPES[4], 0.70,
        {"macro_character": 0.70, "macro_movement": 0.55, "macro_coupling": 0.70, "macro_space": 0.72},
        {"macro_character": 0.52, "macro_movement": 0.63, "macro_coupling": 0.60, "macro_space": 0.58},
        {"brightness": 0.88, "warmth": 0.42, "movement": 0.55, "density": 0.46, "space": 0.76, "aggression": 0.08},
        ["entangled", "orphica", "organon", "timbre", "seafoam"],
        "Timbre blend: ORPHICA's harmonic shimmer colors ORGANON's bio-signal teal.",
    ),
    make_preset(
        "Siphonophore Memory",
        ("ORPHICA", "ORGANON"),
        COUPLING_TYPES[5], 0.66,
        {"macro_character": 0.66, "macro_movement": 0.52, "macro_coupling": 0.66, "macro_space": 0.73},
        {"macro_character": 0.50, "macro_movement": 0.60, "macro_coupling": 0.56, "macro_space": 0.60},
        {"brightness": 0.85, "warmth": 0.40, "movement": 0.52, "density": 0.42, "space": 0.74, "aggression": 0.07},
        ["entangled", "orphica", "organon", "envelope", "siphonophore"],
        "Envelope linking: ORPHICA's long decay imprints on ORGANON's long-term cellular pattern.",
    ),

    # ---- ORPHICA × OUROBOROS (coupling types 6-11) ----
    make_preset(
        "Glass Serpent",
        ("ORPHICA", "OUROBOROS"),
        COUPLING_TYPES[6], 0.76,
        {"macro_character": 0.76, "macro_movement": 0.62, "macro_coupling": 0.76, "macro_space": 0.68},
        {"macro_character": 0.68, "macro_movement": 0.70, "macro_coupling": 0.72, "macro_space": 0.40},
        {"brightness": 0.90, "warmth": 0.35, "movement": 0.64, "density": 0.55, "space": 0.70, "aggression": 0.20},
        ["entangled", "orphica", "ouroboros", "harmonic", "glass"],
        "ORPHICA's harmonic folding braids with OUROBOROS's self-consuming loop — brightness 0.90.",
    ),
    make_preset(
        "Siren Attractor",
        ("ORPHICA", "OUROBOROS"),
        COUPLING_TYPES[7], 0.82,
        {"macro_character": 0.80, "macro_movement": 0.68, "macro_coupling": 0.82, "macro_space": 0.65},
        {"macro_character": 0.74, "macro_movement": 0.76, "macro_coupling": 0.78, "macro_space": 0.36},
        {"brightness": 0.92, "warmth": 0.32, "movement": 0.70, "density": 0.60, "space": 0.68, "aggression": 0.28},
        ["entangled", "orphica", "ouroboros", "chaos", "siren"],
        "Chaos injection from OUROBOROS disrupts ORPHICA's crystalline lattice — brightness 0.92.",
    ),
    make_preset(
        "Seafoam Recursion",
        ("ORPHICA", "OUROBOROS"),
        COUPLING_TYPES[8], 0.74,
        {"macro_character": 0.74, "macro_movement": 0.60, "macro_coupling": 0.74, "macro_space": 0.70},
        {"macro_character": 0.66, "macro_movement": 0.68, "macro_coupling": 0.70, "macro_space": 0.40},
        {"brightness": 0.88, "warmth": 0.36, "movement": 0.62, "density": 0.52, "space": 0.72, "aggression": 0.18},
        ["entangled", "orphica", "ouroboros", "resonance", "recursion"],
        "Resonance shared between ORPHICA's glass harmonic and OUROBOROS's recursive loop.",
    ),
    make_preset(
        "Harp Ouroboros",
        ("ORPHICA", "OUROBOROS"),
        COUPLING_TYPES[9], 0.70,
        {"macro_character": 0.70, "macro_movement": 0.56, "macro_coupling": 0.70, "macro_space": 0.72},
        {"macro_character": 0.62, "macro_movement": 0.64, "macro_coupling": 0.65, "macro_space": 0.42},
        {"brightness": 0.86, "warmth": 0.36, "movement": 0.58, "density": 0.50, "space": 0.74, "aggression": 0.15},
        ["entangled", "orphica", "ouroboros", "spatial", "harp"],
        "Spatial coupling places OUROBOROS's eternal loop inside ORPHICA's resonant chamber.",
    ),
    make_preset(
        "Crystal Chaos",
        ("ORPHICA", "OUROBOROS"),
        COUPLING_TYPES[10], 0.80,
        {"macro_character": 0.78, "macro_movement": 0.66, "macro_coupling": 0.80, "macro_space": 0.66},
        {"macro_character": 0.72, "macro_movement": 0.74, "macro_coupling": 0.76, "macro_space": 0.38},
        {"brightness": 0.91, "warmth": 0.33, "movement": 0.68, "density": 0.58, "space": 0.70, "aggression": 0.25},
        ["entangled", "orphica", "ouroboros", "spectral", "crystal"],
        "Spectral morphing crystallizes OUROBOROS's noise into ORPHICA's glass-harmonic structure.",
    ),
    make_preset(
        "Velocity Siren",
        ("ORPHICA", "OUROBOROS"),
        COUPLING_TYPES[11], 0.72,
        {"macro_character": 0.72, "macro_movement": 0.58, "macro_coupling": 0.72, "macro_space": 0.70},
        {"macro_character": 0.64, "macro_movement": 0.66, "macro_coupling": 0.67, "macro_space": 0.42},
        {"brightness": 0.87, "warmth": 0.35, "movement": 0.60, "density": 0.52, "space": 0.72, "aggression": 0.18},
        ["entangled", "orphica", "ouroboros", "velocity", "siren"],
        "Velocity coupling: delicate ORPHICA touch slows OUROBOROS's cycle; hard attacks accelerate it.",
    ),

    # ---- ORPHICA × ORACLE (coupling types 0-5) ----
    make_preset(
        "Harp Prophecy",
        ("ORPHICA", "ORACLE"),
        COUPLING_TYPES[0], 0.68,
        {"macro_character": 0.68, "macro_movement": 0.55, "macro_coupling": 0.68, "macro_space": 0.74},
        {"macro_character": 0.54, "macro_movement": 0.60, "macro_coupling": 0.60, "macro_space": 0.58},
        {"brightness": 0.88, "warmth": 0.38, "movement": 0.55, "density": 0.44, "space": 0.76, "aggression": 0.08},
        ["entangled", "orphica", "oracle", "frequency", "prophecy"],
        "ORPHICA's high-frequency shimmer seeds ORACLE's stochastic breakpoint mapping.",
    ),
    make_preset(
        "Seafoam Augury",
        ("ORPHICA", "ORACLE"),
        COUPLING_TYPES[1], 0.65,
        {"macro_character": 0.65, "macro_movement": 0.52, "macro_coupling": 0.65, "macro_space": 0.76},
        {"macro_character": 0.52, "macro_movement": 0.58, "macro_coupling": 0.56, "macro_space": 0.60},
        {"brightness": 0.86, "warmth": 0.36, "movement": 0.52, "density": 0.42, "space": 0.78, "aggression": 0.07},
        ["entangled", "orphica", "oracle", "amplitude", "seafoam"],
        "Amplitude of ORPHICA's glissando modulates ORACLE's GENDY stochastic parameters.",
    ),
    make_preset(
        "Siren Oracle",
        ("ORPHICA", "ORACLE"),
        COUPLING_TYPES[2], 0.74,
        {"macro_character": 0.74, "macro_movement": 0.60, "macro_coupling": 0.74, "macro_space": 0.72},
        {"macro_character": 0.60, "macro_movement": 0.64, "macro_coupling": 0.66, "macro_space": 0.56},
        {"brightness": 0.90, "warmth": 0.36, "movement": 0.60, "density": 0.48, "space": 0.74, "aggression": 0.09},
        ["entangled", "orphica", "oracle", "filter", "siren"],
        "Filter coupling — ORPHICA's bright filter lattice intersects ORACLE's maqam framework — brightness 0.90.",
    ),
    make_preset(
        "Crystal Augury",
        ("ORPHICA", "ORACLE"),
        COUPLING_TYPES[3], 0.62,
        {"macro_character": 0.62, "macro_movement": 0.50, "macro_coupling": 0.62, "macro_space": 0.76},
        {"macro_character": 0.50, "macro_movement": 0.55, "macro_coupling": 0.54, "macro_space": 0.60},
        {"brightness": 0.84, "warmth": 0.36, "movement": 0.50, "density": 0.40, "space": 0.78, "aggression": 0.06},
        ["entangled", "orphica", "oracle", "pitch", "crystal"],
        "Pitch sync locks ORPHICA's overtone series to ORACLE's microtonal breakpoint lattice.",
    ),
    make_preset(
        "Glass Prophecy",
        ("ORPHICA", "ORACLE"),
        COUPLING_TYPES[4], 0.70,
        {"macro_character": 0.70, "macro_movement": 0.56, "macro_coupling": 0.70, "macro_space": 0.74},
        {"macro_character": 0.56, "macro_movement": 0.61, "macro_coupling": 0.62, "macro_space": 0.58},
        {"brightness": 0.88, "warmth": 0.38, "movement": 0.56, "density": 0.46, "space": 0.76, "aggression": 0.08},
        ["entangled", "orphica", "oracle", "timbre", "glass"],
        "Timbre blend: ORACLE's GENDY waveform acquires ORPHICA's glass-harmonic sheen.",
    ),
    make_preset(
        "Siphonophore Oracle",
        ("ORPHICA", "ORACLE"),
        COUPLING_TYPES[5], 0.68,
        {"macro_character": 0.68, "macro_movement": 0.54, "macro_coupling": 0.68, "macro_space": 0.75},
        {"macro_character": 0.54, "macro_movement": 0.59, "macro_coupling": 0.60, "macro_space": 0.59},
        {"brightness": 0.86, "warmth": 0.37, "movement": 0.54, "density": 0.44, "space": 0.77, "aggression": 0.07},
        ["entangled", "orphica", "oracle", "envelope", "siphonophore"],
        "Envelope linking: ORPHICA's sustain whisper triggers ORACLE's prophetic glissando arc.",
    ),

    # ---- ORPHICA × OBSCURA (coupling types 6-11) ----
    make_preset(
        "Crystal Daguerreotype",
        ("ORPHICA", "OBSCURA"),
        COUPLING_TYPES[6], 0.72,
        {"macro_character": 0.72, "macro_movement": 0.58, "macro_coupling": 0.72, "macro_space": 0.72},
        {"macro_character": 0.58, "macro_movement": 0.52, "macro_coupling": 0.62, "macro_space": 0.56},
        {"brightness": 0.90, "warmth": 0.35, "movement": 0.55, "density": 0.48, "space": 0.74, "aggression": 0.08},
        ["entangled", "orphica", "obscura", "harmonic", "crystal"],
        "ORPHICA's harmonic spectrum folded into OBSCURA's silver surface — brightness 0.90.",
    ),
    make_preset(
        "Seafoam Obscura",
        ("ORPHICA", "OBSCURA"),
        COUPLING_TYPES[7], 0.68,
        {"macro_character": 0.68, "macro_movement": 0.54, "macro_coupling": 0.68, "macro_space": 0.74},
        {"macro_character": 0.54, "macro_movement": 0.48, "macro_coupling": 0.58, "macro_space": 0.58},
        {"brightness": 0.87, "warmth": 0.34, "movement": 0.52, "density": 0.44, "space": 0.76, "aggression": 0.07},
        ["entangled", "orphica", "obscura", "chaos", "seafoam"],
        "Chaos injection from OBSCURA's string network disturbs ORPHICA's crystalline calm.",
    ),
    make_preset(
        "Harp Resonance",
        ("ORPHICA", "OBSCURA"),
        COUPLING_TYPES[8], 0.76,
        {"macro_character": 0.76, "macro_movement": 0.62, "macro_coupling": 0.76, "macro_space": 0.70},
        {"macro_character": 0.62, "macro_movement": 0.55, "macro_coupling": 0.66, "macro_space": 0.54},
        {"brightness": 0.88, "warmth": 0.36, "movement": 0.58, "density": 0.50, "space": 0.72, "aggression": 0.09},
        ["entangled", "orphica", "obscura", "resonance", "harp"],
        "Resonance shared between glass-harmonic and OBSCURA's plate — spacious and luminous.",
    ),
    make_preset(
        "Siren Exposure",
        ("ORPHICA", "OBSCURA"),
        COUPLING_TYPES[9], 0.65,
        {"macro_character": 0.65, "macro_movement": 0.52, "macro_coupling": 0.65, "macro_space": 0.76},
        {"macro_character": 0.52, "macro_movement": 0.46, "macro_coupling": 0.56, "macro_space": 0.60},
        {"brightness": 0.85, "warmth": 0.33, "movement": 0.50, "density": 0.40, "space": 0.80, "aggression": 0.06},
        ["entangled", "orphica", "obscura", "spatial", "siren"],
        "Spatial coupling extends ORPHICA's siphonophore ring into OBSCURA's reverberant field — space 0.80.",
    ),
    make_preset(
        "Seafoam Silver",
        ("ORPHICA", "OBSCURA"),
        COUPLING_TYPES[10], 0.78,
        {"macro_character": 0.78, "macro_movement": 0.64, "macro_coupling": 0.78, "macro_space": 0.70},
        {"macro_character": 0.64, "macro_movement": 0.57, "macro_coupling": 0.68, "macro_space": 0.54},
        {"brightness": 0.92, "warmth": 0.34, "movement": 0.60, "density": 0.52, "space": 0.72, "aggression": 0.08},
        ["entangled", "orphica", "obscura", "spectral", "seafoam"],
        "Spectral morphing: OBSCURA's silver plate becomes ORPHICA's resonant canvas — brightness 0.92.",
    ),
    make_preset(
        "Velocity Glass",
        ("ORPHICA", "OBSCURA"),
        COUPLING_TYPES[11], 0.66,
        {"macro_character": 0.66, "macro_movement": 0.52, "macro_coupling": 0.66, "macro_space": 0.74},
        {"macro_character": 0.52, "macro_movement": 0.46, "macro_coupling": 0.58, "macro_space": 0.58},
        {"brightness": 0.86, "warmth": 0.33, "movement": 0.50, "density": 0.42, "space": 0.76, "aggression": 0.06},
        ["entangled", "orphica", "obscura", "velocity", "glass"],
        "Velocity coupling: soft ORPHICA touch reflects; hard strike refracts through OBSCURA.",
    ),
]


def main():
    os.makedirs(PRESET_DIR, exist_ok=True)
    all_presets = OTTONI_PRESETS + OLE_PRESETS + ORPHICA_PRESETS

    # Validate extreme DNA rule
    violations = [p["name"] for p in all_presets if not validate_extreme(p["dna"])]
    if violations:
        print(f"WARNING: {len(violations)} presets lack an extreme DNA dimension:")
        for v in violations:
            print(f"  - {v}")

    saved = 0
    skipped = 0
    for preset in all_presets:
        ok, path = save_preset(preset)
        if ok:
            saved += 1
            print(f"  SAVED  {preset['name']} -> {os.path.basename(path)}")
        else:
            skipped += 1
            print(f"  SKIP   {preset['name']} (exists)")

    print(f"\nDone. {saved} saved, {skipped} skipped. Total attempted: {len(all_presets)}")


if __name__ == "__main__":
    main()
