#!/usr/bin/env python3
"""
xpn_coupling_recipe_expander.py — XO_OX Coupling Recipe Expander

Takes an existing .xometa coupling preset (or a recipe JSON spec) and generates
a family of related presets with different Sonic DNA emphasis profiles.

Each variation shifts parameter values proportionally toward a DNA target:
  1. feliX-dominant     — brightness up, warmth down
  2. Oscar-dominant     — warmth up, brightness down
  3. Movement-heavy     — movement up, density down
  4. Static             — movement down, density up
  5. Spatial            — space up, aggression down
  6. Aggressive         — aggression up, space down

Usage:
    python xpn_coupling_recipe_expander.py \\
        --source "Beat_Drives_Fat.xometa" \\
        [--variations 6] \\
        [--output-dir ./expanded/] \\
        [--dry-run]

    # Recipe JSON spec (alternative to .xometa source):
    python xpn_coupling_recipe_expander.py \\
        --source recipe_spec.json \\
        --output-dir ./expanded/
"""

import argparse
import copy
import json
import random
import sys
from pathlib import Path

# =============================================================================
# Variation profiles — (dna_boosts, dna_reductions, param_hint_words)
# Each entry: list of (dna_dimension, shift_amount) pairs
# =============================================================================

VARIATION_PROFILES = [
    {
        "id": "felix_dominant",
        "label": "feliX Dominant",
        "description": "Bright, crystalline, forward-facing — feliX polarity",
        "boosts":     [("brightness", +0.20), ("space", +0.10)],
        "reductions": [("warmth", -0.20), ("density", -0.10)],
        # param heuristics: params with these substrings lean bright/spatial
        "bright_hints": ["cutoff", "bright", "treble", "high", "shimmer", "crystal", "air"],
        "dark_hints":   ["warm", "bass", "body", "low", "sub", "mud", "body"],
    },
    {
        "id": "oscar_dominant",
        "label": "Oscar Dominant",
        "description": "Warm, grounded, resonant depth — Oscar polarity",
        "boosts":     [("warmth", +0.20), ("density", +0.10)],
        "reductions": [("brightness", -0.20), ("space", -0.10)],
        "bright_hints": ["cutoff", "bright", "treble", "high", "shimmer", "crystal", "air"],
        "dark_hints":   ["warm", "bass", "body", "low", "sub", "mud", "body"],
    },
    {
        "id": "movement_heavy",
        "label": "Movement Heavy",
        "description": "Alive with motion — LFOs and envelopes surging",
        "boosts":     [("movement", +0.25), ("aggression", +0.05)],
        "reductions": [("density", -0.15), ("space", -0.05)],
        "bright_hints": ["lfo", "rate", "speed", "pulse", "mod", "vibrato", "flutter"],
        "dark_hints":   ["sustain", "hold", "static", "stable", "steady"],
    },
    {
        "id": "static",
        "label": "Static",
        "description": "Dense, held, minimal motion — texture over movement",
        "boosts":     [("density", +0.20), ("warmth", +0.10)],
        "reductions": [("movement", -0.25), ("brightness", -0.05)],
        "bright_hints": ["sustain", "hold", "layer", "density", "stack"],
        "dark_hints":   ["lfo", "rate", "speed", "pulse", "mod", "vibrato"],
    },
    {
        "id": "spatial",
        "label": "Spatial",
        "description": "Wide, open, room-filling — cavernous presence",
        "boosts":     [("space", +0.25), ("brightness", +0.05)],
        "reductions": [("aggression", -0.20), ("density", -0.10)],
        "bright_hints": ["reverb", "delay", "room", "hall", "size", "tail", "spread", "width"],
        "dark_hints":   ["drive", "bite", "crush", "distort", "compress", "gate"],
    },
    {
        "id": "aggressive",
        "label": "Aggressive",
        "description": "Raw, biting, forward-thrust — maximum attack presence",
        "boosts":     [("aggression", +0.25), ("brightness", +0.10)],
        "reductions": [("space", -0.20), ("warmth", -0.10)],
        "bright_hints": ["drive", "bite", "crush", "distort", "gain", "sat", "compress"],
        "dark_hints":   ["reverb", "delay", "room", "size", "tail", "spread"],
    },
]

# =============================================================================
# Name generator word banks (inline subset from xpn_preset_name_generator.py)
# =============================================================================

_NAME_BANKS = {
    "felix_dominant":   (["Crystal", "Glass", "Prism", "Arctic", "Chrome", "Neon"], ["Shard", "Veil", "Light", "Air", "Edge"]),
    "oscar_dominant":   (["Amber", "Ember", "Cedar", "Honey", "Hearth", "Rust"],   ["Glow", "Root", "Shore", "Tide", "Fold"]),
    "movement_heavy":   (["Surge", "Cascade", "Flutter", "Spiral", "Storm"],        ["Drift", "Pulse", "Bloom", "Wave", "Rush"]),
    "static":           (["Still", "Held", "Frozen", "Steady", "Poised"],           ["Mass", "Thicket", "Layer", "Veil", "Field"]),
    "spatial":          (["Cathedral", "Vast", "Ocean", "Horizon", "Expanse"],      ["Hall", "Wind", "Rain", "Haze", "Fog"]),
    "aggressive":       (["Bite", "Grind", "Strike", "Crush", "Raw"],               ["Edge", "Flint", "Salt", "Iron", "Claw"]),
}

