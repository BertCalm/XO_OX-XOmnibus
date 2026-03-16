#!/usr/bin/env python3
"""Generate OBBLIGATO coupling presets for XOmnibus.

Fills the coupling heatmap gap: OBBLIGATO (Rascal Coral #FF8A7A, dual wind
engine with BOND macro, obbl_ prefix) had ZERO coupling presets.

Generates:
  - 4 Constellation pairs  × 4 presets = 16  (OHM, ORPHICA, OTTONI, OLE)
  - 12 legacy pairs        × 3 presets = 36  (OPAL, ORACLE, OBLONG, ODYSSEY,
    OCEANIC, OVERDUB, ORBITAL, ORGANON, OSPREY, ORIGAMI, OBLIQUE, OWLFISH)
  Total: 52 presets → Presets/XOmnibus/Entangled/
"""

import argparse
import json
import os
import random
import sys
from pathlib import Path

# ---------------------------------------------------------------------------
# DNA baselines
# ---------------------------------------------------------------------------
OBBL_DNA = {
    "brightness": 0.55,
    "warmth": 0.65,
    "movement": 0.60,
    "density": 0.65,
    "space": 0.60,
    "aggression": 0.45,
}

PARTNER_DNA = {
    "OHM":     {"brightness": 0.45, "warmth": 0.75, "movement": 0.55, "density": 0.60, "space": 0.70, "aggression": 0.30},
    "ORPHICA": {"brightness": 0.80, "warmth": 0.50, "movement": 0.70, "density": 0.45, "space": 0.75, "aggression": 0.25},
    "OTTONI":  {"brightness": 0.60, "warmth": 0.60, "movement": 0.50, "density": 0.70, "space": 0.55, "aggression": 0.60},
    "OLE":     {"brightness": 0.65, "warmth": 0.70, "movement": 0.75, "density": 0.60, "space": 0.60, "aggression": 0.55},
    "OPAL":    {"brightness": 0.70, "warmth": 0.50, "movement": 0.75, "density": 0.45, "space": 0.80, "aggression": 0.20},
    "ORACLE":  {"brightness": 0.50, "warmth": 0.40, "movement": 0.60, "density": 0.70, "space": 0.70, "aggression": 0.40},
    "OBLONG":  {"brightness": 0.60, "warmth": 0.65, "movement": 0.50, "density": 0.65, "space": 0.55, "aggression": 0.50},
    "ODYSSEY": {"brightness": 0.55, "warmth": 0.50, "movement": 0.70, "density": 0.50, "space": 0.70, "aggression": 0.30},
    "OCEANIC": {"brightness": 0.65, "warmth": 0.55, "movement": 0.50, "density": 0.55, "space": 0.75, "aggression": 0.30},
    "OVERDUB": {"brightness": 0.45, "warmth": 0.70, "movement": 0.55, "density": 0.55, "space": 0.75, "aggression": 0.25},
    "ORBITAL": {"brightness": 0.60, "warmth": 0.65, "movement": 0.55, "density": 0.60, "space": 0.60, "aggression": 0.50},
    "ORGANON": {"brightness": 0.60, "warmth": 0.60, "movement": 0.65, "density": 0.75, "space": 0.65, "aggression": 0.40},
    "OSPREY":  {"brightness": 0.55, "warmth": 0.55, "movement": 0.50, "density": 0.60, "space": 0.70, "aggression": 0.35},
    "ORIGAMI": {"brightness": 0.70, "warmth": 0.45, "movement": 0.65, "density": 0.55, "space": 0.55, "aggression": 0.55},
    "OBLIQUE": {"brightness": 0.75, "warmth": 0.40, "movement": 0.70, "density": 0.50, "space": 0.60, "aggression": 0.55},
    "OWLFISH": {"brightness": 0.40, "warmth": 0.60, "movement": 0.40, "density": 0.65, "space": 0.80, "aggression": 0.20},
}

