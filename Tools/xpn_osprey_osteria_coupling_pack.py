#!/usr/bin/env python3
"""Generate OSPREY + OSTERIA coupling presets for XOmnibus.

Seeds coupling coverage for the sister shore engines (ShoreSystem B012 blessing).
Produces 41 Entangled-mood .xometa presets across 13 engine pairs.

Usage:
    python Tools/xpn_osprey_osteria_coupling_pack.py
    python Tools/xpn_osprey_osteria_coupling_pack.py --dry-run
    python Tools/xpn_osprey_osteria_coupling_pack.py --count 5 --seed 42
    python Tools/xpn_osprey_osteria_coupling_pack.py --output-dir /tmp/test_presets
"""

import argparse
import json
import random
from pathlib import Path

# ---------------------------------------------------------------------------
# DNA baselines
# ---------------------------------------------------------------------------

DNA = {
    "Osprey":   {"brightness": 0.55, "warmth": 0.55, "movement": 0.50, "density": 0.60, "space": 0.70, "aggression": 0.35},
    "Osteria":  {"brightness": 0.40, "warmth": 0.75, "movement": 0.45, "density": 0.70, "space": 0.60, "aggression": 0.45},
    "Oceanic":  {"brightness": 0.65, "warmth": 0.55, "movement": 0.50, "density": 0.55, "space": 0.75, "aggression": 0.30},
    "Overdub":  {"brightness": 0.45, "warmth": 0.70, "movement": 0.55, "density": 0.55, "space": 0.75, "aggression": 0.25},
    "Opal":     {"brightness": 0.70, "warmth": 0.50, "movement": 0.75, "density": 0.45, "space": 0.80, "aggression": 0.20},
    "Oracle":   {"brightness": 0.50, "warmth": 0.40, "movement": 0.60, "density": 0.70, "space": 0.70, "aggression": 0.40},
    "Organon":  {"brightness": 0.60, "warmth": 0.60, "movement": 0.65, "density": 0.75, "space": 0.65, "aggression": 0.40},
    "Oblong":   {"brightness": 0.60, "warmth": 0.65, "movement": 0.50, "density": 0.65, "space": 0.55, "aggression": 0.50},
    "Orbital":  {"brightness": 0.60, "warmth": 0.65, "movement": 0.55, "density": 0.60, "space": 0.60, "aggression": 0.50},
    "Owlfish":  {"brightness": 0.40, "warmth": 0.60, "movement": 0.40, "density": 0.65, "space": 0.80, "aggression": 0.20},
}

COUPLING_TYPES = ["SYNC_LFO", "FILTER_MOD", "AMP_MOD", "FORMANT_CROSS"]

# ---------------------------------------------------------------------------
# Name vocabulary
# ---------------------------------------------------------------------------

OSPREY_WORDS = ["Shore", "Coastal", "Tide", "Azure", "Migrate", "Flock", "Dive", "Glide", "Wingspan", "Azulejo"]
OSTERIA_WORDS = ["Harbor", "Port", "Wine Dark", "Quayside", "Mooring", "Cellar", "Vintage", "Anchor", "Cask", "Porto"]
MARQUEE_NAMES = [
    "The Shore and Harbor",
    "Coastal Meeting",
    "Azure Porto",
    "Shore Wine",
    "Tide and Cask",
]

# Per-engine adjective pools used to compose secondary-pair names
ENGINE_WORDS = {
    "Oceanic":  ["Pelagic", "Brine", "Current", "Phosphor", "Drift", "Tidal"],
    "Overdub":  ["Tape", "Echo", "Dub", "Reel", "Saturate", "Delay"],
    "Opal":     ["Granular", "Shimmer", "Glass", "Prismatic", "Scatter", "Float"],
    "Oracle":   ["Stochastic", "Prophecy", "Maqam", "Fractal", "Vision", "Omen"],
    "Organon":  ["Metabolic", "Pulse", "Organ", "Breath", "Tissue", "Flux"],
    "Oblong":   ["Amber", "Grit", "Warm", "Bass", "Weight", "Ground"],
    "Orbital":  ["Spin", "Orbit", "Circuit", "Loop", "Radial", "Revolve"],
    "Owlfish":  ["Abyssal", "Mixtur", "Gold", "Deep", "Resonate", "Murk"],
}

