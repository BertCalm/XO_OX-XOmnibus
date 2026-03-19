#!/usr/bin/env python3
"""Generate coupling presets for OBSIDIAN + OBSCURA and their engine pairs.

OBSIDIAN (Crystal White #E8E0D8) — crystal/mineral engine
OBSCURA  (Daguerreotype Silver #8A9BA8) — early photography/daguerreotype stiffness engine

Preset breakdown:
  OBSIDIAN + OBSCURA (marquee pair): 5 presets
  OBSIDIAN pairs (ORACLE/OWLFISH/OPAL/ORGANON/ORIGAMI/ORBITAL/OCEANIC/OPTIC): 8 × 3 = 24
  OBSCURA  pairs (ORACLE/ORGANON/OCEANIC/ORBITAL/ODYSSEY/OWLFISH/OVERDUB/OCELOT): 8 × 3 = 24
  Total: 53 presets
"""

import argparse
import json
import os
import random
from pathlib import Path

# ---------------------------------------------------------------------------
# Sonic DNA baselines
# ---------------------------------------------------------------------------
DNA_BASELINES = {
    "OBSIDIAN": dict(brightness=0.9, warmth=0.2, movement=0.3, density=0.5,  space=0.6,  aggression=0.1),
    "OBSCURA":  dict(brightness=0.4, warmth=0.6, movement=0.35,density=0.6,  space=0.65, aggression=0.2),
    "ORACLE":   dict(brightness=0.5, warmth=0.4, movement=0.6, density=0.7,  space=0.7,  aggression=0.4),
    "OWLFISH":  dict(brightness=0.4, warmth=0.6, movement=0.4, density=0.65, space=0.8,  aggression=0.2),
    "OPAL":     dict(brightness=0.7, warmth=0.5, movement=0.75,density=0.45, space=0.8,  aggression=0.2),
    "ORGANON":  dict(brightness=0.6, warmth=0.6, movement=0.65,density=0.75, space=0.65, aggression=0.4),
    "ORIGAMI":  dict(brightness=0.7, warmth=0.45,movement=0.65,density=0.55, space=0.55, aggression=0.55),
    "ORBITAL":  dict(brightness=0.6, warmth=0.65,movement=0.55,density=0.6,  space=0.6,  aggression=0.5),
    "OCEANIC":  dict(brightness=0.65,warmth=0.55,movement=0.5, density=0.55, space=0.75, aggression=0.3),
    "OPTIC":    dict(brightness=0.85,warmth=0.3, movement=0.9, density=0.4,  space=0.5,  aggression=0.45),
    "ODYSSEY":  dict(brightness=0.55,warmth=0.5, movement=0.7, density=0.5,  space=0.7,  aggression=0.3),
    "OVERDUB":  dict(brightness=0.45,warmth=0.7, movement=0.55,density=0.55, space=0.75, aggression=0.25),
    "OCELOT":   dict(brightness=0.65,warmth=0.6, movement=0.6, density=0.55, space=0.6,  aggression=0.5),
}

# ---------------------------------------------------------------------------
# Name vocabularies
# ---------------------------------------------------------------------------
VOCAB_OBSIDIAN  = ["Crystal", "Mineral", "Vitreous", "Volcanic Glass", "Sharp Edge", "Pure", "Facet", "Obsidian"]
VOCAB_OBSCURA   = ["Daguerreotype", "Silver Plate", "Exposure", "Latent", "Fixed Image", "Stiff", "Collodion", "Albumen"]
VOCAB_MARQUEE   = ["Crystal Exposure", "Mineral Daguerreotype", "Glass Plate", "Pure Silver", "Vitreous Image"]

COUPLING_TYPES  = [
    "Envelope->Filter", "LFO->Pitch", "Audio->FM", "Env->Amp",
    "Mod->Cutoff", "Gate->Env", "Pitch->Rate", "CV->Depth",
]

