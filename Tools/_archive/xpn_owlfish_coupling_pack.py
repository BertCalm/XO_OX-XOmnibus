#!/usr/bin/env python3
"""Generate coupling presets filling the OWLFISH zero-coverage gap.

OWLFISH (Abyssal Gold #B8860B, Mixtur-Trautonium oscillator, owl_ prefix)
pairs with 12 partner engines — 3 presets per pair = 36 presets total.
All written to Presets/XOlokun/Entangled/ (mood: Entangled).

Usage:
    python Tools/xpn_owlfish_coupling_pack.py
    python Tools/xpn_owlfish_coupling_pack.py --dry-run
    python Tools/xpn_owlfish_coupling_pack.py --seed 42 --count 2
    python Tools/xpn_owlfish_coupling_pack.py --output-dir /path/to/Entangled
"""

import argparse
import json
import os
import pathlib
import random
import sys

# ---------------------------------------------------------------------------
# OWLFISH Sonic DNA
# ---------------------------------------------------------------------------
OWLFISH_DNA = {
    "brightness": 0.4,
    "warmth": 0.6,
    "movement": 0.4,
    "density": 0.65,
    "space": 0.8,
    "aggression": 0.2,
}

PARTNER_DNA = {
    "ORACLE":    {"brightness": 0.5,  "warmth": 0.4,  "movement": 0.6,  "density": 0.7,  "space": 0.7,  "aggression": 0.4},
    "OCEANIC":   {"brightness": 0.65, "warmth": 0.55, "movement": 0.5,  "density": 0.55, "space": 0.75, "aggression": 0.3},
    "OPAL":      {"brightness": 0.7,  "warmth": 0.5,  "movement": 0.75, "density": 0.45, "space": 0.8,  "aggression": 0.2},
    "OBSIDIAN":  {"brightness": 0.9,  "warmth": 0.2,  "movement": 0.3,  "density": 0.5,  "space": 0.6,  "aggression": 0.1},
    "ORGANON":   {"brightness": 0.6,  "warmth": 0.6,  "movement": 0.65, "density": 0.75, "space": 0.65, "aggression": 0.4},
    "ODYSSEY":   {"brightness": 0.55, "warmth": 0.5,  "movement": 0.7,  "density": 0.5,  "space": 0.7,  "aggression": 0.3},
    "OVERDUB":   {"brightness": 0.45, "warmth": 0.7,  "movement": 0.55, "density": 0.55, "space": 0.75, "aggression": 0.25},
    "OBLONG":    {"brightness": 0.6,  "warmth": 0.65, "movement": 0.5,  "density": 0.65, "space": 0.55, "aggression": 0.5},
    "ORBITAL":   {"brightness": 0.6,  "warmth": 0.65, "movement": 0.55, "density": 0.6,  "space": 0.6,  "aggression": 0.5},
    "OUROBOROS": {"brightness": 0.5,  "warmth": 0.4,  "movement": 0.85, "density": 0.75, "space": 0.5,  "aggression": 0.8},
    "OSPREY":    {"brightness": 0.55, "warmth": 0.55, "movement": 0.5,  "density": 0.6,  "space": 0.7,  "aggression": 0.35},
    "OCELOT":    {"brightness": 0.65, "warmth": 0.6,  "movement": 0.6,  "density": 0.55, "space": 0.6,  "aggression": 0.5},
}

# Engine key used in presets (matches existing presets in the repo)
ENGINE_KEY = {
    "OWLFISH":   "XOwlfish",
    "ORACLE":    "Oracle",
    "OCEANIC":   "Oceanic",
    "OPAL":      "Opal",
    "OBSIDIAN":  "Obsidian",
    "ORGANON":   "Organon",
    "ODYSSEY":   "Odyssey",
    "OVERDUB":   "Overdub",
    "OBLONG":    "Oblong",
    "ORBITAL":   "Orbital",
    "OUROBOROS": "Ouroboros",
    "OSPREY":    "Osprey",
    "OCELOT":    "Ocelot",
}

