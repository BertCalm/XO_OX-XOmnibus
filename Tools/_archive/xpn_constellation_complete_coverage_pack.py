#!/usr/bin/env python3
"""
xpn_constellation_complete_coverage_pack.py

Generates 72 Entangled mood .xometa presets to close coverage gaps for
OHM and OBBLIGATO (Constellation engines with fewer coupling partners).

OHM character: Sage green #87AE73, communal/hippie jam, MEDDLING/COMMUNE axis
OBBLIGATO character: Rascal Coral #FF8A7A, dual wind, BOND macro

36 OHM presets x 6 partner engines (6 each)
36 OBBLIGATO presets x 6 partner engines (6 each)
Total: 72 presets
"""

import json
import os
import re

# --- Output path ------------------------------------------------------------

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUTPUT_DIR = os.path.join(REPO_ROOT, "Presets", "XOceanus", "Entangled")
os.makedirs(OUTPUT_DIR, exist_ok=True)

# --- Partner engines --------------------------------------------------------

PARTNERS = ["ORGANON", "OUROBOROS", "OBSIDIAN", "ORIGAMI", "ORACLE", "OBSCURA"]
PRESETS_PER_PAIR = 6

# --- Engine-flavour vocabulary ----------------------------------------------

PARTNER_FLAVOR = {
    "ORGANON":   ["Metabolic", "Variational", "Organic", "Cellular", "Living"],
    "OUROBOROS": ["Recursive", "Serpent", "Chaotic", "Leashed", "Strange"],
    "OBSIDIAN":  ["Crystal", "Glassy", "Deep", "Fractured", "Mirror"],
    "ORIGAMI":   ["Folded", "Crease", "Geometric", "Paper", "Unfolded"],
    "ORACLE":    ["Prophetic", "Stochastic", "Ancient", "Broken", "GENDY"],
    "OBSCURA":   ["Silver", "Daguerreotype", "Ghosted", "Vintage", "Exposed"],
}

# --- Descriptions -----------------------------------------------------------

OHM_DESCRIPTIONS = {
    "ORGANON":   "Sage green commune meets bioluminescent metabolism — OHM's harmonics breathe through ORGANON's variational free energy.",
    "OUROBOROS": "The hippie circle and the serpent — OHM's drone locks into OUROBOROS's recursive chaos with a gentle leash.",
    "OBSIDIAN":  "Sage smoke drifts through crystal fractures — OHM's communal hum resonates inside OBSIDIAN's glassy depth.",
    "ORIGAMI":   "The commune folds itself into geometric patience — OHM's organ harmonics crease through ORIGAMI's folded architecture.",
    "ORACLE":    "Hippie sage wisdom meets stochastic prophecy — OHM's COMMUNE axis channels through ORACLE's GENDY breakpoints.",
    "OBSCURA":   "A daguerreotype of a circle gathering — OHM's drone exposed on OBSCURA's silver plate, vintage and timeless.",
}

OBBLIGATO_DESCRIPTIONS = {
    "ORGANON":   "Obligatory reeds breathe through variational metabolism — OBBLIGATO's BOND macro drives ORGANON's cellular respiration.",
    "OUROBOROS": "Coral wind tangles the serpent — OBBLIGATO's dual breath couples into OUROBOROS's recursive topology.",
    "OBSIDIAN":  "Rascal coral fractures crystal — OBBLIGATO's obligatory voice resonates inside OBSIDIAN's mirror depth.",
    "ORIGAMI":   "Two reeds fold themselves into geometry — OBBLIGATO's BOND creases through ORIGAMI's architectural patience.",
    "ORACLE":    "The obligatory prophecy — OBBLIGATO's breath drives ORACLE's stochastic breakpoints into necessary patterns.",
    "OBSCURA":   "Coral wind on silver plate — OBBLIGATO's dual voice exposed through OBSCURA's daguerreotype age.",
}

# --- Tags -------------------------------------------------------------------

OHM_TAGS_BASE = ["entangled", "coupling", "constellation", "ohm", "commune", "sage", "drone"]
OBL_TAGS_BASE = ["entangled", "coupling", "constellation", "obbligato", "bond", "wind", "coral"]

