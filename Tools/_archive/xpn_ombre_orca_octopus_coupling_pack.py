#!/usr/bin/env python3
"""Generate coupling presets for OMBRE, ORCA, and OCTOPUS engines.

Covers:
  - OMBRE pairs: ODYSSEY, OVERDUB, OPAL, ORACLE, OBSIDIAN, ORBITAL (6 × 3 = 18 presets)
  - ORCA pairs:  ONSET, OBLONG, OUROBOROS, ORACLE, OVERWORLD, OBSIDIAN (6 × 3 = 18 presets)
  - OCTOPUS pairs: ORACLE, OPTIC, OBLIQUE, OUTWIT, ORIGAMI, OCEANIC (6 × 3 = 18 presets)
  - Triads: OMBRE+ORCA, ORCA+OCTOPUS, OMBRE+OCTOPUS (3 × 3 = 9 presets)

Total: 63 Entangled presets.
"""

import argparse
import json
import os
import random
from pathlib import Path

# ---------------------------------------------------------------------------
# DNA baselines
# ---------------------------------------------------------------------------

DNA = {
    "OMBRE": {
        "brightness": 0.45, "warmth": 0.6, "movement": 0.55,
        "density": 0.6, "space": 0.65, "aggression": 0.3,
    },
    "ORCA": {
        "brightness": 0.3, "warmth": 0.35, "movement": 0.7,
        "density": 0.7, "space": 0.5, "aggression": 0.75,
    },
    "OCTOPUS": {
        "brightness": 0.7, "warmth": 0.4, "movement": 0.85,
        "density": 0.6, "space": 0.5, "aggression": 0.55,
    },
    # Partner engine DNA estimates (used when blending)
    "ODYSSEY":   {"brightness": 0.55, "warmth": 0.6, "movement": 0.5,  "density": 0.5, "space": 0.55, "aggression": 0.2},
    "OVERDUB":   {"brightness": 0.35, "warmth": 0.65, "movement": 0.45, "density": 0.45, "space": 0.6, "aggression": 0.15},
    "OPAL":      {"brightness": 0.6,  "warmth": 0.5,  "movement": 0.6,  "density": 0.55, "space": 0.6, "aggression": 0.2},
    "ORACLE":    {"brightness": 0.5,  "warmth": 0.45, "movement": 0.65, "density": 0.6, "space": 0.55, "aggression": 0.35},
    "OBSIDIAN":  {"brightness": 0.35, "warmth": 0.4,  "movement": 0.4,  "density": 0.65, "space": 0.5, "aggression": 0.4},
    "ORBITAL":   {"brightness": 0.6,  "warmth": 0.55, "movement": 0.55, "density": 0.5, "space": 0.5, "aggression": 0.3},
    "ONSET":     {"brightness": 0.5,  "warmth": 0.4,  "movement": 0.75, "density": 0.6, "space": 0.3, "aggression": 0.55},
    "OBLONG":    {"brightness": 0.5,  "warmth": 0.6,  "movement": 0.5,  "density": 0.5, "space": 0.35, "aggression": 0.2},
    "OUROBOROS": {"brightness": 0.45, "warmth": 0.35, "movement": 0.7,  "density": 0.5, "space": 0.3, "aggression": 0.5},
    "OVERWORLD": {"brightness": 0.55, "warmth": 0.4,  "movement": 0.6,  "density": 0.45, "space": 0.3, "aggression": 0.3},
    "OPTIC":     {"brightness": 0.8,  "warmth": 0.3,  "movement": 0.8,  "density": 0.4, "space": 0.4, "aggression": 0.3},
    "OBLIQUE":   {"brightness": 0.65, "warmth": 0.45, "movement": 0.7,  "density": 0.5, "space": 0.45, "aggression": 0.45},
    "OUTWIT":    {"brightness": 0.6,  "warmth": 0.4,  "movement": 0.8,  "density": 0.55, "space": 0.45, "aggression": 0.5},
    "ORIGAMI":   {"brightness": 0.55, "warmth": 0.5,  "movement": 0.6,  "density": 0.5, "space": 0.45, "aggression": 0.35},
    "OCEANIC":   {"brightness": 0.5,  "warmth": 0.55, "movement": 0.55, "density": 0.55, "space": 0.65, "aggression": 0.25},
}

# ---------------------------------------------------------------------------
# Name vocabulary
# ---------------------------------------------------------------------------

VOCAB = {
    "OMBRE":   ["Shadow", "Memory", "Gradient", "Fade", "Recall", "Dusk", "Twilight", "Liminal"],
    "ORCA":    ["Hunt", "Breach", "Echo", "Surge", "Apex", "Current", "Sonar", "Pursuit"],
    "OCTOPUS": ["Ink", "Arm", "Signal", "Shift", "Mimic", "Flash", "Pattern", "Adapt"],
    "TRIAD":   ["Deep Signal", "Apex Memory", "Shadow Breach", "Predator Dream", "Chromatic Hunt"],
}

