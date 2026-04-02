#!/usr/bin/env python3
"""OSTINATO Factory Preset Generator — 150 factory presets for The Fire Circle.

Generates .xometa JSON files across 7 moods:
  Foundation (30): World percussion essentials + single-instrument studies
  Atmosphere (25): Ambient environments, sparse rhythms, meditative circles
  Entangled (20): Coupling-ready ensembles with high CIRCLE macro
  Prism (25): Genre production — Afrobeat, Latin, Electronic, Cinematic, World Fusion
  Flux (20): Rhythmic movement — polyrhythmic, evolving, tempo-driven
  Aether (15): Experimental — detuned, extreme processing, beatbox machines
  Family (15): OSTINATO coupled with 15 different fleet engines
"""

import json
import os

PRESET_DIR = os.path.join(os.path.dirname(__file__), "..", "Presets", "XOceanus")

# ── Instrument constants ──────────────────────────────────────────────────────
DJEMBE, DUNDUN, CONGA, BONGOS, CAJON, TAIKO = 0, 1, 2, 3, 4, 5
TABLA, DOUMBEK, FRAMEDRUM, SURDO, TONGDRUM, BEATBOX = 6, 7, 8, 9, 10, 11

# Pattern constants
PAT_BASIC, PAT_VAR, PAT_FILL, PAT_SPARSE = 0, 1, 2, 3
PAT_STYLE_A, PAT_STYLE_B, PAT_STYLE_C, PAT_DOUBLE = 4, 5, 6, 7

# Body model constants (for bodyModel choice: 0=Auto, 1=Cylindrical, 2=Conical, 3=Box, 4=Open)
BM_AUTO, BM_CYLINDER, BM_CONICAL, BM_BOX, BM_OPEN = 0, 1, 2, 3, 4

# Articulation constants (per-instrument, all use 0-3)
ART_1, ART_2, ART_3, ART_4 = 0, 1, 2, 3

# ── OSTINATO defaults ─────────────────────────────────────────────────────────
# Per-seat defaults (8 seats), default instruments: Djembe, Taiko, Conga, Tabla, Cajon, Doumbek, Frame, Surdo
DEF_INSTRUMENTS = [DJEMBE, TAIKO, CONGA, TABLA, CAJON, DOUMBEK, FRAMEDRUM, SURDO]
DEF_PANS = [-0.7, -0.4, -0.1, 0.2, 0.5, 0.3, 0.0, -0.5]

DEFAULTS = {}
for s in range(8):
    pre = f"osti_seat{s+1}_"
    DEFAULTS[pre + "instrument"] = DEF_INSTRUMENTS[s]
    DEFAULTS[pre + "articulation"] = 0
    DEFAULTS[pre + "tuning"] = 0.0
    DEFAULTS[pre + "decay"] = 0.5
    DEFAULTS[pre + "brightness"] = 0.5
    DEFAULTS[pre + "body"] = 0.5
    DEFAULTS[pre + "level"] = 0.7
    DEFAULTS[pre + "pan"] = DEF_PANS[s]
    DEFAULTS[pre + "pattern"] = 0
    DEFAULTS[pre + "patternVol"] = 0.5
    DEFAULTS[pre + "velSens"] = 0.7
    DEFAULTS[pre + "pitchEnv"] = 0.0
    DEFAULTS[pre + "exciterMix"] = 0.5
    DEFAULTS[pre + "bodyModel"] = 0

# Macros
DEFAULTS["osti_macroGather"] = 0.5
DEFAULTS["osti_macroFire"] = 0.5
DEFAULTS["osti_macroCircle"] = 0.0
DEFAULTS["osti_macroSpace"] = 0.0

# Globals
DEFAULTS["osti_tempo"] = 120.0
DEFAULTS["osti_swing"] = 0.0
DEFAULTS["osti_masterTune"] = 0.0
DEFAULTS["osti_masterDecay"] = 1.0
DEFAULTS["osti_masterFilter"] = 18000.0
DEFAULTS["osti_masterReso"] = 0.1
DEFAULTS["osti_reverbSize"] = 0.4
DEFAULTS["osti_reverbDamp"] = 0.3
DEFAULTS["osti_reverbMix"] = 0.15
DEFAULTS["osti_compThresh"] = -12.0
DEFAULTS["osti_compRatio"] = 4.0
DEFAULTS["osti_compAttack"] = 5.0
DEFAULTS["osti_compRelease"] = 50.0
DEFAULTS["osti_circleAmount"] = 0.0
DEFAULTS["osti_humanize"] = 0.3
DEFAULTS["osti_masterLevel"] = 0.8


# ── Helper: build seat overrides compactly ────────────────────────────────────
def seat(num, instrument=None, articulation=None, tuning=None, decay=None,
         brightness=None, body=None, level=None, pan=None, pattern=None,
         patternVol=None, velSens=None, pitchEnv=None, exciterMix=None,
         bodyModel=None):
    """Build param overrides for a single seat (1-indexed)."""
    pre = f"osti_seat{num}_"
    d = {}
    if instrument is not None: d[pre + "instrument"] = instrument
    if articulation is not None: d[pre + "articulation"] = articulation
    if tuning is not None: d[pre + "tuning"] = tuning
    if decay is not None: d[pre + "decay"] = decay
    if brightness is not None: d[pre + "brightness"] = brightness
    if body is not None: d[pre + "body"] = body
    if level is not None: d[pre + "level"] = level
    if pan is not None: d[pre + "pan"] = pan
    if pattern is not None: d[pre + "pattern"] = pattern
    if patternVol is not None: d[pre + "patternVol"] = patternVol
    if velSens is not None: d[pre + "velSens"] = velSens
    if pitchEnv is not None: d[pre + "pitchEnv"] = pitchEnv
    if exciterMix is not None: d[pre + "exciterMix"] = exciterMix
    if bodyModel is not None: d[pre + "bodyModel"] = bodyModel
    return d


def seats(*seat_list):
    """Merge multiple seat() dicts into one overrides dict."""
    d = {}
    for s in seat_list:
        d.update(s)
    return d


def mute_seats(*nums):
    """Set level=0 for specified seat numbers (1-indexed)."""
    d = {}
    for n in nums:
        d[f"osti_seat{n}_level"] = 0.0
    return d


def make_preset(name, mood, overrides, dna, desc="", tags=None, engines=None,
                coupling=None, coupling_intensity="None"):
    """Build a full .xometa dict from name + param overrides."""
    params = dict(DEFAULTS)
    params.update(overrides)

    preset = {
        "schema_version": 1,
        "name": name,
        "mood": mood,
        "engines": engines or ["Ostinato"],
        "author": "XO_OX Designs",
        "version": "1.0.0",
        "description": desc,
        "tags": tags or [],
        "macroLabels": ["GATHER", "FIRE", "CIRCLE", "SPACE"],
        "couplingIntensity": coupling_intensity,
        "tempo": params.get("osti_tempo", 120.0),
        "dna": {
            "brightness": dna[0], "warmth": dna[1], "movement": dna[2],
            "density": dna[3], "space": dna[4], "aggression": dna[5]
        },
        "parameters": {"Ostinato": params},
    }
    if coupling:
        preset["coupling"] = {"pairs": coupling}
    return preset


def save(preset):
    """Write preset to the correct mood directory."""
    mood = preset["mood"]
    safe_name = preset["name"].replace(" ", "_").replace("'", "").replace(",", "")
    filename = f"Ostinato_{safe_name}.xometa"
    path = os.path.join(PRESET_DIR, mood, filename)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w") as f:
        json.dump(preset, f, indent=2)
    return path


# ══════════════════════════════════════════════════════════════════════════════
# FOUNDATION (30) — World percussion essentials + single-instrument studies
# ══════════════════════════════════════════════════════════════════════════════

FOUNDATION = [
    # 1-8: Single instrument spotlights (solo seat studies)
    ("Solo Djembe", {
        **mute_seats(2, 3, 4, 5, 6, 7, 8),
        **seat(1, instrument=DJEMBE, articulation=ART_1, brightness=0.6, decay=0.6, level=0.9),
        "osti_macroGather": 0.7, "osti_macroFire": 0.4,
    }, (0.5, 0.6, 0.2, 0.1, 0.1, 0.3),
     "One djembe, one voice. The foundation of West African rhythm.",
     ["foundation", "djembe", "solo", "west-africa"]),

    ("Solo Taiko", {
        **mute_seats(1, 3, 4, 5, 6, 7, 8),
        **seat(2, instrument=TAIKO, articulation=ART_1, brightness=0.4, decay=0.8, level=0.9,
               body=0.7),
        "osti_macroGather": 0.8, "osti_macroFire": 0.6,
    }, (0.3, 0.7, 0.15, 0.1, 0.2, 0.5),
     "Thunder from a single taiko. Deep body, long decay.",
     ["foundation", "taiko", "solo", "japan"]),

    ("Solo Tabla", {
        **mute_seats(1, 2, 4, 5, 6, 7, 8),
        **seat(3, instrument=TABLA, articulation=ART_2, brightness=0.7, decay=0.5,
               level=0.9, body=0.4),
        "osti_macroGather": 0.7, "osti_macroFire": 0.3,
    }, (0.6, 0.5, 0.2, 0.1, 0.1, 0.2),
     "Tabla tin — the ringing center strike. Harmonic modes sing.",
     ["foundation", "tabla", "solo", "india"]),

    ("Solo Cajon", {
        **mute_seats(1, 2, 3, 5, 6, 7, 8),
        **seat(4, instrument=CAJON, articulation=ART_1, brightness=0.5, decay=0.6,
               level=0.9, body=0.6),
        "osti_macroGather": 0.6, "osti_macroFire": 0.4,
    }, (0.4, 0.6, 0.2, 0.1, 0.15, 0.3),
     "Cajon bass stroke. Box resonance, rectangular modes.",
     ["foundation", "cajon", "solo", "peru"]),

    ("Solo Doumbek", {
        **mute_seats(1, 2, 3, 4, 6, 7, 8),
        **seat(5, instrument=DOUMBEK, articulation=ART_2, brightness=0.7, decay=0.4,
               level=0.9, body=0.4),
        "osti_macroGather": 0.7, "osti_macroFire": 0.3,
    }, (0.6, 0.4, 0.2, 0.1, 0.1, 0.2),
     "Doumbek tek — sharp rim attack, ceramic brilliance.",
     ["foundation", "doumbek", "solo", "middle-east"]),

    ("Solo Frame Drum", {
        **mute_seats(1, 2, 3, 4, 5, 7, 8),
        **seat(6, instrument=FRAMEDRUM, articulation=ART_1, brightness=0.5, decay=0.7,
               level=0.9, body=0.3),
        "osti_macroGather": 0.5, "osti_macroFire": 0.3,
    }, (0.4, 0.5, 0.15, 0.1, 0.2, 0.15),
     "Open frame drum. Minimal body, pure membrane resonance.",
     ["foundation", "frame-drum", "solo", "mediterranean"]),

    ("Solo Tongue Drum", {
        **mute_seats(1, 2, 3, 4, 5, 6, 8),
        **seat(7, instrument=TONGDRUM, articulation=ART_1, brightness=0.5, decay=0.8,
               level=0.9, body=0.3),
        "osti_macroGather": 0.5, "osti_macroFire": 0.2,
    }, (0.5, 0.6, 0.1, 0.1, 0.15, 0.05),
     "Steel tongue drum — harmonic series rings clean and long.",
     ["foundation", "tongue-drum", "solo", "meditative"]),

    ("Solo Beatbox", {
        **mute_seats(1, 2, 3, 4, 5, 6, 7),
        **seat(8, instrument=BEATBOX, articulation=ART_1, brightness=0.6, decay=0.4,
               level=0.9),
        "osti_macroGather": 0.8, "osti_macroFire": 0.5,
    }, (0.5, 0.4, 0.3, 0.15, 0.05, 0.4),
     "Beatbox kick. Synthetic percussion, no tradition but all groove.",
     ["foundation", "beatbox", "solo", "electronic"]),

    # 9-14: Basic ensemble combinations
    ("First Circle", {
        **seat(1, instrument=DJEMBE, pattern=PAT_BASIC),
        **seat(2, instrument=TAIKO, pattern=PAT_BASIC, level=0.6),
        **seat(3, instrument=CONGA, pattern=PAT_VAR),
        **seat(4, instrument=TABLA, pattern=PAT_SPARSE, level=0.5),
        **mute_seats(5, 6, 7, 8),
        "osti_macroGather": 0.6, "osti_macroFire": 0.4,
        "osti_humanize": 0.3,
    }, (0.45, 0.55, 0.3, 0.35, 0.1, 0.25),
     "Four seats. The smallest circle that feels complete.",
     ["foundation", "quartet", "basic", "circle"]),

    ("Full Default", {
        "osti_macroGather": 0.5, "osti_macroFire": 0.5,
    }, (0.5, 0.5, 0.35, 0.55, 0.15, 0.3),
     "All eight seats at defaults. The complete fire circle as designed.",
     ["foundation", "default", "full-circle", "reference"]),

    ("Djembe and Dundun", {
        **seat(1, instrument=DJEMBE, pattern=PAT_BASIC, brightness=0.6, level=0.8),
        **seat(2, instrument=DUNDUN, pattern=PAT_VAR, brightness=0.4, level=0.75),
        **mute_seats(3, 4, 5, 6, 7, 8),
        "osti_macroGather": 0.7, "osti_macroFire": 0.5,
    }, (0.45, 0.6, 0.25, 0.2, 0.1, 0.3),
     "The classic West African pair. Djembe leads, dundun anchors.",
     ["foundation", "djembe", "dundun", "pair", "west-africa"]),

    ("Conga and Bongos", {
        **seat(1, instrument=CONGA, articulation=ART_1, pattern=PAT_BASIC, level=0.8),
        **seat(2, instrument=BONGOS, articulation=ART_1, pattern=PAT_VAR, level=0.75,
               tuning=2.0),
        **mute_seats(3, 4, 5, 6, 7, 8),
        "osti_macroGather": 0.6, "osti_macroFire": 0.4,
        "osti_swing": 20.0,
    }, (0.5, 0.55, 0.3, 0.2, 0.1, 0.2),
     "Cuban rhythm pair. Conga grounds, bongos dance above.",
     ["foundation", "conga", "bongos", "latin", "pair"]),

    ("Tabla Duo", {
        **seat(1, instrument=TABLA, articulation=ART_2, pattern=PAT_BASIC, level=0.8,
               brightness=0.6),
        **seat(2, instrument=TABLA, articulation=ART_4, pattern=PAT_VAR, level=0.7,
               tuning=-5.0, brightness=0.3),
        **mute_seats(3, 4, 5, 6, 7, 8),
        "osti_macroGather": 0.7, "osti_macroFire": 0.3,
    }, (0.55, 0.5, 0.25, 0.2, 0.1, 0.15),
     "Tabla and bayan — the bright dayan and deep bayan pair.",
     ["foundation", "tabla", "pair", "india", "classical"]),

    ("Surdo Foundation", {
        **seat(1, instrument=SURDO, pattern=PAT_BASIC, level=0.85, brightness=0.3),
        **seat(2, instrument=SURDO, pattern=PAT_VAR, level=0.7, tuning=5.0,
               articulation=ART_2),
        **seat(3, instrument=SURDO, pattern=PAT_SPARSE, level=0.6, tuning=12.0,
               articulation=ART_3),
        **mute_seats(4, 5, 6, 7, 8),
        "osti_macroGather": 0.8, "osti_macroFire": 0.6,
        "osti_tempo": 100.0,
    }, (0.25, 0.7, 0.2, 0.25, 0.15, 0.4),
     "Three surdos — low, mid, high. The heartbeat of samba.",
     ["foundation", "surdo", "trio", "brazil", "samba"]),

    # 15-22: Articulation and parameter studies
    ("Bright Circle", {
        **seats(*[seat(s+1, brightness=0.85) for s in range(8)]),
        "osti_masterFilter": 20000.0, "osti_macroFire": 0.7,
    }, (0.8, 0.35, 0.35, 0.55, 0.1, 0.35),
     "All seats at maximum brightness. The fire burns high.",
     ["foundation", "bright", "study", "fire"]),

    ("Dark Circle", {
        **seats(*[seat(s+1, brightness=0.15) for s in range(8)]),
        "osti_masterFilter": 4000.0, "osti_macroFire": 0.2,
    }, (0.15, 0.75, 0.3, 0.55, 0.1, 0.15),
     "All seats dark and muffled. The fire in embers.",
     ["foundation", "dark", "study", "muffled"]),

    ("Long Decay", {
        **seats(*[seat(s+1, decay=1.8) for s in range(8)]),
        "osti_masterDecay": 1.8, "osti_macroFire": 0.3,
    }, (0.45, 0.55, 0.25, 0.55, 0.3, 0.15),
     "Maximum decay on every seat. Strokes ring into each other.",
     ["foundation", "decay", "study", "resonant"]),

    ("Tight and Dry", {
        **seats(*[seat(s+1, decay=0.08, brightness=0.6) for s in range(8)]),
        "osti_masterDecay": 0.6, "osti_reverbMix": 0.0,
        "osti_macroGather": 0.9, "osti_macroFire": 0.6,
        "osti_macroSpace": 0.0,
    }, (0.55, 0.4, 0.35, 0.55, 0.0, 0.4),
     "Short decay, no reverb, tight gather. Precision percussion.",
     ["foundation", "tight", "dry", "study"]),

    ("Body Resonance", {
        **seats(*[seat(s+1, body=0.9) for s in range(8)]),
        "osti_macroFire": 0.4,
    }, (0.4, 0.65, 0.3, 0.55, 0.2, 0.2),
     "Maximum body resonance on every seat. The drums become rooms.",
     ["foundation", "body", "resonance", "study"]),

    ("Slap Circle", {
        **seat(1, instrument=DJEMBE, articulation=ART_2, brightness=0.8),
        **seat(2, instrument=CONGA, articulation=ART_2, brightness=0.8),
        **seat(3, instrument=DOUMBEK, articulation=ART_2, brightness=0.8),
        **seat(4, instrument=CAJON, articulation=ART_2, brightness=0.75),
        **seat(5, instrument=FRAMEDRUM, articulation=ART_4, brightness=0.8),
        **mute_seats(6, 7, 8),
        "osti_macroGather": 0.7, "osti_macroFire": 0.6,
    }, (0.7, 0.35, 0.3, 0.4, 0.1, 0.45),
     "All slap articulations. Bright, sharp, percussive edge.",
     ["foundation", "slap", "articulation", "bright"]),

    ("Bass Circle", {
        **seat(1, instrument=DJEMBE, articulation=ART_3, brightness=0.2),
        **seat(2, instrument=TAIKO, articulation=ART_1, brightness=0.25),
        **seat(3, instrument=SURDO, articulation=ART_1, brightness=0.2),
        **seat(4, instrument=CAJON, articulation=ART_1, brightness=0.15),
        **seat(5, instrument=DUNDUN, articulation=ART_1, brightness=0.2),
        **mute_seats(6, 7, 8),
        "osti_masterFilter": 6000.0, "osti_macroFire": 0.6,
    }, (0.15, 0.8, 0.25, 0.4, 0.15, 0.45),
     "All bass articulations. Low, dark, powerful. Chest hits.",
     ["foundation", "bass", "articulation", "deep"]),

    # 23-26: Macro studies
    ("Gather Study", {
        "osti_macroGather": 0.0, "osti_humanize": 0.7,
        **seats(*[seat(s+1, pattern=PAT_VAR) for s in range(8)]),
    }, (0.5, 0.5, 0.45, 0.55, 0.15, 0.2),
     "GATHER at zero — maximum looseness. Push the macro to hear quantization tighten.",
     ["foundation", "gather", "macro", "study"]),

    ("Fire Study", {
        "osti_macroFire": 0.0,
    }, (0.35, 0.6, 0.3, 0.55, 0.15, 0.15),
     "FIRE at zero — gentle and quiet. Push the macro to hear intensity build.",
     ["foundation", "fire", "macro", "study"]),

    ("Circle Study", {
        "osti_circleAmount": 0.6, "osti_macroCircle": 0.7,
    }, (0.5, 0.5, 0.4, 0.6, 0.15, 0.25),
     "CIRCLE active — seats trigger sympathetic resonance in neighbors.",
     ["foundation", "circle", "macro", "study", "sympathetic"]),

    ("Space Study", {
        "osti_macroSpace": 0.8, "osti_reverbSize": 0.7,
        "osti_reverbMix": 0.4, "osti_reverbDamp": 0.2,
    }, (0.45, 0.5, 0.3, 0.55, 0.7, 0.2),
     "SPACE macro high — the circle in a cathedral. Reverb and distance.",
     ["foundation", "space", "macro", "study", "reverb"]),

    # 27-30: Tuning and tempo explorations
    ("Tuned Bells", {
        **seat(1, instrument=TONGDRUM, tuning=0.0, brightness=0.6, decay=1.5),
        **seat(2, instrument=TONGDRUM, tuning=7.0, brightness=0.6, decay=1.5),
        **seat(3, instrument=TONGDRUM, tuning=5.0, brightness=0.6, decay=1.5),
        **seat(4, instrument=TONGDRUM, tuning=12.0, brightness=0.6, decay=1.5),
        **mute_seats(5, 6, 7, 8),
        "osti_macroGather": 0.5, "osti_macroFire": 0.2,
        "osti_reverbMix": 0.3, "osti_reverbSize": 0.5,
    }, (0.6, 0.55, 0.2, 0.3, 0.4, 0.05),
     "Four tongue drums tuned to a chord. Melodic percussion.",
     ["foundation", "tongue-drum", "tuned", "melodic", "bells"]),

    ("Pitch Envelope", {
        **seats(*[seat(s+1, pitchEnv=0.6, brightness=0.6) for s in range(8)]),
        "osti_macroFire": 0.5,
    }, (0.55, 0.45, 0.35, 0.55, 0.1, 0.3),
     "Positive pitch envelope on every seat. Strikes swoop downward.",
     ["foundation", "pitch-env", "study", "sweep"]),

    ("Rush Hour", {
        "osti_tempo": 180.0, "osti_macroGather": 0.85,
        "osti_macroFire": 0.7, "osti_compThresh": -18.0,
        "osti_compRatio": 6.0,
        **seats(*[seat(s+1, pattern=PAT_DOUBLE) for s in range(8)]),
    }, (0.55, 0.4, 0.7, 0.7, 0.1, 0.5),
     "180 BPM, double patterns, tight gather. Maximum velocity.",
     ["foundation", "fast", "tempo", "intense"]),

    ("Slow Ceremony", {
        "osti_tempo": 60.0, "osti_macroGather": 0.3,
        "osti_humanize": 0.6, "osti_macroFire": 0.3,
        **seat(1, instrument=TAIKO, pattern=PAT_SPARSE, decay=1.5, level=0.85),
        **seat(2, instrument=FRAMEDRUM, pattern=PAT_SPARSE, decay=1.2, level=0.6),
        **seat(3, instrument=SURDO, pattern=PAT_SPARSE, decay=1.0, level=0.5),
        **mute_seats(4, 5, 6, 7, 8),
        "osti_reverbMix": 0.3, "osti_reverbSize": 0.6,
    }, (0.35, 0.65, 0.1, 0.2, 0.45, 0.2),
     "60 BPM, sparse patterns, loose timing. A slow ritual.",
     ["foundation", "slow", "ceremony", "sparse"]),

    ("Exciter Study", {
        **seat(1, instrument=DJEMBE, exciterMix=0.0, level=0.7, pattern=PAT_BASIC),
        **seat(2, instrument=DJEMBE, exciterMix=0.25, level=0.7, pattern=PAT_BASIC,
               tuning=3.0),
        **seat(3, instrument=DJEMBE, exciterMix=0.75, level=0.7, pattern=PAT_BASIC,
               tuning=5.0),
        **seat(4, instrument=DJEMBE, exciterMix=1.0, level=0.7, pattern=PAT_BASIC,
               tuning=7.0),
        **mute_seats(5, 6, 7, 8),
        "osti_macroGather": 0.7, "osti_macroFire": 0.4,
    }, (0.5, 0.5, 0.25, 0.3, 0.1, 0.2),
     "Four djembes with different exciter mixes. Noise to pitched across the circle.",
     ["foundation", "exciter", "study", "comparison"]),
]