ENGINE_ID = {
    "OBSIDIAN": "Obsidian",
    "OBSCURA":  "Obscura",
    "ORACLE":   "Oracle",
    "OWLFISH":  "Owlfish",
    "OPAL":     "Opal",
    "ORGANON":  "Organon",
    "ORIGAMI":  "Origami",
    "ORBITAL":  "Orbital",
    "OCEANIC":  "Oceanic",
    "OPTIC":    "Optic",
    "ODYSSEY":  "Odyssey",
    "OVERDUB":  "Overdub",
    "OCELOT":   "Ocelot",
}

PARAM_PREFIX = {
    "OBSIDIAN": "obsidian_",
    "OBSCURA":  "obscura_",
    "ORACLE":   "oracle_",
    "OWLFISH":  "owl_",
    "OPAL":     "opal_",
    "ORGANON":  "organon_",
    "ORIGAMI":  "origami_",
    "ORBITAL":  "orb_",
    "OCEANIC":  "ocean_",
    "OPTIC":    "optic_",
    "ODYSSEY":  "drift_",
    "OVERDUB":  "dub_",
    "OCELOT":   "ocelot_",
}

# ---------------------------------------------------------------------------
# Per-engine stub parameters (representative set only — stubs)
# ---------------------------------------------------------------------------
def stub_params(engine_key, rng):
    prefix = PARAM_PREFIX[engine_key]
    base = DNA_BASELINES[engine_key]
    vary = lambda v: round(max(0.0, min(1.0, v + rng.uniform(-0.08, 0.08))), 3)
    return {
        f"{prefix}brightness":  vary(base["brightness"]),
        f"{prefix}warmth":      vary(base["warmth"]),
        f"{prefix}movement":    vary(base["movement"]),
        f"{prefix}density":     vary(base["density"]),
        f"{prefix}space":       vary(base["space"]),
        f"{prefix}aggression":  vary(base["aggression"]),
        f"{prefix}level":       round(rng.uniform(0.7, 0.9), 3),
    }

# ---------------------------------------------------------------------------
# DNA blend helper
# ---------------------------------------------------------------------------
def blend_dna(key_a, key_b, weight_a=0.5, rng=None):
    a, b = DNA_BASELINES[key_a], DNA_BASELINES[key_b]
    w = weight_a
    def mix(x, y):
        v = x * w + y * (1.0 - w)
        if rng:
            v += rng.uniform(-0.04, 0.04)
        return round(max(0.0, min(1.0, v)), 3)
    return {k: mix(a[k], b[k]) for k in a}

# ---------------------------------------------------------------------------
# Preset factory
# ---------------------------------------------------------------------------
DATE = "2026-03-16"

def make_preset(name, desc, tags, engine_keys, coupling_type, coupling_amount, dna_blend, rng):
    engine_ids = [ENGINE_ID[k] for k in engine_keys]
    params = {}
    for k in engine_keys:
        params[ENGINE_ID[k]] = stub_params(k, rng)
    intensity = rng.choice(["Moderate", "Deep", "Subtle"])
    return {
        "schema_version": 1,
        "name": name,
        "mood": "Entangled",
        "engines": engine_ids,
        "author": "XO_OX",
        "version": "1.0.0",
        "description": desc,
        "tags": tags,
        "macroLabels": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
        "couplingIntensity": intensity,
        "tempo": None,
        "created": DATE,
        "legacy": {"sourceInstrument": None, "sourceCategory": None, "sourcePresetName": None},
        "parameters": params,
        "coupling": {
            "pairs": [
                {
                    "engineA": engine_ids[0],
                    "engineB": engine_ids[1],
                    "type": coupling_type,
                    "amount": round(coupling_amount, 3),
                }
            ]
        },
        "sequencer": None,
        "dna": dna_blend,
    }

# ---------------------------------------------------------------------------
# Name generation helpers
# ---------------------------------------------------------------------------
def marquee_name(idx, vocab, rng):
    # Use fixed marquee vocab first, then blend words
    if idx < len(vocab):
        return vocab[idx]
    a = rng.choice(VOCAB_OBSIDIAN)
    b = rng.choice(VOCAB_OBSCURA)
    return f"{a} {b}"[:30]

