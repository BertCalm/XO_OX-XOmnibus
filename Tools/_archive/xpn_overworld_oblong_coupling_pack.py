#!/usr/bin/env python3
"""Generate OVERWORLD + OBLONG coupling expansion presets for XOmnibus.

Covers:
  - OVERWORLD × OBLONG marquee (4 presets)
  - OVERWORLD new pairs: OMBRE, ORCA, OCTOPUS, OVERLAP, OUTWIT, OBBLIGATO,
                          ORPHICA, OHM, OTTONI, OLE  (10 × 3 = 30)
  - OBLONG new pairs: OMBRE, ORCA, OCTOPUS, ORPHICA, OTTONI, ORGANON,
                       OSPREY, OSTERIA, OCEANIC  (9 × 3 = 27)
  Total: 61 presets, all Entangled mood.

CLI:
  python xpn_overworld_oblong_coupling_pack.py [--output-dir DIR] [--dry-run]
                                                [--seed N] [--count N]
"""

import argparse
import json
import os
import random
from pathlib import Path

# ---------------------------------------------------------------------------
# Sonic DNA baselines
# ---------------------------------------------------------------------------

DNA = {
    "OVERWORLD":  dict(brightness=0.75, warmth=0.40, movement=0.50, density=0.65, space=0.55, aggression=0.60),
    "OBLONG":     dict(brightness=0.60, warmth=0.65, movement=0.50, density=0.65, space=0.55, aggression=0.50),
    "OMBRE":      dict(brightness=0.45, warmth=0.60, movement=0.55, density=0.60, space=0.65, aggression=0.30),
    "ORCA":       dict(brightness=0.30, warmth=0.35, movement=0.70, density=0.70, space=0.50, aggression=0.75),
    "OCTOPUS":    dict(brightness=0.70, warmth=0.40, movement=0.85, density=0.60, space=0.50, aggression=0.55),
    "OVERLAP":    dict(brightness=0.65, warmth=0.55, movement=0.60, density=0.70, space=0.85, aggression=0.25),
    "OUTWIT":     dict(brightness=0.60, warmth=0.45, movement=0.80, density=0.65, space=0.55, aggression=0.50),
    "OBBLIGATO":  dict(brightness=0.55, warmth=0.65, movement=0.60, density=0.65, space=0.60, aggression=0.45),
    "ORPHICA":    dict(brightness=0.80, warmth=0.50, movement=0.70, density=0.45, space=0.75, aggression=0.25),
    "OHM":        dict(brightness=0.45, warmth=0.75, movement=0.55, density=0.60, space=0.70, aggression=0.30),
    "OTTONI":     dict(brightness=0.60, warmth=0.60, movement=0.50, density=0.70, space=0.55, aggression=0.60),
    "OLE":        dict(brightness=0.65, warmth=0.70, movement=0.75, density=0.60, space=0.60, aggression=0.55),
    "ORGANON":    dict(brightness=0.60, warmth=0.60, movement=0.65, density=0.75, space=0.65, aggression=0.40),
    "OSPREY":     dict(brightness=0.55, warmth=0.55, movement=0.50, density=0.60, space=0.70, aggression=0.35),
    "OSTERIA":    dict(brightness=0.40, warmth=0.75, movement=0.45, density=0.70, space=0.60, aggression=0.45),
    "OCEANIC":    dict(brightness=0.65, warmth=0.55, movement=0.50, density=0.55, space=0.75, aggression=0.30),
}

# Engine display names → engine key used in .engines array
ENGINE_NAMES = {
    "OVERWORLD":  "XOverworld",
    "OBLONG":     "XOblongBob",
    "OMBRE":      "XOmbre",
    "ORCA":       "XOrca",
    "OCTOPUS":    "XOctopus",
    "OVERLAP":    "XOverlap",
    "OUTWIT":     "XOutwit",
    "OBBLIGATO":  "XObbligato",
    "ORPHICA":    "XOrphica",
    "OHM":        "XOhm",
    "OTTONI":     "XOttoni",
    "OLE":        "XOlé",
    "ORGANON":    "XOrganon",
    "OSPREY":     "XOsprey",
    "OSTERIA":    "XOsteria",
    "OCEANIC":    "XOceanic",
}