# ══════════════════════════════════════════════════════════════════════════════
# ATMOSPHERE (25) — Ambient, meditative, environmental circles
# ══════════════════════════════════════════════════════════════════════════════

ATMOSPHERE = [
    ("Midnight Embers", {
        **seat(1, instrument=FRAMEDRUM, pattern=PAT_SPARSE, brightness=0.3,
               decay=1.2, level=0.6),
        **seat(2, instrument=TONGDRUM, pattern=PAT_SPARSE, brightness=0.4,
               decay=1.5, level=0.5, tuning=5.0),
        **mute_seats(3, 4, 5, 6, 7, 8),
        "osti_tempo": 55.0, "osti_macroGather": 0.2, "osti_humanize": 0.7,
        "osti_macroFire": 0.15, "osti_macroSpace": 0.6,
        "osti_reverbMix": 0.45, "osti_reverbSize": 0.7, "osti_reverbDamp": 0.2,
    }, (0.3, 0.65, 0.15, 0.15, 0.7, 0.05),
     "Two voices in the dark. The fire is almost out.",
     ["atmosphere", "sparse", "night", "meditative"]),

    ("Rain on Skin", {
        **seat(1, instrument=FRAMEDRUM, articulation=ART_3, brightness=0.6,
               decay=0.3, level=0.4, pattern=PAT_FILL),
        **seat(2, instrument=FRAMEDRUM, articulation=ART_2, brightness=0.5,
               decay=0.4, level=0.35, pattern=PAT_VAR),
        **seat(3, instrument=BONGOS, articulation=ART_3, brightness=0.7,
               decay=0.15, level=0.3, pattern=PAT_FILL),
        **mute_seats(4, 5, 6, 7, 8),
        "osti_tempo": 140.0, "osti_macroGather": 0.3, "osti_humanize": 0.8,
        "osti_macroFire": 0.15, "osti_masterFilter": 8000.0,
    }, (0.5, 0.45, 0.5, 0.4, 0.2, 0.05),
     "Finger rolls and light taps. Rain pattering on drum skins.",
     ["atmosphere", "rain", "gentle", "texture"]),

    ("Desert Caravan", {
        **seat(1, instrument=DOUMBEK, pattern=PAT_STYLE_A, brightness=0.6,
               level=0.75),
        **seat(2, instrument=FRAMEDRUM, pattern=PAT_SPARSE, brightness=0.5,
               decay=0.8, level=0.5),
        **seat(3, instrument=TABLA, pattern=PAT_SPARSE, brightness=0.5,
               level=0.45, tuning=2.0),
        **mute_seats(4, 5, 6, 7, 8),
        "osti_tempo": 80.0, "osti_swing": 30.0, "osti_humanize": 0.5,
        "osti_macroGather": 0.4, "osti_macroSpace": 0.4,
        "osti_reverbMix": 0.3, "osti_reverbSize": 0.55,
    }, (0.5, 0.5, 0.25, 0.25, 0.45, 0.15),
     "Three voices crossing the sand. Doumbek leads, frame and tabla follow.",
     ["atmosphere", "desert", "middle-east", "caravan"]),

    ("Temple Dawn", {
        **seat(1, instrument=TONGDRUM, tuning=0.0, decay=1.8, brightness=0.5,
               pattern=PAT_SPARSE, level=0.7),
        **seat(2, instrument=TONGDRUM, tuning=7.0, decay=1.8, brightness=0.5,
               pattern=PAT_SPARSE, level=0.6),
        **seat(3, instrument=TONGDRUM, tuning=12.0, decay=1.8, brightness=0.5,
               pattern=PAT_SPARSE, level=0.5),
        **mute_seats(4, 5, 6, 7, 8),
        "osti_tempo": 45.0, "osti_macroGather": 0.3, "osti_macroFire": 0.15,
        "osti_macroSpace": 0.7,
        "osti_reverbMix": 0.5, "osti_reverbSize": 0.8, "osti_reverbDamp": 0.15,
    }, (0.45, 0.6, 0.1, 0.2, 0.8, 0.0),
     "Tongue drums ringing in a stone temple. Sacred harmonics at sunrise.",
     ["atmosphere", "temple", "meditative", "harmonic"]),

    ("Forest Floor", {
        **seat(1, instrument=DJEMBE, articulation=ART_4, pattern=PAT_SPARSE,
               brightness=0.3, decay=0.2, level=0.35),
        **seat(2, instrument=CAJON, articulation=ART_3, pattern=PAT_FILL,
               brightness=0.4, decay=0.15, level=0.3),
        **seat(3, instrument=BONGOS, articulation=ART_3, pattern=PAT_SPARSE,
               brightness=0.5, decay=0.1, level=0.25),
        **seat(4, instrument=FRAMEDRUM, articulation=ART_3, pattern=PAT_VAR,
               brightness=0.55, decay=0.2, level=0.3),
        **mute_seats(5, 6, 7, 8),
        "osti_tempo": 100.0, "osti_macroGather": 0.2, "osti_humanize": 0.9,
        "osti_macroFire": 0.1, "osti_masterFilter": 6000.0,
    }, (0.35, 0.55, 0.35, 0.35, 0.15, 0.05),
     "Ghost notes and muted touches. Things moving in the undergrowth.",
     ["atmosphere", "forest", "ghost-notes", "subtle"]),

    ("Underwater Drums", {
        **seats(*[seat(s+1, brightness=0.15, body=0.9, decay=1.2) for s in range(4)]),
        **mute_seats(5, 6, 7, 8),
        "osti_masterFilter": 2000.0, "osti_masterReso": 0.4,
        "osti_macroFire": 0.3, "osti_macroSpace": 0.5,
        "osti_reverbMix": 0.4, "osti_reverbSize": 0.6,
        "osti_tempo": 85.0,
    }, (0.1, 0.8, 0.25, 0.35, 0.55, 0.1),
     "Everything filtered deep. Drums heard from beneath the surface.",
     ["atmosphere", "underwater", "filtered", "deep"]),

    ("Mountain Echo", {
        **seat(1, instrument=TAIKO, pattern=PAT_SPARSE, decay=1.5, level=0.8,
               brightness=0.5),
        **mute_seats(2, 3, 4, 5, 6, 7, 8),
        "osti_tempo": 50.0, "osti_macroGather": 0.5,
        "osti_macroSpace": 0.9,
        "osti_reverbMix": 0.6, "osti_reverbSize": 0.85, "osti_reverbDamp": 0.1,
        "osti_macroFire": 0.5,
    }, (0.4, 0.55, 0.1, 0.1, 0.9, 0.3),
     "Single taiko in vast mountain reverb. Each stroke echoes forever.",
     ["atmosphere", "mountain", "echo", "taiko", "vast"]),

    ("Campfire Whisper", {
        **seat(1, instrument=DJEMBE, articulation=ART_1, pattern=PAT_SPARSE,
               brightness=0.4, level=0.5, decay=0.6),
        **seat(2, instrument=CONGA, articulation=ART_4, pattern=PAT_SPARSE,
               brightness=0.5, level=0.4, decay=0.3),
        **seat(3, instrument=DOUMBEK, articulation=ART_3, pattern=PAT_SPARSE,
               brightness=0.5, level=0.35, decay=0.3),
        **mute_seats(4, 5, 6, 7, 8),
        "osti_tempo": 70.0, "osti_macroGather": 0.3, "osti_humanize": 0.6,
        "osti_macroFire": 0.15, "osti_macroSpace": 0.3,
        "osti_reverbMix": 0.25, "osti_masterLevel": 0.6,
    }, (0.4, 0.55, 0.2, 0.2, 0.35, 0.05),
     "Quiet conversation between drums. The circle talks softly.",
     ["atmosphere", "campfire", "quiet", "intimate"]),

    ("Tide Rhythm", {
        **seat(1, instrument=SURDO, pattern=PAT_BASIC, brightness=0.25,
               decay=1.0, level=0.6),
        **seat(2, instrument=FRAMEDRUM, articulation=ART_3, pattern=PAT_VAR,
               brightness=0.4, decay=0.6, level=0.4),
        **mute_seats(3, 4, 5, 6, 7, 8),
        "osti_tempo": 40.0, "osti_macroGather": 0.2, "osti_humanize": 0.7,
        "osti_macroSpace": 0.5, "osti_macroFire": 0.2,
        "osti_reverbMix": 0.35, "osti_reverbSize": 0.6,
    }, (0.25, 0.65, 0.15, 0.15, 0.55, 0.1),
     "Surdo pulse like waves. Frame drum rolls like foam. The ocean breathes.",
     ["atmosphere", "tide", "ocean", "slow"]),

    ("Monsoon Drums", {
        **seat(1, instrument=TABLA, pattern=PAT_FILL, brightness=0.6,
               decay=0.4, level=0.6),
        **seat(2, instrument=DOUMBEK, pattern=PAT_FILL, brightness=0.65,
               decay=0.3, level=0.55),
        **seat(3, instrument=BONGOS, pattern=PAT_FILL, brightness=0.55,
               decay=0.25, level=0.45),
        **seat(4, instrument=FRAMEDRUM, articulation=ART_3, pattern=PAT_FILL,
               brightness=0.5, decay=0.3, level=0.4),
        **mute_seats(5, 6, 7, 8),
        "osti_tempo": 150.0, "osti_macroGather": 0.25, "osti_humanize": 0.8,
        "osti_macroFire": 0.3, "osti_masterFilter": 10000.0,
    }, (0.5, 0.45, 0.6, 0.5, 0.2, 0.15),
     "Dense fill patterns with loose timing. A downpour of percussion.",
     ["atmosphere", "monsoon", "dense", "rain"]),

    ("Starlight Pulse", {
        **seat(1, instrument=TONGDRUM, pattern=PAT_SPARSE, decay=2.0,
               brightness=0.6, level=0.5, tuning=0.0),
        **seat(2, instrument=TONGDRUM, pattern=PAT_SPARSE, decay=2.0,
               brightness=0.6, level=0.45, tuning=4.0),
        **seat(3, instrument=TONGDRUM, pattern=PAT_SPARSE, decay=2.0,
               brightness=0.6, level=0.4, tuning=7.0),
        **seat(4, instrument=TONGDRUM, pattern=PAT_SPARSE, decay=2.0,
               brightness=0.6, level=0.35, tuning=12.0),
        **mute_seats(5, 6, 7, 8),
        "osti_tempo": 40.0, "osti_macroGather": 0.15, "osti_humanize": 0.5,
        "osti_macroSpace": 0.8,
        "osti_reverbMix": 0.55, "osti_reverbSize": 0.8,
    }, (0.55, 0.55, 0.1, 0.25, 0.85, 0.0),
     "Tuned tongue drums in wide reverb. Each note a point of light.",
     ["atmosphere", "stars", "harmonic", "ambient"]),

    ("Cave Ceremony", {
        **seat(1, instrument=DJEMBE, articulation=ART_3, pattern=PAT_SPARSE,
               decay=1.0, level=0.7, body=0.8),
        **seat(2, instrument=SURDO, pattern=PAT_SPARSE, decay=1.2, level=0.6,
               body=0.9, brightness=0.2),
        **mute_seats(3, 4, 5, 6, 7, 8),
        "osti_tempo": 55.0, "osti_macroGather": 0.4,
        "osti_macroSpace": 0.7, "osti_macroFire": 0.4,
        "osti_reverbMix": 0.5, "osti_reverbSize": 0.75, "osti_reverbDamp": 0.15,
    }, (0.2, 0.7, 0.15, 0.15, 0.75, 0.25),
     "Bass drums in a stone cave. Body resonance fills the darkness.",
     ["atmosphere", "cave", "bass", "ritual"]),

    ("Wind Chimes", {
        **seat(1, instrument=TONGDRUM, articulation=ART_3, tuning=0.0,
               decay=1.8, brightness=0.75, level=0.4, pattern=PAT_FILL),
        **seat(2, instrument=TONGDRUM, articulation=ART_3, tuning=3.0,
               decay=1.8, brightness=0.75, level=0.35, pattern=PAT_FILL),
        **seat(3, instrument=TONGDRUM, articulation=ART_3, tuning=7.0,
               decay=1.8, brightness=0.75, level=0.3, pattern=PAT_FILL),
        **seat(4, instrument=TONGDRUM, articulation=ART_3, tuning=10.0,
               decay=1.8, brightness=0.75, level=0.25, pattern=PAT_FILL),
        **mute_seats(5, 6, 7, 8),
        "osti_tempo": 200.0, "osti_macroGather": 0.1, "osti_humanize": 0.9,
        "osti_macroFire": 0.1, "osti_macroSpace": 0.6,
        "osti_reverbMix": 0.45, "osti_reverbSize": 0.65,
        "osti_masterLevel": 0.55,
    }, (0.7, 0.45, 0.4, 0.4, 0.6, 0.0),
     "Harmonic overtones scattered by wind. Maximum randomness.",
     ["atmosphere", "chimes", "random", "harmonic"]),

    ("Heartbeat", {
        **seat(1, instrument=SURDO, articulation=ART_1, pattern=PAT_BASIC,
               brightness=0.2, decay=0.5, level=0.8, body=0.8),
        **mute_seats(2, 3, 4, 5, 6, 7, 8),
        "osti_tempo": 72.0, "osti_macroGather": 0.9,
        "osti_macroFire": 0.3, "osti_masterFilter": 3000.0,
    }, (0.1, 0.75, 0.1, 0.1, 0.1, 0.15),
     "One deep surdo. 72 BPM. A human heartbeat in percussion.",
     ["atmosphere", "heartbeat", "minimal", "primal"]),

    ("Frozen Lake", {
        **seat(1, instrument=TONGDRUM, tuning=0.0, decay=2.0, brightness=0.65,
               level=0.5, pattern=PAT_SPARSE, bodyModel=BM_OPEN),
        **seat(2, instrument=DUNDUN, articulation=ART_3, tuning=12.0, decay=1.5,
               brightness=0.7, level=0.4, pattern=PAT_SPARSE),
        **mute_seats(3, 4, 5, 6, 7, 8),
        "osti_tempo": 45.0, "osti_macroGather": 0.3,
        "osti_macroSpace": 0.7, "osti_macroFire": 0.15,
        "osti_reverbMix": 0.5, "osti_reverbSize": 0.75, "osti_reverbDamp": 0.1,
    }, (0.55, 0.4, 0.1, 0.15, 0.75, 0.0),
     "Crystalline tongue drum and metallic bell over cold reverb.",
     ["atmosphere", "frozen", "bell", "crystalline"]),

    ("Breath Cycle", {
        **seat(1, instrument=FRAMEDRUM, articulation=ART_3, pattern=PAT_BASIC,
               brightness=0.45, decay=0.8, level=0.55),
        **seat(2, instrument=DJEMBE, articulation=ART_3, pattern=PAT_SPARSE,
               brightness=0.3, decay=0.7, level=0.45),
        **mute_seats(3, 4, 5, 6, 7, 8),
        "osti_tempo": 50.0, "osti_macroGather": 0.2, "osti_humanize": 0.6,
        "osti_macroFire": 0.2, "osti_macroSpace": 0.4,
        "osti_reverbMix": 0.3,
    }, (0.3, 0.6, 0.15, 0.15, 0.45, 0.05),
     "Frame drum rolls like inhale, djembe bass like exhale. A breathing circle.",
     ["atmosphere", "breath", "organic", "cycle"]),

    ("Silk Road Dusk", {
        **seat(1, instrument=DOUMBEK, articulation=ART_1, pattern=PAT_STYLE_A,
               brightness=0.5, level=0.65),
        **seat(2, instrument=TABLA, articulation=ART_1, pattern=PAT_SPARSE,
               brightness=0.55, level=0.5, tuning=3.0),
        **seat(3, instrument=FRAMEDRUM, pattern=PAT_SPARSE, decay=0.9,
               level=0.4, brightness=0.4),
        **mute_seats(4, 5, 6, 7, 8),
        "osti_tempo": 75.0, "osti_swing": 25.0, "osti_humanize": 0.5,
        "osti_macroGather": 0.35, "osti_macroSpace": 0.5,
        "osti_reverbMix": 0.35, "osti_reverbSize": 0.55,
    }, (0.5, 0.55, 0.25, 0.25, 0.5, 0.1),
     "Doumbek, tabla, and frame drum meet at a crossroads. East of everything.",
     ["atmosphere", "silk-road", "crossroads", "dusk"]),

    ("Ghost Circle", {
        **seats(*[seat(s+1, level=0.2, brightness=0.3, decay=0.8,
                       pattern=PAT_SPARSE) for s in range(8)]),
        "osti_tempo": 90.0, "osti_macroGather": 0.2, "osti_humanize": 0.7,
        "osti_macroFire": 0.1, "osti_masterFilter": 5000.0,
        "osti_macroSpace": 0.5, "osti_reverbMix": 0.4,
        "osti_masterLevel": 0.5,
    }, (0.2, 0.6, 0.3, 0.4, 0.5, 0.05),
     "All eight seats barely audible. A phantom circle around a dying fire.",
     ["atmosphere", "ghost", "quiet", "all-seats"]),

    ("Volcanic Rumble", {
        **seat(1, instrument=TAIKO, articulation=ART_1, decay=1.5, brightness=0.2,
               body=0.9, level=0.85, pattern=PAT_BASIC),
        **seat(2, instrument=SURDO, articulation=ART_1, decay=1.3, brightness=0.15,
               body=0.9, level=0.75, pattern=PAT_VAR),
        **seat(3, instrument=DUNDUN, articulation=ART_1, decay=1.0, brightness=0.2,
               body=0.85, level=0.65, pattern=PAT_SPARSE),
        **mute_seats(4, 5, 6, 7, 8),
        "osti_tempo": 65.0, "osti_macroFire": 0.7,
        "osti_masterFilter": 3000.0, "osti_compThresh": -20.0,
        "osti_compRatio": 8.0,
    }, (0.1, 0.8, 0.2, 0.25, 0.2, 0.55),
     "Low drums with maximum body. The earth trembles.",
     ["atmosphere", "volcanic", "deep", "rumble"]),

    ("Oasis Shimmer", {
        **seat(1, instrument=TABLA, articulation=ART_2, pattern=PAT_STYLE_B,
               brightness=0.7, decay=0.6, level=0.6),
        **seat(2, instrument=DOUMBEK, articulation=ART_4, pattern=PAT_SPARSE,
               brightness=0.8, decay=0.3, level=0.45),
        **seat(3, instrument=TONGDRUM, articulation=ART_3, pattern=PAT_SPARSE,
               brightness=0.7, decay=1.5, level=0.4, tuning=7.0),
        **mute_seats(4, 5, 6, 7, 8),
        "osti_tempo": 80.0, "osti_macroGather": 0.4, "osti_humanize": 0.4,
        "osti_macroSpace": 0.5,
        "osti_reverbMix": 0.35, "osti_reverbSize": 0.5,
    }, (0.65, 0.5, 0.25, 0.25, 0.5, 0.05),
     "Bright tabla and doumbek with harmonic tongue drum. Desert mirage.",
     ["atmosphere", "oasis", "bright", "shimmer"]),

    ("Harbor Fog", {
        **seat(1, instrument=SURDO, pattern=PAT_SPARSE, decay=1.5,
               brightness=0.2, level=0.6, body=0.8),
        **seat(2, instrument=DUNDUN, articulation=ART_3, pattern=PAT_SPARSE,
               decay=0.8, brightness=0.6, level=0.4),
        **mute_seats(3, 4, 5, 6, 7, 8),
        "osti_tempo": 50.0, "osti_macroGather": 0.3,
        "osti_masterFilter": 6000.0, "osti_macroSpace": 0.6,
        "osti_reverbMix": 0.45, "osti_reverbSize": 0.65, "osti_reverbDamp": 0.2,
        "osti_humanize": 0.5,
    }, (0.3, 0.6, 0.1, 0.15, 0.6, 0.1),
     "Deep surdo fog horn. Dundun bell in the distance.",
     ["atmosphere", "harbor", "fog", "distant"]),

    ("Moonlit Courtyard", {
        **seat(1, instrument=DOUMBEK, pattern=PAT_STYLE_A, brightness=0.55,
               level=0.6, decay=0.5),
        **seat(2, instrument=FRAMEDRUM, articulation=ART_2, pattern=PAT_SPARSE,
               brightness=0.5, level=0.45, decay=0.7),
        **seat(3, instrument=CONGA, articulation=ART_4, pattern=PAT_SPARSE,
               brightness=0.5, level=0.35, decay=0.4),
        **mute_seats(4, 5, 6, 7, 8),
        "osti_tempo": 85.0, "osti_swing": 15.0,
        "osti_macroGather": 0.4, "osti_humanize": 0.5,
        "osti_macroSpace": 0.5, "osti_macroFire": 0.25,
        "osti_reverbMix": 0.35, "osti_reverbSize": 0.5,
    }, (0.45, 0.55, 0.25, 0.25, 0.5, 0.1),
     "Three voices in an open courtyard. Moonlight on drum skins.",
     ["atmosphere", "courtyard", "night", "elegant"]),

    ("Fjord Stillness", {
        **seat(1, instrument=TONGDRUM, tuning=0.0, decay=2.0, brightness=0.55,
               level=0.5, pattern=PAT_SPARSE, bodyModel=BM_OPEN),
        **seat(2, instrument=FRAMEDRUM, articulation=ART_2, pattern=PAT_SPARSE,
               decay=1.0, brightness=0.45, level=0.4),
        **mute_seats(3, 4, 5, 6, 7, 8),
        "osti_tempo": 42.0, "osti_macroGather": 0.2, "osti_humanize": 0.6,
        "osti_macroSpace": 0.8, "osti_macroFire": 0.1,
        "osti_reverbMix": 0.55, "osti_reverbSize": 0.8,
    }, (0.4, 0.5, 0.1, 0.1, 0.8, 0.0),
     "Still water between mountains. Tongue drum and frame drum barely touch.",
     ["atmosphere", "fjord", "still", "vast"]),

    ("Bazaar Echoes", {
        **seat(1, instrument=DOUMBEK, pattern=PAT_STYLE_B, brightness=0.6,
               level=0.65),
        **seat(2, instrument=TABLA, articulation=ART_1, pattern=PAT_STYLE_A,
               brightness=0.65, level=0.55),
        **seat(3, instrument=BONGOS, pattern=PAT_SPARSE, brightness=0.55,
               level=0.4),
        **seat(4, instrument=FRAMEDRUM, articulation=ART_4, pattern=PAT_SPARSE,
               brightness=0.6, level=0.35),
        **mute_seats(5, 6, 7, 8),
        "osti_tempo": 90.0, "osti_swing": 20.0, "osti_humanize": 0.5,
        "osti_macroGather": 0.4, "osti_macroSpace": 0.4,
        "osti_reverbMix": 0.3, "osti_reverbSize": 0.45,
    }, (0.55, 0.5, 0.3, 0.35, 0.4, 0.15),
     "Four traditions echoing through narrow market streets.",
     ["atmosphere", "bazaar", "market", "echoes"]),

    ("Tundra Dawn", {
        **seat(1, instrument=SURDO, pattern=PAT_SPARSE, decay=1.5,
               brightness=0.15, level=0.6, body=0.9),
        **seat(2, instrument=DUNDUN, articulation=ART_1, pattern=PAT_SPARSE,
               decay=1.0, brightness=0.2, level=0.45),
        **mute_seats(3, 4, 5, 6, 7, 8),
        "osti_tempo": 48.0, "osti_macroGather": 0.3,
        "osti_macroFire": 0.15, "osti_masterFilter": 3000.0,
        "osti_macroSpace": 0.6,
        "osti_reverbMix": 0.45, "osti_reverbSize": 0.7, "osti_reverbDamp": 0.15,
    }, (0.1, 0.7, 0.1, 0.1, 0.65, 0.1),
     "Deep muffled drums across frozen plain. First light on ice.",
     ["atmosphere", "tundra", "frozen", "dawn"]),
]


