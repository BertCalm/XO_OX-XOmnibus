#!/usr/bin/env python3
"""
xpn_preset_movement_expander.py

Generate high-movement presets for kinetic engines in XOlokun.
Targets the Flux mood with three intensity levels: Pulse, Surge, Storm.
"""

import json
import os
import argparse
import random
from pathlib import Path

# ---------------------------------------------------------------------------
# Engine configuration
# ---------------------------------------------------------------------------

ENGINES = {
    "OPTIC":      {"prefix": "optic_",   "color": "#FF6B35"},
    "OUROBOROS":  {"prefix": "ouro_",    "color": "#9B59B6"},
    "ODYSSEY":    {"prefix": "drift_",   "color": "#3498DB"},
    "ORACLE":     {"prefix": "oracle_",  "color": "#F39C12"},
    "ORIGAMI":    {"prefix": "origami_", "color": "#27AE60"},
    "OBLIQUE":    {"prefix": "oblq_",    "color": "#E74C3C"},
    "OVERWORLD":  {"prefix": "ow_",      "color": "#1ABC9C"},
    "ONSET":      {"prefix": "perc_",    "color": "#E67E22"},
    "OUTWIT":     {"prefix": "owit_",    "color": "#CC6600"},
    "OLE":        {"prefix": "ole_",     "color": "#FF4081"},
}

# ---------------------------------------------------------------------------
# Movement level definitions
# ---------------------------------------------------------------------------

LEVELS = {
    "Pulse": {
        "movement_range": (0.75, 0.85),
        "aggression":     0.4,
        "density_range":  (0.60, 0.75),
        "vocabulary":     ["Oscillate", "Breathe", "Throb", "Pulse",
                           "Rhythm", "Undulate", "Wave", "Cycle"],
        "description":    "rhythmic, breathing, organic oscillation",
    },
    "Surge": {
        "movement_range": (0.85, 0.95),
        "aggression":     0.6,
        "density_range":  (0.65, 0.80),
        "vocabulary":     ["Drive", "Propel", "Rush", "Current",
                           "Flow", "Torrent", "Surge", "Velocity"],
        "description":    "fast, driving, propulsive",
    },
    "Storm": {
        "movement_range": (0.95, 1.00),
        "aggression":     0.8,
        "density_range":  (0.70, 1.00),
        "vocabulary":     ["Chaos", "Storm", "Turbulence", "Frenzy",
                           "Tempest", "Maelstrom", "Vortex", "Deluge"],
        "description":    "maximum kinetic energy, chaotic motion",
    },
}

# ---------------------------------------------------------------------------
# Per-engine parameter templates
# ---------------------------------------------------------------------------

