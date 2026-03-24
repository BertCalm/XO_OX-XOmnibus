#!/usr/bin/env python3
"""Generate .xometa coupling preset stubs for OHM, ORPHICA, OTTONI, and OLE.

Covers 6 intra-Constellation pairs (OHM/ORPHICA, OHM/OTTONI, OHM/OLE,
ORPHICA/OTTONI, ORPHICA/OLE, OTTONI/OLE) — 4 presets each — plus 11 external
legacy pairs (3 per engine, with OLE's OHM slot omitted since it is an intra pair).

OBBLIGATO is covered separately by xpn_constellation_coupling_pack.py.

External pairs:
  OHM   + ORACLE, OCEANIC, OBLONG   (9 presets)
  ORPHICA + OPAL, ORACLE, OCEANIC   (9 presets)
  OTTONI  + ONSET, OBLONG, OUROBOROS (9 presets)
  OLE   + ONSET, ORACLE             (6 presets — OHM is covered by intra)

Total: 24 intra + 33 external = 57 presets.

Usage:
    python3 Tools/xpn_ohm_orphica_ottoni_ole_coupling_pack.py
    python3 Tools/xpn_ohm_orphica_ottoni_ole_coupling_pack.py --dry-run
    python3 Tools/xpn_ohm_orphica_ottoni_ole_coupling_pack.py --seed 42
    python3 Tools/xpn_ohm_orphica_ottoni_ole_coupling_pack.py --output-dir /tmp/test
"""

import argparse
import json
import os
import random
from pathlib import Path

# ---------------------------------------------------------------------------
# Engine DNA baselines
# ---------------------------------------------------------------------------

ENGINE_DNA = {
    "OHM":       {"brightness": 0.45, "warmth": 0.75, "movement": 0.55,
                  "density": 0.60, "space": 0.70, "aggression": 0.30},
    "ORPHICA":   {"brightness": 0.80, "warmth": 0.50, "movement": 0.70,
                  "density": 0.45, "space": 0.75, "aggression": 0.25},
    "OTTONI":    {"brightness": 0.60, "warmth": 0.60, "movement": 0.50,
                  "density": 0.70, "space": 0.55, "aggression": 0.60},
    "OLE":       {"brightness": 0.65, "warmth": 0.70, "movement": 0.75,
                  "density": 0.60, "space": 0.60, "aggression": 0.55},
    "ORACLE":    {"brightness": 0.50, "warmth": 0.40, "movement": 0.60,
                  "density": 0.70, "space": 0.70, "aggression": 0.40},
    "OCEANIC":   {"brightness": 0.65, "warmth": 0.55, "movement": 0.50,
                  "density": 0.55, "space": 0.75, "aggression": 0.30},
    "OBLONG":    {"brightness": 0.60, "warmth": 0.65, "movement": 0.50,
                  "density": 0.65, "space": 0.55, "aggression": 0.50},
    "OPAL":      {"brightness": 0.70, "warmth": 0.50, "movement": 0.75,
                  "density": 0.45, "space": 0.80, "aggression": 0.20},
    "ONSET":     {"brightness": 0.55, "warmth": 0.50, "movement": 0.80,
                  "density": 0.75, "space": 0.50, "aggression": 0.70},
    "OUROBOROS": {"brightness": 0.50, "warmth": 0.40, "movement": 0.85,
                  "density": 0.75, "space": 0.50, "aggression": 0.80},
}

DNA_KEYS = ["brightness", "warmth", "movement", "density", "space", "aggression"]

# ---------------------------------------------------------------------------
# Engine parameter prefix map (for stubs)
# ---------------------------------------------------------------------------

ENGINE_PREFIX = {
    "OHM":       "ohm_",
    "ORPHICA":   "orph_",
    "OTTONI":    "otto_",
    "OLE":       "ole_",
    "ORACLE":    "oracle_",
    "OCEANIC":   "ocean_",
    "OBLONG":    "bob_",
    "OPAL":      "opal_",
    "ONSET":     "perc_",
    "OUROBOROS": "ouro_",
}

# ---------------------------------------------------------------------------
# Intra-Constellation pair definitions
# 6 pairs × 4 preset names each = 24 intra presets
# ---------------------------------------------------------------------------

INTRA_PAIRS: list[tuple[str, str, list[str]]] = [
    (
        "OHM", "ORPHICA",
        ["Commune Harp", "Ohm Pluck", "Sage Microsound", "Meddling Strings"],
    ),
    (
        "OHM", "OTTONI",
        ["Brass Commune", "Hippy Fanfare", "Patina Sage", "Green Horn"],
    ),
    (
        "OHM", "OLE",
        ["Clave Commune", "Drama Sage", "Afro-Hippy", "Ritual Ole"],
    ),
    (
        "ORPHICA", "OTTONI",
        ["Harp Fanfare", "Microsound Brass", "Siren Patina", "Crystal Horn"],
    ),
    (
        "ORPHICA", "OLE",
        ["Harp Clave", "Microsound Rhythm", "Siren Drama", "Crystal Ritual"],
    ),
    (
        "OTTONI", "OLE",
        ["Brass Drama", "Patina Clave", "Horn Ritual", "Fanfare Ole"],
    ),
]

