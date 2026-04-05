#!/usr/bin/env python3
"""
Guru Bin Library Transcendent — Batch Patch Script
Patches stub presets (zero macros + no LFO depth + no filter env) with
mood-appropriate expressive parameters.

Issues: #688 (macros), #690 (velocity/filterEnv), #696 (init-state audit)
"""

import json
import os
import hashlib
import sys
from collections import defaultdict

PRESET_DIR = os.path.join(os.path.dirname(__file__), '..', 'Presets', 'XOceanus')

# ─── Engine Parameter Atlas ───────────────────────────────────────────
# Discovered from well-formed presets in the library

ENGINE_FILTER_ENV = {
    "Oaken": "oaken_filterEnvAmount", "Oasis": "oas_filterEnvAmt",
    "Obbligato": "obbl_filterEnvAmt", "Obelisk": "obel_filterEnvAmount",
    "Obese": "fat_fltEnvAmt", "Obiont": "obnt_filterEnvAmt",
    "Oblique": "oblq_filterEnvAmt", "Oblong": "bob_fltEnvAmt",
    "Obrix": "obrix_fltEnvAmt", "Obscura": "obscura_filterEnvAmt",
    "Obsidian": "obsidian_filterEnvAmt", "OceanDeep": "deep_filterEnvAmt",
    "Oceanic": "ocean_filterEnvAmt", "Ocelot": "ocelot_filterEnvDepth",
    "Ochre": "ochre_filterEnvAmount", "Octave": "oct_filterEnvAmount",
    "Octopus": "octo_filterEnvAmt", "OddOscar": "morph_filterEnvDepth",
    "OddfeliX": "snap_filterEnvDepth", "Oddfellow": "oddf_filterEnvAmt",
    "Odyssey": "drift_filterEnvAmt", "Offering": "ofr_filterEnvAmt",
    "Ogre": "ogre_fltEnvAmt", "Ohm": "ohm_filterEnvAmt",
    "Okeanos": "okan_filterEnvAmt", "Olate": "olate_filterEnvAmount",
    "Ole": "ole_filterEnvAmt", "Oleg": "oleg_filterEnvAmt",
    "Ombre": "ombre_filterEnvAmt", "Omega": "omega_filterEnvAmount",
    "Onkolo": "onko_filterEnvAmt", "Onset": "perc_filterEnvAmt",
    "Opal": "opal_filterEnvAmt", "Opaline": "opal2_fltEnvAmt",
    "Opcode": "opco_filterEnvAmt", "OpenSky": "sky_filterEnvAmt",
    "Opera": "opera_filterEnvAmt", "Optic": "optic_filterEnvAmt",
    "Oracle": "oracle_filterEnvAmt", "Orbital": "orb_filterEnvAmt",
    "Orbweave": "weave_filterEnvAmt", "Orca": "orca_filterEnvAmt",
    "Orchard": "orch_filterEnvAmt", "Organism": "org_filterEnvAmt",
    "Organon": "organon_filterEnvAmt", "Origami": "origami_filterEnvAmt",
    "Orphica": "orph_filterEnvAmt", "Osier": "osier_filterEnvAmt",
    "Osmosis": "osmo_filterEnvAmt", "Osprey": "osprey_filterEnvDepth",
    "Osteria": "osteria_filterEnvDepth", "Ostinato": "osti_filterEnvAmt",
    "Otis": "otis_filterEnvAmount", "Oto": "oto_filterEnvAmt",
    "Ottoni": "otto_filterEnvAmt", "Ouie": "ouie_filterEnvAmt",
    "Ouroboros": "ouro_filterEnvAmt", "Outlook": "look_filterEnvAmt",
    "Outflow": "out_filterEnvAmt", "Outwit": "owit_filterEnvAmt",
    "Oven": "oven_filterEnvAmt", "Overbite": "poss_filterEnvAmount",
    "Overcast": "cast_filterEnvAmt", "Overdub": "dub_filterEnvAmt",
    "Overflow": "flow_filterEnvAmt", "Overgrow": "grow_filterEnvAmt",
    "Overlap": "olap_filterEnvAmt", "Overtone": "over_filterEnvAmt",
    "Overwash": "wash_filterEnvAmt", "Overworn": "worn_filterEnvAmt",
    "Overworld": "ow_filterEnvDepth", "Oware": "owr_fltEnvAmt",
    "Owlfish": "owl_filterEnvDepth", "Oxalis": "oxal_filterEnvAmt",
    "Oxbow": "oxb_filterEnvAmt", "Oxytocin": "oxy_filterEnvAmt",
}

