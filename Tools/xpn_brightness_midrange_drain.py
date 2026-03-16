#!/usr/bin/env python3
"""
xpn_brightness_midrange_drain.py

Floods extreme brightness zones (≤0.12 or ≥0.88) to counter the 60.6% midrange cluster.
Generates 80 presets: 40 Foundation + 40 Atmosphere, each split 20 dark / 20 bright.

Run from repo root or Tools/ directory.
"""

import json
import os
import random
import sys

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FOUNDATION_DIR = os.path.join(REPO_ROOT, "Presets", "XOmnibus", "Foundation")
ATMOSPHERE_DIR = os.path.join(REPO_ROOT, "Presets", "XOmnibus", "Atmosphere")

random.seed(42)  # reproducible


# ---------------------------------------------------------------------------
# Engine parameter tables — representative param sets per engine prefix
# ---------------------------------------------------------------------------

ENGINE_PARAMS = {
    "OBLONG": {
        "prefix": "bob_",
        "params": ["bob_osc1wave", "bob_osc2wave", "bob_filterCutoff", "bob_filterReso",
                   "bob_ampAttack", "bob_ampDecay", "bob_ampSustain", "bob_ampRelease",
                   "bob_lfoRate", "bob_lfoDepth", "bob_reverbMix", "bob_delayMix",
                   "bob_character", "bob_movement", "bob_space", "bob_coupling"],
    },
    "OBESE": {
        "prefix": "fat_",
        "params": ["fat_drive", "fat_saturation", "fat_filterCutoff", "fat_filterReso",
                   "fat_ampAttack", "fat_ampDecay", "fat_ampSustain", "fat_ampRelease",
                   "fat_subLevel", "fat_bodyLevel", "fat_reverbMix", "fat_delayMix",
                   "fat_character", "fat_movement", "fat_space", "fat_coupling"],
    },
    "OVERDUB": {
        "prefix": "dub_",
        "params": ["dub_drive", "dub_tapeDelay", "dub_springReverb", "dub_filterCutoff",
                   "dub_ampAttack", "dub_ampDecay", "dub_ampSustain", "dub_ampRelease",
                   "dub_lfoRate", "dub_lfoDepth", "dub_reverbMix", "dub_delayMix",
                   "dub_character", "dub_movement", "dub_space", "dub_coupling"],
    },
    "OVERBITE": {
        "prefix": "poss_",
        "params": ["poss_filterCutoff", "poss_filterReso", "poss_envAmount",
                   "poss_ampAttack", "poss_ampDecay", "poss_ampSustain", "poss_ampRelease",
                   "poss_lfoRate", "poss_lfoDepth", "poss_reverbMix", "poss_delayMix",
                   "poss_character", "poss_movement", "poss_space", "poss_coupling",
                   "poss_bite"],
    },
    "ODDOSCAR": {
        "prefix": "oddo_",
        "params": ["oddo_oscAwave", "oddo_oscBwave", "oddo_filterCutoff", "oddo_filterReso",
                   "oddo_ampAttack", "oddo_ampDecay", "oddo_ampSustain", "oddo_ampRelease",
                   "oddo_lfoRate", "oddo_lfoDepth", "oddo_reverbMix", "oddo_delayMix",
                   "oddo_character", "oddo_movement", "oddo_space", "oddo_coupling"],
    },
    "ORGANON": {
        "prefix": "org_",
        "params": ["org_drawbar1", "org_drawbar2", "org_drawbar3", "org_filterCutoff",
                   "org_ampAttack", "org_ampDecay", "org_ampSustain", "org_ampRelease",
                   "org_rotarySpeed", "org_rotaryDepth", "org_reverbMix", "org_delayMix",
                   "org_character", "org_movement", "org_space", "org_coupling"],
    },
    "OBSIDIAN": {
        "prefix": "obs_",
        "params": ["obs_glassResonance", "obs_filterCutoff", "obs_filterReso",
                   "obs_ampAttack", "obs_ampDecay", "obs_ampSustain", "obs_ampRelease",
                   "obs_lfoRate", "obs_lfoDepth", "obs_reverbMix", "obs_delayMix",
                   "obs_character", "obs_movement", "obs_space", "obs_coupling",
                   "obs_darkMatter"],
    },
    "ORACLE": {
        "prefix": "orc_",
        "params": ["orc_prophecyDepth", "orc_filterCutoff", "orc_filterReso",
                   "orc_ampAttack", "orc_ampDecay", "orc_ampSustain", "orc_ampRelease",
                   "orc_lfoRate", "orc_lfoDepth", "orc_reverbMix", "orc_delayMix",
                   "orc_character", "orc_movement", "orc_space", "orc_coupling",
                   "orc_vision"],
    },
    "ORIGAMI": {
        "prefix": "ori_",
        "params": ["ori_foldDepth", "ori_foldRate", "ori_filterCutoff", "ori_filterReso",
                   "ori_ampAttack", "ori_ampDecay", "ori_ampSustain", "ori_ampRelease",
                   "ori_lfoRate", "ori_lfoDepth", "ori_reverbMix", "ori_delayMix",
                   "ori_character", "ori_movement", "ori_space", "ori_coupling"],
    },
    "OUROBOROS": {
        "prefix": "uro_",
        "params": ["uro_feedbackLoop", "uro_filterCutoff", "uro_filterReso",
                   "uro_ampAttack", "uro_ampDecay", "uro_ampSustain", "uro_ampRelease",
                   "uro_lfoRate", "uro_lfoDepth", "uro_reverbMix", "uro_delayMix",
                   "uro_character", "uro_movement", "uro_space", "uro_coupling",
                   "uro_cycle"],
    },
    "ODDFELIX": {
        "prefix": "oddf_",
        "params": ["oddf_oscAwave", "oddf_oscBwave", "oddf_filterCutoff", "oddf_filterReso",
                   "oddf_ampAttack", "oddf_ampDecay", "oddf_ampSustain", "oddf_ampRelease",
                   "oddf_lfoRate", "oddf_lfoDepth", "oddf_reverbMix", "oddf_delayMix",
                   "oddf_character", "oddf_movement", "oddf_space", "oddf_coupling"],
    },
    "OPTIC": {
        "prefix": "opt_",
        "params": ["opt_prismAngle", "opt_spectralShift", "opt_filterCutoff", "opt_filterReso",
                   "opt_ampAttack", "opt_ampDecay", "opt_ampSustain", "opt_ampRelease",
                   "opt_lfoRate", "opt_lfoDepth", "opt_reverbMix", "opt_delayMix",
                   "opt_character", "opt_movement", "opt_space", "opt_coupling"],
    },
    "ORPHICA": {
        "prefix": "orph_",
        "params": ["orph_pluckDecay", "orph_resonanceBody", "orph_filterCutoff", "orph_filterReso",
                   "orph_ampAttack", "orph_ampDecay", "orph_ampSustain", "orph_ampRelease",
                   "orph_lfoRate", "orph_lfoDepth", "orph_reverbMix", "orph_delayMix",
                   "orph_character", "orph_movement", "orph_space", "orph_coupling"],
    },
    "OBLIQUE": {
        "prefix": "oblq_",
        "params": ["oblq_warpAngle", "oblq_filterCutoff", "oblq_filterReso",
                   "oblq_ampAttack", "oblq_ampDecay", "oblq_ampSustain", "oblq_ampRelease",
                   "oblq_lfoRate", "oblq_lfoDepth", "oblq_reverbMix", "oblq_delayMix",
                   "oblq_character", "oblq_movement", "oblq_space", "oblq_coupling",
                   "oblq_skew"],
    },
    "ORBITAL": {
        "prefix": "orb_",
        "params": ["orb_orbitPeriod", "orb_eccentricity", "orb_filterCutoff", "orb_filterReso",
                   "orb_ampAttack", "orb_ampDecay", "orb_ampSustain", "orb_ampRelease",
                   "orb_lfoRate", "orb_lfoDepth", "orb_reverbMix", "orb_delayMix",
                   "orb_character", "orb_movement", "orb_space", "orb_coupling"],
    },
    "OCELOT": {
        "prefix": "ocel_",
        "params": ["ocel_huntMode", "ocel_filterCutoff", "ocel_filterReso",
                   "ocel_ampAttack", "ocel_ampDecay", "ocel_ampSustain", "ocel_ampRelease",
                   "ocel_lfoRate", "ocel_lfoDepth", "ocel_reverbMix", "ocel_delayMix",
                   "ocel_character", "ocel_movement", "ocel_space", "ocel_coupling",
                   "ocel_prowl"],
    },
    "OVERWORLD": {
        "prefix": "ow_",
        "params": ["ow_era", "ow_chipWave", "ow_filterCutoff", "ow_filterReso",
                   "ow_ampAttack", "ow_ampDecay", "ow_ampSustain", "ow_ampRelease",
                   "ow_lfoRate", "ow_lfoDepth", "ow_reverbMix", "ow_delayMix",
                   "ow_character", "ow_movement", "ow_space", "ow_coupling"],
    },
    "OHM": {
        "prefix": "ohm_",
        "params": ["ohm_commune", "ohm_meddling", "ohm_filterCutoff", "ohm_filterReso",
                   "ohm_ampAttack", "ohm_ampDecay", "ohm_ampSustain", "ohm_ampRelease",
                   "ohm_lfoRate", "ohm_lfoDepth", "ohm_reverbMix", "ohm_delayMix",
                   "ohm_character", "ohm_movement", "ohm_space", "ohm_coupling"],
    },
    "OBBLIGATO": {
        "prefix": "obbl_",
        "params": ["obbl_bond", "obbl_wind1", "obbl_wind2", "obbl_filterCutoff",
                   "obbl_ampAttack", "obbl_ampDecay", "obbl_ampSustain", "obbl_ampRelease",
                   "obbl_lfoRate", "obbl_lfoDepth", "obbl_reverbMix", "obbl_delayMix",
                   "obbl_character", "obbl_movement", "obbl_space", "obbl_coupling"],
    },
    "OMBRE": {
        "prefix": "ombr_",
        "params": ["ombr_fadeDepth", "ombr_filterCutoff", "ombr_filterReso",
                   "ombr_ampAttack", "ombr_ampDecay", "ombr_ampSustain", "ombr_ampRelease",
                   "ombr_lfoRate", "ombr_lfoDepth", "ombr_reverbMix", "ombr_delayMix",
                   "ombr_character", "ombr_movement", "ombr_space", "ombr_coupling"],
    },
    "OBSCURA": {
        "prefix": "obsc_",
        "params": ["obsc_veilDepth", "obsc_filterCutoff", "obsc_filterReso",
                   "obsc_ampAttack", "obsc_ampDecay", "obsc_ampSustain", "obsc_ampRelease",
                   "obsc_lfoRate", "obsc_lfoDepth", "obsc_reverbMix", "obsc_delayMix",
                   "obsc_character", "obsc_movement", "obsc_space", "obsc_coupling"],
    },
    "OCEANIC": {
        "prefix": "ocn_",
        "params": ["ocn_depthZone", "ocn_pressure", "ocn_filterCutoff", "ocn_filterReso",
                   "ocn_ampAttack", "ocn_ampDecay", "ocn_ampSustain", "ocn_ampRelease",
                   "ocn_lfoRate", "ocn_lfoDepth", "ocn_reverbMix", "ocn_delayMix",
                   "ocn_character", "ocn_movement", "ocn_space", "ocn_coupling"],
    },
    "OPAL": {
        "prefix": "opal_",
        "params": ["opal_grainSize", "opal_grainDensity", "opal_grainPitch", "opal_filterCutoff",
                   "opal_ampAttack", "opal_ampDecay", "opal_ampSustain", "opal_ampRelease",
                   "opal_lfoRate", "opal_lfoDepth", "opal_reverbMix", "opal_delayMix",
                   "opal_character", "opal_movement", "opal_space", "opal_coupling"],
    },
    "OSPREY": {
        "prefix": "osp_",
        "params": ["osp_diveSpeed", "osp_filterCutoff", "osp_filterReso",
                   "osp_ampAttack", "osp_ampDecay", "osp_ampSustain", "osp_ampRelease",
                   "osp_lfoRate", "osp_lfoDepth", "osp_reverbMix", "osp_delayMix",
                   "osp_character", "osp_movement", "osp_space", "osp_coupling"],
    },
    "OCTOPUS": {
        "prefix": "oct_",
        "params": ["oct_armCount", "oct_inkCloud", "oct_filterCutoff", "oct_filterReso",
                   "oct_ampAttack", "oct_ampDecay", "oct_ampSustain", "oct_ampRelease",
                   "oct_lfoRate", "oct_lfoDepth", "oct_reverbMix", "oct_delayMix",
                   "oct_character", "oct_movement", "oct_space", "oct_coupling"],
    },
    "ONSET": {
        "prefix": "ons_",
        "params": ["ons_machine", "ons_punch", "ons_space", "ons_mutate",
                   "ons_kickTune", "ons_snareTone", "ons_hihatDecay", "ons_filterCutoff",
                   "ons_reverbMix", "ons_delayMix", "ons_character", "ons_movement",
                   "ons_density", "ons_aggression", "ons_coupling", "ons_outputLevel"],
    },
}