# ---------------------------------------------------------------------------
# Coupling types available in MegaCouplingMatrix
# ---------------------------------------------------------------------------
COUPLING_TYPES = [
    "Amp->Filter",
    "Env->Pitch",
    "LFO->Pan",
    "Env->Morph",
    "Audio->Wavetable",
    "Pitch->Filter",
    "Amp->Drive",
    "Env->LFO Rate",
    "LFO->Filter",
    "Filter->Pitch",
    "Amp->Space",
    "Env->Drive",
]

# ---------------------------------------------------------------------------
# Name vocabulary
# ---------------------------------------------------------------------------
NAME_WORDS = [
    "Bond", "Rascal", "Coral", "Double Reed", "Breath Bond", "Wind Tie",
    "Dual Voice", "Coral Thread", "Obligate", "Necessary", "Bound Voice",
    "The Bond",
]

# ---------------------------------------------------------------------------
# Per-partner flavour: (coupling_type_pool, description_template, tag_extras)
# ---------------------------------------------------------------------------
PARTNER_FLAVOUR = {
    "OHM": {
        "types": ["LFO->Filter", "Amp->Filter", "Env->LFO Rate"],
        "descs": [
            "OBBLIGATO's BOND tightens as OHM's meditative drone grounds the dual reeds.",
            "A hippy-dad commune — OHM's commune axis holds OBBLIGATO's breath voices in harmonic orbit via BOND.",
            "Warm reed solidarity: BOND macro ties OBBLIGATO's two winds to OHM's slow LFO breath.",
            "OHM hum meets coral coral wind — BOND creates an obligate resonance between the sage and the reed.",
        ],
        "tags": ["ohm", "drone", "meditative"],
    },
    "ORPHICA": {
        "types": ["Amp->Filter", "Env->Pitch", "LFO->Pan"],
        "descs": [
            "ORPHICA's microsound harp plucks resonate through OBBLIGATO's BOND, doubling the coral reed shimmer.",
            "Siphonophore colony meets dual wind: BOND macro synchronises OBBLIGATO's voices with ORPHICA's grain clusters.",
            "Bright harp threads weave through OBBLIGATO's breath pair; BOND knots them into coral lace.",
            "ORPHICA granular scatter feeds OBBLIGATO's filter; BOND casts the result as a double coral shimmer.",
        ],
        "tags": ["orphica", "granular", "shimmer"],
    },
    "OTTONI": {
        "types": ["Amp->Drive", "Env->Pitch", "Amp->Filter"],
        "descs": [
            "Triple brass GROW meets OBBLIGATO's BOND — two reeds locked to three brass voices in patina warmth.",
            "OTTONI's brass swell triggers OBBLIGATO's BOND macro, forcing the wind pair to rise in unison.",
            "Coral and patina: BOND binds OBBLIGATO's dual reeds to OTTONI's third brass harmonic for obligate resonance.",
            "OBBLIGATO's necessary voices obligate themselves to OTTONI's GROW arc via the BOND coupling.",
        ],
        "tags": ["ottoni", "brass", "grow"],
    },
    "OLE": {
        "types": ["LFO->Pan", "Env->Morph", "Amp->Filter"],
        "descs": [
            "Afro-Latin DRAMA ignites OBBLIGATO's BOND, fusing the dual reeds into a percussive coral call.",
            "OLE's hibiscus rhythms knot through OBBLIGATO's BOND macro in tonal solidarity.",
            "OBBLIGATO rascal coral answers OLE's communal call — BOND creates an obligate rhythmic breath tie.",
            "Wind and drums in obligate union: BOND tethers OBBLIGATO's voice pair to OLE's DRAMA pulse.",
        ],
        "tags": ["ole", "afro-latin", "rhythmic"],
    },
    "OPAL": {
        "types": ["Audio->Wavetable", "Env->Morph", "LFO->Filter"],
        "descs": [
            "OPAL grain clouds drift through OBBLIGATO's BOND — coral reed and lavender granular in obligate suspension.",
            "OBBLIGATO's dual voices scatter into OPAL's grain field; BOND collects them back into a coral thread.",
            "Granular lavender wraps OBBLIGATO's necessary wind pair via BOND macro coupling.",
        ],
        "tags": ["opal", "granular", "suspended"],
    },
    "ORACLE": {
        "types": ["Pitch->Filter", "Env->LFO Rate", "Filter->Pitch"],
        "descs": [
            "ORACLE stochastic pitch arcs activate OBBLIGATO's BOND, making the coral reeds an obligate prophecy.",
            "GENDY maqam lattice meets dual wind BOND — OBBLIGATO's voices are bound to ORACLE's spectral decree.",
            "Prophecy indigo speaks through OBBLIGATO's coral reeds; BOND makes the dual breath an obligate response.",
        ],
        "tags": ["oracle", "stochastic", "maqam"],
    },
    "OBLONG": {
        "types": ["Amp->Filter", "Env->Pitch", "LFO->Pan"],
        "descs": [
            "Amber warmth from OBLONG holds OBBLIGATO's BOND — coral reed and amber filter in necessary harmony.",
            "OBLONG's bob filter sweeps OBBLIGATO's dual voices; BOND tethers the coral thread to amber curves.",
            "Rascal coral meets warm amber: BOND creates a literal tonal tie between the two wind voices and OBLONG's resonance.",
        ],
        "tags": ["oblong", "amber", "warm"],
    },
    "ODYSSEY": {
        "types": ["LFO->Filter", "Env->Morph", "Amp->Space"],
        "descs": [
            "ODYSSEY's tidal drift modulates OBBLIGATO's BOND, spreading the coral reed pair across the voyage.",
            "An obligate odyssey: BOND locks OBBLIGATO's two winds to ODYSSEY's wavetable tidal sweep.",
            "Violet tide meets rascal coral — BOND creates a breath bond that shifts with ODYSSEY's ocean current.",
        ],
        "tags": ["odyssey", "tidal", "drift"],
    },
    "OCEANIC": {
        "types": ["Amp->Space", "LFO->Filter", "Env->LFO Rate"],
        "descs": [
            "Phosphorescent teal surrounds OBBLIGATO's coral reeds; BOND surfaces as bioluminescent wind tie.",
            "OCEANIC separation engine holds OBBLIGATO's dual voices in obligate aquatic suspension via BOND.",
            "Coral reef breath: BOND ties OBBLIGATO's wind pair to OCEANIC's chromophore shimmer column.",
        ],
        "tags": ["oceanic", "bioluminescent", "aquatic"],
    },
    "OVERDUB": {
        "types": ["Audio->Wavetable", "Amp->Drive", "Amp->Space"],
        "descs": [
            "Tape warmth embraces OBBLIGATO's BOND — coral reeds echo into OVERDUB's olive spring reverb.",
            "OVERDUB's dub delay loops OBBLIGATO's dual voices; BOND makes the tape echo an obligate breath bond.",
            "Olive and coral: BOND ties OBBLIGATO's necessary wind pair to OVERDUB's lo-fi tape warmth.",
        ],
        "tags": ["overdub", "tape", "echo"],
    },
    "ORBITAL": {
        "types": ["Env->Pitch", "Amp->Filter", "LFO->Pan"],
        "descs": [
            "ORBITAL group envelopes govern OBBLIGATO's BOND — coral reeds placed in warm red orbital arc.",
            "Rascal coral in orbit: BOND synchronises OBBLIGATO's dual voices to ORBITAL's group swell.",
            "ORBITAL brightness sweeps OBBLIGATO's filter; BOND makes the obligate wind arc a complete orbit.",
        ],
        "tags": ["orbital", "group-envelope", "warm"],
    },
    "ORGANON": {
        "types": ["Env->LFO Rate", "Filter->Pitch", "Amp->Drive"],
        "descs": [
            "ORGANON metabolic rate pulses through OBBLIGATO's BOND — free energy meets obligate coral breath.",
            "Variational free energy meets dual wind BOND: ORGANON drives OBBLIGATO's voice metabolism.",
            "Bioluminescent cyan and rascal coral in BOND union — ORGANON's variational metabolism breathes through both reeds.",
        ],
        "tags": ["organon", "metabolic", "bioluminescent"],
    },
    "OSPREY": {
        "types": ["Amp->Filter", "LFO->Pan", "Pitch->Filter"],
        "descs": [
            "OSPREY's shore blend carries OBBLIGATO's BOND into azulejo coastal wind.",
            "Five-coastline data ties OBBLIGATO's dual voices via BOND — shore culture woven into coral reed.",
            "BOND makes OBBLIGATO's two winds an obligate coastal dialogue with OSPREY's ShoreSystem.",
        ],
        "tags": ["osprey", "shore", "coastal"],
    },
    "ORIGAMI": {
        "types": ["Env->Morph", "Amp->Filter", "LFO->Filter"],
        "descs": [
            "Vermillion fold meets rascal coral — BOND creases OBBLIGATO's dual reeds into ORIGAMI's fold point.",
            "ORIGAMI fold geometry shapes OBBLIGATO's BOND macro, sculpting the coral breath into angular form.",
            "Necessary voices, necessary folds: BOND ties OBBLIGATO's wind pair to ORIGAMI's vermillion crease.",
        ],
        "tags": ["origami", "fold", "geometric"],
    },
    "OBLIQUE": {
        "types": ["Pitch->Filter", "LFO->Pan", "Env->Pitch"],
        "descs": [
            "OBLIQUE prism scatter refracts OBBLIGATO's BOND — coral reeds split into prismatic violet threads.",
            "Oblique bounce geometry modulates OBBLIGATO's dual voices; BOND knots them into prismatic coral.",
            "BOND makes OBBLIGATO's obligate wind pair bounce at oblique angles through OBLIQUE's prism color.",
        ],
        "tags": ["oblique", "prism", "prismatic"],
    },
    "OWLFISH": {
        "types": ["Amp->Space", "LFO->Filter", "Env->LFO Rate"],
        "descs": [
            "OWLFISH Mixtur-Trautonium subharmonics anchor OBBLIGATO's BOND in abyssal gold depth.",
            "Abyssal gold pressure surrounds OBBLIGATO's coral reeds; BOND surfaces as a deep obligate breath tie.",
            "BOND tethers OBBLIGATO's dual voices to OWLFISH's subharmonic column — coral meets the abyss.",
        ],
        "tags": ["owlfish", "subharmonic", "abyssal"],
    },
}

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def blend_dna(a: dict, b: dict, weight_a: float = 0.5) -> dict:
    """Blend two DNA dicts. weight_a is how much OBBLIGATO dominates."""
    wb = 1.0 - weight_a
    return {k: round(a[k] * weight_a + b[k] * wb, 3) for k in a}