# ══════════════════════════════════════════════════════════════════════════════
# PRISM (25) — Genre production — Afrobeat, Latin, Electronic, Cinematic, Fusion
# ══════════════════════════════════════════════════════════════════════════════

PRISM = [
    # --- Afrobeat (5) ---
    ("Afrobeat Drive", {
        **seat(1, instrument=DJEMBE, pattern=PAT_STYLE_A, brightness=0.6,
               level=0.8, velSens=0.8),
        **seat(2, instrument=DUNDUN, pattern=PAT_BASIC, brightness=0.4,
               level=0.7),
        **seat(3, instrument=CONGA, pattern=PAT_VAR, brightness=0.55,
               level=0.7),
        **seat(4, instrument=DJEMBE, articulation=ART_2, pattern=PAT_STYLE_B,
               brightness=0.7, level=0.5),
        **seat(5, instrument=DUNDUN, articulation=ART_3, pattern=PAT_BASIC,
               brightness=0.7, level=0.45),
        **mute_seats(6, 7, 8),
        "osti_tempo": 115.0, "osti_swing": 15.0,
        "osti_macroGather": 0.65, "osti_macroFire": 0.6,
        "osti_humanize": 0.35,
    }, (0.55, 0.5, 0.5, 0.55, 0.1, 0.4),
     "Djembe, dundun, and conga in Afrobeat pocket. Bell pattern anchors.",
     ["prism", "afrobeat", "west-africa", "groove"]),

    ("Lagos Heat", {
        **seat(1, instrument=DJEMBE, pattern=PAT_STYLE_B, brightness=0.65,
               level=0.8),
        **seat(2, instrument=CONGA, pattern=PAT_STYLE_A, brightness=0.6,
               level=0.7),
        **seat(3, instrument=BONGOS, pattern=PAT_VAR, brightness=0.65,
               level=0.55, tuning=3.0),
        **seat(4, instrument=DUNDUN, articulation=ART_3, pattern=PAT_BASIC,
               brightness=0.7, level=0.5),
        **mute_seats(5, 6, 7, 8),
        "osti_tempo": 125.0, "osti_macroGather": 0.7, "osti_macroFire": 0.65,
        "osti_compThresh": -15.0,
    }, (0.6, 0.45, 0.55, 0.45, 0.1, 0.45),
     "High-energy Afrobeat with bell pattern and conga fills.",
     ["prism", "afrobeat", "lagos", "energetic"]),

    ("Kora Drums", {
        **seat(1, instrument=DJEMBE, articulation=ART_1, pattern=PAT_BASIC,
               brightness=0.5, decay=0.6, level=0.7),
        **seat(2, instrument=DJEMBE, articulation=ART_2, pattern=PAT_SPARSE,
               brightness=0.7, decay=0.3, level=0.45),
        **seat(3, instrument=DUNDUN, pattern=PAT_BASIC, brightness=0.35,
               level=0.6),
        **mute_seats(4, 5, 6, 7, 8),
        "osti_tempo": 95.0, "osti_swing": 20.0, "osti_humanize": 0.45,
        "osti_macroGather": 0.5, "osti_macroFire": 0.4,
    }, (0.5, 0.55, 0.35, 0.25, 0.15, 0.25),
     "Manding rhythm — djembe pair with dundun bass. Griot groove.",
     ["prism", "afrobeat", "manding", "traditional"]),

    ("Highlife Drums", {
        **seat(1, instrument=CONGA, pattern=PAT_STYLE_A, level=0.75),
        **seat(2, instrument=BONGOS, pattern=PAT_VAR, level=0.6, tuning=2.0),
        **seat(3, instrument=DUNDUN, articulation=ART_3, pattern=PAT_BASIC,
               level=0.55, brightness=0.7),
        **seat(4, instrument=CAJON, articulation=ART_3, pattern=PAT_BASIC,
               level=0.5, brightness=0.5),
        **mute_seats(5, 6, 7, 8),
        "osti_tempo": 110.0, "osti_swing": 10.0,
        "osti_macroGather": 0.6, "osti_macroFire": 0.5,
    }, (0.5, 0.55, 0.4, 0.4, 0.1, 0.25),
     "Highlife percussion — conga lead with bell and ghost cajon.",
     ["prism", "afrobeat", "highlife", "ghana"]),

    ("Sahel Groove", {
        **seat(1, instrument=DJEMBE, pattern=PAT_STYLE_C, brightness=0.55,
               level=0.75),
        **seat(2, instrument=DOUMBEK, pattern=PAT_STYLE_A, brightness=0.6,
               level=0.6),
        **seat(3, instrument=FRAMEDRUM, pattern=PAT_VAR, brightness=0.5,
               decay=0.7, level=0.5),
        **mute_seats(4, 5, 6, 7, 8),
        "osti_tempo": 100.0, "osti_swing": 30.0, "osti_humanize": 0.5,
        "osti_macroGather": 0.5, "osti_macroFire": 0.5,
    }, (0.5, 0.5, 0.35, 0.25, 0.15, 0.25),
     "Where West Africa meets the Sahara. Djembe, doumbek, and frame drum.",
     ["prism", "afrobeat", "sahel", "crossover"]),

    # --- Latin (5) ---
    ("Salsa Fire", {
        **seat(1, instrument=CONGA, articulation=ART_1, pattern=PAT_STYLE_A,
               level=0.8, brightness=0.6),
        **seat(2, instrument=CONGA, articulation=ART_2, pattern=PAT_STYLE_B,
               level=0.65, brightness=0.7),
        **seat(3, instrument=BONGOS, pattern=PAT_VAR, level=0.6, tuning=2.0),
        **seat(4, instrument=DUNDUN, articulation=ART_3, pattern=PAT_BASIC,
               level=0.5, brightness=0.7),
        **mute_seats(5, 6, 7, 8),
        "osti_tempo": 185.0, "osti_macroGather": 0.75, "osti_macroFire": 0.7,
        "osti_compThresh": -15.0, "osti_compRatio": 6.0,
    }, (0.6, 0.45, 0.6, 0.45, 0.1, 0.45),
     "Conga tumbaos and bongo martillo. The kitchen burns hot.",
     ["prism", "latin", "salsa", "conga"]),

    ("Samba Batucada", {
        **seat(1, instrument=SURDO, pattern=PAT_BASIC, brightness=0.3,
               level=0.85, body=0.7),
        **seat(2, instrument=SURDO, pattern=PAT_VAR, brightness=0.35,
               tuning=5.0, level=0.7),
        **seat(3, instrument=SURDO, articulation=ART_3, pattern=PAT_BASIC,
               brightness=0.6, tuning=12.0, level=0.55),
        **seat(4, instrument=CONGA, articulation=ART_2, pattern=PAT_FILL,
               brightness=0.7, level=0.5),
        **seat(5, instrument=BONGOS, pattern=PAT_DOUBLE, brightness=0.65,
               level=0.45),
        **mute_seats(6, 7, 8),
        "osti_tempo": 135.0, "osti_macroGather": 0.7, "osti_macroFire": 0.65,
        "osti_compThresh": -18.0, "osti_compRatio": 6.0,
    }, (0.45, 0.55, 0.6, 0.55, 0.1, 0.5),
     "Three surdos plus repique and tamborim layers. Carnival energy.",
     ["prism", "latin", "samba", "brazil", "batucada"]),

    ("Bossa Subtle", {
        **seat(1, instrument=CONGA, articulation=ART_4, pattern=PAT_SPARSE,
               brightness=0.5, level=0.6, decay=0.4),
        **seat(2, instrument=BONGOS, articulation=ART_1, pattern=PAT_BASIC,
               brightness=0.55, level=0.5),
        **seat(3, instrument=CAJON, articulation=ART_3, pattern=PAT_SPARSE,
               brightness=0.4, level=0.4),
        **mute_seats(4, 5, 6, 7, 8),
        "osti_tempo": 140.0, "osti_swing": 15.0,
        "osti_macroGather": 0.55, "osti_macroFire": 0.25,
        "osti_humanize": 0.35,
    }, (0.45, 0.55, 0.3, 0.25, 0.1, 0.1),
     "Fingertip conga, soft bongos, ghost cajon. Bossa nova whisper.",
     ["prism", "latin", "bossa", "subtle"]),

    ("Cumbia Wheels", {
        **seat(1, instrument=CONGA, pattern=PAT_STYLE_C, level=0.75),
        **seat(2, instrument=BONGOS, pattern=PAT_BASIC, level=0.6),
        **seat(3, instrument=CAJON, pattern=PAT_BASIC, level=0.65,
               brightness=0.5),
        **seat(4, instrument=DUNDUN, articulation=ART_3, pattern=PAT_BASIC,
               level=0.5, brightness=0.65),
        **mute_seats(5, 6, 7, 8),
        "osti_tempo": 95.0, "osti_swing": 25.0,
        "osti_macroGather": 0.6, "osti_macroFire": 0.5,
    }, (0.5, 0.5, 0.45, 0.4, 0.1, 0.3),
     "Cumbia rhythm cycle. Conga leads, bell anchors, cajon steady.",
     ["prism", "latin", "cumbia", "colombia"]),

    ("Rumba Guaguanco", {
        **seat(1, instrument=CONGA, articulation=ART_1, pattern=PAT_STYLE_B,
               level=0.8, brightness=0.6),
        **seat(2, instrument=CONGA, articulation=ART_2, pattern=PAT_STYLE_A,
               level=0.7, brightness=0.7, tuning=4.0),
        **seat(3, instrument=CONGA, articulation=ART_3, pattern=PAT_BASIC,
               level=0.65, brightness=0.4, tuning=-3.0),
        **seat(4, instrument=CAJON, articulation=ART_2, pattern=PAT_SPARSE,
               level=0.45, brightness=0.6),
        **mute_seats(5, 6, 7, 8),
        "osti_tempo": 105.0, "osti_swing": 20.0,
        "osti_macroGather": 0.6, "osti_macroFire": 0.55,
    }, (0.55, 0.5, 0.45, 0.4, 0.1, 0.35),
     "Three congas — quinto, tres, tumba. The conversation of rumba.",
     ["prism", "latin", "rumba", "conga-trio"]),

    # --- Electronic (5) ---
    ("808 Circle", {
        **seat(1, instrument=BEATBOX, articulation=ART_1, pattern=PAT_BASIC,
               brightness=0.3, decay=0.7, body=0.8, level=0.85),
        **seat(2, instrument=BEATBOX, articulation=ART_2, pattern=PAT_STYLE_A,
               brightness=0.6, decay=0.2, level=0.65),
        **seat(3, instrument=BEATBOX, articulation=ART_3, pattern=PAT_DOUBLE,
               brightness=0.8, decay=0.1, level=0.5),
        **seat(4, instrument=BEATBOX, articulation=ART_4, pattern=PAT_VAR,
               brightness=0.5, level=0.4),
        **mute_seats(5, 6, 7, 8),
        "osti_tempo": 130.0, "osti_macroGather": 0.85, "osti_macroFire": 0.5,
    }, (0.5, 0.45, 0.45, 0.45, 0.05, 0.4),
     "Four beatbox voices as an electronic drum kit. Modal 808.",
     ["prism", "electronic", "808", "beatbox"]),

    ("Tribal Techno", {
        **seat(1, instrument=TAIKO, pattern=PAT_BASIC, brightness=0.4,
               decay=0.5, level=0.8, body=0.6),
        **seat(2, instrument=DJEMBE, articulation=ART_2, pattern=PAT_STYLE_A,
               brightness=0.7, level=0.55),
        **seat(3, instrument=BONGOS, pattern=PAT_DOUBLE, brightness=0.65,
               level=0.45),
        **seat(4, instrument=BEATBOX, articulation=ART_3, pattern=PAT_DOUBLE,
               brightness=0.8, decay=0.08, level=0.4),
        **mute_seats(5, 6, 7, 8),
        "osti_tempo": 130.0, "osti_macroGather": 0.8, "osti_macroFire": 0.65,
        "osti_compThresh": -18.0, "osti_compRatio": 8.0,
    }, (0.55, 0.45, 0.55, 0.5, 0.1, 0.55),
     "Taiko and djembe slaps over techno hi-hats. Ancient meets machine.",
     ["prism", "electronic", "tribal", "techno"]),

    ("IDM Scatter", {
        **seat(1, instrument=BEATBOX, pattern=PAT_FILL, brightness=0.6,
               decay=0.15, level=0.6),
        **seat(2, instrument=BONGOS, articulation=ART_2, pattern=PAT_FILL,
               brightness=0.7, decay=0.1, level=0.5),
        **seat(3, instrument=DOUMBEK, articulation=ART_4, pattern=PAT_FILL,
               brightness=0.75, decay=0.08, level=0.45),
        **seat(4, instrument=TABLA, articulation=ART_1, pattern=PAT_STYLE_C,
               brightness=0.65, decay=0.12, level=0.4),
        **mute_seats(5, 6, 7, 8),
        "osti_tempo": 155.0, "osti_macroGather": 0.3, "osti_humanize": 0.6,
        "osti_macroFire": 0.4,
    }, (0.6, 0.35, 0.65, 0.5, 0.1, 0.35),
     "Fast fills with loose timing. Glitchy scatter across traditions.",
     ["prism", "electronic", "idm", "glitch"]),

    ("Drum and Bass", {
        **seat(1, instrument=BEATBOX, articulation=ART_1, pattern=PAT_BASIC,
               brightness=0.35, decay=0.4, level=0.85, body=0.7),
        **seat(2, instrument=BEATBOX, articulation=ART_2, pattern=PAT_STYLE_B,
               brightness=0.65, decay=0.15, level=0.6),
        **seat(3, instrument=BEATBOX, articulation=ART_3, pattern=PAT_DOUBLE,
               brightness=0.8, decay=0.05, level=0.5),
        **seat(4, instrument=DJEMBE, articulation=ART_2, pattern=PAT_STYLE_C,
               brightness=0.7, decay=0.1, level=0.45),
        **mute_seats(5, 6, 7, 8),
        "osti_tempo": 172.0, "osti_macroGather": 0.85, "osti_macroFire": 0.6,
        "osti_compThresh": -20.0, "osti_compRatio": 10.0,
    }, (0.55, 0.4, 0.65, 0.5, 0.05, 0.55),
     "Beatbox kit at 172. Compressed and fast. The jungle circle.",
     ["prism", "electronic", "drum-and-bass", "jungle"]),

    ("Ambient Glitch", {
        **seat(1, instrument=TONGDRUM, pattern=PAT_SPARSE, decay=1.5,
               brightness=0.55, level=0.5, tuning=0.0),
        **seat(2, instrument=BEATBOX, articulation=ART_3, pattern=PAT_FILL,
               brightness=0.7, decay=0.05, level=0.3),
        **seat(3, instrument=DOUMBEK, articulation=ART_4, pattern=PAT_SPARSE,
               brightness=0.65, decay=0.1, level=0.25),
        **mute_seats(4, 5, 6, 7, 8),
        "osti_tempo": 90.0, "osti_macroGather": 0.2, "osti_humanize": 0.7,
        "osti_macroFire": 0.2, "osti_macroSpace": 0.6,
        "osti_reverbMix": 0.4, "osti_reverbSize": 0.6,
    }, (0.5, 0.45, 0.35, 0.3, 0.55, 0.1),
     "Tongue drum bells with scattered micro-percussion. Organic glitch.",
     ["prism", "electronic", "ambient", "glitch"]),

    # --- Cinematic (5) ---
    ("War Drums", {
        **seat(1, instrument=TAIKO, articulation=ART_1, pattern=PAT_BASIC,
               brightness=0.4, decay=1.0, level=0.9, body=0.8),
        **seat(2, instrument=TAIKO, articulation=ART_4, pattern=PAT_VAR,
               brightness=0.5, decay=0.8, level=0.75, tuning=5.0),
        **seat(3, instrument=SURDO, pattern=PAT_BASIC, brightness=0.25,
               decay=1.2, level=0.8, body=0.9),
        **seat(4, instrument=DUNDUN, pattern=PAT_BASIC, brightness=0.3,
               decay=0.8, level=0.65),
        **mute_seats(5, 6, 7, 8),
        "osti_tempo": 90.0, "osti_macroGather": 0.8, "osti_macroFire": 0.8,
        "osti_compThresh": -15.0, "osti_compRatio": 6.0,
        "osti_reverbMix": 0.25, "osti_reverbSize": 0.5,
    }, (0.3, 0.65, 0.35, 0.45, 0.3, 0.75),
     "Taiko army with surdo and dundun reinforcement. Epic and massive.",
     ["prism", "cinematic", "war-drums", "epic"]),

    ("Ritual Ascent", {
        **seat(1, instrument=TAIKO, pattern=PAT_STYLE_A, decay=1.2, level=0.8,
               brightness=0.45),
        **seat(2, instrument=DJEMBE, pattern=PAT_STYLE_B, level=0.65,
               brightness=0.55),
        **seat(3, instrument=FRAMEDRUM, articulation=ART_3, pattern=PAT_VAR,
               level=0.5, brightness=0.5, decay=0.8),
        **seat(4, instrument=DOUMBEK, pattern=PAT_STYLE_C, level=0.45,
               brightness=0.6),
        **seat(5, instrument=TABLA, pattern=PAT_SPARSE, level=0.4,
               brightness=0.6),
        **mute_seats(6, 7, 8),
        "osti_tempo": 95.0, "osti_macroGather": 0.55, "osti_macroFire": 0.6,
        "osti_reverbMix": 0.3, "osti_reverbSize": 0.55,
    }, (0.5, 0.5, 0.45, 0.45, 0.35, 0.5),
     "Five traditions building to climax. FIRE macro drives the crescendo.",
     ["prism", "cinematic", "ritual", "building"]),

    ("Suspense Pulse", {
        **seat(1, instrument=SURDO, articulation=ART_2, pattern=PAT_BASIC,
               brightness=0.2, decay=0.4, level=0.7, body=0.8),
        **seat(2, instrument=TAIKO, articulation=ART_3, pattern=PAT_SPARSE,
               brightness=0.6, decay=0.2, level=0.35),
        **mute_seats(3, 4, 5, 6, 7, 8),
        "osti_tempo": 75.0, "osti_macroGather": 0.9,
        "osti_macroFire": 0.3, "osti_masterFilter": 5000.0,
        "osti_reverbMix": 0.3, "osti_reverbSize": 0.5,
    }, (0.2, 0.6, 0.15, 0.15, 0.35, 0.35),
     "Muted surdo heartbeat with rim accents. Tension before the storm.",
     ["prism", "cinematic", "suspense", "tension"]),

    ("Ancient Temple", {
        **seat(1, instrument=TAIKO, pattern=PAT_SPARSE, decay=1.5,
               brightness=0.4, level=0.8, body=0.8),
        **seat(2, instrument=TABLA, articulation=ART_4, pattern=PAT_SPARSE,
               brightness=0.3, decay=1.0, level=0.55),
        **seat(3, instrument=TONGDRUM, pattern=PAT_SPARSE, decay=2.0,
               brightness=0.5, level=0.4, tuning=0.0),
        **mute_seats(4, 5, 6, 7, 8),
        "osti_tempo": 50.0, "osti_macroGather": 0.4,
        "osti_macroSpace": 0.8,
        "osti_reverbMix": 0.55, "osti_reverbSize": 0.8, "osti_reverbDamp": 0.15,
    }, (0.35, 0.6, 0.1, 0.2, 0.8, 0.25),
     "Sparse hits in a vast stone space. Archaeological rhythm.",
     ["prism", "cinematic", "temple", "ancient"]),

    ("Chase Scene", {
        **seat(1, instrument=DJEMBE, pattern=PAT_DOUBLE, brightness=0.65,
               level=0.8),
        **seat(2, instrument=CONGA, pattern=PAT_DOUBLE, brightness=0.6,
               level=0.7),
        **seat(3, instrument=BONGOS, pattern=PAT_DOUBLE, brightness=0.7,
               level=0.6),
        **seat(4, instrument=BEATBOX, articulation=ART_1, pattern=PAT_BASIC,
               brightness=0.35, decay=0.5, level=0.75),
        **seat(5, instrument=BEATBOX, articulation=ART_3, pattern=PAT_DOUBLE,
               brightness=0.8, decay=0.05, level=0.45),
        **mute_seats(6, 7, 8),
        "osti_tempo": 160.0, "osti_macroGather": 0.8, "osti_macroFire": 0.75,
        "osti_compThresh": -20.0, "osti_compRatio": 10.0,
    }, (0.6, 0.4, 0.75, 0.6, 0.05, 0.65),
     "Maximum tempo, double patterns, all seats driving. Relentless pursuit.",
     ["prism", "cinematic", "chase", "intense"]),

    # --- World Fusion (5) ---
    ("Silk and Steel", {
        **seat(1, instrument=TABLA, pattern=PAT_STYLE_A, brightness=0.6,
               level=0.7),
        **seat(2, instrument=CAJON, pattern=PAT_BASIC, brightness=0.5,
               level=0.65),
        **seat(3, instrument=DJEMBE, pattern=PAT_VAR, brightness=0.55,
               level=0.55),
        **seat(4, instrument=DOUMBEK, pattern=PAT_STYLE_B, brightness=0.6,
               level=0.5),
        **mute_seats(5, 6, 7, 8),
        "osti_tempo": 100.0, "osti_swing": 15.0,
        "osti_macroGather": 0.55, "osti_macroFire": 0.5,
    }, (0.55, 0.5, 0.4, 0.4, 0.1, 0.3),
     "Tabla, cajon, djembe, doumbek. Four continents, one groove.",
     ["prism", "fusion", "world", "multicultural"]),

    ("East Meets West", {
        **seat(1, instrument=TABLA, pattern=PAT_STYLE_B, brightness=0.6,
               level=0.7),
        **seat(2, instrument=DJEMBE, pattern=PAT_STYLE_A, brightness=0.55,
               level=0.65),
        **seat(3, instrument=CAJON, pattern=PAT_BASIC, brightness=0.5,
               level=0.6),
        **mute_seats(4, 5, 6, 7, 8),
        "osti_tempo": 105.0, "osti_swing": 15.0,
        "osti_macroGather": 0.55, "osti_macroFire": 0.5,
    }, (0.5, 0.55, 0.4, 0.3, 0.1, 0.25),
     "Tabla, djembe, and cajon. India, Africa, and Peru at one fire.",
     ["prism", "fusion", "east-west", "trio"]),

    ("Pacific Rim", {
        **seat(1, instrument=TAIKO, pattern=PAT_BASIC, level=0.8,
               brightness=0.45),
        **seat(2, instrument=TONGDRUM, pattern=PAT_STYLE_A, level=0.55,
               brightness=0.6, tuning=5.0, decay=1.2),
        **seat(3, instrument=BONGOS, pattern=PAT_STYLE_B, level=0.5),
        **seat(4, instrument=SURDO, pattern=PAT_BASIC, level=0.65,
               brightness=0.3),
        **mute_seats(5, 6, 7, 8),
        "osti_tempo": 95.0, "osti_macroGather": 0.6, "osti_macroFire": 0.55,
    }, (0.45, 0.55, 0.4, 0.4, 0.15, 0.35),
     "Taiko, tongue drum, bongos, surdo. The Pacific rim speaks.",
     ["prism", "fusion", "pacific", "multicultural"]),

    ("Mediterranean Sun", {
        **seat(1, instrument=FRAMEDRUM, pattern=PAT_STYLE_A, level=0.7,
               brightness=0.55, decay=0.7),
        **seat(2, instrument=DOUMBEK, pattern=PAT_STYLE_B, level=0.65,
               brightness=0.6),
        **seat(3, instrument=CAJON, pattern=PAT_BASIC, level=0.6,
               brightness=0.5),
        **seat(4, instrument=CONGA, pattern=PAT_VAR, level=0.5),
        **mute_seats(5, 6, 7, 8),
        "osti_tempo": 110.0, "osti_swing": 10.0,
        "osti_macroGather": 0.6, "osti_macroFire": 0.5,
    }, (0.55, 0.5, 0.4, 0.4, 0.1, 0.25),
     "Frame drum, doumbek, cajon, conga. Mediterranean coast groove.",
     ["prism", "fusion", "mediterranean", "warm"]),

    ("Street Corner", {
        **seat(1, instrument=BEATBOX, articulation=ART_1, pattern=PAT_BASIC,
               level=0.8, brightness=0.35, decay=0.5),
        **seat(2, instrument=BEATBOX, articulation=ART_2, pattern=PAT_STYLE_A,
               level=0.6, brightness=0.6),
        **seat(3, instrument=CAJON, pattern=PAT_BASIC, level=0.7,
               brightness=0.5),
        **seat(4, instrument=BONGOS, pattern=PAT_VAR, level=0.5,
               brightness=0.55),
        **mute_seats(5, 6, 7, 8),
        "osti_tempo": 95.0, "osti_swing": 15.0,
        "osti_macroGather": 0.6, "osti_macroFire": 0.45,
    }, (0.5, 0.5, 0.4, 0.4, 0.1, 0.3),
     "Beatbox and cajon on a street corner. Urban acoustic fusion.",
     ["prism", "fusion", "street", "urban"]),
]