# LFO params: (rate_key, depth_key) pairs — LFO1 only (minimum for D005)
# Some engines have unique naming
ENGINE_LFO = {
    "Oaken": [("oaken_lfo1Rate", "oaken_lfo1Depth")],
    "Oasis": [("oas_lfo1Rate", "oas_lfo1Depth")],
    "Obbligato": [("obbl_lfoRateA", "obbl_lfoDepthA")],
    "Obelisk": [("obel_lfo1Rate", "obel_lfo1Depth")],
    "Obese": [("fat_lfo1Rate", "fat_lfo1Depth")],
    "Obiont": [("obnt_lfo1Rate", "obnt_lfo1Depth")],
    "Oblique": [("oblq_lfo1Rate", "oblq_lfo1Depth")],
    "Oblong": [("bob_lfo1Rate", "bob_lfo1Depth")],
    "Obrix": [("obrix_lfo1Rate", "obrix_lfo1Depth")],
    "Obscura": [("obscura_lfo1Rate", "obscura_lfo1Depth")],
    "Obsidian": [("obsidian_lfoRate", "obsidian_lfoDepth")],
    "OceanDeep": [("deep_lfo1Rate", "deep_lfo1Depth")],
    "Oceanic": [("ocean_lfo1Rate", "ocean_lfo1Depth")],
    "Ocelot": [("ocelot_lfoRate", "ocelot_lfoRate")],  # only has rate — set rate as proxy
    "Ochre": [("ochre_lfo1Rate", "ochre_lfo1Depth")],
    "Octave": [("oct_lfo1Rate", "oct_lfo1Depth")],
    "Octopus": [("octo_lfo1Rate", "octo_lfo1Depth")],
    "OddOscar": [("morph_lfoRate", "morph_lfoRate")],  # only rate param
    "OddfeliX": [("snap_lfoRate", "snap_lfoDepth")],
    "Oddfellow": [("oddf_lfo1Rate", "oddf_lfo1Depth")],
    "Odyssey": [("drift_lfoRate", "drift_lfoDepth")],
    "Offering": [("ofr_lfo1Rate", "ofr_lfo1Depth")],
    "Ogre": [("ogre_lfo1Rate", "ogre_lfo1Depth")],
    "Ohm": [("ohm_lfo1Rate", "ohm_lfo1Depth")],
    "Okeanos": [("okan_lfo1Rate", "okan_lfo1Depth")],
    "Olate": [("olate_lfo1Rate", "olate_lfo1Depth")],
    "Ole": [("ole_lfoRate", "ole_lfoRate")],  # single param
    "Oleg": [("oleg_lfo1Rate", "oleg_lfo1Depth")],
    "Ombre": [("ombre_lfo1Rate", "ombre_lfo1Depth")],
    "Omega": [("omega_lfo2Rate", "omega_lfo2Depth")],  # only LFO2
    "Onkolo": [("onko_lfo1Rate", "onko_lfo1Depth")],
    "Onset": [("perc_lfo1Rate", "perc_lfo1Depth")],
    "Opal": [("opal_lfo1Rate", "opal_lfo1Depth")],
    "Opaline": [("opal2_lfo1Rate", "opal2_lfo1Depth")],
    "Opcode": [("opco_lfo1Rate", "opco_lfo1Depth")],
    "OpenSky": [("sky_lfo1Rate", "sky_lfo1Depth")],
    "Opera": [("opera_lfo1Rate", "opera_lfo1Depth")],
    "Optic": [("optic_lfoRate", "optic_lfoRate")],  # single param
    "Oracle": [("oracle_lfo1Rate", "oracle_lfo1Depth")],
    "Orbital": [("orb_lfo1Rate", "orb_lfo1Depth")],
    "Orbweave": [("weave_lfo1Rate", "weave_lfo1Depth")],
    "Orca": [("orca_lfo1Rate", "orca_lfo1Depth")],
    "Orchard": [("orch_lfo1Rate", "orch_lfo1Depth")],
    "Organism": [("org_lfo1Rate", "org_lfo1Depth")],
    "Organon": [("organon_lfo1Rate", "organon_lfo1Depth")],
    "Origami": [("origami_lfo1Rate", "origami_lfo1Depth")],
    "Orphica": [("orph_lfo1Rate", "orph_lfo1Depth")],
    "Osier": [("osier_lfo1Rate", "osier_lfo1Depth")],
    "Osmosis": [("osmo_lfo1Rate", "osmo_lfo1Depth")],
    "Osprey": [("osprey_lfo2Rate", "osprey_lfo2Depth")],  # LFO2 has depth
    "Osteria": [("osteria_lfoDepth", "osteria_lfoDepth")],  # single depth
    "Ostinato": [("osti_lfo1Rate", "osti_lfo1Depth")],
    "Otis": [("otis_lfo1Rate", "otis_lfo1Depth")],
    "Oto": [("oto_lfo1Rate", "oto_lfo1Depth")],
    "Ottoni": [("otto_lfoRate", "otto_lfoRate")],  # single param
    "Ouie": [("ouie_lfo1Rate", "ouie_lfo1Depth")],
    "Ouroboros": [("ouro_lfoRate", "ouro_lfoRate")],  # single param
    "Outlook": [("look_lfo1Rate", "look_lfo1Depth")],
    "Outflow": [("out_lfo1Rate", "out_lfo1Depth")],
    "Outwit": [("owit_lfo1Rate", "owit_lfo1Depth")],
    "Oven": [("oven_lfo1Rate", "oven_lfo1Depth")],
    "Overbite": [("poss_lfo1Rate", "poss_lfo1Depth")],
    "Overcast": [("cast_lfo1Rate", "cast_lfo1Depth")],
    "Overdub": [("dub_lfoDepth", "dub_lfoDepth")],  # single depth
    "Overflow": [("flow_lfo1Rate", "flow_lfo1Depth")],
    "Overgrow": [("grow_lfo1Rate", "grow_lfo1Depth")],
    "Overlap": [("olap_lfo1Rate", "olap_lfo1Depth")],
    "Overtone": [("over_lfo1Rate", "over_lfo1Depth")],
    "Overwash": [("wash_lfo1Rate", "wash_lfo1Depth")],
    "Overworn": [("worn_lfo1Rate", "worn_lfo1Depth")],
    "Overworld": [("ow_fmLfoRate", "ow_fmLfoDepth")],
    "Oware": [("owr_lfo1Rate", "owr_lfo1Depth")],
    "Owlfish": [("owl_lfoRate", "owl_lfoDepth")],
    "Oxalis": [("oxal_lfo1Rate", "oxal_lfo1Depth")],
    "Oxbow": [("oxb_lfo1Rate", "oxb_lfo1Depth")],
    "Oxytocin": [("oxy_lfo_rate", "oxy_lfo_depth")],
}

