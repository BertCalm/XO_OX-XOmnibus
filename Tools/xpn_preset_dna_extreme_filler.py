#!/usr/bin/env python3
"""
xpn_preset_dna_extreme_filler.py — XO_OX DNA Extreme Zone Meta-Filler

Analyzes the current fleet DNA distribution across all 6 dimensions, identifies
which are most compressed (high % of presets in the 0.25-0.75 midrange), then
auto-generates extreme-zone stub presets using engine-appropriate candidates.

Combines the functionality of individual expansion pack tools (brightness, warmth,
density, aggression, movement, space) into one smart dispatcher.

Usage:
    python xpn_preset_dna_extreme_filler.py
    python xpn_preset_dna_extreme_filler.py --dimensions auto
    python xpn_preset_dna_extreme_filler.py --dimensions brightness,aggression
    python xpn_preset_dna_extreme_filler.py --dry-run --seed 42
    python xpn_preset_dna_extreme_filler.py --presets-dir ./Presets/XOceanus --count 3
"""

import argparse
import json
import os
import random
import statistics
import sys
from pathlib import Path
from datetime import date

# ---------------------------------------------------------------------------
# Engine-to-Dimension Extreme Map
# ---------------------------------------------------------------------------
# Each entry lists engine short names best suited for that extreme zone.
# brightness_high = engines whose character shines bright / airy / crystalline
# brightness_low  = engines whose character lives in dark / murky / subterranean

ENGINE_EXTREMES: dict[str, list[str]] = {
    "brightness_high": ["OPTIC", "OVERWORLD", "OBLIQUE", "ORIGAMI", "ORPHICA", "ODDFELIX"],
    "brightness_low":  ["ORCA", "OVERDUB", "OWLFISH", "OCEANIC", "ODDOSCAR"],
    "warmth_high":     ["OVERDUB", "OVERBITE", "OBLONG", "OSTERIA", "OHM", "OBESE"],
    "warmth_low":      ["ODYSSEY", "OVERWORLD", "OPTIC", "OUTWIT", "ORACLE"],
    "density_high":    ["OPAL", "OBESE", "OUROBOROS", "ORGANON", "ONSET", "OVERLAP"],
    "density_low":     ["ODYSSEY", "OVERWORLD", "OPTIC", "ORIGAMI", "OBSCURA"],
    "movement_high":   ["OPTIC", "OUROBOROS", "ODYSSEY", "OLE", "OUTWIT", "ORACLE"],
    "movement_low":    ["OBSIDIAN", "OWLFISH", "OBSCURA", "OVERDUB", "OCELOT"],
    "space_high":      ["OPAL", "ORACLE", "OVERDUB", "OCEANIC", "OWLFISH", "ORGANON"],
    "space_low":       ["ONSET", "OBLONG", "OBESE", "OVERBITE", "OVERWORLD"],
    "aggression_high": ["ONSET", "OBESE", "OUROBOROS", "OVERWORLD", "ORCA", "OCTOPUS"],
    "aggression_low":  ["OBSIDIAN", "OPAL", "OVERDUB", "ORACLE", "OWLFISH"],
}

# Mood assignment per extreme zone
ZONE_MOOD: dict[str, str] = {
    "brightness_high": "Prism",
    "brightness_low":  "Aether",
    "warmth_high":     "Foundation",
    "warmth_low":      "Prism",
    "density_high":    "Foundation",
    "density_low":     "Aether",
    "movement_high":   "Flux",
    "movement_low":    "Atmosphere",
    "space_high":      "Aether",
    "space_low":       "Flux",
    "aggression_high": "Flux",
    "aggression_low":  "Atmosphere",
}

# All 6 DNA dimensions
DIMENSIONS = ["brightness", "warmth", "density", "movement", "space", "aggression"]

# Extreme bands
LOW_BAND  = (0.0, 0.15)
HIGH_BAND = (0.85, 1.0)
MID_BAND  = (0.25, 0.75)

# ---------------------------------------------------------------------------
# Name vocabulary per dimension × polarity
# ---------------------------------------------------------------------------

