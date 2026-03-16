#!/usr/bin/env python3
"""xpn_orca_octopus_ombre_trio_pack.py

Generate Entangled presets for the ORCA × OCTOPUS × OMBRE deep-ocean trio.

Coverage:
  - 6  three-way marquee presets  (ORCA + OCTOPUS + OMBRE)
  - 12 ORCA   × OCTOPUS pairs
  - 12 ORCA   × OMBRE   pairs
  - 12 OCTOPUS × OMBRE  pairs
  - 12 ORCA   with other engines  (ORACLE/OBSIDIAN/OUROBOROS/OPTIC/OBLIQUE/
                                   ONSET/OVERDUB/OPAL/ORGANON/ORBITAL/OSPREY/OSTERIA)
  - 12 OCTOPUS with other engines (same 12 partners)

Total: 66 presets.

Usage:
    python Tools/xpn_orca_octopus_ombre_trio_pack.py
    python Tools/xpn_orca_octopus_ombre_trio_pack.py --dry-run
    python Tools/xpn_orca_octopus_ombre_trio_pack.py --seed 77 --output-dir /tmp/test
"""

import argparse
import json
import random
from pathlib import Path
from typing import Dict, List, Optional, Tuple

# ---------------------------------------------------------------------------
# DNA baselines (canonical per task brief + CLAUDE.md engine table)
# ---------------------------------------------------------------------------

DNA: Dict[str, Dict[str, float]] = {
    # Core trio — exact values from task brief
    "ORCA":    {"brightness": 0.2, "warmth": 0.3, "movement": 0.65, "density": 0.7,  "space": 0.45, "aggression": 0.85},
    "OCTOPUS": {"brightness": 0.7, "warmth": 0.4, "movement": 0.9,  "density": 0.8,  "space": 0.5,  "aggression": 0.45},
    "OMBRE":   {"brightness": 0.4, "warmth": 0.6, "movement": 0.5,  "density": 0.55, "space": 0.7,  "aggression": 0.2},
    # Partner engines (estimated, used for blended DNA)
    "ORACLE":    {"brightness": 0.5,  "warmth": 0.45, "movement": 0.65, "density": 0.6,  "space": 0.55, "aggression": 0.35},
    "OBSIDIAN":  {"brightness": 0.3,  "warmth": 0.4,  "movement": 0.4,  "density": 0.65, "space": 0.5,  "aggression": 0.4},
    "OUROBOROS": {"brightness": 0.45, "warmth": 0.35, "movement": 0.7,  "density": 0.5,  "space": 0.3,  "aggression": 0.5},
    "OPTIC":     {"brightness": 0.8,  "warmth": 0.3,  "movement": 0.8,  "density": 0.4,  "space": 0.4,  "aggression": 0.3},
    "OBLIQUE":   {"brightness": 0.65, "warmth": 0.45, "movement": 0.7,  "density": 0.5,  "space": 0.45, "aggression": 0.45},
    "ONSET":     {"brightness": 0.5,  "warmth": 0.4,  "movement": 0.75, "density": 0.6,  "space": 0.3,  "aggression": 0.55},
    "OVERDUB":   {"brightness": 0.35, "warmth": 0.65, "movement": 0.45, "density": 0.45, "space": 0.6,  "aggression": 0.15},
    "OPAL":      {"brightness": 0.6,  "warmth": 0.5,  "movement": 0.6,  "density": 0.55, "space": 0.6,  "aggression": 0.2},
    "ORGANON":   {"brightness": 0.55, "warmth": 0.55, "movement": 0.6,  "density": 0.65, "space": 0.5,  "aggression": 0.25},
    "ORBITAL":   {"brightness": 0.6,  "warmth": 0.55, "movement": 0.55, "density": 0.5,  "space": 0.5,  "aggression": 0.3},
    "OSPREY":    {"brightness": 0.5,  "warmth": 0.55, "movement": 0.5,  "density": 0.5,  "space": 0.6,  "aggression": 0.2},
    "OSTERIA":   {"brightness": 0.45, "warmth": 0.7,  "movement": 0.4,  "density": 0.55, "space": 0.55, "aggression": 0.2},
}