# Parameter prefix map (engine ID → prefix)
PREFIX = {
    "Osprey":  "osprey_",
    "Osteria": "osteria_",
    "Oceanic": "ocean_",
    "Overdub": "dub_",
    "Opal":    "opal_",
    "Oracle":  "oracle_",
    "Organon": "organon_",
    "Oblong":  "bob_",
    "Orbital": "orb_",
    "Owlfish": "owl_",
}

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def blend_dna(engine_a: str, engine_b: str, rng: random.Random) -> dict:
    """Blend two engine DNA baselines with a small random nudge."""
    a = DNA[engine_a]
    b = DNA[engine_b]
    result = {}
    for k in a:
        mid = (a[k] + b[k]) / 2.0
        nudge = rng.uniform(-0.05, 0.05)
        result[k] = round(min(1.0, max(0.0, mid + nudge)), 3)
    return result


def coupling_amount(rng: random.Random) -> float:
    return round(rng.uniform(0.45, 0.85), 2)


def make_preset(
    name: str,
    engine_a: str,
    engine_b: str,
    coupling_type: str,
    amount: float,
    dna: dict,
    description: str,
    tags: list,
) -> dict:
    intensity = "Deep" if amount >= 0.7 else "Moderate"
    return {
        "schema_version": 1,
        "name": name,
        "mood": "Entangled",
        "engines": [engine_a, engine_b],
        "author": "XO_OX Designs",
        "version": "1.0.0",
        "description": description,
        "tags": ["coupling"] + tags,
        "macroLabels": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
        "couplingIntensity": intensity,
        "tempo": None,
        "dna": dna,
        "parameters": {
            f"{PREFIX[engine_a]}shoreBlend": 0.5,
            f"{PREFIX[engine_b]}shoreBlend": 0.5,
        },
        "coupling": {
            "pairs": [
                {
                    "engineA": engine_a,
                    "engineB": engine_b,
                    "type": coupling_type,
                    "amount": amount,
                }
            ]
        },
        "sequencer": None,
    }


# ---------------------------------------------------------------------------
# Preset definitions
# ---------------------------------------------------------------------------

