#!/usr/bin/env python3
"""Generate XOnset .xometa preset files for the XOmnibus gallery."""

import json
import os
from datetime import date

PRESET_DIR = os.path.join(os.path.dirname(os.path.dirname(__file__)), "Presets", "XOmnibus")

# Default voice values (matching kVoiceCfg in OnsetEngine.h)
VOICE_DEFAULTS = {
    1: {"blend": 0.2, "algoMode": 1, "pitch": 0, "decay": 0.5,  "tone": 0.5, "snap": 0.3, "body": 0.5, "character": 0.0, "level": 0.7, "pan": 0.0, "envShape": 0},
    2: {"blend": 0.5, "algoMode": 0, "pitch": 0, "decay": 0.3,  "tone": 0.5, "snap": 0.3, "body": 0.5, "character": 0.0, "level": 0.7, "pan": 0.0, "envShape": 0},
    3: {"blend": 0.7, "algoMode": 0, "pitch": 0, "decay": 0.05, "tone": 0.5, "snap": 0.3, "body": 0.5, "character": 0.0, "level": 0.7, "pan": 0.0, "envShape": 0},
    4: {"blend": 0.7, "algoMode": 0, "pitch": 0, "decay": 0.4,  "tone": 0.5, "snap": 0.3, "body": 0.5, "character": 0.0, "level": 0.7, "pan": 0.0, "envShape": 0},
    5: {"blend": 0.4, "algoMode": 3, "pitch": 0, "decay": 0.25, "tone": 0.5, "snap": 0.3, "body": 0.5, "character": 0.0, "level": 0.7, "pan": 0.0, "envShape": 0},
    6: {"blend": 0.3, "algoMode": 1, "pitch": 0, "decay": 0.4,  "tone": 0.5, "snap": 0.3, "body": 0.5, "character": 0.0, "level": 0.7, "pan": 0.0, "envShape": 0},
    7: {"blend": 0.6, "algoMode": 2, "pitch": 0, "decay": 0.3,  "tone": 0.5, "snap": 0.3, "body": 0.5, "character": 0.0, "level": 0.7, "pan": 0.0, "envShape": 0},
    8: {"blend": 0.8, "algoMode": 1, "pitch": 0, "decay": 0.35, "tone": 0.5, "snap": 0.3, "body": 0.5, "character": 0.0, "level": 0.7, "pan": 0.0, "envShape": 0},
}

GLOBAL_DEFAULTS = {
    "perc_level": 0.8, "perc_drive": 0.0, "perc_masterTone": 0.5,
    "perc_macro_machine": 0.5, "perc_macro_punch": 0.5, "perc_macro_space": 0.0, "perc_macro_mutate": 0.0,
    "perc_xvc_kick_to_snare_filter": 0.15, "perc_xvc_snare_to_hat_decay": 0.10,
    "perc_xvc_kick_to_tom_pitch": 0.0, "perc_xvc_snare_to_perc_blend": 0.0,
    "perc_xvc_hat_choke": 1.0, "perc_xvc_global_amount": 0.5,
    "perc_char_grit": 0.0, "perc_char_warmth": 0.5,
    "perc_fx_delay_time": 0.3, "perc_fx_delay_feedback": 0.3, "perc_fx_delay_mix": 0.0,
    "perc_fx_reverb_size": 0.4, "perc_fx_reverb_decay": 0.3, "perc_fx_reverb_mix": 0.0,
    "perc_fx_lofi_bits": 16.0, "perc_fx_lofi_mix": 0.0,
}


def make_params(voice_overrides=None, global_overrides=None):
    """Build full parameter dict from defaults + overrides."""
    params = {}
    vo = voice_overrides or {}
    for v in range(1, 9):
        prefix = f"perc_v{v}_"
        defaults = VOICE_DEFAULTS[v].copy()
        if v in vo:
            defaults.update(vo[v])
        for key, val in defaults.items():
            params[prefix + key] = val

    g = GLOBAL_DEFAULTS.copy()
    if global_overrides:
        g.update(global_overrides)
    params.update(g)
    return params


def make_preset(name, mood, description, tags, params, voice_overrides=None,
                global_overrides=None, coupling_intensity="None",
                dna=None):
    """Generate a complete .xometa preset dict."""
    return {
        "schema_version": 1,
        "name": name,
        "mood": mood,
        "engines": ["Onset"],
        "author": "XO_OX",
        "version": "1.0.0",
        "description": description,
        "tags": tags,
        "macroLabels": ["MACHINE", "PUNCH", "SPACE", "MUTATE"],
        "couplingIntensity": coupling_intensity,
        "tempo": None,
        "created": str(date.today()),
        "legacy": {
            "sourceInstrument": "XOnset",
            "sourceCategory": mood,
            "sourcePresetName": None
        },
        "parameters": {
            "Onset": params if params else make_params(voice_overrides, global_overrides)
        },
        "coupling": None,
        "sequencer": None,
        "dna": dna or {"brightness": 0.5, "warmth": 0.5, "movement": 0.3, "density": 0.5, "space": 0.2, "aggression": 0.5}
    }


def write_preset(preset, mood):
    """Write preset to the correct mood directory."""
    subdir = os.path.join(PRESET_DIR, mood, "Onset")
    os.makedirs(subdir, exist_ok=True)
    filename = preset["name"].replace(" ", "_") + ".xometa"
    path = os.path.join(subdir, filename)
    with open(path, "w") as f:
        json.dump(preset, f, indent=2)
    print(f"  Written: {mood}/Onset/{filename}")


# =============================================================================
# HERO PRESETS (10)
# =============================================================================

HERO_PRESETS = [
    # 1. 808 Reborn — pure circuit, deep sub, classic
    make_preset(
        "808 Reborn", "Foundation",
        "Classic 808 character through circuit models. Deep sub kick, snappy snare, metallic hats.",
        ["808", "circuit", "classic", "deep", "sub"],
        make_params(
            voice_overrides={
                1: {"blend": 0.0, "decay": 0.8, "tone": 0.4, "snap": 0.5, "body": 0.7, "character": 0.15},
                2: {"blend": 0.1, "decay": 0.35, "tone": 0.6, "snap": 0.5, "body": 0.4, "character": 0.2},
                3: {"blend": 0.1, "decay": 0.04, "tone": 0.7, "snap": 0.4},
                4: {"blend": 0.1, "decay": 0.5, "tone": 0.6},
                5: {"blend": 0.05, "decay": 0.3, "snap": 0.6, "character": 0.15},
                6: {"blend": 0.0, "decay": 0.45, "body": 0.65, "snap": 0.4},
                7: {"blend": 0.05, "decay": 0.25, "snap": 0.3},
                8: {"blend": 0.1, "decay": 0.3},
            },
            global_overrides={"perc_char_grit": 0.15, "perc_char_warmth": 0.6, "perc_xvc_global_amount": 0.3}
        ),
        dna={"brightness": 0.35, "warmth": 0.75, "movement": 0.15, "density": 0.5, "space": 0.1, "aggression": 0.3}
    ),

    # 2. Membrane Theory — modal resonator showcase
    make_preset(
        "Membrane Theory", "Prism",
        "Every voice tuned to Modal resonator. Ringing metallic membranes with inharmonic partials.",
        ["modal", "resonant", "metallic", "ringing", "tuned"],
        make_params(
            voice_overrides={
                1: {"blend": 0.85, "algoMode": 1, "decay": 0.6, "character": 0.4, "tone": 0.6},
                2: {"blend": 0.9, "algoMode": 1, "decay": 0.4, "character": 0.5, "tone": 0.7},
                3: {"blend": 0.95, "algoMode": 1, "decay": 0.15, "character": 0.6, "tone": 0.8},
                4: {"blend": 0.95, "algoMode": 1, "decay": 0.5, "character": 0.6, "tone": 0.75},
                5: {"blend": 0.85, "algoMode": 1, "decay": 0.35, "character": 0.45, "snap": 0.5},
                6: {"blend": 0.8, "algoMode": 1, "decay": 0.5, "character": 0.35, "tone": 0.55},
                7: {"blend": 0.9, "algoMode": 1, "decay": 0.4, "character": 0.55, "tone": 0.65},
                8: {"blend": 0.95, "algoMode": 1, "decay": 0.45, "character": 0.7, "tone": 0.85},
            },
            global_overrides={"perc_fx_reverb_mix": 0.2, "perc_fx_reverb_decay": 0.4}
        ),
        dna={"brightness": 0.75, "warmth": 0.3, "movement": 0.25, "density": 0.6, "space": 0.35, "aggression": 0.4}
    ),

    # 3. Quantum Kit — both layers audible at blend center
    make_preset(
        "Quantum Kit", "Flux",
        "Every voice at the blend midpoint. Circuit and algorithm superimposed — both heard simultaneously.",
        ["hybrid", "dual-layer", "quantum", "blend", "complex"],
        make_params(
            voice_overrides={
                1: {"blend": 0.5, "decay": 0.6, "snap": 0.4, "body": 0.6, "character": 0.2},
                2: {"blend": 0.5, "decay": 0.3, "snap": 0.45, "character": 0.25},
                3: {"blend": 0.5, "decay": 0.06, "tone": 0.65},
                4: {"blend": 0.5, "decay": 0.35, "tone": 0.6},
                5: {"blend": 0.5, "decay": 0.28, "snap": 0.4, "character": 0.2},
                6: {"blend": 0.5, "decay": 0.45, "body": 0.55, "character": 0.15},
                7: {"blend": 0.5, "decay": 0.3, "character": 0.3},
                8: {"blend": 0.5, "decay": 0.35, "character": 0.25},
            },
            global_overrides={"perc_char_grit": 0.1}
        ),
        dna={"brightness": 0.55, "warmth": 0.5, "movement": 0.35, "density": 0.65, "space": 0.15, "aggression": 0.45}
    ),

    # 4. Living Machine — MACHINE macro sweep from analog to digital
    make_preset(
        "Living Machine", "Foundation",
        "Designed for MACHINE macro performance. Sweep from pure analog circuit to pure digital algorithm.",
        ["performance", "sweep", "macro", "morph", "live"],
        make_params(
            voice_overrides={
                1: {"blend": 0.3, "decay": 0.65, "snap": 0.45, "body": 0.6, "character": 0.2},
                2: {"blend": 0.4, "decay": 0.3, "snap": 0.4, "character": 0.25},
                3: {"blend": 0.5, "decay": 0.06, "tone": 0.6},
                4: {"blend": 0.5, "decay": 0.4, "tone": 0.55},
                5: {"blend": 0.35, "decay": 0.25, "snap": 0.5},
                6: {"blend": 0.25, "decay": 0.45, "body": 0.6},
                7: {"blend": 0.45, "decay": 0.3, "character": 0.2},
                8: {"blend": 0.6, "decay": 0.35, "character": 0.3},
            },
            global_overrides={"perc_xvc_kick_to_snare_filter": 0.2, "perc_xvc_global_amount": 0.6}
        ),
        dna={"brightness": 0.5, "warmth": 0.55, "movement": 0.6, "density": 0.55, "space": 0.15, "aggression": 0.4}
    ),

    # 5. Time Stretch — Karplus-Strong heavy, metallic sustain
    make_preset(
        "Time Stretch", "Atmosphere",
        "Karplus-Strong string models on every voice. Plucked metallic sustain, resonant decay tails.",
        ["karplus-strong", "plucked", "sustain", "metallic", "resonant"],
        make_params(
            voice_overrides={
                1: {"blend": 0.8, "algoMode": 2, "decay": 1.2, "tone": 0.45, "snap": 0.5, "character": 0.3},
                2: {"blend": 0.85, "algoMode": 2, "decay": 0.8, "tone": 0.55, "snap": 0.6, "character": 0.4},
                3: {"blend": 0.9, "algoMode": 2, "decay": 0.3, "tone": 0.7, "snap": 0.4, "character": 0.5},
                4: {"blend": 0.9, "algoMode": 2, "decay": 0.9, "tone": 0.65, "character": 0.45},
                5: {"blend": 0.75, "algoMode": 2, "decay": 0.5, "snap": 0.5, "character": 0.35},
                6: {"blend": 0.8, "algoMode": 2, "decay": 1.0, "tone": 0.4, "body": 0.6, "character": 0.25},
                7: {"blend": 0.85, "algoMode": 2, "decay": 0.7, "character": 0.5},
                8: {"blend": 0.9, "algoMode": 2, "decay": 0.8, "character": 0.55},
            },
            global_overrides={"perc_fx_reverb_mix": 0.3, "perc_fx_reverb_size": 0.5, "perc_fx_reverb_decay": 0.5}
        ),
        dna={"brightness": 0.6, "warmth": 0.4, "movement": 0.2, "density": 0.4, "space": 0.55, "aggression": 0.25}
    ),

    # 6. Dub Pressure — echo throws, tape character
    make_preset(
        "Dub Pressure", "Foundation",
        "Dark tape echo on the kit. Delay throws on snare and clap, warm dub pressure.",
        ["dub", "echo", "tape", "delay", "warm"],
        make_params(
            voice_overrides={
                1: {"blend": 0.15, "decay": 0.7, "body": 0.7, "snap": 0.45, "character": 0.1},
                2: {"blend": 0.25, "decay": 0.35, "snap": 0.5, "tone": 0.55},
                3: {"blend": 0.2, "decay": 0.05, "tone": 0.6},
                4: {"blend": 0.2, "decay": 0.45, "tone": 0.55},
                5: {"blend": 0.15, "decay": 0.3, "snap": 0.55},
                6: {"blend": 0.1, "decay": 0.5, "body": 0.6},
                7: {"blend": 0.2, "decay": 0.3},
                8: {"blend": 0.25, "decay": 0.35},
            },
            global_overrides={
                "perc_char_grit": 0.2, "perc_char_warmth": 0.65,
                "perc_fx_delay_time": 0.375, "perc_fx_delay_feedback": 0.55, "perc_fx_delay_mix": 0.3,
                "perc_fx_reverb_mix": 0.15, "perc_fx_reverb_size": 0.35,
            }
        ),
        dna={"brightness": 0.35, "warmth": 0.7, "movement": 0.45, "density": 0.5, "space": 0.6, "aggression": 0.3}
    ),

    # 7. Future 909 — hybrid blend on snare/hat, modern punch
    make_preset(
        "Future 909", "Foundation",
        "909-inspired with algorithmic enhancement. Snare and hats blended 50/50 for a hybrid future sound.",
        ["909", "hybrid", "modern", "punchy", "bright"],
        make_params(
            voice_overrides={
                1: {"blend": 0.1, "decay": 0.45, "snap": 0.6, "body": 0.5, "character": 0.1},
                2: {"blend": 0.5, "decay": 0.25, "snap": 0.65, "tone": 0.65, "character": 0.2},
                3: {"blend": 0.5, "decay": 0.03, "tone": 0.75, "snap": 0.5},
                4: {"blend": 0.5, "decay": 0.35, "tone": 0.7},
                5: {"blend": 0.45, "decay": 0.2, "snap": 0.6, "character": 0.15},
                6: {"blend": 0.15, "decay": 0.35, "snap": 0.55, "body": 0.55},
                7: {"blend": 0.4, "decay": 0.2, "snap": 0.5, "character": 0.15},
                8: {"blend": 0.5, "decay": 0.25, "character": 0.2},
            },
            global_overrides={"perc_drive": 0.15, "perc_masterTone": 0.6}
        ),
        dna={"brightness": 0.65, "warmth": 0.4, "movement": 0.2, "density": 0.55, "space": 0.1, "aggression": 0.55}
    ),

    # 8. Particle Drums — designed for OPAL coupling
    make_preset(
        "Particle Drums", "Entangled",
        "Designed for ONSET x OPAL coupling. Sharp transients that granulate beautifully through time clouds.",
        ["coupling", "granular", "transient", "opal", "scattered"],
        make_params(
            voice_overrides={
                1: {"blend": 0.3, "decay": 0.4, "snap": 0.7, "body": 0.4, "character": 0.25},
                2: {"blend": 0.6, "decay": 0.2, "snap": 0.75, "tone": 0.7, "character": 0.3},
                3: {"blend": 0.7, "decay": 0.03, "snap": 0.6, "tone": 0.8},
                4: {"blend": 0.7, "decay": 0.25, "tone": 0.75},
                5: {"blend": 0.5, "decay": 0.15, "snap": 0.7, "character": 0.25},
                6: {"blend": 0.35, "decay": 0.3, "snap": 0.6, "body": 0.5},
                7: {"blend": 0.65, "algoMode": 2, "decay": 0.2, "snap": 0.6, "character": 0.35},
                8: {"blend": 0.75, "decay": 0.25, "snap": 0.55, "character": 0.3},
            },
            global_overrides={"perc_fx_reverb_mix": 0.1, "perc_masterTone": 0.6}
        ),
        coupling_intensity="Medium",
        dna={"brightness": 0.7, "warmth": 0.3, "movement": 0.5, "density": 0.45, "space": 0.4, "aggression": 0.5}
    ),

    # 9. Analog Heart — pure circuit, zero blend
    make_preset(
        "Analog Heart", "Foundation",
        "Pure circuit models. Zero algorithmic blend. The warm beating heart of analog drum machines.",
        ["analog", "circuit", "pure", "warm", "classic"],
        make_params(
            voice_overrides={
                1: {"blend": 0.0, "decay": 0.55, "tone": 0.45, "snap": 0.35, "body": 0.6},
                2: {"blend": 0.0, "decay": 0.3, "tone": 0.55, "snap": 0.4},
                3: {"blend": 0.0, "decay": 0.05, "tone": 0.6},
                4: {"blend": 0.0, "decay": 0.4, "tone": 0.55},
                5: {"blend": 0.0, "decay": 0.25, "snap": 0.45},
                6: {"blend": 0.0, "decay": 0.4, "body": 0.6},
                7: {"blend": 0.0, "decay": 0.3, "snap": 0.35},
                8: {"blend": 0.0, "decay": 0.3},
            },
            global_overrides={"perc_char_warmth": 0.6}
        ),
        dna={"brightness": 0.4, "warmth": 0.7, "movement": 0.1, "density": 0.5, "space": 0.1, "aggression": 0.3}
    ),

    # 10. Mutant Factory — MUTATE macro cranked, evolving kit
    make_preset(
        "Mutant Factory", "Flux",
        "MUTATE macro at full. Every hit drifts. The kit evolves, never repeating the same sound twice.",
        ["mutant", "evolving", "random", "drift", "generative"],
        make_params(
            voice_overrides={
                1: {"blend": 0.4, "decay": 0.5, "snap": 0.4, "body": 0.55, "character": 0.3},
                2: {"blend": 0.5, "decay": 0.3, "snap": 0.45, "character": 0.35},
                3: {"blend": 0.6, "decay": 0.06, "tone": 0.6, "character": 0.25},
                4: {"blend": 0.6, "decay": 0.35, "character": 0.3},
                5: {"blend": 0.45, "decay": 0.25, "snap": 0.5, "character": 0.3},
                6: {"blend": 0.35, "decay": 0.4, "body": 0.55, "character": 0.25},
                7: {"blend": 0.55, "decay": 0.3, "character": 0.4},
                8: {"blend": 0.65, "decay": 0.35, "character": 0.35},
            },
            global_overrides={
                "perc_macro_mutate": 0.8,
                "perc_char_grit": 0.15,
                "perc_fx_delay_mix": 0.1, "perc_fx_delay_time": 0.25, "perc_fx_delay_feedback": 0.35,
            }
        ),
        dna={"brightness": 0.55, "warmth": 0.45, "movement": 0.85, "density": 0.55, "space": 0.25, "aggression": 0.5}
    ),
]