def engine_params(engine_name: str, prefix: str, level_name: str,
                  movement: float, density: float, aggression: float,
                  rng: random.Random) -> dict:
    """Return a dict of engine-specific parameters."""

    params: dict = {}

    if engine_name == "OPTIC":
        params = {
            f"{prefix}scan_rate":      round(movement * rng.uniform(0.85, 1.0), 4),
            f"{prefix}beam_width":     round(rng.uniform(0.2, 0.6), 4),
            f"{prefix}flicker_depth":  round(aggression * rng.uniform(0.7, 1.0), 4),
            f"{prefix}persistence":    round(1.0 - movement * rng.uniform(0.6, 0.9), 4),
            f"{prefix}chromatic_ab":   round(aggression * rng.uniform(0.3, 0.8), 4),
            f"{prefix}mod_rate":       round(movement * rng.uniform(2.0, 8.0), 4),
            f"{prefix}density":        round(density, 4),
        }

    elif engine_name == "OUROBOROS":
        params = {
            f"{prefix}cycle_speed":    round(movement * rng.uniform(0.8, 1.0), 4),
            f"{prefix}loop_tension":   round(aggression * rng.uniform(0.5, 1.0), 4),
            f"{prefix}morph_rate":     round(movement * rng.uniform(0.6, 1.0), 4),
            f"{prefix}tail_bite":      round(rng.uniform(0.3, 0.9), 4),
            f"{prefix}ouroboros_depth":round(density, 4),
            f"{prefix}chaos_factor":   round(aggression * rng.uniform(0.4, 1.0), 4),
            f"{prefix}self_ref_gain":  round(rng.uniform(0.1, 0.6), 4),
        }

    elif engine_name == "ODYSSEY":
        params = {
            f"{prefix}drift_rate":     round(movement * rng.uniform(0.7, 1.0), 4),
            f"{prefix}warp_depth":     round(aggression * rng.uniform(0.4, 1.0), 4),
            f"{prefix}phase_scatter":  round(movement * rng.uniform(0.5, 1.0), 4),
            f"{prefix}velocity":       round(movement, 4),
            f"{prefix}density":        round(density, 4),
            f"{prefix}turbulence":     round(aggression * rng.uniform(0.6, 1.0), 4),
            f"{prefix}current_depth":  round(rng.uniform(0.2, 0.8), 4),
        }

    elif engine_name == "ORACLE":
        params = {
            f"{prefix}prophecy_rate":  round(movement * rng.uniform(0.8, 1.0), 4),
            f"{prefix}vision_blur":    round((1.0 - movement) * rng.uniform(0.1, 0.5), 4),
            f"{prefix}tremor":         round(aggression * rng.uniform(0.5, 1.0), 4),
            f"{prefix}flux_depth":     round(movement * rng.uniform(0.6, 1.0), 4),
            f"{prefix}density":        round(density, 4),
            f"{prefix}chaos_seed":     round(aggression * rng.uniform(0.3, 1.0), 4),
            f"{prefix}cascade":        round(rng.uniform(0.3, 0.9), 4),
        }

    elif engine_name == "ORIGAMI":
        params = {
            f"{prefix}fold_speed":     round(movement * rng.uniform(0.7, 1.0), 4),
            f"{prefix}crease_depth":   round(aggression * rng.uniform(0.4, 1.0), 4),
            f"{prefix}unfurl_rate":    round(movement * rng.uniform(0.5, 1.0), 4),
            f"{prefix}tension":        round(aggression * rng.uniform(0.5, 0.9), 4),
            f"{prefix}layer_density":  round(density, 4),
            f"{prefix}flutter":        round(movement * rng.uniform(0.6, 1.0), 4),
            f"{prefix}collapse_amt":   round(rng.uniform(0.1, 0.7), 4),
        }

    elif engine_name == "OBLIQUE":
        params = {
            f"{prefix}angle_rate":     round(movement * rng.uniform(0.8, 1.0), 4),
            f"{prefix}deviation":      round(aggression * rng.uniform(0.5, 1.0), 4),
            f"{prefix}slant_depth":    round(movement * rng.uniform(0.4, 0.9), 4),
            f"{prefix}shear":          round(aggression * rng.uniform(0.3, 0.8), 4),
            f"{prefix}density":        round(density, 4),
            f"{prefix}skew_mod":       round(movement * rng.uniform(0.5, 1.0), 4),
            f"{prefix}oblique_factor": round(rng.uniform(0.4, 1.0), 4),
        }

    elif engine_name == "OVERWORLD":
        params = {
            f"{prefix}era_crossfade":  round(rng.uniform(0.0, 1.0), 4),
            f"{prefix}glitch_rate":    round(movement * rng.uniform(0.6, 1.0), 4),
            f"{prefix}glitch_depth":   round(aggression * rng.uniform(0.5, 1.0), 4),
            f"{prefix}arp_speed":      round(movement * rng.uniform(0.7, 1.0), 4),
            f"{prefix}density":        round(density, 4),
            f"{prefix}chaos":          round(aggression * rng.uniform(0.4, 1.0), 4),
            f"{prefix}pulse_width":    round(rng.uniform(0.2, 0.8), 4),
        }

    elif engine_name == "ONSET":
        params = {
            f"{prefix}machine":        round(rng.uniform(0.3, 1.0), 4),
            f"{prefix}punch":          round(aggression * rng.uniform(0.6, 1.0), 4),
            f"{prefix}space":          round(rng.uniform(0.1, 0.6), 4),
            f"{prefix}mutate":         round(movement * rng.uniform(0.5, 1.0), 4),
            f"{prefix}density":        round(density, 4),
            f"{prefix}pattern_speed":  round(movement * rng.uniform(0.7, 1.0), 4),
            f"{prefix}velocity_range": round(movement * rng.uniform(0.6, 1.0), 4),
        }

    elif engine_name == "OUTWIT":
        params = {
            f"{prefix}arm_activity":   round(movement * rng.uniform(0.7, 1.0), 4),
            f"{prefix}ca_rule":        rng.randint(0, 255),
            f"{prefix}tentacle_speed": round(movement * rng.uniform(0.6, 1.0), 4),
            f"{prefix}ink_burst":      round(aggression * rng.uniform(0.4, 1.0), 4),
            f"{prefix}density":        round(density, 4),
            f"{prefix}chaos_rule":     round(aggression * rng.uniform(0.5, 1.0), 4),
            f"{prefix}camouflage":     round(rng.uniform(0.0, 0.5), 4),
        }

    elif engine_name == "OLE":
        params = {
            f"{prefix}drama":          round(aggression * rng.uniform(0.6, 1.0), 4),
            f"{prefix}rhythm_drive":   round(movement * rng.uniform(0.7, 1.0), 4),
            f"{prefix}percussion_mix": round(density * rng.uniform(0.5, 1.0), 4),
            f"{prefix}call_response":  round(rng.uniform(0.2, 0.8), 4),
            f"{prefix}density":        round(density, 4),
            f"{prefix}frenzy":         round(movement * aggression * rng.uniform(0.5, 1.0), 4),
            f"{prefix}groove_offset":  round(rng.uniform(0.0, 0.3), 4),
        }

    return params


