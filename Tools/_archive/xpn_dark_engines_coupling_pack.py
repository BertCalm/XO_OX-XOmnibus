#!/usr/bin/env python3
"""Generate Entangled coupling preset stubs for the four "dark character" engines:

  OBESE      — Hot Pink #FF1493, fat_ prefix, B015 Mojo Control, analog/digital axis
  OVERBITE   — Fang White #F0EDE8, poss_ prefix, B008 Five-Macro System
  OBSIDIAN   — Crystal White #E8E0D8, obsidian_ prefix, crystalline resonator
  OUROBOROS  — Strange Attractor Red #FF2D2D, ouro_ prefix, B003 Leash, B007 Velocity Coupling

Generates ~56 presets:
  4  four-way marquee presets (OBESE + OVERBITE + OBSIDIAN + OUROBOROS)
  12 intra-group pair presets (all 6 pairs × 2 presets each)
  16 OBESE × external partners (1 preset each)
  16 OVERBITE × external partners (1 preset each)
  4  OBSIDIAN × ODDFELIX/ODDOSCAR/OVERDUB/ODYSSEY
  4  OUROBOROS × ODDFELIX/ODDOSCAR/OVERDUB/ODYSSEY

All presets written to Presets/XOmnibus/Entangled/. Skips existing files.

Usage:
    python3 Tools/xpn_dark_engines_coupling_pack.py
    python3 Tools/xpn_dark_engines_coupling_pack.py --dry-run
    python3 Tools/xpn_dark_engines_coupling_pack.py --seed 99 --output-dir /tmp/test
"""

import argparse
import json
import random
from pathlib import Path

# ---------------------------------------------------------------------------
# Sonic DNA baselines
# ---------------------------------------------------------------------------

DNA = {
    # Dark four
    "OBESE":     {"brightness": 0.50, "warmth": 0.70, "movement": 0.60, "density": 0.80, "space": 0.40, "aggression": 0.70},
    "OVERBITE":  {"brightness": 0.45, "warmth": 0.55, "movement": 0.50, "density": 0.70, "space": 0.35, "aggression": 0.65},
    "OBSIDIAN":  {"brightness": 0.85, "warmth": 0.20, "movement": 0.30, "density": 0.50, "space": 0.60, "aggression": 0.15},
    "OUROBOROS": {"brightness": 0.50, "warmth": 0.40, "movement": 0.85, "density": 0.70, "space": 0.40, "aggression": 0.75},
    # External partners
    "ONSET":     {"brightness": 0.55, "warmth": 0.50, "movement": 0.80, "density": 0.75, "space": 0.50, "aggression": 0.70},
    "ORACLE":    {"brightness": 0.50, "warmth": 0.40, "movement": 0.60, "density": 0.70, "space": 0.70, "aggression": 0.40},
    "ORGANON":   {"brightness": 0.55, "warmth": 0.50, "movement": 0.65, "density": 0.65, "space": 0.60, "aggression": 0.45},
    "OCEANIC":   {"brightness": 0.60, "warmth": 0.55, "movement": 0.70, "density": 0.60, "space": 0.65, "aggression": 0.40},
    "OCELOT":    {"brightness": 0.65, "warmth": 0.60, "movement": 0.75, "density": 0.55, "space": 0.50, "aggression": 0.60},
    "OPTIC":     {"brightness": 0.90, "warmth": 0.30, "movement": 0.80, "density": 0.40, "space": 0.55, "aggression": 0.35},
    "OBLIQUE":   {"brightness": 0.75, "warmth": 0.40, "movement": 0.70, "density": 0.50, "space": 0.60, "aggression": 0.55},
    "OSPREY":    {"brightness": 0.55, "warmth": 0.60, "movement": 0.55, "density": 0.65, "space": 0.60, "aggression": 0.50},
    "OSTERIA":   {"brightness": 0.40, "warmth": 0.75, "movement": 0.45, "density": 0.70, "space": 0.60, "aggression": 0.45},
    "OWLFISH":   {"brightness": 0.60, "warmth": 0.55, "movement": 0.50, "density": 0.65, "space": 0.55, "aggression": 0.50},
    "OHM":       {"brightness": 0.45, "warmth": 0.75, "movement": 0.55, "density": 0.60, "space": 0.70, "aggression": 0.30},
    "ORPHICA":   {"brightness": 0.80, "warmth": 0.50, "movement": 0.70, "density": 0.45, "space": 0.75, "aggression": 0.25},
    "OTTONI":    {"brightness": 0.60, "warmth": 0.60, "movement": 0.50, "density": 0.70, "space": 0.55, "aggression": 0.60},
    "OLE":       {"brightness": 0.65, "warmth": 0.70, "movement": 0.75, "density": 0.60, "space": 0.60, "aggression": 0.55},
    "OMBRE":     {"brightness": 0.55, "warmth": 0.50, "movement": 0.45, "density": 0.55, "space": 0.65, "aggression": 0.35},
    "ORCA":      {"brightness": 0.45, "warmth": 0.35, "movement": 0.70, "density": 0.75, "space": 0.45, "aggression": 0.80},
    # OBSIDIAN/OUROBOROS extended partners
    "ODDFELIX":  {"brightness": 0.70, "warmth": 0.55, "movement": 0.55, "density": 0.55, "space": 0.55, "aggression": 0.45},
    "ODDOSCAR":  {"brightness": 0.55, "warmth": 0.65, "movement": 0.60, "density": 0.55, "space": 0.60, "aggression": 0.40},
    "OVERDUB":   {"brightness": 0.35, "warmth": 0.75, "movement": 0.60, "density": 0.55, "space": 0.75, "aggression": 0.25},
    "ODYSSEY":   {"brightness": 0.50, "warmth": 0.55, "movement": 0.65, "density": 0.50, "space": 0.65, "aggression": 0.30},
}

