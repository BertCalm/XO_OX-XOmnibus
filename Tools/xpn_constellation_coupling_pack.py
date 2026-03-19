#!/usr/bin/env python3
"""Generate .xometa coupling preset stubs for the 5 Constellation Fast Track engines.

Covers all 10 intra-Constellation pairs (5 choose 2) plus the top 5 legacy-engine
pairings (one per Constellation engine, chosen by smallest Euclidean DNA distance).

Usage:
    python3 Tools/xpn_constellation_coupling_pack.py
    python3 Tools/xpn_constellation_coupling_pack.py --dry-run
    python3 Tools/xpn_constellation_coupling_pack.py --count 5 --output-dir /tmp/test
"""

import argparse
import json
import math
import random
from pathlib import Path

# ---------------------------------------------------------------------------
# Engine DNA baselines
# ---------------------------------------------------------------------------

CONSTELLATION_ENGINES = {
    "OHM": {
        "label": "Hippy Dad jam",
        "dna": {"warmth": 0.75, "brightness": 0.45, "movement": 0.55,
                "density": 0.60, "space": 0.70, "aggression": 0.30},
    },
    "ORPHICA": {
        "label": "Microsound harp",
        "dna": {"brightness": 0.80, "warmth": 0.50, "movement": 0.70,
                "density": 0.45, "space": 0.75, "aggression": 0.25},
    },
    "OBBLIGATO": {
        "label": "Dual wind",
        "dna": {"warmth": 0.65, "brightness": 0.55, "movement": 0.60,
                "density": 0.65, "space": 0.60, "aggression": 0.45},
    },
    "OTTONI": {
        "label": "Triple brass",
        "dna": {"brightness": 0.60, "warmth": 0.60, "movement": 0.50,
                "density": 0.70, "space": 0.55, "aggression": 0.60},
    },
    "OLE": {
        "label": "Afro-Latin trio",
        "dna": {"brightness": 0.65, "warmth": 0.70, "movement": 0.75,
                "density": 0.60, "space": 0.60, "aggression": 0.55},
    },
}

# Legacy engines with DNA baselines (representative sample used for distance calc)
LEGACY_ENGINES = {
    "OddfeliX": {
        "dna": {"brightness": 0.55, "warmth": 0.60, "movement": 0.50,
                "density": 0.55, "space": 0.50, "aggression": 0.45},
    },
    "Overdub": {
        "dna": {"brightness": 0.35, "warmth": 0.75, "movement": 0.60,
                "density": 0.55, "space": 0.70, "aggression": 0.25},
    },
    "Odyssey": {
        "dna": {"brightness": 0.50, "warmth": 0.55, "movement": 0.65,
                "density": 0.50, "space": 0.65, "aggression": 0.30},
    },
    "Overworld": {
        "dna": {"brightness": 0.70, "warmth": 0.40, "movement": 0.65,
                "density": 0.60, "space": 0.45, "aggression": 0.55},
    },
    "OPAL": {
        "dna": {"brightness": 0.75, "warmth": 0.50, "movement": 0.70,
                "density": 0.45, "space": 0.80, "aggression": 0.20},
    },
    "ONSET": {
        "dna": {"brightness": 0.50, "warmth": 0.55, "movement": 0.70,
                "density": 0.80, "space": 0.35, "aggression": 0.70},
    },
    "OVERBITE": {
        "dna": {"brightness": 0.55, "warmth": 0.65, "movement": 0.55,
                "density": 0.70, "space": 0.50, "aggression": 0.65},
    },
    "OVERDRIVE": {
        "dna": {"brightness": 0.65, "warmth": 0.50, "movement": 0.60,
                "density": 0.65, "space": 0.45, "aggression": 0.70},
    },
}

DNA_KEYS = ["brightness", "warmth", "movement", "density", "space", "aggression"]

# ---------------------------------------------------------------------------
# Evocative name banks per engine pair (order-independent lookup)
# ---------------------------------------------------------------------------