PARTNER_TAGS = {
    "ORGANON":   ["organon", "metabolic", "bioluminescent"],
    "OUROBOROS": ["ouroboros", "chaotic", "recursive", "serpent"],
    "OBSIDIAN":  ["obsidian", "crystal", "glassy"],
    "ORIGAMI":   ["origami", "folded", "geometric"],
    "ORACLE":    ["oracle", "stochastic", "prophetic"],
    "OBSCURA":   ["obscura", "daguerreotype", "silver"],
}

# --- Coupling pools ---------------------------------------------------------

PARTNER_COUPLING_POOL = {
    "ORGANON":   ["Amp->Drive", "Filter->Filter", "Macro->Param", "Spectral->Harmonic"],
    "OUROBOROS": ["Env->Cutoff", "LFO->Pitch", "Rhythm->Gate", "Amp->Drive"],
    "OBSIDIAN":  ["Filter->Filter", "Spectral->Harmonic", "Macro->Param", "Amp->Drive"],
    "ORIGAMI":   ["Pitch->Mod", "Filter->Filter", "Macro->Param", "Env->Cutoff"],
    "ORACLE":    ["Spectral->Harmonic", "LFO->Pitch", "Rhythm->Gate", "Env->Cutoff"],
    "OBSCURA":   ["Filter->Filter", "Amp->Drive", "Macro->Param", "Pitch->Mod"],
}

COUPLING_INTENSITIES = ["Light", "Medium", "Deep", "Extreme"]

# --- DNA profiles -----------------------------------------------------------
# Each list has exactly PRESETS_PER_PAIR entries.
# Every entry must have at least one dimension <= 0.15 or >= 0.85.

OHM_DNA_PROFILES = {
    "ORGANON": [
        {"brightness": 0.45, "warmth": 0.82, "movement": 0.61, "density": 0.55, "space": 0.70, "aggression": 0.12},
        {"brightness": 0.88, "warmth": 0.67, "movement": 0.58, "density": 0.49, "space": 0.62, "aggression": 0.14},
        {"brightness": 0.52, "warmth": 0.73, "movement": 0.90, "density": 0.60, "space": 0.55, "aggression": 0.10},
        {"brightness": 0.40, "warmth": 0.86, "movement": 0.47, "density": 0.85, "space": 0.58, "aggression": 0.18},
        {"brightness": 0.61, "warmth": 0.75, "movement": 0.53, "density": 0.48, "space": 0.92, "aggression": 0.13},
        {"brightness": 0.55, "warmth": 0.88, "movement": 0.66, "density": 0.57, "space": 0.63, "aggression": 0.08},
    ],
    "OUROBOROS": [
        {"brightness": 0.58, "warmth": 0.60, "movement": 0.87, "density": 0.64, "space": 0.42, "aggression": 0.55},
        {"brightness": 0.72, "warmth": 0.55, "movement": 0.91, "density": 0.58, "space": 0.38, "aggression": 0.62},
        {"brightness": 0.49, "warmth": 0.63, "movement": 0.85, "density": 0.88, "space": 0.44, "aggression": 0.50},
        {"brightness": 0.65, "warmth": 0.58, "movement": 0.79, "density": 0.62, "space": 0.35, "aggression": 0.86},
        {"brightness": 0.55, "warmth": 0.67, "movement": 0.93, "density": 0.70, "space": 0.47, "aggression": 0.58},
        {"brightness": 0.80, "warmth": 0.51, "movement": 0.88, "density": 0.55, "space": 0.40, "aggression": 0.53},
    ],
    "OBSIDIAN": [
        {"brightness": 0.32, "warmth": 0.52, "movement": 0.38, "density": 0.72, "space": 0.85, "aggression": 0.20},
        {"brightness": 0.28, "warmth": 0.48, "movement": 0.42, "density": 0.68, "space": 0.88, "aggression": 0.15},
        {"brightness": 0.92, "warmth": 0.50, "movement": 0.35, "density": 0.75, "space": 0.82, "aggression": 0.18},
        {"brightness": 0.35, "warmth": 0.55, "movement": 0.40, "density": 0.90, "space": 0.80, "aggression": 0.22},
        {"brightness": 0.30, "warmth": 0.45, "movement": 0.88, "density": 0.65, "space": 0.85, "aggression": 0.12},
        {"brightness": 0.42, "warmth": 0.60, "movement": 0.36, "density": 0.78, "space": 0.89, "aggression": 0.16},
    ],
    "ORIGAMI": [
        {"brightness": 0.75, "warmth": 0.58, "movement": 0.48, "density": 0.88, "space": 0.50, "aggression": 0.15},
        {"brightness": 0.68, "warmth": 0.55, "movement": 0.44, "density": 0.92, "space": 0.47, "aggression": 0.12},
        {"brightness": 0.85, "warmth": 0.62, "movement": 0.52, "density": 0.85, "space": 0.44, "aggression": 0.10},
        {"brightness": 0.72, "warmth": 0.50, "movement": 0.90, "density": 0.80, "space": 0.52, "aggression": 0.18},
        {"brightness": 0.78, "warmth": 0.65, "movement": 0.45, "density": 0.87, "space": 0.55, "aggression": 0.08},
        {"brightness": 0.65, "warmth": 0.60, "movement": 0.50, "density": 0.86, "space": 0.42, "aggression": 0.14},
    ],
    "ORACLE": [
        {"brightness": 0.45, "warmth": 0.38, "movement": 0.88, "density": 0.60, "space": 0.72, "aggression": 0.28},
        {"brightness": 0.38, "warmth": 0.32, "movement": 0.85, "density": 0.65, "space": 0.78, "aggression": 0.22},
        {"brightness": 0.50, "warmth": 0.42, "movement": 0.92, "density": 0.55, "space": 0.68, "aggression": 0.35},
        {"brightness": 0.42, "warmth": 0.35, "movement": 0.88, "density": 0.88, "space": 0.75, "aggression": 0.25},
        {"brightness": 0.48, "warmth": 0.40, "movement": 0.85, "density": 0.58, "space": 0.90, "aggression": 0.30},
        {"brightness": 0.35, "warmth": 0.45, "movement": 0.91, "density": 0.62, "space": 0.70, "aggression": 0.20},
    ],
    "OBSCURA": [
        {"brightness": 0.48, "warmth": 0.65, "movement": 0.35, "density": 0.58, "space": 0.88, "aggression": 0.10},
        {"brightness": 0.55, "warmth": 0.70, "movement": 0.30, "density": 0.55, "space": 0.92, "aggression": 0.08},
        {"brightness": 0.90, "warmth": 0.60, "movement": 0.38, "density": 0.52, "space": 0.85, "aggression": 0.12},
        {"brightness": 0.45, "warmth": 0.68, "movement": 0.32, "density": 0.60, "space": 0.90, "aggression": 0.15},
        {"brightness": 0.52, "warmth": 0.72, "movement": 0.88, "density": 0.50, "space": 0.87, "aggression": 0.10},
        {"brightness": 0.58, "warmth": 0.65, "movement": 0.14, "density": 0.57, "space": 0.85, "aggression": 0.08},
    ],
}

