#!/usr/bin/env python3
"""
XOceanus Preset Breeder — Genetic Sound Design

Breeds two parent presets together using crossover + mutation to produce
offspring presets. Uses the Sonic DNA system for intelligent breeding.

Usage:
    python3 breed_presets.py "Parent A Name" "Parent B Name" [--offspring N] [--mutation-rate 0.15]
    python3 breed_presets.py --random [--offspring N]
    python3 breed_presets.py --opposites [--offspring N]  # breeds the most different pair
"""

import json
import math
import random
import sys
from pathlib import Path
from datetime import date

PRESET_DIR = Path(__file__).parent.parent / "Presets" / "XOceanus"
OUTPUT_DIR = PRESET_DIR  # offspring go into the main library
TODAY = date.today().isoformat()

# Import DNA utilities
sys.path.insert(0, str(Path(__file__).parent))
from compute_preset_dna import compute_dna, euclidean_distance, find_similar, find_opposite


# ---------------------------------------------------------------------------
# Crossover
# ---------------------------------------------------------------------------

def crossover(parent_a: dict, parent_b: dict) -> dict:
    """
    Genetic crossover: for each parameter, randomly select from Parent A or B.
    For engine-specific params, only cross over parameters that exist in both parents.
    """
    engines_a = set(parent_a.get("engines", []))
    engines_b = set(parent_b.get("engines", []))
    shared_engines = engines_a & engines_b
    all_engines = engines_a | engines_b

    child_params = {}
    child_engines = []

    for engine in all_engines:
        params_a = parent_a.get("parameters", {}).get(engine, {})
        params_b = parent_b.get("parameters", {}).get(engine, {})

        if engine in shared_engines:
            # Both parents have this engine — crossover
            child_engines.append(engine)
            all_keys = set(params_a.keys()) | set(params_b.keys())
            child_p = {}
            for key in all_keys:
                if key in params_a and key in params_b:
                    # Both parents have it — 50/50 pick
                    child_p[key] = random.choice([params_a[key], params_b[key]])
                elif key in params_a:
                    # Only A has it — 70% chance to include
                    if random.random() < 0.7:
                        child_p[key] = params_a[key]
                else:
                    # Only B has it — 70% chance to include
                    if random.random() < 0.7:
                        child_p[key] = params_b[key]
            child_params[engine] = child_p
        else:
            # Only one parent has this engine — include with 50% chance
            if random.random() < 0.5:
                child_engines.append(engine)
                source = params_a if engine in engines_a else params_b
                child_params[engine] = dict(source)

    # Must have at least one engine
    if not child_engines:
        fallback = random.choice(list(all_engines))
        child_engines = [fallback]
        source_params = parent_a.get("parameters", {}).get(fallback) or \
                       parent_b.get("parameters", {}).get(fallback, {})
        child_params[fallback] = dict(source_params)

    return {
        "engines": sorted(child_engines),
        "parameters": child_params,
    }


# ---------------------------------------------------------------------------
# Mutation — USER DECISION POINT
# ---------------------------------------------------------------------------

def mutate(params: dict, engine: str, rate: float = 0.15,
           parent_a_dna: dict = None, parent_b_dna: dict = None) -> dict:
    """
    Apply mutations to offspring parameters.

    Strategy: DNA-aware hybrid mutation.
      - Base mutation rate applies uniformly to all parameters
      - Parameters that map to DNA dimensions where parents DIFFER MOST
        get a 2x mutation boost (more exploration in the interesting space)
      - Mutation magnitude: Gaussian noise, ±10% of parameter value
      - Frequency/cutoff params use log-scale mutation to stay musical

    Args:
        params: parameter dict for one engine
        engine: engine name (for knowing parameter semantics)
        rate: base probability that any parameter mutates (0.0–1.0)
        parent_a_dna: DNA vector of parent A (optional, for DNA-aware mode)
        parent_b_dna: DNA vector of parent B (optional, for DNA-aware mode)
    """
    # Identify which DNA dimensions the parents differ most on
    dna_divergence = {}
    if parent_a_dna and parent_b_dna:
        for dim in ["brightness", "warmth", "movement", "density", "space", "aggression"]:
            dna_divergence[dim] = abs(parent_a_dna.get(dim, 0.5) - parent_b_dna.get(dim, 0.5))

    # Map parameter names to DNA dimensions (rough heuristic)
    PARAM_TO_DNA = {
        "cutoff": "brightness", "filter": "brightness", "shimmer": "brightness",
        "tone": "brightness", "bright": "brightness",
        "drift": "warmth", "sub": "warmth", "haze": "warmth", "warm": "warmth",
        "bob": "warmth", "bloom": "warmth",
        "lfo": "movement", "tidal": "movement", "morph": "movement",
        "cur": "movement", "mod": "movement", "rate": "movement", "wow": "movement",
        "detune": "density", "unison": "density", "blend": "density",
        "poly": "density", "level": "density", "fm": "density",
        "reverb": "space", "delay": "space", "space": "space",
        "chorus": "space", "phaser": "space", "send": "space",
        "drive": "aggression", "reso": "aggression", "comp": "aggression",
        "snap": "aggression", "fracture": "aggression", "dust": "aggression",
    }

    mutated = dict(params)

    for key, value in params.items():
        if not isinstance(value, (int, float)):
            continue

        # Determine effective mutation rate
        effective_rate = rate

        # DNA-aware boost: find which DNA dimension this param maps to
        if dna_divergence:
            key_lower = key.lower()
            for keyword, dim in PARAM_TO_DNA.items():
                if keyword in key_lower:
                    divergence = dna_divergence.get(dim, 0)
                    # Higher divergence = higher mutation rate (up to 2x)
                    effective_rate = rate * (1.0 + divergence)
                    break

        if random.random() > effective_rate:
            continue

        # Apply mutation
        if isinstance(value, int):
            # Integer params: ±1 with occasional ±2
            delta = random.choice([-2, -1, -1, 1, 1, 2])
            mutated[key] = max(0, value + delta)
        else:
            # Float params: Gaussian noise
            # Use log-scale for frequency-like params
            key_lower = key.lower()
            if any(f in key_lower for f in ["cutoff", "freq", "rate"]) and value > 100:
                # Log-scale mutation for frequencies
                log_val = math.log(max(value, 1))
                log_val += random.gauss(0, 0.15)
                mutated[key] = round(math.exp(log_val), 3)
            else:
                # Linear mutation: ±10% of value, minimum ±0.02
                magnitude = max(abs(value) * 0.1, 0.02)
                delta = random.gauss(0, magnitude)
                new_val = value + delta
                # Clamp 0-1 params (heuristic: if original is 0-1 range)
                if 0 <= value <= 1:
                    new_val = max(0.0, min(1.0, new_val))
                mutated[key] = round(new_val, 4)

    return mutated