# Intra-constellation pair names keyed as frozenset
PAIR_NAMES: dict[frozenset, list[str]] = {
    frozenset({"OHM", "ORPHICA"}): [
        "Drone Harp Reverie", "Overtone Pluck", "Still Waters Shimmer",
        "Communal Harp", "Harmonic Campfire", "Siphon Bloom",
    ],
    frozenset({"OHM", "OBBLIGATO"}): [
        "Wind Commune", "Woodsmoke Oboe", "Pastoral Drift",
        "Open Air Choir", "Reed Meditation", "Dune Breath",
    ],
    frozenset({"OHM", "OTTONI"}): [
        "Brass Gathering", "Sun Salute Horn", "Copper Resonance",
        "Warm Fanfare", "Village Bell", "Bronze Haze",
    ],
    frozenset({"OHM", "OLE"}): [
        "Dance Circle Dawn", "Clave Commune", "Afro Zen",
        "Drum Incense", "Rhythmic Stillness", "Earth Groove",
    ],
    frozenset({"ORPHICA", "OBBLIGATO"}): [
        "Microwind Harp", "Spectral Reed", "Glass Oboe",
        "Filament Breath", "Grain Swell", "Ether Reed",
    ],
    frozenset({"ORPHICA", "OTTONI"}): [
        "Brass Shimmer", "Grain Trumpet", "Metallic Harp",
        "Spectral Fanfare", "Copper Filament", "Resonant Fog",
    ],
    frozenset({"ORPHICA", "OLE"}): [
        "Afro Shimmer", "Clave Grain", "Kora Micro",
        "Spectral Conga", "Harp Carnival", "Filament Fiesta",
    ],
    frozenset({"OBBLIGATO", "OTTONI"}): [
        "Brass Duet", "Wind Brass Drift", "Coupled Winds",
        "Bond Fanfare", "Oboe Trumpet", "Brass Wind Circle",
    ],
    frozenset({"OBBLIGATO", "OLE"}): [
        "Wind Drama", "Oboe Carnival", "Reed Fiesta",
        "Dance Breath", "Clave Oboe", "Drama Wind",
    ],
    frozenset({"OTTONI", "OLE"}): [
        "Brass Drama", "Trumpet Fiesta", "Grow Carnival",
        "Latin Brass", "Conga Horn", "Trombone Clave",
    ],
}

# Legacy pair name templates — filled in with engine names at runtime
LEGACY_NAME_TEMPLATES = [
    "{const} {legacy} Bridge",
    "{const} Meets {legacy}",
    "{legacy} Through {const}",
    "{const} Pulse {legacy}",
    "{legacy} Constellation",
    "{const} Legacy Weave",
]

# Coupling types used in stubs
COUPLING_TYPES = [
    "Env->Filter", "Amp->Filter", "Env->Morph", "LFO->Pitch",
    "Env->Level", "Mod->Cutoff", "Gate->Env", "Pitch->Mod",
]

# Macro label sets
MACRO_LABELS_POOL = [
    ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    ["WARMTH", "BLOOM", "ENTANGLE", "AIR"],
    ["TEXTURE", "PULSE", "BRIDGE", "DEPTH"],
    ["VOICE", "SWELL", "WEAVE", "SPREAD"],
]

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def dna_distance(a: dict, b: dict) -> float:
    return math.sqrt(sum((a.get(k, 0.5) - b.get(k, 0.5)) ** 2 for k in DNA_KEYS))


def blend_dna(a: dict, b: dict) -> dict:
    return {k: round((a.get(k, 0.5) + b.get(k, 0.5)) / 2, 3) for k in DNA_KEYS}


def jitter(val: float, scale: float = 0.08, rng: random.Random = None) -> float:
    if rng is None:
        rng = random
    return round(max(0.0, min(1.0, val + rng.uniform(-scale, scale))), 3)


def jitter_dna(dna: dict, rng: random.Random) -> dict:
    return {k: jitter(v, rng=rng) for k, v in dna.items()}