NAME_VOCAB: dict[str, list[str]] = {
    "brightness_high": [
        "Crystalline", "Radiant", "Scintilla", "Luminous", "Solar",
        "Quartz", "Vitreous", "Coronal", "Phosphor", "Incandescent",
        "Spectral", "Gleam", "Brilliance", "Prismatic", "Photon",
    ],
    "brightness_low": [
        "Void", "Abyss", "Murk", "Obsidian", "Pitch",
        "Tenebrous", "Umbral", "Stygian", "Crepuscular", "Blackwater",
        "Subterranean", "Nocturne", "Eclipse", "Penumbra", "Chasm",
    ],
    "warmth_high": [
        "Ember", "Molten", "Amber", "Hearthstone", "Smoldering",
        "Velvet", "Sunbaked", "Cinnamon", "Ochre", "Fireside",
        "Honeyed", "Caramel", "Sienna", "Tallow", "Furnace",
    ],
    "warmth_low": [
        "Glacial", "Sterile", "Aluminum", "Clinical", "Crystalline",
        "Permafrost", "Antiseptic", "Titanium", "Austere", "Polar",
        "Cryogenic", "Cerulean", "Tundra", "Brittle", "Anechoic",
    ],
    "density_high": [
        "Saturate", "Swarm", "Colossus", "Mass", "Accretion",
        "Sediment", "Monolith", "Cluster", "Confluence", "Aggregate",
        "Stratum", "Convergence", "Overload", "Dense", "Tectonic",
    ],
    "density_low": [
        "Sparse", "Filament", "Solitary", "Wisp", "Single",
        "Alone", "Stripped", "Bare", "Thread", "Sliver",
        "Singular", "Loner", "Ghost Note", "Thin", "Isolated",
    ],
    "movement_high": [
        "Kinetic", "Frantic", "Turbulent", "Storm", "Vortex",
        "Flicker", "Cascade", "Tremolo", "Frenzy", "Rapid",
        "Oscillate", "Flutter", "Surge", "Accelerate", "Torrent",
    ],
    "movement_low": [
        "Stasis", "Frozen", "Inert", "Suspended", "Static",
        "Glacial", "Motionless", "Arrested", "Dormant", "Tectonic",
        "Becalmed", "Still Point", "Held", "Plateau", "Petrified",
    ],
    "space_high": [
        "Cavern", "Expanse", "Infinite", "Cosmos", "Cathedral",
        "Void", "Boundless", "Atmospheric", "Horizon", "Interstellar",
        "Cavernous", "Expansive", "Reverberant", "Open", "Hall",
    ],
    "space_low": [
        "Close", "Intimate", "Dry", "Direct", "Contact",
        "Compressed", "Booth", "Proximate", "Cellular", "Anechoic",
        "Tight", "Close-mic", "Arid", "Contained", "Compact",
    ],
    "aggression_high": [
        "Savage", "Feral", "Assault", "Brutal", "Shred",
        "Violent", "Predator", "Rampage", "Obliterate", "Ferocious",
        "Hostile", "Vicious", "Wrathful", "Raze", "Devastate",
    ],
    "aggression_low": [
        "Tender", "Docile", "Serene", "Gentle", "Placid",
        "Lullaby", "Whisper", "Pastoral", "Soothe", "Meek",
        "Velvet", "Delicate", "Tranquil", "Mild", "Hushed",
    ],
}

SUFFIXES = [
    "Drift", "Wave", "Pulse", "Field", "Current",
    "Flow", "Layer", "Cloud", "Zone", "Signal",
    "Bloom", "Haze", "State", "Form", "Mode",
]

# ---------------------------------------------------------------------------
# Engine → canonical XOceanus engine ID (for preset "engines" field)
# ---------------------------------------------------------------------------

ENGINE_ID_MAP: dict[str, str] = {
    "ODDFELIX":   "OddfeliX",
    "ODDOSCAR":   "OddOscar",
    "OVERDUB":    "Overdub",
    "ODYSSEY":    "Odyssey",
    "OBLONG":     "Oblong",
    "OBESE":      "Obese",
    "ONSET":      "Onset",
    "OVERWORLD":  "Overworld",
    "OPAL":       "Opal",
    "ORBITAL":    "Orbital",
    "ORGANON":    "Organon",
    "OUROBOROS":  "Ouroboros",
    "OBSIDIAN":   "Obsidian",
    "OVERBITE":   "Overbite",
    "ORIGAMI":    "Origami",
    "ORACLE":     "Oracle",
    "OBSCURA":    "Obscura",
    "OCEANIC":    "Oceanic",
    "OCELOT":     "Ocelot",
    "OPTIC":      "Optic",
    "OBLIQUE":    "Oblique",
    "OSPREY":     "Osprey",
    "OSTERIA":    "Osteria",
    "OWLFISH":    "Owlfish",
    "OHM":        "Ohm",
    "ORPHICA":    "Orphica",
    "OBBLIGATO":  "Obbligato",
    "OTTONI":     "Ottoni",
    "OLE":        "Ole",
    "OMBRE":      "Ombre",
    "ORCA":       "Orca",
    "OCTOPUS":    "Octopus",
    "OVERLAP":    "Overlap",
    "OUTWIT":     "Outwit",
}