# ---------------------------------------------------------------------------
# Engine key (used in "engines" arrays and coupling blocks)
# ---------------------------------------------------------------------------

ENGINE_KEY = {
    "OBESE":     "XObese",
    "OVERBITE":  "XOverbite",
    "OBSIDIAN":  "XObsidian",
    "OUROBOROS": "XOuroboros",
    "ONSET":     "XOnset",
    "ORACLE":    "XOracle",
    "ORGANON":   "XOrganon",
    "OCEANIC":   "XOceanic",
    "OCELOT":    "XOcelot",
    "OPTIC":     "XOptic",
    "OBLIQUE":   "XOblique",
    "OSPREY":    "XOsprey",
    "OSTERIA":   "XOsteria",
    "OWLFISH":   "XOwlfish",
    "OHM":       "XOhm",
    "ORPHICA":   "XOrphica",
    "OTTONI":    "XOttoni",
    "OLE":       "XOlé",
    "OMBRE":     "XOmbre",
    "ORCA":      "XOrca",
    "ODDFELIX":  "OddfeliX",
    "ODDOSCAR":  "OddOscar",
    "OVERDUB":   "XOverdub",
    "ODYSSEY":   "XOdyssey",
}

# ---------------------------------------------------------------------------
# Parameter prefixes
# ---------------------------------------------------------------------------

PREFIX = {
    "OBESE":     "fat_",
    "OVERBITE":  "poss_",
    "OBSIDIAN":  "obsidian_",
    "OUROBOROS": "ouro_",
    "ONSET":     "perc_",
    "ORACLE":    "oracle_",
    "ORGANON":   "organon_",
    "OCEANIC":   "ocean_",
    "OCELOT":    "ocelot_",
    "OPTIC":     "optic_",
    "OBLIQUE":   "oblq_",
    "OSPREY":    "osprey_",
    "OSTERIA":   "osteria_",
    "OWLFISH":   "owl_",
    "OHM":       "ohm_",
    "ORPHICA":   "orph_",
    "OTTONI":    "otto_",
    "OLE":       "ole_",
    "OMBRE":     "ombre_",
    "ORCA":      "orca_",
    "ODDFELIX":  "snap_",
    "ODDOSCAR":  "morph_",
    "OVERDUB":   "dub_",
    "ODYSSEY":   "drift_",
}

# ---------------------------------------------------------------------------
# Coupling types available in MegaCouplingMatrix
# ---------------------------------------------------------------------------

COUPLING_TYPES = [
    "Amp->Filter",
    "Env->Morph",
    "LFO->Pitch",
    "Rhythm->Blend",
    "Audio->Wavetable",
    "Filter->Drive",
    "Amp->Amp",
    "Velocity->Blend",
    "Chaos->Mod",
]

