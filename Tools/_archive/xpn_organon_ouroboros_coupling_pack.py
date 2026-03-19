#!/usr/bin/env python3
"""Generate Organon + Ouroboros coupling presets for XOmnibus.

Covers:
  - ORGANON + OUROBOROS marquee pair: 6 presets
  - ORGANON pairs (8 partners × 3):   24 presets
  - OUROBOROS pairs (8 partners × 3): 24 presets
  Total: 54 presets → Presets/XOmnibus/Entangled/
"""

import argparse
import json
import os
import random
from pathlib import Path

DATE = "2026-03-16"
SCHEMA_VERSION = 1
AUTHOR = "XO_OX"
VERSION = "1.0.0"

# ---------------------------------------------------------------------------
# Sonic DNA baselines
# ---------------------------------------------------------------------------
DNA_BASELINES = {
    "ORGANON":   dict(brightness=0.60, warmth=0.60, movement=0.65, density=0.75, space=0.65, aggression=0.40),
    "OUROBOROS": dict(brightness=0.50, warmth=0.40, movement=0.85, density=0.75, space=0.50, aggression=0.80),
    "ORACLE":    dict(brightness=0.50, warmth=0.40, movement=0.60, density=0.70, space=0.70, aggression=0.40),
    "ODYSSEY":   dict(brightness=0.55, warmth=0.50, movement=0.70, density=0.50, space=0.70, aggression=0.30),
    "OPAL":      dict(brightness=0.70, warmth=0.50, movement=0.75, density=0.45, space=0.80, aggression=0.20),
    "OBLONG":    dict(brightness=0.60, warmth=0.65, movement=0.50, density=0.65, space=0.55, aggression=0.50),
    "ORBITAL":   dict(brightness=0.60, warmth=0.65, movement=0.55, density=0.60, space=0.60, aggression=0.50),
    "OCEANIC":   dict(brightness=0.65, warmth=0.55, movement=0.50, density=0.55, space=0.75, aggression=0.30),
    "OVERWORLD": dict(brightness=0.75, warmth=0.40, movement=0.50, density=0.65, space=0.55, aggression=0.60),
    "OPTIC":     dict(brightness=0.85, warmth=0.30, movement=0.90, density=0.40, space=0.50, aggression=0.45),
    "ORIGAMI":   dict(brightness=0.70, warmth=0.45, movement=0.65, density=0.55, space=0.55, aggression=0.55),
    "OCELOT":    dict(brightness=0.65, warmth=0.60, movement=0.60, density=0.55, space=0.60, aggression=0.50),
    "ONSET":     dict(brightness=0.55, warmth=0.50, movement=0.80, density=0.75, space=0.50, aggression=0.70),
    "OBSIDIAN":  dict(brightness=0.90, warmth=0.20, movement=0.30, density=0.50, space=0.60, aggression=0.10),
}

# ---------------------------------------------------------------------------
# Parameter prefix map (engine short-name → param prefix)
# ---------------------------------------------------------------------------
PREFIX = {
    "ORGANON":   "organon_",
    "OUROBOROS": "ouro_",
    "ORACLE":    "oracle_",
    "ODYSSEY":   "drift_",
    "OPAL":      "opal_",
    "OBLONG":    "bob_",
    "ORBITAL":   "orb_",
    "OCEANIC":   "ocean_",
    "OVERWORLD": "ow_",
    "OPTIC":     "optic_",
    "ORIGAMI":   "origami_",
    "OCELOT":    "ocelot_",
    "ONSET":     "perc_",
    "OBSIDIAN":  "obsidian_",
}

# Display names used in preset "engines" array
DISPLAY = {
    "ORGANON":   "Organon",
    "OUROBOROS": "Ouroboros",
    "ORACLE":    "Oracle",
    "ODYSSEY":   "Odyssey",
    "OPAL":      "Opal",
    "OBLONG":    "Oblong",
    "ORBITAL":   "Orbital",
    "OCEANIC":   "Oceanic",
    "OVERWORLD": "Overworld",
    "OPTIC":     "Optic",
    "ORIGAMI":   "Origami",
    "OCELOT":    "Ocelot",
    "ONSET":     "Onset",
    "OBSIDIAN":  "Obsidian",
}

