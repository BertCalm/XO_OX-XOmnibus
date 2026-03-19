#!/usr/bin/env python3
"""Generate coupling presets for OCEANIC and OCELOT engine pairs.

OCEANIC (Phosphorescent Teal #00B4A0) — phosphorescent bio-separation engine
OCELOT  (Ocelot Tawny #C5832B)        — biome-based engine

Generates:
  OCEANIC × 7 partners  =  7 × 3  = 21 presets
  OCELOT  × 7 partners  =  7 × 3  = 21 presets
  OCEANIC + OCELOT pair =  4 presets
  Total                 = 46 presets
"""

import argparse
import json
import random
from pathlib import Path

# ---------------------------------------------------------------------------
# Sonic DNA baselines
# ---------------------------------------------------------------------------

DNA = {
    "OCEANIC":   {"brightness": 0.65, "warmth": 0.55, "movement": 0.50, "density": 0.55, "space": 0.75, "aggression": 0.30},
    "OCELOT":    {"brightness": 0.65, "warmth": 0.60, "movement": 0.60, "density": 0.55, "space": 0.60, "aggression": 0.50},
    "ORACLE":    {"brightness": 0.50, "warmth": 0.40, "movement": 0.60, "density": 0.70, "space": 0.70, "aggression": 0.40},
    "ORGANON":   {"brightness": 0.60, "warmth": 0.60, "movement": 0.65, "density": 0.75, "space": 0.65, "aggression": 0.40},
    "ORIGAMI":   {"brightness": 0.70, "warmth": 0.45, "movement": 0.65, "density": 0.55, "space": 0.55, "aggression": 0.55},
    "OBLIQUE":   {"brightness": 0.75, "warmth": 0.40, "movement": 0.70, "density": 0.50, "space": 0.60, "aggression": 0.55},
    "ORBITAL":   {"brightness": 0.60, "warmth": 0.65, "movement": 0.55, "density": 0.60, "space": 0.60, "aggression": 0.50},
    "OPTIC":     {"brightness": 0.85, "warmth": 0.30, "movement": 0.90, "density": 0.40, "space": 0.50, "aggression": 0.45},
    "OWLFISH":   {"brightness": 0.40, "warmth": 0.60, "movement": 0.40, "density": 0.65, "space": 0.80, "aggression": 0.20},
    "OBLONG":    {"brightness": 0.60, "warmth": 0.65, "movement": 0.50, "density": 0.65, "space": 0.55, "aggression": 0.50},
    "ODYSSEY":   {"brightness": 0.55, "warmth": 0.50, "movement": 0.70, "density": 0.50, "space": 0.70, "aggression": 0.30},
    "OVERWORLD": {"brightness": 0.75, "warmth": 0.40, "movement": 0.50, "density": 0.65, "space": 0.55, "aggression": 0.60},
}

# ---------------------------------------------------------------------------
# Engine metadata
# ---------------------------------------------------------------------------

ENGINE_PREFIX = {
    "OCEANIC":   "ocean_",
    "OCELOT":    "ocelot_",
    "ORACLE":    "ora_",
    "ORGANON":   "org_",
    "ORIGAMI":   "ori_",
    "OBLIQUE":   "obl_",
    "ORBITAL":   "orb_",
    "OPTIC":     "opt_",
    "OWLFISH":   "owl_",
    "OBLONG":    "bob_",
    "ODYSSEY":   "ody_",
    "OVERWORLD": "ow_",
}

# Display name used in the engines array (matches XOmnibus registration)
ENGINE_NAME = {
    "OCEANIC":   "XOceanic",
    "OCELOT":    "XOcelot",
    "ORACLE":    "XOracle",
    "ORGANON":   "XOrganon",
    "ORIGAMI":   "XOrigami",
    "OBLIQUE":   "XOblique",
    "ORBITAL":   "XOrbital",
    "OPTIC":     "XOptic",
    "OWLFISH":   "XOwlfish",
    "OBLONG":    "XOblong",
    "ODYSSEY":   "XOdyssey",
    "OVERWORLD": "XOverworld",
}

# ---------------------------------------------------------------------------
# Name vocabulary
# ---------------------------------------------------------------------------

OCEANIC_VOCAB = [
    "Bioluminescent", "Phosphorescent", "Teal Current", "Deep Glow",
    "Separation", "Bio-Signal", "Marine Layer",
]

