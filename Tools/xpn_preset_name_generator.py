#!/usr/bin/env python3
"""
xpn_preset_name_generator.py — XO_OX preset name generator from Sonic DNA.

Generates evocative 2-3 word preset names based on the 6D Sonic DNA profile.
Names follow XO_OX brand convention: evocative, no jargon, no special chars
except hyphens.

Usage:
    python xpn_preset_name_generator.py \\
        --brightness 0.8 --warmth 0.2 --movement 0.6 \\
        --density 0.3 --space 0.7 --aggression 0.1 \\
        [--count 10] [--existing-dir <presets_dir>] [--seed 42]
"""

import argparse
import json
import random
import sys
from pathlib import Path

# ---------------------------------------------------------------------------
# Word Banks — keyed by (dimension, polarity)
# ---------------------------------------------------------------------------

WORD_BANKS = {
    # Brightness
    ("brightness", "high"): ["Crystal", "Glass", "Neon", "Sharp", "Prism", "Arctic", "Chrome"],
    ("brightness", "low"):  ["Shadow", "Dusk", "Velvet", "Smoke", "Obsidian", "Murk", "Dim"],

    # Warmth
    ("warmth", "high"): ["Amber", "Ember", "Honey", "Cedar", "Saffron", "Rust", "Hearth"],
    ("warmth", "low"):  ["Frost", "Steel", "Void", "Glacial", "Porcelain", "Chalk"],

    # Movement
    ("movement", "high"): ["Drift", "Flutter", "Cascade", "Spiral", "Surge", "Pulse", "Storm"],
    ("movement", "low"):  ["Still", "Held", "Frozen", "Steady", "Poised", "Anchored"],

    # Density
    ("density", "high"): ["Dense", "Cluster", "Layer", "Thicket", "Swarm", "Mass"],
    ("density", "low"):  ["Sparse", "Thread", "Filament", "Wisp", "Trace", "Shimmer"],

    # Space
    ("space", "high"): ["Cathedral", "Vast", "Ocean", "Horizon", "Expanse", "Beyond"],
    ("space", "low"):  ["Close", "Tight", "Intimate", "Dry", "Direct", "Booth"],

    # Aggression
    ("aggression", "high"): ["Bite", "Grind", "Slash", "Strike", "Crush", "Raw"],
    ("aggression", "low"):  ["Gentle", "Soft", "Tender", "Calm", "Quiet", "Serene"],
}

# Connector / mid words for 3-word names — evocative bridge words
CONNECTORS = [
    "of", "in", "at", "and", "the", "a",
]

# Evocative nouns usable as standalone second/third words (texture, scene)
SCENE_WORDS = [
    "Light", "Field", "Rain", "Bloom", "Tide", "Haze", "Wind", "Shore",
    "Glow", "Veil", "Fold", "Root", "Moss", "Salt", "Flint", "Fog",
    "Bone", "Ash", "Silk", "Brine", "Dew", "Slate", "Bark", "Ore",
    "Ink", "Sand", "Coal", "Petal", "Grain", "Flare", "Husk", "Dawn",
    "Hollow", "Basin", "Ledge", "Plume", "Ridge", "Canopy", "Depth",
]

# ---------------------------------------------------------------------------
# Jargon blacklist (sourced from xpn_pack_naming_validator.py)
# ---------------------------------------------------------------------------

JARGON_BLACKLIST = {
    "LPF", "HPF", "BPF", "VCA", "VCO", "VCF", "OSC", "ENV", "LFO",
    "ADSR", "EQ", "HZ", "DB", "MS",
}

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

DIMENSIONS = ["brightness", "warmth", "movement", "density", "space", "aggression"]
THRESHOLD_HIGH = 0.6
THRESHOLD_LOW  = 0.4


def polarity(value: float) -> str:
    """Return 'high', 'low', or 'mid' for a 0-1 DNA value."""
    if value >= THRESHOLD_HIGH:
        return "high"
    if value <= THRESHOLD_LOW:
        return "low"
    return "mid"