# ---------------------------------------------------------------------------
# Name vocabulary
# ---------------------------------------------------------------------------
VOCAB = {
    "ORGANON":   ["Metabolism", "Homeostasis", "Adaptive", "Entropy", "Emergence",
                  "Gradient", "Vital", "Feedback"],
    "OUROBOROS": ["Chaos", "Attractor", "Topology", "Leash", "Strange",
                  "Loop", "Recursive", "Bifurcate"],
    "MARQUEE":   ["Strange Metabolism", "Chaotic Homeostasis", "Adaptive Attractor",
                  "Living Chaos", "Vital Loop"],
}

# Coupling types available in MegaCouplingMatrix
COUPLING_TYPES = [
    "PitchSync", "FilterChain", "EnvelopeFollow", "RingMod",
    "AmplitudeModulation", "FrequencyModulation", "WaveShape", "GrainFeed",
    "ResonantLock", "SpectralBlend", "DriftSync", "ChaoticFeedback",
]

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def blend_dna(a_key: str, b_key: str, weight_a: float = 0.5) -> dict:
    """Linear blend of two DNA baselines (weight_a in A, 1-weight_a in B)."""
    a = DNA_BASELINES[a_key]
    b = DNA_BASELINES[b_key]
    w = weight_a
    return {k: round(a[k] * w + b[k] * (1.0 - w), 3) for k in a}


def jitter(val: float, scale: float = 0.08) -> float:
    """Add small random perturbation, clamped to [0,1]."""
    return round(min(1.0, max(0.0, val + random.uniform(-scale, scale))), 3)


def jitter_dna(dna: dict, scale: float = 0.07) -> dict:
    return {k: jitter(v, scale) for k, v in dna.items()}


def coupling_amount() -> float:
    return round(random.uniform(0.45, 0.90), 2)


def make_coupling_pairs(eng_a: str, eng_b: str, n: int = 2) -> list:
    """Generate n coupling pair dicts between two engines."""
    types = random.sample(COUPLING_TYPES, min(n, len(COUPLING_TYPES)))
    pairs = []
    for t in types:
        pairs.append({
            "engineA": DISPLAY[eng_a],
            "engineB": DISPLAY[eng_b],
            "type": t,
            "amount": coupling_amount(),
        })
    return pairs


def stub_params(engine: str) -> dict:
    """Return minimal stub parameter dict for an engine."""
    pfx = PREFIX[engine]
    # All engines get a small set of representative params at mid values
    params = {
        f"{pfx}macroCharacter": round(random.uniform(0.4, 0.7), 2),
        f"{pfx}macroMovement":  round(random.uniform(0.4, 0.8), 2),
        f"{pfx}macroCoupling": round(random.uniform(0.5, 0.9), 2),
        f"{pfx}macroSpace":    round(random.uniform(0.4, 0.7), 2),
    }
    # Engine-specific signature params
    if engine == "ORGANON":
        params.update({
            "organon_metabolicRate": round(random.uniform(0.3, 1.2), 3),
            "organon_enzymeSelect":  round(random.uniform(500.0, 2000.0), 1),
            "organon_catalystDrive": round(random.uniform(0.3, 0.8), 2),
            "organon_dampingCoeff":  round(random.uniform(0.1, 0.5), 2),
            "organon_signalFlux":    round(random.uniform(0.3, 0.8), 2),
            "organon_membrane":      round(random.uniform(0.1, 0.4), 2),
        })
    elif engine == "OUROBOROS":
        params.update({
            "ouro_topology":       round(random.uniform(0.0, 1.0), 2),
            "ouro_leashAmount":    round(random.uniform(0.2, 0.9), 2),
            "ouro_attractorType":  random.randint(0, 3),
            "ouro_chaosDrive":     round(random.uniform(0.4, 1.0), 2),
            "ouro_bifurcation":    round(random.uniform(0.0, 0.8), 2),
            "ouro_recursionDepth": random.randint(1, 8),
        })
    elif engine == "ORACLE":
        params.update({
            "oracle_breakpoints": round(random.uniform(2.0, 16.0), 1),
            "oracle_stochastic":  round(random.uniform(0.2, 0.9), 2),
        })
    elif engine == "ODYSSEY":
        params.update({
            "drift_oscA_mode": random.randint(0, 4),
            "drift_drift":     round(random.uniform(0.0, 0.5), 2),
        })
    elif engine == "OPAL":
        params.update({
            "opal_grainSize":    round(random.uniform(20.0, 200.0), 1),
            "opal_grainDensity": round(random.uniform(0.3, 0.9), 2),
        })
    elif engine == "OBLONG":
        params.update({
            "bob_fltCutoff": round(random.uniform(800.0, 8000.0), 1),
            "bob_fltReso":   round(random.uniform(0.1, 0.7), 2),
        })
    elif engine == "ORBITAL":
        params.update({
            "orb_brightness":     round(random.uniform(0.4, 0.8), 2),
            "orb_groupEnvAttack": round(random.uniform(0.001, 0.1), 4),
        })
    elif engine == "OCEANIC":
        params.update({
            "ocean_separation": round(random.uniform(0.2, 0.8), 2),
            "ocean_depth":      round(random.uniform(0.3, 0.9), 2),
        })
    elif engine == "OVERWORLD":
        params.update({
            "ow_era":      round(random.uniform(0.0, 1.0), 2),
            "ow_chipType": random.randint(0, 2),
        })
    elif engine == "OPTIC":
        params.update({
            "optic_pulseRate":  round(random.uniform(0.5, 8.0), 2),
            "optic_autoPulse":  1,
        })
    elif engine == "ORIGAMI":
        params.update({
            "origami_foldPoint": round(random.uniform(0.2, 0.8), 2),
            "origami_crease":    round(random.uniform(0.3, 0.9), 2),
        })
    elif engine == "OCELOT":
        params.update({
            "ocelot_biome":  random.randint(0, 3),
            "ocelot_stalk":  round(random.uniform(0.3, 0.8), 2),
        })
    elif engine == "ONSET":
        params.update({
            "perc_noiseLevel": round(random.uniform(0.4, 0.9), 2),
            "perc_transient":  round(random.uniform(0.5, 1.0), 2),
        })
    elif engine == "OBSIDIAN":
        params.update({
            "obsidian_depth":   round(random.uniform(0.3, 0.8), 2),
            "obsidian_clarity": round(random.uniform(0.5, 1.0), 2),
        })
    return params


