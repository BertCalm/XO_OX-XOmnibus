#!/usr/bin/env python3
"""
xpn_overlap_outwit_deep_expansion.py

Generates 84 Entangled mood .xometa presets covering new coupling pairs for
OVERLAP and OUTWIT — the two newest XOmnibus engines.

Target pairs (6 presets each):
  OVERLAP × {ORGANON, OUROBOROS, OBSIDIAN, ORIGAMI, ORACLE, OBSCURA, OCEANIC} = 42
  OUTWIT  × {ORGANON, OUROBOROS, OBSIDIAN, ORIGAMI, ORACLE, OBSCURA, OCEANIC} = 42
  Total: 84 presets

OVERLAP (Lion's Mane jellyfish, knot-topology FDN, #00FFB4):
  lush, spatial, interconnected
  bias: high space (0.6-0.9), moderate movement (0.3-0.65), low-moderate aggression (0.05-0.4)

OUTWIT (Giant Pacific Octopus 8-arm Wolfram CA, #CC6600):
  chaotic, intelligent, generative
  bias: high movement (0.6-0.95), moderate-high density (0.45-0.9), variable aggression (0.2-0.85)

Each preset gets at least one Sonic DNA dimension at an extreme (≤0.15 or ≥0.85).
Coupling types rotate through all 12 in the MegaCouplingMatrix.
"""

import json
import random
from pathlib import Path

# ---------------------------------------------------------------------------
# Config
# ---------------------------------------------------------------------------

SEED = 2026_03_16
OUTPUT_DIR = (
    Path(__file__).resolve().parent.parent / "Presets" / "XOmnibus" / "Entangled"
)

COUPLING_TYPES = [
    "FREQUENCY_SHIFT",
    "AMPLITUDE_MOD",
    "FILTER_MOD",
    "PITCH_SYNC",
    "TIMBRE_BLEND",
    "ENVELOPE_LINK",
    "HARMONIC_FOLD",
    "CHAOS_INJECT",
    "RESONANCE_SHARE",
    "SPATIAL_COUPLE",
    "SPECTRAL_MORPH",
    "VELOCITY_COUPLE",
]

PARTNERS = ["ORGANON", "OUROBOROS", "OBSIDIAN", "ORIGAMI", "ORACLE", "OBSCURA", "OCEANIC"]

# Correct frozen parameter prefixes per CLAUDE.md
PARTNER_PREFIXES = {
    "ORGANON":   "organon_",
    "OUROBOROS": "ouro_",
    "OBSIDIAN":  "obsidian_",
    "ORIGAMI":   "origami_",
    "ORACLE":    "oracle_",
    "OBSCURA":   "obscura_",
    "OCEANIC":   "ocean_",
}

OVERLAP_PREFIX = "olap_"
OUTWIT_PREFIX  = "owit_"

# ---------------------------------------------------------------------------
# Partner engine flavour — used for names and descriptions
# ---------------------------------------------------------------------------

PARTNER_FLAVOUR = {
    "ORGANON":   ("logical-metabolic", "Aristotelian organ-pipe variational synthesis"),
    "OUROBOROS": ("recursive self-feeding", "strange-attractor topology"),
    "OBSIDIAN":  ("dark volcanic crystalline", "obsidian physical-modelling"),
    "ORIGAMI":   ("folded geometric", "Vermillion fold-based synthesis"),
    "ORACLE":    ("prophetic stochastic", "GENDY + Maqam liminal tones"),
    "OBSCURA":   ("daguerreotype-veiled spectral", "plate physics and silver grain"),
    "OCEANIC":   ("deep-current tidal", "phosphorescent abyssal pressure"),
}

# ---------------------------------------------------------------------------
# Name banks — 6 unique names per (primary × partner) pair
# ---------------------------------------------------------------------------