# Param prefixes
PREFIXES = {
    "OVERWORLD": "ow_",
    "OBLONG":    "bob_",
    "OMBRE":     "ombr_",
    "ORCA":      "orca_",
    "OCTOPUS":   "oct_",
    "OVERLAP":   "olap_",
    "OUTWIT":    "owit_",
    "OBBLIGATO": "obbl_",
    "ORPHICA":   "orph_",
    "OHM":       "ohm_",
    "OTTONI":    "otto_",
    "OLE":       "ole_",
    "ORGANON":   "org_",
    "OSPREY":    "ospy_",
    "OSTERIA":   "ost_",
    "OCEANIC":   "ocn_",
}

# Vocabulary pools for name generation
OW_VOCAB  = ["Chip", "NES", "ERA", "Neon", "Pixel", "Retro", "Genesis", "Grid"]
BOB_VOCAB = ["Foundation", "Amber", "Anchor", "Bottom", "Grounding", "Core", "Warm Bass", "Brick"]

# Marquee names (OVERWORLD × OBLONG)
MARQUEE_NAMES = ["NES Foundation", "Chip Amber", "Neon Bottom", "Pixel Anchor"]

# Coupling types cycled across pairs
COUPLING_TYPES = [
    "Env->Filter",
    "Amp->Morph",
    "LFO->Pitch",
    "Audio->Wavetable",
    "Env->Morph",
    "Amp->Filter",
    "LFO->Amp",
    "Pitch->Env",
    "Filter->Amp",
    "LFO->Filter",
]

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def blend_dna(key_a: str, key_b: str, weight: float = 0.5) -> dict:
    """Blend two DNA dicts; weight 0 = all A, 1 = all B."""
    a, b = DNA[key_a], DNA[key_b]
    return {
        k: round(a[k] * (1 - weight) + b[k] * weight, 3)
        for k in a
    }


def jitter(val: float, rng: random.Random, scale: float = 0.06) -> float:
    return round(max(0.0, min(1.0, val + rng.uniform(-scale, scale))), 3)


def jitter_dna(dna: dict, rng: random.Random) -> dict:
    return {k: jitter(v, rng) for k, v in dna.items()}


def coupling_intensity(amount: float) -> str:
    if amount >= 0.75:
        return "Deep"
    if amount >= 0.5:
        return "Moderate"
    return "Light"


def stub_params(engine_key: str, dna: dict, rng: random.Random) -> dict:
    """Generate minimal stub parameters for one engine."""
    pfx = PREFIXES[engine_key]
    return {
        f"{pfx}outputLevel":    round(rng.uniform(0.72, 0.88), 3),
        f"{pfx}outputPan":      round(rng.uniform(-0.08, 0.08), 3),
        f"{pfx}couplingLevel":  round(rng.uniform(0.55, 0.85), 3),
        f"{pfx}couplingBus":    rng.randint(0, 3),
        f"{pfx}filterCutoff":   round(0.3 + dna["brightness"] * 0.5, 3),
        f"{pfx}filterReso":     round(rng.uniform(0.15, 0.45), 3),
        f"{pfx}ampAttack":      round(rng.uniform(5, 30), 1),
        f"{pfx}ampDecay":       round(rng.uniform(150, 600), 1),
        f"{pfx}ampSustain":     round(rng.uniform(0.55, 0.9), 3),
        f"{pfx}ampRelease":     round(rng.uniform(300, 900), 1),
        f"{pfx}reverbMix":      round(dna["space"] * 0.6, 3),
        f"{pfx}reverbSize":     round(0.3 + dna["space"] * 0.4, 3),
    }


def make_preset(
    name: str,
    engine_a: str,
    engine_b: str,
    coupling_type: str,
    coupling_amount: float,
    dna: dict,
    desc: str,
    tags: list[str],
    rng: random.Random,
) -> dict:
    eng_a_name = ENGINE_NAMES[engine_a]
    eng_b_name = ENGINE_NAMES[engine_b]
    params = {}
    params[eng_a_name] = stub_params(engine_a, dna, rng)
    params[eng_b_name] = stub_params(engine_b, dna, rng)

    return {
        "schema_version": 1,
        "name": name,
        "mood": "Entangled",
        "engines": [eng_a_name, eng_b_name],
        "author": "XO_OX",
        "version": "1.0.0",
        "description": desc,
        "tags": ["coupling", "entangled"] + tags,
        "macroLabels": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
        "couplingIntensity": coupling_intensity(coupling_amount),
        "dna": dna,
        "sonic_dna": dna,
        "parameters": params,
        "coupling": {
            "type": coupling_type,
            "pairs": [
                {
                    "engineA": eng_a_name,
                    "engineB": eng_b_name,
                    "type": coupling_type,
                    "amount": round(coupling_amount, 3),
                }
            ],
        },
    }