def generate_name(eng_a: str, eng_b: str, idx: int, is_marquee: bool = False) -> str:
    """Produce a unique 2-3 word preset name."""
    if is_marquee:
        base = VOCAB["MARQUEE"][idx % len(VOCAB["MARQUEE"])]
        suffixes = ["I", "II", "III", "IV", "V", "VI"]
        return f"{base} {suffixes[idx % len(suffixes)]}"

    vocab_a = VOCAB.get(eng_a, ["Signal"])
    vocab_b = VOCAB.get(eng_b, ["Form"])

    word_a = vocab_a[idx % len(vocab_a)]
    word_b = vocab_b[(idx + 3) % len(vocab_b)]

    patterns = [
        f"{word_a} {word_b}",
        f"{word_b} {word_a}",
        f"{word_a} {word_b} Field",
        f"Living {word_b}",
        f"{word_a} State",
        f"Deep {word_b}",
    ]
    return patterns[idx % len(patterns)]


def generate_description(eng_a: str, eng_b: str, coupling_pairs: list) -> str:
    ctypes = ", ".join(sorted({p["type"] for p in coupling_pairs}))
    return (
        f"{DISPLAY[eng_a]} meets {DISPLAY[eng_b]} — "
        f"coupling via {ctypes}. "
        f"Metabolic energy meets chaotic topology in an Entangled field."
    )


def make_preset(name: str, eng_a: str, eng_b: str, coupling_pairs: list,
                dna_blend: dict, idx: int, is_marquee: bool = False) -> dict:
    tags = [eng_a.lower(), eng_b.lower(), "coupled", "entangled",
            "complex", "metabolism" if eng_a == "ORGANON" or eng_b == "ORGANON" else "organic",
            "chaos" if eng_a == "OUROBOROS" or eng_b == "OUROBOROS" else "dynamic"]

    params = {}
    params[DISPLAY[eng_a]] = stub_params(eng_a)
    params[DISPLAY[eng_b]] = stub_params(eng_b)

    intensity_choices = ["Moderate", "Strong", "Extreme"]
    intensity = intensity_choices[idx % len(intensity_choices)]
    if is_marquee:
        intensity = "Extreme"

    return {
        "schema_version": SCHEMA_VERSION,
        "name": name,
        "mood": "Entangled",
        "engines": [DISPLAY[eng_a], DISPLAY[eng_b]],
        "author": AUTHOR,
        "version": VERSION,
        "description": generate_description(eng_a, eng_b, coupling_pairs),
        "tags": tags,
        "macroLabels": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
        "couplingIntensity": intensity,
        "tempo": None,
        "created": DATE,
        "legacy": {
            "sourceInstrument": None,
            "sourceCategory": None,
            "sourcePresetName": None,
        },
        "parameters": params,
        "coupling": {
            "type": coupling_pairs[0]["type"] if coupling_pairs else "FilterChain",
            "pairs": coupling_pairs,
        },
        "sequencer": None,
        "dna": jitter_dna(dna_blend),
    }


