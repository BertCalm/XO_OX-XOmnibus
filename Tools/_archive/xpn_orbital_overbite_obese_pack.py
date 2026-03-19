#!/usr/bin/env python3
"""
xpn_orbital_overbite_obese_pack.py — XO_OX Designs

Deepens coupling coverage for ORBITAL, OVERBITE, and OBESE, three engines
that still have gap coverage in the Entangled preset library.

  ORBITAL  — Warm Red #FF6B6B  | orb_  prefix | B001 Group Envelope System
  OVERBITE — Fang White #F0EDE8 | poss_ prefix | B008 Five-Macro System
  OBESE    — Hot Pink #FF1493   | fat_  prefix | B015 Mojo Control (analog/digital axis)

Generates ~66 presets:
  -  6  three-way marquee presets  (ORBITAL × OVERBITE × OBESE)
  - 20  ORBITAL pair presets       (1 per partner engine)
  - 20  OVERBITE pair presets      (1 per partner engine)
  - 20  OBESE pair presets         (1 per partner engine)

Total: 66 presets (existing files are skipped).

Usage:
    python Tools/xpn_orbital_overbite_obese_pack.py
    python Tools/xpn_orbital_overbite_obese_pack.py --dry-run
    python Tools/xpn_orbital_overbite_obese_pack.py --seed 77
    python Tools/xpn_orbital_overbite_obese_pack.py --output-dir /tmp/test_out
"""

import argparse
import json
import random
from pathlib import Path

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

DATE = "2026-03-16"

# ---------------------------------------------------------------------------
# Sonic DNA baselines
# ---------------------------------------------------------------------------

DNA_BASELINES: dict[str, dict[str, float]] = {
    # Primary trio
    "ORBITAL":    dict(brightness=0.6,  warmth=0.6,  movement=0.55, density=0.65, space=0.5,  aggression=0.5),
    "OVERBITE":   dict(brightness=0.45, warmth=0.55, movement=0.5,  density=0.7,  space=0.35, aggression=0.65),
    "OBESE":      dict(brightness=0.5,  warmth=0.7,  movement=0.6,  density=0.8,  space=0.4,  aggression=0.7),
    # Partner engines
    "OPTIC":      dict(brightness=0.85, warmth=0.35, movement=0.75, density=0.4,  space=0.65, aggression=0.35),
    "OBLIQUE":    dict(brightness=0.7,  warmth=0.55, movement=0.7,  density=0.55, space=0.55, aggression=0.55),
    "OCELOT":     dict(brightness=0.65, warmth=0.6,  movement=0.6,  density=0.55, space=0.6,  aggression=0.5),
    "OSPREY":     dict(brightness=0.55, warmth=0.55, movement=0.6,  density=0.6,  space=0.65, aggression=0.4),
    "OSTERIA":    dict(brightness=0.4,  warmth=0.75, movement=0.45, density=0.7,  space=0.6,  aggression=0.45),
    "OCEANIC":    dict(brightness=0.55, warmth=0.65, movement=0.65, density=0.55, space=0.7,  aggression=0.35),
    "OWLFISH":    dict(brightness=0.5,  warmth=0.5,  movement=0.45, density=0.65, space=0.55, aggression=0.4),
    "OHM":        dict(brightness=0.45, warmth=0.75, movement=0.55, density=0.6,  space=0.7,  aggression=0.3),
    "ORPHICA":    dict(brightness=0.8,  warmth=0.5,  movement=0.7,  density=0.45, space=0.75, aggression=0.25),
    "OBBLIGATO":  dict(brightness=0.55, warmth=0.65, movement=0.6,  density=0.65, space=0.6,  aggression=0.45),
    "OTTONI":     dict(brightness=0.6,  warmth=0.6,  movement=0.5,  density=0.7,  space=0.55, aggression=0.6),
    "OLE":        dict(brightness=0.65, warmth=0.7,  movement=0.75, density=0.6,  space=0.6,  aggression=0.55),
    "OMBRE":      dict(brightness=0.5,  warmth=0.65, movement=0.5,  density=0.6,  space=0.6,  aggression=0.35),
    "ORCA":       dict(brightness=0.45, warmth=0.45, movement=0.65, density=0.75, space=0.5,  aggression=0.75),
    "OCTOPUS":    dict(brightness=0.6,  warmth=0.5,  movement=0.7,  density=0.65, space=0.55, aggression=0.65),
    "OVERLAP":    dict(brightness=0.55, warmth=0.5,  movement=0.7,  density=0.7,  space=0.65, aggression=0.45),
    "OUTWIT":     dict(brightness=0.55, warmth=0.45, movement=0.8,  density=0.7,  space=0.5,  aggression=0.7),
    "ORACLE":     dict(brightness=0.5,  warmth=0.4,  movement=0.6,  density=0.7,  space=0.7,  aggression=0.4),
    "ORGANON":    dict(brightness=0.55, warmth=0.6,  movement=0.7,  density=0.65, space=0.55, aggression=0.45),
    "OUROBOROS":  dict(brightness=0.5,  warmth=0.4,  movement=0.85, density=0.75, space=0.5,  aggression=0.8),
}

