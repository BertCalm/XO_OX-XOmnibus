#!/usr/bin/env python3
"""XPN Family Mood Expander — generate Family mood presets for XOmnibus.

Family is the 7th mood: curated multi-engine "family portrait" presets that
blend 2-4 engines in a unified sound. NOT coupling modulation routes — these
are about sonic cohabitation and shared character.

Usage:
    python Tools/xpn_family_mood_expander.py
    python Tools/xpn_family_mood_expander.py --count 5 --seed 42
    python Tools/xpn_family_mood_expander.py --dry-run
    python Tools/xpn_family_mood_expander.py --output-dir /tmp/family_test
"""

import argparse
import json
import random
from pathlib import Path

# ---------------------------------------------------------------------------
# Engine parameter prefix table (frozen — never rename after release)
# ---------------------------------------------------------------------------
ENGINE_PREFIX = {
    "OddfeliX":  "snap_",
    "OddOscar":  "morph_",
    "Overdub":   "dub_",
    "Odyssey":   "drift_",
    "Oblong":    "bob_",
    "Obese":     "fat_",
    "Onset":     "perc_",
    "Overworld": "ow_",
    "Opal":      "opal_",
    "Orbital":   "orb_",
    "Organon":   "organon_",
    "Ouroboros": "ouro_",
    "Obsidian":  "obsidian_",
    "Origami":   "origami_",
    "Oracle":    "oracle_",
    "Obscura":   "obscura_",
    "Oceanic":   "ocean_",
    "Ocelot":    "ocelot_",
    "Overbite":  "poss_",
    "Optic":     "optic_",
    "Oblique":   "oblq_",
    "Osprey":    "osprey_",
    "Osteria":   "osteria_",
    "Owlfish":   "owl_",
    "Ohm":       "ohm_",
    "Orphica":   "orph_",
    "Obbligato": "obbl_",
    "Ottoni":    "otto_",
    "Ole":       "ole_",
    "Ombre":     "ombre_",
    "Orca":      "orca_",
    "Octopus":   "octo_",
    "Overlap":   "olap_",
    "Outwit":    "owit_",
}