# ---------------------------------------------------------------------------
# Name vocabularies — dark, aggressive, evocative
# ---------------------------------------------------------------------------

DARK_VOCAB = [
    "Abyssal", "Venom", "Viscera", "Wraith", "Corrosive", "Scar", "Marrow",
    "Hemorrhage", "Obsidian Clad", "Hellmouth", "Bleed Out", "Hollow Bone",
    "Undertow", "Necrotic", "Rot Signal", "Black Mold", "Iron Ruin",
    "Predator Pulse", "Ash Collapse", "Void Gnash", "Carrion Loop",
    "Leash Break", "Fracture Bloom", "Tectonic Bite", "Pressure Scar",
    "Hungry Dark", "Feral Lattice", "Smoked Mirror", "Crude Recursion",
    "Infernal Coat", "Drag Current", "Dark Feast", "Howl Compress",
    "Rancid Shimmer", "Brutalist Bloom", "Cloven Signal", "Wound Carrier",
    "Caustic Weave", "Tar Harmonic", "Blunt Ritual", "Corroded Halo",
    "Static Marrow", "Diesel Oracle", "Teeth of Glass", "Soot Angel",
    "Rusted Prophecy", "Sulfur Bloom", "Burnt Recursion", "Razor Commune",
    "Fossil Drive", "Cruel Resonance", "Grave Pulse", "Charred Cathedral",
    "Serrated Drift", "Hexagonal Rot", "Smear Lattice", "Cursed Warmth",
]

# Four-way marquee names
MARQUEE_NAMES = [
    "The Dark Conclave",
    "Hellgate Convergence",
    "Abyssal Entanglement",
    "Corrosive Tetrad",
]

# Intra-group pair names — keyed by sorted engine tuple
INTRA_PAIR_NAMES = {
    ("OBESE",     "OVERBITE"):  ["Mojo Fang", "Saturated Bite"],
    ("OBESE",     "OBSIDIAN"):  ["Hot Crystal", "Pink Glass Fracture"],
    ("OBESE",     "OUROBOROS"): ["Mojo Loop", "Fat Attractor"],
    ("OVERBITE",  "OBSIDIAN"):  ["Glass Tooth", "Crystalline Gnash"],
    ("OVERBITE",  "OUROBOROS"): ["Fang Chaos", "Leash Bite"],
    ("OBSIDIAN",  "OUROBOROS"): ["Crystal Attractor", "Strange Glass"],
}

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def blend_dna(dna_a: dict, dna_b: dict, rng: random.Random, weight: float = 0.5) -> dict:
    keys = ["brightness", "warmth", "movement", "density", "space", "aggression"]
    result = {}
    for k in keys:
        base = dna_a[k] * (1 - weight) + dna_b[k] * weight
        nudge = rng.uniform(-0.04, 0.04)
        result[k] = round(min(1.0, max(0.0, base + nudge)), 3)
    return result


def blend_dna_multi(engines: list, rng: random.Random) -> dict:
    """Blend DNA from a list of engine names with equal weight + slight nudge."""
    keys = ["brightness", "warmth", "movement", "density", "space", "aggression"]
    result = {}
    n = len(engines)
    for k in keys:
        avg = sum(DNA[e][k] for e in engines) / n
        nudge = rng.uniform(-0.04, 0.04)
        result[k] = round(min(1.0, max(0.0, avg + nudge)), 3)
    return result


def coupling_intensity(amount: float) -> str:
    if amount >= 0.75:
        return "Deep"
    if amount >= 0.5:
        return "Moderate"
    return "Light"


def stub_params(engine_name: str, dna: dict, rng: random.Random) -> dict:
    p = PREFIX[engine_name]
    return {
        f"{p}outputLevel":    round(rng.uniform(0.75, 0.90), 2),
        f"{p}outputPan":      round(rng.uniform(-0.05, 0.05), 2),
        f"{p}couplingLevel":  round(rng.uniform(0.55, 0.85), 2),
        f"{p}couplingBus":    0,
        f"{p}brightness":     dna["brightness"],
        f"{p}warmth":         dna["warmth"],
        f"{p}aggression":     dna["aggression"],
    }