# ---------------------------------------------------------------------------
# Tag vocabulary per dimension × polarity
# ---------------------------------------------------------------------------

TAGS_VOCAB: dict[str, list[str]] = {
    "brightness_high": ["bright", "airy", "crystalline", "shimmer", "luminous"],
    "brightness_low":  ["dark", "murky", "subterranean", "void", "nocturnal"],
    "warmth_high":     ["warm", "analog", "rich", "saturated", "organic"],
    "warmth_low":      ["cold", "clinical", "digital", "sterile", "icy"],
    "density_high":    ["dense", "layered", "complex", "massive", "thick"],
    "density_low":     ["sparse", "minimal", "stripped", "solo", "clean"],
    "movement_high":   ["animated", "kinetic", "tremolo", "turbulent", "fast"],
    "movement_low":    ["static", "frozen", "sustained", "still", "held"],
    "space_high":      ["reverberant", "expansive", "cavernous", "spatial", "wide"],
    "space_low":       ["dry", "intimate", "close", "direct", "compressed"],
    "aggression_high": ["aggressive", "brutal", "savage", "driven", "intense"],
    "aggression_low":  ["gentle", "soft", "tender", "peaceful", "delicate"],
}

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def load_all_xometa(presets_dir: Path) -> list[dict]:
    """Recursively load all .xometa files from presets_dir."""
    presets = []
    for path in presets_dir.rglob("*.xometa"):
        try:
            with open(path, "r", encoding="utf-8") as f:
                data = json.load(f)
            data["_path"] = str(path)
            presets.append(data)
        except (json.JSONDecodeError, OSError) as exc:
            print(f"[WARN] Loading preset {path.name}: {exc}", file=sys.stderr)
    return presets


def extract_dna_values(presets: list[dict], dimension: str) -> list[float]:
    """Return list of DNA values for a given dimension across all presets."""
    values = []
    for p in presets:
        dna = p.get("dna") or {}
        if dimension in dna and dna[dimension] is not None:
            try:
                values.append(float(dna[dimension]))
            except (TypeError, ValueError) as exc:
                print(f"[WARN] Parsing DNA value for dimension '{dimension}': {exc}", file=sys.stderr)
    return values


def compression_score(values: list[float]) -> float:
    """Return fraction of values in the 0.25-0.75 midrange (0.0-1.0)."""
    if not values:
        return 0.0
    mid_count = sum(1 for v in values if MID_BAND[0] <= v <= MID_BAND[1])
    return mid_count / len(values)


def compute_histograms(presets: list[dict]) -> dict[str, dict]:
    """Compute per-dimension stats dict."""
    result = {}
    for dim in DIMENSIONS:
        vals = extract_dna_values(presets, dim)
        low_count  = sum(1 for v in vals if v < LOW_BAND[1])
        high_count = sum(1 for v in vals if v > HIGH_BAND[0])
        mid_count  = sum(1 for v in vals if MID_BAND[0] <= v <= MID_BAND[1])
        total = len(vals)
        result[dim] = {
            "total":      total,
            "low_count":  low_count,
            "high_count": high_count,
            "mid_count":  mid_count,
            "compression": compression_score(vals),
            "mean":   statistics.mean(vals) if vals else 0.5,
            "stdev":  statistics.stdev(vals) if len(vals) > 1 else 0.0,
        }
    return result


def pick_most_compressed(histograms: dict[str, dict], top_n: int = 3) -> list[str]:
    """Return the top_n most compressed dimensions by compression score."""
    ranked = sorted(DIMENSIONS, key=lambda d: histograms[d]["compression"], reverse=True)
    return ranked[:top_n]


def make_preset_name(zone_key: str, engine_short: str, suffix_pool: list[str]) -> str:
    """Generate a 2-word evocative preset name for the given zone."""
    vocab = NAME_VOCAB.get(zone_key, ["Extreme"])
    word1 = random.choice(vocab)
    word2 = random.choice(suffix_pool)
    # avoid dupe words
    attempts = 0
    while word2 == word1 and attempts < 10:
        word2 = random.choice(suffix_pool)
        attempts += 1
    return f"{word1} {word2}"