# ---------------------------------------------------------------------------
# The 10 Family portrait concepts
# ---------------------------------------------------------------------------
FAMILY_CONCEPTS = [
    {
        "id":       "the_core",
        "label":    "The Core",
        "engines":  ["OddfeliX", "OddOscar"],
        "story":    (
            "The founding siblings — feliX the neon tetra and Oscar the axolotl. "
            "Neon electric blue meets axolotl gill pink. Where digital crispness "
            "and organic morphology were born together, they still breathe as one."
        ),
        "variants": [
            {
                "suffix":     "Warm",
                "name":       "The Core Warm",
                "description": (
                    "feliX and Oscar at home — warm overtones, gentle movement. "
                    "The founding siblings in their most welcoming arrangement."
                ),
                "dna_bias": {"warmth": 0.78, "brightness": 0.55, "movement": 0.45,
                             "density": 0.5,  "space": 0.65,  "aggression": 0.22},
            },
            {
                "suffix":     "Portrait",
                "name":       "The Core Portrait",
                "description": (
                    "A neutral family portrait — feliX sharp and focused, "
                    "Oscar fluid beside them. The definitive sibling snapshot."
                ),
                "dna_bias": {"warmth": 0.62, "brightness": 0.62, "movement": 0.5,
                             "density": 0.52, "space": 0.6,  "aggression": 0.28},
            },
            {
                "suffix":     "Cool",
                "name":       "The Core Cool",
                "description": (
                    "The siblings in a cooler, more spectral register. "
                    "Crystalline feliX shimmer over slow axolotl morphology."
                ),
                "dna_bias": {"warmth": 0.6,  "brightness": 0.72, "movement": 0.55,
                             "density": 0.44, "space": 0.75, "aggression": 0.2},
            },
        ],
    },
    {
        "id":       "the_foundation",
        "label":    "The Foundation",
        "engines":  ["Oblong", "Onset", "Obese"],
        "story":    (
            "OBLONG, ONSET, and OBESE — the rhythm section. Bob lays the harmonic "
            "foundation, Onset drives the pulse, Obese saturates everything with "
            "hot-pink character. The trio that holds the whole XOmnibus together."
        ),
        "variants": [
            {
                "suffix":     "Warm",
                "name":       "Foundation Warm",
                "description": (
                    "The rhythm section in amber and hot-pink warmth — "
                    "tight, saturated, familiar."
                ),
                "dna_bias": {"warmth": 0.75, "brightness": 0.5,  "movement": 0.6,
                             "density": 0.72, "space": 0.42, "aggression": 0.38},
            },
            {
                "suffix":     "Groove",
                "name":       "Foundation Groove",
                "description": (
                    "Neutral groove mode — Oblong chords, Onset pockets, "
                    "Obese glue. The session-ready configuration."
                ),
                "dna_bias": {"warmth": 0.65, "brightness": 0.55, "movement": 0.68,
                             "density": 0.65, "space": 0.48, "aggression": 0.35},
            },
            {
                "suffix":     "Cool",
                "name":       "Foundation Cool",
                "description": (
                    "A cooler, more clinical foundation — Onset precise, "
                    "Obese restrained, Oblong spectral."
                ),
                "dna_bias": {"warmth": 0.61, "brightness": 0.62, "movement": 0.64,
                             "density": 0.6,  "space": 0.55, "aggression": 0.28},
            },
        ],
    },
    {
        "id":       "the_dreamers",
        "label":    "The Dreamers",
        "engines":  ["Odyssey", "Opal", "Oracle"],
        "story":    (
            "ODYSSEY, OPAL, and ORACLE — the evocative trio. Drift's wavetable "
            "wandering, Opal's granular clouds, Oracle's stochastic prophecy. "
            "Together they conjure sounds that exist only in the space between "
            "memory and imagination."
        ),
        "variants": [
            {
                "suffix":     "Warm",
                "name":       "Dreamers Warm",
                "description": (
                    "Drift and grain and prophecy in amber light — a warm dream "
                    "state where melody dissolves into texture."
                ),
                "dna_bias": {"warmth": 0.72, "brightness": 0.52, "movement": 0.65,
                             "density": 0.55, "space": 0.8,  "aggression": 0.15},
            },
            {
                "suffix":     "Drift",
                "name":       "Dreamers Drift",
                "description": (
                    "Balanced dreaming — Odyssey anchors, Opal scatters, "
                    "Oracle whispers breakpoints into the void."
                ),
                "dna_bias": {"warmth": 0.65, "brightness": 0.58, "movement": 0.7,
                             "density": 0.5,  "space": 0.82, "aggression": 0.18},
            },
            {
                "suffix":     "Cool",
                "name":       "Dreamers Cool",
                "description": (
                    "The trio in coolest form — glacial wavetables, sparse grain, "
                    "cold prophecy from a distant Oracle."
                ),
                "dna_bias": {"warmth": 0.62, "brightness": 0.68, "movement": 0.62,
                             "density": 0.42, "space": 0.88, "aggression": 0.12},
            },
        ],
    },
    {
        "id":       "the_architects",
        "label":    "The Architects",
        "engines":  ["Overworld", "Orbital", "Origami"],
        "story":    (
            "OVERWORLD, ORBITAL, and ORIGAMI — the structure builders. "
            "Chip-era geometry from OVERWORLD, ORBITAL's warm-red harmonic groups, "
            "ORIGAMI's precise vermillion folds. They build sound the way "
            "architects build spaces — with intention and silence."
        ),
        "variants": [
            {
                "suffix":     "Warm",
                "name":       "Architects Warm",
                "description": (
                    "Warm structural mass — OVERWORLD's NES warmth, ORBITAL bloom, "
                    "ORIGAMI creases catching amber light."
                ),
                "dna_bias": {"warmth": 0.7,  "brightness": 0.55, "movement": 0.48,
                             "density": 0.6,  "space": 0.62, "aggression": 0.28},
            },
            {
                "suffix":     "Blueprint",
                "name":       "Architects Blueprint",
                "description": (
                    "The neutral blueprint — each engine distinct in register, "
                    "the trio locked in harmonic architecture."
                ),
                "dna_bias": {"warmth": 0.62, "brightness": 0.62, "movement": 0.5,
                             "density": 0.58, "space": 0.65, "aggression": 0.25},
            },
            {
                "suffix":     "Cool",
                "name":       "Architects Cool",
                "description": (
                    "Cold geometry — chip precision, orbital distance, "
                    "origami edges sharp as code."
                ),
                "dna_bias": {"warmth": 0.61, "brightness": 0.72, "movement": 0.45,
                             "density": 0.52, "space": 0.72, "aggression": 0.22},
            },
        ],
    },
    {
        "id":       "the_depths",
        "label":    "The Depths",
        "engines":  ["Oceanic", "Owlfish", "Organon"],
        "story":    (
            "OCEANIC, OWLFISH, and ORGANON — the deep-water family. "
            "Phosphorescent teal, abyssal gold, bioluminescent cyan. "
            "Chromatophore modulation, Mixtur-Trautonium harmonics, "
            "and variational free-energy metabolism — three engines "
            "that evolved in the dark and glow on their own terms."
        ),
        "variants": [
            {
                "suffix":     "Warm",
                "name":       "The Depths Warm",
                "description": (
                    "Bioluminescent warmth from the mesopelagic zone — "
                    "deep but not cold, alive with metabolic light."
                ),
                "dna_bias": {"warmth": 0.68, "brightness": 0.48, "movement": 0.6,
                             "density": 0.62, "space": 0.78, "aggression": 0.2},
            },
            {
                "suffix":     "Abyssal",
                "name":       "The Depths Abyssal",
                "description": (
                    "The neutral deep — OCEANIC separates, OWLFISH sings in "
                    "subharmonics, ORGANON metabolizes the pressure."
                ),
                "dna_bias": {"warmth": 0.65, "brightness": 0.42, "movement": 0.55,
                             "density": 0.7,  "space": 0.82, "aggression": 0.22},
            },
            {
                "suffix":     "Cool",
                "name":       "The Depths Cool",
                "description": (
                    "The hadal zone — cold, crushing, crystalline. "
                    "Three instruments adapted to absolute darkness."
                ),
                "dna_bias": {"warmth": 0.62, "brightness": 0.38, "movement": 0.48,
                             "density": 0.75, "space": 0.88, "aggression": 0.25},
            },
        ],
    },
    {
        "id":       "the_coastline",
        "label":    "The Coastline",
        "engines":  ["Osprey", "Osteria", "Oceanic"],
        "story":    (
            "OSPREY, OSTERIA, and OCEANIC — the shore family. Azulejo blue, "
            "porto wine, phosphorescent teal. The ShoreSystem blessing (B012) "
            "connects OSPREY and OSTERIA through 5 coastline cultural datasets; "
            "OCEANIC provides the intertidal zone where land-breath meets water."
        ),
        "variants": [
            {
                "suffix":     "Warm",
                "name":       "Coastline Warm",
                "description": (
                    "Warm shore light — OSPREY's afternoon tide, OSTERIA's "
                    "harbor heat, OCEANIC shimmer on the surface."
                ),
                "dna_bias": {"warmth": 0.76, "brightness": 0.6,  "movement": 0.58,
                             "density": 0.52, "space": 0.68, "aggression": 0.2},
            },
            {
                "suffix":     "Tidal",
                "name":       "Coastline Tidal",
                "description": (
                    "The tide in balance — coastal instruments sharing "
                    "the ShoreSystem data, OCEANIC connecting them to open water."
                ),
                "dna_bias": {"warmth": 0.68, "brightness": 0.58, "movement": 0.62,
                             "density": 0.5,  "space": 0.72, "aggression": 0.18},
            },
            {
                "suffix":     "Cool",
                "name":       "Coastline Cool",
                "description": (
                    "Dawn shore — cool light, mist, OSPREY offshore, "
                    "OSTERIA shuttered, OCEANIC grey and wide."
                ),
                "dna_bias": {"warmth": 0.62, "brightness": 0.65, "movement": 0.55,
                             "density": 0.45, "space": 0.78, "aggression": 0.15},
            },
        ],
    },
    {
        "id":       "the_constellation",
        "label":    "The Constellation",
        "engines":  ["Ohm", "Orphica", "Obbligato", "Ottoni", "Ole"],
        "story":    (
            "OHM, ORPHICA, OBBLIGATO, OTTONI, and OLE — all five Constellation "
            "engines in one room. Hippy Dad jam, microsound harp, dual wind, "
            "triple brass, and Afro-Latin trio. The fastest-built family "
            "in XOmnibus history, now gathered for their first group portrait."
        ),
        "variants": [
            {
                "suffix":     "Warm",
                "name":       "Constellation Warm",
                "description": (
                    "The gathering — all five Constellation siblings in warm "
                    "acoustic register. Sage, seafoam, coral, patina, hibiscus."
                ),
                "dna_bias": {"warmth": 0.78, "brightness": 0.58, "movement": 0.62,
                             "density": 0.68, "space": 0.62, "aggression": 0.22},
            },
            {
                "suffix":     "Seance",
                "name":       "Constellation Seance",
                "description": (
                    "The five seances replayed as sound — each engine "
                    "channeling its founding ghost, all five at once."
                ),
                "dna_bias": {"warmth": 0.68, "brightness": 0.62, "movement": 0.65,
                             "density": 0.62, "space": 0.68, "aggression": 0.28},
            },
            {
                "suffix":     "Cool",
                "name":       "Constellation Cool",
                "description": (
                    "The Constellation at night — cool starfield, each voice "
                    "a distinct point of light in the shared sky."
                ),
                "dna_bias": {"warmth": 0.62, "brightness": 0.72, "movement": 0.55,
                             "density": 0.55, "space": 0.82, "aggression": 0.18},
            },
        ],
    },
    {
        "id":       "the_chaos_family",
        "label":    "The Chaos Family",
        "engines":  ["Ouroboros", "Oracle", "Outwit"],
        "story":    (
            "OUROBOROS, ORACLE, and OUTWIT — the stochastic siblings. Strange "
            "attractor red, prophecy indigo, octopus orange. Lorenz/Halvorsen "
            "chaos, GENDY stochastic synthesis, and Wolfram cellular automata. "
            "They are unpredictable alone; together they negotiate a shared randomness."
        ),
        "variants": [
            {
                "suffix":     "Warm",
                "name":       "Chaos Family Warm",
                "description": (
                    "The chaotic trio in a warmer, more hospitable mode — "
                    "the leash engaged, ORACLE constrained, OUTWIT patterned."
                ),
                "dna_bias": {"warmth": 0.68, "brightness": 0.52, "movement": 0.78,
                             "density": 0.58, "space": 0.65, "aggression": 0.35},
            },
            {
                "suffix":     "Entangled",
                "name":       "Chaos Family Entangled",
                "description": (
                    "Balanced chaos — three stochastic systems sharing "
                    "state. Order emerges at the edges of their interaction."
                ),
                "dna_bias": {"warmth": 0.65, "brightness": 0.58, "movement": 0.82,
                             "density": 0.6,  "space": 0.68, "aggression": 0.38},
            },
            {
                "suffix":     "Cool",
                "name":       "Chaos Family Cool",
                "description": (
                    "Cold deterministic chaos — strange attractors in a "
                    "crystalline lattice, ORACLE's prophecy reduced to pure math."
                ),
                "dna_bias": {"warmth": 0.62, "brightness": 0.65, "movement": 0.75,
                             "density": 0.55, "space": 0.75, "aggression": 0.32},
            },
        ],
    },
    {
        "id":       "the_tape_room",
        "label":    "The Tape Room",
        "engines":  ["Overdub", "Obscura", "Overbite"],
        "story":    (
            "OVERDUB, OBSCURA, and OVERBITE — the lo-fi family. Olive, "
            "daguerreotype silver, fang white. Spring reverb, stiff-string "
            "physical modeling, and opossum-energy bass character. "
            "Three instruments built for warmth, wear, and weight."
        ),
        "variants": [
            {
                "suffix":     "Warm",
                "name":       "Tape Room Warm",
                "description": (
                    "Deep tape warmth — OVERDUB's spring reverb, OBSCURA's "
                    "damp stiffness, OVERBITE's saturated low end."
                ),
                "dna_bias": {"warmth": 0.82, "brightness": 0.42, "movement": 0.45,
                             "density": 0.68, "space": 0.58, "aggression": 0.3},
            },
            {
                "suffix":     "Session",
                "name":       "Tape Room Session",
                "description": (
                    "The late-night session — three lo-fi engines locked in "
                    "the same room, trading character."
                ),
                "dna_bias": {"warmth": 0.75, "brightness": 0.48, "movement": 0.5,
                             "density": 0.65, "space": 0.55, "aggression": 0.32},
            },
            {
                "suffix":     "Cool",
                "name":       "Tape Room Cool",
                "description": (
                    "Morning-after cool — tape hiss settling, OBSCURA "
                    "spectral, OVERBITE restrained and coiled."
                ),
                "dna_bias": {"warmth": 0.65, "brightness": 0.55, "movement": 0.42,
                             "density": 0.6,  "space": 0.65, "aggression": 0.25},
            },
        ],
    },
    {
        "id":       "the_digital_tribe",
        "label":    "The Digital Tribe",
        "engines":  ["Optic", "Oblique", "Overworld"],
        "story":    (
            "OPTIC, OBLIQUE, and OVERWORLD — the electronic family. Phosphor "
            "green, prism violet, neon green. Visual synthesis meets prismatic "
            "bounce meets chip-era geometry. Three engines that live in the "
            "frequency ranges where digital began."
        ),
        "variants": [
            {
                "suffix":     "Warm",
                "name":       "Digital Tribe Warm",
                "description": (
                    "The tribe in a warmer, more nostalgic register — "
                    "OVERWORLD's NES amber, OBLIQUE's funk warmth, "
                    "OPTIC's phosphor glow."
                ),
                "dna_bias": {"warmth": 0.68, "brightness": 0.65, "movement": 0.62,
                             "density": 0.55, "space": 0.62, "aggression": 0.28},
            },
            {
                "suffix":     "Grid",
                "name":       "Digital Tribe Grid",
                "description": (
                    "The neutral grid — three electronic engines in precise "
                    "register, each distinct, all digital."
                ),
                "dna_bias": {"warmth": 0.62, "brightness": 0.72, "movement": 0.65,
                             "density": 0.5,  "space": 0.65, "aggression": 0.3},
            },
            {
                "suffix":     "Cool",
                "name":       "Digital Tribe Cool",
                "description": (
                    "Cold phosphor — the tribe at their most clinical. "
                    "OPTIC pulses, OBLIQUE prisms, OVERWORLD geometric and exact."
                ),
                "dna_bias": {"warmth": 0.61, "brightness": 0.78, "movement": 0.58,
                             "density": 0.48, "space": 0.72, "aggression": 0.25},
            },
        ],
    },
]

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def jitter(value: float, rng: random.Random, amount: float = 0.06) -> float:
    """Apply small random jitter while clamping to [0.0, 1.0]."""
    return round(max(0.0, min(1.0, value + rng.uniform(-amount, amount))), 3)