# ---------------------------------------------------------------------------
# External pairs
# 3 external pairs per primary engine × 3 presets each = 27 external presets
# OLE's third external pair (OLE+OHM) is already covered by intra pairs,
# so its third slot is omitted here — total remains 27 as spec counts it.
# ---------------------------------------------------------------------------

EXTERNAL_PAIRS: list[tuple[str, str]] = [
    # OHM externals
    ("OHM",     "ORACLE"),
    ("OHM",     "OCEANIC"),
    ("OHM",     "OBLONG"),
    # ORPHICA externals
    ("ORPHICA", "OPAL"),
    ("ORPHICA", "ORACLE"),
    ("ORPHICA", "OCEANIC"),
    # OTTONI externals
    ("OTTONI",  "ONSET"),
    ("OTTONI",  "OBLONG"),
    ("OTTONI",  "OUROBOROS"),
    # OLE externals — OLE+OHM is already covered by intra pairs above
    ("OLE",     "ONSET"),
    ("OLE",     "ORACLE"),
    # OLE+OHM is intra; omitted here so total external = 9 pairs × 3 = 27
]

EXTERNAL_PRESETS_PER_PAIR = 3

# ---------------------------------------------------------------------------
# Coupling types and macro label pools
# ---------------------------------------------------------------------------

COUPLING_TYPES = [
    "Env->Filter", "Amp->Filter", "Env->Morph", "LFO->Pitch",
    "Env->Level", "Mod->Cutoff", "Gate->Env", "Pitch->Mod",
]

MACRO_LABELS_POOL = [
    ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    ["WARMTH", "BLOOM", "ENTANGLE", "AIR"],
    ["TEXTURE", "PULSE", "BRIDGE", "DEPTH"],
    ["VOICE", "SWELL", "WEAVE", "SPREAD"],
    ["COMMUNE", "DRAMA", "RITUAL", "FLOW"],
    ["BRASS", "HARP", "GROVE", "BLOOM"],
]

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def blend_dna(a: dict, b: dict) -> dict:
    return {k: round((a.get(k, 0.5) + b.get(k, 0.5)) / 2, 3) for k in DNA_KEYS}


def jitter(val: float, scale: float = 0.08, rng: random.Random = None) -> float:
    if rng is None:
        rng = random
    return round(max(0.0, min(1.0, val + rng.uniform(-scale, scale))), 3)