OCELOT_VOCAB = [
    "Biome", "Tawny", "Savanna", "Prowl",
    "Territorial", "Dappled", "Habitat", "Range",
]

OCEANIC_OCELOT_VOCAB = [
    "Bioluminescent Biome", "Teal Habitat", "Marine Territory", "Phosphor Prowl",
]

# Coupling types that make musical sense
COUPLING_TYPES = [
    "Amp->Filter",
    "Env->Morph",
    "Audio->Wavetable",
    "Rhythm->Blend",
    "LFO->Cutoff",
    "Mod->Space",
    "Filter->Filter",
]

# ---------------------------------------------------------------------------
# Coupling type descriptions per pair characteristic
# ---------------------------------------------------------------------------

PARTNER_COUPLING_NOTES = {
    # OCEANIC partners
    ("OCEANIC", "ORACLE"):    ("Env->Morph",      "OCEANIC bio-signal modulates ORACLE temporal fold depth."),
    ("OCEANIC", "ORGANON"):   ("Audio->Wavetable", "OCEANIC separation spectrum feeds ORGANON partial banks."),
    ("OCEANIC", "ORIGAMI"):   ("Mod->Space",       "OCEANIC marine layer expands into ORIGAMI fold geometry."),
    ("OCEANIC", "OBLIQUE"):   ("LFO->Cutoff",      "OCEANIC phosphorescent pulse steers OBLIQUE spectral tilt."),
    ("OCEANIC", "ORBITAL"):   ("Filter->Filter",   "OCEANIC bio-separation filter couples with ORBITAL resonant ring."),
    ("OCEANIC", "OPTIC"):     ("Amp->Filter",      "OCEANIC deep glow amplitude opens OPTIC brightness gate."),
    ("OCEANIC", "OWLFISH"):   ("Rhythm->Blend",    "OCEANIC current rhythm blends into OWLFISH subharmonic pressure."),
    # OCELOT partners
    ("OCELOT", "ORACLE"):     ("Env->Morph",       "OCELOT prowl envelope shapes ORACLE temporal perception."),
    ("OCELOT", "OBLONG"):     ("Amp->Filter",      "OCELOT territorial snap triggers OBLONG curiosity filter."),
    ("OCELOT", "ODYSSEY"):    ("Rhythm->Blend",    "OCELOT habitat rhythm blends with ODYSSEY tidal drift."),
    ("OCELOT", "OBLIQUE"):    ("LFO->Cutoff",      "OCELOT savanna LFO steers OBLIQUE spectral angle."),
    ("OCELOT", "ORIGAMI"):    ("Mod->Space",       "OCELOT dappled light modulates ORIGAMI fold depth."),
    ("OCELOT", "ORBITAL"):    ("Filter->Filter",   "OCELOT range filter couples with ORBITAL resonant sweep."),
    ("OCELOT", "OVERWORLD"):  ("Audio->Wavetable", "OCELOT biome audio injects into OVERWORLD ERA wavetable."),
    # OCEANIC + OCELOT cross
    ("OCEANIC", "OCELOT"):    ("Amp->Filter",      "Phosphorescent teal meets tawny biome — marine meets savanna."),
}

# ---------------------------------------------------------------------------
# Helper: blend two DNA dicts (equal weight)
# ---------------------------------------------------------------------------

def blend_dna(a: dict, b: dict, weight_a: float = 0.5) -> dict:
    weight_b = 1.0 - weight_a
    return {k: round(a[k] * weight_a + b[k] * weight_b, 4) for k in a}


def clamp(v: float, lo: float = 0.0, hi: float = 1.0) -> float:
    return max(lo, min(hi, v))


def jitter_dna(dna: dict, rng: random.Random, amount: float = 0.05) -> dict:
    return {k: round(clamp(v + rng.uniform(-amount, amount)), 4) for k, v in dna.items()}


# ---------------------------------------------------------------------------
# Stub parameter blocks
# ---------------------------------------------------------------------------