# ---------------------------------------------------------------------------
# Parameter prefix map (frozen IDs from CLAUDE.md)
# ---------------------------------------------------------------------------

_PREFIX: Dict[str, str] = {
    "ORCA":      "orca_",
    "OCTOPUS":   "octo_",
    "OMBRE":     "ombre_",
    "ORACLE":    "oracle_",
    "OBSIDIAN":  "obsidian_",
    "OUROBOROS": "ouro_",
    "OPTIC":     "optic_",
    "OBLIQUE":   "oblq_",
    "ONSET":     "perc_",
    "OVERDUB":   "dub_",
    "OPAL":      "opal_",
    "ORGANON":   "organon_",
    "ORBITAL":   "orb_",
    "OSPREY":    "osprey_",
    "OSTERIA":   "osteria_",
}


def prefix(engine: str) -> str:
    return _PREFIX.get(engine, engine.lower() + "_")


# ---------------------------------------------------------------------------
# Name vocabulary
# ---------------------------------------------------------------------------

VOCAB: Dict[str, List[str]] = {
    "ORCA":    ["Hunt", "Breach", "Echo", "Surge", "Apex", "Current", "Sonar", "Pursuit",
                "Dive", "Strike", "Depth", "Tide"],
    "OCTOPUS": ["Ink", "Arm", "Signal", "Shift", "Mimic", "Flash", "Pattern", "Adapt",
                "Pulse", "Veil", "Scatter", "Glow"],
    "OMBRE":   ["Shadow", "Memory", "Gradient", "Fade", "Recall", "Dusk", "Twilight",
                "Liminal", "Drift", "Haze", "Trace", "Lull"],
}

PARTNER_WORDS: dict[str, list[str]] = {
    "ORACLE":    ["Vision", "Break", "Fracture", "Dream", "Omen", "Seer"],
    "OBSIDIAN":  ["Glass", "Void", "Edge", "Cut", "Shard", "Dark"],
    "OUROBOROS": ["Coil", "Loop", "Return", "Spiral", "Cycle", "Knot"],
    "OPTIC":     ["Beam", "Flare", "Trace", "Scan", "Pixel", "Ray"],
    "OBLIQUE":   ["Angle", "Bounce", "Prism", "Slant", "Skew", "Arc"],
    "ONSET":     ["Strike", "Hit", "Pulse", "Snap", "Crack", "Burst"],
    "OVERDUB":   ["Tape", "Dub", "Loop", "Echo", "Layer", "Ghost"],
    "OPAL":      ["Shimmer", "Grain", "Scatter", "Mist", "Veil", "Opal"],
    "ORGANON":   ["Cell", "Organ", "Bloom", "Grow", "Form", "Bind"],
    "ORBITAL":   ["Ring", "Orbit", "Pulse", "Arc", "Spin", "Curve"],
    "OSPREY":    ["Shore", "Dive", "Coast", "Wing", "Sweep", "Tide"],
    "OSTERIA":   ["Cellar", "Port", "Wood", "Smoke", "Grain", "Steep"],
}

COUPLING_TYPES: list[str] = [
    "Amp->Filter", "Env->Morph", "Audio->Wavetable", "Env->Decay",
    "Audio->FM", "Rhythm->Blend", "Env->Filter", "Amp->Morph",
    "Env->Pitch", "Amp->Drive", "Rhythm->Gate", "Audio->Reverb",
]

# Marquee triad names — 6 distinct, hand-crafted
TRIAD_NAMES: list[str] = [
    "Abyss Signal",
    "Chromatic Hunt",
    "Deep Memory",
    "Shadow Breach",
    "Predator Dream",
    "Apex Twilight",
]

# ---------------------------------------------------------------------------
# DNA helpers
# ---------------------------------------------------------------------------

_DNA_KEYS = ["brightness", "warmth", "movement", "density", "space", "aggression"]


def blend_dna(*engine_names: str, weights: list[float] | None = None) -> dict[str, float]:
    """Weighted average of DNA across named engines."""
    if weights is None:
        weights = [1.0] * len(engine_names)
    total = sum(weights)
    return {
        k: round(sum(DNA[e][k] * w for e, w in zip(engine_names, weights)) / total, 3)
        for k in _DNA_KEYS
    }