# =============================================================================
# REPLICA & REIMAGINED KITS
# =============================================================================

REPLICA_PRESETS = [
    # 808 Faithful
    make_preset(
        "808 Deep Sub", "Foundation",
        "Faithful 808 sub bass kick with long decay. Pure circuit warmth.",
        ["808", "sub", "deep", "bass", "classic"],
        make_params(
            voice_overrides={
                1: {"blend": 0.0, "decay": 1.0, "tone": 0.35, "snap": 0.55, "body": 0.8, "character": 0.1},
                2: {"blend": 0.0, "decay": 0.3, "tone": 0.55, "snap": 0.45},
                3: {"blend": 0.0, "decay": 0.04, "tone": 0.65},
                4: {"blend": 0.0, "decay": 0.45, "tone": 0.6},
                5: {"blend": 0.0, "decay": 0.3, "snap": 0.55},
                6: {"blend": 0.0, "decay": 0.5, "body": 0.7, "pitch": -5},
                7: {"blend": 0.0, "decay": 0.25},
                8: {"blend": 0.0, "decay": 0.3},
            },
            global_overrides={"perc_char_warmth": 0.65}
        ),
        dna={"brightness": 0.3, "warmth": 0.8, "movement": 0.1, "density": 0.45, "space": 0.1, "aggression": 0.25}
    ),

    # 808 Reimagined
    make_preset(
        "808 Ghost Circuit", "Foundation",
        "808 body with FM ghost notes bleeding through. The machine remembers its digital future.",
        ["808", "reimagined", "FM", "ghost", "hybrid"],
        make_params(
            voice_overrides={
                1: {"blend": 0.15, "decay": 0.7, "tone": 0.4, "snap": 0.5, "body": 0.7, "character": 0.2},
                2: {"blend": 0.3, "decay": 0.3, "tone": 0.6, "snap": 0.5, "character": 0.25},
                3: {"blend": 0.25, "decay": 0.05, "tone": 0.7, "character": 0.15},
                4: {"blend": 0.25, "decay": 0.45, "tone": 0.65, "character": 0.15},
                5: {"blend": 0.2, "decay": 0.28, "snap": 0.55, "character": 0.2},
                6: {"blend": 0.1, "decay": 0.45, "body": 0.65},
                7: {"blend": 0.2, "decay": 0.25, "character": 0.2},
                8: {"blend": 0.3, "decay": 0.3, "character": 0.25},
            },
            global_overrides={"perc_char_grit": 0.1, "perc_char_warmth": 0.55}
        ),
        dna={"brightness": 0.4, "warmth": 0.65, "movement": 0.2, "density": 0.55, "space": 0.1, "aggression": 0.35}
    ),

    # 909 Faithful
    make_preset(
        "909 Bright Punch", "Foundation",
        "Faithful 909 character. Bright, punchy, tight. Higher snap, shorter decays.",
        ["909", "punchy", "bright", "tight", "classic"],
        make_params(
            voice_overrides={
                1: {"blend": 0.0, "decay": 0.35, "tone": 0.55, "snap": 0.65, "body": 0.45, "character": 0.05},
                2: {"blend": 0.0, "decay": 0.2, "tone": 0.7, "snap": 0.7, "character": 0.15},
                3: {"blend": 0.0, "decay": 0.03, "tone": 0.75, "snap": 0.5},
                4: {"blend": 0.0, "decay": 0.3, "tone": 0.7},
                5: {"blend": 0.0, "decay": 0.2, "snap": 0.65, "character": 0.1},
                6: {"blend": 0.0, "decay": 0.3, "snap": 0.55, "body": 0.5},
                7: {"blend": 0.0, "decay": 0.2, "snap": 0.5},
                8: {"blend": 0.0, "decay": 0.25},
            },
            global_overrides={"perc_drive": 0.1, "perc_masterTone": 0.6}
        ),
        dna={"brightness": 0.65, "warmth": 0.35, "movement": 0.15, "density": 0.55, "space": 0.05, "aggression": 0.55}
    ),

    # 909 Reimagined
    make_preset(
        "909 Modal Ring", "Prism",
        "909 snap with Modal resonators ringing underneath. Metallic 909 with tuned membrane overtones.",
        ["909", "reimagined", "modal", "ringing", "metallic"],
        make_params(
            voice_overrides={
                1: {"blend": 0.35, "algoMode": 1, "decay": 0.4, "snap": 0.6, "body": 0.5, "character": 0.3},
                2: {"blend": 0.4, "algoMode": 1, "decay": 0.25, "snap": 0.65, "tone": 0.65, "character": 0.35},
                3: {"blend": 0.45, "algoMode": 1, "decay": 0.04, "tone": 0.7, "character": 0.3},
                4: {"blend": 0.45, "algoMode": 1, "decay": 0.35, "tone": 0.7, "character": 0.3},
                5: {"blend": 0.35, "algoMode": 1, "decay": 0.2, "snap": 0.6, "character": 0.25},
                6: {"blend": 0.3, "algoMode": 1, "decay": 0.35, "snap": 0.55, "character": 0.3},
                7: {"blend": 0.4, "algoMode": 1, "decay": 0.2, "character": 0.35},
                8: {"blend": 0.45, "algoMode": 1, "decay": 0.3, "character": 0.4},
            },
            global_overrides={"perc_drive": 0.1, "perc_masterTone": 0.6, "perc_fx_reverb_mix": 0.15}
        ),
        dna={"brightness": 0.7, "warmth": 0.35, "movement": 0.2, "density": 0.6, "space": 0.3, "aggression": 0.5}
    ),

    # Linndrum Reimagined
    make_preset(
        "Linn Plucked", "Prism",
        "LinnDrum reimagined with Karplus-Strong. Every hit is a plucked string tuned to drum pitches.",
        ["linndrum", "reimagined", "plucked", "string", "karplus-strong"],
        make_params(
            voice_overrides={
                1: {"blend": 0.7, "algoMode": 2, "decay": 0.6, "tone": 0.45, "snap": 0.5, "body": 0.55, "character": 0.3},
                2: {"blend": 0.75, "algoMode": 2, "decay": 0.35, "tone": 0.6, "snap": 0.55, "character": 0.35},
                3: {"blend": 0.8, "algoMode": 2, "decay": 0.08, "tone": 0.7, "snap": 0.4, "character": 0.4},
                4: {"blend": 0.8, "algoMode": 2, "decay": 0.4, "tone": 0.65, "character": 0.4},
                5: {"blend": 0.7, "algoMode": 2, "decay": 0.25, "snap": 0.5, "character": 0.3},
                6: {"blend": 0.65, "algoMode": 2, "decay": 0.5, "body": 0.6, "character": 0.25},
                7: {"blend": 0.75, "algoMode": 2, "decay": 0.3, "character": 0.4},
                8: {"blend": 0.8, "algoMode": 2, "decay": 0.35, "character": 0.45},
            },
            global_overrides={"perc_fx_reverb_mix": 0.2, "perc_fx_reverb_size": 0.45}
        ),
        dna={"brightness": 0.6, "warmth": 0.45, "movement": 0.15, "density": 0.5, "space": 0.4, "aggression": 0.3}
    ),

    # Phase Distortion kit
    make_preset(
        "CZ Crystal", "Prism",
        "Casio CZ-inspired drums. Phase distortion on every voice — crystalline, sharp, digital warmth.",
        ["CZ", "phase-distortion", "crystal", "digital", "sharp"],
        make_params(
            voice_overrides={
                1: {"blend": 0.85, "algoMode": 3, "decay": 0.5, "snap": 0.5, "body": 0.5, "character": 0.5},
                2: {"blend": 0.9, "algoMode": 3, "decay": 0.25, "snap": 0.55, "tone": 0.65, "character": 0.55},
                3: {"blend": 0.9, "algoMode": 3, "decay": 0.05, "tone": 0.75, "character": 0.5},
                4: {"blend": 0.9, "algoMode": 3, "decay": 0.35, "tone": 0.7, "character": 0.5},
                5: {"blend": 0.85, "algoMode": 3, "decay": 0.2, "snap": 0.6, "character": 0.45},
                6: {"blend": 0.8, "algoMode": 3, "decay": 0.4, "body": 0.55, "character": 0.45},
                7: {"blend": 0.85, "algoMode": 3, "decay": 0.25, "character": 0.55},
                8: {"blend": 0.9, "algoMode": 3, "decay": 0.3, "character": 0.6},
            },
            global_overrides={"perc_masterTone": 0.6}
        ),
        dna={"brightness": 0.75, "warmth": 0.3, "movement": 0.15, "density": 0.55, "space": 0.1, "aggression": 0.45}
    ),

    # FM Percussion kit
    make_preset(
        "FM Percussion Lab", "Prism",
        "Pure FM synthesis drums. 2-operator percussion with self-feedback. DX meets drum machine.",
        ["FM", "DX", "digital", "operator", "synthesis"],
        make_params(
            voice_overrides={
                1: {"blend": 0.9, "algoMode": 0, "decay": 0.5, "snap": 0.5, "body": 0.5, "character": 0.4},
                2: {"blend": 0.95, "algoMode": 0, "decay": 0.25, "snap": 0.55, "tone": 0.6, "character": 0.5},
                3: {"blend": 0.95, "algoMode": 0, "decay": 0.05, "tone": 0.7, "character": 0.45},
                4: {"blend": 0.95, "algoMode": 0, "decay": 0.3, "tone": 0.65, "character": 0.45},
                5: {"blend": 0.9, "algoMode": 0, "decay": 0.2, "snap": 0.6, "character": 0.5},
                6: {"blend": 0.85, "algoMode": 0, "decay": 0.4, "body": 0.55, "character": 0.35},
                7: {"blend": 0.9, "algoMode": 0, "decay": 0.25, "character": 0.5},
                8: {"blend": 0.95, "algoMode": 0, "decay": 0.3, "character": 0.55},
            },
            global_overrides={"perc_masterTone": 0.55}
        ),
        dna={"brightness": 0.7, "warmth": 0.25, "movement": 0.2, "density": 0.6, "space": 0.1, "aggression": 0.5}
    ),

    # LoFi degraded kit
    make_preset(
        "Dusty Circuits", "Flux",
        "Lo-fi degraded drum machine. Bitcrushed, warm, dusty. The sound of worn-out hardware.",
        ["lofi", "dusty", "crushed", "warm", "degraded"],
        make_params(
            voice_overrides={
                1: {"blend": 0.1, "decay": 0.6, "tone": 0.4, "snap": 0.4, "body": 0.65, "character": 0.2},
                2: {"blend": 0.15, "decay": 0.3, "tone": 0.5, "snap": 0.4, "character": 0.15},
                3: {"blend": 0.15, "decay": 0.05, "tone": 0.55},
                4: {"blend": 0.15, "decay": 0.4, "tone": 0.5},
                5: {"blend": 0.1, "decay": 0.25, "snap": 0.4},
                6: {"blend": 0.05, "decay": 0.45, "body": 0.6},
                7: {"blend": 0.1, "decay": 0.25},
                8: {"blend": 0.15, "decay": 0.3},
            },
            global_overrides={
                "perc_char_grit": 0.35, "perc_char_warmth": 0.7,
                "perc_fx_lofi_bits": 10.0, "perc_fx_lofi_mix": 0.5,
                "perc_masterTone": 0.4,
            }
        ),
        dna={"brightness": 0.3, "warmth": 0.7, "movement": 0.15, "density": 0.45, "space": 0.1, "aggression": 0.35}
    ),

    # ONSET x OVERWORLD coupling kit
    make_preset(
        "Chip Arcade", "Entangled",
        "Designed for ONSET x OVERWORLD coupling. Chip noise modulates blend — arcade meets analog.",
        ["coupling", "chip", "overworld", "arcade", "NES"],
        make_params(
            voice_overrides={
                1: {"blend": 0.35, "decay": 0.5, "snap": 0.5, "body": 0.55, "character": 0.2},
                2: {"blend": 0.5, "decay": 0.25, "snap": 0.55, "tone": 0.6, "character": 0.3},
                3: {"blend": 0.6, "decay": 0.04, "tone": 0.7, "character": 0.2},
                4: {"blend": 0.6, "decay": 0.3, "tone": 0.65, "character": 0.2},
                5: {"blend": 0.4, "decay": 0.2, "snap": 0.55, "character": 0.25},
                6: {"blend": 0.3, "decay": 0.4, "body": 0.55},
                7: {"blend": 0.5, "decay": 0.25, "character": 0.3},
                8: {"blend": 0.55, "decay": 0.3, "character": 0.3},
            },
            global_overrides={
                "perc_xvc_global_amount": 0.6,
                "perc_fx_lofi_bits": 12.0, "perc_fx_lofi_mix": 0.2,
            }
        ),
        coupling_intensity="Medium",
        dna={"brightness": 0.6, "warmth": 0.35, "movement": 0.45, "density": 0.5, "space": 0.15, "aggression": 0.5}
    ),

    # ONSET x SNAP coupling kit
    make_preset(
        "Pluck Triggers", "Entangled",
        "Designed for ONSET x SNAP coupling. Karplus-Strong pluck envelopes drive drum filter and decay.",
        ["coupling", "pluck", "snap", "locked", "groove"],
        make_params(
            voice_overrides={
                1: {"blend": 0.25, "decay": 0.55, "snap": 0.45, "body": 0.6, "character": 0.15},
                2: {"blend": 0.4, "decay": 0.3, "snap": 0.5, "tone": 0.6},
                3: {"blend": 0.5, "decay": 0.05, "tone": 0.65},
                4: {"blend": 0.5, "decay": 0.4, "tone": 0.6},
                5: {"blend": 0.35, "decay": 0.25, "snap": 0.5},
                6: {"blend": 0.2, "decay": 0.45, "body": 0.6},
                7: {"blend": 0.45, "algoMode": 2, "decay": 0.3, "character": 0.25},
                8: {"blend": 0.5, "decay": 0.3},
            },
            global_overrides={"perc_xvc_kick_to_snare_filter": 0.25, "perc_xvc_global_amount": 0.65}
        ),
        coupling_intensity="Light",
        dna={"brightness": 0.5, "warmth": 0.5, "movement": 0.4, "density": 0.5, "space": 0.15, "aggression": 0.4}
    ),
]


# =============================================================================
# GENRE KITS (15)
# =============================================================================