# ---------------------------------------------------------------------------
# Engine ID strings (used in preset "engines" arrays and "parameters" keys)
# ---------------------------------------------------------------------------

ENGINE_ID: dict[str, str] = {
    "ORBITAL":   "Orbital",
    "OVERBITE":  "Overbite",
    "OBESE":     "Obese",
    "OPTIC":     "Optic",
    "OBLIQUE":   "Oblique",
    "OCELOT":    "Ocelot",
    "OSPREY":    "Osprey",
    "OSTERIA":   "Osteria",
    "OCEANIC":   "Oceanic",
    "OWLFISH":   "Owlfish",
    "OHM":       "Ohm",
    "ORPHICA":   "Orphica",
    "OBBLIGATO": "Obbligato",
    "OTTONI":    "Ottoni",
    "OLE":       "Ole",
    "OMBRE":     "Ombre",
    "ORCA":      "Orca",
    "OCTOPUS":   "Octopus",
    "OVERLAP":   "Overlap",
    "OUTWIT":    "Outwit",
    "ORACLE":    "Oracle",
    "ORGANON":   "Organon",
    "OUROBOROS": "Ouroboros",
}

# ---------------------------------------------------------------------------
# Parameter prefixes (frozen — never change after release)
# ---------------------------------------------------------------------------

ENGINE_PREFIX: dict[str, str] = {
    "ORBITAL":   "orb_",
    "OVERBITE":  "poss_",
    "OBESE":     "fat_",
    "OPTIC":     "optic_",
    "OBLIQUE":   "oblq_",
    "OCELOT":    "ocelot_",
    "OSPREY":    "osprey_",
    "OSTERIA":   "osteria_",
    "OCEANIC":   "ocean_",
    "OWLFISH":   "owl_",
    "OHM":       "ohm_",
    "ORPHICA":   "orph_",
    "OBBLIGATO": "obbl_",
    "OTTONI":    "otto_",
    "OLE":       "ole_",
    "OMBRE":     "ombre_",
    "ORCA":      "orca_",
    "OCTOPUS":   "octo_",
    "OVERLAP":   "olap_",
    "OUTWIT":    "owit_",
    "ORACLE":    "oracle_",
    "ORGANON":   "organon_",
    "OUROBOROS": "ouro_",
}

# ---------------------------------------------------------------------------
# Coupling types supported by MegaCouplingMatrix
# ---------------------------------------------------------------------------

COUPLING_TYPES = [
    "timbral_blend",
    "rhythm_sync",
    "filter_modulation",
    "amplitude_sidechain",
    "pitch_follow",
    "envelope_share",
    "harmonic_lock",
    "spectral_cross",
]

COUPLING_INTENSITIES = ["Subtle", "Moderate", "Strong", "Extreme"]

# ---------------------------------------------------------------------------
# Name vocabularies
# ---------------------------------------------------------------------------

VOCAB_ORBITAL = [
    "Apogee", "Perihelion", "Ephemeris", "Nodal", "Parallax",
    "Transit", "Opposition", "Declination", "Arc", "Phase",
    "Harmonic", "Resonant", "Cycle", "Radial", "Elliptic",
    "Inclination", "Limb", "Syzygy", "Ascending", "Zenith",
]

VOCAB_OVERBITE = [
    "Fang", "Bite", "Gnash", "Grip", "Clench",
    "Snap", "Chew", "Latch", "Gnaw", "Lockjaw",
    "Snarl", "Chomp", "Jaw", "Reflex", "Maw",
    "Tusk", "Enamel", "Canine", "Incisor", "Marrow",
]