OBL_DNA_PROFILES = {
    "ORGANON": [
        {"brightness": 0.60, "warmth": 0.85, "movement": 0.65, "density": 0.58, "space": 0.62, "aggression": 0.18},
        {"brightness": 0.72, "warmth": 0.86, "movement": 0.70, "density": 0.52, "space": 0.55, "aggression": 0.22},
        {"brightness": 0.55, "warmth": 0.88, "movement": 0.60, "density": 0.90, "space": 0.48, "aggression": 0.15},
        {"brightness": 0.88, "warmth": 0.75, "movement": 0.55, "density": 0.62, "space": 0.50, "aggression": 0.20},
        {"brightness": 0.65, "warmth": 0.82, "movement": 0.92, "density": 0.55, "space": 0.65, "aggression": 0.12},
        {"brightness": 0.58, "warmth": 0.90, "movement": 0.68, "density": 0.60, "space": 0.58, "aggression": 0.25},
    ],
    "OUROBOROS": [
        {"brightness": 0.75, "warmth": 0.52, "movement": 0.88, "density": 0.65, "space": 0.38, "aggression": 0.72},
        {"brightness": 0.68, "warmth": 0.48, "movement": 0.85, "density": 0.70, "space": 0.35, "aggression": 0.78},
        {"brightness": 0.82, "warmth": 0.55, "movement": 0.92, "density": 0.60, "space": 0.40, "aggression": 0.68},
        {"brightness": 0.72, "warmth": 0.45, "movement": 0.88, "density": 0.88, "space": 0.42, "aggression": 0.75},
        {"brightness": 0.65, "warmth": 0.50, "movement": 0.90, "density": 0.68, "space": 0.38, "aggression": 0.85},
        {"brightness": 0.78, "warmth": 0.58, "movement": 0.86, "density": 0.62, "space": 0.45, "aggression": 0.70},
    ],
    "OBSIDIAN": [
        {"brightness": 0.82, "warmth": 0.48, "movement": 0.42, "density": 0.75, "space": 0.88, "aggression": 0.25},
        {"brightness": 0.88, "warmth": 0.45, "movement": 0.38, "density": 0.80, "space": 0.85, "aggression": 0.20},
        {"brightness": 0.75, "warmth": 0.52, "movement": 0.44, "density": 0.72, "space": 0.90, "aggression": 0.28},
        {"brightness": 0.85, "warmth": 0.42, "movement": 0.90, "density": 0.78, "space": 0.82, "aggression": 0.22},
        {"brightness": 0.80, "warmth": 0.50, "movement": 0.40, "density": 0.88, "space": 0.87, "aggression": 0.18},
        {"brightness": 0.92, "warmth": 0.48, "movement": 0.45, "density": 0.70, "space": 0.84, "aggression": 0.30},
    ],
    "ORIGAMI": [
        {"brightness": 0.78, "warmth": 0.62, "movement": 0.52, "density": 0.90, "space": 0.45, "aggression": 0.20},
        {"brightness": 0.85, "warmth": 0.58, "movement": 0.48, "density": 0.88, "space": 0.42, "aggression": 0.15},
        {"brightness": 0.72, "warmth": 0.65, "movement": 0.55, "density": 0.92, "space": 0.48, "aggression": 0.22},
        {"brightness": 0.90, "warmth": 0.60, "movement": 0.88, "density": 0.85, "space": 0.50, "aggression": 0.18},
        {"brightness": 0.80, "warmth": 0.68, "movement": 0.50, "density": 0.87, "space": 0.44, "aggression": 0.12},
        {"brightness": 0.75, "warmth": 0.55, "movement": 0.45, "density": 0.90, "space": 0.52, "aggression": 0.25},
    ],
    "ORACLE": [
        {"brightness": 0.62, "warmth": 0.42, "movement": 0.90, "density": 0.65, "space": 0.75, "aggression": 0.38},
        {"brightness": 0.55, "warmth": 0.38, "movement": 0.88, "density": 0.70, "space": 0.80, "aggression": 0.42},
        {"brightness": 0.68, "warmth": 0.45, "movement": 0.92, "density": 0.60, "space": 0.72, "aggression": 0.35},
        {"brightness": 0.58, "warmth": 0.40, "movement": 0.85, "density": 0.88, "space": 0.78, "aggression": 0.45},
        {"brightness": 0.65, "warmth": 0.48, "movement": 0.90, "density": 0.62, "space": 0.85, "aggression": 0.40},
        {"brightness": 0.72, "warmth": 0.35, "movement": 0.88, "density": 0.68, "space": 0.70, "aggression": 0.50},
    ],
    "OBSCURA": [
        {"brightness": 0.70, "warmth": 0.72, "movement": 0.38, "density": 0.60, "space": 0.90, "aggression": 0.15},
        {"brightness": 0.65, "warmth": 0.78, "movement": 0.35, "density": 0.55, "space": 0.88, "aggression": 0.12},
        {"brightness": 0.85, "warmth": 0.68, "movement": 0.40, "density": 0.58, "space": 0.92, "aggression": 0.18},
        {"brightness": 0.72, "warmth": 0.75, "movement": 0.88, "density": 0.62, "space": 0.85, "aggression": 0.10},
        {"brightness": 0.68, "warmth": 0.80, "movement": 0.32, "density": 0.52, "space": 0.90, "aggression": 0.14},
        {"brightness": 0.78, "warmth": 0.70, "movement": 0.42, "density": 0.65, "space": 0.87, "aggression": 0.20},
    ],
}