def nudge(dna: dict[str, float], seed_val: int) -> dict[str, float]:
    """Apply small deterministic per-preset variation so variants differ."""
    rng = random.Random(seed_val)
    return {k: round(min(1.0, max(0.0, dna[k] + rng.uniform(-0.06, 0.06))), 3)
            for k in _DNA_KEYS}


# ---------------------------------------------------------------------------
# Preset builder
# ---------------------------------------------------------------------------

def make_preset(
    name: str,
    desc: str,
    tags: list[str],
    engines: list[str],
    coupling_type: str,
    amount: float,
    dna: dict[str, float],
) -> dict:
    """Build a single .xometa dict.

    For 2-engine pairs: engines = [A, B], primary coupling A→B.
    For 3-engine triads: engines = [A, B, C], coupling A→B + B→C.
    """
    pairs = [
        {
            "engineA": engines[0],
            "engineB": engines[1],
            "type": coupling_type,
            "amount": round(amount, 3),
        }
    ]
    if len(engines) == 3:
        pairs.append(
            {
                "engineA": engines[1],
                "engineB": engines[2],
                "type": coupling_type,
                "amount": round(amount * 0.75, 3),
            }
        )

    intensity = "Deep" if amount >= 0.7 else "Moderate"

    return {
        "schema_version": 1,
        "name": name,
        "mood": "Entangled",
        "engines": engines,
        "author": "XO_OX Designs",
        "version": "1.0.0",
        "description": desc,
        "tags": ["entangled", "coupling"] + tags,
        "macroLabels": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
        "couplingIntensity": intensity,
        "tempo": None,
        "dna": dna,
        "parameters": {},
        "coupling": {"pairs": pairs},
        "sequencer": None,
    }


# ---------------------------------------------------------------------------
# Section builders
# ---------------------------------------------------------------------------

# 3-way marquee presets — 6 unique presets, each a distinct narrative angle
def build_trio_marquee(rng: random.Random) -> list[dict]:
    """6 unique ORCA + OCTOPUS + OMBRE three-engine marquee presets."""
    # Each entry: (engine order, coupling type, amount, desc fragment, extra tags)
    configs = [
        (["ORCA", "OCTOPUS", "OMBRE"], "Audio->FM",        0.80,
         "Apex predator envelops alien intelligence, dissolving into shadow memory.",
         ["apex", "alien", "memory", "deep-trio"]),
        (["OCTOPUS", "ORCA", "OMBRE"], "Env->Morph",       0.75,
         "Chromatophore shifts trigger hunt surges; twilight absorbs the aftermath.",
         ["chromatophore", "hunt", "twilight", "deep-trio"]),
        (["OMBRE", "ORCA", "OCTOPUS"], "Amp->Filter",      0.78,
         "Fading memory ignites predator breach; ink cloud scatters the signal.",
         ["memory", "breach", "ink", "deep-trio"]),
        (["ORCA", "OMBRE", "OCTOPUS"], "Rhythm->Blend",    0.72,
         "Echolocation pulse seeds gradient fade; arms multiply the resonance.",
         ["echolocation", "gradient", "arms", "deep-trio"]),
        (["OCTOPUS", "OMBRE", "ORCA"], "Env->Decay",       0.76,
         "Signal flash dims to dusk; apex current sweeps the residue.",
         ["signal", "dusk", "current", "deep-trio"]),
        (["OMBRE", "OCTOPUS", "ORCA"], "Env->Filter",      0.74,
         "Liminal recall bleeds into chromatic pattern; surge completes the arc.",
         ["liminal", "pattern", "surge", "deep-trio"]),
    ]
    presets = []
    for i, (eng_order, ctype, amt, desc, extra_tags) in enumerate(configs):
        seed = rng.randint(0, 99999)
        base_dna = blend_dna(*eng_order, weights=[0.4, 0.35, 0.25])
        dna = nudge(base_dna, seed)
        name = TRIAD_NAMES[i]
        tags = [e.lower() for e in eng_order] + extra_tags
        presets.append(make_preset(name, desc, tags, eng_order, ctype, round(amt, 2), dna))
    return presets