# ---------------------------------------------------------------------------
# Naming
# ---------------------------------------------------------------------------

def breed_name(name_a: str, name_b: str, generation: int = 1) -> str:
    """Generate a child name by blending parent names."""
    words_a = name_a.split()
    words_b = name_b.split()

    strategies = [
        # Take first word of A, last word of B
        lambda: f"{words_a[0]} {words_b[-1]}",
        # Take first word of B, last word of A
        lambda: f"{words_b[0]} {words_a[-1]}",
        # Blend: first half of A's first word + second half of B's last word
        lambda: f"{words_a[0][:len(words_a[0])//2+1]}{words_b[-1][len(words_b[-1])//2:]}",
        # Prefix mutation
        lambda: f"{'Neo ' if generation == 1 else 'Ultra '}{random.choice(words_a + words_b)}",
    ]

    name = random.choice(strategies)()

    # Ensure uniqueness and length
    if len(name) > 28:
        name = name[:28]

    return name.strip()


# ---------------------------------------------------------------------------
# Breed
# ---------------------------------------------------------------------------

def breed(parent_a: dict, parent_b: dict, mutation_rate: float = 0.15) -> dict:
    """Breed two presets into an offspring."""

    # Crossover parameters
    child = crossover(parent_a, parent_b)

    # Get parent DNAs for mutation guidance
    dna_a = parent_a.get("dna")
    dna_b = parent_b.get("dna")

    # Mutate each engine's parameters
    for engine in child["engines"]:
        child["parameters"][engine] = mutate(
            child["parameters"][engine], engine, mutation_rate,
            parent_a_dna=dna_a, parent_b_dna=dna_b
        )

    # Pick mood: from whichever parent contributed more parameters
    total_a = sum(1 for e in child["engines"]
                  for k in child["parameters"].get(e, {})
                  if k in parent_a.get("parameters", {}).get(e, {}))
    total_b = sum(1 for e in child["engines"]
                  for k in child["parameters"].get(e, {})
                  if k in parent_b.get("parameters", {}).get(e, {}))
    mood = parent_a["mood"] if total_a >= total_b else parent_b["mood"]

    # Generate name
    name = breed_name(parent_a["name"], parent_b["name"])

    # Build offspring preset
    offspring = {
        "schema_version": 1,
        "name": name,
        "mood": mood,
        "engines": child["engines"],
        "author": "XO_OX",
        "version": "1.0.0",
        "description": f"Bred from '{parent_a['name']}' × '{parent_b['name']}'.",
        "tags": list(set(
            parent_a.get("tags", [])[:3] + parent_b.get("tags", [])[:3] + ["bred"]
        )),
        "macroLabels": parent_a.get("macroLabels", ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"]),
        "couplingIntensity": random.choice([
            parent_a.get("couplingIntensity", "None"),
            parent_b.get("couplingIntensity", "None"),
        ]),
        "tempo": parent_a.get("tempo") or parent_b.get("tempo"),
        "created": TODAY,
        "legacy": {
            "sourceInstrument": None,
            "sourceCategory": None,
            "sourcePresetName": None,
        },
        "lineage": {
            "parentA": parent_a["name"],
            "parentB": parent_b["name"],
            "generation": 1,
            "mutationRate": mutation_rate,
        },
        "parameters": child["parameters"],
        "coupling": None,
        "sequencer": None,
    }

    # Compute DNA for offspring
    offspring["dna"] = compute_dna(offspring)

    return offspring


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def load_all_presets() -> list:
    """Load all .xometa presets from the library."""
    presets = []
    for fpath in PRESET_DIR.rglob("*.xometa"):
        try:
            presets.append(json.loads(fpath.read_text()))
        except json.JSONDecodeError:
            continue
    return presets


def find_preset(name: str, presets: list) -> dict:
    """Find a preset by name (case-insensitive)."""
    name_lower = name.lower()
    for p in presets:
        if p["name"].lower() == name_lower:
            return p
    return None


def main():
    n_offspring = 4
    mutation_rate = 0.15

    # Parse args
    args = sys.argv[1:]
    positional = []
    use_random = False
    use_opposites = False

    i = 0
    while i < len(args):
        if args[i] == "--offspring" and i + 1 < len(args):
            n_offspring = int(args[i + 1])
            i += 2
        elif args[i] == "--mutation-rate" and i + 1 < len(args):
            mutation_rate = float(args[i + 1])
            i += 2
        elif args[i] == "--random":
            use_random = True
            i += 1
        elif args[i] == "--opposites":
            use_opposites = True
            i += 1
        else:
            positional.append(args[i])
            i += 1

    # Load library
    all_presets = load_all_presets()
    print(f"Loaded {len(all_presets)} presets\n")

    if use_random:
        parent_a, parent_b = random.sample(all_presets, 2)
    elif use_opposites:
        # Find the pair with maximum DNA distance
        max_dist = 0
        best_pair = (None, None)
        for ia, a in enumerate(all_presets):
            if "dna" not in a: continue
            for b in all_presets[ia+1:]:
                if "dna" not in b: continue
                d = euclidean_distance(a["dna"], b["dna"])
                if d > max_dist:
                    max_dist = d
                    best_pair = (a, b)
        parent_a, parent_b = best_pair
        print(f"Most opposite pair (distance {max_dist:.3f}):")
    elif len(positional) >= 2:
        parent_a = find_preset(positional[0], all_presets)
        parent_b = find_preset(positional[1], all_presets)
        if not parent_a:
            print(f"ERROR: Preset not found: '{positional[0]}'")
            return 1
        if not parent_b:
            print(f"ERROR: Preset not found: '{positional[1]}'")
            return 1
    else:
        print("Usage: breed_presets.py \"Parent A\" \"Parent B\" [--offspring N]")
        print("       breed_presets.py --random [--offspring N]")
        print("       breed_presets.py --opposites [--offspring N]")
        return 1

    print(f"Parent A: {parent_a['name']} [{', '.join(parent_a['engines'])}] ({parent_a['mood']})")
    if parent_a.get("dna"):
        d = parent_a["dna"]
        print(f"  DNA: B={d['brightness']:.2f} W={d['warmth']:.2f} M={d['movement']:.2f} "
              f"D={d['density']:.2f} S={d['space']:.2f} A={d['aggression']:.2f}")

    print(f"Parent B: {parent_b['name']} [{', '.join(parent_b['engines'])}] ({parent_b['mood']})")
    if parent_b.get("dna"):
        d = parent_b["dna"]
        print(f"  DNA: B={d['brightness']:.2f} W={d['warmth']:.2f} M={d['movement']:.2f} "
              f"D={d['density']:.2f} S={d['space']:.2f} A={d['aggression']:.2f}")

    if parent_a.get("dna") and parent_b.get("dna"):
        dist = euclidean_distance(parent_a["dna"], parent_b["dna"])
        print(f"\nGenetic distance: {dist:.3f}")

    print(f"\nBreeding {n_offspring} offspring (mutation rate: {mutation_rate:.0%})...\n")

    for i in range(n_offspring):
        offspring = breed(parent_a, parent_b, mutation_rate)
        d = offspring["dna"]

        print(f"  [{i+1}] {offspring['name']}")
        print(f"      Engines: {', '.join(offspring['engines'])} | Mood: {offspring['mood']}")
        print(f"      DNA: B={d['brightness']:.2f} W={d['warmth']:.2f} M={d['movement']:.2f} "
              f"D={d['density']:.2f} S={d['space']:.2f} A={d['aggression']:.2f}")
        print(f"      Tags: {', '.join(offspring['tags'][:5])}")

        # Save
        mood_dir = OUTPUT_DIR / offspring["mood"]
        mood_dir.mkdir(parents=True, exist_ok=True)
        out_path = mood_dir / f"{offspring['name']}.xometa"

        # Avoid overwriting
        suffix = 0
        while out_path.exists():
            suffix += 1
            out_path = mood_dir / f"{offspring['name']} {suffix}.xometa"

        with open(out_path, "w") as f:
            json.dump(offspring, f, indent=2)
        print(f"      Saved: {out_path.relative_to(PRESET_DIR)}")
        print()

    return 0


if __name__ == "__main__":
    sys.exit(main())
