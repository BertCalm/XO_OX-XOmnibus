#!/usr/bin/env python3
"""
xpn_hot_dense_preset_pack.py

Generates "Hot Dense" quadrant presets (warmth 0.75-1.0, density 0.75-1.0)
for XOmnibus engines. Targets thick analog bass and saturated pads in the
Foundation mood category.

Usage:
    python3 xpn_hot_dense_preset_pack.py
    python3 xpn_hot_dense_preset_pack.py --dry-run
    python3 xpn_hot_dense_preset_pack.py --count 5 --engines OBLONG,OBESE
    python3 xpn_hot_dense_preset_pack.py --output-dir Presets/XOmnibus/Foundation/ --seed 42
"""

import json
import os
import argparse
import random
from pathlib import Path

# ---------------------------------------------------------------------------
# Engine definitions
# ---------------------------------------------------------------------------

ENGINE_PREFIXES = {
    "OBLONG":    "bob_",
    "OBESE":     "fat_",
    "OVERDUB":   "dub_",
    "ONSET":     "perc_",
    "OVERBITE":  "poss_",
    "OUROBOROS": "ouro_",
    "ORGANON":   "organon_",
    "ORBITAL":   "orb_",
    "OSTERIA":   "osteria_",
    "OHM":       "ohm_",
}

# Per-engine aggression targets (others default to 0.5)
ENGINE_AGGRESSION = {
    "OBESE":    0.7,
    "OVERDUB":  0.4,
    "OBLONG":   0.5,
    "ONSET":    0.65,
    "OVERBITE": 0.55,
    "OUROBOROS":0.6,
    "ORGANON":  0.45,
    "ORBITAL":  0.5,
    "OSTERIA":  0.5,
    "OHM":      0.45,
}

# Per-engine parameter stubs — these are the key hot/dense-flavored params
# with sensible defaults for each engine's character.
ENGINE_PARAM_STUBS = {
    "OBLONG": {
        "bob_drive":        0.72,
        "bob_filter_cutoff":0.38,
        "bob_filter_res":   0.28,
        "bob_env_attack":   0.05,
        "bob_env_decay":    0.55,
        "bob_env_sustain":  0.75,
        "bob_env_release":  0.40,
        "bob_sub_level":    0.80,
        "bob_saturation":   0.68,
        "bob_body":         0.85,
    },
    "OBESE": {
        "fat_drive":        0.80,
        "fat_filter_cutoff":0.30,
        "fat_filter_res":   0.20,
        "fat_env_attack":   0.02,
        "fat_env_decay":    0.60,
        "fat_env_sustain":  0.70,
        "fat_env_release":  0.35,
        "fat_sub":          0.90,
        "fat_mid_boost":    0.55,
        "fat_compress":     0.75,
    },
    "OVERDUB": {
        "dub_drive":        0.50,
        "dub_tape_sat":     0.78,
        "dub_delay_mix":    0.30,
        "dub_reverb_mix":   0.35,
        "dub_filter_cutoff":0.42,
        "dub_filter_res":   0.18,
        "dub_env_attack":   0.10,
        "dub_env_release":  0.55,
        "dub_warmth":       0.88,
        "dub_density":      0.80,
    },
    "ONSET": {
        "perc_drive":       0.65,
        "perc_body":        0.82,
        "perc_punch":       0.75,
        "perc_decay":       0.50,
        "perc_tone":        0.35,
        "perc_sub":         0.85,
        "perc_transient":   0.60,
        "perc_compress":    0.70,
        "perc_saturation":  0.72,
        "perc_space":       0.38,
    },
    "OVERBITE": {
        "poss_filter_cutoff":0.35,
        "poss_filter_res":   0.22,
        "poss_drive":        0.62,
        "poss_env_attack":   0.04,
        "poss_env_decay":    0.58,
        "poss_env_sustain":  0.72,
        "poss_env_release":  0.42,
        "poss_sub":          0.80,
        "poss_bite":         0.55,
        "poss_warmth":       0.85,
    },
    "OUROBOROS": {
        "ouro_feedback":    0.68,
        "ouro_drive":       0.72,
        "ouro_filter_cutoff":0.33,
        "ouro_filter_res":  0.25,
        "ouro_density":     0.85,
        "ouro_warmth":      0.88,
        "ouro_cycle_rate":  0.40,
        "ouro_saturation":  0.78,
        "ouro_body":        0.80,
        "ouro_tail":        0.50,
    },
    "ORGANON": {
        "organon_drive":        0.55,
        "organon_filter_cutoff":0.40,
        "organon_filter_res":   0.20,
        "organon_drawbar_low":  0.88,
        "organon_drawbar_mid":  0.65,
        "organon_drawbar_high": 0.30,
        "organon_rotary_speed": 0.25,
        "organon_overdrive":    0.60,
        "organon_warmth":       0.90,
        "organon_chorus_mix":   0.35,
    },
    "ORBITAL": {
        "orb_drive":        0.58,
        "orb_filter_cutoff":0.38,
        "orb_filter_res":   0.22,
        "orb_env_attack":   0.08,
        "orb_env_decay":    0.52,
        "orb_env_sustain":  0.78,
        "orb_env_release":  0.45,
        "orb_pulse_width":  0.55,
        "orb_sub_osc":      0.82,
        "orb_warmth":       0.85,
    },
    "OSTERIA": {
        "osteria_drive":        0.60,
        "osteria_filter_cutoff":0.42,
        "osteria_filter_res":   0.18,
        "osteria_env_attack":   0.06,
        "osteria_env_decay":    0.54,
        "osteria_env_sustain":  0.74,
        "osteria_env_release":  0.48,
        "osteria_warmth":       0.88,
        "osteria_body":         0.82,
        "osteria_saturation":   0.65,
    },
    "OHM": {
        "ohm_drive":        0.52,
        "ohm_filter_cutoff":0.45,
        "ohm_filter_res":   0.18,
        "ohm_env_attack":   0.12,
        "ohm_env_decay":    0.50,
        "ohm_env_sustain":  0.76,
        "ohm_env_release":  0.55,
        "ohm_warmth":       0.90,
        "ohm_resonance":    0.30,
        "ohm_commune":      0.70,
    },
}