def pick_names(pair_key: frozenset, count: int, rng: random.Random) -> list[str]:
    pool = PAIR_NAMES.get(pair_key, [])
    if len(pool) >= count:
        return rng.sample(pool, count)
    # If not enough, supplement with generic names
    names = list(pool)
    engines = sorted(list(pair_key))
    i = 0
    while len(names) < count:
        names.append(f"{engines[0]} {engines[1]} #{i+1}")
        i += 1
    return names[:count]


def legacy_names(const_engine: str, legacy_engine: str, count: int, rng: random.Random) -> list[str]:
    templates = rng.sample(LEGACY_NAME_TEMPLATES, min(count, len(LEGACY_NAME_TEMPLATES)))
    names = [t.format(const=const_engine.capitalize(), legacy=legacy_engine) for t in templates]
    while len(names) < count:
        names.append(f"{const_engine} {legacy_engine} Coupling {len(names)+1}")
    return names[:count]


def make_stub(
    name: str,
    engine_a: str,
    engine_b: str,
    blended_dna: dict,
    coupling_type: str,
    coupling_amount: float,
    macro_labels: list[str],
    created: str = "2026-03-16",
) -> dict:
    return {
        "schema_version": 1,
        "name": name,
        "mood": "Entangled",
        "engines": [engine_a, engine_b],
        "author": "XO_OX",
        "version": "1.0.0",
        "description": (
            f"{engine_a} and {engine_b} coupled via {coupling_type}. "
            f"Blended Constellation DNA — stub seeded for expansion."
        ),
        "tags": ["coupling", "constellation", engine_a.lower(), engine_b.lower()],
        "macroLabels": macro_labels,
        "couplingIntensity": "Deep" if coupling_amount >= 0.7 else "Moderate",
        "tempo": None,
        "created": created,
        "parameters": {},
        "coupling": {
            "pairs": [
                {
                    "engineA": engine_a,
                    "engineB": engine_b,
                    "type": coupling_type,
                    "amount": round(coupling_amount, 3),
                }
            ]
        },
        "sequencer": None,
        "dna": blended_dna,
    }


# ---------------------------------------------------------------------------
# Pair generation
# ---------------------------------------------------------------------------

def build_intra_constellation_pairs() -> list[tuple[str, str]]:
    names = list(CONSTELLATION_ENGINES.keys())
    pairs = []
    for i in range(len(names)):
        for j in range(i + 1, len(names)):
            pairs.append((names[i], names[j]))
    return pairs


def best_legacy_match(const_engine: str) -> str:
    const_dna = CONSTELLATION_ENGINES[const_engine]["dna"]
    best = min(LEGACY_ENGINES.items(), key=lambda kv: dna_distance(const_dna, kv[1]["dna"]))
    return best[0]