GENRE_PRESETS = [
    make_preset(
        "Trap Bounce", "Foundation",
        "Modern trap kit. Long 808 sub, sharp snare, rapid hats, distorted clap.",
        ["trap", "808", "sub", "modern", "hard"],
        make_params(
            voice_overrides={
                1: {"blend": 0.05, "decay": 1.2, "tone": 0.3, "snap": 0.6, "body": 0.85, "character": 0.1},
                2: {"blend": 0.2, "decay": 0.2, "tone": 0.7, "snap": 0.75, "character": 0.15},
                3: {"blend": 0.15, "decay": 0.02, "tone": 0.8, "snap": 0.6},
                4: {"blend": 0.15, "decay": 0.25, "tone": 0.75},
                5: {"blend": 0.1, "decay": 0.15, "snap": 0.7, "character": 0.1},
                6: {"blend": 0.05, "decay": 0.5, "body": 0.7, "pitch": -3},
                7: {"blend": 0.3, "decay": 0.15, "snap": 0.5},
                8: {"blend": 0.4, "decay": 0.2},
            },
            global_overrides={"perc_drive": 0.2, "perc_char_grit": 0.15, "perc_char_warmth": 0.5}
        ),
        dna={"brightness": 0.5, "warmth": 0.6, "movement": 0.2, "density": 0.6, "space": 0.05, "aggression": 0.65}
    ),

    make_preset(
        "Boom Bap OG", "Foundation",
        "Classic boom bap. Punchy kick, cracking snare, tight hats. Old school NYC.",
        ["boom-bap", "hip-hop", "classic", "punchy", "NYC"],
        make_params(
            voice_overrides={
                1: {"blend": 0.0, "decay": 0.4, "tone": 0.5, "snap": 0.55, "body": 0.65, "character": 0.05},
                2: {"blend": 0.05, "decay": 0.25, "tone": 0.6, "snap": 0.6, "character": 0.1},
                3: {"blend": 0.0, "decay": 0.04, "tone": 0.65},
                4: {"blend": 0.0, "decay": 0.35, "tone": 0.6},
                5: {"blend": 0.05, "decay": 0.2, "snap": 0.55},
                6: {"blend": 0.0, "decay": 0.4, "body": 0.6},
                7: {"blend": 0.1, "decay": 0.2},
                8: {"blend": 0.1, "decay": 0.25},
            },
            global_overrides={"perc_char_warmth": 0.6, "perc_drive": 0.1}
        ),
        dna={"brightness": 0.45, "warmth": 0.65, "movement": 0.15, "density": 0.55, "space": 0.1, "aggression": 0.5}
    ),

    make_preset(
        "Garage Two Step", "Foundation",
        "UK garage shuffle. Tight kick, pitched snare, swung hats, rolling percussion.",
        ["garage", "UK", "shuffle", "2step", "rolling"],
        make_params(
            voice_overrides={
                1: {"blend": 0.1, "decay": 0.35, "tone": 0.5, "snap": 0.5, "body": 0.55},
                2: {"blend": 0.15, "decay": 0.2, "tone": 0.65, "snap": 0.55, "pitch": 3},
                3: {"blend": 0.1, "decay": 0.03, "tone": 0.7},
                4: {"blend": 0.1, "decay": 0.3, "tone": 0.65},
                5: {"blend": 0.1, "decay": 0.15, "snap": 0.5},
                6: {"blend": 0.1, "decay": 0.3, "body": 0.5, "pitch": 2},
                7: {"blend": 0.2, "decay": 0.2, "snap": 0.4},
                8: {"blend": 0.15, "decay": 0.25},
            },
            global_overrides={"perc_masterTone": 0.55, "perc_char_warmth": 0.55}
        ),
        dna={"brightness": 0.55, "warmth": 0.5, "movement": 0.35, "density": 0.5, "space": 0.15, "aggression": 0.35}
    ),

    make_preset(
        "House Classic", "Foundation",
        "Classic house drums. Four-on-the-floor punch, open hats, clap on two and four.",
        ["house", "classic", "four-on-floor", "dance", "club"],
        make_params(
            voice_overrides={
                1: {"blend": 0.05, "decay": 0.4, "tone": 0.5, "snap": 0.55, "body": 0.6, "character": 0.05},
                2: {"blend": 0.1, "decay": 0.25, "tone": 0.6, "snap": 0.5},
                3: {"blend": 0.05, "decay": 0.04, "tone": 0.7},
                4: {"blend": 0.05, "decay": 0.45, "tone": 0.65},
                5: {"blend": 0.1, "decay": 0.2, "snap": 0.55},
                6: {"blend": 0.05, "decay": 0.35, "body": 0.55},
                7: {"blend": 0.15, "decay": 0.2},
                8: {"blend": 0.2, "decay": 0.3},
            },
            global_overrides={"perc_drive": 0.1, "perc_char_warmth": 0.55}
        ),
        dna={"brightness": 0.5, "warmth": 0.55, "movement": 0.2, "density": 0.55, "space": 0.1, "aggression": 0.45}
    ),

    make_preset(
        "Techno Industrial", "Flux",
        "Industrial techno. Distorted kick, metallic noise snare, harsh hats, grinding textures.",
        ["techno", "industrial", "distorted", "harsh", "metallic"],
        make_params(
            voice_overrides={
                1: {"blend": 0.1, "decay": 0.3, "tone": 0.4, "snap": 0.7, "body": 0.5, "character": 0.3},
                2: {"blend": 0.3, "decay": 0.2, "tone": 0.5, "snap": 0.65, "character": 0.35},
                3: {"blend": 0.4, "decay": 0.03, "tone": 0.6, "character": 0.25},
                4: {"blend": 0.4, "decay": 0.2, "tone": 0.55, "character": 0.3},
                5: {"blend": 0.25, "decay": 0.15, "snap": 0.6, "character": 0.3},
                6: {"blend": 0.15, "decay": 0.35, "body": 0.5, "character": 0.25},
                7: {"blend": 0.35, "decay": 0.2, "character": 0.35},
                8: {"blend": 0.4, "decay": 0.25, "character": 0.3},
            },
            global_overrides={"perc_drive": 0.35, "perc_char_grit": 0.4, "perc_masterTone": 0.45}
        ),
        dna={"brightness": 0.45, "warmth": 0.3, "movement": 0.3, "density": 0.65, "space": 0.1, "aggression": 0.8}
    ),

    make_preset(
        "Jungle Breakbeat", "Flux",
        "Jungle/DnB breaks. Fast decay, chopped feel, pitched toms, rolling percussion.",
        ["jungle", "dnb", "breakbeat", "fast", "rolling"],
        make_params(
            voice_overrides={
                1: {"blend": 0.1, "decay": 0.25, "tone": 0.55, "snap": 0.6, "body": 0.5},
                2: {"blend": 0.15, "decay": 0.15, "tone": 0.65, "snap": 0.7},
                3: {"blend": 0.1, "decay": 0.02, "tone": 0.75},
                4: {"blend": 0.1, "decay": 0.2, "tone": 0.7},
                5: {"blend": 0.15, "decay": 0.12, "snap": 0.6},
                6: {"blend": 0.1, "decay": 0.25, "body": 0.5, "pitch": 5},
                7: {"blend": 0.2, "decay": 0.15, "snap": 0.5},
                8: {"blend": 0.25, "decay": 0.4},
            },
            global_overrides={"perc_drive": 0.15, "perc_masterTone": 0.6, "perc_char_warmth": 0.45}
        ),
        dna={"brightness": 0.6, "warmth": 0.4, "movement": 0.5, "density": 0.7, "space": 0.1, "aggression": 0.6}
    ),

    make_preset(
        "Afrobeat Pulse", "Foundation",
        "Afrobeat drums. Warm low end, talking drum toms, shaker percussion, open feel.",
        ["afrobeat", "warm", "organic", "talking-drum", "percussion"],
        make_params(
            voice_overrides={
                1: {"blend": 0.05, "decay": 0.45, "tone": 0.45, "snap": 0.4, "body": 0.65},
                2: {"blend": 0.1, "decay": 0.3, "tone": 0.55, "snap": 0.45},
                3: {"blend": 0.15, "decay": 0.04, "tone": 0.6},
                4: {"blend": 0.15, "decay": 0.35, "tone": 0.55},
                5: {"blend": 0.1, "decay": 0.25, "snap": 0.4},
                6: {"blend": 0.2, "algoMode": 1, "decay": 0.5, "body": 0.6, "character": 0.3, "pitch": 4},
                7: {"blend": 0.3, "decay": 0.15, "snap": 0.35, "character": 0.2},
                8: {"blend": 0.25, "decay": 0.2, "character": 0.15},
            },
            global_overrides={"perc_char_warmth": 0.65}
        ),
        dna={"brightness": 0.45, "warmth": 0.65, "movement": 0.4, "density": 0.55, "space": 0.15, "aggression": 0.3}
    ),

    make_preset(
        "Reggaeton Dem", "Foundation",
        "Dembow rhythm kit. Snappy kick, cracking snare, tight percussion for reggaeton groove.",
        ["reggaeton", "dembow", "latin", "snappy", "groove"],
        make_params(
            voice_overrides={
                1: {"blend": 0.05, "decay": 0.35, "tone": 0.5, "snap": 0.6, "body": 0.6},
                2: {"blend": 0.1, "decay": 0.2, "tone": 0.65, "snap": 0.65},
                3: {"blend": 0.05, "decay": 0.03, "tone": 0.7},
                4: {"blend": 0.05, "decay": 0.3, "tone": 0.65},
                5: {"blend": 0.1, "decay": 0.18, "snap": 0.6},
                6: {"blend": 0.1, "decay": 0.3, "body": 0.55, "pitch": 3},
                7: {"blend": 0.15, "decay": 0.15, "snap": 0.5},
                8: {"blend": 0.2, "decay": 0.2},
            },
            global_overrides={"perc_drive": 0.1, "perc_char_warmth": 0.5}
        ),
        dna={"brightness": 0.55, "warmth": 0.5, "movement": 0.3, "density": 0.55, "space": 0.05, "aggression": 0.5}
    ),

    make_preset(
        "Lo-Fi Study", "Atmosphere",
        "Mellow lo-fi drums. Soft hits, warm saturation, vinyl crackle character. Study beats.",
        ["lofi", "chill", "mellow", "vinyl", "study"],
        make_params(
            voice_overrides={
                1: {"blend": 0.05, "decay": 0.4, "tone": 0.4, "snap": 0.3, "body": 0.6},
                2: {"blend": 0.1, "decay": 0.25, "tone": 0.5, "snap": 0.35},
                3: {"blend": 0.05, "decay": 0.04, "tone": 0.55},
                4: {"blend": 0.05, "decay": 0.3, "tone": 0.5},
                5: {"blend": 0.1, "decay": 0.2, "snap": 0.35},
                6: {"blend": 0.05, "decay": 0.35, "body": 0.55},
                7: {"blend": 0.1, "decay": 0.2},
                8: {"blend": 0.1, "decay": 0.25},
            },
            global_overrides={
                "perc_char_grit": 0.15, "perc_char_warmth": 0.7,
                "perc_fx_lofi_bits": 12.0, "perc_fx_lofi_mix": 0.3,
                "perc_masterTone": 0.4, "perc_level": 0.7,
            }
        ),
        dna={"brightness": 0.3, "warmth": 0.75, "movement": 0.1, "density": 0.4, "space": 0.2, "aggression": 0.15}
    ),

    make_preset(
        "Synthwave Drums", "Prism",
        "80s synthwave kit. Gated reverb snare, punchy kick, electronic toms, bright cymbals.",
        ["synthwave", "80s", "gated", "retro", "neon"],
        make_params(
            voice_overrides={
                1: {"blend": 0.1, "decay": 0.35, "tone": 0.5, "snap": 0.55, "body": 0.55},
                2: {"blend": 0.15, "decay": 0.3, "tone": 0.6, "snap": 0.6, "character": 0.1},
                3: {"blend": 0.2, "decay": 0.03, "tone": 0.75},
                4: {"blend": 0.2, "decay": 0.35, "tone": 0.7},
                5: {"blend": 0.15, "decay": 0.2, "snap": 0.55},
                6: {"blend": 0.2, "decay": 0.4, "body": 0.5, "pitch": 2},
                7: {"blend": 0.25, "decay": 0.25, "character": 0.15},
                8: {"blend": 0.3, "decay": 0.4, "character": 0.1},
            },
            global_overrides={
                "perc_fx_reverb_mix": 0.35, "perc_fx_reverb_size": 0.5, "perc_fx_reverb_decay": 0.4,
                "perc_masterTone": 0.6,
            }
        ),
        dna={"brightness": 0.65, "warmth": 0.4, "movement": 0.2, "density": 0.5, "space": 0.55, "aggression": 0.4}
    ),

    make_preset(
        "Deep House", "Foundation",
        "Deep house drums. Warm sub kick, muted hats, soft clap, minimal and deep.",
        ["deep-house", "minimal", "warm", "sub", "smooth"],
        make_params(
            voice_overrides={
                1: {"blend": 0.0, "decay": 0.5, "tone": 0.4, "snap": 0.45, "body": 0.7},
                2: {"blend": 0.05, "decay": 0.2, "tone": 0.5, "snap": 0.4},
                3: {"blend": 0.0, "decay": 0.03, "tone": 0.55},
                4: {"blend": 0.0, "decay": 0.35, "tone": 0.5},
                5: {"blend": 0.05, "decay": 0.18, "snap": 0.4},
                6: {"blend": 0.0, "decay": 0.35, "body": 0.6},
                7: {"blend": 0.1, "decay": 0.2},
                8: {"blend": 0.1, "decay": 0.3},
            },
            global_overrides={"perc_char_warmth": 0.65, "perc_masterTone": 0.45}
        ),
        dna={"brightness": 0.35, "warmth": 0.7, "movement": 0.15, "density": 0.4, "space": 0.15, "aggression": 0.25}
    ),

    make_preset(
        "Minimal Tech", "Prism",
        "Minimal techno. Tight, clicky kick, rimshot snare, crisp hats, sparse and precise.",
        ["minimal", "techno", "click", "precise", "tight"],
        make_params(
            voice_overrides={
                1: {"blend": 0.15, "decay": 0.25, "tone": 0.55, "snap": 0.7, "body": 0.4},
                2: {"blend": 0.2, "decay": 0.15, "tone": 0.7, "snap": 0.65},
                3: {"blend": 0.15, "decay": 0.02, "tone": 0.8},
                4: {"blend": 0.15, "decay": 0.2, "tone": 0.75},
                5: {"blend": 0.2, "decay": 0.12, "snap": 0.6},
                6: {"blend": 0.15, "decay": 0.25, "body": 0.4},
                7: {"blend": 0.2, "decay": 0.15, "snap": 0.55},
                8: {"blend": 0.25, "decay": 0.2},
            },
            global_overrides={"perc_masterTone": 0.65}
        ),
        dna={"brightness": 0.7, "warmth": 0.3, "movement": 0.15, "density": 0.4, "space": 0.1, "aggression": 0.45}
    ),

    make_preset(
        "Drill Pressure", "Foundation",
        "UK drill kit. Sliding 808 sub, sharp snares, rapid hi-hat rolls, dark energy.",
        ["drill", "UK", "dark", "808", "sliding"],
        make_params(
            voice_overrides={
                1: {"blend": 0.05, "decay": 1.0, "tone": 0.35, "snap": 0.55, "body": 0.8, "character": 0.1},
                2: {"blend": 0.15, "decay": 0.18, "tone": 0.65, "snap": 0.7, "character": 0.1},
                3: {"blend": 0.1, "decay": 0.02, "tone": 0.75, "snap": 0.55},
                4: {"blend": 0.1, "decay": 0.2, "tone": 0.7},
                5: {"blend": 0.1, "decay": 0.15, "snap": 0.65},
                6: {"blend": 0.05, "decay": 0.45, "body": 0.65, "pitch": -2},
                7: {"blend": 0.2, "decay": 0.15, "snap": 0.5},
                8: {"blend": 0.25, "decay": 0.2},
            },
            global_overrides={"perc_drive": 0.15, "perc_char_grit": 0.1, "perc_masterTone": 0.45}
        ),
        dna={"brightness": 0.4, "warmth": 0.55, "movement": 0.25, "density": 0.6, "space": 0.05, "aggression": 0.7}
    ),

    make_preset(
        "Dancehall Fire", "Foundation",
        "Dancehall drums. Tight punchy kit with pitched percussion and aggressive snap.",
        ["dancehall", "caribbean", "punchy", "pitched", "fire"],
        make_params(
            voice_overrides={
                1: {"blend": 0.05, "decay": 0.35, "tone": 0.5, "snap": 0.6, "body": 0.6},
                2: {"blend": 0.1, "decay": 0.22, "tone": 0.6, "snap": 0.65, "pitch": 2},
                3: {"blend": 0.1, "decay": 0.03, "tone": 0.7},
                4: {"blend": 0.1, "decay": 0.3, "tone": 0.65},
                5: {"blend": 0.1, "decay": 0.18, "snap": 0.6},
                6: {"blend": 0.15, "decay": 0.3, "body": 0.55, "pitch": 5},
                7: {"blend": 0.2, "decay": 0.18, "snap": 0.55, "pitch": 3},
                8: {"blend": 0.2, "decay": 0.2},
            },
            global_overrides={"perc_drive": 0.15, "perc_char_warmth": 0.5}
        ),
        dna={"brightness": 0.55, "warmth": 0.5, "movement": 0.35, "density": 0.55, "space": 0.05, "aggression": 0.55}
    ),

    make_preset(
        "Footwork Juke", "Flux",
        "Chicago footwork. Ultra-fast hats, stuttered kicks, pitched snare rolls, frenetic energy.",
        ["footwork", "juke", "chicago", "fast", "stuttered"],
        make_params(
            voice_overrides={
                1: {"blend": 0.1, "decay": 0.2, "tone": 0.5, "snap": 0.65, "body": 0.45},
                2: {"blend": 0.15, "decay": 0.12, "tone": 0.65, "snap": 0.7, "pitch": 4},
                3: {"blend": 0.1, "decay": 0.015, "tone": 0.8, "snap": 0.55},
                4: {"blend": 0.1, "decay": 0.15, "tone": 0.75},
                5: {"blend": 0.15, "decay": 0.1, "snap": 0.65},
                6: {"blend": 0.1, "decay": 0.2, "body": 0.45, "pitch": 6},
                7: {"blend": 0.2, "decay": 0.12, "snap": 0.5},
                8: {"blend": 0.25, "decay": 0.15},
            },
            global_overrides={"perc_drive": 0.1, "perc_masterTone": 0.6}
        ),
        dna={"brightness": 0.6, "warmth": 0.35, "movement": 0.65, "density": 0.7, "space": 0.05, "aggression": 0.6}
    ),
]

# =============================================================================
# HYBRID & MORPHING KITS (10)
# =============================================================================