VOCAB_OBESE = [
    "Mojo", "Saturate", "Drive", "Crush", "Thick",
    "Warmth", "Grain", "Compress", "Clip", "Bulk",
    "Density", "Wool", "Velvet", "Analog Heat", "Overload",
    "Fat", "Swell", "Bloom", "Heft", "Girth",
]

PARTNER_VOCAB: dict[str, list[str]] = {
    "OPTIC":     ["Phosphor", "Pulse", "Spectrum", "Luminance", "Retina", "Scan", "Flicker", "Wavelength", "Photon", "Beam"],
    "OBLIQUE":   ["Prism", "Refract", "Slant", "Skew", "Glitch", "Vector", "Funk", "Bounce", "Off-Axis", "Kink"],
    "OCELOT":    ["Prowl", "Dappled", "Tawny", "Savanna", "Habitat", "Territorial", "Biome", "Stalk", "Pounce", "Range"],
    "OSPREY":    ["Shore", "Dive", "Tide", "Plunge", "Coastline", "Salt", "Current", "Soar", "Thermal", "Shoal"],
    "OSTERIA":   ["Wine", "Cellar", "Ferment", "Tannin", "Vintage", "Barrel", "Slow", "Hearth", "Tavern", "Cask"],
    "OCEANIC":   ["Bioluminescent", "Phosphorescent", "Thermal", "Column", "Current", "Pelagic", "Abyssal", "Surge", "Bloom", "Drift"],
    "OWLFISH":   ["Abyssal", "Lantern", "Trautonium", "Mixtur", "Gold", "Depth", "Lure", "Whisker", "Night", "Still"],
    "OHM":       ["Commune", "Hum", "Mantric", "Sage", "Still", "Drift", "Mellow", "Pastoral", "Warm", "Hippy"],
    "ORPHICA":   ["Pluck", "Gossamer", "Filament", "Lyre", "Siren", "Harp", "Vibrato", "Microsound", "Shimmer", "Aether"],
    "OBBLIGATO": ["Bond", "Breath", "Wind", "Weave", "Interlock", "Oblige", "Duet", "Voice", "Braid", "Thread"],
    "OTTONI":    ["Brass", "Horn", "Fanfare", "Patina", "Bell", "Mouthpiece", "Valved", "Flare", "Embouchure", "Resonance"],
    "OLE":       ["Drama", "Flare", "Sway", "Flamencoid", "Surge", "Fiesta", "Spin", "Pulse", "Carnival", "Hibiscus"],
    "OMBRE":     ["Shadow", "Mauve", "Twilight", "Fade", "Gradient", "Perception", "Blur", "Penumbra", "Mist", "Veil"],
    "ORCA":      ["Hunt", "Breach", "Echo-Locate", "Pod", "Deep", "Apex", "Sonar", "Fjord", "Current", "Fluke"],
    "OCTOPUS":   ["Ink", "Arm", "Chromatophore", "Mantle", "Suction", "Camouflage", "Neural", "Alien", "Tentacle", "Adapt"],
    "OVERLAP":   ["Knot", "Topology", "FDN", "Tangle", "Weave", "Lattice", "Mesh", "Resonator", "Dense", "Bloom"],
    "OUTWIT":    ["Wolfram", "Cellular", "Automaton", "Arm", "Strategy", "Pattern", "Evolve", "Compute", "Adapt", "Rule"],
    "ORACLE":    ["Prophecy", "Stochastic", "Maqam", "Augury", "Omen", "Foresight", "Signal", "Void", "Breakpoint", "Spectral"],
    "ORGANON":   ["Metabolism", "Cell", "Enzyme", "Membrane", "Catalyst", "Living", "Tissue", "Organic", "Variational", "Pulse"],
    "OUROBOROS": ["Serpent", "Loop", "Feed", "Chaos", "Gnaw", "Recursive", "Devour", "Spiral", "Infinite", "Gnaw"],
}

# ---------------------------------------------------------------------------
# Three-way marquee preset definitions (hand-named)
# ---------------------------------------------------------------------------