# --- Name generators --------------------------------------------------------

OHM_WORDS_A = [
    "Commune", "Circle", "Sage", "Grove", "Drift", "Ohm", "Haze",
    "Meadow", "Solstice", "Canopy", "Ember", "Mycelium", "Tide",
    "Gather", "Bloom", "Smoke", "Chant", "Weave", "Root", "Sun",
]
OHM_WORDS_B = [
    "Ring", "Pulse", "Thread", "Tone", "Veil", "Spiral", "Knot",
    "Mist", "Drift", "Hum", "Fold", "Breath", "Flow", "Bond",
    "Glow", "Sway", "Cloud", "Web", "Hymn", "Wave",
]

OBL_WORDS_A = [
    "Bond", "Reed", "Coral", "Rascal", "Breath", "Obligate", "Duet",
    "Tie", "Wind", "Voice", "Pledge", "Weave", "Bind", "Nestle",
    "Flute", "Pipe", "Melody", "Thread", "Tandem", "Pair",
]
OBL_WORDS_B = [
    "Vow", "Current", "Lock", "Union", "Sway", "Pulse", "Dance",
    "Arc", "Drift", "Knot", "Tangle", "Link", "Flow", "Trace",
    "Chord", "Braid", "Hum", "Lace", "Ring", "Bind",
]