def make_pair_preset(
    name: str,
    desc: str,
    tags: list,
    engine_a: str,
    engine_b: str,
    coupling_type: str,
    coupling_amount: float,
    dna: dict,
    rng: random.Random,
) -> dict:
    key_a = ENGINE_KEY[engine_a]
    key_b = ENGINE_KEY[engine_b]
    dna_a = blend_dna(DNA[engine_a], dna, rng, 0.35)
    dna_b = blend_dna(DNA[engine_b], dna, rng, 0.35)
    return {
        "schema_version": 1,
        "name": name,
        "mood": "Entangled",
        "sonic_dna": dna,
        "engines": [key_a, key_b],
        "author": "XO_OX",
        "version": "1.0.0",
        "description": desc,
        "tags": ["coupling", "entangled", "dark"] + tags,
        "macroLabels": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
        "couplingIntensity": coupling_intensity(coupling_amount),
        "dna": dna,
        "parameters": {
            key_a: stub_params(engine_a, dna_a, rng),
            key_b: stub_params(engine_b, dna_b, rng),
        },
        "coupling": {
            "type": coupling_type,
            "amount": round(coupling_amount, 2),
            "pairs": [
                {
                    "engineA": key_a,
                    "engineB": key_b,
                    "type": coupling_type,
                    "amount": round(coupling_amount, 2),
                }
            ],
        },
    }


def make_quad_preset(
    name: str,
    desc: str,
    tags: list,
    engines: list,   # 4 engine names
    primary_couple: tuple,
    coupling_type: str,
    coupling_amount: float,
    dna: dict,
    rng: random.Random,
) -> dict:
    keys = [ENGINE_KEY[e] for e in engines]
    params = {}
    for e in engines:
        edna = blend_dna(DNA[e], dna, rng, 0.3)
        params[ENGINE_KEY[e]] = stub_params(e, edna, rng)

    ea, eb = primary_couple
    key_a = ENGINE_KEY[ea]
    key_b = ENGINE_KEY[eb]

    return {
        "schema_version": 1,
        "name": name,
        "mood": "Entangled",
        "sonic_dna": dna,
        "engines": keys,
        "author": "XO_OX",
        "version": "1.0.0",
        "description": desc,
        "tags": ["coupling", "entangled", "dark", "quad"] + tags,
        "macroLabels": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
        "couplingIntensity": coupling_intensity(coupling_amount),
        "dna": dna,
        "parameters": params,
        "coupling": {
            "type": coupling_type,
            "amount": round(coupling_amount, 2),
            "pairs": [
                {"engineA": key_a, "engineB": key_b, "type": coupling_type, "amount": round(coupling_amount, 2)},
                {"engineA": ENGINE_KEY["OBSIDIAN"], "engineB": ENGINE_KEY["OUROBOROS"], "type": "Chaos->Mod", "amount": round(rng.uniform(0.6, 0.85), 2)},
            ],
        },
    }


# ---------------------------------------------------------------------------
# Section builders
# ---------------------------------------------------------------------------

DARK_FOUR = ["OBESE", "OVERBITE", "OBSIDIAN", "OUROBOROS"]

def build_marquee_presets(rng: random.Random) -> list:
    """4 four-way presets featuring all dark engines simultaneously."""
    configs = [
        (
            MARQUEE_NAMES[0],
            "All four dark engines locked: OBESE warmth, OVERBITE bite, OBSIDIAN crystal clarity, OUROBOROS infinite recursion. Full-spectrum horror.",
            ["conclave", "marquee"],
            ("OBESE", "OVERBITE"),
            "Amp->Filter", 0.85,
        ),
        (
            MARQUEE_NAMES[1],
            "OUROBOROS leash controls OBESE drive. OBSIDIAN lattice filters OVERBITE fang. Two axes of dark modulation colliding.",
            ["hellgate", "marquee"],
            ("OUROBOROS", "OBESE"),
            "Chaos->Mod", 0.80,
        ),
        (
            MARQUEE_NAMES[2],
            "OBSIDIAN crystalline resonance cracks under OVERBITE pressure. OBESE drives both into feedback. Eternal dark bloom.",
            ["abyssal", "marquee"],
            ("OBSIDIAN", "OVERBITE"),
            "Filter->Drive", 0.78,
        ),
        (
            MARQUEE_NAMES[3],
            "OUROBOROS velocity coupling feeds OBSIDIAN. OVERBITE gnash drives OBESE saturation. Maximum dark character density.",
            ["corrosive", "marquee", "velocity"],
            ("OUROBOROS", "OBSIDIAN"),
            "Velocity->Blend", 0.82,
        ),
    ]
    presets = []
    for name, desc, tags, primary, ctype, camount in configs:
        dna = blend_dna_multi(DARK_FOUR, rng)
        presets.append(make_quad_preset(name, desc, tags, DARK_FOUR, primary, ctype, camount, dna, rng))
    return presets