# Partner secondary words (used to build "Word + Concept" names)
PARTNER_WORDS = {
    "ODYSSEY":   ["Tide", "Drift", "Wave", "Lull"],
    "OVERDUB":   ["Tape", "Dub", "Loop", "Echo"],
    "OPAL":      ["Shimmer", "Grain", "Scatter", "Mist"],
    "ORACLE":    ["Vision", "Break", "Fracture", "Dream"],
    "OBSIDIAN":  ["Glass", "Void", "Edge", "Cut"],
    "ORBITAL":   ["Ring", "Orbit", "Pulse", "Arc"],
    "ONSET":     ["Strike", "Hit", "Pulse", "Snap"],
    "OBLONG":    ["Curve", "Roll", "Wander", "Drift"],
    "OUROBOROS": ["Coil", "Loop", "Return", "Spiral"],
    "OVERWORLD": ["Pixel", "Chip", "Era", "Map"],
    "OPTIC":     ["Beam", "Flare", "Trace", "Scan"],
    "OBLIQUE":   ["Angle", "Bounce", "Prism", "Slant"],
    "OUTWIT":    ["Cell", "Arm", "Rule", "Evolve"],
    "ORIGAMI":   ["Fold", "Crease", "Form", "Unfold"],
    "OCEANIC":   ["Current", "Bloom", "Drift", "Depth"],
}

# Coupling types used across pairs
COUPLING_TYPES = [
    "Amp->Filter", "Env->Morph", "Audio->Wavetable", "Env->Decay",
    "Audio->FM", "Rhythm->Blend", "Env->Filter", "Amp->Morph",
]

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def blend_dna(*engine_names, weights=None):
    """Average DNA across named engines, optionally weighted."""
    keys = ["brightness", "warmth", "movement", "density", "space", "aggression"]
    if weights is None:
        weights = [1.0] * len(engine_names)
    total = sum(weights)
    result = {}
    for k in keys:
        result[k] = round(
            sum(DNA[e][k] * w for e, w in zip(engine_names, weights)) / total,
            3,
        )
    return result


def _nudge(dna, seed_val):
    """Apply a small deterministic per-preset variation so 3 variants differ."""
    rng = random.Random(seed_val)
    keys = ["brightness", "warmth", "movement", "density", "space", "aggression"]
    return {k: round(min(1.0, max(0.0, dna[k] + rng.uniform(-0.06, 0.06))), 3) for k in keys}


def make_preset(name, desc, tags, engine_a, engine_b, coupling_type, amount, dna,
                engine_b2=None):
    """Build a .xometa dict. engine_b2 is for 3-engine triad presets."""
    engines = [engine_a, engine_b]
    pairs = [{"engineA": engine_a, "engineB": engine_b,
              "type": coupling_type, "amount": round(amount, 3)}]
    if engine_b2:
        engines.append(engine_b2)
        # secondary coupling: B → B2
        pairs.append({"engineA": engine_b, "engineB": engine_b2,
                      "type": coupling_type, "amount": round(amount * 0.8, 3)})

    intensity = "Deep" if amount >= 0.7 else "Moderate"

    return {
        "schema_version": 1,
        "name": name,
        "mood": "Entangled",
        "engines": engines,
        "author": "XO_OX Designs",
        "version": "1.0.0",
        "description": desc,
        "tags": ["coupling"] + tags,
        "macroLabels": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
        "couplingIntensity": intensity,
        "tempo": None,
        "dna": dna,
        "parameters": {},
        "coupling": {"pairs": pairs},
        "sequencer": None,
    }


def _prefix(engine):
    """Return the canonical parameter prefix for an engine."""
    PREFIX_MAP = {
        "OMBRE": "ombre_", "ORCA": "orca_", "OCTOPUS": "octo_",
        "ODYSSEY": "drift_", "OVERDUB": "dub_", "OPAL": "opal_",
        "ORACLE": "oracle_", "OBSIDIAN": "obsidian_", "ORBITAL": "orb_",
        "ONSET": "perc_", "OBLONG": "bob_", "OUROBOROS": "ouro_",
        "OVERWORLD": "ow_", "OPTIC": "optic_", "OBLIQUE": "oblq_",
        "OUTWIT": "owit_", "ORIGAMI": "origami_", "OCEANIC": "ocean_",
    }
    return PREFIX_MAP.get(engine, engine.lower() + "_")

# ---------------------------------------------------------------------------
# Preset definitions
# ---------------------------------------------------------------------------

