#!/usr/bin/env python3
"""Generate OBESE + OVERBITE coupling pack for XOceanus.

OBESE  — Hot Pink #FF1493, Mojo control B015, fat saturation engine, fat_ prefix
OVERBITE — Fang White #F0EDE8, Five-Macro system B008, bass-forward character, poss_ prefix

Generates 53 presets total:
  - 5  OBESE × OVERBITE marquee presets
  - 24 OBESE new pairs (8 partners × 3 presets each)
  - 24 OVERBITE new pairs (8 partners × 3 presets each)

All presets written to Presets/XOceanus/Entangled/.
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
    "OBESE":     {"brightness": 0.5,  "warmth": 0.7,  "movement": 0.45, "density": 0.8,  "space": 0.4,  "aggression": 0.75},
    "OVERBITE":  {"brightness": 0.45, "warmth": 0.65, "movement": 0.5,  "density": 0.7,  "space": 0.5,  "aggression": 0.65},
    "OBLONG":    {"brightness": 0.6,  "warmth": 0.65, "movement": 0.5,  "density": 0.65, "space": 0.55, "aggression": 0.5},
    "ONSET":     {"brightness": 0.55, "warmth": 0.5,  "movement": 0.8,  "density": 0.75, "space": 0.5,  "aggression": 0.7},
    "OUROBOROS": {"brightness": 0.5,  "warmth": 0.4,  "movement": 0.85, "density": 0.75, "space": 0.5,  "aggression": 0.8},
    "OVERWORLD": {"brightness": 0.75, "warmth": 0.4,  "movement": 0.5,  "density": 0.65, "space": 0.55, "aggression": 0.6},
    "OBLIQUE":   {"brightness": 0.75, "warmth": 0.4,  "movement": 0.7,  "density": 0.5,  "space": 0.6,  "aggression": 0.55},
    "OHM":       {"brightness": 0.45, "warmth": 0.75, "movement": 0.55, "density": 0.6,  "space": 0.7,  "aggression": 0.3},
    "OTTONI":    {"brightness": 0.6,  "warmth": 0.6,  "movement": 0.5,  "density": 0.7,  "space": 0.55, "aggression": 0.6},
    "ORACLE":    {"brightness": 0.5,  "warmth": 0.4,  "movement": 0.6,  "density": 0.7,  "space": 0.7,  "aggression": 0.4},
    "OVERDUB":   {"brightness": 0.45, "warmth": 0.7,  "movement": 0.55, "density": 0.55, "space": 0.75, "aggression": 0.25},
    "ORIGAMI":   {"brightness": 0.7,  "warmth": 0.45, "movement": 0.65, "density": 0.55, "space": 0.55, "aggression": 0.55},
    "OSTERIA":   {"brightness": 0.4,  "warmth": 0.75, "movement": 0.45, "density": 0.7,  "space": 0.6,  "aggression": 0.45},
}

# Engine name → XOceanus engine key (used in "engines" array and coupling pairs)
ENGINE_KEY = {
    "OBESE":     "XObese",
    "OVERBITE":  "XOverbite",
    "OBLONG":    "XOblong",
    "ONSET":     "XOnset",
    "OUROBOROS": "XOuroboros",
    "OVERWORLD": "XOverworld",
    "OBLIQUE":   "XOblique",
    "OHM":       "XOhm",
    "OTTONI":    "XOttoni",
    "ORACLE":    "XOracle",
    "OVERDUB":   "XOverdub",
    "ORIGAMI":   "XOrigami",
    "OSTERIA":   "XOsteria",
}

# Engine name → parameter prefix
PREFIX = {
    "OBESE":     "fat_",
    "OVERBITE":  "poss_",
    "OBLONG":    "bob_",
    "ONSET":     "ons_",
    "OUROBOROS": "ouro_",
    "OVERWORLD": "ow_",
    "OBLIQUE":   "obl_",
    "OHM":       "ohm_",
    "OTTONI":    "ott_",
    "ORACLE":    "orc_",
    "OVERDUB":   "dub_",
    "ORIGAMI":   "ori_",
    "OSTERIA":   "ost_",
}

# ---------------------------------------------------------------------------
# Name vocabularies
# ---------------------------------------------------------------------------

VOCAB_OBESE  = ["Fat", "Saturate", "Drive", "Mojo", "Thick", "Crush", "Analog Heat", "Warm Clip", "Compress"]
VOCAB_OVERBITE = ["Bite", "Fang", "Gnash", "Bass Forward", "Snap", "Chew", "Grip", "Clench"]

MARQUEE_NAMES = [
    "Fat Bite",
    "Fang Saturation",
    "Mojo Gnash",
    "Thick Snap",
    "The Hungry Pair",
]

# Partner descriptors for name construction
PARTNER_DESCRIPTOR = {
    "OBLONG":    ["Curious", "Wandering", "Bob"],
    "ONSET":     ["Rhythmic", "Percussion", "Drum"],
    "OUROBOROS": ["Recursive", "Infinite", "Loop"],
    "OVERWORLD": ["Chip", "Pixel", "8-Bit"],
    "OBLIQUE":   ["Angled", "Slant", "Off-Axis"],
    "OHM":       ["Commune", "Warm Drone", "Hippy"],
    "OTTONI":    ["Brass", "Horn", "Fanfare"],
    "ORACLE":    ["Prophecy", "Vision", "Foretold"],
    "OVERDUB":   ["Tape", "Echo", "Dub"],
    "ORIGAMI":   ["Fold", "Crisp", "Paper"],
    "OSTERIA":   ["Table", "Feast", "Slow"],
}

COUPLING_TYPES = [
    "Amp->Filter",
    "Env->Morph",
    "LFO->Pitch",
    "Rhythm->Blend",
    "Audio->Wavetable",
    "Filter->Drive",
    "Amp->Amp",
]

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def blend_dna(dna_a: dict, dna_b: dict, rng: random.Random, weight: float = 0.5) -> dict:
    """Blend two DNA dicts with a slight random nudge."""
    keys = ["brightness", "warmth", "movement", "density", "space", "aggression"]
    result = {}
    for k in keys:
        base = dna_a[k] * (1 - weight) + dna_b[k] * weight
        nudge = rng.uniform(-0.04, 0.04)
        result[k] = round(min(1.0, max(0.0, base + nudge)), 3)
    return result


def coupling_intensity(amount: float) -> str:
    if amount >= 0.75:
        return "Deep"
    if amount >= 0.5:
        return "Moderate"
    return "Light"


def stub_params(engine_name: str, dna: dict, rng: random.Random) -> dict:
    """Generate minimal stub parameters for an engine using its prefix."""
    p = PREFIX[engine_name]
    return {
        f"{p}outputLevel": round(rng.uniform(0.75, 0.9), 2),
        f"{p}outputPan": round(rng.uniform(-0.05, 0.05), 2),
        f"{p}couplingLevel": round(rng.uniform(0.55, 0.8), 2),
        f"{p}couplingBus": 0,
        f"{p}brightness": dna["brightness"],
        f"{p}warmth": dna["warmth"],
        f"{p}aggression": dna["aggression"],
    }


def make_preset(
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
    intensity = coupling_intensity(coupling_amount)

    dna_a_blended = blend_dna(DNA[engine_a], dna, rng, 0.35)
    dna_b_blended = blend_dna(DNA[engine_b], dna, rng, 0.35)

    return {
        "schema_version": 1,
        "name": name,
        "mood": "Entangled",
        "sonic_dna": dna,
        "engines": [key_a, key_b],
        "author": "XO_OX",
        "version": "1.0.0",
        "description": desc,
        "tags": ["coupling", "entangled"] + tags,
        "macroLabels": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
        "couplingIntensity": intensity,
        "dna": dna,
        "parameters": {
            key_a: stub_params(engine_a, dna_a_blended, rng),
            key_b: stub_params(engine_b, dna_b_blended, rng),
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


# ---------------------------------------------------------------------------
# Preset definitions
# ---------------------------------------------------------------------------

def build_marquee_presets(rng: random.Random, count: int) -> list:
    """5 OBESE × OVERBITE marquee presets."""
    configs = [
        (
            MARQUEE_NAMES[0],
            "OBESE saturation drives OVERBITE filter. Maximum low-end character.",
            ["obese", "overbite", "fat", "bite"],
            "Amp->Filter", 0.8,
        ),
        (
            MARQUEE_NAMES[1],
            "OVERBITE fang attack modulates OBESE drive amount. Bite shapes the heat.",
            ["obese", "overbite", "fang", "saturation"],
            "Env->Morph", 0.7,
        ),
        (
            MARQUEE_NAMES[2],
            "OBESE Mojo macro locks to OVERBITE gnash envelope. High-character sync.",
            ["obese", "overbite", "mojo", "gnash"],
            "Amp->Amp", 0.75,
        ),
        (
            MARQUEE_NAMES[3],
            "OVERBITE snap triggers OBESE compressor sidechain. Thick rhythmic pulse.",
            ["obese", "overbite", "thick", "snap"],
            "Rhythm->Blend", 0.65,
        ),
        (
            MARQUEE_NAMES[4],
            "Full bidirectional entanglement — OBESE and OVERBITE feeding each other. Ravenous.",
            ["obese", "overbite", "bidirectional", "hungry"],
            "Audio->Wavetable", 0.85,
        ),
    ]
    presets = []
    for name, desc, tags, ctype, camount in configs:
        dna = blend_dna(DNA["OBESE"], DNA["OVERBITE"], rng, 0.5)
        presets.append(make_preset(name, desc, tags, "OBESE", "OVERBITE", ctype, camount, dna, rng))
    return presets[:count]


def build_obese_pair_presets(partner: str, rng: random.Random, count: int) -> list:
    """Generate `count` OBESE × partner presets."""
    descs_by_partner = {
        "OBLONG":    "OBESE warmth coats OBLONG curiosity. Fat Bob, saturated wandering.",
        "ONSET":     "OBESE drive compresses ONSET transients. Thick percussion character.",
        "OUROBOROS": "OBESE feedback loop entangles with OUROBOROS recursion. Infinite saturation.",
        "OVERWORLD": "OBESE analog heat warms OVERWORLD chiptune. Fat pixels.",
        "OBLIQUE":   "OBESE crush modulates OBLIQUE angle. Off-axis harmonic density.",
        "OHM":       "OBESE drive softened by OHM commune warmth. Hot hippie.",
        "OTTONI":    "OBESE saturation into OTTONI brass. Thick fanfare.",
        "ORACLE":    "OBESE mojo reveals ORACLE vision. Saturated prophecy.",
    }
    presets = []
    desc_base = descs_by_partner[partner]
    descriptors = PARTNER_DESCRIPTOR.get(partner, [partner.capitalize()])
    used_ctypes = []

    for i in range(count):
        voc = rng.choice(VOCAB_OBESE)
        desc_word = descriptors[i % len(descriptors)]
        name = f"{voc} {desc_word}"
        # avoid duplicate coupling types within a partner set
        available = [c for c in COUPLING_TYPES if c not in used_ctypes] or COUPLING_TYPES
        ctype = rng.choice(available)
        used_ctypes.append(ctype)
        camount = round(rng.uniform(0.55, 0.85), 2)
        dna = blend_dna(DNA["OBESE"], DNA[partner], rng, rng.uniform(0.4, 0.6))
        tags = ["obese", partner.lower(), "fat"]
        desc = f"{desc_base} [{ctype}]"
        presets.append(make_preset(name, desc, tags, "OBESE", partner, ctype, camount, dna, rng))
    return presets


def build_overbite_pair_presets(partner: str, rng: random.Random, count: int) -> list:
    """Generate `count` OVERBITE × partner presets."""
    descs_by_partner = {
        "OBLONG":    "OVERBITE fang bites into OBLONG's harmonic curiosity. Bass-forward Bob.",
        "ONSET":     "OVERBITE snap aligns with ONSET hits. Bitten percussion.",
        "OVERDUB":   "OVERBITE grip feeds OVERDUB tape. Bass-forward echo.",
        "ORACLE":    "OVERBITE gnash decodes ORACLE vision. Bass prophecy.",
        "OBLIQUE":   "OVERBITE clench bends OBLIQUE angle. Bitten geometry.",
        "OHM":       "OVERBITE bass warmth meets OHM commune. Grounded drone.",
        "ORIGAMI":   "OVERBITE fang creases ORIGAMI fold. Bass-crisp edge.",
        "OSTERIA":   "OVERBITE bite at the OSTERIA table. Slow feast, fat bass.",
    }
    presets = []
    desc_base = descs_by_partner[partner]
    descriptors = PARTNER_DESCRIPTOR.get(partner, [partner.capitalize()])
    used_ctypes = []

    for i in range(count):
        voc = rng.choice(VOCAB_OVERBITE)
        desc_word = descriptors[i % len(descriptors)]
        name = f"{voc} {desc_word}"
        available = [c for c in COUPLING_TYPES if c not in used_ctypes] or COUPLING_TYPES
        ctype = rng.choice(available)
        used_ctypes.append(ctype)
        camount = round(rng.uniform(0.5, 0.80), 2)
        dna = blend_dna(DNA["OVERBITE"], DNA[partner], rng, rng.uniform(0.4, 0.6))
        tags = ["overbite", partner.lower(), "bite"]
        desc = f"{desc_base} [{ctype}]"
        presets.append(make_preset(name, desc, tags, "OVERBITE", partner, ctype, camount, dna, rng))
    return presets


# ---------------------------------------------------------------------------
# Build all presets
# ---------------------------------------------------------------------------

OBESE_PARTNERS   = ["OBLONG", "ONSET", "OUROBOROS", "OVERWORLD", "OBLIQUE", "OHM", "OTTONI", "ORACLE"]
OVERBITE_PARTNERS = ["OBLONG", "ONSET", "OVERDUB", "ORACLE", "OBLIQUE", "OHM", "ORIGAMI", "OSTERIA"]


def build_all(rng: random.Random, count_per_pair: int) -> list:
    presets = []
    presets += build_marquee_presets(rng, min(5, count_per_pair + 2))
    for partner in OBESE_PARTNERS:
        presets += build_obese_pair_presets(partner, rng, count_per_pair)
    for partner in OVERBITE_PARTNERS:
        presets += build_overbite_pair_presets(partner, rng, count_per_pair)
    return presets


# ---------------------------------------------------------------------------
# I/O
# ---------------------------------------------------------------------------

def write_preset(preset: dict, output_dir: Path) -> Path:
    output_dir.mkdir(parents=True, exist_ok=True)
    filename = preset["name"].replace("/", "-") + ".xometa"
    filepath = output_dir / filename
    with open(filepath, "w") as f:
        json.dump(preset, f, indent=2)
        f.write("\n")
    return filepath


def deduplicate_names(presets: list) -> list:
    seen = {}
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
# CLI
# ---------------------------------------------------------------------------

def main():
    repo_root = Path(__file__).resolve().parent.parent
    default_output = repo_root / "Presets" / "XOceanus" / "Entangled"

    parser = argparse.ArgumentParser(
        description="Generate OBESE + OVERBITE coupling pack presets for XOceanus."
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=default_output,
        help="Directory to write .xometa files (default: Presets/XOceanus/Entangled/)",
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
        help="Presets per partner pair (default: 3). Marquee always produces 5.",
    )
    args = parser.parse_args()

    rng = random.Random(args.seed)
    presets = build_all(rng, args.count)
    presets = deduplicate_names(presets)

    # Expected totals at default --count 3:
    #   marquee: 5
    #   obese pairs: 8 partners × 3 = 24
    #   overbite pairs: 8 partners × 3 = 24
    #   total: 53

    print(f"OBESE + OVERBITE Coupling Pack — {len(presets)} presets")
    print(f"Output: {args.output_dir}")
    print()

    written = 0
    for preset in presets:
        if args.dry_run:
            engines = " × ".join(preset["engines"])
            print(f"  [DRY-RUN] {preset['name']}  ({engines})")
        else:
            path = write_preset(preset, args.output_dir)
            print(f"  Wrote: {path.name}")
            written += 1

    if not args.dry_run:
        print(f"\nDone — {written} presets written to {args.output_dir}")
    else:
        print(f"\nDry run complete — {len(presets)} presets would be written.")


if __name__ == "__main__":
    main()