def build_intra_group_pairs(rng: random.Random) -> list:
    """2 presets for each of the 6 intra-group pairs = 12 presets."""
    import itertools
    presets = []
    for a, b in itertools.combinations(DARK_FOUR, 2):
        key = (a, b)
        names = INTRA_PAIR_NAMES[key]
        descs = [
            f"{a} and {b} locked in dark coupling. Mutual aggression, no escape.",
            f"Deep {a}×{b} entanglement — one engine feeds the other's worst impulse.",
        ]
        ctypes_used = []
        for i in range(2):
            available = [c for c in COUPLING_TYPES if c not in ctypes_used] or COUPLING_TYPES
            ctype = rng.choice(available)
            ctypes_used.append(ctype)
            camount = round(rng.uniform(0.60, 0.85), 2)
            dna = blend_dna(DNA[a], DNA[b], rng, 0.5)
            tags = [a.lower(), b.lower()]
            presets.append(make_pair_preset(names[i], descs[i], tags, a, b, ctype, camount, dna, rng))
    return presets


# External partners for OBESE
OBESE_EXTERNAL = [
    "ONSET", "ORACLE", "ORGANON", "OCEANIC", "OCELOT",
    "OPTIC", "OBLIQUE", "OSPREY", "OSTERIA", "OWLFISH",
    "OHM", "ORPHICA", "OTTONI", "OLE", "OMBRE", "ORCA",
]

# Evocative descriptions: OBESE × each external
OBESE_DESCS = {
    "ONSET":    "OBESE saturation crushes ONSET transients into concrete. Fat percussion.",
    "ORACLE":   "OBESE mojo clouds ORACLE prophecy — revelation through analog heat.",
    "ORGANON":  "OBESE drive feeds ORGANON metabolism. Fat energy for living synthesis.",
    "OCEANIC":  "OBESE warmth bleeds into OCEANIC phosphorescence. Hot deep water.",
    "OCELOT":   "OBESE analog grain textures OCELOT biome. Thick predator coat.",
    "OPTIC":    "OBESE saturation overdrives OPTIC pulse rate. Visual burn.",
    "OBLIQUE":  "OBESE mojo bends OBLIQUE angle past parallel. Off-axis fat.",
    "OSPREY":   "OBESE warmth floods OSPREY shore channel. Fat coastal signal.",
    "OSTERIA":  "OBESE saturation deepens OSTERIA vintage. Porto wine with heat.",
    "OWLFISH":  "OBESE drive locks OWLFISH Mixtur-Trautonium into harmonic overload.",
    "OHM":      "OBESE analog heat warms OHM commune beyond comfort. Hot hippie.",
    "ORPHICA":  "OBESE crush darkens ORPHICA microsound harp. Heavy pluck.",
    "OTTONI":   "OBESE saturation drives OTTONI brass into clip. Fat fanfare.",
    "OLE":      "OBESE mojo ignites OLE drama macro. Saturated Latin fire.",
    "OMBRE":    "OBESE density blurs OMBRE's perception gradient. Heavy memory.",
    "ORCA":     "OBESE compression matches ORCA apex predator. Two hungers, one output.",
}

# External partners for OVERBITE
OVERBITE_EXTERNAL = [
    "ONSET", "ORACLE", "ORGANON", "OCEANIC", "OCELOT",
    "OPTIC", "OBLIQUE", "OSPREY", "OSTERIA", "OWLFISH",
    "OHM", "ORPHICA", "OTTONI", "OLE", "OMBRE", "ORCA",
]