# ══════════════════════════════════════════════════════════════════════════════
# FLUX (20) — Rhythmic movement, polyrhythm, evolving patterns
# ══════════════════════════════════════════════════════════════════════════════

FLUX = [
    ("Polyrhythm 3v4", {
        **seat(1, instrument=DJEMBE, pattern=PAT_STYLE_A, level=0.75,
               brightness=0.55),
        **seat(2, instrument=CONGA, pattern=PAT_STYLE_B, level=0.7,
               brightness=0.55),
        **mute_seats(3, 4, 5, 6, 7, 8),
        "osti_tempo": 110.0, "osti_macroGather": 0.6, "osti_macroFire": 0.5,
    }, (0.5, 0.5, 0.55, 0.25, 0.1, 0.25),
     "Djembe in 3, conga in 4. The fire circle's favorite conversation.",
     ["flux", "polyrhythm", "3-against-4", "groove"]),

    ("Shifting Sands", {
        **seat(1, instrument=DOUMBEK, pattern=PAT_STYLE_A, level=0.7),
        **seat(2, instrument=TABLA, pattern=PAT_STYLE_C, level=0.6),
        **seat(3, instrument=DJEMBE, pattern=PAT_STYLE_B, level=0.55),
        **seat(4, instrument=FRAMEDRUM, pattern=PAT_VAR, level=0.5,
               decay=0.7),
        **mute_seats(5, 6, 7, 8),
        "osti_tempo": 105.0, "osti_swing": 30.0, "osti_humanize": 0.5,
        "osti_macroGather": 0.4, "osti_macroFire": 0.45,
    }, (0.5, 0.5, 0.5, 0.4, 0.15, 0.2),
     "Four different style patterns layered. The groove constantly shifts.",
     ["flux", "shifting", "layered", "organic"]),

    ("Accelerando", {
        **seats(*[seat(s+1, pattern=PAT_VAR) for s in range(6)]),
        **mute_seats(7, 8),
        "osti_tempo": 80.0, "osti_macroGather": 0.5, "osti_macroFire": 0.3,
        "osti_humanize": 0.4,
    }, (0.5, 0.5, 0.45, 0.5, 0.15, 0.25),
     "Start slow. Push FIRE macro to feel the circle accelerate into frenzy.",
     ["flux", "accelerando", "building", "dynamic"]),

    ("Swing Heavy", {
        **seat(1, instrument=DJEMBE, pattern=PAT_BASIC, level=0.8),
        **seat(2, instrument=CAJON, pattern=PAT_BASIC, level=0.7),
        **seat(3, instrument=BONGOS, pattern=PAT_VAR, level=0.55),
        **seat(4, instrument=DUNDUN, articulation=ART_3, pattern=PAT_BASIC,
               level=0.5, brightness=0.65),
        **mute_seats(5, 6, 7, 8),
        "osti_tempo": 100.0, "osti_swing": 65.0,
        "osti_macroGather": 0.55, "osti_macroFire": 0.5,
    }, (0.5, 0.55, 0.45, 0.4, 0.1, 0.3),
     "Maximum swing. The circle leans hard into every other beat.",
     ["flux", "swing", "groove", "feel"]),

    ("Ghost Fills", {
        **seat(1, instrument=DJEMBE, pattern=PAT_BASIC, level=0.75),
        **seat(2, instrument=DJEMBE, articulation=ART_4, pattern=PAT_FILL,
               level=0.25, brightness=0.3, decay=0.15),
        **seat(3, instrument=CONGA, pattern=PAT_BASIC, level=0.7),
        **seat(4, instrument=CONGA, articulation=ART_3, pattern=PAT_FILL,
               level=0.2, brightness=0.3, decay=0.1),
        **mute_seats(5, 6, 7, 8),
        "osti_tempo": 115.0, "osti_macroGather": 0.6,
        "osti_macroFire": 0.4, "osti_humanize": 0.4,
    }, (0.4, 0.55, 0.5, 0.45, 0.1, 0.15),
     "Main patterns with ghost note fills layered beneath. Depth through subtlety.",
     ["flux", "ghost-notes", "fills", "layered"]),

    ("Stutter Step", {
        **seat(1, instrument=BEATBOX, articulation=ART_1, pattern=PAT_DOUBLE,
               level=0.7, brightness=0.35, decay=0.3),
        **seat(2, instrument=BEATBOX, articulation=ART_2, pattern=PAT_DOUBLE,
               level=0.55, brightness=0.6, decay=0.1),
        **seat(3, instrument=DOUMBEK, articulation=ART_2, pattern=PAT_FILL,
               level=0.4, brightness=0.7, decay=0.08),
        **mute_seats(4, 5, 6, 7, 8),
        "osti_tempo": 140.0, "osti_macroGather": 0.7, "osti_macroFire": 0.5,
        "osti_compThresh": -18.0,
    }, (0.55, 0.4, 0.65, 0.4, 0.05, 0.45),
     "Double-time beatbox with doumbek fills. Stuttering machine groove.",
     ["flux", "stutter", "fast", "electronic"]),

    ("Breath and Pulse", {
        **seat(1, instrument=SURDO, pattern=PAT_BASIC, brightness=0.25,
               decay=0.7, level=0.75, body=0.7),
        **seat(2, instrument=FRAMEDRUM, articulation=ART_3, pattern=PAT_VAR,
               brightness=0.45, decay=0.5, level=0.4),
        **seat(3, instrument=DJEMBE, pattern=PAT_SPARSE, level=0.5),
        **mute_seats(4, 5, 6, 7, 8),
        "osti_tempo": 80.0, "osti_macroGather": 0.35, "osti_humanize": 0.6,
        "osti_macroFire": 0.3,
    }, (0.35, 0.6, 0.3, 0.25, 0.15, 0.15),
     "Surdo breathes, frame drum rolls, djembe speaks. Push-pull dynamics.",
     ["flux", "organic", "push-pull", "breathing"]),

    ("Cross Current", {
        **seat(1, instrument=TAIKO, pattern=PAT_STYLE_A, level=0.7,
               brightness=0.45),
        **seat(2, instrument=TABLA, pattern=PAT_STYLE_B, level=0.6,
               brightness=0.6),
        **seat(3, instrument=DJEMBE, pattern=PAT_STYLE_C, level=0.55,
               brightness=0.55),
        **seat(4, instrument=CAJON, pattern=PAT_STYLE_A, level=0.5,
               brightness=0.5),
        **seat(5, instrument=DOUMBEK, pattern=PAT_STYLE_B, level=0.45,
               brightness=0.6),
        **mute_seats(6, 7, 8),
        "osti_tempo": 108.0, "osti_macroGather": 0.5, "osti_macroFire": 0.55,
    }, (0.5, 0.5, 0.55, 0.5, 0.1, 0.3),
     "Five different style patterns crossing. Where they align creates the groove.",
     ["flux", "cross-rhythm", "five-seats", "complex"]),

    ("Humanize Max", {
        **seats(*[seat(s+1, pattern=PAT_VAR) for s in range(8)]),
        "osti_tempo": 100.0, "osti_macroGather": 0.1, "osti_humanize": 1.0,
        "osti_macroFire": 0.5,
    }, (0.5, 0.5, 0.5, 0.55, 0.15, 0.2),
     "All eight seats, maximum humanization. As loose as a real circle.",
     ["flux", "humanize", "loose", "organic"]),

    ("Machine Lock", {
        **seats(*[seat(s+1, pattern=PAT_BASIC) for s in range(8)]),
        "osti_tempo": 120.0, "osti_macroGather": 1.0, "osti_humanize": 0.0,
        "osti_macroFire": 0.6,
    }, (0.5, 0.45, 0.4, 0.6, 0.1, 0.35),
     "All eight seats, zero humanization, full gather. Perfect machine.",
     ["flux", "locked", "quantized", "machine"]),

    ("Tabla Trance", {
        **seat(1, instrument=TABLA, articulation=ART_2, pattern=PAT_STYLE_A,
               brightness=0.65, level=0.75),
        **seat(2, instrument=TABLA, articulation=ART_1, pattern=PAT_STYLE_B,
               brightness=0.6, level=0.65, tuning=5.0),
        **seat(3, instrument=TABLA, articulation=ART_3, pattern=PAT_STYLE_C,
               brightness=0.5, level=0.55, tuning=-3.0),
        **seat(4, instrument=TABLA, articulation=ART_4, pattern=PAT_BASIC,
               brightness=0.35, level=0.6, tuning=-7.0),
        **mute_seats(5, 6, 7, 8),
        "osti_tempo": 135.0, "osti_macroGather": 0.6, "osti_macroFire": 0.55,
        "osti_circleAmount": 0.3,
    }, (0.55, 0.5, 0.55, 0.45, 0.1, 0.25),
     "Four tablas at different tunings, interlocking patterns. Hypnotic.",
     ["flux", "tabla", "trance", "interlocking"]),

    ("Cajon Fingerstyle", {
        **seat(1, instrument=CAJON, articulation=ART_1, pattern=PAT_BASIC,
               level=0.8, brightness=0.45),
        **seat(2, instrument=CAJON, articulation=ART_2, pattern=PAT_STYLE_A,
               level=0.65, brightness=0.7),
        **seat(3, instrument=CAJON, articulation=ART_3, pattern=PAT_VAR,
               level=0.4, brightness=0.4),
        **seat(4, instrument=CAJON, articulation=ART_4, pattern=PAT_SPARSE,
               level=0.5, brightness=0.55),
        **mute_seats(5, 6, 7, 8),
        "osti_tempo": 95.0, "osti_swing": 20.0,
        "osti_macroGather": 0.6, "osti_macroFire": 0.4,
    }, (0.5, 0.55, 0.35, 0.35, 0.1, 0.25),
     "One cajon, four articulations, four patterns. The box speaks volumes.",
     ["flux", "cajon", "fingerstyle", "single-instrument"]),

    ("Djembe Circle", {
        **seat(1, instrument=DJEMBE, articulation=ART_1, pattern=PAT_BASIC,
               level=0.8),
        **seat(2, instrument=DJEMBE, articulation=ART_2, pattern=PAT_STYLE_A,
               level=0.6, brightness=0.7),
        **seat(3, instrument=DJEMBE, articulation=ART_3, pattern=PAT_VAR,
               level=0.65, brightness=0.3),
        **seat(4, instrument=DJEMBE, articulation=ART_4, pattern=PAT_SPARSE,
               level=0.4, brightness=0.3),
        **seat(5, instrument=DJEMBE, articulation=ART_1, pattern=PAT_STYLE_B,
               level=0.55, tuning=5.0),
        **seat(6, instrument=DJEMBE, articulation=ART_2, pattern=PAT_STYLE_C,
               level=0.45, tuning=7.0, brightness=0.7),
        **mute_seats(7, 8),
        "osti_tempo": 115.0, "osti_macroGather": 0.55, "osti_macroFire": 0.55,
        "osti_humanize": 0.4,
    }, (0.5, 0.55, 0.5, 0.55, 0.1, 0.35),
     "Six djembes in conversation. Tone, slap, bass, mute — the full language.",
     ["flux", "djembe", "circle", "conversation"]),

    ("Taiko Storm", {
        **seat(1, instrument=TAIKO, articulation=ART_1, pattern=PAT_DOUBLE,
               level=0.85, decay=0.8, body=0.8),
        **seat(2, instrument=TAIKO, articulation=ART_4, pattern=PAT_DOUBLE,
               level=0.7, tuning=5.0),
        **seat(3, instrument=TAIKO, articulation=ART_2, pattern=PAT_STYLE_A,
               level=0.6, tuning=10.0, brightness=0.6),
        **seat(4, instrument=TAIKO, articulation=ART_3, pattern=PAT_FILL,
               level=0.5, brightness=0.7, decay=0.15),
        **mute_seats(5, 6, 7, 8),
        "osti_tempo": 145.0, "osti_macroGather": 0.75, "osti_macroFire": 0.8,
        "osti_compThresh": -20.0, "osti_compRatio": 8.0,
    }, (0.4, 0.55, 0.65, 0.5, 0.1, 0.7),
     "Four taikos in full assault. Double time, high fire, compressed power.",
     ["flux", "taiko", "storm", "intense", "japan"]),

    ("Conga Cascade", {
        **seat(1, instrument=CONGA, articulation=ART_1, pattern=PAT_STYLE_A,
               level=0.75),
        **seat(2, instrument=CONGA, articulation=ART_2, pattern=PAT_STYLE_B,
               level=0.65, tuning=4.0),
        **seat(3, instrument=CONGA, articulation=ART_3, pattern=PAT_STYLE_C,
               level=0.55, tuning=-3.0),
        **seat(4, instrument=CONGA, articulation=ART_4, pattern=PAT_VAR,
               level=0.45, brightness=0.55),
        **mute_seats(5, 6, 7, 8),
        "osti_tempo": 120.0, "osti_macroGather": 0.55, "osti_macroFire": 0.5,
        "osti_circleAmount": 0.3,
    }, (0.5, 0.55, 0.5, 0.4, 0.1, 0.25),
     "Four congas cascading through interlocking patterns. Each triggers the next.",
     ["flux", "conga", "cascade", "interlocking"]),

    ("Velocity Dance", {
        **seats(*[seat(s+1, velSens=1.0, pattern=PAT_STYLE_A) for s in range(6)]),
        **mute_seats(7, 8),
        "osti_tempo": 105.0, "osti_macroGather": 0.6,
        "osti_macroFire": 0.5,
    }, (0.5, 0.5, 0.45, 0.5, 0.1, 0.35),
     "Maximum velocity sensitivity. Your touch shapes every hit.",
     ["flux", "velocity", "expressive", "dynamic"]),

    ("Exciter Extremes", {
        **seat(1, instrument=DJEMBE, exciterMix=0.0, pattern=PAT_BASIC,
               level=0.7),
        **seat(2, instrument=DJEMBE, exciterMix=1.0, pattern=PAT_BASIC,
               level=0.7, tuning=5.0),
        **seat(3, instrument=TAIKO, exciterMix=0.0, pattern=PAT_VAR,
               level=0.65),
        **seat(4, instrument=TAIKO, exciterMix=1.0, pattern=PAT_VAR,
               level=0.65, tuning=7.0),
        **mute_seats(5, 6, 7, 8),
        "osti_tempo": 100.0, "osti_macroGather": 0.6,
        "osti_macroFire": 0.5,
    }, (0.5, 0.5, 0.35, 0.35, 0.1, 0.25),
     "Same drums, opposite exciter mixes. Noise vs pitched side by side.",
     ["flux", "exciter", "contrast", "study"]),

    ("Circle Sympathy", {
        **seats(*[seat(s+1, pattern=PAT_SPARSE) for s in range(8)]),
        "osti_circleAmount": 0.8, "osti_macroCircle": 0.9,
        "osti_tempo": 90.0, "osti_macroGather": 0.4,
        "osti_humanize": 0.5,
    }, (0.5, 0.5, 0.5, 0.55, 0.15, 0.2),
     "Sparse hits but maximum CIRCLE. Every strike triggers neighbors.",
     ["flux", "circle", "sympathetic", "chain-reaction"]),

    ("All Patterns", {
        **seat(1, instrument=DJEMBE, pattern=PAT_BASIC, level=0.75),
        **seat(2, instrument=CONGA, pattern=PAT_VAR, level=0.7),
        **seat(3, instrument=TABLA, pattern=PAT_FILL, level=0.55),
        **seat(4, instrument=CAJON, pattern=PAT_SPARSE, level=0.65),
        **seat(5, instrument=DOUMBEK, pattern=PAT_STYLE_A, level=0.55),
        **seat(6, instrument=BONGOS, pattern=PAT_STYLE_B, level=0.5),
        **seat(7, instrument=FRAMEDRUM, pattern=PAT_STYLE_C, level=0.45),
        **seat(8, instrument=TAIKO, pattern=PAT_DOUBLE, level=0.6),
        "osti_tempo": 105.0, "osti_macroGather": 0.55, "osti_macroFire": 0.5,
    }, (0.5, 0.5, 0.5, 0.65, 0.1, 0.3),
     "Every pattern type used once. Maximum variety from a single circle.",
     ["flux", "all-patterns", "variety", "showcase"]),

    ("Double Time Flip", {
        **seat(1, instrument=DJEMBE, pattern=PAT_BASIC, level=0.75),
        **seat(2, instrument=DJEMBE, pattern=PAT_DOUBLE, level=0.5,
               articulation=ART_4, brightness=0.3),
        **seat(3, instrument=CONGA, pattern=PAT_BASIC, level=0.7),
        **seat(4, instrument=CONGA, pattern=PAT_DOUBLE, level=0.45,
               articulation=ART_3, brightness=0.3),
        **mute_seats(5, 6, 7, 8),
        "osti_tempo": 100.0, "osti_macroGather": 0.6, "osti_macroFire": 0.5,
    }, (0.45, 0.55, 0.55, 0.45, 0.1, 0.25),
     "Basic and double patterns paired. Ghost doubles between main hits.",
     ["flux", "double-time", "ghost", "layered"]),
]