def build_preset_specs(rng: random.Random) -> list:
    """Return list of (name, engine_a, engine_b, coupling_type, description, tags)."""
    specs = []

    # ---- 5 OSPREY + OSTERIA marquee presets ----
    marquee_couplings = rng.sample(COUPLING_TYPES, 4) + rng.sample(COUPLING_TYPES, 1)
    for i, preset_name in enumerate(MARQUEE_NAMES):
        ctype = marquee_couplings[i % len(marquee_couplings)]
        desc_map = {
            "The Shore and Harbor": "Sister shore engines in full ShoreSystem resonance — coastline culture meets port culture.",
            "Coastal Meeting":      "Azure shoreline and porto harbor converge on the breakwater between open sea and docked vessels.",
            "Azure Porto":          "Azulejo-tiled façades echo the porto wine cellars — blue meets burgundy at the water's edge.",
            "Shore Wine":           "Salt-air tides breathe through barrel-aged harmonics; the shore pours into the cask.",
            "Tide and Cask":        "Tidal rhythm synchronizes with the slow fermentation pulse of a harbor cellar.",
        }
        specs.append((
            preset_name,
            "Osprey", "Osteria",
            ctype,
            desc_map[preset_name],
            ["osprey", "osteria", "shore", "harbor"],
        ))

    # ---- OSPREY secondary pairs (6 engines × 3 presets = 18) ----
    osprey_partners = ["Oceanic", "Overdub", "Opal", "Oracle", "Organon", "Oblong"]
    for partner in osprey_partners:
        osp_words = rng.sample(OSPREY_WORDS, 3)
        eng_words = rng.sample(ENGINE_WORDS.get(partner, ["Wave", "Tone", "Signal"]), 3)
        for i in range(3):
            ctype = rng.choice(COUPLING_TYPES)
            name = f"{osp_words[i]} {eng_words[i]}"
            desc = (
                f"Osprey shore voice coupled to {partner} via {ctype.replace('_', ' ').title()}. "
                f"Coastal character meets {partner.lower()} texture."
            )
            specs.append((
                name,
                "Osprey", partner,
                ctype,
                desc,
                ["osprey", partner.lower(), "shore"],
            ))

    # ---- OSTERIA secondary pairs (6 engines × 3 presets = 18) ----
    osteria_partners = ["Oceanic", "Overdub", "Oblong", "Oracle", "Orbital", "Owlfish"]
    for partner in osteria_partners:
        ost_words = rng.sample(OSTERIA_WORDS, 3)
        eng_words = rng.sample(ENGINE_WORDS.get(partner, ["Wave", "Tone", "Signal"]), 3)
        for i in range(3):
            ctype = rng.choice(COUPLING_TYPES)
            name = f"{ost_words[i]} {eng_words[i]}"
            desc = (
                f"Osteria harbor voice coupled to {partner} via {ctype.replace('_', ' ').title()}. "
                f"Porto wine warmth meets {partner.lower()} character."
            )
            specs.append((
                name,
                "Osteria", partner,
                ctype,
                desc,
                ["osteria", partner.lower(), "harbor"],
            ))

    return specs


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description="Generate OSPREY + OSTERIA coupling presets for XOmnibus."
    )
    parser.add_argument(
        "--output-dir",
        default=None,
        help="Output directory (default: Presets/XOmnibus/Entangled/ relative to repo root).",
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
        help="Random seed for reproducibility.",
    )
    parser.add_argument(
        "--count",
        type=int,
        default=3,
        help="Presets per secondary engine pair (default: 3). Marquee pair always gets 5.",
    )
    args = parser.parse_args()

    rng = random.Random(args.seed)

    # Resolve output directory
    if args.output_dir:
        output_dir = Path(args.output_dir)
    else:
        repo_root = Path(__file__).parent.parent
        output_dir = repo_root / "Presets" / "XOmnibus" / "Entangled"

    if not args.dry_run:
        output_dir.mkdir(parents=True, exist_ok=True)

    # Build specs (always use count=3 for secondary; marquee is always 5)
    # If --count differs from 3, regenerate secondary pairs with adjusted count
    specs_full = build_preset_specs(rng)

    # The first 5 are marquee; rest are secondary pairs in groups of 3
    marquee_specs = specs_full[:5]
    secondary_specs = specs_full[5:]

    # Trim or extend secondary specs based on --count
    if args.count != 3:
        rng2 = random.Random(args.seed)
        secondary_specs_adjusted = []
        osprey_partners = ["Oceanic", "Overdub", "Opal", "Oracle", "Organon", "Oblong"]
        osteria_partners = ["Oceanic", "Overdub", "Oblong", "Oracle", "Orbital", "Owlfish"]
        for partners, anchor_engine, vocab in [
            (osprey_partners, "Osprey", OSPREY_WORDS),
            (osteria_partners, "Osteria", OSTERIA_WORDS),
        ]:
            for partner in partners:
                anch_words = rng2.sample(vocab, max(args.count, 1))
                eng_words = rng2.sample(ENGINE_WORDS.get(partner, ["Wave", "Tone", "Signal"]), max(args.count, 1))
                for i in range(args.count):
                    ctype = rng2.choice(COUPLING_TYPES)
                    anch_w = anch_words[i % len(anch_words)]
                    eng_w = eng_words[i % len(eng_words)]
                    name = f"{anch_w} {eng_w}"
                    other_name = anchor_engine.lower()
                    desc = (
                        f"{anchor_engine} shore voice coupled to {partner} via "
                        f"{ctype.replace('_', ' ').title()}. "
                        f"{'Coastal character' if anchor_engine == 'Osprey' else 'Porto wine warmth'} "
                        f"meets {partner.lower()} texture."
                    )
                    secondary_specs_adjusted.append((
                        name,
                        anchor_engine, partner,
                        ctype,
                        desc,
                        [other_name, partner.lower()],
                    ))
        secondary_specs = secondary_specs_adjusted

    all_specs = marquee_specs + secondary_specs

    # Write (or dry-run) presets
    written = 0
    skipped = 0
    for spec in all_specs:
        name, engine_a, engine_b, ctype, desc, tags = spec
        amount = coupling_amount(rng)
        dna = blend_dna(engine_a, engine_b, rng)
        preset = make_preset(name, engine_a, engine_b, ctype, amount, dna, desc, tags)

        filename = name + ".xometa"
        filepath = output_dir / filename

        if args.dry_run:
            print(f"[DRY RUN] {filepath}")
        else:
            if filepath.exists():
                print(f"  skip (exists): {filename}")
                skipped += 1
                continue
            with open(filepath, "w") as f:
                json.dump(preset, f, indent=2)
                f.write("\n")
            print(f"  wrote: {filename}")
        written += 1

    total = len(all_specs)
    if args.dry_run:
        print(f"\nDry run complete — {total} presets would be generated.")
    else:
        print(f"\nDone — {written} written, {skipped} skipped (already exist). Total planned: {total}.")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