def _build_12_pair(
    engine_a: str,
    engine_b: str,
    word_a: str,
    word_b_list: list[str],
    coupling_type: str,
    base_amount: float,
    desc_frag: str,
    base_tags: list[str],
    rng: random.Random,
    vocab_a: list[str] | None = None,
) -> list[dict]:
    """Generate 12 variants for a given engine pair.

    Names cycle through vocab_a × word_b_list combinations to stay unique.
    """
    if vocab_a is None:
        vocab_a = VOCAB[engine_a]
    presets = []
    for i in range(12):
        seed = rng.randint(0, 99999)
        w1 = vocab_a[i % len(vocab_a)]
        w2 = word_b_list[i % len(word_b_list)]
        name = f"{w1} {w2}"
        amt = round(base_amount + rng.uniform(-0.07, 0.07), 2)
        amt = max(0.35, min(0.95, amt))
        base_dna = blend_dna(engine_a, engine_b, weights=[0.55, 0.45])
        dna = nudge(base_dna, seed)
        desc = (
            f"{engine_a}×{engine_b} — {desc_frag}. "
            f"{prefix(engine_a)}drives {prefix(engine_b)}filter via {coupling_type}."
        )
        tags = base_tags + [engine_a.lower(), engine_b.lower()]
        presets.append(
            make_preset(name, desc, tags, [engine_a, engine_b], coupling_type, amt, dna)
        )
    return presets


# ---------- ORCA × OCTOPUS (12) ----------
def build_orca_octopus(rng: random.Random) -> list[dict]:
    return _build_12_pair(
        engine_a="ORCA",
        engine_b="OCTOPUS",
        word_a=None,                  # uses VOCAB["ORCA"]
        word_b_list=["Ink", "Arm", "Flash", "Adapt", "Pulse", "Glow",
                     "Signal", "Veil", "Shift", "Pattern", "Mimic", "Scatter"],
        coupling_type="Audio->FM",
        base_amount=0.72,
        desc_frag="apex echolocation modulates chromatophore resonance",
        base_tags=["apex-alien", "wavetable", "chromatophore"],
        rng=rng,
    )


# ---------- ORCA × OMBRE (12) ----------
def build_orca_ombre(rng: random.Random) -> list[dict]:
    return _build_12_pair(
        engine_a="ORCA",
        engine_b="OMBRE",
        word_a=None,
        word_b_list=["Shadow", "Memory", "Fade", "Dusk", "Haze", "Lull",
                     "Recall", "Trace", "Twilight", "Gradient", "Liminal", "Drift"],
        coupling_type="Amp->Filter",
        base_amount=0.68,
        desc_frag="hunt surge bleeds into shadow memory",
        base_tags=["predator-memory", "dark", "dusk"],
        rng=rng,
    )


# ---------- OCTOPUS × OMBRE (12) ----------
def build_octopus_ombre(rng: random.Random) -> list[dict]:
    return _build_12_pair(
        engine_a="OCTOPUS",
        engine_b="OMBRE",
        word_a=None,
        word_b_list=["Shadow", "Memory", "Fade", "Dusk", "Haze", "Lull",
                     "Recall", "Trace", "Twilight", "Gradient", "Liminal", "Drift"],
        coupling_type="Env->Morph",
        base_amount=0.65,
        desc_frag="chromatic arm patterns dissolve into gradient twilight",
        base_tags=["chromatophore-shadow", "alien-memory"],
        rng=rng,
    )


# ---------- ORCA × 12 other engines ----------
_OTHER_12 = [
    "ORACLE", "OBSIDIAN", "OUROBOROS", "OPTIC", "OBLIQUE", "ONSET",
    "OVERDUB", "OPAL", "ORGANON", "ORBITAL", "OSPREY", "OSTERIA",
]