def build_ombre_pairs(rng):
    """18 presets: OMBRE × {ODYSSEY, OVERDUB, OPAL, ORACLE, OBSIDIAN, ORBITAL} × 3 variants."""
    pairs = [
        # (partner, coupling_type, base_amount, base_desc_fragment, base_tags)
        ("ODYSSEY",  "Env->Morph",       0.6,  "tidal memory",   ["odyssey", "ombre", "memory"]),
        ("OVERDUB",  "Audio->Wavetable", 0.65, "tape shadow",    ["overdub", "ombre", "tape"]),
        ("OPAL",     "Amp->Filter",      0.55, "grain twilight", ["opal", "ombre", "grain"]),
        ("ORACLE",   "Env->Filter",      0.6,  "liminal vision", ["oracle", "ombre", "vision"]),
        ("OBSIDIAN", "Amp->Morph",       0.7,  "void memory",    ["obsidian", "ombre", "dark"]),
        ("ORBITAL",  "Rhythm->Blend",    0.55, "orbital fade",   ["orbital", "ombre", "ring"]),
    ]
    presets = []
    ombre_words = VOCAB["OMBRE"]
    for partner, ctype, base_amt, desc_frag, tags in pairs:
        partner_words = PARTNER_WORDS[partner]
        for variant_idx in range(3):
            seed = rng.randint(0, 99999)
            w1 = ombre_words[(variant_idx * 3) % len(ombre_words)]
            w2 = partner_words[variant_idx % len(partner_words)]
            name = f"{w1} {w2}"
            amt = round(base_amt + rng.uniform(-0.05, 0.05), 2)
            base_dna = blend_dna("OMBRE", partner, weights=[0.6, 0.4])
            dna = _nudge(base_dna, seed)
            desc = (f"OMBRE {desc_frag}: {_prefix('OMBRE')}blend modulates "
                    f"{_prefix(partner)}filter. Shadow memory coupling.")
            presets.append(make_preset(name, desc, tags, "OMBRE", partner, ctype, amt, dna))
    return presets


def build_orca_pairs(rng):
    """18 presets: ORCA × {ONSET, OBLONG, OUROBOROS, ORACLE, OVERWORLD, OBSIDIAN} × 3 variants."""
    pairs = [
        ("ONSET",     "Rhythm->Blend",    0.75, "hunt rhythm",    ["onset", "orca", "percussion"]),
        ("OBLONG",    "Amp->Filter",      0.65, "surge curiosity",["oblong", "orca", "bass"]),
        ("OUROBOROS", "Audio->FM",        0.7,  "apex chaos",     ["ouroboros", "orca", "chaos"]),
        ("ORACLE",    "Env->Morph",       0.6,  "sonar vision",   ["oracle", "orca", "depth"]),
        ("OVERWORLD", "Audio->Wavetable", 0.65, "deep pixel",     ["overworld", "orca", "chip"]),
        ("OBSIDIAN",  "Env->Decay",       0.75, "abyss breach",   ["obsidian", "orca", "dark"]),
    ]
    presets = []
    orca_words = VOCAB["ORCA"]
    for partner, ctype, base_amt, desc_frag, tags in pairs:
        partner_words = PARTNER_WORDS[partner]
        for variant_idx in range(3):
            seed = rng.randint(0, 99999)
            w1 = orca_words[(variant_idx * 2 + 1) % len(orca_words)]
            w2 = partner_words[variant_idx % len(partner_words)]
            name = f"{w1} {w2}"
            amt = round(base_amt + rng.uniform(-0.05, 0.05), 2)
            base_dna = blend_dna("ORCA", partner, weights=[0.55, 0.45])
            dna = _nudge(base_dna, seed)
            desc = (f"ORCA {desc_frag}: {_prefix('ORCA')}huntMacro drives "
                    f"{_prefix(partner)}filter. Apex predator coupling.")
            presets.append(make_preset(name, desc, tags, "ORCA", partner, ctype, amt, dna))
    return presets