def dominant_dimensions(dna: dict, n: int = 3) -> list[tuple[str, str]]:
    """Return the n most extreme dimensions as (dimension, polarity) pairs.

    'Most extreme' = furthest from 0.5.
    """
    scored = []
    for dim in DIMENSIONS:
        v = dna[dim]
        distance = abs(v - 0.5)
        pol = polarity(v)
        if pol != "mid":
            scored.append((distance, dim, pol))
    scored.sort(reverse=True)
    return [(dim, pol) for _, dim, pol in scored[:n]]


def pick_word(rng: random.Random, dim: str, pol: str) -> str:
    """Pick a random word from the appropriate bank."""
    key = (dim, pol)
    bank = WORD_BANKS.get(key, [])
    if not bank:
        return rng.choice(SCENE_WORDS)
    return rng.choice(bank)


def is_jargon_clean(name: str) -> bool:
    """Return True if name contains no blacklisted jargon words."""
    for word in name.split():
        if word.upper().rstrip("-") in JARGON_BLACKLIST:
            return False
    return True


def load_existing_names(presets_dir: Path) -> set[str]:
    """Collect all preset names from .xometa files under presets_dir."""
    names: set[str] = set()
    for filepath in presets_dir.rglob("*.xometa"):
        try:
            with filepath.open("r", encoding="utf-8") as f:
                data = json.load(f)
            name = data.get("name") or data.get("presetName") or ""
            if name:
                names.add(name.strip().lower())
        except (json.JSONDecodeError, OSError):
            pass
    return names


def title_case(name: str) -> str:
    """Title-case a preset name, handling hyphens and connectors correctly."""
    LOWER_CONNECTORS = {"of", "in", "at", "and", "a", "the"}
    words = name.split()
    result = []
    for i, word in enumerate(words):
        if "-" in word:
            # Capitalize each part of a hyphenated compound
            parts = [p.capitalize() for p in word.split("-")]
            result.append("-".join(parts))
        elif i > 0 and word.lower() in LOWER_CONNECTORS:
            result.append(word.lower())
        else:
            result.append(word.capitalize())
    return " ".join(result)


# ---------------------------------------------------------------------------
# Name generation strategies
# ---------------------------------------------------------------------------

def generate_two_word(rng: random.Random, doms: list[tuple[str, str]]) -> str:
    """adj + noun — first dominant dim + second dominant or scene word."""
    if len(doms) >= 2:
        w1 = pick_word(rng, *doms[0])
        w2 = pick_word(rng, *doms[1])
    elif len(doms) == 1:
        w1 = pick_word(rng, *doms[0])
        w2 = rng.choice(SCENE_WORDS)
    else:
        w1 = rng.choice(SCENE_WORDS)
        w2 = rng.choice(SCENE_WORDS)
    return f"{w1} {w2}"


def generate_three_word_adj_noun_noun(rng: random.Random, doms: list[tuple[str, str]]) -> str:
    """adj + noun + noun — three dominant dims."""
    words = []
    for i in range(min(3, len(doms))):
        words.append(pick_word(rng, *doms[i]))
    while len(words) < 3:
        words.append(rng.choice(SCENE_WORDS))
    return f"{words[0]} {words[1]} {words[2]}"


def generate_three_word_adj_of_noun(rng: random.Random, doms: list[tuple[str, str]]) -> str:
    """adj + 'of' + noun — e.g., 'Crystal Veil of Frost'."""
    w1 = pick_word(rng, *doms[0]) if doms else rng.choice(SCENE_WORDS)
    w2 = rng.choice(SCENE_WORDS)
    return f"{w1} of {w2}"


def generate_hyphen_compound(rng: random.Random, doms: list[tuple[str, str]]) -> str:
    """two words joined by hyphen — e.g., 'Glass-Drift'."""
    if len(doms) >= 2:
        w1 = pick_word(rng, *doms[0])
        w2 = pick_word(rng, *doms[1])
    else:
        w1 = rng.choice(SCENE_WORDS)
        w2 = rng.choice(SCENE_WORDS)
    return f"{w1}-{w2}"