def build_dna(bias: dict, rng: random.Random) -> dict:
    """Construct 6D Sonic DNA from bias dict with light jitter."""
    defaults = {
        "brightness": 0.55,
        "warmth":     0.65,
        "movement":   0.55,
        "density":    0.55,
        "space":      0.65,
        "aggression": 0.28,
    }
    merged = {**defaults, **bias}
    return {k: jitter(merged[k], rng, 0.04) for k in
            ("brightness", "warmth", "movement", "density", "space", "aggression")}


def stub_parameters(engines, rng):
    """Generate a minimal parameter stub keyed by the first engine's prefix.

    Only the primary (first) engine gets stub parameters; others are present
    as empty dicts to signal they are loaded but at default values.
    """
    params: dict = {}
    for i, engine in enumerate(engines):
        prefix = ENGINE_PREFIX.get(engine, engine.lower()[:3] + "_")
        if i == 0:
            params[engine] = {
                f"{prefix}character": round(rng.uniform(0.4, 0.8), 3),
                f"{prefix}movement":  round(rng.uniform(0.3, 0.7), 3),
                f"{prefix}space":     round(rng.uniform(0.4, 0.8), 3),
            }
        else:
            params[engine] = {}
    return params


def build_tags(engines, extra=None):
    """Assemble tag list: family + multi-engine + engine names + extras."""
    tags = ["family", "multi-engine"]
    tags += [e.lower() for e in engines]
    if extra:
        tags += extra
    return tags