# ---------------------------------------------------------------------------
# Preset definitions: (name, engine, dark/bright polarity, DNA overrides)
# ---------------------------------------------------------------------------

def r(lo, hi):
    """Random float in [lo, hi], 2 decimal places."""
    return round(random.uniform(lo, hi), 2)


def dark_brightness():
    return round(random.uniform(0.04, 0.12), 2)


def bright_brightness():
    return round(random.uniform(0.88, 0.98), 2)


def atm_dark_brightness():
    return round(random.uniform(0.04, 0.11), 2)


def atm_bright_brightness():
    return round(random.uniform(0.89, 0.99), 2)


def make_engine_params(engine_name):
    """Generate plausible parameter values for a given engine."""
    info = ENGINE_PARAMS.get(engine_name)
    if not info:
        return {}
    params = {}
    for p in info["params"]:
        # Envelope times get larger values, most params stay 0-1
        if any(x in p for x in ["Attack", "Decay", "Release"]):
            params[p] = round(random.uniform(5, 800), 1)
        elif any(x in p for x in ["wave", "Wave", "Mode", "mode", "legatoMode", "div", "Div"]):
            params[p] = random.choice([0, 1])
        elif "Sustain" in p:
            params[p] = r(0.3, 1.0)
        elif any(x in p for x in ["bodyFreq", "Freq"]):
            params[p] = round(random.uniform(20, 800), 1)
        else:
            params[p] = r(0.0, 1.0)
    return params


