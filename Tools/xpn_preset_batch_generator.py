#!/usr/bin/env python3
"""
xpn_preset_batch_generator.py — Batch .xometa preset generator for XOceanus engines.

Generates preset files systematically covering the DNA space using three strategies:
  grid      — Latin hypercube sampling (even distribution across DNA space)
  random    — Random DNA within feliX/Oscar bias zone
  targeted  — Cluster around a target centroid with configurable spread

Usage:
  python xpn_preset_batch_generator.py --spec batch_spec.json [--dry-run] [--seed 42]

Spec JSON fields:
  engine              Engine name, e.g. "OPAL"
  prefix              Param prefix, e.g. "opal_"
  count               Number of presets to generate
  mood_distribution   Dict of mood -> count (must sum to count)
  dna_strategy        "grid" | "random" | "targeted"
  target_dna          (targeted only) centroid DNA values
  spread              (targeted only) spread radius, default 0.25
  felix_bias          (random only) feliX bias 0-1, shifts brightness/space up
  name_prefix         Word prepended to generated names
  output_dir          Output directory for .xometa files (relative or absolute)
  macro_labels        Optional list of 4 macro label strings
  tags_extra          Optional list of extra tags to always include
  schema_version      Schema version int, default 1
"""

import argparse
import json
import math
import random
import sys
from pathlib import Path
from typing import Dict, List, Optional, Tuple, Union

# ---------------------------------------------------------------------------
# DNA axis word banks — maps axis value ranges to adjective lists
# ---------------------------------------------------------------------------

WORD_BANK: Dict[str, List[Tuple[float, List[str]]]] = {
    # (threshold, words_for_values_above_threshold)
    "brightness": [
        (0.75, ["Radiant", "Blazing", "Crystalline", "Luminous", "Incandescent"]),
        (0.50, ["Clear", "Bright", "Open", "Gleaming", "Vivid"]),
        (0.25, ["Muted", "Veiled", "Diffuse", "Dusky", "Smoky"]),
        (0.00, ["Dark", "Shadowed", "Murky", "Deep", "Obsidian"]),
    ],
    "warmth": [
        (0.75, ["Molten", "Amber", "Velvet", "Smoldering", "Saffron"]),
        (0.50, ["Warm", "Honeyed", "Tawny", "Burnished", "Glowing"]),
        (0.25, ["Cool", "Silver", "Temperate", "Pale", "Slate"]),
        (0.00, ["Glacial", "Arctic", "Frozen", "Icy", "Cryogenic"]),
    ],
    "movement": [
        (0.75, ["Surging", "Turbulent", "Torrential", "Rushing", "Cascading"]),
        (0.50, ["Flowing", "Drifting", "Undulating", "Rolling", "Rippling"]),
        (0.25, ["Still", "Hovering", "Adrift", "Languid", "Gliding"]),
        (0.00, ["Static", "Frozen", "Inert", "Suspended", "Anchored"]),
    ],
    "density": [
        (0.75, ["Dense", "Thick", "Layered", "Saturated", "Massive"]),
        (0.50, ["Full", "Rich", "Textured", "Woven", "Lush"]),
        (0.25, ["Sparse", "Thin", "Airy", "Skeletal", "Minimal"]),
        (0.00, ["Bare", "Naked", "Void", "Empty", "Trace"]),
    ],
    "space": [
        (0.75, ["Vast", "Infinite", "Cavernous", "Oceanic", "Abyssal"]),
        (0.50, ["Deep", "Expansive", "Resonant", "Open", "Spacious"]),
        (0.25, ["Close", "Intimate", "Near", "Present", "Focused"]),
        (0.00, ["Dry", "Tight", "Direct", "Immediate", "Compact"]),
    ],
    "aggression": [
        (0.75, ["Savage", "Brutal", "Ferocious", "Violent", "Crushing"]),
        (0.50, ["Driven", "Intense", "Charged", "Sharp", "Biting"]),
        (0.25, ["Gentle", "Soft", "Subtle", "Tender", "Mild"]),
        (0.00, ["Serene", "Peaceful", "Placid", "Tranquil", "Passive"]),
    ],
}

DNA_AXES = ["brightness", "warmth", "movement", "density", "space", "aggression"]

# ---------------------------------------------------------------------------
# DNA → parameter mapping helpers
# ---------------------------------------------------------------------------