_CONNECTORS = ["of the", "in", "at", "and", "meets", "through"]


def _generate_name(profile_id: str, base_name: str, index: int, rng: random.Random) -> str:
    """Generate an evocative name for a variation."""
    adjectives, nouns = _NAME_BANKS.get(profile_id, (["New"], ["Sound"]))
    adj = rng.choice(adjectives)
    noun = rng.choice(nouns)
    # Avoid trivially repeating the base name
    candidate = f"{adj} {noun}"
    if candidate.lower() == base_name.lower():
        noun = rng.choice([n for n in nouns if n != noun] or nouns)
        candidate = f"{adj} {noun}"
    return candidate


# =============================================================================
# Parameter scaling helpers
# =============================================================================

def _clamp(value: float, lo: float = 0.0, hi: float = 1.0) -> float:
    return max(lo, min(hi, value))


def _is_normalized(v) -> bool:
    """True if the value is a float in [0, 1] range (DNA-scale parameter)."""
    return isinstance(v, float) and 0.0 <= v <= 1.0


def _param_hints_match(param_key: str, hint_list: list) -> bool:
    key_lower = param_key.lower()
    return any(h in key_lower for h in hint_list)


def _scale_parameters(params: dict, profile: dict, strength: float = 0.15) -> dict:
    """
    Scale normalized parameters in the preset toward the profile's targets.

    Strategy:
    - bright_hints params: shift toward high end for boosts, low end for reductions
    - dark_hints params: inverse
    - All other normalized floats: small nudge proportional to the net DNA shift direction
    - Integer / non-normalized values: untouched
    """
    scaled = {}
    bright_hints = profile.get("bright_hints", [])
    dark_hints = profile.get("dark_hints", [])

    # Net shift direction: +1 = push toward 1.0, -1 = push toward 0.0, 0 = neutral
    net_boost = sum(amt for _, amt in profile.get("boosts", []))
    net_reduce = sum(amt for _, amt in profile.get("reductions", []))
    net_direction = net_boost + net_reduce  # e.g. +0.30 - 0.20 = +0.10

    for key, val in params.items():
        if not _is_normalized(val):
            scaled[key] = val
            continue

        if _param_hints_match(key, bright_hints):
            # Push toward 1.0 if net positive, toward 0.0 if net negative
            delta = strength if net_direction >= 0 else -strength
        elif _param_hints_match(key, dark_hints):
            delta = -strength if net_direction >= 0 else strength
        else:
            # Generic nudge — small, proportional to net direction
            delta = net_direction * strength * 0.4

        scaled[key] = round(_clamp(val + delta), 4)

    return scaled


def _shift_dna(dna: dict, profile: dict) -> dict:
    """Apply DNA boosts and reductions, clamping to [0, 1]."""
    result = dict(dna)
    for dim, amt in profile.get("boosts", []):
        if dim in result:
            result[dim] = round(_clamp(result[dim] + amt), 3)
    for dim, amt in profile.get("reductions", []):
        if dim in result:
            result[dim] = round(_clamp(result[dim] + amt), 3)  # amt is already negative
    return result


# =============================================================================
# Preset I/O
# =============================================================================

def load_source(source_path: Path) -> dict:
    """Load a .xometa or recipe JSON file into a dict."""
    raw = json.loads(source_path.read_text(encoding="utf-8"))

    # If it's a recipe JSON (no schema_version), wrap it into a minimal preset skeleton
    if "schema_version" not in raw:
        preset = _recipe_to_preset(raw)
    else:
        preset = raw

    return preset