def make_dna(dimension: str, polarity: str) -> dict[str, float]:
    """Generate a DNA dict with the target dimension at the extreme."""
    dna: dict[str, float] = {}
    for dim in DIMENSIONS:
        if dim == dimension:
            if polarity == "high":
                dna[dim] = round(random.uniform(HIGH_BAND[0], HIGH_BAND[1]), 2)
            else:
                dna[dim] = round(random.uniform(LOW_BAND[0], LOW_BAND[1]), 2)
        else:
            # moderate value for non-target dims — slightly biased toward center
            dna[dim] = round(random.uniform(0.25, 0.75), 2)
    return dna


def make_description(zone_key: str, engine_short: str, name: str) -> str:
    """Short evocative description."""
    dim, polarity = zone_key.rsplit("_", 1)
    pol_word = "extreme high" if polarity == "high" else "extreme low"
    return f"{engine_short} at {pol_word} {dim}. Auto-generated extreme zone filler."


def make_tags(zone_key: str) -> list[str]:
    """Return 3-4 tags for the preset."""
    base = TAGS_VOCAB.get(zone_key, ["extreme"])
    return random.sample(base, min(3, len(base))) + ["extreme-zone"]


def make_preset(
    name: str,
    mood: str,
    engine_short: str,
    zone_key: str,
    dimension: str,
    polarity: str,
) -> dict:
    """Assemble a complete .xometa dict."""
    engine_id = ENGINE_ID_MAP.get(engine_short, engine_short.capitalize())
    dna = make_dna(dimension, polarity)
    return {
        "schema_version": 1,
        "name": name,
        "mood": mood,
        "engines": [engine_id],
        "author": "XO_OX",
        "version": "1.0.0",
        "description": make_description(zone_key, engine_short, name),
        "tags": make_tags(zone_key),
        "macroLabels": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
        "couplingIntensity": "None",
        "tempo": None,
        "created": str(date.today()),
        "legacy": {
            "sourceInstrument": None,
            "sourceCategory": None,
            "sourcePresetName": None,
        },
        "parameters": {},
        "coupling": None,
        "sequencer": None,
        "dna": dna,
    }


def sanitize_filename(name: str) -> str:
    """Convert preset name to a safe filename stem."""
    safe = "".join(c if c.isalnum() or c in (" ", "-", "_") else "" for c in name)
    return safe.strip().replace(" ", "_")


# ---------------------------------------------------------------------------
# Core generation logic
# ---------------------------------------------------------------------------

def generate_for_zone(
    zone_key: str,
    count: int,
    output_dir: Path,
    existing_names: set[str],
    dry_run: bool,
) -> list[dict]:
    """Generate `count` presets per engine for the given zone_key."""
    dim, polarity = zone_key.rsplit("_", 1)
    engines = ENGINE_EXTREMES.get(zone_key, [])
    mood = ZONE_MOOD.get(zone_key, "Flux")
    mood_dir = output_dir / mood

    generated = []
    suffix_pool = SUFFIXES.copy()

    for engine_short in engines:
        produced = 0
        attempts = 0
        while produced < count and attempts < count * 20:
            attempts += 1
            name = make_preset_name(zone_key, engine_short, suffix_pool)
            if name in existing_names:
                continue
            preset = make_preset(name, mood, engine_short, zone_key, dim, polarity)
            existing_names.add(name)
            generated.append(preset)

            if not dry_run:
                mood_dir.mkdir(parents=True, exist_ok=True)
                out_path = mood_dir / f"{sanitize_filename(name)}.xometa"
                # Avoid overwriting — add numeric suffix if needed
                collision = 0
                while out_path.exists():
                    collision += 1
                    out_path = mood_dir / f"{sanitize_filename(name)}_{collision}.xometa"
                with open(out_path, "w", encoding="utf-8") as f:
                    json.dump(preset, f, indent=2)
                    f.write("\n")

            produced += 1

    return generated


# ---------------------------------------------------------------------------
# Reporting
# ---------------------------------------------------------------------------