PRESET_NAMES: dict[tuple, list[str]] = {
    # OVERLAP pairs
    ("OVERLAP", "ORGANON"): [
        "Metabolic Frond",
        "Pipe Cathedral",
        "Organ Web",
        "Logical Tendril",
        "Aristotelian Bloom",
        "Biochemical Lattice",
    ],
    ("OVERLAP", "OUROBOROS"): [
        "Cyclic Bloom",
        "Tail-Swallow Halo",
        "Recursive Weave",
        "Serpent Canopy",
        "Eternal Frond",
        "Self-Threading Veil",
    ],
    ("OVERLAP", "OBSIDIAN"): [
        "Volcanic Veil",
        "Lava Glass Curtain",
        "Obsidian Tendril",
        "Dark Crystal Bloom",
        "Shard Lattice",
        "Magma Frond",
    ],
    ("OVERLAP", "ORIGAMI"): [
        "Paper Medusa",
        "Crease Diffusion",
        "Tessellated Bloom",
        "Origami Canopy",
        "Pleated Current",
        "Folded Frond",
    ],
    ("OVERLAP", "ORACLE"): [
        "Prophecy Cave",
        "Sibylline Haze",
        "Oracle Bloom",
        "Augury Lattice",
        "Vatic Tendril",
        "Liminal Weave",
    ],
    ("OVERLAP", "OBSCURA"): [
        "Silver Diffusion",
        "Daguerreotype Bloom",
        "Crepuscular Weave",
        "Veiled Canopy",
        "Spectral Lattice",
        "Shadow Frond",
    ],
    ("OVERLAP", "OCEANIC"): [
        "Abyssal Bloom",
        "Tidal Frond",
        "Deep Current Mesh",
        "Hadal Weave",
        "Benthic Canopy",
        "Brine Lattice",
    ],
    # OUTWIT pairs
    ("OUTWIT", "ORGANON"): [
        "Tactical Syllogism",
        "8-Arm Logic Gate",
        "Categorical Assault",
        "Predicate Strike",
        "Biochemical Swarm",
        "Method Chaos",
    ],
    ("OUTWIT", "OUROBOROS"): [
        "Recursive Arms",
        "Infinite Strike Pattern",
        "Serpent Swarm",
        "Cyclic Ambush",
        "Self-Feeding Chaos",
        "Tail-Loop Assault",
    ],
    ("OUTWIT", "OBSIDIAN"): [
        "Volcanic Swarm",
        "Obsidian Predator",
        "Dark Arm Slash",
        "Shard Storm",
        "Lava Strike",
        "Magma Assault",
    ],
    ("OUTWIT", "ORIGAMI"): [
        "Folded Strike",
        "Tessellated Chaos",
        "Paper Predator",
        "Crease Assault",
        "Origami Ambush",
        "Pleated Swarm",
    ],
    ("OUTWIT", "ORACLE"): [
        "Prophecy Code",
        "Augury Chaos",
        "Sibylline Swarm",
        "Vatic Predator",
        "Oracle Ambush",
        "Maqam Strike",
    ],
    ("OUTWIT", "OBSCURA"): [
        "Shadow Strike",
        "Silver Pattern",
        "Daguerreotype Chaos",
        "Veiled Assault",
        "Obscure Predator",
        "Crepuscular Strike",
    ],
    ("OUTWIT", "OCEANIC"): [
        "Deep Swarm",
        "Abyssal Strike",
        "Tidal Assault",
        "Hadal Predator",
        "Benthic Chaos",
        "Brine Ambush",
    ],
}

# ---------------------------------------------------------------------------
# DNA helpers
# ---------------------------------------------------------------------------

def r(lo: float, hi: float, rng: random.Random) -> float:
    return round(rng.uniform(lo, hi), 2)


def make_dna(rng: random.Random, overrides=None) -> dict:
    """
    Build a 6D Sonic DNA dict where at least one dimension is extreme (≤0.15 or ≥0.85).
    Optional overrides steer ranges for engine bias.
    """
    dims = {
        "brightness": (0.1, 0.9),
        "warmth":     (0.1, 0.9),
        "movement":   (0.1, 0.9),
        "density":    (0.1, 0.9),
        "space":      (0.1, 0.9),
        "aggression": (0.1, 0.9),
    }
    if overrides:
        dims.update(overrides)

    dna = {k: r(lo, hi, rng) for k, (lo, hi) in dims.items()}

    # Guarantee at least one extreme dimension
    extreme_key = rng.choice(list(dna.keys()))
    if rng.random() < 0.5:
        dna[extreme_key] = round(rng.uniform(0.0, 0.15), 2)
    else:
        dna[extreme_key] = round(rng.uniform(0.85, 1.0), 2)

    return dna


# ---------------------------------------------------------------------------
# Parameter stubs
# ---------------------------------------------------------------------------