# ══════════════════════════════════════════════════════════════════════════════
# AETHER (15) — Experimental, detuned, extreme, beatbox machines
# ══════════════════════════════════════════════════════════════════════════════

AETHER = [
    ("Detuned Ritual", {
        **seat(1, instrument=DJEMBE, tuning=-8.0, decay=1.5, body=0.9,
               pattern=PAT_SPARSE, level=0.7),
        **seat(2, instrument=TAIKO, tuning=6.5, decay=1.2, body=0.85,
               pattern=PAT_SPARSE, level=0.65),
        **seat(3, instrument=SURDO, tuning=-12.0, decay=1.8, body=0.95,
               pattern=PAT_SPARSE, level=0.6),
        **mute_seats(4, 5, 6, 7, 8),
        "osti_tempo": 55.0, "osti_macroGather": 0.2, "osti_humanize": 0.8,
        "osti_macroFire": 0.4, "osti_reverbMix": 0.4, "osti_reverbSize": 0.7,
    }, (0.3, 0.6, 0.2, 0.2, 0.55, 0.2),
     "Drums tuned to strange intervals. A ritual from a forgotten tradition.",
     ["aether", "detuned", "ritual", "strange"]),

    ("Noise Orchestra", {
        **seats(*[seat(s+1, exciterMix=0.0, brightness=0.8, decay=0.15,
                       pattern=PAT_FILL) for s in range(8)]),
        "osti_tempo": 155.0, "osti_macroGather": 0.4, "osti_humanize": 0.6,
        "osti_macroFire": 0.6, "osti_masterFilter": 12000.0,
    }, (0.7, 0.25, 0.6, 0.7, 0.05, 0.5),
     "All exciter mix to noise. Eight channels of shaped static.",
     ["aether", "noise", "experimental", "harsh"]),

    ("Pitched Gamelan", {
        **seats(*[seat(s+1, instrument=TONGDRUM, exciterMix=1.0,
                       tuning=[-2, 0, 3, 5, 7, 10, 12, 14][s],
                       brightness=0.65, decay=1.8,
                       pattern=[PAT_STYLE_A, PAT_STYLE_B, PAT_STYLE_C,
                               PAT_VAR, PAT_SPARSE, PAT_STYLE_A,
                               PAT_STYLE_B, PAT_SPARSE][s],
                       level=[0.6, 0.65, 0.55, 0.5, 0.45, 0.5, 0.45, 0.4][s])
                 for s in range(8)]),
        "osti_tempo": 80.0, "osti_macroGather": 0.4, "osti_humanize": 0.35,
        "osti_macroSpace": 0.5,
        "osti_reverbMix": 0.35, "osti_reverbSize": 0.55,
    }, (0.6, 0.55, 0.4, 0.55, 0.5, 0.05),
     "Eight tongue drums tuned to pelog-ish scale. Synthetic gamelan.",
     ["aether", "gamelan", "tuned", "melodic"]),

    ("Body Override", {
        **seat(1, instrument=DJEMBE, bodyModel=BM_BOX, pattern=PAT_BASIC,
               level=0.7),
        **seat(2, instrument=TABLA, bodyModel=BM_CYLINDER, pattern=PAT_VAR,
               level=0.65),
        **seat(3, instrument=DOUMBEK, bodyModel=BM_OPEN, pattern=PAT_STYLE_A,
               level=0.6),
        **seat(4, instrument=CAJON, bodyModel=BM_CONICAL, pattern=PAT_BASIC,
               level=0.55),
        **mute_seats(5, 6, 7, 8),
        "osti_tempo": 100.0, "osti_macroGather": 0.6,
    }, (0.5, 0.5, 0.35, 0.35, 0.15, 0.2),
     "Wrong body models on every drum. A djembe in a box, tabla in a cylinder.",
     ["aether", "body-override", "experimental", "wrong"]),

    ("Pitch Sweep Down", {
        **seats(*[seat(s+1, pitchEnv=1.0, brightness=0.7, decay=0.8)
                 for s in range(8)]),
        "osti_tempo": 100.0, "osti_macroGather": 0.6, "osti_macroFire": 0.5,
    }, (0.55, 0.45, 0.4, 0.55, 0.1, 0.35),
     "Maximum pitch envelope on all seats. Every hit swoops from high to low.",
     ["aether", "pitch-sweep", "downward", "dramatic"]),

    ("Reverse Time", {
        **seats(*[seat(s+1, pitchEnv=-1.0, brightness=0.6, decay=1.0)
                 for s in range(8)]),
        "osti_tempo": 85.0, "osti_macroGather": 0.5, "osti_macroFire": 0.4,
        "osti_reverbMix": 0.3, "osti_reverbSize": 0.55,
    }, (0.5, 0.5, 0.4, 0.55, 0.35, 0.25),
     "Negative pitch envelope. Hits sweep upward — time flowing backward.",
     ["aether", "reverse", "pitch-sweep", "upward"]),

    ("Beatbox Machine", {
        **seats(*[seat(s+1, instrument=BEATBOX,
                       articulation=[ART_1, ART_2, ART_3, ART_4,
                                    ART_1, ART_2, ART_3, ART_4][s],
                       pattern=[PAT_BASIC, PAT_STYLE_A, PAT_DOUBLE,
                               PAT_FILL, PAT_VAR, PAT_STYLE_B,
                               PAT_STYLE_C, PAT_SPARSE][s],
                       level=[0.8, 0.6, 0.5, 0.4, 0.7, 0.55, 0.45, 0.35][s],
                       tuning=[0, 0, 0, 0, 5, 5, -3, -7][s])
                 for s in range(8)]),
        "osti_tempo": 130.0, "osti_macroGather": 0.8, "osti_macroFire": 0.6,
        "osti_compThresh": -18.0, "osti_compRatio": 8.0,
    }, (0.5, 0.4, 0.55, 0.65, 0.05, 0.45),
     "Eight beatbox voices — every articulation, every pattern. Full machine.",
     ["aether", "beatbox", "machine", "full"]),

    ("Compressed Wall", {
        **seats(*[seat(s+1, pattern=PAT_DOUBLE, decay=0.3, level=0.6,
                       brightness=0.55) for s in range(8)]),
        "osti_tempo": 150.0, "osti_macroGather": 0.9,
        "osti_compThresh": -30.0, "osti_compRatio": 20.0,
        "osti_compAttack": 0.5, "osti_compRelease": 15.0,
        "osti_macroFire": 0.8,
    }, (0.5, 0.45, 0.55, 0.8, 0.05, 0.65),
     "Maximum compression, all double patterns. A wall of percussion.",
     ["aether", "compressed", "wall", "extreme"]),

    ("Micro Rhythms", {
        **seat(1, instrument=TABLA, articulation=ART_1, pattern=PAT_FILL,
               decay=0.05, level=0.5, brightness=0.8),
        **seat(2, instrument=DOUMBEK, articulation=ART_4, pattern=PAT_FILL,
               decay=0.04, level=0.45, brightness=0.85),
        **seat(3, instrument=BONGOS, articulation=ART_2, pattern=PAT_FILL,
               decay=0.04, level=0.4, brightness=0.8),
        **mute_seats(4, 5, 6, 7, 8),
        "osti_tempo": 200.0, "osti_macroGather": 0.3, "osti_humanize": 0.7,
        "osti_macroFire": 0.3, "osti_masterFilter": 15000.0,
    }, (0.7, 0.3, 0.7, 0.5, 0.05, 0.25),
     "Extremely short decays at 200 BPM. Clicks and ticks at the edge of rhythm.",
     ["aether", "micro", "fast", "clicks"]),

    ("Resonance Drone", {
        **seat(1, instrument=TAIKO, decay=2.0, body=1.0, brightness=0.3,
               level=0.8, pattern=PAT_SPARSE, bodyModel=BM_CYLINDER),
        **seat(2, instrument=SURDO, decay=2.0, body=1.0, brightness=0.2,
               level=0.7, pattern=PAT_SPARSE, tuning=-5.0),
        **mute_seats(3, 4, 5, 6, 7, 8),
        "osti_tempo": 40.0, "osti_macroGather": 0.3,
        "osti_macroFire": 0.3, "osti_masterDecay": 2.0,
        "osti_reverbMix": 0.5, "osti_reverbSize": 0.8,
        "osti_circleAmount": 0.5,
    }, (0.2, 0.7, 0.15, 0.2, 0.6, 0.15),
     "Max decay, max body, max reverb. Drums become drones.",
     ["aether", "drone", "resonance", "sustained"]),

    ("Cross Body", {
        **seat(1, instrument=TABLA, bodyModel=BM_BOX, level=0.7, pattern=PAT_STYLE_A),
        **seat(2, instrument=DJEMBE, bodyModel=BM_OPEN, level=0.65, pattern=PAT_STYLE_B),
        **seat(3, instrument=TAIKO, bodyModel=BM_CONICAL, level=0.6, pattern=PAT_VAR),
        **seat(4, instrument=CAJON, bodyModel=BM_CYLINDER, level=0.55, pattern=PAT_BASIC),
        **seat(5, instrument=SURDO, bodyModel=BM_OPEN, level=0.5, pattern=PAT_SPARSE),
        **mute_seats(6, 7, 8),
        "osti_tempo": 95.0, "osti_macroGather": 0.5,
    }, (0.5, 0.5, 0.4, 0.45, 0.15, 0.2),
     "Every drum has the wrong body. Familiar instruments in alien shells.",
     ["aether", "cross-body", "hybrid", "experimental"]),

    ("Filter Sweep", {
        **seats(*[seat(s+1, pattern=PAT_VAR) for s in range(8)]),
        "osti_masterFilter": 400.0, "osti_masterReso": 0.7,
        "osti_tempo": 110.0, "osti_macroGather": 0.6,
        "osti_macroFire": 0.5,
    }, (0.1, 0.7, 0.35, 0.55, 0.1, 0.15),
     "Master filter nearly closed. Open it slowly to reveal the circle.",
     ["aether", "filter", "sweep", "reveal"]),

    ("Harmonic Stack", {
        **seats(*[seat(s+1, instrument=TONGDRUM,
                       tuning=[0, 12, 7, 5, 12, 0, 7, 5][s],
                       decay=2.0, brightness=0.65,
                       pattern=PAT_BASIC,
                       level=[0.65, 0.55, 0.5, 0.45, 0.5, 0.45, 0.4, 0.35][s])
                 for s in range(8)]),
        "osti_tempo": 70.0, "osti_macroGather": 0.7,
        "osti_circleAmount": 0.5, "osti_macroCircle": 0.6,
        "osti_reverbMix": 0.3, "osti_reverbSize": 0.5,
    }, (0.6, 0.55, 0.35, 0.55, 0.4, 0.05),
     "Tongue drums in octaves and fifths. Circle coupling creates chords.",
     ["aether", "harmonic", "stacked", "tuned"]),

    ("Infinite Decay", {
        **seats(*[seat(s+1, decay=2.0, body=0.9, brightness=0.4,
                       pattern=PAT_SPARSE) for s in range(8)]),
        "osti_masterDecay": 2.0, "osti_tempo": 45.0,
        "osti_macroGather": 0.2, "osti_humanize": 0.6,
        "osti_macroSpace": 0.7,
        "osti_reverbMix": 0.5, "osti_reverbSize": 0.75,
        "osti_circleAmount": 0.6, "osti_macroCircle": 0.7,
    }, (0.3, 0.6, 0.25, 0.45, 0.65, 0.1),
     "Maximum decay, maximum circle. Each hit sustains into the next.",
     ["aether", "infinite", "sustain", "wash"]),

    ("Alien Campfire", {
        **seats(*[seat(s+1, instrument=BEATBOX,
                       tuning=[-12, -7, 0, 3, 7, 12, -5, 5][s],
                       bodyModel=[BM_BOX, BM_CONICAL, BM_OPEN, BM_CYLINDER,
                                 BM_BOX, BM_OPEN, BM_CONICAL, BM_CYLINDER][s],
                       pattern=[PAT_STYLE_A, PAT_STYLE_B, PAT_STYLE_C,
                               PAT_VAR, PAT_SPARSE, PAT_FILL,
                               PAT_DOUBLE, PAT_BASIC][s],
                       brightness=0.6, decay=0.3,
                       level=[0.6, 0.55, 0.5, 0.5, 0.45, 0.4, 0.4, 0.35][s])
                 for s in range(8)]),
        "osti_tempo": 110.0, "osti_macroGather": 0.5, "osti_humanize": 0.5,
        "osti_macroFire": 0.5, "osti_circleAmount": 0.4,
    }, (0.5, 0.4, 0.5, 0.6, 0.1, 0.3),
     "Eight beatbox voices, wrong bodies, strange tunings. A fire circle from another world.",
     ["aether", "alien", "beatbox", "wrong-body"]),
]