# ---------------------------------------------------------------------------
# Preset builder
# ---------------------------------------------------------------------------

def build_preset(engine_name: str, level_name: str, index: int,
                 rng: random.Random) -> dict:
    """Construct a single preset JSON object."""

    engine_cfg = ENGINES[engine_name]
    level_cfg  = LEVELS[level_name]

    prefix    = engine_cfg["prefix"]
    mv_lo, mv_hi = level_cfg["movement_range"]
    dn_lo, dn_hi = level_cfg["density_range"]

    movement   = round(rng.uniform(mv_lo, mv_hi), 4)
    density    = round(rng.uniform(dn_lo, dn_hi), 4)
    aggression = round(level_cfg["aggression"] + rng.uniform(-0.05, 0.05), 4)
    aggression = max(0.0, min(1.0, aggression))

    word  = rng.choice(level_cfg["vocabulary"])
    name  = f"{word} {engine_name.title()} {index + 1}"

    dna = {
        "movement":   movement,
        "density":    density,
        "aggression": aggression,
        "warmth":     round(rng.uniform(0.1, 0.5), 4),
        "brightness": round(rng.uniform(0.4, 1.0), 4),
        "complexity": round(rng.uniform(0.4, 0.9), 4),
        "space":      round(rng.uniform(0.2, 0.7), 4),
    }

    engine_specific = engine_params(
        engine_name, prefix, level_name,
        movement, density, aggression, rng
    )

    preset = {
        "name":        name,
        "engine":      engine_name,
        "mood":        "Flux",
        "level":       level_name,
        "description": f"{level_name} — {level_cfg['description']}",
        "dna":         dna,
        "parameters":  engine_specific,
        "tags":        ["flux", "movement", level_name.lower(), engine_name.lower()],
        "color":       engine_cfg["color"],
        "version":     "1.0",
    }

    return preset


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def parse_args():
    parser = argparse.ArgumentParser(
        description="Generate high-movement Flux presets for XOlokun kinetic engines."
    )
    parser.add_argument(
        "--output-dir",
        default="Presets/XOlokun/Flux/",
        help="Directory to write preset JSON files (default: Presets/XOlokun/Flux/)",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print what would be written without writing files",
    )
    parser.add_argument(
        "--engines",
        nargs="+",
        choices=list(ENGINES.keys()),
        default=list(ENGINES.keys()),
        metavar="ENGINE",
        help="Engines to generate presets for (default: all 10)",
    )
    parser.add_argument(
        "--count-per-level",
        type=int,
        default=3,
        metavar="N",
        help="Number of presets per engine per level (default: 3)",
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

    output_dir = Path(args.output_dir)
    if not args.dry_run:
        output_dir.mkdir(parents=True, exist_ok=True)

    total = 0
    summary: list[str] = []

    for engine_name in args.engines:
        for level_name in LEVELS:
            for idx in range(args.count_per_level):
                preset = build_preset(engine_name, level_name, idx, rng)

                # Sanitise filename
                safe_name = preset["name"].replace(" ", "_").replace("/", "-")
                filename  = f"{engine_name}_{level_name}_{safe_name}.json"
                filepath  = output_dir / filename

                if args.dry_run:
                    summary.append(f"[DRY RUN] Would write: {filepath}")
                else:
                    with open(filepath, "w", encoding="utf-8") as fh:
                        json.dump(preset, fh, indent=2)
                    summary.append(f"Written: {filepath}")

                total += 1

    for line in summary:
        print(line)

    engines_used = len(args.engines)
    levels_count = len(LEVELS)
    print(
        f"\nDone. {total} presets "
        f"({engines_used} engines × {levels_count} levels × {args.count_per_level} per level)"
        f"{' [DRY RUN — no files written]' if args.dry_run else f' → {output_dir}'}"
    )


if __name__ == "__main__":
    main()