def overlap_engine_params(rng: random.Random) -> dict:
    """OVERLAP (Lion's Mane FDN): biased toward high space, moderate movement, low aggression."""
    p = OVERLAP_PREFIX
    return {
        f"{p}size":          r(0.55, 0.95, rng),
        f"{p}decay":         r(0.50, 0.92, rng),
        f"{p}damping":       r(0.20, 0.60, rng),
        f"{p}diffusion":     r(0.50, 0.95, rng),
        f"{p}modDepth":      r(0.10, 0.50, rng),
        f"{p}modRate":       r(0.03, 0.35, rng),
        f"{p}preDelay":      r(0.00, 0.30, rng),
        f"{p}earlyMix":      r(0.20, 0.55, rng),
        f"{p}lateMix":       r(0.40, 0.88, rng),
        f"{p}filterCutoff":  r(0.35, 0.85, rng),
        f"{p}filterReso":    r(0.10, 0.45, rng),
        f"{p}frondDensity":  r(0.30, 0.80, rng),
        f"{p}nodeCount":     rng.randint(6, 16),
        f"{p}crossfeedAmt":  r(0.20, 0.65, rng),
        f"{p}spinRate":      r(0.02, 0.28, rng),
        f"{p}shimmerMix":    r(0.00, 0.45, rng),
        f"{p}couplingLevel": r(0.55, 0.90, rng),
        f"{p}couplingBus":   0,
        f"{p}outputLevel":   r(0.68, 0.88, rng),
        f"{p}outputPan":     round(rng.uniform(-0.25, 0.25), 2),
    }


def outwit_engine_params(rng: random.Random) -> dict:
    """OUTWIT (Giant Pacific Octopus Wolfram CA): biased toward high movement, dense, variable aggression."""
    p = OUTWIT_PREFIX
    return {
        f"{p}rule":          rng.choice([30, 54, 90, 110, 126, 150, 182, 220, 22, 45]),
        f"{p}arms":          8,
        f"{p}cellSize":      r(0.10, 0.55, rng),
        f"{p}density":       r(0.35, 0.85, rng),
        f"{p}evolveRate":    r(0.35, 0.85, rng),
        f"{p}mutateProb":    r(0.04, 0.40, rng),
        f"{p}pitchSpread":   r(0.20, 0.70, rng),
        f"{p}filterCutoff":  r(0.25, 0.85, rng),
        f"{p}filterReso":    r(0.10, 0.55, rng),
        f"{p}ampAttack":     r(0.01, 0.18, rng),
        f"{p}ampRelease":    r(0.15, 0.88, rng),
        f"{p}armStagger":    r(0.00, 0.50, rng),
        f"{p}borderMode":    rng.randint(0, 2),
        f"{p}inkCloudMix":   r(0.00, 0.50, rng),
        f"{p}chroma1":       r(0.30, 0.90, rng),
        f"{p}chroma2":       r(0.30, 0.90, rng),
        f"{p}polyDepth":     r(0.30, 0.85, rng),
        f"{p}couplingLevel": r(0.50, 0.90, rng),
        f"{p}couplingBus":   0,
        f"{p}outputLevel":   r(0.68, 0.88, rng),
        f"{p}outputPan":     round(rng.uniform(-0.25, 0.25), 2),
    }


def partner_engine_params(partner: str, rng: random.Random) -> dict:
    """Generic partner parameters using the correct frozen prefix from CLAUDE.md."""
    p = PARTNER_PREFIXES[partner]
    return {
        f"{p}character":      r(0.25, 0.80, rng),
        f"{p}movement":       r(0.25, 0.78, rng),
        f"{p}coupling":       r(0.50, 0.90, rng),
        f"{p}space":          r(0.20, 0.78, rng),
        f"{p}filterCutoff":   r(0.28, 0.82, rng),
        f"{p}filterReso":     r(0.08, 0.55, rng),
        f"{p}ampAttack":      r(0.04, 0.40, rng),
        f"{p}ampSustain":     r(0.40, 0.90, rng),
        f"{p}ampRelease":     r(0.18, 0.75, rng),
        f"{p}reverbMix":      r(0.08, 0.60, rng),
        f"{p}couplingLevel":  r(0.50, 0.88, rng),
        f"{p}outputLevel":    r(0.68, 0.88, rng),
    }


# ---------------------------------------------------------------------------
# DNA bias ranges per engine
# ---------------------------------------------------------------------------

OVERLAP_DNA_BIAS = {
    "space":      (0.60, 0.92),
    "movement":   (0.28, 0.65),
    "aggression": (0.05, 0.40),
}