def pair_name(primary_vocab, partner_key, idx, rng):
    partner_vocab = {
        "ORACLE":  ["Prophet", "Breakpoint", "Oracle"],
        "OWLFISH": ["Abyssal", "Luminous", "Deep Filament"],
        "OPAL":    ["Granular", "Dispersed", "Iridescent"],
        "ORGANON": ["Metabolic", "Enzymatic", "Living"],
        "ORIGAMI": ["Folded", "Crease", "Geometric"],
        "ORBITAL": ["Looping", "Rotating", "Gravitational"],
        "OCEANIC": ["Coastal", "Phosphor", "Wave"],
        "OPTIC":   ["Visual", "Pulse", "Phosphene"],
        "ODYSSEY": ["Drifting", "Voyager", "Wander"],
        "OVERDUB": ["Dubbed", "Layered", "Saturated"],
        "OCELOT":  ["Spotted", "Hunter", "Feral"],
    }.get(partner_key, ["Coupled", "Linked", "Merged"])

    words = primary_vocab + partner_vocab
    w1 = rng.choice(primary_vocab)
    w2 = rng.choice(partner_vocab)
    name = f"{w1} {w2}"
    return name[:30]

# ---------------------------------------------------------------------------
# Preset suite definitions
# ---------------------------------------------------------------------------
def build_presets(count_per_pair, rng):
    presets = []

    # --- MARQUEE: OBSIDIAN + OBSCURA (5 presets) ---
    marquee_descs = [
        "Crystal meets daguerreotype — vitreous clarity fused with photographic stiffness.",
        "Mineral hardness and silver plate emulsion in slow mutual exposure.",
        "Glass plate tension: the moment before the image is fixed forever.",
        "Pure volcanic edge etches a latent image into collodion suspension.",
        "Sharp crystal facets refract through the albumen of early photography.",
    ]
    for i in range(5):
        name = marquee_name(i, VOCAB_MARQUEE, rng)
        ctype = rng.choice(COUPLING_TYPES)
        amt = rng.uniform(0.55, 0.85)
        dna = blend_dna("OBSIDIAN", "OBSCURA", weight_a=0.5, rng=rng)
        tags = ["obsidian", "obscura", "crystal", "daguerreotype", "marquee", "entangled"]
        p = make_preset(name, marquee_descs[i], tags, ["OBSIDIAN", "OBSCURA"], ctype, amt, dna, rng)
        presets.append(p)

    # --- OBSIDIAN pairs ---
    obsidian_partners = ["ORACLE", "OWLFISH", "OPAL", "ORGANON", "ORIGAMI", "ORBITAL", "OCEANIC", "OPTIC"]
    obsidian_descs = {
        "ORACLE":   "Crystal clarity channels prophecy — obsidian edge meets broken oracle timbre.",
        "OWLFISH":  "Mineral hardness couples with abyssal gold filament. Cold light from deep.",
        "OPAL":     "Vitreous shards scatter into granular iridescence. Crystal dispersal field.",
        "ORGANON":  "Pure mineral feeds enzymatic metabolism. Hardness becoming life.",
        "ORIGAMI":  "Sharp facets fold. Obsidian geometry meets paper crease mathematics.",
        "ORBITAL":  "Volcanic glass rotates in gravitational loop. Crystal orbital path.",
        "OCEANIC":  "Crystal edge meets phosphorescent teal. Mineral dissolves into coast.",
        "OPTIC":    "Faceted obsidian refracts phosphor pulse. Sharp visual artifact.",
    }
    for partner in obsidian_partners:
        for j in range(count_per_pair):
            name = pair_name(VOCAB_OBSIDIAN, partner, j, rng)
            ctype = rng.choice(COUPLING_TYPES)
            amt = rng.uniform(0.4, 0.8)
            dna = blend_dna("OBSIDIAN", partner, weight_a=0.55, rng=rng)
            tags = ["obsidian", partner.lower(), "crystal", "entangled"]
            desc = obsidian_descs.get(partner, f"OBSIDIAN couples with {partner}.")
            p = make_preset(name, desc, tags, ["OBSIDIAN", partner], ctype, amt, dna, rng)
            presets.append(p)

    # --- OBSCURA pairs ---
    obscura_partners = ["ORACLE", "ORGANON", "OCEANIC", "ORBITAL", "ODYSSEY", "OWLFISH", "OVERDUB", "OCELOT"]
    obscura_descs = {
        "ORACLE":   "Daguerreotype stiffness channels oracle prophecy. Fixed image of broken futures.",
        "ORGANON":  "Silver plate metabolism — photochemical reaction feeds enzymatic life.",
        "OCEANIC":  "Latent coastal image. Collodion suspension meets phosphorescent wave.",
        "ORBITAL":  "Exposure loop. Fixed image rotates in warm red gravitational field.",
        "ODYSSEY":  "Silver plate voyage. Albumen of a long drifting journey.",
        "OWLFISH":  "Daguerreotype and abyssal filament — stiffness meets deep bioluminescence.",
        "OVERDUB":  "Collodion dub. Old photographic chemistry warped by tape saturation.",
        "OCELOT":   "Fixed image of a spotted hunter. Daguerreotype predator portrait.",
    }
    for partner in obscura_partners:
        for j in range(count_per_pair):
            name = pair_name(VOCAB_OBSCURA, partner, j, rng)
            ctype = rng.choice(COUPLING_TYPES)
            amt = rng.uniform(0.4, 0.8)
            dna = blend_dna("OBSCURA", partner, weight_a=0.55, rng=rng)
            tags = ["obscura", partner.lower(), "daguerreotype", "entangled"]
            desc = obscura_descs.get(partner, f"OBSCURA couples with {partner}.")
            p = make_preset(name, desc, tags, ["OBSCURA", partner], ctype, amt, dna, rng)
            presets.append(p)

    return presets

# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------
def main():
    parser = argparse.ArgumentParser(
        description="Generate OBSIDIAN + OBSCURA coupling presets for XOmnibus."
    )
    repo_root = Path(__file__).resolve().parent.parent
    default_out = str(repo_root / "Presets" / "XOmnibus" / "Entangled")

    parser.add_argument("--output-dir", default=default_out,
                        help="Directory to write .xometa files (default: Presets/XOmnibus/Entangled/)")
    parser.add_argument("--dry-run", action="store_true",
                        help="Print preset names without writing files")
    parser.add_argument("--seed", type=int, default=None,
                        help="Random seed for reproducible output")
    parser.add_argument("--count", type=int, default=3,
                        help="Presets per OBSIDIAN/OBSCURA pair (default: 3; marquee always 5)")
    args = parser.parse_args()

    rng = random.Random(args.seed)
    presets = build_presets(args.count, rng)

    marquee = [p for p in presets if "marquee" in p["tags"]]
    obsidian_pairs = [p for p in presets if "obsidian" in p["tags"] and "marquee" not in p["tags"]]
    obscura_pairs  = [p for p in presets if "obscura" in p["tags"] and "marquee" not in p["tags"]]

    print(f"OBSIDIAN+OBSCURA marquee : {len(marquee):3d} presets")
    print(f"OBSIDIAN pairs           : {len(obsidian_pairs):3d} presets")
    print(f"OBSCURA  pairs           : {len(obscura_pairs):3d} presets")
    print(f"Total                    : {len(presets):3d} presets")

    if args.dry_run:
        print("\n--- DRY RUN ---")
        for p in presets:
            print(f"  [{p['engines'][0]:12s} + {p['engines'][1]:12s}]  {p['name']}")
        return

    out_dir = Path(args.output_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    written = 0
    skipped = 0
    for preset in presets:
        filename = preset["name"].replace(" ", "_").replace("/", "-") + ".xometa"
        filepath = out_dir / filename
        if filepath.exists():
            skipped += 1
            continue
        with open(filepath, "w", encoding="utf-8") as f:
            json.dump(preset, f, indent=2)
        written += 1

    print(f"\nWritten : {written}")
    print(f"Skipped : {skipped} (already exist)")
    print(f"Output  : {out_dir}")


if __name__ == "__main__":
    main()