def make_preset(concept: dict, variant: dict, rng: random.Random) -> dict:
    engines  = concept["engines"]
    dna      = build_dna(variant["dna_bias"], rng)
    params   = stub_parameters(engines, rng)

    return {
        "schema_version": 1,
        "name":           variant["name"],
        "mood":           "Family",
        "engines":        engines,
        "author":         "XO_OX Designs",
        "version":        "1.0.0",
        "description":    f"{concept['story'].strip()} {variant['description'].strip()}",
        "tags":           build_tags(engines),
        "macroLabels":    ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
        "couplingIntensity": None,
        "tempo":          None,
        "dna":            dna,
        "parameters":     params,
        "coupling":       None,
        "sequencer":      None,
    }


def safe_filename(name: str) -> str:
    """Convert preset name to a safe filename."""
    return name.replace(" ", "_").replace("/", "-") + ".xometa"


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def parse_args() -> argparse.Namespace:
    repo_root = Path(__file__).resolve().parent.parent
    default_output = repo_root / "Presets" / "XOmnibus" / "Family"

    p = argparse.ArgumentParser(
        description="Generate Family mood presets for XOmnibus.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    p.add_argument(
        "--output-dir",
        type=Path,
        default=default_output,
        metavar="DIR",
        help=f"Output directory (default: {default_output})",
    )
    p.add_argument(
        "--dry-run",
        action="store_true",
        help="Print preset names without writing files.",
    )
    p.add_argument(
        "--count",
        type=int,
        default=3,
        metavar="N",
        help="Number of variants to generate per concept (default: 3, max: 3).",
    )
    p.add_argument(
        "--seed",
        type=int,
        default=None,
        metavar="INT",
        help="Random seed for reproducible output.",
    )
    return p.parse_args()