# ══════════════════════════════════════════════════════════════════════════════
# ENTANGLED (20) — Coupling-ready ensembles with high CIRCLE macro
# ══════════════════════════════════════════════════════════════════════════════

ENTANGLED = [
    ("Coupling Donor", {
        **seat(1, instrument=DJEMBE, pattern=PAT_BASIC, level=0.8),
        **seat(2, instrument=CONGA, pattern=PAT_VAR, level=0.7),
        **seat(3, instrument=TAIKO, pattern=PAT_BASIC, level=0.65),
        **mute_seats(4, 5, 6, 7, 8),
        "osti_macroGather": 0.7, "osti_macroFire": 0.6,
        "osti_circleAmount": 0.5, "osti_macroCircle": 0.6,
    }, (0.5, 0.5, 0.4, 0.3, 0.1, 0.35),
     "Strong rhythmic output for coupling to other engines.",
     ["entangled", "donor", "coupling", "rhythmic"]),

    ("Coupling Receiver", {
        **seat(1, instrument=FRAMEDRUM, pattern=PAT_SPARSE, decay=1.0,
               level=0.6),
        **seat(2, instrument=TONGDRUM, pattern=PAT_SPARSE, decay=1.5,
               level=0.5),
        **mute_seats(3, 4, 5, 6, 7, 8),
        "osti_macroGather": 0.3, "osti_macroFire": 0.2,
        "osti_circleAmount": 0.8,
    }, (0.4, 0.55, 0.3, 0.15, 0.2, 0.05),
     "Sparse, sensitive. Designed to receive coupling input from fleet engines.",
     ["entangled", "receiver", "coupling", "sensitive"]),

    ("Circle Maximum", {
        **seats(*[seat(s+1, pattern=PAT_SPARSE, level=0.6) for s in range(8)]),
        "osti_circleAmount": 1.0, "osti_macroCircle": 1.0,
        "osti_tempo": 80.0, "osti_macroGather": 0.4,
        "osti_humanize": 0.4,
    }, (0.5, 0.5, 0.5, 0.55, 0.15, 0.2),
     "Maximum sympathetic resonance. Every seat triggers every other.",
     ["entangled", "circle", "maximum", "sympathetic"]),

    ("Fire Bridge", {
        **seat(1, instrument=DJEMBE, pattern=PAT_STYLE_A, level=0.75,
               brightness=0.6),
        **seat(2, instrument=TAIKO, pattern=PAT_BASIC, level=0.7,
               brightness=0.4),
        **seat(3, instrument=CONGA, pattern=PAT_STYLE_B, level=0.6),
        **mute_seats(4, 5, 6, 7, 8),
        "osti_macroGather": 0.6, "osti_macroFire": 0.7,
        "osti_circleAmount": 0.6,
    }, (0.5, 0.5, 0.45, 0.35, 0.1, 0.4),
     "High-energy output for coupling bridges. Fire macro drives intensity.",
     ["entangled", "bridge", "fire", "intense"]),

    ("Tabla Resonator", {
        **seat(1, instrument=TABLA, articulation=ART_2, pattern=PAT_STYLE_A,
               brightness=0.65, decay=0.8, level=0.7),
        **seat(2, instrument=TABLA, articulation=ART_4, pattern=PAT_VAR,
               brightness=0.3, decay=1.0, level=0.6, tuning=-7.0),
        **mute_seats(3, 4, 5, 6, 7, 8),
        "osti_circleAmount": 0.7, "osti_macroCircle": 0.7,
        "osti_macroFire": 0.3,
    }, (0.55, 0.5, 0.35, 0.2, 0.15, 0.15),
     "Tabla pair as a coupling resonator. Harmonic modes respond to fleet input.",
     ["entangled", "tabla", "resonator", "harmonic"]),

    ("Texture Feed", {
        **seat(1, instrument=FRAMEDRUM, articulation=ART_3, pattern=PAT_FILL,
               decay=0.3, level=0.5, brightness=0.5),
        **seat(2, instrument=BONGOS, pattern=PAT_FILL, decay=0.15,
               level=0.4, brightness=0.6),
        **seat(3, instrument=DOUMBEK, articulation=ART_4, pattern=PAT_FILL,
               decay=0.1, level=0.35, brightness=0.7),
        **mute_seats(4, 5, 6, 7, 8),
        "osti_tempo": 160.0, "osti_macroGather": 0.3, "osti_humanize": 0.6,
        "osti_circleAmount": 0.5,
    }, (0.55, 0.4, 0.55, 0.45, 0.1, 0.2),
     "Dense rhythmic texture for coupling feed. Short hits, fast fills.",
     ["entangled", "texture", "feed", "dense"]),

    ("Pulse Echo", {
        **seat(1, instrument=SURDO, pattern=PAT_BASIC, decay=0.7,
               level=0.75, brightness=0.3),
        **mute_seats(2, 3, 4, 5, 6, 7, 8),
        "osti_tempo": 90.0, "osti_macroGather": 0.8,
        "osti_reverbMix": 0.4, "osti_reverbSize": 0.6,
        "osti_circleAmount": 0.5,
    }, (0.25, 0.6, 0.2, 0.1, 0.45, 0.2),
     "Single surdo pulse with reverb. Coupling rhythmic heartbeat.",
     ["entangled", "pulse", "reverb", "heartbeat"]),

    ("Drift Weave", {
        **seat(1, instrument=DJEMBE, pattern=PAT_STYLE_A, level=0.7),
        **seat(2, instrument=TABLA, pattern=PAT_STYLE_B, level=0.6),
        **seat(3, instrument=DOUMBEK, pattern=PAT_STYLE_C, level=0.55),
        **mute_seats(4, 5, 6, 7, 8),
        "osti_tempo": 100.0, "osti_macroGather": 0.35, "osti_humanize": 0.6,
        "osti_circleAmount": 0.6, "osti_macroCircle": 0.6,
    }, (0.5, 0.5, 0.5, 0.3, 0.15, 0.2),
     "Loose weave of three traditions. Coupling drifts between sources.",
     ["entangled", "weave", "drift", "loose"]),

    ("Spatial Knot", {
        **seats(*[seat(s+1, pattern=PAT_SPARSE, level=0.5,
                       pan=[-0.9, -0.6, -0.3, 0.0, 0.3, 0.6, 0.9, 0.0][s])
                 for s in range(8)]),
        "osti_macroSpace": 0.7, "osti_reverbMix": 0.4,
        "osti_reverbSize": 0.6, "osti_circleAmount": 0.5,
        "osti_tempo": 85.0,
    }, (0.45, 0.5, 0.35, 0.45, 0.6, 0.15),
     "Wide stereo placement + space macro. The circle fills the room.",
     ["entangled", "spatial", "wide", "stereo"]),

    ("Sympathetic Bass", {
        **seat(1, instrument=SURDO, decay=1.5, body=0.9, brightness=0.2,
               pattern=PAT_BASIC, level=0.8),
        **seat(2, instrument=TAIKO, decay=1.2, body=0.85, brightness=0.25,
               pattern=PAT_SPARSE, level=0.65),
        **mute_seats(3, 4, 5, 6, 7, 8),
        "osti_circleAmount": 0.8, "osti_macroCircle": 0.8,
        "osti_macroFire": 0.5, "osti_masterFilter": 4000.0,
    }, (0.15, 0.75, 0.25, 0.2, 0.15, 0.3),
     "Deep bass pair with high sympathetic coupling. Surdo and taiko feed each other.",
     ["entangled", "bass", "sympathetic", "deep"]),

    ("Bright Scatter", {
        **seat(1, instrument=TABLA, articulation=ART_1, pattern=PAT_FILL,
               brightness=0.8, decay=0.2, level=0.55),
        **seat(2, instrument=DOUMBEK, articulation=ART_2, pattern=PAT_FILL,
               brightness=0.8, decay=0.15, level=0.5),
        **seat(3, instrument=BONGOS, articulation=ART_2, pattern=PAT_FILL,
               brightness=0.75, decay=0.15, level=0.45),
        **mute_seats(4, 5, 6, 7, 8),
        "osti_tempo": 145.0, "osti_macroGather": 0.3,
        "osti_macroFire": 0.4, "osti_circleAmount": 0.4,
    }, (0.7, 0.35, 0.6, 0.45, 0.1, 0.25),
     "Bright fills scattered with sympathetic coupling. Coupling adds sparkle.",
     ["entangled", "bright", "scatter", "fills"]),

    ("Gather Control", {
        **seats(*[seat(s+1, pattern=PAT_STYLE_A) for s in range(8)]),
        "osti_macroGather": 0.0, "osti_humanize": 0.8,
        "osti_circleAmount": 0.6,
    }, (0.5, 0.5, 0.5, 0.55, 0.15, 0.2),
     "GATHER at zero. Push it to hear the circle snap to grid. Coupling follows.",
     ["entangled", "gather", "control", "macro"]),

    ("Harmonic Couple", {
        **seat(1, instrument=TONGDRUM, tuning=0.0, decay=1.8, pattern=PAT_BASIC,
               level=0.65, brightness=0.6),
        **seat(2, instrument=TONGDRUM, tuning=7.0, decay=1.8, pattern=PAT_VAR,
               level=0.55, brightness=0.6),
        **seat(3, instrument=TONGDRUM, tuning=12.0, decay=1.8, pattern=PAT_SPARSE,
               level=0.5, brightness=0.6),
        **mute_seats(4, 5, 6, 7, 8),
        "osti_circleAmount": 0.7, "osti_macroCircle": 0.7,
        "osti_reverbMix": 0.3, "osti_reverbSize": 0.5,
    }, (0.55, 0.55, 0.3, 0.25, 0.4, 0.05),
     "Tuned tongue drums with circle coupling. Harmonic content for fleet.",
     ["entangled", "harmonic", "tongue-drum", "tuned"]),

    ("Macro Entangle", {
        "osti_macroGather": 0.4, "osti_macroFire": 0.5,
        "osti_macroCircle": 0.6, "osti_macroSpace": 0.3,
        "osti_circleAmount": 0.5,
        **seats(*[seat(s+1, pattern=PAT_VAR) for s in range(8)]),
    }, (0.5, 0.5, 0.45, 0.55, 0.3, 0.25),
     "All four macros active. Every axis of control engaged for coupling.",
     ["entangled", "all-macros", "full-control"]),

    ("Fire Scatter", {
        **seat(1, instrument=DJEMBE, articulation=ART_2, pattern=PAT_STYLE_B,
               brightness=0.75, level=0.7),
        **seat(2, instrument=CONGA, articulation=ART_2, pattern=PAT_STYLE_C,
               brightness=0.7, level=0.6),
        **seat(3, instrument=CAJON, articulation=ART_2, pattern=PAT_FILL,
               brightness=0.7, level=0.55),
        **seat(4, instrument=BEATBOX, articulation=ART_2, pattern=PAT_DOUBLE,
               brightness=0.7, level=0.5),
        **mute_seats(5, 6, 7, 8),
        "osti_macroFire": 0.8, "osti_macroGather": 0.6,
        "osti_circleAmount": 0.5, "osti_tempo": 135.0,
    }, (0.65, 0.4, 0.55, 0.5, 0.05, 0.5),
     "All slap articulations at high FIRE. Coupling sends sharp transients.",
     ["entangled", "fire", "slap", "transient"]),

    ("Space Entangle", {
        **seat(1, instrument=TAIKO, pattern=PAT_SPARSE, decay=1.2, level=0.7),
        **seat(2, instrument=FRAMEDRUM, pattern=PAT_SPARSE, decay=0.9,
               level=0.55),
        **mute_seats(3, 4, 5, 6, 7, 8),
        "osti_macroSpace": 0.8, "osti_reverbMix": 0.5,
        "osti_reverbSize": 0.7, "osti_circleAmount": 0.6,
        "osti_tempo": 65.0,
    }, (0.35, 0.55, 0.2, 0.15, 0.7, 0.2),
     "Reverberant sparse pattern. Coupling carries the space information.",
     ["entangled", "space", "reverb", "sparse"]),

    ("Velocity Couple", {
        **seats(*[seat(s+1, velSens=1.0, pattern=PAT_STYLE_A) for s in range(6)]),
        **mute_seats(7, 8),
        "osti_circleAmount": 0.6, "osti_macroCircle": 0.6,
        "osti_macroGather": 0.5, "osti_macroFire": 0.5,
        "osti_tempo": 105.0,
    }, (0.5, 0.5, 0.45, 0.5, 0.1, 0.3),
     "Maximum velocity sensitivity + circle coupling. Touch shapes the network.",
     ["entangled", "velocity", "expressive", "dynamic"]),

    ("Journey Circle", {
        **seats(*[seat(s+1, pattern=PAT_SPARSE, decay=1.5, level=0.5,
                       brightness=0.45) for s in range(8)]),
        "osti_circleAmount": 0.7, "osti_macroCircle": 0.7,
        "osti_macroSpace": 0.5, "osti_reverbMix": 0.35,
        "osti_tempo": 55.0, "osti_macroGather": 0.25,
        "osti_humanize": 0.6,
    }, (0.4, 0.55, 0.35, 0.45, 0.5, 0.1),
     "Long, evolving circle with maximum sympathetic coupling. A journey.",
     ["entangled", "journey", "evolving", "sustained"]),

    ("Compressed Feed", {
        **seats(*[seat(s+1, pattern=PAT_BASIC) for s in range(8)]),
        "osti_compThresh": -25.0, "osti_compRatio": 12.0,
        "osti_compAttack": 1.0, "osti_compRelease": 20.0,
        "osti_macroGather": 0.8, "osti_macroFire": 0.6,
        "osti_circleAmount": 0.4,
    }, (0.5, 0.45, 0.4, 0.65, 0.1, 0.4),
     "Heavy compression creates consistent coupling output. Uniform energy.",
     ["entangled", "compressed", "consistent", "feed"]),

    ("Tuned Donor", {
        **seat(1, instrument=TONGDRUM, tuning=0.0, decay=1.5, brightness=0.6,
               pattern=PAT_BASIC, level=0.65),
        **seat(2, instrument=TONGDRUM, tuning=7.0, decay=1.5, brightness=0.6,
               pattern=PAT_VAR, level=0.55),
        **seat(3, instrument=TONGDRUM, tuning=12.0, decay=1.5, brightness=0.6,
               pattern=PAT_SPARSE, level=0.5),
        **mute_seats(4, 5, 6, 7, 8),
        "osti_circleAmount": 0.6, "osti_macroCircle": 0.6,
        "osti_reverbMix": 0.25,
    }, (0.55, 0.55, 0.3, 0.25, 0.35, 0.05),
     "Harmonic tongue drums as pitched coupling source. Melodic rhythm for the fleet.",
     ["entangled", "tuned", "donor", "melodic"]),
]