_ORCA_OTHER_CFG: dict[str, tuple[str, float, str, list[str]]] = {
    # partner: (coupling_type, base_amt, desc_frag, extra_tags)
    "ORACLE":    ("Env->Morph",       0.63, "sonar pulse illuminates oracle prophecy",      ["vision", "depth"]),
    "OBSIDIAN":  ("Env->Decay",       0.74, "apex breach cuts through obsidian void",       ["dark", "edge"]),
    "OUROBOROS": ("Audio->FM",        0.70, "hunt current feeds chaos spiral",              ["chaos", "loop"]),
    "OPTIC":     ("Amp->Drive",       0.65, "echolocation drives optic pulse beam",         ["beam", "scan"]),
    "OBLIQUE":   ("Audio->Wavetable", 0.66, "surge modulates prismatic bounce",             ["prism", "angle"]),
    "ONSET":     ("Rhythm->Blend",    0.76, "whale strike locks onto percussive grid",      ["percussion", "rhythm"]),
    "OVERDUB":   ("Env->Filter",      0.62, "sonar echo feeds tape loop ghost",             ["tape", "echo"]),
    "OPAL":      ("Amp->Filter",      0.60, "breach shimmer grains disperse",               ["grain", "shimmer"]),
    "ORGANON":   ("Audio->FM",        0.68, "apex metabolic drive excites organon cells",   ["cell", "biology"]),
    "ORBITAL":   ("Rhythm->Gate",     0.64, "hunt pulse gates orbital ring sequence",       ["ring", "orbit"]),
    "OSPREY":    ("Amp->Morph",       0.61, "ocean hunt sweeps coastal shore blend",        ["shore", "coast"]),
    "OSTERIA":   ("Env->Pitch",       0.58, "deep current stirs cellar warmth",             ["warmth", "wood"]),
}


def build_orca_others(rng: random.Random) -> list[dict]:
    """12 presets — one per partner engine, ORCA as source."""
    presets = []
    orca_vocab = VOCAB["ORCA"]
    for i, partner in enumerate(_OTHER_12):
        ctype, base_amt, desc_frag, extra_tags = _ORCA_OTHER_CFG[partner]
        partner_words = PARTNER_WORDS[partner]
        seed = rng.randint(0, 99999)
        w1 = orca_vocab[i % len(orca_vocab)]
        w2 = partner_words[i % len(partner_words)]
        name = f"{w1} {w2}"
        amt = round(base_amt + rng.uniform(-0.05, 0.05), 2)
        base_dna = blend_dna("ORCA", partner, weights=[0.55, 0.45])
        dna = nudge(base_dna, seed)
        desc = (
            f"ORCA×{partner} — {desc_frag}. "
            f"{prefix('ORCA')}huntMacro drives {prefix(partner)}filter."
        )
        tags = ["orca", partner.lower()] + extra_tags
        presets.append(make_preset(name, desc, tags, ["ORCA", partner], ctype, amt, dna))
    return presets


# ---------- OCTOPUS × 12 other engines ----------
_OCTOPUS_OTHER_CFG: dict[str, tuple[str, float, str, list[str]]] = {
    "ORACLE":    ("Env->Filter",      0.64, "arm pattern reveals oracle fracture",          ["vision", "pattern"]),
    "OBSIDIAN":  ("Amp->Morph",       0.70, "ink cloud darkens obsidian edge",              ["dark", "ink"]),
    "OUROBOROS": ("Audio->FM",        0.68, "eight arms feed ouroboros chaos loop",         ["chaos", "arms"]),
    "OPTIC":     ("Amp->Drive",       0.66, "chromatophore flash drives optic beam",        ["beam", "flash"]),
    "OBLIQUE":   ("Audio->Wavetable", 0.63, "mimic signal bounces through prism",           ["mimic", "prism"]),
    "ONSET":     ("Rhythm->Blend",    0.72, "arm pulse locks rhythmic strike pattern",      ["percussion", "pulse"]),
    "OVERDUB":   ("Env->Decay",       0.62, "ink veil echoes through tape ghost",           ["tape", "ink"]),
    "OPAL":      ("Amp->Filter",      0.60, "scatter signal dissolves into opal grain",     ["grain", "scatter"]),
    "ORGANON":   ("Audio->FM",        0.69, "decentralised arms modulate organon cells",    ["cell", "biology"]),
    "ORBITAL":   ("Rhythm->Gate",     0.65, "arm signal gates orbital ring spin",           ["ring", "orbit"]),
    "OSPREY":    ("Env->Morph",       0.61, "chromatic shift morphs shore coastline",       ["shore", "morph"]),
    "OSTERIA":   ("Amp->Morph",       0.59, "signal glow warms cellar resonance",           ["warmth", "glow"]),
}