def preset(name, engine, brightness, mood, bright_zone, warmth=None, movement=None,
           density=None, space=None, aggression=None):
    """Build a complete .xometa preset dict."""
    w = warmth if warmth is not None else r(0.2, 0.9)
    mv = movement if movement is not None else r(0.1, 0.9)
    dn = density if density is not None else r(0.2, 0.95)
    sp = space if space is not None else r(0.1, 0.85)
    ag = aggression if aggression is not None else r(0.05, 0.9)

    polarity = "bright" if bright_zone else "dark"
    tags = [mood.lower(), polarity, "extreme-brightness"]

    dna = {
        "brightness": brightness,
        "warmth": w,
        "movement": mv,
        "density": dn,
        "space": sp,
        "aggression": ag,
    }

    return {
        "schema_version": 1,
        "name": name,
        "mood": mood,
        "engines": [engine],
        "author": "XO_OX",
        "version": "1.0",
        "description": f"{engine} — {polarity} extreme brightness preset for {mood}.",
        "tags": tags,
        "macroLabels": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
        "couplingIntensity": "None",
        "sonic_dna": dna,
        "dna": dna,
        "parameters": {engine: make_engine_params(engine)},
        "coupling": {"pairs": []},
    }


# ---------------------------------------------------------------------------
# Preset catalogue
# ---------------------------------------------------------------------------