MARQUEE_3WAY: list[tuple[str, str, list[str], str]] = [
    (
        "Warm Red Bite Crush",
        "A three-body entanglement — ORBITAL's group envelope kindles OVERBITE's fang, "
        "OBESE saturates the exchange. Dense, characterful, unstoppable.",
        ["timbral_blend", "amplitude_sidechain"],
    ),
    (
        "Mojo Gnash Transit",
        "OBESE Mojo macro drives OVERBITE into lockjaw while ORBITAL's arc decay "
        "shapes the release. Analog heat with a bite.",
        ["filter_modulation", "envelope_share"],
    ),
    (
        "Fang Apogee Bloom",
        "OVERBITE's fang attack modulates ORBITAL's phase spread; OBESE bloom fills "
        "the tail. A predatory orbital bloom.",
        ["harmonic_lock", "spectral_cross"],
    ),
    (
        "Velvet Jaw Phase",
        "OBESE velvet compression wraps ORBITAL's harmonic resonance; OVERBITE clench "
        "punctuates every cycle. Plush ferocity.",
        ["rhythm_sync", "timbral_blend"],
    ),
    (
        "Perihelion Snap Drive",
        "ORBITAL perihelion triggers OVERBITE snap; OBESE drive saturates the "
        "moment of closest approach. Dangerous warmth.",
        ["pitch_follow", "amplitude_sidechain"],
    ),
    (
        "The Ravenous Triad",
        "Full bidirectional entanglement across all three. ORBITAL sustains the orbit, "
        "OVERBITE gnaws its edges, OBESE crushes the debris into heat.",
        ["spectral_cross", "harmonic_lock"],
    ),
]

# ---------------------------------------------------------------------------
# Partner engine list (20 partners, same for all three primaries)
# ---------------------------------------------------------------------------

PARTNER_ENGINES: list[str] = [
    "OPTIC", "OBLIQUE", "OCELOT", "OSPREY", "OSTERIA",
    "OCEANIC", "OWLFISH", "OHM", "ORPHICA", "OBBLIGATO",
    "OTTONI", "OLE", "OMBRE", "ORCA", "OCTOPUS",
    "OVERLAP", "OUTWIT", "ORACLE", "ORGANON", "OUROBOROS",
]

assert len(PARTNER_ENGINES) == 20, "Partner list must be exactly 20 engines"

# ---------------------------------------------------------------------------
# DNA helpers
# ---------------------------------------------------------------------------

def blend_dna(*engine_names: str, rng: random.Random) -> dict[str, float]:
    """Average Sonic DNA of N engines with a small random nudge."""
    baselines = [DNA_BASELINES[e] for e in engine_names]
    keys = list(baselines[0].keys())
    result: dict[str, float] = {}
    for k in keys:
        mid = sum(b[k] for b in baselines) / len(baselines)
        nudge = rng.uniform(-0.04, 0.04)
        result[k] = round(max(0.0, min(1.0, mid + nudge)), 3)
    return result


# ---------------------------------------------------------------------------
# Parameter stub builder
# ---------------------------------------------------------------------------

def make_engine_params(engine: str, rng: random.Random) -> dict[str, float]:
    """Generate minimal stub parameters for any engine using its canonical prefix."""
    prefix = ENGINE_PREFIX[engine]
    params: dict[str, float | int] = {
        f"{prefix}macro1":      round(rng.uniform(0.3, 0.7), 3),
        f"{prefix}macro2":      round(rng.uniform(0.3, 0.7), 3),
        f"{prefix}macro3":      round(rng.uniform(0.3, 0.7), 3),
        f"{prefix}macro4":      round(rng.uniform(0.3, 0.7), 3),
        f"{prefix}couplingOut": round(rng.uniform(0.4, 0.8), 3),
        f"{prefix}couplingIn":  round(rng.uniform(0.4, 0.8), 3),
        f"{prefix}outputLevel": round(rng.uniform(0.75, 0.92), 3),
        f"{prefix}outputPan":   round(rng.uniform(-0.04, 0.04), 3),
    }
    # Engine-specific signature parameters
    if engine == "ORBITAL":
        params["orb_groupEnv"]     = round(rng.uniform(0.3, 0.85), 3)
        params["orb_phaseSpread"]  = round(rng.uniform(0.2, 0.8),  3)
        params["orb_arcDecay"]     = round(rng.uniform(0.3, 0.9),  3)
        params["orb_harmonicAmt"]  = round(rng.uniform(0.3, 0.75), 3)
        params["orb_resonantPeak"] = round(rng.uniform(0.2, 0.7),  3)
    elif engine == "OVERBITE":
        params["poss_biteDepth"]   = round(rng.uniform(0.4, 0.9),  3)
        params["poss_fangAttack"]  = round(rng.uniform(0.05, 0.4), 3)
        params["poss_gnashAmt"]    = round(rng.uniform(0.3, 0.8),  3)
        params["poss_snapRelease"] = round(rng.uniform(0.2, 0.7),  3)
    elif engine == "OBESE":
        params["fat_satDrive"]     = round(rng.uniform(0.4, 0.9),  3)
        params["fat_mojoCtrl"]     = round(rng.uniform(0.3, 0.85), 3)
        params["fat_analog"]       = round(rng.uniform(0.0, 1.0),  3)
        params["fat_digital"]      = round(rng.uniform(0.0, 1.0),  3)
        params["fat_compress"]     = round(rng.uniform(0.3, 0.75), 3)
    return params