STRATEGIES = [
    generate_two_word,
    generate_two_word,           # weighted: 2-word more common
    generate_two_word,
    generate_three_word_adj_noun_noun,
    generate_three_word_adj_of_noun,
    generate_hyphen_compound,
]


def generate_candidates(
    dna: dict,
    count: int,
    rng: random.Random,
    existing_names: set[str],
    max_attempts: int = 500,
) -> list[str]:
    """Generate `count` unique, clean preset name candidates."""
    doms = dominant_dimensions(dna, n=3)
    results: list[str] = []
    seen: set[str] = set()
    attempts = 0

    while len(results) < count and attempts < max_attempts:
        attempts += 1
        strategy = rng.choice(STRATEGIES)
        raw = strategy(rng, doms)
        name = title_case(raw)

        # Enforce max 30 chars (XO_OX rule)
        if len(name) > 30:
            continue

        # Jargon check
        if not is_jargon_clean(name):
            continue

        # Deduplicate within candidates and against existing
        key = name.lower()
        if key in seen or key in existing_names:
            continue

        seen.add(key)
        results.append(name)

    return results


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        description="Generate XO_OX-style preset names from 6D Sonic DNA.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python xpn_preset_name_generator.py \\
      --brightness 0.8 --warmth 0.2 --movement 0.6 \\
      --density 0.3 --space 0.7 --aggression 0.1

  python xpn_preset_name_generator.py \\
      --brightness 0.2 --warmth 0.9 --movement 0.1 \\
      --density 0.8 --space 0.3 --aggression 0.5 \\
      --count 5 --seed 42

  python xpn_preset_name_generator.py \\
      --brightness 0.5 --warmth 0.5 --movement 0.5 \\
      --density 0.5 --space 0.5 --aggression 0.5 \\
      --existing-dir Presets/XOceanus/
""",
    )
    for dim in DIMENSIONS:
        p.add_argument(
            f"--{dim}",
            type=float,
            required=True,
            metavar="0.0-1.0",
            help=f"DNA {dim} value (0.0–1.0)",
        )
    p.add_argument(
        "--count",
        type=int,
        default=10,
        metavar="N",
        help="Number of candidate names to generate (default: 10)",
    )
    p.add_argument(
        "--existing-dir",
        type=Path,
        default=None,
        metavar="DIR",
        help="Directory to scan for existing .xometa presets (deduplication)",
    )
    p.add_argument(
        "--seed",
        type=int,
        default=None,
        help="Random seed for reproducible output",
    )
    p.add_argument(
        "--json",
        action="store_true",
        help="Output results as JSON array instead of plain text",
    )
    return p


def validate_dna(dna: dict) -> list[str]:
    errors = []
    for dim, val in dna.items():
        if not (0.0 <= val <= 1.0):
            errors.append(f"--{dim} must be between 0.0 and 1.0, got {val}")
    return errors


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()

    dna = {dim: getattr(args, dim) for dim in DIMENSIONS}

    errors = validate_dna(dna)
    if errors:
        for e in errors:
            print(f"ERROR: {e}", file=sys.stderr)
        return 1

    rng = random.Random(args.seed)

    existing_names: set[str] = set()
    if args.existing_dir is not None:
        existing_dir = args.existing_dir
        if not existing_dir.is_dir():
            print(f"ERROR: --existing-dir '{existing_dir}' is not a directory.", file=sys.stderr)
            return 1
        existing_names = load_existing_names(existing_dir)

    candidates = generate_candidates(
        dna=dna,
        count=args.count,
        rng=rng,
        existing_names=existing_names,
    )

    if not candidates:
        print("WARNING: No candidates generated — try a different seed or relax DNA values.",
              file=sys.stderr)
        return 1

    if args.json:
        print(json.dumps(candidates, indent=2))
    else:
        print(f"Generated {len(candidates)} preset name(s) for DNA profile:")
        for dim in DIMENSIONS:
            pol = polarity(dna[dim])
            print(f"  {dim:>12}: {dna[dim]:.2f}  ({pol})")
        print()
        for name in candidates:
            print(f"  {name}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