# ---------------------------------------------------------------------------
# Name vocabulary
# ---------------------------------------------------------------------------

NAME_WORDS = [
    "Amber", "Hearth", "Embers", "Molasses", "Humid", "Saturate",
    "Boil", "Smolder", "Cauldron", "Thick", "Amber Resin", "Warm Body",
    "Dense Fog", "Heavy Clay", "Tar Pit", "Velvet Crush", "Wool Stack",
    "Humid Grove", "Pressure Cooker",
]

NAME_SUFFIXES = [
    "I", "II", "III", "Deep", "Low", "Dark", "Core", "Bass",
    "Pad", "Drift", "Pull", "Weight", "Push",
]

# ---------------------------------------------------------------------------
# Preset generation helpers
# ---------------------------------------------------------------------------

def clamp(value, lo=0.0, hi=1.0):
    return max(lo, min(hi, value))


def jitter(base, spread=0.08, rng=None):
    """Apply small random perturbation to a base value."""
    r = rng if rng else random
    return clamp(base + r.uniform(-spread, spread))


def make_preset_name(engine, index, used_names, rng):
    """Generate a unique human-readable preset name."""
    word = rng.choice(NAME_WORDS)
    suffix = rng.choice(NAME_SUFFIXES)
    candidate = f"{word} {suffix}"
    # Avoid exact duplicates within the run
    attempt = 0
    while candidate in used_names and attempt < 20:
        word = rng.choice(NAME_WORDS)
        suffix = rng.choice(NAME_SUFFIXES)
        candidate = f"{word} {suffix}"
        attempt += 1
    used_names.add(candidate)
    return candidate


def build_sonic_dna(engine, rng):
    """Return Sonic DNA dict for hot/dense quadrant."""
    aggression_base = ENGINE_AGGRESSION.get(engine, 0.5)
    return {
        "warmth":     round(jitter(rng.uniform(0.80, 1.0), 0.04, rng), 3),
        "density":    round(jitter(rng.uniform(0.80, 1.0), 0.04, rng), 3),
        "aggression": round(jitter(aggression_base, 0.08, rng), 3),
        "movement":   round(jitter(rng.uniform(0.40, 0.60), 0.06, rng), 3),
        "space":      round(jitter(rng.uniform(0.35, 0.55), 0.06, rng), 3),
        "brightness": round(jitter(rng.uniform(0.30, 0.60), 0.08, rng), 3),
    }


def build_engine_params(engine, rng):
    """Return parameter dict with jitter applied to each stub value."""
    stubs = ENGINE_PARAM_STUBS.get(engine, {})
    params = {}
    for key, base_val in stubs.items():
        params[key] = round(jitter(base_val, 0.06, rng), 4)
    return params


