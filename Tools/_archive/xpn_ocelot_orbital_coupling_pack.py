#!/usr/bin/env python3
"""
xpn_ocelot_orbital_coupling_pack.py — XO_OX Designs
Seed coupling coverage for OCELOT and ORBITAL.

Generates 46 stub .xometa presets in Entangled mood:
  - 4  OCELOT × ORBITAL marquee presets
  - 18 OCELOT new-pair presets  (OVERDUB, OBESE, ONSET, OBBLIGATO, OLE, OHM × 3 each)
  - 24 ORBITAL new-pair presets (OPAL, ORACLE, OBESE, ONSET, OUROBOROS, ODYSSEY, OHM, ORPHICA × 3 each)

Usage:
    python Tools/xpn_ocelot_orbital_coupling_pack.py
    python Tools/xpn_ocelot_orbital_coupling_pack.py --dry-run
    python Tools/xpn_ocelot_orbital_coupling_pack.py --seed 42 --count 2
    python Tools/xpn_ocelot_orbital_coupling_pack.py --output-dir /tmp/test_out
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
    "OCELOT":    dict(brightness=0.65, warmth=0.60, movement=0.60, density=0.55, space=0.60, aggression=0.50),
    "ORBITAL":   dict(brightness=0.60, warmth=0.65, movement=0.55, density=0.60, space=0.60, aggression=0.50),
    "OVERDUB":   dict(brightness=0.45, warmth=0.70, movement=0.55, density=0.55, space=0.75, aggression=0.25),
    "OBESE":     dict(brightness=0.50, warmth=0.70, movement=0.45, density=0.80, space=0.40, aggression=0.75),
    "ONSET":     dict(brightness=0.55, warmth=0.50, movement=0.80, density=0.75, space=0.50, aggression=0.70),
    "OBBLIGATO": dict(brightness=0.55, warmth=0.65, movement=0.60, density=0.65, space=0.60, aggression=0.45),
    "OLE":       dict(brightness=0.65, warmth=0.70, movement=0.75, density=0.60, space=0.60, aggression=0.55),
    "OHM":       dict(brightness=0.45, warmth=0.75, movement=0.55, density=0.60, space=0.70, aggression=0.30),
    "OPAL":      dict(brightness=0.70, warmth=0.50, movement=0.75, density=0.45, space=0.80, aggression=0.20),
    "ORACLE":    dict(brightness=0.50, warmth=0.40, movement=0.60, density=0.70, space=0.70, aggression=0.40),
    "OUROBOROS": dict(brightness=0.50, warmth=0.40, movement=0.85, density=0.75, space=0.50, aggression=0.80),
    "ODYSSEY":   dict(brightness=0.55, warmth=0.50, movement=0.70, density=0.50, space=0.70, aggression=0.30),
    "ORPHICA":   dict(brightness=0.80, warmth=0.50, movement=0.70, density=0.45, space=0.75, aggression=0.25),
}

# ---------------------------------------------------------------------------
# Engine → parameter prefix mapping (from CLAUDE.md)
# ---------------------------------------------------------------------------
ENGINE_PREFIX = {
    "OCELOT":    "ocelot_",
    "ORBITAL":   "orb_",
    "OVERDUB":   "dub_",
    "OBESE":     "fat_",
    "ONSET":     "perc_",
    "OBBLIGATO": "obbl_",
    "OLE":       "ole_",
    "OHM":       "ohm_",
    "OPAL":      "opal_",
    "ORACLE":    "oracle_",
    "OUROBOROS": "ouro_",
    "ODYSSEY":   "drift_",
    "ORPHICA":   "orph_",
}

# Engine ID strings used in the preset "engines" array
ENGINE_ID = {
    "OCELOT":    "Ocelot",
    "ORBITAL":   "Orbital",
    "OVERDUB":   "Overdub",
    "OBESE":     "Obese",
    "ONSET":     "Onset",
    "OBBLIGATO": "Obbligato",
    "OLE":       "Ole",
    "OHM":       "Ohm",
    "OPAL":      "Opal",
    "ORACLE":    "Oracle",
    "OUROBOROS": "Ouroboros",
    "ODYSSEY":   "Odyssey",
    "ORPHICA":   "Orphica",
}

# ---------------------------------------------------------------------------
# Name vocabularies
# ---------------------------------------------------------------------------
VOCAB_OCELOT  = ["Biome", "Tawny", "Dappled", "Prowl", "Territorial", "Range", "Habitat", "Savanna"]
VOCAB_ORBITAL = ["Group", "Envelope", "Phase", "Arc", "Cycle", "Orbit", "Resonant", "Harmonic"]

COUPLING_TYPES = ["timbral_blend", "rhythm_sync", "filter_modulation", "amplitude_sidechain",
                  "pitch_follow", "envelope_share", "harmonic_lock", "spectral_cross"]

COUPLING_INTENSITIES = ["Subtle", "Moderate", "Strong", "Extreme"]

DATE = "2026-03-16"

# ---------------------------------------------------------------------------
# DNA helpers
# ---------------------------------------------------------------------------
def blend_dna(engine_a: str, engine_b: str, rng: random.Random) -> dict:
    """Average DNA of two engines, then apply a small random nudge."""
    a = DNA_BASELINES[engine_a]
    b = DNA_BASELINES[engine_b]
    keys = list(a.keys())
    blended = {}
    for k in keys:
        mid = (a[k] + b[k]) / 2.0
        # ±0.05 jitter, clamped to [0.0, 1.0]
        nudge = rng.uniform(-0.05, 0.05)
        blended[k] = round(max(0.0, min(1.0, mid + nudge)), 3)
    return blended


# ---------------------------------------------------------------------------
# Parameter stub builders
# ---------------------------------------------------------------------------
def make_ocelot_params(rng: random.Random) -> dict:
    p = "ocelot_"
    return {
        f"{p}biome":       round(rng.uniform(0.3, 0.8), 3),
        f"{p}tawnyDrive":  round(rng.uniform(0.3, 0.7), 3),
        f"{p}prowlRate":   round(rng.uniform(0.2, 0.8), 3),
        f"{p}rangeFilter": round(rng.uniform(0.3, 0.9), 3),
        f"{p}habitatMix":  round(rng.uniform(0.3, 0.7), 3),
        f"{p}macro1":      round(rng.uniform(0.3, 0.7), 3),
        f"{p}macro2":      round(rng.uniform(0.3, 0.7), 3),
        f"{p}macro3":      round(rng.uniform(0.3, 0.7), 3),
        f"{p}macro4":      round(rng.uniform(0.3, 0.7), 3),
        f"{p}couplingOut": round(rng.uniform(0.4, 0.8), 3),
    }


def make_orbital_params(rng: random.Random) -> dict:
    p = "orb_"
    return {
        f"{p}brightness":   round(rng.uniform(0.4, 0.8), 3),
        f"{p}groupEnv":     round(rng.uniform(0.3, 0.7), 3),
        f"{p}phaseSpread":  round(rng.uniform(0.2, 0.8), 3),
        f"{p}arcDecay":     round(rng.uniform(0.3, 0.9), 3),
        f"{p}resonantPeak": round(rng.uniform(0.2, 0.7), 3),
        f"{p}harmonicAmt":  round(rng.uniform(0.3, 0.7), 3),
        f"{p}macro1":       round(rng.uniform(0.3, 0.7), 3),
        f"{p}macro2":       round(rng.uniform(0.3, 0.7), 3),
        f"{p}macro3":       round(rng.uniform(0.3, 0.7), 3),
        f"{p}macro4":       round(rng.uniform(0.3, 0.7), 3),
        f"{p}couplingIn":   round(rng.uniform(0.4, 0.8), 3),
    }


def make_partner_params(engine: str, rng: random.Random) -> dict:
    prefix = ENGINE_PREFIX[engine]
    return {
        f"{prefix}macro1":      round(rng.uniform(0.3, 0.7), 3),
        f"{prefix}macro2":      round(rng.uniform(0.3, 0.7), 3),
        f"{prefix}macro3":      round(rng.uniform(0.3, 0.7), 3),
        f"{prefix}macro4":      round(rng.uniform(0.3, 0.7), 3),
        f"{prefix}couplingOut": round(rng.uniform(0.4, 0.8), 3),
        f"{prefix}couplingIn":  round(rng.uniform(0.4, 0.8), 3),
    }


# ---------------------------------------------------------------------------
# Preset builder
# ---------------------------------------------------------------------------
def make_preset(name: str, primary: str, partner: str,
                rng: random.Random, idx: int) -> dict:
    """Build a single .xometa stub for primary ↔ partner coupling."""
    dna = blend_dna(primary, partner, rng)
    ctype = rng.choice(COUPLING_TYPES)
    intensity = rng.choice(COUPLING_INTENSITIES)
    tempo = rng.choice([90, 100, 105, 110, 115, 120, 125, 130])

    # Build parameters block — one key per engine
    params = {}
    if primary == "OCELOT":
        params[ENGINE_ID["OCELOT"]] = make_ocelot_params(rng)
    else:
        params[ENGINE_ID["ORBITAL"]] = make_orbital_params(rng)
    params[ENGINE_ID[partner]] = make_partner_params(partner, rng)

    # Coupling pairs
    coupling_pairs = [
        {
            "engineA": ENGINE_ID[primary],
            "engineB": ENGINE_ID[partner],
            "type": ctype,
            "amount": round(rng.uniform(0.4, 0.85), 3),
        }
    ]

    engines = [ENGINE_ID[primary], ENGINE_ID[partner]]

    # Description
    primary_lc = primary.lower().capitalize()
    partner_lc = partner.lower().capitalize()
    desc = (
        f"{primary_lc} × {partner_lc} coupling ({ctype.replace('_', ' ')}). "
        f"Stub preset — sound-design pass pending."
    )

    tags = [primary.lower(), partner.lower(), ctype.replace("_", "-"), "coupling", "entangled"]

    return {
        "schema_version": 1,
        "name": name,
        "mood": "Entangled",
        "engines": engines,
        "author": "XO_OX",
        "version": "1.0.0",
        "description": desc,
        "tags": tags,
        "macroLabels": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
        "couplingIntensity": intensity,
        "tempo": tempo,
        "created": DATE,
        "legacy": {
            "sourceInstrument": None,
            "sourceCategory": None,
            "sourcePresetName": None,
        },
        "parameters": params,
        "coupling": {"pairs": coupling_pairs},
        "sequencer": None,
        "dna": dna,
    }


# ---------------------------------------------------------------------------
# Name generation helpers
# ---------------------------------------------------------------------------
def unique_name(base_vocab_a: list, base_vocab_b: list,
                used: set, rng: random.Random) -> str:
    """Generate a two-word name from vocab lists, avoiding duplicates."""
    attempts = 0
    while attempts < 200:
        w1 = rng.choice(base_vocab_a)
        w2 = rng.choice(base_vocab_b)
        candidate = f"{w1} {w2}"
        if candidate not in used:
            used.add(candidate)
            return candidate
        attempts += 1
    # Fallback with random suffix
    suffix = rng.randint(100, 999)
    name = f"{rng.choice(base_vocab_a)} {rng.choice(base_vocab_b)} {suffix}"
    used.add(name)
    return name


# ---------------------------------------------------------------------------
# Pair definitions
# ---------------------------------------------------------------------------
OCELOT_PAIRS = ["OVERDUB", "OBESE", "ONSET", "OBBLIGATO", "OLE", "OHM"]
ORBITAL_PAIRS = ["OPAL", "ORACLE", "OBESE", "ONSET", "OUROBOROS", "ODYSSEY", "OHM", "ORPHICA"]

# Secondary vocab banks keyed by partner engine
PARTNER_VOCAB = {
    "OVERDUB":   ["Echo", "Dub", "Tape", "Riddim", "Send", "Wash"],
    "OBESE":     ["Heavy", "Dense", "Thick", "Drive", "Saturate", "Mass"],
    "ONSET":     ["Strike", "Hit", "Snap", "Burst", "Trigger", "Punch"],
    "OBBLIGATO": ["Bond", "Breath", "Wind", "Weave", "Voice", "Oblige"],
    "OLE":       ["Drama", "Flare", "Sway", "Pulse", "Spin", "Surge"],
    "OHM":       ["Commune", "Hum", "Mellow", "Drift", "Still", "Sage"],
    "OPAL":      ["Grain", "Scatter", "Shimmer", "Cloud", "Particle", "Float"],
    "ORACLE":    ["Prophecy", "Stochastic", "Signal", "Omen", "Void", "Foresight"],
    "OUROBOROS": ["Serpent", "Loop", "Feed", "Chaos", "Gnaw", "Spiral"],
    "ODYSSEY":   ["Voyage", "Glide", "Wander", "Drift", "Journey", "Waypoint"],
    "ORPHICA":   ["Pluck", "Siren", "Harp", "Shimmer", "Microsound", "Filament"],
}


# ---------------------------------------------------------------------------
# Main generation logic
# ---------------------------------------------------------------------------
def generate_all(count: int, rng: random.Random) -> list:
    """Return list of (filename_stem, preset_dict)."""
    presets = []
    used_names: set = set()

    # ---- Marquee: OCELOT × ORBITAL (4 presets) ----
    marquee_names = [
        "Territorial Orbit",
        "Savanna Arc",
        "Dappled Phase",
        "Prowl Cycle",
    ]
    for mname in marquee_names:
        used_names.add(mname)
        preset = make_preset(mname, "OCELOT", "ORBITAL", rng, len(presets))
        presets.append(preset)

    # ---- OCELOT new pairs (count per partner) ----
    for partner in OCELOT_PAIRS:
        pvocab = PARTNER_VOCAB[partner]
        for _ in range(count):
            name = unique_name(VOCAB_OCELOT, pvocab, used_names, rng)
            preset = make_preset(name, "OCELOT", partner, rng, len(presets))
            presets.append(preset)

    # ---- ORBITAL new pairs (count per partner) ----
    for partner in ORBITAL_PAIRS:
        pvocab = PARTNER_VOCAB[partner]
        for _ in range(count):
            name = unique_name(VOCAB_ORBITAL, pvocab, used_names, rng)
            preset = make_preset(name, "ORBITAL", partner, rng, len(presets))
            presets.append(preset)

    return presets


# ---------------------------------------------------------------------------
# I/O helpers
# ---------------------------------------------------------------------------
def safe_filename(name: str) -> str:
    return name.replace(" ", "_").replace("/", "-") + ".xometa"


def write_preset(preset: dict, output_dir: Path) -> Path:
    output_dir.mkdir(parents=True, exist_ok=True)
    path = output_dir / safe_filename(preset["name"])
    with open(path, "w", encoding="utf-8") as f:
        json.dump(preset, f, indent=2)
        f.write("\n")
    return path


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------
def parse_args():
    parser = argparse.ArgumentParser(
        description="Generate OCELOT + ORBITAL coupling preset stubs."
    )
    repo_root = Path(__file__).resolve().parent.parent
    default_out = repo_root / "Presets" / "XOceanus" / "Entangled"
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=default_out,
        help=f"Directory to write .xometa files (default: {default_out})",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print preset names without writing files",
    )
    parser.add_argument(
        "--seed",
        type=int,
        default=None,
        help="Random seed for reproducibility",
    )
    parser.add_argument(
        "--count",
        type=int,
        default=3,
        help="Presets per partner pair (default: 3). Marquee is always 4.",
    )
    return parser.parse_args()


def main():
    args = parse_args()
    rng = random.Random(args.seed)

    presets = generate_all(args.count, rng)

    marquee_count = 4
    ocelot_pair_count = len(OCELOT_PAIRS) * args.count
    orbital_pair_count = len(ORBITAL_PAIRS) * args.count
    total = marquee_count + ocelot_pair_count + orbital_pair_count

    print(f"OCELOT × ORBITAL Coupling Pack")
    print(f"  Marquee presets  : {marquee_count}")
    print(f"  OCELOT pairs     : {len(OCELOT_PAIRS)} partners × {args.count} = {ocelot_pair_count}")
    print(f"  ORBITAL pairs    : {len(ORBITAL_PAIRS)} partners × {args.count} = {orbital_pair_count}")
    print(f"  Total            : {total}")
    print()

    if args.dry_run:
        print("[dry-run] Would write:")
        for p in presets:
            print(f"  {safe_filename(p['name'])}")
        print(f"\n[dry-run] {len(presets)} files — nothing written.")
        return

    written = []
    for p in presets:
        path = write_preset(p, args.output_dir)
        written.append(path)
        print(f"  wrote  {path.name}")

    print(f"\nDone. {len(written)} preset(s) written to: {args.output_dir}")


if __name__ == "__main__":
    main()