def make_stub_params(engine_key: str, dna: dict, rng: random.Random) -> dict:
    prefix = ENGINE_PREFIX[engine_key]
    space  = dna["space"]
    bright = dna["brightness"]
    warm   = dna["warmth"]
    move   = dna["movement"]
    agg    = dna["aggression"]
    dens   = dna["density"]

    base = {
        f"{prefix}outputLevel":    round(0.75 + rng.uniform(-0.05, 0.05), 3),
        f"{prefix}outputPan":      0.0,
        f"{prefix}couplingLevel":  round(0.55 + move * 0.2, 3),
        f"{prefix}couplingBus":    0,
        f"{prefix}filterCutoff":   round(clamp(bright * 0.8 + 0.15), 3),
        f"{prefix}filterReso":     round(clamp(agg * 0.5), 3),
        f"{prefix}ampAttack":      round(10 + (1.0 - agg) * 40),
        f"{prefix}ampDecay":       round(200 + dens * 300),
        f"{prefix}ampSustain":     round(clamp(0.5 + warm * 0.3), 3),
        f"{prefix}ampRelease":     round(400 + space * 600),
        f"{prefix}reverbMix":      round(clamp(space * 0.6), 3),
        f"{prefix}reverbSize":     round(clamp(space * 0.8), 3),
        f"{prefix}reverbDamp":     round(clamp(1.0 - bright * 0.5), 3),
    }

    # Engine-specific extras
    if engine_key == "OCEANIC":
        base.update({
            f"{prefix}chromaShift":    round(clamp(move * 0.7), 3),
            f"{prefix}sepDepth":       round(clamp(bright * 0.6 + 0.1), 3),
            f"{prefix}bioRate":        round(clamp(move * 0.5 + 0.1), 3),
            f"{prefix}phosphorGlow":   round(clamp(1.0 - agg * 0.5), 3),
            f"{prefix}marineWidth":    round(clamp(space * 0.7 + 0.2), 3),
        })
    elif engine_key == "OCELOT":
        base.update({
            f"{prefix}biomeBlend":     round(clamp(warm * 0.6 + 0.2), 3),
            f"{prefix}prowlAmount":    round(clamp(agg * 0.7 + 0.1), 3),
            f"{prefix}habitatDepth":   round(clamp(space * 0.5 + 0.2), 3),
            f"{prefix}tawnyWarm":      round(clamp(warm * 0.8), 3),
            f"{prefix}rangeSize":      round(clamp(space * 0.6 + 0.15), 3),
        })

    return base


# ---------------------------------------------------------------------------
# Preset factory
# ---------------------------------------------------------------------------

def make_preset(
    name: str,
    engine_a: str,
    engine_b: str,
    coupling_type: str,
    description: str,
    dna: dict,
    rng: random.Random,
    coupling_amount: float = 0.6,
) -> dict:
    coupling_intensity = "Deep" if coupling_amount >= 0.7 else "Moderate"

    tags = [
        "coupling", "entangled",
        engine_a.lower(), engine_b.lower(),
        coupling_type.replace("->", "-").lower(),
    ]

    params = {}
    params[ENGINE_NAME[engine_a]] = make_stub_params(engine_a, dna, rng)
    params[ENGINE_NAME[engine_b]] = make_stub_params(engine_b, blend_dna(DNA[engine_a], DNA[engine_b], 0.5), rng)

    return {
        "schema_version": 1,
        "name": name,
        "mood": "Entangled",
        "sonic_dna": dna,
        "engines": [ENGINE_NAME[engine_a], ENGINE_NAME[engine_b]],
        "author": "XO_OX",
        "version": "1.0.0",
        "description": description,
        "tags": tags,
        "macroLabels": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
        "couplingIntensity": coupling_intensity,
        "dna": dna,
        "parameters": params,
        "coupling": {
            "pairs": [
                {
                    "engineA": ENGINE_NAME[engine_a],
                    "engineB": ENGINE_NAME[engine_b],
                    "type": coupling_type,
                    "amount": round(coupling_amount, 3),
                }
            ]
        },
        "sequencer": None,
    }


# ---------------------------------------------------------------------------
# Preset plan
# ---------------------------------------------------------------------------