# Macro param names per engine (4 per engine, mapped to standard labels)
# Standard: CHARACTER, MOVEMENT, COUPLING, SPACE
ENGINE_MACROS = {
    "Oaken": ["oaken_macroCharacter", "oaken_macroMovement", "oaken_macroCoupling", "oaken_macroSpace"],
    "Oasis": ["oas_macroCharacter", "oas_macroMovement", "oas_macroCoupling", "oas_macroSpace"],
    "Obbligato": ["obbl_macroBond", "obbl_macroBreath", "obbl_macroWind", "obbl_macroMischief"],
    "Obelisk": ["obel_macroCharacter", "obel_macroMovement", "obel_macroCoupling", "obel_macroSpace"],
    "Obese": ["fat_macroMojo", "fat_macroGrit", "fat_macroSize", "fat_macroCrush"],
    "Obiont": ["obnt_macroCharacter", "obnt_macroMovement", "obnt_macroCoupling", "obnt_macroSpace"],
    "Oblique": ["oblq_macroCharacter", "oblq_macroMovement", "oblq_macroCoupling", "oblq_macroSpace"],
    "Oblong": ["bob_macroCharacter", "bob_macroMovement", "bob_macroCoupling", "bob_macroSpace"],
    "Obrix": ["obrix_macroCharacter", "obrix_macroMovement", "obrix_macroCoupling", "obrix_macroSpace"],
    "Obscura": ["obscura_macroCharacter", "obscura_macroMovement", "obscura_macroCoupling", "obscura_macroSpace"],
    "Obsidian": ["obsidian_macroCharacter", "obsidian_macroMovement", "obsidian_macroCoupling", "obsidian_macroSpace"],
    "OceanDeep": ["deep_macroPressure", "deep_macroCreature", "deep_macroWreck", "deep_macroAbyss"],
    "Oceanic": ["ocean_macroCharacter", "ocean_macroMovement", "ocean_macroCoupling", "ocean_macroSpace"],
    "Ocelot": ["ocelot_macroCharacter", "ocelot_macroMovement", "ocelot_macroCoupling", "ocelot_macroSpace"],
    "Ochre": ["ochre_macroCharacter", "ochre_macroMovement", "ochre_macroCoupling", "ochre_macroSpace"],
    "Octave": ["oct_macroCharacter", "oct_macroMovement", "oct_macroCoupling", "oct_macroSpace"],
    "Octopus": ["octo_macroCharacter", "octo_macroMovement", "octo_macroCoupling", "octo_macroSpace"],
    "OddOscar": ["morph_macroCharacter", "morph_macroMovement", "morph_macroCoupling", "morph_macroSpace"],
    "OddfeliX": ["snap_macroDart", "snap_macroDepth", "snap_macroSchool", "snap_macroSurface"],
    "Oddfellow": ["oddf_macroCharacter", "oddf_macroMovement", "oddf_macroCoupling", "oddf_macroSpace"],
    "Odyssey": ["drift_macroCharacter", "drift_macroMovement", "drift_macroCoupling", "drift_macroSpace"],
    "Offering": ["ofr_macroCharacter", "ofr_macroMovement", "ofr_macroCoupling", "ofr_macroSpace"],
    "Ogre": ["ogre_macroCharacter", "ogre_macroMovement", "ogre_macroCoupling", "ogre_macroSpace"],
    "Ohm": ["ohm_macroMeddling", "ohm_macroCommune", "ohm_macroJam", "ohm_macroMeadow"],
    "Okeanos": ["okan_macroCharacter", "okan_macroMovement", "okan_macroCoupling", "okan_macroSpace"],
    "Olate": ["olate_macroCharacter", "olate_macroMovement", "olate_macroCoupling", "olate_macroSpace"],
    "Ole": ["ole_macroDrama", "ole_macroFuego", "ole_macroIsla", "ole_macroSides"],
    "Oleg": ["oleg_macroCharacter", "oleg_macroMovement", "oleg_macroCoupling", "oleg_macroSpace"],
    "Ombre": ["ombre_macroCharacter", "ombre_macroMovement", "ombre_macroCoupling", "ombre_macroSpace"],
    "Omega": ["omega_macroCharacter", "omega_macroMovement", "omega_macroCoupling", "omega_macroSpace"],
    "Onkolo": ["onko_macroCharacter", "onko_macroMovement", "onko_macroCoupling", "onko_macroSpace"],
    "Onset": ["perc_macro_punch", "perc_macro_mutate", "perc_macro_machine", "perc_macro_space"],
    "Opal": ["opal_macroCharacter", "opal_macroMovement", "opal_macroCoupling", "opal_macroSpace"],
    "Opaline": ["opal2_macroCharacter", "opal2_macroMovement", "opal2_macroCoupling", "opal2_macroSpace"],
    "Opcode": ["opco_macroCharacter", "opco_macroMovement", "opco_macroCoupling", "opco_macroSpace"],
    "OpenSky": ["sky_macroCharacter", "sky_macroMovement", "sky_macroCoupling", "sky_macroSpace"],
    "Opera": ["opera_macroCharacter", "opera_macroMovement", "opera_macroCoupling", "opera_macroSpace"],
    "Optic": ["optic_macroCharacter", "optic_macroMovement", "optic_macroCoupling", "optic_macroSpace"],
    "Oracle": ["oracle_macroCharacter", "oracle_macroMovement", "oracle_macroCoupling", "oracle_macroSpace"],
    "Orbital": ["orb_macroSpectrum", "orb_macroEvolve", "orb_macroCoupling", "orb_macroSpace"],
    "Orbweave": ["weave_macroWeave", "weave_macroTension", "weave_macroKnot", "weave_macroSpace"],
    "Orca": ["orca_macroCharacter", "orca_macroMovement", "orca_macroCoupling", "orca_macroSpace"],
    "Orchard": ["orch_macroCharacter", "orch_macroMovement", "orch_macroCoupling", "orch_macroSpace"],
    "Organism": ["org_macroRule", "org_macroSeed", "org_macroCoupling", "org_macroMutate"],
    "Organon": ["organon_macroCharacter", "organon_macroMovement", "organon_macroCoupling", "organon_macroSpace"],
    "Origami": ["origami_macroCharacter", "origami_macroMovement", "origami_macroCoupling", "origami_macroSpace"],
    "Orphica": ["orph_macroPluck", "orph_macroSurface", "orph_macroFracture", "orph_macroDivine"],
    "Osier": ["osier_macroCharacter", "osier_macroMovement", "osier_macroCoupling", "osier_macroSpace"],
    "Osmosis": ["osmo_macro1", "osmo_macro2", "osmo_macro3", "osmo_macro4"],
    "Osprey": ["osprey_macroCharacter", "osprey_macroMovement", "osprey_macroCoupling", "osprey_macroSpace"],
    "Osteria": ["osteria_macroCharacter", "osteria_macroMovement", "osteria_macroCoupling", "osteria_macroSpace"],
    "Ostinato": ["osti_macroGather", "osti_macroFire", "osti_macroCircle", "osti_macroSpace"],
    "Otis": ["otis_macroCharacter", "otis_macroMovement", "otis_macroCoupling", "otis_macroSpace"],
    "Oto": ["oto_macroA", "oto_macroB", "oto_macroC", "oto_macroD"],
    "Ottoni": ["otto_macroCharacter", "otto_macroMovement", "otto_macroCoupling", "otto_macroSpace"],
    "Ouie": ["ouie_macroHammer", "ouie_macroCartilage", "ouie_macroAmpullae", "ouie_macroCurrent"],
    "Ouroboros": ["ouro_macroCharacter", "ouro_macroMovement", "ouro_macroCoupling", "ouro_macroSpace"],
    "Outlook": ["look_macroCharacter", "look_macroMovement", "look_macroCoupling", "look_macroSpace"],
    "Outflow": ["out_macroCharacter", "out_macroMovement", "out_macroCoupling", "out_macroSpace"],
    "Outwit": ["owit_macroCharacter", "owit_macroMovement", "owit_macroCoupling", "owit_macroSpace"],
    "Oven": ["oven_macroCharacter", "oven_macroMovement", "oven_macroCoupling", "oven_macroSpace"],
    "Overbite": ["poss_macroBelly", "poss_macroBite", "poss_macroPlayDead", "poss_macroScurry"],
    "Overcast": ["cast_macroCharacter", "cast_macroMovement", "cast_macroCoupling", "cast_macroSpace"],
    "Overdub": ["dub_macroCharacter", "dub_macroMovement", "dub_macroCoupling", "dub_macroSpace"],
    "Overflow": ["flow_macroCharacter", "flow_macroMovement", "flow_macroCoupling", "flow_macroSpace"],
    "Overgrow": ["grow_macroCharacter", "grow_macroMovement", "grow_macroCoupling", "grow_macroSpace"],
    "Overlap": ["olap_macroCharacter", "olap_macroMovement", "olap_macroCoupling", "olap_macroSpace"],
    "Overtone": ["over_macroDepth", "over_macroColor", "over_macroCoupling", "over_macroSpace"],
    "Overwash": ["wash_macroCharacter", "wash_macroMovement", "wash_macroCoupling", "wash_macroSpace"],
    "Overworn": ["worn_macroCharacter", "worn_macroMovement", "worn_macroCoupling", "worn_macroSpace"],
    "Overworld": ["ow_macroEra", "ow_macroCrush", "ow_macroGlitch", "ow_macroSpace"],
    "Oware": ["owr_macroMaterial", "owr_macroMallet", "owr_macroCoupling", "owr_macroSpace"],
    "Owlfish": ["owl_macroCharacter", "owl_macroMovement", "owl_macroCoupling", "owl_macroSpace"],
    "Oxalis": ["oxal_macroCharacter", "oxal_macroMovement", "oxal_macroCoupling", "oxal_macroSpace"],
    "Oxbow": ["oxb_macroCharacter", "oxb_macroMovement", "oxb_macroCoupling", "oxb_macroSpace"],
    "Oxytocin": ["oxy_macroCharacter", "oxy_macroMovement", "oxy_macroCoupling", "oxy_macroSpace"],
}