def main() -> None:
    args = parse_args()
    rng  = random.Random(args.seed)

    count = max(1, min(args.count, 3))  # cap at 3 — we define exactly 3 variants each

    written  = 0
    skipped  = 0
    previews = []

    if not args.dry_run:
        args.output_dir.mkdir(parents=True, exist_ok=True)

    for concept in FAMILY_CONCEPTS:
        variants = concept["variants"][:count]
        for variant in variants:
            preset   = make_preset(concept, variant, rng)
            filename = safe_filename(preset["name"])

            if args.dry_run:
                previews.append({
                    "name":    preset["name"],
                    "engines": preset["engines"],
                    "file":    filename,
                    "dna":     preset["dna"],
                })
            else:
                dest = args.output_dir / filename
                if dest.exists():
                    skipped += 1
                    continue
                with open(dest, "w", encoding="utf-8") as fh:
                    json.dump(preset, fh, indent=2, ensure_ascii=False)
                    fh.write("\n")
                written += 1
                print(f"  wrote  {dest.name}")

    if args.dry_run:
        print(f"\n[dry-run] Would generate {len(previews)} presets:\n")
        for p in previews:
            engines_str = " + ".join(p["engines"])
            dna = p["dna"]
            print(f"  {p['name']}")
            print(f"    engines : {engines_str}")
            print(f"    file    : {p['file']}")
            print(
                f"    dna     : "
                f"B={dna['brightness']:.2f}  W={dna['warmth']:.2f}  "
                f"M={dna['movement']:.2f}  D={dna['density']:.2f}  "
                f"S={dna['space']:.2f}  A={dna['aggression']:.2f}"
            )
            print()
    else:
        total = written + skipped
        print(
            f"\nFamily mood expander complete — "
            f"{written} written, {skipped} skipped (already exist), "
            f"{total} total attempted."
        )


if __name__ == "__main__":
    main()