def jitter_dna(dna: dict, rng: random.Random) -> dict:
    return {k: jitter(v, rng=rng) for k, v in dna.items()}


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
    prefix_a = ENGINE_PREFIX.get(engine_a, engine_a.lower() + "_")
    prefix_b = ENGINE_PREFIX.get(engine_b, engine_b.lower() + "_")
    tags = [
        "coupling",
        "constellation",
        engine_a.lower(),
        engine_b.lower(),
    ]
    description = (
        f"{engine_a} ({prefix_a.rstrip('_')}) and {engine_b} ({prefix_b.rstrip('_')}) "
        f"coupled via {coupling_type}. "
        f"Blended Entangled DNA — stub seeded for expansion."
    )
    return {
        "schema_version": 1,
        "name": name,
        "mood": "Entangled",
        "engines": [engine_a, engine_b],
        "author": "XO_OX",
        "version": "1.0.0",
        "description": description,
        "tags": tags,
        "macroLabels": macro_labels,
        "couplingIntensity": "Deep" if coupling_amount >= 0.70 else "Moderate",
        "tempo": None,
        "created": created,
        "parameters": {
            engine_a: {f"{prefix_a}macro1": 0.5, f"{prefix_a}macro2": 0.5,
                       f"{prefix_a}macro3": 0.5, f"{prefix_a}macro4": 0.5},
            engine_b: {f"{prefix_b}macro1": 0.5, f"{prefix_b}macro2": 0.5,
                       f"{prefix_b}macro3": 0.5, f"{prefix_b}macro4": 0.5},
        },
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


def external_name(engine_a: str, engine_b: str, index: int, rng: random.Random) -> str:
    templates = [
        "{a} {b} Bridge",
        "{a} Meets {b}",
        "{b} Through {a}",
        "{a} {b} Weave",
        "{b} {a} Surge",
        "{a} {b} Pulse",
    ]
    # Use a deterministic selection based on index so names don't collide
    template = templates[index % len(templates)]
    return template.format(a=engine_a.capitalize(), b=engine_b.capitalize())


# ---------------------------------------------------------------------------
# Preset generation
# ---------------------------------------------------------------------------

def generate_stubs(rng: random.Random) -> list[dict]:
    stubs = []

    # --- Intra-Constellation: 6 pairs × 4 presets = 24 ---
    for engine_a, engine_b, names in INTRA_PAIRS:
        dna_a = ENGINE_DNA[engine_a]
        dna_b = ENGINE_DNA[engine_b]
        base_dna = blend_dna(dna_a, dna_b)

        for name in names:
            jittered = jitter_dna(base_dna, rng)
            coupling_type = rng.choice(COUPLING_TYPES)
            coupling_amount = round(rng.uniform(0.45, 0.85), 3)
            macros = rng.choice(MACRO_LABELS_POOL)
            stubs.append(make_stub(name, engine_a, engine_b, jittered,
                                   coupling_type, coupling_amount, macros))

    # --- External pairs: up to 12 pairs × 3 presets ---
    # The spec lists OLE+OHM as one external pair even though OHM+OLE is intra.
    # We include it to hit the 27 external preset count.
    seen_external_pairs: set[frozenset] = set()
    external_count = 0

    for engine_a, engine_b in EXTERNAL_PAIRS:
        pair_key = frozenset({engine_a, engine_b})
        # Skip if this pair was already handled as an intra pair
        intra_keys = {frozenset({a, b}) for a, b, _ in INTRA_PAIRS}
        if pair_key in intra_keys:
            # OLE+OHM is intra, but spec lists it as OLE's 3rd external slot.
            # Generate 3 additional presets with distinct context names.
            pass  # fall through — generate anyway with external naming

        dna_a = ENGINE_DNA[engine_a]
        dna_b = ENGINE_DNA[engine_b]
        base_dna = blend_dna(dna_a, dna_b)

        for i in range(EXTERNAL_PRESETS_PER_PAIR):
            name = external_name(engine_a, engine_b, external_count + i, rng)
            jittered = jitter_dna(base_dna, rng)
            coupling_type = rng.choice(COUPLING_TYPES)
            coupling_amount = round(rng.uniform(0.45, 0.85), 3)
            macros = rng.choice(MACRO_LABELS_POOL)
            stubs.append(make_stub(name, engine_a, engine_b, jittered,
                                   coupling_type, coupling_amount, macros))

        external_count += EXTERNAL_PRESETS_PER_PAIR

    return stubs


# ---------------------------------------------------------------------------
# File I/O
# ---------------------------------------------------------------------------

def write_stub(stub: dict, output_dir: Path) -> Path:
    output_dir.mkdir(parents=True, exist_ok=True)
    filename = stub["name"].replace("/", "-") + ".xometa"
    filepath = output_dir / filename
    with open(filepath, "w", encoding="utf-8") as fh:
        json.dump(stub, fh, indent=2)
        fh.write("\n")
    return filepath


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main() -> None:
    repo_root = Path(__file__).resolve().parent.parent
    default_output = repo_root / "Presets" / "XOlokun" / "Entangled"

    parser = argparse.ArgumentParser(
        description=(
            "Generate OHM/ORPHICA/OTTONI/OLE intra-Constellation and external "
            "coupling .xometa stubs (51 total: 24 intra + 27 external)."
        )
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
        help="Print stubs as JSON to stdout without writing files.",
    )
    parser.add_argument(
        "--seed",
        type=int,
        default=None,
        help="Random seed for reproducible output.",
    )
    args = parser.parse_args()

    rng = random.Random(args.seed)
    stubs = generate_stubs(rng)

    intra_total = sum(len(names) for _, _, names in INTRA_PAIRS)
    external_total = len(EXTERNAL_PAIRS) * EXTERNAL_PRESETS_PER_PAIR
    total = len(stubs)

    print(f"OHM / ORPHICA / OTTONI / OLE Coupling Pack — {total} stubs")
    print(f"  Intra-Constellation (6 pairs × 4) : {intra_total}")
    print(f"  External pairs ({len(EXTERNAL_PAIRS)} × {EXTERNAL_PRESETS_PER_PAIR})          : {external_total}")
    print()

    print("Intra-Constellation pairs:")
    for engine_a, engine_b, names in INTRA_PAIRS:
        print(f"  {engine_a} + {engine_b}: {len(names)} presets")
    print()

    print("External pairs:")
    for engine_a, engine_b in EXTERNAL_PAIRS:
        print(f"  {engine_a} + {engine_b}: {EXTERNAL_PRESETS_PER_PAIR} presets")
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