# ─── Mood Profiles ────────────────────────────────────────────────────
# (character_range, movement_range, coupling_range, space_range,
#  filter_env_range, lfo_rate_range, lfo_depth_range)

MOOD_PROFILES = {
    "Foundation":  ((0.40, 0.60), (0.25, 0.45), (0.10, 0.30), (0.25, 0.45), (0.35, 0.55), (0.08, 0.25), (0.15, 0.30)),
    "Atmosphere":  ((0.25, 0.45), (0.30, 0.50), (0.10, 0.25), (0.60, 0.80), (0.30, 0.50), (0.02, 0.12), (0.15, 0.35)),
    "Entangled":   ((0.45, 0.65), (0.40, 0.60), (0.50, 0.75), (0.35, 0.55), (0.40, 0.60), (0.10, 0.30), (0.20, 0.40)),
    "Prism":       ((0.50, 0.70), (0.40, 0.60), (0.15, 0.35), (0.35, 0.55), (0.40, 0.60), (0.10, 0.30), (0.20, 0.35)),
    "Flux":        ((0.60, 0.80), (0.65, 0.85), (0.15, 0.35), (0.25, 0.45), (0.50, 0.70), (0.20, 0.50), (0.25, 0.45)),
    "Aether":      ((0.30, 0.50), (0.35, 0.55), (0.10, 0.25), (0.70, 0.90), (0.30, 0.45), (0.01, 0.08), (0.10, 0.25)),
    "Family":      ((0.40, 0.60), (0.30, 0.50), (0.30, 0.55), (0.40, 0.60), (0.40, 0.55), (0.05, 0.20), (0.15, 0.30)),
    "Submerged":   ((0.35, 0.55), (0.25, 0.45), (0.15, 0.35), (0.55, 0.75), (0.35, 0.55), (0.02, 0.10), (0.15, 0.30)),
    "Coupling":    ((0.45, 0.65), (0.40, 0.60), (0.55, 0.80), (0.35, 0.55), (0.40, 0.60), (0.08, 0.25), (0.20, 0.40)),
    "Crystalline": ((0.50, 0.70), (0.30, 0.50), (0.10, 0.25), (0.45, 0.65), (0.45, 0.65), (0.05, 0.20), (0.15, 0.30)),
    "Deep":        ((0.40, 0.60), (0.20, 0.40), (0.10, 0.25), (0.50, 0.70), (0.35, 0.55), (0.01, 0.08), (0.10, 0.25)),
    "Ethereal":    ((0.30, 0.50), (0.30, 0.50), (0.10, 0.20), (0.65, 0.85), (0.25, 0.45), (0.01, 0.06), (0.10, 0.20)),
    "Kinetic":     ((0.55, 0.75), (0.65, 0.85), (0.15, 0.35), (0.25, 0.45), (0.50, 0.70), (0.25, 0.60), (0.25, 0.45)),
    "Luminous":    ((0.45, 0.65), (0.35, 0.55), (0.10, 0.25), (0.50, 0.70), (0.40, 0.60), (0.05, 0.15), (0.15, 0.30)),
    "Organic":     ((0.35, 0.55), (0.35, 0.55), (0.15, 0.35), (0.40, 0.60), (0.35, 0.55), (0.03, 0.15), (0.15, 0.35)),
}