def build_octopus_others(rng: random.Random) -> list[dict]:
    """12 presets — one per partner engine, OCTOPUS as source."""
    presets = []
    octo_vocab = VOCAB["OCTOPUS"]
    for i, partner in enumerate(_OTHER_12):
        ctype, base_amt, desc_frag, extra_tags = _OCTOPUS_OTHER_CFG[partner]
        partner_words = PARTNER_WORDS[partner]
        seed = rng.randint(0, 99999)
        w1 = octo_vocab[i % len(octo_vocab)]
        w2 = partner_words[i % len(partner_words)]
        name = f"{w1} {w2}"
        amt = round(base_amt + rng.uniform(-0.05, 0.05), 2)
        base_dna = blend_dna("OCTOPUS", partner, weights=[0.55, 0.45])
        dna = nudge(base_dna, seed)
        desc = (
            f"OCTOPUS×{partner} — {desc_frag}. "
            f"{prefix('OCTOPUS')}armDepth modulates {prefix(partner)}filter."
        )
        tags = ["octopus", partner.lower()] + extra_tags
        presets.append(make_preset(name, desc, tags, ["OCTOPUS", partner], ctype, amt, dna))
    return presets


# ---------------------------------------------------------------------------
# I/O
# ---------------------------------------------------------------------------

def write_preset(preset: dict, output_dir: str, dry_run: bool) -> tuple[Path, bool]:
    mood_dir = Path(output_dir) / "XOmnibus" / preset["mood"]
    filepath = mood_dir / (preset["name"] + ".xometa")
    if dry_run:
        return filepath, False
    mood_dir.mkdir(parents=True, exist_ok=True)
    with open(filepath, "w") as f:
        json.dump(preset, f, indent=2)
        f.write("\n")
    return filepath, True


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main() -> int:
    parser = argparse.ArgumentParser(
        description="Generate ORCA × OCTOPUS × OMBRE Entangled coupling presets for XOmnibus."
    )
    default_out = Path(__file__).resolve().parent.parent / "Presets"
    parser.add_argument(
        "--output-dir",
        default=str(default_out),
        help="Root Presets directory (default: ../Presets relative to this script)",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print preset names without writing files",
    )
    parser.add_argument(
        "--seed",
        type=int,
        default=42,
        help="Random seed for deterministic variant generation (default: 42)",
    )
    args = parser.parse_args()

    rng = random.Random(args.seed)

    all_presets = (
        build_trio_marquee(rng)           #  6 — three-way marquee
        + build_orca_octopus(rng)         # 12 — ORCA × OCTOPUS
        + build_orca_ombre(rng)           # 12 — ORCA × OMBRE
        + build_octopus_ombre(rng)        # 12 — OCTOPUS × OMBRE
        + build_orca_others(rng)          # 12 — ORCA × other 12
        + build_octopus_others(rng)       # 12 — OCTOPUS × other 12
    )

    # Deduplicate names: append roman suffix on collision
    seen: dict[str, int] = {}
    roman = ["", " II", " III", " IV", " V", " VI"]
    for preset in all_presets:
        base = preset["name"]
        count = seen.get(base, 0)
        if count > 0:
            preset["name"] = base + roman[min(count, len(roman) - 1)]
        seen[base] = count + 1

    written = 0
    for preset in all_presets:
        filepath, did_write = write_preset(preset, args.output_dir, args.dry_run)
        if args.dry_run:
            eng_str = "+".join(preset["engines"])
            print(f"  [dry-run]  {preset['mood']}/{preset['name']}.xometa  [{eng_str}]")
        else:
            print(f"  wrote      {filepath}")
            written += 1

    total = len(all_presets)
    if args.dry_run:
        print(f"\nDry run complete — {total} presets listed (0 files written).")
    else:
        print(f"\nDone — {written}/{total} presets written to {args.output_dir}/XOmnibus/Entangled/")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