# ---------------------------------------------------------------------------
# Preset batch definitions
# ---------------------------------------------------------------------------

def build_all_presets(count: int) -> list:
    """Build all preset dicts according to spec."""
    presets = []

    # ---- ORGANON + OUROBOROS marquee (6 presets) ----
    marquee_dna = blend_dna("ORGANON", "OUROBOROS", 0.5)
    for i in range(6):
        name = generate_name("ORGANON", "OUROBOROS", i, is_marquee=True)
        n_pairs = 2 + (i % 2)  # 2 or 3 coupling pairs for complexity
        pairs = make_coupling_pairs("ORGANON", "OUROBOROS", n=n_pairs)
        presets.append(make_preset(name, "ORGANON", "OUROBOROS", pairs,
                                   marquee_dna, i, is_marquee=True))

    # ---- ORGANON pairs (8 partners × count) ----
    organon_partners = ["ORACLE", "ODYSSEY", "OPAL", "OBLONG",
                        "ORBITAL", "OCEANIC", "OVERWORLD", "OPTIC"]
    for partner in organon_partners:
        blend = blend_dna("ORGANON", partner, weight_a=0.55)
        for i in range(count):
            name = generate_name("ORGANON", partner, i)
            pairs = make_coupling_pairs("ORGANON", partner, n=2)
            presets.append(make_preset(name, "ORGANON", partner, pairs, blend, i))

    # ---- OUROBOROS pairs (8 partners × count) ----
    ouroboros_partners = ["ORACLE", "OVERWORLD", "OPTIC", "ORIGAMI",
                          "OCELOT", "ONSET", "OBSIDIAN", "ORBITAL"]
    for partner in ouroboros_partners:
        blend = blend_dna("OUROBOROS", partner, weight_a=0.55)
        for i in range(count):
            name = generate_name("OUROBOROS", partner, i)
            pairs = make_coupling_pairs("OUROBOROS", partner, n=2)
            presets.append(make_preset(name, "OUROBOROS", partner, pairs, blend, i))

    return presets


def safe_filename(name: str, eng_a: str = "", eng_b: str = "") -> str:
    base = name.replace(" ", "_").replace("/", "-")
    if eng_a and eng_b:
        suffix = f"_{eng_a[:3]}_{eng_b[:3]}"
    else:
        suffix = ""
    return base + suffix + ".xometa"


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description="Generate Organon + Ouroboros coupling presets for XOmnibus."
    )
    repo_root = Path(__file__).resolve().parent.parent
    default_out = str(repo_root / "Presets" / "XOmnibus" / "Entangled")

    parser.add_argument(
        "--output-dir", default=default_out,
        help="Directory to write .xometa files (default: Presets/XOmnibus/Entangled/)",
    )
    parser.add_argument(
        "--dry-run", action="store_true",
        help="Print preset names without writing files",
    )
    parser.add_argument(
        "--seed", type=int, default=None,
        help="Random seed for reproducible output",
    )
    parser.add_argument(
        "--count", type=int, default=3,
        help="Presets per ORGANON-partner and OUROBOROS-partner pair (default: 3)",
    )
    args = parser.parse_args()

    if args.seed is not None:
        random.seed(args.seed)

    presets = build_all_presets(args.count)

    if args.dry_run:
        print(f"[dry-run] Would write {len(presets)} presets to: {args.output_dir}")
        for p in presets:
            eng_a, eng_b = p["engines"][0].upper(), p["engines"][1].upper()
            print(f"  {safe_filename(p['name'], eng_a, eng_b)}  —  {p['engines']}  [{p['coupling']['type']}]")
        return

    out_dir = Path(args.output_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    written = 0
    for preset in presets:
        eng_a, eng_b = preset["engines"][0].upper(), preset["engines"][1].upper()
        filename = safe_filename(preset["name"], eng_a, eng_b)
        filepath = out_dir / filename
        with open(filepath, "w", encoding="utf-8") as f:
            json.dump(preset, f, indent=2)
        written += 1
        print(f"  wrote: {filepath.name}")

    print(f"\nDone. {written} presets written to {out_dir}")


if __name__ == "__main__":
    main()