def deterministic_rand(seed_str, index=0):
    """Deterministic float [0,1) from preset name + index."""
    h = hashlib.md5(f"{seed_str}_{index}".encode()).hexdigest()
    return int(h[:8], 16) / 0xFFFFFFFF


def lerp(lo, hi, t):
    return round(lo + (hi - lo) * t, 3)


def is_stub(data):
    """Check if preset needs patching: all macros zero + no LFO depth + no filter env."""
    macros = data.get('macros', {})
    macro_vals = [v for v in macros.values() if isinstance(v, (int, float))]
    all_macros_zero = all(v == 0 for v in macro_vals) if macro_vals else True

    params = data.get('parameters', {})
    has_lfo_depth = False
    has_filter_env = False

    for eng, ep in params.items():
        if not isinstance(ep, dict):
            continue
        for pk, pv in ep.items():
            pkl = pk.lower()
            if ('lfo' in pkl and 'depth' in pkl) and isinstance(pv, (int, float)) and pv > 0:
                has_lfo_depth = True
            if ('filterenv' in pkl or 'fltenv' in pkl) and isinstance(pv, (int, float)) and pv > 0:
                has_filter_env = True

    return all_macros_zero and not has_lfo_depth and not has_filter_env


def patch_preset(data, path):
    """Add macros, filter env, and LFO to a stub preset. Returns True if modified."""
    mood = data.get('mood', 'Foundation')
    profile = MOOD_PROFILES.get(mood, MOOD_PROFILES['Foundation'])
    char_r, move_r, coup_r, space_r, fenv_r, lfo_rate_r, lfo_depth_r = profile

    name = data.get('name', os.path.basename(path))
    engines = data.get('engines', [])
    has_coupling = bool(data.get('coupling', {}).get('pairs', []))

    # Generate deterministic varied values from preset name
    t_char = deterministic_rand(name, 0)
    t_move = deterministic_rand(name, 1)
    t_coup = deterministic_rand(name, 2)
    t_space = deterministic_rand(name, 3)
    t_fenv = deterministic_rand(name, 4)
    t_lfo_rate = deterministic_rand(name, 5)
    t_lfo_depth = deterministic_rand(name, 6)

    char_val = lerp(*char_r, t_char)
    move_val = lerp(*move_r, t_move)
    space_val = lerp(*space_r, t_space)
    fenv_val = lerp(*fenv_r, t_fenv)
    lfo_rate_val = lerp(*lfo_rate_r, t_lfo_rate)
    lfo_depth_val = lerp(*lfo_depth_r, t_lfo_depth)

    # Coupling macro: higher if preset actually has coupling pairs
    if has_coupling:
        coup_val = lerp(0.40, 0.75, t_coup)
    else:
        coup_val = lerp(*coup_r, t_coup)

    # Set top-level macros
    data['macros'] = {
        "CHARACTER": char_val,
        "MOVEMENT": move_val,
        "COUPLING": coup_val,
        "SPACE": space_val,
    }

    # Patch each engine's parameters
    if 'parameters' not in data:
        data['parameters'] = {}

    for eng in engines:
        if eng not in data['parameters']:
            data['parameters'][eng] = {}
        ep = data['parameters'][eng]

        # Per-engine macro params (slight variation per engine in multi-engine presets)
        if eng in ENGINE_MACROS:
            macro_params = ENGINE_MACROS[eng]
            eng_offset = hash(eng) % 100 / 1000.0  # ±0.05 variation
            ep[macro_params[0]] = min(1.0, max(0.0, round(char_val + eng_offset - 0.025, 3)))
            ep[macro_params[1]] = min(1.0, max(0.0, round(move_val + eng_offset - 0.025, 3)))
            ep[macro_params[2]] = min(1.0, max(0.0, round(coup_val + eng_offset - 0.025, 3)))
            ep[macro_params[3]] = min(1.0, max(0.0, round(space_val + eng_offset - 0.025, 3)))

        # Filter envelope
        if eng in ENGINE_FILTER_ENV:
            fenv_key = ENGINE_FILTER_ENV[eng]
            if fenv_key not in ep or ep.get(fenv_key, 0) == 0:
                ep[fenv_key] = fenv_val

        # LFO rate + depth
        if eng in ENGINE_LFO:
            for rate_key, depth_key in ENGINE_LFO[eng]:
                if rate_key not in ep or ep.get(rate_key, 0) == 0:
                    ep[rate_key] = lfo_rate_val
                if depth_key != rate_key:  # Skip if same key (single-param engines)
                    if depth_key not in ep or ep.get(depth_key, 0) == 0:
                        ep[depth_key] = lfo_depth_val

    return True