OVERBITE_DESCS = {
    "ONSET":    "OVERBITE snap triggers ONSET voice envelopes. Bass-forward percussion.",
    "ORACLE":   "OVERBITE fang punctures ORACLE's maqam scale. Bitten prophecy.",
    "ORGANON":  "OVERBITE bite drives ORGANON free energy. Predatory metabolism.",
    "OCEANIC":  "OVERBITE grip locks OCEANIC chromatophore flutter. Bass-deep water.",
    "OCELOT":   "OVERBITE gnash tracks OCELOT hunting cycle. Bitten tawny.",
    "OPTIC":    "OVERBITE snap modulates OPTIC visual pulse. Rhythmic visual bite.",
    "OBLIQUE":  "OVERBITE clench skews OBLIQUE prism angle. Gnashed geometry.",
    "OSPREY":   "OVERBITE bass thickens OSPREY shore blend. Grounded coastal character.",
    "OSTERIA":  "OVERBITE bite at the OSTERIA table. Bass-forward slow feast.",
    "OWLFISH":  "OVERBITE fang bends OWLFISH subharmonic. Bitten abyssal gold.",
    "OHM":      "OVERBITE bite interrupts OHM commune — grounded by gnash.",
    "ORPHICA":  "OVERBITE snap disrupts ORPHICA microsound cloud. Bass and glass.",
    "OTTONI":   "OVERBITE fang bends OTTONI brass transient. Bitten fanfare.",
    "OLE":      "OVERBITE bass drives OLE's DRAMA macro beyond comfort.",
    "OMBRE":    "OVERBITE grip marks OMBRE shadow gradient. Dark memory bite.",
    "ORCA":     "OVERBITE fang meets ORCA hunt macro. Two apex predators.",
}

# OBSIDIAN extended partners
OBSIDIAN_EXTRA = ["ODDFELIX", "ODDOSCAR", "OVERDUB", "ODYSSEY"]
OBSIDIAN_DESCS = {
    "ODDFELIX": "OBSIDIAN crystal lattice refracts ODDFELIX neon tetra signal. Cold colour.",
    "ODDOSCAR": "OBSIDIAN angular glass shapes ODDOSCAR gill flutter. Crystal axolotl.",
    "OVERDUB":  "OBSIDIAN sharp transients cut through OVERDUB tape warmth. Ice on tape.",
    "ODYSSEY":  "OBSIDIAN resonant clarity freezes ODYSSEY drift trajectory. Crystal voyage.",
}

# OUROBOROS extended partners
OUROBOROS_EXTRA = ["ODDFELIX", "ODDOSCAR", "OVERDUB", "ODYSSEY"]
OUROBOROS_DESCS = {
    "ODDFELIX": "OUROBOROS velocity coupling reshapes ODDFELIX filter sweep. Strange tetra.",
    "ODDOSCAR": "OUROBOROS leash bends ODDOSCAR morph axis. Recursive axolotl.",
    "OVERDUB":  "OUROBOROS loop feeds OVERDUB tape echo. Infinite dub recursion.",
    "ODYSSEY":  "OUROBOROS attractor locks ODYSSEY LFO phase. Captured voyage.",
}


def pick_name(rng: random.Random, used: set) -> str:
    pool = [n for n in DARK_VOCAB if n not in used]
    if not pool:
        pool = DARK_VOCAB
    name = rng.choice(pool)
    used.add(name)
    return name


def build_external_presets(
    anchor: str,
    partners: list,
    descs: dict,
    rng: random.Random,
    used_names: set,
) -> list:
    """1 preset per partner pair for an anchor engine vs. external list."""
    presets = []
    ctypes_used = []
    for partner in partners:
        available = [c for c in COUPLING_TYPES if c not in ctypes_used] or COUPLING_TYPES
        ctype = rng.choice(available)
        ctypes_used.append(ctype)
        camount = round(rng.uniform(0.55, 0.85), 2)
        dna = blend_dna(DNA[anchor], DNA[partner], rng, rng.uniform(0.4, 0.6))
        name = pick_name(rng, used_names)
        desc = descs.get(partner, f"{anchor} dark coupling with {partner}.")
        tags = [anchor.lower(), partner.lower()]
        presets.append(make_pair_preset(name, desc, tags, anchor, partner, ctype, camount, dna, rng))
    return presets