HYBRID_PRESETS = [
    make_preset(
        "Crossfade Journey", "Flux",
        "Designed for MACHINE macro sweep. Each voice at different blend points across the X-O spectrum.",
        ["morph", "crossfade", "sweep", "spectrum", "performance"],
        make_params(
            voice_overrides={
                1: {"blend": 0.0, "decay": 0.55, "snap": 0.45, "body": 0.6},
                2: {"blend": 0.15, "decay": 0.3, "snap": 0.5},
                3: {"blend": 0.3, "decay": 0.05, "tone": 0.6},
                4: {"blend": 0.45, "decay": 0.4, "tone": 0.6},
                5: {"blend": 0.6, "decay": 0.25, "snap": 0.5, "character": 0.2},
                6: {"blend": 0.7, "decay": 0.45, "body": 0.55, "character": 0.25},
                7: {"blend": 0.85, "decay": 0.3, "character": 0.35},
                8: {"blend": 1.0, "decay": 0.35, "character": 0.4},
            },
            global_overrides={"perc_macro_machine": 0.5}
        ),
        dna={"brightness": 0.55, "warmth": 0.5, "movement": 0.7, "density": 0.55, "space": 0.15, "aggression": 0.4}
    ),

    make_preset(
        "Circuit Breaker", "Flux",
        "Unstable hybrid. High character on all voices creates erratic, breaking-point sounds.",
        ["unstable", "breaking", "erratic", "high-character", "edge"],
        make_params(
            voice_overrides={
                1: {"blend": 0.4, "decay": 0.5, "snap": 0.5, "body": 0.55, "character": 0.6},
                2: {"blend": 0.5, "decay": 0.25, "snap": 0.55, "character": 0.65},
                3: {"blend": 0.55, "decay": 0.05, "tone": 0.65, "character": 0.55},
                4: {"blend": 0.55, "decay": 0.35, "character": 0.6},
                5: {"blend": 0.45, "decay": 0.2, "snap": 0.55, "character": 0.6},
                6: {"blend": 0.4, "decay": 0.4, "body": 0.55, "character": 0.55},
                7: {"blend": 0.5, "decay": 0.25, "character": 0.7},
                8: {"blend": 0.6, "decay": 0.3, "character": 0.65},
            },
            global_overrides={"perc_char_grit": 0.25, "perc_drive": 0.2}
        ),
        dna={"brightness": 0.55, "warmth": 0.35, "movement": 0.5, "density": 0.6, "space": 0.1, "aggression": 0.7}
    ),

    make_preset(
        "Algorithm City", "Prism",
        "Pure digital. All voices at full blend — FM, Modal, KS, PhaseDist only. Zero circuit.",
        ["digital", "algorithm", "pure", "FM", "synthesis"],
        make_params(
            voice_overrides={
                1: {"blend": 1.0, "algoMode": 0, "decay": 0.5, "snap": 0.5, "body": 0.5, "character": 0.4},
                2: {"blend": 1.0, "algoMode": 0, "decay": 0.25, "snap": 0.55, "character": 0.45},
                3: {"blend": 1.0, "algoMode": 0, "decay": 0.05, "tone": 0.7, "character": 0.4},
                4: {"blend": 1.0, "algoMode": 0, "decay": 0.3, "tone": 0.65, "character": 0.4},
                5: {"blend": 1.0, "algoMode": 3, "decay": 0.2, "snap": 0.55, "character": 0.5},
                6: {"blend": 1.0, "algoMode": 1, "decay": 0.45, "body": 0.5, "character": 0.35},
                7: {"blend": 1.0, "algoMode": 2, "decay": 0.3, "character": 0.5},
                8: {"blend": 1.0, "algoMode": 1, "decay": 0.35, "character": 0.45},
            },
            global_overrides={"perc_masterTone": 0.55}
        ),
        dna={"brightness": 0.7, "warmth": 0.2, "movement": 0.2, "density": 0.6, "space": 0.1, "aggression": 0.5}
    ),

    make_preset(
        "Twin Engine", "Entangled",
        "Every voice at exact 50/50 blend. Both synthesis engines equally weighted, maximally entangled.",
        ["twin", "balanced", "dual", "entangled", "equal"],
        make_params(
            voice_overrides={
                1: {"blend": 0.5, "decay": 0.5, "snap": 0.5, "body": 0.5},
                2: {"blend": 0.5, "decay": 0.3, "snap": 0.5},
                3: {"blend": 0.5, "decay": 0.05, "tone": 0.6},
                4: {"blend": 0.5, "decay": 0.35},
                5: {"blend": 0.5, "decay": 0.2, "snap": 0.5},
                6: {"blend": 0.5, "decay": 0.4, "body": 0.5},
                7: {"blend": 0.5, "decay": 0.25},
                8: {"blend": 0.5, "decay": 0.3},
            },
            global_overrides={"perc_xvc_global_amount": 0.6}
        ),
        coupling_intensity="Light",
        dna={"brightness": 0.5, "warmth": 0.5, "movement": 0.3, "density": 0.55, "space": 0.15, "aggression": 0.45}
    ),

    make_preset(
        "Half Machine", "Flux",
        "Kick/snare pure circuit, hats/perc pure algorithm. The kit is split between two worlds.",
        ["split", "half", "contrast", "circuit-vs-algo", "divided"],
        make_params(
            voice_overrides={
                1: {"blend": 0.0, "decay": 0.5, "snap": 0.5, "body": 0.6},
                2: {"blend": 0.0, "decay": 0.3, "snap": 0.55},
                3: {"blend": 1.0, "algoMode": 0, "decay": 0.04, "tone": 0.7, "character": 0.4},
                4: {"blend": 1.0, "algoMode": 0, "decay": 0.3, "tone": 0.65, "character": 0.4},
                5: {"blend": 0.0, "decay": 0.2, "snap": 0.5},
                6: {"blend": 0.0, "decay": 0.4, "body": 0.6},
                7: {"blend": 1.0, "algoMode": 2, "decay": 0.25, "character": 0.45},
                8: {"blend": 1.0, "algoMode": 1, "decay": 0.35, "character": 0.5},
            }
        ),
        dna={"brightness": 0.55, "warmth": 0.45, "movement": 0.25, "density": 0.55, "space": 0.1, "aggression": 0.45}
    ),

    make_preset(
        "Blend Walker", "Flux",
        "MUTATE drifts the blend axis. Each hit wanders between circuit and algorithm unpredictably.",
        ["drift", "wandering", "mutate", "blend", "unpredictable"],
        make_params(
            voice_overrides={
                1: {"blend": 0.5, "decay": 0.5, "snap": 0.45, "body": 0.55, "character": 0.2},
                2: {"blend": 0.5, "decay": 0.28, "snap": 0.5, "character": 0.2},
                3: {"blend": 0.5, "decay": 0.05, "tone": 0.6, "character": 0.15},
                4: {"blend": 0.5, "decay": 0.35, "character": 0.15},
                5: {"blend": 0.5, "decay": 0.22, "snap": 0.5, "character": 0.2},
                6: {"blend": 0.5, "decay": 0.4, "body": 0.55, "character": 0.2},
                7: {"blend": 0.5, "decay": 0.25, "character": 0.25},
                8: {"blend": 0.5, "decay": 0.3, "character": 0.2},
            },
            global_overrides={"perc_macro_mutate": 0.6}
        ),
        dna={"brightness": 0.5, "warmth": 0.5, "movement": 0.75, "density": 0.5, "space": 0.15, "aggression": 0.4}
    ),

    make_preset(
        "Spectrum Kit", "Prism",
        "Each algo mode represented. FM kick, Modal snare, KS hats, PhaseDist perc. Full algorithm spectrum.",
        ["spectrum", "diverse", "all-modes", "varied", "colorful"],
        make_params(
            voice_overrides={
                1: {"blend": 0.8, "algoMode": 0, "decay": 0.5, "snap": 0.5, "body": 0.55, "character": 0.35},
                2: {"blend": 0.8, "algoMode": 1, "decay": 0.3, "snap": 0.5, "character": 0.4},
                3: {"blend": 0.85, "algoMode": 0, "decay": 0.04, "tone": 0.7, "character": 0.35},
                4: {"blend": 0.85, "algoMode": 2, "decay": 0.35, "tone": 0.65, "character": 0.4},
                5: {"blend": 0.8, "algoMode": 3, "decay": 0.2, "snap": 0.55, "character": 0.45},
                6: {"blend": 0.75, "algoMode": 1, "decay": 0.45, "body": 0.55, "character": 0.3},
                7: {"blend": 0.85, "algoMode": 2, "decay": 0.25, "character": 0.4},
                8: {"blend": 0.9, "algoMode": 3, "decay": 0.3, "character": 0.5},
            }
        ),
        dna={"brightness": 0.65, "warmth": 0.35, "movement": 0.2, "density": 0.6, "space": 0.1, "aggression": 0.45}
    ),

    make_preset(
        "Parallel Universe", "Entangled",
        "Two kits in one. Odd voices circuit, even voices algorithm. Play them together or apart.",
        ["parallel", "dual-kit", "odd-even", "split", "layered"],
        make_params(
            voice_overrides={
                1: {"blend": 0.0, "decay": 0.5, "snap": 0.5, "body": 0.6},
                2: {"blend": 1.0, "algoMode": 0, "decay": 0.3, "snap": 0.55, "character": 0.4},
                3: {"blend": 0.0, "decay": 0.04, "tone": 0.65},
                4: {"blend": 1.0, "algoMode": 1, "decay": 0.35, "tone": 0.65, "character": 0.4},
                5: {"blend": 0.0, "decay": 0.2, "snap": 0.5},
                6: {"blend": 1.0, "algoMode": 1, "decay": 0.45, "body": 0.55, "character": 0.35},
                7: {"blend": 0.0, "decay": 0.25},
                8: {"blend": 1.0, "algoMode": 2, "decay": 0.35, "character": 0.45},
            },
            global_overrides={"perc_xvc_global_amount": 0.5}
        ),
        coupling_intensity="Light",
        dna={"brightness": 0.55, "warmth": 0.45, "movement": 0.3, "density": 0.6, "space": 0.1, "aggression": 0.45}
    ),

    make_preset(
        "Digital Melt", "Flux",
        "High MUTATE with heavy character. Sounds decompose and reform with each trigger.",
        ["melt", "decompose", "mutate", "unstable", "generative"],
        make_params(
            voice_overrides={
                1: {"blend": 0.6, "decay": 0.5, "snap": 0.45, "body": 0.5, "character": 0.45},
                2: {"blend": 0.65, "decay": 0.25, "snap": 0.5, "character": 0.5},
                3: {"blend": 0.7, "decay": 0.05, "tone": 0.6, "character": 0.4},
                4: {"blend": 0.7, "decay": 0.35, "character": 0.45},
                5: {"blend": 0.6, "decay": 0.2, "snap": 0.5, "character": 0.45},
                6: {"blend": 0.55, "decay": 0.4, "body": 0.5, "character": 0.4},
                7: {"blend": 0.65, "decay": 0.25, "character": 0.5},
                8: {"blend": 0.75, "decay": 0.3, "character": 0.55},
            },
            global_overrides={"perc_macro_mutate": 0.7, "perc_char_grit": 0.2}
        ),
        dna={"brightness": 0.5, "warmth": 0.4, "movement": 0.8, "density": 0.55, "space": 0.15, "aggression": 0.55}
    ),

    make_preset(
        "Phase Shift", "Prism",
        "Phase distortion on every voice. CZ-series inspired with shifting harmonic spectra.",
        ["phase-distortion", "CZ", "shifting", "harmonic", "digital"],
        make_params(
            voice_overrides={
                1: {"blend": 0.75, "algoMode": 3, "decay": 0.45, "snap": 0.5, "body": 0.55, "character": 0.4},
                2: {"blend": 0.8, "algoMode": 3, "decay": 0.25, "snap": 0.5, "character": 0.45},
                3: {"blend": 0.8, "algoMode": 3, "decay": 0.04, "tone": 0.7, "character": 0.4},
                4: {"blend": 0.8, "algoMode": 3, "decay": 0.3, "tone": 0.65, "character": 0.4},
                5: {"blend": 0.75, "algoMode": 3, "decay": 0.2, "snap": 0.55, "character": 0.45},
                6: {"blend": 0.7, "algoMode": 3, "decay": 0.4, "body": 0.5, "character": 0.35},
                7: {"blend": 0.8, "algoMode": 3, "decay": 0.25, "character": 0.5},
                8: {"blend": 0.85, "algoMode": 3, "decay": 0.3, "character": 0.45},
            },
            global_overrides={"perc_masterTone": 0.6}
        ),
        dna={"brightness": 0.65, "warmth": 0.3, "movement": 0.25, "density": 0.55, "space": 0.1, "aggression": 0.45}
    ),
]

# =============================================================================
# COUPLING KITS (8) — Designed for cross-engine modulation
# =============================================================================