# ---------------------------------------------------------------------------
# Preset builder
# ---------------------------------------------------------------------------

def make_preset(
    name: str,
    desc: str,
    primary: str,
    partners: list[str],
    coupling_types_used: list[str],
    rng: random.Random,
) -> dict:
    all_engines = [primary] + partners
    dna = blend_dna(*all_engines, rng=rng)

    # Parameters block keyed by Engine ID
    params: dict[str, dict] = {}
    for e in all_engines:
        params[ENGINE_ID[e]] = make_engine_params(e, rng)

    # Coupling pairs
    coupling_pairs = []
    primary_id = ENGINE_ID[primary]
    for i, partner in enumerate(partners):
        ctype = coupling_types_used[i % len(coupling_types_used)]
        coupling_pairs.append({
            "engineA": primary_id,
            "engineB": ENGINE_ID[partner],
            "type": ctype,
            "amount": round(rng.uniform(0.45, 0.88), 3),
        })
    # 3-way: add a partner↔partner link
    if len(partners) == 2:
        coupling_pairs.append({
            "engineA": ENGINE_ID[partners[0]],
            "engineB": ENGINE_ID[partners[1]],
            "type": rng.choice(COUPLING_TYPES),
            "amount": round(rng.uniform(0.35, 0.70), 3),
        })

    tags = [e.lower() for e in all_engines] + ["coupling", "entangled"]
    intensity = rng.choice(COUPLING_INTENSITIES)
    tempo = rng.choice([90, 95, 100, 105, 110, 115, 120, 125, 130])

    return {
        "schema_version": 1,
        "name": name,
        "mood": "Entangled",
        "engines": [ENGINE_ID[e] for e in all_engines],
        "author": "XO_OX",
        "version": "1.0.0",
        "description": desc,
        "tags": tags,
        "macroLabels": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
        "couplingIntensity": intensity,
        "tempo": tempo,
        "created": DATE,
        "legacy": {
            "sourceInstrument": None,
            "sourceCategory": None,
            "sourcePresetName": None,
        },
        "parameters": params,
        "coupling": {"pairs": coupling_pairs},
        "sequencer": None,
        "dna": dna,
    }


# ---------------------------------------------------------------------------
# Name generation
# ---------------------------------------------------------------------------

def unique_name(
    vocab_primary: list[str],
    vocab_partner: list[str],
    used: set[str],
    rng: random.Random,
) -> str:
    """Pick an unused two-word name from the cross-product of two vocab lists."""
    attempts = 0
    while attempts < 400:
        w1 = rng.choice(vocab_primary)
        w2 = rng.choice(vocab_partner)
        candidate = f"{w1} {w2}"
        if candidate not in used:
            used.add(candidate)
            return candidate
        attempts += 1
    # Fallback with numeric suffix
    suffix = rng.randint(100, 999)
    name = f"{rng.choice(vocab_primary)} {rng.choice(vocab_partner)} {suffix}"
    used.add(name)
    return name


# ---------------------------------------------------------------------------
# Primary vocab map
# ---------------------------------------------------------------------------

PRIMARY_VOCAB: dict[str, list[str]] = {
    "ORBITAL":  VOCAB_ORBITAL,
    "OVERBITE": VOCAB_OVERBITE,
    "OBESE":    VOCAB_OBESE,
}