def make_ohm_name(partner: str, i: int) -> str:
    flavor = PARTNER_FLAVOR[partner]
    a = OHM_WORDS_A[i % len(OHM_WORDS_A)]
    b = OHM_WORDS_B[(i * 3 + 7) % len(OHM_WORDS_B)]
    f = flavor[i % len(flavor)]
    candidates = [
        f"OHM {f} {a}",
        f"{a} {f} Hum",
        f"Sage {f} {b}",
        f"OHM {a} {b}",
        f"{f} {a} Circle",
        f"Green {f} {b}",
    ]
    return candidates[i % len(candidates)]


def make_obbligato_name(partner: str, i: int) -> str:
    flavor = PARTNER_FLAVOR[partner]
    a = OBL_WORDS_A[i % len(OBL_WORDS_A)]
    b = OBL_WORDS_B[(i * 3 + 5) % len(OBL_WORDS_B)]
    f = flavor[i % len(flavor)]
    candidates = [
        f"OBBLIGATO {f} {a}",
        f"{a} {f} Reed",
        f"Coral {f} {b}",
        f"OBBLIGATO {a} {b}",
        f"{f} Bond {a}",
        f"Rascal {f} {b}",
    ]
    return candidates[i % len(candidates)]


def to_snakecase(name: str) -> str:
    s = name.lower()
    s = re.sub(r"[^a-z0-9]+", "_", s)
    return s.strip("_")


# --- Engine parameter builder -----------------------------------------------

def build_engine_params(engine: str, dna: dict) -> dict:
    params = {}
    if engine == "OHM":
        params["ohm_macroMeddling"] = round(0.4 + dna["movement"] * 0.4, 3)
        params["ohm_macroCommune"] = round(0.5 + dna["warmth"] * 0.3, 3)
        params["ohm_orgHarmonics"] = round(dna["brightness"] * 0.8 + 0.1, 3)
        params["ohm_droneFund"] = round(0.3 + dna["density"] * 0.4, 3)
    elif engine == "OBBLIGATO":
        params["obbl_breathA"] = round(0.35 + dna["movement"] * 0.45, 3)
        params["obbl_breathB"] = round(0.30 + dna["density"] * 0.40, 3)
        params["obbl_macroBond"] = round(0.50 + dna["warmth"] * 0.35, 3)
    elif engine == "ORGANON":
        params["organon_metabolicRate"] = round(0.4 + dna["movement"] * 0.5, 3)
        params["organon_variationalEnergy"] = round(0.3 + dna["warmth"] * 0.6, 3)
    elif engine == "OUROBOROS":
        params["ouro_topology"] = round(0.3 + dna["aggression"] * 0.6, 3)
        params["ouro_chaosLeash"] = round(max(0.0, 1.0 - dna["aggression"] * 0.5), 3)
    elif engine == "OBSIDIAN":
        params["obsidian_depth"] = round(0.5 + dna["space"] * 0.4, 3)
        params["obsidian_crystalResonance"] = round(0.3 + dna["brightness"] * 0.5, 3)
    elif engine == "ORIGAMI":
        params["origami_foldPoint"] = round(0.2 + dna["density"] * 0.6, 3)
        params["origami_creaseDensity"] = round(0.4 + dna["density"] * 0.4, 3)
    elif engine == "ORACLE":
        params["oracle_breakpoints"] = round(0.3 + dna["movement"] * 0.5, 3)
        params["oracle_stochasticRate"] = round(0.4 + dna["movement"] * 0.4, 3)
    elif engine == "OBSCURA":
        params["obscura_stiffness"] = round(0.3 + (1.0 - dna["movement"]) * 0.5, 3)
        params["obscura_daguerreotypeAge"] = round(0.4 + dna["space"] * 0.4, 3)
    return params


