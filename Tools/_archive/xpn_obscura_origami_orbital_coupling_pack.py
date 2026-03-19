#!/usr/bin/env python3
"""
xpn_obscura_origami_orbital_coupling_pack.py — XO_OX Designs
Seed coupling coverage for OBSCURA, ORIGAMI, and ORBITAL.

Generates ~60 stub .xometa presets in Entangled mood:
  - 6  OBSCURA × ORIGAMI × ORBITAL 3-way marquee presets
  - 18 OBSCURA new-pair presets  (1 per partner engine)
  - 18 ORIGAMI new-pair presets  (1 per partner engine)
  - 18 ORBITAL new-pair presets  (1 per partner engine)

Total: 60 presets (before skip-existing deduplication).

Usage:
    python Tools/xpn_obscura_origami_orbital_coupling_pack.py
    python Tools/xpn_obscura_origami_orbital_coupling_pack.py --dry-run
    python Tools/xpn_obscura_origami_orbital_coupling_pack.py --seed 42
    python Tools/xpn_obscura_origami_orbital_coupling_pack.py --output-dir /tmp/test_out
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
    # Primary trio
    "OBSCURA":    dict(brightness=0.45, warmth=0.50, movement=0.40, density=0.60, space=0.55, aggression=0.25),
    "ORIGAMI":    dict(brightness=0.70, warmth=0.50, movement=0.65, density=0.55, space=0.50, aggression=0.45),
    "ORBITAL":    dict(brightness=0.60, warmth=0.60, movement=0.55, density=0.65, space=0.50, aggression=0.50),
    # Partner engines
    "ONSET":      dict(brightness=0.55, warmth=0.50, movement=0.80, density=0.75, space=0.50, aggression=0.70),
    "OBLONG":     dict(brightness=0.55, warmth=0.65, movement=0.50, density=0.60, space=0.55, aggression=0.40),
    "OVERWORLD":  dict(brightness=0.60, warmth=0.55, movement=0.65, density=0.55, space=0.60, aggression=0.35),
    "OVERDUB":    dict(brightness=0.45, warmth=0.70, movement=0.55, density=0.55, space=0.75, aggression=0.25),
    "OPAL":       dict(brightness=0.70, warmth=0.50, movement=0.75, density=0.45, space=0.80, aggression=0.20),
    "ORGANON":    dict(brightness=0.55, warmth=0.60, movement=0.70, density=0.65, space=0.55, aggression=0.45),
    "OUROBOROS":  dict(brightness=0.50, warmth=0.40, movement=0.85, density=0.75, space=0.50, aggression=0.80),
    "OBSIDIAN":   dict(brightness=0.35, warmth=0.45, movement=0.30, density=0.70, space=0.60, aggression=0.30),
    "ORACLE":     dict(brightness=0.50, warmth=0.40, movement=0.60, density=0.70, space=0.70, aggression=0.40),
    "OPTIC":      dict(brightness=0.80, warmth=0.40, movement=0.75, density=0.40, space=0.65, aggression=0.35),
    "OBLIQUE":    dict(brightness=0.70, warmth=0.55, movement=0.70, density=0.55, space=0.55, aggression=0.55),
    "OCELOT":     dict(brightness=0.65, warmth=0.60, movement=0.60, density=0.55, space=0.60, aggression=0.50),
    "OSPREY":     dict(brightness=0.55, warmth=0.55, movement=0.60, density=0.60, space=0.65, aggression=0.40),
    "OSTERIA":    dict(brightness=0.50, warmth=0.70, movement=0.50, density=0.65, space=0.55, aggression=0.45),
    "OHM":        dict(brightness=0.45, warmth=0.75, movement=0.55, density=0.60, space=0.70, aggression=0.30),
    "ORPHICA":    dict(brightness=0.80, warmth=0.50, movement=0.70, density=0.45, space=0.75, aggression=0.25),
    "OBBLIGATO":  dict(brightness=0.55, warmth=0.65, movement=0.60, density=0.65, space=0.60, aggression=0.45),
    "OLE":        dict(brightness=0.65, warmth=0.70, movement=0.75, density=0.60, space=0.60, aggression=0.55),
}

# ---------------------------------------------------------------------------
# Engine → parameter prefix mapping
# ---------------------------------------------------------------------------
ENGINE_PREFIX = {
    "OBSCURA":    "obscura_",
    "ORIGAMI":    "origami_",
    "ORBITAL":    "orb_",
    "ONSET":      "perc_",
    "OBLONG":     "bob_",
    "OVERWORLD":  "ow_",
    "OVERDUB":    "dub_",
    "OPAL":       "opal_",
    "ORGANON":    "organon_",
    "OUROBOROS":  "ouro_",
    "OBSIDIAN":   "obsidian_",
    "ORACLE":     "oracle_",
    "OPTIC":      "optic_",
    "OBLIQUE":    "oblq_",
    "OCELOT":     "ocelot_",
    "OSPREY":     "osprey_",
    "OSTERIA":    "osteria_",
    "OHM":        "ohm_",
    "ORPHICA":    "orph_",
    "OBBLIGATO":  "obbl_",
    "OLE":        "ole_",
}

# Engine ID strings used in the preset "engines" array
ENGINE_ID = {
    "OBSCURA":    "Obscura",
    "ORIGAMI":    "Origami",
    "ORBITAL":    "Orbital",
    "ONSET":      "Onset",
    "OBLONG":     "Oblong",
    "OVERWORLD":  "Overworld",
    "OVERDUB":    "Overdub",
    "OPAL":       "Opal",
    "ORGANON":    "Organon",
    "OUROBOROS":  "Ouroboros",
    "OBSIDIAN":   "Obsidian",
    "ORACLE":     "Oracle",
    "OPTIC":      "Optic",
    "OBLIQUE":    "Oblique",
    "OCELOT":     "Ocelot",
    "OSPREY":     "Osprey",
    "OSTERIA":    "Osteria",
    "OHM":        "Ohm",
    "ORPHICA":    "Orphica",
    "OBBLIGATO":  "Obbligato",
    "OLE":        "Ole",
}

# ---------------------------------------------------------------------------
# Name vocabularies  (per engine + per partner)
# ---------------------------------------------------------------------------
VOCAB_OBSCURA = [
    "Albumen", "Daguerreotype", "Exposure", "Grain", "Latent",
    "Plate", "Silver", "Stiffness", "Tonal", "Varnish",
    "Wet Plate", "Collodion", "Ferrotype", "Ambrotype", "Calotype",
]
VOCAB_ORIGAMI = [
    "Crease", "Fold", "Pleat", "Score", "Valley",
    "Mountain", "Tessellate", "Unfurl", "Geometry", "Washi",
    "Facet", "Vertex", "Net", "Originator", "Squash",
]
VOCAB_ORBITAL = [
    "Group", "Envelope", "Phase", "Arc", "Cycle",
    "Resonant", "Harmonic", "Apogee", "Perihelion", "Declination",
    "Nodal", "Ephemeris", "Parallax", "Transit", "Opposition",
]

PARTNER_VOCAB = {
    "ONSET":     ["Strike", "Hit", "Snap", "Burst", "Trigger", "Punch", "Crack", "Rim"],
    "OBLONG":    ["Amber", "Waver", "Tilt", "Hollow", "Bevel", "Slant", "Warm", "Analog"],
    "OVERWORLD": ["Era", "Chip", "Pixelate", "Arcade", "Cartridge", "Retro", "8Bit", "Scanline"],
    "OVERDUB":   ["Echo", "Dub", "Tape", "Riddim", "Send", "Wash", "Delay", "Spring"],
    "OPAL":      ["Grain", "Scatter", "Shimmer", "Cloud", "Particle", "Float", "Iridescent", "Spray"],
    "ORGANON":   ["Metabolic", "Cell", "Membrane", "Enzyme", "Catalyst", "Living", "Organic", "Tissue"],
    "OUROBOROS": ["Serpent", "Loop", "Feed", "Chaos", "Gnaw", "Spiral", "Recursive", "Devour"],
    "OBSIDIAN":  ["Obsidian", "Volcanic", "Glass", "Depth", "Crystal", "Dark", "Vitreous", "Lava"],
    "ORACLE":    ["Prophecy", "Stochastic", "Signal", "Omen", "Void", "Foresight", "Maqam", "Augury"],
    "OPTIC":     ["Pulse", "Spectrum", "Phosphor", "Wavelength", "Retina", "Luminance", "Scan", "Flicker"],
    "OBLIQUE":   ["Prism", "Bounce", "Refract", "Slant", "Funk", "Glitch", "Vector", "Skew"],
    "OCELOT":    ["Tawny", "Prowl", "Habitat", "Savanna", "Territorial", "Dappled", "Biome", "Range"],
    "OSPREY":    ["Shore", "Dive", "Coastline", "Tide", "Shoreline", "Current", "Plunge", "Salt"],
    "OSTERIA":   ["Porto", "Wine", "Cellar", "Vintage", "Barrel", "Cork", "Tannin", "Ferment"],
    "OHM":       ["Commune", "Hum", "Mellow", "Drift", "Still", "Sage", "Mantric", "Pastoral"],
    "ORPHICA":   ["Pluck", "Siren", "Harp", "Microsound", "Filament", "Gossamer", "Lyre", "Vibrato"],
    "OBBLIGATO": ["Bond", "Breath", "Wind", "Weave", "Voice", "Oblige", "Duet", "Interlock"],
    "OLE":       ["Drama", "Flare", "Sway", "Pulse", "Spin", "Surge", "Fiesta", "Flamencoid"],
}

COUPLING_TYPES = [
    "timbral_blend", "rhythm_sync", "filter_modulation", "amplitude_sidechain",
    "pitch_follow", "envelope_share", "harmonic_lock", "spectral_cross",
]

COUPLING_INTENSITIES = ["Subtle", "Moderate", "Strong", "Extreme"]

DATE = "2026-03-16"

# ---------------------------------------------------------------------------
# DNA helpers
# ---------------------------------------------------------------------------
def blend_dna(*engine_names, rng: random.Random) -> dict:
    """Average DNA of N engines, then apply a small random nudge."""
    baselines = [DNA_BASELINES[e] for e in engine_names]
    keys = list(baselines[0].keys())
    blended = {}
    for k in keys:
        mid = sum(b[k] for b in baselines) / len(baselines)
        nudge = rng.uniform(-0.05, 0.05)
        blended[k] = round(max(0.0, min(1.0, mid + nudge)), 3)
    return blended


# ---------------------------------------------------------------------------
# Parameter stub builders
# ---------------------------------------------------------------------------
def make_obscura_params(rng: random.Random) -> dict:
    p = "obscura_"
    return {
        f"{p}stiffness":    round(rng.uniform(0.2, 0.8), 3),
        f"{p}graininess":   round(rng.uniform(0.2, 0.7), 3),
        f"{p}exposure":     round(rng.uniform(0.3, 0.9), 3),
        f"{p}latentDepth":  round(rng.uniform(0.2, 0.8), 3),
        f"{p}silverTone":   round(rng.uniform(0.3, 0.7), 3),
        f"{p}macro1":       round(rng.uniform(0.3, 0.7), 3),
        f"{p}macro2":       round(rng.uniform(0.3, 0.7), 3),
        f"{p}macro3":       round(rng.uniform(0.3, 0.7), 3),
        f"{p}macro4":       round(rng.uniform(0.3, 0.7), 3),
        f"{p}couplingOut":  round(rng.uniform(0.4, 0.8), 3),
    }


def make_origami_params(rng: random.Random) -> dict:
    p = "origami_"
    return {
        f"{p}foldPoint":    round(rng.uniform(0.2, 0.8), 3),
        f"{p}crease":       round(rng.uniform(0.3, 0.9), 3),
        f"{p}valleyDepth":  round(rng.uniform(0.2, 0.8), 3),
        f"{p}pleatRate":    round(rng.uniform(0.2, 0.8), 3),
        f"{p}tessellate":   round(rng.uniform(0.3, 0.7), 3),
        f"{p}macro1":       round(rng.uniform(0.3, 0.7), 3),
        f"{p}macro2":       round(rng.uniform(0.3, 0.7), 3),
        f"{p}macro3":       round(rng.uniform(0.3, 0.7), 3),
        f"{p}macro4":       round(rng.uniform(0.3, 0.7), 3),
        f"{p}couplingOut":  round(rng.uniform(0.4, 0.8), 3),
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


def make_primary_params(engine: str, rng: random.Random) -> dict:
    if engine == "OBSCURA":
        return make_obscura_params(rng)
    if engine == "ORIGAMI":
        return make_origami_params(rng)
    if engine == "ORBITAL":
        return make_orbital_params(rng)
    return make_partner_params(engine, rng)


# ---------------------------------------------------------------------------
# Preset builder
# ---------------------------------------------------------------------------
def make_preset(name: str, primary: str, partners: list,
                rng: random.Random) -> dict:
    """Build a single .xometa stub. partners may contain 1 or 2 engines."""
    all_engines = [primary] + partners
    dna = blend_dna(*all_engines, rng=rng)
    ctype = rng.choice(COUPLING_TYPES)
    intensity = rng.choice(COUPLING_INTENSITIES)
    tempo = rng.choice([90, 100, 105, 110, 115, 120, 125, 130])

    # Parameters block
    params = {}
    params[ENGINE_ID[primary]] = make_primary_params(primary, rng)
    for p in partners:
        params[ENGINE_ID[p]] = make_partner_params(p, rng)

    # Coupling pairs
    coupling_pairs = []
    for i, partner in enumerate(partners):
        coupling_pairs.append({
            "engineA": ENGINE_ID[primary],
            "engineB": ENGINE_ID[partner],
            "type": rng.choice(COUPLING_TYPES),
            "amount": round(rng.uniform(0.4, 0.85), 3),
        })
    # For 3-way presets, add a partner↔partner link as well
    if len(partners) == 2:
        coupling_pairs.append({
            "engineA": ENGINE_ID[partners[0]],
            "engineB": ENGINE_ID[partners[1]],
            "type": rng.choice(COUPLING_TYPES),
            "amount": round(rng.uniform(0.35, 0.75), 3),
        })

    engines = [ENGINE_ID[e] for e in all_engines]

    short_partners = " + ".join(p.lower().capitalize() for p in partners)
    primary_cap = primary.lower().capitalize()
    ctype_str = ctype.replace("_", " ")
    desc = (
        f"{primary_cap} × {short_partners} coupling ({ctype_str}). "
        f"Stub preset — sound-design pass pending."
    )
    tags = [e.lower() for e in all_engines] + [ctype.replace("_", "-"), "coupling", "entangled"]

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
def unique_name(vocab_a: list, vocab_b: list, used: set,
                rng: random.Random) -> str:
    attempts = 0
    while attempts < 300:
        w1 = rng.choice(vocab_a)
        w2 = rng.choice(vocab_b)
        candidate = f"{w1} {w2}"
        if candidate not in used:
            used.add(candidate)
            return candidate
        attempts += 1
    # Fallback with numeric suffix
    suffix = rng.randint(100, 999)
    name = f"{rng.choice(vocab_a)} {rng.choice(vocab_b)} {suffix}"
    used.add(name)
    return name


# ---------------------------------------------------------------------------
# Partner list (same 18 partners for all three primaries, per brief)
# ---------------------------------------------------------------------------
PARTNER_ENGINES = [
    "ONSET", "OBLONG", "OVERWORLD", "OVERDUB", "OPAL", "ORGANON",
    "OUROBOROS", "OBSIDIAN", "ORACLE", "OPTIC", "OBLIQUE", "OCELOT",
    "OSPREY", "OSTERIA", "OHM", "ORPHICA", "OBBLIGATO", "OLE",
]

# Primary vocabulary map
PRIMARY_VOCAB = {
    "OBSCURA": VOCAB_OBSCURA,
    "ORIGAMI": VOCAB_ORIGAMI,
    "ORBITAL": VOCAB_ORBITAL,
}

# ---------------------------------------------------------------------------
# Marquee preset definitions — 6 hand-named 3-way presets
# ---------------------------------------------------------------------------
MARQUEE_PRESETS = [
    ("Silver Valley Arc",      "OBSCURA", ["ORIGAMI", "ORBITAL"]),
    ("Latent Fold Phase",      "ORIGAMI", ["OBSCURA", "ORBITAL"]),
    ("Exposure Group Crease",  "OBSCURA", ["ORBITAL", "ORIGAMI"]),
    ("Apogee Tessellate Grain","ORBITAL", ["ORIGAMI", "OBSCURA"]),
    ("Daguerreotype Pleat Cycle","OBSCURA",["ORIGAMI", "ORBITAL"]),
    ("Resonant Unfurl Tonal",  "ORBITAL", ["ORIGAMI", "OBSCURA"]),
]

# ---------------------------------------------------------------------------
# Main generation logic
# ---------------------------------------------------------------------------
def generate_all(rng: random.Random) -> list:
    """Return list of preset dicts (name embedded)."""
    presets = []
    used_names: set = set()

    # ---- 3-way marquee (6 presets) ----
    for mname, primary, partners in MARQUEE_PRESETS:
        used_names.add(mname)
        preset = make_preset(mname, primary, partners, rng)
        presets.append(preset)

    # ---- Per-primary single-partner presets (18 × 3 = 54 presets) ----
    for primary in ("OBSCURA", "ORIGAMI", "ORBITAL"):
        pvocab = PRIMARY_VOCAB[primary]
        for partner in PARTNER_ENGINES:
            svocab = PARTNER_VOCAB[partner]
            name = unique_name(pvocab, svocab, used_names, rng)
            preset = make_preset(name, primary, [partner], rng)
            presets.append(preset)

    return presets


# ---------------------------------------------------------------------------
# I/O helpers
# ---------------------------------------------------------------------------
def safe_filename(name: str) -> str:
    return name.replace(" ", "_").replace("/", "-") + ".xometa"


def write_preset(preset: dict, output_dir: Path) -> tuple:
    """Write preset file; return (path, was_skipped)."""
    output_dir.mkdir(parents=True, exist_ok=True)
    path = output_dir / safe_filename(preset["name"])
    if path.exists():
        return path, True
    with open(path, "w", encoding="utf-8") as f:
        json.dump(preset, f, indent=2)
        f.write("\n")
    return path, False


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------
def parse_args():
    parser = argparse.ArgumentParser(
        description="Generate OBSCURA + ORIGAMI + ORBITAL coupling preset stubs."
    )
    repo_root = Path(__file__).resolve().parent.parent
    default_out = repo_root / "Presets" / "XOmnibus" / "Entangled"
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
    return parser.parse_args()


def main():
    args = parse_args()
    rng = random.Random(args.seed)

    presets = generate_all(rng)

    marquee_count = len(MARQUEE_PRESETS)
    pair_count = len(PARTNER_ENGINES) * 3   # OBSCURA + ORIGAMI + ORBITAL
    total = marquee_count + pair_count

    print("OBSCURA × ORIGAMI × ORBITAL Coupling Pack")
    print(f"  Marquee 3-way presets  : {marquee_count}")
    print(f"  OBSCURA single-pair    : {len(PARTNER_ENGINES)} partners × 1 = {len(PARTNER_ENGINES)}")
    print(f"  ORIGAMI single-pair    : {len(PARTNER_ENGINES)} partners × 1 = {len(PARTNER_ENGINES)}")
    print(f"  ORBITAL single-pair    : {len(PARTNER_ENGINES)} partners × 1 = {len(PARTNER_ENGINES)}")
    print(f"  Total                  : {total}")
    print()

    if args.dry_run:
        print("[dry-run] Would write:")
        for p in presets:
            print(f"  {safe_filename(p['name'])}")
        print(f"\n[dry-run] {len(presets)} files — nothing written.")
        return

    written = []
    skipped = []
    for p in presets:
        path, was_skipped = write_preset(p, args.output_dir)
        if was_skipped:
            skipped.append(path)
        else:
            written.append(path)
            print(f"  wrote   {path.name}")

    if skipped:
        print(f"\n  skipped {len(skipped)} existing file(s).")

    print(f"\nDone. {len(written)} preset(s) written to: {args.output_dir}")


if __name__ == "__main__":
    main()