def build_preset(engine, index, used_names, rng):
    """Construct a full preset dict for one engine."""
    prefix = ENGINE_PREFIXES[engine]
    name = make_preset_name(engine, index, used_names, rng)
    dna = build_sonic_dna(engine, rng)
    params = build_engine_params(engine, rng)

    preset = {
        "name": name,
        "engine": engine,
        "prefix": prefix,
        "mood": "Foundation",
        "category": "Hot Dense",
        "tags": ["warm", "dense", "analog", "bass", "saturated"],
        "sonic_dna": dna,
        "parameters": params,
        "meta": {
            "generator": "xpn_hot_dense_preset_pack.py",
            "version": "1.0.0",
            "quadrant": "hot_dense",
            "warmth_range": "0.80-1.0",
            "density_range": "0.80-1.0",
        },
    }
    return preset


# ---------------------------------------------------------------------------
# Main generation logic
# ---------------------------------------------------------------------------

def generate_presets(engines, count_per_engine, rng):
    """Generate all presets across the specified engines."""
    all_presets = []
    used_names = set()

    for engine in engines:
        for i in range(count_per_engine):
            preset = build_preset(engine, i, used_names, rng)
            all_presets.append(preset)

    return all_presets


def write_preset(preset, output_dir, dry_run):
    """Write a single preset to disk as JSON."""
    engine_dir = output_dir / preset["engine"]
    if not dry_run:
        engine_dir.mkdir(parents=True, exist_ok=True)

    safe_name = preset["name"].replace(" ", "_").replace("/", "-")
    filename = f"{preset['engine']}_{safe_name}.json"
    filepath = engine_dir / filename

    if dry_run:
        print(f"  [DRY RUN] Would write: {filepath}")
    else:
        with open(filepath, "w", encoding="utf-8") as f:
            json.dump(preset, f, indent=2)
        print(f"  Wrote: {filepath}")

    return filepath


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def parse_args():
    parser = argparse.ArgumentParser(
        description="Generate Hot Dense quadrant presets for XOmnibus engines.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument(
        "--output-dir",
        default="Presets/XOmnibus/Foundation/",
        help="Output directory for generated presets (default: Presets/XOmnibus/Foundation/)",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print what would be written without creating files.",
    )
    parser.add_argument(
        "--count",
        type=int,
        default=3,
        metavar="N",
        help="Number of presets per engine (default: 3)",
    )
    parser.add_argument(
        "--engines",
        type=str,
        default=",".join(ENGINE_PREFIXES.keys()),
        help="Comma-separated list of engines to target (default: all 10)",
    )
    parser.add_argument(
        "--seed",
        type=int,
        default=None,
        help="Random seed for reproducible output.",
    )
    return parser.parse_args()


def main():
    args = parse_args()

    # Validate engines
    requested = [e.strip().upper() for e in args.engines.split(",") if e.strip()]
    unknown = [e for e in requested if e not in ENGINE_PREFIXES]
    if unknown:
        print(f"ERROR: Unknown engine(s): {', '.join(unknown)}")
        print(f"Valid engines: {', '.join(ENGINE_PREFIXES.keys())}")
        raise SystemExit(1)

    if args.count < 1:
        print("ERROR: --count must be >= 1")
        raise SystemExit(1)

    # Set up RNG
    rng = random.Random(args.seed)

    # Resolve output directory relative to repo root (or absolute if given)
    output_dir = Path(args.output_dir)
    if not output_dir.is_absolute():
        # Resolve relative to this script's grandparent (repo root)
        repo_root = Path(__file__).resolve().parent.parent
        output_dir = repo_root / output_dir

    total = len(requested) * args.count
    print(f"XPN Hot Dense Preset Pack")
    print(f"  Engines    : {', '.join(requested)}")
    print(f"  Per engine : {args.count}")
    print(f"  Total      : {total}")
    print(f"  Output dir : {output_dir}")
    print(f"  Dry run    : {args.dry_run}")
    print(f"  Seed       : {args.seed if args.seed is not None else 'random'}")
    print()

    presets = generate_presets(requested, args.count, rng)

    written = []
    for preset in presets:
        fp = write_preset(preset, output_dir, args.dry_run)
        written.append(fp)

    print()
    if args.dry_run:
        print(f"Dry run complete. {len(written)} presets would be written.")
    else:
        print(f"Done. {len(written)} presets written to {output_dir}")

    # Summary table
    print()
    print(f"{'Engine':<12} {'Count':>5}  {'Sample name'}")
    print("-" * 50)
    engine_counts = {}
    engine_samples = {}
    for p in presets:
        e = p["engine"]
        engine_counts[e] = engine_counts.get(e, 0) + 1
        if e not in engine_samples:
            engine_samples[e] = p["name"]
    for e in requested:
        print(f"  {e:<10} {engine_counts.get(e, 0):>5}  {engine_samples.get(e, '')}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