def build_all(rng: random.Random) -> list:
    used_names: set = set()

    # Seed used_names with marquee + intra names
    for n in MARQUEE_NAMES:
        used_names.add(n)
    for names in INTRA_PAIR_NAMES.values():
        for n in names:
            used_names.add(n)

    presets = []
    presets += build_marquee_presets(rng)             # 4
    presets += build_intra_group_pairs(rng)           # 12
    presets += build_external_presets("OBESE",     OBESE_EXTERNAL,    OBESE_DESCS,    rng, used_names)  # 16
    presets += build_external_presets("OVERBITE",  OVERBITE_EXTERNAL, OVERBITE_DESCS, rng, used_names)  # 16
    presets += build_external_presets("OBSIDIAN",  OBSIDIAN_EXTRA,    OBSIDIAN_DESCS, rng, used_names)  # 4
    presets += build_external_presets("OUROBOROS", OUROBOROS_EXTRA,   OUROBOROS_DESCS, rng, used_names) # 4
    return presets


# ---------------------------------------------------------------------------
# Deduplication
# ---------------------------------------------------------------------------

def deduplicate_names(presets: list) -> list:
    seen: dict = {}
    result = []
    for p in presets:
        orig = p["name"]
        if orig not in seen:
            seen[orig] = 0
            result.append(p)
        else:
            seen[orig] += 1
            p = dict(p)
            p["name"] = f"{orig} {seen[orig]}"
            result.append(p)
    return result


# ---------------------------------------------------------------------------
# I/O
# ---------------------------------------------------------------------------

def write_preset(preset: dict, output_dir: Path, skip_existing: bool = True) -> tuple:
    output_dir.mkdir(parents=True, exist_ok=True)
    filename = preset["name"].replace("/", "-") + ".xometa"
    filepath = output_dir / filename
    if skip_existing and filepath.exists():
        return filepath, False
    with open(filepath, "w") as f:
        json.dump(preset, f, indent=2)
        f.write("\n")
    return filepath, True


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main():
    repo_root = Path(__file__).resolve().parent.parent
    default_output = repo_root / "Presets" / "XOmnibus" / "Entangled"

    parser = argparse.ArgumentParser(
        description="Generate dark-character engine Entangled coupling presets for XOmnibus.",
    )
    parser.add_argument(
        "--output-dir", type=Path, default=default_output,
        help="Directory to write .xometa files (default: Presets/XOmnibus/Entangled/)",
    )
    parser.add_argument(
        "--dry-run", action="store_true",
        help="Print preset names without writing files.",
    )
    parser.add_argument(
        "--seed", type=int, default=7,
        help="Random seed for reproducibility (default: 7).",
    )
    parser.add_argument(
        "--no-skip", action="store_true",
        help="Overwrite existing files instead of skipping them.",
    )
    args = parser.parse_args()

    rng = random.Random(args.seed)
    presets = build_all(rng)
    presets = deduplicate_names(presets)

    print(f"Dark Engines Coupling Pack — {len(presets)} presets")
    print(f"Output: {args.output_dir}")
    print()

    written = 0
    skipped = 0
    for preset in presets:
        engines_str = " × ".join(preset["engines"])
        if args.dry_run:
            print(f"  [DRY-RUN] {preset['name']}  ({engines_str})")
        else:
            path, did_write = write_preset(preset, args.output_dir, skip_existing=not args.no_skip)
            if did_write:
                print(f"  Wrote:   {path.name}")
                written += 1
            else:
                print(f"  Skipped: {path.name}  (already exists)")
                skipped += 1

    if not args.dry_run:
        print(f"\nDone — {written} written, {skipped} skipped → {args.output_dir}")
    else:
        total = len(presets)
        print(f"\nDry run complete — {total} presets would be written.")
        print()
        print("Expected breakdown:")
        print("  4   four-way marquee (OBESE + OVERBITE + OBSIDIAN + OUROBOROS)")
        print("  12  intra-group pairs (6 pairs × 2 presets each)")
        print("  16  OBESE × external partners")
        print("  16  OVERBITE × external partners")
        print("   4  OBSIDIAN × OddfeliX / OddOscar / Overdub / Odyssey")
        print("   4  OUROBOROS × OddfeliX / OddOscar / Overdub / Odyssey")
        print(f"  --")
        print(f"  {total}  total")


if __name__ == "__main__":
    main()