def print_histogram_report(histograms: dict[str, dict]) -> None:
    print("\n=== Fleet DNA Histogram ===")
    header = f"{'Dimension':<14} {'Total':>6} {'Low(0-0.15)':>11} {'Mid(0.25-0.75)':>14} {'High(0.85-1.0)':>14} {'Compression':>12}"
    print(header)
    print("-" * len(header))
    for dim in DIMENSIONS:
        h = histograms[dim]
        total = h["total"] or 1
        print(
            f"{dim:<14} {h['total']:>6} "
            f"{h['low_count']:>6} ({100*h['low_count']//total:>2}%)"
            f"  {h['mid_count']:>6} ({100*h['mid_count']//total:>2}%)"
            f"   {h['high_count']:>6} ({100*h['high_count']//total:>2}%)"
            f"   {h['compression']*100:>8.1f}%"
        )
    print()


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="XO_OX DNA Extreme Zone Meta-Filler — auto-generates extreme presets for compressed DNA dimensions.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument(
        "--presets-dir",
        type=Path,
        default=None,
        help="Root directory to scan for .xometa files (default: auto-detect Presets/XOceanus/ relative to this script).",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=None,
        help="Root output directory for generated presets (default: same as --presets-dir).",
    )
    parser.add_argument(
        "--dimensions",
        default="auto",
        help=(
            'Comma-separated list of dimensions to fill, or "auto" to select the 3 most compressed. '
            "Valid: brightness, warmth, density, movement, space, aggression (default: auto)."
        ),
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print what would be generated without writing files.",
    )
    parser.add_argument(
        "--count",
        type=int,
        default=2,
        help="Number of presets to generate per engine per extreme per dimension (default: 2).",
    )
    parser.add_argument(
        "--seed",
        type=int,
        default=None,
        help="Random seed for reproducibility.",
    )
    return parser.parse_args()


def resolve_presets_dir(explicit) -> Path:
    """Find the Presets/XOceanus directory relative to this script."""
    if explicit is not None:
        return explicit.expanduser().resolve()
    # Walk up from script location
    script_dir = Path(__file__).parent
    candidates = [
        script_dir.parent / "Presets" / "XOceanus",
        script_dir / "Presets" / "XOceanus",
    ]
    for c in candidates:
        if c.exists():
            return c
    # Fallback: return first candidate and let caller handle missing
    return candidates[0]


def main() -> int:
    args = parse_args()

    if args.seed is not None:
        random.seed(args.seed)

    presets_dir = resolve_presets_dir(args.presets_dir)
    output_dir  = args.output_dir.expanduser().resolve() if args.output_dir else presets_dir

    if not presets_dir.exists():
        print(f"ERROR: Presets directory not found: {presets_dir}", file=sys.stderr)
        return 1

    print(f"Scanning: {presets_dir}")
    presets = load_all_xometa(presets_dir)
    print(f"Loaded {len(presets)} presets.")

    histograms = compute_histograms(presets)
    print_histogram_report(histograms)

    # Resolve dimensions
    if args.dimensions.strip().lower() == "auto":
        target_dims = pick_most_compressed(histograms, top_n=3)
        print(f"Auto-selected 3 most compressed dimensions: {', '.join(target_dims)}")
    else:
        raw = [d.strip().lower() for d in args.dimensions.split(",")]
        invalid = [d for d in raw if d not in DIMENSIONS]
        if invalid:
            print(f"ERROR: Unknown dimension(s): {', '.join(invalid)}", file=sys.stderr)
            print(f"Valid: {', '.join(DIMENSIONS)}", file=sys.stderr)
            return 1
        target_dims = raw
        print(f"Target dimensions: {', '.join(target_dims)}")

    # Collect existing preset names to avoid collisions
    existing_names: set[str] = {p.get("name", "") for p in presets}

    # Generate
    total_generated = 0
    for dim in target_dims:
        for polarity in ("low", "high"):
            zone_key = f"{dim}_{polarity}"
            engines = ENGINE_EXTREMES.get(zone_key, [])
            mood = ZONE_MOOD.get(zone_key, "Flux")
            expected = len(engines) * args.count

            print(f"\n--- {zone_key.upper()} → mood:{mood} | engines:{len(engines)} | target:{expected} presets ---")

            results = generate_for_zone(
                zone_key=zone_key,
                count=args.count,
                output_dir=output_dir,
                existing_names=existing_names,
                dry_run=args.dry_run,
            )

            for r in results:
                action = "[DRY-RUN]" if args.dry_run else "[WRITE]"
                dna_val = r["dna"].get(dim, "?")
                print(f"  {action} {r['name']} ({r['engines'][0]}) {dim}={dna_val:.2f}")

            total_generated += len(results)

    # Summary
    print(f"\n{'=' * 50}")
    if args.dry_run:
        print(f"DRY RUN — would generate {total_generated} presets.")
    else:
        print(f"Generated {total_generated} presets → {output_dir}")
    print(f"{'=' * 50}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