COUPLING_PRESETS = [
    make_preset(
        "Onset Morph Feed", "Entangled",
        "ONSET x MORPH coupling. Wavetable morphing modulates drum blend and character axes.",
        ["coupling", "morph", "wavetable", "teal", "cross-engine"],
        make_params(
            voice_overrides={
                1: {"blend": 0.4, "decay": 0.55, "snap": 0.45, "body": 0.6, "character": 0.2},
                2: {"blend": 0.5, "decay": 0.3, "snap": 0.5, "character": 0.25},
                3: {"blend": 0.55, "decay": 0.05, "tone": 0.65, "character": 0.2},
                4: {"blend": 0.55, "decay": 0.35, "character": 0.2},
                5: {"blend": 0.45, "decay": 0.22, "snap": 0.5, "character": 0.2},
                6: {"blend": 0.35, "decay": 0.4, "body": 0.55, "character": 0.15},
                7: {"blend": 0.5, "decay": 0.25, "character": 0.25},
                8: {"blend": 0.6, "decay": 0.3, "character": 0.3},
            },
            global_overrides={"perc_xvc_global_amount": 0.6}
        ),
        coupling_intensity="Medium",
        dna={"brightness": 0.5, "warmth": 0.5, "movement": 0.55, "density": 0.5, "space": 0.15, "aggression": 0.4}
    ),

    make_preset(
        "Dub Echo Drums", "Entangled",
        "ONSET x DUB coupling. Overdub's tape delay and spring reverb process drum hits in send/return.",
        ["coupling", "dub", "echo", "tape", "send-return"],
        make_params(
            voice_overrides={
                1: {"blend": 0.15, "decay": 0.6, "snap": 0.45, "body": 0.65, "character": 0.1},
                2: {"blend": 0.2, "decay": 0.3, "snap": 0.5, "tone": 0.55},
                3: {"blend": 0.15, "decay": 0.04, "tone": 0.6},
                4: {"blend": 0.15, "decay": 0.4, "tone": 0.55},
                5: {"blend": 0.15, "decay": 0.25, "snap": 0.5},
                6: {"blend": 0.1, "decay": 0.45, "body": 0.6},
                7: {"blend": 0.2, "decay": 0.25},
                8: {"blend": 0.2, "decay": 0.3},
            },
            global_overrides={
                "perc_char_warmth": 0.65,
                "perc_fx_delay_time": 0.35, "perc_fx_delay_feedback": 0.5, "perc_fx_delay_mix": 0.25,
                "perc_fx_reverb_mix": 0.15,
            }
        ),
        coupling_intensity="Medium",
        dna={"brightness": 0.35, "warmth": 0.65, "movement": 0.5, "density": 0.5, "space": 0.6, "aggression": 0.3}
    ),

    make_preset(
        "Alien Impact", "Entangled",
        "ONSET x DRIFT coupling. Drift's alien textures modulate drum character. Familiar hits, alien tails.",
        ["coupling", "drift", "alien", "psychedelic", "impact"],
        make_params(
            voice_overrides={
                1: {"blend": 0.3, "decay": 0.6, "snap": 0.5, "body": 0.55, "character": 0.3},
                2: {"blend": 0.45, "decay": 0.35, "snap": 0.5, "character": 0.35},
                3: {"blend": 0.5, "decay": 0.06, "tone": 0.65, "character": 0.25},
                4: {"blend": 0.5, "decay": 0.4, "character": 0.3},
                5: {"blend": 0.4, "decay": 0.25, "snap": 0.5, "character": 0.3},
                6: {"blend": 0.35, "decay": 0.45, "body": 0.55, "character": 0.25},
                7: {"blend": 0.5, "decay": 0.3, "character": 0.35},
                8: {"blend": 0.6, "decay": 0.4, "character": 0.4},
            },
            global_overrides={
                "perc_fx_reverb_mix": 0.25, "perc_fx_reverb_size": 0.5, "perc_fx_reverb_decay": 0.45,
                "perc_macro_mutate": 0.3,
            }
        ),
        coupling_intensity="High",
        dna={"brightness": 0.5, "warmth": 0.4, "movement": 0.6, "density": 0.5, "space": 0.5, "aggression": 0.45}
    ),

    make_preset(
        "Amber Hits", "Entangled",
        "ONSET x BOB coupling. Bob's warm analog textures color the drum hits. Amber-tinted percussion.",
        ["coupling", "bob", "warm", "amber", "analog"],
        make_params(
            voice_overrides={
                1: {"blend": 0.1, "decay": 0.55, "snap": 0.4, "body": 0.65, "character": 0.1},
                2: {"blend": 0.15, "decay": 0.3, "snap": 0.45, "character": 0.1},
                3: {"blend": 0.1, "decay": 0.04, "tone": 0.6},
                4: {"blend": 0.1, "decay": 0.35, "tone": 0.55},
                5: {"blend": 0.1, "decay": 0.22, "snap": 0.45},
                6: {"blend": 0.1, "decay": 0.45, "body": 0.6},
                7: {"blend": 0.15, "decay": 0.25},
                8: {"blend": 0.15, "decay": 0.3},
            },
            global_overrides={"perc_char_warmth": 0.7, "perc_char_grit": 0.1}
        ),
        coupling_intensity="Light",
        dna={"brightness": 0.4, "warmth": 0.75, "movement": 0.2, "density": 0.5, "space": 0.15, "aggression": 0.25}
    ),

    make_preset(
        "Bass Drum Engine", "Entangled",
        "ONSET x FAT coupling. XObese's bass engine adds sub-harmonic weight to every drum voice.",
        ["coupling", "fat", "bass", "sub", "heavy"],
        make_params(
            voice_overrides={
                1: {"blend": 0.05, "decay": 0.7, "tone": 0.35, "snap": 0.5, "body": 0.8, "character": 0.1},
                2: {"blend": 0.1, "decay": 0.3, "tone": 0.5, "snap": 0.5},
                3: {"blend": 0.1, "decay": 0.04, "tone": 0.6},
                4: {"blend": 0.1, "decay": 0.35, "tone": 0.55},
                5: {"blend": 0.1, "decay": 0.2, "snap": 0.5},
                6: {"blend": 0.05, "decay": 0.5, "body": 0.7, "pitch": -3},
                7: {"blend": 0.15, "decay": 0.25},
                8: {"blend": 0.15, "decay": 0.3},
            },
            global_overrides={"perc_char_warmth": 0.6, "perc_masterTone": 0.4}
        ),
        coupling_intensity="Medium",
        dna={"brightness": 0.3, "warmth": 0.75, "movement": 0.15, "density": 0.6, "space": 0.1, "aggression": 0.4}
    ),

    make_preset(
        "Chip Drums NES", "Entangled",
        "ONSET x OVERWORLD coupling. NES pulse and noise character on drum voices. 8-bit percussion.",
        ["coupling", "overworld", "NES", "8bit", "chip"],
        make_params(
            voice_overrides={
                1: {"blend": 0.4, "decay": 0.35, "snap": 0.55, "body": 0.5, "character": 0.25},
                2: {"blend": 0.5, "decay": 0.2, "snap": 0.6, "tone": 0.65, "character": 0.3},
                3: {"blend": 0.55, "decay": 0.03, "tone": 0.75, "character": 0.25},
                4: {"blend": 0.55, "decay": 0.2, "tone": 0.7, "character": 0.25},
                5: {"blend": 0.45, "decay": 0.15, "snap": 0.55, "character": 0.3},
                6: {"blend": 0.35, "decay": 0.3, "body": 0.5, "character": 0.2},
                7: {"blend": 0.5, "decay": 0.2, "character": 0.3},
                8: {"blend": 0.55, "decay": 0.25, "character": 0.3},
            },
            global_overrides={
                "perc_fx_lofi_bits": 8.0, "perc_fx_lofi_mix": 0.35,
                "perc_masterTone": 0.6,
            }
        ),
        coupling_intensity="Medium",
        dna={"brightness": 0.6, "warmth": 0.3, "movement": 0.35, "density": 0.5, "space": 0.1, "aggression": 0.5}
    ),

    make_preset(
        "Granular Scatter", "Entangled",
        "ONSET x OPAL coupling. Drum transients scattered through granular clouds. Hits dissolve into texture.",
        ["coupling", "opal", "granular", "scatter", "cloud"],
        make_params(
            voice_overrides={
                1: {"blend": 0.35, "decay": 0.5, "snap": 0.6, "body": 0.5, "character": 0.2},
                2: {"blend": 0.5, "decay": 0.3, "snap": 0.65, "character": 0.25},
                3: {"blend": 0.55, "decay": 0.04, "tone": 0.7, "snap": 0.5},
                4: {"blend": 0.55, "decay": 0.35, "tone": 0.65},
                5: {"blend": 0.45, "decay": 0.2, "snap": 0.6, "character": 0.2},
                6: {"blend": 0.3, "decay": 0.4, "body": 0.55},
                7: {"blend": 0.5, "decay": 0.25, "character": 0.3},
                8: {"blend": 0.6, "decay": 0.35, "character": 0.3},
            },
            global_overrides={
                "perc_fx_reverb_mix": 0.2, "perc_fx_reverb_size": 0.45,
                "perc_fx_delay_mix": 0.1, "perc_fx_delay_feedback": 0.4,
            }
        ),
        coupling_intensity="High",
        dna={"brightness": 0.55, "warmth": 0.4, "movement": 0.6, "density": 0.45, "space": 0.55, "aggression": 0.35}
    ),

    make_preset(
        "Snap Locked", "Entangled",
        "ONSET x SNAP coupling. OddfeliX's percussive side locks into drum patterns. Tight interlocking groove.",
        ["coupling", "snap", "locked", "groove", "percussive"],
        make_params(
            voice_overrides={
                1: {"blend": 0.2, "decay": 0.45, "snap": 0.55, "body": 0.6},
                2: {"blend": 0.3, "decay": 0.25, "snap": 0.6, "tone": 0.6},
                3: {"blend": 0.35, "decay": 0.04, "tone": 0.65, "snap": 0.5},
                4: {"blend": 0.35, "decay": 0.3, "tone": 0.6},
                5: {"blend": 0.25, "decay": 0.2, "snap": 0.55},
                6: {"blend": 0.2, "decay": 0.4, "body": 0.55},
                7: {"blend": 0.35, "algoMode": 2, "decay": 0.25, "character": 0.2},
                8: {"blend": 0.4, "decay": 0.3},
            },
            global_overrides={"perc_xvc_kick_to_snare_filter": 0.25, "perc_xvc_global_amount": 0.7}
        ),
        coupling_intensity="Medium",
        dna={"brightness": 0.5, "warmth": 0.5, "movement": 0.4, "density": 0.55, "space": 0.1, "aggression": 0.45}
    ),
]

# =============================================================================
# ATMOSPHERE & AETHER KITS (15)
# =============================================================================

ATMOSPHERE_PRESETS = [
    make_preset(
        "Rain Percussion", "Atmosphere",
        "Soft, scattered hits like rain on surfaces. Gentle reverb tails, muted transients.",
        ["rain", "gentle", "scattered", "muted", "ambient"],
        make_params(
            voice_overrides={
                1: {"blend": 0.3, "decay": 0.3, "tone": 0.4, "snap": 0.25, "body": 0.5},
                2: {"blend": 0.35, "decay": 0.2, "tone": 0.45, "snap": 0.25},
                3: {"blend": 0.4, "decay": 0.03, "tone": 0.5},
                4: {"blend": 0.4, "decay": 0.25, "tone": 0.45},
                5: {"blend": 0.3, "decay": 0.15, "snap": 0.2},
                6: {"blend": 0.25, "decay": 0.35, "body": 0.5},
                7: {"blend": 0.35, "decay": 0.2},
                8: {"blend": 0.4, "decay": 0.25},
            },
            global_overrides={
                "perc_level": 0.6,
                "perc_fx_reverb_mix": 0.45, "perc_fx_reverb_size": 0.6, "perc_fx_reverb_decay": 0.5,
                "perc_masterTone": 0.4,
            }
        ),
        dna={"brightness": 0.35, "warmth": 0.55, "movement": 0.3, "density": 0.35, "space": 0.7, "aggression": 0.1}
    ),

    make_preset(
        "Ghost Hits", "Atmosphere",
        "Barely-there drum hits. Faint transients dissolving into reverb. The memory of percussion.",
        ["ghost", "faint", "ethereal", "dissolving", "memory"],
        make_params(
            voice_overrides={
                1: {"blend": 0.4, "decay": 0.6, "tone": 0.4, "snap": 0.2, "body": 0.4},
                2: {"blend": 0.45, "decay": 0.35, "tone": 0.45, "snap": 0.2},
                3: {"blend": 0.5, "decay": 0.04, "tone": 0.5},
                4: {"blend": 0.5, "decay": 0.4, "tone": 0.45},
                5: {"blend": 0.4, "decay": 0.25, "snap": 0.2},
                6: {"blend": 0.35, "decay": 0.45, "body": 0.45},
                7: {"blend": 0.45, "decay": 0.3},
                8: {"blend": 0.5, "decay": 0.4},
            },
            global_overrides={
                "perc_level": 0.5,
                "perc_fx_reverb_mix": 0.55, "perc_fx_reverb_size": 0.7, "perc_fx_reverb_decay": 0.6,
                "perc_fx_delay_mix": 0.15, "perc_fx_delay_feedback": 0.45,
                "perc_masterTone": 0.35,
            }
        ),
        dna={"brightness": 0.3, "warmth": 0.5, "movement": 0.2, "density": 0.25, "space": 0.8, "aggression": 0.05}
    ),

    make_preset(
        "Space Debris", "Atmosphere",
        "Metallic fragments floating in reverb space. Sparse, distant, orbital percussion.",
        ["space", "metallic", "distant", "orbital", "sparse"],
        make_params(
            voice_overrides={
                1: {"blend": 0.6, "algoMode": 1, "decay": 0.5, "tone": 0.5, "snap": 0.3, "character": 0.3},
                2: {"blend": 0.65, "algoMode": 1, "decay": 0.35, "tone": 0.55, "character": 0.35},
                3: {"blend": 0.7, "algoMode": 1, "decay": 0.06, "tone": 0.6, "character": 0.3},
                4: {"blend": 0.7, "algoMode": 1, "decay": 0.4, "tone": 0.6, "character": 0.35},
                5: {"blend": 0.6, "algoMode": 1, "decay": 0.25, "snap": 0.3, "character": 0.25},
                6: {"blend": 0.55, "algoMode": 1, "decay": 0.5, "body": 0.5, "character": 0.3},
                7: {"blend": 0.7, "algoMode": 1, "decay": 0.3, "character": 0.4},
                8: {"blend": 0.75, "algoMode": 1, "decay": 0.45, "character": 0.4},
            },
            global_overrides={
                "perc_fx_reverb_mix": 0.5, "perc_fx_reverb_size": 0.65, "perc_fx_reverb_decay": 0.55,
                "perc_fx_delay_mix": 0.2, "perc_fx_delay_feedback": 0.4, "perc_fx_delay_time": 0.45,
            }
        ),
        dna={"brightness": 0.55, "warmth": 0.35, "movement": 0.25, "density": 0.35, "space": 0.75, "aggression": 0.2}
    ),

    make_preset(
        "Frozen Drums", "Atmosphere",
        "Icy, crystalline percussion. Bright attacks with sustained frozen reverb tails.",
        ["frozen", "icy", "crystal", "sustained", "bright"],
        make_params(
            voice_overrides={
                1: {"blend": 0.5, "algoMode": 2, "decay": 0.7, "tone": 0.6, "snap": 0.4, "character": 0.3},
                2: {"blend": 0.55, "algoMode": 2, "decay": 0.45, "tone": 0.65, "snap": 0.4, "character": 0.35},
                3: {"blend": 0.6, "decay": 0.05, "tone": 0.75},
                4: {"blend": 0.6, "decay": 0.5, "tone": 0.7},
                5: {"blend": 0.5, "algoMode": 2, "decay": 0.3, "snap": 0.4, "character": 0.25},
                6: {"blend": 0.45, "algoMode": 2, "decay": 0.55, "body": 0.5, "character": 0.3},
                7: {"blend": 0.55, "algoMode": 2, "decay": 0.4, "character": 0.35},
                8: {"blend": 0.6, "algoMode": 2, "decay": 0.5, "character": 0.3},
            },
            global_overrides={
                "perc_fx_reverb_mix": 0.5, "perc_fx_reverb_size": 0.7, "perc_fx_reverb_decay": 0.6,
                "perc_masterTone": 0.65,
            }
        ),
        dna={"brightness": 0.7, "warmth": 0.25, "movement": 0.15, "density": 0.4, "space": 0.75, "aggression": 0.2}
    ),

    make_preset(
        "Cloud Percussion", "Atmosphere",
        "Soft billowing hits. Long reverb, low snap, warm tones. Drums as clouds.",
        ["cloud", "soft", "billowing", "warm", "padlike"],
        make_params(
            voice_overrides={
                1: {"blend": 0.25, "decay": 0.6, "tone": 0.4, "snap": 0.15, "body": 0.6},
                2: {"blend": 0.3, "decay": 0.4, "tone": 0.45, "snap": 0.15},
                3: {"blend": 0.35, "decay": 0.06, "tone": 0.45},
                4: {"blend": 0.35, "decay": 0.45, "tone": 0.4},
                5: {"blend": 0.25, "decay": 0.3, "snap": 0.15},
                6: {"blend": 0.2, "decay": 0.5, "body": 0.6},
                7: {"blend": 0.3, "decay": 0.35},
                8: {"blend": 0.35, "decay": 0.4},
            },
            global_overrides={
                "perc_level": 0.65,
                "perc_char_warmth": 0.7,
                "perc_fx_reverb_mix": 0.5, "perc_fx_reverb_size": 0.6, "perc_fx_reverb_decay": 0.55,
                "perc_masterTone": 0.35,
            }
        ),
        dna={"brightness": 0.3, "warmth": 0.7, "movement": 0.15, "density": 0.35, "space": 0.75, "aggression": 0.05}
    ),

    make_preset(
        "Distant Impacts", "Atmosphere",
        "Far-away explosions and impacts. Heavy reverb, deep sub presence, muffled attack.",
        ["distant", "impact", "deep", "muffled", "cinematic"],
        make_params(
            voice_overrides={
                1: {"blend": 0.15, "decay": 0.9, "tone": 0.3, "snap": 0.3, "body": 0.8},
                2: {"blend": 0.2, "decay": 0.5, "tone": 0.4, "snap": 0.3},
                3: {"blend": 0.25, "decay": 0.06, "tone": 0.4},
                4: {"blend": 0.25, "decay": 0.5, "tone": 0.4},
                5: {"blend": 0.2, "decay": 0.35, "snap": 0.3},
                6: {"blend": 0.15, "decay": 0.6, "body": 0.7},
                7: {"blend": 0.25, "decay": 0.35},
                8: {"blend": 0.3, "decay": 0.5},
            },
            global_overrides={
                "perc_fx_reverb_mix": 0.6, "perc_fx_reverb_size": 0.75, "perc_fx_reverb_decay": 0.65,
                "perc_masterTone": 0.3, "perc_char_warmth": 0.6,
            }
        ),
        dna={"brightness": 0.2, "warmth": 0.65, "movement": 0.15, "density": 0.45, "space": 0.85, "aggression": 0.35}
    ),

    make_preset(
        "Cosmic Dust", "Atmosphere",
        "Tiny metallic particles in vast space. High-frequency modal shimmer with endless decay.",
        ["cosmic", "shimmer", "tiny", "metallic", "vast"],
        make_params(
            voice_overrides={
                1: {"blend": 0.7, "algoMode": 1, "decay": 0.4, "tone": 0.65, "snap": 0.3, "character": 0.45},
                2: {"blend": 0.75, "algoMode": 1, "decay": 0.3, "tone": 0.7, "character": 0.5},
                3: {"blend": 0.8, "algoMode": 1, "decay": 0.05, "tone": 0.8, "character": 0.45},
                4: {"blend": 0.8, "algoMode": 1, "decay": 0.35, "tone": 0.75, "character": 0.5},
                5: {"blend": 0.7, "algoMode": 1, "decay": 0.25, "snap": 0.3, "character": 0.4},
                6: {"blend": 0.65, "algoMode": 1, "decay": 0.45, "body": 0.45, "character": 0.4},
                7: {"blend": 0.75, "algoMode": 1, "decay": 0.3, "character": 0.5},
                8: {"blend": 0.8, "algoMode": 1, "decay": 0.4, "character": 0.55},
            },
            global_overrides={
                "perc_level": 0.6,
                "perc_fx_reverb_mix": 0.45, "perc_fx_reverb_size": 0.6, "perc_fx_reverb_decay": 0.55,
                "perc_fx_delay_mix": 0.15, "perc_fx_delay_time": 0.4, "perc_fx_delay_feedback": 0.35,
            }
        ),
        dna={"brightness": 0.75, "warmth": 0.25, "movement": 0.3, "density": 0.35, "space": 0.7, "aggression": 0.15}
    ),

    # === AETHER presets ===
    make_preset(
        "Singularity", "Aether",
        "Collapsed percussion. Extreme compression of time and space. Hits implode rather than explode.",
        ["singularity", "extreme", "implosion", "collapsed", "experimental"],
        make_params(
            voice_overrides={
                1: {"blend": 0.7, "algoMode": 0, "decay": 0.15, "snap": 0.8, "body": 0.3, "character": 0.6},
                2: {"blend": 0.75, "algoMode": 0, "decay": 0.1, "snap": 0.8, "character": 0.65},
                3: {"blend": 0.8, "decay": 0.01, "tone": 0.5, "character": 0.5},
                4: {"blend": 0.8, "decay": 0.08, "character": 0.55},
                5: {"blend": 0.7, "algoMode": 3, "decay": 0.08, "snap": 0.75, "character": 0.6},
                6: {"blend": 0.65, "decay": 0.12, "body": 0.35, "character": 0.55},
                7: {"blend": 0.75, "decay": 0.08, "character": 0.65},
                8: {"blend": 0.8, "decay": 0.1, "character": 0.7},
            },
            global_overrides={"perc_drive": 0.3, "perc_char_grit": 0.4, "perc_masterTone": 0.5}
        ),
        dna={"brightness": 0.5, "warmth": 0.2, "movement": 0.3, "density": 0.7, "space": 0.05, "aggression": 0.8}
    ),

    make_preset(
        "Quantum Noise", "Aether",
        "Full MUTATE + high character. Every hit is genuinely unpredictable. Quantum probability percussion.",
        ["quantum", "noise", "random", "unpredictable", "probability"],
        make_params(
            voice_overrides={
                1: {"blend": 0.5, "decay": 0.4, "snap": 0.5, "body": 0.5, "character": 0.5},
                2: {"blend": 0.5, "decay": 0.25, "snap": 0.5, "character": 0.55},
                3: {"blend": 0.5, "decay": 0.04, "tone": 0.6, "character": 0.45},
                4: {"blend": 0.5, "decay": 0.3, "character": 0.5},
                5: {"blend": 0.5, "decay": 0.2, "snap": 0.5, "character": 0.5},
                6: {"blend": 0.5, "decay": 0.35, "body": 0.5, "character": 0.45},
                7: {"blend": 0.5, "decay": 0.25, "character": 0.55},
                8: {"blend": 0.5, "decay": 0.3, "character": 0.5},
            },
            global_overrides={"perc_macro_mutate": 1.0, "perc_char_grit": 0.2}
        ),
        dna={"brightness": 0.5, "warmth": 0.4, "movement": 0.95, "density": 0.55, "space": 0.15, "aggression": 0.5}
    ),

    make_preset(
        "Abstract Machine", "Aether",
        "Non-percussion percussion. Sounds that reference drums but exist in an abstract sound space.",
        ["abstract", "non-drum", "conceptual", "strange", "art"],
        make_params(
            voice_overrides={
                1: {"blend": 0.9, "algoMode": 1, "decay": 0.8, "tone": 0.3, "snap": 0.15, "body": 0.7, "character": 0.7},
                2: {"blend": 0.85, "algoMode": 2, "decay": 0.6, "snap": 0.2, "character": 0.65},
                3: {"blend": 0.95, "algoMode": 3, "decay": 0.15, "tone": 0.4, "character": 0.6},
                4: {"blend": 0.95, "algoMode": 0, "decay": 0.5, "character": 0.7},
                5: {"blend": 0.9, "algoMode": 1, "decay": 0.4, "snap": 0.2, "character": 0.6},
                6: {"blend": 0.85, "algoMode": 2, "decay": 0.7, "body": 0.6, "character": 0.55},
                7: {"blend": 0.9, "algoMode": 3, "decay": 0.5, "character": 0.7},
                8: {"blend": 0.95, "algoMode": 0, "decay": 0.55, "character": 0.75},
            },
            global_overrides={"perc_macro_mutate": 0.4, "perc_fx_reverb_mix": 0.3, "perc_fx_reverb_size": 0.5}
        ),
        dna={"brightness": 0.45, "warmth": 0.4, "movement": 0.55, "density": 0.5, "space": 0.45, "aggression": 0.4}
    ),

    make_preset(
        "Void Percussion", "Aether",
        "Drums from the void. Extreme reverb, barely audible transients, infinite decay tails.",
        ["void", "infinite", "decay", "barely-there", "dark"],
        make_params(
            voice_overrides={
                1: {"blend": 0.4, "decay": 1.0, "tone": 0.3, "snap": 0.1, "body": 0.5},
                2: {"blend": 0.45, "decay": 0.7, "tone": 0.35, "snap": 0.1},
                3: {"blend": 0.5, "decay": 0.1, "tone": 0.35},
                4: {"blend": 0.5, "decay": 0.6, "tone": 0.3},
                5: {"blend": 0.4, "decay": 0.5, "snap": 0.1},
                6: {"blend": 0.35, "decay": 0.8, "body": 0.5},
                7: {"blend": 0.45, "decay": 0.6},
                8: {"blend": 0.5, "decay": 0.7},
            },
            global_overrides={
                "perc_level": 0.5,
                "perc_fx_reverb_mix": 0.7, "perc_fx_reverb_size": 0.85, "perc_fx_reverb_decay": 0.75,
                "perc_fx_delay_mix": 0.2, "perc_fx_delay_feedback": 0.5, "perc_fx_delay_time": 0.5,
                "perc_masterTone": 0.25,
            }
        ),
        dna={"brightness": 0.2, "warmth": 0.45, "movement": 0.15, "density": 0.3, "space": 0.95, "aggression": 0.1}
    ),

    make_preset(
        "Neural Beats", "Aether",
        "AI-inspired generative percussion. High MUTATE, XVC feedback, evolving cross-voice relationships.",
        ["neural", "generative", "AI", "evolving", "feedback"],
        make_params(
            voice_overrides={
                1: {"blend": 0.5, "decay": 0.45, "snap": 0.5, "body": 0.55, "character": 0.3},
                2: {"blend": 0.55, "decay": 0.28, "snap": 0.5, "character": 0.35},
                3: {"blend": 0.6, "decay": 0.05, "tone": 0.6, "character": 0.25},
                4: {"blend": 0.6, "decay": 0.35, "character": 0.3},
                5: {"blend": 0.5, "decay": 0.22, "snap": 0.5, "character": 0.3},
                6: {"blend": 0.45, "decay": 0.4, "body": 0.55, "character": 0.25},
                7: {"blend": 0.55, "decay": 0.28, "character": 0.35},
                8: {"blend": 0.6, "decay": 0.3, "character": 0.3},
            },
            global_overrides={
                "perc_macro_mutate": 0.65,
                "perc_xvc_global_amount": 0.8,
                "perc_xvc_kick_to_snare_filter": 0.3,
                "perc_xvc_snare_to_hat_decay": 0.25,
                "perc_xvc_kick_to_tom_pitch": 0.2,
                "perc_xvc_snare_to_perc_blend": 0.15,
            }
        ),
        dna={"brightness": 0.5, "warmth": 0.45, "movement": 0.8, "density": 0.6, "space": 0.2, "aggression": 0.45}
    ),

    make_preset(
        "Anti Matter", "Aether",
        "Inverted drum physics. Low where high should be, fast where slow should be. Everything reversed.",
        ["anti", "inverted", "reversed", "strange", "physics"],
        make_params(
            voice_overrides={
                1: {"blend": 0.9, "decay": 0.15, "tone": 0.8, "snap": 0.1, "body": 0.2, "character": 0.6},
                2: {"blend": 0.1, "decay": 0.6, "tone": 0.3, "snap": 0.2, "character": 0.1},
                3: {"blend": 0.0, "decay": 0.3, "tone": 0.3},
                4: {"blend": 0.0, "decay": 0.05, "tone": 0.3},
                5: {"blend": 0.95, "algoMode": 0, "decay": 0.5, "snap": 0.15, "character": 0.7},
                6: {"blend": 1.0, "algoMode": 1, "decay": 0.1, "body": 0.2, "character": 0.5, "pitch": 10},
                7: {"blend": 0.0, "decay": 0.5, "snap": 0.1},
                8: {"blend": 0.0, "decay": 0.05},
            },
            global_overrides={"perc_masterTone": 0.35, "perc_char_grit": 0.2}
        ),
        dna={"brightness": 0.4, "warmth": 0.35, "movement": 0.3, "density": 0.5, "space": 0.15, "aggression": 0.5}
    ),

    make_preset(
        "Hyperspace", "Aether",
        "Extreme FX processing. Maximum delay feedback, huge reverb, lo-fi crunch. Drums entering hyperspace.",
        ["hyperspace", "extreme", "maxFX", "warp", "crunch"],
        make_params(
            voice_overrides={
                1: {"blend": 0.3, "decay": 0.4, "snap": 0.5, "body": 0.55, "character": 0.2},
                2: {"blend": 0.4, "decay": 0.25, "snap": 0.5, "character": 0.25},
                3: {"blend": 0.45, "decay": 0.04, "tone": 0.6, "character": 0.2},
                4: {"blend": 0.45, "decay": 0.3, "character": 0.2},
                5: {"blend": 0.35, "decay": 0.2, "snap": 0.5, "character": 0.2},
                6: {"blend": 0.25, "decay": 0.4, "body": 0.55},
                7: {"blend": 0.4, "decay": 0.25, "character": 0.25},
                8: {"blend": 0.5, "decay": 0.3, "character": 0.3},
            },
            global_overrides={
                "perc_drive": 0.25, "perc_char_grit": 0.3,
                "perc_fx_delay_time": 0.4, "perc_fx_delay_feedback": 0.7, "perc_fx_delay_mix": 0.4,
                "perc_fx_reverb_mix": 0.5, "perc_fx_reverb_size": 0.8, "perc_fx_reverb_decay": 0.7,
                "perc_fx_lofi_bits": 10.0, "perc_fx_lofi_mix": 0.3,
            }
        ),
        dna={"brightness": 0.4, "warmth": 0.35, "movement": 0.5, "density": 0.55, "space": 0.8, "aggression": 0.6}
    ),

    make_preset(
        "DNA Shift", "Aether",
        "MACHINE and MUTATE both active. The kit's DNA drifts between analog and digital on every hit.",
        ["DNA", "shifting", "macro", "dual-macro", "evolving"],
        make_params(
            voice_overrides={
                1: {"blend": 0.4, "decay": 0.5, "snap": 0.45, "body": 0.55, "character": 0.25},
                2: {"blend": 0.45, "decay": 0.28, "snap": 0.5, "character": 0.3},
                3: {"blend": 0.5, "decay": 0.05, "tone": 0.6, "character": 0.2},
                4: {"blend": 0.5, "decay": 0.35, "character": 0.25},
                5: {"blend": 0.4, "decay": 0.22, "snap": 0.5, "character": 0.25},
                6: {"blend": 0.35, "decay": 0.4, "body": 0.55, "character": 0.2},
                7: {"blend": 0.45, "decay": 0.28, "character": 0.3},
                8: {"blend": 0.55, "decay": 0.3, "character": 0.3},
            },
            global_overrides={"perc_macro_machine": 0.3, "perc_macro_mutate": 0.5}
        ),
        dna={"brightness": 0.5, "warmth": 0.5, "movement": 0.7, "density": 0.5, "space": 0.15, "aggression": 0.4}
    ),
]