# ---------------------------------------------------------------------------
# Evocative descriptions for primary × partner pairs
# ---------------------------------------------------------------------------

PAIR_DESC: dict[tuple[str, str], str] = {
    # ORBITAL pairs
    ("ORBITAL", "OPTIC"):     "ORBITAL group envelope modulates OPTIC photon scan rate. Light orbits its own source.",
    ("ORBITAL", "OBLIQUE"):   "ORBITAL arc decay bends OBLIQUE's prism angle. Off-axis harmonic trajectories.",
    ("ORBITAL", "OCELOT"):    "ORBITAL phase spread tracks OCELOT territorial biome. A warm predatory cycle.",
    ("ORBITAL", "OSPREY"):    "ORBITAL perihelion triggers OSPREY's dive vector. Tidal arc, coastal descent.",
    ("ORBITAL", "OSTERIA"):   "ORBITAL resonance ferments inside OSTERIA cask. Slow vintage orbit.",
    ("ORBITAL", "OCEANIC"):   "ORBITAL group envelope rides OCEANIC thermal column. Bioluminescent sweep.",
    ("ORBITAL", "OWLFISH"):   "ORBITAL Mixtur harmonic locks to OWLFISH lantern depth. Abyssal overtone arc.",
    ("ORBITAL", "OHM"):       "ORBITAL cycle breathes through OHM commune warmth. Pastoral resonance drift.",
    ("ORBITAL", "ORPHICA"):   "ORBITAL phase spread plucks ORPHICA gossamer filaments. Aerial microsound arc.",
    ("ORBITAL", "OBBLIGATO"): "ORBITAL harmonic arc obliges OBBLIGATO breath weave. Bonded trajectory.",
    ("ORBITAL", "OTTONI"):    "ORBITAL resonant peak excites OTTONI brass flare. Fanfare from elliptic focus.",
    ("ORBITAL", "OLE"):       "ORBITAL transit sparks OLELÉ drama. Flamencoid orbit, hibiscus arc.",
    ("ORBITAL", "OMBRE"):     "ORBITAL arc fades through OMBRE gradient. Penumbral orbital decay.",
    ("ORBITAL", "ORCA"):      "ORBITAL cycle drives ORCA hunt sonar. Apex predator on an elliptic path.",
    ("ORBITAL", "OCTOPUS"):   "ORBITAL phase spread activates OCTOPUS chromatophore arm depth. Radial alien cycle.",
    ("ORBITAL", "OVERLAP"):   "ORBITAL group envelope feeds OVERLAP FDN knot topology. Dense resonant mesh.",
    ("ORBITAL", "OUTWIT"):    "ORBITAL harmonic cycle seeds OUTWIT Wolfram rule evolution. Pattern emerges.",
    ("ORBITAL", "ORACLE"):    "ORBITAL ephemeris modulates ORACLE breakpoint stochastic grid. Foreseen arc.",
    ("ORBITAL", "ORGANON"):   "ORBITAL metabolic rate couples to ORGANON variational free energy. Living orbit.",
    ("ORBITAL", "OUROBOROS"): "ORBITAL cycle feeds OUROBOROS infinite loop. The orbit devours itself.",
    # OVERBITE pairs
    ("OVERBITE", "OPTIC"):    "OVERBITE fang attack gates OPTIC phosphor scan. Bite-timed light pulse.",
    ("OVERBITE", "OBLIQUE"):  "OVERBITE clench bends OBLIQUE prism color. Bitten geometry, refracted gnash.",
    ("OVERBITE", "OCELOT"):   "OVERBITE snap aligns with OCELOT pounce velocity. Savanna predator duet.",
    ("OVERBITE", "OSPREY"):   "OVERBITE jaw locks OSPREY dive thermal. Bass-forward coastal impact.",
    ("OVERBITE", "OSTERIA"):  "OVERBITE bite at the OSTERIA table. Slow feast, fat bass, gnashed vintage.",
    ("OVERBITE", "OCEANIC"):  "OVERBITE fang punctures OCEANIC bioluminescent column. Abyssal bite.",
    ("OVERBITE", "OWLFISH"):  "OVERBITE latch onto OWLFISH Mixtur overtone. Deep bass pressure.",
    ("OVERBITE", "OHM"):      "OVERBITE bass warmth into OHM commune. Grounded drone, tooth on sage.",
    ("OVERBITE", "ORPHICA"):  "OVERBITE canine clips ORPHICA lyre filament. Severed gossamer resonance.",
    ("OVERBITE", "OBBLIGATO"):"OVERBITE snap obliges OBBLIGATO interlock. Mandatory bite, woven breath.",
    ("OVERBITE", "OTTONI"):   "OVERBITE maw feeds OTTONI brass embouchure. Fang-shaped fanfare.",
    ("OVERBITE", "OLE"):      "OVERBITE reflex strikes OLELÉ drama pulse. Bitten flamencoid surge.",
    ("OVERBITE", "OMBRE"):    "OVERBITE bite fades through OMBRE shadow gradient. Penumbral gnash.",
    ("OVERBITE", "ORCA"):     "OVERBITE jaw syncs to ORCA hunt sonar. Two apex predators, one signal.",
    ("OVERBITE", "OCTOPUS"):  "OVERBITE tusk activates OCTOPUS ink cloud defense. Reflex arc entanglement.",
    ("OVERBITE", "OVERLAP"):  "OVERBITE gnash seeded into OVERLAP FDN topology. Bitten resonator knot.",
    ("OVERBITE", "OUTWIT"):   "OVERBITE snap triggers OUTWIT cellular rule-change. Strategy via reflex.",
    ("OVERBITE", "ORACLE"):   "OVERBITE lockjaw decodes ORACLE augury. Bass prophecy, gnashed stochastic.",
    ("OVERBITE", "ORGANON"):  "OVERBITE gnaw feeds ORGANON metabolic enzyme. Living bite, organic density.",
    ("OVERBITE", "OUROBOROS"):"OVERBITE recursive gnaw meets OUROBOROS self-loop. Infinite bite feedback.",
    # OBESE pairs
    ("OBESE", "OPTIC"):       "OBESE saturation clips OPTIC photon luminance. Overdriven light scan.",
    ("OBESE", "OBLIQUE"):     "OBESE mojo warmth into OBLIQUE prism refraction. Off-axis saturated bounce.",
    ("OBESE", "OCELOT"):      "OBESE analog heat envelops OCELOT tawny habitat. Warm predatory density.",
    ("OBESE", "OSPREY"):      "OBESE compress drives OSPREY shore plunge. Saturated coastal impact.",
    ("OBESE", "OSTERIA"):     "OBESE woolly warmth ferments in OSTERIA cask. Dense slow vintage heat.",
    ("OBESE", "OCEANIC"):     "OBESE saturation into OCEANIC thermal bloom. Warm bioluminescent density.",
    ("OBESE", "OWLFISH"):     "OBESE drive warms OWLFISH abyssal lantern. Fat depth, gold light.",
    ("OBESE", "OHM"):         "OBESE drive softened by OHM commune warmth. Hot hippie, sage and girth.",
    ("OBESE", "ORPHICA"):     "OBESE saturation into ORPHICA gossamer filament. Fat microsound paradox.",
    ("OBESE", "OBBLIGATO"):   "OBESE bulk obliges OBBLIGATO breath weave. Saturated duet, heavy bond.",
    ("OBESE", "OTTONI"):      "OBESE saturation into OTTONI brass flare. Thick fanfare, overloaded.",
    ("OBESE", "OLE"):         "OBESE mojo fuels OLELÉ drama surge. Fat flamencoid heat.",
    ("OBESE", "OMBRE"):       "OBESE velvet density through OMBRE gradient. Saturated twilight fade.",
    ("OBESE", "ORCA"):        "OBESE crush amplitude into ORCA sonar hunt. Heavy apex impact.",
    ("OBESE", "OCTOPUS"):     "OBESE analog heat warms OCTOPUS chromatophore arm adaptations. Fat alien.",
    ("OBESE", "OVERLAP"):     "OBESE saturation feeds OVERLAP FDN resonator mesh. Dense entangled bloom.",
    ("OBESE", "OUTWIT"):      "OBESE mojo drives OUTWIT Wolfram arm evolution. Saturated cellular pattern.",
    ("OBESE", "ORACLE"):      "OBESE mojo reveals ORACLE stochastic vision. Saturated prophecy density.",
    ("OBESE", "ORGANON"):     "OBESE analog heat catalyzes ORGANON variational metabolism. Living saturation.",
    ("OBESE", "OUROBOROS"):   "OBESE feedback loop entangles OUROBOROS infinite recursion. Eternal saturation.",
}