# --- Preset builder ---------------------------------------------------------

def build_preset(primary: str, partner: str, name: str, dna: dict, i: int,
                 description: str, tags: list) -> dict:
    coupling_type = PARTNER_COUPLING_POOL[partner][i % len(PARTNER_COUPLING_POOL[partner])]
    coupling_amount = round(0.55 + (i % 5) * 0.08, 3)
    intensity = COUPLING_INTENSITIES[i % len(COUPLING_INTENSITIES)]

    character = round((dna["warmth"] + dna["brightness"]) / 2, 3)
    movement = round(dna["movement"], 3)
    coupling_macro = round(min(1.0, 0.55 + dna["density"] * 0.3), 3)
    space = round(dna["space"], 3)

    all_params = {}
    all_params.update(build_engine_params(primary, dna))
    all_params.update(build_engine_params(partner, dna))

    return {
        "schema_version": 1,
        "name": name,
        "mood": "Entangled",
        "engines": [primary, partner],
        "author": "XO_OX",
        "version": "1.0.0",
        "description": description,
        "tags": tags,
        "macroLabels": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
        "couplingIntensity": intensity,
        "tempo": None,
        "created": "2026-03-16",
        "parameters": all_params,
        "coupling": {
            "pairs": [
                {
                    "engineA": primary,
                    "engineB": partner,
                    "type": coupling_type,
                    "amount": coupling_amount,
                }
            ]
        },
        "macros": {
            "CHARACTER": character,
            "MOVEMENT": movement,
            "COUPLING": coupling_macro,
            "SPACE": space,
        },
        "sequencer": None,
        "dna": {k: round(v, 3) for k, v in dna.items()},
    }


def validate_dna_extremes(dna: dict) -> bool:
    return any(v <= 0.15 or v >= 0.85 for v in dna.values())


# --- Main -------------------------------------------------------------------

def generate_all():
    # Collect filenames already on disk (stem only, no extension)
    used_on_disk = {os.path.splitext(f)[0] for f in os.listdir(OUTPUT_DIR) if f.endswith(".xometa")}

    configs = []

    for partner in PARTNERS:
        for i in range(PRESETS_PER_PAIR):
            dna = OHM_DNA_PROFILES[partner][i]
            assert validate_dna_extremes(dna), f"OHM x {partner} [{i}] lacks extreme DNA"
            configs.append(("OHM", partner, make_ohm_name(partner, i), dna, i,
                            OHM_DESCRIPTIONS[partner], OHM_TAGS_BASE + PARTNER_TAGS[partner]))

        for i in range(PRESETS_PER_PAIR):
            dna = OBL_DNA_PROFILES[partner][i]
            assert validate_dna_extremes(dna), f"OBBLIGATO x {partner} [{i}] lacks extreme DNA"
            configs.append(("OBBLIGATO", partner, make_obbligato_name(partner, i), dna, i,
                            OBBLIGATO_DESCRIPTIONS[partner], OBL_TAGS_BASE + PARTNER_TAGS[partner]))

    # Deduplicate names within this run
    seen_names: set = set()
    written = 0
    skipped = 0

    for primary, partner, raw_name, dna, i, desc, tags in configs:
        name = raw_name
        bump = 0
        while name in seen_names:
            bump += 1
            name = f"{raw_name} {bump}"
        seen_names.add(name)

        snake = to_snakecase(name)
        if snake in used_on_disk:
            print(f"  SKIP (exists): {snake}.xometa")
            skipped += 1
            continue

        preset = build_preset(primary, partner, name, dna, i, desc, list(tags))
        out_path = os.path.join(OUTPUT_DIR, f"{snake}.xometa")
        with open(out_path, "w", encoding="utf-8") as f:
            json.dump(preset, f, indent=2, ensure_ascii=False)
            f.write("\n")
        used_on_disk.add(snake)
        written += 1
        print(f"  WROTE [{primary} x {partner}]: {snake}.xometa")

    print(f"\nDone — {written} written, {skipped} skipped.")
    return written


if __name__ == "__main__":
    print(f"Output dir: {OUTPUT_DIR}\n")
    generate_all()