# =============================================================================
# FX-HEAVY & PERFORMANCE KITS (10)
# =============================================================================

FX_PRESETS = [
    make_preset(
        "Echo Chamber", "Prism",
        "Heavy delay processing. Long feedback, rhythmic echoes, delay as an instrument.",
        ["echo", "delay", "rhythmic", "feedback", "spatial"],
        make_params(
            voice_overrides={
                1: {"blend": 0.15, "decay": 0.5, "snap": 0.5, "body": 0.6},
                2: {"blend": 0.2, "decay": 0.25, "snap": 0.55, "tone": 0.6},
                3: {"blend": 0.2, "decay": 0.04, "tone": 0.65},
                4: {"blend": 0.2, "decay": 0.35, "tone": 0.6},
                5: {"blend": 0.15, "decay": 0.2, "snap": 0.5},
                6: {"blend": 0.15, "decay": 0.4, "body": 0.55},
                7: {"blend": 0.2, "decay": 0.25},
                8: {"blend": 0.25, "decay": 0.3},
            },
            global_overrides={
                "perc_fx_delay_time": 0.333, "perc_fx_delay_feedback": 0.6, "perc_fx_delay_mix": 0.4,
            }
        ),
        dna={"brightness": 0.5, "warmth": 0.5, "movement": 0.55, "density": 0.5, "space": 0.65, "aggression": 0.3}
    ),

    make_preset(
        "Spring Loaded", "Prism",
        "Bright, springy drums. High reverb with short decay — the sound of spring reverb tanks.",
        ["spring", "reverb", "bright", "bouncy", "tank"],
        make_params(
            voice_overrides={
                1: {"blend": 0.1, "decay": 0.35, "tone": 0.55, "snap": 0.55, "body": 0.5},
                2: {"blend": 0.15, "decay": 0.2, "tone": 0.65, "snap": 0.6},
                3: {"blend": 0.15, "decay": 0.03, "tone": 0.7},
                4: {"blend": 0.15, "decay": 0.3, "tone": 0.65},
                5: {"blend": 0.1, "decay": 0.18, "snap": 0.55},
                6: {"blend": 0.1, "decay": 0.35, "body": 0.5},
                7: {"blend": 0.15, "decay": 0.2},
                8: {"blend": 0.2, "decay": 0.25},
            },
            global_overrides={
                "perc_fx_reverb_mix": 0.4, "perc_fx_reverb_size": 0.3, "perc_fx_reverb_decay": 0.25,
                "perc_masterTone": 0.6,
            }
        ),
        dna={"brightness": 0.6, "warmth": 0.45, "movement": 0.2, "density": 0.5, "space": 0.55, "aggression": 0.35}
    ),

    make_preset(
        "Saturated", "Foundation",
        "Warm saturation on everything. Drive pushed, warmth maxed. Fat, compressed, punchy.",
        ["saturated", "warm", "driven", "fat", "compressed"],
        make_params(
            voice_overrides={
                1: {"blend": 0.05, "decay": 0.45, "tone": 0.45, "snap": 0.5, "body": 0.65},
                2: {"blend": 0.1, "decay": 0.25, "tone": 0.55, "snap": 0.55},
                3: {"blend": 0.1, "decay": 0.04, "tone": 0.6},
                4: {"blend": 0.1, "decay": 0.35, "tone": 0.55},
                5: {"blend": 0.1, "decay": 0.2, "snap": 0.5},
                6: {"blend": 0.05, "decay": 0.4, "body": 0.6},
                7: {"blend": 0.1, "decay": 0.2},
                8: {"blend": 0.15, "decay": 0.25},
            },
            global_overrides={"perc_drive": 0.35, "perc_char_grit": 0.25, "perc_char_warmth": 0.75}
        ),
        dna={"brightness": 0.4, "warmth": 0.8, "movement": 0.1, "density": 0.6, "space": 0.05, "aggression": 0.5}
    ),

    make_preset(
        "Crushed Velvet", "Flux",
        "Extreme bitcrushing with warm character. 8-bit drums wrapped in velvet warmth.",
        ["crushed", "bitcrush", "velvet", "8bit", "warm"],
        make_params(
            voice_overrides={
                1: {"blend": 0.1, "decay": 0.5, "tone": 0.4, "snap": 0.4, "body": 0.6},
                2: {"blend": 0.15, "decay": 0.25, "tone": 0.5, "snap": 0.4},
                3: {"blend": 0.15, "decay": 0.04, "tone": 0.55},
                4: {"blend": 0.15, "decay": 0.35, "tone": 0.5},
                5: {"blend": 0.1, "decay": 0.2, "snap": 0.4},
                6: {"blend": 0.1, "decay": 0.4, "body": 0.6},
                7: {"blend": 0.15, "decay": 0.2},
                8: {"blend": 0.15, "decay": 0.25},
            },
            global_overrides={
                "perc_fx_lofi_bits": 8.0, "perc_fx_lofi_mix": 0.6,
                "perc_char_warmth": 0.7, "perc_masterTone": 0.4,
            }
        ),
        dna={"brightness": 0.3, "warmth": 0.65, "movement": 0.15, "density": 0.5, "space": 0.1, "aggression": 0.4}
    ),

    make_preset(
        "Tape Machine", "Flux",
        "Everything through tape. Warm, slightly wobbly, saturated, with analog character.",
        ["tape", "analog", "warm", "wobbly", "vintage"],
        make_params(
            voice_overrides={
                1: {"blend": 0.05, "decay": 0.5, "tone": 0.4, "snap": 0.4, "body": 0.6, "character": 0.1},
                2: {"blend": 0.1, "decay": 0.3, "tone": 0.5, "snap": 0.4, "character": 0.1},
                3: {"blend": 0.1, "decay": 0.04, "tone": 0.55},
                4: {"blend": 0.1, "decay": 0.35, "tone": 0.5},
                5: {"blend": 0.1, "decay": 0.22, "snap": 0.4},
                6: {"blend": 0.05, "decay": 0.4, "body": 0.6},
                7: {"blend": 0.1, "decay": 0.25},
                8: {"blend": 0.15, "decay": 0.3},
            },
            global_overrides={
                "perc_char_grit": 0.2, "perc_char_warmth": 0.7,
                "perc_fx_lofi_bits": 14.0, "perc_fx_lofi_mix": 0.2,
                "perc_macro_mutate": 0.15, "perc_masterTone": 0.4,
            }
        ),
        dna={"brightness": 0.3, "warmth": 0.75, "movement": 0.2, "density": 0.5, "space": 0.1, "aggression": 0.3}
    ),

    make_preset(
        "Punch Mode", "Foundation",
        "Maximum PUNCH macro. Snap and body biased for impact. Competition-loud drum hits.",
        ["punch", "impact", "loud", "macro", "competition"],
        make_params(
            voice_overrides={
                1: {"blend": 0.1, "decay": 0.4, "tone": 0.5, "snap": 0.65, "body": 0.7},
                2: {"blend": 0.15, "decay": 0.22, "tone": 0.6, "snap": 0.7},
                3: {"blend": 0.1, "decay": 0.03, "tone": 0.7, "snap": 0.55},
                4: {"blend": 0.1, "decay": 0.3, "tone": 0.65},
                5: {"blend": 0.15, "decay": 0.18, "snap": 0.65},
                6: {"blend": 0.1, "decay": 0.35, "body": 0.65, "snap": 0.5},
                7: {"blend": 0.15, "decay": 0.2, "snap": 0.55},
                8: {"blend": 0.2, "decay": 0.25},
            },
            global_overrides={"perc_macro_punch": 0.8, "perc_drive": 0.15}
        ),
        dna={"brightness": 0.55, "warmth": 0.5, "movement": 0.1, "density": 0.6, "space": 0.05, "aggression": 0.65}
    ),

    make_preset(
        "SP1200 Grit", "Foundation",
        "E-mu SP-1200 inspired. 12-bit crunch, warm low end, legendary gritty texture.",
        ["SP1200", "12bit", "gritty", "warm", "classic"],
        make_params(
            voice_overrides={
                1: {"blend": 0.0, "decay": 0.5, "tone": 0.4, "snap": 0.45, "body": 0.65},
                2: {"blend": 0.05, "decay": 0.25, "tone": 0.55, "snap": 0.5},
                3: {"blend": 0.0, "decay": 0.04, "tone": 0.6},
                4: {"blend": 0.0, "decay": 0.35, "tone": 0.55},
                5: {"blend": 0.05, "decay": 0.2, "snap": 0.45},
                6: {"blend": 0.0, "decay": 0.4, "body": 0.6},
                7: {"blend": 0.05, "decay": 0.2},
                8: {"blend": 0.05, "decay": 0.25},
            },
            global_overrides={
                "perc_fx_lofi_bits": 12.0, "perc_fx_lofi_mix": 0.4,
                "perc_char_warmth": 0.65, "perc_char_grit": 0.2,
                "perc_masterTone": 0.45,
            }
        ),
        dna={"brightness": 0.35, "warmth": 0.7, "movement": 0.1, "density": 0.5, "space": 0.1, "aggression": 0.4}
    ),

    make_preset(
        "606 Toy Box", "Prism",
        "Roland TR-606 reimagined. Small, bright, toy-like character with algorithmic shimmer.",
        ["606", "toy", "bright", "small", "retro"],
        make_params(
            voice_overrides={
                1: {"blend": 0.15, "decay": 0.25, "tone": 0.6, "snap": 0.55, "body": 0.4, "pitch": 3},
                2: {"blend": 0.2, "decay": 0.18, "tone": 0.7, "snap": 0.6, "pitch": 2},
                3: {"blend": 0.15, "decay": 0.03, "tone": 0.75},
                4: {"blend": 0.15, "decay": 0.25, "tone": 0.7},
                5: {"blend": 0.2, "decay": 0.15, "snap": 0.55},
                6: {"blend": 0.15, "decay": 0.25, "body": 0.4, "pitch": 5},
                7: {"blend": 0.2, "decay": 0.15, "snap": 0.5},
                8: {"blend": 0.25, "decay": 0.2},
            },
            global_overrides={"perc_masterTone": 0.65}
        ),
        dna={"brightness": 0.7, "warmth": 0.35, "movement": 0.15, "density": 0.45, "space": 0.1, "aggression": 0.35}
    ),

    make_preset(
        "Oberheim DMX", "Prism",
        "DMX-inspired digital character. Crunchy 8-bit transients, bright, punchy, 80s digital warmth.",
        ["DMX", "Oberheim", "digital", "80s", "crunchy"],
        make_params(
            voice_overrides={
                1: {"blend": 0.2, "decay": 0.4, "tone": 0.55, "snap": 0.55, "body": 0.55},
                2: {"blend": 0.25, "decay": 0.22, "tone": 0.65, "snap": 0.6},
                3: {"blend": 0.3, "decay": 0.03, "tone": 0.7},
                4: {"blend": 0.3, "decay": 0.3, "tone": 0.65},
                5: {"blend": 0.25, "decay": 0.18, "snap": 0.55},
                6: {"blend": 0.2, "decay": 0.35, "body": 0.5},
                7: {"blend": 0.3, "decay": 0.2},
                8: {"blend": 0.3, "decay": 0.25},
            },
            global_overrides={
                "perc_fx_lofi_bits": 12.0, "perc_fx_lofi_mix": 0.25,
                "perc_masterTone": 0.6,
            }
        ),
        dna={"brightness": 0.6, "warmth": 0.4, "movement": 0.15, "density": 0.55, "space": 0.1, "aggression": 0.45}
    ),

    make_preset(
        "Simmons Electric", "Prism",
        "Simmons SDS-V inspired. Electronic tom sweeps, pitched kicks, 80s electronic drum character.",
        ["Simmons", "electronic", "80s", "sweep", "pitched"],
        make_params(
            voice_overrides={
                1: {"blend": 0.3, "algoMode": 0, "decay": 0.4, "tone": 0.5, "snap": 0.5, "body": 0.5, "character": 0.3},
                2: {"blend": 0.35, "algoMode": 0, "decay": 0.25, "tone": 0.6, "snap": 0.55, "character": 0.3},
                3: {"blend": 0.4, "decay": 0.03, "tone": 0.7, "character": 0.2},
                4: {"blend": 0.4, "decay": 0.3, "tone": 0.65, "character": 0.2},
                5: {"blend": 0.3, "algoMode": 0, "decay": 0.2, "snap": 0.55, "character": 0.3},
                6: {"blend": 0.35, "algoMode": 0, "decay": 0.5, "body": 0.5, "character": 0.35, "pitch": 5},
                7: {"blend": 0.4, "algoMode": 0, "decay": 0.3, "character": 0.3, "pitch": 3},
                8: {"blend": 0.45, "algoMode": 0, "decay": 0.35, "character": 0.3},
            },
            global_overrides={"perc_fx_reverb_mix": 0.2, "perc_fx_reverb_size": 0.4}
        ),
        dna={"brightness": 0.6, "warmth": 0.35, "movement": 0.25, "density": 0.5, "space": 0.35, "aggression": 0.45}
    ),
]