def coupling_intensity(amount: float) -> str:
    if amount >= 0.75:
        return "Deep"
    if amount >= 0.5:
        return "Moderate"
    return "Light"


def make_preset(
    name: str,
    partner: str,
    coupling_type: str,
    coupling_amount: float,
    dna: dict,
    description: str,
    tags: list,
) -> dict:
    return {
        "schema_version": 1,
        "name": name,
        "mood": "Entangled",
        "engines": ["OBBLIGATO", partner],
        "author": "XO_OX",
        "version": "1.0.0",
        "description": description,
        "tags": ["coupling", "obbligato", "bond"] + tags,
        "macroLabels": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
        "couplingIntensity": coupling_intensity(coupling_amount),
        "tempo": None,
        "created": "2026-03-16",
        "parameters": {
            "obbl_breathA": round(random.uniform(0.4, 0.8), 3),
            "obbl_breathB": round(random.uniform(0.4, 0.8), 3),
            "obbl_macroBond": round(random.uniform(0.5, 1.0), 3),
        },
        "coupling": {
            "pairs": [
                {
                    "engineA": "OBBLIGATO",
                    "engineB": partner,
                    "type": coupling_type,
                    "amount": round(coupling_amount, 3),
                }
            ]
        },
        "sequencer": None,
        "dna": dna,
    }