OUTWIT_DNA_BIAS = {
    "movement":   (0.60, 0.95),
    "density":    (0.45, 0.90),
}


def overlap_dna(rng: random.Random) -> dict:
    return make_dna(rng, overrides=OVERLAP_DNA_BIAS)


def outwit_dna(rng: random.Random) -> dict:
    return make_dna(rng, overrides=OUTWIT_DNA_BIAS)


# ---------------------------------------------------------------------------
# Preset factory
# ---------------------------------------------------------------------------

def make_preset(
    primary: str,
    partner: str,
    name: str,
    coupling_type: str,
    rng: random.Random,
) -> dict:
    if primary == "OVERLAP":
        primary_dna = overlap_dna(rng)
        primary_params = overlap_engine_params(rng)
        macro_labels = ["TOPOLOGY", "DIFFUSION", "COUPLING", "SPACE"]
        flavour = PARTNER_FLAVOUR[partner]
        description = (
            f"Lion's Mane knot-topology FDN entangled with {partner}'s "
            f"{flavour[0]} character ({flavour[1]}). "
            f"Coupling: {coupling_type.replace('_', ' ').title()}."
        )
        tags = ["entangled", "coupling", "overlap", partner.lower(), "spatial", "lush"]
    else:
        primary_dna = outwit_dna(rng)
        primary_params = outwit_engine_params(rng)
        macro_labels = ["ARMS", "CHAOS", "COUPLING", "DENSITY"]
        flavour = PARTNER_FLAVOUR[partner]
        description = (
            f"Giant Pacific Octopus 8-arm Wolfram CA entangled with {partner}'s "
            f"{flavour[0]} character ({flavour[1]}). "
            f"Coupling: {coupling_type.replace('_', ' ').title()}."
        )
        tags = ["entangled", "coupling", "outwit", partner.lower(), "chaotic", "generative"]

    coupling_amount = r(0.50, 0.92, rng)
    intensity = "Deep" if coupling_amount >= 0.72 else "Moderate"

    return {
        "schema_version": 1,
        "name": name,
        "mood": "Entangled",
        "engines": [primary, partner],
        "author": "XO_OX",
        "version": "1.0.0",
        "description": description,
        "tags": tags,
        "macroLabels": macro_labels,
        "macros": {
            "CHARACTER": r(0.35, 0.78, rng),
            "MOVEMENT":  r(0.35, 0.78, rng),
            "COUPLING":  r(0.55, 0.92, rng),
            "SPACE":     r(0.40, 0.88, rng),
        },
        "couplingIntensity": intensity,
        "sonic_dna": primary_dna,
        "dna": primary_dna,
        "parameters": {
            primary: primary_params,
            partner: partner_engine_params(partner, rng),
        },
        "coupling": {
            "type": coupling_type,
            "source": primary,
            "target": partner,
            "amount": coupling_amount,
            "pairs": [
                {
                    "engineA": primary,
                    "engineB": partner,
                    "type": coupling_type,
                    "amount": coupling_amount,
                }
            ],
        },
    }


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def snake_case(name: str) -> str:
    return (
        name.lower()
        .replace(" ", "_")
        .replace("-", "_")
        .replace("'", "")
        .replace(",", "")
        .replace("/", "_")
    )


def main() -> None:
    rng = random.Random(SEED)
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

    coupling_idx = 0
    written = 0
    skipped = 0

    for primary in ("OVERLAP", "OUTWIT"):
        for partner in PARTNERS:
            names = PRESET_NAMES[(primary, partner)]
            for name in names:
                coupling_type = COUPLING_TYPES[coupling_idx % len(COUPLING_TYPES)]
                coupling_idx += 1

                filename = snake_case(name) + ".xometa"
                filepath = OUTPUT_DIR / filename

                if filepath.exists():
                    skipped += 1
                    continue

                preset = make_preset(primary, partner, name, coupling_type, rng)

                with open(filepath, "w", encoding="utf-8") as f:
                    json.dump(preset, f, indent=2, ensure_ascii=False)
                    f.write("\n")

                written += 1
                print(f"  wrote: {filename}")

    total = written + skipped
    print(f"\nDone — {written} written, {skipped} skipped ({total} total)")
    print(f"Output: {OUTPUT_DIR}")


if __name__ == "__main__":
    main()