FOUNDATION_DARK = [
    # OBLONG (2)
    ("Basalt Pressure", "OBLONG", dark_brightness, False, dict(warmth=r(0.6, 0.9), density=r(0.7, 0.95))),
    ("Iron Core Pulse", "OBLONG", dark_brightness, False, dict(warmth=r(0.5, 0.8), movement=r(0.3, 0.6))),
    # OBESE (2)
    ("Subterranean Mass", "OBESE", dark_brightness, False, dict(warmth=r(0.7, 0.95), density=r(0.8, 1.0))),
    ("Asphalt Drift", "OBESE", dark_brightness, False, dict(aggression=r(0.1, 0.4))),
    # OVERDUB (2)
    ("Tape Void", "OVERDUB", dark_brightness, False, dict(space=r(0.4, 0.7))),
    ("Dub Burial", "OVERDUB", dark_brightness, False, dict(warmth=r(0.6, 0.85), density=r(0.6, 0.85))),
    # OVERBITE (2)
    ("Cave Fang", "OVERBITE", dark_brightness, False, dict(aggression=r(0.5, 0.8))),
    ("Gnaw Silence", "OVERBITE", dark_brightness, False, dict(density=r(0.3, 0.6), movement=r(0.1, 0.35))),
    # ODDOSCAR (2)
    ("Oscar Abyss", "ODDOSCAR", dark_brightness, False, dict(warmth=r(0.4, 0.7))),
    ("Polar Marrow", "ODDOSCAR", dark_brightness, False, dict(space=r(0.2, 0.5))),
    # ORGANON (2)
    ("Reed Crypt", "ORGANON", dark_brightness, False, dict(warmth=r(0.55, 0.8))),
    ("Drawbar Depth", "ORGANON", dark_brightness, False, dict(density=r(0.6, 0.9))),
    # OBSIDIAN (2)
    ("Volcanic Glass", "OBSIDIAN", dark_brightness, False, dict(aggression=r(0.3, 0.6))),
    ("Magma Seam", "OBSIDIAN", dark_brightness, False, dict(warmth=r(0.7, 0.95), movement=r(0.15, 0.4))),
    # ORACLE (2)
    ("Blind Seer", "ORACLE", dark_brightness, False, dict(space=r(0.5, 0.8))),
    ("Prophecy Pit", "ORACLE", dark_brightness, False, dict(density=r(0.5, 0.75))),
    # ORIGAMI (2)
    ("Creased Shadow", "ORIGAMI", dark_brightness, False, dict(warmth=r(0.3, 0.6))),
    ("Folded Dark", "ORIGAMI", dark_brightness, False, dict(movement=r(0.2, 0.5))),
    # OUROBOROS (2)
    ("Serpent Loop", "OUROBOROS", dark_brightness, False, dict(density=r(0.7, 0.95), movement=r(0.4, 0.7))),
    ("Endless Descent", "OUROBOROS", dark_brightness, False, dict(space=r(0.3, 0.65))),
]