def name_for(partner: str, index: int, rng: random.Random) -> str:
    """Generate a unique-enough preset name for OBBLIGATO + partner."""
    word = rng.choice(NAME_WORDS)
    partner_short = partner.capitalize()
    suffixes = [
        f"{word} with {partner_short}",
        f"Coral {word} × {partner_short}",
        f"OBBLIGATO {word} — {partner_short}",
        f"Rascal {word} ft {partner_short}",
        f"Bound {partner_short}",
        f"Necessary {partner_short}",
        f"Double Reed {partner_short}",
        f"Wind Tie {partner_short}",
        f"Obligate {partner_short} {index + 1}",
        f"Coral Thread {partner_short}",
        f"Breath Bond {partner_short}",
        f"The Bond {partner_short} {index + 1}",
    ]
    return suffixes[index % len(suffixes)]


# ---------------------------------------------------------------------------
# Core generator
# ---------------------------------------------------------------------------

def generate_presets(seed: int, count_legacy: int, count_constellation: int) -> list:
    rng = random.Random(seed)
    presets = []

    CONSTELLATION_PARTNERS = ["OHM", "ORPHICA", "OTTONI", "OLE"]
    LEGACY_PARTNERS = [
        "OPAL", "ORACLE", "OBLONG", "ODYSSEY", "OCEANIC", "OVERDUB",
        "ORBITAL", "ORGANON", "OSPREY", "ORIGAMI", "OBLIQUE", "OWLFISH",
    ]

    def gen_for_partner(partner: str, count: int):
        flavour = PARTNER_FLAVOUR[partner]
        types = flavour["types"]
        descs = flavour["descs"]
        tags = flavour["tags"]
        partner_dna = PARTNER_DNA[partner]

        for i in range(count):
            coupling_type = types[i % len(types)]
            desc = descs[i % len(descs)]
            amount = round(rng.uniform(0.45, 0.85), 3)
            # Slightly vary OBBLIGATO weight per variant
            weight_a = round(rng.uniform(0.4, 0.65), 2)
            dna = blend_dna(OBBL_DNA, partner_dna, weight_a)
            preset_name = name_for(partner, i, rng)
            presets.append(
                make_preset(preset_name, partner, coupling_type, amount, dna, desc, tags)
            )

    for partner in CONSTELLATION_PARTNERS:
        gen_for_partner(partner, count_constellation)

    for partner in LEGACY_PARTNERS:
        gen_for_partner(partner, count_legacy)

    return presets


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def parse_args():
    parser = argparse.ArgumentParser(
        description="Generate OBBLIGATO coupling presets for XOmnibus."
    )
    parser.add_argument(
        "--output-dir",
        default=None,
        help="Override output directory (default: Presets/XOmnibus/Entangled/ relative to repo root)",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print preset names without writing files",
    )
    parser.add_argument(
        "--seed",
        type=int,
        default=42,
        help="Random seed for reproducible output (default: 42)",
    )
    parser.add_argument(
        "--count",
        type=int,
        default=3,
        help="Presets per legacy partner (default: 3). Constellation partners always get 4.",
    )
    return parser.parse_args()


def main():
    args = parse_args()

    # Resolve output directory
    if args.output_dir:
        out_dir = Path(args.output_dir)
    else:
        repo_root = Path(__file__).resolve().parent.parent
        out_dir = repo_root / "Presets" / "XOmnibus" / "Entangled"

    presets = generate_presets(
        seed=args.seed,
        count_legacy=args.count,
        count_constellation=4,
    )

    if args.dry_run:
        print(f"Dry run — {len(presets)} presets would be written to: {out_dir}")
        for p in presets:
            partner = p["engines"][1]
            print(f"  [{partner:10s}]  {p['name']}")
        return

    out_dir.mkdir(parents=True, exist_ok=True)

    written = 0
    skipped = 0
    for preset in presets:
        filename = preset["name"] + ".xometa"
        filepath = out_dir / filename
        if filepath.exists():
            skipped += 1
            continue
        with open(filepath, "w", encoding="utf-8") as f:
            json.dump(preset, f, indent=2, ensure_ascii=False)
            f.write("\n")
        written += 1

    total = written + skipped
    print(
        f"OBBLIGATO coupling pack: {written} written, {skipped} skipped "
        f"(already existed) — {total} total → {out_dir}"
    )


if __name__ == "__main__":
    main()