# Name vocabulary
OWLFISH_WORDS = [
    "Stria", "Subharmonic", "Partial", "Resonant", "Deep Tone",
    "Trautonium", "Spectral Cord", "Abyssal String", "Golden Thread",
    "Harmonic Series", "Overtone Veil", "Bass Partial",
]

# ---------------------------------------------------------------------------
# Per-pair configuration: (coupling_type, amount, description_theme)
# Each pair has 3 preset variants differing in name, coupling amount, and DNA bias
# ---------------------------------------------------------------------------
PAIR_CONFIGS = {
    "ORACLE": {
        "theme": "prophetic + abyssal depth",
        "coupling_types": ["Amp->Filter", "LFO->Pitch", "Env->Resonance"],
        "amounts": [0.55, 0.70, 0.45],
        "desc_templates": [
            "OWLFISH subharmonic series divines ORACLE's filter threshold. Trautonium resonance meets prophetic clarity.",
            "ORACLE's slow LFO oracle-pulse modulates OWLFISH fundamental pitch. Depth prophecy.",
            "OWLFISH envelope releases trigger ORACLE resonance swells. Abyssal wisdom.",
        ],
        "tags_extra": ["prophetic", "depth", "resonance"],
    },
    "OCEANIC": {
        "theme": "abyssal gold + phosphorescent teal",
        "coupling_types": ["Amp->Filter", "Audio->Wavetable", "LFO->Morph"],
        "amounts": [0.6, 0.5, 0.65],
        "desc_templates": [
            "OWLFISH golden harmonics modulate OCEANIC's teal filter. Twin abyssal siblings.",
            "OCEANIC's phosphorescent wave shapes OWLFISH partial balance. Bioluminescent bass.",
            "OCEANIC LFO drift steers OWLFISH timbral morph. Depth siblings converge.",
        ],
        "tags_extra": ["oceanic", "bioluminescent", "depth"],
    },
    "OPAL": {
        "theme": "golden + granular timbre blend",
        "coupling_types": ["Env->GrainSize", "Amp->Filter", "LFO->Pitch"],
        "amounts": [0.5, 0.6, 0.4],
        "desc_templates": [
            "OWLFISH envelope contour sculpts OPAL grain size. Harmonic series fractured into cloud.",
            "OWLFISH amplitude swells open OPAL's spectral filter. Gold dissolves into granules.",
            "OPAL LFO shimmer modulates OWLFISH subharmonic pitch. Granular warmth below.",
        ],
        "tags_extra": ["granular", "timbre", "spectral"],
    },
    "OBSIDIAN": {
        "theme": "abyssal + crystal polar clarity",
        "coupling_types": ["Amp->Filter", "Env->Attack", "LFO->Resonance"],
        "amounts": [0.45, 0.55, 0.5],
        "desc_templates": [
            "OWLFISH warm harmonic series drives OBSIDIAN crystal filter. Dark gold meets cold glass.",
            "OBSIDIAN attack precision gates OWLFISH subharmonic envelope. Clarity from the abyss.",
            "OWLFISH LFO warmth modulates OBSIDIAN resonance. Polar opposites in harmonic dialogue.",
        ],
        "tags_extra": ["crystal", "polar", "clarity"],
    },
    "ORGANON": {
        "theme": "metabolism + trautonium resonance",
        "coupling_types": ["Env->Filter", "Amp->Morph", "LFO->Pitch"],
        "amounts": [0.6, 0.55, 0.5],
        "desc_templates": [
            "ORGANON metabolic envelope feeds OWLFISH filter. Biological rhythm meets Trautonium.",
            "OWLFISH amplitude drives ORGANON morph crossfade. Organ-pipe biology.",
            "ORGANON rhythmic LFO modulates OWLFISH fundamental. Metabolic harmonic series.",
        ],
        "tags_extra": ["metabolism", "organic", "trautonium"],
    },
    "ODYSSEY": {
        "theme": "drift + subharmonic",
        "coupling_types": ["LFO->Pitch", "Env->Filter", "Amp->Morph"],
        "amounts": [0.65, 0.55, 0.5],
        "desc_templates": [
            "ODYSSEY tidal drift modulates OWLFISH subharmonic pitch. Long journey, deep resonance.",
            "OWLFISH envelope swell opens ODYSSEY filter passage. Subharmonic tide.",
            "ODYSSEY amplitude shapes OWLFISH timbral morph. Drifting through the harmonic series.",
        ],
        "tags_extra": ["drift", "tidal", "journey"],
    },
    "OVERDUB": {
        "theme": "tape + harmonic series",
        "coupling_types": ["Amp->Filter", "Audio->Tape", "LFO->DelayTime"],
        "amounts": [0.5, 0.6, 0.45],
        "desc_templates": [
            "OWLFISH harmonic swell opens OVERDUB dub filter. Trautonium harmonics meet Caribbean tape.",
            "OVERDUB tape warmth processes OWLFISH subharmonic output. Analogue depth.",
            "OWLFISH LFO modulates OVERDUB delay time. Harmonic series in echo space.",
        ],
        "tags_extra": ["tape", "dub", "echo"],
    },
    "OBLONG": {
        "theme": "foundational + Trautonium bass",
        "coupling_types": ["Amp->Filter", "LFO->Pitch", "Env->Resonance"],
        "amounts": [0.65, 0.55, 0.6],
        "desc_templates": [
            "OBLONG foundational pulse modulates OWLFISH filter sweep. Foundation meets Trautonium.",
            "OWLFISH subharmonic pitch couples to OBLONG bass register. Double foundation.",
            "OBLONG envelope resonance shapes OWLFISH partial mix. Structural harmonic depth.",
        ],
        "tags_extra": ["foundational", "bass", "structural"],
    },
    "ORBITAL": {
        "theme": "group envelope + partials",
        "coupling_types": ["Env->Partial", "LFO->Filter", "Amp->Morph"],
        "amounts": [0.6, 0.55, 0.5],
        "desc_templates": [
            "ORBITAL group envelope sculpts OWLFISH partial balance. Orbital mechanics shape harmonic series.",
            "OWLFISH LFO warmth modulates ORBITAL filter pass. Trautonium in orbit.",
            "ORBITAL amplitude drives OWLFISH timbral morph. Gravitational partial coupling.",
        ],
        "tags_extra": ["orbital", "partials", "gravitational"],
    },
    "OUROBOROS": {
        "theme": "chaos + resonant series",
        "coupling_types": ["Chaos->Filter", "Amp->Resonance", "LFO->Pitch"],
        "amounts": [0.75, 0.65, 0.7],
        "desc_templates": [
            "OUROBOROS chaos signal drives OWLFISH filter instability. Serpent disrupts harmonic order.",
            "OWLFISH amplitude excites OUROBOROS resonant feedback. Harmonic series meets chaos loop.",
            "OUROBOROS LFO turbulence modulates OWLFISH subharmonic pitch. Abyssal chaos.",
        ],
        "tags_extra": ["chaos", "feedback", "turbulence"],
    },
    "OSPREY": {
        "theme": "coastal + deep water",
        "coupling_types": ["Amp->Filter", "LFO->Morph", "Env->Pitch"],
        "amounts": [0.5, 0.55, 0.45],
        "desc_templates": [
            "OSPREY coastal swell opens OWLFISH abyssal filter. Surface meets depth.",
            "OWLFISH LFO warmth shapes OSPREY timbral morph. Deep water coastal drift.",
            "OSPREY envelope dive modulates OWLFISH pitch descent. Osprey plunges to OWLFISH depths.",
        ],
        "tags_extra": ["coastal", "surface", "depth-contrast"],
    },
    "OCELOT": {
        "theme": "biome + spectral",
        "coupling_types": ["Env->Filter", "Amp->Morph", "LFO->Resonance"],
        "amounts": [0.5, 0.55, 0.45],
        "desc_templates": [
            "OCELOT biome envelope shapes OWLFISH spectral filter. Jungle and abyss.",
            "OWLFISH amplitude drives OCELOT morph crossfade. Trautonium spectral predator.",
            "OCELOT LFO stalks OWLFISH resonance. Biome hunting the harmonic series.",
        ],
        "tags_extra": ["biome", "spectral", "predator"],
    },
}