def build_preset_plan(count: int, rng: random.Random) -> list[dict]:
    """Return list of (name, engine_a, engine_b, coupling_type, description, dna, coupling_amount)."""

    plan = []

    # --- OCEANIC pairs ---
    oceanic_partners = ["ORACLE", "ORGANON", "ORIGAMI", "OBLIQUE", "ORBITAL", "OPTIC", "OWLFISH"]
    for partner in oceanic_partners:
        key = ("OCEANIC", partner)
        coupling_type, base_desc = PARTNER_COUPLING_NOTES[key]
        blended = blend_dna(DNA["OCEANIC"], DNA[partner])
        for i in range(count):
            vocab_word = rng.choice(OCEANIC_VOCAB)
            partner_label = partner.capitalize()
            suffix = ["", " II", " III", " IV", " V"][i] if i < 5 else f" {i+1}"
            preset_name = f"{vocab_word} {partner_label}{suffix}".strip()
            jittered = jitter_dna(blended, rng, 0.04)
            amount = round(rng.uniform(0.50, 0.75), 3)
            plan.append({
                "name": preset_name,
                "engine_a": "OCEANIC",
                "engine_b": partner,
                "coupling_type": coupling_type,
                "description": base_desc,
                "dna": jittered,
                "coupling_amount": amount,
            })

    # --- OCELOT pairs ---
    ocelot_partners = ["ORACLE", "OBLONG", "ODYSSEY", "OBLIQUE", "ORIGAMI", "ORBITAL", "OVERWORLD"]
    for partner in ocelot_partners:
        key = ("OCELOT", partner)
        coupling_type, base_desc = PARTNER_COUPLING_NOTES[key]
        blended = blend_dna(DNA["OCELOT"], DNA[partner])
        for i in range(count):
            vocab_word = rng.choice(OCELOT_VOCAB)
            partner_label = partner.capitalize()
            suffix = ["", " II", " III", " IV", " V"][i] if i < 5 else f" {i+1}"
            preset_name = f"{vocab_word} {partner_label}{suffix}".strip()
            jittered = jitter_dna(blended, rng, 0.04)
            amount = round(rng.uniform(0.50, 0.75), 3)
            plan.append({
                "name": preset_name,
                "engine_a": "OCELOT",
                "engine_b": partner,
                "coupling_type": coupling_type,
                "description": base_desc,
                "dna": jittered,
                "coupling_amount": amount,
            })

    # --- OCEANIC + OCELOT cross (fixed 4) ---
    cross_key = ("OCEANIC", "OCELOT")
    cross_coupling_type, cross_base_desc = PARTNER_COUPLING_NOTES[cross_key]
    blended_cross = blend_dna(DNA["OCEANIC"], DNA["OCELOT"])
    for i, cross_name in enumerate(OCEANIC_OCELOT_VOCAB):
        jittered = jitter_dna(blended_cross, rng, 0.03)
        amount = round(rng.uniform(0.55, 0.75), 3)
        plan.append({
            "name": cross_name,
            "engine_a": "OCEANIC",
            "engine_b": "OCELOT",
            "coupling_type": cross_coupling_type,
            "description": cross_base_desc,
            "dna": jittered,
            "coupling_amount": amount,
        })

    return plan


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main() -> None:
    repo_root = Path(__file__).resolve().parent.parent
    default_output = repo_root / "Presets" / "XOmnibus" / "Entangled"

    parser = argparse.ArgumentParser(
        description="Generate OCEANIC + OCELOT coupling presets for XOmnibus."
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=default_output,
        help="Directory to write .xometa files (default: Presets/XOmnibus/Entangled/)",
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
        help="Number of presets per engine pair (default: 3). Affects pair groups, not the 4 cross presets.",
    )

    args = parser.parse_args()

    rng = random.Random(args.seed)
    plan = build_preset_plan(args.count, rng)

    if not args.dry_run:
        args.output_dir.mkdir(parents=True, exist_ok=True)

    written = 0
    skipped = 0

    for spec in plan:
        preset = make_preset(
            name=spec["name"],
            engine_a=spec["engine_a"],
            engine_b=spec["engine_b"],
            coupling_type=spec["coupling_type"],
            description=spec["description"],
            dna=spec["dna"],
            rng=rng,
            coupling_amount=spec["coupling_amount"],
        )

        filename = preset["name"] + ".xometa"
        filepath = args.output_dir / filename

        if args.dry_run:
            engines = " × ".join(preset["engines"])
            print(f"[DRY RUN] {filename}  ({engines}, {preset['coupling']['pairs'][0]['type']})")
            written += 1
        else:
            if filepath.exists():
                print(f"  SKIP (exists): {filename}")
                skipped += 1
                continue
            with open(filepath, "w", encoding="utf-8") as fh:
                json.dump(preset, fh, indent=2)
                fh.write("\n")
            print(f"  WRITE: {filename}")
            written += 1

    print()
    total = written + skipped
    if args.dry_run:
        print(f"Dry run complete — {written} presets would be written.")
    else:
        print(f"Done — {written} written, {skipped} skipped ({total} total).")


if __name__ == "__main__":
    main()