FOUNDATION_BRIGHT = [
    # ODDFELIX (2)
    ("Prism Felix", "ODDFELIX", bright_brightness, True, dict(warmth=r(0.1, 0.4))),
    ("Felix Radiance", "ODDFELIX", bright_brightness, True, dict(movement=r(0.6, 0.9))),
    # OPTIC (2)
    ("Spectral Bloom", "OPTIC", bright_brightness, True, dict(density=r(0.2, 0.5))),
    ("Light Prism", "OPTIC", bright_brightness, True, dict(space=r(0.5, 0.8))),
    # ORPHICA (2)
    ("Crystal Harp", "ORPHICA", bright_brightness, True, dict(movement=r(0.5, 0.8))),
    ("Siphon Shimmer", "ORPHICA", bright_brightness, True, dict(warmth=r(0.2, 0.5))),
    # OBLIQUE (2)
    ("Angled Light", "OBLIQUE", bright_brightness, True, dict(aggression=r(0.1, 0.4))),
    ("Refraction Edge", "OBLIQUE", bright_brightness, True, dict(density=r(0.15, 0.45))),
    # ORIGAMI (2) — also in bright list
    ("Paper Lantern", "ORIGAMI", bright_brightness, True, dict(space=r(0.4, 0.7))),
    ("Fold Flare", "ORIGAMI", bright_brightness, True, dict(movement=r(0.6, 0.85))),
    # ORBITAL (2)
    ("Solar Arc", "ORBITAL", bright_brightness, True, dict(movement=r(0.55, 0.85))),
    ("Perihelion Flash", "ORBITAL", bright_brightness, True, dict(density=r(0.2, 0.55))),
    # OCELOT (2)
    ("Spotted Glare", "OCELOT", bright_brightness, True, dict(aggression=r(0.4, 0.7))),
    ("Ocelot Streak", "OCELOT", bright_brightness, True, dict(movement=r(0.65, 0.9))),
    # OVERWORLD (2)
    ("Chip Sunrise", "OVERWORLD", bright_brightness, True, dict(warmth=r(0.15, 0.45))),
    ("NES Noon", "OVERWORLD", bright_brightness, True, dict(density=r(0.2, 0.5))),
    # OHM (2)
    ("Commune Gleam", "OHM", bright_brightness, True, dict(space=r(0.5, 0.8))),
    ("Hippy Sun", "OHM", bright_brightness, True, dict(warmth=r(0.3, 0.6))),
    # OBBLIGATO (2)
    ("Obliged Light", "OBBLIGATO", bright_brightness, True, dict(movement=r(0.5, 0.8))),
    ("Bond Radiance", "OBBLIGATO", bright_brightness, True, dict(density=r(0.15, 0.4))),
]