# ---------------------------------------------------------------------------
# OWLFISH default parameter stubs (owl_ prefix)
# ---------------------------------------------------------------------------
def owlfish_params(rng, variant_idx):
    """Generate OWLFISH parameter stub with light variation per variant."""
    base = {
        "owl_portamento":    round(rng.uniform(0.0, 0.1), 3),
        "owl_legatoMode":    rng.choice([True, False]),
        "owl_morphGlide":    round(rng.uniform(0.2, 0.5), 3),
        "owl_subMix":        round(rng.uniform(0.5, 0.85), 3),
        "owl_subDiv1":       0,
        "owl_subDiv2":       1,
        "owl_subDiv3":       2,
        "owl_subDiv4":       3,
        "owl_subLevel1":     round(rng.uniform(0.7, 1.0), 3),
        "owl_subLevel2":     round(rng.uniform(0.5, 0.8), 3),
        "owl_subLevel3":     round(rng.uniform(0.3, 0.6), 3),
        "owl_subLevel4":     round(rng.uniform(0.1, 0.4), 3),
        "owl_mixtur":        round(rng.uniform(0.3, 0.7), 3),
        "owl_fundWave":      rng.randint(0, 2),
        "owl_subWave":       rng.randint(0, 2),
        "owl_bodyFreq":      round(rng.uniform(60.0, 120.0), 1),
        "owl_bodyLevel":     round(rng.uniform(0.4, 0.8), 3),
        "owl_compRatio":     round(rng.uniform(1.5, 3.0), 2),
        "owl_compThreshold": round(rng.uniform(-18.0, -6.0), 1),
        "owl_compAttack":    round(rng.uniform(0.02, 0.1), 3),
    }
    return base