# ══════════════════════════════════════════════════════════════════════════════
# FAMILY (15) — OSTINATO coupled with 15 different fleet engines
# ══════════════════════════════════════════════════════════════════════════════

partners = [
    ("OddfeliX",  "Fire x Neon", "Drum circle drives the neon tetra. Rhythm shapes timbre.", {
        **seat(1, instrument=DJEMBE, pattern=PAT_BASIC, level=0.75),
        **seat(2, instrument=CONGA, pattern=PAT_VAR, level=0.6),
        **mute_seats(3, 4, 5, 6, 7, 8),
        "osti_macroGather": 0.7, "osti_macroFire": 0.5,
    }, (0.5, 0.5, 0.4, 0.3, 0.1, 0.25), ["family", "oddfelix", "rhythmic"]),

    ("OddOscar", "Fire x Axolotl", "Ostinato pulses shape Oscar's morphing. Rhythm meets mutation.", {
        **seat(1, instrument=TAIKO, pattern=PAT_BASIC, level=0.8),
        **seat(2, instrument=DJEMBE, articulation=ART_2, pattern=PAT_STYLE_A, level=0.55),
        **mute_seats(3, 4, 5, 6, 7, 8),
        "osti_macroGather": 0.65, "osti_macroFire": 0.55,
    }, (0.45, 0.55, 0.35, 0.25, 0.1, 0.3), ["family", "oddoscar", "morph"]),

    ("Overdub", "Fire x Dub", "Drum circle through tape delay. Rhythmic echoes in olive fog.", {
        **seat(1, instrument=DJEMBE, pattern=PAT_STYLE_A, level=0.7),
        **seat(2, instrument=SURDO, pattern=PAT_BASIC, level=0.65, brightness=0.3),
        **mute_seats(3, 4, 5, 6, 7, 8),
        "osti_macroGather": 0.5, "osti_macroFire": 0.4,
        "osti_reverbMix": 0.3,
    }, (0.4, 0.6, 0.4, 0.25, 0.35, 0.15), ["family", "overdub", "dub", "echo"]),

    ("Odyssey", "Fire x Drift", "The circle drifts. Drums dissolve into wandering tones.", {
        **seat(1, instrument=FRAMEDRUM, pattern=PAT_SPARSE, decay=1.0, level=0.6),
        **seat(2, instrument=TONGDRUM, pattern=PAT_SPARSE, decay=1.5, level=0.5),
        **mute_seats(3, 4, 5, 6, 7, 8),
        "osti_macroGather": 0.3, "osti_humanize": 0.6,
        "osti_macroSpace": 0.4,
    }, (0.4, 0.55, 0.4, 0.2, 0.35, 0.05), ["family", "odyssey", "drift"]),

    ("Opal", "Fire x Grain", "Drum transients feed granular clouds. Percussion dissolves.", {
        **seat(1, instrument=DJEMBE, pattern=PAT_BASIC, level=0.7, brightness=0.6),
        **seat(2, instrument=TABLA, pattern=PAT_VAR, level=0.55, brightness=0.65),
        **mute_seats(3, 4, 5, 6, 7, 8),
        "osti_macroGather": 0.6, "osti_macroFire": 0.4,
        "osti_reverbMix": 0.25,
    }, (0.5, 0.5, 0.4, 0.3, 0.3, 0.15), ["family", "opal", "granular"]),

    ("Onset", "Fire x Drums", "Two drum engines coupled. Physical modeling meets physical modeling.", {
        **seat(1, instrument=TAIKO, pattern=PAT_BASIC, level=0.75, decay=0.8),
        **seat(2, instrument=DJEMBE, pattern=PAT_STYLE_A, level=0.6),
        **seat(3, instrument=CONGA, pattern=PAT_VAR, level=0.55),
        **mute_seats(4, 5, 6, 7, 8),
        "osti_macroGather": 0.7, "osti_macroFire": 0.6,
    }, (0.45, 0.5, 0.45, 0.4, 0.1, 0.4), ["family", "onset", "percussion"]),

    ("Overworld", "Fire x Chip", "Drum circle meets chiptune. Pixel flames.", {
        **seat(1, instrument=BEATBOX, articulation=ART_1, pattern=PAT_BASIC, level=0.75),
        **seat(2, instrument=BEATBOX, articulation=ART_2, pattern=PAT_STYLE_A, level=0.55),
        **seat(3, instrument=BEATBOX, articulation=ART_3, pattern=PAT_DOUBLE, level=0.45),
        **mute_seats(4, 5, 6, 7, 8),
        "osti_macroGather": 0.8, "osti_macroFire": 0.5,
        "osti_tempo": 130.0,
    }, (0.5, 0.4, 0.45, 0.35, 0.05, 0.3), ["family", "overworld", "chiptune"]),

    ("Organon", "Fire x Metabolism", "Drums as biological pulses. The circle breathes and metabolizes.", {
        **seat(1, instrument=SURDO, pattern=PAT_BASIC, decay=1.0, level=0.7, brightness=0.3),
        **seat(2, instrument=FRAMEDRUM, articulation=ART_3, pattern=PAT_VAR, level=0.5, decay=0.6),
        **mute_seats(3, 4, 5, 6, 7, 8),
        "osti_macroGather": 0.4, "osti_humanize": 0.5,
        "osti_circleAmount": 0.5,
    }, (0.35, 0.6, 0.4, 0.2, 0.2, 0.1), ["family", "organon", "metabolic"]),

    ("Obese", "Fire x Fat", "Drum transients through saturation. Fattened rhythms.", {
        **seat(1, instrument=DJEMBE, pattern=PAT_BASIC, level=0.8, brightness=0.5),
        **seat(2, instrument=CAJON, pattern=PAT_BASIC, level=0.7, brightness=0.45),
        **mute_seats(3, 4, 5, 6, 7, 8),
        "osti_macroGather": 0.7, "osti_macroFire": 0.6,
        "osti_compThresh": -18.0,
    }, (0.4, 0.55, 0.35, 0.3, 0.1, 0.45), ["family", "obese", "saturated"]),

    ("Ouroboros", "Fire x Feedback", "Drum circle feeding a strange attractor. Rhythmic chaos.", {
        **seat(1, instrument=DJEMBE, pattern=PAT_STYLE_B, level=0.7),
        **seat(2, instrument=DOUMBEK, pattern=PAT_STYLE_C, level=0.6),
        **seat(3, instrument=TABLA, pattern=PAT_STYLE_A, level=0.55),
        **mute_seats(4, 5, 6, 7, 8),
        "osti_macroGather": 0.5, "osti_macroFire": 0.5,
        "osti_circleAmount": 0.6,
    }, (0.45, 0.45, 0.5, 0.35, 0.15, 0.35), ["family", "ouroboros", "feedback"]),

    ("Obscura", "Fire x Silver", "Drum shadows in daguerreotype. Percussion preserved in silver.", {
        **seat(1, instrument=TAIKO, pattern=PAT_SPARSE, decay=1.2, level=0.7, brightness=0.4),
        **seat(2, instrument=FRAMEDRUM, pattern=PAT_SPARSE, decay=0.8, level=0.5),
        **mute_seats(3, 4, 5, 6, 7, 8),
        "osti_macroGather": 0.4, "osti_macroSpace": 0.5,
        "osti_reverbMix": 0.35,
    }, (0.35, 0.5, 0.2, 0.15, 0.45, 0.1), ["family", "obscura", "vintage"]),

    ("Obrix", "Fire x Reef", "Drum circle meets coral reef. Physical and modular collide.", {
        **seat(1, instrument=DJEMBE, pattern=PAT_BASIC, level=0.75),
        **seat(2, instrument=CONGA, pattern=PAT_VAR, level=0.65),
        **seat(3, instrument=TABLA, pattern=PAT_STYLE_A, level=0.55),
        **mute_seats(4, 5, 6, 7, 8),
        "osti_macroGather": 0.6, "osti_macroFire": 0.5,
        "osti_circleAmount": 0.4,
    }, (0.5, 0.5, 0.4, 0.3, 0.15, 0.25), ["family", "obrix", "reef"]),

    ("Oceanic", "Fire x Tides", "The drum circle at the shore. Phosphorescent rhythm.", {
        **seat(1, instrument=SURDO, pattern=PAT_BASIC, decay=1.0, level=0.7, brightness=0.25),
        **seat(2, instrument=DJEMBE, pattern=PAT_SPARSE, level=0.5),
        **seat(3, instrument=FRAMEDRUM, articulation=ART_3, pattern=PAT_VAR, level=0.4, decay=0.5),
        **mute_seats(4, 5, 6, 7, 8),
        "osti_macroGather": 0.4, "osti_macroSpace": 0.5,
        "osti_humanize": 0.5,
    }, (0.35, 0.6, 0.35, 0.25, 0.45, 0.1), ["family", "oceanic", "tidal"]),

    ("Osprey", "Fire x Shore", "Where the drum circle meets the coastline. Sand and rhythm.", {
        **seat(1, instrument=DOUMBEK, pattern=PAT_STYLE_A, level=0.7, brightness=0.6),
        **seat(2, instrument=CONGA, pattern=PAT_VAR, level=0.6),
        **mute_seats(3, 4, 5, 6, 7, 8),
        "osti_macroGather": 0.5, "osti_macroFire": 0.4,
        "osti_reverbMix": 0.25,
    }, (0.5, 0.55, 0.35, 0.2, 0.3, 0.15), ["family", "osprey", "shore"]),

    ("Overlap", "Fire x Entangle", "Drum circle through topological coupling. Knots in rhythm.", {
        **seat(1, instrument=DJEMBE, pattern=PAT_BASIC, level=0.7),
        **seat(2, instrument=TAIKO, pattern=PAT_BASIC, level=0.65),
        **seat(3, instrument=CONGA, pattern=PAT_VAR, level=0.55),
        **mute_seats(4, 5, 6, 7, 8),
        "osti_macroGather": 0.6, "osti_macroFire": 0.5,
        "osti_circleAmount": 0.5,
    }, (0.45, 0.5, 0.4, 0.3, 0.2, 0.2), ["family", "overlap", "topology"]),
]

FAMILY_PRESETS = []
for partner_name, preset_name, desc, overrides, dna, tags in partners:
    FAMILY_PRESETS.append(make_preset(preset_name, "Family", overrides, dna, desc, tags,
        engines=["Ostinato", partner_name],
        coupling=[{"engineA": "Ostinato", "engineB": partner_name,
                   "type": "AudioToFM", "amount": 0.5}],
        coupling_intensity="Medium"))


# ══════════════════════════════════════════════════════════════════════════════
# GENERATE ALL PRESETS
# ══════════════════════════════════════════════════════════════════════════════

def main():
    count = 0
    # Foundation
    for name, overrides, dna, desc, tags in FOUNDATION:
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

    print(f"Generated {count} OSTINATO presets across 7 moods")
    # Distribution check
    from collections import Counter
    moods = Counter()
    moods["Foundation"] = len(FOUNDATION)
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