def _recipe_to_preset(recipe: dict) -> dict:
    """Convert a bare recipe JSON spec into a minimal .xometa preset skeleton."""
    engine_a = recipe.get("engineA", recipe.get("engine_a", "Engine_A"))
    engine_b = recipe.get("engineB", recipe.get("engine_b", "Engine_B"))
    coupling_type = recipe.get("couplingType", recipe.get("coupling_type", "Amp->Filter"))
    dna = recipe.get("sonic_dna", recipe.get("dna", {
        "brightness": 0.5, "warmth": 0.5, "movement": 0.5,
        "density": 0.5, "space": 0.5, "aggression": 0.5
    }))

    return {
        "schema_version": 1,
        "name": recipe.get("name", f"{engine_a} x {engine_b}"),
        "mood": recipe.get("mood", "Entangled"),
        "sonic_dna": dna,
        "dna": dna,
        "engines": [engine_a, engine_b],
        "author": recipe.get("author", "XO_OX"),
        "version": "1.0.0",
        "description": recipe.get("description", f"Coupling recipe: {engine_a} → {engine_b} ({coupling_type})"),
        "tags": recipe.get("tags", ["coupling", "entangled"]),
        "macroLabels": recipe.get("macroLabels", ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"]),
        "couplingIntensity": recipe.get("couplingIntensity", "Medium"),
        "parameters": recipe.get("parameters", {engine_a: {}, engine_b: {}}),
        "coupling": {
            "pairs": [
                {
                    "engineA": engine_a,
                    "engineB": engine_b,
                    "type": coupling_type,
                    "amount": recipe.get("couplingAmount", 0.5),
                }
            ]
        },
    }


def build_variation(source: dict, profile: dict, name: str) -> dict:
    """Create one variation preset from source, applying the given profile."""
    variation = copy.deepcopy(source)

    variation["name"] = name

    # Shift Sonic DNA
    src_dna = source.get("sonic_dna") or source.get("dna") or {}
    new_dna = _shift_dna(src_dna, profile)
    variation["sonic_dna"] = new_dna
    variation["dna"] = new_dna

    # Scale parameters for each engine
    new_params = {}
    for engine_key, engine_params in source.get("parameters", {}).items():
        if isinstance(engine_params, dict):
            new_params[engine_key] = _scale_parameters(engine_params, profile)
        else:
            new_params[engine_key] = engine_params
    variation["parameters"] = new_params

    # Add variation metadata
    variation["variation_profile"] = profile["id"]
    variation["variation_label"] = profile["label"]
    variation["variation_description"] = profile["description"]
    variation["variation_source"] = source.get("name", "unknown")

    # Append variation tag
    tags = list(variation.get("tags", []))
    for tag in ["coupling", "entangled", profile["id"].replace("_", "-")]:
        if tag not in tags:
            tags.append(tag)
    variation["tags"] = tags

    return variation


# =============================================================================
# Main
# =============================================================================

def parse_args(argv=None):
    parser = argparse.ArgumentParser(
        description="Expand a coupling preset into a family of DNA-shifted variations."
    )
    parser.add_argument(
        "--source", required=True,
        help="Path to source .xometa preset or recipe JSON spec."
    )
    parser.add_argument(
        "--variations", type=int, default=6,
        help="Number of variations to generate (1–6, default: 6)."
    )
    parser.add_argument(
        "--output-dir", default="./expanded_variations",
        help="Output directory for generated .xometa files (default: ./expanded_variations)."
    )
    parser.add_argument(
        "--dry-run", action="store_true",
        help="Print what would be generated without writing files."
    )
    parser.add_argument(
        "--seed", type=int, default=None,
        help="Random seed for reproducible name generation."
    )
    return parser.parse_args(argv)


def main(argv=None):
    args = parse_args(argv)

    source_path = Path(args.source)
    if not source_path.exists():
        print(f"ERROR: Source file not found: {source_path}", file=sys.stderr)
        sys.exit(1)

    # Load source preset
    try:
        source = load_source(source_path)
    except (json.JSONDecodeError, OSError) as exc:
        print(f"ERROR: Failed to load source: {exc}", file=sys.stderr)
        sys.exit(1)

    base_name = source.get("name", source_path.stem)
    n = max(1, min(6, args.variations))
    profiles = VARIATION_PROFILES[:n]

    rng = random.Random(args.seed if args.seed is not None else hash(base_name) & 0xFFFFFF)

    output_dir = Path(args.output_dir)

    print(f"Source preset : {base_name}")
    print(f"Variations    : {n}")
    print(f"Output dir    : {output_dir}")
    print(f"Dry run       : {args.dry_run}")
    print()

    generated = []
    for idx, profile in enumerate(profiles):
        var_name = _generate_name(profile["id"], base_name, idx, rng)
        variation = build_variation(source, profile, var_name)
        filename = f"{var_name.replace(' ', '_')}.xometa"
        out_path = output_dir / filename
        generated.append((profile, var_name, variation, out_path))

    # Summary table
    print(f"{'#':<3} {'Profile':<20} {'Name':<28} {'File'}")
    print("-" * 80)
    for idx, (profile, var_name, variation, out_path) in enumerate(generated, 1):
        dna = variation.get("sonic_dna", {})
        dna_str = (f"br={dna.get('brightness',0):.2f} "
                   f"wm={dna.get('warmth',0):.2f} "
                   f"mv={dna.get('movement',0):.2f} "
                   f"sp={dna.get('space',0):.2f} "
                   f"ag={dna.get('aggression',0):.2f}")
        print(f"{idx:<3} {profile['label']:<20} {var_name:<28} → {out_path.name}")
        print(f"    DNA: {dna_str}")

    print()

    if args.dry_run:
        print("Dry run — no files written.")
        return

    output_dir.mkdir(parents=True, exist_ok=True)
    written = 0
    for profile, var_name, variation, out_path in generated:
        out_path.write_text(json.dumps(variation, indent=2, ensure_ascii=False), encoding="utf-8")
        written += 1
        print(f"  Written: {out_path}")

    print(f"\nDone — {written} variation(s) written to {output_dir}/")


if __name__ == "__main__":
    main()