# =============================================================================
# WORLD PERCUSSION (12) — Culturally-specific acoustic drum character
# =============================================================================

WORLD_PRESETS = [
    make_preset(
        "Taiko Strike", "Foundation",
        "Japanese taiko drum. Deep resonant body, sharp attack, powerful decay. Pure circuit warmth.",
        ["taiko", "japanese", "acoustic", "deep", "powerful"],
        make_params(
            voice_overrides={
                1: {"blend": 0.0, "decay": 0.7, "tone": 0.35, "snap": 0.55, "body": 0.85, "character": 0.05},
                2: {"blend": 0.05, "decay": 0.45, "tone": 0.45, "snap": 0.5, "body": 0.7},
                3: {"blend": 0.1, "decay": 0.08, "tone": 0.55},
                4: {"blend": 0.1, "decay": 0.5, "tone": 0.5},
                5: {"blend": 0.05, "decay": 0.3, "snap": 0.45, "body": 0.55},
                6: {"blend": 0.0, "decay": 0.65, "body": 0.8, "pitch": -2},
                7: {"blend": 0.05, "decay": 0.35, "snap": 0.4},
                8: {"blend": 0.1, "decay": 0.4, "body": 0.6},
            },
            global_overrides={"perc_char_warmth": 0.6, "perc_fx_reverb_mix": 0.2, "perc_fx_reverb_size": 0.45}
        ),
        dna={"brightness": 0.3, "warmth": 0.7, "movement": 0.1, "density": 0.6, "space": 0.35, "aggression": 0.55}
    ),

    make_preset(
        "Djembe Circuit", "Foundation",
        "West African djembe through circuit models. Bass, tone, slap voices — organic hand drum character.",
        ["djembe", "african", "hand-drum", "organic", "bass-tone-slap"],
        make_params(
            voice_overrides={
                1: {"blend": 0.0, "decay": 0.4, "tone": 0.4, "snap": 0.35, "body": 0.7},    # bass
                2: {"blend": 0.05, "decay": 0.2, "tone": 0.6, "snap": 0.5, "character": 0.1}, # tone
                3: {"blend": 0.1, "decay": 0.05, "tone": 0.7, "snap": 0.6},                   # slap
                4: {"blend": 0.1, "decay": 0.25, "tone": 0.65, "snap": 0.55},
                5: {"blend": 0.05, "decay": 0.15, "snap": 0.5},
                6: {"blend": 0.0, "decay": 0.35, "body": 0.65, "pitch": 3},
                7: {"blend": 0.05, "decay": 0.15, "snap": 0.45},
                8: {"blend": 0.1, "decay": 0.2},
            },
            global_overrides={"perc_char_warmth": 0.6, "perc_masterTone": 0.5}
        ),
        dna={"brightness": 0.45, "warmth": 0.65, "movement": 0.35, "density": 0.55, "space": 0.1, "aggression": 0.35}
    ),

    make_preset(
        "Batucada Rio", "Foundation",
        "Brazilian samba percussion. Surdo bass, caixa snare, repinique hits — Rio carnival energy.",
        ["batucada", "samba", "brazilian", "carnival", "percussion"],
        make_params(
            voice_overrides={
                1: {"blend": 0.0, "decay": 0.6, "tone": 0.4, "snap": 0.4, "body": 0.75},   # surdo
                2: {"blend": 0.05, "decay": 0.15, "tone": 0.65, "snap": 0.65},               # caixa
                3: {"blend": 0.0, "decay": 0.03, "tone": 0.7},
                4: {"blend": 0.0, "decay": 0.25, "tone": 0.65},
                5: {"blend": 0.05, "decay": 0.12, "snap": 0.6},                               # repinique
                6: {"blend": 0.0, "decay": 0.45, "body": 0.65, "pitch": 4},
                7: {"blend": 0.05, "decay": 0.15, "snap": 0.55},
                8: {"blend": 0.1, "decay": 0.2},
            },
            global_overrides={"perc_drive": 0.1, "perc_masterTone": 0.55}
        ),
        dna={"brightness": 0.55, "warmth": 0.5, "movement": 0.5, "density": 0.65, "space": 0.1, "aggression": 0.5}
    ),

    make_preset(
        "Tabla Resonance", "Prism",
        "North Indian tabla through Modal resonators. Bayan bass, dayan treble — tuned membrane character.",
        ["tabla", "indian", "classical", "modal", "tuned"],
        make_params(
            voice_overrides={
                1: {"blend": 0.6, "algoMode": 1, "decay": 0.45, "tone": 0.35, "snap": 0.4, "body": 0.65, "character": 0.35},  # bayan
                2: {"blend": 0.7, "algoMode": 1, "decay": 0.3, "tone": 0.7, "snap": 0.5, "character": 0.4, "pitch": 5},      # dayan
                3: {"blend": 0.75, "algoMode": 1, "decay": 0.06, "tone": 0.75, "character": 0.35},
                4: {"blend": 0.75, "algoMode": 1, "decay": 0.35, "tone": 0.7, "pitch": 3},
                5: {"blend": 0.65, "algoMode": 1, "decay": 0.2, "snap": 0.5, "character": 0.3},
                6: {"blend": 0.6, "algoMode": 1, "decay": 0.4, "body": 0.6, "character": 0.3},
                7: {"blend": 0.7, "algoMode": 1, "decay": 0.25, "character": 0.4, "pitch": 7},
                8: {"blend": 0.75, "algoMode": 1, "decay": 0.3, "character": 0.45},
            },
            global_overrides={"perc_fx_reverb_mix": 0.15, "perc_fx_reverb_size": 0.4}
        ),
        dna={"brightness": 0.6, "warmth": 0.55, "movement": 0.3, "density": 0.55, "space": 0.3, "aggression": 0.25}
    ),

    make_preset(
        "Mbira Kalimba", "Prism",
        "African thumb piano through KS synthesis. Plucked tines with resonant gourd overtones.",
        ["mbira", "kalimba", "thumb-piano", "african", "plucked"],
        make_params(
            voice_overrides={
                1: {"blend": 0.85, "algoMode": 2, "decay": 0.6, "tone": 0.55, "snap": 0.45, "body": 0.5, "character": 0.3, "pitch": -7},
                2: {"blend": 0.9, "algoMode": 2, "decay": 0.5, "tone": 0.6, "snap": 0.5, "character": 0.3, "pitch": -3},
                3: {"blend": 0.9, "algoMode": 2, "decay": 0.4, "tone": 0.65, "snap": 0.4, "character": 0.25},
                4: {"blend": 0.9, "algoMode": 2, "decay": 0.55, "tone": 0.6, "pitch": 2, "character": 0.3},
                5: {"blend": 0.85, "algoMode": 2, "decay": 0.45, "snap": 0.45, "character": 0.25, "pitch": 5},
                6: {"blend": 0.8, "algoMode": 2, "decay": 0.5, "body": 0.5, "character": 0.2},
                7: {"blend": 0.85, "algoMode": 2, "decay": 0.4, "character": 0.35, "pitch": 9},
                8: {"blend": 0.9, "algoMode": 2, "decay": 0.5, "character": 0.4, "pitch": 12},
            },
            global_overrides={"perc_fx_reverb_mix": 0.25, "perc_fx_reverb_size": 0.45}
        ),
        dna={"brightness": 0.65, "warmth": 0.55, "movement": 0.2, "density": 0.4, "space": 0.45, "aggression": 0.1}
    ),

    make_preset(
        "Cajon Box", "Foundation",
        "Spanish cajon — bass thump, slap crack, snare wire rattle. Compact wooden box percussion.",
        ["cajon", "spanish", "flamenco", "wooden", "compact"],
        make_params(
            voice_overrides={
                1: {"blend": 0.0, "decay": 0.4, "tone": 0.45, "snap": 0.45, "body": 0.65},   # bass
                2: {"blend": 0.05, "decay": 0.25, "tone": 0.6, "snap": 0.65, "character": 0.1}, # slap
                3: {"blend": 0.1, "decay": 0.04, "tone": 0.65, "snap": 0.5},                    # wire
                4: {"blend": 0.1, "decay": 0.3, "tone": 0.6, "snap": 0.5},
                5: {"blend": 0.05, "decay": 0.15, "snap": 0.55},
                6: {"blend": 0.0, "decay": 0.35, "body": 0.6},
                7: {"blend": 0.05, "decay": 0.18},
                8: {"blend": 0.1, "decay": 0.25},
            },
            global_overrides={"perc_char_warmth": 0.55}
        ),
        dna={"brightness": 0.5, "warmth": 0.6, "movement": 0.3, "density": 0.5, "space": 0.1, "aggression": 0.35}
    ),

    make_preset(
        "Steel Pan Digital", "Prism",
        "Caribbean steel pan through FM and Modal. Tuned metal resonance — each voice a different pitch.",
        ["steel-pan", "caribbean", "tuned", "FM", "modal"],
        make_params(
            voice_overrides={
                1: {"blend": 0.7, "algoMode": 1, "decay": 0.65, "tone": 0.5, "snap": 0.45, "body": 0.5, "character": 0.4, "pitch": -7},
                2: {"blend": 0.75, "algoMode": 1, "decay": 0.55, "tone": 0.6, "character": 0.45, "pitch": -4},
                3: {"blend": 0.8, "algoMode": 1, "decay": 0.45, "tone": 0.65, "character": 0.4},
                4: {"blend": 0.8, "algoMode": 1, "decay": 0.6, "tone": 0.6, "character": 0.4, "pitch": 3},
                5: {"blend": 0.75, "algoMode": 1, "decay": 0.5, "snap": 0.45, "character": 0.35, "pitch": 5},
                6: {"blend": 0.7, "algoMode": 1, "decay": 0.55, "body": 0.5, "character": 0.35, "pitch": 7},
                7: {"blend": 0.75, "algoMode": 1, "decay": 0.5, "character": 0.4, "pitch": 10},
                8: {"blend": 0.8, "algoMode": 1, "decay": 0.55, "character": 0.45, "pitch": 12},
            },
            global_overrides={"perc_fx_reverb_mix": 0.3, "perc_fx_reverb_size": 0.5}
        ),
        dna={"brightness": 0.7, "warmth": 0.45, "movement": 0.2, "density": 0.45, "space": 0.5, "aggression": 0.15}
    ),

    make_preset(
        "Konnakol Micro", "Prism",
        "South Indian konnakol-inspired micro-percussion. Fast syllabic hits with modal ring.",
        ["konnakol", "carnatic", "micro", "fast", "syllabic"],
        make_params(
            voice_overrides={
                1: {"blend": 0.5, "algoMode": 1, "decay": 0.2, "tone": 0.55, "snap": 0.55, "body": 0.45, "character": 0.35},
                2: {"blend": 0.55, "algoMode": 1, "decay": 0.15, "tone": 0.65, "snap": 0.6, "character": 0.4},
                3: {"blend": 0.6, "algoMode": 1, "decay": 0.04, "tone": 0.7, "snap": 0.5, "character": 0.35},
                4: {"blend": 0.6, "algoMode": 1, "decay": 0.2, "tone": 0.65, "pitch": 4},
                5: {"blend": 0.5, "algoMode": 1, "decay": 0.12, "snap": 0.55, "character": 0.35},
                6: {"blend": 0.45, "algoMode": 1, "decay": 0.2, "body": 0.5, "character": 0.3},
                7: {"blend": 0.55, "algoMode": 1, "decay": 0.15, "character": 0.4, "pitch": 7},
                8: {"blend": 0.6, "algoMode": 1, "decay": 0.18, "character": 0.45},
            },
            global_overrides={"perc_masterTone": 0.6}
        ),
        dna={"brightness": 0.65, "warmth": 0.45, "movement": 0.6, "density": 0.65, "space": 0.1, "aggression": 0.3}
    ),

    make_preset(
        "Balafon Wood", "Prism",
        "West African balafon — wooden keys over gourd resonators. KS synthesis with warm decay.",
        ["balafon", "african", "wooden", "tuned", "marimba"],
        make_params(
            voice_overrides={
                1: {"blend": 0.8, "algoMode": 2, "decay": 0.4, "tone": 0.4, "snap": 0.55, "body": 0.6, "character": 0.25, "pitch": -9},
                2: {"blend": 0.85, "algoMode": 2, "decay": 0.35, "tone": 0.5, "snap": 0.5, "character": 0.25, "pitch": -5},
                3: {"blend": 0.85, "algoMode": 2, "decay": 0.3, "tone": 0.55, "snap": 0.45, "character": 0.2},
                4: {"blend": 0.85, "algoMode": 2, "decay": 0.4, "tone": 0.5, "pitch": 2},
                5: {"blend": 0.8, "algoMode": 2, "decay": 0.3, "snap": 0.5, "character": 0.2, "pitch": 5},
                6: {"blend": 0.75, "algoMode": 2, "decay": 0.45, "body": 0.55, "character": 0.2},
                7: {"blend": 0.8, "algoMode": 2, "decay": 0.35, "character": 0.25, "pitch": 9},
                8: {"blend": 0.85, "algoMode": 2, "decay": 0.4, "character": 0.3, "pitch": 12},
            },
            global_overrides={"perc_char_warmth": 0.6, "perc_fx_reverb_mix": 0.2}
        ),
        dna={"brightness": 0.55, "warmth": 0.6, "movement": 0.2, "density": 0.45, "space": 0.35, "aggression": 0.1}
    ),

    make_preset(
        "Gamelan Strike", "Prism",
        "Balinese gamelan — metallic gong and gender tones through Phase Distortion synthesis.",
        ["gamelan", "balinese", "gong", "metallic", "phase-distortion"],
        make_params(
            voice_overrides={
                1: {"blend": 0.85, "algoMode": 3, "decay": 0.8, "tone": 0.45, "snap": 0.4, "body": 0.55, "character": 0.55, "pitch": -5},
                2: {"blend": 0.9, "algoMode": 3, "decay": 0.6, "tone": 0.55, "character": 0.6, "pitch": -2},
                3: {"blend": 0.9, "algoMode": 3, "decay": 0.3, "tone": 0.65, "character": 0.5},
                4: {"blend": 0.9, "algoMode": 3, "decay": 0.7, "tone": 0.6, "character": 0.55, "pitch": 3},
                5: {"blend": 0.85, "algoMode": 3, "decay": 0.5, "snap": 0.4, "character": 0.5, "pitch": 5},
                6: {"blend": 0.8, "algoMode": 3, "decay": 0.65, "body": 0.5, "character": 0.5},
                7: {"blend": 0.85, "algoMode": 3, "decay": 0.55, "character": 0.6, "pitch": 7},
                8: {"blend": 0.9, "algoMode": 3, "decay": 0.7, "character": 0.55, "pitch": 10},
            },
            global_overrides={"perc_fx_reverb_mix": 0.35, "perc_fx_reverb_size": 0.55}
        ),
        dna={"brightness": 0.65, "warmth": 0.4, "movement": 0.25, "density": 0.5, "space": 0.55, "aggression": 0.3}
    ),

    make_preset(
        "Nyahbinghi Roots", "Foundation",
        "Rastafarian nyahbinghi ceremony drums. Funde bass rhythm, repeater lead, akete percussion.",
        ["nyahbinghi", "rastafarian", "ceremonial", "roots", "dub"],
        make_params(
            voice_overrides={
                1: {"blend": 0.0, "decay": 0.55, "tone": 0.35, "snap": 0.3, "body": 0.75},   # funde bass
                2: {"blend": 0.05, "decay": 0.3, "tone": 0.5, "snap": 0.4},                   # funde mid
                3: {"blend": 0.05, "decay": 0.04, "tone": 0.6},
                4: {"blend": 0.1, "decay": 0.35, "tone": 0.5},
                5: {"blend": 0.1, "decay": 0.2, "snap": 0.4},                                  # repeater
                6: {"blend": 0.0, "decay": 0.45, "body": 0.7, "pitch": -3},
                7: {"blend": 0.05, "decay": 0.2, "snap": 0.35},
                8: {"blend": 0.1, "decay": 0.3},
            },
            global_overrides={"perc_char_warmth": 0.65, "perc_fx_reverb_mix": 0.15}
        ),
        dna={"brightness": 0.3, "warmth": 0.7, "movement": 0.25, "density": 0.5, "space": 0.25, "aggression": 0.2}
    ),

    make_preset(
        "Darbuka Digital", "Prism",
        "Middle Eastern darbuka through FM and Modal synthesis. Doum, tek, ka — algorithmic riq.",
        ["darbuka", "middle-eastern", "riq", "FM", "doum-tek-ka"],
        make_params(
            voice_overrides={
                1: {"blend": 0.6, "algoMode": 0, "decay": 0.4, "tone": 0.4, "snap": 0.45, "body": 0.6, "character": 0.35},  # doum
                2: {"blend": 0.65, "algoMode": 1, "decay": 0.2, "tone": 0.7, "snap": 0.6, "character": 0.4},                # tek
                3: {"blend": 0.7, "algoMode": 1, "decay": 0.05, "tone": 0.75, "snap": 0.5, "character": 0.35},              # ka
                4: {"blend": 0.7, "algoMode": 0, "decay": 0.25, "tone": 0.65, "character": 0.35},
                5: {"blend": 0.6, "algoMode": 1, "decay": 0.15, "snap": 0.55, "character": 0.35},
                6: {"blend": 0.55, "algoMode": 0, "decay": 0.35, "body": 0.55, "character": 0.3},
                7: {"blend": 0.65, "algoMode": 1, "decay": 0.2, "character": 0.4},
                8: {"blend": 0.7, "algoMode": 0, "decay": 0.25, "character": 0.35},
            },
            global_overrides={"perc_fx_reverb_mix": 0.15}
        ),
        dna={"brightness": 0.6, "warmth": 0.5, "movement": 0.4, "density": 0.55, "space": 0.2, "aggression": 0.3}
    ),
]