def build_legacy_pairs() -> list[tuple[str, str]]:
    """One best legacy match per Constellation engine (5 pairs)."""
    pairs = []
    seen_legacy: set[str] = set()
    for const_engine in CONSTELLATION_ENGINES:
        legacy = best_legacy_match(const_engine)
        # If already claimed, pick next closest not yet used
        if legacy in seen_legacy:
            ranked = sorted(
                LEGACY_ENGINES.items(),
                key=lambda kv: dna_distance(CONSTELLATION_ENGINES[const_engine]["dna"], kv[1]["dna"]),
            )
            for name, _ in ranked:
                if name not in seen_legacy:
                    legacy = name
                    break
        seen_legacy.add(legacy)
        pairs.append((const_engine, legacy))
    return pairs


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def generate_stubs(count: int, rng: random.Random) -> list[dict]:
    stubs = []

    intra_pairs = build_intra_constellation_pairs()
    legacy_pairs = build_legacy_pairs()

    for engine_a, engine_b in intra_pairs:
        pair_key = frozenset({engine_a, engine_b})
        dna_a = CONSTELLATION_ENGINES[engine_a]["dna"]
        dna_b = CONSTELLATION_ENGINES[engine_b]["dna"]
        base_dna = blend_dna(dna_a, dna_b)
        names = pick_names(pair_key, count, rng)

        for i, name in enumerate(names):
            jittered = jitter_dna(base_dna, rng)
            coupling_type = rng.choice(COUPLING_TYPES)
            coupling_amount = round(rng.uniform(0.45, 0.85), 3)
            macros = rng.choice(MACRO_LABELS_POOL)
            stubs.append(make_stub(name, engine_a, engine_b, jittered,
                                   coupling_type, coupling_amount, macros))

    for const_engine, legacy_engine in legacy_pairs:
        const_dna = CONSTELLATION_ENGINES[const_engine]["dna"]
        legacy_dna = LEGACY_ENGINES[legacy_engine]["dna"]
        base_dna = blend_dna(const_dna, legacy_dna)
        names = legacy_names(const_engine, legacy_engine, count, rng)

        for name in names:
            jittered = jitter_dna(base_dna, rng)
            coupling_type = rng.choice(COUPLING_TYPES)
            coupling_amount = round(rng.uniform(0.45, 0.85), 3)
            macros = rng.choice(MACRO_LABELS_POOL)
            stubs.append(make_stub(name, const_engine, legacy_engine, jittered,
                                   coupling_type, coupling_amount, macros))

    return stubs


def write_stub(stub: dict, output_dir: Path) -> Path:
    output_dir.mkdir(parents=True, exist_ok=True)
    filename = stub["name"].replace("/", "-") + ".xometa"
    filepath = output_dir / filename
    with open(filepath, "w", encoding="utf-8") as fh:
        json.dump(stub, fh, indent=2)
        fh.write("\n")
    return filepath


def main() -> None:
    repo_root = Path(__file__).resolve().parent.parent
    default_output = repo_root / "Presets" / "XOmnibus" / "Entangled"

    parser = argparse.ArgumentParser(
        description="Generate Constellation coupling .xometa stubs for XOmnibus."
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=default_output,
        help=f"Destination directory (default: {default_output})",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print stubs as JSON without writing files.",
    )
    parser.add_argument(
        "--count",
        type=int,
        default=3,
        metavar="N",
        help="Number of presets per pair (default: 3).",
    )
    parser.add_argument(
        "--seed",
        type=int,
        default=None,
        help="Random seed for reproducible output.",
    )
    args = parser.parse_args()

    if args.count < 1:
        parser.error("--count must be >= 1")

    rng = random.Random(args.seed)
    stubs = generate_stubs(args.count, rng)

    # Summary
    intra_count = len(build_intra_constellation_pairs()) * args.count
    legacy_count = len(build_legacy_pairs()) * args.count
    total = intra_count + legacy_count

    print(f"Constellation Coupling Pack — {total} stubs")
    print(f"  Intra-Constellation pairs : {len(build_intra_constellation_pairs())} × {args.count} = {intra_count}")
    print(f"  Legacy pairs              : {len(build_legacy_pairs())} × {args.count} = {legacy_count}")

    # Print legacy assignments
    print("\nLegacy partner assignments (closest DNA distance):")
    for const_engine, legacy_engine in build_legacy_pairs():
        dist = dna_distance(
            CONSTELLATION_ENGINES[const_engine]["dna"],
            LEGACY_ENGINES[legacy_engine]["dna"],
        )
        print(f"  {const_engine:12s} → {legacy_engine:12s}  (dist={dist:.3f})")

    print()

    if args.dry_run:
        for stub in stubs:
            print(f"--- {stub['name']} ({stub['engines'][0]} × {stub['engines'][1]}) ---")
            print(json.dumps(stub, indent=2))
            print()
    else:
        written = 0
        for stub in stubs:
            path = write_stub(stub, args.output_dir)
            print(f"  wrote: {path.name}")
            written += 1
        print(f"\nDone — {written} files written to {args.output_dir}")


if __name__ == "__main__":
    main()