def main():
    dry_run = '--dry-run' in sys.argv
    verbose = '--verbose' in sys.argv or '-v' in sys.argv

    patched_by_engine = defaultdict(int)
    patched_by_mood = defaultdict(int)
    total_patched = 0
    errors = []

    for root, dirs, files in os.walk(PRESET_DIR):
        if '_quarantine' in root:
            continue
        for f in files:
            if not f.endswith('.xometa'):
                continue
            path = os.path.join(root, f)
            try:
                with open(path) as fh:
                    data = json.load(fh)
            except Exception as e:
                errors.append(f"{path}: {e}")
                continue

            if not is_stub(data):
                continue

            if patch_preset(data, path):
                total_patched += 1
                mood = data.get('mood', 'Unknown')
                patched_by_mood[mood] += 1
                for eng in data.get('engines', []):
                    patched_by_engine[eng] += 1

                if verbose:
                    print(f"  PATCH: {data.get('name', '?')} ({mood})")

                if not dry_run:
                    with open(path, 'w') as fh:
                        json.dump(data, fh, indent=2, ensure_ascii=False)
                        fh.write('\n')

    # Report
    print(f"\n{'DRY RUN — ' if dry_run else ''}Batch Patch Complete")
    print(f"{'='*50}")
    print(f"Total patched: {total_patched}")
    print(f"\nBy engine (top 20):")
    for eng, count in sorted(patched_by_engine.items(), key=lambda x: -x[1])[:20]:
        print(f"  {eng}: {count}")
    print(f"\nBy mood:")
    for mood, count in sorted(patched_by_mood.items(), key=lambda x: -x[1]):
        print(f"  {mood}: {count}")
    if errors:
        print(f"\nErrors ({len(errors)}):")
        for e in errors[:10]:
            print(f"  {e}")


if __name__ == '__main__':
    main()