def build_octopus_pairs(rng):
    """18 presets: OCTOPUS × {ORACLE, OPTIC, OBLIQUE, OUTWIT, ORIGAMI, OCEANIC} × 3 variants."""
    pairs = [
        ("ORACLE",   "Env->Filter",      0.65, "ink vision",     ["oracle", "octopus", "signal"]),
        ("OPTIC",    "Amp->Morph",       0.6,  "flash beam",     ["optic", "octopus", "visual"]),
        ("OBLIQUE",  "Audio->Wavetable", 0.6,  "arm prism",      ["oblique", "octopus", "kinetic"]),
        ("OUTWIT",   "Audio->FM",        0.7,  "pattern cell",   ["outwit", "octopus", "ca"]),
        ("ORIGAMI",  "Env->Morph",       0.55, "shift fold",     ["origami", "octopus", "morph"]),
        ("OCEANIC",  "Rhythm->Blend",    0.6,  "mimic current",  ["oceanic", "octopus", "water"]),
    ]
    presets = []
    octo_words = VOCAB["OCTOPUS"]
    for partner, ctype, base_amt, desc_frag, tags in pairs:
        partner_words = PARTNER_WORDS[partner]
        for variant_idx in range(3):
            seed = rng.randint(0, 99999)
            w1 = octo_words[(variant_idx + 2) % len(octo_words)]
            w2 = partner_words[variant_idx % len(partner_words)]
            name = f"{w1} {w2}"
            amt = round(base_amt + rng.uniform(-0.05, 0.05), 2)
            base_dna = blend_dna("OCTOPUS", partner, weights=[0.6, 0.4])
            dna = _nudge(base_dna, seed)
            desc = (f"OCTOPUS {desc_frag}: {_prefix('OCTOPUS')}armDepth modulates "
                    f"{_prefix(partner)}filter. Chromatophore coupling.")
            presets.append(make_preset(name, desc, tags, "OCTOPUS", partner, ctype, amt, dna))
    return presets


def build_triads(rng):
    """9 presets: 3 engine triads × 3 variants each."""
    triad_names = VOCAB["TRIAD"]
    triads = [
        # (engineA, engineB, engineC, coupling_type, base_amount, tags)
        ("OMBRE", "ORCA", None,
         "Amp->Filter", 0.7,
         ["ombre", "orca", "triad", "predator-memory"]),
        ("ORCA", "OCTOPUS", None,
         "Audio->FM", 0.72,
         ["orca", "octopus", "triad", "apex-alien"]),
        ("OMBRE", "OCTOPUS", None,
         "Env->Morph", 0.65,
         ["ombre", "octopus", "triad", "shadow-shift"]),
    ]
    presets = []
    name_idx = 0
    for engA, engB, _, ctype, base_amt, tags in triads:
        # For triads the third engine is the other O3 member
        triad_members = {"OMBRE", "ORCA", "OCTOPUS"}
        triad_members.discard(engA)
        triad_members.discard(engB)
        engC = triad_members.pop()

        for variant_idx in range(3):
            seed = rng.randint(0, 99999)
            name = triad_names[(name_idx + variant_idx) % len(triad_names)]
            # Append a roman numeral suffix for the 3 variants
            suffix = ["I", "II", "III"][variant_idx]
            name = f"{name} {suffix}"
            amt = round(base_amt + rng.uniform(-0.04, 0.04), 2)
            base_dna = blend_dna(engA, engB, engC, weights=[0.4, 0.35, 0.25])
            dna = _nudge(base_dna, seed)
            desc = (f"Three-engine coupling: {engA} → {engB} → {engC}. "
                    f"{_prefix(engA)}, {_prefix(engB)}, {_prefix(engC)} — "
                    f"shadow, apex, chromatophore entangled.")
            preset = make_preset(name, desc, tags, engA, engB, ctype, amt, dna,
                                 engine_b2=engC)
            presets.append(preset)
        name_idx += 1
    return presets


# ---------------------------------------------------------------------------
# I/O
# ---------------------------------------------------------------------------

def write_preset(preset, output_dir, dry_run):
    mood_dir = Path(output_dir) / "XOmnibus" / preset["mood"]
    filepath = mood_dir / (preset["name"] + ".xometa")
    if dry_run:
        return filepath, False
    mood_dir.mkdir(parents=True, exist_ok=True)
    with open(filepath, "w") as f:
        json.dump(preset, f, indent=2)
        f.write("\n")
    return filepath, True


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description="Generate OMBRE / ORCA / OCTOPUS coupling presets for XOmnibus."
    )
    default_out = Path(__file__).resolve().parent.parent / "Presets"
    parser.add_argument(
        "--output-dir",
        default=str(default_out),
        help="Root Presets directory (default: ../Presets relative to this script)",
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
        help="Random seed for variant generation (default: 42)",
    )
    args = parser.parse_args()

    rng = random.Random(args.seed)

    all_presets = (
        build_ombre_pairs(rng)
        + build_orca_pairs(rng)
        + build_octopus_pairs(rng)
        + build_triads(rng)
    )

    written = 0
    skipped = 0
    for preset in all_presets:
        filepath, did_write = write_preset(preset, args.output_dir, args.dry_run)
        if args.dry_run:
            print(f"  [dry-run] {preset['mood']}/{preset['name']}.xometa  engines={preset['engines']}")
            skipped += 1
        else:
            print(f"  wrote    {filepath}")
            written += 1

    total = len(all_presets)
    if args.dry_run:
        print(f"\nDry run complete — {total} presets would be written (0 files created).")
    else:
        print(f"\nDone — {written}/{total} presets written to {args.output_dir}/XOmnibus/Entangled/")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