# =============================================================================
# DNA CORNER FILLS (8) — Target underrepresented sonic extremes
# =============================================================================

DNA_CORNER_PRESETS = [
    # Max aggression + max warmth (paradox: brutally warm)
    make_preset(
        "Angry Velvet", "Flux",
        "Paradox: maximum aggression with maximum warmth. Brutal but soft. Velvet hammer hits.",
        ["paradox", "aggressive", "warm", "brutal", "velvet"],
        make_params(
            voice_overrides={
                1: {"blend": 0.05, "decay": 0.4, "snap": 0.7, "body": 0.7, "character": 0.2},
                2: {"blend": 0.1, "decay": 0.2, "snap": 0.75, "character": 0.2},
                3: {"blend": 0.1, "decay": 0.03, "tone": 0.55, "snap": 0.6},
                4: {"blend": 0.1, "decay": 0.25, "tone": 0.5},
                5: {"blend": 0.1, "decay": 0.15, "snap": 0.65},
                6: {"blend": 0.05, "decay": 0.35, "body": 0.7},
                7: {"blend": 0.1, "decay": 0.18, "snap": 0.55},
                8: {"blend": 0.1, "decay": 0.2},
            },
            global_overrides={"perc_drive": 0.4, "perc_char_grit": 0.1, "perc_char_warmth": 0.8,
                               "perc_fx_lofi_bits": 12.0, "perc_fx_lofi_mix": 0.15}
        ),
        dna={"brightness": 0.4, "warmth": 0.85, "movement": 0.2, "density": 0.65, "space": 0.05, "aggression": 0.9}
    ),

    # Max movement + near-zero density (ghost chaos)
    make_preset(
        "Ghost Chaos", "Aether",
        "Maximum MUTATE with near-silence. Barely-there hits that never repeat. Ghost-level presence.",
        ["ghost", "chaos", "sparse", "mutate", "barely-there"],
        make_params(
            voice_overrides={
                1: {"blend": 0.5, "decay": 0.3, "snap": 0.15, "body": 0.25},
                2: {"blend": 0.5, "decay": 0.2, "snap": 0.15},
                3: {"blend": 0.5, "decay": 0.04, "tone": 0.5},
                4: {"blend": 0.5, "decay": 0.2},
                5: {"blend": 0.5, "decay": 0.15, "snap": 0.15},
                6: {"blend": 0.5, "decay": 0.25, "body": 0.25},
                7: {"blend": 0.5, "decay": 0.18},
                8: {"blend": 0.5, "decay": 0.2},
            },
            global_overrides={"perc_macro_mutate": 1.0, "perc_level": 0.4,
                               "perc_fx_reverb_mix": 0.55, "perc_fx_reverb_size": 0.7}
        ),
        dna={"brightness": 0.5, "warmth": 0.4, "movement": 1.0, "density": 0.1, "space": 0.7, "aggression": 0.1}
    ),

    # Max space + max density (overwhelming room)
    make_preset(
        "Cathedral Collapse", "Aether",
        "Enormous reverb + densely layered hits. The whole room resonating. Overwhelming spatial mass.",
        ["cathedral", "massive", "dense", "reverb", "overwhelming"],
        make_params(
            voice_overrides={
                1: {"blend": 0.2, "decay": 0.6, "snap": 0.5, "body": 0.7, "character": 0.2},
                2: {"blend": 0.25, "decay": 0.35, "snap": 0.5, "character": 0.2},
                3: {"blend": 0.3, "decay": 0.06, "tone": 0.6, "character": 0.15},
                4: {"blend": 0.3, "decay": 0.4, "character": 0.15},
                5: {"blend": 0.2, "decay": 0.25, "snap": 0.5},
                6: {"blend": 0.15, "decay": 0.5, "body": 0.65},
                7: {"blend": 0.25, "decay": 0.3, "character": 0.15},
                8: {"blend": 0.3, "decay": 0.4, "character": 0.2},
            },
            global_overrides={
                "perc_fx_reverb_mix": 0.75, "perc_fx_reverb_size": 0.9, "perc_fx_reverb_decay": 0.8,
                "perc_fx_delay_mix": 0.3, "perc_fx_delay_feedback": 0.55, "perc_char_warmth": 0.55,
            }
        ),
        dna={"brightness": 0.4, "warmth": 0.55, "movement": 0.2, "density": 0.9, "space": 0.95, "aggression": 0.35}
    ),

    # Max brightness + zero aggression (crystalline gentle)
    make_preset(
        "Crystal Gentle", "Atmosphere",
        "Maximum brightness with zero aggression. Pure, crystalline, utterly gentle Modal tones.",
        ["crystal", "gentle", "bright", "delicate", "pure"],
        make_params(
            voice_overrides={
                1: {"blend": 0.95, "algoMode": 1, "decay": 0.5, "tone": 0.85, "snap": 0.15, "body": 0.2, "character": 0.6},
                2: {"blend": 0.95, "algoMode": 1, "decay": 0.35, "tone": 0.9, "snap": 0.1, "character": 0.65},
                3: {"blend": 1.0, "algoMode": 1, "decay": 0.08, "tone": 0.95, "character": 0.6},
                4: {"blend": 1.0, "algoMode": 1, "decay": 0.4, "tone": 0.9, "character": 0.6},
                5: {"blend": 0.95, "algoMode": 1, "decay": 0.25, "snap": 0.1, "character": 0.55},
                6: {"blend": 0.9, "algoMode": 1, "decay": 0.45, "body": 0.25, "character": 0.5},
                7: {"blend": 0.95, "algoMode": 1, "decay": 0.3, "character": 0.65},
                8: {"blend": 1.0, "algoMode": 1, "decay": 0.35, "character": 0.7},
            },
            global_overrides={
                "perc_level": 0.6, "perc_fx_reverb_mix": 0.4, "perc_fx_reverb_size": 0.55,
                "perc_masterTone": 0.75,
            }
        ),
        dna={"brightness": 0.95, "warmth": 0.3, "movement": 0.1, "density": 0.3, "space": 0.55, "aggression": 0.02}
    ),

    # High warmth + zero density (single warm hit)
    make_preset(
        "One Warm Kick", "Foundation",
        "The simplest kit: one perfect warm kick, everything else silent. Pure bass thud. Full focus.",
        ["minimal", "one-shot", "kick", "warm", "focused"],
        make_params(
            voice_overrides={
                1: {"blend": 0.0, "decay": 0.65, "tone": 0.3, "snap": 0.4, "body": 0.85},
                2: {"blend": 0.0, "decay": 0.001, "level": 0.0},
                3: {"blend": 0.0, "decay": 0.001, "level": 0.0},
                4: {"blend": 0.0, "decay": 0.001, "level": 0.0},
                5: {"blend": 0.0, "decay": 0.001, "level": 0.0},
                6: {"blend": 0.0, "decay": 0.001, "level": 0.0},
                7: {"blend": 0.0, "decay": 0.001, "level": 0.0},
                8: {"blend": 0.0, "decay": 0.001, "level": 0.0},
            },
            global_overrides={"perc_char_warmth": 0.75, "perc_masterTone": 0.4}
        ),
        dna={"brightness": 0.2, "warmth": 0.9, "movement": 0.05, "density": 0.05, "space": 0.05, "aggression": 0.2}
    ),

    # Max density + min space (wall of sound)
    make_preset(
        "Dry Wall", "Foundation",
        "Zero reverb, zero delay. Every voice tight and dry. Maximum density, minimum space. A wall.",
        ["dry", "tight", "dense", "no-reverb", "wall"],
        make_params(
            voice_overrides={
                1: {"blend": 0.1, "decay": 0.3, "snap": 0.65, "body": 0.6},
                2: {"blend": 0.1, "decay": 0.18, "snap": 0.7},
                3: {"blend": 0.05, "decay": 0.02, "tone": 0.7, "snap": 0.6},
                4: {"blend": 0.05, "decay": 0.2, "tone": 0.65},
                5: {"blend": 0.1, "decay": 0.12, "snap": 0.6},
                6: {"blend": 0.05, "decay": 0.3, "body": 0.6},
                7: {"blend": 0.1, "decay": 0.15, "snap": 0.55},
                8: {"blend": 0.1, "decay": 0.18},
            },
            global_overrides={
                "perc_fx_reverb_mix": 0.0, "perc_fx_delay_mix": 0.0, "perc_fx_lofi_mix": 0.0,
                "perc_drive": 0.2, "perc_masterTone": 0.6,
            }
        ),
        dna={"brightness": 0.55, "warmth": 0.45, "movement": 0.1, "density": 0.95, "space": 0.02, "aggression": 0.55}
    ),

    # Max brightness + max aggression (industrial metallic)
    make_preset(
        "Metallic Fury", "Flux",
        "Brightest, most aggressive possible kit. Metallic Modal + heavy drive. Industrial percussive violence.",
        ["metallic", "bright", "aggressive", "industrial", "fury"],
        make_params(
            voice_overrides={
                1: {"blend": 0.4, "algoMode": 1, "decay": 0.3, "tone": 0.85, "snap": 0.75, "body": 0.35, "character": 0.7},
                2: {"blend": 0.5, "algoMode": 1, "decay": 0.2, "tone": 0.9, "snap": 0.8, "character": 0.75},
                3: {"blend": 0.6, "algoMode": 1, "decay": 0.03, "tone": 0.95, "character": 0.65},
                4: {"blend": 0.6, "algoMode": 1, "decay": 0.2, "tone": 0.9, "character": 0.7},
                5: {"blend": 0.5, "algoMode": 1, "decay": 0.15, "snap": 0.75, "character": 0.65},
                6: {"blend": 0.4, "algoMode": 1, "decay": 0.25, "tone": 0.85, "character": 0.6},
                7: {"blend": 0.55, "algoMode": 1, "decay": 0.18, "character": 0.75},
                8: {"blend": 0.65, "algoMode": 1, "decay": 0.22, "character": 0.8},
            },
            global_overrides={"perc_drive": 0.45, "perc_char_grit": 0.45, "perc_masterTone": 0.65}
        ),
        dna={"brightness": 0.95, "warmth": 0.15, "movement": 0.2, "density": 0.7, "space": 0.05, "aggression": 0.95}
    ),

    # Low brightness + low density + high movement (subtle living texture)
    make_preset(
        "Living Texture", "Atmosphere",
        "Warm, dark, sparse — but MUTATE makes it breathe. Barely visible but always alive.",
        ["texture", "dark", "sparse", "breathing", "subtle"],
        make_params(
            voice_overrides={
                1: {"blend": 0.3, "decay": 0.5, "tone": 0.3, "snap": 0.2, "body": 0.5},
                2: {"blend": 0.35, "decay": 0.3, "tone": 0.35, "snap": 0.2},
                3: {"blend": 0.4, "decay": 0.04, "tone": 0.35},
                4: {"blend": 0.4, "decay": 0.3, "tone": 0.3},
                5: {"blend": 0.3, "decay": 0.2, "snap": 0.2},
                6: {"blend": 0.25, "decay": 0.4, "body": 0.45},
                7: {"blend": 0.35, "decay": 0.25},
                8: {"blend": 0.4, "decay": 0.3},
            },
            global_overrides={
                "perc_macro_mutate": 0.7, "perc_level": 0.5,
                "perc_fx_reverb_mix": 0.35, "perc_char_warmth": 0.65,
                "perc_masterTone": 0.35,
            }
        ),
        dna={"brightness": 0.2, "warmth": 0.65, "movement": 0.8, "density": 0.2, "space": 0.5, "aggression": 0.05}
    ),
]


ALL_PRESETS = {
    "Hero": HERO_PRESETS,
    "Replica & Extended": REPLICA_PRESETS,
    "Genre": GENRE_PRESETS,
    "Hybrid & Morphing": HYBRID_PRESETS,
    "Coupling": COUPLING_PRESETS,
    "Atmosphere & Aether": ATMOSPHERE_PRESETS,
    "FX & Performance": FX_PRESETS,
    "World Percussion": WORLD_PRESETS,
    "DNA Corner Fills": DNA_CORNER_PRESETS,
}


def main():
    print("Generating XOnset presets...")
    total = 0
    for category, presets in ALL_PRESETS.items():
        print(f"\n=== {category} ({len(presets)}) ===")
        for p in presets:
            write_preset(p, p["mood"])
        total += len(presets)
    print(f"\nTotal: {total} presets generated")


if __name__ == "__main__":
    main()