# ---------------------------------------------------------------------------
# Generate all presets
# ---------------------------------------------------------------------------

def generate_all(rng: random.Random) -> list[dict]:
    presets: list[dict] = []
    used_names: set[str] = set()

    # ---- 3-way marquee (6 presets) ----
    for name, desc, ctypes in MARQUEE_3WAY:
        used_names.add(name)
        preset = make_preset(name, desc, "ORBITAL", ["OVERBITE", "OBESE"], ctypes, rng)
        presets.append(preset)

    # ---- Per-primary × partner single-pair presets (20 per primary) ----
    for primary in ("ORBITAL", "OVERBITE", "OBESE"):
        pvocab = PRIMARY_VOCAB[primary]
        for partner in PARTNER_ENGINES:
            svocab = PARTNER_VOCAB[partner]
            name = unique_name(pvocab, svocab, used_names, rng)
            desc = PAIR_DESC.get((primary, partner), f"{primary} × {partner} coupling. Stub preset.")
            ctypes = [rng.choice(COUPLING_TYPES), rng.choice(COUPLING_TYPES)]
            preset = make_preset(name, desc, primary, [partner], ctypes, rng)
            presets.append(preset)

    return presets


# ---------------------------------------------------------------------------
# I/O helpers
# ---------------------------------------------------------------------------