def partner_params_stub(partner_key, rng):
    """Minimal partner param stub — generic keys to signal intent."""
    prefixes = {
        "ORACLE":    "ora_",
        "OCEANIC":   "oce_",
        "OPAL":      "opl_",
        "OBSIDIAN":  "obs_",
        "ORGANON":   "org_",
        "ODYSSEY":   "ody_",
        "OVERDUB":   "dub_",
        "OBLONG":    "bob_",
        "ORBITAL":   "orb_",
        "OUROBOROS": "oub_",
        "OSPREY":    "osp_",
        "OCELOT":    "oct_",
    }
    pfx = prefixes.get(partner_key, "p_")
    return {
        f"{pfx}level":   round(rng.uniform(0.6, 0.9), 3),
        f"{pfx}attack":  round(rng.uniform(0.05, 0.4), 3),
        f"{pfx}release": round(rng.uniform(0.3, 1.2), 3),
        f"{pfx}filter":  round(rng.uniform(400.0, 3200.0), 1),
        f"{pfx}resonance": round(rng.uniform(0.1, 0.5), 3),
    }


# ---------------------------------------------------------------------------
# DNA blending
# ---------------------------------------------------------------------------
def blend_dna(partner_key, weight_owlfish=0.55):
    """Blend OWLFISH DNA with partner DNA, weighted toward OWLFISH."""
    p_dna = PARTNER_DNA[partner_key]
    w_a = weight_owlfish
    w_b = 1.0 - w_a
    blended = {}
    for k in OWLFISH_DNA:
        blended[k] = round(OWLFISH_DNA[k] * w_a + p_dna[k] * w_b, 3)
    return blended