# ---------------------------------------------------------------------------
# Preset catalogue definitions
# ---------------------------------------------------------------------------

def build_marquee_specs() -> list[dict]:
    """4 OVERWORLD × OBLONG marquee presets."""
    return [
        dict(
            name="NES Foundation",
            engine_a="OVERWORLD", engine_b="OBLONG",
            coupling_type="Amp->Filter",
            coupling_amount=0.78,
            weight=0.45,
            desc="Chip NES grid locked to Oblong amber foundation. Retro pixel meets warm brick.",
            tags=["overworld", "oblong", "chip", "foundation", "nes", "amber"],
        ),
        dict(
            name="Chip Amber",
            engine_a="OVERWORLD", engine_b="OBLONG",
            coupling_type="Env->Morph",
            coupling_amount=0.65,
            weight=0.55,
            desc="Overworld ERA triangle drives Oblong morph. Neon warmth fused to amber grounding.",
            tags=["overworld", "oblong", "chip", "amber", "era", "neon"],
        ),
        dict(
            name="Neon Bottom",
            engine_a="OVERWORLD", engine_b="OBLONG",
            coupling_type="LFO->Filter",
            coupling_amount=0.72,
            weight=0.4,
            desc="Overworld neon LFO sweeps Oblong bottom-end filter. Pixel grid over warm anchor.",
            tags=["overworld", "oblong", "neon", "bottom", "pixel", "anchor"],
        ),
        dict(
            name="Pixel Anchor",
            engine_a="OVERWORLD", engine_b="OBLONG",
            coupling_type="Pitch->Env",
            coupling_amount=0.60,
            weight=0.5,
            desc="Genesis pixel pitch quantised to Oblong anchor envelope. Grounding the grid.",
            tags=["overworld", "oblong", "pixel", "anchor", "genesis", "core"],
        ),
    ]


# (engine_b_key, name_template_index, extra_tags)
OVERWORLD_PAIRS = [
    ("OMBRE",     ["Chip Fade",      "Neon Gradient",   "Pixel Ombre"  ], ["chip", "ombre", "gradient"]),
    ("ORCA",      ["Grid Predator",  "Chip Strike",     "Neon Orca"    ], ["chip", "orca", "aggression"]),
    ("OCTOPUS",   ["Pixel Arms",     "Retro Tangle",    "NES Octopus"  ], ["chip", "octopus", "movement"]),
    ("OVERLAP",   ["ERA Shimmer",    "Chip Space",      "Neon Overlap" ], ["chip", "overlap", "space"]),
    ("OUTWIT",    ["Grid Trickster", "Pixel Wit",       "Retro Outwit" ], ["chip", "outwit", "movement"]),
    ("OBBLIGATO", ["NES Obligate",   "Chip Bond",       "ERA Duet"     ], ["chip", "obbligato", "harmonic"]),
    ("ORPHICA",   ["Pixel Harp",     "Neon Orphica",    "Retro Pluck"  ], ["chip", "orphica", "bright"]),
    ("OHM",       ["Chip Commune",   "NES Resonance",   "ERA Current"  ], ["chip", "ohm", "warm"]),
    ("OTTONI",    ["Pixel Brass",    "Retro Fanfare",   "Grid Ottoni"  ], ["chip", "ottoni", "brass"]),
    ("OLE",       ["Neon Drama",     "Chip Fiesta",     "ERA Ole"      ], ["chip", "ole", "rhythm"]),
]

OBLONG_PAIRS = [
    ("OMBRE",   ["Amber Gradient", "Warm Fade",       "Foundation Ombre" ], ["oblong", "ombre", "warm"]),
    ("ORCA",    ["Amber Strike",   "Bottom Predator", "Brick Orca"        ], ["oblong", "orca", "depth"]),
    ("OCTOPUS", ["Core Tangle",    "Anchor Arms",     "Warm Octopus"      ], ["oblong", "octopus", "movement"]),
    ("ORPHICA", ["Amber Harp",     "Foundation Pluck","Warm Orphica"      ], ["oblong", "orphica", "harmonic"]),
    ("OTTONI",  ["Brick Brass",    "Warm Fanfare",    "Bottom Ottoni"     ], ["oblong", "ottoni", "brass"]),
    ("ORGANON", ["Core Organ",     "Foundation Flow", "Amber Organon"     ], ["oblong", "organon", "texture"]),
    ("OSPREY",  ["Grounding Dive", "Anchor Osprey",   "Warm Raptor"       ], ["oblong", "osprey", "space"]),
    ("OSTERIA", ["Brick Kitchen",  "Warm Osteria",    "Foundation Table"  ], ["oblong", "osteria", "warmth"]),
    ("OCEANIC", ["Amber Depth",    "Core Oceanic",    "Bottom Current"    ], ["oblong", "oceanic", "depth"]),
]