def dna_to_params(prefix: str, dna: Dict[str, float]) -> Dict[str, Union[float, int]]:
    """
    Map DNA values to a plausible parameter block for common synth axes.
    All values are normalised 0-1 unless noted.
    """
    b = dna["brightness"]
    w = dna["warmth"]
    m = dna["movement"]
    d = dna["density"]
    s = dna["space"]
    a = dna["aggression"]

    # Derived values
    cutoff_hz = 200.0 + b * 15800.0          # 200 Hz – 16 kHz
    resonance = 0.1 + a * 0.65               # 0.10 – 0.75
    drive = 0.05 + a * 0.90                  # 0.05 – 0.95
    attack = 0.005 + (1.0 - a) * 2.0        # fast when aggressive
    decay = 0.1 + m * 2.9                    # 0.1 – 3.0 s
    sustain = max(0.0, min(1.0, d * 0.9 + 0.05))
    release = 0.1 + s * 5.9                  # 0.1 – 6.0 s
    lfo_rate = 0.05 + m * 4.95              # 0.05 – 5.0 Hz
    lfo_depth = m * 0.5
    shimmer = b * 0.6
    warmth_param = w
    noise_level = a * 0.3
    sub_level = (w + d) / 2.0 * 0.6
    osc_detune = m * 0.5

    params: Dict[str, Union[float, int]] = {
        f"{prefix}filterCutoff": round(cutoff_hz, 1),
        f"{prefix}filterReso": round(resonance, 3),
        f"{prefix}drive": round(drive, 3),
        f"{prefix}attack": round(attack, 4),
        f"{prefix}decay": round(decay, 3),
        f"{prefix}sustain": round(sustain, 3),
        f"{prefix}release": round(release, 3),
        f"{prefix}lfoRate": round(lfo_rate, 3),
        f"{prefix}lfoDepth": round(lfo_depth, 3),
        f"{prefix}shimmer": round(shimmer, 3),
        f"{prefix}warmth": round(warmth_param, 3),
        f"{prefix}noiseLevel": round(noise_level, 3),
        f"{prefix}subLevel": round(sub_level, 3),
        f"{prefix}detune": round(osc_detune, 3),
        f"{prefix}level": 0.75,
    }
    return params


# ---------------------------------------------------------------------------
# Name generation
# ---------------------------------------------------------------------------

def _word_for_axis(axis: str, value: float, rng: random.Random) -> str:
    tiers = WORD_BANK[axis]
    for threshold, words in tiers:
        if value >= threshold:
            return rng.choice(words)
    return rng.choice(tiers[-1][1])


def generate_name(dna: dict[str, float], name_prefix: str, index: int, rng: random.Random) -> str:
    """Build a preset name from DNA values + user prefix."""
    # Pick the two most dominant axes by value
    ranked = sorted(DNA_AXES, key=lambda ax: dna[ax], reverse=True)
    primary_axis = ranked[0]
    secondary_axis = ranked[1]

    adj1 = _word_for_axis(primary_axis, dna[primary_axis], rng)
    adj2 = _word_for_axis(secondary_axis, dna[secondary_axis], rng)

    parts = []
    if name_prefix:
        parts.append(name_prefix)
    parts.append(adj1)
    parts.append(adj2)
    # Avoid identical names by appending a roman numeral suffix when needed
    roman = ["I", "II", "III", "IV", "V", "VI", "VII", "VIII", "IX", "X"]
    parts.append(roman[index % len(roman)])
    return " ".join(parts)


# ---------------------------------------------------------------------------
# DNA sampling strategies
# ---------------------------------------------------------------------------

def _latin_hypercube(n: int, dims: int, rng: random.Random) -> list[list[float]]:
    """Latin hypercube sampling: each axis gets n evenly-spaced strata, shuffled."""
    samples = []
    for _ in range(dims):
        strata = [(i + rng.random()) / n for i in range(n)]
        rng.shuffle(strata)
        samples.append(strata)
    # Transpose: list of n points, each with `dims` coordinates
    return [[samples[d][i] for d in range(dims)] for i in range(n)]


def sample_grid(count: int, rng: random.Random) -> list[dict[str, float]]:
    """Latin hypercube across all 6 DNA axes."""
    raw = _latin_hypercube(count, len(DNA_AXES), rng)
    return [dict(zip(DNA_AXES, pt)) for pt in raw]


def sample_random(count: int, felix_bias: float, rng: random.Random) -> list[dict[str, float]]:
    """
    Random DNA. felix_bias shifts brightness/space upward (feliX character = bright + spacious).
    A bias of 0.0 is neutral; 1.0 pushes fully into feliX territory.
    Oscar bias is the inverse (dark + dense).
    """
    results = []
    for _ in range(count):
        dna = {}
        for ax in DNA_AXES:
            v = rng.random()
            if ax in ("brightness", "space"):
                v = v * (1.0 - felix_bias * 0.4) + felix_bias * 0.4
            elif ax in ("density", "warmth"):
                # Oscar pulls density + warmth up when bias is negative
                v = v * (1.0 - (1.0 - felix_bias) * 0.3) + (1.0 - felix_bias) * 0.15
            dna[ax] = round(min(1.0, max(0.0, v)), 4)
        results.append(dna)
    return results