ATMOSPHERE_DARK = [
    # OBSIDIAN (2)
    ("Obsidian Veil", "OBSIDIAN", atm_dark_brightness, False, dict(space=r(0.55, 0.85))),
    ("Dark Glass Dome", "OBSIDIAN", atm_dark_brightness, False, dict(density=r(0.4, 0.7))),
    # OMBRE (2)
    ("Twilight Fade", "OMBRE", atm_dark_brightness, False, dict(movement=r(0.2, 0.5), warmth=r(0.4, 0.7))),
    ("Gradient Dusk", "OMBRE", atm_dark_brightness, False, dict(space=r(0.5, 0.8))),
    # OBSCURA (2)
    ("Occult Mist", "OBSCURA", atm_dark_brightness, False, dict(density=r(0.3, 0.6))),
    ("Shrouded Canal", "OBSCURA", atm_dark_brightness, False, dict(space=r(0.6, 0.9))),
    # OCEANIC (2)
    ("Bathypelagic Drift", "OCEANIC", atm_dark_brightness, False, dict(space=r(0.6, 0.9), warmth=r(0.5, 0.75))),
    ("Hadal Current", "OCEANIC", atm_dark_brightness, False, dict(density=r(0.5, 0.8))),
    # ODDOSCAR (2)
    ("Oscar Midnight", "ODDOSCAR", atm_dark_brightness, False, dict(movement=r(0.15, 0.4))),
    ("Oscar Still Water", "ODDOSCAR", atm_dark_brightness, False, dict(density=r(0.35, 0.65))),
    # ORACLE (2)
    ("Omen Atmosphere", "ORACLE", atm_dark_brightness, False, dict(space=r(0.55, 0.85))),
    ("Void Oracle", "ORACLE", atm_dark_brightness, False, dict(warmth=r(0.25, 0.55))),
    # OUROBOROS (2)
    ("Cyclic Dark", "OUROBOROS", atm_dark_brightness, False, dict(movement=r(0.35, 0.65))),
    ("Ouroboros Night", "OUROBOROS", atm_dark_brightness, False, dict(density=r(0.6, 0.85))),
    # OPAL (2)
    ("Opal Shadow", "OPAL", atm_dark_brightness, False, dict(space=r(0.5, 0.8))),
    ("Granular Dusk", "OPAL", atm_dark_brightness, False, dict(movement=r(0.2, 0.5))),
    # OSPREY (2)
    ("Nighthawk Glide", "OSPREY", atm_dark_brightness, False, dict(space=r(0.4, 0.7))),
    ("Osprey Nocturne", "OSPREY", atm_dark_brightness, False, dict(warmth=r(0.3, 0.6))),
    # ORACLE x2 (spec calls for 20 across 10 engines, Oracle appears twice in list)
    ("Sibyl Hollow", "ORACLE", atm_dark_brightness, False, dict(aggression=r(0.05, 0.3))),
    ("Oracle Depth", "ORACLE", atm_dark_brightness, False, dict(density=r(0.55, 0.8))),
]