def generate_all_specs(count: int) -> list[dict]:
    """Return all preset specs (marquee + pairs), limiting pair names to `count` per pair."""
    specs = []

    # Marquee
    for m in build_marquee_specs():
        specs.append(m)

    # OVERWORLD pairs
    for (partner, names, tags) in OVERWORLD_PAIRS:
        used_names = names[:count]
        for i, name in enumerate(used_names):
            weight = 0.35 + i * 0.1
            ct_idx = (OVERWORLD_PAIRS.index((partner, names, tags)) * 3 + i) % len(COUPLING_TYPES)
            amount = 0.5 + (i % 3) * 0.12
            specs.append(dict(
                name=name,
                engine_a="OVERWORLD", engine_b=partner,
                coupling_type=COUPLING_TYPES[ct_idx],
                coupling_amount=amount,
                weight=weight,
                desc=f"Overworld {OW_VOCAB[ct_idx % len(OW_VOCAB)]} character entangled with {partner.title()} texture.",
                tags=["overworld", partner.lower()] + tags,
            ))

    # OBLONG pairs
    for (partner, names, tags) in OBLONG_PAIRS:
        used_names = names[:count]
        for i, name in enumerate(used_names):
            weight = 0.45 + i * 0.08
            ct_idx = (OBLONG_PAIRS.index((partner, names, tags)) * 3 + i + 5) % len(COUPLING_TYPES)
            amount = 0.52 + (i % 3) * 0.10
            specs.append(dict(
                name=name,
                engine_a="OBLONG", engine_b=partner,
                coupling_type=COUPLING_TYPES[ct_idx],
                coupling_amount=amount,
                weight=weight,
                desc=f"Oblong {BOB_VOCAB[ct_idx % len(BOB_VOCAB)]} grounding paired with {partner.title()} character.",
                tags=["oblong", partner.lower()] + tags,
            ))

    return specs


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main() -> None:
    repo_root = Path(__file__).resolve().parent.parent
    default_out = repo_root / "Presets" / "XOmnibus" / "Entangled"

    parser = argparse.ArgumentParser(
        description="Generate OVERWORLD + OBLONG coupling expansion presets."
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=default_out,
        help=f"Directory to write .xometa files (default: {default_out})",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print preset names without writing files.",
    )
    parser.add_argument(
        "--seed",
        type=int,
        default=42,
        help="Random seed for reproducibility (default: 42).",
    )
    parser.add_argument(
        "--count",
        type=int,
        default=3,
        choices=range(1, 4),
        metavar="N",
        help="Number of presets per engine pair (1-3, default: 3).",
    )
    args = parser.parse_args()

    rng = random.Random(args.seed)
    specs = generate_all_specs(args.count)

    written = 0
    skipped = 0

    for spec in specs:
        ea, eb = spec["engine_a"], spec["engine_b"]
        weight = spec.get("weight", 0.5)
        dna_blended = blend_dna(ea, eb, weight)
        dna_jittered = jitter_dna(dna_blended, rng)

        preset = make_preset(
            name=spec["name"],
            engine_a=ea,
            engine_b=eb,
            coupling_type=spec["coupling_type"],
            coupling_amount=spec["coupling_amount"],
            dna=dna_jittered,
            desc=spec["desc"],
            tags=spec["tags"],
            rng=rng,
        )

        filename = spec["name"].replace("/", "-") + ".xometa"
        out_path = args.output_dir / filename

        if args.dry_run:
            print(f"[DRY RUN] {out_path}")
            written += 1
            continue

        args.output_dir.mkdir(parents=True, exist_ok=True)

        if out_path.exists():
            skipped += 1
            print(f"  skip  {filename}  (exists)")
            continue

        with open(out_path, "w", encoding="utf-8") as f:
            json.dump(preset, f, indent=2)
            f.write("\n")

        written += 1
        print(f"  write {filename}")

    total = written + skipped
    action = "Would write" if args.dry_run else "Wrote"
    print(f"\n{action} {written} preset(s), skipped {skipped} existing — {total} total.")


if __name__ == "__main__":
    main()