def sample_targeted(
    count: int,
    target: dict[str, float],
    spread: float,
    rng: random.Random,
) -> list[dict[str, float]]:
    """Cluster presets around target centroid using Gaussian perturbation."""
    results = []
    for _ in range(count):
        dna = {}
        for ax in DNA_AXES:
            centre = target.get(ax, 0.5)
            v = centre + rng.gauss(0.0, spread)
            dna[ax] = round(min(1.0, max(0.0, v)), 4)
        results.append(dna)
    return results


# ---------------------------------------------------------------------------
# Preset assembly
# ---------------------------------------------------------------------------

def build_preset(
    name: str,
    engine: str,
    prefix: str,
    mood: str,
    dna: dict[str, float],
    macro_labels: list[str],
    tags_extra: list[str],
    schema_version: int,
) -> dict:
    """Assemble a full .xometa dict."""
    # Auto-generate tags from DNA
    auto_tags: list[str] = []
    for ax in DNA_AXES:
        val = dna[ax]
        if val >= 0.7:
            auto_tags.append(ax)
        elif val <= 0.25:
            auto_tags.append(f"low-{ax}")
    tags = list(dict.fromkeys(auto_tags + tags_extra))  # dedup, preserve order

    params = dna_to_params(prefix, dna)
    engine_display = engine.capitalize() if engine.islower() else engine
    # Title-case the engine name for the parameters block key to match xometa convention
    param_engine_key = engine_display

    return {
        "schema_version": schema_version,
        "author": "XO_OX Designs",
        "version": "1.0.0",
        "couplingIntensity": "None",
        "tempo": None,
        "macroLabels": macro_labels,
        "coupling": {"pairs": []},
        "sequencer": None,
        "name": name,
        "mood": mood,
        "engines": [engine_display],
        "description": f"{name} — {mood.lower()} character preset.",
        "tags": tags,
        "dna": {ax: round(dna[ax], 4) for ax in DNA_AXES},
        "parameters": {
            param_engine_key: params,
        },
    }


# ---------------------------------------------------------------------------
# Mood distribution expansion
# ---------------------------------------------------------------------------

def expand_moods(mood_dist: Dict[str, int], count: int) -> List[str]:
    """
    Expand the mood distribution into an ordered list of mood strings.
    If the distribution doesn't sum to `count`, scale proportionally.
    """
    total = sum(mood_dist.values())
    moods: List[str] = []
    if total == count:
        for mood, n in mood_dist.items():
            moods.extend([mood] * n)
    else:
        # Scale distribution to match requested count
        remaining = count
        items = list(mood_dist.items())
        for i, (mood, n) in enumerate(items):
            if i == len(items) - 1:
                allocated = remaining
            else:
                allocated = round(n / total * count)
                remaining -= allocated
            moods.extend([mood] * max(0, allocated))
        # Trim or pad to exact count
        moods = (moods * math.ceil(count / max(len(moods), 1)))[:count]
    return moods


# ---------------------------------------------------------------------------
# Main batch generation logic
# ---------------------------------------------------------------------------