ATMOSPHERE_BRIGHT = [
    # ODDFELIX (2)
    ("Felix Sky", "ODDFELIX", atm_bright_brightness, True, dict(density=r(0.1, 0.4))),
    ("Felix Apex", "ODDFELIX", atm_bright_brightness, True, dict(movement=r(0.65, 0.9))),
    # OPTIC (2)
    ("Optic Aurora", "OPTIC", atm_bright_brightness, True, dict(space=r(0.6, 0.9))),
    ("Refracted Sky", "OPTIC", atm_bright_brightness, True, dict(density=r(0.15, 0.45))),
    # ORPHICA (2)
    ("Orphica Cloud", "ORPHICA", atm_bright_brightness, True, dict(space=r(0.55, 0.85))),
    ("Microsound Halo", "ORPHICA", atm_bright_brightness, True, dict(density=r(0.15, 0.4))),
    # OBLIQUE (2)
    ("Slant Shine", "OBLIQUE", atm_bright_brightness, True, dict(movement=r(0.5, 0.8))),
    ("Oblique Zenith", "OBLIQUE", atm_bright_brightness, True, dict(warmth=r(0.15, 0.4))),
    # OCTOPUS (2)
    ("Biolume Bloom", "OCTOPUS", atm_bright_brightness, True, dict(movement=r(0.6, 0.9))),
    ("Eight Arm Gleam", "OCTOPUS", atm_bright_brightness, True, dict(space=r(0.5, 0.8))),
    # ORBITAL (2)
    ("Orbital Apex", "ORBITAL", atm_bright_brightness, True, dict(density=r(0.15, 0.45))),
    ("Aphelion Glow", "ORBITAL", atm_bright_brightness, True, dict(movement=r(0.6, 0.85))),
    # OVERWORLD (2)
    ("SNES Afternoon", "OVERWORLD", atm_bright_brightness, True, dict(warmth=r(0.2, 0.5))),
    ("Genesis Glare", "OVERWORLD", atm_bright_brightness, True, dict(movement=r(0.55, 0.8))),
    # ONSET (2)
    ("Machine Daylight", "ONSET", atm_bright_brightness, True, dict(aggression=r(0.4, 0.7))),
    ("Punch Horizon", "ONSET", atm_bright_brightness, True, dict(density=r(0.3, 0.6))),
    # OHM (2)
    ("Ohm Zenith", "OHM", atm_bright_brightness, True, dict(space=r(0.55, 0.85))),
    ("Commune Daybreak", "OHM", atm_bright_brightness, True, dict(warmth=r(0.2, 0.5))),
    # OBBLIGATO (2)
    ("Obbligato Dawn", "OBBLIGATO", atm_bright_brightness, True, dict(density=r(0.15, 0.45))),
    ("Wind Bond Shine", "OBBLIGATO", atm_bright_brightness, True, dict(movement=r(0.5, 0.8))),
]


# ---------------------------------------------------------------------------
# Generator
# ---------------------------------------------------------------------------

def safe_filename(name):
    return name.replace(" ", "_").replace("/", "-").replace("\\", "-") + ".xometa"


def write_preset(dest_dir, preset_name, engine, brightness_fn, is_bright, mood, extras):
    filename = safe_filename(preset_name)
    filepath = os.path.join(dest_dir, filename)
    if os.path.exists(filepath):
        print(f"  SKIP (exists): {filename}")
        return False

    b = brightness_fn()
    kw = {k: (v() if callable(v) else v) for k, v in extras.items()}
    data = preset(preset_name, engine, b, mood, is_bright, **kw)

    with open(filepath, "w") as f:
        json.dump(data, f, indent=2)
    print(f"  WRITE: {filename}  brightness={b:.2f}  engine={engine}")
    return True


def main():
    os.makedirs(FOUNDATION_DIR, exist_ok=True)
    os.makedirs(ATMOSPHERE_DIR, exist_ok=True)

    sections = [
        ("Foundation DARK", FOUNDATION_DIR, "Foundation", FOUNDATION_DARK),
        ("Foundation BRIGHT", FOUNDATION_DIR, "Foundation", FOUNDATION_BRIGHT),
        ("Atmosphere DARK", ATMOSPHERE_DIR, "Atmosphere", ATMOSPHERE_DARK),
        ("Atmosphere BRIGHT", ATMOSPHERE_DIR, "Atmosphere", ATMOSPHERE_BRIGHT),
    ]

    total_written = 0
    total_skipped = 0

    for label, dest, mood, catalogue in sections:
        print(f"\n=== {label} ({len(catalogue)} presets) ===")
        written = skipped = 0
        for entry in catalogue:
            name, engine, b_fn, is_bright, extras = entry
            ok = write_preset(dest, name, engine, b_fn, is_bright, mood, extras)
            if ok:
                written += 1
            else:
                skipped += 1
        print(f"  → {written} written, {skipped} skipped")
        total_written += written
        total_skipped += skipped

    print(f"\n{'='*50}")
    print(f"DONE — {total_written} presets written, {total_skipped} skipped")
    print(f"Foundation: {FOUNDATION_DIR}")
    print(f"Atmosphere: {ATMOSPHERE_DIR}")


if __name__ == "__main__":
    main()