def safe_filename(name: str) -> str:
    return name.replace(" ", "_").replace("/", "-").replace("×", "x") + ".xometa"


def write_preset(preset: dict, output_dir: Path) -> tuple[Path, bool]:
    """Write preset to file; return (path, was_skipped)."""
    output_dir.mkdir(parents=True, exist_ok=True)
    path = output_dir / safe_filename(preset["name"])
    if path.exists():
        return path, True
    with open(path, "w", encoding="utf-8") as fh:
        json.dump(preset, fh, indent=2)
        fh.write("\n")
    return path, False


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Generate ORBITAL + OVERBITE + OBESE coupling presets for XOmnibus. "
            "Writes ~66 .xometa files to Presets/XOmnibus/Entangled/ (skips existing)."
        )
    )
    repo_root = Path(__file__).resolve().parent.parent
    default_out = repo_root / "Presets" / "XOmnibus" / "Entangled"
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=default_out,
        help=f"Directory to write .xometa files (default: {default_out})",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print preset names without writing files",
    )
    parser.add_argument(
        "--seed",
        type=int,
        default=77,
        help="Random seed for reproducibility (default: 77)",
    )
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    rng = random.Random(args.seed)

    presets = generate_all(rng)

    marquee_count = len(MARQUEE_3WAY)
    pair_count = len(PARTNER_ENGINES) * 3   # ORBITAL + OVERBITE + OBESE
    total_expected = marquee_count + pair_count

    print("ORBITAL × OVERBITE × OBESE Coupling Pack")
    print(f"  3-way marquee presets : {marquee_count}")
    print(f"  ORBITAL pair presets  : {len(PARTNER_ENGINES)}")
    print(f"  OVERBITE pair presets : {len(PARTNER_ENGINES)}")
    print(f"  OBESE pair presets    : {len(PARTNER_ENGINES)}")
    print(f"  Total expected        : {total_expected}")
    print(f"  Generated             : {len(presets)}")
    print(f"  Output dir            : {args.output_dir}")
    print()

    if args.dry_run:
        for p in presets:
            engines = " × ".join(p["engines"])
            print(f"  [dry-run]  {p['name']}  ({engines})")
        print(f"\n[dry-run] {len(presets)} presets — nothing written.")
        return

    written: list[Path] = []
    skipped: list[Path] = []
    for p in presets:
        path, was_skipped = write_preset(p, args.output_dir)
        if was_skipped:
            skipped.append(path)
        else:
            written.append(path)
            print(f"  wrote   {path.name}")

    if skipped:
        print(f"\n  skipped {len(skipped)} existing file(s).")

    print(f"\nDone. {len(written)} preset(s) written to: {args.output_dir}")


if __name__ == "__main__":
    main()