# ---------------------------------------------------------------------------
# Preset construction
# ---------------------------------------------------------------------------
def make_preset(partner_key, variant_idx, rng, count):
    """Build a single coupling preset for OWLFISH + partner."""
    cfg = PAIR_CONFIGS[partner_key]

    # Name: pick a unique word from OWLFISH vocabulary + partner keyword
    word_idx = (variant_idx + list(PARTNER_DNA.keys()).index(partner_key)) % len(OWLFISH_WORDS)
    word = OWLFISH_WORDS[word_idx]
    partner_short = partner_key.capitalize()
    name = f"{word} {partner_short}"

    coupling_type = cfg["coupling_types"][variant_idx % len(cfg["coupling_types"])]
    amount = round(cfg["amounts"][variant_idx % len(cfg["amounts"])] + rng.uniform(-0.05, 0.05), 3)
    amount = max(0.1, min(1.0, amount))

    desc = cfg["desc_templates"][variant_idx % len(cfg["desc_templates"])]
    tags = ["coupling", "owlfish", partner_key.lower()] + cfg["tags_extra"]

    # Weight DNA toward OWLFISH on variant 0, blend evenly on 1, toward partner on 2
    weights = [0.65, 0.5, 0.4]
    dna = blend_dna(partner_key, weight_owlfish=weights[variant_idx % 3])

    engine_a_key = ENGINE_KEY["OWLFISH"]
    engine_b_key = ENGINE_KEY[partner_key]

    preset = {
        "schema_version": 1,
        "name": name,
        "mood": "Entangled",
        "engines": [engine_a_key, engine_b_key],
        "author": "XO_OX Designs",
        "version": "1.0.0",
        "description": desc,
        "tags": tags,
        "macroLabels": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
        "couplingIntensity": "Deep" if amount >= 0.65 else "Moderate",
        "tempo": None,
        "dna": dna,
        "parameters": {
            engine_a_key: owlfish_params(rng, variant_idx),
            engine_b_key: partner_params_stub(partner_key, rng),
        },
        "coupling": {
            "routes": [
                {
                    "source": engine_a_key,
                    "dest": engine_b_key,
                    "type": coupling_type,
                    "amount": amount,
                }
            ]
        },
        "sequencer": None,
    }
    return preset


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------
def parse_args():
    parser = argparse.ArgumentParser(
        description="Generate OWLFISH coupling presets for XOlokun."
    )
    default_out = pathlib.Path(__file__).parent.parent / "Presets" / "XOlokun" / "Entangled"
    parser.add_argument(
        "--output-dir",
        type=pathlib.Path,
        default=default_out,
        help="Directory to write .xometa files (default: Presets/XOlokun/Entangled/)",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print preset names and paths without writing files.",
    )
    parser.add_argument(
        "--seed",
        type=int,
        default=None,
        help="Random seed for reproducible output.",
    )
    parser.add_argument(
        "--count",
        type=int,
        default=3,
        help="Number of presets per engine pair (default: 3).",
    )
    return parser.parse_args()


def main():
    args = parse_args()
    rng = random.Random(args.seed)

    partners = list(PAIR_CONFIGS.keys())  # 12 partners
    total = len(partners) * args.count
    written = 0
    skipped = 0

    if not args.dry_run:
        args.output_dir.mkdir(parents=True, exist_ok=True)

    print(f"OWLFISH Coupling Pack — {len(partners)} pairs × {args.count} presets = {total} total")
    print(f"Output dir: {args.output_dir}")
    if args.dry_run:
        print("[DRY RUN — no files written]\n")

    for partner_key in partners:
        for variant_idx in range(args.count):
            preset = make_preset(partner_key, variant_idx, rng, args.count)
            filename = preset["name"].replace(" ", "_") + ".xometa"
            filepath = args.output_dir / filename

            if args.dry_run:
                print(f"  [DRY] {filename}  ({preset['coupling']['routes'][0]['type']}, amt={preset['coupling']['routes'][0]['amount']})")
                written += 1
                continue

            if filepath.exists():
                print(f"  [SKIP] {filename} (already exists)")
                skipped += 1
                continue

            with open(filepath, "w", encoding="utf-8") as f:
                json.dump(preset, f, indent=2)
                f.write("\n")

            print(f"  [OK]   {filename}")
            written += 1

    print(f"\nDone. {written} {'would be ' if args.dry_run else ''}written, {skipped} skipped.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