def generate_batch(spec: Dict, seed: Optional[int], dry_run: bool) -> None:
    engine: str = spec["engine"]
    prefix: str = spec.get("prefix", engine.lower() + "_")
    count: int = spec["count"]
    mood_dist: dict[str, int] = spec.get("mood_distribution", {"Foundation": count})
    strategy: str = spec.get("dna_strategy", "random")
    target_dna: dict[str, float] = spec.get("target_dna", {ax: 0.5 for ax in DNA_AXES})
    spread: float = spec.get("spread", 0.25)
    felix_bias: float = spec.get("felix_bias", 0.5)
    name_prefix: str = spec.get("name_prefix", engine.capitalize())
    output_dir: Path = Path(spec.get("output_dir", "Presets/XOceanus"))
    macro_labels: list[str] = spec.get(
        "macro_labels", ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"]
    )
    tags_extra: list[str] = spec.get("tags_extra", [])
    schema_version: int = spec.get("schema_version", 1)

    rng = random.Random(seed)

    # Sample DNA points
    if strategy == "grid":
        dna_points = sample_grid(count, rng)
    elif strategy == "targeted":
        dna_points = sample_targeted(count, target_dna, spread, rng)
    else:  # "random" (default)
        dna_points = sample_random(count, felix_bias, rng)

    # Expand mood list
    moods = expand_moods(mood_dist, count)
    # Shuffle moods so they're interleaved, not clustered
    rng.shuffle(moods)

    # Track used names to avoid collisions
    used_names = set()
    presets: List[Dict] = []

    for i, (dna, mood) in enumerate(zip(dna_points, moods)):
        # Generate unique name
        base_name = generate_name(dna, name_prefix, i, rng)
        name = base_name
        collision = 0
        while name in used_names:
            collision += 1
            name = f"{base_name} {collision}"
        used_names.add(name)

        preset = build_preset(
            name=name,
            engine=engine,
            prefix=prefix,
            mood=mood,
            dna=dna,
            macro_labels=macro_labels,
            tags_extra=tags_extra,
            schema_version=schema_version,
        )
        presets.append(preset)

    if dry_run:
        _print_dry_run(presets)
        return

    # Write files
    written = 0
    skipped = 0
    for preset in presets:
        mood = preset["mood"]
        dest_dir = output_dir / mood
        dest_dir.mkdir(parents=True, exist_ok=True)
        safe_name = preset["name"].replace("/", "-").replace("\\", "-")
        dest_file = dest_dir / f"{safe_name}.xometa"
        if dest_file.exists():
            print(f"  [SKIP] {dest_file} (already exists)")
            skipped += 1
            continue
        dest_file.write_text(json.dumps(preset, indent=2, ensure_ascii=False))
        print(f"  [WRITE] {dest_file}")
        written += 1

    print(f"\nDone. {written} written, {skipped} skipped. Strategy: {strategy}, seed: {seed}.")


def _print_dry_run(presets: List[Dict]) -> None:
    print(f"\n{'─'*64}")
    print(f"  DRY RUN — {len(presets)} presets (no files written)")
    print(f"{'─'*64}")
    header = f"{'#':>4}  {'Mood':<14} {'Name':<38}  DNA summary"
    print(header)
    print("─" * 90)
    for i, p in enumerate(presets, 1):
        dna = p["dna"]
        dna_str = " ".join(
            f"{ax[:3]}={v:.2f}" for ax, v in dna.items()
        )
        print(f"  {i:>2}.  {p['mood']:<14} {p['name']:<38}  {dna_str}")
    print(f"{'─'*64}\n")


# ---------------------------------------------------------------------------
# CLI entry point
# ---------------------------------------------------------------------------

def main() -> None:
    parser = argparse.ArgumentParser(
        description="Batch .xometa preset generator for XOceanus engines.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument(
        "--spec",
        required=True,
        metavar="FILE",
        help="Path to batch specification JSON file.",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print preset names and DNA without writing files.",
    )
    parser.add_argument(
        "--seed",
        type=int,
        default=None,
        metavar="N",
        help="Random seed for reproducibility.",
    )
    parser.add_argument(
        "--output-dir",
        default=None,
        metavar="DIR",
        help="Override output_dir from spec.",
    )
    parser.add_argument(
        "--count",
        type=int,
        default=None,
        metavar="N",
        help="Override count from spec.",
    )
    args = parser.parse_args()

    spec_path = Path(args.spec)
    if not spec_path.exists():
        print(f"Error: spec file not found: {spec_path}", file=sys.stderr)
        sys.exit(1)

    with spec_path.open() as f:
        spec: dict = json.load(f)

    # CLI overrides
    if args.output_dir is not None:
        spec["output_dir"] = args.output_dir
    if args.count is not None:
        spec["count"] = args.count

    # Validate required fields
    for required in ("engine", "count"):
        if required not in spec:
            print(f"Error: spec missing required field '{required}'", file=sys.stderr)
            sys.exit(1)

    valid_strategies = {"grid", "random", "targeted"}
    strategy = spec.get("dna_strategy", "random")
    if strategy not in valid_strategies:
        print(
            f"Error: dna_strategy must be one of {valid_strategies}, got '{strategy}'",
            file=sys.stderr,
        )
        sys.exit(1)

    if strategy == "targeted" and "target_dna" not in spec:
        print(
            "Warning: dna_strategy is 'targeted' but no target_dna provided; using 0.5 centroid.",
            file=sys.stderr,
        )

    count = spec["count"]
    if count <= 0:
        print("Error: count must be a positive integer.", file=sys.stderr)
        sys.exit(1)

    print(f"Engine : {spec['engine']}")
    print(f"Count  : {count}")
    print(f"Strategy: {strategy}")
    print(f"Seed   : {args.seed if args.seed is not None else 'not set (non-deterministic)'}")
    if not args.dry_run:
        print(f"Output : {spec.get('output_dir', 'Presets/XOceanus')}")
    print()

    generate_batch(spec=spec, seed=args.seed, dry_run=args.dry_run)


if __name__ == "__main__":
    main()
